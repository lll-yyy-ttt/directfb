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
#include <limits.h>

#include <pthread.h>
#include <directfb.h>
#include <directfb_util.h>
#include <directfb_version.h>

#include <direct/clock.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/util.h>

#include "lite_internal.h"
#include "lite_config.h"

#include "util.h"
#include "window.h"
#include "cursor.h"
#include "event.h"

#define DFB_USER_EVENT(e)       ((DFBUserEvent*) (e))
#define DFB_UNIVERSAL_EVENT(e)  ((DFBUniversalEvent*) (e))
#define DFB_WINDOW_EVENT(e)     ((DFBWindowEvent*)(e))

D_DEBUG_DOMAIN(LiteWindowDomain, "LiTE/Window", "LiteWindow");
D_DEBUG_DOMAIN(LiteUpdateDomain, "LiTE/Updates", "LiteWindow Updates");
D_DEBUG_DOMAIN(LiteMotionDomain, "LiTE/Motion", "LiteWindow Motion");

LiteWindowTheme *liteDefaultWindowTheme = NULL;

static LiteWindow               **window_array_global   = NULL;
static LiteWindow                *modal_window_global = NULL;
static LiteWindow                *entered_window_global = NULL;
static int                        num_windows_global = 0;
static long long                  last_update_time = 0; /* milliseconds, based on direct_clock_get_millis */
static long long                  minimum_update_freq = 200; /* in milliseconds */
static IDirectFBEventBuffer      *event_buffer_global = NULL;

/* holds ref to window that's grabbed mouse or keyboard */
static IDirectFBWindow           *win_pointergrabbed_global = NULL;

static pthread_mutex_t timeout_mutex = PTHREAD_MUTEX_INITIALIZER;
static LiteWindowTimeout *timeout_queue = NULL;
static int timeout_next_id = 1;

static pthread_mutex_t idle_mutex = PTHREAD_MUTEX_INITIALIZER;
static LiteWindowIdle *idle_queue = NULL;
static int idle_next_id = 1;

static int event_loop_nesting_level = 0;

static void add_window(LiteWindow *window);
static void remove_window(LiteWindow *window);
static void destroy_window_data(LiteWindow *window);

static LiteWindow *find_window_by_id(DFBWindowID id);

static void flush_resize(LiteWindow *window);
static void flush_motion(LiteWindow *window);

static int handle_move(LiteWindow *window, DFBWindowEvent *ev);
static int handle_resize(LiteWindow *window, DFBWindowEvent *ev);
static int handle_close(LiteWindow *window);
static int handle_destroy(LiteWindow *window);
static int handle_enter(LiteWindow *window, DFBWindowEvent *ev);
static int handle_leave(LiteWindow *window, DFBWindowEvent *ev);
static int handle_lost_focus(LiteWindow *window);
static int handle_got_focus(LiteWindow *window);
static int handle_motion(LiteWindow *window, DFBWindowEvent *ev);
static int handle_button(LiteWindow *window, DFBWindowEvent *ev);
static int handle_key_up(LiteWindow *window, DFBWindowEvent *ev);
static int handle_key_down(LiteWindow *window, DFBWindowEvent *ev);
static int handle_wheel(LiteWindow *window, DFBWindowEvent *ev);
static int focus_traverse(LiteWindow *window);
static LiteBox *find_child(LiteBox *box, int *x, int *y);
static void child_coords(LiteBox *box, int *x, int *y);

static void validate_entered_box(LiteWindow *window);
static void render_title(LiteWindow *window);
static void render_border(LiteWindow *window);

static DFBResult draw_window(LiteBox *box, const DFBRegion *region, DFBBoolean clear);
static DFBResult create_event_buffer(IDirectFBWindow *window);

static DFBResult run_window_event_loop(LiteWindow *window, bool once);

/* Public API implementations */

DFBResult
lite_new_window(IDirectFBDisplayLayer *layer,
                DFBRectangle          *rect,
                DFBWindowCapabilities  caps,
                LiteWindowTheme       *theme,
                const char            *title,
                LiteWindow           **ret_window)
{
     LITE_NULL_PARAMETER_CHECK(rect);
     LITE_NULL_PARAMETER_CHECK(ret_window);
     
     DFBResult ret = DFB_OK;

     D_DEBUG_ENTER( LiteWindowDomain, "( %p, %p, 0x%x, %p, '%s' )", layer, rect, caps, theme, title );

     *ret_window = NULL;

     /* allocate window struct */
     LiteWindow* window = D_CALLOC(1, sizeof(LiteWindow));

     /* call the actual init function that will initialize various structs */
     ret = lite_init_window(window, layer, rect, caps, theme, title);

     if (ret != DFB_OK) {
          D_FREE(window);
          window = NULL;
     }

     *ret_window = window;

     D_DEBUG_EXIT( LiteWindowDomain, "() -> %p", window );

     return ret;
}

DFBResult
lite_init_window(LiteWindow            *win,
                 IDirectFBDisplayLayer *layer,
                 DFBRectangle          *win_rect,
                 DFBWindowCapabilities  caps,
                 LiteWindowTheme       *theme,
                 const char            *title)
{
     LITE_NULL_PARAMETER_CHECK(win);
     LITE_NULL_PARAMETER_CHECK(win_rect);

     DFBResult              ret = DFB_OK;
     DFBRectangle           rect;
     DFBWindowDescription   dsc;
     DFBWindowOptions       options;
     DFBDisplayLayerConfig  dlc;
     IDirectFBWindow       *window = NULL;
     IDirectFBSurface      *surface = NULL;
     IDirectFBSurface      *subsurface = NULL;
     DFBWindowID            id;
     LiteCursor            *curr_cursor = NULL;
     LiteThemeFrame        *frame = NULL;

     D_DEBUG_ENTER( LiteWindowDomain, "( %p, %p, 0x%x, %p, '%s' )", layer, win_rect, caps, theme, title );

     D_DEBUG_AT( LiteWindowDomain, "  -> %d, %d - %dx%d\n", win_rect->x, win_rect->y, win_rect->w, win_rect->h );

     if (win_rect->w <= 0 || win_rect->h <= 0)
          return DFB_INVAREA;

     win->theme = theme;

     if (!layer)
          layer = lite_layer;

     layer->GetConfiguration(layer, &dlc);

     lite_get_current_cursor(&curr_cursor);

     /* create window */
     dsc.flags  = DWDESC_POSX | DWDESC_POSY |
                  DWDESC_WIDTH | DWDESC_HEIGHT | DWDESC_CAPS;
     dsc.width  = win_rect->w;
     dsc.height = win_rect->h;
     dsc.caps = getenv("LITE_WINDOW_DOUBLEBUFFER") ? caps | DWCAPS_DOUBLEBUFFER : caps;

     /*if (caps & DWCAPS_ALPHACHANNEL) {
          dsc.flags       |= DWDESC_SURFACE_CAPS;
          dsc.surface_caps = DSCAPS_PREMULTIPLIED;
     }*/

     if (theme != liteNoWindowTheme && theme->theme.flags & LITE_THEME_HAS_FRAME) {
          frame = &theme->theme.frame;
          D_MAGIC_ASSERT( frame, LiteThemeFrame );
     }

     if (frame) {
          dsc.width  += frame->parts[LITE_THEME_FRAME_PART_LEFT].rect.w + frame->parts[LITE_THEME_FRAME_PART_RIGHT].rect.w;
          dsc.height += frame->parts[LITE_THEME_FRAME_PART_TOP].rect.h  + frame->parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h;
          dsc.caps   |= DWCAPS_NODECORATION;
     }

     /* if specific center flags were passed in, calculate the x/y values */
     if(LITE_CENTER_HORIZONTALLY == win_rect->x)
          dsc.posx = ((int)dlc.width - (int)dsc.width) / 2;
     else {
          dsc.posx = win_rect->x;

          if (frame)
               dsc.posx -= frame->parts[LITE_THEME_FRAME_PART_LEFT].rect.w;
     }

     if(LITE_CENTER_VERTICALLY == win_rect->y)
          dsc.posy = ((int)dlc.height - (int)dsc.height) / 2;
     else {
          dsc.posy = win_rect->y;

          if (frame)
               dsc.posy -= frame->parts[LITE_THEME_FRAME_PART_TOP].rect.h;
     }

     if (title)
     {
          dsc.name[0] = '\0';
          int l = strlen(title);
          int n = l > (DFB_WINDOW_NAME_MAX_LENGTH-1) ? (DFB_WINDOW_NAME_MAX_LENGTH-1) : l;
          strncpy(dsc.name, title, n);
          dsc.name[n] = '\0';

          dsc.flags |= DWDESC_NAME;
     }

     /* DFB window creation */
     ret = layer->CreateWindow(layer, &dsc, &window);
     if (ret) {
          DirectFBError("window_new: CreateWindow", ret);
          D_DEBUG_EXIT( LiteWindowDomain, "()" );
          return ret;
     }

     /* get ID */
     window->GetID(window, &id);

     /* set lite's cursor shape */
     if (curr_cursor && curr_cursor->surface)
          window->SetCursorShape(window, curr_cursor->surface, curr_cursor->hot_x, curr_cursor->hot_y);

     /* get surface */
     ret = window->GetSurface(window, &surface);
     if (ret) {
          DirectFBError("window_new: Window->GetSurface", ret);
          window->Release(window);
          D_DEBUG_EXIT( LiteWindowDomain, "()" );
          return ret;
     }

     /* get sub surface */
     if (frame) {
          rect.x = frame->parts[LITE_THEME_FRAME_PART_LEFT].rect.w;
          rect.y = frame->parts[LITE_THEME_FRAME_PART_TOP].rect.h;
     }
     else {
          rect.x = 0;
          rect.y = 0;
     }

     rect.w = win_rect->w;
     rect.h = win_rect->h;

     ret = surface->GetSubSurface(surface, &rect, &subsurface);
     if (ret) {
          DirectFBError("window_new: Surface->GetSubSurface", ret);
          surface->Release(surface);
          window->Release(window);
          D_DEBUG_EXIT( LiteWindowDomain, "()" );
          return ret;
     }

     /* set opaque content region */
     window->SetOpaqueRegion(window, rect.x, rect.y,
                             rect.x + rect.w - 1, rect.y + rect.h - 1);

     /* honor window shape in input handling */
     window->GetOptions(window, &options);
//     window->SetOptions(window, options | DWOP_SHAPED);
     window->SetOptions(window, options | DWOP_OPAQUE_REGION);

     win->box.type    = LITE_TYPE_WINDOW;
     win->box.rect    = rect;
     win->box.is_visible = 1;
     win->box.is_active = 1;
     win->box.surface = subsurface;
     win->box.catches_all_events = 0;

     win->box.Draw    = draw_window;
     win->box.Destroy = lite_destroy_box_contents;

     /* one initial ref that will be removed on receiving DFB_DESTROYED  */
     win->internal_ref_count = 1;

     win->id          = id;
     win->width       = dsc.width;
     win->height      = dsc.height;
     win->window      = window;
     win->surface     = surface;
     win->creator     = NULL;

     win->bg.enabled  = DFB_TRUE;

     /* pick up the backgrouund colors, either from theme or using default ones */
     if(theme == liteNoWindowTheme) {
          win->bg.color.a  = DEFAULT_WINDOW_COLOR_A;
          win->bg.color.r  = DEFAULT_WINDOW_COLOR_R;
          win->bg.color.g  = DEFAULT_WINDOW_COLOR_G;
          win->bg.color.b  = DEFAULT_WINDOW_COLOR_B;
     }
     else {
          win->bg.color.a  = LITE_THEME(theme)->bg_color.a;
          win->bg.color.r  = LITE_THEME(theme)->bg_color.r;
          win->bg.color.g  = LITE_THEME(theme)->bg_color.g;
          win->bg.color.b  = LITE_THEME(theme)->bg_color.b;
     }

     /* give the child box focus to ourself */
     win->focused_box = &win->box;

     /* set window title */
     if (title)
          win->title = D_STRDUP (title);

     /* init update mutex */
     direct_util_recursive_pthread_mutex_init( &win->updates.lock );

     /* render title bar and borders */
     if (frame) {
          DFBDimension size = { win->width, win->height };

          lite_theme_frame_target_update( &win->frame_target, frame, &size );

          render_title(win);
          render_border(win);
     }

     /* post initial update */
     lite_update_box(LITE_BOX(win), NULL);

     /* add our window to the global linked list of windows */
     add_window(win);

     /* create or attach the event buffer */
     ret = create_event_buffer(window);
     if (ret != DFB_OK) {
          D_DEBUG_EXIT( LiteWindowDomain, "()" );
          return ret;
     }

     /* set the window flags */
     win->flags = LITE_WINDOW_RESIZE | LITE_WINDOW_MINIMIZE ;

     D_DEBUG_EXIT( LiteWindowDomain, "()" );

     return DFB_OK;
}

