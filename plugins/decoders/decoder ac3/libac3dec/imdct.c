/*
* This source code is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*       
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
* File Name: imdct.c							
*
* Reference:
*
* Author:                                                
*
* Description:
*
* 	
* 
* History:
* 02/23/2005  
*  
*
*CodeReview Log:
* 
*/
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "ac3.h"
#include "decode.h"
#include "imdct.h"

/* typecast M_PI as a single-precision FPU number */
#ifndef M_PI
# ifdef PI
#  define M_PI ((float)PI)
# else
#  define M_PI ((float)3.14159265358979323846)
# endif
#endif

void imdct_do_256(float x[],float y[],float delay[]);
void imdct_do_512(float x[],float y[],float delay[]);

typedef struct complex_s
{
	float real;
	float imag;
} complex_t;


#define N 512

static complex_t buf[N/4];

/* 128 point bit-reverse LUT */
const static uint_8 bit_reverse_512[] = {
	0x00, 0x40, 0x20, 0x60, 0x10, 0x50, 0x30, 0x70, 
	0x08, 0x48, 0x28, 0x68, 0x18, 0x58, 0x38, 0x78, 
	0x04, 0x44, 0x24, 0x64, 0x14, 0x54, 0x34, 0x74, 
	0x0c, 0x4c, 0x2c, 0x6c, 0x1c, 0x5c, 0x3c, 0x7c, 
	0x02, 0x42, 0x22, 0x62, 0x12, 0x52, 0x32, 0x72, 
	0x0a, 0x4a, 0x2a, 0x6a, 0x1a, 0x5a, 0x3a, 0x7a, 
	0x06, 0x46, 0x26, 0x66, 0x16, 0x56, 0x36, 0x76, 
	0x0e, 0x4e, 0x2e, 0x6e, 0x1e, 0x5e, 0x3e, 0x7e, 
	0x01, 0x41, 0x21, 0x61, 0x11, 0x51, 0x31, 0x71, 
	0x09, 0x49, 0x29, 0x69, 0x19, 0x59, 0x39, 0x79, 
	0x05, 0x45, 0x25, 0x65, 0x15, 0x55, 0x35, 0x75, 
	0x0d, 0x4d, 0x2d, 0x6d, 0x1d, 0x5d, 0x3d, 0x7d, 
	0x03, 0x43, 0x23, 0x63, 0x13, 0x53, 0x33, 0x73, 
	0x0b, 0x4b, 0x2b, 0x6b, 0x1b, 0x5b, 0x3b, 0x7b, 
	0x07, 0x47, 0x27, 0x67, 0x17, 0x57, 0x37, 0x77, 
	0x0f, 0x4f, 0x2f, 0x6f, 0x1f, 0x5f, 0x3f, 0x7f};

const static uint_8 bit_reverse_256[] = {
	0x00, 0x20, 0x10, 0x30, 0x08, 0x28, 0x18, 0x38, 
	0x04, 0x24, 0x14, 0x34, 0x0c, 0x2c, 0x1c, 0x3c, 
	0x02, 0x22, 0x12, 0x32, 0x0a, 0x2a, 0x1a, 0x3a, 
	0x06, 0x26, 0x16, 0x36, 0x0e, 0x2e, 0x1e, 0x3e, 
	0x01, 0x21, 0x11, 0x31, 0x09, 0x29, 0x19, 0x39, 
	0x05, 0x25, 0x15, 0x35, 0x0d, 0x2d, 0x1d, 0x3d, 
	0x03, 0x23, 0x13, 0x33, 0x0b, 0x2b, 0x1b, 0x3b, 
	0x07, 0x27, 0x17, 0x37, 0x0f, 0x2f, 0x1f, 0x3f};

/* Twiddle factor LUT */
static complex_t *w[12];

static complex_t w_1[1];
static complex_t w_2[2];
static complex_t w_4[4];
static complex_t w_8[8];
static complex_t w_16[16];
static complex_t w_32[32];
static complex_t w_64[64];

