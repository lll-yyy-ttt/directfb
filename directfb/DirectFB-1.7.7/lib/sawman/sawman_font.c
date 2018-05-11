/*
   (c) Copyright 2012-2013  DirectFB integrated media GmbH
   (c) Copyright 2001-2013  The world wide DirectFB Open Source Community (directfb.org)
   (c) Copyright 2000-2004  Convergence (integrated media) GmbH

   All rights reserved.

   Written by Denis Oliver Kropp <dok@directfb.org>,
              Andreas Shimokawa <andi@directfb.org>,
              Marek Pikarski <mass@directfb.org>,
              Sven Neumann <neo@directfb.org>,
              Ville Syrjälä <syrjala@sci.fi> and
              Claudio Ciccani <klan@users.sf.net>.

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

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <math.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <directfb.h>

#include <core/CoreDFB.h>

#include <core/fonts.h>
#include <core/gfxcard.h>
#include <core/surface.h>
#include <core/surface_buffer.h>

#include <core/CoreSurface.h>

#include <gfx/convert.h>

#include <media/idirectfbfont.h>

#include <direct/hash.h>

#include <direct/mem.h>
#include <direct/memcpy.h>
#include <direct/messages.h>
#include <direct/utf8.h>
#include <direct/util.h>

#include <misc/conf.h>
#include <misc/util.h>

#include <dgiff.h>
#include "sawman_font.h"

/**********************************************************************************************************************/

#if 1

#undef SIZEOF_LONG
#include <ft2build.h>
#include FT_GLYPH_H

#ifndef FT_LOAD_TARGET_MONO
    /* FT_LOAD_TARGET_MONO was added in FreeType-2.1.3. We have to use
       (less good) FT_LOAD_MONOCHROME with older versions. Make it an
       alias for code simplicity. */
    #define FT_LOAD_TARGET_MONO FT_LOAD_MONOCHROME
#endif

#ifndef FT_LOAD_FORCE_AUTOHINT
    #define FT_LOAD_FORCE_AUTOHINT 0
#endif
#ifndef FT_LOAD_TARGET_LIGHT
    #define FT_LOAD_TARGET_LIGHT 0
#endif

static FT_Library      library           = NULL;
static int             library_ref_count = 0;
static pthread_mutex_t library_mutex     = PTHREAD_MUTEX_INITIALIZER;

#define KERNING_CACHE_MIN    0
#define KERNING_CACHE_MAX  127
#define KERNING_CACHE_SIZE (KERNING_CACHE_MAX - KERNING_CACHE_MIN + 1)

#define KERNING_DO_CACHE(a,b)      ((a) >= KERNING_CACHE_MIN && \
                                    (a) <= KERNING_CACHE_MAX && \
                                    (b) >= KERNING_CACHE_MIN && \
                                    (b) <= KERNING_CACHE_MAX)

#define KERNING_CACHE_ENTRY(a,b)   \
     (data->kerning[(a)-KERNING_CACHE_MIN][(b)-KERNING_CACHE_MIN])

#define CHAR_INDEX(c)    (((c) < 256) ? data->indices[c] : FT_Get_Char_Index( data->face, c ))

typedef struct {
     FT_Face      face;
     int          disable_charmap;
     int          fixed_advance;
     bool         fixed_clip;
     unsigned int indices[256];
     int          outline_radius;
     int          outline_opacity;
     float        up_unit_x;     /* unit vector pointing 'up' in for */
     float        up_unit_y;     /* this font's rotation             */
} FT2ImplData;

typedef struct {
     bool        initialised;
     signed char x;
     signed char y;
} KerningCacheEntry;

typedef struct {
     FT2ImplData base;

     KerningCacheEntry kerning[KERNING_CACHE_SIZE][KERNING_CACHE_SIZE];
} FT2ImplKerningData;

/**********************************************************************************************************************/

static DFBResult
ft2UTF8GetCharacterIndex( CoreFont     *thiz,
                          unsigned int  character,
                          unsigned int *ret_index )
{
     FT2ImplData *data = thiz->impl_data;

     D_MAGIC_ASSERT( thiz, CoreFont );

     if (data->disable_charmap)
          *ret_index = character;
     else {
          pthread_mutex_lock ( &library_mutex );

          *ret_index = CHAR_INDEX( character );

          pthread_mutex_unlock ( &library_mutex );
     }

     return DFB_OK;
}

static DFBResult
ft2UTF8DecodeText( CoreFont       *thiz,
                   const void     *text,
                   int             length,
                   unsigned int   *ret_indices,
                   int            *ret_num )
{
     int pos = 0, num = 0;
     const u8 *bytes   = text;
     FT2ImplData *data = thiz->impl_data;

     D_MAGIC_ASSERT( thiz, CoreFont );
     D_ASSERT( text != NULL );
     D_ASSERT( length >= 0 );
     D_ASSERT( ret_indices != NULL );
     D_ASSERT( ret_num != NULL );

     pthread_mutex_lock ( &library_mutex );

     while (pos < length) {
          unsigned int c;

          if (bytes[pos] < 128)
               c = bytes[pos++];
          else {
               c = DIRECT_UTF8_GET_CHAR( &bytes[pos] );
               pos += DIRECT_UTF8_SKIP(bytes[pos]);
          }

          if (data->disable_charmap)
               ret_indices[num++] = c;
          else
               ret_indices[num++] = CHAR_INDEX( c );
     }

     pthread_mutex_unlock ( &library_mutex );

     *ret_num = num;

     return DFB_OK;
}

static const CoreFontEncodingFuncs ft2UTF8Funcs = {
     .GetCharacterIndex = ft2UTF8GetCharacterIndex,
     .DecodeText        = ft2UTF8DecodeText,
};

/**********************************************************************************************************************/

static DFBResult
ft2Latin1GetCharacterIndex( CoreFont     *thiz,
                            unsigned int  character,
                            unsigned int *ret_index )
{
     FT2ImplData *data = thiz->impl_data;

     D_MAGIC_ASSERT( thiz, CoreFont );

     if (data->disable_charmap)
          *ret_index = character;
     else
          *ret_index = data->indices[character];

     return DFB_OK;
}

