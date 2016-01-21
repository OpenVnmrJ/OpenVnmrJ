/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

static char* Sid () {
    return "@(#)colorimginfo.c 18.1 03/21/08 (c)1993 SISCO";
}
#include <stdio.h>
#include <math.h>
// #include <stream.h>
#ifdef LINUX
// #include <strstream>
#else
// #include <strstream.h>
#endif
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "ddllib.h"
#include "initstart.h"
#include "stderr.h"
#include "msgprt.h"
#include "smemspace.h"
#include "ibcursors.h"
#include "gtools.h"
//#include "imginfo.h"
#include "colorimginfo.h"

extern void FlushGraphics();

 /*
  *  The following ColorImginfo constructor creates a new ColorImginfo
  *  that is a copy of another ColorImginfo
  *
  */

ColorImginfo::ColorImginfo(ColorImginfo *original) : Imginfo(original)
{
}

ColorImginfo::ColorImginfo(void) : Imginfo()
{
    st = new DDLSymbolTable();
}
  
ColorImginfo::~ColorImginfo(void){ }


/************************************************************************
*									*
*  Display the data associated with this image at the current location.
*/
void
ColorImginfo::display_data(Gframe *)
{
    Gdev *gdev = Gframe::gdev;

    if (pixmap &&
	(pixwd == pixmap_fmt.pixwd &&
	 pixht == pixmap_fmt.pixht &&
	 datastx == pixmap_fmt.datastx &&
	 datasty == pixmap_fmt.datasty &&
	 datawd == pixmap_fmt.datawd &&
	 dataht == pixmap_fmt.dataht &&
	 vs == pixmap_fmt.vs))
    {
	// We can just put up the old image again.
	/* store the current type of draw operation */
	int prev_op = G_Get_Op(gdev);
	
	/* set the type of draw operation */
	G_Set_Op(gdev, GXcopy);
	
	XCopyArea(gdev->xdpy, pixmap,
		  gdev->xid, gdev->xgc,
		  0, 0,
		  pixwd, pixht,
		  pixstx, pixsty);
	
	/* restore the original X draw operation */
	G_Set_Op(gdev, prev_op);
    }else{
	// We need to calculate a new image
	int cursor = set_cursor_shape(IBCURS_BUSY);
	if (pixmap){
	    // We have an obsolete pixmap--free it
	    XFreePixmap(display, pixmap);
	    pixmap = 0;
	}

	//
	// HERE IS THE GUTS OF THE MULTI-COLOR DISPLAY ROUTINE.
	// This version assumes 8-bit color, so we have to dither to
	// display all the colors we want.
	//
	// Get a buffer to put the dithered image in.
	float *cdata;
	if ( ! (cdata = (float *)memspace_float1D(pixwd*pixht))){
	    msgerr_print("ColorImginfo::display_data(): Can't get memory");
	    return;
	}

	// Construct the dithered image
	make_dithered_image(cdata, pixwd, pixht);

	// Put up the data.
	pixmap =
	g_display_image(gdev,
			cmsindex,
			cdata,
			pixwd, pixht,	// Size of data array
			0, 0,		// First data point to display
			pixwd, pixht,	// Number of data pts to display
			pixstx, pixsty,	// Upper-left corner on screen
			pixwd, pixht,	// Size on screen
			TOP,		// Orientation: 1st point at top
			1.0,		// Intensity scale factor
			FALSE);		// Don't Return a pixmap

	release_memspace(cdata);
	
	//
	// END OF SPECIAL STUFF FOR MULTI-COLOR DISPLAY.
	//
	
	// Remember which display has the pixmap and what its parms are.
	display = gdev->xdpy;
	pixmap_fmt.pixwd = pixwd;
	pixmap_fmt.pixht = pixht;
	pixmap_fmt.datastx = datastx;
	pixmap_fmt.datasty = datasty;
	pixmap_fmt.datawd = datawd;
	pixmap_fmt.dataht = dataht;
	pixmap_fmt.vs = vs;

	FlushGraphics();
	(void)set_cursor_shape(cursor);
    }

    // Now display the ROIs/labels
    RoitoolIterator element(display_list);
    Roitool *tool;
    Gmode mode;

    update_scale_factors();	// Init xscale, xoffset, etc.

    while (element.NotEmpty()){
	tool = ++element;
	mode = tool->setcopy();
	tool->draw();
	tool->setGmode(mode);
    }
    display_ood = FALSE;	// Mark display updated
}

