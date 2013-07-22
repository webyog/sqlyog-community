#include "behavior_aux.h"

namespace htmlayout 
{

/*

BEHAVIOR: richtext-current-objects

Will update ul.richtext-current-objects by the stack of current objects in richtext  
 

TYPICAL USE CASE:
   <richtext style="behavior: ~ richtext-current-objects" objects-list="...selector...">
   </richtext>
   <ul.richtext-current-objects>
     <li>body</li>
     <li>p</li>
     ...
   </ul>
*/

struct current_objects_updater: public event_handler
{
    dom::element objects_bar;
    // ctor
    current_objects_updater(const dom::element& ob): 
        event_handler(HANDLE_INITIALIZATION | HANDLE_BEHAVIOR_EVENT),
        objects_bar(ob) {}

    virtual void detached  (HELEMENT he ) 
    { 
      delete this; // don't need it anymore.
    } 

    virtual BOOL on_event (HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason ) 
    { 
      if( type != UI_STATE_CHANGED ) 
        return FALSE; // we handle only UI_STATE_CHANGED here.
      
      assert(objects_bar.is_valid());

      // call of getCurrentObjects method
      const json::value stack = dom::element(he).xcall("getCurrentObjects"); 
      assert(stack.is_array());

      objects_bar.clear();
      int n = stack.length();
      for( int i = 0; i < n; ++i )
      {
        dom::element li = dom::element::create("li", stack[i].to_string().c_str());
        objects_bar.append(li);
      }
      return FALSE; /*mark it as not handled because some other behaviors in the chain may want to handle it */ 
    }

};

struct richtext_current_objects: public behavior
{
    richtext_current_objects(): behavior(HANDLE_BEHAVIOR_EVENT, "richtext-current-objects") {}

    // this behavior has unique instance for each element it is attached to
    virtual event_handler* attach (HELEMENT he ) 
    { 
      dom::element el = dom::element(he);
      const wchar_t* selector = el.get_attribute("objects-list"); 
      assert(selector); // no @objects-bar
      if(!selector) return 0;
      dom::element root = el.root();
      dom::element objects_list = root.find_first("%S",selector);
      assert(objects_list.is_valid()); // no such element
      if(!objects_list.is_valid()) return 0;
      return new current_objects_updater(objects_list); 
    }
};


// instantiating and attaching it to the global list
richtext_current_objects richtext_current_objects_factory;




} // htmlayout namespace

