////////////////////////////////////////////////////////////////////////////
//                           **** WAVPACK ****                            //
//                  Hybrid Lossless Wavefile Compressor                   //
//              Copyright (c) 1998 - 2006 Conifer Software.               //
//               MMX optimizations (c) 2006 Joachim Henke                 //
//                          All Rights Reserved.                          //
//      Distributed under the BSD Software License (see license.txt)      //
////////////////////////////////////////////////////////////////////////////

// extra2.c

// This module handles the "extra" mode for stereo files.

#include "wavpack_local.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define LOG_LIMIT 6912
//#define EXTRA_DUMP

#ifdef DEBUG_ALLOC
#define malloc malloc_db
#define realloc realloc_db
#define free free_db
void *malloc_db (uint32_t size);
void *realloc_db (void *ptr, uint32_t size);
void free_db (void *ptr);
int32_t dump_alloc (void);
#endif

//////////////////////////////// local tables ///////////////////////////////

typedef struct {
    int32_t *sampleptrs [MAX_NTERMS+2];
    struct decorr_pass dps [MAX_NTERMS];
    int nterms, log_limit, gt16bit;
    uint32_t best_bits;
} WavpackExtraInfo;

#ifdef OPT_MMX

static void decorr_stereo_pass (int32_t *in_samples, int32_t *out_samples, int32_t num_samples, struct decorr_pass *dpp, int dir)
{
    const __m64
        delta = _mm_set1_pi32 (dpp->delta),
        fill = _mm_set1_pi32 (0x7bff),
        mask = _mm_set1_pi32 (0x7fff),
        round = _mm_set1_pi32 (512),
        zero = _mm_set1_pi32 (0);
    __m64
        sum_AB = zero,
        weight_AB = _mm_set_pi32 (restore_weight (store_weight (dpp->weight_B)), restore_weight (store_weight (dpp->weight_A))),
        left_right, sam_AB, tmp0, tmp1, samples_AB [MAX_TERM];
    int k, m = 0;

    if (dir < 0) {
        out_samples += (num_samples - 1) * 2;
        in_samples += (num_samples - 1) * 2;
        dir = -2;
    }
    else
        dir = 2;

    for (k = 0; k < MAX_TERM; ++k) {
        ((int32_t *) samples_AB) [k * 2] = exp2s (log2s (dpp->samples_A [k]));
        ((int32_t *) samples_AB) [k * 2 + 1] = exp2s (log2s (dpp->samples_B [k]));
    }

    if (dpp->term > 0) {
        if (dpp->term == 17) {
            while (num_samples--) {
                left_right = *(__m64 *) in_samples;
                tmp0 = samples_AB [0];
                sam_AB = _m_paddd (tmp0, tmp0);
                sam_AB = _m_psubd (sam_AB, samples_AB [1]);
                samples_AB [0] = left_right;
                samples_AB [1] = tmp0;

                tmp0 = _m_paddd (sam_AB, sam_AB);
                tmp1 = _m_pand (sam_AB, mask);
                tmp0 = _m_psrldi (tmp0, 16);
                tmp1 = _m_pmaddwd (tmp1, weight_AB);
                tmp0 = _m_pmaddwd (tmp0, weight_AB);
                tmp1 = _m_paddd (tmp1, round);
                tmp0 = _m_pslldi (tmp0, 5);
                tmp1 = _m_psradi (tmp1, 10);
                left_right = _m_psubd (left_right, tmp0);
                left_right = _m_psubd (left_right, tmp1);

                *(__m64 *) out_samples = left_right;

                tmp0 = _m_pxor (sam_AB, left_right);
                tmp0 = _m_psradi (tmp0, 31);
                tmp1 = _m_pxor (delta, tmp0);
                tmp1 = _m_psubd (tmp1, tmp0);
                sam_AB = _m_pcmpeqd (sam_AB, zero);
                tmp0 = _m_pcmpeqd (left_right, zero);
                tmp0 = _m_por (tmp0, sam_AB);
                tmp0 = _m_pandn (tmp0, tmp1);
                weight_AB = _m_paddd (weight_AB, tmp0);

                sum_AB = _m_paddd (sum_AB, weight_AB);

                in_samples += dir;
                out_samples += dir;
            }
        }
        else if (dpp->term == 18) {
            while (num_samples--) {
                left_right = *(__m64 *) in_samples;
                tmp0 = samples_AB [0];
                sam_AB = _m_psubd (tmp0, samples_AB [1]);
                sam_AB = _m_psradi (sam_AB, 1);
                sam_AB = _m_paddd (sam_AB, tmp0);
                samples_AB [0] = left_right;
                samples_AB [1] = tmp0;

                tmp0 = _m_paddd (sam_AB, sam_AB);
                tmp1 = _m_pand (sam_AB, mask);
                tmp0 = _m_psrldi (tmp0, 16);
                tmp1 = _m_pmaddwd (tmp1, weight_AB);
                tmp0 = _m_pmaddwd (tmp0, weight_AB);
                tmp1 = _m_paddd (tmp1, round);
                tmp0 = _m_pslldi (tmp0, 5);
                tmp1 = _m_psradi (tmp1, 10);
                left_right = _m_psubd (left_right, tmp0);
                left_right = _m_psubd (left_right, tmp1);

                *(__m64 *) out_samples = left_right;

                tmp0 = _m_pxor (sam_AB, left_right);
                tmp0 = _m_psradi (tmp0, 31);
                tmp1 = _m_pxor (delta, tmp0);
                tmp1 = _m_psubd (tmp1, tmp0);
                sam_AB = _m_pcmpeqd (sam_AB, zero);
                tmp0 = _m_pcmpeqd (left_right, zero);
                tmp0 = _m_por (tmp0, sam_AB);
                tmp0 = _m_pandn (tmp0, tmp1);
                weight_AB = _m_paddd (weight_AB, tmp0);

                sum_AB = _m_paddd (sum_AB, weight_AB);

                in_samples += dir;
                out_samples += dir;
            }
        }
        else {
            k = dpp->term & (MAX_TERM - 1);
            while (num_samples--) {
                left_right = *(__m64 *) in_samples;
                sam_AB = samples_AB [m];
                samples_AB [k] = left_right;

                tmp0 = _m_paddd (sam_AB, sam_AB);
                tmp1 = _m_pand (sam_AB, mask);
                tmp0 = _m_psrldi (tmp0, 16);
                tmp1 = _m_pmaddwd (tmp1, weight_AB);
                tmp0 = _m_pmaddwd (tmp0, weight_AB);
                tmp1 = _m_paddd (tmp1, round);
                tmp0 = _m_pslldi (tmp0, 5);
                tmp1 = _m_psradi (tmp1, 10);
                left_right = _m_psubd (left_right, tmp0);
                left_right = _m_psubd (left_right, tmp1);

                *(__m64 *) out_samples = left_right;

                tmp0 = _m_pxor (sam_AB, left_right);
                tmp0 = _m_psradi (tmp0, 31);
                tmp1 = _m_pxor (delta, tmp0);
                tmp1 = _m_psubd (tmp1, tmp0);
                sam_AB = _m_pcmpeqd (sam_AB, zero);
                tmp0 = _m_pcmpeqd (left_right, zero);
                tmp0 = _m_por (tmp0, sam_AB);
                tmp0 = _m_pandn (tmp0, tmp1);
                weight_AB = _m_paddd (weight_AB, tmp0);

                sum_AB = _m_paddd (sum_AB, weight_AB);

                in_samples += dir;
                out_samples += dir;
                k = (k + 1) & (MAX_TERM - 1);
                m = (m + 1) & (MAX_TERM - 1);
            }
        }
    }
    else {
        if (dpp->term == -1) {
            while (num_samples--) {
                left_right = *(__m64 *) in_samples;
                sam_AB = samples_AB [0];
                samples_AB [0] = _m_punpckhdq (left_right, sam_AB);
                sam_AB = _m_punpckldq (sam_AB, left_right);

                tmp0 = _m_paddd (sam_AB, sam_AB);
                tmp1 = _m_pand (sam_AB, mask);
                tmp0 = _m_psrldi (tmp0, 16);
                tmp1 = _m_pmaddwd (tmp1, weight_AB);
                tmp0 = _m_pmaddwd (tmp0, weight_AB);
                tmp1 = _m_paddd (tmp1, round);
                tmp0 = _m_pslldi (tmp0, 5);
                tmp1 = _m_psradi (tmp1, 10);
                left_right = _m_psubd (left_right, tmp0);
                left_right = _m_psubd (left_right, tmp1);

                *(__m64 *) out_samples = left_right;

                tmp0 = _m_pcmpeqd (sam_AB, zero);
                tmp1 = _m_pcmpeqd (left_right, zero);
                tmp0 = _m_por (tmp0, tmp1);
                tmp0 = _m_pandn (tmp0, delta);
                sam_AB = _m_pxor (sam_AB, left_right);
                sam_AB = _m_psradi (sam_AB, 31);
                tmp1 = _m_psubd (fill, sam_AB);
                weight_AB = _m_pxor (weight_AB, sam_AB);
                weight_AB = _m_paddd (weight_AB, tmp1);
                weight_AB = _m_paddsw (weight_AB, tmp0);
                weight_AB = _m_psubd (weight_AB, tmp1);
                weight_AB = _m_pxor (weight_AB, sam_AB);

                sum_AB = _m_paddd (sum_AB, weight_AB);

                in_samples += dir;
                out_samples += dir;
            }
        }
        else if (dpp->term == -2) {
            while (num_samples--) {
                left_right = *(__m64 *) in_samples;
                sam_AB = samples_AB [0];
                samples_AB [0] = _m_punpckldq (sam_AB, left_right);
                sam_AB = _m_punpckhdq (left_right, sam_AB);

                tmp0 = _m_paddd (sam_AB, sam_AB);
                tmp1 = _m_pand (sam_AB, mask);
                tmp0 = _m_psrldi (tmp0, 16);
                tmp1 = _m_pmaddwd (tmp1, weight_AB);
                tmp0 = _m_pmaddwd (tmp0, weight_AB);
                tmp1 = _m_paddd (tmp1, round);
                tmp0 = _m_pslldi (tmp0, 5);
                tmp1 = _m_psradi (tmp1, 10);
                left_right = _m_psubd (left_right, tmp0);
                left_right = _m_psubd (left_right, tmp1);

                *(__m64 *) out_samples = left_right;

                tmp0 = _m_pcmpeqd (sam_AB, zero);
                tmp1 = _m_pcmpeqd (left_right, zero);
                tmp0 = _m_por (tmp0, tmp1);
                tmp0 = _m_pandn (tmp0, delta);
                sam_AB = _m_pxor (sam_AB, left_right);
                sam_AB = _m_psradi (sam_AB, 31);
                tmp1 = _m_psubd (fill, sam_AB);
                weight_AB = _m_pxor (weight_AB, sam_AB);
                weight_AB = _m_paddd (weight_AB, tmp1);
                weight_AB = _m_paddsw (weight_AB, tmp0);
                weight_AB = _m_psubd (weight_AB, tmp1);
                weight_AB = _m_pxor (weight_AB, sam_AB);

                sum_AB = _m_paddd (sum_AB, weight_AB);

                in_samples += dir;
                out_samples += dir;
            }
        }
        else if (dpp->term == -3) {
            while (num_samples--) {
                left_right = *(__m64 *) in_samples;
                sam_AB = samples_AB [0];
                tmp0 = _m_punpckhdq (left_right, left_right);
                samples_AB [0] = _m_punpckldq (tmp0, left_right);

                tmp0 = _m_paddd (sam_AB, sam_AB);
                tmp1 = _m_pand (sam_AB, mask);
                tmp0 = _m_psrldi (tmp0, 16);
                tmp1 = _m_pmaddwd (tmp1, weight_AB);
                tmp0 = _m_pmaddwd (tmp0, weight_AB);
                tmp1 = _m_paddd (tmp1, round);
                tmp0 = _m_pslldi (tmp0, 5);
                tmp1 = _m_psradi (tmp1, 10);
                left_right = _m_psubd (left_right, tmp0);
                left_right = _m_psubd (left_right, tmp1);

                *(__m64 *) out_samples = left_right;

                tmp0 = _m_pcmpeqd (sam_AB, zero);
                tmp1 = _m_pcmpeqd (left_right, zero);
                tmp0 = _m_por (tmp0, tmp1);
                tmp0 = _m_pandn (tmp0, delta);
                sam_AB = _m_pxor (sam_AB, left_right);
                sam_AB = _m_psradi (sam_AB, 31);
                tmp1 = _m_psubd (fill, sam_AB);
                weight_AB = _m_pxor (weight_AB, sam_AB);
                weight_AB = _m_paddd (weight_AB, tmp1);
                weight_AB = _m_paddsw (weight_AB, tmp0);
                weight_AB = _m_psubd (weight_AB, tmp1);
                weight_AB = _m_pxor (weight_AB, sam_AB);

                sum_AB = _m_paddd (sum_AB, weight_AB);

                in_samples += dir;
                out_samples += dir;
            }
        }
    }
    dpp->sum_A = ((int32_t *) &sum_AB) [0];
    dpp->sum_B = ((int32_t *) &sum_AB) [1];
    dpp->weight_A = ((int32_t *) &weight_AB) [0];
    dpp->weight_B = ((int32_t *) &weight_AB) [1];

    for (k = 0; k < MAX_TERM; ++k) {
        dpp->samples_A [k] = ((int32_t *) samples_AB) [m * 2];
        dpp->samples_B [k] = ((int32_t *) samples_AB) [m * 2 + 1];
        m = (m + 1) & (MAX_TERM - 1);
    }
    _mm_empty ();
}

