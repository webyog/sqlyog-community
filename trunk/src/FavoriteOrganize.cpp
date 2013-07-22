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


/*! \file OrganizeFavorite.cpp
    \brief Implementaion of functions defined in OrganizeFavorite class
    
    These functions helps in organizing the items in favorites of SQLyog.
*/

#include <shlobj.h>
#include "FavoriteOrganize.h"
#include "MDIWindow.h"
#include "SQLTokenizer.h"
#include "GUIHelper.h"
#include "commonhelper.h"
#include "EditorFont.h"

extern	PGLOBALS		pGlobals;

COrganizeFavorite::COrganizeFavorite()
{
	m_olddata[0] = 0;
}

COrganizeFavorite::~COrganizeFavorite()
{
}


// Function to initialize  Organize Favorite Dialog Box //
wyBool
COrganizeFavorite::Display(HWND hwnd)
{	
	CFavoriteMenu		menu;

	DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_ORGFAVORITES) ,
					pGlobals->m_pcmainwin->GetHwnd(), COrganizeFavorite::OrganizeFavoriteDlgProc, (LPARAM)this);

	menu.Display(pGlobals->m_pcmainwin->GetHwnd());

	return wyFalse;
}


INT_PTR  CALLBACK 
COrganizeFavorite::OrganizeFavoriteDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	COrganizeFavorite	*favor=(COrganizeFavorite*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	DlgControl* pdlgctrl;
	switch(message)
	{
		case WM_INITDIALOG:
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
            LocalizeWindow(hwnd);
			PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
			break;

		case WM_INITDLGVALUES:
            favor->OnWMInitdlgValues(hwnd);
			break;
		
		case WM_COMMAND :
		    favor->OnWMCommand(hwnd, wParam);
			break;

	    case WM_NOTIFY : 
            favor->OnWMNotify(hwnd, lParam);
			break;

		case WM_GETMINMAXINFO:
			favor->OnWMSizeInfo(lParam);
			break;
				
		case WM_SIZE:
			favor->OrgFavResize(hwnd);
			break;

		case WM_PAINT:
			favor->OnPaint(hwnd);
            return 1;
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
			SaveInitPos(hwnd, ORGFAV_SECTION);
			break;
	
	    default :
		    break;
	}

	return 0;
}

/* Subclassed procedure for tree control user to handle VK_f2 and VK_RETURN */

LRESULT CALLBACK
COrganizeFavorite::TreeWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	COrganizeFavorite *favor = (COrganizeFavorite*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	
	switch(message) 
	{
		case WM_KEYDOWN:
			switch(wParam)
			{
				case VK_F2:
					favor->RenameFavorite(hwnd);
					break;
				
				case  VK_DELETE :
					favor->DeleteFavoriteFromTree(hwnd);
					break;
				
				case  VK_INSERT :
					favor->CreateNewFolder(hwnd);
					break;
			
				default:
					break;
			}
			break;
	
		default:
			break;
	}
	return CallWindowProc(favor->m_wndproc, hwnd, message, wParam, lParam); 
}

INT_PTR  CALLBACK 
COrganizeFavorite::MovetoFolderProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	COrganizeFavorite *favor = (COrganizeFavorite*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	
	switch(message)
	{
		case WM_INITDIALOG:
			SetWindowLongPtr(hwnd, GWLP_USERDATA, lParam);
            LocalizeWindow(hwnd);
			PostMessage(hwnd, WM_INITDLGVALUES, 0, 0);
			break;

		case WM_INITDLGVALUES:
			SendDlgItemMessage(hwnd, IDC_MOVETREE, TVM_SETIMAGELIST, 0, (LPARAM)favor->m_image.GetImageHandle());
			favor->InitTreeView(GetDlgItem(hwnd , IDC_MOVETREE), wyFalse);
			break;

		case WM_COMMAND :
			switch(LOWORD(wParam))
			{
				case IDOK:
					favor->ProcessOK(hwnd);
					EndDialog(hwnd, 0);
					break;
				
				case IDCANCEL:
					EndDialog(hwnd, 0);
                    break;

				default :
					break;
			}
			break;

		default :
			break;
	}
return 0;
}

