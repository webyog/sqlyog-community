#include "behavior_aux.h"

#include <math.h>
#include <windows.h>
#include <commctrl.h>
#include <shellapi.h>

namespace htmlayout 
{

/*
BEHAVIOR: shell-icon
    goal: Renders icon associated with the file with the given extension.
TYPICAL USE CASE:
    <img class="shell-icon" filename="something.png" /> 
    will render icon for the png files registered in system if style of such image will be defined as:
STYLE:
    img.shell-icon { width:18px; height:18px; behavior:shell-icon;  } 
*/

struct shellicon: public behavior
{
    // ctor
    shellicon(): behavior(HANDLE_DRAW, "shell-icon") {}

/*    void draw_file_icon(HDC hdc, int x, int y, LPCWSTR filename)
    {
        SHFILEINFOW sfi;
#ifdef _WIN32_WCE
        HIMAGELIST hlist = (HIMAGELIST) SHGetFileInfo( filename, 0, &sfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON );
#else
        HIMAGELIST hlist = (HIMAGELIST) SHGetFileInfoW( filename, 0, &sfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON );
#endif
        if(hlist)
          ImageList_Draw(hlist, sfi.iIcon, hdc, x, y, ILD_TRANSPARENT); 
    }
*/
    
    virtual BOOL on_draw   (HELEMENT he, UINT draw_type, HDC hdc, const RECT& rc ) 
    { 
      if( draw_type != DRAW_CONTENT )
        return FALSE; /*do default draw*/ 

      dom::element el = he;
      const wchar_t* filename = el.get_attribute("filename");
      if( !filename )
        return FALSE;  // no such attribute at all.


      SHFILEINFOW sfi;
#ifdef _WIN32_WCE
      HIMAGELIST hlist = (HIMAGELIST) SHGetFileInfo( filename, 0, &sfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON );
#else
      HIMAGELIST hlist = (HIMAGELIST) SHGetFileInfoW( filename, 0, &sfi, sizeof(SHFILEINFO), SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX | SHGFI_SMALLICON );
#endif
      if(!hlist)
        return FALSE;
      
//#ifndef _WIN32_WCE
//      int szx = GetSystemMetrics(SM_CXSMSIZE);
//      int szy = GetSystemMetrics(SM_CYSMSIZE);
//#else
      int szx = 16;
      int szy = 16;
      ImageList_GetIconSize(hlist, &szx, &szy);
//#endif
      int x = rc.left + (rc.right - rc.left - szx) / 2;
      int y = rc.top + (rc.bottom - rc.top - szy) / 2;

      //draw_file_icon(hdc, x, y, filename);
      ImageList_Draw(hlist, sfi.iIcon, hdc, x, y, ILD_TRANSPARENT); 
           
      return TRUE; /*skip default draw as we did it already*/ 
    }

   
};

// instantiating and attaching it to the global list
shellicon shellicon_instance;

} // htmlayout namespace


