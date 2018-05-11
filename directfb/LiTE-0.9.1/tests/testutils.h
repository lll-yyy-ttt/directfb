/*
   (c) Copyright 2005  Kent Sandvik

   Written by Kent Sandvik <kent@directfb.org>
              
   This file is subject to the terms and conditions of the MIT License:

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation
   files (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software,
   and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef __LITE_TESTUTILS_H__
#define __LITE_TESTUTILS_H__

#include <stddef.h>
#include <stdio.h>

#include <directfb.h>

#include <lite/window.h>

#define LTEST(name, x...)                                       \
{                                                               \
     DFBResult err = x;                                         \
     fprintf(stderr, "LTEST: %s...",name );                     \
     if (err != DFB_OK) {                                       \
          fprintf(stderr, "FAILED, error: %d  function: %s file: %s line: %d \n",  \
                    DirectFBError( #x, err),__FUNCTION__,  __FILE__, __LINE__);    \
     }                                                          \
     else {                                                     \
          fprintf(stderr, "OK\n");                              \
     }                                                          \
}                                               

void ltest_start_timer(char *name);
int ltest_stop_timer(void);
char* ltest_get_timestamp_name(int index);
int ltest_get_num_timestamps(void);
void ltest_print_timestamps(void);
void ltest_display_timestamps(void);
LiteWindow* ltest_create_main_window(const char* title);


#endif /* __LITE_TESTUTILS_H__ */
