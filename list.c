const char list_rcs[] = "$Id: list.c,v 1.7 2001/08/05 16:06:20 jongfoster Exp $";
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
 *    Revision 1.7  2001/08/05 16:06:20  jongfoster
 *    Modifiying "struct map" so that there are now separate header and
 *    "map_entry" structures.  This means that functions which modify a
 *    map no longer need to return a pointer to the modified map.
 *    Also, it no longer reverses the order of the entries (which may be
 *    important with some advanced template substitutions).
 *
 *    Revision 1.6  2001/07/31 14:44:51  oes
 *    list_to_text() now appends empty line at end
 *
 *    Revision 1.5  2001/06/29 21:45:41  oes
 *    Indentation, CRLF->LF, Tab-> Space
 *
 *    Revision 1.4  2001/06/29 13:30:22  oes
 *    - Added Convenience function enlist_unique_header(),
 *      which takes the Header name and value as separate
 *      arguments and thus saves the pain of sprintf()ing
 *      and determining the Header name length to enlist_unique
 *    - Improved comments
 *    - Removed logentry from cancelled commit
 *
 *    Revision 1.3  2001/06/03 19:12:24  oes
 *    functions for new struct map, extended enlist_unique
 *
 *    Revision 1.2  2001/06/01 18:49:17  jongfoster
 *    Replaced "list_share" with "list" - the tiny memory gain was not
 *    worth the extra complexity.
 *
 *    Revision 1.1  2001/05/31 21:11:53  jongfoster
 *    - Moved linked list support to new "list.c" file.
 *      Structure definitions are still in project.h,
 *      function prototypes are now in "list.h".
 *    - Added support for "struct list_share", which is identical
 *      to "struct list" except it saves memory by not duplicating
 *      the strings.  Obviously, this only works if there is some
 *      other way of managing the memory used by the strings.
 *      (These list_share lists are used for lists which last
 *      for only 1 request, and where all the list entries are
 *      just coming directly from entries in the actionsfile.)
 *      Note that you still need to destroy list_share lists
 *      properly to free the nodes - it's only the strings
 *      which are shared.
 *
 *
 *********************************************************************/


#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include <assert.h>

#ifndef _WIN32
#include <unistd.h>
#endif

#include "project.h"
#include "jcc.h"
#include "list.h"
#include "miscutil.h"

const char list_h_rcs[] = LIST_H_VERSION;


/*********************************************************************
 *
 * Function    :  enlist
 *
 * Description :  Append a string into a specified string list.
 *
 * Parameters  :
 *          1  :  header = pointer to list 'dummy' header
 *          2  :  str = string to add to the list (maybe NULL)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void enlist(struct list *header, const char *str)
{
   struct list *cur = (struct list *)malloc(sizeof(*cur));
   struct list *last;

   if (cur)
   {
      cur->str  = (str ? strdup(str) : NULL);
      cur->next = NULL;

      last = header->last;
      if (last == NULL)
      {
         last = header;
      }

      last->next   = cur;
      header->last = cur;
   }

}


/*********************************************************************
 *
 * Function    :  enlist_first
 *
 * Description :  Append a string as first element into a specified
 *                string list.
 *
 * Parameters  :
 *          1  :  header = pointer to list 'dummy' header
 *          2  :  str = string to add to the list (maybe NULL)
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void enlist_first(struct list *header, const char *str)
{
   struct list *cur = (struct list *)malloc(sizeof(*cur));

   if (cur)
   {
      cur->str  = (str ? strdup(str) : NULL);
      cur->next = header->next;

      header->next = cur;
      if (header->last == NULL)
      {
         header->last = cur;
      }
   }

}


/*********************************************************************
 *
 * Function    :  enlist_unique
 *
 * Description :  Append a string into a specified string list,
 *                if & only if it's not there already.
 *                If the n argument is nonzero, only compare up to
 *                the nth character. 
 *
 * Parameters  :
 *          1  :  header = pointer to list 'dummy' header
 *          2  :  str = string to add to the list (maybe NULL)
 *          3  :  n = number of chars to use for uniqueness test
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void enlist_unique(struct list *header, const char *str, int n)
{
   struct list *last;
   struct list *cur = header->next;

   while (cur != NULL)
   {
      if ((cur->str != NULL) && (
         (n && (0 == strncmp(str, cur->str, n))) || 
         (!n && (0 == strcmp(str, cur->str)))))
      {
         /* Already there */
         return;
      }
      cur = cur->next;
   }

   cur = (struct list *)malloc(sizeof(*cur));

   if (cur != NULL)
   {
      cur->str  = (str ? strdup(str) : NULL); /* FIXME check retval */
      cur->next = NULL;

      last = header->last;
      if (last == NULL)
      {
         last = header;
      }
      last->next   = cur;
      header->last = cur;
   }
}


