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

#include <lite/lite.h>
#include <lite/util.h>

#include "check.h"

#define MARKER_CAPTION_GAP   (6)

D_DEBUG_DOMAIN(LiteCheckDomain, "LiTE/Check", "LiteCheck");

LiteCheckTheme *liteDefaultCheckTheme = NULL;

struct _LiteCheck {
     LiteBox         box;
     LiteCheckTheme *theme;

     const char     *caption_text;
     int             hilite;
     int             enabled;
     LiteCheckState  state;

     CheckPressFunc  press;
     void           *press_data;
};

static int on_enter(LiteBox *box, int x, int y);
static int on_leave(LiteBox *box, int x, int y);
static int on_button_down(LiteBox *box, int x, int y,
                          DFBInputDeviceButtonIdentifier button);
static int on_button_up(LiteBox *box, int x, int y,
                        DFBInputDeviceButtonIdentifier button);
static void draw_check_caption(LiteBox *box,
               DFBRectangle      *rc,
               const char        *text,
               LiteFont          *font,
               DFBColor          *font_color);
static DFBResult draw_check(LiteBox *box, const DFBRegion *region, 
                              DFBBoolean clear);
static DFBResult destroy_check(LiteBox *box);

DFBResult 
lite_new_check(LiteBox   *parent, 
          DFBRectangle   *rect,
          const char     *caption_text,
          LiteCheckTheme *theme,
          LiteCheck     **ret_check)
{
     LiteCheck *check = NULL;
     DFBResult res;

     LITE_NULL_PARAMETER_CHECK(parent);
     LITE_NULL_PARAMETER_CHECK(rect);
     LITE_NULL_PARAMETER_CHECK(theme);
     LITE_NULL_PARAMETER_CHECK(ret_check);

     if( caption_text == NULL )
          caption_text = ""; 

     check = D_CALLOC(1, sizeof(LiteCheck));

     check->box.parent = parent;
     check->caption_text = D_STRDUP(caption_text);
     check->theme = theme; 
     check->box.rect = *rect;

     res = lite_init_box(LITE_BOX(check));
     if (res != DFB_OK) {
          D_FREE(check);
          return res;
     }

     check->box.type           = LITE_TYPE_CHECK;
     check->box.Draw           = draw_check;
     check->box.Destroy        = destroy_check;

     check->box.OnEnter        = on_enter;
     check->box.OnLeave        = on_leave;
     check->box.OnButtonDown   = on_button_down;
     check->box.OnButtonUp     = on_button_up;

     check->enabled            = 1;
     check->hilite             = 0;
     check->state              = LITE_CHS_UNCHECKED;
     
     *ret_check = check;

     D_DEBUG_AT(LiteCheckDomain, "Created new check object: %p\n", check);

     return DFB_OK;
}

DFBResult
lite_set_check_caption(LiteCheck *check,
                      const char *caption_text)
{
     LITE_NULL_PARAMETER_CHECK(check);
     if( NULL == caption_text )
          caption_text = "";

     if( strcmp( check->caption_text, caption_text ) )
     {
          check->caption_text = D_STRDUP(caption_text);
          lite_update_box(LITE_BOX(check), NULL); 
     }
     return DFB_OK;
}

DFBResult 
lite_enable_check(LiteCheck *check, int enabled)
{
     LITE_NULL_PARAMETER_CHECK(check);

     if ((check->enabled && !enabled) ||
         (!check->enabled && enabled)) {
          check->enabled = enabled;

          lite_update_box(LITE_BOX(check), NULL);
     }

     return DFB_OK;
}

DFBResult 
lite_check_check(LiteCheck *check, LiteCheckState state)
{
     LITE_NULL_PARAMETER_CHECK(check);

     /* return if unknown state */
     switch (state) {
          case LITE_CHS_UNCHECKED:
          case LITE_CHS_CHECKED:
               break;
          default:
               return DFB_INVARG;
     }

     /* no need to update et rest if it's the same state, just return */
     if (check->state == state)
          return DFB_OK;

     check->state = state;

     /* need to update the box in case the state changed */
     lite_update_box(LITE_BOX(check), NULL);

     return DFB_OK;
}

DFBResult
lite_get_check_state(LiteCheck      *check,
                     LiteCheckState *ret_state)
{
     LITE_NULL_PARAMETER_CHECK(check);
     LITE_NULL_PARAMETER_CHECK(ret_state);

     *ret_state = check->state;
     return DFB_OK;
}



