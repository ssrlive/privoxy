const char pcrs_rcs[] = "$Id: pcrs.c,v 1.6 2001/06/03 11:03:48 oes Exp $";

/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/pcrs.c,v $
 *
 * Purpose     :  This is the alpha release of libpcrs. It is only published
 *                at this early stage of development, because it is
 *                needed for a new feature in JunkBuster.
 *
 *                While no inconsistencies, memory leaks or functional bugs
 *                are known at this time, there *could* be plenty ;-). Also,
 *                Many pcre-specific options are not yet supported, and
 *                error handling needs improvement.
 *
 *                pcrs is a supplement to the brilliant pcre library by Philip
 *                Hazel (ph10@cam.ac.uk) and adds Perl-style substitution. That
 *                is, it mimics Perl's 's' operator.
 *
 *                Currently, there's no documentation besides comments and the
 *                source itself ;-)
 *
 * Copyright   :  Written and Copyright (C) 2000 by Andreas Oesterhelt
 *                <andreas@oesterhelt.org>
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
 *    $Log: pcrs.c,v $
 *    Revision 1.6  2001/06/03 11:03:48  oes
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
 *    Revision 1.5  2001/05/29 09:50:24  jongfoster
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
 *    Revision 1.4  2001/05/25 14:12:40  oes
 *    Fixed bug: Empty substitutes now detected
 *
 *    Revision 1.3  2001/05/25 11:03:55  oes
 *    Added sanity check for NULL jobs to pcrs_exec_substitution
 *
 *    Revision 1.2  2001/05/22 18:46:04  oes
 *
 *    - Enabled filtering banners by size rather than URL
 *      by adding patterns that replace all standard banner
 *      sizes with the "Junkbuster" gif to the re_filterfile
 *
 *    - Enabled filtering WebBugs by providing a pattern
 *      which kills all 1x1 images
 *
 *    - Added support for PCRE_UNGREEDY behaviour to pcrs,
 *      which is selected by the (nonstandard and therefore
 *      capital) letter 'U' in the option string.
 *      It causes the quantifiers to be ungreedy by default.
 *      Appending a ? turns back to greedy (!).
 *
 *    - Added a new interceptor ijb-send-banner, which
 *      sends back the "Junkbuster" gif. Without imagelist or
 *      MSIE detection support, or if tinygif = 1, or the
 *      URL isn't recognized as an imageurl, a lame HTML
 *      explanation is sent instead.
 *
 *    - Added new feature, which permits blocking remote
 *      script redirects and firing back a local redirect
 *      to the browser.
 *      The feature is conditionally compiled, i.e. it
 *      can be disabled with --disable-fast-redirects,
 *      plus it must be activated by a "fast-redirects"
 *      line in the config file, has its own log level
 *      and of course wants to be displayed by show-proxy-args
 *      Note: Boy, all the #ifdefs in 1001 locations and
 *      all the fumbling with configure.in and acconfig.h
 *      were *way* more work than the feature itself :-(
 *
 *    - Because a generic redirect template was needed for
 *      this, tinygif = 3 now uses the same.
 *
 *    - Moved GIFs, and other static HTTP response templates
 *      to project.h
 *
 *    - Some minor fixes
 *
 *    - Removed some >400 CRs again (Jon, you really worked
 *      a lot! ;-)
 *
 *    Revision 1.1.1.1  2001/05/15 13:59:02  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


#include <pcre.h>
#include <string.h>
#include "pcrs.h"
const char pcrs_h_rcs[] = PCRS_H_VERSION;


/*********************************************************************
 *
 * Function    :  my_strsep
 *
 * Description :  Convenience function. It acts like strsep, except that
 *                it respects quoting of the delimiter character with the
 *                quote character. (And, of course, quoting the quote char
 *                with itself.)  Called from `pcrs_make_job'.
 *
 * Parameters  :
 *          1  :  token = current token
 *          2  :  text = string to tokenize
 *          3  :  delimiter = single character deliminter
 *          4  :  quote_char = character to cause quoting
 *
 * Returns     :  -1 => failure, else the length of the token found.
 *                In the latter case, *text is the token's start.
 *
 *********************************************************************/
int my_strsep(char *token, char **text, char delimiter, char quote_char)
{
   int i, k=0, limit, quoted = FALSE;

   limit = strlen(*text);
   if ( 0 == limit )
   {
      return -1;
   }

   token[0] = '\0';

   for (i=0; i < limit; i++)
   {
      if (text[0][i] == delimiter && !quoted)
      {
         *text += 1;
         break;
      }
      else if (text[0][i] == quote_char && !quoted && i+1 < limit && text[0][i+1] == delimiter)
      {
         quoted = TRUE;
         continue;
      }
      token[k++] = text[0][i];
      quoted = FALSE;
   }
   token[k] = '\0';
   *text += i;
   return k;

}


/*********************************************************************
 *
 * Function    :  pcrs_compile_perl_options
 *
 * Description :  This function parses a string containing the options to
 *                Perl's s/// operator. It returns an integer that is the
 *                pcre equivalent of the symbolic optstring.
 *                Since pcre doesn't know about Perl's 'g' (global) option,
 *                but pcrs needs it, the globalflag integer is set if 'g'
 *                is encountered.
 *
 * Parameters  :
 *          1  :  optstring = string with options in perl syntax
 *          2  :  globalflag = see description
 *
 * Returns     :  option integer suitable for pcre 
 *
 *********************************************************************/
int pcrs_compile_perl_options(char *optstring, int *globalflag)
{
   size_t i;
   int rc = 0;
   *globalflag = 0;
   for (i=0; i < strlen(optstring); i++)
   {
      switch(optstring[i])
      {
         case 'e': break;
         case 'g': *globalflag = 1; break;
         case 'i': rc |= PCRE_CASELESS; break;
         case 'm': rc |= PCRE_MULTILINE; break;
         case 'o': break;
         case 's': rc |= PCRE_DOTALL; break;
         case 'x': rc |= PCRE_EXTENDED; break;
         case 'U': rc |= PCRE_UNGREEDY; break;
         default:  break;
      }
   }
   return rc;

}


/*********************************************************************
 *
 * Function    :  pcrs_compile_replacement
 *
 * Description :  This function takes a Perl-style replacement (2nd argument
 *                to the s/// operator and returns a compiled pcrs_substitute,
 *                or NULL if memory allocation for the substitute structure
 *                fails.
 *
 * Parameters  :
 *          1  :  replacement = replacement part of s/// operator
 *                              in perl syntax
 *          2  :  errptr = pointer to an integer in which error
 *                         conditions can be returned.
 *
 * Returns     :  pcrs_substitute data structure, or NULL if an
 *                error is encountered. In that case, *errptr has
 *                the reason.
 *
 *********************************************************************/
pcrs_substitute *pcrs_compile_replacement(char *replacement, int *errptr)
{
   int length, i, k = 0, l = 0, quoted = 0, idx;
   char *text, *num_ptr, *numbers = "0123456789";
   pcrs_substitute *r;

   r = (pcrs_substitute *)malloc(sizeof(pcrs_substitute));
   if (r == NULL) return NULL;
   memset(r, '\0', sizeof(pcrs_substitute));

   text = strdup(replacement);      /* must be free()d by caller */
   if (text  == NULL)
   {
      *errptr = PCRS_ERR_NOMEM;
      free(r);
      return NULL;
   }

   length = strlen(replacement);

   for (i=0; i < length; i++)
   {
      /* Backslash treatment */
      if (replacement[i] == '\\')
      {
         if (quoted)
         {
            text[k++] = replacement[i];
            quoted = 0;
         }
         else
         {
            quoted = 1;
         }
         continue;
      }

      /* Dollar treatment */
      if (replacement[i] == '$' && !quoted && i < length - 1)
      {
         if (strchr("0123456789&", replacement[i + 1]) == NULL)
         {
            text[k++] = replacement[i];
         }
         else
         {
            r->block_length[l] = k - r->block_offset[l];
            r->backref[l] = 0;
            if (replacement[i + 1] != '&')
            {
               while ((num_ptr = strchr(numbers, replacement[++i])) != NULL && i < length)
               {
                  idx = num_ptr - numbers;
                  r->backref[l] = r->backref[l] * 10 + idx;
               }
               i--;
            }
            else
               i++;
            if (r->backref[l] < PCRS_MAX_SUBMATCHES)
               r->backref_count[r->backref[l]] += 1;
            l++;
            r->block_offset[l] = k;
         }
         continue;
      }

      /* Plain char treatment */
      text[k++] = replacement[i];
      quoted = 0;
   }

   text[k] = '\0';
   r->text = text;
   r->backrefs = l;
   r->block_length[l] = k - r->block_offset[l];
   return r;

}


/*********************************************************************
 *
 * Function    :  pcrs_free_job
 *
 * Description :  Frees the memory used by a pcrs_job struct and its
 *                dependant structures. Returns a pointer to the next
 *                job, if there was any, or NULL otherwise.
 *
 * Parameters  :
 *          1  :  job = pointer to the pcrs_job structure to be freed
 *
 * Returns     : a pointer to the next job, if there was any, or
 *               NULL otherwise. 
 *
 *********************************************************************/
pcrs_job *pcrs_free_job(pcrs_job *job)
{
   pcrs_job *next;

   if (job == NULL)
   {
      return NULL;
   }
   else
   {
      next = job->next;
      if (job->pattern != NULL) free(job->pattern);
      if (job->hints != NULL) free(job->hints);
      if (job->substitute != NULL)
      {
         if (job->substitute->text != NULL) free(job->substitute->text);
         free(job->substitute);
      }
      free(job);
   }
   return next;

}


/*********************************************************************
 *
 * Function    :  pcrs_make_job
 *
 * Description :  Main entry point. Takes a string with a Perl-style
 *                s/// command and returns a corresponding pcrs_job,
 *                or NULL if compiling the job fails at any stage.
 *                Diagnostics could obviously be improved.
 *
 * Parameters  :
 *          1  :  command = string with perl-style s/// command
 *          2  :  errptr = pointer to an integer in which error
 *                         conditions can be returned.
 *
 * Returns     :  a corresponding pcrs_job data structure, or NULL
 *                if an error was encountered. In that case, *errptr
 *                has the reason.
 *
 *********************************************************************/
pcrs_job *pcrs_make_job(char *command, int *errptr)
{
   char *dummy, *token, delimiter;
   const char *error;
   int i = 0, globalflag;
   pcrs_job *newjob;

   /* Get and init memory */
   if ((newjob = (pcrs_job *)malloc(sizeof(pcrs_job))) == NULL)
   {
      *errptr = PCRS_ERR_NOMEM;
      return NULL;
   }
   memset(newjob, '\0', sizeof(pcrs_job));

   /* Command too short? */
   if (strlen(command) < 4)
   {
      *errptr = PCRS_ERR_CMDSYNTAX;
      pcrs_free_job(newjob);
      return NULL;
   }

   /* Split command into tokens and handle them */
   delimiter = command[1];
   token = (char *)malloc(strlen(command)); /* current token */
   dummy = (char *)malloc(strlen(command)); /* must store pattern, since we can't */
                                            /* use it until the options are known */
   while (my_strsep(token, &command, delimiter, '\\') >= 0)
   {
      switch (i)
      {
         /* We don't care about the command and assume 's' */
         case 0:
            break;

         /* The pattern */
         case 1:
            strcpy(dummy, token);
            break;

         /* The substitute */
         case 2:
            if ((newjob->substitute = pcrs_compile_replacement(token, errptr)) == NULL)
            {
               pcrs_free_job(newjob);
               return NULL;
            }
            else
            {
               break;
            }

         /* The options */
         case 3:
            newjob->options = pcrs_compile_perl_options(token, &globalflag);
            newjob->globalflag = globalflag;
            break;

         /* There shouldn't be anything else! */
         default:
            *errptr = PCRS_ERR_CMDSYNTAX;
            pcrs_free_job(newjob);
            return NULL;
      }
      i++;
   }
   free(token);

   /* We have a valid substitute? */
   if (newjob->substitute == NULL)
   {
      *errptr = PCRS_ERR_CMDSYNTAX;
      pcrs_free_job(newjob);
      return NULL;
   }

   /* Compile the pattern */
   newjob->pattern = pcre_compile(dummy, newjob->options, &error, errptr, NULL);
   if (newjob->pattern == NULL)
   {
      pcrs_free_job(newjob);
      return NULL;
   }
   free(dummy);

   /*
    * Generate hints. This has little overhead, since the
    * hints will be NULL for a boring pattern anyway.
    */
   newjob->hints = pcre_study(newjob->pattern, 0, &error);
   if (error != NULL)
   {
      *errptr = PCRS_ERR_STUDY;
      pcrs_free_job(newjob);
      return NULL;
   }

   return newjob;

}


/*********************************************************************
 *
 * Function    :  create_pcrs_job
 *
 * Description :  Create a job from all its components, if you don't
 *                have a Perl command to start from. Rather boring.
 *
 * Parameters  :
 *          1  :  pattern = pointer to pcre pattern
 *          2  :  hints = pointer to pcre hints
 *          3  :  options = options in pcre format
 *          4  :  globalflag = flag that indicates if global matching is desired
 *          5  :  substitute = pointer to pcrs_substitute data structure
 *          2  :  errptr = pointer to an integer in which error
 *                         conditions can be returned.
 *
 * Returns     :  pcrs_job structure, or NULL if an error was encountered.
 *                In that case, *errptr has the reason why.
 *
 *********************************************************************/
pcrs_job *create_pcrs_job(pcre *pattern, pcre_extra *hints, int options, int globalflag, pcrs_substitute *substitute, int *errptr)
{
   pcrs_job *newjob;

   if ((newjob = (pcrs_job *)malloc(sizeof(pcrs_job))) == NULL)
   {
      *errptr = PCRS_ERR_NOMEM;
      return NULL;
   }
   memset(newjob, '\0', sizeof(pcrs_job));

   newjob->pattern = pattern;
   newjob->hints = hints;
   newjob->options = options;
   newjob->globalflag = globalflag;
   newjob->substitute = substitute;

   return(newjob);

}


/*********************************************************************
 *
 * Function    :  pcrs_exec_substitution
 *
 * Description :  Modify the subject by executing the regular substitution
 *                defined by the job. Since the result may be longer than
 *                the subject, its space requirements are precalculated in
 *                the matching phase and new memory is allocated accordingly.
 *                It is the caller's responsibility to free the result when
 *                it's no longer needed.
 *
 *                FIXME: MUST HANDLE SUBJECTS THAT ARE LONGER THAN subject_length
                         CORRECTLY! --oes
 *
 * Parameters  :
 *          1  :  job = the pcrs_job to be executed
 *          2  :  subject = the subject (== original) string
 *          3  :  subject_length = the subject's length
 *          4  :  result = char** for returning  the result 
 *          5  :  result_length = int* for returning the result's length
 *
 * Returns     :  the number of substitutions that were made. May be > 1
 *                if job->globalflag was set
 *
 *********************************************************************/
int pcrs_exec_substitution(pcrs_job *job, char *subject, int subject_length, char **result, int *result_length)
{
   int offsets[3 * PCRS_MAX_SUBMATCHES],
      offset = 0, i=0, k, matches_found, newsize, submatches;
   pcrs_match matches[PCRS_MAX_MATCHES];
   char *result_offset;


   /* Sanity first */
   if (job == NULL || job->pattern == NULL || job->substitute == NULL)
   {
      *result = NULL;
      return(PCRS_ERR_BADJOB);
   }

   newsize=subject_length;


   /* Find.. */
   while ((submatches = pcre_exec(job->pattern, job->hints, subject, subject_length, offset, 0, offsets, 99)) > 0)
   {
      matches[i].submatches = submatches;
      for (k=0; k < submatches; k++)
      {
         matches[i].submatch_offset[k] = offsets[2 * k];
         matches[i].submatch_length[k] = offsets[2 * k + 1] - offsets[2 * k]; /* Non-found optional submatches have length -1-(-1)==0 */
         newsize += matches[i].submatch_length[k] * job->substitute->backref_count[k]; /* reserve mem for each submatch as often as it is ref'd */
      }
      newsize += strlen(job->substitute->text) - matches[i].submatch_length[0]; /* plus replacement text size minus match text size */

      /* Non-global search or limit reached? */
      if (++i >= PCRS_MAX_MATCHES || !job->globalflag ) break;

      /* Don't loop on empty matches */
      if (offsets[1] == offset)
         if (offset < subject_length)
            offset++;
         else
            break;
      /* Go find the next one */
      else
         offset = offsets[1];
   }
   if (submatches < -1) return submatches;   /* Pass pcre error through */
   matches_found = i;


   /* ..get memory ..*/
   if ((*result = (char *)malloc(newsize)) == NULL)   /* must be free()d by caller */
   {
      return PCRS_ERR_NOMEM;
   }


   /* ..and replace */
   offset = 0;
   result_offset = *result;

   for (i=0; i < matches_found; i++)
   {
      memcpy(result_offset, subject + offset, matches[i].submatch_offset[0] - offset); /* copy the chunk preceding the match */
      result_offset += matches[i].submatch_offset[0] - offset;

      /* For every segment of the substitute.. */
      for (k=0; k <= job->substitute->backrefs; k++)
      {
         /* ...copy its text.. */
         memcpy(result_offset, job->substitute->text + job->substitute->block_offset[k], job->substitute->block_length[k]);
         result_offset += job->substitute->block_length[k];

         /* ..plus, if it's not the last chunk (i.e.: There IS a backref).. */
         if (k != job->substitute->backrefs
             /* ..and a nonempty match.. */
             && matches[i].submatch_length[job->substitute->backref[k]] > 0
             /* ..and in legal range, ... */
             && job->substitute->backref[k] <= PCRS_MAX_SUBMATCHES)
         {
            /* copy the submatch that is ref'd. */
            memcpy(
               result_offset,
               subject + matches[i].submatch_offset[job->substitute->backref[k]],
               matches[i].submatch_length[job->substitute->backref[k]]
            );
            result_offset += matches[i].submatch_length[job->substitute->backref[k]];
         }
      }
      offset =  matches[i].submatch_offset[0] + matches[i].submatch_length[0];
   }

   /* Copy the rest. */
   memcpy(result_offset, subject + offset, subject_length - offset);

   *result_length = newsize;
   return matches_found;

}


/*
  Local Variables:
  tab-width: 3
  end:
*/
