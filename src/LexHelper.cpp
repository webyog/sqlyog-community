/* Copyright (C) 2012 Webyog Inc

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


#include "LexHelper.h"

LexHelper::LexHelper()
{
	m_buffer = NULL;
	m_all = wyFalse;
}

LexHelper::~LexHelper()
{
}

// Start lexing the buffer which is passed as parameter.
// As any informationis found we allocate new structure about the text and add it to the 
// linked list whose infomation is stored globally so that we can traverse through it and
// color the text.

wyInt32 
LexHelper::StartLex(wyChar * buffer, wyUInt32 dwcurpos, OPDETAILS	*opdetails, wyBool isuse)
{
	wyChar  curr;
    wyChar  table[SIZE_512] = {0};
	wyChar  totable[SIZE_512] = {0};
	
    wyBool  bop = wyFalse, btype = wyFalse, btable = wyFalse, bif = wyFalse, btotable = wyFalse, bnot = wyFalse;
	wyInt32 oper = 0, obj = 0;
    const wyChar* tempsubstr;

	m_buffer = buffer;
	m_curpos = 0;
	
	if(!m_buffer)
		return -1;
	
	while(1)
	{
		curr = *(m_buffer + m_curpos);

		if(curr == C_NULL)
			break;

		m_curpos = SkipWhiteSpaces();

		// Get words.
		// The function will make a node depending upon of keywords or function name.
		if(bop == wyFalse)
		{
            if(GetOperationType(&oper, isuse) == wyFalse)
                break;
			
			bop = wyTrue;

            if(isuse == wyTrue)
                btype = wyTrue;
			
		}
        else if(btype == wyFalse)
		{
            if(GetObjectType(&obj) == wyFalse)
                break;
			
			btype = wyTrue;
			
		} else if(!btable)
		{
			if(!GetWordLex(table, SIZE_512))
				break;
				
			if(stricmp(table, "if")== 0)
			{
				bif = wyTrue;
			}
            else if(stricmp(table, "not") == 0 && bif == wyTrue)
            {
                bnot = wyTrue;
            }
			else if((stricmp(table, "exists")!= 0)|| bif == wyFalse)
			{
				btable = wyTrue;
				m_all  = wyTrue;
			}
		}
		
		if(m_all == wyTrue && oper == OPER_RENAME && obj == OBJ_TABLE)
		{
			if(btotable == wyFalse)
			{
				btotable = wyTrue;
				continue;
			}
			else
			{
				if(!GetWordLex(totable, SIZE_512))
					break;
				
				if(stricmp(totable, "TO")== 0)
					continue;

				wyString    strto(totable);
				wyString    dbto, tblto;
				wyInt32     pos;
		
				if(strto.GetLength() == 0)
					break;
				
				pos = strto.Find(".", 0);

				if(pos != -1)	
				{
					dbto.SetAs(strto.Substr(0, pos));
					if(dbto.GetCharAt(0) == '`')
					{
						dbto.Erase(0, 1);
						dbto.Strip(1);
					}
				}
				else
					dbto.SetAs("");
					
				tblto.SetAs(strto.Substr(pos + 1, strto.GetLength()));
				
				if(tblto.GetCharAt(0) == '`')
				{
					tblto.Erase(0,1);
					tblto.Strip(1);
				}
				strncpy(opdetails->totable, tblto.GetString(), SIZE_512-1);
				strncpy(opdetails->todb, dbto.GetString(), SIZE_512-1);

			}
		}
		
		if(btable)
			break;
	}
	
	if(!m_all)
	{	
		return 1;
	}
	else
	{		
		opdetails->op_type =(OPERTYPE)oper;
		opdetails->obj_type =(OBJTYPE)obj;	
		
		wyString    str, db, tbl;
		wyInt32     position;
		str.SetAs(table);
		
		if(str.GetLength() == 0)
			return 1;
		
		position = str.Find(".", 0);
			
		if(position != -1)	
		{
			db.SetAs(str.Substr(0, position));
			if(db.GetCharAt(0) == '`')
			{
				db.Erase(0, 1);
				db.Strip(1);
			}
		}
		else
        {
            position = -1;
			db.SetAs("");
        }
		
        if((tempsubstr = str.Substr(position + 1, str.GetLength())))
        {
            tbl.SetAs(tempsubstr);
        }
		
		if(tbl.GetCharAt(0) == '`')
		{
			tbl.Erase(0,1);
			tbl.Strip(1);
		}
		strncpy(opdetails->table, tbl.GetString(), SIZE_512-1);
		strncpy(opdetails->db, db.GetString(), SIZE_512-1);
	}

	return 0;
}

// Function to skip white spaces when lexing because we dont need any data about it.
wyUInt32 
LexHelper::SkipWhiteSpaces()
{
	while(*(m_buffer+m_curpos)== C_SPACE ||
		    *(m_buffer+m_curpos)== C_TAB  ||
			*(m_buffer+m_curpos)== C_NEWLINE  ||
			*(m_buffer+m_curpos)== C_CARRIAGE)
		m_curpos++;

	return m_curpos;
}

// Lex coment.
// Three types of commenting style provided by mySQL is checked.
wyBool   
LexHelper::GetCommentLex()
{
	wyChar curr = *(m_buffer+m_curpos);
	wyInt32 start = m_curpos;

	if(curr == C_FRONT_SLASH)
	{
		
		if(*(m_buffer+m_curpos+1)== C_STAR)	// Its comment
		{
			m_curpos += 2;

			while(1)
			{
				if((*(m_buffer+m_curpos)== C_STAR)&&(*(m_buffer+m_curpos+1)== C_FRONT_SLASH))
				{
					m_curpos+=2;
					break;
				}
				else if(*(m_buffer+m_curpos)== '\0')
					break;

				m_curpos++;
			}
		}
		else								   // Its a arith.	
		{
			m_curpos++;
		}
	}
	else if(curr == C_HASH)					// Again a diff type of comment. of -- type
	{
		m_curpos++;

		while((*(m_buffer+m_curpos)!= C_NEWLINE)&& 
				(*(m_buffer+m_curpos)!= C_NULL)&& 
				(*(m_buffer+m_curpos)!= C_CARRIAGE))
			m_curpos++;
		
	}
	else if(curr == C_DASH)
	{
		if((*(m_buffer+m_curpos+1)== C_DASH)&&(*(m_buffer+m_curpos+2)== C_SPACE))
		{
			m_curpos += 2;
			while((*(m_buffer+m_curpos)!= C_NEWLINE)&&(*(m_buffer+m_curpos)!= C_NULL)&&(*(m_buffer+m_curpos)!= C_CARRIAGE))
				m_curpos++;
		}
		else
		{
			m_curpos++;
		}
	}
	
	if(start == m_curpos)
		return wyFalse;
	
	 return wyTrue;
}

// Function get value data. that is text between single quote or double quote.
// This is only found in insert or update statement or queries where the datas is changed.

// Function to get main words like function words, keywords or arith words
wyInt32 
LexHelper::GetWordLex(wyChar *word, wyInt32 bufflen)
{
	wyInt32 i = 0;
	wyBool bquote = wyFalse;

    word[0] = 0;

	while(1)
	{
		if(((m_buffer[m_curpos] == C_NULL)/*||
			(m_buffer[m_curpos] == C_OPEN_BRACKET)|| 
			(m_buffer[m_curpos] == C_CLOSE_BRACKET)|| 
			(m_buffer[m_curpos] == C_COMMA)||
			(m_buffer[m_curpos] == C_SEMICOLON)||
			(m_buffer[m_curpos] == C_PLUS)||
			(m_buffer[m_curpos] == C_STAR)||
			(m_buffer[m_curpos] == C_GREATER_THEN)||
			(m_buffer[m_curpos] == C_LESS_THEN)||
			(m_buffer[m_curpos] == C_EQUAL)||
			(m_buffer[m_curpos] == C_EXCLAMATION)||
			(m_buffer[m_curpos] == C_AND)||
			(m_buffer[m_curpos] == C_OR)||
			(m_buffer[m_curpos] == C_FRONT_SLASH)||
			(m_buffer[m_curpos] == C_HASH)||
			(m_buffer[m_curpos] == C_DASH)||
			(m_buffer[m_curpos] == C_SINGLE_QUOTE)||
			(m_buffer[m_curpos] == C_DOUBLE_QUOTE)*/||
			(m_buffer[m_curpos] == C_CARRIAGE)||
			(m_buffer[m_curpos] == C_NEWLINE)||
			(m_buffer[m_curpos] == C_SPACE)||
			(m_buffer[m_curpos] == C_TAB)||
            (m_buffer[m_curpos] == C_OPEN_BRACKET))&& !bquote)
			break;
			
		if(m_buffer[m_curpos] == '`')
            bquote = (bquote == wyTrue)? wyFalse: wyTrue;

		word[i] = m_buffer[m_curpos];
		i++;
		m_curpos++;
        
		//make sure there's no overflow
		if(i == (bufflen - 1))
			break;

		if(m_buffer[m_curpos-1] == '`' && 	 m_buffer[m_curpos] != '.' && bquote == wyFalse && word[0] == '`')
			break;
	}

	word[i] = 0;
	
	return i; // char count
}

