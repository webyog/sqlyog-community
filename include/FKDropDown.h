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

#ifndef _FK_DROPDOWN_H_
#define _FK_DROPDOWN_H_

#include "CommonHelper.h"
#include "htmlayout.h"


class DataView;
class FormView;

class FKDropDown
{
    public:
									 ///Parameterised constructor2
        /**
        @param pdvb                 : IN DataViewBase pointer
        @param pmyres               : IN pointer to the mysql result structure
        @param ptqt                 : IN pointer to the TabQueryTypes, should be NULL for Table Data
        @param params               : IN pointer to the behavior params if the call is from from view
        */                                    
                                    FKDropDown(DataView* pdv, BEHAVIOR_EVENT_PARAMS* params);

        ///Destructor
                                    ~FKDropDown();

        ///Function creates the dialog box
        /**
        @returns the dialog return value
        */
        wyInt32                     Create();

        ///Function creates the FK list
        /**
        @param ppfkinfo             : OUT pointer to pointer to the fk structure
        @param dbname               : IN database name
        @param tablename            : IN table name
        @param ptunnel              : IN tunnel pointer
        @param pcreatetable         : IN create table statement
        @returns total number of elements in the array, will be one more than valid number of elements
        */
        static wyInt32              CreateFKList(FKEYINFOPARAM** ppfkinfo, const wyChar* dbname, const wyChar* tablename, Tunnel* ptunnel, wyString* pcreatetable);

        ///Function deletes the fk list
        /**
        @param pfkinfo              : IN pointer to the fk list
        @param count                : IN total number of elements in the array
        */
        static void                 DeleteFKList(FKEYINFOPARAM** ppfkinfo, wyInt32* count);

        static wyBool               IsParentFromDiffDB(FKEYINFOPARAM* pfkinfo, wyInt32 count, wyInt32 index, wyInt32 col);

     
    private:
        ///Callback procedure for dialog box
        /**
        @param hwnd                 : IN window handle
        @param message              : IN window message
        @param wparam               : IN message parameter
        @param lparam               : IN message parameter
        @returns TRUE if the message is handled, else FALSE
        */
        static INT_PTR	CALLBACK    DlgProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Callback procedure for filter edit control
        /**
        @param hwnd                 : IN window handle
        @param message              : IN window message
        @param wparam               : IN message parameter
        @param lparam               : IN message parameter
        @returns non zero on success else zero
        */
        static LRESULT CALLBACK     SearchCtrlProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Function initializes the dialog box
        /**
        @param hwnd                 : IN handle to the dialog box
        @returns wyTrue on success else wyFalse
        */
        wyBool                      InitDialog(HWND hwnd);

        ///Function to handle WM_COMMAND
        /**
        @param wparam               : IN message parameter
        @param lparam               : IN message parameter
        @returns TRUE if handles, else FALSE
        */
        BOOL                        OnWMCommand(WPARAM wparam, LPARAM lparam);

        ///Function enable/disables the navigation controls
        /**
        @param set                  : IN enable/disable
        */
        void                        SetLimits(BOOL set);

        ///Function initializes the navigation controls
        /**
        @returns void
        */
        void                        InitializeNaviagationControls();

        ///Function creates the index, which is the core module 
        /**
        @returns wyTrue if tables are from diff databases, else wyFalse
        */
        wyBool                      CreateIndex();

        ///Function creates the grid
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                      CreateGrid();

        ///Callback procedure for grid notifications
        /**
        @param hwnd                 : IN window handle
        @param message              : IN window message
        @param wparam               : IN message parameter
        @param lparam               : IN message parameter
        @returns non zero on success else zero
        */
        static LRESULT CALLBACK     GridWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

        ///Function executes the query
        /**
        @param isignorefilter       : IN whether to ignore the filter while forming the query
        @returns wyTrue on success, else wyFalse
        */
        wyBool                      ExecuteQuery(wyBool isignorefilter = wyFalse);

