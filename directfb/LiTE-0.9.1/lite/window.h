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
 * @brief  This file contains definitions for the LiTE window interface.
 * @ingroup LiTE
 * @file window.h
 * <hr>
 */

#ifndef __LITE__WINDOW_H__
#define __LITE__WINDOW_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <direct/types.h>

#include <directfb.h>

#include <pthread.h>

#include "box.h"
#include "font.h"
#include "theme.h"

#define LITE_WINDOW_MAX_UPDATES    4

/* @brief Macro to convert a subclassed LiteWindow into a LiteWindow */
#define LITE_WINDOW(l) ((LiteWindow*)(l))

/* @brief LiteWindow blend modes */
typedef enum {
     LITE_BLEND_ALWAYS,                         /**< Always blend */
     LITE_BLEND_NEVER,                          /**< Never blend */
     LITE_BLEND_AUTO                            /**< Automatically blend */
} LiteBlendMode;

/* @brief Window flags */
typedef enum {
    LITE_WINDOW_MODAL           = 1 << 1L,      /**< Modal window */
    LITE_WINDOW_RESIZE          = 1 << 2L,      /**< Resizable window */
    LITE_WINDOW_MINIMIZE        = 1 << 3L,      /**< Window that could be minimized */
    LITE_WINDOW_HANDLING_EVENTS = 1 << 4L,      /**< @deprecated Flag replaced with internal_ref_count */
    LITE_WINDOW_DESTROYED       = 1 << 5L,      /**< Window is marked for destruction */
    LITE_WINDOW_FIXED           = 1 << 6L,      /**< Window can't be moved or resized */
    LITE_WINDOW_DRAWN           = 1 << 7L,      /**< Window has been rendered at least once */
    LITE_WINDOW_PENDING_RESIZE  = 1 << 8L,      /**< At least one resize event is pending */
    LITE_WINDOW_DISABLED        = 1 << 9L,      /**< Window does not respond to events */
    LITE_WINDOW_CONFIGURED      = 1 << 10L,     /**< Window received the initial pos/size event */
} LiteWindowFlags;

/* @brief Window alignment flags */
typedef enum {
     LITE_CENTER_HORIZONTALLY = -1,             /**< Center window on the x-axis */
     LITE_CENTER_VERTICALLY   = -2              /**< Center window on the y-axis */
}LiteAlignmentFlags;

/* @brief Full window opacity level (no alpha blending) */
#define liteFullWindowOpacity 0xff

/* @brief No window opacity (window invisible ) */
#define liteNoWindowOpacity 0x00


/* @brief LiteWindowTheme window theme structure */
typedef struct  _LiteWindowTheme {
     LiteTheme theme;                           /**< Base LiTE Theme */

     LiteFont *title_font;                      /**< Title font */
     DFBColor title_color;                      /**< Color for font */
     int title_x_offset;                        /**< X offset of title, LITE_CENTER_HORIZONTALLY to center */
     int title_y_offset;                        /**< Y offset of title */
} LiteWindowTheme;

/* @brief Standard window themes */
/* @brief No window theme */
#define          liteNoWindowTheme  NULL

/* @brief LiTE default window theme */
extern LiteWindowTheme *liteDefaultWindowTheme;



/* @brief Window mouse event callback.
 * This prototype is used for defining a callback for cases
 * where mouse event handling inside the window is intercepted.
 *
 * @param evt                   OUT:    Window event
 * @param data                  OUT:    Context data
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
typedef DFBResult (*LiteWindowMouseFunc) (DFBWindowEvent* evt, void *data);

/* @brief Window keyboard event callback.
 * This prototype is used for defining a callback for cases
 * where keyboard event handling inside the window is intercepted.
 *
 * @param evt                   OUT:    Window event
 * @param data                  OUT:    Context data
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
typedef DFBResult (*LiteWindowKeyboardFunc) (DFBWindowEvent* evt, void *data);

/* @brief Window scroll wheel event callback.
 * This prototype is used for defining a callback for cases
 * where mouse scroll event handling inside the window is intercepted.
 *
 * @param evt                   OUT:    Window event
 * @param data                  OUT:    Context data
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
typedef DFBResult (*LiteWindowWheelFunc) (DFBWindowEvent* evt, void *data);

/* @brief Window event callback.
 * This prototype is used for defining a callback for cases
 * where window events are intercepted before the toolbox has a
 * chance to process these.
 *
 * @param evt                   OUT:    Window event
 * @param data                  OUT:    Context data
 *
 * @return DFBResult            Returns DFB_OK if successful.
 *                              Return any other event to indicate that
 *                              the event is processed and should not be
 *                              processed by the toolbox.
 */
