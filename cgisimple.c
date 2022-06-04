/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/cgisimple.c,v $
 *
 * Purpose     :  Simple CGIs to get information about Privoxy's
 *                status.
 *
 * Copyright   :  Written by and Copyright (C) 2001-2022 the
 *                Privoxy team. https://www.privoxy.org/
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
 **********************************************************************/


#include "config.h"

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#if defined (HAVE_ACCESS) && defined (HAVE_UNISTD_H)
#include <unistd.h>
#endif /* def HAVE_ACCESS && HAVE_UNISTD_H */

#include "project.h"
#include "cgi.h"
#include "cgisimple.h"
#include "list.h"
#include "encode.h"
#include "jcc.h"
#include "filters.h"
#include "actions.h"
#include "miscutil.h"
#include "loadcfg.h"
#include "parsers.h"
#include "urlmatch.h"
#include "errlog.h"
#ifdef FEATURE_CLIENT_TAGS
#include "client-tags.h"
#endif

static jb_err show_defines(struct map *exports);
static jb_err cgi_show_file(struct client_state *csp,
                            struct http_response *rsp,
                            const struct map *parameters);
static jb_err load_file(const char *filename, char **buffer, size_t *length);

/*********************************************************************
 *
 * Function    :  cgi_default
 *
 * Description :  CGI function that is called for the CGI_SITE_1_HOST
 *                and CGI_SITE_2_HOST/CGI_SITE_2_PATH base URLs.
 *                Boring - only exports the default exports.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : none
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory
 *
 *********************************************************************/
jb_err cgi_default(struct client_state *csp,
                   struct http_response *rsp,
                   const struct map *parameters)
{
   struct map *exports;

   (void)parameters;

   assert(csp);
   assert(rsp);

   if (NULL == (exports = default_exports(csp, "")))
   {
      return JB_ERR_MEMORY;
   }

   return template_fill_for_cgi(csp, "default", exports, rsp);
}


/*********************************************************************
 *
 * Function    :  cgi_error_404
 *
 * Description :  CGI function that is called if an unknown action was
 *                given.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : none
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_error_404(struct client_state *csp,
                     struct http_response *rsp,
                     const struct map *parameters)
{
   struct map *exports;

   assert(csp);
   assert(rsp);
   assert(parameters);

   if (NULL == (exports = default_exports(csp, NULL)))
   {
      return JB_ERR_MEMORY;
   }

   rsp->status = strdup_or_die("404 Privoxy configuration page not found");

   return template_fill_for_cgi(csp, "cgi-error-404", exports, rsp);
}


#ifdef FEATURE_GRACEFUL_TERMINATION
/*********************************************************************
 *
 * Function    :  cgi_die
 *
 * Description :  CGI function to shut down Privoxy.
 *                NOTE: Turning this on in a production build
 *                would be a BAD idea.  An EXTREMELY BAD idea.
 *                In short, don't do it.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : none
 *
 * Returns     :  JB_ERR_OK on success
 *
 *********************************************************************/
jb_err cgi_die (struct client_state *csp,
                struct http_response *rsp,
                const struct map *parameters)
{
   static const char status[] = "200 OK Privoxy shutdown request received";
   static const char body[] =
      "<html>\n"
      "<head>\n"
      " <title>Privoxy shutdown request received</title>\n"
      " <link rel=\"shortcut icon\" href=\"" CGI_PREFIX "error-favicon.ico\" type=\"image/x-icon\">\n"
      " <link rel=\"stylesheet\" type=\"text/css\" href=\"" CGI_PREFIX "send-stylesheet\">\n"
      "</head>\n"
      "<body>\n"
      "<h1>Privoxy shutdown request received</h1>\n"
      "<p>Privoxy is going to shut down after the next request.</p>\n"
      "</body>\n"
      "</html>\n";

   assert(csp);
   assert(rsp);
   assert(parameters);

   /* quit */
   g_terminate = 1;

   csp->flags &= ~CSP_FLAG_CLIENT_CONNECTION_KEEP_ALIVE;

   rsp->content_length = 0;
   rsp->head_length = 0;
   rsp->is_static = 0;

   rsp->body = strdup_or_die(body);
   rsp->status = strdup_or_die(status);

   return JB_ERR_OK;
}
#endif /* def FEATURE_GRACEFUL_TERMINATION */


/*********************************************************************
 *
 * Function    :  cgi_show_request
 *
 * Description :  Show the client's request and what sed() would have
 *                made of it.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : none
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_show_request(struct client_state *csp,
                        struct http_response *rsp,
                        const struct map *parameters)
{
   char *p;
   struct map *exports;

   assert(csp);
   assert(rsp);
   assert(parameters);

   if (NULL == (exports = default_exports(csp, "show-request")))
   {
      return JB_ERR_MEMORY;
   }

   /*
    * Repair the damage done to the IOB by get_header()
    */
   for (p = csp->client_iob->buf; p < csp->client_iob->cur; p++)
   {
      if (*p == '\0') *p = '\n';
   }

   /*
    * Export the original client's request and the one we would
    * be sending to the server if this wasn't a CGI call
    */

   if (map(exports, "client-request", 1, html_encode(csp->client_iob->buf), 0))
   {
      free_map(exports);
      return JB_ERR_MEMORY;
   }

   if (map(exports, "processed-request", 1,
         html_encode_and_free_original(
#ifdef FEATURE_HTTPS_INSPECTION
                                       csp->http->ssl ?
                                       list_to_text(csp->https_headers) :
#endif
                                       list_to_text(csp->headers)
                                       ), 0))
   {
      free_map(exports);
      return JB_ERR_MEMORY;
   }

   return template_fill_for_cgi(csp, "show-request", exports, rsp);
}


#ifdef FEATURE_CLIENT_TAGS
/*********************************************************************
 *
 * Function    :  cgi_create_client_tag_form
 *
 * Description :  Creates a HTML form to enable or disable a given
 *                client tag.
 *                XXX: Could use a template.
 *
 * Parameters  :
 *          1  :  form = Buffer to fill with the generated form
 *          2  :  size = Size of the form buffer
 *          3  :  tag = Name of the tag this form should affect
 *          4  :  toggle_state = Desired state after the button pressed 0
 *          5  :  expires = Whether or not the tag should be enabled.
 *                          Only checked if toggle_state is 1.
 *
 * Returns     :  void
 *
 *********************************************************************/
static void cgi_create_client_tag_form(char *form, size_t size,
   const char *tag, int toggle_state, int expires)
{
   char *button_name;

   if (toggle_state == 1)
   {
      button_name = (expires == 1) ? "Enable" : "Enable temporarily";
   }
   else
   {
      assert(toggle_state == 0);
      button_name = "Disable";
   }

   snprintf(form, size,
      "<form method=\"GET\" action=\""CGI_PREFIX"toggle-client-tag\" style=\"display: inline\">\n"
      " <input type=\"hidden\" name=\"tag\" value=\"%s\">\n"
      " <input type=\"hidden\" name=\"toggle-state\" value=\"%i\">\n"
      " <input type=\"hidden\" name=\"expires\" value=\"%u\">\n"
      " <input type=\"submit\" value=\"%s\">\n"
      "</form>", tag, toggle_state, !expires, button_name);
}

