/*
 * DFBTerm
 *
 * Copyright (C) 2001-2002  convergence integrated media
 * Authors: Denis Oliver Kropp <dok@directfb.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>

#include <pthread.h>
#include <directfb.h>

#include <lite/lite.h>
#include <lite/font.h>
#include <lite/window.h>

#include "vtx.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define BGALPHA 0xe0

#define TERM_DEFAULT_FONTSIZE 13
#define TERM_DEFAULT_WIDTH    100
#define TERM_DEFAULT_HEIGHT   30

typedef struct {
     pthread_t             update_thread;
     bool                  update_closing;

     pthread_mutex_t       lock;

     LiteWindow           *lw;
     IDirectFBSurface     *surface;
     IDirectFBFont        *font;
     struct _vtx          *vtx;
     int                   CW, CH;
     int                   width, height;
     int                   cols, rows;
     int                   cursor_state;
     DFBRegion             flip_region;
     DFBBoolean            flip_pending;

     IDirectFBSurface     *bar_surface;
     int                   bar_start, bar_end;

     DFBBoolean            in_resize;
     DFBBoolean            minimized;
     DFBBoolean            hot_key;

     struct timeval        last_click;

     DFBInputDeviceModifierMask modifiers;
} Term;


static void *vt_update_thread (void *arg);

static void vt_draw_text    (void           *user_data,
                             struct vt_line *line,
                             int             row,
                             int             col,
                             int             len,
                             int             attr);

static void vt_scroll_area  (void           *user_data,
                             int             firstrow,
                             int             count,
                             int             offset,
                             int             fill);

static int  vt_cursor_state (void           *user_data,
                             int             state);


static void term_update_scrollbar (Term           *term);

static void term_handle_button    (Term           *term,
                                   DFBWindowEvent *evt);
static void term_handle_motion    (Term           *term,
                                   DFBWindowEvent *evt);

static void term_handle_key       (Term           *term,
                                   DFBWindowEvent *evt);

static void term_scroll           (Term           *term,
                                   int             scroll);

static void term_add_flip         (Term           *term,
                                   DFBRegion      *region);

static void term_flush_flip       (Term           *term);

static int term_on_resize (LiteWindow *lw, int width, int height);

static void term_usage (void);

/* static data */

/* The first 16 values are the ansi colors, the last
 * two are the default foreground and default background
 */
static const u8 default_red[] =
{0x00,0xaa,0x00,0xaa,0x00,0xaa,0x00,0xbb,
     0x77,0xff,0x55,0xff,0x55,0xff,0x55,0xff,
     0xb0,0x00};

static const u8 default_grn[] =
{0x00,0x00,0xaa,0x55,0x00,0x00,0xaa,0xbb,
     0x77,0x55,0xff,0xff,0x55,0x55,0xff,0xff,
     0xb0,0x00};

static const u8 default_blu[] =
{0x00,0x00,0x00,0x00,0xaa,0xaa,0xaa,0xbb,
     0x77,0x55,0x55,0x55,0xff,0xff,0xff,0xff,
     0xb0,0x00};

/* remapping table for function keys 5-12 */
static const unsigned char f5_f12_remap[] = {15,17,18,19,20,21,23,24};


/***/

static Term      *gterm;
static IDirectFB *dfb;

