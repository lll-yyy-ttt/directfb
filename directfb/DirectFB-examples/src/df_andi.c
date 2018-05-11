/*
   (c) Copyright 2000-2002  convergence integrated media GmbH.
   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Hundt <andi@fischlustig.de> and
              Sven Neumann <neo@directfb.org>.

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

#include <directfb.h>

#include <direct/util.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "util.h"

/* height and width of the penguin surface (more that one penguin on it) */
#define XTUXSIZE 400
#define YTUXSIZE 240

/* height and width of one sprite */
#define XSPRITESIZE 40
#define YSPRITESIZE 60

/* new animation frame is set every ANIMATION_DELAY steps */
#define ANIMATION_DELAY 5

/* resolution of the destination mask */
static int DESTINATION_MASK_WIDTH;
static int DESTINATION_MASK_HEIGHT;

/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...)                                                    \
     {                                                                    \
          err = x;                                                        \
          if (err != DFB_OK) {                                            \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ );     \
               DirectFBErrorFatal( #x, err );                             \
          }                                                               \
     }

/* DirectFB interfaces needed by df_andi */
static IDirectFB               *dfb;
static IDirectFBSurface        *primary;
static IDirectFBSurface        *secondary;
static IDirectFBEventBuffer    *keybuffer;
static IDirectFBImageProvider  *provider;
static IDirectFBFont           *font;

/* DirectFB surfaces used by df_andi */
static IDirectFBSurface *tuximage;
static IDirectFBSurface *background;
static IDirectFBSurface *destination_mask;
static IDirectFBScreen       *primary_screen;
static IDirectFBDisplayLayer *secondary_layer;

static DFBScreenEncoderConfig  encoderCfg;

/* values on which placement of penguins and text depends */
static int xres;
static int yres;
static int fontheight;

static bool triple;
static bool alpha;
static bool do_print_fps;

static unsigned int rand_pool = 0x12345678;
static unsigned int rand_add  = 0x87654321;

static bool dual_output_enabled = false;

static inline unsigned int myrand()
{
     rand_pool ^= ((rand_pool << 7) | (rand_pool >> 25));
     rand_pool += rand_add;
         rand_add  += rand_pool;

     return rand_pool;
}

/* anmation frame */
typedef struct _PenguinFrame {
     DFBRectangle rect;
     struct _PenguinFrame *next;
} PenguinFrame;

/* linked lists of animation frames */
static PenguinFrame* left_frames = NULL;
static PenguinFrame* right_frames = NULL;
static PenguinFrame* up_frames = NULL;
static PenguinFrame* down_frames = NULL;

typedef enum {
     DIR_LEFT,
     DIR_RIGHT,
     DIR_UP,
     DIR_DOWN
} Direction;

/* penguin struct, needed for each penguin on the screen */
typedef struct _Penguin {
     int x, y;
     int x_togo;
     int y_togo;
     int moving;
     int sprite_nr;
     int delay;
     PenguinFrame *left_frame;
     PenguinFrame *right_frame;
     PenguinFrame *up_frame;
     PenguinFrame *down_frame;
     PenguinFrame **frameset;
     struct _Penguin *next;
} Penguin;

/* needed for the penguin linked list */
static Penguin *penguins     = NULL;
static Penguin *last_penguin = NULL;

/* number of penguins currently on screen */
static int population = 0;

/* number of destination coordinates in coords array */
static int nr_coords = 0;

/* coors array has hardcoded maximum possible length */
static int *coords;

/*
 * adds one penguin to the list, and sets initial state
 */
static void spawn_penguin()
{
     Penguin *new_penguin = (Penguin*)calloc( 1, sizeof(Penguin) );

     new_penguin->x = xres/2;
     new_penguin->y = yres/2;
     new_penguin->x_togo = 0;
     new_penguin->y_togo = 0;
     new_penguin->moving = 1;

     new_penguin->delay = 5;

     new_penguin->left_frame = left_frames;
     new_penguin->right_frame = right_frames;
     new_penguin->up_frame = up_frames;
     new_penguin->down_frame = down_frames;
     new_penguin->frameset = &new_penguin->down_frame;

     new_penguin->next = NULL;

     if (!penguins) {
          penguins = new_penguin;
          last_penguin = new_penguin;
     }
     else {
          last_penguin->next = new_penguin;
          last_penguin = new_penguin;
     }
     population++;
}

