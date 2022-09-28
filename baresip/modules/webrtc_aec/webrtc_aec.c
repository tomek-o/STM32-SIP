/**
 * @file webrtc_aec.c  WebRTC Acoustic Echo Cancellation
 *
 * Copyright (C) 2014 tomeko.net
 */
#include <stdlib.h>
#include <string.h>
#include <re.h>
#include <baresip.h>

#define WEBRTC_USE_NS 0	///< use noise suppression
#include <webrtc/modules/audio_processing/aec/include/echo_cancellation.h>
#include <webrtc/modules/audio_processing/ns/include/noise_suppression_x.h>


#define DEBUG_MODULE "webrtc_aec"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct webrtc_st {
	uint32_t nsamp;
	int16_t *out;

	void*		AEC_inst;
	NsxHandle*  NS_inst;
	int16_t* 	tmp_frame;
	int16_t* 	tmp_frame2;

	int msInSndCardBuf;
	int skew;
};

struct enc_st {
	struct aufilt_enc_st af;  /* base class */
	struct webrtc_st *st;
};

struct dec_st {
	struct aufilt_dec_st af;  /* base class */
	struct webrtc_st *st;
};

static void print_webrtc_aec_error(const char* tag, void *AEC_inst) {
	unsigned status = WebRtcAec_get_error_code(AEC_inst);
	DEBUG_WARNING("WebRTC AEC ERROR (%s) %d\n", tag, status);
}

static void enc_destructor(void *arg)
{
	struct enc_st *st = arg;

	list_unlink(&st->af.le);
	mem_deref(st->st);
}


static void dec_destructor(void *arg)
{
	struct dec_st *st = arg;

	list_unlink(&st->af.le);
	mem_deref(st->st);
}

static void webrtc_aec_destructor(void *arg)
{
	struct webrtc_st *st = arg;

	if (st->AEC_inst) {
		WebRtcAec_Free(st->AEC_inst);
		st->AEC_inst = NULL;
	}
#if WEBRTC_USE_NS == 1
	if (st->NS_inst) {
		WebRtcNsx_Free(st->NS_inst);
		st->NS_inst = NULL;
	}
#endif

	mem_deref(st->out);
	mem_deref(st->tmp_frame);
	mem_deref(st->tmp_frame2);
}


static int aec_alloc(struct webrtc_st **stp, void **ctx, struct aufilt_prm *prm)
{
	struct webrtc_st *st;
	int err = 0, tmp;
	int status;
	const struct config *cfg = conf_config();

	if (!stp || !ctx || !prm)
		return EINVAL;

	{
		int nsamp = prm->ch * prm->frame_size;
		if (nsamp != 80 && nsamp != 160) {
			/** \todo For 16kHz sampling frame size = 320, unsupported by WebRTC.
				Additional framing required.
			*/
			DEBUG_WARNING("WebRTC AEC disabled - unsupported frame length\n");
			return EFAULT;        
        }
    }

	if (*ctx) {
		*stp = mem_ref(*ctx);
		return 0;
	}

	st = mem_zalloc(sizeof(*st), webrtc_aec_destructor);
	if (!st)
		return ENOMEM;

	st->nsamp = prm->ch * prm->frame_size;
	st->msInSndCardBuf = cfg->webrtc.msInSndCardBuf;
	st->skew = cfg->webrtc.skew;

	st->out = mem_alloc(2 * st->nsamp, NULL);
	if (!st->out) {
		err = ENOMEM;
		goto out;
	}

	status = WebRtcAec_Create(&st->AEC_inst);
    if(status){
		err = ENOMEM;
		goto out;
	}
	status = WebRtcAec_Init(st->AEC_inst, prm->srate, prm->srate);
	if(status != 0) {
		print_webrtc_aec_error("WebRtcAec_Init", st->AEC_inst);
		err = EFAULT;
		goto out;
    }
	/*
    AecConfig aec_config;
	aec_config.nlpMode = ...
    aec_config.skewMode = kAecTrue;
    aec_config.metricsMode = kAecFalse;
    aec_config.delay_logging = kAecFalse;
	status = WebRtcAec_set_config(echo->AEC_inst, aec_config);
    if(status != 0) {
         ...
    }
	*/

#if WEBRTC_USE_NS == 1
	status = WebRtcNsx_Create(&st->NS_inst);
    if(status != 0) {
		err = ENOMEM;
		goto out;
	}

	status = WebRtcNsx_Init(st->NS_inst, prm->srate);
	if(status != 0) {
		DEBUG_WARNING("Could not init noise suppressor");
		err = EFAULT;
		goto out;
	}

	status = WebRtcNsx_set_policy(st->NS_inst, 0);
	if (status != 0) {
		DEBUG_WARNING("Could not set noise suppressor policy");
	}
#endif

	st->tmp_frame = mem_zalloc(2 * st->nsamp, NULL);
	st->tmp_frame2 = mem_zalloc(2 * st->nsamp, NULL);
	if (!st->tmp_frame || !st->tmp_frame2) {
		err = ENOMEM;
		goto out;
	}
	DEBUG_NOTICE("WebRTC AEC loaded: enc=%uHz, msInSndCardBuf=%d, skew=%d\n",
		prm->srate, st->msInSndCardBuf, st->skew);

out:
	if (err) {
		if (st->AEC_inst) {
			WebRtcAec_Free(st->AEC_inst);
			st->AEC_inst = NULL;
		}
#if WEBRTC_USE_NS == 1
		if (st->NS_inst) {
			WebRtcNsx_Free(st->NS_inst);
			st->NS_inst = NULL;
		}
#endif		
		mem_deref(st);
	} else {
		*ctx = *stp = st;
	}

	return err;
}