int
main (int argc, char *argv[])
{
     int                     id, quit = 0;
     DFBResult               ret;
     DFBRectangle            rect;
     DFBFontDescription      desc;
     IDirectFBWindow        *window;
     IDirectFBSurface       *surface;
     IDirectFBEventBuffer   *buffer;
     IDirectFBFont          *font;
     DFBRectangle            win_rect;
     struct _vtx            *vtx;
     Term                   *term;
     char                   *command    = NULL;
     int                     fontsize   = TERM_DEFAULT_FONTSIZE;
     char                   *geometry   = 0;
     char                   *geosep     = 0;
     int                     termpos    = 0;
     int                     termposx   = 0;
     int                     termposy   = 0;
     int                     termwidth  = TERM_DEFAULT_WIDTH;
     int                     termheight = TERM_DEFAULT_HEIGHT;
     int                     i          = 0;

     ret = DirectFBInit (&argc, &argv);
     if (ret) {
          DirectFBError ("DirectFBInit", ret);
          return -1;
     }

     /* parse command line options */
     for (i = 1; i < argc; i++) {
          if (!strcmp (argv[i], "--help")) {
               term_usage();
               return 0;
          }
          else if (strstr (argv[i], "--fontsize=") == argv[i]) {
               fontsize = atoi (1 + index (argv[i], '='));
               if (fontsize < 2 || fontsize > 666) {
                    DirectFBError ("bad fontsize", 1);
                    return -666;
               }
          }
          else if ((strstr (argv[i], "--geometry=") == argv[i]) ||
                   (strstr (argv[i], "--size=") == argv[i])) {
               geometry = 1 + index (argv[i], '=');
               geosep   = index (geometry, 'x');
               if (!geosep) {
                    DirectFBError ("bad geometry format", 1);
                    term_usage();
                    return -666;
               }

               termheight = atoi (geosep + 1);
               if (termheight < 1 || termheight > 666) {
                    DirectFBError ("bad geometry height", 1);
                    return -666;
               }

               (*geosep) = 0;
               termwidth = atoi (geometry);
               if (termwidth < 1 || termwidth > 666) {
                    DirectFBError ("bad geometry width", 1);
                    return -666;
               }
          }
          else if (strstr (argv[i], "--position=") == argv[i]) {
               geometry = 1 + index (argv[i], '=');
               geosep   = index (geometry, ',');
               if (!geosep) {
                    DirectFBError ("bad position format", 1);
                    term_usage();
                    return -666;
               }

               termposy = atoi (geosep + 1);

               (*geosep) = 0;
               termposx = atoi (geometry);

               termpos = 1;
          }
          else if (strstr (argv[i], "-c") == argv[i]) {
               if (argv[i + 1] && *argv[i + 1] && *argv[i + 1] != '-')
                    command = strdup (argv[++i]);
          }

     }

     ret = DirectFBCreate (&dfb);
     if (ret) {
          DirectFBError ("DirectFBCreate", ret);
          return -2;
     }

     if (lite_open( &argc, &argv ) != DFB_OK) {
          dfb->Release (dfb);
          return -3;
     }


     /* Load terminal font */
     desc.flags  = DFDESC_HEIGHT;
     desc.height = fontsize;

     ret = dfb->CreateFont (dfb, LITEFONTDIR"/Misc-Fixed.pfa", &desc, &font);
     if (ret) {
          lite_close();
          dfb->Release (dfb);
          return -4;
     }

     gterm = term = calloc (1, sizeof(Term));

     pthread_mutex_init (&term->lock, NULL);

     term->font = font;
     term->cols = termwidth;
     term->rows = termheight;

     font->GetGlyphExtents (font, 'O', NULL, &term->CW);
     font->GetHeight (font, &term->CH);

     term->CH--;

     term->width  = term->CW * termwidth;
     term->height = term->CH * termheight;

     /* Create window */
     win_rect.x = LITE_CENTER_HORIZONTALLY;
     win_rect.y = LITE_CENTER_VERTICALLY;
     win_rect.w = term->width+2;
     win_rect.h = term->height;

     ret = lite_new_window ( NULL,
                             &win_rect,
                             DWCAPS_ALPHACHANNEL,
                             liteDefaultWindowTheme,
                             "Terminal",
                             &term->lw);
     if (!term->lw) {
          lite_close();
          dfb->Release (dfb);
          return -5;
     }

     /* Due to the custom event handling routine we need this. */
     lite_draw_box( LITE_BOX(term->lw), NULL, true );
     term->lw->flags |= LITE_WINDOW_DRAWN;

     lite_set_window_background( term->lw, NULL );
     lite_set_window_blend_mode( term->lw, LITE_BLEND_AUTO, LITE_BLEND_AUTO );

     term->lw->step_x = term->CW;
     term->lw->step_y = term->CH;

     term->lw->OnResize = term_on_resize;

     window = term->lw->window;
     surface = term->lw->box.surface;

     if (termpos)
          window->MoveTo( window, termposx, termposy );

     /* Initialize sub area for terminal */
     rect.x = 0;
     rect.y = 0;
     rect.w = term->width;
     rect.h = term->height;

     surface->GetSubSurface (surface, &rect, &term->surface);

     term->surface->Clear (term->surface, 0x00, 0x00, 0x00, BGALPHA);
     term->surface->Flip (term->surface, NULL, 0);

     term->surface->SetFont (term->surface, font);

     /* Initialize sub area for scroll bar */
     rect.x = term->width;
     rect.w = 2;

     surface->GetSubSurface (surface, &rect, &term->bar_surface);

     /* Create event buffer */
     lite_get_event_buffer( &buffer );

     /* VTX */
     vtx = vtx_new (term->cols, term->rows, term);

     vtx->draw_text = vt_draw_text;
     vtx->scroll_area = vt_scroll_area;
     vtx->cursor_state = vt_cursor_state;

     if ((id = vt_forkpty (&vtx->vt,
                           VT_DO_UTMP_LOG | VT_DO_WTMP_LOG | VT_DO_LASTLOG)) == 0) {
          char *shell, *name;
          struct passwd *pw;

          /* get shell from passwd */
          pw = getpwuid (getuid());
          if (pw) {
               shell = pw->pw_shell;
               name  = strrchr (shell, '/');
          }
          else {
               shell = "/bin/sh";
               name  = "sh";
          }

          setenv ("TERM", "xterm", 1);

          chdir(getenv("HOME"));

          if (command) {
               char cmd[strlen (command) + 1];
               strcpy (cmd, command);
               free (command);
               execl (shell, name, "-c", cmd, NULL);
          }
          else {
               execl (shell, name,  NULL);
          }

          perror ("Could not exec");
          _exit(127);
     }
     else if (id == -1) {
          fprintf (stderr, "vt_forkpty failed.\n");
          term->surface->Release (term->surface);
          term->bar_surface->Release (term->bar_surface);
          lite_destroy_window (term->lw);
          lite_close();
          dfb->Release (dfb);
          return -6;
     }

     term->vtx    = vtx;

     term->bar_start = -1;
     term->bar_end   = -1;

     pthread_mutex_lock (&term->lock);

     pthread_create (&term->update_thread, NULL, vt_update_thread, (void*) term);

     vt_cursor_state (term, 1);

     lite_set_window_opacity( term->lw, 0xd0 );

     vt_scrollback_set (&vtx->vt, 4000);

     window->RequestFocus( window );

     /* Loop */
     while (!quit && !term->update_closing) {
          DFBWindowEvent evt;
          int            scroll = 0;

          pthread_mutex_unlock (&term->lock);

          buffer->WaitForEvent (buffer);

          pthread_mutex_lock (&term->lock);

          while (buffer->GetEvent (buffer, DFB_EVENT(&evt)) == DFB_OK) {
               if (lite_handle_window_event (term->lw, &evt))
                    continue;

               switch (evt.type) {
                    case DWET_GOTFOCUS:
                         lite_set_window_opacity( term->lw, 0xff );
                         break;
                    case DWET_LOSTFOCUS:
                         lite_set_window_opacity( term->lw, 0xd0 );
                         /* fall through */
                    case DWET_BUTTONDOWN:
                    case DWET_BUTTONUP:
                         term_handle_button (term, &evt);
                         break;
                    case DWET_MOTION:
                         term_handle_motion (term, &evt);
                         break;
                    case DWET_KEYDOWN:
                         if (term->modifiers & DIMM_SHIFT) {
                              switch (evt.key_symbol) {
                                   case DIKS_CURSOR_UP:
                                        scroll -= 1;
                                        break;
                                   case DIKS_CURSOR_DOWN:
                                        scroll += 1;
                                        break;
                                   case DIKS_PAGE_UP:
                                        scroll -= term->rows - 1;
                                        break;
                                   case DIKS_PAGE_DOWN:
                                        scroll += term->rows - 1;
                                        break;
                                   default:
                                        term_handle_key (term, &evt);
                              }
                         }
                         else
                    case DWET_KEYUP:
                              term_handle_key (term, &evt);
                         break;
                    case DWET_WHEEL:
                         if (evt.modifiers & DIMM_CONTROL) {
                              if (evt.step > 0) {
                                   if (vtx->vt.mode & VTMODE_APP_CURSOR)
                                        for (i=0; i<evt.step; i++)
                                             vt_writechild (&vtx->vt, "\033OA", 3);
                                   else
                                        for (i=0; i<evt.step; i++)
                                             vt_writechild (&vtx->vt, "\033[A", 3);
                              }
                              else {
                                   if (vtx->vt.mode & VTMODE_APP_CURSOR)
                                        for (i=0; i>evt.step; i--)
                                             vt_writechild (&vtx->vt, "\033OB", 3);
                                   else
                                        for (i=0; i>evt.step; i--)
                                             vt_writechild (&vtx->vt, "\033[B", 3);
                              }
                         }
                         else if (evt.modifiers & DIMM_SHIFT)
                              scroll -= evt.step;
                         else
                              scroll -= (term->rows - 1) * evt.step;
                         break;
                    case DWET_CLOSE:
                    case DWET_DESTROYED:
                         quit = 1;
                         break;
                    default:
                         ;
               }
          }

          lite_flush_window_events (term->lw);

          if (scroll)
               term_scroll (term, scroll);

          term_flush_flip (term);
     }

     if (!term->update_closing)
          pthread_cancel (term->update_thread);

     pthread_join (term->update_thread, NULL);

     pthread_mutex_unlock (&term->lock);
     pthread_mutex_destroy (&term->lock);

     vt_closepty (&vtx->vt);
     vtx_destroy (vtx);

     term->surface->Release (term->surface);
     term->bar_surface->Release (term->bar_surface);

     font->Release (font);

     lite_destroy_window (term->lw);

     free (term);

     lite_close();

     dfb->Release (dfb);

     return 0;
}

