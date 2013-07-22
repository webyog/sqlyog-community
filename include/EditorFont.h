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



#ifndef _EDITORFONT_H_
#define _EDITORFONT_H_

#include "global.h"
#include "FrameWindowHelper.h"
#include "Scintilla.h"
#include "PreferenceBase.h"
#include "scilexer.h"

#define DEF_COMMENTCOLOR	8421376   
#define DEF_STRINGCOLOR		255		  
#define DEF_FUNCTIONCOLOR	14499311  
#define DEF_KEYWORDCOLOR	16735838  
#define DEF_NORMALCOLOR		131586
#define DEF_OPERATORCOLOR   RGB(128,0,0)
#define DEF_BKGNDEDITORCOLOR    16777215
#define DEF_BKGNDCANVASCOLOR    16777215
#define DEF_TEXTSELECTION       RGB(192,192,192)
#define DEF_MARGINNUMBER        RGB(240,240,240)
#define DEF_BRACEBADFG          RGB(0,0,0)
#define DEF_BRACEBADBG          RGB(255, 111, 111)
#define DEF_BRACELIGHTFG        RGB(255, 15, 15)
#define DEF_BRACELIGHTBG        RGB(255, 255, 0)

//Font Preference Default Values
#define		FONTNAME_DEFAULT			"Courier New"
#define		FONTSIZE_DEFAULT			9
#define		FONTSTYLE_DEFAULT			0
#define		FONTITALIC_DEFAULT			0

//Keyword and Function case
#define		KEYWORDCASE_DEFAULT			"UPPERCASE"
#define		FUNCTIONCASE_DEFAULT		"UPPERCASE"

#define     INSERTSPACES_DEFAULT        0

#define SCI_C_KEYWORDS5		20
#define SCI_C_KEYWORDS6		21
#define SCI_C_KEYWORDS7		16



class EditorFont
{
public:

	/// Sets a font style
    /**
    @param hwndedit:    IN Window Handler.
    @param fromini:     IN From ini file.
    @returns void.    
    */
	static void SetFont(HWND hwndedit, wyChar *tabname, wyBool fromini);

	/// Sets keywords
    /**
    @param tunnel:    IN pointer of type tunnel.
    @param mysql:     IN MySql pointer.
	@param hwndedit:  IN Window Handler.
    @returns void.    
    */
	static void SetLexerWords(HWND hwndedit, wyString &keys, wyString &funcs);

	/// Sets keywords style
    /**
    @param hwndedit:  IN Window Handler.
    @returns void.    
    */
	static void SetLexerWordsStyles(HWND hwndedit);

	/// Changes the word wrap mode for the edit control.
    /**
    @param hwndedit:  IN Window Handler.
    @returns void.    
    */
	static void SetWordWrap(HWND hwndedit);

	/// Sets Color
    /**
    @param tunnel		:	IN pointer of type tunnel.
    @param fromini		:   IN From ini file.
	@param isinfotab	:	IN def. parameter tells wheter it is infotab or not
    @returns void.    
    */
	static void SetColor(HWND hwndedit, wyBool fromini, wyBool isinfotab = wyFalse);

	/// Sets the width for the line numbers
    /**
    @param hwndedit:  IN Window Handler.
    @returns void.    
    */
	static void SetLineNumberWidth(HWND hwndedit);

	/// Changes the editor as a whole.
	/**
    @param tunnel:	 IN tunnel pointer.
	@param mysql:	 IN MySQL pointer.
	@param hwndedit: IN Window Handler.
	@param fromini:	 IN From ini file. 	
    @returns void.    
    */
	static void FormatEditor(HWND hwndedit, wyBool fromini, 
                             wyString &keys, wyString &funcs);

	/// Select colors from an ini file.
	/**
    @param hwndedit		:	 IN ini file. 	
	@param directory	:	 IN Path of ini file.
	@param isinfotab	:	IN def. parameter tells wheter it is infotab or not
    @returns void.  
	*/
	static void SetColorFromIni(HWND hwndedit, wyWChar directory[], wyBool isinfotab);
	
	/// Select default colors .
	/**
	@param hwndedit		:	 IN ini file.
	@param isinfotab	:	IN def. parameter tells wheter it is infotab or not
	@returns void.  
	*/
	static void SetColorDefault(HWND hwndedit, wyBool isinfotab);

	/// Select Keyword & Function case from an ini file.
	/**
    @param hwndedit:	 IN ini file.	
	@returns void.  
	*/
	static void SetCase(HWND hwndedit);
};
#endif