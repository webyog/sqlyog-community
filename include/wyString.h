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


#include "Datatype.h"
#include <stdarg.h>
#include <ctype.h>

#ifndef _WIN32
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdarg.h>
#include <wchar.h>
#endif

/**
*	A macro that returns default Chunk size.
*/
#define     BUF_CHUNK_ALLOCATION    256

#ifndef _WYSTRING_H_
#define _WYSTRING_H_

/*! \class wyString
    \brief A simple implementation of a minimal string class.

    The main idea of the class is that it will do memory management internally, so that the 
    application does not face any buffer overflow issue.Basic methods like add, set etc. are implemented. We are not implementing any operator 
    overloading as we strive to keep the class as minimalist as possible.
*/

class wyString
{
public:

      
    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
    wyString();

	/// Constructor that will initialize the string with an initial value passed as parameter.
    /**
    Create a string with an initial data

    @param initbuf:     IN Data that the string needs to be initialized with.
    @param ismysql41:   IN Parameter of type wyBool which by default is wyTrue.
    */
	wyString(const wyChar* initbuf, wyBool ismysql41 = wyTrue);

    /// Constructor with initial buffer size.
    /**
    This will initialize the member variables and create a default buffer of initsize.
    Having a default buffer results in less memory management code like malloc/realloc

    @param initsize: IN Initial size that needs to be allocated.
    */
	
	wyString(wyUInt32 initsize);

	/// Constructor that will initialize the Wide string with an initial value passed as parameter.
    /**
    Create a string with an initial data

    @param initbuf: IN Data that the string needs to be initialized with.
    */
	wyString(const wyWChar *initbuf);

    /// Constructor that will initialize the string with an initial value passed as parameter.
    /**
    Create a string with an initial data

    @param initbuf:     IN Data that the string needs to be initialized with.
    @param initsize:    IN Initial buffer length so that we don't have to do strlen in the function to get the length.
    */
	wyString(const wyChar* initbuf, wyUInt32 initsize);
	
	/// Destructor
    /**
    Free up all the allocated resources
    */
	~wyString();


    wyString& operator=(const wyString& rhstr);

    /// Add string to existing data buffer
    /**
    Concatenate new buffer to older buffer. Similar to strcat()

    @param buffer:    IN buffer to append
    @returns Length of the string.
     */
    wyUInt32     Add(const wyChar *buffer);
    
    /// Add string to existing data buffer
    /**
    Concatenate new buffer to older buffer. Similar to sprintf()
    
    @param formatstring:     IN Format-control string.
    @param ...:     IN Optional arguments
    @sa wyString::Add(wyString& toadd)
    @returns Length of the buffer.
    */
    wyUInt32     AddSprintf(const wyChar* formatstring, ...);    

    /// Sets data to the string passed as parameter
    /**
    Copys buffer to the data. Similar to strcpy()

    @param toset:     IN String that needs to be set.
    @sa wyString::SetAs(const wyChar* toset, const wyUInt32 len)
    @sa wyString::SetAs(wyString&)
    @sa wyString::SetAs(wyString&, const wyUInt32 len)
    @returns Length of the buffer.
    */
	wyUInt32 SetAs(const wyWChar* toset);
	wyUInt32 SetAs(const wyWChar* toset, wyInt32 length);

    /// Sets a specific length of data to the string passed as parameter
    /**
    Copys buffer to the data. Similar to strncpy()

    @param toset:     IN String that needs to be set.
    @param len:     IN Number of characters to set.
    @sa wyString::SetAs(const wyChar* toset)
    @sa wyString::SetAs(wyString&)
    @sa wyString::SetAs(wyString&, const wyUInt32 len)
    @returns Length of the buffer.
    */
	wyUInt32	 SetAs(const wyChar* toset, const wyUInt32 len);

	/// Sets a specific length of data to the string passed as parameter
    /**
    Copys buffer to the data. Similar to strncpy()

    @param toset:     IN String that needs to be set.
    @param len:     IN Number of characters to set.
    @sa wyString::SetAs(const wyChar* toset)
    @sa wyString::SetAs(wyString&)
    @sa wyString::SetAs(wyString&, const wyUInt32 len)
    @returns Length of the buffer.
    */
	VOID		SetAsDirect(const wyChar* toset, const wyUInt32 len);

