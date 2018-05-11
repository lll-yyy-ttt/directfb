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

#include <leck/scrollbar.h>
#include <leck/textbutton.h>
#include <leck/label.h>

#define WINDOW_WIDTH          (300)
#define WINDOW_HEIGHT         (200)
#define SCROLLBAR_THICKNESS   (16)

LiteScrollbar *scrollbar_hori=0, *scrollbar_verti=0;

static int
on_window_resize(LiteWindow *window, int width, int height)
{
     DFBRectangle rect;
     
     rect.x = 0; rect.y = height-SCROLLBAR_THICKNESS-1;
     rect.w = width-SCROLLBAR_THICKNESS; rect.h = SCROLLBAR_THICKNESS;
     LITE_BOX(scrollbar_hori)->rect = rect;

     rect.x = width-SCROLLBAR_THICKNESS-1; rect.y = 0;
     rect.w = SCROLLBAR_THICKNESS; rect.h = height-SCROLLBAR_THICKNESS;
     LITE_BOX(scrollbar_verti)->rect = rect;

     lite_enable_scrollbar(scrollbar_verti, height < (WINDOW_HEIGHT*3/2) );

     lite_update_box(LITE_BOX(scrollbar_hori), NULL);
     lite_update_box(LITE_BOX(scrollbar_verti), NULL);

     return 1;
}

static void
button_pressed( LiteTextButton *button, void *ctx )
{
     lite_close_window( LITE_WINDOW(ctx) );
}


static void
scrollbar_updated( LiteScrollbar  *scrollbar,
                   LiteScrollInfo *info, 
                   void           *ctx)
{
     LiteLabel *label;
     char       buffer[128];
     LiteScrollInfo sinfo;

     if( ! info )
     {
          lite_get_scroll_info(scrollbar, &sinfo);
     }
     else
     {
          sinfo = *info;
     }

     label = LITE_LABEL(ctx);
     sprintf(buffer, "(%d,%d,%d),(%d,%d)", sinfo.min, sinfo.page_size,
             sinfo.max, sinfo.pos, sinfo.track_pos);
     lite_set_label_text(label, buffer);
}



int
main (int argc, char *argv[])
{
     LiteTextButton *button;
     LiteLabel      *label_info, *label_info2;
     LiteLabel      *label_hori_title, *label_hori;
     LiteLabel      *label_verti_title, *label_verti;
     LiteScrollInfo  sinfo;
     LiteWindow     *window;
     DFBRectangle    rect;
     DFBResult       res;

     /* initialize */
     if (lite_open( &argc, &argv ))
          return 1;

     /* create a window */
     rect.x = LITE_CENTER_HORIZONTALLY;
     rect.y = LITE_CENTER_VERTICALLY;
     rect.w = WINDOW_WIDTH;
     rect.h = WINDOW_HEIGHT;

     res = lite_new_window( NULL, 
                            &rect,
                            DWCAPS_ALPHACHANNEL, 
                            liteDefaultWindowTheme,
                            "Scrollbar Test",
                            &window );


     /* setup info label */
     rect.x = 20; rect.y = 50; rect.w = 210, rect.h = 17;
     res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 10, &label_info);
     lite_set_label_text( label_info, "(min,page_size,max),(pos,track_pos)");

     /* setup info label2 */
     rect.x = 20; rect.y = 70; rect.w = 310, rect.h = 17;
     res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 10, &label_info2);
     lite_set_label_text( label_info, "if(height>300) disable(vscroll);");

     /* setup the label_hori */
     rect.x = 10; rect.y = WINDOW_HEIGHT-50; rect.w = 150; rect.h = 20;
     res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 10, &label_hori);
     lite_set_label_alignment(label_hori, LITE_LABEL_LEFT);
     lite_set_label_text( label_hori, "" );

     rect.y += 17;
     res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 10, &label_hori_title);
     lite_set_label_alignment(label_hori_title, LITE_LABEL_LEFT);
     lite_set_label_text( label_hori_title, "Horizontal scrollbar");


     /* setup the label_verti */
     rect.x = WINDOW_WIDTH-150; rect.y = 10; rect.w = 130; rect.h = 20;
     res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 10, &label_verti);
     lite_set_label_alignment( label_verti, LITE_LABEL_RIGHT);
     lite_set_label_text( label_verti, "" );

     rect.y += 17;
     res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 10, &label_verti_title);
     lite_set_label_alignment(label_verti_title, LITE_LABEL_RIGHT);
     lite_set_label_text( label_verti_title, "Vertical scrollbar");



     /* setup the horizontal scrollbar */
//     if( !defaultScrollbarTheme )
//     {
          res = lite_new_scrollbar_theme(
               EXAMPLESDATADIR "/scrollbar.png", 3,
               &liteDefaultScrollbarTheme);
          if( res != DFB_OK )
               return res;
//     }
             

     rect.x = 0; rect.y = WINDOW_HEIGHT-SCROLLBAR_THICKNESS-1;
     rect.w = WINDOW_WIDTH-SCROLLBAR_THICKNESS; rect.h = SCROLLBAR_THICKNESS;
     res = lite_new_scrollbar(LITE_BOX(window), &rect,
               false, liteDefaultScrollbarTheme, &scrollbar_hori);
     lite_on_scrollbar_update(scrollbar_hori, scrollbar_updated, (void*)label_hori);

     /* setup the vertical scrollbar */
     rect.x = WINDOW_WIDTH-SCROLLBAR_THICKNESS-1; rect.y = 0;
     rect.w = SCROLLBAR_THICKNESS; rect.h = WINDOW_HEIGHT-SCROLLBAR_THICKNESS;
     res = lite_new_scrollbar(LITE_BOX(window), &rect,
               true, liteDefaultScrollbarTheme, &scrollbar_verti);
     sinfo.min = 30;
     sinfo.max = 100;
     sinfo.page_size = 10;
     sinfo.line_size = 1;
     sinfo.pos = 50;
     sinfo.track_pos = -1;
     lite_set_scroll_info(scrollbar_verti, &sinfo);
     lite_on_scrollbar_update(scrollbar_verti, scrollbar_updated, (void*)label_verti);


     /* set label caption with initial scrollbar state */
     scrollbar_updated(scrollbar_hori, NULL, label_hori);
     scrollbar_updated(scrollbar_verti, NULL, label_verti);


     /* setup the button */
     res = lite_new_text_button_theme(
          EXAMPLESDATADIR "/textbuttonbgnd.png",
          &liteDefaultTextButtonTheme);

     rect.x = 20; rect.y = 20; rect.w = 70; rect.h = 22;
     res = lite_new_text_button(LITE_BOX(window), &rect, "Exit", liteDefaultTextButtonTheme, &button);
     lite_on_text_button_press( button, button_pressed, window );

     /* setup window resize callback */
     window->OnResize = on_window_resize;   

     /* show the window */
     lite_set_window_opacity( window, liteFullWindowOpacity );

     /* run the default event loop */
     lite_window_event_loop( window, 0 );

     /* destroy the window with all this children and resources */
     lite_destroy_window( window );

     lite_destroy_scrollbar_theme(liteDefaultScrollbarTheme);
     lite_destroy_text_button_theme(liteDefaultTextButtonTheme);

     /* deinitialize */
     lite_close();

     return 0;
}
