#include "H261Enc_Block.h"

#include "H261Enc_dct.h"
#include "H261Enc_Put.h"

#include <stdio.h>

uint8_t clip255(int x)
{
	if (x < 0)
		return 0;
	if (x > 255)
		return 255;
	return (uint8_t)x;
}

int clip127(int x)
{
	if (x < -127)
		return -127;
	if (x > 127)
		return 127;
	return x;
}

/* debugging aid */
void print_block(char *text, int block[64])
{
	printf("%s: ", text);
	for (int i = 0; i < 64; i++)
	{
		if ((i % 8) == 0)
			printf("| ");
		else
			printf(", ");

		printf("%4d", block[i]);
	}
	printf("\n");
}

/* debugging aid */
void print_block_stride(char *text, uint8_t *block, int stride)
{
	printf("%s: ", text);
	for (int i = 0; i < 64; i++)
	{
		if ((i % 8) == 0)
			printf("| ");
		else
			printf(", ");

		printf("%4d", block[(i % 8) + (i / 8) * stride]);
	}
	printf("\n");
}

/** load a 8x8 block from a picture plane */
void load(int block[64], uint8_t *x, int stride)
{
	int *dst = block;

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			*dst++ = x[j];
		}
		x += stride;
	}
}

void load_diff(int block[64], uint8_t *curr, uint8_t *prev, int stride)
{
	int *dst = block;

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			*dst++ = (int)curr[j] - (int)prev[j]; /* difference must be calculated signed */
		}
		curr += stride;
		prev += stride;
	}
}

/** store a 8x8 block to a picture plane */
void store(uint8_t *x, int stride, int block[64])
{
	int *src = block;

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			x[j] = clip255(*src++); /* saturation */
		}
		x += stride;
	}
}

void store_sum(uint8_t *reco, uint8_t *prev, int stride, int block[64])
{
	int *src = block;

	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			reco[j] = clip255(*src++ + prev[j]);
		}
		reco += stride;
		prev += stride;
	}
}

void quant_intra(int level[64], int coeff[64], int qp)
{
	/*
	 * The exact formula is not defined in the H.261 recommendation, it's up to the implementor
	 * a common, reasonable choice is Q(AC) = clip127( sign*( abs(AC)/2QP );
	 */
	int dead_zone = 0;

	level[0] = (coeff[0] + 4) / 8; /* DC, clipped to range 1..254 */
	if (level[0] < 1)
		level[0] = 1;
	if (level[0] > 254)
		level[0] = 254;

	for (int i = 1; i < 64; i++)
	{
		if (coeff[i] > dead_zone)
			level[i] = clip127((coeff[i] - dead_zone) / (2 * qp));
		else if (coeff[i] < -dead_zone)
			level[i] = -clip127((-coeff[i] - dead_zone) / (2 * qp));
		else
			level[i] = 0;
	}
}

void quant_inter(int level[64], int coeff[64], int qp)
{
	/*
	 * The exact formula is not defined in the H.261 recommendation, it's up to the implementor
	 * a common, reasonable choice is Q(AC) =	(AC-QP/2)/2QP );
	 */
	int dead_zone = 0;

	for (int i = 0; i < 64; i++)
	{
		if (coeff[i] > dead_zone)
			level[i] = clip127((coeff[i] - dead_zone) / (2 * qp));
		else if (coeff[i] < -dead_zone)
			level[i] = -clip127((-coeff[i] - dead_zone) / (2 * qp));
		else
			level[i] = 0;
	}
}

