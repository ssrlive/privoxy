#ifndef _PROJECT_H
#define _PROJECT_H
#define PROJECT_H_VERSION "$Id: project.h,v 1.1.1.1 2001/05/15 13:59:03 oes Exp $"
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


struct proxy_args
{
   char *header;
   char *invocation;
   char *gateways;
   char *trailer;
};


struct iob
{
   char *buf;
   char *cur;
   char *eod;
};


struct list
{
   char *str;
   struct list *last;
   struct list *next;
};

#define IOB_PEEK(CSP) ((CSP->iob->cur > CSP->iob->eod) ? (CSP->iob->eod - CSP->iob->cur) : 0)
#define IOB_RESET(CSP) if(CSP->iob->buf) free(CSP->iob->buf); memset(CSP->iob, '\0', sizeof(CSP->iob));


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
   int  send_user_cookie;
   int  accept_server_cookie;
   int  cfd;
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
   /*
    * by haroon - most of credit to srt19170
    * We add an "on/off" toggle here that is used to effectively toggle
    * the Junkbuster off or on
    */
   int   toggled_on;
#endif

   char *ip_addr_str;
   long  ip_addr_long;
   char *referrer;

#if defined(DETECT_MSIE_IMAGES)
   /* Types the client will accept.
    * Bitmask - see ACCEPT_TYPE_XXX constants.
    */
   int accept_types;
#endif /* defined(DETECT_MSIE_IMAGES) */

   const struct gateway *gw;
   struct http_request http[1];

   struct iob iob[1];

   struct list headers[1];
   struct list cookie_list[1];
#ifdef PCRS
   int is_text;
#endif /* def PCRS */

   char   *x_forwarded;

   int active;

   /* files associated with this client */
   struct file_list *blist;   /* blockfile */
   struct file_list *clist;   /* cookiefile */
   struct file_list *flist;   /* forwardfile */

#ifdef ACL_FILES
   struct file_list *alist;   /* aclfile */
#endif /* def ACL_FILES */

#ifdef USE_IMAGE_LIST
   struct file_list *ilist;   /* imagefile */
#endif /* def USE_IMAGE_LIST */

#ifdef KILLPOPUPS
   struct file_list *plist;   /* kill popup file */
#endif /* def KILLPOPUPS */

#ifdef PCRS
     struct file_list *rlist;   /* Perl re_filterfile */
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


struct interceptors
{
   char *str;
   char  len;
   char *(*interceptor)(struct http_request *http, struct client_state *csp);
};


/* this allows the proxy to permit/block access to any host and/or path */

struct url_spec
{
   char  *spec;
   char  *domain;
   char  *dbuf;
   char **dvec;
   int    dcnt;
   int    toplevel;

   char *path;
   int   pathlen;
   int   port;
#ifdef REGEX
   regex_t *preg;
#endif
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


struct block_spec
{
   struct url_spec url[1];
   int   reject;
   struct block_spec *next;
};


struct cookie_spec
{
   struct url_spec url[1];
   int   send_user_cookie;
   int   accept_server_cookie;
   struct cookie_spec *next;
};


struct forward_spec
{
   struct url_spec url[1];
   int   reject;
   struct gateway gw[1];
   struct forward_spec *next;
};


#ifdef PCRS
struct re_filterfile_spec
{
   struct list patterns[1];
   /* See README.re_filter */
   pcrs_job *joblist;
};
#endif /* def PCRS */


#ifdef KILLPOPUPS
/* Entries on popup blocklist */
struct popup_blocklist
{
   char *host_name;
   struct popup_blocklist *next;
};

/* Actual type used in file object */
struct popup_settings
{
   struct popup_blocklist *blocked;
   struct popup_blocklist *allowed;
};
#endif /* def KILLPOPUPS */

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

#define SZ(X)  (sizeof(X) / sizeof(*X))

#define WHITEBG   "<body bgcolor=\"#ffffff\" link=\"#000078\" alink=\"#ff0022\" vlink=\"#787878\">\n"
#define BODY      "<body bgcolor=\"#f8f8f0\" link=\"#000078\" alink=\"#ff0022\" vlink=\"#787878\">\n"
#define BANNER    "<strong>Internet J<small>UNK<i><font color=\"red\">BUSTER</font></i></small></strong>"

#ifdef FORCE_LOAD
/*
 * FIXME: Unfortunately, IE lowercases the domain name.  JunkBuster does
 * a case-sensitive compare.  JunkBuster should be modified to do a
 * case-insensitive compatison.  As a temporary workaround, I've lowercased
 * the FORCE_PREFIX.
 *
 * #define FORCE_PREFIX "IJB-FORCE-LOAD-"
 */
#define FORCE_PREFIX "ijb-force-load-"
#endif /* def FORCE_LOAD */

#define HOME_PAGE_URL  "http://ijbswa.sourceforge.net/"
#define REDIRECT_URL HOME_PAGE_URL "redirect.php?v=" VERSION "&to="

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef _PROJECT_H */

/*
  Local Variables:
  tab-width: 3
  end:
*/
