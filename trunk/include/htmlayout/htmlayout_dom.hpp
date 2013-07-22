/*
 * Terra Informatica Lightweight Embeddable HTMLayout control
 * http://terrainformatica.com/htmlayout
 * 
 * HTMLayout DOM implementation. C++ wrapper
 * 
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 * 
 * (C) 2003-2004, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

/**\file
 * \brief \link htmlayout_dom.h DOM \endlink C++ wrapper
 **/

#ifndef __htmlayout_dom_hpp__
#define __htmlayout_dom_hpp__

#pragma once

#include "value.h"

#include "htmlayout_dom.h"
#include "htmlayout_aux.h"

#include "htmlayout_queue.h"

#include <assert.h>
#include <stdio.h> // for vsnprintf
#include <vector> // for dom_iterator

#pragma warning( push )

  #pragma warning(disable:4786) //identifier was truncated...
  #pragma warning(disable:4996) //'strcpy' was declared deprecated
  #pragma warning(disable:4100) //unreferenced formal parameter 

/**htmlayout namespace.*/
namespace htmlayout
{

  /**dom namespace.*/
  namespace dom 
  {

  /**callback structure.
   * Used with #htmlayout::dom::element::select() function. 
   **/
    class callback 
    {
    public:
    /**Is called for every element that match criteria specified when calling to #htmlayout::dom::element::select() function.*/
      virtual bool on_element(HELEMENT he) = 0; /* return false to continue enumeration*/
#if !defined(_MSC_VER) || _MSC_VER > 1200
      virtual ~callback() {} // this line causing internal compiler error in VC6
#endif
    };

    class expando; // DOM element expando structure


    /** Helper class for element [] operator **/
    class attribute_accessor {
    public:
        HELEMENT el_;
        LPCSTR key_;
        attribute_accessor(HELEMENT el, LPCSTR key) 
        { 
          el_ = (HTMLayout_UseElement(el) == HLDOM_OK) ? el : 0; 
          key_ = key; 
        }

        ~attribute_accessor()
        {
           HLDOM_RESULT r = HTMLayout_UnuseElement(el_);
           assert(r == HLDOM_OK);r;
        }
  
        attribute_accessor &operator= (const wchar_t* value)
        {
            HLDOM_RESULT r = HTMLayoutSetAttributeByName(el_, key_, value);
            assert(r == HLDOM_OK);r;
            return *this;
        }

        attribute_accessor &operator= (int value)
        {
            HLDOM_RESULT r = HTMLayoutSetAttributeByName(el_, key_, i2w(value));
            assert(r == HLDOM_OK);r;
            return *this;
        }

        bool operator ==(const attribute_accessor &rv) const
        {
            return wcscmp(this->operator const wchar_t*(), rv.operator const wchar_t*()) == 0;
        }

        bool operator !=(const attribute_accessor &rv) const
        {
            return wcscmp(this->operator const wchar_t*(), rv.operator const wchar_t*()) != 0;
        }

        bool operator <(const attribute_accessor &rv) const
        {
            return wcscmp(this->operator const wchar_t*(), rv.operator const wchar_t*()) < 0;
        }

        bool operator >(const attribute_accessor &rv) const
        {
            return wcscmp(this->operator const wchar_t*(), rv.operator const wchar_t*()) > 0;
        }

        operator const wchar_t*() const
        {
            const wchar_t* lpw = 0;
            if( key_[0] == '-' && (HTMLayoutGetStyleAttribute(el_, key_, &lpw) == HLDOM_OK) && lpw)
               return lpw;
            HTMLayoutGetAttributeByName(el_, key_[0] == '-' ? key_ + 1 : key_, &lpw);
            return lpw;
        }

        /** To get int value use it this way: element["attribute"].get(0) **/
        int get(int default_value)
        {
            const wchar_t* attr = this->operator const wchar_t*();
            return aux::wtoi(attr,default_value);
        }

        void remove()
        {
            HTMLayoutSetAttributeByName(el_, key_, 0);
        }
    };



  /**DOM element. 
     Smart pointer, pretty much std::shared_ptr thing */
   
    class element
    {
    protected:
      HELEMENT he;

      void use(HELEMENT h) { he = (HTMLayout_UseElement(h) == HLDOM_OK)? h: 0; }
      void unuse() { if(he) HTMLayout_UnuseElement(he); he = 0; }
      void set(HELEMENT h) { unuse(); use(h); }

    public:
    /**Construct \c undefined element .
     **/
      element(): he(0) { }

    /**Construct \c element from existing element handle.
     * \param h \b #HELEMENT
     **/
      element(HELEMENT h)       { use(h); }

    /**Copy constructor;
     * \param e \b #element
     **/
      element(const element& e) { use(e.he); }

      operator HELEMENT() const { return he; }

    /**Destructor.*/
      ~element()                { unuse(); }

      /** Get/set attribute of DOM element. It is possible to get style attribute via placing '-' in the beginning of attribute name */
      attribute_accessor operator[](const char* key)
      {
          return attribute_accessor(he, key);
      }

	  dom::element operator[](unsigned int child_index)
	  {
		  assert(children_count() > child_index);
		  return dom::element(child(child_index));
	  }


    /**Assign \c element an \c #HELEMENT
     * \param h \b #HELEMENT
     * \return \b #element&
     **/
      element& operator = (HELEMENT h) { set(h); return *this; }

    /**Assign \c element another \c #element
     * \param e \b #element
     * \return \b #element&
     **/
      element& operator = (const element& e) { set(e.he); return *this; }

    /**Test equality of this and another \c #element's
     * \param rs \b const \b #element 
     * \return \b bool, true if elements are equal, false otherwise
     **/
      bool operator == (const element& rs ) const { return he == rs.he; }
      bool operator == (HELEMENT rs ) const { return he == rs; }

    /**Test equality of this and another \c #element's
     * \param rs \b const \b #element 
     * \return \b bool, true if elements are not equal, false otherwise
     **/
      bool operator != (const element& rs ) const { return he != rs.he; }

    /**Test whether element is valid.
     * \return \b bool, true if element is valid, false otherwise
     **/
      bool is_valid() const { return he != 0; }

    /**Tests whether element is memeber of the DOM or is it detached.
     * \return \b bool, true if element is in the DOM, false otherwise
     **/
      bool is_alive() const { return root() != 0; }


