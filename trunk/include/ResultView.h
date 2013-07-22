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

Author: Vishal P.R, Janani SriGuha

*********************************************/

#ifndef _RESULTVIEW_H_
#define _RESULTVIEW_H_

#include "DataView.h"

//Data for ResultView
class MySQLResultDataEx : public MySQLDataEx
{
    public:
        ///Constructor
        /**
        @param pmdi     : IN MDIWindow pointer
        @param query    : IN query executed
        @param len      : IN length of the query
        */
                        MySQLResultDataEx(MDIWindow* pmdi, const wyChar* query, wyUInt32 len = 0);

        ///Destructor
                        ~MySQLResultDataEx();

        ///Function to initialize the modifyable members in the class 
        /**
        @returns void
        */
        void            Initialize();

        ///Function to handle the view persistence for the query
        /**
        @param isset    : IN wyTrue if it should be saved, wyFalse if you want to load
        @returns ViewType enumeration
        */
        ViewType        HandleViewPersistence(wyBool isset);

        ///Function handles the limit persistance for the query
        /**
        @param isset    : IN wyTrue if it should be saved, wyFalse if you want to load
        @returns void
        */
        void            HandleLimitPersistence(wyBool isset);

        ///Function to set the query member of the class
        /**
        @param query    : IN query executed
        @param len      : IN length of the query
        */
        void            SetQuery(const wyChar* query, wyUInt32 len);

        ///Function to get the query
        /**
        @returns query
        */
        wyString&       GetQuery();

        ///Total time taken for the query
        wyInt64			m_totaltime;

        ///Execution time for the query
	    wyInt64			m_exectime;

        ///Is the query select query
	    wyBool			m_isselectquery;

        ///Index of the selected table in the combobox
        wyInt32         m_selectedtable;

        ///List of the tables in the result set
        List*           m_ptablelist;

        ///Does query contain limit clause
        wyBool          m_isquerylimited;

        //From MySQL Version 5.6.3 Explain and Explain Extended are available for INSERT, UPDATE, DELETE and REPLACE
        // This variable will identify whether to add profiling results in that case or not
        wyBool          m_isExplain;

        ///Formatted query by removing space and new line
        wyString        m_formattedquery;

        ///Is the result part of multiple result. like the output of CALL with multiple SELECTS
        wyBool          m_ispartofmultiresult;

    private:
        ///Helper function to calculate CRC, to save view persistence
        /**
        @returns void
        */
        void            CalculateQueryCRC();

        ///Query executed
        wyString		m_query;

        ///CRC for the query
        wyUInt32        m_crcquery;
};

//Class representing the table in the result set
class ResultTabTableElem : public wyElem
{
    public:
        ///Constructor
        /**
        @param db       : IN database name
        @param table    : IN table name
        */
                        ResultTabTableElem(wyString& db, wyString& table);

        ///Destructor
                        ~ResultTabTableElem();

        ///Database name
        wyString*       m_pdb;

        ///Table name
        wyString*       m_ptable;

        ///Is the db for the table original?
        wyBool          m_isorigdb;
};

//Class representing result view
class ResultView : public DataView
{
	public:
        ///Constuctor
        /**
        @param wnd              : IN MDI window pointer
        @param hwndparent       : IN parent window handle
        */
		                        ResultView(MDIWindow* wnd, HWND hwndparent);

        ///Destructor
		                        ~ResultView();

        ///Function to set the data
        /**
        @param data             : IN data pointer
        @returns void
        */
		void                    SetData(MySQLDataEx* data);

        ///Subclassed window proc for the addition toolbar created
        /**
		@param hwnd             : IN handle to the Table Data edit Window. 
	    @param message          : IN specifies the message. 
		@param wparam           : IN specifies additional message-specific information. 
		@param lparam           : IN specifies additional message-specific information. 
		@returns LRESULT
		*/
        static LRESULT CALLBACK ToolBarWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Change the toolbar button icon when execution starts
        /**
        @param isexecstart      : IN is execution starting?
        @param id               : IN button identifier
        @returns wyTrue if successful else wyFalse
        */
        wyBool                  ChangeToolButtonIcon(wyBool isexecstart, wyInt32 id);

        ///Reset all toolbar button images to its original state.
        /**
        @returns void
        */
        void                    ResetToolBarButtons();

