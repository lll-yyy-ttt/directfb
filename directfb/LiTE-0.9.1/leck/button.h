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
 * @brief  This file contains definitions for the LiTE LiteButton structure.
 * @ingroup LiTE
 * @file button.h
 * <hr>
 **/

#ifndef __LITE__BUTTON_H__
#define __LITE__BUTTON_H__

#ifdef __cplusplus
extern "C"
{
#endif


#include <directfb.h>

#include <lite/box.h>
#include <lite/theme.h>

/* @brief Macro to convert a generic LiteBox to a LiteButton */
#define LITE_BUTTON(l) ((LiteButton*)(l))


/* @brief LiteButton structure that is derived from a LiteBox */
typedef struct _LiteButton LiteButton;

/* @brief ButtonState reflects the various states the button will be drawn in */
typedef enum { 
  LITE_BS_NORMAL,                       /**< Button is in a normal draw state */
  LITE_BS_PRESSED,                      /**< Button is in a pressed draw state */
  LITE_BS_HILITE,                       /**< Button is in a hilite (and off) draw state */
  LITE_BS_DISABLED,                     /**< Button is in a disabled (and off) draw state */
  LITE_BS_HILITE_ON,                    /**< Button is in a hilite (and on) draw state */
  LITE_BS_DISABLED_ON,                  /**< Button is in a disabled (and on) draw state */
  LITE_BS_MAX
} LiteButtonState;

/* @brief ButtonType reflects the types a button can be */
typedef enum {
  LITE_BT_PUSH,                         /**< Push button (default) */
  LITE_BT_TOGGLE                        /**< Toggle button */
} LiteButtonType;

/* @brief LiteButton theme */
typedef struct _LiteButtonTheme {
     LiteTheme theme;                   /**< base LiTE theme */

     struct {
          struct {
               IDirectFBSurface *surface;
               int width;
               int height;
          } normal;                             /** normal image */
          struct {
               IDirectFBSurface *surface;
               int width;
               int height;
          } pressed;                            /** pressed image */
          struct {
               IDirectFBSurface *surface;
               int width;
               int height;
          } hilite;                             /**< hilite image */
          struct {
               IDirectFBSurface *surface;
               int width;
               int height;
          } disabled;                           /**< disabled image */
     } images;                                  /**< button images  */
} LiteButtonTheme;

/* @brief Standard button themes */
/* @brief No button theme */
#define liteNoButtonTheme       NULL

/* @brief LiTE default button theme */
extern LiteButtonTheme *liteDefaultButtonTheme;



/* @brief Callback function prototype
 * This callback is used for installing functions when the button is pressed.
 */
typedef void (*ButtonPressFunc) (LiteButton *button, void *ctx);

/* @brief Create a new LiteButton object
 * This function will create a new LiteButton object
 *
 * @param parent                IN:     Valid parent LiteBox
 * @param rect                  IN:     Rectangle for the button coordinates
 * @param theme                 IN:     Button theme
 * @param ret_button            OUT:    Valid LiteButton object
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_new_button(LiteBox         *parent,
                          DFBRectangle    *rect,
                          LiteButtonTheme *theme,
                          LiteButton     **ret_button);

/* @brief Enable the LiteButton
 * This function will enable the LiteButton.
 * 
 * @param button                IN:     Valid LiteButton object
 * @param enabled               IN:     Enabled state
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_enable_button(LiteButton *button, int enabled);

/* @brief Set the LiteButton type
 * This function will set the LiteButton type based on the ButtonType
 * constants.
 * 
 * @param button                IN:     Valid LiteButton object
 * @param type                  IN:     LiteButton type
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_button_type(LiteButton *button, LiteButtonType type);

/* @brief Set the LiteButton state
 * This function will set the LiteButton state based on the ButtonState
 * constants.
 *
 * @param button                IN:     Valid LiteButton object
 * @param state                 IN:     LiteButton state
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_button_state(LiteButton *button, LiteButtonState state);

/* @brief Get the LiteButton state
 * This function will return the LiteButton state based on the ButtonState
 * constants.
 *
 * @param button                IN:     Valid LiteButton object
 * @param state                 OUT:    LiteButton state
 *
 * @return DFBResult            Returns LiteButton state
 */
DFBResult lite_get_button_state(LiteButton *button, LiteButtonState *state);

/* @brief Set the LiteButton image
 * This function will set the image of a LiteButton. 
 * The file should contain an image type supported by a
 * DirectFB image provider. Each ButtonState could have
 * a separate image, if no image is provided then the state
 * will use whatever image is set in a supported state.
 *
 * @param button                IN:     Valid LiteButton object
 * @param state                 IN:     ButtonState
 * 
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_button_image(LiteButton      *button,
                                LiteButtonState  state,
                                const char      *filename);

/* @brief Set the LiteButton image using an Image Description
 * This function will set the LiteButton image using a 
 * DirectFB DFBSurfaceDescription. Such description structures
 * could be embedded inside application code or shared libraries
 *
 * @param button                IN:     Valid LiteButton object
 * @param state                 IN:     ButtonState
 * @param dsc                   IN:     DFBSurfaceDescription structure
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_button_image_desc(LiteButton            *button, 
                                     LiteButtonState        state, 
                                     DFBSurfaceDescription *dsc);

/* @brief Set the LiteButton surface based on ButtonState
 * This function is used to directly set the underlying IDirectFBSurface
 * for a LiteButton ButtonState. This could be used for controlling 
 * compound LiteObjects and directly set the image resourcesa using
 * a set of reusable surface elements.
 *
 * @param button                IN:     Valid LiteButton object
 * @param state                 IN:     ButtonState
 * @param surface               IN:     IDirectFBSurface
 * 
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_button_image_surface(LiteButton       *button, 
                                        LiteButtonState   state,
                                        IDirectFBSurface *surface); 

/* @brief Install a Button Press Callback function
 * This function will install a ButtonPressFunc callback for triggering
 * when a LiteButton changes ButtonState. You could also pass in data
 * that is then returned when the callback triggers.
 *
 * @param button                IN:     Valid LiteButton object
 * @param callback              IN:     Callback
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
int lite_on_button_press(LiteButton     *button,
                         ButtonPressFunc callback,
                         void           *data);
                        
#ifdef __cplusplus
}
#endif

#endif /*  __LITE__BUTTON_H__  */
