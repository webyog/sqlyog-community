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

#include "UserManager.h"
#include "MySQLVersionHelper.h"
#include "DoubleBuffer.h"

#define U_ACCESSDENIED          _("SQLyog cannot retrieve what information is required to populate the User Manager dialog. \
Most likely the user does not have SELECT privilege to privileges tables in the `mysql` database. \
Without that information SQLyog User Manager cannot continue.")

#define U_GLOBAL_PRIV           _(L"Global Privileges")
#define U_OBJECTS_PRIV          _(L"Object Level Privileges")
#define U_NEW_USER              _(L"New User")
#define U_RENAME_USER           _(L"Rename User")
#define U_EDIT_USER             _(L"Edit User")
#define U_ORIG_PRIVTABLE        "UM_ORIG_PRIVTABLE"
#define U_PRIVTABLE             "UM_PRIVTABLE"
#define U_PASSWORD              L"~!@#$%^&*()_+"
#define U_REPASSWORD            L"+_)(*&^%$#@!~"

#define U_CONTEXTHELP_USER      _(L"Edit the user name, password and various limitations of this user")
#define U_CONTEXTHELP_ADDUSER   _(L"Create a new user with limitations")
#define U_CONTEXTHELP_GLOBAL    _(L"GRANT/REVOKE global privileges for this user")
#define U_CONTEXTHELP_OBJECT    _(L"GRANT/REVOKE privileges to selected objects for this user")
#define U_SAVEBUTTON_TEXT       _(L"&Save Changes")
#define U_CANCELBUTTON_TEXT     _(L"&Cancel Changes")

#define U_DBS               1
#define U_TABLES            2
#define U_INDEXES           4
#define U_COLUMNS           8
#define U_FUNCTIONS         16
#define U_PROCEDURES        32
#define U_SERVER_ADMIN      64
#define U_FILE_ACCESS       128

wyBool UserManager::m_initcompleted = wyFalse;

//privilege-column mapping
wyChar* UserManager::m_privmapping[] = {
    "ALTER", "Alter_priv",
    "ALTER ROUTINE", "Alter_routine_priv",
    "CREATE", "Create_priv",
    "CREATE ROUTINE", "Create_routine_priv",
    "CREATE TABLESPACE", "Create_tablespace_priv",
    "CREATE TEMPORARY TABLES", "Create_tmp_table_priv",
    "CREATE VIEW", "Create_view_priv",
    "CREATE USER", "Create_user_priv",
    "DELETE", "Delete_priv",
    "DROP", "Drop_priv",
    "EVENT", "Event_priv",
    "EXECUTE", "Execute_priv",
    "FILE", "File_priv",
    "GRANT", "Grant_priv",
    "INDEX", "Index_priv",
    "INSERT", "Insert_priv",
    "LOCK TABLES", "Lock_tables_priv",
    "PROCESS", "Process_priv",
    "REFERENCES", "References_priv",
    "RELOAD", "Reload_priv",
    "REPLICATION CLIENT", "Repl_client_priv",
    "REPLICATION SLAVE", "Repl_slave_priv",
    "SELECT", "Select_priv",
    "SHOW DATABASES", "Show_db_priv",
    "SHOW VIEW", "Show_view_priv",
    "SHUTDOWN", "Shutdown_priv",
    "SUPER", "Super_priv",
    "TRIGGER", "Trigger_priv",
    "UPDATE", "Update_priv",
};

//default constructor
UserManager::UserManager()
{
    wyInt32 i;

    m_hwnd = NULL;
    m_hmdi = NULL;
    m_isedited = wyFalse;
    m_selindex = -1;
    m_himagelist = NULL;
    m_isnewuser = wyFalse;
    m_privarray = NULL;
    m_privcount = 0;
    m_isautomatedchange = wyFalse;
    m_hwndnote = NULL;
    m_isselectallcheck = wyFalse;
    m_ismysql502 = wyFalse;

    for(i = 0; i < U_MAXLIMITATIONS; ++i)
    {
        m_showlimitations[i] = wyFalse;
    }

    m_isallradiochecked = wyTrue;
    m_lastcheckedindex = -1;
    m_ispasswordchanged = wyFalse;
    m_isusercombochanging = wyFalse;
    m_husericon = NULL;
    m_hglobalprivicon = NULL;
    m_hobjprivicon = NULL;
    m_gripproc = NULL;
    m_isdeleteuser = wyFalse;
	m_userlist = NULL;
#ifdef _DEBUG
    m_sqlite.SetLogging(wyTrue);
#else
    m_sqlite.SetLogging(wyFalse);
#endif
}

//destructor
UserManager::~UserManager()
{
    wyInt32     i;
    wyString    temp;
    DlgControl* pdlgctrl;

    temp.SetAs(m_sqlite.GetDbName());
    
    if(temp.GetLength())
    {
        //delete the per user items and close the sqlite connection
        DeleteUserLevelItems();
        m_sqlite.Close();

        for(i = 0; i < m_privcount; ++i)
        {
            delete m_privarray[i];
        }

        delete[] m_privarray;
    }
    
    if(m_himagelist)
    {
        ImageList_Destroy(m_himagelist);
    }

    if(m_husericon)
    {
        DestroyIcon(m_husericon);
    }

    if(m_hglobalprivicon)
    {
        DestroyIcon(m_hglobalprivicon);
    }

    if(m_hobjprivicon)
    {
        DestroyIcon(m_hobjprivicon);
    }

    while((pdlgctrl = (DlgControl*)m_controllist.GetFirst()))
    {
        m_controllist.Remove(pdlgctrl);
        delete pdlgctrl;
    }
	while(m_userlist)
	{	
		USERLIST *temp1 = m_userlist;
		m_userlist = m_userlist->next;
		delete temp1;
	}
}

PrivilegedObject::PrivilegedObject(wyInt32 privcount, Privileges** privileges, wyInt32 context, wyInt32 value)
{
    wyInt32 i;

    m_dbname.SetAs("");
    m_objectname.SetAs("");
    m_columnname.SetAs("");
    m_objecttype = 0;
    m_privileges = NULL;

    if(privcount)
    {
        m_privileges = new wyInt32[privcount];
    }

    for(i = 0; i < privcount; ++i)
    {
        if(privileges[i]->context & context)
        {
            m_privileges[i] = 0;
        }
        else
        {
            m_privileges[i] = value;
        }
    }
}

PrivilegedObject::~PrivilegedObject()
{
    delete[] m_privileges;
}

//initiate the user manager dialog
wyInt32
UserManager::Create(HWND hwnd)
{
    //reset the initaliate flag
    m_initcompleted = wyFalse;

    //create the dialog
    return DialogBoxParam(pGlobals->m_hinstance, 
                          MAKEINTRESOURCE(IDD_USERMANAGER), 
                          hwnd, 
                          UserManager::DlgProc, 
                          (LPARAM)this);
}

//standard dialog box procedure
INT_PTR CALLBACK    
UserManager::DlgProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam)
{
    UserManager* pum = (UserManager*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch(message)
    {
        case WM_INITDIALOG: 
            SetWindowLongPtr(hwnd, GWLP_USERDATA, lparam);
            LocalizeWindow(hwnd);
            pum = (UserManager*)lparam;
            
            if(pum->InitDialog(hwnd) == wyFalse)
            {
                EndDialog(hwnd, 0);
            }
            else
            {
                //set the initiate flag
                m_initcompleted = wyTrue;
            }
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
	{
		EnableWindow(GetDlgItem(hwnd, IDC_DELUSER), wyFalse);
		EnableWindow(GetDlgItem(hwnd, IDC_NEWUSER), wyFalse);
	}
#endif
            break;

        case WM_COMMAND:
            return pum->OnWMCommand(wparam, lparam);
            break;

        case WM_NOTIFY:
            return pum->OnWMNotify(wparam, lparam);
            break;

        case WM_CTLCOLORSTATIC:
            //set the background for the note window
            if(GetDlgItem(hwnd, IDC_PRIV_NOTE) == (HWND)lparam)
            {
                SetBkMode((HDC)wparam, TRANSPARENT);
			    return (BOOL)GetSysColorBrush(COLOR_WINDOW);
            }
            return FALSE;

        case WM_SIZE:
		    if(wparam == SIZE_RESTORED)
            {
			    pum->OnResize();
                break;
            }
            return FALSE;

        case WM_HELP:
            pum->HandleHelp();
            break;

        case WM_ERASEBKGND:
            //done for double buffering
            break;

        case WM_PAINT:
            pum->OnPaint(hwnd);
            break;

        case WM_GETMINMAXINFO:
            pum->OnWMSizeInfo(lparam);
            break;

        case WM_DESTROY:
            if(m_initcompleted == wyTrue)
            {
                //reset the db context
                pum->SetResetDBContext(wyFalse);

                //save the position
                SaveInitPos(hwnd, USERMANAGERINTERFACE_SECTION);
            }

            return FALSE;

        default:
            return FALSE;
    }

    return TRUE;
}

//function sets the minimum track size
void
UserManager::OnWMSizeInfo(LPARAM lparam)
{
    MINMAXINFO* pminmax = (MINMAXINFO*)lparam;

    pminmax->ptMinTrackSize.x = m_wndrect.right - m_wndrect.left;
    pminmax->ptMinTrackSize.y = m_wndrect.bottom - m_wndrect.top;
}

//function handles the WM_PAINT and paints the window using double buffering
void
UserManager::OnPaint(HWND hwnd)
{
    HDC         hdc;
	PAINTSTRUCT ps;

    VERIFY(hdc = BeginPaint(hwnd, &ps));
    //DoubleBuffer db(hwnd, hdc);
    DoubleBuffer::EraseBackground(hwnd, hdc, NULL, GetSysColor(COLOR_3DFACE));
    //db.EraseBackground(GetSysColor(COLOR_3DFACE));
    //db.PaintWindow();
	EndPaint(hwnd, &ps);
}

//the function initates the dialog box
wyBool
UserManager::InitDialog(HWND hwnd)
{
    wyInt32     x,y, padding;
    RECT        temp, rectpriv;
    wyString    query, errormsg;
    HICON       hicon;
    HWND        hwndgripper;
    
    //initialize some handles
    m_hwnd = hwnd;
    m_hwndnote = GetDlgItem(m_hwnd, IDC_PRIV_NOTE);
    m_hmdi = GetActiveWin();
    m_ismysql502 = IsMySQL502(m_hmdi->m_tunnel, &m_hmdi->m_mysql);
    GetClientRect(m_hwnd, &m_dlgrect);
    GetWindowRect(m_hwnd, &m_wndrect);

    //set the icon
    hicon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_USERMANAGER));
    SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hicon);
    DestroyIcon(hicon);
    
    //position the select all check box and list view
    GetWindowRect(GetDlgItem(m_hwnd, IDC_USERNAME_PROMPT), &rectpriv);
    MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rectpriv, 2);
    x = rectpriv.left;
    y = rectpriv.top;
    GetWindowRect(GetDlgItem(m_hwnd, IDC_PRIVLIST), &rectpriv);
    MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rectpriv, 2);
    GetWindowRect(GetDlgItem(m_hwnd, IDC_SELECTALLCHECK), &temp);
    MapWindowPoints(NULL, m_hwnd, (LPPOINT)&temp, 2);
    padding = rectpriv.top - temp.top;
    rectpriv.left = x;
    rectpriv.top = y;
    SetWindowPos(GetDlgItem(m_hwnd, IDC_SELECTALLCHECK), NULL, rectpriv.left, rectpriv.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
    SetWindowPos(GetDlgItem(m_hwnd, IDC_PRIVLIST), NULL, rectpriv.left, rectpriv.top + padding, 
                 rectpriv.right - rectpriv.left,
                 rectpriv.bottom - rectpriv.top - padding, 
                 SWP_NOZORDER);

    //set the position of help note
    GetWindowRect(m_hwndnote, &temp);
    MapWindowPoints(NULL, m_hwnd, (LPPOINT)&temp, 2);
    y = rectpriv.top + ((rectpriv.bottom - rectpriv.top) / 2) - ((temp.bottom - temp.top) / 2);
    SetWindowPos(m_hwndnote, GetDlgItem(m_hwnd, IDC_PRIVLIST), temp.left, y, 0, 0, SWP_NOSIZE);

    //hide the note initially
    ShowWindow(m_hwndnote, SW_HIDE);

    //position the tree views
    GetWindowRect(GetDlgItem(m_hwnd, IDC_PRIVOBTREE), &rectpriv);
    MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rectpriv, 2);
    GetWindowRect(GetDlgItem(m_hwnd, IDC_OBTREE), &temp);
    MapWindowPoints(NULL, m_hwnd, (LPPOINT)&temp, 2);
    x = temp.right - temp.left;
    y = rectpriv.bottom - temp.top;
    SetWindowPos(GetDlgItem(m_hwnd, IDC_OBTREE), NULL, 0, 0, x, y, SWP_NOZORDER | SWP_NOMOVE);
    SetWindowPos(GetDlgItem(m_hwnd, IDC_PRIVOBTREE), NULL, temp.left, temp.top, x, y, SWP_NOZORDER);

    //hide the privileged object tree initially
    ShowWindow(GetDlgItem(m_hwnd, IDC_PRIVOBTREE), SW_HIDE);

    //check the all radio button 
    SendMessage(GetDlgItem(m_hwnd, IDC_PRIVILEGEDCHECK), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);

    //enable check boxes in list view
    SendMessage(GetDlgItem(m_hwnd, IDC_PRIVLIST), LVM_SETEXTENDEDLISTVIEWSTYLE, 
                (WPARAM)LVS_EX_CHECKBOXES,(LPARAM)LVS_EX_CHECKBOXES);

    //limit the text in various edit controls
    SendMessage(GetDlgItem(m_hwnd, IDC_USERNAME), EM_LIMITTEXT, (WPARAM)32, 0);
    SendMessage(GetDlgItem(m_hwnd, IDC_HOSTNAME), CB_LIMITTEXT, (WPARAM)60, 0);

    //add some default strings for the host combo
    SendMessage(GetDlgItem(m_hwnd, IDC_HOSTNAME), CB_ADDSTRING, 0, (LPARAM)L"%");
    SendMessage(GetDlgItem(m_hwnd, IDC_HOSTNAME), CB_ADDSTRING, 0, (LPARAM)L"localhost");
    SendMessage(GetDlgItem(m_hwnd, IDC_HOSTNAME), CB_ADDSTRING, 0, (LPARAM)L"127.0.0.1");

    //populate the users registered with the server
    if(PopulateUserCombo() == -1)
    {
        return wyFalse;
    }

    //get various privileges available in the server
    if(GetServerPrivileges() == wyFalse)
    {
        return wyFalse;
    }

    //create the sqlite memory db
    m_sqlite.Open(":memory:");

    //create the privilege tables
    CreatePrivilegeTables();

    //set the spin control ranges
    SetUDConrolsRange();

    //set the flag to identify whether all radio button is checked
    m_isallradiochecked = wyTrue;

    //show all objects initially
    ShowAllObjects();

    //initialize the tree view controls
    InitTreeView();

    //set the position and size of the static control that is used to draw the size gripper
    temp = m_dlgrect;
    hwndgripper = GetDlgItem(m_hwnd, IDC_GRIP);
    temp.left = temp.right - GetSystemMetrics(SM_CXHSCROLL);
	temp.top = temp.bottom - GetSystemMetrics(SM_CYVSCROLL);
    SetWindowPos(hwndgripper, NULL, temp.left, temp.top, temp.right - temp.left, temp.bottom - temp.top, SWP_NOZORDER);
    
    SetWindowText(hwndgripper, L"");

    //set the user data and subclass the control
    SetWindowLongPtr(hwndgripper, GWLP_USERDATA, (LONG_PTR)this);
    m_gripproc = (WNDPROC)SetWindowLongPtr(hwndgripper, GWLP_WNDPROC, (LONG_PTR)UserManager::GripProc);
    
    //get the control rectangles and some properties
    GetCtrlRects();

    //set the initial position of the dialog
    SetWindowPositionFromINI(m_hwnd, USERMANAGERINTERFACE_SECTION, m_wndrect.right - m_wndrect.left, m_wndrect.bottom - m_wndrect.top);
    PositionCtrls();

    //set the db context to mysql
    SetResetDBContext(wyTrue);



    return wyTrue;
}

//function sets/reset the context db to mysql
void
UserManager::SetResetDBContext(wyBool isset)
{
    wyString    query;
    MYSQL_RES*  myres;
    MYSQL_ROW   myrow;

    if(isset == wyTrue)
    {
        //first save the active db context
        query.SetAs("select database()");

        if((myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query)))
        {
            if((myrow = m_hmdi->m_tunnel->mysql_fetch_row(myres)) && myrow[0])
            {
                m_selecteddatabase.SetAs(myrow[0], m_hmdi->m_ismysql41);
            }

            m_hmdi->m_tunnel->mysql_free_result(myres);
        }

        //now use mysql db
        query.SetAs("use `mysql`");
        ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);
    }
    else
    {
        //if there is a db context avaialble, use it
        if(m_selecteddatabase.GetLength())
        {
            query.Sprintf("use `%s`", m_selecteddatabase.GetString());
            ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);
        }
    }
}

