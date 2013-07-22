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

Author: Shubhansh

*********************************************/



#ifndef _CExportImportConnection_H
#define _CExportImportConnection_H

#include "FrameWindowHelper.h"
#include "Global.h"


//Constants defined for remembering user preference in case of conflict in connection name while importing connection details
#define REMEMBER    1
#define KEEPBOTH    4
#define REPLACE     8
#define SKIP        16

//Structure used to store connection name and corresponding section name,
//used for sorting and finding the existing connection names
struct ConDetails
{
    //wyWChar connname[100];      // holds Connection name
    wyWChar *conname;
    wyWChar ininame[15];        // holds Section name
};

//Class used for Exporting and Importing Connection Details.
class ExportImportConnection
{

private:

    // Variable holds the number of items in ListView Control
    wyInt32         m_LVitemcount;          

    // Variable holds the number of checked items in ListView Control
    wyInt32         m_LVitemchecked;        

    // Variable identifies whether dialog is Import or Export Connection Details Dialog. true for Import and false for export
    bool            m_isimport;             

    // Variable holds the number of Connections in SQLyog.ini
    wyInt32         m_ConInIni;  

    // Handle to ListView Control
    HWND            m_hwndLV;               

    // Holds the name of file from which Connection details needs to be transferred
    // SQLyog.ini in case of Export Connection Details
    // User Defined file in case of Import Connection Details
    wyString        m_fromfile;             
    
    // Holds the name of file to which Connection details needs to be transferred
    // User defined file in case of Export Connection Details
    // SQLyog.ini in case of Import Connection Details
    wyString        m_tofile;               
    
    // Holds index of last item checked in ListView Control. Used for implementing Shift+RightClick in case of multiple selection in sequence
    wyInt32         m_lastItemChecked;      

    // wyTrue if Multiple Selection operation is in progress else wyFalse 
    wyBool          m_isShiftOperation;

    // Holds the Default Export/Import Dialog RECT Coordinates 
    RECT            m_wndrect;

    //holds the section name from which data is to be tranferred.
    wyChar          m_fromSec[30];

public:
    
    /// Constructor
    /**
    @param isexport             : IN true if Import Dialog & false for Export Dialog
	*/
    ExportImportConnection(bool isimport);
    
    //Destructor
    ~ExportImportConnection();
    
    
    /// Creates the Export/Import Connection Details dialog
    /**
    @param hwnd             : handle of Parent Window/Dialog
    @param return           : Return value from Dialog
    */
    wyInt32                 ActivateConnectionManagerDialog(HWND hwnd);

