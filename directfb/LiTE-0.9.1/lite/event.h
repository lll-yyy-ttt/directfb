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
 * @brief  This file contains definitions for the LiTE event interface.
 * @ingroup LiTE
 * @file event.h
 * <hr>
 **/

#ifndef __LITE__EVENT_H__
#define __LITE__EVENT_H__

#ifdef __cplusplus
extern "C"
{
#endif


#include <directfb.h>

#include "box.h"


/* @brief Get curent key modifiers 
 * This function is used to get the current key modifier values.
 * 
 * @return int                  Returns the currently active key modifier.
 */
int lite_get_current_key_modifier(void);

/* @brief Exit the even loop.
 * Exit the current event loop. You need to restart the event loop
 * in case there are windows waiting for events.
 *
 * @return DFBResult            Returns DFB_OK if successful. 
 */
DFBResult lite_exit_event_loop(void);

/* @brief Get the pointer to the current event buffer
 * This function will return the pointer of the one and only event buffer
 * that is active in LiTE applications.
 *
 * @param evt_buffer            OUT:    Currently active event buffer
 *
 * return DFBResult             Returns DFB_OK if successful.
 */
DFBResult lite_get_main_event_buffer(IDirectFBEventBuffer **evt_buffer);

/* @brief Set the idle loop exit state 
 * This function will enable or disable the state of exiting the event
 * loop if no pending events are available.
 *
 * @param state                 IN:     Sets the state if the event loop will exit
 *                                      if no pending events or not. 
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_exit_idle_loop(bool state);

 
#ifdef __cplusplus
}
#endif

#endif /*  __LITE__WINDOW_H__  */