DFBResult
lite_window_set_creator(LiteWindow *window, LiteWindow *creator)
{
    LITE_NULL_PARAMETER_CHECK(window);
    LITE_WINDOW_PARAMETER_CHECK(window);

    window->creator = creator;
    return DFB_OK;
}

DFBResult
lite_window_get_creator(LiteWindow *window, LiteWindow **creator)
{
     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     *creator = window->creator;

     return DFB_OK;
}

static void
release_grabs(void)
{
     if (win_pointergrabbed_global) {
          win_pointergrabbed_global->UngrabPointer (win_pointergrabbed_global);
          win_pointergrabbed_global->UngrabKeyboard (win_pointergrabbed_global);
          win_pointergrabbed_global = NULL;
          D_DEBUG_AT( LiteWindowDomain, "release_grabs: win_pointergrabbed_global = NULL\n");
     }
}

DFBResult
lite_release_window_drag_box(LiteWindow *window)
{
     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     if (window->drag_box != NULL) {
          if (!(window->flags & LITE_WINDOW_MODAL)) {
               release_grabs();
          }
          window->drag_box = NULL;
     }

     return DFB_OK;
}

DFBResult
lite_window_set_modal(LiteWindow *window, bool modal)
{
     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     if (modal) {
          if (modal_window_global) {
               modal_window_global->window->UngrabPointer (modal_window_global->window);
               modal_window_global->window->UngrabKeyboard (modal_window_global->window);
          }
          release_grabs();
          modal_window_global = window;
          window->flags |= LITE_WINDOW_MODAL;
          window->window->GrabKeyboard (window->window);
          window->window->GrabPointer (window->window);
          win_pointergrabbed_global = window->window;
          D_DEBUG_AT( LiteWindowDomain, "lite_window_set_modal: win_pointergrabbed_global = %p (from %p)\n", 
                      window->window, window );

          /* if the window was created duing a mouse down event
           * adjust the drag_box on both windows.
           **/
          {
               int n;
               int exists = 0;
               for (n=0; n<num_windows_global; n++)
               {
                    if (window_array_global[n]->drag_box)
                    {
                         window_array_global[n]->drag_box = NULL;
                         exists = 1;
                    }
               }
               if (exists)
               {
                    int cx,cy;
                    int wx,wy;
                    int dx,dy;

                    lite_layer->GetCursorPosition (lite_layer, &cx, &cy);
                    window->window->GetPosition (window->window, &wx, &wy);

                    dx = cx - wx;
                    dy = cy - wy;
                    window->drag_box = find_child (LITE_BOX(window), &dx, &dy);
               }
          }
     }
     else {
          window->flags &= ~LITE_WINDOW_MODAL;

          if (modal_window_global == window) {
               modal_window_global = NULL;
               release_grabs();
               if (window->creator && (window->creator->flags & LITE_WINDOW_MODAL))
               {
                    modal_window_global = window->creator;
                    window->creator->window->GrabKeyboard (window->creator->window);
                    window->creator->window->GrabPointer (window->creator->window);
                    win_pointergrabbed_global = window->creator->window;
                    D_DEBUG_AT( LiteWindowDomain, 
                                "lite_window_set_modal: win_pointergrabbed_global = %p (creator), from %p\n", 
                                window->creator->window, window->creator );
               }
               else {
                    /* find the last modal window and restore its modality */
                    int n;
                    for (n=0; n<num_windows_global; n++)
                         if (window_array_global[n] == window)
                              break;

                    for (; n > 0; n--) {
                         if (window_array_global[n - 1]->flags & LITE_WINDOW_MODAL) {
                              lite_window_set_modal (window_array_global[n - 1], true);
                              break;
                         }
                    }
               }
          }
     }

     return DFB_OK;
}

static void
draw_updated_windows(void)
{
     int         i, n;

     D_DEBUG_AT( LiteUpdateDomain, "%s()\n", __FUNCTION__ );

     /* go through each window and call update pending */
     for (i = 0; i < num_windows_global; i++) {
          LiteWindow* win = window_array_global[i];
          int         pending;

          if (win->flags & LITE_WINDOW_DESTROYED)
               continue;

          pthread_mutex_lock( &win->updates.lock );

          pending = win->updates.pending;
          if (pending) {
               D_DEBUG_AT( LiteUpdateDomain, "  -> updating window %d (%p)\n", win->id, win );

               for (n=0; n<pending && win->updates.pending; n++) {
                    DFBRegion region = win->updates.regions[0];

                    if (--win->updates.pending)
                        direct_memmove( &win->updates.regions[0], &win->updates.regions[1],
                                        sizeof(DFBRegion) * win->updates.pending );

                    D_DEBUG_AT( LiteUpdateDomain, "  -> %d, %d - %dx%d (%d left)\n",
                                DFB_RECTANGLE_VALS_FROM_REGION( &region ), win->updates.pending );

/*                    fprintf( stderr, "==========================> UPDATE = %4d,%4d - %4dx%4d = <- %d\n",
                             DFB_RECTANGLE_VALS_FROM_REGION( &region ), win->id );
*/
                    lite_draw_box(LITE_BOX(win), &region, DFB_TRUE);
               }

               /* flush opacity change */
               if (! (win->flags & LITE_WINDOW_DRAWN)) {
                    win->flags |= LITE_WINDOW_DRAWN;

                    win->window->SetOpacity( win->window, win->opacity );
               }
          }

          pthread_mutex_unlock( &win->updates.lock );
     }

     last_update_time = direct_clock_get_millis();
}

/* enqueue an item with a NULL callback to always exit from event loop at that time */
DFBResult
lite_enqueue_window_timeout(int timeout, LiteTimeoutFunc callback,
     void *callback_data, int *timeout_id)
{
     LiteWindowTimeout *new_item = D_CALLOC(1, sizeof(LiteWindowTimeout));

     pthread_mutex_lock(&timeout_mutex);

     new_item->timeout = direct_clock_get_millis() + timeout;
     new_item->callback = callback;
     new_item->callback_data = callback_data;
     new_item->id = timeout_next_id++;
     if (timeout_next_id == 0) timeout_next_id = 1;

     /* return item pointer for possible use in removing callback later */
     if (timeout_id)
          *timeout_id = new_item->id;

     D_DEBUG_AT(LiteWindowDomain, "Enqueue timeout of %d milliseconds (trigger time %d.%d) with callback %p(%p)\n",
          timeout, (int)(new_item->timeout / 1000), (int)(new_item->timeout % 1000),
          new_item->callback, new_item->callback_data);

     /* insert after all other items with same or newer timeout */
     LiteWindowTimeout **current = &timeout_queue;
     while (*current && (*current)->timeout <= new_item->timeout)
          current = &(*current)->next;
     new_item->next = *current;
     *current = new_item;

     pthread_mutex_unlock(&timeout_mutex);
     prvlite_wakeup_event_loop();

     return DFB_OK;
}

DFBResult
lite_remove_window_timeout(int timeout_id)
{
     DFBResult result = DFB_INVARG;

     pthread_mutex_lock(&timeout_mutex);

     LiteWindowTimeout **ptr_to_current = &timeout_queue;
     LiteWindowTimeout *current = timeout_queue;

     while (current) {
           if (current->id == timeout_id) {
                 *ptr_to_current = current->next;
                 D_FREE(current);
                 result = DFB_OK;
                 break;
           }

           ptr_to_current = &current->next;
           current = current->next;
     }

     pthread_mutex_unlock(&timeout_mutex);

     return result;
}

DFBResult
lite_rebase_window_timeouts(long long adjustment)
{
     pthread_mutex_lock(&timeout_mutex);
     LiteWindowTimeout *current = timeout_queue;
     while (current) {
          current->timeout += adjustment;
          current = current->next;
     }
     pthread_mutex_unlock(&timeout_mutex);
     prvlite_wakeup_event_loop();
     return DFB_OK;
}


/* return next item in timeout event queue to schedule. Return
 * DFB_FAILURE if no timeout events available.
 */
static DFBResult
remove_next_timeout_callback(LiteTimeoutFunc *callback, void **callback_data)
{
     DFBResult ret = DFB_FAILURE;
     pthread_mutex_lock(&timeout_mutex);

     long long now = direct_clock_get_millis();

     if (timeout_queue && timeout_queue->timeout <= now) {
          LiteWindowTimeout *current = timeout_queue;
          ret = DFB_OK;
          *callback = current->callback;
          *callback_data = current->callback_data;
          timeout_queue = current->next;
          D_FREE(current);
     }

     pthread_mutex_unlock(&timeout_mutex);
     return ret;
}