/************************************************************************
*									*
*  Construct a color image by dithering.
*  Put it in the buffer "cdata", with dimensions "nx" by "ny".
*/
void
ColorImginfo::make_dithered_image(float *cdata, int nx, int ny)
{
    int i, j, k;
    const int maxcycle = 6;
    const int maxstr = 1024;

    //
    // Get colormap and intensity scaling info for each color
    //
    // Locations of different sections of colormap:
    char cms_name[maxstr+1];
    init_get_cmp_filename(cms_name);	// Init file name

    // Number of entries of each type
    int mark_levels, grey_levels, color_levels;
    if (init_get_val(cms_name, "mark-color", "d", &mark_levels) == NOT_OK){
	MSGERR("Unable to read mark-color.");
	mark_levels = 8;
    }
    if (init_get_val(cms_name, "gray-color", "d", &grey_levels) == NOT_OK){
	MSGERR("Unable to read gray-color.");
	grey_levels = 128;
    }
    if (init_get_val(cms_name, "false-color", "d", &color_levels) == NOT_OK){
	MSGERR("Unable to read false-color.");
	color_levels = 81;
    }

    // Set all the intensity scaling factors.
    char *color;
    double voffset[maxbands];	// Colormap_value = voffset + x * vscale
    double vscale[maxbands];	//
    int valmin[maxbands];	// Colormap value for 0 intensity
    int valmax[maxbands];	// Colormap value for maximum intensity
    int min_grey = 0;
    int min_red = grey_levels;
    int min_green = grey_levels + color_levels / 3;
    int min_blue = grey_levels + (2 * color_levels) / 3;
    for (i=0; i<nbands; i++){
	st->GetValue("enum_bands", color, i);
	if (strcmp(color, "grey") == 0){
	    valmin[i] = min_grey;
	    valmax[i] = grey_levels - 1;
	}else if (strcmp(color, "red") == 0){
	    valmin[i] = min_red;
	    valmax[i] = valmin[i] + color_levels / 3 - 1;
	}else if (strcmp(color, "green") == 0){
	    valmin[i] = min_green;
	    valmax[i] = valmin[i] + color_levels / 3 - 1;
	}else if (strcmp(color, "blue") == 0){
	    valmin[i] = min_blue;
	    valmax[i] = valmin[i] + color_levels / 3 - 1;
	}
	if (vmax[i] == vmin[i]){
	    vscale[i] = 0;
	}else{
	    vscale[i] = (valmax[i] - valmin[i]) / (vmax[i] - vmin[i]);
	}
	voffset[i] = valmin[i] - vmin[i] * vscale[i];
    }

    //
    static const int xpix_per_cycle[] = {1, 2, 3, 2};
    static const int ypix_per_cycle[] = {1, 1, 2, 2};
    static const int cycle_len[] = {1, 2, 6, 4};
    static const int ncycles[] = {1, 2, 3, 2};
    static const int row_incs[][maxbands][maxcycle] = {{{ 0, 0, 0, 0, 0, 0},
							{ 0, 0, 0, 0, 0, 0},
							{ 1,-1, 1,-1, 1,-1},
							{ 1,-1, 1,-1, 0, 0}},
						       {{ 0, 0, 0, 0, 0, 0},
							{ 0, 0, 0, 0, 0, 0},
							{-1, 0, 0, 1, 0, 0},
							{ 1,-1, 1,-1, 0, 0}},
						       {{ 0, 0, 0, 0, 0, 0},
							{ 0, 0, 0, 0, 0, 0},
							{ 0, 1, 0, 0,-1, 0},
							{ 0, 0, 0, 0, 0, 0}}};
    
    static const int col_incs[][maxbands][maxcycle] = {{{ 1, 0, 0, 0, 0, 0},
							{ 1, 1, 0, 0, 0, 0},
							{ 0, 1, 0, 1, 0, 1},
							{ 0, 1, 0, 1, 0, 0}},
						       {{ 1, 0, 0, 0, 0, 0},
							{-1, 3, 0, 0, 0, 0},
							{ 1,-1, 2, 0,-1, 2},
							{ 0,-1, 0, 3, 0, 0}},
						       {{ 0, 0, 0, 0, 0, 0},
							{ 0, 0, 0, 0, 0, 0},
							{-1, 0, 2,-1, 1, 2},
							{ 0, 0, 0, 0, 0, 0}}};
    
    static const int row_offsets[][maxbands] = {{0, 0, 0, 0},
						{0, 0, 1, 0},
						{0, 0, 0, 0}};
    static const int col_offsets[][maxbands] = {{0, 0, 0, 0},
						{0, 1, 0, 1},
						{0, 0, 1, 0}};
    
    int row_offset;
    int col_offset;
    double index;
    int iindex;
    int outdex;

    int outcol_inc = xpix_per_cycle[nbands-1];
    int outrow_inc = ypix_per_cycle[nbands-1];
    int cycles_per_row = (nx * outrow_inc) / cycle_len[nbands-1];
    int n_logical_rows = ny / outrow_inc;
    int data_per_cycle = cycle_len[nbands-1] / nbands;
    double index_inc = (double)(datawd * outcol_inc) / (nx * data_per_cycle);

    int inrow;
    int iband;
    int icycle;
    float *indata = (float *)GetData();
    int outrow;
    for (k=outrow=0; k < n_logical_rows; k++, outrow+=outrow_inc){
	// Interpolate to get input row number
	inrow = (int)(outrow * (double)dataht / ny);
	if (inrow >= dataht) inrow = dataht - 1;
	index = inrow * datawd;

	icycle = k % ncycles[nbands-1];
	col_offset = col_offsets[icycle][nbands-1];
	row_offset = row_offsets[icycle][nbands-1];
	outdex = outrow * nx + col_offset + row_offset * nx;
	for (i=0; i<cycles_per_row; i++){
	    for (j=iband=0; j<cycle_len[nbands-1]; j++){
		iindex = (int)index * nbands;
		cdata[outdex] = (voffset[iband] +
				 vscale[iband] * indata[iindex + iband]);
		if (cdata[outdex] < valmin[iband]){
		    cdata[outdex] = valmin[iband];
		}else if (cdata[outdex] > valmax[iband]){
		    cdata[outdex] = valmax[iband];
		}

		// Update the output pixel pointer
		outdex += (col_incs[icycle][nbands-1][j]
			   + nx * row_incs[icycle][nbands-1][j]);
		// Update the color
		if (++iband == nbands){
		    // Time to increment the input data index
		    index += index_inc;
		    iband = 0;
		}
	    }
	}
    }

    // Clear out the margins, in case the data doesn't exactly fit.
    j = nx - nx % outcol_inc;
    for (i=j; i<nx; i++){
	for (k=0; k<ny; k++){
	    cdata[k*nx + i] = min_red;
	}
    }
    j = ny - ny % outrow_inc;
    for (i=0; i<nx; i++){
	for (k=j; k<ny; k++){
	    cdata[k*nx + i] = min_red;
	}
    }
}

