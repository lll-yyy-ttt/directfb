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

#ifndef __SAWMAN__SAWMAN_CONFIG_H__
#define __SAWMAN__SAWMAN_CONFIG_H__

#include <directfb.h>
#include <core/coretypes.h>
#include <sawman.h>

typedef enum {
     SAWMAN_BORDER_TYPE_NONE,
     SAWMAN_BORDER_TYPE_COLOR,
     SAWMAN_BORDER_TYPE_THEME,
} SaWManBorderType;

typedef struct {
     int                   thickness;
     DFBColor              focused[4];
     DFBColor              unfocused[4];
     int                   focused_index[4];
     int                   unfocused_index[4];
} SaWManBorderColor;

typedef struct {
     int                   top;
     int                   top_edge;
     int                   bottom;
     int                   left;
     int                   right;

     CoreSurface          *theme_surface_top;
     CoreSurface          *theme_surface_bottom;
     CoreSurface          *theme_surface_left;
     CoreSurface          *theme_surface_right;

     CoreSurface          *close;
     DFBRectangle          close_rect;
     DFBColor              close_select;

     CoreSurface          *min;
     DFBRectangle          min_rect;
     DFBColor              min_select;

     CoreSurface          *max;
     DFBRectangle          max_rect;
     DFBColor              max_select;

     int                   min_window_width;
     int                   min_window_height;

     DFBInsets             desktop_insets;

     CoreSurface          *cursor;
     CoreSurface          *l;
     CoreSurface          *r;
     CoreSurface          *t;
     CoreSurface          *b;
     CoreSurface          *lt;
     CoreSurface          *lb;
     CoreSurface          *rt;
     CoreSurface          *rb;

     int          cursor_hot_x;
     int          cursor_hot_y;

     int          l_hot_x;
     int          l_hot_y;

     int          r_hot_x;
     int          r_hot_y;

     int          t_hot_x;
     int          t_hot_y;

     int          b_hot_x;
     int          b_hot_y;

     int          lt_hot_x;
     int          lt_hot_y;

     int          lb_hot_x;
     int          lb_hot_y;

     int          rt_hot_x;
     int          rt_hot_y;

     int          rb_hot_x;
     int          rb_hot_y;

     CoreFont             *font;
     DFBTextEncodingID     encoding;
     DFBColor              title_color;
} SaWManBorderTheme;

typedef struct {
     SaWManBorderType      type;

     DFBDimension          resolution;
     DFBSurfacePixelFormat format;

     SaWManBorderColor     color;
     SaWManBorderTheme     theme;
} SaWManBorderInit;

typedef struct {
     SaWManBorderInit     *border;
     SaWManBorderInit      borders[3];

     bool                  show_empty;  /* Don't hide layer when no window is visible. */

     unsigned int          flip_once_timeout;

     struct {
          bool                     hw;
          DFBDisplayLayerID        layer_id;
     }                     cursor;

     DFBDimension          resolution;

     bool                  static_layer;

     int                   update_region_mode;

     bool                  keep_implicit_key_grabs;

     DFBDimension          passive3d_mode;

     bool                  hide_cursor_without_window;
} SaWManConfig;


extern SaWManConfig *sawman_config;


/*
 * Allocate Config struct, fill with defaults and parse command line options
 * for overrides. Options identified as SaWMan options are stripped out
 * of the array.
 */
DirectResult sawman_config_init( int *argc, char **argv[] );

/* remove config structure, if there is any */
DirectResult sawman_config_shutdown( void );

/*
 * Set individual option. Used by sawman_config_init(), and SaWManSetOption()
 */
DirectResult sawman_config_set( const char *name, const char *value );

const char *sawman_config_usage( void );

void sawman_load_theme( CoreDFB *core );
void sawman_unload_theme( void );

void sawman_adjust_window_bounds( const SaWManWindow *sawwin, DFBRectangle *bounds );

DirectResult sawman_window_maximization( SaWManWindow *sawwin, DFBBoolean recover );


#endif /* __SAWMAN__SAWMAN_CONFIG_H__ */

