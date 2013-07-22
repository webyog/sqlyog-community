/*
 * Terra Informatica Lightweight Embeddable HTMLayout control.
 *
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 *
 * http://terrainformatica.com/htmlayout
 *
 * (C) 2003, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

#ifndef __htmprint_h__
#define __htmprint_h__

#include "htmlayout.h"
#include "htmlayout_notifications.hpp"

typedef void* HTMPRINT;

typedef enum tagHPRESULT
{
  HPR_OK = 0,
  HPR_INVALID_HANDLE,
  HPR_INVALID_FORMAT,
  HPR_FILE_NOT_FOUND,
  HPR_INVALID_PARAMETER,
  HPR_INVALID_STATE, // attempt to do operation on empty document
} HPRESULT;


//|
//| file htmprint.h
//|
//| Printing (windowless HTML rendering) API.
//|

//| Create instance of the engine
EXTERN_C  HTMPRINT HLAPI HTMPrintCreateInstance();

//| Destroy instance of the engine
EXTERN_C  HPRESULT HLAPI HTMPrintDestroyInstance(HTMPRINT hPrint);

//| Set custom tag value to the instance of the engine.
//| This tag value will be used in all callback invocations.
EXTERN_C  HPRESULT HLAPI HTMPrintSetTag(HTMPRINT hPrint, LPVOID tag);

//| Get custom tag value from the instance of the engine.
EXTERN_C  HPRESULT HLAPI HTMPrintGetTag(HTMPRINT hPrint, LPVOID *tag);

//| Load HTML from file 
EXTERN_C  HPRESULT HLAPI HTMPrintLoadHtmlFromFile(HTMPRINT hPrint, LPCSTR path);
//| Load HTML from file 
EXTERN_C  HPRESULT HLAPI HTMPrintLoadHtmlFromFileW(HTMPRINT hPrint, LPCWSTR path);


//| Load HTML from memory buffer
EXTERN_C  HPRESULT HLAPI HTMPrintLoadHtmlFromMemory(HTMPRINT hPrint,LPCSTR baseURI, LPCBYTE dataptr, DWORD datasize);

//| Measure loaded HTML
EXTERN_C  HPRESULT HLAPI HTMPrintMeasure(HTMPRINT hPrint, HDC hdc,
          int scaledWidth,    // number of screen pixels in viewportWidth
          int viewportWidth,  // width of rendering area in device (physical) units  
          int viewportHeight,  // height of rendering area in device (physical) units  
          int* pOutNumberOfPages); // out, number of pages if HTML does not fit into viewportHeight

//| Render , per se
EXTERN_C  HPRESULT HLAPI HTMPrintRender(HTMPRINT hPrint, HDC hdc, 
          int viewportX, // x position of rendering area in device (physical) units
          int viewportY, // y position of rendering area in device (physical) units   
          int pageNo);   // number of page to render

//| Set data of requested resource 
EXTERN_C  HPRESULT HLAPI HTMPrintSetDataReady(HTMPRINT hPrint, LPCSTR url, LPCBYTE data, DWORD dataSize);

//| Get minimum width of loaded document 
EXTERN_C  HPRESULT HLAPI HTMPrintGetDocumentMinWidth(HTMPRINT hPrint, LPDWORD v);

//| Get minimum height of loaded document measured for viewportWidth
EXTERN_C  HPRESULT HLAPI HTMPrintGetDocumentHeight(HTMPRINT hPrint, LPDWORD v);

//| Set media type for CSS engine, use this before loading the document
//| See: http://www.w3.org/TR/REC-CSS2/media.html 
EXTERN_C  HPRESULT HLAPI HTMPrintSetMediaType(HTMPRINT hPrint, LPCSTR mediatype);

//|
//| Callback function type
//|
//| HtmLayout will call it each time when it needs to to download any type of external
//| resource referred in this document - image, script or CSS.
//|
//| return FALSE if you dont want to load any images
//| return TRUE if you want image to be provessed. You may call set_data_ready() to supply your own
//| image or css content
//| 
//| While handling this callback host application may call HTMPrintSetDataReady
//|

typedef BOOL CALLBACK HTMPRINT_LOAD_DATA(HTMPRINT hPrint, LPVOID tag, LPCSTR url);

//| Set callback method. tag is used for passing into 
//| future calls of HTMPRINT_LOAD_DATA "as is"
EXTERN_C  HPRESULT HLAPI HTMPrintSetLoadDataCallback(HTMPRINT hPrint, HTMPRINT_LOAD_DATA* cb);

//| Set standard notification callback handler. tag is used for passing into 
//| future calls of LPHTMLAYOUT_NOTIFY "as is"
EXTERN_C  HPRESULT HLAPI HTMPrintSetCallback(HTMPRINT hPrint, LPHTMLAYOUT_NOTIFY cb, LPVOID cbParam);

//|
//| Callback function type
//|
//| HtmLayout will call it each time when it renderes hyperlinked area
//|
typedef VOID CALLBACK HTMPRINT_HYPERLINK_AREA(HTMPRINT hPrint, LPVOID tag, RECT* area, LPCSTR url);

//| Set callback method. tag is used for passing into 
//| future calls of HTMPRINT_LOAD_DATA "as is"
EXTERN_C  HPRESULT HLAPI HTMPrintSetHyperlinkAreaCallback(HTMPRINT hPrint, HTMPRINT_HYPERLINK_AREA* cb);


//|
//| Callback function type: next page required.
//|
//| HtmLayout will call it in measuring phase when next page is needed.
//| Parameters:
//|   pPageViewportHeight - address of current pageViewportHeight variable. On input it contains viewportHeight value.
//|                         You can change this value thus to handle pages with variable content area heights.
//|                          

typedef VOID CALLBACK HTMPRINT_NEXT_PAGE(HTMPRINT hPrint, LPVOID tag, 
              UINT pageNo, /*in, number of page to be measured */ 
              INT pageOffsetY, /*in, "CSS pixels", offset of this page from the beginning of the document */ 
              LPUINT pPageViewportHeight, /*inout, "CSS pixels", current height of the content area (viewport) on the page */
              LPBOOL pbContinue /*out, true if you need to measure document further */
            );

