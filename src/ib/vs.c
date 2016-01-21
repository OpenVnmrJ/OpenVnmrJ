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
*  Routines related to adjusting vertical scale:			*
*	- by clicking the LEFT button on the image			*
*	- by enterring vs value	from input window			*
*									*
*************************************************************************/
#include <string.h>
#include <math.h>

#include "stderr.h"
#include "msgprt.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "common.h"
#include "inputwin.h"
#include "movie.h"
#include "contrast.h"
#include "macroexec.h"
#include "vs.h"
#include "vscale.h"

// Initialize static class members
Flag Vscale::bind = 0;
int Vscale::vs_band = 9;
Menu_item Vscale::bind_menu_item = 0;

/************************************************************************
*									*
*  Creator of vs-routine.                                             	*
*  (This function can only be called once.)                             *
*                                                                       */
Vs_routine::Vs_routine(Gdev *)
{
   active = FALSE;

   props_menu =
   xv_create(NULL, MENU,
	     MENU_GEN_PIN_WINDOW, Gtools::get_gtools_frame(), "Vs Props",
	     MENU_ITEM,
	     MENU_STRING,            "V-scale ...",
	     MENU_NOTIFY_PROC,               &Vs_routine::menu_handler,
	     MENU_CLIENT_DATA,       VS_VS,
	     NULL,
	     MENU_ITEM,
	     MENU_STRING,		"Gamma ...",
	     MENU_NOTIFY_PROC,               &Vs_routine::menu_handler,
	     MENU_CLIENT_DATA,       VS_CONTRAST,
	     NULL,
	     NULL);
   Vscale::bind_menu_item =
   (Menu_item)xv_create(NULL, MENUITEM,
			MENU_STRING, "Bind",
			MENU_NOTIFY_PROC, &Vs_routine::menu_handler,
			MENU_CLIENT_DATA, VS_BIND,
			NULL);
   xv_set(props_menu, MENU_APPEND_ITEM, Vscale::bind_menu_item, NULL);
   Vscale::bind = FALSE;
}

/************************************************************************
*                                                                       *
*  Execute user selected menu.                                           *
*                                                                       */
void       
Vs_routine::menu_handler(Menu, Menu_item i)
{
   int j;

   switch ((Vs_props_menu)xv_get(i, MENU_CLIENT_DATA))
   {       
      case VS_BIND:
         if (Vscale::bind){
	     Vscale::set_bind(FALSE);
	 }else{
	     Vscale::set_bind(TRUE);
	 }
	 break;

      case VS_VS:
	 Vscale::vscale();
	 /*inputwin_show((int)VS_VS, &Vscale::set_attr,
	    "V-scale (Range: VS >= 0)");

	 // Init VS field to VS of first selected frame with an image
         Gframe *gframe;      // loop for selected frame
         for (j=1, gframe=Frame_select::get_selected_frame(j);
              gframe;
              gframe = Frame_select::get_selected_frame(++j))
         {
	     if (gframe->imginfo){
		 set_vscale(gframe->imginfo->vs);
		 break;
	     }
	     }*/
	 break;

       case VS_CONTRAST:
	 Vscale::contrast();
	 break;
   }
}

/************************************************************************
*                                                                       *
*  Initialize anything related to Vs.  It is called when the user just	*
*  selects the gtool Vs.                                              	*
*                                                                       */
void       
Vs_routine::start(Panel props, Gtype)
{          
   active = TRUE;
   xv_set(props, PANEL_ITEM_MENU, props_menu, NULL);
   Gtools::set_props_label("V-scale Properties");
}

/************************************************************************
*                                                                       *
*  Clean-up routine (about to leave gtool Vs).  It is called when     	*
*  the user has selected another tool.                                  *
*                                                                       */
void                                                                     
Vs_routine::end(void)
{                                                                        
   G_Set_LineWidth(Gframe::gdev, 0);
   active = FALSE;
}

