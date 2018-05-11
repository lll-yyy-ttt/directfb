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
#include <string.h>

#include <direct/mem.h>

#include <lite/util.h>
#include <lite/window.h>

#include "scrollbar.h"

D_DEBUG_DOMAIN(LiteScrollbarDomain, "LiTE/Scrollbar", "LiteScrollbar");

LiteScrollbarTheme *liteDefaultScrollbarTheme = NULL;

typedef enum _ScrollMode{
     SCROLL_LINE,
     SCROLL_PAGE,
     SCROLL_ABSOLUTE_POS
} ScrollMode;

enum {
     GSR_BTN1,
     GSR_BTN2,
     GSR_THUMB
};

typedef enum _ScrollbarHittestArea{
     SHA_OUTSIDE,
     SHA_BTN1,
     SHA_BTN2,
     SHA_THUMB,
     SHA_BETWEEN_THUMB_BTN1,
     SHA_BETWEEN_THUMB_BTN2,
     SAH_MAXVALUE
} ScrollbarHittestArea;

typedef enum _LiteScrollbarState
{
     LSS_NORMAL,
     LSS_HILITE,
     LSS_PRESSED_BTN1,
     LSS_PRESSED_BTN2,
     LSS_PRESSED_THUMB,
     LSS_DISABLED
}LiteScrollbarState;

struct _LiteScrollbar {
     LiteBox             box;
     LiteScrollbarTheme *theme;
     LiteScrollInfo      info;
     int                 vertical;
     LiteScrollbarState  state;
     ScrollbarUpdateFunc update;
     void               *update_data;
     int                 thumb_press_offset;
};

static int on_enter(LiteBox *box, int x, int y);
static int on_leave(LiteBox *box, int x, int y);
static int on_focus_in(LiteBox *box);
static int on_focus_out(LiteBox *box);

static int on_button_down(LiteBox *box, int x, int y,
                          DFBInputDeviceButtonIdentifier button);
static int on_button_up(LiteBox *box, int x, int y,
                          DFBInputDeviceButtonIdentifier button);
static int on_motion(LiteBox *box, int x, int y,
                           DFBInputDeviceButtonMask buttons);

static DFBResult draw_scrollbar(LiteBox *box, const DFBRegion *region, DFBBoolean clear);
static DFBResult destroy_scrollbar(LiteBox *box);

static void
draw_enlarged_image(IDirectFBSurface *target_surface,
                    DFBRectangle     *target_rect,
                    IDirectFBSurface *src_image_surface,
                    DFBRectangle     *src_image_rect,
                    int               image_margin);

static void
get_scrollbar_rect( LiteScrollbar *scrollbar,
                    int            area,
                    DFBRectangle  *rc);

static int
pt_in_rect( int x, int y, DFBRectangle *rc);

static ScrollbarHittestArea
scrollbar_hittest( LiteScrollbar *scrollbar,
                   int            x,
                   int            y);

static void
on_scroll( LiteScrollbar *scrollbar,
           ScrollMode     mode,
           int            param,
           int            dragging );

int
can_scroll( LiteScrollInfo* info);