void attach_imginfo(ColorImginfo *&new_imginfo, ColorImginfo *old_imginfo)
{
    (new_imginfo = old_imginfo)->ref_count++;
}

void detach_imginfo(ColorImginfo *&imginfo)
{

    if (imginfo && --imginfo->ref_count <= 0){
	delete imginfo;
    }
    imginfo = NULL;
}


/****************************************************************/
/*	Zoom Routines						*/
/****************************************************************/
Gframe *
ColorImginfo::zoom_full(Gframe *)
{
    return(Gframe::big_gframe());
}

/************************************************************************
*                 
* zoom/unzoom
*
*************************************************************************/
void ColorImginfo::zoom(Gframe *)
{
    MSGERR("Zooming of color image not supported.");
}


void ColorImginfo::unzoom(Gframe *)
{
    MSGERR("Zooming of color image not supported.");
}

#ifdef NOT_DEFINED

/***********************************************************************
*
* ROI/Marker data and screen routines.
*
************************************************************************/


// Update data coords for everything in the display list
// See declaration of class Imginfo for explanation of xscale, xoffset, etc.
void ColorImginfo::update_data_coordinates()
{
    RoitoolIterator element(display_list);
    Marker *tool;

    update_scale_factors();

    while (element.NotEmpty()){
	tool = (Marker *)element++;
	tool->update_data_coords();
    }
}