typedef DFBResult (*LiteWindowEventFunc) (DFBWindowEvent* evt, void *data);

/* @brief univesal event callback.
 * This prototype is used for defining a callback for cases
 * where universal events inside the window is intercepted.
 *
 * @param evt                   OUT:    Universal event
 * @param data                  OUT:    Context data
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
typedef DFBResult (*LiteWindowUniversalEventFunc) (DFBUniversalEvent* evt, void *data);

/* @brief Window user event callback.
 * This prototype is used for defining a callback for cases
 * where user events inside the window is intercepted.
 *
 * @param evt                   OUT:    User event
 * @param data                  OUT:    Context data
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
typedef DFBResult (*LiteWindowUserEventFunc) (DFBUserEvent* evt, void *data);

/* @brief LiteWindow structure */
typedef struct _LiteWindow {
  LiteBox           box;                        /**< Underlying LiteBox */
  struct _LiteWindow *creator;                  /**< Creator window */

  int               width;                      /**< Window width */
  int               height;                     /**< Window height */

  u8                opacity;                    /**< Window opacity */

  DFBWindowID       id;                         /**< Window ID */

  char             *title;                      /**< Window title */
  IDirectFBWindow  *window;                     /**< Underlying DirectFB window */
  IDirectFBSurface *surface;                    /**< Underlying DirectFB surface */

  LiteWindowFlags   flags;                      /**< Window flags */
  int               moving;                     /**< If the window could be moved or not */
  int               resizing;                   /**< If the window could be resized or not */
  int               old_x;                      /**< Original x position */
  int               old_y;                      /**< Original y position */

  int               step_x;                     /**< X steps */
  int               step_y;                     /**< Y steps */

  int               min_width;                  /**< Minimum width */
  int               min_height;                 /**< Minimum height */

  int               last_width;                 /**< Last width */
  int               last_height;                /**< Last height */

  DFBWindowEvent    last_resize;                /**< Last resize event */
  DFBWindowEvent    last_motion;                /**< Last motion event */

  struct timeval    last_click;                 /**< Last time window was clicked */

  int               has_focus;                  /**< Window has focus or not */

  LiteWindowMouseFunc   raw_mouse_func;         /**< Raw mouse callback */
  void                 *raw_mouse_data;         /**< Raw mouse callback data*/
  LiteWindowMouseFunc   raw_mouse_moved_func;   /**< Raw mouse move callback */
  void                 *raw_mouse_moved_data;   /**< Raw mouse move callback data */
  LiteWindowMouseFunc   mouse_func;             /**< Mouse callback */
  void                 *mouse_data;             /**< Mouse callback data */

  LiteWindowKeyboardFunc raw_keyboard_func;     /**< Raw keyboard callback */
  void                 *raw_keyboard_data;      /**< Raw keyboard callback data */
  LiteWindowKeyboardFunc keyboard_func;         /**< Keyboard callback */
  void                 *keyboard_data;          /**< Keyboard callback data */

  LiteWindowEventFunc   window_event_func;      /**< Window event callback */
  void                 *window_event_data;      /**< Window event callback data */

  LiteWindowUniversalEventFunc universal_event_func;   /**< Universal event callback */
  void                 *universal_event_data;   /**< Universal event callback data */

  LiteWindowUserEventFunc user_event_func;      /**< User event callback */
  void                 *user_event_data;        /**< User event callback data */

  LiteWindowWheelFunc   raw_wheel_func;         /**< Raw wheel callback */
  void                 *raw_wheel_data;         /**< Raw wheel callback data*/
  LiteWindowWheelFunc   wheel_func;             /**< Scroll wheel callback */
  void                 *wheel_data;             /**< Scroll wheel callback data */

  struct {
       pthread_mutex_t  lock;
       int              pending;
       DFBRegion        regions[LITE_WINDOW_MAX_UPDATES];
  } updates;                                    /**< Update areas */

  LiteBlendMode     content_mode;               /**< Content blend mode */
  LiteBlendMode     opacity_mode;               /**< Opacity blend mode */

  struct {
       DFBBoolean   enabled;
       DFBColor     color;
  } bg;                                         /**< Background color*/

  LiteBox          *entered_box;                /**< Currently entered box */
  LiteBox          *focused_box;                /**< Currently focused box */
  LiteBox          *drag_box;                   /**< Currently dragged box */

  LiteWindowTheme  *theme;                      /**< Window theme installed */
  LiteThemeFrameTarget frame_target;

  int (*OnMove)     (struct _LiteWindow *self, int x, int y);   /**< Move callback */
  int (*OnResize)   (struct _LiteWindow *self, int width, int height); /**< Resize callback */
  int (*OnClose)    (struct _LiteWindow *self);                 /**< Close callback */
  int (*OnDestroy)  (struct _LiteWindow *self);                 /**< Destroy callback -- called in response to
								     DFB_DESTROYED, most of the data structures
								     have already been freed at this time.  Override
								     the internal LiteBox's Destroy callback for most
								     destruction actions. */
  int (*OnFocusIn)  (struct _LiteWindow *self);                 /**< Focus in callback */
  int (*OnFocusOut) (struct _LiteWindow *self);                 /**< Focus out callback */
  int (*OnEnter)    (struct _LiteWindow *self, int x, int y);   /**< Enter callback */
  int (*OnLeave)    (struct _LiteWindow *self, int x, int y);   /**< Leave callback */
  int (*OnBoxAdded)   (struct _LiteWindow *self, LiteBox *box); /**< Box added callback */
  int (*OnBoxToBeRemoved) (struct _LiteWindow *self, LiteBox *box); /**< Box To be Removed callback */

  int               internal_ref_count;         /**< counts event loops and handlers active on window */
} LiteWindow;


