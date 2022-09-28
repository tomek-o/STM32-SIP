/*
 * SpanDSP - a series of DSP components for telephony
 *
 * g726.h - ITU G.726 codec.
 *
 * Written by Steve Underwood <steveu@coppice.org>
 *
 * Copyright (C) 2006 Steve Underwood
 *
 *  Despite my general liking of the GPL, I place my own contributions 
 *  to this code in the public domain for the benefit of all mankind -
 *  even the slimy ones who might try to proprietize my work and use it
 *  to my detriment.
 *
 * Based on G.721/G.723 code which is:
 *
 * This source code is a product of Sun Microsystems, Inc. and is provided
 * for unrestricted use.  Users may copy or modify this source code without
 * charge.
 *
 * SUN SOURCE CODE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING
 * THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun source code is provided with no support and without any obligation on
 * the part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS SOFTWARE
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * $Id: g726.h,v 1.8 2006/09/03 04:43:46 steveu Exp $
 */

/*! \file */

#if !defined(_G726_H_)
#define _G726_H_

/*! \page g726_page G.726 encoding and decoding
\section g726_page_sec_1 What does it do?

The G.726 module is a bit exact implementation of the full ITU G.726 specification.
It supports:
    - 16 kbps, 24kbps, 32kbps, and 40kbps operation.
    - Tandem adjustment, for interworking with A-law and u-law.
    - Annex A support, for use in environments not using A-law or u-law.

It passes the ITU tests.

\section g726_page_sec_2 How does it work?
???.
*/

#include <stdint.h>

enum
{
    G726_PACKING_NONE = 0,
    G726_PACKING_LEFT = 1,
    G726_PACKING_RIGHT = 2
};

struct g726_state_s;

typedef int16_t (*g726_decoder_func_t)(struct g726_state_s *s, uint8_t code);

typedef uint8_t (*g726_encoder_func_t)(struct g726_state_s *s, int16_t amp);

/*
 * The following is the definition of the state structure
 * used by the G.726 encoder and decoder to preserve their internal
 * state between successive calls.  The meanings of the majority
 * of the state structure fields are explained in detail in the
 * CCITT Recommendation G.721.  The field names are essentially indentical
 * to variable names in the bit level description of the coding algorithm
 * included in this Recommendation.
 */
typedef struct g726_state_s
{
    /*! The bit rate */
    int rate;
    /*! The number of bits per sample */
    int bits_per_sample;
    /*! One fo the G.726_PACKING_xxx options */
    int packing;

    /*! Locked or steady state step size multiplier. */
    int32_t yl;
    /*! Unlocked or non-steady state step size multiplier. */
    int16_t yu;
    /*! int16_t term energy estimate. */
    int16_t dms;
    /*! Long term energy estimate. */
    int16_t dml;
    /*! Linear weighting coefficient of 'yl' and 'yu'. */
    int16_t ap;
    
    /*! Coefficients of pole portion of prediction filter. */
    int16_t a[2];
    /*! Coefficients of zero portion of prediction filter. */
    int16_t b[6];
    /*! Signs of previous two samples of a partially reconstructed signal. */
    int16_t pk[2];
    /*! Previous 6 samples of the quantized difference signal represented in
        an internal floating point format. */
    int16_t dq[6];
    /*! Previous 2 samples of the quantized difference signal represented in an
        internal floating point format. */
    int16_t sr[2];
    /*! Delayed tone detect */
    int td;
    
    unsigned int in_buffer;
    int in_bits;
    unsigned int out_buffer;
    int out_bits;

    g726_encoder_func_t enc_func;
    g726_decoder_func_t dec_func;
} g726_state_t;

#ifdef __cplusplus
extern "C" {
#endif

/*! Initialise a G.726 encode or decode context.
    \param s The G.726 context.
    \param bit_rate The required bit rate for the ADPCM data.
           The valid rates are 16000, 24000, 32000 and 40000.
	\param packing One of the G.726_PACKING_xxx options.
    \return A pointer to the G.726 context, or NULL for error. */
g726_state_t *g726_init(g726_state_t *s, int bit_rate, int packing);

/*! Free a G.726 encode or decode context.
    \param s The G.726 context.
    \return 0 for OK. */
int g726_release(g726_state_t *s);

/*! Decode a buffer of G.726 ADPCM data to linear PCM, a-law or u-law.
    \param s The G.726 context.
    \param amp
    \param g726_data
    \param g726_bytes
    \return The number of samples returned. */
int g726_decode(g726_state_t *s,
                int16_t amp[],
                const uint8_t g726_data[],
                int g726_bytes);

/*! Encode a buffer of linear PCM data to G.726 ADPCM.
    \param s The G.726 context.
    \param g726_data
    \param amp
    \param samples
    \return The number of bytes of G.726 data produced. */
int g726_encode(g726_state_t *s,
                uint8_t g726_data[],
                const int16_t amp[],
                int samples);

#ifdef __cplusplus
}
#endif

#endif
/*- End of file ------------------------------------------------------------*/