    /**Get number of child elements.
     * \return \b int, number of child elements
     **/
      unsigned int children_count() const 
      { 
        UINT count = 0;
        HTMLayoutGetChildrenCount(he, &count);
        return count;
      }

    /**Get Nth child element.
     * \param index \b unsigned \b int, number of the child element
     * \return \b #HELEMENT, child element handle
     **/
      HELEMENT child( unsigned int index ) const 
      { 
        HELEMENT child = 0;
        HTMLayoutGetNthChild(he, index, &child);
        return child;
      }

    /**Get parent element.
     * \return \b #HELEMENT, handle of the parent element
     **/
      HELEMENT parent( ) const 
      { 
        HELEMENT hparent = 0;
        HTMLayoutGetParentElement(he, &hparent);
        return hparent;
      }

    /**Get index of this element in its parent collection.
     * \return \b unsigned \b int, index of this element in its parent collection
     **/
      unsigned int index( ) const 
      { 
        UINT index = 0;
        HTMLayoutGetElementIndex(he, &index);
        return index;
      }

    /**Get number of the attributes.
     * \return \b unsigned \b int, number of the attributes
     **/
      unsigned int get_attribute_count( ) const 
      { 
        UINT n = 0;
        HTMLayoutGetAttributeCount(he, &n);
        return n;
      }

    /**Get attribute value by its index.
     * \param n \b unsigned \b int, number of the attribute
     * \return \b const \b wchar_t*, value of the n-th attribute
     **/
      const wchar_t* get_attribute( unsigned int n ) const 
      { 
        LPCWSTR lpw = 0;
        HTMLayoutGetNthAttribute(he, n, 0, &lpw);
        return lpw;
      }

    /**Get attribute name by its index.
     * \param n \b unsigned \b int, number of the attribute
     * \return \b const \b char*, name of the n-th attribute
     **/
      const char* get_attribute_name( unsigned int n ) const 
      { 
        LPCSTR lpc = 0;
        HTMLayoutGetNthAttribute(he, n, &lpc, 0);
        return lpc;
      }

    /**Get attribute value by name.
     * \param name \b const \b char*, name of the attribute
     * \return \b const \b wchar_t*, value of the n-th attribute
     **/
      const wchar_t* get_attribute( const char* name ) const 
      { 
        LPCWSTR lpw = 0;
        HTMLayoutGetAttributeByName(he, name, &lpw);
        return lpw;
      }

    /**Add or replace attribute.
     * \param name \b const \b char*, name of the attribute
     * \param value \b const \b wchar_t*, name of the attribute
     **/
    void set_attribute( const char* name, const wchar_t* value )
      { 
        HTMLayoutSetAttributeByName(he, name, value);
      }

    /**Get attribute integer value by name.
     * \param name \b const \b char*, name of the attribute
     * \return \b int , value of the attribute
     **/
      int get_attribute_int( const char* name, int def_val = 0 ) const 
      { 
        const wchar_t* txt = get_attribute(name);
        if(!txt) return def_val;
        return _wtoi(txt);
      }

    /**Special form of get attribute value by name, it tries to get value from attribute 
     * collection and if fails then from custom style attribute (that has to start from '-')
     * \param name \b const \b char*, name of the attribute that shall start from "-" sign
     * \return \b const wchar_t* , value of the attribute
     **/
      const wchar_t* attribute( const char* name, const wchar_t* default_value ) const 
      { 
        assert(name[0] == '-');
        const wchar_t* txt = get_attribute(name+1);
        if(txt) return txt;
        txt = get_style_attribute(name);
        if(txt) return txt;
        return default_value;
      }
      int attribute( const char* name, int default_value ) const 
      { 
        wchar_t none[1] = {0};        
        const wchar_t* txt = attribute( name, none );
        if(txt == none) return default_value;
        return aux::wtoi(txt,default_value);
      }

      color attribute( const char* name, color default_value ) const 
      { 
        wchar_t none[1] = {0};
        const wchar_t* txt = attribute( name, none );
        if(txt == none) return default_value;
        return color::parse(aux::chars_of(txt), default_value);
      }


      
    /**Remove attribute.
     * \param name \b const \b char*, name of the attribute
     **/
    void remove_attribute( const char* name ) 
      { 
        HTMLayoutSetAttributeByName(he, name, 0);
      }
      

    /**Get style attribute of the element by its name.
     * \param name \b const \b char*, name of the style attribute, e.g. "background-color"
     * \return \b const \b wchar_t*, value of the style attribute
     *
     * Also all style attributes of the element are available in "style" attribute of the element.
     **/
    const wchar_t* get_style_attribute( const char* name ) const 
      { 
        LPCWSTR lpw = 0;
        HTMLayoutGetStyleAttribute(he, name, &lpw);
        return lpw;
      }

    /**Set style attribute.
     * \param name \b const \b char*, name of the style attribute
     * \param value \b const \b wchar_t*, value of the style attribute
     *
     * \par Example:
     * \code e.set_style_attribute("background-color", L"red"); \endcode
     **/
    void set_style_attribute( const char* name, const wchar_t* value )  
      { 
        HTMLayoutSetStyleAttribute(he, name, value);
      }

    /** Clear style attribute that was defined by set_style_attribute.
     * \param name \b const \b char*, name of the style attribute
     *
     * \par Example:
     * \code e.clear_style_attribute("background-color"); \endcode
     **/
    void clear_style_attribute( const char* name )  
      { 
        HTMLayoutSetStyleAttribute(he, name, 0);
      }

    /** Clear all style attribute that was defined by set_style_attribute.
     */
    void clear_all_style_attributes() 
      { 
        HTMLayoutSetStyleAttribute(he, 0, 0);
      }


    /**Get root DOM element of the HTMLayout document.
     * \param hHTMLayoutWnd \b HWND, HTMLayout window
     * \return \b #HELEMENT, root element
     * \see also \b #root
     **/
      static HELEMENT root_element(HWND hHTMLayoutWnd)
      {
        HELEMENT h = 0;
        HTMLayoutGetRootElement(hHTMLayoutWnd,&h);
        //HLDOM_RESULT r = HTMLayoutGetRootElement(hHTMLayoutWnd,&h);
        //assert(r == HLDOM_OK);r;
        return h;
      }

