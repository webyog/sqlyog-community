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

#ifdef _WIN32
#include <shlobj.h>
#include <mlang.h>
#include <Tlhelp32.h>
#include <iomanip>
//#include "wyIni.h"
#else
 #include <unistd.h>
#endif
#include <iomanip>
#include "modes.h"
#include "aes.h"
#include "filters.h"
#include <string>
#include <stddef.h>
#include <assert.h>
#include "pcre.h"
#include "Verify.h"
#include "AppInfo.h"
#include "CommonHelper.h" 
#include "L10nText.h"
#include "SQLTokenizer.h"
#ifdef COMMUNITY 
#include "Global.h"
#else
#include "CommonJobStructures.h"
#endif

#include "MySQLVersionHelper.h" 

#if defined WIN32 && ! defined _CONSOLE
#include "GUIHelper.h" 
#include "FrameWindowHelper.h"
extern	PGLOBALS		pGlobals;
#else
	#include "Main.h"
	
#endif

extern	FILE	*logfilehandle;

#include "ClientMySQLWrapper.h"
//#include "Verify.h"
#include "Tunnel.h"

#if ! defined COMMUNITY && defined _WIN32
#include "wyFile.h"
#include "HelperEnt.h"
#endif

#ifdef COMMUNITY
#include "TunnelCommunity.h"
#else
#include "TunnelEnt.h"
#endif

static wyChar table64[]=
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static CryptoPP::byte AESKey[16] = { };//provide any Key
static CryptoPP::byte AESiv[16] = {}; //Provide any IV

Tunnel * 
CreateTunnel(wyBool istunnel)
{
#ifdef COMMUNITY
    return(new TunnelCommunity());
#else
    return(new TunnelEnt((istunnel == wyTrue)?true:false));
#endif
}

/* Create proper timestamp to create the file name */
void
GetTimeString(wyString& buff, wyChar *timesep)
{
	tm			*newtime;
	time_t		aclock;
	
	time(&aclock);
	newtime = localtime(&aclock);

	buff.Sprintf("%04d-%02d-%02d %02d%s%02d%s%02d", 
							1900 + newtime->tm_year,
							newtime->tm_mon + 1,
							newtime->tm_mday,
							newtime->tm_hour,
							timesep,
							newtime->tm_min,
							timesep,
							newtime->tm_sec
							);
}

// mm
void YogDebugLog(int errcode, const char* msg, char* file, int line)
{
#ifdef _WIN32
//#ifdef _DEBUG
	wyWChar		directory[MAX_PATH+1];
	wyWChar		*lpfileport;
	wyBool		ret;
        
	ret = SearchFilePath(L"sqlyog_logx", L".log", MAX_PATH, directory, &lpfileport);

	if(ret != wyTrue) 
	{
		return;
	}

	wyString buffer;
	wyString strtime;
	GetTimeString(strtime, ":");
	buffer.Sprintf("[%s] %s [F]: %s (%d), [E]: %d, [M]: %s",  APPVERSION, 
															  strtime.GetString(), 
															  (file?file:"(n/a)"), 
															  line, 
															  errcode,
															  (msg?msg:"(n/a)")
															);

	FILE* fp = _wfopen(directory, L"a+");
	if(!fp) 
	{
		return;
	}

	fprintf(fp, "%s\r\n", buffer.GetString());
	fclose(fp);
	
#else
	/*UNREFERENCED_PARAMETER(msg);
	UNREFERENCED_PARAMETER(file);
	UNREFERENCED_PARAMETER(line);*/
#endif
//#endif
}

wyBool 
SearchFilePath(wyWChar *filename, wyWChar *extension, wyInt32 bufferlength, 
                                                wyWChar *buffer, wyWChar **lpfileport)
{

#ifdef _WIN32
	wyWChar      path[MAX_PATH + 1], fullpath[MAX_PATH + 1];
	wyString    strpath, fullpathstr;
	FILE        *fp;
	wyString	filenamestr, extensionstr;
	wyBool		issqlyogexefiles = wyFalse;
	wyBool		isdefaultconfigpath = wyFalse;

#ifndef _CONSOLE
	if(pGlobals->m_configdirpath.GetLength())
		isdefaultconfigpath = wyTrue;
#endif

	//Check the file name to serch is SQLyogEnt.exe, SJA.exe, or SQLyog.exe
	if(!filename)
		return wyFalse;

	//Set flag to wyTrue if the file name to search is .exe
	issqlyogexefiles = CheckSQLyogFiles(filename);
    
	/*Look files other than .exe in AppData Folder.
	.exe files looks into Installed folder only*/
	if(issqlyogexefiles == wyFalse)
	{
		filenamestr.SetAs(filename);	
		extensionstr.SetAs(extension);

		if(isdefaultconfigpath == wyFalse)
		{
			if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, fullpath)))
			{
				fullpathstr.SetAs(fullpath);
				fullpathstr.Add("\\SQLyog");

				if(extension && !wcsicmp(extension, L".ini"))
				{					
					strpath.Sprintf("%s%s", filenamestr.GetString(), extensionstr.GetString());
					fullpathstr.AddSprintf("\\%s", strpath.GetString());
					
					//wcscpy(buffer, fullpathstr.GetAsWideChar());					
					wcsncpy(buffer, fullpathstr.GetAsWideChar(), bufferlength - 1);					
					buffer[bufferlength - 1] = '\0';

					return wyTrue;
				}				
			}

			else
				return wyFalse;
		}
#ifndef _CONSOLE
		else
		{
			fullpathstr.SetAs(pGlobals->m_configdirpath);
			
			if(extension && !wcsicmp(extension, L".ini"))
			{
				fullpathstr.AddSprintf("\\%s%s", filenamestr.GetString(), extensionstr.GetString());

				//wcscpy(buffer, fullpathstr.GetAsWideChar());
				wcsncpy(buffer, fullpathstr.GetAsWideChar(), bufferlength - 1);					
				buffer[bufferlength - 1] = '\0';

				return wyTrue;
			}
		}
#endif
	}

    // First search in App folder. For exe, it looks into Installed folder only
	if(issqlyogexefiles == wyTrue || !SearchPath(fullpathstr.GetAsWideChar(), filename, extension, bufferlength, buffer, lpfileport))
    {
        // If not found search in installation folder
		if(SearchPath(NULL, filename, extension, bufferlength, buffer, lpfileport) == 0)
        {   
            /*Look files other than .exe in AppData Folder.
			.exe files looks into Installed folder only*/
			if(issqlyogexefiles == wyTrue)
	            return wyFalse;

			else if(fullpathstr.GetLength())
				wcscpy(buffer, fullpathstr.GetAsWideChar());

		    if(extension)
		    {
			    wcscat(buffer, extension);
			    filenamestr.SetAs(filename);	
			    extensionstr.SetAs(extension);
			    strpath.Sprintf("%s%s", filenamestr.GetString(), extensionstr.GetString());
                fullpathstr.AddSprintf("\\%s", strpath.GetString());

                if(CheckSQLyogFiles(strpath.GetAsWideChar()) == wyTrue)
	                return wyFalse;
		    }
        	
            fp = _wfopen(fullpathstr.GetAsWideChar(), L"a");

		    if(fp != NULL)
		    {
                fullpathstr.Strip(strpath.GetLength());
                SearchPath(fullpathstr.GetAsWideChar(), filename, extension, bufferlength, buffer, lpfileport);
			    fclose(fp);	
			    return wyTrue;
		    }

#ifndef _CONSOLE

			if(isdefaultconfigpath == wyTrue)
			{
				//wcscpy(path, pGlobals->m_configdirpath.GetAsWideChar());
				wcsncpy(path, pGlobals->m_configdirpath.GetAsWideChar(), MAX_PATH);
				path[MAX_PATH] = '\0';
			}
            			
		    else if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
		    {
			    wcscat(path, L"\\SQLyog");
			}
#endif
        		
			if(!SearchPath(path, filename, extension, bufferlength, buffer, lpfileport))
			{
				/*	if the file is not there(normally .ini file must be there , but .log or session file may not, 
					so we will create a dummy file */

                if(CheckFileExists(buffer, path, filename, extension) == wyFalse)
                    return wyFalse;
			}
		    
		}

		else
        {
			/*Look files other than .exe in AppData Folder.
			.exe files looks into Installed folder only. Return wyTrue because the .exe path is found*/
            if(issqlyogexefiles == wyTrue)
				return wyTrue;

		    if(extension)
		    {
			    filenamestr.SetAs(filename);
			    extensionstr.SetAs(extension);
			    strpath.Sprintf("%s%s", filenamestr.GetString(), extensionstr.GetString());
        	
		        if(CheckSQLyogFiles(strpath.GetAsWideChar()) == wyTrue)
			        return wyTrue;
		    }
        
		    fp = _wfopen(buffer, L"r+");

		    if(fp == NULL)
		    {
			    /* if the file with the specified condition doesn't exists?*/
			    if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
			    {
				    wcscat(path, L"\\SQLyog");
        		
				    if(!SearchPath(path, filename, extension, bufferlength, buffer, lpfileport))
				    {
					    /*	if the file is not ther(normally .ini file must be ther , but .log or session file may not, 
						    so we will create a dummy file */
                        if(CheckFileExists(buffer, path, filename, extension) == wyFalse)
                            return wyFalse;
				    }
			    }		
		    }
		    else
			    fclose(fp);
        }
	}
	else
	{
		/*Look files other than .exe in AppData Folder.
		.exe files looks into Installed folder only*/
		if(issqlyogexefiles == wyTrue)
			return wyTrue;

		if(extension)
		{
			filenamestr.SetAs(filename);
			extensionstr.SetAs(extension);
			strpath.Sprintf("%s%s", filenamestr.GetString(), extensionstr.GetString());
    	
		    if(CheckSQLyogFiles(strpath.GetAsWideChar()) == wyTrue)
			    return wyTrue;
		}
    
		fp = _wfopen(buffer, L"r+");

		if(fp == NULL)
		{
			/* if the file with the specified condition doesn't exists?*/
			if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, path)))
			{
				wcscat(path, L"\\SQLyog");
    		
				if(!SearchPath(path, filename, extension, bufferlength, buffer, lpfileport))
				{
					/*	if the file is not ther(normally .ini file must be ther , but .log or session file may not, 
						so we will create a dummy file */
                    if(CheckFileExists(buffer, path, filename, extension) == wyFalse)
                        return wyFalse;
				}
			}		
		}
		else
			fclose(fp);
	}
#endif
	return wyTrue;
}

#ifdef _WIN32
wyBool 
CheckSQLyogFiles(const wyWChar *filename)
{
     if((wcsicmp(filename, L"sja.exe")== 0)||
	    (wcsicmp(filename, L"sqlyog.exe")== 0)||
	    (wcsicmp(filename, L"sqlyogent.exe")== 0))
				return wyTrue;
     return wyFalse;
}
#endif

wyBool 
CheckFileExists(wyWChar *buffer, const wyWChar *path, const wyWChar *filename, const wyWChar *extension)
{
    FILE    *fp;

	wcscpy(buffer, path);
	wcscat(buffer, L"\\");
	wcscat(buffer, filename);

    if(extension)
		wcscat(buffer, extension);
#ifdef _WIN32
    fp = _wfopen(buffer, L"a");
#else
    wyString buff;

    buff.SetAs(buffer);
    fp = fopen(buff.GetString(), "a");
#endif

	if(fp == NULL)
		return wyFalse;

	fclose(fp);

    return wyTrue;
}


wyInt32
CopyTableFromNewToOld(Tunnel * tunnel, PMYSQL mysql, Tunnel * newtargettunnel, 
					  PMYSQL newtargetmysql, const wyChar *db, const wyChar *table, wyString & query)
/* issue in Char set. it will return 0 if success, 1 if failure in source, 2 if failure in target */

{
    wyString        strcreate, indexname, indexcolumnname, tmpindex;
    wyInt32         typeindex = 0, nameindex = 0;
	MYSQL_RES	    *myres;
    MYSQL_ROW       myrow;
	wyBool	        isunique = wyFalse, isfulltext = wyFalse;

	query.Sprintf("show full fields from `%s`.`%s`", db, table);
    myres = SjaExecuteAndGetResult(tunnel, mysql, query);
	if(!myres)
		return 1;
		
    strcreate.Sprintf("Create table `%s`(", table);
    GetFieldInfoString(tunnel, myres, strcreate, newtargettunnel, newtargetmysql);
	sja_mysql_free_result(tunnel, myres);
		
	query.Sprintf("show keys from `%s`.`%s` ", db, table);

    myres = SjaExecuteAndGetResult(tunnel, mysql, query);
	if(!myres)
		return 1;

	while(myrow = sja_mysql_fetch_row(tunnel, myres))
	{
        tmpindex.SetAs(myrow[GetFieldIndex(tunnel, myres, "key_name")]);

		if(indexname.GetLength() && indexname.Compare(myrow[GetFieldIndex(tunnel, myres, "key_name")]) == 0)
			indexcolumnname.AddSprintf("`%s`,", myrow[GetFieldIndex(tunnel, myres, "column_name")]);
		else
		{
			if(indexname.GetLength() == 0)
			{
				indexname.SetAs(myrow[GetFieldIndex(tunnel, myres, "key_name")]);
				indexcolumnname.Sprintf("`%s`,", myrow[GetFieldIndex(tunnel, myres, "column_name")]);

				// check whether its unique.
				if(stricmp(myrow[GetFieldIndex(tunnel, myres, "non_unique")], "0") == 0)
					isunique = wyTrue;

				if(IsMySQL402(tunnel, mysql))
                {
					if(myrow[GetFieldIndex(tunnel, myres, "index_type")] && 
                       strstr(myrow[GetFieldIndex(tunnel, myres, "index_type")], "FULLTEXT"))
						isfulltext = wyTrue;
				}
			}
			else
			{
				if(indexname.GetLength())
				{
					indexcolumnname.Strip(1);
					strcreate.Add(",");

					if(indexname.CompareI("PRIMARY") == 0)
						strcreate.Add("primary key ");
					else
					{	
						if(isunique == wyTrue)
							strcreate.Add("unique ");
						else if(isfulltext == wyTrue)
							strcreate.Add("fulltext ");
						else
							strcreate.Add("key ");

						strcreate.AddSprintf("`%s`", indexname.GetString());
					}

					strcreate.AddSprintf("(%s)", indexcolumnname.GetString());

					indexname.Clear();
                    indexcolumnname.Clear();
					isunique   =   wyFalse;
					isfulltext =   wyFalse;

					// now copy this key into the buffer.
					indexcolumnname.Sprintf("`%s`,", myrow[GetFieldIndex(tunnel, myres, "column_name")]);
					indexname.SetAs(myrow[GetFieldIndex(tunnel, myres,"key_name")]);

					if(stricmp(myrow[GetFieldIndex(tunnel, myres, "non_unique")], "0") == 0)
						isunique = wyTrue;

					if(IsMySQL402(tunnel, mysql) && myrow[GetFieldIndex(tunnel, myres, "index_type")] && 
                        strstr(myrow[GetFieldIndex(tunnel, myres, "index_type")], "FULLTEXT"))
						isfulltext = wyTrue;
				}
			}
		}
	}

	if(indexname.GetLength() > 0)
	{
		indexcolumnname.Strip(1);
		strcreate.Add(",");

		if(indexname.CompareI("PRIMARY") == 0)
			strcreate.Add("primary key ");
		else
		{
			if(isunique == wyTrue)
				strcreate.Add("unique ");
			else if(isfulltext == wyTrue)
				strcreate.Add("fulltext ");
			else
				strcreate.Add("key ");

			strcreate.AddSprintf("`%s`", indexname.GetString());
		}

		strcreate.AddSprintf("(%s)", indexcolumnname.GetString());
	}

	sja_mysql_free_result(tunnel, myres);
	query.Sprintf("show table status from `%s`", db);
    myres = SjaExecuteAndGetResult(tunnel, mysql, query);
	if(!myres)
		return 1;

	typeindex = (IsMySQL41(tunnel, mysql)? 
                GetFieldIndex(tunnel, myres, "engine") : 
                GetFieldIndex(tunnel, myres, "Engine"));

	nameindex = GetFieldIndex(tunnel, myres, "Name");
	strcreate.Add(")");
    
	while((myrow = sja_mysql_fetch_row(tunnel, myres))!= NULL)
	{
		if(stricmp(myrow[nameindex], table) == 0)
		{
            if(IsMySQL4(newtargettunnel, newtargetmysql) == wyTrue)
			    strcreate.AddSprintf("%s%s", 
                    (IsMySQL41(newtargettunnel, newtargetmysql)? "Engine = " :  "Type = "), 
                    myrow[typeindex]);

			break;
		}
	}
	
	if(myres)
		sja_mysql_free_result(tunnel, myres);
  
	query.SetAs(strcreate);

	return 0;
}

