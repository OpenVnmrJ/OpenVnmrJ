/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/***********************************************************************
* THIS FILE NO LONGER USED.  The information contained herein has been *
* ------------------------   transferred to graphics.h.		       *
***********************************************************************/

/*****************************************************************************
* File gdevsw.h:  Contains data structures used for switching among graphics *
*      --------   functions, based on what output device is in use.          *
*****************************************************************************/

/* Here is a stupendously clever programming trick (warning) */
#ifdef FILE_IS_GDEVSW.C
#define uextern
#define vextern extern
#else
#ifdef FILE_IS_GRAPHICS.C
#define uextern extern
#define vextern
#else
#define uextern extern
#define vextern extern
#endif
#endif

/* The following items allow switching between different terminal types at    */
/* boot time, rather than every time a graphics function is executed.         */

vextern int (*_setdisplay)();		/* Pointer to correct setdisplay fcn  */
vextern int (*_coord0)();		/* Pointer to correct coord0 fcn      */
vextern int (*_sunGraphClear)();	/* Pointer to correct sunGraph... fcn */
vextern int (*_grf_batch)();		/* Pointer to correct grf_batch fcn   */
vextern int (*_sun_window)();		/* Pointer to correct sun_window fcn  */
vextern int (*_change_contrast)();	/* Pointer to correct change_c... fcn */
vextern int (*_change_color)();		/* Pointer to correct change_c... fcn */
vextern int (*_usercoordinate)();	/* Pointer to correct usercoor... fcn */

/* The following items allow switching between different terminal, printer,   */
/* and plotter devices when that switch is made, rather than each time a      */
/* graphics function is called.						      */

struct gdevsw {
   int (*_endgraphics)();
   int (*_graf_clear)();
   int (*_color)();
   int (*_charsize)();
   int (*_amove)();
   int (*_rdraw)();
   int (*_adraw)();
   int (*_dchar)();
   int (*_dstring)();
   int (*_dvchar)();
   int (*_dvstring)();
   int (*_ybars)();
   int (*_normalmode)();
   int (*_xormode)();
   int (*_grayscale_box)();
   int (*_box)();
};

#define C_PLOT 		0	/* Indicates plotting, but not raster */
#define C_RASTER 	1	/* Indicates raster plotting	      */
#define C_TERMINAL	2	/* Indicates output to terminal       */
#define MGDEV C_TERMINAL+1 


uextern struct gdevsw gdevsw_array[MGDEV];   /* Array of structures of    */
					     /* pointers to graphics      */
					     /* functions.  Set at bootup */
					     /* time.			  */
uextern struct gdevsw *active_gdevsw;	/* Pointer to an entry of the above   */
					/* array.  Set when changing among    */
					/* plot, raster, and terminal output. */

#undef uextern
#undef vextern
