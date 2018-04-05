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

Author: Shubhansh

*********************************************/

#include "ExportImportConnectionDetails.h"

//Constructor. isimport used to determine whether dialog is Export/Import Connection Dialog
ExportImportConnection::ExportImportConnection(bool isimport)
{
    m_LVitemcount       = 0;
    m_LVitemchecked     = 0;
    m_isimport          = isimport;
    m_ConInIni          = 0;
    m_hwndLV            = NULL;
    m_isTransfer        = wyFalse;
    m_isStop            = wyFalse;
    m_lastItemChecked   = -1;
    m_isShiftOperation  = wyFalse;
    m_initcompleted     = wyFalse;
    m_endDialog         = wyFalse;
    m_conflictNo        = 0;
    m_checkAll          = wyTrue;
    memset(&m_wndrect, 0, sizeof(RECT));
    memset(m_fromSec, 0, 20);
    memset(m_conflictName,0,MAX_PATH);
    m_fromfile.SetAs("");
    m_tofile.SetAs("");
}

//Destructor
ExportImportConnection::~ExportImportConnection(void)
{
    
    
}



// Adds the header columns in the ListView Control
void
ExportImportConnection::AddColumnsToListViewCtrl(HWND hwnd)
{
    LVCOLUMN lvc={0};

    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    
    lvc.iSubItem=0;
    lvc.pszText=_(L"     Connection Name");
    lvc.cx=171;
    lvc.fmt=LVCFMT_LEFT;
    
    ListView_InsertColumn(hwnd,0,&lvc);

    lvc.iSubItem=1;
    lvc.pszText=_(L"Host");
    lvc.cx=105;
    lvc.fmt=LVCFMT_LEFT;
    ListView_InsertColumn(hwnd,1,&lvc);

    lvc.iSubItem=2;
    lvc.pszText=_(L"Username");
    lvc.cx=80;
    lvc.fmt=LVCFMT_LEFT;
    ListView_InsertColumn(hwnd,2,&lvc);

    lvc.iSubItem=3;
    lvc.pszText=_(L"Port");
    lvc.cx=70;
    lvc.fmt=LVCFMT_CENTER;
    ListView_InsertColumn(hwnd,3,&lvc);

    lvc.iSubItem=4;
    lvc.pszText=_(L"Connection type");
    lvc.cx=100;
    lvc.fmt=LVCFMT_CENTER;
    ListView_InsertColumn(hwnd,4,&lvc);
}

// function adds rows to ListView Control
void
ExportImportConnection::AddRowsToListViewControl(HWND hwnd, wyInt32 cd, wyWChar *connname,wyWChar *hostname, wyWChar *username,wyWChar *port, wyBool isSSH, wyBool isSSL, wyBool isHTTP)
{
    LVITEM LvItem;
    wyWChar *strSSL = L"SSL";
    wyWChar *strHTTP  = L"HTTP";
    wyWChar *strSSH = L"SSH";
    wyWChar *strDirect = L"Direct";
    
    memset(&LvItem,0,sizeof(LvItem));
    
    LvItem.mask=LVIF_TEXT|LVIF_PARAM;                                   // Text Style
    LvItem.cchTextMax = 256;                                            // Max size of text
    LvItem.iItem=0;                                                     // choose item  
    LvItem.iSubItem=0;                                                  // Put in first coluom
    LvItem.pszText=connname;                                            // Text to display (can be from a char variable) (Items)
    LvItem.lParam=(LPARAM)cd;                                           // cd is connection number. e.g. if section in file is named as "Connection 46" then cd contains only 46
    LvItem.iItem=SendMessage(hwnd,LVM_INSERTITEM,0,(LPARAM)&LvItem);    // Send info to the Listview
    
    // Enter text to SubItems
    LvItem.mask=LVIF_TEXT;
    LvItem.iSubItem=1;
    LvItem.pszText=hostname;
    SendMessage(hwnd,LVM_SETITEM,0,(LPARAM)&LvItem);
        
    LvItem.iSubItem=2;
    LvItem.pszText=username;
    SendMessage(hwnd,LVM_SETITEM,0,(LPARAM)&LvItem);
        
    LvItem.iSubItem=3;
    LvItem.pszText=port;
    SendMessage(hwnd,LVM_SETITEM,0,(LPARAM)&LvItem);

    LvItem.iSubItem=4;
    if(isSSH == wyTrue)
    {
        LvItem.pszText = strSSH;
    }
    else if(isSSL == wyTrue)
    {
        LvItem.pszText = strSSL;
    }
    else if(isHTTP == wyTrue)
    {
        LvItem.pszText = strHTTP;
    }
    else
        LvItem.pszText = strDirect;

    SendMessage(hwnd,LVM_SETITEM,0,(LPARAM)&LvItem);

    LvItem.stateMask = LVIS_STATEIMAGEMASK;
    LvItem.state = INDEXTOSTATEIMAGEMASK(2);
    SendMessage(hwnd, LVM_SETITEMSTATE, LvItem.iItem, (LPARAM)&LvItem);
    
}

//function handles the WM_INITCOMMAND message and initialises the content of Export Connection Details Dialog. 
void
ExportImportConnection::OnInitConnectionManagerDialog(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
    wyWChar     directory[MAX_PATH + 1]={0}, *lpfileport=0;
    wyUInt32	ret = 0;
	wyInt32     cono = 0;
    wyString    dirstr, allsecnames, connnamestr, connhostnamestr, connusernamestr, connportstr;
	wyChar      *allsectionnames = NULL,seps[] = ";", *tempconsecname = NULL;
    const wyChar *file = NULL;
    wyBool      isSSH = wyFalse;
    wyBool      isHTTP = wyFalse;
    wyBool      isSSL = wyFalse;
    wyInt32     i=0;
    HICON       hicon = NULL;

    GetWindowRect(hwnd,&m_wndrect);
    ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
    if(ret == 0)
        return;
    
    //Place  SQLyog icon at top of Dialog
    hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
    DestroyIcon(hicon);

    m_fromfile.SetAs(directory);
    
    dirstr.SetAs(directory);
	file=dirstr.GetString();
    
    m_hwndLV=GetDlgItem(hwnd,IDC_CONNECTION_LIST);
    ListView_SetExtendedListViewStyle(m_hwndLV, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    
    //Retrieve all section names from ini
    m_ConInIni=wyIni::IniGetSection(&allsecnames, &dirstr);
    allsectionnames = (wyChar*)allsecnames.GetString();
    tempconsecname = strtok(allsectionnames, seps);
	
    if(m_ConInIni > 0)
    {
        EnableWindow(m_hwndLV, TRUE);
        AddColumnsToListViewCtrl(m_hwndLV);
        while(tempconsecname)
        {
            //Flooding structure with details
            wyIni::IniGetString(tempconsecname, "Name", "", &connnamestr, file);
            wyIni::IniGetString(tempconsecname,"Host","",&connhostnamestr,file);
            wyIni::IniGetString(tempconsecname,"User","",&connusernamestr,file);
            wyIni::IniGetString(tempconsecname,"Port","",&connportstr,file);
            isSSH = (wyIni::IniGetInt(tempconsecname, "SSH", 0, file))?(wyTrue):(wyFalse);
	        isHTTP = (wyIni::IniGetInt(tempconsecname, "Tunnel", 0, file))?(wyTrue):(wyFalse);
            isSSL = wyIni::IniGetInt(tempconsecname, "SslChecked", 0, file)?(wyTrue):(wyFalse);
            cono=atoi(tempconsecname+10);
            
            //If name is there in section then add the corresponding details to listview control
            if(connnamestr.Compare("") != 0)
            {
                AddRowsToListViewControl(m_hwndLV, cono, 
                    connnamestr.GetAsWideChar(), 
                    connhostnamestr.GetAsWideChar(), 
                    connusernamestr.GetAsWideChar(), 
                    connportstr.GetAsWideChar(), 
                    isSSH, isSSL, isHTTP);
                ++i;
            }
            tempconsecname = strtok(NULL, seps);
        }
        if(i > 0)
        {
            m_LVitemcount=i;
            m_LVitemchecked=i;
            Button_SetCheck(GetDlgItem(hwnd,IDC_CHKUNCHK),true);
        }
        else
        {
            EnableWindow(GetDlgItem(hwnd,IDC_CHKUNCHK), FALSE);
            EnableWindow(m_hwndLV,FALSE);
            EnableWindow(GetDlgItem(hwnd,IDC_CONNECTION_FILE), FALSE);
            EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_CONNECTION), FALSE);
        }
        EnableWindow(GetDlgItem(hwnd,IDOK),FALSE);
    }
    else
    {
        EnableWindow(m_hwndLV,FALSE);
    }

    //Set dialog position as it was when closed last time
    SetWindowPositionFromINI(hwnd, EXPORTCONNECTIONDETAILS_SECTION, m_wndrect.right - m_wndrect.left, m_wndrect.bottom - m_wndrect.top);
    PositionControls_Export(hwnd,NULL);
}