DFBResult 
lite_on_check_press(LiteCheck     *check, 
                    CheckPressFunc func, 
                    void          *funcdata)
{
     LITE_NULL_PARAMETER_CHECK(check);

     check->press      = func;
     check->press_data = funcdata;

     return DFB_OK;
}


DFBResult lite_new_check_theme(
                    const char      *image_path,
                    LiteCheckTheme **ret_theme)
{
     LiteCheckTheme* theme = NULL;
     DFBResult res;

     LITE_NULL_PARAMETER_CHECK(image_path);
     LITE_NULL_PARAMETER_CHECK(ret_theme);

     theme = D_CALLOC(1, sizeof(LiteCheckTheme));
     memset(theme, 0, sizeof( LiteCheckTheme ));

     res = lite_util_load_image(
          image_path,
          DSPF_UNKNOWN,
          &theme->all_images.surface,
          &theme->all_images.width,
          &theme->all_images.height,
          NULL);
     if( res != DFB_OK )
          goto error2;

     res = lite_get_font("default", LITE_FONT_PLAIN, 13, DEFAULT_FONT_ATTRIBUTE,
          &theme->font);
     if( res != DFB_OK )
          goto error1;

     theme->font_color.a = 255;
     theme->font_color.r = 0;
     theme->font_color.g = 0;
     theme->font_color.b = 0;

     *ret_theme = theme;

     return DFB_OK;

error1:
     lite_release_font(theme->font);
error2:
     theme->all_images.surface->Release(theme->all_images.surface);
     D_FREE(theme);

     return DFB_FAILURE;
}

DFBResult lite_destroy_check_theme(LiteCheckTheme *theme)
{
     LITE_NULL_PARAMETER_CHECK(theme);

     if( theme->all_images.surface )
          theme->all_images.surface->Release(
               theme->all_images.surface);

     lite_release_font(theme->font);

     D_FREE(theme);

     return DFB_OK;
}

/* internals */

static DFBResult
destroy_check(LiteBox *box)
{
     LiteCheck *check = NULL;

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_CHECK);

     check = LITE_CHECK(box);

     D_DEBUG_AT(LiteCheckDomain, "Destroy check: %p\n", check);

     if (!check)
          return DFB_FAILURE;
    return lite_destroy_box(box);
}

static int 
on_enter(LiteBox *box, int x, int y)
{
     LiteCheck *check = LITE_CHECK(box);

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_CHECK);

     if( ! check->hilite )
     {
          check->hilite = 1;
          lite_update_box( box, NULL );
     }

     return 1;
}

static int 
on_leave(LiteBox *box, int x, int y)
{
     LiteCheck *check = LITE_CHECK(box);

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(check), LITE_TYPE_CHECK);

     if( check->hilite )
     {
          check->hilite = 0;
          lite_update_box( box, NULL );
     }

     return 1;
}

static int 
on_button_down(LiteBox                       *box, 
               int                            x, 
               int                            y,
               DFBInputDeviceButtonIdentifier button)
{
     LiteCheck *check = LITE_CHECK(box);
     
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(check), LITE_TYPE_CHECK);

     /* Do nothing */


     return 1;

}

static int 
on_button_up(LiteBox                       *box, 
             int                            x, 
             int                            y,
             DFBInputDeviceButtonIdentifier button)
{
     LiteCheck *check = LITE_CHECK(box);

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(check), LITE_TYPE_CHECK);

     /* do not send button selected messages unless mouse up occurs within
        the bounds of the widget */
     if (x < box->rect.w && y < box->rect.h) {
//          lite_set_text_button_state(buttonbox, LITE_TBS_HILITE);
 
          if ( check->enabled )
          {

               if( check->state == LITE_CHS_UNCHECKED )
                    check->state = LITE_CHS_CHECKED;
               else
                    check->state = LITE_CHS_UNCHECKED;

               lite_update_box( box, NULL );

               if( check->press )
                    check->press( check, check->state, check->press_data);
          }
     }

     return 1;
}

