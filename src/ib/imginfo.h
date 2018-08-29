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

#ifndef _IMGINFO_H
#define _IMGINFO_H
/************************************************************************
*
*  Ramani Pichumani
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA  94538
*
*************************************************************************/

#include <X11/Xlib.h>
#include "sisfile.h"
#include "siscms.h"
#include "roitool.h"
#include "vsfunc.h"

typedef	enum {
   	SPECTRUM,
   	IMAGE,
   	STACK_PLOT,
	MVS_LFID,
	MVS,
	MVS_CF,
	COLORMAP,
	SPECTRUM_RAW,
	IMAGE_RAW,
	MVS_RAW,
	FILTER_SPECTRUM,
	NUM_DISPLAY_TYPE
} Disp_type ;

typedef	enum {
   	NOSWAP,
	SWAP
} Display_data_swap ;

// All parameters affecting the pixel values displayed are in here.
class ImageFormat{
  public:
    short pixwd, pixht;		// Size of displayed image in pixels.
    int datastx, datasty;	// Upper left corner of data window.
    int datawd, dataht;		// Amount of data displayed.
    int interpolated;		// If TRUE, pixels interpolated from data.
    float vs;			// Intensity scaling factor.
    VsFunc *vsfunc;
};

class DDLSymbolTable;

// Image information inside the frame.  
class Imginfo 
{
  public:
    int ref_count;			// How many pointers to this Imginfo
    Imginfo(void) { Initialize(); }
    void Initialize();
    void InitializeSymTab(Sisfile_rank, Sisfile_bit, Sisfile_type,
			  int fast, int medium, int slow, int hyperslow,
			  int alloc_data = 0);
    Imginfo(Imginfo* from);
    Imginfo(Sisfile_rank new_rank, Sisfile_bit new_bit, Sisfile_type new_type,
	    int fast, int medium, int slow, int hyperslow,
	    int alloc_data = 0);
    
    virtual ~Imginfo(void);

    int memoryCheck(DDLSymbolTable *, char *errmsg, int *mbytes);
    int memoryCheck(int MbytesRequested, char *errmsg, int *MbytesShort);
    Imginfo* ddldata_load(char *filename, char *errmsg,
			  DDLSymbolTable *st=NULL);
    void ddldata_write(char *filename);
    Imginfo* LoadImage(char *filename, char *errmsg,
		       DDLSymbolTable *st=NULL);
    Imginfo* LoadImage(char *path, char *name, char *errmsg,
		       DDLSymbolTable *st=NULL);
    int SaveImage(char *filename = "NoName.fdf");
    char* GetData();
    int GetRank();
    int GetFast();
    int GetMedium();
    int GetSlow();
    double GetRatioFast();
    double GetRatioMedium();
    double GetRatioSlow();
    char *GetDirpath();
    char *GetFilename();
    char *GetFilepath();
    Flag GetOrientation(double *);

    void get_minmax(double *min, double *max);
    
    // Clip a point's coordinates to keep it inside the image
    virtual void keep_point_in_image(short *x, short *y);

    // Redisplay data
    virtual void display_data(Gframe *gframe);
    virtual void overlay_data(Gframe *gframe);
    void redisplay_data_in_rect(int x0, int y0, int x1, int y1);
    
    // Convert coordinates from data to screen frame
    virtual int XDataToScreen(float);	// Convert x-coord
    virtual int YDataToScreen(float);	// Convert y-coord

    // Convert coordinates from screen to data
    virtual float XScreenToData(int);	// Convert x-coord
    virtual float YScreenToData(int);	// Convert y-coord

    // Convert screen coords from this image to another
    Gpoint ScreenToScreen(Imginfo *to, int x, int y);

    virtual void update_scale_factors();// Update screen-to-data scale factors
    D3Dpoint pixel_to_magnet_frame(Fpoint);	// Data units to mag frame (cm)
    // Transform coords by coordinate rotation.
    Flag rotate_coords(double *orientation, double *position);
    Flag rot90_header();	// Update header vars for 90deg image rotation
    Flag flip_header();		// Update header vars for left-right image flip

    // Get the user coords in cm. of the displayed data rectangle
    Flag get_user_coords_of_displayed_data(double *x0, double *y0, double *z0,
					   double *x1, double *y1, double *z1);
    
    virtual void update_data_coordinates();	// Update everything in the 
						// display list
    void update_data_coordinates(Roitool *);	// Update one tool
    virtual void update_screen_coordinates();	// Update everything in the 
						// display list
    void update_screen_coordinates(Roitool *);	// Update one tool

    // Update datastx, etc. to fit current data into given rectangle on screen
    Flag update_image_position(int x1, int y1, int x2, int y2);
    
    // Update datastx, etc. to fit given rectangle in user coords into
    //	given screen rectangle
    Flag update_image_position(int pixx1, int pixy1, int pixx2, int pixy2,
			       double usrx1, double usry1,
			       double usrx2, double usry2);
    
    // Calculate how to fit a display of a given shape (cm_wide X cm_high)
    //  into a given screen rectangle. Returns screen position and dimensions.
    void get_image_position(int pix_x1, int pix_y1, int pix_x2, int pix_y2,
			    double cm_wide, double cm_high,
			    int *stx, int *sty, int *pix_wide, int *pix_high);

    // Get the spatial x, y, z dimensions of the data set; i.e., the number of
    // data points in each spatial dimension.
    Flag get_spatial_dimensions(int *nx, int *ny, int *nz);

    // Get the spatial x, y, z size of the data set in cm.
    Flag get_spatial_spans(double *dx, double *dy, double *dz);

