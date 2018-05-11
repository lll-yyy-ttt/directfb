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

#include <direct/mem.h>

#include <lite/util.h>
#include <lite/window.h>

#include "scrollbar.h"
#include "list.h"

D_DEBUG_DOMAIN(LiteListDomain, "LiTE/List", "LiteList");

LiteListTheme *liteDefaultListTheme = NULL;

struct _LiteList {
     LiteBox                 box;
     LiteListTheme          *theme;
     LiteScrollbar          *scrollbar;
     ListSelChangeFunc       sel_change;
     ListDrawItemFunc        draw_item;
     void                   *sel_change_data;
     void                   *draw_item_data;
     int                     item_count;
     int                     cur_item_index;
     int                     disabled; 
     int                     row_height;
     LiteListItemData       *arr_data;
};

static int on_enter(LiteBox *box, int x, int y);
static int on_leave(LiteBox *box, int x, int y);
static int on_focus_in(LiteBox *box);
static int on_focus_out(LiteBox *box);
static int on_button_down(LiteBox *box, int x, int y,
                          DFBInputDeviceButtonIdentifier button);
static int on_button_up(LiteBox *box, int x, int y,
                          DFBInputDeviceButtonIdentifier button);
static int on_motion(LiteBox *box, int x, int y,
                          DFBInputDeviceButtonMask buttons);
static int on_key_down(LiteBox *box, DFBWindowEvent *evt);
static void on_vertical_scrollbar_update(LiteScrollbar* scrollbar,
                          LiteScrollInfo *info, void *ctx);
static DFBResult draw_list(LiteBox *box, const DFBRegion *region, DFBBoolean clear);
static DFBResult destroy_list(LiteBox *box);
static int
pt_in_rect( int x, int y, DFBRectangle *rc);
static int
list_hittest( LiteList *list, int x, int y);
static void
get_vertical_scrollbar_rect( LiteList *list, DFBRectangle *rect);
static int
list_needs_scroll( LiteList *list );
static void
list_update_scrollbar( LiteList *list );

int can_scroll( LiteScrollInfo *info); // scrollbar helper func.
                                       // defined at scrollbar.c file

DFBResult 
lite_new_list(LiteBox         *parent, 
           DFBRectangle       *rect,
           LiteListTheme      *theme,
           LiteList          **ret_list)
{
     DFBResult   res;
     DFBRectangle scrollbar_rect; 
     LiteList *list = NULL;

     LITE_NULL_PARAMETER_CHECK(parent);
     LITE_NULL_PARAMETER_CHECK(rect);
     LITE_NULL_PARAMETER_CHECK(theme);
     LITE_NULL_PARAMETER_CHECK(ret_list);

     list = D_CALLOC(1, sizeof(LiteList));

     list->box.parent = parent;
     if( !theme )
          theme = liteDefaultListTheme;
     list->theme = theme;
     list->box.rect = *rect;

     res = lite_init_box(LITE_BOX(list));
     if (res != DFB_OK) {
          D_FREE(list);
          return res;
     }
    
     scrollbar_rect.x = scrollbar_rect.y = 0;
     scrollbar_rect.w = scrollbar_rect.h = 10;
     get_vertical_scrollbar_rect(list, &scrollbar_rect);

     res = lite_new_scrollbar(
          LITE_BOX(list),
          &scrollbar_rect,
          true, // vertical
          theme->scrollbar_theme,
          &list->scrollbar);
     if(res != DFB_OK) {
          D_FREE(list);
          return res;
     }


     list->box.type    = LITE_TYPE_LIST;
     list->box.Draw    = draw_list;
     list->box.Destroy = destroy_list;

     list->box.OnEnter      = on_enter;
     list->box.OnLeave      = on_leave;
     list->box.OnFocusIn    = on_focus_in;
     list->box.OnFocusOut   = on_focus_out;
     list->box.OnButtonDown = on_button_down;
     list->box.OnButtonUp   = on_button_up;
     list->box.OnMotion     = on_motion;
     list->box.OnKeyDown    = on_key_down;
     
     list->sel_change           = NULL;
     list->draw_item            = NULL;
     list->sel_change_data      = NULL;
     list->draw_item_data       = NULL;
     list->item_count           = 0;
     list->cur_item_index       = -1;
     list->disabled             = 0; 
     list->row_height           = 20;
     list->arr_data             = NULL;

     lite_on_scrollbar_update(list->scrollbar, 
          on_vertical_scrollbar_update, (void*)list);

     lite_update_box(LITE_BOX(list), NULL);

     *ret_list = list;

     D_DEBUG_AT(LiteListDomain, "Created new list object: %p\n", list);

     return DFB_OK;
}

