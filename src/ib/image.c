/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
static char *SCCSid(){
    return "@(#)image.c 18.1 03/21/08 (c)1991-92 SISCO";
}

#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "graphics.h"
#include "stderr.h"
#include "spline.h"

extern VsFunc *get_default_vsfunc();
static int bytesPerPixel;
static int pixFormat;

/* Here are the function declartions for the static functions local */
/* to this file. The function declaration for the public funtions */
/* g_display_image(), convert_float_data() are in graphics.h. */

/*  This function builds the image pixel array. It duplicates scan lines */
/*  when expanding an image, and uses maximum of groups of scan lines    */
/*  when compressing an image. It does not do any smoothing of expanded  */
/*  image data.                                                          */
static int
convert_float_data(float *indata,/* pointer to input (floating point) data */
        char *outdata,          /* pointer to output pixel data */
        int points_per_line,    /* # data points per scan line (width)*/
        int scan_lines,         /* scan lines (height of) data */
        int x_offset,           /* x offset in data to start at */
        int y_offset,           /* y offset in data to start at */
        int src_width,          /* width of data to use */
        int src_height,         /* height of data to use */
        int pix_width,          /* (resulting) image width in pixels */
        int pix_height,         /* (resulting) image height in pixels */
        Orientation direction);  /* indata orientation */

/* This function sets up the global variables internal to this module */
/* That the individual line filling routines might have to use. It also */
/* chooses which line filling routine gets called by convert_float_data() */
 
static void line_fill_init(int Line_size,  /* pixels per line in image */
                        int Data_size,  /* points per line in data */
                        Gdev *gdev, /* Pointer to SISCO colormap */
                        Siscms_type cmsindex, /* Index of SISCO colormap */
                        float Vs);   /* Vertical scale to apply to data */

static void line_fill_reset(Siscms_type cmsindex);

/* This function hs the same purpose as the above.  However, it doesn't */
/* use Gdev.  								*/
static void raw_line_fill_init(
	int Line_size,  	/* pixels per line in image */
        int Data_size,  	/* points per line in data */
	int pixel_offset,	/* pixel offset */
	int num_gray,		/* number gray level */
        float Vs);   		/* Vertical scale to apply to data */

static void line_same (float *indata, char *outdata);
 
/* This function converts floating point values into gray-scale indexes */
/* from the selected colormap in the siscms structure. It maps the data */
/* values onto the pixels using expansion. It duplicates values */
/* from the group being expanded into the scan line of pixels. */
 
static void line_expand (float *indata, char *outdata);
 
/* This function converts floating point values into gray-scale indexes */
/* from the selected colormap in the siscms structure. It maps the data */
/* values onto the pixels using compression. It uses the maximum data */
/* value from the group being compressed to convert into the colormap */
/* index. */
 
static void line_compress (float *indata, char *outdata);
 
/* This function retrieves a pointer to a trace from the input data */
/* If the x_offset is more than the width, this function returns an error */
 
static float *gettrace(float *start,           /* start of data array */
                int width,              /* width of data array */
                int x_offset,           /* X offset into array */
                int trace);             /* trace # to retrieve */


/* end of function declarations */

struct {
    u_char *lookuptable;
    int tablesize;
    float mindata;
    float maxdata;
    u_char uflowcmi;
    u_char oflowcmi;
    float scale;
} im;
static u_char ramp[] = {0, 1, 2, 3, 4, 5, 6, 7};

int smooth_flag = 0 ;
int num_lines_repeated = 0, num_points_repeated = 0 ;
int *repeated_lines = NULL, *repeated_points = NULL ;

float
interp(float first_value, float second_value, 
       int first_loc, int second_loc, int desired_loc)
{
  float f1, f2, f3, d1, d2, dr ;

  f1 = (float) first_loc ;
  f2 = (float) second_loc ;
  f3 = (float) desired_loc ;
  d1 = (f2 - f1) ;
  d2 = (f3 - f1) ;
  dr = d2 / d1 ;

  return ( first_value + (dr * (second_value - first_value)) ) ;
}


