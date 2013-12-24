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

#include "FindAndReplace.h"

FindAndReplace::FindAndReplace(HWND hwnd)
{
	m_hwndedit = hwnd;

	m_havefound = wyFalse;
	m_firstfound = 0;
	m_count = 0;
	m_cyclecompleted = wyFalse;
	m_startpos = 0;
	m_found = wyFalse;
	m_flag = wyFalse;
    m_findflag = 0;
}

FindAndReplace::~FindAndReplace()
{    

}

/**
 Function to implement FIND and FIND REPLACE & REPLACE ALL functionality.
 This function is called whenever there is a message from the findreplace dialog box.
*/
wyBool
FindAndReplace::FindReplace(HWND hwnd, LPARAM lparam, wyBool val)
{
	LPFINDREPLACE		lpfr;
	lpfr = (LPFINDREPLACE)lparam;

    m_findflag = lpfr->Flags;

	// check if the message to end the dialog.
	if(lpfr->Flags & FR_DIALOGTERM) 
	{
		OnEndDlg(lpfr);
		return wyFalse;
	}
		
	 //save the selection in m_lastfind
	CopyFindData(&m_lastfind, lpfr);
	
	//if find next is pressed
	if(lpfr->Flags & FR_FINDNEXT) 
	{
        //save the search text and search option
        pGlobals->m_pcmainwin->m_findtext.SetAs(lpfr->lpstrFindWhat);
        CopyFindData(&pGlobals->m_pcmainwin->m_frstruct, lpfr);

		// for Query Editor Find 
		if(val == wyFalse)
		{
			 OnFindNext(hwnd, lpfr);
		}
		else
		{
			//for Find In Show result in text
			FindNextText(hwnd, lparam);
		}

        //set the focus to owner window
        //SetFocus(lpfr->hwndOwner);

        return wyTrue;
	}
		
	// if replace button is pressed
	if(lpfr->Flags & FR_REPLACE) 
	{
		OnReplaceButton(lpfr);
		return wyTrue;
	}
		
	// if replace all is clicked
	if(lpfr->Flags & FR_REPLACEALL) 
	{   OnReplaceAll(lpfr);
		return wyTrue;
	}

	return wyTrue;
}

