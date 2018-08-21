/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include "imagemath.h"

int
mathfunc()
{
    int i;
    int j;
    int imgnbr;
    float ymax;
    
    if (nbr_infiles<1 || input_sizes_differ || !want_output(0)){
	return FALSE;
    }
    create_output_files(2, in_object[0]);

    for (i=0; i<img_size; i++){
	imgnbr = 0;
	ymax = in_data[0][i];
	for (j=1; j<nbr_infiles; j++){
	    if (ymax < in_data[j][i]){
		ymax = in_data[j][i];
		imgnbr = j;
	    }
	}
	out_data[0][i] = ymax;
	if (want_output(1)){
	    out_data[1][i] = imgnbr + 1;
	}
    }
    return TRUE;
}
