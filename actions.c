const char actions_rcs[] = "$Id: actions.c,v ???? $";
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/actions.c,v $
 *
 * Purpose     :  Declares functions to work with actions files
 *                Functions declared include: FIXME
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
 *    $Log: actions.c,v $
 *
 *********************************************************************/


#include "config.h"

#include <stdio.h>
#include <string.h>

#include "project.h"
#include "jcc.h"
#include "list.h"
#include "actions.h"
#include "miscutil.h"
#include "errlog.h"
#include "loaders.h"

const char actions_h_rcs[] = ACTIONS_H_VERSION;

/* This structure is used to hold user-defined aliases */
struct action_alias
{
   const char * name;
   struct action_spec action[1];
   struct action_alias * next;
};


/*
 * Must declare this in this file for the above structure.
 * FIXME: Make this static or put structure in header.
 */
extern int get_actions (char *line, struct action_alias * alias_list,
                        struct action_spec *cur_action);

/*
 * We need the main list of options.
 *
 * First, we need a way to tell between boolean, string, and multi-string
 * options.  For string and multistring options, we also need to be
 * able to tell the difference between a "+" and a "-".  (For bools,
 * the "+"/"-" information is encoded in "add" and "mask").  So we use
 * an enumerated type (well, the preprocessor equivalent).  Here are
 * the values:
 */
#define AV_NONE       0 /* +opt -opt */
#define AV_ADD_STRING 1 /* +stropt{string} */
#define AV_REM_STRING 2 /* -stropt */
#define AV_ADD_MULTI  3 /* +multiopt{string} +multiopt{string2} */
#define AV_REM_MULTI  4 /* -multiopt{string} -multiopt{*}       */

/*
 * We need a structure to hold the name, flag changes, 
 * type, and string index.
 */
struct action_name
{
   const char * name;
   unsigned mask;   /* a bit set to "0" = remove action */
   unsigned add;    /* a bit set to "1" = add action */
   int takes_value; /* an AV_... constant */
   int index;       /* index into strings[] or multi[] */
};

/*
 * And with those building blocks in place, here's the array.
 */