        ///Functions to resize the window
        /**
        @returns void
        */
        void                    Resize();

        ///Function to create toolbars and additional controls
        /**
        @returns void
        */
        void                    CreateToolBar();

        ///Function to perform after query operations. Used only for refresh operation
        /**
        @param pexeinfo         : IN execution info
        @return void
        */
        void                    OnQueryFinish(LPEXEC_INFO pexeinfo);
        
        ///Refresh view. It executes the query
        /**
        @returns error status casted into integer
        */
        wyInt32                 RefreshDataView();

        ///The function initializes the required members and allocate a new object and copy the members to be freed. 
        /**
        @param pdata            : IN/OUT data
        @returns allocated data that should be deleted
        */
        MySQLDataEx*            ResetData(MySQLDataEx* pdata);

        ///Function to add tool buttons
        /**
        @returns void
        */
        void                    AddToolButtons();

        ///Handles the table combo box change
        /**
        @returns void
        */
        void                    OnTableComboChange();

        ///Function draws the query on info ribbon at the bottom
        /**
        @param hdc              : IN device context
        @param pdrawrect        : IN drawing rectangle
        @returns void
        */
        void                    DrawTableInfoRibbon(HDC hdc, RECT drawrect);

        ///Enable tool button
        /**
        @param isenable         : IN enable or disable
        @param id               : IN button identifier, if it is -1 then it will enable/disable all the buttons in the toolbar
        @returns wyTrue on success else wyFalse
        */
        wyBool                  EnableToolButton(wyBool isenable, wyInt32 id);

        ///Function to get index of the column from the field res
        /**
        @param col              : IN column index
        @returns index of the column on success else -1
        */
        wyInt32                 GetFieldResultIndexOfColumn(wyInt32 col);

        ///Function builds the table list from the result set
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  BuildTableList();

        ///Function sets the table combo items from the table list
        /**
        @returns void
        */
        void                    SetTablesCombo();

        //Function to set the table combo drop width
        /**
        @param index           : IN index of the item to be updated
        @returns void
        */
        void                    UpdateTableComboItem(wyInt32 index);

        ///Callback function for table combo box change, the function is called once any insert/update operation pending in the view is performed
        /**
        @param lpexeinfo        : IN execution info
        @returns 0
        */
        static wyInt32 CALLBACK TableComboChangeCallback(LPEXEC_INFO lpexeinfo);

        ///The last function to be called in sequance when the table combo changes
        /**
        @param te               : IN thread error status
        @returns void
        */
        void                    EndTableComboChange(ThreadExecStatus te);

        ///Function enables/disable limit check box based on data availability and whether the query has a limit clause
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                  EnableLimitCheckbox();

        ///Helper function to get the toolbar handle that contain the button id
        /**
        @param id               : IN button id
        @returns toolbar handle if successful else NULL
        */
        HWND                    GetToolBarFromID(wyInt32 id);

        ///Function enables the tool buttons based on various factors. 
        /**
        @returns void
        */
        void                    EnableToolButtons();

        ///Function to enable/disable any toolbuttons relevent for data modification. eg, duplicate row, add row, delete row, save changes etc
        /**
        @param isenable         : IN whether to enable or disable
        @returns void
        */
        void                    EnableDMLButtons(wyBool isenable);

        ///Function refreshes the table combo box
        /**
        @returns void
        */
        void                    RefreshTablesCombo();

        ///Helper function to select an item in the combo box
        /**
        @param i                : IN index of the item to be selected
        @returns void
        */
        void                    SelectTableComboItem(wyInt32 i);

        ///Function to show the help file relevent to result tab
        /**
        @returns void
        */
        void                    ShowHelpFile();

        ///Gets auto increment column index.
        /**
        @returns auto increment column index if exists, otherwise -1
        */
        wyInt32                 GetAutoIncrIndex();

        ///Function to export the data
        /**
        @returns void
        */
        void                    ExportData();

	protected:
        ///Table combo box
		HWND			        m_hwndtablecombo;

        ///Second toolbar
		HWND			        m_hwndtoolbar2;

        ///Data	
		MySQLResultDataEx*      m_mydata;

        ///Original toolbar proc for the second toolbar
        WNDPROC                 m_toolbarorigproc;
};

#endif