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

#include <malloc.h>
#include <stdio.h>
#include <assert.h>
#include "wyString.h"
#include "CommonHelper.h" 

#ifndef _WIN32
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdarg.h>
#include <wchar.h>
#include <ctype.h>
#endif

#define NOT_ENOUGH_MEMORY _("Not enough memory, application terminated! ")

wyString::wyString()
{	
	Init();
	InitBuf(BUF_CHUNK_ALLOCATION);
}

wyString::wyString(wyUInt32 initsize)
{
	Init();
	InitBuf(initsize);
}

wyString::wyString(const wyChar* initbuf, wyBool ismysql41)
{	
    Init();
	SetAs(initbuf, ismysql41);
}

wyString::wyString(const wyWChar* initbuf)
{
	Init();

#ifdef _WIN32
	wyInt32 length = WideCharToMultiByte(CP_UTF8, 0, initbuf, -1, NULL, 0, NULL, NULL);
#else
    //set codepage to UTF-8
    setlocale(LC_ALL, "en_US.utf8");
    wyInt32 length = wcstombs(NULL, initbuf, 0);
#endif

	Allocate(length + 1);

#ifdef _WIN32
	m_strsize = WideCharToMultiByte(CP_UTF8, 0, initbuf, -1, m_strdata, length, NULL, NULL);
#else
    m_strsize = wcstombs(m_strdata, initbuf, length);
#endif

}

wyString::wyString(const wyChar* initbuf, wyUInt32 initsize)
{
    Init();
	InitBuf(initsize);
	SetAs(initbuf, initsize);
}

wyString::wyString(const wyString& str)
{
    Init();
    SetAs(((wyString&)str).GetString(), ((wyString&)str).GetLength());
}

wyString::~wyString()
{
    if(m_substr)
        free(m_substr);

	if (m_strdata)
 		free(m_strdata);
	
	if(m_widecharbuffer)
		free(m_widecharbuffer);

	if(m_ansibuffer)
		free(m_ansibuffer);
}

wyString& 
wyString::operator=(const wyString& rhstr)
{
    this->SetAs((wyString&)rhstr);
    return *this;
}

wyUInt32 
wyString::Add(const wyChar *buffer)
{	    
	return Add(buffer, strlen(buffer));
}

wyUInt32    
wyString::Add(const wyChar *buffer, wyInt32 length)
{
	wyUInt32		len;
	wyUInt32		required;

	len = (length == -1) ? strlen(buffer) : length;

	required = (m_strsize + len) * sizeof(wyChar) + sizeof(wyChar);

	if(required > m_allocated)
	{
		m_allocated = required;

		m_strdata = (wyChar*)realloc(m_strdata, m_allocated);
		
		if(m_strdata == NULL)
		{
			WriteToLogFile(NOT_ENOUGH_MEMORY);
			exit(0);
		}
	}
	
	// copy new stuff
	memcpy(m_strdata + m_strsize, buffer, len * sizeof(wyChar));
	
	// set the correct len
	m_strsize += len;

	m_strdata[m_strsize] = 0;

    return m_strsize;
}

wyUInt32    
wyString::AddSprintf(const wyChar* formatstring, ...)
{
    va_list			args;
    wyUInt32        len;

    va_start(args, formatstring); /* Initialize variable arguments. */

    len = SNprintf(formatstring, args, m_strsize);

    va_end(args); /* Reset variable arguments.      */

    return len;
}

wyUInt32   
wyString::SetAs(wyString& str)
{
	return SetAs(str.GetString(), str.GetLength());
}

wyUInt32 
wyString::SetAs(const wyWChar* toset)
{	
	if(wcslen(toset) == 0)
	{
		SetAs("");
		return 0;
	}
#ifdef _WIN32
	wyInt32 length = WideCharToMultiByte(CP_UTF8, 0, toset, -1, NULL, 0, NULL, NULL);
#else
    //set codepage to UTF-8
    setlocale(LC_ALL, "en_US.utf8");
    wyInt32 length = wcstombs(NULL, toset, 0);
#endif
    
	return SetAs(toset, length);
}

wyUInt32 
wyString::SetAs(const wyWChar* toset, wyInt32 length)
{	
	wyChar		*utf8 = NULL;
	wyUInt32	len = 0;
	wyInt32		lengthofutf8 = 0;		

	if(length == 0)
	{
		SetAs("");
		return 0;
	}
    
	utf8 = (wyChar *)calloc(sizeof(wyChar), length + 1);
	
	if(utf8 == NULL)
	{
		WriteToLogFile(NOT_ENOUGH_MEMORY);
		exit(0);
	}

#ifdef _WIN32
	lengthofutf8 = WideCharToMultiByte(CP_UTF8, 0, toset, -1, utf8, length, NULL, NULL);
#else
	setlocale(LC_ALL, "en_US.utf8");
    lengthofutf8 = wcstombs(utf8, toset, length);
#endif
	
	len = SetAs(utf8, (wyUInt32)lengthofutf8);

	free(utf8);
	utf8 = NULL;
	return len;	
}


