#ifndef PCRS_H_INCLUDED
#define PCRS_H_INCLUDED

/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/pcrs.h,v $
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
 *    Revision 1.6  2001/07/29 18:52:06  jongfoster
 *    Renaming _PCRS_H, and adding "extern C {}"
 *
 *    Revision 1.5  2001/07/18 17:27:00  oes
 *    Changed interface; Cosmetics
 *
 *    Revision 1.4  2001/06/29 13:33:19  oes
 *    - Cleaned up, commented and adapted to reflect the
 *      changes in pcrs.c
 *    - Introduced the PCRS_* flags
 *
 *    Revision 1.3  2001/06/09 10:58:57  jongfoster
 *    Removing a single unused #define which referenced BUFSIZ
 *
 *    Revision 1.2  2001/05/25 11:03:55  oes
 *    Added sanity check for NULL jobs to pcrs_exec_substitution
 *
 *    Revision 1.1.1.1  2001/05/15 13:59:02  oes
 *    Initial import of version 2.9.3 source tree
 *
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

#define PCRS_H_VERSION "$Id: pcrs.h,v 1.6 2001/07/29 18:52:06 jongfoster Exp $"


#include <pcre.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Constants:
 */

#define FALSE 0
#define TRUE 1

/* Capacity */
#define PCRS_MAX_MATCHES 300
#define PCRS_MAX_SUBMATCHES 33

/* Error codes */
#define PCRS_ERR_NOMEM     -10      /* Failed to acquire memory. */
#define PCRS_ERR_CMDSYNTAX -11      /* Syntax of s///-command */
#define PCRS_ERR_STUDY     -12      /* pcre error while studying the pattern */
#define PCRS_ERR_BADJOB    -13      /* NULL job pointer, pattern or substitute */

/* Flags */
#define PCRS_GLOBAL          1      /* Job should be applied globally, as with perl's g option */
#define PCRS_SUCCESS         2      /* Job did previously match */
#define PCRS_TRIVIAL         4      /* Backreferences in the substitute are ignored */

/*
 * Data types:
 */

/* A compiled substitute */
typedef struct PCRS_SUBSTITUTE {
  char *text;                               /* The plaintext part of the substitute, with all backreferences stripped */
  int backrefs;                             /* The number of backreferences */
  int block_offset[PCRS_MAX_SUBMATCHES];    /* Array with the offsets of all plaintext blocks in text */
  int block_length[PCRS_MAX_SUBMATCHES];    /* Array with the lengths of all plaintext blocks in text */
  int backref[PCRS_MAX_SUBMATCHES];         /* Array with the backref number for all plaintext block borders */
  int backref_count[PCRS_MAX_SUBMATCHES];   /* Array with the number of reference to each backref index */
} pcrs_substitute;

typedef struct PCRS_MATCH {
  /* char *buffer; */
  int submatches;                           /* Number of submatches. Note: The zeroth is the whole match */
  int submatch_offset[PCRS_MAX_SUBMATCHES]; /* Offset for each submatch in the subject */
  int submatch_length[PCRS_MAX_SUBMATCHES]; /* Length of each submatch in the subject */
} pcrs_match;

typedef struct PCRS_JOB {
  pcre *pattern;                            /* The compiled pcre pattern */
  pcre_extra *hints;                        /* The pcre hints for the pattern */
  int options;                              /* The pcre options (numeric) */
  int flags;                                /* The pcrs and user flags (see "Flags" above) */
  pcrs_substitute *substitute;              /* The compiles pcrs substitute */
  struct PCRS_JOB *next;                    /* Pointer for chaining jobs to joblists */
} pcrs_job;

/*
 * Prototypes:
 */

/* Main usage */
extern pcrs_job        *pcrs_compile_command(const char *command, int *errptr);
extern pcrs_job        *pcrs_compile(const char *pattern, const char *substitute, const char *options, int *errptr);
extern int              pcrs_execute(pcrs_job *job, char *subject, int subject_length, char **result, int *result_length);

/* Freeing jobs */
extern pcrs_job        *pcrs_free_job(pcrs_job *job);
extern void             pcrs_free_joblist(pcrs_job *joblist);

/* Expert usage */
extern int              pcrs_compile_perl_options(const char *optstring, int *flags);
extern pcrs_substitute *pcrs_compile_replacement(const char *replacement, int trivialflag, int *errptr);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef PCRS_H_INCLUDED */

/*
  Local Variables:
  tab-width: 3
  end:
*/
