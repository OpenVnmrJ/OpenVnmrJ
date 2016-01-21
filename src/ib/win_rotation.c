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
 *  Window routines related to image rotation and reflection.
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
#include "ddllib.h"
#include "macroexec.h"
#include "win_rotation.h"

extern void win_print_msg(char *, ...);

// Names of bitmap files for image rotation buttons, and macro parms.
// The list must be terminated with a NULL entry.
RotTbl Win_rot::rot_tbl[] = {
    {"rot_90.bm",	"90",		ROT_90},
    {"rot_180.bm",	"180",		ROT_180},
    {"rot_270.bm",	"270",		ROT_270},
    {"rot_E-W.bm",	"flip",		ROT_FLIP_E_W},
    {"rot_E-W.bm",	"flip0",	ROT_FLIP_E_W},
    {"rot_N-S.bm",	"flip90",	ROT_FLIP_N_S},
    {"rot_NE-SW.bm",	"flip45",	ROT_FLIP_NE_SW},
    {"rot_NW-SE.bm",	"flip135",	ROT_FLIP_NW_SE},
    {NULL, NULL, ROT_90}
};

static Win_rot *winrot=NULL;

/************************************************************************
 *                                                                       *
 *  Show the control window.
 *									*/
void
winpro_rotation_show(void)
{
    // Create window object only if it is not created
    if (winrot == NULL){
	winrot = new Win_rot;
    }else{
	winrot->show_window();
    }
}

/************************************************************************
 *                                                                       *
 *  Get a server image for one of the button labels
 *									*/
Server_image
Win_rot::get_rot_image(Rottype rottype)
{
    int i;
    Server_image image;
    char initname[1024];
    char workbuf[1024];

    for (i=0; rot_tbl[i].file; i++){
	if (rot_tbl[i].rottype == rottype){
	    break;
	}
    }
    if (!rot_tbl[i].file){
	STDERR_1("get_rot_image(): internal error, rottype=%d", rottype);
	exit(1);
    }
    (void)init_get_env_name(initname);
    (void)sprintf(workbuf, "%s/%s", initname, rot_tbl[i].file);
    image = (Server_image)xv_create(NULL, SERVER_IMAGE,
				    SERVER_IMAGE_BITMAP_FILE, workbuf,
				    NULL);
    if (image == NULL){
	// Error
	STDERR_1("get_rot_image(): cannot create %s", workbuf);
	exit(1);
    }
    return image;
}

/************************************************************************
 *                                                                       *
 *  Creator of window.							*
 *									*/
