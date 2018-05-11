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
 * @brief  This file contains definitions for the LiTE theme structures and interfaces.
 * @ingroup LiTE
 * @file theme.h
 * <hr>
 **/

#ifndef __LITE__THEME_H__
#define __LITE__THEME_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <directfb.h>



typedef struct {
     IDirectFBSurface   *source;     /**< surface holding frame part image */
     DFBRectangle        rect;       /**< portion of surface containing the frame part image */
} LiteThemeFramePart;

typedef enum {
     LITE_THEME_FRAME_PART_TOP,
     LITE_THEME_FRAME_PART_BOTTOM,
     LITE_THEME_FRAME_PART_LEFT,
     LITE_THEME_FRAME_PART_RIGHT,
     LITE_THEME_FRAME_PART_TOPLEFT,
     LITE_THEME_FRAME_PART_TOPRIGHT,
     LITE_THEME_FRAME_PART_BOTTOMLEFT,
     LITE_THEME_FRAME_PART_BOTTOMRIGHT,

     _LITE_THEME_FRAME_PART_NUM
} LiteThemeFramePartIndex;

typedef struct {
     int                 magic;

     LiteThemeFramePart  parts[_LITE_THEME_FRAME_PART_NUM];
} LiteThemeFrame;


typedef struct {
     DFBRectangle        dest[_LITE_THEME_FRAME_PART_NUM];  /**< rectangles on destination surface, e.g. of a window */
} LiteThemeFrameTarget;



DFBResult lite_theme_frame_load( LiteThemeFrame  *frame,
                                 const char     **filenames );

void      lite_theme_frame_unload( LiteThemeFrame *frame );

void      lite_theme_frame_target_update( LiteThemeFrameTarget *target,
                                          const LiteThemeFrame *frame,
                                          const DFBDimension   *size );



typedef enum {
     LITE_NO_THEME_FLAGS      = 0x00000000,

     LITE_THEME_HAS_FRAME     = 0x00000001,

     LITE_ALL_THEME_FLAGS     = 0x00000001,
} LiteThemeFlags;

/* @brief LiteTheme base theme structure */
typedef struct _LiteTheme {
     LiteThemeFlags flags;

     DFBColor       bg_color;                  /**< background color */
     DFBColor       fg_color;                  /**< foreground color */

     LiteThemeFrame frame;            /**< Frame structure */
} LiteTheme;


/* @brief Macro to convert a specific theme into a LiteTheme */
#define LITE_THEME(t) ((LiteTheme *)(t))



#ifdef __cplusplus
}
#endif

#endif /*  __LITE__THEME_H  */
