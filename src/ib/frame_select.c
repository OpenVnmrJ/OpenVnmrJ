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
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Programming Notes:							*
*  -----------------							*
*  									*
*  This file contain two main class routines: class Gframe and 		*
*  Frame_select.  Gframe is used to hold the information of all frames,	*
*  and Frame_select will hold information of all selected frames. In	*
*  other words, Frame_select is to manipulate selected frames only.	*
*									*
*  Description								*
*  -----------								*
*									*
*  A frame is the place where an image or spectrum can be drawn.  This	*
*  file contains routines to create/resize/move/copy/delete a frame.  	*
*									*
*  To create a frame:							*
*	Hold the LEFT mouse button down, drag, and relase the button.	*
*  To move a frame:							*
* 	Click the LEFT mouse button inside the frame, hold the LEFT	*
*  	mouse button down, and drag.					*
*  To resize a frame:							*
*	Click the LEFT mouse button inside the frame, position the mouse*
*	cursor close the either four corners of the frame, hold the LEFT*
*	mouse button down, and drag.					*
*  To copy a frame:							*
*	Click the LEFT mouse button inside the frame, hold down 'ctrl'	*
*   	key, hold the LEFT mouse button down, and drag.			*
*  To delete a frame:							*
*	User the properties menu.					*
*  To select a frame:							*
*	User the properties menu.					*
*									*
*  LEFT mouse button is used to select a specific frame, and unselect 	*
*       all other  frames.						*
*  MIDDLE mouse button is used to toggle between 'select' and 'unselect'*
*       of a frame.  It doesn't affect all other frames.		*
*  RIGHT mouse button is NOT functional.				*
*  									*
*  NOTE 								*
*  ----									*
*  - a frame should be large enough in order to exist.			*
*  - a frame must not overlap with other frames.			*
*									*
*************************************************************************/

/*
#define DEBUG
*/

// This debug should be taken out at FINAL release
#define	DEBUG_BETA	1


#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
// #include <stream.h>
#ifdef LINUX
// #include <strstream>
#else
// #include <strstream.h>
#endif
#include "ddllib.h"
#include "graphics.h"
#include "gtools.h"

#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "stderr.h"
#include "common.h"
#include "msgprt.h"
#include "interrupt.h"
#include "filelist_id.h"
#include "initstart.h"
#include "zoom.h"
#include "inputwin.h"
#include "confirmwin.h"
#include "ddlfile.h"
#include "statmacs.h"
#include "debug.h"
#include "macroexec.h"

// The following two definitions should be defined but are not in the
// current release of SABER-C++


extern void win_print_msg(char *, ...);

extern int canvas_width ;
extern int canvas_height ;
extern int canvas_stx ;
extern int canvas_sty ;
extern void canvas_repaint_proc(void);

#ifdef DEBUG
#define DEBUG_FRAME debug_frame();
#else
#define DEBUG_FRAME
#endif DEBUG

// For the sake of clearity between erase and draw, these macros are defined.
// They are actually the same.
#define	Erase_1(e)		draw(e)
#define	Erase_2(p,e)		(p)->draw(e)
#define	Corner_Check(a,b,p,q,v)  com_point_in_rect(a, b, p-v, q-v, p+v, q+v)

static const int line_thin=0;	// Thin line width
static const int line_fat=2;	// 2 pixel line width

// This variable consists a list of all frames created on the screen,
// except the first item. The first item of the list is a TEMPORARY item
// serves as a 'working' buffer (item).  DON'T count the first item as
// a frame.
extern Gframe *framehead;

// targetframe points the the next frame that data will be displayed in.

extern Gframe *targetframe;

extern int auto_skip_frame;

// This variable consists a list of all selected frames, which its
// pointers (item) 'frameptr' point to the same address as in Gframe.
// Also note that the first item of the list is a TEMPORARY item which
// serves as a 'working' buffer.  DON'T count the first item as a 
// selected frame.
extern Frame_select *selecthead;

extern char* magicnumber;

/************************************************************************
*									*
*  Creator for Frame_select.						*
*									*/
Frame_select::Frame_select(Gframe *gframe, int nth)
{
   frameptr = gframe;
   numnth = nth;
   next = NULL;
}

/************************************************************************
*									*
*  Destructor for Frame_select.						*
*									*/
Frame_select::~Frame_select(void)
{}

/************************************************************************
*									*
*  Add a selected frame in the LAST of the selected frame list.		*
*  Note that we copy all information from "working" item (the first	*
*  item) into the new created frame.					*
*									*/
void
Frame_select::insert(void)
{
   Frame_select *ptr;

   frameptr->set_select(TRUE);	// Indicate the frame is selected

   // Find the last item
   for (ptr=selecthead; ptr->next; ptr=ptr->next);

   // Append a new item into the last item
   ptr->next = new Frame_select(frameptr, ptr->numnth+1);

   // Reset the working buffer
   frameptr = NULL;
}

/************************************************************************
*									*
*  Deselect all selected frames.					*
*									*/
void
Frame_select::deselect(void)
{
   Frame_select *ptr;

   for (ptr=next; ptr; ptr=ptr->next)
   {
      // Deselect before (un)marking frame, or targetframe will get set
      ptr->frameptr->set_select(FALSE);
      ptr->frameptr->mark();
   }
   remove(REMOVE_SELECT_ALL_ITEM);
}

