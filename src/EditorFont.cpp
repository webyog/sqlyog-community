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


#include "EditorFont.h"
#include "CommonHelper.h"
#include "wyTheme.h"

#define BOLD_DEFINED  700
#define DEF_FONT_SIZE 9


//Function to set the font.
void 
EditorFont::SetFont(HWND hwndedit, wyChar tabname[], wyBool fromini)
{
	//First delete the original font if it has been created already.
	wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH + 1] = {0};
	wyString	fontname, dirstr;
	wyInt32		fontitalic = 0;
	wyInt32		fontstyle = 0;
	BYTE		fontcharset = 0;
	wyInt32		fontsize = 0;

	//Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	
	dirstr.SetAs(directory);

	//Get the font details from the ini file
	if (fromini) 
	{
		wyIni::IniGetString((LPCSTR)tabname, "FontName", FONTNAME_DEFAULT, &fontname, dirstr.GetString());
		fontsize = wyIni::IniGetInt((LPCSTR)tabname, "FontSize", FONTSIZE_DEFAULT, dirstr.GetString());
		fontitalic = wyIni::IniGetInt((LPCSTR)tabname, "FontItalic", FONTITALIC_DEFAULT, dirstr.GetString());
		fontstyle = wyIni::IniGetInt((LPCSTR)tabname, "FontStyle", FONTSTYLE_DEFAULT, dirstr.GetString());
		fontcharset = wyIni::IniGetInt((LPCSTR)tabname, "FontCharSet", DEFAULT_CHARSET , dirstr.GetString());
		
		if(fontstyle != BOLD_DEFINED)
			fontstyle = 0;

		fontcharset = wyIni::IniGetInt((LPCSTR)tabname, "FontCharSet", DEFAULT_CHARSET, dirstr.GetString());
	}
	else 
	{
		fontname.SetAs("Courier New");
		fontsize = DEF_FONT_SIZE;
	}
	
	SendMessage(hwndedit, SCI_STYLESETFONT, STYLE_DEFAULT, (LPARAM)fontname.GetString());
	SendMessage(hwndedit, SCI_STYLESETSIZE, STYLE_DEFAULT, fontsize);
	SendMessage(hwndedit, SCI_STYLESETITALIC, STYLE_DEFAULT, fontitalic);
	SendMessage(hwndedit, SCI_STYLESETBOLD, STYLE_DEFAULT, fontstyle);
	SendMessage(hwndedit, SCI_STYLESETCHARACTERSET, STYLE_DEFAULT, fontcharset);
    SendMessage(hwndedit, SCI_SETTABWIDTH, GetTabSize(), 0 );

    if(IsInsertSpacesForTab() == wyTrue)
        SendMessage(hwndedit, SCI_SETUSETABS, FALSE, 0);
    else
        SendMessage(hwndedit, SCI_SETUSETABS, TRUE, 0);

	return;
}

void
EditorFont::SetLexerWords(HWND hwndedit, wyString &keys, wyString &funcs)
{
	SendMessage(hwndedit, SCI_SETKEYWORDS, (WPARAM)0, (LPARAM)keys.GetString());
	SendMessage(hwndedit, SCI_SETKEYWORDS, (WPARAM)3, (LPARAM)funcs.GetString());
}

void
EditorFont::SetLexerWordsStyles(HWND hwndedit)
{
	SendMessage(hwndedit, SCI_STYLESETFORE, (WPARAM)SCE_MYSQL_DEFAULT, (LPARAM)RGB(0,0,0));
    SendMessage(hwndedit, SCI_STYLESETBOLD, (WPARAM)SCE_MYSQL_DEFAULT, (LPARAM)FALSE);
	SendMessage(hwndedit, SCI_STYLESETFORE, (WPARAM)SCE_C_COMMENTDOCKEYWORD, (LPARAM)RGB(0,0,0));
	SendMessage(hwndedit, SCI_STYLESETBOLD, (WPARAM)SCE_C_COMMENTDOCKEYWORD, (LPARAM)FALSE);

	//SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_NUMBER, RGB(128,0,0));
	SendMessage(hwndedit, SCI_STYLESETFORE, (WPARAM)SCE_MYSQL_OPERATOR, (LPARAM)RGB(128,0,0));

	SendMessage(hwndedit, SCI_STYLESETFORE, (WPARAM)SCE_MYSQL_SQSTRING, (LPARAM)RGB(255,0,0));
	SendMessage(hwndedit, SCI_STYLESETBOLD, (WPARAM)SCE_MYSQL_SQSTRING, (LPARAM)TRUE);
    SendMessage(hwndedit, SCI_STYLESETFORE, (WPARAM)SCE_MYSQL_DQSTRING, (LPARAM)RGB(255,0,0));
	SendMessage(hwndedit, SCI_STYLESETBOLD, (WPARAM)SCE_MYSQL_DQSTRING, (LPARAM)TRUE);

	SendMessage(hwndedit, SCI_STYLESETFORE, (WPARAM)SCE_MYSQL_COMMENTLINE, (LPARAM)RGB(0,128,128));
	SendMessage(hwndedit, SCI_STYLESETBOLD, (WPARAM)SCE_MYSQL_COMMENTLINE, (LPARAM)TRUE);
    
    SendMessage(hwndedit, SCI_STYLESETFORE, (WPARAM)SCE_MYSQL_HIDDENCOMMAND, (LPARAM)RGB(0,128,128));
    SendMessage(hwndedit, SCI_STYLESETBOLD, (WPARAM)SCE_MYSQL_HIDDENCOMMAND, (LPARAM)TRUE);
    
	SendMessage(hwndedit, SCI_STYLESETFORE, (WPARAM)SCE_MYSQL_COMMENT, (LPARAM)RGB(0,128,128));
	SendMessage(hwndedit, SCI_STYLESETBOLD, (WPARAM)SCE_MYSQL_COMMENT, (LPARAM)TRUE);

    SendMessage(hwndedit, SCI_STYLESETFORE, (WPARAM)SCE_MYSQL_IDENTIFIER, (LPARAM)RGB(0,0,0));
	SendMessage(hwndedit, SCI_STYLESETBOLD, (WPARAM)SCE_MYSQL_IDENTIFIER, (LPARAM)FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, (WPARAM)SCE_MYSQL_QUOTEDIDENTIFIER, (LPARAM)RGB(0,0,0));
	SendMessage(hwndedit, SCI_STYLESETBOLD, (WPARAM)SCE_MYSQL_QUOTEDIDENTIFIER, (LPARAM)FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, (WPARAM)SCE_MYSQL_VARIABLE, (LPARAM)RGB(0,0,0));
	SendMessage(hwndedit, SCI_STYLESETBOLD, (WPARAM)SCE_MYSQL_VARIABLE, (LPARAM)FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, (WPARAM)SCE_MYSQL_SYSTEMVARIABLE, (LPARAM)RGB(0,0,0));
	SendMessage(hwndedit, SCI_STYLESETBOLD, (WPARAM)SCE_MYSQL_SYSTEMVARIABLE, (LPARAM)FALSE);
}