wyBool 
LexHelper::GetOperationType(wyInt32 *oper, wyBool isuse)
{
    wyChar  op_word[512] = {0};

	if(!GetWordLex(op_word, 512))
		return wyFalse;
							
	if(stricmp(op_word , "Create")== 0)
		*oper = OPER_CREATE;
	else if(stricmp(op_word , "Drop")== 0 || isuse == wyTrue)
		*oper = OPER_DROP;
	else if(stricmp(op_word , "Alter")== 0)
		*oper = OPER_ALTER;
	else if(stricmp(op_word , "Rename")== 0)
		*oper = OPER_RENAME;
	else
		return wyFalse;

    return wyTrue;
}

wyBool 
LexHelper::GetObjectType(wyInt32 *obj)
{
    wyChar  obj_type[512] = {0};

	if(!GetWordLex(obj_type, 512))
		return wyFalse;

	if(strnicmp(obj_type, "OR", 2) == 0) 
	{
		m_curpos = SkipWhiteSpaces();
        
		if(!GetWordLex(obj_type, 512))
			return wyFalse;
        else
        {
            if(strnicmp(obj_type, "REPLACE", 7) == 0)
            {
                m_curpos = SkipWhiteSpaces();
                
                if(!GetWordLex(obj_type, 512))
			        return wyFalse;
            }
            else
                return wyFalse;
        }
	}

    if(strnicmp(obj_type, "ALGORITHM=", 10) == 0) 
	{	
        if(strlen(obj_type) == 10)
        {
            m_curpos = SkipWhiteSpaces();
            
            if(!GetWordLex(obj_type, 512))
                return wyFalse;
        }

        m_curpos = SkipWhiteSpaces();
        
        if(!GetWordLex(obj_type, 512))
			return wyFalse;
	}
    else if(strnicmp(obj_type, "ALGORITHM", 9) == 0)
    {
        m_curpos = SkipWhiteSpaces();
        
        if(!GetWordLex(obj_type, 512))
            return wyFalse;
        else
        {
            if(strlen(obj_type) == 1)
            {
                m_curpos = SkipWhiteSpaces();
                
                if(!GetWordLex(obj_type, 512))
                    return wyFalse;
            }
        }

        m_curpos = SkipWhiteSpaces();
        
        if(!GetWordLex(obj_type, 512))
            return wyFalse;
    }
    
    if(strnicmp(obj_type, "DEFINER=", 8) == 0) 
	{	
        if(strlen(obj_type) == 8)
        {
            m_curpos = SkipWhiteSpaces();
        
            if(!GetWordLex(obj_type, 512))
                return wyFalse;
        }

        m_curpos = SkipWhiteSpaces();
        
        if(!GetWordLex(obj_type, 512))
			return wyFalse;
	}
    else if(strnicmp(obj_type, "DEFINER", 7) == 0)
    {
        m_curpos = SkipWhiteSpaces();
        
        if(!GetWordLex(obj_type, 512))
            return wyFalse;
        else
        {
            if(strlen(obj_type) == 1)
            {
                m_curpos = SkipWhiteSpaces();
        
                if(!GetWordLex(obj_type, 512))
                    return wyFalse;
            }
        }

        m_curpos = SkipWhiteSpaces();
        
        if(!GetWordLex(obj_type, 512))
            return wyFalse;
    }
	
    if(strnicmp(obj_type, "SQL",3) == 0)
    {
        m_curpos = SkipWhiteSpaces();
        
        if(!GetWordLex(obj_type, 512))          // get SECURITY key word for views
            return wyFalse;
        else
        {
            m_curpos = SkipWhiteSpaces();
        
            if(!GetWordLex(obj_type,512))   // get SQL SECURITY parameter for views
                return wyFalse;
            else
            {
                m_curpos = SkipWhiteSpaces();
        
                if(!GetWordLex(obj_type, 512))      // get probable Object type if above keywords are used
                    return wyFalse;
            }
        }
    }

	if((stricmp(obj_type , "Table") == 0))
        *obj = OBJ_TABLE;
    else if(stricmp(obj_type , "View") == 0)
		*obj = OBJ_VIEW;
	else if(stricmp(obj_type , "Database") == 0)
		*obj = OBJ_DATABASE;
	else if(stricmp(obj_type , "Procedure") == 0)
		*obj = OBJ_PROCEDURE;
	else if(stricmp(obj_type , "Function") == 0)
		*obj = OBJ_FUNCTION;
	else if(stricmp(obj_type , "Trigger") == 0)
		*obj = OBJ_TRIGGER;
	else if(stricmp(obj_type , "Event") == 0)
		*obj = OBJ_EVENT;

	else
		return wyFalse;

    return wyTrue;
}