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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
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

#ifndef __G_SLIST_H__
#define __G_SLIST_H__

typedef struct _DFBTermSList       DFBTermSList;

struct _DFBTermSList {
     void   *data;
     DFBTermSList *next;
};

/* Singly linked lists
 */
DFBTermSList*      dfbterm_slist_alloc         (void);
void               dfbterm_slist_free          (DFBTermSList           *list);
DFBTermSList*      dfbterm_slist_append        (DFBTermSList           *list,
                                                void             *data);
DFBTermSList*      dfbterm_slist_prepend       (DFBTermSList           *list,
                                                void             *data);
DFBTermSList*      dfbterm_slist_insert        (DFBTermSList           *list,
                                                void             *data,
                                                int               position);
DFBTermSList*      dfbterm_slist_insert_before (DFBTermSList           *slist,
                                                DFBTermSList           *sibling,
                                                void             *data);
DFBTermSList*      dfbterm_slist_concat        (DFBTermSList           *list1,
                                                DFBTermSList           *list2);
DFBTermSList*      dfbterm_slist_remove        (DFBTermSList           *list,
                                                const void       *data);
DFBTermSList*      dfbterm_slist_remove_all    (DFBTermSList           *list,
                                                const void       *data);
DFBTermSList*      dfbterm_slist_remove_link   (DFBTermSList           *list,
                                                DFBTermSList           *link);
DFBTermSList*      dfbterm_slist_delete_link   (DFBTermSList           *list,
                                                DFBTermSList           *link);
DFBTermSList*      dfbterm_slist_reverse       (DFBTermSList           *list);
DFBTermSList*      dfbterm_slist_copy          (DFBTermSList           *list);
DFBTermSList*      dfbterm_slist_nth           (DFBTermSList           *list,
                                                unsigned int      n);
DFBTermSList*      dfbterm_slist_find          (DFBTermSList           *list,
                                                const void       *data);
int                dfbterm_slist_position      (DFBTermSList           *list,
                                                DFBTermSList           *llink);
int                dfbterm_slist_index         (DFBTermSList           *list,
                                                const void       *data);
DFBTermSList*      dfbterm_slist_last          (DFBTermSList           *list);
unsigned int       dfbterm_slist_length        (DFBTermSList           *list);
void*              dfbterm_slist_nth_data      (DFBTermSList           *list,
                                                unsigned int      n);

#define  dfbterm_slist_next(slist)	((slist) ? (((DFBTermSList *)(slist))->next) : NULL)

#endif /* __DFBTERM_SLIST_H__ */

