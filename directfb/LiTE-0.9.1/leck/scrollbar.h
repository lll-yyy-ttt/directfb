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
 * @brief  This file contains definitions for the LiTE scrollbar structure.
 * @ingroup LiTE
 * @file scrollbar.h
 * <hr>
 **/

#ifndef __LITE__SCROLLBAR_H__
#define __LITE__SCROLLBAR_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <lite/box.h>
#include <lite/theme.h>

/* @brief Macro to convert a generic LiteBox into a LiteScrollbar */
#define LITE_SCROLLBAR(l) ((LiteScrollbar*)(l))

/* @brief LiteScrollbar structure */
typedef struct _LiteScrollbar LiteScrollbar;

/* @brief Scrollbar theme */
typedef struct _LiteScrollbarTheme {
     LiteTheme theme;           /**< base LiTE theme */
     int image_margin;
     struct {
          IDirectFBSurface *surface;
          int width;
          int height;
     } all_images;

} LiteScrollbarTheme; 
                                   
/* @brief Standard scrollbar themes */
/* @brief No scrollbar theme */
#define liteNoScrollbarTheme       NULL

/* @brief default scrollbar theme */
extern LiteScrollbarTheme *liteDefaultScrollbarTheme;

/* @brief ScrollInfo structure */
typedef struct _LiteScrollInfo {
     unsigned int min;         /**< minimum range */
     unsigned int max;         /**< maximum range */
     unsigned int page_size;   /**< page size for a proportional scroll bar */
     unsigned int line_size;   /**< line size */
     int          pos; /**< scroll position. It doesn't change while dragging */
     int          track_pos; /**< tracking position. -1 while not dragging */
} LiteScrollInfo;

/* @brief Callback function prototype for scrollbar updates 
 * 
 * @param scrollbar            IN:     Valid LiteScrollbar object
 * @param scrollInfo           IN:     Scroll info
 * @param data                 IN:     Data context
 *
 * @return void
*/
typedef void (*ScrollbarUpdateFunc) (LiteScrollbar *scrollbar,
                                    LiteScrollInfo *info,
                                    void           *data);

/* @brief Create a new LiteScrollbar object
 * This function will create a new LiteScrollbar object.
 *
 * @param parent                IN:     Valid parent LiteBox
 * @rect                        IN:     Rectangle coordinates for the scrollbar
 * @vertical                    IN:     1 if vertical, 0 if horizontal
 * @theme                       IN:     Scrollbar theme. NULL isn't allowed
 * @param ret_slider            OUT:    Returns a valid LiteScrollbar object
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_new_scrollbar(LiteBox         *parent,
                          DFBRectangle       *rect,
                          int                 vertical,
                          LiteScrollbarTheme *theme,
                          LiteScrollbar     **ret_scrollbar);

/* @brief set the scrollbar enable/disable
 *
 * @param scrollbar             IN:     Valid LiteScrollbar object
 * @param enable                IN:     1-enable, 0-disable
 *
 * @return DFBResult            Returns DFB_OK if successful
 */
DFBResult lite_enable_scrollbar(LiteScrollbar *scrollbar, int enable);


/* @brief Set the scroll position
 * 
 * @param scrollbar             IN:     Valid LiteScrollbar object
 * @param pos                   IN:     New scrollbar position
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_set_scroll_pos(LiteScrollbar *scrollbar, int pos);


/* @brief Get the scroll position
 * Get current scroll position. If it is being dragging, it returns
 * track_pos value.
 *
 * @param scrollbar             IN:     Valid LiteScrollbar object
 * @param pos                   OUT:    current pos value. track_pos if dragging
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_get_scroll_pos(LiteScrollbar *scrollbar, int *pos);

/* @brief Set scroll position, range, page size
 *
 * @param scrollbar             IN:     Valid LiteScrollbar object
 * @param info                  IN:     Valid LiteScrollInfo object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_scroll_info(LiteScrollbar *scrollbar, LiteScrollInfo *info);


/* @brief Get scroll position, range, page size
 *
 * @param scrollbar             IN:     Valid LiteScrollbar object
 * @param info                  OUT:    Valid empty LiteScrollInfo object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_get_scroll_info(LiteScrollbar *scrollbar, LiteScrollInfo *info);
                        

/* @brief Install the scrollbar update callback
 * This function will install a scrollbar update callback. This
 * callback is triggered every time the slider position is changed or dragged.
 * 
 * @param scrollbar             IN:     Valid LiteScrollbar object
 * @param callback              IN:     Valid callback function
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_on_scrollbar_update (LiteScrollbar   *scrollbar,
                                ScrollbarUpdateFunc  callback,
                                void                *data);


/* @brief Makes a scrollbar theme from a image
 * This function makes a theme from a image. The image should contain all
 * sub-section images of scrollbar
 *
 * @param image_path            IN:     Image path for a image
 * @param image_margin          IN:     Pixel margin of thumb sub-section image 
 * @param ret_theme             OUT:    Returns a valid theme object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_new_scrollbar_theme(const char           *image_path,
                                   int                  image_margin,
                                   LiteScrollbarTheme **ret_theme);


/* @brief Destroy a scrollbar theme
 * This function releases resources of theme and free object's memory
 *
 * @param theme                 IN:     theme to destroy
 *
 * @return DFBResult            Returns DFB_OK If successful.
 */
DFBResult lite_destroy_scrollbar_theme(LiteScrollbarTheme *theme);


#ifdef __cplusplus
}
#endif

#endif /*  __LITE__SCROLLBAR_H__  */
