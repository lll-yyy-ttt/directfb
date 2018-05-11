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

#include <direct/clock.h>

#include <lite/window.h>

#include <leck/label.h>

#include "testutils.h"

/* globals */

typedef struct {
    char *name;
    long long value;
} LTestTimeStampRecord;

#define MAX_NUM_TIMESTAMPS 100
static LTestTimeStampRecord timestamps_global[MAX_NUM_TIMESTAMPS];
static int current_timestamp_global = 0;

void ltest_start_timer(char *name)
{
    timestamps_global[current_timestamp_global + 1].name = name;
    timestamps_global[current_timestamp_global].value = direct_clock_get_millis();
}

int ltest_stop_timer(void)
{
    int index;

    current_timestamp_global++;
    index = current_timestamp_global;

    timestamps_global[current_timestamp_global].value = direct_clock_get_millis();
    
    return index;
}

char* ltest_get_timestamp_name(int index)
{
    return timestamps_global[index].name;
}

int ltest_get_num_timestamps(void)
{
    return current_timestamp_global;
}

void ltest_print_timestamps(void)
{
    int i;
    
    for( i = 1; i < ltest_get_num_timestamps() + 1 ; i++) {
        fprintf(stderr, "TIME: %s: %lld (ms)\n",
                timestamps_global[i].name,
                timestamps_global[i].value - timestamps_global[i -1].value);
    
    }
}

void ltest_display_timestamps(void)
{
     DFBResult res;
     LiteWindow *window; 
     LiteLabel *label;
     DFBRectangle rect;
     char buf[128];
     int x_offset = 10;
     int y_offset= 20;
     int i;
   
     rect.x = 500; rect.y = 50; rect.w = 250; rect.h = 350;
     res = lite_new_window(NULL, &rect, DWCAPS_ALPHACHANNEL,
                             liteDefaultWindowTheme,
                             "TimeStamp Display", &window);
     res = lite_set_window_opacity(window, liteFullWindowOpacity); 

     printf("NUM timeSTAMPS: %d\n", ltest_get_num_timestamps());

     for (i = 1; i < ltest_get_num_timestamps() + 1; i++) {
          rect.x = x_offset; rect.y = y_offset; rect.w = 200 - x_offset;
          res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme,
                                   12, &label);
          snprintf(buf, sizeof(buf), "%s: %lld (ms)",
                         ltest_get_timestamp_name(i),
                         timestamps_global[i].value -   
                         timestamps_global[i - 1]. value);
          res = lite_set_label_text(label, buf);
          y_offset += 12;
     }
}

LiteWindow* ltest_create_main_window(const char *title)
{
     LiteWindow *window = NULL;
     DFBResult res;
     DFBRectangle rect;

     /* use hard-coded values */
     rect.x = 5; rect.y = 5; rect.w = 600; rect.h = 400;
     res = lite_new_window(NULL, &rect, DWCAPS_ALPHACHANNEL,
                              liteDefaultWindowTheme, title, &window);
     res = lite_set_window_opacity(window, liteFullWindowOpacity);

     return window;
}
