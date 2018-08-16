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

#ifndef _DATAVIEW_H_
#define _DATAVIEW_H_

#include "wyTheme.h"
#include "Global.h"
#include "CommonHelper.h"
#include "SortAndFilter.h"
#include "MySQLRowExArray.h"

class FKDropDown;
class CCustGrid;
class FormView;
class DataView;
class ExportCsv;
class SortAndFilter;
class FindAndReplace;

#define DATAVIEW_TOPPADDING         3

//Custom messages used to communicate between worker thread and UI thread
#define UM_THREADFINISHED           WM_USER + 204
#define UM_UPDATEWARNING	        WM_USER + 206
#define UM_DELETEWARNING	        WM_USER + 207
#define UM_MYSQLERROR		        WM_USER + 208
#define UM_CONFIRMREFRESH           WM_USER + 209
#define UM_ERRFLAGSTATE             WM_USER + 210
#define UM_CLEARDATA                WM_USER + 211
#define UM_STARTEXECUTION           WM_USER + 212
#define UM_ENDEXECUTION             WM_USER + 213
#define UM_SETREFRESHVALUES         WM_USER + 214
#define UM_CONFIRMDELETE            WM_USER + 216
#define UM_DELETEROW                WM_USER + 217
#define UM_CANCELCHANGES            WM_USER + 218
#define UM_GETCURSELROW             WM_USER + 219
#define UM_SETROWCOUNT              WM_USER + 221
#define UM_QUERYGENERATIONFAILED    WM_USER + 222
#define UM_DOCLIENTSIDESORT         WM_USER + 224
#define UM_DOCLIENTSIDEFILTER       WM_USER + 225

//Notification messages used to get the color info
#define NM_GETCOLORS                WM_USER + 220
#define NM_GETHYPERLINKCOLOR        WM_USER + 223

//Extra images
#define IMG_INDEX_STOP          0
#define IMG_INDEX_RESET_FILTER  1
#define IMG_INDEX_LAST          2

//Commands used in GetImageIndex function
#define GETIMG_COMMAND          1
#define GETIMG_INDEX            2
#define GETIMG_IMG              3

//Some predefines
#define SAVE_WARNING            _(L"Data modified but not saved")
#define	SHOW_WARNING            _(L"Show warnings")

//Default limit for data view
#define DEF_LIMIT               pGlobals->m_highlimitglobal

//Error flag states
#define EFS_MODIFIED            1
#define EFS_WARNING             2
#define EFS_NONE                0

//Constants controling refresh status for various views
#define TEXTVIEW_REFRESHED      1
#define FORMVIEW_REFRESHED      2
#define VIEW_REFRESHED          (TEXTVIEW_REFRESHED | FORMVIEW_REFRESHED)

//Structure for colorinfo NM_GETHYPERLINKCOLOR and NM_GETCOLORS notification
typedef struct tagNMDVCOLORINFO
{
    //Standard windows notification structure
    NMHDR       hdr;

    //Dual color used for forground and background
    DUALCOLOR   drawcolor;
} NMDVCOLORINFO, *LPNMDVCOLORINFO;

//Structure used to carry the data between cross thread messaging
typedef struct thread_message_param
{
    //Event object used to sync between GUI thread and worker thread. Call SetThreadMessageEvent with lParam in the message to signal it
    HANDLE  m_hevent;

    //Additional data for the message
    LPARAM  m_lparam;
} THREAD_MSG_PARAM, *LPTHREAD_MSG_PARAM;

//Enumeration for possible thread action
enum ThreadAction
{
    TA_NONE = 0,
	TA_INSERT,
	TA_UPDATE,
	TA_DELETE,
	TA_REFRESH,
    TA_CLEARDATA
};

//Enumeration for possible thread return status
enum ThreadExecStatus
{
    TE_SUCCESS = 0,
    TE_ERROR,
    TE_STOPPED,
    TE_CANCEL
};

//Enumeration for possible actions for ThreadAction
enum LastAction
{
    LA_NONE = 0,
    LA_NEXT,
    LA_PREV,
    LA_LIMITCLICK,
    LA_SORT,
    LA_FILTER,
    LA_CALLBACK,
    LA_ROWCHANGE
};

//Execution info
typedef struct execution_info
{
    //Current thread action
    ThreadAction        m_ta;

    //Last action that caused execution
    LastAction          m_la;

    //WPARAM for this execution
    WPARAM              m_wparam;

    //LPARAM for this execution
    LPARAM              m_lparam;

    //Execution return status
    ThreadExecStatus    m_ret;

    //DataView pointer
    DataView*           m_pdataview;
}EXEC_INFO, *LPEXEC_INFO;

//Satructure used to supply the data to worker thread function
typedef struct thread_param
{
    //Thread handle, should be NULL if it the same thread
	HANDLE          m_hthread;

    /**The thread should wait for this before it start executing to make sure that we have a vaild thread handle
    If the it is the same thread then this will be NULL
    **/
    HANDLE          m_hthreadstartevent;

    //DataView pointer
	DataView*       m_dataview;

    //Execution info structure
	EXEC_INFO	    m_execinfo;

    //Additional parameter if any
    void*           m_param;

    //Flag says whether the thread function should post messages before and after each operations
    wyBool          m_issendmsg;

    //Last focused window
    HWND            m_hwndfocus;
} THREAD_PARAM, *LPTHREAD_PARAM;

//Enumeration for display mode
enum ViewType
{
    DUMMY = -1,
    GRID = 0,
    FORM, 
    TEXT,
};

//Callback function for LA_CALLBACK action 
typedef wyInt32 (CALLBACK* LACALLBACK)(LPEXEC_INFO);

//Data class for DataView. Derive from this class if you want to customize something
class MySQLDataEx
{
    public:
        ///Parameterized constructor
        /**
        pmdi                : IN Active MDI window pointer
        */
                            MySQLDataEx(MDIWindow* pmdi);

        ///Virtual distructor. Has to be virtual to avoid memory leaks
        /**
        */
        virtual             ~MySQLDataEx();

        ///Get the number of saved rows count
        /**
        @returns number of saved rows
        */
        wyInt32             GetSavedRowCount();

/*Virtual functions--------------------------------------------------------------------------------------------------------------------------------------------------*/

        ///The function that initializes the modifyable members in the class.
        ///Implement this in the derived class and then call MySQLDataEx::Initialize() from the implementation to initialize the members in the base class
        /**
        @returns void
        */
        virtual void        Initialize();

        ///Function handles the view persistance for the data class. Derived class can implement this to override the behavior
        /**
        @param isset        : IN wyTrue if it should be saved, wyFalse if you want to load
        @returns ViewType enumeration
        */
        virtual ViewType    HandleViewPersistence(wyBool isset);

        ///Function handles the limit persistance for the data. Derived class can implement this to override the behavior
        /**
        @param isset        : IN wyTrue if it should be saved, wyFalse if you want to load
        @returns void
        */
        virtual void        HandleLimitPersistence(wyBool isset);

        ///Free allocated resources such as row array and warnings.
        /**
        @returns void
        */
        virtual void        FreeAllocatedResources();

        ///Free everything
        /**
        @returns void
        */
        virtual void        Free();

/*End of virtual functions-------------------------------------------------------------------------------------------------------------------------------------------*/
     
        //MDI windoi pointer
        MDIWindow*          m_pmdi;

        //Row array used as the index for MYSQL_ROW pointers
        MySQLRowExArray*    m_rowarray;

