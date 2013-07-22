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

#ifndef _OBJECT_INFO_H_
#define _OBJECT_INFO_H_

#include "GUIHelper.h"
#ifndef COMMUNITY
#include "SchemaOptimizer.h"
#include "RedundantIndexFinder.h"

class SchemaOptimizer;
class RedundantIndexFinder;
#endif

class FindAndReplace;

class ObjectInfo
{
    public:
        ObjectInfo(MDIWindow* pmdi, HWND hwndparent);
        ~ObjectInfo();

        void        Create();
        void        CreateEditWindow();
        void        CreateHTMLControl();
	    void        CreateToolBar();
        void        SetColor();
        void        SetFont();
        static LRESULT CALLBACK HtmlWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
        static LRESULT CALLBACK ToolbarWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK FrameProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        static LRESULT CALLBACK EditProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        void        ShowOnTop();

        void        Refresh(wyBool isforce = wyFalse);

        void        WriteSelectedFormatOption();

        HWND        GetActiveDisplayWindow();

        void        InitView();

//-------------------------------------------------------------------------------


	wyInt32  OnWmCommand(HWND hwnd, WPARAM wParam);

    ///Function to resize the Edit box.
    /**
    @returns void
    */
	void	Resize();

    void    OnResize();

    /// Creates the Query window
    /**
    @param wnd      : IN MDIWindow class pointer
    @returns wyTrue on success
    */
	wyBool		Create(MDIWindow *wnd);
	
    /// Appends the current Title line
    /**
    @param title        : IN/OUT Title string
    @returns void
    */
    void        AppendLine(wyString &title);
	
    /// Function to show various information about the Server.
    /**
    @param pcquerywnd   : IN MDIWindow class pointer
    */
	wyBool		ShowServerInfo(MDIWindow *pcquerywnd);

	/// Function to show various information about the database.
    /**
    @param pcquerywnd   : IN MDIWindow class pointer
    @param db           : IN Database name
    */
	wyBool		ShowDBInfo(MDIWindow *pcquerywnd, const wyChar *db);

    /// Function to show various information about the table.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
    @param db           : IN Database name
    @param table        : IN Table name
	@param obj			: IN type of object to be shown in Info tab(view or table)
	@return wyTrue on success else return wyFalse
    */
	wyBool		ShowTableInfo(MDIWindow *pcquerywnd, const wyChar *db, const wyChar *table, OBJECT obj, wyBool istoanalyse = wyFalse);

