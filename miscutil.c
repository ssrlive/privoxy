const char miscutil_rcs[] = "$Id: miscutil.c,v 1.1 2001/05/13 21:57:06 administrator Exp $";
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/miscutil.c,v $
 *
 * Purpose     :  zalloc, hash_string, safe_strerror, strcmpic,
 *                strncmpic, and MinGW32 strdup functions.  These are
 *                each too small to deserve their own file but don't 
 *                really fit in any other file.
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
 *    $Log: miscutil.c,v $
 *
 *********************************************************************/


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#include "miscutil.h"

const char miscutil_h_rcs[] = MISCUTIL_H_VERSION;

/* Fix a problem with Solaris.  There should be no effect on other
 * platforms.
 * Solaris's isspace() is a macro which uses it's argument directly
 * as an array index.  Therefore we need to make sure that high-bit
 * characters generate +ve values, and ideally we also want to make
 * the argument match the declared parameter type of "int".
 */
#define ijb_tolower(__X) tolower((int)(unsigned char)(__X))

/*********************************************************************
 *
 * Function    :  zalloc
 *
 * Description :  Malloc some memory and set it to '\0'.
 *                The way calloc() ought to be -acjc
 *
 * Parameters  :
 *          1  :  size = Size of memory chunk to return.
 *
 * Returns     :  Pointer to newly malloc'd memory chunk.
 *
 *********************************************************************/
void *zalloc(int size)
{
   void * ret;

   if ((ret = (void *)malloc(size)) != NULL)
   {
      memset(ret, 0, size);
   }

   return(ret);
}


/*********************************************************************
 *
 * Function    :  hash_string
 *
 * Description :  Take a string and compute a (hopefuly) unique numeric
 *                integer value.  This has several uses, but being able
 *                to "switch" a string the one of my favorites.
 *
 * Parameters  :
 *          1  :  s : string to be hashed.
 *
 * Returns     :  an unsigned long variable with the hashed value.
 *
 *********************************************************************/
unsigned long hash_string( const char* s )
{
   unsigned long h = 0ul; 

   for ( ; *s; ++s )
   {
      h = 5 * h + *s;
   }

   return (h);

}


#ifdef __MINGW32__
/*********************************************************************
 *
 * Function    :  strdup
 *
 * Description :  For some reason (which is beyond me), gcc and WIN32
 *                don't like strdup.  When a "free" is executed on a
 *                strdup'd ptr, it can at times freez up!  So I just
 *                replaced it and problem was solved.
 *
 * Parameters  :
 *          1  :  s = string to duplicate
 *
 * Returns     :  Pointer to newly malloc'ed copy of the string.
 *
 *********************************************************************/
char *strdup( const char *s )
{
   char * result = (char *)malloc( strlen(s)+1 );

   if (result != NULL)
   {
      strcpy( result, s );
   }

   return( result );
}

#endif /* def __MINGW32__ */



/*********************************************************************
 *
 * Function    :  safe_strerror
 *
 * Description :  Variant of the library routine strerror() which will
 *                work on systems without the library routine, and
 *                which should never return NULL.
 *
 * Parameters  :
 *          1  :  err = the `errno' of the last operation.
 *
 * Returns     :  An "English" string of the last `errno'.  Allocated
 *                with strdup(), so caller frees.  May be NULL if the
 *                system is out of memory.
 *
 *********************************************************************/
char *safe_strerror(int err)
{
   char *s = NULL;
   char buf[BUFSIZ];


#ifndef NOSTRERROR
   s = strerror(err);
#endif /* NOSTRERROR */

   if (s == NULL)
   {
      sprintf(buf, "(errno = %d)", err);
      s = buf;
   }

   return(strdup(s));

}


/*********************************************************************
 *
 * Function    :  strcmpic
 *
 * Description :  Case insensitive string comparison
 *
 * Parameters  :
 *          1  :  s1 = string 1 to compare
 *          2  :  s2 = string 2 to compare
 *
 * Returns     :  0 if s1==s2, Negative if s1<s2, Positive if s1>s2
 *
 *********************************************************************/
int strcmpic(char *s1, char *s2)
{
   while (*s1 && *s2)
   {
      if ( ( *s1 != *s2 ) && ( ijb_tolower(*s1) != ijb_tolower(*s2) ) )
      {
         break;
      }
      s1++, s2++;
   }
   return(ijb_tolower(*s1) - ijb_tolower(*s2));

}


/*********************************************************************
 *
 * Function    :  strncmpic
 *
 * Description :  Case insensitive string comparison (upto n characters)
 *
 * Parameters  :
 *          1  :  s1 = string 1 to compare
 *          2  :  s2 = string 2 to compare
 *          3  :  n = maximum characters to compare
 *
 * Returns     :  0 if s1==s2, Negative if s1<s2, Positive if s1>s2
 *
 *********************************************************************/
int strncmpic(char *s1, char *s2, size_t n)
{
   if (n <= 0) return(0);

   while (*s1 && *s2)
   {
      if ( ( *s1 != *s2 ) && ( ijb_tolower(*s1) != ijb_tolower(*s2) ) )
      {
         break;
      }

      if (--n <= 0) break;

      s1++, s2++;
   }
   return(ijb_tolower(*s1) - ijb_tolower(*s2));

}


/*
  Local Variables:
  tab-width: 3
  end:
*/
