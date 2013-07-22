/**********************************************************************
 *
 * Filename:    ycrc.cpp
 * 
 * Description: fast implementation of the CRC standard.
 *
 * Notes:       The parameters for each supported CRC standard are
 *				defined in the header file crc.h.  The implementations
 *				here should stand up to further additions to that list.
 *
 * 
 * Copyright (c) 2000 by Michael Barr.  This software is placed into
 * the public domain and may be used for any purpose.  However, this
 * notice must not be changed or removed and no warranty is either
 * expressed or implied by its publication or distribution.
 **********************************************************************/
 
#include "WyCrc.h"

wyCrc::wyCrc()
{
    CrcInit();
}

wyCrc::~wyCrc()
{
}

/*********************************************************************
 *
 * Function:    reflect()
 * 
 * Description: Reorder the bits of a binary sequence, by reflecting
 *				them about the middle position.
 *
 * Notes:		No checking is done that nBits <= 32.
 *
 * Returns:		The reflection of the original data.
 *
 *********************************************************************/
wyUInt32 
wyCrc::Reflect(wyUInt32 pData, unsigned char pBits)
{
	wyUInt32  reflection = 0x00000000;
	wyUChar   bit;

	/*
	 * Reflect the data about the center bit.
	 */
	
	for (bit = 0; bit < pBits; ++bit)
	{
		/*
		 * If the LSB bit is set, set the reflection of it.
		 */
		
		if (pData & 0x01)
		{
			reflection |= (1 << ((pBits - 1) - bit));
		}

		pData = (pData >> 1);
	}

	return (reflection);

}	/* reflect() */

/*********************************************************************
 *
 * Function:    crcInit()
 * 
 * Description: Populate the partial CRC lookup table.
 *
 * Notes:		This function must be rerun any time the CRC standard
 *				is changed.  If desired, it can be run "offline" and
 *				the table results stored in an embedded system's ROM.
 *
 * Returns:		None defined.
 *
 *********************************************************************/

void wyCrc::CrcInit()
{
    wyUInt32	remainder;
	wyInt32		dividend;
	wyUChar		bit;

    /*
     * Compute the remainder of each possible dividend.
     */
    for (dividend = 0; dividend < 256; ++dividend)
    {
        /*
         * Start with the dividend followed by zeros.
         */
        remainder = dividend << (WIDTH_CRC - 8);

        /*
         * Perform modulo-2 division, a bit at a time.
         */
        for (bit = 8; bit > 0; --bit)
        {
            /*
             * Try to divide the current data bit.
             */			
            if (remainder & TOPBIT)
            {
                remainder = (remainder << 1) ^ POLYNOMIAL;
            }
            else
            {
                remainder = (remainder << 1);
            }
        }

        /*
         * Store the result into the table.
         */
        vCrcTable[dividend] = remainder;
    }

}   /* crcInit() */


/*********************************************************************
 *
 * Function:    crcFast()
 * 
 * Description: Compute the CRC of a given message.
 *
 * Notes:		crcInit() must be called first.
 *
 * Returns:		The CRC of the message.
 *
 *********************************************************************/
wyUInt32
wyCrc::CrcFast(unsigned char const pMessage[], wyInt32 pBytes)
{
    wyUInt32	       remainder = INITIAL_REMAINDER;
    wyUChar          data;
	wyInt32         byte;

    /*
     * Divide the message by the polynomial, a byte at a time.
     */
    for (byte = 0; byte < pBytes; ++byte)
    {
        data =  REFLECT_DATA(pMessage[byte]) ^ (remainder >> (WIDTH_CRC - 8));
  		remainder = vCrcTable[data] ^ (remainder << 8);
    }

    /*
     * The final remainder is the CRC.
     */
    return (REFLECT_REMAINDER(remainder) ^ FINAL_XOR_VALUE);
}   /* crcFast() */

