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
*  Ramani Pichumani                     
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							* 
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  Routine related to sisfile header and data.				*
*									*
*************************************************************************/
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
#include <sys/param.h>
#include <unistd.h>
#include <string.h>
#include "ddllib.h"
#include "msgprt.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "stderr.h"
#include "ddlfile.h"
#include "roitool.h"
#include "zoom.h"
#include "movie.h"
#include "ibcursors.h"
#include "common.h"
#include "vscale.h"

#ifndef __OSFCN_H
#ifndef __SYSENT_H
/* This is needed if osfcn.h or sysent.h is not included anywhere */
// extern "C" int ftruncate(int, int);  this breaks new compilers, conflicting definitions
#endif
#endif

#define MSGERR1(str,arg1)\
	   (void)msgerr_print(str"\n", arg1);
#define MSGERR0(str)\
	   (void)msgerr_print(str"\n");

extern void FlushGraphics();

// extern "C" char* strstr(const char*, const char*);

/************************************************************************
*
*  Create a list class for imginfo to use with 
*
*/
Create_ListClass(Imginfo);

ImginfoLink::~ImginfoLink() {}

ImginfoLink& ImginfoLink::Print() {
  printf("object[%d]\n", item);
  return *this;
}

/************************************************************************
*
*  Load image.  It maps the file and put the necessary information
*  into Imginfo.
*  It will put an error message into 'errmsg' if error occurs and if 
*  'errmsg' is not equal to NULL.					
*
*  Return a pointer to Imginfo or NULL.		      
*
*/

Imginfo* 
Imginfo::LoadImage(char *path, char *name, char *errmsg,
		   DDLSymbolTable *ddlst)
{
  Imginfo *imginfo_rtn;
  char *filename = NULL;

  if (path && name){
      filename = new char[strlen(path) + strlen(name) + 2];
      strcpy(filename, path);
      strcat(filename, "/");
      strcat(filename, name);
  }

  imginfo_rtn = LoadImage(filename, errmsg, ddlst);
  delete [] filename;
  return imginfo_rtn;
}

Imginfo* 
Imginfo::LoadImage(char *filename, char *errmsg,
		   DDLSymbolTable *ddlst)
{
  return ddldata_load(filename, errmsg, ddlst);
}

 /*
  *  The following Imginfo constructor creates a new Imginfo that is a copy
  *  of another Imginfo
  *
  */

Imginfo::Imginfo(Imginfo *original)
{
    Initialize();
    *this = *original;
    cmsindex = original->cmsindex;
    pixstx = original->pixstx;
    pixsty = original->pixsty;
    pixwd = original->pixwd;
    pixht = original->pixht;
    datastx = original->datastx;
    datasty = original->datasty;
    datawd = original->datawd;
    dataht = original->dataht;
    vs = original->vs;
    vsfunc = new VsFunc(original->vsfunc);
    pixmap_fmt.vsfunc = 0;
    type = original->type;
    zlinex1 = original->zlinex1;
    zliney1 = original->zliney1;
    zlinex2 = original->zlinex2;
    zliney2 = original->zliney2;
    st = (DDLSymbolTable *)original->st->CloneList();
    display_list = new RoitoolList();	// Don't copy ROI's/Labels!
    display_ood = TRUE;			// Display is out of date.
    pixmap = 0;				// No stored image initially.
    maskmap = 0;			// No stored image mask initially.
    ref_count = 1;			// Create with one reference.
}  

/*
**
**  Initialize() places default values in zlinex,y and cmsindex
**
**
*/
     
void 
Imginfo::Initialize()
{
  pixmap_fmt.pixwd = pixmap_fmt.pixht = pixmap_fmt.datastx = 0;
  pixmap_fmt.datasty = pixmap_fmt.datawd = pixmap_fmt.dataht = 0;
  pixmap_fmt.vsfunc = 0;
  pixmap_ood = TRUE;
  datastx = datasty = datawd = dataht = 0;
  zlinex1 = zliney1 = 0.25;
  zlinex2 = zliney2 = 0.75;
  cmsindex = SISCMS_2;
  st = 0;
  vs = 1;
  vo = 0;
  vsfunc = new VsFunc(get_default_vsfunc());
  disp_type = IMAGE;
  display_list = new RoitoolList();
  display_ood = TRUE;		// Display initially out of date.
  pixmap = 0;			// No stored image initially.
  maskmap = 0;			// No stored image mask initially.
  ref_count = 1;		// Create with one reference.
}

Imginfo::~Imginfo(void)
{
    RoitoolIterator element(display_list);
    Roitool *tool;
    while (element.NotEmpty()){
	tool = ++element;
	tool->deselect();
    }

    delete vsfunc;
    delete pixmap_fmt.vsfunc;
    if (st) st->Delete();
    if (pixmap) XFreePixmap(display, pixmap);
    if (maskmap) XFreePixmap(display, maskmap);
}

int
Imginfo::pixmap_re_scale()
{
    int rtn = FALSE;
    if ( pixwd != pixmap_fmt.pixwd ||
	 pixht != pixmap_fmt.pixht ||
	 datastx != pixmap_fmt.datastx ||
	 datasty != pixmap_fmt.datasty ||
	 datawd != pixmap_fmt.datawd ||
	 dataht != pixmap_fmt.dataht)
    {
	rtn = TRUE;
    }
    return rtn;
}

int
Imginfo::pixmap_re_vs()
{
    return vs != pixmap_fmt.vs;
}

int
Imginfo::need_new_pixmap()
{
    int rtn = FALSE;
    if ( !pixmap || pixmap_re_scale() || pixmap_re_vs() ){
	rtn = TRUE;
    }
    if (rtn && pixmap){
	// We have an obsolete pixmap--free it
	XFreePixmap(display, pixmap);
	pixmap = 0;
    }
    return rtn;
}

void
Imginfo::remove_pixmap_for_updt()
{
    if (pixmap){
	// We have an obsolete pixmap--free it
	XFreePixmap(display, pixmap);
	pixmap = 0;
    }
}

void
attach_imginfo(Imginfo *&new_imginfo, Imginfo *old_imginfo)
{
    new_imginfo = old_imginfo;
    if (old_imginfo){
	(old_imginfo)->ref_count++;
    }
}

void
detach_imginfo(Imginfo *&imginfo)
{

    if (imginfo && --imginfo->ref_count <= 0){
	delete imginfo;
    }
    imginfo = NULL;
}

char* 
Imginfo::GetData()
{
  if (st) {
    return st->GetData();
  } else {
    return NULL;
  }
}

char* 
Imginfo::GetDirpath()
{
  char *ret = NULL;

  if (st) {
    st->GetValue("dirpath", ret);
  }
  return ret;
}

char* 
Imginfo::GetFilename()
{
  char *ret = NULL;

  if (st) {
    st->GetValue("filename", ret);
  }
  return ret;
}

char* 
Imginfo::GetFilepath()
{
    char *name;
    static char fullname[MAXPATHLEN];

    *fullname = '\0';
    name = GetDirpath();
    if (name){
	strcpy(fullname, name);
    }
    if (fullname[strlen(fullname) - 1] != '/'){
	strcat(fullname, "/");
    }
    name = GetFilename();
    if (name){
	if (*name == '/'){
	    name++;
	}
	strcat(fullname, name);
    }
    // Delete extra leading "/"
    char *cp;
    for (cp=fullname ; *cp == '/' && *(cp+1) == '/'; cp++);
    return cp;
}

Flag 
Imginfo::GetOrientation(double *orientation)
{
    int i;
    for (i=0; i<9; i++){
	if ( ! st->GetValue("orientation", orientation[i], i) ){
	    return FALSE;
	}
    }
    return TRUE;
}


int 
Imginfo::GetRank()
{
  int ret = 2;

  if (st) {
    st->GetValue("rank", ret);
  }
  switch (ret) {
  case 1:
    ret = RANK_1D;
    break;
	
  case 2:
    ret = RANK_2D;
    break;

  case 3:
    ret = RANK_3D;
    break;

  case 4:
    ret = RANK_4D;
    break;
    
  default:
    ret = RANK_2D;
  }
  
  return ret;
}

int 
Imginfo::GetFast()
{
  int ret = 256;

  if (st) {
      st->GetValue("matrix", ret, 0);
  }
  return ret;
}

int 
Imginfo::GetMedium()
{
  int ret = 256;

  if (st) {
      st->GetValue("matrix", ret, 1);
  }
  return ret;
}

int 
Imginfo::GetSlow()
{
  int ret = 256;

  if (st) {
      st->GetValue("matrix", ret, 2);
  }
  return ret;
}

double 
Imginfo::GetRatioFast()
{
  double ret = 1.0;

  if (st) {
      st->GetValue("span", ret, 0);
  }
  return ret;
}

double 
Imginfo::GetRatioMedium()
{
  double ret = 1.0;

  if (st) {
      st->GetValue("span", ret, 1);
  }
  return ret;
}

double 
Imginfo::GetRatioSlow()
{
  double ret = 1.0;

  if (st) {
      st->GetValue("span", ret, 2);
  }
  return ret;
}

Imginfo::Imginfo(Sisfile_rank new_rank,
		 Sisfile_bit new_bit,
		 Sisfile_type new_type,
		 int new_fast, 
		 int new_medium, 
		 int new_slow, 
		 int new_hyperslow,
		 int alloc_data)
{
  Initialize();

  st = new DDLSymbolTable();
  InitializeSymTab(new_rank,
		   new_bit,
		   new_type,
		   new_fast, 
		   new_medium, 
		   new_slow, 
		   new_hyperslow,
		   alloc_data);
}