/*********************************************************************
 *
 * Function    :  cgi_show_client_tags
 *
 * Description :  Shows the tags that can be set based on the client
 *                address (opt-in).
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : none
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_show_client_tags(struct client_state *csp,
                        struct http_response *rsp,
                        const struct map *parameters)
{
   struct map *exports;
   struct client_tag_spec *this_tag;
   jb_err err = JB_ERR_OK;
   char *client_tag_status;
   char buf[1000];
   time_t refresh_delay;

   assert(csp);
   assert(rsp);
   assert(parameters);

   if (NULL == (exports = default_exports(csp, "client-tags")))
   {
      return JB_ERR_MEMORY;
   }
   assert(csp->client_address != NULL);

   this_tag = csp->config->client_tags;
   if (this_tag->name == NULL)
   {
      client_tag_status = strdup_or_die("<p>No tags have been configured.</p>\n");
   }
   else
   {
      client_tag_status = strdup_or_die("<table border=\"1\">\n"
         "<tr><th>Tag name</th>\n"
         "<th>Current state</th><th>Change state</th><th>Description</th></tr>\n");
      while ((this_tag != NULL) && (this_tag->name != NULL))
      {
         int tag_state;

         privoxy_mutex_lock(&client_tags_mutex);
         tag_state = client_has_requested_tag(csp->client_address, this_tag->name);
         privoxy_mutex_unlock(&client_tags_mutex);
         if (!err) err = string_append(&client_tag_status, "<tr><td>");
         if (!err) err = string_append(&client_tag_status, this_tag->name);
         if (!err) err = string_append(&client_tag_status, "</td><td>");
         if (!err) err = string_append(&client_tag_status, tag_state == 1 ? "Enabled" : "Disabled");
         if (!err) err = string_append(&client_tag_status, "</td><td>");
         cgi_create_client_tag_form(buf, sizeof(buf), this_tag->name, !tag_state, 1);
         if (!err) err = string_append(&client_tag_status, buf);
         if (tag_state == 0)
         {
            cgi_create_client_tag_form(buf, sizeof(buf), this_tag->name, !tag_state, 0);
            if (!err) err = string_append(&client_tag_status, buf);
         }
         if (!err) err = string_append(&client_tag_status, "</td><td>");
         if (!err) err = string_append(&client_tag_status, this_tag->description);
         if (!err) err = string_append(&client_tag_status, "</td></tr>\n");
         if (err)
         {
            break;
         }
         this_tag = this_tag->next;
      }
      if (!err) err = string_append(&client_tag_status, "</table>\n");
      if (err)
      {
         free_map(exports);
         return JB_ERR_MEMORY;
      }
   }
   refresh_delay = get_next_tag_timeout_for_client(csp->client_address);
   if (refresh_delay != 0)
   {
      snprintf(buf, sizeof(buf), "%u", csp->config->client_tag_lifetime);
      if (map(exports, "refresh-delay", 1, buf, 1))
      {
         freez(client_tag_status);
         free_map(exports);
         return JB_ERR_MEMORY;
      }
   }
   else
   {
      err = map_block_killer(exports, "tags-expire");
      if (err != JB_ERR_OK)
      {
         freez(client_tag_status);
         return err;
      }
   }

   if (map(exports, "client-tags", 1, client_tag_status, 0))
   {
      free_map(exports);
      return JB_ERR_MEMORY;
   }

   if (map(exports, "client-ip-addr", 1, csp->client_address, 1))
   {
      free_map(exports);
      return JB_ERR_MEMORY;
   }

   return template_fill_for_cgi(csp, "client-tags", exports, rsp);
}


/*********************************************************************
 *
 * Function    :  cgi_toggle_client_tag
 *
 * Description :  Toggles a client tag and redirects to the show-tags
 *                page
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : none
 *          1  :  tag = Name of the tag to enable or disable
 *          2  :  toggle-state = How to toggle the tag (0/1)
 *          3  :  expires = Set to 1 if the tag should be enabled
 *                          temporarily, otherwise set to 0
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_toggle_client_tag(struct client_state *csp,
                             struct http_response *rsp,
                             const struct map *parameters)
{
   const char *toggled_tag;
   const char *toggle_state;
   const char *tag_expires;
   time_t time_to_live;

   assert(csp);
   assert(rsp);
   assert(parameters);

   toggled_tag = lookup(parameters, "tag");
   if (*toggled_tag == '\0')
   {
      log_error(LOG_LEVEL_ERROR, "Received tag toggle request without tag");
   }
   else
   {
      tag_expires = lookup(parameters, "expires");
      if (*tag_expires == '0')
      {
         time_to_live = 0;
      }
      else
      {
         time_to_live = csp->config->client_tag_lifetime;
      }
      toggle_state = lookup(parameters, "toggle-state");
      if (*toggle_state == '1')
      {
         enable_client_specific_tag(csp, toggled_tag, time_to_live);
      }
      else
      {
         disable_client_specific_tag(csp, toggled_tag);
      }
   }
   rsp->status = strdup_or_die("302 Done dealing with toggle request");
   if (enlist_unique_header(rsp->headers,
         "Location", CGI_PREFIX "client-tags"))
   {
         return JB_ERR_MEMORY;
   }
   return JB_ERR_OK;

}
#endif /* def FEATURE_CLIENT_TAGS */


/*********************************************************************
 *
 * Function    :  cgi_send_banner
 *
 * Description :  CGI function that returns a banner.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters :
 *           type : Selects the type of banner between "trans", "pattern",
 *                  and "auto". Defaults to "pattern" if absent or invalid.
 *                  "auto" means to select as if we were image-blocking.
 *                  (Only the first character really counts; b and t are
 *                  equivalent).
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_send_banner(struct client_state *csp,
                       struct http_response *rsp,
                       const struct map *parameters)
{
   char imagetype = lookup(parameters, "type")[0];

   if (imagetype != 'a' && imagetype != 'b' &&
       imagetype != 'p' && imagetype != 't')
   {
      log_error(LOG_LEVEL_ERROR, "Overruling invalid image type '%c'.",
         imagetype);
      imagetype = 'p';
   }

   /*
    * If type is auto, then determine the right thing
    * to do from the set-image-blocker action
    */
   if (imagetype == 'a')
   {
      /*
       * Default to pattern
       */
      imagetype = 'p';

#ifdef FEATURE_IMAGE_BLOCKING
      if ((csp->action->flags & ACTION_IMAGE_BLOCKER) != 0)
      {
         static const char prefix1[] = CGI_PREFIX "send-banner?type=";
         static const char prefix2[] = "http://" CGI_SITE_1_HOST "/send-banner?type=";
         const char *p = csp->action->string[ACTION_STRING_IMAGE_BLOCKER];

         if (p == NULL)
         {
            /* Use default - nothing to do here. */
         }
         else if (0 == strcmpic(p, "blank"))
         {
            imagetype = 'b';
         }
         else if (0 == strcmpic(p, "pattern"))
         {
            imagetype = 'p';
         }

         /*
          * If the action is to call this CGI, determine
          * the argument:
          */
         else if (0 == strncmpic(p, prefix1, sizeof(prefix1) - 1))
         {
            imagetype = p[sizeof(prefix1) - 1];
         }
         else if (0 == strncmpic(p, prefix2, sizeof(prefix2) - 1))
         {
            imagetype = p[sizeof(prefix2) - 1];
         }

         /*
          * Everything else must (should) be a URL to
          * redirect to.
          */
         else
         {
            imagetype = 'r';
         }
      }
#endif /* def FEATURE_IMAGE_BLOCKING */
   }

   /*
    * Now imagetype is either the non-auto type we were called with,
    * or it was auto and has since been determined. In any case, we
    * can proceed to actually answering the request by sending a redirect
    * or an image as appropriate:
    */
   if (imagetype == 'r')
   {
      rsp->status = strdup_or_die("302 Local Redirect from Privoxy");
      if (enlist_unique_header(rsp->headers, "Location",
                               csp->action->string[ACTION_STRING_IMAGE_BLOCKER]))
      {
         return JB_ERR_MEMORY;
      }
   }
   else
   {
      if ((imagetype == 'b') || (imagetype == 't'))
      {
         rsp->body = bindup(image_blank_data, image_blank_length);
         rsp->content_length = image_blank_length;
      }
      else
      {
         rsp->body = bindup(image_pattern_data, image_pattern_length);
         rsp->content_length = image_pattern_length;
      }

      if (rsp->body == NULL)
      {
         return JB_ERR_MEMORY;
      }
      if (enlist(rsp->headers, "Content-Type: " BUILTIN_IMAGE_MIMETYPE))
      {
         return JB_ERR_MEMORY;
      }

      rsp->is_static = 1;
   }

   return JB_ERR_OK;

}