/***/

static void term_usage (void)
{
     printf ("\n  usage: dfbterm [--fontsize=X] [--size=COLSxROWS] [--position=X,Y]");
     printf ("\n                 [-c \"shell commands\"] [dfb-options]");
     printf ("\n         (defaults: --fontsize=%d --size=%dx%d)\n\n",
             TERM_DEFAULT_FONTSIZE, TERM_DEFAULT_WIDTH, TERM_DEFAULT_HEIGHT);
}

static void *vt_update_thread (void *arg)
{
     Term        *term = (Term*) arg;
     struct _vtx *vtx  = term->vtx;

     while (1) {
          int            count, update = 0;
          char           buffer[4096];
          fd_set         set;
          struct timeval tv;

          pthread_testcancel();

          FD_ZERO (&set);
          FD_SET (vtx->vt.childfd, &set);
          FD_SET (vtx->vt.msgfd, &set);
          tv.tv_sec  = 10;
          tv.tv_usec = 0;

          if (select (MAX (vtx->vt.childfd, vtx->vt.msgfd) + 1,
                      &set, NULL, NULL, &tv) < 0) {
               perror ("vt_update_thread: select");

               if (errno == EINTR)
                    continue;

               break;
          }

          if (FD_ISSET (vtx->vt.msgfd, &set)) {
               pthread_mutex_lock (&term->lock);
               term->update_closing = true;
               pthread_mutex_unlock (&term->lock);
               break;
          }

          while ( (count = read (vtx->vt.childfd, buffer, 4096)) > 0) {
               vt_cursor_state (term, 0);

               update = 1;
               vt_parse_vt (&vtx->vt, buffer, count);
          }

          pthread_mutex_lock (&term->lock);

          if (update) {
               vt_update (vtx, UPDATE_CHANGES);
               vt_cursor_state (term, 1);

               term_update_scrollbar (term);
               term_flush_flip (term);
          }

          pthread_mutex_unlock (&term->lock);
     }

     return NULL;
}