Pixmap
g_display_image(Gdev *gdev, 		/* pointer to graphics device */
		Siscms_type cms_index,	/* index of siscms Colormap to use */
		float *data, 		/* pointer to data points */
                int points_per_line,    /* data points per scan line */
		int scan_lines,		/* # of scan lines in data */
                int src_x,              /* x offset into source data */
                int src_y,              /* y offset into source data */
                int src_width,          /* width in data set, must be */
                                        /* <= points_per_line */
                int src_height,         /* height to draw in data set */
		int dest_x, 		/* x destination on canvas */
		int dest_y,		/* y destination on canvas */
		int pix_width,		/* width of pixel */
		int pix_height,		/* height of pixel */
                Orientation direction,  /* end from which data encoding */
                                        /* starts in data set */
		float vs,		/* vertical scaling of data */
		int keep_pixmap)	/* If TRUE, try to save pixmap
					 * (defaults to FALSE) */
{
    char 	*pix_data = 0;
    int		prev_op;	/* X-lib op to store while drawing */
    XImage 	*ximage;
    float	*new_data ;
    int         depth;
    Visual      *vis;
    XWindowAttributes  win_attr;

    /* Do some checking on function inputs */
    if (gdev == (Gdev *) NULL)
    {
	STDERR("display_image: passed NULL gdev pointer");
	return 0;
    }

    if (data == (float *) NULL)
    {
	STDERR("g_display_image: passed NULL data pointer");
	return 0;
    }

    /* check the direction value */
    if ((direction == LEFT) || (direction == RIGHT))
    {
	STDERR("g_display_image: LEFT or RIGHT orientation not supported");
	return 0;
    }

    /* create the X-Windows image */
    depth = DefaultDepth(gdev->xdpy, DefaultScreen(gdev->xdpy));
    vis = DefaultVisual(gdev->xdpy, DefaultScreen(gdev->xdpy));
    if (XGetWindowAttributes(gdev->xdpy,  gdev->xid, &win_attr))
    {
        vis = win_attr.visual;
        depth = win_attr.depth;
/*
        depth = win_attr.visual->bits_per_rgb;
*/
    }
    ximage = XCreateImage(gdev->xdpy, vis, depth,
                          ZPixmap, 0, pix_data, pix_width, pix_height, 8, 0);
                                                                                
    if (ximage == (XImage *) NULL)
    {
        STDERR("g_display_image:XCreateImage returned NULL pointer");
        /* you have to release malloc'ed memory if you are not going to call */
        /* XDestroyImage() later. */
        return 0;
    }

    // Info about image/pixmap format
    bytesPerPixel = ximage->bits_per_pixel / 8;
    pixFormat = -1;             // Put each data bit in a separate plane
    if (ximage->format == ZPixmap || ximage->format == XYPixmap) {
        pixFormat = 0;          // Use inefficient, generic processing
        if (ximage->bitmap_bit_order == MSBFirst ||
                ximage->bitmap_bit_order == LSBFirst) {
            pixFormat = bytesPerPixel;
            if (ximage->byte_order == LSBFirst)
                pixFormat += 4;
        }
    }
    //fprintf(stderr,"pixFormat=%d %d\n", pixFormat, ximage->format);/*CMP*/
    if (pixFormat < 1 || pixFormat > 8) {
        STDERR("aipDisplayImage: this pixmap format is not supported");
        return 0;
    }

    /* malloc space for pixel data */
    if ((pix_data = (char *) malloc(pix_width * pix_height * bytesPerPixel)) == NULL)
    {
	STDERR("g_display_image: pix_data malloc failed");
	return 0;
    }

    ximage->data = pix_data; 

    /*  Note about if (smooth_flag)...else  below:
        Of course if smooth_flag is set we want to smooth the image.
        convert_float_data() expands or compresses the data as
        necessary and then uses the colormap to produce pixel
        data.  Thus all that was here originally was what is
        in the else part - a call to line_fill_init() and a call
        to convert_float_data().  If we want to smooth, however,
        we need to separate the two steps - the smoothing is
        meaningless both before the data is expanded, and after 
        the data is transformed into pixel data - it must happen
        in between these two steps, and convert_float_data() does
        both at once.  Therefore to smooth an image, we:
        - Allocate space to hold the data while smoothing it - after
          expanding it and before mapping it to pixel data.  This
          is new_data.
        - Run the data through g_float_to_float(), which performs
          the expansion/compression. 
        - Smooth it top to bottom and then left to right.
        - Proceed as if we were not smoothing - call line_fill_init()
          and then convert_float_data(), giving it new_data, which
          of course has already been expanded but this won't affect
          convert_float_data().
        - Free new_data as it is no longer needed.

        ------>   WARNING!!!   <------
        Smoothing is meaningful only when an image is being expanded!!!
        DO NOT attempt to smooth an image unless you are sure the 
        target dimensions are greater than the source dimensions.
        smooth_flag is turned on and off by the routines at the end
        of this file, smooth() and dont_smooth().  

  ----> ALWAYS TURN SMOOTHING OFF WITH dont_smooth() IMMEDIATELY
        AFTER EACH USE!!
        
        As of this writing, the only time smoothing is used is
        when zooming (look in zoom_image() and zoom_full(). 

        */


    if (smooth_flag)
    {
      if ((new_data=(float *)malloc(pix_width*pix_height*sizeof(float)))==NULL)
      {
	  STDERR("g_display_image: new_data malloc failed");
	  return 0;
      }

      num_lines_repeated = 0 ;

      // Cubic spline interpolation
      // First use g_float_to_float() to extract the imaged region of
      // the data w/o expansion or compression.
      float *xdata;
      if ((xdata=(float *)malloc(src_width*src_height*sizeof(float)))==NULL)
      {
	  STDERR("g_display_image: xdata malloc failed");
	  return 0;
      }
      g_float_to_float(data, xdata, points_per_line, scan_lines, src_x,
			src_y, src_width, src_height, src_width, src_height);

      // Now spline that data into the picture-sized buffer.
      cubic_spline_2D_interpolation(PASSASMINUS,
				    src_width, src_height, xdata,
				    pix_width, pix_height, new_data);

      // Set up this stuff for the fake source data.
      points_per_line = pix_width ;
      scan_lines = pix_height ;
      src_x = 0 ;
      src_y = 0 ;
      src_width = pix_width ;
      src_height = pix_height ;

      free((char *)xdata);

      /* initialize the line-fill routine */
      line_fill_init (pix_width, src_width, gdev, cms_index, vs);
  
      /* scale and convert data to pixel data */
      convert_float_data(new_data, pix_data, points_per_line, scan_lines,
			 src_x, src_y, src_width, src_height,
			 pix_width, pix_height, direction);

      line_fill_reset(cms_index);
      free ((char *) new_data) ;
    }
    else
    {
      /* initialize the line-fill routine */
      line_fill_init (pix_width, src_width, gdev, cms_index, vs);

      /* scale and convert data to pixel data */
      convert_float_data(data, pix_data, points_per_line, scan_lines,
			 src_x, src_y, src_width, src_height,
			 pix_width, pix_height, direction);
      line_fill_reset(cms_index);
    } 
   
    /* store the current type of draw operation */
    prev_op = G_Get_Op(gdev);

    /* set the type of draw operation */
    G_Set_Op(gdev, GXcopy);

    Pixmap pixmap = 0;
    if (keep_pixmap){
	/* Put the image into a pixmap */
	pixmap = XCreatePixmap(gdev->xdpy, gdev->xid,
			       pix_width, pix_height,
			       depth);
	XPutImage(gdev->xdpy, pixmap, gdev->xgc, ximage, 0, 0,
		  0, 0, pix_width, pix_height);
	
	/* Copy it to the screen */
	XCopyArea(gdev->xdpy, pixmap, gdev->xid, gdev->xgc, 0, 0,
		  pix_width, pix_height, dest_x, dest_y);
    }else{
	/* Put image directly on the screen */
	XPutImage(gdev->xdpy, gdev->xid, gdev->xgc, ximage, 0, 0,
		  dest_x, dest_y, pix_width, pix_height);
    }
    
    /* restore the original X draw operation */
    G_Set_Op(gdev, prev_op);

    /* destroy the X-Windows image */
    XDestroyImage(ximage);

    return pixmap;
}			/* end of g_display_image() */