wyUInt32
wyString::SetAs(const wyChar* toset, wyUInt32 len)
{
	Allocate(len + 1);
	strcpy(m_strdata, toset);
	m_strsize = strlen(m_strdata);

	return m_strsize;
}

VOID
wyString::SetAsDirect(const wyChar* toset, const wyUInt32 len)
{
	Clear();
	Add(toset, len);
}


wyUInt32
wyString::SetAs(const wyChar* toset, wyBool ismysql41)
{
	wyInt32 lengthofstring = 0;
	wyInt32 lengthwidechar = 0;
	wyInt32 lengthtoallocate = 0;
	wyInt32 length = 0;
	wyWChar *widecharbuff = 0;
	
	//crash fix http://forums.webyog.com/index.php?showtopic=7438
	if(!toset)
		return 0;

	lengthofstring = strlen(toset);
	length = strlen(toset);

	if(ismysql41 == wyFalse)
	{	
#ifdef _WIN32
		
		// Get the Length for allocation of Widecharacter
		lengthtoallocate = MultiByteToWideChar(CP_ACP, 0, toset, lengthofstring, NULL,NULL);
        		
#else
        
        //set default codepage to ANSI
        setlocale(LC_ALL, "en_US.iso88591");
        lengthtoallocate = mbstowcs(NULL, toset, lengthofstring);
#endif

		widecharbuff = (wyWChar *)calloc(sizeof(wyWChar), (lengthtoallocate + 1) * (sizeof(wyWChar)));//AllocForWideChar(lengthtoallocate + 1);   
		
		if(widecharbuff == NULL)
		{	
			WriteToLogFile(NOT_ENOUGH_MEMORY);
			exit(0);
		}
		
#ifdef _WIN32
		//Conversion to WideCharacter
		lengthwidechar = MultiByteToWideChar(CP_ACP, 0, toset, lengthofstring, widecharbuff, lengthtoallocate);
		
		// Get the Length for Allocation of multibyte
        lengthtoallocate = WideCharToMultiByte(CP_UTF8, 0, widecharbuff, lengthwidechar, NULL, 0, NULL,NULL);
#else
        //Conversion to WideCharacter
        lengthwidechar = mbstowcs(widecharbuff, toset, lengthtoallocate);
        setlocale(LC_ALL, "en_US.utf8");
        lengthtoallocate = wcstombs(NULL, widecharbuff, lengthwidechar);
#endif
		
		Allocate(lengthtoallocate + 1);
		
#ifdef _WIN32
		//Conversion to Multibyte
		m_strsize = WideCharToMultiByte(CP_UTF8, 0, widecharbuff, lengthwidechar, m_strdata, lengthtoallocate, NULL,NULL);
#else
        m_strsize = wcstombs(m_strdata, widecharbuff, lengthtoallocate);
#endif
		
		m_strdata[m_strsize] = '\0';

		free(widecharbuff);	
	}
	else
		SetAs(toset, length);
	
	return m_strsize;
}

wyInt32
wyString::CompareI(const wyChar* tocompare)
{
    return stricmp(m_strdata, tocompare);
}

wyInt32
wyString::CompareNI(const wyChar* tocompare, wyInt32 nochars)
{
    return strnicmp(m_strdata, tocompare, nochars);
}

wyInt32
wyString::CompareI(wyString& tocompare)
{
	return stricmp(m_strdata, tocompare.GetString());	
}

/*Usual stricmp will consider the lowercase value of char during comparison
This is a custom stricmp function but it does compre Uppercase value of chars
*/
wyInt32
wyString::CompareIUppercaseMode(const wyChar *tocompare)
{
	wyInt32 ret = 0;

	if(!m_strdata || !tocompare)
		return 0;

	const char * src = m_strdata;
	const char * dst = tocompare;
	
	while(*src && *dst && !(ret = ::toupper(*src++) - ::toupper(*dst++)));
	
	if(ret < 0)
	{
		ret = -1;
	}
    else if(ret > 0)
	{
        ret = 1;
	}

	return ret;	
}

wyInt32
wyString::Compare(const wyChar* tocompare)
{
    return strcmp(m_strdata, tocompare);
}

wyInt32
wyString::Compare(wyString& tocompare)
{
    return strcmp(m_strdata, tocompare.GetString());
}

wyUInt32    
wyString::Strip(wyUInt32 size)
{
    if(m_allocated == 0)
        return 0;

    if(size > m_strsize) 
	{
        m_strdata[0] = 0;
        SetValues(m_allocated,0);
        return m_strsize;
    }

    m_strdata[m_strsize - size] = 0;

    SetValues(m_allocated, m_strsize - size);

    return m_strsize;
}

