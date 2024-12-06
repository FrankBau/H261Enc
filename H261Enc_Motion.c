#include <stdio.h>

#include "H261Enc_Motion.h"
#include "H261Enc_Intern.h"

int calc_sad(uint8_t *currY, uint8_t *predY, int stride)
{
	int sad = 0;

	for (int i = 0; i < 16; i++)
	{
		for (int j = 0; j < 16; j++)
		{
			sad += iabs((int)*(currY + i * stride + j) - (int)*(predY + i * stride + j));
		}
	}
	return sad;
}

mv_t MotionEstimation(H261Enc *enc, int mb_row, int mb_col)
{
	mv_t mv;

	int width = enc->currPict->width;
	int height = enc->currPict->heigth;

	uint8_t *currY = enc->currPict->Y + width * 16 * mb_row + 16 * mb_col;
	uint8_t *predY = enc->prevPict->Y + width * 16 * mb_row + 16 * mb_col;

	int min_x = mb_col == 0 ? 0 : -15;				   /* leftmost MB */
	int max_x = 16 * (mb_col + 1) == width ? 0 : +15;  /* rightmost MB */
	int min_y = mb_row == 0 ? 0 : -15;				   /* topmost MB */
	int max_y = 16 * (mb_row + 1) == height ? 0 : +15; /* bottommost MB */

	/* we prefer the (0,0) vector which has the shortest code */
	int best_sad = calc_sad(currY, predY, width);
	int best_x = 0;
	int best_y = 0;

	for (int y = min_y; y < max_y; y++)
	{
		for (int x = min_x; x < max_x; x++)
		{
			int sad = calc_sad(currY, predY + width * y + x, width);
			if (sad < best_sad)
			{
				best_sad = sad;
				best_x = x;
				best_y = y;
			}
		}
	}

	mv.x = best_x;
	mv.y = best_y;

	return mv;
}