/***/

static int
unichar_to_utf8 (unsigned int  c,
                 char         *outbuf)
{
     int len = 0;
     int first;
     int i;

     if (c < 0x80) {
          first = 0;
          len = 1;
     }
     else if (c < 0x800) {
          first = 0xc0;
          len = 2;
     }
     else if (c < 0x10000) {
          first = 0xe0;
          len = 3;
     }
     else if (c < 0x200000) {
          first = 0xf0;
          len = 4;
     }
     else if (c < 0x4000000) {
          first = 0xf8;
          len = 5;
     }
     else {
          first = 0xfc;
          len = 6;
     }

     if (outbuf) {
          for (i = len - 1; i > 0; --i) {
               outbuf[i] = (c & 0x3f) | 0x80;
               c >>= 6;
          }
          outbuf[0] = c | first;
     }

     return len;
}

static void vt_draw_text (void           *user_data,
                          struct vt_line *line,
                          int             row,
                          int             col,
                          int             len,
                          int             attr)
{
     Term             *term = (Term*) user_data;
     IDirectFBSurface *surface = term->surface;
     DFBRegion         region;
     int               i, n, x, y, fore, back, fga, bga;
     char              text[len*6]; /* enough space for the worst UTF-8 case */

     if (term->minimized)
          return;

     fore = (attr & VTATTR_FORECOLOURM) >> VTATTR_FORECOLOURB;
     back = (attr & VTATTR_BACKCOLOURM) >> VTATTR_BACKCOLOURB;

     if (attr & VTATTR_BOLD && fore < 8)
          fore |= 8;

     /* for reverse, swap colors */
     if (attr & VTATTR_REVERSE) {
          i    = fore;
          fore = back;
          back = i;

          fga  = BGALPHA;
          bga  = 0xff;
     }
     else {
          fga  = 0xff;
          bga  = BGALPHA;
     }

     x = col * term->CW;
     y = row * term->CH;

     region.x1 = x;
     region.y1 = y;
     region.x2 = x + len * term->CW - 1;
     region.y2 = y + term->CH - 1;


     /* Wrapped lines with default background color? */
     if (line->wrap && back == 17 && !(attr & VTATTR_REVERSE)) {
          int wrap_start = (line->wrap == 1) ? 1 : 0;

          /* First line of a wrap group? */
          if (wrap_start) {
               /* Clear first row of pixels differently
                  to have a seperator between grouped lines. */
               surface->SetColor (surface, 0x40, 0x42, 0x47, bga + 0x0a);
               surface->FillRectangle (surface, x, y, term->CW*len, 1);
          }

          /* Special background. */
          surface->SetColor (surface, 0x20, 0x22, 0x27, bga + 0x0a);
          surface->FillRectangle (surface, x, y + wrap_start,
                                  term->CW*len, term->CH - wrap_start);
     }
     else {
          /* Normal background. */
          surface->SetColor (surface, default_red[back],
                             default_grn[back], default_blu[back], bga);
          surface->FillRectangle (surface, x, y, term->CW*len, term->CH);
     }


     surface->SetColor (surface, default_red[fore],
                        default_grn[fore], default_blu[fore], fga);

     for (i=0, n=0; i<len; i++) {
          unsigned int c;

          c = VT_ASCII(line->data[i+col]);

          if (c < 128)
               text[n++] = c;
          else
               n += unichar_to_utf8 (c, text + n);
     }

     surface->DrawString (surface, text, n, x, y/*+1*/, DSTF_TOPLEFT);

     if (!term->in_resize)
          term_add_flip (term, &region);
}