/************************************************************************
*									*
*  Deselect one frame
*									*/
void
Frame_select::deselect(Gframe *frame)
{
   Frame_select *ptr;

   for (ptr=next; ptr; ptr=ptr->next)
   {
       if (ptr->frameptr == frame){
	   // Deselect before (un)marking frame, or targetframe will get set
	   ptr->frameptr->set_select(FALSE);
	   ptr->frameptr->mark();
       }
   }
   remove(REMOVE_SELECT_ALL_ITEM);
}

/************************************************************************
*									*
*  Delete pointer to a particular frame from selected frame list.
*									*/
void
Frame_select::remove(Gframe *frame)
{
   Frame_select *ptr, *prev;	// loop pointers
   
   for (prev=this, ptr=next; ptr; prev=ptr, ptr=ptr->next){
       if (ptr->frameptr == frame){
	   prev->next = ptr->next;
	   delete ptr;
	   break;
       }
   }
   if (ptr == NULL){
       msgerr_print("remove:BUG:Frame_select::remove:cannot find item !");
   }

   // Update the frame numbers
   int nth = 1;
   for (ptr=next; ptr; ptr=ptr->next){
       ptr->numnth = nth++;
   }
}

/************************************************************************
*									*
*  Delete pointers (addresses) of selected frame list.			*
*									*/
void
Frame_select::remove(Remtype type)
{
   Frame_select *ptr, *prev;	// loop pointers

   if (type == REMOVE_SELECT_ONE_ITEM)
   {
      int nth=1;			// frame number

      // Note that the current selected item to be removed should be
      // in the first item ("working" buffer item).
      for (prev=this, ptr=next; ptr; prev=ptr, ptr=ptr->next)
      {
	 if (ptr->frameptr == frameptr)
	 {
	    prev->next = ptr->next;
	    delete ptr;
	    break;
	 }
      }
      if (ptr == NULL)
	msgerr_print("remove:BUG:Frame_select::remove:cannot find item !");

      // Update the frame number
      for (ptr=next; ptr; ptr=ptr->next)
	 ptr->numnth = nth++;
   }
   else	// type == REMOVE_SELECT_ALL_ITEM
   {
      // Note that we don't remove the FIRST item which serves as a 
      // working item.
      while (ptr = next)
      {
	 next = ptr->next;
	 delete ptr;
      }
   }
}

/************************************************************************
*									*
*  Split each selected frame into multiple frames.			*
*  [MACRO interface]
*  argv[0]: (int) Number of rows
*  argv[1]: (int) Number of columns
*  [STATIC Function]							*
*									*/
int
Frame_select::Split(int argc, char **argv, int, char **)
{
    argc--; argv++;

    int i;
    int ia[2];

    if (MacroExec::getIntArgs(argc, argv, ia, 2) != 2){
	ABORT;
    }
    for (i=0; i<2; i++){
	if (ia[i] < 1 || ia[i] > 100){
	    ABORT;
	}
    }
    split(ia[0], ia[1]);
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Split each selected frame into multiple frames.			*
*  [STATIC Function]							*
*									*/
void
Frame_select::split(int row, int col)
{
   Frame_select *ptr;
   Gframe *gf;

   if (selecthead->next == NULL)
   {
       if (!Gframe::get_first_frame()){
	   // No frames exist, make one and select it.
	   gf = Gframe::big_gframe();
	   gf->mark();
	   selecthead->insert(gf);
       }else{
	   msgerr_print("split:No selected frame");
	   return;
       }
   }

   interrupt_begin();
   for (ptr=selecthead->next; ptr; ptr=ptr->next)
   {
      if (interrupt())
      {
	 interrupt_msg("Interrupt at 'splitting' frame.");
	 return;
      }
      ptr->frameptr->split(row, col);
   }
   interrupt_end();
   macroexec->record("frame_split(%d, %d)\n", row, col);
}

/************************************************************************
*									*
*  Get nth selected frame from a selected frame list.			*
*  Return NULL or a pointer to a frame.					*
*  [STATIC function]							*
*									*/
Gframe *
Frame_select::get_selected_frame(int nth)
{
   Frame_select *ptr;	// selected frame pointer

   for (ptr=selecthead->next; ptr; ptr=ptr->next)
   {
      if (ptr->numnth == nth)
	 return(ptr->frameptr);
   }

   return(NULL);
}

/************************************************************************
*									*
*  Get the first selected frame.
*  Return NULL or a pointer to a frame.					*
*  [STATIC function]							*
*									*/
Gframe *
Frame_select::get_first_selected_frame()
{
    if (! selecthead->next){
	return 0;
    }else{
	return selecthead->next->frameptr;
    }
}

/************************************************************************
*									*
*  Get the next selected frame after "lastframe".
*  Returns a pointer to a frame.					*
*  Return 0 if no more selected frames or "lastframe" is not selected.
*  [STATIC function]							*
*									*/
Gframe *
Frame_select::get_next_selected_frame(Gframe *lastframe)
{
   Frame_select *ptr;	// selected frame pointer

   for (ptr=selecthead->next; ptr; ptr=ptr->next)
   {
       if (ptr->frameptr == lastframe){
	   if (ptr->next){
	       return ptr->next->frameptr;
	   }else{
	       return 0;
	   }
       }
   }
   return 0;
}
