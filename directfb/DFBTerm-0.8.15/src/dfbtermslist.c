/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GLib Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GLib Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GLib at ftp://ftp.gtk.org/pub/gtk/. 
 */

/* 
 * MT safe
 */

#ifdef HAVE_CONFIG_H
     #include <config.h>
#endif

#include <malloc.h>
#include <asm/types.h>

#include "dfbtermslist.h"

DFBTermSList*
dfbterm_slist_alloc (void)
{
     DFBTermSList *list;

     list = calloc (1, sizeof (DFBTermSList));

     return list;
}

void
dfbterm_slist_free (DFBTermSList *list)
{
     DFBTermSList *last;

     while (list) {
          last = list;
          list = list->next;
          free (last);
     }
}

DFBTermSList*
dfbterm_slist_append (DFBTermSList *list,
                      void   *data)
{
     DFBTermSList *new_list;
     DFBTermSList *last;

     new_list = calloc (1, sizeof (DFBTermSList));
     new_list->data = data;

     if (list) {
          last = dfbterm_slist_last (list);
          /* g_assert (last != NULL); */
          last->next = new_list;

          return list;
     } else
          return new_list;
}

DFBTermSList*
dfbterm_slist_prepend (DFBTermSList *list,
                       void   *data)
{
     DFBTermSList *new_list;

     new_list = calloc (1, sizeof (DFBTermSList));
     new_list->data = data;
     new_list->next = list;

     return new_list;
}

DFBTermSList*
dfbterm_slist_insert (DFBTermSList *list,
                      void   *data,
                      int     position)
{
     DFBTermSList *prev_list;
     DFBTermSList *tmp_list;
     DFBTermSList *new_list;

     if (position < 0)
          return dfbterm_slist_append (list, data);
     else if (position == 0)
          return dfbterm_slist_prepend (list, data);

     new_list = calloc (1, sizeof (DFBTermSList));
     new_list->data = data;

     if (!list)
          return new_list;

     prev_list = NULL;
     tmp_list = list;

     while ((position-- > 0) && tmp_list) {
          prev_list = tmp_list;
          tmp_list = tmp_list->next;
     }

     if (prev_list) {
          new_list->next = prev_list->next;
          prev_list->next = new_list;
     } else {
          new_list->next = list;
          list = new_list;
     }

     return list;
}

DFBTermSList*
dfbterm_slist_insert_before (DFBTermSList *slist,
                             DFBTermSList *sibling,
                             void   *data)
{
     if (!slist) {
          slist = dfbterm_slist_alloc ();
          slist->data = data;
          return slist;
     } else {
          DFBTermSList *node, *last = NULL;

          for (node = slist; node; last = node, node = last->next)
               if (node == sibling)
                    break;
          if (!last) {
               node = dfbterm_slist_alloc ();
               node->data = data;
               node->next = slist;

               return node;
          } else {
               node = dfbterm_slist_alloc ();
               node->data = data;
               node->next = last->next;
               last->next = node;

               return slist;
          }
     }
}

DFBTermSList *
dfbterm_slist_concat (DFBTermSList *list1, DFBTermSList *list2)
{
     if (list2) {
          if (list1)
               dfbterm_slist_last (list1)->next = list2;
          else
               list1 = list2;
     }

     return list1;
}

DFBTermSList*
dfbterm_slist_remove (DFBTermSList     *list,
                      const void *data)
{
     DFBTermSList *tmp, *prev = NULL;

     tmp = list;
     while (tmp) {
          if (tmp->data == data) {
               if (prev)
                    prev->next = tmp->next;
               else
                    list = tmp->next;

               free (tmp);
               break;
          }
          prev = tmp;
          tmp = prev->next;
     }

     return list;
}

DFBTermSList*
dfbterm_slist_remove_all (DFBTermSList     *list,
                          const void *data)
{
     DFBTermSList *tmp, *prev = NULL;

     tmp = list;
     while (tmp) {
          if (tmp->data == data) {
               DFBTermSList *next = tmp->next;

               if (prev)
                    prev->next = next;
               else
                    list = next;

               free (tmp);
               tmp = next;
          } else {
               prev = tmp;
               tmp = prev->next;
          }
     }

     return list;
}

static inline DFBTermSList*
_dfbterm_slist_remove_link (DFBTermSList *list,
                            DFBTermSList *link)
{
     DFBTermSList *tmp;
     DFBTermSList *prev;

     prev = NULL;
     tmp = list;

     while (tmp) {
          if (tmp == link) {
               if (prev)
                    prev->next = tmp->next;
               if (list == tmp)
                    list = list->next;

               tmp->next = NULL;
               break;
          }

          prev = tmp;
          tmp = tmp->next;
     }

     return list;
}

DFBTermSList* 
dfbterm_slist_remove_link (DFBTermSList *list,
                           DFBTermSList *link)
{
     return _dfbterm_slist_remove_link (list, link);
}

DFBTermSList*
dfbterm_slist_delete_link (DFBTermSList *list,
                           DFBTermSList *link)
{
     list = _dfbterm_slist_remove_link (list, link);
     free (link);

     return list;
}

DFBTermSList*
dfbterm_slist_copy (DFBTermSList *list)
{
     DFBTermSList *new_list = NULL;

     if (list) {
          DFBTermSList *last;

          new_list = calloc (1, sizeof (DFBTermSList));
          new_list->data = list->data;
          last = new_list;
          list = list->next;
          while (list) {
               last->next = calloc (1, sizeof (DFBTermSList));
               last = last->next;
               last->data = list->data;
               list = list->next;
          }
     }

     return new_list;
}

DFBTermSList*
dfbterm_slist_reverse (DFBTermSList *list)
{
     DFBTermSList *prev = NULL;

     while (list) {
          DFBTermSList *next = list->next;

          list->next = prev;

          prev = list;
          list = next;
     }

     return prev;
}

DFBTermSList*
dfbterm_slist_nth (DFBTermSList       *list,
                   unsigned int  n)
{
     while (n-- > 0 && list)
          list = list->next;

     return list;
}

void*
dfbterm_slist_nth_data (DFBTermSList       *list,
                        unsigned int  n)
{
     while (n-- > 0 && list)
          list = list->next;

     return list ? list->data : NULL;
}

DFBTermSList*
dfbterm_slist_find (DFBTermSList     *list,
                    const void *data)
{
     while (list) {
          if (list->data == data)
               break;
          list = list->next;
     }

     return list;
}

int
dfbterm_slist_position (DFBTermSList *list,
                        DFBTermSList *link)
{
     int i;

     i = 0;
     while (list) {
          if (list == link)
               return i;
          i++;
          list = list->next;
     }

     return -1;
}

int
dfbterm_slist_index (DFBTermSList     *list,
                     const void *data)
{
     int i;

     i = 0;
     while (list) {
          if (list->data == data)
               return i;
          i++;
          list = list->next;
     }

     return -1;
}

DFBTermSList*
dfbterm_slist_last (DFBTermSList *list)
{
     if (list) {
          while (list->next)
               list = list->next;
     }

     return list;
}

unsigned int
dfbterm_slist_length (DFBTermSList *list)
{
     unsigned int length;

     length = 0;
     while (list) {
          length++;
          list = list->next;
     }

     return length;
}
