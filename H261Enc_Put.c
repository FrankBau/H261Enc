#include "H261Enc_Put.h"
#include <assert.h>

typedef struct
{
	uint8_t code;
	uint8_t len;
} vlc_t;

/** output a number of bits */
void PutBits(H261Enc *enc, uint32_t bit_pattern, int size)
{
	int pos = size;

	assert(bit_pattern < (1U << size));
	do
	{
		pos--;
		enc->bit_buffer = (enc->bit_buffer << 1) | (1 & (bit_pattern >> pos));
		enc->bit_buffer_used++;
		if (enc->bit_buffer_used == 8)
		{
			enc->buffer[enc->buffer_used] = 0xFF & enc->bit_buffer;
			enc->buffer_used++;

			enc->bit_buffer = 0;
			enc->bit_buffer_used = 0;
		}

	} while (pos > 0);
}

void PutDC(H261Enc *enc, int dc)
{
	assert(0 < dc && dc < 255);
	PutBits(enc, dc == 128 ? 255 : dc, 8); /* cf. TABLE 6/H.261 */
}

/** TABLE 5/H.261 */
void PutAC(H261Enc *enc, int run, int level)
{
	/* TABLE  5/H.261 */
	static const vlc_t table5[64][128] =
		{
			{0, 0, 6, 3, 8, 5, 10, 6, 12, 8, 76, 9, 66, 9, 20, 11, 58, 13, 48, 13, 38, 13, 32, 13, 52, 14, 50, 14, 48, 14, 46, 14},
			{0, 0, 6, 4, 12, 7, 74, 9, 24, 11, 54, 13, 44, 14, 42, 14},
			{0, 0, 10, 5, 8, 8, 22, 11, 40, 13, 40, 14},
			{0, 0, 14, 6, 72, 9, 56, 13, 38, 14},
			{0, 0, 12, 6, 30, 11, 36, 13},
			{0, 0, 14, 7, 18, 11, 36, 14},
			{0, 0, 10, 7, 60, 13},
			{0, 0, 8, 7, 42, 13},
			{0, 0, 14, 8, 34, 13},
			{0, 0, 10, 8, 34, 14},
			{0, 0, 78, 9, 32, 14},
			{0, 0, 70, 9},
			{0, 0, 68, 9},
			{0, 0, 64, 9},
			{0, 0, 28, 11},
			{0, 0, 26, 11},
			{0, 0, 16, 11},
			{0, 0, 62, 13},
			{0, 0, 52, 13},
			{0, 0, 50, 13},
			{0, 0, 46, 13},
			{0, 0, 44, 13},
			{0, 0, 62, 14},
			{0, 0, 60, 14},
			{0, 0, 58, 14},
			{0, 0, 56, 14},
			{0, 0, 54, 14},
		};

	assert(0 <= run && run <= 63);
	assert(-127 <= level && level <= 127);
	assert(level != 0);

	if (table5[run][iabs(level)].len != 0)
	{
		int sign = level < 0;
		assert(sign == 0 || sign == 1);
		PutBits(enc, table5[run][iabs(level)].code | sign, table5[run][iabs(level)].len);
	}
	else
	{
		/* 20-bit ESC code */
		PutBits(enc, 1, 6);
		PutBits(enc, (uint32_t)run, 6);
		PutBits(enc, 0xFF & (uint32_t)level, 8);
	}
}

/* this is used for INTRA block coding */
void PutRunLevel(H261Enc *enc, int8_t runlevel[129])
{
	int coeffs = *runlevel++; /* number of non-zero TCOEFFs */

	int run = *runlevel++;
	int level = *runlevel++;
	int sign = level < 0;

	assert(0 < coeffs && coeffs <= 64); /* must be at least one coeff because block is coded */
	assert(sign == 0 || sign == 1);

	if (run == 0 && iabs(level) == 1) /* first coeff, cannot be EOB */
	{
		PutBits(enc, (1 << 1) | sign, 2);
	}
	else
	{
		PutAC(enc, run, level);
	}

	for (int i = 1; i < coeffs; i++)
	{
		run = *runlevel++;
		level = *runlevel++;
		PutAC(enc, run, level);
	}
	PutBits(enc, 2, 2); /* EOB */
}

