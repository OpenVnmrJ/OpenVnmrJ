/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/************************************************************************
*
*
*************************************************************************
*
*  Steve York
*  Spectroscopy Imaging Systems Corporation
*  Fremont, CA  94538
*
*************************************************************************/

#ifndef contrast_h
#define contrast_h

/* This function create the contrast command frame, panel and canvas, */ 
/* initialized to the variables indicated by the user. */ 
void
contrast_win_create(Frame owner,
		    int x,          	/* x location of window */
                    int y,              /* y location of window */
		    int wd,		/* width of canvas */
		    int ht,		/* height of canvas */
		    Siscms *siscms,     /* SIS color map */
                    int gs_start,       /* start index of greyscale map */
                    int gs_num);        /* end index of greyscale map */


/* This function registers the contrast function in the contrast module. */ 
/* This is the function that gets called when the user changes the */
/* contrast in the window */
void
contrast_notify_func(long addr_contrast_func);
/* The function would expect the calling prototype */
/* addr_contrast_func(  Siscms *siscms,	SIS color map 
		  	int gs_start,	start index of greyscale map 
			int gs_num);	size of greyscale map
*/

/* This function is registered as the PANEL_NOTIFY_PROC with the button which */
/* calls up the contrast tool. */


/* This function registers the solarization function in the contrast module. */
/* This is the function that gets called when the user changes the */
/* solarization value in the window */
void
solarization_notify_func(long solar_proc);
/* The function would expect the calling prototype */
/* addr_solar_func(int);	TRUE if solarization is displayed
*/


void
contrast_win_show(void);

void
contrast_mask_on(int min_active, int min, int max_active, int max);

void
contrast_mask_off(void);

void
solarization(int flag);

void
set_contrast(float, float, float, float);

void
save_colormap();

void
restore_colormap(float xmin, float xmax);

#endif /* contrast_h */
