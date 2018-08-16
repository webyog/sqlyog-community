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

#ifndef _SORTANDFILTER_H_
#define _SORTANDFILTER_H_

#include "Global.h"
#include "CommonHelper.h"
#include "DataView.h"

//forward declarations
class MySQLDataEx;
enum ThreadAction;
class MySQLRowExArray;
enum ThreadExecStatus;
enum ArrayAction;
class IQueryBuilder;

//Filter type enumeration
enum FilterType 
{
    FT_NONE = -1, 
    FT_EQUAL, 
    FT_NOTEQUAL, 
    FT_GREATERTHAN, 
    FT_LESSTHAN, 
    FT_LIKE, 
    FT_LIKEBEGIN, 
    FT_LIKEEND, 
    FT_LIKEBOTH
};

//Sort type enumeration
enum SortType 
{
    ST_NONE = -1, 
    ST_ASC = 2, 
    ST_DESC = 3
};

//Filter data
typedef struct st_filter_data
{
    ///Column index of the active filter
    wyInt32         m_currcolindex;

    ///Column index of the filter in process
	wyInt32	        m_colindex;

    ///Filter type of the active filter
    FilterType      m_currfiltertype;

    ///Filter type of the filter in process
	FilterType	    m_filtertype;

    ///Filter value of the active filter
    wyString        m_currvalue;

    ///Filter value of the filter in process
	wyString	    m_value;
}FILTER_DATA, *PFILTER_DATA;

//Sort Data
typedef struct st_sort_data
{
    ///Column index 
	wyInt32	    m_colindex;

    ///Current column index
    wyInt32     m_currcolindex;

    ///Sort type of the sort in process
	SortType	m_sorttype;

    ///Current column sort
    SortType    m_currsorttype;
} SORT_DATA, *PSORT_DATA;

///The class that wraps sort and filter functions
class SortAndFilter
{
    public:
        ///Constructor
        /**
        @param isclient             : IN is sorting/filtering to be done on client side. 
                                      If this is wyFalse, then the calling module should execute query to get the result, the class only provides the function to append the necessery sort/filter clauses to the query
        @param data                 : IN data to which the object is associated with
        */
                                    SortAndFilter(wyBool isclient, MySQLDataEx* data);
        
        ///Destructor
                                    ~SortAndFilter();

        ///Function positions the custom filter dialog box with respect to the rectangle given
        /**
        @returns void
        */
        void                        PositionWindow();
        
        ///Function to show the filter dialog
        /**
        @param hwndparent           : IN parent window handle
        @param colno                : IN currently selected column in the grid, can be -1
        @param data                 : IN cell data
        @param datalen              : IN cell data length
        @param prect                : IN rectangle with respect to which the dialog box to be positioned
        @returns IDOK or IDCANCEL
        */
        wyInt32                     OpenFilterDialog(HWND hwndparent, wyInt32 colno, wyChar* data, wyUInt32 datalen, RECT* prect);

        ///Callback procedure for the dialog box
		/**
		@param hwnd                 : IN handle to the Table Data edit Window. 
	    @param message              : IN specifies the message. 
		@param wparam               : IN specifies additional message-specific information. 
		@param lparam               : IN specifies additional message-specific information. 
		@returns LRESULT
		*/
        static INT_PTR CALLBACK     FilterWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

        ///Function to initialize the dialog box
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                      InitDialog();

		/// Initializes the main dlg procedure
		/**
		@param hwndparent:     IN       Window handle
		@returns void
		*/
		void						OnWMInitdlgValues(HWND hwnd);

        ///Function to reset sort
        /**
        @returns void
        */
        void                        ResetSort();

        ///Function to reset filter
        /**
        @param startindex           : IN index where resetting starts
        @returns void
        */
        void                        ResetFilter(wyInt32 startindex = 0);

        ///Function process the filter from custom filter dialog box
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                      ProcessFilter();

        ///Start column sort for the column specified. EndColumnSort should be called for each BeginColoumnSort
        /**
        @param col                  : IN column index
        @param isadd                : IN whether to add to the sort or a new sort
        @returns void
        */
        void                        BeginColoumnSort(wyInt32 col, wyBool isadd);

        ///Function finishes the column sort. This includes copying any new sort applied to the current sort, or canceling it
        /**
        @param issuccess            : IN whether the sorting was successful, if yes it copies the new sort to current sort else it copies the current sort to new sort
        @returns void
        */
        void                        EndColumnSort(wyBool issuccess);