        //MYSQL result
        MYSQL_RES*          m_datares;

        //MYSQL result for SHOW KEYS
        MYSQL_RES*          m_keyres;

        //MYSQL result for SHODW FIELDS
        MYSQL_RES*          m_fieldres;

        //Stores the old row pointer whenever we start updaging
        MYSQL_ROWEX*        m_oldrow;

        //Updated row
        wyInt32			    m_modifiedrow;

        //First row in the limit clause
        wyInt32		        m_startrow;

        //Row count in the limit clause
        wyInt32		        m_limit;

        //Whether limit to be applied
        wyBool              m_islimit;

        //Currently selected row, valid only when setting/resetting/refreshing the data
        wyInt32		        m_selrow;

        //Currently selected column, valid only when setting/resetting/refreshing the data
        wyInt32		        m_selcol;

		//whether column is virtual or not
        wyInt32*		        m_colvirtual;

        //Active database
        wyString		    m_db;

        //Active table
        wyString		    m_table;

        //First row in the grid viewport, valid only when setting/resetting/refreshing the data
        wyInt32		        m_initrow;

        //First column in the grid viewport, valid only when setting/resetting/refreshing the data
        wyInt32		        m_initcol;

        //MYSQL result for any warning generated
        MYSQL_RES*          m_warningres;

        //Number of rows checked. Used to speed up select all check and other operations
        wyInt32             m_checkcount;

        //Auto increment column for the table
        wyInt32             m_autoinccol;

        //FK info array used in FKDropDown
        FKEYINFOPARAM*      m_pfkinfo;

        //Element count in m_pfkinfo array. It is 1+ the actual number of elements
        wyInt32             m_fkcount;

        //Current view mode
        ViewType            m_viewtype;

        //Last view mode
        wyInt32             m_lastview;

        //Last DML query executed. Used to show warning in warning dialog
        wyString            m_lastdmlquery;

        //Create table statement for the table
        wyString            m_createtablestmt;

        //Sort and filter pointer. DataView handles sorting and filtering in the base class itself. 
        //If you want sorting and filtering allocate this in the derived class. Refer SortFilter.h for details
        SortAndFilter*      m_psortfilter;

        //Scroll position in text view, used to maintain persistance between setting/resetting/refreshing data
        POINT               m_textviewscrollpos;

        //Selection start/end position in text view, used to maintain persistance between setting/resetting/refreshing data
        wyInt32             m_textviewselstartpos;
        wyInt32             m_textviewselendpos;

        //Whether the selection is rectangle or not
        wyBool              m_isselectionrect;

        //Whether the row array is populated
        wyBool              m_isrowarrayinitialized;
};

//Calss representing elements in execution stack
class ExecutionElem : public wyElem
{
    public:
        /**Parameterized constructor
        @param execinfo : Execution info
        */
        ExecutionElem(EXEC_INFO& execinfo)
        {
            m_execinfo = execinfo;
        }

        //Execution info
        EXEC_INFO m_execinfo;
};

//Class representing elements in List/Enum 
class EnumListElem : public wyElem
{
    public:
        /**Parameterized constructor
        @param str : Single item in enum/list
        */
        EnumListElem(wyString& str)
        {
            m_str.SetAs(str);
        }

        //Single item in enum/list
        wyString m_str;
};

// abstract interface to be used as a callback interface for DataView. Any clients can construct this with
// non null interface to DataView, and are responsible for implementing that interface which gives the complete
// live query used at that time. null implies they dont need that.
class IQueryBuilder
{
	public:
		virtual void GetQuery(wyString& query) = 0;
};

//Class represents the view. This is an abstact class. You need to derive your own class from this and implement the methods
class DataView
{
	public:
        //Fried classes where it need to access the private/protected members. Sorry for being lazy to implement the public functions to access them ;)
		friend class CalendarCtrl;
		friend class FKDropDown;
        friend class FormView;
	
        ///Parameterized constructor
        /**
        @param wnd              : IN MDI window pointer
        @param hwndparent       : IN handle to parent window
		@param queryBuilder		: IN IQueryBuilder interface supplied from derived class
        */
		                        DataView(MDIWindow* wnd, HWND hwndparent, IQueryBuilder* queryBuilder);

        ///Destrouctor. Has to be virtual to avoid memory leak
		virtual                 ~DataView();

/*Pure virtual functions---------------------------------------------------------------------------------------------------------------------------------------------*/
    
    public:
        ///Functions to resize the window. Derived class has to implement this
        /**
        @returns void
        */
        virtual void            Resize() = 0;
        
    protected:
        ///Create toolbars. Derived class should implement this function to add/customize toolbars
        /**
        @returns void
        */
        virtual void            CreateToolBar() = 0;

        ///Draw info ribbon. The derived class has to implement this function. 
        ///fg color, bg color and bg mode are all set before calling this function. The implementation has to just draw the text
        /**
        @param hdc              : IN device context
        @param pdrawrect        : IN drawing rectangle
        @returns void
        */
        virtual void            DrawTableInfoRibbon(HDC hdc, RECT pdrawrect) = 0;

        ///Refresh data view. Derived class has to implement this function
        /**
        @returns error status casted into integer
        */
        virtual wyInt32         RefreshDataView() = 0;

        ///Derived class has to implement this. 
        ///The function should initialize the required members and allocate a new object and copy the members to be freed. 
        ///Make sure you have proper assignment operator overloaded for member objects so that you dont end up with shadow copy.
        /**
        @param pdata            : IN/OUT data. This can be typecasted into derived version of MySQLDataEx
        @returns allocated data that should be deleted. Caller can typecast it into derived version of MySQLDataEx
        */
        virtual MySQLDataEx*    ResetData(MySQLDataEx* pdata) = 0;

        ///Function to get index of the column from the field res. Derived class has to implement this
        /**
        @param col              : IN column index
        @returns index of the column on success else -1
        */
        virtual wyInt32         GetFieldResultIndexOfColumn(wyInt32 col) = 0;

        ///Reset all toolbar button images to its original state. Derived class has to implement this
        /**
        @returns void
        */
        virtual void            ResetToolBarButtons() = 0;

        ///Gets auto increment column index. Derived class has to implement this
        /**
        @returns auto increment column index if exists, otherwise -1
        */     
        virtual wyInt32         GetAutoIncrIndex() = 0;

/*End of pure virtual functions--------------------------------------------------------------------------------------------------------------------------------------*/


/*Virtual functions--------------------------------------------------------------------------------------------------------------------------------------------------*/

    public:
        ///Public function to set the data. Implement this in the derived class to set derived data pointers and then call DataView::SetData to set data pointer in base class
        ///Warning** If you dont call DataView::SetData from the derived class implementation then it will fail to function properly
        /**
        @param data             : IN data to be set
        @returns void
        */
        virtual void            SetData(MySQLDataEx* data);

        ///Function to export the data. Derived class can implement this to add custom logic to the export function
        /**
        @returns void
        */
        virtual void            ExportData();

    protected:
        ///Function to perform after query operations. Used only for refresh operation
        ///Derived class can implment their own version to initialize members and all and can call DataView::OnQueryFinish
        /**
        @param pexeinfo         : IN execution info
        @return void
        */
        virtual void            OnQueryFinish(LPEXEC_INFO pexeinfo);

