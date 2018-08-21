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
    return "@(#)win_arithmetic.c 18.1 03/21/08 (c)1991-92 SISCO";
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
*  Window routines related to arithmetic operation.			*
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
#include "zoom.h"
#include "initstart.h"
#include "convert.h"
#include "process.h"

extern void win_print_msg(char *, ...);

// Class used to create arithmetic controller
class Win_arith
{
   private:
      typedef enum
      {
	 ARITH_ADDITION,
	 ARITH_SUBTRACTION,
	 ARITH_MULTIPLICATION,
	 ARITH_DIVISION
      } Arithtype;

      Frame frame;	// Parent
      Frame popup;	// Popup frame (subframe)
      Panel_item apply_item;
      Panel_item constant_item;

      static void done_proc(Frame);
      static void arith_execute(Panel_item);
      static void apply_proc(Panel_item, int);
      static int operation_two_images(Arithtype, Gframe *, Gframe *, 
	 Gframe *);
      static int operation_image_and_const(Arithtype, Gframe *, float,
	 Gframe *);

   public:
      Win_arith(void);
      ~Win_arith(void);
      void show_window() { xv_set(popup, XV_SHOW, TRUE, NULL); }
};

static Win_arith *winarith=NULL;

/************************************************************************
*                                                                       *
*  Show the histogram window.						*
*									*/
void
winpro_arithmetic_show(void)
{
   // Create window object only if it is not created
   if (winarith == NULL)
      winarith = new Win_arith;
   else
      winarith->show_window();
}

/************************************************************************
*                                                                       *
*  Creator of window.							*
*									*/
Win_arith::Win_arith(void)
{
   Panel panel;		// panel
   int xitempos;	// current panel item position
   int yitempos;	// current panel item position
   Panel_item item;	// Panle item
   int xpos, ypos;      // window position
   char initname[128];	// init file

   // Get the initialized file of window position
   (void)init_get_win_filename(initname);

   // Get the position of the control panel
   if (init_get_val(initname, "WINPRO_ARITH", "dd", &xpos, &ypos) == NOT_OK)
   {
      xpos = 400;
      ypos = 40;
   }

   frame = xv_create(NULL, FRAME, NULL);

   popup = xv_create(frame, FRAME_CMD,
	XV_X,		xpos,
	XV_Y,		ypos,
	FRAME_LABEL,	"Arithmetic",
	FRAME_DONE_PROC,	&Win_arith::done_proc,
	FRAME_CMD_PUSHPIN_IN,	TRUE,
	NULL);
   
   panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);

   xitempos = 5;
   yitempos = 5;
   item = xv_create(panel,	PANEL_BUTTON,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"Add",
		PANEL_CLIENT_DATA,	ARITH_ADDITION,
		PANEL_NOTIFY_PROC,	&Win_arith::arith_execute,
		NULL);
   xitempos += (int)xv_get(item, XV_WIDTH) + 5;
   item = xv_create(panel,	PANEL_BUTTON,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"Subtract",
		PANEL_CLIENT_DATA,	ARITH_SUBTRACTION,
		PANEL_NOTIFY_PROC,	&Win_arith::arith_execute,
		NULL);
   xitempos += (int)xv_get(item, XV_WIDTH) + 5;
   item = xv_create(panel,	PANEL_BUTTON,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"Multiply",
		PANEL_CLIENT_DATA,	ARITH_MULTIPLICATION,
		PANEL_NOTIFY_PROC,	&Win_arith::arith_execute,
		NULL);
   xitempos += (int)xv_get(item, XV_WIDTH) + 5;
   item = xv_create(panel,	PANEL_BUTTON,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"Divide",
		PANEL_CLIENT_DATA,	ARITH_DIVISION,
		PANEL_NOTIFY_PROC,	&Win_arith::arith_execute,
		NULL);
   xitempos = 7;
   yitempos += (int)xv_get(item, XV_HEIGHT) + 10;
   apply_item = xv_create(panel,	PANEL_CHOICE_STACK,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"Operands:",
	        PANEL_CHOICE_STRINGS,	"Image <op> Image",
			  		"Image <op> Constant",
					NULL,
		PANEL_NOTIFY_PROC,	&Win_arith::apply_proc,
		NULL);

   xitempos += (int)xv_get(apply_item, XV_WIDTH) + 5;
   yitempos += 5;
   constant_item = xv_create(panel,	PANEL_TEXT,
		XV_X,		xitempos,
		XV_Y,		yitempos,
		PANEL_LABEL_STRING,	"",
		PANEL_VALUE,		"0",
		PANEL_VALUE_DISPLAY_LENGTH, 6,
		PANEL_SHOW_ITEM, FALSE,
		NULL);

   window_fit(panel);
   window_fit(popup);
   window_fit(frame);
   xv_set(popup, XV_SHOW, TRUE, NULL);
}

