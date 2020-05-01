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


#define MyFree(x)HeapFree(GetProcessHeap(),NULL,x)

#include "FrameWindowHelper.h"
#include "ExportMultiFormat.h"
#include "Global.h"
#include "Tunnel.h"			
#include "scintilla.h"
#include "ClientMySQLWrapper.h"
#include "CommonHelper.h"
#include "MySQLVersionHelper.h"
#include "LexHelper.h"
#include "SQLTokenizer.h"
#include "GUIHelper.h"
#include "pcre.h"
#include "PreferenceCommunity.h"
#include "SQLFormatter.h"

#ifndef COMMUNITY
#include "HelperEnt.h"
#include "ConnectionEnt.h"
#include "PreferenceEnt.h"
#include "TabSchemaDesigner.h"
#endif

#define			LOST_CONNECTION_ERROR	2013
#define			MYSQL_SERVER_GONE		2006
#define			NO_DB_SELECTED			1046
#define			MYSQL_CONNECT_ERROR		2003
#define			CR_CONNECTION_ERROR		2002


#define			MAX_QUERY_SIZE			(1024*4)
#define			ZERO					0
#define			ONE						1
#define			TWO						2
#define			IN_TRANSACTION			-10
#define			IN_READONLY				-20

extern PGLOBALS	pGlobals;		
extern HACCEL	g_accel;

// Function to compare two values sent by the qsort and binarysearch function.

wyInt32 compare(const void *arg1, const void *arg2)
{
	return _stricmp((wyChar*)arg1,(wyChar*)arg2);
}

// This is a general function which adds all the query which has been executed in 
// the program into the current MDIWINDOWS QueryHistory Edit Box.
wyInt32 my_query(MDIWindow *wnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *query, 
                 wyUInt32 length, wyBool batch, wyBool isend, wyInt32 * stop, 
                 wyInt32 querycount, wyBool profile, wyBool currentwnd, 
				 bool isread, wyBool isimport, wyBool fksethttpimport, HWND fortransactionprompt)
{
	wyInt32     ret = 0, pos = 0, transactioncheck = -1, presult = 6;
	wyChar		*newquery = 0;
	const wyChar *oldquery = query;
 	wyString    str, Query;
	wyInt64		timetaken = 0, tsrecon = 0;
	wyBool	    isbadforxml = wyFalse;
	wyString	recstr;
    wyChar      *unFormattedQuery = NULL;
    SQLFormatter    formatter;
	
	_ASSERT(wnd);

	// create a buffer.
	if(length == -1)
		length = strlen(query);

	if(length)
	{
		//Formats query only if it's needed to pass 'History' tab
		if(wnd && profile == wyTrue)
        {
            str.SetAs(oldquery);
            formatter.GetQueryWtOutComments(&str, &Query);
            Query.RTrim();
            Query.LTrim();
            if(Query.GetLength())
            {
                newquery = FormatQuery(Query.GetString(), pos);
            }
            else
            {
			    newquery = FormatQuery(oldquery, pos);
            }
            str.Clear();
        }		
		#ifndef COMMUNITY
	//	if(pGlobals->m_entlicense.CompareI("Professional") != 0)
	//	{
			if(wnd->m_conninfo.m_isreadonly == wyTrue)
			{
				if(newquery)
					str.SetAs(newquery);
				else
					str.SetAs(query);
				if(ReadOnlyQueryAllow(&str) == wyFalse)
					return IN_READONLY;				
			}
			if(wnd->m_ptransaction && (wnd->m_ptransaction->m_starttransactionenabled == wyFalse))
			{
				if(newquery)
					str.SetAs(newquery);
				else
					str.SetAs(query);
				transactioncheck = wnd->m_ptransaction->TransactionContinue(&str, fortransactionprompt);
			}
	//	}
		if(transactioncheck == 0) //dosent matter for professional, as initial value of transactioncheck is != 0
			return IN_TRANSACTION;
		#endif
		//Its needed for encoding scheme with HTTP tunneling only
		if(tunnel && tunnel->IsTunnel() == true)
			isbadforxml = IsBadforXML(query);
		
		if(wnd)
			wnd->m_execstarttime = timetaken = GetHighPrecisionTickCount(); //execution starting time
		
		ret  = HandleMySQLRealQuery(tunnel,*mysql, query, length, isbadforxml,
			batch, isend, (bool*)stop, isread ? true : false, fksethttpimport == wyTrue ? true:false);
				
		tsrecon = GetHighPrecisionTickCount();

		if(wnd)
		{
			wnd->m_execendtime = tsrecon;//execution ending time
			timetaken = wnd->m_execendtime - timetaken; //execution time
			wnd->m_lastquerytime = timetaken;
		}
		
		//Re-instatiating the SSH session 
		if(!tunnel->IsTunnel() && wnd && isimport == wyFalse && currentwnd == wyTrue &&
			ret &&((tunnel->mysql_errno(*mysql)== MYSQL_CONNECT_ERROR) ||
			(tunnel->mysql_errno(*mysql)== NO_DB_SELECTED) ||
			(tunnel->mysql_errno(*mysql)== MYSQL_SERVER_GONE) ||
             tunnel->mysql_errno(*mysql) == LOST_CONNECTION_ERROR ||
				tunnel->mysql_errno(*mysql) == CR_CONNECTION_ERROR)&& querycount == 0 && (!stop || !(*stop)))
		{	
			wnd->m_isreconnected = wyFalse; 
			if(wnd->ReConnect(tunnel, mysql, wnd->m_conninfo.m_isssh, isimport, profile) == wyTrue) 
			{
				wnd->m_isreconnected = wyTrue;

				if(profile == wyTrue)
				{
					//total time elapsed for reconnecting and execution(failed execution)
					tsrecon = timetaken + (GetHighPrecisionTickCount() - tsrecon);

					#ifndef COMMUNITY

						if(pGlobals->m_entlicense.CompareI("Professional") != 0 && wnd->m_ptransaction && (wnd->m_ptransaction->m_starttransactionenabled == wyFalse))
							recstr.SetAs(_("  SQLyog reconnected. The transaction has been rolled back due to connection error."));
						else
					#endif
					recstr.SetAs(_("  SQLyog reconnected"));
					//profile comment to history
					my_queryprofile(wnd, tsrecon, recstr.GetString(), wyTrue); 
				}
#ifndef COMMUNITY
				if(pGlobals->m_entlicense.CompareI("Professional") != 0)
				{
					if(wnd->m_ptransaction)
					{
						wnd->m_ptransaction->m_autocommit =			wyTrue;
						wnd->m_ptransaction->m_isolationmode =		ID_TRX_REPEATABLEREAD;
					}
					if(wnd->m_ptransaction && (wnd->m_ptransaction->m_starttransactionenabled == wyFalse))
					{
						wnd->m_ptransaction->CallOnCommit();
						presult = MessageBox(wnd->GetHwnd() , _(L"SQLyog reconnected. The transaction has been rolled back due to connection error."), 
						   _(L"Warning"), MB_ICONWARNING | MB_OK | MB_DEFBUTTON2);
					}
				}
#endif
				/* try to reexecute the query */	
					return my_query(wnd, tunnel, mysql, query, length, batch, isend, stop, 1, profile);
				
			}

			if(profile == wyTrue)//profile the history tab with comment'SQLyog reconnect failed'
			{				
				//total time elapsed for reconnecting and execution(failed execution)
				tsrecon = timetaken + (GetHighPrecisionTickCount() - tsrecon);
				#ifndef COMMUNITY
					if(pGlobals->m_entlicense.CompareI("Professional") != 0 && wnd->m_ptransaction)
					{
						wnd->m_ptransaction->m_autocommit =			wyTrue;
						wnd->m_ptransaction->m_isolationmode =		ID_TRX_REPEATABLEREAD;
					}
					if(pGlobals->m_entlicense.CompareI("Professional") != 0 && wnd->m_ptransaction && (wnd->m_ptransaction->m_starttransactionenabled == wyFalse))
					{
						//wnd->m_ptransaction->CallOnCommit();
						recstr.SetAs(_("  SQLyog reconnect failed. The Transaction has been rolled back due to connection error."));
					}
					else
				#endif
				recstr.SetAs(_("  SQLyog reconnect failed"));
				
				//profile comment to history
				my_queryprofile(wnd, tsrecon, recstr.GetString(), wyTrue);  
			}	
#ifndef COMMUNITY
				if(pGlobals->m_entlicense.CompareI("Professional") != 0)
				{
					if(wnd->m_ptransaction)
					{
						wnd->m_ptransaction->m_autocommit =			wyTrue;
						wnd->m_ptransaction->m_isolationmode =		ID_TRX_REPEATABLEREAD;
					}
					if(wnd->m_ptransaction && (wnd->m_ptransaction->m_starttransactionenabled == wyFalse))
					{
						presult = MessageBox(wnd->GetHwnd() , _(L"SQLyog reconnect Failed. The Transaction has been rolled back due to connection error."), 
						   _(L"Warning"), MB_ICONWARNING | MB_OK | MB_DEFBUTTON2);
						wnd->m_ptransaction->CallOnCommit();
					}
				}
#endif
		}
		#ifndef COMMUNITY
		if(pGlobals->m_entlicense.CompareI("Professional") != 0 && !ret && wnd->m_ptransaction)
		{
			if(transactioncheck != -1 && !wnd->m_ptransaction->m_starttransactionenabled && wnd->m_ptransaction->m_autocommit)
			{
					wnd->m_ptransaction->CallOnCommit();
					if(wnd->m_ptransaction->m_implicitcommit)
					{
						wnd->m_ptransaction->m_implicitcommit = wyFalse;
						recstr.SetAs(_("An implicit commit occurred"));
						my_queryprofile(wnd, 1, recstr.GetString(), wyTrue);
					}
			}		
		}
		#endif
		
		/* now we add the query for profiling */
		/* if its tunneling then we dont add the profiling as it will be wrong */
		/* we get the profiling timings from the XML from HTTP */
		/* starting from 4.07 RC2 we only add queries of length 4KB and less */
		if(wnd && !tunnel->IsTunnel() && profile == wyTrue && length < MAX_QUERY_SIZE)
		{         
			//History profiling 
			my_queryprofile(wnd, timetaken, newquery + pos, wyFalse, tunnel->mysql_errno(*mysql), tunnel->mysql_error(*mysql));
		}

       if(!ret && newquery && wnd && currentwnd == wyTrue && wnd->m_grpprocess == wyFalse && !tunnel->IsTunnel())
       {
          unFormattedQuery = _strdup(query);
          if(unFormattedQuery)
          {
            pGlobals->m_pcmainwin->m_connection->HandlerAddToMessageQueue(wnd, unFormattedQuery);
            free(unFormattedQuery);
          }
          
       }

		free(newquery);
	}
	
	return ret;
}

// query profiling to history tab
wyBool
my_queryprofile(MDIWindow *wnd, wyInt64 timetaken, const wyChar *proftext, wyBool iscomment, wyInt32 errnum, const wyChar *errormsg)
{
	TabTypes    *tab = NULL;
	TabHistory	*ptabhistory;
	wyWChar     systime[SIZE_256];
	wyString	systimestr, str, tempstr;
	SYSTEMTIME	localtime ={0};
	wyChar *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	
	if(!wnd || !proftext)
		return wyFalse;

	ptabhistory = wnd->GetActiveHistoryTab();
	tab = wnd->GetActiveTabEditor();

	memset(systime, 0, sizeof(systime));

	GetTimeFormat(LOCALE_USER_DEFAULT, 0, NULL, NULL, systime, ((SIZE_256-1) * 2));
	systimestr.SetAs(systime);
	
	GetLocalTime(&localtime);

	if(iscomment == wyTrue)// set the history log text as comment
		str.Sprintf("/*[%d-%s %s][%I64d ms] %s */\r\n", localtime.wDay, months[localtime.wMonth - 1],systimestr.GetString(), timetaken / 1000, proftext);

	else // to add query
	{
		tempstr.SetAs(proftext);
		tempstr.RTrim();
		if (errnum > 0)
		{
			if (tempstr.GetLength() && tempstr.GetString()[tempstr.GetLength() - 1] != ';')
				str.Sprintf("/*[%d-%s %s][%I64d ms]*/ %s; /*[Error Code %d: %s]*/\r\n", localtime.wDay, months[localtime.wMonth - 1], systimestr.GetString(), timetaken / 1000, proftext, errnum, errormsg);
			else
				str.Sprintf("/*[%d-%s %s][%I64d ms]*/ %s /*[Error Code %d: %s]*/\r\n", localtime.wDay, months[localtime.wMonth - 1], systimestr.GetString(), timetaken / 1000, proftext, errnum, errormsg);
		}
		else
		{
			if (tempstr.GetLength() && tempstr.GetString()[tempstr.GetLength() - 1] != ';')
				str.Sprintf("/*[%d-%s %s][%I64d ms]*/ %s;\r\n", localtime.wDay, months[localtime.wMonth - 1], systimestr.GetString(), timetaken / 1000, proftext);
			else
				str.Sprintf("/*[%d-%s %s][%I64d ms]*/ %s\r\n", localtime.wDay, months[localtime.wMonth - 1], systimestr.GetString(), timetaken / 1000, proftext);
		}
	}

	//Add text to history tab
	if(ptabhistory)
	{
		FormatQueryByRemovingSpaceAndNewline(&str);			
		ptabhistory->AddText(str);
	}
    	
#ifndef COMMUNITY
	else
	{
		//Handling the history for Schema Designer					
		if(wnd->m_pctabmodule && (wnd->m_pctabmodule->GetActiveTabImage() == IDI_SCHEMADESIGNER_16))
		{
			TabSchemaDesigner * tschmadesigner = (TabSchemaDesigner*)wnd->m_pctabmodule->GetActiveTabType();

			if(tschmadesigner->m_history.GetLength() > MAX_HISTORY_SIZE)
				tschmadesigner->m_history.Clear();

			tschmadesigner->m_history.Add(str.GetString());
		}	
	}
	
#endif

	return wyTrue;    
}

wyChar*
FormatQuery(const wyChar *query, wyInt32 &position)
{
	wyString	newquerystr;
	wyWChar		*tempwchar = 0;
	wyChar		*newquery = NULL, *tempquery;
	wyInt32		pos, counter;

	if(!query)
		return NULL;

	VERIFY(newquery = _strdup(query));

	if(!newquery)
		return NULL;

	pos = 0;
	tempquery = AllocateBuff(strlen(newquery) + 1);
	strcpy(tempquery, newquery);
	newquerystr.SetAs(tempquery);

	strcpy(newquery, newquerystr.GetString());
	// now we change all 13 to 10.
	for(counter=0; newquery[counter]; counter++)
    {
		if(newquery[counter]== 13 || newquery[counter]== 10 || newquery[counter]== C_TAB)
		{
			newquery[counter]= ' ';
		}
	}

	newquerystr.SetAs(newquery);
	tempwchar = (wyWChar *)newquerystr.GetAsWideChar();

	while(tempwchar && iswspace(*(tempwchar+pos)))
	{
		pos++;
	}

	position = pos;
	free(tempquery);
	return newquery;
}

PMYSQL
GetActiveMySQLPtr()
{
	HWND		hwndmdi;
	MDIWindow	*pcquerywnd;

	if(!pGlobals->m_hwndclient)
		return NULL;

	hwndmdi	=(HWND)SendMessage(pGlobals->m_hwndclient, WM_MDIGETACTIVE, 0, 0);

	if(!hwndmdi)
		return NULL;

	pcquerywnd  =(MDIWindow*)GetWindowLongPtr(hwndmdi, GWLP_USERDATA);

	return &pcquerywnd->m_mysql;
}

