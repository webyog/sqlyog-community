/** \mainpage Terra Informatica Lightweight Embeddable HTMLayout control.
 *
 * \section legal_sec In legalese
 *
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 *
 * <a href="http://terrainformatica.com/htmlayout">HTMLayout Home</a>
 *
 * (C) 2003-2006, Terra Informatica Software, Inc. and Andrew Fedoniouk
 *
 * \section structure_sec Structure of the documentation
 *
 * See <a href="files.html">Files</a> section.
 **/

/*!\file
\brief HTMLite - Windowless but still interactive HTML/CSS engine. 
*/

#ifndef __htmlite_h__
#define __htmlite_h__

#include "htmlayout.h"

typedef void* HTMLITE;

typedef enum tagHLTRESULT
{
  HLT_OK = 0,
  HLT_INVALID_HANDLE,
  HLT_INVALID_FORMAT,
  HLT_FILE_NOT_FOUND,
  HLT_INVALID_PARAMETER,
  HLT_INVALID_STATE, // attempt to do operation on empty document
  HLT_OK_DELAYED,    // load operation is accepted but delayed.
} HLTRESULT;


/** REFRESH_AREA notification.
 *
 * - HLN_REFRESH_AREA
 * 
 **/
#define HLN_REFRESH_AREA 0xAFF + 0x20

typedef struct tagNMHL_REFRESH_AREA
{
    NMHDR     hdr;              /**< Default WM_NOTIFY header, only code field is used in HTMLite */
    RECT      area;             /**< [in] area to refresh.*/
} NMHL_REFRESH_AREA, FAR *LPNMHL_REFRESH_AREA;

/** HLN_SET_TIMER notification.
 *
 * - HLN_SET_TIMER
 * 
 **/
#define HLN_SET_TIMER 0xAFF + 0x21

typedef struct tagNMHL_SET_TIMER
{
    NMHDR     hdr;              /**< Default WM_NOTIFY header, only code field is used in HTMLite */
    UINT_PTR  timerId;          /**< [in] id of the timer event.*/
    UINT      elapseTime;       /**< [in] elapseTime of the timer event, milliseconds. 
                                          If it is 0 then this timer has to be stoped. */
} NMHL_SET_TIMER, FAR *LPNMHL_SET_TIMER;


/** HLN_SET_CURSOR notification.
 *
 * - HLN_SET_CURSOR
 * 
 **/
#define HLN_SET_CURSOR 0xAFF + 0x22

typedef struct tagNMHL_SET_CURSOR
{
    NMHDR     hdr;              /**< Default WM_NOTIFY header, only code field is used in HTMLite */
    UINT      cursorId;         /**< [in] id of the cursor, .*/

} NMHL_SET_CURSOR, FAR *LPNMHL_SET_CURSOR;

/** Create instance of the engine
 * \return \b HTMLITE, instance handle of the engine.
 **/
EXTERN_C  HTMLITE HLAPI HTMLiteCreateInstance();

