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
*  Description								*
*  -----------								*
*									*
*  This file contains routines related to ROI tool.  They serve an 	*
*  interface between events (user mouse input) and graphics drawing	*
*  routines.								*
*									*
*  This file also contains a base class Roitool routines which serves	*
*  as general routines.							*
*									*
*************************************************************************/
#include <ctype.h>
#include <math.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include "stderr.h"
#include "confirmwin.h"
#include "graphics.h"
#include "gtools.h"
#include "win_stat.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "roitool.h"
#include "line.h"
#include "point.h"
#include "polygon.h"
#include "oval.h"
#include "label.h"
#include "msgprt.h"
#include "initstart.h"
#include "inputwin.h"
#include "filelist_id.h"
#include "convert.h"
#include "histogram.h"
#include "ddllib.h"
#include "macroexec.h"
#include "zoom.h"

//#ifdef SABER
//#define debug cout
//#else     
//#define debug name2(/,/)
//#endif

#define debug cout

extern short xview_to_ascii(short);

void canvas_repaint_proc(void);

// Initialize static class members
Gdev *Roitool::gdev = 0;       	// Graphics device
int Roitool::aperture = 0;	// Sensitivity value to resize a corner
int Roitool::max_active_tools = 10; // # of tools that can track together
int Roitool::copy_color = 0;	// Current default for drawing in "copy" mode
int Roitool::active_color = 0;	// Color used while moving ROI
int Roitool::xor_color = 0;	// Value used for XOR'ing stuff
int Roitool::color = 0;		// Current color value to use.
int Roitool::bind = 0;		// ROI binding
Gframe *Roitool::curr_frame = 0; // Remembers frame while creating ROI
Gpoint Roitool::sel_down = {0,0}; // Remembers where button went down
Pixmap Roitool::bkg_pixmap_id = NULL; // Handle of background pixmap
int Roitool::bkg_width = 0;
int Roitool::bkg_height = 0;

Raction_type Roitool::force_acttype = ROI_NO_ACTION;
Raction_type Roitool::acttype = ROI_NO_ACTION;

Menu_item Roi_routine::bind_menu_item = 0;
Gtype Roi_routine::gtooltype = (Gtype)0;

Roitool *roitool[ROI_NUM];
Roitool *selected_text = NULL;	// NOT CURRENTLY USED
Roitool *active_tool = NULL;
Roitype roitype;		// Type of ROI


Create_ListClass(Roitool);
// This is a list of all objects on the screen
RoitoolList* gObjects;  
RoitoolList* deleteStack;  
int delete_stack_depth_limit = 1;

RoitoolList *selected_ROIs;
RoitoolList *active_tools;	// ROIs being created/modified interactively
RoitoolList *clone_tools;	// ROIs being created/modified at completion

RoitoolLink::~RoitoolLink() {}

RoitoolLink&
RoitoolLink::Print()
{
  printf("object[%d]\n", item);
  return *this;
}

Create_ListClass(Lpoint);
LpointLink::~LpointLink() {}

LpointLink&
LpointLink::Print()
{
  printf("object[%d]\n", item);
  return *this;
}

/************************************************************************
*									*
*  win_graphics_redraw is normally called by canvas_repaint_proc() when *
*  it gets a "Redisplay" request from the user or a REPAINT request     *
*  from xview.                                                          *
*									*
*************************************************************************/


void
redraw_all_roitools(){
    //debug << "redraw_all_roitools()" << endl;

    Gframe *frame;
    for (frame = Gframe::get_first_frame();
	 frame;
	 frame = Gframe::get_next_frame(frame) )
    {
	Imginfo *img;
	if (img=frame->imginfo){
	    RoitoolIterator element(img->display_list);
	    Roitool *tool;
	    Gmode mode;
	    
	    while (tool = ++element){
		mode = tool->setcopy();
		tool->draw();
		tool->setGmode(mode);
	    }
	}
    }
}

/************************************************************************
*									*
*                                                                       */
void
Roi_routine::SelectToolsInBox(Gpoint *corner)
{
    Roitool *tool;

    if (corner[0].x != G_INIT_POS
	&& abs(corner[0].x - corner[1].x) > 1
	&& abs(corner[0].y - corner[1].y) > 1)
    {
	// Deselect everything
	while (tool = selected_ROIs->Pop()){
	    tool->deselect();
	    tool->owner_frame->imginfo->display_ood = TRUE;
	}

	if (corner[0].x != G_INIT_POS){
	    Gframe *frame;
	    Imginfo *img;
	    frame=Gframe::get_first_frame();
	    for ( ; frame; frame=Gframe::get_next_frame(frame)){
		if (img=frame->imginfo){
		    RoitoolIterator element(img->display_list);
		    while (tool = ++element){
			if (tool->x_min >= corner[0].x
			    && tool->y_min >= corner[0].y
			    && tool->x_max <= corner[1].x
			    && tool->y_max <= corner[1].y)
			{
			    tool->select(ROI_NOREFRESH, TRUE);
			    img->display_ood = TRUE;
			}
		    }
		}
	    }
	}
	Frame_data::redraw_ood_images();
    }
}

/************************************************************************
*									*
*  Finish off the creation of active ROItools
*									*/
void
Roi_routine::finish_tool_creation(short x, short y, short action)
{
    int polygon = (active_tool
		   && active_tool->created_type == ROI_POLYGON);
    
    if (active_tool &&
	active_tool->active_action(x, y, action) == REACTION_CREATE_OBJECT &&
	Roitool::acttype != ROI_ROTATE_DONE &&
	Roitool::curr_frame)
    {
	Imginfo *oldimg = active_tool->owner_frame->imginfo;
	RoitoolIterator element(active_tools);
	Roitool *tool;
	while (tool = ++element){
	    if (!tool->roi_state(ROI_STATE_CREATE)){
		tool->forget_bkg();
	    }
	    AppendObject(tool, tool->owner_frame);
	    tool->select(ROI_COPY, TRUE);
	    tool->owner_frame->imginfo->update_data_coordinates(tool);
	}
    }

    // If click-and-release, select this ROI only
    if (Roitool::acttype == ROI_MOVE_DONE
	&& x == Roitool::sel_down.x
	&& y == Roitool::sel_down.y)
    {
	active_tool->select(ROI_COPY, FALSE);
    }

    if ( !active_tool->roi_state(ROI_STATE_CREATE)){
	Roitool::clone_first_active();
    }
    
    // If the ROI tool still doesn't exist, it means that it is
    // still being created.
    if (active_tool && active_tool->roi_state(ROI_STATE_EXIST)) {
	Roitool::acttype = ROI_NO_ACTION;
	//debug << "Roitool::acttype = ROI_NO_ACTION" << endl;
	SetRoiType(ROI_SELECTOR, GTOOL_SELECTOR);
    } else {
	Roitool::acttype = ROI_CREATE;
	//debug << "Roitool::acttype = ROI_CREATE" << endl;
    }
    if ( ! (active_tool && active_tool->created_type == ROI_POLYGON
	&& active_tool->roi_state(ROI_STATE_CREATE)))
    {
	// We are not making a polygon
	active_tool->setxor();
    }
}

