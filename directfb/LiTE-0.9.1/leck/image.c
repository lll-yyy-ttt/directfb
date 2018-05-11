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

#include <direct/mem.h>

#include <lite/util.h>

#include "image.h"

D_DEBUG_DOMAIN(LiteImageDomain, "LiTE/Image", "LiteImage");

LiteImageTheme *liteDefaultImageTheme = NULL;

struct _LiteImage {
     LiteBox                  box;
     LiteImageTheme          *theme;
     DFBRectangle             clipping_rect;

     int                      width, height;
     IDirectFBSurface        *surface;

     DFBImageDescription      image_desc;
     DFBSurfaceBlittingFlags  blitting_flags;
};


static DFBResult draw_image(LiteBox *box, const DFBRegion *region, 
                              DFBBoolean clear);
static DFBResult destroy_image(LiteBox *box);

DFBResult 
lite_new_image(LiteBox        *parent, 
               DFBRectangle   *rect,
               LiteImageTheme *theme,
               LiteImage     **ret_image)
{
     LiteImage *image = NULL;
     DFBResult  res;

     image = D_CALLOC(1, sizeof(LiteImage));

     image->box.parent = parent;
     image->theme      = theme;
     image->box.rect   = *rect;

     res = lite_init_box(LITE_BOX(image));
     if (res != DFB_OK) {
          D_FREE(image);
          return res;
     }
     
     image->box.type    = LITE_TYPE_IMAGE;
     image->box.Draw    = draw_image;
     image->box.Destroy = destroy_image;
     
     *ret_image = image;

     D_DEBUG_AT(LiteImageDomain, "Created new image object: %p\n", image);

     return DFB_OK;
}

DFBResult 
lite_load_image(LiteImage *image, const char *filename)
{
     DFBResult res;

     LITE_NULL_PARAMETER_CHECK(image);
     LITE_NULL_PARAMETER_CHECK(filename);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(image), LITE_TYPE_IMAGE);

     D_DEBUG_AT(LiteImageDomain, "Load image from file: %s\n", filename);

     if (image->surface) {
          image->surface->Release( image->surface );
          image->surface = NULL;
     }

     res = lite_util_load_image_premultiplied(filename, DSPF_UNKNOWN, &image->surface,
                                              &image->width, &image->height, &image->image_desc);
     if (res != DFB_OK)
          return res;

     if (image->image_desc.caps & DICAPS_ALPHACHANNEL)
          image->blitting_flags = DSBLIT_BLEND_ALPHACHANNEL;
     else
          image->blitting_flags = DSBLIT_NOFX;

     return lite_update_box(LITE_BOX(image), NULL);
}

DFBResult
lite_set_image_blitting_flags(LiteImage *image,
                              DFBSurfaceBlittingFlags flags)
{
     LITE_NULL_PARAMETER_CHECK(image);

     image->blitting_flags = flags;

     return DFB_OK;
}

DFBResult
lite_set_image_clipping(LiteImage *image,
                        const DFBRectangle *rect)
{
     LITE_NULL_PARAMETER_CHECK(image);

     image->clipping_rect.x = rect->x;
     image->clipping_rect.y = rect->y;
     image->clipping_rect.w = rect->w;
     image->clipping_rect.h = rect->h;

     if (!image->surface)
          return DFB_OK;

     return lite_update_box(LITE_BOX(image), NULL);
}
    
DFBResult lite_set_image_reflection(LiteImage *image, LiteImage *reflection)
{
     LITE_NULL_PARAMETER_CHECK(image);
     LITE_NULL_PARAMETER_CHECK(reflection);

     int from, to;
     int c,y, desty, prevy;
     DFBRectangle rect;

     /* clear the destination image and enable blending */
     reflection->surface->Clear( reflection->surface, 0,0,0,255 );
     reflection->surface->SetPorterDuff( reflection->surface, DSPD_SRC_OVER );
     reflection->surface->SetBlittingFlags( reflection->surface, DSBLIT_BLEND_ALPHACHANNEL |
                                                                 DSBLIT_BLEND_COLORALPHA   |
                                                                 DSBLIT_SRC_PREMULTCOLOR );
     
     from = image->height;
     to   = reflection->height/2;
     desty = 0; prevy = 0;

     rect.x = 0; rect.y = 0; rect.w = image->width; rect.h = 1;
     
     for( y=0; y<from; y++ )
     {
          c = MIN( 255, y*256/from );
          
          desty = (from-1) - to - (y*(to-1))/(from-1);
          if( desty != prevy ) {
               rect.y = y;
               reflection->surface->SetColor( reflection->surface, c,c,c,c );
               reflection->surface->Blit( reflection->surface, image->surface, &rect, 0, desty );
          }
          prevy = desty;
     }
     
     reflection->surface->SetBlittingFlags( reflection->surface, reflection->blitting_flags );
     
     return DFB_OK;
}

DFBResult lite_set_image_visible(LiteImage *image,
                                 bool visible)
{
     if( image->box.is_visible == visible )
          return DFB_OK;
          
     if( visible ) {
          image->box.is_visible = true;
          return lite_update_box( &image->box, NULL );
     }
          
     lite_update_box( &image->box, NULL );
     
     /* make invisible */     
     image->box.is_visible = false;
     return DFB_OK;     
}

/* internals */

static DFBResult 
destroy_image(LiteBox *box)
{
     LiteImage *image = LITE_IMAGE(box);

     D_ASSERT(box != NULL);

     D_DEBUG_AT(LiteImageDomain, "Destroy image: %p\n", image);

     if (!image)
          return DFB_FAILURE;

     if (image->surface)
          image->surface->Release(image->surface);

     return lite_destroy_box(box);
}

static DFBResult 
draw_image(LiteBox         *box, 
           const DFBRegion *region, 
           DFBBoolean       clear)
{
     LiteImage        *image   = LITE_IMAGE(box);
     IDirectFBSurface *surface = box->surface;

     D_ASSERT(box != NULL);

     if (image->surface) {
          surface->SetBlittingFlags(surface, image->blitting_flags);

          if (image->blitting_flags & DSBLIT_BLEND_ALPHACHANNEL)
               surface->SetPorterDuff(surface, DSPD_SRC_OVER);

          if (image->clipping_rect.w != 0 || image->clipping_rect.h != 0)
               surface->StretchBlit(surface, image->surface, &image->clipping_rect, NULL);
          else
               surface->StretchBlit(surface, image->surface, NULL, NULL);
     }
     else if (clear)
          lite_clear_box(box, region);

     return DFB_OK; 
}

