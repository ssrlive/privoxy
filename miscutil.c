const char miscutil_rcs[] = "$Id: miscutil.c,v 1.8 2001/06/05 22:32:01 jongfoster Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/miscutil.c,v $
 *
 * Purpose     :  zalloc, hash_string, safe_strerror, strcmpic,
 *                strncmpic, strsav, chomp, and MinGW32 strdup
 *                functions. 
 *                These are each too small to deserve their own file
 *                but don't really fit in any other file.
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
 *    Revision 1.8  2001/06/05 22:32:01  jongfoster
 *    New function make_path() to splice directory and file names together.
 *
 *    Revision 1.7  2001/06/03 19:12:30  oes
 *    introduced bindup()
 *
 *    Revision 1.7  2001/06/03 11:03:48  oes
 *    Makefile/in
 *
 *    introduced cgi.c
 *
 *    actions.c:
 *
 *    adapted to new enlist_unique arg format
 *
 *    conf loadcfg.c
 *
 *    introduced confdir option
 *
 *    filters.c filtrers.h
 *
 *     extracted-CGI relevant stuff
 *
 *    jbsockets.c
 *
 *     filled comment
 *
 *    jcc.c
 *
 *     support for new cgi mechansim
 *
 *    list.c list.h
 *
 *    functions for new list type: "map"
 *    extended enlist_unique
 *
 *    miscutil.c .h
 *    introduced bindup()
 *
 *    parsers.c parsers.h
 *
 *    deleted const struct interceptors
 *
 *    pcrs.c
 *    added FIXME
 *
 *    project.h
 *
 *    added struct map
 *    added struct http_response
 *    changes struct interceptors to struct cgi_dispatcher
 *    moved HTML stuff to cgi.h
 *
 *    re_filterfile:
 *
 *    changed
 *
 *    showargs.c
 *    NO TIME LEFT
 *
 *    Revision 1.6  2001/06/01 18:14:49  jongfoster
 *    Changing the calls to strerr() to check HAVE_STRERR (which is defined
 *    in config.h if appropriate) rather than the NO_STRERR macro.
 *
 *    Revision 1.5  2001/06/01 10:31:51  oes
 *    Added character class matching to trivimatch; renamed to simplematch
 *
 *    Revision 1.4  2001/05/31 17:32:31  oes
 *
 *     - Enhanced domain part globbing with infix and prefix asterisk
 *       matching and optional unanchored operation
 *
 *    Revision 1.3  2001/05/29 23:10:09  oes
 *
 *
 *     - Introduced chomp()
 *     - Moved strsav() from showargs to miscutil
 *
 *    Revision 1.2  2001/05/29 09:50:24  jongfoster
 *    Unified blocklist/imagelist/permissionslist.
 *    File format is still under discussion, but the internal changes
 *    are (mostly) done.
 *
 *    Also modified interceptor behaviour:
 *    - We now intercept all URLs beginning with one of the following
 *      prefixes (and *only* these prefixes):
 *        * http://i.j.b/
 *        * http://ijbswa.sf.net/config/
 *        * http://ijbswa.sourceforge.net/config/
 *    - New interceptors "home page" - go to http://i.j.b/ to see it.
 *    - Internal changes so that intercepted and fast redirect pages
 *      are not replaced with an image.
 *    - Interceptors now have the option to send a binary page direct
 *      to the client. (i.e. ijb-send-banner uses this)
 *    - Implemented show-url-info interceptor.  (Which is why I needed
 *      the above interceptors changes - a typical URL is
 *      "http://i.j.b/show-url-info?url=www.somesite.com/banner.gif".
 *      The previous mechanism would not have intercepted that, and
 *      if it had been intercepted then it then it would have replaced
 *      it with an image.)
 *
 *    Revision 1.1.1.1  2001/05/15 13:59:00  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ctype.h>

#include "miscutil.h"
#include "errlog.h"