// Function to Rename a Favorite //
wyBool
COrganizeFavorite::RenameFavorite(HWND hwnd)
{	
	HTREEITEM	item;
	
	item = TreeView_GetSelection(hwnd) ;
	
	if(!item || item == TreeView_GetRoot(hwnd)) // for root node we  will not allow rename
		return wyFalse;
			
	TreeView_EditLabel(hwnd, item);
	return wyTrue;
}


// Function to select parent Folder // 
wyBool
COrganizeFavorite::ProcessOK(HWND hwnd)
{
	HTREEITEM	item;
	HWND		htree = GetDlgItem(hwnd, IDC_MOVETREE);

	item = TreeView_GetSelection(htree);
	// to get  destination folder path
	if(SelItemPath(htree, item, m_destpath) == wyFalse)
        return wyFalse;

	return wyTrue;
}

wyBool
COrganizeFavorite::CreateNewFolder(HWND  tree)
{	
	wyInt32			state;					
	wyString		foldername;			
	TVITEM			item;
	wyString		path;
    wyString        strfolder("");
	HTREEITEM		child ,selitem , fold ;
	
	// Initially keep the new favorite name as "New Folder"
	foldername.SetAs("New Folder");
	selitem = TreeView_GetSelection(tree);
	state = TreeView_GetItemState(tree, selitem, TVIS_STATEIMAGEMASK);

    item.mask = TVIF_IMAGE;
	item.hItem = selitem;
	
	TreeView_GetItem(tree, &item);
	if(SelItemPath(tree, selitem, path, wyTrue) == wyFalse)
        return wyFalse;

	child = TreeView_GetChild(tree, selitem);
    CreateUniqueFolderName(tree, (item.iImage == NFILE)?selitem:child, foldername);

    fold = AddItem(tree, (item.iImage == NFILE)?TreeView_GetParent(tree, selitem): selitem, foldername.GetString(), 1);
	TreeView_Select(tree, fold, TVGN_CARET);	

    path.AddSprintf("\\%s", foldername.GetString()) ;
	
	if(InsertFavoriteItem(path, wyTrue, strfolder))
	{
		TreeView_EditLabel(tree, fold);
		return wyTrue;
	}
	else 
		TreeView_DeleteItem(tree, fold);
		return wyFalse;
 }

// Function to complete Rename Favorite & Create new Favorite //
wyBool
COrganizeFavorite::EndLabelEdit(HWND hwnd, LPNMTVDISPINFO  tvdisp)
{	
	HWND			htree;
	wyWChar			newdata[MAX_PATH + 1] = {0};		/*new data after editing */
	TVITEM			item;
	wyString		path, oldname, newname, error;
	HTREEITEM		hitem , parent;
	
	htree = GetDlgItem(hwnd, IDC_ORGTREE);
	hitem = TreeView_GetSelection(htree);
	parent = TreeView_GetParent(htree, hitem);

	if(SelItemPath(htree, parent, path) == wyFalse)
        return wyFalse;

	if(tvdisp->item.pszText != NULL)
	{	
		wcsncpy(newdata , tvdisp->item.pszText, MAX_PATH);
		if(wcslen (newdata) == 0)
			wcsncpy(newdata , m_olddata, MAX_PATH);
	}
	else
		// if the user did not edit the favorite then keep the old name 
		wcsncpy(newdata, m_olddata, MAX_PATH);
		
	wyString	olddatastr(m_olddata);
	wyString	newdatastr(newdata);
	oldname.Sprintf("%s\\%s", path.GetString(), olddatastr.GetString());
	newname.Sprintf("%s\\%s", path.GetString(), newdatastr.GetString());
	
	item.mask = TVIF_IMAGE;
	item.hItem = hitem;
	
	TreeView_GetItem ( htree , &item );
	if ( item.iImage == NFILE )
	{
		oldname.Add(".sql");
		newname.Add(".sql"); 
	}
    error.SetAs(_("Error renaming "));
	error.AddSprintf("'%s'.", olddatastr.GetString());

	// Movefile function is for renaming an existing file  or to move  file to another location// 
	// oldname is the existing file path  & newname is the new path of the file 
    if(::MoveFile(oldname.GetAsWideChar(), newname.GetAsWideChar()))
	{	
		// set the favorite name as the new name 
		item.mask		=  TVIF_TEXT ;
		item.hItem		=  tvdisp->item.hItem ;
		item.cchTextMax =  MAX_PATH;
		item.pszText	=  newdata;
		
		TreeView_SetItem ( GetDlgItem ( hwnd , IDC_ORGTREE ), &item );
		return wyTrue;
	}
	
	DisplayErrorText(GetLastError(), error.GetString());
	return wyFalse;
}