DFBResult 
lite_new_scrollbar(LiteBox         *parent, 
                DFBRectangle       *rect,
                int                 vertical,
                LiteScrollbarTheme *theme,
                LiteScrollbar     **ret_scrollbar)
{
     DFBResult   res; 
     LiteScrollbar *scrollbar = NULL;

     LITE_NULL_PARAMETER_CHECK(parent);
     LITE_NULL_PARAMETER_CHECK(rect);
     LITE_NULL_PARAMETER_CHECK(theme);
     LITE_NULL_PARAMETER_CHECK(ret_scrollbar);

     scrollbar = D_CALLOC(1, sizeof(LiteScrollbar));

     scrollbar->box.parent = parent;
     scrollbar->theme = theme;
     scrollbar->box.rect = *rect;

     res = lite_init_box(LITE_BOX(scrollbar));
     if (res != DFB_OK) {
          D_FREE(scrollbar);
          return res;
     }

     scrollbar->box.type    = LITE_TYPE_SCROLLBAR;
     scrollbar->box.Draw    = draw_scrollbar;
     scrollbar->box.Destroy = destroy_scrollbar;

     scrollbar->box.OnEnter      = on_enter;
     scrollbar->box.OnLeave      = on_leave;
     scrollbar->box.OnFocusIn    = on_focus_in;
     scrollbar->box.OnFocusOut   = on_focus_out;
     scrollbar->box.OnButtonDown = on_button_down;
     scrollbar->box.OnButtonUp   = on_button_up;
     scrollbar->box.OnMotion     = on_motion;

     scrollbar->vertical = vertical;
     scrollbar->state    = LSS_NORMAL;

     scrollbar->info.min = 0;
     scrollbar->info.max = 20;
     scrollbar->info.page_size = 7;
     scrollbar->info.line_size = 1;
     scrollbar->info.pos = 0;
     scrollbar->info.track_pos = -1;

     scrollbar->thumb_press_offset = 0;

     lite_update_box(LITE_BOX(scrollbar), NULL);

     *ret_scrollbar = scrollbar;

     D_DEBUG_AT(LiteScrollbarDomain, "Created new scrollbar object: %p\n", scrollbar);

     return DFB_OK;
}

DFBResult
lite_enable_scrollbar(LiteScrollbar *scrollbar, int enable)
{
     LITE_NULL_PARAMETER_CHECK(scrollbar);

     int curEnable = scrollbar->state != LSS_DISABLED;
     
     if( curEnable != !(!enable))
     {
          scrollbar->state = enable ? LSS_NORMAL : LSS_DISABLED;
          scrollbar->box.is_active = !(!enable);
          lite_update_box(&scrollbar->box, NULL);
     }
     
     return DFB_OK;
}

DFBResult
lite_get_scroll_info(LiteScrollbar *scrollbar, LiteScrollInfo *info)
{
     LITE_NULL_PARAMETER_CHECK(scrollbar);
     LITE_NULL_PARAMETER_CHECK(info);

     *info = scrollbar->info;

     return DFB_OK;
}

DFBResult 
lite_set_scroll_pos(LiteScrollbar *scrollbar, int pos)
{
     LITE_NULL_PARAMETER_CHECK(scrollbar);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(scrollbar), LITE_TYPE_SCROLLBAR);

     if (pos < scrollbar->info.min)
          pos = scrollbar->info.min;
     else if (pos > scrollbar->info.max)
          pos = scrollbar->info.max;

     if (pos == scrollbar->info.pos)
          return DFB_OK;

     scrollbar->info.pos = pos;

     return lite_update_box(LITE_BOX(scrollbar), NULL);
}

DFBResult
lite_get_scroll_pos(LiteScrollbar *scrollbar, int *pos)
{
     LITE_NULL_PARAMETER_CHECK(scrollbar);
     LITE_NULL_PARAMETER_CHECK(pos);

     if( scrollbar->info.track_pos != -1 )
          *pos = scrollbar->info.track_pos;
     else
          *pos = scrollbar->info.pos;

     return DFB_OK;
}

DFBResult 
lite_on_scrollbar_update(LiteScrollbar    *scrollbar,
                      ScrollbarUpdateFunc  func,
                      void                *funcdata)
{
     LITE_NULL_PARAMETER_CHECK(scrollbar);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(scrollbar), LITE_TYPE_SCROLLBAR);

     scrollbar->update = func;
     scrollbar->update_data = funcdata;

     return DFB_OK;
}

