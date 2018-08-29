/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

/* 
 */

/************************************************************************
*									*
*  Charly Gatot								*
*  Spectroscopy Imaging Systems Corporation				*
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  This file contains routines to browse the files in a directory.      *
*  It provides a "Save" and "Load" buttons (but only one can be active
*  at a time).  User can register their function so that user-function  *
*  will be called if either these buttons are pressed.			*
*									*
*  Usage								*
*  -----								*
*  1. User can enter a filename from the text-panel and type <CR> from	*
*     keyboard.  If it is a directory,  it will go into that directory. *
*     Otherwise, it will execute user register function.  It supports   *
*     '~' (user name) and '$' (environment) characters.			*
*  2. User can enter a fractional filename from the tex-panel and type 	*
*     ESC for filename completion.  It will complete a filename, or as  *
*     many characters as possible from the current directory or lists 	*
*     all ambiguous filenames.  Note that it only works for current 	*
*     directory, and doesn't support '~' or '$'.			*
*  3. User can use the mouse to click on the filename from file-listing *
*     window.  It will load the filename to the text-panel as if 	*
*     the user had typed the filename.   If the user double-clicks it,
*     it will do exactly as in usage 1.					*
*									*
*************************************************************************/
#include <stdio.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/font.h>
#include <xview/notice.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
/* Note that xview.h uses K&R C varargs.h, which        */
/* is not compatible with stdarg.h for C++ or ANSI C.   */

#ifdef __OBJECTCENTER__
#ifdef va_start
#undef va_start
#undef va_end
#undef va_arg
#endif
#include <stdarg.h>
/* #include <varargs.h> */
#endif

#include "stderr.h"
#include "filelist.h"

#define MAXPATHLEN 1024			/* Max length for file path names */
#define	LOAD_FILE_MSG	"Load File From :"
#define	SAVE_FILE_MSG	"Save File To :"

#define	ESC_CHAR	27

/* Maximum files that can be put in the panel list */
#define	MAX_FILE	460

/* Object handler. */
/* Info common to all filelists. */
/* Note that only one of "load_but" or "save_but" can be */
/*  active (shown) at a time. */
typedef struct _filelist_hdl {
    Frame owner;			/* owner of window */
    int x,
        y;				/* position of window */
    int filename_wd;			/* width of filename */
    int filename_num;			/* number of filenames */
    int filename_xpos, filename_ypos;	/* position of list */
    Xv_font font;
    Frame popup;
    Panel panel;
    Panel_item filemsg;
    Panel_item filename;
    Panel_item dirmsg;
    Panel_item dirlist;
    Panel_item load_but;
    Panel_item save_but;
    Panel_item load_all_but;
    Panel_item msg;
    Panel_item errmsg;
}             Filelist_hdl;

/* Files information in the panel list */
/* Info specific to each filelist */
typedef struct _fileinfo {
    long id;				/* ID for a specific LOAD/SAVE */
    char *dirpath;			/* directory path name */
    int (*savefunc) (char *, char *);
    int (*loadfunc) (char *, char *);
    int (*loadallfunc) (char *, char *);
    struct _fileinfo *next;
}         Fileinfo;


void filelist_win_hide(void);

extern char *expand_filename(char *, char *);

static char *parent_login;		/* parent directory of user login */

static char dirname[MAXPATHLEN];	/* directory path name */
static int numfile = 0;			/* number of file in the current
					   directory */
static int filelist_row;		/* The filelist row we last read */
static Filelist_hdl *fhdl = NULL;	/* object handles */
static Fileinfo *finfoheader = NULL;	/* file information in panel list */
static Fileinfo *curfinfo = NULL;	/* curent file information in panel
					   list */

static void file_create_win(void);
static void file_action_proc(Panel_item, Event *);
static int file_select_proc(Panel_item, char *, caddr_t,
			    Panel_list_op, Event *);
static void file_enter_proc(Panel_item i, Event * e);
static void file_complete_name(void);
static void file_execute_proc(void);
static int file_listing(char *);
static void file_list_update(char **filename, int nfile);
static void file_free(char **, int);
static int str_cmp(char **, char **);
static void ring_bell(void);
static void err_msg(char *format,...);
static char *short_path(char *);
static void show_dir_path(char *dirpath);

/************************************************************************
*									*
*  Create file list handler.  Note that it doesn't create its window	*
*  until it is necessary.						*
*  Return OK or NOT_OK.							*
*									*/
int
filelist_win_create(Frame owner,
		    int x, int y,	/* window position */
		    int filename_wd,	/* filename width */
		    int filelist_num)	/* number of listing files */
{
    if (fhdl){
	STDERR("filelist_win_create:filelist window has already been created");
	return (NOT_OK);
    }
    if ((fhdl = (Filelist_hdl *) malloc(sizeof(Filelist_hdl))) == NULL){
	PERROR("filelist_win_create:malloc");
	return (NOT_OK);
    }
    fhdl->owner = owner;
    fhdl->x = x;
    fhdl->y = y;
    fhdl->filename_wd = filename_wd;
    fhdl->filename_num = filelist_num;
    fhdl->popup = NULL;
    fhdl->filename = NULL;
    return (OK);
}

