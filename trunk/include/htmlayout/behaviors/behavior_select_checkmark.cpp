#include "behavior_aux.h"

namespace htmlayout 
{

/*

BEHAVIOR: select-checkmarked
    goal: add-in for behavior:select, 
          implements chekmarked <option>'s 
COMMENTS: 
    Define it as in CSS as 
    #my-tree-view { behavior: select select-checkmarked; }

SAMPLE:
    See:
    http://www.terrainformatica.com/codelib/view.php?sid=5
    for HTML and CSS styles.
*/

struct select_checkmark: public behavior
{
    // ctor
    select_checkmark(): behavior(HANDLE_MOUSE | HANDLE_KEY | HANDLE_FOCUS, "select-checkmark") {}

#define CHECK_ATTR "check"

    virtual void attached  (HELEMENT he ) 
    {
      dom::element el = he;
      init_options(el);
      el.update();
    }

    enum NODE_STATE { NODE_OFF = 0, NODE_ON = 1, NODE_MIXED = 2 };
    NODE_STATE get_state(dom::element item)
    {
        if(aux::wcseq(item.get_attribute("check"),L"on"))
          return NODE_ON;
        else if(aux::wcseq(item.get_attribute("check"),L"mixed"))
          return NODE_MIXED;
        else 
          return NODE_OFF;
    }
    void set_state(dom::element item, NODE_STATE st)
    {
        switch( st )
        {
        case NODE_ON: item.set_attribute("check",L"on"); break;
        case NODE_MIXED: item.set_attribute("check",L"mixed"); break;
        case NODE_OFF: item.set_attribute("check",L"off"); break;
        }
    }

    NODE_STATE init_options(dom::element n)
    {
      //NODE_STATE n_state = NODE_MIXED;
      int n_off = 0;
      int n_on = 0;
      int n_total = 0; 
      for(int i = 0; i < int(n.children_count()); ++i)
      {
        dom::element t = n.child(i);
        NODE_STATE t_state;
        if( aux::streq(t.get_element_type(),"options") )
          t_state = init_options(t);
        else if( aux::streq(t.get_element_type(),"option") )
          t_state = get_state(t);
        else
          continue;
        
        set_state(t, t_state);
 
        switch( t_state )      
        {
          case NODE_OFF: ++n_off; break;
          case NODE_ON: ++n_on; break;
        }
        ++n_total; 
                 
      }
      if( n_off == n_total ) return NODE_OFF;
      if( n_on == n_total ) return NODE_ON;

      return NODE_MIXED;

    }

    virtual BOOL handle_mouse  (HELEMENT he, MOUSE_PARAMS& params ) 
    {
      if( params.cmd != MOUSE_DOWN && params.cmd != (MOUSE_DOWN | HANDLED))
        return false;

#ifdef _DEBUG
      if(params.is_on_icon)
        params.is_on_icon = params.is_on_icon;
#endif

      if( (params.button_state != MAIN_MOUSE_BUTTON) || !params.is_on_icon ) // left mouse button on caption icon
        return false; 
                  
      dom::element item = params.target;
      
      if(!item.is_valid()) // click on item
        return false;

      if(!aux::streq(item.get_element_type(),"caption"))
        return false;

      // ok, we've got a click on checkmark icon of <caption>

      toggle_checkmark( he, item.parent() );
      //::UpdateWindow(item.get_element_hwnd(false));

      return true; // as it is always ours then stop event bubbling
    }

    virtual BOOL handle_key    (HELEMENT he, KEY_PARAMS& params ) 
    { 
      if( params.cmd != KEY_DOWN || params.key_code != VK_SPACE)
        return false;
      
      // ok, here we have spacebar down
      dom::element select = he;

      dom::element item = select.find_first(":current");
      if( item.is_valid() )
      {
        toggle_checkmark( select, item );
        return TRUE;
      }
      return FALSE; 
    }

    
    //toggles checkmark
    void toggle_checkmark( dom::element select, dom::element item )
    {

      const wchar_t* _old_state = item.get_attribute(CHECK_ATTR);
      std::wstring  old_state = _old_state?_old_state:L"";

      if( aux::streq(item.get_element_type(), "options"))
      {
        // non-terminal node case 
        
        // inner function
        struct child_toggler: dom::callback
        {
          const wchar_t* state;
          child_toggler(const wchar_t* st): state(st) {}
          inline bool on_element(HELEMENT he) 
          { 
            htmlayout::dom::element item = he; 
            item.set_attribute( CHECK_ATTR, state );
            return false;
          }
        };
        
        const wchar_t* new_state;
        NODE_STATE old_state = get_state(item);
        if(old_state == NODE_OFF)
          new_state = L"on";
        else 
          new_state = L"off";

        child_toggler ct( new_state );
        // do it for all children
        item.find_all(&ct, "option,options");
        // and for itself
        item.set_attribute( CHECK_ATTR, new_state );
        //item.update(RESET_STYLE_DEEP);
      }
      else if( aux::streq(item.get_element_type(), "option"))
      {
        // terminal node
        const wchar_t* new_state;
        if(aux::wcseq(item.get_attribute("check"),L"on"))
        {
          new_state = L"off";
        }
        else 
          new_state = L"on";

        item.set_attribute( CHECK_ATTR, new_state );
        //item.update(RESET_STYLE_THIS);
      }
      
      // as state was changed we need to update parent chain here too.
      dom::element p = item.parent();
      while( p.is_valid() && p != select )
      {
        if( aux::streq(p.get_element_type(),"options" ))
        {
          setup_node(p);
        }
        p = p.parent();
      }

    }

    void setup_node( dom::element node )
    {
      dom::element on = node.find_first("option[check='on'],options[check='on']");
      dom::element off = node.find_first("option[check='off'],options[check='off']");
      dom::element mixed = node.find_first("options[check='mixed']");

      const wchar_t* _prev = node.get_attribute(CHECK_ATTR);
      std::wstring  prev = _prev?_prev:L"";
      
      if( mixed.is_valid() || (on.is_valid() && off.is_valid()))
      {
        node.set_attribute(CHECK_ATTR,L"mixed");
        //if(prev != L"mixed") node.update(RESET_STYLE_THIS);
      }
      else if( on.is_valid() )
      {
        node.set_attribute(CHECK_ATTR,L"on");
        //if(prev != L"on") node.update(RESET_STYLE_THIS);
      }
      else
      {
        node.set_attribute(CHECK_ATTR,L"off");
        //if(prev != L"off") node.update(RESET_STYLE_THIS);
      }
    }

};

// instantiating and attaching it to the global list
select_checkmark select_checkmark_instance;
}


