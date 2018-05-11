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
#include <unistd.h>
#include <stdio.h>

#include <direct/clock.h>
#include <direct/direct.h>
#include <direct/log.h>
#include <direct/mem.h>
#include <direct/util.h>

#include <lite/lite.h>
#include <lite/window.h>

#include <leck/button.h>
#include <leck/image.h>
#include <leck/label.h>
#include <leck/textline.h>


#define MIMI(t)     t / 1000, t % 1000


typedef struct {
     LiteBox    box;

     LiteLabel *labels[64];
} BenchBox;

/**********************************************************************************************************************/

static DirectLog *bench_log;

/**********************************************************************************************************************/

static DFBResult
bench_new( LiteBox   *parent,
           int        x,
           int        y,
           int        width,
           int        height,
           BenchBox **ret_bench )
{
     int           i;
     DFBResult     ret;
     DFBRectangle  rect = { x, y, width, height };
     BenchBox     *bench;

     bench = D_CALLOC( 1, sizeof(BenchBox) );
     if (!bench)
          return D_OOM();

     ret = lite_init_box_at( LITE_BOX(bench), parent, &rect );
     if (ret) {
          D_FREE( bench );
          return ret;
     }


     /* setup the labels */
     for (i=0; i<D_ARRAY_SIZE(bench->labels); i++) {
          int x = i % 4;
          int y = i / 4;

          rect.x = x * 90;
          rect.y = y * 20;
          rect.w = 80;
          rect.h = 20;

          lite_new_label( LITE_BOX(bench), &rect, liteNoLabelTheme, 18, &bench->labels[i] );

          lite_set_label_text( bench->labels[i], "Benchmark" );
     }


     *ret_bench = bench;

     return DFB_OK;
}

/**********************************************************************************************************************/

static long long started = 0;
static long long prev    = 0;
static bool      running = false;
static bool      pending = false;

static void
print_timestamp( const char *message )
{
     long long micros = direct_clock_get_micros();


     if (!running) {
          started = prev = micros;

          direct_log_printf( bench_log, "[%4d.%03d] - %-40s .", 0, 0, message );

          running = true;
     }
     else {
          int total = micros - started;

          if (pending) {
               int diff = micros - prev;

               direct_log_printf( bench_log, ". (%4d.%03d)\n[%4d.%03d] - %-40s .",
                                  MIMI(diff), MIMI(total), message );
          }
          else
               direct_log_printf( bench_log, "[%4d.%03d] - %-40s .",
                                  MIMI(total), message );

          prev = micros;
     }

     pending = true;
}

static void
flush_timestamp()
{
     if (pending) {
          long long micros = direct_clock_get_micros();
          int       diff   = micros - prev;

          direct_log_printf( bench_log, ". (%4d.%03d)..\n", MIMI(diff) );

          pending = false;
     }
}

/**********************************************************************************************************************/

static void
button_pressed( LiteButton *button, void *ctx )
{
     lite_close_window( LITE_WINDOW(ctx) );
}

#if 0
static DFBResult TimeoutFunc (void *data)
{
     static int  n     = 0;
     LiteLabel  *label = data;
     char        buf[10];

     snprintf( buf, sizeof(buf), "n = %d", n );

     lite_set_label_text( label, buf );

     if (n++ == 50)
          return DFB_OK;

     return DFB_INTERRUPTED;
}
#endif

static DFBResult IdleResizeFunc (void *data)
{
     static int  num_calls = 0;
     LiteWindow *window    = data;

     lite_resize_window( window, window->box.rect.w ^ 8, window->box.rect.h ^ 8 );

     if (++num_calls > 6000)
          return DFB_INTERRUPTED;

     lite_enqueue_idle_callback( IdleResizeFunc, window, NULL );

     return DFB_OK;
}

int
main (int argc, char *argv[])
{
     DFBResult     ret;
     DFBRectangle  rect;
     LiteButton   *button;
     LiteLabel    *label;
     LiteWindow   *window;
     BenchBox     *bench;

     direct_initialize();

     direct_log_create( DLT_FILE, "bench.log", &bench_log );


     print_timestamp( "lite_open()" );

     /* initialize */
     if (lite_open( &argc, &argv ))
          return 1;


     print_timestamp( "lite_new_window()" );

     /* create a window */
     rect.x = 0;
     rect.y = 0;
     rect.w = 32;//740;
     rect.h = 32;//340;

     lite_new_window( NULL,
                      &rect,
                      DWCAPS_NONE,
                      liteNoWindowTheme,
                      "Benchmark",
                      &window );


     print_timestamp( "lite_new_label()" );

     rect.x = 600; rect.y = 30; rect.w = 120; rect.h = 20;
     lite_new_label( LITE_BOX(window), &rect, liteNoLabelTheme, 18, &label );


     print_timestamp( "lite_set_label_text()" );

     lite_set_label_text( label, "Welcome" );

     print_timestamp( "bench_new()" );

     /* setup the bench */
     bench_new( LITE_BOX(window), 10, 10, 600, 300, &bench );


     print_timestamp( "lite_new_button()" );

     /* setup the button */
     rect.x = 630; rect.y = 130; rect.w = 50; rect.h = 50;
     lite_new_button(LITE_BOX(window), &rect, liteNoButtonTheme, &button);


     print_timestamp( "lite_set_button_image() x 4" );

     lite_set_button_image( button, LITE_BS_NORMAL, EXAMPLESDATADIR "/stop.png" );
     lite_set_button_image( button, LITE_BS_DISABLED, EXAMPLESDATADIR "/stop_disabled.png" );
     lite_set_button_image( button, LITE_BS_HILITE, EXAMPLESDATADIR "/stop_highlighted.png" );
     lite_set_button_image( button, LITE_BS_PRESSED, EXAMPLESDATADIR "/stop_pressed.png" );


     print_timestamp( "lite_on_button_press()" );

     lite_on_button_press( button, button_pressed, window );


     /* show the window */
     print_timestamp( "lite_set_window_opacity()" );

     lite_set_window_opacity( window, liteFullWindowOpacity );


     print_timestamp( "lite_draw_box()" );

     lite_draw_box( LITE_BOX(window), NULL, DFB_TRUE );



     print_timestamp( "lite_enqueue_idle_callback()" );

     lite_enqueue_idle_callback( IdleResizeFunc, window, NULL );


     /* run the default event loop */
     print_timestamp( "lite_window_event_loop() x 6000 idle resize/draw" );

     ret = lite_window_event_loop( window, 0 );
     if (ret != DFB_INTERRUPTED)
          D_DERROR( ret, "lite_window_event_loop( window, 0 )\n" );


     /* destroy the window with all this children and resources */
     print_timestamp( "lite_destroy_window()" );

     lite_destroy_window( window );


     /* deinitialize */
     print_timestamp( "lite_close()" );

     lite_close();

     flush_timestamp();

     direct_log_destroy( bench_log );

     direct_shutdown();

     return 0;
}