static DFBResult
ft2Latin1DecodeText( CoreFont       *thiz,
                     const void     *text,
                     int             length,
                     unsigned int   *ret_indices,
                     int            *ret_num )
{
     int i;
     const u8 *bytes   = text;
     FT2ImplData *data = thiz->impl_data;

     D_MAGIC_ASSERT( thiz, CoreFont );
     D_ASSERT( text != NULL );
     D_ASSERT( length >= 0 );
     D_ASSERT( ret_indices != NULL );
     D_ASSERT( ret_num != NULL );

     if (data->disable_charmap) {
          for (i=0; i<length; i++)
               ret_indices[i] = bytes[i];
     }
     else {
          for (i=0; i<length; i++)
               ret_indices[i] = data->indices[bytes[i]];
     }

     *ret_num = length;

     return DFB_OK;
}

static const CoreFontEncodingFuncs ft2Latin1Funcs = {
     .GetCharacterIndex = ft2Latin1GetCharacterIndex,
     .DecodeText        = ft2Latin1DecodeText,
};

/**********************************************************************************************************************/

static DFBResult
render_glyph( CoreFont      *thiz,
              unsigned int   index,
              CoreGlyphData *info )
{
     FT_Error     err;
     FT_Face      face;
     FT_Int       load_flags;
     u8          *src;
     int          y;
     FT2ImplData *data    = thiz->impl_data;
     CoreSurface *surface = info->surface;
     CoreSurfaceBufferLock  lock;

     pthread_mutex_lock ( &library_mutex );

     face = data->face;

     load_flags = (unsigned long) face->generic.data;
     load_flags |= FT_LOAD_RENDER;

     if ((err = FT_Load_Glyph( face, index, load_flags ))) {
          D_DEBUG( "DirectFB/FontFT2: Could not render glyph for character index #%d!\n", index );
          pthread_mutex_unlock ( &library_mutex );
          return DFB_FAILURE;
     }

     pthread_mutex_unlock ( &library_mutex );

     err = dfb_surface_lock_buffer( surface, CSBR_BACK, CSAID_CPU, CSAF_WRITE, &lock );
     if (err) {
          D_DERROR( err, "DirectFB/FontFT2: Unable to lock surface!\n" );
          return err;
     }

     info->width = face->glyph->bitmap.width;
     if (info->width + info->start > surface->config.size.w)
          info->width = surface->config.size.w - info->start;

     info->height = face->glyph->bitmap.rows;
     if (info->height > surface->config.size.h)
          info->height = surface->config.size.h;

     /* bitmap_left and bitmap_top are relative to the glyph's origin on the
        baseline.  info->left and info->top are relative to the top-left of the
        character cell. */
     info->left =   face->glyph->bitmap_left - thiz->ascender*thiz->up_unit_x;
     info->top  = - face->glyph->bitmap_top  - thiz->ascender*thiz->up_unit_y;

     if (info->layer == 1 && info->width > 0 && info->height > 0) {
          int   xoffset, yoffset;
          void *addr;
          void *blurred = NULL;
          int   radius  = data->outline_radius;

          switch (face->glyph->bitmap.pixel_mode) {
               case ft_pixel_mode_grays:
                    blurred = D_CALLOC( 1, (info->width + radius) * (info->height + radius) );
                    if (blurred) {
                         for (yoffset=0; yoffset<radius; yoffset++) {
                              for (xoffset=0; xoffset<radius; xoffset++) {
                                   src = face->glyph->bitmap.buffer;

                                   for (y=0; y < info->height; y++) {
                                        int  i;
                                        u8  *dst8 = blurred + xoffset + (y + yoffset) * (info->width + radius);

                                        for (i=0; i<info->width; i++) {
                                             int val = dst8[i] + src[i]/radius;

                                             dst8[i] = (val < 255) ? val : 255;
                                        }

                                        src += face->glyph->bitmap.pitch;
                                   }
                              }
                         }
                    }
                    else
                         D_OOM();
                    break;

               case ft_pixel_mode_mono:
                    D_UNIMPLEMENTED();
                    break;

               default:
                    break;

          }


          info->width  += radius;
          info->height += radius;

          info->left   -= (radius - 1) / 2;
          info->top    -= (radius - 1) / 2;

          if (blurred) {
               addr = lock.addr + DFB_BYTES_PER_LINE(surface->config.format, info->start);
               src  = blurred;

               for (y=0; y < info->height; y++) {
                    int  i;
                    u8  *dst8  = addr;
                    u32 *dst32 = addr;

                    switch (face->glyph->bitmap.pixel_mode) {
                         case ft_pixel_mode_grays:
                              switch (surface->config.format) {
                                   case DSPF_ARGB:
                                   case DSPF_ABGR:
                                        if (thiz->surface_caps & DSCAPS_PREMULTIPLIED) {
                                             for (i=0; i<info->width; i++)
                                                  dst32[i] = ((data->outline_opacity + 1) * src[i] / 256) * 0x01010101;
                                        }
                                        else {
                                             for (i=0; i<info->width; i++)
                                                  dst32[i] = (((data->outline_opacity + 1) * src[i] / 256) << 24) | 0xFFFFFF;
                                        }
                                        break;
                                   case DSPF_A8:
                                        for (i=0; i<info->width; i++)
                                             dst8[i] = (data->outline_opacity + 1) * src[i] / 256;
                                        break;
                                   default:
                                        D_UNIMPLEMENTED();
                                        break;
                              }
                              break;

                         case ft_pixel_mode_mono:
                              D_UNIMPLEMENTED();
                              break;

                         default:
                              break;

                    }

                    src  += info->width;
                    addr += lock.pitch;
               }

               D_FREE( blurred );
          }
     }
     else {
          if (data->fixed_clip) {
               while (info->left + info->width > data->fixed_advance)
                    info->left--;

               if (info->left < 0)
                    info->left = 0;

               if (info->width > data->fixed_advance)
                    info->width = data->fixed_advance;
          }

          src = face->glyph->bitmap.buffer;
          lock.addr += DFB_BYTES_PER_LINE(surface->config.format, info->start);

          for (y=0; y < info->height; y++) {
               int  i, j, n;
               u8  *dst8  = lock.addr;
               u16 *dst16 = lock.addr;
               u32 *dst32 = lock.addr;

               switch (face->glyph->bitmap.pixel_mode) {
                    case ft_pixel_mode_grays:
                         switch (surface->config.format) {
                              case DSPF_ARGB:
                              case DSPF_ABGR:
                                   if (thiz->surface_caps & DSCAPS_PREMULTIPLIED) {
                                        for (i=0; i<info->width; i++)
                                             dst32[i] = src[i] * 0x01010101;
                                   }
                                   else
                                        for (i=0; i<info->width; i++)
                                             dst32[i] = (src[i] << 24) | 0xFFFFFF;
                                   break;
                              case DSPF_AiRGB:
                                   for (i=0; i<info->width; i++)
                                        dst32[i] = ((src[i] ^ 0xFF) << 24) | 0xFFFFFF;
                                   break;
                              case DSPF_ARGB8565:
                                   for (i = 0, j = -1; i < info->width; ++i) {
                                       u32 d;
                                       if (thiz->surface_caps & DSCAPS_PREMULTIPLIED) {
                                            d = src[i] * 0x01010101;
                                            d = ARGB_TO_ARGB8565 (d);
                                       }
                                       else
                                            d = (src[i] << 16) | 0xFFFF;
#ifdef WORDS_BIGENDIAN
                                        dst8[++j] = (d >> 16) & 0xff;
                                        dst8[++j] = (d >>  8) & 0xff;
                                        dst8[++j] = (d >>  0) & 0xff;
#else
                                        dst8[++j] = (d >>  0) & 0xff;
                                        dst8[++j] = (d >>  8) & 0xff;
                                        dst8[++j] = (d >> 16) & 0xff;
#endif
                                   }
                                   break;
                              case DSPF_ARGB4444:
                              case DSPF_RGBA4444:
                                   if (thiz->surface_caps & DSCAPS_PREMULTIPLIED) {
                                        for (i=0; i<info->width; i++)
                                             dst16[i] = (src[i] >> 4) * 0x1111;
                                   }
                                   else {
                                        if( surface->config.format == DSPF_ARGB4444 ) {
                                             for (i=0; i<info->width; i++)
                                                  dst16[i] = (src[i] << 8) | 0x0FFF;
                                        } else {
                                             for (i=0; i<info->width; i++)
                                                  dst16[i] = (src[i] >> 4) | 0xFFF0;
                                        }
                                   }
                                   break;
                              case DSPF_ARGB2554:
                                   for (i=0; i<info->width; i++)
                                        dst16[i] = (src[i] << 8) | 0x3FFF;
                                   break;
                              case DSPF_ARGB1555:
                                   if (thiz->surface_caps & DSCAPS_PREMULTIPLIED) {
                                        for (i=0; i<info->width; i++) {
                                             unsigned short x = src[i] >> 3;
                                             dst16[i] = ((src[i] & 0x80) << 8) |
                                                  (x << 10) | (x << 5) | x;
                                        }
                                   }
                                   else {
                                        for (i=0; i<info->width; i++)
                                             dst16[i] = (src[i] << 8) | 0x7FFF;
                                   }
                                   break;
                              case DSPF_RGBA5551:
                                   if (thiz->surface_caps & DSCAPS_PREMULTIPLIED) {
                                        for (i=0; i<info->width; i++) {
                                             unsigned short x = src[i] >> 3;
                                             dst16[i] =  (x << 11) | (x << 6) | (x << 1) |
                                                  (src[i] >> 7);
                                        }
                                   }
                                   else {
                                        for (i=0; i<info->width; i++)
                                             dst16[i] = 0xFFFE | (src[i] >> 7);
                                   }
                                   break;
                              case DSPF_A8:
                                   direct_memcpy( lock.addr, src, info->width );
                                   break;
                              case DSPF_A4:
                                   for (i=0, j=0; i<info->width; i+=2, j++)
                                        dst8[j] = (src[i] & 0xF0) | (src[i+1] >> 4);
                                   break;
                              case DSPF_A1:
                                   for (i=0, j=0; i < info->width; ++j) {
                                        register u8 p = 0;

                                        for (n=0; n<8 && i<info->width; ++i, ++n)
                                             p |= (src[i] & 0x80) >> n;

                                        dst8[j] = p;
                                   }
                                   break;
                              case DSPF_A1_LSB:
                                   for (i=0, j=0; i < info->width; ++j) {
                                        register u8 p = 0;

                                        for (n=0; n<8 && i<info->width; ++i, ++n)
                                             p |= (src[i] & 0x80) >> (7-n);

                                        dst8[j] = p;
                                   }
                                   break;
                              case DSPF_LUT2:
                                   for (i=0, j=0; i < info->width; ++j) {
                                        register u8 p = 0;

                                        for (n=0; n<8 && i<info->width; ++i, n+=2)
                                             p |= (src[i] & 0xC0) >> n;

                                        dst8[j] = p;
                                   }
                                   break;
                              default:
                                   D_UNIMPLEMENTED();
                                   break;
                         }
                         break;

                    case ft_pixel_mode_mono:
                         switch (surface->config.format) {
                              case DSPF_ARGB:
                              case DSPF_ABGR:
                                   if (thiz->surface_caps & DSCAPS_PREMULTIPLIED) {
                                        for (i=0; i<info->width; i++)
                                             dst32[i] = (src[i>>3] & (1<<(7-(i%8)))) ?
                                                          0xFFFFFFFF : 0x00000000;
                                   }
                                   else {
                                        for (i=0; i<info->width; i++)
                                             dst32[i] = (((src[i>>3] & (1<<(7-(i%8)))) ?
                                                          0xFF : 0x00) << 24) | 0xFFFFFF;
                                   }
                                   break;
                              case DSPF_AiRGB:
                                   if (thiz->surface_caps & DSCAPS_PREMULTIPLIED) {
                                        for (i=0; i<info->width; i++)
                                             dst32[i] = (src[i>>3] & (1<<(7-(i%8)))) ?
                                                          0x00FFFFFF : 0xFF000000;
                                   }
                                   else {
                                        for (i=0; i<info->width; i++)
                                             dst32[i] = (((src[i>>3] & (1<<(7-(i%8)))) ?
                                                          0x00 : 0xFF) << 24) | 0xFFFFFF;
                                   }
                                   break;
                              case DSPF_ARGB8565:
                                   for (i = 0, j = -1; i < info->width; ++i) {
                                        u32 d;
                                        if (thiz->surface_caps & DSCAPS_PREMULTIPLIED) {
                                             d = ((src[i>>3] & (1<<(7-(i%8)))) ?
                                                  0xffffff : 0x000000);
                                        }
                                        else
                                            d = (((src[i>>3] & (1<<(7-(i%8)))) ?
                                                  0xff : 0x00) << 16) | 0xffff;
#ifdef WORDS_BIGENDIAN
                                        dst8[++j] = (d >> 16) & 0xff;
                                        dst8[++j] = (d >>  8) & 0xff;
                                        dst8[++j] = (d >>  0) & 0xff;
#else
                                        dst8[++j] = (d >>  0) & 0xff;
                                        dst8[++j] = (d >>  8) & 0xff;
                                        dst8[++j] = (d >> 16) & 0xff;
#endif
                                   }
                                   break;
                              case DSPF_ARGB4444:
                                   for (i=0; i<info->width; i++)
                                        dst16[i] = (((src[i>>3] & (1<<(7-(i%8)))) ?
                                                     0xF : 0x0) << 12) | 0xFFF;
                                   break;
                              case DSPF_RGBA4444:
                                   for (i=0; i<info->width; i++)
                                        dst16[i] = (((src[i>>3] & (1<<(7-(i%8)))) ?
                                                     0xF : 0x0)      ) | 0xFFF0;
                                   break;
                              case DSPF_ARGB2554:
                                   for (i=0; i<info->width; i++)
                                        dst16[i] = (((src[i>>3] & (1<<(7-(i%8)))) ?
                                                     0x3 : 0x0) << 14) | 0x3FFF;
                                   break;
                              case DSPF_ARGB1555:
                                   for (i=0; i<info->width; i++)
                                        dst16[i] = (((src[i>>3] & (1<<(7-(i%8)))) ?
                                                     0x1 : 0x0) << 15) | 0x7FFF;
                                   break;
                              case DSPF_RGBA5551:
                                   for (i=0; i<info->width; i++)
                                        dst16[i] = 0xFFFE | (((src[i>>3] & (1<<(7-(i%8)))) ?
                                                     0x1 : 0x0));
                                   break;
                              case DSPF_A8:
                                   for (i=0; i<info->width; i++)
                                        dst8[i] = (src[i>>3] &
                                                   (1<<(7-(i%8)))) ? 0xFF : 0x00;
                                   break;
                              case DSPF_A4:
                                   for (i=0, j=0; i<info->width; i+=2, j++)
                                        dst8[j] = ((src[i>>3] &
                                                   (1<<(7-(i%8)))) ? 0xF0 : 0x00) |
                                                  ((src[(i+1)>>3] &
                                                   (1<<(7-((i+1)%8)))) ? 0x0F : 0x00);
                                   break;
                              case DSPF_A1:
                                   direct_memcpy( lock.addr, src, DFB_BYTES_PER_LINE(DSPF_A1, info->width) );
                                   break;
                              case DSPF_A1_LSB:
                                   for (i=0, j=0; i < info->width; ++j) {
                                        register u8 p = 0;

                                        for (n=0; n<8 && i<info->width; ++i, ++n)
                                             p |= (((src[i] >> n) & 1) << (7-n));

                                        dst8[j] = p;
                                   }
                                   break;
                              default:
                                   D_UNIMPLEMENTED();
                                   break;
                         }
                         break;

                    default:
                         break;

               }

               src += face->glyph->bitmap.pitch;

               lock.addr += lock.pitch;
          }
     }

     dfb_surface_unlock_buffer( surface, &lock );

     return DFB_OK;
}


