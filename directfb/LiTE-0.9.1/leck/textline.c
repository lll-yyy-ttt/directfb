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
#include <lite/lite_build.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <direct/mem.h>
#include <lite/lite.h>
#include <lite/util.h>
#include <lite/font.h>
#include <lite/window.h>

#include "textline.h"

#if LITE_HAS_FUSIONDALE
#include <core/ComponentData.h>
#endif

D_DEBUG_DOMAIN(LiteTextlineDomain, "LiTE/Textline", "LiteTextline");

LiteTextLineTheme *liteDefaultTextLineTheme = NULL;

struct _LiteTextLine {
     LiteBox            box;
     LiteTextLineTheme *theme;

     LiteFont          *font;
     char              *text;
     unsigned int       cursor_pos;
     int                modified;

     char              *backup;

     TextLineEnterFunc  enter;
     void              *enter_data;

     TextLineAbortFunc  abort;
     void              *abort_data;
};

static int on_focus_in(LiteBox *box);
static int on_focus_out(LiteBox *box);
static int on_key_down(LiteBox *box, DFBWindowEvent *ev);
static int on_button_down(LiteBox *box, int x, int y,
                           DFBInputDeviceButtonIdentifier button);

static DFBResult draw_textline(LiteBox *box, const DFBRegion *region, DFBBoolean clear);
static DFBResult destroy_textline(LiteBox *box);

DFBResult 
lite_new_textline(LiteBox           *parent, 
                  DFBRectangle      *rect,
                  LiteTextLineTheme *theme,
                  LiteTextLine     **ret_textline)
{
     DFBResult      res;
     LiteTextLine  *textline = NULL;
     IDirectFBFont *font_interface = NULL;

     textline = D_CALLOC(1, sizeof(LiteTextLine));

     textline->box.parent = parent;
     textline->theme = theme;
     textline->box.rect = *rect;

     res = lite_init_box(LITE_BOX(textline));
     if (res != DFB_OK) {
          D_FREE(textline);
          return res;
     }

     res = lite_get_font("default", LITE_FONT_PLAIN, rect->h *9/10 - 6,
                              DEFAULT_FONT_ATTRIBUTE, &textline->font);
     if (res != DFB_OK) {
          D_FREE(textline);
          return res;
     }

     res = lite_font(textline->font, &font_interface);
     if (res != DFB_OK) {
          D_FREE(textline);
          return res;
     }

     textline->box.type    = LITE_TYPE_TEXTLINE;
     textline->box.Draw    = draw_textline;
     textline->box.Destroy = destroy_textline;

     textline->box.OnFocusIn    = on_focus_in;
     textline->box.OnFocusOut   = on_focus_out;
     textline->box.OnKeyDown    = on_key_down;
     textline->box.OnButtonDown = on_button_down;

     textline->text = D_STRDUP("");

     textline->box.surface->SetFont(textline->box.surface, font_interface);

     res = lite_update_box(LITE_BOX(textline), NULL);
     if (res != DFB_OK) {
          D_FREE(textline);
          return res;
     }
     
     *ret_textline = textline;

     D_DEBUG_AT(LiteTextlineDomain, "Created new textline object: %p\n", textline);
     
     return DFB_OK;
}

DFBResult 
lite_set_textline_text(LiteTextLine *textline, const char *text)
{
     LITE_NULL_PARAMETER_CHECK(textline);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(textline), LITE_TYPE_TEXTLINE);
     LITE_NULL_PARAMETER_CHECK(text);

     D_DEBUG_AT(LiteTextlineDomain, "Set text: %s for textline: %p\n", text, textline);

     if (!strcmp(textline->text, text)) {
          if (!textline->modified)
               return DFB_OK;
     }
     else {
          if (textline->modified)
               D_FREE(textline->backup);

          D_FREE(textline->text);

          textline->text = D_STRDUP(text);
          textline->cursor_pos = strlen(text);
     }

     textline->modified = 0;

     return lite_update_box(LITE_BOX(textline), NULL);
}

DFBResult
lite_get_textline_text (LiteTextLine *textline, char **ret_text)
{
     LITE_NULL_PARAMETER_CHECK(textline);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(textline), LITE_TYPE_TEXTLINE);
     LITE_NULL_PARAMETER_CHECK(ret_text);

     *ret_text = D_STRDUP(textline->text);

     return DFB_OK;
}

