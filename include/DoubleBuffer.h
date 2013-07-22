/* Copyright (C) 2013 Webyog Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA

*/

/*****************************************************************************
Author : Vishal P.R

This is a wrapper class which can be used to double buffer any windows control. 
It has all the useful functions to easily achieve double buffering thereby
reducing flickering
*****************************************************************************/

#ifndef _DOUBLEBUFFER_H_
#define _DOUBLEBUFFER_H_

#include "Datatype.h"
#include "List.h"

#define UM_ISIGNOREHWNDFROMPAINT   WM_USER + 500

//class to store the child window handle
class ChildWndHandle : public wyElem
{
    public:

        ///Parameterized cons
        /**
        @param hwnd     : handle to the window
        */
                        ChildWndHandle(HWND hwnd);

        ///Destructor
                        ~ChildWndHandle();

        ///Handle to the window
        HWND            m_hwnd;
};

///Class to store the memory bitmaps and associated device context
class MemBuffer : public wyElem
{
    public:

        ///Default constructor
	                        MemBuffer();

        ///Parameterized constructor
        /**
        @param hdc          : IN handle to the device context
        @param rect         : IN pointer to the area cordinates
        */
	    MemBuffer(HDC hdc, RECT* rect);

        ///Helper function to initialize
        /**
        @param hdc          : IN handle to the device context
        @param rect         : IN pointer to the area cordinates
        */
	    void                Initialize(HDC hdc, RECT* rect);

        //Destructor
	                        ~MemBuffer();

        ///Handle to the memory dc
	    HDC                 m_hmemdc;

        ///Cordinates of the actual rectangle 
	    RECT                m_rect;

        ///Handle to the memory bitmap
	    HBITMAP             m_hbitmap;

        ///Handle to the old bitmap
	    HBITMAP             m_holdbitmap;
};

///Class that does double buffering
class DoubleBuffer
{
    public:

        ///Parameterized constructor
        /**
        @param hwnd                 : IN handle to the main window for which the buffering is required
        */
	                                DoubleBuffer(HWND hwnd);

        ///Parameterized constructor
        /**
        @param hwnd                 : IN handle to the main window for which the buffering is required
        @param hdc                  : IN supplied device context
        */
        DoubleBuffer(HWND hwnd, HDC hdc);

        ///Add a child buffer
        /**
        @param rect                 : IN pointer to the region cordinate
        @returns void
        */
	    void                        AddChildBuffer(RECT* rect);

        ///Copy the specified buffer contents to screen
        /**
        @param index                : IN zero based index of the child buffer (-1 if the parent buffer)
        @param rect                 : IN pointer to the rectangle that needs to be copied, applicble only if index = -1
        @return void
        */
	    void                        CopyBufferToScreen(wyInt32 index = -1, RECT* rect = NULL);

        ///Copy screen contents to buffer
        /**
        @param rect                 : IN pointer to the rectangle that needs to be copied
        @param index                : IN zero based index of the child buffer (-1 if parent buffer)
        @returns void
        */
	    void                        CopyScreenToBuffer(RECT* rect, wyInt32 index = -1);
	
        ///Frees the buffer
        /**
        @param index                : IN zero based index of the child buffer
        @returns void
        */
	    void                        FreeBuffer(wyInt32 index);

        ///Copy a buffer to another buffer
        /**
        @param srcindex             : IN zero based index of the child buffer (-1 if parent buffer)
        @param targindex            : IN zero based index of the child buffer (-1 if parent buffer)
        @param srcrect              : IN cordinate of the src rectangle
        @param targrect             : IN cordinate of the target rectangle
        @returns void
        */
	    void                        CopyBufferToBuffer(wyInt32 srcindex, wyInt32 targindex, RECT* srcrect = NULL, RECT* targrect = NULL);

        ///Standard window procedure to enumerate child windows
        /**
        @param hwnd                 : IN handle to the window
        @param lParam               : IN user supplied parameter
        @returns TRUE on scucess else FALSE
        */
        static BOOL CALLBACK        EnumChidWndProc(HWND hwnd, LPARAM lParam);

        ///Function that copies the child window rectangles to the buffer and then copies it back to the device
        /**
        @returns void
        */
        void                        PaintWindow();

        ///Overloaded version of PaintWindow which copies the given child windows rectangles to the buffer and then copies it back to the device
        /**
        @param phwnd                : IN handles to the windows to be copied
        @param count                : IN number of handles in the array
        @returns void
        */
        void                        PaintWindow(HWND* phwnd, wyInt32 count);

		///Overloaded version of PaintWindow which copies the given child windows rectangles to the buffer and then copies it back to the device
        /**
        @param phwnd                : IN handles to the windows to be copied
        @param count                : IN number of handles in the array
        @returns void
        */
        void                        PaintWindow(HWND hwndlist[]);

        ///Fills the window client area with the color given
        /**
        @params bgcolor             : IN background color to be filled; default is white
        @returns void
        */
        void                        EraseBackground(COLORREF bgcolor);

        static void                 EraseBackground(HWND hwnd, HDC hdc, RECT* prect, COLORREF bgcolor);

        ///Fills the window client area with the background brush
        /**
        @returns void
        */
        void                        EraseBackground();

        ///Destructor
	                                ~DoubleBuffer();

        ///Handle to the actual device context
	    HDC                         m_hdc;

        ///Handle to the window
	    HWND                        m_hwnd;

        ///The memory buffer for parent
	    MemBuffer                   m_buffer;

        ///List for storing child buffers
        List                        m_childbuffer;

        //flag that diff the supplied DC and created DC
        wyBool                      m_isexternaldc;
};

#endif _DOUBLEBUFFER_H_