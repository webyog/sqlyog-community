#include "behavior_aux.h"

#include <math.h>

#ifdef _WIN32_WCE
#include <atlbase.h>
#include <atlapp.h>
#include <atlmisc.h>
#endif

namespace htmlayout 
{

/*
BEHAVIOR: path
    goal: Renders content of the element with path ellipsis.
TYPICAL USE CASE:
    <td style="behavior:path; overflow-x:hidden; white-space:nowrap;text-overflow:ellipsis; ">
SAMPLE:
*/

// Source: /Program Files/Test/Test/test.wav
// Dest:   /Program Files/Te.../test.wav

#ifdef _WIN32_WCE

static int StringWidth(LPCWSTR s, HDC dc)
{
   SIZE size;
   GetTextExtentPoint32W(dc, s, wcslen(s), &size);
   return size.cx;
}

static int CharWidth(wchar_t c, HDC dc)
{
   SIZE size;
   GetTextExtentPoint32W(dc, &c, 1, &size);
   return size.cx;
}

void FormatFileName(HDC dc, const CString &fileName, CString &dottedFileName, int boxWidth)
{
  int stringWidth = StringWidth(fileName, dc);

  dottedFileName = fileName;

  // too small file name
  if( stringWidth < boxWidth ) return;

  // extract filename and path
  CString sPath = fileName;
  CString sFile = fileName;
  int i = sPath.ReverseFind('\\');
  if (i != -1)
  {
    sPath = sPath.Left(i);
    sFile.Delete(0, i);
  }
  else return; // nothing to format

  int pathWidth = StringWidth(sPath, dc);
  int fileWidth = stringWidth - pathWidth;
  if( fileWidth >= boxWidth )
  {
    dottedFileName = sFile;
    return; // too big file name
  }

  int dotsWidth = StringWidth(CString(L"...\\"), dc);
  int curWidth = dotsWidth;
  i = 0;
  while( (curWidth < (boxWidth - fileWidth)) && (i < sPath.GetLength()) )
  {
    curWidth += CharWidth(sPath[i++], dc);
  }
  if(curWidth >= boxWidth && i > 0)
    i--;

  sPath = sPath.Left(i);
  dottedFileName = sPath + L"..." + sFile;
}

#endif

struct path: public behavior
{
    // ctor
    path(): behavior(HANDLE_DRAW, "path") {}

    virtual BOOL on_draw   (HELEMENT he, UINT draw_type, HDC hdc, const RECT& rc ) 
    { 
      if( draw_type != DRAW_CONTENT )
        return FALSE; /*do default draw*/ 

      dom::element el = he;
      
      UINT pta = GetTextAlign(hdc);
      SetTextAlign(hdc, TA_LEFT | TA_TOP |TA_NOUPDATECP); 
#ifdef _WIN32_WCE
    CString fileName(el.text());
    CString dottedFileName;
    FormatFileName(hdc, fileName, dottedFileName, rc.right - rc.left);
    DrawTextW(hdc, dottedFileName, -1, const_cast<RECT*>(&rc), DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX | DT_NOCLIP);
#else
      DrawTextW(hdc, el.text(), -1, const_cast<RECT*>(&rc), DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_PATH_ELLIPSIS | DT_NOPREFIX);
#endif
      SetTextAlign(hdc, pta); 
      return TRUE; /*skip default draw as we did it already*/ 
    }
  
};

// instantiating and attaching it to the global list
path path_instance;

} // htmlayout namespace





