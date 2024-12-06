#ifndef H261Enc_Put_h
#define H261Enc_Put_h

#include "H261Enc_Intern.h"

void PutBits(H261Enc *enc, uint32_t bit_pattern, int size);

void PutMBA(H261Enc *enc, int mba);

void PutCBP(H261Enc *enc, int cbp);

void PutDC(H261Enc *enc, int dc);

void PutAC(H261Enc *enc, int run, int level);

void PutRunLevel(H261Enc *enc, int8_t runlevel[129]);

void PutMVD(H261Enc *enc, mv_t mv, mv_t pred_mv);

#endif