const wyChar* 
wyString::GetString()
{
	return m_strdata;
}

wyUInt32  
wyString::GetLength()
{
    return m_strsize;
}

wyInt32
wyString::FindChar(wyChar searchchar, wyUInt32 pos)
{
	wyChar *searchres = NULL;

   if(pos < m_strsize)
	   searchres = strchr(m_strdata + pos, searchchar);

	return (!searchres ?-1 : (int)(searchres - m_strdata));
}

wyChar
wyString::GetCharAt(wyUInt32 pos)
{
    if(pos > m_allocated)
        return 0;

    return m_strdata[pos];
}

wyInt32
wyString::SetCharAt(wyUInt32 pos, wyChar text)
{
    if(pos > m_allocated)
        return -1;

	m_strdata[pos] = text;

    return 0;
}

wyUInt32    
wyString::Sprintf(const wyChar* formatstring, ...)
{
	va_list			args;
    wyUInt32        len;

    m_strsize = 0;

    va_start(args, formatstring);  /* Initialize variable arguments. */

    len = SNprintf(formatstring, args, m_strsize);

    va_end(args); /* Reset variable arguments.      */

    return len;
}

#ifdef _WIN32
wyUInt32    
wyString::SprintfP(const wyChar* formatstring, ...)
{
    va_list			args;
    wyUInt32        len;

    m_strsize = 0;

    va_start(args, formatstring);  /* Initialize variable arguments. */

    len = SNprintfP(formatstring, args, m_strsize);

    va_end(args); /* Reset variable arguments.      */

    return len;
}
#endif

wyWChar*
wyString::GetAsWideChar(wyUInt32 *len, wyBool isreadonly)
{	
	wyInt32 widelen;

	if(m_widecharbuffer)            // if somethin is there already? then free memory
    {
        if(isreadonly == wyTrue)
        {
            return m_widecharbuffer;
        }

		free(m_widecharbuffer); 
    }
	
	m_widecharbuffer = NULL;

    // allocate
	m_widecharbuffer = (wyWChar *)calloc(sizeof(wyWChar), (m_strsize + 1) * (sizeof(wyWChar)));
	
	if(m_widecharbuffer == NULL)
	{	
		WriteToLogFile(NOT_ENOUGH_MEMORY);
		exit(0);
	}
		
#ifdef _WIN32
	
	/*Function to get Wide character.
	* Fills the Wide character buffer with converted 
	* wide character data.*/

	widelen = MultiByteToWideChar(CP_UTF8, 0, m_strdata, m_strsize, m_widecharbuffer, 
						(m_strsize * sizeof(wyWChar)));
	

	if(len != NULL)
		*len = widelen;
	
	return m_widecharbuffer;
#else
    setlocale(LC_ALL,"en_US.utf8");

    widelen = mbstowcs(m_widecharbuffer, m_strdata, (m_strsize * sizeof(wyWChar)));
    
    if(len != NULL)
        *len  = widelen;

    return m_widecharbuffer;
#endif
}

wyWChar*
wyString::GetAsWideCharEnc(wyInt32 encoding, wyUInt32 *len)
{	
	wyInt32 widelen;

	if(m_widecharbuffer)            // if somethin is there already? then free memory
    {

		free(m_widecharbuffer); 
    }
	
	m_widecharbuffer = NULL;

    // allocate
	m_widecharbuffer = (wyWChar *)calloc(sizeof(wyWChar), (m_strsize + 1) * (sizeof(wyWChar)));
	
	if(m_widecharbuffer == NULL)
	{	
		WriteToLogFile(NOT_ENOUGH_MEMORY);
		exit(0);
	}
		
#ifdef _WIN32
	
	/*Function to get Wide character.
	* Fills the Wide character buffer with converted 
	* wide character data.*/

	widelen = MultiByteToWideChar(encoding, 0, m_strdata, m_strsize, m_widecharbuffer, 
						(m_strsize * sizeof(wyWChar)));
	

	if(len != NULL)
		*len = widelen;
	
	return m_widecharbuffer;
#endif
}

void 
wyString::Init()
{
    m_strdata = NULL;
    m_allocated = 0;
    m_strsize = 0;
    m_substr = NULL;
	m_widecharbuffer = NULL;
	m_ansibuffer = NULL;
	m_utf8str = NULL;
}

void 
wyString::InitBuf(wyUInt32 initsize)
{
    if(m_strdata)
        free(m_strdata);

    m_strdata = (wyChar *)calloc(sizeof(wyChar), initsize);
	
	if(m_strdata == NULL)
	{	
		WriteToLogFile(NOT_ENOUGH_MEMORY);
		exit(0);
	}

	m_widecharbuffer = NULL;
	m_ansibuffer = NULL;
	SetValues(initsize,0);
}