//subclassed window procedure for the static control that displays the grip control
BOOL CALLBACK    
UserManager::GripProc(HWND hwnd, wyUInt32 message, WPARAM wparam, LPARAM lparam)
{
    HDC             hdc;
    PAINTSTRUCT     ps;
    RECT            temp;
    UserManager*    pum = (UserManager*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    if(message == WM_PAINT)
    {
        VERIFY(hdc = BeginPaint(hwnd, &ps));
        GetClientRect(hwnd, &temp);
        DrawFrameControl(hdc, &temp, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
        EndPaint(hwnd, &ps);
        return 0;
    }

    return CallWindowProc(pum->m_gripproc, hwnd, message, wparam, lparam);
}

//function gets the control rectangles that is used while resizing the dialog box
void
UserManager::GetCtrlRects()
{
    RECT    rect;
    wyInt32 i, count;
    HWND    hwnd;

    //ctrlid    size-x     size-y
    wyInt32 ctrlid[] = {
        IDC_UM_USER, 0, 0,
        IDC_USERCOMBO, 1, 0,
        IDC_OBTREE, 1, 1,
        IDC_PRIVOBTREE, 1, 1,
        IDC_PRIVILEGEDCHECK, 0, 0,
        IDC_DELUSER, 0, 0,
        IDC_NEWUSER, 0, 0,
        IDC_UM_HELP, 0, 0,
        IDC_CONTEXTICON, 0, 0,
        IDC_CONTEXTHELP, 0, 0,
        IDC_USERNAME_PROMPT, 0, 0,
        IDC_USERNAME, 0, 0,
        IDC_HOST_PROMPT, 0, 0,
        IDC_HOSTNAME, 0, 0,
        IDC_PASSWORD_PROMPT, 0, 0,
        IDC_PASSWORD, 0, 0,
        IDC_PASSWORD2_PROMPT, 0, 0,
        IDC_PASSWORD_CONFIRM, 0, 0,
        IDC_MAXQUERY_PROMPT, 0, 0,
        IDC_MAXQUERIES, 0, 0,
        IDC_MAXQUERY_SPIN, 0, 0,
        IDC_MAXUPDATE_PROMPT, 0, 0,
        IDC_MAXUPDATE, 0, 0,
        IDC_MAXUPDATE_SPIN, 0, 0,
        IDC_MAXCONN_PROMPT, 0, 0,
        IDC_MAXCONN, 0, 0,
        IDC_MAXCONN_SPIN, 0, 0,
        IDC_MAXCONNSIM_PROMPT, 0, 0,
        IDC_MAXSIMCONN, 0, 0,
        IDC_MAXSIMCONN_SPIN, 0, 0,
        IDC_HELPZERO, 0, 0,
        IDC_SELECTALLCHECK, 0, 0,
        IDC_PRIVLIST, 0, 1,
        IDC_GRIP, 0, 0,
        IDC_SAVE_CHANGES, 0, 0,
        IDC_CANCEL_CHANGES, 0, 0,
        IDCANCEL, 0, 0
    };

    count = sizeof(ctrlid)/sizeof(ctrlid[0]);

    //store the control handles and related infos in the linked list
    for(i = 0; i < count; i+=3)
    {
        hwnd = GetDlgItem(m_hwnd, ctrlid[i]);
        GetWindowRect(hwnd, &rect);
        MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rect, 2);
        m_controllist.Insert(new DlgControl(hwnd, 
                                            ctrlid[i], 
                                            &rect, 
                                            ctrlid[i + 1] ? wyTrue : wyFalse, 
                                            ctrlid[i + 2] ? wyTrue : wyFalse));
    }
}

//function positions the controls in the dialog box
void
UserManager::PositionCtrls()
{
    DlgControl* pdlgctrl;
    wyInt32     leftpadding, toppadding, rightpadding, bottompadding, width, height;
    wyInt32     x, y;
    RECT        rect, temp;
    HDWP        hdefwp;

    GetClientRect(m_hwnd, &rect);

    //BeginDeferWindowPos is used to make the control positioning atomic
    hdefwp = BeginDeferWindowPos(m_controllist.GetCount() + 1);

    //iterate throught the control lists
    for(pdlgctrl = (DlgControl*)m_controllist.GetFirst(); pdlgctrl;
        pdlgctrl = (DlgControl*)pdlgctrl->m_next)
    {
        leftpadding = pdlgctrl->m_rect.left - m_dlgrect.left;
        toppadding = pdlgctrl->m_rect.top - m_dlgrect.top;
        rightpadding = m_dlgrect.right - pdlgctrl->m_rect.right;
        bottompadding = m_dlgrect.bottom - pdlgctrl->m_rect.bottom;
        width = pdlgctrl->m_rect.right - pdlgctrl->m_rect.left;
        height = pdlgctrl->m_rect.bottom - pdlgctrl->m_rect.top;
        
        if(pdlgctrl->m_issizex == wyFalse)
        {
            if(pdlgctrl->m_id == IDC_PRIVILEGEDCHECK ||
               pdlgctrl->m_id == IDC_UM_USER)
            {
                x = leftpadding;
            }
            else
            {
                x = rect.right - rightpadding - width;
            }
        }
        else
        {
            x = leftpadding;
            width = rect.right - rightpadding - leftpadding;
        }

        switch(pdlgctrl->m_id)
        {
            case IDC_PRIVILEGEDCHECK:
            case IDC_SAVE_CHANGES:
            case IDC_CANCEL_CHANGES:
            case IDCANCEL:
            case IDC_GRIP:
                y = rect.bottom - bottompadding - height;
                break;

            default:
                y = toppadding;
        }

        if(pdlgctrl->m_issizey == wyTrue)
        {
            height = rect.bottom - bottompadding - y;
        }

        //change the control position
        hdefwp = DeferWindowPos(hdefwp, pdlgctrl->m_hwnd, NULL, x, y, width, height, SWP_NOZORDER);

        //if it is the list view, then we need to position the help node in the middle of the list view
        if(pdlgctrl->m_id == IDC_PRIVLIST)
        {
            GetWindowRect(m_hwndnote, &temp);
            MapWindowPoints(NULL, m_hwnd, (LPPOINT)&temp, 2);
            x = x + width / 2 - (temp.right - temp.left) / 2;
            y = y + height / 2 - (temp.bottom - temp.top) / 2;

            //position the help note
            hdefwp = DeferWindowPos(hdefwp, m_hwndnote, GetDlgItem(m_hwnd, IDC_PRIVLIST), x, y, 0, 0, SWP_NOSIZE);
        }
    }

    //finish the operation and apply changes
    EndDeferWindowPos(hdefwp);
}

//WM_SIZE handler
void
UserManager::OnResize()
{
    if(m_initcompleted == wyFalse)
    {
        return;
    }

    //position the controls
    PositionCtrls();

    //we need to invalidate the list view and help note
    if(IsWindowVisible(m_hwndnote))
    {
        InvalidateRect(GetDlgItem(m_hwnd, IDC_PRIVLIST), NULL, TRUE);
        UpdateWindow(GetDlgItem(m_hwnd, IDC_PRIVLIST));
        InvalidateRect(m_hwndnote, NULL, TRUE);
        UpdateWindow(m_hwndnote);
    }
}

//function executes the query to save the password if necessery
wyBool
UserManager::SavePassword()
{
    wyString    query, password, password2, tempuser, temphost, temppassword;
    wyWChar     buffer[SIZE_128];

    //get the strings from the password field
    GetDlgItemText(m_hwnd, IDC_PASSWORD, buffer, SIZE_128 - 1);
    password.SetAs(buffer);
    GetDlgItemText(m_hwnd, IDC_PASSWORD_CONFIRM, buffer, SIZE_128 - 1);
    password2.SetAs(buffer);
    
    //if the user has modified the password fields
    if(m_ispasswordchanged == wyTrue)
    {
        if(password.Compare(password2))
        {
            MessageBox(m_hwnd, _(L"Passwords do not match"), 
                       pGlobals->m_appname.GetAsWideChar(), 
                       MB_OK | MB_ICONINFORMATION);
            return wyFalse;
        }

        //prepare and execute the query
        query.Sprintf("GRANT USAGE ON *.* TO '%s'@'%s' IDENTIFIED BY '%s'",
            EscapeMySQLString(m_username.GetString(), tempuser).GetString(),
            EscapeMySQLString(m_host.GetString(), temphost).GetString(),
            EscapeMySQLString(password.GetString(), temppassword).GetString());

        if(ExecuteUMQuery(query) == wyFalse)
        {
            return wyFalse;
        }
    }

    return wyTrue;
}

//the function creates the privilege tables
void
UserManager::CreatePrivilegeTables()
{
    wyString    query, temp, constraint, errormsg;
    wyInt32     i;

    query.Sprintf("CREATE TABLE %s(", U_ORIG_PRIVTABLE);
    
    temp.SetAs("DB_NAME VARCHAR(70), \
                OBJECT_NAME VARCHAR(70), \
                COLUMN_NAME VARCHAR(70), \
                OBJECT_TYPE INT");

    for(i = 0; i < m_privcount; ++i)
    {
        temp.AddSprintf(", ");
        temp.AddSprintf("[%s] INT DEFAULT -1", m_privarray[i]->priv.GetString());
    }

    constraint.SetAs("PRIMARY KEY(DB_NAME, OBJECT_NAME, COLUMN_NAME, OBJECT_TYPE)");

    //create a table to store the original privileges
    query.Sprintf("CREATE TABLE %s(%s, %s)", U_ORIG_PRIVTABLE, temp.GetString(), constraint.GetString());
    m_sqlite.Execute(&query, &errormsg);

    //create a table as a working copy of the original user privileges
    query.Sprintf("CREATE TABLE %s(%s, %s)", U_PRIVTABLE, temp.GetString(), constraint.GetString());
    m_sqlite.Execute(&query, &errormsg);
}

wyString& 
UserManager::EscapeSQLiteString(const wyChar* str, wyString& string)
{
    wyString temp;

    temp.SetAs(str);
    temp.FindAndReplace("'", "''");
    string.SetAs(temp);
    return string;
}

wyString& 
UserManager::EscapeMySQLString(const wyChar* str, wyString& string)
{
    wyChar buffer[SIZE_512];

    m_hmdi->m_tunnel->mysql_real_escape_string(m_hmdi->m_mysql, buffer, str, strlen(str));
    string.SetAs(buffer);
    return string;
}

//function truncates desttable table and copies the content of other to the desttable
void
UserManager::TruncateAndReplacePrivTable(wyChar* desttable, wyChar* srctable, wyBool validate)
{
    wyString    query, errmsg, tempstring;
    wyInt32     i, temp;
    wyBool      flag;

    //truncate the dest table
    query.Sprintf("DELETE FROM %s", desttable);
    m_sqlite.Execute(&query, &errmsg);

    //get the data from src table
    query.Sprintf("SELECT * FROM %s", srctable);
    m_sqlite.Prepare(NULL, (wyChar*)query.GetString());

    while(m_sqlite.Step(NULL, wyFalse) && m_sqlite.GetLastCode() == SQLITE_ROW)
    {
        query.Sprintf("INSERT INTO %s VALUES(", desttable);
        flag = wyFalse;

        for(i = 0; i < m_privcount + 4; ++i)
        {
            if(i != 0)
            {
                query.Add(", ");
            }

            if(i < 3)
            {
                query.AddSprintf("'%s'", EscapeSQLiteString(m_sqlite.GetText(NULL, i), tempstring).GetString());
            }
            else
            {
                temp = m_sqlite.GetInt(NULL, i);

                if(i > 3 && temp == 1)
                {
                    flag = wyTrue;
                }

                query.AddSprintf("%d", temp);
            }
        }

        //validate before forming the query
        //this helps to populate the modified state after saving changes without querying MySQL server
        if(validate == wyTrue && flag == wyFalse)
        {
            continue;
        }

        query.Add(")");

        //execute the insert query
        m_sqlite.Execute(&query, &errmsg);
    }

    m_sqlite.Finalize(NULL);
}

//function handles any relevent WM_COMMAND messages
wyBool
UserManager::OnWMCommand(WPARAM wparam, LPARAM lparam)
{
    wyInt32 ret;
    //for any edit control changes
    if(m_initcompleted == wyTrue && 
       (HIWORD(wparam) == EN_CHANGE || HIWORD(wparam) == CBN_EDITCHANGE))
    { 
        if(LOWORD(wparam)!= IDC_USERCOMBO)	
			SetDirtyFlag(wyTrue);

		if(LOWORD(wparam) == IDC_PASSWORD ||
           LOWORD(wparam) == IDC_PASSWORD_CONFIRM)
        {
            m_ispasswordchanged = wyTrue;
        }

		if(LOWORD(wparam)== IDC_USERCOMBO){
			OnHandleEditChange();
		}

        return wyTrue;
    }
	if(m_initcompleted == wyTrue && 
       (HIWORD(wparam) == CBN_SELCHANGE && LOWORD(wparam) == IDC_USERCOMBO))
	{
		if(!SendMessage(GetDlgItem(m_hwnd,IDC_USERCOMBO), CB_GETDROPPEDSTATE, NULL, NULL))
			OnUserComboChange();
	}

    switch(LOWORD(wparam))
    {
        case IDC_PRIVILEGEDCHECK:
            ret = SendMessage(GetDlgItem(m_hwnd, IDC_PRIVILEGEDCHECK), BM_GETCHECK, 0, 0);
            
            //update the SQLite working copy with any pending changes before switching the views 
            OnTreeViewSelChanging(NULL);

            //flag that tells which view is selected
            m_isallradiochecked = (ret == BST_CHECKED) ? wyFalse : wyTrue;

            ShowAllObjects(m_isallradiochecked);

            //clean up
            OnTreeViewSelChanged(NULL);
            break;

        case IDC_SELECTALLCHECK:
            OnSelectAllCheck();
            break;

        case IDC_NEWUSER:
            OnNewUser();
            break;

        case IDC_DELUSER:
            OnDeleteUser();
            break;

        case IDC_USERCOMBO:
			///check upon closeup, not when changed
           if(HIWORD(wparam) == CBN_CLOSEUP)// || HIWORD(wparam) == CBN_SELCHANGE)
            {
					OnUserComboChange();
            }
            break;

        case IDC_HOSTNAME:
            if(HIWORD(wparam) == CBN_SELCHANGE && m_initcompleted)
            {
                SetDirtyFlag();
            }
            break;

        case IDC_CANCEL_CHANGES:
            OnCancelChanges();
            break;

        case IDC_SAVE_CHANGES:
            OnSaveChanges();
            break;

        case IDC_UM_HELP:
            HandleHelp(wyTrue);
            break;

        case IDCANCEL:
            
            if(SavePrompt() == wyFalse)
            {
                break;
            }
            
            EndDialog(m_hwnd, 0);
            break;

        default:
            return wyFalse;
    }

    return wyTrue;
}

//function handles the save button 
void
UserManager::OnSaveChanges()
{
    HWND        hwndcancel = GetDlgItem(m_hwnd, IDC_CANCEL_CHANGES);
    HWND        hwndsave = GetDlgItem(m_hwnd, IDC_SAVE_CHANGES);
    HWND        hwndcombo = GetDlgItem(m_hwnd, IDC_USERCOMBO);
    HWND        hwndobtree = GetDlgItem(m_hwnd, IDC_OBTREE);
    HWND        hwndprivtree = GetDlgItem(m_hwnd, IDC_PRIVOBTREE);
    HTREEITEM   htemp;
    wyString    user;

    //if it is to create a new user
    if(m_isnewuser == wyTrue)
    {
        if(AddNewUser() == wyFalse)
        {
            return;
        }

        //flush the privileges
        FlushPrivileges();

        //add the created user to the combo box and internaly call the CBN_CHANGE handler. 
        user.Sprintf("%s@%s", m_username.GetString(), m_host.GetString());
        m_selindex = SendMessage(hwndcombo, CB_ADDSTRING, 0, (LPARAM)user.GetAsWideChar());
        SendMessage(hwndcombo, CB_SETITEMDATA, (WPARAM)m_selindex, (LPARAM)m_username.GetLength());
        SendMessage(hwndcombo, CB_SETCURSEL, (WPARAM)m_selindex, 0);
		
        SetWindowText(hwndcancel, U_CANCELBUTTON_TEXT);
        SetWindowText(hwndsave, U_SAVEBUTTON_TEXT);
        m_isedited = wyFalse;
        OnUserComboChange();

        //show all objects 
        SendMessage(GetDlgItem(m_hwnd, IDC_PRIVILEGEDCHECK), BM_SETCHECK, (WPARAM)BST_UNCHECKED, 0);
        m_isallradiochecked = wyTrue;
        ShowAllObjects();
        
        //select the global privilege initially
        htemp = TreeView_GetRoot(hwndprivtree);
        htemp = TreeView_GetChild(hwndprivtree, htemp);
        htemp = TreeView_GetNextSibling(hwndprivtree, htemp);
        TreeView_SelectItem(hwndprivtree, htemp);

        htemp = TreeView_GetRoot(hwndobtree);
        htemp = TreeView_GetChild(hwndobtree, htemp);
        TreeView_SelectItem(hwndobtree, htemp);

        //reset the flags
        m_isnewuser = wyFalse;
        EnableDisableSaveCancel();
//		SendMessage(hwndcombo, CB_DELETESTRING, m_selindex, 0);
		SendMessage(hwndcombo, CB_SETCURSEL,(WPARAM)m_selindex, 0);
		SetFocus(GetDlgItem(m_hwnd, IDC_OBTREE));
    }
    else
    {
        //apply the changes to the curretn user
        ApplyChanges(wyTrue);
        SetFocus(GetTreeViewHandle());
    }
}

//function adds a new user
wyBool
UserManager::AddNewUser()
{
    wyString    query, username, host, password, password2, tempuser, temphost, temppassword, temp;
    wyWChar     buffer[SIZE_128];
    wyInt32     ret;

    GetDlgItemText(m_hwnd, IDC_USERNAME, buffer, SIZE_128 - 1);
    username.SetAs(buffer);
    username.RTrim();
    GetDlgItemText(m_hwnd, IDC_HOSTNAME, buffer, SIZE_128 - 1);
    host.SetAs(buffer);
    host.RTrim();

    //get the passwords
    GetDlgItemText(m_hwnd, IDC_PASSWORD, buffer, SIZE_128 - 1);
    password.SetAs(buffer);
    GetDlgItemText(m_hwnd, IDC_PASSWORD_CONFIRM, buffer, SIZE_128 - 1);
    password2.SetAs(buffer);

    //compare the passwords
    if(password.Compare(password2))
    {
        MessageBox(m_hwnd, _(L"Passwords do not match"), 
                   pGlobals->m_appname.GetAsWideChar(), 
                   MB_OK | MB_ICONINFORMATION);
        return wyFalse;
    }

    //validate the user name and form the error string
    if(!username.GetLength())
    {
        ret = MessageBox(m_hwnd, _(L"You have not specifed a username. Do you want to create anonymous user?"), 
                         pGlobals->m_appname.GetAsWideChar(), 
                         MB_YESNO | MB_ICONQUESTION);

        if(ret != IDYES)
        {
            return wyFalse;
        }
    }

    EscapeMySQLString(username.GetString(), tempuser);
    EscapeMySQLString(host.GetString(), temphost);
    EscapeMySQLString(password.GetString(), temppassword);

    //if it is mysql version > 5.02 use the CREATE USER stmt
    if(m_ismysql502 == wyTrue)
    {
        query.Sprintf("CREATE USER '%s'@'%s' IDENTIFIED BY '%s'",
            tempuser.GetString(),
            temphost.GetString(),
            temppassword.GetString());

        if(ExecuteUMQuery(query) == wyFalse)
        {
            return wyFalse;
        }
    }
    //use the INSERT stmt to insert into user table
    else
    {
        query.Sprintf("INSERT INTO mysql.user(`User`, `Host`, `Password`) VALUES('%s', '%s', PASSWORD('%s'))",
            tempuser.GetString(),
            temphost.GetString(),
            temppassword.GetString());

        if(ExecuteUMQuery(query) == wyFalse)
        {
            return wyFalse;
        }
    }
	
    m_username.SetAs(username);
    m_host.SetAs(host);

	temp.SetAs(username.GetString());
	temp.AddSprintf("@%s",host.GetString()); 

	USERLIST *tempnode = new USERLIST; 
	tempnode->m_uname.SetAs(temp.GetString());
	//tempnode->m_dropdown = wyFalse;
	tempnode->m_dropdown = wyTrue;
	tempnode->m_itemvalue.Sprintf("%d",m_usercount);
	tempnode->next = NULL;

	USERLIST *itr = m_userlist;
	while(itr->next)
		itr = itr->next;
	itr->next = tempnode;



    ApplyLimitations();
	m_usercount += 1;
    return wyTrue;
}

//handler for select all check box
void
UserManager::OnSelectAllCheck()
{
    BOOL    checkstate;
    wyInt32 i, itemcount;
    HWND    hwndlistview  = GetDlgItem(m_hwnd, IDC_PRIVLIST);
    
    //get the check state
    if(SendMessage(GetDlgItem(m_hwnd, IDC_SELECTALLCHECK), BM_GETCHECK, 0, 0) == BST_CHECKED)
    {
        checkstate = TRUE;
    }
    else
    {
        checkstate = FALSE;
    }

    //get the item count in list view and set the flag
    itemcount = ListView_GetItemCount(hwndlistview);
    m_isselectallcheck = wyTrue;

    //check all the items in the list view
    for(i = 0; i < itemcount; ++i)
    {
        ListView_SetCheckState(hwndlistview, i, checkstate);
    }

    //reset the flags
    m_isselectallcheck = wyFalse;

    //set the dirty flag
    SetDirtyFlag();
}

//function populates user combo
wyInt32
UserManager::PopulateUserCombo()
{
    wyString    query;
    MYSQL_RES*  myres;
    MYSQL_ROW   row;
    wyString    temp;
    wyInt32     retflag = 0, index, length, i=0;
    HWND        hwndusercombo = GetDlgItem(m_hwnd, IDC_USERCOMBO);

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    //reset the combo box contents
    SendMessage(hwndusercombo, CB_RESETCONTENT, 0, 0);
	query.SetAs("SELECT COUNT(`Host`) FROM `mysql`.`user`");
	myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);

    if(myres == NULL)
    {
        OnUMError(query.GetString(), wyTrue);
        return -1;
    }
	if((row =  m_hmdi->m_tunnel->mysql_fetch_row(myres)))
    {
        temp.SetAs(row[0], m_hmdi->m_ismysql41);
		m_usercount = temp.GetAsInt32();
	}
	m_hmdi->m_tunnel->mysql_free_result(myres);

    //execute the query and get all the entries
    query.SetAs("SELECT `Host`, `User` FROM `mysql`.`user`");
    myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);

    if(myres == NULL)
    {
        OnUMError(query.GetString(), wyTrue);
        return -1;
    }

    //add it to combo box in the ascending order
	USERLIST *itr;
    while((row =  m_hmdi->m_tunnel->mysql_fetch_row(myres)))
    {
		USERLIST *tempnode = new USERLIST;
        temp.SetAs(row[1], m_hmdi->m_ismysql41);
        length = temp.GetLength();
        temp.AddSprintf("@%s", row[0]);        
        index = SendMessage(hwndusercombo, CB_ADDSTRING, 0, (LPARAM)temp.GetAsWideChar());
        SendMessage(hwndusercombo, CB_SETITEMDATA, (WPARAM)index, (LPARAM)length);
		tempnode->m_uname.SetAs(temp.GetString());
		tempnode->m_dropdown = wyTrue;
		tempnode->m_itemvalue.Sprintf("%d",i);
		tempnode->next = NULL;
		i++;
		if(!m_userlist)
		{	
			itr = tempnode;
			m_userlist = tempnode;
		}
		else
		{
			itr->next = tempnode;
			itr = itr->next;
		}
    }

    m_hmdi->m_tunnel->mysql_free_result(myres);

    //set the initial selection index in the combo box
    m_selindex = 0;
    SendMessage(hwndusercombo, CB_SETCURSEL, (WPARAM)m_selindex, 0);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return retflag;
}