/** Destroy instance of the engine
 * \param[in] hLite \b HTMLITE, handle.
 * \return \b HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteDestroyInstance(HTMLITE hLite);

/** Set custom tag value to the instance of the engine.
 * \param[in] hLite \b HTMLITE, handle.
 * \param[in] tag \b LPVOID, any pointer.
 * \return \b HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteSetTag(HTMLITE hLite, LPVOID tag);

/** Get custom tag value from the instance of the engine.
 * \param[in] hLite \b HTMLITE, handle.
 * \param[in] tag \b LPVOID*, pointer to value receiving tag value.
 * \return \b HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteGetTag(HTMLITE hLite, LPVOID *tag);

/** Load HTML from file 
 * \param[in] hLite \b HTMLITE, handle.
 * \param[out] path \b LPCWSTR, path or URL of the html file to load.
 * \return \b HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteLoadHtmlFromFile(HTMLITE hLite, LPCWSTR path);

/** Load HTML from memory buffer 
 * \param[in] hLite \b HTMLITE, handle.
 * \param[in] baseURI \b LPCWSTR, base url.
 * \param[in] dataptr \b LPCBYTE, pointer to the buffer
 * \param[in] datasize \b DWORD, length of the data in the buffer
 * \return \b HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteLoadHtmlFromMemory(HTMLITE hLite,LPCWSTR baseURI, LPCBYTE dataptr, DWORD datasize);

/** Measure loaded HTML
 * \param[in] hLite \b HTMLITE, handle.
 * \param[in] viewWidth \b INT, width of the view area.
 * \param[in] viewHeight \b INT, height of the view area.
 * \return \b HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteMeasure(HTMLITE hLite,
          INT viewWidth,    // width of rendering area in pixels 
          INT viewHeight);  // height of rendering area in pixels

/** Render HTML
 * \param[in] hLite \b HTMLITE, handle.
 * \param[in] hdc \b HDC, device context
 * \param[in] x \b INT, x,y,sx and sy have the same meaning as rcPaint in PAINTSTRUCT
 * \param[in] y \b INT, 
 * \param[in] sx \b INT, 
 * \param[in] sy \b INT, "dirty" rectangle coordinates.
 * \return \b HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteRender(HTMLITE hLite, HDC hdc, 
          INT x,  // x position of area to render in pixels  
          INT y,  // y position of area to render in pixels  
          INT sx, // width of area to render in pixels  
          INT sy); // height of area to render in pixels  


/** Render portion of HTML 
 * \param[in] hLite \b HTMLITE, handle.
 * \param[in] hdc \b HDC, device context
 * \param[in] dst_x \b INT, device pixels, dst_x,dst_y,dst_sx and dst_sy have the same meaning as rcPaint in PAINTSTRUCT
 * \param[in] dst_y \b INT, 
 * \param[in] dst_sx \b INT, 
 * \param[in] dst_sy \b INT, "dirty" rectangle coordinates.
 * \param[in] src_x \b INT, pixels, src_x,src_y,src_sx and src_sy define portion of document to render at dst
 * \param[in] src_y \b INT, 
 * \param[in] src_sx \b INT, Not used at the moment!
 * \param[in] src_sy \b INT, Not used at the moment! 
 * \return \b HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteRenderEx(HTMLITE hLite, HDC hdc, 
          INT dst_x,  // x position of area to render in pixels  
          INT dst_y,  // y position of area to render in pixels  
          INT dst_sx, // width of area to render in pixels  
          INT dst_sy, // height of area to render in pixels  
          INT src_x,  // x position of document area to render
          INT src_y,  // y position of document area to render
          INT src_sx, // width of document area to render
          INT src_sy); // height of document area to render

/** Render HTML on 24bpp or 32bpp dib 
 * \param[in] hLite \b HTMLITE, handle.
 * \param[in] hbmp \b HBITMAP, device context
 * \param[in] x \b INT, 
 * \param[in] y \b INT, 
 * \param[in] sx \b INT, 
 * \param[in] sy \b INT, "dirty" rectangle coordinates.
 * \return \b HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteRenderOnBitmap(HTMLITE hLite, HBITMAP hbmp, 
          INT x,    // x position of area to render  
          INT y,    // y position of area to render  
          INT sx,   // width of area to render  
          INT sy);  // height of area to render  

/** executes all pending changes */
EXTERN_C  BOOL     HLAPI HTMLiteUpdateView( HTMLITE hLite );

/**This function is used in response to HLN_LOAD_DATA request. 
 *
 * \param[in] hLite \b HTMLITE, handle.
 * \param[in] uri \b LPCWSTR, URI of the data requested by HTMLayout.
 * \param[in] data \b LPBYTE, pointer to data buffer.
 * \param[in] dataSize \b DWORD, length of the data in bytes.
 *
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteSetDataReady(HTMLITE hLite, LPCWSTR url, LPCBYTE data, DWORD dataSize);

/**Use this function outside of HLN_LOAD_DATA request. This function is needed when you
 * you have your own http client implemented in your application.
 *
 * \param[in] hLite \b HTMLITE, handle.
 * \param[in] uri \b LPCWSTR, URI of the data requested by HTMLayout.
 * \param[in] data \b LPBYTE, pointer to data buffer.
 * \param[in] dataLength \b DWORD, length of the data in bytes.
 * \param[in] dataType \b UINT, type of resource to load. See HTMLayoutResourceType.
 * \return \b BOOL, TRUE if HTMLayout accepts the data or \c FALSE if error occured 
 **/

