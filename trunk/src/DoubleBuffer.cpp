/* Copyright (C) 2013 Webyog Inc

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

/*********************************************

Author: Vishal P.R

*********************************************/

#include "DoubleBuffer.h"

ChildWndHandle::ChildWndHandle(HWND hwnd)
{
    m_hwnd = hwnd;
}

ChildWndHandle::~ChildWndHandle()
{
}

MemBuffer::MemBuffer()
{
}

MemBuffer::MemBuffer(HDC hdc, RECT* rect)
{
	Initialize(hdc, rect);
}

MemBuffer::~MemBuffer()
{
	SelectObject(m_hmemdc, m_holdbitmap);
 	DeleteDC(m_hmemdc);
	DeleteObject(m_hbitmap);
}

//function initializes the resources
void 
MemBuffer::Initialize(HDC hdc, RECT* rect)
{
	m_rect = *rect;	
    m_rect.right += 40;
    m_rect.bottom += 40;


    //create the memory DC and memory bitmap
	m_hmemdc = CreateCompatibleDC(hdc);
	m_hbitmap = CreateCompatibleBitmap(hdc, m_rect.right - m_rect.left, m_rect.bottom - m_rect.top);

    //select the bitmap into the dc
	m_holdbitmap = (HBITMAP)SelectObject(m_hmemdc, m_hbitmap);
}

//parameterized constructor that uses the given window handle to find the dc
DoubleBuffer::DoubleBuffer(HWND hwnd)
{
	RECT rect;

    //initialize the members
	m_hwnd = hwnd;
	m_hdc = GetDC(hwnd);
    GetClientRect(hwnd, &rect);	
	m_buffer.Initialize(m_hdc, &rect);
    m_isexternaldc = wyFalse;
}

//contstuctor with external dc supplied
DoubleBuffer::DoubleBuffer(HWND hwnd, HDC hdc)
{
	RECT rect;

	m_hwnd = hwnd;
	m_hdc = hdc;
	GetClientRect(hwnd, &rect);	
	m_buffer.Initialize(m_hdc, &rect);
    m_isexternaldc = wyTrue;
}

DoubleBuffer::~DoubleBuffer()
{
	while(m_childbuffer.GetCount())
	{
		FreeBuffer(0);
	}

    if(m_isexternaldc == wyFalse)
    {
	    ReleaseDC(m_hwnd, m_hdc);
    }
}

//function that adds extra buffer that can be used for child controls, good for custom painting the controls
void 
DoubleBuffer::AddChildBuffer(RECT* rect)
{
	MemBuffer* pbuff;

	pbuff = new MemBuffer(m_hdc, rect);
	m_childbuffer.Insert(pbuff);
}

//the function copies the memory bitmap to the actual dc
void 
DoubleBuffer::CopyBufferToScreen(wyInt32 index, RECT* rect)
{
	MemBuffer* pbuff;
	wyInt32 width, height;

	if(index = -1)
	{
        if(rect)
        {
            BitBlt(m_hdc, rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top, m_buffer.m_hmemdc, rect->left, rect->top, SRCCOPY); 
        }
        else
        {
            BitBlt(m_hdc, m_buffer.m_rect.left, m_buffer.m_rect.top, 
                m_buffer.m_rect.right - m_buffer.m_rect.left, 
                m_buffer.m_rect.bottom - m_buffer.m_rect.top, m_buffer.m_hmemdc, 0, 0, SRCCOPY); 
        }
	}
	else
	{
		pbuff = (MemBuffer*)m_childbuffer[index];

		if(!pbuff)
        {
			return;
        }

		width = pbuff->m_rect.right - pbuff->m_rect.left;
		height = pbuff->m_rect.bottom - pbuff->m_rect.top;
		BitBlt(m_hdc, pbuff->m_rect.left, pbuff->m_rect.top, width, height, pbuff->m_hmemdc, 0, 0, SRCCOPY); 
	}
}