void 
GetFieldInfoString(Tunnel *tunnel, MYSQL_RES *myres, 
                   wyString &strcreate, Tunnel *tgttunnel, PMYSQL tgtmysql)
{
    MYSQL_ROW   myrow;
    wyBool      first = wyTrue;
    wyInt32     nfieldval, ntypeval,nnullval; 
    wyInt32     ndefval, nextraval, ncommentval;
	wyString	type;

    nfieldval	= GetFieldIndex(tunnel, myres, "field");
    ntypeval	= GetFieldIndex(tunnel, myres, "type");
	nnullval	= GetFieldIndex(tunnel, myres, "null");
	ndefval		= GetFieldIndex(tunnel, myres, "default");
	nextraval	= GetFieldIndex(tunnel, myres, "extra");
	ncommentval = GetFieldIndex(tunnel, myres, "comment");

	while(myrow = sja_mysql_fetch_row(tunnel, myres))
	{
		if(first == wyFalse)
			strcreate.Add(", ");

		if(myrow[nfieldval])
			strcreate.AddSprintf("`%s` ", myrow[nfieldval]);
		
        if(myrow[ntypeval])
		{
			type.SetAs(myrow[ntypeval]);
			strcreate.AddSprintf("%s ", myrow[ntypeval]);
		}

		if(stricmp(myrow[nnullval],"YES") == 0)
			strcreate.Add("NULL ");
		else
			strcreate.Add("NOT NULL ");

		if(myrow[ndefval])
			strcreate.AddSprintf(" default '%s' ", myrow[ndefval]);

		if(myrow[nextraval])
		{
			if(type.CompareI("timestamp") == 0)
			{
				//In 5.1 server If type is timestamp then Extra info contains ON UPDATE CURRENT_TIMESTAMP
				//this is added in 4.1. if server version is > 4.1 then only we are adding On update clause to create table stmt
				if(IsMySQL41(tgttunnel, tgtmysql) == wyTrue)
					strcreate.AddSprintf("%s ", myrow[nextraval]);
			}
			else
				strcreate.AddSprintf("%s ", myrow[nextraval]);
		}

		if(myrow[ncommentval] && IsMySQL41(tgttunnel, tgtmysql))
			strcreate.AddSprintf(" comment '%s' ", myrow[ncommentval]);
		
        first = wyFalse;
	}    
}


/* function only required in SQLyog GUI as its already defined in SQLyogJob */
wyInt32 
GetFieldIndex(Tunnel *tunnel, MYSQL_RES * result, wyChar * colname)
{
	wyUInt32        num_fields;
	wyUInt32        count;
	MYSQL_FIELD		*fields;
	wyString		myfieldstr;
	//wyBool			ismysql41 = ((GetActiveWin())->m_ismysql41);

	num_fields	= sja_mysql_num_fields(tunnel, result);
	fields		= sja_mysql_fetch_fields(tunnel, result);

	for(count = 0; count < num_fields; count++)
	{
		myfieldstr.SetAs(fields[count].name);

		if(0 == (myfieldstr.CompareI(colname)))
			return count;
	}

//    assert(0);
    
	return -1;
}
wyInt32 GetBodyOfTrigger(wyString *body )
{
int index;
wyString result;
//Find the index of string "for each row"
index=body->FindI("for each row");
if(index!=-1)
//now body will be statrting of string+index+lenght of "for each row"+1
{
result.SetAs(body->GetString()+index+13);
body->Clear();
body->SetAs(result);
return 1;
}
else return -1;
}
#ifdef _WIN32
wyBool 
GetCreateTriggerString(HWND hwnd, Tunnel * tunnel, PMYSQL mysql, const wyChar *db, const wyChar *trigger, wyString &strtrigger, wyString &strmsg, wyBool isdefiner)
{
	wyInt32		pos;
	wyChar		*ch = NULL;
	wyString    deftemp, def, query,query1, definer,bodyoftrigger;
	MYSQL_RES	*myres,*myres1;
	MYSQL_ROW	myrow,myrow1=NULL;
	wyInt32		coldefiner = -1; 
	wyInt32		coltablename = -1;
	wyInt32		colevent = -1;
	wyInt32		colstmt = -1;
	wyInt32		coltiming = -1;
	wyBool      isshowcreateworked=wyTrue;
    //There are mutiple issues with show triggers and show create trigger in MySQL servers--
	//with show create trigger there are following issues-
	//1-SHOW CREATE TRIGGER was added in MySQL 5.1.21 so it will not work for older versions
	//2-This bug reprot-http://bugs.mysql.com/bug.php?id=58996
	//With show triggers there are following issues--
	//1-Curruption of quotes refer-http://forums.webyog.com/index.php?showtopic=7625&hl= and http://bugs.mysql.com/bug.php?id=75685.
	//So now here is the logic for getting things correct up to maximum extent--
	//fire both queries and
	//First try to use show create trigger if query works then get the body of trigger from the result
	//if show create trigger fails use show triggers--because user can have trigger without quotes
	//Below is the implementation of this logic.
	wyBool ismysql5017 = IsMySQL5017(tunnel, mysql);
	wyBool ismysql41 = IsMySQL41(tunnel, mysql);

	query.Sprintf("show triggers from `%s` where `trigger` = '%s'", db, trigger);
    myres = SjaExecuteAndGetResult(tunnel, mysql, query);
	
	query1.Sprintf("show create trigger `%s`. `%s`", db, trigger);
	myres1 = SjaExecuteAndGetResult(tunnel, mysql, query1);
	if(!myres)
	{
		GetError(tunnel, mysql, strmsg);
		return wyFalse;
	}
	if(!myres1)
	{
	isshowcreateworked=wyFalse;
	}
		
	if(sja_mysql_num_rows(tunnel, myres)== 0)
	{
		strmsg.Sprintf("Trigger '%s' doesn't exists!", trigger);
		sja_mysql_free_result(tunnel, myres);
		return wyFalse;
	}

	myrow = sja_mysql_fetch_row(tunnel, myres);
	if(isshowcreateworked)
	myrow1= sja_mysql_fetch_row(tunnel, myres1);
	if(!myrow || !myrow[0])
	{
		strmsg.SetAs(_("Unable to retrieve information. Please check your permission."));
		sja_mysql_free_result(tunnel, myres);
        return wyFalse;		
	}
		
	//body of trigger using show create triger
	if(isshowcreateworked && myrow1[2])
	{
	bodyoftrigger.SetAs(myrow1[2]);
	if(GetBodyOfTrigger(&bodyoftrigger)==-1)
		{
         strmsg.SetAs(_("Unable to retrieve body of trigger."));
		 sja_mysql_free_result(tunnel, myres);
		  sja_mysql_free_result(tunnel, myres1);
           return wyFalse;

	     }
	}
//	Trigger Definer
	if(!isdefiner)
		if(ismysql5017 == wyTrue) 
		{
			coldefiner = GetFieldIndex(tunnel, myres, "Definer");

			if(coldefiner == -1)
			{
				sja_mysql_free_result(tunnel, myres);
				if(myres1)
		        sja_mysql_free_result(tunnel, myres1);
				return wyFalse;
			}
		
			if(myrow[coldefiner])
			{
				definer.SetAs(myrow[coldefiner], ismysql41);
			}
		}

	//Trigger Table
	coltablename = GetFieldIndex(tunnel, myres, "Table");
	if(coltablename == -1)
	{
#if defined _WIN32 && ! defined  _CONSOLE
		
		MessageBox(hwnd, _(L"Unable to retrive the table for this trigger"), 
					pGlobals->m_appname.GetAsWideChar(), MB_OK | MB_ICONINFORMATION);
		  sja_mysql_free_result(tunnel, myres);
		  if(myres1)
		        sja_mysql_free_result(tunnel, myres1);

#endif
		return wyFalse;
	}
		
	//Timing
	coltiming = GetFieldIndex(tunnel, myres, "Timing");
	if(coltiming == -1)// || !myrow[coltiming])
	{
		sja_mysql_free_result(tunnel, myres);
		if(myres1)
		        sja_mysql_free_result(tunnel, myres1);
		return wyFalse;
	}
		
	//Event
	colevent = GetFieldIndex(tunnel, myres, "Event");
	if(colevent == -1)
	{
		sja_mysql_free_result(tunnel, myres);
		  if(myres1)
		        sja_mysql_free_result(tunnel, myres1);
		return wyFalse;
	}
			

	//Trgger Statement
	colstmt = GetFieldIndex(tunnel, myres, "Statement");
	if(colstmt == -1)// || !myrow[colstmt])
	{
		sja_mysql_free_result(tunnel, myres);
		  if(myres1)
		        sja_mysql_free_result(tunnel, myres1);
		return wyFalse;
	}
		
	//Parse the Definer and inserts single quotes ('user'@'host')
	if(!isdefiner)
		if(definer.GetLength())
		{
			ch = strrchr((wyChar*)definer.GetString(), '@');
		
			pos = ch - definer.GetString();
		
			if(pos > 0)
			{	
				definer.Substr(0, pos);

				ch++;

				deftemp.Sprintf("'%s'@'%s'", definer.Substr(0, pos), ch);

				//Keeps the definer
				def.Sprintf("/*!50017 DEFINER = %s */", deftemp.GetString()); 
			}
		}
		
	//if(myrow && myrow[0] && myrow[1] && myrow[3] && myrow[4])
	if(myrow && myrow[colevent] && myrow[colstmt] && myrow[coltiming] && myrow[coltablename])
    {
		/*strtrigger.Sprintf("CREATE\n%s%s\n%sTRIGGER `%s`.`%s` %s %s ON `%s`.`%s` \n%sFOR EACH ROW %s;\n", 
			FMT_SPACE_4, def.GetString(), FMT_SPACE_4,db, myrow[0], myrow[4], myrow[1], db, table, FMT_SPACE_4, myrow[3]);*/
		/*strtrigger.Sprintf("CREATE\n%s%s\n%sTRIGGER `%s` %s %s ON `%s` \n%sFOR EACH ROW %s;\n", 
			FMT_SPACE_4, def.GetString(), FMT_SPACE_4, myrow[0], myrow[4], myrow[1], table, FMT_SPACE_4, myrow[3]);*/

		if(isdefiner)
		{
			if(isshowcreateworked && bodyoftrigger.GetLength()!=0)
			{strtrigger.Sprintf("CREATE\n%sTRIGGER `%s` %s %s ON `%s` \n%sFOR EACH ROW %s;\n", 
			FMT_SPACE_4, trigger, myrow[coltiming], myrow[colevent], myrow[coltablename], FMT_SPACE_4, bodyoftrigger.GetString());}
			else
			{
			strtrigger.Sprintf("CREATE\n%sTRIGGER `%s` %s %s ON `%s` \n%sFOR EACH ROW %s;\n", 
			FMT_SPACE_4, trigger, myrow[coltiming], myrow[colevent], myrow[coltablename], FMT_SPACE_4,myrow[colstmt]);
			}
		}
		else
		{
			if(isshowcreateworked && bodyoftrigger.GetLength()!=0)
			{strtrigger.Sprintf("CREATE\n%s%s\n%sTRIGGER `%s` %s %s ON `%s` \n%sFOR EACH ROW %s;\n", 
			FMT_SPACE_4, def.GetString(), FMT_SPACE_4, trigger, myrow[coltiming], myrow[colevent], myrow[coltablename], FMT_SPACE_4, bodyoftrigger.GetString());}
			else
			{
			strtrigger.Sprintf("CREATE\n%s%s\n%sTRIGGER `%s` %s %s ON `%s` \n%sFOR EACH ROW %s;\n", 
			FMT_SPACE_4, def.GetString(), FMT_SPACE_4, trigger, myrow[coltiming], myrow[colevent], myrow[coltablename], FMT_SPACE_4, myrow[colstmt]);
			}
		}
		
    }
    else
    {
        strmsg.SetAs(_("Unable to retrieve information. Please check your permission."));
        return wyFalse;
    }
	sja_mysql_free_result(tunnel, myres);
	sja_mysql_free_result(tunnel, myres1);

	return wyTrue;
}
#endif

void GetError(Tunnel *tunnel, PMYSQL mysql, wyString &strmsg)
{
	strmsg.Sprintf("Error No. %d\n%s", sja_mysql_errno(tunnel, *mysql), 
                        sja_mysql_error(tunnel, *mysql));
	return ;
}

wyBool 
GetCreateFunctionString(Tunnel * tunnel, PMYSQL mysql, const wyChar *db, const wyChar * function, 
                                wyString &strfunction, wyString &strmsg, wyString *queryex)
{
	wyString    query;
	MYSQL_RES	*myres;
	MYSQL_ROW	myrow;

	query.Sprintf("show create function `%s`.`%s`", db, function);

	if(queryex)
		queryex->SetAs(query);

#if defined WIN32 && ! defined _CONSOLE
	myres = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query);
#else	
	myres = SjaExecuteAndGetResult(tunnel, mysql, query);
#endif
	
	if(!myres)
	{
		GetError(tunnel, mysql, strmsg);
		return wyFalse;
	}

	if(sja_mysql_num_rows(tunnel, myres)== 0)
	{
		strmsg.Sprintf("Function '%s' doesn't exists!", function);
		sja_mysql_free_result(tunnel, myres);
		return wyFalse;
	}

	myrow = sja_mysql_fetch_row(tunnel, myres);
	
    if(myrow && myrow[2])
	    strfunction.SetAs(myrow[2]);
    else
    {
        //strmsg.SetAs(_("Unable to retrieve information. Please check your permission."));
		strmsg.SetAs(PROCEDURE_FUNC_ERRMSG);
        return wyFalse;
    }

	sja_mysql_free_result(tunnel, myres);

	return wyTrue;
}

wyBool 
GetCreateProcedureString(Tunnel * tunnel, PMYSQL mysql, const wyChar *db, const wyChar *procedure, 
                         wyString &strprocedure, wyString &strmsg, wyString  *queryex)
{	
	MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
	wyString    query;
	
	query.Sprintf("show create procedure `%s`.`%s`", db, procedure);
	if(queryex)
		queryex->SetAs(query);

#if defined WIN32 && ! defined _CONSOLE
	myres = ExecuteAndGetResult(GetActiveWin(), tunnel, mysql, query);
#else	
	myres = SjaExecuteAndGetResult(tunnel, mysql, query);
#endif
	
	
	if(!myres)
    {
		GetError(tunnel, mysql, strmsg);
		return wyFalse;
	}
	
	if(sja_mysql_num_rows(tunnel, myres)== 0)
	{
		strmsg.Sprintf("Procedure '%s' doesn't exists!", procedure);
		sja_mysql_free_result(tunnel, myres);
		return wyFalse;
	}

	myrow = sja_mysql_fetch_row(tunnel, myres);

	if(myrow[2])
    {
        strprocedure.SetAs(myrow[2]);
    }
    else
    {
        //strmsg.SetAs(_("Unable to retrieve information. Please check your permission."));
        strmsg.SetAs(PROCEDURE_FUNC_ERRMSG);
		return wyFalse;
    }

	sja_mysql_free_result(tunnel, myres);
	return wyTrue;
}
wyBool 
GetCreateEventString(Tunnel * tunnel, PMYSQL mysql, const wyChar *db, const wyChar *event, 
					 wyString &strevent, wyString &strmsg, wyString *queryex)
{
	wyString    query;
	MYSQL_RES	*myres;
	MYSQL_ROW	myrow;
	wyInt32     index = 0;
	
	query.Sprintf("show create event `%s`.`%s`", db, event);

	if(queryex)
		queryex->SetAs(query);

    myres = SjaExecuteAndGetResult(tunnel, mysql, query);
	
	if(!myres)
    {
		GetError(tunnel, mysql, strmsg);
		return wyFalse;
	}
	
	if(sja_mysql_num_rows(tunnel, myres)== 0)
	{
		strmsg.Sprintf("Event '%s' doesn't exists!", event);
		sja_mysql_free_result(tunnel, myres);
		return wyFalse;
	}

	myrow = sja_mysql_fetch_row(tunnel, myres);
	index = GetFieldIndex(tunnel, myres, "Create Event"); 

	if(index >= 0)
    {
        strevent.SetAs(myrow[index]);
    }
    else
    {
        strmsg.SetAs(_("Unable to retrieve information. Please check your permission."));
        return wyFalse;
    }

	sja_mysql_free_result(tunnel, myres);
	return wyTrue;
}

wyUInt32 
GetRowCount(Tunnel * m_tunnel, MYSQL *m_mysql, const wyChar *db, 
            const wyChar *table, const wyChar *whereclause)
{
	wyInt32             rcount;
	MYSQL_ROW			rownum;	
	MYSQL_RES			*rowinfo=NULL;
    wyString            query;

	/* select count(*)to get the number of rows from desired table */ 
    query.Sprintf("select count(*)from `%s`.`%s`%s%s", 
        db, table, (whereclause)?(" where "):(""),(whereclause)?(whereclause):(""));
		
    rowinfo = SjaExecuteAndGetResult(m_tunnel, &m_mysql, query);
    if(!rowinfo)
		return 0;

	rownum = sja_mysql_fetch_row(m_tunnel, rowinfo);
	rcount = atoi(rownum[0]);
	sja_mysql_free_result(m_tunnel, rowinfo);
	
	return  rcount;
}


wyBool 
GetCreateTableString(Tunnel * tunnel, PMYSQL pmysql, const wyChar *db, 
					 const wyChar* tbl, wyString &strcreate, wyString &query)
{
	MYSQL_ROW			myrow;	
	MYSQL_RES			*res = NULL;
	wyChar              *err;
    wyBool              find = wyFalse;

	strcreate.Clear();

	/* select count(*)to get the number of rows from desired table */ 
	if(!query.GetLength())
	{
		if(db)
			query.Sprintf("show create table `%s`.`%s`", db, tbl);
		else
			query.Sprintf("show create table `%s`", tbl);
	}													
		
    res = SjaExecuteAndGetResult(tunnel, pmysql, query);
    if(!res)
	{
			err = (wyChar*)sja_mysql_error(tunnel, *pmysql);
			return wyFalse;
	}

	myrow = sja_mysql_fetch_row(tunnel, res);
	
	if(myrow && myrow[1])// check for the on update current_timestamp
	{
		strcreate.SetAs(myrow[1]);
		find = wyTrue;
	}
	
	if(res)
		sja_mysql_free_result(tunnel, res);

	return  find;
}

