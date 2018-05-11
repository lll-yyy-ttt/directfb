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

/**
 * @defgroup LiTE Toolbox Enabler
 * @brief The LiTE Toolbox Enabler
 */

/**
 * @brief  This file contains definitions for the LiTE general interface.
 * @ingroup LiTE
 * @file lite.h
 * <hr>
 **/

#ifndef __LITE__LITE_H__
#define __LITE__LITE_H__

#ifdef __cplusplus
extern "C"
{
#endif

#include <directfb.h>
#include <direct/types.h>
#include <lite/lite_build.h>

#if LITE_HAS_FUSIONDALE
#include <fusiondale.h>
#endif

/* @brief Start the LiTE framework
 * This function will start the LiTE framework by creating the underlying
 * services such as DirectFB layers and interfaces. Any other global
 * initializaton is also done here.
 * 
 * @param argc                  argc values passed from main
 * @param argv                  argv values passed from main
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_open( int *argc, char **argv[] );

/* @brief Close the LiTE framework
 * This function will close the LiTE framework, release any
 * resources creating using the start sequence and so on.
 *
 * @return bool                 Returns true if successful.
 */
DFBResult lite_close(void);

/* @brief Test if default window theme is loaded.
 * This function tests if the default window theme was loaded or not.
 * The lite environment by default loads the window theme unless
 * the environment variable LITE_NO_THEME is exported, or the
 * lite_config.h DEFAULT_WINDOW_THEME is set to false.
 * 
 * @return bool                 Returns true if successful.
 */
bool lite_default_window_theme_loaded();

/* @brief Get the underlying IDirectFB interface.
 * This function returns the IDirectFB interface that was created
 * during the lite_open() sequence.
 *  
 * @return IDirectFB*           Pointer to the IDirectFB interface.        
 */
IDirectFB* lite_get_dfb_interface(void);

/* @brief Get the underlying IDirectFB Display layer interface.
 * Thus function returns the DirectFB display layer interface that
 * was created during the lite_open sequence.
 *
 * @param ret_layer             OUT:    DirectFB display layter
 *
 * @return DFBResult            Returns DFB_OK if successful.     
 */
DFBResult lite_get_layer_interface(IDirectFBDisplayLayer **ret_layer);


DFBResult lite_get_layer_size( int *ret_width,
                               int *ret_height );

#if LITE_HAS_FUSIONDALE
IComa* lite_get_coma_interface(void);

IComaComponent* lite_get_osk_component(void);

DFBResult lite_dale_get_component( const char* name, IComaComponent** component );

DFBResult lite_dale_get_local( unsigned int bytes, void** ret );

DFBResult lite_dale_call( IComaComponent* component, ComaMethodID method, void* arg );

#endif

#ifdef __cplusplus
}
#endif

#endif /*  __LITE__LITE_H__  */