//Validates filename and get full path.
bool
ExportImportConnection::ValidateFileName(HWND hwnd, wyWChar *filename)
{   
    wyChar      fn[MAX_PATH+1]={0};
    HANDLE      hfile=NULL;
    wyWChar     *req=NULL;
    wyInt32         x=0;
    wyString    str(filename);

    str.LTrim();
    str.RTrim();
    
    //Check if filename is not empty.
    if(wcscmp(str.GetAsWideChar(),L"") != 0)
    {
        x=wcslen(filename);
        
        //Check for .sycs extension in end
        // Check if file name is less than 6 characters.
        //If filename length is less than 6 characters then it is sure that it is not having .sycs extension
        if(x < 6)
        {
            wcscat(filename,L".sycs");
        }
        else
        {
            req=filename+wcslen(filename)-5;
            if(wcscmp(req,L".sycs") != 0)
            {
                wcscat(filename,L".sycs");
            }
        }
        
        ::wcstombs(fn,filename,MAX_PATH);
        
        strcpy(fn,_fullpath(NULL,fn,MAX_PATH));
        mbstowcs(filename,fn,MAX_PATH);
        
        if(m_isimport)
        {
            hfile   =  CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	        
            //Check if file exists
            if(hfile == INVALID_HANDLE_VALUE)
            {
		        DisplayErrorText(GetLastError(), _("Could not open file."));
                CloseHandle(hfile);
		        return false;
	        }
            else
            {
                CloseHandle(hfile);
                return true;
            }
        }
        else
        {
            // Ask user to overwrite the file if exists
            if(OverWriteFile(hwnd,filename) == wyTrue)
            {
                hfile   =   CreateFile(filename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,NULL,NULL);
                if(hfile == INVALID_HANDLE_VALUE)
                {
		            DisplayErrorText(GetLastError(), _("Could not open file."));
                    CloseHandle(hfile);
		            return false;
	            }
                else
                {
                    CloseHandle(hfile);
                    return true;
                }
            }
            else
            {
                return false;
            }
        }
    }
    yog_message(hwnd, _(L"Invalid Filename!"),pGlobals->m_appname.GetAsWideChar(),MB_ICONERROR|MB_OK);
    return false;
}

// Shows dialog box for asking filename to be saved
wyBool 
ExportImportConnection::GetExpFile(HWND hwnd,wyWChar* filename,wyInt32 length)
{
    OPENFILENAME	openfilename;

	memset(&openfilename, 0, sizeof(openfilename));

	openfilename.lStructSize       = OPENFILENAME_SIZE_VERSION_400;
	openfilename.hwndOwner         = hwnd;
	openfilename.hInstance         = pGlobals->m_pcmainwin->GetHinstance();
    openfilename.lpstrFilter       = CONMANFILE;
	openfilename.lpstrDefExt       = L"sycs";
    openfilename.lpstrCustomFilter =(LPWSTR)NULL;
	openfilename.nFilterIndex      = 1L;
	openfilename.lpstrFile         = filename;
	openfilename.nMaxFile          = length;
	openfilename.lpstrTitle        = _(L"Export As");
	openfilename.Flags             = OFN_HIDEREADONLY;
	openfilename.lpfnHook          =(LPOFNHOOKPROC)(FARPROC)NULL;
	openfilename.lpTemplateName    =(LPWSTR)NULL;
	
	if(GetSaveFileName(&openfilename))
	{
		return wyFalse;
	}

	return wyTrue;
}

//Shows dialog for asking filename to be opened
wyBool 
ExportImportConnection::GetImpFile(HWND hwnd,wyWChar* filename,wyInt32 length)
{
    OPENFILENAME	openfilename;

	memset(&openfilename, 0, sizeof(openfilename));

	openfilename.lStructSize       = OPENFILENAME_SIZE_VERSION_400;
	openfilename.hwndOwner         = hwnd;
	openfilename.hInstance         = pGlobals->m_pcmainwin->GetHinstance();
    openfilename.lpstrFilter       = CONMANFILE;
	openfilename.lpstrDefExt       = L"sycs";
    openfilename.lpstrCustomFilter =(LPWSTR)NULL;
	openfilename.nFilterIndex      = 1L;
	openfilename.lpstrFile         = filename;
	openfilename.nMaxFile          = length;
	openfilename.lpstrTitle        = _(L"Load File");
	openfilename.Flags             = OFN_HIDEREADONLY;
	openfilename.lpfnHook          =(LPOFNHOOKPROC)(FARPROC)NULL;
	openfilename.lpTemplateName    =(LPWSTR)NULL;
	
	if(GetOpenFileName(&openfilename))
	{
       return wyFalse;
	}

	return wyTrue;
}

//Handles the Export Conenction Details Operation when "Export" Button is pressed from Dialog box
wyBool
ExportImportConnection::OnExportOkPressed(HWND hwnd)
{
    LVITEM          li;
    wyWChar         filename[MAX_PATH+1]={0};
    wyChar          dir[MAX_PATH+1];
    wyInt32         cono=0;
    wyString        tempString;
    wyWChar wc_secname[15] = {0};
    wyChar  secname[30] = {0};
    wyWChar  conName[MAX_PATH+1] = {0};
    wyString statusText("");
                
    li.mask=LVIF_PARAM | LVIF_TEXT;
    li.iSubItem=0;
    li.pszText = conName;
    li.cchTextMax = MAX_PATH;

            
    GetWindowText(GetDlgItem(hwnd,IDC_CONNECTION_FILE),filename,MAX_PATH);
    if(ValidateFileName(hwnd,filename) && m_isStop == wyFalse)
    {
        //Check if no item is selected in Listview control
        if(m_LVitemchecked <= 0)
        {
            MessageBox(hwnd,_(L"No Connection Detail Selected for Export"),pGlobals->m_appname.GetAsWideChar(),MB_OK|MB_ICONINFORMATION);
            return wyFalse;
        }
            
        wcstombs(dir,filename,MAX_PATH);

        m_tofile.SetAs(dir);
        m_isTransfer=wyTrue;
        for(int i=0;i<m_LVitemcount && m_isStop == wyFalse;i++)
        {
            //Check checkbox state for each row and export the connections that are checked
            if(ListView_GetCheckState(m_hwndLV,i) != 0)
            {
                li.iItem=i;
                SendMessage(m_hwndLV,LVM_GETITEM,0,(LPARAM)&li);
                cono=(wyInt32)li.lParam;

                tempString.Sprintf("Connection %d",cono);
                wcscpy(wc_secname,tempString.GetAsWideChar());
                wcstombs(secname,wc_secname,30);

                tempString.SetAs(li.pszText);

                //Set status text
                statusText.Sprintf(_("Transferring %s"),tempString.GetString());
                SetWindowText(GetDlgItem(hwnd,IDC_STATUS), statusText.GetAsWideChar());
                
                // Transfer full section details from ini to export file
                wyIni::IniTransferFullSection(secname, secname, m_fromfile.GetString(), m_tofile.GetString());
            }
        }

        //After exporting details hide set status text to nothing
        SetWindowText(GetDlgItem(hwnd,IDC_STATUS), L"");
        m_isTransfer=wyFalse;
        if(m_isStop == wyFalse)
        {
            MessageBox(hwnd,_(L"Connection(s) exported successfully."),pGlobals->m_appname.GetAsWideChar(),MB_OK|MB_ICONINFORMATION);
            return(wyTrue);
        }
        else
        {
            return wyFalse;
        }
    }
    return wyFalse;
}


