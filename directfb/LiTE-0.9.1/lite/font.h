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
 * @brief  This file contains definitions for the LiTE font interface.
 * @ingroup LiTE
 * @file font.h
 * <hr>
 **/
#ifndef __LITE__FONT_H__
#define __LITE__FONT_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <directfb.h>

#include "box.h"

/* @brief Macro to convert a derived data structure into a generic LiteFont */
#define LITE_FONT(l) ((LiteFont*)(l))

/* @brief Font styles */
typedef enum {
  LITE_FONT_PLAIN  = 0,         /**< Plain font   */
  LITE_FONT_BOLD   = 1,         /**< Bold font    */
  LITE_FONT_ITALIC = 2          /**< Italics font */
} LiteFontStyle;

/* @brief LiteFont structure
 * LiteFont is the generic object for font handling in LiTE.
 */
typedef struct _LiteFont LiteFont;

/* @brief Default Font attribute */
#define DEFAULT_FONT_ATTRIBUTE DFFA_NONE

/* @brief Get font based on specification 
 * Create a font object based on specifications such as style, size, and so
 * forth.
 * 
 * @param spec                  IN:     Font specification
 * @param style                 IN:     Font style
 * @param size                  IN:     Font size
 * @param attr                  IN:     Font attributes
 * @param ret_font              OUT:    Font object
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_get_font(const char       *spec, 
                        LiteFontStyle     style, 
                        int               size, 
                        DFBFontAttributes attr, 
                        LiteFont        **ret_font);

/* @brief Get font from file
 * This function will return a font resource object from a font resource
   in a file. The font file path should be an absolute path.
 *
 * @param file                  IN:     File where the font resource resides.
 * @param attr                  IN:     Font attributes
 * @param ret_font              OUT:    Font object
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_get_font_from_file(const char       *file, 
                                  int               size, 
                                  DFBFontAttributes attr, 
                                  LiteFont        **ret_font);

/* @brief Increase the reference count on a font object
 * This function increases the reference count on a font object. 
 * This is used to indicate that this font is used in another context
 * and should not be purged.
 *
 * @param font                  IN:     Valid LiteFont object
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_ref_font(LiteFont *font);

/* @brief Release the font object
 * This function decreases the font reference count so the object 
 * eventually could be purged.
 *
 * @param font                  IN:     Valid LiteFont object
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_release_font(LiteFont *font);

/* @brief Get the underying IDirectFB font object
 * This function returns the underlying IDirectFB font object
 * from the LiteFont object.
 *
 * @param font                  IN:     Valid LiteFont object
 * @param ret_dfb_font          OUT:    IDirectFB object
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_font(LiteFont *font, IDirectFBFont **ret_dfb_font);

/* @brief Set the active font
 * This function will set the current active font in a LiteBox object.
 *
 * @param box                   IN:     Valid LiteBox object
 * @param font                  IN:     Valid LiteFont object
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_set_active_font(LiteBox *box, LiteFont *font);

/* @brief Get current font from LiteBox
 * This function will get the currently set font in the LiteBox object.
 *
 * @param box                   IN:     Valid LiteBox object
 * @param font                  IN:     Valid LiteFont object
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_get_active_font(LiteBox *box, LiteFont **font);

/* @brief Get the filename of the font
  * This function returns the full path and filename
  * from the LiteFont object.  The string is valid
  * as long as the font is allocated.
  *
  * @param font                  IN:     Valid LiteFont object
  * @param file                 OUT:     pointer to a string
  *
  * @return DFBResult            Returns DFB_OK if successful.     
  */
DFBResult lite_get_font_filename(LiteFont *font, const char **file);
#ifdef __cplusplus
}
#endif


#endif