static const struct action_name action_names[] =
{
   /*
    * Well actually there's no data here - it's in actionlist.h
    * This keeps it together to make it easy to change.
    *
    * Here's the macros used to format it:
    */
#define DEFINE_ACTION_MULTI(name,index)                   \
   { "+" name, ACTION_MASK_ALL, 0, AV_ADD_MULTI, index }, \
   { "-" name, ACTION_MASK_ALL, 0, AV_REM_MULTI, index },
#define DEFINE_ACTION_STRING(name,flag,index)                 \
   { "+" name, ACTION_MASK_ALL, flag, AV_ADD_STRING, index }, \
   { "-" name, ~flag, 0, AV_REM_STRING, index },
#define DEFINE_ACTION_BOOL(name,flag)   \
   { "+" name, ACTION_MASK_ALL, flag }, \
   { "-" name, ~flag, 0 },
#define DEFINE_ACTION_ALIAS 1 /* Want aliases please */

#include "actionlist.h"

#undef DEFINE_ACTION_MULTI
#undef DEFINE_ACTION_STRING
#undef DEFINE_ACTION_BOOL
#undef DEFINE_ACTION_ALIAS

 /*
   { "+add-header",        ACTION_MASK_ALL, 0,                      AV_ADD_MULTI,  ACTION_MULTI_ADD_HEADER },
   { "+block",             ACTION_MASK_ALL, ACTION_BLOCK           },
   { "+fast-redirects",    ACTION_MASK_ALL, ACTION_FAST_REDIRECTS  },
   { "+filter",            ACTION_MASK_ALL, ACTION_FILTER          },
   { "+hide-forwarded",    ACTION_MASK_ALL, ACTION_HIDE_FORWARDED  },
   { "+hide-from",         ACTION_MASK_ALL, ACTION_HIDE_FROM,       AV_ADD_STRING, ACTION_STRING_FROM       },
   { "+hide-referer",      ACTION_MASK_ALL, ACTION_HIDE_REFERER,    AV_ADD_STRING, ACTION_STRING_REFERER    },
   { "+hide-referrer",     ACTION_MASK_ALL, ACTION_HIDE_REFERER,    AV_ADD_STRING, ACTION_STRING_REFERER    },
   { "+hide-user-agent",   ACTION_MASK_ALL, ACTION_HIDE_USER_AGENT, AV_ADD_STRING, ACTION_STRING_USER_AGENT },
   { "+image",             ACTION_MASK_ALL, ACTION_IMAGE           },
   { "+no-cookies-read",   ACTION_MASK_ALL, ACTION_NO_COOKIE_READ  },
   { "+no-cookies-set",    ACTION_MASK_ALL, ACTION_NO_COOKIE_SET   },
   { "+no-popup",          ACTION_MASK_ALL, ACTION_NO_POPUPS       },
   { "+no-popups",         ACTION_MASK_ALL, ACTION_NO_POPUPS       },
   { "+vanilla-wafer",     ACTION_MASK_ALL, ACTION_VANILLA_WAFER   },
   { "+wafer",             ACTION_MASK_ALL, 0,                      AV_ADD_MULTI,  ACTION_MULTI_WAFER },
   { "-add-header",        ACTION_MASK_ALL,         0, AV_REM_MULTI,  ACTION_MULTI_ADD_HEADER },
   { "-block",             ~ACTION_BLOCK,           0 },
   { "-fast-redirects",    ~ACTION_FAST_REDIRECTS,  0 },
   { "-filter",            ~ACTION_FILTER,          0 },
   { "-hide-forwarded",    ~ACTION_HIDE_FORWARDED,  0 },
   { "-hide-from",         ~ACTION_HIDE_FROM,       0, AV_REM_STRING, ACTION_STRING_FROM       },
   { "-hide-referer",      ~ACTION_HIDE_REFERER,    0, AV_REM_STRING, ACTION_STRING_REFERER    },
   { "-hide-referrer",     ~ACTION_HIDE_REFERER,    0, AV_REM_STRING, ACTION_STRING_REFERER    },
   { "-hide-user-agent",   ~ACTION_HIDE_USER_AGENT, 0, AV_REM_STRING, ACTION_STRING_USER_AGENT },
   { "-image",             ~ACTION_IMAGE,           0 },
   { "-no-cookies-read",   ~ACTION_NO_COOKIE_READ,  0 },
   { "-no-cookies-set",    ~ACTION_NO_COOKIE_SET,   0 },
   { "-no-popup",          ~ACTION_NO_POPUPS,       0 },
   { "-no-popups",         ~ACTION_NO_POPUPS,       0 },
   { "-vanilla-wafer",     ~ACTION_VANILLA_WAFER,   0 },
   { "-wafer",             ACTION_MASK_ALL,         0, AV_REM_MULTI,  ACTION_MULTI_WAFER },
*/
   { NULL, 0, 0 } /* End marker */
};


/*********************************************************************
 *
 * Function    :  merge_actions
 *
 * Description :  Merge two actions together.
 *                Similar to "cur_action += new_action".
 *
 * Parameters  :
 *          1  :  cur_action = Current actions, to modify.
 *          2  :  new_action = Action to add.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void merge_actions (struct action_spec *dest, 
                    const struct action_spec *src)
{
   int i;

   dest->mask &= src->mask;
   dest->add  &= src->mask;
   dest->add  |= src->add;

   for (i = 0; i < ACTION_STRING_COUNT; i++)
   {
      char * str = src->string[i];
      if (str)
      {
         freez(dest->string[i]);
         dest->string[i] = strdup(str);
      }
   }

   for (i = 0; i < ACTION_MULTI_COUNT; i++)
   {
      if (src->multi_remove_all[i])
      {
         /* Remove everything from dest */
         destroy_list(dest->multi_remove[i]);
         destroy_list(dest->multi_add[i]);
         dest->multi_remove_all[i] = 1;
         list_duplicate(dest->multi_add[i], src->multi_add[i]);
      }
      else if (dest->multi_remove_all[i])
      {
         /*
          * dest already removes everything, so we only need to worry
          * about what we add.
          */
         list_remove_list(dest->multi_add[i], src->multi_remove[i]);
         list_append_list_unique(dest->multi_add[i], src->multi_add[i]);
      }
      else
      {
         /* No "remove all"s to worry about. */
         list_remove_list(dest->multi_add[i], src->multi_remove[i]);
         list_append_list_unique(dest->multi_remove[i], src->multi_remove[i]);
         list_append_list_unique(dest->multi_add[i], src->multi_add[i]);
      }
   }
}


