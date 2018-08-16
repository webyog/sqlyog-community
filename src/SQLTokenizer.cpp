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


/*	This file implements a generic SQL tokenizer. It is optimized much more for reading from
	file as used in YogImport and not so optimized for query window as the memory usage is
	not much in that aspect.

	Additional comments within the code :)		

	========================================================= */

/* conditional inclusion for read function */
#ifdef _WIN32
    #include <io.h>
#else
    #include<wctype.h>
    #include <unistd.h>    
    #include<wchar.h>
	#include <fcntl.h>
#endif

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "SQLTokenizer.h"
#include "ClientMySQLWrapper.h"
#include "MySQLVersionHelper.h"

/* simple macro to know whether serctype is from FILE or from buffer */

#define		IsFile(srctype)				(srctype==SRC_FILE)
#define		IsBuffer(srctype)			(srctype==SRC_BUFFER)

#define		C_TAB										'\t'
#define		C_SPACE										' '
#define		C_NULL										'\0'
#define		C_NEWLINE									'\n'
#define		C_CARRIAGE									'\r'
#define		C_OPEN_BRACKET								'('
#define		C_CLOSE_BRACKET								')'
#define		C_COMMA										','
#define		C_SEMICOLON									';'
#define		C_PLUS										'+'
#define		C_GREATER_THEN								'>'
#define		C_LESS_THEN									'<'
#define		C_EQUAL										'='
#define		C_EXCLAMATION								'!'
#define		C_AND										'&'
#define		C_OR										'|'
#define		C_STAR										'*'
#define		C_FRONT_SLASH								'/'
#define		C_BACK_SLASH								'\\'
#define		C_DASH										'-'
#define		C_HASH										'#'
#define		C_SINGLE_QUOTE								'\''
#define		C_DOUBLE_QUOTE								'\"'
#define		C_TILDE										'`'


wyInt32 IsPrefix (const wyChar *s, const wyChar *t)
{
  while(*t)
    if(*s++ != *t++)return 0;
  return 1;					/* WRONG */
}

wyChar*
TrimLeft(wyChar *sCurrPos)
{
	while(*sCurrPos)
		if(!isspace(*sCurrPos++))
			return --sCurrPos;
	
	return 0;
}

wyWChar*
TrimLeft(wyWChar *sCurrPos)
{
	while(*sCurrPos)
		if(!iswspace(*sCurrPos++))
			return --sCurrPos;
	return 0;
}

void 
TrimSpacesAtEnd(wyChar *sCurrPos)
{
	wyChar *cEnd, *p;

    cEnd = sCurrPos + strlen(sCurrPos);

    while(cEnd > sCurrPos && (p=cEnd-1, isspace(*p)))
		cEnd = p;
    
	*cEnd = '\0';
} 

void 
TrimSpacesAtEnd(wyWChar *sCurrPos)
{
	wyWChar *cEnd, *p;

    cEnd = sCurrPos + wcslen(sCurrPos);

    while(cEnd > sCurrPos && (p=cEnd-1, iswspace(*p)))
		cEnd = p;
    
	*cEnd = '\0';
} 

SQLTokenizer::SQLTokenizer(Tunnel *tunnel, PMYSQL mysql, YOGTOKENSOURCE srctype, void * source, wyUInt32 len)
{
	wyString	    filename;
    m_tunnel        =   tunnel;
	m_mysql         =   mysql;
	m_srctype		=	srctype;
	m_srclen		=	len;
	m_src			=	calloc(len+1, 1);
	m_lastcopy		=	(wyChar*)m_src;
	m_bufferend	    =   wyFalse;
    m_supportedversion = wyFalse;
	

	memcpy(m_src, source, len);
	
	if(m_lastcopy)
		filename.SetAs(m_lastcopy);

	/* if its of file type then we open up the file and keep the pointer */
	if(IsFile(m_srctype))
    {
#ifdef _WIN32
		m_importfile = _wopen(filename.GetAsWideChar(), O_RDONLY);
#else
		m_importfile = open(filename.GetString(), O_RDONLY);
#endif // _WIN32

		
	}
	else
		m_importfile = -1;

	InitStuffs();
}

SQLTokenizer::~SQLTokenizer()
{

	/* basically free up all the resources */
	if(m_src)
		free(m_src);

	if(m_query)
		free(m_query);

	if(m_importfile >= 0)
		close(m_importfile);

	if(m_bufferedline->m_buf)
		free(m_bufferedline->m_buf);

	if(m_bufferedline)
		free(m_bufferedline);
}

