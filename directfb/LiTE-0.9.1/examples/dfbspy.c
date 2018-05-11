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
#include <errno.h>

#include <direct/clock.h>
#include <direct/messages.h>

#include <directfb.h>

#include <lite/lite.h>
#include <lite/window.h>

#include <leck/button.h>
#include <leck/image.h>
#include <leck/label.h>
#include <leck/textline.h>

/**************************************************************************************************/

typedef struct {
     int lease_purchase;
     int cede;
     int attach;
     int detach;
     int ref_up;
     int ref_down;
     int prevail_swoop;
     int dismiss;
} FusionStat;

static int
read_stat( FusionStat *stat, int world )
{
     int   c;
     FILE *f;
     char  buf[32];
     
     snprintf( buf, sizeof(buf), "/proc/fusion/%d/stat", world );

     f = fopen( buf, "r" );
     if (!f) {
          D_PERROR( "LiTE/DFBSpy: Could not open %s!\n", buf );
          return errno;
     }

     do {
          c = fgetc( f );
     } while (c != '\n' && c != EOF);

     if (fscanf( f, "%d %d %d %d %d %d %d %d",
                 &stat->lease_purchase, &stat->cede, &stat->attach, &stat->detach,
                 &stat->ref_up, &stat->ref_down, &stat->prevail_swoop, &stat->dismiss ) < 8)
          fprintf( stderr, "Parsing failed!\n" );

     fclose( f );

     return 0;
}

/**************************************************************************************************/

#define CALC(x)     stats.x = (int)(((s.x - last_stat.x) * 1000 / (float) diff) + 0.5f)

static FusionStat last_stat, stats;
static long long  last_millis;

static int
update_stats( int world )
{
     FusionStat s;
     long long  last   = last_millis;
     long long  millis = direct_clock_get_millis();
     long long  diff   = millis - last_millis;

     if (diff < 100 || (last > 0 && diff < 400))
          return 0;

     if (read_stat( &s, world ))
          return 0;

     last_millis = millis;

     CALC( lease_purchase );
     CALC( cede );
     CALC( attach );
     CALC( detach );
     CALC( ref_up );
     CALC( ref_down );
     CALC( prevail_swoop );
     CALC( dismiss );

     last_stat = s;

     return last > 0;
}

/**************************************************************************************************/

static void
update_number( LiteLabel *label, void *ctx )
{
     char buf[20];
     int  num = *(int*)ctx;

     snprintf( buf, sizeof(buf), "%d", num );

     lite_set_label_text( label, buf );
}

/**************************************************************************************************/

static const struct {
     const char     *label;

     void          (*update)( LiteLabel *label, void *ctx );
     void           *ctx;
} list[] = {
     { "lease/purchase", update_number, &stats.lease_purchase },
     { "cede",           update_number, &stats.cede },
     { "attach",         update_number, &stats.attach },
     { "detach",         update_number, &stats.detach },
     { "ref up",         update_number, &stats.ref_up },
     { "ref down",       update_number, &stats.ref_down },
     { "prevail/swoop",  update_number, &stats.prevail_swoop },
     { "dismiss",        update_number, &stats.dismiss }
};

#define NUM_LIST    (sizeof(list)/sizeof(list[0]))

/**************************************************************************************************/

static void
button_pressed( LiteButton *button, void *ctx )
{
     lite_close_window( LITE_WINDOW(ctx) );
}

/**************************************************************************************************/

int
main (int argc, char *argv[])
{
     int           i;
     int           world = 0;
     LiteButton   *button;
     LiteImage    *image;
     LiteWindow   *window;
     LiteLabel    *labels[NUM_LIST];
     DFBRectangle rect;
     const char   *session;
     DFBResult     res;

     /* initialize */
     if (lite_open( &argc, &argv ))
          return 1;

     session = getenv( "DIRECTFB_SESSION" );
     if (session)
          world = atoi( session );

     /* create a window */
     rect.x = LITE_CENTER_HORIZONTALLY;
     rect.y = LITE_CENTER_VERTICALLY;
     rect.w = 172;
     rect.h = 248;  
     res = lite_new_window( NULL,
                            &rect,
                            DWCAPS_ALPHACHANNEL, 
                            liteDefaultWindowTheme,
                            "DirectFB Spy",
                            &window );


     /* setup the image */
     rect.x = 10; rect.y = 10; rect.w = 52; rect.h = 30;
     res = lite_new_image(LITE_BOX(window), &rect, liteNoImageTheme, &image);

     lite_load_image( image, EXAMPLESDATADIR "/D.png" );


     /* setup the labels */
     for (i=0; i<NUM_LIST; i++) {
          int        offset = i << 4;
          LiteLabel *label;
          DFBColor  textcolor = { 0xff, 0x00, 0x00, 0xff};

          rect.x = 10; rect.y = 50 + offset; rect.w = 112;
          res = lite_new_label (LITE_BOX(window), &rect, liteNoLabelTheme, 14, &label );
          lite_set_label_color (label, &textcolor);
          //lite_set_label_font( label, "whiterabbit", LITE_FONT_PLAIN, 14 );
          lite_set_label_text (label, list[i].label );

          rect.x = 122; rect.y = 50 + offset; rect.w = 40;
          res = lite_new_label(LITE_BOX(window), &rect, liteNoLabelTheme, 14, &labels[i] );
          //lite_set_label_font( labels[i], "whiterabbit", LITE_FONT_PLAIN, 14 );
          lite_set_label_alignment (labels[i], LITE_LABEL_RIGHT );
     }


     /* setup the button */
     rect.x = 61; rect.y = 188; rect.w = 50;  rect.h = 50;
     res = lite_new_button( LITE_BOX(window), &rect, 
                            liteNoButtonTheme, 
                            &button); 
     lite_set_button_image( button, LITE_BS_NORMAL, EXAMPLESDATADIR "/stop.png" );
     lite_set_button_image( button, LITE_BS_DISABLED, EXAMPLESDATADIR "/stop_disabled.png" );
     lite_set_button_image( button, LITE_BS_HILITE, EXAMPLESDATADIR "/stop_highlighted.png" );
     lite_set_button_image( button, LITE_BS_PRESSED, EXAMPLESDATADIR "/stop_pressed.png" );

     lite_on_button_press( button, button_pressed, window );


     /* show the window */
     lite_set_window_opacity( window, liteFullWindowOpacity );


     /* run the default event loop */
     while (lite_window_event_loop( window, 100 ) != DFB_DESTROYED) {
          if (update_stats( world )) {
               for (i=0; i<NUM_LIST; i++)
                    list[i].update( labels[i], list[i].ctx );
          }
     }


     /* destroy the window with all this children and resources */
     lite_destroy_window( window );

     /* deinitialize */
     lite_close();

     return 0;
}