//Function changes the word wrap mode for the edit control.
void
EditorFont::SetWordWrap(HWND hwndedit)
{
	SetEditWordWrap(hwndedit, wyTrue, wyTrue);
}

void
EditorFont::SetLineNumberWidth(HWND hwndedit)
{
	wyInt32 linenumwidth = 1, pixelwidth, linecount;

	//The margin size will be expanded if the current buffer's maximum
	//Line number would overflow the margin.
	linecount = SendMessage(hwndedit, SCI_GETLINECOUNT, 0, 0 );

	while (linecount >= 10) 
	{
		linecount /= 10;
		++ linenumwidth;
	}

	//The 4 here allows for spacing: 1 pixel on left and 3 on right.
	pixelwidth = 4 + linenumwidth * (SendMessage(hwndedit, SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)"9"));

	SendMessage(hwndedit, SCI_SETMARGINWIDTHN, 0, pixelwidth);
}

void 
EditorFont::FormatEditor(HWND hwndedit, wyBool fromini, 
                         wyString &keys, wyString &funcs)
{
	//SetFont(hwndedit, "HistoryFont", fromini);

	SetLexerWords(hwndedit, keys, funcs);

	SetLexerWordsStyles(hwndedit);

	SetColor(hwndedit, wyTrue);
	
	SetCase(hwndedit);

	SetWordWrap(hwndedit);
}

void 
EditorFont::SetColor(HWND hwndedit, wyBool fromini, wyBool isinfotab)
{
	wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH+1] = {0};

	//Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);

	if (fromini) 
		SetColorFromIni(hwndedit, directory, isinfotab);
	else 
		SetColorDefault(hwndedit, isinfotab);
	
	ApplyEditorThemeColors(hwndedit, isinfotab);
	ApplyDialogEditorDefaultColors(hwndedit, isinfotab);
}

void
EditorFont::SetThemeColor(HWND hwndedit, wyBool fromini, wyBool isinfotab)
{
	wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH + 1] = { 0 };
	EDITORCOLORINFO editorcolors;

	//Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);

	if (fromini)
		GetColorFromIni(hwndedit, directory, &editorcolors, isinfotab);
	else
		GetColorDefault(hwndedit, &editorcolors, isinfotab);

	ApplyEditorThemeColors(hwndedit, &editorcolors, isinfotab);
	ApplyDialogEditorDefaultColors(hwndedit, isinfotab);

}

