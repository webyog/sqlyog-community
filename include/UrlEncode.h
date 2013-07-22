/* Copyright (C) 2013 Webyog Inc.

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

#include <stdio.h>

#ifndef __CURLENCODE_H_
#define __CURLENCODE_H_

class CURLEncode
{

private:

	bool	convert	 (	char val, char * output );

	int		m_strlen;

public:
	
	CURLEncode();
	virtual ~CURLEncode() { };
	
	bool	URLEncode (const char * val, char * output, unsigned long * outlen = NULL );
	bool	decToHex (	char num, char * hex, int radix );
	bool	isUnsafe (	char compareChar );
};

#endif