static DFBResult
get_glyph_info( CoreFont      *thiz,
                unsigned int   index,
                CoreGlyphData *info )
{
     FT_Error err;
     FT_Face  face;
     FT_Int   load_flags;
     FT2ImplData *data = (FT2ImplData*) thiz->impl_data;

     pthread_mutex_lock ( &library_mutex );

     face = data->face;

     load_flags = (unsigned long) face->generic.data;

     if ((err = FT_Load_Glyph( face, index, load_flags ))) {
          D_DEBUG( "DirectFB/FontFT2: Could not load glyph for character index #%d!\n", index );

          pthread_mutex_unlock ( &library_mutex );

          return DFB_FAILURE;
     }

     if (face->glyph->format != ft_glyph_format_bitmap) {
          err = FT_Render_Glyph( face->glyph,
                                 (load_flags & FT_LOAD_TARGET_MONO) ? ft_render_mode_mono : ft_render_mode_normal );
          if (err) {
               D_ERROR( "DirectFB/FontFT2: Could not render glyph for character index #%d!\n", index );

               pthread_mutex_unlock ( &library_mutex );

               return DFB_FAILURE;
          }
     }

     pthread_mutex_unlock ( &library_mutex );

     info->width   = face->glyph->bitmap.width;
     info->height  = face->glyph->bitmap.rows;

     if (data->fixed_advance) {
          info->xadvance = - data->fixed_advance * thiz->up_unit_y;
          info->yadvance =   data->fixed_advance * thiz->up_unit_x;
     }
     else {
          info->xadvance =   face->glyph->advance.x << 2;
          info->yadvance = - face->glyph->advance.y << 2;
     }

     if (data->fixed_clip && info->width > data->fixed_advance)
          info->width = data->fixed_advance;

     if (info->layer == 1 && info->width > 0 && info->height > 0) {
          info->width  += data->outline_radius;
          info->height += data->outline_radius;
     }

     return DFB_OK;
}