EXTERN_C  HLTRESULT HLAPI HTMLiteSetDataReadyAsync(HTMLITE hLite, LPCWSTR uri, LPCBYTE data, DWORD dataSize, UINT type);

/**Get minimum width of loaded document 
 * ATTN: for this method to work document shall have following style:
 *    html { overflow: none; }
 * Otherwize consider to use
 *    HTMLayoutGetScrollInfo( root, ... , LPSIZE contentSize );  
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteGetDocumentMinWidth(HTMLITE hLite, LPINT v);

/**Get minimum height of loaded document 
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteGetDocumentMinHeight(HTMLITE hLite, LPINT v);

/**Set media type for CSS engine, use this before loading the document
 * \See: http://www.w3.org/TR/REC-CSS2/media.html 
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteSetMediaType(HTMLITE hLite, LPCSTR mediatype);

/**Get root DOM element of loaded HTML document. 
 * \param[in] hLite \b HTMLITE, handle.
 * \param[out] phe \b HELEMENT*, address of variable receiving handle of the root element (<html>).
 * \return \b HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteGetRootElement(HTMLITE hLite, HELEMENT* phe);

/** Get Element handle by its UID.
 * \param[in] hwnd \b HTMLITE, handle.
 * \param[in] uid \b UINT
 * \param[out] phe \b #HELEMENT*, variable to receive HELEMENT handle
 * \return \b #HLDOM_RESULT
 *
 * This function retrieves element UID by its handle. Returns NULL if it's not found.
 *
 **/

EXTERN_C HLTRESULT HLAPI HTMLiteGetElementByUID(HTMLITE hLite, UINT uid, HELEMENT* phe);

/** Find DOM element by point (x,y).
 * \param[in] hLite \b HTMLITE, handle.
 * \param[in] x \b INT, x coordinate of the point.
 * \param[in] y \b INT, y coordinate of the point.
 * \param[in] phe \b HELEMENT*, address of variable receiving handle of the element or 0 if there are no such element.
 * \return \b HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteFindElement(HTMLITE hLite, INT x, INT y, HELEMENT* phe);

//|
//| Callback function type
//|
//| HtmLayout will call it for callbacks defined in htmlayout.h, e.g. NMHL_ATTACH_BEHAVIOR
//| 

typedef UINT CALLBACK HTMLITE_CALLBACK(HTMLITE hLite, LPNMHDR hdr);

/** Set callback function. 
 * \param[in] hLite \b HTMLITE, handle.
 * \param[in] cb \b HTMLITE_CALLBACK, address of callback function.
 * \return \b HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteSetCallback(HTMLITE hLite, HTMLITE_CALLBACK* cb);

EXTERN_C  HLTRESULT HLAPI HTMLiteTraverseUIEvent(HTMLITE hLite, UINT evt, LPVOID eventCtlStruct, LPBOOL bOutProcessed );

/** advance focus to focusable element. 
 * \param[in] hLite \b HTMLITE, handle.
 * \param[in] where \b FOCUS_ADVANCE_CMD, where to advance.
 * \param[out] pRes \b BOOL*, TRUE if focus was advanced, FLASE - otherwise.
 * \return \b HLTRESULT.
 **/
