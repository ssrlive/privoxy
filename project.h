#ifndef PROJECT_H_INCLUDED
#define PROJECT_H_INCLUDED
#define PROJECT_H_VERSION "$Id: project.h,v 1.47 2002/02/20 23:15:13 jongfoster Exp $"
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/project.h,v $
 *
 * Purpose     :  Defines data structures which are widely used in the
 *                project.  Does not define any variables or functions
 *                (though it does declare some macros).
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
 *    $Log: project.h,v $
 *    Revision 1.47  2002/02/20 23:15:13  jongfoster
 *    Parsing functions now handle out-of-memory gracefully by returning
 *    an error code.
 *
 *    Revision 1.46  2002/01/17 21:06:09  jongfoster
 *    Now #defining the URLs of the config interface
 *
 *    Minor changes to struct http_request and struct url_spec due to
 *    standardizing that struct http_request is used to represent a URL, and
 *    struct url_spec is used to represent a URL pattern.  (Before, URLs were
 *    represented as seperate variables and a partially-filled-in url_spec).
 *
 *    Revision 1.45  2002/01/09 14:33:27  oes
 *    Added HOSTENT_BUFFER_SIZE
 *
 *    Revision 1.44  2001/12/30 14:07:32  steudten
 *    - Add signal handling (unix)
 *    - Add SIGHUP handler (unix)
 *    - Add creation of pidfile (unix)
 *    - Add action 'top' in rc file (RH)
 *    - Add entry 'SIGNALS' to manpage
 *    - Add exit message to logfile (unix)
 *
 *    Revision 1.43  2001/11/22 21:57:51  jongfoster
 *    Making action_spec->flags into an unsigned long rather than just an
 *    unsigned int.
 *    Adding ACTION_NO_COOKIE_KEEP
 *
 *    Revision 1.42  2001/11/05 21:42:41  steudten
 *    Include DBG() macro.
 *
 *    Revision 1.41  2001/10/28 19:12:06  jongfoster
 *    Adding ijb_toupper()
 *
 *    Revision 1.40  2001/10/26 17:40:47  oes
 *    Moved ijb_isspace and ijb_tolower to project.h
 *    Removed http->user_agent, csp->referrer and csp->accept_types
 *
 *    Revision 1.39  2001/10/25 03:45:02  david__schmidt
 *    Adding a (void*) cast to freez() because Visual Age C++ won't expand the
 *    macro when called with a cast; so moving the cast to the macro def'n
 *    seems to both eliminate compiler warnings (on darwin and OS/2, anyway) and
 *    doesn't make macro expansion complain.  Hope this works for everyone else
 *    too...
 *
 *    Revision 1.38  2001/10/23 21:19:04  jongfoster
 *    New error-handling support: jb_err type and JB_ERR_xxx constants
 *    CGI functions now return a jb_err, and their parameters map is const.
 *    Support for RUNTIME_FEATUREs to enable/disable config editor
 *    Adding a few comments
 *
 *    Revision 1.37  2001/10/14 22:14:01  jongfoster
 *    Removing name_length field from struct cgi_dispatcher, as this is
 *    now calculated at runtime from the "name" field.
 *
 *    Revision 1.36  2001/10/10 16:45:15  oes
 *    Added LIMIT_CONNECT action and string
 *    Fixed HTTP message line termination
 *    Added CFORBIDDEN HTTP message
 *
 *    Revision 1.35  2001/10/07 18:06:43  oes
 *    Added status member to struct http_request
 *
 *    Revision 1.34  2001/10/07 15:45:25  oes
 *    Added url member to struct http_request and commented all
 *      members
 *
 *    Added CT_TABOO
 *
 *    Added ACTION_DOWNGRADE and ACTION_NO_COMPRESSION
 *
 *    Replaced struct client_state members rejected,
 *      force, active and toggled_on with "flags" bitmap.
 *
 *    Added CSP_FLAG_MODIFIED and CSP_FLAG_CHUNKED
 *
 *    Added buffer_limit to struct configuration_spec
 *
 *    Revision 1.33  2001/09/20 13:30:08  steudten
 *
 *    Make freez() more secure in case of: if (exp) { free(z) ; a=*z }
 *    Last case will set z to NULL in free(z) and thats bad..
 *
 *    Revision 1.32  2001/09/16 23:02:51  jongfoster
 *    Fixing warning
 *
 *    Revision 1.31  2001/09/16 13:20:29  jongfoster
 *    Rewrite of list library.  Now has seperate header and list_entry
 *    structures.  Also added a large sprinking of assert()s to the list
 *    code.
 *
 *    Revision 1.30  2001/09/13 23:52:00  jongfoster
 *    Support for both static and dynamically generated CGI pages
 *
 *    Revision 1.29  2001/09/13 23:29:43  jongfoster
 *    Defining FORWARD_SPEC_INITIALIZER
 *
 *    Revision 1.28  2001/09/13 23:05:50  jongfoster
 *    Changing the string paramater to the header parsers a "const".
 *
 *    Revision 1.27  2001/08/05 16:06:20  jongfoster
 *    Modifiying "struct map" so that there are now separate header and
 *    "map_entry" structures.  This means that functions which modify a
 *    map no longer need to return a pointer to the modified map.
 *    Also, it no longer reverses the order of the entries (which may be
 *    important with some advanced template substitutions).
 *
 *    Revision 1.26  2001/07/30 22:08:36  jongfoster
 *    Tidying up #defines:
 *    - All feature #defines are now of the form FEATURE_xxx
 *    - Permanently turned off WIN_GUI_EDIT
 *    - Permanently turned on WEBDAV and SPLIT_PROXY_ARGS
 *
 *    Revision 1.25  2001/07/29 18:43:08  jongfoster
 *    Changing #ifdef _FILENAME_H to FILENAME_H_INCLUDED, to conform to
 *    ANSI C rules.
 *
 *    Revision 1.24  2001/07/25 17:20:27  oes
 *    Introduced http->user_agent
 *
 *    Revision 1.23  2001/07/18 12:32:23  oes
 *    - Added ACTION_STRING_DEANIMATE
 *    - moved #define freez from jcc.h to project.h
 *
 *    Revision 1.22  2001/07/15 17:51:41  jongfoster
 *    Renaming #define STATIC to STATIC_PCRE
 *
 *    Revision 1.21  2001/07/13 14:03:19  oes
 *     - Reorganized regex header inclusion and #defines to
 *       comply to the scheme in configure.in
 *     - Added csp->content_type and its CT_* keys
 *     - Added ACTION_DEANIMATE
 *     - Removed all #ifdef PCRS
 *
 *    Revision 1.20  2001/06/29 21:45:41  oes
 *    Indentation, CRLF->LF, Tab-> Space
 *
 *    Revision 1.19  2001/06/29 13:33:36  oes
 *    - Improved comments
 *    - Introduced http_request.host_ip_addr_str
 *    - Introduced http_response.head_length
 *    - Introduced config.my_ip_addr_str, config.my_hostname,
 *      config.admin_address and config.proxy_info_url
 *    - Removed config.proxy_args_header and config.proxy_args_trailer,
 *      renamed config.proxy_args_invocation to config.proxy_args
 *    - Removed HTML snipplets and GIFs
 *    - Removed logentry from cancelled commit
 *
 *    Revision 1.18  2001/06/09 10:57:39  jongfoster
 *    Adding definition of BUFFER_SIZE.
 *    Changing struct cgi_dispatcher to use "const" strings.
 *
 *    Revision 1.17  2001/06/07 23:15:09  jongfoster
 *    Merging ACL and forward files into config file.
 *    Moving struct gateway members into struct forward_spec
 *    Removing config->proxy_args_gateways
 *    Cosmetic: Adding a few comments
 *
 *    Revision 1.16  2001/06/04 18:31:58  swa
 *    files are now prefixed with either `confdir' or `logdir'.
 *    `make redhat-dist' replaces both entries confdir and logdir
 *    with redhat values
 *
 *    Revision 1.15  2001/06/04 11:28:53  swa
 *    redirect did not work due to missing /
 *
 *    Revision 1.14  2001/06/03 11:03:48  oes
 *    Added struct map,
 *    added struct http_response,
 *    changed struct interceptors to struct cgi_dispatcher,
 *    moved HTML stuff to cgi.h
 *
 *    Revision 1.13  2001/06/01 20:05:36  jongfoster
 *    Support for +image-blocker{}: added ACTION_IMAGE_BLOCKER
 *    constant, and removed csp->tinygif.
 *
 *    Revision 1.12  2001/06/01 18:49:17  jongfoster
 *    Replaced "list_share" with "list" - the tiny memory gain was not
 *    worth the extra complexity.
 *
 *    Revision 1.11  2001/06/01 10:32:47  oes
 *    Added constants for anchoring selection bitmap
 *
 *    Revision 1.10  2001/05/31 21:33:53  jongfoster
 *    Changes for new actions file, replacing permissionsfile
 *    and parts of the config file.  Also added support for
 *    list_shared.
 *
 *    Revision 1.9  2001/05/31 17:32:31  oes
 *
 *     - Enhanced domain part globbing with infix and prefix asterisk
 *       matching and optional unanchored operation
 *
 *    Revision 1.8  2001/05/29 20:09:15  joergs
 *    HTTP_REDIRECT_TEMPLATE fixed.
 *
 *    Revision 1.7  2001/05/29 09:50:24  jongfoster
 *    Unified blocklist/imagelist/actionslist.
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
 *    Revision 1.6  2001/05/27 22:17:04  oes
 *
 *    - re_process_buffer no longer writes the modified buffer
 *      to the client, which was very ugly. It now returns the
 *      buffer, which it is then written by chat.
 *
 *    - content_length now adjusts the Content-Length: header
 *      for modified documents rather than crunch()ing it.
 *      (Length info in csp->content_length, which is 0 for
 *      unmodified documents)
 *
 *    - For this to work, sed() is called twice when filtering.
 *
 *    Revision 1.5  2001/05/26 00:28:36  jongfoster
 *    Automatic reloading of config file.
 *    Removed obsolete SIGHUP support (Unix) and Reload menu option (Win32).
 *    Most of the global variables have been moved to a new
 *    struct configuration_spec, accessed through csp->config->globalname
 *    Most of the globals remaining are used by the Win32 GUI.
 *
 *    Revision 1.4  2001/05/22 18:46:04  oes
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
 *    Revision 1.3  2001/05/20 01:21:20  jongfoster
 *    Version 2.9.4 checkin.
 *    - Merged popupfile and cookiefile, and added control over PCRS
 *      filtering, in new "actionsfile".
 *    - Implemented LOG_LEVEL_FATAL, so that if there is a configuration
 *      file error you now get a message box (in the Win32 GUI) rather
 *      than the program exiting with no explanation.
 *    - Made killpopup use the PCRS MIME-type checking and HTTP-header
 *      skipping.
 *    - Removed tabs from "config"
 *    - Moved duplicated url parsing code in "loaders.c" to a new funcition.
 *    - Bumped up version number.
 *
 *    Revision 1.2  2001/05/17 23:01:01  oes
 *     - Cleaned CRLF's from the sources and related files
 *
 *    Revision 1.1.1.1  2001/05/15 13:59:03  oes
 *    Initial import of version 2.9.3 source tree
 *
 *
 *********************************************************************/


/* Declare struct FILE for vars and funcs. */
#include <stdio.h>

/* Need time_t for file_list */
#include <time.h>

/*
 * Include appropriate regular expression libraries.
 * Note that pcrs and pcre (native) are needed for cgi
 * and are included anyway.
 */

#if defined(REGEX_PCRE) || defined (REGEX_GNU)
# define REGEX
#endif /* defined(REGEX_PCRE) || defined (REGEX_GNU) */

#ifdef STATIC_PCRE
#  include "pcre.h"
#else
#  include <pcre.h>
#endif

#ifdef STATIC_PCRS
#  include "pcrs.h"
#else
#  include <pcrs.h>
#endif

#if defined(REGEX_PCRE)
#  ifdef STATIC_PCRE
#    include "pcreposix.h"
#  else
#    include <pcreposix.h>
#  endif
#endif /* defined(REGEX_PCRE) */

#if defined(REGEX_GNU)
#  include "gnu_regex.h"
#endif

#ifdef AMIGA
#include "amiga.h"
#endif /* def AMIGA */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DEBUG
extern int ldebug;
#define DBG(a,b)        { if ( ldebug >= a ) { printf b ; }}
#else
#define DBG(a,b)
#endif /* _DEBUG */



/*
 * Error codes.  Functions returning these should return a jb_err
 */
#define JB_ERR_OK         0 /* Success, no error                        */
#define JB_ERR_MEMORY     1 /* Out of memory                            */
#define JB_ERR_CGI_PARAMS 2 /* Missing or corrupt CGI parameters        */
#define JB_ERR_FILE       3 /* Error opening, reading or writing a file */
#define JB_ERR_PARSE      4 /* Error parsing file                       */
#define JB_ERR_MODIFIED   5 /* File has been modified outside of the    */
                            /* CGI actions editor.                      */
typedef int jb_err;


/*
 * This macro is used to free a pointer that may be NULL
 */
#define freez(X)  { if(X) { free((void*)X); X = NULL ; } }


/* Fix a problem with Solaris.  There should be no effect on other
 * platforms.
 * Solaris's isspace() is a macro which uses it's argument directly
 * as an array index.  Therefore we need to make sure that high-bit
 * characters generate +ve values, and ideally we also want to make
 * the argument match the declared parameter type of "int".
 *
 * Note: Remember to #include <ctype.h> if you use these macros.
 */
#define ijb_toupper(__X) toupper((int)(unsigned char)(__X))
#define ijb_tolower(__X) tolower((int)(unsigned char)(__X))
#define ijb_isspace(__X) isspace((int)(unsigned char)(__X))  

/*
 * Use for statically allocated buffers if you have no other choice.
 * Remember to check the length of what you write into the buffer
 * - we don't want any buffer overflows!
 */
#define BUFFER_SIZE 5000

/*
 * Buffer size for capturing struct hostent data in the
 * gethostby(name|addr)_r library calls. Since we don't
 * loop over gethostbyname_r, the buffer must be sufficient
 * to accomodate multiple IN A RRs, as used in DNS round robin
 * load balancing. W3C's wwwlib uses 1K, so that should be
 * good enough for us, too.
 */
#define HOSTENT_BUFFER_SIZE 1024

/*
 * So you can say "while (FOREVER) { ...do something... }"
 */
#define FOREVER 1

/* Default IP and port to listen on */
#define HADDR_DEFAULT   "127.0.0.1"
#define HADDR_PORT      8000

/*
 * pid file name
 */
#define PID_FILE_NAME   "junkbuster.pid"

/* Forward defs for various structures */

/* Need this for struct client_state */
struct configuration_spec;


/* Generic linked list of strings */

struct list_entry
{
   const char *str;
   struct list_entry *next;
};

struct list
{
   struct list_entry *first;
   struct list_entry *last;
};


/* A map from a string to another string */

struct map_entry
{
   const char *name;
   const char *value;
   struct map_entry *next;
};

struct map
{
   struct map_entry *first;
   struct map_entry *last;
};


struct http_request
{
   char *cmd;      /* Whole command line: method, URL, Version */
   char *ocmd;     /* Backup of original cmd for CLF logging */
   char *gpc;      /* HTTP method: GET, POST, .. */
   char *url;      /* The URL */
   char *ver;      /* Protocol version */
   int status;     /* HTTP Status */

   char *host;     /* Host part of URL */
   int   port;     /* Port of URL or 80 (default) */
   char *path;     /* Path of URL */
   char *hostport; /* host[:port] */
   int   ssl;      /* Flag if protocol is https */

   char *host_ip_addr_str; /* String with dotted decimal representation
                            * of host's IP. NULL before connect_to() */

   char  *dbuffer;     /* Buffer with '\0'-delimited domain name.           */
   char **dvec;        /* List of pointers to the strings in dbuffer.       */
   int    dcount;      /* How many parts to this domain? (length of dvec)   */
};

/* Response generated by CGI, blocker, or error handler */
struct http_response
{
  char *status;           /* HTTP status (string)*/
  struct list headers[1]; /* List of header lines */
  char *head;             /* Formatted http response head */
  int   head_length;      /* Length of http response head */
  char *body;             /* HTTP document body */
  int   content_length;   /* Length of body, REQUIRED if binary body */
  int   is_static;        /* Nonzero if the content will never change and
                           * should be cached by the broser (e.g. images) */
};

/* A URL pattern */
struct url_spec
{
   char  *spec;        /* The string which was parsed to produce this       */
                       /* url_spec.  Used for debugging or display only.    */

   /* Hostname matching, or dbuffer == NULL to match all hosts */
   char  *dbuffer;     /* Buffer with '\0'-delimited domain name.           */
   char **dvec;        /* List of pointers to the strings in dbuffer.       */
   int    dcount;      /* How many parts to this domain? (length of dvec)   */
   int    unanchored;  /* Bitmap - flags are ANCHOR_LEFT and ANCHOR_RIGHT.  */

   /* Port matching: */
   int   port;         /* The port number, or 0 to match all ports.         */

   /* Path matching: */
   char *path;         /* The path prefix (if not using regex), or source   */
                       /* for the regex.                                    */
   int   pathlen;      /* ==strlen(path).  Needed for prefix matching.      */
#ifdef REGEX
   regex_t *preg;      /* Regex for matching path part                      */
#endif
};
#ifdef REGEX
#define URL_SPEC_INITIALIZER { NULL, NULL, NULL, 0, 0, 0, NULL, 0, NULL }
#else /* ifndef REGEX */
#define URL_SPEC_INITIALIZER { NULL, NULL, NULL, 0, 0, 0, NULL, 0 }
#endif /* ndef REGEX */

/* Constants for host part matching in URLs */
#define ANCHOR_LEFT  1
#define ANCHOR_RIGHT 2


/* An I/O buffer */
struct iob
{
   char *buf;
   char *cur;
   char *eod;
};


#define IOB_PEEK(CSP) ((CSP->iob->cur > CSP->iob->eod) ? (CSP->iob->eod - CSP->iob->cur) : 0)
#define IOB_RESET(CSP) if(CSP->iob->buf) free(CSP->iob->buf); memset(CSP->iob, '\0', sizeof(CSP->iob));

/* Keys for csp->content_type */
#define CT_TEXT   1 /* Suitable for pcrs filtering */
#define CT_GIF    2 /* Suitable for GIF filtering */
#define CT_TABOO  3 /* DONT filter */

#define ACTION_MASK_ALL        (~0U)

#define ACTION_MOST_COMPATIBLE 0x00000000UL

#define ACTION_BLOCK           0x00000001UL
#define ACTION_DEANIMATE       0x00000002UL
#define ACTION_DOWNGRADE       0x00000004UL
#define ACTION_FAST_REDIRECTS  0x00000008UL
#define ACTION_FILTER          0x00000010UL
#define ACTION_HIDE_FORWARDED  0x00000020UL
#define ACTION_HIDE_FROM       0x00000040UL
#define ACTION_HIDE_REFERER    0x00000080UL /* sic - follow HTTP, not English */
#define ACTION_HIDE_USER_AGENT 0x00000100UL
#define ACTION_IMAGE           0x00000200UL
#define ACTION_IMAGE_BLOCKER   0x00000400UL
#define ACTION_NO_COMPRESSION  0x00000800UL
#define ACTION_NO_COOKIE_KEEP  0x00001000UL
#define ACTION_NO_COOKIE_READ  0x00002000UL
#define ACTION_NO_COOKIE_SET   0x00004000UL
#define ACTION_NO_POPUPS       0x00008000UL
#define ACTION_VANILLA_WAFER   0x00010000UL
#define ACTION_LIMIT_CONNECT   0x00020000UL

#define ACTION_STRING_DEANIMATE     0
#define ACTION_STRING_FROM          1
#define ACTION_STRING_IMAGE_BLOCKER 2
#define ACTION_STRING_REFERER       3
#define ACTION_STRING_USER_AGENT    4
#define ACTION_STRING_LIMIT_CONNECT 5
#define ACTION_STRING_COUNT         6

#define ACTION_MULTI_ADD_HEADER     0
#define ACTION_MULTI_WAFER          1
#define ACTION_MULTI_COUNT          2

/*
 * This structure contains a list of actions to apply to a URL.
 * It only contains positive instructions - no "-" options.
 * It is not used to store the actions list itself, only for
 * url_actions() to return the current values.
 */
struct current_action_spec
{
   unsigned long flags;    /* a bit set to "1" = add action    */

   /* For those actions that require parameters: */

   /* each entry is valid if & only if corresponding entry in "add" set. */
   char * string[ACTION_STRING_COUNT];

   /* Strings to add */
   struct list multi[ACTION_MULTI_COUNT][1];
};


/*
 * This structure contains a set of changes to actions.
 * It can contain both positive and negative instructions.
 * It is used to store an entry in the actions list.
 */
struct action_spec
{
   unsigned long mask;   /* a bit set to "0" = remove action */
   unsigned long add;    /* a bit set to "1" = add action    */

   /* For those actions that require parameters: */

   /* each entry is valid if & only if corresponding entry in "add" set. */
   char * string[ACTION_STRING_COUNT];

   /* Strings to remove. */
   struct list multi_remove[ACTION_MULTI_COUNT][1];

   /* If nonzero, remove *all* strings. */
   int         multi_remove_all[ACTION_MULTI_COUNT];

   /* Strings to add */
   struct list multi_add[ACTION_MULTI_COUNT][1];
};

/*
 * This structure is used to store the actions list.
 *
 * It contains a URL pattern, and the chages to the actions.
 * It is a linked list.
 */
struct url_actions
{
   struct url_spec url[1];

   struct action_spec action[1];

   struct url_actions * next;
};


/*
 * Flags for use in csp->flags
 */
#define CSP_FLAG_ACTIVE     0x01 /* Set if this client is processing data.
                                  * Cleared when the thread associated with
                                  * this structure dies. */
#define CSP_FLAG_CHUNKED    0x02 /* Set if the server's reply is in "chunked"
                                  * transfer encoding */
#define CSP_FLAG_FORCED     0x04 /* Set if this request was enforced, although
                                  * it would normally have been blocked. */
#define CSP_FLAG_MODIFIED   0x08 /* Set if any modification to the body was done */
#define CSP_FLAG_REJECTED   0x10 /* Set if request was blocked.  */
#define CSP_FLAG_TOGGLED_ON 0x20 /* Set if we are toggled on (FEATURE_TOGGLE) */

/*
 * The state of a JunkBuster processing thread.
 */
struct client_state
{
   /* The proxy's configuration */
   struct configuration_spec * config;

   /* The actions to perform on the current request */
   struct current_action_spec  action[1];

   /* socket to talk to client (web browser) */
   int  cfd;

   /* socket to talk to server (web server or proxy) */
   int  sfd;

   /* Multi-purpose flag container, see CSP_FLAG_* above */
   unsigned short int flags;

   /*
    * Client PC's IP address, as reported by the accept()_ function.
    * Both as string and number
    */
   char *ip_addr_str;
   long  ip_addr_long;


   /* Our IP address and hostname, i.e. the IP address that
      the client used to reach us, and the associated hostname,
      both as strings
    */
   char *my_ip_addr_str;
   char *my_hostname;

   /* The URL that was requested */
   struct http_request http[1];

   /* An I/O buffer used for buffering data read from the client */
   struct iob iob[1];

   /* List of all headers for this request */
   struct list headers[1];

   /* List of all cookies for this request */
   struct list cookie_list[1];

   /* MIME-Type key, see CT_* above */
   unsigned short int content_type;

   /* The "X-Forwarded-For:" header sent by the client */
   char   *x_forwarded;

   /* files associated with this client */
   struct file_list *actions_list;

   struct file_list *rlist;   /* pcrs job file */
   size_t content_length;     /* Length after content modification */

#ifdef FEATURE_TRUST
   struct file_list *tlist;   /* trustfile */
#endif /* def FEATURE_TRUST */

   struct client_state *next;
};


/*
 * A function to add a header
 */
typedef jb_err (*add_header_func_ptr)(struct client_state *);

/*
 * A function to process a header
 */
typedef jb_err (*parser_func_ptr    )(struct client_state *, char **);

/*
 * List of functions to run on a list of headers
 */
struct parsers
{
   char *str;
   char  len;
   parser_func_ptr parser;
};


/*
 * List of available CGI functions.
 */
struct cgi_dispatcher
{
   const char * const name;
   jb_err    (* const handler)(struct client_state *csp, struct http_response *rsp, const struct map *parameters);
   const char * const description;
};


/*
 * A data file used by JunkBuster.  Kept in a linked list.
 */
struct file_list
{
   /*
    * this is a pointer to the data structures associated with the file.
    * Read-only once the structure has been created.
    */
   void *f;

   /* Normally NULL.  When we are finished with file (i.e. when we have
    * loaded a new one), set to a pointer to an unloader function.
    * Unloader will be called by sweep() (called from main loop) when
    * all clients using this file are done.  This prevents threading
    * problems.
    */
   void (*unloader)(void *);

   /* Used internally by sweep().  Do not access from elsewhere. */
   int active;

   /* Following variables allow us to check if file has been changed.
    * Read-only once the structure has been created.
    */
   time_t lastmodified;
   char * filename;

   /* Pointer to next entry in the linked list of all "file_list"s.
    * This linked list is so that sweep() can navigate it.
    * Since sweep() can remove items from the list, we must be careful
    * to only access this value from main thread (when we know sweep
    * won't be running).
    */
   struct file_list *next;
};


#ifdef FEATURE_TRUST
struct block_spec
{
   struct url_spec url[1];
   int    reject;
   struct block_spec *next;
};
#endif /* def FEATURE_TRUST */


#define SOCKS_NONE    0    /* Don't use a SOCKS server */
#define SOCKS_4      40    /* original SOCKS 4 protocol */
#define SOCKS_4A     41    /* as modified for hosts w/o external DNS */

struct forward_spec
{
   struct url_spec url[1];

   /* Connection type - must be a SOCKS_xxx constant */
   int   type;

   /* SOCKS server */
   char *gateway_host;
   int   gateway_port;

   /* Parent HTTP proxy */
   char *forward_host;
   int   forward_port;

   /* For the linked list */
   struct forward_spec *next;
};
#define FORWARD_SPEC_INITIALIZER { { URL_SPEC_INITIALIZER }, 0, NULL, 0, NULL, 0, NULL }


struct re_filterfile_spec
{
   char *username;
   char *filtername;
   struct list patterns[1];
   pcrs_job *joblist;
};

#ifdef FEATURE_ACL
#define ACL_PERMIT   1  /* accept connection request */
#define ACL_DENY     2  /* reject connection request */

struct access_control_addr
{
   unsigned long addr;
   unsigned long mask;
   unsigned long port;
};

struct access_control_list
{
   struct access_control_addr src[1];
   struct access_control_addr dst[1];

   short action;
   struct access_control_list *next;
};
#endif /* def FEATURE_ACL */


/* Maximum number of loaders (actions, re_filter, ...) */
#define NLOADERS 8


#define RUNTIME_FEATURE_CGI_EDIT_ACTIONS  1
#define RUNTIME_FEATURE_CGI_TOGGLE        2


/*
 * Data loaded from the configuration file.
 *
 * (Anomaly: toggle is still handled through a global, not this structure)
 */
struct configuration_spec
{
   int debug;
   int multi_threaded;

   /* Features that can be enabled/disabled throuigh the config file */
   unsigned feature_flags;

   const char *logfile;

   const char *confdir;
   const char *logdir;
   const char *actions_file;

   /* The administrator's email address */
   char *admin_address;

   /* A URL with info on this proxy */
   char *proxy_info_url;

   const char *re_filterfile;

#ifdef FEATURE_COOKIE_JAR
   const char * jarfile;
   FILE * jar;
#endif /* def FEATURE_COOKIE_JAR */

   /*
    * Port and IP to bind to.
    * Defaults to HADDR_DEFAULT:HADDR_PORT == 127.0.0.1:8000
    */
   const char *haddr;
   int         hport;

   /* Size limit for IOB */
   size_t buffer_limit;

#ifdef FEATURE_TRUST
   const char * trustfile;

   struct list trust_info[1];
   struct url_spec *trust_list[64];
#endif /* def FEATURE_TRUST */

#ifdef FEATURE_ACL
   struct access_control_list *acl;
#endif /* def FEATURE_ACL */

   struct forward_spec *forward;

   /* All options from the config file, HTML-formatted */
   char *proxy_args;

   /* the configuration file object. */
   struct file_list *config_file_list;

   /* List of loaders */
   int (*loaders[NLOADERS])(struct client_state *);

   /* bool, nonzero if we need to bind() to the new port */
   int need_bind;
};


#define SZ(X)  (sizeof(X) / sizeof(*X))

#ifdef FEATURE_FORCE_LOAD
#define FORCE_PREFIX "/IJB-FORCE-LOAD"
#endif /* def FEATURE_FORCE_LOAD */

/* Hardwired URLs */
#define HOME_PAGE_URL       "http://ijbswa.sourceforge.net"
#define REDIRECT_URL        HOME_PAGE_URL "/redirect.php?v=" VERSION "&to="

/*
 * The "hosts" to intercept and display CGI pages.
 * First one is a hostname only, second one can specify host and path.
 *
 * Notes:
 * 1) Do not specify the http: prefix
 * 2) CGI_SITE_2_PATH must not end with /, one will be added automatically.
 * 3) CGI_SITE_2_PATH must start with /, unless it is the empty string.
 */
#define CGI_SITE_1_HOST "i.j.b"
#define CGI_SITE_2_HOST "ijbswa.sourceforge.net"
#define CGI_SITE_2_PATH "/config"

/*
 * The prefix for CGI pages.  Written out in generated HTML.
 * INCLUDES the trailing slash.
 */
#define CGI_PREFIX  "http://" CGI_SITE_2_HOST CGI_SITE_2_PATH "/"


/* HTTP snipplets */
static const char CSUCCEED[] =
   "HTTP/1.0 200 Connection established\n"
   "Proxy-Agent: IJ/" VERSION "\r\n\r\n";

static const char CHEADER[] =
   "HTTP/1.0 400 Invalid header received from browser\r\n\r\n";

static const char CFORBIDDEN[] =
   "HTTP/1.0 403 Connection not allowable\r\nX-Hint: If you read this message interactively, then you know why this happens ,-)\r\n\r\n";

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef PROJECT_H_INCLUDED */

/*
  Local Variables:
  tab-width: 3
  end:
*/