Pixmap
g_display_image(Gdev *gdev, 		/* pointer to graphics device */
		Siscms_type cms_index,	/* index of siscms Colormap to use */
		float *data, 		/* pointer to data points */
                int points_per_line,    /* data points per scan line */
		int scan_lines,		/* # of scan lines in data */
                int src_x,              /* x offset into source data */
                int src_y,              /* y offset into source data */
                int src_width,          /* width in data set, must be */
                                        /* <= points_per_line */
                int src_height,         /* height to draw in data set */
		int dest_x, 		/* x destination on canvas */
		int dest_y,		/* y destination on canvas */
		int pix_width,		/* width of pixel */
		int pix_height,		/* height of pixel */
                Orientation direction,  /* end from which data encoding */
                                        /* starts in data set */
		VsFunc *vsfunc,		/* vertical scaling of data */
		int keep_pixmap)	/* If TRUE, try to save pixmap
					 * (defaults to FALSE) */
{
    char 	*pix_data = 0;
    int		prev_op;	/* X-lib op to store while drawing */
    XImage 	*ximage;
    float	*new_data ;
    float	vs;
    int         depth;
    Visual      *vis;
    XWindowAttributes  win_attr;

    /* Do some checking on function inputs */
    if (gdev == (Gdev *) NULL)
    {
	STDERR("display_image: passed NULL gdev pointer");
	return 0;
    }
    
    if (data == (float *) NULL)
    {
	STDERR("g_display_image: passed NULL data pointer");
	return 0;
    }
    
    /* check the direction value */
    if ((direction == LEFT) || (direction == RIGHT))
    {
	STDERR("g_display_image: LEFT or RIGHT orientation not supported");
	return 0;
    }
    
    /* create the X-Windows image */
    depth = DefaultDepth(gdev->xdpy, DefaultScreen(gdev->xdpy));
    vis = DefaultVisual(gdev->xdpy, DefaultScreen(gdev->xdpy));
    if (XGetWindowAttributes(gdev->xdpy,  gdev->xid, &win_attr))
    {
        vis = win_attr.visual;
        depth = win_attr.depth;
/*
        depth = win_attr.visual->bits_per_rgb;
*/
    }
    ximage = XCreateImage(gdev->xdpy, vis, depth,
			  ZPixmap, 0, pix_data, pix_width, pix_height, 8, 0);
    
    if (ximage == (XImage *) NULL)
    {
	STDERR("g_display_image:XCreateImage returned NULL pointer");
	/* you have to release malloc'ed memory if you are not going to call */
	/* XDestroyImage() later. */
	return 0;
    }

    // Info about image/pixmap format
    bytesPerPixel = ximage->bits_per_pixel / 8;
    pixFormat = -1;             // Put each data bit in a separate plane
    if (ximage->format == ZPixmap || ximage->format == XYPixmap) {
        pixFormat = 0;          // Use inefficient, generic processing
        if (ximage->bitmap_bit_order == MSBFirst ||
                ximage->bitmap_bit_order == LSBFirst) {
            pixFormat = bytesPerPixel;
            if (ximage->byte_order == LSBFirst)
                pixFormat += 4;
        }
    }
    //fprintf(stderr,"pixFormat=%d %d\n", pixFormat, ximage->format);/*CMP*/
    if (pixFormat < 1 || pixFormat > 8) {
        STDERR("aipDisplayImage: this pixmap format is not supported");
        return 0;
    }


    /* malloc space for pixel data */
    if ((pix_data = (char *) malloc(pix_width * pix_height * bytesPerPixel)) == NULL)
    {
	STDERR("g_display_image: pix_data malloc failed");
	return 0;
    }
    
    ximage->data = pix_data; 
    
    /*  Note about if (smooth_flag)...else  below:
        Of course if smooth_flag is set we want to smooth the image.
        convert_float_data() expands or compresses the data as
        necessary and then uses the colormap to produce pixel
        data.  Thus all that was here originally was what is
        in the else part - a call to line_fill_init() and a call
        to convert_float_data().  If we want to smooth, however,
        we need to separate the two steps - the smoothing is
        meaningless both before the data is expanded, and after 
        the data is transformed into pixel data - it must happen
        in between these two steps, and convert_float_data() does
        both at once.  Therefore to smooth an image, we:
        - Allocate space to hold the data while smoothing it - after
	expanding it and before mapping it to pixel data.  This
	is new_data.
        - Run the data through g_float_to_float(), which performs
	the expansion/compression. 
        - Smooth it top to bottom and then left to right.
        - Proceed as if we were not smoothing - call line_fill_init()
	and then convert_float_data(), giving it new_data, which
	of course has already been expanded but this won't affect
	convert_float_data().
        - Free new_data as it is no longer needed.
	
        */

    // Do not fiddle with the table's ref_count, as we only use the table
    // for the duration of the g_display_image() call.
    if (vsfunc->lookup && vsfunc->lookup->table){
	im.lookuptable = vsfunc->lookup->table;
	im.tablesize = vsfunc->lookup->size;
    }else{
	im.lookuptable = ramp;
	im.tablesize = sizeof(ramp);
    }
    im.mindata = vsfunc->min_data;
    im.maxdata = vsfunc->max_data;
    if ( (im.maxdata - im.mindata) < 1.0)
       im.maxdata = im.mindata + 1.0;
    im.uflowcmi = vsfunc->uflow_cmi;
    im.oflowcmi = vsfunc->oflow_cmi;
    im.scale = im.tablesize / (im.maxdata - im.mindata);
    vs = ((im.lookuptable[im.tablesize-1] - im.lookuptable[0] - 1)
	  / (im.maxdata - im.mindata));

    if (smooth_flag)
    {
	if ((new_data=(float *)malloc(pix_width*pix_height*sizeof(float)))==NULL)
	{
	    STDERR("g_display_image: new_data malloc failed");
	    return 0;
	}
	
	num_lines_repeated = 0 ;
	
	// Cubic spline interpolation
	// First use g_float_to_float() to extract the imaged region of
	// the data w/o expansion or compression.
	float *xdata;
	if ((xdata=(float *)malloc(src_width*src_height*sizeof(float)))==NULL)
	{
	    STDERR("g_display_image: xdata malloc failed");
	    return 0;
	}
	g_float_to_float(data, xdata, points_per_line, scan_lines, src_x,
			 src_y, src_width, src_height, src_width, src_height);
	
	// Now spline that data into the picture-sized buffer.
	cubic_spline_2D_interpolation(PASSASMINUS,
				      src_width, src_height, xdata,
				      pix_width, pix_height, new_data);
	
	// Set up this stuff for the fake source data.
	points_per_line = pix_width ;
	scan_lines = pix_height ;
	src_x = 0 ;
	src_y = 0 ;
	src_width = pix_width ;
	src_height = pix_height ;
	
	free((char *)xdata);
	
	/* initialize the line-fill routine */
	line_fill_init (pix_width, src_width, gdev, cms_index, vs);
	
	/* scale and convert data to pixel data */
	convert_float_data(new_data, pix_data, points_per_line, scan_lines,
			   src_x, src_y, src_width, src_height,
			   pix_width, pix_height, direction);
	
	free ((char *) new_data) ;
    }
    else
    {
	/* initialize the line-fill routine */
	line_fill_init (pix_width, src_width, gdev, cms_index, vs);
	
	/* scale and convert data to pixel data */
	convert_float_data(data, pix_data, points_per_line, scan_lines,
			   src_x, src_y, src_width, src_height,
			   pix_width, pix_height, direction);
    } 
    
    /* store the current type of draw operation */
    prev_op = G_Get_Op(gdev);

    /* set the type of draw operation */
    G_Set_Op(gdev, GXcopy);

    Pixmap pixmap = 0;
    if (keep_pixmap){
	/* Put the image into a pixmap */
	pixmap = XCreatePixmap(gdev->xdpy, gdev->xid,
			       pix_width, pix_height,
			       depth);
	XPutImage(gdev->xdpy, pixmap, gdev->xgc, ximage, 0, 0,
		  0, 0, pix_width, pix_height);
	
	/* Copy it to the screen */
	XCopyArea(gdev->xdpy, pixmap, gdev->xid, gdev->xgc, 0, 0,
		  pix_width, pix_height, dest_x, dest_y);
    }else{
	/* Put image directly on the screen */
	XPutImage(gdev->xdpy, gdev->xid, gdev->xgc, ximage, 0, 0,
		  dest_x, dest_y, pix_width, pix_height);
    }
    
    /* restore the original X draw operation */
    G_Set_Op(gdev, prev_op);

    /* destroy the X-Windows image */
    XDestroyImage(ximage);

    return pixmap;
}			/* end of g_display_image() */


