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


#include "List.h"

List::List() 
{ 
#ifdef  _WIN32
	m_first=NULL;m_last=NULL; 
#else
	m_first=0;m_last=0; 
#endif
	m_elemcount=0;
};
    
/// Destructor
/**
Free up all the allocated resources
*/
List::~List()
{
    wyElem* pelem;

    for(pelem = this->GetFirst(); pelem; pelem = this->GetFirst())
    {
        this->Remove(pelem);
        delete pelem;
    }
}

/* Inserts a node in the last position */
wyElem*
List::Insert(wyElem* pelem)
{
	// If there are no nodes then make it the first node.
	if(!m_first){
		m_first=pelem;

#ifdef _WIN32
		pelem->m_prev = NULL;
#else
		pelem->m_prev = 0;
#endif

	}

	// If there is no entry as the last then we add it differently.
	if(m_last){
		m_last->m_next = pelem;
		pelem->m_prev = m_last;

#ifdef  _WIN32
		pelem->m_next = NULL;
#else
		pelem->m_next = 0;
#endif

		m_last = pelem;
	} else {
		m_last=pelem;
#ifdef  _WIN32
		pelem->m_next = NULL;
#else
		pelem->m_next = 0;
#endif

	}

	m_elemcount++;

	return pelem;
}


/* Removes a node and make the necessary changes */
wyElem*
List::Remove(wyElem *pelem)
{
	wyElem*	prevelem;
	wyElem*	nextelem;

	prevelem = pelem->m_prev;
	nextelem = pelem->m_next;

	// make the previous point to the next.
	if(prevelem)
		prevelem->m_next = nextelem;
	else
		m_first = nextelem;

	// make the next element the previous elements next.
	if(nextelem)
		nextelem->m_prev = prevelem;

	// reduce the element count.
	m_elemcount--;

	// change the last element.
	if(!nextelem)
		m_last = prevelem;
	
	if(!m_elemcount){
#ifdef  _WIN32
		m_first=NULL;m_last=NULL;
#else
		m_first=0;m_last=0;
#endif

	}

    return nextelem;
}

/* Inserts a Element node before the element whose pointer is passed as parameter */
wyElem*
List::InsertBefore(wyElem* pBefore, wyElem* pelem)
{
		wyElem*	prevelem=pBefore->m_prev;

        if(!prevelem)
        {
            pelem->m_next = pBefore;
            pBefore->m_prev = pelem;
            m_first = pelem;
#ifdef  _WIN32
            pelem->m_prev = NULL;
#else
			pelem->m_prev = 0;
#endif
        }
        else
        {
	        prevelem->m_next = pelem;
	        pBefore->m_prev = pelem;
	        pelem->m_next = pBefore;
	        pelem->m_prev = prevelem;
        }
	    
	    m_elemcount++;

	return pelem;
}

/* Inserts a node after the node whose pointer is passed as the parameter */
wyElem*
List::InsertAfter(wyElem* pAfter, wyElem* pelem)
{

	pelem->m_next		= pAfter->m_next;
	
	pAfter->m_next		= pelem;
	pelem->m_prev		= pAfter;

	m_elemcount++;

	if(!pelem->m_next)
		m_last = pelem;
    else
    {
        pelem->m_next->m_prev = pelem;
    }

	return pelem;

}

/* Inserts a node at the nTH position */
wyElem*
List::InsertBefore(wyUInt32 nbefore, wyElem* pelem)
{
	wyUInt32 i;
	wyElem*	elempointer;
	
	VERIFY(m_first);

	// if the user wants to add in the first position
	if(nbefore == 0){
		m_first->m_prev = pelem;
		pelem->m_next = m_first->m_next;
		m_first = pelem;
		return pelem;
	}

	// If the user wants to add at the last.
	if(nbefore>(m_elemcount-1))
		return Insert(pelem);

	// traverse thru the list and add it.
	i=0; elempointer=m_first;
	while(i<nbefore)
	{
		elempointer=elempointer->m_next;
		i++;
	}
	//for(i=0, elempointer=m_first; i<nbefore ; i++, elempointer=elempointer->m_next);

	return InsertBefore(elempointer, pelem);

}

/* Returns a pointer to the element whose index is passed as parameter */
wyElem*
List::operator [](wyUInt32 index)
{
    wyUInt32 i;
	wyElem   *elempointer;

	// Some boundary condition.
	i=0; elempointer=m_first;
	while(i<index)
	{
		elempointer=elempointer->m_next;
		i++;
	}

	return elempointer;
}

/* Removes a node based on the index */
wyElem*
List::Remove(wyUInt32 index)
{
    wyUInt32 i;
	wyElem *elempointer;

	// Some boundary condition.
	if(!index)
		return Remove(m_first);

	i=0; elempointer=m_first;
	while(i<index)
	{
		elempointer=elempointer->m_next;
		i++;
	}

	return Remove(elempointer);
}
