#include "behavior_aux.h"

namespace htmlayout 
{

/*

BEHAVIOR: collapsible-by-icon
    goal: Implementation of collapsible elements, trees, panels, etc.
COMMENTS: 
   Structure of collapsible tree is pretty much similar to the new proposed <nl> element.
   See: "http://www.w3.org/TR/xhtml2/mod-list.html#sec_11.2."
SAMPLE:
   See: samples/behaviors/treeview.htm

*/

struct collapsible_by_icon: public behavior
{
    // ctor
    collapsible_by_icon(): behavior(HANDLE_MOUSE, "collapsible-by-icon") {}

    virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
    {
      if( event_type != MOUSE_DOWN && event_type != MOUSE_DCLICK)
        return false;

      // el is presumably <li>;
      dom::element el = he;
      dom::element target_el = target;
      dom::element first_icon_el = el.find_first(".icon" /*css selector, sic!*/); 
                            // find first element with class "icon"

      if(first_icon_el != target_el)
         return true; // event is not targeted to element having class "icon"

      // ok, we've got here MOUSE_DOWN targeted to icon.

      // get open/closed state.
      bool is_closed = false;
      const wchar_t* pv = el.get_attribute("state");
      if(pv)
        is_closed = wcsicmp(pv,L"close") == 0; 

      // toggle value of attribute "state" and 
      // correspondent state flag - this is needed to play animation
      if(is_closed)
        el.set_attribute("state",L"open");
      else
        el.set_attribute("state",L"close");
        
      return true; // as it is ours then stop event bubbling
    }
   
};

// instantiating and attaching it to the global list
collapsible_by_icon collapsible_by_icon_instance;

}






