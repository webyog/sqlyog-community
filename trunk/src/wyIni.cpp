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

#include "wyIni.h"
#include "wyString.h"
#include "Global.h"
#include "CommonHelper.h"

extern PGLOBALS	pGlobals;

wyBool 
wyIni::Initialize(const wyChar* filename, const wyChar* sec, const wyChar* key)
{
    EnterCriticalSection(&pGlobals->m_csiniglobal);
	InitializeContent();
	
	m_errormsg = NULL;
	m_flag  = REPLACE;
    m_sectionnameonly = wyFalse;
    m_filename.SetAs(filename);
	
    if(LoadFile(m_filename.GetString(), sec, key) == wyFalse)
		return wyFalse;

	return wyTrue;
}

void 
wyIni::SetSectionNameOnly(wyBool issectiononly)
{
    m_sectionnameonly = issectiononly;
}

void 
wyIni::Finialize(wyBool iswrite)
{

    if(iswrite == wyTrue)
    SaveFile();

    FreeListData();

    LeaveCriticalSection(&pGlobals->m_csiniglobal);
}

// get value
wyInt32
wyIni::IniGetString(const wyChar *sec,const wyChar *key, const wyChar *defval, wyString *returnedstring, const wyChar *path)
{
    wyIni       inimngr;
    wyInt32     noofchars;
    
    if(!path)
    {
        returnedstring->SetAs(defval);
        noofchars = strlen(defval);
        return noofchars;
    }

    inimngr.Initialize(path, sec, key);
    
    noofchars = inimngr.GetValue(sec, key, defval, returnedstring);
    inimngr.Finialize(wyFalse);
    return noofchars;
}

wyInt32  
wyIni::IniGetInt(const wyChar *sec,const wyChar *key, wyInt32 defval, const wyChar *path)
{
    wyIni       inimngr;
    wyChar      defvalbuffer[20] = {0};
    wyInt32     noofchars;
    wyString    retint;

    if(!path)
        return defval;

    inimngr.Initialize(path, sec, key);
    
    noofchars = inimngr.GetValue(sec, key, itoa(defval, defvalbuffer, 10), &retint);  

    if(retint.GetLength() == 0)
        retint.SetAs(defvalbuffer);

    inimngr.Finialize(wyFalse);
    return retint.GetAsInt32();
}

wyInt32
wyIni::IniGetSection(wyString *allsecnames, wyString *path)
{
    wyString retstr;
    wyIni    inimngr;
    wyInt32  noofsections;
    
    inimngr.Initialize(path->GetString());
    inimngr.SetSectionNameOnly(wyTrue);
    noofsections = inimngr.GetAllSectionDetails(allsecnames);
    inimngr.Finialize(wyFalse);

    return noofsections;
}


void
wyIni::IniGetSectionDetailsInit(wyString *sec, wyString *path)
{
    //Initialize(path->GetString(),sec->GetString());
	Initialize(path->GetString());
}

void
wyIni::IniGetSectionDetailsFinalize()
{
	Finialize(wyFalse);
}

wyInt32
wyIni::IniGetString2(const wyChar *sec,const wyChar *key, const wyChar *defval, wyString *returnedstring, const wyChar *path)
{
    
    wyInt32     noofchars;
    
    if(!path)
    {
        returnedstring->SetAs(defval);
        noofchars = strlen(defval);
        return noofchars;
    }

    
    
    noofchars = GetValue(sec, key, defval, returnedstring);
   
    return noofchars;
}

wyInt32  
wyIni::IniGetInt2(const wyChar *sec,const wyChar *key, wyInt32 defval, const wyChar *path)
{
    
    wyChar      defvalbuffer[20] = {0};
    wyInt32     noofchars;
    wyString    retint;

    if(!path)
        return defval;

    
    
    noofchars = GetValue(sec, key, itoa(defval, defvalbuffer, 10), &retint);  

    if(retint.GetLength() == 0)
        retint.SetAs(defvalbuffer);

    
    return retint.GetAsInt32();
}