	///Shows the Table's Column info in Info tab
	/**
	@param pcquerywnd   : IN MDIWindow class pointer
	@param myres		: IN MYSQL_RES pointer
	@param table        : IN Table name
	@param obj			: IN Object type(Index)
	@return wyTrue on success else return wyFalse
	*/
	wyBool		ShowTableColumnInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *table, OBJECT obj);
     
	///SHows the Table's Index info in Info tab
	/**
	@param pcquerywnd   : IN MDIWindow class pointer
	@param myres		: IN MYSQL_RES pointer
	@param table        : IN Table name
	@param obj			: IN Object type(table)
	@return wyTrue on success else return wyFalse
	*/
	wyBool		ShowTableIndexInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *table, OBJECT obj);

	///Shows the Table/View DDL info in Info tab
	/**
	@param pcquerywnd   : IN MDIWindow class pointer
	@param myres		: IN MYSQL_RES pointer
	@param table        : IN Table name
	@param obj			: IN Object type(table)
	@return wyTrue on success else return wyFalse
	*/
	wyBool		ShowTableDDLInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *table, OBJECT obj);

    /// Function to show various information about PROCEDURE.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
    @param db           : IN Database name
    @param procedure    : IN Procedure name
    */
	wyBool		ShowProcedureInfo(MDIWindow *pcquerywnd, const wyChar *db, const wyChar *procedure);

	///Shows the Procedure - DDL info in HTML format
	/**
	@param pcquerywnd   : IN MDIWindow class pointer
	@param myres		: IN MYSQL_RES pointer
	@param procedure    : IN Proc name
	@return wyTrue on success else return wyFalse
	*/
	wyBool		ShowProcedureInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *procedure);

    /// Function to show various information about FUNCTION.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
    @param db           : IN Database name
    @param function     : IN function name
    */
	wyBool		ShowFunctionInfo(MDIWindow *pcquerywnd, const wyChar *db, const wyChar *function);

	///Shows the Procedure - DDL info in HTML format
	/**
	@param pcquerywnd   : IN MDIWindow class pointer
	@param myres		: IN MYSQL_RES pointer
	@param function     : IN Function name
	@return wyTrue on success else return wyFalse
	*/
	wyBool		ShowFunctionInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *function);

    /// Function to show various information about the trigger.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
    @param db           : IN Database name
    @param table        : IN Table name
    @param trigger      : IN Trigger name
    */
	wyBool		ShowTriggerInfo(MDIWindow *pcquerywnd, const wyChar *db, const wyChar *table, const wyChar *trigger);

	///Shows the Trigger info in HTML format
	/**
	@param pcquerywnd   : IN MDIWindow class pointer
	@param myres		: IN MYSQL_RES pointer
	@param trigger     : IN Trigger name
	@return wyTrue on success else return wyFalse
	*/
	wyBool		ShowTriggerInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *trigger);
 
	/// Function to show various information about Event.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
    @param db           : IN Database name
    @param function     : IN Event name
    */
	wyBool		ShowEventInfo(MDIWindow *pcquerywnd, const wyChar *db, const wyChar *event);

	///Shows the Event info in HTML format
	/**
	@param pcquerywnd   : IN MDIWindow class pointer
	@param myres		: IN MYSQL_RES pointer
	@param event		: IN Event name
	@return wyTrue on success else return wyFalse
	*/
	wyBool		ShowEventInfoHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *event);
	
	/// Function to show information about all Tables.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
    @param db           : IN Database name
	@param obj			: IN type of object to be shown in Info tab
    @returns wyTrue on success else wyFalse
    */
	wyBool		ShowAllTables(MDIWindow *pcquerywnd, const wyChar *db, OBJECT obj);

	/// Function to show information about all Tables in HTML format
	/**
	@param pcquerywnd   : IN/OUT MDIWindow class pointer
	@param myres        : pointer to MYSQL_RES
    @param db           : IN Database name
	@param obj			: IN type of object to be shown in Info tab
    @returns wyTrue on success else wyFalse
	*/
	wyBool		ShowAllTablesHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *db, OBJECT obj);
	
	/// Function to show information about all Views.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
    @param db           : IN Database name
	@param obj			: IN type of object to be shown in Info tab
    @returns wyTrue on success else wyFalse
    */
	wyBool		ShowAllViews(MDIWindow *pcquerywnd, const wyChar *db, OBJECT obj);

	/// Function to show information about all Views in HTML format.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
	@param myres        : pointer to MYSQL_RES
    @param db           : IN Database name
	@param obj			: IN type of object to be shown in Info tab
    @returns wyTrue on success else wyFalse
    */
	wyBool		ShowAllViewsHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *db, OBJECT obj);
	
	/// Function to show information about all Procedures.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
    @param db           : IN Database name
	@param obj			: IN type of object to be shown in Info tab
    @returns wyTrue on success else wyFalse
    */
	wyBool		ShowAllProcedures(MDIWindow *pcquerywnd, const wyChar *db, OBJECT obj);

	/// Function to show information about all 'Procedures' in HTML format.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
	@param myres        : pointer to MYSQL_RES
    @param db           : IN Database name
	@param obj			: IN type of object to be shown in Info tab
    @returns wyTrue on success else wyFalse
    */
	wyBool		ShowAllProceduresHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *db, OBJECT obj);
	
	/// Function to show information about all Functions.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
    @param db           : IN Database name
	@param obj			: IN type of object to be shown in Info tab
    @returns wyTrue on success else wyFalse
    */
	wyBool		ShowAllFunctions(MDIWindow *pcquerywnd, const wyChar *db, OBJECT obj);

	/// Function to show information about all 'Functions' in HTML format.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
	@param myres        : pointer to MYSQL_RES
    @param db           : IN Database name
	@param obj			: IN type of object to be shown in Info tab
    @returns wyTrue on success else wyFalse
    */
	wyBool		ShowAllFunctionsHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *db, OBJECT obj);
	
	/// Function to show information about all Triggers.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
    @param db           : IN Database name
	@param obj			: IN type of object to be shown in Info tab
    @returns wyTrue on success else wyFalse
    */
	wyBool		ShowAllTriggers(MDIWindow *pcquerywnd, const wyChar *db, OBJECT obj);

	/// Function to show information about all 'Triggers' in HTML format.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
	@param myres        : pointer to MYSQL_RES
    @param db           : IN Database name
	@param obj			: IN type of object to be shown in Info tab
    @returns wyTrue on success else wyFalse
    */
	wyBool		ShowAllTriggersHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *db, OBJECT obj);
	
	/// Function to show information about all Events.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
    @param db           : IN Database name
	@param obj			: IN type of object to be shown in Info tab
    @returns wyTrue on success else wyFalse
    */
	wyBool		ShowAllEvents(MDIWindow *pcquerywnd, const wyChar *db, OBJECT obj);

	/// Function to show information about all 'Events' in HTML format.
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
	@param myres        : pointer to MYSQL_RES
    @param db           : IN Database name
	@param obj			: IN type of object to be shown in Info tab
    @returns wyTrue on success else wyFalse
    */
	wyBool		ShowAllEventsHtmlFormat(MDIWindow *pcquerywnd, MYSQL_RES *myres, const wyChar *db, OBJECT obj);

    /// Generic function to add the information whose query is passed as parameter.
    /**
    @param tunnel       : IN Tunnel pointer
    @param mysql        : IN Mysql pointer
    @param query        : IN Query string
    @param temptitle    : IN Temp title
    @param createtable  : IN Show create statement or not
	@returns MYSQL_RES pointer if success else return NULL
    */
	MYSQL_RES*		AddInfo(Tunnel *tunnel, PMYSQL mysql, wyString &query, wyString &temptitle,  
						wyBool createtable = wyFalse, MYSQL_RES *res = NULL);

	///Add teh MySQL result into info tab in normal form(Not table fom) to avoid padding spaces in DDL statements while copying
	/**
	@param pcquerywnd : IN Con. window pointer
	@param objname	  : IN Object name
	@param query	  : IN Query to execute
	@param temptitle  : IN Result set title
	@param obj		  : IN Selected object from obj-browser
	@returns MYSQL_RES pointer if success else return NULL
	*/
	MYSQL_RES*		AddMySQLColInfo(MDIWindow *pcquerywnd, const wyChar *objname, wyString &query, wyString &temptitle, OBJECT obj, MYSQL_RES *res = NULL);

    /// Show details of the particular object selected
    /**
    @param pcquerywnd   : IN/OUT MDIWindow class pointer
    @param db           : IN Database name
    @param table        : IN Table name
    @param obj          : IN Object type
    @param trigger      : IN Trigger name
	@returns wyTrue on success else wyFalse
    */
	wyBool		ShowValues(MDIWindow *pcquerywnd, const wyChar *db, const wyChar *table, OBJECT obj, wyBool isforce, const wyChar *trigger = NULL);

	///Show HTML formatted o/p on select 'server' icon on object browser
	/**
	@param wnd	: IN Con. window pointer
	@param res	: IN MySQL result set
	@param isshowvars : IN WyTrue for 'SHOW VARIABLES o/p, and wyFalse for SHOW STATUS o/p
	@param htmlinbuff : OUT HTML buffor keeps formatted output
	@return wyTrue on success else return wyFalse
	*/
	wyBool		ShowHtmlFormatResultOnServerObject(MDIWindow *wnd, MYSQL_RES *res, wyBool isshowvars, wyString *htmlinbuff);					

	///Add HTML formatted result in to info tab as per the selection on 'Object browser'
	/**
	@param wnd : IN Con window pointer
	@param myres : IN MySQL result set
	@param columns : IN defined the coulms to be considered, NULL if to display all columns
	@param db : IN object name
	@param obj : IN Object type
	*/
	wyBool		AddObjectInfoHtmlFormat(MDIWindow *wnd, MYSQL_RES *myres, wyString *columns, const wyChar *db, OBJECT obj, wyBool isdbobject = wyFalse, wyBool istopasscontrol = wyTrue);


	void		PrintHTML(OBJECT obj, wyString* htmlbuffer);

	
	wyBool		HandleOptimizedSchemaTable(MDIWindow *wnd, wyString *htmlbuffer, MYSQL_RES *myresfields, wyString *columns);

	///Finds the HTML title and Image
	/**
	@param object		: IN Object type
	@param db			: IN object name
	@param title		: OUT title of html
	@param image		: OUT Image path
	@param imagecaption : OUT Image caption
	@param isdbobject	: IN sets wyTrue if selected object is 'OBJECT_DATABASE'
	@return wyTrue for success else return wyFalse
	*/
	wyBool		AddHtmlFormatTitleAndImage(OBJECT object, const wyChar *db, wyString *title, wyString *image, wyString *imagecaption, wyBool isdbobject = wyFalse);

	//Display 'Index' info for table in HTML format
	/**
	@param wnd	 : IN Con window pointer
	@param myres : IN MySQL result set
	@param db	 : IN object name
	@return wyTrue if its success else return wyFalse
	*/
	wyBool		AddObjectInfoHtmlFormatForIndex(MDIWindow *wnd, MYSQL_RES *myres, const wyChar *db, wyString *htmlbuffer);

	///Add selected columns of result sets to HTML
	/**
	@param wnd			: IN Con. window pointer
	@param objectname	: IN Object name
	@param myres		: IN MYSQL_RES pointer
	@param obj			: IN Selected OBJECT TYPE
	@param columns		: IN Buffer keeps the column names to display
	@return VOID
	*/
	VOID		AddHtmlSelectedMySQLColumns(MDIWindow *wnd, const wyChar *objectname, MYSQL_RES *myres, OBJECT obj, wyString *columns, wyString *htmlbuffer);

	//Add 'column information' html table's column headers
	/**
	@param tok			: IN tocken holds the coulmn name
	@param htmlbuffer	: IN Html buffer
	@param strindex		: IN Indexes of the mysql rows to be displayed
	@param wnd			: IN con window handle
	@param myres		: IN mysql result set
	@return void
	*/
	VOID		AddColumnHeaderForTableColumnInfo(wyChar *tok, wyString *htmlbuffer, wyString &strindex,  MDIWindow *wnd, MYSQL_RES *myres);

	///Add Html coumns if not select option ;Optimize schema' for table
	/**
	@param tok			: IN tocken holds the coulmn name
	@param htmlbuffer	: IN Html buffer
	@param strindex		: IN Indexes of the mysql rows to be displayed
	@param wnd			: IN con window handle
	@param myres		: IN mysql result set
	*/
	VOID		AddColumnHeaderWithOutSchemaAnalyze(wyChar *tok, wyString *htmlbuffer, wyString &strindex,  MDIWindow *wnd, MYSQL_RES *myres, 
					OBJECT obj, wyInt32 &formatcolindex, wyInt32 &datalencolindex, wyInt32 &indexlencolindex);

	///Add Column information html table(For table column information) 
	/**
	@param strindex		: IN Indexes of the mysql rows to be displayed
	@param keytype		: IN coulmns represends the 'Key'
	@param htmlbuffer	: IN Html buffer
	@param wnd			: IN con window handle
	@param myres		: IN mysql result set
	@return void
	*/
	VOID		AddColumnDataForTableColumnInformation(wyString &strindex, wyInt32 keytype, wyString *htmlbuffer, MDIWindow *wnd, MYSQL_RES *myres);

	///Add Column information html table(For other than table column information) 
	/**
	@param strindex			: IN Indexes of the mysql rows to be displayed
	@param keytype			: IN coulmns represends the 'Key'
	@param htmlbuffer		: IN Html buffer
	@param wnd				: IN con window handle
	@param myres			: IN mysql result set
	@param obj				: IN object type
	@param objectname		: IN name of the object
	@param datalencolindex	: IN index of the datalen when db selectd
	@param indexlencolindex : IN "Index_length" when db selected
    @param formatcolindex	: IN column index to be formatted.
	@return void
	*/
	VOID		AddColumnDataInformation(wyString &strindex, wyInt32 keytype, wyString *htmlbuffer, MDIWindow *wnd, MYSQL_RES *myres, 
				OBJECT obj, const wyChar *objectname, wyInt32 datalencolindex, 
				wyInt32 indexlencolindex, wyInt32 formatcolindex);

	///Add extra column 'Optimal_fieldtype' on click 'Optimize schema' option
	/**
	@param htmlbuffer	: IN Html buffer
	@return void
	*/
	VOID		AddOptimazierFieldColumn(wyString *htmlbuffer);//, wyInt32 fieldwidth);

	///Optimized Schema(ALTER statement)
	/**
	@param myrowanalyse		: IN MYSQL_ROW of o/p of PROCEDURE ANALYSE()
	@param indexoptimize	: IN index of column 'optimal_fieldtype'
	@param ismysql41		: IN whether version is mySQL 41 or not
	@param fldname			: IN field name
	@param htmlbuffer		: IN Html buffer
	@param isautoincr		: IN flag for AUTO_INCREMENT 
	@return void
	*/
	VOID		FrameOptimizedSchemaStatement(MYSQL_ROW myrowanalyse, wyInt32 indexoptimize, wyBool ismysql41, wyChar *fldname, wyString *htmlbuffer, wyBool isautoincr);

	///Add all column of result set
	/**
	@param wnd	   : IN Con. window pointer
	@param myres   : IN MYSQL_RES pointer
	@param obj	   : IN Selected OBJECT TYPE
	@return VOID
	*/
	VOID		AddHtmlAllMySQLColumns(MDIWindow *wnd, MYSQL_RES *myres, OBJECT obj);

	//Add the 'Total Size' column data when selected 'Tables', (Total size = Data_length + Index_length)
	/**
	@param totallen : IN reference to Toal size variable
	@return void
	*/
	VOID		AddTableTotalDataSize(wyInt64 &totallen);

	///Initializing the HTML buffer with css. styles
	/**
	@return VOID
	*/
	VOID		HtmlBufferInit(wyString *htmlbuffer);
	
	///Converts time in seconds to appropriate format
	/**
	@param pcounter : IN time in seconds
	@param timebuff : OUT buffer keeps formatted time
	@return void
	*/
	VOID			TimeUnits(wyUInt32 pcounter, wyString *timebuff);

	///Convert the Numeric data in to better format(T, G, M, K)
	/**
	@param pBytes			: IN value to be formatted
	@param pDecimals		: IN number of decimals after the decimal point
	@param formattedvalue	: OUT buffer holds the formatted numeric value
	@return VOID
	*/
	VOID			StorageUnits(wyInt64 &pBytes, wyInt32 pDecimals, wyString *formattedvalue);

	///Format the storage value with its Unit
	/**
	@param ret				: IN Formatted value
	@param value			: IN Quatient
	@param formattedvalue	: OUT string gets the formatted value with unit
	@param unit				: IN unit to append(T, G, M, K)
	@return void
	*/
	VOID			FormatStoarageUnit(double *ret, wyInt32 value, wyString *formattedvalue, wyChar *unit);
	
	///Keeps the .ini base value for Format(HTML, TEXT) when select Info tab 1st time
	/**
	@return void
	*/
	VOID			SetSelectedResultFormatOption();

	///Adds the MySQL result sets in normal form(Not in table form)in text mode to avoid extra spacing while copying
	/**
	@param wnd			: IN Con. window pointer
	@param objectname	: IN Object name
	@param myres		: IN MYSQL_RES pointer
	@param temptitle	: IN Title of section
	@param column		: IN coulmns to display in Text mode
	@param obj			: IN Object type selected
	@return VOID
	*/
	VOID			FormatTextModeSQLStatement(MDIWindow *wnd, const wyChar *objectname, MYSQL_RES **res, wyString *temptitle, wyChar *column, OBJECT obj);

	VOID			FreeTableLevelResultsets();

	VOID			AddTableOptimizeTableSchema();

    ///Function adds alter stmts if any to the html buffer
    /**
    @param htmlbuffer   : IN/OUT string buffer to store html tags and symbols
    @returns void
    */
    void            AddAlterStmtForRedundantIndex(wyString* htmlbuffer);

    void            FreeDBLevelResultsets();

    void            Show(wyBool isshow);

    wyInt32         OnContextMenu(LPARAM lParam);