void 
EditorFont::SetColorFromIni(HWND hwndedit, wyWChar directory[], wyBool isinfotab)
{

	COLORREF    color,bgcolor=RGB(255,255,255),forecolor,backcolor;
	wyString    dirstr(directory);

    SetThemeAwareMarginColors(hwndedit, dirstr);
    
    //Change selection color
    backcolor   =   wyIni::IniGetInt(GENERALPREFA, "SelectionBgColor",   DEF_TEXTSELECTION, dirstr.GetString());
    SendMessage(hwndedit,SCI_SETSELBACK,1,backcolor);

    bgcolor=wyIni::IniGetInt(GENERALPREFA, "EditorBgColor", DEF_BKGNDEDITORCOLOR, dirstr.GetString()); 
    SendMessage( hwndedit, SCI_STYLESETBACK, STYLE_DEFAULT, (LPARAM)bgcolor);
        
    SendMessage( hwndedit, SCI_SETCARETFORE,bgcolor ^ 0xFFFFFF,0); //Change Caret color in editor window

        
    color = wyIni::IniGetInt(GENERALPREFA, "NormalColor", DEF_NORMALCOLOR, dirstr.GetString()); 

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_QUOTEDIDENTIFIER, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_QUOTEDIDENTIFIER, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_QUOTEDIDENTIFIER, FALSE);
    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_IDENTIFIER, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_IDENTIFIER, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_IDENTIFIER, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_VARIABLE, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_VARIABLE, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_VARIABLE, FALSE);
 
    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_SYSTEMVARIABLE, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_SYSTEMVARIABLE, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_SYSTEMVARIABLE, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_KNOWNSYSTEMVARIABLE, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_KNOWNSYSTEMVARIABLE, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_KNOWNSYSTEMVARIABLE, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DATABASEOBJECT, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DATABASEOBJECT, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_DATABASEOBJECT, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_PROCEDUREKEYWORD, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_PROCEDUREKEYWORD, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_PROCEDUREKEYWORD, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DEFAULT, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DEFAULT, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_DEFAULT, FALSE);


	color = wyIni::IniGetInt(GENERALPREFA, "CommentColor", DEF_COMMENTCOLOR, dirstr.GetString()); 	
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENTLINE, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_COMMENTLINE, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENT, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_COMMENT, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_COMMENT, TRUE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_COMMENTLINE, TRUE);

	color = wyIni::IniGetInt(GENERALPREFA, "HiddenCmdColor", DEF_COMMENTCOLOR, dirstr.GetString()); 	
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_HIDDENCOMMAND, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_HIDDENCOMMAND, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_HIDDENCOMMAND, TRUE);
    
    
    //In info tab, no highlighting for string literals
	if(isinfotab == wyTrue)
		color = wyIni::IniGetInt(GENERALPREFA, "NormalColor", DEF_NORMALCOLOR, dirstr.GetString()); 
	else
		color = wyIni::IniGetInt(GENERALPREFA, "StringColor", DEF_STRINGCOLOR, dirstr.GetString()); 	
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_STRING, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_STRING, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_STRING, TRUE);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_SQSTRING, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_SQSTRING, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_SQSTRING, TRUE);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DQSTRING, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DQSTRING, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_DQSTRING, TRUE);

	
	color = wyIni::IniGetInt(GENERALPREFA, "KeywordColor", DEF_KEYWORDCOLOR, dirstr.GetString());
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_KEYWORD, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_KEYWORD, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_KEYWORD, TRUE);
    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_MAJORKEYWORD, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_MAJORKEYWORD, bgcolor);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_MAJORKEYWORD, TRUE);

	color = wyIni::IniGetInt( GENERALPREFA, "FunctionColor", DEF_FUNCTIONCOLOR, dirstr.GetString()); 	
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_FUNCTION, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_FUNCTION, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_FUNCTION, TRUE);

    color = wyIni::IniGetInt( GENERALPREFA, "OperatorColor", DEF_OPERATORCOLOR, dirstr.GetString()); 	
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_OPERATOR, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_OPERATOR, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_OPERATOR, TRUE);

    color = wyIni::IniGetInt( GENERALPREFA, "NumberColor", DEF_NORMALCOLOR, dirstr.GetString()); 	
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_NUMBER, color);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_NUMBER, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_NUMBER, FALSE);
}



void 
EditorFont::SetColorDefault(HWND hwndedit, wyBool isinfotab)
{
	
	//SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_WORD, RGB(0,0,0));
	//SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_WORD, FALSE);

    //SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_TWIG,RGB(0,0,0));
	//SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_TWIG,FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DEFAULT, RGB(0,0,0));
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_DEFAULT, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_QUOTEDIDENTIFIER, RGB(0,0,0));
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_QUOTEDIDENTIFIER, FALSE);
    
    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_IDENTIFIER, RGB(0,0,0));
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_IDENTIFIER, FALSE);


	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_KEYWORD, RGB(0,0,255));
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_KEYWORD, TRUE);

	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_FUNCTION, RGB(255,0,255));
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_FUNCTION, TRUE);

	//In info tab, no highlighting for string literals
	if(isinfotab == wyTrue)
		SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_STRING, RGB(0,0,0));	
	else
		SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_STRING, RGB(255,0,0));
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_STRING, TRUE);

	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENT, RGB(0,128,128));
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENTLINE, RGB(0,128,128));
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_COMMENT, TRUE);
    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_HIDDENCOMMAND, RGB(0,128,128));
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_HIDDENCOMMAND, TRUE);

}