    /// Sets data to the string passed as parameter
    /**
    Copies buffer to the data. Similar to strcpy()
    @param str:     IN String that needs to be set.
    @sa wyString::SetAs(const wyChar* toset)
    @sa wyString::SetAs(const wyChar* toset, const wyUInt32 len)
    @sa wyString::SetAs(wyString&, const wyUInt32 len)
    @returns Length of the buffer.
    */
    wyUInt32     SetAs(wyString& str);

    /// Sets a specific length of data to the string passed as parameter
    /**
    Copies buffer to the data. Similar to strncpy()

    @param str:     IN String that needs to be set.
    @param len:     IN Number of characters to set.
    @sa wyString::SetAs(const wyChar* toset)
    @sa wyString::SetAs(const wyChar* toset, const wyUInt32 len)
    @sa wyString::SetAs(wyString&)
    @returns Length of the buffer.    
    */
	wyUInt32	SetAs(wyString& str, const wyUInt32 len);
	
	 /// Sets a specific length of data to the string passed as parameter
    /**
    Copies buffer to the data. Similar to strncpy()

    @param toset :     IN String that needs to be set.
    @param wybool:     flag which tells whether MySQL server version is newer than 4.1 or not.
    @sa wyString::SetAs(const wyChar* toset)
    @sa wyString::SetAs(const wyChar* toset, const wyUInt32 len)
    @sa wyString::SetAs(wyString&)
    @returns Length of the buffer. 
	*/
	wyUInt32	SetAs(const wyChar* toset, wyBool ismysql41 = wyTrue);	

    /**
    Performs a case insensitive compare with the string data

    @param tocompare:     IN String that needs to be compared.
    @returns < if string is less than tocompare, 0 if equal and > 0 if greater then tocompare.    
    */
	wyInt32    CompareI(const wyChar* tocompare);

	/**Perform case insensitive comparision for n nunber of characters
	@param tocompare:     IN String that needs to be compared.
	@param nochars  :	   IN nunber of characters to compare
	*/
	wyInt32    CompareNI(const wyChar* tocompare, wyInt32 nochars);

	/**
    Performs a case sensitive compare with the string data

    @param tocompare:     IN String that needs to be compared.
    @returns < if string is less than tocompare, 0 if equal and > 0 if greater then tocompare.    
    */
	wyInt32    Compare(const wyChar* tocompare);

    /**
    Performs a case insensitive compare with the string data

    @param tocompare:     IN String that needs to be compared.
    @returns < if string is less than tocompare, 0 if equal and > 0 if greater then tocompare.    
    */
	wyInt32    CompareI(wyString& tocompare);

	///CompreI after converting to Uppercase(stricmp works Lower case mode)
	/**
	@param tocompare : IN string to compare with
	@return 0 if no diff, -1 if tocompare is grater, 1 if tocompare is smaller
	*/
	wyInt32    CompareIUppercaseMode(const wyChar* tocompare);

	/**
    Performs a case sensitive compare with the string data

    @param tocompare:     IN String that needs to be compared.
    @returns < if string is less than tocompare, 0 if equal and > 0 if greater then tocompare.    
    */
	wyInt32    Compare(wyString& tocompare);

    /// Functions strips off number of characters from the string from end
    /** 
    Useful when a user wants to strip off certain number of characters from end. If size < the length,
    it terminates the string at 0. Thus it becomes zero length string
    
    @param      size:   IN Number of characters to strip
    @returns    New length of the string
    */
    wyUInt32    Strip(wyUInt32 size);

    /// Returns the string buffer as wyChar*
    /**
    Return the internal buffer as wyChar*.
    @returns String buffer.  
    */
    const wyChar* GetString();

    /// Returns length of string buffer
    /**
    Return the internal buffer length.
    @returns String length.  
    */
    wyUInt32       GetLength();

    /// Returns character from the buffer at the position specified.
    /** Instead of using direct array, we use this coz its safe as in the function we do a check that
    somebody has not asked for an index beyond what is allocated.

    @param      pos: Position for which you want to get the charcter from
    @returns    Character at that position or NULL if its beyond the allocated range
    */
    wyChar GetCharAt(wyUInt32 pos);