wyBool 
CheckForOnUpdate(wyString &strcreate, wyInt32 fieldpos)
{
	wyChar      *create, *row, *sep = "\n";
	wyInt32     count=0;
	wyString    strtemp;
	wyBool      find = wyFalse;
	
	if(!strcreate.GetLength())
		return find;

	create = (wyChar*)calloc(strcreate.GetLength()+ 1, sizeof(wyChar));
	strcpy(create, strcreate.GetString());
	row = strtok(create, sep);
	
	while(count <= fieldpos)
	{
		row = strtok(NULL, sep);
		
		if(!row)
		{
			free(create);
			return find;
		}
		
		count++;
	}
	
	strtemp.SetAs(row);
	
	if(strtemp.FindI("on update CURRENT_TIMESTAMP", 0) != -1)
		find = wyTrue;
	
	free(create);
	return find;
}
wyBool GetExpressionValue(wyChar * currentrow, wyString * expression)
{
wyChar * find="AS";
wyBool found=wyFalse;
const char *ptr = strstr(currentrow,find);
if(ptr) {
	found=wyTrue;
  int index = ptr - currentrow+2;
   while(currentrow[index]!='S'&& currentrow[index]!='P'&& currentrow[index]!='V' )
   {
	   expression->AddSprintf("%c",currentrow[index]);
   
     index++;
   }
}

return found;

}
wyBool GetCheckConstraintValue(wyChar * currentrow, wyString * expression)
{
	if (currentrow == NULL)
	{
		return wyFalse;
	}
	wyChar * find = "CHECK", *findcomment = "COMMENT";
	wyBool found = wyFalse;
	wyChar *ptr = strstr(currentrow, find);
	wyChar *ptrc = strstr(currentrow, findcomment);
	wyString s1, s2,s3;
	s1.SetAs(currentrow);
	s2.SetAs("");
	s3.SetAs("COMMENT");
	wyInt32 index=0;
	if (ptr) {
		if (ptrc) {
			index = ptr - currentrow + 5;
			s2.SetAs(s1.Substr(index, 7));
			while (s2.CompareI(s3)!=0)//(currentrow[index + 2] != 'C' && currentrow[index + 3] != 'O' && currentrow[index + 4] != 'M')
			{
				expression->AddSprintf("%c", currentrow[index]);
				index++;
				s2.SetAs(s1.Substr(index, 7));
			}
			found = wyTrue;
		}
		else
		{
			 index = ptr - currentrow + 5;

			while (wyTrue)
			{
				if ((currentrow[index] == ',' && currentrow[index + 2] == ' ')|| (currentrow[index] == '\0' ))
					break;

				expression->AddSprintf("%c", currentrow[index]);
				index++;
			}
			found = wyTrue;
		}

	}

	return found;

}

wyBool GettablelevelCheckConstraintValue(wyChar * currentrow, wyString * expression)
{
	wyChar * find = "CHECK";
	wyBool found = wyFalse;/*, withcomment = wyFalse, withoutcomment = wyFalse;*/
	wyChar *ptr = strstr(currentrow, find);
	wyInt32 index=0;
	if (ptr) {
		found = wyTrue;
		index =( ptr - currentrow) + 5;
		while (currentrow[index] != '\0')
		{
			/*if (currentrow[index] == '\0' && currentrow[index + 2] == ' ')
				break;*/
			expression->AddSprintf("%c", currentrow[index]);
			index++;
		}
		const char last = expression->GetCharAt(expression->GetLength() - 1);
		if (last == ',')
		{
			expression->Strip(1);
		}
	}

	return found;
}

void
CheckForQuotesAndReplace(wyString *name)
{
	wyBool flag = wyFalse;
	const char first = name->GetCharAt(0);
	const char last = name->GetCharAt(name->GetLength() - 1);

	if (first == '`' && last == '`')
	{
		name->Strip(1);
		name->Erase(0, 1);
	}
	return ;
}

wyBool GetCheckConstraintName(wyChar * currentrow, wyString * checkconstraintname)
{
	wyChar * find = "CONSTRAINT";
	wyBool found = wyFalse;
	wyString s1, s2(""), p;
	wyInt32 index = 0;

	s1.SetAs(currentrow);

	wyChar *ptr = strstr(currentrow, find);
	if (ptr) {
			found = wyTrue;
			index = ptr - currentrow + 12;
				while (currentrow[index] != '`')
				{
					checkconstraintname->AddSprintf("%c", currentrow[index]);
					index++;
				}

		}

	return found;

}
wyInt32
GetBitFieldColumnWidth(wyString &strcreate, wyInt32 fieldpos)
{
	wyChar      *create, *row, *sep = "\n", *tok, *chr;
	wyInt32     count=0, ret;
	wyString    strtemp;

	if(!strcreate.GetLength())
		return -1;

	create = (wyChar*)calloc(strcreate.GetLength()+ 1, sizeof(wyChar));
	strcpy(create, strcreate.GetString());
	row = strtok(create, sep);

	while(count <= fieldpos)
	{
		row = strtok(NULL, sep);
		
		if(!row)
		{
			free(create);
			return -1;
		}		
		count++;
	}

	strtemp.SetAs(row);

	chr = strrchr(row, '`');
	if(!chr)
	{
		free(create);
		return -1;
	}

	tok = strtok(chr, "(");
	if(!tok)
	{
		free(create);
		return -1;
	}
	
	tok = strtok(NULL, tok);
	if(tok)
	{
		ret = atoi(tok);
		free(create);
		return ret;
	}
	free(create);
	return -1;
}

wyBool 
CheckForVariable(const wyChar *val)
{
	if((stricmp(val, "CURRENT_TIMESTAMP"	)== 0)||
		(stricmp(val, "CURRENT_USER"		)== 0)||
		(stricmp(val, "UTC_TIMESTAMP"		)== 0)||
		(stricmp(val, "CURRENT_USER"		)== 0)||
		(stricmp(val, "LOCALTIMESTAMP"		)== 0))
		return wyTrue;
	
	return wyFalse;
}

wyInt32 
GetSelectViewStmt(const wyChar *db, wyString &selectstmt)
{
    wyInt32 len;
	wyString dbnameasstring(db);
	// depends on the preferences we can go for whether to use "show" command or not 
	len = selectstmt.Sprintf("select `TABLE_NAME` from `INFORMATION_SCHEMA`.`TABLES` where `TABLE_SCHEMA` = '%s' and `TABLE_TYPE` = 'VIEW'", dbnameasstring.GetString());
    
	return len;
}
wyInt32 
GetSelectProcedureStmt(const wyChar *db, wyString &selectstmt, wyBool iscollate)
{
	wyInt32 len;
 	wyString dbname(db);    // This will convert to Widechar to UTF8 format.
	// depends on the preferences we can go for whether to use "show" command or not 
	if(iscollate)
		len = selectstmt.Sprintf("select `SPECIFIC_NAME` from `INFORMATION_SCHEMA`.`ROUTINES` where BINARY `ROUTINE_SCHEMA` = '%s' and ROUTINE_TYPE = 'PROCEDURE'", dbname.GetString());
	else
		len = selectstmt.Sprintf("select `SPECIFIC_NAME` from `INFORMATION_SCHEMA`.`ROUTINES` where `ROUTINE_SCHEMA` = '%s' and ROUTINE_TYPE = 'PROCEDURE'", dbname.GetString());
	return len;
}

wyInt32 
GetSelectFunctionStmt(const wyChar *db, wyString &selectstmt, wyBool iscollate)
{
	wyInt32 len;
	wyString dbnamestr(db);
	// depends on the preferences we can go for whether to use "show" command or not 
	if(iscollate)
		len = selectstmt.Sprintf("select `SPECIFIC_NAME` from `INFORMATION_SCHEMA`.`ROUTINES` where BINARY `ROUTINE_SCHEMA` = '%s' and ROUTINE_TYPE = 'FUNCTION'", dbnamestr.GetString());
	else
		len = selectstmt.Sprintf("select `SPECIFIC_NAME` from `INFORMATION_SCHEMA`.`ROUTINES` where `ROUTINE_SCHEMA` = '%s'and ROUTINE_TYPE = 'FUNCTION'", dbnamestr.GetString());
	return len;
}
wyInt32 
GetSelectEventStmt(const wyChar *db, wyString &selectstmt, wyBool iscollate)
{
	wyInt32 len = 0;
	wyString dbnamestr(db);
	if(iscollate)
		len = selectstmt.Sprintf("select `EVENT_NAME` from `INFORMATION_SCHEMA`.`EVENTS` where BINARY `EVENT_SCHEMA` = '%s' order by EVENT_NAME", dbnamestr.GetString());
	else
		len = selectstmt.Sprintf("select `EVENT_NAME` from `INFORMATION_SCHEMA`.`EVENTS` where `EVENT_SCHEMA` = '%s' order by EVENT_NAME", dbnamestr.GetString());
	return len;
}

wyInt32 
GetSelectTriggerStmt(const wyChar *db, wyString &selectstmt)
{
	wyInt32 len = 0;
	wyString dbnamestr(db);
	//len = selectstmt.Sprintf("select `TRIGGER_NAME` from `INFORMATION_SCHEMA`.`TRIGGERS` where `TRIGGER_SCHEMA` = '%s'", dbnamestr.GetString());
	len = selectstmt.Sprintf("SHOW TRIGGERS from `%s`", dbnamestr.GetString());
	return len;
}

#ifdef _WIN32
wyBool
GetApplicationFolder(wyWChar * path)
{
    wyInt32 filelen = 0;
    wyInt32 index = 0;

    filelen =(GetModuleFileName(NULL, path, MAX_PATH));
    
    if(filelen == 0)
    {
        path[0] = 0;
        return wyFalse;
    }

    /*
        To get the module path we need to stripout the application name from the path,
        the application name can be SQLyog.exe or SQLyogEnt.exe.
        We returns when we finds a '\'(Slash)in the path while parsing from the end.
    */
    for(index = filelen - 1; index >= 0; index--)
    {
        if(path[index] == '\\')
        {
            path[index] = '\0';
            break;
        }
    }

    return wyTrue;
}
#endif

//Allocation for Wide Character Buffer
wyWChar*
AllocateBuffWChar(wyInt32 size)
{
	return((wyWChar*)calloc(sizeof(wyWChar), size * sizeof(wyWChar)));
}

wyChar* 
AllocateBuff(wyInt32 size)
{
    return((wyChar*)calloc(sizeof(wyChar), size));
}

wyChar* 
ReAllocateBuff(wyChar *buff, wyInt32 size)
{
    return((wyChar*)realloc(buff, sizeof(wyChar)*size));
}


#ifdef _WIN32

wyInt32  
GetModuleNameLength()
{
    wyWChar fullpath[MAX_PATH+1] = {0};
    wyWChar extractpath[MAX_PATH+1] = {0}, *filename;
    wyInt32 len;

    if(GetModuleFileName(NULL, fullpath, MAX_PATH - 1) == NULL)
    {
	    return 0;
    }

	// extract the directory
	if(GetFullPathName(fullpath, MAX_PATH, extractpath, &filename ) == NULL )
    {        
		return 0;
    }


	*(filename-1) = 0;		// eat the trailing slash

    len = (wyInt32)wcslen(filename);

    return len;
}

wyBool
GetModuleDir(wyString &path)
{
	wyWChar fullpath[MAX_PATH + 1] = { 0 };
	wyWChar extractpath[MAX_PATH + 1] = { 0 }, *filename;
	wyInt32 len;

	if (GetModuleFileName(NULL, fullpath, MAX_PATH - 1) == NULL)
	{
		return wyFalse;
	}

	// extract the directory
	if (GetFullPathName(fullpath, MAX_PATH, extractpath, &filename) == NULL)
	{
		return wyFalse;
	}

	*(filename - 1) = 0;		// eat the trailing slash

	path.SetAs(extractpath);

	return wyTrue;
}


 #endif


wyChar *
GetEscapedValue(Tunnel *tunnel, PMYSQL mysql, const wyChar *value)
{
	wyInt32		length	=	strlen(value);
    wyChar		*tbuff	=	AllocateBuff(length * 2 + 1);

	sja_mysql_real_escape_string(tunnel, *mysql, tbuff, value, (unsigned long)length);

    return tbuff;
}

