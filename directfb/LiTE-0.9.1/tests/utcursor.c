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
#include <unistd.h>

#include <lite/lite.h>
#include <lite/cursor.h>

#include "testutils.h"

#define TIMEOUT 1000

void run_cursor_tests(LiteWindow *window);
void run_cursor_performance_tests(LiteWindow *window);

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

     window = ltest_create_main_window("Cursor Unit Test");

     run_cursor_tests(window);
     run_cursor_performance_tests(window);

     ltest_display_timestamps();

     lite_window_event_loop(window, timeout);

     lite_destroy_window(window);
     lite_close();
     ltest_print_timestamps();

     return 0;
}

void run_cursor_tests(LiteWindow *window)
{
     /* hide cursor */
     LTEST("Hide Cursor", lite_hide_cursor()); 
     sleep(1);

     /* show cursor */
     LTEST("Show cursor", lite_show_cursor());
}

#define NUM_LABELS  500

void run_cursor_performance_tests(LiteWindow *window)
{
}



