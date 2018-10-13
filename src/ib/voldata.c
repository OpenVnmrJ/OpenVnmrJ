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
 *  Description								*
 *  -----------								*
 *									*
 *  Window routines for slice extraction from 3D data
 *									*
 *************************************************************************/
#include "stderr.h"
#include <stdio.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <math.h>
#include <memory.h>
#include "msgprt.h"
#include "common.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "ibcursors.h"
#include "zoom.h"
#include "initstart.h"
#include "convert.h"
// #include "process.h"
#include "ddllib.h"
#include "macroexec.h"
#include "voldata.h"
#include "interrupt.h"
#include "confirmwin.h"

extern Gframe *framehead;
extern Gframe *targetframe;
extern Frame_select *selecthead;

/* These get set for real in VolData::VolData() */
int VolData::front_plane = 0;
int VolData::top_plane = 0;
int VolData::side_plane = 0;

static VolData *vdat = NULL;

/************************************************************************
 *                                                                       *
 *  Show the slice extraction window.
 *
 *  STATIC
 *									*/
void
VolData::extract_slices(Imginfo *imginfo)
{
    // Create window object only if it is not created
    if (vdat == NULL){
	vdat = new VolData;
    }
    vdat->show_window();

    int newItem = TRUE;
    if (!imginfo){
	imginfo = vdat->vimage;
	newItem = FALSE;
    }
    vdat->set_data(imginfo, newItem);
}

/************************************************************************
 *                                                                       *
 *  Creator of window.							*
 *									*/