void 
wyString::Allocate(wyUInt32 size)
{
	// Allocate buffer for the internal data stream. While allocating, 3 cases arise:
    // 1. No buffer is allocated initially so we just need to allocate buffer of size 'size'
    // 2. If the buffer already allocated is less then the size requested then we realloc
    // 3. Buffer allocated is more then the size required then we dont do anything
	if(size <= 0)
		return;

	if(m_strdata == NULL) 
	{
		m_strdata		= (wyChar *)calloc(sizeof(wyChar), size + 1);//AllocForwyChar(size+1);
		
		if(m_strdata == NULL)
		{	
			WriteToLogFile(NOT_ENOUGH_MEMORY);
			exit(0);
		}

		SetValues(size, 0);
	} 
	else if(m_allocated < size)
	{
		m_strdata		= (wyChar*)realloc(m_strdata, sizeof(wyChar)*size);
		
		if(m_strdata == NULL)
		{
			WriteToLogFile(NOT_ENOUGH_MEMORY);
			exit(0);
		}
		
		SetValues(size, 0);
	}
	else
	{
		SetValues(m_allocated, 0);
	}

    return;
}

void
wyString::SetValues(wyUInt32 allocated, wyUInt32 strsize)
{
	m_allocated = allocated;
	m_strsize	= strsize;
    if(m_substr)
	{
		free(m_substr);
	}
	m_substr  = NULL;
	if(m_utf8str)
	{
		free(m_utf8str);
	}
	m_utf8str   = NULL;
	
}

wyUInt32 
wyString::SNprintf(const wyChar* formatstring, va_list args, wyUInt32 length)
{
    wyInt32		    size = -1;
#ifndef _WIN32
	va_list         argtemp;
#endif

    // If buffer is not allocated
    if(m_allocated == 0) // 0 or NULL
	{
		Allocate(BUF_CHUNK_ALLOCATION);
	}

#ifdef _WIN32
    // We are deducting -1 from the passed length because according to MSDN docs:
    // If the string written to the buffer fits without the null terminator, the buffer is not null-terminated and the function returns success
    // (m_allocated - length - 1) is the buffer that is left in the string
	size = _vsnprintf(m_strdata + length, m_allocated - length - 1, formatstring, args);
#else
    va_copy(argtemp, args);
    size = vsnprintf(m_strdata + length, m_allocated - length - 1, formatstring, argtemp);
    va_end(argtemp);
#endif

	while(size == -1 || size >= m_allocated - length - 1)
	{
		Allocate(m_allocated * 2);
#ifdef _WIN32
        // See comment above
		size = _vsnprintf(m_strdata + length, m_allocated - length - 1, formatstring, args);
#else
        va_copy(argtemp, args);
        size = vsnprintf(m_strdata + length, m_allocated - length - 1, formatstring, argtemp);
        va_end(argtemp);
#endif
	}

	// Final total length
    m_strsize = size + length;

    m_strdata[m_strsize] = '\0';
   
	return m_strsize;
}

#ifdef _WIN32
wyUInt32    
wyString::SNprintfP(const wyChar* formatstring, va_list args, wyUInt32 length )
{
    wyInt32		    size = -1;

    // If buffer is not allocated
    if(m_allocated == 0) // 0 or NULL
	{
		Allocate(BUF_CHUNK_ALLOCATION);
	}

    // We are deducting -1 from the passed length because according to MSDN docs:
    // If the string written to the buffer fits without the null terminator, the buffer is not null-terminated and the function returns success
    // (m_allocated - length - 1) is the buffer that is left in the string
	size = _vsprintf_p(m_strdata + length, m_allocated - length - 1, formatstring, args);

	while(size == -1 || size >= m_allocated - length - 1)
	{
		Allocate(m_allocated * 2);
		size = _vsprintf_p(m_strdata + length, m_allocated - length - 1, formatstring, args);
	}

	// Final total length
    m_strsize = size + length;

    m_strdata[m_strsize] = '\0';
   
	return m_strsize;
}
#endif

wyInt32 
wyString::Find(const wyChar *searchstr, wyUInt32 pos)
{
    wyChar *find = NULL;
    wyInt32 result = -1;

    if(pos < m_strsize)
		find = strstr(m_strdata + pos, searchstr);

    if(find != NULL)
		result = (int)(find - m_strdata);// + 1);

    return result;
}

//Search the string in case in-sensitve mode; only For WINDOWS
wyInt32
wyString::FindI(wyChar *searchstr, wyUInt32 pos)
{
	wyChar *find = NULL;
    			
    if(pos < m_strsize)
	{
		find = wyString::StrIStr(m_strdata + pos, searchstr);
	}
	
	//if string is found
    if(find)
	{
		return (find - m_strdata);
	}
	
    return -1;
}

