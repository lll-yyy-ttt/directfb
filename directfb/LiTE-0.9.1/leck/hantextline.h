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
 * @brief  This file contains definitions for the LiTE hangul textline structure.
 * @ingroup LiTE
 *
 * LiteHanTextLine is a specialized LiteTextLine for Korean input
 *
 * @file hantextline.h
 * <hr>
 **/

#ifndef __LITE__HANTEXTLINE_H__
#define __LITE__HANTEXTLINE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <lite/box.h>
#include <lite/theme.h>

#include <mext_imhangul.h>

/* @brief Macro to convert a generic LiteBox into a LiteHanTextLine */
#define LITE_HANTEXTLINE(l) ((LiteHanTextLine*)(l))

/* @brief LiteHanTextLine structure */
typedef struct _LiteHanTextLine LiteHanTextLine;

/* @brief HanTextline theme */
typedef struct _LiteHanTextLineTheme {
     LiteTheme theme;           /**< base LiTE theme */

     LiteFont *font;            /**< text line font */
     DFBColor *font_color;      /**< text line font color */
     int font_size;             /**< text line font size  */
} LiteHanTextLineTheme;

/* @brief Standard han text line themes */
/* @brief No text line theme */
#define liteNoHanTextLineTheme     NULL

/* @brief default text line theme */
extern LiteHanTextLineTheme *liteDefaultHanTextLineTheme;


/* @brief TextHanLine enter callback prototype 
 * This callback is use for installing a callback to trigger
 * when the enter key is triggered in a focused LiteHanTextLine.
 *
 * @param text                  IN:     Text in the text line
 * @param data                  IN:     Data context
 * 
 * @return void
 */     
typedef void (*HanTextLineEnterFunc) (const char *text, void *data);

/* @brief HanTextLine abort callback prototype 
 * This callback is use for installing a callback to trigger
 * when the Escape key is triggered in a focused LiteHanTextLine.
 *
 * @param data                  IN:     Data context
 * 
 * @return void
 */     
typedef void (*HanTextLineAbortFunc) (void *data);

/* @brief Create a new LiteHanTextLine object
 * This function will create a new LiteHanTextLine object.
 *
 * @param parent                IN:     Valid parent LiteBox
 * @param rect                  IN:     Rectangle for the text line
 * @param theme                 IN:     Text line theme
 * @param ret_textline          OUT:    Valid LiteHanTextLine object
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_new_hantextline(LiteBox           *parent,
                            DFBRectangle         *rect,
                            LiteHanTextLineTheme *theme,
                            LiteHanTextLine     **ret_textline);

/* @brief Set the text field of a han text line.
 * This function will set the text field of a text line.
 * 
 * @param textline              IN:     Valid LiteTextLine object
 * @param text                  IN:     text string
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_set_hantextline_text (LiteHanTextLine *textline, const char *text);

/* @brief Install the hantextline enter callback function.
 * This function will set the hantextline enter callback function
 * that is triggered when the ENTER key is called.
 * 
 * @param textline              OUT:    Valid LiteTextLine object
 * @param callback              IN:     Valid callback
 * @param data                  IN:     Data context.
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_on_hantextline_enter(LiteHanTextLine  *textline,
                                 HanTextLineEnterFunc callback,
                                    void             *data);

/* @brief Install the textline abort callback function.
 * This function will set the textline abort callback function
 * that is triggered when the ESCAPE key is called.
 * 
 * @param textline              OUT:    Valid LiteTextLine object
 * @param callback              IN:     Valid callback
 * @param data                  IN:     Data context.
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_on_hantextline_abort(LiteHanTextLine     *textline,
                                    HanTextLineAbortFunc callback,
                                    void                *data);

#ifdef __cplusplus
}
#endif

#endif /*  __LITE__HANTEXTLINE_H__  */
