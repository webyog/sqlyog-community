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

#include <iostream>
#include "L10nText.h"
#include "CommonHelper.h"
#include "WyCrc.h"

L10nText*   L10nText::m_l10nt = NULL;
wyInt32     L10nText::m_sqliteretcode = 0;
wyString    L10nText::m_sqlitemsg;
wyChar*     L10nText::m_sourcefile = NULL;
wyInt32     L10nText::m_sourceline = 0;

//--------------------------------- L10n APIs
wyInt32 
InitL10n(const wyChar* langcode, const wyChar* dictionary, wyBool ismemoryindex, wyBool islogerror, wyBool islogaccess)
{    
    if(L10nText::Allocate() == wyFalse)
    {
        return 1;
    }

    return L10nText::Init(langcode, dictionary, ismemoryindex, islogerror, islogaccess);
}

void 
CloseL10n()
{
    L10nText::Release();
}

#ifdef _DEBUG
wyChar* 
GetL10nText(const wyChar* str, wyChar* sourcefile, wyInt32 sourceline)
{
    L10nText::m_sourcefile = sourcefile;
    L10nText::m_sourceline = sourceline;
    return (wyChar*)L10nText::GetText(str);
}

wyWChar* 
GetL10nText(const wyWChar* str, wyChar* sourcefile, wyInt32 sourceline)
{
    L10nText::m_sourcefile = sourcefile;
    L10nText::m_sourceline = sourceline;
    return (wyWChar*)L10nText::GetText(str);
}
#else
wyChar* 
GetL10nText(const wyChar* str)
{
    return (wyChar*)L10nText::GetText(str);
}

wyWChar* 
GetL10nText(const wyWChar* str)
{
    return (wyWChar*)L10nText::GetText(str);
}
#endif

wyInt32          
GetLastL10nSQliteCode()
{
    return L10nText::GetLastSQliteCode();
}

const wyChar*      
GetLastL10nSQliteMsg()
{
    return L10nText::GetLastSQliteMsg();
}

Language*
GetL10nLanguages(const wyChar* dbname)
{
    return L10nText::GetLanguages(dbname);
}

wyInt32 
GetL10nLanguageCount(const wyChar* dbname)
{
    return L10nText::GetLanguageCount(dbname);
}

void
FreeL10nLanguages(Language* plang)
{
    delete[] plang;
    plang = NULL;
}

const wyChar*
GetL10nLangcode()
{
    return L10nText::GetLangCode();
}

wyInt32
GetL10nDBVersion(const wyChar* dbname)
{
    return L10nText::GetDBVersion(dbname);
}

wyInt32
GetL10nFontNameAndSize(const wyChar** fontname)
{
    return L10nText::GetFontNameAndSize(fontname);
}

#ifdef WIN32
HFONT
GetL10nFont()
{
    return L10nText::GetFont();
}
#endif

//---------------------------------

LangString::LangString(wyInt32 id, const wyChar* string)
{
    m_string = NULL;
    m_string = new wyString(string, strlen(string) + 1);
    m_id = id;
}

LangString::~LangString()
{
    delete m_string;
}

L10nText::L10nText()
{
#ifdef WIN32
    InitializeCriticalSection(&m_cs);
    m_hfont = NULL;
#endif
    m_index = NULL;
    m_indexcount = 0;
    m_islogerror = wyFalse;
	m_islogaccess = wyFalse;
    m_pindexsqlite = NULL;
    m_ismemoryindex = wyFalse;
    m_isen = wyFalse;
    m_fontsize = 0;
}

L10nText::~L10nText()
{
    if(m_index)
    {
        ReleaseStringIndex();
    }

    if(m_ismemoryindex == wyTrue && m_pindexsqlite)
    {
        delete m_pindexsqlite;
    }

#ifdef WIN32
    if(m_hfont)
    {
        DeleteObject(m_hfont);
    }

    DeleteCriticalSection(&m_cs);
#endif
}

wyBool
L10nText::Allocate()
{
    delete L10nText::m_l10nt;
    m_sqlitemsg.Clear();
    m_sqliteretcode = 0;
    L10nText::m_l10nt = new(std::nothrow) L10nText();

    if(L10nText::m_l10nt == NULL)
    {
        return wyFalse;
    }

    return wyTrue;
}

