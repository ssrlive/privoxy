#ifndef _LIST_H
#define _LIST_H
#define LIST_H_VERSION "$Id: list.h,v NOT CHECKED IN YET $"
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/list.c,v $
 *
 * Purpose     :  Declares functions to handle lists.
 *                Functions declared include:
 *                   `destroy_list', `enlist' and `list_to_text'
 *
 * Copyright   :  Written by and Copyright (C) 2001 the SourceForge
 *                IJBSWA team.  http://ijbswa.sourceforge.net
 *
 *                Based on the Internet Junkbuster originally written
 *                by and Copyright (C) 1997 Anonymous Coders and 
 *                Junkbusters Corporation.  http://www.junkbusters.com
 *
 *                This program is free software; you can redistribute it 
 *                and/or modify it under the terms of the GNU General
 *                Public License as published by the Free Software
 *                Foundation; either version 2 of the License, or (at
 *                your option) any later version.
 *
 *                This program is distributed in the hope that it will
 *                be useful, but WITHOUT ANY WARRANTY; without even the
 *                implied warranty of MERCHANTABILITY or FITNESS FOR A
 *                PARTICULAR PURPOSE.  See the GNU General Public
 *                License for more details.
 *
 *                The GNU General Public License should be included with
 *                this file.  If not, you can view it at
 *                http://www.gnu.org/copyleft/gpl.html
 *                or write to the Free Software Foundation, Inc., 59
 *                Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * Revisions   :
 *    $Log: list.c,v $
 *
 *********************************************************************/


#include "project.h"

#ifdef __cplusplus
extern "C" {
#endif


extern void enlist(struct list *h, const char *s);
extern void destroy_list(struct list *h);
extern char *list_to_text(struct list *h);

void enlist_unique(struct list *header, const char *str);

int list_remove_item(struct list *header, const char *str);
int list_remove_list(struct list *header, const struct list *to_remove);

void list_duplicate(struct list *dest, const struct list *src);
void list_append_list_unique(struct list *dest, const struct list *src);

void destroy_list_share(struct list_share *h);
void enlist_share(struct list_share *header, const char *str);
void enlist_unique_share(struct list_share *header, const char *str);
int list_remove_item_share(struct list_share *header, const char *str);
int list_remove_list_share(struct list_share *dest, const struct list *src);
void list_duplicate_share(struct list_share *dest, const struct list *src);
void list_append_list_unique_share(struct list_share *dest, const struct list *src);

/* Revision control strings from this header and associated .c file */
extern const char list_rcs[];
extern const char list_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef _LIST_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
