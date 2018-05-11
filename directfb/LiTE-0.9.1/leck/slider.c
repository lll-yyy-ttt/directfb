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

#include "slider.h"

D_DEBUG_DOMAIN(LiteSliderDomain, "LiTE/Slider", "LiteSlider");

LiteSliderTheme *liteDefaultSliderTheme = NULL;

struct _LiteSlider {
     LiteBox            box;
     LiteSliderTheme   *theme;
     float              pos;
     bool               vertical;
     SliderUpdateFunc   update;
     void              *update_data;
};

static int on_enter(LiteBox *box, int x, int y);
static int on_focus_in(LiteBox *box);
static int on_focus_out(LiteBox *box);

static int on_button_down(LiteBox *box, int x, int y,
                          DFBInputDeviceButtonIdentifier button);

static int on_motion(LiteBox *box, int x, int y,
                           DFBInputDeviceButtonMask buttons);

static DFBResult draw_slider(LiteBox *box, const DFBRegion *region, DFBBoolean clear);
static DFBResult destroy_slider(LiteBox *box);

DFBResult 
lite_new_slider(LiteBox         *parent, 
                DFBRectangle    *rect,
                LiteSliderTheme *theme,
                LiteSlider     **ret_slider)
{
     DFBResult   res; 
     LiteSlider *slider = NULL;

     slider = D_CALLOC(1, sizeof(LiteSlider));

     slider->box.parent = parent;
     slider->theme = theme;
     slider->box.rect = *rect;

     res = lite_init_box(LITE_BOX(slider));
     if (res != DFB_OK) {
          D_FREE(slider);
          return res;
     }

     slider->box.type    = LITE_TYPE_SLIDER;
     slider->box.Draw    = draw_slider;
     slider->box.Destroy = destroy_slider;

     slider->box.OnEnter      = on_enter;
     slider->box.OnFocusIn    = on_focus_in;
     slider->box.OnFocusOut   = on_focus_out;
     slider->box.OnButtonDown = on_button_down;
     slider->box.OnMotion     = on_motion;

     slider->vertical=(slider->box.rect.h > slider->box.rect.w) ? true : false;

     lite_update_box(LITE_BOX(slider), NULL);

     *ret_slider = slider;

     D_DEBUG_AT(LiteSliderDomain, "Created new slider object: %p\n", slider);

     return DFB_OK;
}

DFBResult 
lite_set_slider_pos(LiteSlider *slider, float pos)
{
     LITE_NULL_PARAMETER_CHECK(slider);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(slider), LITE_TYPE_SLIDER);

     if (pos < 0.0f)
          pos = 0.0f;
     else if (pos > 1.0f)
          pos = 1.0f;

     if (pos == slider->pos)
          return DFB_OK;

     slider->pos = pos;

     return lite_update_box(LITE_BOX(slider), NULL);
}

DFBResult 
lite_on_slider_update(LiteSlider       *slider,
                      SliderUpdateFunc  func,
                      void             *funcdata)
{
     LITE_NULL_PARAMETER_CHECK(slider);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(slider), LITE_TYPE_SLIDER);

     slider->update = func;
     slider->update_data = funcdata;

     return DFB_OK;
}


/* internals */

static DFBResult 
destroy_slider(LiteBox *box)
{
     LiteSlider *slider =  NULL;

     D_ASSERT(box != NULL);

     slider = LITE_SLIDER(box);

     D_DEBUG_AT(LiteSliderDomain, "Destroy slider: %p\n", slider);

     if (!slider)
          return DFB_FAILURE;

     return lite_destroy_box(box);
}

