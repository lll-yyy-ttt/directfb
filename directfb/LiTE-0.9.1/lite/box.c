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

#include <pthread.h>
#include <directfb.h>

#include <direct/debug.h>
#include <direct/mem.h>
#include <direct/messages.h>

#include <directfb_util.h>
#include <directfb_version.h>

#include "util.h"

#include "box.h"
#include "window.h"

D_DEBUG_DOMAIN(LiteBoxDomain, "LiTE/Box", "LiteBox");

static void draw_box_and_children(LiteBox *box, const DFBRegion *region,
                                   DFBBoolean clear);

static void defocus_me_or_children (LiteWindow *top, LiteBox *box);
static void deenter_me_or_children (LiteWindow *top, LiteBox *box);
static void undrag_me_or_children (LiteWindow *top, LiteBox *box);

DFBResult
lite_new_box(LiteBox **box,
             LiteBox  *parent,
             int       x,
             int       y,
             int       width,
             int       height)
{
     LiteBox   *bx = NULL;
     DFBResult  res;

     LITE_NULL_PARAMETER_CHECK(parent);

     bx = D_CALLOC(1, sizeof(LiteBox));

     bx->parent = parent;

     bx->rect.x = x;
     bx->rect.y = y;
     bx->rect.w = width;
     bx->rect.h = height;

     res = lite_init_box(bx);
     if (res != DFB_OK) {
          D_FREE(bx);
          return res;
     }

     *box = bx;

     D_DEBUG_AT(LiteBoxDomain, "Created new box object: %p\n", bx);

     return DFB_OK;
}

DFBResult
lite_draw_box(LiteBox         *box,
              const DFBRegion *region,
              DFBBoolean       flip)
{
     LITE_NULL_PARAMETER_CHECK(box);

     if (box->is_visible == 0)
          return DFB_OK;
         
     if (box->rect.h == 0 || box->rect.w == 0)
          return DFB_OK;

     DFBRegion full_reg = { 0, 0, box->rect.w - 1,  box->rect.h - 1 };
     if (region == NULL) {
          region = &full_reg;
     }

     D_DEBUG_AT(LiteBoxDomain,
                "Update box: %p at %d,%d %dx%d\n", box, region->x1, region->y1,
                region->x2 - region->x1 + 1, region->y2 - region->y1 + 1);

/*     fprintf( stderr, "==========================>   DRAW   %4d,%4d - %4dx%4d   <- %d\n",
                DFB_RECTANGLE_VALS_FROM_REGION( region ),
                (box->type == LITE_TYPE_WINDOW) ? LITE_WINDOW(box)->id : -1 );
*/
      /* FIXME: Also suppress non-toplevel drawing? */
     if (box->type == LITE_TYPE_WINDOW && (LITE_WINDOW(box)->flags & LITE_WINDOW_PENDING_RESIZE)) {
          D_DEBUG_AT( LiteBoxDomain, "  -> resize is pending, not drawing...\n" );
          return DFB_OK;
     }

     draw_box_and_children(box, region, DFB_TRUE);

     if (flip)
         box->surface->Flip(box->surface, region, getenv("LITE_WINDOW_DOUBLEBUFFER") ? DSFLIP_BLIT: 0);

      if (box->type == LITE_TYPE_WINDOW) {
         LiteWindow* window = (LiteWindow*)box;
         window->flags |= LITE_WINDOW_DRAWN;
         window->window->SetOpacity(window->window, window->opacity);
      }

      return DFB_OK;
}

DFBResult
lite_update_box(LiteBox *box, const DFBRegion *region)
{
     LITE_NULL_PARAMETER_CHECK(box);

     DFBRegion reg;

     if (region == NULL) {
          reg.x1 = reg.y1 = 0;
          reg.x2 = box->rect.w - 1;
          reg.y2 = box->rect.h - 1;
     }
     else {
          reg = *region;
     }
     
     while (1) {
          if (reg.x2 < reg.x1 || reg.y2 < reg.y1)
               return DFB_OK;

          if (reg.x1 > box->rect.w - 1 || reg.x2 < 0 ||
              reg.y1 > box->rect.h - 1 || reg.y2 < 0)
               return DFB_OK;

          /* stop if we hit an invisible box */
          if (box->is_visible == 0)
               return DFB_OK;

          if (box->parent) {
               dfb_region_translate(&reg, box->rect.x, box->rect.y);
               box = box->parent;
          }
          else {
               if (box->type == LITE_TYPE_WINDOW)
                    lite_update_window(LITE_WINDOW(box), &reg);
               else
                    D_DEBUG_AT(LiteBoxDomain,
                               "Can't update a box without a top level parent!\n");
               
               return DFB_OK;
          }
     }
}