const char miscutil_h_rcs[] = MISCUTIL_H_VERSION;

/* Fix a problem with Solaris.  There should be no effect on other
 * platforms.
 * Solaris's isspace() is a macro which uses it's argument directly
 * as an array index.  Therefore we need to make sure that high-bit
 * characters generate +ve values, and ideally we also want to make
 * the argument match the declared parameter type of "int".
 */
#define ijb_tolower(__X) tolower((int)(unsigned char)(__X))
#define ijb_isspace(__X) isspace((int)(unsigned char)(__X))   

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


#ifdef HAVE_STRERROR
   s = strerror(err);
#endif /* HAVE_STRERROR */

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
int strcmpic(const char *s1, const char *s2)
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
int strncmpic(const char *s1, const char *s2, size_t n)
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


/*********************************************************************
 *
 * Function    :  chomp
 *
 * Description :  In-situ-eliminate all leading and trailing whitespace
 *                from a string.
 *
 * Parameters  :
 *          1  :  s : string to be chomped.
 *
 * Returns     :  chomped string
 *
 *********************************************************************/
char *chomp(char *string)
{
   char *p, *q, *r;

   /* 
    * strip trailing whitespace
    */
   p = string + strlen(string);
   while (p > string && ijb_isspace(*(p-1)))
   {
      p--;
   }
   *p = '\0';

   /* 
    * find end of leading whitespace 
    */
   q = r = string;
   while (*q && ijb_isspace(*q))
   {
      q++;
   }

   /*
    * if there was any, move the rest forwards
    */
   if (q != string)
   {
      while (q <= p)
      {
         *r++ = *q++;
      }
   }

   return(string);

}

/*********************************************************************
 *
 * Function    :  strsav
 *
 * Description :  Reallocate "old" and append text to it.  This makes
 *                it easier to append to malloc'd strings.
 *
 * Parameters  :
 *          1  :  old = Old text that is to be extended.  Will be
 *                free()d by this routine.
 *          2  :  text_to_append = Text to be appended to old.
 *
 * Returns     :  Pointer to newly malloc'ed appended string.
 *                If there is no text to append, return old.  Caller
 *                must free().
 *
 *********************************************************************/
char *strsav(char *old, const char *text_to_append)
{
   int old_len, new_len;
   char *p;

   if (( text_to_append == NULL) || (*text_to_append == '\0'))
   {
      return(old);
   }

   if (NULL != old)
   {
      old_len = strlen(old);
   }
   else
   {
      old_len = 0;
   }

   new_len = old_len + strlen(text_to_append) + 1;

   if (old)
   {
      if ((p = realloc(old, new_len)) == NULL)
      {
         log_error(LOG_LEVEL_FATAL, "realloc(%d) bytes failed!", new_len);
         /* Never get here - LOG_LEVEL_FATAL causes program exit */
      }
   }
   else
   {
      if ((p = (char *)malloc(new_len)) == NULL)
      {
         log_error(LOG_LEVEL_FATAL, "malloc(%d) bytes failed!", new_len);
         /* Never get here - LOG_LEVEL_FATAL causes program exit */
      }
   }

   strcpy(p + old_len, text_to_append);
   return(p);

}


/*********************************************************************
 *
 * Function    :  simplematch
 *
 * Description :  String matching, with a (greedy) '*' wildcard that
 *                stands for zero or more arbitrary characters and
 *                character classes in [], which take both enumerations
 *                and ranges.
 *
 * Parameters  :
 *          1  :  pattern = pattern for matching
 *          2  :  text    = text to be matched
 *
 * Returns     :  0 if match, else nonzero
 *
 *********************************************************************/