/************************************************************************
*									*
*  Mouse event Graphics-tool: ROI.					*
*  This function will be called if there is an event related to a frame.*
*  (I.e., if the Pointer or an ROI or Text button is selected
*  in the gtools panel, keyboard/mouse events come through here.)
*  If the event has been handled by the ROI, then return ROUTINE_DONE 	*
*  else ROUTINE_FRAME_SELECT.						*
*                                                                       */
Routine_type
Roi_routine::process(Event *e)
{
    Routine_type rtn = ROUTINE_DONE;

    short x = event_x(e);
    short y = event_y(e);
    short action = event_action(e);
    Gframe *frame = Gframe::get_gframe_with_pos(x, y);

    switch (action){
	
      case LOC_DRAG:
	//
	// Mouse moving with a button pressed
	//
	//debug << "process() LOC_DRAG" << endl;
	if (active_tool){
	    RoitoolIterator element(active_tools);
	    Roitool *tool;
	    Imginfo *oldimg = NULL;
	    if (active_tool->owner_frame){
		oldimg = active_tool->owner_frame->imginfo;
	    }
	    while (tool = ++element){
		Imginfo *newimg = NULL;
		if (tool->owner_frame){
		    newimg = tool->owner_frame->imginfo;
		}
		// Don't draw the handles while we are moving (too slow)
		short state = tool->roi_state(ROI_STATE_MARK);
		tool->roi_clear_state(ROI_STATE_MARK);
		if (newimg && oldimg){
		    Gpoint npt = oldimg->ScreenToScreen(newimg, x, y);
		    tool->action(npt.x, npt.y, action);
		}else{
		    tool->action(x, y, action);
		}
		// Restore handles
		tool->roi_set_state(state);
	    }


	    // Update histogram statistics
	    Imginfo *img;
	    int update = FALSE;
	    element.GotoFirst();
	    while (tool = ++element){
		if (tool->owner_frame && (img = tool->owner_frame->imginfo)
		    && (tool->created_type != ROI_TEXT
			|| tool->roi_state(ROI_STATE_EXIST)))
		{
		    img->update_data_coordinates(tool);
		    if (tool->is_selected()){
			update = TRUE;
		    }
		}
	    }
	    if (update){
		Win_stat::win_stat_update(2);
	    }
	}
	rtn = ROUTINE_DONE;
	break;
	
      case LOC_MOVE:
	//
	// Mouse moving - no button pressed
	//   (Needed for polygon drawing w/ click at each vertex.)
	//
	//debug << "process() LOC_MOVE" << endl;
	if (active_tool && active_tool->roi_state(ROI_STATE_CREATE)) {
	    active_tool->active_action(x, y, action);
	}
	rtn = ROUTINE_DONE;
	break;
	
      case ACTION_SELECT:
	//
	// Handle left mouse button events
	//
	// Note that this only reports up and down events of the left button.
	// Motion events are reported by LOC_DRAG
	
	if (event_is_down(e)){
	    //
	    // Handle left-button-down events
	    //
	    //debug << "process() ACTION_SELECT, down" << endl;

	    Roitool::sel_down.x = x;
	    Roitool::sel_down.y = y;
	    Roitool::curr_frame = frame;  // Remember which frame we clicked in
	    
	    if (active_tool &&active_tool->created_type == ROI_POLYGON
		&& active_tool->roi_state(ROI_STATE_CREATE))
	    {
		// We are in the middle of making a polygon
		active_tool->active_action(x, y, action);
		rtn = ROUTINE_DONE;
	    }else if (Roitool::acttype == ROI_CREATE
		      && active_tool != roitool[GTOOL_SELECTOR]
		      && Roitool::curr_frame
		      && Roitool::curr_frame->imginfo)
	    {
		//
		// Create a new ROI
		//
		//debug << "Create a new roi definition." << endl;

		if (event_shift_is_down(e) && active_tool){
		    // Do not revert to Selector after creation
		    gtooltype = toolToGtoolType(active_tool);
		}else{
		    gtooltype = (Gtype)0;
		}
		while (active_tools->Pop()); // Empty the active tools list
		while (clone_tools->Pop()); // Empty the clone tools list
		int nactive = 0;
		Roitool *tool = create_roitool(Roitool::curr_frame);
		active_tools->Push(tool);
		tool->action(x, y, action);
		if (Roitool::bind){
		    Imginfo *cur_img = Roitool::curr_frame->imginfo;
		    Gframe *gframe;
		    for (gframe=Frame_select::get_first_selected_frame();
			 gframe;
			 gframe=Frame_select::get_next_selected_frame(gframe))
		    {
			Imginfo *img = gframe->imginfo;
			if (img && gframe != Roitool::curr_frame){
			    tool = create_roitool(gframe);
			    if (nactive < Roitool::max_active_tools){
				active_tools->Push(tool);
				Gpoint npt = cur_img->ScreenToScreen(img, x, y);
				tool->action(npt.x, npt.y, action);
				nactive++;
			    }else{
				tool->select(ROI_NOREFRESH, TRUE);
				clone_tools->Push(tool);
			    }
			}
		    }
		}
		active_tool = active_tools->First()->Item();
		rtn = ROUTINE_DONE;
	    }else{
		// User wants to modify an existing tool, so we must
		// try to find and select an ROI tool that the user
		// has just clicked on (if any).
		Roitool *new_selected_tool = 0;
		Imginfo *img;
		Roitool::force_acttype = ROI_NO_ACTION;
		if (event_ctrl_is_down(e)){
		    // Ctrl down means rotate the selected ROI tool
		    // Roitool::force_acttype = ROI_ROTATE;
		}
		if (event_shift_is_down(e)){
		    // Shift down means move the selected ROI tool
		    Roitool::force_acttype = ROI_MOVE;
		}
		if (Roitool::curr_frame
		    && (img = Roitool::curr_frame->imginfo))
		{
		    RoitoolIterator element(img->display_list);
		    element.GotoLast();

		    Roitool *tool;
		    while (tool = --element) {
			if (tool->is_selectable(x, y)){
			    active_tool = new_selected_tool = tool;
			    break;
			}
		    }
		}

		if (new_selected_tool) {
		    //
		    // Select an exisitng ROI
		    //
		    // The user has clicked on a pre-existing tool
		    // Make list of active ROIs
		    while (active_tools->Pop()); // Empty active tools list
		    while (clone_tools->Pop()); // Empty clone tools list
		    active_tools->Push(new_selected_tool);
		    Roitool *tool;
		    if (Roitool::bind){
			// Find (nearly) identical ROIs in selected gframes
			Imginfo *cur_img = Roitool::curr_frame->imginfo;
			Gframe *gf;
			int nact = 0;
			for (gf=Gframe::get_first_frame();
			     gf;
			     gf=Gframe::get_next_frame(gf))
			{
			    Imginfo *img = gf->imginfo;
			    if (gf != Roitool::curr_frame && img){
				// Look at all ROIs in this image
				RoitoolIterator toollist(img->display_list);
				while (tool = ++toollist){
				    if (tool->matches(new_selected_tool)){
					Gpoint npt;
					npt = cur_img->ScreenToScreen(img,x,y);
					if (nact < Roitool::max_active_tools){
					    tool->is_selected(npt.x, npt.y);
					    active_tools->Push(tool);
					    nact++;
					}else{
					    clone_tools->Push(tool);
					}
				    }
				}
			    }
			}
		    }
		    // Only need this because "is_selected()" can add a
		    //  vertex to a polygon.  We have to do it after
		    //  we look for matching polygons.
		    new_selected_tool->is_selected(x, y);

		    // Now operate on everything in the active list
		    RoitoolIterator alist(active_tools);
		    while (tool = ++alist){
			tool->setactive();
			tool->save_bkg();
			if (tool->is_selected()){
			    tool->select(ROI_XOR, TRUE); // Updates screen
			}else{
			    tool->refresh(ROI_XOR);
			}
		    }
		}else{
		    // Clicked outside any existing tool
		    // -- activate the selection box.
		    active_tool = roitool[ROI_SELECTOR];
		    while (active_tools->Pop()); // Empty active tools list
		    active_tools->Push(active_tool);
		    Roitool::acttype = ROI_CREATE;
		}
		active_tool->active_action(x, y, action);
		rtn = ROUTINE_DONE;
	    }
	}else{
	    //
	    // Handle left-button-up events
	    //
	    //debug << "process() ACTION_SELECT, up" << endl;
	    //debug << "User has just released the mouse button." << endl;
	    
	    switch (Roitool::acttype){
		
	      case ROI_CREATE:
		Roitool::acttype = ROI_CREATE_DONE;
		//debug << "Roitool::acttype = ROI_CREATE_DONE" << endl;
		if (active_tool == roitool[ROI_SELECTOR]){
		    // Following three lines need to be in this order.
		    active_tool->erase();
		    SelectToolsInBox(active_tool->pnt);
		    active_tool->init_point();

		    Roitool::acttype = ROI_NO_ACTION;
		    //debug << "Roitool::acttype = ROI_NO_ACTION" << endl;
		    SetRoiType(ROI_SELECTOR, GTOOL_SELECTOR);
		    if (x == Roitool::sel_down.x && y == Roitool::sel_down.y){
			rtn = ROUTINE_FRAME_SELECT;
		    }else{
			rtn = ROUTINE_DONE;
		    }
		    active_tool = 0;
		    while (active_tools->Pop());
		}else{
		    finish_tool_creation(x, y, action);
		    rtn = ROUTINE_DONE;
		}
		Win_stat::win_stat_update(1);
		break;
		
	      case ROI_RESIZE:
		Roitool::acttype = ROI_RESIZE_DONE;
		//debug << "Roitool::acttype = ROI_RESIZE_DONE" << endl;
		finish_tool_creation(x, y, action);
		rtn = ROUTINE_DONE;
		Win_stat::win_stat_update(1);
		break;
		
	      case ROI_MOVE:
		Roitool::acttype = ROI_MOVE_DONE;
		//debug << "Roitool::acttype = ROI_MOVE_DONE" << endl;
		finish_tool_creation(x, y, action);
		rtn = ROUTINE_DONE;
		Win_stat::win_stat_update(1);
		break;
		
	      case ROI_ROTATE:
		Roitool::acttype = ROI_ROTATE_DONE;
		//SetRoiType(ROI_SELECTOR, GTOOL_SELECTOR);
		//debug << "Roitool::acttype = ROI_ROTATE_DONE" << endl;
		finish_tool_creation(x, y, action);
		rtn = ROUTINE_DONE;
		Win_stat::win_stat_update(1);
		break;
		
	      default: 
		Roitool::acttype = ROI_NO_ACTION;
		//debug << "Roitool::acttype = ROI_NO_ACTION" << endl;
		rtn = ROUTINE_FRAME_SELECT;
		break;
	    }
	    if (!active_tool || active_tool->roi_state(ROI_STATE_EXIST)){
		// Turn off active_tool unless we are in process of creation
		active_tool = 0;
		while (active_tools->Pop());
	    }
	    if (gtooltype && active_tool
		&& active_tool->created_type != ROI_POLYGON)
	    {
		// User wants to create more of same
		SetRoiType(ROI_SELECTOR,  GTOOL_SELECTOR);
		SetRoiType(gtoolTypeToRoitoolType(gtooltype),  gtooltype);
		gtooltype = (Gtype)0;
	    }
	}			// End of left-button-up events
	break;
	
      case ACTION_ADJUST:
	//
	// Handle middle mouse button events
	//
	if (event_is_down(e)){
	    //
	    // Handle middle button down events
	    //
	    if (active_tool){
		RoitoolIterator alist(active_tools);
		Roitool *tool;
		int polyoff = FALSE;
		while (tool = ++alist){
		    if (!tool->roi_state(ROI_STATE_EXIST)
			&& tool->mouse_middle()==REACTION_CREATE_OBJECT)
		    {
			// Finish off a polygon ROI
		        polyoff = TRUE;
			Gframe *toolowner = tool->owner_frame;
			AppendObject(tool, toolowner);
			tool->setxor();
			tool->forget_bkg();
			toolowner->imginfo->update_data_coordinates(tool);
			tool->select(ROI_COPY, TRUE);
		    }
		}
		if (gtooltype && polyoff){
		  SetRoiType(ROI_SELECTOR,  GTOOL_SELECTOR);
		  SetRoiType(gtoolTypeToRoitoolType(gtooltype),
			     gtooltype);
		  gtooltype = (Gtype)0;
		}
		Roitool::clone_first_active();
		/*Roitool::acttype = ROI_NO_ACTION;/*Breaks gtooltype--CMP*/
		//debug << "Roitool::acttype = ROI_NO_ACTION" << endl;
		Win_stat::win_stat_update(1);
		rtn = ROUTINE_DONE;
	    }else{
		// Toggle selection of ROI--if we are on one
		// Look for a tool at our position
		Roitool::curr_frame = frame;  // Deal only with this frame
		Roitool *sel_tool = 0;
		Imginfo *img;
		if (frame && (img = Roitool::curr_frame->imginfo)){
		    RoitoolIterator tlist(img->display_list);
		    tlist.GotoLast();
		    while (sel_tool = --tlist){
			if (sel_tool->is_selectable(x, y)){
			    break;
			}
		    }
		}
		active_tool = 0;

		if (sel_tool){
		    // The user has clicked on a pre-existing tool
		    if (sel_tool->is_selected()){
			selected_ROIs->Remove(sel_tool);
			sel_tool->deselect();
			sel_tool->owner_frame->imginfo->display_ood = TRUE;
			Frame_data::redraw_ood_images();
		    }else{
			sel_tool->select(ROI_COPY, TRUE);
		    }
		    Win_stat::win_stat_update(1);
		    rtn = ROUTINE_DONE;
		}else{
		    rtn = ROUTINE_FRAME_TOGGLE;
		}
		Roitool::acttype = ROI_NO_ACTION;
	    }
	}else{
	    //
	    // Handle middle button up events
	    //
	    /*active_tool = 0;
	    while (active_tools->Pop());/*Breaks gtooltype--CMP*/
	    rtn = ROUTINE_DONE;
	}
	break;
	
      case ACTION_MENU:
	//
	// Handle right mouse button events
	//
	rtn = ROUTINE_DONE;
	break;
	
	
      default:
	//
	// Handle misc. keyboard events.
	//
	if (event_is_down(e)){
	    // Check who gets ASCII events
	    // Labels get priority for printable chars
	    int islabel = FALSE;
	    {
		// Look for selected text labels (that can eat characters)
		RoitoolIterator element(selected_ROIs);
		Roitool *tool;
		while (tool = ++element){
		    if (strcasecmp(tool->name(), "label") == 0){
			islabel = TRUE;
		    }
		}
	    }		
	    int chr = xview_to_ascii(action);
	    if (chr == '\f'){	// Ctl-L
		// Refresh the screen
		canvas_repaint_proc();
		rtn = ROUTINE_DONE;
	    }else if ((chr == '\032' && event_shift_is_down(e)) // Shift-Ctl-Z
		       || (!islabel && chr == 'Z')) // Shift-Z
	    {
		Zoomf::zoom_quick(frame, x, y, Z_ZOOM);	// Zoom in
		rtn = ROUTINE_DONE;
	    }else if ((chr == '\032' && !event_shift_is_down(e)) // Ctl-z
		       || (!islabel && chr == 'z')) // z
	    {
		Zoomf::zoom_quick(frame, x, y, Z_UNZOOM); // Zoom out
		rtn = ROUTINE_DONE;
	    }else if ((chr == '\004') // Ctl-C
		      || (!islabel && chr == 'c') // c
		      || (!islabel && chr == 'C')) // C
	    {
		Zoomf::zoom_quick(frame, x, y, Z_PAN); // Pan
		rtn = ROUTINE_DONE;
	    }else if (islabel && isascii(chr)){
		// Handle entries for text labels
		RoitoolIterator element(selected_ROIs);
		Roitool *tool;
		while (tool = ++element){
		    if (strcasecmp(tool->name(), "label") == 0){
			tool->handle_text(chr);
		    }
		}
		rtn = ROUTINE_DONE;
	    }else{
		//debug << "Unhandled action in Roi_routine::process()" << endl;
		rtn = ROUTINE_DONE;
	    }
	}
	break;
    }
    return rtn;
}