        ///Change the toolbar button icon when execution starts
        ///Derived class can implement this function to update button image toolbars defined in that
        /**
        @param isexecstart      : IN is execution starting?
        @param id               : IN button identifier
        @returns wyTrue if successful else wyFalse
        */
        virtual wyBool          ChangeToolButtonIcon(wyBool isexecstart, wyInt32 id);

        ///Function creates the padding window to give some space above toolbar. Derived class can implment this to stop refresh toolbar being created
        /**
        @returns void
        */
        virtual void            CreatePaddingWindow();

        ///Function creates the refresh toolbar. Derived class can implment this to stop refresh toolbar being created
        /**
        @returns void
        */
        virtual void            CreateRefreshBar();

        ///Enable tool button. Derived class can implement this but call the base class version to avoid code duplication
        /**
        @param isenable         : IN enable or disable
        @param id               : IN button identifier, if it is -1 then it will enable/disable all the buttons in the toolbar
        @returns wyTrue on success else wyFalse
        */
        virtual wyBool          EnableToolButton(wyBool isenable, wyInt32 id = -1);


        ///Function enables/disable limit check box based on data availability. Derived class can implment this to provide its own logic to enable/disable it
        /**
        @returns wyTrue on success else wyFalse
        */
        virtual wyBool          EnableLimitCheckbox();

        ///Helper function to get the toolbar handle that contain the button id
        ///Implment this in the derived class if it has multiple toolbars
        /**
        @param id               : IN button id
        @returns toolbar handle if successful else NULL
        */
        virtual HWND            GetToolBarFromID(wyInt32 id);

        ///Function to handle WM_COMMAND. Derived class can implement this to handle additional commands
        /**
        @param wparam           : IN message parameter
        @param lparam           : IN message parameter
        @returns void
        */
        virtual void            OnWMCommand(WPARAM wparam, LPARAM lparam);

        ///Function gets the database associated with a column in the result set. Derived class can overrride this
        /**
        @param db               : OUT database name
        @param col              : IN  column index
        @returns wyTrue on success else wyFalse
        */
        virtual wyBool          GetDBName(wyString& db, wyInt32 col);

        ///Function gets the original table name associated with a column in the result set. Derived class can overrride this
        /**
        @param table            : OUT table name
        @param col              : IN  column index
        @returns wyTrue on success else wyFalse
        */
        virtual wyBool          GetTableName(wyString& table, wyInt32 col);

        ///Function gets the original column name associated with a column in the result set. Derived class can overrride this
        /**
        @param column           : OUT column name
        @param col              : IN  column index
        @returns wyTrue on success else wyFalse
        */
        virtual wyBool          GetColumnName(wyString& column, wyInt32 col);

        ///Delete a row
        /**
        @param row              : IN     row index to be deleted
        @param response         : IN/OUT action to be taken if there are duplicates. If it is -1, it will check, prompt and store the response. 
                                         Did this to avoid prompting multiple times when you are deleting multiple rows
        @returns error status casted to integer
        */
        virtual wyInt32         DeleteRow(wyInt32 row, wyInt32& response);

        ///Function enables the tool buttons based on various factors. 
        //Derived class can implement this to support additional toolbars/buttons but it is advised to call the base class version to avoid code duplication
        /**
        @returns void
        */
        virtual void            EnableToolButtons();

        ///Notification handler before execution starts for a thread action
        ///Derived class can implement this to perfomr additional operation. You can call the base class version to avoid code duplication
        /**
        @param ta               : IN thread action for which the execution started
        @param ptp              : IN thread parameter
        @returns void
        */
        virtual void            OnExecutionStart(ThreadAction ta, LPTHREAD_PARAM ptp);

        ///Notification handler after execution for a thread action
        ///Derived class can implement this to perfomr additional operation. You can call the base class version to avoid code duplication
        /**
        @param ta               : IN thread action for which the execution completed
        @param ptp              : IN thread parameter
        @returns void
        */
        virtual void            OnExecutionEnd(ThreadAction ta, LPTHREAD_PARAM ptp);

        ///function supplies the tooltip for toolbar. Derived class can implement this to support additional toolbars/toolbuttons
        /**
        @param lpnmtt           : IN/OUT the tooltip notification structure
        @returns wyTrue if handled, else wyFalse
        */
        virtual wyBool          OnToolTipInfo(LPNMTTDISPINFO lpnmtt);

        ///Handles filter on context menu
        /**
        @param id               : IN menu item identifier
        @returns void
        */
        virtual void            OnRightClickFilter(wyInt32 id);

        ///Function gets the banner text. The text is used to draw the text if there is no data associated with the view. 
        ///Derived class can implement this to supply its own text
        /**
        @param bannertext       : OUT text to be drawn
        @returns void
        */
        virtual void            GetBanner(wyString& bannertext);

        ///Function to show help file. Derived class can implment this to show a more relevent help file
        /**
        @returns void
        */
        virtual void            ShowHelpFile();

/*End of virtual functions-------------------------------------------------------------------------------------------------------------------------------------------*/
        
    public:
        ///Function creates the view
        /**
        @returns void
        */
		void                    Create();
		
        ///Public function to get the data
        /**
        @return data pointer, can be NULL
        */
        MySQLDataEx*            GetData();
		
        ///Function shows the windows based on the disp mode selected
        /**
        @param show             : IN SW_SHOW or SW_HID. Anything apart from SW_HIDE will be considered as SW_SHOW
        @returns void
        */
        void                    ShowAll(wyInt32 show);
        
        ///Public function to perform a thread action
        /**
        @param ta               : IN thread action to perform
        @param issendmsg        : IN whether to send message before and after each operation   
        @param spawnthread      : IN whether to spawn a new thread
        @param la               : IN last action associated with thread action
        @returns unsigned casted ThreadExecStatus
        */
        wyUInt32                Execute(ThreadAction ta, wyBool issendmsg, wyBool spawnthread, LastAction la = LA_NONE);

        ///Public function set the parent window handle of the view
        /**
        @param hwndparent       : IN handle to the new paraent window, does nothing if hwndparent is NULL
        @returns parent window handle
        */
        HWND                    SetParentWindow(HWND hwndparent);

        ///Function enables/disables the toolbar
        /**
        @param isenable         : IN enable/disable
        @returns void
        */
        void                    EnableToolbar(wyBool isenable);

        ///Frame window procedure
        /**
        @param hwnd             : IN window handle
        @param message          : IN window message
        @param wparam           : IN message parameter
        @param lparam           : IN message parameter
        @returns LRESULT for the message
        */
        static LRESULT CALLBACK FrameWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Static function to calculate the ribbon height
        /**
        @param hwnd             : IN window handle
        @returns height of the window
        */
        static wyInt32          CalculateRibbonHeight(HWND hwnd);

        ///Function returns the active window handle for the disp mode selected
        /**
        @returns handle to the window if data is not NULL, else NULL
        */
        HWND                    GetActiveDispWindow();

        ///Function switch the view to the mode given
        /**
        @param toview           : IN view mode to switch to
        @returns void
        */
        void                    SwitchView(ViewType toview = DUMMY);

        ///Function sets the buffered drawing flag on/off.
        /**
        @param isbuffered       : IN is buffered or not
        @returns void
        */
        void                    SetBufferedDrawing(wyBool isbuffered);

        ///Function shows the text view
        /**
        @returns void
        */
        void                    ShowTextView();

        ///Function process the accelerator to switch to form mode
        /**
        @return void
        */
        void                    AccelaratorSwitchForm();