//function copies the actual bitmap to the screen
void 
DoubleBuffer::CopyScreenToBuffer(RECT* rect, wyInt32 index)
{
	RECT* srcrect;
	wyInt32 width, height;
	MemBuffer* pbuff;

	srcrect = (rect == NULL) ? &(m_buffer.m_rect) : rect;
	width = srcrect->right - srcrect->left;
	height = srcrect->bottom - srcrect->top;

	if(index == -1)
	{
		BitBlt(m_buffer.m_hmemdc, srcrect->left, srcrect->top, width, height, m_hdc, srcrect->left, srcrect->top, SRCCOPY);
	}
	else
	{
		pbuff = (MemBuffer*)m_childbuffer[index];

		if(!pbuff)
        {
			return;
        }

		BitBlt(pbuff->m_hmemdc, 0, 0, width, height, m_hdc, srcrect->left, srcrect->top, SRCCOPY);
	}
}

//free any child buffer indexed
void 
DoubleBuffer::FreeBuffer(wyInt32 index)
{
	MemBuffer* pbuff;

	pbuff = (MemBuffer*)m_childbuffer[index];
	
	if(!pbuff)
    {
		return;
    }
	
	m_childbuffer.Remove(pbuff);
	delete pbuff;	
}

//a function that copies a particular buffer to another buffer. this is pretty useful when u want to copy the child controls to the main bitmap
void 
DoubleBuffer::CopyBufferToBuffer(wyInt32 srcindex, wyInt32 targindex, RECT* srcrect, RECT* targrect)
{
	RECT* temptargrect, tempsrcrect;
	MemBuffer *srcbuff, *targbuff;
	int width, height;

	srcbuff = (srcindex == -1) ? &m_buffer : (MemBuffer*)m_childbuffer[srcindex];
	targbuff = (targindex == -1) ? &m_buffer : (MemBuffer*)m_childbuffer[targindex];

	if(srcbuff == NULL || targbuff == NULL)
    {
		return;
    }

	if(srcrect == NULL)
	{
		tempsrcrect.left = 0;
		tempsrcrect.top = 0;
	}
	else
    {
		tempsrcrect = *srcrect;
    }

	temptargrect = (targrect == NULL) ? &(srcbuff->m_rect) : targrect;
	width = temptargrect->right - temptargrect->left;
	height = temptargrect->bottom - temptargrect->top;
	BitBlt(targbuff->m_hmemdc, temptargrect->left, temptargrect->top, width, height, srcbuff->m_hmemdc, tempsrcrect.left, tempsrcrect.top, SRCCOPY);
}

//a helper function that can be used in most of the cases. 
//this will enumerate any immediate child controls of the window and copies the repective screen to the memory bitmap
//once it finished copying all the child controls, it will put it back to the screen
void 
DoubleBuffer::PaintWindow()
{
    List                childlist;
    ChildWndHandle*     pchild;
    wyInt32             index;
    RECT                rect;
    wyInt32             isignore;

    pchild = new ChildWndHandle(m_hwnd);
    childlist.Insert(pchild);
    EnumChildWindows(m_hwnd, DoubleBuffer::EnumChidWndProc, (LPARAM)&childlist);
    
    //process it in the reverse Z-order; i.e bottom up
    for(index = childlist.GetCount() - 1, pchild = (ChildWndHandle*)childlist.GetLast(); 
        index > 0; 
        --index, pchild = (ChildWndHandle*)pchild->m_prev)
    {
        isignore = 0;
        
        //check for visibility
        if(IsWindowVisible(pchild->m_hwnd) == FALSE)
        {
            continue;
        }

        if(SendMessage(pchild->m_hwnd, WM_GETDLGCODE, NULL, NULL) == DLGC_STATIC)
        {
            SendMessage(m_hwnd, UM_ISIGNOREHWNDFROMPAINT, (WPARAM)pchild->m_hwnd, (LPARAM)&isignore);
        }

        if(isignore == 0)
        {
            GetWindowRect(pchild->m_hwnd, &rect);
            MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rect, 2);
            CopyScreenToBuffer(&rect);
        }
    }

    CopyBufferToScreen();

    for(pchild = (ChildWndHandle*)childlist.GetFirst(); pchild; pchild = (ChildWndHandle*)childlist.GetFirst())
    {
        childlist.Remove(pchild);
        delete pchild;
    }
}