wyBool
COrganizeFavorite::MoveToFolder(HWND hwnd)
{	
	HWND			htree;
	wyWChar			data[MAX_PATH+1] = {0};
	TVITEM			item;
	wyString		srcpath, error;
	HTREEITEM		selitem;		// selected item in Organize Favorite Treeview //
		
	htree = GetDlgItem(hwnd, IDC_ORGTREE);
	selitem = TreeView_GetSelection(htree);

	if(SelItemPath(htree, selitem, srcpath) == wyFalse)
        return wyFalse;

	item.mask			= TVIF_IMAGE | TVIF_TEXT;
	item.hItem			= selitem;
	item.cchTextMax		= MAX_PATH;
	item.pszText		= data;
	
	TreeView_GetItem(htree, &item);
	
	if(item.iImage == NFILE)
	{
		wcscat(data, L".sql");
		srcpath.Add(".sql");
	}	

	wyString	datastr(data);
	
	DialogBoxParam(pGlobals->m_hinstance, MAKEINTRESOURCE(IDD_MOVETOFOLDER),	
						hwnd, COrganizeFavorite::MovetoFolderProc, (LPARAM)this);
	
	if(m_destpath.GetLength() == 0) 
		return wyFalse;

	m_destpath.AddSprintf("\\%s", datastr.GetString());
    if(!::MoveFile(srcpath.GetAsWideChar(), m_destpath.GetAsWideChar()))
    {
		error.Sprintf(_("You cannot move '%s' to itself."), datastr.GetString());
		DisplayErrorText(GetLastError(), (char*) error.GetString());
		return wyFalse;
	}

	m_destpath.SetAs("");
	return wyTrue;
}

/// Function to delete the  selected Favorite //
wyBool
COrganizeFavorite::DeleteFavoriteFromTree(HWND htree)
{
	INT				ret;
	wyWChar          data[MAX_PATH + 1] = {0};
	TVITEM			item;
	wyString		msg;
	HTREEITEM		selitem ;
		
	selitem = TreeView_GetSelection(htree);
	
	if(selitem == NULL) 
		return wyFalse;
		
	if(selitem == TreeView_GetRoot(htree)) // for root node we will not allow delete
		return wyFalse;
	
	item.mask		= TVIF_TEXT;
	item.hItem		= selitem;
	item.cchTextMax = MAX_PATH;
	item.pszText	= data;

	TreeView_GetItem(htree , &item);
	
	wyString	datastr(data);

	msg.Sprintf(_("Are you sure want to delete '%s'?"), datastr.GetString());
	ret = yog_message(htree, msg.GetAsWideChar(), 
						pGlobals->m_appname.GetAsWideChar(), MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2);
		
	switch(ret)
	{
		case IDYES :
			break;

		case IDNO:
		case IDCANCEL :
			return wyTrue;
	}
	
	if(ProcessDelete(htree, selitem))
		TreeView_DeleteItem(htree, selitem);

	return wyTrue;
}