//function populates the user privileges
wyBool
UserManager::PopulateUserInfo()
{
    TVITEM      tvi = {0};
    HWND        hwndobtree = GetDlgItem(m_hwnd, IDC_OBTREE);
    wyString    user;

    //set the root item
    user.Sprintf("%s@%s", m_username.GetString(), m_host.GetString());
    tvi.mask = TVIF_TEXT;
    tvi.hItem = TreeView_GetRoot(hwndobtree);
    tvi.pszText = user.GetAsWideChar();
    tvi.cchTextMax = user.GetLength() + 1;
    TreeView_SetItem(hwndobtree, &tvi);

    //get the user privileges
    if(m_isnewuser == wyFalse)
    {
        GetUserPrivileges();
    }

    //insert the privileged objects
    InsertPrivilegedObject();
    return wyTrue;
}

//function prompts the user to save the changes and saves/discards according to users choice
wyBool
UserManager::SavePrompt()
{
    wyInt32 ret;
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
	{
		ApplyChanges(wyFalse);
		return wyTrue;
	}
#endif

    //check whether there is an unsaved change
    if(m_isedited == wyTrue)
    {
        ret = MessageBox(m_hwnd, _(L"Do you want to save the changes?"), 
                         pGlobals->m_appname.GetAsWideChar(), MB_ICONQUESTION | MB_YESNOCANCEL);
        
        if(ret == IDNO)
        {
            //discard the changes
            ApplyChanges(wyFalse);
        }
        else if(ret == IDYES)
        {
            //save the changes
            return ApplyChanges(wyTrue);
        }
        else
        {
            return wyFalse;
        }
    }

    return wyTrue;
}

//handler for new user button
wyBool
UserManager::OnNewUser()
{
    HICON hicon;

    //prompt if any unsaved changes is detected
    if(SavePrompt() == wyFalse)
    {
        return wyFalse;
    }

    EnableUserControls();   
    SetDlgItemText(m_hwnd, IDC_CANCEL_CHANGES, _(L"&Cancel"));
    SetDlgItemText(m_hwnd, IDC_SAVE_CHANGES, _(L"Creat&e"));
    //SetDlgItemText(m_hwnd, IDC_OBJECTINFO, U_NEW_USER);

    //change the icon
    SetDlgItemText(m_hwnd, IDC_CONTEXTHELP, U_CONTEXTHELP_ADDUSER);
    hicon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_USER_ADD));
    SendDlgItemMessage(m_hwnd, IDC_CONTEXTICON, STM_SETICON, (WPARAM)hicon, 0);
    DestroyIcon(hicon);

    //hide unwanted controls and set the focus
    ShowHideControls(m_umimageindex);
    SetFocus(GetDlgItem(m_hwnd, IDC_USERNAME));

    //set the flags
    m_isnewuser = wyTrue;
    InvalidateRect(m_hwnd, NULL, FALSE);
    UpdateWindow(m_hwnd);
    return wyTrue;
}

//enumeration procedure to enable/disable the controls
BOOL CALLBACK 
UserManager::EnableChildren(HWND hwnd, LPARAM lparam)
{
    BOOL    enable = (BOOL)lparam;
    wyInt32 ctrlid = GetDlgCtrlID(hwnd);   

    if(ctrlid != IDC_SAVE_CHANGES && ctrlid != IDC_CANCEL_CHANGES)
    {
        EnableWindow(hwnd, enable);
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue && (ctrlid == IDC_DELUSER || ctrlid == IDC_NEWUSER))
	{
		EnableWindow(hwnd, wyFalse);
	}
#endif
    }

    return TRUE;
}

//function handles the help
void
UserManager::HandleHelp(wyBool ishelpclicked)
{
    ShowHelp("http://sqlyogkb.webyog.com/category/129-user-management");
}

//handler function for CBN_CHANGE. this is an import function as this alone triggers all the other functions to populate the user details
void
UserManager::OnUserComboChange()
{
    wyString    curruser;
    HWND        hwndusercombo = GetDlgItem(m_hwnd, IDC_USERCOMBO);
    wyWChar*    buffer;
    wyInt32     len;
    wyChar*     temp;

    //return for any unwanted calls
    if(m_initcompleted == wyTrue && 
       m_isnewuser == wyFalse &&
       m_isdeleteuser == wyFalse &&
       m_selindex == SendMessage(hwndusercombo, CB_GETCURSEL, 0, 0))
    {
		len = SendMessage(hwndusercombo, CB_GETLBTEXTLEN, (WPARAM)m_selindex, 0) + 1;
		buffer = new wyWChar[len + 1];
		SendMessage(hwndusercombo, CB_GETLBTEXT, (WPARAM)m_selindex, (LPARAM)buffer);
		wyString user;
		user.SetAs(buffer);
		delete buffer;
		if(user.CompareI(m_username) == 0)
			return;
    }

    //prompt if there is any unsaved changes
    if(SavePrompt() == wyFalse)
    {
        SendMessage(hwndusercombo, CB_SETCURSEL, (WPARAM)m_selindex, 0);
        return;
    }
    
    //set the flag to identify that some evnet handlers are called from this function
    m_isusercombochanging = wyTrue;

    //set the selected index
    m_selindex = SendMessage(hwndusercombo, CB_GETCURSEL, 0, 0);

    if(m_selindex == -1)
    {
		wyWChar     str[70] = {0};

		if(GetWindowText(GetDlgItem(m_hwnd,IDC_USERCOMBO), str, 65))
		if(SendMessage(GetDlgItem(m_hwnd,IDC_USERCOMBO), CB_FINDSTRING, -1,(LPARAM)str) != CB_ERR)
		{
			EndDialog(m_hwnd, 0);
			MessageBox(m_hwnd, 
                   _(L"User Manager cannot continue with empty user set. Closing now"), 
                   pGlobals->m_appname.GetAsWideChar(),
                   MB_OK | MB_ICONINFORMATION);
		}
        return;
		
    }

    //set the current user name and host name
    len = SendMessage(hwndusercombo, CB_GETLBTEXTLEN, (WPARAM)m_selindex, 0) + 1;
    buffer = new wyWChar[len + 1];
    SendMessage(hwndusercombo, CB_GETLBTEXT, (WPARAM)m_selindex, (LPARAM)buffer);
    curruser.SetAs(buffer);
    delete[] buffer;
	len =  curruser.FindIWithReverse("@", 0 , wyTrue);
    temp = curruser.Substr(0, curruser.GetLength() - len - 1);
    m_username.SetAs(temp ? temp : "");
    temp = curruser.Substr(curruser.GetLength() - len, len);
    m_host.SetAs(temp ? temp : "");

    //delete the user level items
    DeleteUserLevelItems();

    //poulate the user info
    PopulateUserInfo();

    //fill the user infos
    FillUserInfo();

    //reset the flags
    SetDirtyFlag(wyFalse);
    m_ispasswordchanged = wyFalse;
    m_isusercombochanging = wyFalse;

    //automate the rest
    OnTreeViewSelChanged();
}

void
UserManager::OnHandleEditChange()
{
    int			id = -1, status = 0, index;
    int         len = -1, textlen=-1;
	wyInt32		temp = 0;
	wyWChar     str[70] = {0};
	wyChar		str1[140] = {0};
	wyWChar		textstr[70] = {0};
    HWND        hwndusercombo = GetDlgItem(m_hwnd, IDC_USERCOMBO);
	USERLIST *itr = m_userlist;

	len = GetWindowText(hwndusercombo, str, 65);
	int ret = wcstombs ( str1, str, sizeof(str1) ); 
	if(len)
	{	
		while(itr)
		{	 
			if(itr->m_uname.GetLength() == 0)
			{
				itr = itr->next;
				continue;
			}
			wyString t;
			t.SetAs(itr->m_uname.GetAsWideChar());
			temp = t.FindI(str1, 0);
			if(temp != -1)
			{
				status=1;
			}
			if(temp != -1 && itr->m_dropdown == wyFalse)
			{
				index=SendMessage(hwndusercombo, CB_ADDSTRING, 0,(LPARAM)itr->m_uname.GetAsWideChar());
				VERIFY(SendMessage(hwndusercombo, CB_SETITEMDATA, index,itr->m_itemvalue.GetAsInt32()));
				itr->m_dropdown=wyTrue;	
			}
			
			if(temp == -1 && itr->m_dropdown==wyTrue)
			{
				 int index_delete=SendMessage(hwndusercombo,CB_FINDSTRINGEXACT,-1,(LPARAM)itr->m_uname.GetAsWideChar());
				 SendMessage(hwndusercombo, CB_DELETESTRING, index_delete,NULL);
				 itr->m_dropdown=wyFalse;
			}
			itr = itr->next;
	   }
	
	
	}
	
	if(!len || status == 0)
	{
		SendMessage(hwndusercombo, CB_SETCURSEL, -1, 0);
		
		for(itr = m_userlist; itr; itr = itr->next)
		{
			if(itr->m_uname.GetLength())
			{			
				if(itr->m_dropdown == wyFalse)
				{
					index=SendMessage(hwndusercombo, CB_ADDSTRING, 0,(LPARAM)itr->m_uname.GetAsWideChar());
					VERIFY(SendMessage(hwndusercombo, CB_SETITEMDATA, index, (LPARAM)itr->m_itemvalue.GetAsInt32()));
					itr->m_dropdown=wyTrue;
				}		
			}
		}

		if(len)
		{
			SetWindowText(hwndusercombo,str);
			if(SendMessage(hwndusercombo, CB_GETDROPPEDSTATE, NULL, NULL) == FALSE)
			{
				SendMessage(hwndusercombo, CB_SHOWDROPDOWN, TRUE, NULL);
				SetCursor(LoadCursor(NULL, IDC_ARROW));					
			}
			SendMessage(hwndusercombo, CB_SETEDITSEL, NULL, MAKELPARAM(0,-1));
		}		
	}
	else
	{
		if(SendMessage(hwndusercombo, CB_GETDROPPEDSTATE, NULL, NULL) == FALSE)
		{
			SendMessage(hwndusercombo, CB_SHOWDROPDOWN, TRUE, NULL);
			SetCursor(LoadCursor(NULL, IDC_ARROW));					
		}
		int indexh = SendMessage(hwndusercombo, CB_FINDSTRING, -1,(LPARAM)str);
		if(indexh != CB_ERR)
			SendMessage(hwndusercombo, CB_SETCURSEL, indexh, NULL);
		SetWindowText(hwndusercombo,str);	
		SendMessage(hwndusercombo, CB_SETEDITSEL, NULL, MAKELPARAM(0,-1));
		SendMessage(hwndusercombo, CB_SETEDITSEL, NULL, MAKELPARAM(-1,-1));
	}
	
     
}

