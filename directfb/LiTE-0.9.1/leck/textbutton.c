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

#include "textbutton.h"

D_DEBUG_DOMAIN(LiteTextButtonDomain, "LiTE/TextButton", "LiteTextButton");

#define IMG_MARGIN	(4)

LiteTextButtonTheme *liteDefaultTextButtonTheme = NULL;

struct _LiteTextButton {
     LiteBox              box;
     LiteTextButtonTheme *theme;

     const char          *caption_text;
     int                  enabled;
     LiteTextButtonState  state;

     TextButtonPressFunc  press;
     void                *press_data;
};

static int on_enter(LiteBox *box, int x, int y);
static int on_leave(LiteBox *box, int x, int y);
static int on_button_down(LiteBox *box, int x, int y,
                          DFBInputDeviceButtonIdentifier button);
static int on_button_up(LiteBox *box, int x, int y,
                        DFBInputDeviceButtonIdentifier button);
static void draw_text_button_bkgnd(LiteBox  *box,
               IDirectFBSurface *surface_image,
               DFBRectangle     *rect_image);
static void draw_text_button_caption(LiteBox *box,
               const char        *text,
               LiteFont          *font,
               DFBColor          *font_color);
static DFBResult draw_text_button(LiteBox *box, const DFBRegion *region, 
                              DFBBoolean clear);
static DFBResult destroy_text_button(LiteBox *box);

DFBResult 
lite_new_text_button(LiteBox        *parent, 
                DFBRectangle        *rect,
                const char          *caption_text,
                LiteTextButtonTheme *theme,
                LiteTextButton     **ret_button)
{
     LiteTextButton *button = NULL;
     DFBResult res;

     LITE_NULL_PARAMETER_CHECK(parent);
     LITE_NULL_PARAMETER_CHECK(rect);
     LITE_NULL_PARAMETER_CHECK(theme);
     LITE_NULL_PARAMETER_CHECK(ret_button);

     if( caption_text == NULL )
          caption_text = "";

     button = D_CALLOC(1, sizeof(LiteTextButton));

     button->box.parent = parent;
     button->caption_text = D_STRDUP(caption_text);
     button->theme = theme; 
     button->box.rect = *rect;

     res = lite_init_box(LITE_BOX(button));
     if (res != DFB_OK) {
          D_FREE(button);
          return res;
     }

     button->box.type           = LITE_TYPE_TEXT_BUTTON;
     button->box.Draw           = draw_text_button;
     button->box.Destroy        = destroy_text_button;

     button->box.OnEnter        = on_enter;
     button->box.OnLeave        = on_leave;
     button->box.OnButtonDown   = on_button_down;
     button->box.OnButtonUp     = on_button_up;

     button->enabled            = 1;
     button->state              = LITE_TBS_NORMAL;
     
     *ret_button = button;

     D_DEBUG_AT(LiteTextButtonDomain, "Created new text button object: %p\n", button);

     return DFB_OK;
}

DFBResult
lite_set_text_button_caption(LiteTextButton *button, const char* caption_text)
{
     LITE_NULL_PARAMETER_CHECK(button);
     if( !caption_text )
          caption_text = "";

     if( strcmp(button->caption_text, caption_text) )
     {
          button->caption_text = D_STRDUP(caption_text);
          lite_update_box(LITE_BOX(button), NULL);
     }
     return DFB_OK;
}

DFBResult 
lite_enable_text_button(LiteTextButton *button, int enabled)
{
     LITE_NULL_PARAMETER_CHECK(button);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(button), LITE_TYPE_TEXT_BUTTON);

     if ((button->enabled && !enabled) ||
         (!button->enabled && enabled)) {
          button->enabled = enabled;

          lite_update_box(LITE_BOX(button), NULL);
     }

     return DFB_OK;
}

DFBResult 
lite_set_text_button_state(LiteTextButton *button, LiteTextButtonState state)
{
     LITE_NULL_PARAMETER_CHECK(button);

     /* return if unknown state */
     switch (state) {
          case LITE_TBS_NORMAL:
          case LITE_TBS_PRESSED:
          case LITE_TBS_HILITE:
          case LITE_TBS_DISABLED:
               break;
          default:
               return DFB_INVARG;
     }

     /* no need to update et rest if it's the same state, just return */
     if (button->state == state)
          return DFB_OK;

     button->state = state;

     /* need to update the box in case the state changed */
     if (button->enabled) {
          lite_update_box(LITE_BOX(button), NULL);
     }

     return DFB_OK;
}

DFBResult 
lite_on_text_button_press(LiteTextButton     *button, 
                          TextButtonPressFunc func, 
                          void               *funcdata)
{
     LITE_NULL_PARAMETER_CHECK(button);

     button->press      = func;
     button->press_data = funcdata;

     return DFB_OK;
}