/*********************************************************************
 *
 * Function    :  enlist_unique_header
 *
 * Description :  Make a HTTP header from the two strings name and value,
 *                and append the result into a specified string list,
 *                if & only if there isn't already a header with that name.
 *
 * Parameters  :
 *          1  :  header = pointer to list 'dummy' header
 *          2  :  name = name of header to be added
 *          3  :  value = value of header to be added
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void enlist_unique_header(struct list *header, const char *name, const char *value)
{
   struct list *last;
   struct list *cur = header->next;
   int length;
   char *dummy;

   if (name == NULL || value == NULL) return;

   dummy = strdup(name);
   dummy = strsav(dummy, ": ");
   length = strlen(dummy);

   while (cur != NULL)
   {
      if ((cur->str != NULL) && 
   		(0 == strncmp(dummy, cur->str, length)))
      {
         /* Already there */
         return;
      }
      cur = cur->next;
   }

   cur = (struct list *)malloc(sizeof(*cur));

   if (cur != NULL)
   {
      cur->str  = strsav(dummy, value);
      cur->next = NULL;

      last = header->last;
      if (last == NULL)
      {
         last = header;
      }
      last->next   = cur;
      header->last = cur;
   }
}


/*********************************************************************
 *
 * Function    :  destroy_list
 *
 * Description :  Destroy a string list (opposite of enlist)
 *
 * Parameters  :
 *          1  :  header = pointer to list 'dummy' header
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void destroy_list(struct list *header)
{
   struct list *p, *n;

   for (p = header->next; p ; p = n)
   {
      n = p->next;
      freez(p->str);
      free(p);
   }

   memset(header, '\0', sizeof(*header));

}


/*********************************************************************
 *
 * Function    :  list_to_text
 *
 * Description :  "Flaten" a string list into 1 long \r\n delimited string,
 *                adding an empty line at the end.
 *
 * Parameters  :
 *          1  :  h = pointer to list 'dummy' header
 *
 * Returns     :  NULL on malloc error, else new long string.
 *
 *********************************************************************/
char *list_to_text(struct list *h)
{
   struct list *p;
   char *ret = NULL;
   char *s;
   int size = 2;

   for (p = h->next; p ; p = p->next)
   {
      if (p->str)
      {
         size += strlen(p->str) + 2;
      }
   }

   if ((ret = (char *)malloc(size + 1)) == NULL)
   {
      return(NULL);
   }

   ret[size] = '\0';

   s = ret;

   for (p = h->next; p ; p = p->next)
   {
      if (p->str)
      {
         strcpy(s, p->str);
         s += strlen(s);
         *s++ = '\r'; *s++ = '\n';
      }
   }
   *s++ = '\r'; *s++ = '\n';

   return(ret);

}


/*********************************************************************
 *
 * Function    :  list_remove_item
 *
 * Description :  Remove a string from a specified string list.
 *
 * Parameters  :
 *          1  :  header = pointer to list 'dummy' header
 *          2  :  str = string to remove from the list
 *
 * Returns     :  Number of times it was removed.
 *
 *********************************************************************/
int list_remove_item(struct list *header, const char *str)
{
   struct list *prev = header;
   struct list *cur = prev->next;
   int count = 0;

   while (cur != NULL)
   {
      if ((cur->str != NULL) && (0 == strcmp(str, cur->str)))
      {
         count++;

         prev->next = cur->next;
         free(cur->str);
         free(cur);
      }
      else
      {
         prev = cur;
      }
      cur = prev->next;
   }

   header->last = prev;

   return count;
}


/*********************************************************************
 *
 * Function    :  list_remove_list
 *
 * Description :  Remove all strings in one list from another list.
 *                This is currently a brute-force algorithm
 *                (it compares every pair of strings).
 *
 * Parameters  :
 *          1  :  dest = list to change
 *          2  :  src = list of strings to remove
 *
 * Returns     :  Total number of strings removed.
 *
 *********************************************************************/