        ///Function applies the changes in the grid, baseically it calls Execute. insert/update
        /**
        @param la               : IN last action happened
        @param wparam           : IN wparam associated with the action
        @param lparam           : IN lparam associated with the action
        @returns error status
        */
        wyInt32                 ApplyChanges(LastAction la, WPARAM wparam, LPARAM lparam);


        ///Function sets the fonts
        /*
        @returns void
        */
        void                    SetAllFonts();

        ///Function sets the text view color
        /**
        @returns void
        */
        void                    SetColor();

        ///Atomic function to check the thread stop status
        /**
        @param stop             : IN 0 = set status to start, 1 = set status to stop, -1 = retreive the status
        @returns thread stop status
        */
        wyInt32		            ThreadStopStatus(wyInt32 stop = -1);

#ifndef COMMUNITY
        ///Form view pointer
        FormView*               m_formview;
#endif
        ///Find an replace
        FindAndReplace          *m_findreplace;

        ///Handle to the last focused window
        HWND                    m_lastfocus;

    protected:
        ///Function to show the window wrt the active disp mode. It does all error checking before showing the window
        /**
        @param show             : IN SW_SHOW to show window, SW_HIDE to hide window
        @returns wyTrue on success else wyFalse
        */
        wyBool                  ShowDataWindow(wyInt32 show);

        ///Overloaded version of Execute
        /**
        @param lpexecinfo       : IN EXEC_INFO pointer
        @param issendmsg        : IN whether to send message or not before and after each operation
        @param spawnthread      : IN whether to spawn a new thread
        @returns exec status casted into unsigned int
        */
        wyUInt32                Execute(LPEXEC_INFO lpexecinfo, wyBool issendmsg, wyBool spawnthread);

        ///Function sets database and table names
        /**
        @param db               : IN database name
        @param table            : IN table name
        @returns void
        */
        void                    SetDbTable(const wyChar* db, const wyChar* table);

        ///Function shows the context menu for grid/form view
        /**
        @param row              : IN row where message happened
        @param col              : IN col where message happened
        @param pt               : IN point in screen cordinates where the message happened
        @returns void
        */
        void                    ShowContextMenu(wyInt32 row, wyInt32 col, LPPOINT pt);

        ///Function saves the current state of all the windows associated
        /**
        @returns void
        */
        void                    SaveViewState();

        ///Function restores the view state saved by SaveViewState
        /**
        @param lpexeinfo        : IN exec info, used to identify client side filter
        @returns void
        */
        void                    SetViewState(LPEXEC_INFO lpexeinfo = NULL);

        ///Function saves the column width whenever grid splliter moves. It also resets the formview refresh status so that the field width will be adjusted 
        /**
        @param col              : IN column where the splitter moved
        @returns void
        */
        void                    OnSplitterMove(wyInt32 col);

		void					OnColNameMouseHover(WPARAM wparam, wyString&);

		void					ResetDataViewTooltip();
		////void				ResettingTooltip(WPARAM wparam,LPARAM lparam);
        ///Function creates the outer frame window 
        /**
        @returns void
        */
		void                    CreateFrame();

        ///Function creates the grid window 
        /**
        @returns void
        */
		void                    CreateGrid();

        ///Function creates the text window 
        /**
        @returns void
        */
		void                    CreateText();

        ///Set grid font
        /**
        @returns void
        */
		void                    SetGridFont();

        ///Function creates form view
        /**
        @returns void
        */
		void                    CreateForm();
		
        ///Function creates the warning windows
        /**
        @returns void
        */
        void                    CreateWarnings();

        ///Append sort info to the query to be executed
        /**
        @param query            : IN query string to which the sort info to be appended
        @returns void
        */
        void                    GetSortInfo(wyString& query);

        ///Append filter info to the query to be executed
        /**
        @param query            : IN query string to which the filter info to be appended
        @returns void
        */
        void                    GetFilterInfo(wyString& query);

        ///Append limit to the query to be executed, if any
        /**
        @param query            : IN query string to which the limit to be appended
        @returns void
        */
        void                    GetLimits(wyString &query);

        ///Function to wait for the thread to finish and close the thread handle
        /**
        @param wparam           : IN not used
        @param lparam           : IN thread handle
        @returns void
        */
		void                    OnUMThreadFinish(WPARAM wparam, LPARAM lparam);

        ///Function allocates the rowsex array
        /**
        @returns the error status
        */
        ThreadExecStatus        AllocateRowsExArray();

        ///Function handles next, and adjusts the limit appropriately
        /**
        @returns void
        */
        void                    HandleNext();

        ///Function handles previous, and adjusts the limit appropriately
        /**
        @returns void
        */
        void                    HandlePrevious();

        ///Function gets the table details, such as key res, field res etc
        ThreadExecStatus        GetTableDetails();

        ///Helper function to disable the context menu items
        /**
        @param hmenu            : IN menu handle
        @returns void
        */
        void                    DisableMenuItems(HMENU hmenu);

        ///Function enables filter menu items based on various factors
        /*
        @param hmenu            : IN menu handle
        @param row              : IN row where the context menu message happened
        @param col              : IN column where the context menu message happened
        @returns void
        */
        void                    SetFilterMenu(HMENU hmenu, wyInt32 row, wyInt32 col);

        ///Function sets the copy menu items based on various factors
        /*
        @param hmenu            : IN menu handle
        @param row              : IN row where the context menu message happened
        @param col              : IN column where the context menu message happened
        @returns void
        */
        void                    SetCopyMenu(HMENU hmenu, wyInt32 row, wyInt32 col);

        ///Function resets the view
        /**
        @param isresetexecstack : IN whether to reset the execution stack
        @returns void
        */
        void                    ResetView(wyBool isresetexecstack = wyTrue);

        ///Destroy calandar control
        /**
        @returns void
        */
        void                    DestroyCalandarControl();

        ///Function gets the image index on a toolbar button
        /**
        @param id               : IN  button identifier
        @param pisorigimage     : OUT whether the current image index is the original image index set to the button
        @returns image index
        */
        wyInt32                 GetButtonImage(wyInt32 id, wyBool* pisorigimage);

		///Gets the display data information
        /**
        @param wparam           : IN GVDISPINFO param
        @returns void
        */
        void                    OnGvnGetDispInfoData(WPARAM wparam);

	    ///Handles the blob values
        /**
        @param wparam           : IN WPARAM parameter, not used
        @param lparam           : IN tells whether the charecter key was down when the event triggered
        @returns wyTrue on success else wyFalse
        */
	    wyBool                  HandleBlobValue(WPARAM wparam, LPARAM lparam);

        ///Gets the display length information
        /**
        @param wparam           : IN GVDISPINFO parameter
        @returns 1 if length > 0 else 0
        */
        wyInt32                 OnGvnGetDispLenInfo(WPARAM wparam);
        
        ///Checks for a function
        /**
        @param funcname         : IN Function name
        @returns wyTrue if it is function name else wyFalse
        */
        wyBool                  CheckForFunction(const wyChar *funcname);
				
		///Window procedure for the various windows
		/**
		@param hwnd             : IN handle to the Table Data edit Window. 
	    @param message          : IN specifies the message. 
		@param wparam           : IN specifies additional message-specific information. 
		@param lparam           : IN specifies additional message-specific information. 
		@returns LRESULT
		*/
		static LRESULT CALLBACK EditWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
        static LRESULT CALLBACK RefreshToolWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
        static LRESULT CALLBACK ShowWarningWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK GridWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Function checks whether the column is bit column
        /**
        @param col              : IN column index
        @returns wyTrue if it is bit column else wyFalse
        */
		wyBool                  IsBin(wyInt32 col);