DFBResult lite_new_text_button_theme(
          const char *image_path,
          LiteTextButtonTheme **ret_theme)
{
     LiteTextButtonTheme* theme = NULL;
     DFBResult res;

     LITE_NULL_PARAMETER_CHECK(image_path);
     LITE_NULL_PARAMETER_CHECK(ret_theme);

     theme = D_CALLOC(1, sizeof(LiteTextButtonTheme));
     memset(theme, 0, sizeof( LiteTextButtonTheme ));

     res = lite_util_load_image(
          image_path,
          DSPF_UNKNOWN,
          &theme->all_images.surface,
          &theme->all_images.width,
          &theme->all_images.height,
          NULL);
     if( res != DFB_OK )
          goto error2;

     // default font
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

DFBResult lite_destroy_text_button_theme(LiteTextButtonTheme* theme)
{
     LITE_NULL_PARAMETER_CHECK(theme);

     theme->all_images.surface->Release(
          theme->all_images.surface);

     lite_release_font(theme->font);

     D_FREE(theme);

     return DFB_OK;
}

/* internals */

static DFBResult
destroy_text_button(LiteBox *box)
{
     LiteTextButton *button = NULL;

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_TEXT_BUTTON);

     button = LITE_TEXTBUTTON(box);

     D_DEBUG_AT(LiteTextButtonDomain, "Destroy text button: %p\n", button);

     if (!button)
          return DFB_FAILURE;
    return lite_destroy_box(box);
}

static int 
on_enter(LiteBox *box, int x, int y)
{
     LiteTextButton *buttonbox = LITE_TEXTBUTTON(box);

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_TEXT_BUTTON);

     lite_set_text_button_state(buttonbox, LITE_TBS_HILITE);

     return 1;
}

static int 
on_leave(LiteBox *box, int x, int y)
{
     LiteTextButton *buttonbox = LITE_TEXTBUTTON(box);

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_TEXT_BUTTON);

     lite_set_text_button_state(buttonbox, LITE_TBS_NORMAL);

     return 1;
}

static int 
on_button_down(LiteBox                       *box, 
               int                            x, 
               int                            y,
               DFBInputDeviceButtonIdentifier button)
{
     LiteTextButton *buttonbox = LITE_TEXTBUTTON(box);
     
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_TEXT_BUTTON);

     lite_set_text_button_state(buttonbox, LITE_TBS_PRESSED);

     return 1;
}

static int 
on_button_up(LiteBox                       *box, 
             int                            x, 
             int                            y,
             DFBInputDeviceButtonIdentifier button)
{
     LiteTextButton *buttonbox = LITE_TEXTBUTTON(box);

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_TEXT_BUTTON);

     /* do not send button selected messages unless mouse up occurs within
        the bounds of the widget */
     if (x < box->rect.w && y < box->rect.h) {
          lite_set_text_button_state(buttonbox, LITE_TBS_HILITE);
 
          if (buttonbox->enabled && buttonbox->press)
               buttonbox->press(buttonbox, buttonbox->press_data);
     }

     return 1;
}

static DFBResult 
draw_text_button(LiteBox         *box, 
            const DFBRegion *region, 
            DFBBoolean       clear)
{
     LiteTextButtonState state;
     LiteTextButton *button = LITE_TEXTBUTTON(box);
     DFBRectangle btn_rectangle;	

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_TEXT_BUTTON);

     if (button->enabled)
          state = button->state;
     else
          state = LITE_TBS_DISABLED;

     D_DEBUG_AT(LiteTextButtonDomain,
          "draw_button() state:%d. clear:%d\n", state, clear);


     if (clear)
          lite_clear_box(box, region);

     box->surface->SetClip(box->surface, NULL);
     box->surface->SetBlittingFlags(box->surface, DSBLIT_BLEND_ALPHACHANNEL);

     btn_rectangle.x = 0;
     btn_rectangle.y = 0;
     btn_rectangle.w = button->theme->all_images.width;
     btn_rectangle.h = button->theme->all_images.height / 4; // 4 states

     switch(state)
     {
     case LITE_TBS_DISABLED:
          btn_rectangle.y = btn_rectangle.h * 3;
          break;

     case LITE_TBS_HILITE:
          btn_rectangle.y = btn_rectangle.h * 2;
          break;

     case LITE_TBS_PRESSED:
          btn_rectangle.y = btn_rectangle.h;
          break;

     case LITE_TBS_NORMAL:
     default:
          btn_rectangle.y = 0; 
          break;
     }


     draw_text_button_bkgnd(box, button->theme->all_images.surface,
                            &btn_rectangle);

     if ( button->caption_text )
          draw_text_button_caption(box,
                                   button->caption_text,
                                   button->theme->font,
                                  &button->theme->font_color);

     return DFB_OK;
}