static void vt_scroll_area (void *user_data,
                            int   firstrow,
                            int   count,
                            int   offset,
                            int   fill)
{
     Term             *term = (Term*) user_data;
     IDirectFBSurface *surface = term->surface;
     DFBRegion         region;
     DFBRectangle      rect;

     if (term->minimized)
          return;

     rect.x = 0;
     rect.y = (firstrow+offset) * term->CH;
     rect.w = term->width;
     rect.h = count * term->CH;

     surface->Blit (surface, surface, &rect, 0, firstrow * term->CH);

     region.x1 = 0;
     region.x2 = term->width - 1;

     region.y1 = firstrow * term->CH;
     region.y2 = region.y1 + count * term->CH - 1;

     /* scrolling during resize won't happen anyways */
     if (!term->in_resize)
          term_add_flip (term, &region);
}

static int vt_cursor_state (void *user_data,
                            int   state)
{
     Term *term = (Term*) user_data;

     /* only call vt_draw_cursor if the state has changed */
     if (term->cursor_state ^ state) {
          vt_draw_cursor (term->vtx, state);
          term->cursor_state = state;
     }

     return term->cursor_state;
}

/***/

static void term_update_scrollbar (Term *term)
{
     IDirectFBSurface *surface = term->bar_surface;
     int               start, end, total;

     if (term->minimized)
          return;

     total = term->vtx->vt.scrollbacklines + term->rows;
     start = (term->vtx->vt.scrollbacklines +
              term->vtx->vt.scrollbackoffset) * term->height / total;
     end   = (term->vtx->vt.scrollbacklines +
              term->vtx->vt.scrollbackoffset + term->rows) * term->height / total;

     if (!term->in_resize && start == term->bar_start && end == term->bar_end)
          return;

     /* OPTIMIZE: calculate which parts must be updated */
     if (start) {
          surface->SetColor (surface, 0, 0, 0, BGALPHA);
          surface->FillRectangle (surface, 0, 0, 2, start);
     }

     if (end > start) {
          surface->SetColor (surface, 0x80, 0x80, 0x80, BGALPHA);
          surface->FillRectangle (surface, 0, start, 2, end - start);
     }

     if (end < term->height) {
          surface->SetColor (surface, 0, 0, 0, BGALPHA);
          surface->FillRectangle (surface, 0, end, 2, term->height - end);
     }

     if (!term->in_resize)
          surface->Flip (surface, NULL, 0);

     term->bar_start = start;
     term->bar_end   = end;
}