static Flag raw_flag=FALSE;/* use to indicate whether to obtain X11 pixel */
			/* or regular user pixel */
void
g_float_to_pixel(float *indata, /* pointer to input (floating point) data */
        char *outdata,          /* pointer to output pixel data */
        int points_per_line,    /* # data points per scan line (width)*/
        int scan_lines,         /* scan lines (height of) data */
        int x_offset,           /* x offset in data to start at */
        int y_offset,           /* y offset in data to start at */
        int src_width,          /* width of data to use */
        int src_height,         /* height of data to use */
        int pix_width,          /* (resulting) image width in pixels */
        int pix_height,         /* (resulting) image height in pixels */
        float vs,               /* vertical scale to apply */
        int pixel_offset,       /* pixel offset */ 
        int num_pixel_level,    /* number of pixel level */ 
        Orientation direction)  /* indata orientation */
{
   if ((indata == NULL) || (outdata == NULL))
   {
      STDERR("g_float_to_pixel: passed NULL data pointer");
      return;
   }

   /* initialize the line-fill routine */
   raw_line_fill_init(pix_width, src_width, pixel_offset, 
		      num_pixel_level, vs);

   raw_flag = TRUE;

   /* scale and convert data to pixel data */
   convert_float_data(indata, outdata, points_per_line, scan_lines, x_offset, 
	y_offset, src_width, src_height, pix_width, pix_height, direction);

   raw_flag = FALSE;
} /* end of function g_float_to_pixel */

static int   line_size; 	/* number of image points in an image line */
static int   data_size;  /* number of data points available to fill image line */
static int   D1, D2; /* decision-variable deltas for expanding/compressing */
static int   g_max_gray;
static int   g_min_gray;
static u_long *x_color;
static float vs;

/* pointer to function to use for filling out image lines with data points */
static void (*LineFill)(float *indata, char *outdata);

/****************************************************************************
 line_same
 
 This function fits the number of data points into a same-size image line.
 
 INPUT ARGS:
   indata      A pointer to the start of the data points to be loaded into
               the image line.
   outdata     A pointer to the start of the image line that gets the data.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 GLOBALS USED:
   line_size   The number of image points in an image line.
   g_max_gray  The maximum gray-level value.
   g_min_gray  The minimum gray-level value.
   x_color     Pointer to the X colormap
   vs	       The vertical scale of the image.
 GLOBALS CHANGED:
   none
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:
   This function is called via the "LineFill" global variable.

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static void
setImagePixel(char **ppout, u_long xcolor)
{
    u_long pix;
    switch (pixFormat) {
      case 1:                   // 8 bits
      case 5:
        *((*ppout)++) = (char)xcolor;
        break;
      case 2:                   // 16 bits
        *((*ppout)++) = (char)(xcolor >> 8);
        *((*ppout)++) = (char)(xcolor);
        break;
      case 3:                   // 24 bits
        *((*ppout)++) = (char)(xcolor >> 16);
        *((*ppout)++) = (char)(xcolor >> 8);
        *((*ppout)++) = (char)(xcolor);
        break;
      case 4:                   // 32 bits
        *((*ppout)++) = (char)(xcolor >> 24);
        *((*ppout)++) = (char)(xcolor >> 16);
        *((*ppout)++) = (char)(xcolor >> 8);
        *((*ppout)++) = (char)(xcolor);
        break;
      case 6:                   // 16 bits, LSB first
        *((*ppout)++) = (char)(xcolor);
        *((*ppout)++) = (char)(xcolor >> 8);
        break;
      case 7:                   // 24 bits, LSB first
        *((*ppout)++) = (char)(xcolor);
        *((*ppout)++) = (char)(xcolor >> 8);
        *((*ppout)++) = (char)(xcolor >> 16);
        break;
      case 8:                   // 32 bits, LSB first
        *((*ppout)++) = (char)(xcolor);
        *((*ppout)++) = (char)(xcolor >> 8);
        *((*ppout)++) = (char)(xcolor >> 16);
        *((*ppout)++) = (char)(xcolor >> 24);
        break;
    }
}

static void line_same (float *indata, char *outdata)
{
    /**************************************************************************
    LOCAL VARIABLES:
 
    p_in     A fast pointer to the data being loaded into the image line.
    p_out    A fast pointer to the start of the image line.
    cmi      The colormap index value.
    i        A counter for the length of the image line.
    */
    float *p_in  = indata;
    char *p_out = outdata;
    int  cmi;
    int    i;
 
    if (raw_flag)
    {
       // see aipGraphicsWin.C
       //for (i = line_size; i > 0; --i, ++p_in, ++p_out)
       for (i = line_size; i > 0; --i, ++p_in)
       {
	   /* Test a data value against limits and convert to a */
	   /* colormap entry. */
	   if (*p_in <= im.mindata){
	       *p_out = im.uflowcmi;
	   }else if (*p_in >= im.maxdata){
	       *p_out = im.oflowcmi;
	   }else{
	       *p_out =
	       im.lookuptable[(int)((*p_in - im.mindata) * im.scale)];
	   }
       }
    }
    else
    {
       //for (i = line_size; i > 0; --i, ++p_in, ++p_out)
       for (i = line_size; i > 0; --i, ++p_in)
       {
	   /* Test a data value against limits and convert to a */
	   /* colormap entry. */
	   if (*p_in <= im.mindata){
	       cmi = im.uflowcmi;
	   }else if (*p_in >= im.maxdata){
	       cmi = im.oflowcmi;
	   }else{
	       cmi =
	       im.lookuptable[(int)((*p_in - im.mindata) * im.scale)];
	   }
	   //*p_out = (char)x_color[cmi];
	   setImagePixel(&p_out, x_color[cmi]);
       }
    }
}  /* end of function "line_same" */

