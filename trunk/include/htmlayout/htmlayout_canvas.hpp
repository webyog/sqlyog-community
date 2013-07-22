/*
 * Terra Informatica Lightweight Embeddable HTMLayout control
 * http://terrainformatica.com/htmlayout
 * 
 * struct canvas is an implementation of so called drawing behavior.
 * It is an abstract class - draw() function needs to be implemented.
 * 
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 * 
 * 
 * (C) 2003-2008, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

#ifndef __htmlayout_canvas_h__
#define __htmlayout_canvas_h__

#include "htmlayout.h"
#include "htmlayout_aux.h"
#include "htmlayout_graphin.h"
#include "htmlayout_behavior.hpp"

namespace htmlayout
{

  // canvas is a drawing behavior
  // see behavior_clock_graphin.cpp for the incarnation sample 
  struct canvas: event_handler
  {
    image*      surface;
    DRAW_EVENTS where;
    bool        default_draw;
    bool        redraw;
    canvas ( 
          UINT        subsriptions, // what events to handle
          DRAW_EVENTS layer, // which layer to draw
          bool        draw_default = false // true if draws this layer completely, false if it needs dafault drawing on that layer
          ): event_handler(subsriptions | HANDLE_DRAW), surface(0), where(layer), redraw(false), default_draw(draw_default) {}

    virtual ~canvas() { delete surface; }

    virtual void detached  (HELEMENT /*he*/ ) 
    { 
      delete this;
    } 

    virtual BOOL on_draw   (HELEMENT he, UINT draw_type, HDC hdc, const RECT& rc ) 
    { 
      if( (DRAW_EVENTS)draw_type != where )
        return FALSE; // do default draw
      int w = rc.right - rc.left;
      int h = rc.bottom - rc.top;
      if( !surface )
      {
        surface = image::create(w,h);
        redraw = true;
      }
      else if( w != surface->width() || h != surface->height() )
      {
        delete surface;
        surface = image::create(w,h);
        redraw = true;
      }
      else if(redraw)
        surface->clear();

      if( redraw )
      {
        graphics gx(surface);
        draw( he, gx, w, h );
        redraw = false;
      }
      surface->blit( hdc, rc.left, rc.top );

      return default_draw? TRUE : FALSE;  
    }

    // ask the canvas to 1) clear the surface and to 2) call draw() on it
    void request_redraw( HELEMENT he, bool force_update = false )
    {
        redraw = true;
        dom::element(he).update(force_update?REDRAW_NOW:0); 
    }

    // overrridable, that needs to draw something useful using gx 
    virtual void draw( HELEMENT he, graphics& gx, UINT width, UINT height ) = 0;


  };

  template < class CANVAS >
    struct canvas_factory:  behavior
  {
    canvas_factory(const char* name): behavior(HANDLE_INITIALIZATION, name) {}

    // this behavior has unique instance for each element it is attached to
    virtual event_handler* attach (HELEMENT /*he*/ ) 
    { 
      return new CANVAS(); 
    }
  };
  //  e.g. 
  //   canvas_factory<graphin_clock> graphin_clock_factory("graphin-clock");


}

#endif