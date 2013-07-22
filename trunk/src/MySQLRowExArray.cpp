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

/*********************************************

Author: Vishal P.R

*********************************************/

#include "MySQLRowExArray.h"

//constructor for rowex
MySQLRowEx::MySQLRowEx(MYSQL_ROW row, wyInt32 state, wyBool checked)
{
    m_row = row;
    m_state = state;
    m_ischecked = checked;
}

//destructor
MySQLRowEx::~MySQLRowEx()
{
    //if the row is mysql row, then free it
    if(IsMySQLRow() == wyFalse && m_row)
    {
        free(m_row);
    }
}

//helper function to check whether the row is mysql row
wyBool
MySQLRowEx::IsMySQLRow()
{
    return (m_state & ROW_MYSQL) ? wyTrue : wyFalse;
}

//function to check whether the row is new row
wyBool
MySQLRowEx::IsNewRow()
{
    return (m_state & ROW_NEW) ? wyTrue : wyFalse;
}

//---------------------------------------------------------------------------

//array constructor
MySQLRowExArray::MySQLRowExArray(ARRAYCALLBACK proc, void* pdata)
{
    m_array = NULL;
    m_arraylen = 0;
    m_callbackproc = proc;
    m_plongdata = pdata;
}

//destructor
MySQLRowExArray::~MySQLRowExArray()
{
    //resize the array to zero size
    Resize(0);
}

//function to get the number of elements in the array
wyInt32 
MySQLRowExArray::GetLength()
{
    return m_arraylen;
}

//set the callback proc
void 
MySQLRowExArray::SetCallBackProc(ARRAYCALLBACK proc)
{
    m_callbackproc = proc;
}

//set the long data
void
MySQLRowExArray::SetLongData(void* pdata)
{
    m_plongdata = pdata;
}

//helper function to notify an action 
void 
MySQLRowExArray::Notify(ArrayAction action, wyInt32 index, void* extra)
{
    //check whether we have valid callback
    if(m_callbackproc)
    {
        //notify
        m_callbackproc(action, m_plongdata, index, extra);
    }
}

//insert an item
wyBool
MySQLRowExArray::Insert(MySQLRowEx* prowex, wyInt32 index)
{
    wyInt32 i;
    
    //first resize the array to fit the new item
    Resize(m_arraylen + 1);

    //set the index of the item
    if(index < 0 || index >= m_arraylen)
    {
        index = m_arraylen - 1;
    }

    //now shift the elements
    for(i = m_arraylen - 1; i > index; --i)
    {
        m_array[i] = m_array[i - 1];
    }

    //insert the new element
    m_array[index] = prowex;

    //notify the action
    Notify(AA_INSERT, index);

    return wyTrue;
}

//function to update an item
wyBool
MySQLRowExArray::Update(wyInt32 index, MySQLRowEx* prowex, wyBool isnotify)
{
    MySQLRowEx* ptemp;

    //make sure we have a valid index
    if(index >= 0 && index < m_arraylen)
    {
        //get the old item
        ptemp = m_array[index];

        //update
        m_array[index] = prowex;

        //notify
        if(isnotify == wyTrue)
        {
            Notify(AA_UPDATE, index, ptemp);
        }

        return wyTrue;
    }

    return wyFalse;
}

//function to remove an item
MySQLRowEx*
MySQLRowExArray::Remove(wyInt32 index)
{
    MySQLRowEx* ptemp;
    wyInt32     i;

    //validate the index
    if(index < 0 || index >= m_arraylen)
    {
        return NULL;
    }

    //notify the action
    Notify(AA_REMOVE, index, m_array[index]);

    //get the item
    ptemp = m_array[index];

    //shift the items
    for(i = index + 1; i < m_arraylen; ++i)
    {
        m_array[i - 1] = m_array[i];
    }

    //reallocate the array
    Allocate(m_arraylen - 1);

    //return the element that is removed
    return ptemp;
}