wyBool
wyIni::IniIsConnectionExists(const wyWChar *path)
{
    FILE		*in_stream = NULL;
    wyUInt32	trycount = 0;
    wyChar		buffer[1024] = {0};
    wyIni		mgr;
    wyInt32     c = 0;
    wyChar		*pdest = NULL;
    wyInt32		index = 0, len = 0;
    wyChar		current_section[1024] = {0};
    wyString    temp;
    wyBool      isConSec = wyFalse;


    if(!path)
        return wyFalse;

    EnterCriticalSection(&pGlobals->m_csiniglobal);

    do
	{
		if((in_stream = _wfopen(path, L"rb")))
		{
			break;
		}
        else
		{
			if(GetLastError() == ERROR_FILE_NOT_FOUND)
			{
                LeaveCriticalSection(&pGlobals->m_csiniglobal);
				return wyFalse;				
			}

			Sleep(FILE_LOCK_WAIT);
			trycount++;	
		}

	}while(trycount <= FILE_LOCK_WAIT_TRY_COUNT);
	
	if(trycount > FILE_LOCK_WAIT_TRY_COUNT)
	{
		MessageBox(NULL, _(L"Files are inaccessible"), L"SQLyog", MB_ICONERROR | MB_OKCANCEL); 
        LeaveCriticalSection(&pGlobals->m_csiniglobal);
        return wyFalse;
    }

    while(fgets(buffer,sizeof(buffer) -1,in_stream) != NULL)
	{
        buffer[sizeof(buffer) -1] = '\0';

        if(!c)
            mgr.StripUTF8Bom(buffer);
        c++;

		mgr.Trim(buffer);

        if(strlen(buffer) == 0)
            continue;

        if(buffer[0] == '[')
        {
            pdest = strchr(buffer,']');
			if(pdest != NULL)
			{
			    index = pdest - buffer;
				::memcpy(current_section, buffer+1, index-1);
				current_section[index-1] = '\0';
                temp.SetAs(current_section);
                if( strstr(temp.GetString(), "Connection ") != NULL)
                {
                    isConSec = wyTrue;
                }
                else
                {
                    isConSec = wyFalse;
                }
            }
        }
        else if(isConSec == wyTrue)
        {
            len = 0;
            len = strlen(buffer);
            if(strncmp(buffer, "Name=", 5) == 0)
            {
                if(len > 5)
                {
                    pdest = buffer+5;
                    mgr.Trim(pdest);
                    if(strlen(pdest) > 0)
                    {
                        ::fclose(in_stream);
                        LeaveCriticalSection(&pGlobals->m_csiniglobal);
                        return wyTrue;	
                    }
                }
            }
        }
    }
    ::fclose(in_stream);
    LeaveCriticalSection(&pGlobals->m_csiniglobal);
    return wyFalse;
}

