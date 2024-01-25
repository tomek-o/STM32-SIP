/**
 * @file audio_dac.c  DAC sound driver, working also as audio loopback source
 *
 */
#include <string.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "softphone/inc/dac.h"


#define DEBUG_MODULE "audio_dac"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct auplay_st {
	struct auplay *ap;      /* inheritance */
	auplay_write_h *wh;
	void *arg;
	volatile bool ready;
};

static struct auplay *auplay;

static void dac_cb(int16_t *samples, unsigned int samples_count, void *arg)
{
	struct auplay_st *st = arg;
    if (st->ready)
        st->wh((uint8_t*)samples, 2*samples_count, st->arg);
}

static void auplay_destructor(void *arg)
{
	struct auplay_st *st = arg;

	st->ready = false;
	dac_client_unregister(st);

	mem_deref(st->ap);
}

static int play_alloc(struct auplay_st **stp, struct auplay *ap,
		      struct auplay_prm *prm, const char *device,
		      auplay_write_h *wh, void *arg)
{
	struct auplay_st *st;
	int err;

	(void)device;

	prm->fmt = AUFMT_S16LE;

	st = mem_zalloc(sizeof(*st), auplay_destructor);
	if (!st)
		return ENOMEM;

	st->ap  = mem_ref(ap);
	st->wh  = wh;
	st->arg = arg;

	err = dac_client_register(dac_cb, prm->srate, st);
	if (err)
		goto out;

	st->ready = true;

 out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}


struct ausrc_st {
	struct ausrc *as;      /* inheritance */
	ausrc_read_h *rh;
	void *arg;
	volatile bool ready;
};

static struct ausrc *ausrc;     /* loopback */

static void dac_loopback_cb(const int16_t *samples, unsigned int samples_count, void *arg)
{
	struct ausrc_st *st = arg;
    if (st->ready)
        st->rh((uint8_t*)samples, 2*samples_count, st->arg);
}

static void ausrc_destructor(void *arg)
{
	struct ausrc_st *st = arg;

	st->ready = false;
	dac_loopback_client_unregister(st);

	mem_deref(st->as);
}

static int src_alloc(struct ausrc_st **stp, struct ausrc *as,
                    struct media_ctx **ctx,
                    struct ausrc_prm *prm, const char *device,
                    ausrc_read_h *rh, ausrc_error_h *errh, void *arg)
{
	struct ausrc_st *st;
	int err;

	(void)device;

	prm->fmt = AUFMT_S16LE;

	st = mem_zalloc(sizeof(*st), ausrc_destructor);
	if (!st)
		return ENOMEM;

	st->as  = mem_ref(as);
	st->rh  = rh;
	st->arg = arg;

	err = dac_loopback_client_register(dac_loopback_cb, prm->srate, st);
	if (err)
		goto out;

	st->ready = true;

 out:
	if (err)
		mem_deref(st);
	else
		*stp = st;

	return err;
}



static int audio_dac_init(void)
{
	int err = auplay_register(&auplay, "audio_dac", play_alloc);
	if (err)
        return err;
    err = ausrc_register(&ausrc, "audio_dac", src_alloc);
    return err;
}

static int audio_dac_close(void)
{
	auplay = mem_deref(auplay);
	ausrc = mem_deref(ausrc);
	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(audio_dac) = {
	"audio_dac",
	"sound",
	audio_dac_init,
	audio_dac_close
};
