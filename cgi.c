const char cgi_rcs[] = "$Id: cgi.c,v 1.3 2001/06/03 19:12:16 oes Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/cgi.c,v $
 *
 * Purpose     :  Declares functions to intercept request, generate
 *                html or gif answers, and to compose HTTP resonses.
 *                
 *                Functions declared include:
 * 
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
 *    $Log: cgi.c,v $
 *    Revision 1.3  2001/06/03 19:12:16  oes
 *    introduced new cgi handling
 *
 *    Revision 1.1  2001/06/03 11:03:48  oes
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
 *
 *
 **********************************************************************/


#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "project.h"
#include "cgi.h"
#include "list.h"
#include "pcrs.h"
#include "encode.h"
#include "ssplit.h"
#include "jcc.h"
#include "filters.h"
#include "actions.h"
#include "errlog.h"
#include "miscutil.h"
#include "showargs.h"

const char cgi_h_rcs[] = CGI_H_VERSION;

const struct cgi_dispatcher cgi_dispatchers[] = {
   { "show-status", 
         11, cgi_show_status,  
         "Show information about the version and configuration" }, 
/* { "show-url-info",
         13, cgi_show_url_info, 
         "Show which actions apply to a URL and why"  },*/
   { "send-banner",
         11, cgi_send_banner, 
         "HIDE Send the transparent or \"Junkbuster\" gif" },
#ifdef TRUST_FILES
/* { "untrusted-url",
         15, ij_untrusted_url,
	      "HIDE Show why a URL was not trusted" }, */
#endif /* def TRUST_FILES */
   { "",
         0, cgi_default,
         "HIDE Send a page linking to all unhidden CGIs" },
   { NULL, 0, NULL, NULL }
};


/*********************************************************************
 * 
 * Function    :  dispatch_cgi
 *
 * Description :  Checks if a request URL has either the magical hostname
 *                i.j.b or matches HOME_PAGE_URL/config/. If so, it parses
 *                the (rest of the) path as a cgi name plus query string,
 *                prepares a map that maps CGI parameter names to their values,
 *                initializes the http_response struct, and calls the 
 *                relevant CGI handler function.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  http_response if match, NULL if nonmatch or handler fail
 *
 *********************************************************************/
struct http_response *cgi_dispatch(struct client_state *csp)
{
   char *argstring = NULL;
   const struct cgi_dispatcher *d;
   struct map *param_list;
   struct http_response *response;

   /*
    * Should we intercept ?
    */

   /* Either the host matches CGI_PREFIX_HOST ..*/
   if (0 == strcmpic(csp->http->host, CGI_PREFIX_HOST))
   {
      /* ..then the path will all be for us */
      argstring = csp->http->path;
   }
   /* Or it's the host part of HOME_PAGE_URL ? */
   else if (   (0 == strcmpic(csp->http->host, *&HOME_PAGE_URL + 7 ))
            && (0 == strncmpic(csp->http->path,"/config", 7))
            && ((csp->http->path[7] == '/') || (csp->http->path[7] == '\0')))
   {
      /* then it's everything following "/config" */
      argstring = csp->http->path + 7;
   }
   else
   {
      return NULL;
   }

   /* 
    * We have intercepted it.
    */

   /* Get mem for response */
   if (NULL == ( response = zalloc(sizeof(*response))))
   {
      return NULL;
   }

   /* remove any leading slash */
   if (*argstring == '/')
   {
      argstring++;
   }

   log_error(LOG_LEVEL_GPC, "%s%s cgi call", csp->http->hostport, csp->http->path);
   log_error(LOG_LEVEL_CLF, "%s - - [%T] \"%s\" 200 3", 
                            csp->ip_addr_str, csp->http->cmd); 

   for (d = cgi_dispatchers; d->handler; d++)
   {
      if (strncmp(argstring, d->name, d->name_length) == 0)
      {
         param_list = parse_cgi(argstring + d->name_length);
         if ((d->handler)(csp, response, param_list))
	      {
	         freez(response);
	      }

	      free_map(param_list);
	      return(response);
      }
   }

