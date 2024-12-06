#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "H261Enc_Intern.h"
#include "H261Enc_Put.h"
#include "H261Enc_MacroBlock.h"

int H261Enc_AllocPicture(H261Picture **pict, int width, int height)
{
	assert((width == 176 && height == 144) || (width == 352 && height == 288));

	*pict = (H261Picture *)malloc(sizeof(H261Picture));

	(*pict)->temporal_reference = 0;

	(*pict)->Y = (uint8_t *)malloc(width * height);
	(*pict)->U = (uint8_t *)malloc(width / 2 * height / 2);
	(*pict)->V = (uint8_t *)malloc(width / 2 * height / 2);

	(*pict)->width = width;
	(*pict)->heigth = height;

	return 0;
}

int H261Enc_FreePicture(H261Picture *pict)
{
	free(pict->Y);
	pict->Y = NULL;
	free(pict->U);
	pict->U = NULL;
	free(pict->V);
	pict->V = NULL;

	return 0;
}

/*************************************************************************************/

int H261Enc_Open(H261Enc **enc)
{
	*enc = (H261Enc *)malloc(sizeof(H261Enc));
	memset(*enc, 0, sizeof(H261Enc));

	/* setup defaults */
	(*enc)->setup.w = 352;
	(*enc)->setup.h = 288;
	(*enc)->setup.q = 8;
	return 0;
}

int H261Enc_GetSetup(H261Enc *enc, H261Enc_setup *enc_setup)
{
	assert(enc != NULL);
	assert(enc_setup != NULL);

	memcpy(enc_setup, &enc->setup, sizeof(H261Enc_setup));
	return 0;
}

int H261Enc_Setup(H261Enc *enc, H261Enc_setup *enc_setup)
{
	assert(enc != NULL);
	assert(enc_setup != NULL);

	/*TODO: parameter validation */
	memcpy(&enc->setup, enc_setup, sizeof(H261Enc_setup));
	return 0;
}

int H261Enc_EncodePicture(H261Enc *enc, uint8_t *buffer, int buffer_len, H261Picture *pict)
{
	int tr = pict->temporal_reference & 0x1F; /* only 5 least significant bits of time stamp are encoded */

	enc->buffer = buffer;
	enc->buffer_len = buffer_len;
	enc->buffer_used = 0;

	enc->currPict = pict;

	if (enc->prevPict == NULL)
	{
		/* very first picture in sequence */
		H261Enc_AllocPicture(&enc->prevPict, 352, 288);
		H261Enc_AllocPicture(&enc->recoPict, 352, 288);
	}
	else
	{
		H261Picture *temp = enc->recoPict;
		enc->recoPict = enc->prevPict;
		enc->prevPict = temp;
	}

	printf("H261Enc: encode picture %4d\n", pict->temporal_reference);

	/* picture layer */
	PutBits(enc, 0x00010, 20); 	/* PSC */
	PutBits(enc, tr, 5);	  	/* TR */
	if(enc->currPict->temporal_reference == 0)
		PutBits(enc, 0x07, 6);	  /* PTYPE */
	else
		PutBits(enc, 0x07, 6);	  /* PTYPE */
	PutBits(enc, 0, 1);		  /* PEI */

	/* GOB layer */
	{
		int gob_number;

		for (gob_number = 1; gob_number <= 12; gob_number++)
		{
			int mb_number;
			int mba = 1;

			PutBits(enc, 0x001, 16);	   /* GBSC */
			PutBits(enc, gob_number, 4);   /* GN */
			PutBits(enc, enc->setup.q, 5); /* GQUANT */
			PutBits(enc, 0, 1);			   /* GEI */

			for (mb_number = 1; mb_number <= 33; mb_number++) /* macroblock layer */
			{
				int coded = H261Enc_EncodeMacroBlock(enc, gob_number, mb_number, mba);
				if (coded)
					mba = 1;
				else
					mba++;
			}
		}
	}

	if (0)
	{ /* output reconstructed video pictures */
		FILE *h;
		int W = 352;
		int H = 288;
		h = fopen("C:/h261out/reco.yuv", "ab+");
		fwrite(enc->recoPict->Y, W * H, 1, h);
		fwrite(enc->recoPict->U, W / 2 * H / 2, 1, h);
		fwrite(enc->recoPict->V, W / 2 * H / 2, 1, h);

		fclose(h);
	}

	return enc->buffer_used;
}

int H261Enc_Close(H261Enc *enc)
{
	while(enc->bit_buffer_used > 0) {
		PutBits(enc, 0, 1);
	}
	
	if (enc->prevPict != NULL)
	{
		H261Enc_FreePicture(enc->prevPict);
		enc->prevPict = NULL;
	}

	if (enc->recoPict != NULL)
	{
		H261Enc_FreePicture(enc->recoPict);
		enc->recoPict = NULL;
	}

	free(enc);
	return 0;
}