/************************************************************************
*									*
*  Create filelist window.						*
*									*/
static
void file_create_win(void)
{
    int x_pos,
        y_pos;
    char *envhome;

    if (fhdl == NULL){
	STDERR("file_create_win:filelist handler has not been created");
	return;
    }
    (void) getcwd(dirname,MAXPATHLEN);

    /* Get the parent directory of user's home directory. Note that	 */
    /* it is a complete path name including the file-mount name.	 */
    /* The way we do this is to change directory to the login directory  */
    /* get the full-path name, and change it back to the current working */
    /* directory.							 */
    if (envhome = getenv("HOME")){
	int i;
	char temp[MAXPATHLEN];

	strcpy(temp, getenv("HOME"));
	/* Get rid of the login name.  Hence we have parent directory */
	/* of home directory.						*/
	i = strlen(temp);
	while (temp[i] != '/'){
	    i--;
	}
	temp[i] = 0;
	parent_login = strdup(temp);
    }else{
	parent_login = (char *)malloc(1);	/* allocate 1 memory */
	parent_login[0] = 0;
    }

    /* Set the default values */
    if (fhdl->filename_wd == 0){
	fhdl->filename_wd = 380;
    }
    if (fhdl->filename_num == 0){
	fhdl->filename_num = 10;
    }


    if ((fhdl->font = xv_find(fhdl->owner, FONT,
			      FONT_FAMILY, FONT_FAMILY_LUCIDA,
			      FONT_STYLE, FONT_STYLE_BOLD,
			      NULL))
	== NULL)
    {
	STDERR("file_create_win: FONT_FAMILY_LUCIDA");
    }

    /* Create pop-up frame */
    fhdl->popup = xv_create(fhdl->owner, FRAME_CMD,
			    FRAME_LABEL, "File Browser",
			    XV_X, fhdl->x,
			    XV_Y, fhdl->y,
			    XV_FONT, fhdl->font,
			    FRAME_SHOW_RESIZE_CORNER, TRUE,
			    NULL);

    /* Get a panel from pop-up frame */
    fhdl->panel = xv_get(fhdl->popup, FRAME_CMD_PANEL);

    /* Set the panel font */
    xv_set(fhdl->panel, XV_FONT, fhdl->font, NULL);

    x_pos = 12;
    y_pos = 6;
    fhdl->filemsg = xv_create(fhdl->panel, PANEL_MESSAGE,
			      XV_X, x_pos,
			      XV_Y, y_pos,
			      PANEL_LABEL_STRING, LOAD_FILE_MSG,
			      NULL);

    fhdl->load_but = xv_create(fhdl->panel, PANEL_BUTTON,
			       XV_X, x_pos + fhdl->filename_wd - 40,
			       XV_Y, y_pos,
			       PANEL_LABEL_STRING, "Load",
			       PANEL_CLIENT_DATA, FILELIST_LOAD,
			       PANEL_NOTIFY_PROC, file_action_proc,
			       NULL);

    fhdl->save_but = xv_create(fhdl->panel, PANEL_BUTTON,
			       XV_X, x_pos + fhdl->filename_wd - 40,
			       XV_Y, y_pos,
			       PANEL_LABEL_STRING, "Save",
			       PANEL_CLIENT_DATA, FILELIST_SAVE,
			       PANEL_NOTIFY_PROC, file_action_proc,
			       NULL);

    y_pos += (int) xv_get(fhdl->filemsg, XV_HEIGHT) + 8;
    fhdl->load_all_but = xv_create(fhdl->panel, PANEL_BUTTON,
				   XV_X, x_pos + fhdl->filename_wd - 40,
				   XV_Y, y_pos,
				   PANEL_LABEL_STRING, "Load All",
				   PANEL_CLIENT_DATA, FILELIST_LOAD_ALL,
				   PANEL_NOTIFY_PROC, file_action_proc,
				   NULL);

    y_pos += (int) xv_get(fhdl->filemsg, XV_HEIGHT) + 8;
    fhdl->filename = xv_create(fhdl->panel, PANEL_TEXT,
			       XV_X, x_pos,
			       XV_Y, y_pos,
			       PANEL_VALUE, "",
		PANEL_VALUE_DISPLAY_LENGTH, (int) (fhdl->filename_wd / 10),
			       PANEL_NOTIFY_LEVEL, PANEL_SPECIFIED,
			       PANEL_NOTIFY_STRING, "\t\r\t\033",
			       PANEL_NOTIFY_PROC, file_enter_proc,
			       NULL);

    y_pos += (int) xv_get(fhdl->filename, XV_HEIGHT) + 14;
    fhdl->dirmsg = xv_create(fhdl->panel, PANEL_MESSAGE,
			     XV_X, x_pos,
			     XV_Y, y_pos,
			     PANEL_LABEL_STRING, "",
			     NULL);
    show_dir_path(short_path(dirname));

    y_pos += (int) xv_get(fhdl->dirmsg, XV_HEIGHT) + 8;
    fhdl->filename_xpos = x_pos;
    fhdl->filename_ypos = y_pos;
    fhdl->dirlist = xv_create(fhdl->panel, PANEL_LIST,
			      XV_X, x_pos,
			      XV_Y, y_pos,
			      PANEL_LIST_WIDTH, fhdl->filename_wd,
			      PANEL_LIST_DISPLAY_ROWS, fhdl->filename_num,
			      PANEL_NOTIFY_PROC, file_select_proc,
			      NULL);

    y_pos += (int) xv_get(fhdl->dirlist, XV_HEIGHT) + 8;
    fhdl->msg = xv_create(fhdl->panel, PANEL_MESSAGE,
			  PANEL_LABEL_STRING, "Message",
			  XV_X, x_pos,
			  XV_Y, y_pos,
			  NULL);

    y_pos += (int) xv_get(fhdl->msg, XV_HEIGHT) + 8;
    fhdl->errmsg = xv_create(fhdl->panel, PANEL_MESSAGE,
			     PANEL_LABEL_STRING, "Error Message",
			     XV_X, x_pos,
			     XV_Y, y_pos,
			     NULL);

    window_fit(fhdl->panel);
    window_fit(fhdl->popup);
}