/*
 * removes one penguin (the first) from the list
 */
static void destroy_penguin()
{
     Penguin *first_penguin = penguins;

     if (penguins) {
          penguins = penguins->next;
          free (first_penguin);
          population--;
     }
}

/*
 * removes a given number of penguins
 */
static void destroy_penguins(int number)
{
     while (number--)
          destroy_penguin();
}


/*
 * adds a given number of penguins
 */
static void spawn_penguins(int number)
{
     while (number--)
          spawn_penguin();
}

/*
 * blits all penguins to the screen
 */
static void draw_penguins()
{
     int           j = population;
     Penguin      *p = penguins;
     DFBRectangle  rects[population < 256 ? population : 256];
     DFBPoint      points[population < 256 ? population : 256];

     primary->SetBlittingFlags( primary, alpha ? DSBLIT_BLEND_ALPHACHANNEL : DSBLIT_SRC_COLORKEY );

     while (j > 0) {
          int n = 0;

          while (n < j && n < 256) {
               rects[n] = (*(p->frameset))->rect;

               points[n].x = p->x;
               points[n].y = p->y;

               n++;
               p = p->next;
          }

          primary->BatchBlit( primary, tuximage, rects, points, n );

          j -= n;
     }
}

/*
 *  moves and clipps all penguins, penguins that are in formation shiver,
 *  penguins not in formation "walk"
 */
static void move_penguins( int step )
{
     Penguin *p = penguins;

     while (p) {
          if (ABS(p->x_togo) < step)
               p->x_togo = 0;

          if (ABS(p->y_togo) < step)
               p->y_togo = 0;


          if (!p->x_togo && !p->y_togo) {
               if (p->moving) {
                    /* walking penguins get new destination if they have reached
                       their destination */
                    p->x_togo = (myrand()%101) - 50;
                    p->y_togo = (myrand()%101) - 50;
               }
               else {
                    p->frameset = &p->down_frame;

                    /* penguins that have reached their
                       formation point jitter */
                    p->x += myrand()%3 - 1;
                    p->y += myrand()%3 - 1;
               }
          }
          /* increase/decrease coordinates and to go variables */
          if (p->x_togo > 0) {
               p->x -= step;
               p->x_togo -= step;

               p->frameset = &p->left_frame;
          }
          if (p->x_togo < 0) {
               p->x += step;
               p->x_togo += step;

               p->frameset = &p->right_frame;
          }
          if (p->y_togo > 0) {
               p->y -= step;
               p->y_togo -= step;

               p->frameset = &p->up_frame;
          }
          if (p->y_togo < 0) {
               p->y += step;
               p->y_togo += step;

               p->frameset = &p->down_frame;
          }

          /* clip penguin */
          if (p->x < 0)
               p->x = 0;

          if (p->y < 0)
               p->y = 0;

          if (p->x > xres - XSPRITESIZE)
               p->x = xres - XSPRITESIZE;

          if (p->y > yres - YSPRITESIZE)
               p->y = yres - YSPRITESIZE;

          if (p->delay == 0) {
               *(p->frameset) = (*(p->frameset))->next;
               p->delay = 5;
          }
          else {
               p->delay--;
          }

          p = p->next;
     }
}

/*
 * searches a destination point in the coords array for each penguin
 */
static void penguins_search_destination() {
     Penguin *p = penguins;
     if (nr_coords) {
          while (p) {
               int entry = (myrand()%nr_coords) * 2;

               p->x_togo= p->x - coords[entry]   * xres/1000.0f;
               p->y_togo= p->y - coords[entry+1] * yres/1000.0f;
               p->moving = 0;

               p = p->next;
          }
     }
}

