#ifndef __value_h__
#define __value_h__

#ifdef WIN32
#include <windows.h>
#endif

#if !defined(STATIC_LIB)
  #if defined(HTMLAYOUT_EXPORTS) || defined(SCITER_EXPORTS)
    #define VALAPI __declspec(dllexport) __stdcall
  #else
    #define VALAPI __declspec(dllimport) __stdcall
  #endif
#else
  #define VALAPI
#endif

enum VALUE_RESULT
{
  HV_OK_TRUE = -1,
  HV_OK = 0,
  HV_BAD_PARAMETER = 1,
  HV_INCOMPATIBLE_TYPE = 2
};

typedef struct
{
  UINT   t;
  UINT   u;
  UINT64 d;
} VALUE;

#define FLOAT_VALUE   double
typedef const unsigned char* LPCBYTES;


enum VALUE_TYPE
{
    T_UNDEFINED = 0,
    T_NULL = 1,
    T_BOOL,
    T_INT,
    T_FLOAT,
    T_STRING,
    T_DATE,     // INT64 - contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC), a.k.a. FILETIME on Windows
    T_CURRENCY, // INT64 - 14.4 fixed number. E.g. dollars = int64 / 10000; 
    T_LENGTH,   // length units, value is int or float, units are VALUE_UNIT_TYPE
    T_ARRAY,
    T_MAP,
    T_FUNCTION,
    T_BYTES,      // sequence of bytes - e.g. image data
    T_OBJECT,     // scripting object proxy (TISCRIPT/SCITER)
    T_DOM_OBJECT  // DOM object (CSSS!), use get_object_data to get HELEMENT 
};

enum VALUE_UNIT_TYPE
{
    UT_EM = 1, //height of the element's font. 
    UT_EX = 2, //height of letter 'x' 
    UT_PR = 3, //%
    UT_SP = 4, //%% "springs", a.k.a. flex units
    reserved1 = 5, 
    reserved2 = 6, 
    UT_PX  = 7, //pixels
    UT_IN  = 8, //inches (1 inch = 2.54 centimeters). 
    UT_CM  = 9, //centimeters. 
    UT_MM  = 10, //millimeters. 
    UT_PT  = 11, //points (1 point = 1/72 inches). 
    UT_PC  = 12, //picas (1 pica = 12 points). 
    UT_DIP = 13,
    reserved3 = 14, 
    UT_COLOR = 15, // color in int
    UT_URL   = 16,  // url in string
    UT_SYMBOL = 0xFFFF, // for T_STRINGs designates symbol string ( so called NAME_TOKEN - CSS or JS identifier )
};

enum VALUE_UNIT_TYPE_DATE
{
    DT_HAS_DATE         = 0x01, // date contains date portion
    DT_HAS_TIME         = 0x02, // date contains time portion HH:MM
    DT_HAS_SECONDS      = 0x04, // date contains time and seconds HH:MM:SS
    DT_UTC              = 0x10, // T_DATE is known to be UTC. Otherwise it is local date/time
};

// Sciter or TIScript specific
enum VALUE_UNIT_TYPE_OBJECT
{
    UT_OBJECT_ARRAY  = 0,   // type T_OBJECT of type Array
    UT_OBJECT_OBJECT = 1,   // type T_OBJECT of type Object
    UT_OBJECT_CLASS  = 2,   // type T_OBJECT of type Type (class or namespace)
    UT_OBJECT_NATIVE = 3,   // type T_OBJECT of native Type with data slot (LPVOID)
    UT_OBJECT_FUNCTION = 4, // type T_OBJECT of type Function
    UT_OBJECT_ERROR = 5,    // type T_OBJECT of type Error
};


/**
 * ValueInit - initialize VALUE storage
 * This call has to be made before passing VALUE* to any other functions
 */
EXTERN_C UINT VALAPI ValueInit( VALUE* pval );

/**
 * ValueClear - clears the VALUE and deallocates all assosiated structures that are not used anywhere else.
 */
EXTERN_C UINT VALAPI ValueClear( VALUE* pval );