   freez(response);
   return(NULL);

}


/*********************************************************************
 *
 * Function    :  parse_cgi
 *
 * Description :  Parse a URL-encoded argument string into name/value
 *                pairs and store them in a struct map list.
 *
 * Parameters  :
 *          1  :  string = string to be parsed 
 *
 * Returns     :  poniter to param list, or NULL if failiure
 *
 *********************************************************************/
struct map *parse_cgi(char *argstring)
{
   char *tmp, *p;
   char *vector[BUFSIZ];
   int pairs, i;
   struct map *cgi_params = NULL;

   if(*argstring == '?') argstring++;
   tmp = strdup(argstring);

   pairs = ssplit(tmp, "&", vector, SZ(vector), 1, 1);

   for (i = 0; i < pairs; i++)
   {
      if ((NULL != (p = strchr(vector[i], '='))) && (*(p+1) != '\0'))
      {
         *p = '\0';
         cgi_params = map(cgi_params, url_decode(vector[i]), 0, url_decode(++p), 0);
      }
   }

   free(tmp);
   return(cgi_params);

}


/*********************************************************************
 *
 * Function    :  make_http_response
 *
 * Description :  Fill in the missing headers in an http response,
 *                and flatten the headers to an http head.
 *
 * Parameters  :
 *          1  :  rsp = pointer to http_response to be processed
 *
 * Returns     :  length of http head, or 0 on failiure
 *
 *********************************************************************/
int make_http_response(struct http_response *rsp)
{
  char buf[BUFSIZ];

  /* Fill in the HTTP Status */
  sprintf(buf, "HTTP/1.0 %s", rsp->status ? rsp->status : "200 OK");
  enlist_first(rsp->headers, buf);

  /* Set the Content-Length */
  if (rsp->content_length == 0)
  {
     rsp->content_length = rsp->body ? strlen(rsp->body) : 0;
  }


  sprintf(buf, "Content-Length: %d", rsp->content_length);
  enlist(rsp->headers, buf);

  /* Fill in the default headers FIXME: Are these correct? sequence OK? check rfc! */
  enlist_unique(rsp->headers, "Pragma: no-cache", 7);
  enlist_unique(rsp->headers, "Last-Modified: Thu Jul 31, 1997 07:42:22 pm GMT", 14);
  enlist_unique(rsp->headers, "Expires:       Thu Jul 31, 1997 07:42:22 pm GMT", 8);
  enlist_unique(rsp->headers, "Content-Type: text/html", 13);
  enlist(rsp->headers, "");
  

  /* Write the head */
  if (NULL == (rsp->head = list_to_text(rsp->headers)))
  {
    free_http_response(rsp);
    return(0);
  }
 
  return(strlen(rsp->head));
}
  

/*********************************************************************
 *
 * Function    :  free_http_response
 *
 * Description :  Free the memory occupied by an http_response
 *                and its depandant structures.
 *
 * Parameters  :
 *          1  :  rsp = pointer to http_response to be freed
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void free_http_response(struct http_response *rsp)
{
   if(rsp)
   {
      freez(rsp->status);
      freez(rsp->head);
      freez(rsp->body);
      destroy_list(rsp->headers);
   }
}


/*********************************************************************
 *
 * Function    :  fill_template
 *
 * Description :  CGI support function that loads a given HTML
 *                template from the confdir, and fills it in
 *                by replacing @name@ with value using pcrs,
 *                for each item in the output map.
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           3 :  template = name of the HTML template to be used
 *           2 :  answers = map with fill in symbol -> name pairs
 *                FIXME: needs better name!
 *
 * Returns     :  char * with filled out form, or NULL if failiure
 *
 *********************************************************************/
