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
 * @brief  This file contains definitions for the LiTE LiteBox structure.
 * @ingroup LiTE
 * @file box.h
 * <hr>
 */

#ifndef __LITE__BOX_H__
#define __LITE__BOX_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <directfb.h>

/* @brief Macro to convert a subclassed structure into a generic LiteBox
 * This macro is handy for converting any Lite object that is based on LiteBox
   to a generic LiteBox structure.
 */
#define LITE_BOX(l) ((LiteBox*)(l))

/* @brief LiteBox Type
 * This defines the type of the LiteBox.
 * All LiteWindow types are in the range 0x1000-0x7FFF, while valid
 * LiteBox types start at 0x8000 and continue through the integer
 * range.  The first 0x1000 items of each range are reserved for
 * use by the LiTE library for basic types.
 */
typedef enum {
  LITE_TYPE_WINDOW      = 0x1000,            /**< LiteWindow type */
  LITE_TYPE_WINDOW_USER = 0x2000,            /**< Start of user-defined LiteWindow types */
  LITE_TYPE_BOX         = 0x8000,            /**< LiteBox type */
  LITE_TYPE_BUTTON      = 0x8001,            /**< LiteButton type */
  LITE_TYPE_ANIMATION   = 0x8002,            /**< LiteAnimation type */
  LITE_TYPE_IMAGE       = 0x8003,            /**< LiteImage type */
  LITE_TYPE_LABEL       = 0x8004,            /**< LiteLabel type */
  LITE_TYPE_SLIDER      = 0x8005,            /**< LiteSlider type */
  LITE_TYPE_TEXTLINE    = 0x8006,            /**< LiteTextLine type */
  LITE_TYPE_PROGRESSBAR = 0x8007,            /**< LiteProgressBar type */
  LITE_TYPE_TEXT_BUTTON = 0x8008,            /**< LiteTextButton type */  
  LITE_TYPE_CHECK       = 0x8009,            /**< LiteCheck type */
  LITE_TYPE_SCROLLBAR   = 0x800A,            /**< LiteScrollbar type */
  LITE_TYPE_LIST        = 0x800B,            /**< LiteList type */
  LITE_TYPE_BOX_USER    = 0x9000,            /**< Start of user-defined LiteBox types */
  LITE_TYPE_HANTEXTLINE = 0xF000,            /**< LiteHanTextLine type(KOREAN specialized textline) */
} LiteBoxType;

/* @brief LiteBox structure
 * The LiteBox is the most common data structure in the LiTE framework.
 * It is used to build more complex widgets and compound structures.
 * Important event handling such as mouse and keyboard event handling
 * is handled in the various callbacks belonging to LiteBox.
 */
typedef struct _LiteBox {
  struct _LiteBox   *parent;            /**< Parent of the LiteBox */

  int                n_children;        /**< Num children in the child array */
  struct _LiteBox  **children;          /**< Child array */

  LiteBoxType        type;              /**< Type of LiteBox */

  DFBRectangle       rect;              /**< Rectangle of the LiteBox */
  IDirectFBSurface  *surface;           /**< LiteBox surface */

  DFBColor          *background;

  void              *user_data;         /**< User-provided data */

  int                is_focused;        /**< Indicates if the LiteBox is focused or not */
  int                is_visible;        /**< Indicates if the LiteBox is visible or not */
  int                is_active;         /**< Indicates if the LiteBox receives input or not */
  int                catches_all_events;/**< Indicates if the LiteBox prevents events from being handled by its children */
  int                handle_keys;       /**< Indicate if the LiteBox handles keyboard events */

  int  (*OnFocusIn)   (struct _LiteBox *self);                  /**< Focus-in callback */
  int  (*OnFocusOut)  (struct _LiteBox *self);                  /**< Focus-out callback  */

  int  (*OnEnter)     (struct _LiteBox *self, int x, int y);    /**< Enter callback */
  int  (*OnLeave)     (struct _LiteBox *self, int x, int y);    /**< Leave callback */
  int  (*OnMotion)    (struct _LiteBox *self, int x, int y,     /**< Motion callback */
                       DFBInputDeviceButtonMask buttons);
  int  (*OnButtonDown)(struct _LiteBox *self, int x, int y,
                       DFBInputDeviceButtonIdentifier button);  /**< Button down callback */
  int  (*OnButtonUp)  (struct _LiteBox *self, int x, int y,     /**< Button up callback */
                       DFBInputDeviceButtonIdentifier button);
  int  (*OnKeyDown)   (struct _LiteBox *self, DFBWindowEvent *evt); /**< Key down callback */
  int  (*OnKeyUp)     (struct _LiteBox *self, DFBWindowEvent *evt); /**< Key up callback */
  int  (*OnWheel)     (struct _LiteBox *self, DFBWindowEvent *evt); /**< Scroll wheel callback*/

  DFBResult (*Draw)      (struct _LiteBox *self, const DFBRegion *region, DFBBoolean clear); /**< Draw callback */
  DFBResult (*DrawAfter) (struct _LiteBox *self, const DFBRegion *region); /**< DrawAfter callback */
  DFBResult (*Destroy)   (struct _LiteBox *self);                  /**< Destroy callback */
} LiteBox;