void
Imginfo::InitializeSymTab(Sisfile_rank new_rank,
			  Sisfile_bit new_bit,
			  Sisfile_type new_type,
			  int new_fast, 
			  int new_medium, 
			  int new_slow, 
			  int new_hyperslow,
			  int alloc_data)
{

  switch (new_rank) {
      
    case RANK_1D:
      st->SetValue("rank", 1);
      break;
      
    case RANK_2D:
      st->SetValue("rank", 2);
      break;
      
    case RANK_3D:
      st->SetValue("rank", 3);
      break;
      
    case RANK_4D:
      st->SetValue("rank", 4);
      break;
      
    default:
      st->SetValue("rank", 2);
      break;
      
  }
  
  // st->SetValue("bit", new_bit);
  switch (new_bit) {
      
    case BIT_1:
      st->SetValue("bits", 1);
      break;
      
    case BIT_8:
      st->SetValue("bits", 8);
      break;
      
    case BIT_12:
      st->SetValue("bits", 12);
      break;
      
    case BIT_16:
      st->SetValue("bits", 16);
      break;
      
    case BIT_32:
      st->SetValue("bits", 32);
      break;
      
    case BIT_64:
      st->SetValue("bits", 64);
      break;
      
    default:
      st->SetValue("bits", 32);
      break;
      
  }
  
  type = new_type;
  
  /* Don't confuse ddl's "storage" with procpar's "type" */
  switch (type) {
      
    case TYPE_CHAR:
      st->SetValue("storage", "char");
      break;
      
    case TYPE_SHORT:
      st->SetValue("storage", "short");
      break;
      
    case TYPE_INT:
      st->SetValue("storage", "int");
      break;
      
    case TYPE_FLOAT:
      st->SetValue("storage", "float");
      break;
      
    case TYPE_DOUBLE:
      st->SetValue("storage", "double");
      break;
      
    case TYPE_ANY:
    default:
      st->SetValue("storage", "float");
      break;
      
  }
  
  st->CreateArray("matrix");
  st->SetValue("matrix", new_fast, 0);
  st->SetValue("matrix", new_medium, 1);
  datastx = datasty = 0;
  datawd = new_fast;
  dataht = new_medium;
  
  st->CreateArray("abscissa");
  st->SetValue("abscissa", "cm", 0);
  st->SetValue("abscissa", "cm", 1);
  
  st->CreateArray("ordinate");
  st->SetValue("ordinate", "intensity", 0);
  
  
  int typesize;		/* data type of size */
  
  switch (type)  {
      
    case TYPE_SHORT:
      typesize = sizeof(short);
      break;
      
    case TYPE_FLOAT:
      typesize = sizeof(float);
      break;
      
    default:
      PERROR_1("Sisfile:This data TYPE (%d) is not supported yet", type);
      typesize = sizeof(int); break;
  }
  
  int filesize = sizeof(Sisheader) +
  new_fast * new_medium * new_slow * new_hyperslow * typesize;
  
  if (alloc_data) {
      char *new_data = new char[filesize];
      if (new_data == NULL) {
	  PERROR_1("Imginfo(): cannot allocate %d bytes!\n", filesize);
	  return;
      }
      st->SetData(new_data, filesize);
  }
  
  st->SetValue("spatial_rank", "2dfov");
  
  /* Don't confuse propar's "type" with ddl's "storage" */
  st->SetValue("type", "absval");
  
  /* The following values are made up from thin air because
   * procpar has limited information about them
   *
   */
  
  st->CreateArray("span");
  st->SetValue("span", 10, 0);
  st->SetValue("span", 15, 1);
  
  st->CreateArray("origin");
  st->SetValue("origin", 5, 0);
  st->SetValue("origin", 7, 1);
  
  st->CreateArray("nucleus");
  st->SetValue("nucleus", "H1", 0);
  st->SetValue("nucleus", "H1", 1);
  
  st->CreateArray("nucfreq");
  st->SetValue("nucfreq", 200.067, 0);
  st->SetValue("nucfreq", 200.067, 1);
  
  st->CreateArray("location");
  st->SetValue("location", 0.0, 0);
  st->SetValue("location", 0.0, 1);
  st->SetValue("location", 0.0, 2);
  
  st->CreateArray("roi");
  st->SetValue("roi", 10.0, 0);
  st->SetValue("roi", 15.0, 1);
  st->SetValue("roi", 0.2, 2);
  
  st->CreateArray("orientation");
  st->SetValue("orientation", 1.0, 0);
  st->SetValue("orientation", 0.0, 1);
  st->SetValue("orientation", 0.0, 2);	
  st->SetValue("orientation", 0.0, 3);
  st->SetValue("orientation", 1.0, 4);
  st->SetValue("orientation", 0.0, 5);	
  st->SetValue("orientation", 0.0, 6);
  st->SetValue("orientation", 0.0, 7);
  st->SetValue("orientation", 1.0, 8);	
  
  /* End of fictional values */
}

int 
Imginfo::SaveImage(char *filename)
{
  if (st == NULL) {
    PERROR("Imginfo::SaveImage() called with invalid image!\n");
    return NOT_OK;
  }

  st->SaveSymbolsAndData(filename);
  return OK;
}

// Update the image scale factors
// See declaration of class Imginfo for explanation of xscale, xoffset, etc.
void
Imginfo::update_scale_factors()
{
    xscale = (pixwd - 1.0) / datawd;
    xoffset = pixstx - datastx * xscale + 0.5;
    yscale = (pixht - 1.0) / dataht;
    yoffset = pixsty - datasty * yscale + 0.5;
}
  

// Update data coords for everything in the display list
// See declaration of class Imginfo for explanation of xscale, xoffset, etc.
void
Imginfo::update_data_coordinates()
{
    RoitoolIterator element(display_list);
    Roitool *tool;

    update_scale_factors();

    while (element.NotEmpty()){
	tool = ++element;
	tool->update_data_coords();
    }
}

// Update data coords for one ROI tool
// See declaration of class Imginfo for explanation of xscale, xoffset, etc.
void
Imginfo::update_data_coordinates(Roitool *tool)
{
    update_scale_factors();
    tool->update_data_coords();
}

// Update screen coords for everything in the display list
void
Imginfo::update_screen_coordinates()
{
    RoitoolIterator element(display_list);
    Roitool *tool;

    update_scale_factors();

    while (element.NotEmpty()){
	tool = ++element;
	tool->update_screen_coords();
    }
}

// Update pixstx, etc. to fit current data (defined by datastx, etc.)
// into given rectangle on screen at maximum size.
// Note that the image will cover at least part of the given boundary.
Flag
Imginfo::update_image_position(int x1, int y1, int x2, int y2)
{
    int nx, ny, nz;
    double lenx, leny, lenz;
    if (   ! get_spatial_dimensions(&nx, &ny, &nz)
	|| ! get_spatial_spans(&lenx, &leny, &lenz))
    {
	msgerr_print("Can't get image size.");
	return FALSE;
    }
    if (nx == 0 || ny == 0){
	msgerr_print("Image has 0 width or height.");
	return FALSE;
    }
    double cm_wd = fabs(lenx) * datawd / nx;
    double cm_ht = fabs(leny) * dataht / ny;
    
    int stx, sty, wd, ht;
    get_image_position(x1, y1, x2, y2,
		       cm_wd, cm_ht,
		       &stx, &sty, &wd, &ht);
    pixstx = stx;
    pixsty = sty;
    pixwd = wd;
    pixht = ht;
    
    return TRUE;
}