/************************************************************************
*									*
*  Destroy the object handler.						*
*									*/
void
filelist_win_destroy(void)
{
    (void) xv_destroy_safe(fhdl->popup);
    (void) free((char *) fhdl);
    fhdl = NULL;

    while (curfinfo = finfoheader){
	finfoheader = finfoheader->next;
	if (curfinfo->dirpath){
	    free(curfinfo->dirpath);
	}
	free((char *) curfinfo);
    }
}

/************************************************************************
*									*
*  Register user functions to call when the "Load" or "Save" button is  *
*  selected.								*
*									*/
void
filelist_notify_func(long id,
		     Flist_type ftype,
		     long addr_loadfunc,
		     long addr_savefunc,
		     long addr_loadallfunc)
{
    Fileinfo *ptr,
            *prev;			/* Fileinfo pointers */
    char curdir[MAXPATHLEN];		/* buffer for current directory name */

    if (fhdl == NULL){
	STDERR("filelist_notify_func:File browser window has NOT been created");
	return;
    }
    for (prev = ptr = finfoheader; ptr; prev = ptr, ptr = ptr->next){
	/* Only allow to change callback function if */
	/* ftype != FILELIST_NEW */
	if (ptr->id == id){
	    if (ftype == FILELIST_CHANGE){
		ptr->loadfunc = (int (*) (char *, char *)) addr_loadfunc;
		ptr->loadallfunc = (int (*) (char *, char *)) addr_loadallfunc;
		ptr->savefunc = (int (*) (char *, char *)) addr_savefunc;
	    }else if (ftype == FILELIST_DELETE){
		if (ptr == finfoheader)
		    finfoheader = ptr->next;
		else
		    prev->next = ptr->next;

		if (ptr == curfinfo){
		    /* We should close the file-browser if we delete its */
		    /* item from the finfoheader list.		    */
		    xv_set(fhdl->popup, FRAME_CMD_PUSHPIN_IN, FALSE,
			   XV_SHOW, FALSE,
			   NULL);
		    curfinfo = NULL;
		}
		if (ptr->dirpath)
		    free(ptr->dirpath);
		free((char *) ptr);
	    }else
		err_msg("Id %d exists", id);
	    return;
	}
    }

    /* Cannot find the ID */
    if (ftype != FILELIST_NEW){
	STDERR_1("filelist_notify_func:No such registered ID: %d", id);
	return;
    }
    /* === Now we know that ftype is FILELIST_NEW ==== */

    /* Allocate memory for a new fileinfo */
    if ((ptr = (Fileinfo *) malloc(sizeof(Fileinfo))) == NULL){
	PERROR("filelist_notify_func:malloc");
	exit(1);
    }
    ptr->id = id;
    getcwd(curdir,MAXPATHLEN);
    ptr->dirpath = strdup(curdir);
    ptr->loadfunc = (int (*) (char *, char *)) addr_loadfunc;
    ptr->loadallfunc = (int (*) (char *, char *)) addr_loadallfunc;
    ptr->savefunc = (int (*) (char *, char *)) addr_savefunc;
    ptr->next = NULL;

    /* Link it into a list at the last position */
    if (prev){
	prev->next = ptr;
    }else{
	finfoheader = ptr;
    }
}

/************************************************************************
*                                                                       *
*  Set the default directory to use for a given FileBrowser if
*  filelist_win_show() is called with a NULL directory pointer.
*									*/
void
filelist_set_directory(long id,		/* ID of FileBrowser to change */
		       char *dirname)	/* Directory pathname */
{
    Fileinfo *finfo;
    
    /* Find the requested ID */
    for (finfo=finfoheader; finfo; finfo=finfo->next){
	if (finfo->id == id){
	    break;
	}
    }
    
    if (finfo == NULL){
	STDERR_1("filelist_set_directory(): No such ID registered: %d", id);
	return;
    }else{
	free(finfo->dirpath);
	finfo->dirpath = strdup(dirname);
    }    
}

