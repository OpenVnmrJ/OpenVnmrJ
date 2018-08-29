/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

static char *Sid(){
    return "@(#)framelist.c 18.1 03/21/08 (c)1991-92 SISCO";
}

//#define DMALLOC	// Uncomment to run with debug_malloc

/************************************************************************
*									*
*  Doug Landau
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  This frame contains routines to add and delete frames to the         *
*  playlist.  User can register their function so that it will be       *
*  called when frames are added or deleted.                    		*
*									*
*  Usage								*
*  -----								*
*  1. This program registers itself with the file-browser.             	*
*     The file-browser then passes selected path/filenames which        *
*     appear in this window.  At this time this program calls           *
*     whatever function has been registered with it and passes          *
*     the same information along, together with a number which is       *
*     the position of the name in the list.                             *
*                                                                       *
*  2. User can use the mouse to click on a framename, highlighting it.  *
*     The next framename sent from the filebrowser will be inserted     *
*     before the highlighted position.  The blank line immediately      *
*     following the last name in the list may also be highlighted.      * 
*									*
*  3. The delete button causes the frame_browser to call the same       *
*     function that it does upon adding a new frame to the list, but    *
*     instead of a path name, file name, and insertion point, it passes *
*     "delete", "delete", and the number in the list of the name to     *
*     delete.                                                           *
*************************************************************************/
#include <stdio.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/font.h>
#include <unistd.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
/* Notes that xview.h uses K&R C varargs.h which        */
/* is not compatible with stdarg.h for C++ or ANSI C.   */

#ifdef __OBJECTCENTER__
#ifdef va_start
#undef va_start
#undef va_end
#undef va_arg
#endif
#endif

#include <stdarg.h>
#include "stderr.h"
#include "framelist.h"
#include "filelist.h"
#include "filelist_id.h"
#include "initstart.h"
#include "ddllib.h"
#include "msgprt.h"

/* Maximum frames can be listed in panel list */
#define	MAX_FRAME	460
#define DEFAULT_VERT_GAP  25

typedef struct _Framelist_hdl
{
   Frame owner;		/* owner of window */
   int x, y;		/* position of window */
   int framename_x;	/* Position of filelist */
   int framename_y;
   int framename_wd;	/* width of filename */
   int framename_num;	/* number of filenames */
   Xv_font font;
   Frame popup;
   Panel panel;
   Panel_item dirlist;
   Panel_item delete_but;
   Panel_item load_but;
   Panel_item save_but;
   Panel_item msg;
} Framelist_hdl;



static Framelist_hdl *fhdl=NULL; /* object handles */
static void frame_create_win(void);
static void frame_action_proc(Panel_item, Event *);
static void frame_delete_proc(Panel_item, Event *);
static void frame_save_proc(Panel_item, Event *);
static void frame_load_proc(Panel_item, Event *);
static void frame_list_update(void);
static void frame_free(char **, int);
static void ring_bell(void);
static void show_nframes(int numframes);
static void err_msg(char *format, ...);
static int framelist_get_name(char *, char *);
static int frame_get_select(void);
static int framelist_get_all(char *, char *);

static int numframes = 0 ;	/* number of frames */
char *framename[MAX_FRAME];	/* filee part of image directory */
char *framepath[MAX_FRAME];	/* path part of image directory */
char *framevs[MAX_FRAME];	/* vs of image */
char playlist_name[128];	/* name of playlist file to save to/load from */

static double (*okfunc)(char *, char *, int) = NULL ;


