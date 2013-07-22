#include "behavior_aux.h"

#include <commctrl.h> // for tooltip support

namespace htmlayout 
{

/*
BEHAVIOR: popup
    goal: 
          
          
TYPICAL USE CASE:
    
SAMPLE:
*/

bool belongs_to( dom::element parent, dom::element child )
{
  if( !child.is_valid()) 
    return false;
  if( parent == child )
    return true;
  return belongs_to( parent, child.parent() );
}

struct dropdown: public behavior
{
    // ctor
    dropdown(): behavior(HANDLE_MOUSE | HANDLE_BEHAVIOR_EVENT, "dropdown") { }
    
    virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
    {
      dom::element el = he;
     
      switch( event_type )
      {
        case MOUSE_DOWN: 
          {
            dom::element popup_el = el.find_first(".popup,popup"); // either class or element <popup>
            if( belongs_to(popup_el,target) )
              return true;

            if( popup_el.is_valid() && el.is_valid() && !el.get_state(STATE_OWNS_POPUP) )
            {
              //el.set_attribute("popup-shown",L""); // set popup-shown attribute 
                     // to indicate that popup has been shown
                     // this may be used by CSS to indicate UI state
              // set_attribute("popup-shown") above is obsolete as you can use 
              // :popup and :owns-popup pseudo-classes in CSS now.  
              //el.update(true); // render state
              ::HTMLayoutShowPopup(popup_el,el,2); // show it below
              //popup_el.set_state(0,STATE_FOCUS);
              //el.set_attribute("popup-shown",0);   // remove popup-shown attribute 
            }
            return true;
          }
          break;
      }
      return false;
    }
  
};

struct popup: public behavior
{
    // ctor
    popup(): behavior(HANDLE_MOUSE | HANDLE_BEHAVIOR_EVENT | HANDLE_FOCUS, "popup") {}

    /*
    virtual void attached  (HELEMENT he ) 
    { 
      dom::element el = he;
    } 
   
    virtual void detached  (HELEMENT he ) 
    { 
      dom::element el = he;
    } 
    */

    virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
    {
      switch( event_type )
      {
        case MOUSE_DOWN:
          return true;
        case MOUSE_UP | HANDLED:
        case MOUSE_UP:
          // notification could be done by command behavior
          // ... 
          // and close window
          ::HTMLayoutHidePopup(he);
          return true;
      }
      return false; 
    }
 
    virtual BOOL on_event (HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason ) 
    { 
      switch( type )
      {
      case BUTTON_CLICK: 
#pragma warning( suppress:4063 ) // case 'XXX' is not a valid value for switch of enum 'BEHAVIOR_EVENTS'
      case BUTTON_CLICK | HANDLED:
      case HYPERLINK_CLICK: 
#pragma warning( suppress:4063 ) // case 'XXX' is not a valid value for switch of enum 'BEHAVIOR_EVENTS'
      case HYPERLINK_CLICK | HANDLED:
         ::HTMLayoutHidePopup(he);
         break;
      }
      return false;
    }
  
    virtual BOOL on_focus  (HELEMENT he, HELEMENT target, UINT event_type ) 
    { 
      if( (event_type == FOCUS_LOST) || (event_type == (FOCUS_LOST | SINKING)) )
      {
        if( !belongs_to( he, target ) )
          ::HTMLayoutHidePopup(he);
        return TRUE;
      }
      return FALSE;
    }

};


// instantiating and attaching it to the global list

dropdown dropdown_instance;
popup popup_instance;


} // htmlayout namespace
