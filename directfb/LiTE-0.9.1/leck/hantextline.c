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
#include <string.h>

#include <lite/lite.h>
#include <lite/util.h>
#include <lite/font.h>
#include <lite/window.h>

#include <mext_imhangul.h>

#include "hantextline.h"


D_DEBUG_DOMAIN(LiteHanTextlineDomain, "LiTE/HanTextline", "HanLiteTextline");

LiteHanTextLineTheme *liteDefaultHanTextLineTheme = NULL;
MExtHangulDescription han_desc;
IMExtHangul *han = NULL;
int han_showkeyevent = 0;
int han_refcount = 0;

#define TEXTDRAW_OFFSET_X  (3)
#define ABS(X)      ( (X)<0 ? -1*(X):(X) )

struct _LiteHanTextLine {
     LiteBox               box;
     LiteHanTextLineTheme *theme;

     LiteFont             *font;
     char                 *text;
     unsigned int          cursor_pos;/**< N-th byte, NOT N-th character */
     int                   modified;
     int                   scroll;    /**< horizontal scroll position in pixel unit */

     char                 *backup;

     HanTextLineEnterFunc  enter;
     void                 *enter_data;

     HanTextLineAbortFunc  abort;
     void                 *abort_data;
};

typedef enum {
     CARET_MOVE_LEFT,
     CARET_MOVE_RIGHT,
     STRING_DEL,
     STRING_INSERT,
     STRING_WHOLE_NEW,
} SCROLL_ADJUST_HINT;

static int on_focus_in(LiteBox *box);
static int on_focus_out(LiteBox *box);
static int on_key_down(LiteBox *box, DFBWindowEvent *ev);
static int on_button_down(LiteBox *box, int x, int y,
                           DFBInputDeviceButtonIdentifier button);

static DFBResult draw_hantextline(LiteBox *box, const DFBRegion *region, DFBBoolean clear);
static DFBResult destroy_hantextline(LiteBox *box);
static void adjust_scroll_pos(LiteHanTextLine *textline, SCROLL_ADJUST_HINT hint);
static void flush_preedit(LiteHanTextLine *textline);
static void setup_hangul(void);
static void release_hangul(void);
static void dump_string(char* str);
static void dump_data(LiteHanTextLine *textline);

