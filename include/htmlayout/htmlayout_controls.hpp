/*
 * Terra Informatica Lightweight Embeddable HTMLayout control
 * http://terrainformatica.com/htmlayout
 * 
 * Active DOM elements (a.k.a windowless controls)
 * 
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 * 
 * 
 * (C) 2003-2005, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

#ifndef __htmlayout_controls_hpp__
#define __htmlayout_controls_hpp__

#pragma once

/*!\file
\brief Controls and their values.
*/

#include <stdio.h> // snprintf

#include "value.h" 
#include "htmlayout_dom.hpp"
#include "htmlayout_value.hpp"
#include "htmlayout_aux.h" // utf8 

namespace htmlayout 
{

/** Get type of input control this DOM element behaves as. 
 * \param[in] el const dom::element&, The element.
 **/
  inline CTL_TYPE get_ctl_type(const dom::element& el) { return el.get_ctl_type(); }
  
  struct  selected_cb: dom::callback 
  {
    std::vector<dom::element> elements;
    inline bool on_element(HELEMENT he) 
    { 
      elements.push_back(he);
      return false; // keep enumerating
    }
  };

  inline json::value get_radio_index( dom::element& el )
  {
    selected_cb selected;
    dom::element r = el.parent(); // ATTN: I assume here that all radios in the group belong to the same parent!
    r.find_all(&selected, "[type='radio'][name='%S']", el.get_attribute("name"));
    for( unsigned int n = 0; n < selected.elements.size(); ++n )
      if ( selected.elements[n].get_state(STATE_CHECKED) )
        return json::value(int(n)); 
    return json::value();
  }

  inline void set_radio_index( dom::element& el, const json::value& t  )
  {
    selected_cb selected;
    dom::element r = el.parent(); // ATTN: I assume here that all radios in the group belong to the same parent!
    r.find_all(&selected, "[type='radio'][name='%S']", el.get_attribute("name"));
    unsigned int idx = (unsigned int)t.get(0);
    for( unsigned int n = 0; n < selected.elements.size(); ++n )
    {
      dom::element& e = selected.elements[n];
      if ( n == idx)
      {
        e.set_value(json::value(true));
        break;
      }
    }
  }

  // the same as above but for arbitrary root/name 
  inline void set_radio_index( dom::element root, const char* name, unsigned int idx  )
  {
    selected_cb selected;
    root.find_all(&selected, "[type='radio'][name='%S']", name);
    for( unsigned int n = 0; n < selected.elements.size(); ++n )
    {
      dom::element& e = selected.elements[n];
      if ( n == idx)
      {
        e.set_value(json::value(true));
        break;
      }
    }
  }

  // returns bit mask - checkboxes set
  inline json::value get_checkbox_bits(dom::element& el )
  {
    selected_cb selected;
    dom::element r = el.parent(); // ATTN: I assume here that all checkboxes in the group belong to the same parent!
    r.find_all(&selected, "[type='checkbox'][name='%S']", el.get_attribute("name"));
    int m = 1, v = 0;
    for( unsigned int n = 0; n < selected.elements.size(); ++n, m <<= 1 )
      if ( selected.elements[n].get_state(STATE_CHECKED) ) v |= m;
    return selected.elements.size()==1?json::value(v==1):json::value(v); // for alone checkbox we return true/false 
  }

  // sets checkboxes by bit mask 
  inline void set_checkbox_bits(dom::element& el, const json::value& t )
  {
    selected_cb selected;
    dom::element r = el.parent(); // ATTN: I assume here that all checkboxes in the group belong to the same parent!
    r.find_all(&selected, "[type='checkbox'][name='%S']", el.get_attribute("name"));
    int m = 1, v = selected.elements.size()==1?(t.get(false)?1:0):t.get(0);
    for( unsigned int n = 0; n < selected.elements.size(); ++n, m <<= 1 )
    {
      dom::element& e = selected.elements[n];
      if( (v & m) != 0)
          e.set_state(  STATE_CHECKED, 0 ) ;
      else
          e.set_state(  0, STATE_CHECKED ) ;
    }
  }
  