void 
EditorFont::SetCase(HWND hwndedit)
{
	wyWChar		*lpfileport = 0;
	wyWChar		directory[MAX_PATH+1] = {0};
	wyString	dirstr;
	wyString	keywordcase, functioncase;

	//Get the complete path.
	SearchFilePath(L"sqlyog", L".ini", MAX_PATH, directory, &lpfileport);
	dirstr.SetAs(directory);
	
	//Keyword case
	wyIni::IniGetString(GENERALPREFA, "KeywordCase", KEYWORDCASE_DEFAULT, &keywordcase, dirstr.GetString());
	wyIni::IniGetString(GENERALPREFA,  "FunctionCase", FUNCTIONCASE_DEFAULT, &functioncase, dirstr.GetString());

	if(keywordcase.CompareI("lowercase") == 0)
        SendMessage(hwndedit, SCI_STYLESETCASE, SCE_MYSQL_MAJORKEYWORD, SC_CASE_LOWER);
	else if(keywordcase.CompareI("Unchanged") == 0)
        SendMessage(hwndedit, SCI_STYLESETCASE, SCE_MYSQL_MAJORKEYWORD, SC_CASE_MIXED);
	else
        SendMessage(hwndedit, SCI_STYLESETCASE, SCE_MYSQL_MAJORKEYWORD, SC_CASE_UPPER);

	if(functioncase.CompareI("lowercase") == 0)
        SendMessage(hwndedit, SCI_STYLESETCASE, SCE_MYSQL_FUNCTION, SC_CASE_LOWER);
	else if(functioncase.CompareI("Unchanged") == 0)
        SendMessage(hwndedit, SCI_STYLESETCASE, SCE_MYSQL_FUNCTION, SC_CASE_MIXED);
	else
        SendMessage(hwndedit, SCI_STYLESETCASE, SCE_MYSQL_FUNCTION, SC_CASE_UPPER);
}

// Centralized theme-aware function for setting margin colors
void
EditorFont::SetThemeAwareMarginColors(HWND hwndedit, wyString &dirstr)
{
    EDITORINFO editorInfo = wyTheme::IdentifyEditorType(hwndedit);
    if (editorInfo.isTabPreviewEditor)
    {
        return;
    }
    
    COLORREF forecolor, backcolor;
    
    EDITORCOLORINFO editorcolors;
    if (wyTheme::GetEditorColors(&editorcolors))
    {
        if (editorcolors.m_mask & EDITORCF_LINENUMBER)
            forecolor = editorcolors.m_linenumber;
            
        if (editorcolors.m_mask & EDITORCF_MARGIN)
            backcolor = editorcolors.m_margin;
	}
	else {
		forecolor = wyIni::IniGetInt(GENERALPREFA, "NumberMarginforegroundColor", DEF_MARGINNUMBERFG, dirstr.GetString());
		backcolor = wyIni::IniGetInt(GENERALPREFA, "NumberMarginbackgroundColor", DEF_MARGINNUMBER, dirstr.GetString());
	}
    
    SendMessage(hwndedit, SCI_STYLESETFORE, STYLE_LINENUMBER, forecolor);
    SendMessage(hwndedit, SCI_STYLESETBACK, STYLE_LINENUMBER, backcolor);
    
    InvalidateRect(hwndedit, NULL, TRUE);
    UpdateWindow(hwndedit);
}
void
EditorFont::ApplyEditorThemeColors(HWND hwndedit, wyBool isinfotab)
{
	EDITORCOLORINFO editorcolors;
	if (!wyTheme::GetEditorColors(&editorcolors) ||
		!wyTheme::IsDarkThemeActive() ||
		!IsMainWindowEditor(hwndedit))
		return;
	// Apply basic editor colors
	if (editorcolors.m_mask & EDITORCF_BACKGROUND)
	{
		SendMessage(hwndedit, SCI_STYLESETBACK, STYLE_DEFAULT, editorcolors.m_background);
	}
	if (editorcolors.m_mask & EDITORCF_FOREGROUND)
	{
		SendMessage(hwndedit, SCI_STYLESETFORE, STYLE_DEFAULT, editorcolors.m_foreground);
	}
	if (editorcolors.m_mask & EDITORCF_SELECTION)
	{
		SendMessage(hwndedit, SCI_SETSELBACK, 1, editorcolors.m_selection);
	}
	if (editorcolors.m_mask & EDITORCF_CARETLINE)
	{
		SendMessage(hwndedit, SCI_SETCARETLINEBACK, editorcolors.m_caretline, 0);
	}
	if (editorcolors.m_mask & EDITORCF_CARET)
	{
		SendMessage(hwndedit, SCI_SETCARETFORE, editorcolors.m_caret, 0);
	}
	COLORREF normalcolor = (editorcolors.m_mask & EDITORCF_NORMAL) ? editorcolors.m_normal : editorcolors.m_foreground;
	COLORREF bgcolor = (editorcolors.m_mask & EDITORCF_BACKGROUND) ? editorcolors.m_background : RGB(30, 30, 30);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_QUOTEDIDENTIFIER, normalcolor);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_QUOTEDIDENTIFIER, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_IDENTIFIER, normalcolor);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_IDENTIFIER, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_VARIABLE, normalcolor);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_VARIABLE, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_SYSTEMVARIABLE, normalcolor);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_SYSTEMVARIABLE, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_KNOWNSYSTEMVARIABLE, normalcolor);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_KNOWNSYSTEMVARIABLE, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DATABASEOBJECT, normalcolor);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DATABASEOBJECT, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_PROCEDUREKEYWORD, normalcolor);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_PROCEDUREKEYWORD, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DEFAULT, normalcolor);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DEFAULT, bgcolor);
	// Apply comment colors
	if (editorcolors.m_mask & EDITORCF_COMMENT)
	{
		SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENTLINE, editorcolors.m_comment);
		SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_COMMENTLINE, bgcolor);
		SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENT, editorcolors.m_comment);
		SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_COMMENT, bgcolor);
		SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_COMMENT, TRUE);
		SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_COMMENTLINE, TRUE);
	}
	// Apply hidden command colors
	if (editorcolors.m_mask & EDITORCF_HIDDENCMD)
	{
		SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_HIDDENCOMMAND, editorcolors.m_hiddencmd);
		SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_HIDDENCOMMAND, bgcolor);
		SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_HIDDENCOMMAND, TRUE);
	}
	COLORREF stringcolor = normalcolor;
	if (!isinfotab && (editorcolors.m_mask & EDITORCF_STRING))
	{
		stringcolor = editorcolors.m_string;
	}
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_STRING, stringcolor);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_STRING, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_STRING, TRUE);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_SQSTRING, stringcolor);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_SQSTRING, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_SQSTRING, TRUE);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DQSTRING, stringcolor);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DQSTRING, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_DQSTRING, TRUE);
	// Apply keyword colors
	if (editorcolors.m_mask & EDITORCF_KEYWORD)
	{
		SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_KEYWORD, editorcolors.m_keyword);
		SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_KEYWORD, bgcolor);
		SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_KEYWORD, TRUE);
		SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_MAJORKEYWORD, editorcolors.m_keyword);
		SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_MAJORKEYWORD, bgcolor);
		SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_MAJORKEYWORD, TRUE);
	}
	// Apply function colors
	if (editorcolors.m_mask & EDITORCF_FUNCTION)
	{
		SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_FUNCTION, editorcolors.m_function);
		SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_FUNCTION, bgcolor);
		SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_FUNCTION, TRUE);
	}
	// Apply operator colors
	if (editorcolors.m_mask & EDITORCF_OPERATOR)
	{
		SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_OPERATOR, editorcolors.m_operator);
		SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_OPERATOR, bgcolor);
		SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_OPERATOR, TRUE);
	}
	// Apply number colors
	if (editorcolors.m_mask & EDITORCF_NUMBER)
	{
		SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_NUMBER, editorcolors.m_number);
		SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_NUMBER, bgcolor);
		SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_NUMBER, FALSE);
	}
}

