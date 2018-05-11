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

#include <lite/font.h>
#include <lite/util.h>

#include "label.h"

D_DEBUG_DOMAIN(LiteLabelDomain, "LiTE/Label", "LiteLabel");

LiteLabelTheme *liteDefaultLabelTheme = NULL;

struct _LiteLabel {
     LiteBox             box;
     LiteLabelTheme     *theme;

     LiteFont           *font;
     char               *text;
     DFBColor            text_color;

     LiteLabelAlignment  alignment;
};

static DFBResult draw_label(LiteBox *box, const DFBRegion *region, 
                              DFBBoolean clear);
static DFBResult destroy_label(LiteBox *box);

DFBResult 
lite_new_label(LiteBox        *parent,
               DFBRectangle   *rect,
               LiteLabelTheme *theme,
               int             font_size,
               LiteLabel     **ret_label)
{
     LiteLabel *label = NULL;
     LiteFont  *font = NULL;
     int        height;
     DFBResult  res;
     IDirectFBFont *font_interface = NULL;

     res = lite_get_font("default", LITE_FONT_PLAIN, font_size, 
                         DEFAULT_FONT_ATTRIBUTE, &font);
     if (res != DFB_OK)
          return res;

     res = lite_font(font, &font_interface);
     if (res != DFB_OK)
          return res;
          
     font_interface->GetHeight(font_interface, &height);

     label = D_CALLOC(1, sizeof(LiteLabel));

     label->box.parent = parent;
     label->theme = theme;
     label->box.rect = *rect;

     /* default black font color */
     label->text_color.r = 0x00;
     label->text_color.g = 0x00;
     label->text_color.b = 0x00;
     label->text_color.a = 0xff;

     res = lite_init_box(LITE_BOX(label));
     if (res != DFB_OK) { 
          lite_release_font(font);
          D_FREE(label);
          return res;
     }

     label->box.type    = LITE_TYPE_LABEL;
     label->box.Draw    = draw_label;
     label->box.Destroy = destroy_label;

     label->font = font;
     label->text = D_STRDUP("");

     *ret_label = label;

     D_DEBUG_AT(LiteLabelDomain, "Created new label object: %p\n", label);

     return DFB_OK;
}

DFBResult 
lite_set_label_text(LiteLabel  *label, const char *text)
{
     LITE_NULL_PARAMETER_CHECK(label);
     LITE_NULL_PARAMETER_CHECK(text);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(label), LITE_TYPE_LABEL);

     D_DEBUG_AT(LiteLabelDomain, "Set label: %p with text: %s\n", label, text);

     if (!strcmp(label->text, text))
          return DFB_OK;

     D_FREE(label->text);

     label->text = D_STRDUP(text);

     return lite_update_box(LITE_BOX(label), NULL);
}

DFBResult 
lite_set_label_alignment(LiteLabel *label, LiteLabelAlignment alignment)
{
     LITE_NULL_PARAMETER_CHECK(label);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(label), LITE_TYPE_LABEL);


     D_DEBUG_AT(LiteLabelDomain, "Set label alignment: %d for label: %p\n", alignment, label);

     if (label->alignment == alignment)
          return DFB_OK;

     label->alignment = alignment;

     return lite_update_box(LITE_BOX(label), NULL);
}

DFBResult 
lite_set_label_font(LiteLabel        *label,
                    const char       *spec,
                    LiteFontStyle     style,
                    int               size,
                    DFBFontAttributes attr)
{
     DFBResult      result;
     LiteFont      *font = NULL;
     IDirectFBFont *font_interface = NULL;

     LITE_NULL_PARAMETER_CHECK(label);
     LITE_NULL_PARAMETER_CHECK(spec);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(label), LITE_TYPE_LABEL);

     D_DEBUG_AT(LiteLabelDomain, "Set label font spec: %s with style: %d and size: %d for label: %p\n",
                    spec, style, size, label);

     result = lite_get_font(spec, style, size, attr, &font);
     if (result != DFB_OK)
          return result;

     result = lite_font(font, &font_interface);
     if (result != DFB_OK)
          return result;

     lite_release_font(label->font);

     label->font = font;

     font_interface->GetHeight(font_interface, &label->box.rect.h);

     return lite_update_box(LITE_BOX(label), NULL);
}

DFBResult 
lite_set_label_color(LiteLabel *label, DFBColor *color)
{
     DFBResult ret = DFB_OK; 

     LITE_NULL_PARAMETER_CHECK(label);
     LITE_NULL_PARAMETER_CHECK(color);
     LITE_BOX_TYPE_PARAMETER_CHECK(LITE_BOX(label), LITE_TYPE_LABEL);
     
     if (!DFB_COLOR_EQUAL(label->text_color, *color)) {
               label->text_color = *color; 
               ret = lite_update_box(LITE_BOX(label), NULL);
     }
     else
          ret = DFB_OK;

     return ret;
}


/* internals */

static DFBResult 
destroy_label(LiteBox *box)
{
     LiteLabel *label = LITE_LABEL(box);

     D_DEBUG_AT(LiteLabelDomain, "Destroy label: %p\n", label);

     D_ASSERT(box != NULL);

     D_FREE(label->text);

     lite_release_font(label->font);

     return lite_destroy_box(box);
}

static DFBResult 
draw_label(LiteBox         *box, 
           const DFBRegion *region, 
           DFBBoolean       clear)
{
     DFBResult            result;
     int                  x = 0;
     DFBSurfaceTextFlags  flags = DSTF_TOP;
     IDirectFBSurface    *surface = box->surface;
     LiteLabel           *label = LITE_LABEL(box);
     IDirectFBFont       *font_interface = NULL;

     D_ASSERT(box != NULL);

     result = lite_font(label->font, &font_interface);
     if (result != DFB_OK)
          return result;

     surface->SetFont(surface, font_interface);
     surface->SetClip(surface, region);

     /* Fill the background */
     if (clear)
          lite_clear_box(box, region);

     switch (label->alignment) {
          case LITE_LABEL_LEFT:
               flags |= DSTF_LEFT;
               break;

          case LITE_LABEL_RIGHT:
               x = box->rect.w - 1;
               flags |= DSTF_RIGHT;
               break;

          case LITE_LABEL_CENTER:
               x = box->rect.w / 2;
               flags |= DSTF_CENTER;
               break;
     }

     /* Draw the text */
     surface->SetColor(surface, 
                         label->text_color.r, label->text_color.g,
                         label->text_color.b, label->text_color.a);
     surface->DrawString(surface, label->text, -1, x, 0, flags);

     return DFB_OK;
}

