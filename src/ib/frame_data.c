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
    return "@(#)frame_data.c 18.1 03/21/08 (c)1991-92 SISCO";
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
*  Routines related to load/dispay parameter and data of a frame.	*
*									*
*************************************************************************/

#include <math.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <unistd.h>
#include "stderr.h"
#include "graphics.h"
#include "gtools.h"
#include "roitool.h"
#include "imginfo.h"
#include "params.h"
#include "gframe.h"
#include "msgprt.h"
#include "statmacs.h"
// #include "iostream.h"
#include "ibcursors.h"
#include "ddllib.h"
#include "macroexec.h"

extern void win_print_msg(char *, ...);
extern void FlushGraphics(void);

// Initialize static class members
//int DDLNodeLink::refs = 0;
//int DDLNodeList::refs = 0;

int
Frame_data::load_data_oldformat(Gframe *gframe, char *dirpath, char *filename,
				int display_data_flag,
				int load_silently)

/*
 *  Load parameters and sisdata into a frame from a directory from
 *  <dir>/curpar, <dir>/datdir/phasefile.sis.
 *
 *  If there is no <dir>/datdir/phasefile.sis or it is out-of-data, it
 *  will convert <dir>/datdir/phasefile to <dir>/datdir/phasefile.sis.
 *
 *  Here are the steps for reading in data in the old format:
 *
 *  + Invoke Frame_data::load_params(gframe, exppath, curpar,...);
 *
 *      this creates the gframe->params structure
 *
 *  + Release the old gframe->imginfo (if it exists).
 *
 *  + Convert the phasefile into a SIS file (old format) if the SIS file
 *    is non-existent or if it is older than the phasefile.
 *
 *  + Invoke Frame_data::load_data(gframe, exppath, phasefile.sis, ...)
 *
 *  If any of the above steps fail, it is necessary to call
 *  PS_release(gframe->params) in order to deallocate the params structure
 *  
 */