static DFBResult
get_kerning( CoreFont     *thiz,
             unsigned int  prev,
             unsigned int  current,
             int          *kern_x,
             int          *kern_y)
{
     FT_Vector vector;

     FT2ImplKerningData *data = thiz->impl_data;
     KerningCacheEntry *cache = NULL;

     D_ASSUME( (kern_x != NULL) || (kern_y != NULL) );

     /*
      * Use cached values if characters are in the
      * cachable range and the cache entry is already filled.
      */
     if (KERNING_DO_CACHE (prev, current)) {
          cache = &KERNING_CACHE_ENTRY (prev, current);

          if (!cache->initialised && FT_HAS_KERNING(data->base.face)) {
               FT_Vector vector;

               pthread_mutex_lock ( &library_mutex );

               /* Lookup kerning values for the character pair. */
               FT_Get_Kerning( data->base.face,
                               prev, current, ft_kerning_default, &vector );

               cache->x = (signed char) ((int)(- vector.x*data->base.up_unit_x + vector.y*data->base.up_unit_x) >> 6);
               cache->y = (signed char) ((int)(  vector.y*data->base.up_unit_y + vector.x*data->base.up_unit_x) >> 6);

               cache->initialised = true;

               pthread_mutex_unlock ( &library_mutex );
          }

          if (kern_x)
               *kern_x = (int) cache->x;

          if (kern_y)
               *kern_y = (int) cache->y;

          return DFB_OK;
     }

     pthread_mutex_lock ( &library_mutex );

     /* Lookup kerning values for the character pair. */
     /* The vector returned by FreeType does not allow for any rotation. */
     FT_Get_Kerning( data->base.face,
                     prev, current, ft_kerning_default, &vector );

     pthread_mutex_unlock ( &library_mutex );

     /* Convert to integer. */
     if (kern_x)
          *kern_x = (int)(- vector.x*thiz->up_unit_y + vector.y*thiz->up_unit_x) >> 6;

     if (kern_y)
          *kern_y = (int)(  vector.y*thiz->up_unit_y + vector.x*thiz->up_unit_x) >> 6;

     return DFB_OK;
}