DFBResult lite_set_scroll_info(LiteScrollbar  *scrollbar,
                               LiteScrollInfo *info)
{
     int now_dragging;
     int actual_max;

     LITE_NULL_PARAMETER_CHECK(scrollbar);
     LITE_NULL_PARAMETER_CHECK(info);

     now_dragging = (scrollbar->info.track_pos != -1);
 
     scrollbar->info = *info;

     actual_max = scrollbar->info.max
          - ((scrollbar->info.page_size > 0)?scrollbar->info.page_size:0);
     if( scrollbar->info.pos < scrollbar->info.min )
          scrollbar->info.pos = scrollbar->info.min;
     if( scrollbar->info.pos > actual_max )
          scrollbar->info.pos = actual_max;
     if( scrollbar->info.track_pos != -1 )
     {
          if( scrollbar->info.track_pos < scrollbar->info.min )
               scrollbar->info.track_pos = scrollbar->info.min;
          if( scrollbar->info.track_pos > actual_max )
               scrollbar->info.track_pos = actual_max;
     }
     

     if( !now_dragging )
          scrollbar->info.track_pos = -1;

     if( ! can_scroll( info ) )
     {
          scrollbar->box.is_active = false;
          scrollbar->info.pos = scrollbar->info.min; // reset pos to min
     }
     else
     {
          if( scrollbar->state != LSS_DISABLED )
          {
               scrollbar->box.is_active = true; 
          }
     }
     
     lite_update_box(LITE_BOX(scrollbar), NULL);

     return DFB_OK;
}

DFBResult lite_new_scrollbar_theme(const char          *image_path,
                                   int                  image_margin,
                                   LiteScrollbarTheme **ret_theme)
{
     DFBResult res;
     LiteScrollbarTheme *theme;

     LITE_NULL_PARAMETER_CHECK(image_path);
     LITE_NULL_PARAMETER_CHECK(ret_theme);

     theme = D_CALLOC(1, sizeof( LiteScrollbarTheme) );
     memset(theme, 0, sizeof( LiteScrollbarTheme) );

     theme->image_margin = image_margin;  

     res = lite_util_load_image(image_path, DSPF_UNKNOWN,
                                &theme->all_images.surface,
                                &theme->all_images.width,
                                &theme->all_images.height,
                                NULL);
     if( res != DFB_OK )
          goto error;

     *ret_theme = theme;
     return DFB_OK;

error:
     D_FREE(theme);

     return res;
}

DFBResult lite_destroy_scrollbar_theme(LiteScrollbarTheme *theme)
{
     LITE_NULL_PARAMETER_CHECK(theme);

     theme->all_images.surface->Release(
          theme->all_images.surface);

     D_FREE(theme);

     return DFB_OK;
}



/* internals */

static DFBResult 
destroy_scrollbar(LiteBox *box)
{
     LiteScrollbar *scrollbar =  NULL;

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_SCROLLBAR);

     scrollbar = LITE_SCROLLBAR(box);

     D_DEBUG_AT(LiteScrollbarDomain, "Destroy scrollbar: %p\n", scrollbar);

     if (!scrollbar)
          return DFB_FAILURE;

     return lite_destroy_box(box);
}

