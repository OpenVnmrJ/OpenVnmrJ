/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  Two main routines:							*
*     - Display image on the screen, where its data is type of short	*
*       (12 bits).							*
*     - Convert type of short data to pixel data where its value range	*
*       from 0 to max_value as specified.  (typically max_value is	*
*       4096 (2 to the power 12)					*
*									*
*									*
*     NOTE!!!  There is no provision for smoothing short data!!         *
*     That code would go in this file.  For more info see comments      *
*     in image.c.  Smoothing for float data is implemented in           *
*     that file and to implement smoothing for short data one would     *
*     copy the appropriate code into this file, changing 'float'        *
*     to 'short' everywhere, etc.  Note that there is some code         *
*     related to smoothing that would NOT be duplicated,  i.e.          *
*     the smooth() and dont_smooth() routines.                          *
*									*
*************************************************************************/
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "graphics.h"
#include "stderr.h"

static int
convert_short_data(short *indata,/* pointer to input data */
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

static void line_fill_init(int Line_size,  /* pixels per line in image */
                        int Data_size,  /* points per line in data */
                        Gdev *gdev, /* Pointer to SISCO colormap */
                        Siscms_type cmsindex, /* Index of SISCO colormap */
                        float Vs);   /* Vertical scale to apply to data */

static void raw_line_fill_init(
	int Line_size,  	/* pixels per line in image */
        int Data_size,  	/* points per line in data */
	int pixel_offset,	/* pixel offset */
	int num_gray,		/* number gray level */
        float Vs);   		/* Vertical scale to apply to data */

static void line_same (short *indata, char *outdata);
static void line_expand (short *indata, char *outdata);
static void line_compress (short *indata, char *outdata);
static short *gettrace(short *start,           /* start of data array */
                int width,              /* width of data array */
                int x_offset,           /* X offset into array */
                int trace);             /* trace # to retrieve */

static Flag raw_flag=FALSE;/* use to indicate whether to obtain X11 pixel */
			/* or regular user pixel */
static int   line_size;  /* number of image points in an image line */
static int   data_size;  /* number of data points available to fill image line */
static int   D1, D2; /* decision-variable deltas for expanding/compressing */
static int   g_max_gray;
static int   g_min_gray;
static u_long *x_color;
static float vs;

/* pointer to function to use for filling out image lines with data points */
static void (*LineFill)(short *indata, char *outdata);
/* end of function declarations, start of code */

extern int smooth_flag ;

void
g_display_short_image(Gdev *gdev, 	/* pointer to graphics device */
		Siscms_type cms_index,	/* index of siscms Colormap to use */
		short *data, 		/* pointer to data points */
                int points_per_line,    /* data points per scan line */
		int scan_lines,		/* # of scan lines in data */
                int src_x,              /* x offset into source data */
                int src_y,              /* y offset into source data */
                int src_width,          /* width in data set, must be */
                                        /* <= points_per_line */
                int src_height,         /* height to draw in data set */
		int dest_x, 		/* x destination on canvas */
		int dest_y,		/* y destination on canvas */
		int pix_width,		/* pixel width */
		int pix_height,		/* pixel height */
                Orientation direction,  /* end from which data encoding */
                                        /* starts in data set */
		float vs)		/* vertical scaling of data */
{
    char 	*pix_data;
    int		prev_op;	/* X-lib op to store while drawing */
    XImage 	*ximage;
    int         smooth_x, smooth_y ;
    short       *new_data ;

    /* Do some checking on function inputs */
    if (gdev == (Gdev *) NULL)
    {
	WARNING_OFF(Sid);
	STDERR("g_display_image2: passed NULL gdev pointer");
	return;
    }

    if (data == (short *) NULL)
    {
	STDERR("g_display_image2: passed NULL data pointer");
	return;
    }

    /* check the direction value */
    if ((direction == LEFT) || (direction == RIGHT))
    {
	STDERR("g_display_image2: LEFT or RIGHT orientation not supported");
	return;
    }

    /* malloc space for pixel data */
    if ((pix_data = (char *) malloc(pix_width * pix_height)) == NULL)
    {
	STDERR("g_display_short_image: pix_data malloc failed");
	return;
    }

       
    if (smooth_flag)
    {
      if ((new_data = (short *)malloc(pix_width * pix_height * 2)) == NULL)
      {
          STDERR("g_display_short_image: new_data malloc failed");
          return;
      }
      g_short_to_short (data, new_data, points_per_line, scan_lines, src_x,
                        src_y, src_width, src_height, pix_width, pix_height);

      points_per_line = pix_width ;

      scan_lines = pix_height ;
      src_x = 0 ;
      src_y = 0 ;
      src_width = pix_width ;
      src_height = pix_height ;

      /* smooth top and bottom lines */
      for (smooth_x = 1 ; smooth_x < pix_width - 1 ; smooth_x++)
      {
        new_data [smooth_x] =
        ( new_data [smooth_x + 1] + new_data [smooth_x - 1] ) / 2 ;
         new_data [((pix_height-1) * pix_width) + smooth_x ] =
        ( new_data [((pix_height-1) * pix_width) + smooth_x + 1] +
         new_data [((pix_height-1) * pix_width) + smooth_x - 1] ) / 2 ;
      }
 
      /* smooth leftmost and rightmost lines */
      for (smooth_y = 1 ; smooth_y < pix_height -1; smooth_y++)
      {
          new_data [ (smooth_y * pix_width) ] =
          ( new_data [ ( (smooth_y-1) * pix_width ) ] +
            new_data [ ( (smooth_y+1) * pix_width ) ] ) / 2 ;
          new_data [ (smooth_y * pix_width) + (pix_width-1) ] =
          ( new_data [ ( (smooth_y-1) * pix_width ) + (pix_width-1) ] +
            new_data [ ( (smooth_y+1) * pix_width ) + (pix_width-1) ] ) / 2 ;
      }
 
      /* smooth the rest */
      for (smooth_y = 1 ; smooth_y < pix_height - 1 ; smooth_y++)
        for (smooth_x = 1 ; smooth_x < pix_width - 1 ; smooth_x++)
        {
          new_data [ (smooth_y * pix_width) + smooth_x ] =
          ( new_data [ ( smooth_y * pix_width ) + smooth_x + 1 ] +
            new_data [ ( smooth_y * pix_width ) + smooth_x - 1 ] ) / 2 ;
          new_data [ (smooth_y * pix_width) + smooth_x ] =
          ( new_data [ ( (smooth_y-1) * pix_width ) + smooth_x ] +
            new_data [ ( (smooth_y+1) * pix_width ) + smooth_x ] ) / 2 ;
        }
 
      /* initialize the line-fill routine*/
      line_fill_init (pix_width, src_width, gdev, cms_index, vs);
 
      /* scale and convert data to pixel data */
     convert_short_data(new_data, pix_data, points_per_line, scan_lines, src_x,
          src_y, src_width, src_height, pix_width, pix_height, direction);
 
      free ((char *) new_data) ;
    }
    else
    {
      /* initialize the line-fill routine */
      line_fill_init (pix_width, src_width, gdev, cms_index, vs);

      /* scale and convert data to pixel data */
      convert_short_data(data, pix_data, points_per_line, scan_lines, src_x, 
	  src_y, src_width, src_height, pix_width, pix_height, direction);
    }
   
    /* create the X-Windows image */
    ximage = XCreateImage(gdev->xdpy,
        DefaultVisual(gdev->xdpy, DefaultScreen(gdev->xdpy)),
        DefaultDepth(gdev->xdpy, DefaultScreen(gdev->xdpy)),
        ZPixmap, 0, pix_data, pix_width, pix_height, 8, 0);

    if (ximage == (XImage *) NULL)
    {
	STDERR("g_display_image2:XCreateImage returned NULL pointer");
	/* you have to release malloc'ed memory if you are not going to call */
	/* XDestroyImage() later. */
	free(pix_data);
	return;
    }

    /* store the current type of draw operation */
    prev_op = G_Get_Op(gdev);

    /* set the type of draw operation */
    G_Set_Op(gdev, GXcopy);

    /* draw the X-Windows image */
    XPutImage(gdev->xdpy, gdev->xid, gdev->xgc, ximage, 0, 0, dest_x, dest_y,
	pix_width, pix_height);
    
    /* restore the original X draw operation */
    G_Set_Op(gdev, prev_op);

    /* destroy the X-Windows image */
    XDestroyImage(ximage);
}	/* end of g_display_image2() */

void
g_short_to_pixel(short *indata, /* pointer to input data */
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
      STDERR("g_short_to_pixel: passed NULL data pointer");
      return;
   }

   /* initialize the line-fill routine */
   raw_line_fill_init(pix_width, src_width, pixel_offset, 
		      num_pixel_level, vs);

   raw_flag = TRUE;

   /* scale and convert data to pixel data */
   convert_short_data(indata, outdata, points_per_line, scan_lines, x_offset, 
	y_offset, src_width, src_height, pix_width, pix_height, direction);

   raw_flag = FALSE;
} /* end of function g_short_to_pixel */


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

static void line_same (short *indata, char *outdata)
{
    /**************************************************************************
    LOCAL VARIABLES:
 
    p_in     A fast pointer to the data being loaded into the image line.
    p_out    A fast pointer to the start of the image line.
    max_gray A fast variable to hold the maximum gray-level value.
    min_gray A fast variable to hold the minimum gray-level value.
    cmindex  A fast variable for testing the colormap index value.
    i        A counter for the length of the image line.
    */
    register short *p_in  = indata;
    register char *p_out = outdata;
    register short  max_gray = g_max_gray;
    register short  min_gray = g_min_gray;
    register int  cmindex;
    register int    i;
 
    if (raw_flag)
    {
       for (i = line_size; i > 0; --i, ++p_in, ++p_out)
       {
           /* convert a data value to a colormap index, and */
	   /* test against limits */
    	   if ((cmindex = (int) (vs * (*p_in) + min_gray)) > max_gray)
	       *p_out = (u_char)max_gray;
	    else
	       *p_out = (u_char)cmindex;
       }
    }
    else
    {
       for (i = line_size; i > 0; --i, ++p_in, ++p_out)
       {
           /* convert a data value to a colormap index, and */
	   /* test against limits */
    	   if ((cmindex = (int) (vs * (*p_in) + min_gray)) > max_gray)
	       cmindex = max_gray;
	   *p_out = (u_char) x_color[cmindex];
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

static void line_expand (short *indata, char *outdata)
{
    /**************************************************************************
    LOCAL VARIABLES:
 
    p_in     A fast pointer to the data being loaded into the image line.
    p_out    A fast pointer to the start of the image line.
    max_gray A fast variable to hold the maximum gray-level value.
    min_gray A fast variable to hold the minimum gray-level value.
    dv       The decision variable (see Foley & van Dam).
    cmindex  A fast variable for testing the converted gray-level value.
    i        A counter for the length of the image line.
    */
    register short *p_in  = indata;
    register char *p_out = outdata;
    register short  max_gray = g_max_gray;
    register short  min_gray = g_min_gray;
    register int    dv;
    register int  cmindex;
    register int    i;

    /* check the imput and output buffer pointers */
    if (indata = (short *) NULL)
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
    dv = D1 - line_size;

    /* convert a data value to a colormap index, and test against limits */
    if ((cmindex = (int) (vs * (*p_in) + min_gray)) > max_gray)
	cmindex = max_gray;

    if (raw_flag)
    {
       for (i = line_size; i > 0; --i, ++p_out)
       {
	  /* load the data value into the output buffer */
	  *p_out = (u_char) cmindex;

	  /* adjust the decision variable */
	  if (dv < 0)
              dv += D1;
	  else
	  {
              dv += D2;

              /* move to the next input point */
              ++p_in;

    	      /* convert a data value to a colormap index, */
	      /* and test against limits */
    	      if ((cmindex = (int) (vs * (*p_in) + min_gray)) > max_gray)
		  cmindex = max_gray;
	   }
	}
    }
    else
    {
       for (i = line_size; i > 0; --i, ++p_out)
       {
	  /* load the data value into the output buffer */
	  *p_out = (u_char) x_color[cmindex];

	  /* adjust the decision variable */
	  if (dv < 0)
              dv += D1;
	  else
	  {
              dv += D2;

              /* move to the next input point */
              ++p_in;

    	      /* convert a data value to a colormap index, */
	      /* and test against limits */
    	      if ((cmindex = (int) (vs * (*p_in) + min_gray)) > max_gray)
		  cmindex = max_gray;
	   }
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

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static void line_compress (short *indata, char *outdata)
{
    /**************************************************************************
    LOCAL VARIABLES:
  
    p_in     A fast pointer to the data being loaded into the image line.
    p_out    A fast pointer to the start of the image line.
    max_gray A fast variable to hold the maximum gray-level value.
    min_gray A fast variable to hold the minimum gray-level value.
    dv       The decision variable (see Foley & van Dam).
    cmindex  A fast variable for testing the colormap index value.
    i        A counter for the number of data points to load.
    max_val  The maximum value of an input data point, which causes the
            maximum of sequential deleted data points to be loaded.
    */
    register short *p_in  = indata;
    register char *p_out = outdata;
    register short  max_gray = g_max_gray;
    register short  min_gray = g_min_gray;
    register int    dv;
    register int  cmindex;
    register int    i;
    register short  max_val = (short)0;

    /* set the starting value of the decision variable */
    dv = D1 - data_size;

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

    	       /* convert a data value to a colormap index, and test */
	       /* against limits */
    	       if ((cmindex = (int) (vs * max_val + min_gray)) > max_gray)
		  *p_out++ = (u_char)max_gray;
	       else
	          *p_out++ = (u_char)cmindex;

               max_val = (short)0;
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

    	       /* convert a data value to a colormap index, and test */
	       /* against limits */
    	       if ((cmindex = (int) (vs * max_val + min_gray)) > max_gray)
		   cmindex = max_gray;

               /* load the data value into the output buffer, and move to the
               next point */ 
	       *p_out++ = (u_char) x_color[cmindex];

               max_val = (short)0;
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
 convert_short_data

 This function converts short data to n-bit pixel data, depending
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

static int convert_short_data (
			short *indata, 
			char *outdata, 
			int x_data, 
			int y_data, 
			int x_offset,
			int y_offset,
			int width,
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
    static char   	sub_msg[] = "convert_short_data:";
    int    		trace;
    short 		*max_val;
    int    		dv, d1, d2;
    register char 	*p_out;
    int    		i;
    register short 	*trace_ptr;

    WARNING_OFF(width);

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
		    	trace)) == (short *) NULL)
            	    return (NOT_OK);

         	(*LineFill)(trace_ptr, p_out);
         	p_out += x_image;
      	    }
	}
	else if (direction == BOTTOM)
	/* Vnmr data starts at bottom scan line and comes up */
	{
      	    for (trace = y_data-1 - y_offset; trace > y_data-1 - y_offset -
		height; trace--)
      	    {
         	if ( (trace_ptr = gettrace (indata, x_data, x_offset,
		    	trace)) == (short *) NULL)
            	    return (NOT_OK);

         	(*LineFill)(trace_ptr, p_out);
         	p_out += x_image;
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
	dv = d1 - y_image;

	if (direction == TOP)
	{
	    /* top down direction */
            trace_ptr = NULL;
            for (i = 0, trace = y_offset; i < y_image; i++, p_out += x_image)
	    {
             	/* adjust the decision variable */
             	if (dv < 0)
		{
                    dv += d1;
                    if (trace_ptr)
                    {
                       (void)memcpy(p_out, p_out-x_image, x_image *
                       sizeof(*p_out));
                    }
                    else
                    {
                       /* Only get executed once (at the most)*/
                       if ( (trace_ptr = gettrace (indata, x_data, x_offset,
                           trace)) == (short *)NULL)
                               return (NOT_OK);
                       /* load the data trace into the output buffer */
                       (*LineFill)(trace_ptr, p_out);
                    }
		}
             	else
             	{
                    dv += d2;
 
	    	    if ( (trace_ptr = gettrace (indata, x_data, x_offset,
			trace++)) == (short *) NULL)
            	    	    return (NOT_OK);
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
            for (i = y_image, trace = y_data - y_offset; i > 0; --i, p_out += x_image)
	    {
             	/* adjust the decision variable */
             	if (dv < 0)
		{
                    dv += d1;
                    if (trace_ptr)
                    {
                       (void)memcpy(p_out, p_out-x_image, x_image *
                       sizeof(*p_out));
                    }
                    else
                    {
                       /* Only get executed once (at the most)*/
                       if ( (trace_ptr = gettrace (indata, x_data, x_offset,
                           trace-1)) == (short *)NULL)
                               return (NOT_OK);
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
			trace)) == (short *) NULL)
            	    	    return (NOT_OK);
                    /* load the data trace into the output buffer */
                    (*LineFill)(trace_ptr, p_out);
             	}
             }	/* end of for all display scan lines */
	}	/* end of if display inverted data set */
    }		/* end of if expand scan lines */
    /* compress (delete some of) the data traces to fit into the display lines */
    else
    {
      	/* allocate a maximum buffer */
      	if ((max_val=(short *)calloc((uint)x_data,sizeof(short))) == (short *)NULL)
      	{
	    STDERR("convert_short_data: can't allocate memory");
            return (NOT_OK);
      	}
      	/* set the adjustment values for the decision variable */
      	d1 = 2*y_image;
      	d2 = d1 - 2 * height;
 
      	/* set the starting value of the decision variable */
      	dv = d1 - height;

	if (direction == TOP)
	{
      	    /* starting with the first data trace, load the display lines */
      	    for (trace = y_offset; trace < y_offset + height; trace++)
      	    {
         	if ( (trace_ptr = gettrace (indata, x_data, x_offset,
                	trace)) == (short *)NULL)
            	    return (NOT_OK);

         	/* load this trace into the maximum buffer */
         	for (i = 1; i <= x_data; ++i)
            	    if (*(trace_ptr+i) > *(max_val+i))
               		*(max_val+i) = *(trace_ptr+i);
 
         	/* adjust the decision variable */
         	if (dv < 0)
            	    dv += d1;
         	else
         	{
            	    dv += d2;

            	    /* load the maximum buffer into the output buffer */
            	    (*LineFill)(max_val, p_out);
            	    p_out += x_image;
 
            	    /* clear the maximum buffer */
            	    (void)memset ((char *)max_val, '\0', x_data * sizeof(short));
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
                	trace)) == (short *)NULL)
            	    return (NOT_OK);

         	/* load this trace into the maximum buffer */
         	for (i = 1; i <= x_data; ++i)
            	    if (*(trace_ptr+i) > *(max_val+i))
               		*(max_val+i) = *(trace_ptr+i);
 
         	/* adjust the decision variable */
         	if (dv < 0)
            	    dv += d1;
         	else
         	{
            	    dv += d2;

            	    /* load the maximum buffer into the output buffer */
            	    (*LineFill)(max_val, p_out);
            	    p_out += x_image;
 
            	    /* clear the maximum buffer */
            	    (void)memset ((char *)max_val, '\0', x_data * sizeof(short));
         	}
	    }		/* end of for all traces */
	}		/* end of if display inverted data */
      	free ((char *)max_val);
    }

    return (OK);

}  /* end of function "convert_short_data" */

static short *gettrace(short *start, int width, int x_offset, int trace)
{
    if (x_offset > width)
	return ((short *) NULL);

    return (start + trace * width + x_offset);
}	/* end of gettrace() */
