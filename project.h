#ifndef _PROJECT_H
#define _PROJECT_H
#define PROJECT_H_VERSION "$Id: project.h,v 1.14 2001/06/03 11:03:48 oes Exp $"
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
 *    Revision 1.14  2001/06/03 11:03:48  oes
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
 *
 * PCRS           ==> Include pcre
 * REGEX && PCRE  ==> Include pcre and pcreposix
 * REGEX && !PCRE ==> Include gnu_regex
 *
 * STATIC  ==> Use  #include "pcre.h"  (compiling at same time)
 * !STATIC ==> Use  #include <pcre.h>  (System library)
 *
 */
#if (defined(REGEX) && defined(PCRE)) || defined(PCRS)
#  ifdef STATIC
#    include "pcre.h"
#  else
#    include <pcre.h>
#  endif
#endif /* (defined(REGEX) && defined(PCRE)) || defined(PCRS) */

#if defined(REGEX) && defined(PCRE)
#  ifdef STATIC
#    include "pcreposix.h"
#  else
#    include <pcreposix.h>
#  endif
#endif /* defined(REGEX) && defined(PCRE) */

#if defined(REGEX) && !defined(PCRE)
#  include "gnu_regex.h"
#endif

#ifdef PCRS
#include "pcrs.h"
#endif /* def PCRS */

#ifdef AMIGA 
#include "amiga.h" 
#endif /* def AMIGA */