#else

static void decorr_stereo_pass (int32_t *in_samples, int32_t *out_samples, int32_t num_samples, struct decorr_pass *dpp, int dir)
{
    int m = 0, i;

    dpp->sum_A = dpp->sum_B = 0;

    if (dir < 0) {
        out_samples += (num_samples - 1) * 2;
        in_samples += (num_samples - 1) * 2;
        dir = -2;
    }
    else
        dir = 2;

    dpp->weight_A = restore_weight (store_weight (dpp->weight_A));
    dpp->weight_B = restore_weight (store_weight (dpp->weight_B));

    for (i = 0; i < 8; ++i) {
        dpp->samples_A [i] = exp2s (log2s (dpp->samples_A [i]));
        dpp->samples_B [i] = exp2s (log2s (dpp->samples_B [i]));
    }

    switch (dpp->term) {

        case 2:
            while (num_samples--) {
                int32_t sam, tmp;

                sam = dpp->samples_A [0];
                dpp->samples_A [0] = dpp->samples_A [1];
                out_samples [0] = tmp = (dpp->samples_A [1] = in_samples [0]) - apply_weight (dpp->weight_A, sam);
                update_weight (dpp->weight_A, dpp->delta, sam, tmp);
                dpp->sum_A += dpp->weight_A;

                sam = dpp->samples_B [0];
                dpp->samples_B [0] = dpp->samples_B [1];
                out_samples [1] = tmp = (dpp->samples_B [1] = in_samples [1]) - apply_weight (dpp->weight_B, sam);
                update_weight (dpp->weight_B, dpp->delta, sam, tmp);
                dpp->sum_B += dpp->weight_B;

                in_samples += dir;
                out_samples += dir;
            }

            break;

        case 17:
            while (num_samples--) {
                int32_t sam, tmp;

                sam = 2 * dpp->samples_A [0] - dpp->samples_A [1];
                dpp->samples_A [1] = dpp->samples_A [0];
                out_samples [0] = tmp = (dpp->samples_A [0] = in_samples [0]) - apply_weight (dpp->weight_A, sam);
                update_weight (dpp->weight_A, dpp->delta, sam, tmp);
                dpp->sum_A += dpp->weight_A;

                sam = 2 * dpp->samples_B [0] - dpp->samples_B [1];
                dpp->samples_B [1] = dpp->samples_B [0];
                out_samples [1] = tmp = (dpp->samples_B [0] = in_samples [1]) - apply_weight (dpp->weight_B, sam);
                update_weight (dpp->weight_B, dpp->delta, sam, tmp);
                dpp->sum_B += dpp->weight_B;

                in_samples += dir;
                out_samples += dir;
            }

            break;

        case 18:
            while (num_samples--) {
                int32_t sam, tmp;

                sam = dpp->samples_A [0] + ((dpp->samples_A [0] - dpp->samples_A [1]) >> 1);
                dpp->samples_A [1] = dpp->samples_A [0];
                out_samples [0] = tmp = (dpp->samples_A [0] = in_samples [0]) - apply_weight (dpp->weight_A, sam);
                update_weight (dpp->weight_A, dpp->delta, sam, tmp);
                dpp->sum_A += dpp->weight_A;

                sam = dpp->samples_B [0] + ((dpp->samples_B [0] - dpp->samples_B [1]) >> 1);
                dpp->samples_B [1] = dpp->samples_B [0];
                out_samples [1] = tmp = (dpp->samples_B [0] = in_samples [1]) - apply_weight (dpp->weight_B, sam);
                update_weight (dpp->weight_B, dpp->delta, sam, tmp);
                dpp->sum_B += dpp->weight_B;

                in_samples += dir;
                out_samples += dir;
            }

            break;

        default: {
            int k = dpp->term & (MAX_TERM - 1);

            while (num_samples--) {
                int32_t sam, tmp;

                sam = dpp->samples_A [m];
                out_samples [0] = tmp = (dpp->samples_A [k] = in_samples [0]) - apply_weight (dpp->weight_A, sam);
                update_weight (dpp->weight_A, dpp->delta, sam, tmp);
                dpp->sum_A += dpp->weight_A;

                sam = dpp->samples_B [m];
                out_samples [1] = tmp = (dpp->samples_B [k] = in_samples [1]) - apply_weight (dpp->weight_B, sam);
                update_weight (dpp->weight_B, dpp->delta, sam, tmp);
                dpp->sum_B += dpp->weight_B;

                in_samples += dir;
                out_samples += dir;
                m = (m + 1) & (MAX_TERM - 1);
                k = (k + 1) & (MAX_TERM - 1);
            }

            if (m) {
                int32_t temp_A [MAX_TERM], temp_B [MAX_TERM];
                int k;

                memcpy (temp_A, dpp->samples_A, sizeof (dpp->samples_A));
                memcpy (temp_B, dpp->samples_B, sizeof (dpp->samples_B));

                for (k = 0; k < MAX_TERM; k++) {
                    dpp->samples_A [k] = temp_A [m];
                    dpp->samples_B [k] = temp_B [m];
                    m = (m + 1) & (MAX_TERM - 1);
                }
            }

            break;
        }

        case -1:
            while (num_samples--) {
                int32_t sam_A, sam_B, tmp;

                sam_A = dpp->samples_A [0];
                out_samples [0] = tmp = (sam_B = in_samples [0]) - apply_weight (dpp->weight_A, sam_A);
                update_weight_clip (dpp->weight_A, dpp->delta, sam_A, tmp);
                dpp->sum_A += dpp->weight_A;

                out_samples [1] = tmp = (dpp->samples_A [0] = in_samples [1]) - apply_weight (dpp->weight_B, sam_B);
                update_weight_clip (dpp->weight_B, dpp->delta, sam_B, tmp);
                dpp->sum_B += dpp->weight_B;

                in_samples += dir;
                out_samples += dir;
            }

            break;

        case -2:
            while (num_samples--) {
                int32_t sam_A, sam_B, tmp;

                sam_B = dpp->samples_B [0];
                out_samples [1] = tmp = (sam_A = in_samples [1]) - apply_weight (dpp->weight_B, sam_B);
                update_weight_clip (dpp->weight_B, dpp->delta, sam_B, tmp);
                dpp->sum_B += dpp->weight_B;

                out_samples [0] = tmp = (dpp->samples_B [0] = in_samples [0]) - apply_weight (dpp->weight_A, sam_A);
                update_weight_clip (dpp->weight_A, dpp->delta, sam_A, tmp);
                dpp->sum_A += dpp->weight_A;

                in_samples += dir;
                out_samples += dir;
            }

            break;

        case -3:
            while (num_samples--) {
                int32_t sam_A, sam_B, tmp;

                sam_A = dpp->samples_A [0];
                sam_B = dpp->samples_B [0];

                dpp->samples_A [0] = tmp = in_samples [1];
                out_samples [1] = tmp -= apply_weight (dpp->weight_B, sam_B);
                update_weight_clip (dpp->weight_B, dpp->delta, sam_B, tmp);
                dpp->sum_B += dpp->weight_B;

                dpp->samples_B [0] = tmp = in_samples [0];
                out_samples [0] = tmp -= apply_weight (dpp->weight_A, sam_A);
                update_weight_clip (dpp->weight_A, dpp->delta, sam_A, tmp);
                dpp->sum_A += dpp->weight_A;

                in_samples += dir;
                out_samples += dir;
            }

            break;
    }
}

