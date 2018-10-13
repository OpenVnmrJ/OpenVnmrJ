/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  Group definitions */

#define G_ALL		0
#define G_SAMPLE	1
#define G_ACQUISITION	2
#define G_PROCESSING	3
#define G_DISPLAY	4
#define G_SPIN		5

#define ACT_NOTEXIST	-1
#define ACT_OFF		0
#define ACT_ON		1

/* tree definitions */
#define NOTREE		-1
#define GLOBAL 		0
#define CURRENT		1
#define PROCESSED	2
#define TEMPORARY	3
#define SYSTEMGLOBAL    4
#define USERTREE        5

/* Display groups */

#define ALL                     0
#define D_ACQUISITION		1
#define D_2DACQUISITION		2
#define D_SAMPLE		3
#define D_DECOUPLING		4
#define D_AFLAGS		5
#define D_PROCESSING		6
#define D_SPECIAL		7

#define D_DISPLAY		8
#define D_REFERENCE		9
#define D_PHASE			10
#define D_CHART			11
#define D_2DDISPLAY		12
#define D_INTEGRAL		13
#define D_DFLAGS		14
#define D_FID			15

#define D_SHIMCOILS		16
#define D_AUTOMATION		17

#define D_NUMBERS		24
#define D_STRINGS		25

/* intmod parameter definitions */
#define INT_OFF		"off"
#define INT_FULL	"full"
#define INT_PARTIAL	"partial"
