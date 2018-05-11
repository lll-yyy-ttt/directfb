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
#include <stdlib.h> /* free */

#include <pthread.h>
#include <directfb.h>

#include "lite_internal.h"

#include "util.h"

D_DEBUG_DOMAIN(LiteUtilDomain, "LiTE/Util", "LiteUtil");

static DFBResult 
_util_load_image(const char             *filename,
                 DFBSurfacePixelFormat   pixelformat,
                 IDirectFBSurface      **surface,
                 int                    *ret_width,
                 int                    *ret_height,
                 DFBImageDescription    *desc,
                 bool                    premultiply)
{
     DFBResult               ret;
     DFBSurfaceDescription   dsc;
     IDirectFBSurface       *image = NULL;
     IDirectFBImageProvider *provider = NULL;

     LITE_NULL_PARAMETER_CHECK(filename);
     LITE_NULL_PARAMETER_CHECK(surface);

     /* Create an image provider for loading the file */
     ret = lite_dfb->CreateImageProvider(lite_dfb, filename, &provider);
     if (ret) {
          D_DERROR(ret,
                   "CreateImageProvider for '%s' failed\n",
                   filename);
          return ret;
     }

     /* Retrieve a surface description for the image */
     ret = provider->GetSurfaceDescription(provider, &dsc);
     if (ret) {
          D_DEBUG_AT(LiteUtilDomain,
                   "GetSurfaceDescription for '%s': %s\n",
                   filename, DirectFBErrorString(ret));
          provider->Release(provider);
          return ret;
     }

     /* Use the specified pixelformat if the image's pixelformat is not ARGB */
     if (pixelformat && (!(dsc.flags & DSDESC_PIXELFORMAT) || dsc.pixelformat != DSPF_ARGB)) {
          dsc.flags      |= DSDESC_PIXELFORMAT;
          dsc.pixelformat = pixelformat;
     }
    
     if (premultiply && DFB_PIXELFORMAT_HAS_ALPHA(dsc.pixelformat)) {
          dsc.flags |= DSDESC_CAPS;
          dsc.caps   = DSCAPS_PREMULTIPLIED;
     }

     /* Create a surface using the description */
     ret = lite_dfb->CreateSurface(lite_dfb, &dsc, &image);
     if (ret) {
          D_DERROR(ret,
                   "load_image: CreateSurface %dx%d failed\n",
                   dsc.width, dsc.height);
          provider->Release(provider);
          return ret;
     }

     /* Render the image to the created surface */
     ret = provider->RenderTo(provider, image, NULL);
     if (ret) {
          D_DEBUG_AT(LiteUtilDomain,
                   "load_image: RenderTo for '%s': %s\n",
                   filename, DirectFBErrorString(ret));
          image->Release(image);
          provider->Release(provider);
          return ret;
     }

     /* Return surface */
     *surface = image;

     /* Return width? */
     if (ret_width)
          *ret_width = dsc.width;

     /* Return height? */
     if (ret_height)
          *ret_height  = dsc.height;

     /* Retrieve the image description? */
     if (desc)
          provider->GetImageDescription(provider, desc);

     /* Release the provider */
     provider->Release(provider);

     return DFB_OK;
}

DFBResult 
lite_util_load_image(const char             *filename,
                     DFBSurfacePixelFormat   pixelformat,
                     IDirectFBSurface      **surface,
                     int                    *ret_width,
                     int                    *ret_height,
                     DFBImageDescription    *desc)
{
     return _util_load_image( filename, pixelformat, surface, ret_width, ret_height, desc, false );
}

DFBResult 
lite_util_load_image_premultiplied(const char             *filename,
                                   DFBSurfacePixelFormat   pixelformat,
                                   IDirectFBSurface      **surface,
                                   int                    *ret_width,
                                   int                    *ret_height,
                                   DFBImageDescription    *desc)
{
     return _util_load_image( filename, pixelformat, surface, ret_width, ret_height, desc, true );
}


DFBResult 
lite_util_load_image_desc(IDirectFBSurface **surface, const DFBSurfaceDescription *dsc) 
{       
     DFBResult         ret;
     IDirectFBSurface *image = NULL;

     LITE_NULL_PARAMETER_CHECK(surface);

     /* Create the image surface */
     ret = lite_dfb->CreateSurface(lite_dfb, dsc, &image); 
     if (ret) {
          D_DEBUG_AT(LiteUtilDomain,
                   "CreateSurface %dx%d: %s\n",
                   dsc->width, dsc->height, DirectFBErrorString(ret));
          return ret;
     }
    
     *surface = image;
 
     return DFB_OK; 
} 

DFBResult 
lite_util_tile_image(IDirectFBSurface *surface,
                       const char       *filename,
                       int               x,
                       int               y,
                       int               nx,
                       int               ny)
{
     DFBResult         ret;
     int               width, height;
     IDirectFBSurface *image = NULL;

     LITE_NULL_PARAMETER_CHECK(surface);
     LITE_NULL_PARAMETER_CHECK(filename);

     ret = lite_util_load_image(filename, DSPF_UNKNOWN,
                                 &image, &width, &height, NULL);
     if (ret)
          return ret;

     surface->SetBlittingFlags(surface, DSBLIT_NOFX);

     while (ny--) {
          int i;

          for (i=0; i<nx; i++) {
               surface->Blit(surface, image, NULL, x + i*width, y);
          }

          y += height;
     }

     image->Release(image);

     return DFB_OK;
}

IDirectFBSurface*
lite_util_sub_surface(IDirectFBSurface *surface,
                      int               x,
                      int               y,
                      int               width,
                      int               height)
{
     DFBResult         ret;
     IDirectFBSurface *sub = NULL;
     DFBRectangle      rect;

     rect.x = x;
     rect.y = y;
     rect.w = width;
     rect.h = height;

     ret = surface->GetSubSurface(surface, &rect, &sub);
     if (ret) {
          DirectFBError("lite_util_sub_surface: surface->GetSubSurface", ret);
          return NULL;
     }

     return sub;
}