    /**Get focus DOM element of the HTMLayout document.
     * \param hHTMLayoutWnd \b HWND, HTMLayout window
     * \return \b #HELEMENT, focus element
     *
     * COMMENT: to set focus use: set_state(STATE_FOCUS)
     *
     **/
      static HELEMENT focus_element(HWND hHTMLayoutWnd)
      {
        HELEMENT h = 0;
        HTMLayoutGetFocusElement(hHTMLayoutWnd,&h);
        return h;
      }
     

    /**Find DOM element of the HTMLayout document by coordinates.
     * \param hHTMLayoutWnd \b HWND, HTMLayout window
     * \param clientPt \b POINT,  coordinates.
     * \return \b #HELEMENT, found element handle or zero
     **/
      static HELEMENT find_element(HWND hHTMLayoutWnd, POINT clientPt)
      {
        HELEMENT h = 0;
        HTMLayoutFindElement(hHTMLayoutWnd, clientPt, &h);
        return h;
      }

     
    /**Set mouse capture.
     * After call to this function all mouse events will be targeted to this element.
     * To remove mouse capture call #htmlayout::dom::element::release_capture().
     **/
      void set_capture() { HTMLayoutSetCapture(he); }

    /**Setup this element as an event root.
     * After that all UI events will be delivered only to this element or its sub-elements.
     * This is in principle close to 'capture' but works for DOM subtrees not only for particular elements.
     * If focus is set to the element outside of the event root it will be moved insed this element.
     **/
      void set_event_root() { HTMLayoutSetEventRoot(he, 0); }
      void set_event_root(element& prev) { HELEMENT phe = 0; HTMLayoutSetEventRoot(he, &phe); prev = phe; }

    /**Reset event root. 
      **/
      void reset_event_root() { HTMLayoutSetEventRoot( dom::element(he).root(), 0); }

    /**Release mouse capture.
     * Mouse capture can be set with #element:set_capture()
     **/
      static void release_capture() { ReleaseCapture(); }

      inline static BOOL CALLBACK callback_func( HELEMENT he, LPVOID param )
      {
        callback *pcall = (callback *)param;
        return pcall->on_element(he)? TRUE:FALSE; // TRUE - stop enumeration
      }

    /**Enumerate all descendant elements.
     * \param pcall \b #htmlayout::dom::callback*, callback structure. Its member function #htmlayout::dom::callback::on_element() is called for every enumerated element.
     * \param tag_name \b const \b char*, comma separated list of tag names to search, e.g. "div", "p", "div,p" etc. Can be NULL.
     * \param attr_name \b const \b char*, name of attribute, can contain wildcard characters, see below. Can be NULL.
     * \param attr_value \b const \b char*, name of attribute, can contain wildcard characters, see below. Can be NULL.
     * \param depth \b int, depth - depth of search. 0 means all descendants, 1 - direct children only, 
     * 2 - children and their children and so on.
     *
     * Wildcard characters in attr_name and attr_value:
     * - '*' - any substring
     * - '?' - any one char
     * - '['char set']' = any one char in set
     *
     * \par Example:
     * - [a-z] - all lowercase letters
       * - [a-zA-Z] - all letters
     * - [abd-z] - all lowercase letters except of 'c'
     * - [-a-z] - all lowercase letters and '-'
     *
     * \par Example:
     * \code document.select(mycallback, "a", "href", "http:*.php"); \endcode
       * will call mycallback.on_element() on each <A> element in the document 
       *           having 'href' attribute with value
       *           starting from "http:" and ending with ".php"
     *
     * \par Example:
     * \code document.select(mycallback); \endcode
     * will enumerate ALL elements in the document.
     **/
      inline void select( callback *pcall,  
        const char* tag_name = 0, 
        const char* attr_name = 0, 
        const wchar_t* attr_value = 0, 
        int depth = 0) const
      {
        HTMLayoutVisitElements( he, tag_name, attr_name, attr_value, callback_func, pcall, depth);
      }

      inline HELEMENT select_parent(const char* selectors, // CSS selectors, comma separated list
                          int depth
                        ) const
      {
        HELEMENT heFound = 0;
        HLDOM_RESULT r = HTMLayoutSelectParent( he, selectors, depth, &heFound );
        assert(r == HLDOM_OK);r;
        return heFound;
      }

      inline HELEMENT select_parent(const wchar_t* selectors, // CSS selectors, comma separated list
                          int depth
                        ) const
      {
        HELEMENT heFound = 0;
        HLDOM_RESULT r = HTMLayoutSelectParentW( he, selectors, depth, &heFound );
        assert(r == HLDOM_OK);r;
        return heFound;
      }

      inline void select_elements( callback *pcall,  
                          const char* selectors // CSS selectors, comma separated list
                        ) const
      {
        HTMLayoutSelectElements( he, selectors, callback_func, pcall);
      }

      inline void select_elements( callback *pcall,  
                          const wchar_t* selectors // CSS selectors, comma separated list
                        ) const
      {
        HTMLayoutSelectElementsW( he, selectors, callback_func, pcall);
      }

 
     /**Get element by id.
     * \param id \b char*, value of the "id" attribute.
     * \return \b #HELEMENT, handle of the first element with the "id" attribute equal to given.
     **/
       HELEMENT get_element_by_id(const char* id) const
       {
         find_first_callback cb;
         select(&cb,0,"id",aux::a2w(id));
         return cb.hfound;
       }

       HELEMENT get_element_by_id(const wchar_t* id) const
       {
         find_first_callback cb;
         select(&cb,0,"id",id);
         return cb.hfound;
       }

    /** Request to repaint the element. If the element has drawing behavior attached it will receive on_draw() call after that.
     **/
       void refresh() const 
       { 
          RECT rc = get_location(BORDER_BOX);
          ::InvalidateRect(get_element_hwnd(false),&rc,FALSE);
       }
 
    /** Request update of styles and/or position of the element and refresh element area in its window.
     * \param[in] remeasure \b bool, true if method element will be redrawn immediately.
     *
     * This method is optional since v 3.3.0.4   
     * Engine will call update internaly when handling DOM mutating methods.  
     *
     **/
       void update( bool remeasure = false ) const 
       { 
          HTMLayoutUpdateElement(he, remeasure? TRUE:FALSE); 
       }

