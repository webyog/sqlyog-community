/*
 * Terra Informatica Lightweight Embeddable HTMLayout control
 * http://terrainformatica.com/htmlayout
 * 
 * Graphin interface, AGG (http://www.antigrain.com) based graphics primitives
 * 
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 * 
 * 
 * (C) 2003-2008, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

#ifndef __htmlayout_graphin_h__
#define __htmlayout_graphin_h__

#include "htmlayout.h"
#include "htmlayout_aux.h"

/*!\file
\brief Graphin support, AGG based graphics primitives
*/

#if defined(WIN32) || defined(_WIN32_WCE)
  #define WIN32_LEAN_AND_MEAN   // exclude unused rarely used stuff from Windows headers
  #include <windows.h>
  typedef HDC HPLATFORMGFX;
#elif defined(XWINDOW)
  #include <X11/Xlib.h>
  typedef struct _HDC_tipa
  {
    Display* d;
    Window   w;
    int      gc;
  } *HPLATFORMGFX;
#endif

#if defined(__GNUC__) || defined(STATIC_LIB)
  #define GRAPHIN_CALL(n) (* n)
  #define GRAPHIN_CALLC   
#elif defined( _MSC_VER )
  #define GRAPHIN_CALL(n) (__stdcall *n) 
  #define GRAPHIN_CALLC __stdcall
#endif


struct graphin_graphics;
struct graphin_image;

typedef graphin_graphics* HGFX;
typedef graphin_image*    HIMG;

#ifndef BYTE
  typedef unsigned char   BYTE;
#endif
#ifndef UINT
  typedef unsigned int    UINT;
#endif
#ifndef INT
  typedef int             INT;
#endif
#ifndef BOOL
  typedef int             BOOL;
#endif

#ifdef UNDER_CE
  typedef float REAL; 
#else
  typedef double REAL; 
#endif

typedef REAL POS;         // position
typedef REAL DIM;         // dimension
typedef REAL ANGLE;       // angle (radians)
typedef unsigned int COLOR; // color

struct COLOR_STOP
{
  COLOR color; 
  float offset; // 0.0 ... 1.0
};

enum GRAPHIN_RESULT
{
  GRAPHIN_PANIC = -1, // e.g. not enough memory
  GRAPHIN_OK = 0,
  GRAPHIN_BAD_PARAM = 1,  // bad parameter
  GRAPHIN_FAILURE = 2,    // operation failed, e.g. restore() without save()
  GRAPHIN_NOTSUPPORTED = 3 // the platform does not support requested feature
};

enum DRAW_PATH_MODE
{
  FILL_ONLY,
  STROKE_ONLY,
  FILL_AND_STROKE,
  FILL_BY_LINE_COLOR
};

enum TEXT_ALIGNMENT
{
  TEXT_ALIGN_TOP,
  TEXT_ALIGN_BOTTOM,
  TEXT_ALIGN_CENTER,
  TEXT_ALIGN_BASELINE,
  TEXT_ALIGN_RIGHT = TEXT_ALIGN_TOP,
  TEXT_ALIGN_LEFT = TEXT_ALIGN_BOTTOM
};

enum LINE_JOIN_TYPE
{
  JOIN_MITER = 0,
  JOIN_MITER_REVERT = 1,
  JOIN_ROUND = 2,
  JOIN_BEVEL = 3,
  JOIN_MITER_ROUND = 4,
};

enum LINE_CAP_TYPE
{
  LINE_CAP_BUTT = 0,
  LINE_CAP_SQUARE = 1,
  LINE_CAP_ROUND = 2,
};

enum GRAPHIN_BLEND_MODE
{
  BLEND_CLEAR,
  BLEND_SRC,
  BLEND_DST,
  BLEND_SRCOVER,
  BLEND_DSTOVER,
  BLEND_SRCIN,
  BLEND_DSTIN,
  BLEND_SRCOUT,
  BLEND_DSTOUT,
  BLEND_SRCATOP,
  BLEND_DSTATOP,
  BLEND_XOR,
  BLEND_ADD,
  BLEND_SUB,
  BLEND_MULTIPLY,
  BLEND_SCREEN,
  BLEND_OVERLAY,
  BLEND_DARKEN,
  BLEND_LIGHTEN,
  BLEND_COLORDODGE,
  BLEND_COLORBURN,
  BLEND_HARDLIGHT,
  BLEND_SOFTLIGHT,
  BLEND_DIFFERENCE,
  BLEND_EXCLUSION,
  BLEND_CONTRAST,
  BLEND_INVERT,
  BLEND_INVERTRGB,

  BLEND_ALPHA,

