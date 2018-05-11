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

#include <stdio.h>

#include <directfb.h>

#include <lite/lite.h>
#include <lite/window.h>

#include <leck/button.h>
#include <leck/image.h>
#include <leck/label.h>


static void
button_pressed( LiteButton *button, void *ctx )
{
     lite_close_window( LITE_WINDOW(ctx) );
}

int
main (int argc, char *argv[])
{
     LiteButton   *button;
     LiteImage    *image;
     LiteLabel    *label;
     LiteWindow   *window;
     DFBRectangle  rect;
     DFBResult     res;

     if (DirectFBInit( &argc, &argv ))
          return 1;

     if (argc < 3) {
          printf( "\n  Usage: msgbox <title> <text>\n\n" );
          return 2;
     }

     if (lite_open( &argc, &argv ))
          return 3;

     rect.x = LITE_CENTER_HORIZONTALLY;
     rect.y = LITE_CENTER_VERTICALLY;
     rect.w = 300;
     rect.h = 100;
     
     res = lite_new_window( NULL, 
                            &rect,
                            DWCAPS_ALPHACHANNEL, 
                            liteDefaultWindowTheme,
                            argv[1],
                            &window );


     /* setup image */
     rect.x = 7; rect.y = 3; rect.w = 64; rect.h = 50;
     res = lite_new_image(LITE_BOX(window), &rect, liteNoImageTheme, &image);

     lite_load_image( image, EXAMPLESDATADIR "/D.png" );

     /* setup label */
     rect.x = 80; rect.y = 15; rect.w = 200;
     res = lite_new_label(LITE_BOX(window),&rect, liteNoLabelTheme, 14, &label );

     lite_set_label_text( label, argv[2] );

     /* setup button */
     rect.x = 240; rect.y = 43; rect.w = 50; rect.h = 50;
     res = lite_new_button(LITE_BOX(window), &rect, liteNoButtonTheme, &button);

     lite_set_button_image( button, LITE_BS_NORMAL, EXAMPLESDATADIR "/stop.png" );
     lite_set_button_image( button, LITE_BS_DISABLED, EXAMPLESDATADIR "/stop_disabled.png" );
     lite_set_button_image( button, LITE_BS_HILITE, EXAMPLESDATADIR "/stop_highlighted.png" );
     lite_set_button_image( button, LITE_BS_PRESSED, EXAMPLESDATADIR "/stop_pressed.png" );

     lite_on_button_press( button, button_pressed, window );



     lite_set_window_opacity( window, liteFullWindowOpacity );

     lite_window_event_loop( window, 0 );

     lite_destroy_window( window );

     lite_close();

     return 0;
}