//function shows/hides the respective tree view
void
UserManager::ShowAllObjects(wyBool show)
{
    if(show == wyTrue)
    {
        ShowWindow(GetDlgItem(m_hwnd, IDC_PRIVOBTREE), SW_HIDE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_OBTREE), SW_SHOW);
    }
    else
    {
        ShowWindow(GetDlgItem(m_hwnd, IDC_OBTREE), SW_HIDE);
        ShowWindow(GetDlgItem(m_hwnd, IDC_PRIVOBTREE), SW_SHOW);
    }
}

//inserts the 3 basic nodes in the tree view
HTREEITEM
UserManager::InitTreeViewNodes(HWND hwndtv)
{
    TVINSERTSTRUCT  insertstruct;
    HTREEITEM       huser, hglobalpriv, hobjpriv;
    wyString        curruser;

    TreeView_DeleteAllItems(hwndtv);

    curruser.Sprintf("%s@%s", m_username.GetString(), m_host.GetString());
    
    insertstruct.hInsertAfter = TVI_SORT;
    insertstruct.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_STATE;
    insertstruct.hParent = NULL;
    insertstruct.item.state = TVIS_EXPANDEDONCE;

    //insert the user node
    insertstruct.item.iImage = m_umimageindex;
    insertstruct.item.iSelectedImage = m_umimageindex;
    insertstruct.item.pszText = curruser.GetAsWideChar();
    insertstruct.item.cchTextMax = curruser.GetLength();
    huser = TreeView_InsertItem(hwndtv, &insertstruct);

    //insert the global priv node
    insertstruct.hParent = huser;
    insertstruct.item.iImage = m_umimageindex + 1;
    insertstruct.item.iSelectedImage = m_umimageindex + 1;
    insertstruct.item.pszText = _(L"Global Privileges");
    insertstruct.item.cchTextMax = wcslen(insertstruct.item.pszText);
    hglobalpriv = TreeView_InsertItem(hwndtv, &insertstruct);

    //insert the object priv node
    insertstruct.hInsertAfter = hglobalpriv;
    insertstruct.item.iImage = m_umimageindex + 2;
    insertstruct.item.iSelectedImage = m_umimageindex + 2;
    insertstruct.item.pszText = _(L"Object Level Privileges");
    insertstruct.item.cchTextMax = wcslen(insertstruct.item.pszText);
    hobjpriv = TreeView_InsertItem(hwndtv, &insertstruct);

    TreeView_Expand(hwndtv, huser, TVE_EXPAND);

    //TreeView_SelectItem(hwndtv, huser);
    return hobjpriv;
}

//function initializes the tree view
void
UserManager::InitTreeView()
{
    //create the image list
    CreateImageList();

    //set the image list
	TreeView_SetImageList(GetDlgItem(m_hwnd, IDC_OBTREE), m_himagelist, TVSIL_NORMAL);
    TreeView_SetImageList(GetDlgItem(m_hwnd, IDC_PRIVOBTREE), m_himagelist, TVSIL_NORMAL);

    //insert the databases in the IDC_OBTREE tree view
    InsertDatabases();

    //automate the user selection
    OnUserComboChange();
}

//function creates the image list and some icons
void
UserManager::CreateImageList()
{
    wyInt32 count;
	HICON   hicon;

    wyInt32 imgres[] = {
        IDI_SERVER, IDI_DATABASE, IDI_OPEN, IDI_TABLE, IDI_COLUMN, 
		IDI_INDEX, IDI_OPEN, IDI_PROCEDURE, IDI_OPEN, IDI_FUNCTION, 
		IDI_OPEN, IDI_VIEW, IDI_OPEN, IDI_TRIGGER, IDI_OPEN, IDI_EVENT, IDI_OPEN, IDI_PRIMARYKEY, IDI_PRIMARYKEY,
        IDI_USER_16, IDI_USER_GLOBALPRIV, IDI_USER_OBJECTPRIV
    };

    m_imagecount = sizeof(imgres)/sizeof(imgres[0]);
    m_umimageindex = m_imagecount - 3;

    VERIFY(m_himagelist = ImageList_Create(16, 16, ILC_COLOR32 | ILC_MASK , m_imagecount, 0));
	
    for(count = 0; count < m_imagecount; ++count)
	{
        hicon = (HICON)LoadImage(pGlobals->m_hinstance, 
                                 MAKEINTRESOURCE(imgres[count]), IMAGE_ICON, 
                                 16, 16, LR_DEFAULTCOLOR);

		ImageList_AddIcon(m_himagelist, hicon);
		DestroyIcon(hicon);
	}

    m_husericon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_USER_WRITE));    
    m_hglobalprivicon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_USER_GLOBALPRIV));
    m_hobjprivicon = LoadIcon(pGlobals->m_hinstance, MAKEINTRESOURCE(IDI_USER_OBJECTPRIV));
}

//handler for notification
wyBool
UserManager::OnWMNotify(WPARAM wparam, LPARAM lparam)
{
    LPNMHDR pnmh = (LPNMHDR)lparam;

    switch(pnmh->code)
    {
        case TVN_ITEMEXPANDING:
            return OnTreeViewItemExpanding(lparam);
            break;

        case TVN_SELCHANGING:
            OnTreeViewSelChanging(lparam);
            break;

        case TVN_SELCHANGED:
            OnTreeViewSelChanged(lparam);
            break;

        case LVN_ITEMCHANGED:
            OnListViewItemChanged(lparam);
            break;

        default:
            return wyFalse;
    }

    return wyTrue;
}

//when an item in the tree view is about to expand
wyBool
UserManager::OnTreeViewItemExpanding(LPARAM lparam)
{
    LPNMTREEVIEW    pnmtv = (LPNMTREEVIEW)lparam;
    TVITEM			tvi;
	TREEVIEWPARAMS  treeviewparam = {0};
    HWND            hwndtv = GetDlgItem(m_hwnd, IDC_OBTREE);
    
    //if it is from the IDC_PRIVOBTREE, i.e the one showing only privileged objects, 
    //then we need to return as any childs for any item in this tree view is allready poulated
    if(pnmtv->hdr.hwndFrom == GetDlgItem(m_hwnd, IDC_PRIVOBTREE))
    {
        TreeView_SelectItem(GetDlgItem(m_hwnd, IDC_PRIVOBTREE), pnmtv->itemNew.hItem);
        return wyTrue;
    }

    //fill the structure and call the generic module to pupulate the nodes on demand
	tvi = pnmtv->itemNew;
	treeviewparam.database = NULL;
    treeviewparam.hwnd = hwndtv;
	treeviewparam.isopentables = wyFalse;
	treeviewparam.isrefresh = wyTrue;
	treeviewparam.issingletable = wyFalse;
    treeviewparam.mysql = &m_hmdi->m_mysql;
    treeviewparam.tunnel = m_hmdi->m_tunnel;
	treeviewparam.tvi = &tvi;
	treeviewparam.checkboxstate = wyFalse;

    return (OnItemExpanding(treeviewparam, wyFalse, wyTrue) == wyTrue) ? wyFalse : wyTrue;
}

//when an item in the list view is changed
void
UserManager::OnListViewItemChanged(LPARAM lparam)
{
    LPNMLISTVIEW    lpnmlist = (LPNMLISTVIEW)lparam;
    HWND            hwndlistview  = GetDlgItem(m_hwnd, IDC_PRIVLIST);
    BOOL            checkstate = FALSE, lastcheckstate;
    wyInt32         i, j, itemcount;

    //if it is because the user clicked on slect all check, then we dont want to process it
    if(m_isselectallcheck == wyTrue)
    {
        return;
    }

    //we need the changes caused only because of checking/unchecing the item
    if((lpnmlist->uNewState & LVIS_STATEIMAGEMASK) == 0)
    {
        return;
    }

    //the code block is for implementing shift click in list view
    if((GetKeyState(VK_SHIFT) & SHIFTED) && 
       m_lastcheckedindex != -1 &&
       m_lastcheckedindex != lpnmlist->iItem)
    {
        //set the flag so that the we dont end up in an infinite loop
        m_isselectallcheck = wyTrue;

        //get the last check state of the last checked item
        lastcheckstate = ListView_GetCheckState(hwndlistview, m_lastcheckedindex);

        //get the check state of the current item
        checkstate = !ListView_GetCheckState(hwndlistview, lpnmlist->iItem);

        //set i and j so that we can loop for i <= j
        if(m_lastcheckedindex > lpnmlist->iItem)
        {
            i = lpnmlist->iItem;
            j = m_lastcheckedindex;
        }
        else
        {
            i = m_lastcheckedindex;
            j = lpnmlist->iItem;
        }

        //set the check state to be used
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
            ListView_SetCheckState(hwndlistview, i, checkstate);
            i++;
        }

        //reset the flag
        m_isselectallcheck = wyFalse;
    }

    itemcount = ListView_GetItemCount(hwndlistview);

    //loop through the list view items anding the check states. this is done for checking/unchecking select all check
    for(checkstate = TRUE, i = 0; i < itemcount; ++i)
    {
        checkstate &= ListView_GetCheckState(hwndlistview, i);
    }

    //send the message to check/uncheck the select all check
    SendMessage(GetDlgItem(m_hwnd, IDC_SELECTALLCHECK), BM_SETCHECK, (checkstate == TRUE) ? BST_CHECKED : BST_UNCHECKED, 0);
    
    //if it is not an automated change like while loading the existing privileges, set the lastchecked index and dirty flag
    if(m_isautomatedchange == wyFalse)
    {
        m_lastcheckedindex = lpnmlist->iItem;
        SetDirtyFlag(wyTrue);
    }
}

//handler function when the selection in the tree view is about to change
void
UserManager::OnTreeViewSelChanging(LPARAM lparam)
{
    TVITEM          tvi;
    LPNMTREEVIEW    lpnmt = (LPNMTREEVIEW)lparam;
    wyInt32         context;
    HWND            hwndtv = GetTreeViewHandle();
    wyBool          skipoptimization = wyFalse;
    
    //we dont process any automated changes
    if(m_isusercombochanging == wyTrue)
    {
        return;
    }

    ZeroMemory(&tvi, sizeof(TVITEM));
    tvi.mask = TVIF_IMAGE;

    if(lpnmt)
    {
        tvi.hItem = lpnmt->itemOld.hItem;
    }
    else
    {
        tvi.hItem = TreeView_GetSelection(hwndtv);
        skipoptimization = wyTrue;
        
        if(tvi.hItem == NULL)
        {
            return;
        }
    }

    //get the selected item
    TreeView_GetItem(hwndtv, &tvi);
    
    //convert the image index of the item to the context
    context = ImageIndexToContext(tvi.iImage);

    //if the context is valid, then update the SQLite working copy
    if(context != -1)
    {
        UpdateSQLite(context, skipoptimization);
    }
}

//handler function when the selection in the tree view is changed
void
UserManager::OnTreeViewSelChanged(LPARAM lparam)
{
    TVITEM          tvi;
    HWND            hwndtv = GetTreeViewHandle();
    LPNMTREEVIEW    lpnmt = (LPNMTREEVIEW)lparam;

    //we dont process any automated changes
    if(m_isusercombochanging == wyTrue)
    {
        return;
    }
    
    ZeroMemory(&tvi, sizeof(TVITEM));
    tvi.mask = TVIF_IMAGE;

    if(lpnmt == NULL)
    {
        tvi.hItem = TreeView_GetSelection(hwndtv);
    }
    else
    {
        tvi.hItem = lpnmt->itemNew.hItem;
    }

    //get the image index
    if(tvi.hItem)
    {
        TreeView_GetItem(hwndtv, &tvi);
    }
    else
    {
        tvi.iImage = m_umimageindex + 2;
    }
    
    //compare the image index and set the context help and icon accordingly
    if(tvi.iImage == m_umimageindex)
    {
        //SetDlgItemText(m_hwnd, IDC_OBJECTINFO, U_EDIT_USER);
        SetDlgItemText(m_hwnd, IDC_CONTEXTHELP, U_CONTEXTHELP_USER);
        SendDlgItemMessage(m_hwnd, IDC_CONTEXTICON, STM_SETICON, (WPARAM)m_husericon, 0);
    }
    else if(tvi.iImage == m_umimageindex + 1)
    {
        //SetDlgItemText(m_hwnd, IDC_OBJECTINFO, U_GLOBAL_PRIV);
        SetDlgItemText(m_hwnd, IDC_CONTEXTHELP, U_CONTEXTHELP_GLOBAL);
        SendDlgItemMessage(m_hwnd, IDC_CONTEXTICON, STM_SETICON, (WPARAM)m_hglobalprivicon, 0);
    }
    else
    {
        //SetDlgItemText(m_hwnd, IDC_OBJECTINFO, U_OBJECTS_PRIV);
        SetDlgItemText(m_hwnd, IDC_CONTEXTHELP, U_CONTEXTHELP_OBJECT);
        SendDlgItemMessage(m_hwnd, IDC_CONTEXTICON, STM_SETICON, (WPARAM)m_hobjprivicon, 0);
    }

    //show/hide, enable/disable controls
    ShowHideControls(tvi.iImage);
    EnumChildWindows(m_hwnd, UserManager::EnableChildren, (LPARAM)TRUE);

    //update the object info to uniqly identify the object in the server
    SetSelectedObjectInfo();
    
    //update the 
    if(tvi.iImage != m_umimageindex)
    {
        UpdateListView(tvi.iImage);
    }
}

//function to update the object info to uniqly identify the object in the server
void
UserManager::SetSelectedObjectInfo(PrivilegedObject* privobj)
{
    HWND        hwndtv;
    TVITEM      tvi;
    wyWChar     buffer[SIZE_128];
    wyInt32     context;
    HTREEITEM   hitem;

    hwndtv = privobj ? GetDlgItem(m_hwnd, IDC_PRIVOBTREE) : GetTreeViewHandle();

    if((hitem = TreeView_GetSelection(hwndtv)) == NULL)
    {
        return;
    }

    tvi.hItem = hitem;
    tvi.mask = TVIF_IMAGE;
    TreeView_GetItem(hwndtv, &tvi);    
    context = ImageIndexToContext(tvi.iImage);

    if(privobj == NULL)
    {
        m_currentdb.Clear();
        m_currentobject.Clear();
        m_currentcolumn.Clear();
    }
    else
    {
        privobj->m_objecttype = tvi.iImage;
    }

    switch(context)
    {
        case U_COLUMNS:
            //if it is column, then we need to update the column name, table name and db name
            tvi.hItem = hitem;
            tvi.mask = TVIF_TEXT | TVIF_PARAM;
            buffer[0] = 0;
            tvi.pszText = buffer;
            tvi.cchTextMax = SIZE_128;
            TreeView_GetItem(hwndtv, &tvi);

            if(privobj)
            {
                privobj->m_columnname.SetAs(tvi.pszText);
                hitem = TreeView_GetParent(hwndtv, hitem);
            }
            else
            {
                m_currentcolumn.SetAs(tvi.pszText);
                hitem = TreeView_GetParent(hwndtv, (m_isallradiochecked == wyTrue) ? TreeView_GetParent(hwndtv, hitem) : hitem);
            }

        case U_TABLES:
        case U_FUNCTIONS:
        case U_PROCEDURES:
            //if it is any object inside a db, then we need to update the object name and db name
            tvi.hItem = hitem;
            tvi.mask = TVIF_TEXT | TVIF_PARAM;
            buffer[0] = 0;
            tvi.pszText = buffer;
            tvi.cchTextMax = SIZE_128;
            TreeView_GetItem(hwndtv, &tvi);

            if(privobj)
            {
                privobj->m_objectname.SetAs(tvi.pszText);
                hitem = TreeView_GetParent(hwndtv, hitem);
            }
            else
            {
                m_currentobject.SetAs(tvi.pszText);
                hitem = TreeView_GetParent(hwndtv, (m_isallradiochecked == wyTrue) ? TreeView_GetParent(hwndtv, hitem) : hitem);
            }

        case U_DBS:
            //update the db name
            tvi.hItem = hitem;
            tvi.mask = TVIF_TEXT | TVIF_PARAM;
            buffer[0] = 0;
            tvi.pszText = buffer;
            tvi.cchTextMax = SIZE_128;
            TreeView_GetItem(hwndtv, &tvi);

            if(privobj)
            {
                privobj->m_dbname.SetAs(tvi.pszText);
            }
            else
            {
                m_currentdb.SetAs(tvi.pszText);
            }
    }
}

//function sets the range of the spin control
void
UserManager::SetUDConrolsRange(wyInt32 maxquery, wyInt32 maxupdate, wyInt32 maxconn, wyInt32 maxsimconn)
{
    SendMessage(GetDlgItem(m_hwnd, IDC_MAXQUERY_SPIN), UDM_SETRANGE32, (WPARAM)0, (LPARAM)maxquery);
    SendMessage(GetDlgItem(m_hwnd, IDC_MAXUPDATE_SPIN), UDM_SETRANGE32, (WPARAM)0, (LPARAM)maxupdate);
    SendMessage(GetDlgItem(m_hwnd, IDC_MAXCONN_SPIN), UDM_SETRANGE32, (WPARAM)0, (LPARAM)maxconn);
    SendMessage(GetDlgItem(m_hwnd, IDC_MAXSIMCONN_SPIN), UDM_SETRANGE32, (WPARAM)0, (LPARAM)maxsimconn);
}