static DFBResult
get_time_until_next_timeout(long long *remaining)
{
     DFBResult ret = DFB_FAILURE;

     pthread_mutex_lock(&timeout_mutex);

     if (timeout_queue) {
          long long now = direct_clock_get_millis();
          ret = DFB_OK;
          *remaining = timeout_queue->timeout - now;
     }

     pthread_mutex_unlock(&timeout_mutex);
     return ret;
}


/***********************************/

DFBResult
lite_enqueue_idle_callback(LiteTimeoutFunc callback, void *callback_data, int *idle_id)
{
     pthread_mutex_lock(&idle_mutex);

     LiteWindowIdle *new_item = D_CALLOC(1, sizeof(LiteWindowIdle));

     new_item->callback = callback;
     new_item->callback_data = callback_data;
     new_item->id = idle_next_id++;
     if (idle_next_id == 0) idle_next_id = 1;

     /* return item pointer for possible use in removing callback later */
     if (idle_id)
          *idle_id = new_item->id;

     D_DEBUG_AT(LiteWindowDomain, "Enqueue idle (id %d) with callback %p(%p)\n",
                new_item->id, new_item->callback, new_item->callback_data);

     /* insert at end of queue */
     LiteWindowIdle **queue = &idle_queue;
     while (*queue)
          queue = &(*queue)->next;
     *queue = new_item;

     pthread_mutex_unlock(&idle_mutex);

     prvlite_wakeup_event_loop();
     return DFB_OK;
}

DFBResult
lite_remove_idle_callback(int idle_id)
{
     /* returned if the ID isn't found */
     DFBResult result = DFB_INVARG;

     pthread_mutex_lock(&idle_mutex);

     LiteWindowIdle **queue = &idle_queue;
     while (*queue) {
          if ((*queue)->id == idle_id)
          {
               LiteWindowIdle *node = *queue;
               *queue = node->next;
               D_FREE(node);
               result = DFB_OK;
               break;
          }
          queue = &(*queue)->next;
     }

     pthread_mutex_unlock(&idle_mutex);

     prvlite_wakeup_event_loop();
     return result;
}

/* returns DFB_FAILURE if there are no entries in idle queue */
static DFBResult
remove_top_idle_callback(LiteTimeoutFunc *callback, void **callback_data)
{
     DFBResult result = DFB_FAILURE;

     pthread_mutex_lock(&idle_mutex);

     if (idle_queue)
     {
          result = DFB_OK;
          LiteWindowIdle *node = idle_queue;
          idle_queue = node->next;
          *callback = node->callback;
          *callback_data = node->callback_data;
          D_FREE(node);
     }

     pthread_mutex_unlock(&idle_mutex);
     return result;
}

DFBResult
lite_window_event_loop(LiteWindow *window, int timeout)
{
     DFBResult   ret;
     int timeout_id = 0;

     LITE_NULL_PARAMETER_CHECK(window);

     /* only destroy window when exiting event loop */
     ++window->internal_ref_count;

     /* wrapper is used to track how deep we are in event loops */
     ++event_loop_nesting_level;

     D_DEBUG_AT(LiteWindowDomain, "Enter window event loop with timeout %d, nesting level %d\n",
                timeout, event_loop_nesting_level);

     /* add stop timeout event for the end-of-timeout period */
     if (timeout > 0)
          lite_enqueue_window_timeout(timeout, NULL, NULL, &timeout_id);

     ret = run_window_event_loop(window, timeout < 0);

     /* remove the timeout event -- this should be harmless if it
      * triggered since it will be already removed */
     if (timeout_id)
          lite_remove_window_timeout(timeout_id);

     /* clear event loop flag, then handle destruction if it was deferred */
     if ((--window->internal_ref_count == 0) &&
         (window->flags & LITE_WINDOW_DESTROYED))
          handle_destroy(window);

     D_DEBUG_AT(LiteWindowDomain, "Exit window event loop at nesting level %d\n",
                event_loop_nesting_level);

     --event_loop_nesting_level;
     return ret;
}

static DFBResult
run_window_event_loop(LiteWindow *window, bool once)
{
     DFBResult   ret = DFB_OK;
     DFBResult   exit_code = DFB_OK;
     DFBEvent    evt;
     bool        handle_window_events = true;
     DFBWindowID window_id = window->id;

     prvlite_set_event_loop_alive(true);    /* always reset this when entering the event loop */

     while (true) {
          /* first test if the alive flag is still enabled, if not we immediately exit */
          if (exit_code != DFB_OK || prvlite_get_event_loop_alive() == false)
               return exit_code;

          if (direct_clock_get_millis() - last_update_time >= minimum_update_freq) {
              /* always flush the window-specific events after processing all events */
              lite_flush_window_events(NULL);

              /* redraw any windows that have been changed due to events */
              draw_updated_windows();
          }

          if ((DFB_OK == event_buffer_global->HasEvent(event_buffer_global))) {
               /* get the event, dispatch to event-specific handler */
               ret = event_buffer_global->GetEvent(event_buffer_global, DFB_EVENT(&evt));
               if (ret != DFB_OK)
                    continue; /* return to top of loop */

               if (evt.clazz == DFEC_USER) {
                    if (window->user_event_func)
                         window->user_event_func(DFB_USER_EVENT(&evt), window->user_event_data);
               }
               else if (evt.clazz == DFEC_UNIVERSAL) {
                    if (window->universal_event_func)
                         window->universal_event_func(DFB_UNIVERSAL_EVENT(&evt),
                                                        window->universal_event_data);
               }
               else if (evt.clazz == DFEC_WINDOW) {
                    DFBWindowEvent *win_event = DFB_WINDOW_EVENT(&evt);

                    /* if we have a global window event callback, trigger this before
                       the toolbox will handle the window events */
                    if (window->window_event_func) {
                        ret = window->window_event_func(win_event, window->window_event_data);
                        if (ret != DFB_OK)
                            handle_window_events = false;
                    }

                    /* unless a possible global window event callback has told us it has handled
                       the window event, process it here */
                    if (handle_window_events == true) {
                         ret = lite_handle_window_event(NULL, win_event);

                         /* we need to take care of the current default behavior of
                            exiting the event loop in case the last window is destroyed
                            or closed, or in case current window is destroyed */
                         if (win_event->type == DWET_DESTROYED || ret == DFB_DESTROYED) {
                              if (num_windows_global == 0) {
                                   return DFB_DESTROYED;
                              }
                              else if (window_id == win_event->window_id) {
                                   return DFB_DESTROYED;
                              }
                         }
                         else if (win_event->type == DWET_CLOSE) {
                              if (num_windows_global == 1) {
                                   /* note the num_windows_global is still 1 with a single
                                      window as we never called remove_window() */
                                   lite_destroy_window(window);
                                   return DFB_DESTROYED;
                              }
                         }
                    }
               } /* end DFEC_WINDOW */

               continue; /* return to top of loop */
          } /* end GetEvent */

          /* always flush the window-specific events after processing all events */
          lite_flush_window_events(NULL);

          /* redraw any windows that have been changed due to events */
          draw_updated_windows();

          /* now check for timeout callbacks */
          LiteTimeoutFunc callback = NULL;
          void *callback_data      = NULL;
          ret = remove_next_timeout_callback(&callback, &callback_data);
          if (ret == DFB_OK)
          {
               exit_code = DFB_TIMEOUT;
               if (callback)
                    exit_code = callback(callback_data);
               continue;
          }

          /* now check for idle callbacks */
          ret = remove_top_idle_callback(&callback, &callback_data);
          if (ret == DFB_OK)
          {
               exit_code = DFB_TIMEOUT;
               if (callback)
                    exit_code = callback(callback_data);
               continue;
          }

          /* if we get here, go to sleep waiting on next event */
          long long remaining = 0;
          ret = get_time_until_next_timeout(&remaining);
          if (ret == DFB_OK)
               ret = event_buffer_global->WaitForEventWithTimeout(
                    event_buffer_global,
                    remaining / 1000,
                    remaining % 1000);
          else if (!once)
               ret = event_buffer_global->WaitForEvent(event_buffer_global);
          else
               return DFB_OK;

          /* go back to top and start again */
     }

     /* shouldn't reach this */
     return DFB_BUG;
}

DFBResult
lite_window_event_available(void)
{
     if (DFB_OK == event_buffer_global->HasEvent(event_buffer_global))
          return DFB_OK;

     long long remaining;
     DFBResult ret = get_time_until_next_timeout(&remaining);
     if (ret == DFB_OK && remaining <= 0)
          return DFB_OK;

     return DFB_BUFFEREMPTY;
}

DFBResult
lite_set_window_enabled(LiteWindow *window, bool enabled)
{
     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     if (enabled)
          window->flags &= ~LITE_WINDOW_DISABLED;
     else
          window->flags |= LITE_WINDOW_DISABLED;

     return DFB_OK;
}

DFBResult
lite_update_all_windows(void)
{
     int i;

     D_DEBUG_AT( LiteUpdateDomain, "%s( )\n", __FUNCTION__ );

     for (i = 0; i < num_windows_global; i++) {
         LiteWindow *window = window_array_global[i];

         /* render title bar and borders */
         if (window->theme != liteNoWindowTheme && window->theme->theme.flags & LITE_THEME_HAS_FRAME) {
              DFBDimension size = { window->width, window->height };

              lite_theme_frame_target_update( &window->frame_target, &window->theme->theme.frame, &size );

              render_title( window );
              render_border( window );
         }

         lite_update_window( window, NULL );
     }

     return DFB_OK;
}