LPSTR
MySqlEscape(LPCSTR pstr, DWORD size, wyUInt32 * newsize, 
				wyChar esc, wyBool isfterm, wyChar fterm, wyBool isfenc, wyChar fenc, wyBool islterm, wyChar lterm, wyBool isescaped, wyBool isescdoublequotes)
{
    DWORD   count = 0, rcount = 0;
	LPSTR	ret = 0;
	wyChar  c;

    ret =(LPSTR)calloc(sizeof(wyChar),(size * 2)+ 1);

    while(count < size)
	{
		c = *(pstr+count);
	
		//if field terminated character is coming inside the field then no need to escape that character
		if((isescaped == wyTrue) && //(isfterm &&(c == fterm))|| 
			(isfenc &&(c == fenc))|| 
			(islterm &&(c == lterm))|| 
			(c == esc)){
			if((c != '"') || ((c != fenc) || (isescdoublequotes == wyFalse)))
			{
			ret[rcount++]= esc;
			ret[rcount++]= c;
			}
			else
			{
				ret[rcount++]= '"';
				ret[rcount++]= c;
			}
		} else if(c == 0){
			ret[rcount++]= esc;
			ret[rcount++]= '0';
		} 
		  else
			ret[rcount++]= c;

		count++;
    }
	*newsize = rcount;
    return ret;
}

wyBool
IsNumber(enum_field_types t)
{
	switch(t)
	{
	case FIELD_TYPE_DECIMAL:
	case FIELD_TYPE_TINY:
	case FIELD_TYPE_SHORT:
	case FIELD_TYPE_LONG:
	case FIELD_TYPE_FLOAT:
	case FIELD_TYPE_DOUBLE:
	case FIELD_TYPE_NULL:
	case FIELD_TYPE_LONGLONG:
	case FIELD_TYPE_INT24:
		return wyTrue;

	default:
		return wyFalse;
	}

}

wyBool
VerifyMemory(LPSTR* lpstr, wyUInt32* ptotalsize, wyUInt32 nsize, wyUInt32 bytestobeadded)
{
    LPSTR ptemp;

    if((nsize + bytestobeadded) * sizeof(wyChar) > *ptotalsize - 1)
    {
        //reallocate the memory
        *ptotalsize += nsize + bytestobeadded + 1;
        ptemp = (wyChar*)HeapReAlloc(GetProcessHeap(), 0, (LPVOID)*lpstr, *ptotalsize);

        //check for allocation failure
        if(ptemp == NULL)
        {
            return wyFalse;
        }

        *lpstr = ptemp;        
    }

    return wyTrue;
}