/************************************************************************
*                                                                       *
*  Destructor of window.						*
*									*/
Win_arith::~Win_arith(void)
{
   xv_destroy_safe(frame);
}

/************************************************************************
*                                                                       *
*  Dismiss the popup window.						*
*  (STATIC)								*
*									*/
void
Win_arith::done_proc(Frame subframe)
{
   xv_set(subframe, XV_SHOW, FALSE, NULL);
   delete winarith;
   winarith = NULL;
   win_print_msg("Arithmetic: Exit");
}

/************************************************************************
*                                                                       *
*  Turn on/off constant value item.					*
*  (STATIC)								*
*									*/
void
Win_arith::apply_proc(Panel_item, int val)
{
   if (val == 0){
       xv_set(winarith->constant_item, PANEL_SHOW_ITEM, FALSE, NULL);
   }else{
       xv_set(winarith->constant_item, PANEL_SHOW_ITEM, TRUE, NULL);
   }
}

/************************************************************************
*                                                                       *
*  Process arithmetic.							*
*  (STATIC)								*
*									*/
void
Win_arith::arith_execute(Panel_item item)
{
   Gframe *src1, *src2, *dst;	// source and destination frames
   int apply_on;		// 0: image <op> contsant
				// 1: image <op> image
   Arithtype optype;		// operation type

   apply_on = (int)xv_get(winarith->apply_item, PANEL_VALUE);

   // For image & constant, assign the first item of the frame containing 
   // data to be the source.  Then assign second item of the frame to be
   // the destination.  If only 1 frame is selected, source and 
   // destination are the same.  For 2 images, assign the first two frames
   // to be sources and the third one to be destination.  If only two
   // frames are selected, assign the second one to be destination
   // (Note that the first, second, third items depend on the order
   // you select the frame.)

   src1 = Frame_select::get_selected_frame(1);

   if (src1 == NULL)
   {
      msgerr_print("arith_execute:No frame is being selected.");
      return;
   }
   else if (src1->imginfo == NULL)
   {
      msgerr_print("arith_execute:Source 1 doesn't contain image");
      return;
   }

   if (apply_on == 1)	// image <op> constant
   {
      if ((dst = Frame_select::get_selected_frame(2)) == NULL)
         dst = src1;
      
      // Only let maximum two selected frame active at a time
      if (Frame_select::get_selected_frame(3))
      {
	 msgerr_print("arith_execute:Maximum 2 frames can be selected at this process");
	 return;
      }
   }
   else			// image <op> image
   {
      if ((src2 = Frame_select::get_selected_frame(2)) == NULL)
      {
         msgerr_print("arith_execute:Source 2 is not selected");
	 return;
      }
      else if (src2->imginfo == NULL)
      {
         msgerr_print("arith_execute:Source 2 doesn't contain image");
         return;
      }

      if ((dst = Frame_select::get_selected_frame(3)) == NULL)
         dst = src2;

      // Only let maximum three selected frame active at a time
      if (Frame_select::get_selected_frame(4))
      {
	 msgerr_print("arith_execute:Maximum 3 frames can be selected at this process");
	 return;
      }
   }

   optype = (Win_arith::Arithtype)xv_get(item, PANEL_CLIENT_DATA);

   // Processing beginnin
   win_print_msg("Arithmetic: Processing.......");

   if (apply_on == 1)
   {
      float const_val = (float)atof((char *)xv_get(winarith->constant_item, 
			   PANEL_VALUE));
      operation_image_and_const(optype, src1, const_val, dst);
   }
   else
      operation_two_images(optype, src1, src2, dst);

   // Display the resul image in the destination frame.  Note that we
   // obtain its information (starting data pointm width and height of
   // data) from source image.
   Frame_data::display_data(dst, src1->imginfo->datastx,
         src1->imginfo->datasty, src1->imginfo->datawd,
         src1->imginfo->dataht, dst->imginfo->vs);

   win_print_msg("Arithmetic: Done.");
}

