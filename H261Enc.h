#ifndef H261Enc_h
#define H261Enc_h

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;

/** structure for a YUV 4:2:0 picture */
typedef struct struct_H261Picture
{
	int temporal_reference; /** time stamp in multiples of 29,97 Hz */

	uint8_t *Y;
	uint8_t *U;
	uint8_t *V;

	int width;	/** width of Y plane in pixels, must be 352 or 176 */
	int heigth; /** height of Y plane in pixels, must be 288 or 144 */

} H261Picture;

int H261Enc_AllocPicture(H261Picture **pict, int width, int height);

int H261Enc_FreePicture(H261Picture *pict);

/** H.261 encoder setup structure */
typedef struct
{
	int w; /** picture width, must be 352 */
	int h; /** picture height, must be 288 */
	int q; /** quantizer 1..31 */
} H261Enc_setup;

/** opaque H.261 encoder handle structure */
typedef struct struct_H261Enc H261Enc;

int H261Enc_Open(H261Enc **enc);

int H261Enc_GetSetup(H261Enc *enc, H261Enc_setup *setup);

int H261Enc_Setup(H261Enc *enc, H261Enc_setup *setup);

int H261Enc_EncodePicture(H261Enc *enc, uint8_t *buffer, int buffer_len, H261Picture *pict);

int H261Enc_Close(H261Enc *enc);

#endif
