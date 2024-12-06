#ifndef H251Enc_dct_h
#define H251Enc_dct_h

#include "H261Enc_Intern.h"

void fdct(int coeff[64], int block[64]);

void idct(int block[64], int coeff[64]);

#endif
