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
 * @brief  This file contains definitions for the LiTE label structure.
 * @ingroup: LiTE
 * @file label.h
 * <hr>
 **/

#ifndef __LITE__LABEL_H__
#define __LITE__LABEL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <lite/box.h>
#include <lite/font.h>
#include <lite/theme.h>


/* @brief Macro to convert a generic LiteBox into a LiteLabel */
#define LITE_LABEL(l) ((LiteLabel*)(l))

/* @brief LiteLabel alignment constants */
typedef enum {
  LITE_LABEL_LEFT,              /**< Align left  */
  LITE_LABEL_RIGHT,             /**< Align right */
  LITE_LABEL_CENTER             /**< Align center*/
} LiteLabelAlignment;

/* @brief LiteLabel structure */
typedef struct _LiteLabel LiteLabel;

/* @brief Label theme */
typedef struct _LiteLabelTheme {
     LiteTheme theme;           /**< base LiTE theme */

     LiteFont *font;            /**< label font */
     DFBColor *font_color;      /**< color used for the font */
     int font_size;             /**< font size */
} LiteLabelTheme;

/* @brief Standard label themes */
/* @brief No label theme */
#define liteNoLabelTheme        NULL

/* @brief default label theme */
extern LiteLabelTheme *liteDefaultLabelTheme;
     

/* @brief Create a new LiteLabel object
 * This function will create a new LiteLabel object.
 *
 * @param parent                IN:     Valid parent LiteBox
 * @param rect                  IN:     Rectangle for the label, note that
 *                                      the height is defined by the font and
 *                                      font size used
 * @param theme                 IN:     Label theme
 * @param font_size             IN:     Label font size
 * @param ret_label             OUT:    Valid LiteLabel object
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_new_label(LiteBox         *parent,
                         DFBRectangle    *rect,
                         LiteLabelTheme  *theme,
                         int              font_size,
                         LiteLabel      **ret_label);

/* @brief Set label text string 
 * This function will set the LiteLabel object text string.
 *
 * @param label                 IN:     Valid LiteLabel object
 * @param text                  IN:     Text string
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_set_label_text(LiteLabel *label, const char *text);

/* @brief Set label text alignment 
 * This function will set the LiteLabel text alignment based 
 * on the LiteLabelAlignment constants.
 *
 * @param label                 IN:     Valid LiteLabel object
 * @param alignment             IN:     Alignment flags
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_set_label_alignment(LiteLabel *label, LiteLabelAlignment aligment);

/* @brief Set label font
 * This function will set the LiteLabel font.
 *
 * @param label                 IN:     Valid LiteLabel object
 * @param spec                  IN:     Font specification 
 * @param style                 IN:     Font style
 * @param size                  IN:     Font size
 * @param attr                  IN:     Font attributes
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_set_label_font(LiteLabel        *label,
                              const char       *spec,
                              LiteFontStyle     style,
                              int               size,
                              DFBFontAttributes attr);

/* @brief Set label text color
 * This function will set the LiteLabel text color.
 *
 * @param label                 IN:     Valid LiteLabel object
 * @param color                 IN:     Color value
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_set_label_color(LiteLabel *label, DFBColor *color);


#ifdef __cplusplus
}
#endif

#endif /*  __LITE__LABEL_H__  */