// Import Thread that handles Importing of connection details from .sycs file to ini file
void
ExportImportConnection::ImportThread(LPVOID lparam)
{
    HWND                    hwnd   =   (HWND)lparam;
    ExportImportConnection  *instance = (ExportImportConnection *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
    HMENU                   hMenu =   NULL;
    wyInt32                 noOfConnections=0;
    ConDetails              **arr_cdName=NULL;
    ConDetails              **arr_cdSec=NULL;
    wyBool                  ret = wyFalse;

    hMenu=GetSystemMenu(hwnd,FALSE);
    EnableMenuItem(hMenu,SC_CLOSE,MF_BYCOMMAND|MF_GRAYED);
    EnableWindow(GetDlgItem(hwnd,IDCANCEL),FALSE);
    EnableWindow(GetDlgItem(hwnd,IDC_CONNECTION_FILE),FALSE);
    EnableWindow(GetDlgItem(hwnd,IDC_LOAD_FILE),FALSE);
    EnableWindow(GetDlgItem(hwnd,IDC_CONNECTION_LIST),FALSE);
    EnableWindow(GetDlgItem(hwnd,IDC_CHKUNCHK),FALSE);
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_CONNECTION),FALSE);
    SetWindowText(GetDlgItem(hwnd,IDOK),_(L"&Stop"));
    
    instance->m_isTransfer = wyTrue;
    
    //Load existing connection details from ini. Required for checking of conflicts in the connection name and section name
    if(instance->m_isStop == wyFalse)
    {
        noOfConnections=instance->LoadSortedDataFromIni(hwnd,&arr_cdName,&arr_cdSec); //arr_cdName=> array of pointers sorted in order of connection names
                                                                                      //arr_cdSec=> array of pointers sorted in order of section names
    }

    //Import data from .sycs file to ini
    if(instance -> m_isStop == wyFalse && noOfConnections != -1)
    {
        ret=instance->ImportDataToIni(hwnd, noOfConnections,arr_cdName,arr_cdSec);
    }
    else if(noOfConnections == -1)
    {   
            yog_message(hwnd,_(L"Memory not sufficient to complete task!"),pGlobals->m_appname.GetAsWideChar(),MB_OK | MB_ICONERROR);
    }

    SetWindowText(GetDlgItem(hwnd,IDC_STATUS),L"");

    for(wyInt32 x=0;x<noOfConnections;x++)
    {
        free(arr_cdName[x]->conname);
        free(arr_cdName[x]);
        arr_cdName[x] = NULL;
        arr_cdSec[x] = NULL;
    }

    free(arr_cdName);
    free(arr_cdSec);

    EnableDisableExportMenuItem();

    if(ret == wyTrue)
    {
        MessageBox(hwnd,_(L"Connection(s) imported successfully."),pGlobals->m_appname.GetAsWideChar(),MB_OK|MB_ICONINFORMATION);
        instance->m_endDialog=wyTrue;
    }
    else
    {
        EnableMenuItem(hMenu,SC_CLOSE,MF_BYCOMMAND|MF_ENABLED);
        EnableWindow(GetDlgItem(hwnd,IDCANCEL),TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_LOAD_FILE),TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_CONNECTION_FILE),TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_CONNECTION_LIST),TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_CHKUNCHK),TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_CONNECTION),TRUE);
        EnableWindow(GetDlgItem(hwnd,IDOK),TRUE);
        SetWindowText(GetDlgItem(hwnd,IDOK),_(L"&Import"));
        instance->m_isStop = wyFalse;
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));

    instance->m_isTransfer=wyFalse;
    SetEvent(instance->m_syncevent);
    _endthread();
}


// Handles the export connection details.
void 
ExportImportConnection::ExportThread(LPVOID lparam)
{
    HWND hwnd=(HWND)lparam;
    ExportImportConnection *instance=(ExportImportConnection *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
    HMENU hMenu = NULL;

    hMenu=GetSystemMenu(hwnd,FALSE);
    
    EnableMenuItem(hMenu,SC_CLOSE,MF_BYCOMMAND|MF_GRAYED);
    EnableWindow(GetDlgItem(hwnd,IDCANCEL),FALSE);
    EnableWindow(GetDlgItem(hwnd,IDC_CHKUNCHK),FALSE);
    EnableWindow(GetDlgItem(hwnd,IDC_CONNECTION_LIST),FALSE);
    EnableWindow(GetDlgItem(hwnd,IDC_CONNECTION_FILE),FALSE);
    EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_CONNECTION),FALSE);
    SetWindowText(GetDlgItem(hwnd,IDOK),_(L"&Stop"));
    
    if(instance->OnExportOkPressed(hwnd) == wyTrue)
    {
        EndDialog(hwnd,IDOK);
    }
    else
    {
        EnableMenuItem(hMenu,SC_CLOSE,MF_BYCOMMAND|MF_ENABLED);
        EnableWindow(GetDlgItem(hwnd,IDCANCEL),TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_CHKUNCHK),TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_CONNECTION_LIST),TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_CONNECTION_FILE),TRUE);
        EnableWindow(GetDlgItem(hwnd,IDC_BUTTON_CONNECTION),TRUE);
        EnableWindow(GetDlgItem(hwnd,IDOK),TRUE);
        SetWindowText(GetDlgItem(hwnd,IDOK),_(L"&Export"));
        instance->m_isStop=wyFalse;
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
    _endthread();
    
}


// Handles the WM_COMMAND message of the Export Connection Details Dialog box
void
ExportImportConnection::OnCommandConnectionManagerDialog(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
    wyWChar     str[270];
    wyString    stri;
    
    switch(LOWORD(wParam))
	{
    case IDOK:
        if(m_isTransfer == wyFalse && m_isStop == wyFalse)
        {
            _beginthread(ExportImportConnection::ExportThread,0,hwnd);
            
        }
        else
        {
            m_isStop=wyTrue;
            EnableWindow(GetDlgItem(hwnd,IDOK),FALSE);
        }
        break;
    case IDCANCEL:
        EndDialog(hwnd,IDCANCEL);
        break;
    case IDC_BUTTON_CONNECTION:
        {
        wyWChar     filename[MAX_PATH+1]={0};
        if(GetExpFile(hwnd,filename,MAX_PATH) == wyFalse)
            SetWindowText(GetDlgItem(hwnd,IDC_CONNECTION_FILE),filename);
        }
        break;
    case IDC_CHKUNCHK:
        if(HIWORD(wParam) == BN_CLICKED)
        {
            int state=SendMessage(GetDlgItem(hwnd,IDC_CHKUNCHK),BM_GETCHECK,0,0);
            switch(state)
            {
            case BST_CHECKED:
                {
                LVITEM LvItem;
                LvItem.stateMask = LVIS_STATEIMAGEMASK;
                LvItem.state = INDEXTOSTATEIMAGEMASK(2);
                SendMessage(GetDlgItem(hwnd,IDC_CONNECTION_LIST), LVM_SETITEMSTATE, -1, (LPARAM)&LvItem);
                }
                break;
            case BST_UNCHECKED:
                {
                LVITEM LvItem;
                LvItem.stateMask = LVIS_STATEIMAGEMASK;
                LvItem.state = INDEXTOSTATEIMAGEMASK(1);
                SendMessage(GetDlgItem(hwnd,IDC_CONNECTION_LIST), LVM_SETITEMSTATE, -1, (LPARAM)&LvItem);
                }
                break;
            }
        }
        break;
    case IDC_CONNECTION_FILE:
        {
            if(HIWORD(wParam) == EN_CHANGE)
            {
                GetWindowText(GetDlgItem(hwnd,IDC_CONNECTION_FILE), str, 269);
                stri.SetAs(str);
                stri.LTrim();
                stri.RTrim();
                if(stri.GetLength() > 0 && m_LVitemchecked)
                    EnableWindow(GetDlgItem(hwnd,IDOK),TRUE);
                else
                    EnableWindow(GetDlgItem(hwnd,IDOK),FALSE);
            }  
        }
        break;
    }
}