static DFBResult 
draw_scrollbar(LiteBox        *box, 
              const DFBRegion *region, 
              DFBBoolean       clear)
{

     IDirectFBSurface *surface = box->surface;
     LiteScrollbar    *scrollbar = LITE_SCROLLBAR(box);
     DFBRectangle      dstRcAll, dstRcBtn1, dstRcBtn2, dstRcThumb;
     DFBRectangle      imgRcBack, imgRcBtn1, imgRcBtn2, imgRcThumb;
     int               thickness;

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_SCROLLBAR);

     surface->SetClip(surface, region);
     thickness = scrollbar->theme->all_images.width/8;

     // Fill the background
     if (clear)
          lite_clear_box(box, region);

     surface->SetDrawingFlags (surface, DSDRAW_NOFX);

     // calc destination rects
     get_scrollbar_rect(scrollbar, GSR_BTN1, &dstRcBtn1);
     get_scrollbar_rect(scrollbar, GSR_BTN2, &dstRcBtn2);

     dstRcAll.x = 0; dstRcAll.y = 0;
     dstRcAll.w = box->rect.w;
     dstRcAll.h = box->rect.h;
     
     // draw background
     imgRcBack.x = thickness*3;
     imgRcBack.y = thickness*2;
     imgRcBack.w = thickness;
     imgRcBack.h = thickness;
     if( !scrollbar->vertical )
          imgRcBack.x += (thickness*4);

     surface->StretchBlit(surface, scrollbar->theme->all_images.surface,
                              &imgRcBack, &dstRcAll);     
     
     // draw btn1 (left or top)
     imgRcBtn1.x = 0; imgRcBtn1.y = 0;
     imgRcBtn1.w = thickness;
     imgRcBtn1.h = thickness;
     if( !scrollbar->vertical )
          imgRcBtn1.x += (thickness*4);
     if( scrollbar->state == LSS_DISABLED || !can_scroll(&scrollbar->info) )
          imgRcBtn1.x += (thickness*3);
     else if( scrollbar->state == LSS_NORMAL )
          imgRcBtn1.x += 0;// do nothing
     else if( scrollbar->state == LSS_PRESSED_BTN1 )
          imgRcBtn1.x += (thickness*2);
     else
          imgRcBtn1.x += thickness;

     surface->StretchBlit(surface, scrollbar->theme->all_images.surface,
                              &imgRcBtn1, &dstRcBtn1);     

     // draw btn2 (right or bottom)
     imgRcBtn2.x = 0; imgRcBtn2.y = thickness;
     imgRcBtn2.w = thickness;
     imgRcBtn2.h = thickness;
     if( !scrollbar->vertical )
          imgRcBtn2.x += (thickness*4);
     if( scrollbar->state == LSS_DISABLED || !can_scroll(&scrollbar->info) )
          imgRcBtn2.x += (thickness*3);
     else if( scrollbar->state == LSS_NORMAL )
          imgRcBtn2.x += 0;// do nothing
     else if( scrollbar->state == LSS_PRESSED_BTN2 )
          imgRcBtn2.x += (thickness*2);
     else
          imgRcBtn2.x += thickness;

     surface->StretchBlit(surface, scrollbar->theme->all_images.surface,
                              &imgRcBtn2, &dstRcBtn2);     


     // draw thumb
     if( scrollbar->state != LSS_DISABLED && can_scroll(&scrollbar->info) )
     {
          get_scrollbar_rect(scrollbar, GSR_THUMB, &dstRcThumb);

          imgRcThumb.x = 0; imgRcThumb.y = thickness*2;
          imgRcThumb.w = imgRcThumb.h = thickness;
          if( !scrollbar->vertical )
               imgRcThumb.x += (thickness*4);
          if( scrollbar->state == LSS_NORMAL )
               imgRcThumb.x += 0; // do nothing
          else if( scrollbar->state == LSS_PRESSED_THUMB )
               imgRcThumb.x += (thickness*2);
          else
               imgRcThumb.x += thickness;
     
          draw_enlarged_image(surface, &dstRcThumb,
                              scrollbar->theme->all_images.surface,
                              &imgRcThumb, scrollbar->theme->image_margin);
     }

     return DFB_OK;
}


static int 
on_enter(LiteBox *box, int x, int y)
{
     LiteScrollbar  *scrollbar;

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_SCROLLBAR);

//   lite_focus_box(box);

     scrollbar = LITE_SCROLLBAR(box);
     if( scrollbar->state == LSS_NORMAL )
     {
          scrollbar->state = LSS_HILITE;
          lite_update_box(box, NULL);
     }

     return 1;
}

static int
on_leave(LiteBox *box, int x, int y)
{
     LiteScrollbar  *scrollbar;

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_SCROLLBAR);

     scrollbar = LITE_SCROLLBAR(box);
     if( scrollbar->state == LSS_HILITE )
     {
          scrollbar->state = LSS_NORMAL;
          lite_update_box(box, NULL);
     }

     return 1;
}

static int 
on_focus_in(LiteBox *box)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_SCROLLBAR);

     lite_update_box(box, NULL);

     return 1;
}

static int 
on_focus_out(LiteBox *box)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_SCROLLBAR);

     lite_update_box(box, NULL);

     return 1;
}