static int encode_update(struct aufilt_enc_st **stp, void **ctx,
			 const struct aufilt *af, struct aufilt_prm *prm)
{
	struct enc_st *st;
	int err;

	if (!stp || !ctx || !af || !prm)
		return EINVAL;

	if (*stp)
		return 0;

	st = mem_zalloc(sizeof(*st), enc_destructor);
	if (!st)
		return ENOMEM;

	err = aec_alloc(&st->st, ctx, prm);

	if (err)
		mem_deref(st);
	else
		*stp = (struct aufilt_enc_st *)st;

	return err;
}


static int decode_update(struct aufilt_dec_st **stp, void **ctx,
			 const struct aufilt *af, struct aufilt_prm *prm)
{
	struct dec_st *st;
	int err;

	if (!stp || !ctx || !af || !prm)
		return EINVAL;

	if (*stp)
		return 0;

	st = mem_zalloc(sizeof(*st), dec_destructor);
	if (!st)
		return ENOMEM;

	err = aec_alloc(&st->st, ctx, prm);

	if (err)
		mem_deref(st);
	else
		*stp = (struct aufilt_dec_st *)st;

	return err;
}


static int encode(struct aufilt_enc_st *st, int16_t *sampv, size_t *sampc)
{
	struct enc_st *est = (struct enc_st *)st;
	struct webrtc_st *wr = est->st;
	int status;

	if (*sampc == wr->nsamp) {
		// cancel echo (modify samples)
#if WEBRTC_USE_NS == 1
		if(wr->NS_inst){
			/* Noise suppression */
			status = WebRtcNsx_Process(
				wr->NS_inst,
				sampv, NULL,
				wr->tmp_frame, NULL);
		}
#endif
		/* Process echo cancellation */
		status = WebRtcAec_Process(
			wr->AEC_inst,
			(wr->NS_inst)?wr->tmp_frame:sampv, NULL,
			wr->tmp_frame2, NULL,
			wr->nsamp,
			wr->msInSndCardBuf,   // 40: PortAudio/DS (60ms/60ms), 120: winwave
			wr->skew);
		if(status != 0){
			print_webrtc_aec_error("Process echo", wr->AEC_inst);
			return status;
		}
		/* Copy temporary buffer back to original */
		memcpy(sampv, wr->tmp_frame2, 2 * wr->nsamp);
	} else if (*sampc) {
		static int unexpected = 0;
		unexpected++;
	}
	return 0;
}


static int decode(struct aufilt_dec_st *st, int16_t *sampv, size_t *sampc)
{
	struct dec_st *dst = (struct dec_st *)st;
	struct webrtc_st *wr = dst->st;
	int status = 0;
	if (*sampc == wr->nsamp) {
		status = WebRtcAec_BufferFarend(wr->AEC_inst, sampv, wr->nsamp);
		if(status != 0) {
			print_webrtc_aec_error("WebRtcAec_BufferFarend", wr->AEC_inst);
		}
	} else if (*sampc) {
		static int unexpected = 0;
		unexpected++;
	}
	return status;
}

static struct aufilt webrtc_aec = {
	LE_INIT, "webrtc_aec", encode_update, encode, decode_update, decode
};

static int module_init(void) {
	aufilt_register(&webrtc_aec);
	return 0;
}

static int module_close(void) {
	aufilt_unregister(&webrtc_aec);
	return 0;
}  

EXPORT_SYM const struct mod_export DECL_EXPORTS(webrtc_aec) = {
	"webrtc_aec",
	"filter",
	module_init,
	module_close
};


