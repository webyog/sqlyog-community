//
// Windows Template Library Extension for
// Terra Informatica Lightweight Embeddable HTMLayout control
// http://terrainformatica.com/htmengine
//
// This file is NOT part of Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.
//
// (C) 2004, Andrew Fedoniouk <andrew@TerraInformatica.com>
//

#ifndef __ATLHTMENGINEVALUES_H__
#define __ATLHTMENGINEVALUES_H__

#pragma once

#ifndef __cplusplus
  #error ATL requires C++ compilation (use a .cpp suffix)
#endif

#include <atlmisc.h>
#include <atlctrls.h>
#include <wtl_htmlayout.h>
#include <wtl_value.h>

/////////////////////////////////////////////////////////////////////////////
// functions in this file
//
// CHTMLayoutCtrlT<TBase>::
//

namespace WTL
{

inline bool eq(const CString& sl, LPCTSTR sr)
 { return sl.CompareNoCase(sr) == 0; }

template <class TBase>
 inline CValue CHTMLayoutCtrlT<TBase>::GetDlgItemValue(HWND hwndControl)
 {
    ATLASSERT(::IsWindow(m_hWnd));
    ATLASSERT(::IsWindow(hwndControl));
    ATLASSERT(::GetParent(hwndControl) == m_hWnd);

    htmlayout::dom::element el = GetDlgItemElement(hwndControl);
    if( !el.is_valid() )
      return CValue();
    
    CString type = el.get_attribute("type");
    
    if(type.IsEmpty() 
      || eq(type,TEXT("text")) 
      || eq(type,TEXT("password"))
      || eq(type,TEXT("textarea")) )
    {
WINDOW_TEXT_AS_VALUE:
      int length = ::GetWindowTextLength(hwndControl) + 1;
      if( length > 1)
      {
        LPTSTR buf = (LPTSTR)_alloca( length * sizeof(TCHAR) );
        ::GetWindowText(hwndControl,buf, length);
        return CValue(buf);
      }
      return CValue(); // undefined value
    }
    else if(eq(type,TEXT("radio")))
    {
      // radio controls (option boxes) having the same name costitute group.
      // value of such group is integer - number of selected option.
      CString name = GetDlgItemName(hwndControl);
      int count = GetDlgItemNameCount(name);
      for(int i = 0; i < count; i++)
      {
        HWND hwndGroupItem = GetDlgItemByName(name,i);
        if(::SendMessage(hwndGroupItem,BM_GETCHECK,0,0) == BST_CHECKED)
          return CValue(i);
      }
      // not set
      return CValue();
    }
    else if(eq(type,TEXT("checkbox")))
    {
      // checkbox controls having the same name costitute checkbox group.
      // value of such group is integer - bitmask of all checked items in the group.
      CString name = GetDlgItemName(hwndControl);
      int count = GetDlgItemNameCount(name);
      unsigned int v = 0; unsigned int bit = 1;
      for(int i = 0; i < count && i < 32; i++)
      {
        HWND hwndGroupItem = GetDlgItemByName(name,i);
        if(::SendMessage(hwndGroupItem,BM_GETCHECK,0,0) == BST_CHECKED)
          v |= bit;
        bit <<= 1;
      }
      return CValue(int(v));
    }
    else if(eq(type,TEXT("select")))
    {
      // list and combo boxes
      CValue size = el.get_attribute("size");

      bool isCombo = size.IsUndefined() || (size.IsInt() && (size.GetInt() <= 1));

      int selitem = ::SendMessage(hwndControl,isCombo? CB_GETCURSEL:LB_GETCURSEL,0,0);
      if(selitem == CB_ERR) // no selection at all
        return CValue();
      return CValue(selitem);
    }
    /*
    else if(eq(type,TEXT("htmlarea")))
    {
      // our HTML editor 
      CHTMLayoutEditorCtrl htmed = hwndControl;
      CValue v;
      int length = htmed.SaveHtmlSize();
      if( length > 0)
      {
        LPTSTR buf = new TCHAR[length + 1];
        htmed.SaveHtml((LPBYTE)buf,length);
        buf[length] = 0;
        v = buf;
        delete[] buf;
      }
      return v;
    }*/
    else if(eq(type,TEXT("button")))
    {
      // radio controls (option boxes) having the same name costitute group.
      // value of such group is integer - number of selected option.
      CString name = GetDlgItemName(hwndControl);
      int count = GetDlgItemNameCount(name);
      for(int i = 0; i < count; i++)
      {
        HWND hwndGroupItem = GetDlgItemByName(name,i);
        if(::SendMessage(hwndGroupItem,BM_GETSTATE,0,0) == BST_PUSHED)
          return CValue(i);
      }
      // not pressed
      return CValue();
    }
    else // try to interpret window text as a value
      goto WINDOW_TEXT_AS_VALUE;      
 }

template <class TBase>
 inline void CHTMLayoutCtrlT<TBase>::GetDlgItemValues(CSimpleMap<CString,CValue>& bag)
 {
    // Get count of all child ctls controlled by HTMEngine
    int count = GetDlgItemNameCount(TEXT("*"));
    for( int i = 0; i < count; ++i )
    {
      HWND    ctlhwnd = GetDlgItemByName(TEXT("*"),i);
      CValue  ctlval = GetDlgItemValue(ctlhwnd);
      CString ctlname = GetDlgItemName(ctlhwnd);
      if(bag.FindKey(ctlname) < 0) // not found
        bag.Add(ctlname,ctlval);
    }
 }

template <class TBase>
 inline void CHTMLayoutCtrlT<TBase>::SetDlgItemValue(HWND hwndControl, CValue v)
{
    ATLASSERT(::IsWindow(m_hWnd));
    ATLASSERT(::IsWindow(hwndControl));
    ATLASSERT(::GetParent(hwndControl) == m_hWnd);

    CString type = GetDlgItemStringAttribute(hwndControl,TEXT("type"));
    
    if(type.IsEmpty() 
      || eq(type,TEXT("text")) 
      || eq(type,TEXT("password"))
      || eq(type,TEXT("textarea")) )
    {
WINDOW_TEXT_AS_VALUE:
      CString s = v.ToString();
      ::SetWindowText(hwndControl,s);
    }
    else if(eq(type,TEXT("radio")))
    {
      if(v.IsInt())
      {
        CString name = GetDlgItemName(hwndControl);
        int numberOfOptions = GetDlgItemNameCount(name);
        int idx = v.GetInt();
        ATLASSERT(idx >= 0 && idx < numberOfOptions);
        HWND hwndGroupItem = GetDlgItemByName(name,idx);
        ATLASSERT(::IsWindow(hwndGroupItem));
        ::SendMessage(hwndGroupItem,BM_SETCHECK,BST_CHECKED,0);
      }
    }
    else if(eq(type,TEXT("checkbox")))
    {
      if(v.IsInt())
      {
        CString name = GetDlgItemName(hwndControl);
        int numberOfBits = GetDlgItemNameCount(name);
        int bits = v.GetInt();
        DWORD bitMask = 1;
        for(int i = 0; i < numberOfBits; ++i)
        {
          HWND hwndGroupItem = GetDlgItemByName(name,i);
          ATLASSERT(::IsWindow(hwndGroupItem));
          ::SendMessage(hwndGroupItem,BM_SETCHECK,(bits & bitMask)?BST_CHECKED:BST_UNCHECKED,0);
          bitMask <<= 1;
        }
      }
    }
    else if(eq(type,TEXT("select")))
    {
      if(v.IsInt())
      {
        // list and combo boxes
        int size = GetDlgItemIntAttribute(hwndControl,TEXT("size"));
        bool isCombo = size <= 1;
        ::SendMessage(hwndControl,isCombo? CB_SETCURSEL:LB_SETCURSEL,v.GetInt(),0);
      }
    }
    /*
    else if(eq(type,TEXT("htmlarea")))
    {
      if(v.IsText())
      {
        // our HTML editor 
        CHTMLayoutEditorCtrl htmed = hwndControl;
        if(v.IsSTR())
          htmed.LoadHtml((LPCBYTE)v.GetSTR(),strlen(v.GetSTR()));
        else
          htmed.LoadHtml((LPCBYTE)v.GetWSTR(),wcslen(v.GetWSTR()));
      }
    }*/
    else if(eq(type,TEXT("button")))
    {
      ; // nothing here?
    }
    else // try to interpret window text as a value
      goto WINDOW_TEXT_AS_VALUE;      
}

template <class TBase>
  inline void CHTMLayoutCtrlT<TBase>::SetDlgItemValue(LPCTSTR name, CValue v)
{
  HWND hwndItem = GetDlgItemByName(name,0);
  ATLASSERT(hwndItem);
  if(hwndItem)
    SetDlgItemValue(hwndItem,v);
}

template <class TBase>
 inline void CHTMLayoutCtrlT<TBase>::SetDlgItemValues(const CSimpleMap<CString,CValue>& bag)
 {
    for( int i = bag.GetSize() - 1; i >= 0; --i )
    {
      HWND hwndItem = GetDlgItemByName(bag.GetKeyAt(i));
      ATLASSERT(hwndItem);
      if(hwndItem)
        SetDlgItemValue(ctlHwnd,bag.GetValueAt(i));
    }
 }

}

#endif