enum FOCUS_ADVANCE_CMD 
{
  FOCUS_REMOVE = -2, /**< Remove focus from the view. This for example will stop caret timer if focus was set on &lt;input type=edit &gt; */
  FOCUS_RESTORE = -1,/**< Set focus on the element that had the focus before or to the first focusable element if the view had no focus set before. */
  FOCUS_NEXT = 0,    /**< Advance focus to the next element. If current focus element is the last one in tab order pRes will receive FALSE; */
  FOCUS_PREV = 1,    /**< Advance focus to the previous element. If current focus element is the first one in tab order pRes will receive FALSE; */
  FOCUS_HOME = 2,    /**< Advance focus to the first element in tab order. pRes will always receive TRUE.  */
  FOCUS_END = 3,     /**< Advance focus to the last element in tab order. pRes will always receive TRUE.  */
};
EXTERN_C  HLTRESULT HLAPI HTMLiteAdvanceFocus(HTMLITE hLite, UINT where, BOOL* pRes );

/** Get next (proposed) focus element. 
 * \param[in] hLite \b HTMLITE, handle.
 * \param[in] where \b FOCUS_ADVANCE_CMD, where to advance.
 * \param[in] currentElement \b an element which is a reference point for next focusable element. Could be null if current view focus should be used.
 * \param[out] ppElement \b HELEMENT*, address of variable that will receive handler of found element.
 * \param[out] pEndReached \b BOOL*, address of variable that will receive TRUE if currentElement was the last one in the tab order.
 * \return \b HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteGetNextFocusable(HTMLITE hLite, UINT where, HELEMENT currentElement, HELEMENT* ppElement, BOOL* pEndReached );

/**Get focused DOM element of the document.
 * \param[in] hLite \b HTMLITE, window for which you need to get focus
 * element
 * \param[out ] phe \b #HELEMENT*, variable to receive focus element.
 * \param[out ] pbViewActiveState \b BOOL*, variable to receive state of the focus element: TRUE the view itself is active (a.k.a. current), FALSE otherwise.
 * \return \b #HLTRESULT
 *
 * phe may receive null value (0) if the view had no focus set yet.
 *
 * COMMENT: To set focus on element use HTMLayoutSetElementState(STATE_FOCUS,0)
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteGetFocusElement(HTMLITE hLite, HELEMENT* phe, BOOL* pbViewFocusState);

/**Get HTMLITE handler of containing window.
 * \param[in] he \b #HELEMENT
 * \param[out] pHTMLite \b HTMLITE*, variable to receive view handle
 * \param[in] reserved \b BOOL - reserved
 * \return \b #HLTRESULT.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteGetElementHTMLITE(HELEMENT he, HTMLITE* pHTMLite, BOOL reserved);

/** Attach/Detach ElementEventProc to the htmlite view.
    All events will start first here (in SINKING phase) and if not consumed will end up here.
    You can install EventHandler only once - it will survive all document reloads.
	If you want to change subscription mask, you can call HTMLiteAttachEventHandler again with the
	same "pep" and "tag" params, it will replace the handler subscription mask.
 **/
EXTERN_C  HLTRESULT HLAPI HTMLiteAttachEventHandler(HTMLITE hLite, LPELEMENT_EVENT_PROC pep, LPVOID tag, UINT subscription);
EXTERN_C  HLTRESULT HLAPI HTMLiteDetachEventHandler(HTMLITE hLite, LPELEMENT_EVENT_PROC pep, LPVOID tag);

#ifdef __cplusplus

// C++ wrapper

#include "htmlayout_behavior.hpp"

class  HTMLite
{
protected:
  HTMLITE                hLite;

  static UINT CALLBACK HTMLiteCB(HTMLITE hLite, LPNMHDR hdr)
  {
    HTMLite *pex = 0;
    HTMLiteGetTag(hLite, (void**)&pex);
    assert(pex);

    switch( hdr->code )
    {
      case HLN_LOAD_DATA: return pex->handleLoadUrlData( LPNMHL_LOAD_DATA(hdr) ); 
      case HLN_DATA_LOADED: return pex->handleUrlDataLoaded( LPNMHL_DATA_LOADED(hdr) ); 
      case HLN_ATTACH_BEHAVIOR: return pex->handleAttachBehavior( LPNMHL_ATTACH_BEHAVIOR(hdr) ); 
      case HLN_REFRESH_AREA: pex->handleRefreshArea( LPNMHL_REFRESH_AREA(hdr) ); break;
      case HLN_SET_TIMER: pex->handleSetTimer( LPNMHL_SET_TIMER(hdr) ); break;
      case HLN_SET_CURSOR: pex->handleSetCursor( LPNMHL_SET_CURSOR(hdr) ); break;
      case HLN_UPDATE_UI: pex->handleUpdate(); break;
    }
    return 0;
  }