char *fill_template(struct client_state *csp, char *template, struct map *answers)
{
   struct map *m;
   pcrs_job *job, *joblist = NULL;
   char buf[BUFSIZ];
   char *new, *old = NULL;
   int error, size;
   FILE *fp;

   /*
    * Open template file or fail
    */
   snprintf(buf, BUFSIZ, "%s/templates/%s", csp->config->confdir, template);

   if(NULL == (fp = fopen(buf, "r")))
	{
	   log_error(LOG_LEVEL_ERROR, "error loading template %s: %E", buf);
      return NULL;
	}
	
   /* 
    * Assemble pcrs joblist from answers map
    */
   for (m = answers; m; m = m->next)
	{
	   int error;

	   snprintf(buf, BUFSIZ, "s°@%s@°%s°ig", m->name, m->value);

	   if(NULL == (job = pcrs_make_job(buf, &error)))
		{
		  log_error(LOG_LEVEL_ERROR, "Adding template fill job %s failed with error %d",
						buf, error);
		  while ( NULL != (joblist = pcrs_free_job(joblist)) ) {};
		  return NULL;
		}
		else
		{
		   job->next = joblist;
			joblist = job;
		}
	}

   /* 
    * Read the file, ignoring comments
    */
	while (fgets(buf, BUFSIZ, fp))
	{
      /* skip lines starting with '#' */
	   if(*buf == '#') continue;
	
      old = strsav(old, buf);
	}
	fclose(fp);

   /*
    * Execute the jobs
    */
  	size = strlen(old) + 1;
   new = old;

   for (job = joblist; NULL != job; job = job->next)
   {
	   pcrs_exec_substitution(job, old, size, &new, &size);
      if (old != buf) free(old);
      old=new;
	}

   /*
    * Free the jobs & return
    */
   while ( NULL != (joblist = pcrs_free_job(joblist)) ) {};
   return(new);

}


/*********************************************************************
 *
 * Function    :  dump_map
 *
 * Description :  HTML-dump a map for debugging
 *
 * Parameters  :
 *          1  :  map = map to dump
 *
 * Returns     :  string with HTML
 *
 *********************************************************************/
char *dump_map(struct map *map)
{
   struct map *p = map;
   char *ret = NULL;


   ret = strsav(ret, "<table>\n");

   while (p)
   {
      ret = strsav(ret, "<tr><td><b>");
      ret = strsav(ret, p->name);
      ret = strsav(ret, "</b></td><td>");
      ret = strsav(ret, p->value);
      ret = strsav(ret, "</td></tr>\n");
      p = p->next;
   }

   ret = strsav(ret, "</table>\n");
   return(ret);
}


/*********************************************************************
 *
 * Function    :  cgi_default
 *
 * Description :  CGI function that is called if no action was given
 *                lists menu of available unhidden CGIs.
 *               
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_default(struct client_state *csp, struct http_response *rsp,
                struct map *parameters)
{
   char *p, *tmp = NULL;
   char buf[BUFSIZ];
   const struct cgi_dispatcher *d;
	struct map *exports = NULL;

   /* List available unhidden CGI's and export as "other-cgis" */
   for (d = cgi_dispatchers; d->handler; d++)
   {
      if (strncmp(d->description, "HIDE", 4))
	   {
         snprintf(buf, BUFSIZ, "<li><a href=%s/config/%s>%s</a></li>",
				  HOME_PAGE_URL, d->name, d->description);
         tmp = strsav(tmp, buf);
      }
	}
	exports = map(exports, "other-cgis", 1, tmp, 0);

   /* If there were other parameters, export a dump as "cgi-parameters" */
   if(parameters)
	{
      p = dump_map(parameters);
	   tmp = strsav(tmp, "<p>What made you think this cgi takes options?\n
                         Anyway, here they are, in case you're interested:</p>\n");
		tmp = strsav(tmp, p);
		exports = map(exports, "cgi-parameters", 1, tmp, 0);
      free(p);
	}
	else
	{
	   exports = map(exports, "cgi-parameters", 1, "", 1);
	}

   rsp->body = fill_template(csp, "default", exports);

   free_map(exports);
   return(0);

}