/*********************************************************************
 *
 * Function    :  copy_action
 *
 * Description :  Copy an action_specs.
 *                Similar to "cur_action = new_action".
 *
 * Parameters  :
 *          1  :  dest = Destination of copy.
 *          2  :  src = Source for copy.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void copy_action (struct action_spec *dest, 
                  const struct action_spec *src)
{
   int i;

   dest->mask = src->mask;
   dest->add  = src->add;

   for (i = 0; i < ACTION_STRING_COUNT; i++)
   {
      char * str = src->string[i];
      dest->string[i] = (str ? strdup(str) : NULL);
   }

   for (i = 0; i < ACTION_MULTI_COUNT; i++)
   {
      dest->multi_remove_all[i] = src->multi_remove_all[i];
      list_duplicate(dest->multi_remove[i], src->multi_remove[i]);
      list_duplicate(dest->multi_add[i],    src->multi_add[i]);
   }
}


/*********************************************************************
 *
 * Function    :  free_action
 *
 * Description :  Free an action_specs.
 *
 * Parameters  :
 *          1  :  src = Source to free.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void free_action (struct action_spec *src)
{
   int i;

   for (i = 0; i < ACTION_STRING_COUNT; i++)
   {
      freez(src->string[i]);
   }

   for (i = 0; i < ACTION_MULTI_COUNT; i++)
   {
      destroy_list(src->multi_remove[i]);
      destroy_list(src->multi_add[i]);
   }

   memset(src, '\0', sizeof(*src));
}


/*********************************************************************
 *
 * Function    :  get_action_token
 *
 * Description :  Parses a line for the first action.
 *                Modifies it's input array, doesn't allocate memory.
 *                e.g. given:
 *                *line="  +abc{def}  -ghi "
 *                Returns:
 *                *line="  -ghi "
 *                *name="+abc"
 *                *value="def"
 *
 * Parameters  :
 *          1  :  line = [in] The line containing the action.
 *                       [out] Start of next action on line, or
 *                       NULL if we reached the end of line before
 *                       we found an action.
 *          2  :  name = [out] Start of action name, null
 *                       terminated.  NULL on EOL
 *          3  :  value = [out] Start of action value, null 
 *                        terminated.  NULL if none or EOL.
 *
 * Returns     :  0 => Ok
 *                nonzero => Mismatched {} (line was trashed anyway)
 *
 *********************************************************************/
int get_action_token(char **line, char **name, char **value)
{
   char * str = *line;
   char ch;

   /* set default returns */
   *line = NULL;
   *name = NULL;
   *value = NULL;

   /* Eat any leading whitespace */
   while ((*str == ' ') || (*str == '\t'))
   {
      str++;
   }

   if (*str == '\0')
   {
      return 0;
   }

   if (*str == '{')
   {
      /* null name, just value is prohibited */
      return 1;
   }

   *name = str;

   /* parse option */
   while (((ch = *str) != '\0') && 
          (ch != ' ') && (ch != '\t') && (ch != '{'))
   {
      if (ch == '}')
      {
         /* error */
         return 1;
      }
      str++;
   }
   *str = '\0';

   if (ch != '{')
   {
      /* no value */
      if (ch == '\0')
      {
         /* EOL - be careful not to run off buffer */
         *line = str;
      }
      else
      {
         /* More to parse next time. */
         *line = str + 1;
      }
      return 0;
   }

   str++;
   *value = str;

   str = strchr(str, '}');
   if (str == NULL)
   {
      /* error */
      *value = NULL;
      return 1;
   }

   /* got value */
   *str = '\0';
   *line = str + 1;

   chomp(*value);

   return 0;
}


/*********************************************************************
 *
 * Function    :  get_actions
 *
 * Description :  Parses a list of actions.
 *
 * Parameters  :
 *          1  :  line = The string containing the actions.
 *                       Will be written to by this function.
 *          2  :  aliases = Custom alias list, or NULL for none.
 *          3  :  cur_action = Where to store the action.  Caller
 *                             allocates memory.
 *
 * Returns     :  0 => Ok
 *                nonzero => Error (line was trashed anyway)
 *
 *********************************************************************/
int get_actions(char *line, struct action_alias * alias_list,
                struct action_spec *cur_action)
{
   memset(cur_action, '\0', sizeof(*cur_action));
   cur_action->mask = ACTION_MASK_ALL;