#endif

static void decorr_stereo_pass_quick (int32_t *in_samples, int32_t *out_samples, int32_t num_samples, struct decorr_pass *dpp, int dir)
{
    int m = 0, i;

    if (dir < 0) {
        out_samples += (num_samples - 1) * 2;
        in_samples += (num_samples - 1) * 2;
        dir = -2;
    }
    else
        dir = 2;

    dpp->weight_A = restore_weight (store_weight (dpp->weight_A));
    dpp->weight_B = restore_weight (store_weight (dpp->weight_B));

    for (i = 0; i < 8; ++i) {
        dpp->samples_A [i] = exp2s (log2s (dpp->samples_A [i]));
        dpp->samples_B [i] = exp2s (log2s (dpp->samples_B [i]));
    }

    switch (dpp->term) {

        case 2:
            while (num_samples--) {
                int32_t sam, tmp;

                sam = dpp->samples_A [0];
                dpp->samples_A [0] = dpp->samples_A [1];
                out_samples [0] = tmp = (dpp->samples_A [1] = in_samples [0]) - apply_weight_i (dpp->weight_A, sam);
                update_weight (dpp->weight_A, dpp->delta, sam, tmp);

                sam = dpp->samples_B [0];
                dpp->samples_B [0] = dpp->samples_B [1];
                out_samples [1] = tmp = (dpp->samples_B [1] = in_samples [1]) - apply_weight_i (dpp->weight_B, sam);
                update_weight (dpp->weight_B, dpp->delta, sam, tmp);

                in_samples += dir;
                out_samples += dir;
            }

            break;

        case 17:
            while (num_samples--) {
                int32_t sam, tmp;

                sam = 2 * dpp->samples_A [0] - dpp->samples_A [1];
                dpp->samples_A [1] = dpp->samples_A [0];
                out_samples [0] = tmp = (dpp->samples_A [0] = in_samples [0]) - apply_weight_i (dpp->weight_A, sam);
                update_weight (dpp->weight_A, dpp->delta, sam, tmp);

                sam = 2 * dpp->samples_B [0] - dpp->samples_B [1];
                dpp->samples_B [1] = dpp->samples_B [0];
                out_samples [1] = tmp = (dpp->samples_B [0] = in_samples [1]) - apply_weight_i (dpp->weight_B, sam);
                update_weight (dpp->weight_B, dpp->delta, sam, tmp);

                in_samples += dir;
                out_samples += dir;
            }

            break;

        case 18:
            while (num_samples--) {
                int32_t sam, tmp;

                sam = dpp->samples_A [0] + ((dpp->samples_A [0] - dpp->samples_A [1]) >> 1);
                dpp->samples_A [1] = dpp->samples_A [0];
                out_samples [0] = tmp = (dpp->samples_A [0] = in_samples [0]) - apply_weight_i (dpp->weight_A, sam);
                update_weight (dpp->weight_A, dpp->delta, sam, tmp);

                sam = dpp->samples_B [0] + ((dpp->samples_B [0] - dpp->samples_B [1]) >> 1);
                dpp->samples_B [1] = dpp->samples_B [0];
                out_samples [1] = tmp = (dpp->samples_B [0] = in_samples [1]) - apply_weight_i (dpp->weight_B, sam);
                update_weight (dpp->weight_B, dpp->delta, sam, tmp);

                in_samples += dir;
                out_samples += dir;
            }

            break;

        default: {
            int k = dpp->term & (MAX_TERM - 1);

            while (num_samples--) {
                int32_t sam, tmp;

                sam = dpp->samples_A [m];
                out_samples [0] = tmp = (dpp->samples_A [k] = in_samples [0]) - apply_weight_i (dpp->weight_A, sam);
                update_weight (dpp->weight_A, dpp->delta, sam, tmp);

                sam = dpp->samples_B [m];
                out_samples [1] = tmp = (dpp->samples_B [k] = in_samples [1]) - apply_weight_i (dpp->weight_B, sam);
                update_weight (dpp->weight_B, dpp->delta, sam, tmp);

                in_samples += dir;
                out_samples += dir;
                m = (m + 1) & (MAX_TERM - 1);
                k = (k + 1) & (MAX_TERM - 1);
            }

            if (m) {
                int32_t temp_A [MAX_TERM], temp_B [MAX_TERM];
                int k;

                memcpy (temp_A, dpp->samples_A, sizeof (dpp->samples_A));
                memcpy (temp_B, dpp->samples_B, sizeof (dpp->samples_B));

                for (k = 0; k < MAX_TERM; k++) {
                    dpp->samples_A [k] = temp_A [m];
                    dpp->samples_B [k] = temp_B [m];
                    m = (m + 1) & (MAX_TERM - 1);
                }
            }

            break;
        }

        case -1:
            while (num_samples--) {
                int32_t sam_A, sam_B, tmp;

                sam_A = dpp->samples_A [0];
                out_samples [0] = tmp = (sam_B = in_samples [0]) - apply_weight_i (dpp->weight_A, sam_A);
                update_weight_clip (dpp->weight_A, dpp->delta, sam_A, tmp);

                out_samples [1] = tmp = (dpp->samples_A [0] = in_samples [1]) - apply_weight_i (dpp->weight_B, sam_B);
                update_weight_clip (dpp->weight_B, dpp->delta, sam_B, tmp);

                in_samples += dir;
                out_samples += dir;
            }

            break;

        case -2:
            while (num_samples--) {
                int32_t sam_A, sam_B, tmp;

                sam_B = dpp->samples_B [0];
                out_samples [1] = tmp = (sam_A = in_samples [1]) - apply_weight_i (dpp->weight_B, sam_B);
                update_weight_clip (dpp->weight_B, dpp->delta, sam_B, tmp);

                out_samples [0] = tmp = (dpp->samples_B [0] = in_samples [0]) - apply_weight_i (dpp->weight_A, sam_A);
                update_weight_clip (dpp->weight_A, dpp->delta, sam_A, tmp);

                in_samples += dir;
                out_samples += dir;
            }

            break;

        case -3:
            while (num_samples--) {
                int32_t sam_A, sam_B, tmp;

                sam_A = dpp->samples_A [0];
                sam_B = dpp->samples_B [0];

                dpp->samples_A [0] = tmp = in_samples [1];
                out_samples [1] = tmp -= apply_weight_i (dpp->weight_B, sam_B);
                update_weight_clip (dpp->weight_B, dpp->delta, sam_B, tmp);

                dpp->samples_B [0] = tmp = in_samples [0];
                out_samples [0] = tmp -= apply_weight_i (dpp->weight_A, sam_A);
                update_weight_clip (dpp->weight_A, dpp->delta, sam_A, tmp);

                in_samples += dir;
                out_samples += dir;
            }

            break;
    }
}

