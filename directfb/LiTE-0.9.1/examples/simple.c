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

#include <leck/button.h>
#include <leck/image.h>
#include <leck/label.h>
#include <leck/textline.h>

static void
button_pressed( LiteButton *button, void *ctx )
{
     lite_close_window( LITE_WINDOW(ctx) );
}

static void
toggle_button_pressed( LiteButton *button, void *ctx )
{
}

int
main (int argc, char *argv[])
{
     LiteButton   *button;
     LiteImage    *image;
     LiteLabel    *label;
     LiteTextLine *textline;
     LiteWindow   *window;
     DFBRectangle  rect;
     DFBResult     res;

     /* initialize */
     if (lite_open( &argc, &argv ))
          return 1;

     /* create a window */
     rect.x = LITE_CENTER_HORIZONTALLY;
     rect.y = LITE_CENTER_VERTICALLY;
     rect.w = 300;
     rect.h = 200;

     res = lite_new_window( NULL, 
                            &rect,
                            DWCAPS_ALPHACHANNEL, 
                            liteDefaultWindowTheme,
                            "Simple",
                            &window );


     /* setup the label */
     rect.x = 10; rect.y = 10; rect.w = 110;
     res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 20, &label);

     lite_set_label_text( label, "Hello World" );

     /* setup the textline */
     rect.x = 10; rect.y = 40; rect.w = 100; rect.h = 20;
     res = lite_new_textline(LITE_BOX(window), &rect, liteNoTextLineTheme, &textline);
     
     rect.x = 10; rect.y = 60; rect.w = 100; rect.h = 20;
     res = lite_new_textline(LITE_BOX(window), &rect, liteNoTextLineTheme, &textline);

     /* setup the button */
     rect.x = 180; rect.y = 130; rect.w = 50; rect.h = 50;
     res = lite_new_button(LITE_BOX(window), &rect, liteNoButtonTheme, &button);

     lite_set_button_image( button, LITE_BS_NORMAL, EXAMPLESDATADIR "/stop.png" );
     lite_set_button_image( button, LITE_BS_DISABLED, EXAMPLESDATADIR "/stop_disabled.png" );
     lite_set_button_image( button, LITE_BS_HILITE, EXAMPLESDATADIR "/stop_highlighted.png" );
     lite_set_button_image( button, LITE_BS_PRESSED, EXAMPLESDATADIR "/stop_pressed.png" );

     lite_on_button_press( button, button_pressed, window );
     
     /* 2nd button */
     rect.x = 230; rect.y = 130; rect.w = 50; rect.h = 50;
     res = lite_new_button(LITE_BOX(window), &rect, liteNoButtonTheme, &button);
     lite_set_button_type( button, LITE_BT_TOGGLE );
 
     lite_set_button_image( button, LITE_BS_NORMAL, EXAMPLESDATADIR "/toggle.png" );
     lite_set_button_image( button, LITE_BS_DISABLED, EXAMPLESDATADIR "/toggle_disabled.png" );
     lite_set_button_image( button, LITE_BS_HILITE, EXAMPLESDATADIR "/toggle_highlighted.png" );
     lite_set_button_image( button, LITE_BS_PRESSED, EXAMPLESDATADIR "/toggle_pressed.png" );
     lite_set_button_image( button, LITE_BS_HILITE_ON, EXAMPLESDATADIR "/toggle_highlighted_on.png" );
     lite_set_button_image( button, LITE_BS_DISABLED_ON, EXAMPLESDATADIR "/toggle_disabled_on.png" );

     lite_on_button_press( button, toggle_button_pressed, window );

     /* setup the image */
     rect.x = 200; rect.y = 20; rect.w = 64; rect.h = 50;
     res = lite_new_image(LITE_BOX(window), &rect, liteNoImageTheme, &image);

     lite_load_image( image, EXAMPLESDATADIR "/D.png" );


     /* show the window */
     lite_set_window_opacity( window, liteFullWindowOpacity );

     /* run the default event loop */
     lite_window_event_loop( window, 0 );

     /* destroy the window with all this children and resources */
     lite_destroy_window( window );

     /* deinitialize */
     lite_close();

     return 0;
}