DFBResult
lite_list_set_row_height(LiteList* list, int row_height)
{
     LITE_NULL_PARAMETER_CHECK(list);
     
     if( row_height < 1 )
          return DFB_FAILURE;

     if( list->row_height != row_height )
     {
          list->row_height = row_height;
          lite_update_box(&list->box, NULL);
          list_update_scrollbar(list);
     }

     return DFB_OK;
}

DFBResult
lite_list_get_row_height(LiteList *list, int *row_height)
{
     LITE_NULL_PARAMETER_CHECK(list);
     LITE_NULL_PARAMETER_CHECK(row_height);

     *row_height = list->row_height;
     
     return DFB_OK;
}


DFBResult
lite_enable_list(LiteList *list, int enable)
{
     LITE_NULL_PARAMETER_CHECK(list);

     int curEnable = !list->disabled;
     
     if( curEnable != !(!enable) )
     {
          list->disabled = (!enable);
          list->box.is_active = !list->disabled;

          if( list->disabled )
               lite_enable_scrollbar(list->scrollbar, false);
          else
          {
               if( list_needs_scroll(list) )
                    lite_enable_scrollbar(list->scrollbar, true);
          }

          lite_update_box(&list->box, NULL);
     }
     
     return DFB_OK;
}

DFBResult lite_list_insert_item(LiteList        *list, 
                                int              index,
                                LiteListItemData data)
{
     LiteListItemData* new_arr;

     LITE_NULL_PARAMETER_CHECK(list);

     if( index < 0 || index >= list->item_count )
          index = list->item_count;

     new_arr = D_CALLOC(list->item_count+1, sizeof(LiteListItemData));
     new_arr[index] = data;

     if( list->arr_data )
     {
          if( index > 0 )
               memcpy(new_arr, list->arr_data, 
               index*sizeof(LiteListItemData)); 
          if( index < list->item_count )
               memcpy(new_arr+index+1, list->arr_data+index,
               (list->item_count-index)*sizeof(LiteListItemData));

          D_FREE(list->arr_data);
     }

     list->arr_data = new_arr;

     ++list->item_count;
     if( index <= list->cur_item_index )
          ++list->cur_item_index;

     list_update_scrollbar(list);
     lite_update_box(&list->box, NULL);

     return DFB_OK;
}

DFBResult lite_list_get_item(LiteList         *list,
                             int               index,
                             LiteListItemData *ret_data)
{
     LITE_NULL_PARAMETER_CHECK(list);
     if( index < 0 || index >= list->item_count )
          return DFB_INVARG;
     
     *ret_data = list->arr_data[index];
     return DFB_OK;
}

DFBResult lite_list_set_item(LiteList *list,
                             int index,
                             LiteListItemData data)
{
     LITE_NULL_PARAMETER_CHECK(list);
     if( index < 0 || index >= list->item_count )
          return DFB_INVARG;

     list->arr_data[index] = data;
 
     lite_update_box(&list->box, NULL);

     return DFB_OK;
}

DFBResult lite_list_del_item(LiteList *list, int index)
{
     
     LITE_NULL_PARAMETER_CHECK(list);
     if( index < 0 || index >= list->item_count )
          return DFB_INVARG;

     --list->item_count;

     if( list->item_count == 0 )
     {
          D_FREE( list->arr_data );
          list->arr_data = NULL;
          list->cur_item_index = -1;
     }
     else
     {
         // we don't re-allocate memory but
         // just shift data after index position

         memmove(list->arr_data+index, list->arr_data+index+1,
               sizeof(LiteListItemData) * (list->item_count-index) );

         if( list->cur_item_index != -1 )
         {
              if( index < list->cur_item_index || 
                    ( index == list->cur_item_index
                    && index == list->item_count ) )
              {
                    --list->cur_item_index;
                    if( list->cur_item_index == -1 )
                         list->cur_item_index = 0;
              }
         }
     }

     list_update_scrollbar(list);
     lite_update_box(&list->box, NULL);

     return DFB_OK;
}

DFBResult lite_list_sort_items(LiteList *list,
          int (*compare)(const LiteListItemData*, const LiteListItemData*))
{
     LITE_NULL_PARAMETER_CHECK(list);

     if( list->item_count < 2 )
          return DFB_OK;

     qsort( list->arr_data,
          list->item_count,
          sizeof( LiteListItemData ),
          (int (*)(const void*, const void*))compare);

     lite_update_box(&list->box, NULL);     

     return DFB_OK;           
}