//transfers complete section from one file to another
wyBool
wyIni::IniTransferFullSection(const wyChar* fromSec, const wyChar* toSec, 
								const wyChar* fromPath, const wyChar* toPath)
{
	FILE		*in_stream = NULL;
	FILE		*out_stream = NULL;
	wyString	fname, temp;
	wyUInt32	trycount = 0;
	wyChar		buffer[1024] = {0};
	wyChar		current_section[1024] = {0};
	wyInt32		index = 0;
	wyInt32		c = 0;
	wyIni		mgr;
	wyChar		*pdest = NULL;
    wyInt32     len = strlen(fromSec);
	
	fname.SetAs(fromPath);
	
	if(!fromPath)
		return wyFalse;

	EnterCriticalSection(&pGlobals->m_csiniglobal);

	do
	{
		if((in_stream = _wfopen(fname.GetAsWideChar(), L"rb")))
		{
			break;
		}

		else
		{
			if(GetLastError() == ERROR_FILE_NOT_FOUND)
			{
                LeaveCriticalSection(&pGlobals->m_csiniglobal);
				return wyFalse;				
			}

			Sleep(FILE_LOCK_WAIT);
			trycount++;	
		}

	}while(trycount <= FILE_LOCK_WAIT_TRY_COUNT);
	
	if(trycount > FILE_LOCK_WAIT_TRY_COUNT)
	{
		MessageBox(NULL, _(L"Files are inaccessible"), L"SQLyog", MB_ICONERROR | MB_OKCANCEL); 
        LeaveCriticalSection(&pGlobals->m_csiniglobal);
        return wyFalse;
    }


	while(fgets(buffer,sizeof(buffer) -1,in_stream) != NULL)
	{
        buffer[sizeof(buffer) -1] = '\0';

        if(!c)
            mgr.StripUTF8Bom(buffer);
        c++;

		mgr.Trim(buffer);

        
        if(strlen(buffer) == 0)
            continue;

		if(buffer[0] == '[')
		{
			pdest = strchr(buffer,']');
			if(pdest != NULL)
			{
				index = pdest - buffer;
				memcpy(current_section, buffer+1, index-1);
				current_section[index-1] = '\0';

                if( index > 1 && (index - 1) == len &&memcmp(current_section, fromSec, len) == 0)
				//if( strcmp(current_section, "") != 0 && memcmp(fromSec, current_section, index-1) == 0)
				{
					fname.SetAs(toPath);
					trycount = 0;

					do
					{
						if((out_stream = _wfopen(fname.GetAsWideChar(), L"ab")))
						{
							break;
						}

						else
						{
							if(GetLastError() == ERROR_FILE_NOT_FOUND)
							{
                                fclose(in_stream);
                                LeaveCriticalSection(&pGlobals->m_csiniglobal);
                                return wyFalse;				
							}

							Sleep(FILE_LOCK_WAIT);
							trycount++;	
						}

	                }while(trycount <= FILE_LOCK_WAIT_TRY_COUNT);
	
	                if(trycount > FILE_LOCK_WAIT_TRY_COUNT)
	                {
		                MessageBox(NULL, _(L"Files are inaccessible"), L"SQLyog", MB_ICONERROR | MB_OKCANCEL); 
		                fclose(in_stream);
                        LeaveCriticalSection(&pGlobals->m_csiniglobal);
                        return wyFalse;
                        //exit(0);
	                }

                    temp.Sprintf("[%s]\r\n", toSec);
                    fputs(temp.GetString(), out_stream);

                    
                    while(fgets(buffer, sizeof(buffer) - 1, in_stream) != NULL)
				    {
                        buffer[sizeof(buffer) -1] = '\0';

		                mgr.Trim(buffer);

                        if(strlen(buffer) == 0)
                            continue;

		                if(buffer[0] == '[')
                        {
                            break;
                        }
                        else
                        {
                            temp.Sprintf("%s\r\n",buffer);
                            fputs(temp.GetString(), out_stream);
                        }
					}
                    fclose(in_stream);
                    fclose(out_stream);
                    LeaveCriticalSection(&pGlobals->m_csiniglobal);
                    return wyTrue;
				}
		    }
	    }		
    }

    if(in_stream)
        fclose(in_stream);
   
	LeaveCriticalSection(&pGlobals->m_csiniglobal);
    return wyFalse;
}




// set value if exitst will be replace
wyBool 
wyIni::IniWriteString(const wyChar *sec,const wyChar *key,  const wyChar *value, const wyChar *path)
{
    wyIni mngr;
    wyBool  issucess;    

    if(!path)
        return wyFalse;

    mngr.Initialize(path);
    issucess = mngr.SetValue(sec,key,value,"",REPLACE);
    mngr.Finialize(wyTrue);
	return issucess;	
}

wyBool 
wyIni::IniWriteInt(const wyChar *sec,const wyChar *key,  wyInt32 value, const wyChar *path)
{
    wyIni  mngr;
    wyBool      issucess;
    wyChar      buffer[40];

    if(!path)
        return wyFalse;

    mngr.Initialize(path);
    wyChar  *val = itoa(value, buffer, 10);
    issucess = mngr.SetValue(sec,key,val,"",REPLACE);
    mngr.Finialize(wyTrue);
	return 	issucess;
}

