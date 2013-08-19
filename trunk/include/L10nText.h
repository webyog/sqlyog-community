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

Author: Vishal P.R

*********************************************/

#ifndef _L10NTEXT_H_
#define _L10NTEXT_H_

#define L10NT_ALLOCATION_FAILED 1
#define L10NT_SQLITE_ERROR      2
#define L10NT_LOOKUP_FAILED     3
#define L10NT_SEARCH_FAILED     4

#ifndef _CONSOLE
#ifdef _DEBUG
#define _(STRING)   GetL10nText(STRING, __FILE__, __LINE__)
#else
#define _(STRING)   GetL10nText(STRING)
#endif
#else 
#define _(STRING)   STRING
#endif

#include "wySqlite.h"
#include "wyString.h"

//Structure that provides the information about the languages available with the current dictinary file; To be used by the client. 
struct Language
{
    wyString    m_langcode;
    wyString    m_lang;
};

//L10n APIs

///Function initializes the library
/**
@param langcode         : IN language code
@param dictionary       : IN dictionary file
@param ismemoryindex    : IN wheather to create the index in the memory or not. With memory index, the library would be faster
@param islogerror       : IN parameter tells to log information whenever a string lookup fails, also activates SQLite logging
@param islogaccess      : IN parameter tells to log information whenever a string lookup is tried
@returns 0 on success else non zero
*/
wyInt32         InitL10n(const wyChar* langcode, const wyChar* dictionary, wyBool ismemoryindex, wyBool islogerror = wyFalse, wyBool islogaccess = wyFalse);

///Function closes the library
void            CloseL10n();

#ifdef _DEBUG
///Thread safe function to get the localized version of the string
/**
@param str      : IN the string to be localized
@returns the pointer to the localized string
*/
wyChar*         GetL10nText(const wyChar* str, wyChar* sourcefile = NULL, wyInt32 sourceline = 0);

///Overoaded version for wide charecter strings
/**
@param str      : IN the string to be localized
@returns the pointer to the localized string
*/
wyWChar*        GetL10nText(const wyWChar* str, wyChar* sourcefile = NULL, wyInt32 sourceline = 0);
#else
///Thread safe function to get the localized version of the string
/**
@param str      : IN the string to be localized
@returns the pointer to the localized string
*/
wyChar*         GetL10nText(const wyChar* str);

///Overoaded version for wide charecter strings
/**
@param str      : IN the string to be localized
@returns the pointer to the localized string
*/
wyWChar*         GetL10nText(const wyWChar* str);
#endif

///Function to get the last SQLite error code inside the library
wyInt32         GetLastL10nSQliteCode();

///Function to get the last SQLite error message inside the library
const wyChar*   GetLastL10nSQliteMsg();

//Function to get the total number of languages in the dictionary
/**
@param dictionary : IN dictionary file, leave it NULL to get the language count from the active dictionary
@returns count of language
*/
wyInt32         GetL10nLanguageCount(const wyChar* dictionary = NULL);

///Function to get the languages in the dictionary fie
/**
@param dictionary : IN dictinary file, leave it NULL to get the languages from the active dictionary
@returns languages in the dictionary in array of Language structures on success, else NULL
*/
Language*       GetL10nLanguages(const wyChar* dictionary = NULL);

///Function frees the Language structure passed in
/**
@param plang    : IN the Language* returned by GetL10nLanguages
*/
void            FreeL10nLanguages(Language* plang);

///Function gets the active language code
/**
@returns the language code if available, else NULL
*/
const wyChar*   GetL10nLangcode();

wyInt32         GetL10nFontNameAndSize(const wyChar** fontname);

wyInt32         GetL10nDBVersion(const wyChar* dbname = NULL);

#ifdef WIN32
HFONT           GetL10nFont();
#endif

//Structure that holds the identifier of the string and the string itself; Used internally by the library
class LangString
{
    public:

        wyInt32     m_id;
        wyString*    m_string;

        LangString(wyInt32 id, const wyChar* string);
        ~LangString();
};

//The class that wraps up the entire library operations.
class L10nText
{
    public:
        ///Static function to get the localized version of the charecter string passed in
        /**
        @param str              : IN charecter string
        @returns a constant char pointer to the localized string if any, otherwise it will return str itself
        */
        static const wyChar*    GetText(const wyChar* str);

        ///Overloaded version meant for wide charecter strings
        /**
        @param str              : IN wide character string
        @returns a constant char pointer to the localized string if any, otherwise it will return str itself
        */
        static const wyWChar*   GetText(const wyWChar* str);

        ///Static function to compare the the ids of two strings, Used to loacte the string in binary search
        /**
        @param elem1            : IN constant pointer to first element
        @param elem2            : IN constant pointer to second element
        @returns -1 if elem1 < elem2 ; 0 if elem1 = elem2; else 1
        */
        static wyInt32          CompareFunct(const void* elem1, const void* elem2);

