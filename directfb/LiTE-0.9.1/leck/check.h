/* 
   (c) Copyright 2001-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2005  convergence GmbH.

   All rights reserved.

   Written by Yoo Chihoon <rhoon@nemustech>

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
 * @brief  This file contains definitions for the LiTE Check structure.
 * @ingroup LiTE
 * @file check.h
 * <hr>
 **/

#ifndef __LITE__CHECK_H__
#define __LITE__CHECK_H__

#ifdef __cplusplus
extern "C"
{
#endif


#include <directfb.h>

#include <lite/box.h>
#include <lite/theme.h>
#include <lite/font.h>

/* @brief Macro to convert a generic LiteBox to a LiteCheck */
#define LITE_CHECK(l) ((LiteCheck*)(l))


/* @brief LiteCheck structure that is derived from a LiteBox */
typedef struct _LiteCheck LiteCheck;

/* @brief CheckState reflects the 'checked'/'unchecked' states fo check control */
typedef enum {
  LITE_CHS_UNCHECKED,
  LITE_CHS_CHECKED
} LiteCheckState;

#define LITE_CHECK_IMAGE_COUNT     (6)

/* @brief LiteCheck theme */
typedef struct _LiteCheckTheme {
     LiteTheme theme;                   /**< base LiTE theme */

     struct {
          IDirectFBSurface *surface;
          int width;
          int height;
     } all_images;                      /** all LITE_CHECK_IMAGE_COUNT images */

     LiteFont *font;			/**< caption text font */
     DFBColor font_color;		/**< color used for the font */
} LiteCheckTheme;

/* @brief Standard check  themes */
/* @brief No check theme */
#define liteNoCheckTheme       NULL

/* @brief LiTE default check theme */
extern LiteCheckTheme *liteDefaultCheckTheme;



/* @brief Callback function prototype
 * This callback is used for installing functions when the check is pressed.
 */
typedef void (*CheckPressFunc) (LiteCheck *check, LiteCheckState new_status, void *ctx);

/* @brief Create a new LiteCheck object
 * This function will create a new LiteCheck object
 *
 * @param parent                IN:     Valid parent LiteBox
 * @param caption_text		IN:	Check caption string
 * @param rect                  IN:     Rectangle for the check coordinates
 * @param theme                 IN:     Check theme
 * @param ret_check             OUT:    Valid LiteCheck object
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_new_check(LiteBox   *parent,
                    DFBRectangle   *rect,
	            const char     *caption_text,
                    LiteCheckTheme *theme,
                    LiteCheck      **ret_check);

/* @brief Set the LiteCheck caption string
 * This function will set the caption string of a LiteCheck.
 * If the string is too long to be drawn inside the LiteCheck,
 * some portion from the end of the string will be drawn as "..."
 *
 * @param check 		IN:	Valid LiteCheck object
 * @param caption_text		IN:	Button caption string
 *
 * @return DFBResult		Returns DFB_OK if successful.
 */
DFBResult lite_set_check_caption(LiteCheck *check, const char* caption_text);

/* @brief Enable the LiteCheck
 * This function will enable the LiteCheck.
 * 
 * @param check                 IN:     Valid Check object
 * @param enabled               IN:     Enabled state
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_enable_check(LiteCheck *check, int enabled);


/* @brief Check the LiteCheck
 * This function will check/uncheck the LiteCheck.
 * 
 * @param check                 IN:     Valid Check object
 * @param check_state           IN:     check state
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_check_check(LiteCheck *check, LiteCheckState check_state);

/* @brief Retreive check state of the control
 *
 * @param check                 IN:     Valid LiteCheck object
 * @param ret_state             OUT:    state
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_get_check_state(LiteCheck *check, LiteCheckState *ret_state);

/* @brief Install a Check Press Callback function
 * This function will install a CheckPressFunc callback for triggering
 * when a LiteCheck changes CheckState. You could also pass in data
 * that is then returned when the callback triggers.
 *
 * @param check                 IN:     Valid LiteCheck object
 * @param callback              IN:     Callback
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_on_check_press(LiteCheck     *check,
                             CheckPressFunc callback,
                             void           *data);

/* @brief Makes a check theme
 *
 * @param image_path            IN:     Image path for all state
 * @param ret_theme             OUT:    Returns new theme
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_new_check_theme(const char      *image_path,
                               LiteCheckTheme **ret_theme);

/* @brief Destroy defaultCheckTheme
 * This function releases resources of theme and free its memory.
 *
 * @return DFBResult            Returns DFB_OK If successful.
 */
DFBResult lite_destroy_check_theme(LiteCheckTheme *theme);
                        
#ifdef __cplusplus
}
#endif

#endif /*  __LITE__CHECK_H__  */