DFBResult 
lite_new_hantextline(LiteBox           *parent, 
                  DFBRectangle         *rect,
                  LiteHanTextLineTheme *theme,
                  LiteHanTextLine     **ret_textline)
{
     DFBResult         res;
     LiteHanTextLine  *textline = NULL;
     IDirectFBFont    *font_interface = NULL;

     LITE_NULL_PARAMETER_CHECK(parent);
     LITE_NULL_PARAMETER_CHECK(rect);
     LITE_NULL_PARAMETER_CHECK(ret_textline);

     if( han_refcount == 0 )
          setup_hangul();
     ++han_refcount;

     textline = D_CALLOC(1, sizeof(LiteHanTextLine));

     textline->box.parent = parent;
     textline->theme = theme;
     textline->box.rect = *rect;

     res = lite_init_box(LITE_BOX(textline));
     if (res != DFB_OK) {
          D_FREE(textline);
          return res;
     }

     res = lite_get_font("hangul", LITE_FONT_PLAIN, rect->h *9/10 - 6,
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

     textline->box.type    = LITE_TYPE_HANTEXTLINE;
     textline->box.Draw    = draw_hantextline;
     textline->box.Destroy = destroy_hantextline;

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

     D_DEBUG_AT(LiteHanTextlineDomain, "Created new hantextline object: %p\n", textline);
     
     return DFB_OK;
}

DFBResult 
lite_set_hantextline_text(LiteHanTextLine *textline, const char *text)
{
     LITE_NULL_PARAMETER_CHECK(textline);
     LITE_NULL_PARAMETER_CHECK(text);

     D_DEBUG_AT(LiteHanTextlineDomain, "Set text: %s for hantextline: %p\n", text, textline);

     han->ICReset(han);

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
lite_on_hantextline_enter(LiteHanTextLine      *textline,
                         HanTextLineEnterFunc  func,
                         void                 *funcdata)
{
     LITE_NULL_PARAMETER_CHECK(textline);

     textline->enter      = func;
     textline->enter_data = funcdata;

     return DFB_OK;
}

DFBResult 
lite_on_hantextline_abort(LiteHanTextLine      *textline,
                         HanTextLineAbortFunc  func,
                         void                 *funcdata)
{
     LITE_NULL_PARAMETER_CHECK(textline);

     textline->abort      = func;
     textline->abort_data = funcdata;

     return DFB_OK;
}

/* internals */

static DFBResult 
destroy_hantextline(LiteBox *box)
{
     LiteHanTextLine *textline = NULL;

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(box, LITE_TYPE_HANTEXTLINE);

     textline = LITE_HANTEXTLINE(box);

     D_DEBUG_AT(LiteHanTextlineDomain, "Destroy hantextline: %p\n", textline);

     if (!textline)
          return DFB_FAILURE;

     if (textline->modified)
          D_FREE(textline->backup);

     D_FREE(textline->text);

     han->ICReset(han);

     --han_refcount;
     if( han_refcount == 0 )
          release_hangul();

     return lite_destroy_box(box);
}

static DFBResult 
draw_hantextline(LiteBox         *box, 
                const DFBRegion *region, 
                DFBBoolean       clear)
{
     DFBResult         result;
     IDirectFBSurface *surface  = box->surface;
     LiteHanTextLine  *textline = LITE_HANTEXTLINE(box);
     int               cursor_x = 0, width_preedit=0;
     IDirectFBFont    *font_interface = NULL;
     DFBBoolean        composing = 0;

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(textline), LITE_TYPE_HANTEXTLINE);

     result = lite_font(textline->font, &font_interface);
     if (result != DFB_OK)
          return result;

     surface->SetClip(surface, region);

//     font_interface->GetStringWidth(font_interface, textline->text, textline->cursor_pos, &cursor_x);
     han->GetStringWidth(han, surface, textline->text,
          textline->cursor_pos, &cursor_x);
     han->GetPreeditWidth(han, surface, &width_preedit);

     surface->SetDrawingFlags(surface, DSDRAW_NOFX);

     /* Fill the background */
     if (textline->modified)
          surface->SetColor(surface, 0xf0, 0xf0, 0xf0, 0xff);
     else
          surface->SetColor(surface, 0xf0, 0xf0, 0xf0, 0xf0);

     surface->FillRectangle(surface, 2, 2, box->rect.w - 4, box->rect.h - 4);

     /* Draw the text */
     surface->SetColor(surface, 0x30, 0x30, 0x30, 0xff);
//     surface->DrawString(surface, textline->text, -1,
//          TEXTDRAW_OFFSET_X-textline->scroll, 2, DSTF_TOPLEFT);
     han->DrawString(han, surface, textline->text, textline->cursor_pos,
          TEXTDRAW_OFFSET_X-textline->scroll, 2, DSTF_TOPLEFT);
     han->DrawPreedit(han, surface,
          TEXTDRAW_OFFSET_X-textline->scroll+cursor_x, 2, DSTF_TOPLEFT, NULL);
     if( textline->cursor_pos < strlen(textline->text) )
          han->DrawString(han, surface, textline->text+textline->cursor_pos,
               strlen(textline->text)-textline->cursor_pos,
               TEXTDRAW_OFFSET_X-textline->scroll+cursor_x+width_preedit, 2,
               DSTF_TOPLEFT);

     /* Draw border */
     if (box->is_focused)
          surface->SetColor(surface, 0xa0, 0xa0, 0xff, 0xff);
     else
          surface->SetColor(surface, 0xe0, 0xe0, 0xe0, 0xff);

     surface->DrawRectangle(surface, 0, 0, box->rect.w, box->rect.h);

     surface->SetColor(surface, 0xc0, 0xc0, 0xc0, 0xff);
     surface->DrawRectangle(surface, 1, 1, box->rect.w - 2, box->rect.h - 2);

     /* Draw the cursor */
     surface->SetDrawingFlags(surface, DSDRAW_BLEND);

     if (box->is_focused)
          surface->SetColor(surface, 0x40, 0x40, 0x80, 0x80);
     else
          surface->SetColor(surface, 0x80, 0x80, 0x80, 0x80);

     han->ICIsComposing(han, &composing);
     if( composing )
          surface->DrawRectangle(surface, 
                cursor_x+TEXTDRAW_OFFSET_X-textline->scroll,
                2, width_preedit, box->rect.h - 6);
     else
          surface->FillRectangle(surface,
                cursor_x+TEXTDRAW_OFFSET_X-textline->scroll,
                4, 1, box->rect.h - 8);

     return DFB_OK;
}

static int 
on_focus_in(LiteBox *box)
{
     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_HANTEXTLINE);

     lite_update_box(box, NULL);

     return 0;
}