/************************************************************************
*									*
*  Create frame list handlers.  Note that it doesn't create its window	*
*  until it is necessary.						*
*  Return OK or NOT_OK.							*
*									*/
int
framelist_win_create(Frame owner,
			int x, int y,  		/* window position */
		        int framename_wd, 	/* filename width */
			int framelist_num)	/* number of listing files */
{
#ifdef DMALLOC
    cerr << "\n";
#endif
   if (fhdl)
   {
      STDERR("framelist_win_create:filelist window has already been created");
      return(NOT_OK);
   }

   if ((fhdl = (Framelist_hdl *)malloc(sizeof(Framelist_hdl))) == NULL)
   {
      PERROR("framelist_win_create:malloc");
      return(NOT_OK);
   }

   fhdl->owner = owner;
   fhdl->x = x;
   fhdl->y = y;
   fhdl->framename_wd = framename_wd;
   fhdl->framename_num = framelist_num;
   fhdl->popup = NULL;

   okfunc = (double (*)(char *, char *,int)) NULL ;

   filelist_notify_func(FILELIST_MOVIE_ID, FILELIST_NEW,
			(long)&framelist_get_name, NULL, 
			(long)&framelist_get_all);

   return(OK);
}

/************************************************************************
*									*
*  Create framelist window.						*
*									*/
static void
frame_create_win(void)
{
   int x_pos, y_pos;
   int name_width, num_names ;
   char initname[128];

   if (fhdl == NULL)
   {
      STDERR("frame_create_win:filelist handler has not been created");
      return;
   }
 
   numframes=0;

   /* Set the default values */
   if (fhdl->framename_wd == 0)
      fhdl->framename_wd = 380;
   if (fhdl->framename_num == 0)
      fhdl->framename_num = 10;

   // Get the initialized file
   (void)init_get_win_filename(initname);

   // Get the initialized file-browser window position and its size
   if (init_get_val(initname, "FILE_BROWSER", "dddd", &x_pos, &y_pos,
       &name_width, &num_names) == NOT_OK)
   {
      // Default
      x_pos = y_pos = 0;
      name_width = 0;
      num_names = 0;
   }
   else
     fhdl->x = (x_pos - fhdl->framename_wd) - 50 ;
     fhdl->y = y_pos ;
   
   if ((fhdl->font = xv_find(fhdl->owner, FONT,
		FONT_FAMILY,	FONT_FAMILY_LUCIDA,
		FONT_STYLE,	FONT_STYLE_BOLD,
		NULL))
      == NULL)
      STDERR("frame_create_win: FONT_FAMILY_LUCIDA");

   /* Create pop-up frame */
   fhdl->popup = xv_create(fhdl->owner,	FRAME_CMD,
		FRAME_LABEL,		"Frame Browser",
		XV_X,			fhdl->x,
		XV_Y,			fhdl->y,
		XV_FONT,		fhdl->font,
		NULL);

   /* Get a panel from pop-up frame */
   fhdl->panel = xv_get(fhdl->popup,	FRAME_CMD_PANEL);

   /* Set the panel font */
   xv_set(fhdl->panel, XV_FONT, fhdl->font, NULL);

   x_pos = 12;
   y_pos = 6;

   fhdl->load_but = xv_create(fhdl->panel,	PANEL_BUTTON,
		XV_X,			x_pos,
		XV_Y,			y_pos,
		PANEL_LABEL_STRING,	"Load Playlist ...",
		PANEL_NOTIFY_PROC,	frame_load_proc,
		NULL);

   int x = (int)xv_get(fhdl->load_but, XV_WIDTH) + 4;
   fhdl->save_but = xv_create(fhdl->panel,	PANEL_BUTTON,
		XV_X,			x_pos + x,
		XV_Y,			y_pos,
		PANEL_LABEL_STRING,	"Save Playlist ...",
		PANEL_NOTIFY_PROC,	frame_save_proc,
		NULL);

   y_pos += DEFAULT_VERT_GAP;
   fhdl->delete_but = xv_create(fhdl->panel,	PANEL_BUTTON,
		XV_X,			x_pos,
		XV_Y,			y_pos,
		PANEL_LABEL_STRING,	"Delete",
		PANEL_NOTIFY_PROC,	frame_delete_proc,
		NULL);

   y_pos += DEFAULT_VERT_GAP;
   fhdl->framename_x = x_pos;
   fhdl->framename_y = y_pos;
   fhdl->dirlist = xv_create(fhdl->panel, PANEL_LIST,
			     XV_X, x_pos,
			     XV_Y, y_pos,
			     PANEL_LIST_WIDTH, fhdl->framename_wd,
			     PANEL_LIST_DISPLAY_ROWS, fhdl->framename_num,
			     PANEL_LIST_INSERT, 0,
			     PANEL_LIST_STRING, 0, "",
			     NULL);

   y_pos += (int)xv_get(fhdl->dirlist,	XV_HEIGHT) + 8;
   fhdl->msg = xv_create(fhdl->panel,	PANEL_MESSAGE,
		PANEL_LABEL_STRING,	"0 Frames",
		XV_X,			x_pos,
		XV_Y,			y_pos,
		NULL);
   
   window_fit(fhdl->panel);
   window_fit(fhdl->popup);
}