// Recursive function to delete the given item  & its sibling//
wyInt32 
COrganizeFavorite::DeleteFavoriteItem(HWND htree, HTREEITEM hitem)
{
	HTREEITEM	sibling;

	if(!ProcessDelete(htree, hitem))
		return wyFalse;

	sibling = TreeView_GetNextSibling(htree, hitem);
	TreeView_DeleteItem(htree, hitem);

	if(sibling)
		DeleteFavoriteItem(htree, sibling);
	
	return wyTrue;
}


// Function to  handle  InitDlgValues message// 
wyBool
COrganizeFavorite::ProcessInitDlg(HWND hwnd)
{
	SendDlgItemMessage(hwnd, IDC_ORGTREE, TVM_SETIMAGELIST, 0, (LPARAM)m_image.GetImageHandle());
	
	InitTreeView(GetDlgItem(hwnd, IDC_ORGTREE), wyTrue);

	// if no files in the favorite folder then disable all the buttons 
	if(TreeView_GetCount(GetDlgItem(hwnd, IDC_ORGTREE)) == 0)
	{
        EnableButtons(hwnd, wyFalse);
        ::EnableWindow(GetDlgItem(hwnd, IDC_CREATE), wyFalse);
	}
	return wyFalse;
}

wyBool
COrganizeFavorite::ProcessDelete(HWND htree, HTREEITEM  hitem)
{
	TVITEM			item;
	wyString		itempath;
	HTREEITEM		child ;

	item.mask		= TVIF_IMAGE ;
	item.hItem		= hitem;
	
	TreeView_GetItem(htree, &item);
	
	if(SelItemPath(htree, hitem, itempath) == wyFalse)
        return wyFalse;
	
	// if the selected item is a folder , delete all its child then delete the folder//
	if(item.iImage == NDIR)
	{
		child = TreeView_GetChild(htree, hitem);
		
		if(child)
			DeleteFavoriteItem(htree, child);
		 		
		if(RemoveDirectory( itempath.GetAsWideChar()) == 0) 
            return OnError(_("Error deleting the favorite item."));
	}
	else
	{			
		itempath.Add(".sql");
	
		if (DeleteFile(itempath.GetAsWideChar()) == 0)
            return OnError(_("Error deleting the favorite item."));
	}		
	return wyTrue;
}

wyInt32 
COrganizeFavorite::GetSelectionImage(HWND hwnd)
{
	HTREEITEM	hItem;
    wyInt32     image;

	hItem = TreeView_GetSelection(hwnd);
    image = GetItemImage(hwnd, hItem); 

	return image;
}

wyInt32 
COrganizeFavorite::GetItemImage(HWND hwnd, HTREEITEM hItem)
{
	TVITEM		tvi;
	
	memset(&tvi, 0, sizeof(TVITEM));

	tvi.mask = TVIF_IMAGE;
	tvi.hItem = hItem;

	TreeView_GetItem(hwnd, &tvi);

	return tvi.iImage;
}