DFBResult
lite_update_window(LiteWindow *window, const DFBRegion *region)
{
     int       i;
     DFBRegion update = { 0, 0, window->box.rect.w - 1, window->box.rect.h - 1 };

     D_DEBUG_AT( LiteUpdateDomain, "%s( %p, %p )\n", __FUNCTION__, window, region );

     LITE_NULL_PARAMETER_CHECK(window);

     DFB_REGION_ASSERT_IF( region );

     /* Check requested region. */
     if (region) {
         D_DEBUG_AT( LiteUpdateDomain, "  -> %d, %d - %dx%d\n", DFB_RECTANGLE_VALS_FROM_REGION( region ) );

         if (!dfb_region_region_intersect( &update, region )) {
             D_DEBUG_AT( LiteUpdateDomain, "  -> fully clipped\n" );

             return DFB_OK;
         }
     }

     D_DEBUG_AT( LiteUpdateDomain, "  -> %d, %d - %dx%d (clipped)\n",
                 DFB_RECTANGLE_VALS_FROM_REGION( &update ) );

/*     fprintf( stderr, "==========================> UPDATE + %4d,%4d - %4dx%4d + <- %d\n",
              DFB_RECTANGLE_VALS_FROM_REGION( &update ), window->id );
*/
     pthread_mutex_lock( &window->updates.lock );

     if (window->flags & LITE_WINDOW_PENDING_RESIZE) {
          D_DEBUG_AT( LiteUpdateDomain, "  -> resize is pending, not queuing an update...\n" );
          pthread_mutex_unlock( &window->updates.lock );
          return DFB_OK;
     }

     if (window->updates.pending == LITE_WINDOW_MAX_UPDATES) {
         D_DEBUG_AT( LiteUpdateDomain, "  -> max updates (%d) reached, merging...\n", LITE_WINDOW_MAX_UPDATES );

         for (i=1; i<window->updates.pending; i++)
             dfb_region_region_union( &window->updates.regions[0], &window->updates.regions[i] );

         window->updates.pending = 1;

         D_DEBUG_AT( LiteUpdateDomain, "  -> new single update: %d, %d - %dx%d [0]\n",
                     DFB_RECTANGLE_VALS_FROM_REGION( &window->updates.regions[0] ) );
     }

     for (i=0; i<window->updates.pending; i++) {
         if (dfb_region_region_intersects( &update, &window->updates.regions[i] )) {
             D_DEBUG_AT( LiteUpdateDomain, "  -> intersects with: %d, %d - %dx%d [%d]\n",
                         DFB_RECTANGLE_VALS_FROM_REGION( &window->updates.regions[i] ), i );

             dfb_region_region_union( &window->updates.regions[i], &update );

             D_DEBUG_AT( LiteUpdateDomain, "  -> resulting union: %d, %d - %dx%d [%d]\n",
                         DFB_RECTANGLE_VALS_FROM_REGION( &window->updates.regions[i] ), i );

             break;
         }
     }

     if (i == window->updates.pending) {
         D_DEBUG_AT( LiteUpdateDomain, "  -> adding %d, %d - %dx%d [%d]\n",
                     DFB_RECTANGLE_VALS_FROM_REGION( &update ), i );

         window->updates.regions[i] = update;

         window->updates.pending++;
     }

     pthread_mutex_unlock( &window->updates.lock );

     prvlite_wakeup_event_loop();
     return DFB_OK;
}

DFBResult
lite_set_window_title(LiteWindow *window, const char *title)
{
     char *new_title = NULL;

     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);
     LITE_NULL_PARAMETER_CHECK(title);

     /* make a copy of the new title first */
     if (title)
          new_title = D_STRDUP(title);

     /* free old title */
     if (window->title)
          D_FREE(window->title);

     /* set new title */
     window->title = new_title;

     /* render/flip title bar */
     if (window->theme != liteNoWindowTheme && window->theme->theme.flags & LITE_THEME_HAS_FRAME) {
          DFBRegion         region;
          IDirectFBSurface *surface = window->surface;

          render_title(window);

          /* The title bar region */
          region = DFB_REGION_INIT_FROM_RECTANGLE( &window->theme->theme.frame.parts[LITE_THEME_FRAME_PART_TOP].rect );

          surface->Flip(surface, &region, getenv("LITE_WINDOW_DOUBLEBUFFER") ? DSFLIP_BLIT: 0);
     }

     return DFB_OK;
}

DFBResult
lite_set_window_opacity(LiteWindow *window, u8 opacity)
{
     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     if (window->opacity_mode == LITE_BLEND_NEVER && opacity && opacity < 0xff)
          opacity = 0xff;

     window->opacity = opacity;

     /* Show uninitialized windows later. */
     if ((window->flags & LITE_WINDOW_DRAWN) || !opacity)
          return window->window->SetOpacity(window->window, opacity);

     return DFB_OK;
}

DFBResult
lite_set_window_background(LiteWindow *window, const DFBColor *color)
{
    LITE_NULL_PARAMETER_CHECK(window);
    LITE_WINDOW_PARAMETER_CHECK(window);

     if (color) {
          if (!window->bg.enabled ||
              !DFB_COLOR_EQUAL(window->bg.color, *color))
          {
               window->bg.enabled = DFB_TRUE;
               window->bg.color   = *color;

               lite_update_box(LITE_BOX(window), NULL);
          }
     }
     else
          window->bg.enabled = DFB_FALSE;

    return DFB_OK;
}

DFBResult
lite_set_window_background_color(LiteWindow *window,
                                 u8          r,
                                 u8          g,
                                 u8          b,
                                 u8          a)
{
     DFBColor color = { a, r, g, b };

     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     return lite_set_window_background(window, &color);
}

DFBResult
lite_set_window_blend_mode(LiteWindow    *window,
                             LiteBlendMode  content,
                             LiteBlendMode  opacity)
{
     DFBGraphicsDeviceDescription desc;
     DFBWindowOptions    options;

     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     lite_dfb->GetDeviceDescription(lite_dfb, &desc);


     switch (content) {
          case LITE_BLEND_ALWAYS:
          case LITE_BLEND_NEVER:
               window->content_mode = content;
               break;
          default:
               if (desc.blitting_flags & DSBLIT_BLEND_ALPHACHANNEL)
                    window->content_mode = LITE_BLEND_ALWAYS;
               else
                    window->content_mode = LITE_BLEND_NEVER;
               break;
     }

     window->window->GetOptions(window->window, &options);

     if (window->content_mode == LITE_BLEND_NEVER)
          options |= DWOP_OPAQUE_REGION;
     else
          options &= ~DWOP_OPAQUE_REGION;

     window->window->SetOptions(window->window, options);


     switch (opacity) {
          case LITE_BLEND_ALWAYS:
          case LITE_BLEND_NEVER:
               window->opacity_mode = opacity;
               break;
          default:
               if (desc.blitting_flags & DSBLIT_BLEND_COLORALPHA)
                    window->opacity_mode = LITE_BLEND_ALWAYS;
               else
                    window->opacity_mode = LITE_BLEND_NEVER;
               break;
     }

     return DFB_OK;
}

DFBResult
lite_resize_window(LiteWindow   *window,
                   unsigned int  width,
                   unsigned int  height)
{
     DFBResult         ret = DFB_OK;
     unsigned int      nw, nh;
     IDirectFBWindow  *win = NULL;

     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     D_DEBUG_AT( LiteWindowDomain, "%s(%p, %u, %u )\n", __FUNCTION__, window, width, height );

     if (width == 0 || width > INT_MAX || height == 0 || height > INT_MAX)
          return DFB_INVAREA;

     win = window->window;

     /* calculate new size */
     nw = width;
     nh = height;

     if (window->theme != liteNoWindowTheme && window->theme->theme.flags & LITE_THEME_HAS_FRAME) {
          LiteThemeFrame *frame = &window->theme->theme.frame;

          D_MAGIC_ASSERT( frame, LiteThemeFrame );

          nw += frame->parts[LITE_THEME_FRAME_PART_LEFT].rect.w  + frame->parts[LITE_THEME_FRAME_PART_RIGHT].rect.w;
          nh += frame->parts[LITE_THEME_FRAME_PART_TOP].rect.h + frame->parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h;

          D_DEBUG_AT( LiteWindowDomain, " -> theme adjusted w/h = ( %u, %u )\n", nw, nh );
     }

     if (nw == window->width && nh == window->height)
          return DFB_OK;

     pthread_mutex_lock( &window->updates.lock );

     window->flags |= LITE_WINDOW_PENDING_RESIZE;
     window->flags &= ~LITE_WINDOW_DRAWN;

     window->updates.pending = 0;

     pthread_mutex_unlock( &window->updates.lock );

     /* set the new size */
     ret = win->Resize(win, nw, nh);
     if (ret)
          DirectFBError("lite_resize_window: Window->Resize", ret);
     else {
          window->width = nw;
          window->height = nh;
     }
     /* remaining stuff happens during DWET_SIZE event in handle_resize */

     return ret;
}

DFBResult
lite_set_window_bounds(LiteWindow   *window,
                       int           x,
                       int           y,
                       unsigned int  width,
                       unsigned int  height)
{
     DFBResult         ret = DFB_OK;
     unsigned int      nw, nh;
     IDirectFBWindow  *win = NULL;

     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     if (width == 0 || width > INT_MAX || height == 0 || height > INT_MAX)
          return DFB_INVAREA;

     win = window->window;

     /* calculate new size */
     nw = width;
     nh = height;

     if (window->theme != liteNoWindowTheme && window->theme->theme.flags & LITE_THEME_HAS_FRAME) {
          LiteThemeFrame *frame = &window->theme->theme.frame;

          D_MAGIC_ASSERT( frame, LiteThemeFrame );

          nw += frame->parts[LITE_THEME_FRAME_PART_LEFT].rect.w + frame->parts[LITE_THEME_FRAME_PART_RIGHT].rect.w;
          nh += frame->parts[LITE_THEME_FRAME_PART_TOP].rect.h  + frame->parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h;
     }

     /* don't resize if we're just repositioning */
     if (nw == window->width && nh == window->height)
     {
         ret = win->MoveTo(win, x, y);
         return ret;
     }

     pthread_mutex_lock( &window->updates.lock );

     window->flags |= LITE_WINDOW_PENDING_RESIZE;
     window->flags &= ~LITE_WINDOW_DRAWN;

     window->updates.pending = 0;

     pthread_mutex_unlock( &window->updates.lock );

     /* set the new size */
     ret = win->SetBounds(win, x, y, nw, nh);
     if (ret)
          DirectFBError("lite_set_window_bounds: Window->SetBounds", ret);
     else {
          window->width = nw;
          window->height = nh;
     }
     /* remaining stuff happens during DWET_POSITION_SIZE event in handle_resize */

     return ret;
}

DFBResult
lite_get_window_size(LiteWindow *window,
                     int        *ret_width,
                     int        *ret_height)
{
     DFBResult          ret = DFB_OK;
     IDirectFBWindow    *win = NULL;

     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     win = window->window;

     ret = win->GetSize(win, ret_width, ret_height);

     return ret;
}

DFBResult
lite_minimize_window(LiteWindow *window)
{
     IDirectFBWindow *win = window->window;

     window->last_width  = window->width;
     window->last_height = window->height;

     lite_resize_window(window, window->min_width, window->min_height);
     win->Move(win, (window->last_width - window->min_width) / 2, 0);

     return DFB_OK;
}

DFBResult
lite_restore_window(LiteWindow *window)
{
     IDirectFBWindow *win = window->window;

     win->Move(win, (window->min_width - window->last_width) / 2, 0);
     lite_resize_window(window, window->last_width, window->last_height);

     return DFB_OK;
}

