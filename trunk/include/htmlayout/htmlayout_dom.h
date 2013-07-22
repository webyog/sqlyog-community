/*
 * Terra Informatica Lightweight Embeddable HTMLayout control
 * http://terrainformatica.com/htmengine
 * 
 * HTMLayout DOM implementation.
 * 
 * The code and information provided "as-is" without
 * warranty of any kind, either expressed or implied.
 * 
 * (C) 2003-2004, Andrew Fedoniouk (andrew@terrainformatica.com)
 */

/**\file
 * \brief HTML Document Object Model
 **/

#ifndef __htmengine_dom_h__
#define __htmengine_dom_h__

#include "value.h"

/**Type of the result value for HTMLayout DOM functions.
 * Possible values are:
 * - \b HLDOM_OK - function completed successfully
 * - \b HLDOM_INVALID_HWND - invalid HWND
 * - \b HLDOM_INVALID_HANDLE - invalid HELEMENT
 * - \b HLDOM_PASSIVE_HANDLE - attempt to use HELEMENT which is not marked by 
 *   #HTMLayout_UseElement()
 * - \b HLDOM_INVALID_PARAMETER - parameter is invalid, e.g. pointer is null
 * - \b HLDOM_OPERATION_FAILED - operation failed, e.g. invalid html in 
 *   #HTMLayoutSetElementHtml()
 **/
typedef int HLDOM_RESULT;

#ifndef LPUINT
  typedef UINT FAR *LPUINT;
#endif

#define HLDOM_OK 0
#define HLDOM_INVALID_HWND 1 
#define HLDOM_INVALID_HANDLE 2
#define HLDOM_PASSIVE_HANDLE 3
#define HLDOM_INVALID_PARAMETER 4
#define HLDOM_OPERATION_FAILED 5
#define HLDOM_OK_NOT_HANDLED (-1) // e.g. delayed

/**DOM element handle.*/
struct htmlayout_dom_element;
typedef htmlayout_dom_element* HELEMENT;

/**DOM range handle.*/
typedef LPVOID HRANGE;
typedef struct hposition { HELEMENT he; INT pos; } HPOSITION;

/**Marks DOM object as used (a.k.a. AddRef).
 * \param[in] he \b #HELEMENT
 * \return \b #HLDOM_RESULT
 * Application should call this function before using element handle. If the 
 * application fails to do that calls to other DOM functions for this handle 
 * may result in an error.
 * 
 * \sa #HTMLayout_UnuseElement()
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayout_UseElement(HELEMENT he);

/**Marks DOM object as unused (a.k.a. Release).
 * Get handle of every element's child element.
 * \param[in] he \b #HELEMENT
 * \return \b #HLDOM_RESULT
 * 
 * Application should call this function when it does not need element's 
 * handle anymore.
 * \sa #HTMLayout_UseElement()
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayout_UnuseElement(HELEMENT he);

/**Get root DOM element of HTML document.
 * \param[in] hwnd \b HWND, HTMLayout window for which you need to get root 
 * element
 * \param[out ] phe \b #HELEMENT*, variable to receive root element
 * \return \b #HLDOM_RESULT
 * 
 * Root DOM object is always a 'HTML' element of the document.
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutGetRootElement(HWND hwnd, HELEMENT *phe);

/**Get focused DOM element of HTML document.
 * \param[in] hwnd \b HWND, HTMLayout window for which you need to get focus 
 * element
 * \param[out ] phe \b #HELEMENT*, variable to receive focus element
 * \return \b #HLDOM_RESULT
 * 
 * phe can have null value (0).
 *
 * COMMENT: To set focus on element use HTMLayoutSetElementState(STATE_FOCUS,0)
 **/

EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutGetFocusElement(HWND hwnd, HELEMENT *phe);


/**Find DOM element by coordinate.
 * \param[in] hwnd \b HWND, HTMLayout window for which you need to find  
 * elementz
 * \param[in] pt \b POINT, coordinates, window client area relative.  
 * \param[out ] phe \b #HELEMENT*, variable to receive found element handle.
 * \return \b #HLDOM_RESULT
 * 
 * If element was not found then *phe will be set to zero.
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutFindElement(HWND hwnd, POINT pt, HELEMENT* phe);



/**Get number of child elements.
 * \param[in] he \b #HELEMENT, element which child elements you need to count
 * \param[out] count \b UINT*, variable to receive number of child elements 
 * \return \b #HLDOM_RESULT
 *
 * \par Example:
 * for paragraph defined as
 * \verbatim <p>Hello <b>wonderfull</b> world!</p> \endverbatim
 * count will be set to 1 as the paragraph has only one sub element: 
 * \verbatim <b>wonderfull</b> \endverbatim
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutGetChildrenCount(HELEMENT he, UINT* count);

/**Get handle of every element's child element.
 * \param[in] he \b #HELEMENT
 * \param[in] n \b UINT, number of the child element
 * \param[out] phe \b #HELEMENT*, variable to receive handle of the child 
 * element
 * \return \b #HLDOM_RESULT
 *
 * \par Example:
 * for paragraph defined as
 * \verbatim <p>Hello <b>wonderfull</b> world!</p> \endverbatim
 * *phe will be equal to handle of &lt;b&gt; element:
 * \verbatim <b>wonderfull</b> \endverbatim
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutGetNthChild(HELEMENT he, UINT n, HELEMENT* phe);

/**Get parent element.
 * \param[in] he \b #HELEMENT, element which parent you need
 * \param[out] p_parent_he \b #HELEMENT*, variable to recieve handle of the 
 * parent element
 * \return \b #HLDOM_RESULT
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutGetParentElement(HELEMENT he, HELEMENT* p_parent_he);

/**Get text of the element and information where child elements are placed.
 * \param[in] he \b #HELEMENT
 * \param[out] characters \b LPWSTR, buffer to receive text. Zero characters 
 * '\\0' will be inserted at places where subelements should be.
 * \param[in,out] length \b LPUINT, at input it is length of the \c characters 
 * array, after function call it is actual amount of characters written.
 * \return \b #HLDOM_RESULT
 *
 * parameter \c characters can be NULL. In this case HTMEngine will just set \c length 
 * equal to number of characters in this element.
 *
 * \par Example:
 * for paragraph defined as
 * \verbatim <p>Hello <b>wonderfull</b> <i>world</i>!</p> \endverbatim
 * text will be "Hello \\0 \\0" ('\\0' is character with code zero)
 **/
 EXTERN_C HLDOM_RESULT HLAPI HTMLayoutGetElementText(HELEMENT he, LPWSTR characters, LPUINT length);


/**Get text of the element and information where child elements are placed.
 * \param[in] he \b #HELEMENT
 * \param[out] utf8bytes \b pointer to byte address receiving UTF8 encoded HTML 
 * \param[in] outer \b BOOL, if TRUE will retunr outer HTML otherwise inner.  
 * \return \b #HLDOM_RESULT
 */
 EXTERN_C HLDOM_RESULT HLAPI HTMLayoutGetElementHtml(HELEMENT he, LPBYTE* utf8bytes, BOOL outer);


/**Callback function used with HTMLayoutGetElementHtmlCB().*/
 typedef void CALLBACK HTMLayoutWriterCallbackB( LPCBYTE utf8, UINT utf8_length, LPVOID param );

