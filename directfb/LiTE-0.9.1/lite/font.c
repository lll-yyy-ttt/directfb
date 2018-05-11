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

#include <direct/mem.h>

#include "util.h"
#include "lite_internal.h"
#include "lite_config.h"

#include "font.h"

D_DEBUG_DOMAIN(LiteFontDomain, "LiTE/Font", "LiteFont");

struct _LiteFont {
     int            refs;
     char          *file;
     int            size;
     IDirectFBFont *font;
     DFBFontAttributes attr;

     LiteFont      *next;
     LiteFont      *prev;
};

static char* font_styles_global[4] = {
     "", "bd", "i", "bi"
};

static LiteFont        *fonts       = NULL;
static pthread_mutex_t  fonts_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Returns existing an font entry from cache after increasing
 * it's reference counter or tries to create a new one otherwise.
 */
static LiteFont *cache_get_entry(const char *name,
                                 int         size,
                                 DFBFontAttributes attr);

static LiteFont *cache_get_entry_from_file(const char *file,
                                            int         size,
                                            DFBFontAttributes attr);

/*
 * Decreases the reference counter of a cache entry
 * and removes/destroys it if the counter is zero.
 */
static void cache_release_entry(LiteFont *entry);


DFBResult 
lite_get_font(const char       *spec, 
              LiteFontStyle     style, 
              int               size, 
              DFBFontAttributes attr, 
              LiteFont        **font)
{
     int       name_len;
     char     *name = NULL;
     char     *c = NULL;
     DFBResult result = DFB_OK;

     LITE_NULL_PARAMETER_CHECK(spec);
     LITE_NULL_PARAMETER_CHECK(font);

     /* translate predefined specs, or use passed in spec as a font
        name in case it does not match a spec */
     if (!strcasecmp(spec, "default"))
          spec = DEFAULT_FONT_SYSTEM;
     else if (!strcasecmp(spec, "monospaced"))
          spec = DEFAULT_FONT_MONOSPACED;
     else if (!strcasecmp(spec, "serif"))
          spec = DEFAULT_FONT_SERIF;
     else if (!strcasecmp(spec, "proportional"))
          spec = DEFAULT_FONT_PROPORTIONAL;
     else if (!strcasecmp(spec, "sansserif"))
          spec = DEFAULT_FONT_SANS_SERIF;
     else if (!strcasecmp(spec, "hangul"))
          spec = DEFAULT_FONT_SYSTEM_HANGUL;

     /* append characters depending on font style */
     name_len = strlen(spec) + 3;
     name = alloca(name_len);

     snprintf(name, name_len, "%s%s", spec, font_styles_global[style&3]);

     /* replace spaces by underscores */
     while ((c = strchr(name, ' ')) != NULL)
          *c = '_';

     /* get the font from the cache,
        if it does not exist yet it will be loaded */
     *font = cache_get_entry(name, size, attr);

     /* if font loading failed try to get our default font,
        but make sure we aren't already trying that */
     if (*font == NULL) {
          if (strcmp(spec, DEFAULT_FONT_SYSTEM)) {
               char fallback[10];

               snprintf(fallback, 10, DEFAULT_FONT_SYSTEM "%s", 
                        font_styles_global[style&3]);
               *font = cache_get_entry(fallback, size, attr);

               if (*font == NULL) {
                    D_ERROR( "LiTE/Font: Could not load fallback "
                             "font '%s' for '"LITEFONTDIR"/%s'!\n", 
                             fallback, name);

                    result = DFB_FILENOTFOUND;
               }
          }
          else {
               D_ERROR( "LiTE/Font: Could not load default font '"LITEFONTDIR"/%s'!\n", name);

               result = DFB_FILENOTFOUND;
          }
     }

     return result;
}

