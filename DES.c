/*
 * Copyright (c) 1990 David G. Koontz.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms.  Inclusion in a product or release
 * as part of a package for sale is not agreed to.  Storing this
 * software in a nonvolatile storage device characterized  as an 
 * integrated circuit providing read only memory (ROM), either as
 * source code or machine executeable instructions is similarly not
 * agreed to.  THIS SOFTWARE IS PROVIDED AS IS'' AND WITHOUT ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE
 * IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE
 */
#ifndef lint
char Copyright[]=
    "@(#) Copyright (c) 1990 David G. Koontz\n All rights reserved.\n";
#endif
/*
 *	fdes.c - faster implementation of DES algorithm.
 */

#include "DES.h"

void
loadkey(key,shift)
    char *key;
    int shift;
{
    register int i,Key,sbox,bit, C_carry,D_carry;
    register unsigned long C = 0;
    register unsigned long D = 0;

    if (shift)			     /* ascii keys -  save bit 0 */
	for (i=0;i < KEY_BYTES;i++)
	    key[i] = key[i] << 1;

    for (i=0;i< CD_BITS;i++) {		/* load C and D registers  */
	if( key[PC1_C[i] >> 4] & BIT((PC1_C[i] & 0xf)))
	    C |= BIT(i);
	if( key[PC1_D[i] >> 4] & BIT((PC1_D[i] & 0xf)))
	    D |= BIT(i);
    }

	for(i=0;i< KEY_BYTES;i++)	  /* erase key source */
	    key[i] = 0;

    for ( Key = 0; Key < KEY_SCHEDULE; Key++) {
	for ( i = 0; i < leftshift[Key];i++) {
	    C_carry = C & 1;
	    D_carry = D & 1;
	    C = C >> 1;		/* rotate C and D */
	    D = D >> 1;
	    if (C_carry)
		C |= BIT(27);
	    if (D_carry)
		D |= BIT(27);
	}
	for (sbox = 0; sbox < 8; sbox++){  /* load Key S Box Key_Scheds */
	    K_S[sbox][Key]=0;

	    for (bit = 0; bit < 6; bit++){

		if ( sbox < 4) {
		    if ( C & BIT(PC2_S18[sbox][bit]) )
			K_S[sbox][Key] |= BIT(bit);
		}
		else {
		    if ( D & BIT(PC2_S18[sbox][bit]) )
			K_S[sbox][Key] |= BIT(bit);
		}
	    }
	}
    }
}

void
no_ip_des(block,desmode)
    union LR_block *block;
    int desmode;
{
    register int round,pre_S,sbox;
    register unsigned long E_S,temp_f;
    register unsigned long Bit31,Bit0;

    for (round = 0; round < KEY_SCHEDULE; round++) {	/* f(R,K) */

	temp_f = 0;
	Bit31 = (block->LR[RR] & BIT(31))?(BIT(4)):0;
	E_S   = block->LR[RR] << 1;
	E_S  |= (Bit31)?1:0;	 /* S1 - R31,R0,R1,R2,R3,R4,...R30 */
	Bit0  = (block->LR[RR] & 1)?(BIT(5)):0;	    /* save for S8 */

	for (sbox = 0; sbox < 8; sbox++) {
	    pre_S = E_S & 0x3F;			/* lower 6 bits  */

	    if (sbox == 7) {	/* S8, pre_S bits 4,5 expected to be 0 */
		pre_S |= Bit0;			
		pre_S |= Bit31;	/* S8 - R27,R28,R29,R30,R31,R0,0,...0 */
	    }
	    else
		E_S >>= 4;	/* S2 - R3,R4,R5,R6,R7,R8,...R30,0,...0 */

	    if ( !desmode )			/* encrypt mode */
		pre_S ^= K_S[sbox][round];
	    else				/* decrypt mode */
		pre_S ^= K_S[sbox][(KEY_SCHEDULE-1)-round];

	    temp_f |= sbox_P[sbox][pre_S];    /* sbox in P output   */
	}				
	temp_f ^= block->LR[LL];	/* f(R,K) EXOR L  */
	block->LR[LL] = block->LR[RR];	/* move R to L */
	block->LR[RR] = temp_f;		/* new R       */
    }

    block->LR[RR] = block->LR[LL];    /* output block is R16L16 */
    block->LR[LL] = temp_f;
}

void
des(block,desmode)
    union LR_block *block;
    int desmode;
{
    int byte,bit;
    static union LR_block ip_iip;

    ip_iip.LR[LL]=0; ip_iip.LR[RR]=0;

    for (byte = 0; byte < 8; byte++) {		    /* IP */
	for (bit = 0; bit < 8; bit++) {
	if ( block->string[byte] & BIT(bit)) 
	    if ( bit & 1)			    /* odd bits to R */
		ip_iip.LR[RR] |= IP[byte][bit];
	    else
		ip_iip.LR[LL] |= IP[byte][bit];	    /* even bits to L */
	}
    }
    no_ip_des(&ip_iip,desmode);

    block->LR[AA] = 0; block->LR[BB] = 0;

    for (byte = 0; byte < 8; byte++) {		    /* Inverse IP */
	for (bit = 0; bit < 8; bit++) {
	    if (ip_iip.string[byte] & BIT(bit))
		if (bit < 4)
		    block->LR[BB] |= IIP[byte][bit];
		else
		    block->LR[AA] |= IIP[byte][bit];
	}
    }
}