  inline json::value get_option_value(const dom::element& opt )
  {
    const wchar_t* val = opt.get_attribute("value");
    if( val ) return json::value::from_string(val);
    return json::value(opt.text().c_str());
  }

/* OBSOLETE stuff

  // single select
  inline json::value get_select_value(dom::element& el )
  {
    json::value v;
    dom::element opt = el.find_first("option:checked,[role='option']:checked"); // select all selected <option>s
    if( opt.is_valid() )
      v = get_option_value(opt);
    return v;
  }

  // single select
  inline void set_select_value(dom::element& el, const json::value& t )
  {
    json::value v;
    std::wstring ws = t.to_string();
    dom::element new_opt = el.find_first("option[value='%S'],[role='option'][value='%S']", ws.c_str(), ws.c_str()); // find it
    if( new_opt.is_valid() )
      new_opt.set_state( STATE_CHECKED | STATE_CURRENT, 0 ); // set state
  }

  // multi-select - returns array of selected values
  inline json::value get_select_values(dom::element& el )
  {
    selected_cb selected;
    el.find_all(&selected, "option:checked,[role='option']:checked"); // select all selected <option>s

    if(selected.elements.size() == 0) 
      return json::value();

    std::vector<json::value> values(selected.elements.size());

    for( unsigned int n = 0; n < selected.elements.size(); ++n )
      values[n] = get_option_value(selected.elements[n]);

    return json::value(&values[0], values.size()); 
  }

  // multi-select - select values
  inline void set_select_values(dom::element& el, const json::value& val_array )
  {
    
    selected_cb selected;
    el.find_all(&selected, "option:checked,[role='option']:checked"); // select all currently selected <option>s

    //if(selected.elements.size() == 0) 
    //std::vector<json::value> values(selected.elements.size());

    for( int n = selected.elements.size() - 1; n >= 0 ; --n )
      selected.elements[n].set_state(0, STATE_CHECKED); // reset values

    if(!val_array.is_array())
      return;

    for( int k = val_array.length(); k >= 0 ; --k )
    {
      std::wstring ws = val_array.nth(k).to_string();
      dom::element opt = el.find_first("option[value='%S'],[role='option'][value='%S']", ws.c_str()); // find it
      if( opt.is_valid() )
        opt.set_state( STATE_CHECKED | (k == 0? STATE_CURRENT:0)); // set state
    }
  }
  */

  // clear checked states in multiselect <select>.
  // this simply resets :checked state for all checked <option>'s
  inline void clear_all_options(dom::element& select_el )
  {
    selected_cb selected;
    select_el.find_all(&selected, "option:checked,[role='option']:checked"); // select all currently selected <option>s

    for( int n = int(selected.elements.size()) - 1; n >= 0 ; --n )
      selected.elements[n].set_state(0, STATE_CHECKED, false); // reset state

    select_el.update();
  }

