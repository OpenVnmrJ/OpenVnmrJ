/*
 * Copyright (C) 2017  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
//============================================================================
// Name        : Procpar.h
// Author      : Varian, Inc.
// Version     : 1.0
// Description : This header file contains definitions and structures which 
//				 will be used to store the information contained in the 
//				 procpar file associated with the study being processed.
//				 Not all fields in the procpar file will be saved.  Only
//  			 those of interest to the FDF conversion will be processed.
//============================================================================

#ifndef PROCPAR_H
#define PROCPAR_H

typedef struct procparEntry
{
	int		type;
	char	name[32];
	;
	float	fvalue;
};
typedef struct procparVals
{
	procparEntry_t	date;
	
} procparVals_t;


#endif	// #ifndef PROCPAR_H
