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

#include <direct/clock.h>

#include <lite/lite.h>
#include <lite/window.h>

#include <leck/button.h>
#include <leck/image.h>
#include <leck/label.h>
#include <leck/textline.h>


typedef struct {
     long startTime;
     long initTime;
     long windowInitTime;
     long labelInitTime;
} AppStats;


static AppStats gStats;

#define TIMEOUT 1000

static long ClockTime(void)
{
     struct timeval tv;
     gettimeofday(&tv, NULL);

     return (tv.tv_sec * 1000L + tv.tv_usec / 1000L);
}



int main (int argc, char *argv[])
{
     LiteWindow *window;
     LiteLabel *label; 
     DFBRectangle rect;
     DFBResult res;

     gStats.startTime = ClockTime();

     if( lite_open(&argc, &argv) )
          return 1;
     gStats.initTime = ClockTime();

     rect.x = 20;
     rect.y = 20;
     rect.w = 300;
     rect.h = 200;
     res = lite_new_window(NULL, &rect, DWCAPS_ALPHACHANNEL, 
                                     liteNoWindowTheme,
                                     "No Theme Window", &window);
     lite_set_window_opacity(window, liteFullWindowOpacity);
     gStats.windowInitTime = ClockTime();

     rect.x = 10; rect.y = 10; rect.w = 400;
     res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 20, &label);
     lite_set_label_text(label, "Testing LiteWindow");
     
     gStats.labelInitTime = ClockTime();

     /* end measuring */

     printf("\nINIT STATS...\n");
     printf("lite_init() time \t= %ld ms\n", 
               (long)gStats.initTime - gStats.startTime);
     printf("Window init time \t= %ld ms\n", 
               (long)gStats.windowInitTime - gStats.initTime);
     printf("Single Font load time \t= %ld ms\n", 
               (long)gStats.labelInitTime - gStats.windowInitTime);


     lite_window_event_loop(window, TIMEOUT);

     lite_destroy_window(window);

     lite_close();

     return 0;
}