int simplematch(char *pattern, char *text)
{
  char *fallback; 
  char *pat = pattern;
  char *txt = text;
  int wildcard = 0;
  
  char lastchar = 'a';
  unsigned i;
  unsigned char charmap[32];
  
  
   while (*txt)
   {

      /* EOF pattern but !EOF text? */
      if (*pat == '\0')
      {
         return 1;
      }

      /* '*' in the pattern?  */
      if (*pat == '*') 
      {
     
         /* The pattern ends afterwards? Speed up the return. */
         if (*++pat == '\0')
         {
            return 0;
         }
     
         /* Else, set wildcard mode and remember position after '*' */
         wildcard = 1;
         fallback = pat;
      }

      /* Character range specification? */
      if (*pat == '[')
      {
         memset(charmap, '\0', sizeof(charmap));

         while (*++pat != ']')
         {
            if (!*pat)
            { 
               return 1;
            }
            else if (*pat == '-')
            {
               if ((*++pat == ']') || *pat == '\0')
               {
                  return(1);
               }
               for(i = lastchar; i <= *pat; i++)
               {
                  charmap[i / 8] |= (1 << (i % 8));
               } 
            }
            else
            {
               charmap[*pat / 8] |= (1 << (*pat % 8));
               lastchar = *pat;
            }
         }
      } /* -END- if Character range specification */


      /* Compare: Char match, or char range match*/
      if ((*pat == *txt)  
      || ((*pat == ']') && (charmap[*txt / 8] & (1 << (*txt % 8)))) )
      {
         /* Sucess, go ahead */
         pat++;
      }
      else
      {
         /* In wildcard mode, just try again after failiure */
         if(wildcard)
         {
            pat = fallback;
         }

         /* Else, bad luck */
         else
         {
            return 1;
         }
      }
      txt++;
   }

   /* Cut off extra '*'s */
   if(*pat == '*')  pat++;

   /* If this is the pattern's end, fine! */
   return(*pat);

}


/*********************************************************************
 *
 * Function    :  bindup
 *
 * Description :  Duplicate the first n characters of a string that may
 *                contain '\0' characters.
 *
 * Parameters  :
 *          1  :  string = string to be duplicated
 *          2  :  n = number of bytes to duplicate
 *
 * Returns     :  pointer to copy, or NULL if failiure
 *
 *********************************************************************/
char *bindup(const char *string, int n)
{
   char *dup;

   if (NULL == (dup = (char *)malloc(n)))
   {
	   return NULL;
	}
   else
	{
	  memcpy(dup, string, n);
	}

   return dup;

}


/*********************************************************************
 *
 * Function    :  make_path
 *
 * Description :  Takes a directory name and a file name, returns 
 *                the complete path.  Handles windows/unix differences.
 *                If the file name is already an absolute path, or if
 *                the directory name is NULL or empty, it returns 
 *                the filename. 
 *
 * Parameters  :
 *          1  :  dir: Name of directory or NULL for none.
 *          2  :  file: Name of file.  Should not be NULL or empty.
 *
 * Returns     :  "dir/file" (Or on windows, "dir\file").
 *                It allocates the string on the heap.  Caller frees.
 *                Returns NULL in error (i.e. NULL file or out of
 *                memory) 
 *
 *********************************************************************/
char * make_path(const char * dir, const char * file)
{
   if ((file == NULL) || (*file == '\0'))
   {
      return NULL; /* Error */
   }

   if ((dir == NULL) || (*dir == '\0') /* No directory specified */
#ifdef _WIN32
      || (*file == '\\') || (file[1] == ':') /* Absolute path (DOS) */
#else /* ifndef _WIN32 */
      || (*file == '/') /* Absolute path (U*ix) */
#endif /* ifndef _WIN32 */
      )
   {
      return strdup(file);
   }
   else
   {
      char * path = malloc(strlen(dir) + strlen(file) + 2);
      strcpy(path, dir);
#ifdef _WIN32
      strcat(path, "\\");
#else /* ifndef _WIN32 */
      strcat(path, "/");
#endif /* ifndef _WIN32 */
      strcat(path, file);

      return path;
   }
}


/*
  Local Variables:
  tab-width: 3
  end:
*/
