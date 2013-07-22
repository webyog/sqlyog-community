/*
 * Terra Informatica Lightweight Embeddable HTMLayout control
 * http://terrainformatica.com/htmlayout
 * 
 * Behaviors support (a.k.a windowless controls)
 * 
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 * 
 * 
 * (C) 2003-2006, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

#ifndef __htmlayout_behavior_h__
#define __htmlayout_behavior_h__

/*!\file
\brief Behaiviors support (a.k.a windowless controls)
*/
#include <windows.h>

#pragma pack(push,8)

  /** event groups.
       **/
  enum EVENT_GROUPS 
  {
      HANDLE_INITIALIZATION = 0x0000,     /** attached/detached */
      HANDLE_MOUSE = 0x0001,              /** mouse events */ 
      HANDLE_KEY = 0x0002,                /** key events */  
      HANDLE_FOCUS = 0x0004,              /** focus events, if this flag is set it also means that element it attached to is focusable */ 
      HANDLE_SCROLL = 0x0008,             /** scroll events */ 
      HANDLE_TIMER = 0x0010,              /** timer event */ 
      HANDLE_SIZE = 0x0020,               /** size changed event */ 
      HANDLE_DRAW = 0x0040,               /** drawing request (event) */
      HANDLE_DATA_ARRIVED = 0x080,        /** requested data () has been delivered */
      HANDLE_BEHAVIOR_EVENT = 0x0100,     /** secondary, synthetic events: 
                                            BUTTON_CLICK, HYPERLINK_CLICK, etc., 
                                            a.k.a. notifications from intrinsic behaviors */
      HANDLE_METHOD_CALL = 0x0200,        /** behavior specific methods */

      HANDLE_EXCHANGE   = 0x1000,         /** system drag-n-drop */
      HANDLE_GESTURE    = 0x2000,         /** touch input events */
      
      HANDLE_ALL        = 0xFFFF,         /** all of them */

      DISABLE_INITIALIZATION = 0x80000000 /** disable INITIALIZATION events to be sent. 
                                              normally engine sends
                                              BEHAVIOR_DETACH / BEHAVIOR_ATTACH events unconditionally,
                                              this flag allows to disable this behavior 
                                            */
  };

/**Element callback function for all types of events. Similar to WndProc
 * \param tag \b LPVOID, tag assigned by HTMLayoutAttachElementProc function (like GWL_USERDATA)
 * \param he \b HELEMENT, this element handle (like HWND)
 * \param evtg \b UINT, group identifier of the event, value is one of EVENT_GROUPS
 * \param prms \b LPVOID, pointer to group specific parameters structure.
 * \return TRUE if event was handled, FALSE otherwise.
 **/

//typedef 
//  BOOL CALLBACK ElementEventProc(LPVOID tag, HELEMENT he, UINT evtg, LPVOID prms );


  enum PHASE_MASK 
  {
      BUBBLING = 0,        // bubbling (emersion) phase
      SINKING  = 0x08000,  // capture (immersion) phase, this flag is or'ed with EVENTS codes below
      HANDLED  = 0x10000   // event already processed.
      
    // see: http://www.w3.org/TR/xml-events/Overview.html#s_intro
  };

  enum MOUSE_BUTTONS 
  {
      MAIN_MOUSE_BUTTON = 0x01, //aka left button
      PROP_MOUSE_BUTTON = 0x02, //aka right button
      MIDDLE_MOUSE_BUTTON = 0x04,
      X1_MOUSE_BUTTON = 0x08,
      X2_MOUSE_BUTTON = 0x10,
  };

  enum KEYBOARD_STATES 
  {
      CONTROL_KEY_PRESSED = 0x1,
      SHIFT_KEY_PRESSED = 0x2,
      ALT_KEY_PRESSED = 0x4
  };

// parameters of evtg == HANDLE_INITIALIZATION

  enum INITIALIZATION_EVENTS
  {
    BEHAVIOR_DETACH = 0,
    BEHAVIOR_ATTACH = 1
  };

  struct INITIALIZATION_PARAMS
  {
    UINT cmd; // INITIALIZATION_EVENTS
  };
 
  enum DRAGGING_TYPE
  {
    NO_DRAGGING,
    DRAGGING_MOVE,
    DRAGGING_COPY,
  };