/* Twiddle factors for IMDCT */
static float xcos1[N/4];
static float xsin1[N/4];
static float xcos2[N/8];
static float xsin2[N/8];

/* Delay buffer for time domain interleaving */
static float delay[6][256];

/* Windowing function for Modified DCT - Thank you acroread */
const static float window[] = {
	0.00014, 0.00024, 0.00037, 0.00051, 0.00067, 0.00086, 0.00107, 0.00130,
	0.00157, 0.00187, 0.00220, 0.00256, 0.00297, 0.00341, 0.00390, 0.00443,
	0.00501, 0.00564, 0.00632, 0.00706, 0.00785, 0.00871, 0.00962, 0.01061,
	0.01166, 0.01279, 0.01399, 0.01526, 0.01662, 0.01806, 0.01959, 0.02121,
	0.02292, 0.02472, 0.02662, 0.02863, 0.03073, 0.03294, 0.03527, 0.03770,
	0.04025, 0.04292, 0.04571, 0.04862, 0.05165, 0.05481, 0.05810, 0.06153,
	0.06508, 0.06878, 0.07261, 0.07658, 0.08069, 0.08495, 0.08935, 0.09389,
	0.09859, 0.10343, 0.10842, 0.11356, 0.11885, 0.12429, 0.12988, 0.13563,
	0.14152, 0.14757, 0.15376, 0.16011, 0.16661, 0.17325, 0.18005, 0.18699,
	0.19407, 0.20130, 0.20867, 0.21618, 0.22382, 0.23161, 0.23952, 0.24757,
	0.25574, 0.26404, 0.27246, 0.28100, 0.28965, 0.29841, 0.30729, 0.31626,
	0.32533, 0.33450, 0.34376, 0.35311, 0.36253, 0.37204, 0.38161, 0.39126,
	0.40096, 0.41072, 0.42054, 0.43040, 0.44030, 0.45023, 0.46020, 0.47019,
	0.48020, 0.49022, 0.50025, 0.51028, 0.52031, 0.53033, 0.54033, 0.55031,
	0.56026, 0.57019, 0.58007, 0.58991, 0.59970, 0.60944, 0.61912, 0.62873,
	0.63827, 0.64774, 0.65713, 0.66643, 0.67564, 0.68476, 0.69377, 0.70269,
	0.71150, 0.72019, 0.72877, 0.73723, 0.74557, 0.75378, 0.76186, 0.76981,
	0.77762, 0.78530, 0.79283, 0.80022, 0.80747, 0.81457, 0.82151, 0.82831,
	0.83496, 0.84145, 0.84779, 0.85398, 0.86001, 0.86588, 0.87160, 0.87716,
	0.88257, 0.88782, 0.89291, 0.89785, 0.90264, 0.90728, 0.91176, 0.91610,
	0.92028, 0.92432, 0.92822, 0.93197, 0.93558, 0.93906, 0.94240, 0.94560,
	0.94867, 0.95162, 0.95444, 0.95713, 0.95971, 0.96217, 0.96451, 0.96674,
	0.96887, 0.97089, 0.97281, 0.97463, 0.97635, 0.97799, 0.97953, 0.98099,
	0.98236, 0.98366, 0.98488, 0.98602, 0.98710, 0.98811, 0.98905, 0.98994,
	0.99076, 0.99153, 0.99225, 0.99291, 0.99353, 0.99411, 0.99464, 0.99513,
	0.99558, 0.99600, 0.99639, 0.99674, 0.99706, 0.99736, 0.99763, 0.99788,
	0.99811, 0.99831, 0.99850, 0.99867, 0.99882, 0.99895, 0.99908, 0.99919,
	0.99929, 0.99938, 0.99946, 0.99953, 0.99959, 0.99965, 0.99969, 0.99974,
	0.99978, 0.99981, 0.99984, 0.99986, 0.99988, 0.99990, 0.99992, 0.99993,
	0.99994, 0.99995, 0.99996, 0.99997, 0.99998, 0.99998, 0.99998, 0.99999,
	0.99999, 0.99999, 0.99999, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000,
	1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000, 1.00000 };

