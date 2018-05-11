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
 * @brief  This file contains definitions for the LiTE scrollbar structure.
 * @ingroup LiTE
 * @file list.h
 * <hr>
 **/

#ifndef __LITE__LIST_H__
#define __LITE__LIST_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <lite/box.h>
#include <lite/theme.h>

#include "scrollbar.h"

/* @brief Macro to convert a generic LiteBox into a LiteList */
#define LITE_LIST(l) ((LiteList*)(l))

/* @brief LiteList structure */
typedef struct _LiteList LiteList;

/* @brief List theme */
typedef struct _LiteListTheme {
     LiteTheme           theme;           /**< base LiTE theme */
     LiteScrollbarTheme *scrollbar_theme; /**< vertical scrollbar theme */
} LiteListTheme; 

/* @brief ListItemData type
 * It is a 4 byte numeric data type
 */
typedef unsigned long LiteListItemData;
                                   
/* @brief ListDrawItem struct */
typedef struct _ListDrawItem {
     int               index_item;
     LiteListItemData  item_data;
     IDirectFBSurface *surface;
     DFBRectangle      rc_item;
     int               selected;
     int               disabled;
} ListDrawItem;

/* @brief Standard list themes */
/* @brief No list theme */
#define liteNoListTheme       NULL

/* @brief default scrollbar theme */
extern LiteListTheme *liteDefaultListTheme;

/* @brief Callback function prototype for list selection change 
 * 
 * @param list                 IN:     Valid LiteList object
 * @param new_item             IN:     index of selection (-1 if none)
 * @param ctx                  IN:     Data context
 *
 * @return void
*/
typedef void (*ListSelChangeFunc) (LiteList *list,
                                   int       new_item,
                                   void     *ctx);

/* @brief Callback function prototype for list item drawing 
 * 
 * @param list                 IN:     Valid LiteList object
 * @param draw_item            IN:     item draw info
 * @param ctx                  IN:     Data context
 *
 * @return void
*/
typedef void (*ListDrawItemFunc) (LiteList     *list,
                                  ListDrawItem *draw_item,
                                  void         *ctx);


/* @brief Create a new LiteList object
 * This function will create a new LiteList object.
 *
 * @param parent                IN:     Valid parent LiteBox
 * @rect                        IN:     Rectangle coordinates for the scrollbar
 * @theme                       IN:     List theme. NULL is not allowed.
 * @param ret_list              OUT:    Returns a valid LiteList object
 * 
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_new_list(LiteBox        *parent,
                        DFBRectangle   *rect,
                        LiteListTheme  *theme,
                        LiteList      **ret_list);

/* @brief Set list's item row height
 * 
 * @param list                  IN:     Valid LiteList object
 * @parma row_height            IN:     New item row height
 *
 * @return DFBResult            Returns DFB_OK if successful
 */
DFBResult lite_list_set_row_height(LiteList *list,
                                    int      row_height);


/* @brief Get list's item row height
 *
 * @param list                  IN:     Valid LiteList object
 * @param row_height            OUT:    Returns current row height
 *
 * @return DFBResult            Returns DFB_OK if successful
 */
DFBResult lite_list_get_row_height(LiteList *list,
                                   int      *row_height);


/* @brief set the list enable/disable
 *
 * @param list                  IN:     Valid LiteList object
 * @param enable                IN:     1-enable, 0-disable
 *
 * @return DFBResult            Returns DFB_OK if successful
 */
DFBResult lite_enable_list(LiteList *list, int enable);


/* @brief Insert data item to list
 * If index is minus value or out of range, it is inserted at end. The list
 * doesn't care about what the data is. It treats data just as a 4 byte value.
 * The inserted data value will be passed to Draw Item Callback func.
 *
 * @param list                  IN:     Valid LiteList object
 * @param index                 IN:     Where to be inserted
 * @param data                  IN:     4 byte data value
 *
 * @return DFBResult            Returns DFB_OK if successful
 */
DFBResult lite_list_insert_item(LiteList        *list, 
                                int              index,
                                LiteListItemData data);


/* @brief Retrieve a data from a list
 *
 * @param list                  IN:     Valid LiteList object
 * @param index                 IN:     Target data index. Valid index required
 * @param data                  OUT:    Returns data
 *
 * @return DFBResult            Returns DFB_OK if successful
 */