    /**Apply changes and refresh element area in its window.
     * \param[in] render_now \b bool, if true element will be redrawn immediately.
     **/
       void update( int mode ) const 
       {  
          HTMLayoutUpdateElementEx(he, mode); 
       }

 
    /**Get next sibling element.
     * \return \b #HELEMENT, handle of the next sibling element if it exists or 0 otherwise
     **/
       HELEMENT next_sibling() const 
       {
         unsigned int idx = index() + 1;
         element pel = parent();
         if(!pel.is_valid())
          return 0;
        if( idx >= pel.children_count() )
          return 0;
        return pel.child(idx);
      }

    /**Get previous sibling element.
     * \return \b #HELEMENT, handle of previous sibling element if it exists or 0 otherwise
     **/
      HELEMENT prev_sibling() const 
      {
        int idx = (int)index() - 1;
        element pel = parent();
        if(!pel.is_valid())
          return 0;
        if( idx < 0 )
          return 0;
        return pel.child(idx);
      }

    /**Get first sibling element.
     * \return \b #HELEMENT, handle of the first sibling element if it exists or 0 otherwise
     **/
       HELEMENT first_sibling() const 
       {
         element pel = parent();
         if(!pel.is_valid())
          return 0;
        return pel.child(0);
      }

    /**Get last sibling element.
     * \return \b #HELEMENT, handle of last sibling element if it exists or 0 otherwise
     **/
      HELEMENT last_sibling() const 
      {
         element pel = parent();
         if(!pel.is_valid())
          return 0;
        return pel.child(pel.children_count() - 1);
      }


    /**Get root of the element
     * \return \b #HELEMENT, handle of document root element (html)
     **/
      HELEMENT root() const 
      {
        element pel = parent();
        if(pel.is_valid()) return pel.root();
        return he;
      }


    /**Get bounding rectangle of the element.
     * \param root_relative \b bool, if true function returns location of the 
     * element relative to HTMLayout window, otherwise the location is given 
     * relative to first scrollable container.
     * \return \b RECT, bounding rectangle of the element.
     **/
      RECT get_location(unsigned int area = ROOT_RELATIVE | CONTENT_BOX) const
      {
        RECT rc = {0,0,0,0};
        HTMLayoutGetElementLocation(he,&rc, area);
        return rc;
      }

      /** Test if point is inside shape rectangle of the element.
        client_pt - client rect relative point          
       **/
      bool is_inside( POINT client_pt ) const
      {
        RECT rc = get_location(ROOT_RELATIVE | BORDER_BOX);
        return PtInRect(&rc,client_pt) != FALSE;
      }


      /**Scroll this element to view.
       **/
      void scroll_to_view(bool toTopOfView = false, bool smooth = false)
      {
        UINT flags = 0;
        if(toTopOfView) flags |= SCROLL_TO_TOP;
        if(smooth) flags |= SCROLL_SMOOTH;
        HTMLayoutScrollToView(he,flags);
      }

      void get_scroll_info(POINT& scroll_pos, RECT& view_rect, SIZE& content_size)
      {
        HLDOM_RESULT r = HTMLayoutGetScrollInfo(he, &scroll_pos, &view_rect, &content_size);
        assert(r == HLDOM_OK); r;
      }
      void set_scroll_pos(POINT scroll_pos, bool smooth = true)
      {
        HLDOM_RESULT r = HTMLayoutSetScrollPos(he, scroll_pos, BOOL(smooth));
        assert(r == HLDOM_OK); r;
      }

      /** get min-intrinsic and max-intrinsic widths of the element. */
      void get_intrinsic_widths(int& min_width,int& max_width)
      {
        HLDOM_RESULT r = HTMLayoutGetElementIntrinsicWidths(he, &min_width, &max_width);
        assert(r == HLDOM_OK); r;
      }
      /** get min-intrinsic height of the element calculated for forWidth. */
      void get_intrinsic_height(int for_width, int& min_height)
      {
        HLDOM_RESULT r = HTMLayoutGetElementIntrinsicHeight(he, for_width, &min_height);
        assert(r == HLDOM_OK); r;
      }

      /**Get element's type.
     * \return \b const \b char*, name of the elements type
     * 
       * \par Example:
       * For &lt;div&gt; tag function will return "div".
     **/
      const char* get_element_type() const
      {
        LPCSTR str = 0;
        HTMLayoutGetElementType(he,&str);
        return str;
      }

    /**Get HWND of containing window.
     * \param root_window \b bool, handle of which window to get:
     * - true - HTMLayout window
     * - false - nearest parent element having overflow:auto or :scroll
     * \return \b HWND
     **/
      HWND get_element_hwnd(bool root_window) const
      {
        HWND hwnd = 0;
        HTMLayoutGetElementHwnd(he,&hwnd, root_window? TRUE : FALSE);
        return hwnd;
      }

    /**Get element UID - identifier suitable for storage.
     * \return \b UID
     **/
      UINT get_element_uid() const
      {
        UINT uid = 0;
        HTMLayoutGetElementUID(he,&uid);
        return uid;
      }

    /**Get element handle by its UID.
     * \param hHTMLayoutWnd \b HWND, HTMLayout window
     * \param uid \b UINT, uid of the element
     * \return \b #HELEMENT, handle of element with the given uid or 0 if not found
     **/
      static HELEMENT element_by_uid(HWND hHTMLayoutWnd, UINT uid)
      {
        HELEMENT h = 0;
        HTMLayoutGetElementByUID(hHTMLayoutWnd, uid,&h);
        return h;
      }

      static HELEMENT from_value(const json::value& val)
      {
         if( val.is_dom_element() )
           return (HELEMENT) val.get_object_data();
         return 0;
      }

    /**Combine given URL with URL of the document element belongs to.
     * \param[in, out] inOutURL \b LPWSTR, at input this buffer contains 
     * zero-terminated URL to be combined, after function call it contains 
     * zero-terminated combined URL
     * \param bufferSize \b UINT, size of the buffer pointed by \c inOutURL
       **/
      void combine_url(LPWSTR inOutURL, UINT bufferSize) const
      {
        HTMLayoutCombineURL(he,inOutURL,bufferSize);
      }

      json::string url(const wchar_t* rel_url) const
      {
        wchar_t buffer[4096]; wcsncpy(buffer,rel_url,4096); buffer[4095] = 0;
        HTMLayoutCombineURL(he,buffer,4096);
        return buffer;
      }