static int 
on_focus_out(LiteBox *box)
{
     LiteHanTextLine *textline = LITE_HANTEXTLINE(box);
     DFBBoolean composing = 0;
     char *new_cursor_ptr = NULL;

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(box), LITE_TYPE_HANTEXTLINE);

     han->ICIsComposing(han, &composing);

     if( composing )
     {
          flush_preedit(textline);

          han->HSCharNext(han,
               textline->text+textline->cursor_pos,
               &new_cursor_ptr);
          textline->cursor_pos = new_cursor_ptr-textline->text;
     }

     lite_update_box(box, NULL);

     return 0;
}

static void 
set_modified(LiteHanTextLine *textline)
{
     D_ASSERT(textline != NULL);

     if (textline->modified)
          return;

     textline->modified = true;

     textline->backup = D_STRDUP(textline->text);
}

static void 
clear_modified(LiteHanTextLine *textline, bool restore)
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
     textline->scroll = 0;
}

static int 
on_key_down(LiteBox *box, DFBWindowEvent *ev)
{
     LiteHanTextLine *textline = LITE_HANTEXTLINE(box);
     int              redraw   = 0;
     DFBBoolean       composing = 0;
     char             buf[32] = {0,};
     int              size_ret=0, len=0;
     char            *new_cursor_ptr = 0;

     D_ASSERT( box != NULL );
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(textline), LITE_TYPE_HANTEXTLINE);


     han->ICIsComposing(han, &composing);

     D_ASSERT(box != NULL);
     D_DEBUG_AT(LiteHanTextlineDomain,
          "on_key_down(), key_symbol = %d\n", ev->key_symbol);

     if( (ev->key_symbol>=32 && ev->key_symbol<=127 && ev->key_symbol!=DIKS_DELETE)
          || ev->key_symbol == DIKS_BACKSPACE )
     {
          MExtHangulLookupStatus status;
          
          han->LookupString(han, (DFBEvent*)ev,
               buf, sizeof(buf), &size_ret, &status);

          D_DEBUG_AT(LiteHanTextlineDomain,
               "on_key_down(), LookupString - size_ret:%d, status:%d\n",
               size_ret, status);

          len = strlen(textline->text);

          if( size_ret && status )
          {
               set_modified(textline);
               textline->text = D_REALLOC(textline->text, len+size_ret+1);
          }

          if( status == MEHS_LOOKUP_CHARS || status == MEHS_LOOKUP_BOTH )
          {
               buf[size_ret] = 0;
               memmove(textline->text+textline->cursor_pos+size_ret,
                    textline->text+textline->cursor_pos,
                    strlen(textline->text+textline->cursor_pos)+1);
               memcpy(textline->text+textline->cursor_pos, buf,
                    size_ret);
               textline->text[len+size_ret] = 0;              
               textline->cursor_pos += size_ret;
          }    
          adjust_scroll_pos(textline, STRING_INSERT);

          redraw = 1;
     }

     dump_data(textline);

     if( composing )
     {
          switch( ev->key_symbol )
          {
          case DIKS_ENTER:
          case DIKS_CURSOR_LEFT:
          case DIKS_CURSOR_RIGHT:
          case DIKS_HOME:
          case DIKS_END:
          case DIKS_DELETE:
               flush_preedit(textline);
               redraw = 1;
               // Here we don't change cursor_pos value
               break;
              
          default:
               break; 
          }
     }



     switch (ev->key_symbol) {
          case DIKS_ENTER:
               if (textline->modified) {
                    if (textline->enter)
                         textline->enter(textline->text, textline->enter_data);

                    clear_modified(textline, false);

                    redraw = 1;
               }
               if(composing)
               {
                    goto Label_DIKS_CURSOR_RIGHT;
               }
               else
                    break;
          case DIKS_ESCAPE:
               han->ICReset(han);
               if (textline->abort)
                    textline->abort(textline->abort_data);

               if (textline->modified) {
                    clear_modified(textline, true);

                    adjust_scroll_pos(textline, STRING_WHOLE_NEW);
                    redraw = 1;
               }
               break;
          case DIKS_CURSOR_RIGHT:
Label_DIKS_CURSOR_RIGHT:
               if (textline->cursor_pos < strlen (textline->text))
               {
                    han->HSCharNext(han,
                         textline->text+textline->cursor_pos,
                         &new_cursor_ptr);
                    textline->cursor_pos = new_cursor_ptr-textline->text;
                    adjust_scroll_pos(textline, CARET_MOVE_RIGHT);
                    redraw = 1;
               }
               break;
          case DIKS_CURSOR_LEFT:
               if (!composing && textline->cursor_pos > 0)
               {
                    han->HSCharPrev(han,
                         textline->text+textline->cursor_pos,
                         textline->text,
                         &new_cursor_ptr);
                    textline->cursor_pos = new_cursor_ptr-textline->text;
                    adjust_scroll_pos(textline, CARET_MOVE_LEFT);
                    redraw = 1;
               }
               break;
          case DIKS_HOME:
               if (textline->cursor_pos > 0) {
                    textline->cursor_pos = 0;
                    adjust_scroll_pos(textline, CARET_MOVE_LEFT);
                    redraw = 1;
               }
               break;
          case DIKS_END:
               if (textline->cursor_pos < strlen (textline->text)) {
                    textline->cursor_pos = strlen (textline->text);
                    adjust_scroll_pos(textline, CARET_MOVE_RIGHT);
                    redraw = 1;
               }
               break;
          case DIKS_DELETE:
               if (composing)
                    goto Label_DIKS_CURSOR_RIGHT;

               if (textline->cursor_pos < strlen (textline->text)) {
                    set_modified(textline);

                    han->HSCharNext(han,
                         textline->text+textline->cursor_pos,
                         &new_cursor_ptr);
                    memmove(textline->text + textline->cursor_pos,
                             new_cursor_ptr,
                             strlen(new_cursor_ptr)+1);

                    textline->text = D_REALLOC(textline->text,
                         strlen(textline->text)+1);

                    adjust_scroll_pos(textline, STRING_DEL);
                    redraw = 1;
               }
               break;
          case DIKS_BACKSPACE:
               if (!composing && textline->cursor_pos > 0) {
                    int len = strlen(textline->text);

                    set_modified(textline);

                    han->HSCharPrev(han,
                         textline->text+textline->cursor_pos,
                         textline->text,
                         &new_cursor_ptr);

                    memmove(new_cursor_ptr,
                         textline->text + textline->cursor_pos,
                         len-textline->cursor_pos+1);// including NULL

                    textline->text = D_REALLOC(textline->text,
                         strlen(textline->text)+1);

                    textline->cursor_pos = new_cursor_ptr-textline->text;

                    adjust_scroll_pos(textline, STRING_DEL);
                    redraw = 1;
               }
               break;

          default:
               break;
     }

     dump_data(textline);

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
     IDirectFBSurface *surface  = box->surface;
     LiteHanTextLine  *textline = LITE_HANTEXTLINE(box);
     int pos=0, total_width = 0, width=0, distance=0, str_len;
     char *cur_ptr;
     DFBBoolean composing=0;

     D_ASSERT(box != NULL);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(textline), LITE_TYPE_HANTEXTLINE);

     lite_focus_box(box);

     // find new cursor position that is nearest to the mouse-down point

     D_DEBUG_AT(LiteHanTextlineDomain, "textline mouse down. x:%d, y:%d\n", x, y);

     dump_data(textline);

     han->ICIsComposing(han,&composing);
     if( composing )
     {
          flush_preedit(textline);
     }

     dump_data(textline);

     str_len = strlen(textline->text);
     han->GetStringWidth(han, surface, textline->text, str_len, &total_width);

     if( (x+textline->scroll) <= TEXTDRAW_OFFSET_X )
          pos = 0;
     if( ( x+textline->scroll ) >= total_width )
          pos = str_len;
     else
     {
          distance = total_width;
     
          for( cur_ptr = textline->text;
               cur_ptr < textline->text+str_len;
               han->HSCharNext(han, cur_ptr, &cur_ptr))
          {
               han->GetStringWidth(han, surface, textline->text,
                    cur_ptr-textline->text, &width);

               if( distance >= ABS(width-(x+textline->scroll-TEXTDRAW_OFFSET_X)) )
               {
                    pos = cur_ptr-textline->text;
                    distance = ABS(width-(x+textline->scroll-TEXTDRAW_OFFSET_X)) ;
               }
          }
     }
     textline->cursor_pos = pos;
     lite_update_box(box, NULL);

     dump_data(textline);

     return 1;
}