/* @brief timeout callback.
 * This prototype is used for defining a callback used when
 * a timeout occurs in the window event loop or the loop goes
 * idle and there are idle callbacks registered.
 *
 * @param data                  OUT:    Context data
 *
 * @return DFBResult            Returns DFB_OK if successful,
 *                              DFB_TIMEOUT to exit the event loop
 */
typedef DFBResult (*LiteTimeoutFunc) (void *data);


/* @brief LiteWindowTimeout structure */
typedef struct _LiteWindowTimeout {
  long long timeout;                                            /**< milliseond time value that triggers this timeout */
  int id;                                                       /**< id value used to remove this timeout */
  LiteTimeoutFunc callback;                                     /**< callback called when timeout happens,
                                                                     NULL function pointer is allowed to force
                                                                     an exit of event loop */
  void *callback_data;                                          /**< context data passed to timeout callback */
  struct _LiteWindowTimeout *next;                              /**< pointer to next timeout in queue */
} LiteWindowTimeout;

/* @brief LiteWindowIdle structure */
typedef struct _LiteWindowIdle {
  int id;                                                       /**< id value used to remove this idle */
  LiteTimeoutFunc callback;                                     /**< callback called when event queue is idle
                                                                     NULL function pointer is allowed to force
                                                                     an exit of event loop */
  void *callback_data;                                          /**< context data passed to callback */
  struct _LiteWindowIdle *next;                                 /**< pointer to next idle in queue */
} LiteWindowIdle;