void
EditorFont::ApplyEditorThemeColors(HWND hwndedit, EDITORCOLORINFO *pDefColors, wyBool isinfotab)
{
	EDITORCOLORINFO themeeditorcolors;

	if (wyTheme::IsDarkThemeActive() &&
		IsMainWindowEditor(hwndedit))

		if (wyTheme::GetEditorColors(&themeeditorcolors)) {
			if (themeeditorcolors.m_mask & EDITORCF_BACKGROUND)
			{
				pDefColors->m_background = themeeditorcolors.m_background;
			}
			if (themeeditorcolors.m_mask & EDITORCF_FOREGROUND)
			{
				pDefColors->m_foreground = themeeditorcolors.m_foreground;
			}
			if (themeeditorcolors.m_mask & EDITORCF_SELECTION)
			{
				pDefColors->m_selection = themeeditorcolors.m_selection;
			}
			if (themeeditorcolors.m_mask & EDITORCF_CARETLINE)
			{
				pDefColors->m_caretline = themeeditorcolors.m_caretline;
			}
			if (themeeditorcolors.m_mask & EDITORCF_CARET)
			{
				pDefColors->m_caret = themeeditorcolors.m_caret;
			}
			// Apply comment colors
			if (themeeditorcolors.m_mask & EDITORCF_COMMENT)
			{
				pDefColors->m_comment = themeeditorcolors.m_comment;
			}
			// Apply comment colors
			if (themeeditorcolors.m_mask & EDITORCF_HIDDENCMD)
			{
				pDefColors->m_hiddencmd = themeeditorcolors.m_hiddencmd;
			}
			pDefColors->m_normal = (themeeditorcolors.m_mask & EDITORCF_NORMAL) ? themeeditorcolors.m_normal : themeeditorcolors.m_foreground;
			pDefColors->m_background = (themeeditorcolors.m_mask & EDITORCF_BACKGROUND) ? themeeditorcolors.m_background : RGB(30, 30, 30);

			COLORREF stringcolor = pDefColors->m_normal;
			if (!isinfotab && (themeeditorcolors.m_mask & EDITORCF_STRING))
			{
				pDefColors->m_string = themeeditorcolors.m_string;
			}
			if (themeeditorcolors.m_mask & EDITORCF_KEYWORD)
			{
				pDefColors->m_keyword = themeeditorcolors.m_keyword;
			}
			if (themeeditorcolors.m_mask & EDITORCF_FUNCTION)
			{
				pDefColors->m_function = themeeditorcolors.m_function;
			}
			if (themeeditorcolors.m_mask & EDITORCF_OPERATOR)
			{
				pDefColors->m_operator = themeeditorcolors.m_operator;
			}

			if (themeeditorcolors.m_mask & EDITORCF_NUMBER)
			{
				pDefColors->m_number = themeeditorcolors.m_number;
			}

		}


	// Apply basic editor colors
	SendMessage(hwndedit, SCI_STYLESETBACK, STYLE_DEFAULT, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETFORE, STYLE_DEFAULT, pDefColors->m_foreground);
	SendMessage(hwndedit, SCI_SETSELBACK, 1, pDefColors->m_selection);
	SendMessage(hwndedit, SCI_SETCARETLINEBACK, pDefColors->m_caretline, 0);
	SendMessage(hwndedit, SCI_SETCARETFORE, pDefColors->m_caret, 0);
	
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_QUOTEDIDENTIFIER, pDefColors->m_normal);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_QUOTEDIDENTIFIER, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_IDENTIFIER, pDefColors->m_normal);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_IDENTIFIER, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_VARIABLE, pDefColors->m_normal);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_VARIABLE, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_SYSTEMVARIABLE, pDefColors->m_normal);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_SYSTEMVARIABLE, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_KNOWNSYSTEMVARIABLE, pDefColors->m_normal);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_KNOWNSYSTEMVARIABLE, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DATABASEOBJECT, pDefColors->m_normal);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DATABASEOBJECT, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_PROCEDUREKEYWORD, pDefColors->m_normal);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_PROCEDUREKEYWORD, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DEFAULT, pDefColors->m_normal);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DEFAULT, pDefColors->m_background);

	// Apply comment colors
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENTLINE, pDefColors->m_comment);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_COMMENTLINE, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENT, pDefColors->m_comment);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_COMMENT, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_COMMENT, TRUE);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_COMMENTLINE, TRUE);
	
	// Apply hidden command colors
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_HIDDENCOMMAND, pDefColors->m_hiddencmd);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_HIDDENCOMMAND, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_HIDDENCOMMAND, TRUE);

	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_STRING, pDefColors->m_string);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_STRING, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_STRING, TRUE);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_SQSTRING, pDefColors->m_string);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_SQSTRING, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_SQSTRING, TRUE);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DQSTRING, pDefColors->m_string);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DQSTRING, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_DQSTRING, TRUE);

	// Apply keyword colors
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_KEYWORD, pDefColors->m_keyword);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_KEYWORD, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_KEYWORD, TRUE);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_MAJORKEYWORD, pDefColors->m_keyword);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_MAJORKEYWORD, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_MAJORKEYWORD, TRUE);

	// Apply function colors
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_FUNCTION, pDefColors->m_function);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_FUNCTION, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_FUNCTION, TRUE);

	// Apply operator colors
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_OPERATOR, pDefColors->m_operator);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_OPERATOR, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_OPERATOR, TRUE);

	// Apply number colors
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_NUMBER, pDefColors->m_number);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_NUMBER, pDefColors->m_background);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_NUMBER, FALSE);
}