/****************************************************************************
 line_expand
 
 This function fits the number of data points into a LONGER image line.
 
 INPUT ARGS:
   indata      A pointer to the start of the data points to be loaded into
               the image line.
   outdata     A pointer to the start of the image line that gets the data.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 GLOBALS USED:
   line_size   The number of image points in an image line.
   D1, D2      Decision-variable deltas used for expanding/compressing the
               data points to fill out an image line (see Foley & van Dam).
   g_max_gray  The maximum gray-level value.
   g_min_gray  The minimum gray-level value.
   x_color     Pointer to the X colormap
   vs	       The vertical scale of the image.
 GLOBALS CHANGED:
   none
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:
   This function is called via the "LineFill" global variable.
   This function uses Bresenham's decision-variable algorithm; for a formal
   development of the method, see Section 11.2.2, "Bresenham's Line Algorithm",
   in Foley & van Dam, "Fundamentals of Interactive Computer Graphics" (1981).

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static void line_expand (float *indata, char *outdata)
{
    /**************************************************************************
    LOCAL VARIABLES:
 
    p_in     A fast pointer to the data being loaded into the image line.
    p_out    A fast pointer to the start of the image line.
    dv       The decision variable (see Foley & van Dam).
    cmi      The colormap index.
    i        A counter for the length of the image line.
    */
    float *p_in  = indata;
    char *p_out = outdata;
    short  max_gray = g_max_gray;
    short  min_gray = g_min_gray;
    int    dv;
    int cmi;
    int    i;

    /* check the imput and output buffer pointers */
    if (indata = (float *) NULL)
    {
	STDERR("line_expand: passed NULL indata pointer");
	return;
    }

    if (outdata = (char *) NULL)
    {
	STDERR("line_expand: passed NULL outdata pointer");
	return;
    }

    /* set the starting value of the decision variable */
    dv = D2;

    /* Test first data value against limits and convert to an */
    /* initial colormap entry. */
    if (*p_in <= im.mindata){
	cmi = im.uflowcmi;
    }else if (*p_in >= im.maxdata){
	cmi = im.oflowcmi;
    }else{
	cmi =
	im.lookuptable[(int)((*p_in - im.mindata) * im.scale)];
    }

    if (raw_flag)
    {
	/* Load the first data value into the output buffer */
	*p_out++ =  (char)cmi;
	for (i = line_size-1; i > 0; --i, ++p_out)
	{
	    /* Adjust the decision variable */
	    if (dv < 0){
		dv += D1;
	    }else{
		dv += D2;
		
		/* Move to the next input point */
		++p_in;
		
		/* Test a data value against limits and convert to a */
		/* colormap entry. */
		if (*p_in <= im.mindata){
		    cmi = im.uflowcmi;
		}else if (*p_in >= im.maxdata){
		    cmi = im.oflowcmi;
		}else{
		    cmi =
		    im.lookuptable[(int)((*p_in - im.mindata) * im.scale)];
		}
	    }
	    /* Load the data value into the output buffer */
	    *p_out =  (char)cmi;
	}
    }
    else
    {
	/* Load the first data value into the output buffer */
	// *p_out++ =  (char)x_color[cmi];
	setImagePixel(&p_out, x_color[cmi]);
	//for (i = line_size-1; i > 0; --i, ++p_out)
	for (i = line_size-1; i > 0; --i)
	{
	    /* Adjust the decision variable */
	    if (dv < 0){
		dv += D1;
	    }else{
		dv += D2;
		
		/* move to the next input point */
		++p_in;
		
		/* Test a data value against limits and convert to a */
		/* colormap entry. */
		if (*p_in <= im.mindata){
		    cmi = im.uflowcmi;
		}else if (*p_in >= im.maxdata){
		    cmi = im.oflowcmi;
		}else{
		    cmi =
		    im.lookuptable[(int)((*p_in - im.mindata) * im.scale)];
		}
	    }
	    /* Load the data value into the output buffer */
	    //*p_out =  (char)x_color[cmi];
	    setImagePixel(&p_out, x_color[cmi]);
	}
    }
}  /* end of function "line_expand" */