wyInt32
wyIni::IniDeleteSection(const wyChar *sec, const wyChar *path)
{
	wyIni    inimngr;
	wyInt32	 issuccess; 
	inimngr.Initialize(path);
	issuccess = inimngr.RemoveSection(sec);	
	inimngr.Finialize(wyTrue);
	return issuccess;
}

wyInt32
wyIni::IniDeleteKey(const wyChar *sec, wyChar *key, const wyChar *path)
{
	wyIni	inimngr;
	wyInt32	issuccess;

	inimngr.Initialize(path);
	issuccess = inimngr.RemoveSel(sec, key);
	inimngr.Finialize(wyTrue);
    
	return issuccess;
}

wyInt32
wyIni::GetAllSectionDetails(wyString *secnames)
{
    // _save
	struct      tagSection *sec = m_content->m_first;
    wyInt32     noofsections = 0;

	while (sec != NULL)
	{
        if(strstr(sec->m_name.GetString(), "Connection"))
        {
            secnames->Add(sec->m_name.GetString());
            secnames->Add(";");
            noofsections++;   
        }
		sec = sec->m_next;		
	}	
	return noofsections;
}

// get value
wyInt32
wyIni::GetValue(const wyChar *sec,const wyChar *key, const wyChar *defval, wyString *returnedstring)
{
    struct  tagSection *st = GetSection(sec);
    struct  tagRecord *result;
    wyInt32 noofchars;

    if(!st)
    {
        noofchars = strlen(defval);
        returnedstring->SetAs(defval);
        return noofchars;
    }

	result =	GetRecord(st,key);	

	if(result != NULL)
    {
        returnedstring->SetAs(result->m_value.GetString());
        return result->m_value.GetLength();
    }
	/*else if(allkeylength == 1 || allkeylength == 0)
    {
		return allkeylength;
    }*/
    else
    {
        noofchars = strlen(defval);
        returnedstring->SetAs(defval);
        return noofchars;
    }
}