static DFBResult
init_freetype( void )
{
     FT_Error err;

     pthread_mutex_lock ( &library_mutex );

     if (!library) {
          D_DEBUG( "DirectFB/FontFT2: Initializing the FreeType2 library.\n" );
          err = FT_Init_FreeType( &library );
          if (err) {
               D_ERROR( "DirectFB/FontFT2: "
                         "Initialization of the FreeType2 library failed!\n" );
               library = NULL;
               pthread_mutex_unlock( &library_mutex );
               return DFB_FAILURE;
          }
     }

     library_ref_count++;
     pthread_mutex_unlock( &library_mutex );

     return DFB_OK;
}


static void
release_freetype( void )
{
     pthread_mutex_lock( &library_mutex );

     if (library && --library_ref_count == 0) {
          D_DEBUG( "DirectFB/FontFT2: Releasing the FreeType2 library.\n" );
          FT_Done_FreeType( library );
          library = NULL;
     }

     pthread_mutex_unlock( &library_mutex );
}


void
sawman_unload_font( CoreFont *font )
{
     if (font->impl_data) {
          FT2ImplData *impl_data = (FT2ImplData*) font->impl_data;

          pthread_mutex_lock ( &library_mutex );
          FT_Done_Face( impl_data->face );
          pthread_mutex_unlock ( &library_mutex );

          D_FREE( impl_data );

          font->impl_data = NULL;
     }

     dfb_font_destroy( font );

     release_freetype();
}