        ///Function forms the query
        /**
        @param query                : OUT the formed query
        @param isignorefilter       : IN whether to ignore the filter while forming the query
        @returns void
        */
        void                        FormQuery(wyString& query, wyBool isignorefilter = wyFalse);

        ///Function creates the columns in the grid
        wyInt32                     CreateColumns();

        ///The function responsible for supplying data for grid to display
        /**
        @param wparam               : IN parameter for the notification message
        @returns void
        */
        void                        OnGVDispInfo(WPARAM wparam);

        ///Function initializes the column dropdown fro filter
        /**
        @returns void
        */
        void                        InitializeColumnsDropDown();

        ///Function creates the filter clause by examining the filter columns
        /**
        @params filter              : OUT filter clause
        @returns void
        */
        void                        GetColumnsToFilter(wyString& filter);

        ///Function is called when we click on Next/Prev button
        /**
        @params hwnd                : IN handle to the window
        @returns void
        */
        void                        OnNextOrPrev(HWND hwnd);

        ///Function returns the start row
        /**
        @returns the start row for limit
        */
        wyInt64                     GetStartRow();

        ///Function returns the row count
        /**
        @returns the row count for limit
        */
        wyInt64                     GetRowCount();
        
        ///Function is called whenever the list item in column dropdown is changed
        /**
        @params lparam              : IN message parameter
        @returns void
        */
        void                        OnColumnListItemChange(LPARAM lparam);

        ///Helper function adds the specified relation to the index
        /**
        @param i                    : IN index of the relation to be added
        @returns void
        */
        void                        AddIndex(wyInt32 i);

        ///Function gets the table name for the query, this may contain joins also depending on the number of parent tables
        /*
        @returns the table name
        */
        wyString&                   GetTableName();

        ///Function is called to select the value from the grid
        /**
        @returns 0 on success else 1
        */
        wyInt32                     OnEnterKeyInGrid();

        ///Function get the limit clause
        /**
        @params limit               : OUT the limit clause for the query
        @returns void
        */
        void                        GetLimits(wyString& limit);

        ///Function gets the filter text entered
        /**
        @param text                 : OUT filter text
        @returns filter text
        */
        wyString&                   GetFilterText(wyString& text);

        ///Function gets and stores the control rectangles in a linked list
        /**
        @returns void
        */
        void                        GetCtrlRects();

        ///Function positions the controls appropriately
        /**
        @returns void
        */
        void                        PositionCtrls();

        ///Function is called when Windows ask fo size information
        /**
        @params lparam              : IN message parameter
        @returns void
        */
        void                        OnWMSizeInfo(LPARAM lparam);

        ///Function is called on grid view button click
        /**
        @returns void
        */
        void                        OnGVButtonClick();

        ///The thread function which executes the query
        /**
        @params lpparam             : IN arguments for the function
        @returns the return value of thread
        */
        static unsigned __stdcall	QueryThread(LPVOID lpparam);     

        ///Function is called after execution, which does the cleanup
        /**
        @returns zero on success else non zero
        */
        wyInt32                     PostExecution();

        ///Function gets/sets thread stop status
        /**
        @params value               : IN if value = -1, then function returns the thread stop status, else sets the value to thread stop status and returns the value
        @returns thread stop status
        */
        wyInt32                     ThreadStopStatus(wyInt32 value = -1);

        ///Callback procedure for grip static control
        /**
        @param hwnd                 : IN window handle
        @param message              : IN window message
        @param wparam               : IN message parameter
        @param lparam               : IN message parameter
        @returns non zero on success else zero
        */
        static LRESULT CALLBACK     GripProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Callback procedure for refresh button
        /**
        @param hwnd                 : IN window handle
        @param message              : IN window message
        @param wparam               : IN message parameter
        @param lparam               : IN message parameter
        @returns non zero on success else zero
        */
        static LRESULT CALLBACK     ExecButtonProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Function is called when the splitter in custom grid moves which saves the column width
        /**
        @params column              : IN the column index
        @returns void
        */
        void                        OnSplitterMove(wyInt32 column);

