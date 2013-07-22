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


#include<shlobj.h>

#include "FavoriteAdd.h"
#include "MDIWindow.h"
#include "EditorBase.h"
#include "SQLTokenizer.h"
#include "FileHelper.h"
#include "CommonHelper.h"
#include "GUIHelper.h"

extern	PGLOBALS		pGlobals;

FavoriteAdd::FavoriteAdd ()
{
}

FavoriteAdd::~FavoriteAdd ()
{
}


// Funtion to Initialize the Add Favorite Dialog Box //
wyBool 
FavoriteAdd::Display(HWND hwnd)
{	
	CFavoriteMenu	menu;
    wyBool          ret;

	DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_ADDFAVORITES), 
        hwnd, FavoriteAdd::AddFavoDlgProc, (LPARAM)this);
	
	//For refreshing the  favorite menu
	ret = menu.Display(pGlobals->m_pcmainwin->GetHwnd());
	return ret;
}


//CallBack Function for AddFavorite  Dialog Box///
INT_PTR  CALLBACK 
FavoriteAdd::AddFavoDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	FavoriteAdd *favor = (FavoriteAdd*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	DlgControl* pdlgctrl;

	switch(message)
	{
	case WM_INITDIALOG:
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
        LocalizeWindow(hwnd);
		PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
		break;

	case WM_INITDLGVALUES:	
	    favor->InitializeAddFavorite(hwnd);
		break;
		
	case WM_NOTIFY:
	    favor->OnWMNotify(hwnd, lParam);
		break;

	case WM_COMMAND :
        favor->OnWMCommand(hwnd, wParam);
        break;

	case WM_GETMINMAXINFO:
		favor->OnWMSizeInfo(lParam);
		break;

	case WM_SIZE:
		favor->AddFavResize(hwnd);
		break;

	case WM_PAINT:
		favor->OnPaint(hwnd);
        return wyTrue; 
		break;

	case WM_ERASEBKGND:
		//blocked for double buffering
		return 1;

	case WM_DESTROY:
		//delete the dialog control list
		while((pdlgctrl = (DlgControl*)favor->m_controllist.GetFirst()))
		{
			favor->m_controllist.Remove(pdlgctrl);
			delete pdlgctrl;
		}
		SaveInitPos(hwnd, ADDFAV_SECTION);
		break;

	default:
		break;
	}
    return wyFalse;
}

void 
FavoriteAdd::OnWMNotify(HWND hwnd, LPARAM lparam)
{
	LPNMHDR lpnmhdr = (LPNMHDR)lparam;
	
	switch(lpnmhdr->code)
	{
		case TVN_SELCHANGED:
			if(GetSelectionImage(hwnd) == NFILE)
			{	
				wyWChar name[MAX_PATH + 1] = {0};
				SendMessage(GetDlgItem(hwnd, IDC_FAVORITENAME), WM_SETTEXT, 0, (LPARAM)GetSelectionText(hwnd, name));
			}
				
		break;
	}
}
void 
FavoriteAdd::OnWMCommand(HWND hwnd, WPARAM wParam)
{
    switch(LOWORD(wParam))
    {
	    case IDOK:
		    if(ProcessOK(hwnd))
			    EndDialog(hwnd, 0);
		    break;

	    case IDC_NEWFOLDER:
		    CreateNewFolder(hwnd);
		    break;

	    case IDCANCEL :
		    EndDialog(hwnd, 0);
		    break;

	    case IDC_CURRENTSQL:
		    EnableExtraOptionWindows(hwnd, wyFalse);
		    break;

	    case IDC_SQLFROMFILE:
		    EnableExtraOptionWindows(hwnd, wyTrue);
		    break;

	    case IDC_OPEN:
		    GetSQLFileName(hwnd);
		    break;
    }
}

void 
FavoriteAdd::InitializeAddFavorite(HWND hwnd)
{
	m_hwnd = hwnd;

    SendDlgItemMessage(hwnd, IDC_ADDFAVORTREE, TVM_SETIMAGELIST, 0, (LPARAM)m_image.GetImageHandle());
	m_haddtree = GetDlgItem(hwnd, IDC_ADDFAVORTREE);
	// Function call to initialize the TreeView control 
	// parameter 0 , to add only folder types
	InitTreeView(m_haddtree, wyTrue);
	
	/* by default check we select IDC_CURRENT */
	SendMessage(GetDlgItem(hwnd, IDC_CURRENTSQL), BM_SETCHECK, BST_CHECKED, BST_CHECKED);
	
	SendMessage(GetDlgItem(hwnd, IDC_FAVORITENAME), EM_SETLIMITTEXT, _MAX_FNAME, 0);

	GetClientRect(m_hwnd, &m_dlgrect);
	GetWindowRect(m_hwnd, &m_wndrect);

	//set the initial position of the dialog
	
	
	GetCtrlRects();
	

	SetWindowPositionFromINI(m_hwnd, ADDFAV_SECTION, 
		m_wndrect.right - m_wndrect.left, 
		m_wndrect.bottom - m_wndrect.top);
	PositionCtrls();
}