  BLEND_COUNT,
};


typedef BOOL GRAPHIN_CALLC image_write_function(LPVOID prm, LPBYTE data, UINT data_length);

struct Graphin
{
// image primitives
  GRAPHIN_RESULT 
        GRAPHIN_CALL(imageCreate)( UINT width, UINT height, HIMG* poutImg );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(imageRelease)( HIMG himg );

  GRAPHIN_RESULT
        GRAPHIN_CALL(imageGetInfo)( HIMG himg,
             BYTE** data,
             UINT* width,
             UINT* height,
             INT* stride,
             UINT* pixel_format);

  GRAPHIN_RESULT
        GRAPHIN_CALL(imageClear)( HIMG himg );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(imageBlit)( HPLATFORMGFX dst, INT dst_x, INT dst_y,
                    HIMG src, INT src_x, INT src_y,
                    INT width, INT height, BOOL blend );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(imageLoad)( const BYTE* bytes, UINT num_bytes, HIMG* pout_img ); // load png/jpeg/etc. image from stream of bytes

  GRAPHIN_RESULT 
        GRAPHIN_CALL(imageSave) // save png/jpeg/etc. image to stream of bytes
          ( HIMG himg, 
          image_write_function* pfn, void* prm, /* function and its param passed "as is" */
          UINT bpp /*24,32 if alpha needed*/,  
          UINT type /* 0 - png, 1 - jpg*/,
          UINT quality /*  only for jpeg, 10 - 100 */ );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(imageHandle) // returns HBITMAP of the image, it is always 32bpp DIB handle.
          ( HIMG himg, 
            HBITMAP* phbmp );


  // SECTION: graphics primitives and drawing operations

  // create color value
  COLOR 
        GRAPHIN_CALL(RGBT)(UINT red, UINT green, UINT blue, UINT transparency /*= 0*/);
  // Note: transparent color (no-color value) is rgba(?, ?, ?, 0xff);

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gCreate)(HIMG img, HGFX* pout_gfx );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gRelease) (HGFX gfx);

// Draws line from x1,y1 to x2,y2 using current lineColor and lineGradient.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gLine) ( HGFX hgfx, POS x1, POS y1, POS x2, POS y2 );

// Draws triangle using current lineColor/lineGradient and fillColor/fillGradient.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gTriangle) ( HGFX hgfx, POS x1, POS y1, POS x2, POS y2, POS x3, POS y3 );

// Draws rectangle using current lineColor/lineGradient and fillColor/fillGradient with (optional) rounded corners.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gRectangle) ( HGFX hgfx, POS x1, POS y1, POS x2, POS y2 );

// Draws rounded rectangle using current lineColor/lineGradient and fillColor/fillGradient with (optional) rounded corners.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gRoundedRectangle) ( HGFX hgfx, POS x1, POS y1, POS x2, POS y2, DIM* radii8 /*DIM[8] - four rx/ry pairs */);

// Draws circle or ellipse using current lineColor/lineGradient and fillColor/fillGradient.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gEllipse) ( HGFX hgfx, POS x, POS y, POS rx, POS ry );

// Draws closed arc using current lineColor/lineGradient and fillColor/fillGradient.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gArc) ( HGFX hgfx, POS x, POS y, POS rx, POS ry, ANGLE start, ANGLE sweep );

// Draws star.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gStar) ( HGFX hgfx, POS x, POS y, POS r1, POS r2, ANGLE start, UINT rays );

// Closed polygon.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gPolygon) ( HGFX hgfx, POS* xy, UINT num_points );

  // Polyline.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gPolyline) ( HGFX hgfx, POS* xy, UINT num_points );

// SECTION: Path operations

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gOpenPath) ( HGFX hgfx );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gMoveTo) ( HGFX hgfx, POS x, POS y, BOOL relative );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gLineTo) ( HGFX hgfx, POS x, POS y, BOOL relative );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gHLineTo) ( HGFX hgfx, POS x, BOOL relative );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gVLineTo) ( HGFX hgfx, POS y, BOOL relative );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gArcTo) ( HGFX hgfx, POS x, POS y, ANGLE angle, POS rx, POS ry, BOOL is_large_arc, BOOL sweep_flag, BOOL relative );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gEllipseTo) ( HGFX hgfx, POS x, POS y, POS rx, POS ry, BOOL clockwise );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gQuadraticCurveTo) ( HGFX hgfx, POS xc, POS yc, POS x, POS y, BOOL relative );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gBezierCurveTo) ( HGFX hgfx, POS xc1, POS yc1, POS xc2, POS yc2, POS x, POS y, BOOL relative );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gClosePath) ( HGFX hgfx );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gDrawPath) ( HGFX hgfx, DRAW_PATH_MODE dpm );