/*
 * removes all penguins
 */
static void destroy_all_penguins()
{
     Penguin *p = penguins;

     while (p) {
          penguins = p->next;
          free(p);
          p = penguins;
     }
}

/*
 * revives all penguins, penguins that are in formation move again
 */
static void revive_penguins()
{
     Penguin *p = penguins;

     while (p) {
          p->moving = 1;
          p = p->next;
     }
}

/*
 * interprets the destination mask from the destination_mask surface, all back
 * pixels become formation points, and are stored in the coords array
 */
static void read_destination_mask()
{
     DFBResult  ret;
     int        x, y;
     void      *ptr;
     int        pitch;

     coords = calloc( DESTINATION_MASK_WIDTH * DESTINATION_MASK_HEIGHT, sizeof(int) );


     ret = destination_mask->Lock( destination_mask, DSLF_READ, &ptr, &pitch );
     if (ret) {
          DirectFBError( "Could not lock destination mask surface", ret );
          return;
     }

     for (y=0;y<DESTINATION_MASK_HEIGHT;y++) {
          u32 *src = ptr + y * pitch;

          for (x=0;x<DESTINATION_MASK_WIDTH;x++) {
               if ((src[x] & 0x00FFFFFF) == 0) {
                    coords[nr_coords*2  ] = (x *(1000/DESTINATION_MASK_WIDTH));
                    coords[nr_coords*2+1] = (y *(1000/DESTINATION_MASK_HEIGHT));
                    nr_coords++;
               }
          }
     }

     destination_mask->Unlock( destination_mask );
}

/*
 * initializes the animation frames for a specified direction at yoffset
 */
static void initialize_direction_frames(PenguinFrame **direction_frames, int yoffset)
{
     PenguinFrame* new_frame = NULL;
     PenguinFrame* last_frame = NULL;

     if (!*direction_frames) {
          int i;

          for (i = 0; i < (XTUXSIZE/XSPRITESIZE - 1) ;i++) {
               new_frame = (PenguinFrame*) malloc( sizeof(PenguinFrame) ); //FIXME: leak

               new_frame->rect.x = i*XSPRITESIZE;
               new_frame->rect.y = YSPRITESIZE*yoffset;
               new_frame->rect.w = XSPRITESIZE;
               new_frame->rect.h = YSPRITESIZE;

               if (!*direction_frames) {
                    *direction_frames = new_frame;
               }
               else {
                    last_frame->next = new_frame;
               }
               last_frame = new_frame;
          }
          last_frame->next = *direction_frames;
     }
}

/*
 * initialize all animation frames
 */
static void initialize_animation()
{
     initialize_direction_frames( &down_frames,  0 );
     initialize_direction_frames( &left_frames,  1 );
     initialize_direction_frames( &up_frames,    2 );
     initialize_direction_frames( &right_frames, 3 );
}

/*
 * set up DirectFB and load resources
 */
