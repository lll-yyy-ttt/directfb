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
 * @brief  This file contains definitions for the LiTE textline structure.
 * @ingroup LiTE
 * @file font.h
 * <hr>
 **/

#ifndef __LITE__TEXTLINE_H__
#define __LITE__TEXTLINE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <lite/box.h>
#include <lite/theme.h>

/* @brief Macro to convert a generic LiteBox into a LiteTextLine */
#define LITE_TEXTLINE(l) ((LiteTextLine*)(l))

/* @brief LiteTextLine structure */
typedef struct _LiteTextLine LiteTextLine;

/* @brief Textline theme */
typedef struct _LiteTextLineTheme {
     LiteTheme theme;           /**< base LiTE theme */

     LiteFont *font;            /**< text line font */
     DFBColor *font_color;      /**< text line font color */
     int font_size;             /**< text line font size  */
} LiteTextLineTheme;

/* @brief Standard text line themes */
/* @brief No text line theme */
#define liteNoTextLineTheme     NULL

/* @brief default text line theme */
extern LiteTextLineTheme *liteDefaultTextLineTheme;


/* @brief TextLine enter callback prototype 
 * This callback is use for installing a callback to trigger
 * when the enter key is triggered in a focused LiteTextLine.
 *
 * @param text                  IN:     Text in the text line
 * @param data                  IN:     Data context
 * 
 * @return void
 */     
typedef void (*TextLineEnterFunc) (const char *text, void *data);

/* @brief TextLine abort callback prototype 
 * This callback is use for installing a callback to trigger
 * when the Escape key is triggered in a focused LiteTextLine.
 *
 * @param data                  IN:     Data context
 * 
 * @return void
 */     
typedef void (*TextLineAbortFunc) (void *data);

/* @brief Create a new LiteTextLine object
 * This function will create a new LiteTextLine object.
 *
 * @param parent                IN:     Valid parent LiteBox
 * @param rect                  IN:     Rectangle for the text line
 * @param theme                 IN:     Text line theme
 * @param ret_textline          OUT:    Valid LiteTextLine object
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_new_textline(LiteBox           *parent,
                            DFBRectangle      *rect,
                            LiteTextLineTheme *theme,
                            LiteTextLine     **ret_textline);

/* @brief Set the text field of a text line.
 * This function will set the text field of a text line.
 * 
 * @param textline              IN:     Valid LiteTextLine object
 * @param text                  IN:     text string
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_set_textline_text (LiteTextLine *textline, const char *text);
DFBResult lite_get_textline_text (LiteTextLine *textline, char **ret_text);

/* @brief Install the textline enter callback function.
 * This function will set the textline enter callback function
 * that is triggered when the ENTER key is called.
 * 
 * @param textline              OUT:    Valid LiteTextLine object
 * @param callback              IN:     Valid callback
 * @param data                  IN:     Data context.
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_on_textline_enter(LiteTextLine     *textline,
                                 TextLineEnterFunc callback,
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
DFBResult lite_on_textline_abort(LiteTextLine     *textline,
                                 TextLineAbortFunc callback,
                                 void             *data);

#ifdef __cplusplus
}
#endif

#endif /*  __LITE__TEXTLINE_H__  */
