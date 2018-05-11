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

#include "animation.h"

D_DEBUG_DOMAIN(LiteAnimationDomain, "LiTE/Animation", "LiteAnimation");

LiteAnimationTheme *liteDefaultAnimationTheme = NULL;

struct _LiteAnimation {
     LiteBox             box;
     LiteAnimationTheme* theme;

     int                 stretch;

     int                 still_frame;
     int                 current;
     int                 timeout;
     long                last_time;

     IDirectFBSurface*   image;
     int                 frame_width;
     int                 frame_height;
     int                 frames;
     int                 frames_h;
     int                 frames_v;
};

static DFBResult draw_animation(LiteBox *box, const DFBRegion *region, 
                                   DFBBoolean clear);
static DFBResult destroy_animation(LiteBox *box);

DFBResult 
lite_new_animation(LiteBox            *parent, 
                   DFBRectangle       *rect,
                   LiteAnimationTheme *theme,
                   LiteAnimation     **ret_animation)
{
     LiteAnimation *animation = NULL;
     DFBResult res;

     animation = D_CALLOC(1, sizeof(LiteAnimation));

     animation->box.parent = parent;
     animation->theme = theme;

     animation->box.rect.x = rect->x;
     animation->box.rect.y = rect->y;
     animation->box.rect.w = rect->w;
     animation->box.rect.h = rect->h;

     res = lite_init_box(LITE_BOX(animation));
     if (res != DFB_OK) {
          D_FREE(animation);
          return res;
     }

     animation->box.type    = LITE_TYPE_ANIMATION;
     animation->box.Draw    = draw_animation;
     animation->box.Destroy = destroy_animation;

     *ret_animation = animation;
    
     D_DEBUG_AT(LiteAnimationDomain, "Created new animation object: %p\n", animation);

     return DFB_OK;
}

DFBResult 
lite_load_animation(LiteAnimation *animation,
                    const char    *filename,
                    int            still_frame,
                    int            frame_width,
                    int            frame_height)
{
     int               frames, frames_v, frames_h;
     int               image_width, image_height;
     IDirectFBSurface *image = NULL;
     DFBResult         res;

     LITE_NULL_PARAMETER_CHECK(animation);
     LITE_NULL_PARAMETER_CHECK(filename);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(animation), LITE_TYPE_ANIMATION);

     if (frame_width < 1 || frame_height < 1)
          return DFB_INVARG;

     res = lite_util_load_image(filename, DSPF_UNKNOWN, &image,
                                &image_width, &image_height, NULL);
     if(res != DFB_OK)
          return res; 

     if ((image_width % frame_width) || (image_height % frame_height)) {
          D_DEBUG_AT(LiteAnimationDomain, "image width/height "
                   "not a multiple of frame width/height!\n");
          image->Release(image);
          return DFB_FAILURE;
     }

     frames_h = image_width / frame_width;
     frames_v = image_height / frame_height;
     frames   = frames_h * frames_v;

     if (still_frame >= frames) {
          D_DEBUG_AT(LiteAnimationDomain, 
               "lite_load_animation: still frame out of bounds!\n");
          image->Release(image);
          return DFB_FAILURE;
     }

     lite_stop_animation(animation);

     if (animation->image)
          animation->image->Release(animation->image);

     animation->image        = image;
     animation->still_frame  = still_frame;
     animation->current      = -1;
     animation->frame_width  = frame_width;
     animation->frame_height = frame_height;
     animation->frames_h     = frames_h;
     animation->frames_v     = frames_v;
     animation->frames       = frames;

     if (frame_width != animation->box.rect.w  || 
               frame_height != animation->box.rect.h) {
          animation->stretch = 1;
     }

     lite_stop_animation(animation);

     return DFB_OK;
}

DFBResult 
lite_start_animation(LiteAnimation *animation, unsigned int ms_timeout)
{
     LITE_NULL_PARAMETER_CHECK(animation);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(animation), LITE_TYPE_ANIMATION);

     animation->current = (animation->still_frame < 0) ? 0 : animation->still_frame;
     animation->timeout = ms_timeout ? ms_timeout : 1;

     return lite_update_box(LITE_BOX(animation), NULL);
}

int 
lite_update_animation(LiteAnimation *animation)
{
     struct timeval tv;
     long           new_time, diff;

     LITE_NULL_PARAMETER_CHECK(animation);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(animation), LITE_TYPE_ANIMATION);

     if (!animation->timeout)
          return 0;

     gettimeofday(&tv, NULL);

     new_time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
     diff     = new_time - animation->last_time;

     if (diff >= animation->timeout) {
          int advance = diff / animation->timeout;

          animation->current += advance;
          animation->current %= animation->frames;

          animation->last_time += advance * animation->timeout;

          lite_update_box(LITE_BOX(animation), NULL);

          return 1;
     }

     return 0;
}

DFBResult 
lite_stop_animation(LiteAnimation *animation)
{
     LITE_NULL_PARAMETER_CHECK(animation);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(animation), LITE_TYPE_ANIMATION);

     animation->timeout = 0;

     if (animation->still_frame >= 0  &&
         animation->current != animation->still_frame) {
          animation->current = animation->still_frame;

          return lite_update_box(LITE_BOX(animation), NULL);
     }

     return DFB_OK;
}

int 
lite_animation_running(LiteAnimation *animation)
{
     LITE_NULL_PARAMETER_CHECK(animation);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(animation), LITE_TYPE_ANIMATION);

     return animation->timeout > 0;
}

/* internals */

static DFBResult 
destroy_animation(LiteBox *box)
{
     LiteAnimation *animation = LITE_ANIMATION(box);

     D_ASSERT(box != NULL);

     if (animation->image)
          animation->image->Release(animation->image);

     return lite_destroy_box(LITE_BOX(animation));
}

static DFBResult 
draw_animation(LiteBox         *box, 
               const DFBRegion *region, 
               DFBBoolean       clear)
{
     DFBRectangle      rect;
     IDirectFBSurface *surface = box->surface;
     LiteAnimation    *animation = LITE_ANIMATION(box);

     D_ASSERT(box != NULL);

     if (clear)
          lite_clear_box(box, region);

     rect.w = animation->frame_width;
     rect.h = animation->frame_height;
     rect.x = (animation->current % animation->frames_h) * rect.w;
     rect.y = (animation->current / animation->frames_h) * rect.h;

     surface->SetBlittingFlags(surface, DSBLIT_BLEND_ALPHACHANNEL);
     surface->SetClip(surface, region);

     if (animation->stretch)
          surface->StretchBlit(surface, animation->image, &rect, NULL);
     else
          surface->Blit(surface, animation->image, &rect, 0, 0);

     return DFB_OK; 
}