        ///Function gets/sets the state of the option "Filter on values already entered"
        /**
        @params value               : IN < 0 to read the value, else to set the value
        @returns the value
        */
        wyInt32                     FilterOnValuesEntered(wyInt32 value);

        ///Function positions the window relative to the cell/filed in the grid/form view
        /**
        @params prect               : IN window rectangle
        @returns void
        */
        void                        PositionWindow(RECT* prect);

        ///Function gets the database name from the result set
        /**
        @param col                  : IN column name
        @returns dbname
        */
        const wyChar*               GetDbName(wyInt32 col);

        ///FK info and count
        FKEYINFOPARAM*              m_pfkinfo;
        wyInt32                     m_fkcount;

        ///FK column name
        wyString                    m_fkcolumnname;

        ///Handle to dialog, parent grid and grid
        HWND                        m_hwnd;
        HWND                        m_hwndgrid;
        HWND                        m_hwndparentgrid;

        ///Selected column and row index in the parent grid/form
        wyInt32                     m_column;
        wyInt32                     m_row;

        ///Index and index count for respecive fk cionstraints in m_pfkinfo
        wyInt32*                    m_index;
        wyInt32                     m_indexcount;

        ///Result set pointer
        MYSQL_RES*                  m_myres;

        ///MySQL row pointers for the result set, used to access the rows directly
        MYSQL_ROW*                  m_myrows;

        ///MDI window pointer
        MDIWindow*                  m_pmdi;
        
        ///Handle to the columns dropdown
        HWND                        m_hwndcolumns;

        ///Sort column and sort order
        wyInt32                     m_sortcol;
        wyInt32                     m_sortorder;

        ///Whether a notification is sent because of an automated action
        wyBool                      m_isautomated;

        ///Table name, can be a join of multiple tables
        wyString                    m_table;

        ///Column index of the result set curresponding to the fk column
        wyInt32                     m_item;

        ///Dynamic string for database and table names
        wyString*                   m_pdb;
        wyString*                   m_ptable;

        ///Selected column and row
        wyInt32                     m_selcol;
        wyInt32                     m_selrow;

        ///Event parameter for action in form view
        BEHAVIOR_EVENT_PARAMS*      m_params;

        ///Control list used for resizing
        List                        m_controllist;

        ///Window rectangle
        RECT                        m_wndrect;

        ///Dialog rectangle
        RECT                        m_dlgrect;

        ///Shared resource to check whether the thread is stopped
        wyInt32                     m_threadstopstatus;

        ///Previously selected row and column
        wyInt32                     m_prevselcol;
        wyInt32                     m_prevselrow;

        ///Critical section variable
        CRITICAL_SECTION            m_cs;

        ///Filter condition
        wyString                    m_filter;

        ///Last checked item in the columns dropdown. used for Shift-Click
        wyInt32                     m_lastcheckeditem;
        
        ///Composite filter if any
        wyString                    m_compositefilter;

        ///The button text to be shown in the blob columns
        wyString                    m_buttontext;

        ///Text for the column
        wyString                    m_text;

        ///Whether the initialization is completed
        wyBool                      m_isinitcompleted;

        ///Whether to filter on values already entered in the other columns in the grid
        wyInt32                     m_filteronvaluesentered;

        ///MySQL res in the parent grid
        MYSQL_RES*                  m_parentmyres;

        ///The grid notification function ptr
        WNDPROC                     m_pfuncptr;

		//DataView Pointer			
		DataView*					m_pdv;

        ///Temperory pointers fo database and table names
        wyString*                   m_pdbtemp;
        wyString*                   m_ptabletemp;

		
};

///Public function to invoke FK lookup
/**
@params pdvb    : IN DataViewBase pointer
@params pmyres  : IN MySQL result set pointer of the parent grid
@params ptqt    : IN TabQueryTypes pointer
@params params  : IN BEHAVIOR_EVENT_PARAMS of parent form view
@returns the dialog return value
*/
wyInt32         FKLookupAndInsert(DataView* pdvb, BEHAVIOR_EVENT_PARAMS* params);

#endif