    /**Set inner or outer html of the element.
     * \param html \b const \b unsigned \b char*, UTF-8 encoded string containing html text
     * \param html_length \b size_t, length in bytes of \c html
     * \param where \b int, possible values are:
     * - SIH_REPLACE_CONTENT - replace content of the element
     * - SIH_INSERT_AT_START - insert html before first child of the element
     * - SIH_APPEND_AFTER_LAST - insert html after last child of the element
     **/
      void set_html( const unsigned char* html, size_t html_length, int where = SIH_REPLACE_CONTENT)
      { 
        if(html == 0 || html_length == 0)
          clear();
        else
        {
          HLDOM_RESULT r = HTMLayoutSetElementHtml(he, html, DWORD(html_length), where);
          assert(r == HLDOM_OK); r;
        }
      }

      /*const unsigned char* 
          get_html( bool outer = true) const 
      { 
        unsigned char* utf8bytes = 0;
        HLDOM_RESULT r = HTMLayoutGetElementHtml(he, &utf8bytes, outer? TRUE:FALSE);
        assert(r == HLDOM_OK); r;
        return utf8bytes;
      }*/

      static void CALLBACK _writer_a(LPCBYTE utf8, UINT utf8_length, LPVOID pstdstr)
      {
        json::astring* dst = (json::astring*)pstdstr;
		    *dst = aux::chars((const char*)utf8,utf8_length);
      }

      json::astring get_html( bool outer = true) const 
      { 
        json::astring str;
        HLDOM_RESULT r = HTMLayoutGetElementHtmlCB(he, outer, &_writer_a, &str);
        assert(r == HLDOM_OK); r;
        return str;
      }

      // get text as utf8 bytes
      /* const unsigned char* get_text() const
      {
        unsigned char* utf8bytes = 0;
        HLDOM_RESULT r = HTMLayoutGetElementInnerText(he, &utf8bytes);
        assert(r == HLDOM_OK); r;
        return utf8bytes;
      } */

      static void CALLBACK _writer_w(const wchar_t* text, unsigned int text_length, void* pstdstr)
      {
        json::string* dst = (json::string*)pstdstr;
		    *dst = aux::wchars(text,text_length);
      }

      // get text as wchar_t words 
      json::string text() const
      {
        json::string str;
        HLDOM_RESULT r = HTMLayoutGetElementInnerTextCB(he, &_writer_w, &str);
        assert(r == HLDOM_OK); r;
        return str;
      }

      void  set_text(const wchar_t* utf16, size_t utf16_length)
      {
        HLDOM_RESULT r = HTMLayoutSetElementInnerText16(he, utf16, (UINT)utf16_length);
        assert(r == HLDOM_OK); r;
      }

      void  set_text(const wchar_t* t)
      {
        assert(t);
        if( t ) set_text( t, wcslen(t) );
      }

      void clear() // clears content of the element
      {
        HLDOM_RESULT r = HTMLayoutSetElementInnerText16(he, L"", 0);
        assert(r == HLDOM_OK); r;
      }


    /**Delete element.
     * This function removes element from the DOM tree, detaches all behaviors and deletes it if there are no more references to it.
     **/
      void destroy() 
      {
        HTMLayoutDeleteElement(he);
        unuse();
      }

      inline int vsnprintf(wchar_t *buffer, size_t count, const wchar_t *format, va_list argptr) const
      {
        return _vsnwprintf( buffer, count, format, argptr );
      }

      inline int vsnprintf(char *buffer, size_t count, const char *format, va_list argptr) const
      {
        return _vsnprintf( buffer, count, format, argptr );
      }

      // Find first child element matching the selector
      // :root is the element itself
      template<class char_t>
        HELEMENT find_first( const char_t* selector, ... ) const
        {
          char_t buffer[2049]; buffer[0]=0;
          va_list args; va_start ( args, selector ); vsnprintf( buffer, 2048, selector, args ); va_end ( args );
          find_first_callback find_first;
          select_elements( &find_first, buffer); // find first element satisfying given CSS selector
          return find_first.hfound;
        }

      // Find all child elements matching the selector
      // :root is the element itself
      template<class char_t>
        void find_all( callback* cb, const char_t* selector, ... ) const
        {
          char_t buffer[2049]; buffer[0]=0;
          va_list args; va_start ( args, selector ); vsnprintf( buffer, 2048, selector, args ); va_end ( args );
          select_elements( cb, buffer); // find all elements satisfying given CSS selector
          //assert(find_first.hfound);
        }

      class find_all_callback: public callback 
      {
        std::vector<dom::element>* all;
        find_all_callback():all(0) {}
      public:
        find_all_callback(std::vector<dom::element>& _all):all(&_all) {}
        ~find_all_callback() {}
        bool on_element(HELEMENT he) { all->push_back(dom::element(he)); return false; /*continue enumeration*/ }
      };

      template<class char_t>
        void find_all( std::vector<dom::element>& all, const char_t* selector, ... ) const
      {
        char_t buffer[2049]; buffer[0]=0;
        va_list args; va_start ( args, selector ); vsnprintf( buffer, 2048, selector, args ); va_end ( args );
        find_all_callback cb(all);
        select_elements( &cb, buffer); // find all elements satisfying given CSS selector
      }

      // will find first parent satisfying given css selector(s), will check element itself
      // :root is document root object
      template<class char_t>
      HELEMENT find_nearest_parent(const char_t* selector, ...) const
      {
        char_t buffer[2049]; buffer[0]=0;
        va_list args; va_start ( args, selector ); vsnprintf( buffer, 2048, selector, args );
        va_end ( args );
        return select_parent(buffer, 0);
      }

      // test this element against CSS selector(s) 
      // :root is document root object
      template<class char_t>
      bool test(const char_t* selector, ...) const
      {
        char_t buffer[2049]; buffer[0]=0;
        va_list args; va_start ( args, selector ); _vsnprintf( buffer, 2048, selector, args ); va_end ( args );
        return select_parent(buffer, 1) != 0;
      }

    /**Get UI state bits of the element as set of ELEMENT_STATE_BITS 
     **/
      unsigned int get_state() const
      {
        UINT state = 0;
        HLDOM_RESULT r = HTMLayoutGetElementState(he,&state);
        assert(r == HLDOM_OK); r;
        return (ELEMENT_STATE_BITS) state;
      }

    /**Checks if particular UI state bits are set in the element.
     **/
      bool get_state(/*ELEMENT_STATE_BITS*/ unsigned int bits) const
      {
        UINT state = 0;
        HLDOM_RESULT r = HTMLayoutGetElementState(he,&state);
        assert(r == HLDOM_OK); r;
        return (state & bits) != 0;
      }