DFBResult 
lite_get_font_from_file(const char       *file, 
                        int               size, 
                        DFBFontAttributes attr, 
                        LiteFont        **font)
{
     DFBResult result = DFB_OK;

     LITE_NULL_PARAMETER_CHECK(file);
     LITE_NULL_PARAMETER_CHECK(font);

    /* get the font from the cache,
        if it does not exist yet it will be loaded */
     *font = cache_get_entry_from_file(file, size, attr);

     /* if font loading failed try to get our default font,
        but make sure we aren't already trying that */
     if (*font == NULL) {
          *font = cache_get_entry(DEFAULT_FONT_SYSTEM, size, attr);

          if (*font == NULL) {
               D_DEBUG_AT(LiteFontDomain, "Could not load fallback "
                              "font '%s' for '%s'!\n", DEFAULT_FONT_SYSTEM, file);
               result = DFB_FILENOTFOUND;
          }
     }

     return result;
}

DFBResult 
lite_ref_font(LiteFont *font)
{
     DFBResult      result = DFB_OK;

     LITE_NULL_PARAMETER_CHECK(font);

     pthread_mutex_lock(&fonts_mutex);

     if (font->refs < 1) {
          D_DEBUG_AT(LiteFontDomain, "Cannot reference unloaded font");
          result = DFB_INVARG;
     }
     else
          font->refs++;

     D_DEBUG_AT(LiteFontDomain, "lite_ref_font: font %p (interface %p) has refs == %d\n",
                font, font->font, font->refs);
    
     pthread_mutex_unlock(&fonts_mutex);

     return result;
}

DFBResult
lite_get_font_filename(LiteFont *font, const char **file)
{
    *file = font->file;
    return DFB_OK;
}

DFBResult 
lite_release_font(LiteFont *font)
{
     LITE_NULL_PARAMETER_CHECK(font);

     cache_release_entry(font);

     return DFB_OK;
}

DFBResult 
lite_font(LiteFont *font, IDirectFBFont **font_interface)
{
     LITE_NULL_PARAMETER_CHECK(font);
     LITE_NULL_PARAMETER_CHECK(font_interface);
     LITE_NULL_PARAMETER_CHECK(font->font);

     *font_interface = font->font;

     return DFB_OK;
}

DFBResult 
lite_set_active_font(LiteBox *box, LiteFont *font)
{
     LITE_NULL_PARAMETER_CHECK(box);
     LITE_NULL_PARAMETER_CHECK(font);

     if (box->surface == NULL) {
          D_DEBUG_AT(LiteFontDomain, "NULL surface");
          return DFB_FAILURE;
     }

     if (font->font == NULL) {
          D_DEBUG_AT(LiteFontDomain, "Font missing IDirectFBFont interface");
          return DFB_FAILURE;
     }

     /* now set the font on the surface */
     return box->surface->SetFont(box->surface, font->font);
}

DFBResult 
lite_get_active_font(LiteBox *box, LiteFont **font)
{
     IDirectFBFont *font_interface = NULL;
     LiteFont      *entry = NULL;
     DFBResult      result = DFB_OK;

     LITE_NULL_PARAMETER_CHECK(box);
     LITE_NULL_PARAMETER_CHECK(font);

     if (box->surface == NULL) {
          D_DEBUG_AT(LiteFontDomain, "NULL surface");
          return DFB_FAILURE;
     }

     result = box->surface->GetFont(box->surface, &font_interface);
     
     /* verify that font is in the cache */
     pthread_mutex_lock(&fonts_mutex);
     *font = NULL;
     for (entry = fonts; entry; entry = entry->next) {
          if (entry->font == font_interface) {
               *font = entry;
          }
     }
     pthread_mutex_unlock(&fonts_mutex);

     font_interface->Release(font_interface);

     if (*font == NULL) {
          D_DEBUG_AT(LiteFontDomain, "no font set");
          result = DFB_FAILURE;
     }

     return result;
}

/* internals */