VolData::VolData()
{
    Panel panel;		// panel
    int xitempos;		// current panel item position
    int yitempos;		// current panel item position
    Panel_item item;		// Panel item
    int xpos, ypos;		// window position
    char initname[1024];	// init file

    gframe = NULL;
    vimage = NULL;

    // Get the initialized file of window position
    (void)init_get_win_filename(initname);

    // Get the position of the control panel
    if (init_get_val(initname, "WINPRO_SLICER", "dd", &xpos, &ypos) == NOT_OK){
	xpos = 400;
	ypos = 60;
    }

    frame = xv_create(NULL, FRAME, NULL);
    popup = xv_create(frame, FRAME_CMD,
		      XV_X,		xpos,
		      XV_Y,		ypos,
		      FRAME_LABEL,	"Slice Extraction",
		      FRAME_CMD_PUSHPIN_IN,	TRUE,
		      NULL);
    
    panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);
    xv_set(panel,
	   PANEL_LAYOUT, PANEL_VERTICAL,
	   PANEL_ITEM_X_GAP, 20,
	   NULL);

    orient = xv_create(panel, PANEL_CHECK_BOX,
		     PANEL_LAYOUT, PANEL_VERTICAL,
		     PANEL_LABEL_STRING, "Orientation",
		     PANEL_CHOICE_STRINGS,
		     "xy (front)", "xz (top)", "yz (side)", NULL,
		     PANEL_VALUE, 1,
		     NULL);
    /* NB: THESE NUMBERS MUST MATCH BUTTON ORDER. */
    front_plane = 0;
    top_plane = 1;
    side_plane = 2;

    item = extract_button = xv_create(panel, PANEL_BUTTON,
				      PANEL_LABEL_STRING, "Extract Slices",
				      PANEL_NOTIFY_PROC, &VolData::execute,
				      PANEL_INACTIVE, TRUE,
				      NULL);

    xitempos = (int)xv_get(item, XV_X);
    yitempos = (int)xv_get(item, XV_Y);

    item = xv_create(panel, PANEL_MESSAGE,
		     PANEL_NEXT_COL, -1,
		     PANEL_LABEL_STRING, "First",
		     PANEL_LABEL_BOLD, TRUE,
		     NULL);

    front.first = xv_create(panel, PANEL_NUMERIC_TEXT,
		     PANEL_VALUE_DISPLAY_LENGTH, 4,
		     PANEL_MIN_VALUE, 0,
		     PANEL_MAX_VALUE, 0,
		     PANEL_NOTIFY_PROC, VolData::slice_callback,
		     NULL);

    top.first = xv_create(panel, PANEL_NUMERIC_TEXT,
		     PANEL_VALUE_DISPLAY_LENGTH, 4,
		     PANEL_MIN_VALUE, 0,
		     PANEL_MAX_VALUE, 0,
		     PANEL_NOTIFY_PROC, VolData::slice_callback,
		     NULL);

    side.first = xv_create(panel, PANEL_NUMERIC_TEXT,
		     PANEL_VALUE_DISPLAY_LENGTH, 4,
		     PANEL_MIN_VALUE, 0,
		     PANEL_MAX_VALUE, 0,
		     PANEL_NOTIFY_PROC, VolData::slice_callback,
		     NULL);

    item = xv_create(panel, PANEL_MESSAGE,
		     PANEL_NEXT_COL, -1,
		     PANEL_LABEL_STRING, "Last",
		     PANEL_LABEL_BOLD, TRUE,
		     NULL);

    front.last = xv_create(panel, PANEL_NUMERIC_TEXT,
		     PANEL_VALUE_DISPLAY_LENGTH, 4,
		     PANEL_MIN_VALUE, 0,
		     PANEL_MAX_VALUE, 0,
		     PANEL_NOTIFY_PROC, VolData::slice_callback,
		     NULL);

    top.last = xv_create(panel, PANEL_NUMERIC_TEXT,
		     PANEL_VALUE_DISPLAY_LENGTH, 4,
		     PANEL_MIN_VALUE, 0,
		     PANEL_MAX_VALUE, 0,
		     PANEL_NOTIFY_PROC, VolData::slice_callback,
		     NULL);

    side.last = xv_create(panel, PANEL_NUMERIC_TEXT,
		     PANEL_VALUE_DISPLAY_LENGTH, 4,
		     PANEL_MIN_VALUE, 0,
		     PANEL_MAX_VALUE, 0,
		     PANEL_NOTIFY_PROC, VolData::slice_callback,
		     NULL);

    item = xv_create(panel, PANEL_MESSAGE,
		     PANEL_NEXT_COL, -1,
		     PANEL_LABEL_STRING, "Incr",
		     PANEL_LABEL_BOLD, TRUE,
		     NULL);

    front.inc = xv_create(panel, PANEL_NUMERIC_TEXT,
		     PANEL_VALUE_DISPLAY_LENGTH, 4,
		     PANEL_MIN_VALUE, 1,
		     PANEL_MAX_VALUE, 1,
		     NULL);

    top.inc = xv_create(panel, PANEL_NUMERIC_TEXT,
		     PANEL_VALUE_DISPLAY_LENGTH, 4,
		     PANEL_MIN_VALUE, 1,
		     PANEL_MAX_VALUE, 1,
		     NULL);

    side.inc = xv_create(panel, PANEL_NUMERIC_TEXT,
		     PANEL_VALUE_DISPLAY_LENGTH, 4,
		     PANEL_MIN_VALUE, 1,
		     PANEL_MAX_VALUE, 1,
		     NULL);

    mip_button = xv_create(panel, PANEL_BUTTON,
			   XV_X, (xitempos +
				  (int)xv_get(extract_button, XV_WIDTH) + 5),
			   XV_Y, yitempos,
			   PANEL_LABEL_STRING, "Extract MIPs",
			   PANEL_NOTIFY_PROC, &VolData::execute,
			   PANEL_INACTIVE, TRUE,
			   NULL);

    disp_button = xv_create(panel, PANEL_BUTTON,
			    XV_X, ((int)xv_get(mip_button, XV_X)
				   + (int)xv_get(mip_button, XV_WIDTH)
				   + 5),
			    XV_Y, yitempos,
			    PANEL_LABEL_STRING, "Put 3D",
			    PANEL_NOTIFY_PROC, &VolData::enframe,
			    NULL);


    del_button = xv_create(panel, PANEL_BUTTON,
			   XV_X, ((int)xv_get(disp_button, XV_X)
				  + (int)xv_get(disp_button, XV_WIDTH)
				  + 5),
			   XV_Y, yitempos,
			   PANEL_LABEL_STRING, "Unload ...",
			   PANEL_NOTIFY_PROC, &VolData::unload,
			   NULL);

    yitempos += (int)xv_get(extract_button, XV_HEIGHT) + 5;

    file_menu = (Menu)xv_create(NULL, MENU,
				MENU_NOTIFY_PROC, &VolData::fmenu_proc,
				NULL);

    fchoice = xv_create(panel, PANEL_ABBREV_MENU_BUTTON,
			XV_X, xitempos,
			XV_Y, yitempos,
			PANEL_LABEL_STRING, "File",
			PANEL_ITEM_MENU, file_menu,
			NULL);

    pfname = xv_create(panel, PANEL_MESSAGE,
		       XV_X, (xitempos +
			      (int)xv_get(fchoice, XV_WIDTH) + 5),
		       XV_Y, yitempos,
		       PANEL_LABEL_STRING, "    *** NO DATA ***",
		       /*PANEL_LABEL_BOLD, TRUE,*/
		       NULL);

    window_fit(panel);
    window_fit(popup);
    window_fit(frame);
}