const wyChar*			
SQLTokenizer::GetQuery(wyUInt32 * len, wyUInt32 * linenum, 
						    wyInt32 *isdelimeter, wyChar * deli)
{
	wyString	querystr, filename;
	wyChar		*delimiter;
	wyChar		*delipos;
    wyBool      isansi;

    m_supportedversion = wyFalse;

	/* check for two error condition
		a)m_importfile is null and m_srctype == FILE. Thus things cannot be read from file so return error */
	if(IsFile(m_srctype)&& !m_importfile)
		goto error;	/* errror no. 1 */

	/* before sending we realloc the buf size to original */
	m_buffsize = LINE_LENGTH;

	m_query[0] = 0;

	for(;;)
	{
		if(IsFile(m_srctype))
        {
            m_line = SetLine();
            if(m_linenumber == 0)
            {   
                if(m_lastcopy)
                    filename.SetAs(m_lastcopy);
                
                isansi = CheckForUtf8Bom(filename);
                if(isansi == wyFalse)
                    m_line = m_line + 3; 
            }
        }
		else if(m_bufend)
			m_line = SetLine();

		if(IsFile(m_srctype))
			m_linenumber++;  // keep track of line number
        else if(m_addlinenum)
        {
			m_linenumber++;
			m_addlinenum = wyFalse;
		} 
        else 
			m_addlinenum = wyTrue;

		if(!m_line)
            break;
   
   		/* if the line is a single line comment we ignore it */
   		if(IsFile(m_srctype)&& !m_instring &&(m_line[0] == '#' || 
                (m_line[0] == '-' && m_line[1] == '-')|| m_line[0] == 0))
   			continue;		
   
   		/* parse and add it to the buffer, add line will return once it finds a valid query */
		if(AddLine(/*m_line*/)== 1)
        {
            m_tquery = GetCommentLex(m_query);

			if(IsFile(m_srctype)&& !TrimLeft(m_tquery))
				continue;

			/*	now it might be just a delimeter changing query so we dont this query.
				we just internaly change the delimter value */
			delimiter = TrimLeft(m_tquery);

			/* bug as reported at: http://www.webyog.com/forums/index.php?showtopic=1801&st=0&p=7879&#entry7879 */
			/* the query might be just of length 0 */
			if(!delimiter)
				goto success;

			if(0 == strnicmp(delimiter, "delimiter", 9) && m_strsize)
            {
				TrimSpacesAtEnd(delimiter);

				delipos = TrimLeft(delimiter + 10);
				
				/* fix a bug as reported in evenum id# 66..double delimiter crashes the query */
				if(delipos)
                {
					m_delimiterlen = strlen(strncpy(m_delimiter, delipos, 15));
					
					/* we send the delimiter value too */
					/* required for batch import using PHP tunneling */
					if(deli)
						strncpy(deli, m_delimiter, 10);
				}

                
                // mark the end of the line, so allow to add the linecount in hte next iteration!
                m_addlinenum = wyTrue;
                m_bufend = wyTrue;
                

				*len = m_strsize;
				*linenum = m_linenumber;

             

				*isdelimeter = 1;
				m_strsize = 0;
				return delimiter;				/* get back to next query as delimeter is not a valid query and we just ignore it */
			}

			goto success;
		}
		
	}
	querystr.SetAs(m_query);

	if(!m_line && !m_bufferend && TrimLeft(querystr.GetAsWideChar())&& m_strsize)
    {
		m_bufferend = wyTrue;
		goto success;
	}

error:
	
	*len = *linenum = (wyUInt32)-1;
	*isdelimeter = 0;
	return 0;

success:

	*len = m_strsize;
	*linenum = m_linenumber;
	*isdelimeter = wyFalse;
	m_strsize = 0;			/* reschedule it */
	return m_query;
}

void
SQLTokenizer::InitStuffs()
{
	/* initialize */
	m_bytesread		= 0;
	m_linenumber	= 0;
	m_strsize		= 0;
	m_query			= NULL;
	m_instring		= 0;
	m_comment		= wyFalse;
    m_conditionalcomment = wyFalse;
	m_line			= 0;
	m_bufend		= wyTrue;
	m_addlinenum	= wyTrue;

	strcpy(m_delimiter, DEFAULT_DELIMITER);
	m_delimiterlen = strlen(DEFAULT_DELIMITER);
	m_query = (wyChar*)calloc(LINE_LENGTH, 1);
	*m_query = 0;
	m_buffsize = LINE_LENGTH;

	AllocAndInitBufferedLine(MAX_ALLOWED_PACKET + 512);
}

