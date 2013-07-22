#include "behavior_aux.h"
#include "htmlayout_canvas.hpp"

#include <math.h>
#include <time.h>


namespace htmlayout 
{

/*
BEHAVIOR: chart
    goal: Renders chart. Demonstrates GRAPHIN interface (AGG backed graphics)
TYPICAL USE CASE:
    <img style="behavior:chart" >
SAMPLE:
*/

struct chart: public canvas
{
    typedef canvas super;
    
    int step;
    int steps;

    json::value data;
    
    // ctor
    chart():canvas(HANDLE_TIMER | HANDLE_BEHAVIOR_EVENT | HANDLE_METHOD_CALL, DRAW_CONTENT)
    {
      step = 0;
      steps = 32;
    }
    
    virtual void attached  (HELEMENT he ) 
    { 
      super::attached(he);
      dom::element el = he;
      if(el.visible())
        HTMLayoutSetTimer( he, 10 ); // animation timer      

      // get data for our chart:
      const wchar_t* data_selector = el.get_attribute("data");
      if( data_selector )
      {
        dom::element root = el.root();
        dom::element del = root.find_first(data_selector);
        if(del.is_valid())
          data = del.get_value(); // if it is <script type="application/json"> then the value is parsed automatically.
      }
    } 
  
    virtual void detached  (HELEMENT he ) 
    { 
      HTMLayoutSetTimer( he, 0 ); // remove timer if any
      data.clear();
      super::detached(he);
    } 

    // demonstrates calls from CSSS! script
    virtual BOOL on_script_call(HELEMENT he, LPCSTR name, UINT argc, json::value* argv, json::value& retval) 
    { 
      if(aux::streq(name, "animate"))
      {
        if( argc == 1 && argv[0].is_int() ) // we accept single parameter - n - number of steps.
          steps = argv[0].get(32);

        // simply call initial animation
        step = 0;
        attached(he);
        return TRUE; // done, method found
      }
      return FALSE; 
    }


    virtual BOOL on_timer  (HELEMENT he ) 
    { 
      dom::element el = he;
      if( !el.visible())
        return FALSE;

      step += 1;
      if( step >= steps  )
      {
        step = steps - 1;
        return FALSE; // done animation.
      }
     
      request_redraw(he,true);

      return TRUE; // keep going
    }

    virtual BOOL on_event (HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason ) 
    {
      if( type == VISIUAL_STATUS_CHANGED )
      {
        if(reason /* shown */)
        {
          step = 0;
          HTMLayoutSetTimer( he, 10 ); // set 10 milliseconds timer 
        }
        else
        {
          step = steps - 1;
          HTMLayoutSetTimer( he, 0 ); // remove timer
        }
      }
      return false;
    }

    virtual void draw_message( graphics& gx, UINT width, UINT height, aux::wchars text )
    {
        gx.text_alignment(TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER);
        gx.font("Arial", 17);
        gx.no_line();
        gx.fill_color(color(0xFF,0,0));
        gx.text(width / 2, height / 2, text);
    }

    // canvas::draw overridable
    virtual void draw( HELEMENT he, graphics& gx, UINT width, UINT height )
    { 
      if( !data.is_array() )
      {
        draw_message( gx, width, height, const_wchars("Data not found") );
        return; 
      }

      color black(0,0,0);

       // x axis
      gx.line_cap(LINE_CAP_BUTT);
      gx.line_color(black);
      gx.line( 0.5, height - 0.5, width - 0.5, height - 0.5 ); // 0.5 - to draw line in the middle of the pixel

      for( int n = 0; n < data.length(); ++n )
      {
        json::value bar_def = data[n];
        json::value color_v = bar_def[L"color"]; 
        json::value value_v = bar_def[L"value"];
        if( color_v.is_undefined() || value_v.is_undefined())
        {
          draw_message( gx, width, height, const_wchars("Bad data structure") );
          return; 
        }
        draw_bar(gx, width, height, n, data.length(), color(color_v.get(0)), value_v.get(1.0));
      }
    }

    void draw_bar( graphics& gx, UINT width, UINT height, int n_bar, int total_bars, color c, double val )
    {
        double bar_width = double(width) / (2*total_bars);
        double bar_height = double(height - 2) * val * (double(step)/double(steps-1));

        double bar_x1 = bar_width/2 + n_bar * 2 * bar_width;
        double bar_x2 = bar_x1 + bar_width;
        double bar_y2 = height - 0.5;
        double bar_y1 = bar_y2 - bar_height;

        color c_outline(0,0,0,0);

        gx.line_color(c_outline);
        gx.line_width(3);
        
        COLOR_STOP cs[3] = 
        {
          { gcolor(c) , 0.0f },
          { gcolor(0xff,0xff,0xff), 0.33f },
          { gcolor(c), 1.0f }
        };
        gx.fill_linear_gradient( bar_x1,bar_y1,bar_x2,bar_y1, cs, 3);

        gx.rectangle(bar_x1,bar_y1,bar_x2,bar_y2,6,6,0,0);
    }

    /*
    void draw_caption( HELEMENT he, graphics& gx, UINT width, UINT height, aux::wchars text )
    {
        gx.state_save();
          gx.text_alignment(TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER);
          gx.font("Times New Roman", 18);
          gx.no_line();
          gx.fill_linear_gradient( width/3,0,(2*width)/3, 0, color(0xDF,0,0), color(0,0,0x7F));
          gx.text(width / 2, height / 3, text);
        gx.state_restore();
    }*/

   
};

// instantiating and attaching it to the global list
canvas_factory<chart> graphin_chart_factory("chart");

} // htmlayout namespace