DFBResult
sawman_load_font( CoreDFB                     *core,
                  const char                  *filename,
                  DFBFontDescription          *desc,
                  CoreFont                   **ret_font )
{
     int                 i;
     DFBResult           ret;
     CoreFont           *font;
     FT_Face             face;
     FT_Error            err;
     FT_Int              load_flags = FT_LOAD_DEFAULT;
     FT2ImplData        *data;
     bool                disable_charmap = false;
     bool                disable_kerning = false;
     bool                load_mono = false;
     u32                 mask = 0;
     DFBFontAttributes   attributes = DFFA_NONE;

     float sin_rot = 0.0;
     float cos_rot = 1.0;

     D_DEBUG( "DirectFB/FontFT2: "
              "Construct font from file `%s' (index %d) at pixel size %d x %d and rotation %d.\n",
              filename,
              (desc->flags & DFDESC_INDEX)    ? desc->index    : 0,
              (desc->flags & DFDESC_WIDTH)    ? desc->width    : 0,
              (desc->flags & DFDESC_HEIGHT)   ? desc->height   : 0,
              (desc->flags & DFDESC_ROTATION) ? desc->rotation : 0 );

     if (init_freetype() != DFB_OK) {
          return DFB_FAILURE;
     }

     pthread_mutex_lock ( &library_mutex );
     err = FT_New_Face( library, filename, 0, &face );
     pthread_mutex_unlock ( &library_mutex );
     if (err) {
          switch (err) {
               case FT_Err_Unknown_File_Format:
                    D_ERROR( "DirectFB/FontFT2: "
                              "Unsupported font format in file `%s'!\n", filename );
                    break;
               default:
                    D_ERROR( "DirectFB/FontFT2: "
                              "Failed loading face %d from font file `%s'!\n",
                              (desc->flags & DFDESC_INDEX) ? desc->index : 0,
                              filename );
                    break;
          }
          return DFB_FAILURE;
     }

     if ((desc->flags & DFDESC_ROTATION) && desc->rotation) {
          if (!FT_IS_SCALABLE(face)) {
               D_ERROR( "DirectFB/FontFT2: "
                         "Face %d from font file `%s' is not scalable so cannot be rotated\n",
                         (desc->flags & DFDESC_INDEX) ? desc->index : 0,
                         filename );
               pthread_mutex_lock ( &library_mutex );
               FT_Done_Face( face );
               pthread_mutex_unlock ( &library_mutex );
               return DFB_UNSUPPORTED;
          }

          float rot_radians = 2.0 * M_PI * desc->rotation / (1<<24);
          sin_rot = sin(rot_radians);
          cos_rot = cos(rot_radians);

          int sin_rot_fx = (int)(sin_rot*65536.0);
          int cos_rot_fx = (int)(cos_rot*65536.0);
          FT_Matrix matrix;
          matrix.xx =  cos_rot_fx;
          matrix.xy = -sin_rot_fx;
          matrix.yx =  sin_rot_fx;
          matrix.yy =  cos_rot_fx;

          pthread_mutex_lock ( &library_mutex );
          FT_Set_Transform( face, &matrix, NULL );
          /* FreeType docs suggest FT_Set_Transform returns an error code, but it seems
             that this is not the case. */
          pthread_mutex_unlock ( &library_mutex );
     }

     if (dfb_config->font_format == DSPF_A1 ||
               dfb_config->font_format == DSPF_A1_LSB ||
               dfb_config->font_format == DSPF_ARGB1555 ||
               dfb_config->font_format == DSPF_RGBA5551)
          load_mono = true;

     if (desc->flags & DFDESC_ATTRIBUTES) {
          if (desc->attributes & DFFA_NOHINTING)
               load_flags |= FT_LOAD_NO_HINTING;
          if (desc->attributes & DFFA_NOBITMAP)
               load_flags |= FT_LOAD_NO_BITMAP;
          if (desc->attributes & DFFA_AUTOHINTING)
               load_flags |= FT_LOAD_FORCE_AUTOHINT;
          if (desc->attributes & DFFA_SOFTHINTING)
               load_flags |= FT_LOAD_TARGET_LIGHT;
          if (desc->attributes & DFFA_NOCHARMAP)
               disable_charmap = true;
          if (desc->attributes & DFFA_NOKERNING)
               disable_kerning = true;
          if (desc->attributes & DFFA_MONOCHROME)
               load_mono = true;
          if (desc->attributes & DFFA_VERTICAL_LAYOUT)
               load_flags |= FT_LOAD_VERTICAL_LAYOUT;

#ifdef FT_LOAD_OBLIQUE
          if (desc->attributes & DFFA_STYLE_ITALIC)
               load_flags |= FT_LOAD_OBLIQUE;
#endif

          attributes = desc->attributes;
     }

     if (load_mono)
          load_flags |= FT_LOAD_TARGET_MONO;

     if (!disable_charmap) {
          pthread_mutex_lock ( &library_mutex );
          err = FT_Select_Charmap( face, ft_encoding_unicode );
          pthread_mutex_unlock ( &library_mutex );

#if FREETYPE_MINOR > 0

          /* ft_encoding_latin_1 has been introduced in freetype-2.1 */
          if (err) {
               D_DEBUG( "DirectFB/FontFT2: "
                        "Couldn't select Unicode encoding, "
                        "falling back to Latin1.\n");
               pthread_mutex_lock ( &library_mutex );
               err = FT_Select_Charmap( face, ft_encoding_latin_1 );
               pthread_mutex_unlock ( &library_mutex );
          }
#endif
          if (err) {
               D_DEBUG( "DirectFB/FontFT2: "
                        "Couldn't select Unicode/Latin1 encoding, "
                        "trying Symbol.\n");
               pthread_mutex_lock ( &library_mutex );
               err = FT_Select_Charmap( face, ft_encoding_symbol );
               pthread_mutex_unlock ( &library_mutex );

               if (!err)
                    mask = 0xf000;
          }
     }

#if 0
     if (err) {
          D_ERROR( "DirectFB/FontFT2: "
                    "Couldn't select a suitable encoding for face %d from font file `%s'!\n", (desc->flags & DFDESC_INDEX) ? desc->index : 0, filename );
          pthread_mutex_lock ( &library_mutex );
          FT_Done_Face( face );
          pthread_mutex_unlock ( &library_mutex );
          return DFB_FAILURE;
     }
#endif

     if (desc->flags & (DFDESC_HEIGHT       | DFDESC_WIDTH |
                        DFDESC_FRACT_HEIGHT | DFDESC_FRACT_WIDTH))
     {
          int fw = 0, fh = 0;

          if (desc->flags & DFDESC_FRACT_HEIGHT)
               fh = desc->fract_height;
          else if (desc->flags & DFDESC_HEIGHT)
               fh = desc->height << 6;

          if (desc->flags & DFDESC_FRACT_WIDTH)
               fw = desc->fract_width;
          else if (desc->flags & DFDESC_WIDTH)
               fw = desc->width << 6;

          pthread_mutex_lock ( &library_mutex );
          err = FT_Set_Char_Size( face, fw, fh, 0, 0 );
          pthread_mutex_unlock ( &library_mutex );
          if (err) {
               D_ERROR( "DirectB/FontFT2: "
                         "Could not set pixel size to %d x %d!\n",
                         (desc->flags & DFDESC_WIDTH)  ? desc->width  : 0,
                         (desc->flags & DFDESC_HEIGHT) ? desc->height : 0 );
               pthread_mutex_lock ( &library_mutex );
               FT_Done_Face( face );
               pthread_mutex_unlock ( &library_mutex );
               return DFB_FAILURE;
          }
     }

     face->generic.data = (void *)(unsigned long) load_flags;
     face->generic.finalizer = NULL;

     ret = dfb_font_create( core, desc, filename, &font );
     if (ret) {
          pthread_mutex_lock ( &library_mutex );
          FT_Done_Face( face );
          pthread_mutex_unlock ( &library_mutex );
          return ret;
     }

     font->attributes = attributes;
     font->flags      = CFF_SUBPIXEL_ADVANCE;

     D_ASSERT( font->pixel_format == DSPF_ARGB ||
               font->pixel_format == DSPF_ABGR ||
               font->pixel_format == DSPF_AiRGB ||
               font->pixel_format == DSPF_ARGB8565 ||
               font->pixel_format == DSPF_ARGB4444 ||
               font->pixel_format == DSPF_RGBA4444 ||
               font->pixel_format == DSPF_ARGB2554 ||
               font->pixel_format == DSPF_ARGB1555 ||
               font->pixel_format == DSPF_RGBA5551 ||
               font->pixel_format == DSPF_A8 ||
               font->pixel_format == DSPF_A4 ||
               font->pixel_format == DSPF_A1 ||
               font->pixel_format == DSPF_A1_LSB ||
               font->pixel_format == DSPF_LUT2 );

     font->ascender   = face->size->metrics.ascender >> 6;
     font->descender  = face->size->metrics.descender >> 6;
     font->height     = font->ascender + ABS(font->descender) + 1;
     font->maxadvance = face->size->metrics.max_advance >> 6;

     font->up_unit_x  = -sin_rot;
     font->up_unit_y  = -cos_rot;

     D_DEBUG( "DirectFB/FontFT2: height = %d, ascender = %d, descender = %d, maxadvance = %d, up unit: %5.2f,%5.2f\n",
              font->height, font->ascender, font->descender, font->maxadvance, font->up_unit_x, font->up_unit_y );

     font->GetGlyphData = get_glyph_info;
     font->RenderGlyph  = render_glyph;

     if (FT_HAS_KERNING(face) && !disable_kerning) {
          font->GetKerning = get_kerning;
          data = D_CALLOC( 1, sizeof(FT2ImplKerningData) );
     }
     else
          data = D_CALLOC( 1, sizeof(FT2ImplData) );

     data->face            = face;
     data->disable_charmap = disable_charmap;

     if (attributes & DFFA_OUTLINED) {
          if (desc->flags & DFDESC_OUTLINE_WIDTH)
               data->outline_radius = 1 + (desc->outline_width >> 16) * 2;
          else
               data->outline_radius = 3;

          if (desc->flags & DFDESC_OUTLINE_OPACITY)
               data->outline_opacity = desc->outline_opacity;
          else
               data->outline_opacity = 0xff;
     }

     if (desc->flags & DFDESC_FIXEDADVANCE) {
          data->fixed_advance = desc->fixed_advance;
          font->maxadvance    = desc->fixed_advance;

          if ((desc->flags & DFDESC_ATTRIBUTES) && (desc->attributes & DFFA_FIXEDCLIP))
               data->fixed_clip = true;
     }

     for (i=0; i<256; i++)
          data->indices[i] = FT_Get_Char_Index( face, i | mask );

     data->up_unit_x = font->up_unit_x;
     data->up_unit_y = font->up_unit_y;

     font->impl_data = data;

     dfb_font_register_encoding( font, "UTF8",   &ft2UTF8Funcs,   DTEID_UTF8 );
     dfb_font_register_encoding( font, "Latin1", &ft2Latin1Funcs, DTEID_OTHER );

     *ret_font = font;

     return DFB_OK;
}