/*********************************************************************
 *
 * Function    :  cgi_send_banner
 *
 * Description :  CGI function that returns a banner. 
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * CGI Parameters :
 *           type : Selects the type of banner between "trans" and "jb".
 *                  Defaults to "jb" if absent or != "trans".
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_send_banner(struct client_state *csp, struct http_response *rsp,
                    struct map *parameters)
{
   if(strcmp(lookup(parameters, "type"), "trans"))
   {
     rsp->body = bindup(CJBGIF, sizeof(CJBGIF));
     rsp->content_length = sizeof(CJBGIF);
   }
   else
   {
     rsp->body = bindup(CBLANKGIF, sizeof(CBLANKGIF));
     rsp->content_length = sizeof(CBLANKGIF);
   }   

   enlist(rsp->headers, "Content-Type: image/gif");

   return(0);
}


#ifdef FAST_REDIRECTS
/*********************************************************************
 *
 * Function    :  redirect_url
 *
 * Description :  Checks for redirection URLs and returns a HTTP redirect
 *                to the destination URL.
 *
 * Parameters  :
 *          1  :  http = http_request request, check `basename's of blocklist
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  NULL if URL was clean, HTTP redirect otherwise.
 *
 *********************************************************************/
char *redirect_url(struct http_request *http, struct client_state *csp)
{
   char *p, *q;

   p = q = csp->http->path;
   log_error(LOG_LEVEL_REDIRECTS, "checking path: %s", p);

   /* find the last URL encoded in the request */
   while (p = strstr(p, "http://"))
   {
      q = p++;
   }

   /* if there was any, generate and return a HTTP redirect */
   if (q != csp->http->path)
   {
      log_error(LOG_LEVEL_REDIRECTS, "redirecting to: %s", q);

      p = (char *)malloc(strlen(HTTP_REDIRECT_TEMPLATE) + strlen(q));
      sprintf(p, HTTP_REDIRECT_TEMPLATE, q);
      return(p);
   }
   else
   {
      return(NULL);
   }

}
#endif /* def FAST_REDIRECTS */



/*********************************************************************
 *
 * Function    :  cgi_show_status
 *
 * Description :  CGI function that returns a a web page describing the
 *                current status of IJB.
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * CGI Parameters :
 *           type : Selects the type of banner between "trans" and "jb".
 *                  Defaults to "jb" if absent or != "trans".
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_show_status(struct client_state *csp, struct http_response *rsp,
                    struct map *parameters)
{
   char *s = NULL;
   const struct gateway *g;
   int i;

   struct map *exports = NULL;

#ifdef SPLIT_PROXY_ARGS
   FILE * fp;
   char buf[BUFSIZ];
   char * p;
   const char * filename = NULL;
   char * file_description = NULL;


   p = lookup(parameters, "file");
   switch (*p)
   {
   case 'p':
      if (csp->actions_list)
      {
         filename = csp->actions_list->filename;
         file_description = "Actions List";
      }
      break;
   case 'f':
      if (csp->flist)
      {
         filename = csp->flist->filename;
         file_description = "Forward List";
      }
      break;

#ifdef ACL_FILES
   case 'a':
      if (csp->alist)
      {
         filename = csp->alist->filename;
         file_description = "Access Control List";
      }
      break;
#endif /* def ACL_FILES */

#ifdef PCRS
   case 'r':
      if (csp->rlist)
      {
         filename = csp->rlist->filename;
         file_description = "Regex Filter List";
      }
      break;
#endif /* def PCRS */

#ifdef TRUST_FILES
   case 't':
      if (csp->tlist)
      {
         filename = csp->tlist->filename;
         file_description = "Trust List";
      }
      break;
#endif /* def TRUST_FILES */
   }

   if (NULL != filename)
   {
 	   exports = map(exports, "filename", 1, file_description, 1);
      exports = map(exports, "filepath", 1, html_encode(filename), 0);

      if ((fp = fopen(filename, "r")) == NULL)
      {
         exports = map(exports, "content", 1, "</pre><h1>ERROR OPENING FILE!</h1><pre>", 1);
      }
      else
      {
         while (fgets(buf, sizeof(buf), fp))
         {
            p = html_encode(buf);
            if (p)
            {
               s = strsav(s, p);
               freez(p);
               s = strsav(s, "<br>");
            }
         }
         fclose(fp);
         exports = map(exports, "contents", 1, s, 0);
     }
         rsp->body = fill_template(csp, "show-status-file", exports);;
         free_map(exports);
         return(0);

   }