// parameters of evtg == HANDLE_MOUSE

  enum MOUSE_EVENTS
  {
      MOUSE_ENTER = 0,
      MOUSE_LEAVE = 1,
      MOUSE_MOVE  = 2,
      MOUSE_UP    = 3,
      MOUSE_DOWN  = 4,
      MOUSE_DCLICK = 5,
      MOUSE_WHEEL = 6, 
      MOUSE_TICK  = 7, // mouse pressed ticks
      MOUSE_IDLE  = 8, // mouse stay idle for some time

      DROP        = 9,   // item dropped, target is that dropped item 
      DRAG_ENTER  = 0xA, // drag arrived to the target element that is one of current drop targets.  
      DRAG_LEAVE  = 0xB, // drag left one of current drop targets. target is the drop target element.  
      DRAG_REQUEST = 0xC,  // drag src notification before drag start. To cancel - return true from handler.

      MOUSE_CLICK = 0xFF, // mouse click event

      DRAGGING = 0x100, // This flag is 'ORed' with MOUSE_ENTER..MOUSE_DOWN codes if dragging operation is in effect.
                        // E.g. event DRAGGING | MOUSE_MOVE is sent to underlying DOM elements while dragging.
  };

  struct MOUSE_PARAMS
  {
      UINT      cmd;          // MOUSE_EVENTS
      HELEMENT  target;       // target element
      POINT     pos;          // position of cursor, element relative
      POINT     pos_document; // position of cursor, document root relative
      UINT      button_state; // MOUSE_BUTTONS or MOUSE_WHEEL_DELTA
      UINT      alt_state;    // KEYBOARD_STATES 
      UINT      cursor_type;  // CURSOR_TYPE to set, see CURSOR_TYPE
      BOOL      is_on_icon;   // mouse is over icon (foreground-image, foreground-repeat:no-repeat)
      
      HELEMENT  dragging;     // element that is being dragged over, this field is not NULL if (cmd & DRAGGING) != 0
      UINT      dragging_mode;// see DRAGGING_TYPE. 
  };

  enum CURSOR_TYPE
  {
      CURSOR_ARROW, //0
      CURSOR_IBEAM, //1
      CURSOR_WAIT,  //2
      CURSOR_CROSS, //3
      CURSOR_UPARROW,  //4
      CURSOR_SIZENWSE, //5
      CURSOR_SIZENESW, //6
      CURSOR_SIZEWE,   //7
      CURSOR_SIZENS,   //8
      CURSOR_SIZEALL,  //9 
      CURSOR_NO,       //10
      CURSOR_APPSTARTING, //11
      CURSOR_HELP,        //12
      CURSOR_HAND,        //13
      CURSOR_DRAG_MOVE,   //14 
      CURSOR_DRAG_COPY,   //15
  };
  

// parameters of evtg == HANDLE_KEY

  enum KEY_EVENTS
  {
      KEY_DOWN = 0,
      KEY_UP,
      KEY_CHAR
  };

  struct KEY_PARAMS
  {
      UINT      cmd;          // KEY_EVENTS
      HELEMENT  target;       // target element
      UINT      key_code;     // key scan code, or character unicode for KEY_CHAR
      UINT      alt_state;    // KEYBOARD_STATES   
  };

// parameters of evtg == HANDLE_FOCUS