/* QUERY_TOKEN *AllocAndInitBufferedLine()
 * This fuction alloccates memory for line buffer
 */
QUERY_TOKEN *
SQLTokenizer::AllocAndInitBufferedLine(wyUInt32 max_size)
{	
    wyInt32     fileid;

	fileid = m_importfile;
	
	m_bufferedline =(QUERY_TOKEN*) malloc(sizeof(*m_bufferedline));
	
#ifdef _WIN32
	if(!m_bufferedline)
	{
		MessageBox(NULL, _(L"Not enough memory, application terminated! "), _(L"Error"), MB_ICONERROR | MB_OK);
		exit(0);
	}
#endif
   

	wyBool result = InitBufferedLineStructure((IsFile(m_srctype))?(fileid):(-1), IO_SIZE, max_size, m_bufferedline);

	if(result == wyFalse)
		return 0;
  
	return m_bufferedline;

}


/*	This fuction Initializes  QUERY_TOKEN  elements for the first time
 InitLineBufferStruct
 */

wyBool
SQLTokenizer::InitBufferedLineStructure(wyInt32 file, wyUInt32 size, wyUInt32 maxbuf, QUERY_TOKEN *buffer)
{
  
	memset((wyChar*)buffer, 0 , sizeof(buffer[0]));
 
	buffer->m_file	 = file;
	buffer->m_bufread= size;
	buffer->m_maxsize= maxbuf;
  
	/*	this allocation is actually only used when reading from file. 
		reading from buffer, we just copy m_src into it so that we can use the same buffer and code 
		for both mechanism. results in double memory usage but generaly people dont
		write mbs of sql in the sql window so it can be worked with :)*/
	if(IsFile(m_srctype))
        buffer->m_buf =(wyChar*)malloc(buffer->m_bufread + 1);
    else 
        buffer->m_buf =(wyChar*)calloc(m_srclen + 1, 1);
	
	buffer->m_end = buffer->m_buf;
	buffer->m_lineend = buffer->m_buf;
	buffer->m_buf[0] = 0;				
  
	return wyTrue;
}


/*This function calles FindLineLength()function to find out actual length 
  Removes '\n' at the end of each line to make single line query 	
 */
wyChar *
SQLTokenizer::SetLine()
{
	wyChar			*pos;
	wyUInt32        getlen;

    pos = FindLineLength(m_bufferedline, &getlen);
   	if(! pos)
		return 0;
    
    /* check for the newline wyChar and remove it */    
	if(getlen && pos[getlen-1] == '\n')
		getlen--;				
  
	m_bufferedline->m_readlength = getlen; 
	  
	//make null at the end of line
	pos[getlen] = 0;
  
    return pos;
}

/* This function finds out actual length of a line,
 and that can be used to execute MySQL real query
 it calles FillBuffer()fuction which returns Line to FinedLength 
 */

wyChar *
SQLTokenizer::FindLineLength(QUERY_TOKEN *buffer, wyUInt32 *out_length)
{
	wyChar      *pos;
	wyUInt32	length;
	
	/* end of line will be the start of new line */
	buffer->m_linestart	=	buffer->m_lineend;
  
	for(;;)
    {
    	pos = buffer->m_lineend; // set pointer at the end of line
    
       /* move pos pointer till newline wyChar and value of pos equal to null */
		while(*pos != '\n' && *pos)
			pos++;
   
		if(pos == buffer->m_end)
		{
			if((wyUInt32)(pos - buffer->m_linestart)< buffer->m_maxsize)
      		{
				if(!(length = FillBuffer(buffer)) || length ==(wyUInt32)-1)
					return 0;
				
				continue;
      		}

			pos--;					/* break line here */
		} 
    	
		buffer->m_lineend = pos + 1;
    	*out_length =(wyUInt32)(pos + 1 - buffer->m_eof - buffer->m_linestart);
		return buffer->m_linestart;
	}
}


/*This function reads maximum_allowed number of bytes(4096)
 in to the buffer from import file or the passed buffer
 */
