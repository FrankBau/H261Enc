#include "H261Enc_MacroBlock.h"

#include "H261Enc_Put.h"
#include "H261Enc_Block.h"
#include "H261Enc_Motion.h"

/** return 0 for skipped macroblock, 1 else */
int H261Enc_EncodeMacroBlock(H261Enc *enc, int gob_number, int mb_number, int mba)
{
	/* cf. FIGURE 6/H.261 and FIGURE 8/H.261 */
	int gob_div2 = (gob_number - 1) / 2;
	int gob_mod2 = (gob_number - 1) % 2;

	int mb_div11 = (mb_number - 1) / 11;
	int mb_mod11 = (mb_number - 1) % 11;

	int row = 3 * gob_div2 + mb_div11;
	int col = 11 * gob_mod2 + mb_mod11;

	int width = enc->currPict->width;

	uint8_t *currY = enc->currPict->Y + width * 16 * row + 16 * col;
	uint8_t *recoY = enc->recoPict->Y + width * 16 * row + 16 * col;

	uint8_t *currU = enc->currPict->U + width / 2 * 8 * row + 8 * col;
	uint8_t *recoU = enc->recoPict->U + width / 2 * 8 * row + 8 * col;

	uint8_t *currV = enc->currPict->V + width / 2 * 8 * row + 8 * col;
	uint8_t *recoV = enc->recoPict->V + width / 2 * 8 * row + 8 * col;

	// printf("tr=%8d; gob=%2d mb=%2d\n", pict->temporal_reference, gob_number, mb_number );

	/* This is a raw estimate. One block has max 64 TCOEFF, one 1 TOEFF max. 20 Bit plus EOB plus headers... */
	/* checking the value once per macroblock is much cheaper than checking in lower H.261 layers */
	assert(enc->buffer_used + 2048 < enc->buffer_len);

	/* 4.2.3.4 Motion vector data - predictor */
	if ((mb_mod11 == 0) || (mba != 1) /* || (MTYPE of the previous macroblock was not MC) */)
	{
		enc->pred_mv.x = enc->pred_mv.y = 0;
	}

	/* mode decisions go here */

	if (enc->currPict->temporal_reference == 0)
	{
		/* INTRA mode */
		PutMBA(enc, mba);	/* VLC code for MBA */
		PutBits(enc, 1, 4); /* VLC code for MTYPE=Intra */
							/* MQUANT - not present */
							/* MVD - not present */
							/* CBP - not present */

		/* FIGURE 10/H.261 */
		H261Enc_EncodeIntraBlock(enc, currY, recoY, width);
		H261Enc_EncodeIntraBlock(enc, currY + 8, recoY + 8, width);

		H261Enc_EncodeIntraBlock(enc, currY + 8 * width, recoY + 8 * width, width);
		H261Enc_EncodeIntraBlock(enc, currY + 8 * width + 8, recoY + 8 * width + 8, width);

		H261Enc_EncodeIntraBlock(enc, currU, recoU, width / 2);
		H261Enc_EncodeIntraBlock(enc, currV, recoV, width / 2);

		return -1;
	}
	else
	{
		/* INTER mode */
		mv_t mv = MotionEstimation(enc, row, col);

		/* H.261: The motion vector for both colour difference blocks is derived by halving the component
		 * values of the macroblock vector and truncating the magnitude parts towards zero to yield integer components.
		 */
		int mv_chroma_x = (mv.x < 0) ? -((-mv.x) >> 1) : (mv.x >> 1);
		int mv_chroma_y = (mv.y < 0) ? -((-mv.y) >> 1) : (mv.y >> 1);

		uint8_t *prevY = enc->prevPict->Y + width * (16 * row + mv.y) + 16 * col + mv.x;
		uint8_t *prevU = enc->prevPict->U + width / 2 * (8 * row + mv_chroma_y) + 8 * col + mv_chroma_x;
		uint8_t *prevV = enc->prevPict->V + width / 2 * (8 * row + mv_chroma_y) + 8 * col + mv_chroma_x;

		int8_t runlevel1[129];
		int8_t runlevel2[129];
		int8_t runlevel3[129];
		int8_t runlevel4[129];
		int8_t runlevel5[129];
		int8_t runlevel6[129];

		int P1 = H261Enc_EncodeInterBlock(enc, runlevel1, currY, prevY, recoY, width);
		int P2 = H261Enc_EncodeInterBlock(enc, runlevel2, currY + 8, prevY + 8, recoY + 8, width);

		int P3 = H261Enc_EncodeInterBlock(enc, runlevel3, currY + 8 * width, prevY + 8 * width, recoY + 8 * width, width);
		int P4 = H261Enc_EncodeInterBlock(enc, runlevel4, currY + 8 * width + 8, prevY + 8 * width + 8, recoY + 8 * width + 8, width);

		int P5 = H261Enc_EncodeInterBlock(enc, runlevel5, currU, prevU, recoU, width / 2);
		int P6 = H261Enc_EncodeInterBlock(enc, runlevel6, currV, prevV, recoV, width / 2);

		int cbp = 32 * P1 + 16 * P2 + 8 * P3 + 4 * P4 + 2 * P5 + P6; /* 4.2.3.5 coded block pattern */

		if (cbp == 0)
		{
			if (mv.x == 0 && mv.y == 0)
				return 0; /* MB not coded */

			PutMBA(enc, mba);			   /* VLC code for MBA */
			PutBits(enc, 1, 9);			   /* VLC code for MTYPE=Inter+MC noCBP noTCOEFF */
			PutMVD(enc, mv, enc->pred_mv); /* MVD */
			enc->pred_mv = mv;			   /* mv is new predictor */
			return 1;
		}
		else
		{
			PutMBA(enc, mba); /* VLC code for MBA */

			if (mv.x == 0 && mv.y == 0)
			{
				PutBits(enc, 1, 1); /* VLC code for MTYPE=Inter noMVD CBP TCOEFF */
			}
			else
			{
				PutBits(enc, 1, 8); /* VLC code for MTYPE=Inter+MC */
				PutMVD(enc, mv, enc->pred_mv);
			}

			PutCBP(enc, cbp); /* CBP */
			if (P1)
				PutRunLevel(enc, runlevel1);
			if (P2)
				PutRunLevel(enc, runlevel2);
			if (P3)
				PutRunLevel(enc, runlevel3);
			if (P4)
				PutRunLevel(enc, runlevel4);
			if (P5)
				PutRunLevel(enc, runlevel5);
			if (P6)
				PutRunLevel(enc, runlevel6);

			enc->pred_mv = mv;

			return 1;
		}
	}
}
