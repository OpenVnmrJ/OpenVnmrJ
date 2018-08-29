/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

static char *Sid() {
    return "@(#)win_filter.c 18.1 03/21/08 (c)1992 SISCO";
}

/************************************************************************
*									*
*  Charly Gatot								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  Window routines related filtering (Convolution filter with N x N)	*
*									*
*************************************************************************/
#include "stderr.h"
#include <stream.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <unistd.h>
#include "msgprt.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "initstart.h"
#include "convert.h"
#include "process.h"
#include "interrupt.h"
#include "filelist_id.h"

extern void win_print_msg(char *, ...);

// Class used to create filter controller
class Win_filter
{
   private:
      Frame frame;	// Parent
      Frame popup;	// Popup frame (subframe)
      Panel_item mask_item;
      Panel_item mean_item;
      Panel_item filt_item[5][5];

      static void done_proc(Frame);
      static Menu menu_load_pullright(Menu_item, Menu_generate);
      static void menu_save_notify(void);
      static void load_mask(char *, char *);
      static void save_mask(char *, char *);
      static void filter_execute(void);
      static void mask_proc(Panel_item, int);
      static void func_notify_stat(int);

   public:
      Win_filter(void);
      ~Win_filter(void);
      void show_window() { xv_set(popup, XV_SHOW, TRUE, NULL); }
};

static int current_mask = 0;	// current mask 0:(3x3), 1:(5x5) value
static Win_filter *winfilter=NULL;

/************************************************************************
*                                                                       *
*  Show the window.							*
*									*/
void
winpro_filter_show(void)
{
   if (winfilter == NULL)
      winfilter = new Win_filter;
   else
      winfilter->show_window();
}

/************************************************************************
*                                                                       *
*  Creator of window.							*
*									*/
Win_filter::Win_filter(void)
{
   Panel panel;		// panel
   int xitempos;	// current panel item position
   int yitempos;	// current panel item position
   Panel_item item;	// Panle item
   int xpos, ypos;      // window position
   char initname[128];	// init file
   int i, j;		// loop counter
   int item_width;
   Menu menu;		// menu;

   (void)init_get_win_filename(initname);

   // Get the position of the control panel
   if (init_get_val(initname, "WINPRO_FILTER", "dd", &xpos, &ypos) == NOT_OK)
   {
      xpos = 400;
      ypos = 20;
   }

   frame = xv_create(NULL, FRAME, NULL);

   popup = xv_create(frame, FRAME_CMD,
	XV_X,		xpos,
	XV_Y,		ypos,
	FRAME_LABEL,	"Filter",
	FRAME_DONE_PROC,	&Win_filter::done_proc,
	FRAME_CMD_PUSHPIN_IN,	TRUE,
	NULL);
   
   panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);

   xitempos = 5;
   yitempos = 5;
   item = xv_create(panel,	PANEL_BUTTON,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"Apply",
		PANEL_NOTIFY_PROC,	&Win_filter::filter_execute,
		NULL);
   yitempos += (int)xv_get(item, XV_HEIGHT) + 8;
   mask_item = xv_create(panel,	PANEL_CHOICE_STACK,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"Mask",
		PANEL_CHOICE_STRINGS,	" 3 x 3 ",
					" 5 x 5 ",
					NULL,
		PANEL_VALUE,		current_mask,
		PANEL_NOTIFY_PROC,	&Win_filter::mask_proc,
		NULL);

   yitempos += (int)xv_get(mask_item, XV_HEIGHT) + 8;
   mean_item =
   xv_create(panel, PANEL_CHOICE,
	     XV_X, xitempos,
	     XV_Y, yitempos,
	     PANEL_CHOICE_STRINGS, "Mean", "Median", NULL,
	     PANEL_VALUE, 0,
	     NULL);

   // Create Menu
   menu = xv_create(NULL, MENU_COMMAND_MENU,
	MENU_ITEM,
		MENU_STRING,		"Save...",
		MENU_NOTIFY_PROC,	&Win_filter::menu_save_notify,
		NULL,
	MENU_ITEM,
		MENU_STRING,		"Load",
		MENU_GEN_PULLRIGHT,     &Win_filter::menu_load_pullright,
		NULL,
	 NULL);

   yitempos += (int)xv_get(mean_item, XV_HEIGHT) + 8;
   item = xv_create(panel, PANEL_BUTTON,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"Mask-Files",
		PANEL_ITEM_MENU,	menu,
		NULL);

   xitempos = (int)xv_get(mask_item, XV_WIDTH) + 10; 
   yitempos = 5;
   item_width = 50;
   for (i=0; i<5; i++)
   {
      for (j=0; j<5; j++)
      {
	 filt_item[i][j] = xv_create(panel,     PANEL_TEXT,
		XV_X,	xitempos + i * item_width,
		XV_Y,	yitempos + j * 15,
		PANEL_VALUE_DISPLAY_LENGTH,	8,
		PANEL_VALUE_STORED_LENGTH,	20,
		PANEL_VALUE_UNDERLINED,	TRUE,
		NULL);
	 if (i == 0 && j == 0) {
	     item_width = (int)xv_get(filt_item[0][0], XV_WIDTH) + 5;
	 }
      }
   }

   window_fit(panel);
   window_fit(popup);
   window_fit(frame);

   // Hide some of the filter item for 3 x 3
   if (current_mask == 0)
   {
      for (i=0; i<5; i++)
      {
         for (j=0; j<5; j++)
         {
	    if ((i > 2) || (j > 2))
	       xv_set(filt_item[i][j], PANEL_SHOW_ITEM, FALSE, NULL);
	 }
      }
   }

   // Register the notify function to save a mask
   filelist_notify_func(FILELIST_FILTER_ID, FILELIST_NEW,
	  NULL, (long)&Win_filter::save_mask);

   /* Set the default directory */
   char pathname[1024];
   (void)init_get_env_name(pathname);
   (void)strcat(pathname, "/filter");
   filelist_set_directory(FILELIST_FILTER_ID, pathname);
   
   xv_set(popup, XV_SHOW, TRUE, NULL);
}

