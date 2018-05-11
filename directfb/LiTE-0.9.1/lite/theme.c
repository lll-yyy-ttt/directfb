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

#include <direct/debug.h>

#include "lite_internal.h"
#include "lite_config.h"

#include "util.h"

#include "theme.h"

D_DEBUG_DOMAIN(LiteThemeDomain, "LiTE/Theme", "LiteTheme");

DFBResult
lite_theme_frame_load( LiteThemeFrame  *frame,
                       const char     **filenames )
{
     int       i, y;
     int       width  = 0;
     int       height = 0;
     DFBResult ret;

     D_DEBUG_ENTER( LiteThemeDomain, "( %p, %p )", frame, filenames );

     D_ASSERT( frame != NULL );
     D_ASSERT( filenames != NULL );

     for (i=0; i<_LITE_THEME_FRAME_PART_NUM; i++) {
          D_ASSERT( filenames[i] != NULL );

          ret = lite_util_load_image( filenames[i],
                                      DSPF_UNKNOWN,
                                      &frame->parts[i].source,
                                      &frame->parts[i].rect.w,
                                      &frame->parts[i].rect.h,
                                      NULL );
          if (ret) {
               D_DERROR( ret, "Lite/ThemeFrame: Loading '%s' failed!\n", filenames[i] );

               while (i--)
                    frame->parts[i].source->Release( frame->parts[i].source );

               return ret;
          }

          if (width < frame->parts[i].rect.w)
               width = frame->parts[i].rect.w;

          height += frame->parts[i].rect.h;
     }


     IDirectFB             *dfb;
     IDirectFBSurface      *compact;
     DFBSurfaceDescription  desc;

     desc.flags       = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_PIXELFORMAT;
     desc.width       = width;
     desc.height      = height;
     desc.pixelformat = DSPF_ARGB; //FIXME

     DirectFBCreate( &dfb );

     dfb->CreateSurface( dfb, &desc, &compact );

     compact->Clear( compact, 0, 0, 0, 0 );

     for (i=0, y=0; i<_LITE_THEME_FRAME_PART_NUM; i++) {
          compact->Blit( compact, frame->parts[i].source, &frame->parts[i].rect, 0, y );

          compact->AddRef( compact );

          frame->parts[i].source->Release( frame->parts[i].source );

          frame->parts[i].source = compact;

          frame->parts[i].rect.x = 0;
          frame->parts[i].rect.y = y;

          y += frame->parts[i].rect.h;
     }

     compact->ReleaseSource( compact );
     compact->Release( compact );

     dfb->Release( dfb );

     D_MAGIC_SET( frame, LiteThemeFrame );

     return DFB_OK;
}

void
lite_theme_frame_unload( LiteThemeFrame *frame )
{
     int i;

     D_DEBUG_ENTER( LiteThemeDomain, "( %p )", frame );

     D_MAGIC_ASSERT( frame, LiteThemeFrame );

     for (i=0; i<_LITE_THEME_FRAME_PART_NUM; i++) {
          if (frame->parts[i].source) {
               frame->parts[i].source->Release( frame->parts[i].source );
               frame->parts[i].source = NULL;
          }
     }

     D_MAGIC_CLEAR( frame );
}