/************************************************************************
*									*
*  Destroy the object handlers.						*
*									*/
void
framelist_win_destroy(void)
{
   frame_free(framename, numframes);
   frame_free(framepath, numframes);
   frame_free(framevs, numframes);

//   filelist_notify_func(FILELIST_MOVIE_ID, FILELIST_DELETE,
//                         NULL, NULL);

   (void)xv_destroy_safe(fhdl->popup);
   (void)free((char *)fhdl);
   fhdl = NULL;

}

/************************************************************************
*									*
*  User register function to call when a new file/frame name is added   *
*  or the delete button is pressed.                                     *
*									*/
void
framelist_notify_func(long addr_okfunc)
{
   if (fhdl == NULL)
   {
      STDERR("framelist_notify_func:File browser window has NOT been created");
      return;
   }

   if (addr_okfunc == NULL)
     okfunc = (double (*)(char *, char *,int)) NULL ;
   else
     if (okfunc == NULL)
       okfunc = (double (*)(char *, char *,int))addr_okfunc;
     else
       STDERR
       ("framelist_notify_func:Callback function has already been registered");

}


/************************************************************************
*									*
*  Pop up the frame list frame.						*
*  The "dir_path" indicates go to the specified directory.  If it is	*
*  NULL, use the default directory (which is kept in Frameinfo).		*
*  The title name will be showed up in the title window. If it is NULL,	*
*  the title "Frame Browser" will be shown.				*
*									*/
void
framelist_win_show(char *title)	/* Title of frame-browser */
{

   /* Create framelist window if it is not created */
   if (fhdl->popup == NULL)
      frame_create_win();

   xv_set(fhdl->delete_but, PANEL_SHOW_ITEM, TRUE, NULL);

   /* Change the title */
   xv_set(fhdl->popup, FRAME_LABEL, title ? title : "Playlist", NULL);

   xv_set(fhdl->popup, 
	  FRAME_CMD_PUSHPIN_IN,	TRUE,
	  XV_SHOW, TRUE,
	  NULL);

   xv_set(fhdl->dirlist, PANEL_LIST_SELECT,numframes,TRUE,NULL);

}


/************************************************************************
*									*
*  User has clicked the "Delete" button.				*
*									*/
static void
frame_delete_proc(Panel_item, Event *)
{
  int selected_row, framenum ;

  selected_row = frame_get_select() ;
  if ((numframes>0)&&(selected_row>=0)&&(selected_row<numframes))
  {

    okfunc("delete", "delete", selected_row);
    free(framename[selected_row]); 
    free(framepath[selected_row]); 
    free(framevs[selected_row]); 
    for (framenum=selected_row; framenum<numframes-1; framenum++)
    {
      framename[framenum] = framename[framenum+1];
      framepath[framenum] = framepath[framenum+1];
      framevs[framenum] = framevs[framenum+1];
    }
    numframes-- ;
    frame_list_update();

    xv_set(fhdl->dirlist, PANEL_LIST_SELECT, selected_row, TRUE, NULL);
  }
}