  virtual bool is_closed_tab_cycle( ) { return false; } // this will cause set_focus_on(html::view::FOCUS_NEXT) to return false if there is 
                                                        // no next element in the tab order.

public:
  HTMLite(const char* mediaType = "screen"):hLite(0) 
  {
    hLite = HTMLiteCreateInstance();
    assert(hLite);
    if(hLite)
    {
      // set tag 
      HTMLiteSetTag(hLite,this);
      //set media type
      HTMLiteSetMediaType(hLite, mediaType);
      // register callback
      HTMLiteSetCallback(hLite, &HTMLiteCB );
    }
  }
  ~HTMLite() { destroy(); }
  
  void destroy() 
  { 
    if(hLite) 
    {
      HTMLiteDestroyInstance(hLite);
      hLite = 0;
    }
  }
  bool load(LPCWSTR path)
  {
    HLTRESULT hr = HTMLiteLoadHtmlFromFile(hLite, path);
    assert(hr == HLT_OK);
    return hr == HLT_OK;
  }

  bool load(LPCBYTE dataptr, DWORD datasize, LPCWSTR baseURI = L"")
  {
    HLTRESULT hr = HTMLiteLoadHtmlFromMemory(hLite, baseURI, dataptr, datasize);
    assert(hr == HLT_OK);
    return hr == HLT_OK;
  }

  bool  measure(  int viewWidth,  // width of rendering area in device (physical) units  
                  int viewHeight) // height of rendering area in device (physical) units  
  {
    HLTRESULT hr = HTMLiteMeasure(hLite, viewWidth, viewHeight);
    //assert(hr == HLT_OK); 
    return hr == HLT_OK;
  }
  
  bool  render(HDC hdc, int x, int y, int width, int height)
  {
    HLTRESULT hr = HTMLiteRender(hLite, hdc, x, y, width, height);
    assert(hr == HLT_OK);
    return hr == HLT_OK;
  }

  bool  render(HDC hdc, int x, int y, int width, int height, int src_x, int src_y)
  {
    HLTRESULT hr = HTMLiteRenderEx(hLite, hdc, x, y, width, height, src_x, src_y, width, height);
    assert(hr == HLT_OK);
    return hr == HLT_OK;
  }

  

  bool  render(HBITMAP hbmp, int x, int y, int width, int height)
  {
    HLTRESULT hr = HTMLiteRenderOnBitmap(hLite, hbmp, x, y, width, height);
    assert(hr == HLT_OK);
    return hr == HLT_OK;
  }

  bool  setDataReady(LPCBYTE data, DWORD dataSize)
  {
    HLTRESULT hr = HTMLiteSetDataReady(hLite, 0, data, dataSize);
    assert(hr == HLT_OK);
    return hr == HLT_OK;
  }

  bool  setDataReadyAsync(LPCWSTR url, LPCBYTE data, DWORD dataSize, UINT dataType)
  {
    HLTRESULT hr = HTMLiteSetDataReadyAsync(hLite, url, data, dataSize,dataType);
    assert(hr == HLT_OK);
    return hr == HLT_OK;
  }
  

  // Get current document measured height for width
  // given in measure scaledWidth/viewportWidth parameters.
  // ATTN: You need call first measure to get valid result.
  // retunrn value is in screen pixels.

  int getDocumentMinHeight()
  {
    int v;
    HLTRESULT hr = HTMLiteGetDocumentMinHeight(hLite, &v);
    assert(hr == HLT_OK); 
    return (hr == HLT_OK)? v:0;
  }

  // Get current document measured minimum (intrinsic) width.
  // ATTN: You need call first measure to get valid result.
  // return value is in screen pixels.