/************************************************************************
*									*
*  Reset colormap contrast settings
*  [MACRO interface]
*  argc=0: Display the contrast window
*	OR
*  argc=1:	linear mappping (no gamma correction)
*  argv[0]: (double) Slope of contrast line
*	OR
*  argc=2:	linear mappping (no gamma correction)
*  argv[0]: (double) Intensity of 0 value pixel
*  argv[1]: (double) Intensity of highest value pixel
*	OR
*  argc=3:	gamma correction
*  argv[0]: (double) Intensity of 0 value pixel
*  argv[1]: (double) Intensity of highest value pixel
*  argv[2]: (double) gamma ( I = k * V**gamma )
*	OR
*  argc=4:	gamma correction with logarithmic intensity steps
*  argv[0]: (double) Intensity of 0 value pixel
*  argv[1]: (double) Intensity of highest value pixel
*  argv[2]: (double) gamma ( I = k * V**gamma )
*  argv[3]: (double) max screen contrast ( I(max)/I(min) )
*
*  [STATIC Function]
***********************************************************************/
int
Vscale::Contrast(int argc, char **argv, int, char **)
{
    argc--; argv++;

    switch (argc){
      case 0:			// Just bring up the window
	contrast();
	break;
      case 1:
	{
	    double lefty;
	    double righty;
	    double slope;
	    if (MacroExec::getDoubleArgs(argc, argv, &slope, 1) != 1){
		ABORT;
	    }
	    righty = (1 + slope) / 2;
	    lefty = 1 - righty;
	    set_contrast(lefty, righty, 0.0, 0.0);
	    break;
	}
      case 2:
      case 3:
      case 4:
	{
	    double y[4] = {0.0, 0.0, 0.0, 0.0};
	    if (MacroExec::getDoubleArgs(argc, argv, y, 4) < 2){
		ABORT;
	    }
	    set_contrast(y[0], y[1], y[2], y[3]);
	    break;
	}
      default:
	ABORT;
    }
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Display the vscale window
*
*  [STATIC Function]							*
*									*/
void
Vscale::vscale()
{
    vs_win_show();
    /*macroexec->record("display_vscale\n");/*CMP*/
    return;
}

void
Vscale::vscale_callback(VsFunc *vsf)
{
    rescale_image(vsf);
    if (vsf->command){
	/*macroexec->record("display_vscale(%s)\n", vsf->command);/*CMP*/
    }
}

void
Vscale::vscale_init()
{
    vs_notify_func(Vscale::vscale_callback);
    return;
}

/************************************************************************
*									*
*  Display the contrast window
*
*  [STATIC Function]							*
*									*/
void
Vscale::contrast()
{
    contrast_win_show();
    macroexec->record("display_contrast\n");
    return;
}

void
Vscale::contrast_init()
{
    contrast_notify_func((long)contrast_callback);
    solarization_notify_func((long)solar_callback);
    contrast_mask_off();
    return;
}

void
Vscale::contrast_callback(float lefty,
			  float righty,
			  float gamma,
			  float contrast)
{
    if (gamma == 0){
	macroexec->record("display_contrast(%.2f, %.2f)\n", lefty, righty);
    }else if (contrast == 0){
	macroexec->record("display_contrast(%.2f, %.2f, %.2f)\n",
			  lefty, righty, gamma);
    }else{
	macroexec->record("display_contrast(%.2f, %.2f, %.2f, %.0f)\n",
			  lefty, righty, gamma, contrast);
    }
}

void
Vscale::solar_callback(int flag)
{
    const char *sflag = flag ? "on" : "off";
    macroexec->record("display_saturation('%s')\n", sflag);
}

/************************************************************************
*									*
*  Set the "saturation" display choice
*  [MACRO interface]
*  argv[0]: (char *) on | off
*  [STATIC Function]							*
*									*/
int
Vscale::Saturate(int argc, char **argv, int, char **)
{
    argc--; argv++;

    char *flag;
    if (argc != 1){
	ABORT;
    }
    flag = argv[0];
    if (strcasecmp(flag, "on") == 0){
	solarization(TRUE);
    }else if (strcasecmp(flag, "off") == 0){
	solarization(FALSE);
    }else{
	ABORT;
    }
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Set the "binding" choice
*  [MACRO interface]
*  argv[0]: (char *) on | off
*  [STATIC Function]							*
*									*/
int
Vscale::Bind(int argc, char **argv, int, char **)
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
Vscale::set_bind(int flag)
{
    char *sflag;
    if (flag){
	xv_set(bind_menu_item, MENU_STRING, "Unbind", NULL);
	bind = TRUE;
	sflag = "on";
    }else{
	xv_set(bind_menu_item, MENU_STRING, "Bind", NULL);
	bind = FALSE;
	sflag = "off";
    }
    macroexec->record("display_vs_bind('%s')\n", sflag);
    return;
}

/************************************************************************
*                                                                       *
*  Mouse event Graphics-tool: Vs.                                     	*
*  This function will be called if there is an event related to Vs.   	*
*                                                                       */
Routine_type   
Vs_routine::process(Event *e)
{          
   int x = event_x(e);
   int y = event_y(e);
   static int x_ref;
   static int y_ref;
   static int adjust = FALSE;
           
   switch (event_action(e))
   {
      case ACTION_SELECT:
	 if (event_is_down(e)){
            Gframe *gframe;      // loop for selected frame
            Imginfo *iptr;       // image information pointer

	    // Get the frame that contains the cursor
	    int iframe;
            for (gframe=Gframe::get_first_frame(), iframe=1;
                 gframe;
                 gframe=Gframe::get_next_frame(gframe), iframe++)
            {
		if (iptr=gframe->imginfo){
		    if (com_point_in_rect(x, y,
					  iptr->pixstx,
					  iptr->pixsty,
					  iptr->pixstx + iptr->pixwd - 1,
					  iptr->pixsty + iptr->pixht - 1
					  )
			)
		    {
			break;
		    }
		}
	    }
	    if (!gframe){
		break;
	    }

	    // The point is inside image in this gframe.
	    if (event_ctrl_is_down(e)){
		gframe->imginfo->AutoVscale();
		gframe->imginfo->display_ood = TRUE;
		Frame_data::redraw_ood_images();
	    }else if (gframe->update_image_position()){
		if (event_shift_is_down(e)){
		    gframe->imginfo->vsfunc->min_data = 0;
		    set_vscale(gframe->imginfo->vsfunc->max_data, 0.0);
		}
		float fx = iptr->XScreenToData(x);
		float fy = iptr->YScreenToData(y);
		Vscale::rescale_image(fx, fy, gframe);
            }
	 } // if (event_is_down)
	 break;
     case ACTION_ADJUST:
	 if (event_is_down(e)){
	     adjust = TRUE;
	     save_colormap();
	     x_ref = x;
	     y_ref = y;
	 }else{
	     // event_is_up
	     adjust = FALSE;
	     restore_colormap(0.0, 1.0);
	     float dy = (y - y_ref) / 100.0;
	     float dx = (x - x_ref) / 500.0;
	     float xrange = pow(2.0, dy);
	     float xc = 0.5 - dx;
	     Vscale::rescale_image(xc + xrange/2, xc - xrange/2, TRUE);
	 }
	 break;
     case LOC_DRAG:
	 if (adjust){
	     float dy = (y - y_ref) / 100.0;
	     float dx = (x - x_ref) / 500.0;
	     float xrange = pow(2.0, dy);
	     float xc = 0.5 - dx;
	     restore_colormap(xc - xrange/2, xc + xrange/2);
	 }
	 break;
   } // switch (event_action)
   return(ROUTINE_DONE);
}

/************************************************************************
*                                                                       *
*  Calculate a new vertical scale.					*
*  Looks at all data in frame (main data and overlays) and calculates
*  new "vs" from first image found.
*  Returns new calculated vs value, or 0 on error.
*  (STATIC)								*
*									*/
float
Vscale::calculate_new_vs(Gframe *gframe, float x, float y)
{
    float newvs;
    Imginfo *img = gframe->imginfo;
    if (!img){
	return 0;
    }

    if (newvs = img->calculate_new_image_vs(x, y, vs_band)){
	return newvs;
    }
    
    ImginfoIterator element(gframe->overlay_list);
    Imginfo *overlay;
    while (element.NotEmpty()){
	overlay = ++element;
	if (newvs = overlay->calculate_new_image_vs(x, y, vs_band)){
	    return newvs;
	}
    }
    return 0;
}
    
/************************************************************************
*                                                                       *
*  Set new vs value in all images in a frame.
*  Returns TRUE if anything was changed, FALSE if nothing changed.
*  (STATIC)                                                             *
*									*/
Flag
Vscale::set_new_vs(Gframe *gframe, float new_vs)
{
    Movie_frame *mhead;

    if ( ! gframe->imginfo){
	return FALSE;
    }
    
    Flag rtn = FALSE;
    if (gframe->imginfo->set_new_image_vs(new_vs)){
	rtn = TRUE;
    }
    
    ImginfoIterator element(gframe->overlay_list);
    Imginfo *overlay;
    while (element.NotEmpty()){
	overlay = ++element;
	if (overlay->set_new_image_vs(new_vs)){
	    rtn = TRUE;
	}
    }
    if (Vscale::bind && (mhead = in_a_movie(gframe->imginfo))){
	// Set scale on all images in movie.
	Movie_frame *mframe = mhead->nextframe;
	do{
	    mframe->img->vsfunc->max_data = new_vs;
	    mframe = mframe->nextframe;
	} while (mframe != mhead->nextframe);
    }
    if (rtn){
	gframe->display_data();
    }
    return rtn;
}

/************************************************************************
*                                                                       *
*  Set new min and max data values in all images in a frame.
*  Max_data and min_data are NORMALIZED max and min values, i.e.,
*  0==>old min value, 1==> old max value, 0.5==>halfway between.
*  Returns TRUE if anything was changed, FALSE if nothing changed.
*  (STATIC)                                                             *
*									*/
Flag
Vscale::set_new_vs(Gframe *gframe, float max_data, float min_data,
		   Flag set_vstool)
{
    Movie_frame *mhead;

    if ( ! gframe->imginfo){
	return FALSE;
    }
    
    Flag rtn = FALSE;
    if (gframe->imginfo->set_new_image_vs(max_data, min_data, set_vstool)){
	rtn = TRUE;
    }
    
    ImginfoIterator element(gframe->overlay_list);
    Imginfo *overlay;
    while (element.NotEmpty()){
	overlay = ++element;
	if (overlay->set_new_image_vs(max_data, min_data, set_vstool)){
	    rtn = TRUE;
	}
    }
    if (Vscale::bind && (mhead = in_a_movie(gframe->imginfo))){
	// Set scale on all images in movie.
	Movie_frame *mframe = mhead->nextframe;
	do{
	    mframe->img->vsfunc->max_data = max_data;
	    mframe->img->vsfunc->min_data = min_data;
	    mframe = mframe->nextframe;
	} while (mframe != mhead->nextframe);
    }
    if (rtn){
	gframe->display_data();
    }
    return rtn;
}

/************************************************************************
*                                                                       *
*  Set new vertical scale function in all images in a frame.
*  Returns TRUE if anything was changed, FALSE if nothing changed.
*  (STATIC)                                                             *
*									*/
Flag
Vscale::set_new_vs(Gframe *gframe, VsFunc *newfunc)
{
    Movie_frame *mhead;

    if ( ! gframe->imginfo){
	return FALSE;
    }
    
    Flag rtn = FALSE;
    if (gframe->imginfo->set_new_image_vs(newfunc)){
	rtn = TRUE;
    }
    
    ImginfoIterator element(gframe->overlay_list);
    Imginfo *overlay;
    while (element.NotEmpty()){
	overlay = ++element;
	if (overlay->set_new_image_vs(newfunc)){
	    rtn = TRUE;
	}
    }
    if (Vscale::bind && (mhead = in_a_movie(gframe->imginfo))){
	// Set scale on all images in movie.
	Movie_frame *mframe = mhead->nextframe;
	do{
	    delete mframe->img->vsfunc;
	    mframe->img->vsfunc = new VsFunc(newfunc);
	    mframe = mframe->nextframe;
	} while (mframe != mhead->nextframe);
    }
    if (rtn){
	gframe->display_data();
    }
    return rtn;
}

/************************************************************************
*									*
*  Reset "vertical" scale
*  [MACRO interface]
*  argv[0]: (double) new VS value to apply to all selected frames
*	OR
*  argv[0]: (double) X pixel coordinate
*  argv[1]: (double) Y pixel coordinate
*  argv[2]: (int) <optional> frame number
*	Apply auto vs scaling at pixel X,Y either for all selected frames
*	(binding ignored) or for the specified frame (and then also apply
*	the same VS to other selected frames if binding is on)
*  [STATIC Function]							*
*									*/
int
Vscale::Vs(int argc, char **argv, int, char **)
{
    argc--; argv++;
    switch (argc){
      case 1:			// Specified vs applied to all selected frames
	{
	    double vs;
	    if (MacroExec::getDoubleArgs(argc, argv, &vs, 1) != 1){
		ABORT;
	    }
	    rescale_image(vs);
	    break;
	}
      case 2:
	{
	    double pix[2];
	    if (MacroExec::getDoubleArgs(argc, argv, pix , 2) != 2){
		ABORT;
	    }
	    rescale_image(pix[0], pix[1]);
	    break;
	}
      case 3:
	{
	    int i;
	    double pix[2];
	    int nframe;
	    Gframe *fptr;
	    if (MacroExec::getDoubleArgs(argc, argv, pix , 2) != 2){
		ABORT;
	    }
	    if (MacroExec::getIntArgs(argc-2, argv+2, &nframe, 1) != 1){
		ABORT;
	    }
	    for (fptr=Gframe::get_first_frame(), i=1;
		 i<nframe && fptr;
		 fptr=Gframe::get_next_frame(fptr), i++);
	    if (!fptr){
		msgerr_print("There is no frame #%d", nframe);
		ABORT;
	    }
	    rescale_image(pix[0], pix[1], fptr);
	    break;
	}
      default:
	ABORT;
    }
    return PROC_COMPLETE;
}

/************************************************************************
*                                                                       *
*  Redisplay image with a new vs value for all selected frames
*  (STATIC)                                                             *
*									*/
void
Vscale::rescale_image(float newvs)
{
    Gframe *gframe;      // loop for selected frame
    int i;		 // loop counter
    int j;
    
    for (i=1, j=0, gframe=Frame_select::get_selected_frame(i);
	 gframe;
	 i++, gframe=Frame_select::get_selected_frame(i))
    {
	set_new_vs(gframe, newvs);
	if (j == 0){
	    set_vscale(newvs);
	    j++;
	}
    }
    macroexec->record("display_vs(%.4g)\n", newvs);
}

/************************************************************************
*                                                                       *
*  Redisplay image with a new vs value for all selected frames
*  Max_data and min_data are NORMALIZED to the range 0-1 for the old values.
*  (STATIC)                                                             *
*									*/
void
Vscale::rescale_image(float max_data, float min_data, Flag set_vstool)
{
    Gframe *gframe;      // loop for selected frame
    int i;		 // loop counter
    int j;
    
    for (i=1, j=0, gframe=Frame_select::get_selected_frame(i);
	 gframe;
	 i++, gframe=Frame_select::get_selected_frame(i))
    {
	set_new_vs(gframe, max_data, min_data, i==1 && set_vstool);
	/*if (j == 0 && set_vstool){
	    set_vscale(max_data, min_data);
	    j++;
	    }*/
    }
    //macroexec->record("display_vs(%.4g)\n", newvs);
}

/************************************************************************
*                                                                       *
*  Redisplay image with a new vertical scale function for all selected
*  frames
*  (STATIC)                                                             *
*									*/
void
Vscale::rescale_image(VsFunc *vsfunc)
{
    Gframe *gframe;      // loop for selected frame
    int i;		 // loop counter
    int j;
    
    for (i=1, j=0, gframe=Frame_select::get_selected_frame(i);
	 gframe;
	 i++, gframe=Frame_select::get_selected_frame(i))
    {
	set_new_vs(gframe, vsfunc);
    }
    /*macroexec->record("display_vs(%.4g)\n", newvs);*/
}

/************************************************************************
*                                                                       *
*  Calculate a new vs value for each selected frame and redisplay
*  (STATIC)                                                             *
*									*/
void
Vscale::rescale_image(float x,	// X-coord of pixel to scale to
		      float y)	// Y-coord of pixel to scale to
{
    Gframe *gframe;		// loop for selected frame
    int i;			// loop counter
    float newvs;
    
    for (i=1, gframe=Frame_select::get_selected_frame(i);
	 gframe;
	 i++, gframe=Frame_select::get_selected_frame(i))
    {
	if (newvs = calculate_new_vs(gframe, x, y)){
	    set_new_vs(gframe, newvs);
	    set_vscale(newvs);
	}
    }
    macroexec->record("display_vs(%.2f, %.2f)\n", x, y);
}

/************************************************************************
*                                                                       *
*  Calculate a new vs value for a frame and redisplay.  If binding on,
*  rescale all selected frames to the same scale.
*  (STATIC)                                                             *
*									*/
void
Vscale::rescale_image(float x,	// X-coord of reference pixel
		      float y,	// Y-coord of reference pixel
		      Gframe *gframe)
{
    int i;			// loop counter
    float newvs;
    Gframe *gf;
    
    if (newvs = calculate_new_vs(gframe, x, y)){
	// Got a new scale
	if (bind){
	    if ( ! gframe->is_a_selected_frame() ){
		set_new_vs(gframe, newvs); // Rescale this image
	    }
	    // Rescale all selected frame images
	    for (i=1, gf=Frame_select::get_selected_frame(i);
		 gf;
		 i++, gf=Frame_select::get_selected_frame(i))
	    {
		set_new_vs(gf, newvs);
	    }
	}else{
	    set_new_vs(gframe, newvs); // Rescale only this image
	}
	set_vscale(newvs);

	// Get the serial number of gframe
	int iframe;
	for (gf=Gframe::get_first_frame(), iframe=1;
	     gf && gf != gframe;
	     gf=Gframe::get_next_frame(gf), iframe++);
	macroexec->record("display_vs(%.2f, %.2f, %d)\n",
			  x, y, iframe);
    }
}

/************************************************************************
*                                                                       *
*  Set Vs attributes: vs-value						*
*  Return OK or NOT_OK.                                                 *
*  (STATIC)                                                             *
*                                                                       */
int
Vscale::set_attr(int id, char *attr_str)
{
    if (id == VS_VS){
	Gframe *gframe;		// loop for selected frame
	int i;			// loop counter
	Imginfo *imgf;		// image information pointer
	float val;
	
	if (*attr_str == '?'){
	    msginfo_print("Vertical-scale:\n");
	    for (i=1, gframe=Frame_select::get_selected_frame(i);
		 gframe;
		 i++, gframe=Frame_select::get_selected_frame(i))
	    {
		if (imgf = gframe->imginfo){
		    if (imgf->GetFilename()){
			msginfo_print("\t%s:\t%g\n",
				      imgf->GetFilename(), imgf->vs);
		    }else{
			msginfo_print("\t<no-name>:\t%g\n",imgf->vs);
		    }
		}
	    }
	}else if (((val = (float)atof(attr_str)) < 0)){
	    msgerr_print("Vs is out of range (%f)", val);
	    return(NOT_OK);
	}else{
	    rescale_image(val);
	}
    }
    return(OK);
}