/* @brief Create a new LiteWindow object
 * This function will create a new LiteWindow object. You
 * could pass in the size, layer used, various capabilities
 * for the window (such as alpha blending), and a top part title
 * in case the window has a window theme installed.
 * You could install the default theme (liteDefaultWindowTheme),
 * no theme (liteNoWindowTheme), or your own custom window theme
 * based on the LiteWindowTheme structure.
 *
 * @param layer                 IN:     Window display layer, if NULL
                                        the default layer is used.
 * @param rect                  IN:     Window size
 * @param cap                   IN:     Window capabilities.
 * @param theme                 IN:     Window theme
 * @param title                 IN:     Top part window title
 * @param ret_window            OUT:    Valid window object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_new_window(IDirectFBDisplayLayer *layer,
                          DFBRectangle          *rect,
                          DFBWindowCapabilities  caps,
                          LiteWindowTheme       *theme,
                          const char            *title,
                          LiteWindow           **ret_window);

/* @brief Initialize a Window object
 * This function will initialize a window object. This object
 * is mostly called from specific windows subclassed from the
 * window objeect. lite_new_window() will call this function.
 *
 * @param win                   IN:     Window object
 * @param layer                 IN:     Layer used
 * @param win_rect              IN:     Window size
 * @param caps                  IN:     Window capabilities
 * @param theme                 IN:     Window theme
 * @param title                 IN:     Window title
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_init_window(LiteWindow            *win,
                           IDirectFBDisplayLayer *layer,
                           DFBRectangle          *win_rect,
                           DFBWindowCapabilities  caps,
                           LiteWindowTheme       *theme,
                           const char            *title);

/* @brief Set the window creator of a specific window
 * This function will store the original window that created a specific window.
 * This feature is needed in cases of modal windows that will open another LiteWindow,
 * if the window knows of the creator window, it will forward all events to the new
 * window created. In practice it means that all input events are forwarded to
 * a new window that is bigger than the original modal window.

 * @param window                IN:     The window that needs to know about its creator window
 * @param creator               IN:     The window that is creating the new LiteWindow.
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_window_set_creator(LiteWindow *window, LiteWindow *creator);

/* @brief Get the window creator of a specific window
 * This function will get the creator window of a specific window, or the
 * window that created a specific window.
 * It is assumed that an earlier lite_window_set_creator() has been called, if
 * not NULL will be returned.
 *
 * @param window                IN:     The window that needs to know about its creator window
 * @param creator               OUT:    The window that created a specific LiteWindow
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_window_get_creator(LiteWindow *window, LiteWindow **creator);

/* @brief Set the modal state of a window
 * This function will set the modal state of a window, if enabled
 * the window will be only window receiving events. If false
 * all created windows will receive events.
 *
 * @param win                   IN:     Valid window object
 * @param modal                 IN:     Modal state of the window
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_window_set_modal(LiteWindow *window, bool modal);

/* @brief Start the window event loop
 * This function start the event loop. You only need to do this
 * for one (main) window, all other windows will equally receive
 * events unless a window has been set into modal state.
 * If you pass in a timeout value the event loop will run until
 * the timeout has expired, and then return from the function. This
 * could be used for creating idle event handling in the actual
 * application.
 *
 * @param win                   IN:     Valid window object
 * @param timeout               IN:     Timeout value for the event loop
 *                                      0 means to not leave the event loop after timeout
 *                                      -1 means to run event loop once and return
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_window_event_loop(LiteWindow *window, int timeout);

/* @brief Check to see if an event is available
 * This function is used within an event handler, timeout handler, or
 * idle callback to see if there are any pending events or timeouts
 * ready to be called.
 *
 * @return DFBResult            Returns DFB_OK if there are events or timeouts
 *                              ready to be processed.  Returns DFB_BUFFEREMPTY
 *                              if there are no pending items.
 */
DFBResult lite_window_event_available(void);

/* @brief Enqueue a timeout callback
 * This function enqueues a timeout callback that will be called in
 * the future.  These callback functions are called from the window
 * event loop and are one-shot only, removed after the call.
 *
 * @param timeout               IN:     Time from now to active callback function
 * @param callback              IN:     Pointer to callback function to call after timeout.
 *                                      Can be NULL to cause event loop to exit at timeout
 *                                      instead.
 * @param callback_data         IN:     Context value to pass to callback
 * @param timeout_id            OUT:    Pointer to integer that holds ID of new timeout.
 *                                      The ID value will never be 0.
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_enqueue_window_timeout(int timeout, LiteTimeoutFunc callback,
                                      void *callback_data, int *timeout_id);

/* @brief Remove a timeout callback from the queue
 * This function removes a timeout callback that has been previously
 * added using lite_enqueue_window_timeout.  This is used to remove a
 * callback that is no longer needed before it has been called.
 *
 * @param timeout_id            IN:     ID of timeout item to remove
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_remove_window_timeout(int timeout_id);

/* @brief Adjust timeouts for time change
 *
 * @param adjustment      IN: adjustment in milliseconds to apply to all timeouts
 * @return DFBResult      Returns DFB_OK if successful
 */
DFBResult lite_rebase_window_timeouts(long long adjustment);