// load
wyBool 
wyIni::LoadFile(const wyChar *filename, const wyChar* secname, const wyChar* keyname)
{
	FILE *in_stream;
	wyChar buffer[1024]; // earlier size is 255, then if it contains more than 255 then it is truncating.
	wyChar comments[1024];
	wyChar current_section[1024];	
	wyChar key[1024];
	wyChar value[1024];
	wyChar *pdest;
	wyInt32  index;
    wyInt32 c = 0;
    wyBool  addsec = wyTrue;
    wyBool  addseccomplete = wyFalse;
//	DWORD	errornum = 0;

	strcpy(comments,"");
	strcpy(current_section,"");
	m_errormsg = NULL;

    wyString fname;
	wyInt32 trycount = 0;

    if(secname != NULL)
        addsec = wyFalse;

    fname.SetAs(filename);
	do
	{
		if((in_stream = _wfopen(fname.GetAsWideChar(), L"rb")))
		{
			break;
		}

		else
		{
			if(GetLastError() == ERROR_FILE_NOT_FOUND)
			{				
				m_errormsg = "open file error";

				return wyFalse;				
			}

			Sleep(FILE_LOCK_WAIT);
			trycount++;	
		}

	}while(trycount <= FILE_LOCK_WAIT_TRY_COUNT);
	
	if(trycount > FILE_LOCK_WAIT_TRY_COUNT)
	{
		m_errormsg = "open file error";
		MessageBox(NULL, _(L"SQLyog is getting terminated"), L"SQLyog", MB_ICONERROR | MB_OKCANCEL); 

		exit(0);
		//return wyFalse;	
	}
	
	while(fgets(buffer,sizeof(buffer) -1,in_stream) != NULL)
	{
        buffer[sizeof(buffer) -1] = '\0';

        if(!c)
            StripUTF8Bom(buffer);
        c++;

		Trim(buffer);

        // We need to ignore if '\n' are there in ini file.. if they are present then Trimbuffer() would trim it off and 
        // this results in strlen(buffer) will be zero. so we need to continue in that case.

        if(strlen(buffer) == 0)
            continue;

		switch(buffer[0])
		{
			case '[' : // section;
				pdest = strchr(buffer,']');
				if (pdest == NULL)
				{
                    // skip this
					break;
				}
				index = pdest - buffer;
				memcpy(current_section,buffer + 1,index - 1);
				current_section[index - 1] = '\0';

                if(secname != NULL && addsec == wyTrue)
                {
                    addseccomplete = wyTrue;
                    break;
                }

                if(secname != NULL)
                        addsec = wyFalse;
                
                if((addsec == wyFalse && stricmp(current_section, secname) == 0) || addsec == wyTrue) 
                {
					AddSection(current_section,comments);	
                    addsec = wyTrue;
                }

				strcpy(comments,"");
				break;
			case '#' : // comment
			case ';' :
                /*
				if(strlen(comments) > 0)
					strcat(comments,"\n");
				strcat(comments,buffer);
                */
				break;
            
			default : // find content

                if(addsec == wyFalse || m_sectionnameonly == wyTrue)
                    break;

				pdest = strchr(buffer,'=');
				if (pdest == NULL) 
				{
                    // skip this
					break;
				}
				index = pdest - buffer;
				memcpy(key,buffer,index);
				key[index] = '\0';
				memcpy(value,buffer + index + 1,strlen(buffer)-index);
				
				if(strcmp(current_section,"") == 0)
				{
                    // skip this
					break;
				}
				else
				{
                    if(!keyname || strcmp(key, keyname) == 0)
						Append(current_section,key,value);

					strcpy(comments,"");
				}
				break;
		}

        if(addseccomplete == wyTrue)
            break;
	}

	fclose(in_stream);
	
	return wyTrue;
}

// _save
wyBool 
wyIni::SaveFile(const wyChar *filename)
{
	wyString    filenamestr, tempbuff;
	struct      tagSection *sec = m_content->m_first;
	struct      tagRecord *rec = NULL;
	DWORD		byteswritten;
	
    if(filename)
        filenamestr.SetAs(filename);

	m_errormsg = NULL;

//	FILE *in_stream;
//	DWORD tt;

	//in_stream = _wfopen(filenamestr.GetAsWideChar(), L"rb" );
	//tt = GetLastError();
	
	//Check for .ini file is locked or not if locked return it.
	if(IsIniFileLocked(&filenamestr) == wyTrue)
	{
		//MessageBox(NULL, L"SQLyog is getting terminated", L"SQLyog", MB_ICONERROR | MB_OKCANCEL); 
		//exit(0);
		return wyFalse;
	}

		
	while (sec != NULL)
	{
		
        //in_stream = _wfopen(filenamestr.GetAsWideChar(), L"rb" );
		//tt = GetLastError();

		if(sec->m_comments.GetLength() != 0)
		{
           	tempbuff.Sprintf("%s\r\n",sec->m_comments.GetString());
			WriteFile(m_lockfile.m_hfile, tempbuff.GetString(), tempbuff.GetLength(), &byteswritten, NULL);
			
		}
        tempbuff.Sprintf("[%s]\r\n",sec->m_name.GetString());
		WriteFile(m_lockfile.m_hfile, tempbuff.GetString(), tempbuff.GetLength(), &byteswritten, NULL);
		// print section content
		rec = sec->m_datafirst;
		while(rec != NULL)
		{
            if(rec->m_comments.GetLength() != 0)
			{
                tempbuff.Sprintf("%s\r\n",rec->m_comments.GetString());
				WriteFile(m_lockfile.m_hfile, tempbuff.GetString(), tempbuff.GetLength(), &byteswritten, NULL);
			}

            tempbuff.Sprintf("%s=%s\r\n",rec->m_key.GetString(),rec->m_value.GetString());
			WriteFile(m_lockfile.m_hfile, tempbuff.GetString(), tempbuff.GetLength(), &byteswritten, NULL);
			
			rec = rec->m_next;
		}		
		sec = sec->m_next;		
	}	
	
	//Unlock the ini file
	m_lockfile.Close();

	return wyTrue;
}