void
L10nText::Release()
{
    delete L10nText::m_l10nt;
    L10nText::m_l10nt = NULL;
}

wyInt32
L10nText::Init(const wyChar* langcode, const wyChar* dictionary, wyBool ismemoryindex, wyBool islogerror, wyBool islogaccess)
{
    wyInt32 ret;
    
#ifdef WIN32
    EnterCriticalSection(&m_l10nt->m_cs);
#endif
    if(!strcmp(langcode, "en"))
    {
        m_l10nt->m_isen = wyTrue;
    }

    ret = m_l10nt->Initialize(langcode, dictionary, ismemoryindex, islogerror, islogaccess);
#ifdef WIN32
    LeaveCriticalSection(&m_l10nt->m_cs);
#endif

    if(ret != 0)
    {
        if(ret == L10NT_SQLITE_ERROR)
        {
            m_sqliteretcode = m_l10nt->m_sqlite.GetLastCode();
            m_sqlitemsg.SetAs(m_l10nt->m_sqlite.GetErrMsg());
        }

        Release();
    }

    return ret;
}

wyInt32          
L10nText::GetLastSQliteCode()
{
    if(m_l10nt)
    {
        return m_l10nt->m_sqlite.GetLastCode();
    }

    return m_sqliteretcode;
}

const wyChar*      
L10nText::GetLastSQliteMsg()
{
    if(m_l10nt)
    {
        return m_l10nt->m_sqlite.GetErrMsg();
    }

    return m_sqlitemsg.GetString();
}