void
lite_theme_frame_target_update( LiteThemeFrameTarget *target,
                                const LiteThemeFrame *frame,
                                const DFBDimension   *size )
{
     D_DEBUG_ENTER( LiteThemeDomain, "( %p <- %p [%dx%d] )", target, frame, size->w, size->h );

     D_MAGIC_ASSERT( frame, LiteThemeFrame );

     target->dest[LITE_THEME_FRAME_PART_TOPLEFT].x = 0;
     target->dest[LITE_THEME_FRAME_PART_TOPLEFT].y = 0;
     target->dest[LITE_THEME_FRAME_PART_TOPLEFT].w = frame->parts[LITE_THEME_FRAME_PART_TOPLEFT].rect.w;
     target->dest[LITE_THEME_FRAME_PART_TOPLEFT].h = frame->parts[LITE_THEME_FRAME_PART_TOPLEFT].rect.h;

     target->dest[LITE_THEME_FRAME_PART_TOPRIGHT].x = size->w - frame->parts[LITE_THEME_FRAME_PART_TOPRIGHT].rect.w;
     target->dest[LITE_THEME_FRAME_PART_TOPRIGHT].y = 0;
     target->dest[LITE_THEME_FRAME_PART_TOPRIGHT].w = frame->parts[LITE_THEME_FRAME_PART_TOPRIGHT].rect.w;
     target->dest[LITE_THEME_FRAME_PART_TOPRIGHT].h = frame->parts[LITE_THEME_FRAME_PART_TOPRIGHT].rect.h;


     target->dest[LITE_THEME_FRAME_PART_BOTTOMLEFT].x = 0;
     target->dest[LITE_THEME_FRAME_PART_BOTTOMLEFT].y = size->h - frame->parts[LITE_THEME_FRAME_PART_BOTTOMLEFT].rect.h;
     target->dest[LITE_THEME_FRAME_PART_BOTTOMLEFT].w = frame->parts[LITE_THEME_FRAME_PART_BOTTOMLEFT].rect.w;
     target->dest[LITE_THEME_FRAME_PART_BOTTOMLEFT].h = frame->parts[LITE_THEME_FRAME_PART_BOTTOMLEFT].rect.h;

     target->dest[LITE_THEME_FRAME_PART_BOTTOMRIGHT].x = size->w - frame->parts[LITE_THEME_FRAME_PART_BOTTOMRIGHT].rect.w;
     target->dest[LITE_THEME_FRAME_PART_BOTTOMRIGHT].y = size->h - frame->parts[LITE_THEME_FRAME_PART_BOTTOMRIGHT].rect.h;
     target->dest[LITE_THEME_FRAME_PART_BOTTOMRIGHT].w = frame->parts[LITE_THEME_FRAME_PART_BOTTOMRIGHT].rect.w;
     target->dest[LITE_THEME_FRAME_PART_BOTTOMRIGHT].h = frame->parts[LITE_THEME_FRAME_PART_BOTTOMRIGHT].rect.h;


     target->dest[LITE_THEME_FRAME_PART_TOP].x = target->dest[LITE_THEME_FRAME_PART_TOPLEFT].w;
     target->dest[LITE_THEME_FRAME_PART_TOP].y = 0;
     target->dest[LITE_THEME_FRAME_PART_TOP].w = target->dest[LITE_THEME_FRAME_PART_TOPRIGHT].x -
                                                 target->dest[LITE_THEME_FRAME_PART_TOP].x;
     target->dest[LITE_THEME_FRAME_PART_TOP].h = frame->parts[LITE_THEME_FRAME_PART_TOP].rect.h;

     target->dest[LITE_THEME_FRAME_PART_BOTTOM].x = target->dest[LITE_THEME_FRAME_PART_BOTTOMLEFT].w;
     target->dest[LITE_THEME_FRAME_PART_BOTTOM].y = size->h - frame->parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h;
     target->dest[LITE_THEME_FRAME_PART_BOTTOM].w = target->dest[LITE_THEME_FRAME_PART_BOTTOMRIGHT].x -
                                                    target->dest[LITE_THEME_FRAME_PART_BOTTOM].x;
     target->dest[LITE_THEME_FRAME_PART_BOTTOM].h = frame->parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h;


     target->dest[LITE_THEME_FRAME_PART_LEFT].x = 0;
     target->dest[LITE_THEME_FRAME_PART_LEFT].y = target->dest[LITE_THEME_FRAME_PART_TOPLEFT].h;
     target->dest[LITE_THEME_FRAME_PART_LEFT].w = frame->parts[LITE_THEME_FRAME_PART_LEFT].rect.w;
     target->dest[LITE_THEME_FRAME_PART_LEFT].h = target->dest[LITE_THEME_FRAME_PART_BOTTOMLEFT].y -
                                                  target->dest[LITE_THEME_FRAME_PART_LEFT].y;

     target->dest[LITE_THEME_FRAME_PART_RIGHT].x = size->w - frame->parts[LITE_THEME_FRAME_PART_RIGHT].rect.w;
     target->dest[LITE_THEME_FRAME_PART_RIGHT].y = target->dest[LITE_THEME_FRAME_PART_TOPRIGHT].h;
     target->dest[LITE_THEME_FRAME_PART_RIGHT].w = frame->parts[LITE_THEME_FRAME_PART_RIGHT].rect.w;
     target->dest[LITE_THEME_FRAME_PART_RIGHT].h = target->dest[LITE_THEME_FRAME_PART_BOTTOMRIGHT].y -
                                                   target->dest[LITE_THEME_FRAME_PART_RIGHT].y;
}