/************************************************************************
 *									*
 * Sets the roitype to the specified value and invokes any necessary	*
 * notification functions of the change in roitype			*
 *									*/

/* static */
void Roi_routine::SetRoiType(Roitype t, Gtype g)

{
    roitype = t;
    Gtools::select_tool(0, g);
}

/************************************************************************
 *                                                                       *
 *  Update the screen for a particular ROI.
 *  Data is redrawn; unselected tools drawn in solid; selected tools
 *  drawn in XOR mode or COPY mode, depending on value of "mode".
 */
void Roitool::refresh(RoiDrawMode mode)		// ROI_XOR or ROI_COPY
{
    if (mode == ROI_XOR){
	roi_clear_state(ROI_STATE_MARK);
    }else if (is_selected()){
	roi_set_state(ROI_STATE_MARK);
    }
    
    // Refresh my picture
    if ( (mode != ROI_NOREFRESH) && owner_frame){
	if (mode == ROI_XOR){
	    visible = FALSE;	// Do not draw myself
	}
	owner_frame->display_data();
	visible = TRUE;
	//debug << "select(): redisplay_data" << endl;
    }

    if ((mode == ROI_XOR) && owner_frame){
	// Draw myself in XOR mode without handles
	roi_clear_state(ROI_STATE_MARK);
	owner_frame->set_clip_region(FRAME_CLIP_TO_IMAGE);
	draw();
	owner_frame->set_clip_region(FRAME_NO_CLIP);
	if (is_selected()){
	    roi_set_state(ROI_STATE_MARK);
	}
    }

    if (mode != ROI_NOREFRESH){
	// Refresh all other pictures
	Frame_data::redraw_ood_images();
    }
}

/************************************************************************
 *                                                                       *
 *  Select a particular Roitool and update the screen accordingly.
 *  Data is redrawn; unselected tools drawn in solid; selected tool
 *  drawn in XOR mode or COPY mode, depending on value of "mode".
 */
void Roitool::select(RoiDrawMode mode,		// ROI_XOR or ROI_COPY
		     int appendflag)		// Other ROIs remain selected
{
    int already_selected = FALSE;
    Roitool *tool;

    if (appendflag){
	// Look for this tool in the list
	RoitoolIterator slist(selected_ROIs);
	while (tool = ++slist){
	    if (tool == this){
		already_selected = TRUE;
	    }
	}
	
    }else{
	// Empty the selected list
	while (tool = selected_ROIs->Pop()){
	    tool->deselect();			// Deselect all ROIs
	    tool->owner_frame->imginfo->display_ood = TRUE;
	}
    }

    if (!already_selected){
	selected_ROIs->Push(this); // Select new ROI
	owner_frame->imginfo->display_ood = TRUE; // Set Out Of Date flag
    }
    refresh(mode);
}


/************************************************************************
*                                                                       *
*  Get some information about the roi.                                  *
*  Only exists for points and lines. Look in  point.c and line.c.       *
*                                                                       */
void
Roitool::some_info(int)
{
}

/************************************************************************
*                                                                       *
*  Return the latest selected tool
*  STATIC
*                                                                       */
Roitool *
Roitool::get_selected_tool()
{
    if (selected_ROIs->Count() ){
	return selected_ROIs->Top();
    }else{
	return 0;
    }
}

/************************************************************************
*                                                                       *
*  Determine if a gframe has any selected tools in it.
*  STATIC
*                                                                       */
int
Roitool::frame_has_a_selected_tool(Gframe *frame)
{
    RoitoolIterator element(selected_ROIs);
    Roitool *tool;
    while (tool = ++element){
	if (tool->owner_frame == frame){
	    return TRUE;
	}
    }
    return FALSE;
}

void Roitool::deselect()
{
    roi_clear_state(ROI_STATE_MARK);
    selected_ROIs->Remove(this);
    //mark();
}

/************************************************************************
*									*
*  This routine is called after the user has completed the definition   *
*  of an object.  It will append the new object to the object list      *
*									*/
int
Roi_routine::AppendObject(Roitool* tool, Gframe *frame)
{
    if (!tool) return REACTION_NONE;
    
    // The user has completed the definition of the object
    // Now the object must be appended to the object list
    frame->imginfo->display_list->Append(tool);
    tool->owner_frame = frame;
    //debug << "frame->imginfo->display_list->Count = "
    //    	  << frame->imginfo->display_list->Count() << endl;
    //    frame->imginfo->display_list->Print();
    SetRoiType(ROI_SELECTOR,  GTOOL_SELECTOR);
    return(REACTION_CREATE_OBJECT);
}

