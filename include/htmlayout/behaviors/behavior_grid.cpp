#include "behavior_aux.h"

namespace htmlayout 
{
 
/** behavior:grid, browser of tabular data (records).
 *  Supports multiple selection mode. Use "multiple" attribute.
 *
 *   model is like this:
 *   <table fixedrows=1 ... style="behavior:grid"> 
 *     <tr>...
 *     <tr>...
 *   </table> 
 *
 *  LOGICAL EVENTS: 
 *    TABLE_HEADER_CLICK, click on some cell in table header, target is the cell
 *    TABLE_ROW_CLICK,    click on data row in the table, target is the row
 *
 *  See: html_samples/grid/scrollable-table.htm
 *  Authors: Andrew Fedoniouk, initial implementation.
 *           Andrey Kubyshev, multiselection mode.
 *
 **/
    
struct grid: public behavior
{
    // ctor
    grid(const char* name = "grid", int additional_flags = 0): behavior(HANDLE_MOUSE | HANDLE_SCROLL | HANDLE_KEY | HANDLE_FOCUS | additional_flags, name) {}
    
    virtual void attached  (HELEMENT he ) 
    { 
#ifdef _DEBUG
       dom::element table = he;
       assert( aux::streq(table.get_element_type(), "table") ); // must be table.
#endif
       //test: table.set_attribute( "width", L"32px");
    } 

    /** is it multiple selectable? **/
    bool is_multiple (const dom::element& table)
    {
      return table.get_attribute ("multiple") != 0;
    }

    /** Click on column header (fixed row).
        Overrided in sortable-grid **/
    virtual void on_column_click( dom::element& table, dom::element& header_cell )
    {
      table.post_event( TABLE_HEADER_CLICK, header_cell.index(), header_cell); 
    }

    /** returns current row (if any) **/
    dom::element get_current_row( dom::element& table )
    {
      for( int i = table.children_count() - 1; i >= 0 ; --i)
      {
        dom::element t = table.child((unsigned int)i);
        if( t.get_state(STATE_CURRENT))
          return t;
      }
      return dom::element(); // empty
    }

    /** set current row **/
    void set_current_row( dom::element& table, dom::element& row, UINT keyboardStates, bool dblClick = false, bool smooth = false )
    {
      // get previously selected row:
      dom::element prev = get_current_row( table );

      uint new_row_checked = STATE_CHECKED;
      uint old_row_checked = STATE_CHECKED;

      if(is_multiple(table))
      {
         if (keyboardStates & SHIFT_KEY_PRESSED)
         {
            checkall(table, false);
            check_range(table,row.index(),TRUE); // from current to new
         } 
         else 
         {      
           if (keyboardStates & CONTROL_KEY_PRESSED)
           {
             set_checked_row (table,row, true); // toggle
             new_row_checked = 0;
           }
           else
             checkall(table, false);
           set_anchor(table,row.index ());
         }
         old_row_checked = 0;
      }
      
      if( prev.is_valid() )
      {
        if( prev != row ) 
          prev.set_state(0,STATE_CURRENT | old_row_checked); // drop state flags
      }
      row.set_state(STATE_CURRENT | new_row_checked); // set state flags
      row.scroll_to_view(false,smooth);
      //::UpdateWindow(row.get_element_hwnd(false));
      table.post_event( dblClick? TABLE_ROW_DBL_CLICK:TABLE_ROW_CLICK, row.index(), row); 
      table.post_event(WY_GRID_VERTICAL_SCROLL, 0);
    }

    dom::element target_row(const dom::element& table, const dom::element& target)
    {
      if( !target.is_valid() || target.parent() == table)
        return target;
      return target_row(table,target.parent());
    }

    dom::element target_header(const dom::element& header_row, const dom::element& target)
    {
      if( !target.is_valid() || target.parent() == header_row)
        return target;
      return target_header(header_row,target.parent());
    }

    int fixed_rows( const dom::element& table )
    {
      return table.get_attribute_int("fixedrows",0);
    }
  
    void set_checked_row( dom::element& table, dom::element& row, bool toggle = false )
    {
      if(toggle)
      {
        if( row.get_state( STATE_CHECKED) )
          row.set_state( 0,STATE_CHECKED);
        else
          row.set_state( STATE_CHECKED,0);
      }
      else
        row.set_state( STATE_CHECKED,0);
    }

    int get_anchor (const dom::element& table)
    {
      dom::element row = table.find_first("tr:anchor");
      if( row.is_valid() ) return (int)row.index();
      return 0;
    }

    void set_anchor (dom::element& table,const int idx)
    {
      dom::element row = table.find_first("tr:anchor");
      if( row.is_valid() ) row.set_state( 0,STATE_ANCHOR,false);
      row = table.child(idx);
      if( row.is_valid() )
        row.set_state( STATE_ANCHOR,0,false);
    }