void
EditorFont::ApplyDialogEditorDefaultColors(HWND hwndedit, wyBool isinfotab)
{
    // Only apply to dialog editors in dark theme
    if (!wyTheme::IsDarkThemeActive() || IsMainWindowEditor(hwndedit))
        return;

    SendMessage(hwndedit, SCI_STYLECLEARALL, 0, 0);

    SendMessage(hwndedit, SCI_SETLEXERLANGUAGE, 0, (LPARAM)"MySQL");

    SendMessage(hwndedit, SCI_STYLESETBACK, STYLE_DEFAULT, COLOR_WHITE); 
    SendMessage(hwndedit, SCI_STYLESETFORE, STYLE_DEFAULT, RGB_DIFFBLACK); 
    SendMessage(hwndedit, SCI_SETSELBACK, 1, DEF_TEXTSELECTION); 
    SendMessage(hwndedit, SCI_SETCARETFORE, COLOR_WHITE ^ 0xFFFFFF, 0); 

    SendMessage(hwndedit, SCI_STYLESETFORE, STYLE_LINENUMBER, DEF_MARGINNUMBERFG);
    SendMessage(hwndedit, SCI_STYLESETBACK, STYLE_LINENUMBER, DEF_MARGINNUMBER);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DEFAULT, RGB_DIFFBLACK);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DEFAULT, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_DEFAULT, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_C_COMMENTDOCKEYWORD, RGB_DIFFBLACK);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_C_COMMENTDOCKEYWORD, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_C_COMMENTDOCKEYWORD, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_QUOTEDIDENTIFIER, RGB_DIFFBLACK);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_QUOTEDIDENTIFIER, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_QUOTEDIDENTIFIER, FALSE);
    
    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_IDENTIFIER, RGB_DIFFBLACK);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_IDENTIFIER, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_IDENTIFIER, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_VARIABLE, RGB_DIFFBLACK);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_VARIABLE, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_VARIABLE, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_SYSTEMVARIABLE, RGB_DIFFBLACK);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_SYSTEMVARIABLE, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_SYSTEMVARIABLE, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_KNOWNSYSTEMVARIABLE, RGB_DIFFBLACK);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_KNOWNSYSTEMVARIABLE, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_KNOWNSYSTEMVARIABLE, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DATABASEOBJECT, RGB_DIFFBLACK);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DATABASEOBJECT, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_DATABASEOBJECT, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_PROCEDUREKEYWORD, RGB_DIFFBLACK);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_PROCEDUREKEYWORD, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_PROCEDUREKEYWORD, FALSE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_KEYWORD, RGB_DIFFBLUE);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_KEYWORD, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_KEYWORD, TRUE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_MAJORKEYWORD, RGB_DIFFBLUE);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_MAJORKEYWORD, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_MAJORKEYWORD, TRUE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_FUNCTION, DEF_FUNCTIONCOLOR);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_FUNCTION, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_FUNCTION, TRUE);

    if(isinfotab == wyTrue)
    {
        SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_STRING, RGB_DIFFBLACK);
        SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_SQSTRING, RGB_DIFFBLACK);
        SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DQSTRING, RGB_DIFFBLACK);
    }
    else
    {
        SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_STRING, DEF_STRINGCOLOR);
        SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_SQSTRING, DEF_STRINGCOLOR);
        SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DQSTRING, DEF_STRINGCOLOR);
    }
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_STRING, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_STRING, TRUE);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_SQSTRING, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_SQSTRING, TRUE);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DQSTRING, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_DQSTRING, TRUE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENT, DEF_COMMENTCOLOR);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_COMMENT, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_COMMENT, TRUE);
    
    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENTLINE, DEF_COMMENTCOLOR);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_COMMENTLINE, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_COMMENTLINE, TRUE);
    
    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_HIDDENCOMMAND, DEF_COMMENTCOLOR);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_HIDDENCOMMAND, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_HIDDENCOMMAND, TRUE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_OPERATOR, DEF_OPERATORCOLOR);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_OPERATOR, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_OPERATOR, TRUE);

    SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_NUMBER, RGB_DIFFBLACK);
    SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_NUMBER, COLOR_WHITE);
    SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_NUMBER, FALSE);

    SendMessage(hwndedit, SCI_STYLESETCASE, SCE_MYSQL_KEYWORD, SC_CASE_UPPER);
    SendMessage(hwndedit, SCI_STYLESETCASE, SCE_MYSQL_MAJORKEYWORD, SC_CASE_UPPER);
    SendMessage(hwndedit, SCI_STYLESETCASE, SCE_MYSQL_FUNCTION, SC_CASE_UPPER);
}


