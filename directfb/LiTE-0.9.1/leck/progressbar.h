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

/**
 * @brief  This file contains definitions for the LiTE progressbar interface.
 * @ingroup LiTE
 * @file progressbar.h
 * <hr>
 **/

#ifndef __LITE__PROGRESSBAR_H__
#define __LITE__PROGRESSBAR_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <lite/box.h>
#include <lite/theme.h>

/* @brief Macro to convert a generic LiteBox into a LiteProgressBar */
#define LITE_PROGRESSBAR(l) ((LiteProgressBar*)(l))

/* @brief LiteProgressBar structure */
typedef struct _LiteProgressBar LiteProgressBar;

/* @brief Image theme */
typedef struct _LiteProgressBarTheme {
     LiteTheme theme;           /**< base LiTE theme */
     struct {
          IDirectFBSurface *surface;
          int width;
          int height;
     } progressbar;
} LiteProgressBarTheme;     

/* @brief Standard progressbar themes */
/* @brief No progressbar theme */
#define liteNoProgressBarTheme        NULL

/* @brief default progressbar theme */
extern LiteProgressBarTheme *liteDefaultProgressBarTheme;


/* @brief Create a new LiteProgressBar object
 * This function will create a new LiteProgressBar object.
 *
 * @param parent                IN:     Valid parent LiteBox
 * @param rect                  IN:     Rectangle coordinates for the progressbar
 * @param theme                 IN:     Image theme
 * @param ret_progressbar       OUT:    Valid LiteProgressBar object
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_new_progressbar(LiteBox        *parent,
                               DFBRectangle   *rect,
                               LiteProgressBarTheme *theme,
                               LiteProgressBar     **ret_progressbar);

/* @brief Set the images for a LiteProgressBar object 
 * This function will load images from files into an existing 
 * LiteProgressBar object. The file paths should be absolute.
 * The image format shoulds be one of the progressbar provider
 * formats that DirectFB supports. 
 * 
 * @param progressbar           IN:     Valid LiteProgressBar object
 * @param filename_fg           IN:     Path to file with progressbar image (foreground)
 * @param filename_bg           IN:     Path to file with progressbar image (background)
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_set_progressbar_images(LiteProgressBar *progressbar,
                                      const char *filename_fg,
                                      const char *filename_bg);

/* @brief Set the current value of a LiteProgressBar object
 * This function will set the progressbar's value by displaying
 * a fraction of the associated image.
 *
 * @param progressbar           IN:     Valid LiteProgressBar object
 * @param value                 IN:     Value in percentage
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */

DFBResult lite_set_progressbar_value(LiteProgressBar *progressbar,
                                     float value);
                        

#ifdef __cplusplus
}
#endif

#endif /*  __LITE__PROGRESSBAR_H__  */