// end of path opearations

// SECTION: affine tranformations:

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gRotate) ( HGFX hgfx, ANGLE radians, POS* cx /*= 0*/, POS* cy /*= 0*/ );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gTranslate) ( HGFX hgfx, POS cx, POS cy );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gScale) ( HGFX hgfx, double x, double y );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gSkew) ( HGFX hgfx, double dx, double dy );

  // all above in one shot
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gTransform) ( HGFX hgfx, POS m11, POS m12, POS m21, POS m22, POS dx, POS dy );

// end of affine tranformations.

// SECTION: state save/restore

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gStateSave) ( HGFX hgfx );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gStateRestore) ( HGFX hgfx );

// end of state save/restore

// SECTION: drawing attributes

  // set line width for subsequent drawings.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gLineWidth) ( HGFX hgfx, DIM width );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gLineJoin) ( HGFX hgfx, LINE_JOIN_TYPE type );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gLineCap) ( HGFX hgfx, LINE_CAP_TYPE type);

  //GRAPHIN_RESULT GRAPHIN_CALL
  //      (*gNoLine ( HGFX hgfx ) { gLineWidth(hgfx,0.0); }

  // color for solid lines/strokes
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gLineColor) ( HGFX hgfx, COLOR c);

  // color for solid fills
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gFillColor) ( HGFX hgfx, COLOR color );

//inline void
//      graphics_no_fill ( HGFX hgfx ) { graphics_fill_color(hgfx, graphics_rgbt(0,0,0,0xFF)); }

  // setup parameters of linear gradient of lines.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gLineGradientLinear)( HGFX hgfx, POS x1, POS y1, POS x2, POS y2, COLOR_STOP* stops, UINT nstops );

  // setup parameters of linear gradient of fills.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gFillGradientLinear)( HGFX hgfx, POS x1, POS y1, POS x2, POS y2, COLOR_STOP* stops, UINT nstops );

  // setup parameters of line gradient radial fills.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gLineGradientRadial)( HGFX hgfx, POS x, POS y, DIM r, COLOR_STOP* stops, UINT nstops );

  // setup parameters of gradient radial fills.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gFillGradientRadial)( HGFX hgfx, POS x, POS y, DIM r, COLOR_STOP* stops, UINT nstops );

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gFillMode) ( HGFX hgfx, BOOL even_odd /* false - fill_non_zero */ );

// SECTION: text

  // define font attributes for all subsequent text operations.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gSetFont)( HGFX hgfx, const char* name, DIM size, BOOL bold/* = false*/, BOOL italic/* = false*/, ANGLE angle /*= 0*/);

  // draw text at x,y with text alignment
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gText) ( HGFX hgfx, POS x, POS y, const wchar_t* text, UINT text_length);

  // calculates width of the text string
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gTextWidth) ( HGFX hgfx, const wchar_t* text, UINT text_length, DIM* out_width);

  // returns height and ascent of the font.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gFontMetrics)( HGFX hgfx, DIM* out_height, DIM* out_ascent );


  // calculates width of the text string
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gTextAlignment)( HGFX hgfx, TEXT_ALIGNMENT x, TEXT_ALIGNMENT y);

  // SECTION: image rendering

  // draws img onto the graphics surface with current transformation applied (scale, rotation).
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gDrawImage) ( HGFX hgfx, HIMG himg, POS x, POS y,
                              DIM* w /*= 0*/, DIM* h /*= 0*/, UINT* ix /*= 0*/, UINT* iy /*= 0*/, UINT* iw /*= 0*/, UINT* ih /*= 0*/ );

  // blits image bits onto underlying pixel buffer. no affine transformations.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gBlitImage) ( HGFX hgfx, HIMG himg, POS x, POS y, UINT* ix /*= 0*/, UINT* iy /*= 0*/, UINT* iw /*= 0*/, UINT* ih /*= 0*/ );

  // blends image bits with bits of the buffer. no affine transformations.
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gBlendImage) ( HGFX hgfx, HIMG himg, POS x, POS y, UINT opacity /*= 0xff*/, UINT* ix /*= 0*/, UINT* iy /*= 0*/, UINT* iw /*= 0*/, UINT* ih /*= 0*/ );

  // SECTION: coordinate space

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gWorldToScreen) ( HGFX hgfx, POS* inout_x, POS* inout_y);

  //inline GRAPHIN_RESULT
  //      graphics_world_to_screen ( HGFX hgfx, POS* length)
  //{
  //   return graphics_world_to_screen ( hgfx, length, 0);
  //}

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gScreenToWorld) ( HGFX hgfx, POS* inout_x, POS* inout_y);

  //inline GRAPHIN_RESULT
  //      graphics_screen_to_world ( HGFX hgfx, POS* length)
  //{
  //   return graphics_screen_to_world (hgfx, length, 0);
  //}