//--------------------------------------------------------------------------------

        ///Whether it is an object database
	    wyBool		m_isobjectdb;

        /// Window procedure
	    WNDPROC		m_wporigproc;
	
	    ///last selected Database	
	    wyString  m_lastseldb;

	    ///last selected object type
	    OBJECT	m_lastselnode;

	    ///last selected object name
	    wyString    m_lastselobj;

	    ///Tool bar handle
	    HWND		m_hwndtoolbar;

	    //'Refresh' button
	    HWND		m_hwndrefreshinfo;

	    //Window handle for HTML o/p
	    HWND		m_hwndhtmleditor;

	    //Radio button options for 'HTML format' and 'TEext Format'
	    HWND		m_hwndopthtml;
	    HWND		m_hwndopttext;

	    //For static text on toolbar 'Format:'
	    HWND		m_hwndoptstatic;

	    //Imagelist holds the icon to be for tool bar
	    HIMAGELIST	m_himglist;

	    //HTML buffer
	    wyString	m_htmlformatstr;

	    //Buffer keeps the server uptime info
	    wyString	m_serveruptime;

	    //Adds the table info in html format.
	    wyString	m_strtableinfo;

	    //Flag sets wyTrue when select the Info tab 1st time, for setting the format option as per .ini
	    wyBool		m_isinfotabalredyselected;

	    //For PROCEDURE ANALYSE()
	    MYSQL_RES	*m_myrestableanalyse;

	    //For table 'Column Information'
	    MYSQL_RES	*m_myrescolumninfo;

	    //For table 'Index Information'
	    MYSQL_RES	*m_myrestableindexinfo; 

	    //For table 'DDL info'
	    MYSQL_RES	*m_myrestableddlinfo;

	    //Whether selected option 'Opimize table'
	    wyBool		m_istableanalyse;

	    wyBool		m_isschemaoptimized;
	
	    //WHether to display 'Stop Analyse' caption
	    wyBool		m_istostopcaption;

	    //When click on 'Stop analyse' option
	    wyBool		m_isoptimizationaborted;

	    //Sets to wyTrue for hiding the optimal details
	    wyBool		m_ishiddenoptimization;

	    //Optimized 'ALTER' schema
	    wyString	m_optimizedschema;

	    //Page width in pixws for calculating the htl table size
	    wyInt32		m_pagewidth;

	    //Flag to hide the optimized coulmn
	    wyBool		m_istohideoptimizecolumn;

        //Flag tells whether to show redundant index info
        wyBool      m_showredundantindex;

        //Flag tells whether a particluar method is called from RedundantIndexFinder methods
        wyBool      m_calledfromredundantindex;

        //Table infos
        MYSQL_RES*  m_tableinfosres;

        //view infos
        MYSQL_RES*  m_viewinfores;

        //Procedure infos
        MYSQL_RES*  m_procinfores;

        //Function infos
        MYSQL_RES*  m_funcinfores;

        //Trigger infos
        MYSQL_RES*  m_triginfores;

        //Event infos
        MYSQL_RES*  m_eventinfores;

        //DBB ddl info
        MYSQL_RES*  m_dbddlinfores;

#ifndef COMMUNITY
	    ///Schema Optimizer handle
	    SchemaOptimizer*    m_schemaoptimize;

        //Redundant index finder handle
        RedundantIndexFinder* m_redindexfinder;
#endif

	    MDIWindow*  m_wnd;

	    wyBool              m_isMDIclose;

	    FindAndReplace		*m_findreplace;

	    HMENU m_menu;

        WNDPROC             m_origtoolproc;

        HWND                m_hwndparent;

        HWND                m_hwnd;

        HWND                m_hwndframe;

        WNDPROC             m_origframeproc;
};

#endif