/************************************************************************
*									*
*  Delete selected ROIs
*  [MACRO interface]
*  no arguments
*  [STATIC Function]							*
*									*/
int
Roi_routine::Delete(int argc, char **, int, char **)
{
    argc--;

    if (argc != 0){
	ABORT;
    }
    DeleteSelectedTools(GTOOL_REFRESH, FALSE);
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  This routine deletes all the selected tools from the
*  global RoitoolList                                                   *
*  <STATIC>                                                             */
void
Roi_routine::DeleteSelectedTools(GRefreshFlag refresh_flag, Flag confirm)
{
    if (!confirm || confirmwin_popup("Yes", "No", "Delete selected ROIs?", 0)){
	Roitool *tool;
	while (tool = selected_ROIs->Pop()){
	    DeleteObject(tool, GTOOL_NOREFRESH);
	    tool->owner_frame->imginfo->display_ood = TRUE;
	}
	if (refresh_flag != GTOOL_NOREFRESH){
	    Frame_data::redraw_ood_images();
	}
	macroexec->record("roi_delete\n");
    }
}

/************************************************************************
*									*
*  This routine is called to delete a specified roitool from the       *
*  global RoitoolList                                                   *
*  <STATIC>                                                             */
int
Roi_routine::DeleteObject(Roitool* tool, GRefreshFlag refresh_flag)
{
    if (!tool){
	return REACTION_NONE;
    }
    
    Gframe *frame = tool->owner_frame;

    // Free any privious pixmap
    tool->forget_bkg();
    
    // Remove the tool from various lists
    Roitool* deleted_object = frame->imginfo->display_list->Remove(tool);
    selected_ROIs->Remove(tool);
    active_tools->Remove(tool);
    if (refresh_flag != GTOOL_NOREFRESH){
	frame->display_data();
    }
    if (deleted_object){
	// Was in the display list,
	//  push the deleted object onto the deleteStack
	if (deleteStack->Count() >= delete_stack_depth_limit){
	    // Sorry, oldest object is gone forever since stack is full
	    Roitool *rt = deleteStack->First()->Item();
	    //delete deleteStack->Dequeue();/* This decrements Count by 2! */
	    deleteStack->Remove(deleteStack->First());
	    delete rt;
	}
	deleteStack->Push(deleted_object);
    }else{
	delete tool;
    }
    
    //debug << "deleteStack->Count = " << deleteStack->Count() << endl;
    //    deleteStack->Print();
    return(REACTION_DELETE_OBJECT);
}

/************************************************************************
*									*
*  Set the graphics drawing mode for roi to XOR.
*  Returns what the old mode was.
*/
Gmode
Roitool::setxor()
{
    Gmode mode;

    mode.op = G_Get_Op(gdev);
    G_Set_Op(gdev, GXxor);
    mode.color = color;
    color = xor_color;
    return mode;
}

/************************************************************************
*									*
*  Set the graphics drawing mode for roi to COPY.
*  Returns what the old mode was.
*/
Gmode
Roitool::setcopy()
{
    Gmode mode;

    mode.op = G_Get_Op(gdev);
    G_Set_Op(gdev, GXcopy);
    mode.color = color;
    color = my_color;
    return mode;
}

/************************************************************************
*									*
*  Set the graphics mode and color for moving an ROI in copy mode.
*  Like setcopy() except for the color.
*  Set the graphics drawing mode for roi to COPY.
*  Returns what the old mode was.
*/
Gmode
Roitool::setactive()
{
    Gmode mode;

    mode.op = G_Get_Op(gdev);
    G_Set_Op(gdev, GXcopy);
    mode.color = color;
    color = active_color;
    return mode;
}

/************************************************************************
*									*
*  Set the graphics drawing mode for roi to specified value.
*  Returns what the old mode was.
*/
Gmode
Roitool::setGmode(Gmode new_mode)
{
    Gmode old_mode;

    old_mode.op = G_Get_Op(gdev);
    G_Set_Op(gdev, new_mode.op);
    old_mode.color = color;
    color = new_mode.color;
    return old_mode;
}


/************************************************************************
*									*
*  This routine is called to undelete the last object from the deleted  *
*  object stack                                                         *
*  <STATIC>                                                             */

int
Roi_routine::UnDeleteObject() {
  
  Roitool* obj;

  if (obj = deleteStack->Pop()) {
    obj->draw();
    AppendObject(obj, Roitool::curr_frame);
    obj->select();
    SetRoiType(ROI_SELECTOR, GTOOL_SELECTOR);
  } else {
    msgerr_print("No objects to undelete");
  }
    
  //debug << "deleteStack->Count = " << deleteStack->Count() << endl;
  //  deleteStack->Print();
  return(REACTION_NONE);
}

/************************************************************************
*									*
*  Update x_min, y_min, x_max, y_max
*									*/
void
Roitool::update_xyminmax(int x, int y)
{
    if (x < x_min){
	x_min = x;
    }else if (x > x_max){
	x_max = x;
    }
    if (y < y_min){
	y_min = y;
    }else if (y > y_max){
	y_max = y;
    }
}

/************************************************************************
*									*
*  Update x_min, y_min, x_max, y_max
*									*/
void
Roitool::calc_xyminmax()
{
    if (npnts == 0){
	x_min = y_min = x_max = y_max = 0;
    }else{
	x_min = x_max = pnt[0].x;
	y_min = y_max = pnt[0].y;
	int i;
	for (i=1; i<npnts; i++){
	    if (pnt[i].x < x_min){
		x_min = pnt[i].x;
	    }else if (pnt[i].x > x_max){
		x_max = pnt[i].x;
	    }
	    if (pnt[i].y < y_min){
		y_min = pnt[i].y;
	    }else if (pnt[i].y > y_max){
		y_max = pnt[i].y;
	    }
	}
    }
}

/************************************************************************
*									*
*  Create/move/resize all active ROI tools.
*									*/
ReactionType
Roitool::active_action(short x, short y, short action)
{
    ReactionType rtn = (ReactionType)NULL;
    Gpoint npt;

    RoitoolIterator alist(active_tools);
    Roitool *tool;
    Imginfo *oldimg = NULL;
    if (owner_frame){
	oldimg = owner_frame->imginfo;
    }
    while (tool = ++alist){
	if (this == tool){
	    rtn = tool->action(x, y, action);
	}else{
	    Imginfo *newimg = tool->owner_frame->imginfo;
	    if (oldimg && newimg){
		npt = oldimg->ScreenToScreen(newimg, x, y);
		rtn = tool->action(npt.x, npt.y, action);
	    }
	}
    }
    return rtn;
}

/************************************************************************
*									*
*  Return the order of a given tool in the active list.
*  First is 0.  If not in list, return -1.
*
*  STATIC
*									*/
int
Roitool::position_in_active_list(Roitool *tool)
{
    int index;
    Roitool *item;

    if (!active_tools || !active_tools->Count()){
	return -1;
    }
    RoitoolIterator alist(active_tools);
    for (index=0; item = ++alist; index++){
	if (tool == item){
	    return index;
	}
    }
    return -1;
}

/************************************************************************
*									*
*  Make every tool in the active_tools list like the first one
*
*  STATIC
*									*/
void
Roitool::clone_first_active()
{
    if (!active_tools || !active_tools->Count()
	|| !clone_tools || !clone_tools->Count())
    {
	return;
    }
    int count = active_tools->Count() + clone_tools->Count();
    Flag *selectlist = new Flag[count];
    Flag *sel;
    RoitoolList *copylist = new RoitoolList;
    RoitoolIterator alist(active_tools);
    Roitool *tfirst = active_tools->First()->Item();
    Roitool *tool;
    Roitool *toolcopy;
    /* Replace all the secondary active tools */
    sel = selectlist;
    while (tool = ++alist){
	if (tool != tfirst){
	    toolcopy = tfirst->copy(tool->owner_frame);
	    tool->owner_frame->imginfo->display_ood = TRUE;
	    if (toolcopy){
		*(sel++) = tool->is_selected();
		copylist->Push(toolcopy);
		Roi_routine::DeleteObject(tool, GTOOL_NOREFRESH);
	    }
	}
    }
    /* Replace everything in the clone list too */
    RoitoolIterator blist(clone_tools);
    while (tool = ++blist){
	toolcopy = tfirst->copy(tool->owner_frame);
	tool->owner_frame->imginfo->display_ood = TRUE;
	if (toolcopy){
	    *(sel++) = tool->is_selected();
	    copylist->Push(toolcopy);
	    Roi_routine::DeleteObject(tool, GTOOL_NOREFRESH);
	}
    }
    /* Put all the new copies in the active list */
    RoitoolIterator clist(copylist);
    sel = selectlist;
    while (tool = ++clist){
	active_tools->Push(tool);
	if (*(sel++)){
	    tool->select(ROI_NOREFRESH, TRUE);
	}
    }
    /* Update the screen */
    Frame_data::redraw_ood_images();

    delete [] selectlist;
    while (copylist->Pop());	// Delete pointers to tools
    delete copylist;
    return;
}

/************************************************************************
*									*
*  Action routines to create/move/resize ROI tool.			*
*									*/
ReactionType
Roitool::action(short x, short y, short action)
{
    ReactionType rtn = REACTION_NONE;
    int roimarked;
    
    switch (acttype){
	
      case ROI_NO_ACTION:
	break;
	
      case ROI_CREATE:
	keep_point_in_image(&x, &y);
	if (basex == G_INIT_POS){
	    // Object is just being created
	    basex = x_min = x_max = x;
	    basey = y_min = y_max = y;
	    if (roitype == ROI_POINT){
		rtn = create(x, y, action);
	    }
	}else{
	    rtn = create(x, y, action);
	}
	break;
	
      case ROI_RESIZE:
	// Note that 'basex' and 'basey' has been set prior to calling
	// this routine. (look at routine "is_selected" for an
	// appropriate ROI)
	keep_point_in_image(&x, &y);
	update_xyminmax(x, y);
	roimarked = roi_state(ROI_STATE_MARK);
	roi_clear_state(ROI_STATE_MARK);
	rtn = resize(x, y);
	if (roimarked){
	    roi_set_state(ROI_STATE_MARK);
	}
	break;
	
      case ROI_MOVE:
	if (basex == G_INIT_POS){
	    basex = x; basey = y;
	} else {
	    rtn = move(x, y);
	}
	break;
	
      case ROI_ROTATE:
	if (basex == G_INIT_POS){
	    basex = x; basey = y;
	} else {
	    rtn = rotate(x, y);
	}
	break;

      case ROI_CREATE_DONE:
	keep_point_in_image(&x, &y);
	rtn = create_done(x, y, action);
	break;

      case ROI_RESIZE_DONE:
	rtn = resize_done(x, y);
	draw_solid();
	break;

      case ROI_MOVE_DONE:
	rtn = move_done(x, y);
	draw_solid();
	break;

      case ROI_ROTATE_DONE:
	rtn = rotate_done(x, y);
	draw_solid();
	break;
    }
    
    return rtn;
}

/************************************************************************
*									*
*  Returns Gtool type corresponding to an Roitool
*
*  STATIC
*									*/
Gtype
Roi_routine::toolToGtoolType(Roitool *roitool)
{
    if (!roitool){
	return (Gtype)-1;
    }
    int rtype = roitool->created_type;
    switch (rtype){
      case ROI_SELECTOR:
	gtype = GTOOL_SELECTOR; break;
      case ROI_LINE:
	gtype = GTOOL_LINE; break;
      case ROI_POINT:
	gtype = GTOOL_POINT; break;
      case ROI_BOX:
	gtype = GTOOL_RECT; break;
      case ROI_POLYGON:
      case ROI_POLYGON_OPEN:
	if (((Polygon *)roitool)->is_closed()){
	    gtype = GTOOL_PGON;
	}else{
	    gtype = GTOOL_PGON_OPEN;
	}
	break;
      case ROI_TEXT:
	gtype = GTOOL_TEXT; break;
      default:
	gtype = (Gtype)-1;
    }
    return gtype;
}

/************************************************************************
*									*
*  Returns Roitool Type corresponding to an Roitool
*
*  STATIC
*									*/
Roitype
Roi_routine::toolToRoitoolType(Roitool *roitool)
{
    if (!roitool){
	return (Roitype)-1;
    }
    Roitype rtype = roitool->created_type;
    if (rtype == ROI_POLYGON || rtype == ROI_POLYGON_OPEN){
	if (((Polygon *)roitool)->is_closed()){
	    rtype = ROI_POLYGON;
	}else{
	    rtype = ROI_POLYGON_OPEN;
	}
	return rtype;
    }
}

/************************************************************************
*									*
*  Returns Roitool Type corresponding to a Gtype
*
*  STATIC
*									*/
Roitype
Roi_routine::gtoolTypeToRoitoolType(Gtype gtype)
{
    Roitype roitype;

    switch (gtype){
      case GTOOL_SELECTOR:
	roitype = ROI_SELECTOR; break;
      case GTOOL_LINE:
	roitype = ROI_LINE; break;
      case GTOOL_POINT:
	roitype = ROI_POINT; break;
      case GTOOL_RECT:
	roitype = ROI_BOX; break;
//      case GTOOL_OVAL:
//	roitype = ROI_OVAL; break;
      case GTOOL_PGON:
	roitype = ROI_POLYGON;
	break;
      case GTOOL_PGON_OPEN:
	roitype = ROI_POLYGON_OPEN;
	break;
      case GTOOL_TEXT:
	roitype = ROI_TEXT;
	break;
      default:
	roitype = (Roitype)-1;
    }
    return roitype;
}
 
/************************************************************************
*									*
*  Initialize anything related to ROI.  It is called when the user just	*
*  selects any ROI tool.						*
*									*/
void
Roi_routine::start(Panel props, Gtype gtype)
{
    roitype = gtoolTypeToRoitoolType(gtype);
    if (roitype < 0){
	STDERR("Bug: Roi_routine::start: Unknown Roitool");
	return;
    }
    
    active = TRUE;			// ROI is active
    
    Roitool::acttype = ROI_CREATE;
    //debug << "Roitool::acttype = ROI_CREATE" << endl;
    //debug << "roitype = " << (int) roitype << endl;
    //Roitool::acttype = ROI_NO_ACTION;
    
    xv_set(props, PANEL_ITEM_MENU, props_menu, NULL);
    Gtools::set_props_label("ROI Properties");
    
    //roitool[roitype]->draw();		// Draw previous tool if any
}

/************************************************************************
*									*
*  Constructor for the Roitool base class				*
*									*/
Roitool::Roitool() {
    owner_frame = 0;
    bkg_pixmap = FALSE;
    my_color = copy_color;
    markable = TRUE;
    basex = basey = G_INIT_POS;
}

/************************************************************************
*									*
*  Destructor for the Roitool base class				*
*									*/
Roitool::~Roitool()
{
}

/************************************************************************
*									*
*  Creator for Roi_routine.						*
*  <This function can only be called once.>				*
*									*/
Roi_routine::Roi_routine(Gdev *gd)
{
  int i;

  roitype = ROI_LINE;			// initialize
  //debug << "roitype = ROI_LINE" << endl;
  active = FALSE;			// ROI is NOT active yet
  
  // Create ROI tool instances
  roitool[ROI_SELECTOR] = new Selector;
  roitool[ROI_LINE] = new Line;
  roitool[ROI_POINT] = new Point;
  roitool[ROI_BOX] = new Box;
  roitool[ROI_OVAL] = new Oval;
  roitool[ROI_POLYGON] = new Polygon;
  roitool[ROI_TEXT] = new Label;
  ((Polygon*)roitool[ROI_POLYGON])->set_closed(TRUE);
  roitool[ROI_POLYGON_OPEN] = new Polygon;
  ((Polygon*)roitool[ROI_POLYGON_OPEN])->set_closed(FALSE);

  int ncolors = G_Get_Sizecms1(gd);
  int ncols = 2;
  int nrows = 1 + (ncolors - 1) / ncols;

  color_menu = xv_create(NULL, MENU_COMMAND_MENU,
			 MENU_NOTIFY_PROC, &Roitool::roi_menu_color,
			 MENU_NCOLS, ncols,
			 MENU_NROWS, nrows,
			 NULL);
  Menu_item mi;
  for (i=0; i<ncolors; i++){
      mi = (Menu_item)xv_create(NULL, MENUITEM,
				MENU_IMAGE, get_color_chip(i),
				MENU_VALUE, (Xv_opaque)i,
				NULL);
      xv_set(color_menu, MENU_APPEND_ITEM, mi,
	     MENU_NCOLS, ncols,
	     MENU_NROWS, nrows,
	     NULL);
  }

  Menu fontsize_menu =
    xv_create(NULL, MENU_COMMAND_MENU,
	      MENU_NOTIFY_PROC, &Roi_routine::fontsize_menu_handler,
	      MENU_STRINGS, "10 pt", "12 pt", "14 pt", "19 pt", NULL,
	      NULL);
  Label::default_font_size = FONT_MEDIUM;

  props_menu =
  xv_create(NULL,	MENU,
	    MENU_GEN_PIN_WINDOW, Gtools::get_gtools_frame(), "ROI Props",
	    MENU_ITEM,
	    MENU_STRING,		"Delete",
	    MENU_NOTIFY_PROC,		&Roi_routine::menu_handler,
	    MENU_CLIENT_DATA,	ROI_MENU_DELETE,
	    NULL,
//	      MENU_ITEM,
//	      MENU_STRING,		"UnDelete",
//	      MENU_CLIENT_DATA,	ROI_MENU_UNDELETE,
//	      NULL,
//	      MENU_ITEM,
//	      MENU_STRING,		"Mark",
//	      MENU_CLIENT_DATA,	ROI_MENU_MARK,
//	      NULL,
	    MENU_ITEM,
	    MENU_STRING,		"Load",
	    MENU_NOTIFY_PROC,		&Roi_routine::menu_handler,
	    MENU_CLIENT_DATA,	ROI_MENU_LOAD,
	    MENU_GEN_PULLRIGHT,	&Roi_routine::roi_menu_load_pullright,
	    NULL,
	    MENU_ITEM,
	    MENU_STRING,		"Save ...",
	    MENU_NOTIFY_PROC,		&Roi_routine::menu_handler,
	    MENU_CLIENT_DATA,	ROI_MENU_SAVE,
	    NULL,
	    MENU_ITEM,
	    MENU_STRING,		"Color",
	    MENU_NOTIFY_PROC,		&Roi_routine::menu_handler,
	    MENU_PULLRIGHT,		color_menu,
	    NULL,
	    MENU_ITEM,
	    MENU_STRING, 		"Font Size",
	    MENU_NOTIFY_PROC,		&Roi_routine::menu_handler,
	    MENU_PULLRIGHT, fontsize_menu,
	    NULL,
	    MENU_ITEM,
	    MENU_STRING,		"Cursor Tolerance ...",
	    MENU_NOTIFY_PROC,		&Roi_routine::menu_handler,
	    MENU_CLIENT_DATA,	ROI_MENU_APERTURE,
	    NULL,
	    NULL); 

   Roi_routine::bind_menu_item =
   (Menu_item)xv_create(NULL, MENUITEM,
			MENU_STRING, "Bind",
			MENU_NOTIFY_PROC, &Roi_routine::menu_handler,
			MENU_CLIENT_DATA, ROI_MENU_BIND,
			NULL);
   xv_set(props_menu, MENU_APPEND_ITEM, Roi_routine::bind_menu_item, NULL);
   Roitool::bind = FALSE;

  Menu_item mitem =
  (Menu_item)xv_create(NULL, MENUITEM,
		       MENU_STRING, "ROI tracking ...",
		       MENU_NOTIFY_PROC, &Roi_routine::menu_handler,
		       MENU_CLIENT_DATA, ROI_MENU_TRACKING,
		       NULL);
   xv_set(props_menu, MENU_APPEND_ITEM, mitem, NULL);

  // Initialize Roi static variables
  Roitool::gdev = gd;
  Roitool::aperture = G_APERTURE;
  
  // Assign value for XOR'ing to be the last gray value
  Roitool::xor_color = G_Get_Stcms2(gd) + G_Get_Sizecms2(gd) - 1;

  // Assign color to be the second marker value (red?)
  Roitool::copy_color = G_Get_Stcms1(gd) + 1;

  // Assign color to be used while moving the ROI in copy mode (purple?)
  Roitool::active_color = G_Get_Stcms1(gd) + 8;

  // Start with xor_color by defualt
  Roitool::color = Roitool::xor_color;

  // Create the pointer to a list of graphics tools objects
  gObjects = new RoitoolList;

  // Create the pointer to the deleted objects stack
  deleteStack = new RoitoolList;

  // Create the pointer to the list of selected ROIs
  selected_ROIs = new RoitoolList;

  // List of active ROIs
  active_tools = new RoitoolList;
  clone_tools = new RoitoolList;

  filelist_notify_func(FILELIST_MENU_ID, FILELIST_NEW, NULL, 
	(long)&Roi_routine::roi_menu_save);
  
  // Set the default directory path
  char filename[1024];
  (void)init_get_env_name(filename);
  (void)strcat(filename, "/roi");
  filelist_set_directory(FILELIST_MENU_ID, filename);
}

/************************************************************************
*                                                                       *
*  Clean-up routine <about to leave one of ROI tool>.  It is called when*
*  the user has selected another tool.                                  *
*                                                                       */
void
Roi_routine::end(void)
{
//  roitool[roitype]->erase();		// Erase previous tool
  
  G_Set_LineWidth(Roitool::gdev, 1);
  
  active = FALSE;			// ROI is NOT active
}

/************************************************************************
*									*
*  Notifier for the color-selection pull-right menu.
*  <STATIC>
*/
void
Roitool::roi_menu_color(Menu, Menu_item i)
{
    int save_color;

    if (copy_color == color){
	save_color = color = copy_color = (int)xv_get(i, MENU_VALUE);
    }else{
	save_color = color;
	color = copy_color = (int)xv_get(i, MENU_VALUE);
    }

    RoitoolIterator element(selected_ROIs);
    Roitool *tool;
    while (tool = ++element){
	if (tool->my_color != tool->copy_color){
	    tool->my_color = tool->copy_color;
	    tool->owner_frame->imginfo->display_ood = TRUE;
	}
    }
    Frame_data::redraw_ood_images();

    color = save_color;
}

/************************************************************************
*									*
*  Font size sub-menu handler for ROI.					*
*  <STATIC>								*
*									*/
void
Roi_routine::fontsize_menu_handler(Menu, Menu_item i)
{
    char font[10];
    strcpy(font, (char *)(xv_get(i, MENU_STRING)));
    if (strncmp("10", font, 2) == 0){
	Label::default_font_size = FONT_SMALL;
    }else if(strncmp("12", font, 2) == 0){
	Label::default_font_size = FONT_MEDIUM;
    }else if(strncmp("14", font, 2) == 0){
	Label::default_font_size = FONT_LARGE;
    }else if(strncmp("19", font, 2) == 0){
	Label::default_font_size = FONT_EXTRALARGE;
    }else{
	fprintf(stderr,"fontsize_menu_handler: bad menu string\n");
	return;
    }
    RoitoolIterator element(selected_ROIs);
    Roitool *tool;
    while (tool = ++element){
	if (strcasecmp(tool->name(), "label") == 0){
	    Label *label = (Label *)tool;
	    if (label->font_size != label->default_font_size){
		label->font_size = label->default_font_size;
		label->update_screen_coords();
		label->owner_frame->imginfo->display_ood = TRUE;
	    }
	}
    }
    Frame_data::redraw_ood_images();
}

/************************************************************************
*									*
*  Properties menu handler for ROI.					*
*  <STATIC>								*
*									*/
void
Roi_routine::menu_handler(Menu, Menu_item i)
{
    RoitoolIterator element(selected_ROIs);
    
    if (((Roi_props_menu)xv_get(i, MENU_CLIENT_DATA)) == ROI_MENU_UNDELETE) {
	UnDeleteObject();
	return;
    }
    
    switch ((Roi_props_menu)xv_get(i, MENU_CLIENT_DATA)){
	
      case ROI_MENU_DELETE:
	DeleteSelectedTools();
	break;
	
      case ROI_MENU_UNDELETE:
	// Shouldn't get here
	break;
	
      case ROI_MENU_MARK:
	Roitool *tool;
	while (tool = ++element){
	    if (tool->roi_state(ROI_STATE_MARK)){
		tool->roi_clear_state(ROI_STATE_MARK);
		tool->owner_frame->display_data();	// Update image display
	    }else{
		tool->roi_set_state(ROI_STATE_MARK);
		tool->draw_solid();
	    }
	}
	break;
	
      case ROI_MENU_LOAD:
	// We don't need to handle this case, because it is handled by
	// roi_menu_load_pullright and roi_menu_load, because it generates a
	// pullright menu.
	break;
	
      case ROI_MENU_SAVE:
	{
	    char filename[1024];
	    (void)init_get_env_name(filename);
	    (void)strcat(filename, "/roi");
	    filelist_win_show(FILELIST_MENU_ID, FILELIST_SAVE, filename,
			      "Save ROIs");
	}
	break;
	
      case ROI_MENU_APERTURE:
	inputwin_show((int)ROI_MENU_APERTURE, &Roitool::set_attr,
		      "ROI Cursor Tolerance (Range: 1 - 20)");
	break;
      case ROI_MENU_TRACKING:
	inputwin_show((int)ROI_MENU_TRACKING, &Roitool::set_attr,
		      "Max Tracking ROIs (Range: 0 - 200)");
	break;
      case ROI_MENU_BIND:
         if (Roitool::bind){
	     Roi_routine::set_bind(FALSE);
	 }else{
	     Roi_routine::set_bind(TRUE);
	 }
	 break;
    }
}

/************************************************************************
*									*
*  Set the "binding" choice
*  [MACRO interface]
*  argv[0]: (char *) on | off
*  [STATIC Function]							*
*									*/
int
Roi_routine::Bind(int argc, char **argv, int, char **)
{
    argc--; argv++;

    char *flag;
    if (argc != 1){
	ABORT;
    }
    flag = argv[0];
    if (strcasecmp(flag, "on") == 0){
	set_bind(TRUE);
    }else if (strcasecmp(flag, "off") == 0){
	set_bind(FALSE);
    }else{
	ABORT;
    }
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Set the "binding" choice
*  [STATIC Function]							*
*									*/
void
Roi_routine::set_bind(int flag)
{
    char *sflag;
    if (flag){
	xv_set(bind_menu_item, MENU_STRING, "Unbind", NULL);
	Roitool::bind = TRUE;
	sflag = "on";
    }else{
	xv_set(bind_menu_item, MENU_STRING, "Bind", NULL);
	Roitool::bind = FALSE;
	sflag = "off";
    }
    macroexec->record("roi_bind('%s')\n", sflag);
    return;
}

/************************************************************************
*									*
*  User has selected a pulright menu for load procedure.  Generate	*
*  the listing of directory files.  Note that it registers 		*
*  "roi_menu_load" function to call if there is a selection.		*
*  <STATIC>								*
*									*/
Menu
Roi_routine::roi_menu_load_pullright(Menu_item mi, Menu_generate op)
{
    if (op == MENU_DISPLAY){
	char fname[1024];
	(void)init_get_env_name(fname);	/* Get the directory path */
	(void)strcat(fname, "/roi");
	return(filelist_menu_pullright(mi, op, (u_long)roi_menu_load, fname));
    }
    return(filelist_menu_pullright(mi, op, NULL, NULL));
}

/************************************************************************
*									*
*  Load ROIs
*  [MACRO interface]
*  argv[0]: (char *) Full path of file or relative path from $BROWSERDIR/roi.
*  [STATIC Function]							*
*									*/
int
Roi_routine::Load(int argc, char **argv, int, char **)
{
    argc--; argv++;

    if (argc != 1){
	ABORT;
    }
    char *filename = argv[0];
    char fname[1024];
    if (*filename == '/'){
	*fname = '\0';
    }else{
	init_get_env_name(fname);	/* Get the directory path */
	strcat(fname, "/roi/");
    }
    strcat(fname, filename);
    roi_menu_load("", fname);
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  The user has selected a menu item. Execute to load ROI.		*
*  <STATIC>								*
*									*/
void
Roi_routine::roi_menu_load(char *dirpath,	// directory path name
			   char *name)		// filename to be loaded
{
    ifstream infile;	// input stream
    char filename[128];	// complte filename
    
    if (*name == NULL){
	msgerr_print("Need to specify input filename for loading"); 
	return;
    }
    
    (void)sprintf(filename, "%s/%s", dirpath, name);
    
    infile.open(filename, ios::in);
    if (infile.fail()){
	msgerr_print("Couldn't open \"%s\" for reading", filename);
	return;
    }
    
    Roitool::load_roi(infile);
    infile.close(); 
    macroexec->record("roi_load('%s')\n", filename);
}

/************************************************************************
*									*
*  Save ROIs
*  [MACRO interface]
*  argv[0]: (char *) Full path of file
*  [STATIC Function]							*
*									*/
int
Roi_routine::Save(int argc, char **argv, int, char **)
{
    argc--; argv++;

    if (argc != 1){
	ABORT;
    }
    char *filename = argv[0];
    char fname[1024];
    if (*filename == '/'){
	*fname = '\0';
    }else{
	init_get_env_name(fname);	/* Get the directory path */
	strcat(fname, "/roi/");
    }
    strcat(fname, filename);
    roi_menu_save("", fname);
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  The user wants to save ROI tool. Execute to save ROI.		*
*  <STATIC>								*
*									*/
void
Roi_routine::roi_menu_save(char *dirpath, 	// directory path name
			   char *name)		// filename to be loaded
{
    ofstream outfile;	// output stream
    char filename[1024]; // complete filename
    long clock;		// number of today seconds
    char *tdate;	// pointer to the time
    char *tlogin;	// pointer to login name
    char thost[80];	// hostname buffer
    
    if ( ! selected_ROIs->Count() ){
	msgerr_print("No ROIs are selected");
	return;
    }
    
    if (*name == NULL){
	msgerr_print("Need to specify output filename for saving");
	return;
    }
    
    (void)sprintf(filename,"%s/%s", dirpath, name);
    
    outfile.open(filename, ios::out);
    if (outfile.fail()){
	msgerr_print("Couldn't open \"%s\" for writing", filename);
	return;
    }
    
    // Output the comments
    clock = time(NULL);
    if ((tdate = ctime(&clock)) != NULL){
	tdate[strlen(tdate)-1] = 0;
    }
    if ((tlogin = (char *)cuserid(NULL)) == NULL){
	msgerr_print("Warning: Couldn't find login name");
    }
    if (gethostname(thost,80) != 0){
	msgerr_print("Warning:Couldn't find host name");
    }
    
    outfile << "# ** Created by " << tlogin << " on " << tdate
    	    << " at machine " << thost << " **" << "\n";

    // Now save the ROIs
    RoitoolIterator element(selected_ROIs);
    Roitool *tool;
    while (tool = ++element){
	tool->save(outfile);
    }
    
    outfile.close();
    
    filelist_update();
    macroexec->record("roi_save('%s')\n", filename);
}

/************************************************************************
*									*
*  Return NULL if ROI tool is not active, and address of active tool	*
*  if it is active.							*
*  <STATIC>								*
*									*/
Roitool *
Roi_routine::get_roi_tool(void)
{
    if (active){
	return(selected_ROIs->Top());
    }else{
	return(NULL);
    }
}

/************************************************************************
*									*
*  Erase ROI.
*  This routine is used by outside ROI functions or static functions that
*  need to erase the selected ROIs.
* <STATIC>								*
*									*/
void
Roi_routine::roi_erase(void)
{
    RoitoolIterator element(selected_ROIs);
    Roitool *tool;
    while (tool = ++element){
	tool->erase();
    }
}

/************************************************************************
*									*
*  Draw ROI.								*
*  This routine is used by outside ROI functions or static function who	*
*  needs to draw ROI.							*
* <STATIC>								*
*									*/
void
Roi_routine::roi_draw(void)
{
    RoitoolIterator element(selected_ROIs);
    Roitool *tool;
    while (tool = ++element){
	tool->draw();
    }
}

/************************************************************************
 *									*
 *  Create and initialize a copy of an ROItool in a given gframe
 *									*/
Roitool *
Roi_routine::create_roitool(Gframe *gframe)
{
    Roitool *tool;
    short x = 0;
    short y = 0;

    tool = roitool[roitype]->recreate();
    tool->owner_frame = gframe;
    tool->init_point();
    tool->setactive();
    tool->save_bkg();
    return tool;
}

/************************************************************************
*                                                                       *
*  Find the min and max values in the ROI
*									*/
void
Roitool::get_minmax(double *min, double *max)
{
    register float *data;
    if ((data = FirstPixel()) == 0){
	// No pixels in this ROI.
	return;
    }
    
    register float fmin, fmax;
    for (fmin=fmax=*data; data=NextPixel(); ){
	if (*data > fmax){
	    fmax = *data;
	}else if (*data < fmin){
	    fmin = *data;
	}
	
    }

    *min = fmin;
    *max = fmax;
}

/************************************************************************
*                                                                       *
*  Find the histogram and statistics within the ROI.
*  Only pixels with data between the "bot" and "top" values are considered
*  in calculating the statistics.  This will also be the range spanned
*  by the histogram.
*  Return TRUE if successful, otherwise FALSE.
*									*/
int
Roitool::histostats(int nbins, double bot, double top, Stats *stats)
{
    register int index;
    double thickness;
    double z_location;
    Imginfo *img = owner_frame->imginfo;

    register float *data;
    if ((data = FirstPixel()) == 0){
	// No pixels in this ROI.
	return FALSE;
    }

    // Calculate the max, min, and mean from all data in ROI that
    // is within the bot and top limits.
    int npixels = 0;
    double mean = 0;
    float min = *data;
    float max = *data;
    for ( ; data; data=NextPixel() ){
	if (*data >= bot && *data <= top){
	    if (*data > max){
		max = *data;
	    }else if (*data < min){
		min = *data;
	    }
	    mean += *data;
	    npixels++;
	}
    }
    if (npixels){
	mean /= npixels;
	
	// Construct the histogram and calculate standard deviation.
	stats->histogram = new Histogram;
	stats->histogram->nbins = nbins;
	stats->histogram->bottom = bot;
	stats->histogram->top = top;
	int *hist = stats->histogram->counts = new int[nbins];
	for (index=0; index<nbins; index++){
	    hist[index] = 0;
	}
	
	double sdv = 0;
	int low_bin = 0;	// Hold counts of number of off scale bins.
	int high_bin = 0;	// ...A good idea, but we don't use them.
	float offset = bot;
	if (top == bot){
	    // Degenerate case; put all counts in middle bin
	    hist[nbins/2] = npixels;
	    sdv = 0;
	    stats->median = mean;
	}else{
	    float scale = nbins / (top - bot);
	    for (data=FirstPixel(); data; data=NextPixel() ){
		index = (int)(scale * (*data - offset));
		if (*data == top){
		    // Include border case in top bin
		    // (otherwise always lose top pixel when we auto-scale)
		    index = nbins - 1;
		}
		if (*data < bot){
		    low_bin++;
		}else if (*data > top){
		    high_bin++;
		}else{
		    sdv += (*data - mean) * (*data - mean);
		    hist[index]++;
		}
	    }
	    if (npixels > 1){
		sdv = sqrt(sdv / (npixels - 1));
	    }
	    
	    // Calculate approximate median from the histogram.
	    int ipixel = 0;
	    for (index=0; index<nbins; index++){
		ipixel += hist[index];
		if ((ipixel * 2) > npixels){
		    break;
		}
	    }
	    stats->median = bot + (index + 0.5) * (top - bot) / nbins;
	}
	
	stats->min = min;
	stats->max = max;
	stats->mean = mean;
	stats->sdv = sdv;
    }

    // Calculate and load the area in sq cm.
    // ASSUME THE "SPAN" (GetRatioFast/GetRatioMedium) GIVES THE DISTANCE
    // FROM ONE EDGE OF THE PICTURE TO THE OTHER, RATHER THAN THE
    // DISTANCE BETWEEN CENTERS OF THE OPPOSITE EDGE PIXELS.
    // THEREFORE, WE MULTIPLY BY THE NUMBER OF PIXELS, RATHER THAN THE
    // NUMBER OF PIXELS MINUS 1.
    stats->area = (npixels * img->GetRatioFast() / img->GetFast()
		   * img->GetRatioMedium() / img->GetMedium() );

    // Get some additional stuff to put in
    stats->framenum = owner_frame->get_frame_number();
    stats->npixels = npixels;
    img->st->GetValue("location", z_location, 2);
    stats->z_location = z_location;
    img->st->GetValue("roi", thickness, 2);
    stats->thickness = thickness;
    stats->volume = stats->area * thickness;

    return TRUE;
}
    
/************************************************************************
*                                                                       *
*  Find the histogram and statistics within the ROI.
*									*/
void Roitool::histostats(int *hist,
			 int nbins,
			 double *min,
			 double *max,	// If non-zero on input, calculate it
			 double *median,
			 double *mean,
			 double *sdv,
			 double *area)
{
    register int index;
    register float offset;
    register float scale;
    int npixels = 0;
    float bot;		// Range of histogram binning
    float top = 0;
    float *data;
    Imginfo *img = owner_frame->imginfo;

    if ((data = FirstPixel()) == 0){
	// No pixels in this ROI.
	*min = *max = *median = *mean = *sdv = *area = 0;
	for (index=0; index<nbins; hist[index++]=0);
	return;
    }

    // Set the range of the histogram.  If "max" is non-zero,
    // use min and max for the limits; otherwise use new values
    // of min and max that we will calculate below from the data.
    if (*max){
	bot = *min;
	top = *max;

	// Calculate the max, min, and mean from all data in ROI that
	// is within the bot and top limits.
	*min = *max = *data;
	*mean = 0;
	for ( ; data; data=NextPixel() ){
	    if (*data >= bot && *data <= top){
		if (*data > *max){
		    *max = *data;
		}else if (*data < *min){
		    *min = *data;
		}
		*mean += *data;
		npixels++;
	    }
	}
	*mean /= npixels;
    }else{
	// Calculate the max, min, and mean from all data in ROI.
	*min = *max = *data;
	*mean = 0;
	for ( ; data; data=NextPixel() ){
	    if (*data > *max){
		*max = *data;
	    }else if (*data < *min){
		*min = *data;
	    }
	    *mean += *data;
	    npixels++;
	}
	*mean /= npixels;

	// Set the range of the histogram.
	bot = *min;
	top = *max;
    }

    // Construct the histogram and calculate standard deviation.
    for (index=0; index<nbins; index++){
	hist[index] = 0;
    }
    *sdv = 0;
    int low_bin = 0;	// Hold counts of number of bins that are off scale.
    int high_bin = 0;

    offset = bot;
    scale = nbins / (top - bot);
    for (data=FirstPixel(); data; data=NextPixel() ){
	*sdv += (*data - *mean) * (*data - *mean);
	index = (int)(scale * (*data - offset));
	if (*data == top){
	    index = nbins - 1;
	}
	if (*data < bot){
	    low_bin++;
	}else if (*data > top){
	    high_bin++;
	}else{
	    hist[index]++;
	}
    }
    *sdv = sqrt(*sdv / (npixels - 1));
    // ASSUME THE "SPAN" (GetRatioFast/GetRatioMedium) GIVES THE DISTANCE
    // FROM ONE EDGE OF THE PICTURE TO THE OTHER, RATHER THAN THE
    // DISTANCE BETWEEN CENTERS OF THE OPPOSITE EDGE PIXELS.
    // THEREFORE, WE DIVIDE BY THE NUMBER OF PIXELS, RATHER THAN THE
    // NUMBER OF PIXELS MINUS 1.
    *area = (npixels * img->GetRatioFast() / (img->GetFast() - 0)
	     * img->GetRatioMedium() / (img->GetMedium() - 0));

    // Calculate approximate median from the histogram.
    int ipixel = low_bin;
    for (index=0; index<nbins; index++){
	ipixel += hist[index];
	if ((ipixel * 2) > npixels){
	    break;
	}
    }
    *median = bot + (index + 0.5) * (top - bot) / nbins;

    return;
}

/************************************************************************
*									*
*  Zeros out all pixels that are inside the ROI and are outside the
*  intensity range (min <= data <= max).
*									*/
void
Roitool::zero_out(double min, double max)
{
    float *data = FirstPixel();
    if (!data){
	msgerr_print("Roitool::zero_out(): Please select an ROI first");
    }

    for ( ; data; data=NextPixel() ){
	if ((*data > max) || (*data < min)){
	    *data = 0.0;
	}
    }
}

/************************************************************************
*									*
*  Zeros out all pixels that are outside the selected ROIs on this image,
*  or inside an ROI but outside the intensity range (min <= data <= max).
*  STATIC
*									*/
void
Roitool::segment_selected_rois(Gframe *gptr,
			       int min_defined,
			       double min,
			       int max_defined,
			       double max)
{
    char *cptr;
    float *data;
    float *fptr = (float *)gptr->imginfo->GetData();
    register float fmin = min;
    register float fmax = max;

    // Initialize a flag buffer the size of the image
    int size = gptr->imginfo->GetFast() * gptr->imginfo->GetMedium();
    char *flag = new char[size];
    (void)memset(flag, 0, size);
    char *cend = flag + size;

    // For every selected ROI
    RoitoolIterator element(selected_ROIs);
    Roitool *tool;
    while (tool = ++element){
	// If ROI is in this frame ...
	if (tool->owner_frame == gptr){
	    // ... flag the pixels inside the ROI
	    for ( data=tool->FirstPixel(); data; data=tool->NextPixel() ){
		flag[data-fptr] = 1;
	    }
	}
    }

    // Clip flagged points; zero unflagged points
    if (min_defined && max_defined){
	for (cptr=flag; cptr<cend; cptr++, fptr++){
	    if (!*cptr || *fptr < fmin || *fptr > fmax){
		*fptr = 0.0;
	    }
	}
    }else if (min_defined){
	for (cptr=flag; cptr<cend; cptr++, fptr++){
	    if (!*cptr || *fptr < fmin){
		*fptr = 0.0;
	    }
	}
    }else if (max_defined){
	for (cptr=flag; cptr<cend; cptr++, fptr++){
	    if (!*cptr || *fptr > fmax){
		*fptr = 0.0;
	    }
	}
    }else{
	for (cptr=flag; cptr<cend; cptr++, fptr++){
	    if (!*cptr){
		*fptr = 0.0;
	    }
	}
    }
    delete [] flag;
}

/************************************************************************
*									*
*  These are the default "iterator" functions used by those ROI objects
*  for which no valid histogram can be computed.
*									*/
float *
Roitool::FirstPixel()
{
    return 0;
}

float *
Roitool::NextPixel()
{
    return 0;
}

/************************************************************************
*									*
*  Set ROI attributes:color and aperture.				*
*  Return OK or NOT_OK.							*
*  <STATIC>								*
*									*/
int
Roitool::set_attr(int id, char *attr_str)
{
    int val;	// user specified value

    if (id == ROI_MENU_APERTURE){
	if (*attr_str == '?'){
	    msginfo_print("Aperture = %d\n", aperture);
	}else if (((val = atoi(attr_str)) < 1) || (val > 20)){
	    msgerr_print("Aperture value is not within limits");
	    return(NOT_OK);
	}
	else{
	    aperture = val;
	}
    }else if (id == ROI_MENU_TRACKING){
	if (*attr_str == '?'){
	    msginfo_print("Max tracking ROIs = %d\n", max_active_tools);
	}else if (((val = atoi(attr_str)) < 0) || (val > 200)){
	    msgerr_print("Max tracking tools value is not within limits");
	    return(NOT_OK);
	}else{
	    max_active_tools = val;
	}
    }
    return(OK);
}

/************************************************************************
*									*
*  Set the default drawable for the display.  Normally the screen, but
*  sometimes we may want to draw only on a pixmap.
*
*  Returns the original drawable, so we can set it back.
*									*/
XID
Roitool::set_drawable(XID drawable)
{
    Gdev *gdevice = Gframe::gdev;
    XID old_xid = gdevice->xid;
    gdevice->xid = drawable;
    return old_xid;
}

Roitool *
Roitool::copy(Gframe *gframe)
{
	return NULL;
}

/************************************************************************
*									*
*  Draw the ROI
*									*/
void
Roitool::draw(void)
{
   if ((pnt == NULL) || pnt[0].x == G_INIT_POS)
     return;

   set_clip_region(FRAME_CLIP_TO_IMAGE);
   calc_xyminmax();

   if (visibility != VISIBLE_NEVER && visible != FALSE){
       XSetLineAttributes(gdev->xdpy, gdev->xgc,
			   0, LineSolid, CapButt, JoinBevel);
       g_draw_connected_lines(gdev, pnt, npnts, color);
   }
   roi_set_state(ROI_STATE_EXIST);

   if (roi_state(ROI_STATE_MARK))
      mark();

   set_clip_region(FRAME_NO_CLIP);
}

/************************************************************************
*									*
*  Get a new bkg_pixmap_id appropriate for canvas size.
*  If canvas is smaller than old pixmap, nothing is done.
*  If canvas is larger, a new pixmap is allocated, the old pixmap
*  is copied to the new one, and the old one is freed.
*
*  STATIC
*									*/
Flag
Roitool::allocate_bkg()
{
    XWindowAttributes  win_attr;
    int cwidth = Gdev_Win_Width(gdev);
    int cheight = Gdev_Win_Height(gdev);
    int cdepth = DefaultDepth(gdev->xdpy, DefaultScreen(gdev->xdpy));

    if (XGetWindowAttributes(gdev->xdpy, gdev->xid, &win_attr))
         cdepth = win_attr.depth;
/*
         cdepth = win_attr.visual->bits_per_rgb;
*/

    if (!bkg_pixmap_id || cwidth > bkg_width || cheight > bkg_height){
	// Get a new (larger) pixmap.
	cwidth = cwidth > bkg_width ? cwidth : bkg_width;
	cheight = cheight > bkg_height ? cheight : bkg_height;
	Pixmap old_pixmap = bkg_pixmap_id;
	bkg_pixmap_id = XCreatePixmap(gdev->xdpy, gdev->xid,
				   cwidth, cheight, cdepth);
	if (!bkg_pixmap_id){
	    return FALSE;
	}
	if (old_pixmap){
	    // Copy old contents to new pixmap
	    XCopyArea(gdev->xdpy,
		      old_pixmap, bkg_pixmap_id, // Source, Destination
		      gdev->xgc,
		      0, 0, bkg_width, bkg_height, // Source position and size
		      0, 0);			   // Destination position
	}
	bkg_width = cwidth;
	bkg_height = cheight;
    }
    if (!bkg_pixmap_id){
	return FALSE;
    }
    return TRUE;
}

/************************************************************************
*									*
*  Release background pixmap
*
*  STATIC
*									*/
void
Roitool::release_bkg()
{
    if (bkg_pixmap_id){
	XFreePixmap(gdev->xdpy, bkg_pixmap_id);
	bkg_pixmap_id = 0;
    }
    // Clear bkg_pixmap for every roitool
    Gframe *frame;
    for (frame = Gframe::get_first_frame();
	 frame;
	 frame = Gframe::get_next_frame(frame) )
    {
	Imginfo *img;
	if (img=frame->imginfo){
	    RoitoolIterator element(img->display_list);
	    Roitool *tool;
	    while (tool = ++element){
		tool->bkg_pixmap = FALSE;
	    }
	}
    }
}

/************************************************************************
*									*
*  Mark stored background invalid
*									*/
void
Roitool::forget_bkg()
{
    bkg_pixmap = FALSE;
}

/************************************************************************
*									*
*  Save a copy of the "background" image.  This is everything that is
*  displayed in the area that the ROI can move around in, except the
*  ROI itself.
*
*  Returns TRUE if it's saved, otherwise returns FALSE.
*									*/
Flag
Roitool::save_bkg()
{
    Gframe *frame;
    Imginfo *img;
    if (!(frame=owner_frame) || !(img=frame->imginfo) || !img->pixmap) {
	fprintf(stderr,"Cannot store background in pixmap\n");
	return FALSE;
    }
    if (!bkg_pixmap_id){
	allocate_bkg();
    }
    if (!bkg_pixmap_id){
	return FALSE;
    }

    // Copy image to bkg_pixmap_id
    XCopyArea(gdev->xdpy, img->pixmap, bkg_pixmap_id,
	      gdev->xgc,
	      0, 0, img->pixwd, img->pixht,	// Source position and size
	      img->pixstx, img->pixsty);	// Destination position

    // Draw all ROIs except this one on bkg_pixmap_id
    XID old_xid = set_drawable(bkg_pixmap_id);	// Draw on pixmap--not screen
    RoitoolIterator element(img->display_list);
    Roitool *tool;
    Gmode old_mode = setcopy();;
    while (tool = ++element){
	if (tool != this){
	    tool->setcopy();
	    tool->draw();
	}
    }
    setGmode(old_mode);
    set_drawable(old_xid);			// Draw on screen again
    bkg_pixmap = TRUE;

    return TRUE;
}

/************************************************************************
*									*
*  Refresh the "background" image, overwriting the ROI
*									*/
Flag
Roitool::redisplay_bkg(int x1, int y1, int x2, int y2)
{
    Flag rtn = FALSE;

    if (bkg_pixmap && bkg_pixmap_id && owner_frame && owner_frame->imginfo){
	Gdev *gdevice = Gframe::gdev;
	int xmin = x1 < x2 ? x1 : x2;
	int ymin = y1 < y2 ? y1 : y2;
	int xmax = x1 > x2 ? x1 : x2;
	int ymax = y1 > y2 ? y1 : y2;

	Imginfo *img = owner_frame->imginfo;
	if (xmin < img->pixstx){
	    xmin = img->pixstx;
	}else if (xmin > img->pixstx + img->pixwd - 1){
	    xmin = img->pixstx + img->pixwd -1;
	}
	if (ymin < img->pixsty){
	    ymin = img->pixsty;
	}else if (ymin > img->pixsty + img->pixht - 1){
	    ymin = img->pixsty + img->pixht - 1;
	}
	if (xmax < img->pixstx){
	    xmax = img->pixstx;
	}else if (xmax > img->pixstx + img->pixwd - 1){
	    xmax = img->pixstx + img->pixwd -1;
	}
	if (ymax < img->pixsty){
	    ymax = img->pixsty;
	}else if (ymax > img->pixsty + img->pixht - 1){
	    ymax = img->pixsty + img->pixht - 1;
	}
	// Set drawing op to "copy", remembering current op
	int prev_op = G_Get_Op(gdevice);
	G_Set_Op(gdevice, GXcopy);
	XCopyArea(gdevice->xdpy,
		  bkg_pixmap_id, gdevice->xid,	// Source, Destination
		  gdevice->xgc,
		  xmin, ymin,			// Source rectangle position
		  xmax-xmin+1, ymax-ymin+1,	// Size of rectangle
		  xmin, ymin);			// Dest rectangle position
	
	/* restore the original X draw operation */
	G_Set_Op(gdevice, prev_op);
	rtn = TRUE;
    }
    return rtn;
}

/************************************************************************
*									*
*  Erase an ROI
*									*/
void
Roitool::erase(void)
{
    if ( ! redisplay_bkg(x_min, y_min, x_max, y_max)){
	draw();	// (Assumes drawing in XOR mode)
    }
    roi_clear_state(ROI_STATE_EXIST);
}

/************************************************************************
*									*
*  Mark all the points.							*
*									*/
void
Roitool::mark(void)
{
    if (!markable) return;
    int i;
    for (i=0; i<npnts; i++)
    {
	draw_mark(pnt[i].x, pnt[i].y);
    }
}

/************************************************************************
*									*
*  Load ROIs from a file.
*  Looks for lines giving the name of the ROI type (line, box, etc.)
*  and calls the load routine for the appropriate type.
*									*/
void
Roitool::load_roi(ifstream &infile)
{
    const int buflen = 128;
    char buf[buflen];
    char roi_name[buflen];

    while ( infile.getline(buf, buflen) ){
	if (buf[0] == '#'){				// Ignore comment lines
	    continue;
	}else if (strspn(buf, "\t ") == strlen(buf)){	// Ignore blank lines
	    continue;
	}else{
	    sscanf(buf,"%127s",roi_name);	// Expect an ROI type name
	    int i;
	    for (i=1; i<ROI_NUM; i++){
		if ( strcasecmp(roi_name, roitool[i]->name()) == 0 ){
		    roitool[i]->load(infile);
		    break;
		}
	    }
	    if (i == ROI_NUM){
		msgerr_print("Unrecognized ROI type name in input file:");
		msgerr_print(buf);
		return;
	    }
	}
    }
    
    // Refresh the selected frames
    Gframe *gframe;
    int i = 1;
    for (gframe=Frame_select::get_selected_frame(i);
	 gframe;
	 gframe = Frame_select::get_selected_frame(++i))
    {
	if (gframe->imginfo){
	    gframe->display_data();
	}
    }
}


/************************************************************************
*									*
* Used after ROI creation/modification to set where the ROI is relative
* to the data.
* VIRTUAL
*									*/
void Roitool::update_data_coords()
{
    cout << "update_data_coords() not implemented" << endl;
}


/************************************************************************
*									*
* Used after image rotation to rotate the ROI.
* VIRTUAL
*									*/
void
Roitool::rot90_data_coords(int)
{
    cout << "rot90_data_coords() not implemented" << endl;
}


/************************************************************************
*									*
* Used after image flipping to flip the ROI.
* VIRTUAL
*									*/
void
Roitool::flip_data_coords(int)
{
    cout << "flip_data_coords() not implemented" << endl;
}


/************************************************************************
*									*
* Used after window move/resize/zoom to update our idea of where the
* is on the screen, which we need to know if we want to know if the
* mouse is on it.
*									*/
void
Roitool::update_screen_coords()
{
    cout << "update_screen_coords() not implemented" << endl;
}

/************************************************************************
*									*
* Functions to convert from pixel to data coords and back
*									*/
float
Roitool::xpix_to_data(int xp)
{
    Imginfo *img;

    if (owner_frame && (img=owner_frame->imginfo)){
	return img->XScreenToData(xp);
    }else{
	return 0;
    }
}

float
Roitool::ypix_to_data(int yp)
{
    Imginfo *img;

    if (owner_frame && (img=owner_frame->imginfo)){
	return img->YScreenToData(yp);
    }else{
	return 0;
    }
}

int
Roitool::data_to_xpix(float x)
{
    Imginfo *img;
    if (owner_frame && (img=owner_frame->imginfo)){
	return img->XDataToScreen(x);
    }else{
	return 0;
    }
}

int
Roitool::data_to_ypix(float y)
{
    Imginfo *img;
    if (owner_frame && (img=owner_frame->imginfo)){
	return img->YDataToScreen(y);
    }else{
	return 0;
    }
}



void
Roitool::keep_roi_in_image(short *x_motion, short *y_motion)
{
    short x_limit;
    short y_limit;
    Imginfo *img = 0;

    if (owner_frame){
	img = owner_frame->imginfo;
    }

    if (img == 0){
	return;
    }

    if ( (*x_motion >= 0) && (*y_motion >= 0) ){
	x_limit = x_max + *x_motion;
	y_limit = y_max + *y_motion;
	img->keep_point_in_image(&x_limit, &y_limit);
	*x_motion = x_limit - x_max;
	*y_motion = y_limit - y_max;

    }else if ( (*x_motion <= 0) && (*y_motion >= 0) ){
	x_limit = x_min + *x_motion;
	y_limit = y_max + *y_motion;
	img->keep_point_in_image(&x_limit, &y_limit);
	*x_motion = x_limit - x_min;
	*y_motion = y_limit - y_max;

    }else if ( (*y_motion <= 0) && (*x_motion >= 0) ){
	y_limit = y_min + *y_motion;
	x_limit = x_max + *x_motion;
	img->keep_point_in_image(&x_limit, &y_limit);
	*y_motion = y_limit - y_min;
	*x_motion = x_limit - x_max;

    }else if ( (*y_motion <= 0) && (*x_motion <= 0) ){
	y_limit = y_min + *y_motion;
	x_limit = x_min + *x_motion;
	img->keep_point_in_image(&x_limit, &y_limit);
	*y_motion = y_limit - y_min;
	*x_motion = x_limit - x_min;
    }
}

/************************************************************************
*									*
* Clip a point's coordinates to keep it inside the image
*									*/
void
Roitool::keep_point_in_image(short *x, short *y)
{
    Imginfo *img;

    if (owner_frame &&
	(img = owner_frame->imginfo) &&
	created_type != ROI_SELECTOR)
    {
	img->keep_point_in_image(x, y);
    }
}

/************************************************************************
*									*
* Set the clip region to be the image display area.
* "style" is FRAME_CLIP_TO_IMAGE or FRAME_NO_CLIP (to turn off clipping).
*									*/
void Roitool::set_clip_region(ClipStyle style)
{
    if (owner_frame){
	owner_frame->set_clip_region(style);
    }
}

/************************************************************************
*									*
* Redraw the ROI in copy mode to make it solid
*									*/
void Roitool::draw_solid()
{
    Gmode mode;

    if (owner_frame){
	mode = setcopy();
	owner_frame->set_clip_region(FRAME_CLIP_TO_IMAGE);
	draw();
	owner_frame->set_clip_region(FRAME_NO_CLIP);
	setGmode(mode);
    }
}    

/************************************************************************
*									*
* Returns TRUE is specified ROI is "similar" to this one
* in type, shape, and location.
*									*/
#define MATCHTOL 4
Flag
Roitool::matches(Roitool *tool)
{
    int i;
    Flag rtn = TRUE;
    if (npnts != tool->npnts){
	rtn = FALSE;
    }else if (strcasecmp(name(), tool->name()) != 0){
	rtn = FALSE;
    }else{
	// See if all the vertices are in the right places
	Imginfo *img1 = owner_frame->imginfo;
	Imginfo *img2 = tool->owner_frame->imginfo;
	Gpoint newpt;
	for (i=0; i<npnts; i++){
	    newpt = img2->ScreenToScreen(img1, tool->pnt[i].x, tool->pnt[i].y);
	    // NB: Check also for points matching in reverse order.
	    // This is needed for lines, which often get flipped.
	    if ((abs(newpt.x - pnt[i].x) > MATCHTOL
		 && abs(newpt.x - pnt[npnts - i - 1].x) > MATCHTOL)
		|| (abs(newpt.y - pnt[i].y) > MATCHTOL
		    && abs(newpt.y - pnt[npnts - i - 1].y) > MATCHTOL))
	    {
		/*fprintf(stderr,"matches(): posn mismatch: %d vs. %d",
			newpt.x, pnt[i].x);/*CMP*/
		/*fprintf(stderr,"   or  %d vs. %d\n",
			newpt.y, pnt[i].y);/*CMP*/
		rtn = FALSE;
		break;
	    }
	}
    }
    return rtn;
}
#undef MATCHTOL

/************************************************************************
*									*
* Check the list of selected ROIs to see if we're selected.
*									*/
Flag
Roitool::is_selected()
{
    RoitoolIterator element(selected_ROIs);
    Roitool *tool;
    while (tool = ++element){
	if (this == tool){
	    return TRUE;
	}
    }
    return FALSE;
}

/************************************************************************
*									*
* Draw an ROI marker at the specified position, (x, y).
*									*/
void
Roitool::draw_mark(int x, int y)
{
    Gpoint mpnt[4];
    // Note that g_fill_polygon() works asymetrically, which is why
    //  we need to subtract one less than the "MARK_SIZE", but add the
    //  whole "MAKK_SIZE".
    mpnt[0].x = x - MARK_SIZE + 1;
    mpnt[0].y = y - MARK_SIZE + 1;
    mpnt[1].x = x + MARK_SIZE;
    mpnt[1].y = y - MARK_SIZE + 1;
    mpnt[2].x = x + MARK_SIZE;
    mpnt[2].y = y + MARK_SIZE;
    mpnt[3].x = x - MARK_SIZE + 1;
    mpnt[3].y = y + MARK_SIZE;
    g_fill_polygon(gdev, mpnt, 4, color); 
}
