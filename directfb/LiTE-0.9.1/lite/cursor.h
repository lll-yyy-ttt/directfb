/*
   (c) Copyright 2001-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2005  convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

/**
 * @brief  This file contains definitions for the LiTE cursor interface.
 * @ingroup LiTE
 * @file cursor.h
 * <hr>
 **/

#ifndef __LITE__CURSOR_H__
#define __LITE__CURSOR_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <directfb.h>

#include "window.h"


/* @brief LiteCursor structure
 * LiteCursor is the main abstraction for LiTE cursor handling.
 */
typedef struct {
          IDirectFBSurface *surface;
          int               width;
          int               height;
          int               hot_x;
          int               hot_y;
} LiteCursor;

/* @brief Get current global cursor
 * This function will return the currently active global cursor.
 *
 * @param cursor                OUT:    Current global cursor
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_get_current_cursor(LiteCursor **cursor);

/* @brief Set current global cursor
 * This function will set the global cursor.
 *
 * @param cursor                IN:     Valid LiteCursor object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_current_cursor(LiteCursor *cursor);

/* @brief Load cursor from file
 * This function will load a cursor resource from a file.
 * The file should contain an image type supported by DirectFB
 * image loaders.
 *
 * @param cursor                OUT:    Cursor resource
 * @param path                  IN:     File path to a cursor image
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_load_cursor_from_file(LiteCursor *cursor, const char *path);

/* @brief Load cursor from Image Description
 * This function will load a cursor from a valid DFBSurfaceDescription.
 * This could be used for placing surface descriptions as data inside
 * applications or shared libraries.
 *
 * @param cursor                OUT:    Cursor resource
 * @param dsc                   IN:     DFBSurfaceDescription
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_load_cursor_from_desc(LiteCursor *cursor, DFBSurfaceDescription *dsc);

/* @brief Free cursor object
 * This function will free a cursor object including its memory allocations.
 *
 * @param cursor                IN:     Valid LiteCursor object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_free_cursor(LiteCursor *cursor);

/* @brief Set window cursor
 * This function will set the active cursor for a specific window.
 *
 * @param window                IN:     Valid LiteWindow object
 * @param cursor                IN:     Valid LiteCursor object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_window_cursor(LiteWindow *window, LiteCursor *cursor);


/* @brief Hide current cursor
 * This function will hide the current active cursor
 *
 * @return DFBResult            Returns DFB_OK if successful.
 *
 */
DFBResult lite_hide_cursor(void);

/* @brief Show current cursor
 * This function will show the current active cursor in case
 * it was earlier hidden.
 *
 * @return DFBResult            Returns DFB_OK if successful.
 *
 */
DFBResult lite_show_cursor(void);

/* @brief Change cursor opacity
 * This function will change the opacity level of the currently
 * active cursor. Max opacity level means that the cursor is
 * non-transparent, zero value means that the cursor is not
 * visible.
 *
 * @param opacity               IN:     Opacity level, 0 not visible, 255 fully visible
 *
 * @return DFBResult            Returns DFB_OK if successful.
 *
 */
DFBResult lite_change_cursor_opacity(u8 opacity);


/* @brief Get cursor opacity
 * This function will get the cursor opacity level of the currently active
 * cursor.
 *
 * @param opacity               OUT:    Current opacity level of the active cursor.
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_get_cursor_opacity(u8 *opacity);


/* @brief Set cursor hotspot
 * This function will set the cursor hotspot position. The initial value for a
 * LiteCursor structure is 0,0. Use this after you loaded in a cursor resource,
 * and before it is used.
 *
 * @param cursor                IN:     Valid LiteCursor pointer
 * @param hot_x                 IN:     X hotspot value, 0 means leftmost
 * @param hot_y                 IN:     Y hotspot value, 0 means topmost
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_cursor_hotspot(LiteCursor *cursor, unsigned int hot_x, unsigned int hot_y);



#ifdef __cplusplus
}
#endif

#endif /*  __LITE__CURSOR_H  */