#endif /* def SPLIT_PROXY_ARGS */

   exports = map(exports, "redirect-url", 1, REDIRECT_URL, 1);
   exports = map(exports, "version", 1, VERSION, 1);
   exports = map(exports, "home-page", 1, HOME_PAGE_URL, 1);
   exports = map(exports, "invocation-args", 1, csp->config->proxy_args_header, 1);
   exports = map(exports, "gateways", 1, csp->config->proxy_args_gateways, 1);
   exports = map(exports, "gateway-protocols", 1, s, 0);


#ifdef STATISTICS
   exports = map(exports, "statistics", 1, add_stats(NULL), 0);
#else
   exports = map(exports, "statistics", 1, "", 1);
#endif /* ndef STATISTICS */

#ifdef SPLIT_PROXY_ARGS
   if (csp->actions_list)
   {
      exports = map(exports, "actions-filename", 1,  csp->actions_list->filename, 1);
	}
   else
	{
 	   exports = map(exports, "actions-filename", 1, "None specified", 1);
	}

   if (csp->flist)
   {
      exports = map(exports, "forward-filename", 1,  csp->flist->filename, 1);
	}
   else
	{
 	   exports = map(exports, "forward-filename", 1, "None specified", 1);
	}

#ifdef ACL_FILES
   if (csp->alist)
   {
      exports = map(exports, "acl-filename", 1,  csp->alist->filename, 1);
	}
   else
	{
 	   exports = map(exports, "acl-filename", 1, "None specified", 1);
	}
#else
   exports = map(exports, "acl-killer-start.*acl-killer-end", 1, "", 1);
#endif /* ndef ACL_FILES */

#ifdef PCRS
   if (csp->rlist)
   {
      exports = map(exports, "re-filter-filename", 1,  csp->rlist->filename, 1);
	}
   else
	{
 	   exports = map(exports, "re-filter-filename", 1, "None specified", 1);
	}
#else
   exports = map(exports, "re-filter-killer-start.*re-filter-killer-end", 1, "", 1);
#endif /* ndef PCRS */

#ifdef TRUST_FILES
   if (csp->tlist)
   {
      exports = map(exports, "trust-filename", 1,  csp->tlist->filename, 1);
	}
   else
	{
 	   exports = map(exports, "trust-filename", 1, "None specified", 1);
	}
#else
   exports = map(exports, "acl-killer-start.*acl-killer-end", 1, "", 1);
#endif /* ndef TRUST_FILES */

   exports = map(exports, ".list", 1, "" , 1);

#else /* ifndef SPLIT_PROXY_ARGS */
   exports = map(exports, "magic-eliminator-start.*magic-eliminator-end", 1, "", 1);

   if (csp->clist)
   {
      map(exports, "clist", 1, csp->clist->proxy_args , 1);
   }

   if (csp->flist)
   {
      map(exports, "flist", 1, csp->flist->proxy_args , 1);
	}

#ifdef ACL_FILES
   if (csp->alist)
   {
      map(exports, "alist", 1, csp->alist->proxy_args , 1);
	}
#endif /* def ACL_FILES */

#ifdef PCRS
   if (csp->rlist)
   {
      map(exports, "rlist", 1, csp->rlist->proxy_args , 1);
	}
#endif /* def PCRS */

#ifdef TRUST_FILES
    if (csp->tlist)
   {
      map(exports, "tlist", 1, csp->tlist->proxy_args , 1);
	}
#endif /* def TRUST_FILES */

#endif /* ndef SPLIT_PROXY_ARGS */

	s = end_proxy_args(csp->config);
   exports = map(exports, "rcs-and-defines", 1, s , 0);


   rsp->body = fill_template(csp, "show-status", exports);
   free_map(exports);
   return(0);

}

 
 /*********************************************************************
 *
 * Function    :  cgi_show_url_info
 *
 * Description :  (please fill me in)
 *
 * Parameters  :
 *          1  :  http = http_request request for crunched URL
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  ???FIXME
 *
 *********************************************************************/