//wyInt32 
//wyString::Replace(wyInt32 startpos, wyInt32 replacestrlen, const wyChar *replacestr)
//{
//    wyChar *startdump = NULL, *enddump = NULL;
//    wyInt32 startlen, endlen;
//
//    if(replacestrlen == 0)
//        return m_strsize;
//
//    startlen = startpos;
//    endlen = m_strsize - startlen - replacestrlen;
//
//    if(startlen > 0)
//    {
//        startdump = (wyChar *)calloc(sizeof(wyChar), startlen + 1);
//		
//		if(startdump == NULL)
//		{
//			WriteToLogFile(NOT_ENOUGH_MEMORY);
//			exit(0);
//		}
//
//		strncpy(startdump, m_strdata, startlen);
//    }
//
//    if(endlen > 0)
//    {
//        enddump = (wyChar *)calloc(sizeof(wyChar), endlen + 1);
//		
//		if(enddump == NULL)
//		{
//			WriteToLogFile(NOT_ENOUGH_MEMORY);
//			exit(0);
//		}
//
//		strncpy(enddump, m_strdata + startpos + 
//		replacestrlen, endlen);
//    }
//	
//    Sprintf("%s%s%s", (startlen > 0)? startdump:"", replacestr, (endlen > 0)?enddump:"");
//
//	if(startdump)
//		free(startdump);
//
//	if(enddump)
//		free(enddump);
//	
//    return m_strsize;
//}


wyChar *  
wyString::Substr(wyUInt32 startpos, wyUInt32 nochars)
{
    if(startpos >= m_strsize)
        return NULL;

    if(m_substr)
        m_substr = (wyChar*)realloc(m_substr, sizeof(wyChar) * nochars + 1);
    else
        m_substr = (wyChar *)calloc(sizeof(wyChar), nochars + 1);

	if(m_substr == NULL)
	{
		WriteToLogFile(NOT_ENOUGH_MEMORY);
		exit(0);
	}

    strncpy(m_substr, m_strdata + startpos, nochars);
    
    m_substr[nochars] = '\0';
    return m_substr;
}

//inserting string 
wyInt32   
wyString::Insert(wyUInt32 insertpos, wyChar* insstr)
{
	wyString  str;	
	wyChar    *reststr;

	if(insertpos < 0 || insertpos  > m_strsize + 1)
		return -1;	

	reststr = strdup(m_strdata + insertpos);	
	str.SetAs(Substr(0, insertpos));    
	str.Add(insstr);
	str.Add(reststr);		
	SetAs(str.GetString());

	if(reststr)
		free(reststr);

	return m_strsize;     
}

wyBool   
wyString::Erase(wyUInt32 startpos, wyUInt32 nochars)
{
  wyUInt32 pos;

  if(startpos >= m_strsize)
    return wyFalse;

  if(m_strdata)
  {
	  while(m_strsize-startpos && nochars)
	  {
		for(pos = startpos; pos < m_strsize && m_strdata[pos] != '\0'; pos++)
			m_strdata[pos] = m_strdata[pos+1];

		m_strsize--;
		nochars--;
	  }
  }

  return wyTrue;
}

wyInt32    
wyString::LTrim(wyInt32 *ltrimcount)
{
	wyInt32 beforetrim;

	beforetrim = m_strsize;

	while(m_strsize)
	{
		if(isspace(m_strdata[0]))
			Erase(0, 1);
		else
			break;
	}

	//trim spaces count
	if(ltrimcount)
		*ltrimcount = beforetrim - m_strsize;

	return m_strsize;
}

wyInt32    
wyString::RTrim(wyInt32 *rtrimcount)
{
	wyInt32 beforetrim;

	beforetrim = m_strsize;

	while(m_strsize)
    {
		if(isspace(m_strdata[m_strsize-1]))
			Erase(m_strsize - 1, 1);
		else
			break;
	}

	//trim spaces count
	if(rtrimcount)
		*rtrimcount = beforetrim - m_strsize;

	return m_strsize;
}

void    
wyString::Clear()
{
    if(m_strdata)
        m_strdata[0] = 0;

   m_strsize= 0;
}

