#include "behavior_aux.h"

/*
BEHAVIOR: gripper
    
  starts draging of its parent upon receving of MOUSE_DOWN event

TYPICAL USE CASE:
   < div .toolbar >
     <div style="behavior:gripper; width:8px; height:*;" />
     <widget .button>...</widget>
     <widget .button>...</widget>
     <widget .button>...</widget>
   </div>
*/


namespace htmlayout 
{

  struct gripper: public behavior
  {
      // ctor
      gripper(): behavior(HANDLE_MOUSE, "gripper") { }
    
      virtual BOOL handle_mouse  (HELEMENT he, MOUSE_PARAMS& params ) 
      {
        if( params.cmd == MOUSE_DOWN && params.button_state == 1 )
        {
          params.dragging = dom::element(he).parent();
          params.dragging_mode = DRAGGING_MOVE;
          return true;
        }
        return false;
      }
  };

  // instantiating and attaching it to the global list
  gripper gripper_instance;

} // htmlayout namespace