static void reverse_decorr (struct decorr_pass *dpp)
{
    if (dpp->term > MAX_TERM) {
        int32_t sam_A, sam_B;

        if (dpp->term & 1) {
            sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
            sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
        }
        else {
            sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
            sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
        }

        dpp->samples_A [1] = dpp->samples_A [0];
        dpp->samples_B [1] = dpp->samples_B [0];
        dpp->samples_A [0] = sam_A;
        dpp->samples_B [0] = sam_B;

        if (dpp->term & 1) {
            sam_A = 2 * dpp->samples_A [0] - dpp->samples_A [1];
            sam_B = 2 * dpp->samples_B [0] - dpp->samples_B [1];
        }
        else {
            sam_A = (3 * dpp->samples_A [0] - dpp->samples_A [1]) >> 1;
            sam_B = (3 * dpp->samples_B [0] - dpp->samples_B [1]) >> 1;
        }

        dpp->samples_A [1] = sam_A;
        dpp->samples_B [1] = sam_B;
    }
    else if (dpp->term > 1) {
        int i = 0, j = dpp->term - 1, cnt = dpp->term / 2;

        while (cnt--) {
            i &= (MAX_TERM - 1);
            j &= (MAX_TERM - 1);
            dpp->samples_A [i] ^= dpp->samples_A [j];
            dpp->samples_A [j] ^= dpp->samples_A [i];
            dpp->samples_A [i] ^= dpp->samples_A [j];
            dpp->samples_B [i] ^= dpp->samples_B [j];
            dpp->samples_B [j] ^= dpp->samples_B [i];
            dpp->samples_B [i++] ^= dpp->samples_B [j--];
        }
    }
    else if (dpp->term == -1) {
    }
    else if (dpp->term == -2) {
    }
    else if (dpp->term == -3) {
    }
}