wyInt32
L10nText::Initialize(const wyChar* langcode, const wyChar* dictionary, wyBool ismemoryindex, wyBool islogerror, wyBool islogaccess)
{
    wyString        query, size;
    wyInt32         i = 0;
    const wyChar    *pfont = NULL, *pfontsize = NULL, *text;

#ifdef WIN32
    LOGFONT         lf;
    HFONT           hfont;
    HDC             hdc;
#endif

    m_sqlite.SetLogging(islogerror);

    if(m_sqlite.Open(dictionary, wyTrue) == wyFalse)
    {
        return L10NT_SQLITE_ERROR;
    }
    
    m_langcode.SetAs(langcode);
    m_dictionary.SetAs(dictionary);
    m_islogerror = islogerror;
	m_islogaccess = islogaccess;
    m_ismemoryindex = ismemoryindex;
    query.Sprintf("select fontname, fontsize from langindex where langcode = '%s'", m_langcode.GetString());
    m_sqlite.Prepare(NULL, query.GetString());
    
    while(m_sqlite.Step(NULL, wyFalse) && m_sqlite.GetLastCode() == SQLITE_ROW)
    {
        if((pfont = m_sqlite.GetText(NULL, "fontname")))
        {
            m_fontname.SetAs(pfont);
        }

        if((pfontsize = m_sqlite.GetText(NULL, "fontsize")))
        {
            size.SetAs(pfontsize);
        }

        break;
    }

    m_sqlite.Finalize(NULL);

#ifdef WIN32
    hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    GetObject(hfont, sizeof(LOGFONT), &lf);
    
    if(m_fontname.GetLength())
    {
        hdc = GetDC(NULL);
        wcscpy(lf.lfFaceName, m_fontname.GetAsWideChar());
        
        if(size.GetLength())
        {
            m_fontsize = atoi(size.GetString());
            lf.lfHeight = -MulDiv(m_fontsize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
            lf.lfWidth = 0;
        }
        else
        {
            m_fontsize = -MulDiv(lf.lfHeight, 72, GetDeviceCaps(hdc, LOGPIXELSY));
        }

        ReleaseDC(NULL, hdc);
    }

    if((m_hfont = CreateFontIndirect(&lf)))
    {
        DeleteObject(hfont);
    }
    else
    {
        m_hfont = hfont;
    }
#endif

#ifndef _DEBUG
    if(m_l10nt->m_isen == wyTrue)
    {
        return 0;
    }
#endif

    query.Sprintf("select count(*) from \"%s\"", m_langcode.GetString());
    m_sqlite.Prepare(NULL, query.GetString());
    
    while(m_sqlite.Step(NULL, wyFalse) && m_sqlite.GetLastCode() == SQLITE_ROW)
    {
        m_indexcount = m_sqlite.GetInt(NULL, 0);
        break;
    }

    m_sqlite.Finalize(NULL);
    m_index = new(std::nothrow) LangString*[m_indexcount];

    if(m_index == NULL)
    {
        m_indexcount = 0;
        return L10NT_ALLOCATION_FAILED;
    }

    query.Sprintf("select * from \"%s\" order by id asc", m_langcode.GetString());
    m_sqlite.Prepare(NULL, query.GetString());
    
    for(i = 0; m_sqlite.Step(NULL, wyFalse) && m_sqlite.GetLastCode() == SQLITE_ROW; ++i)
    {
        text = m_sqlite.GetText(NULL, "string");

        if(text)
        {
            m_index[i] = new(std::nothrow) LangString(m_sqlite.GetInt(NULL, "id"), text);

            if(m_index[i] == NULL)
            {
                m_indexcount = i;
                return L10NT_ALLOCATION_FAILED;
            }
        }
    }

    m_sqlite.Finalize(NULL);

    if(InitIndexTable() == wyFalse)
    {
        return L10NT_SQLITE_ERROR;
    }

    return 0;
}

void
L10nText::ReleaseStringIndex()
{
    wyInt32 i;

    for(i = 0; i < m_indexcount; ++i)
    {
        delete m_index[i];
    }

    delete[] m_index;
    m_index = NULL;
    m_indexcount = 0;
}

wyInt32
L10nText::CompareFunct(const void* elem1, const void* elem2)
{
    LangString** pelem1 = (LangString**)elem1;
    LangString** pelem2 = (LangString**)elem2;

    if((*pelem1)->m_id > (*pelem2)->m_id)
    {
        return 1;
    }
    else if((*pelem1)->m_id < (*pelem2)->m_id)
    {
        return -1;
    }

    return 0;
}

const wyChar* 
L10nText::GetText(const wyChar* str)
{
    LangString      ls(0, "");
    LangString**    presult;
    LangString*     pls = &ls;
    wyString        tempstring, temp;

    if(!m_l10nt)
    {
        return str;
    }

#ifndef _DEBUG
    if(m_l10nt->m_isen == wyTrue)
    {
        return str;
    }
#endif

    tempstring.SetAs(str);

    if(tempstring.GetLength() == 0)
    {
        return str;
    }

#ifdef WIN32
    EnterCriticalSection(&m_l10nt->m_cs);
#endif

	if(m_l10nt->m_islogaccess == wyTrue)
    {
        if(L10nText::m_sourcefile)
        {
            YogDebugLog(L10NT_SEARCH_FAILED, tempstring.GetString(), L10nText::m_sourcefile, L10nText::m_sourceline);
        }
        else
        {
            YOGLOG(L10NT_SEARCH_FAILED, tempstring.GetString());
        }
    }

    if((ls.m_id = m_l10nt->GetStringId(tempstring.GetString())) == -1)
    {
#ifdef WIN32
        LeaveCriticalSection(&m_l10nt->m_cs);
#endif
        return str;
    }

    presult = (LangString**)bsearch(&pls, m_l10nt->m_index, m_l10nt->m_indexcount, sizeof(LangString*), L10nText::CompareFunct);

    if(presult == NULL)
    {
        if(m_l10nt->m_islogerror == wyTrue)
        {
            temp.Sprintf("%d - %s", ls.m_id, tempstring.GetString());

            if(L10nText::m_sourcefile)
            {
                YogDebugLog(L10NT_SEARCH_FAILED, temp.GetString(), L10nText::m_sourcefile, L10nText::m_sourceline);
            }
            else
            {
                YOGLOG(L10NT_SEARCH_FAILED, temp.GetString());
            }
        }
    }

#ifdef WIN32
    LeaveCriticalSection(&m_l10nt->m_cs);
#endif

    return presult ? (*presult)->m_string->GetString() : str;
}

const wyWChar* 
L10nText::GetText(const wyWChar* str)
{
	LangString      ls(0, "");
    LangString**    presult;
    LangString*     pls = &ls;
    wyString        tempstring, temp;

    if(!m_l10nt)
    {
        return str;
    }

#ifndef _DEBUG
    if(m_l10nt->m_isen == wyTrue)
    {
        return str;
    }
#endif

    tempstring.SetAs(str);

    if(tempstring.GetLength() == 0)
    {
        return str;
    }

#ifdef WIN32
    EnterCriticalSection(&m_l10nt->m_cs);
#endif

	if(m_l10nt->m_islogaccess == wyTrue)
    {
        if(L10nText::m_sourcefile)
        {
            YogDebugLog(L10NT_SEARCH_FAILED, tempstring.GetString(), L10nText::m_sourcefile, L10nText::m_sourceline);
        }
        else
        {
            YOGLOG(L10NT_SEARCH_FAILED, tempstring.GetString());
        }
    }

    if((ls.m_id = m_l10nt->GetStringId(tempstring.GetString())) == -1)
    {
#ifdef WIN32
        LeaveCriticalSection(&m_l10nt->m_cs);
#endif
        return str;
    }

    presult = (LangString**)bsearch(&pls, m_l10nt->m_index, m_l10nt->m_indexcount, sizeof(LangString*), L10nText::CompareFunct);

    if(presult == NULL)
    {
        if(m_l10nt->m_islogerror == wyTrue)
        {
            temp.Sprintf("%d - %s", ls.m_id, tempstring.GetString());

            if(L10nText::m_sourcefile)
            {
                YogDebugLog(L10NT_SEARCH_FAILED, temp.GetString(), L10nText::m_sourcefile, L10nText::m_sourceline);
            }
            else
            {
                YOGLOG(L10NT_SEARCH_FAILED, temp.GetString());
            }
        }
    }

#ifdef WIN32
    LeaveCriticalSection(&m_l10nt->m_cs);
#endif

    return presult ? (*presult)->m_string->GetAsWideChar(NULL, wyTrue) : str;
}

wyInt32
L10nText::GetStringId(const wyChar* str)
{
    wyString    query, tempstring;
    wyInt32     id = -1;
    wyUInt32    crcsum;

    tempstring.SetAs(str);
    crcsum = CalculateCRC(str);
    query.Sprintf("select id from en where crc = %u", crcsum);
    m_pindexsqlite->Prepare(NULL, query.GetString());
    
    while(m_pindexsqlite->Step(NULL, wyFalse) && m_pindexsqlite->GetLastCode() == SQLITE_ROW)
    {
        id = m_pindexsqlite->GetInt(NULL, "id");
        break;
    }
    
    m_pindexsqlite->Finalize(NULL);

    if(id == -1)
    {
        if(m_l10nt->m_islogerror == wyTrue)
        { 
            if(L10nText::m_sourcefile)
            {
                YogDebugLog(L10NT_LOOKUP_FAILED, str, L10nText::m_sourcefile, L10nText::m_sourceline);
            }
            else
            {
                YOGLOG(L10NT_LOOKUP_FAILED, str);
            }
        }
    }

    return id;
}

Language*
L10nText::GetLanguages(const wyChar* dbname)
{
    wyInt32 count = 0, i = 0;
    wySQLite* psqlite = NULL;
    Language* plang = NULL;


    if(dbname)
    {
        if((psqlite = new(std::nothrow) wySQLite()) == NULL || psqlite->Open(dbname, wyTrue) == wyFalse)
        {
            delete psqlite;
            return NULL;
        }
    }
    else if(m_l10nt)
    {
        psqlite = &m_l10nt->m_sqlite;
    }
    
    if(psqlite && (count = GetLanguageCount(dbname)) != 0)
    {
        if((plang = new(std::nothrow) Language[count]) != NULL)
        {
            psqlite->Prepare(NULL, "select * from langindex");
    
            while(psqlite->Step(NULL, wyFalse) && psqlite->GetLastCode() == SQLITE_ROW)
            {
                plang[i].m_langcode.SetAs(psqlite->GetText(NULL, "langcode"));
                plang[i++].m_lang.SetAs(psqlite->GetText(NULL, "lang"));
            }

            psqlite->Finalize(NULL);
        }
    }

    if(dbname)
    {
        delete psqlite;
    }

    return plang;
}

wyInt32
L10nText::GetLanguageCount(const wyChar* dbname)
{
    wyInt32 count = 0;
    wySQLite* psqlite = NULL;

    if(dbname)
    {
        if((psqlite = new(std::nothrow) wySQLite()) == NULL || psqlite->Open(dbname, wyTrue) == wyFalse)
        {
            delete psqlite;
            return 0;
        }
    }
    else if(m_l10nt)
    {
        psqlite = &m_l10nt->m_sqlite;
    }

    if(psqlite && psqlite->Prepare(NULL, "select count(*) from langindex") == wyTrue)
    {        
        while(psqlite->Step(NULL, wyFalse) && psqlite->GetLastCode() == SQLITE_ROW)
        {
            count = psqlite->GetInt(NULL, 0);
            break;
        }

        psqlite->Finalize(NULL);
    }

    if(dbname)
    {
        delete psqlite;
    }

    return count;
}

const wyChar*    
L10nText::GetLangCode()
{
    if(m_l10nt)
    {
        return m_l10nt->m_langcode.GetString();
    }

    return NULL;
}

wyInt32
L10nText::GetDBVersion(const wyChar* dbname)
{
    wySQLite*   psqlite = NULL;
    wyInt32     version = -1;

    if(dbname)
    {
        if((psqlite = new(std::nothrow) wySQLite()) == NULL || psqlite->Open(dbname, wyTrue) == wyFalse)
        {
            delete psqlite;
            return -1;
        }
    }
    else if(m_l10nt)
    {
        psqlite = &m_l10nt->m_sqlite;
    }

    if(psqlite && psqlite->Prepare(NULL, "select version from version") == wyTrue)
    {        
        while(psqlite->Step(NULL, wyFalse) && psqlite->GetLastCode() == SQLITE_ROW)
        {
            version = psqlite->GetInt(NULL, 0);
            break;
        }

        psqlite->Finalize(NULL);
    }

    if(dbname)
    {
        delete psqlite;
    }

    return version;
}

wyBool
L10nText::InitIndexTable()
{
    wyString query, error;
    wyUInt32 crcsum;

    if(m_ismemoryindex == wyFalse)
    {
        m_pindexsqlite = &m_sqlite;
        return wyTrue;
    }

    if((m_pindexsqlite = new(std::nothrow)wySQLite) == NULL || m_pindexsqlite->Open(":memory:") == wyFalse)
    {
        return wyFalse;
    }

    query.SetAs("create table en(id int unique not null, crc int not null, primary key(crc))");
    
    if(m_pindexsqlite->Execute(&query, &error) == wyFalse)
    {
        return wyFalse;
    }

    m_sqlite.Prepare(NULL, "select id, crc from en");

    while(m_sqlite.Step(NULL, wyFalse) && m_sqlite.GetLastCode() == SQLITE_ROW)
    {
        crcsum = (wyUInt32)m_sqlite.GetInt(NULL, "crc");
        query.Sprintf("insert into en(id, crc) values(%d, %u)", m_sqlite.GetInt(NULL, "id"), crcsum);
        m_pindexsqlite->Execute(&query, &error);
    }

    m_sqlite.Finalize(NULL);
    return wyTrue;
}

wyUInt32
L10nText::CalculateCRC(const wyChar* string)
{
    wyCrc crc;
    wyString temp;

    temp.SetAs(string);
    return crc.CrcFast((const unsigned char*)temp.GetString(), temp.GetLength());
}

wyInt32
L10nText::GetFontNameAndSize(const wyChar** fontname)
{
    *fontname = NULL;

    if(m_l10nt)
    {
        if(m_l10nt->m_fontname.GetLength())
        {
            *fontname = m_l10nt->m_fontname.GetString();
        }

        return m_l10nt->m_fontsize;
    }

    return 0;
}

#ifdef WIN32
HFONT
L10nText::GetFont()
{
    if(m_l10nt)
    {
        return m_l10nt->m_hfont;
    }

    return NULL;
}
#endif

