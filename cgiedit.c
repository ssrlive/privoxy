const char cgiedit_rcs[] = "$Id: cgiedit.c,v 1.2 2001/09/16 17:05:14 jongfoster Exp $";
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
 *    $Log: cgiedit.c,v $
 *    Revision 1.2  2001/09/16 17:05:14  jongfoster
 *    Removing unused #include showarg.h
 *
 *    Revision 1.1  2001/09/16 15:47:37  jongfoster
 *    First version of CGI-based edit interface.  This is very much a
 *    work-in-progress, and you can't actually use it to edit anything
 *    yet.  You must #define FEATURE_CGI_EDIT_ACTIONS for these changes
 *    to have any effect.
 *
 *
 **********************************************************************/


#include "config.h"

/*
 * FIXME: Following includes copied from cgi.c - which are actually needed?
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#ifdef _WIN32
#define snprintf _snprintf
#endif /* def _WIN32 */

#include "project.h"
#include "cgi.h"
#include "cgiedit.h"
#include "cgisimple.h"
#include "list.h"
#include "encode.h"
#include "actions.h"
#include "miscutil.h"
#include "errlog.h"

const char cgiedit_h_rcs[] = CGIEDIT_H_VERSION;


#ifdef FEATURE_CGI_EDIT_ACTIONS

struct file_line
{
   struct file_line * next;
   char * raw;
   char * prefix;
   char * unprocessed;
   int type;
   
   union
   {
      struct action_spec action[1];

      struct
      {
         char * name;
         char * svalue;
         int ivalue;
      } setting;

      /* Add more data types here... e.g.


      struct url_spec url[1];

      struct
      {
         struct action_spec action[1];
         const char * name;
      } alias;

      */

   } data;
};

#define FILE_LINE_UNPROCESSED           1
#define FILE_LINE_BLANK                 2
#define FILE_LINE_ALIAS_HEADER          3
#define FILE_LINE_ALIAS_ENTRY           4
#define FILE_LINE_ACTION                5
#define FILE_LINE_URL                   6
#define FILE_LINE_SETTINGS_HEADER       7
#define FILE_LINE_SETTINGS_ENTRY        8
#define FILE_LINE_DESCRIPTION_HEADER    9
#define FILE_LINE_DESCRIPTION_ENTRY    10

/* FIXME: Following list of prototypes is not complete */
/* FIXME: Following non-static functions should be prototyped in .h or made static */
static int  simple_read_line(char **dest, FILE *fp);
static int  edit_read_line  (FILE *fp, char **raw_out, char **prefix_out, char **data_out);
       int  edit_read_file  (FILE *fp, struct file_line ** pfile);
       int  edit_write_file (const char * filename, const struct file_line * file);
       void edit_free_file  (struct file_line * file);


/* FIXME: This should be in project.h and used everywhere */
#define JB_ERR_OK         0 /* Success, no error                        */
#define JB_ERR_MEMORY     1 /* Out of memory                            */
#define JB_ERR_CGI_PARAMS 2 /* Missing or corrupt CGI parameters        */
#define JB_ERR_FILE       3 /* Error opening, reading or writing a file */
#define JB_ERR_PARSE      4 /* Error parsing file                       */
#define JB_ERR_MODIFIED   5 /* File has been modified outside of the    */
                            /* CGI actions editor.                      */


/*********************************************************************
 *
 * Function    :  simple_read_line
 *
 * Description :  Read a single line from a file and return it.
 *                This is basically a version of fgets() that malloc()s
 *                it's own line buffer.  Note that the buffer will
 *                always be a multiple of BUFFER_SIZE bytes long.
 *                Therefore if you are going to keep the string for
 *                an extended period of time, you should probably
 *                strdup() it and free() the original, to save memory.
 *
 *
 * Parameters  :
 *          1  :  dest = destination for newly malloc'd pointer to
 *                line data.  Will be set to NULL on error.
 *          2  :  fp = File to read from
 *
 * Returns     :  JB_ERR_OK     on success
 *                JB_ERR_MEMORY on out-of-memory
 *                JB_ERR_FILE   on EOF.
 *
 *********************************************************************/
static int simple_read_line(char **dest, FILE *fp)
{
   int len;
   char * buf;
   char * newbuf;

   assert(fp);
   assert(dest);

   *dest = NULL;

   if (NULL == (buf = malloc(BUFFER_SIZE)))
   {
      return JB_ERR_MEMORY;
   }
   
   *buf = '\0';
   len = 0;

   while (FOREVER)
   {
      newbuf = buf + len;
      if ((!fgets(newbuf, BUFFER_SIZE, fp)) || (*newbuf == '\0'))
      {
         /* (*newbuf == '\0') should never happen unless fgets fails */
         if (*buf == '\0')
         {
            free(buf);
            return JB_ERR_FILE;
         }
         else
         {
            *dest = buf;
            return JB_ERR_OK;
         }
      }
      len = strlen(buf);
      if ((buf[len - 1] == '\n') || (buf[len - 1] == '\r'))
      {
         *dest = buf;
         return JB_ERR_OK;
      }
      
      if (NULL == (newbuf = realloc(buf, len + BUFFER_SIZE)))
      {
         free(buf);
         return JB_ERR_MEMORY;
      }
      buf = newbuf;
   }
}


/*********************************************************************
 *
 * Function    :  edit_read_line
 *
 * Description :  Read a single non-empty line from a file and return
 *                it.  Trims comments, leading and trailing whitespace
 *                and respects escaping of newline and comment char.
 *                Provides the line in 2 alternative forms: raw and
 *                preprocessed.
 *                - raw is the raw data read from the file.  If the 
 *                  line is not modified, then this should be written
 *                  to the new file.
 *                - prefix is any comments and blank lines that were
 *                  read from the file.  If the line is modified, then
 *                  this should be written out to the file followed
 *                  by the modified data.  (If this string is non-empty
 *                  then it will have a newline at the end).
 *                - data is the actual data that will be parsed
 *                  further by appropriate routines.
 *                On EOF, the 3 strings will all be set to NULL and
 *                0 will be returned.
 *
 * Parameters  :
 *          1  :  fp = File to read from
 *          2  :  raw_out = destination for newly malloc'd pointer to
 *                raw line data.  May be NULL if you don't want it.
 *          3  :  prefix_out = destination for newly malloc'd pointer to
 *                comments.  May be NULL if you don't want it.
 *          4  :  data_out = destination for newly malloc'd pointer to
 *                line data with comments and leading/trailing spaces
 *                removed, and line continuation performed.  May be
 *                NULL if you don't want it.
 *
 * Returns     :  JB_ERR_OK     on success
 *                JB_ERR_MEMORY on out-of-memory
 *                JB_ERR_FILE   on EOF.
 *
 *********************************************************************/