/**Get text of the element and information where child elements are placed.
 * \param[in] he \b #HELEMENT
 * \param[out] utf8bytes \b pointer to byte address receiving UTF8 encoded HTML 
 * \param[in] outer \b BOOL, if TRUE will retunr outer HTML otherwise inner.  
 * \return \b #HLDOM_RESULT
 */
 EXTERN_C HLDOM_RESULT HLAPI HTMLayoutGetElementHtmlCB(HELEMENT he, BOOL outer, HTMLayoutWriterCallbackB* cb, VOID* cb_param);


/**Get inner text of the element.
 * \param[in] he \b #HELEMENT
 * \param[out] utf8bytes \b pointer to byte address receiving UTF8 encoded plain text 
 * \return \b #HLDOM_RESULT
 */
 EXTERN_C HLDOM_RESULT HLAPI HTMLayoutGetElementInnerText(HELEMENT he, LPBYTE* utf8bytes);

/**Set inner text of the element.
 * \param[in] he \b #HELEMENT
 * \param[in] utf8bytes \b pointer, UTF8 encoded plain text 
 * \param[in] length \b UINT, number of bytes in utf8bytes sequence
 * \return \b #HLDOM_RESULT
 */
 EXTERN_C HLDOM_RESULT HLAPI HTMLayoutSetElementInnerText(HELEMENT he, LPCBYTE utf8bytes, UINT length);


/**Get inner text of the element as LPWSTR (utf16 words).
 * \param[in] he \b #HELEMENT
 * \param[out] utf16words \b pointer to byte address receiving UTF16 encoded plain text 
 * \return \b #HLDOM_RESULT
 */
 EXTERN_C HLDOM_RESULT HLAPI HTMLayoutGetElementInnerText16(HELEMENT he, LPWSTR* utf16words);

/**Callback function used with #HTMLayoutGetElementInnerTextCB().*/
 typedef void CALLBACK HTMLayoutWriterCallbackW( LPCWSTR text, UINT text_length, LPVOID param );

/**Get inner text of the element as LPWSTR (utf16 words).
 * \param[in] he \b #HELEMENT
 * \param[out] HTMLayoutWriterCallbackW \b pointer to callback writer function receiving UTF16 encoded text 
 * \return \b #HLDOM_RESULT
 */
 EXTERN_C HLDOM_RESULT HLAPI HTMLayoutGetElementInnerTextCB(HELEMENT he, HTMLayoutWriterCallbackW* cb, VOID* cb_param);

/**Set inner text of the element from LPCWSTR buffer (utf16 words).
 * \param[in] he \b #HELEMENT
 * \param[in] utf16words \b pointer, UTF16 encoded plain text 
 * \param[in] length \b UINT, number of words in utf16words sequence
 * \return \b #HLDOM_RESULT
 */
 EXTERN_C HLDOM_RESULT HLAPI HTMLayoutSetElementInnerText16(HELEMENT he, LPCWSTR utf16words, UINT length);


/**Get number of element's attributes.
 * \param[in] he \b #HELEMENT
 * \param[out] p_count \b LPUINT, variable to receive number of element 
 * attributes.
 * \return \b #HLDOM_RESULT
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutGetAttributeCount(HELEMENT he, LPUINT p_count);

/**Get value of any element's attribute by attribute's number.
 * \param[in] he \b #HELEMENT 
 * \param[in] n \b UINT, number of desired attribute
 * \param[out] p_name \b LPCSTR*, will be set to address of the string 
 * containing attribute name
 * \param[out] p_value \b LPCWSTR*, will be set to address of the string 
 * containing attribute value
 * \return \b #HLDOM_RESULT
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutGetNthAttribute(HELEMENT he, UINT n, LPCSTR* p_name, LPCWSTR* p_value);

/**Get value of any element's attribute by name.
 * \param[in] he \b #HELEMENT 
 * \param[in] name \b LPCSTR, attribute name
 * \param[out] p_value \b LPCWSTR*, will be set to address of the string 
 * containing attribute value
 * \return \b #HLDOM_RESULT
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutGetAttributeByName(HELEMENT he, LPCSTR name, LPCWSTR* p_value);

/**Set attribute's value.
 * \param[in] he \b #HELEMENT 
 * \param[in] name \b LPCSTR, attribute name
 * \param[in] value \b LPCWSTR, new attribute value or 0 if you want to remove attribute.
 * \return \b #HLDOM_RESULT
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutSetAttributeByName(HELEMENT he, LPCSTR name, LPCWSTR value);

/**Remove all attributes from the element.
 * \param[in] he \b #HELEMENT 
 * \return \b #HLDOM_RESULT
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutClearAttributes(HELEMENT he);

/**Get element index. 
 * \param[in] he \b #HELEMENT 
 * \param[out] p_index \b LPUINT, variable to receive number of the element 
 * among parent element's subelements.
 * \return \b #HLDOM_RESULT
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutGetElementIndex(HELEMENT he, LPUINT p_index);


/**Get element's type.
 * \param[in] he \b #HELEMENT 
 * \param[out] p_type \b LPCSTR*, receives name of the element type.
 * \return \b #HLDOM_RESULT
 * 
 * \par Example:
 * For &lt;div&gt; tag p_type will be set to "div".
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutGetElementType(HELEMENT he, LPCSTR* p_type);

/**Get element's style attribute.
 * \param[in] he \b #HELEMENT 
 * \param[in] name \b LPCSTR, name of the style attribute
 * \param[out] p_value \b LPCWSTR*, variable to receive value of the style attribute.
 *
 * Style attributes are those that are set using css. E.g. "font-face: arial" or "display: block".
 *
 * \sa #HTMLayoutSetStyleAttribute()
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutGetStyleAttribute(HELEMENT he, LPCSTR name, LPCWSTR* p_value);

/**Get element's style attribute.
 * \param[in] he \b #HELEMENT 
 * \param[in] name \b LPCSTR, name of the style attribute
 * \param[out] value \b LPCWSTR, value of the style attribute or NULL for clearing the attribute
 *
 * Style attributes are those that are set using css. E.g. "font-face: arial" or "display: block".
 *
 * \sa #HTMLayoutGetStyleAttribute()
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutSetStyleAttribute(HELEMENT he, LPCSTR name, LPCWSTR value);

/*Get bounding rectangle of the element.
 * \param[in] he \b #HELEMENT 
 * \param[out] p_location \b LPRECT, receives bounding rectangle of the element
 * \param[in] rootRelative \b BOOL, if TRUE function returns location of the 
 * element relative to HTMLayout window, otherwise the location is given 
 * relative to first scrollable container.
 *
 * Use HTMLayoutSetStyleAttribute(he, name, NULL) to remove value that was set previously
 * by this function.
 *
 * \return \b #HLDOM_RESULT
 **/

enum ELEMENT_AREAS 
{
  ROOT_RELATIVE = 0x01,       // - or this flag if you want to get HTMLayout window relative coordinates,
                              //   otherwise it will use nearest windowed container e.g. popup window.
  SELF_RELATIVE = 0x02,       // - "or" this flag if you want to get coordinates relative to the origin
                              //   of element iself.
  CONTAINER_RELATIVE = 0x03,  // - position inside immediate container.
  VIEW_RELATIVE = 0x04,       // - position relative to view - HTMLayout window