wyUInt32
SQLTokenizer::FillBuffer(QUERY_TOKEN *buffer)
{
	wyUInt32	readcount;
	wyUInt32	bufbytes =(wyUInt32)(buffer->m_end - buffer->m_linestart);
    wyUInt32    startoffset;

	/* we dont need to do anything on buffer as its already full */
	if(buffer->m_eof)
		return 0;					

	for(;;)
	{
        /* pointer at the begining of new line */
		startoffset =(wyUInt32)(buffer->m_linestart - buffer->m_buf);

		readcount =(buffer->m_bufread - bufbytes)/ IO_SIZE;

		if((readcount *= IO_SIZE))
			break;

		buffer->m_bufread *= 2;

		/* allocates memory to strore a line  */
		if(!(buffer->m_buf =(wyChar*)realloc(buffer->m_buf, buffer->m_bufread+1)))
			return(wyUInt32)- 1;

		buffer->m_linestart= buffer->m_buf + startoffset;

		buffer->m_end = buffer->m_buf + bufbytes;
	}

	/* make room for new line by moving read bytes down in the buffer */ 
	if(buffer->m_linestart != buffer->m_buf)
	{
  		memmove((buffer->m_buf),(buffer->m_linestart), ((wyUInt32)bufbytes));
		buffer->m_end = buffer->m_buf + bufbytes;
	}

	/* depending upon source we copy buffer differently from different source */
	if(IsFile(m_srctype))
    {
		readcount	=(wyUInt32)read(buffer->m_file, (void*)buffer->m_end, readcount);
		m_bytesread += readcount;
	}
    else
    {
		readcount  = ReadFromBuffer(buffer->m_end, readcount); 
		m_bytesread += readcount;
	}

	/* call the callback function so that a user can update a gui */
	//m_gui_routine(m_lp_param, m_bytesread, m_numquery);

	if(!readcount && bufbytes && buffer->m_end[-1] != '\n')
	{
		buffer->m_eof = readcount = 1;
		*buffer->m_end = '\n';
	}

	buffer->m_lineend = (buffer->m_linestart = buffer->m_buf) + bufbytes;
	buffer->m_end += readcount;
	*buffer->m_end = 0;				

	return readcount;
}

wyUInt32 
SQLTokenizer::ReadFromBuffer(wyChar *str, wyUInt32 count)
{
	wyUInt32	bufremain	= (((wyChar*)m_src) + m_srclen) - m_lastcopy;	
	wyUInt32	tocopy		= (bufremain < count)?(bufremain):(count);

	memcpy(str, m_lastcopy, tocopy);
	m_lastcopy += tocopy;

	return tocopy;
}

 /* This function adds a single line to global buff  and delegates to ExcecuteQuery 
 fuction,where queries will get excecuted*/