//function shows/hides the controls based on the image index of the tree view selection
void
UserManager::ShowHideControls(wyInt32 imageindex)
{
    wyInt32 count0, count1, i;
    wyInt32 showcmd, shownote = SW_HIDE;

    wyInt32 ctrlids0[] = {
        IDC_USERNAME_PROMPT, IDC_USERNAME, 
        IDC_HOST_PROMPT, IDC_HOSTNAME, 
        IDC_PASSWORD_PROMPT, IDC_PASSWORD, 
        IDC_PASSWORD2_PROMPT, IDC_PASSWORD_CONFIRM,
        IDC_MAXQUERY_PROMPT, IDC_MAXQUERIES, IDC_MAXQUERY_SPIN,
        IDC_MAXUPDATE_PROMPT, IDC_MAXUPDATE, IDC_MAXUPDATE_SPIN,
        IDC_MAXCONN_PROMPT, IDC_MAXCONN, IDC_MAXCONN_SPIN,
        IDC_MAXCONNSIM_PROMPT, IDC_MAXSIMCONN, IDC_MAXSIMCONN_SPIN,
        IDC_HELPZERO
    };

    wyInt32 ctrlids1[] = {
        IDC_SELECTALLCHECK, IDC_PRIVLIST
    };

    count0 = sizeof(ctrlids0)/sizeof(ctrlids0[0]);
    count1 = sizeof(ctrlids1)/sizeof(ctrlids1[0]);

    for(i = 0; i < count0; ++i)
    {
        showcmd = (imageindex == m_umimageindex) ? SW_SHOW : SW_HIDE;

        switch(i)
        {
            //for max query
            case 8:
            case 9:
            case 10:
                showcmd &= (m_showlimitations[0] == wyTrue) ? shownote = SW_SHOW : SW_HIDE;
                break;

            //for max update
            case 11:
            case 12:
            case 13:
                showcmd &= (m_showlimitations[1] == wyTrue) ? shownote = SW_SHOW : SW_HIDE;
                break;

            //for max connections
            case 14:
            case 15:
            case 16:
                showcmd &= (m_showlimitations[2] == wyTrue) ? shownote = SW_SHOW : SW_HIDE;
                break;

            //for max user connections
            case 17:
            case 18:
            case 19:
                showcmd &= (m_showlimitations[3] == wyTrue) ? shownote = SW_SHOW : SW_HIDE;
                break;

            case 20:
                showcmd &= shownote;
        }
        
        ShowWindow(GetDlgItem(m_hwnd, ctrlids0[i]), showcmd);
    }

    for(i = 0; i < count1; ++i)
    {
        ShowWindow(GetDlgItem(m_hwnd, ctrlids1[i]), 
                   (imageindex == m_umimageindex) ? SW_HIDE : SW_SHOW);
    }

    ShowWindow(m_hwndnote, SW_HIDE);
}

//function sets the dirty flag
void
UserManager::SetDirtyFlag(wyBool set)
{
    if(m_isedited == set)
        return;

    m_isedited = set;
    EnableDisableSaveCancel();
}

//function enables/disables the Save/Cancel buttons
void
UserManager::EnableDisableSaveCancel()
{
    EnableWindow(GetDlgItem(m_hwnd, IDC_SAVE_CHANGES), m_isedited);
    EnableWindow(GetDlgItem(m_hwnd, IDC_CANCEL_CHANGES), m_isedited);
#ifndef COMMUNITY
	if(GetActiveWin()->m_conninfo.m_isreadonly == wyTrue)
	{
		EnableWindow(GetDlgItem(m_hwnd, IDC_SAVE_CHANGES), wyFalse);
		EnableWindow(GetDlgItem(m_hwnd, IDC_CANCEL_CHANGES), wyFalse);
	}
#endif
}

//handler function for Cancel button
void
UserManager::OnCancelChanges()
{
    HWND hwndcancel = GetDlgItem(m_hwnd, IDC_CANCEL_CHANGES);
    HWND hwndsave = GetDlgItem(m_hwnd, IDC_SAVE_CHANGES);
    
    //if it is a new user page
    if(m_isnewuser == wyTrue)
    {
        //fill the details of the currently selected user
        FillUserInfo();

        //call the handler so that we get the correct panel on the right hand side
        OnTreeViewSelChanged();
        
        //set the button texts and reset the flags
        SetWindowText(hwndcancel, U_CANCELBUTTON_TEXT);
        SetWindowText(hwndsave, U_SAVEBUTTON_TEXT);
        m_isnewuser = wyFalse;
        m_isedited = wyFalse;
        m_ispasswordchanged = wyFalse;
        EnableDisableSaveCancel();
    }
    else
    {
        //cancel any unsaved changes
        ApplyChanges(wyFalse);
    }

    SetFocus(GetTreeViewHandle());
}

//function saves/discards the changes made by the user
wyBool
UserManager::ApplyChanges(wyBool issave)
{
    wyString    query, errormsg, temp, user, userold;
	wyWChar *	buffer;
    wyInt32     retval;
    HWND        hwndobtree = GetDlgItem(m_hwnd, IDC_OBTREE);
    HWND        hwndprivtree = GetDlgItem(m_hwnd, IDC_PRIVOBTREE);
    HWND        hwndcombo = GetDlgItem(m_hwnd, IDC_USERCOMBO);
    TVITEM      tvi = {0};

    if(issave == wyFalse)
    {
        //discard the changes by truncating and replacing the working copy with the original copy
        TruncateAndReplacePrivTable(U_PRIVTABLE, U_ORIG_PRIVTABLE);
    }
    else
    {
        //update the SQLite table with any pending changes
        OnTreeViewSelChanging(NULL);

        //rename the user if required
        if(!(retval = RenameUser()))
        {
            return wyFalse;
        }
        //if it renamed the user, then we need to update the combo box and tree views
        else if(retval == 1)
        {
            //flush privileges
            FlushPrivileges();

            user.Sprintf("%s@%s", m_username.GetString(), m_host.GetString());
			wyInt32 len = SendMessage(hwndcombo, CB_GETLBTEXTLEN, (WPARAM)m_selindex, 0) + 1;
			buffer = new wyWChar[len + 1];
			SendMessage(hwndcombo, CB_GETLBTEXT, (WPARAM)m_selindex, (LPARAM)buffer);
			userold.SetAs(buffer);
			delete[] buffer;
			//update the combo box
            SendMessage(hwndcombo, CB_DELETESTRING, (WPARAM)m_selindex, 0);
            m_selindex = SendMessage(hwndcombo, CB_ADDSTRING, 0, (LPARAM)user.GetAsWideChar());
            SendMessage(hwndcombo, CB_SETITEMDATA, (WPARAM)m_selindex, (LPARAM)m_username.GetLength());
            SendMessage(hwndcombo, CB_SETCURSEL, (WPARAM)m_selindex, 0);
			for(USERLIST* itr = m_userlist; itr ;itr = itr->next)
			{
				if(userold.CompareI(itr->m_uname) == 0)
				{
					itr->m_uname.SetAs(user.GetString());
					break;
				}
			}

            //set the tree view item
            tvi.mask = TVIF_TEXT;
            tvi.cchTextMax = user.GetLength() + 1;
            tvi.pszText = user.GetAsWideChar();
            tvi.hItem = TreeView_GetRoot(hwndobtree);
            TreeView_SetItem(hwndobtree, &tvi);
            tvi.hItem = TreeView_GetRoot(hwndprivtree);
            TreeView_SetItem(hwndprivtree, &tvi);
        }

        //save any password changes
        if(SavePassword() == wyFalse)
        {
            return wyFalse;
        }

        //apply any modified limitations
        if(ApplyLimitations() == wyFalse)
        {
            return wyFalse;
        }

        //grant/revoke privileges
        if(ProcessGrantRevoke() == wyFalse)
        {
            return wyFalse;
        }

        //truncate and replace the original table with the working table and vice versa
        //this updates the original tables and eleminates any redundant rows in the working copy
        TruncateAndReplacePrivTable(U_ORIG_PRIVTABLE, U_PRIVTABLE);
        TruncateAndReplacePrivTable(U_PRIVTABLE, U_ORIG_PRIVTABLE);

        //insert privileged objects in the tree view
        InsertPrivilegedObject();
    }

    //call the handler so that we get the proper panel
    OnTreeViewSelChanged();

    //fill the user infos
    FillUserInfo();

    //reset the flags
    SetDirtyFlag(wyFalse);
    m_ispasswordchanged = wyFalse;

    return wyTrue;
}

//function applies any modified limitations
wyBool
UserManager::ApplyLimitations()
{
    wyString    query, tempuser, temphost;
    wyInt32     temp[4], i, isintransaction = 1;
    wyBool      flag = wyFalse;

    wyChar* limitations[] = {
        "MAX_QUERIES_PER_HOUR",
        "MAX_UPDATES_PER_HOUR",
        "MAX_CONNECTIONS_PER_HOUR",
        "MAX_USER_CONNECTIONS"
    };

    wyInt32 ctrlids[4] = {IDC_MAXQUERY_SPIN, IDC_MAXUPDATE_SPIN, IDC_MAXCONN_SPIN, IDC_MAXSIMCONN_SPIN};

    //form the query
    query.Sprintf("GRANT USAGE ON *.* TO '%s'@'%s' WITH", EscapeMySQLString(m_username.GetString(), tempuser).GetString(), EscapeMySQLString(m_host.GetString(), temphost).GetString());

    for(i = 0; i < U_MAXLIMITATIONS; ++i)
    {
        if(m_showlimitations[i] == wyTrue)
        {
            temp[i] = SendMessage(GetDlgItem(m_hwnd, ctrlids[i]), UDM_GETPOS32, 0, (LPARAM)NULL);

            if(temp[i] != m_limitations[i])
            {
                query.AddSprintf(" %s %d", limitations[i], temp[i]);
                flag = wyTrue;
            }
        }
    }

    //if nothing is changed, then we dont want to execute a query
    if(flag == wyFalse)
    {
        return wyTrue;
    }

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction, GetActiveWindow());

	if(isintransaction == 1)
		return wyFalse;

    if(m_hmdi->m_tunnel->mysql_affected_rows(m_hmdi->m_mysql))
    {
        ShowMySQLError(m_hwnd, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query.GetString());
        return wyFalse;
    }

    for(i = 0; i < U_MAXLIMITATIONS; ++i)
    {
        if(m_showlimitations[i] == wyTrue)
        {
            m_limitations[i] = temp[i];
        }
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return wyTrue;
}

//starting point where the grant/revoke queries are framed
wyBool
UserManager::ProcessGrantRevoke()
{
    wyString        query, queryorig;
    sqlite3_stmt*   privstmt;
    sqlite3_stmt*   privorigstmt;

    //query the working copy
    query.Sprintf("SELECT * FROM %s ORDER BY DB_NAME DESC, COLUMN_NAME DESC, OBJECT_TYPE DESC", U_PRIVTABLE);
    m_sqlite.Prepare(&privstmt, (wyChar*)query.GetString());
    
    //loop through the rows in working copy
    while(m_sqlite.Step(&privstmt, wyFalse) && m_sqlite.GetLastCode() == SQLITE_ROW)
    {
        //query the original with the details obtained from the working copy
        //this is done to prevent firing any unwanted grant/revoke query as PrepareGrantRevokeQuery() would compare the result while forming the query
        queryorig.Sprintf("SELECT * FROM %s WHERE DB_NAME = ? AND OBJECT_NAME = ? AND COLUMN_NAME = ? AND OBJECT_TYPE = %d", 
                          U_ORIG_PRIVTABLE, 
                          m_sqlite.GetInt(&privstmt, 3));
        m_sqlite.Prepare(&privorigstmt, (wyChar*)queryorig.GetString());
        m_sqlite.SetText(&privorigstmt, 1, m_sqlite.GetText(&privstmt, 0));
        m_sqlite.SetText(&privorigstmt, 2, m_sqlite.GetText(&privstmt, 1));
        m_sqlite.SetText(&privorigstmt, 3, m_sqlite.GetText(&privstmt, 2));

        if(m_sqlite.Step(&privorigstmt, wyFalse) && m_sqlite.GetLastCode() == SQLITE_ROW)
        {
            if(PrepareGrantRevokeQuery(&privstmt, &privorigstmt) == wyFalse)
            {
                return wyFalse;
            }
        }
        else
        {
            if(PrepareGrantRevokeQuery(&privstmt, NULL) == wyFalse)
            {
                return wyFalse;
            }
        }

        m_sqlite.Finalize(&privorigstmt);
    }

    m_sqlite.Finalize(&privstmt);
    return wyTrue;
}

//function that prepares the grant/revoke queries by comparing the original and working copies
wyBool
UserManager::PrepareGrantRevokeQuery(sqlite3_stmt** privstmt, sqlite3_stmt** privorigstmt)
{
    wyInt32     i, value, origvalue, objecttype;
    wyString    revokequery, grantquery, temp, type, dbname, objectname, tempbuff, tempuser, temphost;
    wyBool      grantgrant = wyFalse, revokegrant = wyFalse;
    wyBool      retval = wyFalse;

    objecttype = m_sqlite.GetInt(privstmt, 3);

    //if db name is empty means, its hould be for all dbs
    if(!dbname.Compare(m_sqlite.GetText(privstmt, 0)))
    {
        dbname.SetAs("*");
    }
    else
    {
        //dbname.Sprintf("`%s`", m_sqlite.GetText(privstmt, 0));
        tempbuff.SetAs(m_sqlite.GetText(privstmt, 0));
        tempbuff.FindAndReplace("`", "``");
        dbname.Sprintf("`%s`", tempbuff.GetString());
    }

    //if object name is empty means, its hould be for all object
    if(!objectname.Compare(m_sqlite.GetText(privstmt, 1)))
    {
        objectname.SetAs("*");
    }
    else
    {
        //objectname.Sprintf("`%s`", m_sqlite.GetText(privstmt, 1));
        tempbuff.SetAs(m_sqlite.GetText(privstmt, 1));
        tempbuff.FindAndReplace("`", "``");
        objectname.Sprintf("`%s`", tempbuff.GetString());
    }

    //now, traverse through the privilege columns in the row
    for(i = 4; i < m_privcount + 4; ++i)
    {
        //if it is -1, means it is not applicable for the object type
        if((value = m_sqlite.GetInt(privstmt, i)) == -1)
        {
            continue;
        }

        //we need an original value to compare against
        if(privorigstmt == NULL)
        {
            if(value == 0)
            {
                continue;
            }

            origvalue = 0;
        }
        else
        {
            origvalue = m_sqlite.GetInt(privorigstmt, i);
        }

        //if original value is same as current value, means we dont want this privilege in our stmts
        if(origvalue != value)
        {
            //for grant query
            if(value == 1)
            {
                //need to handle GRANT privilege seperately
                if(m_grantoptionindex == i - 4)
                {
                    grantgrant = wyTrue;
                    continue;
                }

                if(grantquery.GetLength())
                {
                    grantquery.Add(",");
                }
                
                //add the privilege to the query
                grantquery.AddSprintf(" %s", m_privarray[i - 4]->priv.GetString());

                //handle the columns
                if(objecttype == U_COLUMNS)
                {
                    //grantquery.AddSprintf("(`%s`)", m_sqlite.GetText(privstmt, 2));
                    tempbuff.SetAs(m_sqlite.GetText(privstmt, 2));
                    tempbuff.FindAndReplace("`", "``");
                    grantquery.AddSprintf("(`%s`)", tempbuff.GetString());
                }
            }
            else
            {
                //handle the GRANT privilege
                if(m_grantoptionindex == i - 4)
                {
                    revokegrant = wyTrue;
                    continue;
                }

                if(revokequery.GetLength())
                {
                    revokequery.Add(",");
                }

                //add the privilege to the query
                revokequery.AddSprintf(" %s", m_privarray[i - 4]->priv.GetString());

                //handle columns
                if(objecttype == U_COLUMNS)
                {
                    /*revokequery.AddSprintf("(`%s`)", m_sqlite.GetText(privstmt, 2));*/
                    tempbuff.SetAs(m_sqlite.GetText(privstmt, 2));
                    tempbuff.FindAndReplace("`", "``");
                    revokequery.AddSprintf("(`%s`)", tempbuff.GetString());
                }
            }
        }
    }

    EscapeMySQLString(m_username.GetString(), tempuser);
    EscapeMySQLString(m_host.GetString(), temphost);

    //set the object type
    if(objecttype == U_PROCEDURES)
    {
        type.SetAs("PROCEDURE");
    }
    else if(objecttype == U_FUNCTIONS)
    {
        type.SetAs("FUNCTION");
    }

    if(revokequery.GetLength())
    {
        temp.Sprintf("REVOKE%s ON %s %s.%s FROM '%s'@'%s'", 
                     revokequery.GetString(), 
                     type.GetString(),
                     dbname.GetString(),
                     objectname.GetString(),
                     tempuser.GetString(),
                     temphost.GetString()
                     );
        revokequery.SetAs(temp);
    }

    if(grantgrant == wyTrue && grantquery.GetLength() == 0)
    {
        grantquery.SetAs(" USAGE");
    }

    if(grantquery.GetLength())
    {
        temp.Sprintf("GRANT %s ON %s %s.%s TO '%s'@'%s' %s", 
                     grantquery.GetString(), 
                     type.GetString(),
                     dbname.GetString(),
                     objectname.GetString(),
                     tempuser.GetString(),
                     temphost.GetString(),
                     (grantgrant == wyTrue) ? "WITH GRANT OPTION" : "");
        grantquery.SetAs(temp);
    }

    //execute the query
    if((retval = ExecuteGrantRevoke(&grantquery, &revokequery)) == wyFalse)
    {
        return retval;
    }

    if(revokegrant == wyTrue)
    {
        revokequery.Sprintf("REVOKE GRANT OPTION ON %s %s.%s FROM '%s'@'%s'",
                            type.GetString(),
                            dbname.GetString(),
                            objectname.GetString(),
                            tempuser.GetString(),
                            temphost.GetString());

        retval = ExecuteGrantRevoke(NULL, &revokequery);
    }

    return retval;
}