        ///Function checks whether the column is blob column
        /**
        @param col              : IN column index
        @returns wyTrue if it is blob column else wyFalse
        */
		wyBool			        IsBlob(wyInt32 col);

		wyBool			        IsJSON(wyInt32 col);
        ///Helper function to change the filter icon
        /**
        @returns void
        */
        void                    ChangeFilterIcon();

		///Stops query execution
        /**
        @param buttonid         : IN toolbar button identifier
        @returns wyTrue on success else wyFalse
        */
	    wyBool		            Stop(wyInt32 buttonid);

        ///Function that checks for any unsaved changes left in grid, if so it will tell whether it is insert or update
        ///The function can be invoked from a worker thread or the GUI thread
        /**
        @returns the action to be performed, whether it is TA_INSERT or TA_UPDATE
        */
        ThreadAction            CheckForModifications();

        ///Function sets/gets what the warning window at the bottom shows
        /**
        @param state            : IN can be EFS_NONE or EFS_MODIFIED or EFS_WARNING. If it is -1, then it retreives the current state
        @returns the flag state
        */
        wyInt32                 ErrorFlagState(wyInt32 state = -1);

        ///Give display information
        /**
        @param wparam           : IN GVDISPINFO parameter
	    @param lparam		    : IN value set to 1 if want to check for base2 display
        @returns 1 success else 0
        */
	    wyInt32                 OnGvnGetDispInfo(WPARAM wparam, LPARAM lparam = 0);

        ///Function gets the cell value
        /**
        @param row              : IN  row index
        @param                  : IN  column index
        @param len              : OUT length of the cell value
        @param fromorig         : IN  whether to get the value from original result or the updated result
        @param bitstr           : OUT string rep of the bit value
        */
	    wyChar*                 GetCellValue(wyInt32 row, wyInt32 col, wyUInt32 *len, wyBool fromorig, wyString* bitstr = NULL);

	    ///Starts the row change
        /**
        @param row              : IN row number
        @param col              : IN column number
        @param onkillfocus      : IN focused or not
        @returns TRUE if the change is allowed else FALSE
        */
        BOOL                    OnGvnBeginChangeRow(wyInt32 row, wyInt32 col, wyBool onkillfocus);

	    ///Handles editing in the grid
        /**
        @param row              : IN row index
        @param col              : IN col index
        @returns LRESULT
        */
        LRESULT                 OnGvnBeginLabelEdit(wyInt32 row, wyInt32 col); 

        ///Finished cell editing
        /**
        @param wparam           : IN column index
        @param buff             : IN modified data
        @returns wyTrue
        */
	    wyBool		            OnGvnEndLabelEdit(WPARAM wparam, wyChar *buff);

        ///When a cell editing is finished we have to save the value in our data structure
        /**
        @param wparam           : IN GVDISPINFO param
        @param lparam           : IN whether editing was cancelled
        @returns LRESULT
        */
	    LRESULT                 OnGvnSetOwnerColData(WPARAM wparam, LPARAM lparam);

        ///Function performs sorting when clicked on column header
        /**
        @param wparam           : IN column index
        @param lparam           : IN not used
        @returns wyTrue on success else wyFalse
        */
        wyBool                  OnGvnColumnClick(WPARAM wparam, LPARAM lparam);

        ///Suppy the icon code to be drawn representing the sort order
        /**
        @param wparam           : IN column index
        @returns icon code
        */
        wyInt32                 OnGvnColumnDraw(WPARAM wparam);

        ///Helper function to save the changes in the grid to the data structure
        /**
        @param row              : IN selected row
        @param col              : IN selected column
        @param void             : IN data
        @param collen           : IN data length
        @param isupdateform     : IN whether to update the form view
        @param iscancel         : IN whether the editing was canceled
        @returns TRUE on success else FALSE
        */
	    wyInt32                 UpdateCellValue(wyInt32 row, wyInt32 col, void * data, wyUInt32 collen, wyBool isupdateform, wyBool iscancel);

        ///Function appends special data like function name, default etc to query
        /**
        @param value            : IN     value to be appended
        @param fieldname        : IN     column name
        @param query            : IN/OUT query to which the data to be appenend
        @returns wyTrue if the data is appended else wyFalse
        */
	    wyBool                  AppendSpecialDataToQuery(wyChar* value, wyChar* fieldname, wyString &query);
            
	    ///Adds the default value to the field
        /**
        @param fieldres         : IN     mysql result set
        @param fieldname        : IN     field name
        @param query            : IN/OUT query to which the data to be appenend
        @returns wyTrue on success else wyFalse
        */
        wyBool                  HandlerToAddDefault(MYSQL_RES *fieldres, const wyChar* fieldname, wyString &query);

        ///Function appends data to query
        /**
        @param myrow            : IN     mysql row
        @param col              : IN     column
        @param colcount         : IN     column count
        @param query            : IN/OUT query to which the data to be appenend
        @returns wyTrue if the data is appended wle wyFalse
        */
	    wyBool                  AppendDataToQuery(MYSQL_ROW myrow, wyInt32 col, wyInt32 colcount, wyString &query, wyBool isblob);

		wyBool                  AppendDataToQueryJSON(MYSQL_ROW myrow, wyInt32 col, wyInt32 colcount, wyString &query, wyBool isJSON);
        ///Helper function for showing mysql errors
        /**
        @param query            : IN query that raised the error
        @returns error status
        */
        wyInt32                 HandleErrors(wyString &query);

        ///Helper function to check for duplicate entry
        /**
        @param data             : IN  result of count(*)
        @param response         : OUT response, whether to carry on the process or not. This is to avoid prompting multiple times when deleting rows
        @param message          : IN  context message
        @returns wyTrue if response = IDCANCEL else wyTrue
        */
        wyBool                  CheckForDuplicates(MYSQL_RES* data, wyInt32 &response, wyInt32 message);

        //Handle Toolbar icon dropwown
	    /**
	    @param hdtoolbar        : IN toolbar notification structure
	    @returns 1
	    */
	    wyInt32		            HandleToolBarDropDown(LPNMTOOLBAR hdtoolbar);

    	///Helper function for HandleToolBarDropDown
        /**
        @param tbh			    : IN toolbar notify info
	    @param hwndgrid			: IN handle to the grid
	    @param selectedrows     : IN to checked rowcount
        @returns void
        */
	    void			        OnTBDropDownCopy(NMTOOLBAR* tbh, HWND hwndgrid, wyInt32 selectedrows);

        ///Gets the button type rect, used in view resizing
        /**
        @param hwnd             : IN  handle to the toolbar
        @param prect            : OUT button rect
        @param ctrltype         : IN  
        @returns wyTrue on success else wyFalse
        */
        wyBool                  GetButtonSize(HWND hwnd, RECT* prect, wyInt32 ctrltype);

        ///Subclassed window proc for info window
        /**
		@param hwnd             : IN handle to the Table Data edit Window. 
	    @param message          : IN specifies the message. 
		@param wparam           : IN specifies additional message-specific information. 
		@param lparam           : IN specifies additional message-specific information. 
		@returns LRESULT
		*/
        static LRESULT CALLBACK TableInfoWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Helper function to resize refresh toolbar
        /**
        @param minheight        : IN  minimum height required
        @param prect            : OUT window rect after resizing
        @returns void
        */
        void                    ResizeRefreshTool(wyInt32 minheight, RECT* prect);

