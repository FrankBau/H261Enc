#ifndef H261Enc_Block_h
#define H261Enc_Block_h

#include "H261Enc_Intern.h"

void H261Enc_EncodeIntraBlock(H261Enc *enc, uint8_t *curr, uint8_t *reco, int stride);

int H261Enc_EncodeInterBlock(H261Enc *enc, int8_t runlevel[129], uint8_t *curr, uint8_t *prev, uint8_t *reco, int stride);

/* internal functions listed for building tests */

void quant_intra(int level[64], int coeff[64], int qp);
void quant_inter(int level[64], int coeff[64], int qp);

void dequant_intra(int coeff[64], int level[64], int qp);
void dequant_inter(int coeff[64], int level[64], int qp);

#endif
