const char ssplit_rcs[] = "$Id: ssplit.c,v 1.1 2001/05/13 21:57:07 administrator Exp $";
/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/ssplit.c,v $
 *
 * Purpose     :  A function to split a string at specified deliminters.
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
 *    $Log: ssplit.c,v $
 *
 *********************************************************************/


#include "config.h"

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <malloc.h>
#endif

#include "ssplit.h"
#include "miscutil.h"

const char ssplit_h_rcs[] = SSPLIT_H_VERSION;

/* Define this for lots of debugging information to stdout */
/* #define SSPLIT_VERBOSE */

#ifdef SSPLIT_VERBOSE
/*********************************************************************
 *
 * Function    :  print
 *
 * Description :  Debugging routine to spit info on stdout.  Not very
 *                useful to the non-console based IJB compiles.
 *
 * Parameters  :
 *          1  :  v = an array of strings
 *          2  :  n = number of strings in `v' to dump to stdout
 *
 * Returns     :  N/A
 *
 *********************************************************************/
static void print(char **v, int n)
{
   int i;
   printf("dump %d strings\n", n);
   for (i=0; i < n; i++)
   {
      printf("%d '%s'\n", i, v[i]);
   }

}
#endif /* def SSPLIT_VERBOSE */


/*********************************************************************
 *
 * Function    :  ssplit
 *
 * Description :  Split a string using deliminters in `c'.  Results go
 *                into `v'.
 *
 * Parameters  :
 *          1  :  s = string to split
 *          2  :  c = array of delimiters
 *          3  :  v[] = results vector (aka. array)
 *          4  :  n = number of usable slots in the vector (aka. array size)
 *          5  :  m = consecutive delimiters means multiple fields?
 *          6  :  l = ignore leading field separators?
 *
 * Returns     :  -1 => failure, else the number of fields put in `v'.
 *
 *********************************************************************/
int ssplit(char *s, char *c, char *v[], int n, int m, int l)
{
   char t[256];
   char **x = NULL;
   int xsize = 0;
   unsigned char *p, b;
   int xi = 0;
   int vi = 0;
   int i;
   int last_was_null;

   if (!s)
   {
      return(-1);
   }

   memset(t, '\0', sizeof(t));

   p = (unsigned char *) c;

   if (!p)
   {
      p = (unsigned char *) " \t";  /* default field separators */
   }

   while (*p)
   {
      t[*p++] = 1;   /* separator  */
   }

   t['\0'] = 2;   /* terminator */
   t['\n'] = 2;   /* terminator */

   p = (unsigned char *) s;

   if (l)/* are we to skip leading separators ? */
   {
      while ((b = t[*p]) != 2)
      {
         if (b != 1)
         {
            break;
         }
         p++;
      }
   }

   xsize = 256;

   x = (char **) zalloc((xsize) * sizeof(char *));

   x[xi++] = (char *) p;   /* first pointer is the beginning of string */

   /* first pass:  save pointers to the field separators */
   while ((b = t[*p]) != 2)
   {
      if (b == 1)    /* if the char is a separator ... */
      {
         *p++ = '\0';      /* null terminate the substring */

         if (xi == xsize)
         {
            /* get another chunk */
            int new_xsize = xsize + 256;
            char **new_x = (char **)zalloc((new_xsize) * sizeof(char *));

            for (i=0; i < xsize; i++)
            {
               new_x[i] = x[i];
            }

            free(x);
            xsize = new_xsize;
            x     = new_x;
         }
         x[xi++] = (char *) p;   /* save pointer to beginning of next string */
      }
      else
      {
         p++;
      }
   }
   *p = '\0';     /* null terminate the substring */


#ifdef SSPLIT_VERBOSE
   if (DEBUG(HDR))
   {
      print(x, xi); /* debugging */
   }
#endif /* def SSPLIT_VERBOSE */


   /* second pass: copy the relevant pointers to the output vector */
   last_was_null = 0;
   for (i=0 ; i < xi; i++)
   {
      if (m)
      {
         /* there are NO null fields */
         if (*x[i] == 0)
         {
            continue;
         }
      }
      if (vi < n)
      {
         v[vi++] = x[i];
      }
      else
      {
         free(x);
         return(-1); /* overflow */
      }
   }
   free(x);

#ifdef SSPLIT_VERBOSE
   if (DEBUG(HDR))
   {
      print(v, vi); /* debugging  */
   }
#endif /* def SSPLIT_VERBOSE */

   return(vi);

}


/*
  Local Variables:
  tab-width: 3
  end:
*/