  CONTENT_BOX = 0x00,   // content (inner)  box
  PADDING_BOX = 0x10,   // content + paddings
  BORDER_BOX  = 0x20,   // content + paddings + border
  MARGIN_BOX  = 0x30,   // content + paddings + border + margins 
  
  BACK_IMAGE_AREA = 0x40, // relative to content origin - location of background image (if it set no-repeat)
  FORE_IMAGE_AREA = 0x50, // relative to content origin - location of foreground image (if it set no-repeat)

  SCROLLABLE_AREA = 0x60,   // scroll_area - scrollable area in content box 

};

EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutGetElementLocation(HELEMENT he, LPRECT p_location, UINT areas /*ELEMENT_AREAS*/);

/*Scroll to view.
 * \param[in] he \b #HELEMENT 
 * \param[in] toTopOfView \b #BOOL, if TRUE positions element to top of the view 
 * \return \b #HLDOM_RESULT
 **/

enum HTMLAYOUT_SCROLL_FLAGS
{
  SCROLL_TO_TOP = 0x01,
  SCROLL_SMOOTH = 0x10,
};

EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutScrollToView(HELEMENT he, UINT /*HTMLAYOUT_SCROLL_FLAGS*/ flags);


/**Apply changes and refresh element area in its window.
 * \param[in] he \b #HELEMENT 
 * \param[in] remeasure \b BOOL, TRUE if element's dimensions need to be recalculated.
 * \return \b #HLDOM_RESULT
 *
 * This is optional method since v 3.3.0.4   
 * Engine will call update internaly when handling DOM mutating methods.  
 *
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutUpdateElement(HELEMENT he, BOOL remeasure);

enum UPDATE_ELEMENT_FLAGS
{
  RESET_STYLE_THIS = 0x20,  // reset styles - this may require if you have styles dependent from attributes,
  RESET_STYLE_DEEP = 0x10,  // use these flags after SetAttribute then. RESET_STYLE_THIS is faster than RESET_STYLE_DEEP.

  MEASURE_INPLACE = 0x0001, // use this flag if you do not expect any dimensional changes - this is faster than REMEASURE
  MEASURE_DEEP    = 0x0002, // use this flag if changes of some attributes/content may cause change of dimensions of the element  

  REDRAW_NOW = 0x8000,      // invoke ::UpdateWindow function after applying changes
};

/**Apply changes and refresh element area in its window.
 * \param[in] he \b #HELEMENT 
 * \param[in] flags \b UINT, combination of UPDATE_ELEMENT_FLAGS.
 * \return \b #HLDOM_RESULT
 *
 *  Note HTMLayoutUpdateElement is an equivalent of HTMLayoutUpdateElementEx(,RESET_STYLE_DEEP | REMEASURE [| REDRAW_NOW ])
 *
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutUpdateElementEx(HELEMENT he, UINT flags);


/**Set the mouse capture to the specified element.
 * \param[in] he \b #HELEMENT 
 * \return \b #HLDOM_RESULT
 *
 * After call to this function all mouse events will be targeted to the element.
 * To remove mouse capture call ::ReleaseCapture() function. It is declared somewhere in <windows.h>.
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutSetCapture(HELEMENT he);

/**Set event root the specified element.
 * \param[in] he \b #HELEMENT 
 * \param[out] phePrevRoot \b #HELEMENT* - previous event root element
 * \return \b #HLDOM_RESULT
 *
 * After call to this function all events will be targeted to the element and its sub elements.
 * All elements outside of the EventRoot will behave as if they are disabled
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutSetEventRoot(HELEMENT he, HELEMENT *phePrevRoot);


/**Get HWND of containing window.
 * \param[in] he \b #HELEMENT 
 * \param[out] p_hwnd \b HWND*, variable to receive window handle
 * \param[in] rootWindow \b BOOL, handle of which window to get:
 * - TRUE - HTMLayout window
 * - FALSE - nearest parent element having overflow:auto or :scroll
 * \return \b #HLDOM_RESULT
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutGetElementHwnd(HELEMENT he, HWND* p_hwnd, BOOL rootWindow);



/**Combine given URL with URL of the document element belongs to.
 * \param[in] he \b #HELEMENT
 * \param[in, out] szUrlBuffer \b LPWSTR, at input this buffer contains 
 * zero-terminated URL to be combined, after function call it contains 
 * zero-terminated combined URL
 * \param[in] UrlBufferSize \b DWORD, size of the buffer pointed by 
 * \c szUrlBuffer
 * \return \b #HLDOM_RESULT
 *
 * This function is used for resolving relative references.
 **/
EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutCombineURL(HELEMENT he, LPWSTR szUrlBuffer, DWORD UrlBufferSize);

/**Callback function used with #HTMLayoutVisitElement().*/
typedef BOOL CALLBACK HTMLayoutElementCallback( HELEMENT he, LPVOID param );

/**Call specified function for every element in a DOM that meets specified 
 * criteria.
 * \param[in] he \b #HELEMENT
 * \param[in] tagName \b LPCSTR, comma separated list of tag names to search, e.g. "div", "p", "div,p" etc.
 * Can be NULL.
 * \param[in] attributeName \b LPCSTR, name of attribute, can 
 * contain wildcard characters, see below. Can be NULL.
 * \param[in] attributeValue \b LPCWSTR, value of attribute, 
 * can contain wildcard characters, see below. Can be NULL.
 * \param[in] callback \b #HTMLayoutElementCallback*, address of callback 
 * function being called on each element found.
 * \param[in] param \b LPVOID, additional parameter to be passed to callback 
 * function.
 * \param[in] depth \b DWORD, depth - depth of search. 0 means all descendants, 1 - direct children only, 
 * 2 - children and their children and so on.
 * \return \b #HLDOM_RESULT
 *
 * Wildcard characters in attributeName and attributeValue:
 * - '*' - any substring
 * - '?' - any one char
 * - '['char set']' = any one char in set
 *
 * \par Example:
 * - [a-z] - all lowercase letters
 * - [a-zA-Z] - all letters
 * - [abd-z] - all lowercase letters except of 'c'
 * - [-a-z] - all lowercase letters and '-'
 **/

EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutVisitElements(
          HELEMENT  he, 
          LPCSTR    tagName,
          LPCSTR    attributeName, 
          LPCWSTR   attributeValue, 
          HTMLayoutElementCallback* 
                    callback, 
          LPVOID    param,
          DWORD     depth);

/**Call specified function for every element in a DOM that meets specified 
 * CSS selectors. 
 * See list of supported selectors: http://terrainformatica.com/htmlayout/selectors.whtm
 * \param[in] he \b #HELEMENT
 * \param[in] selector \b LPCSTR, comma separated list of CSS selectors, e.g.: div, #id, div[align="right"].
 * \param[in] callback \b #HTMLayoutElementCallback*, address of callback 
 * function being called on each element found.
 * \param[in] param \b LPVOID, additional parameter to be passed to callback 
 * function.
 * \return \b #HLDOM_RESULT
 *
 * Note that :root pseudo-element matches element 'he' here.
 *
 **/

EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutSelectElements(
          HELEMENT  he, 
          LPCSTR    CSS_selectors,
          HTMLayoutElementCallback* 
                    callback, 
          LPVOID    param);

EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutSelectElementsW(
          HELEMENT  he, 
          LPCWSTR   CSS_selectors,
          HTMLayoutElementCallback* 
                    callback, 
          LPVOID    param);


