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
 * @brief  This file contains definitions for the LiTE LiteButton structure.
 * @ingroup LiTE
 * @file textbutton.h
 * <hr>
 **/

#ifndef __LITE__TEXT_BUTTON_H__
#define __LITE__TEXT_BUTTON_H__

#ifdef __cplusplus
extern "C"
{
#endif


#include <directfb.h>

#include <lite/box.h>
#include <lite/theme.h>
#include <lite/font.h>

/* @brief Macro to convert a generic LiteBox to a LiteTextButton */
#define LITE_TEXTBUTTON(l) ((LiteTextButton*)(l))


/* @brief LiteTextButton structure that is derived from a LiteBox */
typedef struct _LiteTextButton LiteTextButton;

/* @brief ButtonState reflects the various states the button will be drawn in */
typedef enum { 
  LITE_TBS_NORMAL,                       /**< Button is in a normal draw state */
  LITE_TBS_PRESSED,                      /**< Button is in a pressed draw state */
  LITE_TBS_HILITE,                       /**< Button is in a hilite draw state */
  LITE_TBS_DISABLED                      /**< Button is in a disabled draw state */ 
} LiteTextButtonState;

/* @brief LiteTextButton theme */
typedef struct _LiteTextButtonTheme {
     LiteTheme theme;                   /**< base LiTE theme */

     struct {
          IDirectFBSurface *surface;
          int width;
          int height;
     } all_images;                      /** vertically concatenated 
                                        normal,pressed,hilite,disabled image */

     LiteFont *font;			/**< caption text font */
     DFBColor font_color;		/**< color used for the font */
} LiteTextButtonTheme;

/* @brief Standard text button themes */
/* @brief No button theme */
#define liteNoTextButtonTheme       NULL

/* @brief LiTE default text button theme */
extern LiteTextButtonTheme *liteDefaultTextButtonTheme;



/* @brief Callback function prototype
 * This callback is used for installing functions when the button is pressed.
 */
typedef void (*TextButtonPressFunc) (LiteTextButton *button, void *ctx);

/* @brief Create a new LiteTextButton object
 * This function will create a new LiteButton object
 *
 * @param parent                IN:     Valid parent LiteBox
 * @param caption_text		IN:	Button caption string
 * @param rect                  IN:     Rectangle for the button coordinates
 * @param theme                 IN:     Button theme. NULL isn't allowed
 * @param ret_button            OUT:    Valid LiteButton object
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_new_text_button(LiteBox        *parent,
                          DFBRectangle        *rect,
			  const char	      *caption_text,
                          LiteTextButtonTheme *theme,
                          LiteTextButton      **ret_button);

/* @brief Set the LiteTextButton caption string
 * This function will set the caption string of a LiteTextButton.
 * If the string is too long to be drawn inside the LiteTextButton,
 * some portion from the end of the string will be drawn as "..."
 *
 * @param button		IN:	Valid LiteTextButton object
 * @param caption_text		IN:	Button caption string
 *
 * @return DFBResult		Returns DFB_OK if successful.
 */
DFBResult lite_set_text_button_caption(LiteTextButton *button, const char* caption_text);

/* @brief Enable the LiteTextButton
 * This function will enable the LiteTextButton.
 * 
 * @param button                IN:     Valid LiteTextButton object
 * @param enabled               IN:     Enabled state
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_enable_text_button(LiteTextButton *button, int enabled);

/* @brief Set the LiteTextButton state
 * This function will set the LiteTextButton state based on the ButtonState
 * constants.
 *
 * @param button                IN:     Valid LiteButton object
 * @param state                 IN:     LiteTextButton state
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_text_button_state(LiteTextButton *button, LiteTextButtonState state);

/* @brief Install a Button Press Callback function
 * This function will install a TextButtonPressFunc callback for triggering
 * when a LiteTextButton changes TextButtonState. You could also pass in data
 * that is then returned when the callback triggers.
 *
 * @param button                IN:     Valid LiteTextButton object
 * @param callback              IN:     Callback
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_on_text_button_press(LiteTextButton     *button,
                                    TextButtonPressFunc callback,
                                    void           *data);

/* @brief Makes a text button theme
 * This function makes a theme. If you want, you can change font
 * of the theme returned from this function.
 *
 * @param image_pat             IN:     Image path
 * @param font_spec             IN:     Font specification
 * @param ret_theme             OUT:    Returns new theme object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_new_text_button_theme(
                             const char           *image_path,
                             LiteTextButtonTheme **ret_theme);

/* @brief Destroy a text button theme
 * This function releases resources of theme and free its memory
 *
 * @return DFBResult            Returns DFB_OK If successful.
 */
DFBResult lite_destroy_text_button_theme(LiteTextButtonTheme *theme);
                        
#ifdef __cplusplus
}
#endif

#endif /*  __LITE__TEXT_BUTTON_H__  */