static float *window128 = (float*)&window[128];
static float *window256 = (float*)&window[256];

static void swap_cmplx(complex_t *a, complex_t *b)
{
	complex_t tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}



static complex_t cmplx_mult(complex_t a, complex_t b)
{
	complex_t ret;

	ret.real = a.real * b.real - a.imag * b.imag;
	ret.imag = a.real * b.imag + a.imag * b.real;

	return ret;
}


void imdct_init(void)
{
	int i,k;
	complex_t angle_step;
	complex_t current_angle;

	/* Twiddle factors to turn IFFT into IMDCT */
	for( i=0; i < N/4; i++)
	{
		xcos1[i] = -cos(2 * M_PI * (8*i+1)/(8*N)) ; 
		xsin1[i] = -sin(2 * M_PI * (8*i+1)/(8*N)) ;
	}
	
	/* More twiddle factors to turn IFFT into IMDCT */
	for( i=0; i < N/8; i++)
	{
		xcos2[i] = -cos(2 * M_PI * (8*i+1)/(4*N)) ; 
		xsin2[i] = -sin(2 * M_PI * (8*i+1)/(4*N)) ;
	}

	/* Canonical twiddle factors for FFT */
	w[0] = w_1;
	w[1] = w_2;
	w[2] = w_4;
	w[3] = w_8;
	w[4] = w_16;
	w[5] = w_32;
	w[6] = w_64;

	for( i = 0; i < 7; i++)
	{
		angle_step.real = cos(-2.0 * M_PI / (1 << (i+1)));
		angle_step.imag = sin(-2.0 * M_PI / (1 << (i+1)));

		current_angle.real = 1.0;
		current_angle.imag = 0.0;

		for (k = 0; k < 1 << i; k++)
		{
			w[i][k] = current_angle;
			current_angle = cmplx_mult(current_angle,angle_step);
		}
	}
}