//Function writes a formated text on a text buffer and return the pointer.																				
//Function writes a formated text on a text buffer and return the pointer.																				
wyChar * 
    FormatResultSet(Tunnel * tunnel, MYSQL_RES * myres, MYSQL_FIELD * myfield, MySQLDataEx *mdata)
{
	wyUInt32    count, ncounter = 0, *pfieldlenarr, nfieldlen = 0, nmaxlen = 0;
	wyInt32     dwtitlecount, dwcurrline = 1,dwmaxline = 1, dwlinenum, dwlinenumtemp;
    wyUInt32    dwcharpoint; 
    wyInt32     dwnewlinepoint, dwheapsize, dwlast=0, dwsize;
	wyChar      sznull[] = STRING_NULL;
	wyChar      *result;
	MYSQL_ROW	myrow;
	wyString	myfieldstr, myfieldnamestr;
	wyBool		ismysql41 = ((GetActiveWin())->m_ismysql41);
	wyInt32		countmdata(0), rowcounter = 0;;
	wyChar		charlast; 
	wyBool		ismultiline = wyFalse;
    wyUInt32    tempcount1, totalsize;
	wyChar		*myrowdata;// row data
	wyBool      isbit = wyFalse;//bit field flag
	wyString    bitstr;// string coverted bit field
	wyInt32     bitlen;// length of the data in case of bit field type
    //Varible used for aligning the non ansi
	wyString	coldata, mutlilinestr;
	wyUInt32	extrapad = 0, len = 0, totalpad = 0;
	wyBool		isfirstline = wyTrue; //For multi-line(blob) consider only 1st line
	wyInt32		tempcount = 0;

	// Get the row count of m_data.
	
	if(mdata)
	{
        tempcount = mdata->m_rowarray->GetLength();
        tempcount =  tempcount + (tempcount && mdata->m_rowarray->GetRowExAt(tempcount - 1)->IsNewRow() ? -1 : 0);
        countmdata = tempcount;
	}


	dwtitlecount = GetTitleCount(myres, myfield);

	// Heap Allocation.
	if(mdata)
		VERIFY(result =(wyChar*)HeapAlloc(GetProcessHeap(), 0,((dwtitlecount*((unsigned long)countmdata+ 1 + 3)+5))));
	else
		VERIFY(result =(wyChar*)HeapAlloc(GetProcessHeap(), 0,((dwtitlecount*((unsigned long)myres->row_count + 1 + 3)+5))));

	FillMemory(result,(totalsize = (HeapSize(GetProcessHeap(), 0, result))),(BYTE)' ');

    // Allocate memory to store length values of each column.
	// We store for each column either the column length width or max-length in that field
	// which ever is greater. This way we know where to put the text in the editbox.
	VERIFY(pfieldlenarr	=	new wyUInt32[myres->field_count + 1]);
	
	// now get the length. each array field will have length with respect to the previous one.
	for(count = 0; count <= myres->field_count; count++)
	{
		if(count == 0)
			pfieldlenarr[count]	= 0;
		else
		{
			myfieldstr.SetAs(myfield[count - 1].name, ismysql41);
			nfieldlen = myfieldstr.GetLength();
		
			if(IS_BLOBVALUE(myfield[count - 1].type))
				nmaxlen	  = (myfield[count - 1].max_length > 384) ? (384) : (myfield[count - 1].max_length);
			else
			{
				if(myfield[count - 1].type == MYSQL_TYPE_BIT)//for the BIT field
				{
					nmaxlen = myfield[count - 1].length + 3;//additional bcoz of "b''"
				}
				else
				{
					nmaxlen	  = myfield[count - 1].max_length;
				}
			}
			
			if(nfieldlen < 6 && nmaxlen < 6)
				pfieldlenarr[count]	= pfieldlenarr[count - 1] + 8;
			else 
				pfieldlenarr[count]	= pfieldlenarr[count - 1] + ((nfieldlen > nmaxlen) ? (nfieldlen) : (nmaxlen)) +  2;
		}
	}

	/// add the title.
	for(ncounter = 0; ncounter < myres->field_count; ncounter++)
	{
		if(IS_NUM(myfield[ncounter].type))
		{
			INT	len;

            len = _scprintf("%*s", pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter] - 2, myfield[ncounter].name);
            VerifyMemory(&result, &totalsize, pfieldlenarr[ncounter], len);
            len = sprintf(result + pfieldlenarr[ncounter], "%*s", pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter] - 2, myfield[ncounter].name);
			VerifyMemory(&result, &totalsize, pfieldlenarr[ncounter] + len, 2);
			memset(result + pfieldlenarr[ncounter] + len, ' ', 2);
			
		} 
		else 
		{
            INT len = strlen(myfield[ncounter].name);

            VerifyMemory(&result, &totalsize, pfieldlenarr[ncounter], len);
			memcpy(result + pfieldlenarr[ncounter], myfield[ncounter].name, len);
		}

	}

    VerifyMemory(&result, &totalsize, pfieldlenarr[ncounter], strlen("\r\n"));
	sprintf(result + pfieldlenarr[ncounter], "\r\n");

	// add the dash title.
	for(ncounter = 0; ncounter < myres->field_count; ncounter++)
	{
		if(!((ncounter + 1) == myres->field_count))
        {
            VerifyMemory(&result, &totalsize, dwtitlecount + pfieldlenarr[ncounter], (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]) - 2);
			memset(result + dwtitlecount + pfieldlenarr[ncounter], '-', (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]) - 2);
        }
		else
        {
            VerifyMemory(&result, &totalsize, dwtitlecount + pfieldlenarr[ncounter], (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]));
			memset(result + dwtitlecount + pfieldlenarr[ncounter], '-',(pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]));
        }
	}

    VerifyMemory(&result, &totalsize, dwtitlecount + pfieldlenarr[ncounter], strlen("\r\n"));
	sprintf(result + dwtitlecount + pfieldlenarr[ncounter], "\r\n");

	// This is where the complexity increases. If the field is variable length field then
	// you have to pad it up and format it.

	tunnel->mysql_data_seek(myres, 0);
	dwlinenum		=	2;
	dwlinenumtemp	=	2;

	
	//First row
	if(mdata)
	{
        if(mdata->m_rowarray->GetRowExAt(0) != NULL && mdata->m_rowarray->GetRowExAt(0)->IsNewRow() == wyFalse)
		{
            myrow = mdata->m_rowarray->GetRowExAt(0)->m_row;
		}
		else
		{
			myrow = NULL;
		}
		
	}
	else
		myrow = tunnel->mysql_fetch_row(myres);

	while(myrow)
	{
		rowcounter ++;
		dwlinenum	=	++dwcurrline;
		
		//2 added to avoid garbage characters
        VerifyMemory(&result, &totalsize, (dwtitlecount*dwlinenum) + totalpad, dwtitlecount + totalpad + 2);
		memset(result + (dwtitlecount*dwlinenum) + totalpad, ' ', dwtitlecount + totalpad + 2);
		for(ncounter = 0; ncounter < myres->field_count; ncounter++)
		{
			isfirstline = wyTrue;
			extrapad = 0;
			isbit = wyFalse;
			bitstr.Clear();

			ismultiline = wyFalse;
			//converted string for bit
			
			if(myfield[ncounter].type == MYSQL_TYPE_BIT)//checking for the BIT type field.
				isbit = wyTrue;
				
			/*setting the myrowdata...  
			if its a BIT field -> then convert mysql_row data into string and use it ;
			else -> use the mysql_row data*/
			if(isbit)
			{
				GetUtf8ColLength(myrow, myres->field_count, ncounter, (wyUInt32*)&bitlen);
				ConvertStringToBinary(myrow[ncounter], bitlen, pfieldlenarr[ncounter + 1], &bitstr);
				myrowdata = (wyChar *)bitstr.GetString();
			}
			else
				myrowdata = myrow[ncounter];

			if(IS_NUM(myfield[ncounter].type))
			{
				dwlinenumtemp = dwlinenum;

				if(myrowdata == NULL)
				{
					if(ncounter == (myres->field_count - 1))
					{
                        charlast = result[(dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad + (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter])];
                        tempcount1 = _scprintf("%*s", (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]), STRING_NULL);
						VerifyMemory(&result, &totalsize, (dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad, tempcount1);
                        sprintf(result + (dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad, "%*s", (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]), STRING_NULL);
                        VerifyMemory(&result, &totalsize, (dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad + (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]));
						result[(dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad + (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter])] = charlast;
					}
					else
					{
						charlast = result[(dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad + (pfieldlenarr[ncounter+1] - pfieldlenarr[ncounter]) - 2];
                        tempcount1 = _scprintf("%*s", (pfieldlenarr[ncounter+1] - pfieldlenarr[ncounter]) - 2, STRING_NULL);
						VerifyMemory(&result, &totalsize, (dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad, tempcount1);
                        sprintf(result + (dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad, "%*s", (pfieldlenarr[ncounter+1] - pfieldlenarr[ncounter]) - 2, STRING_NULL);
						VerifyMemory(&result, &totalsize, (dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad + (pfieldlenarr[ncounter+1] - pfieldlenarr[ncounter]) - 2);
                        result[(dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad + (pfieldlenarr[ncounter+1] - pfieldlenarr[ncounter]) - 2] = charlast;
					}
					
					//no need to add space for last column.issue reported here http://code.google.com/p/sqlyog/issues/detail?id=689
					if(ncounter != (myres->field_count - 1))
                    {
                        VerifyMemory(&result, &totalsize, ((dwtitlecount * dwlinenum) + pfieldlenarr[ncounter+1]) + totalpad - 2, 2);
						memset(result + ((dwtitlecount * dwlinenum) + pfieldlenarr[ncounter+1]) + totalpad - 2, ' ', 2);
                    }
				} 
				else 
				{					
					if(ncounter == (myres->field_count - 1))
					{
                        charlast = result[(dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad + (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter])];
						//charlast = result[(dwtitlecount * dwlinenum) + pfieldlenarr[ncounter + 1] + totalpad];
						
                        //-----------
                        tempcount1 = _scprintf("%*s", (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]), myrowdata);
						VerifyMemory(&result, &totalsize, (dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad, tempcount1);
                        //-----------

                        sprintf(result + (dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad, "%*s", (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]), myrowdata);
						//memset(result + (dwtitlecount * dwlinenum) + pfieldlenarr[ncounter + 1] + totalpad, '\r', 1);
                        
                        //-----------
                        VerifyMemory(&result, &totalsize, (dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad + (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]));
                        //-----------

						result[(dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad + (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter])] = charlast;
					}
					else
					{
						charlast = result[(dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad + (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]) - 2];
						
                        //-----------
                        tempcount1 = _scprintf("%*s", (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]) - 2, myrowdata);
						VerifyMemory(&result, &totalsize, (dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad, tempcount1);
                        //-----------

                        sprintf(result + (dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad, "%*s", (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]) - 2, myrowdata);

						//-----------
                        VerifyMemory(&result, &totalsize, (dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad + (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]) - 2);
                        //-----------

                        result[(dwtitlecount * dwlinenum) + pfieldlenarr[ncounter] + totalpad + (pfieldlenarr[ncounter + 1] - pfieldlenarr[ncounter]) - 2] = charlast;
					}

					if(ncounter != (myres->field_count - 1))
                    {
                        VerifyMemory(&result, &totalsize, ((dwtitlecount * dwlinenum) + pfieldlenarr[ncounter + 1]) + totalpad - 2, 2);
						memset(result + ((dwtitlecount * dwlinenum) + pfieldlenarr[ncounter + 1]) + totalpad - 2, ' ', 2);
                    }
				}
			}
			else
			{
				dwlinenumtemp		=	dwlinenum;
				// check for null.
				if(myrowdata == NULL)
				{
					for(count = 0; count < 6; count++)
						*(result+(dwlinenumtemp * dwtitlecount) + pfieldlenarr[ncounter] + count + totalpad) = sznull[count];

                    if(ncounter == (myres->field_count - 1))
                        dwlast += pfieldlenarr[ncounter + 1] + 2;

					if(ncounter == (myres->field_count - 1))
					{
                        VerifyMemory(&result, &totalsize, (dwtitlecount * dwlinenumtemp) + pfieldlenarr[myres->field_count] + totalpad, 2);
						*(result + (dwtitlecount * dwlinenumtemp) + pfieldlenarr[myres->field_count] + totalpad) = '\r';
						*(result + (dwtitlecount * dwlinenumtemp) + pfieldlenarr[myres->field_count] + 1 + totalpad) = '\n';
					}
					continue;
				}

				ismultiline = wyFalse;
				
				// pad it up. well count am rewriting this code for the fifth time.
				for(dwcharpoint = 0, dwnewlinepoint = 0; dwcharpoint < pfieldlenarr[ncounter+1] && 
                                                    myrowdata[dwcharpoint] != NULL; dwcharpoint++)
                {
					if(!(myrowdata[dwcharpoint] == '\r' || myrowdata[dwcharpoint] == '\n'))
                    {
                        VerifyMemory(&result, &totalsize, (dwlinenumtemp * dwtitlecount) + pfieldlenarr[ncounter] + dwnewlinepoint + totalpad);
						*(result + (dwlinenumtemp * dwtitlecount) + pfieldlenarr[ncounter] + dwnewlinepoint + totalpad) = myrowdata[dwcharpoint];
						dwnewlinepoint++;
						continue;
					} 
                    else  
                    {
						//Consider the 1st line only for padding the spaces
						if(isfirstline == wyTrue)
						{
							mutlilinestr.SetAsDirect(myrowdata, dwnewlinepoint);
							isfirstline = wyFalse;
						}
						
						// Get the previous heap size.
						dwheapsize	=	HeapSize(GetProcessHeap(), 0,(LPVOID)result);
						
						// now since we have found one more line we allocate line for one more line.
						result	=	(wyChar*)HeapReAlloc(GetProcessHeap(), 0, (LPVOID)result, dwheapsize + dwtitlecount + extrapad + totalpad);

                        VerifyMemory(&result, &totalsize, (dwtitlecount * dwlinenumtemp) + pfieldlenarr[myres->field_count] + totalpad, 2);
						*(result + (dwtitlecount * dwlinenumtemp) + pfieldlenarr[myres->field_count] + totalpad + extrapad) = '\r';
						*(result + (dwtitlecount * dwlinenumtemp) + pfieldlenarr[myres->field_count] + 1 + totalpad + extrapad)= '\n';

						// now change the initial poition.
						dwlinenumtemp++;
						dwnewlinepoint	=	0;
				
						// first pad up everything in the new line to spaces.
						if(ncounter==0 || dwmaxline < dwlinenumtemp )
                        {
							VerifyMemory(&result, &totalsize, (dwlinenumtemp * dwtitlecount) + extrapad + totalpad, dwtitlecount);
							memset(result + (dwlinenumtemp * dwtitlecount) + extrapad + totalpad, ' ', dwtitlecount);
							dwmaxline = dwlinenumtemp;
						}
						dwcurrline	=	dwlinenumtemp;

						// Now check if there is another \r or \n then we take it as one only.
						// SQLyog takes \r, \n, \r\n, \n\r as one line.
						if(myrowdata[dwcharpoint+1] == '\r' || myrowdata[dwcharpoint + 1] == '\n')
							dwcharpoint++;

						ismultiline = wyTrue;						

						continue;
					}
				}
				
				/*Find the numer of space to padd to handle the non-ansi data.
				pad space = number-of-bytes - number-of-characters
				*/
				extrapad = 0;
				if(myrowdata && (ncounter < (myres->field_count - 1)))
				{
					//IF multi-line data present we need to consider the 1st line only
					if(isfirstline == wyTrue)
					{
						coldata.SetAs(myrowdata, ismysql41);
					    len = coldata.GetLength();
					    extrapad = len - wcslen(coldata.GetAsWideChar());

					    //Re-allocate the buffer size for padding spaces
					    if(extrapad)
					    {
						    totalpad += extrapad;
						    dwheapsize	=	HeapSize(GetProcessHeap(), 0,(LPVOID)result);
						    result = (wyChar*)HeapReAlloc(GetProcessHeap(), 0, (LPVOID)result, dwheapsize + extrapad);
					    }					
					}
				}
			}

			if(ncounter == (myres->field_count - 1) || ismultiline == wyTrue)
			{
                VerifyMemory(&result, &totalsize, (dwtitlecount * dwcurrline) + pfieldlenarr[myres->field_count] + totalpad - extrapad, 2);
				*(result + (dwtitlecount * dwcurrline) + pfieldlenarr[myres->field_count] + totalpad - extrapad) = '\r';
				*(result + (dwtitlecount * dwcurrline) + pfieldlenarr[myres->field_count] + 1 + totalpad - extrapad) = '\n';
			}

			if(ismultiline == wyTrue)
				dwlast = (dwtitlecount * dwmaxline) + pfieldlenarr[myres->field_count] + 1 + totalpad - extrapad;

			else
				dwlast = (dwtitlecount * dwcurrline) + pfieldlenarr[myres->field_count] + 1 + totalpad - extrapad;
			isfirstline = wyTrue;
		}
				
		if(mdata)
		{
			if(rowcounter >= countmdata)
				break;

            myrow = mdata->m_rowarray->GetRowExAt(rowcounter)->m_row;
		}
		else
			myrow = tunnel->mysql_fetch_row(myres);
	}//while

	// now strip of the last characters some garbage are there.
	dwsize = HeapSize(GetProcessHeap(), 0, result);
	if(dwlast && dwlast < dwsize)
	{
		for(count = dwlast; count < dwsize; count++)
			result[count-1] = NULL;
	}

	delete[]pfieldlenarr;
	
	return result;
}

/*
	Special function to format create table resultset, unlike FormatResultSet, this function is very specific to
	create table statement and does not create unwanted whitespace for the big create table function
																				*/
wyChar * 
FormatCreateTableResultSet(Tunnel * tunnel, MYSQL_RES * myres, MYSQL_FIELD * myfield)
{
	wyUInt32            count, ncounter = 0, *pfieldlenarr, nfieldlen = 0, nmaxlen = 0;
	wyUInt32            dwtitlecount, dwcurrline = 1, dwlinenum, dwlinenumtemp, dwcharpoint, dwnewlinepoint, dwheapsize, dwlast=0, dwsize;
	wyChar              *result;
	MYSQL_ROW			myrow;
	
    dwtitlecount = GetCreateTableTitleCount(tunnel, myres, myfield);

	VERIFY(result =(wyChar*)HeapAlloc(GetProcessHeap(), 0,((dwtitlecount*((unsigned long)myres->row_count+3)+5))));

	FillMemory(result,(HeapSize(GetProcessHeap(), 0, result)),(BYTE)' ');
	
	// Allocate memory to store length values of each column.
	// We store for each column either the column length width or max-length in that field
	// which ever is greater. This way we know where to put the text in the editbox.
	VERIFY(pfieldlenarr	=	new wyUInt32[myres->field_count+1]);
	
	// now get the length. each array field will have length with respect to the previous one.
	for(count=0; count<=(myres->field_count+1)&&count<3; count++)
	{
		switch(count)
		{
			case 0:
				pfieldlenarr[count]	= 0;
				break;

			case 1:
				nfieldlen = strlen(myfield[count-1].name);
				nmaxlen	  = myfield[count-1].max_length;

				if(nfieldlen < 6 && nmaxlen < 6)
					pfieldlenarr[count]	= pfieldlenarr[count-1]+8;
				else 
					pfieldlenarr[count]	= pfieldlenarr[count-1]+((nfieldlen>nmaxlen)?(nfieldlen):(nmaxlen))+ 2;
				break;

			case 2:
				pfieldlenarr[count]	= pfieldlenarr[count-1]+ GetMaxLineCount(tunnel, myres);
				break;
		}
	}

	// add the title.
	for(ncounter = 0; ncounter < myres->field_count && ncounter < 2; ncounter++)
	{

		if(IS_NUM(myfield[ncounter].type))
        {
			wyInt32 len;
			len = sprintf(result+pfieldlenarr[ncounter], "%*s", pfieldlenarr[ncounter+1]-pfieldlenarr[ncounter]-2, myfield[ncounter].name);
			memset(result+pfieldlenarr[ncounter]+len, ' ', 2);
		} else 
        {
			memcpy(result+pfieldlenarr[ncounter], myfield[ncounter].name, strlen(myfield[ncounter].name));
		}
	}

	sprintf(result+pfieldlenarr[ncounter], "\r\n");

	// add the dash title.
	for(ncounter=0; ncounter < myres->field_count && ncounter < 2; ncounter++)
	{
		if(!((ncounter+1)== myres->field_count))
			memset(result+dwtitlecount+pfieldlenarr[ncounter], '-',(pfieldlenarr[ncounter+1]-pfieldlenarr[ncounter])-2);
		else
			memset(result+dwtitlecount+pfieldlenarr[ncounter], '-',(pfieldlenarr[ncounter+1]-pfieldlenarr[ncounter]));
	}
	sprintf(result+dwtitlecount+pfieldlenarr[ncounter], "\r\n");

	// This is where the complexity increases. If the field is variable length field then
	// you have to pad it up and format it.

	tunnel->mysql_data_seek(myres, 0);
	dwlinenum		=	2;
	dwlinenumtemp	=	2;
	while((myrow = tunnel->mysql_fetch_row(myres)))
	{
		dwlinenum	=	++dwcurrline;
        memset(result+(dwtitlecount*dwlinenum), ' ', dwtitlecount);

		for(ncounter=0; ncounter < myres->field_count  && ncounter < 2; ncounter++)
		{
			if(IS_NUM(myfield[ncounter].type))
			{
				dwlinenumtemp = dwlinenum;

				if(myrow[ncounter]== NULL){
					sprintf(result+(dwtitlecount*dwlinenum)+pfieldlenarr[ncounter], "%*s",(pfieldlenarr[ncounter+1]-pfieldlenarr[ncounter])-2, STRING_NULL);
					memset(result+((dwtitlecount*dwlinenum)+pfieldlenarr[ncounter+1])-2, ' ', 2);
				} 
				else 
				{
					if(ncounter ==(myres->field_count-1))
						sprintf(result+(dwtitlecount*dwlinenum)+pfieldlenarr[ncounter], "%*s",(pfieldlenarr[ncounter+1]-pfieldlenarr[ncounter]), myrow[ncounter]);
					else
						sprintf(result+(dwtitlecount*dwlinenum)+pfieldlenarr[ncounter], "%*s",(pfieldlenarr[ncounter+1]-pfieldlenarr[ncounter])-2, myrow[ncounter]);

					if(ncounter !=(myres->field_count-1))
						memset(result+((dwtitlecount*dwlinenum)+pfieldlenarr[ncounter+1])-2, ' ', 2);
				}
			}
			else
			{
				dwlinenumtemp		=	dwlinenum;

				// pad it up. well count am rewriting this code for the fifth time.
				for(dwcharpoint = 0, dwnewlinepoint=0; myrow[ncounter][dwcharpoint]!= NULL; dwcharpoint++){
					
					if(!(myrow[ncounter][dwcharpoint]== '\r' || myrow[ncounter][dwcharpoint]== '\n'))	{
						*(result+(dwlinenumtemp*dwtitlecount)+pfieldlenarr[ncounter]+dwnewlinepoint)= myrow[ncounter][dwcharpoint];
						dwnewlinepoint++;
						continue;
					} else {
						// Get the previous heap size.
						dwheapsize = HeapSize(GetProcessHeap(), 0,(LPVOID)result);

						// now since we have found one more line we allocate line for one more line.
						result	=(wyChar*)HeapReAlloc(GetProcessHeap(), 0,(LPVOID)result, dwheapsize+dwtitlecount);

						*(result+(dwtitlecount*dwlinenumtemp)+pfieldlenarr[2])= '\r';
						*(result+(dwtitlecount*dwlinenumtemp)+pfieldlenarr[2]+1)= '\n';

						// now change the initial poition.
						dwlinenumtemp++;
						dwnewlinepoint	=	0;

						// first pad up everything in the new line to spaces.
						memset(result+(dwlinenumtemp*dwtitlecount), ' ', dwtitlecount);
						dwcurrline	=	dwlinenumtemp;

						// Now check if there is another \r or \n then we take it as one only.
						// SQLyog takes \r, \n, \r\n, \n\r as one line.
						if(myrow[ncounter][dwcharpoint+1]== '\r' || myrow[ncounter][dwcharpoint+1]== '\n')
							dwcharpoint++;
						continue;
					}
				}
			}

			*(result+(dwtitlecount*dwlinenumtemp)+pfieldlenarr[2])= '\r';
			*(result+(dwtitlecount*dwlinenumtemp)+pfieldlenarr[2]+1)= '\n';
			dwlast =(dwtitlecount*dwlinenumtemp)+pfieldlenarr[2]+1;
		}
	}

	// now strip of the last characters some garbage are there.
	dwsize = HeapSize(GetProcessHeap(), 0, result);
	if(dwlast && dwlast < dwsize)
	{
		for(count = dwlast; count<dwsize; count++)
			result[count - 1]= NULL;
	}

	delete[]pfieldlenarr;
	
	return result;
}

/* returns the max length of the longest line in create table statement */

wyUInt32  
GetMaxLineCount(Tunnel * tunnel, MYSQL_RES * myres)
{
	tunnel->mysql_data_seek(myres, 0);

	MYSQL_ROW		row;
	wyUInt32        maxlen=0;
	wyChar			seps[]= "\r\n";
	wyChar			*copy;
	wyChar			*token;

	row = tunnel->mysql_fetch_row(myres);

	if(!row || !row[1])
		return 0;

	copy =(wyChar*)calloc(sizeof(wyChar), strlen(row[1])+ 1);
	memcpy(copy, row[1], strlen(row[1]));

	token = strtok(copy, seps);
	while(token)
	{
		maxlen = max(maxlen, strlen(token));
	
		token = strtok(NULL, seps);
	}

	free(copy);

	tunnel->mysql_data_seek(myres, 0);

	return(maxlen+2);
}

/* specific function to get title length for create table query formatting to text */

wyUInt32 
GetCreateTableTitleCount(Tunnel * tunnel, MYSQL_RES * myres, MYSQL_FIELD * myfield)
{
	wyUInt32 ncounter = 0, nfieldlen = 0, nmaxlen = 0;
	wyUInt32 dwtemp = 0;

	// get for the first column and use GetMaxLineCount for second column
	for(ncounter=0; ncounter < 1; ncounter++)
	{
		nfieldlen = strlen(myfield[ncounter].name);

		if(IS_BLOBVALUE(myfield[ncounter].type))
			nmaxlen	  = (myfield[ncounter].max_length > 384) ? (384) : (myfield[ncounter].max_length);
		else 
			nmaxlen	  = myfield[ncounter].max_length;

		if(nfieldlen < 6 && nmaxlen < 6)
			dwtemp += 8;
		else 
			dwtemp += ((nfieldlen>nmaxlen)?(nfieldlen):(nmaxlen))+2;
	}

	dwtemp += GetMaxLineCount(tunnel, myres);

	return dwtemp+2;
}

wyUInt32 
GetTitleCount(MYSQL_RES * myres, MYSQL_FIELD * myfield)
{
	wyUInt32	ncounter = 0, nfieldlen=0, nmaxlen=0;
	wyUInt32	dwtemp=0;

	// now get the total count required for showing the title and dash.
	for(ncounter=0; ncounter < myres->field_count; ncounter++)
	{
		nfieldlen = strlen(myfield[ncounter].name);

		if(IS_BLOBVALUE(myfield[ncounter].type))
			nmaxlen	  =(myfield[ncounter].max_length>384)?(384):(myfield[ncounter].max_length);
		else if(myfield[ncounter].type == MYSQL_TYPE_BIT)//Use the length as the 
		{
			nmaxlen = myfield[ncounter].length + 3;//additional bcoz of "b''"
		}
		else
		{
			nmaxlen	  = myfield[ncounter].max_length;
		}

		if(nfieldlen < 6 && nmaxlen < 6)
			dwtemp += 8;
		else 
			dwtemp +=((nfieldlen>nmaxlen)?(nfieldlen):(nmaxlen))+2;
	}

	return (dwtemp + 2);//2 for new line "\r\n"
}

wyChar*
FormatReference(wyChar * key)
{
	wyBool	isbracket = wyFalse, istick = wyFalse;
	wyChar	*text, *text2, *temp;
	DWORD	len, count, k=0;
	wyChar	ref[]= "references";

	len = strlen(key);

	VERIFY(text =AllocateBuff(len+SIZE_64));
	VERIFY(text2 =AllocateBuff(len+SIZE_64));
	strcpy(text, key);

	// change space to ,
	for(count=0; text[count]; count++)
    {
		if(text[count]== C_OPEN_BRACKET)
			isbracket = wyTrue;
		else if(text[count]== C_SPACE)
        {
			if(isbracket)
				text[count]= C_COMMA;
		} 
        else if(text[count]== C_CLOSE_BRACKET)
        {
			isbracket = wyFalse;
		} 
        else if(text[count]== '`')
        {
            istick =(istick == wyTrue)?wyFalse:wyTrue;
		} 
        else if(text[count]== C_FRONT_SLASH && !istick)
			k = count;
	}

	if(k == 0)
		return NULL;

	strcpy(text2, text + k);

	VERIFY(temp = strstr(text, "REFER"));
	*temp = NULL;
	
	strcat(text, ref);
	strcat(text, " ");

	for(count=0; text2[count]; count++)
    {
		if(!count)
			text2[count]= '`';

		if(text2[count]== C_OPEN_BRACKET)
        {
			k = strlen(text2+count);
			memmove(text2+count+1, text2+count, k);
			text2[count]= '`';
			break;
		}
	}
	
	strcat(text, text2);
	free(text2);
	return text;
}

// fills the logfont structure with the defualt values.
void
FillEditFont(PLOGFONT	logf, HWND hwnd)
{
	HDC		dc;

    memset(logf, 0, sizeof(LOGFONT));
	VERIFY(dc = GetDC(hwnd));

	logf->lfHeight		= -MulDiv(8, GetDeviceCaps(dc, LOGPIXELSY), 72);
	logf->lfWeight		= FW_NORMAL;
	wcscpy(logf->lfFaceName, L"Courier New");
	logf->lfCharSet = DEFAULT_CHARSET;

	VERIFY(ReleaseDC(hwnd, dc));

	return;
}

void
FillDataFont(PLOGFONT logf, HWND hwnd)
{
	HDC		dc;
	
    memset(logf, 0, sizeof(LOGFONT));
	VERIFY(dc = GetDC(hwnd));
	
	logf->lfHeight		= -MulDiv(8, GetDeviceCaps(GetDC(hwnd), LOGPIXELSY), 72);
	logf->lfWeight		= FW_NORMAL;
	wcscpy(logf->lfFaceName, L"Courier New");

	logf->lfCharSet = DEFAULT_CHARSET;
	VERIFY(ReleaseDC(hwnd, dc));

	return;
}

void
FillHistoryFont(PLOGFONT logf, HWND hwnd)
{
	HDC		dc;
	
    memset(logf, 0, sizeof(LOGFONT));
	VERIFY(dc = GetDC(hwnd));
	
	logf->lfHeight		= -MulDiv(8, GetDeviceCaps(GetDC(hwnd), LOGPIXELSY), 72);
	logf->lfWeight		= FW_NORMAL;
	wcscpy(logf->lfFaceName, L"Courier New");

	logf->lfCharSet = DEFAULT_CHARSET;
	VERIFY(ReleaseDC(hwnd, dc));

	return;
}

void
FillBlobFont(PLOGFONT logf, HWND hwnd)
{
	HDC		dc;
	
    memset(logf, 0, sizeof(LOGFONT));
	VERIFY(dc = GetDC(hwnd));
	
	logf->lfHeight		= -MulDiv(8, GetDeviceCaps(GetDC(hwnd), LOGPIXELSY), 72);
	logf->lfWeight		= FW_NORMAL;
	wcscpy(logf->lfFaceName, L"Courier New");

	logf->lfCharSet = DEFAULT_CHARSET;
	VERIFY(ReleaseDC(hwnd, dc));

	return;
}

// if the user has specified that the max length of column will be by data then we will have max
wyBool
IsDataTrunc()
{
	wyWChar		directory[MAX_PATH+1]= {0}, *lpfileport = 0;
    wyInt32     opt;
	wyString	dirstr;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	opt = wyIni::IniGetInt(GENERALPREFA, "TruncData", 0, dirstr.GetString());

	if(opt)
		return wyTrue;
	return wyFalse;
}

// functions receives handle to edit control as parameter.
// reads the value from .ini file about the word wrpapping mode and sets it accordingly.
wyInt32 
SetEditWordWrap(HWND hwnd, wyBool isrich, wyBool scintilla)
{
	wyWChar		directory[MAX_PATH+1]= {0};
	wyWChar		*lpfileport=0;
    wyInt32     opt;
	wyString	dirstr;

    if(!hwnd && isrich == wyFalse)
		return 1;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	opt	= wyIni::IniGetInt(GENERALPREFA, "WordWrap", 0/* from 5.1 RC1 default value is false */, dirstr.GetString());

	if(scintilla == wyTrue)
	{
		if(opt)
			SendMessage(hwnd, SCI_SETWRAPMODE, SC_WRAP_WORD, SC_WRAP_WORD);
		else
			SendMessage(hwnd, SCI_SETWRAPMODE, SC_WRAP_NONE, SC_WRAP_NONE);
	}
	else
		SendMessage(hwnd, EM_SETTARGETDEVICE, 0, !opt);

	UpdateWindow(hwnd);
	return 1; 
}

// employs advanced heuristic algorithm to get the best  width when the user has selected
// data truncation option.
// if data truncation is not selected then the app automatically gets the best fit.
// if it is specified then width = data length, but it may happen that lot of space is left
// so we increase the column length.
wyUInt32*
CalculateCorrectWidth(HWND hwnd, MYSQL_FIELD * field_list, wyInt32 skipwidth, wyInt32 field_count)
{
	wyInt32		count, pxadded = 0, diff;
	wyUInt32	length = 1;
	HDC			dc;
	wyWChar		temp[SIZE_256] = {0};
	SIZE		dsize, csize, sixsize;
	RECT		rc;
	wyBool		ismysql41 = ((GetActiveWin())->m_ismysql41);
	
	MYSQL_FIELD *field;
	wyString	fieldnamestr;
    HFONT       font, oldfont;

	// allocate buffer.
	wyUInt32	*dlength	= new wyUInt32[field_count];
	wyUInt32	*clength	= new wyUInt32[field_count];
	wyUInt32	*actlength	= new wyUInt32[field_count]; 
	wyUInt32	clensum, dlensum;

	wmemset(temp, 'X', SIZE_256 - 1);
	VERIFY(dc = GetDC(hwnd));

	font    = GetWindowFont(hwnd);
	oldfont =(HFONT)SelectObject(dc, font);

	// get length for the max column width and data length
	for(count=0; count < field_count; count++)
	{
		field = &field_list[count];
		fieldnamestr.SetAs(field->name, ismysql41);
		//fieldnamestr.GetAsWideChar(&length);
		length = fieldnamestr.GetLength();
        
        if(field->max_length == 0)             
            VERIFY(GetTextExtentPoint32(dc,  temp, strlen("(NULL)"), &dsize));
        
        else
		    VERIFY(GetTextExtentPoint32(dc,  temp,(field->max_length > 255)?(255):(field->max_length), &dsize));

		//VERIFY(GetTextExtentPoint32(dc,  temp, fieldnamestr.GetLength(), &csize));
		VERIFY(GetTextExtentPoint32(dc,  temp, length, &csize));
		VERIFY(GetTextExtentPoint32(dc,  temp, 6, &sixsize));

		actlength[count]= dlength[count] = dsize.cx;

		// it may happen that the column length is less then 
		clength[count] =(csize.cx>dsize.cx)?(csize.cx):(dsize.cx);

		if(!dlength[count])
		actlength[count]= dlength[count]= 50;
	}

	SelectObject(dc, oldfont);
	// get the client width of the window.
	VERIFY(GetClientRect(hwnd, &rc));
	// calulate sum.
	clensum = dlensum = 0;

	for(count=0; count<field_count; count++)
    {
		clensum	+= clength[count];
		dlensum += dlength[count];
	}

	// we check if dlensum is less then the window width then only we have to 
	// go for huristic scanning.
	if(dlensum <(wyUInt32)(rc.right-rc.left))
    {
        pxadded = 0;
		// we get the diff.
		diff =((rc.right-rc.left)-dlensum);
		// remove the initial width from teh calculation
		diff -= skipwidth;

		for(count = 0; count < field_count; count++)
        {
			pxadded +=(clength[count]- dlength[count]);
			
			if(pxadded < diff)
				actlength[count] +=(clength[count]- dlength[count]);
			else
				break;
		}

	}

	delete[]dlength;
	delete[]clength;

	VERIFY(ReleaseDC(hwnd, dc));

	return actlength;
}

// Helper function to know whether the main window is in maximised state or not. If its 
// then we need to increase the menu 
wyBool
IsWindowMaximised()
{
	MDIWindow	*wnd;
    wyInt32     lstyle;

	wnd = GetActiveWin();

	if(!wnd)
		return wyFalse;

	lstyle = GetWindowLongPtr(wnd->m_hwnd, GWL_STYLE);

	if(lstyle & WS_MAXIMIZE)
		return wyTrue;
	else
		return wyFalse;
}

/* Help file operation functions */
//removing helpfile from version 12.05, now will use online docs instead
//wyBool 
//LoadHelpFile()
//{
//	wyInt32     ret;
//	wyWChar      directory[MAX_PATH+1]={0}, *lpfileport=0;
//        wyString directorystr; 
//	// open up the sqlyog.chm.. it is located in the executable directory of SQLyog
//	ret = SearchPath(NULL, L"sqlyog", L".chm", MAX_PATH, directory, &lpfileport);
//
//	if(ret == 0)
//		return wyFalse;
//	directorystr.SetAs(directory);
//	strcpy(pGlobals->m_helpfile, directorystr.GetString());
//
//	return wyTrue;
//}

/* functions to maintain the state of current active database for the connection */
const wyChar *
LeftPadText(const wyChar *text)
{
	const wyChar *temp = text;

	while(temp)
	{
		if(!(isspace(*temp)))
			break;
		temp++;
	}

	return temp;
}
#ifndef COMMUNITY
wyBool
ChangeTransactionState(MDIWindow *wnd, const wyChar *query)
{
	SQLFormatter        formatter;
	wyString			queryStr, newquery;

	queryStr.SetAs(query);
	formatter.GetQueryWtOutComments(&queryStr, &newquery);
	newquery.RTrim();
	newquery.LTrim();

	if(newquery.GetLength() != -1 && (newquery.FindI("START") == 0 || newquery.FindI("SET") == 0 || newquery.FindI("BEGIN") == 0 || newquery.FindI("COMMIT") == 0 || newquery.FindI("ROLLBACK") == 0))
	{
		newquery.SetAs(wnd->m_ptransaction->RemoveExtraSpaces(newquery.GetString()));

		if(newquery.FindI("START") != -1 )
		{
			if(newquery.FindI("TRANSACTION") == 6)
				wnd->m_ptransaction->CallOnStart();
		}
		else if(newquery.FindI("COMMIT") == 0 && newquery.GetLength() > 7)
		{
			wyString tempquery = newquery.Substr(7, newquery.GetLength() - 7);
			newquery.SetAs(wnd->m_ptransaction->RemoveExtraSpaces(tempquery.GetString()));
			if(newquery.FindI("AND CHAIN") == 0)
			{
				wnd->m_ptransaction->CallOnCommit();
				wnd->m_ptransaction->CallOnStart();
			}
		}
		else if(newquery.FindI("ROLLBACK") == 0 && newquery.GetLength() > 9)
		{
			wyString tempquery = newquery.Substr(9, newquery.GetLength() - 9);
			newquery.SetAs(wnd->m_ptransaction->RemoveExtraSpaces(tempquery.GetString()));
			if(newquery.FindI("AND CHAIN") == 0)
			{
				wnd->m_ptransaction->CallOnCommit();
				wnd->m_ptransaction->CallOnStart();
			}
			else if(newquery.FindI("TO") != -1)
			{
				if(newquery.FindI("TO") == 0)
					wnd->m_ptransaction->HandleRollbackEditor(&newquery, 3);
				else if(newquery.FindI("WORK") == 0)
				{
					wyString tempstr;
					tempstr.SetAs(newquery.Substr(0, newquery.GetLength() - 0));
					newquery.SetAs(wnd->m_ptransaction->RemoveExtraSpaces(tempstr.GetString()));
					if(newquery.FindI("TO") == 5)
						wnd->m_ptransaction->HandleRollbackEditor(&newquery, 8);
				}
			}
		}
		else if(newquery.FindI("BEGIN") != -1)
		{
			if(wnd->m_ptransaction->HandleBegin(newquery.GetString()))
				wnd->m_ptransaction->CallOnStart();
		}
		else
		{
			if(newquery.FindI("autocommit") == 4)
			{
				if(!wnd->m_ptransaction->m_autocommit)
				{
					if(newquery.FindI("=") != -1 && newquery.FindI("1") != -1)
					{
						wnd->m_ptransaction->m_autocommit = wyTrue;
						wnd->m_ptransaction->CallOnCommit();
					}
				}
				else
				{
					if(newquery.FindI("=") != -1 && newquery.FindI("0") != -1)
					{
						wnd->m_ptransaction->m_autocommit = wyFalse;
						wnd->m_ptransaction->CallOnStart();
					}
				}
			}
			else if(newquery.FindI("SESSION") == 4 && newquery.FindI("TRANSACTION") != -1 && 
				newquery.FindI("ISOLATION") != -1 && newquery.FindI("LEVEL") != -1)
			{
				if(newquery.FindI("REPEATABLE") != -1 && newquery.FindI("READ") != -1)
				{
					wnd->m_ptransaction->m_isolationmode = ID_TRX_REPEATABLEREAD;
				}
				else if(newquery.FindI("COMMITTED") != -1 && newquery.FindI("READ") != -1 && newquery.FindI("UNCOMMITTED") == -1)
				{
					wnd->m_ptransaction->m_isolationmode = ID_TRX_READCOMMITED;
				}
				else if(newquery.FindI("UNCOMMITTED") != -1 && newquery.FindI("READ") != -1)
				{
					wnd->m_ptransaction->m_isolationmode = ID_TRX_READUNCOMMITED;
				}
				else if(newquery.FindI("SERIALIZABLE") != -1)
				{
					wnd->m_ptransaction->m_isolationmode = ID_TRX_SERIALIZABLE;
				}
			}
		}

	}
	else if(wnd->m_ptransaction->m_starttransactionenabled == wyFalse && newquery.GetLength() != -1 && (newquery.FindI("SAVEPOINT") == 0 || newquery.FindI("RELEASE") == 0))
	{
		if(newquery.FindI("SAVEPOINT") == 0)
		{
			SQLFormatter        formatter;
			wyString			queryStr, newquery;
			queryStr.SetAs(query);
			formatter.GetQueryWtOutComments(&queryStr, &newquery);
			newquery.RTrim();
			newquery.LTrim();
			newquery.SetAs(wnd->m_ptransaction->RemoveExtraSpaces(newquery.GetString()));
			wnd->m_ptransaction->HandleSavepointEditor(newquery.GetString());
		}
		else if(newquery.FindI("RELEASE") == 0)
		{
			SQLFormatter        formatter;
			wyString			queryStr, newquery;
			queryStr.SetAs(query);
			formatter.GetQueryWtOutComments(&queryStr, &newquery);
			newquery.RTrim();
			newquery.LTrim();
			newquery.SetAs(wnd->m_ptransaction->RemoveExtraSpaces(newquery.GetString()));
			queryStr.SetAs(newquery);
			newquery.SetAs(queryStr.Substr(8, queryStr.GetLength() - 8));
			if(newquery.FindI("SAVEPOINT") == 0)
			{
				newquery.SetAs(wnd->m_ptransaction->RemoveExtraSpaces(newquery.GetString()));
				wnd->m_ptransaction->HandleReleaseEditor(&newquery, 10);
			}
		}
	}
	return wyTrue;
}
wyBool
CheckTransactionStart(MDIWindow *wnd, const wyChar *query)
{
	SQLFormatter        formatter;
	wyString			queryStr, newquery;

	queryStr.SetAs(query);
	formatter.GetQueryWtOutComments(&queryStr, &newquery);
	newquery.RTrim();
	newquery.LTrim();

	if(newquery.GetLength() != -1 && (newquery.FindI("START") == 0 || newquery.FindI("BEGIN") == 0 || newquery.FindI("COMMIT") == 0 || newquery.FindI("ROLLBACK") == 0))
	{
		newquery.SetAs(wnd->m_ptransaction->RemoveExtraSpaces(newquery.GetString()));

		if(newquery.FindI("START") != -1 )
		{
			if(newquery.FindI("TRANSACTION") == 6)
			{
				wnd->m_ptransaction->OnTunnelMessage(wnd->m_hwnd);
				return wyTrue;
			}
		}
		else if(newquery.FindI("COMMIT") == 0 && newquery.GetLength() > 7)
		{
			wyString tempquery = newquery.Substr(7, newquery.GetLength() - 7);
			newquery.SetAs(wnd->m_ptransaction->RemoveExtraSpaces(tempquery.GetString()));
			if(newquery.FindI("AND CHAIN") == 0)
			{
				wnd->m_ptransaction->OnTunnelMessage(wnd->m_hwnd);
				return wyTrue;
			}
		}
		else if(newquery.FindI("ROLLBACK") == 0 && newquery.GetLength() > 9)
		{
			wyString tempquery = newquery.Substr(9, newquery.GetLength() - 9);
			newquery.SetAs(wnd->m_ptransaction->RemoveExtraSpaces(tempquery.GetString()));
			if(newquery.FindI("AND CHAIN") == 0)
			{
				wnd->m_ptransaction->OnTunnelMessage(wnd->m_hwnd);
				return wyTrue;
			}
		}
		else if(newquery.FindI("BEGIN") != -1)
		{
			if(wnd->m_ptransaction->HandleBegin(newquery.GetString()))
			{
				wnd->m_ptransaction->OnTunnelMessage(wnd->m_hwnd);
				return wyTrue;
			}
		}
	}
	return wyFalse;
}
wyBool
ReadOnlyQueryAllow(wyString *str)
{
	wyBool ret  = wyFalse;
	if(str->FindI("SHOW") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("Select") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("Describe") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("Explain") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("use") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("set") == 0)
	{
		str->SetAs(RemoveExtraSpaces(str->GetString()));
		if(str->FindI("global") == 4)
			ret = wyFalse;
		else if(str->FindI("@@global") == 4)
			ret = wyFalse;
		else
			ret = wyTrue;
	}
	else if(str->FindI("help") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("handler") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("call") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("Start") == 0)
	{
		str->SetAs(RemoveExtraSpaces(str->GetString()));
		if(str->FindI("Transaction") == 6)
			ret = wyTrue;
	}
	else if(str->FindI("Unlock") == 0)
	{
	str->SetAs(RemoveExtraSpaces(str->GetString()));
	if(str->FindI("Tables") == 7)
			ret = wyFalse;
	else
			ret=wyTrue;
	
	}

	else if(str->FindI("flush") == 0)
	{  
		str->SetAs(RemoveExtraSpaces(str->GetString()));
		 if(str->FindI("tables with read lock")==6 || str->FindI("NO_WRITE_TO_BINLOG TABLES WITH READ LOCK") == 6)
			ret=wyFalse;
		else
		ret = wyTrue;
	}
	else if(str->FindI("commit") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("rollback") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("do") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("savepoint") == 0)
	{
		ret = wyTrue;
	}
	/*
		With create we change return to wyFalse since begin will be present in procedures which we shouldn't allow..
		Begin for starting a transaction should be allowed
	*/
	else if(str->FindI("create") == 0)
	{
		ret = wyFalse;
	}
	else if(str->FindI("begin") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("check") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("analyze") == 0)
	{
		ret = wyTrue;
	}
	else if(str->FindI("optimize") == 0)
	{
		ret = wyTrue;
	}
	return ret;
}
wyString
RemoveExtraSpaces(wyString query)
{
	wyInt32 len = query.GetLength();
	for(int i = 0; i < len; i++)
	{
		if(query.GetCharAt(i) == ' ')
		{
			wyInt32 j = i + 1;
			while(query.GetCharAt(j) == ' ')
				j++;
			if(j > i+1)
			{
				query.Replace( i , j - i, " ");
			}
			break;
		}
	}
	return query.GetString();
}
#endif
wyBool
ChangeContextDB(Tunnel * tunnel, PMYSQL mysql, const wyChar *query, wyBool changeincombo)
{
	const wyChar *tquery =(wyChar*)query, *tquery2 = NULL;
    wyString db;

    tquery = GetCommentLex((wyChar*)tquery);

    for(;(tquery = GetCommentLex((wyChar*)tquery))!= tquery2; tquery2 = tquery)
        tquery = LeftPadText(tquery);

	if(tquery &&(strnicmp(tquery, "use", 3)== 0))
    {
		// that means its an use statement so we have to work on it.
		GetDBFromQuery(tquery, db);

		if(changeincombo)
			pGlobals->m_pcmainwin->ChangeDBInCombo(tunnel, mysql, (wyWChar*)db.GetAsWideChar());

		tunnel->SetDB((wyChar*)db.GetString());
	}
	return wyTrue;
}

void
GetDBFromQuery(const wyChar *query, wyString &db)
{
    OPDETAILS	*op_details = new OPDETAILS;	
	LexHelper	lex;
    wyInt32     len;
	wyString	tablestr;
	wyWChar		*table = 0;
	wyUInt32	length = 1;
	SQLFormatter        formatter;
    wyString            tempdump(query), queryStr;
    wyChar              *querywocomments = NULL;
    
    formatter.GetQueryWtOutComments(&tempdump, &queryStr);
    querywocomments = _strdup(queryStr.GetString());

	if(querywocomments && !lex.StartLex((wyChar*)querywocomments, 0, op_details, wyTrue))
    {
        if(op_details->db)
        {
			 // it might be that the database is sorrounded by ` so we have remove it 
		    len = strlen(op_details->table);
			if(len != 0)
			{
				tablestr.SetAs(op_details->table);
	
				if(op_details->table[len-1] == '`')
					op_details->table[len-1]='\0';
				table = tablestr.GetAsWideChar(&length);
                TrimSpacesAtEnd(table);
				tablestr.SetAs(table);
				strcpy(op_details->table, tablestr.GetString());   		
			    db.SetAs(op_details->table);
			}
	    }
    }

    if(op_details)
        delete op_details;

    if(querywocomments)
        free(querywocomments);

    return;
}

/* Generic function to write text in an xml file handling characters like
   <,>,etc. */
wyBool 
WriteXMLToFile(HANDLE file, const wyChar *text, wyBool crlf)
{
	wyInt32     ret = 1;
	DWORD		dwbyteswritten;
	wyString	textstr;

	while(*text)
	{
		switch(*text)
		{
		case '<':
			ret = WriteFile(file, "&lt;", 4, &dwbyteswritten, NULL);
			break;

		case '>':
			ret = WriteFile(file, "&gt;", 4, &dwbyteswritten, NULL);
			break;

		case '&':
			ret = WriteFile(file, "&amp;", 5, &dwbyteswritten, NULL);
			break;

		case '\"':
			ret = WriteFile(file, "&quot;", 6, &dwbyteswritten, NULL);
			break;

		default:
			if(crlf == wyTrue && *text == '\r' && *(text+1)== '\n'){
				ret = WriteFile(file, "<br>", 4, &dwbyteswritten, NULL);
				text += 2;
			} 
			else 
			{
				textstr.SetAs(text);	
					ret = WriteFile(file, textstr.GetString(), 1, &dwbyteswritten, NULL);
			}
			break;						
		}
		
		if(!ret)
			return wyFalse;

		text++;
	}

	return wyTrue;
}


void
SciRedraw(MDIWindow	*wnd, wyBool redraw)
{
	//Post 8.01
    /*wyInt32     state =(redraw == wyTrue)?1:0;

	SendMessage(wnd->m_pctabmodule->GetHwnd(), WM_SETREDRAW, state, 0);

	if(redraw == wyTrue)
    {
		//InvalidateRect(wnd->m_pctabmodule->GetHwnd(), NULL, FALSE);		

		UpdateWindow(wnd->m_hwnd);
	}*/

	return;
}


/*
 *		Function fills up the COMBOBOX with database names from the objectbrowser in the current
 *		active query window.
 */
wyBool
FillComboWithDBNames(HWND hwndCombo, wyBool isextended)
{
	MDIWindow			*win;
	HWND				objectbrowser;

	wyWChar				dbname[SIZE_512] = {0};
	TVITEM				tvi={0};
	HTREEITEM			titem;
    wyInt32             cnt, count,width = 0;
	
	COMBOBOXEXITEM		cbi = {0};
	
	/* get the active query window */
	VERIFY(win = GetActiveWin());

	if(!win)
		return wyFalse;

	SetCursor(LoadCursor(NULL, IDC_WAIT));

	/* get the object browser */
	objectbrowser = win->m_pcqueryobject->m_hwnd;

	/* now loop thru each db items and add them to the combo box but before that we always clear
	   up the combo box items */
	if(isextended)
		SendMessage(hwndCombo, CB_RESETCONTENT, 0, 0);
	else 
    {
		count = SendMessage(hwndCombo, CB_GETCOUNT, 0, 0);

		for(cnt = 0; cnt < count; cnt++)
			VERIFY((SendMessage(hwndCombo, CB_DELETESTRING, 0, 0))!= CB_ERR);
	}
	
	VERIFY(titem = TreeView_GetRoot(objectbrowser));
	for(titem = TreeView_GetChild(objectbrowser, titem); titem ; 
			titem = TreeView_GetNextSibling(objectbrowser, titem))
	{
		/* get the dbname from the objectbrowser and add it up in the combo box*/
		tvi.mask		=	TVIF_TEXT;
		tvi.hItem		=	titem;
		tvi.cchTextMax	=	((sizeof(dbname)-1)*2);
		tvi.pszText		=	dbname;

		VERIFY(TreeView_GetItem(objectbrowser, &tvi));

		/* add it up in the object browser */
		memset(&cbi, 0, sizeof(cbi));

		cbi.mask			= CBEIF_TEXT | CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
		cbi.pszText			= dbname;
		cbi.cchTextMax		= tvi.cchTextMax;
		cbi.iImage			= 0;
		cbi.iSelectedImage	= 0;
		cbi.iItem			= -1;
		
		if(isextended){
			VERIFY(SendMessage(hwndCombo, CBEM_INSERTITEM, 0L,(LPARAM)&cbi)!= -1);
		} else {
			VERIFY(SendMessage(hwndCombo, CB_ADDSTRING, 0,(LPARAM)dbname)!= CB_ERR);			
		}
	}
	
	width = SetComboWidth(hwndCombo);
	SendMessage(hwndCombo, CB_SETDROPPEDWIDTH, width + 50, 0); //width + 50 means width of the text + width of the scroll bar
	SetCursor(LoadCursor(NULL, IDC_ARROW));

	return wyTrue;
}

// ensures a range is visible in the scintilla control

void 
EnsureRangeVisible(HWND hwnd, wyInt32 posstart, wyInt32 posend)
{
    wyInt32 line;
	wyInt32 linestart	= SendMessage(hwnd, SCI_LINEFROMPOSITION, min(posstart, posend), 0);
	wyInt32 lineend		= SendMessage(hwnd, SCI_LINEFROMPOSITION, max(posstart, posend), 0);

	for(line = linestart; line <= lineend; line++){
		SendMessage(hwnd, SCI_ENSUREVISIBLE, line, 0);
	}
    return;
}

wyChar*
GetDefaultValue(Tunnel* tunnel, MYSQL_RES * myfieldres, MYSQL_RES * myres,  const wyChar * field)
{
	VERIFY(myfieldres);

    wyUInt32    rowcount, count;
	MYSQL_ROW	myrow;
	wyString	myrowstr, myrowfindex;
	wyBool		ismysql41 = ((GetActiveWin())->m_ismysql41);
	
	if(!myfieldres)
		return NULL;

	rowcount =(wyUInt32)myfieldres->row_count;

	for(count = 0; count < rowcount; count++)
    {
		tunnel->mysql_data_seek(myfieldres, count);

		VERIFY(myrow = tunnel->mysql_fetch_row(myfieldres));
		
		myrowstr.SetAs(myrow[0], ismysql41);

		if((myrowstr.Compare(field))== 0)
			return myrow[GetFieldIndex(myfieldres, "default", tunnel)];
		
	}
	return NULL;
}

wyChar*
GetDataType(Tunnel* tunnel, MYSQL_RES * myfieldres, const wyChar * field)
{
	VERIFY(myfieldres);

	wyUInt32    rowcount, count;
	MYSQL_ROW	myrow;
	wyString	myrowstr, myrowfieldindexstr;
	wyBool		ismysql41 = ((GetActiveWin())->m_ismysql41);
	
	rowcount =(wyUInt32)myfieldres->row_count;

	for(count = 0; count < rowcount; count++)
    {
		tunnel->mysql_data_seek(myfieldres, count);
		VERIFY(myrow = tunnel->mysql_fetch_row(myfieldres));

		myrowstr.SetAs(myrow[0], ismysql41);

		if((myrowstr.CompareI(field))== 0)
			return myrow[GetFieldIndex(myfieldres, "type", tunnel)];
	}
	return NULL;
}

wyBool
IsNullable(Tunnel* tunnel, MYSQL_RES * myfieldres, wyChar * field)
{
	VERIFY(myfieldres);

	wyUInt32		rowcount, count;
	MYSQL_ROW		myrow;
	wyString		myrowstr;
	wyBool			ismysql41 = ((GetActiveWin())->m_ismysql41);
    wyInt32         i;

	rowcount =(wyUInt32)myfieldres->row_count;

	for(count = 0; count < rowcount; count++)
    {
		tunnel->mysql_data_seek(myfieldres, count);

		VERIFY(myrow = tunnel->mysql_fetch_row(myfieldres));
		
		myrowstr.SetAs(myrow[0], ismysql41);

		if((myrowstr.Compare(field))== 0)
        {
            if((i = GetFieldIndex(myfieldres, "Null", tunnel)) >= 0)
            {
                if(myrow[i] && !stricmp(myrow[i], "yes"))
                {
				    return wyTrue;
                }
            }
		
            return wyFalse;			
		}
	}

	return wyFalse;
}

// Generic function returns the width of a column for the grid
// it keeps in mind data truncation flag.
wyInt32 
GetColWidth(HWND grid, MYSQL_FIELD * myfield, wyInt32 index)
{	
	wyInt32     cx, extra;
	wyUInt32	length = 1;
	HDC			hdc;
	SIZE		maxlen, max255, maxfield, maxsix, sizeone, max50;
	wyWChar		temptext[SIZE_256] = {0};
	HGDIOBJ		hgdiobj;
    SIZE		blobwidth={0};
	wyString	myfieldnamestr;
	wyBool		ismysql41 = GetActiveWin() ? ((GetActiveWin())->m_ismysql41) : wyTrue;
	// calculate the column width.
	// We get the max-length of the field and allocate a temporary memory with the text character 'e'
	// and then we calculate the length in logical units.
	
	// We also calculate the length of column text. if its more then the area required by the field
	// data then we set this length.
	
	// Also it may happen that the text is long enuf and its too big to show the whole text
	// then we calculate the width required for 255 characters.
	
	// First memset the data with 255 characters and get the size of 255 characters.
	wmemset(temptext, 'X', SIZE_256 - 1);
	memset((void*)&maxlen, 0, sizeof(SIZE));
	memset((void*)&max255, 0, sizeof(SIZE));
	memset((void*)&maxfield, 0, sizeof(SIZE));

	// Get the handle to the DC.
	VERIFY(hdc = GetDC(grid));

	// Now select the font object into the DC.
	hgdiobj = SelectObject(hdc,(HFONT)CustomGrid_GetRowFont(grid));
	_ASSERT(hgdiobj != NULL || hgdiobj != HGDI_ERROR);
	
	myfieldnamestr.SetAs(myfield->name, ismysql41);
	// First get for field name length, then maxi for 255 charcters and then for the maximumlen for the field
	// returned by the resultset.
	myfieldnamestr.GetAsWideChar(&length);
    //length = myfieldnamestr.GetLength();

    //wcscpy(temptext, myfieldnamestr.GetAsWideChar());
	wcsncpy(temptext, myfieldnamestr.GetAsWideChar(), SIZE_256 - 1);
	temptext[SIZE_256 - 1] = '\0';

	VERIFY(GetTextExtentPoint32(hdc, temptext, length, &maxfield));
	VERIFY(GetTextExtentPoint32(hdc, temptext, 64, &max255));
	VERIFY(GetTextExtentPoint32(hdc, temptext, 40, &max50));
	VERIFY(GetTextExtentPoint32(hdc, temptext,(myfield->max_length > 255)?(255):(myfield->max_length), &maxlen));
	VERIFY(GetTextExtentPoint32(hdc, temptext, 6, &maxsix));
	VERIFY(GetTextExtentPoint32(hdc, temptext, 1, &sizeone));
		
	// now calculate which has to implemented,
	extra  = 5;
	// but first we check is it blob
	if(((myfield->type >= FIELD_TYPE_TINY_BLOB)&&(myfield->type <= FIELD_TYPE_BLOB)) || (myfield->type == MYSQL_TYPE_JSON) )
    {
		//cx = maxlen.cx + extra;
		//Same code repeated in else condition also, I will change this
		if(maxfield.cx >= maxlen.cx)	
        {
			if(myfield->max_length == 0)
				cx = max(maxsix.cx, maxfield.cx)+ extra;
			else if((myfieldnamestr.GetLength())< 6)
				cx = maxsix.cx + extra;
			else
				cx = maxfield.cx + extra; //
		}
		else if(maxlen.cx > max255.cx)
			cx = max255.cx + extra;
		else 
			cx =(maxlen.cx > maxsix.cx)?(maxlen.cx):(maxsix.cx)+ extra; //

		cx = max(maxsix.cx, cx);
		cx += extra;

		if(cx < CELLBUTTONMINWIDTH)
			cx = CELLBUTTONMINWIDTH * 2;
	} 
    else 
    { 
		if(IsDataTrunc())
        {
			/* at max in insert/update window we have 256 characters 
				change in 4.0 as half of the population is having problem with witdth size */
			cx = min(myfield->max_length, MAX_COL_WIDTH);
			VERIFY(GetTextExtentPoint32(hdc, temptext, cx, &blobwidth));
			cx = blobwidth.cx + extra;
		} 
        else 
        {
			if(maxfield.cx >= maxlen.cx)	
            {
				if(myfield->max_length == 0)
					cx = max(maxsix.cx, maxfield.cx)+ extra;
				else if((myfieldnamestr.GetLength())< 6)
					cx = maxsix.cx + extra;
				else
					cx = maxfield.cx + extra; //
			}
			else if(maxlen.cx > max255.cx)
				cx = max255.cx + extra;
			else 
				cx =(maxlen.cx > maxsix.cx)?(maxlen.cx):(maxsix.cx)+ extra; //

			cx = max(maxsix.cx, cx);
			cx += extra;
		}
	}

	SelectObject(hdc,(HFONT)hgdiobj);
	ReleaseDC(grid, hdc);
	return cx + SORTICONWIDTH;
}

// Generic function to return the alignment of the column based on the type
// its LEFT for text, right for number and centre for dat
wyInt32 
GetColAlignment(MYSQL_FIELD * field)
{
	/* the above code dosnt work due to a bug in IS_NUM_FIELD so we create another macro called IS_NUMBER and use it */
	if(IS_NUMBER(field->type))
		return GVIF_RIGHT;
	else if(IS_DATE(field->type))
		return GVIF_CENTER;
	else
		GVIF_LEFT;

	return GVIF_LEFT;
}

// generic function to add enum or set values to a column in a grid
// used in insert/update as well as data editing tab
wyBool
AddEnumValues(HWND hwndgrid, Tunnel * m_tunnel, const wyChar * name, MYSQL_RES * myres, 
			   MYSQL_RES * myfieldres, INT index)
{
	VERIFY(myfieldres);

	wyUInt32        rowcount;
	DWORD			count=0, parsecount=0, fieldcount;
	MYSQL_ROW		myrow;
	wyString		myrowstr;
	wyString		textstr;
	wyBool			ismysql41 = (GetActiveWin())->m_ismysql41;
    wyBool          close = wyFalse;
    wyInt32         isnull;
	rowcount =(wyUInt32)myfieldres->row_count;

	for(fieldcount=0; count<rowcount; fieldcount++)
    {
		m_tunnel->mysql_data_seek(myfieldres, fieldcount);

		VERIFY(myrow = m_tunnel->mysql_fetch_row(myfieldres));
		
        if(!myrow)
		    return wyFalse;
		
		myrowstr.SetAs(myrow[GetFieldIndex(myfieldres, "field", m_tunnel)], ismysql41);	

		if((myrowstr.Compare(name))== 0)
			break;
	}

	m_tunnel->mysql_data_seek(myfieldres, fieldcount);

	VERIFY(myrow = m_tunnel->mysql_fetch_row(myfieldres));

     if(!myrow)
            return wyFalse;

	// first copy the datatype so that we can get what we want.
	for(count=0; myrow[1][count]&& myrow[1][count]!= C_OPEN_BRACKET; count++);

	// now we get set value and add it in the list.
    isnull = GetFieldIndex(myfieldres, "Null", m_tunnel);

    if(!myrow[isnull] || stricmp(myrow[isnull], "yes") == 0)
	CustomGrid_InsertTextInList(hwndgrid, index, TEXT(STRING_NULL));

	count += 2;

	while(!close)
    {
		wyChar *text = (wyChar*)calloc(sizeof(wyChar), strlen(myrow[1]) + 1);
		
		for(parsecount=0; myrow[1][count]; parsecount++, count++)
        {
			if(myrow[1][count]== C_SINGLE_QUOTE){

				if(myrow[1][count+1]== C_CLOSE_BRACKET)
                {
					count = count + 2;
					close = wyTrue;
					break;
				}
				else if(myrow[1][count+1] == C_COMMA) 
				{
					count += 3;
					break;
				}
				else if(myrow[1][count+1] == C_SINGLE_QUOTE)//escaping the ' (single quote) character
				{
					count += 1;
				}
			}
			else if(myrow[1][count] == C_BACK_SLASH)
			{
				if(myrow[1][count+1] == C_BACK_SLASH)
				{
					count += 1;
				}
			}		
			text[parsecount]= myrow[1][count];
		}	

		textstr.SetAs(text);

		CustomGrid_InsertTextInList(hwndgrid, index, textstr.GetAsWideChar());
        free(text);
	}
	return wyTrue;
}

// common methods to find whether a given column name is of type of set or enum

wyBool
IsColumnEnum(Tunnel * m_tunnel, MYSQL_RES * myfieldres, MYSQL_RES * myres, const wyChar * name)
{
	VERIFY(myfieldres);

	wyUInt32        rowcount;
	DWORD			count = 0, fieldcount;
	MYSQL_ROW		myrow;
	wyString		namestr, myrowstr;
	wyBool			ismysql41 = ((GetActiveWin())->m_ismysql41);

	//wyChar			text[SIZE_256]={0};

	rowcount =(wyUInt32)myfieldres->row_count;

	if(!rowcount)
        return wyFalse;

	for(fieldcount=0; count < rowcount; fieldcount++)
    {
		m_tunnel->mysql_data_seek(myfieldres, fieldcount);

		myrow = m_tunnel->mysql_fetch_row(myfieldres);
        				
		if(!myrow)
			return wyFalse;
		
		myrowstr.SetAs(myrow[GetFieldIndex(myfieldres, "field", m_tunnel)], ismysql41);

		if((myrowstr.CompareI(name))== 0)
			break;
	}

	VERIFY(fieldcount < rowcount);

	m_tunnel->mysql_data_seek(myfieldres , fieldcount);

	VERIFY(myrow = m_tunnel->mysql_fetch_row(myfieldres));

    if(!myrow)
	    return wyFalse;

	// first copy the datatype so that we can get what we want.
	//for(count = 0; myrow[1][count]&& myrow[1][count]!= C_OPEN_BRACKET; count++)
	//	text[count]= myrow[1][count];
	
	myrowstr.SetAs(myrow[1], ismysql41);

	if(myrow[1] && strnicmp(myrowstr.GetString(), "enum", 4) == 0)
		return wyTrue;
	
    return wyFalse;

}

// Function to check whether the given column is enum or set.

wyBool
IsColumnSet(Tunnel * m_tunnel, MYSQL_RES * myfieldres, MYSQL_RES * myres, const wyChar * name)
{

	VERIFY(myfieldres);

	wyUInt32        rowcount;
	DWORD			count = 0, fieldcount;
	MYSQL_ROW		myrow;
	wyChar			text[SIZE_256]={0};
	wyString		myrowstr;
	wyBool			ismysql41 = ((GetActiveWin())->m_ismysql41);

	rowcount =(wyUInt32)myfieldres->row_count;

	for(fieldcount = 0; count < rowcount; fieldcount++)
    {
		m_tunnel->mysql_data_seek(myfieldres, fieldcount);

		myrow = m_tunnel->mysql_fetch_row(myfieldres);
		if(!myrow)
			return wyFalse;
		
		myrowstr.SetAs(myrow[GetFieldIndex(myfieldres, "field", m_tunnel)], ismysql41);

		if((myrowstr.CompareI(name))== 0)
			break;
	}

	VERIFY(fieldcount < rowcount);

	m_tunnel->mysql_data_seek(myfieldres, fieldcount);

	VERIFY(myrow = m_tunnel->mysql_fetch_row(myfieldres));

	// first copy the datatype so that we can get what we want.
	for(count = 0; myrow[1][count]&& myrow[1][count]!= C_OPEN_BRACKET; count++)
		text[count]= myrow[1][count];

	if(stricmp(text, "set")== 0)
		return wyTrue;
	
    return wyFalse;

}

// common function says whether there is a primary key for the table or not
wyBool
IsAnyPrimary(Tunnel * m_tunnel, MYSQL_RES * mykeyres, wyInt32 * pcount)
{
    MYSQL_ROW	myrow;
    wyInt32     count = 0;

    if(pcount)
    {
        *pcount = 0;
    }

	if(!mykeyres)
		return wyFalse;

	m_tunnel->mysql_data_seek(mykeyres, 0);

	while(myrow = m_tunnel->mysql_fetch_row(mykeyres))
	{
		if((stricmp(myrow[GetFieldIndex(mykeyres, "key_name", m_tunnel)], "primary"))== 0)
        {
            count++;
        }
	}

    if(pcount)
    {
        *pcount = count;
    }

	return count ? wyTrue : wyFalse;
}

// function searches for the column name in the key res and finds if its part of a primary key column
wyBool
IsColumnPrimary(Tunnel *tunnel, MYSQL_RES *myfieldres, wyChar *column)
{
    wyUInt32    count, prival = 0;
	MYSQL_ROW	myrow;

	if(!myfieldres)
		return wyFalse;

	prival = GetFieldIndex(myfieldres, "key", tunnel);

	for(count = 0; count < myfieldres->row_count; count++)
	{
		tunnel->mysql_data_seek(myfieldres, count);

		myrow = tunnel->mysql_fetch_row(myfieldres);

		if((stricmp(column, myrow[0])== 0)&&((strstr(myrow[prival], "PRI"))))
			return wyTrue;
	}

	return wyFalse;
}

// Function reads the global .ini file and returns whether to warn user on update row
// problem
wyBool
UpdatePrompt()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH+1]= {0};
    wyInt32     ret;
	wyString	dirstr;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "PromptUpdate", 1, dirstr.GetString());

	return(ret)?(wyTrue):(wyFalse);
}

wyBool 
AppendBackQuotes()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH+1]= {0};
    wyInt32     ret = 0;
	wyString	dirstr;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "AppendBackQuotes", 1, dirstr.GetString());

	return(ret)?(wyTrue):(wyFalse);
}