/*********************************************************************
 *
 * Function    :  cgi_transparent_image
 *
 * Description :  CGI function that sends a 1x1 transparent image.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_transparent_image(struct client_state *csp,
                             struct http_response *rsp,
                             const struct map *parameters)
{
   (void)csp;
   (void)parameters;

   rsp->body = bindup(image_blank_data, image_blank_length);
   rsp->content_length = image_blank_length;

   if (rsp->body == NULL)
   {
      return JB_ERR_MEMORY;
   }

   if (enlist(rsp->headers, "Content-Type: " BUILTIN_IMAGE_MIMETYPE))
   {
      return JB_ERR_MEMORY;
   }

   rsp->is_static = 1;

   return JB_ERR_OK;

}


/*********************************************************************
 *
 * Function    :  cgi_send_default_favicon
 *
 * Description :  CGI function that sends the standard favicon.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_send_default_favicon(struct client_state *csp,
                                struct http_response *rsp,
                                const struct map *parameters)
{
   static const char default_favicon_data[] =
      "\000\000\001\000\001\000\020\020\002\000\000\000\000\000\260"
      "\000\000\000\026\000\000\000\050\000\000\000\020\000\000\000"
      "\040\000\000\000\001\000\001\000\000\000\000\000\100\000\000"
      "\000\000\000\000\000\000\000\000\000\002\000\000\000\000\000"
      "\000\000\377\377\377\000\377\000\052\000\017\360\000\000\077"
      "\374\000\000\161\376\000\000\161\376\000\000\361\377\000\000"
      "\361\377\000\000\360\017\000\000\360\007\000\000\361\307\000"
      "\000\361\307\000\000\361\307\000\000\360\007\000\000\160\036"
      "\000\000\177\376\000\000\077\374\000\000\017\360\000\000\360"
      "\017\000\000\300\003\000\000\200\001\000\000\200\001\000\000"
      "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
      "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
      "\000\000\200\001\000\000\200\001\000\000\300\003\000\000\360"
      "\017\000\000";
   static const size_t favicon_length = sizeof(default_favicon_data) - 1;

   (void)csp;
   (void)parameters;

   rsp->body = bindup(default_favicon_data, favicon_length);
   rsp->content_length = favicon_length;

   if (rsp->body == NULL)
   {
      return JB_ERR_MEMORY;
   }

   if (enlist(rsp->headers, "Content-Type: image/x-icon"))
   {
      return JB_ERR_MEMORY;
   }

   rsp->is_static = 1;

   return JB_ERR_OK;

}


/*********************************************************************
 *
 * Function    :  cgi_send_error_favicon
 *
 * Description :  CGI function that sends the favicon for error pages.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_send_error_favicon(struct client_state *csp,
                              struct http_response *rsp,
                              const struct map *parameters)
{
   static const char error_favicon_data[] =
      "\000\000\001\000\001\000\020\020\002\000\000\000\000\000\260"
      "\000\000\000\026\000\000\000\050\000\000\000\020\000\000\000"
      "\040\000\000\000\001\000\001\000\000\000\000\000\100\000\000"
      "\000\000\000\000\000\000\000\000\000\002\000\000\000\000\000"
      "\000\000\377\377\377\000\000\000\377\000\017\360\000\000\077"
      "\374\000\000\161\376\000\000\161\376\000\000\361\377\000\000"
      "\361\377\000\000\360\017\000\000\360\007\000\000\361\307\000"
      "\000\361\307\000\000\361\307\000\000\360\007\000\000\160\036"
      "\000\000\177\376\000\000\077\374\000\000\017\360\000\000\360"
      "\017\000\000\300\003\000\000\200\001\000\000\200\001\000\000"
      "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
      "\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000"
      "\000\000\200\001\000\000\200\001\000\000\300\003\000\000\360"
      "\017\000\000";
   static const size_t favicon_length = sizeof(error_favicon_data) - 1;

   (void)csp;
   (void)parameters;

   rsp->body = bindup(error_favicon_data, favicon_length);
   rsp->content_length = favicon_length;

   if (rsp->body == NULL)
   {
      return JB_ERR_MEMORY;
   }

   if (enlist(rsp->headers, "Content-Type: image/x-icon"))
   {
      return JB_ERR_MEMORY;
   }

   rsp->is_static = 1;

   return JB_ERR_OK;

}


/*********************************************************************
 *
 * Function    :  cgi_send_stylesheet
 *
 * Description :  CGI function that sends a css stylesheet found
 *                in the cgi-style.css template
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_send_stylesheet(struct client_state *csp,
                           struct http_response *rsp,
                           const struct map *parameters)
{
   jb_err err;

   assert(csp);
   assert(rsp);

   (void)parameters;

   err = template_load(csp, &rsp->body, "cgi-style.css", 0);

   if (err == JB_ERR_FILE)
   {
      /*
       * No way to tell user; send empty stylesheet
       */
      log_error(LOG_LEVEL_ERROR, "Could not find cgi-style.css template");
   }
   else if (err)
   {
      return err; /* JB_ERR_MEMORY */
   }

   if (enlist(rsp->headers, "Content-Type: text/css"))
   {
      return JB_ERR_MEMORY;
   }

   return JB_ERR_OK;

}


/*********************************************************************
 *
 * Function    :  cgi_send_wpad
 *
 * Description :  CGI function that sends a Proxy Auto-Configuration
 *                (PAC) file.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_send_wpad(struct client_state *csp,
                     struct http_response *rsp,
                     const struct map *parameters)
{
   struct map *exports;

   assert(csp);
   assert(rsp);
   assert(parameters);

   if (NULL == (exports = default_exports(csp, NULL)))
   {
      return JB_ERR_MEMORY;
   }

   if (enlist(rsp->headers, "Content-Type: application/x-ns-proxy-autoconfig"))
   {
      free_map(exports);
      return JB_ERR_MEMORY;
   }

   return template_fill_for_cgi(csp, "wpad.dat", exports, rsp);

}


/*********************************************************************
 *
 * Function    :  cgi_send_url_info_osd
 *
 * Description :  CGI function that sends the OpenSearch Description
 *                template for the show-url-info page. It allows to
 *                access the page through "search engine plugins".
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_send_url_info_osd(struct client_state *csp,
                               struct http_response *rsp,
                               const struct map *parameters)
{
   jb_err err = JB_ERR_MEMORY;
   struct map *exports = default_exports(csp, NULL);

   (void)csp;
   (void)parameters;

   if (NULL != exports)
   {
      err = template_fill_for_cgi(csp, "url-info-osd.xml", exports, rsp);
      if (JB_ERR_OK == err)
      {
         err = enlist(rsp->headers,
            "Content-Type: application/opensearchdescription+xml");
      }
   }

   return err;

}


/*********************************************************************
 *
 * Function    :  get_content_type
 *
 * Description :  Use the file extension to guess the content type
 *                header we should use to serve the file.
 *
 * Parameters  :
 *          1  :  filename = Name of the file whose content type
 *                           we care about
 *
 * Returns     :  The guessed content type.
 *
 *********************************************************************/
