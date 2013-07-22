#include "behavior_aux.h"
#include "htmlayout_dom.hpp"

namespace htmlayout 
{

#define HAS_EVENT_ROOT // undefine this if to use not on latest SDK

struct lb_dialog_instance: public event_handler //behavior
{
    HELEMENT      saved_parent;		//saved in show_dialog()
    unsigned int  saved_index;		//and used by hide_dialog()
    unsigned int  focus_uid;      //saved uid of element in focus


    // ctor
    lb_dialog_instance(): event_handler(HANDLE_KEY | HANDLE_FOCUS | HANDLE_BEHAVIOR_EVENT | HANDLE_METHOD_CALL) 
    {
 		  saved_parent = 0;
		  saved_index = 0;
      focus_uid = 0;
    }
	
	  void show_dialog( HELEMENT he )
    {
      if( saved_parent )
        return; // already shown

		  dom::element self = he;								//parent
      saved_parent = self.parent();
      saved_index  = self.index();
      dom::element root = self.root();					//root <html> element
      
      // saving focus
		  dom::element focus = root.find_first(":focus");
      if(focus.is_valid())
        focus_uid = focus.get_element_uid();
      
      dom::element shim = dom::element::create("div");	//create shim
      shim.set_attribute("class", L"shim");
      root.append(shim);									//adding shim to DOM

      shim.insert(self, 0);								            //make dialog a child of the shim
      self.set_style_attribute("display", L"block");	//make it visible

#if defined(HAS_EVENT_ROOT)
        self.set_event_root(); // set this as an event root.
#else
		    dom::element body = root.find_first("body");
                     body.set_state(STATE_DISABLED); // disable body.
#endif
    }

	  void hide_dialog( HELEMENT he )
    {
      if( !saved_parent ) 
        return; // already hidden
      
		  dom::element self = he;
		  dom::element(saved_parent).insert(self,saved_index);  //move it back to original position in the DOM

      dom::element root = self.root();					            //root <html> element
		  dom::element shim = root.find_first("div.shim");	    //get shim
    	  
                   shim.detach();										        //detaching shim from DOM
                   self.set_style_attribute("display", 0);				//clearing display set in show_dialog()

#if defined(HAS_EVENT_ROOT)
        self.reset_event_root(); // reset event root.
#else
        dom::element body = root.find_first("body");
                     body.set_state(0, STATE_DISABLED);					//enable it again
#endif

      dom::element focus = dom::element::element_by_uid( root.get_element_hwnd(true), focus_uid );
                   focus.set_state(STATE_FOCUS);

      saved_parent = 0;
      saved_index = 0;
    }

	// handler of custom functions for the CSSS! engine
    virtual BOOL on_script_call(HELEMENT he, LPCSTR name, UINT argc, json::value* argv, json::value& retval) 
    { 
		  if( aux::streq(name, "show") )
        {
          show_dialog(he);
          return TRUE;
        }
		  else if( aux::streq(name, "hide") )
        {
		      hide_dialog(he);
          return TRUE;
        }
        return FALSE; 
    }      

    virtual void attached  (HELEMENT he ) 
    { 
		  saved_parent = 0;
		  saved_index = 0;
    } 

    virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
    {
      return FALSE; 
    }

    virtual BOOL on_key(HELEMENT he, HELEMENT target, UINT event_type, UINT code, UINT keyboardStates ) 
    { 
      if(event_type != KEY_DOWN)
        return FALSE;

      dom::element self = he;

      switch(code)
      {
         case VK_RETURN:
           {
             dom::element def = self.find_first("[role='ok-button']");
             if( def.is_valid() )
             {
               METHOD_PARAMS params; params.methodID = DO_CLICK;
               return def.call_behavior_method(&params)? TRUE:FALSE;
             }
             
           } break;
         case VK_ESCAPE:
           {
             dom::element def = self.find_first("[role='cancel-button']");
             if( def.is_valid() )
             {
               METHOD_PARAMS params; params.methodID = DO_CLICK;
               return def.call_behavior_method(&params)? TRUE:FALSE;
             }
           } break;
      }
      return FALSE;
    }

    virtual BOOL on_event (HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason ) 
    { 
      if(type != BUTTON_CLICK)
        return false;

      dom::element button = target;
      if( button.test("[role='ok-button']") || button.test("[role='cancel-button']") )
        hide_dialog(he);
		  
      return FALSE;
    }
  
};

struct lb_dialog: public behavior
{
  lb_dialog(): behavior(HANDLE_BEHAVIOR_EVENT, "light-box-dialog") { }

    // this behavior has unique instance for each element it is attached to
    virtual event_handler* attach (HELEMENT /*he*/ ) 
    { 
      return new lb_dialog_instance(); 
    }
};

// instantiating and attaching it to the global list
lb_dialog lb_dialog_instance;
}