// Update pixstx, etc. and datastx, etc. to fit the given rectangle
// in user coordinates (defined by the upper-left corner (usrx1, usry1)
// and lower-right corner (usrx2, usry2)), into given rectangle on screen.
// Note that the image may cover the given boundary.
Flag
Imginfo::update_image_position(int pixx1, int pixy1,
				    int pixx2, int pixy2,
				    double usrx1, double usry1,
				    double usrx2, double usry2)
{
    // Algorithm:
    //	Transform given user coordinates to data coordinates.
    //	Set datastx, datawd, etc, to include only what is inside the
    //		given rectangle.
    //	Calculate transformation that fits given data coordinates into
    //		available frame at maximum size.
    //	Map datastx, datawd, etc. to pixel coords with same transformation.

    //
    // Get required location values from symbol table
    //
    if ( ! st){
	msgerr_print("No FDF symbol table.");
	return FALSE;
    }
    
    // Location of center of data set in cm.
    double loc_x;
    if ( ! st->GetValue("location", loc_x, 0)){
	msgerr_print("\"location[0]\" not in FDF symbol table.");
	return FALSE;
    }
    double loc_y;
    if ( ! st->GetValue("location", loc_y, 1)){
	msgerr_print("\"location[1]\" not in FDF symbol table.");
	return FALSE;
    }
    
    // Span of data set in cm.
    double span_x, span_y, span_z;
    if ( ! get_spatial_spans(&span_x, &span_y, &span_z)){
	return FALSE;
    }
    if (span_x == 0){
	msgerr_print("update_image_position(): Data set has zero width");
	return FALSE;
    }
    if (span_y == 0){
	msgerr_print("update_image_position(): Data set has zero height");
	return FALSE;
    }

    // (X,Y) coordinates in cm. of first point in data set (user coords)
    double first_x = loc_x - span_x / 2.0;
    double first_y = loc_y - span_y / 2.0;
    
    // Size of data set in data points.
    int ndata_x, ndata_y, ndata_z;
    if ( ! get_spatial_dimensions(&ndata_x, &ndata_y, &ndata_z)){
	return FALSE;
    }

    //
    // Transform usrx1, etc. to data coordinates (datax1, etc.)
    // --Requested rectangle in data coords--
    //
    double datax1 = (usrx1 - first_x) * ndata_x / span_x;
    double datay1 = (usry1 - first_y) * ndata_y / span_y;
    double datax2 = (usrx2 - first_x) * ndata_x / span_x;
    double datay2 = (usry2 - first_y) * ndata_y / span_y;
    if (datax1 == datax2){
	msgerr_print("update_image_position(): Can't display 0 width image.");
	return FALSE;
    }
    if (datay1 == datay2){
	msgerr_print("update_image_position(): Can't display 0 height image.");
	return FALSE;
    }

    //
    // Set limits of data to display (this->datastx, etc.)
    // I.e., clip datax1, etc. to limits of actual data.
    // --Displayed data in data coords--
    //
    datastx = (int)(datax1 <= 0 ? 0 : (datax1 >= ndata_x ? ndata_x-1 : datax1));
    datasty = (int)(datay1 <= 0 ? 0 : (datay1 >= ndata_y ? ndata_y-1 : datay1));
    int lastdata = (int)(datax2 >= ndata_x ? ndata_x : datax2);
    datawd = lastdata <= datastx ? 1 : lastdata - datastx;
    lastdata = (int)(datay2 >= ndata_y ? ndata_y : datay2);
    dataht = lastdata <= datasty ? 1 : lastdata - datasty;

    //
    // Find out how the original given rectangle in user coordinates
    // (usrx1, etc.) will be mapped into the gframe.
    // --Requested rectangle in pixel (screen) coordinates--
    //
    int pix_ul_x, pix_ul_y, pix_width, pix_height;
    get_image_position(pixx1,  pixy1, pixx2, pixy2,
			usrx2 - usrx1, usry2 - usry1,
			&pix_ul_x, &pix_ul_y, &pix_width, &pix_height);

    //
    // Finally, calculate where our displayed data will go within this
    // rectangle.
    // --Displayed data in pixel coords--
    //
    pixstx = (short)(pix_ul_x +
		     pix_width * (datastx - datax1) / (datax2 - datax1));
    pixsty = (short)(pix_ul_y +
		     pix_height * (datasty - datay1) / (datay2 - datay1));
    pixwd = (short)(pix_width * datawd / (datax2 - datax1));
    pixht = (short)(pix_height * dataht / (datay2 - datay1));

    return TRUE;
}

// Given available display region (specified by corners pix_x1, etc.)
// and desired display dimensions in user units (specified by width and
// height in cm on the original object, cm_wide, cm_high),
// return upper-left corner and width and height (in pixels) of the
// displayed image.  Calculated to be as large as possible while
// fitting in the specified region.
void
Imginfo::get_image_position(int pix_x1, int pix_y1, int pix_x2, int pix_y2,
			    double cm_wide, double cm_high,
			    int *stx, int *sty, int *pix_wide, int *pix_high)
{
    //
    // Always put in upper-left-hand-corner of the frame
    //
    *stx = pix_x1;
    *sty = pix_y1;

    int fwd = pix_x2 - pix_x1 + 1;
    int fht = pix_y2 - pix_y1 + 1;

    //
    // Find the width and height of the displayed image.  Note that we have
    // to consider the physical shape of an image, and that we have
    // to fit the image inside the rectangle.
    // We compare physical dimensions of the original data region to
    // the size of the available display region in pixels.
    // The limiting dimension determines the scale factor for the other
    // dimension as well, and we can then calculate the image extent in
    // that dimension.
    //
    if (fabs(fwd / cm_wide) > fabs(fht / cm_high)){
	*pix_wide = (int)fabs(fht * cm_wide / cm_high);
	*pix_high = fht;
    }else{
	*pix_wide = fwd;
	*pix_high = (int)fabs(fwd * cm_high / cm_wide);
    }
}

// Get the spatial x, y, z dimensions of the data set; i.e., the number of
// data points in each spatial dimension.
Flag 
Imginfo::get_spatial_dimensions(int *nx, int *ny, int *nz)
{
    int i, j;
    if ( ! st){
	msgerr_print("get_spatial_dimensions(): No FDF symbol table.");
	return FALSE;
    }
    
    char *spatial_rank;
    if ( ! st->GetValue("spatial_rank", spatial_rank)
	 && ! st->GetValue("subrank", spatial_rank))
    {
	msgerr_print("get_spatial_dimensions(): No \"spatial_rank\" entry.");
	return FALSE;
    }

    int rank;
    if ( ! st->GetValue("rank", rank)){
	msgerr_print("get_spatial_dimensions(): No \"rank\" FDF entry.");
	return FALSE;
    }

    int sp_rank;	// Number of spatial dims in the data set.
    if (strcmp(spatial_rank, "1dfov") == 0){
	sp_rank = 1;
    }else if (strcmp(spatial_rank, "2dfov") == 0){
	sp_rank = 2;
    }else if (strcmp(spatial_rank, "3dfov") == 0){
	sp_rank = 3;
    }else if (   strcmp(spatial_rank, "voxel") == 0
	      || strcmp(spatial_rank, "none") == 0)
    {
	return FALSE;
    }else{
	MSGERR1("get_spatial_dimensions(): unknown spatial_rank type: \"%s\".",
		spatial_rank);
	return FALSE;
    }

    int ndims[3];
    char *abscissa;
    for (i=j=0; i<rank; i++){
	if ( ! st->GetValue("abscissa", abscissa, i)){
	    MSGERR1("get_spatial_dimensions(): No \"abscissa[%d]\" entry.",
		    i);
	    return FALSE;
	}
	if (strcmp(abscissa, "cm") == 0
	    || strcmp(abscissa, "spatial") == 0)
	{
	    if ( ! st->GetValue("matrix", ndims[j++], i)){
		MSGERR1("get_spatial_dimensions(): No \"matrix[%d]\" entry.",
			i);
		return FALSE;
	    }
	}
    }
    if (j == 0){
	MSGERR0("get_spatial_spans(): No spatial type abscissas.");
	return FALSE;
    }else if (j != sp_rank){
	MSGERR1("get_spatial_spans(): Only %d spatial type abscissas.", j);
	return FALSE;
    }

    for ( ; j<3; j++){
	ndims[j] = 1;
    }

    *nx = ndims[0];
    *ny = ndims[1];
    *nz = ndims[2];

    return TRUE;
}

// Get the spatial x, y, z size of the data set; i.e., the signed distance
// in centimeters from the "left" side of the first data point to the
// "right" side of the last data point.
// For non-degenerate directions, returns the signed value of "span";
// for directions that are not "arrayed", returns the unsigned "roi" value.
// Return Value: TRUE on success, FALSE on failure (if the symbol table
//	does not contain the necessary values).
Flag 
Imginfo::get_spatial_spans(double *dx, double *dy, double *dz)
{
    int i, j;
    if ( ! st){
	msgerr_print("get_spatial_spans(): No FDF symbol table.");
	return FALSE;
    }
    
    char *spatial_rank;
    if ( ! st->GetValue("spatial_rank", spatial_rank)
	 && ! st->GetValue("subrank", spatial_rank))
    {
	msgerr_print("get_spatial_spans(): No \"spatial_rank\" FDF entry.");
	return FALSE;
    }

    int rank;
    if ( ! st->GetValue("rank", rank)){
	msgerr_print("get_spatial_spans(): No \"rank\" FDF entry.");
	return FALSE;
    }

    int sp_rank;	// Number of spatial dims in the data set.
    if (strcmp(spatial_rank, "1dfov") == 0){
	sp_rank = 1;
    }else if (strcmp(spatial_rank, "2dfov") == 0){
	sp_rank = 2;
    }else if (strcmp(spatial_rank, "3dfov") == 0){
	sp_rank = 3;
    }else if (   strcmp(spatial_rank, "voxel") == 0
	      || strcmp(spatial_rank, "none") == 0)
    {
	return FALSE;
    }else{
	MSGERR1("get_spatial_spans(): unknown spatial_rank type: \"%s\".",
		spatial_rank);
	return FALSE;
    }

    double span[3];
    char *abscissa;
    for (i=j=0; i<rank; i++){
	if ( ! st->GetValue("abscissa", abscissa, i)){
	    MSGERR1("get_spatial_spans(): No \"abscissa[%d]\" entry.", i);
	    return FALSE;
	}
	if (strcmp(abscissa, "cm") == 0
	    || strcmp(abscissa, "spatial") == 0)
	{
	    if ( ! st->GetValue("span", span[j++], i)){
		MSGERR1("get_spatial_spans(): No \"span[%d]\" entry.", i);
		return FALSE;
	    }
	}
    }
    if (j == 0){
	MSGERR0("get_spatial_spans(): No spatial type abscissas.");
	return FALSE;
    }else if (j != sp_rank){
	MSGERR1("get_spatial_spans(): Only %d spatial type abscissas.", j);
	return FALSE;
    }

    for ( ; j<3; j++){
	if ( ! st->GetValue("roi", span[j], j)){
	    MSGERR1("get_spatial_spans(): No \"roi[%d]\" entry.", j);
	    return FALSE;
	}
    }

    *dx = span[0];
    *dy = span[1];
    *dz = span[2];
    
    return TRUE;
}


