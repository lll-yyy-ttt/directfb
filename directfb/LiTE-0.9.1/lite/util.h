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
 * @brief  This file contains definitions for LiTE utilities.
 * @ingroup LiTE
 * @file util.h
 * <hr>
 **/

#ifndef __LITE__UTIL_H__
#define __LITE__UTIL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <directfb.h>
#include <directfb_util.h>

/* @brief Macro to test for minimum */
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

/* @brief Macro to test for maximum */
#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

/* @brief Macro to clamp values */
#ifndef CLAMP
#define CLAMP(x,l,u) ((x) < (l) ? (l) : ((x) > (u) ? (u) : (x))) 
#endif

/* @brief Test for NULL parameter and return DFB_INVARG */
#define LITE_NULL_PARAMETER_CHECK(exp)                          \
    do {                                                        \
        if ((exp) == NULL) {                                    \
            return DFB_INVARG;                                  \
        }                                                       \
    } while (0)

/* @brief Boolean test for valid LiteWindow or related type */
#define LITE_IS_WINDOW_TYPE(box)                                \
     (LITE_BOX(box)->type >= LITE_TYPE_WINDOW &&                \
      LITE_BOX(box)->type < LITE_TYPE_BOX)

/* @brief Boolean test for valid LiteBox that isn't a LiteWindow */
#define LITE_IS_BOX_TYPE(box)                                   \
     (LITE_BOX(box)->type >= LITE_TYPE_BOX)

/* @brief Test for valid LiteWindow structure, return DFB_INVARG if invalid */
#define LITE_WINDOW_PARAMETER_CHECK(win)                        \
    do {                                                        \
        if (!LITE_IS_WINDOW_TYPE((win))) {                      \
            return DFB_INVARG;                                  \
        }                                                       \
    } while (0)

/* @brief Test for valid LiteBox structure, return DFB_INVARG if invalid */
#define LITE_BOX_PARAMETER_CHECK(box)                           \
    do {                                                        \
        if (!LITE_IS_BOX_TYPE((box))) {                         \
            return DFB_INVARG;                                  \
        }                                                       \
    } while (0)

/* @brief Test for valid LiteBox structure, return DFB_INVARG if invalid */
#define LITE_BOX_TYPE_PARAMETER_CHECK(box, box_type)            \
    do {                                                        \
        if (LITE_BOX(box)->type != (box_type)) {                \
            return DFB_INVARG;                                  \
        }                                                       \
    } while (0)



/* @brief Load image from file 
 * This function will load an image from a file into a IDirectFBSurface
 * structure. You could pass in various values for setting the image
 * format. The file path should be absolute. The image data should 
 * be in a format that the DirectFB image providers support.
 *
 * Note that this function creates a DFB surface each time it is called.
 *
 * @param filename              IN:     File path
 * @param pixelformat           IN:     Pixel format 
 * @param ret_surface           OUT:    Surface
 * @param width                 IN:     Image width
 * @param height                IN:     Image height
 * @param desc                  IN:     Image description
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_util_load_image(const char           *filename,
                               DFBSurfacePixelFormat pixelformat,
                               IDirectFBSurface    **ret_surface,
                               int                  *ret_width,
                               int                  *ret_height,
                               DFBImageDescription  *desc);

DFBResult lite_util_load_image_premultiplied(const char           *filename,
                                             DFBSurfacePixelFormat pixelformat,
                                             IDirectFBSurface    **ret_surface,
                                             int                  *ret_width,
                                             int                  *ret_height,
                                             DFBImageDescription  *desc);

/* @brief Load image from file using surface description 
 * This function will load an image from a file into a IDirectFBSurface
 * using a surface description structure. This could be used
 * for placing DFBSurfaceDescription data inside an application or
 * a shared library, and load this data. The image data should be in
 * a format that the DirectFB image providers support.
 *
 * Note that this function creates a DirectFB surface each time it is called.
 *
 * @param ret_surface           OUT:    Surface
 * @param dsc                   IN:     Surface description
 * 
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_util_load_image_desc(IDirectFBSurface     **ret_surface, 
                                    const DFBSurfaceDescription *dsc);                              

/* @brief Tile image in surface
 * This function will tile images in a pattern format inside a surface
 * using an image from a file. The file path should be absolute. The
 * image data should be in a format that the DirectFB image providers support.
 *
 * @param surface               IN:     Valid surface
 * @param filename              IN:     File with image data 
 * @param x                     IN:     X position of the tiling
 * @param y                     IN:     Y position of the tiling
 * @param nx                    IN:     X offset
 * @param ny                    IN:     Y offset    
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_util_tile_image(IDirectFBSurface *surface,
                               const char       *filename,
                               int               x,
                               int               y,
                               int               nx,
                               int               ny);

/* @brief Get Subsurface of a surface
 * This function return a sub-surface of a DirectFB surface based
 * on size information.
 *
 * @param surface               IN:     Valid surface
 * @param x                     IN:     X position of the subsurface
 * @param y                     IN:     Y position of the subsurface
 * @param width                 IN:     Subsurface width
 * @param height                IN:     Subsurface height
 *
 * @return IDirectFBSurface*     Returns the subsurface, NULL if no valid surface.
 */
IDirectFBSurface* lite_util_sub_surface(IDirectFBSurface *surface,
                                        int               x,
                                        int               y,
                                        int               width,
                                        int               height);


#ifdef __cplusplus
}
#endif

#endif /*  __LITE__UTIL_H__  */