void
imdct_do_512(float x[],float y[],float delay[])
{
	int i,ii,k;
	int p,q;
	int m;
	int two_m;
	int two_m_plus_one;

	float tmp_a_i;
	float tmp_a_r;
	float tmp_b_i;
	float tmp_b_r;

	complex_t *buf_1P, *buf_2P;

	/* Pre IFFT complex multiply */
	ii = 0;
	for( i=0; i < N/4; i++)
	{
		k = N/2-ii-1;
		buf[i].real =(x[k] * xcos1[i])  -  (x[ii]       * xsin1[i]);
		buf[i].imag =-((x[ii] * xcos1[i])  +  (x[k] * xsin1[i]));
		ii+=2;
	}
	//Bit reversed shuffling
	for(i=0; i<N/4; i++) 
	{ 
		k = bit_reverse_512[i];
		if (k < i)
			swap_cmplx(&buf[i],&buf[k]);
	}

	/* FFT Merge */
	for (m=0; m < 7; m++)
	{
		float w_imag,w_real;

		two_m = (1 << m);
		two_m_plus_one = (1 << (m+1));

		for(k = 0; k < two_m; k++)
		{

			w_imag = w[m][k].imag;
			w_real = w[m][k].real;

			for(i = 0; i < 128; i += two_m_plus_one)
			{
				p = k + i;
				q = p + two_m;

				buf_1P = &buf[p];
				buf_2P = &buf[q];
				tmp_a_r = buf_1P->real;
				tmp_a_i = buf_1P->imag;
				tmp_b_r = buf_2P->real * w_real - buf_2P->imag * w_imag;
				tmp_b_i = buf_2P->imag * w_real + buf_2P->real * w_imag;
				buf_1P->real = tmp_a_r + tmp_b_r;
				buf_1P->imag = tmp_a_i + tmp_b_i;
				buf_2P->real = tmp_a_r - tmp_b_r;
				buf_2P->imag = tmp_a_i - tmp_b_i;
			}
		}
	}

	//cmplx conjugate
	buf_1P = buf;
	for( i=0; i < N/4; i++)
	{
		buf_1P->imag *= -1;				//buf[i].imag *= -1;
		buf_1P++;
	}

	/* Post IFFT complex multiply */
	for( i=0; i < N/4; i++)
	{
		/* y[n] = z[n] * (xcos1[n] + j * xsin1[n]) ; */
		buf_1P = &buf[i];
		tmp_a_r = buf_1P->real;
		tmp_a_i = buf_1P->imag;
		buf_1P->real =(tmp_a_r * xcos1[i])  -  (tmp_a_i  * xsin1[i]);
		buf_1P->imag =(tmp_a_r * xsin1[i])  +  (tmp_a_i  * xcos1[i]);
	}
	
	/* Window and convert to real valued signal */
	ii = 0;
	{
		int iip1;
		iip1 = 1;
		for(i=0; i<N/8; i++) 
		{
			//ii = i<<1;
			y[0   + ii]   = -buf[N/8+i].imag   * window[ ii]; 
			y[0   + iip1] =  buf[N/8-i-1].real * window[ ii+1]; 
			y[128 + ii]   = -buf[i].real       * window128[ ii]; 
			y[128 + iip1] =  buf[N/4-i-1].imag * window128[ ii+1]; 
			y[256 + ii]   = -buf[N/8+i].real   * window256[ - ii-1];
			y[256 + iip1] =  buf[N/8-i-1].imag * window256[ - ii-2];
			y[384 + ii]   =  buf[i].imag       * window128[ - ii-1];
			y[384 + iip1] = -buf[N/4-i-1].real * window128[ - ii-2];
			ii+=2;
			iip1+=2;
		}
	}

	/* Overlap and add */
	for(i=0; i< 256; i++) 
	{ 
		y[i] = 2 * (y[i] + delay[i]); 
		delay[i] = y[256 +i]; 
	}
}