static DFBResult 
draw_check(LiteBox         *box, 
           const DFBRegion *region, 
           DFBBoolean       clear)
{
     int image_index = 0;
     LiteCheck *check = LITE_CHECK(box);
     IDirectFBSurface* surface = NULL;
     DFBRectangle check_rectangle, rc, rc_image;	

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(check), LITE_TYPE_CHECK);

     if( check->state == LITE_CHS_CHECKED )
          image_index += 3;

     if( !check->enabled )
          image_index += 2;
     else if( check->hilite )
          image_index += 1;

     D_DEBUG_AT(LiteCheckDomain, 
          "draw_check()  enabled:%d, hilite:%d, state:%d. image_index:%d, clear:%d\n",
          check->enabled, check->hilite, check->state, image_index, clear);

     if (clear)
          lite_clear_box(box, region);

     box->surface->SetClip(box->surface, NULL);
     box->surface->SetBlittingFlags(box->surface, DSBLIT_BLEND_ALPHACHANNEL);

     check_rectangle = box->rect;
     check_rectangle.x = check_rectangle.y = 0;

     // draw check marker area
     surface = check->theme->all_images.surface;
     rc_image.x = image_index * ( check->theme->all_images.width ) / LITE_CHECK_IMAGE_COUNT;
     rc_image.y = 0;
     rc_image.w = check->theme->all_images.width / LITE_CHECK_IMAGE_COUNT;
     rc_image.h = check->theme->all_images.height;

     rc.x = 0;
     rc.y = 0;
     rc.w = rc_image.w;
     rc.h = rc_image.h;

     if( rc_image.h < box->rect.h )
     {
          rc.y = rc.y + (box->rect.h - rc_image.h)/2;
     } 

     if (surface)
          box->surface->Blit(box->surface,
                               surface, &rc_image, rc.x, rc.y);

     // draw caption area
     rc.x = rc_image.w + MARKER_CAPTION_GAP;
     rc.y = 0;
     rc.w = box->rect.w - rc.x;
     rc.h = box->rect.h;

     if ( check->caption_text )
          draw_check_caption(box,&rc,
                             check->caption_text,
                             check->theme->font,
                            &check->theme->font_color);

     return DFB_OK;
}

static void make_truncated_string(char* text, int width, IDirectFBFont* font_interface)
{
     DFBRectangle   ink_rect;
     char          *tail = "...";
     char          *buffer = NULL;
     int            i;

     font_interface->GetStringExtents(
          font_interface,text,-1, NULL, &ink_rect);
     if( ink_rect.w <= width )
          return;

     /* get tail's length */
     font_interface->GetStringExtents(
               font_interface, tail, -1, NULL, &ink_rect);
     if( ink_rect.w >= width )
     {
          strncpy(text, tail, strlen(text)); // prevent buffer overflow
          return;
     }

     buffer = malloc(strlen(text)+4);
     memset(buffer,0,strlen(text)+4);
     strncpy(buffer, text, MAX(0,strlen(text)-3) );
     strcat(buffer, tail);

     while( 1 )
     {
          font_interface->GetStringExtents(
               font_interface,buffer,-1, NULL, &ink_rect);
          if( ink_rect.w <= width )
               break;

          i = strlen(buffer);
          buffer[i-1] = 0;
          buffer[i-4] = '.';   
     }

     strcpy(text,buffer);

     free(buffer);
}

static void
draw_check_caption(LiteBox *box,
               DFBRectangle      *rect,
               const char        *text,
               LiteFont          *font,
               DFBColor          *font_color)
{
     IDirectFBFont *font_interface;
     char buffer[64];
     DFBResult result;
     int draw_text_pos_x, draw_text_pos_y;
     int font_height;

     result = lite_font(font, &font_interface);
     if( result != DFB_OK )
          return;     

     strncpy(buffer, text, sizeof(buffer));
     buffer[sizeof(buffer)-1] = 0;

     make_truncated_string(buffer, rect->w, font_interface);

     font_interface->GetHeight(font_interface, &font_height);

     draw_text_pos_x = rect->x;
     draw_text_pos_y = rect->y + (rect->h - font_height)/2;

     box->surface->SetFont(box->surface, font_interface);
     box->surface->SetColor(box->surface, font_color->r,
                            font_color->g, font_color->b,
                            font_color->a);

     box->surface->DrawString(box->surface, buffer, -1, draw_text_pos_x,
                              draw_text_pos_y, DSTF_LEFT|DSTF_TOP);     

}