DFBResult
lite_focus_box(LiteBox *box)
{
     LiteWindow *window = lite_find_my_window(box);
     LiteBox    *old    = window->focused_box;

     if (old == box)
          return DFB_OK;

     if (old) {
          old->is_focused = 0;

          if (old->OnFocusOut)
               old->OnFocusOut (old);
     }

     window->focused_box = box;

     box->is_focused = 1;

     if (box->OnFocusIn)
          box->OnFocusIn (box);

     return DFB_OK;
}

int
lite_handle_window_event(LiteWindow *win,  DFBWindowEvent *event)
{
     int         ret = DFB_OK;
     LiteWindow *window = NULL;

     /* if the event window id does not correspond to any known id, return */
     if(win == NULL) {
        if (!(window = find_window_by_id(event->window_id)))
            return 0;
     }
     else
        window = win;

     /* deal with window destruction early */
     if (event->type == DWET_DESTROYED) {
          /* remove the initial ref added at creation  */
          --window->internal_ref_count;
          handle_destroy(window);
          return DFB_DESTROYED;
     }
     else if (window->flags & LITE_WINDOW_DESTROYED) {
          /* ignore other events for destroyed or disabled windows */
          return DFB_OK;
     }

     /* let lite_destroy_window() know window is busy */
     ++window->internal_ref_count;

     /* below are many raw callbacks that might be installed, these raw
        callbacks have a chance to intercept events before they are cooked
        in the other cases where the events are processed */

     /* we need to call the raw mouse and keyboard functions before
        the modal test, hence they are here and not futher down in the
        big switch statement */
     if (!(window->flags & LITE_WINDOW_DISABLED)) {
          if (event->type == DWET_BUTTONUP || event->type == DWET_BUTTONDOWN) {
               if (window->raw_mouse_func) {
                    ret = window->raw_mouse_func(event, window->raw_mouse_data);
                    /* test and return if the callback indicates stopping the
                       event flow */
                    if (ret != DFB_OK) {
                         --window->internal_ref_count;
                         return 0;
                    }
               }
          }
          else if (event->type == DWET_MOTION) {
               if (window->raw_mouse_moved_func) {
                    ret = window->raw_mouse_moved_func(event, window->raw_mouse_data);
                    /* test and return if the callback indicates stopping the event flow */
                    if (ret != DFB_OK) {
                         --window->internal_ref_count;
                         return 0;
                    }
               }
          }
          else if(event->type == DWET_KEYUP || event->type == DWET_KEYDOWN) {
               if (window->raw_keyboard_func) {
                    ret = window->raw_keyboard_func(event, window->raw_keyboard_data);
                    /* test and return if the callback indicates stoppping the event flow */
                    if (ret != DFB_OK) {
                         --window->internal_ref_count;
                         return 0;
                    }
               }
          }
          else if(event->type == DWET_WHEEL) {
               if (window->raw_wheel_func) {
                    ret = window->raw_wheel_func(event, window->raw_wheel_data);
                    /* test and return if the callback indicates stopping the event flow  */
                    if (ret != DFB_OK) {
                         --window->internal_ref_count;
                         return 0;
                    }
               }
          }
     }

     /* events handled by all windows */
     switch (event->type) {
          case DWET_POSITION:
               ret = handle_move(window, event);
               break;
          case DWET_SIZE:
               window->last_resize = *event;
               break;
          case DWET_POSITION_SIZE:
               ret = handle_move(window, event);
               if (window->flags & LITE_WINDOW_CONFIGURED)
                    window->last_resize = *event;
               else
                    window->flags |= LITE_WINDOW_CONFIGURED;
               break;
          case DWET_CLOSE:
               ret = handle_close(window);
               break;
          case DWET_LOSTFOCUS:
               ret = handle_lost_focus(window);
               break;
          case DWET_GOTFOCUS:
               ret = handle_got_focus(window);
               break;
          default:
               ;
     }

     /* events handled by enabled windows */
     if (!(window->flags & LITE_WINDOW_DISABLED)) {
          switch (event->type) {
               case DWET_ENTER:
                    window->last_motion = *event;
                    ret = handle_enter(window, event);
                    break;
               case DWET_LEAVE:
                    ret = handle_leave(window, event);
                    break;
               case DWET_MOTION:
                    window->last_motion = *event;
                    if(window->mouse_func)
                         window->mouse_func(event, window->mouse_data);
                    break;
               case DWET_BUTTONUP:
               case DWET_BUTTONDOWN:
                    ret = handle_button(window, event);
                    if (window->mouse_func && !(window->flags & LITE_WINDOW_DESTROYED))
                         window->mouse_func(event, window->mouse_data);
                    break;
               case DWET_KEYUP:
                    ret = handle_key_up(window, event);
                    if(window->keyboard_func && !(window->flags & LITE_WINDOW_DESTROYED))
                         window->keyboard_func(event, window->keyboard_data);
                    break;
               case DWET_KEYDOWN:
                    ret = handle_key_down(window, event);
                    if(window->keyboard_func && !(window->flags & LITE_WINDOW_DESTROYED))
                         window->keyboard_func(event, window->keyboard_data);
                    break;
               case DWET_WHEEL:
                    ret = handle_wheel(window, event);
                    if(window->wheel_func && !(window->flags & LITE_WINDOW_DESTROYED))
                         window->wheel_func(event, window->wheel_data);
                    break;
               default:
                    ;
          }
     }

     /* remove the event loop flag  */
     --window->internal_ref_count;

     /* if window has been marked for destruction, destroy it */
     if (window->flags & LITE_WINDOW_DESTROYED) {
          ret = DFB_DESTROYED;
     }

     return ret;
}

DFBResult
lite_flush_window_events(LiteWindow *window)
{
     if (window) {
          if (!(window->flags & LITE_WINDOW_DESTROYED)) {
               flush_resize(window);
               flush_motion(window);
          }
     }
     else {
          int n;

          for (n=0; n<num_windows_global; n++) {
               lite_flush_window_events(window_array_global[n]);
          }
     }

     return DFB_OK;
}

DFBResult
lite_post_event_to_window(LiteWindow* window, DFBEvent* event)
{
     return event_buffer_global->PostEvent(event_buffer_global, DFB_EVENT(event));
}

DFBResult
lite_on_window_event(LiteWindow         *window,
                     LiteWindowEventFunc func,
                     void               *data)
{
    LITE_NULL_PARAMETER_CHECK(window);
    LITE_WINDOW_PARAMETER_CHECK(window);

    window->window_event_func = func;
    window->window_event_data = data;

    return 0;
}

DFBResult
lite_on_window_universal_event(LiteWindow                   *window,
                               LiteWindowUniversalEventFunc  func,
                               void                         *data)
{
    LITE_NULL_PARAMETER_CHECK(window);
    LITE_WINDOW_PARAMETER_CHECK(window);

    window->universal_event_func = func;
    window->universal_event_data = data;

    return 0;
}

DFBResult
lite_on_window_user_event(LiteWindow       *window,
                    LiteWindowUserEventFunc func,
                    void                   *data)
{
    LITE_NULL_PARAMETER_CHECK(window);
    LITE_WINDOW_PARAMETER_CHECK(window);

    window->user_event_func = func;
    window->user_event_data = data;

    return 0;
}


LiteWindow*
lite_find_my_window(LiteBox *box)
{
     while (box->parent)
          box = box->parent;

     if (box->type == LITE_TYPE_WINDOW)
          return LITE_WINDOW(box);

     return NULL;
}

DFBResult
lite_close_window (LiteWindow *window)
{
     D_DEBUG_ENTER(LiteWindowDomain, "(window == %p)\n", window);

     IDirectFBWindow *dfb_window = NULL;

     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     dfb_window = window->window;

     return dfb_window->Close(dfb_window);
}

DFBResult
lite_destroy_window(LiteWindow *window)
{
     D_DEBUG_ENTER(LiteWindowDomain, "(window == %p)\n", window);

     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     /* don't allow recursive deletion */
     if (window->flags & LITE_WINDOW_DESTROYED) {
          return DFB_OK;
     }

     lite_set_window_opacity(window, liteNoWindowOpacity);
     destroy_window_data(window);

     if (window->internal_ref_count == 0 || event_loop_nesting_level == 0) {
          /* finish destruction if no event loop running */
          window->internal_ref_count = 0;
          handle_destroy(window);
     }

     return DFB_OK;
}

DFBResult
lite_destroy_all_windows(void)
{
     D_DEBUG_ENTER(LiteWindowDomain, "()\n");

     /* destroy all windows in most-recent-first order */
     int num_windows = num_windows_global;
     while (--num_windows >= 0) {

          /* if we deleted a number of windows in previous iteration,
           * we might have to reset counter */
          if (num_windows >= num_windows_global) {
               num_windows = num_windows_global;
               continue;
          }

          LiteWindow *window = window_array_global[num_windows];
          /* don't destroy already destroyed windows that haven't been cleared
           * and don't destroy a window with a creator, as the creator will be
           * responsible for destroying the window */
          if (!(window->flags & LITE_WINDOW_DESTROYED) && 
              window->creator == NULL)
               lite_destroy_window(window);
     }

     return DFB_OK;
}

DFBResult
lite_on_raw_window_mouse(LiteWindow          *window,
                          LiteWindowMouseFunc func,
                          void               *data)
{
     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     window->raw_mouse_func = func;
     window->raw_mouse_data = data;

     return 0;
}

DFBResult
lite_on_raw_window_mouse_moved(LiteWindow          *window,
                               LiteWindowMouseFunc func,
                               void               *data)
{
     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     window->raw_mouse_moved_func = func;
     window->raw_mouse_moved_data = data;

     return 0;
}


DFBResult
lite_on_window_mouse(LiteWindow         *window,
                     LiteWindowMouseFunc func,
                     void               *data)
{
     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     window->mouse_func = func;
     window->mouse_data = data;

     return 0;
}

DFBResult
lite_on_raw_window_keyboard(LiteWindow            *window,
                            LiteWindowKeyboardFunc func,
                            void                  *data)
{
     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     window->raw_keyboard_func = func;
     window->raw_keyboard_data = data;

     return 0;
}

DFBResult
lite_on_window_keyboard(LiteWindow            *window,
                        LiteWindowKeyboardFunc func,
                         void                 *data)
{
     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     window->keyboard_func = func;
     window->keyboard_data = data;

     return 0;
}

DFBResult
lite_on_raw_window_wheel(LiteWindow         *window,
                         LiteWindowWheelFunc func,
                         void           *data)
{
     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     window->raw_wheel_func = func;
     window->raw_wheel_data = data;

     return DFB_OK;
}

DFBResult
lite_on_window_wheel(LiteWindow         *window,
                     LiteWindowWheelFunc func,
                         void           *data)
{
     LITE_NULL_PARAMETER_CHECK(window);
     LITE_WINDOW_PARAMETER_CHECK(window);

     window->wheel_func = func;
     window->wheel_data = data;

     return DFB_OK;
}