static void term_handle_button (Term *term, DFBWindowEvent *evt)
{
     struct _vtx *vtx = term->vtx;
     int button, down, qual = 0;
     long long diff;

     switch (evt->button) {
          case DIBI_LEFT:
               button = 1;
               break;
          case DIBI_MIDDLE:
               button = 2;
               break;
          case DIBI_RIGHT:
               button = 3;
               break;
          default:
               return;
     }

     down = (evt->type == DWET_BUTTONDOWN);

     if (term->modifiers & DIMM_SHIFT)
          qual |= 1;
     if (term->modifiers & DIMM_CONTROL)
          qual |= 4;
     if (term->modifiers & DIMM_ALT) /* meta? */
          qual |= 8;

     if (lite_default_window_theme_loaded()) {
          evt->x -= 5;
          evt->y -= 23;
     }
     evt->x /= term->CW;
     evt->y /= term->CH;

     if (vtx->selectiontype == VT_SELTYPE_NONE) {
          if (!(evt->modifiers & DIMM_SHIFT) &&
              vt_report_button (&vtx->vt, down, button, qual, evt->x, evt->y))
               return;
     }

     /* do our own handling here */
     evt->y += vtx->vt.scrollbackoffset;

     if (down) {
          if (vtx->selected) {
               vtx->selstartx = vtx->selendx;
               vtx->selstarty = vtx->selendy;
               vt_draw_selection (vtx);  /* un-render selection */
               vtx->selected = 0;
          }


          switch (evt->button) {
               case DIBI_LEFT:
                    term->lw->window->GrabPointer (term->lw->window);

                    diff = (evt->timestamp.tv_sec -
                            term->last_click.tv_sec) * (long long) 1000000 +
                           (evt->timestamp.tv_usec - term->last_click.tv_usec);

                    if ((evt->modifiers & DIMM_CONTROL) || diff < 400000)
                         vtx->selectiontype = VT_SELTYPE_WORD;
                    else
                         vtx->selectiontype = VT_SELTYPE_CHAR;

                    vtx->selectiontype |= VT_SELTYPE_BYSTART;

                    vtx->selstartx = evt->x;
                    vtx->selstarty = evt->y;
                    vtx->selendx = evt->x;
                    vtx->selendy = evt->y;

                    if (!vtx->selected) {
                         vtx->selstartxold = evt->x;
                         vtx->selstartyold = evt->y;
                         vtx->selendxold = evt->x;
                         vtx->selendyold = evt->y;
                         vtx->selected = 1;
                    }

                    vt_cursor_state (term, 0);
                    if ((evt->modifiers & DIMM_CONTROL) || diff < 400000) {
                         vtx->selectiontype |= VT_SELTYPE_MOVED;
                         vt_fix_selection (vtx);
                    }
                    vt_draw_selection (vtx);
                    vt_cursor_state (term, 1);

                    term->last_click = evt->timestamp;

                    break;
               case DIBI_MIDDLE:
               case DIBI_RIGHT : {
                         char         *mime_type;
                         void         *data;
                         unsigned int  size;

                         if (dfb->GetClipboardData (dfb, &mime_type, &data, &size))
                              break;

                         if (!strcmp (mime_type, "text/plain")) {
                              unsigned int  i;
                              char         *buf = data;

                              for (i=0; i<size; i++)
                                   if (buf[i] == '\n') buf[i] = '\r';

                              vt_writechild (&vtx->vt, data, size);

                              if (vtx->vt.scrollbackoffset) {
                                   vtx->vt.scrollbackoffset = 0;

                                   vt_update (vtx, UPDATE_SCROLLBACK);
                                   vt_cursor_state (term, 1);

                                   term_update_scrollbar (term);
                              }
                         }

                         free (mime_type);
                         free (data);

                         break;
                    }

               default:
                    break;
          }
     }
     else {
          if (vtx->selectiontype & VT_SELTYPE_BYSTART) {
               vtx->selendx = evt->x + 1;
               vtx->selendy = evt->y;
          }
          else {
               vtx->selstartx = evt->x;
               vtx->selstarty = evt->y;
          }

          if (vtx->selectiontype & VT_SELTYPE_MOVED) {
               int   len;
               char *buf;

               vt_fix_selection (vtx);
               vt_draw_selection (vtx);

               buf = vt_get_selection (vtx, 1, &len);

               dfb->SetClipboardData (dfb, "text/plain", buf, len, NULL);
          }

          vtx->selectiontype = VT_SELTYPE_NONE;

          term->lw->window->UngrabPointer (term->lw->window);
     }
}

static void term_handle_motion (Term *term, DFBWindowEvent *evt)
{
     struct _vtx *vtx = term->vtx;

     if (lite_default_window_theme_loaded()) {
          evt->x -= 5;
          evt->y -= 23;
     }
     evt->x /= term->CW;
     evt->y /= term->CH;

     if (vtx->selectiontype != VT_SELTYPE_NONE) {
          /* move end of selection, and draw it ... */
          if (vtx->selectiontype & VT_SELTYPE_BYSTART) {
               vtx->selendx = evt->x + 1;
               vtx->selendy = evt->y + vtx->vt.scrollbackoffset;
          }
          else {
               vtx->selstartx = evt->x;
               vtx->selstarty = evt->y + vtx->vt.scrollbackoffset;
          }

          vtx->selectiontype |= VT_SELTYPE_MOVED;

          vt_fix_selection (vtx);
          vt_draw_selection (vtx);
     }
}