static void init_resources( int argc, char *argv[] )
{
     DFBResult err;
     int       i;
     DFBSurfaceDescription dsc;

     IDirectFBDisplayLayer      *layer;
     DFBDisplayLayerConfig       config;
     DFBDisplayLayerConfigFlags  ret_failed;

     srand((long)time(0));

     DFBCHECK(DirectFBInit( &argc, &argv ));

     for (i=1; i<argc; i++) {
          if (!strcmp( argv[i], "-a" ))
               alpha = true;
          else if (!strcmp( argv[i], "-f" ))
               do_print_fps = true;
          else {
               fprintf( stderr, "Usage: df_andi [-a] [-f]\n"
                                "\n"
                                "Options:\n"
                                "  -a      use alpha channel for penguins instead of color keying\n"
                                "  -f      print frame rate every second on console\n"
                                );
               exit(0);
          }
     }

     /* create the super interface */
     DFBCHECK(DirectFBCreate( &dfb ));

     err = dfb->GetScreen( dfb, DSCID_PRIMARY, &primary_screen );
     if (err)
       DirectFBError( "Failed to get primary screen", err );

     /* create an input buffer for key events */
     DFBCHECK(dfb->CreateInputEventBuffer( dfb, DICAPS_KEYS,
                                           DFB_FALSE, &keybuffer ));

     /* set our cooperative level to DFSCL_FULLSCREEN for exclusive access to
        the primary layer */
     err = dfb->SetCooperativeLevel( dfb, DFSCL_FULLSCREEN );
     if (err)
       DirectFBError( "Failed to get exclusive access", err );

     /* check if triple buffering is supported */
     config.flags = DLCONF_BUFFERMODE;
     config.buffermode = DLBM_TRIPLE;
     DFBCHECK(dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &layer ));
     layer->TestConfiguration( layer, &config, &ret_failed );
     layer->Release( layer );

     if( ret_failed == DLCONF_NONE )
          triple = true;

     /* get the primary surface, i.e. the surface of the primary layer we have
        exclusive access to */
     dsc.flags = DSDESC_CAPS;
     dsc.caps = DSCAPS_PRIMARY | ( triple ? DSCAPS_TRIPLE : DSCAPS_DOUBLE );
     err = dfb->CreateSurface( dfb, &dsc, &primary );
     if (triple && err) {
          /* with force-windowed: windows do not support triple yet */
          triple = false;
          dsc.caps = DSCAPS_PRIMARY | DSCAPS_DOUBLE;
          DFBCHECK(dfb->CreateSurface( dfb, &dsc, &primary ));
     }

     DFBCHECK(primary->GetSize( primary, &xres, &yres ));

     /* load font */
     {
          DFBFontDescription desc;

          desc.flags = DFDESC_HEIGHT;
          desc.height = 24;

          DFBCHECK(dfb->CreateFont( dfb, FONT, &desc, &font ));
          DFBCHECK(font->GetHeight( font, &fontheight ));
          DFBCHECK(primary->SetFont( primary, font ));
     }

     /* load penguin animation */
     DFBCHECK(dfb->CreateImageProvider( dfb, alpha ? DATADIR"/tux_alpha.png" : DATADIR"/tux.png",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &tuximage ));

     DFBCHECK(provider->RenderTo( provider, tuximage, NULL ));
     provider->Release( provider );

     /* set the colorkey to green */
     tuximage->SetSrcColorKey( tuximage, 0x00, 0xFF, 0x00 );

     /* load the background image */
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR"/wood_andi.jpg",
                                        &provider ));

     DFBCHECK (provider->GetSurfaceDescription (provider, &dsc));
     dsc.flags = DSDESC_WIDTH | DSDESC_HEIGHT;
     dsc.width = xres;
     dsc.height = yres;
     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &background ));

     DFBCHECK(provider->RenderTo( provider, background, NULL ));
     provider->Release( provider );

     /* load the penguin destination mask */
     DFBCHECK(dfb->CreateImageProvider( dfb, DATADIR
                                        "/destination_mask.png",
                                        &provider ));

     DFBCHECK(provider->GetSurfaceDescription( provider, &dsc ));

     dsc.pixelformat = DSPF_RGB32;

     DESTINATION_MASK_WIDTH  = dsc.width;
     DESTINATION_MASK_HEIGHT = dsc.height;

     DFBCHECK(dfb->CreateSurface( dfb, &dsc, &destination_mask ));

     DFBCHECK(provider->RenderTo( provider, destination_mask, NULL ));
     provider->Release( provider );
}

/*
 * deinitializes resources and DirectFB
 */
static void deinit_resources()
{
     free( coords );

     destroy_all_penguins();

     keybuffer->Release( keybuffer );
     font->Release( font );
     tuximage->Release( tuximage );
     background->Release( background );
     destination_mask->Release( destination_mask );
     primary->Release( primary );

     if (dual_output_enabled)
     {
        secondary_layer->Release( secondary_layer );
        secondary->Release( secondary );
     }

     primary_screen->Release( primary_screen );
     dfb->Release( dfb );
}