wyBool
EditorFont::IsMainWindowEditor(HWND hwndedit)
{
	if (!hwndedit)
		return wyFalse;

	EDITORINFO editorInfo = wyTheme::IdentifyEditorType(hwndedit);
	
	// TabPreview editors should NOT get dark theme
	if (editorInfo.isTabPreviewEditor)
	{
		return wyFalse;
	}

	HWND hwndCurrent = hwndedit;
	wyWChar className[256];
	int controlId = editorInfo.controlId;
	
	for (int level = 0; level < 6; level++)
	{
		hwndCurrent = GetParent(hwndCurrent);
		if (!hwndCurrent)
			break;
			
		GetClassName(hwndCurrent, className, sizeof(className)/sizeof(wyWChar));
		
		// check if parentclass is dialog
		if (wcsstr(className, L"#32770") != NULL)  
			return wyFalse;
			
		
		if (wcsstr(className, L"MDIClient") != NULL)
			return wyTrue;
	}
	
	switch (controlId)
	{
		// These are known dialog editor IDs that should get dark theme
		case IDC_QUERYEDIT:        
		case IDC_QUERYMESSAGEEDIT: 
		case IDC_HISTORY:          
		case IDC_TABLEDATAEDIT:    
		case IDC_QUERY:            
			return wyTrue;
		
		// These are known dialog editor IDs that should NOT get dark theme
		case IDC_QUERYPREVIEW:     
		case IDC_EDIT:             
		case IDC_DBEDIT:           
		case IDC_EDITPREFDEMO:    
			return wyFalse;
			
		default:
			HWND hwndTopLevel = hwndCurrent;
			while (GetParent(hwndTopLevel))
				hwndTopLevel = GetParent(hwndTopLevel);
				
			GetClassName(hwndTopLevel, className, sizeof(className)/sizeof(wyWChar));

			return (wcsstr(className, L"#32770") == NULL) ? wyTrue : wyFalse;
	}
}