DFBResult 
lite_on_textline_enter(LiteTextLine      *textline,
                         TextLineEnterFunc  func,
                         void              *funcdata)
{
     LITE_NULL_PARAMETER_CHECK(textline);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(textline), LITE_TYPE_TEXTLINE);

     textline->enter      = func;
     textline->enter_data = funcdata;

     return DFB_OK;
}

DFBResult 
lite_on_textline_abort(LiteTextLine      *textline,
                         TextLineAbortFunc  func,
                         void              *funcdata)
{
     LITE_NULL_PARAMETER_CHECK(textline);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(textline), LITE_TYPE_TEXTLINE);

     textline->abort      = func;
     textline->abort_data = funcdata;

     return DFB_OK;
}

/* internals */

static DFBResult 
destroy_textline(LiteBox *box)
{
     LiteTextLine *textline = NULL;

     D_ASSERT(box != NULL);

     textline = LITE_TEXTLINE(box);

     D_DEBUG_AT(LiteTextlineDomain, "Destroy textline: %p\n", textline);

     if (!textline)
          return DFB_FAILURE;

     if (textline->modified)
          D_FREE(textline->backup);

     D_FREE(textline->text);

     return lite_destroy_box(box);
}

static DFBResult 
draw_textline(LiteBox         *box, 
                const DFBRegion *region, 
                DFBBoolean       clear)
{
     DFBResult         result;
     IDirectFBSurface *surface  = box->surface;
     LiteTextLine     *textline = LITE_TEXTLINE(box);
     int               surface_x, cursor_x = 0;
     IDirectFBFont    *font_interface = NULL;

     D_ASSERT(box != NULL);

     result = lite_font(textline->font, &font_interface);
     if (result != DFB_OK)
          return result;

     surface->SetClip(surface, region);

     font_interface->GetStringWidth(font_interface, textline->text, textline->cursor_pos, &cursor_x);

     /* Draw border */
     surface->SetDrawingFlags(surface, DSDRAW_NOFX);

     if (box->is_focused)
          surface->SetColor(surface, 0xa0, 0xa0, 0xff, 0xff);
     else
          surface->SetColor(surface, 0xe0, 0xe0, 0xe0, 0xff);

     surface->DrawRectangle(surface, 0, 0, box->rect.w, box->rect.h);

     surface->SetColor(surface, 0xc0, 0xc0, 0xc0, 0xff);
     surface->DrawRectangle(surface, 1, 1, box->rect.w - 2, box->rect.h - 2);

     /* Fill the background */
     if (textline->modified)
          surface->SetColor(surface, 0xf0, 0xf0, 0xf0, 0xff);
     else
          surface->SetColor(surface, 0xf0, 0xf0, 0xf0, 0xf0);

     surface->FillRectangle(surface, 2, 2, box->rect.w - 4, box->rect.h - 4);

     /* Draw the text */
     surface->SetColor(surface, 0x30, 0x30, 0x30, 0xff);
     surface_x = 5;
     if (cursor_x > region->x2 - region->x1 - 5)
         surface_x = region->x2 - region->x1 - cursor_x - 5;

     surface->DrawString(surface, textline->text, -1, surface_x, 2, DSTF_TOPLEFT);
     cursor_x += surface_x - 5;

     /* Draw the cursor */
     surface->SetDrawingFlags(surface, DSDRAW_BLEND);

     if (box->is_focused)
          surface->SetColor(surface, 0x40, 0x40, 0x80, 0x80);
     else
          surface->SetColor(surface, 0x80, 0x80, 0x80, 0x80);

     surface->FillRectangle(surface, cursor_x + 5, 4, 1, box->rect.h - 8);

     return DFB_OK;
}

static int 
on_focus_in(LiteBox *box)
{
    D_ASSERT(box != NULL);

#if LITE_HAS_FUSIONDALE
	 OSKRequest request;
	 OSKRequest* req;
	 void *ptr;
	 if(lite_get_osk_component()) {
		 D_ASSERT(box != NULL);

		 request.inputRect = box->rect;
		 request.mode = Standard;
		 request.client = getpid();

		 lite_dale_get_local(sizeof(OSKRequest), &ptr);
		 req = (OSKRequest*) ptr;
		 *req = request;

		 lite_dale_call(lite_get_osk_component(), ShowOSK, (void*) req);
	 }
#endif

     lite_update_box(box, NULL);

     return 0;
}