static DFBResult 
draw_slider(LiteBox           *box, 
              const DFBRegion *region, 
              DFBBoolean       clear)
{
     IDirectFBSurface *surface = box->surface;
     LiteSlider       *slider = LITE_SLIDER(box);
     int               h2 = box->rect.h / 2;
     int               w2 = box->rect.w / 2;
     int               pos = (int) (slider->pos * (box->rect.w-5)) + 2;
     int               posw = (int) (slider->pos * (box->rect.h));

     D_ASSERT(box != NULL);

     surface->SetClip(surface, region);

     /* Fill the background */
     if (clear)
          lite_clear_box(box, region);

     surface->SetDrawingFlags (surface, DSDRAW_NOFX);

     if(slider->vertical) {
          /* Draw vertical line */
          surface->SetColor (surface, 0xe0, 0xe0, 0xe0, 0xff);
          surface->DrawRectangle (surface, w2 - 3, 0, 8, box->rect.h);
          
          surface->SetColor (surface, 0xb0, 0xb0, 0xb0, 0xff);
          surface->DrawRectangle (surface, w2 - 2, 1, 6 , box->rect.h - 2);
          
          if (box->is_focused)
               surface->SetColor (surface, 0xc0, 0xc0, 0xff, 0xf0);
          else
               surface->SetColor (surface, 0xf0, 0xf0, 0xf0, 0xd0);
          surface->FillRectangle (surface, w2 - 1, 2, 4, box->rect.h - 4);
          
          /* Draw the position indicator */
          surface->SetDrawingFlags (surface, DSDRAW_BLEND);
          surface->SetColor (surface, 0x80, 0x80, 0xa0, 0xe0);
          surface->FillRectangle (surface, 0, posw-1, box->rect.w, 1);
          surface->FillRectangle (surface, 0, posw+1, box->rect.w,1);
          
          surface->SetColor (surface, 0xb0, 0xb0, 0xc0, 0xff);
          surface->FillRectangle (surface, 0, posw, box->rect.w, 1);
     }
     else {
          /* Draw horizontal line */
          surface->SetColor (surface, 0xe0, 0xe0, 0xe0, 0xff);
          surface->DrawRectangle (surface, 0, h2 - 3, box->rect.w, 8);
          
          surface->SetColor (surface, 0xb0, 0xb0, 0xb0, 0xff);
          surface->DrawRectangle (surface, 1, h2 - 2, box->rect.w - 2, 6);
          
          if (box->is_focused)
               surface->SetColor (surface, 0xc0, 0xc0, 0xff, 0xf0);
          else
               surface->SetColor (surface, 0xf0, 0xf0, 0xf0, 0xd0);
          surface->FillRectangle (surface, 2, h2 - 1, box->rect.w - 4, 4);
          
          /* Draw the position indicator */
          surface->SetDrawingFlags (surface, DSDRAW_BLEND);
          surface->SetColor (surface, 0x80, 0x80, 0xa0, 0xe0);
          surface->FillRectangle (surface, pos-1, 0, 1, box->rect.h);
          surface->FillRectangle (surface, pos+1, 0, 1, box->rect.h);
          
          surface->SetColor (surface, 0xb0, 0xb0, 0xc0, 0xff);
          surface->FillRectangle (surface, pos, 0, 1, box->rect.h);
     }

     return DFB_OK;
}

static int 
on_enter(LiteBox *box, int x, int y)
{
     D_ASSERT(box != NULL);

     lite_focus_box(box);

     return 1;
}

static int 
on_focus_in(LiteBox *box)
{
     D_ASSERT(box != NULL);

     lite_update_box(box, NULL);

     return 1;
}

static int 
on_focus_out(LiteBox *box)
{
     D_ASSERT(box != NULL);

     lite_update_box(box, NULL);

     return 1;
}

static int 
on_button_down(LiteBox                       *box, 
                 int                            x, 
                 int                            y,
                 DFBInputDeviceButtonIdentifier button)
{
     LiteSlider *slider = LITE_SLIDER(box);
     float       pos = (x - 2) / (float) (box->rect.w - 5);

     D_ASSERT(box != NULL);

     if (pos < 0.0f)
          pos = 0.0f;
     else if (pos > 1.0f)
          pos = 1.0f;

     if (pos == slider->pos)
          return 1;

     slider->pos = pos;

     if (slider->update)
          slider->update(slider, pos, slider->update_data);

     lite_update_box(box, NULL);

     return 1;
}

static int 
on_motion(LiteBox                 *box, 
          int                      x, 
          int                      y,
          DFBInputDeviceButtonMask buttons)
{
     D_ASSERT(box != NULL);

     if (buttons)
          return on_button_down(box, x, y, DIBI_LEFT);

     return 1;
}