static int edit_read_line(FILE *fp, char **raw_out, char **prefix_out, char **data_out)
{
   char *p;          /* Temporary pointer   */
   char *linebuf;    /* Line read from file */
   char *linestart;  /* Start of linebuf, usually first non-whitespace char */
   char newline[3];  /* Used to store the newline - "\n", "\r", or "\r\n"   */
   int contflag = 0; /* Nonzero for line continuation - i.e. line ends '\'  */
   char *raw;        /* String to be stored in raw_out    */
   char *prefix;     /* String to be stored in prefix_out */
   char *data;       /* String to be stored in data_out   */
   int rval = JB_ERR_OK;

   assert(fp);

   /* Set output parameters to NULL */
   if (raw_out)
   {
      *raw_out    = NULL;
   }
   if (prefix_out)
   {
      *prefix_out = NULL;
   }
   if (data_out)
   {
      *data_out   = NULL;
   }

   /* Set string variables to new, empty strings. */

   raw    = malloc(1);
   prefix = malloc(1);
   data   = malloc(1);

   if ((raw == NULL) || (prefix == NULL) || (data == NULL))
   {
      freez(raw);
      freez(prefix);
      freez(data);
      return JB_ERR_MEMORY;
   }

   *raw    = '\0';
   *prefix = '\0';
   *data   = '\0';

   /* Main loop.  Loop while we need more data & it's not EOF. */

   while ( (contflag || (*data == '\0'))
        && (JB_ERR_OK == (rval = simple_read_line(&linebuf, fp))))
   {
      if (string_append(&raw,linebuf))
      {
         free(prefix);
         free(data);
         free(linebuf);
         return JB_ERR_MEMORY;
      }
      
      /* Trim off newline */
      p = linebuf + strlen(linebuf);
      if ((p != linebuf) && ((p[-1] == '\r') || (p[-1] == '\n')))
      {
         p--;
         if ((p != linebuf) && ((p[-1] == '\r') || (p[-1] == '\n')))
         {
            p--;
         }
      }
      strcpy(newline, p);
      *p = '\0';

      /* Line continuation? Trim escape and set flag. */
      contflag = ((p != linebuf) && (*--p == '\\'));
      if (contflag)
      {
         *p = '\0';
      }

      /* Trim leading spaces if we're at the start of the line */
      linestart = linebuf;
      if (*data == '\0')
      {
         /* Trim leading spaces */
         while (*linestart && isspace((int)(unsigned char)*linestart))
         {
            linestart++;
         }
      }

      /* Handle comment characters. */
      p = linestart;
      while ((p = strchr(p, '#')) != NULL)
      {
         /* Found a comment char.. */
         if ((p != linebuf) && (*(p-1) == '\\'))
         {
            /* ..and it's escaped, left-shift the line over the escape. */
            char *q = p - 1;
            while ((*q = *(q + 1)) != '\0')
            {
               q++;
            }
            /* Now scan from just after the "#". */
         }
         else
         {
            /* Real comment.  Save it... */
            if (p == linestart)
            {
               /* Special case:  Line only contains a comment, so all the
                * previous whitespace is considered part of the comment.
                * Undo the whitespace skipping, if any.
                */
               linestart = linebuf;
               p = linestart;
            }
            string_append(&prefix,p);
            if (string_append(&prefix,newline))
            {
               free(raw);
               free(data);
               free(linebuf);
               return JB_ERR_MEMORY;
            }
            *newline = '\0';

            /* ... and chop off the rest of the line */
            *p = '\0';
         }
      } /* END while (there's a # character) */

      /* Write to the buffer */
      if (*linestart)
      {
         if (string_append(&data, linestart))
         {
            free(raw);
            free(prefix);
            free(linebuf);
            return JB_ERR_MEMORY;
         }
      }

      free(linebuf);
   } /* END while(we need more data) */

   /* Handle simple_read_line() errors - ignore EOF */
   if ((rval != JB_ERR_OK) && (rval != JB_ERR_FILE))
   {
      free(raw);
      free(prefix);
      free(data);
      return rval;
   }


   if (*raw)
   {
      /* Got at least some data */

      /* Remove trailing whitespace */         
      chomp(data);

      if (raw_out)
      {
         *raw_out    = raw;
      }
      else
      {
         free(raw);
      }
      if (prefix_out)
      {
         *prefix_out = prefix;
      }
      else
      {
         free(prefix);
      }
      if (data_out)
      {
         *data_out   = data;
      }
      else
      {
         free(data);
      }
      return(0);
   }
   else
   {
      /* EOF and no data there. */

      free(raw);
      free(prefix);
      free(data);

      return JB_ERR_FILE;
   }
}


/*********************************************************************
 *
 * Function    :  edit_read_file
 *
 * Description :  Read a complete file into memory.  
 *                Handles whitespace, comments and line continuation.
 *
 * Parameters  :
 *          1  :  fp = File to read from
 *          2  :  pfile = Destination for a linked list of file_lines.
 *                        Will be set to NULL on error.
 *
 * Returns     :  JB_ERR_OK     on success
 *                JB_ERR_MEMORY on out-of-memory
 *
 *********************************************************************/