/**Find parent of the element by CSS selector. 
 * ATTN: function will test first element itself. 
 * See list of supported selectors: http://terrainformatica.com/htmlayout/selectors.whtm
 * \param[in] he \b #HELEMENT
 * \param[in] selector \b LPCSTR, comma separated list of CSS selectors, e.g.: div, #id, div[align="right"].
 * \param[out] heFound \b #HELEMENT*, address of result HELEMENT 
 * \param[in] depth \b LPVOID, depth of search, if depth == 1 then it will test only element itself. 
 *                     Use depth = 1 if you just want to test he element for matching given CSS selector(s). 
 *                     depth = 0 will scan the whole child parent chain up to the root.
 * \return \b #HLDOM_RESULT
 *
 **/

EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutSelectParent(
          HELEMENT  he, 
          LPCSTR    selector,
          UINT      depth,
          /*out*/ HELEMENT* heFound);

EXTERN_C  HLDOM_RESULT HLAPI HTMLayoutSelectParentW(
          HELEMENT  he, 
          LPCWSTR   selector,
          UINT      depth,
          /*out*/ HELEMENT* heFound);


#define SIH_REPLACE_CONTENT     0 
#define SIH_INSERT_AT_START     1 
#define SIH_APPEND_AFTER_LAST   2 

#define SOH_REPLACE             3 
#define SOH_INSERT_BEFORE       4  
#define SOH_INSERT_AFTER        5  

/**Set inner or outer html of the element.
 * \param[in] he \b #HELEMENT
 * \param[in] html \b LPCBYTE, UTF-8 encoded string containing html text
 * \param[in] htmlLength \b DWORD, length in bytes of \c html.
 * \param[in] where \b UINT, possible values are:
 * - SIH_REPLACE_CONTENT - replace content of the element
 * - SIH_INSERT_AT_START - insert html before first child of the element
 * - SIH_APPEND_AFTER_LAST - insert html after last child of the element
 *
 * - SOH_REPLACE - replace element by html, a.k.a. element.outerHtml = "something"
 * - SOH_INSERT_BEFORE - insert html before the element
 * - SOH_INSERT_AFTER - insert html after the element
 *   ATTN: SOH_*** operations do not work for inline elements like <SPAN>
 *
 * \return /b #HLDOM_RESULT
  **/
EXTERN_C HLDOM_RESULT HLAPI 
      HTMLayoutSetElementHtml(HELEMENT he, LPCBYTE html, DWORD htmlLength, UINT where);

/**Delete element.
 * \param[in] he \b #HELEMENT
 * \return \b #HLDOM_RESULT
 *
 * This function removes element from the DOM tree and then deletes it.
 *
 * \warning After call to this function \c he will become invalid.
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutDeleteElement(HELEMENT he);


/** Element UID support functions.
 *  
 *  Element UID is unique identifier of the DOM element. 
 *  UID is suitable for storing it in structures associated with the view/document.
 *  Access to the element using HELEMENT is more effective but table space of handles is limited.
 *  It is not recommended to store HELEMENT handles between function calls.
 **/


/** Get Element UID.
 * \param[in] he \b #HELEMENT
 * \param[out] puid \b UINT*, variable to receive UID of the element.
 * \return \b #HLDOM_RESULT
 *
 * This function retrieves element UID by its handle.
 *
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutGetElementUID(HELEMENT he, UINT* puid);

/** Get Element handle by its UID.
 * \param[in] hwnd \b HWND, HWND of HTMLayout window
 * \param[in] uid \b UINT
 * \param[out] phe \b #HELEMENT*, variable to receive HELEMENT handle 
 * \return \b #HLDOM_RESULT
 *
 * This function retrieves element UID by its handle.
 *
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutGetElementByUID(HWND hwnd, UINT uid, HELEMENT* phe);


/** Shows block element (DIV) in popup window.
 * \param[in] hePopup \b HELEMENT, element to show as popup
 * \param[in] heAnchor \b HELEMENT, anchor element - hePopup will be shown near this element
 * \param[in] placement \b UINT, values: 
 *     2 - popup element below of anchor
 *     8 - popup element above of anchor
 *     4 - popup element on left side of anchor
 *     6 - popup element on right side of anchor
 *     ( see numpad on keyboard to get an idea of the numbers)
 * \return \b #HLDOM_RESULT
 *
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutShowPopup(HELEMENT hePopup, HELEMENT heAnchor, UINT placement);


/** Shows block element (DIV) in popup window at given position.
 * \param[in] hePopup \b HELEMENT, element to show as popup
 * \param[in] pos \b POINT, popup element position, relative to origin of HTMLayout window.
 * \param[in] mode \b BOOL, LOWORD=1 if animation is needed. HIWORD 1..9 - point of the popup that corresponds to pos 
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutShowPopupAt(HELEMENT hePopup, POINT pos, UINT mode);


/** Shows block element (menu.popup) in popup window at given position and runs popup loop.
 *  function will return when the menu dismissed.
 * \param[in] hePopup \b HELEMENT, element to show as popup
 * \param[in] pos \b POINT, popup element position, relative to origin of HTMLayout window.
 * \param[in] mode \b BOOL, LOWORD=1 if animation is needed. HIWORD 1..9 - point of the popup that corresponds to pos
 * \param[out] pheItem \b HELEMENT*, menu item that was choosen (if any). 
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutTrackPopupAt(HELEMENT hePopup, POINT posRoot, UINT mode, HELEMENT* pheItem);


/** Removes popup window.
 * \param[in] he \b HELEMENT, element which belongs to popup window or popup element itself
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutHidePopup(HELEMENT he);



/**Element callback function for all types of events. Similar to WndProc
 * \param tag \b LPVOID, tag assigned by HTMLayoutAttachElementProc function (like GWL_USERDATA)
 * \param he \b HELEMENT, this element handle (like HWND)
 * \param evtg \b UINT, group identifier of the event, value is one of EVENT_GROUPS
 * \param prms \b LPVOID, pointer to group specific parameters structure.
 * \return TRUE if event was handled, FALSE otherwise.
 **/

typedef BOOL CALLBACK ElementEventProc(LPVOID tag, HELEMENT he, UINT evtg, LPVOID prms );
typedef ElementEventProc* LPELEMENT_EVENT_PROC;

/**
  * Element state flags:
  **/

enum ELEMENT_STATE_BITS 
{
   STATE_LINK             = 0x00000001,
   STATE_HOVER            = 0x00000002,
   STATE_ACTIVE           = 0x00000004,
   STATE_FOCUS            = 0x00000008,
   STATE_VISITED          = 0x00000010,
   STATE_CURRENT          = 0x00000020,  // current (hot) item 
   STATE_CHECKED          = 0x00000040,  // element is checked (or selected)
   STATE_DISABLED         = 0x00000080,  // element is disabled
   STATE_READONLY         = 0x00000100,  // readonly input element 
   STATE_EXPANDED         = 0x00000200,  // expanded state - nodes in tree view 
   STATE_COLLAPSED        = 0x00000400,  // collapsed state - nodes in tree view - mutually exclusive with
   STATE_INCOMPLETE       = 0x00000800,  // one of fore/back images requested but not delivered
   STATE_ANIMATING        = 0x00001000,  // is animating currently
   STATE_FOCUSABLE        = 0x00002000,  // will accept focus
   STATE_ANCHOR           = 0x00004000,  // anchor in selection (used with current in selects)
   STATE_SYNTHETIC        = 0x00008000,  // this is a synthetic element - don't emit it's head/tail
   STATE_OWNS_POPUP       = 0x00010000,  // this is a synthetic element - don't emit it's head/tail
   STATE_TABFOCUS         = 0x00020000,  // focus gained by tab traversal
   STATE_EMPTY            = 0x00040000,  // empty - element is empty (text.size() == 0 && subs.size() == 0) 
                                         //  if element has behavior attached then the behavior is responsible for the value of this flag.
   STATE_BUSY             = 0x00080000,  // busy; loading
   