    ///Standard window procedure for Export Connection Details dialog box
    /**
    @param hwnd             : IN window handle
    @param message          : IN message
    @param wParam           : IN message parameter
    @param lParam           : IN message parameter
    @returns TRUE if the message is handled, else FALSE
    */
    static INT_PTR CALLBACK ConnectionManagerDialogProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);

    ///Standard window procedure for Import Connection Details dialog box
    /**
    @param hwnd             : IN window handle
    @param message          : IN message
    @param wParam           : IN message parameter
    @param lParam           : IN message parameter
    @returns TRUE if the message is handled, else FALSE
    */
    static INT_PTR CALLBACK ConnectionManagerDialogProcImport(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);

    ///Standard window procedure for Conflicts in Connection Name in Import Connection Details dialog box
    /**
    @param hwnd             : IN window handle
    @param message          : IN message
    @param wParam           : IN message parameter
    @param lParam           : IN message parameter
    @returns TRUE if the message is handled, else FALSE
    */
    static INT_PTR CALLBACK DialogProcCnflct(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam);

    ///Function initializes the Export Connection Details dialog box
    /**
    @param hwnd             : IN dialog handle
    @param wParam           : IN message parameter
    @param lParam           : IN message parameter
    */
    void                    OnInitConnectionManagerDialog(HWND hwnd,WPARAM wParam,LPARAM lParam);

    ///Function initializes the Import Connection Details dialog box
    /**
    @param hwnd             : IN dialog handle
    @param wParam           : IN message parameter
    @param lParam           : IN message parameter
    */
    void                    OnInitConnectionImportDialog(HWND hwnd,WPARAM wParam,LPARAM lParam);

    ///Function handles the WM_COMMAND message of Export Connection Details Dialog Box
    /**
    @param hwnd             : IN dialog handle
    @param wParam           : IN message parameter
    @param lParam           : IN message parameter
    */
    void                    OnCommandConnectionManagerDialog(HWND hwnd,WPARAM wParam,LPARAM lParam);

    ///Function handles the WM_COMMAND message of Import Connection Details Dialog Box
    /**
    @param hwnd             : IN dialog handle
    @param wParam           : IN message parameter
    @param lParam           : IN message parameter
    */
    void                    OnCommandConnectionImportDialog(HWND hwnd,WPARAM wParam,LPARAM lParam);

    ///Function Loads data to ListView Control
    /***
    @param hwnd             : IN Dialog handle
    @returns wyTrue
    */
    wyBool                  LoadDataToLV(HWND hwnd);

    /// Function handles WM_NOTIFY messages of both Export and Import Connection Details Dialog
    /**
    @param hwnd             : IN dialog handle
    @param wParam           : IN Message parameters
    @param lParam           : IN Message parameters
    @return TRUE if message is handled else FALSE
    */
    BOOL                    OnNotifyConnectionManagerDialog(HWND hwnd,WPARAM wParam,LPARAM lParam);

    /// Helper Function to LoadDataToLV, adds individual rows to ListView Control
    /**
    @param hwnd             : IN Dialog handle
    @param cd               : IN Section Name id. e.g. if Section is "Connection 46" then cd holds 46. cd is set as lParam for each row of ListView Control
    @param connname         : IN pointer to connection name
    @param hostname         : IN pointer to host name
    @param username         : IN pointer to user name
    @param port             : IN pointer to port number
    */
    void                    AddRowsToListViewControl(HWND hwnd, wyInt32 cd, wyWChar *connname,wyWChar *hostname, wyWChar *username,wyWChar *port, wyBool isSSH, wyBool isSSL, wyBool isHTTP);

    /// Function adds Columns header to ListView Control
    /**
    @param hwnd             : IN dialog handle
    */
    void                    AddColumnsToListViewCtrl(HWND hwnd);

    /// Function to validate user defined file name. Function is used to check file name both in case of Export and Import Connection Dialog
    /**
    @param hwnd             : IN dialog handle to post error message if any
    @param filename         : IN/OUT contains filename to be validated. OUT: full valid file name
    */
    bool                    ValidateFileName(HWND hwnd, wyWChar *filename);

    /// Function to hold WM_CLOSE and WM_DESTROY messages
    /**
    @param hwnd             : IN dialog handle
    */
    void                    OnClose(HWND hwnd);

    /// Function opens the SaveAs dialog, in Export Connection Details, to get the filename to which connection details are to be exported
    /**
    @param hwnd             : IN dialog handle
    @param filename         : OUT Buffer to hold filename
    @param length           : IN length of the path of chosen file
    @return wyFalse if successfull else wyTrue otherwise
    */
    wyBool                  GetExpFile(HWND hwnd,wyWChar* filename,wyInt32 length);

    /// Function opens the Open File dialog, in Import Connection Details, to get the filename to which connection details are to be exported
    /**
    @param hwnd             : IN dialog handle
    @param filename         : OUT Buffer to hold filename
    @param length           : IN length of the path of chosen file
    @return wyFalse if successfull else wyTrue otherwise
    */
    wyBool                  GetImpFile(HWND hwnd,wyWChar* filename,wyInt32 length);

    ///Function to Quicksort data
    /**
    @param arr              : IN pointer to array of ConDetails
    @param lower            : IN lower bound of array
    @param upper            : IN upper bound of the array
    @param param            : IN true for sorting by connection name, false for sorting by section name
    */
    void                    QuickSort(ConDetails **arr,wyInt32 lower, wyInt32 upper,bool param);

    /// Helper Function for QuickSort to find index where to split
    /**
    @param arr              : IN pointer to array of ConDetails
    @param lower            : IN lower bound of array
    @param upper            : In upper bound of array
    @param param            : IN true if split considering Connection name, false if split considering section name
    @return Return index of split
    */
    wyInt32                 Split(ConDetails **arr,wyInt32 lower, wyInt32 upper,bool param);

    /// Function implements Binary search to search for item in array of Connection Details
    /**
    @param arr              : IN pointer to array of ConDetails
    @param name             : IN element to be searched
    @param lower            : IN lower bound of array
    @param upper            : IN upper bound of array
    @param param            : IN true if search in Connection Name, false if search in Section Names
    @return Returns the index of the item if found, else -1 on data absence
    */
    wyInt32                 SearchNameIfExisting(ConDetails **arr,wyWChar *name, wyInt32 lower, wyInt32 upper, bool param);

    /// Function to load data from SQlyog.ini for Import Connection Details dialog
    /**
    @param hwnd             : IN dialog handle
    @param arr_cdName       : OUT pointer to base address to array of ConDetails to be sorted by Connection Name
    @param arr_cdSec        : OUT pointer to base address to array of ConDetails to be sorted by Section Name
    @return Returns the number of connections loaded in array
    */
    wyInt32                 LoadSortedDataFromIni(HWND hwnd, ConDetails ***arr_cdName, ConDetails ***arr_cdSec);

    /// Function to import connection details from .sycs file to SQLyog.ini
    /**
    @param hwnd             : IN dialog handle
    @param noOfConnection   : IN/OUT noOf Connections in SQLyog.ini
    @param arr_cdName       : IN pointer to array of ConDetails sorted by Connection Name
    @param arr_cdSec        : IN pointer to array of ConDetails sorted by Section Name
    @return Returns wyTrue if successfull, wyFalse if user has pressed cancel in case of conflict
    */
    wyBool                  ImportDataToIni(HWND hwnd, wyInt32& noOfConnection,ConDetails **arr_cdName, ConDetails **arr_Sec);

    /// Function to check for Non repeating section name and return the updated connection number available(non conflicting)
    /**
    @param arr_cdSec        : IN pointer to array of ConDetails sorted by Section Name
    @param noOfConnection   : IN number of Connections in SQLyog.ini
    @param cono             : IN Connection no, e.g if section name is "Connection 46" then cono will be 46
    @return Returns the non conflicting connection number
    */
    wyInt32                 GetValidSectionName(ConDetails **arr_cdSec, wyInt32 noOfConnection, wyInt32 cono);

    /// Function to check for Non repeating section name and return the updated connection number available(non conflicting)
    /**
    @param arr_cdName       : IN pointer to array of ConDetails sorted by Connection Name
    @param buffer           : IN/OUT contains the Connection name and while returning holds the non conflicting connection name
    @param noOfConnection   : IN number of Connections in SQLyog.ini
    */
    void                    GetValidConnectionName(ConDetails **arr_cdName, wyWChar *buffer, wyInt32 noOfConnection);

    /// Function updates the array of pointer to ConDetails when new Connections are added to SQLyog.ini
    /**
    @param noOfConnection   : IN number of connections in SQLyog.ini
    @param arr              : IN pointer to array of ConDetails
    @param cd               : IN pointer to ConDetails to be inserted in array
    @param param            : IN true for array sorted by Connection Name, false for array sorted by Section Names
    */
    void                    UpdateArray(wyInt32 noOfConnection,ConDetails **arr,ConDetails *cd,bool param);

    /// Function to handle WM_GETMINMAXINFO message
    /**
    @param lparam           : IN message parameter
    */
    void                    OnWMSizeInfo(LPARAM lparam);

    /// Function handles the positioning of controls in Export Connection Dialog
    /**
    @param hwnd             : IN dialog handle
    @param lParam           : IN message parameter
    */
    void                    PositionControls_Export(HWND hwnd, LPARAM lParam);

    /// Function handles the positioning of controls in Import Connection Dialog
    /**
    @param hwnd             : IN dialog handle
    @param lParam           : IN message parameter
    */
    void                    PositionControls_Import(HWND hwnd, LPARAM lParam);

    /// Function handles WM_PAINT message
    /**
    @param hwnd             : IN Dialog Handle 
    */
    void                    OnPaint(HWND hwnd);

    /// Function handles the WM_SIZE message for Export Connection Details Dialog
    /**
    @param hwnd             : IN Dialog Handle
    @param lParam           : IN Message parameter
    */
    void                    OnResizeExport(HWND hwnd,LPARAM lParam);

    /// Function handles the WM_SIZE message for Import Connection Details Dialog
    /**
    @param hwnd             : IN Dialog Handle
    @param lParam           : IN Message parameter
    */
    void                    OnResizeImport(HWND hwnd,LPARAM lParam);

    ///Function handles the context help request
    /**
    @param ishelpclicked    : IN flag tells whether the function is invoked using the help button
    @returns void
    */
    void                    HandleHelp(wyBool ishelpclicked=wyFalse);

    /// Thread Function that handles Export Connection details operation
    /**
	@param lparam	: IN pass the parm struct passed from thread
	*/
    //static	unsigned __stdcall		ExportThread(LPVOID lparam);
    static      void    __cdecl         ExportThread(LPVOID lparam);

    /// Thread Function that handles Import Connection details operation
    /**
	@param lparam	: IN pass the parm struct passed from thread
	*/
    //static	unsigned __stdcall		ImportThread(LPVOID lparam);
    static      void    __cdecl         ImportThread(LPVOID lparam);



    ///Function performs the action when "Export" button is pressed in Export Connection Details Dialog
    /**
    @param hwnd             : IN dialog Handle
    */
    wyBool                  OnExportOkPressed(HWND hwnd);

    ///Function Colors alternate rows in Listview Control
    /**
    @param hwnd             : IN ListView Control Handle
    */
    void                    ColourAlternateRows(HWND hwnd);

    ///handle when mouse moves, mouse shows WAIT state other than over the 'Stop' button while comparing
	/**
	@param hwnd   : IN handle to the dialog
	@param lParam : IN message parameter
	@return void
	*/
    void			        OnMouseMove(HWND hwnd, LPARAM lParam);


    /// Handles painting of Listview Control
    /**
    @param lParam   : IN message parameter
    @return LRESULT
    */
    LRESULT                 ProcessCustomDraw (LPARAM lParam);

    
    // Holds the conflicted connection name to be shown in Conflict in connection name dialog 
    wyChar m_conflictName[MAX_PATH+1];
    
    // Shows that transfer of Connection Details is in progress
    wyBool m_isTransfer;
    
    // wyTrue if initialisation process is in progress
    wyBool m_initcompleted;

    // wyTrue if Connection Details transfer is stopped, else wyFalse
    wyBool m_isStop;

    // keeps the number of conflicts dealt
    wyUInt32 m_conflictNo;

    // Handle for synchronization event
    HANDLE   m_syncevent;

    // wyTrue if Dialog is to be closed, else wyFalse
    wyBool   m_endDialog;

    // wyTrue if check box in conflict resolution dialog is checked else wyFalse 
    wyBool  m_checkAll;
};
#endif
