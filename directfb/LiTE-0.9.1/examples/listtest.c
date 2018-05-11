/*
   (c) Copyright 2001-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2005  convergence GmbH.

   Written by Denis Oliver Kropp <dok@directfb.org>
              
   This file is subject to the terms and conditions of the MIT License:

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software,
   and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <config.h>

#include <stddef.h>

#include <lite/lite.h>
#include <lite/window.h>
#include <lite/util.h>

#include <leck/textbutton.h>
#include <leck/list.h>
#include <leck/check.h>
#include <string.h>

#define MAIN_WINDOW_WIDTH (350)
#define MAIN_WINDOW_HEIGHT (400)
#define LIST_POS_X (0)
#define LIST_POS_Y (40)

LiteList *list = 0;
int prev_number = 0;

static void
button_exit_pressed( LiteTextButton *button, void *ctx )
{
     lite_close_window( LITE_WINDOW(ctx) );
}

static void
button_add_pressed( LiteTextButton *button, void *ctx )
{
     int count = 0;
     lite_list_get_item_count( list, &count);

     lite_list_insert_item(list, count, (LiteListItemData)++prev_number);
}

static void
button_del_pressed( LiteTextButton *button, void *ctx )
{
     int count=0, cur=0;
     lite_list_get_item_count( list, &count);
    
     if( count == 0 )
          return;

     lite_list_get_selected_item_index(list, &cur);

     lite_list_del_item(list, cur);
      
}

static void
button_scroll_pressed( LiteTextButton *button, void *ctx)
{
     int cur_item = -1;
     lite_list_get_selected_item_index( list, &cur_item );

     lite_list_ensure_visible( list, cur_item );
}

static int
CmpFunc(const LiteListItemData* p1, const LiteListItemData* p2)
{
     // Descending sort comparison func
     // LiteListItemData : 4byte unsigned integer

     return (long)*p2 - (long)*p1;
}

static void
button_sort_pressed( LiteTextButton *button, void *ctx)
{
     int count = 0;
     lite_list_get_item_count(list, &count);

     if( count > 0 )
     {
          lite_list_sort_items( list, CmpFunc );
          lite_list_set_selected_item_index(list, 0);
          lite_list_ensure_visible (list, 0);
     }
}

static void
check_enable_pressed( LiteCheck *check, LiteCheckState new_state, void *ctx)
{
     lite_enable_list( list, (int)new_state );
}

static int
on_window_resize(LiteWindow *window, int width, int height)
{
     DFBRectangle rect;

     if( list )
     {
          rect.x = LIST_POS_X;
          rect.y = LIST_POS_Y;
          rect.w = width - LIST_POS_X;
          rect.h = height - LIST_POS_Y;

          LITE_BOX(list)->rect = rect;
          lite_list_recalc_layout(list);

     }
     return 1;
}

void on_list_draw_item(LiteList *l, ListDrawItem *draw_item, void *ctx)
{
#define  COLOR_COUNT	(3)

     static int arr_color[COLOR_COUNT * 4] =
     { 255, 0, 0, 255,  // red
       0, 255, 0, 255,  // green
       0, 0, 255, 255   // blue
     };
     char buf[64];
     sprintf(buf, "%d", (int)draw_item->item_data);

     draw_item->surface->SetColor(
          draw_item->surface, 
          arr_color[(draw_item->item_data%COLOR_COUNT)*4],
          arr_color[(draw_item->item_data%COLOR_COUNT)*4+1],
          arr_color[(draw_item->item_data%COLOR_COUNT)*4+2],
          arr_color[(draw_item->item_data%COLOR_COUNT)*4+3]);

     draw_item->surface->FillRectangle(
          draw_item->surface,
          draw_item->rc_item.x,
          draw_item->rc_item.y,
          draw_item->rc_item.w,
          draw_item->rc_item.h);

     if( draw_item->disabled )
          draw_item->surface->SetColor(
                    draw_item->surface,
                    100,100,100,255);
     else
          draw_item->surface->SetColor(
                    draw_item->surface,
                    0,0,0,255);

     draw_item->surface->DrawString(
          draw_item->surface,
          buf,
          strlen(buf),
          draw_item->rc_item.x+2,
          draw_item->rc_item.y+2,
          DSTF_LEFT|DSTF_TOP);

     if( draw_item->selected )
     {
          draw_item->surface->SetColor(
               draw_item->surface,
               0,0,0,255);
          draw_item->surface->DrawRectangle(
               draw_item->surface,
               draw_item->rc_item.x+1,
               draw_item->rc_item.y+1,
               draw_item->rc_item.w-2,
               draw_item->rc_item.h-2);
     }
}

static void
list_item_changed(LiteList *list, int new_item, void *ctx)
{
     printf("list item changed : %d\n", new_item);
}

int
main (int argc, char *argv[])
{
     LiteTextButton *button, *buttonNewItem, *buttonDelItem, *buttonScrollTo;
     LiteTextButton *buttonSort;
     LiteCheck      *check;
     LiteWindow     *window;
     DFBRectangle    rect;
     DFBResult       res;
     LiteFont       *font;
     IDirectFBFont  *font_interface;
     int             i, width, height;

     /* initialize */
     if (lite_open( &argc, &argv ))
          return 1;

     lite_get_layer_size( &width, &height );

     lite_get_font("default", LITE_FONT_PLAIN, 16, DEFAULT_FONT_ATTRIBUTE, &font);

     lite_font(font, &font_interface);

     /* create a window */
     rect.x = LITE_CENTER_HORIZONTALLY;
     rect.y = LITE_CENTER_VERTICALLY;

     if (width < MAIN_WINDOW_WIDTH + 23 || height < MAIN_WINDOW_HEIGHT + 42) {
          rect.w = width;
          rect.h = height;

          res = lite_new_window( NULL, 
                                 &rect,
                                 DWCAPS_NONE, 
                                 liteNoWindowTheme,
                                 "List Test",
                                 &window );
     }
     else {
          rect.w = width  = MAIN_WINDOW_WIDTH;
          rect.h = height = MAIN_WINDOW_HEIGHT;

          res = lite_new_window( NULL, 
                                 &rect,
                                 DWCAPS_ALPHACHANNEL, 
                                 liteDefaultWindowTheme,
                                 "List Test",
                                 &window );
     }

     if( res != DFB_OK )
          return res;

     res = lite_new_text_button_theme(
          EXAMPLESDATADIR "/textbuttonbgnd.png",
          &liteDefaultTextButtonTheme);

     if( res != DFB_OK )
          return res;

     /* setup buttons */
     rect.x = 10; rect.y = 10; rect.w = 45; rect.h = 22;
     res = lite_new_text_button(LITE_BOX(window), &rect, "Exit", liteDefaultTextButtonTheme, &button);

     lite_on_text_button_press( button, button_exit_pressed, window );

     rect.x = 60; rect.y = 10; rect.w = 45; rect.h = 22;
     res = lite_new_text_button(LITE_BOX(window), &rect, "Add", liteDefaultTextButtonTheme, &buttonNewItem);

     lite_on_text_button_press( buttonNewItem, button_add_pressed, window );
     
     rect.x = 110; rect.y = 10; rect.w = 45; rect.h = 22;
     res = lite_new_text_button(LITE_BOX(window), &rect, "Del", liteDefaultTextButtonTheme, &buttonDelItem);

     lite_on_text_button_press( buttonDelItem, button_del_pressed, window );

     rect.x = 160; rect.y = 10; rect.w = 60; rect.h = 22;
     res = lite_new_text_button(LITE_BOX(window), &rect, "ScrollTo", liteDefaultTextButtonTheme, &buttonScrollTo);

     lite_on_text_button_press( buttonScrollTo, button_scroll_pressed, window );


     rect.x = 225; rect.y = 10; rect.w = 45; rect.h = 22;
     res = lite_new_text_button(LITE_BOX(window), &rect, "Sort", liteDefaultTextButtonTheme, &buttonSort);
     
     lite_on_text_button_press( buttonSort, button_sort_pressed, window );

     /* setup list */
     res = lite_new_list_theme(EXAMPLESDATADIR "/scrollbar.png", 3,
          &liteDefaultListTheme);
     rect.x = LIST_POS_X;
     rect.y = LIST_POS_Y;
     rect.w = width - LIST_POS_X;
     rect.h = height - LIST_POS_Y;
     res = lite_new_list(LITE_BOX(window), &rect, liteDefaultListTheme, &list);
     lite_list_set_row_height(list, 50);

     (LITE_BOX(list))->surface->SetFont(LITE_BOX(list)->surface, font_interface);

     lite_list_on_draw_item(list, on_list_draw_item, 0);
     lite_list_on_sel_change(list, list_item_changed, 0);

     for(i=0; i<1; ++i)
          button_add_pressed(NULL, NULL);

     /* setup check */
     res = lite_new_check_theme(
         EXAMPLESDATADIR "/checkbox_images.png",
         &liteDefaultCheckTheme);
     rect.x = 275; rect.y = 10; rect.w = 70; rect.h = 22;
     res = lite_new_check(LITE_BOX(window), &rect, "Enable",
         liteDefaultCheckTheme, &check);
     lite_check_check(check, LITE_CHS_CHECKED);
     lite_on_check_press(check, check_enable_pressed, 0);


     /* show the window */
     lite_set_window_opacity( window, liteFullWindowOpacity );

     /* setup window resize callback */
     window->OnResize = on_window_resize;   

     /* run the default event loop */
     lite_window_event_loop( window, 0 );

     /* destroy the window with all this children and resources */
     lite_destroy_window( window );

     lite_destroy_list_theme(liteDefaultListTheme);
     lite_destroy_text_button_theme(liteDefaultTextButtonTheme);
     lite_release_font(font);
     lite_destroy_check_theme(liteDefaultCheckTheme);

     /* deinitialize */
     lite_close();

     return 0;
}