//function executes the grant/revoke queries, order is important. it should first grant and then revoke
wyBool
UserManager::ExecuteGrantRevoke(wyString* grantquery, wyString* revokequery)
{
	wyInt32 isintransaction = 1;

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    //if grant query is present
    if(grantquery && grantquery->GetLength())
    {
        ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, *grantquery, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction, GetActiveWindow());

		if(isintransaction == 1)
			return wyFalse;

        if(m_hmdi->m_tunnel->mysql_affected_rows(m_hmdi->m_mysql))
        {
            ShowMySQLError(m_hwnd, m_hmdi->m_tunnel, &m_hmdi->m_mysql, grantquery->GetString());
            return wyFalse;
        }
    }

    //if revoke query is present
    if(revokequery && revokequery->GetLength())
    {
		isintransaction = 1;
	    ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, *revokequery, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction, GetActiveWindow());

		if(isintransaction == 1)
			return wyFalse;

        if(m_hmdi->m_tunnel->mysql_affected_rows(m_hmdi->m_mysql))
        {
            ShowMySQLError(m_hwnd, m_hmdi->m_tunnel, &m_hmdi->m_mysql, revokequery->GetString());
            return wyFalse;
        }
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return wyTrue;
}

//function executes the necessery queries to drop the user from the server
wyBool
UserManager::DropUser()
{
    wyString query, tempuser, temphost, temp;

    EscapeMySQLString(m_username.GetString(), tempuser);
    EscapeMySQLString(m_host.GetString(), temphost);
	
	temp.SetAs(tempuser.GetString());
	temp.AddSprintf("@%s",temphost.GetString());
	m_usercount -= 1;
	USERLIST *itr = m_userlist, *prev = m_userlist;
	if(temp.CompareI(itr->m_uname) == 0 )
	{
		m_userlist = itr->next;
		delete itr;
	}
	else
	{
		for(itr = itr->next; itr ;itr = itr->next, prev = prev->next)
		{
			if(temp.CompareI(itr->m_uname) == 0)
			{
				prev->next = itr->next;
				delete itr;
				break;
			}	
		}
	}

    //for MySQL > 5.02 , we use the direct DROP USER syntax 
    //else manually delete the user from the mysql tables
    if(m_ismysql502 == wyTrue)
    {
        query.Sprintf("DROP USER '%s'@'%s'",
            tempuser.GetString(),
            temphost.GetString());

        if(ExecuteUMQuery(query) == wyFalse)
        {
            return wyFalse;
        }
        return wyTrue;
    }

    //delete user table entry
    query.Sprintf("DELETE FROM mysql.user WHERE User='%s' AND Host='%s'",
        tempuser.GetString(),
        temphost.GetString());

    if(ExecuteUMQuery(query) == wyFalse)
    {
        return wyFalse;
    }

    //delete db table entry
    query.Sprintf("DELETE FROM mysql.db WHERE User='%s' AND Host='%s'",
        tempuser.GetString(),
        temphost.GetString());

    if(ExecuteUMQuery(query) == wyFalse)
    {
        return wyFalse;
    }

    //delete tables_priv table entry
    query.Sprintf("DELETE FROM mysql.tables_priv WHERE User='%s' AND Host='%s'",
        tempuser.GetString(),
        temphost.GetString());

    if(ExecuteUMQuery(query) == wyFalse)
    {
        return wyFalse;
    }

    //delete columns_privs table entry
    query.Sprintf("DELETE FROM mysql.columns_priv WHERE User='%s' AND Host='%s'",
        tempuser.GetString(),
        temphost.GetString());

    if(ExecuteUMQuery(query) == wyFalse)
    {
        return wyFalse;
    }

    //delete procs_priv table entry
    if(IsMySQL5010(m_hmdi->m_tunnel, &m_hmdi->m_mysql) == wyTrue)
    {
        query.Sprintf("DELETE FROM mysql.procs_priv WHERE User='%s' AND Host='%s'",
            tempuser.GetString(),
            temphost.GetString());

        if(ExecuteUMQuery(query) == wyFalse)
        {
            return wyFalse;
        }
    }

    return wyTrue;
}

//function sets the texts of various controls
void
UserManager::FillUserInfo()
{
    wyInt32 ctrls[] = {IDC_MAXQUERY_SPIN, IDC_MAXUPDATE_SPIN, IDC_MAXCONN_SPIN, IDC_MAXSIMCONN_SPIN};
    wyInt32 i;

    SetDlgItemText(m_hwnd, IDC_USERNAME, m_username.GetAsWideChar());
    SetDlgItemText(m_hwnd, IDC_HOSTNAME, m_host.GetAsWideChar());
    SetDlgItemText(m_hwnd, IDC_PASSWORD, U_PASSWORD);
    SetDlgItemText(m_hwnd, IDC_PASSWORD_CONFIRM, U_REPASSWORD);

    for(i = 0; i < U_MAXLIMITATIONS; ++i)
    {
        if(m_showlimitations[i] == wyTrue)
        {
            SendMessage(GetDlgItem(m_hwnd, ctrls[i]), UDM_SETPOS32, 0, (LPARAM)m_limitations[i]);
        }
    }
}

//function renames the user if any modification is found
wyInt32
UserManager::RenameUser()
{
    wyString    query, username, host, tempuser, temphost, tempusernew, temphostnew;
    wyWChar     buffer[SIZE_128];
    wyInt32     ret;

    //get the user name
    GetDlgItemText(m_hwnd, IDC_USERNAME, buffer, SIZE_128 - 1);
    username.SetAs(buffer);
    username.RTrim();

    //get the host name
    GetDlgItemText(m_hwnd, IDC_HOSTNAME, buffer, SIZE_128 - 1);
    host.SetAs(buffer);
    host.RTrim();

    //check for any modification. if no modification is found, then we simply need to return
    if(!m_username.Compare(username) && !m_host.Compare(host))
    {
        return -1;
    }

    if(!username.GetLength() && m_username.Compare(username))
    {
        ret = MessageBox(m_hwnd, _(L"You have not specifed a username. Do you want to rename it as anonymous user?"), 
                         pGlobals->m_appname.GetAsWideChar(), 
                         MB_YESNOCANCEL | MB_ICONQUESTION);

        if(ret == IDNO)
        {
            return -1;
        }
        else if(ret == IDCANCEL)
        {
            return 0;
        }
    }

    EscapeMySQLString(m_username.GetString(), tempuser);
    EscapeMySQLString(m_host.GetString(), temphost);
    EscapeMySQLString(username.GetString(), tempusernew);
    EscapeMySQLString(host.GetString(), temphostnew);

    //for MySQL > 5.02 we use RENAME USER syntax, otherwise we need to update the mysql tables
    if(m_ismysql502 == wyTrue)
    {
        query.Sprintf("RENAME USER '%s'@'%s' TO '%s'@'%s'",
                      tempuser.GetString(),
                      temphost.GetString(),
                      tempusernew.GetString(),
                      temphostnew.GetString());

        if(ExecuteUMQuery(query) == wyFalse)
        {
            return 0;
        }
        
        m_username.SetAs(username);
        m_host.SetAs(host);
        return 1;
    }

    //update user table entry
    query.Sprintf("UPDATE mysql.user SET User = '%s', Host = '%s' WHERE User='%s' AND Host='%s'",
        tempusernew.GetString(),
        temphostnew.GetString(),
        tempuser.GetString(),
        temphost.GetString());          

    if(ExecuteUMQuery(query) == wyFalse)
    {
        return 0;
    }

    //update db table entry
    query.Sprintf("UPDATE mysql.db SET User = '%s', Host = '%s' WHERE User='%s' AND Host='%s'",
        tempusernew.GetString(),
        temphostnew.GetString(),
        tempuser.GetString(),
        temphost.GetString());

    if(ExecuteUMQuery(query) == wyFalse)
    {
        return 0;
    }


    //update tables_priv table entry
    query.Sprintf("UPDATE mysql.tables_priv SET User = '%s', Host = '%s' WHERE User='%s' AND Host='%s'",
        tempusernew.GetString(),
        temphostnew.GetString(),
        tempuser.GetString(),
        temphost.GetString());

    if(ExecuteUMQuery(query) == wyFalse)
    {
        return 0;
    }

    //update columns_privs table entry
    query.Sprintf("UPDATE mysql.columns_priv SET User = '%s', Host = '%s' WHERE User='%s' AND Host='%s'",
        tempusernew.GetString(),
        temphostnew.GetString(),
        tempuser.GetString(),
        temphost.GetString());

    if(ExecuteUMQuery(query) == wyFalse)
    {
        return 0;
    }

    //update procs_priv table entry
    if(IsMySQL5010(m_hmdi->m_tunnel, &m_hmdi->m_mysql) == wyTrue)
    {
        query.Sprintf("UPDATE mysql.procs_priv SET User = '%s', Host = '%s' WHERE User='%s' AND Host='%s'",
            tempusernew.GetString(),
            temphostnew.GetString(),
            tempuser.GetString(),
            temphost.GetString());

        if(ExecuteUMQuery(query) == wyFalse)
        {
            return 0;
        }
    }

    m_username.SetAs(username);
    m_host.SetAs(host);
    return 1;
}

//helper function to flush the privileges
wyBool
UserManager::FlushPrivileges()
{
    wyString query;

    query.Sprintf("FLUSH PRIVILEGES");

    if(ExecuteUMQuery(query) == wyFalse)
    {
        return wyFalse;
    }

    return wyTrue;
}

//helper function to execute some User Manager queries
wyBool
UserManager::ExecuteUMQuery(wyString& query)
{
    MYSQL_RES*  myres;
	wyInt32 isintransaction = 1;

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query, wyTrue, wyFalse, wyTrue, false, false, wyFalse, 0, wyFalse, &isintransaction, GetActiveWindow());

	if(isintransaction == 1)
		return wyFalse;

    if(!myres && m_hmdi->m_tunnel->mysql_affected_rows(m_hmdi->m_mysql) == -1)
    {
        ShowMySQLError(m_hwnd, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query.GetString());
        return wyFalse;
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return wyTrue;
}

//function enables the user controls
void
UserManager::EnableUserControls(wyBool isnewuser)
{
     wyInt32 ctrlids0[] = {
        IDC_USERNAME_PROMPT, IDC_HOST_PROMPT, IDC_PASSWORD_PROMPT, IDC_PASSWORD2_PROMPT,
        IDC_MAXQUERY_PROMPT, IDC_MAXUPDATE_PROMPT, IDC_MAXCONN_PROMPT, IDC_MAXCONNSIM_PROMPT,
        IDC_MAXQUERY_SPIN, IDC_MAXUPDATE_SPIN, IDC_MAXCONN_SPIN,
        IDC_MAXSIMCONN_SPIN, IDC_CANCEL_CHANGES, IDCANCEL, 
        IDC_CONTEXTHELP, IDC_HELPZERO, IDC_UM_HELP, IDC_GRIP
    };

    wyInt32 ctrlids1[] = {
        IDC_USERNAME, IDC_HOSTNAME, IDC_PASSWORD, IDC_PASSWORD_CONFIRM, IDC_MAXQUERIES, IDC_MAXUPDATE,
        IDC_MAXCONN, IDC_MAXSIMCONN
    };

    wyInt32 i, size;

    EnumChildWindows(m_hwnd, UserManager::EnableChildren, (LPARAM)FALSE);
    size = sizeof(ctrlids1)/sizeof(ctrlids1[0]);

    for(i = 0; i < size; ++i)
    {
        EnableWindow(GetDlgItem(m_hwnd, ctrlids1[i]), TRUE);

        if(isnewuser == wyFalse)
        {
            continue;
        }

        if(i < 4)
        {
            if(i == 1)
            {
                SetDlgItemText(m_hwnd, ctrlids1[i], L"%");
            }
            else
            {
                SetDlgItemText(m_hwnd, ctrlids1[i], L"");
            }
        }
        else
        {
            SetDlgItemText(m_hwnd, ctrlids1[i], L"0");
        }
    }

    size = sizeof(ctrlids0)/sizeof(ctrlids0[0]);

    for(i = 0; i < size; ++i)
    {
        EnableWindow(GetDlgItem(m_hwnd, ctrlids0[i]), TRUE);
    }

    //EnableWindow(GetDlgItem(m_hwnd, IDC_SAVE_CHANGES), FALSE);
    m_isedited = wyFalse;
    m_ispasswordchanged = wyFalse;
}

//handler function for Delete User button
void
UserManager::OnDeleteUser()
{
    wyInt32 ret;
    HWND    hwndcombo = GetDlgItem(m_hwnd, IDC_USERCOMBO);

    //get the confirmation from the user
    ret = MessageBox(m_hwnd, 
                     _(L"Do you really want to delete the user?"), 
                     pGlobals->m_appname.GetAsWideChar(), 
                     MB_ICONQUESTION | MB_YESNO | MB_HELP);

    if(ret == IDYES)
    {
        m_isdeleteuser = wyTrue;

        //drop the user
        if(DropUser() == wyFalse)
        {
            return;
        }

        //flush privileges
        FlushPrivileges();

        //delete the string from the combo box and call the combo change handler to automate the operation
        SendMessage(hwndcombo, CB_DELETESTRING, (WPARAM)m_selindex, 0);
        m_selindex = 0;
		//if there are no more users with the filter applied in edit change deleting the last one will give empty editbox,
		//which shouldnt be allowed, so repopulate usercombo and select first one.
		if(SendMessage(hwndcombo, CB_GETCOUNT, 0, 0) == 0)
		{
			int index;
			for(USERLIST *itr = m_userlist; itr; itr = itr->next)
			{
				if(itr->m_uname.GetLength())
				{			
					index = SendMessage(hwndcombo, CB_ADDSTRING, 0,(LPARAM)itr->m_uname.GetAsWideChar());
					VERIFY(SendMessage(hwndcombo, CB_SETITEMDATA, index, (LPARAM)itr->m_itemvalue.GetAsInt32()));
					itr->m_dropdown=wyTrue;		
				}
			}
		}
        SendMessage(hwndcombo, CB_SETCURSEL, (WPARAM)m_selindex, 0);
        m_isedited = wyFalse;
        OnUserComboChange();
        m_isdeleteuser = wyFalse;
    }
}

//function inserts into sqlite tables
void
UserManager::InsertIntoSQLite(PrivilegedObject* privobject)
{
    wyString    query, fields, values, errmsg, tempstring1, tempstring2, tempstring3;
    wyInt32     i;
    
    fields.SetAs("DB_NAME, OBJECT_NAME, COLUMN_NAME, OBJECT_TYPE");
    values.Sprintf("'%s', '%s', '%s', %d", 
                   EscapeSQLiteString(privobject->m_dbname.GetString(), tempstring1).GetString(),
                   EscapeSQLiteString(privobject->m_objectname.GetString(), tempstring2).GetString(),
                   EscapeSQLiteString(privobject->m_columnname.GetString(), tempstring3).GetString(),
                   privobject->m_objecttype);

    for(i = 0; i < m_privcount; ++i)
    {
        fields.AddSprintf(", ");
        fields.AddSprintf("[%s]", m_privarray[i]->priv.GetString());
        values.AddSprintf(", ");
        values.AddSprintf("%d", privobject->m_privileges[i]);
    }

    query.Sprintf("INSERT INTO %s(%s) VALUES(%s)",
                  U_ORIG_PRIVTABLE,
                  fields.GetString(),
                  values.GetString());

    m_sqlite.Execute(&query, &errmsg);

    query.Sprintf("INSERT INTO %s(%s) VALUES(%s)",
                  U_PRIVTABLE,
                  fields.GetString(),
                  values.GetString());

    m_sqlite.Execute(&query, &errmsg);
}

//function inserts into/updates the working copy
void
UserManager::UpdateSQLite(wyInt32 context, wyBool skipoptimization)
{
    HWND        hwndlv = GetDlgItem(m_hwnd, IDC_PRIVLIST);
    LVITEM      lvi;
    wyWChar     buffer[70];
    wyInt32     count, i;
    wyString    query, errmsg, fields, values, temp, tempstring1, tempstring2, tempstring3;
    BOOL        state;

    count = ListView_GetItemCount(hwndlv);
    wyBool flag = wyFalse;

    //frame the query using the current selected object details
    fields.SetAs("DB_NAME, OBJECT_NAME, COLUMN_NAME, OBJECT_TYPE");
    values.Sprintf("'%s', '%s', '%s', %d", 
                   EscapeSQLiteString(m_currentdb.GetString(), tempstring1).GetString(),
                   EscapeSQLiteString(m_currentobject.GetString(), tempstring2).GetString(),
                   EscapeSQLiteString(m_currentcolumn.GetString(), tempstring3).GetString(),
                   context);

    //now loop through all the items in the list view and add it to the query
    for(i = 0; i < count; ++i)
    {
        state = ListView_GetCheckState(hwndlv, i);
        buffer[0] = 0;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.pszText = buffer;
        lvi.cchTextMax = 69;
        ListView_GetItem(hwndlv, &lvi);

        temp.SetAs(lvi.pszText);
        
        fields.AddSprintf(", ");
        fields.AddSprintf("[%s]", temp.GetString());
        values.AddSprintf(", ");
        values.AddSprintf("%d", state);

        //if atleast one item's check state is different compared to it's initial state that we set while poulating,
        //we need to execute the query
        if(skipoptimization == wyTrue || state != lvi.lParam)
        {
            flag = wyTrue;
        }
    }

    if(flag == wyFalse)
    {
        return;
    }

    query.Sprintf("INSERT OR REPLACE INTO %s(%s) VALUES(%s)",
                  U_PRIVTABLE,
                  fields.GetString(),
                  values.GetString());

    m_sqlite.Execute(&query, &errmsg);
}