wyBool
ShowData()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH + 1] = {0};
    wyInt32     ret;
	wyString	dirstr;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "ShowData", 1, dirstr.GetString());

	return(ret)?(wyTrue):(wyFalse);
}

/* Function returns whether to set focus on edit control or result pane after execution of
   query */
wyBool 
IsEditorFocus()
{
	wyWChar     *lpfileport = 0;
	wyWChar     directory[MAX_PATH + 1]= {0};
    wyInt32     ret;
	wyString	dirstr;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "FocusOnEdit", 1, dirstr.GetString());

	return(ret)?(wyTrue):(wyFalse);
}
/// function returns if  checkenable transaction support check box
wyBool
IsStartTransactionEnable()
{
	wyWChar     *lpfileport = 0;
	wyWChar     directory[MAX_PATH + 1]= {0};
    wyInt32     ret;
	wyString	dirstr;
	

	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "StartTransaction", 1, dirstr.GetString());
	return(ret)?(wyTrue):(wyFalse);
}
//function returns wytrue  if refresh option  is changed from F5 to f9
wyBool
IsRefreshOptionChange()
{
	wyWChar     *lpfileport = 0;
	wyWChar     directory[MAX_PATH + 1]= {0};
    wyInt32     ret;
	wyString	dirstr;
	

	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "SwitchShortcut", 0, dirstr.GetString());//getting Switch shortcut status from init File



	return(ret)?(wyTrue):(wyFalse);

}
wyBool
IsConfirmOnTabClose()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH + 1]= {0};
    wyInt32     ret;
	wyString	dirstr;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "PromptOnTabClose", 1, dirstr.GetString());

	return(ret)?(wyTrue):(wyFalse);
}