/************************************************************************
 *                                                                      *
 *  Destructor of window.						*
 *									*/
VolData::~VolData(void)
{
    xv_destroy_safe(frame);
}

/************************************************************************
 *                                                                      *
 *  Called when a slice number selection changes
 *
 *  STATIC
 *									*/
Panel_setting
VolData::slice_callback(Panel_item item, int, Event *)
{
    int value = (int)xv_get(item, PANEL_VALUE);
    if (item == vdat->front.first){
	if ((int)xv_get(vdat->front.last, PANEL_VALUE) < value){
	    xv_set(vdat->front.last, PANEL_VALUE, value, NULL);
	}
    }else if (item == vdat->front.last){
	if ((int)xv_get(vdat->front.first, PANEL_VALUE) > value){
	    xv_set(vdat->front.first, PANEL_VALUE, value, NULL);
	}
    }else if (item == vdat->top.first){
	if ((int)xv_get(vdat->top.last, PANEL_VALUE) < value){
	    xv_set(vdat->top.last, PANEL_VALUE, value, NULL);
	}
    }else if (item == vdat->top.last){
	if ((int)xv_get(vdat->top.first, PANEL_VALUE) > value){
	    xv_set(vdat->top.first, PANEL_VALUE, value, NULL);
	}
    }else if (item == vdat->side.first){
	if ((int)xv_get(vdat->side.last, PANEL_VALUE) < value){
	    xv_set(vdat->side.last, PANEL_VALUE, value, NULL);
	}
    }else if (item == vdat->side.last){
	if ((int)xv_get(vdat->side.first, PANEL_VALUE) > value){
	    xv_set(vdat->side.first, PANEL_VALUE, value, NULL);
	}
    }
    return PANEL_INSERT;
}

/************************************************************************
 *                                                                      *
 *  Save volume image data.						*
 *									*/
void
VolData::set_data(Imginfo *imginfo, int newItem)
{
    Imginfo *thisimage;
    char *fname = 0;

    detach_imginfo(vimage);
    if (!imginfo){
	xv_set(extract_button, PANEL_INACTIVE, TRUE, NULL);
	xv_set(mip_button, PANEL_INACTIVE, TRUE, NULL);
	xv_set(pfname, PANEL_LABEL_STRING, "No Data", NULL);
    }else{
	attach_imginfo(vimage, imginfo);
	nfast = vimage->GetFast();
	nmedium = vimage->GetMedium();
	nslow = vimage->GetSlow();
	xv_set(front.first, PANEL_MAX_VALUE, nslow-1, NULL);
	xv_set(front.inc, PANEL_MAX_VALUE, nslow-1, NULL);
	xv_set(front.last, PANEL_MAX_VALUE, nslow-1, NULL);
	xv_set(top.first, PANEL_MAX_VALUE, nmedium-1, NULL);
	xv_set(top.inc, PANEL_MAX_VALUE, nmedium-1, NULL);
	xv_set(top.last, PANEL_MAX_VALUE, nmedium-1, NULL);
	xv_set(side.first, PANEL_MAX_VALUE, nfast-1, NULL);
	xv_set(side.inc, PANEL_MAX_VALUE, nfast-1, NULL);
	xv_set(side.last, PANEL_MAX_VALUE, nfast-1, NULL);
	xv_set(extract_button, PANEL_INACTIVE, FALSE, NULL);
	xv_set(mip_button, PANEL_INACTIVE, FALSE, NULL);
	vimage->st->GetValue("filename", fname);
	char *shortname;
	if (fname){
	    shortname = strdup(com_clip_len_front(fname, 43));
	    xv_set(pfname, PANEL_LABEL_STRING, shortname, NULL);
	}
	// The class variable vimage is temporarily holding this image,
	// but if we load another 3D data set, the Imginfo will be detached
	// from vimage and the memory freed.  To keep it around, we
	// attach it to "thisimage", which is effectively stored in the
	// menu widget.
	attach_imginfo(thisimage, vimage);
	if (newItem){
	    Menu_item mi = (Menu_item)xv_create(NULL, MENUITEM,
						MENU_STRING, shortname,
						MENU_CLIENT_DATA, thisimage,
						NULL);
	    xv_set(file_menu, MENU_APPEND_ITEM, mi, NULL);
	}
    }
}