DFBResult
lite_destroy_box_contents(LiteBox *box)
{
     int i;

     LITE_NULL_PARAMETER_CHECK(box);

     /* remove the child from the parent child array, unless
        it's a window (no parent), or recursive */
     if (box->parent != NULL && box->type != LITE_TYPE_WINDOW) {
          lite_remove_child(box->parent, box);
     }

     /* destroy children */
     for (i=0; i<box->n_children; i++) {
          box->children[i]->parent = NULL;
          D_DEBUG_AT(LiteBoxDomain, "Destroy child box: %p\n", box->children[i] );
          box->children[i]->Destroy(box->children[i]);
     }

     /* free surface and used memory */
     box->surface->Release(box->surface);

     if (box->children)
        D_FREE(box->children);

     box->children = NULL;
     box->n_children = 0;

     return DFB_OK;
}

DFBResult
lite_destroy_box(LiteBox *box)
{
     LITE_NULL_PARAMETER_CHECK(box);

     D_DEBUG_AT(LiteBoxDomain, "Destroy box: %p\n", box );

     DFBResult ret = lite_destroy_box_contents(box);
     D_FREE(box);

     return ret;
}


DFBResult
lite_init_box(LiteBox *box)
{
     DFBResult ret;

     LITE_NULL_PARAMETER_CHECK(box);

     if (box->rect.w < 0 || box->rect.h < 0) {
          D_DEBUG_AT(LiteBoxDomain,
                     "lite_init_box: ERROR: negative box width (%d) or height (%d)\n",
                     box->rect.w, box->rect.h);
          return DFB_INVAREA;
     }
     
     if( box->parent == NULL )
     {
          /* no parent specified, complete registration later with lite_init_box_at() */
          return DFB_OK;
     }

     /* Get sub surface */
     ret = box->parent->surface->GetSubSurface(box->parent->surface,
                                               &box->rect, &box->surface);
     if (ret) {
          DirectFBError("lite_init_box: Surface->GetSubSurface", ret);
          return ret;
     }
     
     if( !box->Destroy )
          box->Destroy = lite_destroy_box;

     lite_add_child(box->parent, box);

     box->handle_keys = 1;      /* by default all liteboxes handle keys */
     box->is_visible  = 1;      /* by default all liteboxes are visible */
     box->catches_all_events = 0; /* by default all liteboxes allow events to be handled by their children */
     box->is_active = 1;        /* by default all liteboxes are active */
     
     if( !box->type )
          box->type = LITE_TYPE_BOX;

     return ret;
}

DFBResult
lite_init_box_at(LiteBox            *box,
                 LiteBox            *parent,
                 const DFBRectangle *rect)
{
     LITE_NULL_PARAMETER_CHECK(box);
     LITE_NULL_PARAMETER_CHECK(rect);

     box->parent = parent;
     box->rect   = *rect;

     return lite_init_box( box );
}

DFBResult
lite_reinit_box_and_children(LiteBox *box)
{
     int i;
     DFBResult ret = DFB_OK;

     LITE_NULL_PARAMETER_CHECK(box);

     /* Get new sub surface */
     if (box->parent) {
#if DIRECTFB_MAJOR_VERSION >  1 || \
   (DIRECTFB_MAJOR_VERSION == 1 && DIRECTFB_MINOR_VERSION > 2) || \
   (DIRECTFB_MAJOR_VERSION == 1 && DIRECTFB_MINOR_VERSION == 2 && DIRECTFB_MICRO_VERSION >= 1)
          ret = box->surface->MakeSubSurface(box->surface, box->parent->surface, &box->rect);
#else
          IDirectFBSurface *surface;
          IDirectFBFont *font;

          ret = box->parent->surface->GetSubSurface(box->parent->surface, &box->rect, &surface);
          if (ret) {
               DirectFBError("lite_reinit_box: Surface->GetSubSurface", ret);
               return ret;
          }

          /* transfer the font to the new sub-surface before releasing
             the old sub-surface */
          ret = box->surface->GetFont(box->surface, &font);
          if (font) {
               ret = surface->SetFont(surface, font);
               ret = font->Release(font);
          }

          ret = box->surface->Release(box->surface);
          box->surface = surface;
#endif
     }

     /* reinit children */
     for (i=0; i<box->n_children; i++)
          ret = lite_reinit_box_and_children(box->children[i]);

    return ret;
}

DFBResult
lite_clear_box(LiteBox *box, const DFBRegion *region)
{
     LITE_NULL_PARAMETER_CHECK(box);

     if (box->parent) {
          DFBRegion reg;

          if (region) {
               reg = *region;
               dfb_region_translate(&reg, box->rect.x, box->rect.y);
          }
          else {
               if (box->rect.h == 0 || box->rect.w == 0)
                    return DFB_OK;
               dfb_region_from_rectangle(&reg, &box->rect);
          }

          if (box->parent->Draw)
               box->parent->Draw(box->parent, &reg, DFB_TRUE);
          else
               lite_clear_box(box->parent, &reg);
     }
     else {
          D_DEBUG_AT(LiteBoxDomain,
               "lite_clear_box: No parent present in the LiteBox\n");
     }

     return DFB_OK;
}