// Get the spatial x, y, z location of the data set; i.e., the signed distance
// in centimeters from the center of the magnet to the center of the data set.
// The coordinate system is the "user" coord system--i.e., aligned with the
// data slab.
// Return Value: TRUE on success, FALSE on failure (if the symbol table
//	does not contain the necessary values).
Flag 
Imginfo::get_location(double *x, double *y, double *z)
{
    if ( ! st){
	msgerr_print("get_location(): No FDF symbol table.");
	return FALSE;
    }

    if ( ! st->GetValue("location", *x, 0)){
	msgerr_print("get_location(): No \"location[0]\" FDF entry.");
	return FALSE;
    }
    if ( ! st->GetValue("location", *y, 1)){
	msgerr_print("get_location(): No \"location[1]\" FDF entry.");
	return FALSE;
    }
    if ( ! st->GetValue("location", *z, 2)){
	msgerr_print("get_location(): No \"location[2]\" FDF entry.");
	return FALSE;
    }
    return TRUE;
}

// Convert x-coord from data to screen frame
int
Imginfo::XDataToScreen(float xdata)
{
    // N.B. "xoffset" already includes a 0.5 offset for rounding
    return (int)(xdata * xscale + xoffset);
}

// Convert y-coord from data to screen frame
int
Imginfo::YDataToScreen(float ydata)
{
    // N.B. "yoffset" already includes a 0.5 offset for rounding
    return (int)(ydata * yscale + yoffset);
}

// Convert x-coord from screen frame to data
float
Imginfo::XScreenToData(int xp)
{
    float xd = (xp - xoffset) / xscale;
    if (xd < 0){
	xd = 0;
    }else if (xd > GetFast()){
	xd = GetFast();
    }
    return xd;
}

// Convert y-coord from screen frame to data
float
Imginfo::YScreenToData(int yp)
{
    float yd = (yp - yoffset) / yscale;
    if (yd < 0){
	yd = 0;
    }else if (yd > GetMedium()){
	yd = GetMedium();
    }
    return yd;
}

// Convert screen coords from this image to another
Gpoint
Imginfo::ScreenToScreen(Imginfo *to, int x, int y)
{
    Gpoint rtn;

    rtn.x = rtn.y = 0;
    if (to){
	rtn.x = to->XDataToScreen(XScreenToData(x));
	rtn.y = to->YDataToScreen(YScreenToData(y));
    }
    return rtn;
}

//
// Convert from position in the image (measured in pixel units from the
// first pixel in the data set) to position in the magnet reference frame
// (measured in cm from the center of the magnet with standard orientation).
//
D3Dpoint
Imginfo::pixel_to_magnet_frame(Fpoint pixel)
{
    int i, j, k;
    double dcos[3][3];		// Direction cosines
    double span[2];		// Dimensions of data slab
    double loc[3];		// Distance from magnet center to slab center
    				//    in slab coordinate frame orientation
    double dpixel[2];		// Same as "pixel", but in cm
    double dist[3];		// The answer in slab coordinate system.
    double magd[3];		// The answer as a vector.
    D3Dpoint ret;		// The answer.

    // Get some basic data
    for (i=0; i<3; i++){
	st->GetValue("location", loc[i], i);
	if (i < 2){
	    st->GetValue("span", span[i], i);
	}
    }
    for (i=0, j=0; j<3; j++){
	for (k=0; k<3; k++, i++){
	    st->GetValue("orientation", dcos[j][k], i);
	}
    }

    // Scale pixel address to cm
    dpixel[0] = pixel.x * GetRatioFast() / GetFast();
    dpixel[1] = pixel.y * GetRatioMedium() / GetMedium();

    // Add components of distance from magnet center to point
    dist[0] = loc[0] - 0.5 * span[0] + dpixel[0];
    dist[1] = loc[1] - 0.5 * span[1] + dpixel[1];
    dist[2] = loc[2];

    // Coordinate rotation to magnet frame
    for (i=0; i<3; i++){
	magd[i] = 0;
	for (j=0; j<3; j++){
	    magd[i] += dcos[j][i] * dist[j];
	}
    }
    ret.x = magd[0];
    ret.y = magd[1];
    ret.z = magd[2];
    return ret;
}

//
// Adjust the positional variables in the FDF header appropriately for
// a 90 deg (counterclockwise) rotation of the image.
// Also adjusts the zooming parameters in the imginfo header.
// Other imginfo header parameters (like pixwd) are not basic, and
// are assumed to be recalculated when the image is displayed.
//
Flag
Imginfo::rot90_header()
{
    int i;
    double location[3];
    int matrix[2];
    double orient[9];
    double origin[2];
    double roi[3];
    double span[2];
    double zx1;
    double zx2;

    GetOrientation(orient);
    get_location(&location[0], &location[1], &location[2]);
    span[0] = GetRatioFast();
    span[1] = GetRatioMedium();
    for (i=0; i<3; i++){
	st->GetValue("roi", roi[i], i);
    }
    st->GetValue("origin", origin[0], 0);
    st->GetValue("origin", origin[1], 1);
    matrix[0] = GetFast();
    matrix[1] = GetMedium();

    st->SetValue("matrix", matrix[1], 0);
    st->SetValue("matrix", matrix[0], 1);
    for (i=0; i<3; i++){
	st->SetValue("orientation", orient[i+3], i);
    }
    for (i=3; i<6; i++){
	st->SetValue("orientation", -orient[i-3], i);
    }
    st->SetValue("roi", roi[1], 0);
    st->SetValue("roi", roi[0], 1);
    st->SetValue("origin", origin[1], 0);
    st->SetValue("origin", -(span[0] + origin[0]), 1);
    st->SetValue("span", span[1], 0);
    st->SetValue("span", span[0], 1);
    st->SetValue("location", location[1], 0);
    st->SetValue("location", -location[0], 1);

    // Zoom line locations
    zx1 = zlinex1;
    zx2 = zlinex2;
    zlinex1 = zliney1;
    zlinex2 = zliney2;
    zliney2 = 1 - zx1;
    zliney1 = 1 - zx2;

    // Currently displayed region (from zooming)
    i = datastx;
    datastx = datasty;
    datasty = matrix[0] - i - datawd;
    i = datawd;
    datawd = dataht;
    dataht = i;

    // All the ROItools
    RoitoolIterator element(display_list);
    Roitool *tool;
    while (element.NotEmpty()){
	tool = ++element;
	tool->rot90_data_coords(matrix[0]);
    }

    return TRUE;
}

Flag
Imginfo::flip_header()
{
    int i;
    double location[3];
    int matrix[2];
    double orient[9];
    double origin[2];
    double span[2];

    GetOrientation(orient);
    get_location(&location[0], &location[1], &location[2]);
    span[0] = GetRatioFast();
    st->GetValue("origin", origin[0], 0);
    matrix[0] = GetFast();

    for (i=0; i<3; i++){
	st->SetValue("orientation", -orient[i], i);
    }
    st->SetValue("origin", -(span[0] + origin[0]), 0);
    st->SetValue("location", -location[0], 0);

    // Zoom line locations
    zlinex1 = 1 - zlinex2;
    zlinex2 = 1 - zlinex1;

    // Currently displayed region (from zooming)
    datastx = matrix[0] - datastx - datawd;

    // All the ROItools
    RoitoolIterator element(display_list);
    Roitool *tool;
    while (element.NotEmpty()){
	tool = ++element;
	tool->flip_data_coords(matrix[0]);
    }

    return TRUE;
}

//
// Transform a 3-vector to a rotated (but not translated) coordinate system.
// Used to transform from the user frame (defined by the "orientation" in
// the symbol table) centered on the magnet, to the magnet frame, or any
// specified orientation relative to the magnet frame.
// Starting orientation is as defined in my symbol table.
// Ending orientation is defined by the "new_orient" vector.
// Orientations are given in direction cosines, as described in the FDF
// specification.
// Returns TRUE on success.  Returns FALSE if the orientation is undefined
// in the symbol table, or if either the old or new orientation is invalid.
// Returns the new location in the same vector, "x".
//
Flag
Imginfo::rotate_coords(double *new_orient, double *x)
{
    const float tol = 0.0001;
    int i, j;

    double orientation[9];
    if ( ! GetOrientation(orientation) ){
	msgerr_print("rotate_coords(): \"orientation\" undefined in FDF header");
	return FALSE;
    }

    //
    // Check "orientation" and "new_orient" for consistency.
    //
    // Check length of unit vectors
    double length1, length2;
    for (i=0; i<3; i++){
	length1 = length2 = 0;
	for (j=0; j<3; j++){
	    length1 += orientation[i + 3*j] * orientation[i + 3*j];
	    length2 += orientation[j + 3*i] * orientation[j + 3*i];
	}
	if (fabs(1.0 - length1) > tol || fabs(1.0 - length2) > tol){
	    msgerr_print("rotate_coords(): Bad \"orientation\" in FDF header");
	    return FALSE;
	}
    }
    for (i=0; i<3; i++){
	length1 = length2 = 0;
	for (j=0; j<3; j++){
	    length1 += new_orient[i + 3*j] * new_orient[i + 3*j];
	    length2 += new_orient[j + 3*i] * new_orient[j + 3*i];
	}
	if (fabs(1.0 - length1) > tol || fabs(1.0 - length2) > tol){
	    msgerr_print("rotate_coords(): passed bad new orientation");
	    return FALSE;
	}
    }
    // Check orthoganality (NOT IMPLEMENTED)
	    

    // Rotate to magnet frame
    double xnew[3];
    for (i=0; i<3; i++){
	xnew[i] = 0;
	for (j=0; j<3; j++){
	    xnew[i] += orientation[i + 3*j] * x[j];
	}
    }

    // Rotate to new frame (stored back in original vector)
    for (i=0; i<3; i++){
	x[i] = 0;
	for (j=0; j<3; j++){
	    x[i] += new_orient[j + 3*i] * xnew[j];
	}
    }
    return TRUE;
}