/************************************************************************
*									*
*  Get the current directory for a given filelist.
*  Returns the address of a newly malloced string, which must be
*  freed by the caller.
*									*/
char *
get_filelist_dir(long id)		/* which filelist */
{
    /* Find the requested ID */
    Fileinfo *newcurfinfo;		/* new current curfinfo */
    for (newcurfinfo=finfoheader; newcurfinfo; newcurfinfo=newcurfinfo->next){
	if (newcurfinfo->id == id){
	    break;
	}
    }
    if (newcurfinfo == NULL){
	STDERR_1("get_filelist_dir: No such ID registered: %d", id);
	return NULL;
    }
    if (newcurfinfo->dirpath == NULL){
	STDERR("get_filelist_dir: No default directory");
	return NULL;
    }
    return strdup(newcurfinfo->dirpath);
}

/************************************************************************
*									*
*  Pop up the filelist frame.						*
*  Note that "state" indicates the file-browser for loading or saving.	*
*  The "dir_path" indicates go to the specified directory.  If it is	*
*  NULL, use the default directory (which is kept in Fileinfo).		*
*  The title name will be shown in the title window. If it is NULL,	*
*  the title "File Browser" will be shown.				*
*									*/
void
filelist_win_show(long id,		/* which filelist */
		  Flist_state state,	/* FILELIST_LOAD, FILELIST_LOAD_ALL, */
					/* or FILELIST_SAVE */
		  char *dir_path,	/* destination directory */
		  char *title,		/* Title of file-browser */
		  char *button_name)	/* string to put in button */
{					
    Fileinfo *newcurfinfo;		/* new current curfinfo */
    char dest_dir[MAXPATHLEN];		/* destination directory to list */
    char temp[1024];			/* temporary buffer */

    /* Create filelist window if it is not created */
    if (fhdl->popup == NULL){
	file_create_win();
    }

    err_msg("");

    if (state == FILELIST_LOAD){
	xv_set(fhdl->load_but, PANEL_SHOW_ITEM, TRUE, NULL);
	xv_set(fhdl->load_all_but, PANEL_SHOW_ITEM, FALSE, NULL);
	xv_set(fhdl->save_but, PANEL_SHOW_ITEM, FALSE, NULL);
	xv_set(fhdl->filemsg, PANEL_LABEL_STRING, LOAD_FILE_MSG, NULL);
	/* Change the button name  */
	xv_set(fhdl->load_but, PANEL_LABEL_STRING,
	       button_name ? button_name : "Load", NULL);

    }else if (state == FILELIST_LOAD_ALL){
	xv_set(fhdl->load_but, PANEL_SHOW_ITEM, TRUE, NULL);
	xv_set(fhdl->load_all_but, PANEL_SHOW_ITEM, TRUE, NULL);
	xv_set(fhdl->save_but, PANEL_SHOW_ITEM, FALSE, NULL);
	xv_set(fhdl->filemsg, PANEL_LABEL_STRING, LOAD_FILE_MSG, NULL);
	strcpy(temp, button_name ? button_name : "Load");
	xv_set(fhdl->load_but, PANEL_LABEL_STRING, temp, NULL);
	xv_set(fhdl->load_all_but, PANEL_LABEL_STRING,
	       strcat(temp, " All"), NULL);
    }else if (state == FILELIST_SAVE){
	xv_set(fhdl->load_but, PANEL_SHOW_ITEM, FALSE, NULL);
	xv_set(fhdl->load_all_but, PANEL_SHOW_ITEM, FALSE, NULL);
	xv_set(fhdl->save_but, PANEL_SHOW_ITEM, TRUE, NULL);
	xv_set(fhdl->filemsg, PANEL_LABEL_STRING, SAVE_FILE_MSG, NULL);
	xv_set(fhdl->save_but, PANEL_LABEL_STRING,
	       button_name ? button_name : "Save", NULL);
    }else{
	STDERR_1("filelist_win_show:No known state: %d", state);
	return;
    }

    /* Change the title */
    xv_set(fhdl->popup, FRAME_LABEL, title ? title : "File Browser", NULL);

    /* Find the requested ID */
    for (newcurfinfo=finfoheader; newcurfinfo; newcurfinfo=newcurfinfo->next){
	if (newcurfinfo->id == id){
	    break;
	}
    }

    if (newcurfinfo == NULL){
	STDERR_1("filelist_win_show:No such ID registered: %d", id);
	return;
    }
    if (dir_path){
	(void) strcpy(dest_dir, dir_path);	/* Use requested directory */
    }else{
	(void) strcpy(dest_dir, newcurfinfo->dirpath);	/* Use default dir. */
    }

    strcpy(dirname, dest_dir);

    /* Listing the files */
    file_listing(dirname);

    /* Update the directory message */
    show_dir_path(short_path(dirname));

    /* Update the current curfinfo */
    curfinfo = newcurfinfo;

    xv_set(fhdl->popup,
	   FRAME_CMD_PUSHPIN_IN, TRUE,
	   XV_SHOW, TRUE,
	   NULL);
}

void
filelist_win_hide(void)
{
/*
 *	Hide pop_up window
 */

    xv_set(fhdl->popup,
	   FRAME_CMD_PUSHPIN_IN, FALSE,
	   XV_SHOW, FALSE,
	   NULL);
}



