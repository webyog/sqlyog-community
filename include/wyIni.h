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

#ifndef INI_MANAGE_H
#define INI_MANAGE_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Datatype.h"
#include "wyString.h"   
#include "wyFile.h"

#define IN_COMMENT		'#'
#define INI_ERROR	-1

#define FILE_LOCK_WAIT			 500
#define FILE_LOCK_WAIT_TRY_COUNT 6

typedef enum
{
	REPLACE = 1,
	NON_REPLACE = 0
}REPLACE_FLAG;

/************************************************************************/
/* for record of ini content                                            */
/************************************************************************/
/*! \struct tagRecord
    \brief holds data about a record value
    \param wyString m_comments		holds the comment info
    \param wyString m_key		    holds the key name in current section
    \param wyString m_value		    holds the value info
    \param tagRecord m_next		    holds the value info
*/
struct tagRecord
{
	wyString m_comments;
	wyString m_key;
	wyString m_value;
	struct tagRecord *m_next;
};

/************************************************************************/
/* for content of file                                                  */
/************************************************************************/
/*! \struct tagSection
    \brief holds data about a blob value in the insert update field
    \param tagRecord *m_datafirst	holds the comment info
    \param tagRecord *m_datalast	holds the key name in current section
    \param wyString m_comments		holds the comment info
    \param unsigned int m_datasize	holds the size of data in current section
    \param wyString m_name		    holds the name info
    \param tagSection *m_next		poits to next section info
*/
struct tagSection
{
	struct tagRecord *m_datafirst;
	struct tagRecord *m_datalast;
	unsigned int m_datasize;
	wyString m_comments;
	wyString m_name;
	struct tagSection *m_next;
};

/************************************************************************/
/* for content list                                                     */
/************************************************************************/
/*! \struct tagRecord
    \brief holds data about a record value
    \param unsigned int m_sectionsize		holds the size of a section
    \param wyString m_key		            holds the key name in current section
    \param tagSection *m_first		        points to first section info
    \param tagSection *m_last		        points to last section info
*/
typedef struct 
{
	unsigned int m_sectionsize;
	struct tagSection *m_first;
	struct tagSection *m_last;
}tagContent;

//static tagContent *m_content;

class wyIni
{
    public:
            /// Gets the string from ini file
            /** 
            @param sec		        : IN name of the section
            @param key				: IN key name to which value is to be fetched
            @param defval			: IN default value to be copied if key is not found
            @param returnedstring	: OUT returned the 
            @param size				: IN size structure
            @param path				: IN editable or not, default value is editable
            @returns wyInt32 number of character of the string found
            */
            static wyInt32 IniGetString(const wyChar *sec, const wyChar *key, const wyChar *defval, wyString *returnedstring, const wyChar *path);
            
            /// gets the integer value from ini file
            /** 
            @param sec		        : IN name of the section
            @param key				: IN key name to which value is to be fetched
            @param defval			: IN editable or not, default value is editable
            @param path				: IN editable or not, default value is editable
            @returns wyInt32 number fetched from ini file
            */
            static wyInt32 IniGetInt(const wyChar *sec, const wyChar *key, wyInt32 defval, const wyChar *path);

            /// Creates the blob viewer windows
            /** 
            @param sec		        : IN name of the section
            @param key				: IN key name to which value is to be fetched
            @param defval			: IN editable or not, default value is editable
            @param returnedstring	: IN parent window control
            @param size				: IN PINSERTUPDATEBLOB structure
            @param path				: IN editable or not, default value is editable
            @returns wyInt32 number of character of the string found
            */
            static wyBool  IniWriteString(const wyChar *sec, const wyChar *key,		// will auto replace
				            const wyChar *value, const wyChar *path);	

            /// Transfers Full section from one file to another
            /**
            Note: This function do not delete/update new section in the new file it simply appends the section in the file. 
            If deletion or updation is to be done, it should be done explicitly
            @param fromSec          : IN name of the section from which to transfer data
            @param toSec            : IN name of the target section
            @param fromPath         : IN full path of the file from which section details are to be transferred
            @param toPath           : IN full path of the file to which sections details to be transeffered
            */
            static wyBool   IniTransferFullSection(const wyChar* fromSec, const wyChar* toSec, 
								const wyChar* fromPath, const wyChar* toPath);