{
  char exppath[256];   // path to the experiment being loaded
  char infile[256];    // filename for the phasefile
  char outfile[256];   // filename for the SIS format file
  char errmsg[128];	// error message buffer
  int temp_file_used = FALSE;
  struct stat buf;     // Holds status of phasefile
  double lpe, lro;	// field of view parameters
  char trace[4];   	// trace = "f1" or trace = "f2"
  Sisfile_info *sisfile;

  // Build experiment directory pathname
  (void)sprintf(exppath, "%s/%s", dirpath, filename);
  
  win_print_msg("Loading %s/%s/curpar.......", dirpath, filename);

  if (Frame_data::load_params(gframe, exppath, "curpar",
			      load_silently) == NOT_OK)    {    
    if (!load_silently) {
      msgerr_print("load_data: Error loading %scurpar", filename);
      win_print_msg("Loading %s/%sdatdir/phasefile.sis: ERROR",
		    dirpath, filename);
    }
    return NOT_OK;
  }

  // Build filenames for the convert routine
  (void)sprintf(infile, "%sdatdir/phasefile", exppath);
//  (void)sprintf(outfile, "%sdatdir/phasefile.sis", exppath);
  
  // First check status of phasefile and converted phasefile
  if (stat(infile, &buf) == -1)      {
    if (!load_silently) {
      msgerr_print("load_data:Couldn't find %s/%sdatdir/phasefile",
		   dirpath, filename);
      win_print_msg("Loading %s/%sdatdir/phasefile: ERROR",
		    dirpath, filename);
    }
    PS_release(gframe->params);
    gframe->params = NULL;	// So PS_read will allocate space next time
    return NOT_OK;
  }
  
    
    if (PS_get_string_value(gframe->params, "trace", 3, (char **)&trace)
	!= P_OK)
      (void)sprintf(trace, "f1"); 
    
    if (PS_get_real_value(gframe->params, "lpe", 1, &lpe) != P_OK)
      lpe = 1.0;
    if (PS_get_real_value(gframe->params, "lro", 1, &lro) != P_OK)
      lro = 1.0;
    
    if (strcmp(trace, "f1"))	  {
      if (!load_silently) {
	  msgerr_print(
	  "load_data:WARNING:phasefile with trace='f2' is not yet supported.");
      }
      double temp = lpe;
      lpe = lro;
      lro = temp;
    }    
    // release old image data before overwriting file
    if (gframe->imginfo) gframe->clear();
    detach_imginfo(gframe->imginfo);
    
    win_print_msg("Converting phasefile to SIS file format....");
    if (! (sisfile = Sisfile_info::sisdata_phase2sis(infile, 
			(float)lpe, (float)lro, errmsg) ) )
    {
	    if (!load_silently){
		msgerr_print(
		   "load_data:Can't convert %s/%sdatdir/phasefile to sisfile",
		   dirpath, filename);
		msgerr_print("  Reason:%s", errmsg);
		win_print_msg("Loading %s/ %sdatdir/phasefile.sis: ERROR",
			      dirpath, filename);
	    }
	    PS_release(gframe->params);
	    gframe->params = NULL; // So PS_read will allocate space next time
	    return NOT_OK;
    }
  
  win_print_msg("Loading %s/%sdatdir/phasefile.sis: .......",
		dirpath, filename);
  
  {
    char errmsg[128];	// Error message buffer
    double DEFAULT_VS = 4200.0;
    double vs = DEFAULT_VS;		// vertical scale

    // Release previous data; then allocate memory for new Imginfo
    detach_imginfo(gframe->imginfo);

    sisfile->SetDirpath(dirpath);
    sisfile->SetFilename(filename);


    if (sisfile->header.rank != RANK_2D){
	// Not a 2D data set--I can't handle this!
	msgerr_print("load_data_oldformat(): Not a 2D data set.");
	return NOT_OK;
    }

    gframe->imginfo = new Imginfo(sisfile->header.rank,
				  sisfile->header.bit,
				  sisfile->header.type,
				  sisfile->header.fast,
				  sisfile->header.medium,
				  sisfile->header.slow,
				  sisfile->header.hyperslow);

    // Copy data image from sisfile to st
    gframe->imginfo->st->SetData(sisfile->data, sisfile->DataLength());
    
    // Used gray-scale colormap
    gframe->imginfo->cmsindex = SISCMS_2;
    
    // Get the vertical scale.  Note that we retrieve a vertical scale value
    // from Vnmr, where its gray-leve is 64.  Now, we need to convert it to
    // the current gray-levels
    
    if (gframe->params && PS_exist(gframe->params, "vs"))   {
      if (PS_get_real_value(gframe->params, "vs", 1, &vs) == P_OK) {
	// From experience, we need to multiple the converted vs with 1.7
	// in order to obtain a good image
	vs *= (double) (G_Get_Sizecms2(gframe->gdev) / 64.0) * 1.7;
      } else {
	vs = DEFAULT_VS;
      }
    }
    
    DDLSymbolTable *st = gframe->imginfo->st;
    
    gframe->imginfo->vs = vs;
    st->SetValue("vs", vs);
//    st->SetValue("span", sisfile->header.ratio_fast, 0);
//    st->SetValue("span", sisfile->header.ratio_medium, 1);
    st->SetValue("span", (float)lpe, 0);
    st->SetValue("span", (float)lro, 1);
    st->SetValue("roi", (float)lpe, 0);
    st->SetValue("roi", (float)lro, 1);

    st->SetValue("dirpath", sisfile->GetDirpath());
    st->SetValue("filename", sisfile->GetFilename());
    

    // Display all the data
    if (display_data_flag) {
      display_data(gframe, 0, 0, sisfile->header.fast,
		   sisfile->header.medium,
		   vs);
    }

    win_print_msg("Loading %s/%sdatdir/phasefile: DONE",
		  dirpath, filename);

  }

  delete sisfile;
  return OK;
}


/************************************************************************
*                                                                       *
*  Load parameter data.                                                 *
*  Return OK or NOT_OK.                                                 *
*  (STATIC)                                                             *
*                                                                       */
int
Frame_data::load_params(Gframe *gframe, char *path, char *name, int silent)
{
    char filename[256];
 
    // Build the path to the parameter file
    sprintf(filename, "%s%s", path, name);
 
    // PS_read automatiaccally deals with previous paramaters
    if ((gframe->params = PS_read(filename, gframe->params)) == NULL)
    {
      if (!silent) {
        msgerr_print("load_params:PS_read:Couldn't load parameter");
      }
    }
    return (OK);
}

