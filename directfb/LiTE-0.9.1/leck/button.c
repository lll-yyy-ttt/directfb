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

#include <direct/mem.h>

#include <lite/util.h>

#include "button.h"

D_DEBUG_DOMAIN(LiteButtonDomain, "LiTE/Button", "LiteButton");

LiteButtonTheme *liteDefaultButtonTheme = NULL;

struct _LiteButton {
     LiteBox           box;
     LiteButtonTheme  *theme;

     int               enabled;
     LiteButtonType    type;
     LiteButtonState   state;
     IDirectFBSurface *imgsurface[6];

     ButtonPressFunc   press;
     void             *press_data;
};

static int on_enter(LiteBox *box, int x, int y);
static int on_leave(LiteBox *box, int x, int y);
static int on_button_down(LiteBox *box, int x, int y,
                          DFBInputDeviceButtonIdentifier button);
static int on_button_up(LiteBox *box, int x, int y,
                        DFBInputDeviceButtonIdentifier button);

static DFBResult draw_button(LiteBox *box, const DFBRegion *region, 
                              DFBBoolean clear);
static DFBResult destroy_button(LiteBox *box);


DFBResult 
lite_new_button(LiteBox         *parent, 
                DFBRectangle    *rect,
                LiteButtonTheme *theme,
                LiteButton     **ret_button)
{
     LiteButton *button = NULL;
     DFBResult res; 

     button = D_CALLOC(1, sizeof(LiteButton));

     button->box.parent = parent;
     button->theme = theme; 
     button->box.rect = *rect;

     res = lite_init_box(LITE_BOX(button));
     if (res != DFB_OK) {
          D_FREE(button);
          return res;
     }

     button->box.type           = LITE_TYPE_BUTTON;
     button->box.Draw           = draw_button;
     button->box.Destroy        = destroy_button;

     button->box.OnEnter        = on_enter;
     button->box.OnLeave        = on_leave;
     button->box.OnButtonDown   = on_button_down;
     button->box.OnButtonUp     = on_button_up;

     button->enabled            = 1;
     button->type               = LITE_BT_PUSH;
     button->state              = LITE_BS_NORMAL;
     
     *ret_button = button;

     D_DEBUG_AT(LiteButtonDomain, "Created new button object: %p\n", button);

     return DFB_OK;
}

DFBResult
lite_set_button_type(LiteButton *button, LiteButtonType type)
{
     LITE_NULL_PARAMETER_CHECK(button);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(button), LITE_TYPE_BUTTON);

     button->type = type;
     return DFB_OK;
}

DFBResult 
lite_enable_button(LiteButton *button, int enabled)
{
     LITE_NULL_PARAMETER_CHECK(button);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(button), LITE_TYPE_BUTTON);

     if ((button->enabled && !enabled) ||
         (!button->enabled && enabled)) {
          button->enabled = enabled;

          lite_update_box(LITE_BOX(button), NULL);
     }

     return DFB_OK;
}

DFBResult 
lite_set_button_state(LiteButton *button, LiteButtonState state)
{
     LITE_NULL_PARAMETER_CHECK(button);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(button), LITE_TYPE_BUTTON);

     /* return if unknown state */
     if (state >= LITE_BS_MAX)
          return DFB_INVARG;

     /* no need to update et rest if it's the same state, just return */
     if (button->state == state)
          return DFB_OK;

     button->state = state;

     /* need to update the box in case the state changed */
     if (button->enabled)
          lite_update_box(LITE_BOX(button), NULL);

     return DFB_OK;
}

DFBResult
lite_get_button_state(LiteButton *button, LiteButtonState *state)
{
     LITE_NULL_PARAMETER_CHECK(button);
     LITE_NULL_PARAMETER_CHECK(state);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(button), LITE_TYPE_BUTTON);

     *state = button->state;
     return DFB_OK;
}

DFBResult 
lite_set_button_image(LiteButton *button, 
                      LiteButtonState state, 
                      const char *filename)
{
     DFBResult ret;
     LITE_NULL_PARAMETER_CHECK(button);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(button), LITE_TYPE_BUTTON);
     LITE_NULL_PARAMETER_CHECK(filename);

     D_DEBUG_AT(LiteButtonDomain, "Setting button image for state: %d from file %s\n",
                    state, filename);

     /* check for valid state */
     if (state >= LITE_BS_MAX)
          return DFB_INVARG;

     /* load the image surface from the absolute file path */
     ret = lite_util_load_image(filename, DSPF_UNKNOWN,
                                &button->imgsurface[state], NULL, NULL, NULL);

     /* if known state and not disabled, update the button immediately */
     if (state == button->state || (state == LITE_BS_DISABLED && !button->enabled)) {
          lite_update_box(LITE_BOX(button), NULL);
     }

     return ret;
}

