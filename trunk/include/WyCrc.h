/**********************************************************************
 *
 * Filename:    ycrc.h
 * 
 * Description: A header file describing the various CRC standards.
 *
 * Notes:       
 *
 * 
 * Copyright (c) 2000 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/

#ifndef _CRC_H
#define _CRC_H

#include "Datatype.h"
/*
 * Derive parameters from the standard-specific parameters in ycrc.h.
 */
#define WIDTH_CRC    (8 * sizeof(wyUInt32))
#define TOPBIT       (1 << (WIDTH_CRC - 1))

#define REFLECT_DATA(X)			((unsigned char) Reflect((X), 8))
#define REFLECT_REMAINDER(X)	((wyUInt32) Reflect((X), WIDTH_CRC))


/*
 * Select the CRC standard from the list that follows.
*/

#define POLYNOMIAL			0x04C11DB7
#define INITIAL_REMAINDER	0xFFFFFFFF
#define FINAL_XOR_VALUE		0xFFFFFFFF
#define CHECK_VALUE			0xCBF43926

class wyCrc
{
public:
    wyCrc();
    ~wyCrc();

    void     CrcInit();
	
	wyUInt32   CrcFast(unsigned char const pMessage[], int pBytes);
    wyUInt32   Reflect(wyUInt32 data, unsigned char pBits);
private:
   wyUInt32  vCrcTable[256];
};


#endif /* _crc_h */

