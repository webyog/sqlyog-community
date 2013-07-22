#include "behavior_aux.h"

namespace htmlayout 
{

/*

BEHAVIOR: tabs
   goal: Implementation of the tabs: stacked panels switchable by tabs in tab strip
COMMENTS: 
   <div style="behavior:tabs">
      <ul> <!-- our tab strip, can be placed on any side of tab container. -->
         <li panel="panel-id1" selected >tab1 caption</li>
         <li panel="panel-id2">tab2 caption</li>
      </ul>
      <div name="panel-id1" > first panel content </div>
      <div name="panel-id2"> second panel content </div>
   </div>
SAMPLE:
   See: samples/behaviors/tabs.htm
*/

struct tabs: public behavior
{
    // ctor
    tabs(): behavior(HANDLE_MOUSE | HANDLE_KEY | HANDLE_FOCUS | HANDLE_BEHAVIOR_EVENT | HANDLE_METHOD_CALL , "tabs") {}

    virtual void attached  (HELEMENT he ) 
    { 
      dom::element tabs_el = he;              //:root below matches the element we use to start lookup from.
      dom::element tab_el = tabs_el.find_first(":root>.strip>[panel][selected]"); // initialy selected

      const wchar_t* pname = tab_el.get_attribute("panel");
      // find panel we need to show by default 
      dom::element panel_el = tabs_el.find_first(":root>[name=\"%S\"]", pname);
      if( !panel_el.is_valid())
      {
        assert(false); // what a ...!, panel="somename" without matching name="somename"
        return;
      }

      dom::element tab_strip_el = tab_el.parent();
      tab_strip_el.set_state(STATE_CURRENT,0,true); // :current - make tab strip as current element inside focusable.
      tab_el.set_state(STATE_CURRENT,0,true); // :current - current tab is, well, current.
      panel_el.set_state(STATE_EXPANDED,0,true); // :expanded - current panel is expanded.

    } 

    virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
    {
      if( event_type != MOUSE_DOWN && event_type != MOUSE_DCLICK /*&& event_type != DROP*/)
        return false;

      dom::element tabs_el = he; // our tabs container
      dom::element tab_el = target_tab( target, he ); 

      if(!tab_el.is_valid())
        return false;

      //el here is a <element panel="panel-id1">tab1 caption</element>
      //and we've got here MOUSE_DOWN somewhere on it.

      return select_tab( tabs_el, tab_el );
    }

    static inline bool is_in_focus(const dom::element& el)
    {
      return el.test(":focus") || el.find_nearest_parent(":focus");
    }

    virtual BOOL on_key(HELEMENT he, HELEMENT target, UINT event_type, UINT code, UINT keyboardStates ) 
    { 
      if(event_type != KEY_DOWN)
        return FALSE; // we are handling only KEY_DOWN here

      dom::element tabs_el = he; // our tabs container
      dom::element tab_el = tabs_el.find_first(":root>.strip>[panel]:current"); // currently selected

      switch( code )
      {
        case VK_TAB: if( keyboardStates & CONTROL_KEY_PRESSED )
                          return select_tab( tabs_el, tab_el, 
                                 keyboardStates & SHIFT_KEY_PRESSED? -1:1 );
                     break;
        case VK_LEFT: return is_in_focus(tab_el)? select_tab( tabs_el, tab_el, -1 ):FALSE;
        case VK_RIGHT: return is_in_focus(tab_el)? select_tab( tabs_el, tab_el, 1 ):FALSE;
        case VK_HOME: return is_in_focus(tab_el)? select_tab( tabs_el, tab_el, -2 ):FALSE;
        case VK_END: return is_in_focus(tab_el)? select_tab( tabs_el, tab_el, 2 ):FALSE;
      }
      return FALSE; 
    }

    virtual BOOL on_event (HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason ) 
    { 
      if( type == ACTIVATE_CHILD )
      {
        dom::element newtab = target_tab(target, he);
        if(!newtab.is_valid())
        {
          assert(false); // target is not a tab here.
          return TRUE; 
        }
        dom::element tabs = he;
        select_tab(tabs,newtab);
        return TRUE; 
      }
      return FALSE;
    }
    
    // See: http://rsdn.ru/forum/htmlayout/4286005.1.aspx
    virtual BOOL on_script_call(HELEMENT he, LPCSTR name, UINT argc, json::value* argv, json::value& /*retval*/)
    {
        if(aux::streq(name, "select")) // tabs.select("panel-name");
        {
            if(argc != 1)
            {
                assert(!"tabs: select() - argument required");
                return FALSE;
            }
            json::string panelName = argv[0].get(L"");

            dom::element tabs_el = he; // our tabs container
            dom::element tab_el = tabs_el.find_first(L":root>.strip>[panel=%s]", panelName.c_str());

            select_tab(tabs_el, tab_el);

            return TRUE;
        }
        return FALSE;
    }

