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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <directfb.h>


#include "lite_internal.h"
#include "lite_config.h"
#include "window.h"


D_DEBUG_DOMAIN(LiteEventDomain, "LiTE/Event", "LiteEvent");

static int key_modifier_global = 0;
static IDirectFBEventBuffer *main_event_buffer_global = NULL;
static bool event_loop_alive_global = true;

/* public APIs */
DFBResult
lite_get_main_event_buffer(IDirectFBEventBuffer **evt_buffer)
{
    *evt_buffer = main_event_buffer_global;
    
    return DFB_OK;
}

int lite_get_current_key_modifier(void)
{
     return key_modifier_global;
}

DFBResult lite_exit_event_loop(void)
{
     DFBResult res;
     D_DEBUG_AT(LiteEventDomain, "Exiting the event loop...\n");

     prvlite_set_event_loop_alive(false);
     res = prvlite_wakeup_event_loop();

     return res;
}

DFBResult
lite_set_exit_idle_loop(bool state)
{
     static int idle_id = 0;

     if (state) {
          if (idle_id == 0)
               lite_enqueue_idle_callback(NULL, NULL, &idle_id);
     }
     else {
          if (idle_id != 0)
          {
               lite_remove_idle_callback(idle_id);
               idle_id = 0;
          }
     }

     return DFB_OK;
}     

/* Private APIs */
void prvlite_set_current_key_modifier(int modifier)
{
     key_modifier_global = modifier;
}


DFBResult prvlite_create_event_buffer(IDirectFB* idfb)
{
     DFBResult res = DFB_OK;
     
     D_ASSERT(idfb != NULL);

     res = idfb->CreateEventBuffer(idfb, &main_event_buffer_global);
     if (res == DFB_OK) {
          D_DEBUG_AT(LiteEventDomain, 
                         "Created main event buffer, res: %d\n", res);
     }
     else {
          D_DEBUG_AT(LiteEventDomain, 
                         "Failed creating main event buffer, err: %d\n", res);
     }

     return res;
}

DFBResult
prvlite_release_event_buffer(void)
{
     DFBResult res = DFB_OK;
     
     if (main_event_buffer_global) {
          res = main_event_buffer_global->Release(main_event_buffer_global);
          D_DEBUG_AT(LiteEventDomain, 
                         "Releasing main event buffer, res: %d\n", res);
     }

     return res;
}

DFBResult
prvlite_attach_to_event_buffer(IDirectFBWindow *window)
{
    DFBResult res = DFB_OK;

    D_ASSERT(window != NULL);

    res = window->AttachEventBuffer(window, main_event_buffer_global);
    if(res == DFB_OK) {
        D_DEBUG_AT(LiteEventDomain, 
                    "Attached window: %p of event buffer.\n", window);
    }
    else {
        D_DEBUG_AT(LiteEventDomain, 
                    "Failed to attach window: %p to event buffer, err: %d\n", window, res);
    }

    return res;
}

void
prvlite_set_event_loop_alive(bool state)
{
     event_loop_alive_global = state;
}

bool
prvlite_get_event_loop_alive(void)
{
    return event_loop_alive_global;
}

DFBResult
prvlite_wakeup_event_loop(void)
{
     IDirectFBEventBuffer *event_buffer = NULL;
     DFBResult res = lite_get_event_buffer(&event_buffer);
     if(res == DFB_OK && event_buffer != NULL) {
          res = event_buffer->WakeUp(event_buffer);
          if (res == DFB_INTERRUPTED)
               res = DFB_OK;
     }
     return res;
}
