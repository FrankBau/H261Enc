#ifndef H251Enc_Intern_h
#define H251Enc_Intern_h

#include "H261Enc.h"

#include <assert.h>

typedef struct
{
	int x;
	int y;
} mv_t;

struct struct_H261Enc
{
	H261Enc_setup setup;
	H261Picture *currPict;
	H261Picture *prevPict;
	H261Picture *recoPict;

	uint8_t *buffer; /** H.261 stream buffer */
	int buffer_len;
	int buffer_used;

	uint32_t bit_buffer;
	int bit_buffer_used;

	mv_t pred_mv; /** motion vector predictor */
};

static inline int iabs(int x)
{
	return x < 0 ? -x : x;
}

static inline int imin(int x, int y)
{
	return x < y ? x : y;
}

static inline int imax(int x, int y)
{
	return x > y ? x : y;
}

#endif
