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
*************************************************************************/

#ifndef vscale_h
#define vscale_h

#include "vsfunc.h"

/* This function create the vscale command frame, panel and canvas, */ 
/* initialized to the variables indicated by the user. */ 
void vs_win_create(Frame owner,
		   int x,	/* x location of window */
		   int y,	/* y location of window */
		   int wd,	/* width of canvas */
		   int ht,	/* height of canvas */
		   Siscms *siscms, /* SIS color map */
		   int gs_start, /* start index of greyscale map */
		   int gs_num); /* end index of greyscale map */


/* This function registers the vscale function in the vscale module. */ 
/* This is the function that gets called when the user changes the */
/* vscale in the window */
void vs_notify_func(void (*vs_proc)(VsFunc *));

void vs_win_show(void);
void set_vscale(float datamax);
void set_vscale(float datamax, float datamin);
VsFunc *get_default_vsfunc();

#endif /* vscale_h */