/**
 * ValueCompare - compares two values, returns HV_OK_TRUE if val1 == val2.
 */
EXTERN_C UINT VALAPI ValueCompare( const VALUE* pval1, const VALUE* pval2 );

/**
 * ValueCopy - copies src VALUE to dst VALUE. dst VALUE must be in ValueInit state.
 */
EXTERN_C UINT VALAPI ValueCopy( VALUE* pdst, const VALUE* psrc );

/**
 * ValueIsolate - converts T_OBJECT value types to T_MAP or T_ARRAY.
 * use this method if you need to pass values between different threads.
 * The fanction is applicable for the Sciter 
 */
EXTERN_C UINT VALAPI ValueIsolate( VALUE* pdst );

/**
 * ValueType - returns VALUE_TYPE and VALUE_UNIT_TYPE flags of the VALUE
 */
EXTERN_C UINT VALAPI ValueType( const VALUE* pval, UINT* pType, UINT* pUnits );

/**
 * ValueStringData - returns string data for T_STRING type
 * For T_FUNCTION returns name of the fuction. 
 */
EXTERN_C UINT VALAPI ValueStringData( const VALUE* pval, LPCWSTR* pChars, UINT* pNumChars );

/**
 * ValueStringDataSet - sets VALUE to T_STRING type and copies chars/numChars to
 * internal refcounted buffer assosiated with the value. 
 */
EXTERN_C UINT VALAPI ValueStringDataSet( VALUE* pval, LPCWSTR chars, UINT numChars, UINT units );

/**
 * ValueIntData - retreive integer data of T_INT, T_LENGTH and T_BOOL types
 */
EXTERN_C UINT VALAPI ValueIntData( const VALUE* pval, INT* pData );

/**
 * ValueIntDataSet - sets VALUE integer data of T_INT and T_BOOL types 
 * Optionally sets units field too.
 */
EXTERN_C UINT VALAPI ValueIntDataSet( VALUE* pval, INT data, UINT type, UINT units );

/**
 * ValueInt64Data - retreive 64bit integer data of T_CURRENCY and T_DATE values.
 */
EXTERN_C UINT VALAPI ValueInt64Data( const VALUE* pval, INT64* pData );

/**
 * ValueInt64DataSet - sets 64bit integer data of T_CURRENCY and T_DATE values.
 */
EXTERN_C UINT VALAPI ValueInt64DataSet( VALUE* pval, INT64 data, UINT type, UINT units );

/**
 * ValueFloatData - retreive FLOAT_VALUE (double) data of T_FLOAT and T_LENGTH values.
 */
EXTERN_C UINT VALAPI ValueFloatData( const VALUE* pval, FLOAT_VALUE* pData );

/**
 * ValueFloatDataSet - sets FLOAT_VALUE data of T_FLOAT and T_LENGTH values.
 */
EXTERN_C UINT VALAPI ValueFloatDataSet( VALUE* pval, FLOAT_VALUE data, UINT type, UINT units );

/**
 * ValueBinaryData - retreive integer data of T_BYTES type
 */
EXTERN_C UINT VALAPI ValueBinaryData( const VALUE* pval, LPCBYTES* pBytes, UINT* pnBytes );

/**
 * ValueBinaryDataSet - sets VALUE to sequence of bytes of type T_BYTES 
 * 'type' here must be set to T_BYTES. Optionally sets units field too.
 * The function creates local copy of bytes in its own storage.
 */
EXTERN_C UINT VALAPI ValueBinaryDataSet( VALUE* pval, LPCBYTES pBytes, UINT nBytes, UINT type, UINT units );

/**
 * ValueElementsCount - retreive number of sub-elements for:
 * - T_ARRAY - number of elements in the array; 
 * - T_MAP - number of key/value pairs in the map;
 * - T_FUNCTION - number of arguments in the function;
 */
EXTERN_C UINT VALAPI ValueElementsCount( const VALUE* pval, INT* pn);