DFBResult
lite_release_window_resources(void)
{
     release_grabs();
     return DFB_OK;
}

DFBResult
lite_get_event_buffer(IDirectFBEventBuffer **buffer)
{
     *buffer = event_buffer_global;
     return DFB_OK;
}

bool
lite_default_window_theme_loaded(void)
{
     return liteDefaultWindowTheme != NULL;
}

DFBResult
lite_free_window_theme(LiteWindowTheme *theme)
{
     LITE_NULL_PARAMETER_CHECK(theme);

     if (theme->title_font)
          lite_release_font(theme->title_font);

     if (theme != liteNoWindowTheme && theme->theme.flags & LITE_THEME_HAS_FRAME) {
          LiteThemeFrame *frame = &theme->theme.frame;

          D_MAGIC_ASSERT( frame, LiteThemeFrame );

          lite_theme_frame_unload( frame );

          theme->theme.flags &= ~LITE_THEME_HAS_FRAME;
     }

     D_FREE(theme);
     theme = NULL;

     return DFB_OK;
}


/* internals */

static void
flush_resize(LiteWindow *window)
{
     D_ASSERT(window != NULL);

     if (!window->last_resize.type)
          return;

     handle_resize(window, &window->last_resize);

     window->last_resize.type = 0;
}

static void
flush_motion(LiteWindow *window)
{
     D_ASSERT(window != NULL);

     /* if we're just hitting this because we finished processing events, but there's
      * no cursor motion, we still need to check to see if the cursor moved */
     if (window->entered_box != NULL &&
         window->last_motion.type == 0) {
          validate_entered_box(window);
          return;
     }

     if (window->last_motion.type != 0) {
          handle_motion(window, &window->last_motion);
          window->last_motion.type = 0;
     }
}

static int
handle_move(LiteWindow *window, DFBWindowEvent *ev)
{
     D_ASSERT(window != NULL);
     D_ASSERT(ev != NULL);

     if (window->OnMove)
          return window->OnMove(window, ev->x, ev->y);

     return 0;
}

static int
handle_resize(LiteWindow *window, DFBWindowEvent *ev)
{
     DFBResult         ret;
     DFBRectangle      rect;
     IDirectFBSurface *subsurface = NULL;
     IDirectFBSurface *surface = window->surface;
     LiteThemeFrame   *frame = NULL;

     D_ASSERT(window != NULL);
     D_ASSERT(ev != NULL);

     D_DEBUG_AT( LiteWindowDomain, "%s( %p, %p )\n", __FUNCTION__, window, ev );

/*    fprintf( stderr, "==========================> HANDLE       ---> %4dx%4d <--- <- %d\n",
              ev->w, ev->h, window->id );
*/
     if (window->theme != liteNoWindowTheme && window->theme->theme.flags & LITE_THEME_HAS_FRAME) {
          frame = &window->theme->theme.frame;

          D_MAGIC_ASSERT( frame, LiteThemeFrame );
     }


     /* Get a new sub surface */
     if (frame) {
          rect.x = frame->parts[LITE_THEME_FRAME_PART_LEFT].rect.w;
          rect.y = frame->parts[LITE_THEME_FRAME_PART_TOP].rect.h;
          rect.w = ev->w - frame->parts[LITE_THEME_FRAME_PART_LEFT].rect.w - frame->parts[LITE_THEME_FRAME_PART_RIGHT].rect.w;
          rect.h = ev->h - frame->parts[LITE_THEME_FRAME_PART_TOP].rect.h  - frame->parts[LITE_THEME_FRAME_PART_BOTTOM].rect.h;
     }
     else {
          rect.x = 0;
          rect.y = 0;
          rect.w = ev->w;
          rect.h = ev->h;
     }

#if DIRECTFB_MAJOR_VERSION >  1 || \
   (DIRECTFB_MAJOR_VERSION == 1 && DIRECTFB_MINOR_VERSION > 2) || \
   (DIRECTFB_MAJOR_VERSION == 1 && DIRECTFB_MINOR_VERSION == 2 && DIRECTFB_MICRO_VERSION >= 1)
     ret = window->box.surface->MakeSubSurface(window->box.surface, surface, &rect);
     if (ret)
          DirectFBError("handle_resize: Surface->MakeSubSurface", ret);
     else {
          subsurface = window->box.surface;

          /* Store new sub rectangle */
          window->box.rect = rect;
     }
#else
     ret = surface->GetSubSurface(surface, &rect, &subsurface);
     if (ret)
          DirectFBError("handle_resize: Surface->GetSubSurface", ret);
     else {
          /* Destroy old sub surface */
          window->box.surface->Release(window->box.surface);

          /* Store new sub rect/surface */
          window->box.rect    = rect;
          window->box.surface = subsurface;
     }
#endif

     /* Call custom handler */
     if (window->OnResize)
          window->OnResize(window, rect.w, rect.h);

     /* Give each box a new sub surface. */
     lite_reinit_box_and_children(LITE_BOX(window));

     /* Set opaque content region. */
     window->window->SetOpaqueRegion(window->window, rect.x, rect.y,
                                      rect.x + rect.w - 1, rect.y + rect.h - 1);

     /* Store new size */
     window->width  = ev->w;
     window->height = ev->h;

     /* Render title bar and borders */
     if (frame) {
          DFBDimension size = { window->width, window->height };

          lite_theme_frame_target_update( &window->frame_target, frame, &size );

          render_title(window);
          render_border(window);
     }

     pthread_mutex_lock( &window->updates.lock );

     window->flags &= ~LITE_WINDOW_PENDING_RESIZE;

     /* Redraw window content */
     lite_draw_box(LITE_BOX(window), NULL, DFB_FALSE);

     pthread_mutex_unlock( &window->updates.lock );

     /* Flip window surface */
     surface->Flip(surface, NULL, getenv("LITE_WINDOW_DOUBLEBUFFER") ? DSFLIP_BLIT: 0);

     return 1;
}

static int
handle_close(LiteWindow *window)
{
     D_ASSERT(window != NULL);

     if (window->OnClose)
          return window->OnClose(window);

     return 0;
}

static int
handle_enter(LiteWindow *window, DFBWindowEvent *ev)
{
     D_ASSERT(window != NULL);
     D_ASSERT(ev != NULL);

     D_DEBUG_AT( LiteMotionDomain, "Window %p gets ENTER message\n", window );

     entered_window_global = window;

     if (window->OnEnter)
          window->OnEnter(window, ev->x, ev->y);

     handle_motion(window, ev);

     return 0;
}

static int
handle_leave(LiteWindow *window, DFBWindowEvent *ev)
{
     D_ASSERT(window != NULL);
     D_ASSERT(ev != NULL);

     D_DEBUG_AT( LiteMotionDomain, "Window %p gets LEAVE message\n", window );

     if (window == entered_window_global) {
          LiteBox *entered = window->entered_box;

          D_DEBUG_AT( LiteMotionDomain, "Leaving box %p due to Window Leave\n", entered );

          if (entered && entered->OnLeave)
               entered->OnLeave(entered, -1, -1); /* FIXME */
          
          window->entered_box = NULL;
     }

     lite_release_window_drag_box(window);

     if (window->OnLeave)
          return window->OnLeave(window, ev->x, ev->y);

     // clear the queued motion so we won't reenter the window
     window->last_motion.type = 0;

     if (window == entered_window_global)
          entered_window_global = NULL;
 
     return 0;
}

static int
handle_lost_focus(LiteWindow *window)
{
     D_ASSERT(window != NULL);

     window->has_focus = 0;

     lite_release_window_drag_box(window);     

     if (window->OnFocusOut)
          window->OnFocusOut(window);

     return 0;
}

static void
validate_entered_box(LiteWindow *window)
{
     /* get the cached cursor coordinates */
     int x = window->last_motion.x;
     int y = window->last_motion.y;

     LiteBox *box = LITE_BOX(window);
     if (DFB_RECTANGLE_CONTAINS_POINT(&box->rect, x, y))
     {
          /* if we're tracking mouse motion in a box try searching that box and its children first */
          if (window->entered_box && window->entered_box->parent) {
               int parent_x = x, parent_y = y;
               child_coords(window->entered_box->parent, &parent_x, &parent_y);
               if (DFB_RECTANGLE_CONTAINS_POINT(&window->entered_box->rect, parent_x, parent_y)) {
                    box = window->entered_box;
                    x = parent_x - box->rect.x;
                    y = parent_y - box->rect.y;
               }
          }
          else
          {
               x -=  box->rect.x;
               y -=  box->rect.y;
          }
          box = find_child(box, &x, &y);

          if (!box->is_active)
               return;

          if (window->entered_box != box) {
               D_DEBUG_AT( LiteMotionDomain, "validate_entered_box: at (%d, %d) left box (%p), entered box (%p)\n",
                           window->last_motion.x, window->last_motion.y, window->entered_box, box );
               LiteBox *entered = window->entered_box;

               if (entered && entered->OnLeave)
                    entered->OnLeave(entered, -1, -1); /* FIXME */

               window->entered_box = box;

               if (box->OnEnter)
                    box->OnEnter(box, x, y);
          }
     }
}

static int
handle_got_focus(LiteWindow *window)
{
     D_ASSERT(window != NULL);

     window->has_focus = 1;

     if (window->OnFocusIn)
          window->OnFocusIn(window);

     return 0;
}

