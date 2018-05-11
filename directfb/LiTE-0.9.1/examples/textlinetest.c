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

#include <leck/textbutton.h>
#include <leck/label.h>
#include <leck/textline.h>

static void
button_pressed( LiteTextButton *button, void *ctx )
{
     lite_close_window( LITE_WINDOW(ctx) );
}

int
main (int argc, char *argv[])
{
     LiteTextButton   *button;
     LiteLabel    *label,*label2;
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
                            "Textline Test",
                            &window );

     res = lite_new_text_button_theme(
          EXAMPLESDATADIR "/textbuttonbgnd.png",
          &liteDefaultTextButtonTheme);


     /* setup the label */
     rect.x = 10; rect.y = 10; rect.w = 300;
     res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 15, &label);
     lite_set_label_text( label, "Cursor moves by mouse click." );

     rect.x = 10; rect.y = 40; rect.w = 300;
     res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 15, &label2);
     lite_set_label_text( label2, "Horizontal auto scrolling" );

     /* setup the textline */
     rect.x = 10; rect.y = 70; rect.w = 280; rect.h = 30;
     res = lite_new_textline(LITE_BOX(window), &rect, liteNoTextLineTheme, &textline);
     lite_focus_box(LITE_BOX(textline));

     /* setup the button */
     rect.x = 10; rect.y = 110; rect.w = 50; rect.h = 22;
     res = lite_new_text_button(LITE_BOX(window), &rect, "Exit", liteDefaultTextButtonTheme, &button);

     lite_on_text_button_press( button, button_pressed, window );

     /* show the window */
     lite_set_window_opacity( window, liteFullWindowOpacity );

     /* run the default event loop */
     lite_window_event_loop( window, 0 );

     /* destroy the window with all this children and resources */
     lite_destroy_window( window );

     lite_destroy_text_button_theme(liteDefaultTextButtonTheme);     

     /* deinitialize */
     lite_close();

     return 0;
}
