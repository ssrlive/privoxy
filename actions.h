#ifndef ACTIONS_H_INCLUDED
#define ACTIONS_H_INCLUDED
#define ACTIONS_H_VERSION "$Id: actions.h,v 1.4 2001/09/16 15:47:37 jongfoster Exp $"
/*********************************************************************
 *
 * File        :  $Source: /cvsroot/ijbswa/current/actions.h,v $
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
 *    $Log: actions.h,v $
 *    Revision 1.4  2001/09/16 15:47:37  jongfoster
 *    First version of CGI-based edit interface.  This is very much a
 *    work-in-progress, and you can't actually use it to edit anything
 *    yet.  You must #define FEATURE_CGI_EDIT_ACTIONS for these changes
 *    to have any effect.
 *
 *    Revision 1.3  2001/09/14 00:17:32  jongfoster
 *    Tidying up memory allocation. New function init_action().
 *
 *    Revision 1.2  2001/07/29 19:01:11  jongfoster
 *    Changed _FILENAME_H to FILENAME_H_INCLUDED.
 *    Added forward declarations for needed structures.
 *
 *    Revision 1.1  2001/05/31 21:16:46  jongfoster
 *    Moved functions to process the action list into this new file.
 *
 *
 *********************************************************************/


#ifdef __cplusplus
extern "C" {
#endif


struct action_spec;
struct current_action_spec;
struct client_state;



/* This structure is used to hold user-defined aliases */
struct action_alias
{
   const char * name;
   struct action_spec action[1];
   struct action_alias * next;
};


extern int get_actions (char *line, 
                        struct action_alias * alias_list,
                        struct action_spec *cur_action);
extern void free_alias_list(struct action_alias *alias_list);

extern void init_action(struct action_spec *dest);
extern void free_action(struct action_spec *src);
extern void merge_actions (struct action_spec *dest, 
                           const struct action_spec *src);
extern void copy_action (struct action_spec *dest, 
                         const struct action_spec *src);
extern char * actions_to_text     (struct action_spec *action);
#ifdef FEATURE_CGI_EDIT_ACTIONS
extern char * actions_to_html     (struct action_spec *action);
#endif /* def FEATURE_CGI_EDIT_ACTIONS */

extern void init_current_action     (struct current_action_spec *dest);
extern void free_current_action     (struct current_action_spec *src);
extern void merge_current_action    (struct current_action_spec *dest, 
                                     const struct action_spec *src);
extern char * current_action_to_text(struct current_action_spec *action);

extern int get_action_token(char **line, char **name, char **value);
extern void unload_actions_file(void *file_data);
extern int load_actions_file(struct client_state *csp);



/* Revision control strings from this header and associated .c file */
extern const char actions_rcs[];
extern const char actions_h_rcs[];

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ndef ACTIONS_H_INCLUDED */

/*
  Local Variables:
  tab-width: 3
  end:
*/