void loop_filter(int y[64], int x[64])
{
	y[0 * 8 + 0] = x[0 * 8 + 0]; /* top-left corner */
	for (int i = 1; i < 7; i++)	 /* top row, inner values */
		y[0 * 8 + i] = (x[0 * 8 + i - 1] + 2 * x[0 * 8 + i] + x[0 * 8 + i + 1] + 2) / 4;
	y[0 * 8 + 7] = x[0 * 8 + 7]; /* top-right corner */

	for (int j = 1; j < 7; j++) /* inner rows */
	{
		y[j * 8 + 0] = (x[j * 8 + 0] + 2 * x[(j - 1) * 8 + 0] + x[(j + 1) * 8 + 0] + 2) / 4; /* left corner */
		for (int i = 1; i < 7; i++)															 /* inner values */
			y[j * 8 + i] = (x[(j - 1) * 8 + i - 1] + 2 * x[(j - 1) * 8 + i] + x[(j - 1) * 8 + i + 1] + 2 * x[(j) * 8 + i - 1] + 4 * x[(j) * 8 + i] + 2 * x[(j) * 8 + i + 1] + x[(j + 1) * 8 + i - 1] + 2 * x[(j + 1) * 8 + i] + x[(j + 1) * 8 + i + 1] + 8) / 16;
		y[j * 8 + 7] = (x[j * 8 + 7] + 2 * x[(j - 1) * 8 + 7] + x[(j + 1) * 8 + 7] + 2) / 4; /* right corner */
	}

	y[7 * 8 + 0] = x[7 * 8 + 0]; /* bottom-left corner */
	for (int i = 1; i < 7; i++)	 /* bottom row, inner values */
		y[7 * 8 + i] = (x[7 * 8 + i - 1] + 2 * x[7 * 8 + i] + x[7 * 8 + i + 1] + 2) / 4;
	y[7 * 8 + 7] = x[7 * 8 + 7]; /* bottom-right corner */
}

void dequant_intra(int coeff[64], int level[64], int qp)
{
	/* H.261 says for ACs: */
	/* REC = QUANT*(2*LEVEL+1)   for LEVEL>0 and  odd(QUANT) */
	/* REC = QUANT*(2*LEVEL-1)   for LEVEL<0 and  odd(QUANT) */
	/* REC = QUANT*(2*LEVEL+1)-1 for LEVEL>0 and !odd(QUANT) */
	/* REC = QUANT*(2*LEVEL-1)+1 for LEVEL<0 and !odd(QUANT) */
	/* REC is finally clipped to -2048..+2047 */

	coeff[0] = 8 * level[0]; /* TABLE 6/H.261 */
	for (int i = 1; i < 64; i++)
	{
		if (qp & 1)
		{
			if (level[i] > 0)
				coeff[i] = qp * (2 * level[i] + 1);
			else if (level[i] < 0)
				coeff[i] = qp * (2 * level[i] - 1);
			else
				coeff[i] = 0;
		}
		else
		{
			if (level[i] > 0)
				coeff[i] = qp * (2 * level[i] + 1) - 1;
			else if (level[i] < 0)
				coeff[i] = qp * (2 * level[i] - 1) + 1;
			else
				coeff[i] = 0;
		}
	}
}

void dequant_inter(int coeff[64], int level[64], int qp)
{
	/* H.261 says for ACs: */
	/* REC = QUANT*(2*LEVEL+1)   for LEVEL>0 and  odd(QUANT) */
	/* REC = QUANT*(2*LEVEL-1)   for LEVEL<0 and  odd(QUANT) */
	/* REC = QUANT*(2*LEVEL+1)-1 for LEVEL>0 and !odd(QUANT) */
	/* REC = QUANT*(2*LEVEL-1)+1 for LEVEL<0 and !odd(QUANT) */
	/* REC is finally clipped to -2048..+2047 */

	for (int i = 0; i < 64; i++)
	{
		if (qp & 1)
		{
			if (level[i] > 0)
				coeff[i] = qp * (2 * level[i] + 1);
			else if (level[i] < 0)
				coeff[i] = qp * (2 * level[i] - 1);
			else
				coeff[i] = 0;
		}
		else
		{
			if (level[i] > 0)
				coeff[i] = qp * (2 * level[i] + 1) - 1;
			else if (level[i] < 0)
				coeff[i] = qp * (2 * level[i] - 1) + 1;
			else
				coeff[i] = 0;
		}
	}
}