static void make_truncated_string(char* text, int width, IDirectFBFont* font_interface)
{
     DFBRectangle   ink_rect;
     char          *tail = "...";
     char          *buffer = NULL;
     int            i;

     font_interface->GetStringExtents(font_interface,text,-1, NULL, &ink_rect);
     if( ink_rect.w <= width )
          return;

     /* get tail's length */
     font_interface->GetStringExtents(font_interface, tail, -1, NULL, &ink_rect);
     if( ink_rect.w >= width )
     {
          strncpy(text, tail, strlen(text)); // prevent buffer overflow
          return;
     }

     buffer = malloc(strlen(text)+4);
     memset(buffer,0,strlen(text)+4);
     strncpy(buffer, text, MAX(0,(int)strlen(text)-3) );
     strcat(buffer, tail);

     while( 1 )
     {
          font_interface->GetStringExtents(font_interface,buffer,-1, NULL, &ink_rect);
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
draw_text_button_caption(LiteBox *box,
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

     make_truncated_string(buffer, box->rect.w-2*IMG_MARGIN, font_interface);

     font_interface->GetHeight(font_interface, &font_height);

     draw_text_pos_x = box->rect.w/2;
     draw_text_pos_y = (box->rect.h - font_height)/2;

     box->surface->SetFont(box->surface, font_interface);
     box->surface->SetColor(box->surface, font_color->r,
                            font_color->g, font_color->b,
                            font_color->a);

     box->surface->DrawString(box->surface, buffer, -1, draw_text_pos_x,
                              draw_text_pos_y, DSTF_CENTER|DSTF_TOP);     

}

static void
draw_text_button_bkgnd(LiteBox  *box,
               IDirectFBSurface *surface_image,
               DFBRectangle     *rc_src_img)
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

     DFBRectangle btn_rc, rc_dst[9], rc_img[9];
     int i;

     D_ASSERT(box != NULL);

     btn_rc.x = btn_rc.y = 0;
     btn_rc.w = box->rect.w;
     btn_rc.h = box->rect.h;

     /* left column */
     rc_dst[0].x = rc_dst[3].x = rc_dst[6].x = 0;
     rc_dst[0].w = rc_dst[3].w = rc_dst[6].w = IMG_MARGIN;
     rc_img[0].x = rc_img[3].x = rc_img[6].x = rc_src_img->x;
     rc_img[0].w = rc_img[3].w = rc_img[6].w = IMG_MARGIN;


     /* center column */
     rc_dst[1].x = rc_dst[4].x = rc_dst[7].x = IMG_MARGIN;
     rc_dst[1].w = rc_dst[4].w = rc_dst[7].w = btn_rc.w - 2*IMG_MARGIN;
     rc_img[1].x = rc_img[4].x = rc_img[7].x = rc_src_img->x + IMG_MARGIN;
     rc_img[1].w = rc_img[4].w = rc_img[7].w = rc_src_img->w - 2*IMG_MARGIN;


     /* right column */
     rc_dst[2].x = rc_dst[5].x = rc_dst[8].x = btn_rc.w - IMG_MARGIN;
     rc_dst[2].w = rc_dst[5].w = rc_dst[8].w = IMG_MARGIN;
     rc_img[2].x = rc_img[5].x = rc_img[8].x = rc_src_img->x + rc_src_img->w - IMG_MARGIN;
     rc_img[2].w = rc_img[5].w = rc_img[8].w = IMG_MARGIN;


     /* top row */
     rc_dst[0].y = rc_dst[1].y = rc_dst[2].y = 0;
     rc_dst[0].h = rc_dst[1].h = rc_dst[2].h = IMG_MARGIN;
     rc_img[0].y = rc_img[1].y = rc_img[2].y = rc_src_img->y;
     rc_img[0].h = rc_img[1].h = rc_img[2].h = IMG_MARGIN;


     /* center row */
     rc_dst[3].y = rc_dst[4].y = rc_dst[5].y = IMG_MARGIN;
     rc_dst[3].h = rc_dst[4].h = rc_dst[5].h = btn_rc.h - 2*IMG_MARGIN;
     rc_img[3].y = rc_img[4].y = rc_img[5].y = rc_src_img->y + IMG_MARGIN;
     rc_img[3].h = rc_img[4].h = rc_img[5].h = rc_src_img->h - 2*IMG_MARGIN;


     /* bottom row */
     rc_dst[6].y = rc_dst[7].y = rc_dst[8].y = btn_rc.h - IMG_MARGIN;
     rc_dst[6].h = rc_dst[7].h = rc_dst[8].h = IMG_MARGIN;
     rc_img[6].y = rc_img[7].y = rc_img[8].y = rc_src_img->y + rc_src_img->h - IMG_MARGIN;
     rc_img[6].h = rc_img[7].h = rc_img[8].h = IMG_MARGIN;

     
     for(i=0; i<sizeof(rc_dst)/sizeof(DFBRectangle); ++i)
          box->surface->StretchBlit(box->surface,
                                    surface_image, &rc_img[i], &rc_dst[i]);
}
