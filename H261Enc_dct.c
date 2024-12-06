/* code taken from ITU - Telecommunications Standardization Sector STUDY GROUP 16
 * Video Coding Experts Group (Question 15) Document Q15-H-29, slightly adapted
 */

#define _USE_MATH_DEFINES
#include <math.h>

#include "H261Enc_dct.h"

const double M_PI_16 = M_PI / 16.0;

static int round_int(double x)
{
	return x > 0 ? (int)(x + 0.5) : (int)(x - 0.5);
}

// co[x][u] = cos(M_PI_16 * (2 * x + 1) * u)
const double co[8][8] = {
	1.000000000,	 0.980785280,	 0.923879533,	 0.831469612,	 0.707106781,	 0.555570233,	 0.382683432,	 0.195090322,
	1.000000000,	 0.831469612,	 0.382683432,	-0.195090322,	-0.707106781,	-0.980785280,	-0.923879533,	-0.555570233,
	1.000000000,	 0.555570233,	-0.382683432,	-0.980785280,	-0.707106781,	 0.195090322,	 0.923879533,	 0.831469612,
	1.000000000,	 0.195090322,	-0.923879533,	-0.555570233,	 0.707106781,	 0.831469612,	-0.382683432,	-0.980785280,
	1.000000000,	-0.195090322,	-0.923879533,	 0.555570233,	 0.707106781,	-0.831469612,	-0.382683432,	 0.980785280,
	1.000000000,	-0.555570233,	-0.382683432,	 0.980785280,	-0.707106781,	-0.195090322,	 0.923879533,	-0.831469612,
	1.000000000,	-0.831469612,	 0.382683432,	 0.195090322,	-0.707106781,	 0.980785280,	-0.923879533,	 0.555570233,
	1.000000000,	-0.980785280,	 0.923879533,	-0.831469612,	 0.707106781,	-0.555570233,	 0.382683432,	-0.195090322,
};


/** reference DCT */
void fdct(int coeff[64], int block[64])
{
	for (int u = 0; u < 8; u++)
	{
		double C_u = (u > 0 ? 1 : M_SQRT1_2);
		for (int v = 0; v < 8; v++)
		{
			double C_v = (v > 0 ? 1 : M_SQRT1_2);
			double tmp_x = 0.0;
			for (int x = 0; x < 8; x++)
			{
				double tmp_y = 0.0;
				for (int y = 0; y < 8; y++)
				{
					tmp_y += block[8 * x + y] * co[y][v];
				}
				tmp_x += tmp_y * co[x][u];
			}
			coeff[8 * u + v] = round_int(tmp_x / 4.0 * C_u * C_v);
		}
	}
}

/** reference IDCT */
void idct(int block[64], int coeff[64])
{
	for (int x = 0; x < 8; x++)
	{
		for (int y = 0; y < 8; y++)
		{
			double tmp_u = 0.0;
			for (int u = 0; u < 8; u++)
			{
				double C_u = (u > 0 ? 1 : M_SQRT1_2);
				double tmp_v = 0.0;
				for (int v = 0; v < 8; v++)
				{
					double C_v = (v > 0 ? 1 : M_SQRT1_2);
					tmp_v += C_v * coeff[8 * u + v] *  co[y][v];
				}
				tmp_u += tmp_v * C_u * co[x][u];
			}
			block[8 * x + y] = round_int(tmp_u / 4.0);
		}
	}
}