static const char *get_content_type(const char *filename)
{
   int i;
   struct content_type
   {
      const char extension[6];
      const char content_type[11];
   };
   static const struct content_type content_types[] =
   {
      {".css",  "text/css"},
      {".jpg",  "image/jpeg"},
      {".jpeg", "image/jpeg"},
      {".png",  "image/png"},
   };

   for (i = 0; i < SZ(content_types); i++)
   {
      if (strstr(filename, content_types[i].extension))
      {
         return content_types[i].content_type;
      }
   }

   /* No match by extension, default to html */
   return "text/html";
}

/*********************************************************************
 *
 * Function    :  cgi_send_user_manual
 *
 * Description :  CGI function that sends a file in the user
 *                manual directory.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : file=name.html, the name of the HTML file
 *                  (relative to user-manual from config)
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_send_user_manual(struct client_state *csp,
                            struct http_response *rsp,
                            const struct map *parameters)
{
   const char *filename;
   char *full_path;
   jb_err err = JB_ERR_OK;
   const char *content_type;

   assert(csp);
   assert(rsp);
   assert(parameters);

   if (0 == strncmpic(csp->config->usermanual, "http://", 7) ||
       0 == strncmpic(csp->config->usermanual, "https://", 8))
   {
      log_error(LOG_LEVEL_CGI, "Request for local user-manual "
         "received while user-manual delivery is disabled.");
      return cgi_error_404(csp, rsp, parameters);
   }

   if (!parameters->first)
   {
      /* requested http://p.p/user-manual (without trailing slash) */
      return cgi_redirect(rsp, CGI_PREFIX "user-manual/");
   }

   get_string_param(parameters, "file", &filename);
   if (filename == NULL)
   {
      /* It's '/' so serve the index.html if there is one.  */
      filename = "index.html";
   }
   else if (NULL != strchr(filename, '/') || NULL != strstr(filename, ".."))
   {
      /*
       * We currently only support a flat file
       * hierarchy for the documentation.
       */
      log_error(LOG_LEVEL_ERROR,
         "Rejecting the request to serve '%s' as it contains '/' or '..'",
         filename);
      return JB_ERR_CGI_PARAMS;
   }

   full_path = make_path(csp->config->usermanual, filename);
   if (full_path == NULL)
   {
      return JB_ERR_MEMORY;
   }

   err = load_file(full_path, &rsp->body, &rsp->content_length);
   if (JB_ERR_OK != err)
   {
      assert((JB_ERR_FILE == err) || (JB_ERR_MEMORY == err));
      if (JB_ERR_FILE == err)
      {
         err = cgi_error_no_template(csp, rsp, full_path);
      }
      freez(full_path);
      return err;
   }
   freez(full_path);

   content_type = get_content_type(filename);
   log_error(LOG_LEVEL_CGI,
      "Content-Type guessed for %s: %s", filename, content_type);

   return enlist_unique_header(rsp->headers, "Content-Type", content_type);

}


#ifdef FEATURE_EXTENDED_STATISTICS
/*********************************************************************
 *
 * Function    :  get_block_reason_statistics_table
 *
 * Description :  Produces the block reason statistic table content.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Pointer to the HTML statistic table content or
 *                NULL on out of memory
 *
 *********************************************************************/
static char *get_block_reason_statistics_table(const struct client_state *csp)
{
   char buf[BUFFER_SIZE];
   char *statistics;
   int i;
   struct file_list *fl;
   jb_err err = JB_ERR_OK;

   statistics = strdup_or_die("");

   /* Run through all action files. */
   for (i = 0; i < MAX_AF_FILES; i++)
   {
      struct url_actions *b;
      struct action_spec *last_action = NULL;

      if (((fl = csp->actions_list[i]) == NULL) || ((b = fl->f) == NULL))
      {
         /* Skip empty files */
         continue;
      }

      /* Go through all the actions. */
      for (b = b->next; NULL != b; b = b->next)
      {
         if (last_action == b->action)
         {
            continue;
         }
         if ((b->action->add & ACTION_BLOCK))
         {
            unsigned long long count;
            const char *block_reason = b->action->string[ACTION_STRING_BLOCK];
            const char *encoded_block_reason = html_encode(block_reason);

            if (encoded_block_reason == NULL)
            {
               freez(statistics);
               return NULL;
            }
            get_block_reason_count(block_reason, &count);
            snprintf(buf, sizeof(buf),
               "<tr><td>%s</td><td style=\"text-align: right\">%llu</td>\n",
               encoded_block_reason, count);
            freez(encoded_block_reason);

            if (!err) err = string_append(&statistics, buf);
         }
         last_action = b->action;
      }
   }

   return statistics;

}


/*********************************************************************
 *
 * Function    :  get_filter_statistics_table
 *
 * Description :  Produces the filter statistic table content.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  Pointer to the HTML statistic table content or
 *                NULL on out of memory
 *
 *********************************************************************/
static char *get_filter_statistics_table(const struct client_state *csp)
{
   char buf[BUFFER_SIZE];
   char *statistics;
   int i;
   struct file_list *fl;
   struct re_filterfile_spec *b;
   jb_err err = JB_ERR_OK;

   statistics = strdup_or_die("");

   for (i = 0; i < MAX_AF_FILES; i++)
   {
     fl = csp->rlist[i];
     if ((NULL == fl) || (NULL == fl->f))
     {
        /*
         * Either there are no filter files left or this
         * filter file just contains no valid filters.
         *
         * Continue to be sure we don't miss valid filter
         * files that are chained after empty or invalid ones.
         */
        continue;
     }

     for (b = fl->f; b != NULL; b = b->next)
     {
        if (b->type == FT_CONTENT_FILTER)
        {
           unsigned long long executions;
           unsigned long long response_bodies_modified;
           unsigned long long hits;

           get_filter_statistics(b->name, &executions, &response_bodies_modified, &hits);
           snprintf(buf, sizeof(buf),
              "<tr><td>%s</td><td style=\"text-align: right\">%llu</td>"
              "<td style=\"text-align: right\">%llu</td>"
              "<td style=\"text-align: right\">%llu</td><tr>\n",
              b->name, executions, response_bodies_modified, hits);

           if (!err) err = string_append(&statistics, buf);
        }
     }
   }

   return statistics;

}
#endif /* def FEATURE_EXTENDED_STATISTICS */