/************************************************************************
*                                                                       *
*  Destructor of window.						*
*									*/
Win_filter::~Win_filter(void)
{
   xv_destroy_safe(frame);
}

/************************************************************************
*                                                                       *
*  Dismiss the popup window.						*
*  (STATIC)								*
*									*/
void
Win_filter::done_proc(Frame subframe)
{
   xv_set(subframe, XV_SHOW, FALSE, NULL);
   filelist_notify_func(FILELIST_FILTER_ID, FILELIST_DELETE, NULL, NULL);
   delete winfilter;
   winfilter = NULL;
   win_print_msg("Filter: Exit");
}

/************************************************************************
*                                                                       *
*  Popup the file-browser to save amask values into a file.		*
*  (STATIC)								*
*									*/
void
Win_filter::menu_save_notify(void)
{
   char pathname[1024];
   (void)init_get_env_name(pathname);
   (void)strcat(pathname, "/filter");
   filelist_win_show(FILELIST_FILTER_ID, FILELIST_SAVE, pathname,
		     "File_Browser: Filter");
}

/************************************************************************
*                                                                       *
*  Popup the pull-down menu to load the mask.				*
*  (STATIC)								*
*									*/
Menu
Win_filter::menu_load_pullright(Menu_item mi, Menu_generate op)
{
   if (op == MENU_DISPLAY)
   {
      char pathname[128];   		// pathname

      (void)init_get_env_name(pathname);
      (void)strcat(pathname, "/filter");
      return(filelist_menu_pullright(mi, op,
	     (u_long)&Win_filter::load_mask, pathname));
   }
   return(filelist_menu_pullright(mi, op, NULL, NULL));
}