// Adjust scroll value to make the caret always visible
void adjust_scroll_pos(LiteHanTextLine *textline, SCROLL_ADJUST_HINT hint)
{
     IDirectFBSurface *surface;
     int               pos=0, total_length, width_preedit=0;
     DFBBoolean        composing=0;

     surface = textline->box.surface;
     han->GetPreeditWidth(han, surface, &width_preedit);

     han->GetStringWidth(han, surface, textline->text,
          strlen(textline->text), &total_length);
     if( total_length+width_preedit<textline->box.rect.w-2*TEXTDRAW_OFFSET_X )
     {
          textline->scroll = 0;
          return;
     }

     han->GetStringWidth(han, surface, textline->text,
          textline->cursor_pos, &pos);     

     if( 0 <= pos-textline->scroll+width_preedit
          && pos-textline->scroll+width_preedit < textline->box.rect.w-2*TEXTDRAW_OFFSET_X )
     {
          if( textline->cursor_pos == strlen(textline->text) && textline->scroll > 0 )
          {
               textline->scroll = (pos - (textline->box.rect.w-2*TEXTDRAW_OFFSET_X)+width_preedit);
          }
          else if( total_length-textline->scroll<textline->box.rect.w-2*TEXTDRAW_OFFSET_X
               && textline->scroll > 0
               && STRING_DEL == hint )
          {
               textline->scroll = (total_length - (textline->box.rect.w-2*TEXTDRAW_OFFSET_X));
          }
          if( !composing )
               return;
     }

     if( pos-textline->scroll < 0 )
     {
          textline->scroll = pos;
     }

     if( hint != CARET_MOVE_LEFT )
     {
          if( pos-textline->scroll > textline->box.rect.w-2*TEXTDRAW_OFFSET_X ) 
          {
               textline->scroll =
                    (pos - (textline->box.rect.w-2*TEXTDRAW_OFFSET_X));
          }

          D_DEBUG_AT(LiteHanTextlineDomain,
               "adjust_scroll(), width_preedit:%d, pos:%d,"
               " scroll:%d, rect.w:%d, calc:%d\n",
               width_preedit, pos, textline->scroll,
               textline->box.rect.w-2*TEXTDRAW_OFFSET_X,
               pos+width_preedit-textline->scroll
               - (textline->box.rect.w-2*TEXTDRAW_OFFSET_X));

          if( pos+width_preedit-textline->scroll >= 
               textline->box.rect.w-2*TEXTDRAW_OFFSET_X )
          {
              textline->scroll = 
                    (pos - (textline->box.rect.w-2*TEXTDRAW_OFFSET_X))
                    + width_preedit;
          
          }
     }
}