/************************************************************************
*									*
*  Update the file list from the current directory.  The files might be	*
*  added or deleted.							*
*									*/
void
filelist_update(void)
{
    if (fhdl && fhdl->filename){
	xv_set(fhdl->filename, PANEL_VALUE, "", NULL);
	(void) file_listing(dirname);
    }
}

/************************************************************************
*                                                                       *
*  Get the name of the first file in the current file list.
*  Returns a pointer to the file name, or 0 if there is no file.
*									*/
extern char *
filelist_first_file(void)
{
    char *p = 0;

    filelist_row = 0;		/* Reset row number for filelist_next_file() */
    if ( (int)xv_get(fhdl->dirlist, PANEL_LIST_NROWS) < 1 ){
	return 0;
    }else{
	p = (char *)xv_get(fhdl->dirlist, PANEL_LIST_STRING, 0, p);
	return p;
    }
}
	
/************************************************************************
*                                                                       *
*  Get the name of the next file in the current file list.
*  Returns a pointer to the file name, or 0 if there are no more files.
*									*/
extern char *
filelist_next_file(void)
{
    char *p = 0;

    filelist_row++;		/* Update the row number */
    if ( (int)xv_get(fhdl->dirlist, PANEL_LIST_NROWS) < filelist_row + 1 ){
	return 0;
    }else{
	p = (char *)xv_get(fhdl->dirlist, PANEL_LIST_STRING, filelist_row, p);
	return p;
    }
}
	
/************************************************************************
*									*
*  User has clicked the "Load" or "Save" button.			*
*									*/
static void
file_action_proc(Panel_item i, Event *)
{
    char *filename;			/* pointer to a selected filename */

    err_msg("");

    if ((filename = (char *) xv_get(fhdl->filename, PANEL_VALUE)) == NULL){
	err_msg("No filename selected");
	return;
    }
    if (curfinfo == NULL){
	err_msg("No registered function");
	return;
    }
    if ((Flist_state) xv_get(i, PANEL_CLIENT_DATA) == FILELIST_LOAD){
	if (curfinfo->loadfunc){
	    curfinfo->loadfunc(dirname, filename);
	}
    }else if ((Flist_state) xv_get(i, PANEL_CLIENT_DATA) == FILELIST_SAVE){
	if (curfinfo->savefunc){
	    char path[1025];
	    sprintf(path,"%s/%s", dirname, filename);
	    struct stat statbuf;
	    int staterr = stat(path, &statbuf);
	    if ( staterr || confirm_overwrite(filename) ){
		curfinfo->savefunc(dirname, filename);
	    }
	}
    }else if ((Flist_state) xv_get(i, PANEL_CLIENT_DATA) == FILELIST_LOAD_ALL){
	if (curfinfo->loadallfunc){
	    curfinfo->loadallfunc(dirname, filename);
	}
    }else{
	err_msg("No registered flist state");
	return;
    }
}

/************************************************************************
*									*
*  User has selected a choice from the panel list.			*
*									*/
#define DOUBLECLICK_TIME 1.0
static int
file_select_proc(Panel_item,
		 char *str,
		 caddr_t,
		 Panel_list_op op,
		 Event * e)
{
    /* The time the filename is first selected */
    static struct timeval firsttime;
    static int select = 0;

    err_msg("");

    if (op == PANEL_LIST_OP_DESELECT){
	select = 0;
	return XV_OK;
    }else if (op == PANEL_LIST_OP_SELECT){
	if (str == NULL){
	    printf("file_select_proc: Null argument (str = 0)\n");
	    return XV_OK;
	}
	
	/* Allow double click as if the user had typed the file name. */
	double deltat = (e->ie_time.tv_sec - firsttime.tv_sec +
			 (e->ie_time.tv_usec - firsttime.tv_usec) * 1.0e-6);
	if (deltat <= DOUBLECLICK_TIME && select){
	    select = 0;
	    file_execute_proc();
	}else{
	    select = 1;
	    firsttime.tv_sec = e->ie_time.tv_sec;
	    firsttime.tv_usec = e->ie_time.tv_usec;
	    xv_set(fhdl->filename, PANEL_VALUE, str, NULL);
	}
    }else if (op == PANEL_LIST_OP_VALIDATE){
	// Don't allow user to type in new entries
	return XV_ERROR;
    }else if (op == PANEL_LIST_OP_DELETE){
	// User is deleting a row
	// Delete the file if it's a good idea
	// If we can't or don't want to delete the file, just ignore it
    }
    return XV_OK;
}

/************************************************************************
*									*
*  The user uses the keyboard to execute (instead of the mouse).	*
*									*/
static void
file_enter_proc(Panel_item, Event * e)
{
    if (event_id(e) == ESC_CHAR){
	//file_complete_name();  // Disabled for now (CMP)
    }else{
	/* This is <CR> */
	file_execute_proc();
    }
}