//
// Get data location in magnet (user coordinates).
// The bounding box of the displayed data is returned.  The displayed
// data is defined by datastx, etc.
// Positions are in user coordinates, but relative to the center of the magnet.
// For coords that have 
// Returns TRUE on success.  Returns FALSE if the necessary span or location
// information is unavailable.
//
Flag
Imginfo::get_user_coords_of_displayed_data(double *x0, double *y0, double *z0,
					   double *x1, double *y1, double *z1)
{
    int nx, ny, nz;
    double x, y, z;			// Data slab location
    double xspan, yspan, zspan;		// Data slab size

    if (   ! get_spatial_spans(&xspan, &yspan, &zspan)
	|| ! get_location(&x, &y, &z)
	|| ! get_spatial_dimensions(&nx, &ny, &nz) )
    {
	return FALSE;
    }
    *x0 = x - xspan/2 + xspan * datastx / nx;
    *y0 = y - yspan/2 + yspan * datasty / ny;
    *z0 = z - zspan/2;

    *x1 = *x0 + xspan * datawd / nx;
    *y1 = *y0 + yspan * dataht / ny;
    *z1 = *z0 + zspan;

    return TRUE;
}

/************************************************************************
*                                                                       *
*  Calculate a new vertical scale.					*
*  Algorithm is taken from Vnmr "donci_get_new_vs".			*
*  Return new calculated vs value.					*
*  (STATIC)								*
*									*/
float
Imginfo::calculate_new_image_vs(int x, int y, int vs_band)
{
   int first_trace, num_traces;	// first trace and number of traces
   int first_pnt, num_pnts;	// first point and number of points
   int trace;			// current trace for loop counter
   register int pnt;		// current point for loop coubnter
   float data_value;		// data value
   float newvs;			// new vertcal scale value

   if ( ! com_point_in_rect(x, y, pixstx, pixsty, pixstx+pixwd, pixsty+pixht)){
       return 0;
   }

   x -= vs_band / 2;
   y -= vs_band / 2;

   // Calculate the first pixel data, number of data, first trace, and
   // number of traces.  Also, check for thier limit values
   first_pnt = datastx + datawd * (x-pixstx)/pixwd;
   first_trace = datasty + dataht * (y-pixsty)/pixht;
   num_pnts = datawd * vs_band / pixwd;
   num_traces = dataht * vs_band / pixht;

   if (num_pnts < 1)
      num_pnts = 1;
   else if (num_pnts > datawd)
      num_pnts = datawd;

   if (first_pnt < datastx)
      first_pnt = datastx;
   if (first_pnt > (datastx + datawd - num_pnts))
      first_pnt = datastx + datawd - num_pnts;

   if (num_traces < 1)
      num_traces = 1;
   else if (num_traces > dataht)
      num_traces = dataht;

   if (first_trace < datasty)
      first_trace = datasty;
   if (first_trace > (datasty + dataht - num_traces))
      first_trace = datasty + dataht - num_traces;

   // Find the maximum value within the bandwidth
   switch (type)
   {
      case TYPE_FLOAT:
      {
         register float *pdata;	// pointer to the data
         register float max_value = 0.0;
         for (trace=first_trace; trace<(first_trace+num_traces); trace++)
         {
            pdata = (float *)GetData() + 
	            trace * GetFast();
            for (pnt=first_pnt, pdata += pnt; pnt<(first_pnt+num_pnts); 
	         pnt++, pdata++)
            {
	       if (*pdata > max_value)
	          max_value = *pdata;
            }
	 }
	 data_value = max_value;
      }
	 break;
	 
      case TYPE_SHORT:
      {
         register short *pdata;	// pointer to the data
	 register short max_value=0;
         for (trace=first_trace; trace<(first_trace+num_traces); trace++)
         {
            pdata = (short *)GetData() + 
	            trace * GetFast();
            for (pnt=first_pnt, pdata += pnt; pnt<(first_pnt+num_pnts); 
	         pnt++, pdata++)
            {
	       if (*pdata > max_value)
	          max_value = *pdata;
            }
	 }
	 data_value = (float)max_value;
      }
	 break;

      default:
	 msgerr_print("Vs:This data TYPE (%d) is not supported yet",
	       type);
	 newvs = vs;
	 break;
   }

   // Note that it only works for GRAY SCALE image
   /*if (cmsindex == SISCMS_2)
   {
      if ((newvs = G_Get_Sizecms2(Gframe::gdev) / data_value) < 1e-8)
         newvs = 1e-8;
      else if (newvs > 1e8)
         newvs = 1e8;
   }
   else
   {
     msgerr_print("VS only supports gray-scale. Vs is not calculated"); 
     newvs = vs;
   }

   return(newvs);*/
   return data_value;
}

/************************************************************************
*                                                                       *
*  Calculate a new vertical scale.					*
*  Return new calculated vs value.					*
*  Returns 0.0 on error.
*									*/
float
Imginfo::calculate_new_image_vs(float x, // X coord of ref patch (data pixels)
				float y, // Y coord of ref patch (data pixels)
				int vs_band) // Width of ref patch (scrn pixels)
{
   int first_trace, num_traces;	// first trace and number of traces
   int first_pnt, num_pnts;	// first point and number of points
   int trace;			// current trace for loop counter
   register int pnt;		// current point for loop coubnter
   float data_value;		// data value
   float newvs;			// new vertcal scale value

   int nx = GetFast();
   int ny = GetMedium();
   if (x > nx || y > ny){
       return 0;
   }

   num_pnts = (int)fabs(XScreenToData(vs_band) - XScreenToData(0));
   num_traces = (int)fabs(YScreenToData(vs_band) - YScreenToData(0));

   // Calculate the first pixel data, number of data, first trace, and
   // number of traces.  Also, check for their limit values
   first_pnt = (int)(x+0.5) - num_pnts / 2;
   first_trace = (int)(y+0.5) - num_traces / 2;

   if (num_pnts < 1)
      num_pnts = 1;
   else if (num_pnts > nx)
      num_pnts = nx;

   if (first_pnt < 0)
      first_pnt = 0;
   if (first_pnt > (nx - num_pnts))
      first_pnt = nx - num_pnts;

   if (num_traces < 1)
      num_traces = 1;
   else if (num_traces > ny)
      num_traces = ny;

   if (first_trace < 0)
      first_trace = 0;
   if (first_trace > (ny - num_traces))
      first_trace = ny - num_traces;

   // Find the maximum value within the bandwidth
   switch (type)
   {
      case TYPE_FLOAT:
      {
         register float *pdata;	// pointer to the data
         register float max_value = 0.0;
         for (trace=first_trace; trace<(first_trace+num_traces); trace++)
         {
            pdata = (float *)GetData() + 
	            trace * GetFast();
            for (pnt=first_pnt, pdata += pnt; pnt<(first_pnt+num_pnts); 
	         pnt++, pdata++)
            {
	       if (*pdata > max_value)
	          max_value = *pdata;
            }
	 }
	 data_value = max_value;
      }
	 break;
	 
      case TYPE_SHORT:
      {
         register short *pdata;	// pointer to the data
	 register short max_value=0;
         for (trace=first_trace; trace<(first_trace+num_traces); trace++)
         {
            pdata = (short *)GetData() + 
	            trace * GetFast();
            for (pnt=first_pnt, pdata += pnt; pnt<(first_pnt+num_pnts); 
	         pnt++, pdata++)
            {
	       if (*pdata > max_value)
	          max_value = *pdata;
            }
	 }
	 data_value = (float)max_value;
      }
	 break;

      default:
	 msgerr_print("Vs:This data TYPE (%d) is not supported yet",
	       type);
	 newvs = vs;
	 break;
   }

   // Note that it only works for GRAY SCALE image
   /*if (cmsindex == SISCMS_2)
   {
      if ((newvs = G_Get_Sizecms2(Gframe::gdev) / data_value) < 1e-8)
         newvs = 1e-8;
      else if (newvs > 1e8)
         newvs = 1e8;
   }
   else
   {
     msgerr_print("VS only supports gray-scale. Vs is not calculated"); 
     newvs = vs;
   }

   return(newvs);*/
   return data_value;
}

//
// Auto scale intensity for this image
//
void
Imginfo::AutoVscale()
{
    int i;
    int n;
    double min;
    double max;

    get_minmax(&min, &max);
    if (max == min){
	return;
    }
    int nbins = 1000;
    int npts = GetFast() * GetMedium();
    float *data = (float *)GetData();
    float *end = data + npts;
    int *histogram = new int[nbins];
    for (i=0; i<nbins; i++){
	histogram[i] = 0;
    }
    float scale  = (nbins - 1) / (max - min);
    float *p;
    for (p=data; p<end; p++){
	i = (int)((*p - min) * scale);
	histogram[i]++;
    }

    int percentile = 0.01 * npts;
    for (i=n=0; n<percentile && i<nbins; n += histogram[i++]);
    vsfunc->min_data = min + (i-1) / scale;
    for (i=nbins, n=0; n<percentile && i>0; n += histogram[--i]);
    vsfunc->max_data = min + i / scale;
    if (vsfunc->min_data > 0 && vsfunc->min_data / vsfunc->max_data < 0.05){
	vsfunc->min_data = 0;
    }
    set_vscale(vsfunc->max_data, vsfunc->min_data);
    delete [] histogram;
}   