    void check_range (const dom::element& table, int idx, bool check)
    {
      if (!is_multiple(table)) return;

      int   start_idx = get_anchor(table);
      int   start = min(start_idx,idx );
      int   end = max(start_idx,idx );

      int   f_rows  = fixed_rows(table);
      if(start < f_rows) start = f_rows;

      for( ;end >= start; --end ) 
      {
        dom::element row = table.child(end);
        if(!!row.visible())
        {
          if (check) row.set_state(STATE_CHECKED,0);
          else row.set_state(0,STATE_CHECKED);
        }
      }
    }


    /* Resizeable columns
    virtual BOOL handle_mouse  (HELEMENT he, MOUSE_PARAMS& params ) 
    {
       if( params.cmd == MOUSE_MOVE )
       {
         if(handle_mouse_move(he, params );
           return true;
       }
       return behavior::handle_mouse(he, params);
    }

    virtual BOOL handle_mouse_move (HELEMENT he, MOUSE_PARAMS& params ) 
    {
      // el must be table;
      dom::element table = he;
      int fixed_rows = (int)fixed_rows(table);
      if( !fixed_rows )
        return false;

      dom::element row = target_row(table, dom::element(params.target));
      dom::element header_cell;

      if(row.is_valid()) // click on the row
      {
        if( (int)row.index() < (int)fixed_rows(table) )
        {
          // click on the header cell
          header_cell = target_header(row,target);
          if( header_cell.is_valid() && header_cell.get_style_attribute("width") )  
          {
             if( mouseButtons == MAIN_BUTTON )
               goto RESIZE_COLUMN;
             else
               goto UPDATE_CURSOR; 
          }
        }
      }
      return false;

RESIZE_COLUMN:
      if(header_cell.get_style_attribute("width"))

UPDATE_CURSOR:
      params.cursor_type = CURSOR_SIZEWE;
      return true; 
    }
    */

    virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
    {
        dom::element table = he;

        if(event_type == MOUSE_WHEEL)
        {
            table.post_event(WY_GRID_VERTICAL_SCROLL, SCROLL_STEP_PLUS);
        }

      if( event_type != (MOUSE_DOWN | SINKING) && event_type != MOUSE_DCLICK )
        return false;

      //if(mouseButtons != MAIN_MOUSE_BUTTON) 
      //  return false;

      // el must be table;
      dom::element row = target_row(table, dom::element(target));

      if(row.is_valid()) // click on the row
      {
        if( (int)row.index() < (int)fixed_rows(table) )
        {
          // click on the header cell
          dom::element header_cell = target_header(row,target);
          if( header_cell.is_valid() )  
              on_column_click(table, header_cell);
          return mouseButtons == MAIN_MOUSE_BUTTON;
        }
        set_current_row(table, row, keyboardStates,  event_type == MOUSE_DCLICK);
        HTMLayoutUpdateWindow(table.get_element_hwnd(false));
      }
      return FALSE;//mouseButtons == MAIN_MOUSE_BUTTON; // as it is always ours then stop event bubbling
    }

    //int firstVisibleRow( dom::element& table )