// Handles the WM_NOTIFY messages of both Export/Import Connection Dialogs
BOOL
ExportImportConnection::OnNotifyConnectionManagerDialog(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
    NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)lParam;
    LPNMHDR lphdr=(LPNMHDR)lParam;
    BOOL lastcheckstate=FALSE;
    BOOL checkstate=FALSE;
    wyInt32 i=0,j=0;
    wyWChar str[271]={0};
    wyString stri;

    //Check if notification is from ListView Control
    if(lphdr->hwndFrom == GetDlgItem(hwnd,IDC_CONNECTION_LIST) && m_LVitemcount>0)
    {
        //Check if notification message is for some item changed
        if(lphdr->code == LVN_ITEMCHANGED)
        {
            //Check for change in checkbox state of the item
            if((pNMListView->uNewState & LVIS_STATEIMAGEMASK) != (pNMListView->uOldState & LVIS_STATEIMAGEMASK))
            {
                //Check if shift key is pressed and aggregated shift click check operation is in progress
                if(m_isShiftOperation != wyTrue && (GetKeyState(VK_SHIFT) & SHIFTED) && m_lastItemChecked != -1 && m_lastItemChecked != pNMListView->iItem)
                {
                    //to have check if aggregate shift click check operation is in progress
                    m_isShiftOperation = wyTrue;

                    //Get the check states of last item checked and current item checked
                    lastcheckstate=ListView_GetCheckState(GetDlgItem(hwnd,IDC_CONNECTION_LIST),m_lastItemChecked);
                    checkstate= !ListView_GetCheckState(GetDlgItem(hwnd,IDC_CONNECTION_LIST),pNMListView->iItem);
                    
                    //initailise the values of i and j so that looping can be done accordingly
                    if(m_lastItemChecked > pNMListView->iItem)
                    {
                        i = pNMListView->iItem;
                        j = m_lastItemChecked;
                    }
                    else
                    {
                        i = m_lastItemChecked;
                        j = pNMListView->iItem;
                    }
                    
                    //Determine the check states to be assigned
                    if(lastcheckstate == checkstate)
                    {
                        checkstate = !checkstate;
                    }
                    else
                    {
                        checkstate = lastcheckstate;
                    }

                    //loop through the indexes and set the check state
                    while(i <= j)
                    {
                    ListView_SetCheckState(GetDlgItem(hwnd,IDC_CONNECTION_LIST), i, checkstate);
                    i++;
                    }

                    m_isShiftOperation=wyFalse;
                }

                m_lastItemChecked=pNMListView->iItem;

                //update the number of items checked in the listview control
                if(ListView_GetCheckState(GetDlgItem(hwnd,IDC_CONNECTION_LIST),pNMListView->iItem) == 0)
                    m_LVitemchecked--;
                else
                    m_LVitemchecked++;

                //Set the state of Select/deselect all check box button
                SendMessage(GetDlgItem(hwnd,IDC_CHKUNCHK),BM_SETCHECK,m_LVitemchecked == m_LVitemcount?BST_CHECKED:BST_UNCHECKED,0);
                
                //Enable or Disable IDOK button if atleast one of the rows is checked or export filename is present
                if(m_isimport)
                {
                    if(m_LVitemchecked == 0)
                        EnableWindow(GetDlgItem(hwnd,IDOK),FALSE);
                    else
                        EnableWindow(GetDlgItem(hwnd,IDOK),TRUE);
                }
                else
                {
                    if(m_LVitemchecked == 0)
                    {
                        EnableWindow(GetDlgItem(hwnd,IDOK),FALSE);
                    }
                    else //if(SendMessage(GetDlgItem(hwnd,IDC_CONNECTION_FILE),WM_GETTEXTLENGTH,0,0) != 0)
                    {
                        GetWindowText(GetDlgItem(hwnd,IDC_CONNECTION_FILE), str,270);
                        stri.SetAs(str);
                        stri.LTrim();
                        stri.RTrim();
                        if(stri.GetLength() != 0)
                            EnableWindow(GetDlgItem(hwnd,IDOK),TRUE);
                    }
                }
                return TRUE;
            }
        }
        else if(lphdr->code == NM_CUSTOMDRAW)
        {
            SetWindowLongPtr(hwnd, DWLP_MSGRESULT,(LONG_PTR)ExportImportConnection::ProcessCustomDraw(lParam));
            return TRUE;
        }
    }
    return FALSE;
}

//coloring alternate rows in listview control
LRESULT 
ExportImportConnection::ProcessCustomDraw (LPARAM lParam)
{
    LPNMLVCUSTOMDRAW lplvcd = (LPNMLVCUSTOMDRAW)lParam;
    
    switch(lplvcd->nmcd.dwDrawStage) 
    {
    case CDDS_PREPAINT : 
        return CDRF_NOTIFYITEMDRAW;
        break;    
    case CDDS_ITEMPREPAINT: //Before an item is drawn
        if (((int)lplvcd->nmcd.dwItemSpec%2)==0)
        {
            lplvcd->clrText   = RGB(0,0,0);
            lplvcd->clrTextBk = RGB(230,230,230);
            return CDRF_NEWFONT;
        }
        else
        {
            lplvcd->clrText   = RGB(0,0,0);
            lplvcd->clrTextBk = RGB(255,255,255);
            return CDRF_NEWFONT;
        }
        break;
    }
    return CDRF_DODEFAULT;
}


//Handles WM_DESTROY, WM_CLOSE message 
void 
ExportImportConnection::OnClose(HWND hwnd)
{
    LVITEM li;
    li.mask=LVIF_PARAM;
    li.iSubItem=0;
    
    m_LVitemcount=0;
    m_LVitemchecked=0;
}


//Dialog Procedure to handle  Import Connection Details Dialog
INT_PTR CALLBACK
ExportImportConnection::ConnectionManagerDialogProcImport(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    ExportImportConnection *ExportImportInstance=(ExportImportConnection *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
    RECT rc;
    memset(&rc, 0, sizeof(rc));

    switch(message)
    {
    case WM_INITDIALOG:
        ExportImportInstance=(ExportImportConnection*)lParam;
        SetWindowLongPtr(hwnd,GWLP_USERDATA,lParam);
        LocalizeWindow(hwnd);
        ExportImportInstance->OnInitConnectionImportDialog(hwnd,wParam,lParam);
        ExportImportInstance->m_initcompleted = wyTrue;
        break;
    case WM_COMMAND:
        ExportImportInstance->OnCommandConnectionImportDialog(hwnd,wParam,lParam);
        break;
    case WM_NOTIFY:
        return ExportImportInstance->OnNotifyConnectionManagerDialog(hwnd,wParam,lParam);
        break;
    case WM_PAINT:
        DrawSizeGripOnPaint(hwnd);
        ExportImportInstance->OnPaint(hwnd);
        break;
    case WM_DESTROY:
        ExportImportInstance->OnClose(hwnd);
        if(ExportImportInstance->m_initcompleted == wyTrue)
        {
            SaveInitPos(hwnd, IMPORTCONNECTIONDETAILS_SECTION);
        }
        EndDialog(hwnd,IDCANCEL);
        break;
    case WM_GETMINMAXINFO:
        ExportImportInstance->OnWMSizeInfo(lParam);
        return 0;
        break;
    case WM_SIZE:
        if(wParam == SIZE_RESTORED)
        {
            ExportImportInstance->OnResizeImport(hwnd,lParam);
            break;
        }
        return FALSE;
        break;
    case WM_HELP:
        ExportImportInstance->HandleHelp();
        break;
    case WM_MOUSEMOVE:
        ExportImportInstance->OnMouseMove(hwnd, lParam);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}



// handles the WM_INITCOMMAND message for Import Connection Details Dialog
void
ExportImportConnection::OnInitConnectionImportDialog(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
    wyWChar directory[MAX_PATH]= {0},*lpfileport=0;
    wyUInt32 ret=0;
    HICON    hicon = NULL;

    EnableWindow(GetDlgItem(hwnd,IDC_CONNECTION_LIST),FALSE);
    EnableWindow(GetDlgItem(hwnd,IDOK),FALSE);
    EnableWindow(GetDlgItem(hwnd,IDC_CHKUNCHK),FALSE);
    
    ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
    if(ret == 0)
    {
        yog_message(hwnd,_(L"Can't find sqlyog.ini!!!"),pGlobals->m_appname.GetAsWideChar(),MB_ICONERROR|MB_OK);
        return;
    }

    hicon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN));
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
    DestroyIcon(hicon);

    m_tofile.SetAs(directory);
    m_hwndLV=GetDlgItem(hwnd,IDC_CONNECTION_LIST);
    ListView_SetExtendedListViewStyle(m_hwndLV, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    

    GetWindowRect(hwnd,&m_wndrect);
    SetWindowPositionFromINI(hwnd, IMPORTCONNECTIONDETAILS_SECTION, m_wndrect.right - m_wndrect.left, m_wndrect.bottom - m_wndrect.top);
    PositionControls_Import(hwnd,NULL);
}

//Handles the WM_COMMAND message for Import Connection Details Dialog
void
ExportImportConnection::OnCommandConnectionImportDialog(HWND hwnd,WPARAM wParam,LPARAM lParam)
{
    wyWChar fromfilename[MAX_PATH+1]={0};
    
    
    switch(LOWORD(wParam))
    {
    case IDC_LOAD_FILE:
        if(GetImpFile(hwnd,fromfilename,MAX_PATH) == wyFalse)
        {
            if(ValidateFileName(hwnd,fromfilename))
            {
                m_fromfile.SetAs(fromfilename);
                if(LoadDataToLV(hwnd) == wyFalse || m_LVitemchecked == 0)
                {
                    m_LVitemchecked = 0;
                    m_LVitemcount = 0; 
                    ListView_DeleteAllItems(m_hwndLV);
                    ListView_DeleteColumn(m_hwndLV,0);
                    ListView_DeleteColumn(m_hwndLV,0);
                    ListView_DeleteColumn(m_hwndLV,0);
                    ListView_DeleteColumn(m_hwndLV,0);
                    ListView_DeleteColumn(m_hwndLV,0);
                    EnableWindow(m_hwndLV,FALSE);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHKUNCHK),FALSE);
                    EnableWindow(GetDlgItem(hwnd,IDOK),FALSE);
                    yog_message(hwnd,_(L"Invalid File!"),pGlobals->m_appname.GetAsWideChar(),MB_ICONERROR|MB_OK);
                }
                else
                {
                    EnableWindow(GetDlgItem(hwnd,IDC_CONNECTION_LIST),TRUE);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHKUNCHK),TRUE);
                    EnableWindow(GetDlgItem(hwnd,IDOK),TRUE);
                }
            }
            else
            {
                if(m_LVitemcount != 0)
                {
                    m_LVitemchecked = 0;
                    m_LVitemcount = 0;
                    ListView_DeleteAllItems(m_hwndLV);
                    ListView_DeleteColumn(m_hwndLV,0);
                    ListView_DeleteColumn(m_hwndLV,0);
                    ListView_DeleteColumn(m_hwndLV,0);
                    ListView_DeleteColumn(m_hwndLV,0);
                    ListView_DeleteColumn(m_hwndLV,0);
                    EnableWindow(m_hwndLV,FALSE);
                    EnableWindow(GetDlgItem(hwnd,IDC_CHKUNCHK),FALSE);
                    EnableWindow(GetDlgItem(hwnd,IDOK),FALSE);
                }
            }
        }
        break;
    case IDC_CHKUNCHK:
        if(HIWORD(wParam) == BN_CLICKED)
        {
            int state=SendMessage(GetDlgItem(hwnd,IDC_CHKUNCHK),BM_GETCHECK,0,0);
            switch(state)
            {
            case BST_CHECKED:
                {
                LVITEM LvItem;
                LvItem.stateMask = LVIS_STATEIMAGEMASK;
                LvItem.state = INDEXTOSTATEIMAGEMASK(2);
                SendMessage(GetDlgItem(hwnd,IDC_CONNECTION_LIST), LVM_SETITEMSTATE, -1, (LPARAM)&LvItem);
                }
                break;
            case BST_UNCHECKED:
                {
                LVITEM LvItem;
                LvItem.stateMask = LVIS_STATEIMAGEMASK;
                LvItem.state = INDEXTOSTATEIMAGEMASK(1);
                SendMessage(GetDlgItem(hwnd,IDC_CONNECTION_LIST), LVM_SETITEMSTATE, -1, (LPARAM)&LvItem);
                }
                break;
            }
        }
        break;
    case IDOK:
        if(m_isTransfer == wyFalse && m_isStop == wyFalse)
        {
            m_syncevent = CreateEvent(NULL, TRUE, FALSE, NULL);
            m_endDialog =   wyFalse;
            _beginthread(ExportImportConnection::ImportThread,0,hwnd);
            HandleMsgs(m_syncevent, wyFalse);
            if(m_syncevent)
		        VERIFY(CloseHandle(m_syncevent));
            if(m_endDialog == wyTrue)
            {
                EndDialog(hwnd,IDOK);
                m_endDialog = wyFalse;
            }
        }
        else
        {
            m_isStop=wyTrue;
            EnableWindow(GetDlgItem(hwnd,IDOK),FALSE);
        }
        break;
    case IDCANCEL:
        EndDialog(hwnd,IDCANCEL);
        break;
    
    }
    
}