   STATE_DRAG_OVER        = 0x00100000,  // drag over the block that can accept it (so is current drop target). Flag is set for the drop target block
   STATE_DROP_TARGET      = 0x00200000,  // active drop target.
   STATE_MOVING           = 0x00400000,  // dragging/moving - the flag is set for the moving block.
   STATE_COPYING          = 0x00800000,  // dragging/copying - the flag is set for the copying block.
   STATE_DRAG_SOURCE      = 0x01000000,  // element that is a drag source.
   STATE_DROP_MARKER      = 0x02000000,  // element is drop marker
   
   STATE_PRESSED          = 0x04000000,  // pressed - close to active but has wider life span - e.g. in MOUSE_UP it 
                                         //   is still on; so behavior can check it in MOUSE_UP to discover CLICK condition.
   STATE_POPUP            = 0x08000000,  // this element is out of flow - popup 
   STATE_IS_LTR           = 0x10000000,  // the element or one of its containers has dir=ltr declared
   STATE_IS_RTL           = 0x20000000,  // the element or one of its containers has dir=rtl declared

};

/* how they looked like prior v 3.3.1.42
enum ELEMENT_STATE_BITS 
{
    STATE_LINK        = 0x00000001,  // selector :link,    any element having href attribute
    STATE_HOVER       = 0x00000002,  // selector :hover,   element is under the cursor, mouse hover  
    STATE_ACTIVE      = 0x00000004,  // selector :active,  element is activated, e.g. pressed  
    STATE_FOCUS       = 0x00000008,  // selector :focus,   element is in focus  
    STATE_VISITED     = 0x00000010,  // selector :visited, aux flag - not used internally now.
    STATE_CURRENT     = 0x00000020,  // selector :current, current item in collection, e.g. current <option> in <select>
    STATE_CHECKED     = 0x00000040,  // selector :checked, element is checked (or selected), e.g. check box or itme in multiselect
    STATE_DISABLED    = 0x00000080,  // selector :disabled, element is disabled, behavior related flag.
    STATE_READONLY    = 0x00000100,  // selector :read-only, element is read-only, behavior related flag.
    STATE_EXPANDED    = 0x00000200,  // selector :expanded, element is in expanded state - nodes in tree view e.g. <options> in <select>
    STATE_COLLAPSED   = 0x00000400,  // selector :collapsed, mutually exclusive with EXPANDED
    STATE_INCOMPLETE  = 0x00000800,  // selector :incomplete, element has images (back/fore/bullet) requested but not delivered.
    STATE_ANIMATING   = 0x00001000,  // selector :animating, is currently animating 
    STATE_FOCUSABLE   = 0x00002000,  // selector :focusable, shall accept focus
    STATE_ANCHOR      = 0x00004000,  // selector :anchor, first element in selection (<select miltiple>), STATE_CURRENT is the current.
    STATE_SYNTHETIC   = 0x00008000,  // selector :synthetic, synthesized DOM elements - e.g. all missed cells in tables (<td>) are getting this flag
    STATE_OWNS_POPUP  = 0x00010000,  // selector :owns-popup, anchor(owner) element of visible popup. 
    STATE_TABFOCUS    = 0x00020000,  // selector :tab-focus, element got focus by tab traversal. engine set it together with :focus.
    STATE_EMPTY       = 0x00040000,  // selector :empty - element is empty. 
    STATE_BUSY        = 0x00080000,  // selector :busy, element is busy. HTMLayoutRequestElementData will set this flag if
                                     // external data was requested for the element. When data will be delivered engine will reset this flag on the element. 

    STATE_DRAG_OVER   = 0x00100000,  // drag over the block that can accept it (so is current drop target). Flag is set for the drop target block. At any given moment of time it can be only one such block.
    STATE_DROP_TARGET = 0x00200000,  // active drop target. Multiple elements can have this flag when D&D is active. 
    STATE_MOVING      = 0x00400000,  // dragging/moving - the flag is set for the moving element (copy of the drag-source).
    STATE_COPYING     = 0x00800000,  // dragging/copying - the flag is set for the copying element (copy of the drag-source).
    STATE_DRAG_SOURCE = 0x00C00000,  // is set in element that is being dragged.

    STATE_POPUP       = 0x40000000,  // this element is in popup state and presented to the user - out of flow now
    STATE_PRESSED     = 0x04000000,  // pressed - close to active but has wider life span - e.g. in MOUSE_UP it 
                                     // is still on, so behavior can check it in MOUSE_UP to discover CLICK condition.
    STATE_HAS_CHILDREN= 0x02000000,  // has more than one child.    
    STATE_HAS_CHILD   = 0x01000000,  // has single child.
                               
    STATE_IS_LTR      = 0x20000000,  // selector :ltr, the element or one of its nearest container has @dir and that dir has "ltr" value
    STATE_IS_RTL      = 0x10000000,  // selector :rtl, the element or one of its nearest container has @dir and that dir has "rtl" value    

}; */

/** Get/set state bits, stateBits*** accept or'ed values above
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutGetElementState( HELEMENT he, UINT* pstateBits);

/**
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutSetElementState( HELEMENT he, UINT stateBitsToSet, UINT stateBitsToClear, BOOL updateView);

/** Create new element, the element is disconnected initially from the DOM.
    Element created with ref_count = 1 thus you \b must call HTMLayout_UnuseElement on returned handler.
 * \param[in] tagname \b LPCSTR, html tag of the element e.g. "div", "option", etc.
 * \param[in] textOrNull \b LPCWSTR, initial text of the element or NULL. text here is a plain text - method does no parsing.
 * \param[out ] phe \b #HELEMENT*, variable to receive handle of the element
  **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutCreateElement( LPCSTR tagname, LPCWSTR textOrNull, /*out*/ HELEMENT *phe );

/** Create new element as copy of existing element, new element is a full (deep) copy of the element and
    is disconnected initially from the DOM.
    Element created with ref_count = 1 thus you \b must call HTMLayout_UnuseElement on returned handler.
 * \param[in] he \b #HELEMENT, source element.
 * \param[out ] phe \b #HELEMENT*, variable to receive handle of the new element. 
  **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutCloneElement( HELEMENT he, /*out*/ HELEMENT *phe );

