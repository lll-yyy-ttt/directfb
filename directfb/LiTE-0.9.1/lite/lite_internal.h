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


#ifndef __LITE__LITE_INTERNAL_H__
#define __LITE__LITE_INTERNAL_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <directfb.h>

#include <direct/types.h>
#include <direct/debug.h>

#include "box.h"

/* pointer to the main IDirectFB interface */
extern IDirectFB *lite_dfb;

/* pointer to the main DirectFB display layer */
extern IDirectFBDisplayLayer *lite_layer;

/* load the default window theme in lite_open */
DFBResult prvlite_load_default_window_theme(void);

/* initialize the font system */
void  prvlite_font_init(void);

/* set the current key modifier flag */
void prvlite_set_current_key_modifier(int modifier);

/* set the event loop alive flag */
void prvlite_set_event_loop_alive(bool state);

/* get the event loop alive flag */
bool prvlite_get_event_loop_alive(void);

/* wake up event loop without changing alive flag */
DFBResult prvlite_wakeup_event_loop(void);

/* create the main event loop */
DFBResult prvlite_create_event_buffer(IDirectFB* idfb);

/* release the main event loop */
DFBResult prvlite_release_event_buffer(void);

/* attach to internal event loop */
DFBResult prvlite_attach_to_event_buffer(IDirectFBWindow *window);

/* cleanup resources allocated for window use on app shutdown */
DFBResult prvlite_release_window_resources(void);

#ifdef __cplusplus
}
#endif

#endif /*  __LITE__LITE_INTERNAL_H__  */