static int 
on_focus_out(LiteBox *box)
{
     D_ASSERT(box != NULL);

#if LITE_HAS_FUSIONDALE
     if(lite_get_osk_component())
    	 lite_dale_call(lite_get_osk_component(), HideOSK, NULL);
#endif

     lite_update_box(box, NULL);

     return 0;
}

static void 
set_modified(LiteTextLine *textline)
{
     D_ASSERT(textline != NULL);

     if (textline->modified)
          return;

     textline->modified = true;

     textline->backup = D_STRDUP(textline->text);
}

static void 
clear_modified(LiteTextLine *textline, bool restore)
{
     D_ASSERT(textline != NULL);

     if (!textline->modified)
          return;

     textline->modified = false;

     if (restore) {
          D_FREE(textline->text);

          textline->text = textline->backup;
     }
     else
          D_FREE(textline->backup);

     textline->cursor_pos = 0;
}

static int 
on_key_down(LiteBox *box, DFBWindowEvent *ev)
{
     LiteTextLine *textline = LITE_TEXTLINE(box);
     int           redraw   = 0;

     D_ASSERT(box != NULL);

     switch (ev->key_symbol) {
          case DIKS_ENTER:
               if (textline->modified) {
                    if (textline->enter)
                         textline->enter(textline->text, textline->enter_data);

                    clear_modified(textline, false);

                    redraw = 1;
               }
               break;
          case DIKS_ESCAPE:
               if (textline->abort)
                    textline->abort(textline->abort_data);

               if (textline->modified) {
                    clear_modified(textline, true);

                    redraw = 1;
               }
               break;
          case DIKS_CURSOR_LEFT:
               if (textline->cursor_pos > 0) {
                    textline->cursor_pos--;
                    redraw = 1;
               }
               break;
          case DIKS_CURSOR_RIGHT:
               if (textline->cursor_pos < strlen (textline->text)) {
                    textline->cursor_pos++;
                    redraw = 1;
               }
               break;
          case DIKS_HOME:
               if (textline->cursor_pos > 0) {
                    textline->cursor_pos = 0;
                    redraw = 1;
               }
               break;
          case DIKS_END:
               if (textline->cursor_pos < strlen (textline->text)) {
                    textline->cursor_pos = strlen (textline->text);
                    redraw = 1;
               }
               break;
          case DIKS_DELETE:
               if (textline->cursor_pos < strlen (textline->text)) {
                    int len = strlen (textline->text);

                    set_modified(textline);

                    memmove(textline->text + textline->cursor_pos,
                             textline->text + textline->cursor_pos + 1,
                             len - textline->cursor_pos);

                    textline->text = D_REALLOC(textline->text, len);

                    redraw = 1;
               }
               break;
          case DIKS_BACKSPACE:
               if (textline->cursor_pos > 0) {
                    int len = strlen(textline->text);

                    set_modified(textline);

                    memmove(textline->text + textline->cursor_pos - 1,
                             textline->text + textline->cursor_pos,
                             len - textline->cursor_pos + 1);

                    textline->text = D_REALLOC(textline->text, len);

                    textline->cursor_pos--;

                    redraw = 1;
               }
               break;
          default:
               if (ev->key_symbol >= 32 && ev->key_symbol <= 127) {
                    int len = strlen(textline->text);

                    set_modified(textline);

                    textline->text = D_REALLOC(textline->text, len + 2);

                    memmove(textline->text + textline->cursor_pos + 1,
                             textline->text + textline->cursor_pos,
                             len - textline->cursor_pos + 1);

                    textline->text[textline->cursor_pos] = ev->key_symbol;

                    textline->cursor_pos++;

                    redraw = 1;
               }
               else
                    return 0;
     }

     if (redraw)
          lite_update_box(box, NULL);

     return 1;
}

static int 
on_button_down(LiteBox                       *box, 
               int                            x, 
               int                            y,
               DFBInputDeviceButtonIdentifier button)
{
     D_ASSERT(box != NULL);

     lite_focus_box(box);

     return 1;
}

