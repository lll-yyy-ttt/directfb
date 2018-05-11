/* 
   (c) Copyright 2001-2002  convergence integrated media GmbH.
   (c) Copyright 2002-2005  convergence GmbH.

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the
   Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <directfb.h>

#include "lite_internal.h"
#include "lite_config.h"

#include "font.h"
#include "window.h"
#include "cursor.h"
#include "lite.h"

D_DEBUG_DOMAIN(LiteCoreDomain, "LiTE/Core", "Lite Core");

IDirectFB             *lite_dfb    = NULL;
IDirectFBDisplayLayer *lite_layer  = NULL;
LiteCursor             lite_cursor = { NULL, 0, 0 };

#if LITE_HAS_FUSIONDALE
IFusionDale           *lite_dale    = NULL;
IComa                 *lite_coma    = NULL;
IComaComponent        *lite_oskComp = NULL;
#endif


static DFBResult init_default_cursor(void);
static DFBResult free_default_cursor(void);

static int lite_refs = 0;

DFBResult 
lite_open(int *argc, char **argv[])
{
     if (!lite_refs) {
          DFBResult ret;

          D_DEBUG_AT(LiteCoreDomain, "Open new LiTE instance...\n");

          ret = DirectFBInit(argc, argv);
          if (ret) {
               DirectFBError("lite_init: DirectFBInit", ret);
               return ret;
          }

          ret = DirectFBCreate(&lite_dfb);
          if (ret) {
               DirectFBError("lite_init: DirectFBCreate", ret);
               lite_dfb = NULL;
               return ret;
          }

          ret = lite_dfb->GetDisplayLayer(lite_dfb, DLID_PRIMARY, &lite_layer);
          if (ret) {
               DirectFBError("lite_init: GetDisplayLayer", ret);
               lite_layer = NULL;
               lite_dfb->Release(lite_dfb);
               lite_dfb = NULL;
               return ret;
          }

          ret = prvlite_create_event_buffer(lite_dfb);
          if (ret) {
               DirectFBError("lite_init: Create main event loop", ret);
               lite_layer = NULL;
               lite_dfb->Release(lite_dfb);
               lite_dfb = NULL;
               return ret;
          }
          
          prvlite_font_init();

          if (!getenv("LITE_NO_THEME") && LOAD_DEFAULT_WINDOW_THEME)
               prvlite_load_default_window_theme();

          if (!getenv("LITE_NO_CURSOR"))
               ret = init_default_cursor();

#if LITE_HAS_FUSIONDALE
          ret = FusionDaleInit(argc, argv);
          if (ret) {
               DirectFBError("lite_init: FusionDaleInit() failed!", ret);
               lite_dale = NULL;
               lite_layer = NULL;
               lite_dfb->Release(lite_dfb);
               lite_dfb = NULL;
               return ret;
          }

          ret = FusionDaleCreate(&lite_dale);
          if (ret) {
               DirectFBError("lite_init: FusionDaleCreate() failed!", ret);
               lite_coma = NULL;
               lite_dale = NULL;
               lite_layer = NULL;
               lite_dfb->Release(lite_dfb);
               lite_dfb = NULL;
               return ret;
          }

          ret = lite_dale->EnterComa(lite_dale, "directfb.org", &lite_coma);
          if (ret) {
               DirectFBError("lite_init: EnterComa() failed!", ret);
               lite_coma = NULL;
               lite_dale->Release(lite_dale);
               lite_dale = NULL;
               lite_layer = NULL;
               lite_dfb->Release(lite_dfb);
               lite_dfb = NULL;
               return ret;
          }

          ret = lite_dale_get_component("OSK", &lite_oskComp);
#endif
     }
     else {
          D_DEBUG_AT(LiteCoreDomain, "Another ref to existing LiTE instance, ref: %d...\n", lite_refs); 
     }

     lite_refs++;

     return DFB_OK;
}

DFBResult 
lite_close(void)
{
     D_DEBUG_AT(LiteCoreDomain, "Close LiTE instance...\n");

     if (lite_refs) {
          if (!--lite_refs) {
               D_DEBUG_AT(LiteCoreDomain, "Free existing DFB resources...\n");

#if LITE_HAS_FUSIONDALE
               if(lite_oskComp)
            	   lite_oskComp->Release(lite_oskComp);
               lite_oskComp = NULL;
               lite_coma->Release(lite_coma);
               lite_coma = NULL;
               lite_dale->Release(lite_dale);
               lite_dale = NULL;
#endif

               prvlite_release_event_buffer();
               prvlite_release_window_resources();
               free_default_cursor();

               lite_layer->Release(lite_layer);
               lite_layer = NULL;

               lite_dfb->Release(lite_dfb);
               lite_dfb   = NULL;
          }
     }
     
     return DFB_OK;
}

IDirectFB* 
lite_get_dfb_interface(void)
{
     return lite_dfb;
}

DFBResult 
lite_get_layer_interface(IDirectFBDisplayLayer **ret_layer)
{
    *ret_layer = lite_layer;
 
    return DFB_OK; 
}

DFBResult
lite_get_layer_size( int *ret_width,
                     int *ret_height )
{
     DFBResult             ret;
     DFBDisplayLayerConfig config;

     if (!lite_layer)
          return DFB_DEAD;

     ret = lite_layer->GetConfiguration( lite_layer, &config );
     if (ret) {
          D_DERROR( ret, "LiTE: IDirectFBDisplayLayer::GetConfiguration() failed!\n" );
          return ret;
     }

     if (ret_width)
          *ret_width = config.width;

     if (ret_height)
          *ret_height = config.height;

     return DFB_OK;
}

static DFBResult init_default_cursor(void)
{
     DFBResult res = DFB_OK;

     D_DEBUG_AT(LiteCoreDomain, "Initialize default cursor...\n");

     res = lite_load_cursor_from_file(&lite_cursor, DATADIR "/" DEFAULT_WINDOW_CURSOR);

     if (res == DFB_OK && lite_cursor.surface) {
          res = lite_set_current_cursor(&lite_cursor);          
          if (res != DFB_OK)
               return res; 

          /* adjust a possible cursor hotspot position as well */
          res = lite_set_cursor_hotspot(&lite_cursor, 
                                        DEFAULT_WINDOW_CURSOR_HOTSPOT_X,
                                        DEFAULT_WINDOW_CURSOR_HOTSPOT_Y);
     }

     return res;
}

