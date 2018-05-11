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
#include <leck/check.h>

static void
button_pressed( LiteTextButton *button, void *ctx )
{
     lite_close_window( LITE_WINDOW(ctx) );
}

static void
on_check1_pressed( LiteCheck *check, LiteCheckState state, void *ctx)
{
     LiteCheck* check2 = LITE_CHECK(ctx);

     lite_enable_check(check2, state==LITE_CHS_CHECKED ? 0:1);
}

static void
on_check2_pressed( LiteCheck *check, LiteCheckState state, void *ctx)
{
     LiteTextButton* buttonExit = LITE_TEXTBUTTON(ctx);

     lite_enable_text_button(buttonExit, state==LITE_CHS_CHECKED ? 0:1);
}

int
main (int argc, char *argv[])
{
     LiteTextButton *button;
     LiteTextButtonTheme *textButtonTheme;
     LiteCheck      *check1, *check2;
     LiteWindow     *window;
     DFBRectangle    rect;
     DFBResult       res;

     /* initialize */
     if (lite_open( &argc, &argv ))
          return 1;

     /* create a window */
     rect.x = LITE_CENTER_HORIZONTALLY;
     rect.y = LITE_CENTER_VERTICALLY;
     rect.w = 260;
     rect.h = 160;

     res = lite_new_window( NULL, 
                            &rect,
                            DWCAPS_ALPHACHANNEL, 
                            liteDefaultWindowTheme,
                            "Check Test",
                            &window );

     if( res != DFB_OK )
          return res;

     res = lite_new_text_button_theme(
          EXAMPLESDATADIR "/textbuttonbgnd.png",
          &textButtonTheme);

     if( res != DFB_OK )
          return res;


     res = lite_new_check_theme(
          EXAMPLESDATADIR "/checkbox_images.png",
          &liteDefaultCheckTheme);

     if( res != DFB_OK )
          return res;


     /* setup the exit button */
     rect.x = 30; rect.y = 110; rect.w = 86; rect.h = 22;
     res = lite_new_text_button(LITE_BOX(window), &rect, "Exit", textButtonTheme, &button);

     lite_on_text_button_press( button, button_pressed, window );

     /* setup checks */
     rect.x = 30; rect.y = 30; rect.w = 150; rect.h = 20;
     res = lite_new_check(LITE_BOX(window), &rect, "Disable check2",
          liteDefaultCheckTheme, &check1);

     rect.x = 50; rect.y = 70; rect.w = 187; rect.h = 20;
     res = lite_new_check(LITE_BOX(window), &rect,
          "Check2 - Disable Exit button", liteDefaultCheckTheme, &check2);

     res = lite_set_check_caption(check2, "Check2 - Disable exit button");

     lite_on_check_press(check1, on_check1_pressed, (void*)check2);
     lite_on_check_press(check2, on_check2_pressed, (void*)button);

     /* show the window */
     lite_set_window_opacity( window, liteFullWindowOpacity );

     /* run the default event loop */
     lite_window_event_loop( window, 0 );

     /* destroy the window with all this children and resources */
     lite_destroy_window( window );

     lite_destroy_text_button_theme(textButtonTheme);
     lite_destroy_check_theme(liteDefaultCheckTheme);


     /* deinitialize */
     lite_close();

     return 0;
}