/************************************************************************
*                                                                       *
*  Save mask values into a file.					*
*  The result of the output file contains the following format:		*
*									*
*	# <comments>							*
*	<n> x <n>							*
*       X11 ... X1n							*
*	...								*
*	Xn1 ... Xnn							*
*									*
*   where								*
*	# indicates comments						*
*	n is the mask size n x n					*
*	X11 .. Xnn is the mask entry values.				*
*									*
*  (STATIC)								*
*									*/
void
Win_filter::save_mask(char *dirpath, char *name)
{
   ofstream outfile;    // output stream
   char filename[128];  // complete filename
   long clock;          // number of today seconds
   char *tdate;         // pointer to the time
   char *tlogin;        // pointer to login name
   char thost[80];      // hostname buffer
   int filter[5][5];	// filter buffer
   int filter_size;	// filter size
   int i, j;		// loop counter
   char *ptr;		// pointer to the mask value

   // Make sure user has specified the output filename
   if (*name == NULL)
   {
      msgerr_print("Need to specify output filename for saving");
      return;
   }

   (void)sprintf(filename, "%s/%s", dirpath, name);
   outfile.open(filename, ios::out);
   if (outfile.fail())
   {
      msgerr_print("Couln'd open ``%s'' for writing", filename);
      return;
   }
    
   // Get the mask
   if ((int)xv_get(winfilter->mask_item, PANEL_VALUE) == 0)
      filter_size = 3;		// 3 x 3
   else	
      filter_size = 5;		// 5 x 5

   // Get the mask entry value
   for (i=0; i<filter_size; i++)
   {
      for (j=0; j<filter_size; j++)
      {
	 ptr = (char *)xv_get(winfilter->filt_item[i][j], PANEL_VALUE);
	 if (*ptr == NULL)
	 {
	    msgerr_print("No entry value at (%d x %d)", i, j);
	    return;
	 }

	 filter[i][j] = atoi(ptr);
      }
   }

   // Output the comments
   clock = time(NULL);
   if ((tdate = ctime(&clock)) != NULL)
      tdate[strlen(tdate)-1] = 0;
   if ((tlogin = (char *)cuserid(NULL)) == NULL)
      msgerr_print("Warning: Couldn't find login name");
   if (gethostname(thost,80) != 0)
      msgerr_print("Warning:Couldn't find host name");
    
   outfile << "# ** Created by " << tlogin << " on " << tdate
           << " at machine " << thost << " **" << "\n";
   outfile << "# filter: mask size, followed by mask entry values \n" ;
   outfile << filter_size << " x " << filter_size << "\n";
   for (i=0; i<filter_size; i++)
   {
      for (j=0; j<filter_size; j++)
	 outfile << filter[i][j] << " ";
      outfile << "\n";
   }
   outfile.close();

   filelist_update();
}

/************************************************************************
*                                                                       *
*  Load the mask from a file with format as described in		*
*  Win_filter::save_mask()						*
*									*/
void
Win_filter::load_mask(char *dirpath, char *name)
{
   ifstream infile;     // input stream
   char filename[128];  // complete filename
   char buf[128];       // input buffer
   int filter[5][5];	// filter buffer
   int filter_size;	// filter size
   int size_index;	// 0==>3x3, 1==>5x5
   int i, j;		// loop counter
   int mask_used;	// mask used 3 x 3 or 5 x 5
   char filtbuf[10];	// filter buffer

   // Make sure user has specified the input filename
   if (*name == NULL)
   {
      msgerr_print("Need to specify input filename for loading");
      return;
   }

   (void)sprintf(filename, "%s/%s", dirpath, name);

   infile.open(filename, ios::in);
   if (infile.fail())
   {
      msgerr_print("Couln'd open ``%s'' for reading", filename);
      return;
   }
   
   mask_used = (int)xv_get(winfilter->mask_item, PANEL_VALUE);/*CMP*/

   // Skip all '#'
   while (infile.getline(buf, 128))
   {
      if (buf[0] != '#')
	break;
   }

   if (sscanf(buf, "%d", &filter_size) != 1)
   {
      msgerr_print("No filter-size specified in %s", filename);
      infile.close();
      return;
   }

   // Set the filter size
   switch (filter_size){
     case 3:
       size_index = 0;
       break;
     case 5:
       size_index = 1;
       break;
     default:
       msgerr_print("Filter size is %dx%d; only 3x3 and 5x5 are allowed",
		    filter_size, filter_size);
       infile.close();
       return;
   }
   xv_set(winfilter->mask_item, PANEL_VALUE, size_index, NULL);
   mask_proc((Panel_item)0, size_index);

   // Read the mask-values
   for (i=0; i<filter_size; i++)
   {
      for (j=0; j<filter_size; j++)
      {
	 if ((infile >> filter[i][j]) == NULL)
	 {
            msgerr_print("Not enough data size in %s", filename);
            infile.close();
            return;
	 }
      }
   }
   infile.close();

   // Set the mask values
   for (i=0; i<filter_size; i++)
   {
      for (j=0; j<filter_size; j++)
      {
	 (void)sprintf(filtbuf, "%d", filter[i][j]);
	 xv_set(winfilter->filt_item[i][j], PANEL_VALUE, filtbuf, NULL);
      }
   }
}

/************************************************************************
*                                                                       *
*  Turn on/off constant value item.					*
*  (STATIC)								*
*									*/
void
Win_filter::mask_proc(Panel_item item, int val)
{
   int i, j;	// loop counter

   WARNING_OFF(item);
   current_mask = val;
   Flag flag = (current_mask == 1) ? TRUE : FALSE;

   for (i=0; i<5; i++)
   {
      for (j=0; j<5; j++)
      {
	 if ((i > 2) || (j > 2))
	    xv_set(winfilter->filt_item[i][j], PANEL_SHOW_ITEM, flag, NULL);
      }
   }
}