#else

typedef struct {
     unsigned int                 stamp;

     CoreSurface                 *surface;
     int                          next_x;

     DirectLink                  *glyphs;

     int                          magic;
} CoreFontCacheRow;


typedef struct {
     void *map;     /* Memory map of the file. */
     int   size;    /* Size of the memory map. */

     CoreFontCacheRow            **rows;          /* contain bitmaps of loaded glyphs */
     int                           num_rows;
} DGIFFImplData;

/**********************************************************************************************************************/

DFBResult
sawman_load_font( CoreDFB                     *core,
                  const char                  *filename,
                  DFBFontDescription          *desc,
                  CoreFont                   **ret_font )
{
     DFBResult        ret;
     int              i;
     int              fd;
     struct stat      stat;
     void            *ptr  = MAP_FAILED;
     CoreFont        *font = NULL;
     DGIFFHeader     *header;
     DGIFFFaceHeader *face;
     DGIFFGlyphInfo  *glyphs;
     DGIFFGlyphRow   *row;
     DGIFFImplData   *data = NULL;
     CoreSurfaceConfig config;

     if (desc->flags & DFDESC_ROTATION)
          return DFB_UNSUPPORTED;

     /* Open the file. */
     fd = open( filename, O_RDONLY );
     if (fd < 0) {
          ret = errno2result( errno );
          D_PERROR( "SAWMAN/DGIFF: Failure during open() of '%s'!\n", filename );
          return ret;
     }

     /* Query file size etc. */
     if (fstat( fd, &stat ) < 0) {
          ret = errno2result( errno );
          D_PERROR( "SAWMAN/DGIFF: Failure during fstat() of '%s'!\n", filename );
          goto error;
     }

     /* Memory map the file. */
     ptr = mmap( NULL, stat.st_size, PROT_READ, MAP_SHARED, fd, 0 );
     if (ptr == MAP_FAILED) {
          ret = errno2result( errno );
          D_PERROR( "SAWMAN/DGIFF: Failure during mmap() of '%s'!\n", filename );
          goto error;
     }

     /* Keep entry pointers for main header and face. */
     header = ptr;
     face   = ptr + sizeof(DGIFFHeader);

     /* Check the magic. */
     if (strncmp( (char*) header->magic, "DGIFF", 5 ))
     {
          ret = DFB_UNSUPPORTED;
          D_PERROR( "SAWMAN/DGIFF: Failure Check the magic of '%s'!\n", filename );
          goto error;
     }

     /* Lookup requested face, otherwise use first if nothing requested or show error if not found. */
     if (desc->flags & DFDESC_HEIGHT) {
          for (i=0; i<header->num_faces; i++) {
               if (face->size == desc->height)
                    break;

               face = ((void*) face) + face->next_face;
          }

          if (i == header->num_faces) {
               ret = DFB_UNSUPPORTED;
               D_ERROR( "SAWMAN/DGIFF: Requested size %d not found in '%s'!\n", desc->height, filename );
               goto error;
          }
     }

     glyphs = (void*)(face + 1);
     row    = (void*)(glyphs + face->num_glyphs);

     /* Create the core object. */
     ret = dfb_font_create( core, desc, filename, &font );
     if (ret)
          goto error;

     /* Fill font information. */
     font->ascender     = face->ascender;
     font->descender    = face->descender;
     font->height       = face->height;
     font->up_unit_x    =  0.0;
     font->up_unit_y    = -1.0;

     font->maxadvance   = face->max_advance;
     font->pixel_format = face->pixelformat;
     font->surface_caps = DSCAPS_NONE;
     font->flags        = CFF_SUBPIXEL_ADVANCE;


     data = D_CALLOC( 1, sizeof(DGIFFImplData) );
     if (!data) {
          ret = D_OOM();
          goto error;
     }

     data->num_rows     = face->num_rows;

     if (face->blittingflags)
          font->blittingflags = face->blittingflags;

     CORE_FONT_DEBUG_AT( Font_DGIFF, font );

     /* Allocate array for glyph cache rows. */
     data->rows = D_CALLOC( face->num_rows, sizeof(void*) );
     if (!data->rows) {
          ret = D_OOM();
          goto error;
     }

     /* Build glyph cache rows. */
     bool prealloc = false;

     config.flags  = CSCONF_SIZE | CSCONF_FORMAT;
     config.format = face->pixelformat;

     if (prealloc) {
          config.flags |= CSCONF_PREALLOCATED;

          config.preallocated[1].addr = NULL;
          config.preallocated[1].pitch = 0;
     }

     for (i=0; i<face->num_rows; i++) {
          data->rows[i] = D_CALLOC( 1, sizeof(CoreFontCacheRow) );
          if (!data->rows[i]) {
               ret = D_OOM();
               goto error;
          }

          config.size.w = row->width;
          config.size.h = row->height;

          if (prealloc) {
               config.preallocated[0].addr = (void*)(row+1);
               config.preallocated[0].pitch = row->pitch;
          }

          ret = CoreDFB_CreateSurface( core, &config, prealloc ? CSTF_PREALLOCATED : CSTF_NONE, 0, NULL,
                                       &data->rows[i]->surface );

          if (ret) {
               D_DERROR( ret, "SAWMAN/DGIFF/Font: Could not create preallocated %s %dx%d glyph row surface!\n",
                         dfb_pixelformat_name(face->pixelformat), row->width, row->height );
               goto error;
          }

          if (!prealloc)
               dfb_surface_write_buffer( data->rows[i]->surface, CSBR_BACK, (void*)(row+1), row->pitch, NULL );

          D_MAGIC_SET( data->rows[i], CoreFontCacheRow );

          /* Jump to next row. */
          row = (void*)(row + 1) + row->pitch * row->height;
     }

     /* Build glyph infos. */
     for (i=0; i<face->num_glyphs; i++) {
          CoreGlyphData  *glyph_data;
          DGIFFGlyphInfo *glyph = &glyphs[i];

          glyph_data = D_CALLOC( 1, sizeof(CoreGlyphData) );
          if (!glyph_data) {
               ret = D_OOM();
               goto error;
          }

          glyph_data->surface  = data->rows[glyph->row]->surface;

          glyph_data->start    = glyph->offset;
          glyph_data->width    = glyph->width;
          glyph_data->height   = glyph->height;
          glyph_data->left     = glyph->left;
          glyph_data->top      = glyph->top;
          glyph_data->xadvance = glyph->advance << 8;
          glyph_data->yadvance = 0;

          D_MAGIC_SET( glyph_data, CoreGlyphData );

          direct_hash_insert( font->layers[0].glyph_hash, glyph->unicode, glyph_data );

          if (glyph->unicode < 128)
               font->layers[0].glyph_data[glyph->unicode] = glyph_data;
     }


     data->map  = ptr;
     data->size = stat.st_size;

     font->impl_data = data;

     /* Already close, we still have the map. */
     close( fd );

     *ret_font = font;
     
     return DFB_OK;


error:
     if (font) {
          if (data->rows) {
               for (i=0; i<data->num_rows; i++) {
                    if (data->rows[i]) {
                         if (data->rows[i]->surface)
                              dfb_surface_unref( data->rows[i]->surface );

                         D_FREE( data->rows[i] );
                    }
               }

               D_FREE( data->rows );

               data->rows = NULL;
          }

          dfb_font_destroy( font );
     }

     if (ptr != MAP_FAILED)
          munmap( ptr, stat.st_size );

     close( fd );

     return ret;
}