/** Insert element at \i index position of parent.
    It is not an error to insert element which already has parent - it will be disconnected first, but 
    you need to update elements parent in this case. 
 * \param index \b UINT, position of the element in parent collection. 
   It is not an error to provide index greater than elements count in parent -
   it will be appended.
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutInsertElement( HELEMENT he, HELEMENT hparent, UINT index );

/** Take element out of its container (and DOM tree). 
    Element will be destroyed when its reference counter will become zero
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutDetachElement( HELEMENT he );

/** Start Timer for the element. 
    Element will receive on_timer event
    To stop timer call HTMLayoutSetTimer( he, 0 );
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutSetTimer( HELEMENT he, UINT milliseconds );

/** Start Extended Timer for the element. 
    Element will receive on_timer(, timerId) event
    To stop timer call HTMLayoutSetTimerEx( he, 0, timerId);
 * \param timerId \b UINT_OTR, arbitrary value that will be deliverd to the behavior "as is". 
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutSetTimerEx( HELEMENT he, UINT milliseconds, UINT_PTR timerId );



/** Attach/Detach ElementEventProc to the element 
    See htmlayout::event_handler.
 **/
 
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutAttachEventHandler( HELEMENT he, LPELEMENT_EVENT_PROC pep, LPVOID tag );
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutDetachEventHandler( HELEMENT he, LPELEMENT_EVENT_PROC pep, LPVOID tag );

/** Attach ElementEventProc to the element and subscribe it to events providede by subscription parameter
    See htmlayout::attach_event_handler.
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutAttachEventHandlerEx( HELEMENT he, LPELEMENT_EVENT_PROC pep, LPVOID tag, UINT subscription );


/** Attach/Detach ElementEventProc to the htmlayout window. 
    All events will start first here (in SINKING phase) and if not consumed will end up here.
    You can install Window EventHandler only once - it will survive all document reloads.
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutWindowAttachEventHandler( HWND hwndLayout, LPELEMENT_EVENT_PROC pep, LPVOID tag, UINT subscription );
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutWindowDetachEventHandler( HWND hwndLayout, LPELEMENT_EVENT_PROC pep, LPVOID tag );


/** SendEvent - sends sinking/bubbling event to the child/parent chain of he element.
    First event will be send in SINKING mode (with SINKING flag) - from root to he element itself.
    Then from he element to its root on parents chain without SINKING flag (bubbling phase).

 * \param[in] he \b HELEMENT, element to send this event to.
 * \param[in] appEventCode \b UINT, event ID, see: #BEHAVIOR_EVENTS
 * \param[in] heSource \b HELEMENT, optional handle of the source element, e.g. some list item
 * \param[in] reason \b UINT, notification specific event reason code
 * \param[out] handled \b BOOL*, variable to receive TRUE if any handler handled it, FALSE otherwise.

 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutSendEvent(
          HELEMENT he, UINT appEventCode, HELEMENT heSource, UINT_PTR reason, /*out*/ BOOL* handled);


/** PostEvent - post sinking/bubbling event to the child/parent chain of he element.
 *  Function will return immediately posting event into input queue of the application. 
 *
 * \param[in] he \b HELEMENT, element to send this event to.
 * \param[in] appEventCode \b UINT, event ID, see: #BEHAVIOR_EVENTS
 * \param[in] heSource \b HELEMENT, optional handle of the source element, e.g. some list item
 * \param[in] reason \b UINT, notification specific event reason code

 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutPostEvent(
          HELEMENT he, UINT appEventCode, HELEMENT heSource, UINT reason);

typedef struct _METHOD_PARAMS METHOD_PARAMS;

/** HTMLayoutCallMethod - calls behavior specific method.
 * \param[in] he \b HELEMENT, element - source of the event.
 * \param[in] params \b METHOD_PARAMS, pointer to method param block
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutCallBehaviorMethod(
          HELEMENT he, METHOD_PARAMS* params);

/** HTMLayoutRequestElementData  - request data download for the element.
 * \param[in] he \b HELEMENT, element to deleiver data to.
 * \param[in] url \b LPCWSTR, url to download data from.
 * \param[in] dataType \b UINT, data type, see HTMLayoutResourceType.
 * \param[in] hInitiator \b HELEMENT, element - initiator, can be NULL.

  event handler on the he element (if any) will be notified 
  when data will be ready by receiving HANDLE_DATA_DELIVERY event.

  **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutRequestElementData(
          HELEMENT he, LPCWSTR url, UINT dataType, HELEMENT initiator );


/**
 *  HTMLayoutSendRequest - send GET or POST request for the element
 *
 * event handler on the 'he' element (if any) will be notified 
 * when data will be ready by receiving HANDLE_DATA_DELIVERY event.
 *
 **/ 

enum REQUEST_TYPE
{
  GET_ASYNC,  // async GET
  POST_ASYNC, // async POST
};

typedef struct { LPCWSTR name; LPCWSTR value; } REQUEST_PARAM;

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutHttpRequest( 
          HELEMENT        he,           // element to deliver data 
          LPCWSTR         url,          // url 
          UINT            dataType,     // data type, see HTMLayoutResourceType.
          UINT            requestType,  // one of REQUEST_TYPE values
          REQUEST_PARAM*  requestParams,// parameters 
          UINT            nParams       // number of parameters 
          );

/** HTMLayoutGetScrollInfo  - get scroll info of element with overflow:scroll or auto.
 * \param[in] he \b HELEMENT, element.
 * \param[out] scrollPos \b LPPOINT, scroll position.
 * \param[out] viewRect \b LPRECT, position of element scrollable area, content box minus scrollbars.
 * \param[out] contentSize \b LPSIZE, size of scrollable element content.
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutGetScrollInfo( 
         HELEMENT he, LPPOINT scrollPos, LPRECT viewRect, LPSIZE contentSize );

/** HTMLayoutSetScrollPos  - set scroll position of element with overflow:scroll or auto.
 * \param[in] he \b HELEMENT, element.
 * \param[in] scrollPos \b POINT, new scroll position.
 * \param[in] smooth \b BOOL, TRUE - do smooth scroll.
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutSetScrollPos( 
         HELEMENT he, POINT scrollPos, BOOL smooth );

/** HTMLayoutGetElementIntrinsicWidths  - get min-intrinsic and max-intrinsic widths of the element.
 * \param[in] he \b HELEMENT, element.
 * \param[out] pMinWidth \b INT*, calculated min-intrinsic width.
 * \param[out] pMaxWidth \b INT*, calculated max-intrinsic width.
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutGetElementIntrinsicWidths( HELEMENT he, INT* pMinWidth, INT* pMaxWidth );

/** HTMLayoutGetElementIntrinsicHeight  - get min-intrinsic height of the element calculated for forWidth.
 * \param[in] he \b HELEMENT, element.
 * \param[in] forWidth \b INT*, width to calculate intrinsic height for.
 * \param[out] pHeight \b INT*, calculated min-intrinsic height.
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutGetElementIntrinsicHeight( HELEMENT he, INT forWidth, INT* pHeight );


/** HTMLayoutIsElementVisible - deep visibility, determines if element visible - has no visiblity:hidden and no display:none defined 
    for itself or for any its parents.
 * \param[in] he \b HELEMENT, element.
 * \param[out] pVisible \b LPBOOL, visibility state.
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutIsElementVisible( HELEMENT he, BOOL* pVisible);

/** HTMLayoutIsElementEnabled - deep enable state, determines if element enabled - is not disabled by itself or no one  
    of its parents is disabled.
 * \param[in] he \b HELEMENT, element.
 * \param[out] pEnabled \b LPBOOL, enabled state.
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutIsElementEnabled( HELEMENT he, BOOL* pEnabled );


/**Callback comparator function used with #HTMLayoutSortElements().
   Shall return -1,0,+1 values to indicate result of comparison of two elements
 **/