/************************************************************************
*                                                                       *
*  Print status message of data has been processed.  It is called 	*
*  every certain interval during processing filter.			*
*  (STATIC)								*
*									*/
void
Win_filter::func_notify_stat(int stat)
{
   win_print_msg("filter: processing . . . %d%%", stat);
}

/************************************************************************
*                                                                       *
*  Process filter.							*
*  If the data type is TYPE_FLOAT, it will convert it to TYPE_SHORT.	*
*  (STATIC)								*
*									*/
void
Win_filter::filter_execute(void)
{
   Gframe *srcframe, *dstframe; // source and destination frames
   char errmsg[1024];
   int mask_used;		// use 3x3 or 5x5
   int i,j ;			// loop counter
   char *ptr;			// String giving a filter weighting factor
   int filter_size;		// mask value (either 3 or 5)
   float filter[25];		// filter buffer
   float *data1;
   float *data2;


   // Get the mask
   if ((mask_used = (int)xv_get(winfilter->mask_item, PANEL_VALUE)) == 0)
      filter_size = 3;		// 3 x 3
   else	
      filter_size = 5;		// 5 x 5

   // Get the mask entry value
   for (i=0; i<filter_size; i++)
   {
      for (j=0; j<filter_size; j++)
      {
	 if ((ptr = (char *)xv_get(winfilter->filt_item[i][j], PANEL_VALUE))
	    == NULL)
	 {
	    msgerr_print("No entry value at (%d, %d)", i, j);
	    return;
	 }

	 filter[i*filter_size + j] = atof(ptr);
      }
   }

   // Assign the first item of the frame containing to be the source
   // and the second item to be the destination. If only one frame
   // is selected, source and destination will be the same frame.
   if ((srcframe = Frame_select::get_selected_frame(1)) == NULL)
     {
       msgerr_print("No frame is selected.");
       return;
     }
   
   if (srcframe->imginfo == NULL)
     {
       msgerr_print("Data source doesn't contain image");
       return;
     }

   if ((dstframe = Frame_select::get_selected_frame(2)) == NULL){
       dstframe = srcframe;
   }else if (Frame_select::get_selected_frame(3)){
       // Only let maximum two selected frames at a time
       msgerr_print("Maximum 2 frames can be selected");
       return;
   }

   data1 = (float *)srcframe->imginfo->GetData();
   if (srcframe == dstframe){
       data2 = data1;
   }else{
       // Result goes in a second frame
       // Release any old image data
       if (dstframe->imginfo){
	   dstframe->clear();
       }
       detach_imginfo(dstframe->imginfo);
       
       // Make a copy of the image
       dstframe->imginfo = new Imginfo(srcframe->imginfo);
       data2 = (float *)dstframe->imginfo->GetData();
   }

   win_print_msg("Filter: processing .......");

   interrupt_begin();	// Start detecting interrupt

   int do_median = (int)xv_get(winfilter->mean_item, PANEL_VALUE);

   *errmsg = '\0';
   int ret = filter_mask(data1, data2,
			 srcframe->imginfo->GetFast(),
			 srcframe->imginfo->GetMedium(),
			 filter,
			 filter_size,		// Filter width
			 filter_size,		// Filter height
			 interrupt, // Pointer to interrupt checker function
			 &Win_filter::func_notify_stat,
			 errmsg,
			 do_median);

   interrupt_end();	// End detecting interrupt

   if (ret == ERROR){
       if (*errmsg){
	   msgerr_print(errmsg);
	   win_print_msg("Filter: Error.");
       }else{
	   win_print_msg("Filter: Cancelled.");
       }
       return;
   }
    
    // Need to update the display
    if (dstframe->imginfo->pixmap){
	// Free old pixmap
	XFreePixmap(dstframe->imginfo->display, dstframe->imginfo->pixmap);
	dstframe->imginfo->pixmap = 0;
    }

    Frame_data::display_data(dstframe,
			     dstframe->imginfo->datastx,
			     dstframe->imginfo->datasty,
			     dstframe->imginfo->datawd,
			     dstframe->imginfo->dataht,
			     dstframe->imginfo->vs,
			     FALSE);

   win_print_msg("Filter: Done.");
}