    // select 
    bool select_tab( dom::element& tabs_el, dom::element& tab_el )
    {
      if(tab_el.get_state(STATE_CURRENT))
        // already selected, nothing to do...
        return true; // but we've handled it.
     
      //find currently selected element (tab and panel) and remove "selected" from them
      dom::element prev_panel_el = tabs_el.find_first(":root>[name]:expanded");
      dom::element prev_tab_el = tabs_el.find_first(":root>.strip>[panel]:current");

      // find new tab and panel       
      const wchar_t* pname = tab_el.get_attribute("panel");
      dom::element panel_el = tabs_el.find_first(":root>[name=\"%S\"]", pname);
      
      if( !panel_el.is_valid() || !tab_el.is_valid() )
      {
        assert(false); // panel="somename" without matching name="somename"
        return true;
      }

      if( prev_panel_el.is_valid() )
      {
        prev_panel_el.set_attribute("selected", 0); // remove selected attribute - just in case somone is using attribute selectors    
        prev_panel_el.set_state(STATE_COLLAPSED,0); // set collapsed in case of someone use it for styling
      }
      if( prev_tab_el.is_valid() )
      {
        prev_tab_el.set_attribute("selected", 0); // remove selected attribute
        prev_tab_el.set_state(0,STATE_CURRENT); // reset also state flag, :current
      }
      
      panel_el.set_attribute("selected", L""); // set selected attribute (empty)
      panel_el.set_state(STATE_EXPANDED,0); // expand it
      
      tab_el.set_attribute("selected", L""); // set selected attribute (empty)
      tab_el.set_state(STATE_CURRENT,0); // set also state flag, :current
                  
      // notify all parties involved
      prev_tab_el.post_event(ELEMENT_COLLAPSED,0, prev_tab_el); // source here is old collapsed tab itself
      tab_el.post_event(ELEMENT_EXPANDED,0, tab_el);  // source here is new expanded tab itself
      // NOTE #1: these event will bubble from panel elements up to the root so panel itself, tabs ctl, its parent, etc.
      // will receive these notifications. Handle them if you need to change UI dependent from current tab. 
      // NOTE #2: while handling this event in:
      //        virtual BOOL on_event (HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT reason ),
      // HELEMENT target is the panel element being collapsed/expanded
      
      return true;
    }

    // select next/prev/first/last tab
    bool select_tab( dom::element& tabs_el, dom::element& tab_el, int direction )
    {
      // find new tab 
      dom::element new_tab_el(0);
      switch( direction )
      {
        case -2: new_tab_el = tabs_el.first_sibling(); 
                 while( new_tab_el.is_valid() )
                 {
                   if( !new_tab_el.get_state(STATE_DISABLED) )
                     break;
                   new_tab_el = new_tab_el.next_sibling();
                 }
                 break;
        case -1: new_tab_el = tab_el.prev_sibling(); 
                 while( new_tab_el.is_valid() )
                 {
                   if( !new_tab_el.get_state(STATE_DISABLED) )
                     break;
                   new_tab_el = new_tab_el.prev_sibling();
                 }
                 break;
        case +1: new_tab_el = tab_el.next_sibling(); 
                 while( new_tab_el.is_valid() )
                 {
                   if( !new_tab_el.get_state(STATE_DISABLED) )
                     break;
                   new_tab_el = new_tab_el.next_sibling();
                 }
                 break;
        case +2: new_tab_el = tab_el.last_sibling(); 
                 while( new_tab_el.is_valid() )
                 {
                   if( !new_tab_el.get_state(STATE_DISABLED) )
                     break;
                   new_tab_el = new_tab_el.prev_sibling();
                 }
                 break;
        default: assert(false); return false;
      }

      if( !new_tab_el.is_valid() || new_tab_el.get_attribute("panel") == 0 )  //is not a tab element
        return FALSE;

      return select_tab( tabs_el, new_tab_el );
    }

    dom::element target_tab(HELEMENT he, HELEMENT h_tabs_container)
    {
      if( !he || (he == h_tabs_container) ) 
        return 0;

      dom::element el = he;
      const wchar_t* panel_name = el.get_attribute("panel");
      if(panel_name)
        return el; // here we are!

      return target_tab( el.parent(), h_tabs_container );
    }

   
};

// instantiating and attaching it to the global list
tabs tabs_instance;
}