char *cgi_show_url_info(struct http_request *http, struct client_state *csp)
{
   char * query_string = strchr(http->path, '?');
   char * host = NULL;
   
   if (query_string != NULL)
   {
      query_string = url_decode(query_string + 1);
      if (strncmpic(query_string, "url=", 4) == 0)
      {
         host = strdup(query_string + 4);
      }
      freez(query_string);
   }
   if (host != NULL)
   {
      char * result;
      char * path;
      char * s;
      int port = 80;
      struct file_list *fl;
      struct url_actions *b;
      struct url_spec url[1];
      struct current_action_spec action[1];

      init_current_action(action);

      result = (char *)malloc(sizeof(C_URL_INFO_HEADER) + 2 * strlen(host));
      sprintf(result, C_URL_INFO_HEADER, host, host);

      s = current_action_to_text(action);
      result = strsav(result, "<h3>Defaults:</h3>\n<p><b>{");
      result = strsav(result, s);
      result = strsav(result, " }</b></p>\n<h3>Patterns affecting the URL:</h3>\n<p>\n");
      freez(s);

      s = strchr(host, '/');
      if (s != NULL)
      {
         path = strdup(s);
         *s = '\0';
      }
      else
      {
         path = strdup("");
      }
      s = strchr(host, ':');
      if (s != NULL)
      {
         *s++ = '\0';
         port = atoi(s);
      }

      if (((fl = csp->actions_list) == NULL) || ((b = fl->f) == NULL))
      {
         freez(host);
         freez(path);
         result = strsav(result, C_URL_INFO_FOOTER);
         return result;
      }

      *url = dsplit(host);

      /* if splitting the domain fails, punt */
      if (url->dbuf == NULL)
      {
         freez(host);
         freez(path);
         result = strsav(result, C_URL_INFO_FOOTER);
         return result;
      }

      for (b = b->next; NULL != b; b = b->next)
      {
         if ((b->url->port == 0) || (b->url->port == port))
         {
            if ((b->url->domain[0] == '\0') || (domaincmp(b->url, url) == 0))
            {
               if ((b->url->path == NULL) ||
#ifdef REGEX
                  (regexec(b->url->preg, path, 0, NULL, 0) == 0)
#else
                  (strncmp(b->url->path, path, b->url->pathlen) == 0)
#endif
               )
               {
                  s = actions_to_text(b->action);
                  result = strsav(result, "<b>{");
                  result = strsav(result, s);
                  result = strsav(result, " }</b><br>\n<code>");
                  result = strsav(result, b->url->spec);
                  result = strsav(result, "</code><br>\n<br>\n");
                  freez(s);

                  merge_current_action(action, b->action);
               }
            }
         }
      }

      freez(url->dbuf);
      freez(url->dvec);

      freez(host);
      freez(path);

      s = current_action_to_text(action);
      result = strsav(result, "</p>\n<h2>Final Results:</h2>\n<p><b>{");
      result = strsav(result, s);
      result = strsav(result, " }</b><br>\n<br>\n");
      freez(s);

      free_current_action(action);

      result = strsav(result, C_URL_INFO_FOOTER);
      return result;
   }
   else
   {
      return strdup(C_URL_INFO_FORM);
   }
}



#ifdef TRUST_FILES
/*********************************************************************
 *
 * Function    :  ij_untrusted_url
 *
 * Description :  This "crunch"es "http:/any.thing/ij-untrusted-url" and
 *                returns a web page describing why it was untrusted.
 *
 * Parameters  :
 *          1  :  http = http_request request for crunched URL
 *          2  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  A string that contains why this was untrusted.
 *
 *********************************************************************/