/************************************************************************
*									*
*  User has entered part of the filename, and this routine tries to     *
*  complete a filename from the current directory.  If it finds more    *
*  than one filename to complete,  it will list them in the panel list
*  window.								*
*									*/
static void
file_complete_name(void)
{
    char fractname[128];		/* fraction user name */
    int fractnamlen;			/* fraction user name length */
    register struct dirent *dp; /* directory info pointer for each file */
    DIR *dirptr;			/* directory file pointer */
    struct stat statbuf;		/* status of a file */
    int nfile;				/* number of files */
    char *filename[MAX_FILE];		/* pointer to a filename */
    int i;				/* loop counter */

    (void) strcpy(fractname, (char *) xv_get(fhdl->filename, PANEL_VALUE));
    if ((fractnamlen = strlen(fractname)) == 0)
	return;

    /* open the directory */
    if ((dirptr = opendir(dirname)) == NULL){
	char dirbuf[MAXPATHLEN];

	err_msg("Cannot open %s", short_path(getcwd(dirbuf,MAXPATHLEN)));
	return;
    }
    nfile = 0;
    for (dp = readdir(dirptr); dp != NULL; dp = readdir(dirptr)){
	if (strncmp(dp->d_name, fractname, fractnamlen) == 0){
	    (void) stat(dp->d_name, &statbuf);
	    if ((statbuf.st_mode & S_IFMT) == S_IFDIR){
		if ((filename[nfile] = (char *) malloc(strlen(dp->d_name) + 2))
		    == NULL) {
		    PERROR("file_complte_name:malloc");
		    file_free(filename, nfile);
		    closedir(dirptr);
		    return;
		}
		(void) sprintf(filename[nfile], "%s/", dp->d_name);
	    }else{
		if ((filename[nfile] = strdup(dp->d_name)) == NULL){
		    PERROR("file_complte_name:strdup");
		    file_free(filename, nfile);
		    closedir(dirptr);
		    return;
		}
	    }
	    if (nfile++ == (MAX_FILE - 2)){
		err_msg("Only can list %d maximum files", MAX_FILE);
		break;
	    }
	}
    }

    if (nfile == 0){			/* NO such fractional filename */
	err_msg("No such fractional filename: %s", fractname);
	return;
    }
    if (nfile == 1){			/* We can complete filename */
	xv_set(fhdl->filename, PANEL_VALUE, filename[0], NULL);
    }else{				/* More than one filename found */
	char c0;			/* the first filename character */
	char ch;			/* the rest of the filename
					   character */

	/* Try to complete the fractional filename as many characters  */
	/* as possible.  The idea is to compare all the filenames      */
	/* starting at the position of the 'fractnamlen'.              */
	while (1){
	    /* A character to be checked (for the first filename) */
	    if ((c0 = *(filename[0] + fractnamlen)) == 0)
		break;

	    for (i = 1; i < nfile; i++){
		ch = *(filename[i] + fractnamlen);
		if ((ch == 0) || (ch != c0))
		    goto STOP_COMPLETE;
	    }

	    /* All filenames have the same character at 'fractnamlen' */
	    fractname[fractnamlen++] = c0;
	    fractname[fractnamlen] = 0;
	}

STOP_COMPLETE:

	/* Update the user fractional filename */
	xv_set(fhdl->filename, PANEL_VALUE, fractname, NULL);

	/* Update the panel list to all matching filenames */
	file_list_update(filename, nfile);
    }

    /* Free the filenames */
    file_free(filename, nfile);
}

/************************************************************************
*									*
*  strstr() defined here because it's missing from the CenterLine
*  library.  See man page string(3) for details.
*									*/
#ifndef LINUX
static char *
strstr(char *s1, char *s2)
{
    char *p;
    int i;
    int j;
    int k;

    j = strlen(s2);
    i = strlen(s1) - j;
    if (i < 0){
	return 0;
    }
    for (p=s1; (i >= 0) && (k=strncmp(p, s2, j)); p++, i--);
    if (k){
	return 0;
    }else{
	return p;
    }
}
#endif