/** macro block address */
void PutMBA(H261Enc *enc, int mba)
{
	/* TABLE  1/H.261 */
	static const vlc_t table1[34] =
		{
			0x00, 0, 0x01, 1, 0x03, 3, 0x02, 3, 0x03, 4, 0x02, 4, 0x03, 5, 0x02, 5,
			0x07, 7, 0x06, 7, 0x0B, 8, 0x0A, 8, 0x09, 8, 0x08, 8, 0x07, 8, 0x06, 8,
			0x17, 10, 0x16, 10, 0x15, 10, 0x14, 10, 0x13, 10, 0x12, 10, 0x23, 11, 0x22, 11,
			0x21, 11, 0x20, 11, 0x1F, 11, 0x1E, 11, 0x1D, 11, 0x1C, 11, 0x1B, 11, 0x1A, 11,
			0x19, 11, 0x18, 11};

	assert(0 < mba && mba <= 33); /* 0 is not allowed */
	PutBits(enc, table1[mba].code, table1[mba].len);
}

/** coded block pattern */
void PutCBP(H261Enc *enc, int cbp)
{
	/* TABLE  4/H.261 */
	static const vlc_t table4[64] =
		{
			0x00, 0, 0x0B, 5, 0x09, 5, 0x0D, 6, 0x0D, 4, 0x17, 7, 0x13, 7, 0x1F, 8,
			0x0C, 4, 0x16, 7, 0x12, 7, 0x1E, 8, 0x13, 5, 0x1B, 8, 0x17, 8, 0x13, 8,
			0x0B, 4, 0x15, 7, 0x11, 7, 0x1D, 8, 0x11, 5, 0x19, 8, 0x15, 8, 0x11, 8,
			0x0F, 6, 0x0F, 8, 0x0D, 8, 0x03, 9, 0x0F, 5, 0x0B, 8, 0x07, 8, 0x07, 9,
			0x0A, 4, 0x14, 7, 0x10, 7, 0x1C, 8, 0x0E, 6, 0x0E, 8, 0x0C, 8, 0x02, 9,
			0x10, 5, 0x18, 8, 0x14, 8, 0x10, 8, 0x0E, 5, 0x0A, 8, 0x06, 8, 0x06, 9,
			0x12, 5, 0x1A, 8, 0x16, 8, 0x12, 8, 0x0D, 5, 0x09, 8, 0x05, 8, 0x05, 9,
			0x0C, 5, 0x08, 8, 0x04, 8, 0x04, 9, 0x07, 3, 0x0A, 5, 0x08, 5, 0x0C, 6};

	assert(0 < cbp && cbp <= 63); /* 0 is not allowed because macroblock must then be skipped */
	PutBits(enc, table4[cbp].code, table4[cbp].len);
}

/** put motion vector mv w.r.t. predictor pred_mv */
void PutMVD(H261Enc *enc, mv_t mv, mv_t pred_mv)
{
	static const vlc_t table3[32] =
		{
			0x01, 1, 0x02, 3, 0x02, 4, 0x02, 5, 0x06, 7, 0x0A, 8, 0x08, 8, 0x06, 8,
			0x16, 10, 0x14, 10, 0x12, 10, 0x22, 11, 0x20, 11, 0x1E, 11, 0x1C, 11, 0x1A, 11,
			0x19, 11, 0x1B, 11, 0x1D, 11, 0x1F, 11, 0x21, 11, 0x23, 11, 0x13, 10, 0x15, 10,
			0x17, 10, 0x07, 8, 0x09, 8, 0x0B, 8, 0x07, 7, 0x03, 5, 0x03, 4, 0x03, 3};

	int dx = mv.x - pred_mv.x;
	int dy = mv.y - pred_mv.y;

	/* motion vectors 16 and -16, -15 and 17, ... 14 and-18, 15 and -17 share the same VLC code */
	if (dx < 0)
		dx = 32 + dx;
	if (dy < 0)
		dy = 32 + dy;

	assert(0 <= dx && dx < 32);
	assert(0 <= dy && dy < 32);

	PutBits(enc, table3[dx].code, table3[dx].len);
	PutBits(enc, table3[dy].code, table3[dy].len);
}
