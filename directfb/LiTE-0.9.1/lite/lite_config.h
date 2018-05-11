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
 * @brief  This file contains definitions for LiTE configuration.
 * @ingroup LiTE
 * @file lite_config.h
 * <hr>
 **/

#ifndef __LITE__CONFIG_H__
#define __LITE__CONFIG_H__

#ifdef __cplusplus
extern "C"
{
#endif


/* @brief Set if we want to install the default window theme or not */
#define LOAD_DEFAULT_WINDOW_THEME               true

/* @brief Default window theme title font */
#define DEFAULT_WINDOW_TITLE_FONT               "whiterabbit"

/* @brief Default window theme left frame pattern */
#define DEFAULT_WINDOW_LEFT_FRAME               "links.png"

/* @brief Default window theme right frame pattern */
#define DEFAULT_WINDOW_RIGHT_FRAME              "rechts.png"

/* @brief Default window theme top frame pattern */
#define DEFAULT_WINDOW_TOP_FRAME                "oben.png"

/* @brief Default window theme bottom frame pattern */
#define DEFAULT_WINDOW_BOTTOM_FRAME             "unten.png"

/* @brief Default window theme top left frame pattern */
#define DEFAULT_WINDOW_TOP_LEFT_FRAME           "obenlinks.png"

/* @brief Default window theme top right frame pattern */
#define DEFAULT_WINDOW_TOP_RIGHT_FRAME          "obenrechts.png"

/* @brief Default window theme bottom left frame pattern */
#define DEFAULT_WINDOW_BOTTOM_LEFT_FRAME        "untenlinks.png"

/* @brief Default winidow theme bottom right frame pattern */
#define DEFAULT_WINDOW_BOTTOM_RIGHT_FRAME       "untenrechts.png"

/* @brief Default window background color R component */
#define DEFAULT_WINDOW_COLOR_R                  0xee

/* @brief Default window background color G component */
#define DEFAULT_WINDOW_COLOR_G                  0xee

/* @brief Default window background color B component */
#define DEFAULT_WINDOW_COLOR_B                  0xee

/* @brief Default window background alpha component */
#define DEFAULT_WINDOW_COLOR_A                  0xff

/* @brief Default minimum window size in minimize operation */
#define DEFAULT_WINDOW_MINIMIZE_SIZE            40

/* @brief Default window cursor */
#define DEFAULT_WINDOW_CURSOR                   "cursor.png"
#define DEFAULT_WINDOW_CURSOR_HOTSPOT_X         0
#define DEFAULT_WINDOW_CURSOR_HOTSPOT_Y         0

/* @brief Default System font */
#define DEFAULT_FONT_SYSTEM                     "vera"

/* @brief Default Monospaced font */
#define DEFAULT_FONT_MONOSPACED                 "courier"

/* @brief Default Serif font */
#define DEFAULT_FONT_SERIF                      "times"

/* @brief Default Proportional font */
#define DEFAULT_FONT_PROPORTIONAL               "vera"

/* @brieef Default Sans Serif font */
#define DEFAULT_FONT_SANS_SERIF                 "vera" 

/* @brief Default Hangul(Korean) font */
#define DEFAULT_FONT_SYSTEM_HANGUL              "gulim"

#ifdef __cplusplus
}
#endif

#endif /* __LITE__CONFIG_H__ */