   while (line)
   {
      char * option = NULL;
      char * value = NULL;

      if (get_action_token(&line, &option, &value))
      {
         return 1;
      }

      if (option)
      {
         /* handle option in 'option' */
      
         /* Check for standard action name */
         const struct action_name * action = action_names;

         while ( (action->name != NULL) && (0 != strcmpic(action->name, option)) )
         {
            action++;
         }
         if (action->name != NULL)
         {
            /* Found it */
            cur_action->mask &= action->mask;
            cur_action->add  &= action->mask;
            cur_action->add  |= action->add;

            switch (action->takes_value)
            {
            case AV_NONE:
               /* ignore any option. */
               break;
            case AV_ADD_STRING:
               {
                  /* add single string. */

                  if ((value == NULL) || (*value == '\0'))
                  {
                     return 1;
                  }
                  /* FIXME: should validate option string here */
                  freez (cur_action->string[action->index]);
                  cur_action->string[action->index] = strdup(value);
                  break;
               }
            case AV_REM_STRING:
               {
                  /* remove single string. */

                  freez (cur_action->string[action->index]);
                  break;
               }
            case AV_ADD_MULTI:
               {
                  /* append multi string. */

                  struct list * remove = cur_action->multi_remove[action->index];
                  struct list * add    = cur_action->multi_add[action->index];

                  if ((value == NULL) || (*value == '\0'))
                  {
                     return 1;
                  }

                  list_remove_item(remove, value);
                  enlist_unique(add, value);
                  break;
               }
            case AV_REM_MULTI:
               {
                  /* remove multi string. */

                  struct list * remove = cur_action->multi_remove[action->index];
                  struct list * add    = cur_action->multi_add[action->index];

                  if ( (value == NULL) || (*value == '\0')
                     || ((*value == '*') && (value[1] == '\0')) )
                  {
                     /*
                      * no option, or option == "*".
                      *
                      * Remove *ALL*.
                      */
                     destroy_list(remove);
                     destroy_list(add);
                     cur_action->multi_remove_all[action->index] = 1;
                  }
                  else
                  {
                     /* Valid option - remove only 1 option */

                     if ( !cur_action->multi_remove_all[action->index] )
                     {
                        /* there isn't a catch-all in the remove list already */
                        enlist_unique(remove, value);
                     }
                     list_remove_item(add, value);
                  }
                  break;
               }
            default:
               /* Shouldn't get here unless there's memory corruption. */
               return 1;
            }
         }
         else
         {
            /* try user aliases. */
            const struct action_alias * alias = alias_list;
            
            while ( (alias != NULL) && (0 != strcmpic(alias->name, option)) )
            {
               alias = alias->next;
            }
            if (alias != NULL)
            {
               /* Found it */
               merge_actions(cur_action, alias->action);
            }
            else
            {
               /* Bad action name */
               return 1;
            }
         }
      }
   }

   return 0;
}


/*********************************************************************
 *
 * Function    :  actions_to_text
 *
 * Description :  Converts a actionsfile entry from numeric form
 *                ("mask" and "add") to text.
 *
 * Parameters  :
 *          1  :  mask = As from struct url_actions
 *          2  :  add  = As from struct url_actions
 *
 * Returns     :  A string.  Caller must free it.
 *
 *********************************************************************/