/************************************************************************
* frame_listdir								*
* This is taken from listdir in gframe.c.  This routine recursively  	*
* steps through the directory given to find all the image files in it.	*
* It then loads these image files into the framelist.			*
*************************************************************************/
void frame_listdir(char* dirname) 
{
    char *name;

    for (name=filelist_first_file(); name; name=filelist_next_file()){
	while ( name[strlen(name)-1] == '/'){
	    name[strlen(name)-1] = '\0';	// Strip off terminating "/"
	}
	
	if (!strcmp(name, ".")) continue;
	if (!strcmp(name, "..")) continue;
	if (!strcmp(name, "core")) continue;
	if (!strcmp(name, "curpar")) continue;

	struct stat statbuf;
	struct stat procstat;

	char *filename = new char[strlen(dirname) + strlen(name) + 2];
	char *curpar = new char[strlen(dirname) + strlen(name)
				+ strlen("/curpar") + 2];
	strcpy(filename, dirname);
	strcat(filename, "/");
	strcat(filename, name);

	strcpy(curpar, filename);
	strcat(curpar, "/curpar");
	
	stat(filename, &statbuf);
	if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
	    // This is a directory
	    if (stat(curpar, &procstat) == 0) {
		// filename/curpar exists; load as old style data
		//cout << filename << " [DATA DIRECTORY]"<<endl;
		framelist_get_name(dirname,name);
	    }
	} else if (((statbuf.st_mode & S_IFMT) == S_IFREG)) {
	    // This is a regular file; load as DDL file
	    //cout << endl << "Should load:" << filename << " !!!" << endl;
	    framelist_get_name(dirname,name);
	}
	delete [] filename;
	delete [] curpar;
    }
}

/************************************************************************
* framelist_get_all							*
* Calls frame_listdir to go through the directory to find all the	*
* image files.								*
*************************************************************************/
int framelist_get_all(char *dirpath, char *)
{
  frame_listdir(dirpath);
  return TRUE;
}


/************************************************************************
*									*
*  Get path and filenames of selection made in filebrowser.		*
*                                                                	*
*  Return OK or NOT_OK.							*
*									*/
static int
framelist_get_name(char *name1, char *n2)
{
   int framenum ;        /* counter */
   int where ;           /* where in list to insert new name */
   double the_vs;
   char the_vs_string[80];
 
   if (numframes == (MAX_FRAME))
   {
     err_msg("Only can list %d maximum frames", MAX_FRAME);
     return(NOT_OK);
   }

   where = frame_get_select() ;
   if (where < 0)
     return (NOT_OK) ;

#ifdef DMALLOC
   cerr << "v1 ";
   malloc_verify();
#endif

   // call win_movie to read in the image in file "name1/n2" and 
   // insert it into the where-th position
   if ((the_vs=okfunc(name1, n2, where)) == -1.0){
       return NOT_OK;
   }

#ifdef DMALLOC
   cerr << "v2 ";
   malloc_verify();
   cerr << "\n";
#endif
   
   sprintf(the_vs_string, "%f\n", the_vs);

   if (where > numframes)
   {
     err_msg("Selected item out of range.");
     return(NOT_OK);
   }

   /* insert name(s) at where-th location in list(s)  */
   /* Start by shifting out one space everything from "where" to the */
   /* end of the list.  Start at the end. */
   for (framenum=numframes; framenum>where; framenum--){
       framename[framenum] = framename[framenum-1];
       framepath[framenum] = framepath[framenum-1];
       framevs[framenum] = framevs[framenum-1];
   }

   /* now put the new frame name(s) in place */
   if ((framename[where] = (char *)malloc(strlen(n2)+1)) == NULL)
     {    
       PERROR("framelist_get_name:malloc");
       frame_free(framename, numframes);
       frame_free(framepath, numframes);
       frame_free(framevs, numframes);
       return(NOT_OK);
     } 
   if ((framepath[where] = (char *)malloc(strlen(name1)+1)) == NULL)
     {    
       PERROR("framelist_get_name:malloc");
       frame_free(framename, numframes);
       frame_free(framepath, numframes);
       frame_free(framevs, numframes);
       return(NOT_OK);
     } 
   if ((framevs[where] = (char *)malloc(strlen(the_vs_string)+1)) == NULL)
     {    
       PERROR("framelist_get_name:malloc");
       frame_free(framename, numframes);
       frame_free(framepath, numframes);
       frame_free(framevs, numframes);
       return(NOT_OK);
     } 
   (void)sprintf(framename[where],"%s", n2);
   (void)sprintf(framepath[where],"%s", name1);
   (void)sprintf(framevs[where],"%s", the_vs_string);
   numframes++ ;
//   frame_list_update();
   xv_set(fhdl->dirlist,
	  PANEL_LIST_INSERT, where,
	  PANEL_LIST_STRING, where , framename[where],
	  NULL);

   xv_set(fhdl->dirlist, PANEL_LIST_SELECT, where+1, TRUE, NULL);

   /* Print number of frames */
   show_nframes(numframes);

   return(OK);
}

