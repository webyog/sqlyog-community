#include "behavior_aux.h"
#include "htmlayout_canvas.hpp"

#include <math.h>
#include <time.h>

#include "mm_file.h"


namespace htmlayout 
{

/*
BEHAVIOR: clock
    goal: Renders analog clock according to current tome. Demonstrates GRAPHIN interface (AGG backed up graphics)
TYPICAL USE CASE:
    <img style="behavior:graphin-clock">
SAMPLE:
*/

const double PI = 3.14159265358979323846;

struct graphin_clock: public canvas
{
    typedef canvas super;

    htmlayout::image* pimage;

    // ctor
    graphin_clock():canvas(HANDLE_TIMER | HANDLE_MOUSE | HANDLE_BEHAVIOR_EVENT, DRAW_CONTENT),pimage(0)
    {
    }
    ~graphin_clock()
    {
      delete pimage;
    }
    
    virtual void attached  (HELEMENT he ) 
    { 
      super::attached(he);
      dom::element el = he;
      if(el.visible())
        HTMLayoutSetTimer( he, 1000 ); // set one second timer      

      json::string url = el.url(L"clock-images/seconds-head.png");
      aux::wchars  url_chars = url;
      if( url_chars.like(L"file://*") )
        url_chars.prune(7);

      aux::mm_file imf;
      if( imf.open( url_chars.start ))
      {
         pimage = htmlayout::image::load( imf.bytes() );
      }
    } 
   
    virtual void detached  (HELEMENT he ) 
    { 
      HTMLayoutSetTimer( he, 0 ); // remove timer
      super::detached(he);
    } 

    // for set cursor testing purposes:
    virtual BOOL handle_mouse  (HELEMENT he, MOUSE_PARAMS& params ) 
    {
      if(params.cmd == MOUSE_MOVE)
        params.cursor_type = CURSOR_WAIT; 
      return FALSE;
    }

    virtual BOOL on_timer  (HELEMENT he ) 
    { 
      dom::element el = he;
      if( !el.visible())
        return FALSE;
      
      request_redraw(he);

      return TRUE; /*keep going*/ 
    }

    virtual BOOL on_event (HELEMENT he, HELEMENT target, BEHAVIOR_EVENTS type, UINT_PTR reason ) 
    {
      if( type == VISIUAL_STATUS_CHANGED )
      {
        if(reason /* shown */)
          HTMLayoutSetTimer( he, 1000 ); // set one second timer      
        else
          HTMLayoutSetTimer( he, 0 ); // remove timer
      }
      return false;
    }

    // canvas::draw overridable
    virtual void draw( HELEMENT he, graphics& gx, UINT width, UINT height )
    { 
      SYSTEMTIME st;
      GetLocalTime(&st);
      //Use GetSystemTime(&st); - if you need clocks in UTC or other TZ
      //you may use something like get_attribute("timezone")

      const wchar_t* caption = dom::element(he).get_attribute("caption");
      if( caption )
        draw_caption( he, gx, width, height, aux::chars_of(caption) );

      draw_clock_hand(he, gx,width,height, (st.wHour * 360.0) / 12.0 + st.wMinute / 2.0, 0);
      draw_clock_hand(he, gx,width,height, (st.wMinute * 360.0) / 60.0, 1);
      draw_clock_hand(he, gx,width,height, (st.wSecond * 360.0) / 60.0, 2);
    }

    void draw_clock_hand( HELEMENT he, graphics& gx, UINT sx, UINT sy, double angle_degree, int hand )
    {
       dom::element self(he);
       color c(255,0,0,0);
       int   hand_width_px;

       double radians = (2.0 * PI * (angle_degree - 90.0)) / 360.0 ;
       int radius = min( sy, sx ) / 2 - 16;

       gx.line_cap(LINE_CAP_ROUND);
     
       switch(hand)
       {
        case 0: // hours
           radius -= 24 ; 
           c = self.attribute("-hand-hours", c);
           if(c.transparent()) return;
           hand_width_px = self.attribute("-hand-hours-width", 5);
           gx.line_color(c);
           gx.line_width( hand_width_px );
           break;
        case 1: // minutes
           radius -= 12 ; 
           c = self.attribute("-hand-minutes", c);
           if(c.transparent()) return;
           hand_width_px = self.attribute("-hand-minutes-width", 3);
           gx.line_color(c);
           gx.line_width( hand_width_px  );
           break;
        case 2: // seconds
           c = self.attribute("-hand-seconds", c);
           if(c.transparent()) return;
           hand_width_px = self.attribute("-hand-seconds-width", 1);
           gx.line_color( c );
           gx.line_width( hand_width_px );
           break;
        default:
           assert(false);
       }

       double y = (sy / 2) + 0.5; // + 0.5 is to move it to the center of the pixel.
       double x = (sy / 2) + 0.5;

       double xe = x + int(cos(radians) * radius) + 0.5;
       double ye = y + int(sin(radians) * radius) + 0.5;

       gx.line( x, y, xe, ye );
       if( hand == 2 )
       {
         // circle on the end of seconds hand
         if(pimage)
         {
           int w = pimage->width();
           int h = pimage->height();
           gx.draw_image(pimage,xe - double(w)/2,ye - double(h)/2,w,h,0,0,w,h);
         }
         else // no image
           gx.circle( xe, ye, 4 );
       }
    }

    void draw_caption( HELEMENT he, graphics& gx, UINT width, UINT height, aux::wchars text )
    {
        gx.state_save();
          //gx.rotate(3.1415926 / 2, width / 2, height / 3); // for text rotation testing.
          gx.text_alignment(TEXT_ALIGN_CENTER, TEXT_ALIGN_CENTER);
          gx.font("Times New Roman", 18);
          gx.no_line();
          gx.fill_linear_gradient( width/3,0,(2*width)/3, 0, color(0xDF,0,0), color(0,0,0x7F));
          gx.text(width / 2, height / 3, text);
          //gx.rectangle(0,0,width / 2, height / 3);
        gx.state_restore();
    }

   
};


// instantiating and attaching it to the global list
canvas_factory<graphin_clock> graphin_clock_factory("graphin-clock");

} // htmlayout namespace