static DFBResult free_default_cursor(void)
{
     DFBResult res = DFB_OK;

     D_DEBUG_AT(LiteCoreDomain, "Free default cursor...\n");

     if (lite_cursor.surface)
          lite_cursor.surface->Release(lite_cursor.surface);

     lite_cursor.surface = NULL; 

     return res;
}


#if LITE_HAS_FUSIONDALE
IComa*
lite_get_coma_interface(void)
{
	return lite_coma;
}

IComaComponent*
lite_get_osk_component(void)
{
	return lite_oskComp;
}

DFBResult
lite_dale_get_component( const char* name, IComaComponent** component )
{
	DFBResult res = DFB_OK;
	res = lite_coma->GetComponent(lite_coma, name, 500, component);
	if(res)
		D_DERROR( res, "LiTE: lite_dale_get_component ( %s ) failed!\n", name );
	return res;
}

DFBResult
lite_dale_get_local( unsigned int bytes, void** ret )
{
	DFBResult res = DFB_OK;
	res = lite_coma->GetLocal(lite_coma, bytes, ret);
	if(res)
		D_DERROR( res, "LiTE: lite_dale_get_local failed!\n" );
	return res;
}

DFBResult
lite_dale_call( IComaComponent* component, ComaMethodID method, void* arg )
{
	DFBResult res = DFB_OK;
	int ret_val;
	res = component->Call(component, method, arg, &ret_val);
	if(res)
		D_DERROR( res, "LiTE: lite_dale_call failed!\n" );
	return res;
}
#endif
