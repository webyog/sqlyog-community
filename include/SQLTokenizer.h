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


#ifndef __YOGTOKENIZER__
#define __YOGTOKENIZER__

#include <stdio.h>
#include "Datatype.h"
#include "Tunnel.h"
#include "CommonHelper.h"

#define			IO_SIZE					4096
#define			LINE_LENGTH				4096
#define			DEFAULT_DELIMITER		";"
#define			MAX_ALLOWED_PACKET		16*1024*1024

/**
*common trim function 
*/
wyChar * TrimLeft(wyChar *s);

/**
*  Trim function taking a WideChar Parameter and 
*  And then Trimmiming off trailing whitespaces
*/
wyWChar * TrimLeft(wyWChar *sCurrPos) ;


void TrimSpacesAtEnd(wyChar *s);

void TrimSpacesAtEnd(wyWChar *s);

wyInt32 IsPrefix (const wyChar *s, const wyChar *t);

/**
   * An enum.
   *
    SRC_FILE        =	0,
	SRC_BUFFER      =	1

*/
enum YOGTOKENSOURCE
{
	SRC_FILE=0,
	SRC_BUFFER
};


/*! \struct QUERY_TOKEN_st
    \brief Buffer line details
    \param  wyInt32          m_bufread;			Number of bytes to get with each read()
	\param  wyInt32          m_eof;             End of file
	\param  wyChar          *m_buf;				Buffer		
	\param  wyUInt32         m_maxsize;         Max size
	\param  wyUInt32         m_readlength;		Length of last read string 	
	\param  wyInt32	 		 m_file;			File name
	\param  wyChar          *m_end;				Pointer at buffer end 
	\param  wyChar          *m_linestart;		Starting line
    \param  wyChar          *m_lineend;			ending line     
*/
typedef struct query_token
{
	wyInt32              m_bufread;				
	wyInt32              m_eof;
	wyChar              *m_buf;				
	wyUInt32             m_maxsize;
	wyUInt32             m_readlength;			
	wyInt32				 m_file;
	wyChar              *m_end;					
	wyChar              *m_linestart;
    wyChar              *m_lineend;
	
} QUERY_TOKEN;

class SQLTokenizer
{

private:

    /// The original source query
	void				*m_src;

    /// Tunnel pointer
    Tunnel              *m_tunnel;

    /// MySQL pointer
    PMYSQL              m_mysql;

	/// Query length of all the queries
    wyInt32             m_query_len;

	

	/// Temporary buffer for query
    wyChar				*m_tquery;

    /// Another copy of original src query
	wyChar				*m_lastcopy;

    /// Temporary buffer to keep each query 
	wyChar				*m_query;			

    /// Current line of string
	wyChar				*m_line;

    /// String within quotes in the query 
	wyChar				 m_instring;

	
    /// End of buffer ?
	wyBool				 m_bufend;
	wyBool				 m_bufferend;

	/// Comment in the line ?
	wyBool				 m_comment;

    /// Conditional comment
    wyBool				 m_conditionalcomment;

    /// supported version or not?
    wyBool              m_supportedversion;

    /// Source length
	wyUInt32    		 m_srclen;

    /// Buffer size of the query
    wyUInt32    		 m_buffsize;
    
    /// Size of the string in the query
    wyUInt32    		 m_strsize;
    
    /// Source type	(File or Buffer)
	YOGTOKENSOURCE		 m_srctype;

    /// Buffered line 
	QUERY_TOKEN			*m_bufferedline;					

    /// File to read query from 
	int					m_importfile;						

	/// Number of bytes read from buffer
	wyUInt32    		 m_bytesread;

	/// Keep track of line numbers
	wyUInt32    		 m_linenumber;

	/// Considers the actual line numbers and not the dummy
	wyBool               m_addlinenum;

	/// Delimiter buffer
	wyChar               m_delimiter[16];

	/// Delimiter length
	wyInt32              m_delimiterlen;
    
	/// Initializes all the member variables
	/**
	@returns void
	*/
	void				InitStuffs ();

	/// This function calls FindLineLength()function to find out actual length 
	/// Removes '\n' at the end of each line to make single line query 	
	/**
	@returns a single line query
	*/	
	wyChar*				SetLine ();

	/// This function finds out actual length of a line,
	/// and that can be used to execute MySQL real query
	/// it calls FillBuffer()functions which returns Line to FinedLength 
	/**
	@param buffer		: IN Buffer containing query
	@param length		: OUT Length of the query
	@returns the string containing the query of exact size
	*/
	wyChar*				FindLineLength(QUERY_TOKEN *buffer, wyUInt32 *length);

	///	This function Initializes  QUERY_TOKEN  elements for the first time
	/// InitLineBufferStruct
	/**
	@param buffer		: IN/OUT Line details
	@param file			: IN File name
	@param size			: IN Size of the buffer
	@param max_buffer	: IN Max size of the buffer
	@returns wyTrue on success else wyFalse
	*/
	wyBool              InitBufferedLineStructure(wyInt32 file, wyUInt32 size, wyUInt32 maxbuf, QUERY_TOKEN *buffer);

	/// This function reads maximum_allowed number of bytes(4096)
	/// in to the buffer from import file or the passed buffer
	/**
	@param buffer		: IN/OUT Buffer line details
	@returns new size
	*/
	wyUInt32            FillBuffer(QUERY_TOKEN *buffer);

	/// Reads query again from the memory
	/**
	@param t			: IN/OUT Buffer 
	@param count		: IN Size to read
	@returns the size of the buffer read
	*/
	wyUInt32            ReadFromBuffer (wyChar  *t, wyUInt32 count );

	/// This function allocates memory for line buffer
	/**
	@param max_size		: IN Max buffer size
	@returns buffered line
	*/
	QUERY_TOKEN*		AllocAndInitBufferedLine(wyUInt32 max_size );

	/// This function adds a single line to global buff  and delegates to ExcecuteQuery 
	/// function,where queries will get executed*/
	/**
	@returns 0 on success else -1
	*/
	wyInt32 AddLine();
	
	/// This function Appends line to global buff in case of multiple buffering
	/**
	@param string		: IN String to append
	@param arg_length	: IN Length of the string
	*/
	wyBool  Append(const wyChar *string, wyUInt32 arg_length);

	/// Three types of commenting style provided by mySQL is checked.
	/**
	@param m_ubuffer	: Buffer containing comment
	@returns string omitting the comment
	*/
    wyChar*   GetCommentLex(wyChar *m_ubuffer);

	/// Takes out the blank space from the given text
	/**
	@param text			: Text string
	@returns text removing
	*/
    wyChar*    LeftPadText(wyChar *text);

public:

	SQLTokenizer(Tunnel *tunnel, PMYSQL mysql, YOGTOKENSOURCE srctype, void *source, wyUInt32 len);
    ~SQLTokenizer();

	const wyChar *GetQuery(wyUInt32 *len, wyUInt32 *linenum, wyInt32 *isdelimiter, wyChar *delimiter = NULL);

	
};

#endif