            static wyBool   IniIsConnectionExists(const wyWChar *path);

            /// Creates the blob viewer windows
            /** 
            @param sec		        : IN name of the section
            @param key				: IN key name to which value is to be fetched
            @param defval			: IN editable or not, default value is editable
            @param returnedstring	: IN parent window control
            @param size				: IN PINSERTUPDATEBLOB structure
            @param path				: IN editable or not, default value is editable
            @returns wyInt32 number of character of the string found
            */
            static wyBool  IniWriteInt(const wyChar *sec, const wyChar *key,		// will auto replace
				            wyInt32 value, const wyChar *path);	

            static wyInt32 IniGetSection(wyString *allsecnames, wyString *path);

			static wyInt32 IniDeleteSection(const wyChar *sec, const wyChar *path);

			static wyInt32 IniDeleteKey(const wyChar *sec, wyChar *key, const wyChar *path);

			void IniGetSectionDetailsInit(wyString *sec, wyString *path);
			void IniGetSectionDetailsFinalize();
			wyInt32 IniGetString2(const wyChar *sec, const wyChar *key, const wyChar *defval, wyString *returnedstring, const wyChar *path);
			wyInt32 IniGetInt2(const wyChar *sec, const wyChar *key, wyInt32 defval, const wyChar *path);
    private:
            
		// variables 
            
            wyString m_filename;

            wyChar *m_errormsg;

            REPLACE_FLAG m_flag;

            wyBool       m_sectionnameonly;
			tagContent	*m_content;

			wyFile		m_lockfile;
			
            /************************************************************************/
            /* main ini manage function                                             */
            /************************************************************************/

            wyBool  Initialize(const wyChar* filename, const wyChar* sec = NULL, const wyChar* key = NULL);

            void    SetSectionNameOnly(wyBool issectiononly);

            void    Finialize(wyBool iswrite);

            wyBool  LoadFile(const wyChar *filename, const wyChar* sec, const wyChar* key);

			// save to load filebool
            wyBool SaveFile();	 

            wyBool SaveFileAs(const wyChar *filename);

            wyInt32 GetValue(const wyChar *sec,const wyChar *key, const wyChar *defval, wyString *returnedstring);

			// return data and comment
            wyChar *GetValue(const wyChar *sec,const wyChar *key,		
				            wyChar *comment);

			// select replace or not replace
            wyBool SetValue (const wyChar *sec,const wyChar *key,		
				            const wyChar *value,const wyChar *comment,REPLACE_FLAG flag);

            wyInt32  RemoveSel (const wyChar *sec,wyChar *key);

			// remove all record in section
            wyInt32  RemoveAll (const wyChar * sec);				

            // add section
            void AddSection(const wyChar *sec,const wyChar *comment);	

			// remove section (remove all record in section if not empty)
            wyInt32  RemoveSection(const wyChar *sec);				

			// clear all content
            void Clear();								

            // size of section
            wyInt32  ContentSize();

            wyInt32  SectionSize(wyChar *sec);

            wyChar *GetLastErr();

            // initaial data/save
            void InitializeContent();

            wyBool SaveFile(const wyChar *filename);

			/*Check whether the inii file is locked or not.
			This is for getting access to ini for single process/thread at a time*/
			wyBool IsIniFileLocked(wyString *filename);

            // append data to section
            void Append(const wyChar *sec,const wyChar *key,		
			            const wyChar *value);											

            wyInt32  Remove(const wyChar *sec,const wyChar *key);

            wyInt32  RemoveAll(struct tagSection *sec);
            
			// search section
            struct tagSection *GetSection(const wyChar *sec);		

			// get record
            struct tagRecord *GetRecord(struct tagSection *sec,const wyChar *key);	

            // trim
            void Trim(wyChar *buffer);

			// strip utf8 BOM
            void StripUTF8Bom(wyChar *buffer);
             
            wyInt32 GetAllSectionDetails(wyString *secnames);

            //Freeing the list memory
            void FreeListData();
};
#endif