char * actions_to_text(struct action_spec *action)
{
   unsigned mask = action->mask;
   unsigned add  = action->add;
   char * result = strdup("");
   struct list * lst;

   /* sanity - prevents "-feature +feature" */
   mask |= add;


#define DEFINE_ACTION_BOOL(__name, __bit)   \
   if (!(mask & __bit))                     \
   {                                        \
      result = strsav(result, " -" __name); \
   }                                        \
   else if (add & __bit)                    \
   {                                        \
      result = strsav(result, " +" __name); \
   }

#define DEFINE_ACTION_STRING(__name, __bit, __index) \
   if (!(mask & __bit))                     \
   {                                        \
      result = strsav(result, " -" __name); \
   }                                        \
   else if (add & __bit)                    \
   {                                        \
      result = strsav(result, " +" __name "{"); \
      result = strsav(result, action->string[__index]); \
      result = strsav(result, "}"); \
   }

#define DEFINE_ACTION_MULTI(__name, __index)         \
   if (action->multi_remove_all[__index])            \
   {                                                 \
      result = strsav(result, " -" __name "{*}");    \
   }                                                 \
   else                                              \
   {                                                 \
      lst = action->multi_remove[__index]->next;     \
      while (lst)                                    \
      {                                              \
         result = strsav(result, " -" __name "{");   \
         result = strsav(result, lst->str);          \
         result = strsav(result, "}");               \
         lst = lst->next;                            \
      }                                              \
   }                                                 \
   lst = action->multi_add[__index]->next;           \
   while (lst)                                       \
   {                                                 \
      result = strsav(result, " +" __name "{");      \
      result = strsav(result, lst->str);             \
      result = strsav(result, "}");                  \
      lst = lst->next;                               \
   }

#define DEFINE_ACTION_ALIAS 0 /* No aliases for output */

#include "actionlist.h"

#undef DEFINE_ACTION_MULTI
#undef DEFINE_ACTION_STRING
#undef DEFINE_ACTION_BOOL
#undef DEFINE_ACTION_ALIAS
/*   
   ACTION_MULTI_TO_TEXT (                        "add-header",      ACTION_MULTI_ADD_HEADER)
   ACTION_TO_TEXT       (ACTION_BLOCK,           "block");
   ACTION_TO_TEXT       (ACTION_FAST_REDIRECTS,  "fast-redirects");
   ACTION_TO_TEXT       (ACTION_FILTER,          "filter");
   ACTION_TO_TEXT       (ACTION_HIDE_FORWARDED,  "hide-forwarded");
   ACTION_STRING_TO_TEXT(ACTION_HIDE_FROM,       "hide-from",       ACTION_STRING_FROM);
   ACTION_STRING_TO_TEXT(ACTION_HIDE_REFERER,    "hide-referer",    ACTION_STRING_REFERER);
   ACTION_STRING_TO_TEXT(ACTION_HIDE_USER_AGENT, "hide-user-agent", ACTION_STRING_USER_AGENT);
   ACTION_TO_TEXT       (ACTION_IMAGE,           "image");
   ACTION_TO_TEXT       (ACTION_NO_COOKIE_READ,  "no-cookies-read");
   ACTION_TO_TEXT       (ACTION_NO_COOKIE_SET,   "no-cookies-set");
   ACTION_TO_TEXT       (ACTION_NO_POPUPS,       "no-popups");
   ACTION_TO_TEXT       (ACTION_VANILLA_WAFER,   "vanilla-wafer");
   ACTION_MULTI_TO_TEXT (                        "wafer",           ACTION_MULTI_WAFER);

#undef ACTION_TO_TEXT
#undef ACTION_STRING_TO_TEXT
#undef ACTION_MULTI_TO_TEXT
*/
   return result;
}


/*********************************************************************
 *
 * Function    :  current_actions_to_text
 *
 * Description :  Converts a actionsfile entry to text.
 *
 * Parameters  :
 *          1  :  action = Action
 *
 * Returns     :  A string.  Caller must free it.
 *
 *********************************************************************/