static void decorr_stereo_buffer (WavpackExtraInfo *info, int32_t *samples, int32_t *outsamples, int32_t num_samples, int tindex)
{
    struct decorr_pass dp, *dppi = info->dps + tindex;
    int delta = dppi->delta, pre_delta;
    int term = dppi->term;

    if (delta == 7)
        pre_delta = 7;
    else if (delta < 2)
        pre_delta = 3;
    else
        pre_delta = delta + 1;

    CLEAR (dp);
    dp.term = term;
    dp.delta = pre_delta;
    decorr_stereo_pass (samples, outsamples, num_samples > 2048 ? 2048 : num_samples, &dp, -1);
    dp.delta = delta;

    if (tindex == 0)
        reverse_decorr (&dp);
    else {
        CLEAR (dp.samples_A);
        CLEAR (dp.samples_B);
    }

    memcpy (dppi->samples_A, dp.samples_A, sizeof (dp.samples_A));
    memcpy (dppi->samples_B, dp.samples_B, sizeof (dp.samples_B));
    dppi->weight_A = dp.weight_A;
    dppi->weight_B = dp.weight_B;

    if (delta == 0) {
        dp.delta = 1;
        decorr_stereo_pass (samples, outsamples, num_samples, &dp, 1);
        dp.delta = 0;
        memcpy (dp.samples_A, dppi->samples_A, sizeof (dp.samples_A));
        memcpy (dp.samples_B, dppi->samples_B, sizeof (dp.samples_B));
        dppi->weight_A = dp.weight_A = dp.sum_A / num_samples;
        dppi->weight_B = dp.weight_B = dp.sum_B / num_samples;
    }

//    if (memcmp (dppi, &dp, sizeof (dp)))
//      error_line ("decorr_passes don't match, delta = %d", delta);

    if (info->gt16bit)
        decorr_stereo_pass (samples, outsamples, num_samples, &dp, 1);
    else
        decorr_stereo_pass_quick (samples, outsamples, num_samples, &dp, 1);
}