void 
DoubleBuffer::PaintWindow(HWND *hwndlist)
{
    
	List                childlist;
    ChildWndHandle*     pchild;
    wyInt32             index;
    RECT                rect;

    //process it in the reverse Z-order; i.e bottom up
    for(index = 0; index > 0; --index)
    {
        pchild = (ChildWndHandle*)childlist[index];
        
        //check for visibility
        if(IsWindowVisible(pchild->m_hwnd) == FALSE)
        {
                continue;
        }

        GetWindowRect(pchild->m_hwnd, &rect);
        MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rect, 2);
        CopyScreenToBuffer(&rect);
    }

    CopyBufferToScreen();

    for(pchild = (ChildWndHandle*)childlist.GetFirst(); pchild; pchild = (ChildWndHandle*)childlist.GetFirst())
    {
        childlist.Remove(pchild);
        delete pchild;
    }
}


void 
DoubleBuffer::PaintWindow(HWND* phwnd, wyInt32 count)
{
    wyInt32 i;
    RECT    rect;
  
    for(i = count; i >= 0; --i)
    {
        if(IsWindowVisible(phwnd[i]) == FALSE)
        {
            continue;
        }
		GetWindowRect(phwnd[i], &rect);
        MapWindowPoints(NULL, m_hwnd, (LPPOINT)&rect, 2);
        CopyScreenToBuffer(&rect);
    }

    CopyBufferToScreen();
}

void 
DoubleBuffer::EraseBackground(HWND hwnd, HDC hdc, RECT* prect, COLORREF bgcolor)
{
    HBRUSH  hbr;
    RECT    rect;

    if(prect)
    {
        rect = *prect;
    }
    else
    {
        GetClientRect(hwnd, &rect);
    }

    hbr = CreateSolidBrush(bgcolor);
    FillRect(hdc, &rect, hbr);
    DeleteObject(hbr);
}

//a helper function used to erase the background of a window
//this will fill the memory bitmap with the color given
void 
DoubleBuffer::EraseBackground(COLORREF bgcolor)
{
    HBRUSH  backgroundbrush = CreateSolidBrush(bgcolor);

    FillRect(m_buffer.m_hmemdc, &m_buffer.m_rect, backgroundbrush);
    DeleteObject(backgroundbrush);
}

void 
DoubleBuffer::EraseBackground()
{
    HBRUSH  backgroundbrush = (HBRUSH)GetClassLong(m_hwnd, GCLP_HBRBACKGROUND);

    FillRect(m_buffer.m_hmemdc, &m_buffer.m_rect, backgroundbrush);
}

//enumeration procedure that adds the child window handlesto the list
BOOL CALLBACK 
DoubleBuffer::EnumChidWndProc(HWND hwnd, LPARAM lParam)
{
    ChildWndHandle* ptabhwnd;
    ChildWndHandle* parent;
    List*           plist;
    RECT            rectchild, rectparent;
    
    plist = (List*)lParam;
    parent = (ChildWndHandle*)plist->GetFirst();
 
    if(GetParent(hwnd) == parent->m_hwnd)
    {
        GetWindowRect(parent->m_hwnd, &rectparent);
        GetWindowRect(hwnd, &rectchild);

        if(rectchild.right < rectparent.left || rectchild.left > rectparent.right ||
           rectchild.bottom < rectparent.top || rectchild.top > rectparent.bottom)
        {
            return TRUE;
        }

        ptabhwnd = new ChildWndHandle(hwnd);
        plist->Insert(ptabhwnd);
    }

    return TRUE;
}