wyBool
wyIni::IsIniFileLocked(wyString *filename)
{
	wyInt32 trycount = 0;

	m_lockfile.SetFilename(filename);

	//Wait  for 30 seconds to release the .ini file else return
	do
	{
		if(m_lockfile.OpenWithPermission(GENERIC_WRITE, CREATE_ALWAYS) != -1)
		{
			return wyFalse; //ini is not locked
		}

		Sleep(FILE_LOCK_WAIT);
		trycount++;	

	}while(trycount <= FILE_LOCK_WAIT_TRY_COUNT);
    
	//Sleep(2000);

	return wyTrue;  //ini is locked	
}

void
wyIni::FreeListData()
{
    tagSection *sec;
    tagRecord *rec;

    sec = m_content->m_first;
    tagRecord *temprec;
    tagSection *tempsec;

    while (sec != NULL)
	{
        tempsec = sec->m_next;
        rec = sec->m_datafirst;
        while(rec != NULL)
		{
            temprec = rec->m_next;
            delete rec;
            rec = temprec;
        }
        delete sec;
        sec = tempsec;    
    }
	delete m_content;
}
// save to default file
wyBool 
wyIni::SaveFile()
{
    return SaveFile(m_filename.GetString());
}

// save as (for users)
wyBool 
wyIni::SaveFileAs(const wyChar *filename)
{
	return SaveFile(filename);
}


wyChar *
wyIni::GetValue(const wyChar *sec,const wyChar *key,  wyChar *comment)
{
    struct tagSection *st = GetSection(sec);
    struct tagRecord *result;

    if(!st)
    {
        strcpy(comment,"");
        return "";
    }

	result =	GetRecord(st,key);	

	if(result != NULL)
	{
        strcpy(comment,result->m_comments.GetString());
        return (wyChar *)result->m_value.GetString();
	}	
	else
	{
		strcpy(comment,"");
		return "";
	}
}

wyBool 
wyIni::SetValue(const wyChar *sec,const wyChar *key,  const wyChar *value,const wyChar *comment,REPLACE_FLAG flag)
{
	m_flag = flag;
	m_errormsg = NULL;
	Append(sec,key,value);
	if(m_errormsg == NULL)
		return wyFalse;
	else
		return wyTrue;
}

// get last error
wyChar *
wyIni::GetLastErr()
{
	return m_errormsg;
}

/* helper function section                                              */

//////////////////////////////////////////////////////////////////////////////////////////////////////
// init list of ini file
void 
wyIni::InitializeContent()
{
	//m_content = (content *)malloc(sizeof(content));
	 m_content = new  tagContent;
	if(m_content == NULL)
	{
		m_errormsg = "cannot malloc memory !";
		return;
	}
	
	m_content->m_sectionsize = 0;	
	m_content->m_first = NULL;
	m_content->m_last = NULL;
}