    // Get the location of the center of the data set in cm.
    Flag get_location(double *x, double *y, double *z);

    // Calc intensity scale of image to normalize to given point
    virtual float calculate_new_image_vs(int x, int y, int vs_band);
    virtual float calculate_new_image_vs(float x, float y, int vs_band);
    void AutoVscale();
    Flag set_new_image_vs(float newvs);
    Flag set_new_image_vs(float min_data, float max_data, Flag set_vstool);
    Flag set_new_image_vs(VsFunc *);

    short pixstx, pixsty;	// starting point of pixel-image to display
    short pixwd, pixht;		// size of pixel-image being displayed
    short pixoffx, pixoffy;	// offset into image frame for displaying
    // spectra.
    int datastx, datasty;	// starting point of data image
    int datawd, dataht;		// size of data-image used to display
    VsFunc *vsfunc;		// Vertical scaling function
    float vs;			// vertical scale
    float vo;			// vertical offset
    int type;			// defines data type: float,short,byte...
    Disp_type disp_type;	// defines display type MVS,SPECTRUM... 
    Display_data_swap data_swap; // defines if data should be swapped. (CSI)
    int spatial_dim;		// defines spatial dimension of data (CSI)
    RoitoolList *display_list;	// List of ROIs/labels on image.
    
    
    // defines voxel sizes for MVS data.  To be used until they can be 
    // replaced with roi definitions.
    float voxel_size_x, voxel_size_y; 
    float scale_x,scale_y;
    
    class DDLSymbolTable *st;	// Contains the ddl symbol table
    Siscms_type cmsindex;	// which cms it uses
    
    // These variables are used to draw lines that control the zooming
    // of an image.  Their values are factors of image pixel width and
    // height (pixwd and pixht), ranging from 0 to 1.
    // Note that zlinex1 <= zlinex2, and zliney1 <= zliney2.
    float zlinex1, zliney1, zlinex2, zliney2;

    // zoom functions
    virtual int select_zoom_lines(int x, int y, int aperture, int zline);
    void move_zlines(short dx, short dy,
                     float left, float right, float top, float bottom,
                     int color, int zline);
    virtual void replace_zlines(float, float, float, float, int);
    virtual void draw_zlinex1(int);
    virtual void draw_zlinex2(int);
    virtual void draw_zliney1(int);
    virtual void draw_zliney2(int);
    virtual void zoom(Gframe *);
    virtual void unzoom(Gframe *);
    virtual void quickzoom(Gframe *, int x, int y, float mag);
    virtual Gframe *zoom_full(Gframe *);
//    virtual void unzoom_full();	// uneeded at this point.
    virtual void draw_zoom_lines(int);

    int display_ood;	// True if screen display needs updating (Out Of Date)
    
    // Copy of the image for this display device
    Display *display;
    Pixmap pixmap;
    Pixmap maskmap;
    ImageFormat pixmap_fmt;
    // Check pixmap validity
    int pixmap_re_scale();
    int pixmap_re_vs();
    int need_new_pixmap();
    void remove_pixmap_for_updt();
    int pixmap_ood;	// True if image pixmap is Out Of Date

    // Parameters for data-space to screen-space coordinate conversion.
    // X(screen) = int( X(data) * xscale + xoffset ).
    // Note that X(data) is real, and X(screen) is an integer (pixel label).
    // The upper left corner of a data pixel has integer coordinates.
    // (datastx, datasty) is at the upper left corner of the first data
    // point displayed; (datastx+datawd, datasty+dataht) is at the lower
    // right corner of the last data point.
    float xscale;
    float xoffset;
    float yscale;
    float yoffset;

    // data conversion functions
    void convertdata(unsigned char *, float *, int datasize);
    void convertdata(unsigned short *, float *, int datasize);
    void convertdata(int *, float *, int datasize);
    void convertdata(double *, float *, int datasize);
    void convertdata(float *, unsigned char *, int datasize, 
					double scale_factor, double offset);
    void convertdata(float *, unsigned short *, int datasize, 
					double scale_factor, double offset);
    void convertdata(float *, short *, int datasize, 
					double scale_factor, double offset);
    void convertdata(float *, int *, int datasize, 
					double scale_factor, double offset);

};

void attach_imginfo(Imginfo *&, Imginfo *);
void detach_imginfo(Imginfo *&);

Declare_ListClass(Imginfo);

class ImginfoIterator {
public:
  ImginfoIterator(ImginfoList* r) : rtl(r) {
    if (rtl)
      next = rtl->First();
    else
      next = 0;
  }
  
  ~ImginfoIterator() {};
  
  int NotEmpty() { return (next ? TRUE : FALSE) ; }
  int  IsEmpty() { return (next ? FALSE : TRUE) ; }
  
  ImginfoIterator& GotoLast()  {
    if (rtl)
      next = rtl->Last();
    return *this;
  }
  
  ImginfoIterator& GotoFirst() {
    if (rtl)
      next = rtl->First();
    return *this;
  }
  
  Imginfo *operator++() {
    ImginfoLink* curr;
    curr = next;
    if (next)
      next = next->Next();
    if (curr)
      return curr->Item();
    else
      return 0;
  }
  
  Imginfo *operator--() {
    ImginfoLink* curr;
    curr = next;
    if (next)
      next = next->Prev();
    if (curr)
      return curr->Item();
    else
      return 0;
  }
  
private:
  
  ImginfoLink* next;
  ImginfoList* rtl;
  
};


#endif _IMGINFO_H