        ///Function to begin a filter. EndFilter should be called for each BeginFilter
        /**
        @param command              : IN the command that invoked the call
        @param data                 : IN cell data
        @param datalen              : IN cell data length
        @param col                  : IN selected column, can be -1
        @param hwndparent           : IN handle to the parent window if you are invoking custom filter, can be NULL otherwise
		@param querybuilder			: IN queryBuilder interface
        @param prect                : IN rectangle with respect to which the dialog box to be positioned, can be NULL if the command is not custom filter
        @returns wyTrue on success else wyFalse
        */
        wyBool                      BeginFilter(wyInt32 command, wyChar* data, wyUInt32 datalen, wyInt32 col, HWND hwndparent = NULL, IQueryBuilder* querybuilder = NULL, RECT* prect = NULL);

        ///Function finishes the filter operation. This includes copying any new filter applied to the current filter, or canceling it
        /**
        @param issuccess            : IN whether the filtering was successful, if yes it copies the new filter to current filter
        @returns void
        */
        void                        EndFilter(wyBool issuccess);

        ///Get the column sort
        /**
        @param col                  : IN column index
        @returns ST_NONE, ST_ASC or ST_DESC
        */
        SortType                    GetColumnSort(wyInt32 col);

        ///Function to perform sort/filter or both
        /**
        @returns void
        */
        void                        ExecuteSortAndFilter();

        ///Comparison function to filter on NULL
        /**
        @returns wyTrue if value is NULL, else wyFalse
        */
        wyBool                      FilterOnNull();

        ///Function that checks the type of the filter column on which we filter and calls the required the Function that handles the Filtering based on the Filter condition
        /**
		@Param coltype              : IN mysql column type
		@param data                 : IN filter text
        @param ismysqlrow           : IN is the row a mysql row
        returns wyTrue if the match is found else wyFalse
        */
        wyBool	                    CheckTypeAndFilterOnCondition(enum_field_types coltype, wyChar* data, wyBool ismysqlrow);

		///Function that handles filter on the Integral Types
        /**
		@param data                 : IN pointer to the search text
        returns wyTrue if the match is found else wyFalse
		*/
		wyBool		                FilterOnIntegralTypes(wyChar* data);

		///Function that handles filter on the floating point types
        /**
		@param data                 : IN pointer to the search text
        returns wyTrue if the match is found else wyFalse
		*/
		wyBool		                FilterOnFloatingPointType(wyChar* data);
		
		///Function that handles filter on the string types
        /**
		@param data                 : IN pointer to the search text
        returns wyTrue if the match is found else wyFalse
		*/
		wyBool		                FilterOnStringType(wyChar* data);

		///Function that handles filter on 3 like conditions
        /**
		@param data                 : IN pointer to the search text
        returns wyTrue if the match is found else wyFalse
		*/
		wyBool		                FilterOnLike(wyChar* data);

        ///Function to sort in client side
		/**
        @returns void   
		*/
		wyBool                      SortResult();
		
        ///Compare function for quick sort
		/**
        @param psnf                 : IN application defined value
        @param a		            : IN element of the array to be sorted
        @param b		            : IN element of the array to be sorted
        @return -1 if a < b, 0 if a = b, 1 if a > b
		*/
		static wyInt32              Compare(void* psnf, const void* a, const void* b);

		///Handles sort of the string in compare function
		/*
		@param param1				: IN first element to be comared
		@param param1				: IN second element to be compared
        @param sorttype             : IN sort type
		@return compared result
		*/
		wyInt32                     HandleStringTypeSort(wyString *param1, wyString *param2, wyInt32 sorttype);

		///Handles sort of the floating point in compare
		/*
		@param wystringForArg1		: IN string representation of first argument
		@param wystringForArg2		: IN string representation of second argument
		@param sorttype             : IN sort type
		@return compared result
		*/
		wyInt32                     HandleFloatingTypeSort(wyString* wystringForArg1, wyString* wystringForArg2, wyInt32 sorttype);


		///Handles sort of the integral type in compare
		/*
		@param wystringForArg1		: IN string representation of first argument
		@param wystringForArg2		: IN string representation of second argument
		@param sorttype             : IN sort type
		@return compared result
		*/
		wyInt32                     HandleIntegralTypeSort(wyString* wystringForArg1, wyString* wystringForArg2, wyInt32 sorttype);