DFBResult 
lite_set_button_image_desc(LiteButton            *button, 
                           LiteButtonState        state, 
                           DFBSurfaceDescription *dsc)
{
     DFBResult ret;
     LITE_NULL_PARAMETER_CHECK(button);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(button), LITE_TYPE_BUTTON);
     LITE_NULL_PARAMETER_CHECK(dsc);

     D_DEBUG_AT(LiteButtonDomain, "Setting buttom image for state: %d from description: %p\n",
                    state, dsc);

     /* check for valid button state */
     if (state >= LITE_BS_MAX)
          return DFB_INVARG;

     /* load the image description directly */
     ret = lite_util_load_image_desc(&button->imgsurface[state], dsc);

     /* if known state and not active, immediately update the button */
     if (state == button->state || (state == LITE_BS_DISABLED && !button->enabled)) {
          lite_update_box(LITE_BOX(button), NULL);
     }

     return ret;
}

DFBResult 
lite_set_button_image_surface(LiteButton        *button, 
                              LiteButtonState    state,
                              IDirectFBSurface  *surface)
{
     LITE_NULL_PARAMETER_CHECK(button);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(button), LITE_TYPE_BUTTON);
     
     /* check for valid button state */
     if (state >= LITE_BS_MAX)
          return DFB_INVARG;

     button->imgsurface[state] = surface;

     return DFB_OK;
}

int 
lite_on_button_press(LiteButton     *button, 
                     ButtonPressFunc func, 
                     void           *funcdata)
{
     LITE_NULL_PARAMETER_CHECK(button);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(button), LITE_TYPE_BUTTON);

     button->press      = func;
     button->press_data = funcdata;

     return 0;
}


/* internals */

static DFBResult 
destroy_button(LiteBox *box)
{
     int i;
     LiteButton *button = NULL;

     D_ASSERT(box != NULL);

     button = LITE_BUTTON(box);

     D_DEBUG_AT(LiteButtonDomain, "Destroy button: %p\n", button);

     if (!button)
          return DFB_FAILURE;

     for (i = 0; i < LITE_BS_MAX; i++)
         if (button->imgsurface[i]) {
             button->imgsurface[i]->Release(button->imgsurface[i]);
             button->imgsurface[i] = NULL;
         }

     return lite_destroy_box(box);
}

static int 
on_enter(LiteBox *box, int x, int y)
{
     LiteButton *button = LITE_BUTTON(box);

     D_ASSERT(box != NULL);

     if (button->type == LITE_BT_TOGGLE && 
         (button->state == LITE_BS_PRESSED || 
	  button->state == LITE_BS_DISABLED_ON))
         lite_set_button_state(button, LITE_BS_HILITE_ON);
     else
         lite_set_button_state(button, LITE_BS_HILITE);

     return 1;
}

static int 
on_leave(LiteBox *box, int x, int y)
{
     LiteButton *button = LITE_BUTTON(box);

     D_ASSERT(box != NULL);

     if (button->type == LITE_BT_TOGGLE && 
         (button->state == LITE_BS_HILITE_ON ||
	  button->state == LITE_BS_PRESSED))
         lite_set_button_state(button, LITE_BS_PRESSED);
     else
         lite_set_button_state(button, LITE_BS_NORMAL);

     return 1;
}

static int 
on_button_down(LiteBox                       *box, 
               int                            x, 
               int                            y,
               DFBInputDeviceButtonIdentifier button)
{
     LiteButton *buttonbox = LITE_BUTTON(box);
     
     D_ASSERT(box != NULL);

     switch (buttonbox->type) {
     case LITE_BT_PUSH:
          lite_set_button_state(buttonbox, LITE_BS_PRESSED);
          break;
     case LITE_BT_TOGGLE:
          if (buttonbox->state == LITE_BS_PRESSED ||
	      buttonbox->state == LITE_BS_HILITE_ON)
               lite_set_button_state(buttonbox, LITE_BS_HILITE);
          else
               lite_set_button_state(buttonbox, LITE_BS_PRESSED);
	  break;
     }

     return 1;
}

static int 
on_button_up(LiteBox                       *box, 
             int                            x, 
             int                            y,
             DFBInputDeviceButtonIdentifier button)
{
     LiteButton *buttonbox = LITE_BUTTON(box);

     D_ASSERT(box != NULL);

     /* do not send button selected messages unless mouse up occurs within
        the bounds of the widget */
     if (x >= 0 && x < box->rect.w && y >= 0 && y < box->rect.h) {

          switch (buttonbox->type) {
	  case LITE_BT_PUSH:
               lite_set_button_state(buttonbox, LITE_BS_HILITE);
               break;
	  case LITE_BT_TOGGLE:
               break;
          }

          if (buttonbox->enabled && buttonbox->press)
               buttonbox->press(buttonbox, buttonbox->press_data);
     }

     return 1;
}

static DFBResult 
draw_button(LiteBox         *box, 
            const DFBRegion *region, 
            DFBBoolean       clear)
{
     LiteButtonState state;
     LiteButton *button = LITE_BUTTON(box);

     D_ASSERT(box != NULL);

     if (clear)
          lite_clear_box(box, region);

     box->surface->SetClip(box->surface, NULL);
     box->surface->SetBlittingFlags(box->surface, DSBLIT_BLEND_ALPHACHANNEL);


     if (button->enabled)
          state = button->state;
     else
          state = LITE_BS_DISABLED;

     if (button->imgsurface[state])
          box->surface->Blit(box->surface, button->imgsurface[state], NULL, 0, 0);

     return DFB_OK;
}