//Returns valid section name available for importing connection details
wyInt32
ExportImportConnection::GetValidSectionName(ConDetails **arr_cdSec, wyInt32 noOfConnection,wyInt32 cono)
{
    wyWChar     *wsecname=NULL;
    wyString    tempString;
    
    tempString.Sprintf("Connection %d",cono);
    wsecname=tempString.GetAsWideChar();

    while(SearchNameIfExisting(arr_cdSec,wsecname,0,noOfConnection-1,false) >= 0)
    {
        cono++;
        tempString.Sprintf("Connection %d",cono);
        wsecname=tempString.GetAsWideChar();
    }
    return cono;
}

//Finds for conflicted names and returns the valid connection name with which connection can be imported 
void
ExportImportConnection::GetValidConnectionName(ConDetails **arr_cdName, wyWChar *buffer, wyInt32 noOfConnection)
{
    wyInt32     no=0;
    wyString    tempString;
    wyWChar     temp[256] = {0};
    
    do
    {
        wcscpy(temp,buffer);
        no++;
        tempString.Sprintf("_(%d)",no);
        wcscat(temp,tempString.GetAsWideChar());
    }while(SearchNameIfExisting(arr_cdName,temp,0,noOfConnection-1,true) >= 0);
    
    wcscat(buffer,tempString.GetAsWideChar());
}

//Updates the ConDetails array so that they remain in sorted state when new connection details are inserted 
void
ExportImportConnection::UpdateArray(wyInt32 noOfConnection,ConDetails **arr,ConDetails *cd,bool param)
{
    wyInt32 i=0,insertIndex=noOfConnection;
    
    //fnd the position where data is to be inserted
    while(i < noOfConnection && wcsicmp(param ? arr[i]->conname : arr[i]->ininame, param ? cd->conname : cd->ininame) < 0)
    {
        i++;
    }
    insertIndex=i;
    i=noOfConnection;

    //shift remaining data by one place
    while(i != insertIndex)
    {
        arr[i] = arr[i-1];
        i--;
    }

    //Insert the data at proper index
    arr[insertIndex]=cd;
}

//Function imports data to sqlyog.ini
wyBool
ExportImportConnection::ImportDataToIni(HWND hwnd, wyInt32& noOfConnection,ConDetails **arr_cdName, ConDetails **arr_cdSec)
{
    LVITEM          li;
    ConDetails      *cd=NULL;
    wyChar          secname[30] = {0};
    wyInt32         i=0,action = -1;
    wyWChar         buffer[MAX_PATH+1]={0};
    wyWChar         fromName[MAX_PATH + 1] = {0};
    wyChar          te[14];
    wyWChar         text[MAX_PATH+1]={0};
    wyString        tempString, statusText(""), tempstr;
    wyInt32         cono=0;
    wyInt32         index=0;
    wyBool          isSkipped=wyFalse;
	wyInt64			ret;

    li.iSubItem=0;
    li.mask=LVIF_TEXT|LVIF_PARAM;
    li.pszText=fromName;
    li.cchTextMax=MAX_PATH;
    m_conflictNo = 0;
    for( ; i < m_LVitemcount && m_isStop == wyFalse ; i++)
    {
        if(ListView_GetCheckState(m_hwndLV,i) != 0)
        {
            m_conflictNo++;
            li.iItem=i;
            SendMessage(m_hwndLV,LVM_GETITEM,0,(LPARAM)&li);
            
            cono=(wyInt32)li.lParam;
            tempString.Sprintf("Connection %d",cono);
            
            strcpy(m_fromSec, tempString.GetString());

            index=-1;
            index = SearchNameIfExisting(arr_cdName,li.pszText,0,noOfConnection-1,true);
            
            if(index >= 0)
            {
                if(action == -1)
                {
                    wcstombs(m_conflictName,li.pszText,MAX_PATH);
                    ret = DialogBoxParam(pGlobals->m_hinstance,MAKEINTRESOURCE(IDD_CONNECTION_IMPORT_CONFLICT),
                        hwnd, ExportImportConnection::DialogProcCnflct,(LPARAM)this);
                    if(ret & REMEMBER)
                    {
                        action=ret;
                    }
                }

                if(ret & KEEPBOTH)
                {
                    //for keeping both simply append the importing connection in ini file with new section name and new connection name

                    cono=GetValidSectionName(arr_cdSec,noOfConnection,cono);
                    
                    wcscpy(buffer, li.pszText);

                    GetValidConnectionName(arr_cdName,buffer,noOfConnection);
                    
                    tempstr.SetAs(li.pszText);
                    tempString.SetAs(buffer);

                    if(tempString.GetLength() > 24)
                    {
                        wcscpy(text,buffer);
                        _wcsrev(text);
                        text[6] = '\0';
                        _wcsrev(text);
                        wcstombs(te,text,14);
                        statusText.Sprintf(_("Transferring %.16s... as %.16s...%s"),tempstr.GetString(),tempString.GetString(),te);
                        SetWindowText(GetDlgItem(hwnd,IDC_STATUS), statusText.GetAsWideChar());
                    }
                    else
                    {
                        statusText.Sprintf(_("Transferring %s as %s"),tempstr.GetString(),tempString.GetString());
                        SetWindowText(GetDlgItem(hwnd, IDC_STATUS),statusText.GetAsWideChar());
                    }

                    tempstr.Sprintf("Connection %d",cono);
                    wyIni::IniTransferFullSection(m_fromSec, tempstr.GetString(), m_fromfile.GetString(), m_tofile.GetString());

                    wyIni::IniWriteString(tempstr.GetString(), "Name", tempString.GetString(), m_tofile.GetString());

                    cd= (ConDetails *)calloc(1,sizeof(ConDetails));
                    if(cd == NULL)
                    {
                        yog_message(hwnd,_(L"Memory not sufficient to complete task!"),pGlobals->m_appname.GetAsWideChar(),MB_OK | MB_ICONERROR);
                        m_isStop=wyTrue;
                        return wyFalse;
                    }

                    cd->conname = (wyWChar *)calloc(wcslen(buffer)+1,sizeof(wyWChar));
                    wcscpy(cd->conname,buffer);
                    wcscpy(cd->ininame,tempstr.GetAsWideChar());

                    UpdateArray(noOfConnection,arr_cdName,cd,true);
                    UpdateArray(noOfConnection,arr_cdSec,cd,false);
                    cd=NULL;
                    noOfConnection++;
                }
                else if(ret & REPLACE)
                {
                    //for replacing first delete the existing section, then append the new connection with same section name and same connection name

                    wcstombs(secname, arr_cdName[index]->ininame, 30);

                    tempString.SetAs(li.pszText);
                    statusText.Sprintf(_("Replacing %s"),tempString.GetString());
                    SetWindowText(GetDlgItem(hwnd,IDC_STATUS),statusText.GetAsWideChar());
                        
                    wyIni::IniDeleteSection(secname, m_tofile.GetString());
                    
                    wyIni::IniTransferFullSection(m_fromSec, secname, m_fromfile.GetString(), m_tofile.GetString());
                }
                else if(ret & SKIP)
                {
                    isSkipped=wyTrue;
                }
                else if(ret & IDCANCEL)
                {
                    return wyFalse;
                }
            }
            else
            {
                //If there is no conflict in connection name, then simply check for conflict in section name, 
                //if conflict found then import the connection details with new section name else with same section name 

                cono=GetValidSectionName(arr_cdSec,noOfConnection,cono);
                tempString.Sprintf("Connection %d",cono);

                tempstr.SetAs(li.pszText);
                statusText.Sprintf(_("Transferring %s"),tempstr.GetString());
                SetWindowText(GetDlgItem(hwnd,IDC_STATUS),statusText.GetAsWideChar());
                
                wyIni::IniTransferFullSection(m_fromSec, tempString.GetString(), m_fromfile.GetString(), m_tofile.GetString());

                cd=(ConDetails *)calloc(1,sizeof(ConDetails));
                if(cd == NULL)
                    {
                        yog_message(hwnd,_(L"Memory not sufficient to complete task!"),pGlobals->m_appname.GetAsWideChar(),MB_OK | MB_ICONERROR);
                        m_isStop=wyTrue;
                        return wyFalse;
                    }
                cd->conname = (wyWChar *)calloc(wcslen(li.pszText)+1,sizeof(wyWChar));
                if(cd->conname == NULL)
                {
                    yog_message(hwnd,_(L"Memory not sufficient to complete task!"),pGlobals->m_appname.GetAsWideChar(),MB_OK | MB_ICONERROR);
                    m_isStop=wyTrue;
                    return wyFalse;
                }

                wcscpy(cd->conname,tempstr.GetAsWideChar());
                wcscpy(cd->ininame,tempString.GetAsWideChar());
                
                UpdateArray(noOfConnection,arr_cdName,cd,true);
                UpdateArray(noOfConnection,arr_cdSec,cd,false);
                
                cd=NULL;
                noOfConnection++;
            }
        }
    }


    if(m_isStop == wyTrue || isSkipped == wyTrue)
        return wyFalse;

    return wyTrue;
}