static int
handle_motion(LiteWindow *window, DFBWindowEvent *ev)
{
     D_ASSERT(window != NULL);
     D_ASSERT(ev != NULL);

     LiteBox *box = LITE_BOX(window);

     if (window->moving) {
          window->window->Move(window->window,
                                ev->cx - window->old_x, ev->cy - window->old_y);

          window->old_x = ev->cx;
          window->old_y = ev->cy;

          return 1;
     }

     if (window->resizing) {
          int dx = ev->cx - window->old_x;
          int dy = ev->cy - window->old_y;

          int width, height;

          window->window->GetSize( window->window, &width, &height );

          if (width + dx < window->min_width)
               dx = window->min_width - width;

          if (height + dy < window->min_height)
               dy = window->min_height - height;

          if (window->step_x) {
               if (dx < 0)
                    dx += (-dx) % window->step_x;
               else
                    dx -= dx % window->step_x;
          }

          if (window->step_y) {
               if (dy < 0)
                    dy += (-dy) % window->step_y;
               else
                    dy -= dy % window->step_y;
          }

          window->window->Resize(window->window,
                                 width + dx, height + dy);

          window->old_x += dx;
          window->old_y += dy;

          return 1;
     }

     if (window->drag_box) {
          int      x        = ev->x;
          int      y        = ev->y;
          LiteBox *drag_box = window->drag_box;

          child_coords(drag_box, &x, &y);

          if (drag_box->OnMotion)
               return drag_box->OnMotion(drag_box, x, y, ev->buttons);

          return 0;
     }

     if (DFB_RECTANGLE_CONTAINS_POINT(&box->rect, ev->x, ev->y))
     {
          int x = ev->x - box->rect.x;
          int y = ev->y - box->rect.y;

          /* if we're tracking mouse motion in a box try searching that box and its children first */
          if (window->entered_box && window->entered_box->parent) {
               int parent_x = x, parent_y = y;
               child_coords(window->entered_box->parent, &parent_x, &parent_y);
               if (DFB_RECTANGLE_CONTAINS_POINT(&window->entered_box->rect, parent_x, parent_y)) {
                    box = window->entered_box;
                    x = parent_x - box->rect.x;
                    y = parent_y - box->rect.y;
               }
          }

          box = find_child(box, &x, &y);

          if (!box->is_active)
               return 0;

          if (window->entered_box != box) {
               D_DEBUG_AT( LiteMotionDomain, "handle_motion: at (%d, %d) left box (%p), entered box (%p)\n",
                           window->last_motion.x, window->last_motion.y, window->entered_box, box );

               LiteBox *entered = window->entered_box;

               if (entered && entered->OnLeave)
                    entered->OnLeave(entered, -1, -1); /* FIXME */

               window->entered_box = box;

               if (box->OnEnter)
                    return box->OnEnter(box, x, y);
          }
          else if (box->OnMotion)
               return box->OnMotion(box, x, y, ev->buttons);
     }
     else if (window == entered_window_global)
          handle_leave(window, ev);

     return 0;
}

static int
handle_button(LiteWindow *window, DFBWindowEvent *ev)
{
     DFBResult        ret;
     LiteBox         *box = LITE_BOX(window);
     IDirectFBWindow *win = window->window;

     D_ASSERT(window != NULL);
     D_ASSERT(ev != NULL);

     if (window->moving || window->resizing) {
          if (ev->type == DWET_BUTTONUP && ev->button == DIBI_LEFT) {
          	   if (!(window->flags & LITE_WINDOW_MODAL)) {
                    release_grabs();
          	   }
               window->moving = window->resizing = 0;
          }

          return 1;
     }

     if (window->drag_box) {
          int      x        = ev->x;
          int      y        = ev->y;
          int      result   = 0;
          LiteBox *drag_box = window->drag_box;

          child_coords(drag_box, &x, &y);

          switch (ev->type) {
               case DWET_BUTTONDOWN:
                    if (drag_box->OnButtonDown)
                         result = drag_box->OnButtonDown(drag_box,
                                                          x, y, ev->button);

                    break;

               case DWET_BUTTONUP:
                    if (!ev->buttons) {
                         lite_release_window_drag_box(window);
                    }
                    if (drag_box->OnButtonUp) {
                         result = drag_box->OnButtonUp(drag_box,
                                                        x, y, ev->button);
                    }


                    break;

               default:
                    break;
          }

          return result;
     }

     if (DFB_RECTANGLE_CONTAINS_POINT(&box->rect, ev->x, ev->y)) {
          int x = ev->x - box->rect.x;
          int y = ev->y - box->rect.y;

          box = find_child(box, &x, &y);

          if (!box->is_active)
               return 0;

          switch (ev->type) {
               case DWET_BUTTONDOWN:
                    if (!window->drag_box) {
                    	 if (!(window->flags & LITE_WINDOW_MODAL)) {
                    	      ret = win->GrabPointer(win);
                              if (ret)
                                   DirectFBError("handle_button: "
                                                 "window->GrabPointer", ret);
                              else {
                                   window->drag_box = box;
                                   win_pointergrabbed_global = win;
                                   D_DEBUG_AT( LiteWindowDomain, 
                                               "setting drag_box: win_pointergrabbed_global = %p from %p\n", 
                                               win, window );
                              }
                    	 }
                         else {
                            window->drag_box = box;
                        }
                    }

                    if (box->OnButtonDown)
                         return box->OnButtonDown(box, x, y, ev->button);

                    break;

               case DWET_BUTTONUP:
                    if (box->OnButtonUp)
                         return box->OnButtonUp(box, x, y, ev->button);

                    break;

               default:
                    break;
          }
     }
     else if (ev->type == DWET_BUTTONDOWN) {
          long long diff;

          /* only allow moving and resizing windows if LITE_WINDOW_FIXED is not set */
          if ( !(window->flags & LITE_WINDOW_FIXED)) {
              switch (ev->button) {
                   case DIBI_LEFT:
                        if (!(window->flags & LITE_WINDOW_MODAL)) {
                             ret = win->GrabPointer(win);
                             if (ret) {
                                  DirectFBError("handle_button: window->GrabPointer", ret);
                                  return 0;
                             }
                             else {
                                 win_pointergrabbed_global = win;
                                 D_DEBUG_AT( LiteWindowDomain, 
                                             "dragging title: win_pointergrabbed_global = %p from %p\n", 
                                             win, window );
                             }
                        }

                        diff = (ev->timestamp.tv_sec -
                                window->last_click.tv_sec) * (long long) 1000000 +
                               (ev->timestamp.tv_usec - window->last_click.tv_usec);

                        window->last_click = ev->timestamp;

                        if (ev->x >= (box->rect.x + box->rect.w - 10) &&
                            ev->y >= (box->rect.y + box->rect.h))
                        {
                             /* bottom right corner */
                             if (window->flags & LITE_WINDOW_RESIZE) {
                                window->resizing = true;
                             }
                             else {
                             	  if (!(window->flags & LITE_WINDOW_MODAL)) {
                                       release_grabs();
                             	  }
                             }
                        }
                        else if (ev->y < box->rect.y && diff < 400000) {
                             /* title bar double click */
                             if (window->flags & LITE_WINDOW_MINIMIZE) {
                                 if (window->width > window->min_width ||
                                     window->height > window->min_height)
                                      lite_minimize_window(window);
                                 else
                                      lite_restore_window(window);
                             }
                             if (!(window->flags & LITE_WINDOW_MODAL)) {
                                  release_grabs();
                             }
                        }
                        else {
                             /* rest of the border */
                             if (ev->x > 0 && ev->y > 0 && ev->x < window->width && ev->y < window->height) {
                                  window->moving = true;
                             }
                             else {
                                 if (!(window->flags & LITE_WINDOW_MODAL)) {
                                      release_grabs();
                                 }
                             }
                        }

                        window->old_x = ev->cx;
                        window->old_y = ev->cy;

                        return 1;

                   case DIBI_RIGHT:
                        win->LowerToBottom(win);

                        return 1;

                   default:
                        break;
              } /* end switch */
          }
     }

     return 0;
}

static int
handle_key_up(LiteWindow *window, DFBWindowEvent *ev)
{
     LiteBox *box = window->focused_box;

     D_ASSERT(window != NULL);
     D_ASSERT(ev != NULL);

     prvlite_set_current_key_modifier(ev->modifiers);

     if (box)
     {
          if (box->is_visible == 0)
               return 0;

          if (box->handle_keys == 0)
               return 0;

          if (!box->is_active)
               return 0;

          if (ev->key_id == DIKI_TAB && box)
               return 1;

          return box->OnKeyUp ? box->OnKeyUp(box, ev) : 0;
     }

     return 0;
}

static int
handle_key_down(LiteWindow *window, DFBWindowEvent *ev)
{
     LiteBox *box = window->focused_box;

     D_ASSERT(window != NULL);
     D_ASSERT(ev != NULL);

     prvlite_set_current_key_modifier(ev->modifiers);

     if (box)
     {
          if (box->is_visible == 0)
               return 0;

          if (box->handle_keys == 0)
               return 0;

          if (!box->is_active)
               return 0;

          if (ev->key_id == DIKI_TAB)
               return focus_traverse(window);

          return box->OnKeyDown ? box->OnKeyDown(box, ev) : 0;
     }

     return 0;
}

static int
handle_wheel(LiteWindow *window, DFBWindowEvent *ev)
{
     LiteBox *box = window->focused_box;

     D_ASSERT(window != NULL);
     D_ASSERT(ev != NULL);

     if (box)
     {
          if (box->is_visible == 0)
               return 0;

          if (box->handle_keys == 0)
               return 0;

          if (box->is_active == 0)
               return 0;

          return box->OnWheel ? box->OnWheel(box, ev) : 0;
     }

     return 0;
}

static LiteBox*
find_child(LiteBox *box, int *x, int *y)
{
     D_ASSERT(box != NULL);
     int i;

 search_again:

     if (box->catches_all_events)
          return box;

     for (i = box->n_children - 1; i >= 0; i--) {
          LiteBox *child = box->children[i];
          
          if (child->is_visible &&
              DFB_RECTANGLE_CONTAINS_POINT(&child->rect, *x, *y)) {
               *x -= child->rect.x;
               *y -= child->rect.y;
               box = child;
               goto search_again;
          }
     }
     
     return box;
}

static void
child_coords(LiteBox *box, int *x, int *y)
{
     D_ASSERT(box != NULL);

     int tx = *x, ty = *y;
     while (box) {
          tx -= box->rect.x;
          ty -= box->rect.y;
          box = box->parent;
     }
     *x = tx;
     *y = ty;
}

static LiteWindow*
find_window_by_id(DFBWindowID id)
{
     int n;

     for (n=0; n<num_windows_global; n++)
          if (window_array_global[n]->id == id)
               return window_array_global[n];

     return NULL;
}

static int
focus_traverse(LiteWindow *window)
{
     D_ASSERT(window != NULL);
     /* FIXME */
     return 0;
}

static void
add_window(LiteWindow *window)
{
     D_DEBUG_ENTER(LiteWindowDomain, "(window == %p)\n", window);

     D_ASSERT(window != NULL);

     num_windows_global++;
     window_array_global = D_REALLOC(window_array_global, sizeof(LiteWindow*) * num_windows_global);
     window_array_global[num_windows_global - 1] = window;
}

static void
remove_window(LiteWindow *window)
{
     D_DEBUG_ENTER(LiteWindowDomain, "(window == %p)\n", window);

     int n;

     D_ASSERT(window != NULL);

     for (n=0; n<num_windows_global; n++)
          if (window_array_global[n] == window)
               break;

     if (n == num_windows_global) {
          D_DEBUG_AT(LiteWindowDomain, "Window not found!\n");
          return;
     }

     num_windows_global--;

     for (; n < num_windows_global; n++)
          window_array_global[n] = window_array_global[n+1];

     window_array_global = D_REALLOC(window_array_global, sizeof(LiteWindow*) * num_windows_global);
}

