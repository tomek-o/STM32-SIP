/**
 * @file
 * Null audio driver - input device generating just silence.
 */
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "nullaudio_no_thread_internal.h"
#include <string.h>


#define DEBUG_MODULE "nullaudio_no_thread"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct ausrc_st {
	struct ausrc *as;  /* base class / inheritance */
	uint32_t ptime;
	size_t sampc;
	bool run;
	bool terminated;
	ausrc_read_h *rh;
	ausrc_error_h *errh;
	int16_t *sampv;
	uint64_t ts;
	void *arg;
};


static void ausrc_destructor(void *arg)
{
	struct ausrc_st *st = arg;

	if (st->run) {
		st->run = false;
		/** \note No hazard risk on STM32 if both SIP and media are running in the same task */
		nullaudio_no_thread_unregister(st);
	}

	mem_deref(st->as);
	mem_deref(st->sampv);
}

static void on_poll(void *arg)
{
	struct ausrc_st *st = arg;
	uint64_t now = tmr_jiffies();

    if (!st->run)
        return;

    if (st->ts > now)
        return;

    // returning zero samples
    memset(st->sampv, 0, st->sampc * 2);
    st->rh((uint8_t *)st->sampv, st->sampc*sizeof(int16_t), st->arg);

    st->ts += st->ptime;
}

int nullaudio_no_thread_src_alloc(struct ausrc_st **stp, struct ausrc *as,
		      struct media_ctx **ctx,
		      struct ausrc_prm *prm, const char *device,
		      ausrc_read_h *rh, ausrc_error_h *errh, void *arg)
{
	struct ausrc_st *st;
	int err = 0;

	(void)ctx;
	(void)device;
	(void)errh;

	if (!stp || !as || !prm)
		return EINVAL;

	if (prm->srate == 0 || prm->ch == 0 || prm->frame_size == 0) {
		DEBUG_INFO("nullaudio src: invalid/unitialized audio prm\n");
		return EINVAL;
	}

	st = mem_zalloc(sizeof(*st), ausrc_destructor);
	if (!st)
		return ENOMEM;

	st->as  = mem_ref(as);
	st->rh  = rh;
	st->arg = arg;

	prm->fmt = AUFMT_S16LE;

	st->sampc = prm->frame_size; //prm->srate * prm->ch * prm->ptime / 1000;

	st->ptime = prm->frame_size * 1000 / prm->srate / prm->ch; //prm->ptime;

	st->sampv = mem_alloc(st->sampc * 2, NULL);
	if (!st->sampv) {
		err = ENOMEM;
		goto out;
	}

	DEBUG_INFO("nullaudio src: audio ptime=%u sampc=%zu\n", st->ptime, st->sampc);

    if (nullaudio_no_thread_register(on_poll, st)) {
		err = ENOMEM;
		goto out;
	}
	st->run = true;


out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}
