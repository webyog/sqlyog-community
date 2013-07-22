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

#ifndef WY_LIST
#define WY_LIST

#ifdef _WIN32
#include <windows.h>
#endif
#include "Verify.h"
#include "Datatype.h"


class wyElem;
class List;


class wyElem 
{
public:

    /// Previous element pointer
	wyElem* m_prev;

    /// next element pointer
	wyElem* m_next;

    //added for generic delete function. -- by Vishal
    virtual ~wyElem()
    {
    }
};

class List 
{
    /// pointer to first element
	wyElem*	m_first;

    /// pointer to last element 
	wyElem*	m_last;

    /// element count
	wyUInt32    m_elemcount;

public:

    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    */
	List();
    
	/// Destructor
    /**
    Free up all the allocated resources
    */
	virtual ~List();

    /// returns the pointer to first node in the linked list
    /**
    @returns wyElem*	
    */
	wyElem*	GetFirst(){return m_first;};
    
	/// returns the pointer to last node in the linked list
    /**
    @returns wyElem*
    */
	wyElem*	GetLast(){return m_last;};

    /// Inserts a node at last
    /**
    @param pelem			: IN element to add
    @returns wyElem* new added node	
    */
	wyElem*	Insert(wyElem * pelem);
    
    /// removes a node from the last
    /**
    @param pelem			: IN element to remove
    @returns wyElem* next node in the linked list
    */
	wyElem*	Remove(wyElem * pelem);
    
	/// removes a node from the last
    /**
    @param index			: IN index of the item to remove
    @returns wyElem* next node in the linked list
    */
	wyElem*	Remove(wyUInt32 index);

    /// inserts a node before the given node
    /**
    @param pbefore		: IN before which element
    @param pelem		: IN element to add
    @returns wyElem* node added
    */
	wyElem*	InsertBefore(wyElem* pbefore, wyElem* pelem);
    
	/// inserts a node after the given node
    /**
    @param pafter		: IN after which element
    @param pelem		: IN element to add
    @returns wyElem* node added
    */
	wyElem*	InsertAfter(wyElem* pafter, wyElem* pelem);

    /// inserts a node before the given node index
    /**
    @param pafter		: IN node index
    @param pelem		: IN element to add
    @returns wyElem* node added
    */
	wyElem*	InsertBefore(wyUInt32  nbefore, wyElem* pelem);

    /// Returns a pointer to the element whose index is passed as parameter
    /**
    @param index		: IN index to the element
    @returns wyElem* pointer
    */
	wyElem*	operator[] (wyUInt32 index);	

    /// returns the count of elements in the linked list
    /**
    @returns wyUInt32, number of elements in the list
    */
	wyUInt32    GetCount() {return m_elemcount;};
};

#endif