wyInt32
SQLTokenizer::AddLine()
{
    wyInt32		len = 0;
	wyUChar		inchar;
    wyChar		*pos, *out, *padded;
	wyChar		*line = m_line;
    wyUInt32	olddelimiterlength, length; 
    wyBool		isdelimiter = wyFalse;

	if(IsFile(m_srctype)&& !line[0])
		return 0;

    if(strnicmp(m_line, "DELIMITER", 9) == 0)
        isdelimiter = wyTrue;

	for(pos=out=line; (inchar = (wyUChar)*pos); pos++)
	{
        len++;

        // Commented to Solve the issue reported  here http://www.webyog.com/forums//index.php?showtopic=3480
        // We should Not ignore Spaces and comments if it is read from a File in the Query Editor
		/*if(inchar > 0 && isspace(inchar)&&  out == line)
        {
			// we ignore space only for file type coz it matters in query editor 
			if(IsFile(m_srctype))
				continue;
		}*/

		if(!m_comment && inchar == '\\')
		{
			// Found possbile one character command like \c
			if(!(inchar = (wyUChar)*++pos))
					break;				// readline adds one '\'
			
			if(m_instring || inchar == NULL)	// \N is short for NULL
			{					// Don't allow commands in string
				*out++='\\';
				*out++=(wyChar)inchar;
				continue;
			}
		}
		else if ((	!m_comment  &&  !m_conditionalcomment &&
					(( isdelimiter == wyFalse && *pos == *m_delimiter && IsPrefix(pos + 1, m_delimiter + 1)) || !*pos ) 
					&& !m_instring ) || (inchar > 0 && isspace(inchar) && isdelimiter && len > 10 && 
                    isspace(m_line[len]) == 0)) 
		{					
			olddelimiterlength = m_delimiterlen;

			if(out != line)
			{
				if ((!Append(line, (wyUInt32)(out - line))))
					return 0;					//(out - line)will be line length
		
				out = line;
			}

			out	 =	line;
			pos	 += olddelimiterlength - 1;

			/*	now if its not the end of the line then we will have to start from the next character when
				a user calls the next GetQuery()function */
			if(*(pos+1))
            {
				m_bufend = wyFalse;
				m_line = pos+1;
			} 
            else
				m_bufend = wyTrue;

			return 1;
				
		}
		else if(m_comment == wyFalse && !m_instring &&
					(inchar == '#' || (inchar == '-' && pos[1] == '-' && pos[2] == ' ')))
        {
			/*	if its from file then we ignore it. otherwise we have to move till end of the line
			as we dont ignore the comments */
			//if(IsFile(m_srctype))// changed MANOJ
			//	break;					// comment to end of line but not required in buffer as we just progress
			//else 
            {
				while((inchar = *pos++)&& inchar)
					*out++=(wyChar)inchar;
			}
		}
		else if(!m_instring && inchar == '/' && *(pos+1)== '*' && *(pos+2)!= '!') // checking for comments
		{
			/* before advancing we add it to the buffer if its from buffer */
			//if(!IsFile(m_srctype)) // changed MANOJ
            {
				*out++ =(wyChar)inchar;
				inchar =(wyUChar)*(pos+1);
			}
			pos++;
			m_comment = wyTrue;    //if its a comment set m_comment to 1
		}
       else if(IsFile(m_srctype) && inchar == '/' && *(pos+1)== '*' && *(pos+2)== '!' && !m_instring && m_conditionalcomment == wyFalse) 
        {
            m_conditionalcomment = wyTrue;
            
            //if(IsFile(m_srctype)&& !m_comment) // changed MANOJ
				//*out++=(wyChar)inchar;
        }
        // testing--------------------------------------------------
        else if(IsFile(m_srctype) && m_conditionalcomment == wyTrue && 
                m_comment == wyTrue &&  inchar == '*' && *(pos + 1) ==  '/')
        {
            if(inchar == m_instring)
				m_instring = 0;

            m_comment = wyFalse;
        }
            // testing--------------------------------------------------
        else if(IsFile(m_srctype) && !m_instring && inchar == '*' && *(pos+1)== '/' && !m_instring && m_conditionalcomment == wyTrue) 
        {
            m_conditionalcomment = wyFalse;
            
            //if(IsFile(m_srctype)&& !m_comment)// changed MANOJ
			//	*out++=(wyChar)inchar;
        }
		else if(m_comment && inchar == '*' && *(pos + 1) ==  '/' )
		{
			/* before advancing we add it to the buffer if its from buffer */
			//if(!IsFile(m_srctype)) // changed MANOJ
            {
				*out++ =(wyChar)inchar;
				inchar =(wyUChar)*(pos+1);
			}
			pos++;
			m_comment = wyFalse;	
            len = 0;
            padded = LeftPadText(pos+1);

            if(strnicmp(padded, "DELIMITER", 9) == 0)
                isdelimiter = wyTrue;
		}      
		else
		{				
			if(inchar == m_instring)
				m_instring = 0;
			else if(!m_comment && !m_instring &&
				(inchar == '\'' || inchar == '"' || inchar == '`'))
            {
				/* check whether its ' or " as we need to skip delimeters around it */
				m_instring =(wyChar)inchar;
			}

			/* Add found wyChar to buffer */
			//if(IsFile(m_srctype)&& !m_comment)// changed MANOJ
			//	*out++=(wyChar)inchar;

		}
		

		/* we add everything when its in a buffer */
		if(!inchar) //&& !IsFile(m_srctype))// changed MANOJ
			break;

		//if(!IsFile(m_srctype)) // changed MANOJ
			*out++=(wyChar)inchar;
	}

	if(out != line)
	{ 
        if(isdelimiter == wyTrue && len > 10) 
        {
           if (out != line)
			{
                length = (wyUInt32)(out - line);
				if((!Append(line, length)))
					return 0;					//( out - line ) will be line length
		
				out=line;
			}

		    m_bufend = wyTrue;

			return 1;
        }

		*out++ = '\n';   // set up newline wyChar at end of the line
		length = (wyUInt32)(out - line);

		/* if its not a comment and query is still not ended,then append it */
		if(/*!IsFile(m_srctype)&& */Append(line, length)) // changed MANOJ
        {
			m_bufend = wyTrue;
			if(!m_comment)
				return -1;
			else
				return 0;
		} 
		else if((m_comment == wyFalse) && Append(line, length) && TrimLeft(m_query))
        {
			m_bufend = wyTrue;
			return -1;
		}
	}
    else if(IsFile(m_srctype) && m_conditionalcomment == wyTrue)
    {
        m_bufend = wyTrue;

        if(Append(line, strlen(line)))
            return -1;
    }
  
	m_bufend = wyTrue;

	return 0;
}

 /* This function Appends line to global buff in case of multiple 
 buffering
 */
