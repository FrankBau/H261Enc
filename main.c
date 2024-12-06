#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "H261Enc.h"

#define W 352
#define H 288

void usage()
{
	printf(
		"usage: [quantizer] input.yuv output.h261\n"
		"        quantizer: integer 1..31\n");
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
	H261Enc *enc;
	H261Enc_setup enc_setup;
	H261Picture *pict;

	printf("H.261 encoder\n");

	H261Enc_AllocPicture(&pict, W, H);
	H261Enc_Open(&enc);
	H261Enc_GetSetup(enc, &enc_setup);

	// parameters
	char *filename_yuv = "football_cif.yuv";
	char *filename_261 = "football_cif.h261";
	int nframes = 600;	// number of frames to encode
	enc_setup.q =   5; 	// quantizer (1..31)

	int rc = H261Enc_Setup(enc, &enc_setup);
	if (rc < 0)
	{
		fprintf(stderr, "H261Enc_Setup failed with error %d\n", rc);
		return rc;
	}

	printf("encode %s -> %s\n", filename_yuv, filename_261);

	FILE *f = fopen(filename_yuv, "rb");
	if (f == NULL)
	{
		perror(filename_yuv);
		return -1;
	}

	FILE *g = fopen(filename_261, "wb");
	if (g == NULL)
	{
		perror(filename_261);
		return -1;
	}

	int temporal_reference = 0;
	for (int i=0; i<nframes; ++i)
	{
		uint8_t buffer[4 * W * H]; // upper limit for encoed picture size
		int buffer_len = sizeof(buffer);

		if (fread(pict->Y, W * H, 1, f) != 1)
			break;
		if (fread(pict->U, W / 2 * H / 2, 1, f) != 1)
			break;
		if (fread(pict->V, W / 2 * H / 2, 1, f) != 1)
			break;

		pict->temporal_reference = temporal_reference++;

		rc = H261Enc_EncodePicture(enc, buffer, buffer_len, pict);
		if (rc <= 0)
		{
			fprintf(stderr, "H261Enc_EncodePicture failed with error %d\n", rc);
			break;
		}

		if (fwrite(buffer, rc, 1, g) != 1)
		{
			perror("fwrite error");
			return errno;
		}
	}

	H261Enc_Close(enc);
	H261Enc_FreePicture(pict);

	fclose(f);
	fclose(g);

	return 0;
}
