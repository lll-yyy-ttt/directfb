/*
   (c) Copyright 2005  Kent Sandvik

   Written by Kent Sandvik <kent@directfb.org>
              
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
#include <stdio.h>
#include <string.h>

#include <direct/clock.h>

#include <lite/lite.h>
#include <lite/window.h>

#include <leck/button.h>
#include <leck/image.h>
#include <leck/label.h>
#include <leck/textline.h>

#include "testutils.h"

#define NUM_LABELS  10
#define TIMEOUT 1000

int main (int argc, char *argv[])
{
     LiteWindow *window;
     LiteLabel *label; 
     DFBRectangle rect;
     int timeout = 0;

     if (argc > 1) {
        if ( (strcmp(argv[1], "timeout") == 0) )
            timeout = TIMEOUT;
     } 

     ltest_start_timer("Init LiTE");
     if( lite_open(&argc, &argv) )
          return 1;
     ltest_stop_timer();
    
     rect.x = 0; rect.y = 0; rect.w = 800; rect.h = 480;
     ltest_start_timer("New Window");
     LTEST("New Window", lite_new_window(NULL, &rect, DWCAPS_ALPHACHANNEL, 
                                     liteDefaultWindowTheme,
                                     "LiteWindow test", &window));
     ltest_stop_timer();


     ltest_start_timer("Set Opacities in Window");
     LTEST("Set Window Some Opacity", lite_set_window_opacity(window, 
                0x99));
     LTEST("Set Window No Opacity", lite_set_window_opacity(window, 
                liteNoWindowOpacity));
     LTEST("Set Window Full Opacity", lite_set_window_opacity(window, 
                liteFullWindowOpacity));
     ltest_stop_timer();

     rect.x = 10; rect.y = 10; rect.w = 400;
     ltest_start_timer("New Label and set Text");
     LTEST("New Label", lite_new_label(LITE_BOX(window), &rect, 
                    liteNoLabelTheme, 20, &label));
     LTEST("Set Label Text", lite_set_label_text(label, "Testing LiteWindow"));
     ltest_stop_timer();
     
     ltest_display_timestamps();

     lite_window_event_loop(window, timeout);

     LTEST("Destroy Window", lite_destroy_window(window));
     LTEST("Close LiTE", lite_close());

     ltest_print_timestamps();

     return 0;
}