DFBResult
lite_add_child(LiteBox *parent, LiteBox *child)
{
     LiteWindow *window = NULL;

     LITE_NULL_PARAMETER_CHECK(parent);
     LITE_NULL_PARAMETER_CHECK(child);

     /* update the child array */
     parent->n_children++;

     parent->children = D_REALLOC(parent->children,
                              sizeof(LiteBox*) * parent->n_children);

     parent->children[parent->n_children-1] = child;

     /* get a possible window that the box is included into */
     window = lite_find_my_window(parent);

     /* trigger OnBoxAdded callback if the box will be added */
     if (window != NULL) {
        if (window->OnBoxAdded) {
            window->OnBoxAdded(window, child);
        }
     }

     return DFB_OK;
}

DFBResult
lite_remove_child(LiteBox *parent, LiteBox *child)
{
     int i;
     LiteWindow *window = NULL;

     LITE_NULL_PARAMETER_CHECK(parent);
     LITE_NULL_PARAMETER_CHECK(child);

     /* get a possible window that the box is included into */
     window = lite_find_my_window(child);

     if (window != NULL) {
        /* triggerOnBoxToBeRemoved in case a box is removed */
        if (window->OnBoxToBeRemoved) {
            window->OnBoxToBeRemoved(window, child);
        }

        defocus_me_or_children(window, child);
        deenter_me_or_children(window, child);
        undrag_me_or_children(window, child);
     }

     /* remove the child from the parent array */
     for (i = 0; i < parent->n_children; i++) {
          if (parent->children[i] == child)
               break;
     }

     /* update the child array */
     if (i == parent->n_children) {
          D_DEBUG_AT(LiteBoxDomain,
          "Could not find the child in parent's child directory for removal.\n");

          return DFB_FAILURE;
     }

     /* force an update for the area occupied by the child */
     lite_update_box(child, NULL);

     parent->n_children--;
     for (; i < parent->n_children; i++)
          parent->children[i] = parent->children[i+1];

     parent->children = D_REALLOC(parent->children, 
                         sizeof(LiteBox*) * parent->n_children);

     return DFB_OK;
}
    
DFBResult
lite_set_box_visible(LiteBox *box,
                     bool     visible)
{
     if (box->is_visible == visible)
          return DFB_OK;
          
     if (visible) {
          box->is_visible = true;
          return lite_update_box( box, NULL );
     }
          
     lite_update_box( box, NULL );
     
     /* make invisible */     
     box->is_visible = false;

     return DFB_OK;     
}

/* internals */

static void
draw_box_and_children(LiteBox *box, const DFBRegion *region, DFBBoolean clear)
{
     int i;

     D_ASSERT(box != NULL);

     if (box->is_visible == 0)
        return;

     if (region->x2 < region->x1 || region->y2 < region->y1)
          return;

     if (region->x1 > box->rect.w - 1 || region->x2 < 0 ||
         region->y1 > box->rect.h - 1 || region->y2 < 0)
          return;

     D_DEBUG_AT(LiteBoxDomain,"Draw box:   %p at %d,%d %dx%d\n",
              box, box->rect.x, box->rect.y, box->rect.w, box->rect.h);

     box->surface->SetClip( box->surface, region );

     if (box->background)
          box->surface->Clear( box->surface,
                               box->background->r, box->background->g, box->background->b, box->background->a );

     /* draw box */
     if (box->Draw)
          box->Draw(box, region, clear);

     /* draw children */
     for (i=0; i<box->n_children; i++) {
          DFBRegion  trans = *region;
          LiteBox   *child = box->children[i];
          dfb_region_translate(&trans, -child->rect.x, -child->rect.y);
          draw_box_and_children(child, &trans, DFB_FALSE);
     }
     
     /* box drawing after children have been drawn */
     if (box->DrawAfter)
          box->DrawAfter(box, region);
}

static void
defocus_me_or_children (LiteWindow *top, LiteBox *box)
{
     LiteBox* traverse = NULL;

     D_ASSERT(box != NULL);

     /* traverse from the focused box to see if box is its ancestor */
     if (NULL != top) {
          traverse = top->focused_box;
          while (traverse) {
               if (traverse == box) {
                    /* move focus to the window */
                    top->focused_box = LITE_BOX(top);
                    break;
               }
               traverse = traverse->parent;
          }
     }
}

static void
deenter_me_or_children (LiteWindow *top, LiteBox *box)
{
     LiteBox* traverse = NULL;

     D_ASSERT(box != NULL);

     /* traverse from the entered box to see if box is its ancestor */
     if (NULL != top) {
          traverse = top->entered_box;
          while (traverse) {
               if (traverse == box) {
                    top->entered_box = NULL;
                    break;
               }
               traverse = traverse->parent;
          }
     }
}

static void
undrag_me_or_children (LiteWindow *top, LiteBox *box)
{
     LiteBox* traverse = NULL;

     D_ASSERT(box != NULL);

     /* traverse from the drag_box to see if box is its ancestor */
     if (NULL != top) {
          traverse = top->drag_box;
          while (traverse) {
               if (traverse == box) {
                    lite_release_window_drag_box(top);
                    break;
               }
               traverse = traverse->parent;
          }
     }
}