static void term_scroll (Term *term, int scroll)
{
     struct _vtx *vtx = term->vtx;

     vtx->vt.scrollbackoffset += scroll;

     if (vtx->vt.scrollbackoffset > 0)
          vtx->vt.scrollbackoffset = 0;
     else if (vtx->vt.scrollbackoffset < -vtx->vt.scrollbacklines)
          vtx->vt.scrollbackoffset = -vtx->vt.scrollbacklines;

     vt_update (vtx, UPDATE_SCROLLBACK);
     vt_cursor_state (term, 1);

     term_update_scrollbar (term);
}

static void term_add_flip (Term *term, DFBRegion *region)
{
     if (term->flip_pending) {
          if (term->flip_region.x1 > region->x1)
               term->flip_region.x1 = region->x1;

          if (term->flip_region.y1 > region->y1)
               term->flip_region.y1 = region->y1;

          if (term->flip_region.x2 < region->x2)
               term->flip_region.x2 = region->x2;

          if (term->flip_region.y2 < region->y2)
               term->flip_region.y2 = region->y2;
     }
     else {
          term->flip_region  = *region;
          term->flip_pending = DFB_TRUE;
     }
}

static void term_flush_flip (Term *term)
{
     if (term->flip_pending) {
          IDirectFBSurface *surface = term->surface;

          surface->Flip( surface, &term->flip_region, 0 );

          term->flip_pending = DFB_FALSE;
     }
}

static void term_handle_key (Term *term, DFBWindowEvent *evt)
{
     struct _vtx *vtx = term->vtx;

     term->modifiers = evt->modifiers;

     if (evt->type == DWET_KEYUP)
          return;

     if (evt->modifiers == DIMM_CONTROL && evt->key_symbol == DIKS_ENTER) {
          IDirectFBWindow *window = term->lw->window;

          if (term->hot_key) {
               window->UngrabKey (window, DIKS_ENTER, DIMM_CONTROL);

               lite_restore_window (term->lw);

               window->RequestFocus (window);

               term->hot_key = DFB_FALSE;
          }
          else {
               DFBResult ret;

               ret = window->GrabKey (window, DIKS_ENTER, DIMM_CONTROL);
               if (ret) {
                    DirectFBError ("IDirectFBWindow::GrabKey() failed", ret);
                    return;
               }

               lite_minimize_window (term->lw);

               term->hot_key = DFB_TRUE;
          }

          return;
     }

     if (evt->modifiers  == DIMM_CONTROL &&
         evt->key_symbol >= DIKS_SMALL_A &&
         evt->key_symbol <= DIKS_SMALL_Z) {
          char c = evt->key_symbol - DIKS_SMALL_A + 1;
          vt_writechild (&vtx->vt, &c, 1);
     }
     else if ((evt->key_symbol >   9 && evt->key_symbol < 127) ||
              (evt->key_symbol > 127 && evt->key_symbol < 256)) {
          char c = evt->key_symbol & 0xff;

          if (evt->modifiers & DIMM_CONTROL) {
               switch (evt->key_symbol) {
                    case ' ':
                         vt_writechild (&vtx->vt, "\000", 1);
                         break;
                    case '3':
                    case '[':
                         vt_writechild (&vtx->vt, "\033", 1);
                         break;
                    case '4':
                    case '\\':
                         vt_writechild (&vtx->vt, "\034", 1);
                         break;
                    case '5':
                    case ']':
                         vt_writechild (&vtx->vt, "\035", 1);
                         break;
                    case '6':
                         vt_writechild (&vtx->vt, "\036", 1);
                         break;
                    case '7':
                    case '-':
                         vt_writechild (&vtx->vt, "\037", 1);
                         break;
                    default:
                         vt_writechild (&vtx->vt, &c, 1);
                         break;
               }
          }
          else
               vt_writechild (&vtx->vt, &c, 1);
     }
     else {
          switch (evt->key_symbol) {
               case DIKS_BACKSPACE:
                    vt_writechild (&vtx->vt, "\177", 1);
                    break;
               case DIKS_TAB:
                    if (evt->modifiers & DIMM_SHIFT)   /* back tab */
                         vt_writechild (&vtx->vt, "\033[Z", 3);
                    else
                         vt_writechild (&vtx->vt, "\t", 1);
                    break;
               case DIKS_DELETE:
                    vt_writechild (&vtx->vt, "\033[3~", 4);
                    break;
               case DIKS_INSERT:
                    vt_writechild (&vtx->vt, "\033[2~", 4);
                    break;
               case DIKS_CURSOR_LEFT:
                    if (vtx->vt.mode & VTMODE_APP_CURSOR)
                         vt_writechild (&vtx->vt, "\033OD", 3);
                    else
                         vt_writechild (&vtx->vt, "\033[D", 3);
                    break;
               case DIKS_CURSOR_RIGHT:
                    if (vtx->vt.mode & VTMODE_APP_CURSOR)
                         vt_writechild (&vtx->vt, "\033OC", 3);
                    else
                         vt_writechild (&vtx->vt, "\033[C", 3);
                    break;
               case DIKS_CURSOR_UP:
                    if (vtx->vt.mode & VTMODE_APP_CURSOR)
                         vt_writechild (&vtx->vt, "\033OA", 3);
                    else
                         vt_writechild (&vtx->vt, "\033[A", 3);
                    break;
               case DIKS_CURSOR_DOWN:
                    if (vtx->vt.mode & VTMODE_APP_CURSOR)
                         vt_writechild (&vtx->vt, "\033OB", 3);
                    else
                         vt_writechild (&vtx->vt, "\033[B", 3);
                    break;
               case DIKS_HOME:
                    vt_writechild (&vtx->vt, "\033OH", 3);
                    break;
               case DIKS_END:
                    vt_writechild (&vtx->vt, "\033OF", 3);
                    break;
               case DIKS_PAGE_UP:
                    vt_writechild (&vtx->vt, "\033[5~", 4);
                    break;
               case DIKS_PAGE_DOWN:
                    vt_writechild (&vtx->vt, "\033[6~", 4);
                    break;
               case DIKS_F1:
                    vt_writechild (&vtx->vt, "\033OP", 3);
                    break;
               case DIKS_F2:
                    vt_writechild (&vtx->vt, "\033OQ", 3);
                    break;
               case DIKS_F3:
                    vt_writechild (&vtx->vt, "\033OR", 3);
                    break;
               case DIKS_F4:
                    vt_writechild (&vtx->vt, "\033OS", 3);
                    break;
               case DIKS_F5 ... DIKS_F12:
                    {
                         char buf[6];

                         sprintf (buf, "\033[%d~", f5_f12_remap[evt->key_symbol - DIKS_F5]);
                         vt_writechild (&vtx->vt, buf, strlen(buf));
                    }
                    break;
               default:
                    return;
          }
     }

     if (vtx->selected) {
          vtx->selstartx = vtx->selendx;
          vtx->selstarty = vtx->selendy;
          vt_draw_selection (vtx);  /* un-render selection */
          vtx->selected = 0;
     }

     if (vtx->vt.scrollbackoffset) {
          vtx->vt.scrollbackoffset = 0;

          vt_update (vtx, UPDATE_SCROLLBACK);
          vt_cursor_state (term, 1);

          term_update_scrollbar (term);
     }
}