//return  the keyword case used to display in editor
wyInt32
GetKeyWordCase()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH + 1]= {0};
	wyString	dirstr;
	wyString    kwcasestr;
	wyInt32     kwcase;

	// Get the complete path.	
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);

	wyIni::IniGetString(GENERALPREFA, "KeywordCase", "UPPERCASE", &kwcasestr, dirstr.GetString());
	
	if(kwcasestr.CompareI("uppercase") == 0)
		kwcase = 0;
	else if(kwcasestr.CompareI("lowercase") == 0)
		kwcase = 1;
	else
		kwcase = 2;

	return kwcase;
}
//return the function case used to display in editor
wyInt32
GetFunCase()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH + 1]= {0};
	wyString	dirstr;
	wyString    funcasestr;
	wyInt32     funcase;

	// Get the complete path.	
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);

	wyIni::IniGetString(GENERALPREFA, "FunctionCase", "UPPERCASE", &funcasestr, dirstr.GetString());
	
	if(funcasestr.CompareI("uppercase") == 0)
		funcase = 0;
	else if(funcasestr.CompareI("lowercase") == 0)
		funcase = 1;
	else
		funcase = 2;

	return funcase;
}

wyBool
IsSmartKeyword()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH + 1]= {0};
    wyInt32     ret;
	wyString	dirstr;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "SmartKeyword", 1, dirstr.GetString());

	return(ret)?(wyTrue):(wyFalse);
}

