#include "behavior_aux.h"

namespace htmlayout 
{
 
/** sample of data source behavior 
 *  see behavior:virtual-grid.
 *
 *  See: html_samples/grid/virtual-grid.htm
 *
 **/
    
struct sample_data_source: public behavior
{
    // ctor
    sample_data_source(const char* name = "sample-data-source"): behavior( HANDLE_BEHAVIOR_EVENT, name) {}

    virtual BOOL on_event (HELEMENT, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason )
    { 
       if( type != ROWS_DATA_REQUEST ) return FALSE; 
       // this is data source - we handle here only ROWS_DATA_REQUESTs
       return handle_data_requst( target, *((DATA_ROWS_PARAMS*)reason));
    }

    virtual BOOL handle_data_requst( HELEMENT table_el, DATA_ROWS_PARAMS& drp ) 
    { 
      // absolutely nothin' spectacular here,
      // fill table cells by data from some external source.
      
      drp.totalRecords = 300;

      dom::element tbl = table_el;

      int i = drp.firstRecord;

      for(unsigned int n = drp.firstRowIdx ; n <= drp.lastRowIdx; ++n, ++i )
      {
        dom::element row = tbl.child(n);
        wchar_t buffer[256];
        for( unsigned int c = 0; c < row.children_count(); ++c )
        {
          swprintf(buffer,L"row %d, col %d", i, c); //\x4E00\x4E01\x4E02\x4E03  
          dom::element cell = row.child(c);
          cell.set_text(buffer);
        }
      }
      return TRUE;
    }
};

sample_data_source sample_data_source_instance;

}