/****************************************************************************
 line_compress
 
 This function fits the number of data points into a SHORTER image line.
 
 INPUT ARGS:
   indata      A pointer to the start of the data points to be loaded into
               the image line.
   outdata     A pointer to the start of the image line that gets the data.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 GLOBALS USED:
   data_size   The number of data points available for an image line.
   line_size   The number of image points in an image line.
   D1, D2      Decision-variable deltas used for expanding/compressing the
               data points to fill out an image line (see Foley & van Dam).
   g_max_gray  The maximum gray-level value.
   g_min_gray  The minimum gray-level value.
   x_color     Pointer to the X colormap
   vs	       The vertical scale of the image.
 GLOBALS CHANGED:
   none
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:
   This function is called via the "LineFill" global variable.
   This function uses Bresenham's decision-variable algorithm; for a formal
   development of the method, see Section 11.2.2, "Bresenham's Line Algorithm",
   in Foley & van Dam, "Fundamentals of Interactive Computer Graphics" (1981).
 BUGS:
   Displays the maximum value of the data points binned into a pixel;
   does not do any averaging or division of signal between pixels.

   Only works on POSITIVE DATA.

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static void line_compress (float *indata, char *outdata)
{
    /**************************************************************************
    LOCAL VARIABLES:
  
    p_in     A fast pointer to the data being loaded into the image line.
    p_out    A fast pointer to the start of the image line.
    dv       The decision variable (see Foley & van Dam).
    cmi      The colormap index value.
    i        A counter for the number of data points to load.
    max_val  The maximum value of an input data point, which causes the
            maximum of sequential deleted data points to be loaded.
    */
    float *p_in  = indata;
    char *p_out = outdata;
    int    dv;
    int    i;
    float max_val;
    int cmi;
#ifdef SOLARIS
    float inf = HUGE_VAL;
#else
    float inf = HUGE;
#endif

    max_val = -inf;

    /* set the starting value of the decision variable */
    dv = D2;

    if (raw_flag)
    {
       for (i = data_size; i > 0; --i, ++p_in)
       {
	   if (*p_in > max_val)
               max_val = *p_in;

	   /* adjust the decision variable */
	   if (dv < 0)
               dv += D1;
	   else
	   {
               dv += D2;

    	       /* Test a data value against limits and convert to a */
	       /* colormap entry. */
	       if (max_val <= im.mindata){
		   cmi = im.uflowcmi;
	       }else if (max_val >= im.maxdata){
		   cmi = im.oflowcmi;
	       }else{
		   cmi =
		   im.lookuptable[(int)((max_val - im.mindata) * im.scale)];
	       }
	       *p_out++ = (char)cmi;

               max_val = -inf;
	   }
	}
    }
    else
    {
       for (i = data_size; i > 0; --i, ++p_in)
       {
	   if (*p_in > max_val)
               max_val = *p_in;

	   /* adjust the decision variable */
	   if (dv < 0)
               dv += D1;
	   else
	   {
               dv += D2;

    	       /* Test a data value against limits and convert to a */
	       /* colormap entry. */
	       if (max_val <= im.mindata){
		   cmi = im.uflowcmi;
	       }else if (max_val >= im.maxdata){
		   cmi = im.oflowcmi;
	       }else{
		   cmi =
		   im.lookuptable[(int)((max_val - im.mindata) * im.scale)];
	       }

               /* load the data value into the output buffer, and move to the
               next point */ 
	       // *p_out++ = (char)x_color[cmi];
	       setImagePixel(&p_out, x_color[cmi]);

               max_val = -inf;
	   }
	}
    }

}  /* end of function "line_compress" */

/****************************************************************************
 line_fill_init
 
 This function sets the function and control variables used for filling out
 image lines with data points.
 
 INPUT ARGS:
   Line_size   The number of points in an image line to fill with image data.
   Data_size   The number of data points available for an image line.
   gdev        The SISCO structure describing the graphics device
   int cmsindex   The index of the colormap to use. (2 or 3, typically )
   float Vs	The vertical scaling to use in converting the image
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 GLOBALS USED:
   data_size   The number of data points available for an image line.
   line_size   The number of image points in an image line.
   D1, D2      Decision-variable deltas used for expanding/compressing the
               data points to fill out an image line (see Foley & van Dam).
   g_max_gray  The maximum gray-level value.
   g_min_gray  The minimum gray-level value.
   x_color     Pointer to the X colormap
   vs	       The vertical scale of the image.
   LineFill    A pointer to the function to use for filling out image lines
               with data points.
 GLOBALS CHANGED:
   line_size   The number of image points in an image line.
   data_size   The number of data points available for an image line.
   D1, D2      Decision-variable deltas used for expanding/compressing the
               data points to fill out an image line (see Foley & van Dam).
   g_max_gray  The maximum gray-level value.
   g_min_gray  The minimum gray-level value.
   x_color     Pointer to the X colormap
   vs		The vertical scale of the image.
   LineFill    A pointer to the function to use for filling out image lines
               with data points.
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:
   This function uses Bresenham's decision-variable algorithm; for a formal
   development of the method, see Section 11.2.2, "Bresenham's Line Algorithm",
   in Foley & van Dam, "Fundamentals of Interactive Computer Graphics" (1981).

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static void line_fill_init (int Line_size,/* pixels per line in image */
	int Data_size, 		/* points per line in data */
	Gdev *gdev, 		/* Pointer to SISCO colormap */
	Siscms_type cmsindex, 	/* Index of SISCO colormap */
	float Vs)		/* Vertical scale to apply to data */
{
    /**************************************************************************
    LOCAL VARIABLES:
 
    none
    */

    /* set the sizes of the input and output lines */
    line_size = Line_size;
    data_size  = Data_size;
    vs = Vs;

    /* store the X colormap */
    x_color = gdev->xcolor;

    /* Determine the starting and ending of the colormap you are using */
    switch (cmsindex)
    {
	case SISCMS_1 :
	    g_min_gray = gdev->siscms->st_cms1;
	    g_max_gray = g_min_gray + gdev->siscms->size_cms1 - 1;
	    break;

	case SISCMS_2 :
	    g_min_gray = gdev->siscms->st_cms2;
	    g_max_gray = g_min_gray + gdev->siscms->size_cms2 - 1;
	    break;

	case SISCMS_3 :
	    g_min_gray = gdev->siscms->st_cms3;
	    g_max_gray = g_min_gray + gdev->siscms->size_cms3 - 1;
	    break;

	case SISCMS_4 :
	{
	    int i;
	    VsFunc *tmpvsfunc;
	    tmpvsfunc = get_default_vsfunc();
	    g_min_gray = gdev->siscms->st_cms2;
	    g_max_gray = g_min_gray + gdev->siscms->size_cms2 
				     + gdev->siscms->size_cms3 - 1;
	    im.mindata = 0;
	    im.maxdata = g_max_gray - g_min_gray;
	    im.uflowcmi = im.mindata;
	    im.oflowcmi = im.maxdata;
	    im.tablesize = im.maxdata+1;
	    im.lookuptable = (u_char *) malloc(im.tablesize);
	    if (im.lookuptable == NULL)
	    {
		im.lookuptable = ramp;
		im.tablesize = sizeof(ramp);
	    }
	    im.scale = im.tablesize / (im.maxdata - im.mindata + 1);
	    for (i=0; i<im.tablesize; i++)
	    {
		im.lookuptable[i] = g_min_gray+i;
	    }
	}

    }	/* end of switch on cmsindex */

    /* the data points exactly fit the number of display points */
    if (data_size == line_size)
    {
	LineFill = line_same;
    }
    /* expand (repeat some of) the data points to fill out the display points */
    else if (data_size < line_size)
    {
	D1 = 2*data_size;
	D2 = D1 - 2*line_size;

	LineFill = line_expand;
    }
    /* compress (delete some of) the data points to fit the display points */
    else
    {
	D1 = 2*line_size;
	D2 = D1 - 2*data_size;

	LineFill = line_compress;
    }
}  /* end of function "line_fill_init" */

