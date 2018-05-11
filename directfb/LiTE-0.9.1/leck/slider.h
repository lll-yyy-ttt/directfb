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
 * @brief  This file contains definitions for the LiTE slider structure.
 * @ingroup LiTE
 * @file slider.h
 * <hr>
 **/

#ifndef __LITE__SLIDER_H__
#define __LITE__SLIDER_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <lite/box.h>
#include <lite/theme.h>

/* @brief Macro to convert a generic LiteBox into a LiteSlider */
#define LITE_SLIDER(l) ((LiteSlider*)(l))

/* @brief LiteSlider structure */
typedef struct _LiteSlider LiteSlider;

/* @brief Slider theme */
typedef struct _LiteSliderTheme {
     LiteTheme thene;           /**< base LiTE theme */
} LiteSliderTheme; 
                                   
/* @brief Standard slider themes */
/* @brief No slider theme */
#define liteNoSliderTheme       NULL

/* @brief default slider theme */
extern LiteSliderTheme *liteDefaultSliderTheme;


/* @brief Callback function prototype for slider updates 
 * 
 * @param slider               IN:     Valid LiteSlider object
 * @param pos                  IN:     Slider position
 * @param data                 IN:     Data context
 *
 * @return void
*/
typedef void (*SliderUpdateFunc) (LiteSlider *slider, float pos, void *data);

/* @brief Create a new LiteSlider object
 * This function will create a new LiteImage object.
 *
 * @param parent                IN:     Valid parent LiteBox
 * @rect                        IN:     Rectangle coordinates for the slider
 * @theme                       IN:     Slider theme
 * @param ret_slider            OUT:    Returns a valid LiteSlider object
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_new_slider(LiteBox         *parent,
                          DFBRectangle    *rect,
                          LiteSliderTheme *theme,
                          LiteSlider     **ret_slider);


/* @brief Set the slider position
 * This function will set the slider into a position.
 * 
 * @param slider                IN:     Valid LiteSlider object
 * @param pos                   IN:     New slider position
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_set_slider_pos(LiteSlider *slider, float pos);
                        

/* @brief Modify a slider's orientation
 * This function will set the vertical flag of a slider, controlling
 * how it is drawn and how the thumb will drag.
 * 
 * @param slider                IN:     Valid LiteSlider object
 * @param vertical              IN:     true for vertical, 
 *                                      false for horizonal
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_set_slider_vertical(LiteSlider *slider,
                                   bool vertical);

/* @brief Install the slider update callback
 * This function will install a slider update callback. This
 * callback is triggered every time the slider position is changed.
 * 
 * @param slider                IN:     Valid LiteSlider object
 * @param callback              IN:     Valid callback function
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_on_slider_update (LiteSlider      *slider,
                                SliderUpdateFunc  callback,
                                void             *data);


#ifdef __cplusplus
}
#endif

#endif /*  __LITE__SLIDER_H__  */