/************************************************************************
*									*
*  User has entered a filename.  If it is a directory, go into that     *
*  directory and list all directory files.  Otherwise, return to        *
*  the user function call.						*
*									*/
static void
file_execute_proc(void)
{
    static char markers[] = "/"; /* List of spurious symbols at end of names */
    char fullname[MAXPATHLEN];
    char filename[MAXPATHLEN];
    char *p;				/* Temp */
    char *q;
    char *uname;			/* user seleted filename */
    int i;
    struct stat statbuf;		/* status of a file */

    uname = (char *) xv_get(fhdl->filename, PANEL_VALUE);
    if (*uname == NULL){
	err_msg("No file name entered");
	return;
    }
    if (expand_filename(uname, filename) == NULL){
	err_msg("Can't expand filename");
	return;
    }

    // Strip off any trailing markers
    while ( (p=strpbrk(filename+strlen(filename)-1, markers))
	   && (p > filename))	// Don't remove everything
    {
	*p = 0;
    }

    if (*filename == '/'){
	// We have given a full path name
	strcpy(fullname, filename);
    }else{
	strcpy(fullname, dirname);
	strcat(fullname, "/");
	strcat(fullname, filename);
    }
    
    // Avoid uncontrolled pathname growth

    // Remove excess slashes ("//" is same as "/")
    while (p=strstr(fullname, "//")){
	// Move portion of string beyond first "/" down one place
	do{
	    *p = *(p+1);
	} while (*p++);
    }

    // Remove "/./" elements
    while (p=strstr(fullname, "/./")){
	// Move portion of string beyond "/." down two places
	do{
	    *p = *(p+2);
	} while (*p++);
    }

    // Remove trailing "/."
    i = strlen(fullname);
    if (strcmp(fullname+i-2, "/.") == 0){
	// Filename ends in "/."; cut it off
	fullname[i-2] = 0;
    }

    // Remove "/../" elements (along with preceding element)
    while (p=strstr(fullname, "/../")){
	q = p + 3;				// Point to following "/"
	while ( (*(--p) != '/') && (p > fullname) ); // Point to previous "/"
	do{
	    *p = *q++;			// Move following stuff down
	} while (*p++);
    }

    // Remove trailing "/.." (along with preceding element)
    i = strlen(fullname);
    if (strcmp(fullname+i-3, "/..") == 0){
	// Filename ends in "/.."; cut off last two elements
	for (i=0; (i<2) && (p=strrchr(fullname, '/')); i++){
	    *p = 0;
	}
    }

    // Make sure something is left of the name
    if ( *fullname == 0 ){
	strcpy(fullname, "/");
    }

    int staterr;
    if ((staterr=stat(fullname, &statbuf)) == 0
	&& (statbuf.st_mode & S_IFMT) == S_IFDIR)
    {
	strcpy(dirname, fullname);
	
	/* Get the files */
	if (file_listing(dirname) == NOT_OK){
	    err_msg("Error in file_listing");
	    return;
	}

	/* Update the directory message */
	show_dir_path(short_path(dirname));
	xv_set(fhdl->filename, PANEL_VALUE, "", NULL);

	/* Remember new default directory */
	if (curfinfo){
	    free(curfinfo->dirpath);
	    curfinfo->dirpath = strdup(dirname);
	}
    }else{
	if (curfinfo == NULL){
	    err_msg("No registered function");
	    return;
	}
	/* in this case we will only load/save one file */
	if (xv_get(fhdl->load_but, PANEL_SHOW_ITEM) == TRUE){
	    if (staterr){
		err_msg("Can't find file: %s", short_path(fullname));
		return;
	    }else if ((statbuf.st_mode & S_IFMT) != S_IFREG){
		err_msg("Error: Invalid file or directory.");
		return;
	    }else{
		if (curfinfo->loadfunc){
		    curfinfo->loadfunc(dirname, filename);
		}
	    }
	}else{
	    if (curfinfo->savefunc && (staterr||confirm_overwrite(filename))){
		curfinfo->savefunc(dirname, filename);
	    }
	}
    }
}

/************************************************************************
*									*
*  Get all files from directory "dirpath" and insert them into panel	*
*  list.  All directory files will be appended by '/' at the end.	*
*  Return OK or NOT_OK.							*
*									*/
static int
file_listing(char *dirpath)
{
    register struct dirent *dp; /* directory info pointer for each file */
    DIR *dirptr;			/* directory file pointer */
    struct stat statbuf;		/* status of a file */
    int nfile;				/* number of files */
    char fullname[MAXPATHLEN];
    char *filename[MAX_FILE];		/* pointer to a filename */

    /* open the directory */
    if ((dirptr = opendir(dirpath)) == NULL){
	err_msg("Can't open %s", short_path(dirpath));
	return (NOT_OK);
    }
    /* Read files from directory.  Add '/' for a directory file */
    nfile = 0;
    for (dp = readdir(dirptr); dp != NULL; dp = readdir(dirptr)){
	/* Don't list "hidden" files */
	if (*dp->d_name != '.'){
	    strcpy(fullname, dirname);
	    strcat(fullname, "/");
	    strcat(fullname, dp->d_name);
	    (void) stat(fullname, &statbuf);
	    if ((statbuf.st_mode & S_IFMT) == S_IFDIR){
		if ((filename[nfile] = (char *) malloc(strlen(dp->d_name) + 2))
		    == NULL)
		{
		    PERROR("file_listing:malloc");
		    file_free(filename, nfile);
		    closedir(dirptr);
		    return (NOT_OK);
		}
		(void) sprintf(filename[nfile], "%s/", dp->d_name);
	    }else{
		if ((filename[nfile] = strdup(dp->d_name)) == NULL){
		    PERROR("file_listing:strdup");
		    file_free(filename, nfile);
		    closedir(dirptr);
		    return (NOT_OK);
		}
	    }

	    if (nfile++ == (MAX_FILE - 2)){
		err_msg("Only can list %d maximum files", MAX_FILE);
		break;
	    }
	}
    }

    closedir(dirptr);

    /* Add the "../" so that the user can move up a directory */
    if ((filename[nfile] = strdup("../")) == NULL){
	PERROR("file_listing:strdup");
	file_free(filename, nfile);
	closedir(dirptr);
	return (NOT_OK);
    }
    nfile++;

    file_list_update(filename, nfile);
    file_free(filename, nfile);

    return (OK);
}