void
imdct_do_256(float x[],float y[],float delay[])
{
	int i,k,ii;
	int p,q;
	int m;
	int two_m;
	int two_m_plus_one;

	float tmp_a_i;
	float tmp_a_r;
	float tmp_b_i;
	float tmp_b_r;

	float *data_ptr;
	float *delay_ptr;
	float *window_ptr;

	complex_t *buf_1, *buf_2;
	complex_t *buf_p, *buf_q;

	buf_1 = &buf[0];
	buf_2 = &buf[64];

	/* Pre IFFT complex multiply */
	for(k=0; k<N/8; k++) 
	{ 
		p = 2 * (N/4-2*k-1);
		q = 2 * (2 * k);

		buf_1[k].real = x[p] * xcos2[k] - x[q] * xsin2[k];
		buf_1[k].imag = -(x[q] * xcos2[k] + x[p] * xsin2[k]);
		buf_2[k].real = x[p + 1] * xcos2[k] - x[q + 1] * xsin2[k];
		buf_2[k].imag = -(x[q + 1] * xcos2[k] + x[p + 1] * xsin2[k]); 
	}

	//Bit reversed shuffling
	for(i=0; i<N/8; i++) 
	{ 
		k = bit_reverse_256[i];
		if (k < i)
		{
			swap_cmplx(&buf_1[i],&buf_1[k]);
			swap_cmplx(&buf_2[i],&buf_2[k]);
		}
	}

	/* FFT Merge */
	for (m=0; m < 6; m++)
	{
		two_m = (1 << m);
		two_m_plus_one = (1 << (m+1));

		for(k = 0; k < two_m; k++)
		{
			float w_imag = w[m][k].imag;
			float w_real = w[m][k].real;

			for(i = 0; i < 64; i += two_m_plus_one)
			{
				p = k + i;
				q = p + two_m;

				buf_p = &buf_1[p];
				buf_q = &buf_1[q];
				//Do block 1
				tmp_a_r = buf_p->real;
				tmp_a_i = buf_p->imag;
				tmp_b_r = buf_q->real * w_real - buf_q->imag * w_imag;
				tmp_b_i = buf_q->imag * w_real + buf_q->real * w_imag;
				buf_p->real = tmp_a_r + tmp_b_r;
				buf_p->imag = tmp_a_i + tmp_b_i;
				buf_q->real = tmp_a_r - tmp_b_r;
				buf_q->imag = tmp_a_i - tmp_b_i;

				buf_p = &buf_2[p];
				buf_q = &buf_2[q];
				//Do block 2
				tmp_a_r = buf_p->real;
				tmp_a_i = buf_p->imag;
				tmp_b_r = buf_q->real * w_real - buf_q->imag * w_imag;
				tmp_b_i = buf_q->imag * w_real + buf_q->real * w_imag;
				buf_p->real = tmp_a_r + tmp_b_r;
				buf_p->imag = tmp_a_i + tmp_b_i;
				buf_q->real = tmp_a_r - tmp_b_r;
				buf_q->imag = tmp_a_i - tmp_b_i;
			}
		}
	}

	//cmplx conjugate
	buf_p = buf_1;
	buf_q = buf_2;
	for(i=0; i<N/8; i++) 
	{
		buf_p->imag *= -1;
		buf_q->imag *= -1;
		buf_p++; buf_q++;
	}

	/* Post IFFT complex multiply */
	for( i=0; i < N/8; i++)
	{
		tmp_a_r = buf_1[i].real;
		tmp_a_i = buf_1[i].imag;
		buf_1[i].real =(tmp_a_r * xcos2[i])  -  (tmp_a_i  * xsin2[i]);
		buf_1[i].imag =(tmp_a_r * xsin2[i])  +  (tmp_a_i  * xcos2[i]);
		tmp_a_r = buf_2[i].real;
		tmp_a_i = buf_2[i].imag;
		buf_2[i].real =(tmp_a_r * xcos2[i])  -  (tmp_a_i  * xsin2[i]);
		buf_2[i].imag =(tmp_a_r * xsin2[i])  +  (tmp_a_i  * xcos2[i]);
	}
	
	data_ptr = x;
	delay_ptr = delay;
	window_ptr = (float*)window;

	/* Window and convert to real valued signal */
	for(i=0; i<N/8; i++) 
	{ 
		k = N/8-i-1;
		ii = i<<1;
		y[ii]        = -buf_1[i].imag * window[ii];
		y[ii+1]      =  buf_1[k].real * window[ii+1]; 
		y[N/4+ii]    = -buf_1[i].real * window128[ii]; 
		y[N/4+ii+1]  =  buf_1[k].imag * window128[ii+1];
		y[N/2+ii]    = -buf_2[i].real * window256[-ii-1]; 
		y[N/2+ii+1]  =  buf_2[k].imag * window256[-ii-2]; 
		y[3*N/4+ii]  =  buf_2[i].imag * window128[-ii-1]; 
		y[3*N/4+ii+1]= -buf_2[k].real * window128[-ii-2]; 
	}
	
	/* Overlap and add */
	for(i=0; i<N/2; i++) 
	{ 
		y[i] = 2 * (y[i] + delay[i]); 
		delay[i] = y[N/2+i]; 
	}
}


void  imdct(bsi_t *bsi,audblk_t *audblk, stream_coeffs_t *coeffs, stream_samples_t *samples)
{
	int i;

	for(i=0; i<bsi->nfchans;i++)
	{
		if(audblk->blksw[i])
			imdct_do_256(coeffs->fbw[i],samples->channel[i],delay[i]);
		else
			imdct_do_512(coeffs->fbw[i],samples->channel[i],delay[i]);
	}

	if (bsi->lfeon)
		imdct_do_512(coeffs->lfe,samples->channel[5],delay[5]);
}

