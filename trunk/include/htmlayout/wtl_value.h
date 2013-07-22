//
// Windows Template Library Extension for
// Terra Informatica Lightweight Embeddable HTML Editor
// http://terrainformatica.com/htmengine
//
// Written by Andrew Fedoniouk <andrew@terrainformatica.com>

//
// This file is NOT a part of Windows Template Library.
// The code and information is provided "as-is" without
// warranty of any kind, either expressed or implied.
//
// (C) 2004, Andrew Fedoniouk <andrew@TerraInformatica.com>
//

#ifndef __ATLVALUE_H__
#define __ATLVALUE_H__

#pragma once

#ifndef __cplusplus
  #error ATL requires C++ compilation (use a .cpp suffix)
#endif

/* 
   WARNING!
   THIS HEADER IS OBSOLETE!!!!!!!!   

   Please use json::value instead.

*/


#pragma warning( disable : 4996 )

#include <atlmisc.h>

/////////////////////////////////////////////////////////////////////////////
// Classes in this file
//
// CValueT
//

namespace WTL
{
  template <typename UNITCTL> class CValueT;

  struct UnitsCtl 
  {
     int dummy;
     UnitsCtl(): dummy(0) {}
     void Clear() { /* do nothing in this implementation*/ }
     // cvt to string form
     CString           ToString(const CValueT<UnitsCtl>& v) const;
     // cvt from string form
     CValueT<UnitsCtl> FromString(LPCTSTR str) const;
     // cvt from one type/units to another
     CValueT<UnitsCtl> Convert(const CValueT<UnitsCtl>& src) const;
     // default units used for costruction
     static UnitsCtl 
                defUnits() { static UnitsCtl dv; return dv; }
  };

template <typename UNITCTL>
  class CValueT 
  {
  public:
    typedef enum _CValueType { V_UNDEFINED, V_BOOL, V_INT, V_REAL, V_STRING, V_WSTRING } CValueType;

  protected:
    struct stringData 
    {
      long nRefs;
      void AddRef() { InterlockedIncrement(&nRefs); }
      void Release() { if (InterlockedDecrement(&nRefs) <= 0) delete[] (BYTE*)this; }
      static stringData* AllocateA(LPCSTR lpsz) 
              { 
                size_t sz = strlen(lpsz);
                stringData* pd = (stringData*)new BYTE[ sizeof(stringData) + sizeof(CHAR) * (sz + 1) ]; 
                strcpy((CHAR*)pd->data(),lpsz);
                pd->nRefs = 1; return pd;
              }
      static stringData* AllocateW(LPCWSTR lpsz) 
              { 
                size_t sz = wcslen(lpsz);
                stringData* pd = (stringData*)new BYTE[ sizeof(stringData) + sizeof(WCHAR) * (sz + 1) ]; 
                wcscpy((WCHAR*)pd->data(),lpsz);
                pd->nRefs = 1; return pd;
              }
      void* data() const { return (void *)(this + 1); }
    };

    union dataSlot 
    {
      int           iVal;
      double        rVal;
      __int64       lVal;
      stringData*   sVal;
    } uData;

    CValueType    vtType;
    UNITCTL       ctlUnits;

  public:
    CValueT() :vtType(V_UNDEFINED) { uData.lVal = 0; }
    CValueT(bool v, UNITCTL units = UNITCTL::defUnits())            :vtType(V_UNDEFINED) { Set(v,units); }
    CValueT(int v, UNITCTL units = UNITCTL::defUnits())             :vtType(V_UNDEFINED) { Set(v,units); }
    CValueT(double v, UNITCTL units = UNITCTL::defUnits())          :vtType(V_UNDEFINED) { Set(v,units); }
    CValueT(LPCSTR lpsz, UNITCTL units = UNITCTL::defUnits())       :vtType(V_UNDEFINED) { Set(lpsz,units); }
    CValueT(LPCWSTR lpsz, UNITCTL units = UNITCTL::defUnits())      :vtType(V_UNDEFINED) { Set(lpsz,units); }
    CValueT(const CValueT& src, UNITCTL units = UNITCTL::defUnits()) :vtType(V_UNDEFINED) { Set(src); ctlUnits = units; }
    
    CValueType  Type() const { return vtType; }
    UNITCTL     Units() const { return ctlUnits; }

    void        Clear() { if(IsText()) uData.sVal->Release(); uData.lVal = 0; vtType = V_UNDEFINED; ctlUnits.Clear(); }

    void        Set(const CValueT& src) 
    { 
      Clear(); 
      vtType = src.vtType; 
      ctlUnits = src.ctlUnits; 
      uData.lVal = src.uData.lVal;
      if(IsText()) 
        uData.sVal->AddRef(); 
    }
    void        Set(const CValueT& src, UNITCTL units ) 
    { 
        ATLASSERT(&src != this);
        Set(units.Convert(src)); 
    }