static int term_on_resize (LiteWindow *lw, int width, int height)
{
     DFBRectangle      rect;
     int               tw, th;
     int               rw, rh;
     Term             *term    = gterm;
     IDirectFBSurface *surface = lw->box.surface;

     if (width < 3 || height < 1) {
          term->minimized = DFB_TRUE;
          return 0;
     }
     else
          term->minimized = DFB_FALSE;

     tw = width - 2;
     th = height;

     rw = tw % term->CW;
     rh = th % term->CH;

     if (rw || rh) {
          int nw = tw - rw;
          int nh = th - rh;

          if (tw > term->width)
               nw += term->CW;

          if (th > term->height)
               nh += term->CH;

          lite_resize_window (lw, nw + 2, nh);

          return 0;
     }

     if (term->surface) {
          term->surface->Release (term->surface);
          term->surface = NULL;
     }

     if (term->bar_surface) {
          term->bar_surface->Release (term->bar_surface);
          term->bar_surface = NULL;
     }

     term->width  = tw;
     term->height = th;

     term->cols = term->width  / term->CW;
     term->rows = term->height / term->CH;

     /* Initialize sub area for terminal */
     rect.x = 0;
     rect.y = 0;
     rect.w = term->width;
     rect.h = term->height;

     surface->GetSubSurface (surface, &rect, &term->surface);

     //term->surface->Clear (term->surface, 0x00, 0x00, 0x00, BGALPHA);
     term->surface->SetFont (term->surface, term->font);

     /* Initialize sub area for scroll bar */
     rect.x = term->width;
     rect.w = 2;

     surface->GetSubSurface (surface, &rect, &term->bar_surface);

     term->in_resize = DFB_TRUE;

     vt_resize (&term->vtx->vt,
                term->cols, term->rows,
                term->width, term->height);

     vt_update_rect (term->vtx, 1, 0, 0, term->cols, term->rows);

     term_update_scrollbar (term);

     vt_cursor_state (term, 1);

     term->in_resize    = DFB_FALSE;
     term->flip_pending = DFB_FALSE;

     return 0;
}

