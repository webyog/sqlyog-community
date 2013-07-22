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

/*********************************************

Author: Vishal P.R

*********************************************/
#ifndef _MYSQL_ROWEX_ARRAY_H_
#define _MYSQL_ROWEX_ARRAY_H_

#include "CommonHelper.h"

//Various row states
#define ROW_DEFAULT     0
#define ROW_NEW         1
#define ROW_MYSQL       2
#define ROW_DUPLICATE   4

//Enumeration for various array actions
enum ArrayAction{AA_INSERT, AA_REMOVE, AA_DELETE, AA_UPDATE, AA_RESIZE};

//Callback function for array
typedef void (CALLBACK* ARRAYCALLBACK)(ArrayAction, void*, wyInt32, void*);

//Class represents a row in the data grid
class MySQLRowEx
{
    public:
        ///Constructor
        /**
        @param row          : IN mysql row
        @param state        : IN row state
        @param checked      : IN is row checked?
        */
                            MySQLRowEx(MYSQL_ROW row = NULL, wyInt32 state = ROW_NEW, wyBool checked = wyFalse);

        ///Destructor
                            ~MySQLRowEx();

        ///Function checks whether the row is a mysql returned row
        /**
        @returns wyTrue if it is a mysql row, else wyFalse
        */
        wyBool              IsMySQLRow();

        ///Function checks whether the row is new
        /**
        @returns wyTrue if it is a new row, else wyFalse
        */
        wyBool              IsNewRow();

        ///Actual row
        MYSQL_ROW           m_row;

        ///Row state
	    wyInt32             m_state;

        ///Whether the row is checked
	    wyBool              m_ischecked;
};

typedef MySQLRowEx MYSQL_ROWEX, *PMYSQL_ROWEX;

///Class represents the array implmentation for MySQLRowEx pointers
class MySQLRowExArray
{
    public:
        ///Constructor
        /**
        @param proc             : IN callback procedure
        @param pdata            : IN long data
        */
                                MySQLRowExArray(ARRAYCALLBACK proc = NULL, void* pdata = NULL);

        ///Destructor
                                ~MySQLRowExArray();

        ///Function to set the row array
        /**
        @param arr              : IN array of MySQLRowEx pointers
        @param length           : IN number of elements in the array
        @param isnotify         : IN whether to notify for any action caused by this
        */
        void                    SetRowArray(const MySQLRowEx** arr, wyInt32 length, wyBool isnotify = wyTrue);

        ///Function to set the passed row array as the row array. It frees any rowarray already in use the supplied array
        /**
        @param arr              : IN array of MySQLRowEx pointers
        @param length           : IN number of elements in the array
        */
        void                    UseRowArray(MySQLRowEx** arr, wyInt32 length);

        ///Overload index operator for convenience 
        /**
        @param index            : IN index of the item
        @returns the item
        */
        MySQLRowEx* const       operator[](wyInt32 index);

        ///Function to get the item at index
        /**
        @param index            : IN index of the item
        @returns the item
        */
        MySQLRowEx* const       GetRowExAt(wyInt32 index);

        ///Function to set the callback proc
        /**
        @param proc             : IN callback procedure
        @returns void
        */
        void                    SetCallBackProc(ARRAYCALLBACK proc);

        ///Function to set the long data
        /**
        @param pdata            : IN long data
        @returns void
        */
        void                    SetLongData(void* pdata);

        ///Update an item
        /**
        @param index            : IN index of the item to update
        @param prowex           : IN new item value
        @param isnotify         : IN whether to notify the action
        @returns wyTrue on success
        */
        wyBool                  Update(wyInt32 index, MySQLRowEx* prowex, wyBool isnotify = wyTrue);

        ///Insert an item
        /**
        @param prowex           : IN item
        @param index            : IN index where the item should be inserted. If index is > length of the array or index = -1, then item will be inserted at the end of the array
        @returns wyTrue onsuccess else wyFalse
        */
        wyBool                  Insert(MySQLRowEx* prowex, wyInt32 index = -1);

        ///Delete an item
        /**
        @param index            : IN index of the item to be removed and freed from the array
        @returns wyTrue on success else wyFalse
        */
        wyBool                  Delete(wyInt32 index);

        ///Remove an item from the array
        /**
        @param index            : IN index of the item to be removed
        @returns the item removed 
        */
        MySQLRowEx*             Remove(wyInt32 index);

        ///Get the number of elements in the array
        /**
        @returns the length of the array
        */
        wyInt32                 GetLength();

        ///Resize the array
        /**
        @param len              : IN new length
        @param isdeleteextra    : IN whether to remove and delete or just remove the elements not fitting in the new array size
        @returns wyTrue on success else wyFalse
        */
        wyBool                  Resize(wyInt32 len, wyBool isdeleteextra = wyTrue);

        ///Get the pointer to the array of pointers
        /**
        @returns the array of pointers 
        */
        const MySQLRowEx**      GetRowArray();

    private:
        ///Overloaded assignment operator.
        /**
        @params operand         : IN operand
        @returns reference to *this
        */
        const MySQLRowExArray&  operator=(const MySQLRowExArray& operand);

        ///Helper function to notify any array operations
        /**
        @params action          : IN array action
        @params index           : IN index of the item relevent to the action
        @params extra           : IN extra info if any
        @returns void
        */
        void                    Notify(ArrayAction action, wyInt32 index, void* extra = NULL);

        ///Function to allocate the required amount of memory for the array
        /**
        @params length          : IN new length
        @returns wyTrue on success else wyFalse
        */
        wyBool                  Allocate(wyInt32 length);

        ///Number of elements in the array
        wyInt32                 m_arraylen;

        ///Array of pointers
        MySQLRowEx**            m_array;

        ///Callback proc
        ARRAYCALLBACK           m_callbackproc;

        ///Long data associated with array
        void*                   m_plongdata;
};

#endif