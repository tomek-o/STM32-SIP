/**
 * @file audio_adc.c  ADC sound driver
 *
 */
#include <string.h>
#include <re.h>
#include <rem.h>
#include <baresip.h>
#include "softphone/inc/adc.h"


#define DEBUG_MODULE "audio_adc"
#define DEBUG_LEVEL 5
#include <re_dbg.h>


struct ausrc_st {
	struct ausrc *as;      /* inheritance */
	ausrc_read_h *rh;
	void *arg;
	volatile bool ready;
};

static struct ausrc *ausrc;

static void adc_cb(const int16_t *samples, unsigned int samples_count, void *arg)
{
	struct ausrc_st *st = arg;
    if (st->ready)
        st->rh((uint8_t*)samples, 2*samples_count, st->arg);
}

static void ausrc_destructor(void *arg)
{
	struct ausrc_st *st = arg;

	st->ready = false;
	adc_client_unregister(st);

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

	err = adc_client_register(adc_cb, prm->srate, st);
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

static int audio_adc_init(void)
{
	int err = ausrc_register(&ausrc, "audio_adc", src_alloc);
	return err;
}

static int audio_adc_close(void)
{
	ausrc = mem_deref(ausrc);
	return 0;
}


EXPORT_SYM const struct mod_export DECL_EXPORTS(audio_adc) = {
	"audio_adc",
	"sound",
	audio_adc_init,
	audio_adc_close
};