static LiteFont* 
cache_get_entry_from_file(const char       *file, 
                          int               size, 
                          DFBFontAttributes attr)
{
     DFBResult           ret;
     LiteFont           *entry = NULL;
     IDirectFBFont      *font = NULL;
     DFBFontDescription  desc;

     D_ASSERT(file != NULL);

     /* lock cache */
     pthread_mutex_lock(&fonts_mutex);

     /* lookup existing entry if any */
     for (entry = fonts; entry; entry = entry->next) {
          if (!strcmp(file, entry->file)  &&  size == entry->size) {
               entry->refs++;
               D_DEBUG_AT(LiteFontDomain, 
                         "cache_get_entry_from_file: + existing cache entry found "
                         "for '%s' with size %d and refs %d!\n", file, size, entry->refs);
               pthread_mutex_unlock(&fonts_mutex);
               return entry;
          }
     }

     D_DEBUG_AT(LiteFontDomain, 
                "cache_get_entry_from_file: loading `%s' (%d)...\n", file, size);

     /* try to load the font */
     desc.flags  = DFDESC_HEIGHT;
     desc.height = size;

     /* send DFBFontAttributes on to FreeType2 */
     desc.flags  |= DFDESC_ATTRIBUTES;
     desc.attributes = attr;

     ret = lite_dfb->CreateFont(lite_dfb, file, &desc, &font);
     if (ret) {
          D_DEBUG_AT(LiteFontDomain, "font loading failed, using fallback!\n");
          pthread_mutex_unlock(&fonts_mutex);
          return NULL;
     }

     D_DEBUG_AT(LiteFontDomain, 
                "cache_get_entry_from_file: DFB->CreateFont() returned interface %p\n", font);

     /* create a new entry for it */
     entry = D_CALLOC(1, sizeof(LiteFont));

     entry->refs = 1;
     entry->file = D_STRDUP(file);
     entry->size = size;
     entry->font = font;

     /* insert into cache */
     if (fonts) {
          fonts->prev = entry;
          entry->next = fonts;
     }
     fonts = entry;

     /* unlock cache */
     pthread_mutex_unlock(&fonts_mutex);

     D_DEBUG_AT(LiteFontDomain, "cache_get_entry_from_file: * new cache entry "
                    "added for '%s' with size %d\n", file, size);

     return entry;
}

static LiteFont*
cache_get_entry(const char       *name, 
                int               size, 
                DFBFontAttributes attr)
{
     int  len = strlen(LITEFONTDIR) + 1 + strlen(name) + 6 + 1;
     char filename[len];
     LiteFont *font;

     D_ASSERT(name != NULL);

     /* first, try to load a DGIFF-format font */
     snprintf(filename, len, LITEFONTDIR"/%s.dgiff", name);
     font = cache_get_entry_from_file(filename, size, attr);

     if (font == NULL) {
         /* fallback to using a TTF formatted font */
         snprintf(filename, len, LITEFONTDIR"/%s.ttf", name);
         font = cache_get_entry_from_file(filename, size, attr);
     }

     return font;
}

static void 
cache_release_entry(LiteFont *entry)
{
     D_ASSERT(entry != NULL);

     /* lock cache */
     pthread_mutex_lock(&fonts_mutex);

     /* decrease reference counter and return if it's not zero */
     if (--entry->refs) {
          D_DEBUG_AT(LiteFontDomain, "cache_release_entry: "
                     "cache entry '%s' (%d) has refs %d\n", entry->file, entry->size, entry->refs);
          pthread_mutex_unlock(&fonts_mutex);
          return;
     }

     D_DEBUG_AT(LiteFontDomain, "cache_release_entry: - destroying "
                    "cache entry '%s' (%d)\n", entry->file, entry->size);

     /* remove entry from cache */
     if (entry->next)
          entry->next->prev = entry->prev;

     if (entry->prev)
          entry->prev->next = entry->next;
     else
          fonts = entry->next;

     /* unlock as early as possible */
     pthread_mutex_unlock(&fonts_mutex);

     /* free font resources */
     D_DEBUG_AT(LiteFontDomain, "DFB->Release() on interface %p\n", entry->font);
     entry->font->Release(entry->font);
     D_FREE(entry->file);
     D_FREE(entry);
}

void 
prvlite_font_init(void)
{
     fonts = NULL;
}