    /**Set UI state of the element with optional view update.
     **/
      void set_state(
        /*ELEMENT_STATE_BITS*/ unsigned int bitsToSet, 
        /*ELEMENT_STATE_BITS*/ unsigned int bitsToClear = 0, bool update = true )
      {
        /*if(bitsToSet){
            this->send_event(WY_ELEMENT_STATE_SET, bitsToSet);
        }

        if(bitsToClear){
            this->send_event(WY_ELEMENT_STATE_RESET, bitsToClear);
        }*/

        HLDOM_RESULT r = HTMLayoutSetElementState(he,bitsToSet,bitsToClear, update?TRUE:FALSE);
        assert(r == HLDOM_OK); r;
      }

      void toggle_state(/*ELEMENT_STATE_BITS*/ unsigned int bitsToSet, bool onOff)
      {
        HLDOM_RESULT r = HTMLayoutSetElementState(he,onOff?bitsToSet:0, onOff?0:bitsToSet, TRUE);
        assert(r == HLDOM_OK); r;
      }

      void start_timer(UINT millis, UINT_PTR tid = 0)
      {
        HLDOM_RESULT r = HTMLayoutSetTimerEx(he, millis, tid);
        assert(r == HLDOM_OK); r;
      }

      void stop_timer(UINT_PTR tid = 0)
      {
        HLDOM_RESULT r = HTMLayoutSetTimerEx(he, 0, tid);
        assert(r == HLDOM_OK); r;
      }

      /** "deeply enabled" **/
      bool enabled() 
      {
        BOOL b = FALSE;
        HLDOM_RESULT r = HTMLayoutIsElementEnabled(he,&b);
        assert(r == HLDOM_OK); r;
        return b != 0;
      }

      /** "deeply visible" **/
      bool visible() 
      {
        BOOL b = FALSE;
        HLDOM_RESULT r = HTMLayoutIsElementVisible(he,&b);
        assert(r == HLDOM_OK); r;
        return b != 0;
      }


    /** create brand new element with text (optional).
        Example:
           element div = element::create("div");
        - will create DIV element,
           element opt = element::create("option",L"Europe");
        - will create OPTION element with text "Europe" in it.
     **/
      static element create(const char* tagname, const wchar_t* text = 0)
      {
         element e(0);
         HLDOM_RESULT r = HTMLayoutCreateElement( tagname, text, &e.he ); // don't need 'use' here, as it is already "addrefed" 
         assert(r == HLDOM_OK); r;
         return e;
      }

    /** create brand new copy of this element. Element will be created disconected.
        You need to call insert to inject it in some container.
        Example:
           element select = ...;
           element option1 = ...;
           element option2 = option1.clone();
            select.insert(option2, option1.index() + 1);
        - will create copy of option1 element (option2) and insert it after option1,
     **/
      element clone()
      {
         element e(0);
         HLDOM_RESULT r = HTMLayoutCloneElement( he, &e.he ); // don't need 'use' here, as it is already "addrefed" 
         assert(r == HLDOM_OK); r;
         return e;
      }


    /** Insert element e at \i index position of this element.
     **/
      void insert( const element& e, unsigned int index )
      {
         HLDOM_RESULT r = HTMLayoutInsertElement( e.he, this->he, index );
         assert(r == HLDOM_OK); r;
      }

    /** Append element e as last child of this element.
     **/
      void append( const element& e ) { insert(e,0x7FFFFFFF); }


     /** detach - remove this element from its parent
      **/
      void detach()
      {
        HLDOM_RESULT r = HTMLayoutDetachElement( he );
        assert(r == HLDOM_OK); r;
      }

     /** swap two elements in the DOM
      **/
      void swap(HELEMENT with)
      {
         HTMLayoutSwapElements(he, with);
      }

      /** traverse event - send it by sinking/bubbling on the 
       * parent/child chain of this element
       **/
      bool send_event(unsigned int event_code, UINT_PTR reason = 0, HELEMENT heSource = 0)
      {
        BOOL handled = FALSE;
        HLDOM_RESULT r = HTMLayoutSendEvent(he, event_code, heSource? heSource: he, reason, &handled);
        assert(r == HLDOM_OK); r;
        return handled != 0;
      }

      /** post event - post it in the queue for later sinking/bubbling on the 
       * parent/child chain of this element.
       * method returns immediately
       **/
      void post_event(unsigned int event_code, unsigned int reason = 0, HELEMENT heSource = 0)
      {
        HLDOM_RESULT r = HTMLayoutPostEvent(he, event_code, heSource? heSource: he, reason);
        assert(r == HLDOM_OK); r;
      }

      /** move element to new location given by x_view_rel, y_view_rel - view relative coordinates.
       *  Method defines local styles, so to "stick" it back to the original location you
       *  should call element::clear_all_style_attributes().
       **/
      void move(int x_view_rel, int y_view_rel)
      {
        HLDOM_RESULT r = HTMLayoutMoveElement(he, x_view_rel, y_view_rel);
        assert(r == HLDOM_OK); r;
      }
      void move(int x_view_rel, int y_view_rel, int width, int height)
      {
        HLDOM_RESULT r = HTMLayoutMoveElementEx(he, x_view_rel, y_view_rel, width, height);
        assert(r == HLDOM_OK); r;
      }


      HELEMENT track_popup(POINT root_pt, bool animate = false)
      {
        HELEMENT hItem = 0;
        HTMLayoutTrackPopupAt(he, root_pt, animate? 1:0, &hItem);
        return hItem;
      }

      // 'placement' here is a point of the popup that corresponds to root_pt 
      HELEMENT track_popup(uint placement , POINT root_pt, bool animate = false)
      {
        HELEMENT hItem = 0;
        HTMLayoutTrackPopupAt(he, root_pt, (placement << 16) | (animate?1:0), &hItem);
        return hItem;
      }

      void show_popup(POINT root_pt, bool animate = false)
      {
        HTMLayoutShowPopupAt(he, root_pt, animate? 1:0);
      }

      void show_popup(uint placement, POINT root_pt, bool animate = false)
      {
        HTMLayoutShowPopupAt(he, root_pt, (placement << 16) | (animate?1:0));
      }


