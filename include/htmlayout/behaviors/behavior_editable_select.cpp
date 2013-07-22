#include "behavior_aux.h"

#pragma warning(disable:4786) //identifier was truncated...

namespace htmlayout 
{

/*

BEHAVIOR: editable-select or editable-pick-list
    goal: Adds editiing functionality to <select>.
TYPICAL USE:
   <div style="behavior:editable-select">
      <widget type="select" multiple="checks"> 
         <option>...</option>
      </widget> 
      <input type="button" action="modify">modify list</input>
   </div>
SAMPLE:
*/

  


struct editable_select: public behavior
{

    // ctor
    editable_select(): behavior(HANDLE_FOCUS | HANDLE_KEY | HANDLE_BEHAVIOR_EVENT, "editable-select") {}

    virtual BOOL on_key   (HELEMENT he, HELEMENT target, UINT event_type, UINT code, UINT keyboardStates ) 
    { 
      dom::element t = target;
      if( !aux::streq(t.get_element_type(),"option") )
        return FALSE;
      if( event_type != KEY_DOWN )
        return FALSE;
      
      switch(code)
      {
        case VK_RETURN:
        case VK_DOWN:
          return select_next_option( t );
        case VK_UP:
          return select_prev_option( t );
      }
      return FALSE; 
    }

    BOOL select_next_option( dom::element& option )
    {
       dom::element next = option.next_sibling();
       if( !next.is_valid() ) 
         goto ADD_NEW;
       if( !aux::streq(next.get_element_type(),"option") )
         goto ADD_NEW;
       next.set_state(STATE_FOCUS); 
       return TRUE;
ADD_NEW:
       std::wstring text = option.text();
       if(text.empty() || text == L" ")
       {
         ::MessageBeep(MB_ICONEXCLAMATION);
         return FALSE;
       }

       dom::element select = option.parent();
       next = dom::element::create("option", L" ");
       select.insert(next,option.index()+1);
       select.update();
       next.set_state(STATE_FOCUS); 
       return TRUE;
    }
    BOOL select_prev_option( dom::element& option )
    {
       dom::element next = option.prev_sibling();
       if( !next.is_valid() ) 
         return FALSE;
       if( !aux::streq(next.get_element_type(),"option") )
         return FALSE;
       next.set_state(STATE_FOCUS); 
       return TRUE;
    }

    virtual BOOL on_event (HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason ) 
    { 
      if( type == BUTTON_CLICK )
      {
        dom::element btn = target;
        const wchar_t* action = btn.get_attribute("action");
        if( !action )
          return FALSE;  // it is not a button we know about

        dom::element el = he; // this is our container-observer we attached to
        dom::element sel = el.find_first("select,[type='select']"); // this is our select element
        assert(sel.is_valid());
        if(!sel.is_valid())
          return FALSE; // wrong HTML structure.

        if( aux::wcseq(action,L"edit-list") )
        {
          el.set_attribute("editable",L""); // set 'editable' attribute
          btn.set_text(L"Save");
          btn.set_attribute("action",L"save-list"); // set different action

          // button "add new"
          const unsigned char add_new[] = "<text><button action='add-new'>Add new item here...</button></text>";
          sel.set_html(add_new,sizeof(add_new)-1,SIH_APPEND_AFTER_LAST);

          el.update();
          return TRUE;
        }
        if( aux::wcseq(action,L"save-list") )
        {
          el.set_attribute("editable", 0); // reset 'editable' attribute
          btn.set_text(L"Modify");
          btn.set_attribute("action",L"edit-list"); // set different action

          // remove "add new" button, it is always in last element 
          dom::element last_el = sel.child( sel.children_count() - 1);
          last_el.destroy();

          el.update();
          return TRUE;
        }
        if( aux::wcseq(action,L"add-new") )
        {
          dom::element nel = dom::element::create("option",L" ");
          sel.insert(nel, sel.children_count() - 1);
          sel.update();
          nel.set_state(STATE_FOCUS);
          return TRUE;
        }

      }

      return false;
    }

    virtual BOOL on_focus  (HELEMENT he, HELEMENT target, UINT event_type )
    { 
      if( event_type != (FOCUS_LOST | SINKING) )
        return FALSE;

      // handling lost focus
      dom::element option = target; // target here is new focus element 
      dom::element select = option.parent();
      option = select.find_first("option:focus"); // get current element in focus

      if( !aux::streq(option.get_element_type(),"option") )
        return FALSE;

      std::wstring text = option.text();
      if(text.empty() || text == L" ")
      {
        option.destroy();
        select.update();
        return TRUE;
      }
      return FALSE; 
    }
   
};

// instantiating and attaching it to the global list
editable_select editable_select_instance;

} // htmlayout namespace