#ifdef _WIN32
void WriteLog(const wyChar* str)
{

//#ifndef _DEBUG 
    /* If it is in DEBUG mode don't perform any logging, just return...*/
    //return;
//#endif

    wyWChar      path[MAX_PATH+1] = {0};			/* The folder path will not go beyond MAX_PATH */
	wyWChar		*lpfileport=0;
    wyString    wyStr;							/* This will hold full file path */
    HANDLE      logfile;						/* Log file Handle */
	wyInt32		ret = 0;
	FILE*		log;

	ret = SearchFilePath(L"sqlyog", L".log", MAX_PATH, path, &lpfileport);	/* Failed to retreive the Application folder path*/

	if(ret == wyFalse)
	{
		wyStr.SetAs(path);
		wyStr.Add("\\sqlyog.log");		/* Creates the full file path */

   logfile = CreateFile ( wyStr.GetAsWideChar(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, NULL, NULL ); 

   if(logfile == INVALID_HANDLE_VALUE)
       return;
	}

	wyStr.SetAs(path);
	log = _wfopen(wyStr.GetAsWideChar(), L"a");
	if(!log)
		return;

	fprintf(log, "\n%s", str);
	fclose(log);
    
    return;
}
#endif

wyUInt32
PrepareShowTableInfo(Tunnel * tunnel, PMYSQL mysql, wyString& db, wyString &query)
{
	wyUInt32 count;
	
	if(IsMySQL5010(tunnel, mysql))
	{
		if(db.CompareI("INFORMATION_SCHEMA") != 0)
		{
			//count = query.Sprintf("select `TABLE_NAME` from `INFORMATION_SCHEMA`.`TABLES` where `TABLE_SCHEMA` = '%s' and `TABLE_TYPE` = 'BASE TABLE'", db.GetString());
            count = query.Sprintf("show table status from `%s` where engine is not null", db.GetString());
			return count;
		}
	}
	
	count = query.Sprintf("show tables from `%s`", db.GetString());
	return count;
}

wyUInt32
PrepareShowTable(Tunnel * tunnel, PMYSQL mysql, wyString& db, wyString &query)
{
	wyUInt32 count = 0;
	
	if(IsMySQL5010(tunnel, mysql))
	{
		if(db.CompareI("INFORMATION_SCHEMA") != 0)
		{
			//count = query.Sprintf("select `TABLE_NAME` from `INFORMATION_SCHEMA`.`TABLES` where `TABLE_SCHEMA` = '%s' and `TABLE_TYPE` = 'BASE TABLE'", db.GetString());
            count = query.Sprintf("show full tables from `%s` where table_type = 'BASE TABLE'", db.GetString());
			return count;
		}
	}
	
	count = query.Sprintf("show tables from `%s`", db.GetString());
	return count;
}

/* ---- Base64 Encoding --- */
/*
* EncodeBase64()
*
* Returns 1 if successful. The third argument
* is a pointer to an allocated area holding the base64 data. If something
* went wrong, 0 is returned.
*
*/

wyInt32 
EncodeBase64( const wyChar *inp, size_t insize, wyChar ** outptr)
{
	wyUChar  ibuf[3];
	wyUChar  obuf[4];
	wyUInt32 len = 0;
	wyInt32  i;
	wyInt32  inputparts;
	wyChar   *output;
	wyChar   *base64data;
	wyChar *indata = (wyChar *)inp;

	*outptr = NULL; /* set to NULL in case of failure before we reach the end */

	if(0 == insize)
		insize = strlen(indata);

	base64data = output =(wyChar*)malloc(insize*4/3+4);
	if(NULL == output)
		return 0;

	while(insize > 0){
		for(i = inputparts = 0; i < 3; i++){
			if(insize > 0){
				inputparts++;
				ibuf[i] = *indata;
				indata++;
				insize--;
			}
			else
				ibuf[i] = 0;
		}

		obuf [0] =(ibuf [0] & 0xFC)>> 2;
		obuf [1] =((ibuf [0] & 0x03)<< 4)|((ibuf [1] & 0xF0)>> 4);
		obuf [2] =((ibuf [1] & 0x0F)<< 2)|((ibuf [2] & 0xC0)>> 6);
		obuf [3] = ibuf [2] & 0x3F;

		switch(inputparts){

	case 1: // only one byte read
		len += sprintf(output, "%c%c==",
			table64[obuf[0]],
			table64[obuf[1]]);
		break;

	case 2: // two bytes read 
		len += sprintf(output, "%c%c%c=",
			table64[obuf[0]],
			table64[obuf[1]],
			table64[obuf[2]]);
		break;

	default:
		len += sprintf(output, "%c%c%c%c",
			table64[obuf[0]],
			table64[obuf[1]],
			table64[obuf[2]],
			table64[obuf[3]]);
		break;
		}

		output += 4;
	}

	*output=0;
	*outptr = base64data; // make it return the actual data memory 

	return len; // return true 
}

// decode a base64 buffer
size_t 
DecodeBase64 (const wyChar *src, wyChar *dest)
{
	wyInt32 length = 0;
	wyInt32 equalsTerm = 0;
	wyInt32 i;
	wyInt32 numQuantums;
	wyUChar lastQuantum[3];
	size_t rawlen=0;

	while((src[length] != '=') && src[length])
		length++;
	while(src[length+equalsTerm] == '=')
		equalsTerm++;

	numQuantums = (length + equalsTerm) / 4;

	rawlen = (numQuantums * 3) - equalsTerm;

	for(i = 0; i < numQuantums - 1; i++) {
		DecodeQuantum((wyUChar *)dest, src);
		dest += 3; src += 4;
	}

	DecodeQuantum(lastQuantum, src);
	for(i = 0; i < 3 - equalsTerm; i++)
		dest[i] = lastQuantum[i];

	return rawlen;
}

void 
DecodeQuantum(wyUChar *dest, const wyChar *src)
{
	wyUInt32    x = 0;
	wyInt32     i;
	for(i = 0; i < 4; i++) {
		if(src[i] >= 'A' && src[i] <= 'Z')
			x = (x << 6) + (wyUInt32)(src[i] - 'A' + 0);
		else if(src[i] >= 'a' && src[i] <= 'z')
			x = (x << 6) + (wyUInt32)(src[i] - 'a' + 26);
		else if(src[i] >= '0' && src[i] <= '9')
			x = (x << 6) + (wyUInt32)(src[i] - '0' + 52);
		else if(src[i] == '+')
			x = (x << 6) + 62;
		else if(src[i] == '/')
			x = (x << 6) + 63;
		else if(src[i] == '=')
			x = (x << 6);
	}

	dest[2] = (wyUChar)(x & 255);
	x >>= 8;
	dest[1] = (wyUChar)(x & 255);
	x >>= 8;
	dest[0] = (wyUChar)(x & 255);
}

void
ExtractEngineName(wyChar *engine, wyString &enginename)
{
    wyInt32 index, startindex = 0, endindex = 0, len = strlen(engine);

    wyChar *buff = AllocateBuff(len+1);

    // it will make the starting position of the have_blackhole_engine
    for(index = 0; index < len; index++)
    {
        if(engine[index] == '_')
        {
            index++;
            break;
        }
    }

    startindex = index;

    for(index = len - 1; index > startindex; index--)
    {
        if(engine[index] == '_')
        {
            break;
        }
    }

    endindex = index;

    if(stricmp(engine+index+1, "engine") != 0)
        endindex = len - startindex;

    strncpy(buff, engine + startindex, endindex);

    enginename.SetAs(buff);

    free(buff);

    return;
}

wyBool 
IsSupportedEngine(wyString &enginename) 
{
    wyChar  supported[][20] = { "MyISAM", "ISAM", "InnoDB", "HEAP", "BDB", 
                                "EXAMPLE", "ARCHIVE", "CSV", "BLACKHOLE", "SolidDB",
                                NULL};
    wyInt32 index = 0;

    for(index = 0; supported[index][0] != NULL; index++)
    {
        if(enginename.CompareI(supported[index]) == 0)
        {
            enginename.SetAs(supported[index]);
            return wyTrue;
        }
    }
    
    return wyFalse;
}

void
GetTableEngineString(Tunnel *tunnel, PMYSQL mysql, wyString &strengine)
{
    wyString    query, myrowstr;
    MYSQL_RES   *myres = NULL;
    MYSQL_ROW   myrow;
    wyString    enginename;
	wyBool		ismysql41 = IsMySQL41(tunnel, mysql);

    strengine.Clear();
    strengine.Add(";");

    if(IsMySQL51(tunnel, mysql))
    {
        // Frame the query to get Engines/Type from the information schema
        query.Sprintf("select `ENGINE`, `SUPPORT` from information_schema.Engines");
        
        myres = SjaExecuteAndGetResult(tunnel, mysql, query);

        if(!myres)
        {
            return;
        }

        while(myrow = sja_mysql_fetch_row(tunnel, myres))
        {
            if(stricmp(myrow[0], "FEDERATED") != 0 &&
               stricmp(myrow[0], "MRG_MYISAM") != 0 &&
               stricmp(myrow[0], "MERGE") != 0 &&
               stricmp(myrow[1], "DISABLED") != 0 && stricmp(myrow[1], "NO") != 0)
			{
				myrowstr.SetAs(myrow[0], ismysql41);
				strengine.AddSprintf("%s;", myrow[0]);
			}
        }
    }
    else if(IsMySQL412(tunnel, mysql))
	{
		// Frame the query to get Engines/Type from the information schema
        query.Sprintf("SHOW ENGINES");
        
        myres = SjaExecuteAndGetResult(tunnel, mysql, query);

        if(!myres)
        {
            return;
        }

        while(myrow = sja_mysql_fetch_row(tunnel, myres))
        {
            if(stricmp(myrow[0], "FEDERATED") != 0 &&
               stricmp(myrow[0], "MRG_MYISAM") != 0 &&
               stricmp(myrow[0], "MERGE") != 0 &&
               stricmp(myrow[1], "DISABLED") != 0 && stricmp(myrow[1], "NO") != 0)
			{
				myrowstr.SetAs(myrow[0], ismysql41);
				strengine.AddSprintf("%s;", myrow[0]);
			}
        }
	}
	else
    {
        query.SetAs("show variables like 'have_%'");
        myres = SjaExecuteAndGetResult(tunnel, mysql, query);

        if(myres == NULL)
        {
            return;
        }
        
        while(myrow = sja_mysql_fetch_row(tunnel, myres))
        {
            if(myrow[0] && myrow[1] && stricmp(myrow[1], "YES") == 0)
            {
                ExtractEngineName(myrow[0], enginename);

                if(enginename.GetLength() && IsSupportedEngine(enginename) == wyTrue)
                                    strengine.AddSprintf("%s;", enginename.GetString());
                }
            }
                
        strengine.Add("MyISAM;");
    }

    if(strengine.GetLength() > 0)
        strengine.Strip(1);

    if(myres)
        sja_mysql_free_result(tunnel, myres);

    return;
}

#if defined _WIN32 && ! defined  _CONSOLE
wyInt32 
GetChunkLimit(wyInt32 *chunksize)
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH +1] = {0};
	wyString	dirstr;
		
	// Get the complete path.
	SearchFilePath( L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	
	*chunksize = wyIni::IniGetInt("GENERALPREF", "RowLimit", 1000, dirstr.GetString());

	return 1;
}

wyInt32
DeletePrivateProfileString(wyChar *strkey, wyChar *section, wyChar *filename)
{
    wyInt32 retvalue;

    //retvalue = WritePrivateProfileStringA(section, strkey, NULL, filename);
	retvalue = wyIni::IniDeleteKey(section, strkey, filename);

    return retvalue;
}

wyBool 
IsChunkInsert()
{
	wyWChar		*lpfileport=0;
	wyWChar		directory[MAX_PATH +1] = {0};
	wyString	dirstr;
	wyInt32		ischunkinsert;	
		
	// Get the complete path.
	SearchFilePath( L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	
	ischunkinsert = wyIni::IniGetInt("GENERALPREF", "SetMaxRow", 1, dirstr.GetString());

	return (ischunkinsert == 1)?wyFalse:wyTrue;

}

#endif

wyInt32  
Comparestring(wyWChar **arg1, wyWChar **arg2)
{
#ifdef _WIN32
   // Compare all of both strings: 
   return _wcsicmp(*arg1, *arg2);
#else
   wyString comparg1, comparg2;
   if(*arg1)
       comparg1.SetAs(*arg1);
   if(*arg2)
       comparg2.SetAs(*arg2);
   return comparg1.CompareI(comparg2);
#endif
}

wyBool
GetMySqlDataType(MysqlDataType *rettypedata, MYSQL_FIELD *fields, wyInt32 fieldno)
{
	wyInt32 type = fields[fieldno].type;

	switch(type)
	{
	case MYSQL_TYPE_DECIMAL:
		rettypedata->m_exceltype.SetAs("Number");
		rettypedata->m_mysqltype.SetAs("decimal");
		break;
		
	case MYSQL_TYPE_TINY:
		rettypedata->m_exceltype.SetAs("Number");
		rettypedata->m_mysqltype.SetAs("tinyint");
		break;
	
	case MYSQL_TYPE_SHORT:
		rettypedata->m_exceltype.SetAs("Number");
		rettypedata->m_mysqltype.SetAs("smallint");
		break;
	
	case MYSQL_TYPE_LONG:
		rettypedata->m_exceltype.SetAs("Number");
		rettypedata->m_mysqltype.SetAs("int");
		break;

	case MYSQL_TYPE_FLOAT:
		rettypedata->m_exceltype.SetAs("Number");
		rettypedata->m_mysqltype.SetAs("float");
		break;
		
	case MYSQL_TYPE_DOUBLE:
		rettypedata->m_exceltype.SetAs("Number");
		rettypedata->m_mysqltype.SetAs("double");
		break;

	case MYSQL_TYPE_LONGLONG:
		rettypedata->m_exceltype.SetAs("Number");
		rettypedata->m_mysqltype.SetAs("bigint");
		break;
			
	case MYSQL_TYPE_BIT:
		rettypedata->m_exceltype.SetAs("String");
		rettypedata->m_mysqltype.SetAs("bit");
		break;

	case MYSQL_TYPE_INT24:
		rettypedata->m_exceltype.SetAs("Number");
		rettypedata->m_mysqltype.SetAs("mediumint");
		break;
	
	case MYSQL_TYPE_NEWDECIMAL:
		rettypedata->m_exceltype.SetAs("Number");
		rettypedata->m_mysqltype.SetAs("Decimal");
		break;
	
	case MYSQL_TYPE_NULL:
		rettypedata->m_exceltype.SetAs("String");
		rettypedata->m_mysqltype.SetAs("NULL");
		break;
		
	case MYSQL_TYPE_TIMESTAMP:
		rettypedata->m_exceltype.SetAs("DateTime");
		rettypedata->m_mysqltype.SetAs("timestamp");
		break;
		
	case MYSQL_TYPE_DATE:
		rettypedata->m_exceltype.SetAs("DateTime");
		rettypedata->m_mysqltype.SetAs("date");
		break;

	case MYSQL_TYPE_TIME:
		rettypedata->m_exceltype.SetAs("DateTime");
		rettypedata->m_mysqltype.SetAs("time");
		break;

	case MYSQL_TYPE_DATETIME:
		rettypedata->m_exceltype.SetAs("DateTime");
		rettypedata->m_mysqltype.SetAs("datetime");
		break;
		
	case MYSQL_TYPE_YEAR:
		rettypedata->m_exceltype.SetAs("DateTime");
		rettypedata->m_mysqltype.SetAs("year");
		break;

	case MYSQL_TYPE_NEWDATE:
		rettypedata->m_exceltype.SetAs("DateTime");
		rettypedata->m_mysqltype.SetAs("newdate");
		break;
		
	case MYSQL_TYPE_VARCHAR:
		rettypedata->m_exceltype.SetAs("String");
		rettypedata->m_mysqltype.SetAs("varchar");
		break;
		
	case MYSQL_TYPE_VAR_STRING:
		rettypedata->m_exceltype.SetAs("String");
		rettypedata->m_mysqltype.SetAs("varchar");
		break;

	case MYSQL_TYPE_STRING:
		rettypedata->m_exceltype.SetAs("String");
		rettypedata->m_mysqltype.SetAs("char");
		break;

	case MYSQL_TYPE_ENUM:
		rettypedata->m_exceltype.SetAs("String");
		rettypedata->m_mysqltype.SetAs("enum");
		break;
		
	case MYSQL_TYPE_SET:
		rettypedata->m_exceltype.SetAs("String");
		rettypedata->m_mysqltype.SetAs("set");

	case MYSQL_TYPE_TINY_BLOB:
		rettypedata->m_exceltype.SetAs("String");
        if(fields[fieldno].flags&BINARY_FLAG)
            rettypedata->m_mysqltype.SetAs("blob");
        else
            rettypedata->m_mysqltype.SetAs("text");
		break;
		
	case MYSQL_TYPE_MEDIUM_BLOB:
		rettypedata->m_exceltype.SetAs("String");
		if(fields[fieldno].flags&BINARY_FLAG)
            rettypedata->m_mysqltype.SetAs("blob");
        else
            rettypedata->m_mysqltype.SetAs("text");
		break;

	case MYSQL_TYPE_LONG_BLOB:
		rettypedata->m_exceltype.SetAs("String");
		if(fields[fieldno].flags&BINARY_FLAG)
            rettypedata->m_mysqltype.SetAs("blob");
        else
            rettypedata->m_mysqltype.SetAs("text");
		break;

	case MYSQL_TYPE_BLOB:
		rettypedata->m_exceltype.SetAs("String");
		if(fields[fieldno].flags&BINARY_FLAG)
            rettypedata->m_mysqltype.SetAs("blob");
        else
            rettypedata->m_mysqltype.SetAs("text");
		break;
	
	case MYSQL_TYPE_JSON:
		rettypedata->m_exceltype.SetAs("JSON");
        rettypedata->m_mysqltype.SetAs("JSON");
		break;

	case MYSQL_TYPE_GEOMETRY:
		rettypedata->m_exceltype.SetAs("String");
		rettypedata->m_mysqltype.SetAs("Geometry");
		break;
	}

	return wyTrue;
}

wyBool
GetCommentString(wyString &commentstring)
{
	wyInt32 newpos = 0, pos = 0;

	do
	{
		newpos = pos;
		pos = commentstring.Find("; InnoDB free:", pos + 1);
	}while(pos != -1);

	if(newpos == 0)
	{
		pos = commentstring.Find("InnoDB free:", pos + 1);

		if(pos != -1)
			commentstring.Strip(commentstring.GetLength() - pos);
	}

	else
		commentstring.Strip(commentstring.GetLength() - newpos);
			
	return wyTrue;
}

wyBool
GetCharsetString(wyString &charsetstring)
{
	wyInt32 pos = 0;
    pos = charsetstring.Find("_", pos + 1);

	charsetstring.Strip(charsetstring.GetLength() - (pos));
		
	return wyTrue;
}

void
ReverseString(wyChar *text)
{
    wyInt32     count;
    wyChar      *temptext = NULL;
    wyString    str;

    str.SetAs(text);
    
    temptext = (wyChar *)calloc(sizeof(wyChar), str.GetLength());
    
    strcpy(temptext, text);
    
    for(count = 0; count < str.GetLength(); count++)
    {
        text[count] = text[str.GetLength() - count -1];
    }

    free(temptext);
}

wyBool
UseDatabase(wyString &dbname, MYSQL *mysql, Tunnel *tunnel)
{
	wyString		query;
	
	wyInt32			ret;
	MYSQL_RES		*res;

	query.Sprintf("use `%s`", dbname.GetString());
	
	ret = HandleMySQLRealQuery(tunnel, mysql, query.GetString(), query.GetLength(), wyFalse);
	if(ret)
		return wyFalse;
#ifdef _WIN32   
	res = tunnel->mysql_store_result(mysql);
	if(!res &&mysql->affected_rows == -1)
		return wyFalse;

	tunnel->mysql_free_result(res);
	tunnel->SetDB(dbname.GetString());
#else
    res = ::mysql_store_result(mysql);
    if(!res &&mysql->affected_rows == -1)
		return wyFalse;

    ::mysql_free_result(res);
#endif
	return wyTrue;
}

wyInt32	
HandleMySQLRealQuery(Tunnel *tunnel, MYSQL * mysql, const char * query, 
					 unsigned long length, bool isbadforxml, bool batch, 
					 bool isend, bool * stop, bool isread, bool fksethttpimport)
{
    wyBool			ismysql41 = wyFalse;

#ifdef _WIN32
   ConvertString	cnvstr;

	if(mysql->server_version)
        ismysql41 = IsMySQL41(NULL, &mysql);
	
	if(ismysql41 == wyFalse && isread == false)
	{
		query = cnvstr.Utf8toAnsi(query, length);
		length = strlen(query);
	}
	return tunnel->mysql_real_query (mysql, query, length, isbadforxml, batch, isend, stop, fksethttpimport);
#else
    wyString querystr;

    if(query)
        querystr.SetAs(query);

    if(mysql->server_version)
        ismysql41 = IsMySQL41(NULL, &mysql);
    if(ismysql41 == wyFalse && isread == false)
    {
        query = querystr.GetAsAnsi();
        length = strlen(query);
    }

    return ::mysql_real_query(mysql, query, length);
#endif
}

wyInt32 
HandleSjaMySQLRealQuery(Tunnel * tunnel, MYSQL* mysql,const char * query, unsigned long len, bool isbatch, bool* stop)
{
	wyBool			ismysql41 = wyFalse;

#ifdef _WIN32
	ConvertString	cnvstr;
	
	if(mysql->server_version)
        ismysql41 = IsMySQL41(NULL, &mysql);

	if(ismysql41 == wyFalse)
	{
		query = cnvstr.Utf8toAnsi(query, len);
		len = strlen(query);
	}	
	return tunnel->mysql_real_query(mysql, query, len, (bool)IsBadforXML (query), isbatch, false, stop);
#else
    wyString querystr;

    if(query)
        querystr.SetAs(query);

    if(mysql->server_version)
        ismysql41 = IsMySQL41(NULL, &mysql);
    if(ismysql41 == wyFalse)
    {
        query = querystr.GetAsAnsi();
        len = strlen(query);
    }   
	return ::mysql_real_query( mysql, query, len);
#endif
}

#ifdef _WIN32
void 
ExamineData(wyString &codepage, wyString &buffer)
{
	IMultiLanguage2		*mlang;
    DetectEncodingInfo	info = {0};
    MIMECPINFO          codepageinfo;
	wyInt32				length = 0, cnt = 1;
	wyString			codepagestr;
        
    if(buffer.GetString())
	   length = buffer.GetLength();

	//HRESULT hr		=		S_OK;
    //SUCCEEDED(CoInitialize(NULL));							// init COM

    HRESULT hr = CoInitialize(NULL); // init COM
    if(SUCCEEDED(hr) == false)
    {
        codepage.SetAs("Unicode (UTF-8)");
        return;
    }
       
    hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage2,(void **)&mlang);
    
    //hr = CoCreateInstance(__uuidof(CMultiLanguage), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMultiLanguage2),(void **)&mlang);

    if(SUCCEEDED(hr) == false)
    {
        codepage.SetAs("Unicode (UTF-8)");
	    CoUninitialize();
        return;
    }

	hr = mlang->DetectInputCodepage(0,0, (wyChar *)buffer.GetString(), &length, &info, &cnt);

    if(SUCCEEDED(hr) == true)
    {
        hr = mlang->GetCodePageInfo(info.nCodePage, info.nLangID, &codepageinfo);
        if(SUCCEEDED(hr) == true)
			codepage.SetAs(codepageinfo.wszDescription);
    }
    else
    {
        codepage.SetAs("Unicode (UTF-8)");    
        mlang->Release();
	    CoUninitialize();
        return;
    }

	mlang->Release();
	CoUninitialize();
}
#endif