wyBool
IsInsertTextOnDBClick()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH + 1]= {0};
    wyInt32     ret;
	wyString	dirstr;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "GetTextOnDBClick", 1, dirstr.GetString());

	return(ret)?(wyTrue):(wyFalse);
}

void 
GetColumnName(wyWChar *field)
{
	// we are showing the column as columnname, data type, Nullable(if null)
	//So fro getting the column ,we are searching for first comma.
	//column name with comma is not handling
	field = wcstok(field, L",");
    return;
}

int  IsColumnTypeJson(wyWChar *field)
{
	wchar_t* result=wcschr(field,L',');
	if(result)
	{
		if(wcsncmp(result,L", json",6))
			return 0;
		else 
			return 1;
	}
	else
		return 0;
}

wyBool
IsGetObjectInfoAlways()
{
	wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH+1]= {0};
    wyInt32     ret;
	wyString	dirstr;
	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "GetInfoAlways", 0, dirstr.GetString());

	return(ret)?(wyTrue):(wyFalse);
}


wyBool
IsOpenTablesInOB()
{
	wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH+1]= {0};
    wyInt32     ret;
	wyString	dirstr;
	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "OBOpenTablesByDefault", 0, dirstr.GetString());

	return(ret)?(wyTrue):(wyFalse);
}

wyBool
IsConnectionRestore()
{
	wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH+1]= {0};
    wyInt32     ret;
	wyString	dirstr;
	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "SessionRestore", 1, dirstr.GetString());

	return(ret)?(wyTrue):(wyFalse);
}


wyBool 
IsShowAllInTableData()
{	
	wyInt32 		ret; 
	wyWChar			directory[MAX_PATH + 1]={0}, *lpfileport=0;
	wyString		dirstr;

	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	
	dirstr.SetAs(directory);
	
	ret = wyIni::IniGetInt(GENERALPREFA, "ShowAllInTableData", 0, dirstr.GetString());

	return ret? wyTrue: wyFalse;	
}