// CallBack Function to get the New folder name //
INT_PTR   CALLBACK 
FavoriteAdd::AddFolderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	FavoriteAdd * favor = (FavoriteAdd*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	
	switch (message)
	{	
	case WM_INITDIALOG :
		SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
		SetFocus(GetDlgItem(hwnd, IDC_FOLDERNAME));
		break;

	case WM_COMMAND :
		switch(LOWORD(wParam))
		{
			case IDOK :
				if(favor->NewFolderProcessOK(hwnd))
					EndDialog(hwnd, 0);
				break;

			case IDCANCEL :
				EndDialog(hwnd, 0);
				break;
		}
	 
	default :
		break;
	}
	return wyFalse;
}

/* Function enable/disables two windows when a user wants to open sql from a file */
wyBool
FavoriteAdd::EnableExtraOptionWindows(HWND hwnd, wyBool enable)
{
	EnableWindow(GetDlgItem(hwnd, IDC_SQLFILE), enable);
	EnableWindow(GetDlgItem(hwnd, IDC_OPEN), enable);

	return wyTrue;
}

wyBool
FavoriteAdd::GetSQLFileName(HWND hwnd)
{
	wyWChar tempfile[MAX_PATH + 1] = {0};

	if(InitOpenFile(hwnd, tempfile, SQLINDEX, MAX_PATH))
		SetWindowText(GetDlgItem( hwnd, IDC_SQLFILE), tempfile);

	return wyTrue;
}

// function to get favorite name & query to be stored in favorite