/************************************************************************
*									*
*  Display the number of frames at the bottom of the window
*									*/
static void
show_nframes(int numframes)
{
   char msg[60];

   if (numframes == 1)
     (void)sprintf(msg, "%d frame.", numframes);
   else
     (void)sprintf(msg, "%d frames.", numframes);
   xv_set(fhdl->msg, PANEL_LABEL_STRING, msg, NULL);
}


/************************************************************************
*									*
*  Update the panel frame list.						*
*									*/
static void
frame_list_update(void)
{
    int i;	/* loop counter */

    xv_set(fhdl->dirlist, PANEL_SHOW_ITEM, FALSE, NULL);

    xv_destroy_safe(fhdl->dirlist); // Destroy when safe to do so
    fhdl->dirlist = xv_create(fhdl->panel, PANEL_LIST,
			      XV_X, fhdl->framename_x,
			      XV_Y, fhdl->framename_y,
			      PANEL_LIST_WIDTH, fhdl->framename_wd,
			      PANEL_LIST_DISPLAY_ROWS, fhdl->framename_num,
			      PANEL_SHOW_ITEM, FALSE,
			      NULL);

   /* Put all files into panel list */
   for (i=0; i<numframes; i++)
   {
      xv_set(fhdl->dirlist,
	     PANEL_LIST_INSERT, i,
	     PANEL_LIST_STRING, i, framename[i],
	     NULL);
   }

   // Put in a trailing blank entry
   xv_set(fhdl->dirlist,
	  PANEL_LIST_INSERT,  numframes,
	  PANEL_LIST_STRING,  numframes,   "",
	  NULL);

    // Reveal the finished product
    xv_set(fhdl->dirlist, PANEL_SHOW_ITEM, TRUE, 0);
    
   /* Print number of frames */
   show_nframes(numframes);
}

/************************************************************************
*									*
*  Select (highlight) a list entry			*
*									*/
void
framelist_select(int which)
{
   xv_set(fhdl->dirlist, PANEL_LIST_SELECT, which, TRUE, NULL);
}


/************************************************************************
*									*
*  Get selected row number from list
*  Returns -1 if none are selected					* 
*									*/
static int
frame_get_select(void)
{
  int selected = -1 ;
  int b = FALSE ;

  while ((selected<numframes) && (!b))
  {
    selected++ ;
    b = (int) xv_get(fhdl->dirlist, PANEL_LIST_SELECTED, selected);
  }
  if (b)
    return (selected) ;
  else
    return (-1) ;
}

/************************************************************************
*									*
*  Free frames.								*
*									*/
static void
frame_free(char **frame_name, int nframe)
{
   while (nframe)
      (void)free(frame_name[--nframe]);
}

/************************************************************************
*									*
*  Bell.								*
*									*/
static void
ring_bell(void)
{
   XBell((Display *)xv_get(fhdl->popup, XV_DISPLAY), NULL);
}

/************************************************************************
*									*
*  Ouput error message into "msg" window.				*
*									*/
static void
err_msg(char *format, ...)
{
   va_list vargs;	   /* variable argument pointer */
   char errmsg[128];	   /* error buffer for message */
 
   ring_bell();

   va_start(vargs, format);
   (void)vsprintf(errmsg, format, vargs);
   va_end(vargs); 

   xv_set(fhdl->msg, PANEL_LABEL_STRING, errmsg, NULL);
}