  int getDocumentMinWidth()
  {
    int v;
    HLTRESULT hr = HTMLiteGetDocumentMinWidth(hLite, &v);
    assert(hr == HLT_OK); 
    return (hr == HLT_OK)?v:0;
  }

  HELEMENT getRootElement()
  {
    HELEMENT v;
    HLTRESULT hr = HTMLiteGetRootElement(hLite, &v);
    assert(hr == HLT_OK); 
    return (hr == HLT_OK)?v:0;
  }
 

  // request to load data, override this if you need other data loading policy
  virtual UINT handleLoadUrlData(LPNMHL_LOAD_DATA pn) 
  {
    return 0; // proceed with default image loader
  }

  // data loaded
  virtual UINT handleUrlDataLoaded(LPNMHL_DATA_LOADED pn) 
  {
    return 0; // proceed with default image loader
  }

  // override this if you need other image loading policy
  virtual UINT handleAttachBehavior(LPNMHL_ATTACH_BEHAVIOR pn) 
  {
    htmlayout::event_handler *pb = htmlayout::behavior::find(pn->behaviorName, pn->element);
    if(pb) 
    {
      pn->elementTag = pb;
      pn->elementProc = htmlayout::event_handler::element_proc;
      pn->elementEvents = pb->subscribed_to;
      return TRUE;
    }
    return FALSE; // proceed with default image loader
  }

  virtual void handleRefreshArea( LPNMHL_REFRESH_AREA pn )
  {
    pn->area;//e.g. InvalidateRect(..., pn->area).
  }

  enum HTMLITE_TIMER_ID
  {
    TIMER_IDLE_ID = 1, // nIDEvent in SetTimer cannot be zero
    TIMER_ANIMATION_ID = 2
  };

  virtual void handleSetTimer( LPNMHL_SET_TIMER pn )
  {
    if( pn->elapseTime )
      ;// CreateTimerQueueTimer( this, pn->timerId, pn->elapseTime )
    else
      ;// DeleteTimerQueueTimer( .... )
  }

  virtual void handleSetCursor( LPNMHL_SET_CURSOR pn )
  {
    // (CURSOR_TYPE) pn->cursorId;
  }

  virtual void handleUpdate( )
  {
    //e.g. copy invalid area of pixel buffer to the screen.
  }


  // process mouse event,
  // see MOUSE_PARAMS for the meaning of parameters
  BOOL traverseMouseEvent( UINT mouseCmd, POINT pt, UINT buttons, UINT alt_state ) 
  {
    MOUSE_PARAMS mp; memset(&mp, 0, sizeof(mp));
    mp.alt_state = alt_state;
    mp.button_state = buttons;
    mp.cmd = mouseCmd;
    mp.pos_document = pt;
    BOOL result = FALSE;
    HLDOM_RESULT hr = HTMLiteTraverseUIEvent(hLite, HANDLE_MOUSE, &mp, &result );
    assert(hr == HLDOM_OK); hr;
    return result;
  }

  // process keyboard event,
  // see KEY_PARAMS for the meaning of parameters
  BOOL traverseKeyboardEvent( UINT keyCmd, UINT code, UINT alt_state ) 
  {
    KEY_PARAMS kp; memset(&kp, 0, sizeof(kp));
    kp.alt_state = alt_state;
    kp.key_code = code;
    kp.cmd = keyCmd;
    BOOL result = FALSE;
    HLDOM_RESULT hr = HTMLiteTraverseUIEvent(hLite, HANDLE_KEY, &kp, &result );
    assert(hr == HLDOM_OK); hr;
    return result;
  }

  // process timer event,
  BOOL traverseTimerEvent( UINT timerId ) 
  {
    BOOL result = FALSE;
    HLDOM_RESULT hr = HTMLiteTraverseUIEvent(hLite, HANDLE_TIMER, &timerId, &result );
    assert(hr == HLDOM_OK); hr;
    return result; // host must destroy timer event if result is FALSE.
  }

  

};
#endif //__cplusplus 


#endif