static void recurse_stereo (WavpackContext *wpc, WavpackExtraInfo *info, int depth, int delta, uint32_t input_bits)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    int term, branches = ((wpc->config.extra_flags & EXTRA_BRANCHES) >> 6) - depth;
    int32_t *samples, *outsamples;
    uint32_t term_bits [22], bits;

    if (branches < 1 || depth + 1 == info->nterms)
        branches = 1;

    CLEAR (term_bits);
    samples = info->sampleptrs [depth];
    outsamples = info->sampleptrs [depth + 1];

    for (term = -3; term <= 18; ++term) {
        if (!term || (term > 8 && term < 17))
            continue;

        if (term == 17 && branches == 1 && depth + 1 < info->nterms)
            continue;

        if (term == -1 || term == -2)
            if (!(wps->wphdr.flags & CROSS_DECORR))
                continue;

        if ((wpc->config.flags & CONFIG_FAST_FLAG) && (term > 4 && term < 17))
            continue;

        info->dps [depth].term = term;
        info->dps [depth].delta = delta;
        decorr_stereo_buffer (info, samples, outsamples, wps->wphdr.block_samples, depth);
        bits = log2buffer (outsamples, wps->wphdr.block_samples * 2, info->log_limit);

        if (bits < info->best_bits) {
            info->best_bits = bits;
            CLEAR (wps->decorr_passes);
            memcpy (wps->decorr_passes, info->dps, sizeof (info->dps [0]) * (depth + 1));
            memcpy (info->sampleptrs [info->nterms + 1], info->sampleptrs [depth + 1], wps->wphdr.block_samples * 8);
        }

        term_bits [term + 3] = bits;
    }

    while (depth + 1 < info->nterms && branches--) {
        uint32_t local_best_bits = input_bits;
        int best_term = 0, i;

        for (i = 0; i < 22; ++i)
            if (term_bits [i] && term_bits [i] < local_best_bits) {
                local_best_bits = term_bits [i];
//              term_bits [i] = 0;
                best_term = i - 3;
            }

        if (!best_term)
            break;

        term_bits [best_term + 3] = 0;

        info->dps [depth].term = best_term;
        info->dps [depth].delta = delta;
        decorr_stereo_buffer (info, samples, outsamples, wps->wphdr.block_samples, depth);

//      if (log2buffer (outsamples, wps->wphdr.block_samples * 2, 0) != local_best_bits)
//          error_line ("data doesn't match!");

        recurse_stereo (wpc, info, depth + 1, delta, local_best_bits);
    }
}

static void delta_stereo (WavpackContext *wpc, WavpackExtraInfo *info)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    int lower = FALSE;
    int delta, d;
    uint32_t bits;

    if (wps->decorr_passes [0].term)
        delta = wps->decorr_passes [0].delta;
    else
        return;

    for (d = delta - 1; d >= 0; --d) {
        int i;

        if (!d && (wps->wphdr.flags & HYBRID_FLAG))
            break;

        for (i = 0; i < info->nterms && wps->decorr_passes [i].term; ++i) {
            info->dps [i].term = wps->decorr_passes [i].term;
            info->dps [i].delta = d;
            decorr_stereo_buffer (info, info->sampleptrs [i], info->sampleptrs [i+1], wps->wphdr.block_samples, i);
        }

        bits = log2buffer (info->sampleptrs [i], wps->wphdr.block_samples * 2, info->log_limit);

        if (bits < info->best_bits) {
            lower = TRUE;
            info->best_bits = bits;
            CLEAR (wps->decorr_passes);
            memcpy (wps->decorr_passes, info->dps, sizeof (info->dps [0]) * i);
            memcpy (info->sampleptrs [info->nterms + 1], info->sampleptrs [i], wps->wphdr.block_samples * 8);
        }
        else
            break;
    }

    for (d = delta + 1; !lower && d <= 7; ++d) {
        int i;

        for (i = 0; i < info->nterms && wps->decorr_passes [i].term; ++i) {
            info->dps [i].term = wps->decorr_passes [i].term;
            info->dps [i].delta = d;
            decorr_stereo_buffer (info, info->sampleptrs [i], info->sampleptrs [i+1], wps->wphdr.block_samples, i);
        }

        bits = log2buffer (info->sampleptrs [i], wps->wphdr.block_samples * 2, info->log_limit);

        if (bits < info->best_bits) {
            info->best_bits = bits;
            CLEAR (wps->decorr_passes);
            memcpy (wps->decorr_passes, info->dps, sizeof (info->dps [0]) * i);
            memcpy (info->sampleptrs [info->nterms + 1], info->sampleptrs [i], wps->wphdr.block_samples * 8);
        }
        else
            break;
    }
}

static void sort_stereo (WavpackContext *wpc, WavpackExtraInfo *info)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    int reversed = TRUE;
    uint32_t bits;

    while (reversed) {
        int ri, i;

        memcpy (info->dps, wps->decorr_passes, sizeof (wps->decorr_passes));
        reversed = FALSE;

        for (ri = 0; ri < info->nterms && wps->decorr_passes [ri].term; ++ri) {

            if (ri + 1 >= info->nterms || !wps->decorr_passes [ri+1].term)
                break;

            if (wps->decorr_passes [ri].term == wps->decorr_passes [ri+1].term) {
                decorr_stereo_buffer (info, info->sampleptrs [ri], info->sampleptrs [ri+1], wps->wphdr.block_samples, ri);
                continue;
            }

            info->dps [ri] = wps->decorr_passes [ri+1];
            info->dps [ri+1] = wps->decorr_passes [ri];

            for (i = ri; i < info->nterms && wps->decorr_passes [i].term; ++i)
                decorr_stereo_buffer (info, info->sampleptrs [i], info->sampleptrs [i+1], wps->wphdr.block_samples, i);

            bits = log2buffer (info->sampleptrs [i], wps->wphdr.block_samples * 2, info->log_limit);

            if (bits < info->best_bits) {
                reversed = TRUE;
                info->best_bits = bits;
                CLEAR (wps->decorr_passes);
                memcpy (wps->decorr_passes, info->dps, sizeof (info->dps [0]) * i);
                memcpy (info->sampleptrs [info->nterms + 1], info->sampleptrs [i], wps->wphdr.block_samples * 8);
            }
            else {
                info->dps [ri] = wps->decorr_passes [ri];
                info->dps [ri+1] = wps->decorr_passes [ri+1];
                decorr_stereo_buffer (info, info->sampleptrs [ri], info->sampleptrs [ri+1], wps->wphdr.block_samples, ri);
            }
        }
    }
}

static const uint32_t xtable [] = { 91, 123, 187, 251 };