// focus event dispatch details:
//   First: element that has focus before will get FOCUS_LOST (and its parents will get FOCUS_LOST | SINKING)  
//          FOCUS_PARAMS.target in this event points to the new focus candidate element.   
//          FOCUS_PARAMS.cancel at this point can be set to TRUE to canel focus assignment/ 
//          In FOCUS_LOST HTMLayoutGetFocusElement(HWND hwnd, HELEMENT *phe) will return reference to old focus element.  
//
//   Second: system will set internal variable "current_focus" to the new focus. After that 
//          HTMLayoutGetFocusElement(HWND hwnd, HELEMENT *phe) will return new focus element  
//
//   Third: element that is getting focus gets FOCUS_GOT (and its parents will get FOCUS_GOT | SINKING)  
//          FOCUS_PARAMS.target in this event points to the old focus element.   
//          FOCUS_PARAMS.cancel has no effect. 

  enum FOCUS_EVENTS
  {
      FOCUS_LOST = 0,
      FOCUS_GOT = 1,
  };

  enum FOCUS_CAUSE
  {
      BY_CODE,
      BY_MOUSE,
      BY_KEY_NEXT,
      BY_KEY_PREV
  };

  struct FOCUS_PARAMS
  {
      UINT      cmd;            // FOCUS_EVENTS
      HELEMENT  target;         // target element, for FOCUS_LOST it is a handle of new focus element
                                // and for FOCUS_GOT it is a handle of old focus element, can be NULL
      BOOL      by_mouse_click; // TRUE if focus is being set by mouse click
      BOOL      cancel;         // in FOCUS_LOST phase setting this field to TRUE will cancel transfer focus from old element to the new one.
  };