/*********************************************************************
 *
 * Function    :  cgi_show_status
 *
 * Description :  CGI function that returns a web page describing the
 *                current status of Privoxy.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters :
 *        file :  Which file to show.  Only first letter is checked,
 *                valid values are:
 *                - "a"ction file
 *                - "r"egex
 *                - "t"rust
 *                Default is to show menu and other information.
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_show_status(struct client_state *csp,
                       struct http_response *rsp,
                       const struct map *parameters)
{
   char *s = NULL;
   unsigned i;
   int j;

   char buf[BUFFER_SIZE];
#ifdef FEATURE_STATISTICS
#ifdef MUTEX_LOCKS_AVAILABLE
   float percentage_blocked;
   unsigned long long local_number_of_requests_received;
   unsigned long long local_number_of_requests_blocked;
#else
   float perc_rej;   /* Percentage of http requests rejected */
   int local_urls_read;
   int local_urls_rejected;
#endif
#endif /* ndef FEATURE_STATISTICS */
   jb_err err = JB_ERR_OK;

   struct map *exports;

   assert(csp);
   assert(rsp);
   assert(parameters);

   if ('\0' != *(lookup(parameters, "file")))
   {
      return cgi_show_file(csp, rsp, parameters);
   }

   if (NULL == (exports = default_exports(csp, "show-status")))
   {
      return JB_ERR_MEMORY;
   }

   s = strdup("");
   for (j = 0; (s != NULL) && (j < Argc); j++)
   {
      if (!err) err = string_join  (&s, html_encode(Argv[j]));
      if (!err) err = string_append(&s, " ");
   }
   if (!err) err = map(exports, "invocation", 1, s, 0);

   if (!err) err = map(exports, "options", 1, csp->config->proxy_args, 1);
   if (!err) err = show_defines(exports);

   if (err)
   {
      free_map(exports);
      return JB_ERR_MEMORY;
   }

#ifdef FEATURE_STATISTICS
#ifdef MUTEX_LOCKS_AVAILABLE
   privoxy_mutex_lock(&block_statistics_mutex);
   local_number_of_requests_received = number_of_requests_received;
   local_number_of_requests_blocked = number_of_requests_blocked;
   privoxy_mutex_unlock(&block_statistics_mutex);
#else
   local_urls_read     = urls_read;
   local_urls_rejected = urls_rejected;
#endif
   /*
    * Need to alter the stats not to include the fetch of this
    * page.
    *
    * Can't do following thread safely! doh!
    *
    * urls_read--;
    * urls_rejected--; * This will be incremented subsequently *
    */

#ifdef MUTEX_LOCKS_AVAILABLE
   if (local_number_of_requests_received == 0)
#else
   if (local_urls_read == 0)
#endif
   {
      if (!err) err = map_block_killer(exports, "have-stats");
   }
   else
   {
      if (!err) err = map_block_killer(exports, "have-no-stats");

#ifdef MUTEX_LOCKS_AVAILABLE
      percentage_blocked = (float)local_number_of_requests_blocked * 100.0F /
            (float)local_number_of_requests_received;

      snprintf(buf, sizeof(buf), "%llu", local_number_of_requests_received);
      if (!err) err = map(exports, "requests-received", 1, buf, 1);

      snprintf(buf, sizeof(buf), "%llu", local_number_of_requests_blocked);
      if (!err) err = map(exports, "requests-blocked", 1, buf, 1);

      snprintf(buf, sizeof(buf), "%6.2f", percentage_blocked);
      if (!err) err = map(exports, "percent-blocked", 1, buf, 1);
#else
      perc_rej = (float)local_urls_rejected * 100.0F /
            (float)local_urls_read;

      snprintf(buf, sizeof(buf), "%d", local_urls_read);
      if (!err) err = map(exports, "requests-received", 1, buf, 1);

      snprintf(buf, sizeof(buf), "%d", local_urls_rejected);
      if (!err) err = map(exports, "requests-blocked", 1, buf, 1);

      snprintf(buf, sizeof(buf), "%6.2f", perc_rej);
      if (!err) err = map(exports, "percent-blocked", 1, buf, 1);
#endif
   }

#else /* ndef FEATURE_STATISTICS */
   if (!err) err = map_block_killer(exports, "statistics");
#endif /* ndef FEATURE_STATISTICS */

#ifdef FEATURE_EXTENDED_STATISTICS
   if (!err)
   {
      char *block_reason_statistics = get_block_reason_statistics_table(csp);
      if (block_reason_statistics != NULL)
      {
         err = map(exports, "block-reason-statistics", 1, block_reason_statistics, 0);
      }
      else
      {
         err = map_block_killer(exports, "extended-statistics");
      }
   }
   if (!err)
   {
      char *filter_statistics = get_filter_statistics_table(csp);
      if (filter_statistics != NULL)
      {
         err = map(exports, "filter-statistics", 1, filter_statistics, 0);
      }
      else
      {
         err = map_block_killer(exports, "extended-statistics");
      }
   }
#else /* ndef FEATURE_EXTENDED_STATISTICS */
   if (!err) err = map_block_killer(exports, "extended-statistics");
#endif /* def FEATURE_EXTENDED_STATISTICS */

   /*
    * List all action files in use, together with view and edit links,
    * except for standard.action, which should only be viewable. (Not
    * enforced in the editor itself)
    * FIXME: Shouldn't include hardwired HTML here, use line template instead!
    */
   s = strdup("");
   for (i = 0; i < MAX_AF_FILES; i++)
   {
      if (csp->actions_list[i] != NULL)
      {
         if (!err) err = string_append(&s, "<tr><td>");
         if (!err) err = string_join(&s, html_encode(csp->actions_list[i]->filename));
         snprintf(buf, sizeof(buf),
            "</td><td class=\"buttons\"><a href=\"/show-status?file=actions&amp;index=%u\">View</a>", i);
         if (!err) err = string_append(&s, buf);

#ifdef FEATURE_CGI_EDIT_ACTIONS
         if ((csp->config->feature_flags & RUNTIME_FEATURE_CGI_EDIT_ACTIONS)
            && (NULL != csp->config->actions_file_short[i]))
         {
#ifdef HAVE_ACCESS
            if (access(csp->config->actions_file[i], W_OK) == 0)
            {
#endif /* def HAVE_ACCESS */
               snprintf(buf, sizeof(buf), "&nbsp;&nbsp;<a href=\"/edit-actions-list?f=%u\">Edit</a>", i);
               if (!err) err = string_append(&s, buf);
#ifdef HAVE_ACCESS
            }
            else
            {
               if (!err) err = string_append(&s, "&nbsp;&nbsp;<strong>No write access.</strong>");
            }
#endif /* def HAVE_ACCESS */
         }
#endif

         if (!err) err = string_append(&s, "</td></tr>\n");
      }
   }
   if (!err && *s != '\0')
   {
      err = map(exports, "actions-filenames", 1, s, 0);
   }
   else
   {
      if (!err) err = map(exports, "actions-filenames", 1, "<tr><td>None specified</td></tr>", 1);
      freez(s);
   }

   /*
    * List all re_filterfiles in use, together with view options.
    * FIXME: Shouldn't include hardwired HTML here, use line template instead!
    */
   s = strdup("");
   for (i = 0; i < MAX_AF_FILES; i++)
   {
      if (csp->rlist[i] != NULL)
      {
         if (!err) err = string_append(&s, "<tr><td>");
         if (!err) err = string_join(&s, html_encode(csp->rlist[i]->filename));
         snprintf(buf, sizeof(buf),
            "</td><td class=\"buttons\"><a href=\"/show-status?file=filter&amp;index=%u\">View</a>", i);
         if (!err) err = string_append(&s, buf);
         if (!err) err = string_append(&s, "</td></tr>\n");
      }
   }
   if (!err && *s != '\0')
   {
      err = map(exports, "re-filter-filenames", 1, s, 0);
   }
   else
   {
      if (!err) err = map(exports, "re-filter-filenames", 1, "<tr><td>None specified</td></tr>", 1);
      if (!err) err = map_block_killer(exports, "have-filterfile");
      freez(s);
   }