static void line_fill_reset (Siscms_type cmsindex)
{
    switch (cmsindex)
    {
	case SISCMS_1 :
	    break;

	case SISCMS_2 :
	    break;

	case SISCMS_3 :
	    break;

	case SISCMS_4 :
	    free ((u_char *)im.lookuptable);	    
	    break;
    }
}

static void raw_line_fill_init(
	int Line_size,  	/* pixels per line in image */
        int Data_size,  	/* points per line in data */
	int pixel_offset,	/* pixel offset */
	int num_gray,		/* number gray level */
        float Vs)   		/* Vertical scale to apply to data */
{
    /* set the sizes of the input and output lines */
    line_size = Line_size;
    data_size  = Data_size;
    vs = Vs;

    /* Determine the starting and ending of the colormap you are using */
    g_min_gray = pixel_offset;
    g_max_gray = pixel_offset + num_gray - 1;

    /* the data points exactly fit the number of display points */
    if (data_size == line_size)
    {
	LineFill = line_same;
    }
    /* expand (repeat some of) the data points to fill out the display points */
    else if (data_size < line_size)
    {
	D1 = 2*data_size;
	D2 = D1 - 2*line_size;

	LineFill = line_expand;
    }
    /* compress (delete some of) the data points to fit the display points */
    else
    {
	D1 = 2*line_size;
	D2 = D1 - 2*data_size;

	LineFill = line_compress;
    }
}