/************************************************************************
*									*
*  Extract image slices from a 3D data set.
*  [MACRO interface]
*  argv[0]: See "usage" string, below.
*  argv[1]: ...
*  [STATIC Function]							*
*									*/
int
VolData::Extract(int argc, char **argv, int, char **)
{
    char *name = *argv;

    argc--; argv++;
    if (!vdat || !vdat->vimage){
	// No volume data loaded yet
	msgerr_print("%s: No 3D data set available", name);
	ABORT;
    }

    char usage[100];
    sprintf(usage,
	    "usage: %s(['xy'|'yz'|'xz'], first_slice [, last_slice [, incr]])",
	    name);
    if (!argc){
	msgerr_print(usage);
	ABORT;
    }
    int orient = front_plane;
    if (strcasecmp(*argv, "xy") == 0){
	orient = front_plane;
	argc--; argv++;
    }else if(strcasecmp(*argv, "xz") == 0){
	orient = top_plane;
	argc--; argv++;
    }else if(strcasecmp(*argv, "yz") == 0){
	orient = side_plane;
	argc--; argv++;
    }

    if ((argc < 1) || (argc > 3)){
	msgerr_print(usage);
	ABORT;
    }
    int slices[3];
    int ni = MacroExec::getIntArgs(argc, argv, slices, argc);
    if (ni != argc){
	msgerr_print(usage);
	ABORT;
    }
    int first = slices[0];
    int last = first;
    int incr = 1;
    if (ni >= 2){
	last = slices[1];
    }
    if (ni == 3){
	incr = slices[2];
    }
    vdat->extract_planes(orient, first, last, incr);
    
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Extract Maximum Intensity Projections from a 3D data set.
*  [MACRO interface]
*  argv[0]: See "usage" string, below.
*  argv[1]: ...
*  [STATIC Function]							*
*									*/
int
VolData::Mip(int argc, char **argv, int, char **)
{
    int i;
    int j;
    char *name = *argv;

    argc--; argv++;
    if (!vdat || !vdat->vimage){
	// No volume data loaded yet
	msgerr_print("%s: No 3D data set available", name);
	ABORT;
    }

    char usage[100];
    sprintf(usage,
	    "usage: %s(['xy'|'yz'|'xz'], first_slice [, last_slice [, incr]])",
	    name);
    if (!argc){
	msgerr_print(usage);
	ABORT;
    }

    int orient = front_plane;
    if (strcasecmp(*argv, "xy") == 0){
	orient = front_plane;
	argc--; argv++;
    }else if(strcasecmp(*argv, "xz") == 0){
	orient = top_plane;
	argc--; argv++;
    }else if(strcasecmp(*argv, "yz") == 0){
	orient = side_plane;
	argc--; argv++;
    }

    if ((argc < 1) || (argc > 3)){
	msgerr_print(usage);
	ABORT;
    }
    int slices[3];
    int ni = MacroExec::getIntArgs(argc, argv, slices, argc);
    if (ni != argc){
	msgerr_print(usage);
	ABORT;
    }
    int first = slices[0];
    int last = first;
    int incr = 1;
    if (ni >= 2){
	last = slices[1];
    }
    if (ni == 3){
	incr = slices[2];
    }
    vdat->extract_mip(orient, first, last, incr);
    
    return PROC_COMPLETE;
}

/************************************************************************
 *                                                                      *
 *  Process file selection command
 *  ( STATIC )
 *									*/
void
VolData::fmenu_proc(Menu menu, Menu_item menu_item)
{
    vdat->set_data((Imginfo *)xv_get(menu_item, MENU_CLIENT_DATA), FALSE);
}


/************************************************************************
 *                                                                      *
 *  Process 3D file display command
 *  ( STATIC )
 *									*/
void
VolData::enframe(Panel_item item, Event *)
{
    if (!vdat || !vdat->vimage){
	return;
    }
    Imginfo *img = vdat->vimage;
    Gframe *frame = Frame_select::get_selected_frame(1);
    if (!frame){
	return;
    }
    detach_imginfo(frame->imginfo);
    attach_imginfo(frame->imginfo, img);

    // Use gray-scale colormap
    img->cmsindex = SISCMS_2;

    Frame_data::display_data(frame, 0, 0,
			     img->GetFast(),
			     img->GetMedium(),
			     img->vs);
}

/************************************************************************
 *                                                                      *
 *  Process slice extraction command
 *  ( STATIC )
 *									*/
void
VolData::unload(Panel_item item, Event *)
{
    Menu_item mi;
    int nitems;
    Imginfo *img;
    int i;
    char *fname;
    char msg[1024];

    if (!vdat || !vdat->vimage){
	return;
    }
    vdat->vimage->st->GetValue("filename", fname);
    sprintf(msg,"Unload the 3D data set\n\"%s\"?",
	    com_clip_len_front(fname, 50));
    if (! confirmwin_popup("Yes", "Cancel", msg, 0)){
	return;
    }
    nitems = (int)xv_get(vdat->file_menu, MENU_NITEMS);
    for (i=1; i<=nitems; i++){
	mi = (Menu_item)xv_get(vdat->file_menu, MENU_NTH_ITEM, i);
	img = (Imginfo *)xv_get(mi, MENU_CLIENT_DATA);
	if (img == vdat->vimage){
	    break;
	}
    }
    if (i > nitems){
	fprintf(stderr,"unload(): INTERNAL ERROR: Item not found\n");
    }else{
	xv_set(vdat->file_menu, MENU_REMOVE, i, NULL);
	detach_imginfo(img);
	// Set new imginfo (or None)
	nitems--;
	if (nitems){
	    if (i > nitems) {i = nitems;}
	    mi = (Menu_item)xv_get(vdat->file_menu, MENU_NTH_ITEM, i);
	    img = (Imginfo *)xv_get(mi, MENU_CLIENT_DATA);
	}else{
	    img = NULL;
	}
	vdat->set_data(img, FALSE);
    }
}

/************************************************************************
 *                                                                      *
 *  Process slice extraction command
 *  ( STATIC )
 *									*/
void
VolData::execute(Panel_item item, Event *)
{
    int i;
    int j;
    int k;
    int first;
    int inc;
    int last;
    int planelist[] = {front_plane, top_plane, side_plane};
    Panel_item firstlist[] = {vdat->front.first,
			      vdat->top.first,
			      vdat->side.first};
    Panel_item inclist[] = {vdat->front.inc,
			    vdat->top.inc,
			    vdat->side.inc};
    Panel_item lastlist[] = {vdat->front.last,
			     vdat->top.last,
			     vdat->side.last};

    int nslices;
    int norient = (int)xv_get(vdat->orient, PANEL_VALUE);
    for (k=0; k<3; k++){
	if (norient & (1 << planelist[k])){
	    first = num_text_value(firstlist[k]);
	    inc = num_text_value(inclist[k]);
	    last = num_text_value(lastlist[k]);
	    if (item == vdat->mip_button){
		// Do a MIP extraction
		vdat->extract_mip(planelist[k], first, last, inc);
	    }else{
		// Extract individual slices
		vdat->extract_planes(planelist[k], first, last, inc);
	    }
	}
    }
}

/************************************************************************
 *                                                                       *
 * Extract slices in given (orthogonal) plane.
 */
void
VolData::extract_planes(int orientation, int first, int last, int incr)
{
    int i;
    char macrocmd[100];
    int planelist[] = {front_plane, top_plane, side_plane};
    char *planenames[] = {"xy", "xz", "yz"};

    first = vdat->clip_slice(orientation, first);
    last = vdat->clip_slice(orientation, last);
    interrupt_begin();
    if (Gframe::numFrames() == 0){
	Gframe *gf = Gframe::big_gframe();
	gf->select_frame();
    }
    int nslices = 1 + (last - first) / incr;
    if (nslices > 1 && Gframe::numFrames() == 1){
	// Split up our one frame to get enough to load all slices into.
	int nsplit = (int)sqrt((double)nslices);
	if (nslices > nsplit * nsplit){
	    nsplit++;
	}
	Gframe *gf = Gframe::get_frame_by_number(1);
	gf->select_frame();
	gf->split(nsplit, nsplit);
    }
    for (i=first; i<=last && !interrupt(); i+=incr){
	vdat->extract_plane(planelist[orientation], 1, &i);
    }
    interrupt_end();
    sprintf(macrocmd,"vol_extract('%s', %d, %d, %d)\n",
	    planenames[orientation], first, last, incr);
    macroexec->record(macrocmd);
}

/************************************************************************
 *                                                                       *
 * Extract MIP in an orthogonal plane.
 */
void
VolData::extract_mip(int orientation, int first, int last, int incr)
{
    int i;
    int j;
    char macrocmd[100];
    char *planenames[] = {"xy", "xz", "yz"};

    first = vdat->clip_slice(orientation, first);
    last = vdat->clip_slice(orientation, last);
    int nslices = (last - first + incr) / incr;
    nslices = nslices < 1 ? 1 : nslices;
    int *slicelist = new int[nslices];
    for (i=first, j=0; i<=last; i+=incr, j++){
	slicelist[j] = i;
    }
    vdat->extract_plane(orientation, nslices, slicelist);
    delete[] slicelist;

    sprintf(macrocmd,"vol_mip('%s', %d, %d, %d)\n",
	    planenames[orientation], first, last, incr);
    macroexec->record(macrocmd);
}

/************************************************************************
 *                                                                       *
 *  Extract a slice of data or the MIP of a list of slices.
 *  A simple extraction is just a MIP over one slice.
 *  Set "orientation" to front_plane, top_plane, or side_plane.
 *  Set "slicelist" to the array of slice indices to do and "nslices"
 *  to the number of slices in the array.
 *									*/
void
VolData::extract_plane(int orientation, int nslices, int *slicelist)
{
    int i;
    int j;
    int k;
    int k0;
    int k1;
    int nx;
    int ny;
    float *data;
    int slice;

    if (!vimage){
	return;
    }
    // Make sure we are loading into a valid frame
    if ( targetframe == NULL || !framehead->exists(targetframe) ){
	targetframe = framehead;
	Frame_routine::FindNextFreeFrame();
    }
    gframe = targetframe;
    if (gframe == NULL){
	msgerr_print("load_data: No Frames to load data into.");
	return;
    }
    selecthead->deselect();
    gframe->mark();
    selecthead->insert(gframe);
    
    int cursor = set_cursor_shape(IBCURS_BUSY);
    if (nslices > 1){
	interrupt_begin();
    }

    slice = *slicelist;		// Prepare to get first slice
    if (orientation == front_plane){
	float *buf = new float[nfast * nmedium];
	// Attach a new Imginfo to the gframe
	detach_imginfo(gframe->imginfo);
	gframe->imginfo = new Imginfo();
	gframe->imginfo->st = (DDLSymbolTable *)vimage->st->CloneList(FALSE);
	gframe->imginfo->InitializeSymTab(RANK_2D,
					  BIT_32, TYPE_FLOAT,
					  nfast, nmedium, 1, 1,
					  0);
	// Extract the first slice
	data = (float *)vimage->GetData() + slice * nfast * nmedium;
	for (i=0; i<nfast * nmedium; i++){
	    buf[i] = data[i];
	}
	// Do the MIP of the data
	for (k=1; k<nslices && !interrupt(); k++){
	    slice = slicelist[k];
	    data = (float *)vimage->GetData() + slice * nfast * nmedium;
	    for (i=0; i<nfast * nmedium; i++){
		if (buf[i] < data[i]){
		    buf[i] = data[i];
		}
	    }
	}
	gframe->imginfo->st->SetData((char *)buf,
				     sizeof(float) * nfast * nmedium);
	nx = nfast;
	ny = nmedium;
	delete[] buf;
    }else if (orientation == top_plane){
	float *buf = new float[nfast * nslow];
	// Attach a new Imginfo to the gframe
	detach_imginfo(gframe->imginfo);
	gframe->imginfo = new Imginfo();
	gframe->imginfo->st = (DDLSymbolTable *)vimage->st->CloneList(FALSE);
	gframe->imginfo->InitializeSymTab(RANK_2D,
					  BIT_32, TYPE_FLOAT,
					  nfast, nslow, 1, 1,
					  0);
	// Extract the first slice
	data = (float *)vimage->GetData() + slice * nfast;
	for (j=0; j<nslow; j++){
	    k0 = j * nfast * nmedium;
	    k1 = j * nfast;
	    for (i=0; i<nfast; i++){
		buf[i + k1] = data[i + k0];
	    }
	}
	// Do the MIP of the data
	for (k=1; k<nslices && !interrupt(); k++){
	    slice = slicelist[k];
	    data = (float *)vimage->GetData() + slice * nfast;
	    for (j=0; j<nslow; j++){
		k0 = j * nfast * nmedium;
		k1 = j * nfast;
		for (i=0; i<nfast; i++){
		    if (buf[i + k1] < data[i + k0]){
			buf[i + k1] = data[i + k0];
		    }
		}
	    }
	}
	gframe->imginfo->st->SetData((char *)buf,
				     sizeof(float) * nfast * nslow);
	nx = nfast;
	ny = nslow;
	delete[] buf;
    }else if (orientation == side_plane){
	float *buf = new float[nmedium * nslow];
	// Attach a new Imginfo to the gframe
	detach_imginfo(gframe->imginfo);
	gframe->imginfo = new Imginfo();
	gframe->imginfo->st = (DDLSymbolTable *)vimage->st->CloneList(FALSE);
	gframe->imginfo->InitializeSymTab(RANK_2D,
					  BIT_32, TYPE_FLOAT,
					  nmedium, nslow, 1, 1,
					  0);
	// Extract the first slice
	data = (float *)vimage->GetData() + slice;
	for (j=0; j<nslow; j++){
	    k0 = j * nfast * nmedium;
	    k1 = j * nmedium;
	    for (i=0; i<nmedium; i++){
		buf[i + k1] = data[i*nfast + k0];
	    }
	}
	// Do the MIP of the data
	for (k=1; k<nslices && !interrupt(); k++){
	    slice = slicelist[k];
	    data = (float *)vimage->GetData() + slice;
	    for (j=0; j<nslow; j++){
		k0 = j * nfast * nmedium;
		k1 = j * nmedium;
		for (i=0; i<nmedium; i++){
		    if (buf[i + k1] < data[i*nfast + k0]){
			buf[i + k1] = data[i*nfast + k0];
		    }
		}
	    }
	}
	gframe->imginfo->st->SetData((char *)buf,
				     sizeof(float) * nmedium * nslow);
	nx = nmedium;
	ny = nslow;
	delete[] buf;
    }else{
	fprintf(stderr, "VolData::extract_plane(): Internal error %d\n",
		orientation);
	return;
    }
    extract_plane_header(gframe->imginfo, orientation, nslices, slicelist);
    gframe->imginfo->display_ood = gframe->imginfo->pixmap_ood = TRUE;
    Frame_data::display_data(gframe, 0, 0, nx, ny, gframe->imginfo->vs);

    Frame_routine::FindNextFreeFrame();
    (void)set_cursor_shape(cursor);
    if (nslices > 1){
	interrupt_end();
    }
}

void
VolData::extract_plane_header(Imginfo *img,
			      int slice_orient,
			      int nslices,
			      int *slicelist)
{
    int i;

    DDLSymbolTable *vst = vimage->st;
    DDLSymbolTable *ist = img->st;
    double slice_offset;
    int minslice = slicelist[0];
    int maxslice = slicelist[0];
    for (i=1; i<nslices; i++){
	if (minslice > slicelist[i]){
	    minslice = slicelist[i];
	}else if (maxslice < slicelist[i]){
	    maxslice = slicelist[i];
	}
    }
    float fslice = (maxslice + minslice) / 2;
    nslices = maxslice - minslice + 1; // NB: redefine nslices!

    // Get all relevant values from 3D data set
    int matrix[3];
    vst->GetValue("matrix", matrix[0], 0);
    vst->GetValue("matrix", matrix[1], 1);
    vst->GetValue("matrix", matrix[2], 2);
    double span[3];
    vst->GetValue("span", span[0], 0);
    vst->GetValue("span", span[1], 1);
    vst->GetValue("span", span[2], 2);
    double location[3];
    vst->GetValue("location", location[0], 0);
    vst->GetValue("location", location[1], 1);
    vst->GetValue("location", location[2], 2);
    double origin[3];
    vst->GetValue("origin", origin[0], 0);
    vst->GetValue("origin", origin[1], 1);
    vst->GetValue("origin", origin[2], 2);
    double roi[3];
    vst->GetValue("roi", roi[0], 0);
    vst->GetValue("roi", roi[1], 1);
    vst->GetValue("roi", roi[2], 2);
    double orientation[9];
    vst->GetValue("orientation", orientation[0], 0);
    vst->GetValue("orientation", orientation[1], 1);
    vst->GetValue("orientation", orientation[2], 2);
    vst->GetValue("orientation", orientation[3], 3);
    vst->GetValue("orientation", orientation[4], 4);
    vst->GetValue("orientation", orientation[5], 5);
    vst->GetValue("orientation", orientation[6], 6);
    vst->GetValue("orientation", orientation[7], 7);
    vst->GetValue("orientation", orientation[8], 8);

    ist->SetValue("subrank", "2dfov");
    if (slice_orient == front_plane){
	ist->SetValue("span", span[0], 0);
	ist->SetValue("span", span[1], 1);
	ist->SetValue("location", location[0], 0);
	ist->SetValue("location", location[1], 1);
	slice_offset = ((fslice - (matrix[2] - 1.0) / 2.0)
			* (roi[2] / matrix[2]));
	ist->SetValue("location", location[2] + slice_offset, 2);
	ist->SetValue("origin", origin[0], 0);
	ist->SetValue("origin", origin[1], 1);
	ist->SetValue("roi", roi[0], 0);
	ist->SetValue("roi", roi[1], 1);
	ist->SetValue("roi", (nslices * roi[2]) / matrix[2], 2);
	ist->SetValue("orientation", orientation[0], 0);
	ist->SetValue("orientation", orientation[1], 1);
	ist->SetValue("orientation", orientation[2], 2);	
	ist->SetValue("orientation", orientation[3], 3);
	ist->SetValue("orientation", orientation[4], 4);
	ist->SetValue("orientation", orientation[5], 5);	
	ist->SetValue("orientation", orientation[6], 6);
	ist->SetValue("orientation", orientation[7], 7);
	ist->SetValue("orientation", orientation[8], 8);	
    }else if (slice_orient == top_plane){
	ist->SetValue("span", span[0], 0);
	ist->SetValue("span", span[2], 1);
	ist->SetValue("location", location[0], 0);
	ist->SetValue("location", location[2], 1);
	slice_offset = ((fslice - (matrix[1] - 1.0) / 2.0)
			* (roi[1] / matrix[1]));
	ist->SetValue("location", location[1] + slice_offset, 2);
	ist->SetValue("origin", origin[0], 0);
	ist->SetValue("origin", origin[2], 1);
	ist->SetValue("roi", roi[0], 0);
	ist->SetValue("roi", roi[2], 1);
	ist->SetValue("roi", (nslices * roi[1] / matrix[1]), 2);	
	ist->SetValue("orientation", orientation[0], 0);
	ist->SetValue("orientation", orientation[1], 1);
	ist->SetValue("orientation", orientation[2], 2);	
	ist->SetValue("orientation", orientation[6], 3);
	ist->SetValue("orientation", orientation[7], 4);
	ist->SetValue("orientation", orientation[8], 5);	
	ist->SetValue("orientation", orientation[3], 6);
	ist->SetValue("orientation", orientation[4], 7);
	ist->SetValue("orientation", orientation[5], 8);	
    }else if (slice_orient == side_plane){
	ist->SetValue("span", span[1], 0);
	ist->SetValue("span", span[2], 1);
	ist->SetValue("location", location[1], 0);
	ist->SetValue("location", location[2], 1);
	slice_offset = ((fslice - (matrix[0] - 1.0) / 2.0)
			* (roi[0] / matrix[0]));
	ist->SetValue("location", location[0] + slice_offset, 2);
	ist->SetValue("origin", origin[1], 0);
	ist->SetValue("origin", origin[2], 1);
	ist->SetValue("roi", roi[1], 0);
	ist->SetValue("roi", roi[2], 1);
	ist->SetValue("roi", (nslices * roi[0] / matrix[0]), 2);	
	ist->SetValue("orientation", orientation[6], 0);
	ist->SetValue("orientation", orientation[7], 1);
	ist->SetValue("orientation", orientation[8], 2);	
	ist->SetValue("orientation", orientation[0], 3);
	ist->SetValue("orientation", orientation[1], 4);
	ist->SetValue("orientation", orientation[2], 5);	
	ist->SetValue("orientation", orientation[3], 6);
	ist->SetValue("orientation", orientation[4], 7);
	ist->SetValue("orientation", orientation[5], 8);	
    }else{
	fprintf(stderr, "VolData::extract_plane_header(): Internal error %d\n",
		slice_orient);
    }	
}

/************************************************************************
 *
 *  Numeric Text Items in XView are not guaranteed to return valid
 *  values.  So here is a convenience function.
 *  (STATIC)
 */
int
VolData::num_text_value(Panel_item item)
{
    int i = (int)xv_get(item, PANEL_VALUE);
    int max = (int)xv_get(item, PANEL_MAX_VALUE);
    int min = (int)xv_get(item, PANEL_MIN_VALUE);
    i = max < i ? max : min > i ? min : i;
    return i;
}

/************************************************************************
 *
 *  Make sure a slice number is valid
 */
int
VolData::clip_slice(int slice_orient, int slice)
{
    int max;

    if (slice_orient == front_plane){
	max = vimage->GetSlow() - 1;
    }else if (slice_orient == top_plane){
	max = vimage->GetMedium() - 1;
    }else if (slice_orient == side_plane){
	max = vimage->GetFast() - 1;
    }
    slice = slice < 0 ? 0 : slice > max ? max : slice ;
    return slice;
}