void zig(int zigzag[64], int level[64])
{
	/* FIGURE 12/H.261 */
	int *z = zigzag - 1; /* z is a index 1 based view of zigzag */
	int *x = level;
	z[1] = *x++;
	z[2] = *x++;
	z[6] = *x++;
	z[7] = *x++;
	z[15] = *x++;
	z[16] = *x++;
	z[28] = *x++;
	z[29] = *x++;
	z[3] = *x++;
	z[5] = *x++;
	z[8] = *x++;
	z[14] = *x++;
	z[17] = *x++;
	z[27] = *x++;
	z[30] = *x++;
	z[43] = *x++;
	z[4] = *x++;
	z[9] = *x++;
	z[13] = *x++;
	z[18] = *x++;
	z[26] = *x++;
	z[31] = *x++;
	z[42] = *x++;
	z[44] = *x++;
	z[10] = *x++;
	z[12] = *x++;
	z[19] = *x++;
	z[25] = *x++;
	z[32] = *x++;
	z[41] = *x++;
	z[45] = *x++;
	z[54] = *x++;
	z[11] = *x++;
	z[20] = *x++;
	z[24] = *x++;
	z[33] = *x++;
	z[40] = *x++;
	z[46] = *x++;
	z[53] = *x++;
	z[55] = *x++;
	z[21] = *x++;
	z[23] = *x++;
	z[34] = *x++;
	z[39] = *x++;
	z[47] = *x++;
	z[52] = *x++;
	z[56] = *x++;
	z[61] = *x++;
	z[22] = *x++;
	z[35] = *x++;
	z[38] = *x++;
	z[48] = *x++;
	z[51] = *x++;
	z[57] = *x++;
	z[60] = *x++;
	z[62] = *x++;
	z[36] = *x++;
	z[37] = *x++;
	z[49] = *x++;
	z[50] = *x++;
	z[58] = *x++;
	z[59] = *x++;
	z[63] = *x++;
	z[64] = *x++;
}

void H261Enc_EncodeIntraBlock(H261Enc *enc, uint8_t *curr, uint8_t *reco, int stride)
{
	int block[64];
	int coeff[64];
	int level[64];
	int zigzag[64];

	load(block, curr, stride);
	fdct(coeff, block);
	quant_intra(level, coeff, enc->setup.q);
	zig(zigzag, level);
	PutDC(enc, level[0]); /* DC */
	int run = 0;
	for (int i = 1; i < 64; i++)
	{
		if (zigzag[i] != 0)
		{
			PutAC(enc, run, zigzag[i]);
			run = 0;
		}
		else
		{
			run++;
		}
	}
	PutBits(enc, 2, 2); /* EOB */

	/* reconstruction (lossless stages are skipped) */
	dequant_intra(coeff, level, enc->setup.q);
	idct(block, coeff);
	store(reco, stride, block);
}

/** output run,level pairs to runlevel */
int H261Enc_EncodeInterBlock(H261Enc *enc, int8_t runlevel[129], uint8_t *curr, uint8_t *prev, uint8_t *reco, int stride)
{
	int nonzero = 0;

	int block[64];
	int coeff[64];
	int level[64];
	int zigzag[64];

	load_diff(block, curr, prev, stride);
	fdct(coeff, block);
	quant_inter(level, coeff, enc->setup.q);
	zig(zigzag, level);

	int run = 0;
	int j = 1;
	for (int i = 0; i < 64; i++)
	{
		if (zigzag[i] != 0)
		{
			runlevel[j++] = run;
			runlevel[j++] = zigzag[i];
			run = 0;
			nonzero++;
		}
		else
		{
			run++;
		}
	}
	runlevel[0] = nonzero;

	/* reconstruction (lossless stages are skipped) */
	dequant_inter(coeff, level, enc->setup.q);
	idct(block, coeff);
	store_sum(reco, prev, stride, block);

	return (nonzero > 0) ? 1 : 0;
}