wyBool
IsInfoTabHTMLFormat()
{
	wyInt32 		ret; 
	wyWChar			directory[MAX_PATH + 1]={0}, *lpfileport=0;
	wyString		dirstr;

	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	
	dirstr.SetAs(directory);
	
	ret = wyIni::IniGetInt(GENERALPREFA, "InfoTabFormatOption", INFOTABFORMATOPTION_DEFAULT, dirstr.GetString());

	return ret? wyTrue: wyFalse;	
}

void 
CollectCurrentWindowsVersion()
{
	DWORD dwversion = GetVersion();
 
	// Get the Windows version.
	pGlobals->m_windowsmajorversion =(DWORD)(LOBYTE(LOWORD(dwversion)));
	pGlobals->m_windowsminorversion =(DWORD)(HIBYTE(LOWORD(dwversion)));

	if(dwversion < 0x80000000)           // Windows NT
		pGlobals->m_windowsbuild =(DWORD)(HIWORD(dwversion));
	else if(pGlobals->m_windowsmajorversion < 4)   // Win32s
		pGlobals->m_windowsbuild =(DWORD)(HIWORD(dwversion)& ~0x8000);
	else                                     // Windows Me/98/95
		pGlobals->m_windowsbuild =  0;
}

void 
InitCustomControls()
{
	INITCOMMONCONTROLSEX icex;
	icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	icex.dwICC = ICC_BAR_CLASSES | ICC_USEREX_CLASSES | ICC_WIN95_CLASSES ;	
	VERIFY(InitCommonControlsEx(&icex));

	// load the custom GridControl.
	VERIFY(InitCustomGrid());
    // load the custom TabControl.
	VERIFY(InitCustomTab());
    return;
}

void 
WriteToFile(HANDLE hfile, const wyChar *buffer)
{
    DWORD		dwbyteswritten = 0;
	wyChar		*widestr = NULL;

	widestr	=	GetUtf8String(buffer);
	WRITETOFILE(hfile, widestr, &dwbyteswritten);
	delete[]widestr;
}



void 
HandlerToWriteXMLToFile(HWND hwndpage, HANDLE hfile, wyInt32 id, wyBool crlf)
{
    wyWChar		buffer[SIZE_1024] = {0};
	wyString	bufferstr;

	SendMessage(GetDlgItem(hwndpage, id), WM_GETTEXT, SIZE_1024 - 1,(LPARAM)buffer);
	
	bufferstr.SetAs(buffer);

	WriteXMLToFile(hfile, bufferstr.GetString(), wyFalse);

    return;
}

void 
WriteSmtpTag(HWND hwndpage, HANDLE hfile, const wyChar *tag, wyInt32 id, wyBool crlf)
{
    wyString writebuffer;

    writebuffer.Sprintf("<%s>", tag);
	WriteToFile(hfile, writebuffer.GetString());

    HandlerToWriteXMLToFile(hwndpage, hfile, id, wyFalse);

    writebuffer.Sprintf("</%s>\r\n", tag);
	WriteToFile(hfile, writebuffer.GetString());
    return;
}

ConnectionBase * 
CreateConnection()
{
    ConnectionBase *conn;

#ifdef COMMUNITY
    conn = new ConnectionCommunity();
#else
    wyInt32         retval = 1;

    conn = new CConnectionEnt(&retval);

    if(retval == 0)
    {
        delete conn;
        conn = NULL;
    }
#endif

    return conn;
}

PreferenceBase * 
CreatePreferences()
{
#ifdef COMMUNITY
    return(new PreferencesCommunity());
#else
    return(new PreferencesEnt());
#endif
}

/* simple function to handle and dispatch messages till an event is signalled */
void
HandleMsgs(HANDLE event, wyBool istranslateaccelerator,HWND hwnd)
{
    //	DEBUG_ENTER("HandleMsgs");

    DWORD   result; 
    MSG     msg; 

	while(1)
	{
		// Read all of the messages in this next loop, 
		// removing each message as we read it.
		while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{ 
            // dispatch the message
			if(istranslateaccelerator == wyFalse || !TranslateMDISysAccel(pGlobals->m_hwndclient, &msg))	
            {
				if(istranslateaccelerator == wyFalse || !(TranslateAccelerator(pGlobals->m_pcmainwin->GetHwnd(), g_accel, &msg)))
                {
                    if(pGlobals->m_pcmainwin->m_hwndtooltip && (/*msg.message == WM_LBUTTONDBLCLK ||*/ msg.message == WM_LBUTTONDOWN))
                    {
                        FrameWindow::ShowQueryExecToolTip(wyFalse);
                    }
					//added IsDialogMessage() to handle tabs 
					if (hwnd == NULL || !IsDialogMessage(hwnd, &msg))
					{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
			}
		} // End of PeekMessage while loop.

		// Wait for any message sent or posted to this queue 
		// or for one of the passed handles be set to signaled.
		result = MsgWaitForMultipleObjects(1, &event, 
		                FALSE, INFINITE, QS_ALLINPUT); 

		// The result tells us the type of event we have.
		if(result ==(WAIT_OBJECT_0 + 1))
		{
			// New messages have arrived. 
			// Continue to the top of the always while loop to 
			// dispatch them and resume waiting.
			continue;
		} 
		else 
		{ 
			// the event got signalled
			break; 
		} // End of else clause.
	} // End of the always while loop. 

	return;
}

wyInt32
GetTabSize()
{
    wyInt32     tabsize;
	wyWChar     *lpfileport = 0;
	wyWChar     directory[MAX_PATH+1] = {0};
	wyString	dirstr;
	
	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini",MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	tabsize = wyIni::IniGetInt(GENERALPREFA, "TabSize", DEF_TAB_SIZE, dirstr.GetString());

	//If Tab Size = 0, then it should take default Tab Size 8
	if(tabsize == 0)
		tabsize = DEF_TAB_SIZE;

	return tabsize;
}

// Lex coment.
// Three types of commenting style provided by mySQL is checked.

wyChar *   
GetCommentLex(wyChar *buffer)
{
    wyInt32     curpos = 0;
	wyChar		curr = *(buffer+curpos);
	wyInt32     start = curpos;

	if(curr == C_FRONT_SLASH)
	{
		if(*(buffer+curpos+1)== C_STAR)	// Its comment
		{
			curpos += 2;

			while(1)
			{
				if((*(buffer+curpos)== C_STAR)&&(*(buffer+curpos+1)== C_FRONT_SLASH))
				{
					curpos+=2;
					break;
				}
				else if(*(buffer+curpos)== '\0')
					break;

				curpos++;
			}
		}
		else								   // Its a arith.	
		{
			curpos++;
		}
	}
	else if(curr == C_HASH)					// Again a diff type of comment. of -- type
	{
		curpos++;

		while((*(buffer+curpos)!= C_NEWLINE)&& 
				(*(buffer+curpos)!= C_NULL)&& 
				(*(buffer+curpos)!= C_CARRIAGE))
			curpos++;
		
	}
	else if(curr == C_DASH)
	{
		if((*(buffer+curpos+1)== C_DASH)&&(*(buffer+curpos+2)== C_SPACE))
		{
			curpos += 2;
			while((*(buffer+curpos)!= C_NEWLINE)&&(*(buffer+curpos)!= C_NULL)&&(*(buffer+curpos)!= C_CARRIAGE))
				curpos++;

		}
		else
		{
			curpos++;
		}
	}
	
	if(start == curpos)
		return buffer;
	
    return(buffer+curpos);

}

void 
GetUtf8ColLength(MYSQL_ROW row, wyInt32 num_cols, 
                                        wyInt32 col, wyUInt32 *collength, wyBool ismysql41)
{
	wyChar		*value = NULL;
	wyString	datastr;

	GetColLength(row, num_cols, col, collength);
	
	if(ismysql41 == wyFalse)
	{
		value = row[col];
		if(value && *collength)
		{
			if(!memchr(value, 0, (size_t)collength))
				return;

			datastr.SetAs(value, ismysql41);

			if(datastr.GetLength() < *collength)
				return;

			*collength = datastr.GetLength();
		}
	}
	return;
}

void
GetColLengthArray(MYSQL_ROW row, wyInt32 col, wyUInt32 *len)
{
	MYSQL_ROW       column = row;
	wyUInt32       *arr = NULL, *plen = 0;
	wyInt32         i = 0;
	byte            *start = 0;

    VERIFY(len);
	arr = len;

	// loop thru and get the length of each field
	for (; ((INT)i) <= (col + 1); column++, arr++)
	{
		if (!*column)
		{
			*arr = 0;
			continue;
		}
		
		if (start)
			*plen = (unsigned long) ((byte*)*column-start-1);
		
		start = (byte*)*column;
		plen = arr;

		i++;
	}
}

void 
InitGlobals(PGLOBALS pg)
{
    InitializeCriticalSection(&pg->m_csglobal);
	InitializeCriticalSection(&pg->m_csiniglobal);
	InitializeCriticalSection(&pg->m_cssshglobal);
    pg->m_colcount = 0;
    pg->m_conncount = 0;
    pg->m_entinst = NULL;
    pg->m_findreplace = wyFalse;
   // pg->m_helpfile[0] = '\0';//removing offline helpfile from version 12.05
    pg->m_hinstance = NULL;
    pg->m_hmapfile = NULL;
    pg->m_hwndclient = NULL;
    pg->m_isautocomplete = wyFalse;
	pg->m_isrefreshkeychange = wyFalse;
    pg->m_isautocompletehelp = wyFalse;
    pg->m_isshowtooltip = wyFalse;
    pg->m_modulenamelength = 0;
    pg->m_pcmainwin = NULL;
    pg->m_psqlite = NULL;
    pg->m_sshdupport = 10000;
    pg->m_statusbarsections = 0;
    pg->m_tooltipdelay = 0;
    pg->m_windowsbuild = 0;
    pg->m_windowsmajorversion = 0;
    pg->m_windowsminorversion = 0;
 	pg->m_ispqaenabled = wyTrue;
	pg->m_ispqashowprofile = wyTrue;
    pg->m_pqaprofileenabled = wyFalse;
    pg->m_pqaprofilerbuffsize = -1; 
	pg->m_findcount = 0;
	pg->m_resuttabpageenabled = wyTrue;
	pg->m_retainpagevalue = wyTrue;
	pg->m_highlimitglobal = 1000;

	pg->m_wmnextproc = NULL;
	pg->m_iscustomwmnext = wyFalse;	

	pg->m_menurefreshobj = NULL;
	pg->m_menucurrentquery = NULL;
	pg->m_menuselectedquery = NULL;
	pg->m_menuallquery = NULL;
    pg->m_hiconlist = NULL;
    pg->m_istabledataunderquery = wyTrue;
    pg->m_isinfotabunderquery = wyTrue;	
    pg->m_ishistoryunderquery = wyFalse;
	pg->m_prefpersist = 0;
	pg->m_conrestore = wyFalse;
	pg->m_isannouncementopen = wyTrue;
	pg->m_pcquerywnd = NULL;
	pg->m_announcements = NULL;
	pg->m_mdiwlist = NULL;
	pg->m_sqliteobj = NULL;
	pg->m_issessionsaveactive = wyFalse;
	pg->m_sessionsavemutex = NULL;
	pg->m_sessionrestore = wyTrue;
    return;
}
    
void 
SetGroupProcess(MDIWindow *wnd, wyBool val)
{
    wnd->m_grpprocess = val;

    return;
}
// rebuild only alter db instead of rebuilding all databases
void
SpecificDBRebuild(wyChar *database)
{
	MDIWindow *wnd = NULL;
   
    wnd = GetActiveWin();            
    SetGroupProcess(wnd, wyFalse);

	if(pGlobals->m_isautocomplete == wyTrue)
		pGlobals->m_pcmainwin->m_connection->RebuildTags(wnd, database);

    return;
}

wyBool
RebuildACTags_SpecificDB(ConnectionInfo &src, const wyChar *db_name)
{
    MDIWindow *wnd = NULL;
    CTCITEM     item = {0};
    wyChar      *dbname = NULL;
    int i = 0;
    item.m_mask = CTBIF_LPARAM;
    
    if(pGlobals->m_isautocomplete == wyTrue)
    {   
        for(i = 0; i < pGlobals->m_conncount; i++)
        {
            CustomTab_GetItem(pGlobals->m_pcmainwin->m_hwndconntab, i, &item);
            wnd = (MDIWindow *)item.m_lparam;
        
            if(!src.m_user.Compare(wnd->m_conninfo.m_user) 
                    && src.m_port == wnd->m_conninfo.m_port && !src.m_host.Compare(wnd->m_conninfo.m_host))
            {
                if(src.m_isssh == wnd->m_conninfo.m_isssh && src.m_isssh == wyTrue)
                {
                    if(!src.m_sshhost.Compare(wnd->m_conninfo.m_sshhost) && src.m_sshport == wnd->m_conninfo.m_sshport
                        && !src.m_sshuser.Compare(wnd->m_conninfo.m_sshuser))
                    {
                        break;
                    }
                }
                else if( src.m_ishttp == wnd->m_conninfo.m_ishttp && src.m_ishttp == wyTrue)
                {
                    if(!src.m_url.Compare(wnd->m_conninfo.m_url))
                    {
                        break;
                    }
                }
                else if(src.m_issslchecked == wnd->m_conninfo.m_issslchecked && src.m_issslchecked == wyTrue)
                {
                    if(!src.m_cacert.Compare(wnd->m_conninfo.m_cacert) && !src.m_cipher.Compare(wnd->m_conninfo.m_cipher))
                    {
                        if(src.m_issslauthchecked == wnd->m_conninfo.m_issslauthchecked)
                        {
                            if(src.m_issslauthchecked == wyTrue)
                            {
                                if(!src.m_clicert.Compare(wnd->m_conninfo.m_clicert) && !src.m_clikey.Compare(wnd->m_conninfo.m_clikey))
                                {
                                    break;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                }
                else
                {
                    break;
                }
            }
        }
    
        if(i != pGlobals->m_conncount)
        {   
            dbname = _strdup(db_name);
            if(dbname)
            {
                SetGroupProcess(wnd, wyFalse);

    	        pGlobals->m_pcmainwin->m_connection->RebuildTags(wnd, dbname);
            
                free(dbname);
                return wyTrue;
            }
            else
            {
                return wyFalse;
            }
        }
    }

    return wyFalse;
}



void 
FetchFileNameFromCmdline(const wyChar *filename)
{
	wyString	file, fname;
	wyInt32		fileoptpos = -1, fnamestrtpos = -1, fnameendpos = -1;

	if(filename)
		file.SetAs(filename);   // fetch the filename from commandline argument
		
	fileoptpos = file.FindI("-f", 0);

	//If use '-f' option
	if(fileoptpos!= -1)
	{
		fnamestrtpos = file.FindI("\"", fileoptpos + 1);
		fnameendpos = file.FindI("\"", fnamestrtpos + 1);

		if(fnamestrtpos != -1 && fnameendpos != -1)        
			fname.SetAs(file.Substr(fnamestrtpos + 1 , fnameendpos - (fnamestrtpos + 1)));

		else if(file.GetLength() > fileoptpos + strlen("-f"))            
			fname.SetAs(file.Substr(fileoptpos + strlen("-f"), file.GetLength()));

		file.SetAs(fname);
	}

	// If use with out -f but within double quotes
	else if(file.FindI("\"", 0) != -1)
	{
		fnamestrtpos = file.FindI("\"", 0);
		fnameendpos = file.FindI("\"", fnamestrtpos + 1);		

		if((fnameendpos - (fnamestrtpos + 1)) >  fnamestrtpos + 1) 
			fname.SetAs(file.Substr(fnamestrtpos + 1, fnameendpos - (fnamestrtpos + 1)));

		file.SetAs(fname);
	}
				
	//Just the file name without double quotes
	if(file.GetLength() != 0)
	{
		file.LTrim();
		file.RTrim();
		pGlobals->m_filename.SetAs(file); // stores the file name in PGolbals variable	
	}
}

wyInt32
SetComboWidth(HWND hwndCombo)
{
	HDC hdc;
	wyInt32 i, count,width=0;
	wyWChar Item[512] = {0};
	SIZE    size;
	HFONT font,oldfont;

	count = SendMessage(hwndCombo, CB_GETCOUNT, 0, 0);
	hdc = GetWindowDC(hwndCombo);
	//hdc = GetDC(hwndCombo);
	font = GetWindowFont(hwndCombo);
	oldfont = (HFONT)SelectObject(hdc,font);

	for(i=0;i< count; i++)
	{	
		SendMessage(hwndCombo,CB_GETLBTEXT ,i,(LPARAM)Item);
		GetTextExtentPoint32(hdc,(LPCWSTR)Item, wcslen(Item), &size);
		width = (width<size.cx)?size.cx:width;
	}
	
	return width;
}

void SetFocusToEditor(MDIWindow *wnd, HWND hwnd)
{
	//issue reported here http://code.google.com/p/sqlyog/issues/detail?id=192 .
	//Execute one big query.Open another connection and type some characters. after executing the query in the first connection the characters you are typing is coming to first connection query tab.
	// this is because of after executing the query in first connection focus is going to first connection query tab.
	if(IsEditorFocus())
	{
		if(wnd == GetActiveWin()) 
			PostMessage(hwnd, UM_FOCUS, 0, 0); //setting focus to the query editor
		else
			wnd->m_lastfocus = hwnd; //if we open a new connection while executing,we r setting the last focus
	}

}

wyBool
IsRefreshTableData()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH + 1]= {0};
    wyInt32     ret;
	wyString	dirstr;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "RefreshTableData", 0, dirstr.GetString());

	return(ret)?(wyTrue):(wyFalse);
}

//Setting the size of the tool bar icons
wyInt32
GetToolBarIconSize()
{
	wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH+1] = {0};
	wyInt32		iconsize = ICON_SIZE_24;
	wyString	dirstr;
	wyString	iconsizestr;

	//Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	
	wyIni::IniGetString(GENERALPREFA, "ToolIconSize", TOOLBARICONSIZE_DEFAULT, &iconsizestr, dirstr.GetString());

	if(iconsizestr.CompareI("Large") == 0)
		iconsize = ICON_SIZE_32;
	else if(iconsizestr.CompareI("Small") == 0)
		iconsize = ICON_SIZE_24;
	else
		iconsize = ICON_SIZE_28;
	   
	return iconsize;
}

///checks for Whether we will use sapces for tabs or not.
wyBool
IsInsertSpacesForTab()
{
    wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH+1]= {0};
    wyString	dirstr;
    wyInt32     ret;
	
	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(GENERALPREFA, "InsertSpacesForTab", 0, dirstr.GetString());

    return ret?wyTrue:wyFalse;
}

wyBool
IsRetainColumnWidth()
{
    wyInt32     isretain;
	wyWChar     *lpfileport = 0;
	wyWChar     directory[MAX_PATH+1] = {0};
	wyString	dirstr;
	
	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini",MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	isretain = wyIni::IniGetInt(GENERALPREFA, "RetainColumnWidth", 1, dirstr.GetString());

	return isretain?wyTrue:wyFalse;
}

wyBool
IsASAndAliasAlignment()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH+1]= {0};
    wyInt32     ret;
	wyString	dirstr;
	

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(FORMATTERPREFA , "ASAndAliasAlignment", ASANDALIASALIGN_DEFAULT, dirstr.GetString());

	return(ret)?(wyTrue):(wyFalse);
}
wyBool
IsStacked()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH+1]= {0};
    wyInt32     ret;
	wyString	dirstr;
	
	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(FORMATTERPREFA , "Stacked", STACKED_DEFAULT, dirstr.GetString());

	return(ret)?(wyTrue):(wyFalse);
}