// parameters of evtg == HANDLE_SCROLL

  enum SCROLL_EVENTS
  {
      SCROLL_HOME = 0,
      SCROLL_END,
      SCROLL_STEP_PLUS,
      SCROLL_STEP_MINUS,
      SCROLL_PAGE_PLUS,
      SCROLL_PAGE_MINUS,
      SCROLL_POS,
      SCROLL_SLIDER_RELEASED
  };

  struct SCROLL_PARAMS
  {
      UINT      cmd;          // SCROLL_EVENTS
      HELEMENT  target;       // target element
      INT       pos;          // scroll position if SCROLL_POS
      BOOL      vertical;     // TRUE if from vertical scrollbar
  };

  enum GESTURE_CMD
  {
    GESTURE_REQUEST = 0, // return true and fill flags if it will handle gestures.
    GESTURE_ZOOM,        // The zoom gesture.
    GESTURE_PAN,         // The pan gesture.
    GESTURE_ROTATE,      // The rotation gesture.
    GESTURE_TAP1,        // The tap gesture.
    GESTURE_TAP2,        // The two-finger tap gesture.
  };
  enum GESTURE_STATE 
  {
    GESTURE_STATE_BEGIN   = 1, // starts
    GESTURE_STATE_INERTIA = 2, // events generated by inertia processor
    GESTURE_STATE_END     = 4, // end, last event of the gesture sequence
  };

  enum GESTURE_TYPE_FLAGS // requested 
  {
    GESTURE_FLAG_ZOOM               = 0x0001,
    GESTURE_FLAG_ROTATE             = 0x0002,
    GESTURE_FLAG_PAN_VERTICAL       = 0x0004,
    GESTURE_FLAG_PAN_HORIZONTAL     = 0x0008,
    GESTURE_FLAG_TAP1               = 0x0010, // press & tap
    GESTURE_FLAG_TAP2               = 0x0020, // two fingers tap

    GESTURE_FLAG_PAN_WITH_GUTTER    = 0x4000, // PAN_VERTICAL and PAN_HORIZONTAL modifiers
    GESTURE_FLAG_PAN_WITH_INERTIA   = 0x8000, //
    GESTURE_FLAGS_ALL               = 0xFFFF, //
  };

  struct GESTURE_PARAMS
  {
    UINT      cmd;          // GESTURE_EVENTS
    HELEMENT  target;       // target element
    POINT     pos;          // position of cursor, element relative
    POINT     pos_view;     // position of cursor, view relative
    UINT      flags;        // for GESTURE_REQUEST combination of GESTURE_FLAGs. 
                            // for others it is a combination of GESTURE_STATe's
    UINT      delta_time;   // period of time from previous event.
    SIZE      delta_xy;     // for GESTURE_PAN it is a direction vector 
    double    delta_v;      // for GESTURE_ROTATE - delta angle (radians) 
                            // for GESTURE_ZOOM - zoom value, is less or greater than 1.0    
  };


  enum DRAW_EVENTS
  {
      DRAW_BACKGROUND = 0,
      DRAW_CONTENT = 1,
      DRAW_FOREGROUND = 2,
  };

  // Use ::GetTextColor(hdc) to get current text color of the element while drawing
  struct DRAW_PARAMS
  {
      UINT      cmd;          // DRAW_EVENTS
      HDC       hdc;          // hdc to paint on
      RECT      area;         // element area to paint,  
      UINT      reserved;     //   for DRAW_BACKGROUND/DRAW_FOREGROUND - it is a border box
                              //   for DRAW_CONTENT - it is a content box
  };

  enum EXCHANGE_EVENTS 
  {
    X_DRAG_ENTER,
    X_DRAG_LEAVE,
    X_DRAG,
    X_DROP,
  };

  enum EXCHANGE_DATA_TYPE 
  {
    EXF_UNDEFINED   = 0,
    EXF_TEXT        = 0x01, // FETCH_EXCHANGE_DATA will receive UTF8 encoded string - plain text
    EXF_HTML        = 0x02, // FETCH_EXCHANGE_DATA will receive UTF8 encoded string - html
    EXF_HYPERLINK   = 0x04, // FETCH_EXCHANGE_DATA will receive UTF8 encoded string with pair url\0caption (null separated)
    EXF_JSON        = 0x08, // FETCH_EXCHANGE_DATA will receive UTF8 encoded string with JSON literal
    EXF_FILE        = 0x10, // FETCH_EXCHANGE_DATA will receive UTF8 encoded list of file names separated by nulls
  };
  enum EXCHANGE_COMMANDS
  {
    EXC_NONE = 0,
    EXC_COPY = 1,
    EXC_MOVE = 2,
    EXC_LINK = 4,
  };

  struct EXCHANGE_PARAMS;

  typedef BOOL CALLBACK FETCH_EXCHANGE_DATA(EXCHANGE_PARAMS* params, UINT data_type, LPCBYTE* ppDataStart, UINT* pDataLength );

  struct EXCHANGE_PARAMS
  {
    UINT      cmd;          // EXCHANGE_EVENTS
    HELEMENT  target;       // target element
    POINT     pos;          // position of cursor, element relative
    POINT     pos_view;     // position of cursor, view (window) relative
    UINT      data_types;   // combination of EXCHANGE_DATA_TYPEs above 
    UINT      drag_cmd;     // EXCHANGE_COMMANDS above
    FETCH_EXCHANGE_DATA*
              fetch_data;   // function to fetch the data of desired type.
  };

  enum BEHAVIOR_EVENTS
  {
      BUTTON_CLICK = 0,              // click on button
      BUTTON_PRESS = 1,              // mouse down or key down in button
      BUTTON_STATE_CHANGED = 2,      // checkbox/radio/slider changed its state/value 
      EDIT_VALUE_CHANGING = 3,       // before text change
      EDIT_VALUE_CHANGED = 4,        // after text change
      SELECT_SELECTION_CHANGED = 5,  // selection in <select> changed
      SELECT_STATE_CHANGED = 6,      // node in select expanded/collapsed, heTarget is the node

      POPUP_REQUEST   = 7,           // request to show popup just received, 
                                     //     here DOM of popup element can be modifed.
      POPUP_READY     = 8,           // popup element has been measured and ready to be shown on screen,
                                     //     here you can use functions like ScrollToView.
      POPUP_DISMISSED = 9,           // popup element is closed,
                                     //     here DOM of popup element can be modifed again - e.g. some items can be removed
                                     //     to free memory.

      MENU_ITEM_ACTIVE = 0xA,        // menu item activated by mouse hover or by keyboard,
      MENU_ITEM_CLICK = 0xB,         // menu item click, 
                                     //   BEHAVIOR_EVENT_PARAMS structure layout
                                     //   BEHAVIOR_EVENT_PARAMS.cmd - MENU_ITEM_CLICK/MENU_ITEM_ACTIVE   
                                     //   BEHAVIOR_EVENT_PARAMS.heTarget - the menu item, presumably <li> element
                                     //   BEHAVIOR_EVENT_PARAMS.reason - BY_MOUSE_CLICK | BY_KEY_CLICK

      CONTEXT_MENU_SETUP   = 0xF,    // evt.he is a menu dom element that is about to be shown. You can disable/enable items in it.      
      CONTEXT_MENU_REQUEST = 0x10,   // "right-click", BEHAVIOR_EVENT_PARAMS::he is current popup menu HELEMENT being processed or NULL.
                                     // application can provide its own HELEMENT here (if it is NULL) or modify current menu element.
      
      VISIUAL_STATUS_CHANGED = 0x11, // broadcast notification, sent to all elements of some container being shown or hidden   
      DISABLED_STATUS_CHANGED = 0x12,// broadcast notification, sent to all elements of some container that got new value of :disabled state

      POPUP_DISMISSING = 0x13,       // popup is about to be closed


      // "grey" event codes  - notfications from behaviors from this SDK 
      HYPERLINK_CLICK = 0x80,        // hyperlink click
      TABLE_HEADER_CLICK,            // click on some cell in table header, 
                                     //     target = the cell, 
                                     //     reason = index of the cell (column number, 0..n)
      TABLE_ROW_CLICK,               // click on data row in the table, target is the row
                                     //     target = the row, 
                                     //     reason = index of the row (fixed_rows..n)
      TABLE_ROW_DBL_CLICK,           // mouse dbl click on data row in the table, target is the row
                                     //     target = the row, 
                                     //     reason = index of the row (fixed_rows..n)

      ELEMENT_COLLAPSED = 0x90,      // element was collapsed, so far only behavior:tabs is sending these two to the panels
      ELEMENT_EXPANDED,              // element was expanded,

      ACTIVATE_CHILD,                // activate (select) child, 
                                     // used for example by accesskeys behaviors to send activation request, e.g. tab on behavior:tabs. 

      DO_SWITCH_TAB = ACTIVATE_CHILD,// command to switch tab programmatically, handled by behavior:tabs 
                                     // use it as HTMLayoutPostEvent(tabsElementOrItsChild, DO_SWITCH_TAB, tabElementToShow, 0);

      INIT_DATA_VIEW,                // request to virtual grid to initialize its view
      
      ROWS_DATA_REQUEST,             // request from virtual grid to data source behavior to fill data in the table
                                     // parameters passed throug DATA_ROWS_PARAMS structure.

      UI_STATE_CHANGED,              // ui state changed, observers shall update their visual states.
                                     // is sent for example by behavior:richtext when caret position/selection has changed.

      FORM_SUBMIT,                   // behavior:form detected submission event. BEHAVIOR_EVENT_PARAMS::data field contains data to be posted.
                                     // BEHAVIOR_EVENT_PARAMS::data is of type T_MAP in this case key/value pairs of data that is about 
                                     // to be submitted. You can modify the data or discard submission by returning TRUE from the handler.
      FORM_RESET,                    // behavior:form detected reset event (from button type=reset). BEHAVIOR_EVENT_PARAMS::data field contains data to be reset.
                                     // BEHAVIOR_EVENT_PARAMS::data is of type T_MAP in this case key/value pairs of data that is about 
                                     // to be rest. You can modify the data or discard reset by returning TRUE from the handler.
                                     
      DOCUMENT_COMPLETE,             // behavior:frame have complete document.

      HISTORY_PUSH,                  // behavior:history stuff
      HISTORY_DROP,                     
      HISTORY_PRIOR,
      HISTORY_NEXT,

      HISTORY_STATE_CHANGED,         // behavior:history notification - history stack has changed

      CLOSE_POPUP,                   // close popup request,
      REQUEST_TOOLTIP,               // request tooltip, BEHAVIOR_EVENT_PARAMS.he <- is the tooltip element.

      ANIMATION         = 0xA0,      // animation started (reason=1) or ended(reason=0) on the element.


      FIRST_APPLICATION_EVENT_CODE = 0x100, 
      // all custom event codes shall be greater
      // than this number. All codes below this will be used
      // solely by application - HTMLayout will not intrepret it 
      // and will do just dispatching.
      // To send event notifications with  these codes use
      // HTMLayoutSend/PostEvent API.

      WY_ELEMENT_STATE_SET = FIRST_APPLICATION_EVENT_CODE + 1,
      WY_ELEMENT_STATE_RESET = FIRST_APPLICATION_EVENT_CODE + 2,
      WY_GRID_VERTICAL_SCROLL = FIRST_APPLICATION_EVENT_CODE + 3 
  };

  enum EVENT_REASON
  {
      BY_MOUSE_CLICK = 0,  
      BY_KEY_CLICK = 1, 
      SYNTHESIZED = 2, // synthesized, programmatically generated.
  };

  enum EDIT_CHANGED_REASON
  {
      BY_INS_CHAR = 3,  // single char insertion
      BY_INS_CHARS, // character range insertion, clipboard
      BY_DEL_CHAR,  // single char deletion
      BY_DEL_CHARS, // character range deletion (selection)
  };


  struct BEHAVIOR_EVENT_PARAMS
  {
      UINT     cmd;        // BEHAVIOR_EVENTS
      HELEMENT heTarget;   // target element handler
      HELEMENT he;         // source element e.g. in SELECTION_CHANGED it is new selected <option>, in MENU_ITEM_CLICK it is menu item (LI) element
      UINT     reason;     // EVENT_REASON or EDIT_CHANGED_REASON - UI action causing change.
                           // In case of custom event notifications this may be any 
                           // application specific value.
      JSON_VALUE 
               data;       // auxiliary data accompanied with the event. E.g. FORM_SUBMIT event is using this field to pass collection of values.

  };

  struct TIMER_PARAMS
  {
      UINT_PTR timerId;    // timerId that was used to create timer by using HTMLayoutSetTimerEx
  };


  // identifiers of methods currently supported by intrinsic behaviors,
  // see function HTMLayoutCallMethod 

  enum BEHAVIOR_METHOD_IDENTIFIERS
  {
    DO_CLICK = 0,
    GET_TEXT_VALUE = 1,
    SET_TEXT_VALUE,
      // p - TEXT_VALUE_PARAMS
    
    TEXT_EDIT_GET_SELECTION,
      // p - TEXT_EDIT_SELECTION_PARAMS

    TEXT_EDIT_SET_SELECTION,
      // p - TEXT_EDIT_SELECTION_PARAMS

    // Replace selection content or insert text at current caret position.
    // Replaced text will be selected. 
    TEXT_EDIT_REPLACE_SELECTION, 
      // p - TEXT_EDIT_REPLACE_SELECTION_PARAMS

    // Set value of type="vscrollbar"/"hscrollbar"
    SCROLL_BAR_GET_VALUE,
    SCROLL_BAR_SET_VALUE,

    // get current caret position, it returns rectangle that is relative to origin of the editing element.
    TEXT_EDIT_GET_CARET_POSITION,
      // p - TEXT_CARET_POSITION_PARAMS

    TEXT_EDIT_GET_SELECTION_TEXT, // p - TEXT_SELECTION_PARAMS, OutputStreamProc will receive stream of WCHARs
    TEXT_EDIT_GET_SELECTION_HTML, // p - TEXT_SELECTION_PARAMS, OutputStreamProc will receive stream of BYTEs - utf8 encoded html fragment.
    TEXT_EDIT_CHAR_POS_AT_XY,     // p - TEXT_EDIT_CHAR_POS_AT_XY_PARAMS

    IS_EMPTY      = 0xFC,       // p - IS_EMPTY_PARAMS // set VALUE_PARAMS::is_empty (false/true) reflects :empty state of the element.
    GET_VALUE     = 0xFD,       // p - VALUE_PARAMS 
    SET_VALUE     = 0xFE,       // p - VALUE_PARAMS 

    XCALL                       = 0xFF, // p - XCALL_PARAMS
    FIRST_APPLICATION_METHOD_ID = 0x100 
  };

  struct _METHOD_PARAMS 
  {
    UINT methodID; // see: #BEHAVIOR_METHOD_IDENTIFIERS
  };

  #ifdef __cplusplus 

  // script alike calls
  struct XCALL_PARAMS: METHOD_PARAMS 
  {
    LPCSTR       method_name;
    UINT         argc;
    json::value *argv;
    json::value  retval;
    XCALL_PARAMS(LPCSTR name):method_name(name), argc(0),argv(0) { methodID = XCALL; }
  };

  // GET_VALUE/SET_VALUE methods params
  struct VALUE_PARAMS: METHOD_PARAMS 
  {
    json::value  val;
    VALUE_PARAMS(bool do_set) { methodID = do_set? SET_VALUE : GET_VALUE; }
  };

  // IS_EMPTY method params
  struct IS_EMPTY_PARAMS: METHOD_PARAMS 
  {
    unsigned int is_empty; // !0 - is empty
    IS_EMPTY_PARAMS() { methodID = IS_EMPTY; }
  };


  struct TEXT_VALUE_PARAMS: METHOD_PARAMS
  {
    LPCWSTR text;
    UINT    length;
    TEXT_VALUE_PARAMS(bool to_set = false) { methodID = to_set? SET_TEXT_VALUE: GET_TEXT_VALUE; }
  };

  struct TEXT_EDIT_SELECTION_PARAMS: METHOD_PARAMS
  {
    UINT selection_start;
    UINT selection_end;
    TEXT_EDIT_SELECTION_PARAMS(bool to_set = false) { methodID = to_set? TEXT_EDIT_SET_SELECTION: TEXT_EDIT_GET_SELECTION; }
  };

  struct TEXT_EDIT_REPLACE_SELECTION_PARAMS: METHOD_PARAMS
  {
    LPCWSTR text;
    UINT    text_length;
    TEXT_EDIT_REPLACE_SELECTION_PARAMS() { methodID = TEXT_EDIT_REPLACE_SELECTION; }
  };

  struct TEXT_CARET_POSITION_PARAMS: METHOD_PARAMS
  {
    INT left;
    INT top;
    INT width;
    INT height;

    TEXT_CARET_POSITION_PARAMS() { methodID = TEXT_EDIT_GET_CARET_POSITION; }
  };

  struct TEXT_EDIT_CHAR_POS_AT_XY_PARAMS: METHOD_PARAMS
  {
    INT x,y;         // in
    INT      char_pos;   // out
    HELEMENT he;     // out
    int      he_pos; // out
    TEXT_EDIT_CHAR_POS_AT_XY_PARAMS() { methodID = TEXT_EDIT_CHAR_POS_AT_XY; }
  };


  struct TEXT_SELECTION_PARAMS;
  typedef BOOL CALLBACK OutputStreamProc(TEXT_SELECTION_PARAMS* params, UINT data /* BYTE or WCHAR */ );

  struct TEXT_SELECTION_PARAMS: METHOD_PARAMS
  {
    TEXT_SELECTION_PARAMS(bool html) { methodID = html? TEXT_EDIT_GET_SELECTION_HTML:TEXT_EDIT_GET_SELECTION_TEXT; }
    OutputStreamProc* outs; // output stream
  };

  struct SCROLLBAR_VALUE_PARAMS: METHOD_PARAMS
  {
    INT     value;
    INT     min_value;
    INT     max_value;
    INT     page_value; // page increment
    INT     step_value; // step increment (arrow button click) 
    SCROLLBAR_VALUE_PARAMS(bool to_set = false) { methodID = to_set? SCROLL_BAR_SET_VALUE: SCROLL_BAR_GET_VALUE; }
  };

  #endif

  // see HTMLayoutRequestElementData

  struct DATA_ARRIVED_PARAMS
  {
      HELEMENT  initiator;    // element intiator of HTMLayoutRequestElementData request, 
      LPCBYTE   data;         // data buffer
      UINT      dataSize;     // size of data
      UINT      dataType;     // data type passed "as is" from HTMLayoutRequestElementData
      UINT      status;       // status = 0 (dataSize == 0) - unknown error. 
                              // status = 100..505 - http response status, Note: 200 - OK! 
                              // status > 12000 - wininet error code, see ERROR_INTERNET_*** in wininet.h
      LPCWSTR   uri;          // requested url 
  };


  // request to data source to fill the data.
  // used by virtual-grid and virtual list 
  struct DATA_ROWS_PARAMS
  {
      UINT totalRecords;
      UINT firstRecord; // first visible record - 0..totalRecords

      UINT firstRowIdx; // idx of the first row in the table,
      UINT lastRowIdx;  // idx of the last row in the table. 
                        // content of these rows has to be updated.
  };



#pragma pack(pop)

#endif