char * current_action_to_text(struct current_action_spec *action)
{
   unsigned flags  = action->flags;
   char * result = strdup("");
   struct list_share * lst;

#define DEFINE_ACTION_BOOL(__name, __bit)   \
   if (flags & __bit)                       \
   {                                        \
      result = strsav(result, " +" __name); \
   }                                        \
   else                                     \
   {                                        \
      result = strsav(result, " -" __name); \
   }

#define DEFINE_ACTION_STRING(__name, __bit, __index)    \
   if (flags & __bit)                                   \
   {                                                    \
      result = strsav(result, " +" __name "{");         \
      result = strsav(result, action->string[__index]); \
      result = strsav(result, "}");                     \
   }                                                    \
   else                                                 \
   {                                                    \
      result = strsav(result, " -" __name);             \
   }

#define DEFINE_ACTION_MULTI(__name, __index)            \
   lst = action->multi[__index]->next;                  \
   if (lst == NULL)                                     \
   {                                                    \
      result = strsav(result, " -" __name);             \
   }                                                    \
   else                                                 \
   {                                                    \
      while (lst)                                       \
      {                                                 \
         result = strsav(result, " +" __name "{");      \
         result = strsav(result, lst->str);             \
         result = strsav(result, "}");                  \
         lst = lst->next;                               \
      }                                                 \
   }

#define DEFINE_ACTION_ALIAS 0 /* No aliases for output */

#include "actionlist.h"

#undef DEFINE_ACTION_MULTI
#undef DEFINE_ACTION_STRING
#undef DEFINE_ACTION_BOOL
#undef DEFINE_ACTION_ALIAS
/*   

   ACTION_MULTI_TO_TEXT (                        "add-header",      ACTION_MULTI_ADD_HEADER)
   ACTION_TO_TEXT       (ACTION_BLOCK,           "block");
   ACTION_TO_TEXT       (ACTION_FAST_REDIRECTS,  "fast-redirects");
   ACTION_TO_TEXT       (ACTION_FILTER,          "filter");
   ACTION_TO_TEXT       (ACTION_HIDE_FORWARDED,  "hide-forwarded");
   ACTION_STRING_TO_TEXT(ACTION_HIDE_FROM,       "hide-from",       ACTION_STRING_FROM);
   ACTION_STRING_TO_TEXT(ACTION_HIDE_REFERER,    "hide-referer",    ACTION_STRING_REFERER);
   ACTION_STRING_TO_TEXT(ACTION_HIDE_USER_AGENT, "hide-user-agent", ACTION_STRING_USER_AGENT);
   ACTION_TO_TEXT       (ACTION_IMAGE,           "image");
   ACTION_TO_TEXT       (ACTION_NO_COOKIE_READ,  "no-cookies-read");
   ACTION_TO_TEXT       (ACTION_NO_COOKIE_SET,   "no-cookies-set");
   ACTION_TO_TEXT       (ACTION_NO_POPUPS,       "no-popups");
   ACTION_TO_TEXT       (ACTION_VANILLA_WAFER,   "vanilla-wafer");
   ACTION_MULTI_TO_TEXT (                        "wafer",           ACTION_MULTI_WAFER);

#undef ACTION_TO_TEXT
#undef ACTION_STRING_TO_TEXT
#undef ACTION_MULTI_TO_TEXT
*/
   return result;
}


/*********************************************************************
 *
 * Function    :  init_current_action
 *
 * Description :  Zero out an action.
 *
 * Parameters  :
 *          1  :  dest = An uninitialized current_action_spec.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void init_current_action (struct current_action_spec *dest)
{
   memset(dest, '\0', sizeof(*dest));
   dest->flags = ACTION_MOST_COMPATIBLE;
}


/*********************************************************************
 *
 * Function    :  merge_current_action
 *
 * Description :  Merge two actions together.
 *                Similar to "dest += src".
 *                Differences between this and merge_actions()
 *                is that this one doesn't allocate memory for
 *                strings (so "src" better be in memory for at least
 *                as long as "dest" is, and you'd better free
 *                "dest" using "current_free_action").
 *                Also, there is no  mask or remove lists in dest.
 *                (If we're applying it to a URL, we don't need them)
 *
 * Parameters  :
 *          1  :  dest = Current actions, to modify.
 *          2  :  src = Action to add.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void merge_current_action (struct current_action_spec *dest, 
                           const struct action_spec *src)
{
   int i;

   dest->flags  &= src->mask;
   dest->flags  |= src->add;

   for (i = 0; i < ACTION_STRING_COUNT; i++)
   {
      char * str = src->string[i];
      if (str)
      {
         dest->string[i] = str;
      }
   }

   for (i = 0; i < ACTION_MULTI_COUNT; i++)
   {
      if (src->multi_remove_all[i])
      {
         /* Remove everything from dest */
         destroy_list_share(dest->multi[i]);
         list_duplicate_share(dest->multi[i], src->multi_add[i]);
      }
      else
      {
         list_remove_list_share(dest->multi[i], src->multi_remove[i]);
         list_append_list_unique_share(dest->multi[i], src->multi_add[i]);
      }
   }
}


/*********************************************************************
 *
 * Function    :  free_current_action
 *
 * Description :  Free a current_action_spec.
 *
 * Parameters  :
 *          1  :  src = Source to free.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void free_current_action (struct current_action_spec *src)
{
   int i;

   for (i = 0; i < ACTION_MULTI_COUNT; i++)
   {
      destroy_list_share(src->multi[i]);
   }

   memset(src, '\0', sizeof(*src));
}


/*********************************************************************
 *
 * Function    :  unload_actions_file
 *
 * Description :  Unloads an actions module.
 *
 * Parameters  :
 *          1  :  file_data = the data structure associated with the
 *                            actions file.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void unload_actions_file(void *file_data)
{
   struct url_actions * next;
   struct url_actions * cur = (struct url_actions *)file_data;
   while (cur != NULL)
   {
      next = cur->next;
      free_url(cur->url);
      freez(cur);
      cur = next;
   }

}


/*********************************************************************
 *
 * Function    :  load_actions_file
 *
 * Description :  Read and parse a action file and add to files
 *                list.
 *
 * Parameters  :
 *          1  :  csp = Current client state (buffers, headers, etc...)
 *
 * Returns     :  0 => Ok, everything else is an error.
 *
 *********************************************************************/