//Dialog Procedure for Conflict Resolution Dialog
INT_PTR CALLBACK
ExportImportConnection::DialogProcCnflct(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    ExportImportConnection *ExportImportInstance=(ExportImportConnection *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
    wyString text;
    wyInt64 ret=0;

    switch(message)
    {
    case WM_INITDIALOG:
        ExportImportInstance=(ExportImportConnection*)lParam;
        SetWindowLongPtr(hwnd,GWLP_USERDATA, lParam);

        if(strlen(ExportImportInstance->m_conflictName) > 26)
            text.Sprintf(_("Connection \"%.26s...\" already exists. What would you like to do?"),ExportImportInstance->m_conflictName);
        else
            text.Sprintf(_("Connection \"%s\" already exists. What would you like to do?"),ExportImportInstance->m_conflictName);
        SetWindowText(GetDlgItem(hwnd,IDC_MESSAGE),text.GetAsWideChar());
        
        if(ExportImportInstance->m_conflictNo == ExportImportInstance->m_LVitemchecked || ExportImportInstance->m_LVitemchecked == 1)
        {
            SendMessage(GetDlgItem(hwnd,IDC_CHECKALL),BM_SETCHECK,BST_UNCHECKED,0);
        }
        else
        {
            if(ExportImportInstance->m_checkAll == wyTrue)
            {
                SendMessage(GetDlgItem(hwnd,IDC_CHECKALL),BM_SETCHECK,BST_CHECKED,0);
            }
            else
            {
                SendMessage(GetDlgItem(hwnd,IDC_CHECKALL),BM_SETCHECK,BST_UNCHECKED,0);
            }
        }
        break;
    case WM_COMMAND:
        if(SendMessage(GetDlgItem(hwnd,IDC_CHECKALL),BM_GETCHECK,0,0) == BST_CHECKED)
        {
            ExportImportInstance->m_checkAll = wyTrue;
            ret=REMEMBER;
        }
        else
        {
            ExportImportInstance->m_checkAll = wyFalse;
        }
            
        switch(LOWORD(wParam))
        {
        case IDC_BUTTON_KEEPBOTH:
            ret|=KEEPBOTH;
            EndDialog(hwnd,ret);
            break;
        case IDC_BUTTON_REPLACE:
            ret|=REPLACE;
            EndDialog(hwnd,ret);
            break;
        case IDC_BUTTON_SKIP:
            ret|=SKIP;
            EndDialog(hwnd,ret);
            break;
        case IDCANCEL:
            EndDialog(hwnd,IDCANCEL);
            break;
        }
        break;
    }
    return 0;
}


// Loads data to Listview
wyBool ExportImportConnection::LoadDataToLV(HWND hwnd)
{
    wyString    dirstr,allsecnames,connnamestr,connhostnamestr,connusernamestr,connportstr;
	wyChar      *allsectionnames = NULL,seps[] = ";",*tempconsecname = NULL;
    wyInt32 cono = 0;
    wyBool isSSH = wyFalse;
    wyBool isSSL = wyFalse;
    wyBool isHTTP = wyFalse;
    const wyChar *file;

    dirstr.SetAs(m_fromfile);
	file=dirstr.GetString();
    
    ListView_DeleteAllItems(m_hwndLV);
    
    m_ConInIni=wyIni::IniGetSection(&allsecnames, &dirstr);
    allsectionnames = (wyChar*)allsecnames.GetString();
    tempconsecname = strtok(allsectionnames, seps);
	int i=0;
    if(tempconsecname == NULL)
    {
        return wyFalse;
    }
    else
    {
        if(m_LVitemcount == 0)
            AddColumnsToListViewCtrl(m_hwndLV);
    
        while(tempconsecname)
        {
            //Flooding structure with details
            wyIni::IniGetString(tempconsecname, "Name", "", &connnamestr, file);
            wyIni::IniGetString(tempconsecname,"Host","",&connhostnamestr,file);
            wyIni::IniGetString(tempconsecname,"User","",&connusernamestr,file);
            wyIni::IniGetString(tempconsecname,"Port","",&connportstr,file);
            isSSH = (wyIni::IniGetInt(tempconsecname, "SSH", 0, file))?(wyTrue):(wyFalse);
	        isHTTP = (wyIni::IniGetInt(tempconsecname, "Tunnel", 0, file))?(wyTrue):(wyFalse);
            isSSL = wyIni::IniGetInt(tempconsecname, "SslChecked", 0, file)?(wyTrue):(wyFalse);
        
            cono=atoi(tempconsecname+10);
            if(connnamestr.Compare("") == 0)
                return wyFalse;
            AddRowsToListViewControl(m_hwndLV, cono, connnamestr.GetAsWideChar(), connhostnamestr.GetAsWideChar(), connusernamestr.GetAsWideChar(), connportstr.GetAsWideChar(),isSSH, isSSL, isHTTP);
            ++i;
            tempconsecname = strtok(NULL, seps);
        }
        m_LVitemcount=i;
        m_LVitemchecked=i;
        Button_SetCheck(GetDlgItem(hwnd,IDC_CHKUNCHK),true);
    }
    return wyTrue;
}

//dailog Procedure for Export Connection Details Dialog
INT_PTR CALLBACK
ExportImportConnection::ConnectionManagerDialogProc(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam)
{
    ExportImportConnection *ExportImportInstance=(ExportImportConnection *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
    
    switch(message)
    {
    case WM_INITDIALOG:
        ExportImportInstance=(ExportImportConnection*)lParam;
        SetWindowLongPtr(hwnd,GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
        ExportImportInstance->OnInitConnectionManagerDialog(hwnd,wParam,lParam);
        ExportImportInstance->m_initcompleted = wyTrue;
        break;
    case WM_COMMAND:
        ExportImportInstance->OnCommandConnectionManagerDialog(hwnd,wParam,lParam);
        break;
    case WM_NOTIFY:
        return ExportImportInstance->OnNotifyConnectionManagerDialog(hwnd,wParam,lParam);
        break;
    case WM_PAINT:
        DrawSizeGripOnPaint(hwnd);
        ExportImportInstance->OnPaint(hwnd);
        break;
    case WM_DESTROY:
        ExportImportInstance->OnClose(hwnd);
        if(ExportImportInstance->m_initcompleted == wyTrue)
        {
            SaveInitPos(hwnd, EXPORTCONNECTIONDETAILS_SECTION);
        }
        EndDialog(hwnd,IDCANCEL);
        break;
    case WM_GETMINMAXINFO:
        ExportImportInstance->OnWMSizeInfo(lParam);
        return 0;
        break;
    case WM_SIZE:
        if(wParam == SIZE_RESTORED)
        {
		    ExportImportInstance->OnResizeExport(hwnd,lParam);
            break;
        }
        return FALSE;
        break;
    case WM_HELP:
        ExportImportInstance->HandleHelp();
        break;
    case WM_MOUSEMOVE:
        ExportImportInstance->OnMouseMove(hwnd, lParam);
        break;
        
    default:
        return FALSE;
    }
    return TRUE;
}


/*Handle the mouse state
Shows WAIT while comparing(except over the 'Stop' button) or execting
*/
void
ExportImportConnection::OnMouseMove(HWND hwnd, LPARAM lParam)
{	
	RECT	rctstop;
	HWND	hwndok = NULL;
	POINT   pt;

	if(m_isStop == wyTrue)
		VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));

	else if(m_isTransfer == wyTrue)
	{
		VERIFY(hwndok = GetDlgItem(hwnd, IDOK));

		pt.x = GET_X_LPARAM(lParam);
		pt.y = GET_Y_LPARAM(lParam);

		VERIFY(GetWindowRect(hwndok, &rctstop));
		VERIFY(MapWindowPoints(hwnd, NULL, &pt, 1));

		if(PtInRect(&rctstop, pt))
			VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));		
		else
			VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));				
	}

	else
		SetCursor(LoadCursor(NULL, IDC_ARROW));

	return;
}