void
EditorFont::GetColorFromIni(HWND hwndedit, wyWChar directory[], EDITORCOLORINFO *editorcolors, wyBool isinfotab)
{

	COLORREF    color, bgcolor = RGB(255, 255, 255), forecolor, backcolor;
	wyString    dirstr(directory);

	SetThemeAwareMarginColors(hwndedit, dirstr);

	//Change selection color
	backcolor = wyIni::IniGetInt(GENERALPREFA, "SelectionBgColor", DEF_TEXTSELECTION, dirstr.GetString());
	editorcolors->m_selection = backcolor;

	bgcolor = wyIni::IniGetInt(GENERALPREFA, "EditorBgColor", DEF_BKGNDEDITORCOLOR, dirstr.GetString());
	editorcolors->m_background = bgcolor;
	//SendMessage(hwndedit, SCI_STYLESETBACK, STYLE_DEFAULT, (LPARAM)bgcolor);

	//SendMessage(hwndedit, SCI_SETCARETFORE, , 0); //Change Caret color in editor window
	editorcolors->m_caret = bgcolor ^ 0xFFFFFF;

	color = wyIni::IniGetInt(GENERALPREFA, "NormalColor", DEF_NORMALCOLOR, dirstr.GetString());
	editorcolors->m_foreground = color;


	/*SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_QUOTEDIDENTIFIER, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_QUOTEDIDENTIFIER, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_QUOTEDIDENTIFIER, FALSE);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_IDENTIFIER, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_IDENTIFIER, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_IDENTIFIER, FALSE);

	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_VARIABLE, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_VARIABLE, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_VARIABLE, FALSE);

	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_SYSTEMVARIABLE, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_SYSTEMVARIABLE, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_SYSTEMVARIABLE, FALSE);

	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_KNOWNSYSTEMVARIABLE, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_KNOWNSYSTEMVARIABLE, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_KNOWNSYSTEMVARIABLE, FALSE);

	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DATABASEOBJECT, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DATABASEOBJECT, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_DATABASEOBJECT, FALSE);

	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_PROCEDUREKEYWORD, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_PROCEDUREKEYWORD, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_PROCEDUREKEYWORD, FALSE);

	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DEFAULT, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DEFAULT, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_DEFAULT, FALSE);
	*/

	color = wyIni::IniGetInt(GENERALPREFA, "CommentColor", DEF_COMMENTCOLOR, dirstr.GetString());
	editorcolors->m_comment = color;
	/*SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENTLINE, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_COMMENTLINE, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENT, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_COMMENT, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_COMMENT, TRUE);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_COMMENTLINE, TRUE);
	*/
	color = wyIni::IniGetInt(GENERALPREFA, "HiddenCmdColor", DEF_COMMENTCOLOR, dirstr.GetString());
	editorcolors->m_hiddencmd = color;
	/*SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_HIDDENCOMMAND, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_HIDDENCOMMAND, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_HIDDENCOMMAND, TRUE);
	*/

	//In info tab, no highlighting for string literals
	if (isinfotab == wyTrue)
		color = wyIni::IniGetInt(GENERALPREFA, "NormalColor", DEF_NORMALCOLOR, dirstr.GetString());
	else
		color = wyIni::IniGetInt(GENERALPREFA, "StringColor", DEF_STRINGCOLOR, dirstr.GetString());
	
	editorcolors->m_string = color;
/*	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_STRING, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_STRING, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_STRING, TRUE);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_SQSTRING, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_SQSTRING, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_SQSTRING, TRUE);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DQSTRING, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_DQSTRING, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_DQSTRING, TRUE);
	*/

	color = wyIni::IniGetInt(GENERALPREFA, "KeywordColor", DEF_KEYWORDCOLOR, dirstr.GetString());
	editorcolors->m_keyword = color;
	/*SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_KEYWORD, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_KEYWORD, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_KEYWORD, TRUE);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_MAJORKEYWORD, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_MAJORKEYWORD, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_MAJORKEYWORD, TRUE);
	*/
	color = wyIni::IniGetInt(GENERALPREFA, "FunctionColor", DEF_FUNCTIONCOLOR, dirstr.GetString());
	editorcolors->m_function = color;
	/*SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_FUNCTION, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_FUNCTION, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_FUNCTION, TRUE);
	*/
	color = wyIni::IniGetInt(GENERALPREFA, "OperatorColor", DEF_OPERATORCOLOR, dirstr.GetString());
	editorcolors->m_operator = color;
	/*SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_OPERATOR, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_OPERATOR, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_OPERATOR, TRUE);
	*/

	color = wyIni::IniGetInt(GENERALPREFA, "NumberColor", DEF_NORMALCOLOR, dirstr.GetString());
	editorcolors->m_number = color;
	/*SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_NUMBER, color);
	SendMessage(hwndedit, SCI_STYLESETBACK, SCE_MYSQL_NUMBER, bgcolor);
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_NUMBER, FALSE);
	*/
}



void
EditorFont::GetColorDefault(HWND hwndedit, EDITORCOLORINFO *editorcolors, wyBool isinfotab)
{

	editorcolors->m_foreground = RGB(0, 0, 0);
	//SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_WORD, RGB(0,0,0));
	//SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_WORD, FALSE);

	//SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_TWIG,RGB(0,0,0));
	//SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_TWIG,FALSE);
	/*
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_DEFAULT, RGB(0, 0, 0));
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_DEFAULT, FALSE);

	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_QUOTEDIDENTIFIER, RGB(0, 0, 0));
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_QUOTEDIDENTIFIER, FALSE);

	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_IDENTIFIER, RGB(0, 0, 0));
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_IDENTIFIER, FALSE);
	*/
	editorcolors->m_keyword = RGB(0, 0, 255);
	editorcolors->m_function = RGB(255, 0, 255);

/*	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_KEYWORD, RGB(0, 0, 255));
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_KEYWORD, TRUE);

	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_FUNCTION, RGB(255, 0, 255));
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_FUNCTION, TRUE);
	*/
	//In info tab, no highlighting for string literals
	if (isinfotab == wyTrue) {
		//SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_STRING, RGB(0, 0, 0));
		editorcolors->m_string = RGB(0, 0, 0);
	}
	else {
		editorcolors->m_string = RGB(255, 0, 0);
		//SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_STRING, RGB(255, 0, 0));
	}


	//SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_STRING, TRUE);

	editorcolors->m_comment = RGB(0, 128, 128);
	editorcolors->m_hiddencmd = RGB(0, 128, 128);
	/*
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENT, RGB(0, 128, 128));
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_COMMENTLINE, RGB(0, 128, 128));
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_COMMENT, TRUE);
	SendMessage(hwndedit, SCI_STYLESETFORE, SCE_MYSQL_HIDDENCOMMAND, RGB(0, 128, 128));
	SendMessage(hwndedit, SCI_STYLESETBOLD, SCE_MYSQL_HIDDENCOMMAND, TRUE);
	*/

}