wyBool 
COrganizeFavorite::ReadFromFile(HWND hwnd, wyChar *completepath)
{	
	wyChar		*sqltext = 0;
	wyUInt32    dwfilesize, dwbyteswritten;
	HANDLE		hfile;
	wyString	completepathstr, scitext;

	completepathstr.SetAs(completepath);

	hfile		=	CreateFile(completepathstr.GetAsWideChar(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,
								NULL, NULL);
	
	if(hfile == INVALID_HANDLE_VALUE) 
    {
		if(GetLastError() == ERROR_FILE_NOT_FOUND)
		{
			wyString error;
			error.Sprintf(_("Could not find '%s'\nPlease verify the referenced file exists."), completepath);
			DisplayErrorText(GetLastError(), error.GetString());
		}
		else
			DisplayErrorText(GetLastError(), _("Could not Open Favorite File."));

		return wyFalse;
	}

	dwfilesize = GetFileSize(hfile, NULL);
	
	if(dwfilesize == 0)
    {
		yog_message(pGlobals->m_pcmainwin->GetHwnd(), _(L"File is Empty"), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK);
		CloseHandle(hfile);
		return wyFalse;
	}

	sqltext = AllocateBuff(dwfilesize + 1);

	if(ReadFile(hfile, sqltext, dwfilesize, (LPDWORD)&dwbyteswritten, NULL) == 0)
	{	
		DisplayErrorText(GetLastError(), _("Error reading favorite file."));
        VERIFY(::CloseHandle(hfile));
		free(sqltext);
		return wyFalse;
	}

	if(sqltext)
		scitext.SetAs(sqltext);

	::SendMessage(hwnd, WM_SETTEXT, (WPARAM)dwfilesize, (LPARAM)scitext.GetAsWideChar());
		
    VERIFY(::CloseHandle(hfile));
	free(sqltext);
	return wyFalse;
}

void 
COrganizeFavorite::OnWMInitdlgValues(HWND hwnd)
{
	MDIWindow *wnd= GetActiveWin();
	HWND hwndpreview = GetDlgItem(hwnd, IDC_PREVIEW);

	m_hwnd = hwnd;

    ProcessInitDlg(hwnd);
	// subclass the tree control so that we can handle VK_F2 and VK_RETURN //
	m_wndproc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwnd, IDC_ORGTREE), GWLP_WNDPROC, (LONG_PTR)COrganizeFavorite::TreeWndProc);
	
	// attempt to set scintilla code page to support utf8 data

    //set the lexer language 
	SendMessage(hwndpreview, SCI_SETLEXERLANGUAGE, 0, (LPARAM)"MySQL");

	SendMessage(hwndpreview, SCI_SETCODEPAGE, SC_CP_UTF8, 0);

	EditorFont::FormatEditor(hwndpreview, wyTrue, wnd->m_keywordstring, wnd->m_functionstring);

	/* make the scintilla preview control word wrap */
	SendMessage(hwndpreview, SCI_SETWRAPMODE, SC_WRAP_WORD, SC_WRAP_WORD);

	SetWindowLongPtr(GetDlgItem(hwnd, IDC_ORGTREE), GWLP_USERDATA, (LONG_PTR)this);

    //Line added because on changing color there was margin coming for editor
    SendMessage(hwndpreview, SCI_SETMARGINWIDTHN,1,0);
	
    GetClientRect(m_hwnd, &m_dlgrect);
	GetWindowRect(m_hwnd, &m_wndrect);

	//set the initial position of the dialog
	SetWindowPositionFromINI(m_hwnd, ORGFAV_SECTION, 
		m_wndrect.right - m_wndrect.left, 
		m_wndrect.bottom - m_wndrect.top);
	
	GetCtrlRects();
	PositionCtrls();
}

void 
COrganizeFavorite::OnWMCommand(HWND hwnd, WPARAM wParam)
{
	switch(LOWORD(wParam))
	{
        case IDC_RENAME :
			RenameFavorite(GetDlgItem(hwnd, IDC_ORGTREE));
			break;
    
		case IDC_DELETE :
			DeleteFavoriteFromTree(GetDlgItem(hwnd , IDC_ORGTREE));
			break;

		case IDC_MOVETO :
			if(MoveToFolder(hwnd))
			    InitTreeView(GetDlgItem(hwnd, IDC_ORGTREE), wyTrue) ;
			break;

		case IDC_CREATE :
			CreateNewFolder(GetDlgItem(hwnd, IDC_ORGTREE));
			break;

		case IDCANCEL:
			EndDialog(hwnd, 1);
			break;

		default:
			break;
	}
}

