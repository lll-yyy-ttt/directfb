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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lite_internal.h"
#include "lite_config.h"
#include "lite.h"
#include "util.h"
#include "window.h"

#include "cursor.h"

D_DEBUG_DOMAIN(LiteCursorDomain, "LiTE/Cursor", "LiteCursor");

static LiteCursor *cursor_global = NULL;
static u8 cursor_opacity_global = 255;        /* default opacity is full */

DFBResult
lite_get_current_cursor(LiteCursor** cursor)
{
     LITE_NULL_PARAMETER_CHECK(cursor);
     *cursor = cursor_global;

     return DFB_OK;
}


DFBResult
lite_set_current_cursor(LiteCursor* cursor)
{
     LITE_NULL_PARAMETER_CHECK(cursor);

     cursor_global = cursor;

     return DFB_OK;
}

DFBResult
lite_load_cursor_from_file(LiteCursor *cursor, const char *path)
{
     DFBResult res = DFB_OK;
     int       width;
     int       height;

     LITE_NULL_PARAMETER_CHECK(cursor);
     LITE_NULL_PARAMETER_CHECK(path);

     D_DEBUG_AT(LiteCursorDomain, "Load cursor from file: %s\n", path);

     res = lite_util_load_image(path, DSPF_UNKNOWN,
                                   &cursor->surface,
                                   &width,
                                   &height,
                                   NULL);
     if (res != DFB_OK)
          return res;

     cursor->height = height;
     cursor->width = width;
     cursor->hot_x = 0;
     cursor->hot_y = 0;

     return res;
}

DFBResult
lite_load_cursor_from_desc(LiteCursor  *cursor, DFBSurfaceDescription *dsc)
{
     DFBResult res;

     LITE_NULL_PARAMETER_CHECK(cursor);
     LITE_NULL_PARAMETER_CHECK(dsc);

     D_DEBUG_AT(LiteCursorDomain, "Load cursor from surface descriptor: %p\n", dsc);

     res = lite_util_load_image_desc(&cursor->surface, dsc);

     if (res != DFB_OK)
          return res;

     cursor->width = dsc->width;
     cursor->height = dsc->height;
     cursor->hot_x = 0;
     cursor->hot_y = 0;

     return res;
}

DFBResult
lite_free_cursor(LiteCursor *cursor)
{
     LITE_NULL_PARAMETER_CHECK(cursor);

     D_DEBUG_AT(LiteCursorDomain, "Free cursor: %p\n", cursor);

     if (cursor_global == cursor)
          cursor_global = NULL;

     if(cursor->surface)
          cursor->surface->Release(cursor->surface);

     return DFB_OK;
}

DFBResult
lite_set_window_cursor(LiteWindow *window, LiteCursor *cursor)
{
     DFBResult        res = DFB_OK;

     LITE_NULL_PARAMETER_CHECK(window);
     LITE_NULL_PARAMETER_CHECK(cursor);

     D_DEBUG_AT(LiteCursorDomain, "Set cursor: %p for window: %p\n",
                    cursor, window);

     IDirectFBWindow *win = window->window;
     if (cursor->surface) {
         res = win->SetCursorShape(win, cursor->surface, cursor->hot_x, cursor->hot_y);
     }

     return res;
}

DFBResult
lite_show_cursor(void)
{
     D_DEBUG_AT(LiteCursorDomain, "Show cursor...\n");

     return lite_change_cursor_opacity(255);
}

DFBResult
lite_hide_cursor(void)
{
     D_DEBUG_AT(LiteCursorDomain, "Hide cursor...\n");

     return lite_change_cursor_opacity(0);
}

DFBResult
lite_change_cursor_opacity(u8 opacity)
{
     DFBResult res = DFB_OK;

     D_DEBUG_AT(LiteCursorDomain, "Change cursor opacity to %d\n", opacity);

     res = lite_layer->SetCooperativeLevel(lite_layer, DLSCL_ADMINISTRATIVE);
     res = lite_layer->SetCursorOpacity(lite_layer, opacity);
     res = lite_layer->SetCooperativeLevel(lite_layer, DLSCL_SHARED);

     cursor_opacity_global = opacity;

     return res;
}

DFBResult
lite_get_cursor_opacity(u8 *opacity)
{
     LITE_NULL_PARAMETER_CHECK(opacity);
     *opacity = cursor_opacity_global;

     return DFB_OK;
}

DFBResult
lite_set_cursor_hotspot(LiteCursor *cursor, unsigned int hot_x, unsigned int hot_y)
{
     LITE_NULL_PARAMETER_CHECK(cursor);

     cursor->hot_x = hot_x;
     cursor->hot_y = hot_y;

     return DFB_OK;
}