wyBool 
SQLTokenizer::Append(const wyChar *str, wyUInt32 arglength)
{
	/* if line size is more then the size of globalbuffer , reallocate memory */
	if(arglength + m_strsize+ 1 >= m_buffsize)
	{
		m_buffsize  =  arglength + m_strsize + 2;
		
		m_query =(wyChar*)realloc(m_query, m_buffsize);
	}

    // Patch done after 6.1 Beta 4 for http://www.webyog.com/forums//index.php?s=b25975f7b41008434a7c2ed55e9d9405&showtopic=3558&pid=15127&st=0&#entry15127
    /*if(m_strsize && IsFile(m_srctype)) // append a space char if the queries are reading from a file
    {
        strcat(m_query + m_strsize, " ");
        m_strsize++;
    }*/

    	/* adding up new line to the global buffer  */
	memcpy(m_query + m_strsize, str, arglength);
	*(m_query + m_strsize + arglength)= 0;
	m_strsize += (arglength);
  
	return wyTrue;
}

// Lex coment.
// Three types of commenting style provided by mySQL is checked.

wyChar *   
SQLTokenizer::GetCommentLex(wyChar *m_ubuffer)
{
    wyInt32     m_ucurpos = 0;
	wyChar      curr = *(m_ubuffer+m_ucurpos);
	wyInt32     start = m_ucurpos;


    // new line/ Tab/ Space we will ignore 
    while((*(m_ubuffer+m_ucurpos) == C_NEWLINE) ||
        (*(m_ubuffer+m_ucurpos) == C_CARRIAGE) ||
        (*(m_ubuffer+m_ucurpos) == C_SPACE) ||
        (*(m_ubuffer+m_ucurpos) == C_TAB))
	m_ucurpos++;

    curr = *(m_ubuffer+m_ucurpos);

	if ( curr == C_FRONT_SLASH )
	{
		
		if ( *(m_ubuffer+m_ucurpos+1) == C_STAR )	// Its comment
		{
            if(*(m_ubuffer+m_ucurpos+2) == '!')
                return m_ubuffer;

			m_ucurpos += 2;

			while(1)
			{
				if((*(m_ubuffer+m_ucurpos) == C_STAR) && 
                   (*(m_ubuffer+m_ucurpos+1) == C_FRONT_SLASH))
				{
					m_ucurpos+=2;
					break;
				}
				else if(*(m_ubuffer+m_ucurpos) == '\0')
					break;

				m_ucurpos++;
			}
		}
		else								   // Its a arith.	
		{
			m_ucurpos++;
		}
	}
	else if(curr == C_HASH )					// Again a diff type of comment. of -- type
	{
		m_ucurpos++;

		while((*(m_ubuffer+m_ucurpos) != C_NEWLINE) && 
				(*(m_ubuffer+m_ucurpos) != C_NULL) && 
				(*(m_ubuffer+m_ucurpos) != C_CARRIAGE ))
			m_ucurpos++;
	}
	else if(curr == C_DASH)
	{
		if((*(m_ubuffer+m_ucurpos+1) == C_DASH) && 
          (*(m_ubuffer+m_ucurpos+2) == C_SPACE))
		{
			m_ucurpos += 2;
			while((*(m_ubuffer+m_ucurpos) != C_NEWLINE) &&
                 (*(m_ubuffer+m_ucurpos) != C_NULL) &&
                 (*(m_ubuffer+m_ucurpos) != C_CARRIAGE))
				m_ucurpos++;

		}
		else
			m_ucurpos++;
	}
	
	if(start == m_ucurpos)
		return m_ubuffer;
	
    return GetCommentLex(m_ubuffer + m_ucurpos);

}

wyChar*
SQLTokenizer::LeftPadText(wyChar* text)
{
	wyChar*	temp = text;

	while(temp)
	{
		if(!(isspace(*temp)))
			break;
		temp++;
	}
	return temp;
}
