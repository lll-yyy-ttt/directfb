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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <directfb.h>
#include <directfb_util.h>
#include <directfb_version.h>

#include <direct/util.h>

#define DIRECTFB_ROTATE
#if DIRECTFB_MAJOR_VERSION < 1
#undef DIRECTFB_ROTATE
#else
#if DIRECTFB_MAJOR_VERSION == 1
#if DIRECTFB_MINOR_VERSION < 3
#undef DIRECTFB_ROTATE
#endif
#endif
#endif


/* macro for a safe call to DirectFB functions */
#define DFBCHECK(x...) \
     {                                                                \
          err = x;                                                    \
          if (err != DFB_OK) {                                        \
               fprintf( stderr, "%s <%d>:\n\t", __FILE__, __LINE__ ); \
               DirectFBErrorFatal( #x, err );                         \
          }                                                           \
     }

static inline long myclock()
{
  struct timeval tv;

  gettimeofday (&tv, NULL);
  return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int main( int argc, char *argv[] )
{
     IDirectFB              *dfb;
     IDirectFBDisplayLayer  *layer;

     IDirectFBImageProvider *provider;
     IDirectFBImageProvider *image;
     IDirectFBSurface       *cursurface;
     IDirectFBSurface       *cursurface2;

     IDirectFBWindow        *window1;
     IDirectFBWindow        *window2;
     IDirectFBSurface       *window_surface1;
     IDirectFBSurface       *window_surface2;

     IDirectFBEventBuffer   *buffer;

     IDirectFBFont          *font;

     DFBDisplayLayerConfig         layer_config;
     DFBGraphicsDeviceDescription  gdesc;
     IDirectFBWindow*              upper;
     DFBWindowID                   id1;

     int fontheight;
     int rotation;
     int err;
     int quit = 0;

     IDirectFBWindow* active = NULL;
     int grabbed = 0;
     int startx = 0;
     int starty = 0;
     int endx = 0;
     int endy = 0;
     int winupdate = 0, winx = 0, winy = 0;

     DFBCHECK(DirectFBInit( &argc, &argv ));
     DFBCHECK(DirectFBCreate( &dfb ));

     dfb->GetDeviceDescription( dfb, &gdesc );

     DFBCHECK(dfb->GetDisplayLayer( dfb, DLID_PRIMARY, &layer ));

     layer->GetConfiguration( layer, &layer_config );
#ifdef DIRECTFB_ROTATE
     layer->GetRotation( layer, &rotation );
#endif

     {
          DFBFontDescription desc;

          desc.flags = DFDESC_HEIGHT;
          desc.height = 20;

          DFBCHECK(dfb->CreateFont( dfb, "Misc-Fixed.pfa", &desc, &font ));
          font->GetHeight( font, &fontheight );
     }

     {
          DFBSurfaceDescription desc;

	  DFBCHECK(dfb->CreateImageProvider( dfb,
					     "cursor.png",
					     &provider ));
	  
	  desc.flags  = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_CAPS;
	  desc.width  = 32;
	  desc.height = 32;
	  desc.caps   = DSCAPS_NONE;
	  
	  DFBCHECK(dfb->CreateSurface( dfb, &desc, &cursurface ) );
	  
	  DFBCHECK(provider->RenderTo( provider, cursurface, NULL ) );
	  provider->Release( provider );
     }

     {
          DFBWindowDescription  desc;

          desc.flags = ( DWDESC_NAME | 
                         DWDESC_POSX | DWDESC_POSY |
                         DWDESC_WIDTH | DWDESC_HEIGHT | 
                         DWDESC_STACKING );

          desc.caps = DWCAPS_ALPHACHANNEL;
          desc.surface_caps = DSCAPS_PREMULTIPLIED;
          desc.flags |= DWDESC_CAPS | DWDESC_SURFACE_CAPS;

          strcpy(desc.name, "window2");

          desc.posx   = 20;
          desc.posy   = 120;
          desc.width  = 300;
          desc.height = 200;
          //desc.caps |= DWCAPS_DOUBLEBUFFER;
          desc.surface_caps = DSCAPS_DOUBLE;

          desc.stacking = DWSC_MIDDLE;

          DFBCHECK( layer->CreateWindow( layer, &desc, &window2 ) );
          window2->SetName( window2, "wm-test 2" );
          window2->GetSurface( window2, &window_surface2 );

          window2->CreateEventBuffer( window2, &buffer );

          DFBCHECK(window2->SetCursorShape(window2, cursurface, 0, 0));

          window2->SetOpacity( window2, 0xFF );
     }

     {
          DFBWindowDescription desc;

          desc.flags  = ( DWDESC_NAME |
                          DWDESC_POSX | DWDESC_POSY |
                          DWDESC_WIDTH | DWDESC_HEIGHT | 
                          DWDESC_CAPS | DWDESC_STACKING | DWDESC_SURFACE_CAPS );

          strcpy(desc.name, "window1");

          desc.posx   = 200;
          desc.posy   = 200;
          desc.width  = 512;
          desc.height = 145;
          desc.caps   = DWCAPS_ALPHACHANNEL;// | DWCAPS_NODECORATION;
          desc.surface_caps = DSCAPS_PREMULTIPLIED;

          desc.stacking = DWSC_MIDDLE;

          DFBCHECK(layer->CreateWindow( layer, &desc, &window1 ) );
          //window1->SetName( window1, "wm-test 1" );
          window1->GetSurface( window1, &window_surface1 );
          window_surface1->SetFont( window_surface1, font );

          DFBCHECK(dfb->CreateImageProvider( dfb,
                                             "dfblogo.png",
                                             &image ));
          image->RenderTo( image, window_surface1, NULL );

          window_surface1->SetDrawingFlags( window_surface1, DSDRAW_SRC_PREMULTIPLY );
          window_surface1->SetColor( window_surface1, 0xFF, 0x20, 0x20, 0x90 );
          window_surface1->DrawRectangle( window_surface1,
                                          0, 0, desc.width, desc.height );

          window_surface1->Flip( window_surface1, NULL, 0 );

          window1->AttachEventBuffer( window1, buffer );

          window1->SetOpacity( window1, 0xFF );

          window1->GetID( window1, &id1 );

          {
               DFBSurfaceDescription desc;

               DFBCHECK(dfb->CreateImageProvider( dfb,
                             "cursor_red.png",
                             &provider ));
          
               desc.flags  = DSDESC_WIDTH | DSDESC_HEIGHT | DSDESC_CAPS;
               desc.width  = 32;
               desc.height = 32;
               desc.caps   = DSCAPS_NONE;
              
               DFBCHECK(dfb->CreateSurface( dfb, &desc, &cursurface2 ) );
              
               DFBCHECK(provider->RenderTo( provider, cursurface2, NULL ) );
               provider->Release( provider );
          }

	  DFBCHECK(window1->SetCursorShape(window1, cursurface2, 0, 0));
     }

     window1->RequestFocus( window1 );
     window1->RaiseToTop( window1 );
     upper = window1;

     rotation = 0;

     while (!quit) 
     {
          {
               int w, h;
               static int line = 1;

               window2->GetSize( window2, &w, &h );

               if ( line >= h-1 )
                    line = 1;

               window_surface2->SetDrawingFlags( window_surface2, DSDRAW_SRC_PREMULTIPLY );

               window_surface2->SetColor( window_surface2, 0x00, 0x30, 0x10, 0xc0 );
               window_surface2->DrawRectangle( window_surface2, 0, 0, w, h );
               window_surface2->SetColor( window_surface2, 0x80, 0xa0, 0x00, 0x90 );
               window_surface2->FillRectangle( window_surface2, 1, 1, w-2, h-2 );

               window_surface2->SetColor( window_surface2, 0xff, 0x00, 0x00, 0xff );
               window_surface2->DrawLine( window_surface2, 0, line, w-2, line );

               window_surface2->Flip( window_surface2, NULL, DSFLIP_WAITFORSYNC );

               line++;
          }

          DFBWindowEvent evt;
          buffer->WaitForEventWithTimeout( buffer, 0, 1000 );
          
          while (buffer->GetEvent( buffer, DFB_EVENT(&evt) ) == DFB_OK) {
               IDirectFBWindow* window;

               if (evt.window_id == id1)
                    window = window1;
               else
                    window = window2;

               if (evt.type == DWET_GOTFOCUS) {
                    active = window;
               }
               else if (active) {
                    switch (evt.type) {

                    case DWET_BUTTONDOWN:
                         if (!grabbed) {
                              grabbed = evt.buttons;
                              startx  = evt.cx;
                              starty  = evt.cy;
                              window->GrabPointer( window );
                         }
                         break;

                    case DWET_BUTTONUP:
                         switch (evt.button) {
                              case DIBI_LEFT:
                                   if (grabbed && !evt.buttons) {
                                        window->UngrabPointer( window );
                                        grabbed = 0;
                                   }
                                   break;
                              case DIBI_MIDDLE:
                                   upper->LowerToBottom( upper );
                                   upper =
                                     (upper == window1) ? window2 : window1;
                                   break;
                              case DIBI_RIGHT:
                                   quit = DIKS_DOWN;
                                   break;
                              default:
                                   break;
                         }
                         break;

                    case DWET_KEYDOWN:
                         if (grabbed)
                              break;
                         switch (evt.key_id) {
                              case DIKI_RIGHT:
                                   active->Move (active, 1, 0);
                                   break;
                              case DIKI_LEFT:
                                   active->Move (active, -1, 0);
                                   break;
                              case DIKI_UP:
                                   active->Move (active, 0, -1);
                                   break;
                              case DIKI_DOWN:
                                   active->Move (active, 0, 1);
                                   break;
                              default:
                                   break;
                         }
                         break;

                    case DWET_LOSTFOCUS:
                         if (!grabbed && active == window)
                              active = NULL;
                         break;

                    default:
                         break;

                    }
               }

               switch (evt.type) {

               case DWET_MOTION:
               case DWET_ENTER:
               case DWET_LEAVE:
                    endx = evt.cx;
                    endy = evt.cy;
                    winx = evt.x;
                    winy = evt.y;
                    winupdate = 1;
                    break;

               case DWET_KEYDOWN:
                    switch (evt.key_symbol) {
                    case DIKS_ESCAPE:
                    case DIKS_SMALL_Q:
                    case DIKS_CAPITAL_Q:
                    case DIKS_BACK:
                    case DIKS_STOP:
                         quit = 1;
                         break;
                    case DIKS_SMALL_R:
                         if (active) {
                              rotation = (rotation + 90) % 360;

#ifdef DIRECTFB_ROTATE
                              active->SetRotation( active, rotation );
#endif
                         }
                         break;

                    case DIKS_SMALL_X:
                    case DIKS_CAPITAL_X:
                    {
                         if ( NULL != window1 && NULL != window_surface1 )
                         {
                              window_surface1->Release( window_surface1 );

                              window1->DetachEventBuffer( window1, buffer );

                              window1->Destroy( window1 );
                              window1->Release( window1 );

                              window_surface1 = NULL;
                              window1 = NULL;
                         }
                         break;
                    }
                    default:
                         break;
                    }
                    break;

               case DWET_SIZE:
               case DWET_POSITION_SIZE:
               {
                    if (evt.window_id == id1)
                    {
                         image->RenderTo( image, window_surface1, NULL );

                         window_surface1->SetDrawingFlags( window_surface1, DSDRAW_SRC_PREMULTIPLY );
                         window_surface1->SetColor( window_surface1, 0xFF, 0x20, 0x20, 0x90 );
                         window_surface1->DrawRectangle( window_surface1,
                                          0, 0, evt.w, evt.h );

                         window_surface1->Flip( window_surface1, NULL, 0 );
                    }
               } break;

               case DWET_CLOSE:
               {
                    if (evt.window_id == id1)
                    {
                         window_surface1->Release( window_surface1 );

                         window1->DetachEventBuffer( window1, buffer );

                         window1->Destroy( window1 );
                         window1->Release( window1 );

                         window_surface1 = NULL;
                         window1 = NULL;
                    }
                    else
                    {
                         quit = 1;
                    }
               } break;

               case DWET_DESTROYED:
               {
                    if (evt.window_id == id1)
                    {
                         if ( NULL != window1 && NULL != window_surface1 )
                         {
                              window_surface1->Release( window_surface1 );

                              window1->DetachEventBuffer( window1, buffer );

                              window1->Release( window1 );

                              window_surface1 = NULL;
                              window1 = NULL;
                         }
                    }
                    else
                    {
                         quit = 1;
                    }
               } break;

               default:
                    break;
               }
          }

          if (active) 
          {
               if (grabbed == 1) {
                    //active->Move( active, endx - startx, endy - starty);
                    startx = endx;
                    starty = endy;
               }
               else if (grabbed == 2) {
                    active->SetOpacity( active,
                                        (sin( myclock()/300.0 ) * 85) + 170 );
               }
               else if (winupdate) 
               {
                    if ( NULL != window1 && NULL != window_surface1 )
                    {
                    char buf[32];
                    DFBRectangle rect;
                    DFBRegion    region;

                    snprintf( buf, sizeof(buf), "x/y: %4d,%4d", winx, winy );

                    font->GetStringExtents( font, buf, -1, &rect, NULL );

                    rect.x = 1;
                    rect.y = 1;

                    rect.w += rect.w / 3;
                    rect.h += 40;

                    window_surface1->SetColor( window_surface1, 0x10, 0x10, 0x10, 0x77 );
                    window_surface1->FillRectangles( window_surface1, &rect, 1 );

                    window_surface1->SetColor( window_surface1, 0x88, 0xCC, 0xFF, 0xAA );
                    window_surface1->DrawString( window_surface1, buf, -1, rect.h/4, 5, DSTF_TOPLEFT );


                    int xpos, ypos;
                    DFBCHECK( layer->GetCursorPosition( layer, &xpos, &ypos ) );
                    window1->ScreenToClient( window1, &xpos, &ypos );

                    snprintf( buf, sizeof(buf), "x/y: %4d,%4d", xpos, ypos );

                    window_surface1->SetColor( window_surface1, 0x88, 0xCC, 0xFF, 0xAA );
                    window_surface1->DrawString( window_surface1, buf, -1, rect.h/4, 35, DSTF_TOPLEFT );


                    region = DFB_REGION_INIT_FROM_RECTANGLE( &rect );
                    window_surface1->Flip( window_surface1, &region, 0 );

                    winupdate = 0;
                    }
               }
          }
     }

     buffer->Release( buffer );
     font->Release( font );
     
     if ( NULL != window2 && NULL != window_surface2 )
     {
          window_surface2->Release( window_surface2 );
          window2->Release( window2 );
     }
     
     if ( NULL != window1 && NULL != window_surface1 )
     {
          window_surface1->Release( window_surface1 );
          window1->Release( window1 );
     }

     layer->Release( layer );
     dfb->Release( dfb );

     return 42;
}