char *ij_untrusted_url(struct http_request *http, struct client_state *csp)
{
   int n;
   char *hostport, *path, *refer, *p, *v[9];
   char buf[BUFSIZ];
   struct url_spec **tl, *t;


   static const char format[] =
      "HTTP/1.0 200 OK\r\n"
      "Pragma: no-cache\n"
      "Last-Modified: Thu Jul 31, 1997 07:42:22 pm GMT\n"
      "Expires:       Thu Jul 31, 1997 07:42:22 pm GMT\n"
      "Content-Type: text/html\n\n"
      "<html>\n"
      "<head>\n"
      "<title>Internet Junkbuster: Request for untrusted URL</title>\n"
      "</head>\n"
      BODY
      "<center><h1>"
      BANNER
      "</h1></center>"
      "The " BANNER " Proxy "
      "<A href=\"" HOME_PAGE_URL "\">"
      "(" HOME_PAGE_URL ") </A>"
      "intercepted the request for %s%s\n"
      "because the URL is not trusted.\n"
      "<br><br>\n";

   if ((n = ssplit(http->path, "?+", v, SZ(v), 0, 0)) == 4)
   {
      hostport = url_decode(v[1]);
      path     = url_decode(v[2]);
      refer    = url_decode(v[3]);
   }
   else
   {
      hostport = strdup("undefined_host");
      path     = strdup("/undefined_path");
      refer    = strdup("undefined");
   }

   n  = sizeof(format);
   n += strlen(hostport);
   n += strlen(path    );

   if ((p = (char *)malloc(n)))
   {
      sprintf(p, format, hostport, path);
   }

   strsav(p, "The referrer in this request was <strong>");
   strsav(p, refer);
   strsav(p, "</strong><br>\n");

   freez(hostport);
   freez(path    );
   freez(refer   );

   p = strsav(p, "<h3>The following referrers are trusted</h3>\n");

   for (tl = csp->config->trust_list; (t = *tl) ; tl++)
   {
      sprintf(buf, "%s<br>\n", t->spec);
      p = strsav(p, buf);
   }

   if (csp->config->trust_info->next)
   {
      struct list *l;

      strcpy(buf,
         "<p>"
         "You can learn more about what this means "
         "and what you may be able to do about it by "
         "reading the following documents:<br>\n"
         "<ol>\n"
      );

      p = strsav(p, buf);

      for (l = csp->config->trust_info->next; l ; l = l->next)
      {
         sprintf(buf,
            "<li> <a href=%s>%s</a><br>\n",
               l->str, l->str);
         p = strsav(p, buf);
      }

      p = strsav(p, "</ol>\n");
   }

   p = strsav(p, "</body>\n" "</html>\n");

   return(p);

}
#endif /* def TRUST_FILES */


#ifdef STATISTICS
/*********************************************************************
 *
 * Function    :  add_stats
 *
 * Description :  Statistics function of JB.  Called by `show_proxy_args'.
 *
 * Parameters  :
 *          1  :  s = string that holds the proxy args description page
 *
 * Returns     :  A pointer to the descriptive status web page.
 *
 *********************************************************************/
char *add_stats(char *s)
{
   /*
    * Output details of the number of requests rejected and
    * accepted. This is switchable in the junkbuster config.
    * Does nothing if this option is not enabled.
    */

   float perc_rej;   /* Percentage of http requests rejected */
   char out_str[81];
   int local_urls_read     = urls_read;
   int local_urls_rejected = urls_rejected;

   /*
    * Need to alter the stats not to include the fetch of this
    * page.
    *
    * Can't do following thread safely! doh!
    *
    * urls_read--;
    * urls_rejected--; * This will be incremented subsequently *
    */

   s = strsav(s,"<h2>Statistics for this " BANNER ":</h2>\n");

   if (local_urls_read == 0)
   {

      s = strsav(s,"No activity so far!\n");

   }
   else
   {

      perc_rej = (float)local_urls_rejected * 100.0F /
            (float)local_urls_read;

      sprintf(out_str,
         "%d requests received, %d filtered "
         "(%6.2f %%).",
         local_urls_read, 
         local_urls_rejected, perc_rej);

      s = strsav(s,out_str);
   }

   return(s);
}
#endif /* def STATISTICS */

/*
  Local Variables:
  tab-width: 3
  end:
*/