static float getFreq(DFBScreenEncoderFrequency dfb_freq)
{
    float freq;

    switch (dfb_freq)
    {
        case DSEF_25HZ:     freq = 25;     break;
        case DSEF_29_97HZ:  freq = 29.97;  break;
        case DSEF_50HZ:     freq = 50;     break;
        case DSEF_59_94HZ:  freq = 59.94;  break;
        case DSEF_60HZ:     freq = 60;     break;
        case DSEF_75HZ:     freq = 75;     break;
        case DSEF_30HZ:     freq = 30;     break;
        case DSEF_24HZ:     freq = 24;     break;
        case DSEF_23_976HZ: freq = 23.976; break;
        default:            freq = 0;      break;
    }

    return freq;
}

int main( int argc, char *argv[] )
{
     DFBResult err;
     FPSData fps;
     IdleData idle;
     int     quit = 0;
     int     clipping = 0;
     int     current_format = 0;
     DFBScreenPowerMode powerMode = DSPM_ON;

     init_resources( argc, argv );

     read_destination_mask();

     initialize_animation();

     spawn_penguins( 200 );

     primary->SetDrawingFlags( primary, DSDRAW_BLEND );

     fps_init ( &fps  );
     idle_init( &idle );

     /* main loop */
     while (!quit) {
          DFBInputEvent evt;
          char buf[32];

          /* draw the background image */
          primary->SetBlittingFlags( primary, DSBLIT_NOFX );

          primary->Blit( primary, background, NULL, 0, 0 );

          /* move the penguins 3 times, thats faster ;-) */
          move_penguins( 3 );

          /* draw all penguins */
          draw_penguins();

          /* draw the population string in upper left corner */
          primary->SetColor( primary, 0, 0, 60, 0xa0 );
          primary->FillRectangle( primary, 20, 20, 440, fontheight+5 );

          snprintf( buf, sizeof(buf), "Penguin Population: %d", population );

          primary->SetColor( primary, 180, 200, 255, 0xFF );
          primary->DrawString( primary, buf, -1, 25, 20, DSTF_LEFT | DSTF_TOP );

          snprintf( buf, sizeof(buf), "FPS: %s", fps.fps_string );

          primary->SetColor( primary, 190, 210, 255, 0xFF );
          primary->DrawString( primary, buf, -1, 320, 20, DSTF_LEFT | DSTF_TOP );

          /* Add idle information */
          primary->SetColor( primary, 0, 0, 60, 0xa0 );
          primary->FillRectangle( primary, xres-200, 20, 190, fontheight+5 );

          snprintf( buf, sizeof(buf), "CPU Idle: %s%%", idle.idle_string );

          primary->SetColor( primary, 180, 200, 255, 0xFF );
          primary->DrawString( primary, buf, -1, xres-195, 20, DSTF_LEFT | DSTF_TOP );

          /* flip display */
          primary->Flip( primary, NULL, triple ? DSFLIP_ONSYNC : DSFLIP_WAITFORSYNC );

          fps_count ( &fps,  1000 );
          idle_count( &idle, 1000 );

          if (do_print_fps && !fps.frames)
               printf( "%s\n", fps.fps_string );

          /* process keybuffer */
          while (keybuffer->GetEvent( keybuffer, DFB_EVENT(&evt)) == DFB_OK) {
               if (evt.type == DIET_KEYPRESS) {
                    switch (DFB_LOWER_CASE(evt.key_symbol)) {
                         case DIKS_ESCAPE:
                         case DIKS_SMALL_Q:
                         case DIKS_BACK:
                         case DIKS_STOP:
                         case DIKS_EXIT:
                              /* quit main loop */
                              quit = 1;
                              break;
                         case DIKS_SMALL_S:
                         case DIKS_CURSOR_UP:
                              /* add another penguin */
                              spawn_penguins(10);
                              break;
                         case DIKS_SMALL_R:
                              /* penguins in formation will walk around again */
                              revive_penguins();
                              break;
                         case DIKS_SMALL_D:
                         case DIKS_CURSOR_DOWN:
                              /* removes one penguin */
                              destroy_penguins(10);
                              break;
                         case DIKS_SMALL_C:
                              /*toggle clipping*/
                              clipping=!clipping;
                              if (clipping) {
                                   DFBRegion clipregion =
                                       { 100,100, xres-100, yres-100 };
                                   primary->SetClip( primary, &clipregion );
                              }
                              else
                                   primary->SetClip( primary, NULL );
                              break;
                         case DIKS_SPACE:
                         case DIKS_ENTER:
                         case DIKS_OK:
                              /* penguins go in formation */
                              penguins_search_destination();
                              break;
                         case DIKS_SMALL_O:
                          {
                              DFBScreenOutputConfig cfg;
                              int width, height;

                              /* Cycle around these modes:
                                 -> 720p/60 -> 480i/30 -> 1080p/50 -> 1080p/25 -> 1080p/24 -> 1080i/25 ->
                                    720p/50 -> 576i/25 -> 1080p/60 -> 1080p/30 -> 1080i/30 ->
                              */
                              cfg.flags = DSOCONF_RESOLUTION;
                              if (current_format == 0)      /* 720p/60 -> 480i/30 */
                              {
                                  cfg.resolution = encoderCfg.resolution = DSOR_720_480;
                                  encoderCfg.scanmode = DSESM_INTERLACED;
                                  encoderCfg.frequency = DSEF_29_97HZ;
                                  width = 720;
                                  height = 480;
                              }
                              else if (current_format == 1) /* 480i/30 -> 1080p/50 */
                              {
                                  cfg.resolution = encoderCfg.resolution = DSOR_1920_1080;
                                  encoderCfg.scanmode = DSESM_PROGRESSIVE;
                                  encoderCfg.frequency = DSEF_50HZ;
                                  width = 1920;
                                  height = 1080;
                              }
                              else if (current_format == 2) /* 1080p/50 -> 1080p/25 */
                              {
                                  cfg.resolution = encoderCfg.resolution = DSOR_1920_1080;
                                  encoderCfg.scanmode = DSESM_PROGRESSIVE;
                                  encoderCfg.frequency = DSEF_25HZ;
                                  width = 1920;
                                  height = 1080;
                              }
                              else if (current_format == 3) /* 1080p/25 -> 1080p/24 */
                              {
                                  cfg.resolution = encoderCfg.resolution = DSOR_1920_1080;
                                  encoderCfg.scanmode = DSESM_PROGRESSIVE;
                                  encoderCfg.frequency = DSEF_24HZ;
                                  width = 1920;
                                  height = 1080;
                              }
                              else if (current_format == 4) /* 1080p/24 -> 1080i/25 */
                              {
                                  cfg.resolution = encoderCfg.resolution = DSOR_1920_1080;
                                  encoderCfg.scanmode = DSESM_INTERLACED;
                                  encoderCfg.frequency = DSEF_25HZ;
                                  width = 1920;
                                  height = 1080;
                              }
                              else if (current_format == 5) /* 1080i/25 -> 720p/50 */
                              {
                                  cfg.resolution = encoderCfg.resolution = DSOR_1280_720;
                                  encoderCfg.scanmode = DSESM_PROGRESSIVE;
                                  encoderCfg.frequency = DSEF_50HZ;
                                  width = 1280;
                                  height = 720;
                              }
                              else if (current_format == 6) /* 720p/50 -> 576i/25 */
                              {
                                  cfg.resolution = encoderCfg.resolution = DSOR_720_576;
                                  encoderCfg.scanmode = DSESM_INTERLACED;
                                  encoderCfg.frequency = DSEF_25HZ;
                                  width = 720;
                                  height = 576;
                              }
                              else if (current_format == 7) /* 576i/25 -> 1080p/60 */
                              {
                                  cfg.resolution = encoderCfg.resolution = DSOR_1920_1080;
                                  encoderCfg.scanmode = DSESM_PROGRESSIVE;
                                  encoderCfg.frequency = DSEF_60HZ;
                                  width = 1920;
                                  height = 1080;
                              }
                              else if (current_format == 8) /* 1080p/60 -> 1080p/30 */
                              {
                                  cfg.resolution = encoderCfg.resolution = DSOR_1920_1080;
                                  encoderCfg.scanmode = DSESM_PROGRESSIVE;
                                  encoderCfg.frequency = DSEF_30HZ;
                                  width = 1920;
                                  height = 1080;
                              }
                              else if (current_format == 9) /* 1080p/30 -> 1080i/30 */
                              {
                                  cfg.resolution = encoderCfg.resolution = DSOR_1920_1080;
                                  encoderCfg.scanmode = DSESM_INTERLACED;
                                  encoderCfg.frequency = DSEF_29_97HZ;
                                  width = 1920;
                                  height = 1080;
                              }
                              else                          /* 1080i/30 -> 720p/60 */
                              {
                                  cfg.resolution = encoderCfg.resolution = DSOR_1280_720;
                                  encoderCfg.scanmode = DSESM_PROGRESSIVE;
                                  encoderCfg.frequency = DSEF_60HZ;
                                  width = 1280;
                                  height = 720;
                              }

                              encoderCfg.flags   = (DFBScreenEncoderConfigFlags)(DSECONF_TV_STANDARD | DSECONF_SCANMODE | DSECONF_FREQUENCY |
                              DSECONF_RESOLUTION );
                              encoderCfg.tv_standard = DSETV_DIGITAL;

                              DFBCHECK(primary_screen->SetEncoderConfiguration(primary_screen, 0, &encoderCfg));

                              fprintf(stderr, "Output resolution %d%c/%02.02fHz\n",
                                      height, (encoderCfg.scanmode == DSESM_PROGRESSIVE)?'p':'i', getFreq(encoderCfg.frequency));

                              current_format = (current_format+1)%11;

                              break;
                          }
                         case DIKS_SMALL_P:
                              powerMode = (powerMode == DSPM_ON) ? DSPM_OFF : DSPM_ON;
                              fprintf(stderr, "SetPowerMode %s\n", (powerMode == DSPM_ON) ? "On" : "Off");
                              primary_screen->SetPowerMode(primary_screen, powerMode);
                              break;

                        case DIKS_SMALL_M:
                        {
                             DFBDisplayLayerConfig layerCfg;

                             if (!dual_output_enabled)
                             {
                                 DFBCHECK(dfb->GetDisplayLayer(dfb, DLID_PRIMARY+1, &secondary_layer));
                                 DFBCHECK(secondary_layer->SetCooperativeLevel(secondary_layer, DLSCL_ADMINISTRATIVE));
                                 layerCfg.flags = DLCONF_SOURCE | DLCONF_BUFFERMODE | DLCONF_PIXELFORMAT;
                                 layerCfg.source = DLSID_SURFACE;
                                 layerCfg.buffermode = DLBM_FRONTONLY;
                                 layerCfg.pixelformat = DSPF_AYUV;
                                 DFBCHECK(secondary_layer->SetConfiguration(secondary_layer, &layerCfg));
                                 DFBCHECK(secondary_layer->GetSurface(secondary_layer, &secondary));
                                 secondary->Flip( secondary, NULL, DSFLIP_NONE );

                                 dual_output_enabled =true;
                             }
                             else
                             {

                                if (dual_output_enabled)
                                {
                                   secondary_layer->Release( secondary_layer );
                                   secondary->Release( secondary );
                                }

                                dual_output_enabled = false;
                             }

                             break;
                        }
                         default:
                              break;
                    }
               }
          }
     }

     deinit_resources();
     return 42;
}