// handles help command
void
ExportImportConnection::HandleHelp(wyBool ishelpclicked)
{
        ShowHelp("http://sqlyogkb.webyog.com/article/35-export-import-connection-details");    
}

//Function Called for resizing Import Connection Details Dialog
void
ExportImportConnection::OnResizeImport(HWND hwnd,LPARAM lParam)
{
    if(m_initcompleted == wyFalse)
    {
        return;
    }
    PositionControls_Import(hwnd,lParam);
    return;
}

//Function called for resizing Export Connection Details Dialog
void
ExportImportConnection::OnResizeExport(HWND hwnd,LPARAM lParam)
{
    if(m_initcompleted == wyFalse)
    {
        return;
    }
    PositionControls_Export(hwnd,lParam);
    return;
}

//Function Positions  controls in Import Connection Details Dialog
void 
ExportImportConnection::PositionControls_Import(HWND hwnd, LPARAM lParam)
{
    RECT        rcWnd,rcCtrl,rcCtrl1;
    wyInt32     width=0;
    wyInt32     height=0;
    wyInt32     y=0;
    HDWP        hdefwp;

    hdefwp = BeginDeferWindowPos(7);

    GetClientRect(hwnd,&rcWnd);
    if(lParam != NULL)
    {
        width=LOWORD(lParam);
        height=HIWORD(lParam);
    }
    else
    {
        width=rcWnd.right-rcWnd.left;
        height=rcWnd.bottom-rcWnd.top;
    }

    //Invalidate Grip area
    rcCtrl.top=rcWnd.top;
    rcCtrl.bottom=rcWnd.bottom;
    rcCtrl.left=rcWnd.right-30;
    rcCtrl.right=rcWnd.right;
    InvalidateRect(hwnd,&rcCtrl,FALSE);
    rcCtrl.top=rcWnd.bottom-30;
    rcCtrl.left=rcWnd.left;
    InvalidateRect(hwnd,&rcCtrl,FALSE);

    GetWindowRect(GetDlgItem(hwnd,IDC_CONNECTION_LIST),&rcCtrl1);
    MapWindowPoints(NULL,hwnd,(LPPOINT)&rcCtrl1, 2);
    hdefwp = DeferWindowPos(hdefwp, GetDlgItem(hwnd,IDC_CONNECTION_LIST), NULL, 
        rcCtrl1.left, rcCtrl1.top,width-41,height-82, SWP_NOZORDER);

    y=rcCtrl1.top+height-82;

    GetWindowRect(GetDlgItem(hwnd,IDC_STATUS),&rcCtrl);
    MapWindowPoints(NULL,hwnd,(LPPOINT)&rcCtrl,2);
    hdefwp = DeferWindowPos(hdefwp, GetDlgItem(hwnd,IDC_STATUS),NULL,
        rcCtrl.left, y+8 , rcCtrl.right-rcCtrl.left,rcCtrl.bottom-rcCtrl.top,SWP_NOZORDER);
    
    GetWindowRect(GetDlgItem(hwnd,IDCANCEL),&rcCtrl);
    MapWindowPoints(NULL,hwnd,(LPPOINT)&rcCtrl,2);
    hdefwp = DeferWindowPos(hdefwp, GetDlgItem(hwnd,IDCANCEL),NULL,
        rcCtrl1.left+width-41-rcCtrl.right + rcCtrl.left, y+5 , rcCtrl.right-rcCtrl.left,rcCtrl.bottom-rcCtrl.top,SWP_NOZORDER);
    
    GetWindowRect(GetDlgItem(hwnd,IDOK),&rcCtrl);
    MapWindowPoints(NULL,hwnd,(LPPOINT)&rcCtrl,2);
    hdefwp = DeferWindowPos(hdefwp, GetDlgItem(hwnd,IDOK),NULL,
        rcCtrl1.left+width-122-rcCtrl.right+rcCtrl.left , y+5,rcCtrl.right-rcCtrl.left,rcCtrl.bottom-rcCtrl.top,SWP_NOZORDER);

    GetWindowRect(GetDlgItem(hwnd,IDC_LOAD_FILE),&rcCtrl);
    MapWindowPoints(NULL,hwnd,(LPPOINT)&rcCtrl,2);
    hdefwp = DeferWindowPos(hdefwp, GetDlgItem(hwnd,IDC_LOAD_FILE),NULL,
        rcCtrl1.left+width-41-rcCtrl.right+rcCtrl.left,rcCtrl.top,rcCtrl.right-rcCtrl.left,rcCtrl.bottom-rcCtrl.top,SWP_NOZORDER);

    EndDeferWindowPos(hdefwp);
}

