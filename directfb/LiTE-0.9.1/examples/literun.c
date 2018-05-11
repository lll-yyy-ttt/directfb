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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <lite/lite.h>
#include <lite/window.h>

#include <leck/button.h>
#include <leck/image.h>
#include <leck/label.h>
#include <leck/textline.h>

static LiteWindow *window;

static void
on_enter( const char *text, void *ctx )
{
     int   i   = 0;
     int   len = strlen(text);
     char *buf, *p, *args[len+1];

     if (len < 1)
          return;

     buf = p = strdup( text );


     while (*p) {
          if (*p == ' ') {
               *p++ = 0;
               continue;
          }

          args[i++] = p++;

          while (*p && *p != ' ')
               p++;
     }

     if (i < 1)
          return;

     /* destroy the window with all this children and resources */
     lite_destroy_window( window );

     /* deinitialize */
     lite_close();

     args[i] = NULL;

     execvp( args[0], args );
     perror( "execvp" );

     exit(-1);
}

static void
on_abort( void *ctx )
{
     window->window->Close( window->window );
}

int
main (int argc, char *argv[])
{
     LiteLabel    *label;
     LiteTextLine *textline;
     DFBRectangle  rect;
     DFBResult     res;

     /* initialize */
     if (lite_open( &argc, &argv ))
          return 1;

     /* create a window */
     rect.x = LITE_CENTER_HORIZONTALLY;
     rect.y = LITE_CENTER_VERTICALLY,
     rect.w = 300;
     rect.h = 40;

     res = lite_new_window( NULL, 
                            &rect,
                            DWCAPS_ALPHACHANNEL, 
                            liteDefaultWindowTheme,
                            "Run program...",
                            &window );
     
     
     /* setup the label */
     rect.x = 10; rect.y = 12; rect.w = 60;
     res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 14, &label );
     
     lite_set_label_text( label, "Program" );
     
     /* setup the textline */
     rect.x = 70; rect.y = 9; rect.w = 220; rect.h = 22;
     res = lite_new_textline(LITE_BOX(window), &rect, liteNoTextLineTheme, &textline);

     lite_on_textline_enter( textline, on_enter, NULL );
     lite_on_textline_abort( textline, on_abort, NULL );

     lite_focus_box( LITE_BOX(textline) );
     
     /* show the window */
     lite_set_window_opacity( window, liteFullWindowOpacity );

     window->window->RequestFocus( window->window );

     /* run the default event loop */
     lite_window_event_loop( window, 0 );

     /* destroy the window with all this children and resources */
     lite_destroy_window( window );

     /* deinitialize */
     lite_close();

     return 0;
}