// SECTION: clipping

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gSetClipBox) ( HGFX hgfx, POS x1, POS y1, POS x2, POS y2);

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gGetClipBox) ( HGFX hgfx, POS* x1, POS* y1, POS* x2, POS* y2);

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gPointInClipBox) ( HGFX hgfx, POS x, POS y, BOOL* yes);

  GRAPHIN_RESULT 
        GRAPHIN_CALL(gRectInClipBox) ( HGFX hgfx, POS x1, POS y1, POS x2, POS y2, BOOL* yes);
        
// blending mode        
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gSetBlendMode) ( HGFX gfx, GRAPHIN_BLEND_MODE mode );
  GRAPHIN_RESULT 
        GRAPHIN_CALL(gGetBlendMode) ( HGFX gfx, GRAPHIN_BLEND_MODE* p_mode );
        
// antialiasing gamma
  GRAPHIN_RESULT
          GRAPHIN_CALL(gSetGamma)( HGFX gfx, REAL gamma_value );
  GRAPHIN_RESULT
          GRAPHIN_CALL(gGetGamma)( HGFX gfx, REAL* p_gamma_value );
          
// --- Dashes -------------------------------------------------------------

  // dahed lines

  // Set dash length/gap for strokes
  GRAPHIN_RESULT
          GRAPHIN_CALL(gSetDash)( HGFX gfx, POS dash_len, POS gap_len );

  // remove dashes (stroke styling)
  GRAPHIN_RESULT
          GRAPHIN_CALL(gResetDash)( HGFX gfx );

  // set dash start offset
  GRAPHIN_RESULT
          GRAPHIN_CALL(gSetDashOffset)( HGFX gfx, POS dash_start );

  // get dash start offset
  GRAPHIN_RESULT
          GRAPHIN_CALL(gGetDashOffset)( HGFX gfx, POS* p_dash_start );

  // --- Dashes (end) --------------------------------------------------------
          
  // Set pattern fill (by image) for all consequent DrawPath's 
  GRAPHIN_RESULT
          GRAPHIN_CALL(gFillPattern)( HGFX hgfx, HIMG pattern, UINT opacity, INT offset_x, INT offset_y );
};

typedef Graphin* PGraphin;

/** Get Graphin API
 *
 *  returns ptr to Graphin API functions.
 *  See: htmlayout_graphin.h
 *
 **/

EXTERN_C PGraphin       HLAPI HTMLayoutGetGraphin();

/** Render DOM element on the image
 * \param[in] himg \b HIMG, image where to render the element.
 * \param[in] where \b POINT, location where to render, image coordinates.
 * \param[in] what \b HELEMENT, element to render.
 * \param[in] how \b UINT, flags.
 **/

EXTERN_C GRAPHIN_RESULT HLAPI HTMLayoutRenderElement(HIMG himg, POINT where, HELEMENT what, UINT how);
// ATTN: HTMLayoutRenderElement is not implemented yet.


#if defined(__cplusplus) && !defined( PLAIN_API_ONLY )

namespace htmlayout
{

  inline Graphin* gapi()
  {
    static Graphin* _gapi = HTMLayoutGetGraphin();
    return _gapi;
  }

  struct writer
  {
    virtual bool write( aux::bytes data ) = 0; // redefine to do actual writing of data.start/data.length
    static BOOL GRAPHIN_CALLC image_write_function(LPVOID prm, LPBYTE data, UINT data_length)
    {
      writer* pw = (writer* )prm;
      return pw->write( aux::bytes(data,data_length) );
    }
  };

  class image
  {
    friend class graphics;
    HIMG himg;
    UINT sx;
    UINT sy;

