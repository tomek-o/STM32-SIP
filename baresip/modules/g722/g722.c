/**
 * @file g722.c  G.722 audio codec
 *
 * Copyright (C) 2010 Creytiv.com
 */
#include <stdlib.h>
#include <string.h>
#include <re.h>
#include <baresip.h>
#include <g722.h>
#include <g722_private.h>
#include <g722_encoder.h>
#include <g722_decoder.h>
#include <assert.h>


/**
 * @defgroup g722 g722
 *
 * The G.722 audio codec
 *
 * ## From RFC 3551:

 4.5.2 G722

   G722 is specified in ITU-T Recommendation G.722, "7 kHz audio-coding
   within 64 kbit/s".  The G.722 encoder produces a stream of octets,
   each of which SHALL be octet-aligned in an RTP packet.  The first bit
   transmitted in the G.722 octet, which is the most significant bit of
   the higher sub-band sample, SHALL correspond to the most significant
   bit of the octet in the RTP packet.

   Even though the actual sampling rate for G.722 audio is 16,000 Hz,
   the RTP clock rate for the G722 payload format is 8,000 Hz because
   that value was erroneously assigned in RFC 1890 and must remain
   unchanged for backward compatibility.  The octet rate or sample-pair
   rate is 8,000 Hz.

   ##   Reference:

  http://www.soft-switch.org/spandsp-modules.html

 */


enum {
	G722_SAMPLE_RATE = 16000,
	G722_BITRATE_48k = 48000,
	G722_BITRATE_56k = 56000,
	G722_BITRATE_64k = 64000
};


struct auenc_state {
	struct g722_encode_state enc;
};

struct audec_state {
	struct g722_decode_state dec;
};


static int encode_update(struct auenc_state **aesp,
			 const struct aucodec *ac,
			 struct auenc_param *prm, const char *fmtp)
{
	struct auenc_state *st;
	int err = 0;
	(void)prm;
	(void)fmtp;

	if (!aesp || !ac)
		return EINVAL;

	if (*aesp)
		return 0;

	st = mem_alloc(sizeof(*st), NULL);
	if (!st)
		return ENOMEM;

	if (g722_encoder_init(&st->enc, G722_BITRATE_64k, 0) != 0) {
		err = EPROTO;
		goto out;
	}

 out:
	if (err)
		mem_deref(st);
	else
		*aesp = st;

	return err;
}


static int decode_update(struct audec_state **adsp,
			 const struct aucodec *ac, const char *fmtp)
{
	struct audec_state *st;
	int err = 0;
	(void)fmtp;

	if (!adsp || !ac)
		return EINVAL;

	if (*adsp)
		return 0;

	st = mem_alloc(sizeof(*st), NULL);
	if (!st)
		return ENOMEM;

	if (g722_decoder_init(&st->dec, G722_BITRATE_64k, 0) != 0) {
		err = EPROTO;
		goto out;
	}

 out:
	if (err)
		mem_deref(st);
	else
		*adsp = st;

	return err;
}


static int encode(struct auenc_state *st, uint8_t *buf, size_t *len,
		  const int16_t *sampv, size_t sampc)
{
	int n;

	n = g722_encode(&st->enc, sampv, (int)sampc, buf);
	assert(n < *len);
	if (n <= 0) {
		return EPROTO;
	}
	*len = n;

	return 0;
}


static int decode(struct audec_state *st, int16_t *sampv, size_t *sampc,
		  const uint8_t *buf, size_t len)
{
	int n;

	if (!st || !sampv || !buf)
		return EINVAL;

	n = g722_decode(&st->dec, buf, len, sampv);
	if (n < 0)
		return EPROTO;

	*sampc = n;

	return 0;
}


static struct aucodec g722 = {
	LE_INIT, "9", "G722", 8000, 1, NULL,
	encode_update, encode,
	decode_update, decode, NULL,
	NULL, NULL
};


static int module_init(void)
{
	aucodec_register(&g722);
	return 0;
}


static int module_close(void)
{
	aucodec_unregister(&g722);
	return 0;
}


/** Module exports */
EXPORT_SYM const struct mod_export DECL_EXPORTS(g722) = {
	"g722",
	"codec",
	module_init,
	module_close
};