// add section
void 
wyIni::AddSection(const wyChar *sec,const wyChar *comment)
{
	struct tagSection *temp;
	temp = GetSection(sec);
	
	m_errormsg = NULL;

	if(temp == NULL)
	{
        temp = new  tagSection;
			
		if(temp == NULL)
		{
			m_errormsg = "cannot malloc memory !";
			return;
		}
		
		// for section name
        if(sec)
            temp->m_name.SetAs(sec);

		if((comment[0] != '#' && comment[0] != ';') && (strlen(comment) > 0))
            temp->m_comments.Sprintf("#%s",comment);
		else if(comment)
            temp->m_comments.SetAs(comment);

		// for data link
		temp->m_datafirst = NULL;
		temp->m_datalast = NULL;
		temp->m_next = NULL;
		temp->m_datasize = 0;

		// increment section size
		m_content->m_sectionsize++;

		// for content link
		if (m_content->m_first == NULL)
		{
			m_content->m_first = temp;
			m_content->m_last  = temp;
		}
		else
		{
			m_content->m_last->m_next = temp;
			m_content->m_last = temp;
		}	
	}
	else if(m_flag == REPLACE)
	{
        temp->m_name.SetAs(sec);
		if((comment[0] != '#' || comment[0] != ';') && (strlen(comment) > 0))
            temp->m_comments.Sprintf("#%s",comment);
		else if(comment)
            temp->m_comments.SetAs(comment);
	}
}

// append list
void 
wyIni::Append(const wyChar *sec,const wyChar *key,const wyChar *value)
{
	struct tagSection *tmp_sec;
	struct tagRecord *temp;	
	
	// find section
	tmp_sec = GetSection(sec);

	if(tmp_sec != NULL)
	{
		temp = GetRecord(tmp_sec,key);
		if(temp == NULL)
		{
			//temp = (struct record *)malloc(sizeof(struct record));
            temp = new  tagRecord;
			if(temp == NULL)
			{
				m_errormsg = "cannot malloc memory !";
				return;
			}
			temp->m_next = NULL;	
			
            if(key)
                temp->m_key.SetAs(key);
            if(value)
                temp->m_value.SetAs(value);			

			tmp_sec->m_datasize++;

			if (tmp_sec->m_datafirst == NULL)
			{
				tmp_sec->m_datafirst = temp;
				tmp_sec->m_datalast  = temp;
			}
			else
			{
				tmp_sec->m_datalast->m_next = temp;
				tmp_sec->m_datalast = temp;
			}			
		}
		else if(m_flag == REPLACE)
		{
            temp->m_key.SetAs(key);
            temp->m_value.SetAs(value);
		}
		
	}
	else
	{
		AddSection(sec,"");
		Append(sec,key,value);
	}
}


// search and get section
struct tagSection *
wyIni::GetSection(const wyChar *sec)
{
	wyBool found = wyFalse;
	struct tagSection *esection;
    esection = m_content->m_first;

	while (esection != NULL)
	{	
        if(strcmp(esection->m_name.GetString(),sec) == 0)
		{
			found = wyTrue;
			break;
		}		
		esection = esection->m_next;
	}

	if(found == wyTrue)
		return esection;
	else
		return NULL;
};

// search and get record
struct tagRecord *
wyIni::GetRecord(struct tagSection *sec,const wyChar *key)
{
	wyBool      found = wyFalse;
	struct      tagRecord *tmp;

	tmp = sec->m_datafirst;

	while(tmp != NULL)
	{
        if(key == NULL)
        {
            /*
            if(tmp->m_key.GetLength() > 0)
                *allkeylen = 1;
            else
                *allkeylen = 0;*/
            
            return NULL;
        }
        else
        {
            if(strcmp(key,tmp->m_key.GetString()) == 0)
		    {
			    found = wyTrue;
    			break;
	    	}
		    
            tmp = tmp->m_next;
        }
	}

	if(found == wyTrue)
	{
		return tmp;
	}
	else
	{
        
		return NULL;
	}
}

// remove list //return num of remove 0 nothing to remove 1 is success
wyInt32 
wyIni::Remove(const wyChar *sec,const wyChar *key)
{	
	struct tagSection *temp_sec = GetSection(sec);
	struct tagRecord *tmp,*tmp2;
	wyInt32	remove = 0;
	
	if(temp_sec == NULL)
		return 0;

	tmp = temp_sec->m_datafirst;
	
	if(tmp == NULL)
		return 0;
	
    if(strcmp(key,tmp->m_key.GetString()) == 0)
	{
		temp_sec->m_datafirst = tmp->m_next;
		temp_sec->m_datasize--;
        delete tmp;
		return 1;
	}
	
	while(tmp != NULL)
	{
		if(tmp->m_next != NULL)
		{
            if(strcmp(key,tmp->m_next->m_key.GetString()) == 0)
			{	
				tmp2 = tmp->m_next;				
				tmp->m_next = tmp->m_next->m_next;
				temp_sec->m_datasize--;
				delete tmp2;				
				remove = 1;
				break;
			}
		}		
		tmp = tmp->m_next;
	}		
	return remove;
}

