const char cgiedit_rcs[] = "$Id: cgi.c,v 1.25 2001/09/16 15:02:35 jongfoster Exp $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/cgiedit.c,v $
 *
 * Purpose     :  CGI-based actionsfile editor.
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
 *
 **********************************************************************/


#include "config.h"

/*
 * FIXME: Following includes copied from cgi.c - which are actually needed?
 */

#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
#define snprintf _snprintf
#endif /* def _WIN32 */

#include "project.h"
#include "cgi.h"
#include "cgiedit.h"
#include "list.h"
#include "encode.h"
#include "ssplit.h"
#include "jcc.h"
#include "filters.h"
#include "actions.h"
#include "errlog.h"
#include "miscutil.h"
#include "showargs.h"
#include "loadcfg.h"

const char cgiedit_h_rcs[] = CGIEDIT_H_VERSION;


#ifdef FEATURE_CGI_EDIT_ACTIONS


/*********************************************************************
 *
 * Function    :  cgi_edit_actions_list
 *
 * Description :  CGI function that edits the actions list.
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_edit_actions_list(struct client_state *csp, struct http_response *rsp,
   		                 struct map *parameters)
{
   struct file_list *fl;
   struct url_actions *actions;
   char * actions_html;
   char * next_actions_html;
   char * section_template;
   char * url_template;
   char * sections;
   char * urls;
   struct map * exports = default_exports(csp, NULL);
   struct map * section_exports;
   struct map * url_exports;
   int urlid;
   char buf[50];
   char * s;
   int url_1_2;

   if (((fl = csp->actions_list) == NULL) || ((actions = fl->f) == NULL))
   {
      /* FIXME: Oops, no file to edit */
      free_map(exports);
      return cgi_default(csp, rsp, parameters);
   }

   /* Should do all global exports above this point */

   section_template = template_load(csp, "edit-actions-list-section");
   url_template = template_load(csp, "edit-actions-list-url");

   template_fill(&section_template, exports);
   template_fill(&url_template, exports);

   urlid = 0;
   sections = strdup("");

   ++urlid;
   actions = actions->next;
   if (actions != NULL)
   {
      actions_html = actions_to_html(actions->action);
   }

   while (actions != NULL)
   {
      section_exports = new_map();

      snprintf(buf, 50, "%d", urlid);
      map(section_exports, "sectionid", 1, buf, 1);

      map(section_exports, "actions", 1, actions_html, 1);

      /* Should do all section-specific exports above this point */

      urls = strdup("");
      url_1_2 = 2;

      next_actions_html = NULL;
      do
      {
         freez(next_actions_html);

         url_exports = new_map();

         snprintf(buf, 50, "%d", urlid);
         map(url_exports, "urlid", 1, buf, 1);

         snprintf(buf, 50, "%d", url_1_2);
         map(url_exports, "url-1-2", 1, buf, 1);

         s = html_encode(actions->url->spec);
         map(url_exports, "url", 1, s, 1);

         s = strdup(url_template);
         template_fill(&s, section_exports);
         template_fill(&s, url_exports);
         urls = strsav(urls, s);
         free_map(url_exports);

         ++urlid;
         url_1_2 = 3 - url_1_2;
         actions = actions->next;
         if (actions)
         {
            next_actions_html = actions_to_html(actions->action);
         }
      }
      while (actions && (0 == strcmp(actions_html, next_actions_html)));

      map(section_exports, "urls", 1, urls, 0);

      /* Could also do section-specific exports here, but it wouldn't be as fast */

      s = strdup(section_template);
      template_fill(&s, section_exports);
      sections = strsav(sections, s);
      free_map(section_exports);

      freez(actions_html);
      actions_html = next_actions_html;
   }

   map(exports, "sections", 1, sections, 0);

   /* Could also do global exports here, but it wouldn't be as fast */

   rsp->body = template_load(csp, "edit-actions-list");
   template_fill(&rsp->body, exports);
   free_map(exports);

   return(0);
}


/*********************************************************************
 *
 * Function    :  map_radio
 *
 * Description :  Map a set of radio button values.  E.g. if you have
 *                3 radio buttons, declare them as:
 *                  <option type="radio" name="xyz" @xyz-a@>
 *                  <option type="radio" name="xyz" @xyz-b@>
 *                  <option type="radio" name="xyz" @xyz-c@>
 *                Then map one of the @xyz-?@ variables to "checked"
 *                and all the others to empty by calling:
 *                map_radio(exports, "xyz", "abc", sel)
 *                Where 'sel' is 'a', 'b', or 'c'.
 *
 * Parameters  :
 *           1 :  exports = Exports map to modify.
 *           2 :  optionname = name for map
 *           3 :  values = null-terminated list of values;
 *           4 :  value = Selected value.
 *
 * CGI Parameters : None
 *
 * Returns     :  0 on success, nonzero on error.
 *
 *********************************************************************/
static int map_radio(struct map * exports, const char * optionname, 
                     const char * values, char value)
{
   int len;
   char * buf;
   char * p;
   char c;
   
   assert(exports);
   assert(optionname);
   assert(values);

   len = strlen(optionname);
   buf = malloc(len + 3);
   if (buf == NULL)
   {
      return 1;
   }

   strcpy(buf, optionname);
   p = buf + len;
   *p++ = '-';
   p[1] = '\0';

   while ((c = *values++) != '\0')
   {
      if (c != value)
      {
         *p = c;
         if (map(exports, buf, 1, "", 1))
         {
            free(buf);
            return 1;
         }
      }
   }

   *p = value;
   if (map(exports, buf, 0, "checked", 1))
   {
      free(buf);
      return 1;
   }

   return 0;
}