wyInt32 DetectFileFormat(wyChar *pBuffer, wyInt32 pBytesRead, wyInt32 *pHeaderSize)
{
    wyInt32 i;
    wyInt32 filelen = pBytesRead;

    struct BomLookup bomlook[] = 
    {
	    // define longest headers first
	    { 0xBFBBEF,	  3, NCP_UTF8	  },
	    //{ 0xFFFE,	  2, NCP_UTF16BE  },
	    { 0xFEFF,	  2, NCP_UTF16    },
	    { 0,          0, NCP_ASCII	  },
    };

	for(i = 0; bomlook[i].m_len; i++)
	{
		if(filelen >= bomlook[i].m_len &&
		   memcmp(pBuffer, &bomlook[i].m_bom, bomlook[i].m_len) == 0)
		{
			*pHeaderSize = bomlook[i].m_len;
			return bomlook[i].m_type;
		}
	}

	*pHeaderSize = 0;
	return NCP_ASCII;	// default to ASCII
}

wyBool
CheckForUtf8(wyString &pBuffer)
{
    //checks The valid UTF-8 sequences in binary, if any one fails means it is ANSI(Since we are supporting UTF8/ANSI only)
    //0???????
    //110????? 10??????
    //1110???? 10?????? 10??????
    //11110??? 10?????? 10?????? 10??????
    //111110?? 10?????? 10?????? 10?????? 10??????
    //1111110? 10?????? 10?????? 10?????? 10?????? 10??????

    wyInt32 i = 0;
    wyInt32 j = 0;
    wyInt32 r = 0;
    wyInt32 count = 0;
    wyChar  c;
    wyInt32 len = pBuffer.GetLength();

    wyInt32 B1 = 0x00;
    wyInt32 B2 = 0xC0;
    wyInt32 B3 = 0xE0;
    wyInt32 B4 = 0xF0;
    wyInt32 B5 = 0xF8;
    wyInt32 B6 = 0xFC;

    wyInt32 R1 = 0x80;
    wyInt32 R2 = 0xE0;
    wyInt32 R3 = 0xF0;
    wyInt32 R4 = 0xF8;
    wyInt32 R5 = 0xFC;
    wyInt32 R6 = 0xFE;

    wyInt32 E   = 0x80;
    wyInt32 ER  = 0xC0;

    wyBool  isutf8 = wyTrue;

    if(!len)
        return isutf8;

    for(; i < len; i++)
    {
        j = 0;
        count = 0;
        r = len - i;
        c = pBuffer.GetCharAt(i);

        if(~(c ^ B1) >= R1 || ~(c ^ B1) < 0) // valid
            continue;

        if(~(c ^ B2) >= R2)
            count = 1;
        else if(~(c ^ B3) >= R3)
            count = 2;
        else if(~(c ^ B4) >= R4)
            count = 3;
        else if(~(c ^ B5) >= R5)
            count = 4;
        else if(~(c ^ B6) >= R6)
            count = 5;

        if(count)
        {
            while(1)
            {
                j++;

                if(j > count)
                {
                    i = i + j - 1;
                    break;
                }

                if(j > r - 1)
                    break;

                c = pBuffer.GetCharAt(i + j);

                if(~(c ^ E) < ER) // invalid
                {
                    if(j != 1)
                        i = i + j - 1;
                    else
                        isutf8 = wyFalse;

                    break;
                }
            }
        }
        
        if(j == 0) // non UTF8
        {
            isutf8 = wyFalse;
            break;
        }

        if(j > r - 1)
            break;
    }

    return isutf8;
}

wyBool
CheckForUtf8Bom(wyString &filename)
{
	FILE				*fp = NULL;
	unsigned int		bytes[4] = {0};
	wyBool				isansifile = wyFalse;
	wyInt32				bomcounter = 0, counter;
	/*********************************************/
#ifdef _WIN32
	fp = _wfopen(filename.GetAsWideChar(), L"rb");
#else
    fp = fopen(filename.GetString(), "rb");
#endif
    if(!fp)
        return wyTrue;

	for(counter = 0; counter < 3; counter++)
	{
		 bytes[counter] = fgetc(fp);
		 
		 //239 = 0xEF, 187 = 0xBB and 191 = 0xBF are BOM's for UTF-8
		 //if it is a utf-8 encoded file, here it is checked.
		 if(bytes[counter] == 239 || bytes[counter] == 187 || bytes[counter] == 191)
			 bomcounter++;
		 else
			 break;
	}
	
	if(bomcounter == 3)
		isansifile = wyFalse;
	else
		isansifile = wyTrue;
	
	fclose(fp);
	return isansifile;
}

#ifdef _WIN32

ConvertString::ConvertString()
{
    m_widestr = NULL;
    m_utf8str = NULL;
	m_ansistr = NULL;
}

ConvertString::~ConvertString()
{
    /// Free allocated buffer
    if(m_widestr)
        free(m_widestr);

    m_widestr = NULL;

	if(m_ansistr)
        free(m_ansistr);

	m_ansistr = NULL;

    if(m_utf8str)
        free(m_utf8str);

   m_utf8str = NULL;
}

wyWChar * 
ConvertString::ConvertUtf8ToWideChar(wyChar* utf8str)
{
    wyInt32  length;

    // get the number of widechars required
	length = MultiByteToWideChar(CP_UTF8, 0, utf8str, -1,  NULL,  0);

    //free the existing buffer
    if(m_widestr)
        free(m_widestr);

    m_widestr = AllocateBuffWChar(length + 1);

	MultiByteToWideChar(CP_UTF8, 0, utf8str, -1, m_widestr, length);

	return m_widestr;
}

wyWChar * 
ConvertString::ConvertAnsiToWideChar(wyChar* ansistr)
{
    wyInt32  length;

    // get the number of widechars required
	length = MultiByteToWideChar(CP_ACP, 0, ansistr, -1,  NULL,  0);

    //free the existing buffer
    if(m_widestr)
        free(m_widestr);

    m_widestr = AllocateBuffWChar(length + 1);

	MultiByteToWideChar(CP_ACP, 0, ansistr, -1, m_widestr, length);

	return m_widestr;
}

wyChar* 
ConvertString::ConvertWideCharToUtf8(wyWChar * widestr)
{
    wyInt32 length;

    // the only way to convert an ansi text to utf-8 is this two way.
	length = WideCharToMultiByte(CP_UTF8, 0,(LPWSTR)widestr, -1, NULL, NULL, NULL, NULL);

    //free the existing buffer
    if(m_utf8str)
        free(m_utf8str);

	// allocate space
    m_utf8str = AllocateBuff(length + 1);

	WideCharToMultiByte(CP_UTF8, 0,(LPWSTR)widestr, -1,(LPSTR)m_utf8str, length, NULL, NULL);
    
    return m_utf8str;
}

wyChar*
ConvertString::Utf8toAnsi(const wyChar *utf8, wyInt32 len)
{
	wyInt32 length = MultiByteToWideChar(CP_UTF8, 0, utf8, len, NULL, NULL );
	wyWChar *lpszw = NULL;

	//free the existing buffer
    if(m_ansistr)
        free(m_ansistr);

	lpszw = new wyWChar[length+1];
	m_ansistr = AllocateBuff(length + 1);

	//this step intended only to use WideCharToMultiByte
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, lpszw, length );

	lpszw[length] = '\0';
	//Conversion to ANSI (CP_ACP)
	WideCharToMultiByte(CP_ACP, 0, lpszw, -1, m_ansistr, length, NULL, NULL);

	m_ansistr[length] = 0;

	delete[] lpszw;

	return m_ansistr;
}

wyBool
GetForeignKeyInfo(wyString *showcreate, wyString &key, wyInt32 relno, FKEYINFOPARAM *fkeyparam, const wyChar* dbname)
{
	wyString    query, createstmt;
	wyChar      *text, seps[] = "\n", *token = NULL;
    wyInt32     count = 0;
	wyBool		retval;

	if(!showcreate || !showcreate->GetLength())
		return wyFalse;

	createstmt.SetAs(*showcreate);

	if(key.GetLength())
		key.Clear();
	  
	token = strtok((wyChar*)createstmt.GetString(), seps);

	while(token && count < relno)
	{
		/* to know if the line has FK definition, we can check for either 
		   CONSTRAINT..FOREIGN KEY or REFERENCES
		   but for triple sureity we check for all. its one in million chance
		   if we will have all three in non-FK definition line */
		
		if(strstr(token, "CONSTRAINT")&& 
			 strstr(token, "FOREIGN KEY")&& 
			 strstr(token, "REFERENCES"))
		{
			/* it indeed is a reference so we add the definition in the grid */
			text = strstr(token, "CONSTRAINT");
			text = strstr(text, " ");
            key.SetAs(text);
			token = strtok(NULL, seps);
            count++;
			continue;
		}

		token = strtok(NULL, seps);
	}

	if(count != relno)
        key.Clear();
  
    //Gets all F-key detais and sets into a struct
	if(fkeyparam)
	{
		retval = GetForeignKeyDetails(&key, fkeyparam, dbname);
		if(retval == wyFalse)
			return wyFalse;
	}
	return wyTrue;
}
wyBool
GetForeignKeyDetails(wyString *fkeyinfo, LFKEYINFOPARAM fkeyparam, const wyChar* dbname)
{
	wyString	fkey;
	wyBool		retbool;
	wyString	constrintname, parenttable, parentdb;
	fkeyOption	deloption, updateoption;

	if(!fkeyinfo)
		return wyFalse;

	fkey.SetAs(*fkeyinfo);
		
	if(!fkeyparam && !fkey.GetLength())
		return wyFalse;

	//Parse the Relationship infos and store it in to the structure
	
	//gets F-key name
	retbool = GetConstraintName(&fkey, &constrintname);
	if(retbool == wyFalse)
		return wyFalse;

	//Gets Referenced table name
	retbool = GetParentTable(&fkey, &parenttable, dbname ? &parentdb : NULL);
	if(retbool == wyFalse)
		return wyFalse;
			
	//Referenced table field(s) involved in the relationship
	retbool = GetParentTableFlds(&fkey, fkeyparam);
	if(retbool == wyFalse)
		return wyFalse;

	//F-key table field(s) involved in the relationship
	retbool = GetChildTableFlds(&fkey, fkeyparam);
	if(retbool == wyFalse)
		return wyFalse;
			
	//Gets the ON UPDATE option if any
	updateoption    = GetOnFkeyOption(&fkey, fkeyparam);
	fkeyparam->m_onupdate = updateoption;
	
	//Gets the ON DELETE option if any
	deloption = GetOnFkeyOption(&fkey, fkeyparam, wyFalse);
	fkeyparam->m_ondelete = deloption;

	fkeyparam->m_constraint.SetAs(constrintname);
	fkeyparam->m_parenttable.SetAs(parenttable);
	
    if(!parentdb.GetLength() && dbname)
    {
        parentdb.SetAs(dbname);
    }

    fkeyparam->m_parenttabledb.SetAs(parentdb);
	return wyTrue;
}

wyBool
GetConstraintName(wyString *fkeyinfo, wyString *constraintname)
{
	wyChar		*tok;
	wyString	fkey;

	fkey.SetAs(*fkeyinfo);

	tok = strstr((wyChar*)fkey.GetString(), " FOREIGN KEY");
	if(!tok)
		return wyFalse;

	*tok = 0; 
	constraintname->SetAs(fkey);	
    constraintname->LTrim();

    wyChar  escchar = constraintname->GetCharAt(0);
    wyString tmpstr;

    tmpstr.SetAs(constraintname->GetString());

    tmpstr.Erase(0, 1);
    tmpstr.Strip(1);

    wyString findchars, esccharstr;
    findchars.Sprintf("%c%c", escchar, escchar);

    esccharstr.Sprintf("%c", escchar);

    tmpstr.FindAndReplace(findchars.GetString(), esccharstr.GetString());

    constraintname->Sprintf("\"%s\"",tmpstr.GetString());

	return wyTrue;	
}

wyBool
GetParentTable(wyString *fkeyinfo, wyString *parenttable, wyString* parentdb)
{
	wyChar      *par;
	wyString	fkey, temp;
    wyInt32     i;
    wyBool      dotfound = wyFalse, backquotefound = wyFalse;
    wyChar      encodedby = NULL;

    fkey.SetAs(*fkeyinfo);
    
	par = strstr((wyChar*)fkey.GetString(), "REFERENCES");
	
    if(!par)
		return wyFalse;

    temp.SetAs(par);
    temp.Erase(0, strlen("REFERENCES") + 1);

    encodedby = temp.GetCharAt(0);
    
    wyString enclstr;

    enclstr.Sprintf("%c", encodedby);

    wyInt32 lenstr = temp.GetLength();
    wyString    mystr;


    for(i=0; i<lenstr; i++)
    {
        if(temp.GetCharAt(i) == encodedby)
        {
            if(backquotefound == wyFalse)
            {
                backquotefound = wyTrue;
                continue;
            }
            if(temp.GetCharAt(i+1) == encodedby)
            {
                mystr.AddSprintf("%c", encodedby);
                i++;
                continue;
            }
            if(temp.GetCharAt(i+1) == ' ')
                break;
            if(temp.GetCharAt(i+1) == '.')
            {
                backquotefound = wyFalse;
                i++;
                dotfound = wyTrue;
            }
        }
        if(dotfound)
        {
            if(!parentdb)
                return wyFalse;
            parentdb->SetAs(mystr);
            mystr.Clear();
            dotfound = wyFalse;
        }
        else
            mystr.AddSprintf("%c", temp.GetCharAt(i));
    }

    parenttable->SetAs(mystr);

    /*
    if(parentdb)
    {
        parentdb->Clear();

        for(i = 0, ptr = (wyChar*)temp.GetString(); ptr[i]; ++i)
        {
            if(ptr[i] == encodedby)
            {
                if(backquotefound == wyFalse)
                {
                    backquotefound = wyTrue;
                    continue;
                }

                if(ptr[i + 1] == encodedby)
                {
                    i++;
                    parentdb->AddSprintf("%s", "\\\"");
                    continue;
                }

                break;
            }
            else if(backquotefound == wyTrue)
            {
                parentdb->AddSprintf("%c", ptr[i]);
            }
        }

        for(++i; ptr[i]; ++i)
        {
            if(ptr[i] == '.')
            {
                dotfound = wyTrue;
                break;
            }
            else if(ptr[i] == encodedby)
            {
                break;
            }
        }     

        if(dotfound == wyTrue)
        {
            par = ptr + i + 1;
        }
        else
        {
            parenttable->SetAs(parentdb->GetString());
            parentdb->Clear();
        }
    }
    
    par = strtok(par, enclstr.GetString());
	if(!par)
		return wyFalse;

    if(dotfound == wyFalse)
    {
        par = strtok(NULL, enclstr.GetString());
	    if(!par)
		    return wyFalse;
    }
        
	parenttable->SetAs(par);
	*/

	return wyTrue;
}