DFBResult
lite_list_get_item_count(LiteList *list, int *count)
{
     LITE_NULL_PARAMETER_CHECK(list);

     if( !list || !count )
          return DFB_FAILURE;

     *count = list->item_count;
     return DFB_OK;
}

DFBResult
lite_list_get_selected_item_index(LiteList *list, int *index)
{
     LITE_NULL_PARAMETER_CHECK(list);

     if( !list || !index )
          return DFB_FAILURE;

     *index = list->cur_item_index;
     return DFB_OK;     
}

DFBResult
lite_list_set_selected_item_index(LiteList *list, int index)
{
     LITE_NULL_PARAMETER_CHECK(list);

     if( !list || index<0 || index>=list->item_count )
          return DFB_FAILURE;

     list->cur_item_index = index;

     lite_update_box(&list->box, NULL);
     lite_list_ensure_visible(list, index);

     return DFB_OK;
}

DFBResult
lite_list_ensure_visible(LiteList* list, int index)
{
     DFBRectangle      rcVirtualViewPort;
     LiteScrollInfo    info;
     int               y_view_port_center;
     int               y_item_center, y_item_bottom, y_item_top;

     LITE_NULL_PARAMETER_CHECK(list);

     if( !list || index<0 || index>=list->item_count )
          return DFB_FAILURE;

     if( list->row_height < 1 )
          return DFB_FAILURE;

     lite_get_scroll_info(list->scrollbar, &info);

     if( info.track_pos != -1 )
          return DFB_FAILURE;   // refuse to be called while dragging
     
     rcVirtualViewPort.y = info.track_pos == -1 ? info.pos :
          info.track_pos;
     rcVirtualViewPort.h = list->box.rect.h;
     rcVirtualViewPort.x = 0;
     rcVirtualViewPort.w = list->box.rect.w;

     y_view_port_center = rcVirtualViewPort.y + rcVirtualViewPort.h/2;

     y_item_bottom = list->row_height*index + list->row_height;
     y_item_top = list->row_height*index;
     y_item_center = (y_item_bottom + y_item_top)/2;

     if( y_item_center == y_view_port_center )
     {
          // ok. We're at exact center
     }
     else if( y_item_center < y_view_port_center )
     {
          if( y_item_top >= rcVirtualViewPort.y )
          {
               // ok. Visible item
          }
          else
          {
               info.pos = y_item_top;
               lite_set_scroll_info(list->scrollbar, &info);
               lite_update_box(&list->box, NULL);
          }
     }
     else // y_item_center > y_view_port_center
     {
          if( y_item_bottom <= rcVirtualViewPort.y+rcVirtualViewPort.h )
          {
               // ok. Visible item
          }
          else
          {
               info.pos = y_item_bottom - list->box.rect.h;
               lite_set_scroll_info(list->scrollbar, &info);
               lite_update_box(&list->box, NULL);
          }
     }

     return DFB_OK;
}


DFBResult 
lite_list_on_draw_item(LiteList       *list,
                      ListDrawItemFunc func,
                      void            *ctx)
{
     LITE_NULL_PARAMETER_CHECK(list);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(list), LITE_TYPE_LIST);

     list->draw_item      = func;
     list->draw_item_data = ctx;

     return DFB_OK;
}


DFBResult 
lite_list_on_sel_change(LiteList       *list,
                      ListSelChangeFunc func,
                      void            *ctx)
{
     LITE_NULL_PARAMETER_CHECK(list);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(list), LITE_TYPE_LIST);

     list->sel_change      = func;
     list->sel_change_data = ctx;

     return DFB_OK;
}

DFBResult
lite_list_recalc_layout(LiteList *list)
{
     DFBRectangle rcScrollbar = {0,0,10,10};
     LiteScrollInfo info;

     LITE_NULL_PARAMETER_CHECK(list);

     get_vertical_scrollbar_rect( list, &rcScrollbar );

     LITE_BOX(list->scrollbar)->rect = rcScrollbar;

     lite_get_scroll_info(list->scrollbar, &info);
     info.page_size = list->box.rect.h;
     if( info.page_size < 1 ) info.page_size = 1;
     lite_set_scroll_info(list->scrollbar, &info);

     lite_update_box(LITE_BOX(list->scrollbar), NULL);

     return DFB_OK;
}


