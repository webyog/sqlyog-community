#include "behavior_aux.h"

namespace htmlayout 
{
 
/*

BEHAVIOR: accesskeys
    goal: support of accesskey html attribute
COMMENTS: 

SAMPLE:
   See: html_samples/behaviors/accesskeys.htm

*/


struct accesskeys: public behavior
{
    // ctor
    accesskeys(): behavior(HANDLE_KEY, "accesskeys") {}
    
    virtual BOOL on_key(HELEMENT he, HELEMENT target, UINT event_type, UINT code, UINT keyboardStates ) 
    { 
        const char* keyname = 0;
        if( event_type == (KEY_DOWN | SINKING) && (keyboardStates & ALT_KEY_PRESSED) == 0  )
        {
          keyname = get_key_name( code, keyboardStates);
          if( !keyname )
            return FALSE;
        }
        else if( (event_type == (KEY_CHAR | SINKING)) && (keyboardStates == ALT_KEY_PRESSED) ) 
        {
          if( code != '\'' && code != '\"' )
          {
            static char name[2];
            name[0] = (char) code;
            name[1] = 0;
            keyname = name;
          }
          else
            return false;
        }
        else 
          return false;

        dom::element container = he;

        HWND thisHWnd = container.get_element_hwnd(false);
        
        // handling IFrame in focus situation
        if( ::GetFocus() == thisHWnd) 
        {
          dom::element super_container = dom::element::root_element( ::GetParent(thisHWnd)  );
          if( super_container.is_valid() ) // yes, we have outer frame
          {
            if(process_key(super_container, keyname))
                return TRUE;
          }
        }

        // normal handling

        if(process_key(container, keyname))
            return TRUE;

        // child iframes handling (if any)
        struct CB:public htmlayout::dom::callback 
        {
          const char* keyname;
          bool done;
          virtual bool on_element(HELEMENT he) 
          {
            htmlayout::dom::element iframe = he; 
            if( iframe.enabled() && iframe.visible() ) // only if it is visible and enabled
            {
              HWND hwndIFrame = iframe.get_element_hwnd(false);
              htmlayout::dom::element iframeRoot = htmlayout::dom::element::root_element(hwndIFrame);
              if(accesskeys::process_key( iframeRoot, keyname ))
              {
                done = true;
                return true; // stop enumeration
              }
            }
            return false;
          }
        };
        CB cb;
        cb.done = false;        
        cb.keyname = keyname;
        container.find_all(&cb, "iframe");
        return cb.done;
       
    }

