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

	SetColor(hwndedit, fromini);
	
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
}

void 
EditorFont::SetColorFromIni(HWND hwndedit, wyWChar directory[], wyBool isinfotab)
{

	COLORREF	color,bgcolor=RGB(255,255,255),forecolor,backcolor;
	wyString	dirstr(directory);

	//Change Margin color
    forecolor   =   wyIni::IniGetInt(GENERALPREFA, "NumberMarginFgColor",   RGB(0,0,0), dirstr.GetString());
    backcolor   =   wyIni::IniGetInt(GENERALPREFA, "NumberMarginBgColor",   DEF_MARGINNUMBER, dirstr.GetString());
    SendMessage(hwndedit, SCI_STYLESETFORE,STYLE_LINENUMBER,forecolor);
    SendMessage(hwndedit,SCI_STYLESETBACK,STYLE_LINENUMBER,backcolor);   
    
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

	
    color = wyIni::IniGetInt( GENERALPREFA, "KeywordColor", DEF_KEYWORDCOLOR, dirstr.GetString() ); 	
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