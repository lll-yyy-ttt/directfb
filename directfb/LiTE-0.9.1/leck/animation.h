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

/*
 * @brief  This file contains definitions for the LiTE animation structure.
 * @ingroup LiTE
 * @file animation.h
 * <hr>
 */

#ifndef __LITE__ANIMATION_H__
#define __LITE__ANIMATION_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <lite/box.h>
#include <lite/theme.h>

/* @brief Macro to convert a generic LiteBox to a LiteAnimation structure */
#define LITE_ANIMATION(l) ((LiteAnimation*)(l))

/* @brief LiteAnimation structure that is derived from a LiteBox */
typedef struct _LiteAnimation LiteAnimation;

/* @brief LiteAnimation theme */
typedef struct _LiteAnimationTheme {
     LiteTheme theme;                   /**< base LiTE theme */
} LiteAnimationTheme;

/* @brief Standard animation themes */
/* @brief No animation theme */
#define liteNoAnimationTheme    NULL

/* @brief default animation theme */
extern LiteAnimationTheme *liteDefaultAnimationTheme;

/* @brief Create a new LiteAnimation object 
 * This  function creates a new LiteAnimation object that could be 
 * used for creating animated graphics entities.
 * 
 * @param parent                IN:     Valid parent LiteBox
 * @param rect                  IN:     Rectangle for the animation object
 * @param theme                 IN:     Animation theme
 * @param ret_animation         OUT:    Valid LiteAnimation object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_new_animation(LiteBox            *parent,
                             DFBRectangle       *rect,
                             LiteAnimationTheme *theme,
                             LiteAnimation     **ret_animation);

/* @brief Load an animation image
 * This function will load an image into the animation object.
 * It is assumed that the LiteAnimation object is already initialized.
 *
 * @param animation             IN:     Valid LiteAnimation object
 * @param filename              IN:     Filename where the animation image resides
 * @param still_frame           IN:     Index of the animation frame
 * @param frame_width           IN:     Frame width of the animation frame
 * @param frame_height          IN:     Frame heigth of the animation frame
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_load_animation(LiteAnimation *animation,
                              const char    *filename,
                              int            still_frame,
                              int            frame_width,
                              int            frame_height);

/* @brief Start the animation 
 * This function will start the LiteAnimation object animation
   sequence with a specified timeout between frames
 *
 * @param animation             IN:     Valid LiteAnimation object
 * @param timeout               IN:     Timeout in milliseconds
 *
 * @return DFBResult            Return DFB_OK if successful.
 */
DFBResult lite_start_animation(LiteAnimation *animation, unsigned int timeout);

/* @brief Update the animation sequence
 * This function will update the animation sequence.
 *
 * @param animation             IN:     Valid LiteAnimation object
 *
 * @return int                  Return 1 if animation was done, 0 otherwise
 */
int  lite_update_animation(LiteAnimation *animation);

/* @brief Stop the animation sequence
 * This function will stop the animation sequence.
 *
 * @param animation             IN:     Valid LiteAnimation object
 *
 * @return DFBResult            Returns DFB_OK if succcessful.
 */
DFBResult lite_stop_animation(LiteAnimation *animation);

/* @brief Checks of the animation is running or not
 * This function will test if the animation sequence is running or not
 *
 * @param animation             IN:     Valid LiteAnimation object
 *
 * @return int                  Return 1 if animation is running, 0 otherwise.
 */
int lite_animation_running(LiteAnimation *animation);

                        
#ifdef __cplusplus
}
#endif

#endif /*  __LITE__ANIMATION_H__  */