/**
 * ValueNthElementValue - retreive value of sub-element at index n for:
 * - T_ARRAY - nth element of the array; 
 * - T_MAP - value of nth key/value pair in the map;
 * - T_FUNCTION - value of nth argument of the function;
 */
EXTERN_C UINT VALAPI ValueNthElementValue( const VALUE* pval, INT n, VALUE* pretval);

/**
 * ValueNthElementValueSet - sets value of sub-element at index n for:
 * - T_ARRAY - nth element of the array; 
 * - T_MAP - value of nth key/value pair in the map;
 * - T_FUNCTION - value of nth argument of the function;
 * If the VALUE is not of one of types above then it makes it of type T_ARRAY with 
 * single element - 'val_to_set'.
 */
EXTERN_C UINT VALAPI ValueNthElementValueSet( VALUE* pval, INT n, const VALUE* pval_to_set);

/**Callback function used with #ValueEnumElements().
 * return TRUE to continue enumeration
 */
typedef BOOL CALLBACK KeyValueCallback( LPVOID param, const VALUE* pkey, const VALUE* pval );

/**
 * ValueEnumElements - enumeartes key/value pairs of T_MAP, T_FUNCTION and T_OBJECT values
 * - T_MAP - key of nth key/value pair in the map;
 * - T_FUNCTION - name of nth argument of the function (if any);
 */
EXTERN_C UINT VALAPI ValueNthElementKey( const VALUE* pval, INT n, VALUE* pretval);

EXTERN_C UINT VALAPI ValueEnumElements( VALUE* pval, KeyValueCallback* penum, LPVOID param);

/**
 * ValueSetValueToKey - sets value of sub-element by key:
 * - T_MAP - value of key/value pair with the key;
 * - T_FUNCTION - value of argument with the name key;
 * - T_OBJECT (tiscript) - value of property of the object
 * If the VALUE is not of one of types above then it makes it of type T_MAP with 
 * single pair - 'key'/'val_to_set'.
 *
 * key usually is a value of type T_STRING
 *
 */
EXTERN_C UINT VALAPI ValueSetValueToKey( VALUE* pval, const VALUE* pkey, const VALUE* pval_to_set);

/**
 * ValueGetValueOfKey - retrieves value of sub-element by key:
 * - T_MAP - value of key/value pair with the key;
 * - T_FUNCTION - value of argument with the name key;
 * - T_OBJECT (tiscript) - value of property of the object
 * Otherwise *pretval will have T_UNDEFINED value.
 */
EXTERN_C UINT VALAPI ValueGetValueOfKey( const VALUE* pval, const VALUE* pkey, VALUE* pretval);

enum VALUE_STRING_CVT_TYPE
{
  CVT_SIMPLE,       ///< simple conversion of terminal values 
  CVT_JSON_LITERAL, ///< json literal parsing/emission 
  CVT_JSON_MAP,     ///< json parsing/emission, it parses as if token '{' already recognized 
};

/**
 * ValueToString - converts value to T_STRING inplace:
 * - CVT_SIMPLE - parse/emit terminal values (T_INT, T_FLOAT, T_LENGTH, T_STRING)
 * - CVT_JSON_LITERAL - parse/emit value using JSON literal rules: {}, [], "string", true, false, null 
 * - CVT_JSON_MAP - parse/emit MAP value without enclosing '{' and '}' brackets.
 */
EXTERN_C UINT VALAPI ValueToString( VALUE* pval, /*VALUE_STRING_CVT_TYPE*/ UINT how );

/**
 * ValueFromString - parses string into value:
 * - CVT_SIMPLE - parse/emit terminal values (T_INT, T_FLOAT, T_LENGTH, T_STRING), "guess" non-strict parsing
 * - CVT_JSON_LITERAL - parse/emit value using JSON literal rules: {}, [], "string", true, false, null 
 * - CVT_JSON_MAP - parse/emit MAP value without enclosing '{' and '}' brackets.
 * Returns:
 *   Number of non-parsed characters in case of errors. Thus if string was parsed in full it returns 0 (success)  
 */
EXTERN_C UINT VALAPI ValueFromString( VALUE* pval, LPCWSTR str, UINT strLength, /*VALUE_STRING_CVT_TYPE*/ UINT how );