/* @brief Enqueue an idle callback
 * This function enqueues a idle callback that will be called in the
 * future.  These callback functions are called from the window event
 * loop and are one-shot only, removed after the call.  They are
 * called in a first-in, first-out order when the event queue is empty
 * and when there are no pending timeout callbacks.
 *
 * @param callback              IN:     Pointer to callback function to call on idle.
 *                                      Can be NULL to cause event loop to exit at idle
 *                                      instead.
 * @param callback_data         IN:     Context value to pass to callback
 * @param idle_id               OUT:    Pointer to integer that holds ID of new idle
 *                                      The ID value will never be 0.
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_enqueue_idle_callback(LiteTimeoutFunc callback, void *callback_data, int *idle_id);

/* @brief Remove an idle callback from the queue
 * This function removes an idle callback that has been previously
 * added using lite_enqueue_idle_callback.  This is used to remove a
 * callback that is no longer needed before it has been called.
 *
 * @param timeout_id            IN:     ID of idle callback to remove
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_remove_idle_callback(int idle_id);

/* @brief Update the window
 * This function will update the window in the specified region.
 * If NULL is passed as a region the whole window area will be updated.
 *
 * @param win                   IN:     Valid window object
 * @param region                IN:     Window region
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_update_window(LiteWindow *window, const DFBRegion *region);

/* @brief Update all windows
 * This function will update all windows.
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_update_all_windows(void);

/* @brief Set the window title
 * This function will set the window title provided the window
 * has a theme installed.
 *
 * @param win                   IN:     Valid window object
 * @param title                 IN:     Window title
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_window_title(LiteWindow *window, const char *title);

/* @brief Set the enabled/disable state of a window
 * This function will tell a window if it should handle events that
 * are sent to it.  DESTROYED events will always be handled.
 *
 * @param win                   IN:     Valid window object
 * @param enabled               IN:     If true, window handles events.
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_window_enabled(LiteWindow *window, bool enabled);

/* @brief Set the window opacity level
 * This function will set the window opacity level.
 * Note that after creating a window the opacity level is 0 (liteNoWindowOpacity),
 * so you need to set it to a higher value, such as liteFullWindowOpacity (0xff)
 * in order for the window to show up.
 *
 * @param win                   IN:     Valid window object
 * @param opacity               IN:     Opacity level, 0 to 0xff,
 *                                      liteNoWindowOpacity corresponds to 0,
 *                                      liteFullWindowOpacity corresponds to 0xff
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_window_opacity(LiteWindow *window, u8 opacity);

/* @brief Set the window background color
 * This function will set the window background color. Note
 * that each window has a default window color set in the
 * lite configuration file.
 *
 * @param win                   IN:     Valid window object
 * @param color                 IN:     Color specification
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_window_background(LiteWindow *window, const DFBColor *color);

/* @brief Set the window background color
 * This function will set the window background color. Note
 * that each window has a default window color set in the
 * lite configuration file.
 *
 * @param win                   IN:     Valid window object
 * @param r                     IN:     R color component
 * @param g                     IN:     G color component
 * @param b                     IN:     B color component
 * @param a                     IN:     Alpha component.
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_window_background_color(LiteWindow *window,
                                           u8          r,
                                           u8          g,
                                           u8          b,
                                           u8          a);

/* @brief Set the window blend mode
 * This function will set the window blend mode.
 *
 * @param win                   IN:     Valid window object
 * @param content               IN:     Window content blend mode
 * @param opacity               IN:     Opacity blend mode
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_set_window_blend_mode(LiteWindow   *window,
                                     LiteBlendMode content,
                                     LiteBlendMode opacity);

/* @brief Resize the window
 * This function will resize the window if the resize flag is
 * enabled. By default the resize flag is set.
 *
 * @param win                   IN:     Valid window object
 * @param width                 IN:     New window width
 * @param height                IN:     New window height
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_resize_window(LiteWindow  *window,
                             unsigned int width,
                             unsigned int height);

DFBResult lite_set_window_bounds(LiteWindow   *window,
                                 int           x,
                                 int           y,
                                 unsigned int  width,
                                 unsigned int  height);

/* @brief Get Window size
 * This function will get the current window size.
 *
 * @param win                   IN:     Valid window object
 * @param width                 OUT:    Window width
 * @param height                OUT:    Window height
 *
 * @return DFBResult            Returns DFB_OK if successful.
 *
 */