static int 
on_button_down(LiteBox                       *box, 
                 int                            x, 
                 int                            y,
                 DFBInputDeviceButtonIdentifier button)
{
     LiteScrollbar *scrollbar = LITE_SCROLLBAR(box);
     DFBRectangle rcThumb;
     ScrollbarHittestArea hittest;

     D_ASSERT( box != NULL );
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_SCROLLBAR);

     // hittest
     hittest = scrollbar_hittest(scrollbar, x,y);

     switch( hittest )
     {
     case SHA_BTN1:
          scrollbar->state = LSS_PRESSED_BTN1;
          break;
     
     case SHA_BTN2:
          scrollbar->state = LSS_PRESSED_BTN2;
          break;

     case SHA_THUMB:
          scrollbar->state = LSS_PRESSED_THUMB;
          get_scrollbar_rect(scrollbar, GSR_THUMB, &rcThumb);
          if( scrollbar->vertical )
               scrollbar->thumb_press_offset = y - rcThumb.y;
          else
               scrollbar->thumb_press_offset = x - rcThumb.x;          
          break;

     case SHA_BETWEEN_THUMB_BTN1:
          on_scroll(scrollbar, SCROLL_PAGE, -1, false);
          break;

     case SHA_BETWEEN_THUMB_BTN2:
          on_scroll(scrollbar, SCROLL_PAGE, 1, false);
          break;

     default:
          break;
     }


     lite_update_box(box, NULL);

     return 1;
}



static int 
on_button_up(LiteBox                       *box, 
             int                            x, 
             int                            y,
             DFBInputDeviceButtonIdentifier button)
{
     LiteScrollbar *scrollbar = LITE_SCROLLBAR(box);
     ScrollbarHittestArea hittest;

     D_ASSERT( box != NULL );
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_SCROLLBAR);

     hittest = scrollbar_hittest(scrollbar, x, y);

     switch( scrollbar->state )
     {
     case LSS_PRESSED_BTN1:
          if( hittest == SHA_BTN1 )
               on_scroll(scrollbar, SCROLL_LINE, -1, false);
          break;

     case LSS_PRESSED_BTN2:
          if( hittest == SHA_BTN2 )
               on_scroll(scrollbar, SCROLL_LINE, 1, false);
          break;

     case LSS_PRESSED_THUMB:
          // dragging ended
          if( scrollbar->info.track_pos != -1 &&
              scrollbar->info.track_pos != scrollbar->info.pos )
          {
               on_scroll(scrollbar, SCROLL_ABSOLUTE_POS,
                         scrollbar->info.track_pos, false);
          }
          break;

     default:
          break;
     }

     if( hittest == SHA_OUTSIDE )
          scrollbar->state = LSS_NORMAL;
     else
          scrollbar->state = LSS_HILITE;

     lite_update_box(box, NULL);

     return 1;
}


static int 
on_motion(LiteBox                 *box, 
          int                      x, 
          int                      y,
          DFBInputDeviceButtonMask buttons)
{
     LiteScrollbar* scrollbar = LITE_SCROLLBAR(box);
     DFBRectangle rcThumb;
     int thickness;
     int thumbDomainHeight, thumbHeight, actualMax;
     int value;
     int newPos;

     D_ASSERT( box != NULL );
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_SCROLLBAR);


     thickness = scrollbar->theme->all_images.width/8;

     if(buttons)
     {
          if( scrollbar->state == LSS_PRESSED_THUMB )
          {
               actualMax = scrollbar->info.max;
               if( scrollbar->info.page_size > 0 )
                    actualMax = scrollbar->info.max - scrollbar->info.page_size;

               // to calc thumb bar height
               get_scrollbar_rect(scrollbar, GSR_THUMB, &rcThumb);

               if( scrollbar->vertical )
               {
                    thumbHeight = rcThumb.h;
                    thumbDomainHeight = box->rect.h - 2*thickness;
                    value = y - thickness - scrollbar->thumb_press_offset;
               }
               else
               {
                    thumbHeight = rcThumb.w;
                    thumbDomainHeight = box->rect.w - 2*thickness;
                    value = x - thickness - scrollbar->thumb_press_offset;
               }

               if( value < 0 ) value = 0;
               if( value > (thumbDomainHeight - thumbHeight) )
                    value = thumbDomainHeight - thumbHeight;

               newPos = scrollbar->info.min
                    + ( actualMax - scrollbar->info.min )
                    * ( value )
                    / ( thumbDomainHeight - thumbHeight);
               
               if( newPos != scrollbar->info.track_pos )
               {
                    scrollbar->info.track_pos = newPos;
                    on_scroll(scrollbar, SCROLL_ABSOLUTE_POS, newPos, true);
                    lite_update_box( box, NULL );
               }
          }
     }

     return 1;
}