wyChar*
wyString::GetAsAnsi()
{
    wyInt32 length;
	wyWChar *lpszw = NULL;

#ifdef _WIN32
	length = MultiByteToWideChar(CP_UTF8, 0, m_strdata, m_strsize, NULL, NULL );
	
    if(m_ansibuffer)
        free(m_ansibuffer);
	
	m_ansibuffer = NULL;

	lpszw = new wyWChar[length + 1];
	m_ansibuffer = (wyChar *)calloc(sizeof(wyChar), length + 1);
	
	if(m_ansibuffer == NULL)
	{
		WriteToLogFile(NOT_ENOUGH_MEMORY);
		exit(0);
	}

	//this step intended only to use WideCharToMultiByte
	MultiByteToWideChar(CP_UTF8, 0, m_strdata, -1, lpszw, length);

	lpszw[length] = '\0';

	//Conversion to ANSI (CP_ACP)
	WideCharToMultiByte(CP_ACP, 0, lpszw, -1, m_ansibuffer, length, NULL, NULL);

	m_ansibuffer[length] = 0;

	delete[] lpszw;
	return m_ansibuffer;
#else
    setlocale(LC_ALL,"en_US.utf8");
    length = mbstowcs(NULL, m_strdata, m_strsize);
    
    if(m_ansibuffer)
        free(m_ansibuffer);
    
    m_ansibuffer = NULL;

    lpszw = new wyWChar[length + 1];
	m_ansibuffer = (wyChar *)calloc(sizeof(wyChar), length + 1);
	
	if(m_ansibuffer == NULL)
	{
		WriteToLogFile(NOT_ENOUGH_MEMORY);
		exit(0);
	}

	//this step intended only to use WideCharToMultiByte
    mbstowcs(lpszw, m_strdata, length);

    lpszw[length] = '\0';
    
    //set default codepage to ANSI
    setlocale(LC_ALL, "en_US.iso88591");
    
    //Conversion to ANSI (CP_ACP)
    wcstombs(m_ansibuffer, lpszw, length);
    
    m_ansibuffer[length] = 0;
    
    delete[] lpszw;
    return m_ansibuffer;
#endif
}

wyChar*
wyString::GetAsEncoding(wyInt32 encoding, wyInt32 *len)
{
    wyInt32 length;
	wyWChar *lpszw = NULL;

#ifdef _WIN32
	length = MultiByteToWideChar(CP_UTF8, 0, m_strdata, m_strsize, NULL, NULL );
    if(m_ansibuffer)
        free(m_ansibuffer);
	
	m_ansibuffer = NULL;

	lpszw = new wyWChar[length + 1];

	//this step intended only to use WideCharToMultiByte
	MultiByteToWideChar(CP_UTF8, 0, m_strdata, -1, lpszw, length);
	//MultiByteToWideChar(encoding, 0, m_strdata, -1, lpszw, length);
	lpszw[length] = '\0';

	//Conversion to ANSI (CP_ACP)
	//Conversion to specific encoding
	length = WideCharToMultiByte(encoding, 0, lpszw, -1, m_ansibuffer, 0, NULL, NULL);
	m_ansibuffer = (wyChar *)calloc(length + 1, sizeof(wyChar));
	if(m_ansibuffer == NULL)
	{
		WriteToLogFile(NOT_ENOUGH_MEMORY);
		exit(0);
	}
	WideCharToMultiByte(encoding, 0, lpszw, -1, m_ansibuffer, length, NULL, NULL);
	//m_ansibuffer[length] = 0;
	*len = length;
	delete[] lpszw;
	return m_ansibuffer;

#endif
}

wyUInt32   
wyString::GetAsUInt32(void)
{	
	return(atoi(GetString()));	
}

wyInt32   
wyString::GetAsInt32(void)
{	
	return(atoi(GetString()));	
}

wyInt64   
wyString::GetAsInt64(void)
{	
	
	return(_atoi64(GetString()));	
}
//converting string to double precision values
double
wyString::GetAsDouble(wyChar **endptr)
{	
	return(strtod(m_strdata, endptr));
}

// strips up to a token(including token)
// Parameters
//      pToken: token to find and strip
//      pNewString: the string upto token[OUT]
// Returns
//      TRUE if not empty, FALSE otherwise
wyBool
wyString::StripToken(wyChar * token, wyString * newstring)
{
	wyChar *	found;
	size_t		tokenlen = strlen(token);
	size_t		firstpartlen;
	size_t		secondpartlen;
	wyChar		newstr[128] = {0};

	newstring->Clear();

	if(m_strsize == 0)
		return wyFalse;

	found = strstr(m_strdata, token);

	if(found)
	{
		// copy everything from the left of the token to pNewString
		firstpartlen = found - m_strdata;
		secondpartlen = m_strsize -(firstpartlen+tokenlen);

		//pNewString->StrNCpy(this, firstpartlen);
		strncpy(newstr, this->GetString(), firstpartlen);
		newstring->SetAs(newstr);


		// remove the extracted string and the token from the current string			
		memmove(m_strdata, m_strdata +(firstpartlen+tokenlen), secondpartlen*sizeof(wyChar)+ sizeof(wyChar));

		// set the new size
		m_strsize = secondpartlen;

		return wyTrue;

	}
	else
	{		
		newstring->SetAs(this->GetString());		
		// Make this empty
		this->Clear();
	}
	return wyTrue;		
}