/************************************************************************
*									*
*  Get playlist filename                                   	 	*
*                                                                	*
*									*/
static void
framelist_do_save(char *name1, char *n2)
{
  char outfilename[128]; 
  FILE *outfile ;
  int i;

//  filelist_notify_func(FILELIST_PLAYLIST_ID, FILELIST_DELETE,
//                        NULL, NULL);

  sprintf ( outfilename, "%s/%s", name1, n2 ) ;
  outfile = fopen (outfilename, "w");

  if (!outfile)  
  {
    msgerr_print ( "Framelist:could not open movie file\n" );
    return ;
  }
  fprintf(outfile, "magic=\"movie magic\";\n");
  fprintf(outfile, "int num_movies=%d;\n", numframes);
  fprintf(outfile, "char *movie={\n" );
  for (i=0; i<numframes; i++)
  {
    fprintf(outfile, "\"%s/%s\"", framepath[i], framename[i]);
    //printf("i = %d\n", i);
    if (i < numframes-1){
	fprintf(outfile, ",\n");
    }
  }
  fprintf(outfile, "};\n");
  fclose(outfile);

  // Update the FileList
  filelist_win_show (FILELIST_PLAYLIST_SAVE_ID, FILELIST_SAVE, NULL,
		     "Movie Playlist Saver" );
}


/************************************************************************
*									*
*  Get playlist filename                                   	 	*
*                                                                	*
*									*/
static void
framelist_do_load(char *name1, char *n2)
{
  char filepart[128], pathpart[128] ;
  int place, i, num_movies ;
  char infile[128]; 

  //fprintf(stderr,"framelist_do_load()\n");
//  filelist_notify_func(FILELIST_PLAYLIST_ID, FILELIST_DELETE,
//                        NULL, NULL);
  sprintf ( infile, "%s/%s", name1, n2 ) ;

  DDLSymbolTable *st = ParseDDLFile(infile);

  if (!st){
      printf ("Failed!!\n" );
      return;
  }

  //st->PrintSymbols();
  st->GetValue("num_movies", num_movies);

  char *movie[MAX_FRAME] ;
  for (i = 0 ; i < num_movies; i++) 
  {
    st->GetValue("movie", movie[i], i);
    // now break it up into two parts...
    sprintf (pathpart, "%s", movie[i]);
    place = strlen(pathpart) ;
    place-- ; place-- ;           // get past the '/' at the end if any
    while ((place) && (pathpart[place] != '/'))
      place-- ;
    if (place==0)  place-- ;
    strcpy (filepart, &(pathpart[place+1]));
    pathpart[place] = NULL ;

    framelist_get_name (pathpart, filepart);
  }
}

/************************************************************************
*									*
*  User has clicked the "Save" button.					*
*									*/
static void
frame_save_proc(Panel_item, Event *)
{
    static int filelist_created = FALSE;
    
    if ( ! filelist_created ){
	filelist_notify_func(FILELIST_PLAYLIST_SAVE_ID, FILELIST_NEW, NULL,
			     (long)&framelist_do_save);
	filelist_created = TRUE;
    }
    filelist_win_show (FILELIST_PLAYLIST_SAVE_ID, FILELIST_SAVE, NULL,
		       "Movie Playlist Saver" );
}


/************************************************************************
*									*
*  User has clicked the "Load" button.					*
*									*/
static void
frame_load_proc(Panel_item, Event *)
{
    static int filelist_created = FALSE;
    
    if ( ! filelist_created ){
	filelist_notify_func(FILELIST_PLAYLIST_LOAD_ID, FILELIST_NEW,
			     (long)&framelist_do_load, NULL);
	filelist_created = TRUE;
    }
    filelist_win_show (FILELIST_PLAYLIST_LOAD_ID, FILELIST_LOAD, NULL,
		       "Movie Playlist Loader" );
}

/************************************************************************
*									*
*  Replace a vs                           				*
*									*/
void
frame_replace_vs(int which_frame, double vs)
{
  sprintf(framevs[which_frame],"%f", vs);
  frame_list_update();
}