/**
 * ValueInvoke - function invocation (Sciter/TIScript).
 * - VALUE* pval is a value of type T_OBJECT/UT_OBJECT_FUNCTION
 * - VALUE* pthis - object that will be known as 'this' inside that function.
 * - UINT argc, const VALUE* argv - vector of arguments to pass to the function. 
 * - VALUE* pretval - parse/emit MAP value without enclosing '{' and '}' brackets.
 * - LPCWSTR url - url or name of the script - used for error reporting in the script.
 * Returns:
 *   HV_OK, HV_BAD_PARAMETER or HV_INCOMPATIBLE_TYPE
 */
EXTERN_C UINT VALAPI ValueInvoke( VALUE* pval, VALUE* pthis, UINT argc, const VALUE* argv, VALUE* pretval, LPCWSTR url);
  
#if defined(__cplusplus)

  #include <string>
  #include "aux-slice.h"
  #include "aux-cvt.h"

  #pragma warning( push )
    #pragma warning(disable:4786) //identifier was truncated...

  namespace json
  {

    inline void str_copy(char* dst,const char* src, unsigned n ) { dst && src? strncpy(dst,src,n):0; }
    inline void str_copy(wchar_t* dst,const wchar_t* src, unsigned n ) { dst && src? wcsncpy(dst,src,n):0; }
    inline int  str_length(const char* src ) { return src? strlen(src):0; }
    inline int  str_length(const wchar_t* src ) { return src? wcslen(src):0; }

    template<typename TC>
    class string_t
    {
      struct data { mutable volatile unsigned rc; unsigned length; TC chars[1]; };
      data* _data;

      void init( const TC* s, unsigned n )
      {
        assert(_data == 0); if(!s || !n) return;
        _data = (data*)malloc(sizeof(data) + sizeof(TC) * n);
        if(_data) {  str_copy(_data->chars,s,n); _data->rc = 1; _data->length = n; _data->chars[n] = 0; }
      }
      void init( data* pd ) {  _data = pd; if(_data) ++(_data->rc); }
      
    public: 
      string_t(): _data(0) {}
      string_t(const TC* s)  : _data(0) { init(s, (unsigned int)( str_length(s)) ); }
      string_t(const TC* s, unsigned length)    : _data(0) { init(s,length); }
      string_t(aux::slice<TC> wc)    : _data(0) { init(wc.start,wc.length); }
      string_t(const string_t& s)   : _data(0) { init(s._data); }
      string_t& operator = ( const string_t& s ) { clear(); init(s._data); return *this; }
      string_t& operator = ( const TC* s ) { clear(); init(s,(unsigned int)(str_length(s))); return *this; }
      ~string_t() { clear(); }

      const TC* c_str() const { static TC z = 0; return _data?_data->chars:&z; }
            unsigned length() const { return _data?_data->length: 0; }
                void clear() { if(_data && (--(_data->rc) == 0)) { free(_data); _data = 0; } }
      
      operator const TC*() const { return c_str(); }
      operator const aux::slice<TC>() const { return aux::slice<TC>(c_str(), length()); }
      bool operator == (aux::slice<TC> rs) const { return aux::slice<TC>(c_str(), length()) == rs; }
      bool operator != (aux::slice<TC> rs) const { return aux::slice<TC>(c_str(), length()) != rs; }
    };

    // wide (utf16) string
    typedef string_t<wchar_t> string;
    // ascii or utf8 string
    typedef string_t<char> astring;

    // value by key bidirectional proxy/accessor 
    class value_key_a;
    // value by index bidirectional proxy/accessor 
    class value_idx_a;

    class value: public VALUE
    {
      value(void*) {} // no such thing, sorry
      //void* get(const void* defv) const { return 0; } // and this one too is disabled
      //void* get(const void* defv) { return 0; } // and this one too is disabled
    public:
      value()                 { ValueInit(this); }
     ~value()                 { ValueClear(this); }
      value(const value& src) { ValueInit(this); ValueCopy(this,&src); }
      value(const VALUE& src) { ValueInit(this); ValueCopy(this,&src); }

      value& operator = (const value& src) { ValueCopy(this,&src); return *this; }
      value& operator = (const VALUE& src) { ValueCopy(this,&src); return *this; }

      value( bool v )           { ValueInit(this); ValueIntDataSet(this, v?1:0, T_BOOL, 0); }
      value( int  v )           { ValueInit(this); ValueIntDataSet(this, v, T_INT, 0); }
      value( double v )         { ValueInit(this); ValueFloatDataSet(this, v, T_FLOAT, 0); }

      value( const wchar_t* s, unsigned int slen = 0 ) { ValueInit(this); ValueStringDataSet(this, s, (slen || !s)? slen : (unsigned int)wcslen(s), 0); }
      value( const string& s ) { ValueInit(this); ValueStringDataSet(this, s.c_str(), s.length(), 0); }
      value( const std::wstring& s ) { ValueInit(this); ValueStringDataSet(this, s.c_str(), s.length(), 0); }
      value( const std::string& s ) { aux::a2w as(s.c_str()); ValueInit(this); ValueStringDataSet(this, as, as.length(), UT_SYMBOL); }

      value( aux::bytes bs )    { ValueInit(this); ValueBinaryDataSet(this, bs.start, bs.length, T_BYTES, 0); }
    
      value( const value* arr, unsigned n )  { ValueInit(this); for( unsigned i = 0; i < n; ++i ) set_item(i,arr[i]); }
    
      static value currency( INT64 v )  { value t; ValueInt64DataSet(&t, v, T_CURRENCY, 0); return t;}
      static value date( INT64 v )      { value t; ValueInt64DataSet(&t, v, T_DATE, 0);  return t;}
#ifdef WIN32
      static value date( FILETIME ft )  { value t; ValueInt64DataSet(&t, *((INT64*)&ft), T_DATE, 0); return t;} 
#endif
      static value symbol( aux::wchars wc ) { value t; ValueInit(&t); ValueStringDataSet(&t, wc.start, wc.length , 0xFFFF); return t; }
    
      // string-symbol
      value( const char* s ) 
      { 
        aux::a2w as(s);
        ValueInit(this); ValueStringDataSet(this, as, as.length(), UT_SYMBOL); 
      }

      bool is_undefined() const { return t == T_UNDEFINED; }
      bool is_bool() const { return t == T_BOOL; }
      bool is_int() const { return t == T_INT; }
      bool is_float() const { return t == T_FLOAT; }
      bool is_string() const { return t == T_STRING; }
      bool is_symbol() const { return t == T_STRING && u == UT_SYMBOL; }
      bool is_date() const { return t == T_DATE; }
      bool is_currency() const { return t == T_CURRENCY; }
      bool is_map() const { return t == T_MAP; }
      bool is_array() const { return t == T_ARRAY; }
      bool is_function() const { return t == T_FUNCTION; }
      bool is_bytes() const { return t == T_BYTES; }
      bool is_object() const { return t == T_OBJECT; }
      bool is_dom_element() const { return t == T_DOM_OBJECT; }
      
      bool is_null() const { return t == T_NULL; }

      static value null() { value n; n.t = T_NULL; return n; }

      bool operator == (const value& rs) const 
      {
        if( this == &rs ) return true;
        switch(ValueCompare( this, &rs ))
        {
          case HV_OK: return false;
          case HV_OK_TRUE: return true;
          default: assert(false);
        }
        return false;
      }
      bool operator != (const value& rs) const 
      {
        return !(operator==(rs));
      }

      int get(int defv) const 
      {
        int v;
        if(ValueIntData(this,&v) == HV_OK) return v; 
        return defv;
      }
      double get(double defv) const 
      {
        double v;
        if(ValueFloatData(this,&v) == HV_OK) return v; 
        return defv;
      }
      string get(const wchar_t* defv) const
      {
        aux::wchars wc;
        if(ValueStringData(this,&wc.start,&wc.length) == HV_OK) 
          return string(wc); 
        return string(defv);
      }
      aux::wchars get_chars() const
      {
        aux::wchars s;
        ValueStringData(this,&s.start,&s.length);
        return s;
      }
      aux::bytes get_bytes() const 
      {
        aux::bytes bs;
        ValueBinaryData(this,&bs.start,&bs.length);
        return bs;
      }

#ifdef WIN32
      FILETIME get_date() const
      { 
        INT64 v;
        if(ValueInt64Data(this,&v) == HV_OK) return *((FILETIME*)&v); 
        return FILETIME();
      }
#endif

      bool get(bool defv) const 
      {
        int v;
        if(ValueIntData(this,&v) == HV_OK) return v != 0; 
        return defv;
      }

      static value from_string(const wchar_t* s, unsigned int len = 0, VALUE_STRING_CVT_TYPE ct = CVT_SIMPLE)
      {
        value t;
        if( s ) 
        {
          if(len == 0) len = (unsigned int)wcslen(s);
          ValueFromString( &t, s, len, ct );
        }
        return t;
      }
      static value from_string(const std::wstring& s, VALUE_STRING_CVT_TYPE ct = CVT_SIMPLE)
      {
        return from_string(s.c_str(), (unsigned int)s.length(),ct);
      }
      static value from_string(aux::wchars s, VALUE_STRING_CVT_TYPE ct = CVT_SIMPLE)
      {
        return from_string(s.start, s.length,ct);
      }


      string to_string(int how = CVT_SIMPLE) const
      {
        if( how == CVT_SIMPLE && is_string() )
          return string(get_chars()); // do not need to allocate
        value t = *this;
        ValueToString(&t,how);
        return string(t.get_chars());
      }

      void clear()
      {
        ValueClear(this);
      }

      // if it is an array or map returns number of elements there, otherwise - 0
      // if it is a function - returns number of arguments
      int length() const 
      {
        int n = 0;
        ValueElementsCount( this, &n);
        return n;
      }
      // if it is an array - returns nth element
      // if it is a map - returns nth value of the map
      // if it is a function - returns nth argument
      // otherwise it returns undefined value
      value get_item(int n) const
      {
        value r;
        ValueNthElementValue( this, n, &r);
        return r;
      }

      const value operator[](int n) const { return get_item(n); }
      value_idx_a operator[](int n);

      // if it is a map - returns value under the key in the map
      // if it is a function - returns value of argument with the name
      // otherwise it returns undefined value
      const value operator[](const value& key) const { return get_item(key); }
      value_key_a operator[](const value& key);

      struct enum_cb
      {
        // return true to continue enumeration
        virtual bool on(const value& key, const value& val) = 0;
        static BOOL CALLBACK _callback( LPVOID param, const VALUE* pkey, const VALUE* pval )
      {
          enum_cb* cb = (enum_cb*)param;
          return cb->on( *(value*)pkey, *(value*)pval );
        }
      };

      // enum
      void enum_elements(enum_cb& cb) const
      {
        ValueEnumElements(const_cast<value*>(this), &enum_cb::_callback, &cb);
      }

      value key(int n) const
      {
        value r;
        ValueNthElementKey( this, n, &r);
        return r;
      }

      // if it is an array - sets nth element expanding the array if needed
      // if it is a map - sets nth value of the map;
      // if it is a function - sets nth argument of the function;
      // otherwise it converts this to array and adds v as first element.
      void set_item(int n, const value& v)
      {
        ValueNthElementValueSet( this, n, &v);
      }
      void append(const value& v) 
      {
        ValueNthElementValueSet( this, length(), &v);
      }
      // if it is a map - sets named value in the map;
      // if it is a function - sets named argument of the function;
      // otherwise it converts this to map and adds key/v to it.

      void set_item(const value& key, const value& v)
      {
        ValueSetValueToKey( this,&key,&v );
      }
      const value get_item(const value& key) const
      {
        value r;
        ValueGetValueOfKey( this, &key, &r);
        return r;
      }

      
      // T_OBJECT and T_DOM_OBJECT only, get value of object's data slot
      void* get_object_data() const
      {
        LPCBYTES pv = 0; unsigned int dummy;
        UINT r = ValueBinaryData(this,&pv,&dummy); r;
        assert(r == HV_OK);
        return (void*)pv;
      }

      //
      // Below this point are TISCRIPT/SCITER related methods
      //

#if defined(HAS_TISCRIPT)

      bool is_object_native() const   {  return t == T_OBJECT && u == UT_OBJECT_NATIVE; }
      bool is_object_array() const    {  return t == T_OBJECT && u == UT_OBJECT_ARRAY; }
      bool is_object_function() const {  return t == T_OBJECT && u == UT_OBJECT_FUNCTION; }
      bool is_object_object() const   {  return t == T_OBJECT && u == UT_OBJECT_OBJECT; } // that is plain TS object
      bool is_object_class() const    {  return t == T_OBJECT && u == UT_OBJECT_CLASS; }  // that is TS class
      bool is_object_error() const    {  return t == T_OBJECT && u == UT_OBJECT_ERROR; }  // that is TS error
         
     
      // T_OBJECT only, set value of object's data slot
      void set_object_data(void* pv)
      {
        assert(u == UT_OBJECT_NATIVE);
        ValueBinaryDataSet(this,(LPCBYTES)pv,1,T_OBJECT,0);
      }
      
      // T_OBJECT only, class name of the object: e.g. Array, Function, etc. or custom class name.
      //std::wstring get_object_class_name() const
      //{
      //  assert(is_object());
      //  return get(L"");
      //}
      // T_OBJECT/UT_OBJECT_FUNCTION only, call TS function
      // 'self' here is what will be known as 'this' inside the function, can be undefined for invocations of global functions 
      value call( int argc, const value* argv, value self = value(), const wchar_t* url_or_script_name = 0)
      {
        value rv;
        ValueInvoke(this,&self,argc,argv,&rv,url_or_script_name);
        return rv;
      }

      void isolate()
      {
        ValueIsolate(this);
      }

#endif //defined(HAS_TISCRIPT)
          
      // "smart" or "soft" equality test
      static bool equal(const value& v1, const value& v2)
      {
        if( v1 == v2 ) return true; // strict comparison
        switch ( v1.t > v2.t? v1.t: v2.t )
        {
           case T_BOOL:
             {
               bool const r1 = v1.get(false);
               bool const r2 = v2.get(!r1);
               return r1 == r2;
             }
           case T_INT:
             {
               int const r1 = v1.get(0);
               int const r2 = v2.get(-r1);
               return r1 == r2;
             }
           case T_FLOAT:
             {
               double const r1 = v1.get(0.0);
               double const r2 = v2.get(-r1);
               return r1 == r2;
             }
        }
        return false;
      }
    };

    // value by key bidirectional proxy/accessor 
    class value_key_a
    {
      friend class value;
      value& col;
      value  key;
      value_key_a& operator=(const value_key_a& val) { val; return *this; } // no such thing
    protected:
      value_key_a( value& c, const value& k ): col(c),key(k) {}
    public:
      ~value_key_a() {}
      value_key_a& operator=(const value& val) { col.set_item(key,val); return *this; }
      operator const value() const  { return col.get_item(key); }
    };

    inline value_key_a 
        value::operator[](const value& key) { return value_key_a(*this, key); }

    // value by index bidirectional proxy/accessor 
    class value_idx_a
    {
      friend class value;
      value& col;
      int    idx;
      value_idx_a& operator= (const value_idx_a& val) { val; return *this; } // no such thing
    protected:
      value_idx_a( value& c, int i ): col(c), idx(i) {}
    public:
      ~value_idx_a() {}
      value_idx_a& operator= (const value& val) { col.set_item(idx,val); return *this; }
      operator const value() const              { return col.get_item(idx); }
    };

    inline value_idx_a 
        value::operator[](int idx) { return value_idx_a(*this, idx); }

  }

  #pragma warning( pop )

#endif //defined(__cplusplus)

#endif