void 
COrganizeFavorite::OnWMNotify(HWND hwnd, LPARAM lParam)
{
	LPNMHDR			lpnmhdr = (LPNMHDR)lParam;
	LPNMTVDISPINFO	tvdisp  = (LPNMTVDISPINFO)lParam ;

	switch(lpnmhdr->code)
	{
		case TVN_SELCHANGED:
			if(TreeView_GetSelection(GetDlgItem(hwnd, IDC_ORGTREE)) == TreeView_GetRoot(GetDlgItem(hwnd , IDC_ORGTREE)))
			{
                EnableButtons(hwnd, wyFalse);
                ::SendMessage(GetDlgItem(hwnd, IDC_PREVIEW), SCI_SETREADONLY, (WPARAM)FALSE, (LPARAM)0);
                ::SendMessage(GetDlgItem(hwnd, IDC_PREVIEW), SCI_SETTEXT, (WPARAM)strlen(""), (LPARAM)"");
                ::SendMessage(GetDlgItem(hwnd, IDC_PREVIEW), SCI_SETREADONLY, (WPARAM)TRUE, (LPARAM)0);
			}
			else
			{
                EnableButtons(hwnd, wyTrue);
				::SendMessage(GetDlgItem(hwnd, IDC_PREVIEW), SCI_SETREADONLY, (WPARAM)FALSE, (LPARAM)0);
				
                if(GetSelectionImage(GetDlgItem(hwnd, IDC_ORGTREE)) == NFILE)
					HandleFileSelection(hwnd);
				else
					SendMessage(GetDlgItem(hwnd, IDC_PREVIEW), SCI_SETTEXT, (WPARAM)strlen(""), (LPARAM)"");
						
				SendMessage(GetDlgItem(hwnd, IDC_PREVIEW), SCI_SETREADONLY, (WPARAM)TRUE, (LPARAM)0);
				
			}
			break;
		
		case TVN_BEGINLABELEDIT:
			// before editing store the old text//
			wcsncpy(m_olddata , tvdisp->item.pszText, MAX_PATH - 1);
			/* Since on pressing enter we need to close the editing, we need to set this flag and handle IDOK and IDCANCEL */
            ::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, wyFalse);
			break;
		
		case TVN_ENDLABELEDIT:
            EndLabelEdit(hwnd, tvdisp) ;
            ::SetWindowLongPtr(hwnd, DWLP_MSGRESULT, wyTrue );
			break;
						
		default:
			break;
	}
}

void 
COrganizeFavorite::EnableButtons(HWND hwnd, wyBool isenable)
{
    wyInt32 size = 0, j = 0;
    wyInt32 buttonids[] = {IDC_RENAME, IDC_MOVETO, IDC_DELETE};

	size = sizeof(buttonids)/sizeof(buttonids[0]);

	for(j = 0; j < size; j++)
        ::EnableWindow(::GetDlgItem(hwnd, buttonids[j]), isenable);
}

void 
COrganizeFavorite::HandleFileSelection(HWND hwnd)
{
    wyString	path, textbufferstr;
    wyChar		*sztext = 0, *trimmed = 0;
	DWORD		dwfilesize, dwbyteswritten;
	HANDLE		hfile;
	wyString	szfiletextstr;

	if(SelItemPath(GetDlgItem(hwnd, IDC_ORGTREE), TreeView_GetSelection(GetDlgItem(hwnd, IDC_ORGTREE)), path) == wyFalse)
        return;

	path.Add(".sql");
    hfile		=	::CreateFile(path.GetAsWideChar(), GENERIC_READ, FILE_SHARE_READ, 
                                NULL, OPEN_EXISTING, NULL, NULL);

	if(hfile == INVALID_HANDLE_VALUE) 
	{
			DisplayErrorText(GetLastError(), _("Could not open the referenced query file.\nMake sure that the query file exists."));
			return;
	}

    dwfilesize = ::GetFileSize(hfile, NULL);
	if(dwfilesize == 0)
    {
        ::CloseHandle(hfile);
		return;
	}
	
	//isansifile = CheckForUtf8Bom(path);

	sztext  = AllocateBuff(dwfilesize + 1);
	if (ReadFile(hfile, sztext, (dwfilesize + 1), &dwbyteswritten, NULL) == wyFalse)
	{
    	DisplayErrorText(GetLastError(), _("Error reading favorite file."));
		free(sztext);
        ::CloseHandle(hfile);
		return;
	}
	
	sztext[dwbyteswritten] = 0;
	
	//if(isansifile == wyTrue)
		//textbufferstr.SetAs(sztext, wyFalse);
	//else
	//textbufferstr.SetAs(sztext);

	trimmed = TrimLeft(sztext);
    
    if(!trimmed)
	{
		free(sztext);
        return;
	}

	if(strnicmp(trimmed, "file", strlen("file")) == 0)
	{
		trimmed += 5;
        ReadFromFile(GetDlgItem(hwnd , IDC_PREVIEW), trimmed);
	}
	else 
	{
		szfiletextstr.SetAs(sztext);
		::SendMessage(GetDlgItem(hwnd , IDC_PREVIEW), SCI_SETTEXT, (WPARAM)strlen(sztext), (LPARAM)szfiletextstr.GetString());
	}
	
	VERIFY(::CloseHandle(hfile));
	if(sztext)
		free(sztext);
}

