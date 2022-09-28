/**
 * @file opus/decode.c Opus Decode
 *
 * Copyright (C) 2010 Creytiv.com
 */

#include <re.h>
#include <baresip.h>
#include <opus.h>
#include "opus_priv.h"

#define DEBUG_MODULE "opus"
#define DEBUG_LEVEL 5
#include <re_dbg.h>

struct audec_state {
	OpusDecoder *dec;
	unsigned ch;
};


static void destructor(void *arg)
{
	struct audec_state *ads = arg;

	if (ads->dec)
		opus_decoder_destroy(ads->dec);
}


int opus_decode_update(struct audec_state **adsp, const struct aucodec *ac,
		       const char *fmtp)
{
	struct audec_state *ads;
	int opuserr, err = 0;
	(void)fmtp;

	if (!adsp || !ac || !ac->ch)
		return EINVAL;

	ads = *adsp;

	if (ads)
		return 0;

	ads = mem_zalloc(sizeof(*ads), destructor);
	if (!ads)
		return ENOMEM;

	ads->ch = ac->ch;

	ads->dec = opus_decoder_create(ac->srate, ac->ch, &opuserr);
	if (!ads->dec) {
		DEBUG_WARNING("opus: decoder create: %s\n", opus_strerror(opuserr));
		err = ENOMEM;
		goto out;
	}

 out:
	if (err)
		mem_deref(ads);
	else
		*adsp = ads;

	return err;
}


int opus_decode_frm(struct audec_state *ads, int16_t *sampv, size_t *sampc,
		    const uint8_t *buf, size_t len)
{
	int n;

	if (!ads || !sampv || !sampc || !buf)
		return EINVAL;

	n = opus_decode(ads->dec, buf, (opus_int32)len,
			sampv, (int)(*sampc/ads->ch), 0);
	if (n < 0) {
		DEBUG_WARNING("opus: decode error: %s\n", opus_strerror(n));
		return EPROTO;
	}

	*sampc = n * ads->ch;

	return 0;
}


int opus_decode_pkloss(struct audec_state *ads, int16_t *sampv, size_t *sampc)
{
	int n;

	if (!ads || !sampv || !sampc)
		return EINVAL;

	n = opus_decode(ads->dec, NULL, 0, sampv, (int)(*sampc/ads->ch), 0);
	if (n < 0)
		return EPROTO;

	*sampc = n * ads->ch;

	return 0;
}