        ///Handles sort of year type
		/*
		@param param1       		: IN string representation of first argument
		@param param2       		: IN string representation of second argument
		@param sorttype             : IN sort type
		@return compared result
		*/
		wyInt32	                    HandleSortOnYearCol(wyString* param1, wyString* param2, wyInt32 sorttype);

		//Handling YEAR type needs to be done differently because of YEAR(2) option available.
        /**
		@param data                 : IN pointer to search text
		@returns wyTrue if a match is found else wyFalse
		*/
		wyBool	                    FilterOnYear(wyChar* data);

        ///Function to perform client side filtering
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool                      FilterResult();

        ///Function to set the filter string
        /**
        @param isnewfilter          : IN whether to set the current filter string or the new filter string
        @returns void
        */
        void                        SetFilterString(wyBool isnewfilter);

        ///Function to get the sort string
        /**
        @param sortstring           : OUT sort caluse to be appended
        @returns wyTrue on success else wyFalse
        */
        wyBool                      GetSortString(wyString& sortstring);

        ///Function to get the filter string
        /**
        @param filterstring         : OUT filter caluse to be appended
        @returns wyTrue on success else wyFalse
        */
        wyBool                      GetFilterString(wyString& filterstring);

        ///Function to replicate the changes in the data array to the original array in client side sort/filter.
        //This function is called in response to array notifications
        /**
        @param aa                   : IN array action
        @param index                : IN index of the item
        @param extra                : IN extra parameter, context sensitive
        @returns void
        */
        void                        SyncToOrigRowArray(ArrayAction aa, wyInt32 index, void* extra);

        ///Function to free the original array
        /**
        @returns void
        */
        void                        FreeOrigRowArray(wyBool iscallfromdestructor = wyFalse);

        ///Function to get the string representation of filter type
        /**
        @param ft                   : IN filter type to be stringized
        @returns string representation of ft
        */
        const wyChar*               GetFilterTypeStr(FilterType ft);

        ///Callback function for rowsex array. We need to update the original array when client side sort/filter is used
        /**
        @param aa                   : IN various array actions
        @param data                 : IN long data associated with the array
        @param index                : IN index of the array element being updated/deleted/inserted
        @param extra                : IN context sensitive parameter. Refer MySQLRowExArray.h for details
        @returns void
        */
        static void CALLBACK        RowsExArrayCallback(ArrayAction aa, void* data, wyInt32 index, void* extra);

        ///Overloaded assignement operator
        /**
        @param operand              : IN operand for the assignment operator
        @returns the reference to the current object
        */
        const SortAndFilter&        operator=(const SortAndFilter& operand);

        ///Function initialize the array and ignored check count
        /**
        @returns void
        */
        void                        Initialize();

        ///Original array
        MySQLRowExArray*            m_porigrowarray;

        //Count to keep the number of checked rows that are ignored while filtering
        wyInt32                     m_ignoredcheckcount;

        ///Number of sort columns
        wyInt32                     m_sortcolumns;
        
        ///Number of filter columns
        wyInt32                     m_filtercolumns;

        ///Sort data
        SORT_DATA*                  m_sort;
        
        ///Filter data
        FILTER_DATA*                m_filter;

        ///Whether there is a filter applied
        wyBool                      m_isfilter;

        ///Parent window handle for custom filter dialog
        HWND                        m_hwndparent;
        
        ///Custom filter dialog handle
        HWND                        m_hwnd;
        
        ///Rectangle with respect to which the array to be positioned
        RECT*                       m_prelrect;

        ///Cell data
        wyChar*                     m_celldata;

        ///Cell data length
        wyUInt32                    m_celldatalen;

        ///Data to which the object is associated with
        MySQLDataEx*                m_data;

        ///Selected column in grid
        wyInt32                     m_colno;

        ///MDI window pointer
        MDIWindow*                  m_wnd;

        ///mysql field type
        enum_field_types            m_coltype;

        ///Is client side sort/filter
        wyBool                      m_isclient;

        ///Filter string to be processed
        wyString                    m_filterstring;

        ///Current filter string
        wyString                    m_currfilterstring;

        wyBool                      m_isfilteronnull;

		// IQueryBuilder interface
		IQueryBuilder*				m_querybuilder;
};

#endif