DFBResult lite_new_list_theme(const char     *image_path,
                              int             image_margin,
                              LiteListTheme **ret_theme)
{
     DFBResult res;
     LiteListTheme *theme;

     LITE_NULL_PARAMETER_CHECK(image_path);
     LITE_NULL_PARAMETER_CHECK(ret_theme);

     theme = D_CALLOC(1, sizeof( LiteScrollbarTheme) );
     memset(theme, 0, sizeof( LiteScrollbarTheme) );

     res = lite_new_scrollbar_theme(image_path, image_margin,
          &theme->scrollbar_theme);
     if( res != DFB_OK )
          goto error1;

     // Do something special for list only
     res = DFB_OK;


     if( res != DFB_OK )
          goto error2;

     *ret_theme = theme;

     return DFB_OK;

error2:
     lite_destroy_scrollbar_theme(theme->scrollbar_theme);
     theme->scrollbar_theme = NULL;
error1:
     D_FREE(theme);

     return res;
}

DFBResult lite_destroy_list_theme(LiteListTheme *theme)
{
     LITE_NULL_PARAMETER_CHECK(theme);

     theme->scrollbar_theme->all_images.surface->Release(
          theme->scrollbar_theme->all_images.surface);

     D_FREE( theme );

     return DFB_OK;
}


/* internals */


static DFBResult 
destroy_list(LiteBox *box)
{
     LiteList *list = LITE_LIST(box);

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     D_DEBUG_AT(LiteListDomain, "Destroy List: %p\n", list);

     if (!list)
          return DFB_FAILURE;

     if( list->arr_data )
          D_FREE(list->arr_data);

     return lite_destroy_box(box);
}

static DFBResult 
draw_list(LiteBox        *box, 
         const DFBRegion *region, 
         DFBBoolean       clear)
{
     IDirectFBSurface *surface = box->surface;
     LiteList         *list = LITE_LIST(box);
     int               scrollbar_thickness;
     int               i;
     LiteScrollInfo    scroll_info;
     DFBRectangle      rcScrollbar;
     DFBRectangle      rcItemVirtual, rcItemClient;
     DFBRectangle      rcVirtualViewPort;
     ListDrawItem      ldi;

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     surface->SetClip(surface, region);
     scrollbar_thickness = list->theme->scrollbar_theme->all_images.width/8;

     // Fill the background
     if (clear)
          lite_clear_box(box, region);

     surface->SetDrawingFlags (surface, DSDRAW_NOFX);

     if( !list->draw_item || (list->item_count<1) )
          return DFB_OK;


     get_vertical_scrollbar_rect(list, &rcScrollbar);
     lite_get_scroll_info(list->scrollbar, &scroll_info);

     rcVirtualViewPort.y = scroll_info.track_pos == -1 ? scroll_info.pos :
          scroll_info.track_pos;
     rcVirtualViewPort.h = list->box.rect.h;
     rcVirtualViewPort.x = 0;
     rcVirtualViewPort.w = list->box.rect.w;

     rcItemVirtual.x = 0;
     rcItemVirtual.w = list->box.rect.w - rcScrollbar.w;
     rcItemVirtual.h = list->row_height;

     for(i = 0; i < list->item_count; ++i)
     {
          rcItemVirtual.y = i * list->row_height;

          if(pt_in_rect(rcItemVirtual.x,rcItemVirtual.y,&rcVirtualViewPort)
               || pt_in_rect(rcItemVirtual.x,rcItemVirtual.y+rcItemVirtual.h,
                    &rcVirtualViewPort) )
          {
               rcItemClient = rcItemVirtual;
               rcItemClient.y -= rcVirtualViewPort.y;
               
               ldi.index_item = i;
               ldi.item_data = list->arr_data[i];
               ldi.surface = surface;
               ldi.rc_item = rcItemClient;
               ldi.selected = (i == list->cur_item_index);
               ldi.disabled = list->disabled;

               list->draw_item(list, &ldi, list->draw_item_data);
          }    
     }

     return DFB_OK;
}


static int 
on_enter(LiteBox *box, int x, int y)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     return 1;
}

static int
on_leave(LiteBox *box, int x, int y)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);
     
     return 1;
}

static int 
on_focus_in(LiteBox *box)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     lite_update_box(box, NULL);

     return 1;
}

static int 
on_focus_out(LiteBox *box)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     lite_update_box(box, NULL);

     return 1;
}

static int 
on_button_down(LiteBox                       *box, 
                 int                            x, 
                 int                            y,
                 DFBInputDeviceButtonIdentifier button)
{
     LiteList *list = LITE_LIST(box);
     int cur_selected, new_selected;

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);
     
     lite_focus_box(box);

     cur_selected = list->cur_item_index;
     new_selected = list_hittest(list, x, y);

     list->cur_item_index = new_selected;
     lite_update_box(box, NULL);

     if( cur_selected != new_selected )
     {
          if( list->sel_change )
               list->sel_change(list, new_selected, list->sel_change_data);
     }

     return 1;
}