      void hide_popup()
      {
        HTMLayoutHidePopup(he);
      }
      
      /** attach event handler to the element
       **/
      void attach(event_handler* pevth)
      {
        htmlayout::attach_event_handler(he,pevth);
      }
      /** remove event handler from the list of event handlers of the element.
       **/
      void detach(event_handler* pevth)
      {
        htmlayout::detach_event_handler(he,pevth);
      }

      /** call method, invokes method in all event handlers attached to the element
       **/
      bool call_behavior_method(METHOD_PARAMS* p)
      {
        if(!is_valid())
          return false;
        return ::HTMLayoutCallBehaviorMethod(he,p) == HLDOM_OK;
      }

      json::value xcall(const char* name, 
              json::value p0 = json::value(),
              json::value p1 = json::value(),
              json::value p2 = json::value(),
              json::value p3 = json::value(),
              json::value p4 = json::value(),
              json::value p5 = json::value(),
              json::value p6 = json::value(),
              json::value p7 = json::value())
      {
        XCALL_PARAMS prm( name );
        json::value argv[8];
        prm.argv = argv;
        do 
        {
          if( p0.is_undefined()) break; argv[prm.argc++] = p0; 
          if( p1.is_undefined()) break; argv[prm.argc++] = p1; 
          if( p2.is_undefined()) break; argv[prm.argc++] = p2; 
          if( p3.is_undefined()) break; argv[prm.argc++] = p3; 
          if( p4.is_undefined()) break; argv[prm.argc++] = p4; 
          if( p5.is_undefined()) break; argv[prm.argc++] = p5; 
          if( p6.is_undefined()) break; argv[prm.argc++] = p6; 
          if( p7.is_undefined()) break; argv[prm.argc++] = p7; 
//#pragma warning( suppress:4127 ) //  warning C4127: conditional expression is constant
        } while(false);
        bool r = call_behavior_method(&prm);
        assert(r); r;
        return prm.retval;
      }

      void load_html(const wchar_t* url, HELEMENT initiator = 0)
      {
        HLDOM_RESULT r = HTMLayoutRequestElementData(he,url,HLRT_DATA_HTML,initiator);
        assert(r == HLDOM_OK); r;
      }

      struct comparator 
      {
        comparator() {}
        virtual ~comparator() {}
        virtual int compare(const htmlayout::dom::element& e1, const htmlayout::dom::element& e2) = 0;
        

        static INT CALLBACK scmp( HELEMENT he1, HELEMENT he2, LPVOID param )
        {
          htmlayout::dom::element::comparator* self = 
            static_cast<htmlayout::dom::element::comparator*>(param);

          htmlayout::dom::element e1 = he1;
          htmlayout::dom::element e2 = he2;

          return self->compare( e1,e2 );
        }
      };

      /** reorders children of the element using sorting order defined by cmp 
       **/
      void sort( comparator& cmp, int start = 0, int end = -1 )
      {
        if (end == -1)
          end = children_count();

        HLDOM_RESULT r = HTMLayoutSortElements(he, start, end, &comparator::scmp, &cmp);
        assert(r == HLDOM_OK); r;
      }

      CTL_TYPE get_ctl_type() const
      {
        UINT t = 0;
        HLDOM_RESULT r = ::HTMLayoutControlGetType(he,&t);
        assert(r == HLDOM_OK); r;
        return CTL_TYPE(t);
      }

      json::value get_value() const
      {
        json::value v;
        HLDOM_RESULT r = ::HTMLayoutControlGetValue(he,&v);
        assert(r == HLDOM_OK); r;
        return v;
      }

      void set_value(const json::value& v)
      {
        HLDOM_RESULT r = ::HTMLayoutControlSetValue(he,&v);
        assert(r == HLDOM_OK); r;
      }

      void     set_expando( expando* exp );  
      expando* get_expando();

      //private:
      struct find_first_callback: callback 
      {
        HELEMENT hfound;
        find_first_callback():hfound(0) {}
        ~find_first_callback() {}
        inline bool on_element(HELEMENT he) { hfound = he; return true; /*stop enumeration*/ }
      };

      void delay_measure()
      {
        HLDOM_RESULT r = ::HTMLayoutEnqueueMeasure(he);
        assert(r == HLDOM_OK); r;
      }

    };

    /**Expando - structure that can be associated with the DOM element. 
      *
      * In other words: DOM element can be expanded by this structure
      *
      * Derive your own class/structure from this one if you need some data to be associated with DOM element.
      * 
      * If you want to handle events here then additionally derive your structure from event_handler and 
      * call both: attach_event_handler and set_expando with it.
      *
      **/
    class expando: public HTMLayoutElementExpando
    {
      static void CALLBACK _finalizer(HTMLayoutElementExpando* pexp, HELEMENT he) { static_cast<expando*>(pexp)->finalize(); }
    protected:
      virtual void finalize() { delete this; }
    public:
      expando() { finalizer = _finalizer; }
      virtual ~expando() {} 
    };

    inline void element::set_expando( expando* exp )
    {
      HLDOM_RESULT r = HTMLayoutElementSetExpando(he, static_cast<HTMLayoutElementExpando*>(exp));
      assert(r == HLDOM_OK); r;
    }
   
    inline expando* element::get_expando()
    {
      HTMLayoutElementExpando* pexp = 0;
      HLDOM_RESULT r = HTMLayoutElementGetExpando(he, &pexp);
      assert(r == HLDOM_OK); r;
      return static_cast<expando*>(pexp);
    }


    #define STD_CTORS(T,PT) \
      T() { } \
      T(HELEMENT h): PT(h) { } \
      T(const element& e): PT(e) { } \
      T& operator = (HELEMENT h) { set(h); return *this; } \
      T& operator = (const element& e) { set(e); return *this; }


    class text_selection_params: public TEXT_SELECTION_PARAMS
    {
        pod::buffer<wchar_t>* _wos;
        text_selection_params():TEXT_SELECTION_PARAMS(false),_wos(0){}
    public:
        static BOOL CALLBACK ctl(text_selection_params* prms, UINT data ) { prms->_wos->push(wchar_t(data));  return 1; }
        text_selection_params(pod::buffer<wchar_t>& wos): TEXT_SELECTION_PARAMS(false), _wos(&wos)  { outs = (OutputStreamProc*)ctl; }
    };
    class html_selection_params: public TEXT_SELECTION_PARAMS
    {
        pod::buffer<byte>* _bos;
        html_selection_params():TEXT_SELECTION_PARAMS(true),_bos(0){}
    public:
        static BOOL CALLBACK ctl(html_selection_params* prms, UINT data ) { prms->_bos->push(byte(data));  return 1; }
        html_selection_params(pod::buffer<byte>& bos): TEXT_SELECTION_PARAMS(true), _bos(&bos)  { outs = (OutputStreamProc*)ctl; }
    };


