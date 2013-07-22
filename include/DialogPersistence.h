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




#include "FrameWindowHelper.h"
#include "List.h"

#ifndef __PERSISTENCE__
#define __PERSISTENCE__


/**
   * Wrapper over the different type
*/
enum PersistWinType
{
	TEXTBOX = 0,/**< TEXTBOX, Equivalent to 0*/
	CHECKBOX,/**< CHECKBOX, Equivalent to 1*/
	COMBOBOX_P/**< COMBOBOX_P, Equivalent to 2*/
};


class DialogPersistence : public wyElem
{
public:
    /// constructor
    /**
    @param parent: IN parent window handle
    @param id: IN element id
    @param attribute: IN identifier name
    @param defvalue: IN Default value
    @param section: IN section name
    @param t: IN what type of control (PersistWinType)
    */
    DialogPersistence(HWND parent, wyInt32 id, const wyChar *attribute, const wyChar *defvalue, const wyChar *section, PersistWinType t);

    /// Destructor
    /**
    */
	~DialogPersistence();
    /// Reads from the file
    /**
    @returns void
    */
	void	ReadFromFile();

    /// Writes to the file
    /**
    @returns void
    */
	void	WriteToFile();
	
    /// Parent window handle
	HWND			m_hwndparent;
    /// Control id
	wyInt32			m_id;
    /// Control type(PersistWinType)
	PersistWinType	m_type;
    /// Default value
	wyString        m_defvalue;
    /// Unique attribute name in that section
	wyString        m_attribute;
    /// Section name
	wyString        m_section;
};


class Persist
{
public:

    /// Default constructor
	Persist();
    /// Destructor
	~Persist();

    /// Creates a new section
    /**
    @param section name: IN new section name
    */
	void	Create(wyChar * sectionname);

    /// Adds a particular item in to the linked list
	void	Add(HWND hwnd, wyInt32 id, wyChar * attribute, wyChar * defvalue, PersistWinType t);

    /// Deletes a particular attribute from the current section
    /**
    @param attribute: IN attribute identifier
    @returns void
    */
	void	DeleteFromSection(wyWChar * attribute);

    /// Cancels the write part
    /**
    @returns void
    */
    void    Cancel();

private:
    /// Linked list
	List	    m_list;	
    /// Current section name
	wyString    m_section;
    /// Helps to skip the writing part
	wyBool 	    m_cancel;
};

#endif