    image(HIMG h = 0, UINT sizex = 0, UINT sizey = 0 ): himg(h),sx(sizex),sy(sizey) {}
  public:
    static image* create( UINT sizex, UINT sizey )
    {
      HIMG himg;
      GRAPHIN_RESULT r = gapi()->imageCreate( sizex, sizey, &himg); assert(r == GRAPHIN_OK); r;
      if( himg )
        return new image( himg, sizex, sizey );
      return 0;
    }
    static image* load( aux::bytes data ) // loads image from png or jpeg enoded data
    {
      HIMG himg;
      GRAPHIN_RESULT r = gapi()->imageLoad( data.start, data.length, &himg); assert(r == GRAPHIN_OK); r;
      if( himg )
      {
        BYTE* data; UINT width; UINT height; INT stride; UINT pixel_format;
        gapi()->imageGetInfo( himg, &data, &width, &height, &stride, &pixel_format);
        return new image( himg, width, height );
      }
      return 0;
    }
    void save( writer& w, uint quality = 0 /*JPEG qquality: 20..100, if 0 - PNG */ ) // save image as png or jpeg enoded data
    {
      GRAPHIN_RESULT r;
      if( quality >= 1 && quality <= 100 ) // jpeg
        r = gapi()->imageSave( himg, writer::image_write_function, &w, 24, 1, quality ); 
      else // png
        r = gapi()->imageSave( himg, writer::image_write_function, &w, 32, 0, 0 ); 
      assert(r == GRAPHIN_OK); r;
    }

    HBITMAP handle() // GDI HBITMAP of the 32bpp DIB
    {
      GRAPHIN_RESULT r; HBITMAP h = 0;
      r = gapi()->imageHandle( himg, &h ); 
      assert(r == GRAPHIN_OK); r;
      return h;
    }

    /* example of use of the writer:
    struct _: public writer // helper functor
    {
      pod::byte_buffer& bb;
      inline _( pod::byte_buffer& abb ): bb(abb) {}
      inline virtual bool write( aux::bytes data ) { bb.push(data.start, data.length); return true; }
    };

    void save( pod::byte_buffer& bb, uint quality = 0)
    {
        save(_(bb),quality);
    }*/

    ~image() 
    {
      GRAPHIN_RESULT r = gapi()->imageRelease( himg ); assert(r == GRAPHIN_OK); r;
    }
    
    int width() const { return sx; }
    int height() const { return sy; }

    void clear()
    {
        GRAPHIN_RESULT r = gapi()->imageClear(himg); 
        assert(r == GRAPHIN_OK); r;
    }

    // bitblit it to the hdc
    void blit( HPLATFORMGFX hdc, INT x, INT y )
    {
        GRAPHIN_RESULT r = gapi()->imageBlit(hdc, x, y, himg, 0, 0, sx, sy, TRUE); 
        assert(r == GRAPHIN_OK); r;
    }

  };

  inline COLOR gcolor(const color& c) { return gapi()->RGBT( c.r, c.g, c.b, c.t ); }
  inline COLOR gcolor(uint r, uint g, uint b, uint t = 0) { return gapi()->RGBT( r, g, b, t ); }

  class graphics
  {
    HGFX hgfx;  
  public:

    graphics( image* img ): hgfx(0) 
    {
      assert(img && img->himg); if( !img || !img->himg) return;
      gapi()->gCreate(img->himg,&hgfx);
    }
    ~graphics() { if(hgfx) gapi()->gRelease(hgfx); }

    // Draws line from x1,y1 to x2,y2 using current line_color and line_gradient.
    void line ( POS x1, POS y1, POS x2, POS y2 )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gLine( hgfx, x1, y1, x2, y2 ); 
      assert(r == GRAPHIN_OK); r;
    }