//| Set callback method. tag is used for passing into 
//| future calls of HTMPRINT_LOAD_DATA "as is"
EXTERN_C  HPRESULT HLAPI HTMPrintSetNextPageCallback(HTMPRINT hPrint, HTMPRINT_NEXT_PAGE* cb);



//| Get root DOM element of loaded HTML document. 
EXTERN_C  HPRESULT HLAPI HTMPrintGetRootElement(HTMPRINT hPrint, HELEMENT* phe);

#ifdef __cplusplus 

// C++ wrapper 

class  PrintEx: 
       public htmlayout::notification_handler<PrintEx>
{
  HTMPRINT                hPrint;

  static VOID CALLBACK hyperlinkArea(HTMPRINT hPrint, LPVOID tag, RECT* area, LPCSTR url)
  {
    PrintEx *pex = (PrintEx *)tag;
    assert(pex);
    pex->registerHyperlinkArea(area, url);
  }

  static VOID CALLBACK nextPage(HTMPRINT hPrint, LPVOID tag, UINT pageNo, INT pageOffsetY, LPUINT pPageViewportHeight, LPBOOL pbContinue)
  {
    PrintEx *pex = (PrintEx *)tag;
    assert(pex);
    pex->onNextPage(pageNo, pageOffsetY, *pPageViewportHeight);
  }

public:
  PrintEx(const char* mediaType = "print"):hPrint(0) 
  {
    hPrint = HTMPrintCreateInstance();
    assert(hPrint);
    if(hPrint)
    {
      // set tag 
      HTMPrintSetTag(hPrint,this);
      // register callbacks
      setup_callback(hPrint);
      HTMPrintSetHyperlinkAreaCallback(hPrint, &hyperlinkArea );
      HTMPrintSetNextPageCallback(hPrint, &nextPage );
      //set media type
      HTMPrintSetMediaType(hPrint, mediaType);
    }
  }
  ~PrintEx() { destroy(); }
  
  void destroy() 
  { 
    if(hPrint) 
    {
      HTMPrintDestroyInstance(hPrint);
      hPrint = 0;
    }
  }
  bool load(LPCSTR path)
  {
    HPRESULT hr = HTMPrintLoadHtmlFromFile(hPrint, path);
    assert(hr == HPR_OK);
    return hr == HPR_OK;
  }
  bool load(LPCWSTR path)
  {
    HPRESULT hr = HTMPrintLoadHtmlFromFileW(hPrint, path);
    assert(hr == HPR_OK);
    return hr == HPR_OK;
  }
  bool load(LPCBYTE dataptr, DWORD datasize, LPCSTR baseURI)
  {
    HPRESULT hr = HTMPrintLoadHtmlFromMemory(hPrint, baseURI, dataptr,datasize);
    assert(hr == HPR_OK);
    return hr == HPR_OK;
  }

  int  measure(HDC hdc,
          int scaledWidth,    // number of screen pixels in viewportWidth
          int viewportWidth,  // width of rendering area in device (physical) units  
          int viewportHeight) // height of rendering area in device (physical) units  
          //return number of pages
  {
    int numPages = 0;
    HPRESULT hr = HTMPrintMeasure(hPrint, hdc, scaledWidth, viewportWidth, viewportHeight, &numPages);
    assert(hr == HPR_OK);
    return (hr == HPR_OK)?numPages:0;
  }

  bool  render(HDC hdc, int viewportX, int viewportY, int pageNo)
  {
    HPRESULT hr = HTMPrintRender(hPrint, hdc,viewportX, viewportY, pageNo);
    assert(hr == HPR_OK);
    return hr == HPR_OK;
  }

  bool  setDataReady(LPCSTR url, LPCBYTE data, DWORD dataSize)
  {
    HPRESULT hr = HTMPrintSetDataReady(hPrint, url, data, dataSize);
    assert(hr == HPR_OK);
    return hr == HPR_OK;
  }
  
  // Get current document measured height for width
  // given in measure scaledWidth/viewportWidth parameters.
  // ATTN: You need call first measure to get valid result.
  // retunrn value is in screen pixels.

  DWORD getDocumentHeight()
  {
    DWORD v;
    HPRESULT hr = HTMPrintGetDocumentHeight(hPrint, &v);
    assert(hr == HPR_OK); 
    return (hr == HPR_OK)?v:0;
  }

  // Get current document measured minimum (intrinsic) width.
  // ATTN: You need call first measure to get valid result.
  // return value is in screen pixels.

  DWORD getDocumentMinWidth()
  {
    DWORD v;
    HPRESULT hr = HTMPrintGetDocumentMinWidth(hPrint, &v);
    assert(hr == HPR_OK); 
    return (hr == HPR_OK)?v:0;
  }

  HELEMENT getRootElement()
  {
    HELEMENT v;
    HPRESULT hr = HTMPrintGetRootElement(hPrint, &v);
    assert(hr == HPR_OK); 
    return (hr == HPR_OK)?v:0;
  }
  
  
  // override this if you need other image loading policy
  virtual bool loadUrlData(LPCSTR url) 
  {
    url;
    return true; // proceed with default image loader
    
    /* other options are: */

    /* discard image loading at all:
    return false;
    */

    /* to load data from your own namespace simply call: 
    set_data_ready(url,data,dataSize);
    return true;
    */
  }

  // override this if you want some special processing for hyperlinks. 
  virtual void registerHyperlinkArea(const RECT* area, LPCSTR url)
  {
    area;url;
    //e.g. PDF output.
  }

  // override this if you have variable height pages. 
  virtual void onNextPage(unsigned int pageNo, int pageOffsetY, unsigned int& pageViewportHeight)
  {
     pageNo; pageViewportHeight; pageOffsetY;
  }



};
#endif //__cplusplus 


#endif
