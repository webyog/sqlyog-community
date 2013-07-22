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


#ifndef _IMAGELIST_H
#define	_IMAGELIST_H

#include <windows.h>
#include <commctrl.h>
#include "datatype.h"


class ImageList
{
public:

    /// Default constructor.
    /**
    * Initializes the member variables with the default values.
    * it creates a image list with default value
    */
    ImageList();

    /// Constructor that create a image list with the values passed as arguments
    /**
    @param width		: IN Width, in pixels, of each image.
    @param height		: IN Height, in pixels, of each image
    @param flags		: IN Set of bit flags that specify the type of image list to create(Check MSDN)
    @param initialsize	: IN Number of images that the image list initially contains.
    @param growsize		: IN Number of images by which the image list can grow when the system needs to make room for new images. This parameter represents the number of new images that the resized image list can contain
    */
    ImageList(wyInt32 width, wyInt32 height, wyUInt32 flags, wyInt32 initialsize, wyInt32 growsize);

    /// Destructor
    /**
    Free up all the allocated resources if there are
    */
    ~ImageList();

    /// Add an icon to the image list
    /**
    @param hinstance	: IN Handle to an instance of the module whose executable file contains the icon to be loaded. This parameter must be NULL when a standard icon is being loaded
    @param imageid		: IN Specifies the integer value to be converted.
    @returns wyTrue if user pressed OK, wyFalse if user pressed Cancel
    */
    wyBool Add(HINSTANCE hinstance, wyUInt32 imageid);

    /// Retrieves the image handle
    /**
    @returns HIMAGELIST, handle to image list
    */
    HIMAGELIST GetImageHandle();

private:

    /// Image list handle
    HIMAGELIST m_himagelist;

    // image count;
    wyUInt32 m_imagecount;
};

#endif
    