/************************************************************************
*									*
*  Update the panel file list.						*
*									*/
static void
file_list_update(char **filename, int nfile)
{
    int i;				/* loop counter */
    char msg[60];			/* message to user */

    /* sort directory files by file name */
    if (nfile > 1)
	qsort((char *) filename, nfile, sizeof(char **),
	      (int (*) (const void *, const void *)) str_cmp);

    /* Save the current file number */
    numfile = nfile;

    // Delete the current list and create a new one
    xv_set(fhdl->dirlist, PANEL_SHOW_ITEM, FALSE, 0);
    xv_destroy_safe(fhdl->dirlist); // Destroy after return from this routine
    fhdl->dirlist = xv_create(fhdl->panel, PANEL_LIST,
			      XV_X, fhdl->filename_xpos,
			      XV_Y, fhdl->filename_ypos,
			      PANEL_LIST_WIDTH, fhdl->filename_wd,
			      PANEL_LIST_DISPLAY_ROWS, fhdl->filename_num,
			      PANEL_NOTIFY_PROC, file_select_proc,
			      PANEL_SHOW_ITEM, FALSE,
			      NULL);

    /* Put all files into panel list */
    for (i = 0; i < numfile; i++){
	xv_set(fhdl->dirlist,
	       PANEL_LIST_INSERT, i,
	       PANEL_LIST_STRING, i, filename[i],
	       PANEL_LIST_SELECT, i, FALSE,
	       NULL);
    }
    //xv_set(fhdl->dirlist, PANEL_LIST_SORT, PANEL_FORWARD, 0);//Not implemented
    xv_set(fhdl->dirlist, PANEL_SHOW_ITEM, TRUE, 0);

    /* Print number of files */
    (void) sprintf(msg, "%d files.", numfile);
    xv_set(fhdl->msg, PANEL_LABEL_STRING, msg, NULL);
}

/************************************************************************
*									*
*  Free files.								*
*									*/
static void
file_free(char **filename, int nfile)
{
    while (nfile){
	(void) free(filename[--nfile]);
    }
}

/************************************************************************
*									*
*  Compare two string pointers.						*
*									*/
static int
str_cmp(char **a, char **b)
{
    return (strcmp(*a, *b));
}

/************************************************************************
*									*
*  Bell.								*
*									*/
static void
ring_bell(void)
{
    XBell((Display *) xv_get(fhdl->popup, XV_DISPLAY), 0);
}

/************************************************************************
*									*
*  Ouput error message on filebrowser's "errmsg" line.			*
*									*/
static void
err_msg(char *format,...)
{
    va_list vargs;			/* variable argument pointer */
    char errmsg[128];			/* error buffer for message */

    va_start(vargs, format);
    (void) vsprintf(errmsg, format, vargs);
    va_end(vargs);

    if (*errmsg != 0){
	ring_bell();
    }

    xv_set(fhdl->errmsg, PANEL_LABEL_STRING, errmsg, NULL);
}

/************************************************************************
*									*
*  Convert the pathname to ~<loginnname>/... if possible and return it.	*
*									*/
static char *
short_path(char *path)
{
    int len;
    static char shortpath[128];
    char *homeenv;

    if (strncmp(path, parent_login, len = strlen(parent_login))){
	if (homeenv = getenv("HOME")){
	    for (len = strlen(homeenv); homeenv[len] != '/'; len--);

	    if (strncmp(path, homeenv, len)){
		return (strcpy(shortpath, path));
	    }
	}else{
	    return (strcpy(shortpath, path));
	}
    }
    /* strlen(path) should be greter than "len" */
    if (len < strlen(path)){
	(void) sprintf(shortpath, "~%s", path + len + 1);
    }else{
	return (strcpy(shortpath, path));
    }

    return (shortpath);
}

/************************************************************************
*									*
*  Print out directory name on file-browser.  It limits the length of 	*
*  the string to be printed out.					*
*									*/
static void
show_dir_path(char *dirpath)
{
    /* Append ... at the first three charcaters and cut-off the characters */
    /* which can not be showed up on the control-panel.			 */
    if (strlen(dirpath) > (fhdl->filename_wd / 8)){
	char dirbuf[128];

	(void) sprintf(dirbuf, "...%s",
		       dirpath + strlen(dirpath) - (fhdl->filename_wd / 8));
	xv_set(fhdl->dirmsg, PANEL_LABEL_STRING, dirbuf, NULL);
    }else{
	xv_set(fhdl->dirmsg, PANEL_LABEL_STRING, dirpath, NULL);
    }
}

/************************************************************************
*									*
*  Put up a warning to confirm overwriting of file.
*  "filename" is the name of the file that will be overwritten.
*  Returns TRUE if answer is "Yes", otherwise returns FALSE.
*									*/
int
confirm_overwrite(char *filename)
{
    int notice_stat =
    notice_prompt(fhdl->panel, NULL,
		  NOTICE_MESSAGE_STRINGS,
		  "Overwrite existing file?",
		  filename,
		  NULL,
		  NOTICE_BUTTON_YES, "Yes",
		  NOTICE_BUTTON_NO, "No",
		  NULL);
    if (notice_stat == NOTICE_YES){
	return TRUE;
    }else{
	return FALSE;
    }
}