/*********************************************************************
 *
 * Function    :  actions_to_radio
 *
 * Description :  Converts a actionsfile entry FIXME
 *
 * Parameters  :
 *          1  :  exports = FIXME
 *          2  :  action  = FIXME
 *
 * Returns     :  0 on success, nonzero on error.
 *
 *********************************************************************/
static int actions_to_radio(struct map * exports, struct action_spec *action)
{
   unsigned mask = action->mask;
   unsigned add  = action->add;
   int mapped_param;
   int checked;

   /* sanity - prevents "-feature +feature" */
   mask |= add;


#define DEFINE_ACTION_BOOL(name, bit)       \
   if (!(mask & bit))                       \
   {                                        \
      map_radio(exports, name, "ynx", 'n'); \
   }                                        \
   else if (add & bit)                      \
   {                                        \
      map_radio(exports, name, "ynx", 'y'); \
   }                                        \
   else                                     \
   {                                        \
      map_radio(exports, name, "ynx", 'x'); \
   }

#define DEFINE_ACTION_STRING(name, bit, index)  \
   DEFINE_ACTION_BOOL(name, bit);               \
   mapped_param = 0;

#define DEFINE_CGI_PARAM_RADIO(name, bit, index, value, is_default) \
   if (add & bit)                                        \
   {                                                     \
      checked = !strcmp(action->string[index], value);   \
   }                                                     \
   else                                                  \
   {                                                     \
      checked = is_default;                              \
   }                                                     \
   mapped_param |= checked;                              \
   map(exports, name "-param-" value, 1, (checked ? "checked" : ""), 1);

#define DEFINE_CGI_PARAM_CUSTOM(name, bit, index, default_val)    \
   map(exports, name "-param-custom", 1,                      \
      ((!mapped_param) ? "checked" : ""), 1);                     \
   map(exports, name "-param", 1,                             \
      (((add & bit) && !mapped_param) ?                           \
      action->string[index] : default_val), 1);

#define DEFINE_CGI_PARAM_NO_RADIO(name, bit, index, default_val)  \
   map(exports, name "-param", 1,                             \
      ((add & bit) ? action->string[index] : default_val), 1);

#define DEFINE_ACTION_MULTI(name, index)        \
   if (action->multi_add[index]->first)         \
   {                                            \
      map_radio(exports, name, "ynx", 'y');     \
   }                                            \
   else if (action->multi_remove_all[index])    \
   {                                            \
      map_radio(exports, name, "ynx", 'n');     \
   }                                            \
   else if (action->multi_remove[index]->first) \
   {                                            \
      map_radio(exports, name, "ynx", 'y');     \
   }                                            \
   else                                         \
   {                                            \
      map_radio(exports, name, "ynx", 'x');     \
   }

#define DEFINE_ACTION_ALIAS 0 /* No aliases for output */

#include "actionlist.h"

#undef DEFINE_ACTION_MULTI
#undef DEFINE_ACTION_STRING
#undef DEFINE_ACTION_BOOL
#undef DEFINE_ACTION_ALIAS
#undef DEFINE_CGI_PARAM_CUSTOM
#undef DEFINE_CGI_PARAM_RADIO
#undef DEFINE_CGI_PARAM_NO_RADIO

   return 0;
}


/*********************************************************************
 *
 * Function    :  cgi_edit_actions
 *
 * Description :  CGI function that edits the Actions list.
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_edit_actions(struct client_state *csp, struct http_response *rsp,
   		            struct map *parameters)
{
   struct file_list *fl;
   struct url_actions *actions;
   struct map * exports = default_exports(csp, NULL);
   int sectionid;
   int i;
   const char * s;
   char c;

   if (((fl = csp->actions_list) == NULL) || ((actions = fl->f) == NULL))
   {
      /* FIXME: Oops, No file to edit */
      free_map(exports);
      return cgi_default(csp, rsp, parameters);
   }

   s = lookup(parameters, "section");
   if (!*s)
   {
      /* FIXME: Oops, No file to edit */
      free_map(exports);
      return cgi_default(csp, rsp, parameters);
   }

   sectionid = 0;
   while ((c = *s++) != '\0')
   {
      /* Note:
       * (((unsigned)(-1)) >> 1) == MAXINT.
       * (MAXINT - 9) / 10 is the largest number that 
       * can be safely multiplied by 10 then have 9 added.
       */
      if (c < '0' || c > '9' || sectionid > ((((unsigned)(-1)) >> 1) - 9) / 10)
      {
         /* FIXME: Oops, bad paramaters */
         free_map(exports);
         return cgi_default(csp, rsp, parameters);
      }

      sectionid *= 10;
      sectionid += c - '0';
   }

   for (i = 0; actions != NULL && i < sectionid; i++)
   {
      actions = actions->next;
   }

   if (actions == NULL || i != sectionid || sectionid < 1)
   {
      free_map(exports);
      return cgi_default(csp, rsp, parameters);
   }

   /* FIXME: need to fill in exports here */
   actions_to_radio(exports, actions->action);

   rsp->body = template_load(csp, "edit-actions");
   template_fill(&rsp->body, exports);
   free_map(exports);

   return(0);
}


/*********************************************************************
 *
 * Function    :  cgi_edit_actions_submit
 *
 * Description :  CGI function that actually edits the Actions list.
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  0
 *
 *********************************************************************/
int cgi_edit_actions_submit(struct client_state *csp, struct http_response *rsp,
   		            struct map *parameters)
{
   struct map * exports = default_exports(csp, NULL);

   map(exports, "cgi-parameters", 1, dump_map(parameters), 0);

   rsp->body = template_load(csp, "edit-actions-submit");
   template_fill(&rsp->body, exports);
   free_map(exports);

   return(0);
}


#endif /* def FEATURE_CGI_EDIT_ACTIONS */


/*
  Local Variables:
  tab-width: 3
  end:
*/