// Finds the next text in the Edit control(Show result in text)
void
FindAndReplace::FindNextText(HWND hwnd, LPARAM lparam)
 {
	LPFINDREPLACE       lpfr;
	FINDTEXT            ft;
	wyUInt32            findflags = 0;
	wyInt32             curstart = 0, curend = 0, ret = 0;
	wyBool				upwardsearch = wyFalse;
	wyBool				downwardsearch = wyFalse;
	wyWChar				*findwhat;
	wyString			findstring;
	
	lpfr =(LPFINDREPLACE)lparam;

	// check if the message to end the dialog.
	if(lpfr->Flags & FR_DIALOGTERM)
	{ 
		pGlobals->m_pcmainwin->m_finddlg = NULL;
		//SetFocus(hwnd);
		return; 
	}	

	//if find next is pressed
	if(lpfr->Flags & FR_FINDNEXT)
	{	
		//get the current cursor position
 		ret = SendMessage(hwnd, EM_GETSEL,(WPARAM)&curstart,(LPARAM)&curend);
        
		if(curstart == curend)
		{
			m_flag = wyTrue;
		}
		if((m_flag == wyTrue) && (m_firstfound == curstart) && (m_count >= 2))
		{
            ret = ShowMessage(FINISHED_ONECYCLE, MB_OK | MB_ICONINFORMATION);
			
			if(ret == IDOK)
			{
				m_count = 0;
				m_firstfound = 0;
				return;
			}
		}

		//initialize The findtext structure
		ft.lpstrText = lpfr->lpstrFindWhat;
		findwhat = lpfr->lpstrFindWhat;
		findstring.SetAs(findwhat);
		if((m_findwhattext.Compare(findstring)) != 0)
		{
			m_count = 0;
		}
			
		//set the find flags
		if(lpfr->Flags & FR_DOWN)
        {
			findflags |= FR_DOWN;
			ft.chrg.cpMax = -1;
			ft.chrg.cpMin = curend;
			downwardsearch = wyTrue;
		} 
        else 
        {
			upwardsearch = wyTrue;
			ft.chrg.cpMax = 0;
			ft.chrg.cpMin = curstart;
		}	

		if(lpfr->Flags & FR_MATCHCASE)
			findflags |= FR_MATCHCASE;
		if(lpfr->Flags & FR_WHOLEWORD)
			findflags |= FR_WHOLEWORD;
		
		
		//find the text
		curstart = SendMessage(hwnd, EM_FINDTEXT, (WPARAM)findflags, (LPARAM)&ft);
			
		if(((m_count >= 2)) && (m_firstfound == curstart) && ((m_findwhattext.Compare(findstring))== 0))
		{
            ret = ShowMessage(FINISHED_ONECYCLE, MB_OK | MB_ICONINFORMATION);
			
			if(ret == IDOK)
			{
				m_count = 0;
				m_firstfound = 0;
				return;
			}
		}
		if(m_count == 0)
		{
			m_found = wyFalse;
			m_firstfound = curstart;
			m_findwhattext.SetAs(findwhat);
		}
		
		
		m_count ++;
		if(curstart != -1)
		{
			 //if found select and hilight it
			m_found = wyTrue;
			curend = curstart + wcslen(ft.lpstrText);
			SendMessage(hwnd, EM_SETSEL,(WPARAM)curstart,(LPARAM)curend);
		}

		
		else if((m_found == wyTrue) && (upwardsearch == wyTrue))
		{
			ret = FindNextconfirmationMessage(wyTrue);
			
			if(ret != IDYES)
			{
				m_count = 0;
				m_firstfound = 0;
				m_findwhattext.Clear();
                return;
			}
			else
			{
				ft.chrg.cpMax = 0;
				ft.chrg.cpMin = SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
				curstart = SendMessage(hwnd, EM_FINDTEXT,(WPARAM)findflags,(LPARAM)&ft);
				curend = curstart + wcslen(ft.lpstrText);
				SendMessage(hwnd, EM_SETSEL,(WPARAM)curstart,(LPARAM)curend);
				return;
			}
		}
		else if(m_found == wyTrue)
		{
			ret = FindNextconfirmationMessage(wyFalse);
			
			if(ret != IDYES)
			{
				m_count = 0;
				m_firstfound = 0;
				m_findwhattext.Clear();
                return;
			}
			ft.chrg.cpMax = -1;
			ft.chrg.cpMin = 0;
			curstart = SendMessage(hwnd, EM_FINDTEXT,(WPARAM)findflags,(LPARAM)&ft);
			curend = curstart + wcslen(ft.lpstrText);
			SendMessage(hwnd, EM_SETSEL,(WPARAM)curstart,(LPARAM)curend);
			if(((m_count >= 2)) && (m_firstfound == curstart) && ((m_findwhattext.Compare(findstring))== 0))
			{
                ret = ShowMessage(FINISHED_ONECYCLE, MB_OK | MB_ICONINFORMATION);
			
				if(ret == IDOK)
				{
					m_count = 0;
					m_firstfound = 0;
					return;
				}
			}
		
		}
		else if((curstart == -1) && (m_found == wyTrue) && (m_count == 1) && ((m_findwhattext.Compare(findstring))== 0))
		{
            ret = ShowMessage(FINISHED_ONECYCLE, MB_OK | MB_ICONINFORMATION);
			
			if(ret == IDOK)
			{
				m_count = 0;
				m_firstfound = 0;
				return;
			}
			
		}
		else
		{
			if((upwardsearch == wyTrue) && (m_count == 1))
			{
				ft.chrg.cpMax = 0;
				ft.chrg.cpMin = SendMessage(hwnd, WM_GETTEXTLENGTH, 0, 0);
				curstart = SendMessage(hwnd, EM_FINDTEXT,(WPARAM)findflags,(LPARAM)&ft);
				curend = curstart + wcslen(ft.lpstrText);
				if(curstart != -1)
				{
					ret = FindNextconfirmationMessage(wyTrue);
			
					if(ret != IDYES)
					{
						m_count = 0;
						m_firstfound = 0;
						m_findwhattext.Clear();
						return;
					}
					
					SendMessage(hwnd, EM_SETSEL,(WPARAM)curstart,(LPARAM)curend);
				}
			}
			if((downwardsearch == wyTrue) && (m_count == 1))
			{
				ft.chrg.cpMax = -1;
				ft.chrg.cpMin = 0;
				curstart = SendMessage(hwnd, EM_FINDTEXT,(WPARAM)findflags,(LPARAM)&ft);
				curend = curstart + wcslen(ft.lpstrText);
				if(curstart != -1)
				{
					ret = FindNextconfirmationMessage(wyFalse);
			
					if(ret != IDYES)
					{
						m_count = 0;
						m_firstfound = 0;
						m_findwhattext.Clear();
						return;
					}
					
					SendMessage(hwnd, EM_SETSEL,(WPARAM)curstart,(LPARAM)curend);
				}
				
			}
			
			if(curstart == -1)
			{
			    if(m_count == 1)
				{
					NotFoundMsg(ft.lpstrText);
					m_count = 0;
					m_firstfound = 0;
					return;
				}
				if(m_count >= 2)
				{
                    ret = ShowMessage(FINISHED_ONECYCLE, MB_OK | MB_ICONINFORMATION);
			
					if(ret == IDOK)
					{
						m_count = 0;
						m_firstfound = 0;
						return;
					}
				
				}
			}
		}
	}
	
	return;
}
void
FindAndReplace::OnFindNext(HWND hwnd, LPFINDREPLACE& lpfr)
{
	wyInt32 find;
	
	find = FindNext(lpfr->lpstrFindWhat, 
							(!(lpfr->Flags & FR_DOWN)),
							(lpfr->Flags & FR_WHOLEWORD), 
							(lpfr->Flags & FR_MATCHCASE));
	
	if(!find)
		NotFoundMsg(lpfr->lpstrFindWhat);

	return;
}