static void
draw_enlarged_image(IDirectFBSurface *target_surface,
                    DFBRectangle     *target_rect,
                    IDirectFBSurface *src_image_surface,
                    DFBRectangle     *src_image_rect,
                    int               image_margin)
{

/***********************************************************************
 Region rectangles

    +---+------------------------------------------------+---+
    | 0 |                       1                        | 2 |
    +---+------------------------------------------------+---+
    |   |                                                |   |
    |   |                                                |   |
    | 3 |                       4                        | 5 |
    |   |                                                |   |
    +---+------------------------------------------------+---+
    | 6 |                       7                        | 8 |
    +---+------------------------------------------------+---+

***********************************************************************/

     DFBRectangle rect_dst[9], rect_img[9];
     int i;

     /* left column */
     rect_dst[0].x = rect_dst[3].x = rect_dst[6].x = 0;
     rect_dst[0].w = rect_dst[3].w = rect_dst[6].w = image_margin;
     rect_img[0].x = rect_img[3].x = rect_img[6].x = 0;
     rect_img[0].w = rect_img[3].w = rect_img[6].w = image_margin;

     /* center column */
     rect_dst[1].x = rect_dst[4].x = rect_dst[7].x = image_margin;
     rect_dst[1].w = rect_dst[4].w = rect_dst[7].w = target_rect->w - 2*image_margin;
     rect_img[1].x = rect_img[4].x = rect_img[7].x = image_margin;
     rect_img[1].w = rect_img[4].w = rect_img[7].w = src_image_rect->w - 2*image_margin;

     /* right column */
     rect_dst[2].x = rect_dst[5].x = rect_dst[8].x = target_rect->w - image_margin;
     rect_dst[2].w = rect_dst[5].w = rect_dst[8].w = image_margin;
     rect_img[2].x = rect_img[5].x = rect_img[8].x = src_image_rect->w - image_margin;
     rect_img[2].w = rect_img[5].w = rect_img[8].w = image_margin;


     /* top row */
     rect_dst[0].y = rect_dst[1].y = rect_dst[2].y = 0;
     rect_dst[0].h = rect_dst[1].h = rect_dst[2].h = image_margin;
     rect_img[0].y = rect_img[1].y = rect_img[2].y = 0;
     rect_img[0].h = rect_img[1].h = rect_img[2].h = image_margin;

     /* center row */
     rect_dst[3].y = rect_dst[4].y = rect_dst[5].y = image_margin;
     rect_dst[3].h = rect_dst[4].h = rect_dst[5].h = target_rect->h - 2*image_margin;
     rect_img[3].y = rect_img[4].y = rect_img[5].y = image_margin;
     rect_img[3].h = rect_img[4].h = rect_img[5].h = src_image_rect->h - 2*image_margin;

     /* bottom row */
     rect_dst[6].y = rect_dst[7].y = rect_dst[8].y = target_rect->h - image_margin;
     rect_dst[6].h = rect_dst[7].h = rect_dst[8].h = image_margin;
     rect_img[6].y = rect_img[7].y = rect_img[8].y = src_image_rect->h - image_margin;
     rect_img[6].h = rect_img[7].h = rect_img[8].h = image_margin;


     for(i=0; i<sizeof(rect_dst)/sizeof(DFBRectangle); ++i)
     {
          rect_dst[i].x += target_rect->x;
          rect_dst[i].y += target_rect->y;
          rect_img[i].x += src_image_rect->x;
          rect_img[i].y += src_image_rect->y;
     }
     
     for(i=0; i<sizeof(rect_dst)/sizeof(DFBRectangle); ++i)
          target_surface->StretchBlit(target_surface,
                                    src_image_surface, &rect_img[i], &rect_dst[i]);

}

