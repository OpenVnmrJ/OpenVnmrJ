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
* Convert short data (12 bits) of size wd1 x ht1 to short data (12 bits)*
* of size wd2 x ht2 by expanding or compressing the input data.  	*
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
        short *outdata,          /* pointer to output pixel data */
        int points_per_line,    /* # data points per scan line (width)*/
        int scan_lines,         /* scan lines (height of) data */
        int x_offset,           /* x offset in data to start at */
        int y_offset,           /* y offset in data to start at */
        int src_width,          /* width of data to use */
        int src_height,         /* height of data to use */
        int pix_width,          /* (resulting) image width in pixels */
        int pix_height);         /* (resulting) image height in pixels */

static void line_fill_init(
	int Line_size,  	/* pixels per line in image */
        int Data_size);  	/* points per line in data */

static void line_same (short *indata, short *outdata);
static void line_expand (short *indata, short *outdata);
static void line_compress (short *indata, short *outdata);
static short *gettrace(short *start,           /* start of data array */
                int width,              /* width of data array */
                int x_offset,           /* X offset into array */
                int trace);             /* trace # to retrieve */

static int   line_size;  /* number of image points in an image line */
static int   data_size;  /* number of data points available to fill image line */
static int   D1, D2; /* decision-variable deltas for expanding/compressing */

/* pointer to function to use for filling out image lines with data points */
static void (*LineFill)(short *indata, short *outdata);


void
g_short_to_short(short *indata, /* pointer to input data */
        short *outdata,          /* pointer to output pixel data */
        int points_per_line,    /* # data points per scan line (width)*/
        int scan_lines,         /* scan lines (height of) data */
        int x_offset,           /* x offset in data to start at */
        int y_offset,           /* y offset in data to start at */
        int src_width,          /* width of data to use */
        int src_height,         /* height of data to use */
        int pix_width,          /* (resulting) data width */
        int pix_height)         /* (resulting) data height */
{
   if ((indata == NULL) || (outdata == NULL))
   {
      WARNING_OFF(Sid);
      STDERR("g_short_to_short: passed NULL data pointer");
      return;
   }

   /* initialize the line-fill routine */
   line_fill_init(pix_width, src_width);

   convert_short_data(indata, outdata, points_per_line, scan_lines,
     x_offset, y_offset, src_width, src_height, pix_width, pix_height);

} /* end of function g_short_to_short */


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

static void line_same (short *indata, short *outdata)
{
    /**************************************************************************
    LOCAL VARIABLES:
 
    p_in     A fast pointer to the data being loaded into the image line.
    p_out    A fast pointer to the start of the image line.
    i        A counter for the length of the image line.
    */
    register short *p_in  = indata;
    register short *p_out = outdata;
    register int    i;
 
    for (i = line_size; i > 0; --i, ++p_in, ++p_out)
       *p_out = *p_in;

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

static void line_expand (short *indata, short *outdata)
{
    /**************************************************************************
    LOCAL VARIABLES:
 
    p_in     A fast pointer to the data being loaded into the image line.
    p_out    A fast pointer to the start of the image line.
    dv       The decision variable (see Foley & van Dam).
    i        A counter for the length of the image line.
    */
    register short *p_in  = indata;
    register short *p_out = outdata;
    register int    dv;
    register int    i;

    /* check the imput and output buffer pointers */
    if (indata = (short *) NULL)
    {
	STDERR("line_expand: passed NULL indata pointer");
	return;
    }

    if (outdata = (short *) NULL)
    {
	STDERR("line_expand: passed NULL outdata pointer");
	return;
    }

    /* set the starting value of the decision variable */
    dv = D1 - line_size;

    for (i = line_size; i > 0; --i)
    {
       /* load the data value into the output buffer */
       *p_out++ = *p_in;

       /* adjust the decision variable */
       if (dv < 0)
          dv += D1;
       else
       {
          dv += D2;
          ++p_in;
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

static void line_compress (short *indata, short *outdata)
{
    /**************************************************************************
    LOCAL VARIABLES:
  
    p_in     A fast pointer to the data being loaded into the image line.
    p_out    A fast pointer to the start of the image line.
    dv       The decision variable (see Foley & van Dam).
    i        A counter for the number of data points to load.
    max_val  The maximum value of an input data point, which causes the
            maximum of sequential deleted data points to be loaded.
    */
    register short *p_in  = indata;
    register short *p_out = outdata;
    register int    dv;
    register int    i;
    register short  max_val = (short)0;

    /* set the starting value of the decision variable */
    dv = D1 - data_size;

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
	    *p_out++ = max_val;
            max_val = (short)0;
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
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 GLOBALS USED:
   data_size   The number of data points available for an image line.
   line_size   The number of image points in an image line.
   D1, D2      Decision-variable deltas used for expanding/compressing the
               data points to fill out an image line (see Foley & van Dam).
   LineFill    A pointer to the function to use for filling out image lines
               with data points.
 GLOBALS CHANGED:
   line_size   The number of image points in an image line.
   data_size   The number of data points available for an image line.
   D1, D2      Decision-variable deltas used for expanding/compressing the
               data points to fill out an image line (see Foley & van Dam).
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

static void line_fill_init(
	int Line_size,  	/* pixels per line in image */
        int Data_size)  	/* points per line in data */
{
    /* set the sizes of the input and output lines */
    line_size = Line_size;
    data_size  = Data_size;

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

 This function converts short data (12 bits) to short (12 bits) data

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
			short *outdata, 
			int x_data, 
			int y_data, 
			int x_offset,
			int y_offset,
			int width,
			int height,
			int x_image,
			int y_image)
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
    short               *max_val;
    int    		dv, d1, d2;
    register short 	*p_out;
    int    		i;
    register short 	*trace_ptr;

    WARNING_OFF(width);
    WARNING_OFF(y_data);

    /* set a fast pointer to the output buffer */
    p_out = outdata;

    /* Build an image from the data: there are 3 possibilities:
      the data traces exactly fit the number of display lines */
    if (height == y_image)
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
    /* expand (repeat some of) the data traces to fill out the display lines */
    else if (height < y_image)
    {
	/* set the adjustment values for the decision variable */
	d1 = 2 * height;
	d2 = d1 - (2 * y_image);
 
	/* set the starting value of the decision variable */
	dv = d1 - y_image;

	/* top down direction */
        trace_ptr = NULL;
        for (i = 0, trace = y_offset; i < y_image; i++, p_out += x_image)
	{
           /* adjust the decision variable */
           if (dv < 0)
	   {
              dv += d1;
              if (trace_ptr)
                 (void)memcpy(p_out, p_out-x_image, x_image*sizeof(*p_out));
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