//
// Set new vs value in this image
// Returns TRUE if new value is different, FALSE if it is the same
//
Flag
Imginfo::set_new_image_vs(float new_vs)
{
    if (new_vs == vsfunc->max_data){
	return FALSE;
    }else{
	vsfunc->max_data = new_vs;
	return TRUE;
    }
}

//
// Set new vs value in this image
// Returns TRUE if new value is different, FALSE if it is the same
//
Flag
Imginfo::set_new_image_vs(float max_data, float min_data, Flag set_vstool)
{
    if (max_data == 1.0 && min_data == 0.0 ){
	return FALSE;
    }else{
	float omin = vsfunc->min_data;
	float omax = vsfunc->max_data;
	vsfunc->max_data = omin + max_data * (omax - omin);
	vsfunc->min_data = omin + min_data * (omax - omin);
	if (set_vstool){
	    set_vscale(vsfunc->max_data, vsfunc->min_data);
	}
	return TRUE;
    }
}

//
// Set new vertical scale function in this image
// Returns TRUE if new value is different, FALSE if it is the same
//
Flag
Imginfo::set_new_image_vs(VsFunc *newfunc)
{
    if (vsfunc->isequal(newfunc)){
	return FALSE;
    }else{
	delete vsfunc;
	vsfunc = new VsFunc(newfunc);
	return TRUE;
    }
}

//
// Get min and max values in image
//
void
Imginfo::get_minmax(double *min, double *max)
{
    register float *data = (float *)GetData();
    register float *end = data + GetFast() * GetMedium();
    
    register float fmin, fmax;
    for (fmin=fmax=*data++; data<end; data++){
	if (*data > fmax){
	    fmax = *data;
	}else if (*data < fmin){
	    fmin = *data;
	}
	
    }

    *min = fmin;
    *max = fmax;
}

