const char pcrs_rcs[] = "$Id: pcrs.c,v 1.7 2001/06/29 13:33:04 oes Exp $";

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
 *                Short note: I addition to perl's options, 'U' for ungreedy
 *                and 't' for trivial (i.e.: ignore backrefs in the substitute)
 *                are supported.
 *
 * Copyright   :  Written and Copyright (C) 2000, 2001 by Andreas S. Oesterhelt
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
 *    Revision 1.7  2001/06/29 13:33:04  oes
 *    - Cleaned up, renamed and reordered functions,
 *      improved comments
 *    - Removed my_strsep
 *    - Replaced globalflag with a general flags int
 *      that holds PCRS_GLOBAL, PCRS_SUCCESS, and PCRS_TRIVIAL
 *    - Introduced trivial option that will prevent pcrs
 *      from honouring backreferences in the substitute,
 *      which is useful for large substitutes that are
 *      red in from somewhere and saves the pain of escaping
 *      the backrefs
 *    - Introduced convenience function pcrs_free_joblist()
 *    - Split pcrs_make_job() into pcrs_compile(), which still
 *      takes a complete s/// comand as argument and parses it,
 *      and a new function pcrs_make_job, which takes the
 *      three separate components. This should make for a
 *      much friendlier frontend.
 *    - Removed create_pcrs_job() which was useless
 *    - Fixed a bug in pcrs_execute
 *    - Success flag is now handled by pcrs instead of user
 *    - Removed logentry from cancelled commit
 *
 *    Revision 1.6  2001/06/03 19:12:45  oes
 *    added FIXME
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
 * Function    :  pcrs_compile_perl_options
 *
 * Description :  This function parses a string containing the options to
 *                Perl's s/// operator. It returns an integer that is the
 *                pcre equivalent of the symbolic optstring.
 *                Since pcre doesn't know about Perl's 'g' (global) or pcrs',
 *                'T' (trivial) options but pcrs needs them, the corresponding
 *                flags are set if 'g'or 'T' is encountered.
 *                Note: The 'T' and 'U' options do not conform to Perl.
 *             
 * Parameters  :
 *          1  :  optstring = string with options in perl syntax
 *          2  :  flags = see description
 *
 * Returns     :  option integer suitable for pcre 
 *
 *********************************************************************/