  // selects all options in multiselect.
  inline void select_all_options(dom::element& select_el )
  {
    selected_cb all_options;
    select_el.find_all(&all_options, "option"); // select all currently selected <option>s

    for( int n = int(all_options.elements.size()) - 1; n >= 0 ; --n )
       all_options.elements[n].set_state(STATE_CHECKED,0, false); // set state
    select_el.update();
  }

/** Get value of the DOM element. Returns value for elements recognized by get_ctl_type() function. 
 * \param[in] el \b const dom::element&, The element.
 * \return \b json::value, value of the element.
 **/
  inline json::value get_value(dom::element& el )
  {
    switch(get_ctl_type(el))
    {
      case CTL_EDIT:
      case CTL_DECIMAL:
      case CTL_CURRENCY:
      case CTL_PASSWORD:
      case CTL_NUMERIC: 
      case CTL_PROGRESS:        
      case CTL_SLIDER:          
      case CTL_SELECT_SINGLE:   
      case CTL_SELECT_MULTIPLE: 
      case CTL_DD_SELECT:       
      case CTL_TEXTAREA:
      case CTL_DATE:
      case CTL_CALENDAR:
      default:
        return el.get_value();

      // special cases:
      case CTL_UNKNOWN:         
        if( !aux::wcseq(el.get_attribute("type"),L"hidden"))
          break;
        //else fall below if it is hidden
      case CTL_BUTTON:          return json::value(el.get_attribute("value"));
      case CTL_CHECKBOX:        return get_checkbox_bits(el);
      case CTL_RADIO:           return get_radio_index(el);
      case CTL_HTMLAREA:        return json::value( aux::utf2w(el.get_html(false/*inner*/)));
    }
    return json::value();
  }

/** Set value of the DOM element. Sets value for elements recognized by get_ctl_type() function. 
 * \param[in] el \b const dom::element&, The element.
 * \param[in] v \b const json::value&, The value.
 **/
  inline void set_value(dom::element& el, const json::value& v )
  {
    switch(get_ctl_type(el))
    {
      case CTL_UNKNOWN:         break;
      case CTL_EDIT:
      case CTL_DECIMAL:
      case CTL_CURRENCY:
      case CTL_PASSWORD:
      case CTL_NUMERIC: 
      case CTL_PROGRESS:        
      case CTL_SLIDER:          
      case CTL_SELECT_SINGLE:   
      case CTL_SELECT_MULTIPLE: 
      case CTL_DD_SELECT:       
      case CTL_TEXTAREA:
      case CTL_DATE:
      case CTL_CALENDAR:
      case CTL_HIDDEN:
      default:
        el.set_value(v);
        break;
      // special cases:
      case CTL_BUTTON:          break;
      case CTL_CHECKBOX:        set_checkbox_bits(el,v); break;
      case CTL_RADIO:           set_radio_index(el,v);  break;
      case CTL_HTMLAREA:        
       {
          aux::w2utf utf8(static_cast<const wchar_t*>(v.get( L"" ))); 
          el.set_html( utf8, utf8.length() );
          //el.update();
        } break;
      case CTL_NO:
        assert(false);
        break;
    }
  }

/** Collection (map) of Name/Value pairs. **/
  typedef std::map<std::wstring, json::value> named_values;
  typedef std::map<std::wstring, json::value>::iterator named_values_iterator;

/** Get values of all "controls" contained inside the DOM element. 
 *  Function will gather values of elements having name attribute defined
 *  and recognized by get_ctl_type() function.
 * \param[in] el \b dom::element&, The element.
 * \param[out] all \b named_values&, Collection.
 * \return \b bool, \c true if there are any value was harvested.
 **/
  inline bool get_values(const dom::element& el, named_values& all )
  {
    selected_cb selected;
    el.find_all(&selected, "[name]" ); // select all elements having name attribute
    for( unsigned int n = 0; n < selected.elements.size(); ++n )
    {
      const dom::element& t = selected.elements[n];
      //if( !t.get_style_attribute("behavior") )
      //  continue; - commented out to support input type="hidden" that does not have behavior assigned
      const wchar_t* pn = t.get_attribute("name");
      if( !pn )
      {
        assert(false); // how come?
        continue;
      }
      std::wstring name = pn;
      if( all.find(name) != all.end()) 
        continue; // element with this name is already there, 
                  // checkboxes and radios are groups in fact,
                  // we are returning here only cumulative group value
      int ctl_type = get_ctl_type(t);
      if( ctl_type == CTL_NO/*|| ctl_type == CTL_BUTTON*/)
        continue;
      all[name] = get_value(selected.elements[n]);
    }
    return all.size() != 0;
  }

/** Set values of all "controls" contained inside the DOM element by items 
 *  contained in the all colection. 
 *  Function will set values of elements having name attribute defined
 *  and recognized by get_ctl_type() function.
 * \param[in] el \b dom::element&, The element.
 * \param[out] all \b named_values&, Collection.
 * \return \b bool, \c true if there are any value was harvested.
 **/
  inline bool set_values(dom::element& el, named_values& all )
  {
    int counter = 0;
    for( named_values_iterator it = all.begin(); it != all.end(); it++ )
    {
      std::wstring name = (*it).first;
      //::MessageBoxW(NULL, name, L"" )
      dom::element t = el.find_first("[name='%S']",name.c_str()); 
      if( !t.get_style_attribute("behavior") )
        continue;
      set_value(t, (*it).second);
      ++counter;
    }
    return counter != 0;
  }

} //namespace htmlayout



#endif