//function retrives various privileges for the current user
void
UserManager::GetUserPrivileges()
{
   GetGlobalPrivileges();
   GetDBPrivileges();
   GetTablePrivileges();
   GetColumnPrivileges();

   if(IsMySQL5010(m_hmdi->m_tunnel, &m_hmdi->m_mysql) == wyTrue)
   {
       GetRoutinePrivileges();
   }
}

//function gets the privileges-column mapping
wyChar*
UserManager::GetPrivilegeTableMapping(wyString* key, wyBool iscolumnname)
{
    wyInt32 mappingcount, i, j, k;

    mappingcount = sizeof(m_privmapping) / sizeof(wyChar*) / 2;

    //loop through the static array looking for a match for key
    for(i = 0; i < mappingcount; ++i)
    {
        //set the variables based on the flag; 
        //the flag tells whether the key should be considered as column name or not
        if(iscolumnname == wyFalse)
        {
            j = i * 2;
            k = i * 2 + 1;
        }
        else
        {
            j = i * 2 + 1;
            k = i * 2;
        }

        if(!key->CompareI(m_privmapping[j]))
        {
            return m_privmapping[k];
        }
    }

    return NULL;
}

//function gets the global privileges for the user
void
UserManager::GetGlobalPrivileges()
{
    wyString            query, boolvalue, value, tempuser, temphost;
    MYSQL_RES*          myres;
    MYSQL_ROW           row;
    PrivilegedObject*  privobject = NULL;
    wyInt32             i, fieldindex;
    wyChar*             fieldname;

    wyChar* limitations[] = {
        "max_questions", 
        "max_updates", 
        "max_connections", 
        "max_user_connections"
    };
        
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    query.Sprintf("SELECT * FROM `mysql`.`user` WHERE User = '%s' AND Host = '%s'",
        EscapeMySQLString(m_username.GetString(), tempuser).GetString(),
        EscapeMySQLString(m_host.GetString(), temphost).GetString());
    myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);

    if(myres == NULL)
    {
        ShowMySQLError(m_hwnd, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query.GetString());
        return;
    }

    while((row =  m_hmdi->m_tunnel->mysql_fetch_row(myres)))
    {
        privobject = new PrivilegedObject(m_privcount, m_privarray, 0, 0);
        privobject->m_objecttype = 0;
        
        for(i = 0; i < m_privcount; ++i)
        {
            fieldname = GetPrivilegeTableMapping(&(m_privarray[i]->priv));
            
            if(fieldname)
            {
                fieldindex = GetFieldIndex(m_hmdi->m_tunnel, myres, fieldname);
                boolvalue.SetAs(row[fieldindex], m_hmdi->m_ismysql41);

                if(!boolvalue.CompareI("Y"))
                {
                    privobject->m_privileges[i] = 1;
                }
            }
        }

        InsertIntoSQLite(privobject);
        delete privobject;

        for(i = 0; i < 4; ++i)
        {
            fieldindex = GetFieldIndex(m_hmdi->m_tunnel, myres, limitations[i]);

            if(fieldindex != -1)
            {
                value.SetAs(row[fieldindex], m_hmdi->m_ismysql41);
                m_limitations[i] = value.GetAsInt32();
            }
        }
    }

    m_hmdi->m_tunnel->mysql_free_result(myres);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

//function gets the db privileges for the user
void
UserManager::GetDBPrivileges()
{
    wyString            query, boolvalue, tempuser, temphost;
    MYSQL_RES*          myres;
    MYSQL_ROW           row;
    PrivilegedObject*   privobject = NULL;
    wyInt32             i, fieldindex;
    wyChar*             fieldname;
        
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    query.Sprintf("SELECT * FROM `mysql`.`db` WHERE User = '%s' AND Host = '%s'",
        EscapeMySQLString(m_username.GetString(), tempuser).GetString(),
        EscapeMySQLString(m_host.GetString(), temphost).GetString());
    myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);

    if(myres == NULL)
    {
        ShowMySQLError(m_hwnd, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query.GetString());
        return;
    }

    while((row =  m_hmdi->m_tunnel->mysql_fetch_row(myres)))
    {
        privobject = new PrivilegedObject(m_privcount, m_privarray, U_DBS);
        privobject->m_objecttype = U_DBS;
        fieldindex = GetFieldIndex(m_hmdi->m_tunnel, myres, "Db");
        privobject->m_dbname.SetAs(row[fieldindex], m_hmdi->m_ismysql41);
        
        for(i = 0; i < m_privcount; ++i)
        {
            if(m_privarray[i]->context & U_DBS)
            {
                fieldname = GetPrivilegeTableMapping(&(m_privarray[i]->priv));

                if(fieldname)
                {
                    fieldindex = GetFieldIndex(m_hmdi->m_tunnel, myres, fieldname);
                    boolvalue.SetAs(row[fieldindex], m_hmdi->m_ismysql41);

                    if(!boolvalue.CompareI("Y"))
                    {
                        privobject->m_privileges[i] = 1;
                    }
                }
            }
        }

        InsertIntoSQLite(privobject);
        delete privobject;
    }

    m_hmdi->m_tunnel->mysql_free_result(myres);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

//function gets the routine privileges for the user
void
UserManager::GetRoutinePrivileges()
{
    wyString            query, value, tempuser, temphost;
    MYSQL_RES*          myres;
    MYSQL_ROW           row;
    PrivilegedObject*   privobject = NULL;
    wyInt32             i, count, routinetype;
    wyInt32*            indexarray;
        
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    query.Sprintf("SELECT Db, Routine_name, Routine_type, Proc_priv FROM `mysql`.`procs_priv` WHERE User = '%s' AND Host = '%s'",
        EscapeMySQLString(m_username.GetString(), tempuser).GetString(),
        EscapeMySQLString(m_host.GetString(), temphost).GetString());
    myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);

    if(myres == NULL)
    {
        ShowMySQLError(m_hwnd, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query.GetString());
        return;
    }

    indexarray = new wyInt32[m_privcount];

    while((row =  m_hmdi->m_tunnel->mysql_fetch_row(myres)))
    {
        value.SetAs(row[2], m_hmdi->m_ismysql41);

        if(!value.Compare("FUNCTION"))
        {
            routinetype = U_FUNCTIONS;
        }
        else
        {
            routinetype = U_PROCEDURES;
        }

        privobject = new PrivilegedObject(m_privcount, m_privarray, routinetype);
        privobject->m_objecttype = routinetype;
        privobject->m_dbname.SetAs(row[0], m_hmdi->m_ismysql41);
        privobject->m_objectname.SetAs(row[1], m_hmdi->m_ismysql41);
        value.SetAs(row[3], m_hmdi->m_ismysql41);
        count = GetPrivilegeIndexes(value, indexarray, wyFalse);

        for(i = 0; i < count; ++i)
        {
            privobject->m_privileges[indexarray[i]] = 1;            
        }

        InsertIntoSQLite(privobject);
        delete privobject;
    }

    delete[] indexarray;
    m_hmdi->m_tunnel->mysql_free_result(myres);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

//function gets the table privileges for the user
void
UserManager::GetTablePrivileges()
{
    wyString            query, value, tempuser, temphost;
    MYSQL_RES*          myres;
    MYSQL_ROW           row;
    PrivilegedObject*  privobject = NULL;
    wyInt32             i, count;
    wyInt32*            indexarray;
        
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    query.Sprintf("SELECT Db, Table_name, Table_priv FROM `mysql`.`tables_priv` WHERE User = '%s' AND Host = '%s'",
        EscapeMySQLString(m_username.GetString(), tempuser).GetString(),
        EscapeMySQLString(m_host.GetString(), temphost).GetString());
    myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);

    if(myres == NULL)
    {
        ShowMySQLError(m_hwnd, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query.GetString());
        return;
    }

    indexarray = new wyInt32[m_privcount];

    while((row =  m_hmdi->m_tunnel->mysql_fetch_row(myres)))
    {
        privobject = new PrivilegedObject(m_privcount, m_privarray, U_TABLES);
        privobject->m_objecttype = U_TABLES;
        privobject->m_dbname.SetAs(row[0], m_hmdi->m_ismysql41);
        privobject->m_objectname.SetAs(row[1], m_hmdi->m_ismysql41);
        value.SetAs(row[2], m_hmdi->m_ismysql41);
        count = GetPrivilegeIndexes(value, indexarray, wyFalse);

        for(i = 0; i < count; ++i)
        {
            privobject->m_privileges[indexarray[i]] = 1;            
        }

        InsertIntoSQLite(privobject);
        delete privobject;
    }

    delete[] indexarray;
    m_hmdi->m_tunnel->mysql_free_result(myres);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

//function gets the column privileges for the user
void
UserManager::GetColumnPrivileges()
{
    wyString            query, value, tempuser, temphost;
    MYSQL_RES*          myres;
    MYSQL_ROW           row;
    PrivilegedObject*   privobject = NULL;
    wyInt32             i, count;
    wyInt32*            indexarray;
        
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    query.Sprintf("SELECT Db, Table_name, Column_name, Column_priv FROM `mysql`.`columns_priv` WHERE User = '%s' AND Host = '%s'",
        EscapeMySQLString(m_username.GetString(), tempuser).GetString(),
        EscapeMySQLString(m_host.GetString(), temphost).GetString());
    myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);

    if(myres == NULL)
    {
        ShowMySQLError(m_hwnd, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query.GetString());
        return;
    }

    indexarray = new wyInt32[m_privcount];

    while((row =  m_hmdi->m_tunnel->mysql_fetch_row(myres)))
    {
        privobject = new PrivilegedObject(m_privcount, m_privarray, U_COLUMNS);
        privobject->m_objecttype = U_COLUMNS;
        privobject->m_dbname.SetAs(row[0], m_hmdi->m_ismysql41);
        privobject->m_objectname.SetAs(row[1], m_hmdi->m_ismysql41);
        privobject->m_columnname.SetAs(row[2], m_hmdi->m_ismysql41);
        value.SetAs(row[3], m_hmdi->m_ismysql41);
        count = GetPrivilegeIndexes(value, indexarray, wyFalse);

        for(i = 0; i < count; ++i)
        {
            privobject->m_privileges[indexarray[i]] = 1;            
        }

        InsertIntoSQLite(privobject);
        delete privobject;
    }

    delete[] indexarray;
    m_hmdi->m_tunnel->mysql_free_result(myres);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

//function gets the privileges that is supported by the server
wyBool
UserManager::GetServerPrivileges()
{
    wyString    query, temp;
    MYSQL_RES*  myres;
    MYSQL_ROW   row;
    wyInt32     i, j = 0;
    wyChar*     privname;
    wyChar* limitations[] = {"max_questions", "max_updates", "max_connections", "max_user_connections"};
    
    SetCursor(LoadCursor(NULL, IDC_WAIT));
    query.SetAs("SHOW COLUMNS FROM `mysql`.`user`");
    myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);
    m_privcount = 0;

    if(myres == NULL)
    {
        OnUMError(query.GetString(), wyTrue);
        return wyFalse;
    }

    while((row =  m_hmdi->m_tunnel->mysql_fetch_row(myres)))
    {
        temp.SetAs(row[0], m_hmdi->m_ismysql41);

        if(GetPrivilegeTableMapping(&temp, wyTrue) != NULL)
        {
            m_privcount++;
        }
        else
        {
            for(i = 0; i < U_MAXLIMITATIONS; ++i)
            {
                if(!temp.CompareI(limitations[i]))
                {
                    m_showlimitations[i] = wyTrue;
                }
            }
        }
    }
    
    //create the privilege array used for this server
    m_privarray = new Privileges*[m_privcount];
    m_hmdi->m_tunnel->mysql_data_seek(myres, 0);

    while((row =  m_hmdi->m_tunnel->mysql_fetch_row(myres)))
    {
        temp.SetAs(row[0], m_hmdi->m_ismysql41);

        if((privname = GetPrivilegeTableMapping(&temp, wyTrue)) != NULL)
        {
            m_privarray[j] = new Privileges;
            m_privarray[j]->priv.SetAs(privname);
            m_privarray[j]->context = 0;
            ++j;
        }
    }

    m_hmdi->m_tunnel->mysql_free_result(myres);
    qsort((void*)m_privarray, m_privcount, sizeof(Privileges*), UserManager::CompareFunct);

    for(i = 0; i < m_privcount; ++i)
    {
        if(!m_privarray[i]->priv.CompareI("GRANT"))
        {
            m_grantoptionindex = i;
        }
    }

    if(GetServerPrivsForDB() == wyFalse || GetServerPrivsForTableAndColumn() == wyFalse)
    {
        return wyFalse;
    }

    if(IsMySQL5010(m_hmdi->m_tunnel, &m_hmdi->m_mysql))
    {
        if(GetServerPrivsForRoutine() == wyFalse)
        {
            return wyFalse;
        }
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return wyTrue;
}

//compare function used to sort the priv array
wyInt32
UserManager::CompareFunct(const void* elem1, const void* elem2)
{
    Privileges** pelem1 = (Privileges**)elem1;
    Privileges** pelem2 = (Privileges**)elem2;

    return (*pelem1)->priv.Compare((*pelem2)->priv);
}

//get the server privileges that can be applied for db
wyBool
UserManager::GetServerPrivsForDB()
{
    wyString    query, temp;
    MYSQL_RES*  myres;
    MYSQL_ROW   row;
    wyInt32     j;
    wyChar*     privname;
    
    query.SetAs("SHOW COLUMNS FROM `mysql`.`db`");
    myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);

    if(myres == NULL)
    {
        OnUMError(query.GetString(), wyTrue);
        return wyFalse;
    }

    while((row =  m_hmdi->m_tunnel->mysql_fetch_row(myres)))
    {
        temp.SetAs(row[0], m_hmdi->m_ismysql41);

        if((privname = GetPrivilegeTableMapping(&temp, wyTrue)) != NULL)
        {
            for(j = 0; j < m_privcount; ++j)
            {
                if(!m_privarray[j]->priv.CompareI(privname))
                {
                    m_privarray[j]->context |= U_DBS;
                    break;
                }
            }
        }
    }

    m_hmdi->m_tunnel->mysql_free_result(myres);
    return wyTrue;
}

//get the server privileges that can be applied for tables and columns
wyBool
UserManager::GetServerPrivsForTableAndColumn()
{
    wyString    query, temp;
    MYSQL_RES*  myres;
    MYSQL_ROW   row;
    wyInt32     i, count;
    wyInt32*    indexarray;
    
    query.SetAs("SHOW COLUMNS FROM `mysql`.`tables_priv`");
    myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);

    if(myres == NULL)
    {
        OnUMError(query.GetString(), wyTrue);
        return wyFalse;
    }

    indexarray = new wyInt32[m_privcount];

    while((row =  m_hmdi->m_tunnel->mysql_fetch_row(myres)))
    {
        temp.SetAs(row[0], m_hmdi->m_ismysql41);

        if(!temp.CompareI("Table_priv"))
        {
            temp.SetAs(row[1], m_hmdi->m_ismysql41);
            count = GetPrivilegeIndexes(temp, indexarray, wyTrue);

            for(i = 0; i < count; ++i)
            {
                m_privarray[indexarray[i]]->context |= U_TABLES;
            }
        }
        else if(!temp.CompareI("Column_priv"))
        {
            temp.SetAs(row[1], m_hmdi->m_ismysql41);
            count = GetPrivilegeIndexes(temp, indexarray, wyTrue);

            for(i = 0; i < count; ++i)
            {
                m_privarray[indexarray[i]]->context |= U_COLUMNS;
            }
        }
    }

    m_hmdi->m_tunnel->mysql_free_result(myres);
    delete[] indexarray;
    return wyTrue;
}

//get the server privileges that can be applied for SPs and functions
wyBool
UserManager::GetServerPrivsForRoutine()
{
    wyString    query, temp;
    MYSQL_RES*  myres;
    MYSQL_ROW   row;
    wyInt32     i, count;
    wyInt32*    indexarray;
    
    query.SetAs("SHOW COLUMNS FROM `mysql`.`procs_priv`");
    myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);

    if(myres == NULL)
    {
        OnUMError(query.GetString(), wyTrue);
        return wyFalse;
    }

    indexarray = new wyInt32[m_privcount];

    while((row =  m_hmdi->m_tunnel->mysql_fetch_row(myres)))
    {
        temp.SetAs(row[0], m_hmdi->m_ismysql41);

        if(!temp.CompareI("Proc_priv"))
        {
            temp.SetAs(row[1], m_hmdi->m_ismysql41);
            count = GetPrivilegeIndexes(temp, indexarray, wyTrue);

            for(i = 0; i < count; ++i)
            {
                m_privarray[indexarray[i]]->context |= U_FUNCTIONS | U_PROCEDURES;
            }
        }
    }

    m_hmdi->m_tunnel->mysql_free_result(myres);
    delete[] indexarray;
    return wyTrue;
}