    class editbox: public element
    {
        
      public: 
        STD_CTORS(editbox, element)
        
        bool selection( int& start, int& end )
        {
          TEXT_EDIT_SELECTION_PARAMS sp(false);
          if(!call_behavior_method(&sp))
            return false;
          start = sp.selection_start;
          end = sp.selection_end;
          return true;
        }
        bool select( int start = 0, int end = 0x7FFFFFFF )
        {
          TEXT_EDIT_SELECTION_PARAMS sp(true);
          sp.selection_start = start;
          sp.selection_end = end;
          return call_behavior_method(&sp);
        }
        bool replace(const wchar_t* text, size_t text_length)
        {
          TEXT_EDIT_REPLACE_SELECTION_PARAMS sp;
          sp.text = text;
          sp.text_length = UINT(text_length);
          return call_behavior_method(&sp);
        }

        std::wstring text_value() const
        {
          TEXT_VALUE_PARAMS sp(false);
          if( const_cast<editbox*>(this)->call_behavior_method(&sp) && sp.text && sp.length)
          {
              return std::wstring(sp.text, sp.length);
          }
          return std::wstring();
        }

        void text_value(const std::wstring& s)
        {
          text_value(s.c_str(), s.length());
        }
        void text_value(const wchar_t* text, size_t length)
        {
          TEXT_VALUE_PARAMS sp(true);
          sp.text = text;
          sp.length = UINT(length);
          call_behavior_method(&sp);
          update();
        }

        void text_value(const wchar_t* text)
        {
          TEXT_VALUE_PARAMS sp(true);
          sp.text = text;
          sp.length = text? UINT(wcslen(text)):0;
          call_behavior_method(&sp);
          update();
        }

        void caret_position(int& x, int& y, int& w, int& h)
        {
          TEXT_CARET_POSITION_PARAMS cpp;
          x = y = w = h = 0;
          if(call_behavior_method(&cpp))
          {
            x = cpp.left; y = cpp.top;
            w = cpp.width; h = cpp.height;
          }
        }

        void int_value( int v )
        {
           wchar_t buf[64]; int n = _snwprintf(buf,63,L"%d", v); buf[63] = 0;
           text_value(buf,n);
        }
        
        int int_value( ) const
        {
           return aux::wtoi( text_value().c_str() );
        }

        std::wstring selected_text()
        {
          pod::buffer<wchar_t> wos;
          get_selected_text(wos);
          return std::wstring(wos.data(), wos.length());
        }

        void get_selected_text(pod::buffer<wchar_t>& wos)
        {
          text_selection_params s(wos); 
          call_behavior_method(&s);
        }


        void get_selected_html(pod::buffer<byte>& bos)
        {
          html_selection_params s(bos); 
          call_behavior_method(&s);
        }

        int char_pos_at_xy(int x, int y) const
        {
          TEXT_EDIT_CHAR_POS_AT_XY_PARAMS sp;
          sp.x = x;
          sp.y = y;
          if(!const_cast<editbox*>(this)->call_behavior_method(&sp))
            return -1;
          return sp.char_pos;
        }

    };

    class scrollbar: public element
    {
      public: 
        STD_CTORS(scrollbar, element)

        void get_values( int& val, int& min_val, int& max_val, int& page_val, int& step_val )
        {
          SCROLLBAR_VALUE_PARAMS sbvp;
          call_behavior_method(&sbvp);
          val       = sbvp.value;
          min_val   = sbvp.min_value;
          max_val   = sbvp.max_value;
          page_val  = sbvp.page_value; // page increment
          step_val  = sbvp.step_value; // step increment (arrow button click) 
        }

        void set_values( int val, int min_val, int max_val, int page_val, int step_val )
        {
          SCROLLBAR_VALUE_PARAMS sbvp(true);
          sbvp.value       = val;
          sbvp.min_value   = min_val;
          sbvp.max_value   = max_val; 
          sbvp.page_value  = page_val;
          sbvp.step_value  = step_val;
          call_behavior_method(&sbvp);
        }

        int   get_value()
        {
          SCROLLBAR_VALUE_PARAMS sbvp;
          call_behavior_method(&sbvp);
          return sbvp.value;
        }

        void  set_value( int v )
        {
          SCROLLBAR_VALUE_PARAMS sbvp;
          call_behavior_method(&sbvp);
          sbvp.value = v;
          sbvp.methodID = SCROLL_BAR_SET_VALUE;
          call_behavior_method(&sbvp);
        }
    };

    struct enumerator
    {
      enumerator() {}
      virtual ~enumerator() {} // to make G++ happy.

              void ramble(HELEMENT what, bool forward = true)
              {
                HLDOM_RESULT r = HTMLayoutEnumerate(what,&_cb,this,forward);
                assert(r == HLDOM_OK); r;
              }

      virtual bool on_element_head( HELEMENT )                    { return false; }
      virtual bool on_element_tail( HELEMENT )                    { return false; }
      virtual bool on_char( HELEMENT text, int pos,  WCHAR code ) { return false; }

      inline static BOOL CALLBACK _cb( LPVOID p, HELEMENT he, int pos, int postype, WCHAR code )
      {
        enumerator* pe = (enumerator*)p;
        switch(postype)
        {
          case 0: // - he element head position.
            return pe->on_element_head( he ); 
          case 1: // - he element tail position.
            return pe->on_element_tail( he ); 
          case 2:// - character position.
            return pe->on_char( he, pos, code ); 
        }
        assert(false);
        return false;
      }
    };

    class root_element: public element 
    {
      public:
      root_element(HWND hWnd)
      {
         use(element::root_element(hWnd));
      }

      root_element(HELEMENT child)
      {
         HWND hwnd;
         HTMLayoutGetElementHwnd(child, &hwnd, true);
         use(element::root_element(hwnd));
      }
    };

 } // dom namespace

} // htmlayout namespace

#pragma warning( pop )

#endif