int load_actions_file(struct client_state *csp)
{
   static struct file_list *current_actions_file  = NULL;

   FILE *fp;

   struct url_actions *last_perm;
   struct url_actions *perm;
   char  buf[BUFSIZ];
   struct file_list *fs;
#define MODE_START_OF_FILE 1
#define MODE_ACTIONS       2
#define MODE_ALIAS         3
   int mode = MODE_START_OF_FILE;
   struct action_spec cur_action[1];
   struct action_alias * alias_list = NULL;

   memset(cur_action, '\0', sizeof(*cur_action));

   if (!check_file_changed(current_actions_file, csp->config->actions_file, &fs))
   {
      /* No need to load */
      if (csp)
      {
         csp->actions_list = current_actions_file;
      }
      return 0;
   }
   if (!fs)
   {
      log_error(LOG_LEVEL_FATAL, "can't load actions file '%s': error finding file: %E",
                csp->config->actions_file);
      return 1; /* never get here */
   }

   fs->f = last_perm = (struct url_actions *)zalloc(sizeof(*last_perm));
   if (last_perm == NULL)
   {
      log_error(LOG_LEVEL_FATAL, "can't load actions file '%s': out of memory!",
                csp->config->actions_file);
      return 1; /* never get here */
   }

   if ((fp = fopen(csp->config->actions_file, "r")) == NULL)
   {
      log_error(LOG_LEVEL_FATAL, "can't load actions file '%s': error opening file: %E",
                csp->config->actions_file);
      return 1; /* never get here */
   }

   while (read_config_line(buf, sizeof(buf), fp, fs) != NULL)
   {
      if (*buf == '{')
      {
         /* It's a header block */
         if (buf[1] == '{')
         {
            /* It's {{settings}} or {{alias}} */
            int len = strlen(buf);
            char * start = buf + 2;
            char * end = buf + len - 1;
            if ((len < 5) || (*end-- != '}') || (*end-- != '}'))
            {
               /* too short */
               fclose(fp);
               log_error(LOG_LEVEL_FATAL, 
                  "can't load actions file '%s': invalid line: %s",
                  csp->config->actions_file, buf);
               return 1; /* never get here */
            }

            /* Trim leading and trailing whitespace. */
            end[1] = '\0';
            chomp(start);

            if (*start == '\0')
            {
               /* too short */
               fclose(fp);
               log_error(LOG_LEVEL_FATAL, 
                  "can't load actions file '%s': invalid line: {{ }}",
                  csp->config->actions_file);
               return 1; /* never get here */
            }

            if (0 == strcmpic(start, "alias"))
            {
               /* it's an {{alias}} block */

               mode = MODE_ALIAS;
            }
            else
            {
               /* invalid {{something}} block */
               fclose(fp);
               log_error(LOG_LEVEL_FATAL, 
                  "can't load actions file '%s': invalid line: {{%s}}",
                  csp->config->actions_file, start);
               return 1; /* never get here */
            }
         }
         else
         {
            /* It's an actions block */

            char  actions_buf[BUFSIZ];
            char * end;

            /* set mode */
            mode    = MODE_ACTIONS;

            /* free old action */
            free_action(cur_action);

            /* trim { */
            strcpy(actions_buf, buf + 1);

            /* check we have a trailing } and then trim it */
            end = actions_buf + strlen(actions_buf) - 1;
            if (*end != '}')
            {
               /* too short */
               fclose(fp);
               log_error(LOG_LEVEL_FATAL, 
                  "can't load actions file '%s': invalid line: %s",
                  csp->config->actions_file, buf);
               return 1; /* never get here */
            }
            *end = '\0';

            /* trim any whitespace immediately inside {} */
            chomp(actions_buf);

            if (*actions_buf == '\0')
            {
               /* too short */
               fclose(fp);
               log_error(LOG_LEVEL_FATAL, 
                  "can't load actions file '%s': invalid line: %s",
                  csp->config->actions_file, buf);
               return 1; /* never get here */
            }

            if (get_actions(actions_buf, alias_list, cur_action))
            {
               /* error */
               fclose(fp);
               log_error(LOG_LEVEL_FATAL, 
                  "can't load actions file '%s': invalid line: %s",
                  csp->config->actions_file, buf);
               return 1; /* never get here */
            }
         }
      }
      else if (mode == MODE_ALIAS)
      {
         /* define an alias */
         char  actions_buf[BUFSIZ];
         struct action_alias * new_alias;
         int more = 1;

         char * start = strchr(buf, '=');
         char * end = start;

         if ((start == NULL) || (start == buf))
         {
            log_error(LOG_LEVEL_FATAL, 
               "can't load actions file '%s': invalid alias line: %s",
               csp->config->actions_file, buf);
            return 1; /* never get here */
         }

         if ((new_alias = zalloc(sizeof(*new_alias))) == NULL)
         {
            fclose(fp);
            log_error(LOG_LEVEL_FATAL,
               "can't load actions file '%s': out of memory!",
               csp->config->actions_file);
            return 1; /* never get here */
         }

         /* Eat any the whitespace before the '=' */
         end--;
         while ((*end == ' ') || (*end == '\t'))
         {
            /*
             * we already know we must have at least 1 non-ws char
             * at start of buf - no need to check
             */
            end--;
         }
         end[1] = '\0';

         /* Eat any the whitespace after the '=' */
         start++;
         while ((*start == ' ') || (*start == '\t'))
         {
            start++;
         }
         if (*start == '\0')
         {
            log_error(LOG_LEVEL_FATAL, 
               "can't load actions file '%s': invalid alias line: %s",
               csp->config->actions_file, buf);
            return 1; /* never get here */
         }

         new_alias->name = strdup(buf);

         strcpy(actions_buf, start);

         if (get_actions(actions_buf, alias_list, new_alias->action))
         {
            /* error */
            fclose(fp);
            log_error(LOG_LEVEL_FATAL, 
               "can't load actions file '%s': invalid alias line: %s = %s",
               csp->config->actions_file, buf, start);
            return 1; /* never get here */
         }
         
         /* add to list */
         new_alias->next = alias_list;
         alias_list = new_alias;
      }
      else if (mode == MODE_ACTIONS)
      {
         /* it's a URL pattern */

         /* allocate a new node */
         if ((perm = zalloc(sizeof(*perm))) == NULL)
         {
            fclose(fp);
            log_error(LOG_LEVEL_FATAL,
               "can't load actions file '%s': out of memory!",
               csp->config->actions_file);
            return 1; /* never get here */
         }

         /* Save flags */
         copy_action (perm->action, cur_action);

         /* Save the URL pattern */
         if (create_url_spec(perm->url, buf))
         {
            fclose(fp);
            log_error(LOG_LEVEL_FATAL, 
               "can't load actions file '%s': cannot create URL pattern from: %s",
               csp->config->actions_file, buf);
            return 1; /* never get here */
         }

         /* add it to the list */
         last_perm->next = perm;
         last_perm = perm;
      }
      else if (mode == MODE_START_OF_FILE)
      {
         /* oops - please have a {} line as 1st line in file. */
         fclose(fp);
         log_error(LOG_LEVEL_FATAL, 
            "can't load actions file '%s': first line is invalid: %s",
            csp->config->actions_file, buf);
         return 1; /* never get here */
      }
      else
      {
         /* How did we get here? This is impossible! */
         fclose(fp);
         log_error(LOG_LEVEL_FATAL, 
            "can't load actions file '%s': INTERNAL ERROR - mode = %d",
            csp->config->actions_file, mode);
         return 1; /* never get here */
      }
   }

   fclose(fp);
   
   free_action(cur_action);

   while (alias_list != NULL)
   {
      struct action_alias * next = alias_list->next;
      freez((char *)alias_list->name);
      free_action(alias_list->action);
      free(alias_list);
      alias_list = next;
   }

#ifndef SPLIT_PROXY_ARGS
   if (!suppress_blocklists)
   {
      fs->proxy_args = strsav(fs->proxy_args, "</pre>");
   }
#endif /* ndef SPLIT_PROXY_ARGS */

   /* the old one is now obsolete */
   if (current_actions_file)
   {
      current_actions_file->unloader = unload_actions_file;
   }

   fs->next    = files->next;
   files->next = fs;
   current_actions_file = fs;

   if (csp)
   {
      csp->actions_list = fs;
   }

   return(0);

}