Win_rot::Win_rot(void)
{
    Panel panel;		// panel
    int xitempos;		// current panel item position
    int yitempos;		// current panel item position
    Panel_item item;		// Panel item
    int xpos, ypos;		// window position
    char initname[1024];	// init file

    // Get the initialized file of window position
    (void)init_get_win_filename(initname);

    // Get the position of the control panel
    if (init_get_val(initname, "WINPRO_ROT", "dd", &xpos, &ypos) == NOT_OK)
    {
	xpos = 400;
	ypos = 40;
    }

    frame = xv_create(NULL, FRAME, NULL);

    popup = xv_create(frame, FRAME_CMD,
		      XV_X,		xpos,
		      XV_Y,		ypos,
		      FRAME_LABEL,	"Rotation",
		      FRAME_DONE_PROC,	&Win_rot::done_proc,
		      FRAME_CMD_PUSHPIN_IN,	TRUE,
		      NULL);
    
    panel = (Panel)xv_get(popup, FRAME_CMD_PANEL);

    xitempos = 5;
    yitempos = 5;
    item = xv_create(panel, PANEL_BUTTON,
		     XV_X, xitempos,
		     XV_Y, yitempos,
		     PANEL_LABEL_IMAGE, get_rot_image(ROT_90),
		     PANEL_CLIENT_DATA, ROT_90,
		     PANEL_NOTIFY_PROC, &Win_rot::execute,
		     NULL);
    xitempos += (int)xv_get(item, XV_WIDTH) + 5;
    item = xv_create(panel, PANEL_BUTTON,
		     XV_X, xitempos,
		     XV_Y, yitempos,
		     PANEL_LABEL_IMAGE, get_rot_image(ROT_180),
		     PANEL_CLIENT_DATA, ROT_180,
		     PANEL_NOTIFY_PROC, &Win_rot::execute,
		     NULL);
    xitempos += (int)xv_get(item, XV_WIDTH) + 5;
    item = xv_create(panel, PANEL_BUTTON,
		     XV_X, xitempos,
		     XV_Y, yitempos,
		     PANEL_LABEL_IMAGE, get_rot_image(ROT_270),
		     PANEL_CLIENT_DATA, ROT_270,
		     PANEL_NOTIFY_PROC, &Win_rot::execute,
		     NULL);

    xitempos = 5;
    yitempos += (int)xv_get(item, XV_HEIGHT) + 5;
    item = xv_create(panel, PANEL_BUTTON,
		     XV_X, xitempos,
		     XV_Y, yitempos,
		     PANEL_LABEL_IMAGE, get_rot_image(ROT_FLIP_E_W),
		     PANEL_CLIENT_DATA, ROT_FLIP_E_W,
		     PANEL_NOTIFY_PROC, &Win_rot::execute,
		     NULL);
    xitempos += (int)xv_get(item, XV_WIDTH) + 5;
    item = xv_create(panel, PANEL_BUTTON,
		     XV_X, xitempos,
		     XV_Y, yitempos,
		     PANEL_LABEL_IMAGE, get_rot_image(ROT_FLIP_N_S),
		     PANEL_CLIENT_DATA, ROT_FLIP_N_S,
		     PANEL_NOTIFY_PROC, &Win_rot::execute,
		     NULL);
    xitempos += (int)xv_get(item, XV_WIDTH) + 5;
    item = xv_create(panel, PANEL_BUTTON,
		     XV_X, xitempos,
		     XV_Y, yitempos,
		     PANEL_LABEL_IMAGE, get_rot_image(ROT_FLIP_NW_SE),
		     PANEL_CLIENT_DATA, ROT_FLIP_NW_SE,
		     PANEL_NOTIFY_PROC, &Win_rot::execute,
		     NULL);
    xitempos += (int)xv_get(item, XV_WIDTH) + 5;
    item = xv_create(panel, PANEL_BUTTON,
		     XV_X, xitempos,
		     XV_Y, yitempos,
		     PANEL_LABEL_IMAGE, get_rot_image(ROT_FLIP_NE_SW),
		     PANEL_CLIENT_DATA, ROT_FLIP_NE_SW,
		     PANEL_NOTIFY_PROC, &Win_rot::execute,
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
Win_rot::~Win_rot(void)
{
    xv_destroy_safe(frame);
}

/************************************************************************
 *                                                                       *
 *  Dismiss the popup window.						*
 *  (STATIC)								*
 *									*/
void
Win_rot::done_proc(Frame subframe)
{
    xv_set(subframe, XV_SHOW, FALSE, NULL);
    delete winrot;
    winrot = NULL;
    win_print_msg("Image rotation: Exit");
}

/************************************************************************
*									*
*  Set the "saturation" display choice
*  [MACRO interface]
*  argv[0]: (char *) 90 | 180 | 270 | flip | flip0 | flip45 | flip90 | flip135
*  [STATIC Function]							*
*									*/
int
Win_rot::Rotate(int argc, char **argv, int, char **)
{
    argc--; argv++;

    char *cmd;
    int i;

    if (argc != 1){
	ABORT;
    }
    cmd = argv[0];
    for (i=0; rot_tbl[i].cmd; i++){
	if (strcasecmp(rot_tbl[i].cmd, cmd) == 0){
	    break;
	}
    }
    if (!rot_tbl[i].cmd){
	ABORT;
    }
    rotate(rot_tbl[i].rottype);
    return PROC_COMPLETE;
}

/************************************************************************
 *                                                                       *
 *  Process image rotation/reflection command.
 *  Performs operation on images in all selected frames.
 *  LIMITATIONS:
 *  Does not handle overlay images.
 *  Only deals with "2dfov" data.
 *  (STATIC)								*
 *									*/
void
Win_rot::rotate(Rottype rottype)
{
    int df;			// Output pointer increment in fast dimension
    int ds;			// Output pointer increment in slow dimension
    float *dst;
    float *end;
    float *eol;
    int i;
    float *inbuf;
    int nx, ny;
    float *outbuf;
    int size;			// Number of pixels in image
    float *src;

    // For all selected frames
    Gframe *gframe;
    int iframe;
    Imginfo *img;
    for (iframe=1, gframe=Frame_select::get_selected_frame(iframe);
	 gframe;
	 iframe++, gframe=Frame_select::get_selected_frame(iframe))
    {
	// If frame contains an image, rotate (or flip) it.
	if (img = gframe->imginfo){
	    nx = img->GetFast();
	    ny = img->GetMedium();
	    size = nx * ny;
	    src = inbuf = (float *)img->GetData(); // inbuf points to orig data
	    outbuf = new float[size];	     // outbuf points to new buffer

	    // Set up pointer/increments for selected operation
	    switch (rottype){
	      case ROT_90:
		dst = outbuf + size - ny;
		df = -ny;
		ds = size - ny + 1;
		img->rot90_header();
		break;
	      case ROT_180:
		dst = outbuf + size - 1;
		df = -1;
		ds = -1;
		img->rot90_header();
		img->rot90_header();
		break;
	      case ROT_270:
		dst = outbuf + ny - 1;
		df = ny;
		ds = -size + ny - 1;
		img->rot90_header();
		img->rot90_header();
		img->rot90_header();
		break;
	      case ROT_FLIP_E_W:
		dst = outbuf + nx - 1;
		df = -1;
		ds = 2 * nx - 1;
		img->flip_header();
		break;
	      case ROT_FLIP_N_S:
		dst = outbuf + size - nx;
		df = 1;
		ds = -2 * nx + 1;
		img->flip_header();
		img->rot90_header();
		img->rot90_header();
		break;
	      case ROT_FLIP_NW_SE:
		dst = outbuf + size - 1;
		df = -ny;
		ds = size - ny - 1;
		img->rot90_header();
		img->flip_header();
		break;
	      case ROT_FLIP_NE_SW:
		dst = outbuf;
		df = ny;
		ds = -size + ny + 1;
		img->flip_header();
		img->rot90_header();
		break;
	      default:
		fprintf(stderr,"Win_rot::execute(): Illegal operation: %d\n",
			rottype);
		dst = outbuf;
		ds = df = 1;	// No change to picture
		break;
	    }

	    // Copy data to output buffer in new order
	    end = inbuf + size;
	    while (src < end){
		eol = src + nx - 1; // Points AT last pixel in line
		while (src < eol){
		    *dst = *src++;
		    dst += df;
		}
		*dst = *src++;	// Copy last point in line w/ diff dst incr.
		if (src < end){	// (Avoid debugger complaints.)
		    dst += ds;
		}
	    }
	    // Make output buffer the new data
	    img->st->SetData((char *)outbuf, size * sizeof(float));
	    delete[] outbuf;		// Because data is COPIED in

	    img->display_ood = img->pixmap_ood = TRUE;
	}
    }

    // Redraw the images
    Frame_data::redraw_ood_images();

    // Record the macro command to do this
    for (i=0; rot_tbl[i].cmd; i++){
	if (rot_tbl[i].rottype == rottype){
	    break;
	}
    }
    if (!rot_tbl[i].cmd){
	STDERR_1("rotate(): internal error, rottype=%d", rottype);
	exit(1);
    }
    macroexec->record("rotate('%s')\n", rot_tbl[i].cmd);
}

/************************************************************************
 *                                                                       *
 *  Process image rotation/reflection command from panel button.
 *  (STATIC)								*
 *									*/
void
Win_rot::execute(Panel_item item)
{
    Rottype rottype = (Rottype)xv_get(item, PANEL_CLIENT_DATA);
    rotate(rottype);
}