// removes pChar(single/consecutive occurence) from the left of the buffer, 
// Parameters
//      pChar : Char to remove
// Returns
//      void
void 
wyString::StripLChar(wyChar pchar)
{
	if(m_strsize == 0)
		return;

	while(*m_strdata == pchar && m_strsize > 0)
	{
		m_strsize--;

		if(*(m_strdata+1) != 0)
		{
			memmove(m_strdata, m_strdata+1, m_strsize*sizeof(wyChar));
		}
		else
		{
			*m_strdata = 0;
			return;
		}
	}

	m_strdata[m_strsize] = 0;
}

wyInt32 
wyString::FindIAndReplace(wyChar* findstr, wyChar* replacestr)
{
  wyInt32 flen = strlen(findstr);
  wyInt32 rlen = strlen(replacestr);
  wyInt32 pos = FindI(findstr);

  while(pos != -1) {
    Replace(pos, flen, replacestr);
    pos = FindI(findstr, pos + rlen);
  }
  return GetLength();
}

wyInt32 
wyString::Replace(wyInt32 startpos, wyUInt32 substrlen, const wyChar *replacestr, wyUInt32 replacestrlen) 
{
    if(startpos == -1 || substrlen == -1 || replacestr == NULL) 
    {
        VERIFY(0);
    }

    if(replacestrlen == -1) 
    {
        replacestrlen = strlen(replacestr);
    }

    if (replacestrlen <= substrlen) 
    {
        // if old string length is less than or equal to new string just replace and use memmove
        // copy new string
        memmove(m_strdata + startpos, replacestr, replacestrlen * sizeof(wyChar));
       
        // move buffer
        memmove(m_strdata + startpos + replacestrlen, m_strdata + startpos + substrlen, 
            (m_strsize - (startpos + substrlen)) * sizeof(wyChar));

        m_strsize -= (substrlen - replacestrlen );
        m_strdata[m_strsize] = NULL;

        return m_strsize;
  }

  if(m_allocated <= m_strsize + (replacestrlen - substrlen)) 
  {
    // allocate enough first if not
    m_allocated = (m_strsize + (replacestrlen - substrlen) + 1) * 2;
	wyChar* prev = m_strdata;
	
    m_strdata =(wyChar*)calloc(m_allocated, 1);
	if(!m_strdata) 
    {
		WriteToLogFile(NOT_ENOUGH_MEMORY);
		exit(0);
	}

    // copy existing stuff
	memcpy(m_strdata, prev, m_strsize * sizeof(wyChar));
		// delete prev buf
    if(prev) 
    {
      free(prev);
    }
  }

 // first move the buffer
  memmove(m_strdata + startpos + replacestrlen, m_strdata + startpos + substrlen , 
           (m_strsize - (startpos + substrlen))*sizeof(wyChar));
  
  // copy new string
  memmove(m_strdata + startpos, replacestr, replacestrlen*sizeof(wyChar));
  m_strsize += (replacestrlen - substrlen);
  m_strdata[m_strsize] = NULL;

  return m_strsize;
}
// converts all buffer to lower case
VOID 
wyString::ToLower()
{
	wyChar *p = m_strdata;
    do	
    {	
        *p = tolower(*p);
    }
    while(*p++);
}

VOID 
wyString::ToUpper()
{
	wyChar *p = m_strdata;

    do	
    {	
        *p = toupper(*p);		
    }
    
	while(*p++);
}

// helps to Escape a url
// Parameters
//      pPreserveReserved: reserved characters can only be used in certain places
// Returns
//      void
VOID 
wyString::EscapeURL(wyBool preserve_reserved) 
{
  wyChar *ret = NULL, *to = NULL;
  size_t  buf_size = 0;

  if (!m_strsize) // nothing to escape
    return;
   
    /* our new string is at _most_ 3 times longer */
    ret = (wyChar*) calloc(1, m_strsize * 3);
    /*if(!ret) {
        LOG3(gLog, -1, NOT_ENOUGH_MEMORY);
        exit(0);
    }*/
    to = ret;
    UrlEncode(m_strdata, to, preserve_reserved);

  if(m_strdata) free(m_strdata);

    m_strsize = strlen(ret);
    buf_size = m_strsize + 1;
    ret = (wyChar*)realloc(ret, buf_size);
    m_strdata = ret;
    return;
}

void 
wyString::UrlEncode(const wyChar * from, wyChar * to, wyBool preserve_reserved) 
{
    while (*from) 
	{
        if (isalnum(*from) || IsUrlSafe(*from) ||
          (preserve_reserved == TRUE && IsUrlReservedSafe(*from))) {
            *(to++) = *from;
        } else {
            sprintf(to, "%%%02x", (unsigned char)*from);
            to += 3;
        }
        from++;
    }
}

