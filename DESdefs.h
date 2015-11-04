/******************************************************************************
*
*	DESdefs.h
*
*/

/*
 * Separate from DES.h to allow other files to access the definitions
 */

#define NOSHIFT 		0
#define SHIFT 			1
#define ENCRYPT		0
#define DECRYPT		1
#define BYTES_PER_BLOCK  8

#ifndef BIG_ENDIAN
#define LITTLE_ENDIAN
#endif
#define BIT(x)	( 1 << x )

#define KEY_BYTES		8
#define CD_BITS			28
#define KEY_SCHEDULE		16
#define AA 0
#define BB 1
#define RR 1
#define LL 0

union LR_block {
	unsigned char string[9];
	unsigned long LR[2];
};