/****************************************************************************
 convert_float_data

 This function converts floating-point data to n-bit pixel data, depending
 on the number of gray-levels or false-color levels in the selected color
 map.

 INPUT ARGS:
   indata	A pointer to the array of input values
   outdata	A pointer to the output pixel buffer.
   x_data	The number of data points in each row (trace).
   y_data	The number of rows (traces) in the data.
   x_offset	The X Offset into the data set to start at.
   y_offset	The Y Offset into the data set to start at.
   width	The width of the data array to use
   height	The height of the data array to use
   x_image	The width of the image in pixels.
   y_image	The height of the image in pixels.
   gdev		The SISCO graphics device structure to use
   cmsindex	The index of the colormap to use.
   vs		The vertical scale to use.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   OK           Image successfully converted and loaded.
   NOT_OK       Error building output image.
 GLOBALS CHANGED:
   none
 ERRORS:
   NOT_OK       Can't read data trace: "gettrace()" returned error.
   NOT_OK       Can't allocate temporary memory for building image.
 EXIT VALUES:
   none
 NOTES:
   This function uses Bresenham's decision-variable algorithm; for a formal
   development of the method, see Section 11.2.2, "Bresenham's Line Algorithm",
   in Foley & van Dam, "Fundamentals of Interactive Computer Graphics" (1981).

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static int convert_float_data (
			float *indata, 
			char *outdata, 
			int x_data, 
			int y_data, 
			int x_offset,
			int y_offset,
			int,			// width (not needed)
			int height,
			int x_image,
			int y_image,
			Orientation direction)
{
    /**************************************************************************
    LOCAL VARIABLES:

    sub_msg     The name of this function, for error messages.
    trace       A counter for the trace read from the data.
    max_val     A pointer to a buffer used for generating the max data values
               for traces that will be removed during image compression.
    dv, d1, d2  The decision variable and delta's (see Foley & van Dam).
    p_in	A fast pointer to the input buffer.
    p_out       A fast pointer to the output buffer; register is desired since
               the pointer is changed during image filling.
    i, j        Counters used for image filling and expansion/compression.
    trace_ptr   Points to the trace being currently processed
    */
    static char   	sub_msg[] = "convert_float_data:";
    int    		trace;
    float 		*max_val;
    int    		dv, d1, d2;
    register char 	*p_out;
    int    		i;
    register float 	*trace_ptr;

    /* set a fast pointer to the output buffer */
    p_out = outdata;

    /* Build an image from the data: there are 3 possibilities:
      the data traces exactly fit the number of display lines */
    if (height == y_image)
    {
	if (direction == TOP)
	{
	    /* top down direction */
      	    for (trace = y_offset; trace < y_offset + height; trace++)
      	    {
         	if ( (trace_ptr = gettrace (indata, x_data, x_offset,
		    	trace)) == (float *) NULL) {
            	    return (NOT_OK);
		}

         	(*LineFill)(trace_ptr, p_out);
         	p_out += x_image * bytesPerPixel;
      	    }
	}
	else if (direction == BOTTOM)
	/* Vnmr data starts at bottom scan line and comes up */
	{
      	    for (trace = y_data-1 - y_offset; trace > y_data-1 - y_offset -
		height; trace--)
      	    {
         	if ( (trace_ptr = gettrace (indata, x_data, x_offset,
		    	trace)) == (float *) NULL) {
            	    return (NOT_OK);
		}

         	(*LineFill)(trace_ptr, p_out);
         	p_out += x_image * bytesPerPixel;
      	    }
	}
    }
    /* expand (repeat some of) the data traces to fill out the display lines */
    else if (height < y_image)
    {
	/* set the adjustment values for the decision variable */
	d1 = 2 * height;
	d2 = d1 - (2 * y_image);
 
	/* set the starting value of the decision variable */
	dv = d2;

	if (direction == TOP)
	{
	    /* top down direction */
            trace_ptr = NULL;
            for (i = 0, trace = y_offset; i < y_image; i++, p_out += x_image * bytesPerPixel)
	    {
             	/* adjust the decision variable */
             	if (dv < 0)
		{
                    if (trace_ptr)
                    {
                       dv += d1;
                       (void)memcpy(p_out, p_out-x_image * bytesPerPixel, x_image *
				    bytesPerPixel);
                    }
                    else
                    {
                       /* Only get executed once (at the most)*/
                       if ( (trace_ptr = gettrace (indata, x_data, x_offset,
						   trace++)) == (float *)NULL) {
                               return (NOT_OK);
			}
                       /* load the data trace into the output buffer */
                       (*LineFill)(trace_ptr, p_out);
                    }
		}
             	else
             	{
                    dv += d2;
 
	    	    if ( (trace_ptr = gettrace (indata, x_data, x_offset,
						trace++)) == (float *) NULL) {
            	    	    return (NOT_OK);
		    }
                    /* load the data trace into the output buffer */
                    (*LineFill)(trace_ptr, p_out);
             	}
	    }		/* end of for all image scan lines */
	}
	else if (direction == BOTTOM)
	/* Vnmr data starts at bottom scan line and comes up */
	{
            /* starting with the last data trace, load the display lines */
            trace_ptr = NULL;
            for (i = y_image, trace = y_data - y_offset; i > 0;
		 --i, p_out += x_image * bytesPerPixel)
	    {
             	/* adjust the decision variable */
             	if (dv < 0)
		{
                    dv += d1;
                    if (trace_ptr)
                    {
                       (void)memcpy(p_out, p_out-x_image * bytesPerPixel, x_image *
				      bytesPerPixel);
                    }
                    else
                    {
                       /* Only get executed once (at the most)*/
                       if ( (trace_ptr = gettrace (indata, x_data, x_offset,
						   trace-1)) == (float *)NULL) {
                               return (NOT_OK);
			}
                       /* load the data trace into the output buffer */
                       (*LineFill)(trace_ptr, p_out);
                    }
		}
             	else
             	{
                    dv += d2;
 
                    /* decrement the data trace counter */
                    --trace;

	    	    if ( (trace_ptr = gettrace (indata, x_data, x_offset,
						trace)) == (float *) NULL) {
            	    	    return (NOT_OK);
		    }
                    /* load the data trace into the output buffer */
                    (*LineFill)(trace_ptr, p_out);
             	}
             }	/* end of for all display scan lines */
	}	/* end of if display inverted data set */
    }		/* end of if expand scan lines */
    /* compress (delete some of) the data traces to fit into the display lines */
    else
    {
	int clearbuf = TRUE;

      	/* allocate a maximum buffer */
      	if ((max_val=(float *)malloc((uint)x_data * sizeof(float))) == NULL)
      	{
	    STDERR("convert_float_data: can't allocate memory");
            return (NOT_OK);
      	}
      	/* set the adjustment values for the decision variable */
      	d1 = 2*y_image;
      	d2 = d1 - 2 * height;
 
      	/* set the starting value of the decision variable */
	dv = d2;

	if (direction == TOP)
	{
      	    /* starting with the first data trace, load the display lines */
      	    for (trace = y_offset; trace < y_offset + height; trace++)
      	    {
         	if ( (trace_ptr = gettrace (indata, x_data, x_offset,
                	trace)) == (float *)NULL) {
            	    return (NOT_OK);
		}

         	/* load this trace into the maximum buffer */
		if (clearbuf){
		    for (i = 0; i < x_data; ++i){
			*(max_val+i) = *(trace_ptr+i);
		    }
		    clearbuf = FALSE;
		}else{
		    for (i = 0; i < x_data; ++i){
			if (*(trace_ptr+i) > *(max_val+i)){
			    *(max_val+i) = *(trace_ptr+i);
			}
		    }
		}

         	/* adjust the decision variable */
         	if (dv < 0)
            	    dv += d1;
         	else
         	{
            	    dv += d2;

            	    /* load the maximum buffer into the output buffer */
            	    (*LineFill)(max_val, p_out);
            	    p_out += x_image * bytesPerPixel;
 
            	    /* clear the maximum buffer */
            	    clearbuf = TRUE;
         	}
	    }	/* end of for all traces */
	}
	else if (direction == BOTTOM)
	/* Vnmr data starts at bottom scan line and comes up */
	{
      	    /* starting with the last data trace, load the display lines */
      	    for (trace = y_data-1 - y_offset; trace > y_data-1 - y_offset -
		 height; trace--)
      	    {
         	if ( (trace_ptr = gettrace (indata, x_data, x_offset,
					    trace)) == (float *)NULL) {
            	    return (NOT_OK);
		}

         	/* load this trace into the maximum buffer */
		if (clearbuf){
		    for (i = 0; i < x_data; ++i){
			*(max_val+i) = *(trace_ptr+i);
		    }
		    clearbuf = FALSE;
		}else{
		    for (i = 0; i < x_data; ++i){
			if (*(trace_ptr+i) > *(max_val+i)){
			    *(max_val+i) = *(trace_ptr+i);
			}
		    }
		}

         	/* adjust the decision variable */
         	if (dv < 0)
            	    dv += d1;
         	else
         	{
            	    dv += d2;

            	    /* load the maximum buffer into the output buffer */
            	    (*LineFill)(max_val, p_out);
            	    p_out += x_image * bytesPerPixel;
 
            	    /* clear the maximum buffer */
            	    clearbuf = TRUE;
         	}
	    }		/* end of for all traces */
	}		/* end of if display inverted data */
      	free ((char *)max_val);
    }

    return (OK);

}  /* end of function "convert_float_data" */

static float *gettrace(float *start, int width, int x_offset, int trace)
{
    if (x_offset > width)
	return ((float *) NULL);

    return (start + trace * width + x_offset);
}	/* end of gettrace() */

void smooth (void)
{
  smooth_flag = 1 ;
}

void dont_smooth (void)
{
  smooth_flag = 0 ;
}