#ifdef FEATURE_TRUST
   if (csp->tlist)
   {
      if (!err) err = map(exports, "trust-filename", 1, html_encode(csp->tlist->filename), 0);
   }
   else
   {
      if (!err) err = map(exports, "trust-filename", 1, "None specified", 1);
      if (!err) err = map_block_killer(exports, "have-trustfile");
   }
#else
   if (!err) err = map_block_killer(exports, "trust-support");
#endif /* ndef FEATURE_TRUST */

#ifdef FEATURE_CGI_EDIT_ACTIONS
   if (!err && (csp->config->feature_flags & RUNTIME_FEATURE_CGI_EDIT_ACTIONS))
   {
      err = map_block_killer(exports, "cgi-editor-is-disabled");
   }
#endif /* ndef CGI_EDIT_ACTIONS */

   if (!err) err = map(exports, "force-prefix", 1, FORCE_PREFIX, 1);

   if (err)
   {
      free_map(exports);
      return JB_ERR_MEMORY;
   }

   return template_fill_for_cgi(csp, "show-status", exports, rsp);
}


/*********************************************************************
 *
 * Function    :  cgi_show_url_info
 *
 * Description :  CGI function that determines and shows which actions
 *                Privoxy will perform for a given url, and which
 *                matches starting from the defaults have lead to that.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters :
 *            url : The url whose actions are to be determined.
 *                  If url is unset, the url-given conditional will be
 *                  set, so that all but the form can be suppressed in
 *                  the template.
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_show_url_info(struct client_state *csp,
                         struct http_response *rsp,
                         const struct map *parameters)
{
   char *url_param;
   struct map *exports;
   char buf[150];

   assert(csp);
   assert(rsp);
   assert(parameters);

   if (NULL == (exports = default_exports(csp, "show-url-info")))
   {
      return JB_ERR_MEMORY;
   }

   /*
    * Get the url= parameter (if present) and remove any leading/trailing spaces.
    */
   url_param = strdup_or_die(lookup(parameters, "url"));
   chomp(url_param);

   /*
    * Handle prefixes.  4 possibilities:
    * 1) "http://" or "https://" prefix present and followed by URL - OK
    * 2) Only the "http://" or "https://" part is present, no URL - change
    *    to empty string so it will be detected later as "no URL".
    * 3) Parameter specified but doesn't start with "http(s?)://" - add a
    *    "http://" prefix.
    * 4) Parameter not specified or is empty string - let this fall through
    *    for now, next block of code will handle it.
    */
   if (0 == strncmp(url_param, "http://", 7))
   {
      if (url_param[7] == '\0')
      {
         /*
          * Empty URL (just prefix).
          * Make it totally empty so it's caught by the next if ()
          */
         url_param[0] = '\0';
      }
   }
   else if (0 == strncmp(url_param, "https://", 8))
   {
      if (url_param[8] == '\0')
      {
         /*
          * Empty URL (just prefix).
          * Make it totally empty so it's caught by the next if ()
          */
         url_param[0] = '\0';
      }
   }
   else if ((url_param[0] != '\0')
      && ((NULL == strstr(url_param, "://")
            || (strstr(url_param, "://") > strstr(url_param, "/")))))
   {
      /*
       * No prefix or at least no prefix before
       * the first slash - assume http://
       */
      char *url_param_prefixed = strdup_or_die("http://");

      if (JB_ERR_OK != string_join(&url_param_prefixed, url_param))
      {
         free_map(exports);
         return JB_ERR_MEMORY;
      }
      url_param = url_param_prefixed;
   }

   /*
    * Hide "toggle off" warning if Privoxy is toggled on.
    */
   if (
#ifdef FEATURE_TOGGLE
       (global_toggle_state == 1) &&
#endif /* def FEATURE_TOGGLE */
       map_block_killer(exports, "privoxy-is-toggled-off")
      )
   {
      freez(url_param);
      free_map(exports);
      return JB_ERR_MEMORY;
   }

   if (url_param[0] == '\0')
   {
      /* URL parameter not specified, display query form only. */
      free(url_param);
      if (map_block_killer(exports, "url-given")
        || map(exports, "url", 1, "", 1))
      {
         free_map(exports);
         return JB_ERR_MEMORY;
      }
   }
   else
   {
      /* Given a URL, so query it. */
      jb_err err;
      char *matches;
      char *s;
      int hits = 0;
      struct file_list *fl;
      struct url_actions *b;
      struct http_request url_to_query[1];
      struct current_action_spec action[1];
      int i;

      if (map(exports, "url", 1, html_encode(url_param), 0))
      {
         free(url_param);
         free_map(exports);
         return JB_ERR_MEMORY;
      }

      init_current_action(action);

      if (map(exports, "default", 1, current_action_to_html(csp, action), 0))
      {
         free_current_action(action);
         free(url_param);
         free_map(exports);
         return JB_ERR_MEMORY;
      }

      memset(url_to_query, '\0', sizeof(url_to_query));
      err = parse_http_url(url_param, url_to_query, REQUIRE_PROTOCOL);
      assert((err != JB_ERR_OK) || (url_to_query->ssl == !strncmpic(url_param, "https://", 8)));

      free(url_param);

      if (err == JB_ERR_MEMORY)
      {
         free_http_request(url_to_query);
         free_current_action(action);
         free_map(exports);
         return JB_ERR_MEMORY;
      }
      else if (err)
      {
         /* Invalid URL */

         err = map(exports, "matches", 1, "<b>[Invalid URL specified!]</b>" , 1);
         if (!err) err = map(exports, "final", 1, lookup(exports, "default"), 1);
         if (!err) err = map_block_killer(exports, "valid-url");

         free_current_action(action);
         free_http_request(url_to_query);

         if (err)
         {
            free_map(exports);
            return JB_ERR_MEMORY;
         }

         return template_fill_for_cgi(csp, "show-url-info", exports, rsp);
      }

      /*
       * We have a warning about SSL paths. Hide it for unencrypted sites
       * and unconditionally if https inspection is enabled.
       */
#ifndef FEATURE_HTTPS_INSPECTION
      if (!url_to_query->ssl)
#endif
      {
         if (map_block_killer(exports, "https-and-no-https-inspection"))
         {
            free_current_action(action);
            free_map(exports);
            free_http_request(url_to_query);
            return JB_ERR_MEMORY;
         }
      }

      matches = strdup_or_die("<table summary=\"\" class=\"transparent\">");

      for (i = 0; i < MAX_AF_FILES; i++)
      {
         if (NULL == csp->config->actions_file_short[i]
             || !strcmp(csp->config->actions_file_short[i], "standard.action")) continue;

         b = NULL;
         hits = 1;
         if ((fl = csp->actions_list[i]) != NULL)
         {
            if ((b = fl->f) != NULL)
            {
               /* FIXME: Hardcoded HTML! */
               string_append(&matches, "<tr><th>In file: ");
               string_join  (&matches, html_encode(csp->config->actions_file_short[i]));
               snprintf(buf, sizeof(buf), " <a class=\"cmd\" href=\"/show-status?file=actions&amp;index=%d\">", i);
               string_append(&matches, buf);
               string_append(&matches, "View</a>");
#ifdef FEATURE_CGI_EDIT_ACTIONS
               if (csp->config->feature_flags & RUNTIME_FEATURE_CGI_EDIT_ACTIONS)
               {
#ifdef HAVE_ACCESS
                  if (access(csp->config->actions_file[i], W_OK) == 0)
                  {
#endif /* def HAVE_ACCESS */
                     snprintf(buf, sizeof(buf),
                        " <a class=\"cmd\" href=\"/edit-actions-list?f=%d\">", i);
                     string_append(&matches, buf);
                     string_append(&matches, "Edit</a>");
#ifdef HAVE_ACCESS
                  }
                  else
                  {
                     string_append(&matches, " <strong>No write access.</strong>");
                  }
#endif /* def HAVE_ACCESS */
               }
#endif /* FEATURE_CGI_EDIT_ACTIONS */

               string_append(&matches, "</th></tr>\n");

               hits = 0;
               b = b->next;
            }
         }

         for (; (b != NULL) && (matches != NULL); b = b->next)
         {
            if (url_match(b->url, url_to_query))
            {
               string_append(&matches, "<tr><td>{");
               string_join  (&matches, actions_to_html(csp, b->action));
               string_append(&matches, " }<br>\n<code>");
               string_join  (&matches, html_encode(b->url->spec));
               string_append(&matches, "</code></td></tr>\n");

               if (merge_current_action(action, b->action))
               {
                  freez(matches);
                  free_http_request(url_to_query);
                  free_current_action(action);
                  free_map(exports);
                  return JB_ERR_MEMORY;
               }
               hits++;
            }
         }

         if (!hits)
         {
            string_append(&matches, "<tr><td>(no matches in this file)</td></tr>\n");
         }
      }
      string_append(&matches, "</table>\n");

      /*
       * XXX: Kludge to make sure the "Forward settings" section
       * shows what forward-override{} would do with the requested URL.
       * No one really cares how the CGI request would be forwarded
       * if it wasn't intercepted as CGI request in the first place.
       *
       * From here on the action bitmask will no longer reflect
       * the real url (http://config.privoxy.org/show-url-info?url=.*),
       * but luckily it's no longer required later on anyway.
       */
      free_current_action(csp->action);
      get_url_actions(csp, url_to_query);

      /*
       * Fill in forwarding settings.
       *
       * The possibilities are:
       *  - no forwarding
       *  - http forwarding only
       *  - socks4(a) forwarding only
       *  - socks4(a) and http forwarding.
       *
       * XXX: Parts of this code could be reused for the
       * "forwarding-failed" template which currently doesn't
       * display the proxy port and an eventual second forwarder.
       */
      {
         const struct forward_spec *fwd = forward_url(csp, url_to_query);

         if ((fwd->gateway_host == NULL) && (fwd->forward_host == NULL))
         {
            if (!err) err = map_block_killer(exports, "socks-forwarder");
            if (!err) err = map_block_killer(exports, "http-forwarder");
         }
         else
         {
            char port[10]; /* We save proxy ports as int but need a string here */

            if (!err) err = map_block_killer(exports, "no-forwarder");

            if (fwd->gateway_host != NULL)
            {
               char *socks_type = NULL;

               switch (fwd->type)
               {
                  case SOCKS_4:
                     socks_type = "socks4";
                     break;
                  case SOCKS_4A:
                     socks_type = "socks4a";
                     break;
                  case SOCKS_5:
                     socks_type = "socks5";
                     break;
                  case SOCKS_5T:
                     socks_type = "socks5t";
                     break;
                  default:
                     log_error(LOG_LEVEL_FATAL, "Unknown socks type: %d.", fwd->type);
               }

               if (!err) err = map(exports, "socks-type", 1, socks_type, 1);
               if (!err) err = map(exports, "gateway-host", 1, fwd->gateway_host, 1);
               snprintf(port, sizeof(port), "%d", fwd->gateway_port);
               if (!err) err = map(exports, "gateway-port", 1, port, 1);
            }
            else
            {
               if (!err) err = map_block_killer(exports, "socks-forwarder");
            }

            if (fwd->forward_host != NULL)
            {
               if (!err) err = map(exports, "forward-host", 1, fwd->forward_host, 1);
               snprintf(port, sizeof(port), "%d", fwd->forward_port);
               if (!err) err = map(exports, "forward-port", 1, port, 1);
            }
            else
            {
               if (!err) err = map_block_killer(exports, "http-forwarder");
            }
         }
      }

      free_http_request(url_to_query);

      if (err || matches == NULL)
      {
         free_current_action(action);
         free_map(exports);
         return JB_ERR_MEMORY;
      }

#ifdef FEATURE_CGI_EDIT_ACTIONS
      if ((csp->config->feature_flags & RUNTIME_FEATURE_CGI_EDIT_ACTIONS))
      {
         err = map_block_killer(exports, "cgi-editor-is-disabled");
      }
#endif /* FEATURE_CGI_EDIT_ACTIONS */

      /*
       * If zlib support is available, if no content filters
       * are enabled or if the prevent-compression action is enabled,
       * suppress the "compression could prevent filtering" warning.
       */
#ifndef FEATURE_ZLIB
      if (!content_filters_enabled(action) ||
         (action->flags & ACTION_NO_COMPRESSION))
#endif
      {
         if (!err) err = map_block_killer(exports, "filters-might-be-ineffective");
      }

      if (err || map(exports, "matches", 1, matches , 0))
      {
         free_current_action(action);
         free_map(exports);
         return JB_ERR_MEMORY;
      }

      s = current_action_to_html(csp, action);

      free_current_action(action);

      if (map(exports, "final", 1, s, 0))
      {
         free_map(exports);
         return JB_ERR_MEMORY;
      }
   }

   return template_fill_for_cgi(csp, "show-url-info", exports, rsp);
}


