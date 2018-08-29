/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* 
 */

#ifndef _FILELIST_H
#define _FILELIST_H

/*************************************************************************
*									
*  Charly Gatot
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA	94538
*									
*  Short description of functions in this file:
*
*  filelist_win_create
*	create a file-browser window. (Only called once)
*  filelist_win_show
*	pop-up the file-browser window
*  filelist_notify_func
*	register callback functions
*  filelist_update
*	update files listing
*
*  filelist_menu_pullright (Not related with file-browser)
*	generate a pull-right menu for all files
*	
*************************************************************************/
#include <xview/frame.h>

/* ====================== filelist.c ====================== */

/* To show the filelist for loading or saving*/
typedef enum
{
   FILELIST_LOAD,
   FILELIST_LOAD_ALL,
   FILELIST_SAVE
} Flist_state;

/* To create or change filelist callback functions */
typedef enum
{
   FILELIST_NEW,
   FILELIST_CHANGE,
   FILELIST_DELETE
} Flist_type;

/************************************************************************
*                                                                       *
*  Create file list handlers. This window will provide a file browser	*
*  to browse through files in a directory.  It also provides a callback *
*  procedure to load/save a file.					*
*  Return OK or NOT_OK.                                                 *
*                                                                       */
extern int
filelist_win_create(Frame owner,
	int xpos, int ypos,	/* Position of the popup window (in pixel)*/
	int filename_wd,	/* the maximum width of filename (in pixel)*/
	int filelist_num);	/* maximum number of viewable files */

/************************************************************************
*                                                                       *
*  Pop up the file list frame.                                          *
*  If status = FILELIST_LOAD,  the function 'addr_loadfunc' of		*
*  filelist_notify_func will be called,  if status = FILELIST_SAVE,	*
*  the function 'addr_savefunc' will be called, if status =		*
*  FILELIST_LOAD_ALL the function 'addr_loadallfunc will be called.	*
*  The "id" is used to indicate which caller program wants to use the	*
*  file browser.							*
*                                                                       */
extern void   
filelist_win_show(
	long id,		/* ID of the caller */
	Flist_state status,	/* FILELIST_LOAD, FILELIST_LOAD_ALL, */
				/*  or FILELIST_SAVE */
	char *dirpath,		/* Specific the destination directory */
				/*  If NULL, use default directory */
	char *title,		/* Specify the title window label */
				/*  If NULL, label "File-Browser" */
	char *button_name=NULL);/* Specify the button name, if NULL */
				/* default name depends on 'status' */

/************************************************************************
*                                                                       *
*  User register function to call when the "Load", "Load All" or        *
*  "Save" buton is selected.                                            *
*  Note that all callback functions will take the form of		*
*       addr_func(char *dir_path,  char *selected_filename)		*
*                                                                       */
extern void
filelist_notify_func(
        long id,		/* ID to identify from the caller */
	Flist_type ftype,	/* Create/Change/Delete callback funcs */
   	long addr_loadfunc,     /* Address function to call for loading */
   	long addr_savefunc,	/* Address function to call for saving */
   	long addr_loadallfunc=NULL);	/* Address function to call for */
					/* loading all files*/

/************************************************************************
*                                                                       *
*  Set the default directory to use for a given FileBrowser if
*  filelist_win_show() is called with a NULL directory pointer.
*									*/
extern void
filelist_set_directory(long id,		/* ID of FileBrowser to change */
		       char *dirname);	/* Directory pathname */

/************************************************************************
*                                                                       *
*  Update the listing files.  This function is called when the user has	*
*  added or deleted a file from the current directory.			*
*									*/
extern void
filelist_update(void);

/************************************************************************
*                                                                       *
*  Get the name of the first file in the current file list.
*  Returns a pointer to the file name, or 0 if there is no file.
*									*/
extern char *
filelist_first_file(void);

/************************************************************************
*                                                                       *
*  Get the name of the next file in the current file list.
*  Must call "filelist_first_file()" before calling this one.
*  Returns a pointer to the file name, or 0 if there are no more files.
*									*/
extern char *
filelist_next_file(void);

/************************************************************************
*                                                                       *
*  Put up a warning to confirm overwriting of file.
*  "filename" is the name of the file that will be overwritten.
*  Returns TRUE if answer is "Yes", otherwise returns FALSE.
*									*/
extern int
confirm_overwrite(char *filename);


/* ====================== menu_dir.c ====================== */


/************************************************************************
*                                                                       *
*  Create a menu for all files or directories below it.  If it is a 	*
*  directory, it reculsively creates a pullright-menu.  It only creates	*
*  when it is necessary.						*
*  Return Menu pointing to the pullright created menu.			*
* 									*
*  This routine is typically used along together with MENU_GEN_PULLRIGHT*
*  For example,								*
* 	The user has registerred a callback routine "gen_func" for	*
*	MENU_GEN_PULLRIGHT.  Then routine "gen_func" might call		*
*	filelist_menu_pullright as shown below:				*
*									*
*	Menu gen_func(Menu_item mi, Menu_generate op)			*
*	{								*
*	   if (op == MENU_DISPLAY)					*
*	   {								*
*	      ....							*
*      	      return(filelist_menu_pullright(mi, op, addr_func, path));	*
*	   }								*
*	   else								*
*	   {								*
*	      ....							*
*      	      return(filelist_menu_pullright(mi, op, NULL, NULL));	*
*	   }								*
*	}								*
*       								*
*	The "addr_func" and "path" are only used when it is about to	*
*       display (create) a menu.					*
*									*
*  Note that the callback function will take the form of		*
*       addr_func(char *dir_path,  char *selected_filename)		*
*									*/
extern Menu
filelist_menu_pullright(
        Menu_item mi,		/* Menu item */
	Menu_generate op,	/* Menu operation */
        u_long addr_func,	/* Address function to notify */
	char *start_path);	/* Current directory path */

/*
 * Get an alphabetized list of the directory contents.
 * Returns an array of pointers to strings.
 * Both the array memory and strings memory must be freed by the
 * caller with "free(addr)".
 */
char **
get_dir_contents(char *path);	/* Directory path */

/************************************************************************
*									*
*  Get the current directory for a given filelist.
*  Returns the address of a newly malloced string, which must be
*  freed by the caller.
*									*/
char *
get_filelist_dir(long id); /* which filelist */

#endif _FILELIST_H
