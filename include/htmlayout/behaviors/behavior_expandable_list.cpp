#include "behavior_aux.h"

namespace htmlayout 
{

/*

BEHAVIOR: expandable-list
    goal: Expandable list of items normally collapsed (having only caption)
          - current item (only one) is getting expanded state when selected.

BEHAVIOR: collapsible-list
    goal: It is an expandable list but it will allow all items to be collpased

COMMENTS: 
    model is like this:
    <ul ... behavior: expandable-list> 
      <li> <-- expandable item
         caption
         <div> expandable content </div> 
      </li>
      <li>...
      <li>...
    </ul> 

SAMPLE:
   See: samples/animation/thelist.htm
*/

struct expandable_list: public behavior
{
    // ctor
    expandable_list(const char* name="expandable-list"): behavior(HANDLE_MOUSE | HANDLE_KEY | HANDLE_FOCUS | HANDLE_BEHAVIOR_EVENT, name ) {}
    
    virtual void attached  (HELEMENT he ) 
    { 
      dom::element ctl = he;
      bool got_one = false;
      for( int i = ctl.children_count() - 1; i >= 0 ; --i)
      {
        dom::element t = ctl.child((unsigned int)i);
        if( t.get_attribute("default") && !got_one)
        {
          t.set_state(STATE_CURRENT | STATE_EXPANDED,0,false); // set state flags
          got_one = true;
        }
        else
          t.set_state(STATE_COLLAPSED,0,false); // set state flags
      }
    } 

    /*
    virtual BOOL on_focus  (HELEMENT he, HELEMENT target, UINT event_type )
    { 
      if( event_type != FOCUS_LOST && event_type != FOCUS_LOST )
        return FALSE;

      // highly probable that 
      dom::element current = select.find_first(":current"); // get current element 

      if( !streq(option.get_element_type(),"option") )
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
    */
    
    // set current item
    virtual void set_current_item( const dom::element& ctl, dom::element& item )
    {
      // get previously selected item:
      dom::element prev_current = ctl.find_first(":root > :current");
      dom::element prev = ctl.find_first(":root > :expanded");
    
      if(prev_current != item && prev_current.is_valid())
        prev_current.set_state(0, STATE_CURRENT);

      if( prev.is_valid() )
      {
        if( prev == item ) return; // already here, nothing to do.
        prev.set_state(0,STATE_CURRENT | STATE_EXPANDED); // drop state flags
        // notify all parties involved
        prev.post_event(ELEMENT_COLLAPSED,0, prev); // source here is old collapsed tab itself
      }
      item.set_state(STATE_CURRENT | STATE_EXPANDED); // set state flags
      item.post_event(ELEMENT_EXPANDED,0, item);  // source here is new expanded tab itself

    }

    virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
    {
      if( event_type != MOUSE_DOWN && event_type != MOUSE_DCLICK )
        return false;

      if(mouseButtons != MAIN_MOUSE_BUTTON) 
        return false;

      // el is presumably <li>;
      dom::element ctl = he;
      dom::element item = target_item(ctl, target);

      if(item.is_valid()) // click on the item caption
        set_current_item(ctl, item);

      return true; // as it is always ours then stop event bubbling
    }

    virtual BOOL on_event (HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason ) 
    { 
      if( type == ACTIVATE_CHILD )
      {
        dom::element item = target_item(dom::element(he), target);
        if(item.is_valid()) // click on the item caption
        {
          set_current_item(he, item);
          return TRUE; 
        }
      }
      return FALSE;
    }

    virtual BOOL on_key(HELEMENT he, HELEMENT target, UINT event_type, UINT code, UINT keyboardStates ) 
    { 
      if( event_type == KEY_DOWN )
      {
        dom::element ctl = he;
        switch( code )
        {
          case VK_DOWN: 
            {
               dom::element c = ctl.find_first(":current");
               int idx = c.is_valid()? (c.index() + 1):0;
               if( idx < (int)ctl.children_count() )
               {
                   dom::element nc = ctl.child(idx);
                   set_current_item(ctl, nc); 
               }
            }
            return TRUE;
          case VK_UP:             
            {
               dom::element c = ctl.find_first(":current");
               int idx = c.is_valid()? (c.index() - 1):(ctl.children_count() - 1);
               if( idx >= 0 )
               {
                   dom::element nc = ctl.child(idx);
                   set_current_item(ctl, nc); 
               }
            }
            return TRUE;
        }
        
      }
      return FALSE; 
    }
  
    dom::element target_item(const dom::element& ctl, dom::element target)
    {
      if( target == ctl )
        return dom::element();

      if( !target.is_valid() )
        return dom::element();

      dom::element target_parent = target.parent();
      if( !target_parent.is_valid() )
        return dom::element();
      
      if( target.test("li > .caption") )
        return target_parent; // only if click on "caption" element of <li>. Returns that <li> element.

      return target_item( ctl, target.parent() );
    }

};

struct collapsible_list: public expandable_list
{
    // ctor
    collapsible_list(): expandable_list("collapsible-list") {}

   // set current item
    virtual void set_current_item( const dom::element& ctl, dom::element& item )
    {
      // get previously expanded item:
      dom::element prev = ctl.find_first(":root > :expanded");
      dom::element prev_current = ctl.find_first(":root > :current");
    
      if(prev_current != item && prev_current.is_valid())
        prev_current.set_state(0, STATE_CURRENT);

      if( prev == item )
      {
        prev.set_state(0, STATE_EXPANDED); 
        prev.post_event(ELEMENT_COLLAPSED,0, prev); // source here is old collapsed tab itself
      }
      else 
      {
        if( prev.is_valid() )
        {
          prev.set_state(0, STATE_EXPANDED); // collapse old one
          prev.post_event(ELEMENT_COLLAPSED,0, prev); // source here is old collapsed tab itself
        }
        item.set_state(STATE_CURRENT | STATE_EXPANDED); // set new expanded.
        item.post_event(ELEMENT_EXPANDED,0, item);  // source here is new expanded tab itself
      }
      item.scroll_to_view();
    }
};

// instantiating and attaching it to the global list
expandable_list expandable_list_instance;
collapsible_list collapsible_list_instance;
}