void setup_hangul(void)
{
     char *toggle_key, *show_keyevent_string;
     IDirectFB* idfb;
     DFBResult res;

     D_ASSERT( han == NULL );

     /* setup hangul */
     idfb = lite_get_dfb_interface();
     D_DEBUG_AT(LiteHanTextlineDomain, "setup_hangul(), idfb:0x%08X\n", (int)idfb);

     res = idfb->GetInterface(idfb, "IMExtHangul", "Default", NULL, (void**)&han);
     han->GetDescription(han, &han_desc);
     han_desc.keyboard = MEHK_2;
     han_desc.encoding = MEHE_UTF8;

     if( ( toggle_key = getenv("DFBHAN_TOGGLEKEY")) != NULL )
     {
          char *nptr = toggle_key;
          char *endptr = NULL;

          han_desc.hangul_key[1].symbol = strtoul(nptr, &endptr, 0);
          han_desc.hangul_key[1].modifiers = 0; // no modifier
          if( *endptr == ' ' || *endptr == ',' ) 
          {
               nptr = endptr+1;
               han_desc.hangul_key[1].modifiers = strtoul(nptr, &endptr, 0);
          }
          D_DEBUG_AT(LiteHanTextlineDomain, "DFBHAN_TOGGLEKEY: symbol:%04x, modi:%04x\n",
               han_desc.hangul_key[1].symbol,
               han_desc.hangul_key[1].modifiers);
     }

     han->SetDescription(han, &han_desc);
     han_showkeyevent = 0;
     show_keyevent_string = getenv("DFBHAN_SHOWKEYEVENTS");
     if( show_keyevent_string != NULL && strlen(show_keyevent_string)>0 )
          han_showkeyevent = 1;
}

