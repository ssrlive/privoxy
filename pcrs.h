#ifndef _PCRS_H
#define _PCRS_H

/*********************************************************************
 *
 * File        :  $Source: /home/administrator/cvs/ijb/pcrs.h,v $
 *
 * Purpose     :  This is the pre-pre-alpha realease of libpcrs. It is only
 *                published at this (ugly) stage of development, because it is
 *                needed for a new feature in JunkBuster.
 *
 *                Apart from the code being quite a mess, no inconsistencies,
 *                memory leaks or functional bugs **should** be present.
 *
 *                While you ROTFL at the code, you could just as well mail me
 *                (oes@paradis.rhein.de) with advice for improvement.
 *
 *                pcrs is a supplement to the brilliant pcre library by Philip
 *                Hazel (ph10@cam.ac.uk) and adds Perl-style substitution. That
 *                is, it mimics Perl's 's' operator.
 *
 *                Currently, there's no documentation besides comments and the
 *                source itself ;-)
 *
 * Copyright   :  Written and copyright 2001 by Sourceforge IJBSWA team.
 *
 * Revisions   :
 *    $Log: pcrs.h,v $
 *    Revision 1.4  2001/05/11 01:57:02  rodney
 *    Added new file header standard w/RCS control tags.
 *
 *    revision 1.3  2001/05/08 02:38:13  rodney
 *    Changed C++ "//" style comment to C style comments.
 *
 *    revision 1.2  2001/04/30 02:39:24  rodney
 *    Made this pcrs.h file conditionally included.
 *
 *    revision 1.1  2001/04/16 21:10:38  rodney
 *    Initial checkin
 *
 *********************************************************************/

#define PCRS_H_VERSION "$Id: pcrs.h,v 1.1 2001/05/13 21:57:07 administrator Exp $"



#include <pcre.h>

#define FALSE 0
#define TRUE 1
#define PCRS_MAX_MATCHES 300
#define PCRS_MAX_SUBMATCHES 33
#define CHARBUFSIZ BUFSIZ * sizeof(char)

#define PCRS_ERR_NOMEM     -10      /* Failed to acquire memory. */
#define PCRS_ERR_CMDSYNTAX -11      /* Syntax of s///-command */
#define PCRS_ERR_STUDY     -12      /* pcre error while studying the pattern */

typedef struct S_PCRS_SUBSTITUTE {
  char *text;
  int backrefs;
  int block_offset[PCRS_MAX_SUBMATCHES];
  int block_length[PCRS_MAX_SUBMATCHES];
  int backref[PCRS_MAX_SUBMATCHES];
  int backref_count[PCRS_MAX_SUBMATCHES];
} pcrs_substitute;

typedef struct S_PCRS_MATCH {
  /* char *buffer; */
  int submatches;
  int submatch_offset[PCRS_MAX_SUBMATCHES];
  int submatch_length[PCRS_MAX_SUBMATCHES];
} pcrs_match;

typedef struct S_PCRS_JOB {
  pcre *pattern;
  pcre_extra *hints;
  int options;
  int globalflag;
  int successflag;
  pcrs_substitute *substitute;
  struct S_PCRS_JOB *next;
} pcrs_job;

extern int              pcrs_compile_perl_options(char *optstring, int *globalflag);
extern pcrs_substitute *pcrs_compile_replacement(char *replacement, int *errptr);
extern pcrs_job        *pcrs_free_job(pcrs_job *job);
extern pcrs_job        *pcrs_make_job(char *command, int *errptr);
extern pcrs_job        *create_pcrs_job(pcre *pattern, pcre_extra *hints, int options, int globalflag, pcrs_substitute *substitute, int *errptr);
extern int              pcrs_exec_substitution(pcrs_job *job, char *subject, int subject_length, char **result, int *result_length);


#endif /* ndef _PCRS_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