static int 
on_button_up(LiteBox                       *box, 
             int                            x, 
             int                            y,
             DFBInputDeviceButtonIdentifier button)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     return 1;
}


static int 
on_motion(LiteBox                 *box, 
          int                      x, 
          int                      y,
          DFBInputDeviceButtonMask buttons)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     return 1;
}


static int
on_key_down(LiteBox *box, DFBWindowEvent *evt)
{
     LiteList *list = LITE_LIST(box);
     int cur_selected, new_selected;

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_LIST);

     D_DEBUG_AT(LiteListDomain,
          "on_key_down() called. key_symbol : %d\n", evt->key_symbol);

     if( evt->key_symbol != DIKS_CURSOR_UP
          && evt->key_symbol != DIKS_CURSOR_DOWN
          && evt->key_symbol != DIKS_PAGE_UP
          && evt->key_symbol != DIKS_PAGE_DOWN )
          return 1;

     if( list->item_count < 1 || list->row_height < 1)
          return 1;

     new_selected = cur_selected = list->cur_item_index;

     if( list->cur_item_index == -1)
          new_selected = 0;
     else
     {

          if( evt->key_symbol == DIKS_CURSOR_UP )
          {
               new_selected = cur_selected - 1;
          }
          else if( evt->key_symbol == DIKS_CURSOR_DOWN )
          {
               new_selected = cur_selected + 1;
          }
          else if( evt->key_symbol == DIKS_PAGE_UP )
          {
               new_selected = cur_selected
                    - list->box.rect.h/(list->row_height);
          }
          else if( evt->key_symbol == DIKS_PAGE_DOWN )
          {
               new_selected = cur_selected
                    + list->box.rect.h/(list->row_height);
          }
     }

     if( new_selected < 0 )
          new_selected = 0;
     if( new_selected >= list->item_count )
          new_selected = list->item_count-1;

     if( new_selected != cur_selected )
     {
          lite_list_set_selected_item_index(list, new_selected);
     }     

     return 1;
}

static void
on_vertical_scrollbar_update(LiteScrollbar  *scrollbar,
                             LiteScrollInfo *info,
                             void           *ctx)
{
     LiteList *list = LITE_LIST(ctx);

     lite_update_box(&list->box, NULL);
}

static int
pt_in_rect( int x, int y, DFBRectangle *rc)
{
     // return 1 if (x,y) is in the rc rectangle
     if( !rc ) return 0;
     if( x < rc->x ) return 0;
     if( y < rc->y ) return 0;
     if( x > rc->x+rc->w ) return 0;
     if( y > rc->y+rc->h ) return 0;
     return 1;
}


void get_vertical_scrollbar_rect( LiteList *list, DFBRectangle *rect)
{
     int scrollbar_thickness;
     DFBRectangle rc;

     scrollbar_thickness = list->theme->scrollbar_theme->all_images.width / 8;

     rc = list->box.rect;
     rc.x = rc.w - scrollbar_thickness;
     if( rc.x < 0 ) rc.x = 0;
     rc.w = scrollbar_thickness;
     if( rc.w > list->box.rect.w )
          rc.w = list->box.rect.w;
     rc.y = 0;

     *rect = rc;
}

int list_needs_scroll( LiteList *list )
{
     LiteScrollInfo info;
 
     lite_get_scroll_info(list->scrollbar, &info);

     return can_scroll(&info);
}

int list_hittest( LiteList *list, int x, int y)
{
     int scrollpos = 0;
     int sel;
     DFBResult res;

     if( list->item_count < 1 || list->row_height < 1)
          return -1;

     res = lite_get_scroll_pos(list->scrollbar, &scrollpos);
     if( DFB_OK != res )
          return -1;

     sel =  (y+scrollpos)/list->row_height;
     if( sel < 0 )
          sel = 0;
     if( sel >= list->item_count )
          sel = list->item_count-1;

     return sel;
}

void list_update_scrollbar( LiteList *list )
{
     int total_height;
     LiteScrollInfo info, newInfo;

     if( list->row_height < 1 )
          return;

     lite_get_scroll_info(list->scrollbar, &info);

     total_height = list->row_height * list->item_count;

     newInfo.min = 0;
     newInfo.max = list->row_height * list->item_count;
     newInfo.page_size = list->box.rect.h;
     newInfo.line_size = list->row_height;
     newInfo.pos = info.pos;
     newInfo.track_pos = info.track_pos;
     // range checking will be done by lite_set_scroll_info()

     lite_set_scroll_info(list->scrollbar, &newInfo);

}
