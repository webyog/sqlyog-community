#ifndef __htmlayoutex_h__
#define __htmlayoutex_h__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>

#include "htmlayout.h"

// handle to extended DIB object
typedef void *    HDIBEX;

typedef enum tagHLRESULT
{
  HLR_OK = 0,
  HLR_INVALID_HANDLE,
  HLR_INVALID_FORMAT,
  HLR_FILE_NOT_FOUND,
  HLR_INVALID_PARAMETER,
  HLR_INVALID_STATE, // attempt to do operation on empty document
} HLRESULT;

#ifdef __cplusplus
inline COLORREF SYSCOLOR(int SysColorId) { return SysColorId | 0xFF000000; }
#else
#define SYSCOLOR( SysColorId ) ( SysColorId | 0xFF000000 )
#endif

// setHdibMargins stretch flags
#define DIBEX_STRETCH_TOP           0x1
#define DIBEX_STRETCH_LEFT          0x2
#define DIBEX_STRETCH_CENTER        0x4
#define DIBEX_STRETCH_RIGHT         0x8
#define DIBEX_STRETCH_BOTTOM        0x10

#ifdef __cplusplus

typedef struct tagHTMLAYOUT_HDIB_API
{
  virtual HLRESULT __stdcall loadHdibFromFile(LPCSTR path, HDIBEX* pOutHDib) = 0;
  virtual HLRESULT __stdcall loadHdibFromMemory(LPCBYTE dataptr, DWORD datasize, HDIBEX* pOutHDib) = 0;
  virtual HLRESULT __stdcall setHdibMargins(HDIBEX hdib, int marginLeft, int marginTop, int marginRight, int marginBottom, int stretchFlags) = 0;
  virtual HLRESULT __stdcall destroyHdib(HDIBEX hdib) = 0;
  virtual HLRESULT __stdcall getHdibInfo(HDIBEX hdib, LPBITMAPINFO pBbitmapInfo) = 0;
  virtual HLRESULT __stdcall getHdibBits(HDIBEX hdib,LPBYTE *ppBytes, LPDWORD pNumOfBytes) = 0;
  virtual HLRESULT __stdcall renderHdib(HDC dstDC, int dstX, int dstY, int dstWidth, int dstHeight, HDIBEX hdib) = 0;
  virtual HLRESULT __stdcall renderHdib(HDC dstDC, int dstX, int dstY, int dstWidth, int dstHeight, int srcX, int srcY, HDIBEX hdib) = 0;
  virtual HLRESULT __stdcall renderHdib(HDC dstDC, int dstX, int dstY, int dstWidth, int dstHeight, int srcX, int srcY, int srcWidth, int srcHeight, HDIBEX hdib) = 0;
  virtual HLRESULT __stdcall setColorSchema(HDIBEX hdib,COLORREF DkShadow,COLORREF Shadow,COLORREF Face,COLORREF Light, COLORREF HighLight) = 0;
} HTMLAYOUT_HDIB_API;

#endif //__cplusplus

#define WM_HL_GET_INTERFACE (WM_USER + 0xaff)

#define HLINTERFACE_HDIB_API 0xAFED
#define HLINTERFACE_PRINTING_API (0xAFED + 1) 

#ifdef __cplusplus 
class  dibex 
{
  HTMLAYOUT_HDIB_API* papi;
  HDIBEX hdibex;

public:
  dibex():hdibex(0) 
  {
    papi = (HTMLAYOUT_HDIB_API*) HTMLayoutProc(0, WM_HL_GET_INTERFACE, HLINTERFACE_HDIB_API, 17246);
    assert(papi);
  }
  ~dibex() { destroy(); }

  bool is_valid() const { return hdibex != 0; }
  
  void destroy() 
  { 
    if( hdibex )
    {
      HLRESULT hr;
      hr = papi->destroyHdib(hdibex);
      assert(hr == HLR_OK); hr;
      hdibex = 0;
    }
  }
  bool load(LPCSTR path)
  {
    HLRESULT hr = papi->loadHdibFromFile(path,&hdibex);
    assert(hr == HLR_OK);
    return hr == HLR_OK;
  }
  bool load(LPCBYTE dataptr, DWORD datasize)
  {
    HLRESULT hr = papi->loadHdibFromMemory(dataptr,datasize,&hdibex);
    assert(hr == HLR_OK);
    return hr == HLR_OK;
  }
  bool set_margins(int left, int top, int right, int bottom, int stretch_flags)
  {
    HLRESULT hr = papi->setHdibMargins(hdibex, left, top, right, bottom, stretch_flags);
    assert(hr == HLR_OK);
    return hr == HLR_OK;
  }
  int  width()
  {
    BITMAPINFO bmi;
    HLRESULT hr;
    hr = papi->getHdibInfo(hdibex, &bmi);
    assert(hr == HLR_OK); hr;
    return bmi.bmiHeader.biWidth;
  }
  int  height()
  {
    BITMAPINFO bmi;
    HLRESULT hr;
    hr = papi->getHdibInfo(hdibex, &bmi);
    assert(hr == HLR_OK); hr;
    return bmi.bmiHeader.biHeight;
  }
  void render(HDC dstDC, const RECT& dst)
  {
    HLRESULT hr;
    hr = papi->renderHdib(dstDC, 
      dst.left, 
      dst.top, 
      dst.right - dst.left + 1,
      dst.bottom - dst.top + 1, hdibex);      
    assert(hr == HLR_OK); hr;
  }
  void render(HDC dstDC, const POINT& dst)
  {
    BITMAPINFO bmi;
    HLRESULT hr = papi->getHdibInfo(hdibex, &bmi);
    assert(hr == HLR_OK);
    hr = papi->renderHdib(dstDC, 
      dst.x, 
      dst.y, 
      bmi.bmiHeader.biWidth,
      bmi.bmiHeader.biHeight, hdibex);      
    assert(hr == HLR_OK); hr;
  }
  void render(HDC dstDC, const RECT& dst, const POINT& src)
  {
    HLRESULT hr;
    hr = papi->renderHdib(dstDC, 
      dst.left, 
      dst.top, 
      dst.right - dst.left + 1,
      dst.bottom - dst.top + 1, src.x, src.y, hdibex);      
    assert(hr == HLR_OK); hr;
  }
  void render(HDC dstDC, const RECT& dst, const RECT& src)
  {
    HLRESULT hr;
    hr = papi->renderHdib(dstDC, 
      dst.left, 
      dst.top, 
      dst.right - dst.left,
      dst.bottom - dst.top, 
      src.left, src.right, 
      src.right - src.left,
      src.bottom - src.top, 
      hdibex);      
    assert(hr == HLR_OK); hr;
  }

  void set_color_schema(COLORREF DkShadow,COLORREF Shadow,COLORREF Face,COLORREF Light, COLORREF HighLight)
  {
    HLRESULT hr;
    hr = papi->setColorSchema(hdibex,DkShadow,Shadow,Face,Light,HighLight);
    assert(hr == HLR_OK); hr;
  }

};

#endif //__cplusplus 


#endif