//function to delete an item, this removes the item from the array and then deallocates the memory
wyBool
MySQLRowExArray::Delete(wyInt32 index)
{
    MySQLRowEx* ptemp;
    wyInt32     i;

    //validate the index
    if(index < 0 || index >= m_arraylen)
    {
        return wyFalse;
    }

    //notify the action
    Notify(AA_DELETE, index);

    //get the element
    ptemp = m_array[index];

    //shift the elements
    for(i = index + 1; i < m_arraylen; ++i)
    {
        m_array[i - 1] = m_array[i];
    }

    //reallocate the array
    Allocate(m_arraylen - 1);

    //delete the element
    delete ptemp;

    return wyTrue;
}

//function to resize the array
wyBool 
MySQLRowExArray::Resize(wyInt32 len, wyBool isdeleteextra)
{
    wyInt32     i;
    MySQLRowEx* ptemp;

    //validate the new length
    if(len < 0)
    {
        return wyFalse;
    }

    //if the new length is less than original length, we have to remove/delete the elements
    if(len < m_arraylen)
    {
        //remove the elements from end
        for(i = len; i < m_arraylen; ++i)
        {
            //get the current element
            ptemp = m_array[i];

            if(isdeleteextra == wyFalse)
            {
                //notify remove
                Notify(AA_REMOVE, i, ptemp);

                m_array[i] = NULL;
            }
            else
            {
                //notify delete
                Notify(AA_DELETE, i);

                m_array[i] = NULL;

                //free the item
                delete ptemp;
            }
        }
    }
    
    //reallocate
    Allocate(len);
    return wyTrue;
}

//function to allocate/reallocate the array
wyBool
MySQLRowExArray::Allocate(wyInt32 len)
{
    wyInt32 i;

    //if zero length means, we want to free the array
    if(!len)
    {
        //free the array
        if(m_array)
        {
            free(m_array);
            m_array = NULL;
        }
    }
    else
    {
        //either allocate or reallocate
        if(m_array)
        {
            m_array = (MySQLRowEx**)realloc(m_array, sizeof(MySQLRowEx*) * len);
        }
        else
        {
            m_array = (MySQLRowEx**)malloc(sizeof(MySQLRowEx*) * len);
        }
    }

    //fill NULLs to the newly allocated space
    for(i = m_arraylen; i < len; ++i)
    {
        m_array[i] = NULL;
    }

    //set the array length
    m_arraylen = len;

    //notify resize
    Notify(AA_RESIZE, 0);

    return wyTrue;
}

//index operator
MySQLRowEx* const
MySQLRowExArray::operator[](wyInt32 index)
{
    if(index < 0 || index >=  m_arraylen)
    {
        return NULL;
    }

    return m_array[index];
}

//get the row at the index given
MySQLRowEx* const
MySQLRowExArray::GetRowExAt(wyInt32 index)
{
    if(index < 0 || index >=  m_arraylen)
    {
        return NULL;
    }

    return m_array[index];
}

//overloaded assignment operator
const MySQLRowExArray& 
MySQLRowExArray::operator=(const MySQLRowExArray& operand)
{
    wyInt32 i;

    //first resize the array with the new length
    Resize(operand.m_arraylen);

    //copy the elemetns
    for(i = 0; i < operand.m_arraylen; ++i)
    {
        m_array[i] = operand.m_array[i];
    }

    //copy callback and long data
    m_plongdata = operand.m_plongdata;
    m_callbackproc = operand.m_callbackproc;

    return *this;
}

//function to get the row array
const MySQLRowEx** 
MySQLRowExArray::GetRowArray()
{
    return (const MySQLRowEx**)m_array;
}

//function to set the row array
void 
MySQLRowExArray::SetRowArray(const MySQLRowEx** arr, wyInt32 length, wyBool isnotify)
{
    wyInt32 i;

    //resize the array
    if(isnotify)
    {
        Resize(length);
    }
    else
    {
        Allocate(length);
    }

    //copy the elements
    for(i = 0; i < length; ++i)
    {
        m_array[i] = (MySQLRowEx*)arr[i];
    }
}

//function to set the row array
void 
MySQLRowExArray::UseRowArray(MySQLRowEx** arr, wyInt32 length)
{
    if(m_array)
    {
        free(m_array);
    }

    m_arraylen = length;
    m_array = arr;
}