/************************************************************************
*									*
* Original display_data calling routine.  This was changed for using	*
* overlay_lists in gframe.						*
*									*/
void
Frame_data::display_data(
	Gframe *gframe,			// frame containing data
   	int src_stx, int src_sty,	// data source starting point
   	int src_wd, int src_ht,		// data source width and height
	float vs,			// vertical scale
	int init)                       // initialize canvas region
{
    Imginfo *imghead = NULL;		// pointer of image information header
    // Header of structure pointers
    imghead = gframe->imginfo;
    Frame_data::display_data(gframe,imghead,src_stx,src_sty,src_wd,
						src_ht,vs,init);
}
/************************************************************************
*									*
*  Display image from sisdata.						*
*  Note that it updates all information in Imginfo items.		*
*  									*
*  Before calling g_display_image, it needs to calculate the image	*
*  field of view and its zoomed factor based on the size of display-	*
*  region.   The field of view will indicate the ratio of image 	*
*  between its width and height source data .  The zoomed factor will	*
*  determine the resulting image will look like on the screen. This	*
*  include number of pixels for its width and height so that the image	*
*  will fit into the display-region.					*
*  (STATIC)								*
*									*/
void
Frame_data::display_data(
	Gframe *gframe,			// frame containing data
	Imginfo *imghead,		// pointer of image information header
   	int src_stx, int src_sty,	// data source starting point
   	int src_wd, int src_ht,		// data source width and height
	float vs,			// vertical scale
	int init)                       // initialize canvas region
{
    //cerr << "Start display_data()" << endl;

    
    // Store the image information
    imghead->datastx = src_stx;
    imghead->datasty = src_sty;
    imghead->datawd = src_wd;
    imghead->dataht = src_ht;
    imghead->vs = vs;

    // Update "imghead->pixstx, pixsty, pixwd, pixht" so that image portion
    // defined by datawd, dataht, just fits in the gframe.
    gframe->update_image_position(imghead);

    if (init){
	display_init(gframe);
    }
    gframe->set_clip_region(FRAME_CLIP_TO_IMAGE);

    imghead->display_data(gframe);

    gframe->set_clip_region(FRAME_NO_CLIP);
    gframe->clean = FALSE;
    if (init){
	display_end(gframe);
    }
}

/************************************************************************
*									*
*  Do a necessary set-up or clean-up before displaying image.		*
*  (STATIC)								*
*									*/
void
Frame_data::display_init(Gframe *gframe)
{
   // Clear display-region
   gframe->clear();

   // Unmark if it is marked
   if (gframe->select)
      gframe->mark();
}

/************************************************************************
*									*
*  Do a necessary clean-up after displaying image.			*
*  (STATIC)								*
*									*/
void
Frame_data::display_end(Gframe *gframe)
{
  // Print out title if it is required.  First try to use medium font,
  // and it it doesn't fit in, use small font.  If it still doesn't fit
  // in, clip the title.

  // Mark a frame if it is selected
  if (gframe->select)	
    gframe->mark();
  
  // Draw ROI (if it is active)
  //if (Roi_routine::roi_active())  Roi_routine::roi_draw();
    
  // Indicate the frame is not clean
  gframe->clean = FALSE; 
}

/************************************************************************
*									*
*  Set header data from an external program.
*  [MACRO interface]
*  argv[0]: (char *) The parameter name in the header
*  argv[1]: (char *) The command line for the program
*  [STATIC]
*									*/
int
Frame_data::SetHdr(int argc, char **argv, int retc, char **retv)
{
    argc--; argv++;

    if (argc != 2){
	ABORT;
    }
    set_hdr_parm(argv[0], argv[1]);
    return PROC_COMPLETE;
}

/************************************************************************
*									*
*  Get parameter values from an external program.
*  Run "prog", sending list of filepaths of selected frames to
*  standard input.  Program should send list of values to standard
*  output.  Respective output values are put in headers of selected
*  frames under the name "name".
*  
*  (STATIC)								*
*									*/
void
Frame_data::set_hdr_parm(char *name, // Name to use in header
			 char *prog) // The external program with arguments
{
    // Construct file names and command string
    char *command = new char[strlen(prog)+64];
    char srcfile[32];
    char dstfile[32];
    sprintf(srcfile,"/tmp/hdr_in%d", getpid());
    sprintf(dstfile,"/tmp/hdr_out%d", getpid());
    sprintf(command,"%s <%s >%s", prog, srcfile, dstfile);

    // Write file names to source file
    char *fname;
    Gframe *gf;
    FILE *fd = fopen(srcfile,"w");
    if (!fd){
	delete[] command;
	return;
    }
    for (gf=Frame_select::get_first_selected_frame();
	 gf;
	 gf=Frame_select::get_next_selected_frame(gf))
    {
	if (gf->imginfo && (fname=gf->imginfo->GetFilepath())){
	    fprintf(fd,"%s\n", fname);
	}
    }
    fclose(fd);

    // Run the program
    system(command);
    unlink(srcfile);		// Delete the source file

    // Load the output
    fd = fopen(dstfile,"r");
    if (!fd){
	sprintf(command,"Program failed: %s", prog);
	msgerr_print(command);
	delete[] command;
	return;
    }
    float value;
    int i=1;
    for (gf=Frame_select::get_first_selected_frame();
	 gf;
	 gf=Frame_select::get_next_selected_frame(gf))
    {
	if (gf->imginfo){
	    if (fscanf(fd,"%f", &value) != 1){
		sprintf(command,"Did not get value #%d from program \"%s\"",
			i, prog);
		msgerr_print(command);
		break;
	    }
	    gf->imginfo->st->SetValue(name, value);
	    i++;
	}
    }
    fclose(fd);
    unlink(dstfile);

    delete[] command;
    return;
}