// Update screen coords for everything in the display list
void ColorImginfo::update_screen_coordinates()
{
    RoitoolIterator element(display_list);
    Marker *tool;

    update_scale_factors();

    while (element.NotEmpty()){
	tool = (Marker *)element++;
	tool->update_screen_coords();
    }
}

/************************************************************************/
/* Convert x-coord from data to screen frame (ref marker_src2pix)	*/
/*									*/
/************************************************************************/
int
ColorImginfo::XDataToScreen(float xdata)
{
	if (data_swap == SWAP)
		xdata = xdata - (dim_f1-datastx-datawd);
	return (int) ((xdata*xscale) + xoffset);	
//	return (int) ((xdata*scale_x) + pixstx + pixoffx);	
}

/************************************************************************/
/* Convert y-coord from data to screen frame (ref marker_src2pix)	*/
/*									*/
/************************************************************************/
int
ColorImginfo::YDataToScreen(float ydata)
{
	return (int) ((ydata*(-1.0*yscale)) + yoffset);
}

/************************************************************************/
/* Convert x-coord from screen frame to data (ref marker_pix2src)	*/
/*									*/
/************************************************************************/
float    
ColorImginfo::XScreenToData(int xp)
{
    float xd = (xp - xoffset) / xscale;
    if (xd < 0){
        xd = 0;
    }else if (xd > dim_f1){
        xd = dim_f1;
    }
    if (data_swap == SWAP)
	xd = xd + (dim_f1-datastx-datawd);  
	// xd = datawd - xd;
    return xd;
}
 
/************************************************************************/
/* Convert y-coord from screen frame to data (ref marker_pix2src)	*/
/*									*/
/************************************************************************/
float
ColorImginfo::YScreenToData(int yp)
{
    float yd = -1.0*(yp - yoffset) / yscale;
    if (yd < 0){
        yd = 0;
    }else if (yd > max_intensity){
        yd = max_intensity;
    }    
    return yd;
}



/************************************************************************/
// Update the CSI scale factors
// See declaration of class Imginfo for explanation of xscale, xoffset, etc.
/************************************************************************/
void ColorImginfo::update_scale_factors()
{
    if (data_swap == SWAP)
    {
    	xscale = (double)pixwd/(double)datawd;
    	xoffset = pixstx-
	  ((((double)datastx+(double)datawd)*(double)pixwd/(double)datawd)+0.5);
    }
    else
    {
    	xscale = (double)pixwd / (double)datawd;
    	xoffset = pixstx-(((double)datastx*(double)pixwd/(double)datawd)+0.5);
    }
    yscale = (double)vs;
    yoffset = pixsty + (pixht - pixoffy) - 
     (((double)datasty*((double)pixht-(double)pixoffy)/(double)dataht) + 0.5);
}
  
/************************************************************************/
// Clip a point's coordinates to keep it inside the image
/************************************************************************/
void
ColorImginfo::keep_point_in_image(short *x, short *y)
{
    int left = XDataToScreen(0);
    int right = XDataToScreen(dim_f1 - 1);
    int bottom = YDataToScreen(-max_intensity+1);
    int top = YDataToScreen(max_intensity - 1);

    if (*x < left){
	*x = left;
    }else if (*x > right){
	*x = right;
    }

    if (*y < top){
	*y = top;
    }else if (*y > bottom){
	*y = bottom;
    }
}
#endif NOT_DEFINED