#ifdef __cplusplus
extern "C" {
#endif

#define FOREVER 1

/* Default IP and port to listen on */
#define HADDR_DEFAULT   "127.0.0.1"
#define HADDR_PORT      8000


/* Need this for struct gateway */
struct client_state;

/* Need this for struct client_state */
struct configuration_spec;

/* Generic linked list of strings */
struct list /* FIXME: Why not separate entries and header? */
{
   char *       str;  /* valid in an entry */
   struct list *last; /* valid in header */
   struct list *next;
};

struct map
{
  char *name;
  char *value;
  struct map *next;
};


/* Generic linked list of strings */
struct list_share /* FIXME: Why not separate entries and header? */
{
   const char *       str;  /* valid in an entry */
   struct list_share *last; /* valid in header */
   struct list_share *next;
};

struct http_request
{
   char *cmd;
   char *gpc;
   char *host;
   int   port;
   char *path;
   char *ver;
   char *hostport; /* "host[:port]" */
   int   ssl;
};

/* CGI or blocker generated response */
struct http_response
{
  char *status;             /* HTTP status */
  struct list headers[1]; /* List of header lines */
  int content_length;     /* Length of body, REQUIRED if binary body*/
  char *head;             /* Formatted http response head */
  char *body;             /* HTTP document body */
};
  
struct gateway
{
   /* generic attributes */
   char *name;
   int (*conn)(const struct gateway *, struct http_request *, struct client_state *);
   int   type;

   /* domain specific attributes */
   char *gateway_host;
   int   gateway_port;

   char *forward_host;
   int   forward_port;
};


/* A URL pattern */
struct url_spec
{
   char  *spec;
   char  *domain;        /* fqdn */
   char  *dbuf;
   char **dvec;          /* Domain ptr vector */
   int    dcnt;          /* How many domains in fqdn? */
   int    unanchored;    /* bitmap 0: left, 1: right */

   char *path;
   int   pathlen;
   int   port;
#ifdef REGEX
   regex_t *preg;
#endif
};

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



#define ACTION_MASK_ALL        (~0U)

#define ACTION_MOST_COMPATIBLE 0U

#define ACTION_BLOCK           0x0001U
#define ACTION_FAST_REDIRECTS  0x0002U
#define ACTION_FILTER          0x0004U
#define ACTION_HIDE_FORWARDED  0x0008U
#define ACTION_HIDE_FROM       0x0010U
#define ACTION_HIDE_REFERER    0x0020U /* sic - follow HTTP, not English */
#define ACTION_HIDE_USER_AGENT 0x0040U
#define ACTION_IMAGE           0x0080U
#define ACTION_IMAGE_BLOCKER   0x0100U
#define ACTION_NO_COOKIE_READ  0x0200U
#define ACTION_NO_COOKIE_SET   0x0400U
#define ACTION_NO_POPUPS       0x0800U
#define ACTION_VANILLA_WAFER   0x1000U

#define ACTION_STRING_FROM          0
#define ACTION_STRING_IMAGE_BLOCKER 1
#define ACTION_STRING_REFERER       2
#define ACTION_STRING_USER_AGENT    3
#define ACTION_STRING_COUNT         4

#define ACTION_MULTI_ADD_HEADER     0
#define ACTION_MULTI_WAFER          1
#define ACTION_MULTI_COUNT          2


struct current_action_spec
{
   unsigned flags;    /* a bit set to "1" = add action    */

   /* For those actions that require parameters: */

   /* each entry is valid if & only if corresponding entry in "add" set. */
   char * string[ACTION_STRING_COUNT];

   /* Strings to add */
   struct list multi[ACTION_MULTI_COUNT][1];
};

struct action_spec
{
   unsigned mask;   /* a bit set to "0" = remove action */
   unsigned add;    /* a bit set to "1" = add action    */

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

struct url_actions
{
   struct url_spec url[1];

   struct action_spec action[1];

   struct url_actions * next;
};


/* Constants defining bitmask for csp->accept_types */

#ifdef DETECT_MSIE_IMAGES

/* MSIE detected by user-agent string */
#define ACCEPT_TYPE_IS_MSIE     0x0001

/*
 * *If* this is MSIE, it wants an image.  (Or this is a shift-reload, or
 * it's got an image from this URL before...  yuck!)
 * Only meaningful if ACCEPT_TYPE_IS_MSIE set 
 */
#define ACCEPT_TYPE_MSIE_IMAGE  0x0002

/*
 * *If* this is MSIE, it wants a HTML document.
 * Only meaningful if ACCEPT_TYPE_IS_MSIE set
 */
#define ACCEPT_TYPE_MSIE_HTML   0x0004

#endif /* def DETECT_MSIE_IMAGES */


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


#ifdef STATISTICS
   /* 1 if this URL was rejected, 0 otherwise. Allows actual stats inc to 
    * occur in main thread only for thread-safety. 
    */
   int  rejected;
#endif /* def STATISTICS */

#ifdef FORCE_LOAD
   int force;
#endif /* def FORCE_LOAD */

#ifdef TOGGLE
   int   toggled_on;
#endif /* def TOGGLE */

   /*
    * Client PC's IP address, as reported by the accept()_ function.
    * Both as string and number
    */
   char *ip_addr_str;
   long  ip_addr_long;

#ifdef TRUST_FILES
   /* The referer in this request, if one was specified. */
   char *referrer;
#endif /* def TRUST_FILES */

#if defined(DETECT_MSIE_IMAGES)
   /* Types the client will accept.
    * Bitmask - see ACCEPT_TYPE_XXX constants.
    */
   int accept_types;
#endif /* defined(DETECT_MSIE_IMAGES) */

   /* The URL that was requested */
   struct http_request http[1];

   /* An I/O buffer used for buffering data read from the client */
   struct iob iob[1];

   /* List of all headers for this request */
   struct list headers[1];

   /* List of all cookies for this request */
   struct list cookie_list[1];

#if defined(PCRS) || defined(KILLPOPUPS)
   /* Nonzero if this has a text MIME type */
   int is_text;
#endif /* defined(PCRS) || defined(KILLPOPUPS) */

   /* The "X-Forwarded-For:" header sent by the client */
   char   *x_forwarded;

   /*
    * Nonzero if this client is processing data.
    * Set to zero when the thread associated with this structure dies.
    */
   int active;

   /* files associated with this client */
   struct file_list *flist;   /* forwardfile */
   struct file_list *actions_list;


#ifdef ACL_FILES
   struct file_list *alist;   /* aclfile */
#endif /* def ACL_FILES */

#ifdef PCRS
     struct file_list *rlist;   /* Perl re_filterfile */
     size_t content_length;     /* Length after processing */ 
#endif /* def PCRS */

#ifdef TRUST_FILES
   struct file_list *tlist;   /* trustfile */
#endif /* def TRUST_FILES */

   struct client_state *next;
};


struct parsers
{
   char *str;
   char  len;
   char *(*parser)(const struct parsers *, char *, struct client_state *);
};

struct cgi_dispatcher
{
   char *name;
   int   name_length;
   int   (*handler)(struct client_state *csp, struct http_response *rsp, struct map *parameters);
   char *description;
};

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

#ifndef SPLIT_PROXY_ARGS
   /* String to be displayed as part of show-proxy-args display.
    * Read-only once the structure has been created.
    */
   char *proxy_args;
#endif /* ndef SPLIT_PROXY_ARGS */

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


#ifdef TRUST_FILES
struct block_spec
{
   struct url_spec url[1];
   int    reject;
   struct block_spec *next;
};
#endif /* def TRUST_FILES */


struct forward_spec
{
   struct url_spec url[1];
   struct gateway gw[1];
   struct forward_spec *next;
};


#ifdef PCRS
struct re_filterfile_spec
{
   struct list patterns[1];
   pcrs_job *joblist;
};
#endif /* def PCRS */


#ifdef ACL_FILES
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
#endif /* def ACL_FILES */


/* Maximum number of loaders (actions, block, forward, acl...) */
#define NLOADERS 8

/*
 * Data loaded from the configuration file.
 *
 * (Anomaly: toggle is still handled through a global, not this structure)
 */
struct configuration_spec
{
   int debug;
   int multi_threaded;

   const char *logfile;

   const char *confdir;
   const char *actions_file;
   const char *forwardfile;

#ifdef ACL_FILES
   const char *aclfile;
#endif /* def ACL_FILES */

#ifdef PCRS
   const char *re_filterfile;
#endif /* def PCRS */

#ifdef JAR_FILES
   const char * jarfile;
   FILE * jar;
#endif /* def JAR_FILES */

   /*
    * Port and IP to bind to.
    * Defaults to HADDR_DEFAULT:HADDR_PORT == 127.0.0.1:8000
    */
   const char *haddr;
   int         hport;

#ifndef SPLIT_PROXY_ARGS
   const char *suppress_message;
#endif /* ndef SPLIT_PROXY_ARGS */

#ifndef SPLIT_PROXY_ARGS
   /* suppress listing config files */
   int suppress_blocklists;
#endif /* ndef SPLIT_PROXY_ARGS */

#ifdef TRUST_FILES
   const char * trustfile;

   struct list trust_info[1];
   struct url_spec *trust_list[64];
#endif /* def TRUST_FILES */

   /* Various strings for show-proxy-args */
   char *proxy_args_header;
   char *proxy_args_invocation;
   char *proxy_args_gateways;
   char *proxy_args_trailer;

   /* the configuration file object. */
   struct file_list *config_file_list;

   /* List of loaders */
   int (*loaders[NLOADERS])(struct client_state *);

   /* bool, nonzero if we need to bind() to the new port */
   int need_bind;
};


#define SZ(X)  (sizeof(X) / sizeof(*X))

#define WHITEBG   "<body bgcolor=\"#ffffff\" link=\"#000078\" alink=\"#ff0022\" vlink=\"#787878\">\n"
#define BODY      "<body bgcolor=\"#f8f8f0\" link=\"#000078\" alink=\"#ff0022\" vlink=\"#787878\">\n"
#define BANNER    "<strong>Internet J<small>UNK<i><font color=\"red\">BUSTER</font></i></small></strong>"

#ifdef FORCE_LOAD
#define FORCE_PREFIX "/IJB-FORCE-LOAD"
#endif /* def FORCE_LOAD */

/* Shouldn't end with '/' */
#define HOME_PAGE_URL  "http://ijbswa.sourceforge.net"
#define REDIRECT_URL HOME_PAGE_URL "/redirect.php?v=" VERSION "&to="

#define CGI_PREFIX_HOST "i.j.b"

static const char CSUCCEED[] =
   "HTTP/1.0 200 Connection established\n"
   "Proxy-Agent: IJ/" VERSION "\n\n";

static const char CHEADER[] =
   "HTTP/1.0 400 Invalid header received from browser\n\n";

static const char SHEADER[] =
   "HTTP/1.0 502 Invalid header received from server\n\n";

#ifdef IMAGE_BLOCKING

/*
 * Hint: You can encode your own GIFs like that:
 * perl -e 'while (read STDIN, $c, 1) { printf("\\%.3o,", unpack("C", $c)); }'
 */

static const char BLANKGIF[] =
   "HTTP/1.0 200 OK\r\n"
   "Pragma: no-cache\r\n"
   "Last-Modified: Thu Jul 31, 1997 07:42:22 pm GMT\r\n"
   "Expires:       Thu Jul 31, 1997 07:42:22 pm GMT\r\n"
   "Content-type: image/gif\r\n\r\n"
   "GIF89a\001\000\001\000\200\000\000\377\377\377\000\000"
   "\000!\371\004\001\000\000\000\000,\000\000\000\000\001"
   "\000\001\000\000\002\002D\001\000;";

static const char JBGIF[] =
   "HTTP/1.0 200 OK\r\n"
   "Pragma: no-cache\r\n"
   "Last-Modified: Thu Jul 31, 1997 07:42:22 pm GMT\r\n"
   "Expires:       Thu Jul 31, 1997 07:42:22 pm GMT\r\n"
   "Content-type: image/gif\r\n\r\n"
   "GIF89aD\000\013\000\360\000\000\000\000\000\377\377\377!"
   "\371\004\001\000\000\001\000,\000\000\000\000D\000\013\000"
   "\000\002a\214\217\251\313\355\277\000\200G&K\025\316hC\037"
   "\200\234\230Y\2309\235S\230\266\206\372J\253<\3131\253\271"
   "\270\215\342\254\013\203\371\202\264\334P\207\332\020o\266"
   "N\215I\332=\211\312\3513\266:\026AK)\364\370\365aobr\305"
   "\372\003S\275\274k2\354\254z\347?\335\274x\306^9\374\276"
   "\037Q\000\000;";

#endif /* def IMAGE_BLOCKING */

#if defined(FAST_REDIRECTS) || defined(IMAGE_BLOCKING)

static const char HTTP_REDIRECT_TEMPLATE[] =
      "HTTP/1.0 302 Local Redirect from Junkbuster\r\n" 
      "Pragma: no-cache\r\n"
      "Last-Modified: Thu Jul 31, 1997 07:42:22 pm GMT\r\n"
      "Expires:       Thu Jul 31, 1997 07:42:22 pm GMT\r\n"
      "Location: %s\r\n\r\n";

#endif /*  defined(FAST_REDIRECTS) || defined(IMAGE_BLOCKING) */

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef _PROJECT_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
