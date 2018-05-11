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

#include <lite/lite.h>

#include <leck/label.h>

#include "testutils.h"

#define TIMEOUT 1000

void run_label_tests(LiteWindow *window);
void run_label_performance_tests(LiteWindow *window);

int main (int argc, char *argv[])
{
     LiteWindow *window;
     int timeout = 0;

     if (argc > 1) {
        if ( (strcmp(argv[1], "timeout") == 0) )
            timeout = TIMEOUT;
     } 

     ltest_start_timer("Init LiTE");
     if( lite_open(&argc, &argv) )
          return 1;
     ltest_stop_timer();

     window = ltest_create_main_window("Label Unit Test");

     run_label_tests(window);
     run_label_performance_tests(window);

     ltest_display_timestamps();

     lite_window_event_loop(window, timeout);

     lite_destroy_window(window);
     lite_close();
     ltest_print_timestamps();

     return 0;
}

void run_label_tests(LiteWindow *window)
{
     LiteLabel *label;
     DFBRectangle rect;
     DFBColor color;
     int font_size = 16;
     
     color.r = 0xff; color.g = 0x00; color.g = 0x00; color.a = 0xff;

     /* create a label  */
     rect.x = 10; rect.y = 10; rect.w = 200; rect.h = 20;

     LTEST("Create a Label", lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 
               font_size, &label));

     /* destroy a label */
     LTEST("Destroy a Label", lite_destroy_box(LITE_BOX(label)));

     /* create another label */
     LTEST("Create a Label", lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 
               font_size, &label));

     /* set label test */
     LTEST("Set Label Text", lite_set_label_text(label, "This is a test"));

     /* set label alignment */
     LTEST("Set Label Alignment", lite_set_label_alignment(label, LITE_LABEL_RIGHT));
     LTEST("Set Label Alignment", lite_set_label_alignment(label, LITE_LABEL_LEFT));
     LTEST("Set Label Alignment", lite_set_label_alignment(label, LITE_LABEL_CENTER));

     /* set label color */
     LTEST("Set Label Color", lite_set_label_color(label, &color));
}

#define NUM_LABELS  500

void run_label_performance_tests(LiteWindow *window)
{
     int i;
     LiteLabel *labels[NUM_LABELS];
     DFBResult res;
     DFBRectangle rect;
     int y_offset = 0;
     int font_size = 10;


     /* create many labels and measure */
     ltest_start_timer("Create many labels");
     for (i = 0; i < NUM_LABELS; i++) {
          rect.x = 0; rect.y = y_offset; rect.w = 100; rect.h = 20;
          res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, font_size, &labels[i]);
          y_offset += 10;
     }
     ltest_stop_timer();
}