/*********************************************************************
 *
 * Function    :  cgi_robots_txt
 *
 * Description :  CGI function to return "/robots.txt".
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
jb_err cgi_robots_txt(struct client_state *csp,
                      struct http_response *rsp,
                      const struct map *parameters)
{
   char buf[100];
   jb_err err;

   (void)csp;
   (void)parameters;

   rsp->body = strdup_or_die(
      "# This is the Privoxy control interface.\n"
      "# It isn't very useful to index it, and you're likely to break stuff.\n"
      "# So go away!\n"
      "\n"
      "User-agent: *\n"
      "Disallow: /\n"
      "\n");

   err = enlist_unique(rsp->headers, "Content-Type: text/plain", 13);

   rsp->is_static = 1;

   get_http_time(7 * 24 * 60 * 60, buf, sizeof(buf)); /* 7 days into future */
   if (!err) err = enlist_unique_header(rsp->headers, "Expires", buf);

   return (err ? JB_ERR_MEMORY : JB_ERR_OK);
}


/*********************************************************************
 *
 * Function    :  show_defines
 *
 * Description :  Add to a map the state of all conditional #defines
 *                used when building
 *
 * Parameters  :
 *          1  :  exports = map to extend
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
static jb_err show_defines(struct map *exports)
{
   jb_err err = JB_ERR_OK;
   int i;
   struct feature {
      const char name[31];
      const unsigned char is_available;
   };

   static const struct feature features[] = {
      {
         "FEATURE_64_BIT_TIME_T",
#if (SIZEOF_TIME_T == 8)
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_ACCEPT_FILTER",
#ifdef FEATURE_ACCEPT_FILTER
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_ACL",
#ifdef FEATURE_ACL
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_BROTLI",
#ifdef FEATURE_BROTLI
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_CGI_EDIT_ACTIONS",
#ifdef FEATURE_CGI_EDIT_ACTIONS
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_CLIENT_TAGS",
#ifdef FEATURE_CLIENT_TAGS
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_COMPRESSION",
#ifdef FEATURE_COMPRESSION
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_CONNECTION_KEEP_ALIVE",
#ifdef FEATURE_CONNECTION_KEEP_ALIVE
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_CONNECTION_SHARING",
#ifdef FEATURE_CONNECTION_SHARING
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_EXTERNAL_FILTERS",
#ifdef FEATURE_EXTERNAL_FILTERS
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_FAST_REDIRECTS",
#ifdef FEATURE_FAST_REDIRECTS
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_FORCE_LOAD",
#ifdef FEATURE_FORCE_LOAD
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_GRACEFUL_TERMINATION",
#ifdef FEATURE_GRACEFUL_TERMINATION
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_HTTPS_INSPECTION",
#ifdef FEATURE_HTTPS_INSPECTION
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_IMAGE_BLOCKING",
#ifdef FEATURE_IMAGE_BLOCKING
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_IPV6_SUPPORT",
#ifdef HAVE_RFC2553
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_NO_GIFS",
#ifdef FEATURE_NO_GIFS
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_PTHREAD",
#ifdef FEATURE_PTHREAD
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_STATISTICS",
#ifdef FEATURE_STATISTICS
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_STRPTIME_SANITY_CHECKS",
#ifdef FEATURE_STRPTIME_SANITY_CHECKS
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_TOGGLE",
#ifdef FEATURE_TOGGLE
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_TRUST",
#ifdef FEATURE_TRUST
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_ZLIB",
#ifdef FEATURE_ZLIB
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_DYNAMIC_PCRE",
#ifdef FEATURE_DYNAMIC_PCRE
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_EXTENDED_STATISTICS",
#ifdef FEATURE_EXTENDED_STATISTICS
         1,
#else
         0,
#endif
      },
      {
         "FEATURE_PCRE_HOST_PATTERNS",
#ifdef FEATURE_PCRE_HOST_PATTERNS
         1,
#else
         0,
#endif
      }
   };

   for (i = 0; i < SZ(features); i++)
   {
      err = map_conditional(exports, features[i].name, features[i].is_available);
      if (err)
      {
         break;
      }
   }

   return err;

}


/*********************************************************************
 *
 * Function    :  cgi_show_file
 *
 * Description :  CGI function that shows the content of a
 *                configuration file.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *          2  :  rsp = http_response data structure for output
 *          3  :  parameters = map of cgi parameters
 *
 * CGI Parameters :
 *        file :  Which file to show.  Only first letter is checked,
 *                valid values are:
 *                - "a"ction file
 *                - "r"egex
 *                - "t"rust
 *                Default is to show menu and other information.
 *
 * Returns     :  JB_ERR_OK on success
 *                JB_ERR_MEMORY on out-of-memory error.
 *
 *********************************************************************/