typedef INT CALLBACK ELEMENT_COMPARATOR( HELEMENT he1, HELEMENT he2, LPVOID param );

/** HTMLayoutSortElements - sort children of the element.
 * \param[in] he \b HELEMENT, element which children to be sorted.
 * \param[in] firstIndex \b UINT, first child index to start sorting from.
 * \param[in] lastIndex \b UINT, last index of the sorting range, element with this index will not be included in the sorting.
 * \param[in] cmpFunc \b ELEMENT_COMPARATOR, comparator function.
 * \param[in] cmpFuncParam \b LPVOID, parameter to be passed in comparator function.
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutSortElements( 
         HELEMENT he, UINT firstIndex, UINT lastIndex, 
         ELEMENT_COMPARATOR* cmpFunc, LPVOID cmpFuncParam );

/** HTMLayoutSwapElements - swap element positions.
 * Function changes "insertion points" of two elements. So it swops indexes and parents of two elements.
 * \param[in] he1 \b HELEMENT, first element.
 * \param[in] he2 \b HELEMENT, second element.
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutSwapElements( 
         HELEMENT he1, HELEMENT he2 );



/** HTMLayoutTraverseUIEvent - traverse (sink-and-bubble) MOUSE or KEY event.
 * \param[in] evt \b EVENT_GROUPS, either HANDLE_MOUSE or HANDLE_KEY code.
 * \param[in] eventCtlStruct \b LPVOID, pointer on either MOUSE_PARAMS or KEY_PARAMS structure.
 * \param[out] bOutProcessed \b LPBOOL, pointer to BOOL receiving TRUE if event was processed by some element and FALSE otherwise.
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutTraverseUIEvent( 
         UINT evt, LPVOID eventCtlStruct, LPBOOL bOutProcessed );


/** HTMLayoutProcessUIEvent - request to process MOUSE or KEY on single element without bubbling.
 * \param[in] he \b HELEMENT, element to process event on.
 * \param[in] evt \b EVENT_GROUPS, either HANDLE_MOUSE or HANDLE_KEY code.
 * \param[in] eventCtlStruct \b LPVOID, pointer on either MOUSE_PARAMS or KEY_PARAMS structure.
 * \param[out] bOutProcessed \b LPBOOL, pointer to BOOL receiving TRUE if event was processed by some element and FALSE otherwise.
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutProcessUIEvent( 
         HELEMENT he, UINT evt, LPVOID eventCtlStruct, LPBOOL bOutProcessed );


/** Control types.
 *  Control here is any dom element having appropriate behavior applied
 **/

enum CTL_TYPE
{
    CTL_NO,               ///< This dom element has no behavior at all.
    CTL_UNKNOWN = 1,      ///< This dom element has behavior but its type is unknown.
    CTL_EDIT,             ///< Single line edit box.
    CTL_NUMERIC,          ///< Numeric input with optional spin buttons.
    CTL_BUTTON,           ///< Command button.
    CTL_CHECKBOX,         ///< CheckBox (button).
    CTL_RADIO,            ///< OptionBox (button).
    CTL_SELECT_SINGLE,    ///< Single select, ListBox or TreeView.
    CTL_SELECT_MULTIPLE,  ///< Multiselectable select, ListBox or TreeView.
    CTL_DD_SELECT,        ///< Dropdown single select.
    CTL_TEXTAREA,         ///< Multiline TextBox.
    CTL_HTMLAREA,         ///< WYSIWYG HTML editor.
    CTL_PASSWORD,         ///< Password input element.
    CTL_PROGRESS,         ///< Progress element.
    CTL_SLIDER,           ///< Slider input element.  
    CTL_DECIMAL,          ///< Decimal number input element.
    CTL_CURRENCY,         ///< Currency input element.
    CTL_SCROLLBAR,
    
    CTL_HYPERLINK,
    
    CTL_MENUBAR,
    CTL_MENU,
    CTL_MENUBUTTON,

    CTL_CALENDAR,
    CTL_DATE,
    CTL_TIME,
    
    CTL_FRAME,
    CTL_FRAMESET,

    CTL_GRAPHICS,
    CTL_SPRITE,

    CTL_LIST,
    CTL_RICHTEXT,
    CTL_TOOLTIP,

    CTL_HIDDEN,
    CTL_URL,            ///< URL input element.
    CTL_TOOLBAR,

    CTL_FORM,

};

/** HTMLayoutControlGetType - get type of control - type of behavior assigned to the element.
 * \param[in] he \b HELEMENT, element.
 * \param[out] pType \b UINT*, pointer to variable receiving control type,
 *             for list of values see CTL_TYPE.
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutControlGetType( HELEMENT he, /*CTL_TYPE*/ UINT *pType );

/** HTMLayoutControlGetValue - get value of the control.
 * \param[in] he \b HELEMENT, element.
 * \param[out] pVal \b VALUE*, pointer to variable receiving control value. After use of this value ValueClear MUST be 
 *                             called on it. Otherwise memory leak will happen!
 *
 * *pVal - variable shall be properly initialized before the call
 *
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutControlGetValue( HELEMENT he, VALUE *pVal );

// ATTENTION:
// ATTENTION:
// ATTENTION: ValueClear(pVal); must be called at some point HTMLayoutControlGetValue use.
// ATTENTION:
// ATTENTION:


/** HTMLayoutControlSetValue - set value of the control and update UI.
 * \param[in] he \b HELEMENT, element.
 * \param[in] pVal \b const JSON_VALUE*, pointer to variable to set value from.
 *
 * *pVal - variable shall contain valid JSON_VALUE. 
 *
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutControlSetValue( HELEMENT he, const VALUE *pVal );


/**Callback function used with #HTMLayoutEnumearate().
 * \param[in] he \b HELEMENT, element.
 * \param[in] pos \b int, position in the element.
 * \param[in] postype \b int, position type : 
        0 - he element head position.
        1 - he element tail position.
        2 - character position.
 * \param[in] code \b int, UTF16 code unit value if postype == 3 
 **/

typedef BOOL CALLBACK HTMLayoutEnumerationCallback( LPVOID p, HELEMENT he, int pos, int postype, WCHAR code );

/** HTMLayoutEnumerate - character by character enumeartion of the dom.
 * \param[in] he \b HELEMENT, element.
 * \param[in] pcb \b HTMLayoutEnumearationCallback, pointer to function that is called on each character position.
 * \param[in] forward \b BOOL, direction of the enumeration, TRUE - forward.
 *
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutEnumerate( HELEMENT he, HTMLayoutEnumerationCallback* pcb, LPVOID p, BOOL forward );


/** HTMLayoutGetCharacterRect - position of single character on the screen.
 * \param[in] he \b HELEMENT, element.
 * \param[in] pos \b int, index of the character in the he element.
 * \param[out] outRect \b RECT, rectangle that will receive view relative coordinates of the character placeholder.
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutGetCharacterRect( HELEMENT he, int pos, RECT* outRect );


/** HTMLayoutCSSRuleDef is definitiion of css rule used by
 *
 * - HTMLayoutStyleRuleCallback
 * \copydoc HTMLayoutStyleRuleCallback
 * 
 **/