// Calculate rect of a part of scrollbar.
// Don't query thumb rect when ( disabled || !can_scroll() )
static void
get_scrollbar_rect( LiteScrollbar *scrollbar,
                    int            area,
                    DFBRectangle  *rc)
{
     LiteBox   *box = LITE_BOX(scrollbar);
     int        thickness;
     int        thumbHeight, thumbDomainHeight, min, actualMax, pos;


     if( !scrollbar || !rc )
          return;

     thickness = scrollbar->theme->all_images.width/8;

     switch( area )
     {
     case GSR_BTN1:
          rc->x = 0;
          rc->y = 0;
          if( scrollbar->vertical )
          {
               rc->w = box->rect.w;
               rc->h = thickness;
          }
          else
          {
               rc->w = thickness;
               rc->h = box->rect.h;
          }
          break;

     case GSR_BTN2:
          if( scrollbar->vertical )
          {
               rc->x = 0;
               rc->y = box->rect.h - thickness;
               rc->w = box->rect.w;
               rc->h = thickness;
          }
          else
          {
               rc->x = box->rect.w - thickness;
               rc->y = 0;
               rc->w = thickness;
               rc->h = box->rect.h;
          }
          break;

     case GSR_THUMB:
          if( scrollbar->info.track_pos == -1 )
               pos = scrollbar->info.pos;
          else
               pos = scrollbar->info.track_pos; // now dragging
          
          min = scrollbar->info.min;

          if( scrollbar->vertical )
          {
               rc->x = 0;
               rc->w = box->rect.w;
               thumbDomainHeight = box->rect.h - 2*thickness;
               if( thumbDomainHeight < 0 )
               {
                    rc->y = box->rect.h/2;
                    rc->h = 0;
                    break;
               }
               
               if( thumbDomainHeight <= thickness )
               {
                    rc->y = thickness;
                    rc->h = thumbDomainHeight;
                    break;
               }

               thumbHeight = thickness;
               actualMax = scrollbar->info.max;
               if( scrollbar->info.page_size > 0 )
               {
                    thumbHeight = thumbDomainHeight
                         * scrollbar->info.page_size
                         / (scrollbar->info.max - min );
                    actualMax = scrollbar->info.max - scrollbar->info.page_size;
               }
               if( thumbHeight < thickness )
                    thumbHeight = thickness;
          
               rc->h = thumbHeight;

               if( actualMax == min )
                    rc->y = thickness;
               else
                    rc->y = ( thumbDomainHeight - thumbHeight )
                         * ( pos - min ) / ( actualMax - min ) + thickness;
          }
          else
          {
               rc->y = 0;
               rc->h = box->rect.h;
               thumbDomainHeight = box->rect.w - 2*thickness;
               if( thumbDomainHeight < 0 )
               {
                    rc->x = box->rect.w/2;
                    rc->w = 0;
                    break;
               }
               
               if( thumbDomainHeight <= thickness )
               {
                    rc->x = thickness;
                    rc->w = thumbDomainHeight;
                    break;
               }

               thumbHeight = thickness;
               actualMax = scrollbar->info.max;
               if( scrollbar->info.page_size > 0 )
               {
                    thumbHeight = thumbDomainHeight
                         * scrollbar->info.page_size
                         / (scrollbar->info.max - min );
                    actualMax = scrollbar->info.max - scrollbar->info.page_size;
               }
               if( thumbHeight < thickness )
                    thumbHeight = thickness;
          
               rc->w = thumbHeight;

               if( actualMax == min )
                    rc->x = thickness;
               else
                    rc->x = ( thumbDomainHeight - thumbHeight )
                         * ( pos - min ) / (actualMax - min ) + thickness;
          }

          break;
     }

}


