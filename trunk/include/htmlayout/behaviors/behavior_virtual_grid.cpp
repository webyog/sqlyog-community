#include "behavior_aux.h"

namespace htmlayout 
{
 
/** behavior:virtual-grid, browser of tabular data (records).
 *  records are coming from external source
 *
 *  See: html_samples/grid/virtual-grid.htm
 *  Authors: Andrew Fedoniouk, initial implementation.
 *
 **/
    
struct virtual_grid: public event_handler
{
    int first_row_idx;
    int num_rows;

    // ctor
    virtual_grid(): event_handler(HANDLE_MOUSE | HANDLE_KEY | HANDLE_FOCUS | HANDLE_SCROLL | HANDLE_BEHAVIOR_EVENT) {}
    
    virtual void attached  (HELEMENT he ) 
    {
      first_row_idx = 0;
      num_rows = 0;
      dom::element self = he;
      self.post_event(INIT_DATA_VIEW);

    } 

    virtual void detached  (HELEMENT he ) 
    {
      delete this;
    } 


    // This function is a focal point here,
    // it sends bubbling ROWS_DATA_REQUEST to 
    // the element and all its containers. 
    
    // First element in the chain that handles ROWS_DATA_REQUEST is a data source for this grid.
    
    // Data source can be attached in runtime by using attach_event_handler ( + post_event(INIT_DATA_VIEW)) to the element or
    // declared as e.g. 
    //        behavior: virtual-grid sample-data-source; 
    // this is the case when element is a data source and a view at the same time.
    
    // Another solution is to derive your own behavior from this one and override get_rows_data completely.
    
    virtual void get_rows_data( HELEMENT he )
    {
      dom::element tbl = get_table(he);

      DATA_ROWS_PARAMS drp;
      drp.firstRecord = first_row_idx;
      drp.totalRecords = num_rows;
      drp.firstRowIdx = fixed_rows(tbl);
      drp.lastRowIdx = tbl.children_count() - 1;

      dom::element self = he;
      self.send_event(ROWS_DATA_REQUEST, (UINT_PTR)&drp,tbl);

      first_row_idx = drp.firstRecord;
      num_rows      = drp.totalRecords;
    }


    HELEMENT get_table( HELEMENT he )
    {
      dom::element el = he;
      return el.find_first("table");
    }
    HELEMENT get_v_scrollbar( HELEMENT he )
    {
      dom::element el = he;
      return el.find_first("widget[type='vscrollbar']");
    }


    int num_data_rows( HELEMENT he )
    {
      dom::element tbl = get_table(he);
      int n = fixed_rows(tbl);
      int total = tbl.children_count();
      return total - n;
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
    void set_current_row( dom::element& table, dom::element& row, UINT keyboardStates, bool dblClick = false )
    {
      // get previously selected row:
      dom::element prev = get_current_row( table );
      if( prev.is_valid() )
      {
        if( prev != row ) 
          prev.set_state(0,STATE_CURRENT); // drop state flags
      }
      row.set_state(STATE_CURRENT,0); // set state flags
      row.scroll_to_view();
      table.post_event( dblClick? TABLE_ROW_DBL_CLICK:TABLE_ROW_CLICK, row.index(), row); 
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

    virtual BOOL on_mouse(HELEMENT he, HELEMENT target, UINT event_type, POINT pt, UINT mouseButtons, UINT keyboardStates )
    {
    
      if( event_type == MOUSE_WHEEL )
      {
        int dir = (int)mouseButtons;
        return on_scroll( he, target, dir < 0? SCROLL_STEP_PLUS : SCROLL_STEP_MINUS, 0, TRUE );
      }
    
      if( event_type != MOUSE_DOWN && event_type != MOUSE_DCLICK )
        return false;

      if(mouseButtons != MAIN_MOUSE_BUTTON) 
        return false;

      dom::element table = get_table(he);
      dom::element row = target_row(table, dom::element(target));

      if(row.is_valid()) // click on the row
      {
        if( (int)row.index() < (int)fixed_rows(table) )
        {
          // click on the header cell
          dom::element header_cell = target_header(row,target);
          if( header_cell.is_valid() )  
              on_column_click(table, header_cell);
          return true;
        }
        set_current_row(table, row, keyboardStates,  event_type == MOUSE_DCLICK);
      }
      return true; // as it is always ours then stop event bubbling
    }

    
    virtual BOOL on_key(HELEMENT he, HELEMENT target, UINT event_type, UINT code, UINT keyboardStates ) 
    { 
      if( event_type != KEY_DOWN )
        return false;
      
      dom::element table = get_table(he);
      switch( code )
      {
        case VK_DOWN: 
          {
             dom::element c = get_current_row( table );
             int idx = c.is_valid()? (c.index() + 1):fixed_rows(table);
             while( idx < (int)table.children_count() )
             {
                 dom::element row = table.child(idx);
                 if( aux::wcseq(row.get_style_attribute("display"),L"none" ))
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
                 if( aux::wcseq(row.get_style_attribute("display"),L"none" ))
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
          }
          return TRUE;

        case VK_NEXT:
          {
          }
          return TRUE;

        case VK_HOME:
          {
          }
          return TRUE;

        case VK_END:
          {
          }
          return TRUE;
      }
      return FALSE; 
    }
    
  
    virtual BOOL on_scroll( HELEMENT he, HELEMENT target, SCROLL_EVENTS cmd, INT pos, BOOL isVertical ) 
    { 
      if(!isVertical)
        return FALSE; 

      int visible_rows = num_data_rows(he);
      int row = first_row_idx;

      switch( cmd )
      {
        case SCROLL_HOME:       row = 0; break;
        case SCROLL_END:        row = num_rows - visible_rows + 1; break;
        case SCROLL_STEP_PLUS:  row += 1; break;
        case SCROLL_STEP_MINUS: row -= 1; break;
        case SCROLL_PAGE_PLUS:  row += visible_rows; break;
        case SCROLL_PAGE_MINUS: row -= visible_rows; break;
        case SCROLL_POS:        row = pos; break;
        default: return FALSE; 
      }

      row = aux::limit(row, 0, num_rows - visible_rows + 1);
      
      if( row == first_row_idx ) return TRUE;

      first_row_idx = row;

      dom::element tbl = get_table(he);
      tbl.update(); // establish update 'umbrella'

      get_rows_data(he);

      dom::scrollbar sb = get_v_scrollbar(he);
      sb.set_values(first_row_idx, 0, num_rows, num_data_rows(he), 1);

      HTMLayoutUpdateWindow(tbl.get_element_hwnd(TRUE));
      
      return TRUE;

    }

    virtual BOOL on_event (HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason )
    { 
       if( type != INIT_DATA_VIEW ) return FALSE; 

       first_row_idx = 0;
       get_rows_data( he ); 

       dom::scrollbar sb = get_v_scrollbar(he);
       sb.set_values(first_row_idx, 0, num_rows, num_data_rows(he), 1);

       return TRUE;
    }
      

};


struct virtual_grid_factory: public behavior
{
    virtual_grid_factory(): behavior(HANDLE_BEHAVIOR_EVENT, "virtual-grid") {}

    // this behavior has unique instance for each element it is attached to
    virtual event_handler* attach (HELEMENT /*he*/ ) 
    { 
      return new virtual_grid(); 
    }
};

// instantiating and attaching it to the global list
virtual_grid_factory  virtual_grid_factory;



}