void
sawman_unload_font( CoreFont *font )
{
     DGIFFImplData      *impl = font->impl_data;

     munmap( impl->map, impl->size );

     D_FREE( impl );

     dfb_font_destroy( font );
}

#endif

DFBResult
SawMan_DrawString( CoreGraphicsStateClient *client, CoreFont *core_font,
                   const char *text, int bytes,
                   int x, int y,
                   DFBSurfaceTextFlags flags,
                   DFBTextEncodingID encoding )
{
     DFBResult           ret;
     unsigned int        layers = 1;

     if (!client)
          return DFB_INVARG;

     if (!core_font)
          return DFB_INVARG;

     if (!text)
          return DFB_INVARG;

     if (bytes < 0)
          bytes = strlen (text);

     if (bytes == 0)
          return DFB_OK;

     if (core_dfb->shutdown_running)
          return DFB_OK;

     if (flags & DSTF_OUTLINE) {
          if (!(core_font->attributes & DFFA_OUTLINED))
               return DFB_UNSUPPORTED;

          layers = 2;
     }

     if (!(flags & DSTF_TOP)) {
          x += core_font->ascender * core_font->up_unit_x;
          y += core_font->ascender * core_font->up_unit_y;

          if (flags & DSTF_BOTTOM) {
               x -= core_font->descender * core_font->up_unit_x;
               y -= core_font->descender * core_font->up_unit_y;
          }
     }

     if (flags & (DSTF_RIGHT | DSTF_CENTER)) {
          int          i, num, kx, ky;
          int          xsize = 0;
          int          ysize = 0;
          unsigned int prev = 0;
          unsigned int indices[bytes];

          /* FIXME: Avoid double locking and decoding. */
          dfb_font_lock( core_font );

          /* Decode string to character indices. */
          ret = dfb_font_decode_text( core_font, encoding, text, bytes, indices, &num );
          if (ret) {
               dfb_font_unlock( core_font );
               return ret;
          }

          D_ASSERT( num <= bytes );

          /* Calculate string width. */
          for (i=0; i<num; i++) {
               unsigned int   current = indices[i];
               CoreGlyphData *glyph;

               if (dfb_font_get_glyph_data( core_font, current, 0, &glyph ) == DFB_OK) {
                    xsize += glyph->xadvance;
                    ysize += glyph->yadvance;

                    if (prev && core_font->GetKerning &&
                        core_font->GetKerning( core_font, prev, current, &kx, &ky ) == DFB_OK) {
                         xsize += kx << 8;
                         ysize += ky << 8;
                    }
               }

               prev = current;
          }

          dfb_font_unlock( core_font );

          /* Justify. */
          if (flags & DSTF_RIGHT) {
               x -= xsize >> 8;
               y -= ysize >> 8;
          }
          else if (flags & DSTF_CENTER) {
               x -= xsize >> 9;
               y -= ysize >> 9;
          }
     }

     dfb_gfxcard_drawstring( (const unsigned char*) text, bytes, encoding,
                             x, y,
                             core_font, layers, client, flags );

     return DFB_OK;
}

DFBResult
SawMan_DrawGlyph( CoreGraphicsStateClient *client, CoreFont *core_font,
                  unsigned int character, int x, int y,
                  DFBSurfaceTextFlags flags,
                  DFBTextEncodingID encoding )
{
     DFBResult           ret;
     int                 l;
     CoreGlyphData      *glyph[DFB_FONT_MAX_LAYERS];
     unsigned int        index;
     unsigned int        layers = 1;

     if (!client)
          return DFB_INVARG;

     if (!core_font)
          return DFB_INVARG;

     if (!character)
          return DFB_INVARG;

     if (core_dfb->shutdown_running)
          return DFB_OK;

     if (flags & DSTF_OUTLINE) {
          if (!(core_font->attributes & DFFA_OUTLINED))
               return DFB_UNSUPPORTED;

          layers = 2;
     }

     dfb_font_lock( core_font );

     ret = dfb_font_decode_character( core_font, encoding, character, &index );
     if (ret) {
          dfb_font_unlock( core_font );
          return ret;
     }

     for (l=0; l<layers; l++) {
          ret = dfb_font_get_glyph_data( core_font, index, l, &glyph[l] );
          if (ret) {
               dfb_font_unlock( core_font );
               return ret;
          }
     }

     if (!(flags & DSTF_TOP)) {
          x += core_font->ascender * core_font->up_unit_x;
          y += core_font->ascender * core_font->up_unit_y;

          if (flags & DSTF_BOTTOM) {
               x -= core_font->descender * core_font->up_unit_x;
               y -= core_font->descender * core_font->up_unit_y;
          }
     }

     if (flags & (DSTF_RIGHT | DSTF_CENTER)) {
          if (flags & DSTF_RIGHT) {
               x -= glyph[0]->xadvance;
               y -= glyph[0]->yadvance;
          }
          else if (flags & DSTF_CENTER) {
               x -= glyph[0]->xadvance >> 1;
               y -= glyph[0]->yadvance >> 1;
          }
     }

     dfb_gfxcard_drawglyph( glyph,
                            x, y,
                            core_font, layers, client, flags );

     dfb_font_unlock( core_font );

     return DFB_OK;
}