static jb_err cgi_show_file(struct client_state *csp,
                            struct http_response *rsp,
                            const struct map *parameters)
{
   unsigned i;
   const char * filename = NULL;
   char * file_description = NULL;

   assert(csp);
   assert(rsp);
   assert(parameters);

   switch (*(lookup(parameters, "file")))
   {
   case 'a':
      if (!get_number_param(csp, parameters, "index", &i) && i < MAX_AF_FILES && csp->actions_list[i])
      {
         filename = csp->actions_list[i]->filename;
         file_description = "Actions File";
      }
      break;

   case 'f':
      if (!get_number_param(csp, parameters, "index", &i) && i < MAX_AF_FILES && csp->rlist[i])
      {
         filename = csp->rlist[i]->filename;
         file_description = "Filter File";
      }
      break;

#ifdef FEATURE_TRUST
   case 't':
      if (csp->tlist)
      {
         filename = csp->tlist->filename;
         file_description = "Trust File";
      }
      break;
#endif /* def FEATURE_TRUST */
   }

   if (NULL != filename)
   {
      struct map *exports;
      char *s;
      jb_err err;
      size_t length;

      exports = default_exports(csp, "show-status");
      if (NULL == exports)
      {
         return JB_ERR_MEMORY;
      }

      if (map(exports, "file-description", 1, file_description, 1)
        || map(exports, "filepath", 1, html_encode(filename), 0))
      {
         free_map(exports);
         return JB_ERR_MEMORY;
      }

      err = load_file(filename, &s, &length);
      if (JB_ERR_OK != err)
      {
         if (map(exports, "contents", 1, "<h1>ERROR OPENING FILE!</h1>", 1))
         {
            free_map(exports);
            return JB_ERR_MEMORY;
         }
      }
      else
      {
         s = html_encode_and_free_original(s);
         if (NULL == s)
         {
            free_map(exports);
            return JB_ERR_MEMORY;
         }

         if (map(exports, "contents", 1, s, 0))
         {
            free_map(exports);
            return JB_ERR_MEMORY;
         }
      }

      return template_fill_for_cgi(csp, "show-status-file", exports, rsp);
   }

   return JB_ERR_CGI_PARAMS;
}


/*********************************************************************
 *
 * Function    :  load_file
 *
 * Description :  Loads a file into a buffer.
 *
 * Parameters  :
 *          1  :  filename = Name of the file to be loaded.
 *          2  :  buffer   = Used to return the file's content.
 *          3  :  length   = Used to return the size of the file.
 *
 * Returns     :  JB_ERR_OK in case of success,
 *                JB_ERR_FILE in case of ordinary file loading errors
 *                            (fseek() and ftell() errors are fatal)
 *                JB_ERR_MEMORY in case of out-of-memory.
 *
 *********************************************************************/
static jb_err load_file(const char *filename, char **buffer, size_t *length)
{
   FILE *fp;
   long ret;
   jb_err err = JB_ERR_OK;

   fp = fopen(filename, "rb");
   if (NULL == fp)
   {
      log_error(LOG_LEVEL_ERROR, "Failed to open %s: %E", filename);
      return JB_ERR_FILE;
   }

   /* Get file length */
   if (fseek(fp, 0, SEEK_END))
   {
      log_error(LOG_LEVEL_FATAL,
         "Unexpected error while fseek()ing to the end of %s: %E",
         filename);
   }
   ret = ftell(fp);
   if (-1 == ret)
   {
      log_error(LOG_LEVEL_FATAL,
         "Unexpected ftell() error while loading %s: %E",
         filename);
   }
   *length = (size_t)ret;

   /* Go back to the beginning. */
   if (fseek(fp, 0, SEEK_SET))
   {
      log_error(LOG_LEVEL_FATAL,
         "Unexpected error while fseek()ing to the beginning of %s: %E",
         filename);
   }

   *buffer = zalloc_or_die(*length + 1);

   if (1 != fread(*buffer, *length, 1, fp))
   {
      /*
       * May theoretically happen if the file size changes between
       * fseek() and fread() because it's edited in-place. Privoxy
       * and common text editors don't do that, thus we just fail.
       */
      log_error(LOG_LEVEL_ERROR,
         "Couldn't completely read file %s.", filename);
      freez(*buffer);
      err = JB_ERR_FILE;
   }

   fclose(fp);

   return err;

}


/*
  Local Variables:
  tab-width: 3
  end:
*/