        ///Helper function to resize non-toolbar controls
        /**
        @param parentrect       : IN parent window rectangle
        @returns void
        */
        void                    ResizeControls(RECT parentrect);

        ///Function adds refreshbar buttons
        /**
        @returns void
        */
        void                    AddRefreshBarButtons();

        ///Function sets the display mode
        /**
        @param mode             : IN display mode
        @param isupdateview     : IN whether to update the view immediately
        @returns void
        */
        void                    SetDispMode(ViewType mode, wyBool isupdateview = wyTrue);

        ///Function to generate update query
        /**
        @param query            : OUT the query generated
        @param issetlimit       : IN  whether to append limit
        @return wyTrue if query generation is successful. wyFalse otherwise. Calling function should make sure that query length is > 0
        */
        wyBool                  GenerateUpdateQuery(wyString& query, wyBool issetlimit);

        ///Function to generate delete query
        /**
        @param query            : OUT the query generated
        @param issetlimit       : IN  whether to append limit
        @param row              : IN  row index
        @return wyTrue if query generation is successful. wyFalse otherwise. Calling function should make sure that query length is > 0
        */
        wyBool                  GenerateDeleteQuery(wyString& query, wyBool issetlimit, wyUInt32 row);

        ///Function checks whether the column is readonly
        /**
        @param col              : IN column index
        @returns wyTrue if it is readonly otherwise wyFalse
        */
        wyBool                  IsColumnReadOnly(wyInt32 col);

		//function to check whether a column is virtual or not
		wyInt32					IsColumnVirtual(wyInt32 col);
    
        ///Generic function to add data to query, for update, delete and duplicate check
        /**
        @param data             : IN     row to be used to get the data
        @param query            : IN/OUT query string
        @param delimiter        : IN     the delimiter to use 
        @param nullcheck        : IN     operator to use with NULL data, i.e = or IS NULL
        @param isfirst          : IN     whether it is the first time this function is called for the current query to add data
        @param ischeckspdata    : IN     whether to check for special data like function, default etc
        @returns wyTrue if data is added else wyFalse
        */
        wyBool                  AddDataToQuery(MYSQL_ROW data, wyString &query, const wyChar* delimiter, const wyChar* nullcheck, wyInt32 col, wyBool isfirst = wyTrue, wyBool ischeckspdata = wyFalse);


		wyBool                  AddDataToQueryJSON(MYSQL_ROW data, wyString &query, const wyChar* delimiter, const wyChar* nullcheck, wyInt32 col, wyBool isfirst = wyTrue, wyBool ischeckspdata = wyFalse);

        ///Function generates the query to check duplicates
        /**
        @param row              : IN  row for which the query to be generated
        @param query            : OUT query generated
        @returns wyTrue on success else wyFalse
        */
        wyBool                  GenerateDuplicatesQuery(wyInt32 row, wyString &query);

        ///Function check whether the column is of binary type
        /**
        @param col              : IN column index
        @returns wyTrue if it is binary type else wyFalse
        */
        wyBool                  IsBinary(wyInt32 col);

        ///Generate insert query for the currently selected row
        /**
        @param query            : OUT query generated
        @returns wyTrue if query generated, else wyFalse
        */
        wyBool                  GenerateInsertQuery(wyString &query);

        ///Function cancels any unsaved changes made in the grid
        /**
        @param iscancelediting  : IN whether to cancel grid editing
        @returns wyTrue on success else wyFalse
        */
        wyBool                  CancelChanges(wyBool iscancelediting = wyTrue);

        ///Function generates the query required and updates the row
        /**
        @returns error status casted to integer
        */
        wyInt32                 UpdateRow();
	    
        ///Function generates the query required and inserts a row
        /**
        @returns error status casted to integer
        */
        wyInt32                 InsertRow();
	    
        ///Add a new row to grid
        /**
        @returns void
        */
        void                    AddNewRow();

        ///Function abstracts row deletion. If multiple rows are checked it will loop through and delete one by one
        /**
        @returns error status casted to integer while deleting a row
        */
	    wyInt32                 ProcessDelete();

        ///WM_CONTEXTMENU handler
        /**
        @param lparam           : IN LPARAM associated with the message
        @returns void
        */
        void                    OnContextMenu(LPARAM lparam);

        ///Destroy calandar control on ESC press
        /**
        @returns void
        */
    	void                    HandleEscape();

        ///Handles when click on a row check box
        /**
        @wparam                 : IN row index
        @returns void
        */
	    void                    OnGvnCheckBoxClick(WPARAM wparam);

        ///Handles the grid notification while drawing a row check. We need to supply the proper value to draw the check box
        /**
        @param row              : IN     row index
        @param rowcheck         : IN/OUT GVROWCHECKINFO pointer
        @returns wyTrue on success else wyFalse
        */
	    wyBool                  OnGvnDrawRowCheck(wyInt32 row, void *rowcheck);

        ///Enable/disable limit related windows
        /**
        @param enable           : IN whether to enable or disable
        @returns void
        */
        void                    EnableLimitWindows(wyBool enable = wyTrue);

        ///Function to get the limit from the controls and store in the member variables
        /**
        @returns void
        */
        void                    GetLimitValues();

        ///Function sets the control values
        /**
        @returns void
        */
        void                    SetLimitValues();