// Clip a point's coordinates to keep it inside the image
void
Imginfo::keep_point_in_image(short *x, short *y)
{
    int left = XDataToScreen(0);
    int right = XDataToScreen(GetFast());
    int top = YDataToScreen(0);
    int bottom = YDataToScreen(GetMedium());

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

/************************************************************************
*
*  Refresh a specified rectangular area of the current image.
*  (xmin, ymin) and (xmax, ymax) define the rectangle to refresh
*  relative to the upper-left corner of the canvas (not the image).
*
*/
void
Imginfo::redisplay_data_in_rect(int xmin, int ymin, int xmax, int ymax)
{
    Gdev *gdev = Gframe::gdev;

    if (pixmap && !pixmap_ood &&
	pixwd == pixmap_fmt.pixwd &&
	pixht == pixmap_fmt.pixht &&
	datastx == pixmap_fmt.datastx &&
	datasty == pixmap_fmt.datasty &&
	datawd == pixmap_fmt.datawd &&
	dataht == pixmap_fmt.dataht &&
	vs == pixmap_fmt.vs)
    {
	// We can refresh from our pixmap
	// Set drawing op to "copy", remembering current op
	int prev_op = G_Get_Op(gdev);
	G_Set_Op(gdev, GXcopy);
	XCopyArea(gdev->xdpy, pixmap,
		  gdev->xid, gdev->xgc,
		  xmin-pixstx, ymin-pixsty,
		  xmax-xmin+1, ymax-ymin+1,
		  xmin, ymin);
	
	/* restore the original X draw operation */
	G_Set_Op(gdev, prev_op);
    }else{
	msgerr_print("redisplay_data_in_rect(): No pixmap available.");
    }
}

/************************************************************************
*									*
*  Display this image in its current position.
*/
void
Imginfo::display_data(Gframe *gframe)
{
    Gdev *gdev = Gframe::gdev;

    gframe->set_clip_region(FRAME_NO_CLIP);

    int rescale = FALSE;
    if ( ! (pixmap_ood == FALSE &&
	    pixwd == pixmap_fmt.pixwd &&
	    pixht == pixmap_fmt.pixht &&
	    datastx == pixmap_fmt.datastx &&
	    datasty == pixmap_fmt.datasty &&
	    datawd == pixmap_fmt.datawd &&
	    dataht == pixmap_fmt.dataht))
    {
	rescale = TRUE;
    }
    if (pixmap &&
	!rescale &&
	vsfunc->isequal(pixmap_fmt.vsfunc)
	/*vs == pixmap_fmt.vs*/
	)
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

	//cout << endl;
	//cout << "ratio_fast = " << ratio_fast << endl;
	//cout << "ratio_medium = " << ratio_medium << endl;
	//cout << "fast = " << fast << endl;
	//cout << "medium = " << medium << endl;
	//cout << "src_wd = " << src_wd << endl;
	//cout << "src_ht = " << src_ht << endl;
	//cout << "fwd = " << fwd << endl;
	//cout << "fht = " << fht << endl;
	//cout << endl;
	//cout << "pixwd = " << pixwd << endl;
	//cout << "pixht = " << pixht << endl;
	
	// Display image
	
	switch (type) {
	  case TYPE_FLOAT:
	    pixmap =
	    g_display_image(gdev, cmsindex, 
			    (float *)GetData(), 
			    GetFast(), GetMedium(), 
			    datastx, datasty, 
			    datawd, dataht, 
			    (int)pixstx, (int)pixsty, 
			    (int)pixwd, (int)pixht,
			    TOP,	// Orientation: 1st point at top
			    vsfunc,
			    TRUE);	// Return a pixmap
	    
	    // Remember which display has the pixmap and what its parms are.
	    display = gdev->xdpy;
	    pixmap_fmt.pixwd = pixwd;
	    pixmap_fmt.pixht = pixht;
	    pixmap_fmt.datastx = datastx;
	    pixmap_fmt.datasty = datasty;
	    pixmap_fmt.datawd = datawd;
	    pixmap_fmt.dataht = dataht;
	    pixmap_fmt.vs = vs;
	    delete pixmap_fmt.vsfunc;
	    pixmap_fmt.vsfunc = new VsFunc(vsfunc);
	    pixmap_ood = FALSE;
	    break;
	    
	  default:
	    msgerr_print("Frame_data: This data TYPE (%d) is not supported",
			 type);
	    break;
	}

	(void)set_cursor_shape(cursor);
	FlushGraphics();
    }

    // Now display the ROIs/labels
    RoitoolIterator element(display_list);
    Roitool *tool;
    Gmode mode;

    update_scale_factors();	// Init xscale, xoffset, etc.

    if (rescale){
	update_screen_coordinates();	// Rescale ROI displays to new image
    }

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
*  Display this image in its current position.
*/
void
Imginfo::overlay_data(Gframe *gframe)
{
    Imginfo *baseimg = gframe->imginfo;
    Gdev *gdev = Gframe::gdev;

    gframe->set_clip_region(FRAME_NO_CLIP);

    int rescale = FALSE;
    if ( ! (pixmap_ood == FALSE &&
	    pixwd == pixmap_fmt.pixwd &&
	    pixht == pixmap_fmt.pixht &&
	    datastx == pixmap_fmt.datastx &&
	    datasty == pixmap_fmt.datasty &&
	    datawd == pixmap_fmt.datawd &&
	    dataht == pixmap_fmt.dataht))
    {
	rescale = TRUE;
    }
    if (pixmap &&
	!rescale &&
	vsfunc->isequal(pixmap_fmt.vsfunc)
	/*vs == pixmap_fmt.vs*/
	)
    {
	// We can just put up the old image again.
	/* store the current type of draw operation */
	int prev_op = G_Get_Op(gdev);
	
	if (baseimg->disp_type == IMAGE)
	{
	   /* Only overlay areas that are not zero, so first clear */
	   /* areas to be overlaid. 				   */
	   G_Set_Op(gdev, GXandInverted);
	   XCopyArea(gdev->xdpy, pixmap,
		  gdev->xid, gdev->xgc,
		  0, 0,
		  pixwd, pixht,
		  pixstx, pixsty);

	   /* OR in overlaid data */
	   G_Set_Op(gdev, GXor);
	   XCopyArea(gdev->xdpy, pixmap,
		  gdev->xid, gdev->xgc,
		  0, 0,
		  pixwd, pixht,
		  pixstx, pixsty);
	}
	else {
	   /* set the type of draw operation */
	   G_Set_Op(gdev, GXcopy);
	
	   XCopyArea(gdev->xdpy, pixmap,
		  gdev->xid, gdev->xgc,
		  0, 0,
		  pixwd, pixht,
		  pixstx, pixsty);
	}
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

	//cout << endl;
	//cout << "ratio_fast = " << ratio_fast << endl;
	//cout << "ratio_medium = " << ratio_medium << endl;
	//cout << "fast = " << fast << endl;
	//cout << "medium = " << medium << endl;
	//cout << "src_wd = " << src_wd << endl;
	//cout << "src_ht = " << src_ht << endl;
	//cout << "fwd = " << fwd << endl;
	//cout << "fht = " << fht << endl;
	//cout << endl;
	//cout << "pixwd = " << pixwd << endl;
	//cout << "pixht = " << pixht << endl;
	
	// Display image
	
	switch (type) {
	  case TYPE_FLOAT:
	    pixmap =
	    g_display_image(gdev, cmsindex, 
			    (float *)GetData(), 
			    GetFast(), GetMedium(), 
			    datastx, datasty, 
			    datawd, dataht, 
			    (int)pixstx, (int)pixsty, 
			    (int)pixwd, (int)pixht,
			    TOP,	// Orientation: 1st point at top
			    vsfunc,
			    TRUE);	// Return a pixmap
	    
	    // Remember which display has the pixmap and what its parms are.
	    display = gdev->xdpy;
	    pixmap_fmt.pixwd = pixwd;
	    pixmap_fmt.pixht = pixht;
	    pixmap_fmt.datastx = datastx;
	    pixmap_fmt.datasty = datasty;
	    pixmap_fmt.datawd = datawd;
	    pixmap_fmt.dataht = dataht;
	    pixmap_fmt.vs = vs;
	    delete pixmap_fmt.vsfunc;
	    pixmap_fmt.vsfunc = new VsFunc(vsfunc);
	    pixmap_ood = FALSE;
	    break;
	    
	  default:
	    msgerr_print("Frame_data: This data TYPE (%d) is not supported",
			 type);
	    break;
	}

	(void)set_cursor_shape(cursor);
	FlushGraphics();
    }

    // Now display the ROIs/labels
    RoitoolIterator element(display_list);
    Roitool *tool;
    Gmode mode;

    update_scale_factors();	// Init xscale, xoffset, etc.

    if (rescale){
	update_screen_coordinates();	// Rescale ROI displays to new image
    }

    while (element.NotEmpty()){
	tool = ++element;
	mode = tool->setcopy();
	tool->draw();
	tool->setGmode(mode);
    }
    display_ood = FALSE;	// Mark display updated
}

/****************************************************************/
/*	Zoom Routines						*/
/****************************************************************/

void
Imginfo::quickzoom(Gframe *gframe, int x, int y, float factor)
{
    int min_datels_disp = 5;
    float xdata = XScreenToData(x);
    float ydata = YScreenToData(y);
    // Adjust the scale
    int dwd = datawd / factor;
    int dht = dataht / factor;
    if (dwd == datawd){
	if (factor > 1) dwd--;
	if (factor < 1) dwd++;
    }
    if (dht == dataht){
	if (factor > 1) dht--;
	if (factor < 1) dht++;
    }
    if (dwd < min_datels_disp){
	dwd = min_datels_disp;
    }
    if (dht < min_datels_disp){
	dht = min_datels_disp;
    }

    // Fill the frame
    float wdcm = dwd * GetRatioFast() / GetFast();
    float htcm = dht * GetRatioMedium() / GetMedium();
    int framewd = gframe->max_x() - gframe->min_x() + 1;
    int frameht = gframe->max_y() - gframe->min_y() + 1;
    if (wdcm / framewd < htcm / frameht){
	wdcm = framewd * htcm / frameht;
    }else if (htcm / frameht < wdcm / framewd){
	htcm = frameht * wdcm / framewd;
    }
    dwd = wdcm * GetFast() / GetRatioFast();
    dht = htcm * GetMedium() / GetRatioMedium();
    if (dwd > GetFast()){
	dwd = GetFast();
    }
    if (dht > GetMedium()){
	dht = GetMedium();
    }

    // Position the window on the data
    int stx = xdata - dwd / 2;
    int sty = ydata - dht / 2;
    if (stx < 0){
	stx = 0;
    }else if (stx + dwd > GetFast()){
	stx = GetFast() - dwd;
    }
    if (sty  < 0){
	sty = 0;
    }else if (sty + dht > GetMedium()){
	sty = GetMedium() - dht;
    }

    if (dwd == datawd && dht == dataht && stx == datastx && sty == datasty){
	return;
    }

    datastx = stx;
    datasty = sty;
    datawd = dwd;
    dataht = dht;
    gframe->update_image_position();
    update_screen_coordinates();
    zlinex1 = zliney1 = 0.0;
    zlinex2 = zliney2 = 1.0;
		
    Frame_data::display_data(gframe,
			     datastx, datasty,
			     datawd, dataht,
			     vs);
		
    Movie_frame *mhead;
    if (mhead = in_a_movie(gframe->imginfo)){
	// Set displayed region for all images in movie.
	Movie_frame *mframe = mhead->nextframe;
	do{
	    mframe->img->datastx = stx;
	    mframe->img->datasty = sty;
	    mframe->img->datawd = dwd;
	    mframe->img->dataht = dht;
	    mframe = mframe->nextframe;
	} while (mframe != mhead->nextframe);
    }
    warp_pointer(XDataToScreen(xdata), YDataToScreen(ydata));
}

Gframe *
Imginfo::zoom_full(Gframe *gframe)
{
   Gframe *big_frame;	// current frame pointer
        // if user selected pixel interpolation, turn smoothing on
        //if (smooth_zooms) smooth();

         if ((datawd <= 0) || (dataht <= 0))
         {
            msgerr_print(
	       "Cannot zoom image with width=0 or height=0. Process stops.");
            return ((Gframe *) 0);
         }
	    
         big_frame = Gframe::big_gframe(); 
         big_frame->imginfo = gframe->imginfo ;
         Frame_data::display_data(big_frame, datastx,
                                             datasty,
                                             datawd,
                                             dataht, vs);

        //dont_smooth();   // turn smoothing off
	return(big_frame);
}

void 
Imginfo::zoom(Gframe *gframe)
{
 int stx, sty;	// data starting point
 int wd, ht;	// data width and height
		
		// if user selected pixel interpolation, turn smoothing on
		//if (smooth_zooms) smooth();
		
		// Find the current data starting point and its width, based on
		// the zoom-line factor values
		stx = (datastx
		       + (int)(datawd * zlinex1));
		sty = (datasty
		       + (int)(dataht * zliney1));
		wd = (int)((zlinex2 - zlinex1)
			   * datawd);
		ht = (int)((zliney2 - zliney1)
			   * dataht);
		
		if ((wd <= 0) || (ht <= 0)){
		    msgerr_print("Cannot zoom in on %d x %d area", wd, ht);
		    return;
		}

		datastx = stx;
		datasty = sty;
		datawd = wd;
		dataht = ht;
		gframe->update_image_position();
		update_screen_coordinates();
		zlinex1 = zliney1 = 0.0;
		zlinex2 = zliney2 = 1.0;
		
		Frame_data::display_data(gframe,
					 datastx, datasty,
					 datawd, dataht,
					 vs);
		
		Movie_frame *mhead;
		if (mhead = in_a_movie(gframe->imginfo)){
		    // Set displayed region for all images in movie.
		    Movie_frame *mframe = mhead->nextframe;
		    do{
			mframe->img->datastx = stx;
			mframe->img->datasty = sty;
			mframe->img->datawd = wd;
			mframe->img->dataht = ht;
			mframe = mframe->nextframe;
		    } while (mframe != mhead->nextframe);
		}
		
		// Position the zoom-lines at the image boundary
		//draw_zoom_lines(gframe->imginfo);	// Clear zoom-lines
		//zlinex1 = zliney1 = 0.0;
		//zlinex2 = zliney2 = 1.0;
		//draw_zoom_lines(gframe->imginfo);	// Draw new zoom-lines
		
		//dont_smooth();   // turn smoothing off
}

void 
Imginfo::unzoom(Gframe *gframe)
{
 //float zx1, zy1, zx2, zy2;	// zoomed factor
 int fast = 0, medium = 0;
		
		// Calculate the zoomed factor with respect to original
		// image width and height.
		if (st == NULL) {
		    PERROR("Cannot get image DDL table!\n");
		    return;
		}
		
		st->GetValue("matrix", fast, 0);
		st->GetValue("matrix", medium, 1);
		
		if (fast == 0 || medium == 0) {
		    PERROR("Cannot get image dimensions!\n");
		    return;
		}
		
		// Position zoom lines to be ready to re-zoom
		zlinex1 = (float)datastx / fast;
		zliney1 = (float)datasty / medium;
		zlinex2 = ((float)(datastx
					    + datawd)
				    / (float) fast);
		zliney2 = ((float)(datasty
					    + dataht)
				    / (float) medium);
		
		// Set display to full image
		datastx = 0;
		datasty = 0;
		datawd = fast;
		dataht = medium;
		gframe->update_image_position();
		update_screen_coordinates();
		
		// Display the original data width and height
		Frame_data::display_data(gframe,
					 datastx, datasty,
					 datawd, dataht,
					 vs);
		
		Movie_frame *mhead;
		if (mhead = in_a_movie(gframe->imginfo)){
		    // Set displayed region for all images in movie.
		    Movie_frame *mframe = mhead->nextframe;
		    do{
			mframe->img->datastx = 0;
			mframe->img->datasty = 0;
			mframe->img->datawd = fast;
			mframe->img->dataht = medium;
			mframe = mframe->nextframe;
		    } while (mframe != mhead->nextframe);
		}
}

/************************************************************************
*                                                                       *
*  Return TRUE if any zoom-line of a frame is selected.  Maximum	*
*  selected zoomed-lines is two at a time and should not be in the same	*
*  direction.  For example, if the left zoom-line is selected, the right*
*  zoom-line should not be selected.					*
*  (Dynamic function)							*
*									*/
int
Imginfo::select_zoom_lines(int x, int y, int aperture, int zline)
{
   if ((int) fabs((double) ((int)(pixstx + zlinex1 * pixwd) - x)) < aperture){
      zline |= ZLINE_LEFT;
   }

   if ((int) fabs((double) ((int)(pixstx + zlinex2 * pixwd) - x)) < aperture){
      if (zline & ZLINE_LEFT){
         if ((int)(pixstx + zlinex2 * pixwd) < x){
           zline &= ~ZLINE_LEFT;
           zline |= ZLINE_RIGHT;
         }
     }else{
         zline |= ZLINE_RIGHT;
     }
   }

   if ((int) fabs((double) ((int)(pixsty + zliney1 * pixht) - y)) < aperture){
      zline |= ZLINE_TOP;
   }

   if ((int) fabs((double) ((int)(pixsty + zliney2 * pixht) - y)) < aperture){
      if (zline & ZLINE_TOP){
	  if ((int)(pixsty + zliney2 * pixht) < y){
	      zline &= ~ZLINE_TOP;
	      zline |= ZLINE_BOTTOM;
	  }
      }else{
	  zline |= ZLINE_BOTTOM;
      }
   }
   
   return (zline);
}

/************************************************************************
*                                                                       *
*  Move selected zoom-lines within an image.				*
*  (Dynamic function)							*
*									*/
void
Imginfo::move_zlines(short dx, short dy, // Requested move
		     float fleft, float fright, // Initial zoom line locations
		     float ftop, float fbottom,
		     int color,
		     int zline)	// Which lines to move
{
    short left = (short)(fleft * (pixwd - 1));
    short right = (short)(fright * (pixwd - 1));
    short top = (short)(ftop * (pixht - 1));
    short bottom = (short)(fbottom * (pixht - 1));

    if (zline & ZLINE_ALL){
	float zl;
	float zr;
	float zt;
	float zb;
	if (left + dx < 0){
	    dx = -left;
	}else if (right + dx >= pixwd){
	    dx = pixwd - 1 - right;
	}
	if (top + dy < 0){
	    dy = -top;
	}else if (bottom + dy >= pixht){
	    dy = pixht - 1 - bottom;
	}
	zl = (float)(left + dx) / (pixwd - 1);
	zr = (float)(right + dx) / (pixwd - 1);
	zt = (float)(top + dy) / (pixht - 1);
	zb = (float)(bottom + dy) / (pixht - 1);

	if (zl != zlinex1 || zr != zlinex2 || zt != zliney1 || zb != zliney2){
	    draw_zoom_lines(color);
	    zlinex1 = zl;
	    zlinex2 = zr;
	    zliney1 = zt;
	    zliney2 = zb;
	    draw_zoom_lines(color);
	}
    }else{
	float zoom_result;	// temporary result

	if (zline & ZLINE_LEFT){
	    zoom_result = (float)(left + dx) / (pixwd - 1);
	    if (zoom_result < 0.0){
		zoom_result = 0.0;
	    }else if (zoom_result > zlinex2){
		zoom_result = zlinex2;
	    }
	    draw_zlinex1(color);
	    zlinex1 = zoom_result;
	    draw_zlinex1(color);
	}else if (zline & ZLINE_RIGHT){
	    zoom_result =  (float)(right + dx) / (pixwd - 1);
	    if (zoom_result < zlinex1){
		zoom_result = zlinex1;
	    }else if (zoom_result > 1.0 ){
		zoom_result = 1.0;
	    }
	    draw_zlinex2(color);
	    zlinex2 = zoom_result;
	    draw_zlinex2(color);
	}

	if (zline & ZLINE_TOP){
	    zoom_result =  (float)(top + dy) / (pixht - 1);
	    if (zoom_result < 0.0){
		zoom_result = 0.0;
	    }else if (zoom_result > zliney2){
		zoom_result = zliney2;
	    }
	    draw_zliney1(color);
	    zliney1 = zoom_result;
	    draw_zliney1(color);
	}else if (zline & ZLINE_BOTTOM){
	    zoom_result = (float)(bottom + dy) / (pixht - 1);
	    if (zoom_result < zliney1){
		zoom_result = zliney1;
	    }else if (zoom_result > 1.0 ){
		zoom_result = 1.0;
	    }
	    draw_zliney2(color);
	    zliney2 = zoom_result;
	    draw_zliney2(color);
	}
    }
}

/************************************************************************
*                                                                       *
*  Replace zoom-lines for a specific frame.				*
*  (Dynamic function)							*
*									*/
void
Imginfo::replace_zlines(float linex1, float linex2, float liney1, float liney2,
								int color)
{
   if (zlinex1 != linex1)  {
       draw_zlinex1(color);	// Erase
       zlinex1 = linex1;
       draw_zlinex1(color);	// Draw
   }
   if (zliney1 != liney1)  {
       draw_zliney1(color);	// Erase
       zliney1 = liney1;
       draw_zliney1(color);	// Draw
   }
   if (zlinex2 != linex2)  {
       draw_zlinex2(color);	// Erase
       zlinex2 = linex2;
       draw_zlinex2(color);	// Draw
   }
   if (zliney2 != liney2) {
       draw_zliney2(color);	// Erase
       zliney2 = liney2;
       draw_zliney2(color);	// Draw
   }

}

/************************************************************************
*                                                                       *
*  Draw zoom-lines for a specific frame.				*
*  Programming Note: this function is also called by "Gframe::mark".	*
*  (Dynamic function)							*
*									*/
void
Imginfo::draw_zoom_lines(int color)
{
   draw_zlinex1(color);
   draw_zliney1(color);
   draw_zlinex2(color);
   draw_zliney2(color);
}

/************************************************************************
*                                                                       *
*  Draw a left zoom-line.						*
*  Note that it also prevents overlap with right zoom-line.		*
*  (Dynamic function)							*
*									*/
void
Imginfo::draw_zlinex1(int color)
{
   int left = (int)(zlinex1 * pixwd);
   int right = (int)(zlinex2 * pixwd);

   if (left == right)
      left--;

   int 	linewidth = G_Get_LineWidth(Gframe::gdev);
   G_Set_LineWidth(Gframe::gdev, 2);
   int prev_op = G_Get_Op(Gframe::gdev);
   G_Set_Op(Gframe::gdev, GXxor);

   g_draw_line(Gframe::gdev,pixstx+left, (int)pixsty, 
      pixstx+left, (int)(pixsty+pixht), color);

   G_Set_Op(Gframe::gdev, prev_op);
   G_Set_LineWidth(Gframe::gdev, linewidth);	// Set back to default
}

/************************************************************************
*                                                                       *
*  Draw a right zoom-line.						*
*  Note that it also prevents overlap with left zoom-line.		*
*  (Dynamic function)							*
*									*/
void
Imginfo::draw_zlinex2(int color)
{
   int left = (int)(zlinex1 * pixwd);
   int right = (int)(zlinex2 * pixwd);

   if (left == right)
      right++;

   int 	linewidth = G_Get_LineWidth(Gframe::gdev);
   G_Set_LineWidth(Gframe::gdev, 2);
   int prev_op = G_Get_Op(Gframe::gdev);
   G_Set_Op(Gframe::gdev, GXxor);

   g_draw_line(Gframe::gdev,pixstx+right, (int)pixsty, 
      pixstx+right, (int)(pixsty+pixht), color);

   G_Set_Op(Gframe::gdev, prev_op);
   G_Set_LineWidth(Gframe::gdev, linewidth);	// Set back to default
}

/************************************************************************
*                                                                       *
*  Draw a top zoom-line.						*
*  Note that it also prevents overlap with bottom zoom-line.		*
*  (Dynamic function)							*
*									*/
void
Imginfo::draw_zliney1(int color)
{
   int top = (int)(zliney1 * pixht);
   int bottom = (int)(zliney2 * pixht);

   if (top == bottom)
      top--;

   int 	linewidth = G_Get_LineWidth(Gframe::gdev);
   G_Set_LineWidth(Gframe::gdev, 2);
   int prev_op = G_Get_Op(Gframe::gdev);
   G_Set_Op(Gframe::gdev, GXxor);

   g_draw_line(Gframe::gdev,(int)pixstx, pixsty+top, 
      (int)(pixstx+pixwd), pixsty+top, color);

   G_Set_Op(Gframe::gdev, prev_op);
   G_Set_LineWidth(Gframe::gdev, linewidth);	// Set back to default
}

/************************************************************************
*                                                                       *
*  Draw a bottom zoom-line.						*
*  Note that it also prevents overlap with bottom zoom-line.		*
*  (Dynamic function)							*
*									*/
void
Imginfo::draw_zliney2(int color)
{
   int top = (int)(zliney1 * pixht);
   int bottom = (int)(zliney2 * pixht);

   if (top == bottom)
      bottom++;

   int 	linewidth = G_Get_LineWidth(Gframe::gdev);
   G_Set_LineWidth(Gframe::gdev, 2);
   int prev_op = G_Get_Op(Gframe::gdev);
   G_Set_Op(Gframe::gdev, GXxor);

   g_draw_line(Gframe::gdev,(int)pixstx, pixsty+bottom, 
      (int)(pixstx+pixwd), pixsty+bottom, color);

   G_Set_Op(Gframe::gdev, prev_op);
   G_Set_LineWidth(Gframe::gdev, linewidth);	// Set back to default
}