	/// Sets the character to the buffer at the position specified.
    /** Instead of using direct array, we use this coz its safe as in the function we do a check that
    somebody has not asked for an index beyond what is allocated.

    @param      pos: Position for which you want to get the charcter from
    @returns    0 for success and -1 for fail
    */
	wyInt32 SetCharAt(wyUInt32 pos, wyChar text);

    /// Initialize the buffer according to the formatstring specified
    /**
    @param formatstring:     IN Format-control string.
    @param ...:     IN Optional arguments
    @returns String length.  
    */
    wyUInt32    Sprintf(const wyChar* formatstring, ...);    

#ifdef _WIN32
    wyUInt32    SprintfP(const wyChar* formatstring, ...);    

    wyUInt32    SNprintfP(const wyChar* formatstring, va_list args, wyUInt32 length );
#endif

      ///returns the position of first occurrence of a search string in a string
    /**
    @param searchstr:  IN String to find
    @param pos:     IN position from where we need to search
    @returns wyInt32 position of the string if exists, otherwise -1.
    */
    wyInt32    Find(const wyChar *searchstr, wyUInt32 pos);

	///Returns the position of first occurance of search string (case insensitive)
	/**
	/**
    @param searchstr:  IN String to find
    @param pos		:     IN position from where we need to search
	*/
	wyInt32    FindI(wyChar *searchstr, wyUInt32 pos = 0);

    /// Extracts a substring from a string
    /**
    @param startpos	: IN starting position from where the substring starts
    @param nochars	: IN number of chars in the substring
    @returns wyChar* pointer to the substring
    */
    wyChar *   Substr(wyUInt32 startpos, wyUInt32 nochars);

    /// Replaces a substring in a string
    /**
    @param startpos		: IN starting position from where the substring starts
    @param repalcestrlen: IN number of chars in the substring
	@param replacestr	: IN String to replace
    @returns  wyInt32	new size of the string
	*/
    //wyInt32     Replace(wyInt32 startpos, wyInt32 repalcestrlen, const wyChar *replacestr);

	/// Replaces a substring in a string
    /**
    @param startpos		: IN Starting position from where the substring starts
    @param nochars		: IN No of characters tos delete
    @returns  wyBool	successful or not
	*/
    wyBool      Erase(wyUInt32 startpos, wyUInt32 nochars);

    // Gets the Ansi string
    wyChar*     GetAsAnsi();

	// Gets the Ansi string as selected codepage/encoding
    wyChar*     GetAsEncoding(wyInt32 encoding, wyInt32 *len);

	/// Trims the blank space by one from right
    /**
	@param rtrimcount : OUT how many spaces trimmed.
	@returns wyInt32 resulting length of the string
    */
	wyInt32 RTrim(wyInt32 *rtrimcount = NULL);

	/// Trims the blank space by one from left
    /**
	@param trimcount : OUT how many spaces trimmed.
	@returns wyInt32 resulting length of the string
    */
	wyInt32 LTrim(wyInt32 *ltrimcount = NULL);

	/// Substring
	wyChar  *m_substr;	

	/// Copies the the length of the WideCharacter.
	/**
	@returns wyWChar resulting Wide character
	*/
	wyWChar* GetAsWideChar(wyUInt32 *len = NULL, wyBool isreadonly = wyFalse);
	wyWChar* GetAsWideCharEnc(wyInt32 encoding, wyUInt32 *len);
	///inserting a string into the another string at specified localtion.
	/**
	@param insertpos: IN position at which string to be insert.
	@param insstr   : IN insert string   
	*/
	wyInt32  Insert(wyUInt32 insertpos, wyChar* insstr);

	wyUInt32 GetAsUInt32(void);
	
	wyInt32  GetAsInt32(void);

	wyInt64  GetAsInt64(void);
	double   GetAsDouble(wyChar **stopscan);

	/*find the first occurence of character*/
	wyInt32  FindChar(wyChar searchchar, wyUInt32 pos = 0);

	
	/// Clears the string
	/**
	@returns void
	*/
    void     Clear();

	// removes pChar(single/consecutive occurence) from the left of the buffer, 
	/*
	@param pchar : char to remove
	@returns void
	*/
	void			StripLChar(wyChar pchar);

	///Find and replace the substring
	/**
	@param findstr : IN substring to replace
	@param replacestr : IN substring to replace with
	@return length of modified string
	*/
	wyInt32 FindIAndReplace(wyChar* findstr, wyChar* replacestr);
	