    virtual BOOL on_key(HELEMENT he, HELEMENT target, UINT event_type, UINT code, UINT keyboardStates ) 
    { 
      if( event_type == KEY_DOWN )
      {
        dom::element table = he;
        switch( code )
        {
          case VK_DOWN: 
            {
               dom::element c = get_current_row( table );
               int idx = c.is_valid()? (c.index() + 1):fixed_rows(table);
               while( idx < (int)table.children_count() )
               {
                   dom::element row = table.child(idx);
                   if( !row.visible())
                   {
                     ++idx;
                     continue;
                   }
                   set_current_row(table, row, keyboardStates); 
                   break;
               }
            }
            return TRUE;
          case VK_UP:             
            {
               dom::element c = get_current_row( table );
               int idx = c.is_valid()? (c.index() - 1):(table.children_count() - 1);
               while( idx >= fixed_rows(table) )
               {
                   dom::element row = table.child(idx);
                   if( !row.visible())
                   {
                     --idx;
                     continue;
                   }
                   set_current_row(table, row, keyboardStates); 
                   break;
               }
            }
            return TRUE;
          case VK_PRIOR:
            {
               RECT trc = table.get_location(ROOT_RELATIVE | SCROLLABLE_AREA);
               int y = trc.top - (trc.bottom - trc.top);
               int first = fixed_rows(table);
               dom::element r;
               for( int i = table.children_count() - 1; i >= first; --i )
               {
                   dom::element nr = table.child(i);
                   if( aux::wcseq(nr.get_style_attribute("display"),L"none" ))
                     continue;
                   dom::element pr = r;
                   r = nr;
                   if( r.get_location(ROOT_RELATIVE | BORDER_BOX).top < y )
                   {
                      // row found
                      if(pr.is_valid()) r = pr; // to last fully visible
                      break;
                   }
               }
               set_current_row(table, r, keyboardStates, false, true); 
            }
            return TRUE;

          case VK_NEXT:
            {
               RECT trc = table.get_location(ROOT_RELATIVE | SCROLLABLE_AREA);
               int y = trc.bottom + (trc.bottom - trc.top);
               int last = table.children_count() - 1;
               dom::element r; 
               for( int i = fixed_rows(table); i <= last; ++i )
               {
                   dom::element nr = table.child(i);
                   if( aux::wcseq(nr.get_style_attribute("display"),L"none" ))
                     continue;
                   dom::element pr = r;
                   r = nr;
                   if( r.get_location(ROOT_RELATIVE | BORDER_BOX).bottom > y )
                   {
                      // row found
                      if(pr.is_valid()) r = pr; // to last fully visible
                      break;
                   }
               }
               set_current_row(table, r, keyboardStates, false, true); 
            }
            return TRUE;

          case VK_HOME:
            {
               int idx = fixed_rows(table);
               while( (int)idx < (int)table.children_count() )
               {
                   dom::element row = table.child(idx);
                   if( !row.visible())
                   {
                     ++idx;
                     continue;
                   }
                   set_current_row(table, row, keyboardStates, false, true); 
                   break;
               }
            }
            return TRUE;

          case VK_END:
            {
               int idx = table.children_count() - 1;
               while( idx >= fixed_rows(table) )
               {
                   dom::element row = table.child(idx);
                   if( !row.visible())
                   {
                     --idx;
                     continue;
                   }
                   set_current_row(table, row, keyboardStates, false, true); 
                   break;
               }
            }
            return TRUE;
          case 'A':
            if( is_multiple(table) && (keyboardStates & CONTROL_KEY_PRESSED) != 0 )
            {
              checkall(table, true);
              return TRUE;
            }
            return FALSE;
        }
      }
      return FALSE; 
    }

    void checkall (dom::element& table, bool onOff )
    {
      if( !is_multiple(table) ) return;

      struct unchecker_cb: dom::callback 
      {
          bool on_element(HELEMENT he) 
          { 
            htmlayout::dom::element el = he; if( el.get_state(STATE_CHECKED)) el.set_state(0,STATE_CHECKED ); return false; /*continue enumeration*/ 
          }
      };
      struct checker_cb: dom::callback 
      {
          bool on_element(HELEMENT he) 
          { 
            htmlayout::dom::element el = he; if( !el.get_state(STATE_CHECKED)) el.set_state(STATE_CHECKED,0 ); return false; /*continue enumeration*/ 
          }
      };

      if(onOff) 
      {
        checker_cb checker;
        table.find_all(&checker,"tr");
      }
      else
      {
        unchecker_cb unchecker;
        table.find_all(&unchecker,"tr:checked");
      }
    }

    virtual BOOL handle_scroll(HELEMENT he, SCROLL_PARAMS& params)
    {
        htmlayout::dom::element ele = he;
        
        if(params.vertical)
        {
            ele.post_event(WY_GRID_VERTICAL_SCROLL, params.cmd);
        }       

        return FALSE;
    }
  
};

struct sortable_grid: public grid
{
  typedef grid super;

  // ctor
  sortable_grid(): super("sortable-grid") {}

  virtual void on_column_click( dom::element& table, dom::element& header_cell )
  {
    super::on_column_click( table, header_cell );

    dom::element current = table.find_first("th:checked");
    if( current == header_cell )
      return; // already here, nothing to do.

    if( current.is_valid() )
      current.set_state(0, STATE_CHECKED);
    header_cell.set_state(STATE_CHECKED);

  dom::element ctr = get_current_row( table );
    sort_rows( table, header_cell.index() );
  if( ctr.is_valid() )
    ctr.scroll_to_view();

  }

  struct row_sorter: public dom::element::comparator
  {
    int column_no;

    row_sorter( int col_no ): column_no(col_no) {}

    virtual int compare(const htmlayout::dom::element& r1, const htmlayout::dom::element& r2)
    {
      if( !r1.is_valid() || !r2.is_valid() )
        return 0;

      htmlayout::dom::element c1 = r1.child(column_no);
      htmlayout::dom::element c2 = r2.child(column_no);

	  json::string t1 = c1.text();
      json::string t2 = c2.text();
      return wcscmp(t1,t2);
    }
  };

  void sort_rows( dom::element& table, int column_no )
  {
    row_sorter rs( column_no );

    int fr = fixed_rows( table );
    table.sort(rs,fr);
  }
};


// instantiating and attaching it to the global list
grid          grid_instance;
sortable_grid sortable_grid_instance;

}