void 
COrganizeFavorite::CreateUniqueFolderName(HWND  htree, HTREEITEM node, wyString &foldername)
{
    wyUInt32    count = 1;
    TVITEM	    item;
    wyWChar		data[MAX_PATH + 1] = {0};

	while(node)
	{	
		item.mask		= TVIF_TEXT;
		item.hItem		= node;
		item.cchTextMax = MAX_PATH;
		item.pszText	= data;

		TreeView_GetItem(htree, &item);
        
		if(wcscmp(foldername.GetAsWideChar(), data) != 0 )
			node = TreeView_GetNextSibling(htree, node);
		else
			foldername.Sprintf("New Folder (%d)", ++count);

	}
}

void
COrganizeFavorite::GetCtrlRects()
{
    RECT    rect;
    wyInt32 i, count;
    HWND    hwnd;

    //ctrlid    size-x     size-y
    wyInt32 ctrlid[] = {
       	IDC_ORG_TEXT1, 1, 0,
        IDC_ORG_HORZ, 1, 0,
        IDC_ORG_HORZ2, 1, 0,
        IDC_ORGTREE, 1, 1,
        IDC_PREVIEW, 0, 1,
        IDC_CREATE, 0, 0,
		IDC_MOVETO, 0, 0,
		IDC_RENAME, 0, 0,
        IDC_DELETE, 0, 0,
        IDCANCEL, 0, 0,
		IDC_ORGFAV_GRIP, 0, 0,
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
COrganizeFavorite::PositionCtrls()
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
			switch(pdlgctrl->m_id){
				
				case IDC_ORGFAV_GRIP:
					x = rect.right - width;
					break;

				case IDCANCEL:
					x=rect.right - rightpadding - width;
					break;
				default:
					x = leftpadding;
			}
			
        }
        else
        {
			x = leftpadding;
			width = rect.right - leftpadding - rightpadding;
        }
	    switch(pdlgctrl->m_id)
        {
			case IDCANCEL:
			case IDC_ORG_HORZ2:
			    y = rect.bottom - bottompadding - height;
                break;

			case IDC_ORGFAV_GRIP:
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
COrganizeFavorite::OrgFavResize(HWND hwnd){

	PositionCtrls();
    InvalidateRect(GetDlgItem(hwnd, IDC_ORG_TEXT1), NULL, TRUE);
    UpdateWindow(GetDlgItem(hwnd, IDC_ORG_TEXT1));
}

void
COrganizeFavorite::OnWMSizeInfo(LPARAM lparam)
{
    MINMAXINFO* pminmax = (MINMAXINFO*)lparam;
    pminmax->ptMinTrackSize.x = m_wndrect.right - m_wndrect.left;
    pminmax->ptMinTrackSize.y = m_wndrect.bottom - m_wndrect.top;
}

//function handles the WM_PAINT and paints the window using double buffering
void
COrganizeFavorite::OnPaint(HWND hwnd)
{
    HDC         hdc;
	PAINTSTRUCT ps;
	HWND hwndstat = GetDlgItem(hwnd, IDC_ORGFAV_GRIP);
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