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

#include <lite/lite.h>
#include <lite/window.h>

#include <leck/label.h>
#include <leck/slider.h>

static LiteLabel  *label[4];
static LiteSlider *slider[4];
static LiteWindow *window;

static u8 color[4] = { 0xc0, 0xc0, 0xc0, 0xf0 };

static void
slider_update( LiteSlider *slider, float pos, void *ctx )
{
     color[(unsigned long)ctx] = (u8)(255 * pos);

     lite_set_window_background_color( lite_find_my_window( LITE_BOX(slider) ),
                                       color[0], color[1], color[2], color[3] );
}

static const char *channels[4] = { "Red", "Green", "Blue", "Alpha" };

static int
on_resize (LiteWindow *window, int width, int height)
{
     if (width > 80) {
          int i;

          for (i=0; i<4; i++)
               LITE_BOX(slider[i])->rect.w = width - 80;
     }

     return 0;
}

int
main (int argc, char *argv[])
{
     int        i;
     DFBRectangle rect;
     DFBResult  res;

     /* initialize */
     if (lite_open( &argc, &argv ))
          return 1;

     /* create a window */
     rect.x = LITE_CENTER_HORIZONTALLY;
     rect.y = LITE_CENTER_VERTICALLY;
     rect.w = 300;
     rect.h = 115;

     res = lite_new_window( NULL,
                            &rect,
                            DWCAPS_ALPHACHANNEL,
                            liteDefaultWindowTheme,
                            "Slider Test",
                            &window );

     window->OnResize = on_resize;

     lite_set_window_background_color( window,
                                       color[0], color[1], color[2], 0xee );

     /* setup the labels */
     for (i=0; i<4; i++) {
          rect.x = 10 - (i==1?1:0);
          rect.y =  10 + i * 25;
          rect.w =  55;
          res = lite_new_label( LITE_BOX(window), &rect, liteNoLabelTheme, 18, &label[i]);

          lite_set_label_text( label[i], channels[i] );
     }

     /* setup the sliders */
     for (i=0; i<4; i++) {
          rect.x = 70;
          rect.y = 10 + i * 25;
          rect.w = 220;
          rect.h = 20;
          res = lite_new_slider(LITE_BOX(window), &rect, liteNoSliderTheme, &slider[i] );

          lite_set_slider_pos( slider[i], color[i] / 255.0f );

          lite_on_slider_update( slider[i], slider_update, (void*) (long)i );
     }


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
