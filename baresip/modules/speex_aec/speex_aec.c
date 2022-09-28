/**
 * @file speex_aec.c  Speex Acoustic Echo Cancellation
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <stdlib.h>
#include <string.h>
#include <speex/speex.h>
#include <speex/speex_echo.h>
#include <re.h>
#include <baresip.h>


#define DEBUG_MODULE "speex_aec"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct speex_st {
	uint32_t nsamp;
	int16_t *out;
	SpeexEchoState *state;
};

struct enc_st {
	struct aufilt_enc_st af;  /* base class */
	struct speex_st *st;
};

struct dec_st {
	struct aufilt_dec_st af;  /* base class */
	struct speex_st *st;
};


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


#ifdef SPEEX_SET_VBR_MAX_BITRATE
static void speex_aec_destructor(void *arg)
{
	struct speex_st *st = arg;

	if (st->state)
		speex_echo_state_destroy(st->state);

	mem_deref(st->out);
}


static int aec_alloc(struct speex_st **stp, void **ctx, struct aufilt_prm *prm)
{
	struct speex_st *st;
	int err, tmp, fl;

	if (!stp || !ctx || !prm)
		return EINVAL;

	if (*ctx) {
		*stp = mem_ref(*ctx);
		return 0;
	}

	st = mem_zalloc(sizeof(*st), speex_aec_destructor);
	if (!st)
		return ENOMEM;

	st->nsamp = prm->ch * prm->frame_size;

	st->out = mem_alloc(2 * st->nsamp, NULL);
	if (!st->out) {
		err = ENOMEM;
		goto out;
	}

	/* Echo canceller with 200 ms tail length */
	fl = 10 * prm->frame_size;
	st->state = speex_echo_state_init(prm->frame_size, fl);
	if (!st->state) {
		err = ENOMEM;
		goto out;
	}

	tmp = prm->srate;
	err = speex_echo_ctl(st->state, SPEEX_ECHO_SET_SAMPLING_RATE, &tmp);
	if (err < 0) {
		DEBUG_WARNING("speex_echo_ctl: err=%d\n", err);
	}

	DEBUG_NOTICE("Speex AEC loaded: enc=%uHz\n", prm->srate);

 out:
	if (err)
		mem_deref(st);
	else
		*ctx = *stp = st;

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
	struct speex_st *sp = est->st;

	if (*sampc) {
		speex_echo_capture(sp->state, sampv, sp->out);
		memcpy(sampv, sp->out, 2 * sp->nsamp);
	}

	return 0;
}


static int decode(struct aufilt_dec_st *st, int16_t *sampv, size_t *sampc)
{
	struct dec_st *dst = (struct dec_st *)st;
	struct speex_st *sp = dst->st;

	if (*sampc)
		speex_echo_playback(sp->state, sampv);

	return 0;
}
#endif


static struct aufilt speex_aec = {
	LE_INIT, "speex_aec", encode_update, encode, decode_update, decode
};


static int module_init(void)
{
	/* Note: Hack to check libspeex version */
#ifdef SPEEX_SET_VBR_MAX_BITRATE
	aufilt_register(&speex_aec);
	return 0;
#else
	return ENOSYS;
#endif
}


static int module_close(void)
{
	aufilt_unregister(&speex_aec);
	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(speex_aec) = {
	"speex_aec",
	"filter",
	module_init,
	module_close
};