/*
wyBool
GetParentTable(wyString *fkeyinfo, wyString *parenttable, wyString* parentdb)
{
	wyChar      *par, *ptr;
	wyString	fkey, temp;
    wyInt32     i;
    wyBool      dotfound = wyFalse, backquotefound = wyFalse;

    fkey.SetAs(*fkeyinfo);
    
	par = strstr((wyChar*)fkey.GetString(), "REFERENCES");
	
    if(!par)
        return wyFalse;

    if(parentdb)
    {
        parentdb->Clear();

        for(i = 0, ptr = par; ptr[i]; ++i)
        {
            if(ptr[i] == '`')
            {
                if(backquotefound == wyFalse)
                {
                    backquotefound = wyTrue;
                    continue;
                }

                if(ptr[i + 1] == '`')
                {
                    i++;
                    parentdb->AddSprintf("%s", "``");
                    continue;
                }

                break;
            }
            else if(backquotefound == wyTrue)
            {
                parentdb->AddSprintf("%c", ptr[i]);
            }
        }

        for(++i; ptr[i]; ++i)
        {
            if(ptr[i] == '.')
            {
                dotfound = wyTrue;
                break;
            }
            else if(ptr[i] == '`')
            {
                break;
            }
        }

        if(dotfound == wyTrue)
        {
            par = ptr + i + 1;
        }
        else
        {
            parentdb->Clear();
        }
    }

    par = strtok(par, "`");
	if(!par)
		return wyFalse;

    if(dotfound == wyFalse)
    {
        par = strtok(NULL, "`");
        if(!par)
		    return wyFalse;
    }
        
	parenttable->SetAs(par);
	
	return wyTrue;
}
*/
wyBool
GetParentTableFlds(wyString *fkeyinfo, LFKEYINFOPARAM fkeyparam)
{
	wyChar			*par;
	wyString		fkey;
	RelTableFldInfo *parentfldinfo = NULL;
	List			parentfldlist;
	
	fkey.SetAs(*fkeyinfo);
    
	par = strstr((wyChar*)fkey.GetString(), "REFERENCES");
	    
	wyString    fk;

    if(par)
        fk.SetAs(par);

    if(!fk.GetLength())
        return wyFalse;


    fk.Erase(0, strlen("REFERENCES") + 1);

    wyInt32 ind = 0, nticks = 0;
    wyString    colname;
    wyBool      flag = wyFalse;
    wyString    enclosedstr;

    enclosedstr.Sprintf("%c", fk.GetCharAt(0));

    while(ind < fk.GetLength())
    {
        if(fk.GetCharAt(ind) == enclosedstr.GetCharAt(0))
            nticks++;
        

        if(nticks%2 == 1 && flag)
        {
            if(fk.GetCharAt(ind + 1) == ',' || fk.GetCharAt(ind + 1) == ')')
            {
                nticks = 0;
                wyString tmpstr;
                tmpstr.Sprintf("%c%c", enclosedstr.GetCharAt(0), enclosedstr.GetCharAt(0));
                
                colname.FindAndReplace(tmpstr.GetString(), enclosedstr.GetString());

                parentfldinfo = new RelTableFldInfo(colname.GetString());
		        if(parentfldinfo)
			        *fkeyparam->m_pkeyfldlist->Insert(parentfldinfo);

                if(fk.GetCharAt(ind + 1) == ')')
                    break;

                colname.Clear();
                ind+=4;
            }
        }
        //..checking for the enclosing backtick
		else if(nticks%2 == 0)
		{
            //..true when ind points to the starting bracket
			if(fk.GetCharAt(ind) == '(')
            {
                flag = wyTrue;
                ind+=2;
                if(fk.GetCharAt(ind) == enclosedstr.GetCharAt(0))
                    nticks = 1;
                else
                    nticks = 0;
            }
        }

        if(flag == wyFalse)
            fk.Erase(0, 1);
        else
        {
            colname.AddSprintf("%c", fk.GetCharAt(ind));

            ind++;
        }
    }

    if(flag == wyFalse)
        return wyFalse;
    
    
    



    /*
    par = strtok(par, "`");
	par = strtok(NULL, "`");	
	par = strtok(NULL, "`");	
	if(!par)
		return wyFalse;

	if(strcmp(par, ".") == 0)
    {
        par = strtok(NULL, "`");
        par = strtok(NULL, "(");
    }
    while(par)
	{
		par = strtok(NULL, "`"); 
		if(!par)
			break;

		ret = strcmp(par, ")");
		if(!ret)
			break;
		
		parentfldinfo = new RelTableFldInfo(par); 
		if(parentfldinfo)
			*fkeyparam->m_pkeyfldlist->Insert(parentfldinfo);
		
		par = strtok(NULL, "`"); 		
	}	
    */
	return wyTrue;	
}

wyBool			
GetChildTableFlds(wyString *fkeyinfo, LFKEYINFOPARAM fkeyparam)
{
	wyChar			*par;
	wyString		fkey, enclosedstr;
    wyChar          enclosedby = NULL;
	RelTableFldInfo *childfldinfo = NULL;

	fkey.SetAs(*fkeyinfo);
    
	par = strstr((wyChar*)fkey.GetString(), "FOREIGN KEY (");
	
	///in cluster version 7.3.0 the show create table returns "FOREIGN KEY(" instead of "FOREIGN KEY ("
	if(!par)
		par = strstr((wyChar*)fkey.GetString(), "FOREIGN KEY(");
    wyString fk;
    
    if(par)
        fk.SetAs(par);

    wyInt32 ind = fk.FindChar('(');

    if(ind <= 0)
        return wyFalse;

    enclosedby = fk.GetCharAt(ind + 1);

    //..ind will point to the first letter of the first column name after backtick
    ind+=2;

    wyInt32 nticks = 0;
    wyString colname;

    while(ind < fk.GetLength())
    {
        if(fk.GetCharAt(ind) == enclosedby)
			nticks++;
        
        colname.AddSprintf("%c", fk.GetCharAt(ind));
        
        //..checking for the enclosing backtick
		if(nticks%2 == 1)
		{
            //..true when Last column is fetched or there are still more columns to be extracted
			if(fk.GetCharAt(ind + 1) == ',' || fk.GetCharAt(ind + 1) == ')')
			{
                //..Stripping the backtick(last character)
				colname.Strip(1);
                if(enclosedby == '`')
                    colname.FindAndReplace("``", "`");
                
                childfldinfo = new RelTableFldInfo(colname.GetString()); 
		        if(childfldinfo)
		        {
			        *fkeyparam->m_fkeyfldlist->Insert(childfldinfo);
		        }

                //true when no more columns to extract
                if(fk.GetCharAt(ind + 1) == ')')
                    break;

                //..Moves i to the next column name(after backtick)
                ind++;
                while(fk.GetCharAt(ind) != enclosedby)
                    ind++;
                
                //..reseting variable
                nticks = 0;
                //..Reseting the variable
                colname.Clear();
			}
		}
        ind++;
    }
	return wyTrue;	
}

//Function gets corresponding F-key option : ON DELETE or ON UPDATE if defined
fkeyOption
GetOnFkeyOption(wyString *fkeyinfo, LFKEYINFOPARAM fkeyparam, wyBool isupdate)
{
	wyInt32		pos, slen;
	wyChar		*del = NULL, *update = NULL;
	wyString	fkey, updateoption, deloption, tempbuf;

	fkey.SetAs(*fkeyinfo);

	update = strstr((wyChar*)fkey.GetString(), "ON UPDATE");
	del = strstr((wyChar*)fkey.GetString(), "ON DELETE");
	
	if(!update && !del)
		return NOOPTION;

	if(isupdate == wyTrue)
	{
		if(!update)
			return NOOPTION;

		tempbuf.SetAs(update);        
		pos = tempbuf.Find("ON UPDATE", 0);
		if(pos == -1)
			return NOOPTION;

		tempbuf.Erase(pos, strlen("ON UPDATE"));
		tempbuf.LTrim();
		tempbuf.RTrim();		
	}
	else
	{	
		if(!del)
			return NOOPTION;

		else if(del && update)
		{
			tempbuf.SetAs(del);
			pos = tempbuf.Find(update, 0);
			if(pos == -1)
				return NOOPTION;

			slen = tempbuf.GetLength();
			tempbuf.Erase(pos, (slen - (pos - 1)));

			pos = tempbuf.Find("ON DELETE", 0);
			tempbuf.Erase(pos, strlen("ON DELETE"));
			tempbuf.LTrim();
			tempbuf.RTrim();		
		}

		else if(del && !update)
		{
			tempbuf.SetAs(del);
			pos = tempbuf.Find("ON DELETE", 0);

			if(pos == -1)
				return NOOPTION;

			tempbuf.Erase(pos, strlen("ON UPDATE"));
			tempbuf.LTrim();
			tempbuf.RTrim();
		}
	}

	if(!tempbuf.Compare("RESTRICT"))
		return RESTRICT;

	else if(!tempbuf.Compare("CASCADE"))
		return CASCADE;

	else if(!tempbuf.Compare("SET NULL"))
		return SETNULL;

	else if(!tempbuf.Compare("NO ACTION"))
		return NOACTION;
	
	else
		return NOOPTION;	
}

wyBool
GetDbCollation(Tunnel *tunnel, MYSQL *mysql, wyString &collation)
{
    MYSQL_RES   *myres;
    MYSQL_ROW   myrow;
	wyString	query;
    query.SetAs("SHOW VARIABLES LIKE 'collation_database';");    
    myres = SjaExecuteAndGetResult(tunnel, &mysql, query);
    if(!myres)
    {
        //collation.SetAs("latin1");
        return wyFalse;
	}

    myrow =tunnel->mysql_fetch_row(myres);
    
    if(myrow[1])
        collation.SetAs(myrow[1]);
    
    if(myres)
        tunnel->mysql_free_result(myres);

    return wyTrue;
}

wyBool
GetServerDefaultCharset(Tunnel *tunnel, MYSQL *mysql, wyString &charset, wyString &query)
{
    MYSQL_RES   *myres;
    MYSQL_ROW   myrow;

    query.SetAs("show variables like '%character%';");    
    myres = SjaExecuteAndGetResult(tunnel, &mysql, query);
    if(!myres)
    {
        charset.SetAs("latin1");
        return wyFalse;
	}

    myrow =tunnel->mysql_fetch_row(myres);
    
    if(myrow[1])
        charset.SetAs(myrow[1]);
    
    if(myres)
        tunnel->mysql_free_result(myres);

    return wyTrue;
}
#endif // _WIN32 

// helper function to get the column length in bytes for a given col
// mysql_fetch_lengths() is valid only for the current row of the result set. 
// It returns NULL if you call it before calling mysql_fetch_row() or 
// after retrieving all rows in the result. 
void
GetColLength(MYSQL_ROW row, wyInt32 numcols, wyInt32 col, wyUInt32 *len)
{
	MYSQL_ROW   column = row;
	wyUInt32    lengtharray[4096];// changed the max allowed number of column per table from 3400 to 4096
	wyUInt32    *arr = NULL, *plen = 0;

#ifdef _WIN32
	byte        *start = 0;
#else
    unsigned char *start = 0;
#endif
    
	MYSQL_ROW   end;

    VERIFY (len);
	arr = lengtharray;
	
	if(!column)
		return;

	for(end = column + numcols + 1 ; column != end ; column++, arr++)
	{
		if (!column || !*column)
		{
			*arr= 0;				
            continue;
		}
		
		if(start)					
        {   
#ifdef _WIN32
			*plen = (wyUInt32)((byte*)*column - start - 1);
#else
            *plen = (wyUInt32)((unsigned char*)*column - start - 1);
#endif
        }
#ifdef _WIN32
		start= (byte*)*column;
#else
        start= (unsigned char*)*column;
#endif
		
		plen = arr;
	}
	
	*len = lengtharray[col];
}

wyBool IsDatabaseValid(wyString &dbname, MYSQL *mysql, Tunnel *tunnel)
{	
	wyBool      flag = wyTrue;

#if defined _WIN32 && ! defined  _CONSOLE
	wyWChar		*tempfilter, *filterdatabase;
	wyUInt32	length = 1;
	wyString    db;
	
	dbname.GetAsWideChar(&length);
    tempfilter = AllocateBuffWChar(length + 1); 
	wcsncpy(tempfilter, dbname.GetAsWideChar(), length);
	filterdatabase = wcstok(tempfilter, L";");

	while(filterdatabase)
	{	
		db.SetAs(filterdatabase);
        flag = UseDatabase(db, mysql, tunnel);	
		if(flag == wyFalse)
			break;
		filterdatabase = wcstok(NULL, L";");
	}
    
    free(tempfilter);
#endif
	
	return flag;
}
// before starting transaction set autocommit equal to zero
wyBool
StartTransaction(Tunnel *tunnel, MYSQL *mysql)
{
	wyString query;
	MYSQL_RES *res;
	//use autocommit=0 for starting the transaction.
	//query.Sprintf("SET SESSION TRANSACTION ISOLATION LEVEL REPEATABLE READ;START TRANSACTION;");
	query.Sprintf("set autocommit=0;");
	res = SjaExecuteAndGetResult(tunnel, &mysql, query);

	if(!res && mysql->affected_rows == -1)
		return wyFalse;

	if(res)
		sja_mysql_free_result(tunnel, res);
	return wyTrue;
}
// commit after completed batch process
wyBool
EndTransaction(Tunnel *tunnel, MYSQL *mysql)
{
	wyString query;
	MYSQL_RES *res;
	
	query.Sprintf("commit");
	res = SjaExecuteAndGetResult(tunnel, &mysql, query);

	if(!res && mysql->affected_rows == -1)
		return wyFalse;

	if(res)
		sja_mysql_free_result(tunnel, res);
	return wyTrue;
}

//Handles the ssh reconnection
bool	   ReConnectSSH(ConnectionInfo *coninfo)
{
#if ! defined COMMUNITY	&& defined _WIN32	
	wyInt32					sshret;    
	HANDLE					hsqlyogmapfile = NULL;
	PROCESS_INFORMATION     pi;

	if(coninfo->m_hprocess != INVALID_HANDLE_VALUE) 
	{
		//If SSH connection then close handles at end
		if(coninfo->m_isssh == wyTrue)
			OnExitSSHConnection(&coninfo->m_sshpipeends);

		if(coninfo->m_sshsocket)
			closesocket(coninfo->m_sshsocket);
           
		coninfo->m_sshsocket = NULL;

		TerminateProcess(coninfo->m_hprocess, 1);
	}
	
	sshret = CreateSSHSession(coninfo, &pi, hsqlyogmapfile);

	if(sshret)
	{	
#ifndef _CONSOLE
		if(sshret != 1)//==1 if CreateProcess() is failed
			ShowSSHError(NULL);
//#else
		//ShowSSHError();// it needs when we implement connect for sja features
#endif 
		if(hsqlyogmapfile)
          VERIFY(CloseHandle(hsqlyogmapfile));

        hsqlyogmapfile = NULL;

      	coninfo->m_hprocess = INVALID_HANDLE_VALUE;

		return wyFalse;
	}
    else
    {
        if(hsqlyogmapfile)
          VERIFY(CloseHandle(hsqlyogmapfile));

        hsqlyogmapfile = NULL;
    }

	coninfo->m_hprocess      = pi.hProcess;

#endif
	
    return wyTrue;
}