wyInt32
GetLineBreak()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH+1]= {0};
    wyInt32     ret;
	wyString	dirstr;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(FORMATTERPREFA , "Linebreak", LINEBREAK_DEFAULT, dirstr.GetString());

	return ret;
}
wyInt32
GetIndentation()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH+1]= {0};
    wyInt32     ret;
	wyString	dirstr;

	// Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	ret = wyIni::IniGetInt(FORMATTERPREFA , "Indentation", INDENTATION_DEFAULT , dirstr.GetString());

	return ret;
}

/*find the string having even no of back ticks or odd number of backticks.
if number of backticks are even return wyTrue else wyFalse.
*/
wyBool 
IsEvenBackticks(const wyChar *str, wyChar chartype)
{
	wyInt32   i, len;
	wyInt32   nobackticks = 2;
	wyString  query;

	query.SetAs(str);
	len = query.GetLength();
	
	for(i = 0; i < len; i++)
	{
		if(query.GetCharAt(i) == chartype)
			nobackticks++;
	}

	return (nobackticks%2 == 0? wyTrue: wyFalse);
}

//function for matching the given string with specified pattern
wyInt32 
MatchStringPattern(const wyChar*subject, wyChar *pattern, wyInt32 *ovector, wyInt32 complieoptions)
{
	pcre           *re;
	wyInt32         erroffset, rc = -1;//, i = 0;
	wyInt32         subject_length;	
	const char      *error;
	wyString        tempstr;

	subject_length = (wyInt32)strlen(subject);

	re = pcre_compile(
		pattern,              /* the pattern */
		complieoptions,/* default options */ //match is a case insensitive 
		&error,               /* for error message */
		&erroffset,           /* for error offset */
		NULL);                /* use default character tables */

	/* Compilation failed: print the error message and exit */

	if (re == NULL)
		return 0;

	/*************************************************************************
	* If the compilation succeeded, we call PCRE again, in order to do a     *
	* pattern match against the subject string. This does just ONE match *
	*************************************************************************/

	rc = pcre_exec(
	re,                   /* the compiled pattern */
	NULL,                 /* no extra data - we didn't study the pattern */
	subject,              /* the subject string */
	subject_length,       /* the length of the subject */
	0,                    /* start at offset 0 in the subject */
	PCRE_NO_UTF8_CHECK,             /* default options */
	ovector,              /* output vector for substring information */
	OVECCOUNT);           /* number of elements in the output vector */

	if(re)
		pcre_free(re);

	return rc;
}

wyInt32
IsMatchStringPattern(const wyChar *subject, wyChar* pattern , wyInt32 pcrecompileopts)
{
	wyInt32   regexret = 0;				 // pcre_exec return values.
	wyInt32   ovector[OVECCOUNT];       // output vector for substring information 	

	
	regexret = MatchStringPattern(subject, pattern, ovector, pcrecompileopts);

	if(regexret < 0)	
		return -1;
	else
		return 1;
	
}

/* get number of chars inside open and its corresponding close
paranthesis . if corresponding paranthesis is not found take up to 
end of the query. if paranthesis are there inside back quotes 
ignore it */
wyInt32
GetStrLenWithInParanthesis(const wyChar *str, wyBool isformatter)

{
	wyString    strtemp;
	wyInt32     openbraces = 1, closebraces = 0; 
	wyInt32     i =0, len;
	wyInt32     nobackticks = 2;

	strtemp.SetAs(str);
	len = strtemp.GetLength();	

	while(1)
	{
		if(openbraces == closebraces)
			return(isformatter == wyTrue?(i - 1):(i - 2));	

		//if corresponding paranthesis is not found return -1 value
		if(len - i == 0)
			return(isformatter == wyTrue? -1: i - 1);	

		if(strtemp.GetCharAt(i) == '`')
			nobackticks++;
		
		if((nobackticks %2) == 1)
		{
			i++;
			continue;
		}	

		if(strtemp.GetCharAt(i) == '(')
			openbraces++;
		else if(strtemp.GetCharAt(i) == ')')
			closebraces++;				
		
		i++;
	}	
}

wyBool 
AddXMLToBuffer(wyString * buffer, const wyChar * text, wyBool crlf)
{
	while(*text)
	{
		switch(*text)
		{
		case '<':
			buffer->Add("&lt;");
			break;

		case '>':
			buffer->Add("&gt;");
			break;

		case '&':
			buffer->Add("&amp;");
			break;

		case '\"':
			buffer->Add("&quot;");
			break;

		default:
			if(*text == '\r' && *(text+1)== '\n')
			{
				if(crlf == wyTrue)
				buffer->Add("<br>");
				//fixed a bug, export as html was truncating data
				//text += 2;
				else
				buffer->Add("&#10;");
				text ++;
			} 
			else
			{
				buffer->AddSprintf("%c", *text);
			}
			
			break;						
		}
		
		text++;
	}
	
	return wyTrue;
}

//Setting the width of the listbox
wyInt32
SetListBoxWidth(HWND hwndlist)
{
	HDC			hdc;
	wyInt32		i, count, width = 0;
	wyWChar		item[512] = {0};
	SIZE		size;
	HFONT		font, oldfont;
	
	count   = SendMessage(hwndlist, LB_GETCOUNT, 0, 0);
	hdc	    = GetWindowDC(hwndlist);
	font    = GetWindowFont(hwndlist);
	oldfont = (HFONT)SelectObject(hdc, font);
	
	for(i = 0; i < count; i++)
	{	
		SendMessage(hwndlist, LB_GETTEXT, i, (LPARAM)item);
		GetTextExtentPoint32(hdc, (LPCWSTR)item, wcslen(item), &size);
		width = (width < size.cx) ? size.cx : width;
	}
	
	return width;
}

//Show warnings in messages tab
wyBool
IsShowWarnings()
{
	wyWChar     *lpfileport = 0;
	wyWChar     directory[MAX_PATH + 1]= {0};
    wyInt32     ret;
	wyString	dirstr;
	
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	
	//getting show warnings status from init File
	ret = wyIni::IniGetInt(GENERALPREFA, "ShowWarnings", 0, dirstr.GetString());
	return(ret)?(wyTrue):(wyFalse);

}

wyBool
IsHaltExecutionOnError()
{
	wyWChar     *lpfileport = 0;
	wyWChar     directory[MAX_PATH + 1]= {0};
    wyInt32     ret;
	wyString	dirstr;
	
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	
	//getting show warnings status from init File
	ret = wyIni::IniGetInt(GENERALPREFA, "HaltExecutionOnError", 0, dirstr.GetString());
	return(ret)?(wyTrue):(wyFalse);

}

// Function to write xml tags
wyBool
WriteXMLtags(HANDLE hfile, wyString *tags)
{
	DWORD	dwbyteswritten;
	BOOL	retval;

	retval = WriteFile(hfile, tags->GetString(), tags->GetLength(), &dwbyteswritten, NULL);
	if(retval == FALSE)
	{
		if(hfile)
			VERIFY(CloseHandle(hfile));	

		return wyFalse;
	}

	return wyTrue;
}

//Handle refreshing the objectbrowser
void
ObjectBrowserRefresh(wyWChar *database)
{
	MDIWindow	*wnd;

	VERIFY(wnd = GetActiveWin());

	if(!database) 
		return;

	wnd->m_pcqueryobject->m_seltype = NDATABASE;

	SendMessage(wnd->m_pcqueryobject->m_hwnd, WM_SETREDRAW, FALSE, 0);
	wnd->m_pcqueryobject->GetTreeState();
	wnd->m_pcqueryobject->RefreshObjectBrowser(wnd->m_tunnel, &wnd->m_mysql,wnd);
	wnd->m_pcqueryobject->RestoreTreeState();
	wnd->m_pcqueryobject->m_seldatabase.SetAs(database); 
	wnd->m_pcqueryobject->ExpandDatabase();
	SendMessage(wnd->m_pcqueryobject->m_hwnd, WM_SETREDRAW, TRUE, 0);	
    
    //send the paint message
    InvalidateRect(wnd->m_pcqueryobject->m_hwnd, NULL, FALSE);
    UpdateWindow(wnd->m_pcqueryobject->m_hwnd);
}

// Gets the list of tables/views in a given database
wyInt32
DatabaseTablesGet(const wyChar* database, List* dbtablelist, HWND hwndparent, wyBool isview)
{
	wyInt32			count = 0, ret;
	wyString		query, myrowstr, myrowstr1, dbname;
	MYSQL_RES		*myres;
	MYSQL_ROW		myrow;
	MDIWindow		*wnd;
	wyBool			ismysql41;
	RelTableFldInfo	*tablenameelem = NULL;

	wnd = GetActiveWin();

    if(!wnd)
        return -1;

    ismysql41 = wnd->m_ismysql41;
	dbname.SetAs(database);
    
    VERIFY(SetCursor(LoadCursor(NULL, IDC_WAIT)));

    if(isview == wyFalse)
	    ret = PrepareShowTable(wnd->m_tunnel, &wnd->m_mysql, dbname, query);
    else
        ret = GetSelectViewStmt(dbname.GetString(), query);
	
	myres = ExecuteAndGetResult(GetActiveWin(), wnd->m_tunnel, &wnd->m_mysql, query);

	if(!myres)
	{
        ShowMySQLError(hwndparent, wnd->m_tunnel, &wnd->m_mysql, query.GetString());
		return -1;
	}

	while(myrow = wnd->m_tunnel->mysql_fetch_row(myres))
	{
		myrowstr.SetAs(myrow[0], ismysql41);
		tablenameelem = new RelTableFldInfo(myrowstr.GetString());

		dbtablelist->Insert(tablenameelem);		

		count += 1;
	}		
	
	if(myres)
		wnd->m_tunnel->mysql_free_result(myres);

    VERIFY(SetCursor(LoadCursor(NULL, IDC_ARROW)));
	return 0;    
}


// Functions to get and write limit values from and to SQLyog.ini
wyBool 
GetLimitValues()
{
	wyWChar     *lpfileport = 0;
	wyWChar     directory[MAX_PATH + 1]= {0};
	wyInt32     ret;
	wyString	dirstr;
	
	ret = SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	if(ret == 0)
		return wyFalse;
	
	dirstr.SetAs(directory);
   
	pGlobals->m_lowlimitglobal =  0;
	pGlobals->m_highlimitglobal =  wyIni::IniGetInt(GENERALPREFA, "ResulttabPageRows", 1000, dirstr.GetString());

	if(!pGlobals->m_highlimitglobal)
		pGlobals->m_highlimitglobal = 1000;
	
	return wyTrue;
}

wyBool
IsResultTabPagingEnabled()
{
	wyWChar     *lpfileport = 0;
	wyWChar     directory[MAX_PATH + 1]= {0};
    wyInt32     ret;
	wyString	dirstr;
	
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	
	//getting show warnings status from init File
	ret = wyIni::IniGetInt(GENERALPREFA, "ResuttabPaging", 1, dirstr.GetString());
	pGlobals->m_resuttabpageenabled = (ret ? wyTrue : wyFalse);

	//Retain page value option
	ret = wyIni::IniGetInt(GENERALPREFA, "ResuttabRetainsPage", 1, dirstr.GetString());
	pGlobals->m_retainpagevalue = (ret ? wyTrue : wyFalse);

	return(ret)?(wyTrue):(wyFalse);

}

void
GetTabPositions()
{
    wyWChar     *lpfileport = 0;
	wyWChar     directory[MAX_PATH + 1]= {0};
    wyInt32     ret;
	wyString	dirstr;
	
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	
	ret = wyIni::IniGetInt(GENERALPREFA, "TableDataUnderQuery", 1, dirstr.GetString());
    pGlobals->m_istabledataunderquery = (ret ? wyTrue : wyFalse);

	ret = wyIni::IniGetInt(GENERALPREFA, "InfoTabUnderQuery", 1, dirstr.GetString());
    pGlobals->m_isinfotabunderquery = (ret ? wyTrue : wyFalse);

    ret = wyIni::IniGetInt(GENERALPREFA, "HistoryUnderQuery", 0, dirstr.GetString());
    pGlobals->m_ishistoryunderquery = (ret ? wyTrue : wyFalse);
}