int list_remove_list(struct list *dest, const struct list *src)
{
   struct list *cur = src->next;
   int count = 0;

   while (cur != NULL)
   {
      if (cur->str != NULL)
      {
         count += list_remove_item(dest, cur->str);
      }
      cur = cur->next;
   }

   return count;
}


/*********************************************************************
 *
 * Function    :  list_duplicate
 *
 * Description :  Duplicate a string list
 *
 * Parameters  :
 *          1  :  dest = pointer to destination for copy.  Caller allocs.
 *          2  :  src = pointer to source for copy.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void list_duplicate(struct list *dest, const struct list *src)
{
   struct list * cur_src = src->next;
   struct list * cur_dest = dest;

   memset(dest, '\0', sizeof(*dest));

   while (cur_src)
   {
      cur_dest = cur_dest->next = (struct list *)zalloc(sizeof(*cur_dest));
      if (cur_dest == NULL)
      {
         return;
      }
      cur_dest->str = strdup(cur_src->str);
      cur_src = cur_src->next;
   }

   dest->last = cur_dest;

}


/*********************************************************************
 *
 * Function    :  list_append_list_unique
 *
 * Description :  Append a string list to another list.
 *                Duplicate items are not added.
 *
 * Parameters  :
 *          1  :  dest = pointer to destination for merge.  Caller allocs.
 *          2  :  src = pointer to source for merge.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void list_append_list_unique(struct list *dest, const struct list *src)
{
   struct list * cur = src->next;

   while (cur)
   {
      enlist_unique(dest, cur->str, 0);
      cur = cur->next;
   }
}


/*********************************************************************
 *
 * Function    :  map
 *
 * Description :  Add a mapping from given name to given value to a
 *                given map.
 *
 *                Note: Since all strings will be free()d in free_map()
 *                      later, use the copy flags for constants or
 *                      strings that will be independantly free()d.
 *
 * Parameters  :
 *          1  :  the_map = map to add to
 *          2  :  name = name to add
 *          3  :  nc = flag set if a copy of name should be used
 *          4  :  value = value to add
 *          5  :  vc = flag set if a copy of value should be used
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void map(struct map *the_map, const char *name, int nc, const char *value, int vc)
{
   struct map_entry *new_entry;

   if (NULL == (new_entry = zalloc(sizeof(*new_entry))))
   {
      return;
   }

   new_entry->name  = nc ? strdup(name) : name;
   new_entry->value = vc ? strdup(value) : value;
   /* new_entry->next = NULL;  - implied by zalloc */

   if (the_map->last)
   {
      the_map->last = the_map->last->next = new_entry;
   }
   else
   {
      the_map->last = the_map->first = new_entry;
   }

}


/*********************************************************************
 *
 * Function    :  lookup
 *
 * Description :  Look up an item with a given name in a map, and
 *                return its value
 *
 * Parameters  :
 *          1  :  name = name parameter to look for
 *
 * Returns     :  the value if found, else the empty string
 *
 *********************************************************************/
const char *lookup(const struct map *the_map, const char *name)
{
   const struct map_entry *cur_entry = the_map->first;

   while (cur_entry)
   {
      if (!strcmp(name, cur_entry->name))
      {
         return cur_entry->value;
      }
      cur_entry = cur_entry->next;
   }
   return "";
}


/*********************************************************************
 *
 * Function    :  new_nap
 *
 * Description :  Create a new, empty map.
 *
 * Parameters  :
 *
 * Returns     :  A new, empty map, or NULL if out of memory.
 *
 *********************************************************************/
struct map *new_map(void)
{
   return (struct map *) zalloc(sizeof(struct map));
}


/*********************************************************************
 *
 * Function    :  free_map
 *
 * Description :  Free the memory occupied by a map and its
 *                depandant strings
 *
 * Parameters  :
 *          1  :  cur_entry = map to be freed.  May be NULL.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void free_map(struct map *the_map)
{
   struct map_entry *cur_entry;
   struct map_entry *next_entry;

   if (the_map == NULL)
   {
      return;
   }

   for (cur_entry = the_map->first; cur_entry != NULL; cur_entry = next_entry) 
   {
      freez((char *)cur_entry->name);
      freez((char *)cur_entry->value);

      next_entry = cur_entry->next;
      free(cur_entry);
   }

   the_map->first = the_map->last = NULL;

   free(the_map);
}


/*
  Local Variables:
  tab-width: 3
  end:
*/