        ///Sends thread message synchronously. 
        ///The function checks whether calling thread is same as UI thread and uses either SendMessage or PostMessage
        /**
		@param hwnd             : IN handle to the Table Data edit Window. 
	    @param message          : IN specifies the message. 
		@param wparam           : IN specifies additional message-specific information. 
		@param lparam           : IN specifies additional message-specific information. 
		@returns void
		*/
        static void             SendThreadMessage(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Sets the thread message event so that SendThreadMessage can continue
        /**
        @param lparam           : IN LPARAM associated with the message sent by SendThreadMessage
        @returns void
        */
        static void             SetThreadMessageEvent(LPARAM lparam);

        ///Thread function to execute various operations
        /**
        lpparam                 : IN thread parameter
        @returns thread return value casted to unsigned
        */
        static	unsigned __stdcall  ExecuteThreadProc(LPVOID lpparam);

        ///Wrapper function to enable/disable controls
        /**
        @param isenable         : IN enable/disable controls
        @returns void
        */
        void                    EnableControls(wyBool isenable);
        
        ///Enable/disable refresh controls
        /**
        @param isenable         : IN enable/disable controls
        @returns void
        */
        void                    EnableRefreshControls(wyBool isenable);

        ///Wrapper function to show/hide modified/warning flag. It also sets the modified row so that update operation can be fast
        /**
        @param modifiedrow      : IN row index where updation happened, 
                                     if it is < 0 it will set modified row to -1, if it is -2 then it will show the warning if any
        @param issetstate       : IN whether to set the row state
        @returns void
        */
        void                    HandleRowModified(wyInt32 modifiedrow, wyBool issetstate = wyTrue);

        ///Function gets the image index of a toolbar button or the extra image
        /**
        @param id               : IN can be a button identifier, button index or the extra image index. Interpretation of this parameter is based on the value of flag parameter
        @param flag             : IN can be any of GETIMG_IMG/GETIMG_COMMAND/GETIMG_INDEX. 
                                     if flag = GETIMG_IMG, then id is interpreted as the extra images associated. Possible values of id are IMG_INDEX_STOP and IMG_INDEX_RESET_FILTER
                                     if flag = GETIMG_COMMAND, then id is interpreted as button identifier
                                     if flag = GETIMG_INDEX, then id is interpreted as button index
        @param hwndtool1        : IN handle to the first toolbar that shares the image list
        @param ...              : IN variable list of window handles that shares the image list with hwndtool1. The list should end with a NULL value
        @returns the image index on success else -1
        */
        wyInt32                 GetImageIndex(wyInt32 id, wyInt32 flag, HWND hwndtool1, ...);

        ///Add additional images. Call this for each toolbar if they want IMG_INDEX_STOP and IMG_INDEX_RESET_FILTER in their list. If two toolbar shares the same image list, call it for the second toolbar
        /**
        @param himglist         : IN image list for the toolbar
        @returns void
        */
        void                    AddExtraImages(HIMAGELIST himglist);

        ///Function sets the image indexes as the lparam for each toolbar button. Call this only once when you create the toolbar
        /**
        @param hwndtool         : IN toolbar handle
        @returns void
        */
        void                    SetImageIndexes(HWND hwndtool);

        ///Function gets the last warning
        /**
        @returns wyTrue if there are warnings else wyFalse
        */
        wyBool                  GetLastWarning();

        ///Function duplicates the current row
        /**
        @returns void
        */
        void                    DuplicateCurrentRow();

        ///Wrapper function to execute the query
        /**
        @param query            : IN query to be executed
        @param isdml            : IN whether the query is DML or not
        @returns the result set, can be NULL
        */
        MYSQL_RES*              ExecuteQuery(wyString& query, wyBool isdml = wyFalse);

        ///Function refreshes the active window, can be grid, form or text 
        /**
        @returns void
        */
        void                    RefreshActiveDispWindow();

        ///Function sets the refresh status for form view and text view. 
        //Form view need to be refreshed when the grid is repopulated or the splitter is moved. 
        //Text view need to refresh whenever form view needs a refresh or a row is added/deleted/updated
        /**
        @param isreset          : IN whether to set or reset a particular bit. The bit to be set is based on the flag parameter
        @param flag             : IN flag can be a combination of TEXTVIEW_REFRESHED and FORMVIEW_REFRESHED. 
        @returns void
        */
        void                    SetRefreshStatus(wyBool isreset = wyTrue, wyInt32 flag = VIEW_REFRESHED);

        ///Function copies select data from a particular cell to clipboard
	    /**
	    @returns wyTrue on success else wyFalse
	    */
	    wyBool			        CopyCellDataToClipBoard();

	    ///Function to add selected data in clipboard in CSV format.
	    /**
	    @returns wyTrue on success else wyFalse
	    */
	    wyBool			        AddDataToClipBoard();

	    ///Copies selected rows into clipboard.
	    /**
	    @returns wyTrue on success else wyFalse
	    */
	    wyBool			        AddSelDataToClipBoard();

	    ///Gets data from the clipboard
	    /**
	    @param selected		    : IN selected row or all the rows
        @returns the memory
	    */
	    HGLOBAL			        GetViewData(wyBool selected); 

	    ///Calculate the total memory required to copy the contents of the clipboard to the memory
	    /**
	    @param cesv			    : IN exportCsv class object pointer
	    @param selected		    : IN selected tables or all
	    @param esclen		    : IN length of the escape char
	    @returns total size of the memory
	    */
	    DWORD64			        GetTotalMemSize(ExportCsv *cesv, wyBool selected, wyInt32 esclen); 

        ///Checks for potential buffer overrun with the newly requested size and corrects it if required
        /**
        @param hglobal          : IN/OUT pointer to the handle of the gloabl block
        @param buffer           : IN/OUT pointer to the string buffer
        @param nsize            : IN     consumed buffer size
        @param bytestobeadded   : IN     number of bytes going to be added
        @returns wyTrue on success else wyFalse
        */
        wyBool                  VerifyMemory(HGLOBAL* hglobal, LPWSTR* buffer, wyUInt32 nsize, wyUInt32 bytestobeadded);

	    ///Function to write fixed data into memory.
	    /**
	    @param buffer		    : IN/OUT final modified text 
	    @param nsize		    : OUT    resultant size
	    @param text			    : IN     text to write
	    @param fields		    : IN     fields information
	    @param length		    : IN     fields length
	    @param isescaped	    : IN     is the text escaped
	    @param escape		    : IN     escape char to use
	    @param fterm		    : IN     fields terminated by text
	    @param isfterm		    : IN     fields terminated enabled
	    @param encl			    : IN     fields enclosed by text
	    @param isencl		    : IN     fields enclosed by enabled
        @returns wyTrue on success else wyFalse
	    */
	    wyBool			        WriteFixed(HGLOBAL* hglobal, LPWSTR* buffer, wyUInt32 * nsize, wyChar *text, MYSQL_FIELD * field, wyUInt32 length, wyBool isescaped, wyChar escape, wyChar fterm, wyBool isfterm, wyChar lterm, wyBool islterm, wyChar encl, wyBool isencl);

	    ///Function to write variable length data to the memory.
	    /**
	    @param buffer		    : IN/OUT final modified text 
	    @param nsize		    : OUT    resultant size
	    @param text			    : IN     text to write
	    @param fields		    : IN     fields information
	    @param length		    : IN     fields length
	    @param isescaped	    : IN     is the text escaped ?
	    @param escape		    : IN     escape char to use
	    @param fterm		    : IN     fields terminated by text
	    @param isfterm		    : IN     fields terminated enabled ?
	    @param lterm		    : IN     line terminated by
	    @param islterm		    : IN     is line terminated by enabled ?
	    @param encl			    : IN     fields enclosed by text
	    @param isencl		    : IN     fields enclosed by enabled ?
	    @param isoptionally	    : IN     is optionally selected ?
        @returns wyTrue on success else wyFalse
	    */
	    wyBool 			        WriteVarible(HGLOBAL* hglobal, LPWSTR* buffer, wyUInt32 * nsize, wyChar *text, MYSQL_FIELD * field, wyUInt32 length, wyBool isescaped, wyChar escape, wyChar fterm, wyBool isfterm, wyChar lterm, wyBool islterm, wyWChar encl, wyBool isencl, wyBool isoptionally, wyChar* nullchar, wyBool isnullreplaceby);

        ///Function to set the value of the currently selected cell
        /**
        @param value            : IN value to be updated
        @param isupdateformview : IN whether to update form view
        @return void
        */
        void                    SetValue(wyChar* value, wyBool isupdateformview = wyTrue);

        ///Function sets the text view font
        /**
        @returns void
        */
        void                    SetTextViewFont();

        ///Get the execution info from execution stack.
        /**
        @param execinfo         : IN current execution elem
        @param ispush           : IN whether to push to stack or pop from stack
                                     if push, return value is the previous top element in the stack
                                     if pop, return value is the current top element in the stack. The pop operation removes the elements untill the current top most elements thread action differs the passed elements thread action
        @returns previous top element for push, and current top element for pop
        */
        LPEXEC_INFO             GetExecInfoFromStack(LPEXEC_INFO execinfo, wyBool ispush);

        ///Function creates the columns in the grid
        /**
        @param isupdate         : IN whether to update only. If it is true, then the function wont delete and recreate the columns. Usefull to update column mask
        @returns void
        */
        void                    CreateColumns(wyBool isupdate = wyFalse);
        
        ///Function adds the enum/set values to the grid column
        /**
        @param col              : IN column index
        @returns void
        */
        void                    AddEnumSetColumnValues(wyInt32 col);
        
        ///Function parses the filed info and gets the values in the list 
        /**
        @param col              : IN column index
        @param plist            : IN pointer to the list where to add the values
        @returns void
        */
        void                    GetEnumListValues(wyInt32 col, List* plist);

        ///Function fills the data grid by creating columns and setting the row count and all
        /**
        @returns void
        */
        void                    FillDataGrid();

        ///Function resets the toolbar buttons' image to its original state
        /**
        @param hwndtool         : IN handle to the toolbar button
        */
        void                    ResetButtons(HWND hwndtool);

        ///Function shows/hides the form view
        /**
        @param show             : IN show command. can be SW_SHOW or SW_HIDE
        @param isprobe          : IN do not apply the command. Just check whether the operation is vaid
        @returns wyTrue on success else wyFalse
        */
        wyBool                  ShowFormView(wyInt32 show, wyBool isprobe = wyFalse);

        ///Checks whether form view is the active view
        /**
        @returns wyTrue if formview is the active view, else wyFalse
        */
        wyBool                  IsFormView();

        ///Function sets the status of view buttons
        /**
        @returns void
        */
        void                    UpdateViewButtons();

        ///Function deletes a row from the array
        /**
        @param row              : IN row to be deleted
        @param issetstate       : IN whether to update the status of the row. Check the implementation for details
        @returns void
        */
        void                    FreeRow(wyInt32 row, wyBool issetstate = wyTrue);

        ///Function to determine and supply the select all check state
        /**
        @param lparam           : IN/OUT message parameter
        @returns void
        */
        void                    OnDrawSelectAllCheck(LPARAM lparam);

        ///Function handles click on select all check
        /**
        @param lparam           : IN/OUT message parameter
        @returns void
        */
        void                    OnSelectAllCheck(LPARAM lparam);

        ///Function to get the ribbon color to draw info ribbon
        /**
        @param bg               : OUT background color
        @param fg               : OUT foreground color
        @returns void
        */
        void                    GetRibbonColorInfo(COLORREF* bg, COLORREF* fg);

        ///Function to get hyperlink color info
        /**
        @param fg               : OUT foreground color
        @returns void
        */
        void                    GetHyperLinkColorInfo(COLORREF* fg);

        ///Function checks whether the currently selected row can be duplicated
        /**
        @returns wyTrue if it can be duplicated else wyFalse
        */
        wyBool                  IsCurrRowDuplicatable();

        ///Subclassed window procedure for refresh controls
        /**
		@param hwnd             : IN handle to the Table Data edit Window. 
	    @param message          : IN specifies the message. 
		@param wparam           : IN specifies additional message-specific information. 
		@param lparam           : IN specifies additional message-specific information. 
		@returns LRESULT
		*/
        static LRESULT CALLBACK RefreshCtrlWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

        ///Function sets the limit values
        /**
        @param pexecinfo        : IN execution info. values are set based on m_la member of this parameter
        @returns void
        */
        void                    SetRefreshValues(LPEXEC_INFO pexecinfo);

        ///Shows the context menu for the editor
        /**
        @param lparam       : IN window procedure lParam(Window cordinamtes of mouse)
        @return void
        */
	    wyInt32                 OnTextContextMenu(LPARAM lparam);

        ///Handles the WM_COMMAND from editor
        /**
        @param wparam           : IN command identifier
        @returns 0
        */
    	wyInt32                 OnEditWmCommand(WPARAM wparam);

        ///Function handle end change row notification from grid. To enable disable various toobar buttons
        /**
        @param wparam           : IN row index
        @returns void
        */
        void                    OnGvnEndChangeRow(WPARAM wparam);

        ///Function to handle WM_SYSKEYDOWN message. 
        /**
        @param wparam           : IN message parameter
        @param lparam           : IN message parameter
        @returns zero if the message is handled, else non zero
        */
        wyInt32                 OnSysKeyDown(WPARAM wparam, LPARAM lparam);
       
        ///Parent window handle
		HWND			        m_hwndparent;

        ///Frame window handle
		HWND			        m_hwndframe;

        ///Grid window
		HWND			        m_hwndgrid;

        ///Text window
		HWND			        m_hwndtext;
	
        ///Toolbar
		HWND			        m_hwndtoolbar;

        ///Limit check box
		HWND			        m_hwndislimit;

        ///First row label
		HWND			        m_hwndfirstrowlabel;
		
        ///First row field
        HWND			        m_hwndfirstrow;
		
        ///Previous button
        HWND			        m_hwndprev;
		
        ///Next button
        HWND			        m_hwndnext;
		
        ///Limit label
        HWND			        m_hwndlimitlabel;
		
        ///Limit field
        HWND			        m_hwndlimit;
		
        ///Warning window
        HWND			        m_hwndwarning;
		
        ///Modified window
        HWND			        m_hwndmodified;
        
        ///Info ribbon
        HWND                    m_hwndshowtableinfo;
		
        ///Calander control
        HWND			        m_hwndcal;

        ///Image list for toolbar
		HIMAGELIST	            m_himglist;

        ///Image list for refresh toolbar
        HIMAGELIST              m_hrefreshtoolimglist;

        ///Icon Handles for next and previous
	    HICON                   m_iconnext;
	    HICON                   m_iconprevious;
		
        ///Grid window procedure. Derived class can point this to a different function in the constructor to subclass the grid procedure 
		WNDPROC			        m_gridwndproc;

        ///Original window proc for text view
		WNDPROC			        m_wptextview;

        ///Originla window proc for wanring window
        WNDPROC                 m_warningwndproc;
	
        ///MDI window pointer
		MDIWindow*              m_wnd;

        ///Mark icon for grid
		HICON		            m_markicon;

        ///Critical section
        CRITICAL_SECTION        m_cs;

        ///Data
	    MySQLDataEx*            m_data;

        ///Arguments 
        wyChar**                m_arg;

        ///Argument Count 
        wyInt32                 m_argcount;

        ///Variable to start/stop worker thread
        wyInt32			        m_stop;

        ///Last clicked row, used to implement shift+click in grid
        LONG                    m_lastclick;

        ///Priginal refresh toolbar proc
        WNDPROC                 m_refreshtoolproc;

        ///Font for modified and warning windows
        HFONT                   m_modifiedfont;

        ///Refresh toolbar
        HWND                    m_hwndrefreshtool;

        ///Extra images
        wyInt32                 m_extraimages[IMG_INDEX_LAST];

        ///Total memory size for text operations
        DWORD64                 m_totalsize;

        ///Stack for exec elem
        List                    m_execelemstack;

        ///Refresh status
        wyInt32                 m_refreshstatus;

        ///Tooltip text
        wyString                m_tooltiptext;
        
        ///View type
        ViewType                m_viewtype;

        ///Current context menu
        HMENU                   m_htrackmenu;

        ///Whether to do buffered drawing
        wyBool                  m_isbuffereddrawing;

        //Padding window
        HWND                    m_hwndpadding;

		// IQueryBuilder interface
		IQueryBuilder*			m_querybuilder;

		// backtick string can be used across multiple methods. But reinit them from .ini
		wyChar*					m_backtick;
};

#endif