        ///Static function to allocate the memory for the object
        /**
        @returns wyTrue on success else wyFalse
        */
        static wyBool           Allocate();

        ///Static function to release the memory for the object
        static void             Release();

        ///Static function to initialize the class object m_l10nt
        /**
        @param langcode         : IN language code
        @param dictionary       : IN dictionary file
        @param ismemoryindex    : IN wheather to create the index in the memory or not. With memory index, the library would be faster
        @param islogerror       : IN parameter tells to log information whenever a string lookup fails, also activates SQLite logging
		@param islogaccess      : IN parameter tells to log information whenever a string lookup is tried
        @returns 0 on success else non zero
        */
        static wyInt32          Init(const wyChar* langcode, const wyChar* dictonary, wyBool ismemoryindex,  wyBool islogerror, wyBool islogaccess);

        ///Static function to get the languages available with the dictionary
        /**
        @param dbname           : IN dictionary file name to ge the languages from, keep it NULL to get it from the active dictionary file
        @returns the language structure on success, else NULL
        */
        static Language*        GetLanguages(const wyChar* dbname = NULL);

        ///Static function to get the language count in the dictionary
        /**
        @param dbname           : IN dictionary file name to ge the languages from, keep it NULL to get it from the active dictionary file
        @returns the language count;
        */
        static wyInt32          GetLanguageCount(const wyChar* dbname = NULL);

        ///Function returns the last SQLite return code
        /**
        @returns SQLite error code
        */
        static wyInt32          GetLastSQliteCode();

        ///Function returns the last SQLite message
        /**
        @returns constent pointer to SQLite error message
        */
        static const wyChar*    GetLastSQliteMsg();

        ///Function returns the active language code
        /**
        @return constant pointer to the active lanugage code, else NULL
        */
        static const wyChar*    GetLangCode();

        static wyInt32          GetFontNameAndSize(const wyChar** fontname);

        static wyInt32          GetDBVersion(const wyChar* dbname);

#ifdef WIN32
        static HFONT            GetFont();
#endif

        //Public member to indicate the en language is the one in use
        wyBool                  m_isen;

        //Holds the file name from where the call originated
        static wyChar*          m_sourcefile;
        
        //Holds the line number from where the call originated
        static wyInt32          m_sourceline;

    private:
        ///Private constructor; Done this to prevent accidential creation of multiple instances of the class from outside
                            L10nText();

        //Priate destructor
                            ~L10nText();

        ///Private function to initialize the one and only instance of the class
        /**
        @param langcode         : IN language code
        @param dictionary       : IN dictionary file
        @param ismemoryindex    : IN wheather to create the index in the memory or not. With memory index, the library would be faster
        @param islogerror       : IN parameter tells to log information whenever a string lookup fails, also activates SQLite logging
		@param islogaccess      : IN parameter tells to log information whenever a string lookup is tried
        @returns 0 on success else non zero
        */
        wyInt32             Initialize(const wyChar* langcode, const wyChar* dictonary, wyBool ismemoryindex, wyBool islogerror, wyBool islogaccess);
        
        ///Function released the index created
        void                ReleaseStringIndex();

        ///Function gets the id of the string passed
        /**
        @param str          : IN string to lookup
        @returns string id
        */
        wyInt32             GetStringId(const wyChar* str);

        ///Function initialize the sqlite pointer to use, it can be a memory index or the dictionary file itself
        /**
        @returns wyTrue on success else wyFalse
        */
        wyBool              InitIndexTable();

        ///Function calculates the CRC  for the string
        /**
        @param string       : IN string for which the CRC to be calculated
        @returns CRC
        */
        wyUInt32            CalculateCRC(const wyChar* string);

        //member to hold language code
        wyString            m_langcode;

        //member to indicate the error logging
        wyBool              m_islogerror;

		//member to indicate access logging
        wyBool              m_islogaccess;

        //dictionary file name
        wyString            m_dictionary;

        //SQLite
        wySQLite            m_sqlite;

        //pointer to the index table
        wySQLite*           m_pindexsqlite;

        //Localized version of the strings in memory
        LangString**        m_index;

        //Total indexes
        wyInt32             m_indexcount;

#ifdef WIN32
        //critical section to make the library threadsafe
        CRITICAL_SECTION    m_cs;

        //font handle
        HFONT               m_hfont;
#endif

        //member tells whether memory index is used
        wyBool              m_ismemoryindex;

        wyString            m_fontname;

        wyInt32             m_fontsize;

        //SQLite return code
        static wyInt32      m_sqliteretcode;

        //SQLite error message
        static wyString     m_sqlitemsg;

        //The one and only instance of the class
        static L10nText*    m_l10nt;
};

#endif