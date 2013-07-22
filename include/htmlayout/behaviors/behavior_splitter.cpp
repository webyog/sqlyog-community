#include "behavior_aux.h"

namespace htmlayout 
{

/*

BEHAVIOR: splitter
    goal: Implementation of splitter - element-divider between two other elements.
TYPICAL USE CASE:
    <div style="flow:horizontal">
      <div style="width:100px"></div> <!-- first element -->
      <div style="behavior:splitter"></div>
      <div style="width:100%%"></div> <!-- second element -->
    </div>
SAMPLE:
   See: samples/behaviors/splitters.htm

*/

bool equal(const wchar_t* val, const wchar_t* str)
{
  if(!val)
    return false;
  return wcscmp(val,str) == 0;
}

bool is_spring(const wchar_t* val)
{
  if(!val)
    return false;
  int length = wcslen(val);
  if(length < 3)
    return false;
  return val[length-2] == '%' && val[length-1] == '%';
}

struct splitter: public behavior
{
    // ctor
    splitter(): behavior(HANDLE_MOUSE, "splitter") {}

    int pressed_offset;
    int previous_value;

    virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
    {
      if( mouseButtons != MAIN_MOUSE_BUTTON )
        return false;

      if( event_type == MOUSE_UP)
      {
        dom::element el = he;
        el.release_capture();
        return true;
      }

      if( event_type != MOUSE_MOVE && event_type != MOUSE_DOWN)
        return false;

      // mouse moved and pressed

      // el is our splitter element;
      dom::element splitter_el = he;
      dom::element parent_element = splitter_el.parent();

      // what kind of splitter do we have?
      bool horizontal = equal(parent_element.get_style_attribute("flow"),L"horizontal");

      dom::element first = splitter_el.prev_sibling();
      dom::element second = splitter_el.next_sibling();

      if(!first.is_valid() || !second.is_valid())
        return false; // nothing to do

      bool need_update = horizontal? 
        do_horizontal(event_type, pt, splitter_el,first,second, parent_element): 
        do_vertical(event_type, pt,splitter_el,first,second, parent_element);

      if(need_update && event_type == MOUSE_MOVE)
        parent_element.update(true);  //done! update changes on the view
       
      return true; // it is ours - stop event bubbling
    }

    bool do_horizontal(UINT event_type, POINT pt, 
      dom::element &splitter_el, dom::element &first, dom::element &second,
      dom::element &parent_el)
    {
      // which element width we will change?

      RECT rc_parent = parent_el.get_location();
      RECT rc = first.get_location();

      // if width of first element is less than half of parent we
      // will change its width.
      bool change_first = 
        (rc.right - rc.left) < (rc_parent.right - rc_parent.left)/2;


      if(!change_first)
        rc = second.get_location();

      if(event_type == MOUSE_DOWN)
      {
         pressed_offset = pt.x;
         splitter_el.set_capture();
         return false; // don't need updates
      }
      // mouse move handling
      if(pt.x == pressed_offset)
        return false; // don't need updates

      int width = rc.right - rc.left;

      wchar_t buf[32];
      if(change_first)
      {
        width += (pt.x - pressed_offset);
        if(width >= 0)
        {
          first.delay_measure();
          second.delay_measure();

          swprintf(buf,L"%dpx", width);
          first.set_style_attribute("width",buf);
          second.set_style_attribute("width",L"100%%");
        }
      }
      else
      {
        width -= (pt.x - pressed_offset);
        if(width >= 0)
        {
          first.delay_measure();
          second.delay_measure();

          swprintf(buf,L"%dpx", width);
          first.set_style_attribute("width",L"100%%");
          second.set_style_attribute("width",buf);
        }
      }
      return true; // need update
    }

    bool do_vertical(UINT event_type, POINT pt, 
      dom::element &splitter_el, dom::element &first, dom::element &second,
      dom::element &parent_el)
    {
      RECT rc_parent = parent_el.get_location();
      RECT rc = first.get_location();

      // if width of first element is less than half of parent we
      // will change its width.
      bool change_first = 
        (rc.bottom - rc.top) < (rc_parent.bottom - rc_parent.top)/2;

      if(!change_first)
        rc = second.get_location();

      if(event_type == MOUSE_DOWN)
      {
         pressed_offset = pt.y;
         splitter_el.set_capture();
         return false; // don't need updates
      }
      // mouse move handling
      if(pt.y == pressed_offset)
        return false; // don't need updates

      int height = rc.bottom - rc.top;

      wchar_t buf[32];

      if(change_first)
      {
        height += (pt.y - pressed_offset);
        if(height >= 0)
        {
          first.delay_measure();
          second.delay_measure();

          swprintf(buf,L"%dpx", height);
          first.set_style_attribute("height",buf);
          second.set_style_attribute("height",L"100%%");
        }
      }
      else
      {
        height -= (pt.y - pressed_offset);
        if(height >= 0)
        {
          first.delay_measure();
          second.delay_measure();

          swprintf(buf,L"%dpx", height);
          first.set_style_attribute("height",L"100%%");
          second.set_style_attribute("height",buf);
        }
      }
      return true;
    }
    

   
};

// instantiating and attaching it to the global list
splitter splitter_instance;


}

