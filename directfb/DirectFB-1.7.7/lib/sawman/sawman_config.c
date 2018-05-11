/*
   (c) Copyright 2012-2013  DirectFB integrated media GmbH
   (c) Copyright 2001-2013  The world wide DirectFB Open Source Community (directfb.org)
   (c) Copyright 2000-2004  Convergence (integrated media) GmbH

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Shimokawa <andi@directfb.org>,
              Marek Pikarski <mass@directfb.org>,
              Sven Neumann <neo@directfb.org>,
              Ville Syrjälä <syrjala@sci.fi> and
              Claudio Ciccani <klan@users.sf.net>.

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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sawman.h>

#include <direct/conf.h>
#include <direct/debug.h>
#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/util.h>

#include <misc/conf.h>

#include "sawman_config.h"
#include "sawman_window.h"

#include <core/surface.h>

#include "theme/theme-close.h"
#include "theme/theme-min.h"
#include "theme/theme-max.h"

#include "theme/theme-cursor.h"
#include "theme/theme-l.h"
#include "theme/theme-t.h"
#include "theme/theme-lt.h"
#include "theme/theme-rt.h"

#include "sawman_font.c"


SaWManConfig *sawman_config = NULL;

static const char *config_usage =
     "SaWMan Configuration\n"
     "\n"
     " --sawman-help                       Output SaWMan usage information and exit\n"
     " --sawman:<option>[,<option>]...     Pass options to SaWMan (see below)\n"
     "\n"
     "SaWMan options:\n"
     "\n"
     "  init-border=<num>                  Set border values for tier (0-2)\n"
     "  border-thickness=<num>             border thickness\n"
     "  border-resolution=<width>x<height> Set border's width and height\n"
     "  border-format=<pixelformat>        Set border's pixelformat\n"
     "  border-focused-color-index<0|1|2|3>=<color>   Set focused border's color\n"
     "  border-unfocused-color-index<0|1|2|3>=<color> Set unfocused border's color\n"
     "  border-focused-color<0|1|2|3>=AARRGGBB   Set focused border's color\n"
     "  border-unfocused-color<0|1|2|3>=AARRGGBB Set unfocused border's color\n"
     "  [no-]show-empty                    Show layer even if no window is visible\n"
     "  flip-once-timeout=<num>            Flip once timeout\n"
     "  hw-cursor=<layer-id>               Set HW Cursor mode\n"
     "  resolution=<width>x<height>        Set virtual SaWMan resolution\n"
     "  [no-]static-layer                  Disable layer reconfiguration\n"
     "  update-region-mode=<num>           Set internal update region mode (1-3, default 2)\n"
     "  keep-implicit-key-grabs            Causes implicit key grabs to stay even when window is withdrawn\n"
     "  hide-cursor-without-window         Hides the cursor when no window has control over it\n"
     "\n";


static DFBResult
parse_args( const char *args )
{
     char *buf = alloca( strlen(args) + 1 );

     strcpy( buf, args );

     while (buf && buf[0]) {
          DFBResult  ret;
          char      *value;
          char      *next;

          if ((next = strchr( buf, ',' )) != NULL)
               *next++ = '\0';

          if (strcmp (buf, "help") == 0) {
               fprintf( stderr, "%s", config_usage );
               exit(1);
          }

          if ((value = strchr( buf, '=' )) != NULL)
               *value++ = '\0';

          ret = sawman_config_set( buf, value );
          switch (ret) {
               case DFB_OK:
                    break;
               case DFB_UNSUPPORTED:
                    D_ERROR( "SaWMan/Config: Unknown option '%s'!\n", buf );
                    break;
               default:
                    return ret;
          }

          buf = next;
     }

     return DFB_OK;
}

static DFBResult
load_theme_image( CoreDFB *core, const DFBSurfaceDescription *desc, CoreSurface **surface )
{
     DFBResult ret;

     D_ASSERT( core != NULL );

     D_ASSERT( desc != NULL );

     ret = dfb_surface_create_simple( core, desc->width, desc->height, desc->pixelformat,
                                      desc->colorspace, DSCAPS_NONE, CSTF_SHARED, 0, NULL, surface );
     if (ret) {
          D_DERROR( ret, "Sawman/WM: Could not create %dx%d surface for border tiles!\n",
                    desc->width, desc->height );
          return ret;
     }

     ret = dfb_surface_write_buffer( *surface, CSBR_BACK,
                                     desc->preallocated[0].data, desc->preallocated[0].pitch, NULL );
     if (ret)
          D_DERROR( ret, "Sawman/WM: Could not write to %dx%d surface for border tiles!\n",
                    desc->width, desc->height );

     dfb_surface_globalize( *surface );

     return DFB_OK;
}

void sawman_load_theme( CoreDFB *core )
{
     D_ASSERT( core != NULL );
     D_ASSERT( sawman_config != NULL );

     const SaWManBorderInit *border = &sawman_config->borders[1];
     if ( SAWMAN_BORDER_TYPE_THEME == border->type )
     {
          load_theme_image( core, &theme_close_desc, &sawman_config->borders[1].theme.close );
          load_theme_image( core, &theme_min_desc, &sawman_config->borders[1].theme.min );
          load_theme_image( core, &theme_max_desc, &sawman_config->borders[1].theme.max );

          load_theme_image( core, &theme_cursor_desc, &sawman_config->borders[1].theme.cursor );
          load_theme_image( core, &theme_l_desc, &sawman_config->borders[1].theme.l );
          load_theme_image( core, &theme_t_desc, &sawman_config->borders[1].theme.t );
          load_theme_image( core, &theme_lt_desc, &sawman_config->borders[1].theme.lt );
          load_theme_image( core, &theme_rt_desc, &sawman_config->borders[1].theme.rt );

          sawman_config->borders[1].theme.r = sawman_config->borders[1].theme.l;
          sawman_config->borders[1].theme.b = sawman_config->borders[1].theme.t;
          sawman_config->borders[1].theme.lb = sawman_config->borders[1].theme.rt;
          sawman_config->borders[1].theme.rb = sawman_config->borders[1].theme.lt;

          sawman_config->borders[1].theme.cursor_hot_x = 0;
          sawman_config->borders[1].theme.cursor_hot_y = 0;

          sawman_config->borders[1].theme.l_hot_x = 16;
          sawman_config->borders[1].theme.l_hot_y = 16;

          sawman_config->borders[1].theme.r_hot_x = 16;
          sawman_config->borders[1].theme.r_hot_y = 16;

          sawman_config->borders[1].theme.t_hot_x = 16;
          sawman_config->borders[1].theme.t_hot_y = 16;

          sawman_config->borders[1].theme.b_hot_x = 16;
          sawman_config->borders[1].theme.b_hot_y = 16;

          sawman_config->borders[1].theme.lt_hot_x = 16;
          sawman_config->borders[1].theme.lt_hot_y = 16;

          sawman_config->borders[1].theme.lb_hot_x = 16;
          sawman_config->borders[1].theme.lb_hot_y = 16;

          sawman_config->borders[1].theme.rt_hot_x = 16;
          sawman_config->borders[1].theme.rt_hot_y = 16;

          sawman_config->borders[1].theme.rb_hot_x = 16;
          sawman_config->borders[1].theme.rb_hot_y = 16;

          DFBFontDescription desc;
          desc.flags = DFDESC_HEIGHT;
          desc.height = 16;
          if (DFB_OK != sawman_load_font( core, "decker.ttf", &desc, &sawman_config->borders[1].theme.font ))
          {
               D_ERROR( "Sawman/WM: Could not load font decker.dgiff!\n" );
          }
          sawman_config->borders[1].theme.encoding = 0;
     }
}

void sawman_unload_theme( void )
{
     const SaWManBorderInit *border = &sawman_config->borders[1];
     if ( SAWMAN_BORDER_TYPE_THEME == border->type )
     {
          if ( NULL != sawman_config->borders[1].theme.close )
               dfb_surface_unlink( &sawman_config->borders[1].theme.close );

          if ( NULL != sawman_config->borders[1].theme.min )
               dfb_surface_unlink( &sawman_config->borders[1].theme.min );

          if ( NULL != sawman_config->borders[1].theme.max )
               dfb_surface_unlink( &sawman_config->borders[1].theme.max );

          if ( NULL != sawman_config->borders[1].theme.font )
               sawman_unload_font( sawman_config->borders[1].theme.font );
     }
}

void sawman_adjust_window_bounds( const SaWManWindow *sawwin, DFBRectangle *bounds )
{
     D_MAGIC_ASSERT( sawwin, SaWManWindow );
     D_ASSERT( sawman_config != NULL );
     D_ASSERT( bounds != NULL );

     CoreWindow *window = sawwin->window;

     const SaWManBorderInit *border = &sawman_config->borders[sawman_window_priority(sawwin)];
     if ( SAWMAN_BORDER_TYPE_THEME == border->type &&
          sawman_window_border( sawwin ) )
     {
          bounds->w += border->theme.left + border->theme.right;
          bounds->h += border->theme.top + border->theme.bottom;
     }
}

DirectResult sawman_window_maximization( SaWManWindow *sawwin, DFBBoolean recover )
{
     D_MAGIC_ASSERT( sawwin, SaWManWindow );
     D_ASSERT( sawman_config != NULL );

     CoreWindow *window = sawwin->window;
     D_MAGIC_COREWINDOW_ASSERT( window );

     SaWManTier *tier = sawman_tier_by_class( sawwin->sawman, window->config.stacking );
     D_MAGIC_ASSERT( tier, SaWManTier );

     const SaWManBorderInit *border = &sawman_config->borders[sawman_window_priority(sawwin)];
     if ( SAWMAN_BORDER_TYPE_THEME == border->type &&
          sawman_window_border( sawwin ) )
     {
          if (!recover)
          {
               if (!sawwin->is_maximization)
               {
                    sawwin->recover_x = window->config.bounds.x;
                    sawwin->recover_y = window->config.bounds.y;
                    sawwin->recover_width  = window->config.bounds.w;
                    sawwin->recover_height = window->config.bounds.h;

                    dfb_window_set_bounds( window, -border->theme.left, 0, tier->stack->width, tier->stack->height-border->theme.top );

                    sawwin->is_maximization = true;
               }
          }
          else
          {
               if (sawwin->is_maximization)
               {
                    dfb_window_set_bounds( window, sawwin->recover_x, sawwin->recover_y, sawwin->recover_width, sawwin->recover_height );

                    sawwin->is_maximization = false;
               }
          }
     }

     return DFB_OK;
}

static DFBResult
config_allocate( void )
{
     int i;

     if (sawman_config)
          return DFB_OK;

     sawman_config = D_CALLOC( 1, sizeof(SaWManConfig) );
     if (!sawman_config)
          return D_OOM();

#if 0
     sawman_config->border = &sawman_config->borders[0];

     sawman_config->borders[1].type = SAWMAN_BORDER_TYPE_COLOR;

     sawman_config->borders[0].color.thickness    = 4;
     sawman_config->borders[0].color.focused[0]   = (DFBColor){ 0xff, 0xc0, 0x00, 0x00 };
     sawman_config->borders[0].color.focused[1]   = (DFBColor){ 0xff, 0xb0, 0x00, 0x00 };
     sawman_config->borders[0].color.focused[2]   = (DFBColor){ 0xff, 0xa0, 0x00, 0x00 };
     sawman_config->borders[0].color.focused[3]   = (DFBColor){ 0xff, 0x90, 0x00, 0x00 };
     sawman_config->borders[0].color.unfocused[0] = (DFBColor){ 0xff, 0x80, 0x80, 0x80 };
     sawman_config->borders[0].color.unfocused[1] = (DFBColor){ 0xff, 0x70, 0x70, 0x70 };
     sawman_config->borders[0].color.unfocused[2] = (DFBColor){ 0xff, 0x60, 0x60, 0x60 };
     sawman_config->borders[0].color.unfocused[3] = (DFBColor){ 0xff, 0x50, 0x50, 0x50 };

     for(i=0;i<4;i++) {
          sawman_config->borders[0].color.focused_index[i]   = -1;
          sawman_config->borders[0].color.unfocused_index[i] = -1;
     }
#endif

#if 1
     sawman_config->border = &sawman_config->borders[1];

     sawman_config->borders[1].type = SAWMAN_BORDER_TYPE_THEME;

     sawman_config->borders[1].theme.top = 22;
     sawman_config->borders[1].theme.top_edge = 2;
     sawman_config->borders[1].theme.bottom = 2;
     sawman_config->borders[1].theme.left = 2;
     sawman_config->borders[1].theme.right = 2;

     sawman_config->borders[1].theme.theme_surface_top = NULL;
     sawman_config->borders[1].theme.theme_surface_bottom = NULL;
     sawman_config->borders[1].theme.theme_surface_left = NULL;
     sawman_config->borders[1].theme.theme_surface_right = NULL;

     sawman_config->borders[1].theme.close = NULL;
     sawman_config->borders[1].theme.close_rect.x = sawman_config->borders[1].theme.left;
     sawman_config->borders[1].theme.close_rect.y = sawman_config->borders[1].theme.top_edge;
     sawman_config->borders[1].theme.close_rect.w = sawman_config->borders[1].theme.top;
     sawman_config->borders[1].theme.close_rect.h = sawman_config->borders[1].theme.top - sawman_config->borders[1].theme.top_edge - 1;
     sawman_config->borders[1].theme.close_select = (DFBColor){ 0xff, 0xff, 0x0, 0x0 };

     sawman_config->borders[1].theme.min = NULL;
     sawman_config->borders[1].theme.min_rect.x = sawman_config->borders[1].theme.close_rect.x + sawman_config->borders[1].theme.close_rect.w + 1;
     sawman_config->borders[1].theme.min_rect.y = sawman_config->borders[1].theme.top_edge;
     sawman_config->borders[1].theme.min_rect.w = sawman_config->borders[1].theme.top;
     sawman_config->borders[1].theme.min_rect.h = sawman_config->borders[1].theme.top - sawman_config->borders[1].theme.top_edge - 1;
     sawman_config->borders[1].theme.min_select = (DFBColor){ 0xff, 0xc0, 0xc0, 0xc0 };

     sawman_config->borders[1].theme.max = NULL;
     sawman_config->borders[1].theme.max_rect.x = sawman_config->borders[1].theme.min_rect.x + sawman_config->borders[1].theme.min_rect.w + 1;
     sawman_config->borders[1].theme.max_rect.y = sawman_config->borders[1].theme.top_edge;
     sawman_config->borders[1].theme.max_rect.w = sawman_config->borders[1].theme.top;
     sawman_config->borders[1].theme.max_rect.h = sawman_config->borders[1].theme.top - sawman_config->borders[1].theme.top_edge - 1;
     sawman_config->borders[1].theme.max_select = (DFBColor){ 0xff, 0xc0, 0xc0, 0xc0 };

     sawman_config->borders[1].theme.min_window_width = sawman_config->borders[1].theme.max_rect.x + sawman_config->borders[1].theme.max_rect.w + 20;
     sawman_config->borders[1].theme.min_window_height = sawman_config->borders[1].theme.top + sawman_config->borders[1].theme.bottom + 40;

     sawman_config->borders[1].theme.title_color = (DFBColor){ 0xff, 0x0, 0x0, 0x0 };

     sawman_config->borders[1].theme.desktop_insets.l = 0;
     sawman_config->borders[1].theme.desktop_insets.t = 0;
     sawman_config->borders[1].theme.desktop_insets.r = 0;
     sawman_config->borders[1].theme.desktop_insets.b = 40;
#endif

     sawman_config->update_region_mode = 4;

     sawman_config->static_layer = true;


     return DFB_OK;
}

const char*
sawman_config_usage( void )
{
     return config_usage;
}

DirectResult
sawman_config_set( const char *name, const char *value )
{
     if (strcmp (name, "init-border" ) == 0) {
          if (value) {
               int index;

               if (sscanf( value, "%d", &index ) < 1) {
                    D_ERROR("SaWMan/Config '%s': Could not parse value!\n", name);
                    return DFB_INVARG;
               }

               if (index < 0 || index > D_ARRAY_SIZE(sawman_config->borders)) {
                    D_ERROR("SaWMan/Config '%s': Value %d out of bounds!\n", name, index);
                    return DFB_INVARG;
               }

               sawman_config->borders[index].type = SAWMAN_BORDER_TYPE_COLOR;

               sawman_config->border = &sawman_config->borders[index];
          }
          else {
               D_ERROR("SaWMan/Config '%s': No value specified!\n", name);
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "border-thickness" ) == 0) {
          SaWManBorderInit *border = sawman_config->border;

          if (value) {
               if (sscanf( value, "%d", &border->color.thickness ) < 1) {
                    D_ERROR("SaWMan/Config '%s': Could not parse value!\n", name);
                    return DFB_INVARG;
               }
          }
          else {
               D_ERROR("SaWMan/Config '%s': No value specified!\n", name);
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "border-resolution" ) == 0) {
          SaWManBorderInit *border = sawman_config->border;

          if (value) {
               int width, height;

               if (sscanf( value, "%dx%d", &width, &height ) < 2) {
                    D_ERROR("SaWMan/Config '%s': Could not parse dimension!\n", name);
                    return DFB_INVARG;
               }

               border->resolution.w = width;
               border->resolution.h = height;
          }
          else {
               D_ERROR("SaWMan/Config '%s': No width and height specified!\n", name);
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "border-format" ) == 0) {
          SaWManBorderInit *border = sawman_config->border;

          if (value) {
               DFBSurfacePixelFormat format;

               format = dfb_config_parse_pixelformat( value );
               if (format == DSPF_UNKNOWN) {
                    D_ERROR("SaWMan/Config '%s': Could not parse format!\n", name);
                    return DFB_INVARG;
               }

               border->format = format;
          }
          else {
               D_ERROR("SaWMan/Config '%s': No format specified!\n", name);
               return DFB_INVARG;
          }
     } else
     if (strncmp (name, "border-focused-color-index", 26 ) == 0 || strncmp (name, "border-unfocused-color-index", 28 ) == 0) {
          SaWManBorderInit *border = sawman_config->border;
          int               cindex = (name[7] == 'f') ? (name[26] - '0') : (name[28] - '0');

          if (cindex < 0 || cindex > D_ARRAY_SIZE(border->color.focused)) {
               D_ERROR("SaWMan/Config '%s': Value %d out of bounds!\n", name, cindex);
               return DFB_INVARG;
          }

          if (value) {
               char *error;
               u32   index;

               index = strtoul( value, &error, 10 );

               if (*error) {
                    D_ERROR( "SaWMan/Config '%s': Error in index '%s'!\n", name, error );
                    return DFB_INVARG;
               }

               if (strncmp (name, "border-focused-color-index", 26 ) == 0)
                    border->color.focused_index[cindex] = index;
               else
                    border->color.unfocused_index[cindex] = index;
          }
          else {
               D_ERROR( "SaWMan/Config '%s': No index specified!\n", name );
               return DFB_INVARG;
          }
     } else
     if (strncmp (name, "border-focused-color", 20 ) == 0 || strncmp (name, "border-unfocused-color", 22 ) == 0) {
          SaWManBorderInit *border = sawman_config->border;
          int               cindex = (name[7] == 'f') ? (name[20] - '0') : (name[22] - '0');

          if (cindex < 0 || cindex > D_ARRAY_SIZE(border->color.focused)) {
               D_ERROR("SaWMan/Config '%s': Value %d out of bounds!\n", name, cindex);
               return DFB_INVARG;
          }

          if (value) {
               char *error;
               u32   argb;

               argb = strtoul( value, &error, 16 );

               if (*error) {
                    D_ERROR( "SaWMan/Config '%s': Error in color '%s'!\n", name, error );
                    return DFB_INVARG;
               }

               if (strncmp (name, "border-focused-color", 20 ) == 0) {
                    border->color.focused[cindex].a = (argb & 0xFF000000) >> 24;
                    border->color.focused[cindex].r = (argb & 0xFF0000) >> 16;
                    border->color.focused[cindex].g = (argb & 0xFF00) >> 8;
                    border->color.focused[cindex].b = (argb & 0xFF);
                    border->color.focused_index[cindex] = -1;
               }
               else {
                    border->color.unfocused[cindex].a = (argb & 0xFF000000) >> 24;
                    border->color.unfocused[cindex].r = (argb & 0xFF0000) >> 16;
                    border->color.unfocused[cindex].g = (argb & 0xFF00) >> 8;
                    border->color.unfocused[cindex].b = (argb & 0xFF);
                    border->color.unfocused_index[cindex] = -1;
               }
          }
          else {
               D_ERROR( "SaWMan/Config '%s': No color specified!\n", name );
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "show-empty") == 0) {
          sawman_config->show_empty = true;
     } else
     if (strcmp (name, "no-show-empty") == 0) {
          sawman_config->show_empty = false;
     } else
     if (strcmp (name, "flip-once-timeout") == 0) {
          if (value) {
               char *error;
               u32   timeout;

               timeout = strtoul( value, &error, 10 );

               if (*error) {
                    D_ERROR( "SaWMan/Config '%s': Error in timeout '%s'!\n", name, error );
                    return DFB_INVARG;
               }

               sawman_config->flip_once_timeout = timeout;
          }
          else {
               D_ERROR( "SaWMan/Config '%s': No timeout specified!\n", name );
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "hw-cursor" ) == 0) {
          if (value) {
               int id;

               if (sscanf( value, "%d", &id ) < 1) {
                    D_ERROR("SaWMan/Config '%s': Could not parse value!\n", name);
                    return DFB_INVARG;
               }

               if (id < 0 || id >= MAX_LAYERS) {
                    D_ERROR("SaWMan/Config '%s': Value %d out of bounds!\n", name, id);
                    return DFB_INVARG;
               }

               sawman_config->cursor.hw       = true;
               sawman_config->cursor.layer_id = id;
          }
          else {
               D_ERROR("SaWMan/Config '%s': No value specified!\n", name);
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "resolution" ) == 0) {
          if (value) {
               int width, height;

               if (sscanf( value, "%dx%d", &width, &height ) < 2) {
                    D_ERROR("SaWMan/Config '%s': Could not parse dimension!\n", name);
                    return DFB_INVARG;
               }

               sawman_config->resolution.w = width;
               sawman_config->resolution.h = height;
          }
          else {
               D_ERROR("SaWMan/Config '%s': No width and height specified!\n", name);
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "passive3d-mode" ) == 0) {
          if (value) {
               int width, height;

               if (sscanf( value, "%dx%d", &width, &height ) < 2) {
                    D_ERROR("SaWMan/Config '%s': Could not parse dimension!\n", name);
                    return DFB_INVARG;
               }

               sawman_config->passive3d_mode.w = width;
               sawman_config->passive3d_mode.h = height;
          }
          else {
               D_ERROR("SaWMan/Config '%s': No width and height specified!\n", name);
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "static-layer") == 0) {
          sawman_config->static_layer = true;
     } else
     if (strcmp (name, "no-static-layer") == 0) {
          sawman_config->static_layer = false;
     } else
     if (strcmp (name, "update-region-mode" ) == 0) {
          if (value) {
               int mode;

               if (sscanf( value, "%d", &mode ) < 1) {
                    D_ERROR("SaWMan/Config '%s': Could not parse value!\n", name);
                    return DFB_INVARG;
               }
               if (mode < 1 || mode > 4) {
                    D_ERROR("SaWMan/Config '%s': Value %d out of bounds!\n", name, mode);
                    return DFB_INVARG;
               }
               sawman_config->update_region_mode = mode;
          }
          else {
               D_ERROR("SaWMan/Config '%s': No value specified!\n", name);
               return DFB_INVARG;
          }
     } else
     if (strcmp (name, "keep-implicit-key-grabs") == 0) {
          sawman_config->keep_implicit_key_grabs = true;
     } else
     if (strcmp (name, "hide-cursor-without-window") == 0) {
          sawman_config->hide_cursor_without_window = true;
     } else
          return DFB_UNSUPPORTED;

     return DFB_OK;
}

static DFBResult 
sawman_config_read( const char *filename )
{
     DFBResult  ret = DFB_OK;
     char       line[400];
     FILE      *f;

     f = fopen( filename, "r" );
     if (!f) {
          D_DEBUG( "SaWMan/Config: "
                   "Unable to open config file `%s'!\n", filename );
          return DFB_IO;
     } else {
          D_INFO( "SaWMan/Config: "
                  "Parsing config file '%s'.\n", filename );
     }

     while (fgets( line, 400, f )) {
          char *name  = line;
          char *value = strchr( line, '=' );

          if (value) {
               *value++ = 0;
               direct_trim( &value );
          }

          direct_trim( &name );

          if (!*name || *name == '#')
               continue;

          ret = sawman_config_set( name, value );
          if (ret) {
               if (ret == DFB_UNSUPPORTED)
                    D_ERROR( "SaWMan/Config: In config file `%s': "
                             "Invalid option `%s'!\n", filename, name );
               break;
          }
     }

     fclose( f );

     return ret;
}

DirectResult
sawman_config_init( int *argc, char **argv[] )
{
     DFBResult ret;
     
     if (!sawman_config) {
          char *home = getenv( "HOME" );
          char *prog = NULL;
          char *swargs;

          ret = config_allocate();
          if (ret)
               return ret;
     
          /* Read system settings. */
          ret = sawman_config_read( SYSCONFDIR"/sawmanrc" );
          if (ret  &&  ret != DFB_IO)
               return ret;
               
          /* Read user settings. */
          if (home) {
               int  len = strlen(home) + sizeof("/.sawmanrc");
               char buf[len];
     
               snprintf( buf, len, "%s/.sawmanrc", home );
     
               ret = sawman_config_read( buf );
               if (ret  &&  ret != DFB_IO)
                    return ret;
          }
          
          /* Get application name. */
          if (argc && *argc && argv && *argv) {
               prog = strrchr( (*argv)[0], '/' );
     
               if (prog)
                    prog++;
               else
                    prog = (*argv)[0];
          }
     
          /* Read global application settings. */
          if (prog && prog[0]) {
               int  len = sizeof(SYSCONFDIR"/sawmanrc.") + strlen(prog);
               char buf[len];
     
               snprintf( buf, len, SYSCONFDIR"/sawmanrc.%s", prog );
     
               ret = sawman_config_read( buf );
               if (ret  &&  ret != DFB_IO)
                    return ret;
          }
          
          /* Read user application settings. */
          if (home && prog && prog[0]) {
               int  len = strlen(home) + sizeof("/.sawmanrc.") + strlen(prog);
               char buf[len];
     
               snprintf( buf, len, "%s/.sawmanrc.%s", home, prog );
     
               ret = sawman_config_read( buf );
               if (ret  &&  ret != DFB_IO)
                    return ret;
          }
          
          /* Read settings from environment variable. */
          swargs = getenv( "SAWMANARGS" );
          if (swargs) {
               ret = parse_args( swargs );
               if (ret)
                    return ret;
          }
     }
     
     /* Read settings from command line. */
     if (argc && argv) {
          int i;
          
          for (i = 1; i < *argc; i++) {

               if (!strcmp( (*argv)[i], "--sawman-help" )) {
                    fprintf( stderr, "%s", config_usage );
                    exit(1);
               }

               if (!strncmp( (*argv)[i], "--sawman:", 9 )) {
                    ret = parse_args( (*argv)[i] + 9 );
                    if (ret)
                         return ret;

                    (*argv)[i] = NULL;
               }
          }

          for (i = 1; i < *argc; i++) {
               int k;

               for (k = i; k < *argc; k++)
                    if ((*argv)[k] != NULL)
                         break;

               if (k > i) {
                    int j;

                    k -= i;

                    for (j = i + k; j < *argc; j++)
                         (*argv)[j-k] = (*argv)[j];

                    *argc -= k;
               }
          }
     }

     return DFB_OK;
}

DirectResult sawman_config_shutdown( void )
{
     D_FREE( sawman_config );
     return DFB_OK;
}