wyBool
FavoriteAdd::GetFavoriteName(HWND hwnd, wyString & favname, wyString & favquery)
{
	MDIWindow *c = GetActiveWin();
	VERIFY(c);

	/* nothing should be done if no favoritename is given */
	if(!SendMessage(GetDlgItem(hwnd, IDC_FAVORITENAME) ,WM_GETTEXTLENGTH ,0, 0))
	{
		yog_message(hwnd , _(L"Please enter a reference name for the query") , pGlobals->m_appname.GetAsWideChar() , MB_OK | MB_ICONERROR);
		SetFocus(GetDlgItem(hwnd, IDC_FAVORITENAME));
		return wyFalse;
	}

	/*	first we check if the user has specified to input a file but has not given the
		file name then we return FALSE */
	if(BST_CHECKED == SendMessage(GetDlgItem(hwnd, IDC_SQLFROMFILE), 
									  BM_GETCHECK, 0, 0))
    {
		if(!SendMessage(GetDlgItem(hwnd, IDC_SQLFILE), WM_GETTEXTLENGTH, 0, 0))
        {
			yog_message(hwnd, _(L"Please provide a SQL file name"), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
			SetFocus(GetDlgItem(hwnd, IDC_SQLFILE));
			return wyFalse;
		} 
        else 
        {
			GetDataToBuffer(hwnd, favname, favquery, wyFalse);
			return wyTrue;
		}
	}

	if(SendMessage(c->GetActiveTabEditor()->m_peditorbase->GetHWND(), WM_GETTEXTLENGTH, 0, 0) == 0)   
	{
		yog_message(hwnd, _(L"Query Editor is empty!\nPlease enter some query(s) in the editor."), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
		return wyFalse;
	}

	if(GetDataToBuffer(hwnd, favname, favquery, wyTrue) == wyFalse)
    {
        yog_message(hwnd, _(L"Query Editor is empty!\nPlease enter some query(s) in the editor."), pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
		return wyFalse;
    }
	return wyTrue;
}

wyBool 
FavoriteAdd::GetDataToBuffer(HWND hwnd, wyString &favname, wyString &favquery, wyBool iseditor)
{
	wyUInt32    querylength, favnamelength;
//	wyInt32		length = 0;
    MDIWindow	*c;
	wyWChar		temp[MAX_PATH + 1] = {0};
	wyString	tempstr;
	wyChar		*tempbuff = {0};
	wyWChar		name[MAX_PATH + 1] = {0};

	VERIFY ( c = GetActiveWin () );
	querylength = SendMessage(c->GetActiveTabEditor()->m_peditorbase->GetHWND(), WM_GETTEXTLENGTH, 0, 0); 
	favnamelength	= SendMessage(GetDlgItem(hwnd, IDC_FAVORITENAME), WM_GETTEXTLENGTH ,0, 0);

	if ( !iseditor ) 
    {
		SendMessage(GetDlgItem(hwnd, IDC_SQLFILE), WM_GETTEXT, MAX_PATH, (LPARAM)temp);
		tempstr.SetAs(temp);
		favquery.Sprintf("file=%s", tempstr.GetString());
	} 
    else
    {
		//length = querylength;
        tempbuff = AllocateBuff(querylength + 1);
		SendMessage(c->GetActiveTabEditor()->m_peditorbase->GetHWND(), SCI_GETTEXT, querylength + 1, (LPARAM)tempbuff);

        //if(TrimLeft(tempbuff) == NULL)//only spaces
          //  return wyFalse;

        favquery.SetAs(tempbuff);
        free(tempbuff);
        tempbuff = NULL;
	}

    SendMessage(GetDlgItem(hwnd, IDC_FAVORITENAME), WM_GETTEXT, MAX_PATH, (LPARAM)name);
    favname.SetAs(name);
	return wyTrue;
}


// function to get new folder name 
wyBool 
FavoriteAdd::GetFolderName(HWND hwnd, wyString &foldername)
{
	wyUInt32		folderlength;			/*folder name length */
    wyWChar          folder[_MAX_DIR + 1] = {0};

    folderlength = SendMessage(GetDlgItem(hwnd, IDC_FOLDERNAME), 
								WM_GETTEXTLENGTH, 0, 0);
	if(folderlength == 0)
	{
		yog_message(hwnd, _(L"Please specify the folder name") , pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONERROR);
		return wyFalse;
	}

	SendMessage(GetDlgItem(hwnd, IDC_FOLDERNAME), WM_GETTEXT, _MAX_DIR, (LPARAM)folder);
    foldername.SetAs(folder);
    return wyTrue;
}


// Function to process OK button in getfolder name dialog box//
wyBool 
FavoriteAdd::NewFolderProcessOK(HWND hwnd)
{
	wyString    foldname;			/*foldername */
	wyInt32		ret = 0;
	
	
	if(GetFolderName(hwnd, foldname))
	{
		// Function call to add folder name to the database 
        wyString favquery("");
	 	ret = InsertFavorite(foldname, wyTrue, favquery) ;
				
		if(ret)
			return wyFalse;
		
		return wyTrue;
	}
	else
		return wyFalse;
} 

// Function to add  new folder or file favorite to treeview control //
wyBool 
FavoriteAdd::InsertFavorite(wyString &favorite, wyBool isfolder, wyString &favquery)
{
	wyString		path("");
	HTREEITEM		item , folder = {0}, parent ;
		
	VERIFY(item = TreeView_GetSelection(m_haddtree));
	
	if(SelItemPath(m_haddtree, item, path, wyTrue) == wyFalse)
        return wyFalse;
	
	path.AddSprintf("\\%s", favorite.GetString());
	
	//function call to add the new favorite by giving the path & the content of the favorite file //
	if(InsertFavoriteItem(path, isfolder, favquery))
	{	
		if(isfolder == wyFalse)
			return wyFalse;

        if(GetSelectionImage(GetParent(m_haddtree)) == NFILE)
            VERIFY(parent = TreeView_GetParent(m_haddtree, item));
        else
            parent = item;

		// add the new folder to treeview control//
		folder = AddItem(m_haddtree, parent, favorite.GetString(), NDIR);
		
		TreeView_Select(m_haddtree, folder, TVGN_CARET);
	}
	else
		return wyTrue;

	return wyFalse;
}


// Function call to Process Ok button of Add Favorite dialog box //

wyBool 
FavoriteAdd::ProcessOK(HWND hwnd)
{
	wyString favname, favquery;	//favorite name  , favorite query //
	
	// Function call to get the Favorite name & query to store into the Favorite //
	if(GetFavoriteName(hwnd, favname, favquery)) 
	{		
		if(InsertFavorite(favname, wyFalse, favquery))
		{
			SetFocus(GetDlgItem(hwnd, IDC_FAVORITENAME));
			return wyFalse;
		}
		return wyTrue;
	}
	return wyFalse;
}

// Function to handle create new folder button click in Add favorite dialog box //
wyBool 
FavoriteAdd::CreateNewFolder(HWND hwnd)
{	
    ::DialogBoxParam(pGlobals->m_hinstance, 
					MAKEINTRESOURCE(IDD_GETFOLDERNAME), hwnd ,
					FavoriteAdd::AddFolderProc, (LPARAM)this);
	return wyTrue;
}


INT
FavoriteAdd::GetSelectionImage(HWND hwnd)
{
	HTREEITEM	hItem;
    TVITEM		tvi;

	hItem = TreeView_GetSelection(GetDlgItem(hwnd, IDC_ADDFAVORTREE));
	memset(&tvi, 0, sizeof(TVITEM));

	tvi.mask = TVIF_IMAGE;
	tvi.hItem = hItem;

	TreeView_GetItem(GetDlgItem(hwnd, IDC_ADDFAVORTREE), &tvi);
	return tvi.iImage;
}


wyWChar*
FavoriteAdd::GetSelectionText(HWND hwnd, wyWChar * text)
{
	HTREEITEM	hItem;
    TVITEM		tvi;
    
	hItem = TreeView_GetSelection(GetDlgItem(hwnd, IDC_ADDFAVORTREE));
	memset(&tvi, 0, sizeof(TVITEM));
	tvi.mask = TVIF_TEXT;
	tvi.hItem = hItem;
	tvi.pszText = text;
	tvi.cchTextMax = SIZE_128 - 1;

	TreeView_GetItem(GetDlgItem(hwnd, IDC_ADDFAVORTREE), &tvi);

	return tvi.pszText;
}

void
FavoriteAdd::GetCtrlRects()
{
    RECT    rect;
    wyInt32 i, count;
    HWND    hwnd;

    //ctrlid    size-x     size-y
    wyInt32 ctrlid[] = {
       	IDC_FAVORITENAME, 1, 0,
        IDC_ADDFAVORTREE, 1, 1,
        IDC_CURRENTSQL, 1, 0,
        IDC_SQLFROMFILE, 1, 0,
        IDC_SQLFILE, 1, 0,
        IDC_OPEN, 0, 0,
        IDOK, 0, 0,
        IDCANCEL, 0, 0,
        IDC_NEWFOLDER, 0, 0,
        IDC_STAT_HORZ, 1, 0,
		IDC_ADDFAV_GRIP, 0, 0
    };

    count = sizeof(ctrlid)/sizeof(ctrlid[0]);

    //store the control handles and related information in the linked list
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

void
FavoriteAdd::PositionCtrls()
{
    DlgControl* pdlgctrl;
    wyInt32     leftpadding, toppadding, rightpadding, bottompadding, width, height;
    wyInt32     x, y;

    RECT        rect;

    HDWP        hdefwp;
		
	GetClientRect(m_hwnd, &rect);
	m_controllist.GetCount();

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
			if(pdlgctrl->m_id == IDC_ADDFAV_GRIP)
				x = rect.right - width;
			else
				x=rect.right - rightpadding - width;
        }
        else
        {
			x = leftpadding;
			width = rect.right - leftpadding - rightpadding;
        }
	    switch(pdlgctrl->m_id)
        {
			case IDC_CURRENTSQL:
			case IDC_SQLFROMFILE:
			case IDC_SQLFILE:
			case IDC_OPEN:
                y = rect.bottom - bottompadding - height;
                break;

			case IDC_ADDFAV_GRIP:
				y = rect.bottom - height;
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
    }

    //finish the operation and apply changes
    EndDeferWindowPos(hdefwp);
}

void 
FavoriteAdd::AddFavResize(HWND hwnd){

	PositionCtrls();
}

void
FavoriteAdd::OnWMSizeInfo(LPARAM lparam)
{
    MINMAXINFO* pminmax = (MINMAXINFO*)lparam;
    pminmax->ptMinTrackSize.x = m_wndrect.right - m_wndrect.left;
    pminmax->ptMinTrackSize.y = m_wndrect.bottom - m_wndrect.top;
}

//function handles the WM_PAINT and paints the window using double buffering
void
FavoriteAdd::OnPaint(HWND hwnd)
{
    HDC         hdc;
	PAINTSTRUCT ps;
	HWND hwndstat = GetDlgItem(hwnd, IDC_ADDFAV_GRIP);
	RECT temp;
	
	VERIFY(hdc = BeginPaint(hwnd, &ps));
	DoubleBuffer::EraseBackground(hwnd, hdc, NULL, GetSysColor(COLOR_3DFACE));
	EndPaint(hwnd, &ps);

	//To paint the resize grip
	VERIFY(hdc = BeginPaint(hwndstat, &ps));
    GetClientRect(hwndstat, &temp);
    DrawFrameControl(hdc, &temp, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);
    EndPaint(hwnd, &ps);
}