//function identifies all the privileges given in the first parameter and stores the currusponding mapping index in the second parameter
wyInt32
UserManager::GetPrivilegeIndexes(wyString& value, wyInt32* indexarray, wyBool istype)
{
    const wyChar*   ptr = value.GetString();
    wyChar          temp[2] = {0, 0};
    wyString        buffer;
    wyInt32         i, j;
    
    ptr += (istype == wyTrue) ? 4 : 0;
     
    for(buffer.Clear(), j = 0; *ptr != 0; ++ptr)
    {
        if(*ptr == '\'' || *ptr == ')')
        {
            continue;
        }

        if(*ptr == ',')
        {
            for(i = 0; i < m_privcount; ++i)
            {
                if(!m_privarray[i]->priv.CompareI(buffer))
                {
                    indexarray[j++] = i;
                    break;
                }
            }

            buffer.Clear();
            continue;
        }

        temp[0] = *ptr;
        buffer.Add(temp);
    }

    for(i = 0; i < m_privcount; ++i)
    {
        if(!m_privarray[i]->priv.CompareI(buffer))
        {
            indexarray[j++] = i;
            break;
        }
    }

    return j;
}

//function deletes the user level items. typical cases are while combo change and while destruction
void
UserManager::DeleteUserLevelItems()
{
    wyInt32     i;
    wyString    query, errmsg;

    query.Sprintf("DELETE FROM %s", U_ORIG_PRIVTABLE);
    m_sqlite.Execute(&query, &errmsg);
    query.Sprintf("DELETE FROM %s", U_PRIVTABLE);
    m_sqlite.Execute(&query, &errmsg);
    
    for(i = 0; i < 4; ++i)
    {
        m_limitations[i] = 0;
    }
}

//function updates the list view based on the tree view selection change
void
UserManager::UpdateListView(wyInt32 imageindex)
{
    HWND hwndlistview = GetDlgItem(m_hwnd, IDC_PRIVLIST);
    HWND hwndcheck = GetDlgItem(m_hwnd, IDC_SELECTALLCHECK);

    ListView_DeleteAllItems(hwndlistview);

    //reset this so that we can have shift click functionality works fine
    m_lastcheckedindex = -1;

    if(imageindex == m_umimageindex + 2 || imageindex == NTABLES || 
       imageindex == NFOLDER || imageindex == NVIEWS || 
       imageindex == NSP || imageindex == NFUNC)
    {
        EnableWindow(hwndcheck, FALSE);
        SendMessage(hwndcheck, BM_SETCHECK, BST_UNCHECKED, 0);
        ShowWindow(m_hwndnote, SW_SHOW);
        return;
    }
    else
    {
        EnableWindow(hwndcheck, TRUE);
        SendMessage(hwndcheck, BM_SETCHECK, BST_UNCHECKED, 0);
        ShowWindow(m_hwndnote, SW_HIDE);
    }

    //set the flag so that we can filter out automated changes in the handler
    m_isautomatedchange = wyTrue;

    //get the privileges for the item selected in the tree view and fill it
    PopulatePrivsBasedOnContext(imageindex);
    FillPrivileges(imageindex);

    //reset the flag
    m_isautomatedchange = wyFalse;
}

//function populates the list view based on the tree view selection
void
UserManager::PopulatePrivsBasedOnContext(wyInt32 imageindex)
{
    LVITEM  lvi;
    HWND    hwndlistview = GetDlgItem(m_hwnd, IDC_PRIVLIST);
    wyInt32 i, context = 0;

    lvi.mask = LVIF_TEXT;
	lvi.iItem = 0;
	lvi.iSubItem = 0;

    if(imageindex == m_umimageindex + 1)
    {
        for(i = 0; i < m_privcount; ++i)
        {
            lvi.pszText = m_privarray[i]->priv.GetAsWideChar();
            lvi.cchTextMax = m_privarray[i]->priv.GetLength();

            ListView_InsertItem(hwndlistview, &lvi);
        }

        return;
    }

    context = ImageIndexToContext(imageindex);

    for(i = 0; i < m_privcount; ++i)
    {
        if(m_privarray[i]->context & context)
        {
            lvi.pszText = m_privarray[i]->priv.GetAsWideChar();
            lvi.cchTextMax = m_privarray[i]->priv.GetLength();
            ListView_InsertItem(hwndlistview, &lvi);
        }
    }
}

//function converts the image index to currusponding context
wyInt32
UserManager::ImageIndexToContext(wyInt32 imageindex)
{
    wyInt32 context = -1;

    if(imageindex == m_umimageindex + 1)
    {
        return 0;
    }

    switch(imageindex)
    {
        case NDATABASE:
            context = U_DBS;
            break;

        case NTABLE:
        case NVIEWSITEM:
            context = U_TABLES;
            break;

        case NCOLUMN:
        case NPRIMARYKEY:
            context = U_COLUMNS;
            break;

        case NINDEX:
        case NPRIMARYINDEX:
            context = U_INDEXES;
            break;

        case NSPITEM:
            context = U_PROCEDURES;
            break;

        case NFUNCITEM:
            context = U_FUNCTIONS;
            break;
    }

    return context;
}

//function fills the list view with the currusponding privileges checked
void
UserManager::FillPrivileges(wyInt32 imageindex)
{
    wyInt32         i, count, context, temp, j;
    HWND            hwndlistview = GetDlgItem(m_hwnd, IDC_PRIVLIST);
    LVITEM	        lvi;    
    wyString        query;
    sqlite3_stmt*   stmt;

    count = ListView_GetItemCount(hwndlistview);
    
    context = ImageIndexToContext(imageindex);
    query.Sprintf("SELECT * FROM %s WHERE DB_NAME = ? AND \
                    OBJECT_NAME = ? AND COLUMN_NAME = ? AND \
                    OBJECT_TYPE = %d", U_PRIVTABLE, context);
    m_sqlite.Prepare(&stmt, (wyChar*)query.GetString());
    m_sqlite.SetText(&stmt, 1, m_currentdb.GetString());
    m_sqlite.SetText(&stmt, 2, m_currentobject.GetString());
    m_sqlite.SetText(&stmt, 3, m_currentcolumn.GetString());

    while(m_sqlite.Step(&stmt, wyFalse) && m_sqlite.GetLastCode() == SQLITE_ROW)
    {
        for(j = 0, i = 4; i < m_privcount + 4; ++i)
        {
            temp = m_sqlite.GetInt(&stmt, i);

            if(temp != -1)
            {
                lvi.mask = LVIF_PARAM;
                lvi.iSubItem = 0;
                lvi.lParam = temp;
                lvi.iItem = j;
                ListView_SetItem(hwndlistview, &lvi);
                ListView_SetCheckState(hwndlistview, j++, temp);
            }
        }
    }

    m_sqlite.Finalize(&stmt);
}

//function inserts the databases
void
UserManager::InsertDatabases()
{
	wyString    query, dbname;
	HTREEITEM	hdatabase, hobjectpriv;
    HWND        hwndtv = GetDlgItem(m_hwnd, IDC_OBTREE);

	MYSQL_RES*  myres=NULL;
	MYSQL_ROW	myrow;

    hobjectpriv = InitTreeViewNodes(GetDlgItem(m_hwnd, IDC_OBTREE));

    SetCursor(LoadCursor(NULL, IDC_WAIT));
	query.Sprintf("SHOW DATABASES");
    myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);;

    if(!myres)
    {
        ShowMySQLError(m_hwnd, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query.GetString());
        return;
    }

	while((myrow = m_hmdi->m_tunnel->mysql_fetch_row(myres)))	
    {
        dbname.SetAs(myrow[0], m_hmdi->m_ismysql41);
        hdatabase = InsertNode(hwndtv, hobjectpriv, dbname.GetAsWideChar(), NDATABASE, NDATABASE, 0);
        
        InsertDummyNode(hwndtv, hdatabase);
    }
    
    m_hmdi->m_tunnel->mysql_free_result(myres);
    TreeView_Expand(hwndtv, hobjectpriv, TVM_EXPAND);
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    TreeView_SelectItem(hwndtv, TreeView_GetRoot(hwndtv));
}

//function inserts the privileged objects by reading the SQLite table
void
UserManager::InsertPrivilegedObject()
{
    wyString            query, errmsg, itemname;
    HTREEITEM           htemp;
    HWND                hwndtv = GetDlgItem(m_hwnd, IDC_PRIVOBTREE);
    PrivilegedObject    privob(0, NULL, 0);

    privob.m_objecttype = m_umimageindex + 2;
    SetSelectedObjectInfo(&privob);

    SendMessage(hwndtv, WM_SETREDRAW, (WPARAM)FALSE, 0);
    htemp = InitTreeViewNodes(hwndtv);

    query.Sprintf("SELECT * FROM %s", U_ORIG_PRIVTABLE);
    m_sqlite.Prepare(NULL, (wyChar*)query.GetString());
    
    while(m_sqlite.Step(NULL, wyTrue) && m_sqlite.GetLastCode() == SQLITE_ROW)
    {
        InsertPrivilegedObjectHelper(htemp, m_sqlite.GetInt(NULL, 3));
    }

    m_sqlite.Finalize(NULL);

    if(privob.m_objecttype == m_umimageindex)
    {
        htemp = TreeView_GetParent(hwndtv, htemp);
    }
    else if(privob.m_objecttype == m_umimageindex + 1)
    {
        htemp = TreeView_GetPrevSibling(hwndtv, htemp);
    }
    else if(privob.m_objecttype != m_umimageindex + 2)
    {
        htemp = GetPrevSelection(htemp, &privob);
    }

    TreeView_SelectItem(hwndtv, htemp);
    SendMessage(hwndtv, WM_SETREDRAW, (WPARAM)TRUE, 0);
    InvalidateRect(hwndtv, NULL, TRUE);
    UpdateWindow(hwndtv);
}

//function that recursively inserts any parent item needed before inserting the item selected from the SQLite table
HTREEITEM
UserManager::InsertPrivilegedObjectHelper(HTREEITEM hobjectpriv, wyInt32 context)
{
    wyString        objectname;
    HTREEITEM       hitem, hparent, htemp;
    HWND            hwndtv = GetDlgItem(m_hwnd, IDC_PRIVOBTREE);
    TVINSERTSTRUCT  insertstruct;
    wyInt32         imageindex;

    switch(context)
    {
        case U_DBS:
            hparent = hobjectpriv;
            imageindex = NDATABASE;
            objectname.SetAs(m_sqlite.GetText(NULL, 0));
            break;
        
        case U_FUNCTIONS:
            hparent = InsertPrivilegedObjectHelper(hobjectpriv, U_DBS);
            imageindex = NFUNCITEM;
            objectname.SetAs(m_sqlite.GetText(NULL, 1));
            break;

        case U_PROCEDURES:
            hparent = InsertPrivilegedObjectHelper(hobjectpriv, U_DBS);
            imageindex = NSPITEM;
            objectname.SetAs(m_sqlite.GetText(NULL, 1));
            break;

        case U_TABLES:
            hparent = InsertPrivilegedObjectHelper(hobjectpriv, U_DBS);
            imageindex = NTABLE;
            objectname.SetAs(m_sqlite.GetText(NULL, 1));
            break;

        case U_COLUMNS:
            hparent = InsertPrivilegedObjectHelper(hobjectpriv, U_TABLES);
            imageindex = NCOLUMN;
            objectname.SetAs(m_sqlite.GetText(NULL, 2));
            break;

        default:
            return NULL;
    }

    if(imageindex == NTABLE)
    {
        imageindex = (IsView() == wyTrue) ? NVIEWSITEM : NTABLE;
    }

    if((hitem = FindNode(hwndtv, hparent, &objectname, imageindex)))
    {
        return hitem;
    }

    insertstruct.hInsertAfter = TVI_SORT;
    insertstruct.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE |TVIF_STATE;
    insertstruct.hParent = hparent;
    insertstruct.item.state = TVIS_EXPANDEDONCE;
    insertstruct.item.iImage = imageindex;
    insertstruct.item.iSelectedImage = imageindex;
    insertstruct.item.pszText = objectname.GetAsWideChar();
    insertstruct.item.cchTextMax = objectname.GetLength();

    htemp = TreeView_InsertItem(hwndtv, &insertstruct);
    TreeView_Expand(hwndtv, hparent, TVE_EXPAND);
    return htemp;
}

//function searches for a node name under the given parent and returns the handle
HTREEITEM
UserManager::FindNode(HWND hwndtv, HTREEITEM hitem, wyString* name, wyInt32 imageindex)
{
    TVITEM tvi = {0};
    HTREEITEM htemp;
    wyWChar buffer[SIZE_128];
    wyString temp;

    for(htemp = TreeView_GetChild(hwndtv, hitem); htemp;
        htemp = TreeView_GetNextSibling(hwndtv, htemp))
    {
        memset(&tvi, 0, sizeof(tvi));
        tvi.hItem = htemp;
        tvi.mask = TVIF_TEXT | TVIF_IMAGE;
        buffer[0] = 0;
        tvi.pszText = buffer;
        tvi.cchTextMax = SIZE_128;
        TreeView_GetItem(hwndtv, &tvi);
        temp.SetAs(tvi.pszText);

        if(imageindex == tvi.iImage && !name->Compare(temp))
        {
            return tvi.hItem;
        }
    }

    return NULL;
}

//function used to keep the persistance of the selection while repopulating the privileged objects
HTREEITEM
UserManager::GetPrevSelection(HTREEITEM hobjpriv, PrivilegedObject* privobj)
{
    HWND hwndtv = GetDlgItem(m_hwnd, IDC_PRIVOBTREE);
    HTREEITEM htemp, hitem = hobjpriv;
    wyBool continueflag = wyFalse;
    wyInt32 imageindex;

    if(privobj->m_dbname.GetLength())
    {
        if((htemp = FindNode(hwndtv, hitem, &privobj->m_dbname, NDATABASE)))
        {
            hitem = htemp;
            continueflag = wyTrue;
        }
    }

    if(continueflag == wyTrue && privobj->m_objectname.GetLength())
    {
        continueflag = wyFalse;
        imageindex = privobj->m_objecttype == NCOLUMN ? NTABLE : privobj->m_objecttype;

        if((htemp = FindNode(hwndtv, hitem, &privobj->m_objectname, imageindex)))
        {
            hitem = htemp;
            continueflag = wyTrue;
        }
    }

    if(continueflag == wyTrue && privobj->m_columnname.GetLength())
    {
        if((htemp = FindNode(hwndtv, hitem, &privobj->m_columnname, NCOLUMN)))
        {
            hitem = htemp;
            continueflag = wyTrue;
        }
    }

    return hitem;
}

//helper function that checks whether a given table is a base table or view
wyBool
UserManager::IsView()
{
    MYSQL_RES*  myres=NULL;
	MYSQL_ROW   myrow;
    wyString    query, temp;

    if(IsMySQL5010(m_hmdi->m_tunnel, &m_hmdi->m_mysql) == wyFalse)
    {
        return wyFalse;
    }

    SetCursor(LoadCursor(NULL, IDC_WAIT));
	query.Sprintf("SELECT TABLE_TYPE FROM information_schema.TABLES WHERE TABLE_SCHEMA = '%s' AND TABLE_NAME = '%s'",
                  m_sqlite.GetText(NULL, 0), m_sqlite.GetText(NULL, 1));

    myres = ExecuteAndGetResult(m_hmdi, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);;

    if(!myres)
    {
        ShowMySQLError(m_hwnd, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query.GetString());
        return wyFalse;
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));

	while((myrow = m_hmdi->m_tunnel->mysql_fetch_row(myres)))	
    {
        temp.SetAs(myrow[0]);

        if(!temp.CompareI("VIEW"))
        {
            m_hmdi->m_tunnel->mysql_free_result(myres);
            return wyTrue;
        }
    }
    
    m_hmdi->m_tunnel->mysql_free_result(myres);
    return wyFalse;
}

//helper function that gets the handle for the visible tree view
HWND
UserManager::GetTreeViewHandle()
{
    HWND hwndtv = GetDlgItem(m_hwnd, IDC_OBTREE);

    if(m_isallradiochecked == wyTrue)
    {
        return hwndtv;
    }

    return GetDlgItem(m_hwnd, IDC_PRIVOBTREE);
}

void
UserManager::OnUMError(const wyChar* query, wyBool isinitializing)
{
    wyString temp;

    if(isinitializing == wyTrue && m_hmdi->m_tunnel->mysql_errno(m_hmdi->m_mysql) == 1142)
    {
        temp.Sprintf("MySQL error : (1142) - %s\n\n%s", 
            m_hmdi->m_tunnel->mysql_error(m_hmdi->m_mysql), U_ACCESSDENIED);
        MessageBox(m_hwnd, temp.GetAsWideChar(), L"MySQL Error", MB_ICONERROR | MB_OK);
    }
    else
    {
        ShowMySQLError(m_hwnd, m_hmdi->m_tunnel, &m_hmdi->m_mysql, query);
    }
}

//external api to invoke user manager
wyInt32
CreateUserManager()
{
    UserManager ob;

    return ob.Create(pGlobals->m_pcmainwin->m_hwndmain);
}
