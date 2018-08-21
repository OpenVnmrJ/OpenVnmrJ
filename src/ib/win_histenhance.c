/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

#ifndef lint
   static char *Sid = "Copyright (c) Varian Assoc., Inc.  All Rights Reserved.";
#endif (not) lint

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
*  Window routines related to enahnce images with histogram method:	*
*     Equalization							*
*     Hyperbolization							*
*     Low-Intensity							*
*     High-Intensity.							*
*  									*
*  Note that it works well only for low-resolution images.		*
*									*
*************************************************************************/
#include "stderr.h"
#include <xview/xview.h>
#include <xview/panel.h>
#include <math.h>
#include <memory.h>
#include "msgprt.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "roitool.h"
#include "zoom.h"
#include "initstart.h"
#include "convert.h"
#include "process.h"

extern void win_print_msg(char *, ...);

// Maximum histogram index value.  This is equal to the number of gray-
// level of image data for type TYPE_SHORT (12 bits/pixel).
#define	NUM_HIST_INDEX	4096

// Class used to create histogram-window controller
class Win_histenhance
{
   private:
      Frame frame;	// Parent
      Frame popup;	// Popup frame (subframe)

      static void done_proc(Frame);
      static void hist_execute(Panel_item item);
      static Imginfo *convert_data_to_short(Gframe *);

   public:
      Win_histenhance(void);
      ~Win_histenhance(void);
      void show_window() { xv_set(popup, XV_SHOW, TRUE, NULL); }
};

static Win_histenhance *winhist=NULL;

/************************************************************************
*                                                                       *
*  Show the histogram window.						*
*									*/
void
winpro_histenhance_show(void)
{
   if (winhist == NULL)
      winhist = new Win_histenhance;
   else
      winhist->show_window();
}

/************************************************************************
*                                                                       *
*  Creator of window.							*
*									*/
Win_histenhance::Win_histenhance(void)
{
   Panel panel;		// panel
   int xitempos;	// current panel item position
   int yitempos;	// current panel item position
   Panel_item item;	// Panle item
   int xpos, ypos;      // window position
   char initname[128];	// init file

   (void)init_get_win_filename(initname);

   // Get the position of the control panel
   if (init_get_val(initname, "WINPRO_HIST", "dd", &xpos, &ypos) == NOT_OK)
   {
      WARNING_OFF(Sid);
      xpos = 400;
      ypos = 40;
   }

   frame = xv_create(NULL, FRAME, NULL);

   popup = xv_create(frame, FRAME_CMD,
	XV_X,		xpos,
	XV_Y,		ypos,
	FRAME_LABEL,	"Histogram-Enhancement",
	FRAME_DONE_PROC,	&Win_histenhance::done_proc,
	FRAME_CMD_PUSHPIN_IN,	TRUE,
	NULL);
   
   panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);

   xitempos = 5;
   yitempos = 5;
   item = xv_create(panel,	PANEL_BUTTON,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"Equalization",
		PANEL_CLIENT_DATA,	HIST_EQUALIZATION,
		PANEL_NOTIFY_PROC,	&Win_histenhance::hist_execute,
		NULL);
   yitempos += (int)xv_get(item, XV_HEIGHT) + 7;
   item = xv_create(panel,	PANEL_BUTTON,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"Hyperbolization",
		PANEL_CLIENT_DATA,	HIST_HYPERBOLIZATION,
		PANEL_NOTIFY_PROC,	&Win_histenhance::hist_execute,
		NULL);
   xitempos += (int)xv_get(item, XV_WIDTH) + 8;
   yitempos = 5;
   item = xv_create(panel,	PANEL_BUTTON,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"Low Intensity",
		PANEL_CLIENT_DATA,	HIST_LOWINTENSITY,
		PANEL_NOTIFY_PROC,	&Win_histenhance::hist_execute,
		NULL);
   yitempos += (int)xv_get(item, XV_HEIGHT) + 7;
   item = xv_create(panel,	PANEL_BUTTON,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"High Intensity",
		PANEL_CLIENT_DATA,	HIST_HIGHINTENSITY,
		PANEL_NOTIFY_PROC,	&Win_histenhance::hist_execute,
		NULL);
   /*
   xitempos = 7;
   yitempos += (int)xv_get(item, XV_HEIGHT) + 10;
   apply_item = xv_create(panel,	PANEL_CHOICE,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"Apply To",
		PANEL_CHOICE_STRINGS,	"A selected frame",
					"All selected frames",
					NULL,
		NULL);
   */

   window_fit(panel);
   window_fit(popup);
   window_fit(frame);
   xv_set(popup, XV_SHOW, TRUE, NULL);
}

/************************************************************************
*                                                                       *
*  Destructor of window.						*
*									*/
Win_histenhance::~Win_histenhance(void)
{
   xv_destroy_safe(frame);
}