static int
pt_in_rect( int x, int y, DFBRectangle *rc)
{
     // return 1 if (x,y) is in the rc rectangle
     if( !rc ) return 0;
     if( x < rc->x ) return 0;
     if( y < rc->y ) return 0;
     if( x > rc->x+rc->w ) return 0;
     if( y > rc->y+rc->h ) return 0;
     return 1;
}


static ScrollbarHittestArea
scrollbar_hittest( LiteScrollbar *scrollbar,
                   int            x,
                   int            y)
{
     DFBRectangle rcBox, rc, rcThumb;
     rcBox.x=rcBox.y=0;
     rcBox.w=scrollbar->box.rect.w;
     rcBox.h=scrollbar->box.rect.h;
     rc.x=rc.y=rcThumb.x=rcThumb.y = -1;
     rc.w=rc.h=rcThumb.w=rcThumb.h = 0;

     if( !pt_in_rect( x,y, &rcBox ))
          return SHA_OUTSIDE;

     get_scrollbar_rect(scrollbar, GSR_BTN1, &rc);
     if( pt_in_rect( x,y, &rc) )
          return SHA_BTN1;

     get_scrollbar_rect(scrollbar, GSR_BTN2, &rc);
     if( pt_in_rect( x,y, &rc) )
          return SHA_BTN2;

     get_scrollbar_rect(scrollbar, GSR_THUMB, &rcThumb);
     if( pt_in_rect( x,y, &rcThumb) )
          return SHA_THUMB;

     if( scrollbar->vertical )
     {
          if( y < rcThumb.y )
               return SHA_BETWEEN_THUMB_BTN1;
          else
               return SHA_BETWEEN_THUMB_BTN2;
     }
     else
     {
          if( x < rcThumb.x )
               return SHA_BETWEEN_THUMB_BTN1;
          else
               return SHA_BETWEEN_THUMB_BTN2;
     }
}

static void
on_scroll( LiteScrollbar *scrollbar,
           ScrollMode    mode,
           int           param,
           int           dragging )
{
     int pos,newPos,actualMax;
     LiteScrollInfo info_copy;
     
     pos = scrollbar->info.pos;
     newPos = pos;

     switch( mode )
     {
     case SCROLL_PAGE:
          if( scrollbar->info.page_size > 0 )
          {
               if( param > 0) newPos+=scrollbar->info.page_size;
               else if( param < 0 ) newPos-=scrollbar->info.page_size;
               break;
          }
          // fall through SCROLL_LINE

     case SCROLL_LINE:
          if( param > 0) newPos += scrollbar->info.line_size;
          else if( param < 0) newPos-=scrollbar->info.line_size;
          break;
          
     case SCROLL_ABSOLUTE_POS:
          newPos = param;
          break;

     default:
          break;
     }

     actualMax = (int)scrollbar->info.max;
     if( scrollbar->info.page_size > 0 )
          actualMax -= scrollbar->info.page_size;
     if( actualMax < scrollbar->info.min )
          actualMax = scrollbar->info.min;

     if( newPos < (int)scrollbar->info.min )
          newPos = (int)scrollbar->info.min;
     if( newPos > actualMax )
          newPos = actualMax;
     
     if( pos == newPos )
          return;
     
     if( dragging )
          scrollbar->info.track_pos = newPos;
     else
     {
          scrollbar->info.pos = newPos;
          scrollbar->info.track_pos = -1;
     }

     if( scrollbar->update )
     {
          info_copy = scrollbar->info;
          scrollbar->update(scrollbar, &info_copy, scrollbar->update_data);
     }
}

int can_scroll(LiteScrollInfo *info)
{
     if( info->max <= info->min
           || info->page_size >= (info->max-info->min) )
          return false;
     else
          return true;
}