typedef struct 
{
  INT      rule_type;    /**< 0 - css rule, 
                              1 - inline style attribute (selector is text of @style), 
                              2 - element has styles defined by HTMLayoutSetStyleAttribute */
  LPCSTR   file_url;     /**< url of the file where this rule is defined */
  INT      file_line_no; /**< line number in the file of rule's selector, -1 if unknown */
  LPCWSTR  selector;     /**< text of selector */
} HTMLayoutCSSRuleDef;

typedef void CALLBACK HTMLayoutStyleRuleCallback( HTMLayoutCSSRuleDef* pdef, LPVOID callback_prm );

/** HTMLayoutEnumElementStyles - enums CSS rules applied to the element in its currnet state.
 * \param[in] he \b HELEMENT, element.
 * \param[in] callback \b HTMLayoutStyleRuleCallback*, callback function.
 * \param[in] callback_prm \b LPVOID, parameter of the callback function.
 *
 * HTMLayoutEnumElementStyles enumerates all styles applied to the element (including master CSS rules).
 * It also reports if the element has local style attribute declaration and style attributes set in runtime.
 *
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutEnumElementStyles( HELEMENT he, HTMLayoutStyleRuleCallback* callback, LPVOID callback_prm );

typedef struct _HTMLayoutElementExpando HTMLayoutElementExpando;

typedef void CALLBACK ExpandoRelease( HTMLayoutElementExpando* pexp, HELEMENT he );

struct _HTMLayoutElementExpando
{
  ExpandoRelease* finalizer; // can be either NULL or valid pointer to function
};


/** HTMLayoutElementSetExpando - allows to associate user struct or class (Expando) with the DOM element.
 * \param[in] he \b HELEMENT, element.
 * \param[in] pExpando \b HTMLayoutElementExpando*, ptr to user object of class derived from HTMLayoutElementExpando.
 *
 * HTMLayoutElementSetExpando together with HTMLayoutElementGetExpando allows to associate user struct or class (Expando) with the DOM element.
 * \see dom::expando in htmlayout_dom.hpp
 *
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutElementSetExpando( HELEMENT he, HTMLayoutElementExpando* pExpando );

/** HTMLayoutElementGetExpando - retrives pointer to the Expando object set by HTMLayoutElementSetExpando.
 * \param[in] he \b HELEMENT, element.
 * \param[in] ppExpando \b HTMLayoutElementExpando**, ptr to location where to copy HTMLayoutElementExpando*.
 *
 * \see dom::expando in htmlayout_dom.hpp
 *
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutElementGetExpando( HELEMENT he, HTMLayoutElementExpando** ppExpando );


/** HTMLayoutMoveElement - moves element from its normal place to the position defined by xView, yView.
 *
 * \param[in] he \b HELEMENT, element.
 * \param[in] xView \b INT, new x coordinate of content box of the element relative to the view - htmlayout window.
 * \param[in] yView \b INT, new y coordinate of content box of the element relative to the view - htmlayout window.
 *
 * If element is moved outside of the view then HTMLayoutMoveElement will create popup window for it.
 *
 **/

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutMoveElement( HELEMENT he, INT xView, INT yView);

/** HTMLayoutMoveElementEx - moves and resizes the element from its normal place to the position defined by xView, yView.
 *
 * \param[in] he \b HELEMENT, element.
 * \param[in] xView \b INT, new x coordinate of content box of the element relative to the view - htmlayout window.
 * \param[in] yView \b INT, new y coordinate of content box of the element relative to the view - htmlayout window.
 * \param[in] width \b INT, new width of content box of the element relative to the view - htmlayout window.
 * \param[in] height \b INT, new height of content box of the element relative to the view - htmlayout window.
 *
 * If element is moved outside of the view then HTMLayoutMoveElement will create popup window for it.
 *
 **/
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutMoveElementEx( HELEMENT he, INT xView, INT yView,INT width, INT height);


/** Element animator function used with #HTMLayoutAnimateElement().
  * Returns number of milliseconds for the next animation step or zero to stop animation.
  * \param[in] step \b UINT, is a number of times this function was invoked for the element.
  *                          Starts from 0. When animation is stoped step will be equal to UINT(-1) and
  *                          this will be the last call of this function by the engine for this animation
  *                          session.
  * \param[in] animatorParam \b LPVOID, parameter passed "as is" from the HTMLayoutAnimateElement call. 
  */
typedef UINT CALLBACK HTMLayoutElementAnimator( HELEMENT he, UINT step, LPVOID animatorParam );

/** HTMLayoutAnimateElement - starts animation of the element. 
  * HTMLayoutElementAnimator function does the animation. It defines what and how to animate the element. 
  * \param[in] he \b HELEMENT, element that will be animated.
  * \param[in] pAnimator \b HTMLayoutElementAnimator*, animator worker function.
  * \param[in] animatorParam \b LPVOID, parameter passed "as is" to the HTMLayoutElementAnimator function. 
  */
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutAnimateElement( HELEMENT he, HTMLayoutElementAnimator* pAnimator, LPVOID animatorParam);


EXTERN_C HLDOM_RESULT HLAPI HTMLayoutEnqueueMeasure( HELEMENT he);


/** HTMLayoutParseValue - parses JSON forrmatted text (data).
 * \param[in] text \b LPCWSTR, json text.
 * \param[in] textLength \b UINT, length of json text.
 * \param[in] mode \b UINT, parsing mode: 0 - closed json literal, 1 - open-map mode.
 * \param[out] pVal \b JSON_VALUE *, ptr to location where to store parsed value.
 *
 * \return \b UINT - number of characters that was not parsed due to some error,
 *                   so 0 means success - whole text was parsed.          
 *
 * HTMLayoutParseValue uses relaxed JSON rules, e.g. 
 *   { one:1 2:two }
 * is a valid input. In strict JSON that sample above must be written as:
 *   { "one":1, 2:"two" }
 * \see http://json.org
 **/
EXTERN_C UINT HLAPI HTMLayoutParseValue( LPCWSTR text, UINT textLength, UINT mode, VALUE *pVal );


/** HTMLayoutRange*** - range manipulation routines. 
    ATTN: Not completed yet */

enum ADVANCE_TO 
{
  GO_FIRST = 0,
  GO_LAST  = 1,
  GO_NEXT  = 2,
  GO_PREV  = 3,
  GO_NEXT_CHAR  = 4,
  GO_PREV_CHAR  = 5,
};

EXTERN_C HLDOM_RESULT HLAPI HTMLayoutRangeCreate( HELEMENT he, HRANGE* pRange, BOOL outer );
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutRangeFromSelection( HELEMENT he, HRANGE* pRange );
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutRangeFromPositions( HELEMENT he, HPOSITION* pStart, HPOSITION* pEnd );
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutRangeRelease( HRANGE range );
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutRangeAdvancePos( HRANGE range, UINT cmd, INT* c, HPOSITION* pPos );
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutRangeToHtml( HRANGE range, LPBYTE* pHtmlUtf8Bytes, UINT* numBytes );
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutRangeReplace( HRANGE range, LPBYTE htmlUtf8Bytes, UINT numBytes );
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutRangeInsertHtml( HPOSITION* pPos, LPBYTE htmlUtf8Bytes, UINT numBytes );
EXTERN_C HLDOM_RESULT HLAPI HTMLayoutRangeIsEmpty( HRANGE range, BOOL* pResult );


#endif