	// strips up to a token(including token)
	/*
	@param token	: token to find and strip
	@param newstring: OUT the string upto token
	@returns wytrue if not empty, wyFalse otherwise
	*/
	wyBool			StripToken(wyChar * token, wyString * newstring);

	//Replace with substring
	/**
	@param startpos : IN starting pos of string
	@param replacestrlen : IN length of sustring to replace with
	@param : IN substring to replace with
	@return new length of modified string
	*/
	wyInt32 Replace(wyInt32 startpos, wyUInt32 substrlen, const wyChar *replacestr, wyUInt32 replacestrlen = -1);

	//converts all buffer to lower case
	/**
	@return VOID
	*/
	VOID			ToLower();

	VOID			ToUpper();

	///Escaping the URL (used with HTTP)
	/**
	@return VOID
	*/
	VOID			EscapeURL(wyBool preserve_reserved = wyFalse); 

	///Handle the url to escape
	/**	
	@param from : IN from position
	@param to   : IN to position
	@return VOID
	*/
	VOID			UrlEncode(const wyChar * from, wyChar * to, wyBool preserve_reserved); 
	
	///Check url
	/**
	@return 0
	*/
	wyInt32			IsUrlReservedSafe(wyChar character);

	///Check url
	/**
	@return 0
	*/
	wyInt32			IsUrlSafe(wyChar character); 

    ///Performs a case sensitive find and replaces the findstr with replace string
	/**
	@param findstr      : IN substring to replace
	@param replacestr   : IN substring to replace with
	@return length of modified string
	*/
	wyInt32             FindAndReplace(const wyChar* findstr, const wyChar* replacestr);

	///Returns the position of first occurance of search string (case insensitive)
	//If the 'isreverse = wyTrue', then it searches the substring from end
	/**
	/**
    @param searchstr	:  IN String to find
    @param pos			:     IN position from where we need to search
	@param isreverse	: if its wyTrue then reverse the main string and search string and do search
	*/
	wyInt32				FindIWithReverse(wyChar *searchstr, wyUInt32 pos, wyBool isreverse = wyFalse);

	void JsonEscape();
	
private:

    /// Actual data container.
    wyChar				*m_strdata;                
    
    /// UTF8 buffer
    wyChar              *m_utf8str;

	/// Bytes allocated for buffer 
    wyUInt32			m_allocated;  

    /// Actual bytes used in the buffer
    wyUInt32			m_strsize; 
	
	/// Bytes for operating on a Wide character
	wyWChar				*m_widecharbuffer;
	
	///Bytes for ANSI String
	wyChar				*m_ansibuffer;

	/// Initialize the buffer by default
    void            Init();

    /// Initialize the buffer with buffer size
    /**
    @param newbuffsize: IN Amount of buffer size that need to be allocated.
    */
    void            InitBuf(wyUInt32 newbuffsize);

    /// Allocate the buffer for the specified size.
    /**
    @param size: IN Buffer length.
    */
    void            Allocate(wyUInt32 size);
	
	/// Allocate the buffer for the specified size.
    /**
    @param size: IN Buffer length.
    */
	void			Allocate(const wyWChar *buftoallocate, int len);
	/// Add string with given length
    /**
    @param buffer: IN buffer
    @param length: IN buffer length
    @returns Length of the string.
     */
    wyUInt32    Add(const wyChar *buffer, wyInt32 length);

    /// Change the allocated size and String size.
    /**
    @param allocated: IN Allocated size.
    @param strsize: IN string size.
    */
	void			SetValues(wyUInt32 allocated, wyUInt32 strsize);

    /// Write formatted data to a string
    /**
    @param formatstring: IN Format-control string.
    @param args: Pointer to list of arguments. 
    @param length: Buffer index.
    */
    wyUInt32        SNprintf(const wyChar* formatstring, va_list args, wyUInt32 length );

    static wyChar* StrIStr(wyChar* buffer, wyChar *to_find);
	
	 // declare the default constructor which are not supported
	// in the private section so we won't accidently use this
	
	/*wyString(const wyString& p)
	{
	UNREFERENCED_PARAMETER(p);
	};

	wyString& operator=(const wyString& p) 
	{
		UNREFERENCED_PARAMETER(p);
		return *this;
	} */   

    wyString(const wyString &str);


};

#endif