/************************************************************************
*                                                                       *
*  Dismiss the popup window.						*
*  (STATIC)								*
*									*/
void
Win_histenhance::done_proc(Frame subframe)
{
   xv_set(subframe, XV_SHOW, FALSE, NULL);
   delete winhist;
   winhist = NULL;
   win_print_msg("Histogram: Exit");
}

/************************************************************************
*                                                                       *
*  Process histogram.							*
*  (STATIC)								*
*									*/
void
Win_histenhance::hist_execute(Panel_item item)
{
   Gframe *srcframe, *dstframe;	// source and destination frames
   int hist[NUM_HIST_INDEX];   	// histogram value
   char errmsg[128];		// error message buffer
   register int i;   	 	// loop counter
   float *data1;
   float *data2;
   double min, max, median, mean, sdv, area;

   // Note that assign the first item of the frame containing data to
   // be the source.  Then Assign second item of the frame to be the
   // destination.  If only 1 frame is selected, source and destination
   // are the same.
   srcframe = Frame_select::get_selected_frame(1);

   if (srcframe == NULL)
   {
      msgerr_print("No frame is selected.");
      return;
   }
   else if (srcframe->imginfo == NULL)
   {
      msgerr_print("Source frame doesn't contain an image");
      return;
   }

   // Check if source frame contains ROI
   Roitool *roitool;
   roitool = Roitool::get_selected_tool();
   //roitool = Roi_routine::get_roi_tool();

   // ?????????????
   if (!roitool || (srcframe != roitool->owner_frame)){
       msgerr_print("Source frame doesn't contain selected ROI");
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

   if (roitool && (srcframe == roitool->owner_frame)){
       min = max = median = mean = sdv = area = 0;
       roitool->histostats(hist, NUM_HIST_INDEX,
			   &min, &max,
			   &median, &mean, &sdv, &area);
/*
     {
	 msgerr_print("Histogram: Stop processing");
	 win_print_msg("Histogram: Error");

         return;
      }
*/
   }else{
       // Allocate memory and build a pixel image with type of 'short'

       // Initialize hist value to be 0 and build histogram
   }

   win_print_msg("Histogram: Calculating histogram lookup table.....");

   if (pro_histenhance((Histtype)panel_get(item, PANEL_CLIENT_DATA), hist,
		       hist, NUM_HIST_INDEX, errmsg) == ERROR)
   {
       msgerr_print(errmsg);
       return;
   }

   win_print_msg("Histogram: Processing data ....");

   // Convert the data through histogram look-up table
   int index;
   float scale = (max - min) / (NUM_HIST_INDEX - 1);
   int data_len = srcframe->imginfo->GetFast() * srcframe->imginfo->GetMedium();
   for (i=0; i<data_len; i++){
       index = (int)( (*data1++ - min) / scale + 0.5);
       if (index < 0){
	   *data2++ = 0;
       }else if (index > (NUM_HIST_INDEX - 1)){
	   *data2++ = max;
       }else{
	   *data2++ = hist[index] * scale + min;
       }
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

   win_print_msg("Histogram: Done.");
}

/************************************************************************
*                                                                       *
*  Allocate memory for Imginfo and data and convert gframe's data 	*
*  to type of short with 12 bits and put the result into Sisinfo_file's	*
*  data.								*
*  Return address pointer or NULL.					*
*  (STATIC)								*
*									*/
Imginfo*
Win_histenhance::convert_data_to_short(Gframe *gframe)
{
  char err[128];	// error buffer
  int allocate_data;
  
  Imginfo *imginfo = NULL;
  Imginfo *srcimginfo = gframe->imginfo;

  imginfo = new Imginfo((Sisfile_rank) srcimginfo->GetRank(),
			BIT_12, TYPE_SHORT,
			srcimginfo->GetFast(), srcimginfo->GetMedium(), 1, 1,
			allocate_data = TRUE);
  
  if (imginfo == NULL)   {
    msgerr_print("Couldn't allocate memory:%s", err);
    return(NULL);
  }
   
   switch (srcimginfo->type)
   {
      case TYPE_FLOAT:
         convert_float_to_short((float *)gframe->imginfo->GetData(),
	    (short *)imginfo->GetData(), imginfo->GetFast() * 
	    imginfo->GetMedium(), gframe->imginfo->vs * 
	    NUM_HIST_INDEX / G_Get_Sizecms2(Gframe::gdev), 
	    NUM_HIST_INDEX-1);
	 break;

      case TYPE_SHORT:
         memcpy(imginfo->GetData(), gframe->imginfo->GetData(),
		imginfo->GetFast() * imginfo->GetMedium() * sizeof(short));
         break;

      default:
	 msgerr_print("This data TYPE (%d) is not supported",
		      srcimginfo->type);
	 if (imginfo) delete imginfo;
	 return(NULL);
   }
   return(imginfo);
}