/* @brief Create a new LiteBox
 * This function will create a new LiteBox.
 *
 * @param ret_box               OUT:    Valid LiteBox object
 * @param parent                IN:     Parent of a LiteBox object
 * @param x                     IN:     X coordinate of the LiteBox
 * @param y                     IN:     Y coordinate of the LiteBox
 * @param width                 IN:     Width of the LiteBox
 * @param height                IN:     Heigth of the LiteBox
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_new_box(LiteBox **ret_box,
                       LiteBox  *parent,
                       int       x,
                       int       y,
                       int       width,
                       int       height);

/* @brief Draw the contents of a LiteBox
 * This function will draw the contents of a LiteBox region. This is an immediate
 * draw operation. Use this operation in case you want immediate updates in the LiteBox,
 * however it's more costly than setting a dirty flag using lite_update_box() and handling
 * all dirty LiteBoxes in one single update later.
 *
 * @param box                   IN:     Valid LiteBox
 * @param                       IN:     Region of the LiteBox to be drawn
 * @param                       IN:     Indicate if a DirectFB Flip instruction should be done
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_draw_box(LiteBox         *box,
                        const DFBRegion *region,
                        DFBBoolean       flip);

/* @brief Update the LiteBox
 * This will update the contents of a LiteBox region by setting a dirty flag and later
 * in the event loop all LiteBox entries with a dirty flag will be updated. Use this
 * drawing operation in most cases, as it's not as costly as lite_draw_box.
 *
 * @param box                   IN:     Valid LiteBox
 * @param region                IN:     Region of the LiteBox to be drawn
 *
 * @return DFBResult    Returns DFB_OK if successful.
 */
DFBResult lite_update_box(LiteBox* box, const DFBRegion *region);

/* @brief Destroy a LiteBox
 *
 * This function will destroy a LiteBox, deallocate the memory and so
 * on.  The default Destroy action of a LiteBox points to this
 * function.  Usually, you do not call this function directly, instead
 * you invoke the LiteBox's Destroy method, allowing the box to
 * override the behavior as needed to do its own cleanup.
 *
 * @param box                   IN:     Valid LiteBox
 *
 * @return DFBResult    Returns DFB_OK if sucessful.
 */
DFBResult lite_destroy_box(LiteBox *box);

/* @brief Destroy a LiteBox
 *
 * This function will destroy a LiteBox and all of its children.
 * However, it does not free the memory used by the LiteBox, making it
 * suitable for calling on a LiteBox that's embedded in another data
 * structure.
 * 
 * @param box                   IN:     Valid LiteBox
 * 
 * @return DFBResult    Returns DFB_OK if sucessful.
 */
DFBResult lite_destroy_box_contents(LiteBox *box);

/* @brief Initialize a LiteBox
 * This function will initialize a LiteBox. The difference
 * between lite_new_box() and this function is that this init
 * function assumes lite_new_box() is called so that memory for the LiteBox
 * is allocated. Many LiteBox objects need this separation in order to
 * build their own structure information on top of the LiteBox structure.
 *
 * @param box                   IN:     Valid LiteBox
 *
 * @return DFBResult    Returns DFB_OK if successful.
 */
DFBResult lite_init_box(LiteBox *box);

/* @brief Initialize a LiteBox with additional parameters
 * This function will initialize a LiteBox. The difference
 * between lite_init_box() and this function is that this init
 * function sets the required box values (x,y,w,h,parent) itself.
 * That's much more convenient and avoids the same code ever again.
 *
 * @param box                   IN:     Valid LiteBox
 *
 * @return DFBResult    Returns DFB_OK if successful.
 */
DFBResult lite_init_box_at(LiteBox            *box,
                           LiteBox            *parent,
                           const DFBRectangle *rect );

/* @brief Reinitialize the LiteBox and its children
 * This function will reinitialize LiteBox and all of the LiteBox
 * child entries.
 *
 * @param box                   IN:     Valid LiteBox
 *
 * @return DFBResult    Returns DFB_OK if successful.
 */
DFBResult lite_reinit_box_and_children(LiteBox *box);

/* @brief Clear the contents of a LiteBox
 * This function will clear the contents of a LiteBox region including
 * updating the parent area.
 *
 * @param box                   IN:     Valid LiteBox
 * @param region                IN:     LiteBox region
 *
 * @return DFBResult    Returns DFB_OK if successful.
 */
DFBResult lite_clear_box(LiteBox *box, const DFBRegion *region);

/* @brief Add a LiteBox child into the parent array
 * This function will add a LiteBox child into the parent child
 * array. It could be used for infusing another layer of litebox entries
 * into an existing set of a parent/child hierarchy.
 *
 * @param parent                IN:     Valid LiteBox
 * @param child                 IN:     Valid LiteBox
 *
 * @return DFBResult    Returns DFB_OK if successful.
 */
DFBResult lite_add_child(LiteBox *parent, LiteBox *child);

/* @brief Remove a child from the parent array
 * This function will remove a LiteBox from a parent child array.
 * It could be used for changing the layer hierarchy of parents and children,
 * by removing a specific child.
 *
 * @param parent                IN:     Valid LiteBox
 * @param child                 IN:     Valid LiteBox
 *
 * @return DFBResult    Returns DFB_OK if successful.
 */
DFBResult lite_remove_child(LiteBox *parent, LiteBox *child);

/* change the visiblity of the box. Default: visible */
DFBResult lite_set_box_visible(LiteBox *box,
                               bool     visible);

#ifdef __cplusplus
}
#endif

#endif /*  __LITE__BOX_H__  */
