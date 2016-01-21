/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "ap_device.p"

/* frequency  device local properties */

int rfband;  /* rf band, high  or  low */
int ptsval;  /* PTS type, 160,250,400,500, etc. */
int ptsoptions;  /* PTS Options, latch,over/under range for coherent freq hopping */
int rftype;  /* RF type: a-fixed,b-offsetsyn,c-directsyn,m-image offsetsyn */
int h1freq;  /* proton frequency of instrument: 200,300,400,500,600, etc. */
double freq_stepsize; /* step size that frequency can be changed, units of 1 Hz */
double iffreq;      /* IF freq for offset syn */
double ofsyn_basefrq; /* 1.48 MHz, SISCO 1.5 Mhz */
double ofsyn_constfrq; /* 158.5, 205.5, 20.5 MHZ */
double base_freq;	/* sfrq, dfrq */
double offset_freq;     /* tof, dof */
double init_ptsfreq;	/* first initial PTS frequency setting, for under/over range */
double pts_freq;	/* PTS frequency setting */
double of_freq;		/* offset syn frequency setting */
double init_offset_freq;/* element 1 value of the offset parameter */
double spec_freq;	/* corrected spectrometer frequency at probe */
double overrange;	/* Over/under range value 1e4,1e5 Hz */
int overunderflag;	/* overrange, underrange used on PTS */
int spareoverunderflag;	/* overrange, underrange for spare freq used on PTS */
int codecnt;		/* number acodes generated */
int frq_codes[25];	/* codes generated */
int sweeprtptr;		/* sweep rt parameter pointer 4 swp incr, -1 if abs */
double sweepcenter;	/* sweep center freq Hz */
double sweepwidth;	/* sweep width Hz */
double sweepnp;		/* sweep np */
double sweepincr;	/* sweep increment */
double maxswpwidth;	/* max swp width base on rfband,ptsval and np */
int channel_is_tuned;   /* flag signifying tune A-codes have been sent */