//Instantiate the plink
#ifdef WIN32 
wyInt32
CreateSSHSession(ConnectionInfo *conninfo, PROCESS_INFORMATION * pi, HANDLE &hmapfile)
{
#ifndef COMMUNITY

	wyInt32					prret;
	wyBool					sflag = wyFalse;
	wyString				appname ; //"c:\\temp\\plink.exe -ssh -l rohit -pw amkette -L 3307:127.0.0.1:3306 127.0.0.1";
	wyChar					fullpath[MAX_PATH*2] = {0};
	STARTUPINFO				si;
	SECURITY_ATTRIBUTES		saattr; 
	HANDLE					mutex = NULL, hReadPipe = NULL, hWritePipe = NULL;
	DWORD					retmutex;
	wyFile					plinklock;
	HANDLE					namedmutex;
	SECURITY_ATTRIBUTES		lpMutexAttributes;
	wyString				escuser,escpass;
	lpMutexAttributes.bInheritHandle = TRUE;
	lpMutexAttributes.lpSecurityDescriptor = NULL;
	lpMutexAttributes.nLength = sizeof(lpMutexAttributes);


	/* get the full path */
	VERIFY(GetSjaExecPath(fullpath));
	strcat(fullpath, SSHTUNNELER);

    mutex = CreateEvent(NULL, FALSE, FALSE, TEXT(SQLYOG_MUTEX_NAME));


	namedmutex = CreateMutex(&lpMutexAttributes, FALSE, TEXT(SQLYOG_NAMEDMUTEX));
	WaitForSingleObject(namedmutex, INFINITE);

	hmapfile = CreateSQLyogMMF();

    if(hmapfile == NULL)
        return 1;

	prret = GetLocalEmptyPort(conninfo);

	if(prret == -1)
	{
#ifndef _CONSOLE
		DisplayErrorText(GetLastError(), _("Error in finding the open port"));
#endif

		return 1;		/* createprocess failed */
	}

	escuser.SetAs(conninfo->m_sshuser.GetString());
	escpass.SetAs(conninfo->m_sshpwd.GetString());
	escuser.FindAndReplace("\"","\"\"");
	escpass.FindAndReplace("\"","\"\"");
    if(conninfo->m_ispassword == wyTrue)
    {
	    appname.Sprintf("\"%s\" -ssh -l \"%s\" -pw \"%s\" -L %d:%s:%d -P %d \"%s\"", 
						fullpath, escuser.GetString(), escpass.GetString(), 
                        conninfo->m_localport, conninfo->m_host.GetString(), conninfo->m_port, 
                        conninfo->m_sshport, conninfo->m_sshhost.GetString());
    }
    else
    {
        appname.Sprintf("\"%s\" -ssh -l \"%s\" -pw \"%s\" -L %d:%s:%d -P %d %s -i \"%s\"", 
						fullpath, escuser.GetString(), escpass.GetString(), 
                        conninfo->m_localport, conninfo->m_host.GetString(), conninfo->m_port, 
                        conninfo->m_sshport, conninfo->m_sshhost.GetString(), conninfo->m_privatekeypath.GetString());

    }

	/* clear up all strcutures */
	memset(&si, 0, sizeof(si));
	memset(pi, 0, sizeof(pi));
	memset(&saattr, 0, sizeof(saattr));
	
	/* Set the bInheritHandle flag so pipe handles are inherited. */
	saattr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saattr.bInheritHandle = TRUE; 
	saattr.lpSecurityDescriptor = NULL;
	
	VERIFY(CreatePipe(&hReadPipe, &hWritePipe, &saattr, 0));
	HANDLE hReadPipe2, hWritePipe2;
	VERIFY(CreatePipe(&hReadPipe2, &hWritePipe2, &saattr, 0));

	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;
	si.dwFlags |= STARTF_USESTDHANDLES;
	si.hStdOutput = hWritePipe;
	si.hStdInput = hReadPipe2;
	si.hStdError = hWritePipe;

#ifndef _CONSOLE
	//EnterCriticalSection(&pGlobals->m_csiniglobal);
#endif
		
	prret = CreateProcess(NULL, (wyWChar*)appname.GetAsWideChar(), NULL, NULL, TRUE, 0, NULL, NULL, &si, pi);

	if(prret == 0)
	{
		ReleaseMutex(namedmutex);
		CloseHandle(namedmutex);
		//plinklock.Close();

#ifndef _CONSOLE
		DisplayErrorText(GetLastError(), _("FATAL ERROR: Network error: Connection timed out"));
		//LeaveCriticalSection(&pGlobals->m_csiniglobal);
#endif	
		
		return 1;		/* createprocess failed */
	}

	conninfo->m_sshpipeends.m_hreadpipe = hReadPipe;
	conninfo->m_sshpipeends.m_hwritepipe = hWritePipe;
	conninfo->m_sshpipeends.m_hreadpipe2 = hReadPipe2;
	conninfo->m_sshpipeends.m_hwritepipe2 = hWritePipe2;

    retmutex = WaitForSingleObject(mutex, 30000);

#ifndef _CONSOLE
	//LeaveCriticalSection(&pGlobals->m_csiniglobal);
#endif

    CloseHandle(mutex);

	//Close the temp file
	//plinklock.Close();
	ReleaseMutex(namedmutex);
	CloseHandle(namedmutex);

    if(retmutex==WAIT_OBJECT_0)
        return 0;
    
	if(sflag)
		return 0;
	else {
		
		/* this happens when the create process was success but not connected so we have to terminate the process */
		//If SSH connection then close handles at end
		if(conninfo->m_isssh == wyTrue)
			OnExitSSHConnection(&conninfo->m_sshpipeends);

		if(conninfo->m_sshsocket)
			closesocket(conninfo->m_sshsocket);
           
		conninfo->m_sshsocket = NULL;

		TerminateProcess(pi->hProcess, 1);
		pi->hProcess = INVALID_HANDLE_VALUE;
		
		return 2;
	}

#else
	return 0;
#endif

}

//If SSH connection then close handles at end
void
OnExitSSHConnection(PIPEHANDLES *sshpipehanles)
{
	DWORD lpbytes;
	char *buffer = "exit\n";

	//Executing the exit command to solve issue reporte at tickt #7953(Not stopping BASH.exe when terminating plink)
	WriteFile(sshpipehanles->m_hwritepipe2, buffer, 5, &lpbytes, NULL);
	
	Sleep(200);
	
	if(sshpipehanles->m_hreadpipe != INVALID_HANDLE_VALUE)
		CloseHandle(sshpipehanles->m_hreadpipe);

	if(sshpipehanles->m_hwritepipe != INVALID_HANDLE_VALUE)
		CloseHandle(sshpipehanles->m_hwritepipe);

	if(sshpipehanles->m_hreadpipe2 != INVALID_HANDLE_VALUE)
		CloseHandle(sshpipehanles->m_hreadpipe2);

	if(sshpipehanles->m_hwritepipe2 != INVALID_HANDLE_VALUE)
		CloseHandle(sshpipehanles->m_hwritepipe2);

	sshpipehanles->m_hreadpipe		= INVALID_HANDLE_VALUE;
	sshpipehanles->m_hwritepipe		= INVALID_HANDLE_VALUE;
	sshpipehanles->m_hreadpipe2		= INVALID_HANDLE_VALUE;
	sshpipehanles->m_hwritepipe2	= INVALID_HANDLE_VALUE;
}

#endif

//Reconnect to server
bool	   HandleReconnect(Tunnel *tunnel, PMYSQL mysql, ConnectionInfo *coninfo, const wyChar *dbname)
{
#ifdef _WIN32

	wyString	currentdb;
	
	//database name for issuing database name
	if(dbname)
		currentdb.SetAs(dbname);
	
#ifndef COMMUNITY			
	if(coninfo && coninfo->m_isssh == wyTrue)
	{   
		//for ssh connection
		if(!ReConnectSSH(coninfo))
			return false;		
	}

	/*
	if(coninfo->m_issslchecked == wyTrue)
    {
        if(coninfo->m_issslauthchecked == wyTrue)
        {
            mysql_ssl_set(newmysql, coninfo->m_clikey.GetString(), coninfo->m_clicert.GetString(), 
                            coninfo->m_cacert.GetString(), NULL, 
                            coninfo->m_cipher.GetLength() ? coninfo->m_cipher.GetString() : NULL);
        }
        else
        {
            mysql_ssl_set(newmysql, NULL, NULL, 
                            coninfo->m_cacert.GetString(), NULL, 
                            coninfo->m_cipher.GetLength() ? coninfo->m_cipher.GetString() : NULL);
        }
    }*/
#endif
	
	
	if(currentdb.GetLength() &&  UseDatabase(currentdb, *mysql, tunnel) == wyFalse)
		return false;
			    
	/* if its tunnel then we need the server version */
	if(tunnel->IsTunnel())
    {
		if(!tunnel->GetMySQLVersion(*mysql))
        {
            tunnel->mysql_close(*mysql);
            return false;
        }
    }
	
#endif

	return true;	
}

//Sets the fefault sql mode
void
SetDefSqlMode(Tunnel *tunnel, MYSQL *mysql)
{
	wyString    query;
	MYSQL_RES   *res;
	
	query.Sprintf("set sql_mode=''");

    res = SjaExecuteAndGetResult(tunnel, &mysql, query);

	if(!res && sja_mysql_affected_rows(tunnel, mysql) == -1)
		return;

	sja_mysql_free_result(tunnel, res);
}

//sets the server char set
void
SetCharacterSet(Tunnel *tunnel, MYSQL *mysql, wyString &cpname)
{
	wyString		query;
	MYSQL_RES		*res;
	wyBool			ismysql41 = IsMySQL41(tunnel, &mysql);
    
#ifdef _WIN32
	if(tunnel->IsTunnel())
	{
		if(ismysql41 == wyTrue)
			cpname.SetAs("utf8");
		tunnel->SetCharset(cpname.GetString());
		return;
	}
#endif

	if(ismysql41 == wyTrue)
	{
		query.Sprintf("set names '%s'", "utf8");	
        res = SjaExecuteAndGetResult(tunnel, &mysql, query);
		
		if(res)
			sja_mysql_free_result(tunnel, res);
	}
}

//gets the .exe path
wyInt32
GetSjaExecPath(wyChar *buffer)
{
    wyWChar     directory[MAX_PATH + 1]={0}, *lpfileport=0;
	DWORD       ret;
	wyString	dirstr;
	
	ret = SearchFilePath(L"sja.exe", NULL, MAX_PATH, directory, &lpfileport);

	if(ret == 0)
		return 0;

	directory[lpfileport - directory] = 0;
	dirstr.SetAs(directory);
	strcpy(buffer, dirstr.GetString());

	return strlen(buffer);
}


//Class used to hold the field name that involved in the relationship
RelTableFldInfo::RelTableFldInfo(const wyChar *fieldname)
{
	m_tablefld.SetAs(fieldname);
}

RelTableFldInfo::~RelTableFldInfo()
{
}

SelectedObjects::SelectedObjects(const wyChar *tablename, wyBool ismysql41)
{
	m_selobject.SetAs(tablename, ismysql41);
}
SelectedObjects::~SelectedObjects()
{
}
void
ReleaseMemory(List *list)
{
	SelectedObjects *temp;
	SelectedObjects *elem;
	if(!list)
		return ;
	elem =  (SelectedObjects*)list->GetFirst();		
	while(elem)
	{
		temp = (SelectedObjects*)elem->m_next;		
		elem->m_selobject.Clear();		
		list->Remove(elem);		
		delete elem;
		elem = temp;
	}	
}

void 
commonlog(const char * buff)
{
	
	//wyString    strpath, fullpathstr;
	//FILE        *fp;
	////wyString	filenamestr, extensionstr;
	////wyWChar      path[MAX_PATH + 1], fullpath[MAX_PATH + 1];
	////
	/////*if(SUCCEEDED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, 0, fullpath)))
 ////   {
	////	//wsprintf(path, L"%s\\SQLyog\\8.1_NEW.log\0", fullpath);

 ////       fullpathstr.SetAs(fullpath);
 ////       fullpathstr.Add("\\SQLyog");
 ////   }
	////else
	////	return;

	//fullpathstr.Add("\\slow_log.log");

	//fullpathstr.SetAs("D:\\log_html_87.html");

	//fp = fopen(fullpathstr.GetString() , "a" );
	////////fp = _wfopen(path , L"a" );
	//fprintf(fp, "%s\r\n" , buff);
	//fclose ( fp );
	//
}

//for writing error to log file
wyBool
WriteToLogFile(wyChar *message)
{
	if(!message)
		return wyFalse;

#if defined _WIN32 && ! defined _CONSOLE

	wyWChar		*lpFilePort = 0;
    wyWChar      filepath[MAX_PATH] = {0};
	wyString     errfile;
	FILE		*err;
	
	errfile.SetAs(message);
	MessageBox(NULL, errfile.GetAsWideChar(), pGlobals->m_appname.GetAsWideChar(), MB_ICONERROR | MB_OK | MB_TASKMODAL);         

	SearchFilePath(L"sqlyog", L".err", MAX_PATH, (wyWChar*)filepath, &lpFilePort);
	errfile.SetAs(filepath);
	err = _wfopen(errfile.GetAsWideChar(), L"a");
	if(!err)
		return wyFalse;

	fprintf(err, "\n%s", message);
	fclose(err);
#else

	printf("\n%s", message);
	if(!logfilehandle)
		return wyFalse;

	fprintf(logfilehandle, "\n%s", message);
	
#endif		

	return wyTrue;
}

void
ExecuteInitCommands(MYSQL* mysql, Tunnel* tunnel, wyString& initcommands)
{
    const wyChar	*token;
    SQLTokenizer	*tok;
    wyUInt32	    length = 0;
    wyInt32			isdel = 0;
    wyChar			delimiter[256] = ";\0";
    wyUInt32		len = 0, linenum = 0;
    wyString        query;

    length = initcommands.GetLength();
    tok = new SQLTokenizer(tunnel, &mysql, SRC_BUFFER, (void*)initcommands.GetString(), length);

    while(token = tok->GetQuery(&len, &linenum, &isdel, delimiter))
    {
        query.Clear();
        if(token && strlen(token))
        {
            query.SetAs(token);
            query.LTrim();
            query.RTrim();
        }
        if(query.GetLength())
            mysql_options(mysql, MYSQL_INIT_COMMAND, query.GetString());
    }
	delete tok;
}

VOID
SetMySQLOptions(ConnectionInfo *conn, Tunnel *tunnel, PMYSQL pmysql, wyBool issetcharset)
{
	wyString		strtimeout, sqlmode;
	wyInt32		timeout = 0;
	
	if(!conn || !tunnel)
		return;
    
	if(issetcharset == wyTrue)
		mysql_options(*pmysql, MYSQL_SET_CHARSET_NAME, "utf8");
    
	mysql_options(*pmysql, MYSQL_OPT_RECONNECT,(const char *)"true");
    
	//Set Compress protocol
	if(conn->m_iscompress == wyTrue)
		mysql_options(*pmysql, MYSQL_OPT_COMPRESS, 0);

	//set pwd cleartext
	/*if(conn->m_ispwdcleartext == wyTrue)*/
#ifdef _WIN32
	TCHAR curdir[MAX_PATH];
	wyString wyDir, curdirnew;
	if (GetModuleDir(curdirnew))
	{
		wyDir.SetAs(curdirnew);
		wyDir.Add("\\");
		//MessageBox(NULL, curdirnew.GetAsWideChar(), L"directory from new method", NULL);
	}
	else
	{
		GetCurrentDirectory(MAX_PATH - 1, curdir);
		wyDir.SetAs(curdir);
		wyDir.Add("\\");
		//MessageBox(NULL, wyDir.GetAsWideChar(), L"directory old location", NULL);
	}
	//GetCurrentDirectory(MAX_PATH-1, curdir);
	//wyDir.SetAs(curdir);
	//wyDir.Add("\\");
	mysql_options(*pmysql,MYSQL_PLUGIN_DIR ,wyDir.GetString());
#else

   
	char cwd[1024];
	int len = strlen(getcwd(cwd, sizeof(cwd)-2));
	if (getcwd(cwd, sizeof(cwd)) != NULL) 
	{
		cwd[len]='/';
		cwd[len+1] = '\0';
	}
	
	mysql_options(*pmysql,MYSQL_PLUGIN_DIR ,cwd);
#endif
    //mysql_options(*pmysql, MYSQL_OPT_LOCAL_INFILE, NULL);
	//unsigned int read_timeout = 10;
    //mysql_options(*pmysql, MYSQL_OPT_READ_TIMEOUT, reinterpret_cast<const char*>(&read_timeout));

	//SQL_MODE
#ifdef _WIN32
	if(conn->m_isglobalsqlmode == wyFalse)
	{	
		sqlmode.Sprintf("/*!40101 SET SQL_MODE='%s' */", conn->m_sqlmode.GetString());
		mysql_options(*pmysql, MYSQL_INIT_COMMAND, sqlmode.GetString());		
	}
#else
	sqlmode.SetAs("/*!40101 SET SQL_MODE=''*/");
	mysql_options(*pmysql, MYSQL_INIT_COMMAND, sqlmode.GetString());		
#endif


	//Session wait_timeout
	if(conn->m_isdeftimeout != wyTrue){
		timeout = conn->m_strwaittimeout.GetAsUInt32();
		if(timeout > 28800 || timeout <= 0)
			conn->m_strwaittimeout.SetAs("28800");
		strtimeout.Sprintf("/*!40101 set @@session.wait_timeout=%s */", conn->m_strwaittimeout.GetString());
		mysql_options(*pmysql, MYSQL_INIT_COMMAND, strtimeout.GetString());
	}
}

//Gets the field inforamtion of the view
wyBool  
DumpViewStruct(wyString * buffer, Tunnel * tunnel, const wyChar *view, MYSQL_RES  *res)
{
    
    MYSQL_ROW   row;
    wyInt32     fieldval, typeval;
	wyBool      isfirst = wyTrue;
    wyString    value;

	buffer->AddSprintf("\n/*!50001 CREATE TABLE  `%s`(\n", view);

	fieldval	= GetFieldIndex(tunnel, res, "field");
    typeval		= GetFieldIndex(tunnel, res, "type");

	while(row = sja_mysql_fetch_row(tunnel, res))
	{
		if(isfirst == wyFalse)
			buffer->Add(",\n");
		
		if(row[fieldval])
		{
            value.SetAs(row[fieldval]);
            value.FindAndReplace("`", "``");
            buffer->AddSprintf(" `%s` ", value.GetString());
        }
		
        if(row[typeval])
			buffer->AddSprintf("%s ", row[typeval]);
		
		isfirst = wyFalse;
	}

	buffer->Add("\n)*/;\n");

	return wyTrue;
}

//Gets the engiine for a particular table
wyBool
GetTableEngine(Tunnel * tunnel, MYSQL *mysql, const wyChar *tablename, const wyChar *dbname, wyString * strengine)
{
	wyString	query;
	MYSQL_RES	*res;
	MYSQL_ROW	myrow;
	wyInt32		engineindex = 1;

	query.Sprintf("show table status from `%s` like '%s'", dbname, tablename);

    res = SjaExecuteAndUseResult(tunnel, mysql, query);
	if(!res)
		return wyFalse;
	
	engineindex = (IsMySQL41(tunnel, &mysql) == wyTrue) ? GetFieldIndex(tunnel, res, "Engine") : GetFieldIndex(tunnel, res, "Type");
	
	VERIFY(myrow = sja_mysql_fetch_row(tunnel, res));
	
	if(!myrow || !myrow[engineindex] || engineindex == -1)
	{
		if(res)
			sja_mysql_free_result(tunnel, res);

		return wyFalse;
	}

	strengine->SetAs(myrow[engineindex]);

	if(res)
		sja_mysql_free_result(tunnel, res);

	return wyTrue;
}