DFBResult lite_list_get_item(LiteList         *list,
                             int               index,
                             LiteListItemData *ret_data);


/* @brief Replace a data item within a list
 *
 * @param list                  IN:     Valid LiteList object
 * @param index                 IN:     Target data index. Valid index required
 * @param data                  IN:     New data value
 *
 * @return DFBResult            Returns DFB_OK if successful
 */
DFBResult lite_list_set_item(LiteList *list,
                             int index,
                             LiteListItemData data);

/* @brief Delete a data item within a list
 *
 * @param list                  IN:     Valid LiteList object
 * @param index                 IN:     Target data index
 *
 * @return DFBResult            Returns DFB_OK if successful
 */
DFBResult lite_list_del_item(LiteList *list, int index);


/* @brief get total item count of the list
 *
 * @param list                  IN:     Valid LiteList object
 * @param count                 OUT:    total count
 *
 * @return DFBResult            Returns DFB_OK if successful
 */
DFBResult lite_list_get_item_count(LiteList *list, int *count);


/* @brief get the index of currently selected item
 *
 * @param list                  IN:     Valid LiteList object
 * @param index                 OUT:    current selection index
 *
 * @return DFBResult            Returns DFB_OK if successful
 */
DFBResult lite_list_get_selected_item_index(LiteList *list, int *index);

/* @brief select new item
 *
 * @param list                  IN:     Valid LiteList object
 * @param index                 OUT:    new selection index
 *
 * @return DFBResult            Returns DFB_OK if successful
 */
DFBResult lite_list_set_selected_item_index(LiteList *list, int index);


/* @brief Sort list items with qsort()
 *
 * @param list                  IN:     Valid LiteList object
 * @param compare_func          IN:     Comparison callback called by qsort()
 *
 * @return DFBResult            Returns DFB_OK if successful
 */
DFBResult lite_list_sort_items(LiteList *list,
          int (*compare)(const LiteListItemData*, const LiteListItemData*));

/* @brief Scroll list for an item to be visible
 *
 * @param list                  IN:     Valid LiteList object
 * @param index                 OUT:    target item index
 *
 * @return DFBResult            Returns DFB_OK if successful
 */
DFBResult lite_list_ensure_visible(LiteList *list, int index);


/* @brief Install the item drawing callback func
 *
 * @param list                  IN:     Valid LiteList object
 * @param callback              IN:     Valid callback function
 * @param ctx                   IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_list_on_draw_item(LiteList    *list,
                            ListDrawItemFunc  callback,
                            void             *ctx);


/* @brief Install the selection change callback
 * This function will install a selection change callback. This
 * callback is triggered every time the selected item is changed.
 * 
 * @param list                  IN:     Valid LiteList object
 * @param callback              IN:     Valid callback function
 * @param ctx                   IN:     Data context
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_list_on_sel_change(LiteList     *list,
                             ListSelChangeFunc  callback,
                             void              *ctx);


/* @brief Request to update the internal scrollbar
 * List's parent should call this func whenever resizing list.
 * The impl of list needs to know whether it has been resized, but LiTE's box
 * doesn't have any mechanism for that purpose.
 *
 * @param list                  IN:     Valid LiteList object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_list_recalc_layout(LiteList *list);


/* @brief Makes a list theme
 * This function makes a list theme that contains a scrollbar theme
 *
 * @param scrollbar_image_path  IN:     Image path for a scrollbar image
 * @param scrollbar_image_margin IN:    Boundary pixel size of scrollbar image
 * @param ret_theme             OUT:    Returns new theme object
 *
 * @return DFBResult            Returns DFB_OK if successful.
 */
DFBResult lite_new_list_theme(const char     *scrollbar_image_path,
                              int             scrollbar_image_margin,
                              LiteListTheme **ret_theme);


/* @brief Destroy a list theme
 * This function releases resources of Theme and free object memory
 *
 * @param theme                 IN:      theme to destroy
 *
 * @return DFBResult            Returns DFB_OK If successful.
 */
DFBResult lite_destroy_list_theme(LiteListTheme *theme);


#ifdef __cplusplus
}
#endif

#endif /*  __LITE__LIST_H__  */
