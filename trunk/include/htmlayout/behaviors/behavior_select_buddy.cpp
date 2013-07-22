#include "behavior_aux.h"


namespace htmlayout 
{

/*

BEHAVIOR: select-buddy
    goal: Auxiliary buttons for various <select multiple> actions.
    actions: "select-all" - select all options
             "clear-all" - deselect all options
    for pair of <select>s:
             "move-all" - move all options from source to destination
             "move-selected" - move selected options from source to destination
             "revoke-all" - move all options from destination to source
             "revoke-selected" - move all options from destination to source
TYPICAL USE CASE:
   <div style="behavior:select-buddy">
      <widget type="select" multiple="checks" role="source"> 
         <option>...
      </widget> 
      <input type="button" action="move-all">move all</input>
      <input type="button" action="move-selected">move selected</button>
      <widget type="select" multiple="checks" role="destination"> 
         <option>...
      </widget> 
   </div>
SAMPLE:
*/

struct select_buddy: public behavior
{

    // ctor
    select_buddy(): behavior(HANDLE_BEHAVIOR_EVENT, "select-buddy") {}

    virtual BOOL on_event (HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason ) 
    { 
      if( type == BUTTON_CLICK )
      {
        dom::element btn = target;
        const wchar_t* action = btn.get_attribute("action");
        if( !action )
          return FALSE;  // it is not a button we know about

        dom::element el = he; // this is our container-observer we attached to

        if( aux::wcseq(action,L"select-all") )
          return do_select_all(el);
        if( aux::wcseq(action,L"clear-all") )
          return do_clear_all(el);
        if( aux::wcseq(action,L"move-all") )
          return do_move(el, true, false);
        if( aux::wcseq(action,L"move-selected") )
          return do_move(el, true, true);
        if( aux::wcseq(action,L"revoke-all") )
          return do_move(el, false, false);
        if( aux::wcseq(action,L"revoke-selected") )
          return do_move(el, false, true);

      }
      /*
      else if (type == SELECT_SELECTION_CHANGED)
      {
        dom::element el = he;
        return on_selection_changed(el);
      }
      */

      return false;
    }

    bool do_select_all(  dom::element& el )
    {
      dom::element select = el.find_first("[role='source']");
      if(!select.is_valid())
      {
        assert(false);
        return false;
      }
      select_all_options(select);
      return true;
    }
    bool do_clear_all(  dom::element& el )
    {
      dom::element select = el.find_first("[role='source']");
      if(!select.is_valid())
      {
        assert(false);
        return false;
      }
      clear_all_options(select);
      return true;
    }

    bool do_move(  dom::element& el, bool from_src_to_dst, bool selected_only )
    {
      dom::element select_src = el.find_first("[role='source']");
      dom::element select_dst = el.find_first("[role='destination']");

      if(!select_src.is_valid() || !select_dst.is_valid())
      {
        assert(false);
        return false;
      }

      if( !from_src_to_dst ) // swap src and destination
      {
        dom::element tmp = select_src;  
        select_src = select_dst;
        select_dst = tmp;
      }

      selected_cb options;
      select_src.find_all(&options, selected_only?"option:checked":"option"); // select all currently selected <option>s

      // move elements from one container to another
      if(from_src_to_dst)
        for( int n = options.elements.size() - 1; n >= 0 ; --n )
        {
          dom::element& opt = options.elements[n];
          unsigned int idx = opt.index();
          wchar_t buf[32]; swprintf(buf,L"%d",idx);
          opt.set_attribute("-srcindex",buf);
          select_dst.insert( opt, 0 );
        }
      else
        for( unsigned int n = 0; n < options.elements.size() ; ++n )
        {
          int i = _wtoi(options.elements[n].get_attribute("-srcindex"));
          select_dst.insert( options.elements[n], i );
        }

      select_src.update();
      select_dst.update();
      return true;
    }
  
   
};

// instantiating and attaching it to the global list
select_buddy select_buddy_instance;

} // htmlayout namespace