int edit_read_file(FILE *fp, struct file_line ** pfile)
{
   struct file_line * first_line; /* Keep for return value or to free */
   struct file_line * cur_line;   /* Current line */
   struct file_line * prev_line;  /* Entry with prev_line->next = cur_line */
   int rval;

   assert(fp);
   assert(pfile);

   *pfile = NULL;

   cur_line = first_line = zalloc(sizeof(struct file_line));
   if (cur_line == NULL)
   {
      return JB_ERR_MEMORY;
   }

   cur_line->type = FILE_LINE_UNPROCESSED;

   rval = edit_read_line(fp, &cur_line->raw, &cur_line->prefix, &cur_line->unprocessed);
   if (rval)
   {
      /* Out of memory or empty file. */
      /* Note that empty file is not an error we propogate up */
      free(cur_line);
      return ((rval == JB_ERR_FILE) ? JB_ERR_OK : rval);
   }

   do
   {
      prev_line = cur_line;
      cur_line = prev_line->next = zalloc(sizeof(struct file_line));
      if (cur_line == NULL)
      {
         /* Out of memory */
         edit_free_file(first_line);
         return JB_ERR_MEMORY;
      }

      cur_line->type = FILE_LINE_UNPROCESSED;

      rval = edit_read_line(fp, &cur_line->raw, &cur_line->prefix, &cur_line->unprocessed);
      if ((rval != JB_ERR_OK) && (rval != JB_ERR_FILE))
      {
         /* Out of memory */
         edit_free_file(first_line);
         return JB_ERR_MEMORY;
      }

   }
   while (rval != JB_ERR_FILE);

   /* EOF */

   /* We allocated one too many - free it */
   prev_line->next = NULL;
   free(cur_line);

   *pfile = first_line;
   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  edit_write_file
 *
 * Description :  Write a complete file to disk.
 *
 * Parameters  :
 *          1  :  filename = File to write to.
 *          2  :  file = Data structure to write.
 *
 * Returns     :  JB_ERR_OK     on success
 *                JB_ERR_FILE   on error writing to file.
 *
 *********************************************************************/
int edit_write_file(const char * filename, const struct file_line * file)
{
   FILE * fp;

   assert(filename);

   if (NULL == (fp = fopen(filename, "wt")))
   {
      return JB_ERR_FILE;
   }

   while (file != NULL)
   {
      if (file->raw)
      {
         if (fputs(file->raw, fp) < 0)
         {
            fclose(fp);
            return JB_ERR_FILE;
         }
      }
      else
      {
         if (file->prefix)
         {
            if (fputs(file->prefix, fp) < 0)
            {
               fclose(fp);
               return JB_ERR_FILE;
            }
         }
         if (file->unprocessed)
         {
            if (fputs(file->unprocessed, fp) < 0)
            {
               fclose(fp);
               return JB_ERR_FILE;
            }
            if (fputs("\n", fp) < 0)
            {
               fclose(fp);
               return JB_ERR_FILE;
            }
         }
         else
         {
            /* FIXME: Write data from file->data->whatever */
            assert(0);
         }
      }
      file = file->next;
   }

   fclose(fp);
   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  edit_free_file
 *
 * Description :  Free a complete file in memory.  
 *
 * Parameters  :
 *          1  :  file = Data structure to free.
 *
 * Returns     :  N/A
 *
 *********************************************************************/
void edit_free_file(struct file_line * file)
{
   struct file_line * next;

   while (file != NULL)
   {
      next = file->next;
      file->next = NULL;
      freez(file->raw);
      freez(file->prefix);
      freez(file->unprocessed);
      switch(file->type)
      {
         case 0: /* special case if memory zeroed */
         case FILE_LINE_UNPROCESSED:
         case FILE_LINE_BLANK:
         case FILE_LINE_ALIAS_HEADER:
         case FILE_LINE_SETTINGS_HEADER:
         case FILE_LINE_DESCRIPTION_HEADER:
         case FILE_LINE_DESCRIPTION_ENTRY:
         case FILE_LINE_ALIAS_ENTRY:
         case FILE_LINE_URL:
            /* No data is stored for these */
            break;

         case FILE_LINE_ACTION:
            free_action(file->data.action);
            break;

         case FILE_LINE_SETTINGS_ENTRY:
            freez(file->data.setting.name);
            freez(file->data.setting.svalue);
            break;
         default:
            /* Should never happen */
            assert(0);
            break;
      }
      file->type = 0; /* paranoia */
      free(file);
      file = next;
   }
}


/*********************************************************************
 *
 * Function    :  match_actions_file_header_line
 *
 * Description :  Match an actions file {{header}} line 
 *
 * Parameters  :
 *          1  :  line - String from file
 *          2  :  name - Header to match against
 *
 * Returns     :  0 iff they match.
 *
 *********************************************************************/
static int match_actions_file_header_line(const char * line, const char * name)
{
   int len;

   assert(line);
   assert(name);

   /* Look for "{{" */
   if ((line[0] != '{') || (line[1] != '{'))
   {
      return 1;
   }
   line += 2;

   /* Look for optional whitespace */
   while ( (*line == ' ') || (*line == '\t') )
   {
      line++;
   }

   /* Look for the specified name (case-insensitive) */
   len = strlen(name);
   if (0 != strncmpic(line, name, len))
   {
      return 1;
   }
   line += len;

   /* Look for optional whitespace */
   while ( (*line == ' ') || (*line == '\t') )
   {
      line++;
   }

   /* Look for "}}" and end of string*/
   if ((line[0] != '}') || (line[1] != '}') || (line[2] != '\0'))
   {
      return 1;
   }

   /* It matched!! */
   return 0;
}


/*********************************************************************
 *
 * Function    :  match_actions_file_header_line
 *
 * Description :  Match an actions file {{header}} line 
 *
 * Parameters  :
 *          1  :  line - String from file.  Must not start with
 *                       whitespace (else infinite loop!)
 *          2  :  name - Destination for name
 *          2  :  name - Destination for value
 *
 * Returns     :  JB_ERR_OK     on success
 *                JB_ERR_MEMORY on out-of-memory
 *                JB_ERR_PARSE  if there's no "=" sign, or if there's
 *                              nothing before the "=" sign (but empty
 *                              values *after* the "=" sign are legal).
 *
 *********************************************************************/
static int split_line_on_equals(const char * line, char ** pname, char ** pvalue)
{
   const char * name_end;
   const char * value_start;
   int name_len;

   assert(line);
   assert(pname);
   assert(pvalue);
   assert(*line != ' ');
   assert(*line != '\t');

   *pname = NULL;
   *pvalue = NULL;

   value_start = strchr(line, '=');
   if ((value_start == NULL) || (value_start == line))
   {
      return JB_ERR_PARSE;
   }

   name_end = value_start - 1;

   /* Eat any whitespace before the '=' */
   while ((*name_end == ' ') || (*name_end == '\t'))
   {
      /*
       * we already know we must have at least 1 non-ws char
       * at start of buf - no need to check
       */
      name_end--;
   }

   name_len = name_end - line + 1; /* Length excluding \0 */
   if (NULL == (*pname = (char *) malloc(name_len + 1)))
   {
      return JB_ERR_MEMORY;
   }
   strncpy(*pname, line, name_len);
   (*pname)[name_len] = '\0';

   /* Eat any the whitespace after the '=' */
   value_start++;
   while ((*value_start == ' ') || (*value_start == '\t'))
   {
      value_start++;
   }

   if (NULL == (*pvalue = strdup(value_start)))
   {
      free(*pname);
      *pname = NULL;
      return JB_ERR_MEMORY;
   }

   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  edit_parse_actions_file
 *
 * Description :  Parse an actions file in memory.  
 *
 *                Passed linked list must have the "data" member
 *                zeroed, and must contain valid "next" and
 *                "unprocessed" fields.  The "raw" and "prefix"
 *                fields are ignored, and "type" is just overwritten.
 *
 *                Note that on error the file may have been
 *                partially parsed.
 *
 * Parameters  :
 *          1  :  file = Actions file to be parsed in-place.
 *
 * Returns     :  JB_ERR_OK     on success
 *                JB_ERR_MEMORY on out-of-memory
 *                JB_ERR_PARSE  on error
 *
 *********************************************************************/
int edit_parse_actions_file(struct file_line * file)
{
   struct file_line * cur_line;
   int len;
   const char * text; /* Text from a line */
   char * name;  /* For lines of the form name=value */
   char * value; /* For lines of the form name=value */
   struct action_alias * alias_list = NULL;
   int rval = JB_ERR_OK;

   /* alias_list contains the aliases defined in this file.
    * It might be better to use the "file_line.data" fields
    * in the relavent places instead.
    */

   cur_line = file;

   /* A note about blank line support: Blank lines should only 
    * ever occur as the last line in the file.  This function
    * is more forgiving than that - FILE_LINE_BLANK can occur
    * anywhere.
    */

   /* Skip leading blanks.  Should only happen if file is
    * empty (which is valid, but pointless).
    */
   while ( (cur_line != NULL)
        && (cur_line->unprocessed[0] == '\0') )
   {
      /* Blank line */
      cur_line->type = FILE_LINE_BLANK;
      cur_line = cur_line->next;
   }

   if ( (cur_line != NULL)
     && (cur_line->unprocessed[0] != '{') )
   {
      /* File doesn't start with a header */
      return JB_ERR_PARSE;
   }

   if ( (cur_line != NULL) && (0 ==
      match_actions_file_header_line(cur_line->unprocessed, "settings") ) )
   {
      cur_line->type = FILE_LINE_SETTINGS_HEADER;

      cur_line = cur_line->next;
      while ((cur_line != NULL) && (cur_line->unprocessed[0] != '{'))
      {
         if (cur_line->unprocessed[0])
         {
            cur_line->type = FILE_LINE_SETTINGS_ENTRY;

            rval = split_line_on_equals(cur_line->unprocessed,
                     &cur_line->data.setting.name,
                     &cur_line->data.setting.svalue);
            if (rval != JB_ERR_OK)
            {
               /* Line does not contain a name=value pair, or out-of-memory */
               return rval;
            }
         }
         else
         {
            cur_line->type = FILE_LINE_BLANK;
         }
         cur_line = cur_line->next;
      }
   }

   if ( (cur_line != NULL) && (0 ==
      match_actions_file_header_line(cur_line->unprocessed, "description") ) )
   {
      cur_line->type = FILE_LINE_DESCRIPTION_HEADER;

      cur_line = cur_line->next;
      while ((cur_line != NULL) && (cur_line->unprocessed[0] != '{'))
      {
         if (cur_line->unprocessed[0])
         {
            cur_line->type = FILE_LINE_DESCRIPTION_ENTRY;
         }
         else
         {
            cur_line->type = FILE_LINE_BLANK;
         }
         cur_line = cur_line->next;
      }
   }

   if ( (cur_line != NULL) && (0 ==
      match_actions_file_header_line(cur_line->unprocessed, "alias") ) )
   {
      cur_line->type = FILE_LINE_ALIAS_HEADER;

      cur_line = cur_line->next;
      while ((cur_line != NULL) && (cur_line->unprocessed[0] != '{'))
      {
         if (cur_line->unprocessed[0])
         {
            /* define an alias */
            struct action_alias * new_alias;

            cur_line->type = FILE_LINE_ALIAS_ENTRY;

            rval = split_line_on_equals(cur_line->unprocessed, &name, &value);
            if (rval != JB_ERR_OK)
            {
               /* Line does not contain a name=value pair, or out-of-memory */
               return rval;
            }

            if ((new_alias = zalloc(sizeof(*new_alias))) == NULL)
            {
               /* Out of memory */
               free(name);
               free(value);
               free_alias_list(alias_list);
               return JB_ERR_MEMORY;
            }

            if (get_actions(value, alias_list, new_alias->action))
            {
               /* Invalid action or out of memory */
               free(name);
               free(value);
               free(new_alias);
               free_alias_list(alias_list);
               return JB_ERR_PARSE; /* FIXME: or JB_ERR_MEMORY */
            }

            free(value);

            new_alias->name = name;

            /* add to list */
            new_alias->next = alias_list;
            alias_list = new_alias;
         }
         else
         {
            cur_line->type = FILE_LINE_BLANK;
         }
         cur_line = cur_line->next;
      }
   }

   /* Header done, process the main part of the file */
   while (cur_line != NULL)
   {
      /* At this point, (cur_line->unprocessed[0] == '{') */
      assert(cur_line->unprocessed[0] == '{');
      text = cur_line->unprocessed + 1;
      len = strlen(text) - 1;
      if (text[len] != '}')
      {
         /* No closing } on header */
         free_alias_list(alias_list);
         return JB_ERR_PARSE;
      }

      if (text[0] == '{')
      {
         /* An invalid {{ header.  */
         free_alias_list(alias_list);
         return JB_ERR_PARSE;
      }

      while ( (*text == ' ') || (*text == '\t') )
      {
         text++;
         len--;
      }
      while ( (len > 0)
           && ( (text[len - 1] == ' ')
             || (text[len - 1] == '\t') ) )
      {
         len--;
      }
      if (len <= 0)
      {
         /* A line containing just { } */
         free_alias_list(alias_list);
         return JB_ERR_PARSE;
      }

      cur_line->type = FILE_LINE_ACTION;

      /* Remove {} and make copy */
      if (NULL == (value = (char *) malloc(len + 1)))
      {
         /* Out of memory */
         free_alias_list(alias_list);
         return JB_ERR_MEMORY;
      }
      strncpy(value, text, len);
      value[len] = '\0';

      /* Get actions */
      if (get_actions(value, alias_list, cur_line->data.action))
      {
         /* Invalid action or out of memory */
         free(value);
         free_alias_list(alias_list);
         return JB_ERR_PARSE; /* FIXME: or JB_ERR_MEMORY */
      }

      /* Done with string - it was clobbered anyway */
      free(value);

      /* Process next line */
      cur_line = cur_line->next;

      /* Loop processing URL patterns */
      while ((cur_line != NULL) && (cur_line->unprocessed[0] != '{'))
      {
         if (cur_line->unprocessed[0])
         {
            /* Could parse URL here, but this isn't currently needed */

            cur_line->type = FILE_LINE_URL;
         }
         else
         {
            cur_line->type = FILE_LINE_BLANK;
         }
         cur_line = cur_line->next;
      }
   } /* End main while(cur_line != NULL) loop */

   free_alias_list(alias_list);

   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  edit_read_file
 *
 * Description :  Read a complete actions file into memory and
 *                parses it.
 *
 * Parameters  :
 *          1  :  filename = Path to file to read from
 *          2  :  pfile = Destination for a linked list of file_lines.
 *                        Will be set to NULL on error.
 *
 * Returns     :  JB_ERR_OK     on success
 *                JB_ERR_MEMORY on out-of-memory
 *                JB_ERR_FILE   if the file cannot be opened or
 *                              contains no data
 *
 *********************************************************************/
int edit_read_actions_file(const char * filename, struct file_line ** pfile)
{
   struct file_line * file;
   FILE * fp;
   int rval;

   assert(filename);
   assert(pfile);

   *pfile = NULL;

   if (NULL == (fp = fopen(filename,"rt")))
   {
      return JB_ERR_FILE;
   }

   rval = edit_read_file(fp, &file);

   fclose(fp);

   if (JB_ERR_OK != rval)
   {
      return rval;
   }

   if (JB_ERR_OK != (rval = edit_parse_actions_file(file)))
   {
      edit_free_file(file);
      return rval;
   }

   *pfile = file;
   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  get_file_name_param
 *
 * Description :  Get the name of the file to edit from the parameters
 *                passed to a CGI function.  This function handles
 *                security checks such as blocking urls containing
 *                "/" or ".", prepending the config file directory,
 *                and adding the specified suffix.
 *
 *                (This is an essential security check, otherwise
 *                users may be able to pass "../../../etc/passwd"
 *                and overwrite the password file [linux], "prn:"
 *                and print random data [Windows], etc...)
 *
 *                This function only allows filenames contining the
 *                characters '-', '_', 'A'-'Z', 'a'-'z', and '0'-'9'.
 *                That's probably too restrictive but at least it's
 *                secure.
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  parameters = map of cgi parameters
 *           3 :  suffix = File extension, e.g. ".actions"
 *           4 :  pfilename = destination for full filename.  Caller
 *                free()s.  Set to NULL on error.
 *           5 :  pparam = destination for partial filename,
 *                suitable for use in another URL.  Allocated as part
 *                of the map "parameters", so don't free it.
 *                Set to NULL if not specified.
 *
 * CGI Parameters : None
 *
 * Returns     :  JB_ERR_OK         on success
 *                JB_ERR_MEMORY     on out-of-memory
 *                JB_ERR_CGI_PARAMS if "filename" was not specified
 *                                  or is not valid.
 *
 *********************************************************************/
static int get_file_name_param(struct client_state *csp,
   		                      struct map *parameters,
                               char *suffix,
                               char **pfilename,
                               const char **pparam)
{
   const char *param;
   const char *s;
   char *name;
   char *fullpath;
   char ch;
   int len;

   assert(csp);
   assert(parameters);
   assert(suffix);
   assert(pfilename);
   assert(pparam);

   *pfilename = NULL;
   *pparam = NULL;

   param = lookup(parameters, "filename");
   if (!*param)
   {
      return JB_ERR_CGI_PARAMS;
   }

   *pparam = param;

   len = strlen(param);
   if (len >= FILENAME_MAX)
   {
      /* Too long. */
      return JB_ERR_CGI_PARAMS;
   }

   /* Check every character to see if it's legal */
   s = param;
   while ((ch = *s++) != '\0')
   {
      if ( ((ch < 'A') || (ch > 'Z'))
        && ((ch < 'a') || (ch > 'z'))
        && ((ch < '0') || (ch > '9'))
        && (ch != '-')
        && (ch != '_') )
      {
         /* Probable hack attempt. */
         return JB_ERR_CGI_PARAMS;
      }
   }

   /* Append extension */
   name = malloc(len + strlen(suffix) + 1);
   if (name == NULL)
   {
      return JB_ERR_MEMORY;
   }
   strcpy(name, param);
   strcpy(name + len, suffix);

   /* Prepend path */
   fullpath = make_path(csp->config->confdir, name);
   free(name);
   if (fullpath == NULL)
   {
      return JB_ERR_MEMORY;
   }

   /* Success */
   *pfilename = fullpath;

   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  get_number_param
 *
 * Description :  Get a non-negative integer from the parameters
 *                passed to a CGI function.
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  parameters = map of cgi parameters
 *           3 :  name = Name of CGI parameter to read
 *           4 :  pvalue = destination for value.
 *                         Set to -1 on error.
 *
 * CGI Parameters : None
 *
 * Returns     :  JB_ERR_OK         on success
 *                JB_ERR_MEMORY     on out-of-memory
 *                JB_ERR_CGI_PARAMS if "filename" was not specified
 *                                  or is not valid.
 *
 *********************************************************************/
static int get_number_param(struct client_state *csp,
   		                   struct map *parameters,
                            char *name,
                            int *pvalue)
{
   const char *param;
   char ch;
   int value;

   assert(csp);
   assert(parameters);
   assert(name);
   assert(pvalue);

   *pvalue = -1;

   param = lookup(parameters, name);
   if (!*param)
   {
      return JB_ERR_CGI_PARAMS;
   }

   /* We don't use atoi because I want to check this carefully... */

   value = 0;
   while ((ch = *param++) != '\0')
   {
      if ((ch < '0') || (ch > '9'))
      {
         return JB_ERR_CGI_PARAMS;
      }

      ch -= '0';

      /* Note:
       *
       * <limits.h> defines INT_MAX
       *
       * (INT_MAX - ch) / 10 is the largest number that 
       *     can be safely multiplied by 10 then have ch added.
       */
      if (value > ((INT_MAX - ch) / 10))
      {
         return JB_ERR_CGI_PARAMS;
      }

      value = value * 10 + ch;
   }

   /* Success */
   *pvalue = value;

   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  cgi_edit_actions
 *
 * Description :  CGI function that allows the user to choose which
 *                actions file to edit.
 *
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  0 on success, nonzero on error
 *
 *********************************************************************/
int cgi_edit_actions(struct client_state *csp,
                     struct http_response *rsp,
   		            struct map *parameters)
{

   /* FIXME: Incomplete */
   rsp->status = strdup("302 Local Redirect from Junkbuster");
   enlist_unique_header(rsp->headers, "Location", "http://ijbswa.sourceforge.net/config/edit-actions-list?filename=edit");

   return 0;
}


/*********************************************************************
 *
 * Function    :  cgi_edit_actions_list
 *
 * Description :  CGI function that edits the actions list.
 *                FIXME: This function shouldn't FATAL ever.
 *                FIXME: This function doesn't check the retval of map()
 * Parameters  :
 *           1 :  csp = Current client state (buffers, headers, etc...)
 *           2 :  rsp = http_response data structure for output
 *           3 :  parameters = map of cgi parameters
 *
 * CGI Parameters : None
 *
 * Returns     :  0 on success, 1 on error.
 *
 *********************************************************************/
int cgi_edit_actions_list(struct client_state *csp, struct http_response *rsp,
   		                 struct map *parameters)
{
   char * section_template;
   char * url_template;
   char * sections;
   char * urls;
   char buf[50];
   char * s;
   struct map * exports;
   struct map * section_exports;
   struct map * url_exports;
   struct file_line * file;
   struct file_line * cur_line;
   int line_number = 0;
   int url_1_2;
   int rval;
   char * filename;
   char * filename_param;

   rval = get_file_name_param(csp, parameters, ".action", &filename, &filename_param);
   if (rval)
   {
      /* No filename specified. */
      /* FIXME: Shouldn't FATAL here */
      log_error(LOG_LEVEL_FATAL, "No filename specified");
      return 1;
   }

   if (edit_read_actions_file(filename, &file))
   {
      /* FIXME: Shouldn't FATAL here */
      log_error(LOG_LEVEL_FATAL, "Cannot load file '%s' for editing", filename);
      return 1;
   }

   free(filename);

   if (NULL == (exports = default_exports(csp, NULL)))
   {
      log_error(LOG_LEVEL_FATAL, "Out of memory in cgi_edit_actions_list");
      return 1;
   }

   map(exports, "filename", 1, filename_param, 1);

   /* Should do all global exports above this point */

   if (NULL == (section_template = template_load(csp, "edit-actions-list-section")))
   {
      log_error(LOG_LEVEL_FATAL, "Out of memory in cgi_edit_actions_list");
      return 1;
   }
   if (NULL == (url_template = template_load(csp, "edit-actions-list-url")))
   {
      log_error(LOG_LEVEL_FATAL, "Out of memory in cgi_edit_actions_list");
      return 1;
   }

   template_fill(&section_template, exports);
   template_fill(&url_template, exports);

   /* Find start of actions in file */
   cur_line = file;
   line_number = 1;
   while ((cur_line != NULL) && (cur_line->type != FILE_LINE_ACTION))
   {
      cur_line = cur_line->next;
      line_number++;
   }

   if (NULL == (sections = strdup("")))
   {
      log_error(LOG_LEVEL_FATAL, "Out of memory in cgi_edit_actions_list");
      return 1;
   }

   while ((cur_line != NULL) && (cur_line->type == FILE_LINE_ACTION))
   {
      if (NULL == (section_exports = new_map()))
      {
         log_error(LOG_LEVEL_FATAL, "Out of memory in cgi_edit_actions_list");
         return 1;
      }

      snprintf(buf, 50, "%d", line_number);
      map(section_exports, "sectionid", 1, buf, 1);

      if (NULL == (s = actions_to_html(cur_line->data.action)))
      {
         log_error(LOG_LEVEL_FATAL, "Out of memory in cgi_edit_actions_list");
         return 1;
      }
      map(section_exports, "actions", 1, s, 0);

      /* Should do all section-specific exports above this point */

      if (NULL == (urls = strdup("")))
      {
         log_error(LOG_LEVEL_FATAL, "Out of memory in cgi_edit_actions_list");
         return 1;
      }

      url_1_2 = 2;

      cur_line = cur_line->next;
      line_number++;

      while ((cur_line != NULL) && (cur_line->type == FILE_LINE_URL))
      {
         if (NULL == (url_exports = new_map()))
         {
            log_error(LOG_LEVEL_FATAL, "Out of memory in cgi_edit_actions_list");
            return 1;
         }

         snprintf(buf, 50, "%d", line_number);
         map(url_exports, "urlid", 1, buf, 1);

         snprintf(buf, 50, "%d", url_1_2);
         map(url_exports, "url-1-2", 1, buf, 1);

         if (NULL == (s = html_encode(cur_line->unprocessed)))
         {
            log_error(LOG_LEVEL_FATAL, "Out of memory in cgi_edit_actions_list");
            return 1;
         }

         map(url_exports, "url", 1, s, 0);

         if (NULL == (s = strdup(url_template)))
         {
            log_error(LOG_LEVEL_FATAL, "Out of memory in cgi_edit_actions_list");
            return 1;
         }
         template_fill(&s, section_exports);
         template_fill(&s, url_exports);
         urls = strsav(urls, s);
         free_map(url_exports);

         url_1_2 = 3 - url_1_2;

         cur_line = cur_line->next;
         line_number++;
      }

      map(section_exports, "urls", 1, urls, 0);

      /* Could also do section-specific exports here, but it wouldn't be as fast */

      s = strdup(section_template);
      template_fill(&s, section_exports);
      sections = strsav(sections, s);
      free_map(section_exports);
   }

   edit_free_file(file);

   map(exports, "sections", 1, sections, 0);

   /* Could also do global exports here, but it wouldn't be as fast */

   rsp->body = template_load(csp, "edit-actions-list");
   template_fill(&rsp->body, exports);
   free_map(exports);

   return 0;
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
 * Returns     :  JB_ERR_OK     on success
 *                JB_ERR_MEMORY on out-of-memory
 *
 *********************************************************************/
static int map_radio(struct map * exports,
                     const char * optionname, 
                     const char * values,
                     char value)
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
      return JB_ERR_MEMORY;
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
            return JB_ERR_MEMORY;
         }
      }
   }

   *p = value;
   if (map(exports, buf, 0, "checked", 1))
   {
      free(buf);
      return JB_ERR_MEMORY;
   }

   return JB_ERR_OK;
}


/*********************************************************************
 *
 * Function    :  actions_to_radio
 *
 * Description :  Converts a actionsfile entry into settings for
 *                radio buttons and edit boxes on a HTML form.
 *
 * Parameters  :
 *          1  :  exports = List of substitutions to add to.
 *          2  :  action  = Action to read
 *
 * Returns     :  JB_ERR_OK     on success
 *                JB_ERR_MEMORY on out-of-memory
 *
 *********************************************************************/
static int actions_to_radio(struct map * exports, const struct action_spec *action)
{
   unsigned mask = action->mask;
   unsigned add  = action->add;
   int mapped_param;
   int checked;
   char current_mode;

   assert(exports);
   assert(action);

   mask = action->mask;
   add  = action->add;

   /* sanity - prevents "-feature +feature" */
   mask |= add;


#define DEFINE_ACTION_BOOL(name, bit)                 \
   if (!(mask & bit))                                 \
   {                                                  \
      current_mode = 'n';                             \
   }                                                  \
   else if (add & bit)                                \
   {                                                  \
      current_mode = 'y';                             \
   }                                                  \
   else                                               \
   {                                                  \
      current_mode = 'x';                             \
   }                                                  \
   if (map_radio(exports, name, "ynx", current_mode)) \
   {                                                  \
      return JB_ERR_MEMORY;                           \
   }

#define DEFINE_ACTION_STRING(name, bit, index)        \
   DEFINE_ACTION_BOOL(name, bit);                     \
   mapped_param = 0;

#define DEFINE_CGI_PARAM_RADIO(name, bit, index, value, is_default)  \
   if (add & bit)                                                    \
   {                                                                 \
      checked = !strcmp(action->string[index], value);               \
   }                                                                 \
   else                                                              \
   {                                                                 \
      checked = is_default;                                          \
   }                                                                 \
   mapped_param |= checked;                                          \
   if (map(exports, name "-param-" value, 1, (checked ? "checked" : ""), 1)) \
   {                                                                 \
      return JB_ERR_MEMORY;                                          \
   }

#define DEFINE_CGI_PARAM_CUSTOM(name, bit, index, default_val)       \
   if (map(exports, name "-param-custom", 1,                         \
           ((!mapped_param) ? "checked" : ""), 1))                   \
   {                                                                 \
      return JB_ERR_MEMORY;                                          \
   }                                                                 \
   if (map(exports, name "-param", 1,                                \
           (((add & bit) && !mapped_param) ?                         \
           action->string[index] : default_val), 1))                 \
   {                                                                 \
      return JB_ERR_MEMORY;                                          \
   }

#define DEFINE_CGI_PARAM_NO_RADIO(name, bit, index, default_val)     \
   if (map(exports, name "-param", 1,                                \
           ((add & bit) ? action->string[index] : default_val), 1))  \
   {                                                                 \
      return JB_ERR_MEMORY;                                          \
   }

#define DEFINE_ACTION_MULTI(name, index)              \
   if (action->multi_add[index]->first)               \
   {                                                  \
      current_mode = 'y';                             \
   }                                                  \
   else if (action->multi_remove_all[index])          \
   {                                                  \
      current_mode = 'n';                             \
   }                                                  \
   else if (action->multi_remove[index]->first)       \
   {                                                  \
      current_mode = 'y';                             \
   }                                                  \
   else                                               \
   {                                                  \
      current_mode = 'x';                             \
   }                                                  \
   if (map_radio(exports, name, "ynx", current_mode)) \
   {                                                  \
      return JB_ERR_MEMORY;                           \
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

   return JB_ERR_OK;
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
 * Returns     :  0 on success, nonzero on error
 *
 *********************************************************************/
int cgi_edit_actions_for_url(struct client_state *csp,
                             struct http_response *rsp,
   		                    struct map *parameters)
{
   struct map * exports;
   int sectionid;

   struct file_line * file;
   struct file_line * cur_line;
   int line_number;
   int rval;
   char * filename;
   char * filename_param;

   rval = get_file_name_param(csp, parameters, ".action", &filename, &filename_param);
   if (rval)
   {
      /* No filename specified. */
      /* FIXME: Shouldn't FATAL here */
      log_error(LOG_LEVEL_FATAL, "No filename specified");
      return 1;
   }

   if (get_number_param(csp, parameters, "section", &sectionid))
   {
      /* FIXME: Shouldn't FATAL here */
      log_error(LOG_LEVEL_FATAL, "No 'section' parameter");
      return cgi_default(csp, rsp, parameters);
   }

   if (edit_read_actions_file(filename, &file))
   {
      /* FIXME: Shouldn't FATAL here */
      log_error(LOG_LEVEL_FATAL, "Cannot load file '%s' for editing", filename);
      return 1;
   }
   cur_line = file;

   for (line_number = 1; (cur_line != NULL) && (line_number < sectionid); line_number++)
   {
      cur_line = cur_line->next;
   }

   if ( (cur_line == NULL)
     || (line_number != sectionid)
     || (sectionid < 1)
     || (cur_line->type != FILE_LINE_ACTION))
   {
      /* FIXME: Shouldn't FATAL here */
      log_error(LOG_LEVEL_FATAL, "Bad sectionid!");
      return 1;
   }

   exports = default_exports(csp, NULL);
   map(exports, "filename", 1, filename_param, 1);
   map(exports, "section", 1, lookup(parameters, "section"), 1);

   actions_to_radio(exports, cur_line->data.action);

   edit_free_file(file);

   rsp->body = template_load(csp, "edit-actions-for-url");
   template_fill(&rsp->body, exports);
   free_map(exports);

   return 0;
}


/*********************************************************************
 *
 * Function    :  actions_from_radio
 *
 * Description :  Converts a map of parameters passed to a CGI function
 *                into an actionsfile entry.
 *
 * Parameters  :
 *          1  :  parameters = parameters to the CGI call
 *          2  :  action  = Action to change.  Must be valid before
 *                          the call, actions not specified will be
 *                          left unchanged.
 *
 * Returns     :  JB_ERR_OK     on success
 *                JB_ERR_MEMORY on out-of-memory
 *
 *********************************************************************/
static int actions_from_radio(const struct map * parameters,
                              struct action_spec *action)
{
   const char * param;
   char * param_dup;
   char ch;

   assert(parameters);
   assert(action);

#define DEFINE_ACTION_BOOL(name, bit)                 \
   if (NULL != (param = lookup(parameters, name)))    \
   {                                                  \
      ch = toupper((int)param[0]);                    \
      if (ch == 'Y')                                  \
      {                                               \
         action->add  |= bit;                         \
         action->mask |= bit;                         \
      }                                               \
      else if (ch == 'N')                             \
      {                                               \
         action->add  &= ~bit;                        \
         action->mask &= ~bit;                        \
      }                                               \
      else if (ch == 'X')                             \
      {                                               \
         action->add  &= ~bit;                        \
         action->mask |= bit;                         \
      }                                               \
   }

#define DEFINE_ACTION_STRING(name, bit, index)                    \
   if (NULL != (param = lookup(parameters, name)))                \
   {                                                              \
      ch = toupper((int)param[0]);                                \
      if (ch == 'Y')                                              \
      {                                                           \
         param = lookup(parameters, name "-mode");                \
         if ((*param == '\0') || (0 == strcmp(param, "CUSTOM")))  \
         {                                                        \
            param = lookup(parameters, name "-param");            \
         }                                                        \
         if (*param != '\0')                                      \
         {                                                        \
            if (NULL == (param_dup = strdup(param)))              \
            {                                                     \
               return JB_ERR_MEMORY;                              \
            }                                                     \
            freez(action->string[index]);                         \
            action->add  |= bit;                                  \
            action->mask |= bit;                                  \
            action->string[index] = param_dup;                    \
         }                                                        \
      }                                                           \
      else if (ch == 'N')                                         \
      {                                                           \
         if (action->add & bit)                                   \
         {                                                        \
            freez(action->string[index]);                         \
         }                                                        \
         action->add  &= ~bit;                                    \
         action->mask &= ~bit;                                    \
      }                                                           \
      else if (ch == 'X')                                         \
      {                                                           \
         if (action->add & bit)                                   \
         {                                                        \
            freez(action->string[index]);                         \
         }                                                        \
         action->add  &= ~bit;                                    \
         action->mask |= bit;                                     \
      }                                                           \
   }

#define DEFINE_ACTION_MULTI(name, index)                          \
   if (NULL != (param = lookup(parameters, name)))                \
   {                                                              \
      ch = toupper((int)param[0]);                                \
      if (ch == 'Y')                                              \
      {                                                           \
         /* FIXME */                                              \
      }                                                           \
      else if (ch == 'N')                                         \
      {                                                           \
         list_remove_all(action->multi_add[index]);               \
         list_remove_all(action->multi_remove[index]);            \
         action->multi_remove_all[index] = 1;                     \
      }                                                           \
      else if (ch == 'X')                                         \
      {                                                           \
         list_remove_all(action->multi_add[index]);               \
         list_remove_all(action->multi_remove[index]);            \
         action->multi_remove_all[index] = 0;                     \
      }                                                           \
   }

#define DEFINE_CGI_PARAM_CUSTOM(name, bit, index, default_val)
#define DEFINE_CGI_PARAM_RADIO(name, bit, index, value, is_default)
#define DEFINE_CGI_PARAM_NO_RADIO(name, bit, index, default_val)

#define DEFINE_ACTION_ALIAS 0 /* No aliases for URL parsing */

#include "actionlist.h"

#undef DEFINE_ACTION_MULTI
#undef DEFINE_ACTION_STRING
#undef DEFINE_ACTION_BOOL
#undef DEFINE_ACTION_ALIAS
#undef DEFINE_CGI_PARAM_CUSTOM
#undef DEFINE_CGI_PARAM_RADIO
#undef DEFINE_CGI_PARAM_NO_RADIO

   return JB_ERR_OK;
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
 * Returns     :  0 on success, nonzero on error
 *
 *********************************************************************/
int cgi_edit_actions_submit(struct client_state *csp, struct http_response *rsp,
   		            struct map *parameters)
{
/*
   struct map * exports;
*/
   int sectionid;
   char * actiontext;
   char * newtext;
   int len;

   struct file_line * file;
   struct file_line * cur_line;
   int line_number;
   int rval;
   char * filename;
   const char * filename_param;
   char * target;

   rval = get_file_name_param(csp, parameters, ".action", &filename, &filename_param);
   if (rval)
   {
      /* No filename specified. */
      /* FIXME: Shouldn't FATAL here */
      log_error(LOG_LEVEL_FATAL, "No filename specified");
      return 1;
   }

   if (get_number_param(csp, parameters, "section", &sectionid))
   {
      /* FIXME: Shouldn't FATAL here */
      log_error(LOG_LEVEL_FATAL, "No 'section' parameter");
      return cgi_default(csp, rsp, parameters);
   }

   if (edit_read_actions_file(filename, &file))
   {
      /* FIXME: Shouldn't FATAL here */
      log_error(LOG_LEVEL_FATAL, "Cannot load file '%s' for editing", filename);
      return 1;
   }
   cur_line = file;

   for (line_number = 1; (cur_line != NULL) && (line_number < sectionid); line_number++)
   {
      cur_line = cur_line->next;
   }

   if ( (cur_line == NULL)
     || (line_number != sectionid)
     || (sectionid < 1)
     || (cur_line->type != FILE_LINE_ACTION))
   {
      edit_free_file(file);
      return cgi_default(csp, rsp, parameters);
   }

   if (actions_from_radio(parameters, cur_line->data.action))
   {
      /* Out of memory */
      edit_free_file(file);
      return 1;
   }

   if (NULL == (actiontext = actions_to_text(cur_line->data.action)))
   {
      /* Out of memory */
      edit_free_file(file);
      return 1;
   }

   len = strlen(actiontext);
   if (NULL == (newtext = malloc(len + 2)))
   {
      /* Out of memory */
      free(actiontext);
      edit_free_file(file);
      return 1;
   }
   strcpy(newtext, actiontext);
   free(actiontext);
   newtext[0]       = '{';
   newtext[len]     = '}';
   newtext[len + 1] = '\0';

   freez(cur_line->raw);
   freez(cur_line->unprocessed);
   cur_line->unprocessed = newtext;

   if (edit_write_file(filename, file))
   {
      /* Error writing file */
      edit_free_file(file);
      /* FIXME: Shouldn't FATAL here */
      log_error(LOG_LEVEL_FATAL, "Cannot save file '%s' after editing", filename);
      return 1;
   }

   edit_free_file(file);
   free(filename);


   target = strdup("http://ijbswa.sourceforge.net/config/edit-actions-list?filename=");
   string_append(&target, filename_param);

   if (target == NULL)
   {
      /* Out of memory */
      /* FIXME: Shouldn't FATAL here */
      log_error(LOG_LEVEL_FATAL, "Out of memory");
      return 1;
   }

   rsp->status = strdup("302 Local Redirect from Junkbuster");
   enlist_unique_header(rsp->headers, "Location", target);
   free(target);

   return 0;
   /* return cgi_edit_actions_list(csp, rsp, parameters); */
}


#endif /* def FEATURE_CGI_EDIT_ACTIONS */


/*
  Local Variables:
  tab-width: 3
  end:
*/