    void        Set(LPCSTR lpsz, UNITCTL units = UNITCTL::defUnits())  { Clear(); if(lpsz && lpsz[0]) { uData.sVal = stringData::AllocateA(lpsz); vtType = V_STRING; ctlUnits = units; } }
    void        Set(LPCWSTR lpsz, UNITCTL units = UNITCTL::defUnits()) { Clear(); if(lpsz && lpsz[0]) { uData.sVal = stringData::AllocateW(lpsz); vtType = V_WSTRING; ctlUnits = units; } }
    void        Set(int i, UNITCTL units = UNITCTL::defUnits())        { Clear(); uData.iVal = i; vtType = V_INT; ctlUnits = units; }
    void        Set(bool b, UNITCTL units = UNITCTL::defUnits())       { Clear(); uData.iVal = (int)b; vtType = V_BOOL; ctlUnits = units; }
    void        Set(double r, UNITCTL units = UNITCTL::defUnits())     { Clear(); uData.rVal = r; vtType = V_REAL; ctlUnits = units; }

    ~CValueT()  { Clear(); }

    bool IsUndefined() const { return (vtType == V_UNDEFINED); }
    bool IsText() const { return (vtType == V_STRING) || (vtType == V_WSTRING); }
    bool IsInt() const { return (vtType == V_INT); }
    bool IsReal() const { return (vtType == V_REAL); }
    bool IsBool() const { return (vtType == V_BOOL); }
    bool IsSTR() const { return (vtType == V_STRING); }
    bool IsWSTR() const { return (vtType == V_WSTRING); }

    CValueT& operator = (LPCWSTR lps) { Set(lps); return *this; }
    CValueT& operator = (LPCSTR lps) { Set(lps); return *this; }
    CValueT& operator = (int v) { Set(v); return *this; }
    CValueT& operator = (double v) { Set(v); return *this; }
    CValueT& operator = (bool v) { Set(v); return *this; }

    int GetInt() const 
    { 
      ATLASSERT(vtType == V_INT);
      return (vtType == V_INT)? uData.iVal: 0;
    }
    double GetReal() const  
    { 
      ATLASSERT(vtType == V_REAL);
      return (vtType == V_REAL)? uData.rVal: 0.0;
    }
    bool GetBool() const 
    { 
      ATLASSERT(vtType == V_BOOL);
      return (vtType == V_BOOL)? uData.iVal != 0 :false;
    }
    LPCSTR GetSTR() const 
    {
      ATLASSERT(vtType == V_STRING);
      return  (vtType == V_STRING)? (LPCSTR)uData.sVal->data():"";
    }
    LPCWSTR GetWSTR() const 
    {
      ATLASSERT(vtType == V_WSTRING);
      return (vtType == V_WSTRING)? (LPCWSTR)uData.sVal->data():L"";
    }
    CString GetText() const 
    {
      ATLASSERT(vtType == V_STRING || vtType == V_WSTRING);
      if(vtType == V_STRING)
        return CString( (LPCSTR) uData.sVal->data() );
      if(vtType == V_WSTRING)
        return CString( (LPCWSTR) uData.sVal->data() );
      return CString();
    }

    CString ToString() const { return ctlUnits.ToString(*this); }
    void    FromString(LPCTSTR str) { Set(ctlUnits.FromString(str)); }


  };

// default implementation of Units controller

  inline CValueT<UnitsCtl>  UnitsCtl::FromString(LPCTSTR lpsz) const
  {
    //blind conversion in default units controller
    if(lpsz == 0 || lpsz[0] == 0)
      return CValueT<UnitsCtl>(); // undefined

    LPCTSTR endptr = lpsz + _tcslen(lpsz);
    TCHAR* lastptr;
    int i = _tcstol(lpsz,&lastptr,10);
    if(lastptr == endptr)
      return CValueT<UnitsCtl>(i);
    double d = _tcstod(lpsz,&lastptr);
    if(lastptr == endptr)
      return CValueT<UnitsCtl>(d);
    if(_tcsicmp(lpsz,TEXT("true"))==0 || _tcsicmp(lpsz,TEXT("yes"))==0)
      return CValueT<UnitsCtl>(true);
    if(_tcsicmp(lpsz,TEXT("false"))==0 || _tcsicmp(lpsz,TEXT("no"))==0)
      return CValueT<UnitsCtl>(false);

    return CValueT<UnitsCtl>(lpsz);
  }

  inline CString UnitsCtl::ToString(const CValueT<UnitsCtl>& v) const
  {
    CString out;
    switch(v.Type())
    {
    case CValueT<UnitsCtl>::V_UNDEFINED:
      out = TEXT("{undefined}");
      break;
    case CValueT<UnitsCtl>::V_BOOL:
      out = v.GetBool()? TEXT("true"):TEXT("false");
      break;
    case CValueT<UnitsCtl>::V_INT:
      out.Format(TEXT("%d"),v.GetInt());
      break;
    case CValueT<UnitsCtl>::V_REAL:
      out.Format(TEXT("%f"),v.GetReal());
      break;
    case CValueT<UnitsCtl>::V_STRING:
    case CValueT<UnitsCtl>::V_WSTRING:
      return v.GetText();
    }
    return out;
  }

  inline CValueT<UnitsCtl> UnitsCtl::Convert(const CValueT<UnitsCtl>& src) const 
  {
    // nothing spectacular here, just copy.
    return CValueT<UnitsCtl>(src);
  }

  typedef CValueT<UnitsCtl> CValue;

} //namespace WTL


#endif // __ATLVALUE_H__