DFBResult lite_get_window_size(LiteWindow *window,
                               int        *ret_width,
                               int        *ret_height);

/* @brief Minimize the window
 * This function will minimze the window if the minimize flag
 * is set. By default the minimize flag is set.
 *
 * @param win                   IN:     Valid window object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_minimize_window(LiteWindow *window);

/* @brief Maximize the window
 * This function will maximize the window if the minimize flag
 * is set and the window is minimized. By default the minimize flag is set.
 *
 * @param win                   IN:     Valid window object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_restore_window(LiteWindow *window);

/* @brief Set focus to a specific LiteBox
 * This function will set the focus flag on a specified LiteBox in
 * the window.
 *
 * @param box                   IN:     LiteBox to be focused
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_focus_box(LiteBox *box);

/* @brief Remove drag box attribute of a window
 * This function will clear the LiteWindow's drag_box field and
 * release any cursor grabs taken to manage the drag state.  Any
 * further motion or mouse up events will be sent to the LiteBox under
 * the cursor, not the one on which the original mouse-down occured.
 *
 * @param win                   IN:     Valid window object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_release_window_drag_box(LiteWindow *window);

/* @brief Handle window events
 * This function will call the function to handle incoming window
 * events. It is called by default from the window event loop, and
 * is usually not needed unless you want to directly call window event
 * handling from external code.
 *
 * @param win                   IN:     Valid window object
 * @param event                 IN:     Valid window event
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
int lite_handle_window_event(LiteWindow *window, DFBWindowEvent *event);

/* @brief Flush window events
 * This function will flush any outstanding window events.
 *
 * @param win                   IN:     Valid window object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_flush_window_events(LiteWindow *window);

/* @brief Find window that a LiteBox belongs to
 * This function will find what window a specific LiteBox belongs to.
 *
 * @param box                   IN:     Valid LiteBox
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
LiteWindow* lite_find_my_window(LiteBox *box);

/* @brief Close window
 * This function will close a window. If this window is the last
 * window in the application, it will exit the event loop.
 *
 * @param win                   IN:     Valid window object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_close_window(LiteWindow *window);

/* @brief Destroy window
 * This function will close a window and destroy the window resources.
 * If this window is the last window in the application,
 * it will exit the event loop.
 *
 * @param win                   IN:     Valid window object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_destroy_window(LiteWindow *window);

/* @brief Destroy all windows
 * This function will destroy all of the windows in the system.
 * This should cause all of the pending event loops to exit.
 * 
 * @param win                   IN:     Valid window object
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_destroy_all_windows(void);

/* @brief Install a raw mouse event callback
 * This function will install a raw mouse event callback that
 * is triggered before the window has processed the mouse down or up event
 * and delegated it to a specific LiteBox. If DFB_OK is not returned
 * then the event handling for this mouse event is terminated.
 *
 * @param window                IN:     Valid window object
 * @param callback              IN:     Callback to be triggered
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_on_raw_window_mouse(LiteWindow         *window,
                                   LiteWindowMouseFunc callback,
                                   void               *data);

/* @brief Install a raw mouse move event callback
 * This function will install a raw mouse move event callback that
 * is triggered before the window has processed the mouse move event
 * and delegated it to a specific LiteBox. If DFB_OK is not returned
 * then the event handling for this mouse event is terminated.
 *
 * Note that the callback should not do extensive work as it will slow
 * down the user-perceived performance.
 *
 * @param window                IN:     Valid window object
 * @param callback              IN:     Callback to be triggered
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_on_raw_window_mouse_moved(LiteWindow         *window,
                                         LiteWindowMouseFunc callback,
                                         void               *data);

/* @brief Install a mouse event callback
 * This function will install a mouse event callback that
 * is triggered after the window has processed the mouse event
 * and delegated it to a specific LiteBox.
 *
 * @param window                IN:     Valid window object
 * @param callback              IN:     Callback to be triggered
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_on_window_mouse(LiteWindow         *window,
                               LiteWindowMouseFunc callback,
                               void               *data);

/* @brief Install a raw keyboard event callback
 * This function will install a raw keyboard event callback that
 * is triggered before the window has processed the keyboard event
 * and delegated it to a specific LiteBox. If DFB_OK is not returned
 * then the event handling for this keyboard event is terminated.
 *
 * @param window                IN:     Valid window object
 * @param callback              IN:     Callback to be triggered
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_on_raw_window_keyboard(LiteWindow            *window,
                                      LiteWindowKeyboardFunc callback,
                                      void                  *data);

/* @brief Install a keyboard event callback
 * This function will install a keyboard event callback that
 * is triggered after the window has processed the keyboard event
 * and delegated it to a specific LiteBox.
 *
 * @param window                IN:     Valid window object
 * @param callback              IN:     Callback to be triggered
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_on_window_keyboard(LiteWindow            *window,
                                  LiteWindowKeyboardFunc callback,
                                  void                  *data);

/* @brief Install a window event callback
 * This function will install a window event callback that
 * is triggered before the LiTE toolbox has processed the actual event.
 * This could be used to filter or otherwise override any default window
 * event processing. If the callback installs does return a value not
 * equal to DFB_OK, then it's assumed that this event should not be forwarded
 * to the toolbox.
 *
 * This function should be carefully used, as the callback will override all
 * default behavior of the toolbox.
 *
 * @param window                IN:     Valid window object
 * @param callback              IN:     Callback to be triggered
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_on_window_event(LiteWindow         *window,
                               LiteWindowEventFunc callback,
                               void               *data);

/* @brief Install a universal event callback
 * This function will install a universal event callback that
 * is triggered when the window is processing universal events
 * Note that LiTE is internally not using any universal events.
 *
 * @param window                IN:     Valid window object
 * @param callback              IN:     Callback to be triggered
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_on_window_universal_event(LiteWindow                   *window,
                                         LiteWindowUniversalEventFunc  callback,
                                         void                         *data);

/* @brief Install a user event callback
 * This function will install a user event callback that
 * is triggered when the window is processing custom user events
 * Note that LiTE is internally not using any user events.
 *
 * @param window                IN:     Valid window object
 * @param callback              IN:     Callback to be triggered
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_on_window_user_event(LiteWindow             *window,
                                    LiteWindowUserEventFunc callback,
                                    void                   *data);

/* @brief Install a scroll wheel event callback
 * This function will install a raw scroll wheel event callback that
 * is triggered before the window has processed scroll wheel events
 * and delegated them to specific LiteBox entries.  If DFB_OK is not
 * returned then the event handling for the wheel event is terminated.
 *
 * @param window                IN:     Valid window object
 * @param callback              IN:     Callback to be triggered
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_on_raw_window_wheel(LiteWindow         *window,
                                    LiteWindowWheelFunc callback,
                                    void               *data);

/* @brief Install a scroll wheel event callback
 * This function will install a scroll wheel event callback that
 * is triggered when the window is processing scroll wheel events
 *
 * @param window                IN:     Valid window object
 * @param callback              IN:     Callback to be triggered
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_on_window_wheel(LiteWindow         *window,
                               LiteWindowWheelFunc callback,
                               void               *data);

/* @brief Post custom events
 * This function will post events to the event queue.
 * Note that you could post any kinds of events to the queue,
 * including window events and user events.
 * Note that DirectFB Input events are not processed on this layer,
 * they have been converted to DFB Window events that enter the DirectFB
 * event queue.
 *
 * @param window                IN:     Valid window object
 * @param event                 IN:     Custom event
 * @param data                  IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_post_event_to_window(LiteWindow *window, DFBEvent *event);

/* @brief Release window resources
 * This function will release all window resources.
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_release_window_resources (void);

/* @brief Get access to the event queue
 * This function will return the event queue pointer.
 * This could be used in case you need to post or peek inside
 * the event queue outside the window code itself.
 *
 * @param ret_event_buffer      OUT:    Current event buffer
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_get_event_buffer(IDirectFBEventBuffer **ret_event_buffer);

/* @brief Free the window theme resources
 *
 * This function will release the window theme resources
 * including freeing the used memory in the various
 * DirectFB sub-surfaces.
 *
 * @param theme                 IN:     Window theme
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult
lite_free_window_theme(LiteWindowTheme *theme);


#ifdef __cplusplus
}
#endif

#endif /*  __LITE__WINDOW_H__  */