void
FindAndReplace::NotFoundMsg(wyString tofind)
{

	wyString msg;
	
	msg.AddSprintf(_("Finished searching the document. Cannot find \"%s\""), tofind.GetString());
	
    ShowMessage(msg.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
}
//finds next text in a scintilla control (Query Editor)	
wyInt32
FindAndReplace::FindNext(wyWChar *findwhat, wyInt32 reversedirection, wyInt32 wholeword, wyInt32 matchcase)
{
	
	wyInt32         flags = 0;
	wyInt32         startposition;   //Starting postion to search a text
	wyInt32         endposition;	 //Search a text till the end position
	wyInt32			ret; 
    wyInt32         posfind; //Postion where the string is found
	wyInt32         start, end;
    CharacterRange  cr;
	wyString		findwhatstr;
	wyBool			upwardsearch = wyFalse;
	wyInt32			*count;// = 0;
    wyBool          condition = wyFalse;
	
	findwhatstr.SetAs(findwhat);

	//how many times the selected text found 
	count = &pGlobals->m_findcount;

	 //set appropriate flags
	if(wholeword)
        flags |= SCFIND_WHOLEWORD; 

	if(matchcase)
        flags |= SCFIND_MATCHCASE; 
	
	cr.cpMin = SendMessage(m_hwndedit, SCI_GETSELECTIONSTART, 0, 0);
	cr.cpMax = SendMessage(m_hwndedit, SCI_GETSELECTIONEND, 0, 0);
    
	if(*count == 0)
	{
		m_cyclecompleted = wyFalse;
		m_startpos = cr.cpMin;
		
        if(cr.cpMin == cr.cpMax)
		{
            if(reversedirection)
            {
		        upwardsearch = wyTrue;
		        startposition = cr.cpMin - 1;
		        endposition = 0;
        	}
            else
            {
			    startposition = cr.cpMin;//search starting from the current cursor position
			    endposition = SendMessage(m_hwndedit, SCI_GETLENGTH, 0, 0);
            }

			posfind = FindTextPostion(&findwhatstr, flags, startposition, endposition);

			if(posfind == -1)
			{
				goto start ;//if text not found, it will start search from beginning
			}
			else
			{
				(*count)++;  
			    goto label;
			}

		}
	}

    if(reversedirection == wyFalse)
        condition = (m_startpos <= cr.cpMin) ? wyTrue : wyFalse;
    else
        condition = (m_startpos >= cr.cpMin) ? wyTrue : wyFalse;

	//If one cycle of serching a text is completed
    if((*count != 0) && condition && m_cyclecompleted == wyTrue)
	{
        ShowMessage(FINISHED_ONECYCLE, MB_OK | MB_ICONINFORMATION);
		m_cyclecompleted = wyFalse;
		*count = 0;
		m_startpos = 0;
		return 1;
	}

	startposition   = cr.cpMax;
	endposition     = SendMessage(m_hwndedit, SCI_GETLENGTH, 0, 0);
	
	//If Up Radio Button is checked
	if(reversedirection)
    {
		upwardsearch = wyTrue;
		startposition = cr.cpMin - 1;
		endposition = 0;
		
	} 
    posfind = FindTextPostion(&findwhatstr, flags, startposition, endposition);
	
start:
	if(posfind == -1)
	{
		startposition = 0;
		endposition = SendMessage(m_hwndedit, SCI_GETLENGTH, 0, 0);
		posfind = FindTextPostion(&findwhatstr, flags, startposition, endposition);
		
		//If text not found
		if(posfind == -1)
		{
			*count = 0;
			m_startpos = 0;
			return 0;
		}
		
		//Handles if beginning of the document is reached
		if(upwardsearch == wyTrue)
		{
			ret = FindNextconfirmationMessage(wyTrue);
			
			if(ret != IDYES)
				return 1;

            m_cyclecompleted = wyTrue;

			startposition = SendMessage(m_hwndedit, SCI_GETLENGTH, 0, 0);
			endposition = 0;
			posfind = FindTextPostion(&findwhatstr, flags, startposition, endposition);
			if(posfind == -1)
			{
				*count = 0;
				m_startpos = 0;
				return 0;
			}
			else
			{
				(*count)++;
				goto label;
			}
		}
		
		//Handles if end of the document is reached 
		ret = FindNextconfirmationMessage(wyFalse);
			
		if(ret != IDYES)
			return 1;
		m_cyclecompleted = wyTrue;
		startposition = 0; 
		endposition = SendMessage(m_hwndedit, SCI_GETLENGTH, 0, 0);
		posfind = FindTextPostion(&findwhatstr, flags, startposition, endposition);
		if(posfind == -1)
		{
			*count = 0;
			m_startpos = 0;
			return 0;			
		}
	}
	
		(*count)++;  
label:	start   = SendMessage(m_hwndedit, SCI_GETTARGETSTART, 0, 0);
		end     = SendMessage(m_hwndedit, SCI_GETTARGETEND, 0, 0);
		EnsureRangeVisible(m_hwndedit, start, end);
		SendMessage(m_hwndedit, SCI_SETSEL, start, end);

		return 1;
}

wyInt32 
FindAndReplace::FindTextPostion(wyString * findwhatstr, wyInt32 flags, wyInt32 startposition, wyInt32 endposition)
{
	wyInt32 posfind;

	SendMessage(m_hwndedit, SCI_SETTARGETSTART, startposition, 0);
	SendMessage(m_hwndedit, SCI_SETTARGETEND, endposition, 0);
	if(flags == SCFIND_WHOLEWORD && flags == SCFIND_MATCHCASE)
	{
		SendMessage(m_hwndedit, SCFIND_WHOLEWORD, flags, 0);
		SendMessage(m_hwndedit, SCI_SETSEARCHFLAGS, flags, 0);
	}
	else if(flags == SCFIND_WHOLEWORD)
	{
		SendMessage(m_hwndedit, SCFIND_WHOLEWORD, flags, 0);
		SendMessage(m_hwndedit, SCI_SETSEARCHFLAGS, flags, 0);
	}
	else
	{
		SendMessage(m_hwndedit, SCI_SETSEARCHFLAGS, flags, 0);
	}

	posfind = SendMessage(m_hwndedit, SCI_SEARCHINTARGET, findwhatstr->GetLength(),(LPARAM)findwhatstr->GetString());
	return posfind;
}

wyInt32 
FindAndReplace::FindNextconfirmationMessage(wyBool  isbeginning)
{
	return ShowMessage(isbeginning == wyTrue ? BEGINNING_REACHED : END_REACHED, MB_YESNOCANCEL | MB_DEFBUTTON2 | MB_ICONQUESTION);
}


VOID
FindAndReplace::ReplaceOnce(wyWChar *replacewhat, wyWChar *replacewith) 
{
	wyString strreplacewith;

	strreplacewith.SetAs(replacewith);

	if(m_havefound)
	{
		wyInt32 replacelen = strreplacewith.GetLength();
		
		CharacterRange cr = GetSelection();
		
		::SendMessage(m_hwndedit, SCI_SETTARGETSTART, cr.cpMin, 0);
	
		::SendMessage(m_hwndedit, SCI_SETTARGETEND, cr.cpMax, 0);
		
		::SendMessage(m_hwndedit, SCI_REPLACETARGET, replacelen, (LPARAM)strreplacewith.GetString());

		::SendMessage(m_hwndedit, SCI_SETSEL, cr.cpMin, cr.cpMin + replacelen);

		m_havefound = wyFalse;
	}
}


wyInt32
FindAndReplace::ReplaceAll(wyString& replacewhat, wyString& replacewith, wyUInt32 wholeworld, wyUInt32 matchcase)
{
	if (replacewhat.GetLength() == 0)
	{
		return -1;
	}

	wyInt32 startposition	= 0;
	wyInt32 endposition		= SendMessage(m_hwndedit, SCI_GETLENGTH, 0, 0);

	wyInt32 flags = 0;
	
	if(wholeworld)flags = SCFIND_WHOLEWORD;
	if(matchcase)flags  = SCFIND_MATCHCASE;

	::SendMessage(m_hwndedit, SCI_SETTARGETSTART, startposition, 0);
	::SendMessage(m_hwndedit, SCI_SETTARGETEND, endposition, 0);
	::SendMessage(m_hwndedit, SCI_SETSEARCHFLAGS, flags, 0);
	
	wyInt32 posfind = SendMessage(m_hwndedit, SCI_SEARCHINTARGET, replacewhat.GetLength(), (LPARAM)replacewhat.GetString());

	if((posfind != -1) && (posfind <= endposition))
	{

		::SendMessage(m_hwndedit, SCI_BEGINUNDOACTION, 0, 0);
			
		return Replace(posfind, endposition,
			                    replacewhat, replacewith,
     				            wholeworld, matchcase);
	}

	return 0;
}

wyInt32
FindAndReplace::Replace(wyInt32 posfind, wyInt32 endposition,
					         wyString &replacewhat, wyString &replacewith,
					         wyUInt32 wholeworld, wyUInt32 matchcase)
{   
   
	wyInt32   lastmatch    = posfind;
	wyInt32   replacements = 0;
	
	//Replacement loop
	while(posfind != -1)
	{
		wyInt32 lentarget   = SendMessage(m_hwndedit, SCI_GETTARGETEND, 0, 0) - SendMessage(m_hwndedit, SCI_GETTARGETSTART, 0, 0);
		
		wyInt32 movepasteol = 0;

		wyInt32 replacelen  = replacewith.GetLength();

		wyInt32 lenreplaced = replacelen;
		
		::SendMessage(m_hwndedit, SCI_REPLACETARGET, replacelen, (LPARAM)replacewith.GetString());
		
		//Modify for change caused by replacement
		endposition += lenreplaced - lentarget;
		
		// For the special cases of start of line and end of line
		// something better could be done but there are too many special cases
		lastmatch = posfind + lenreplaced + movepasteol;
		
		if(lentarget == 0)
		{
			lastmatch = SendMessage(m_hwndedit, SCI_POSITIONAFTER, lastmatch, 0);
		}

		if(lastmatch >= endposition)
		{
			// Run off the end of the document/selection with an empty match
			posfind = -1;
		}
		else 
		{
			::SendMessage(m_hwndedit, SCI_SETTARGETSTART, lastmatch, 0);
			::SendMessage(m_hwndedit, SCI_SETTARGETEND, endposition, 0);
			
			posfind = SendMessage(m_hwndedit, SCI_SEARCHINTARGET, replacewhat.GetLength(), (LPARAM)replacewhat.GetString());
		}
		
	    replacements++;
	}
		::SendMessage(m_hwndedit, SCI_SETSEL, lastmatch, lastmatch);

		::SendMessage(m_hwndedit, SCI_ENDUNDOACTION, 0, 0);

		return replacements;
}
 //Function to create a copy of find dialog.
 //Required for subsequent find.

void
FindAndReplace::CopyFindData(FINDREPLACE * pdest, FINDREPLACE * psrc)
{
	if(pdest == psrc)
		return;
	
	// delete dyn. allocated stuff from pDest
	if(pdest->lpstrFindWhat)
	{
		pdest->lpstrFindWhat = 0;
	}

	if(pdest->lpstrReplaceWith)
	{
		pdest->lpstrReplaceWith = 0;
	}
	
	// do a mem copy
	memcpy(pdest, psrc, sizeof(FINDREPLACE));

	 //copy dynamically allocated stuff
	if (pdest->lpstrFindWhat)
		wcscpy(pdest->lpstrFindWhat, psrc->lpstrFindWhat);
	else
		pdest->lpstrFindWhat = 0;

	if (psrc->lpstrReplaceWith)
		wcscpy(pdest->lpstrReplaceWith, psrc->lpstrReplaceWith );
	else
		pdest->lpstrReplaceWith = 0;

	return;
}

void
FindAndReplace::OnEndDlg(LPFINDREPLACE& lpfr)
{
	pGlobals->m_pcmainwin->m_finddlg = NULL;

	//resetting the count to zero
	pGlobals->m_findcount = 0;
	
	// set focus back to the SQL editor.
	SetFocus(m_hwndedit);

	return;
}


void
FindAndReplace::OnReplaceButton(LPFINDREPLACE& lpfr)
{
	CharacterRange crange;
	CharacterRange crtextrange;

	if(m_havefound)
	{
		ReplaceOnce(lpfr->lpstrFindWhat, lpfr->lpstrReplaceWith);

		m_havefound = FindNext(lpfr->lpstrFindWhat, 
									    (wyBool)(!(lpfr->Flags & FR_DOWN)),
										(wyBool)(lpfr->Flags & FR_WHOLEWORD), 
 								        (wyBool)(lpfr->Flags & FR_MATCHCASE));
    }
	else
	{
		crange = GetSelection();
		
		pGlobals->m_findcount = 0;
		
		::SendMessage (m_hwndedit, SCI_SETSEL, crange.cpMin, crange.cpMin);
		
		m_havefound = FindNext(lpfr->lpstrFindWhat, 
									    (wyBool)(!(lpfr->Flags & FR_DOWN)),
										(wyBool)(lpfr->Flags & FR_WHOLEWORD), 
									    (wyBool)(lpfr->Flags & FR_MATCHCASE));
	
		
		crtextrange = GetSelection();
		//checking whether the selected text is same as text to be replaced.
		//if it is not same, then we are selecting the text,then on next replace click we will replace the text.
		//if it is same then we will replace the text.
		if(m_havefound && crtextrange.cpMin == crange.cpMin && crtextrange.cpMax == crange.cpMax )
	    {
		   //replace the selected text
		   ReplaceOnce(lpfr->lpstrFindWhat, lpfr->lpstrReplaceWith);

		  // Find the next given text
		   m_havefound = FindNext(lpfr->lpstrFindWhat, 
									    (wyBool)(!(lpfr->Flags & FR_DOWN)),
										(wyBool)(lpfr->Flags & FR_WHOLEWORD), 
									    (wyBool)(lpfr->Flags & FR_MATCHCASE));
	   }
	}

	//if text not found
	if(m_havefound == wyFalse)
	{
		NotFoundMsg(lpfr->lpstrFindWhat);
		return;
	}
	return;
}

void
FindAndReplace::OnReplaceAll(LPFINDREPLACE& lpfr)
{
	wyString	findwhat;
	wyString	replacewith;

	if(lpfr->lpstrFindWhat)
		findwhat.SetAs(lpfr->lpstrFindWhat);
	if(lpfr->lpstrReplaceWith)
		replacewith.SetAs(lpfr->lpstrReplaceWith);

	wyInt32 replacements = ReplaceAll(findwhat,
			                            replacewith,
										lpfr->Flags & FR_WHOLEWORD,
										lpfr->Flags & FR_MATCHCASE);
	if(replacements == -1)
		return;

	if(!replacements)
	{
		NotFoundMsg(lpfr->lpstrFindWhat);
	}
	else 
	{
		wyString	msg;
		msg.Sprintf(_("Replaced %d instance(s)."), replacements);
        ShowMessage(msg.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
	}
	return;
}

CharacterRange 
FindAndReplace::GetSelection() 
{
	
	CharacterRange crange;
	crange.cpMin = SendMessage(m_hwndedit, SCI_GETSELECTIONSTART, 0, 0);
	crange.cpMax = SendMessage(m_hwndedit, SCI_GETSELECTIONEND, 0, 0);
	
	return crange;
}

//Subclassed procedure for Find/Replace dialog
LRESULT	CALLBACK 
FindAndReplace::FindWndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    HWND    hwndtemp;

    switch(message)
    {
        /*case WM_SETACTIVEWINDOW:
            SetActiveWindow(hwnd);
            return 1*/;

        case WM_COMMAND:

            hwndtemp = (HWND)lparam;

            if(HIWORD(wparam) == BN_CLICKED)
            {
                switch(LOWORD(wparam))
                {
                    //whole word control id
                    case 0x410:
                        if(SendMessage(hwndtemp, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            pGlobals->m_pcmainwin->m_frstruct.Flags |= FR_WHOLEWORD;
                        }
                        else
                        {
                            pGlobals->m_pcmainwin->m_frstruct.Flags ^= FR_WHOLEWORD;
                        }

                        break;

                    //match case control id
                    case 0x411:
                        if(SendMessage(hwndtemp, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            pGlobals->m_pcmainwin->m_frstruct.Flags |= FR_MATCHCASE;
                        }
                        else
                        {
                            pGlobals->m_pcmainwin->m_frstruct.Flags ^= FR_MATCHCASE;
                        }

                        break;

                    //search up control id
                    case 0x420:
                        if(SendMessage(hwndtemp, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            pGlobals->m_pcmainwin->m_frstruct.Flags ^= FR_DOWN;
                        }

                        break;

                    //search down control id
                    case 0x421:
                        if(SendMessage(hwndtemp, BM_GETCHECK, 0, 0) == BST_CHECKED)
                        {
                            pGlobals->m_pcmainwin->m_frstruct.Flags |= FR_DOWN;
                        }

                        break;
                }
            }

            break;
    }

    return CallWindowProc(pGlobals->m_pcmainwin->m_findproc, hwnd, message, wparam, lparam);
}

wyInt32
FindAndReplace::ShowMessage(wyWChar* message, wyInt32 flag)
{
    wyInt32 ret;
    HWND    hwnd;
    
    hwnd = GetActiveWindow();

    if(pGlobals->m_pcmainwin->m_finddlg && IsWindowVisible(pGlobals->m_pcmainwin->m_finddlg))
    {
        EnableWindow(pGlobals->m_pcmainwin->m_finddlg, FALSE);
    }
    
    ret = MessageBox(pGlobals->m_pcmainwin->m_hwndmain, 
                     message, 
                     (m_findflag & FR_REPLACE) ? _(L"Replace") : (m_findflag & FR_REPLACEALL) ? _(L"Replace All") : _(L"Find"), 
                     flag);
	
    if(pGlobals->m_pcmainwin->m_finddlg && IsWindowVisible(pGlobals->m_pcmainwin->m_finddlg))
    {
        EnableWindow(pGlobals->m_pcmainwin->m_finddlg, TRUE);
    }

    if(hwnd == pGlobals->m_pcmainwin->m_finddlg)
    {
        PostMessage(pGlobals->m_pcmainwin->m_hwndmain, UM_SETFOCUSAFTERFINDMSG, (WPARAM)hwnd, 0);
    }

    return ret;
}