// remove all record
wyInt32 
wyIni::RemoveAll(const wyChar *sec)
{
	struct tagSection *temp_sec = GetSection(sec);
	struct tagRecord *tmp;
	wyInt32 remove = 0;
	
	if(temp_sec == NULL)
		return 0;

	tmp = temp_sec->m_datafirst;
	while(tmp != NULL)
	{
		temp_sec->m_datafirst = tmp->m_next;
		temp_sec->m_datasize--;
		free(tmp);
		remove++;
		tmp = temp_sec->m_datafirst;
	}
	return remove;
}

// remove selection record
wyInt32 
wyIni::RemoveSel(const wyChar *sec,wyChar *key)
{
	return Remove(sec,key);
}

// remove all record
wyInt32  
wyIni::RemoveAll(struct tagSection *sec)
{
	struct tagRecord *tmp;
	wyInt32 remove = 0;

	if(sec == NULL)
		return 0;

	tmp = sec->m_datafirst;
	while(tmp != NULL)
	{
		sec->m_datafirst = tmp->m_next;
		sec->m_datasize--;
		free(tmp);
		remove++;
		tmp = sec->m_datafirst;
	}
	return remove;
}

// remove section
wyInt32  
wyIni::RemoveSection(const wyChar *sec)
{
	struct tagSection *esection = m_content->m_first,*temp;

	if(esection == NULL)
		return 0;
	
    if(strcmp(sec,esection->m_name.GetString()) == 0)
	{
		RemoveAll(esection);
		m_content->m_first = esection->m_next;
		m_content->m_sectionsize--;
		free(esection);
		return 1;
	}
	
	while (esection != NULL)
	{	
        if(strcmp(esection->m_next->m_name.GetString(),sec) == 0)
		{
			RemoveAll(esection->m_next);
			temp = esection->m_next;				
			esection->m_next = esection->m_next->m_next;
			m_content->m_sectionsize--;
			free(temp);				
			break;
		}		
		esection = esection->m_next;
	}		
	return 1;
}

// clear all content
void
wyIni::Clear()
{
	struct tagSection *tmp;
	if(m_content == NULL)
		return;

	tmp = m_content->m_first;
	while(tmp != NULL)
	{
		m_content->m_first = tmp->m_next;
		m_content->m_sectionsize--;
		free(tmp);
		tmp = m_content->m_first;
	}	
}

// get size of content (number of section
wyInt32 
wyIni::ContentSize()
{
	return m_content->m_sectionsize;
}

// get size of selection section
wyInt32 
wyIni::SectionSize(wyChar *sec)
{
	struct tagSection *temp = GetSection(sec);
	return temp->m_datasize;
}

// trime ' ' \n\t\r
void 
wyIni::Trim(wyChar *buffer)
{
	if(buffer[strlen(buffer)-1] == '\n')
		buffer[strlen(buffer)-2] = '\0';
}

void 
wyIni::StripUTF8Bom(wyChar *buffer)
{
    if(!buffer)
        return;

    wyInt32 len = strlen(buffer);
    //0xEF, 0xBB and 0xBF are BOM's for UTF-8
    //if it is a utf-8 encoded file, here it is checked.
    if(len > 2 && (unsigned char)buffer[0] == 0xEF && (unsigned char)buffer[1] == 0xBB && (unsigned char)buffer[2] == 0xBF)
    {
        memmove(buffer, buffer + 3, len - 3);
        buffer[len - 3] = '\0';
    }

    return;
}