/************************************************************************
*                                                                       *
*  Operate arithmetic on an image with a constant.			*
*  Note that the constant value needs to be rationalize with the 	*
*  vertical scale before operation takes place.				*
*  Return SUCCESS or ERROR.						*
*  (STATIC)								*
*									*/
int
Win_arith::operation_image_and_const(Arithtype optype,
   Gframe *src, float const_val, Gframe *dst)
{
    char err[128];		// error message buffer
    int ret;			// return value

    Imginfo *imginfo = NULL;    // Destination image
    Imginfo *srcimginfo = src->imginfo;
    float *data1 = (float *)srcimginfo->GetData();
    float *data2;
    int data_len = srcimginfo->GetFast() * srcimginfo->GetMedium();
    if (src == dst){
	data2 = data1;
    }else{
	// Result goes in a second frame
	// Release old image data
	if (dst->imginfo){
	    dst->clear();
	}
	detach_imginfo(dst->imginfo);

	imginfo = new Imginfo(srcimginfo);
	data2 = (float *)imginfo->GetData();
	dst->imginfo = imginfo;
    }

    switch (optype){
      
    case ARITH_ADDITION:
      ret = arith_fadd_image_const(data1, const_val, data2, data_len, err);
      break;
      
    case ARITH_SUBTRACTION:
      ret = arith_fsub_image_const(data1, const_val, data2, data_len, err);
      break;
      
    case ARITH_MULTIPLICATION:
      ret = arith_fmul_image_const(data1, const_val, data2, data_len, err);
      break;
      
    case ARITH_DIVISION:
      ret = arith_fdiv_image_const(data1, const_val, data2, data_len, err);
      break;
  }

  if (ret == ERROR){
    //if (imginfo) delete imginfo;
    msgerr_print("operation_image_and_const:%s",err);
    return(ERROR);
  }
    
    // Need to update the display
    if (dst->imginfo->pixmap){
	// Free old pixmap
	XFreePixmap(dst->imginfo->display, dst->imginfo->pixmap);
	dst->imginfo->pixmap = 0;
    }

    Frame_data::display_data(dst,
			     dst->imginfo->datastx,
			     dst->imginfo->datasty,
			     dst->imginfo->datawd,
			     dst->imginfo->dataht,
			     dst->imginfo->vs,
			     FALSE);

    return(SUCCESS);
}

/************************************************************************
 *                                                                      *
 *  Do an arithmetic operation on two images.  Result is stored either
 *  in the second image or in a third selected frame.
 *  Return SUCCESS or ERROR.						*
 *  (STATIC)								*
 *									*/
int
Win_arith::operation_two_images(Arithtype optype,
				Gframe *src1, Gframe *src2, Gframe *dst)
{
    float *data1;		// Pointers to source data
    float *data2;
    float *data3;		// Pointer to destination data
    Imginfo *imginfo = NULL;    // Destination image
    Imginfo *srcimginfo = NULL;	// source image
    
    char err[128];		// error message buffer
    int ret;			// return value
    
    
    // Make sure both of the source have the same size of data
    if ((src1->imginfo->GetFast() != src2->imginfo->GetFast()) ||
	(src1->imginfo->GetMedium() != src2->imginfo->GetMedium()))
    {
	msgerr_print("operation_two_images: data sets are not the same size.");
	return(ERROR);
    }
    
    srcimginfo = src2->imginfo;
    int data_len = srcimginfo->GetFast() * srcimginfo->GetMedium();
    data1 = (float *)src1->imginfo->GetData();
    data2 = (float *)srcimginfo->GetData();
    
    if (dst == src2){
	data3 = data2;
    }else{
	// Result goes in a third frame
	// Release old image data
	if (dst->imginfo){
	    dst->clear();
	}
	detach_imginfo(dst->imginfo);

	imginfo = new Imginfo(srcimginfo);
	data3 = (float *)imginfo->GetData();
	dst->imginfo = imginfo;
    }
    dst->imginfo->vs = (src1->imginfo->vs + srcimginfo->vs) / 2.0;
    
    switch (optype){
	
      case ARITH_ADDITION:
	ret = arith_fadd_images(data1, data2, data3,
				srcimginfo->GetFast() * srcimginfo->GetMedium(),
				err);
	break;
	
      case ARITH_SUBTRACTION:
	ret = arith_fsub_images(data1, data2, data3,
				srcimginfo->GetFast() * srcimginfo->GetMedium(),
				err);
	break;
	
      case ARITH_MULTIPLICATION:
	ret = arith_fmul_images(data1, data2, data3,
				srcimginfo->GetFast() * srcimginfo->GetMedium(),
				err);
	break;
	
      case ARITH_DIVISION:
	ret = arith_fdiv_images(data1, data2, data3,
				srcimginfo->GetFast() * srcimginfo->GetMedium(),
				err);
	break;
    }
    if (ret == ERROR){
	msgerr_print("operation_two_images:%s",err);
	if (imginfo) delete imginfo;
	return(ERROR);
    }
    
    
    // Need to update the display
    if (dst->imginfo->pixmap){
	// Free old pixmap
	XFreePixmap(dst->imginfo->display, dst->imginfo->pixmap);
	dst->imginfo->pixmap = 0;
    }

    Frame_data::display_data(dst,
			     dst->imginfo->datastx,
			     dst->imginfo->datasty,
			     dst->imginfo->datawd,
			     dst->imginfo->dataht,
			     dst->imginfo->vs,
			     FALSE);

    return(SUCCESS);
}