int pcrs_compile_perl_options(char *optstring, int *flags)
{
   size_t i;
   int rc = 0;
   *flags = 0;
   for (i=0; i < strlen(optstring); i++)
   {
      switch(optstring[i])
      {
         case 'e': break;
         case 'g': *flags |= PCRS_GLOBAL; break;
         case 'i': rc |= PCRE_CASELESS; break;
         case 'm': rc |= PCRE_MULTILINE; break;
         case 'o': break;
         case 's': rc |= PCRE_DOTALL; break;
         case 'x': rc |= PCRE_EXTENDED; break;
         case 'U': rc |= PCRE_UNGREEDY; break;
   	   case 'T': *flags |= PCRS_TRIVIAL; break;
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
pcrs_substitute *pcrs_compile_replacement(char *replacement, int trivialflag, int *errptr)
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

   if (trivialflag)
   {
       k = length;
   }
   else
   {
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
   } /* -END- if (!trivialflag) */

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
 * Returns     :  a pointer to the next job, if there was any, or
 *                NULL otherwise. 
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
 * Function    :  pcrs_free_joblist
 *
 * Description :  Iterates through a chained list of pcrs_job's and
 *                frees them using pcrs_free_job.
 *
 * Parameters  :
 *          1  :  joblist = pointer to the first pcrs_job structure to
 *                be freed
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void pcrs_free_joblist(pcrs_job *joblist)
{
   while ( NULL != (joblist = pcrs_free_job(joblist)) ) {};

   return;

}


/*********************************************************************
 *
 * Function    :  pcrs_compile
 *
 * Description :  Main entry point. Takes a string with a Perl-style
 *                s/// command and returns a corresponding pcrs_job,
 *                or NULL if compiling the job fails at any stage.
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
pcrs_job *pcrs_compile(char *command, int *errptr)
{
   int i, k, l, limit, quoted = FALSE;
   char delimiter;
   char *tokens[4];   
   pcrs_job *newjob;

   i = k = l = 0;

   /*
    * Tokenize the perl command
    */
   limit = strlen(command);
   if (limit < 4)
   {
      *errptr = PCRS_ERR_CMDSYNTAX;
      return NULL;
   }
   else
   {
     delimiter = command[1];
   }

   tokens[l] = (char *) malloc(limit + 1);

   for (i=0; i <= limit; i++)
   {

      if (command[i] == delimiter && !quoted)
      {
    	   if (l == 3)
   		{
   		   l = -1;
            break;
         }
          tokens[0][k++] = '\0';
         tokens[++l] = tokens[0] + k;
         continue;
      }

      else if (command[i] == '\\' && !quoted && i+1 < limit && command[i+1] == delimiter)
      {
         quoted = TRUE;
         continue;
      }
      tokens[0][k++] = command[i];
      quoted = FALSE;
   }


   /*
    * Syntax error ?
    */
   if (l != 3)
   {
      *errptr = PCRS_ERR_CMDSYNTAX;
      free(tokens[0]);
      return NULL;
   }

   newjob = pcrs_make_job(tokens[1], tokens[2], tokens[3], errptr);
   free(tokens[0]);
   return newjob;

}


/*********************************************************************
 *
 * Function    :  pcrs_make_job
 *
 * Description :  Takes the three arguments to a perl s/// command
 *                and compiles a pcrs_job structure from them.
 *
 * Parameters  :
 *          1  :  pattern = string with perl-style pattern
 *          2  :  substitute = string with perl-style substitute
 *          3  :  options = string with perl-style options
 *          4  :  errptr = pointer to an integer in which error
 *                         conditions can be returned.
 *
 * Returns     :  a corresponding pcrs_job data structure, or NULL
 *                if an error was encountered. In that case, *errptr
 *                has the reason.
 *
 *********************************************************************/
pcrs_job *pcrs_make_job(char *pattern, char *substitute, char *options, int *errptr)
{
   pcrs_job *newjob;
   int flags;
   const char *error;

   /* 
    * Handle NULL arguments
    */
   if (pattern == NULL) pattern = "";
   if (substitute == NULL) substitute = "";
   if (options == NULL) options = "";

   /* 
    * Get and init memory
    */
   if (NULL == (newjob = (pcrs_job *)malloc(sizeof(pcrs_job))))
   {
      *errptr = PCRS_ERR_NOMEM;
      return NULL;
   }
   memset(newjob, '\0', sizeof(pcrs_job));


   /*
    * Evaluate the options
    */
   newjob->options = pcrs_compile_perl_options(options, &flags);
   newjob->flags = flags;


   /*
    * Compile the pattern
    */
   newjob->pattern = pcre_compile(pattern, newjob->options, &error, errptr, NULL);
   if (newjob->pattern == NULL)
   {
      pcrs_free_job(newjob);
      return NULL;
   }


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
 

   /*
    * Compile the substitute
    */
   if (NULL == (newjob->substitute = pcrs_compile_replacement(substitute, newjob->flags & PCRS_TRIVIAL, errptr)))
   {
      pcrs_free_job(newjob);
      return NULL;
   }
 
   return newjob;

}


/*********************************************************************
 *
 * Function    :  pcrs_execute
 *
 * Description :  Modify the subject by executing the regular substitution
 *                defined by the job. Since the result may be longer than
 *                the subject, its space requirements are precalculated in
 *                the matching phase and new memory is allocated accordingly.
 *                It is the caller's responsibility to free the result when
 *                it's no longer needed.
 *
 * Parameters  :
 *          1  :  job = the pcrs_job to be executed
 *          2  :  subject = the subject (== original) string
 *          3  :  subject_length = the subject's length 
 *                INCLUDING the terminating zero, if string!
 *          4  :  result = char** for returning  the result 
 *          5  :  result_length = int* for returning the result's length
 *
 * Returns     :  the number of substitutions that were made. May be > 1
 *                if job->flags contained PCRS_GLOBAL
 *
 *********************************************************************/
int pcrs_execute(pcrs_job *job, char *subject, int subject_length, char **result, int *result_length)
{
   int offsets[3 * PCRS_MAX_SUBMATCHES],
       offset, i, k,
       matches_found,
       newsize,
       submatches;
   pcrs_match matches[PCRS_MAX_MATCHES];
   char *result_offset;

   offset = i = k = 0;

   /* 
    * Sanity check
    */
   if (job == NULL || job->pattern == NULL || job->substitute == NULL)
   {
      *result = NULL;
      return(PCRS_ERR_BADJOB);
   }

   
   /*
    * Find the pattern and calculate the space
    * requirements for the result (newsize)
    */
   newsize=subject_length;

   while ((submatches = pcre_exec(job->pattern, job->hints, subject, subject_length, offset, 0, offsets, 3 * PCRS_MAX_SUBMATCHES)) > 0)
   {
      job->flags |= PCRS_SUCCESS;
      matches[i].submatches = submatches;
      for (k=0; k < submatches; k++)
      {
         matches[i].submatch_offset[k] = offsets[2 * k];

         /* Note: Non-found optional submatches have length -1-(-1)==0 */
         matches[i].submatch_length[k] = offsets[2 * k + 1] - offsets[2 * k]; 

         /* reserve mem for each submatch as often as it is ref'd */
         newsize += matches[i].submatch_length[k] * job->substitute->backref_count[k]; 
      }
      /* plus replacement text size minus match text size */
      newsize += strlen(job->substitute->text) - matches[i].submatch_length[0]; 

      /* Non-global search or limit reached? */
      if (++i >= PCRS_MAX_MATCHES || !(job->flags & PCRS_GLOBAL) ) break;

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
   /* Pass pcre error through if failiure*/
   if (submatches < -1) return submatches;   
   matches_found = i;


   /* 
    * Get memory for the result
    */
   if ((*result = (char *)malloc(newsize)) == NULL)   /* must be free()d by caller */
   {
      return PCRS_ERR_NOMEM;
   }


   /* 
    * Replace
    */
   offset = 0;
   result_offset = *result;

   for (i=0; i < matches_found; i++)
   {
      /* copy the chunk preceding the match */
      memcpy(result_offset, subject + offset, matches[i].submatch_offset[0] - offset); 
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