static void analyze_stereo (WavpackContext *wpc, int32_t *samples, int do_samples)
{
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    WavpackExtraInfo info;
    int i;

    info.gt16bit = ((wps->wphdr.flags & MAG_MASK) >> MAG_LSB) >= 16;

#ifdef LOG_LIMIT
    info.log_limit = (((wps->wphdr.flags & MAG_MASK) >> MAG_LSB) + 4) * 256;

    if (info.log_limit > LOG_LIMIT)
        info.log_limit = LOG_LIMIT;
#else
    info.log_limit = 0;
#endif

    if (wpc->config.flags & (CONFIG_HIGH_FLAG | CONFIG_VERY_HIGH_FLAG))
        wpc->config.extra_flags = xtable [wpc->config.xmode - 4];
    else
        wpc->config.extra_flags = xtable [wpc->config.xmode - 3];

    info.nterms = wps->num_terms;

    for (i = 0; i < info.nterms + 2; ++i)
        info.sampleptrs [i] = malloc (wps->wphdr.block_samples * 8);

    memcpy (info.dps, wps->decorr_passes, sizeof (info.dps));
    memcpy (info.sampleptrs [0], samples, wps->wphdr.block_samples * 8);

    for (i = 0; i < info.nterms && info.dps [i].term; ++i)
        if (info.gt16bit)
            decorr_stereo_pass (info.sampleptrs [i], info.sampleptrs [i + 1], wps->wphdr.block_samples, info.dps + i, 1);
        else
            decorr_stereo_pass_quick (info.sampleptrs [i], info.sampleptrs [i + 1], wps->wphdr.block_samples, info.dps + i, 1);

    info.best_bits = log2buffer (info.sampleptrs [info.nterms], wps->wphdr.block_samples * 2, 0) * 1;
    memcpy (info.sampleptrs [info.nterms + 1], info.sampleptrs [i], wps->wphdr.block_samples * 8);

    if (wpc->config.extra_flags & EXTRA_BRANCHES)
        recurse_stereo (wpc, &info, 0, (int) floor (wps->delta_decay + 0.5),
            log2buffer (info.sampleptrs [0], wps->wphdr.block_samples * 2, 0));

    if (wpc->config.extra_flags & EXTRA_SORT_FIRST)
        sort_stereo (wpc, &info);

    if (wpc->config.extra_flags & EXTRA_TRY_DELTAS) {
        delta_stereo (wpc, &info);

        if ((wpc->config.extra_flags & EXTRA_ADJUST_DELTAS) && wps->decorr_passes [0].term)
            wps->delta_decay = (float)((wps->delta_decay * 2.0 + wps->decorr_passes [0].delta) / 3.0);
        else
            wps->delta_decay = 2.0;
    }

    if (wpc->config.extra_flags & EXTRA_SORT_LAST)
        sort_stereo (wpc, &info);

    if (do_samples)
        memcpy (samples, info.sampleptrs [info.nterms + 1], wps->wphdr.block_samples * 8);

    for (i = 0; i < info.nterms; ++i)
        if (!wps->decorr_passes [i].term)
            break;

    wps->num_terms = i;

    for (i = 0; i < info.nterms + 2; ++i)
        free (info.sampleptrs [i]);
}

static void stereo_add_noise (WavpackStream *wps, int32_t *lptr, int32_t *rptr)
{
    int shaping_weight, new = wps->wphdr.flags & NEW_SHAPING;
    int32_t error [2], temp, cnt;

    scan_word (wps, rptr, wps->wphdr.block_samples, -1);
    cnt = wps->wphdr.block_samples;
    CLEAR (error);

    if (wps->wphdr.flags & HYBRID_SHAPE) {
        while (cnt--) {
            shaping_weight = (wps->dc.shaping_acc [0] += wps->dc.shaping_delta [0]) >> 16;
            temp = -apply_weight (shaping_weight, error [0]);

            if (new && shaping_weight < 0 && temp) {
                if (temp == error [0])
                    temp = (temp < 0) ? temp + 1 : temp - 1;

                lptr [0] += (error [0] = nosend_word (wps, rptr [0], 0) - rptr [0] + temp);
            }
            else
                lptr [0] += (error [0] = nosend_word (wps, rptr [0], 0) - rptr [0]) + temp;

            shaping_weight = (wps->dc.shaping_acc [1] += wps->dc.shaping_delta [1]) >> 16;
            temp = -apply_weight (shaping_weight, error [1]);

            if (new && shaping_weight < 0 && temp) {
                if (temp == error [1])
                    temp = (temp < 0) ? temp + 1 : temp - 1;

                lptr [1] += (error [1] = nosend_word (wps, rptr [1], 1) - rptr [1] + temp);
            }
            else
                lptr [1] += (error [1] = nosend_word (wps, rptr [1], 1) - rptr [1]) + temp;

            lptr += 2;
            rptr += 2;
        }

        wps->dc.shaping_acc [0] -= wps->dc.shaping_delta [0] * wps->wphdr.block_samples;
        wps->dc.shaping_acc [1] -= wps->dc.shaping_delta [1] * wps->wphdr.block_samples;
    }
    else
        while (cnt--) {
            lptr [0] += nosend_word (wps, rptr [0], 0) - rptr [0];
            lptr [1] += nosend_word (wps, rptr [1], 1) - rptr [1];
            lptr += 2;
            rptr += 2;
        }
}