#if ! defined COMMUNITY && defined _WIN32
wyInt32
GetLocalEmptyPort(ConnectionInfo *con)
{
	wyInt32 namelen = 0, retval = 0, ret, port = 0;
#ifdef _WIN32	
	SOCKET	psocket;		
	sockaddr_in name;

	memset(&name,0, sizeof(sockaddr_in));

	name.sin_family =  AF_INET;
	name.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	name.sin_port = htons(0);
	memset(name.sin_zero, '\0', sizeof(name.sin_zero));

	namelen = sizeof(sockaddr_in);

	psocket = socket(AF_INET, SOCK_STREAM, 0);

	if(psocket == INVALID_SOCKET)
		return -1;

	ret = ::bind(psocket, (sockaddr*)&name, sizeof(name));

	if(ret)
	{
		closesocket(psocket);
		con->m_sshsocket = NULL;

		return -1;
	}

	retval = getsockname(psocket, (sockaddr*)&name, &namelen);

	if(retval)
	{
		closesocket(psocket);
		con->m_sshsocket = NULL;
		return -1;
	}
		
	port = ntohs(name.sin_port);

	con->m_localport = port;

	//changed in 11.31beta1 - we use a named mutex to protect another SQLyog/sja from getting the same port
	/*Handle the local port in between processe and threads
	- Concern is time lap between closesocket and PLINK, another SQLyog/sja could get the same port.
	- To avoid this we keep a file open and it would close only once the PLINK is connected
	-Also we always allow connect plink whether the 'lock-file' functionalites got failed or not 
	*/
	//if(LockPlinkLockFile(plinklock) == wyFalse)
	//{
	//	closesocket(psocket);
	//	con->m_sshsocket = NULL;	
	//	
	//	return port;
	//}

	if(psocket)
		closesocket(psocket);

	
	psocket = NULL;
	
	con->m_sshsocket = NULL;

#endif
	
	return port;
}
#endif

wyBool
InitWinSock()
{
#ifdef _WIN32
	WORD ver;
	WSADATA data;

	ver = MAKEWORD(2, 0);
	VERIFY(WSAStartup(ver, &data)== 0);	
#endif 
	return wyTrue;
}

#if defined _WIN32
wyBool 
LockPlinkLockFile(wyFile *plinklock) 
{		
	//wyInt32 trycount = 1;
	wyString tempfile;

	if(!plinklock)
		return wyFalse;
	
	//Gets the temp folder path
	if(plinklock->GetTempFilePath(&tempfile) == wyFalse)
		return wyFalse;
	    
	tempfile.Add("SQLyog_session.tmp");
	
	// make sure you are setting file before calling any common function
	plinklock->SetFilename(&tempfile);

	//do
	//{
		if (plinklock->CheckIfFileExists() == wyFalse || 
			plinklock->RemoveFile())
		{
			if(plinklock->OpenWithPermission(GENERIC_WRITE, CREATE_NEW) != -1)
			{
				return wyTrue;				
			}			
		}
		
		//Sleep(PLINK_LOCK_WAIT);
		//trycount++;		
	
	//}while(trycount <= PLINK_LOCK_WAIT_TRY_COUNT);
	
	return wyFalse;	
}
#endif


/*Terminationg the child processes of a process.
-Its for killing plink.exe when user click on 'Terminate' button on wizard to terminate SJA.exe
*/
#ifdef _WIN32
void			
KillProcessTree(HANDLE parentproc)
{
	BOOL			bContinue = TRUE;
	unsigned long	procid = 0;
	PROCESSENTRY32	pe;
	HANDLE			hChildProc = NULL, hSnap = NULL;
	
	procid = GetProcessId(parentproc);
		
	memset(&pe, 0, sizeof(PROCESSENTRY32));
	pe.dwSize = sizeof(PROCESSENTRY32);
	
	hSnap = :: CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (::Process32First(hSnap, &pe))
	{
		bContinue = TRUE;

		// kill child processes
		while (bContinue)
		{
			// only kill child processes
			if (pe.th32ParentProcessID == procid)
			{
				hChildProc = ::OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe.th32ProcessID);

				//Terminate child process
				if (hChildProc)
				{
					::TerminateProcess(hChildProc, 1);
					::CloseHandle(hChildProc);												
				}                               
			}
			bContinue = ::Process32Next(hSnap, &pe);
		}						
	}		
}
#endif

void 
InitConnectionDetails(ConnectionInfo *conn)
{
    conn->m_port =0;
    conn->m_isssh = wyFalse;
	conn->m_ishttp = wyFalse;
    conn->m_sshport = 0;
    conn->m_localport = 0;
	conn->m_timeout = 0;
    conn->m_ishttp              = wyFalse;
    conn->m_ishttpuds			= wyFalse;
	conn->m_ispassword          = wyTrue;
    conn->m_isssh               = wyFalse;
    conn->m_issshsavepassword   = wyFalse;
    conn->m_issslchecked        = wyFalse;
    conn->m_issslauthchecked    = wyFalse;
    conn->m_issslchecked        = wyFalse;
	conn->m_iscompress			= wyTrue;	
	conn->m_isdeftimeout		= wyTrue;
	conn->m_strwaittimeout.SetAs("28800"); 
	conn->m_isreadonly			= wyFalse;
	conn->m_isencrypted = 0;
	//conn->m_ispwdcleartext		= wyFalse;

#ifdef _WIN32	
	conn->m_isglobalsqlmode		= wyFalse;
	conn->m_rgbconn				= RGB(255, 255, 255);
	conn->m_rgbfgconn				= RGB(0, 0, 0);
#endif
	    
    return;
}

//the function gets the foreign key info using information_schema
//the function is intentianally made like GetForeignKeyInfo for compliance with the existing code, thus ignoring the room for improvement
wyBool
GetForeignKeyInfo50(MYSQL_RES* myres, Tunnel* ptunnel, wyInt32 relno, FKEYINFOPARAM* pfkinfoparam)
{ 
#ifdef _WIN32	

	wyString    constraint, temp;
    MYSQL_ROW   myrow;
    wyInt32     i = 0;
    wyBool      ret = wyFalse;

    ptunnel->mysql_data_seek(myres, 0);

    while((myrow =  ptunnel->mysql_fetch_row(myres)))
    {
        if(!constraint.GetLength() || constraint.Compare(myrow[0]))
        {
            constraint.SetAs(myrow[0]);
            i++;

            if(i == relno)
            {
                ret = wyTrue;
                temp.Sprintf("`%s`", constraint.GetString());
                pfkinfoparam->m_constraint.SetAs(temp);
                pfkinfoparam->m_ondelete = pfkinfoparam->m_onupdate = (fkeyOption)0;
                pfkinfoparam->m_parenttable.SetAs(myrow[3]);
            }
        }

        if(i < relno)
        {
            continue;
        }
        else if(i > relno)
        {
            break;
        }

        pfkinfoparam->m_fkeyfldlist->Insert(new RelTableFldInfo(myrow[2]));
        pfkinfoparam->m_pkeyfldlist->Insert(new RelTableFldInfo(myrow[4]));
    }

    return ret;
#else
	return wyTrue;
#endif
}

//the function gets the result set for constraints info using information_schema
MYSQL_RES*
GetConstraintRes(Tunnel* ptunnel, PMYSQL pmysql, const wyChar* dbname, const wyChar* tablename)
{
    wyChar* constraintquery = "SELECT \
    `CONSTRAINT_NAME`,\
    `TABLE_NAME`,\
    `COLUMN_NAME`,\
    `REFERENCED_TABLE_NAME`,\
    `REFERENCED_COLUMN_NAME` \
FROM\
    `information_schema`.`KEY_COLUMN_USAGE` \
WHERE `CONSTRAINT_SCHEMA` = '%s' \
    AND `TABLE_SCHEMA` = '%s' \
    AND `REFERENCED_TABLE_SCHEMA` = '%s' \
    AND `TABLE_NAME` = '%s' \
    AND `CONSTRAINT_NAME` IN \
    (SELECT \
        CONSTRAINT_NAME \
    FROM \
        `information_schema`.`TABLE_CONSTRAINTS` \
    WHERE `CONSTRAINT_SCHEMA` = '%s' \
        AND `TABLE_SCHEMA` = '%s' \
        AND `TABLE_NAME` = '%s' \
        AND `CONSTRAINT_TYPE` = 'FOREIGN KEY') \
GROUP BY `CONSTRAINT_NAME`, \
    `TABLE_NAME`, \
    `COLUMN_NAME`, \
    `ORDINAL_POSITION`";

    MYSQL_RES*  myres;
    wyString    query;

    query.Sprintf(constraintquery,
        dbname, dbname, dbname, tablename, 
        dbname, dbname, tablename);

    myres = SjaExecuteAndGetResult(ptunnel, pmysql, query);
    return myres;
}

wyBool
IsDatatypeNumeric(wyString  &datatype)
{
    if(!(
        datatype.CompareI("bit") == 0 ||
        datatype.CompareI("tinyint") == 0 ||
        datatype.CompareI("bool") == 0 ||
        datatype.CompareI("boolean") == 0 ||
        datatype.CompareI("smallint") == 0 ||
        datatype.CompareI("mediumint") == 0 ||
        datatype.CompareI("int") == 0 ||
        datatype.CompareI("integer") == 0 ||
        datatype.CompareI("bigint") == 0 ||
        datatype.CompareI("float") == 0 ||
        datatype.CompareI("double") == 0 ||
        datatype.CompareI("decimal") == 0 ||
        datatype.CompareI("dec") == 0
        ))
        return wyFalse;

    return wyTrue;
}


wyBool
DecodePassword_Absolute(wyString &text)
{
	wyChar      pwd[512]={0}, pwdutf8[512] = {0};

	strcpy(pwdutf8, text.GetString());
	
	DecodeBase64(pwdutf8, pwd);
	RotateBitLeft((wyUChar*)pwd);
	strncpy(pwdutf8, pwd, 511);
	text.SetAs(pwdutf8);
	
	return wyTrue;
}


/*rotates bit right */
void
RotateBitRight(wyUChar *str)
{
	wyInt32     count;

	for(count = 0; str[count]; count++)
		str[count] = (((str[count])>>(1)) | ((str[count])<<(8 - (1))));

    return;
}

// We keep the name in encrypted form.
// so we do a bit rotation of 1 on the left before writing it into the registry.
void 
RotateBitLeft(wyUChar *str)
{
	wyInt32     count;

    for(count = 0; str[count]; count++)
		str[count] = (((str[count])<<(1)) | ((str[count])>>(8 - (1))));

	return;
}

wyBool
EncodePassword_Absolute(wyString &text)
{
	wyChar *encode = NULL, pwdutf8[512] = {0};

	strcpy(pwdutf8, text.GetString());
	RotateBitRight((wyUChar*)pwdutf8); 
	EncodeBase64(pwdutf8, strlen(pwdutf8), &encode);
	strncpy(pwdutf8, encode, 511);

	text.SetAs(pwdutf8);
	
	if(encode)
		free(encode);

	return wyTrue;
}

//wyInt32 extra : part of the pattern that should not be erased but only used in pattern for matching
void
RemoveDefiner(wyString &text, const wyChar* pattern, wyInt32 extra)
{
	//wyString pattern("DEFINER=`.*`@`.*`\\s");
	//wyInt32   regexret = 0;	
	wyInt32   ovector[30];  
	pcre           *re;
	wyInt32         erroffset, rc = -1;//, i = 0;
	wyInt32         subject_length;	
	const char      *error;
	wyString        tempstr;

	

	subject_length = (wyInt32)strlen(text.GetString());

	re = pcre_compile(
		pattern,              /* the pattern */
		PCRE_UTF8|PCRE_CASELESS|PCRE_NEWLINE_CR,/* default options */ //match is a case insensitive 
		&error,               /* for error message */
		&erroffset,           /* for error offset */
		NULL);                /* use default character tables */

	/* Compilation failed: print the error message and exit */

	if (re == NULL)
		return ; 

	/*************************************************************************
	* If the compilation succeeded, we call PCRE again, in order to do a     *
	* pattern match against the subject string. This does just ONE match *
	*************************************************************************/

	rc = pcre_exec(
	re,                   /* the compiled pattern */
	NULL,                 /* no extra data - we didn't study the pattern */
	text.GetString(),              /* the subject string */
	subject_length,       /* the length of the subject */
	0,                    /* start at offset 0 in the subject */
	PCRE_NO_UTF8_CHECK,             /* default options */
	ovector,              /* output vector for substring information */
	30);           /* number of elements in the output vector */

	if(re)
		pcre_free(re);

	if(rc == 1)
	{
		text.Erase(ovector[0], ovector[1]-ovector[0]-extra);
	}


}

void
RemoveBrackets(wyString &text, const wyChar* pattern)
{
	
	wyInt32   ovector[30];
	pcre           *re;
	wyInt32         erroffset, rc = -1, sucess = 0;//, i = 0;
	wyInt32         subject_length, text_length;
	const char      *error;
	wyString tempstr, strfirst,strlast;
	wyChar * tempstr1 = NULL;
	


	subject_length = (wyInt32)strlen(text.GetString());

	re = pcre_compile(
		pattern,              /* the pattern */
		PCRE_UTF8 | PCRE_CASELESS | PCRE_NEWLINE_CR,/* default options */ //match is a case insensitive 
		&error,               /* for error message */
		&erroffset,           /* for error offset */
		NULL);                /* use default character tables */

							  /* Compilation failed: print the error message and exit */

	if (re == NULL)
		return;

	/*************************************************************************
	* If the compilation succeeded, we call PCRE again, in order to do a     *
	* pattern match against the subject string. This does just ONE match *
	*************************************************************************/

	rc = pcre_exec(
		re,                   /* the compiled pattern */
		NULL,                 /* no extra data - we didn't study the pattern */
		text.GetString(),              /* the subject string */
		subject_length,       /* the length of the subject */
		0,                    /* start at offset 0 in the subject */
		PCRE_NO_UTF8_CHECK,             /* default options */
		ovector,              /* output vector for substring information */
		30);           /* number of elements in the output vector */

	if (re)
		pcre_free(re);

	tempstr.SetAs(text.GetString());
	strlast.SetAs(text.GetString());

	if (rc == 2)
	{
		tempstr1=tempstr.Substr(ovector[0], ovector[1]);
		if (tempstr1 != NULL)
		{
			text.SetAs(tempstr1);
		}
		strlast.Erase(ovector[0], ovector[1] - ovector[0]);
	}
		//Add method to remove the opening and closing brackets
		text_length = (wyInt32)strlen(strlast.GetString());
		
		const char first = strlast.GetCharAt(0);
		const char last = strlast.GetCharAt(text_length - 1);
	

		if (strcmp(&first,"(")==0  && strcmp(&last, ")")==0)
		{
			strlast.Erase(0, 1);
			strlast.Strip(1);
		}
		strfirst.AddSprintf("%s%s", text.GetString(), strlast.GetString());
		text.SetAs(strfirst);

}

//void DebugLog(const char *buffer)
//{
//        wyWChar                directory[MAX_PATH+1];
//        wyWChar                *lpfileport;
//        wyBool                ret;
//        
//        ret = SearchFilePath(L"sqlyog_debug1", L".log", MAX_PATH, directory, &lpfileport);
//
//        if(ret != wyTrue)
//        {
//                return;
//        }
//
//        FILE* fp = _wfopen(directory, L"a+");
//        if(!fp)
//        {
//                return;
//        }
//
//        fprintf(fp, "%s\r\n", buffer);
//        fclose(fp);
//}

wyBool
EncodePassword(wyString &text)
{
	wyString ciphertext, plaintext("");
	wyString temptext("");

	plaintext.SetAs(text);
	// create an encrypted object
	CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption enc;
	enc.SetKeyWithIV(AESKey, sizeof(AESKey), AESiv);

	std::string encText;
	CryptoPP::StreamTransformationFilter encFilter(enc, new CryptoPP::StringSink(encText));

	//encryption
	encFilter.Put(reinterpret_cast<const CryptoPP::byte*>(plaintext.GetString()), plaintext.GetLength());
	encFilter.MessageEnd();
	text.SetAsDirect(encText.data(), encText.length());
	
	return wyTrue;
}

wyBool
DecodePassword(wyString &text)
{
	wyString ciphertext(""), decryptedtext("");

	if (text.GetLength() <= 0)
	{
		return wyFalse;
	}

	ciphertext.SetAsDirect(text.GetString(), text.GetLength());

	CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption dec;
	dec.SetKeyWithIV(AESKey, sizeof(AESKey), AESiv);

	// Conversion filter for decryption
	std::string decText;
	CryptoPP::StreamTransformationFilter decFilter(dec, new CryptoPP::StringSink(decText));
	decFilter.Put(reinterpret_cast<const CryptoPP::byte*>(ciphertext.GetString()), ciphertext.GetLength());
	decFilter.MessageEnd();

	text.SetAs(decText.c_str());

	return wyTrue;
}

wyBool
MigratePassword(wyString conn, wyString dirstr, wyString &pwdstr)
{

	DecodePassword_Absolute(pwdstr);
	EncodePassword(pwdstr);
	wyChar *encodestr=pwdstr.EncodeBase64Password();
	pwdstr.Clear();
	pwdstr.SetAs(encodestr);

	if (encodestr)
		free(encodestr);
	return wyTrue;
	
}

wyBool
MigratePassword(wyString &pwdstr)
{

	DecodePassword_Absolute(pwdstr);
	EncodePassword(pwdstr);
	wyChar *encodestr=pwdstr.EncodeBase64Password();
	pwdstr.Clear();
	pwdstr.SetAs(encodestr);

	if (encodestr)
		free(encodestr);
	return wyTrue;

}
