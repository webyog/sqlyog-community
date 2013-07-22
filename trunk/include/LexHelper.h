/* Copyright (C) 2012 Webyog Inc.

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


#ifndef _CLEXER_H_
#define _CLEXER_H_

#include "FrameWindowHelper.h"


enum obj_type
{
	OBJ_DATABASE = 3,/**< OBJ_DATABASE, means database.*/  
	OBJ_TABLE,/**< OBJ_TABLE, means table.*/
    OBJ_VIEW,
	OBJ_PROCEDURE = 6,/**< OBJ_PROCEDURE, means procedure.*/
	OBJ_FUNCTION,/**< OBJ_FUNCTION, means function.*/
	OBJ_TRIGGER,
	OBJ_EVENT
};

/*! Creates a type obj_type*/ 
typedef obj_type OBJTYPE;

/**
  *This will define the type of operation.
  *Operations can be any one of these.
  */
enum oper_type
{
	OPER_CREATE	=	1,/**< OPER_CREATE, means create operation.*/
	OPER_DROP,/**< OPER_DROP, means drop operation.*/
	OPER_ALTER,/**< OPER_ALTER, means alter operation.*/
	OPER_RENAME/**< OPER_RENAME, means rename operation.*/
};

/*! Creates a type oper_type*/ 
typedef oper_type OPERTYPE;

/*! \struct OPDETAILS
    \brief This will store the details of operation
    \param OPERTYPE op_type, type of operation
    \param OBJTYPE  obj_type, type of object
    \param wyChar	table[512], table name / object name
    \param wyChar	db[512], dbname if present
    \param wyChar   todb[512], tgt dbanem if present
    \param wyChar	totable[512], to table name if present
*/
struct OPDETAILS
{
	OPERTYPE    op_type;
	OBJTYPE     obj_type;
	wyChar	    table[512];
	wyChar	    db[512];
	wyChar	    todb[512];
	wyChar	    totable[512];
};


class LexHelper
{

public:

	/// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	LexHelper();

     /// Destructor
    /**
    Free up all the allocated resources
    */
	~LexHelper();

    /// Invokes the parsing
    /**
    @param buffer			: IN the buffer to parse
    @param dwcurpos			: IN starting position
    @param opdetails		: OUT operation details
    @returns wyInt32, 1 i success, otherwise 0.
    */
	wyInt32 StartLex(wyChar *buffer, wyUInt32 dwcurpos, OPDETAILS *opdetails, wyBool isuse = wyFalse);
	
    /// Moves the cursor position the next non space char
    /**
    @returns wyUInt32 new cursor position
    */
	wyUInt32 SkipWhiteSpaces();

    ///Skips the Comment in the query -- Three types of comment
    /**
    @returns wyBool, wyTrue is there is any comment string, otherwise wyFalse
    */
	wyBool   GetCommentLex();

    ///Gets the word from the current cursor position
    /**
    @param word				: OUT word from cursor position
	@param bufflen			: IN Buffer length
    @returns wyInt32, Number of chars in the word.
    */
	wyInt32  GetWordLex(wyChar *word, wyInt32 bufflen);

    ///Gets the query operation type, like "Drop", "Create", "Alter" and "rename"
    /**
    @param oper				: OUT operation type
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
    wyBool   GetOperationType(wyInt32 *oper, wyBool isuse);

    ///Gets the query object type like "Table", "procedure" or funtion
    /**
    @param obj				: OUT object type
    @returns wyBool, wyTrue if success, otherwise wyFalse
    */
    wyBool   GetObjectType(wyInt32 *obj);

    /// Buffer to parse
	wyChar      *m_buffer;

    /// Current cursor position
	wyUInt32    m_curpos;
    
	/// Flag to check all types of parsing is over or not, initially it will be wyFalse
	wyBool      m_all;
};

#endif
