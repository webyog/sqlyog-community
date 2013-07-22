/* Copyright (C) 2013 Webyog Inc

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA

*/

#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "URLEncode.h"

#define MAX_BUFFER_SIZE 4096

// HEX Values array
static char hexVals[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

// UNSAFE String
static char UnsafeString[] = "\"<>%\\^[]`+$,@:;/!#?=&'";

// PURPOSE OF THIS FUNCTION IS TO CONVERT A GIVEN CHAR TO URL HEX FORM

CURLEncode::CURLEncode()
{
	m_strlen = (int)strlen(UnsafeString);
}

bool
CURLEncode::convert (char val, char * output ) 
{
	output[0] = '%';
	
	decToHex (val, output+1, 16 );	

	return  true;
}

// THIS IS A HELPER FUNCTION.
// PURPOSE OF THIS FUNCTION IS TO GENERATE A HEX REPRESENTATION OF GIVEN CHARACTER
bool
CURLEncode::decToHex(char num, char * output, int radix)
{	
	int		temp=0, i = 0;	
	char	csTmp[10] = {0};
	int		num_char;
	bool	rev=false;

	num_char = (int)num;
	
	// ISO-8859-1 
	// IF THE IF LOOP IS COMMENTED, THE CODE WILL FAIL TO GENERATE A 
	// PROPER URL ENCODE FOR THE CHARACTERS WHOSE RANGE IN 127-255(DECIMAL)
	if (num_char < 0)
		num_char = 256 + num_char;

	while (num_char >= radix )
    {
		temp = num_char % radix;
		num_char = (int)floor((double)(num_char / radix));;
		csTmp[i] = hexVals[temp];
		i++;
    }
	
	/* url require %d to be %0d */
	if (!i) 
	{
		csTmp[i++] = '0';
		rev = true;
	}

	csTmp[i] = hexVals[num_char];

	if (!rev)
		_strrev (csTmp);
	
	strcpy (output, csTmp);
	
	return true;
}

// PURPOSE OF THIS FUNCTION IS TO CHECK TO SEE IF A CHAR IS URL UNSAFE.
// TRUE = UNSAFE, FALSE = SAFE

bool 
CURLEncode::isUnsafe(char compareChar)
{
	bool	bcharfound = false;
	char	tmpsafeChar;

	for(int ichar_pos = 0; ichar_pos < m_strlen ;ichar_pos++)
	{
		tmpsafeChar = UnsafeString[ichar_pos]; 

		if(tmpsafeChar == compareChar)
		{ 
			bcharfound = true;
			break;
		} 
	}

	int char_ascii_value = 0;
	char_ascii_value = (int) compareChar;

	if(bcharfound == false &&  char_ascii_value > 32 && char_ascii_value < 123)
	{
		return false;
	}
	// found no unsafe chars, return false		
	else
	{
		return true;
	}
	
}
// PURPOSE OF THIS FUNCTION IS TO CONVERT A STRING 
// TO URL ENCODE FORM.

bool
CURLEncode::URLEncode(const char * pcsEncode, char * output, unsigned long * outlen)
{	
	int				ichar_pos = 0;
	unsigned long	curpos=0;
	char			ch;
	char			hex[10];
	output[0] = 0;
	
	for(; (ch = pcsEncode[ichar_pos]) != 0 ; ichar_pos++)
	{

		if(!isUnsafe(ch) )
		{
			// Safe Character		
			curpos		+=	sprintf (output+curpos, "%c", ch );
		}
		else
		{
			// get Hex Value of the Character
			convert(ch, hex );
			curpos		+= sprintf(output+curpos, "%s", hex);
		}
	}

	if(outlen)
		*outlen = curpos;
	
	return true;
}