    // Draws triangle using current lineColor/lineGradient and fillColor/fillGradient.
    void triangle ( POS x1, POS y1, POS x2, POS y2, POS x3, POS y3 )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gTriangle( hgfx, x1, y1, x2, y2, x3, y3 ); 
      assert(r == GRAPHIN_OK); r;
    }

    // Draws rectangle using current lineColor/lineGradient and fillColor/fillGradient 
    void rectangle ( POS x1, POS y1, POS x2, POS y2 )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gRectangle( hgfx, x1, y1, x2, y2 ); 
      assert(r == GRAPHIN_OK); r;
    }

    // Draws rounded rectangle using current lineColor/lineGradient and fillColor/fillGradient with rounded corners.
    void rectangle ( POS x1, POS y1, POS x2, POS y2, DIM rAll )
    {
      DIM rad[8] = { rAll, rAll, rAll, rAll, rAll, rAll, rAll, rAll};
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gRoundedRectangle( hgfx, x1, y1, x2, y2, rad ); 
      assert(r == GRAPHIN_OK); r;
    }

    void rectangle ( POS x1, POS y1, POS x2, POS y2, DIM rTopLeft, DIM rTopRight, DIM rBottomRight, DIM rBottomLeft )
    {
      DIM rad[8] = { rTopLeft, rTopLeft, rTopRight, rTopRight, rBottomRight, rBottomRight, rBottomLeft, rBottomLeft };
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gRoundedRectangle( hgfx, x1, y1, x2, y2, rad ); 
      assert(r == GRAPHIN_OK); r;
    }


    // Draws circle or ellipse using current lineColor/lineGradient and fillColor/fillGradient.
    void ellipse ( POS x, POS y, POS rx, POS ry )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gEllipse( hgfx, x, y, rx, ry ); 
      assert(r == GRAPHIN_OK); r;
    }
    void circle ( POS x, POS y, POS radii )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gEllipse( hgfx, x, y, radii, radii ); 
      assert(r == GRAPHIN_OK); r;
    }

    // Draws closed arc using current lineColor/lineGradient and fillColor/fillGradient.
    void arc ( POS x, POS y, POS rx, POS ry, ANGLE start, ANGLE sweep )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gArc( hgfx, x, y, rx, ry, start, sweep ); 
      assert(r == GRAPHIN_OK); r;
    }

    // Draws star.
    void star ( POS x, POS y, POS r1, POS r2, ANGLE start, UINT rays )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gStar( hgfx, x, y, r1, r2, start, rays ); 
      assert(r == GRAPHIN_OK); r;
    }

    // Draws closed polygon using current lineColor/lineGradient and fillColor/fillGradient.
    void polygon ( POS* xy, UINT num_points )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gPolygon( hgfx, xy, num_points ); 
      assert(r == GRAPHIN_OK); r;
    }

    // you bet
    void polyline ( POS* xy, unsigned int num_points )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gPolyline( hgfx, xy, num_points ); 
      assert(r == GRAPHIN_OK); r;
    }

    // SECTION: Path operations

    void open_path()
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gOpenPath( hgfx ); 
      assert(r == GRAPHIN_OK); r;
    }

    void move_to ( POS x, POS y, bool relative )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gMoveTo( hgfx, x, y, relative ); 
      assert(r == GRAPHIN_OK); r;
    }

    void line_to ( POS x, POS y, bool relative )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gLineTo( hgfx, x, y, relative ); 
      assert(r == GRAPHIN_OK); r;
    }

    void hline_to ( POS x, bool relative )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gHLineTo( hgfx, x, relative ); 
      assert(r == GRAPHIN_OK); r;
    }

    void vline_to ( POS y, bool relative )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gVLineTo( hgfx, y, relative ); 
      assert(r == GRAPHIN_OK); r;
    }

    void arc_to( POS x, POS y, ANGLE angle, POS rx, POS ry, bool is_large_arc, bool sweep_flag, bool relative )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gArcTo( hgfx, x, y, angle, rx, ry, is_large_arc, sweep_flag, relative ); 
      assert(r == GRAPHIN_OK); r;
    }

    void ellipse_to( POS x, POS y, POS rx, POS ry, bool clockwise )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gEllipseTo( hgfx, x, y, rx, ry, clockwise ); 
      assert(r == GRAPHIN_OK); r;
    }

    void quadratic_curve_to ( POS xc, POS yc, POS x, POS y, bool relative )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gQuadraticCurveTo( hgfx, xc, yc, x, y, relative ); 
      assert(r == GRAPHIN_OK); r;
    }

    void bezier_curve_to ( POS xc1, POS yc1, POS xc2, POS yc2, POS x, POS y, bool relative )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gBezierCurveTo( hgfx, xc1, yc1, xc2, yc2, x, y, relative ); 
      assert(r == GRAPHIN_OK); r;
    }

    void close_path()
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gClosePath( hgfx ); 
      assert(r == GRAPHIN_OK); r;
    }

    void draw_path(DRAW_PATH_MODE dpm)
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gDrawPath( hgfx, dpm ); 
      assert(r == GRAPHIN_OK); r;
    }

    // end of path opearations

    // SECTION: affine tranformations:

    void rotate ( ANGLE radians )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gRotate( hgfx, radians, 0, 0 ); 
      assert(r == GRAPHIN_OK); r;
    }

    void rotate ( ANGLE radians, POS center_x, POS center_y )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gRotate( hgfx, radians, &center_x, &center_y ); 
      assert(r == GRAPHIN_OK); r;
    }

    void translate ( POS cx, POS cy )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gTranslate( hgfx, cx, cy ); 
      assert(r == GRAPHIN_OK); r;
    }

    void scale ( REAL x, REAL y )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gScale( hgfx, x, y ); 
      assert(r == GRAPHIN_OK); r;
    }

    void skew ( REAL dx, REAL dy )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gSkew( hgfx, dx, dy ); 
      assert(r == GRAPHIN_OK); r;
    }

    // all above in one shot
    void transform ( POS m11, POS m12, POS m21, POS m22, POS dx, POS dy )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gTransform( hgfx, m11, m12, m21, m22, dx, dy ); 
      assert(r == GRAPHIN_OK); r;
    }

    // end of affine tranformations.

    // SECTION: state save/restore

    void state_save ( )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gStateSave( hgfx ); 
      assert(r == GRAPHIN_OK); r;
    }

    void state_restore ( )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gStateRestore( hgfx ); 
      assert(r == GRAPHIN_OK); r;
    }

    // end of state save/restore

    // SECTION: drawing attributes

    // set line width for subsequent drawings.
    void line_width ( DIM width )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gLineWidth( hgfx, width ); 
      assert(r == GRAPHIN_OK); r;
    }

    void no_line ( ) 
    { 
      line_width(0.0); 
    }

    void line_color ( color c ) 
    { 
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gLineColor( hgfx, gcolor(c)); 
      assert(r == GRAPHIN_OK); r;
    }

    void line_cap(LINE_CAP_TYPE ct)
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gLineCap( hgfx, ct); 
      assert(r == GRAPHIN_OK); r;
    }

    void line_join(LINE_JOIN_TYPE jt)
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gLineJoin( hgfx, jt); 
      assert(r == GRAPHIN_OK); r;
    }

    // color for solid fills
    void fill_color ( color c )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gFillColor( hgfx, gcolor(c)); 
      assert(r == GRAPHIN_OK); r;
    }

    void no_fill ( ) 
    { 
      fill_color ( color(0,0,0,0xff) );
    }

    // setup parameters of linear gradient of lines.
    void line_linear_gradient( POS x1, POS y1, POS x2, POS y2, COLOR_STOP* stops, UINT nstops )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gLineGradientLinear( hgfx, x1, y1, x2, y2, stops, nstops ); 
      assert(r == GRAPHIN_OK); r;
    }
    void line_linear_gradient( POS x1, POS y1, POS x2, POS y2, color c1, color c2 )
    {
      COLOR_STOP stops[2] = { {gcolor(c1), 0.0}, {gcolor(c2), 1.0} };
      fill_linear_gradient( x1, y1, x2, y2, stops, 2 );
    }

    // setup parameters of linear gradient of fills.
    void fill_linear_gradient( POS x1, POS y1, POS x2, POS y2, COLOR_STOP* stops, UINT nstops )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gFillGradientLinear( hgfx, x1, y1, x2, y2, stops, nstops); 
      assert(r == GRAPHIN_OK); r;
    }

    void fill_linear_gradient( POS x1, POS y1, POS x2, POS y2, color c1, color c2 )
    {
      COLOR_STOP stops[2] = { {gcolor(c1), 0.0}, {gcolor(c2), 1.0} };
      fill_linear_gradient( x1, y1, x2, y2, stops, 2 );
    }

    // setup parameters of line gradient radial fills.
    void line_radial_gradient( POS x, POS y, DIM radii, COLOR_STOP* stops, UINT nstops )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gFillGradientRadial( hgfx, x, y, radii, stops, nstops); 
      assert(r == GRAPHIN_OK); r;
    }

    void line_radial_gradient( POS x, POS y, DIM radii, color c1, color c2 )
    {
      COLOR_STOP stops[2] = { {gcolor(c1), 0.0}, {gcolor(c2), 1.0} };
      line_radial_gradient( x, y, radii, stops, 2 );
    }

    // setup parameters of gradient radial fills.
    void fill_radial_gradient( POS x, POS y, DIM radii, COLOR_STOP* stops, UINT nstops )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gFillGradientRadial( hgfx, x, y, radii, stops, nstops); 
      assert(r == GRAPHIN_OK); r;
    }

    void fill_radial_gradient( POS x, POS y, DIM radii, color c1, color c2 )
    {
      COLOR_STOP stops[2] = { {gcolor(c1), 0.0}, {gcolor(c2), 1.0} };
      fill_radial_gradient( x, y, radii, stops, 2 );
    }


    void fill_mode( bool even_odd /* false - fill_non_zero */ )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gFillMode( hgfx, even_odd ); 
      assert(r == GRAPHIN_OK); r;
    }

    // SECTION: text

    // define font attributes for all subsequent text operations.
    void font(  const char* name, DIM size, bool bold = false, bool italic = false, ANGLE angle = 0)
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gSetFont( hgfx, name, size, bold, italic, angle ); 
      assert(r == GRAPHIN_OK); r;
    }

    // draw text at x,y with text alignment
    void text(POS x, POS y, aux::wchars t)
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gText( hgfx, x, y, t.start, t.length ); 
      assert(r == GRAPHIN_OK); r;
    }

    // calculates width of the text string
    DIM text_width( aux::wchars t )
    {
      DIM width;
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gTextWidth( hgfx, t.start, t.length, &width ); 
      assert(r == GRAPHIN_OK); r;
      return width;
    }

    // returns height and ascent of the font.
    void font_metrics( DIM& height, DIM& ascent )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gFontMetrics( hgfx, &height, &ascent ); 
      assert(r == GRAPHIN_OK); r;
    }

    // set text alignments
    void text_alignment( TEXT_ALIGNMENT x, TEXT_ALIGNMENT y)
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gTextAlignment( hgfx, x, y ); 
      assert(r == GRAPHIN_OK); r;
    }

    // SECTION: image rendering

    // draws img onto the graphics surface with current transformation applied (scale, rotation). expensive
    void draw_image ( image* pimg, POS x, POS y, DIM w, DIM h, UINT ix, UINT iy, UINT iw, UINT ih )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gDrawImage( hgfx, pimg->himg, x, y, &w, &h, &ix, &iy, &iw, &ih ); 
      assert(r == GRAPHIN_OK); r;
    }
    // draws whole img onto the graphics surface with current transformation applied (scale, rotation). expensive
    void draw_image ( image* pimg, POS x, POS y )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gDrawImage( hgfx, pimg->himg, x, y, 0, 0, 0, 0, 0, 0 ); 
      assert(r == GRAPHIN_OK); r;
    }
    // blends image bits with bits of the surface. no affine transformations. less expensive
    void blend_image ( image* pimg, POS x, POS y, UINT opacity, UINT ix, UINT iy, UINT iw, UINT ih )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gBlendImage( hgfx, pimg->himg, x, y, opacity, &ix, &iy, &iw, &ih ); 
      assert(r == GRAPHIN_OK); r;
    }
    // blends image bits with bits of the surface. no affine transformations. less expensive
    void blend_image ( image* pimg, POS x, POS y, UINT opacity )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gBlendImage( hgfx, pimg->himg, x, y, opacity, 0, 0, 0, 0 ); 
      assert(r == GRAPHIN_OK); r;
    }
    // blits image bits onto underlying pixel buffer. no affine transformations. 
    void blit_image ( image* pimg, POS x, POS y, UINT ix, UINT iy, UINT iw, UINT ih )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gBlitImage( hgfx, pimg->himg, x, y, &ix, &iy, &iw, &ih ); 
      assert(r == GRAPHIN_OK); r;
    }
    // blits image bits onto underlying pixel buffer. no affine transformations. 
    void blit_image ( image* pimg, POS x, POS y )
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gBlitImage( hgfx, pimg->himg, x, y, 0, 0, 0, 0 ); 
      assert(r == GRAPHIN_OK); r;
    }

    // SECTION: coordinate space

    void world_to_screen ( POS& inout_x, POS& inout_y)
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gWorldToScreen( hgfx, &inout_x, &inout_y ); 
      assert(r == GRAPHIN_OK); r;
    }

    void world_to_screen ( DIM& inout_length)
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gWorldToScreen( hgfx, &inout_length, 0 ); 
      assert(r == GRAPHIN_OK); r;
    }

    void screen_to_world ( POS& inout_x, POS& inout_y)
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gScreenToWorld( hgfx, &inout_x, &inout_y ); 
      assert(r == GRAPHIN_OK); r;
    }

    void screen_to_world ( DIM& inout_length)
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gScreenToWorld( hgfx, &inout_length, 0 ); 
      assert(r == GRAPHIN_OK); r;
    }

    // SECTION: clipping

    void set_clip_box ( POS x1, POS y1, POS x2, POS y2)
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gSetClipBox( hgfx, x1, y1, x2, y2 ); 
      assert(r == GRAPHIN_OK); r;
    }

    void get_clip_box ( POS& x1, POS& y1, POS& x2, POS& y2)
    {
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gGetClipBox( hgfx, &x1, &y1, &x2, &y2 ); 
      assert(r == GRAPHIN_OK); r;
    }

    bool is_visible ( POS x, POS y )
    {
      BOOL is_in = 0;
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gPointInClipBox( hgfx, x, y, &is_in ); 
      assert(r == GRAPHIN_OK); r;
      return is_in != 0;
    }

    bool is_visible ( POS x1, POS y1, POS x2, POS y2 )
    {
      BOOL is_in = 0;
      assert(hgfx);
      GRAPHIN_RESULT r = gapi()->gRectInClipBox( hgfx, x1, y1, x2, y2, &is_in ); 
      assert(r == GRAPHIN_OK); r;
      return is_in != 0;
    }
  };

}

#endif //defined(__cplusplus) && !defined( PLAIN_API_ONLY )


#endif