static void
destroy_window_data(LiteWindow *window)
{
     D_DEBUG_ENTER(LiteWindowDomain, "(window == %p)\n", window);

     /* destroy all of the data associated with the window, except for the
      * IDFBWindow -> LiteWindow association in our LiTE hashtable.  If the
      * window needs cleanup code, it should have been set in the window->box->destroy.
      * window->OnDestroy is called when the DFB_DESTROYED event is seen, but the
      * contents of the LiteWindow are mostly invalid then. */

     window->flags |= LITE_WINDOW_DESTROYED;

     lite_window_set_modal(window, false);
     if (entered_window_global == window)
          entered_window_global = NULL;

     lite_release_window_drag_box(window);

     if (window->title) {
          D_FREE(window->title);
          window->title = NULL;
     }

     window->box.Destroy(&window->box);

     if (window->surface) {
          window->surface->Release(window->surface);
          window->surface = NULL;
     }

     if (window->window)
          window->window->Destroy(window->window);

     pthread_mutex_destroy( &window->updates.lock );
}

static int
handle_destroy(LiteWindow* window)
{
     D_DEBUG_ENTER(LiteWindowDomain, "(window == %p)\n", window);
     D_ASSERT(window != NULL);

     int ret = DFB_OK;

     /* if the window is processing events or an event loop, delay destruction */
     if (window->internal_ref_count > 0)
          return ret;

     if (window->OnDestroy)
          ret = window->OnDestroy(window);

     if (window->window) {
          IDirectFBEventBuffer *buffer = NULL;
          lite_get_event_buffer(&buffer);
          window->window->DetachEventBuffer(window->window, buffer);
          event_buffer_global->Release(event_buffer_global);

          window->window->Release(window->window);
          window->window = NULL;
     }

     remove_window(window);

     D_FREE( window );

     return ret;
}

static void
render_title(LiteWindow *window)
{
     int               string_width;
     IDirectFBSurface *surface;
     IDirectFBFont    *font_interface = NULL;
     LiteWindowTheme  *theme;
     LiteThemeFrame   *frame;
     DFBResult         result;

     D_ASSERT( window != NULL );

     surface = window->surface;
     D_ASSERT( surface != NULL );

     theme = window->theme;
     D_ASSERT( theme != NULL );

     frame = &theme->theme.frame;
     D_MAGIC_ASSERT( frame, LiteThemeFrame );

     if (theme->title_font != NULL) {
          result = lite_font( theme->title_font, &font_interface );
          if (result != DFB_OK)
               return;
     }

     surface->SetClip( surface, NULL );
     surface->SetRenderOptions( surface, DSRO_NONE );

     /* fill title bar background */
     surface->StretchBlit( surface, frame->parts[LITE_THEME_FRAME_PART_TOP].source,
                           &frame->parts[LITE_THEME_FRAME_PART_TOP].rect,
                           &window->frame_target.dest[LITE_THEME_FRAME_PART_TOP] );

     /* draw title */
     if (window->title && font_interface) {
          font_interface->GetStringWidth(font_interface, window->title, -1, &string_width);

          surface->SetColor(surface,
                            theme->title_color.r,
                            theme->title_color.g,
                            theme->title_color.b,
                            theme->title_color.a);
          surface->SetFont(surface, font_interface);

          font_interface->GetStringWidth(font_interface,
                                         window->title, -1,
                                         &string_width);


          int x = window->frame_target.dest[LITE_THEME_FRAME_PART_TOP].x;

          if (theme->title_x_offset == LITE_CENTER_HORIZONTALLY)
               x += (window->box.rect.w - string_width) / 2;
          else
               x += theme->title_x_offset;

          int y = window->frame_target.dest[LITE_THEME_FRAME_PART_TOP].y + theme->title_y_offset;

          surface->DrawString(surface, window->title, -1, x, y, DSTF_TOPLEFT);
     }
     else {
          string_width = 0;
     }

     window->min_width  = window->frame_target.dest[LITE_THEME_FRAME_PART_TOPLEFT].w + string_width +
                          window->frame_target.dest[LITE_THEME_FRAME_PART_TOPRIGHT].w;
     window->min_height = window->frame_target.dest[LITE_THEME_FRAME_PART_TOP].h +
                          window->frame_target.dest[LITE_THEME_FRAME_PART_BOTTOM].h;
}

static void
render_border(LiteWindow *window)
{
     int               i;
     IDirectFBSurface *surface;
     LiteWindowTheme  *theme;
     LiteThemeFrame   *frame;

     D_ASSERT( window != NULL );

     surface = window->surface;
     D_ASSERT( surface != NULL );

     theme = window->theme;
     D_ASSERT( theme != NULL );

     frame = &theme->theme.frame;
     D_MAGIC_ASSERT( frame, LiteThemeFrame );

     surface->SetClip( surface, NULL );
     surface->SetRenderOptions( surface, DSRO_NONE );

     /* Start with second item, excluding the title bar (special case) */
     for (i=LITE_THEME_FRAME_PART_BOTTOM; i<_LITE_THEME_FRAME_PART_NUM; i++) {
          surface->StretchBlit( surface, frame->parts[i].source,
                                &frame->parts[i].rect, &window->frame_target.dest[i] );
     }

     if(getenv("LITE_WINDOW_DOUBLEBUFFER"))
    	 surface->Flip(surface, NULL, DSFLIP_BLIT);
}

static DFBResult
draw_window(LiteBox         *box,
            const DFBRegion *region,
            DFBBoolean       clear)
{
     LiteWindow *window = LITE_WINDOW(box);

     D_ASSERT(box != NULL);

     if (window->bg.enabled) {
          IDirectFBSurface *surface = box->surface;

          surface->SetClip(surface, region);

          surface->Clear(surface,
                          window->bg.color.r,
                          window->bg.color.g,
                          window->bg.color.b,
                          window->bg.color.a);
     }

     return DFB_OK;
}

static DFBResult create_event_buffer(IDirectFBWindow *window)
{
     DFBResult ret = DFB_OK;

     /* first time we need to create the event buffer, second time we attach to it */
     if (!event_buffer_global) {
          ret = window->CreateEventBuffer(window, &event_buffer_global);
          if (ret) {
               DirectFBError("window_new: IDirectFBWindow::"
                              "CreateEventBuffer() failed", ret);
          }

          /* extra AddRef to keep this around until LiTE exits */
          event_buffer_global->AddRef(event_buffer_global);
     }
     else {
          event_buffer_global->AddRef(event_buffer_global);
          ret = window->AttachEventBuffer(window, event_buffer_global);
          if (ret) {
               DirectFBError("window_new: IDirectFBWindow::"
                              "AttachEventBuffer() failed", ret);
          }
     }

     return ret;
}

DFBResult
prvlite_load_default_window_theme(void)
{
     DFBResult ret = DFB_FAILURE;
     LiteWindowTheme *theme = NULL;
     LiteTheme *t = NULL;

     theme = D_CALLOC(1, sizeof(LiteWindowTheme));

     /* configure the default background color settings */
     t = LITE_THEME(theme);
     t->bg_color.r = DEFAULT_WINDOW_COLOR_R;
     t->bg_color.g = DEFAULT_WINDOW_COLOR_G;
     t->bg_color.b = DEFAULT_WINDOW_COLOR_B;
     t->bg_color.a = DEFAULT_WINDOW_COLOR_A;

     /* load title font */
     ret = lite_get_font(DEFAULT_WINDOW_TITLE_FONT, LITE_FONT_PLAIN, 16,
                              DEFAULT_FONT_ATTRIBUTE, &theme->title_font);
     if (ret != DFB_OK) {
          D_DEBUG_AT(LiteWindowDomain, "Could not load font 'DEFAULT_WINDOW_TITLE_FONT'!\n");
     }

     /* default titles are centered horizontally and black */
     theme->title_x_offset = LITE_CENTER_HORIZONTALLY;
     theme->title_y_offset = 6;
     theme->title_color.r = 0;
     theme->title_color.g = 0;
     theme->title_color.b = 0;
     theme->title_color.a = 0xFF;

     const char *filenames[] = {
          DATADIR "/" DEFAULT_WINDOW_TOP_FRAME,
          DATADIR "/" DEFAULT_WINDOW_BOTTOM_FRAME,
          DATADIR "/" DEFAULT_WINDOW_LEFT_FRAME,
          DATADIR "/" DEFAULT_WINDOW_RIGHT_FRAME,
          DATADIR "/" DEFAULT_WINDOW_TOP_LEFT_FRAME,
          DATADIR "/" DEFAULT_WINDOW_TOP_RIGHT_FRAME,
          DATADIR "/" DEFAULT_WINDOW_BOTTOM_LEFT_FRAME,
          DATADIR "/" DEFAULT_WINDOW_BOTTOM_RIGHT_FRAME
     };

     /* load the various default bitmap entries for the window frame */
     ret = lite_theme_frame_load( &theme->theme.frame, filenames );
     if (ret)
          goto error;

     theme->theme.flags |= LITE_THEME_HAS_FRAME;

     /* assign the theme to the global variable */
     liteDefaultWindowTheme = theme;

     return DFB_OK;


error:
     lite_free_window_theme(theme);

     return ret;
}

/* this is used when a LiTE app is being shutdown */
DFBResult
prvlite_release_window_resources(void)
{
     event_loop_nesting_level = 0;

     release_grabs();

     /* destroy all windows in most-recent-first order */
     int num_windows = num_windows_global;
     while (--num_windows >= 0) {

          /* if we deleted a number of windows in previous iteration,
           * we might have to reset counter */
          if (num_windows >= num_windows_global) {
               num_windows = num_windows_global;
               continue;
          }

          LiteWindow *window = window_array_global[num_windows];
          /* don't destroy already destroyed windows that haven't been cleared
           * and don't destroy a window with a creator, as the creator will be
           * responsible for destroying the window */
          if (window->creator == NULL) {
               window->internal_ref_count = 0;
               if (!(window->flags & LITE_WINDOW_DESTROYED))
                    destroy_window_data(window);
               handle_destroy(window);
          }
     }

     if (event_buffer_global) {
          event_buffer_global->Release(event_buffer_global);
          event_buffer_global = NULL;
     }

     pthread_mutex_destroy(&timeout_mutex);
     LiteWindowTimeout *timeout = timeout_queue;
     while (timeout) {
         LiteWindowTimeout *next = timeout->next;
         D_FREE(timeout);
         timeout = next;
     }

     pthread_mutex_destroy(&idle_mutex);
     LiteWindowIdle *idle = idle_queue;
     while (idle) {
         LiteWindowIdle *next = idle->next;
         D_FREE(idle);
         idle = next;
     }

     if (liteDefaultWindowTheme) {
         lite_free_window_theme(liteDefaultWindowTheme);
     }

     return DFB_OK;
}
