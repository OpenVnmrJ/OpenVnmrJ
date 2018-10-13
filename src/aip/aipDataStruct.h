/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef AIPDATASTRUCT_H
#define AIPDATASTRUCT_H

#include <sys/param.h>

#ifndef MAXPATHLEN
#define MAXPATHLEN (1024)
#endif

#define MAXRANK 4		/* Max dimensions in data */
#define RFCHANS 6
typedef struct _dataStruct {
    /*
     * DATA DESCRIPTION
     *
     * Header entries are compatible with the FDF header values.
     * See the "User Programming" manual on the FDF file format for
     * additional details.
     * Note that "data" is always an array of "float"s.
     */
    float *data;		/* matrix[0] * matrix[1] *... array of data */
    char filepath[MAXPATHLEN];
    char type[16];		/* E.g., "absval", "real", "imag", "complex" */
    char spatial_rank[16];	/* E.g., "2dfov", "3dfov", "voxel", "none" */
    char storage[16];		/* E.g., "float" */
    int order;                  /* Display order of image within group */
    int bits;			/* Bits per word, e.g., 32 */
    int rank;			/* Dimensionality of the data (<= MAXRANK) */
    int matrix[MAXRANK];	/* Number of points in each dim (fastest 1st) */
    char abscissa[MAXRANK][16];	/* Units of each dim (e.g., "cm", "ppm1") */
    char ordinate[16];		/* Units of data (e.g., "intensity") */
    double span[MAXRANK];	/* Size of data set in "abscissa" units */
    double origin[MAXRANK];	/* Distance from user origin to 1st data pnt */
    double orientation[9];	/* Direction cosines of user coord system
				 * relative to magnet frame */
    double location[3];		/* Position of data volume in magnet (cm);
				 * orientation of coords is "orientation" */
    double roi[3];		/* Size of data volume (cm) */
    double euler[3];
    char planeOrient[16];       // "xy","xz","yz" for extracted planes, otherwise empty

    /* The following are used when an abscisssa is "ppm#".  E.g., "ppm2" means
     * to use ppm units using the second nucleus in the list (nucleus[1]).
     */
    char nucleus[RFCHANS][16];	/* tn for each rf chan (e.g., {"H1","C13"}) */
    double nucfreq[RFCHANS];	/* Freqs corresponding to tn values */

    /*
     * Extra info that the user may want associated with the data.
     * A null-terminated list of strings.  Each string is a parameter
     * name followed by the value, like:
     *   "time 1.25"
     *   "diagnosis Diagnosis is acute boredom."
     *
     * The list of pointers and also each string must be malloc'ed memory;
     * auxparms must be NULL if there is no list.
     */
    char **auxparms;
} dataStruct_t;
#undef MAXRANK
#undef RFCHANS

#endif /* (not) AIPDATASTRUCT_H */