    static BOOL process_key( dom::element& container, const char* keyname )
    {
        // find all callback 
        struct:public htmlayout::dom::callback 
        {
          htmlayout::dom::element hot_key_element;
          virtual bool on_element(HELEMENT he) 
          {
            htmlayout::dom::element t = he;
            if( !t.enabled() )
              return false;
            if( t.test("menu>li") )
            {
              hot_key_element = t;
              return true; // found, stop;
            }
            if(t.visible())
            {
              hot_key_element = t;
              return true; // found, stop;
            }
            return false;
          }
        } cb;
        
        //Original version was:
        //  container.find_all(&cb, "[accesskey=='%s']", keyname);
        
        //By request of Christopher Brown, the Great, from Symantec this became as: 
        container.find_all(&cb, "[accesskey=='%s'],[accesskey-alt=='%s']", keyname, keyname);

        if( cb.hot_key_element.is_valid())
        {
          METHOD_PARAMS prm; prm.methodID = DO_CLICK; 
          //cb.hot_key_element.set_state(STATE_FOCUS);
          if(cb.hot_key_element.call_behavior_method(&prm))
            return true;
          
          // accesskey is defined for the element but it does not
          // handle DO_CLICK. Ask parents to activate it.
          // See behavior_tabs.cpp...
          htmlayout::dom::element hot_element_parent = cb.hot_key_element.parent();
          
          return hot_element_parent.send_event(ACTIVATE_CHILD,0, cb.hot_key_element);
        }
        return false;
    }

    
    // get symbolic name of the key combination
    const char* get_key_name( UINT scanCode, UINT keyboardStates)
    {
      if( (keyboardStates & SHIFT_KEY_PRESSED) != 0)
        return 0;

      if( scanCode == VK_MENU )
        return 0;

      static char buffer[ 20 ]; buffer[0] = 0;
      if( (keyboardStates & CONTROL_KEY_PRESSED) != 0)
        strcat(buffer,"^"); // ctrl
      if( keyboardStates == 0)
        strcat(buffer,"!"); // key without ALT modificator (as is).

      if( scanCode >= 'A' && scanCode <= 'Z' )
      {
        char s[2]; s[0] = char(scanCode); s[1] = 0;
        strcat(buffer,s);
        return buffer;
      } 
      else if( scanCode >= '0' && scanCode <= '9' )
      {
        char s[2]; s[0] = char(scanCode); s[1] = 0;
        strcat(buffer,s);
        return buffer;
      } 
      else switch(scanCode)
      {
         case VK_BACK        : strcat(buffer,"BACK"); return buffer;           

         case VK_CLEAR       : strcat(buffer,"CLEAR"); return buffer;          
         case VK_RETURN      : strcat(buffer,"RETURN"); return buffer;         

         case VK_ESCAPE      : strcat(buffer,"ESCAPE"); return buffer;         

         case VK_SPACE       : strcat(buffer,"SPACE"); return buffer;          
         case VK_PRIOR       : strcat(buffer,"PRIOR"); return buffer;          
         case VK_NEXT        : strcat(buffer,"NEXT"); return buffer;           
         case VK_END         : strcat(buffer,"END"); return buffer;            
         case VK_HOME        : strcat(buffer,"HOME"); return buffer;           
         case VK_LEFT        : strcat(buffer,"LEFT"); return buffer;           
         case VK_UP          : strcat(buffer,"UP"); return buffer;             
         case VK_RIGHT       : strcat(buffer,"RIGHT"); return buffer;          
         case VK_DOWN        : strcat(buffer,"DOWN"); return buffer;           
         case VK_SELECT      : strcat(buffer,"SELECT"); return buffer;         
         case VK_PRINT       : strcat(buffer,"PRINT"); return buffer;          
         case VK_EXECUTE     : strcat(buffer,"EXECUTE"); return buffer;        
         case VK_SNAPSHOT    : strcat(buffer,"SNAPSHOT"); return buffer;       
         case VK_INSERT      : strcat(buffer,"INSERT"); return buffer;         
         case VK_DELETE      : strcat(buffer,"DELETE"); return buffer;         
         case VK_HELP        : strcat(buffer,"HELP"); return buffer;           

         case VK_LWIN        : strcat(buffer,"LWIN"); return buffer;           
         case VK_RWIN        : strcat(buffer,"RWIN"); return buffer;           
         case VK_APPS        : strcat(buffer,"APPS"); return buffer;           

         case VK_NUMPAD0     : strcat(buffer,"NUMPAD0"); return buffer;        
         case VK_NUMPAD1     : strcat(buffer,"NUMPAD1"); return buffer;        
         case VK_NUMPAD2     : strcat(buffer,"NUMPAD2"); return buffer;        
         case VK_NUMPAD3     : strcat(buffer,"NUMPAD3"); return buffer;        
         case VK_NUMPAD4     : strcat(buffer,"NUMPAD4"); return buffer;        
         case VK_NUMPAD5     : strcat(buffer,"NUMPAD5"); return buffer;        
         case VK_NUMPAD6     : strcat(buffer,"NUMPAD6"); return buffer;        
         case VK_NUMPAD7     : strcat(buffer,"NUMPAD7"); return buffer;        
         case VK_NUMPAD8     : strcat(buffer,"NUMPAD8"); return buffer;        
         case VK_NUMPAD9     : strcat(buffer,"NUMPAD9"); return buffer;        
         case VK_MULTIPLY    : strcat(buffer,"MULTIPLY"); return buffer;       
         case VK_ADD         : strcat(buffer,"ADD"); return buffer;            
         case VK_SEPARATOR   : strcat(buffer,"SEPARATOR"); return buffer;      
         case VK_SUBTRACT    : strcat(buffer,"SUBTRACT"); return buffer;       
         case VK_DECIMAL     : strcat(buffer,"DECIMAL"); return buffer;        
         case VK_DIVIDE      : strcat(buffer,"DIVIDE"); return buffer;         
         case VK_F1          : strcat(buffer,"F1"); return buffer;             
         case VK_F2          : strcat(buffer,"F2"); return buffer;             
         case VK_F3          : strcat(buffer,"F3"); return buffer;             
         case VK_F4          : strcat(buffer,"F4"); return buffer;             
         case VK_F5          : strcat(buffer,"F5"); return buffer;             
         case VK_F6          : strcat(buffer,"F6"); return buffer;             
         case VK_F7          : strcat(buffer,"F7"); return buffer;             
         case VK_F8          : strcat(buffer,"F8"); return buffer;             
         case VK_F9          : strcat(buffer,"F9"); return buffer;             
         case VK_F10         : strcat(buffer,"F10"); return buffer;            
         case VK_F11         : strcat(buffer,"F11"); return buffer;            
         case VK_F12         : strcat(buffer,"F12"); return buffer;            
         case VK_F13         : strcat(buffer,"F13"); return buffer;            
         case VK_F14         : strcat(buffer,"F14"); return buffer;            
         case VK_F15         : strcat(buffer,"F15"); return buffer;            
         case VK_F16         : strcat(buffer,"F16"); return buffer;            
         case VK_F17         : strcat(buffer,"F17"); return buffer;            
         case VK_F18         : strcat(buffer,"F18"); return buffer;            
         case VK_F19         : strcat(buffer,"F19"); return buffer;            
         case VK_F20         : strcat(buffer,"F20"); return buffer;            
         case VK_F21         : strcat(buffer,"F21"); return buffer;            
         case VK_F22         : strcat(buffer,"F22"); return buffer;            
         case VK_F23         : strcat(buffer,"F23"); return buffer;            
         case VK_F24         : strcat(buffer,"F24"); return buffer;            
      }
      return 0;
    }

};

// instantiating and attaching it to the global list
accesskeys accesskeys_instance;

}