//Function positions controls in Export Connection Details Dialog Box
void
ExportImportConnection::PositionControls_Export(HWND hwnd, LPARAM lParam)
{
    RECT rcWnd,rcCtrl,rcCtrl1;
    wyInt32 width=0;
    wyInt32 height=0;
    wyInt32 y=0;
    HDWP        hdefwp;

    hdefwp = BeginDeferWindowPos(7);

    GetClientRect(hwnd,&rcWnd);
        
    if(lParam != NULL)
    {
        width=LOWORD(lParam);
        height=HIWORD(lParam);
    }
    else
    {
        width=rcWnd.right-rcWnd.left;
        height=rcWnd.bottom-rcWnd.top;
    }

    //Invalidate Grip area
    rcCtrl.top=rcWnd.top;
    rcCtrl.bottom=rcWnd.bottom;
    rcCtrl.left=rcWnd.right-30;
    rcCtrl.right=rcWnd.right;
    InvalidateRect(hwnd,&rcCtrl,FALSE);
    rcCtrl.top=rcWnd.bottom-30;
    rcCtrl.left=rcWnd.left;
    InvalidateRect(hwnd,&rcCtrl,FALSE);

    GetWindowRect(GetDlgItem(hwnd,IDC_CONNECTION_LIST),&rcCtrl1);
    MapWindowPoints(NULL,hwnd,(LPPOINT)&rcCtrl1, 2);
    hdefwp = DeferWindowPos(hdefwp, GetDlgItem(hwnd,IDC_CONNECTION_LIST), NULL, 
        rcCtrl1.left, rcCtrl1.top,width-43,height-119, SWP_NOZORDER);

    y=rcCtrl1.top+height-107;

    GetWindowRect(GetDlgItem(hwnd,IDC_STATUS),&rcCtrl);
    MapWindowPoints(NULL,hwnd,(LPPOINT)&rcCtrl, 2);
    hdefwp = DeferWindowPos(hdefwp, GetDlgItem(hwnd,IDC_STATUS), NULL, 
        rcCtrl1.left, y+37 ,rcCtrl.right - rcCtrl.left, rcCtrl.bottom - rcCtrl.top, SWP_NOZORDER);


    GetWindowRect(GetDlgItem(hwnd,IDC_STATIC_FILE),&rcCtrl);
    MapWindowPoints(NULL,hwnd,(LPPOINT)&rcCtrl,2);
    hdefwp = DeferWindowPos(hdefwp, GetDlgItem(hwnd,IDC_STATIC_FILE),NULL,
        rcCtrl.left,y+2,rcCtrl.right-rcCtrl.left,rcCtrl.bottom-rcCtrl.top,SWP_NOZORDER);

    GetWindowRect(GetDlgItem(hwnd,IDC_CONNECTION_FILE),&rcCtrl);
    MapWindowPoints(NULL,hwnd,(LPPOINT)&rcCtrl,2);
    hdefwp = DeferWindowPos(hdefwp, GetDlgItem(hwnd,IDC_CONNECTION_FILE),NULL,
        rcCtrl.left,y,width-139,rcCtrl.bottom-rcCtrl.top,SWP_NOZORDER);

    GetWindowRect(GetDlgItem(hwnd,IDC_BUTTON_CONNECTION),&rcCtrl1);
    MapWindowPoints(NULL,hwnd,(LPPOINT)&rcCtrl1,2);
    hdefwp = DeferWindowPos(hdefwp, GetDlgItem(hwnd,IDC_BUTTON_CONNECTION),NULL,
        rcCtrl.left+width-134,y,rcCtrl1.right-rcCtrl1.left,rcCtrl1.bottom-rcCtrl1.top,SWP_NOZORDER);

    GetWindowRect(GetDlgItem(hwnd,IDCANCEL),&rcCtrl1);
    MapWindowPoints(NULL,hwnd,(LPPOINT)&rcCtrl1,2);
    hdefwp = DeferWindowPos(hdefwp, GetDlgItem(hwnd,IDCANCEL),NULL,
        rcCtrl.left+width-135-(rcCtrl1.right-rcCtrl1.left)/2,y+34,rcCtrl1.right-rcCtrl1.left,rcCtrl1.bottom-rcCtrl1.top,SWP_NOZORDER);
    
    GetWindowRect(GetDlgItem(hwnd,IDOK),&rcCtrl1);
    MapWindowPoints(NULL,hwnd,(LPPOINT)&rcCtrl1,2);
    hdefwp = DeferWindowPos(hdefwp, GetDlgItem(hwnd,IDOK),NULL,
        rcCtrl.left+width-142-3*(rcCtrl1.right-rcCtrl1.left)/2,y+34,rcCtrl1.right-rcCtrl1.left,rcCtrl1.bottom-rcCtrl1.top,SWP_NOZORDER);
    
    EndDeferWindowPos(hdefwp);
}

// Returns the minimum Dialog size info
void
ExportImportConnection::OnWMSizeInfo(LPARAM lparam)
{
    MINMAXINFO* pminmax = (MINMAXINFO*)lparam;
    if(m_initcompleted == wyTrue)
    {
        pminmax->ptMinTrackSize.x = m_wndrect.right - m_wndrect.left;
        pminmax->ptMinTrackSize.y = m_wndrect.bottom - m_wndrect.top;
    }
}

// handles WM_PAINT message
void
ExportImportConnection::OnPaint(HWND hwnd)
{
    HDC         hdc;
	PAINTSTRUCT ps;

    VERIFY(hdc = BeginPaint(hwnd, &ps));
    DoubleBuffer::EraseBackground(hwnd, hdc, NULL, GetSysColor(COLOR_3DFACE));
    EndPaint(hwnd, &ps);
}

// Function used to activate Export/Import Connection Details Dialog Box
wyInt32
ExportImportConnection::ActivateConnectionManagerDialog(HWND hwnd)
{
    wyInt32 ret;
    if(m_isimport)
    {
        ret = DialogBoxParam(pGlobals->m_hinstance,MAKEINTRESOURCE(IDD_CONNECTION_IMPORT_MANAGER),hwnd,ExportImportConnection::ConnectionManagerDialogProcImport,(LPARAM)this);
    }
    else
    {
        ret = DialogBoxParam(pGlobals->m_hinstance,MAKEINTRESOURCE(IDD_CONNECTION_DETAILS_MANAGER),hwnd,ExportImportConnection::ConnectionManagerDialogProc,(LPARAM)this);
    }
            return ret;
}


// Split Function used for Quick Sort. Returns appropriate index for pivot
wyInt32
ExportImportConnection::Split(ConDetails **arr, wyInt32 lower, wyInt32 upper, bool param)
{
    wyInt32 p=0,q=0;
    ConDetails * temp = NULL;
    p=lower+1;
    q=upper;
    
    while(q >= p && m_isStop == wyFalse)
    {
        const wyWChar *pivot = param ? arr[lower]->conname : arr[lower]->ininame;
        while(p <= upper && wcsicmp(param ? arr[p]->conname : arr[p]->ininame, pivot) < 0)
        {
            p++;
        }
        while(q > lower && wcsicmp(param ? arr[q]->conname : arr[q]->ininame, pivot) >= 0)
        {
            q--;
        }
        if(q > p)
        {
            temp = arr[p];
            arr[p]=arr[q];
            arr[q]=temp;
        }
    }

    if(q != lower)
    {
        temp= arr[lower];
        arr[lower]=arr[q];
        arr[q]=temp;
    }

    return q;
}

// Function performs quick sort
void
ExportImportConnection::QuickSort(ConDetails **arr,wyInt32 lower, wyInt32 upper, bool param)
{
    wyInt32 i;

    if(upper > lower && m_isStop == wyFalse)
    {
        i=Split(arr,lower, upper,param);
        QuickSort(arr,lower,i-1,param);
        QuickSort(arr,i+1,upper,param);
    }
}


//Function returns index of "name" being searched in array. Implements Binary Search
wyInt32 
ExportImportConnection::SearchNameIfExisting(ConDetails **arr,wyWChar *name, wyInt32 lower, wyInt32 upper, bool param)
{
    wyInt32 mid=-1;

    while(upper >= lower)
    {
        mid=(upper+lower)/2;
        if(wcsicmp(param ? arr[mid]->conname : arr[mid]-> ininame ,name) == 0)
        {
            return mid;
        }
        else if(wcsicmp(param ? arr[mid]->conname : arr[mid]->ininame,name) < 0)
        {
            lower=mid+1;
        }
        else 
        {
            upper=mid-1;
        }
    }
    return(-1);
}

// Function loads data from sqlyog.ini for sorting and further finding conflicts with that to be imported.
wyInt32 
ExportImportConnection::LoadSortedDataFromIni(HWND hwnd, ConDetails ***arr_cdName, ConDetails ***arr_cdSec)
{
    wyUInt32	noOfConnections=0;
    wyChar      *allsectionnames = NULL,seps[] = ";",*tempconsecname= NULL;
    wyString    dirstr,allsecnames;
    ConDetails  *cd=NULL;
    wyString    tempString;
    wyInt32     i=0;

    dirstr.SetAs(m_tofile);

    //Load connection names to memory
    noOfConnections=wyIni::IniGetSection(&allsecnames, &dirstr);
    allsectionnames = (wyChar*)allsecnames.GetString();
    tempconsecname = strtok(allsectionnames, seps);
    
    
    *arr_cdName  =   (ConDetails**)calloc((noOfConnections+m_LVitemchecked+1),sizeof(ConDetails*));
    *arr_cdSec   =   (ConDetails**)calloc((noOfConnections+m_LVitemchecked+1),sizeof(ConDetails*));  

    if(! (*arr_cdName) || !(*arr_cdSec))
        return -1;

    while(tempconsecname)
    {
        cd=(ConDetails *)calloc( 1,sizeof(ConDetails));
        if( cd == NULL)
        {
            return -1;
        }
        wyIni::IniGetString(tempconsecname, "Name", "", &tempString, dirstr.GetString());
        if(tempString.Compare("") != 0)
        {
            mbstowcs(cd->ininame,tempconsecname,20);
            cd->conname = (wyWChar *)calloc(wcslen(tempString.GetAsWideChar())+1,sizeof(wyWChar));
            if(cd->conname == NULL)
            {
                return -1;
            }
            wcscpy(cd->conname,tempString.GetAsWideChar());
            (*arr_cdName)[i]=cd;
            (*arr_cdSec)[i]=cd;
            ++i;
        }
        tempconsecname = strtok(NULL, seps);
    }

    noOfConnections = i;
    // Sort the connection names
    QuickSort(*arr_cdName,0,noOfConnections-1,true);

    QuickSort(*arr_cdSec,0,noOfConnections-1,false);
    
    return noOfConnections;
}