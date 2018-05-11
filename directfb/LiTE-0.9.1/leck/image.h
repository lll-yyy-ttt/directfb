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
 * @brief  This file contains definitions for the LiTE image interface.
 * @ingroup LiTE
 * @file image.h
 * <hr>
 **/

#ifndef __LITE__IMAGE_H__
#define __LITE__IMAGE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <lite/box.h>
#include <lite/theme.h>

/* @brief Macro to convert a generic LiteBox into a LiteImage */
#define LITE_IMAGE(l) ((LiteImage*)(l))


/* @brief LiteImage structure */
typedef struct _LiteImage LiteImage;

/* @brief Image theme */
typedef struct _LiteImageTheme {
     LiteTheme theme;           /**< base LiTE theme */
     struct {
          IDirectFBSurface *surface;
          int width;
          int height;
     } image;
} LiteImageTheme;     

/* @brief Standard image themes */
/* @brief No image theme */
#define liteNoImageTheme        NULL

/* @brief default image theme */
extern LiteImageTheme *liteDefaultImageTheme;


/* @brief Create a new LiteImage object
 * This function will create a new LiteImage object.
 *
 * @param parent                IN:     Valid parent LiteBox
 * @param rect                  IN:     Rectangle coordinates for the image
 * @param theme                 IN:     Image theme
 * @param ret_image             OUT:    Valid LiteImage object
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_new_image(LiteBox        *parent,
                         DFBRectangle   *rect,
                         LiteImageTheme *theme,
                         LiteImage     **ret_image);

/* @brief Load an image into a LiteImage object 
 * This function will load an image from a file into an existing LiteImage
 * object. The file path should be absolute. The image format should be
 * one of the image provider formats that DirectFB supports. 
 * 
 * @param image                 IN:     Valid LiteImage object
 * @param filename              IN:     Path to file with image
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_load_image(LiteImage *image, const char *filename);

/* @brief Set the Image blitting flags 
 * This function will set the image blitting flags for the blitting
 * operations as part of the internal drawing of the LiteImage.
 * The default flag is DSBLIT_BLEND_ALPHACHANNEL.
 *
 * @param image                 IN:     Valid LiteImage object
 * @param flags                 IN:     DFBSurface blitting flags
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */

DFBResult lite_set_image_blitting_flags(LiteImage *image,
                                        DFBSurfaceBlittingFlags flags);
                        
/* @brief Set the Image clipping area
 * This function will set the image clipping area for the blitting
 * operation. If not specified, the image will be stretch-blitted to
 * its destination
 *
 * @param image                 IN:     Valid LiteImage object
 * @param rect                  IN:     DFBRectangle for the image source
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */

DFBResult lite_set_image_clipping(LiteImage *image,
                                  const DFBRectangle *rect);

/* put the reflection of an image into the specified other image.
 * reflection must be the same size as the original image.
 */
DFBResult lite_set_image_reflection(LiteImage *image, LiteImage *reflection);

/* change the visiblity of the image. Default: visible */
DFBResult lite_set_image_visible(LiteImage *image,
                                 bool visible);

#ifdef __cplusplus
}
#endif

#endif /*  __LITE__IMAGE_H__  */
