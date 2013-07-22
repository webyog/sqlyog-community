#include "behavior_aux.h"

namespace htmlayout 
{

/*

BEHAVIOR: actions
  
  Simplistic action interpretter.

  on button clicks exectues actions described by 'action' attribute.

TYPICAL USE CASE:
   < div style="behavior:actions" >
        ....
        <button action="alert: Hello world!" >
        .... 
   </div>
*/

static bool parse_args( aux::wchars a, aux::wchars& arg );
static bool parse_args( aux::wchars a, aux::wchars& arg1, aux::wchars& arg2 );
static bool parse_args( aux::wchars a, aux::wchars& arg1, aux::wchars& arg2, aux::wchars& arg3, aux::wchars& arg4 );

// parse dtring "checked" or "!checked" to int STATE_CHECKED, etc.
static int parse_state( aux::wchars sst );


struct actions: public behavior
{

    // ctor
    actions(): behavior(HANDLE_BEHAVIOR_EVENT, "actions") {}

    virtual BOOL on_event (HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason ) 
    { 
      if( type != BUTTON_CLICK )
        return FALSE;
      
      dom::element btn = target;
      const wchar_t* action = btn.get_attribute("action");
      if( !action )
        return FALSE;  // it is not a button we know about
      if(interpret_action( btn.root(), action) )
        return TRUE;
      return FALSE;
    }

    // Just an idea: here should go some simple interpretter.
    // If you know one - let me know.
    // For a while it is just dumb thing like this:
    bool interpret_action( dom::element root, const wchar_t* action )
    {
      aux::wchars a = aux::chars_of(action);
      if( a.like(L"alert:*"))
      {
        aux::wchars msg;
        if(parse_args(a,msg))
          ::MessageBoxW(root.get_element_hwnd(true),msg.start,L"alert!",MB_OK);
        return true;
      }
      else if( a.like(L"copy-value:*")) // copy-value: dst selector , src selector
      {
        aux::wchars src_sel, dst_sel;
        if(!parse_args(a,dst_sel,src_sel))
          return true;
        dom::element src = root.find_first((const char*)aux::w2a(src_sel));
        dom::element dst = root.find_first((const char*)aux::w2a(dst_sel));
        if(!src.is_valid() || !dst.is_valid())
        {
          assert(0); // not found!
          return false;
        }
        json::value v = src.get_value();
        dst.set_value(v);
        dst.set_state(STATE_FOCUS);
        return true;
      }
      else if( a.like(L"copy-state:*")) 
        return action_copy_state( root, a);
      
      return false;
    }

    struct Actor: dom::callback 
    {
      int  state_to_set;
      int  state_to_clear;
      Actor():state_to_set(0),state_to_clear(0) {}
      virtual bool on_element(HELEMENT he) 
      {
         htmlayout::dom::element el = he; 
         el.set_state(state_to_set,state_to_clear); 
         return false; /*continue enumeration*/ }
      virtual ~Actor() {}
    };

    // copy-state: dst-state, dst-selector, src-state, src-selector
    bool action_copy_state( dom::element root, aux::wchars cmd)
    {
      aux::wchars src_state, src_sel, dst_state, dst_sel;
      if(!parse_args(cmd,dst_state, dst_sel, src_state, src_sel))
        return true;
      dom::element src = root.find_first((const char*)aux::w2a(src_sel));
      if(!src.is_valid())
        return true;

      Actor actor;

      bool v = src.get_state( parse_state(src_state) );
      if(src_state[0] == '!') v = !v;
      
      actor.state_to_set = 0; actor.state_to_clear = 0;
      if(v) { actor.state_to_set = parse_state( dst_state ); actor.state_to_clear = 0; }
      else  { actor.state_to_set = 0; actor.state_to_clear = parse_state( dst_state ); }
      
      // walk through all dst_sel elements.
      root.find_all(&actor, (const char*)aux::w2a(dst_sel));

      return true;

    }
};

// instantiating and attaching it to the global list
actions actions_instance;


bool parse_args( aux::wchars a, aux::wchars& arg )
{
  aux::wtokens wt( a, L":" );
  aux::wchars t;
  for(int n = 0; n < 2; ++n)
    if(!wt.next(t))
    {
      assert(0); // wrong format!
      return false;
    }
    else if( n == 1) 
      arg = t;
  return true;
}

bool parse_args( aux::wchars a, aux::wchars& arg1, aux::wchars& arg2 )
{
  aux::wtokens wt( a, L":," );
  aux::wchars t;
  for(int n = 0; n < 3; ++n)
    if(!wt.next(t))
    {
      assert(0); // wrong format!
      return false;
    }
    else switch( n ) 
    {
      case 1: arg1 = aux::trim(t); break;
      case 2: arg2 = aux::trim(t); break;
    }
  return true;
}

bool parse_args( aux::wchars a, aux::wchars& arg1, aux::wchars& arg2, aux::wchars& arg3, aux::wchars& arg4 )
{
  aux::wtokens wt( a, L":," );
  aux::wchars t;
  for(int n = 0; n < 5; ++n)
    if(!wt.next(t))
    {
      assert(0); // wrong format!
      return false;
    }
    else switch( n ) 
    {
      case 1: arg1 = aux::trim(t); break;
      case 2: arg2 = aux::trim(t); break;
      case 3: arg3 = aux::trim(t); break;
      case 4: arg4 = aux::trim(t); break;
    }
  return true;
}

int parse_state( aux::wchars sst )
{
  if( sst[0] == '!' )
  {
    ++sst.start;
    --sst.length; 
  }
  if( sst == const_wchars("checked") ) return STATE_CHECKED;
  if( sst == const_wchars("active") ) return STATE_ACTIVE;
  if( sst == const_wchars("focus") ) return STATE_FOCUS;
  if( sst == const_wchars("current") ) return STATE_CURRENT;
  if( sst == const_wchars("checked") ) return STATE_CHECKED;
  if( sst == const_wchars("disabled") ) return STATE_DISABLED;
  if( sst == const_wchars("readonly") ) return STATE_READONLY;
  if( sst == const_wchars("expanded") ) return STATE_EXPANDED;
  if( sst == const_wchars("collapsed") ) return STATE_COLLAPSED;
  if( sst == const_wchars("anchor") ) return STATE_ANCHOR;
  if( sst == const_wchars("tabfocus") ) return STATE_TABFOCUS;
  if( sst == const_wchars("busy") ) return STATE_BUSY;
  assert(0); // sst is not recognized 
  return 0;
}



} // htmlayout namespace