void execute_stereo (WavpackContext *wpc, int32_t *samples, int no_history, int do_samples)
{
    int32_t *temp_buffer [2], *best_buffer, *noisy_buffer = NULL, *js_buffer = NULL;
    struct decorr_pass temp_decorr_pass, save_decorr_passes [MAX_NTERMS];
    WavpackStream *wps = wpc->streams [wpc->current_stream];
    int32_t num_samples = wps->wphdr.block_samples;
    int32_t buf_size = sizeof (int32_t) * num_samples * 2;
    uint32_t best_size = (uint32_t) -1, size;
    int log_limit, force_js = 0, force_ts = 0, pi, i;

    for (i = 0; i < num_samples * 2; ++i)
        if (samples [i])
            break;

    if (i == num_samples * 2) {
        wps->wphdr.flags &= ~((uint32_t) JOINT_STEREO);
        CLEAR (wps->decorr_passes);
        wps->num_terms = 0;
        init_words (wps);
        return;
    }

#ifdef LOG_LIMIT
    log_limit = (((wps->wphdr.flags & MAG_MASK) >> MAG_LSB) + 4) * 256;

    if (log_limit > LOG_LIMIT)
        log_limit = LOG_LIMIT;
#else
    log_limit = 0;
#endif

    if (wpc->config.flags & CONFIG_JOINT_OVERRIDE) {
        if (wps->wphdr.flags & JOINT_STEREO)
            force_js = 1;
        else
            force_ts = 1;
    }

    CLEAR (save_decorr_passes);
    temp_buffer [0] = malloc (buf_size);
    temp_buffer [1] = malloc (buf_size);
    best_buffer = malloc (buf_size);

    if (wps->num_passes > 1 && (wps->wphdr.flags & HYBRID_FLAG)) {
        CLEAR (temp_decorr_pass);
        temp_decorr_pass.delta = 2;
        temp_decorr_pass.term = 18;

        decorr_stereo_pass (samples, temp_buffer [0],
            num_samples > 2048 ? 2048 : num_samples, &temp_decorr_pass, -1);

        reverse_decorr (&temp_decorr_pass);
        decorr_stereo_pass (samples, temp_buffer [0], num_samples, &temp_decorr_pass, 1);
        CLEAR (temp_decorr_pass);
        temp_decorr_pass.delta = 2;
        temp_decorr_pass.term = 17;

        decorr_stereo_pass (temp_buffer [0], temp_buffer [1],
            num_samples > 2048 ? 2048 : num_samples, &temp_decorr_pass, -1);

        decorr_stereo_pass (temp_buffer [0], temp_buffer [1], num_samples, &temp_decorr_pass, 1);
        noisy_buffer = malloc (buf_size);
        memcpy (noisy_buffer, samples, buf_size);
        stereo_add_noise (wps, noisy_buffer, temp_buffer [1]);
        no_history = 1;
    }

    if (no_history || wps->num_passes >= 7)
        wps->best_decorr = wps->mask_decorr = 0;

    for (pi = 0; pi < wps->num_passes;) {
        WavpackDecorrSpec *wpds;
        int nterms, c, j;

        if (!pi)
            c = wps->best_decorr;
        else {
            if (wps->mask_decorr == 0)
                c = 0;
            else
                c = (wps->best_decorr & (wps->mask_decorr - 1)) | wps->mask_decorr;

            if (c == wps->best_decorr) {
                wps->mask_decorr = wps->mask_decorr ? ((wps->mask_decorr << 1) & (wps->num_decorrs - 1)) : 1;
                continue;
            }
        }

        wpds = &wps->decorr_specs [c];
        nterms = (int) strlen (wpds->terms);

        while (1) {
            if (force_js || (wpds->joint_stereo && !force_ts)) {
                if (!js_buffer) {
                    int32_t *lptr, cnt = num_samples;

                    lptr = js_buffer = malloc (buf_size);
                    memcpy (js_buffer, noisy_buffer ? noisy_buffer : samples, buf_size);

                    while (cnt--) {
                        lptr [1] += ((lptr [0] -= lptr [1]) >> 1);
                        lptr += 2;
                    }
                }

                memcpy (temp_buffer [0], js_buffer, buf_size);
            }
            else
                memcpy (temp_buffer [0], noisy_buffer ? noisy_buffer : samples, buf_size);

            CLEAR (save_decorr_passes);

            for (j = 0; j < nterms; ++j) {
                CLEAR (temp_decorr_pass);
                temp_decorr_pass.delta = wpds->delta;
                temp_decorr_pass.term = wpds->terms [j];

                if (temp_decorr_pass.term < 0 && !(wps->wphdr.flags & CROSS_DECORR))
                    temp_decorr_pass.term = -3;

                decorr_stereo_pass (temp_buffer [j&1], temp_buffer [~j&1],
                    num_samples > 2048 ? 2048 : num_samples, &temp_decorr_pass, -1);

                if (j) {
                    CLEAR (temp_decorr_pass.samples_A);
                    CLEAR (temp_decorr_pass.samples_B);
                }
                else
                    reverse_decorr (&temp_decorr_pass);

                memcpy (save_decorr_passes + j, &temp_decorr_pass, sizeof (struct decorr_pass));

                if (((wps->wphdr.flags & MAG_MASK) >> MAG_LSB) >= 16)
                    decorr_stereo_pass (temp_buffer [j&1], temp_buffer [~j&1], num_samples, &temp_decorr_pass, 1);
                else
                    decorr_stereo_pass_quick (temp_buffer [j&1], temp_buffer [~j&1], num_samples, &temp_decorr_pass, 1);
            }

            size = log2buffer (temp_buffer [j&1], num_samples * 2, log_limit);

            if (size == (uint32_t) -1 && nterms)
                nterms >>= 1;
            else
                break;
        }

        if (size < best_size) {
            memcpy (best_buffer, temp_buffer [j&1], buf_size);
            memcpy (wps->decorr_passes, save_decorr_passes, sizeof (struct decorr_pass) * MAX_NTERMS);
            wps->num_terms = nterms;
            wps->best_decorr = c;
            best_size = size;
        }

        if (pi++)
            wps->mask_decorr = wps->mask_decorr ? ((wps->mask_decorr << 1) & (wps->num_decorrs - 1)) : 1;
    }

    if (force_js || (wps->decorr_specs [wps->best_decorr].joint_stereo && !force_ts))
        wps->wphdr.flags |= JOINT_STEREO;
    else
        wps->wphdr.flags &= ~((uint32_t) JOINT_STEREO);

    if (wpc->config.xmode > 3) {
        if (wps->wphdr.flags & JOINT_STEREO) {
            analyze_stereo (wpc, js_buffer, do_samples);

            if (do_samples)
                memcpy (samples, js_buffer, buf_size);
        }
        else if (noisy_buffer) {
            analyze_stereo (wpc, noisy_buffer, do_samples);

            if (do_samples)
                memcpy (samples, noisy_buffer, buf_size);
        }
        else
            analyze_stereo (wpc, samples, do_samples);
    }
    else if (do_samples)
        memcpy (samples, best_buffer, buf_size);

    if (wpc->config.xmode > 3 || no_history || wps->joint_stereo != wps->decorr_specs [wps->best_decorr].joint_stereo) {
        wps->joint_stereo = wps->decorr_specs [wps->best_decorr].joint_stereo;
        scan_word (wps, best_buffer, num_samples, -1);
    }

    if (noisy_buffer)
        free (noisy_buffer);

    if (js_buffer)
        free (js_buffer);

    free (temp_buffer [1]);
    free (temp_buffer [0]);
    free (best_buffer);

#ifdef EXTRA_DUMP
    if (1) {
        char string [256], substring [20];
        int i;

        sprintf (string, "%s: terms =",
            (wps->wphdr.flags & JOINT_STEREO) ? "JS" : "TS");

        for (i = 0; i < wps->num_terms; ++i) {
            if (wps->decorr_passes [i].term) {
                if (i && wps->decorr_passes [i-1].delta == wps->decorr_passes [i].delta)
                    sprintf (substring, " %d", wps->decorr_passes [i].term);
                else
                    sprintf (substring, " %d->%d", wps->decorr_passes [i].term,
                        wps->decorr_passes [i].delta);
            }
            else
                sprintf (substring, " *");

            strcat (string, substring);
        }

        error_line (string);
    }
#endif
}

