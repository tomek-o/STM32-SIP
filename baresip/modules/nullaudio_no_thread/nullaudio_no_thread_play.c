/**
 * @file
 * Null audio - playback.
 */
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "nullaudio_no_thread_internal.h"
#include <string.h>

#define DEBUG_MODULE "nullaudio_no_thread"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct auplay_st {
	struct auplay *ap;      /* inheritance */
	uint32_t ptime;
	size_t sampc;
	volatile bool run;      /* \note might not work with out-of-order CPU */
	auplay_write_h *wh;
	int16_t *sampv;
	uint64_t ts;
	void *arg;
};


static void auplay_destructor(void *arg)
{
	struct auplay_st *st = arg;

	if (st->run) {
		st->run = false;
		/** \note No hazard risk on STM32 if both SIP and media are running in the same task */
		nullaudio_no_thread_unregister(st);
	}

	mem_deref(st->ap);
	mem_deref(st->sampv);
}

static void on_poll(void *arg)
{
	struct auplay_st *st = arg;
	uint64_t now = tmr_jiffies();

	if (!st->run)
	    return;

    if (st->ts > now)
        return;

	if (st->wh) {
		st->wh((uint8_t*)st->sampv, st->sampc*sizeof(int16_t), st->arg);
	}

	st->ts += st->ptime;
}

int nullaudio_no_thread_play_alloc(struct auplay_st **stp, struct auplay *ap,
		       struct auplay_prm *prm, const char *device,
		       auplay_write_h *wh, void *arg)
{
	struct auplay_st *st;
	int err = 0;
	(void)device;

	if (!stp || !ap || !prm)
		return EINVAL;

	st = mem_zalloc(sizeof(*st), auplay_destructor);
	if (!st)
		return ENOMEM;

	st->ap  = mem_ref(ap);
	st->wh  = wh;
	st->arg = arg;

	prm->fmt = AUFMT_S16LE;


	st->sampc = prm->frame_size; //prm->srate * prm->ch * prm->ptime / 1000;

	st->ptime = prm->frame_size * 1000 / prm->srate / prm->ch; //prm->ptime;

	st->sampv = mem_alloc(st->sampc * 2, NULL);
	if (!st->sampv) {
        err = ENOMEM;
		goto out;
	}
	memset(st->sampv, 0, st->sampc * 2);

	DEBUG_INFO("nullaudio play: audio ptime=%u sampc=%zu\n", st->ptime, st->sampc);

    if (nullaudio_no_thread_register(on_poll, st)) {
		err = ENOMEM;
		goto out;
	}

    st->ts = tmr_jiffies();
	st->run = true;

 out:
	if (err) {
		mem_deref(st);
	} else
		*stp = st;

	return err;
}
