/* 
   (c) Copyright 2001-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2005  convergence GmbH.

   All rights reserved.

   Written by Daniel Mack <daniel@caiaq.de>
   Based on code by Denis Oliver Kropp <dok@directfb.org>

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

#include <direct/mem.h>

#include <lite/util.h>

#include "progressbar.h"

D_DEBUG_DOMAIN(LiteProgressBarDomain, "LiTE/Progressbar", "LiteProgressBar");

LiteProgressBarTheme *liteDefaultProgressBarTheme = NULL;

struct _LiteProgressBar {
     LiteBox                  box;
     LiteProgressBarTheme    *theme;

     int                      width, height;
     IDirectFBSurface        *surface_fg, *surface_bg;
     float                    value;
};


static DFBResult draw_progressbar(LiteBox *box, const DFBRegion *region, 
                              DFBBoolean clear);
static DFBResult destroy_progressbar(LiteBox *box);

DFBResult 
lite_new_progressbar(LiteBox              *parent, 
                     DFBRectangle         *rect,
                     LiteProgressBarTheme *theme,
                     LiteProgressBar     **ret_progressbar)
{
     LiteProgressBar *progressbar = NULL;
     DFBResult  res;

     progressbar = D_CALLOC(1, sizeof(LiteProgressBar));

     progressbar->box.parent = parent;
     progressbar->theme = theme;
     progressbar->box.rect = *rect;

     res = lite_init_box(LITE_BOX(progressbar));
     if (res != DFB_OK) {
          D_FREE(progressbar);
          return res;
     }

     progressbar->box.type    = LITE_TYPE_PROGRESSBAR;
     progressbar->box.Draw    = draw_progressbar;
     progressbar->box.Destroy = destroy_progressbar;
     progressbar->value       = 0.0f;
     
     *ret_progressbar = progressbar;

     D_DEBUG_AT(LiteProgressBarDomain, "Created new progressbar object: %p\n", progressbar);

     return DFB_OK;
}

DFBResult 
lite_set_progressbar_images(LiteProgressBar *progressbar,
                            const char *filename_fg,
                            const char *filename_bg)
{
     DFBResult res;

     LITE_NULL_PARAMETER_CHECK(progressbar);
     LITE_NULL_PARAMETER_CHECK(filename_fg);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(progressbar), LITE_TYPE_PROGRESSBAR);

     D_DEBUG_AT(LiteProgressBarDomain, "Load progressbar from files: %s %s\n",
                                        filename_fg, filename_bg);

     res = lite_util_load_image(filename_fg, DSPF_UNKNOWN, &progressbar->surface_fg,
                                &progressbar->width, &progressbar->height, NULL);
     if (res != DFB_OK)
          return res;
     
     if (filename_bg) {
          res = lite_util_load_image(filename_bg, DSPF_UNKNOWN, &progressbar->surface_bg,
                                     &progressbar->width, &progressbar->height, NULL);
          if(res != DFB_OK)
                return res;
     }

     return lite_update_box(LITE_BOX(progressbar), NULL);

}

DFBResult lite_set_progressbar_value(LiteProgressBar *progressbar,
                                     float            value)
{
     LITE_NULL_PARAMETER_CHECK(progressbar);

     progressbar->value = value;
     lite_update_box(LITE_BOX(progressbar), NULL);

     return DFB_OK;
}
    

/* internals */

static DFBResult 
destroy_progressbar(LiteBox *box)
{
     LiteProgressBar *progressbar = LITE_PROGRESSBAR(box);

     D_ASSERT(box != NULL);

     D_DEBUG_AT(LiteProgressBarDomain, "Destroy progressbar: %p\n", progressbar);

     if (!progressbar)
          return DFB_FAILURE;

     if (progressbar->surface_fg)
          progressbar->surface_fg->Release(progressbar->surface_fg);
     
     if (progressbar->surface_bg)
          progressbar->surface_fg->Release(progressbar->surface_bg);

     return lite_destroy_box(box);
}

static DFBResult 
draw_progressbar(LiteBox         *box, 
                 const DFBRegion *region, 
                 DFBBoolean       clear)
{
     LiteProgressBar  *progressbar = LITE_PROGRESSBAR(box);
     IDirectFBSurface *surface = box->surface;
     DFBRectangle      rect;

     D_ASSERT(box != NULL);
     
     surface->SetClip(surface, region);

     rect.x = 0;
     rect.y = 0;
     rect.h = box->rect.h;
     rect.w = (int) ((float) box->rect.w * progressbar->value);
     
     if (progressbar->surface_fg) {
          if (progressbar->surface_bg)
	       surface->Blit(surface, progressbar->surface_bg, NULL, 0, 0);

          surface->Blit(surface, progressbar->surface_fg, &rect, 0, 0);
     } else if (clear)
          lite_clear_box(box, region);

     return DFB_OK; 
}