// checks whether the char is included in the reserved list
// Parameters
//      pChar:  char to check
// Returns
//      1 if reserved, 0 otherwise
wyInt32 
wyString::IsUrlReservedSafe(wyChar character)
{
    //reserved characters can only be used in certain places:
 wyChar urlreserved [] = { ';', '/', '?', ':', '@', '&', '=', '+', '$', ',', '\0' };
  wyInt32 i;

    for (i = 0; urlreserved[i]; ++i) {
        if (character == urlreserved[i]) return 1;
    }

    return 0;
}

//checks char is always safe for putting in a URL:
// Parameters
// pChar: char to check
// Returns
// 1 if safe, 0 otherwise
wyInt32 
wyString::IsUrlSafe(wyChar character) 
{
	// see url(7) for details
	// XXX don't touch the 0 (zero) at the end of either of these arrays!
	// these are always safe for putting in a URL:
	wyChar urlsafe [] = { '-', '_', '.', '!', '~', '*', '\'', '(', ')', '\0' };
	wyInt32 i;

	for (i = 0; urlsafe[i]; ++i) 
	{
		if(character == urlsafe[i]) 
			return 1;
	}	

	return 0;
}

//Performs a case sensitive find and replaces the findstr with replace string
wyInt32 
wyString::FindAndReplace(const wyChar* findstr, const wyChar* replacestr)
{
  wyInt32 flen = strlen(findstr);
  wyInt32 rlen = strlen(replacestr);
  wyInt32 pos = Find(findstr, 0);

  while(pos != -1) {
    Replace(pos, flen, replacestr, rlen);
    pos = Find(findstr, pos + rlen);
  }
  return GetLength();
}

wyInt32
wyString::FindIWithReverse(wyChar *searchstr, wyUInt32 pos, wyBool isreverse)
{
	wyChar *find = NULL;
    wyInt32 result = -1;
	wyString strlwr;
	wyString strrv;
    wyChar* tempbuff;
	
	//string to search
	strlwr.SetAs(searchstr);
	strlwr.ToLower();

	//full string where search is done
	tempbuff = strdup(m_strdata);
	strrv.SetAs(tempbuff) ;
	free(tempbuff);
	strrv.ToLower();
		
    if(pos < m_strsize)
	{
		/*for searching substring from end. 
		Here 1st reverses both main string and search string. Then do search*/
		if(isreverse == wyTrue)
		{
#ifdef WIN32
			tempbuff = strdup(strrv.GetString());
			strrv.SetAs(strrev(tempbuff));
			free(tempbuff);

			if(!strrv.GetLength())
				return -1;

            tempbuff = strdup(strlwr.GetString());
			find = (wyChar*)strstr(strrv.GetString() + pos, strrev(tempbuff)); 
			free(tempbuff);
#else
	return -1;
#endif
		}
		else
		{
			find = (wyChar*)strstr(strrv.GetString() + pos, strlwr.GetString());
		}
	}
	
	//if string is found
    if(find)
	{
		result = (int)(find - strrv.GetString());
	}
	else
	{
		result = -1;
	}

    return result;
}

wyChar* 
wyString::StrIStr(wyChar* buffer, wyChar *to_find) { 
  // Check for NULL
  if (!*to_find) {
    return buffer;
  }
  // loop through all till a match found, otherwise just return null
  for ( ; *buffer; ++buffer) {
    if (toupper(*buffer) == toupper(*to_find)) {
       /* Matched starting char -- loop through remaining chars.*/
       wyChar *h, *n;
       for (h = buffer, n = to_find; *h && *n; ++h, ++n) {
          if (toupper(*h) != toupper(*n)) {
             break;
          }
       }
       if (!*n) {/* matched all of 'to_find' to null termination */
          return buffer; /* return the start of the match */
       }
    }
  }
  return NULL;
}

void wyString::JsonEscape() {
    wyInt32 index;
    wyChar ch;
    wyString temp;

    for (index = 0; index < GetLength(); index++) {
        ch = GetCharAt(index);

        switch (ch) {
            case '\"':
                Replace(index, 1, "\\\"");
                index++;
                break;
            case '\\':
                Replace(index, 1, "\\\\");
                index++;
                break;
            case '\b':
                Replace(index, 1, "\\b");
                index++;
                break;
            case '\f':
                Replace(index, 1, "\\f");
                index++;
                break;
            case '\n':
                Replace(index, 1, "\\n");
                index++;
                break;
            case '\r':
                Replace(index, 1, "\\r");
                index++;
                break;
            case '\t':
                Replace(index, 1, "\\t");
                index++;
                break;
            default:
            {
                if (ch > 0 && ch <= 0x1F) {
                    temp.Sprintf("\\u%04x;", ch);
                    Replace(index, 1, temp.GetString(), temp.GetLength());
                    index += (temp.GetLength() - 1);
                }
            }
        }
    }
}
