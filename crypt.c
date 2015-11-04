/******************************************************************************
*
*	crypt.c
*
*	Internet Talk encrypt/decrypt messages using DES 
*
*	by Mark R Boyns
*	August 1991
*
*	Normal DES is used unless FASTDES is defined
*	Define CIPHER_BLOCK_CHAIN for cipher block chaining mode
*
*/

#include "DESdefs.h"

int encryptMessage(size, plaintext, ciphertext)
int size;
char *plaintext;
char *ciphertext;
{
	union LR_block data;
	int length = size, pad = 0;
#ifdef CIPHER_BLOCK_CHAIN
	static char previous[8];
	register int i;
#endif

	do {
		if (length > BYTES_PER_BLOCK) {
			bcopy(plaintext, data.string, BYTES_PER_BLOCK);
			plaintext+=BYTES_PER_BLOCK;
		}
		else {
			bcopy(plaintext, data.string, length);
			while(length < BYTES_PER_BLOCK) {
				data.string[length++] = '\0';
				pad++;
			}
		}
#ifdef CIPHER_BLOCK_CHAIN
		for(i=0; i<8; i++)
			data.string[i] ^= previous[i];
#endif
#ifdef FASTDES
		no_ip_des(&data, ENCRYPT);
#else
		des(&data, ENCRYPT);
#endif
		bcopy(data.string, ciphertext, BYTES_PER_BLOCK);
		ciphertext+=BYTES_PER_BLOCK;
#ifdef CIPHER_BLOCK_CHAIN
		bcopy(data.string, previous, 8);
#endif
	} while ((length -= BYTES_PER_BLOCK) > 0);

	return (size + pad);
}

int decryptMessage(size, ciphertext, plaintext)
int size;
char *ciphertext;
char *plaintext;
{
	union LR_block data;
	int length = size;
#ifdef CIPHER_BLOCK_CHAIN
	static char previous[8];
	register int i;
#endif

	do {
		bcopy(ciphertext, data.string, BYTES_PER_BLOCK);
#ifdef FASTDES
		no_ip_des(&data, DECRYPT);
#else
		des(&data, DECRYPT);
#endif
#ifdef CIPHER_BLOCK_CHAIN
		for(i=0; i<8; i++)
			data.string[i] ^= previous[i];
		bcopy(ciphertext, previous, 8);
#endif
		bcopy(data.string, plaintext, BYTES_PER_BLOCK);
		ciphertext+=BYTES_PER_BLOCK;
		plaintext+=BYTES_PER_BLOCK;
	} while ((length -= BYTES_PER_BLOCK) > 0);
}