void release_hangul(void)
{
     D_ASSERT( han != NULL );

     han->Release(han);
     han = NULL;
}

/*
 * flush preedit contents to textline->text, but do not change textline->cursor_pos
 */
void flush_preedit(LiteHanTextLine *textline)
{
     char             buf[32] = {0,};
     int              size_ret=0, len=0;

     han->ICFlush(han, buf, sizeof(buf), &size_ret);
     if( size_ret > 0 )
     {
          set_modified(textline);
          buf[size_ret]=0;

          printf("flush_preedit(), buf:'");
          dump_string(buf);
          printf("', size_ret:%d\n", size_ret);

          len = strlen(textline->text);
          textline->text = D_REALLOC(textline->text,
               len+size_ret+1);
          memmove(textline->text+textline->cursor_pos+size_ret,
               textline->text+textline->cursor_pos,
               len-textline->cursor_pos);
          memcpy(textline->text+textline->cursor_pos,
               buf,
               size_ret);
          textline->text[len+size_ret]=0;
     }
}

void dump_string(char* str)
{
     unsigned char *p = (unsigned char*)str;

     while(*p)
     {
          printf(" %02X", *p);
          ++p;
     }
}

void dump_data(LiteHanTextLine *textline)
{

     printf("text : '");

     dump_string(textline->text);

     printf("', len:%d, cursor_pos:%d\n",
          strlen(textline->text),
          textline->cursor_pos);
}
