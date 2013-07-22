#ifndef __behavior_aux__
#define __behavior_aux__

#pragma warning( push )
#pragma warning(disable:4786) //identifier was truncated...

#include "htmlayout.h"
#include "htmlayout_dom.h"
#include "htmlayout_behavior.h"

#include <string.h>
#include <assert.h>
#include <stdio.h>

#include "notifications.h"

inline int width( const RECT& rc ) { return rc.right - rc.left; }
inline int height( const RECT& rc ) { return rc.bottom - rc.top; }

#pragma warning( pop )

#endif


