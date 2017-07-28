/*
 * Copyright (C) 2017  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
//============================================================================
// Name        : ConvertFieldToTag.cpp
// Author      : Varian, Inc.
// Version     : 1.0
// Description : The functions contained in this source file will parse FDF
//				 fields into binary values and store them into an internal
//				 structure.  These values will later be converted into DICOM
//				 tags.
//============================================================================

#include <fstream>
#include <iostream>
using namespace std;

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include "attrtype.h"
#include "attrothr.h"
#include "attrmxls.h"
#include "transynu.h"
#include "elmconst.h"
#include "mesgtext.h"
#include "ioopt.h"
#include "dcopt.h"
#include "rawsrc.h"

#include "FdfHeader.h"

// Converts an FDF floating point array string to a binary array
static void fillFloatArray(float *outArray, char *inString)
{
	int		i = 0;
	char 	*pch;

	pch = strtok(inString, "{,}");
	while(pch != NULL)
	{
		outArray[i++] = atof(pch);
		pch = strtok(NULL, "{,}");
	}
}

// Converts an FDF string array to a string array
static void fillStringArray(char *outArray, char *inString, int elementSize)
{
	int		i = 0;
	char	*pch;

	pch = strtok(inString, "{,\" }");
	while(pch != NULL)
	{
		strcpy(outArray, pch);
		pch = strtok(NULL, "{,\" }");
		outArray += elementSize;
	}
}

// Search for, validate and convert an FDF field to internal
// binary representation of the appropriate type.  Types
// include int, float, string, int array, float array and
// string array.
extern int convertFdfFieldToTag(char *fieldType, char *fieldName,
		char *fieldValue, fdfEntries_t *fdfValues)
{
	int				i = 0;
	char			*pch;

	// Find the tag entry we're trying to process in the known fields
	for(i = 0 ; fdfInput[i].name[0] != '\0' ; i++)
	{
		if(strcmp(fieldName, fdfInput[i].name) == 0)
		{
			break;
		}
	}
	// If we didn't find a matching entry, return an error
	if(fdfInput[i].name == NULL)
	{
		printf("Uknown field found, ignoring...\n");
		return 1;
	}

	// Now figure out which type of entry we have and process it based on known types
	if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "rank") == 0))
	{
		fdfValues->rank = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "*spatial_rank") == 0))
	{
		strcpy(fdfValues->spatial_rank, fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "*storage") == 0))
	{
		strcpy(fdfValues->storage, fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "bits") == 0))
	{
		fdfValues->bits = atoi(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "*type") == 0))
	{
		strcpy(fdfValues->type, fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "matrix[]") == 0))
	{
		fillFloatArray(fdfValues->matrix, fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "*abscissa[]") == 0))
	{
		fillStringArray((char *)fdfValues->abscissa, fieldValue, sizeof(fdfValues->abscissa[0]));
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "*ordinate[]") == 0))
	{
		fillStringArray((char *)fdfValues->ordinate, fieldValue, sizeof(fdfValues->ordinate[0]));
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "span[]") == 0))
	{
		fillFloatArray(fdfValues->span, fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "origin[]") == 0))
	{
		fillFloatArray(fdfValues->origin, fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "*nucleus[]") == 0))
	{
		fillStringArray((char *)fdfValues->nucleus, fieldValue, sizeof(fdfValues->nucleus[0]));
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "nucfreq[]") == 0))
	{
		fillFloatArray(fdfValues->nucfreq, fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "location[]") == 0))
	{
		fillFloatArray(fdfValues->location, fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "roi[]") == 0))
	{
		fillFloatArray(fdfValues->roi, fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "orientation[]") == 0))
	{
		fillFloatArray(fdfValues->orientation, fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "gap") == 0))
	{
		fdfValues->gap = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "*array_name") == 0))
	{
		strcpy(fdfValues->array_name, fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "*file") == 0))
	{
		if(fdfValues->file[0] == 0)
		{
			strcpy(fdfValues->file, fieldName);
		}
		else
		{
			strcpy(fdfValues->file2, fieldName);
		}
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "slice_no") == 0))
	{
		fdfValues->slice_no = atoi(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "slices") == 0))
	{
		fdfValues->slices = atoi(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "echo_no") == 0))
	{
		fdfValues->echo_no = atoi(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "echoes") == 0))
	{
		fdfValues->echoes = atoi(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "TE") == 0))
	{
		fdfValues->TE = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "te") == 0))
	{
		fdfValues->te = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "TR") == 0))
	{
		fdfValues->TR = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "tr") == 0))
	{
		fdfValues->tr = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "ro_size") == 0))
	{
		fdfValues->ro_size = atoi(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "pe_size") == 0))
	{
		fdfValues->pe_size = atoi(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "*sequence") == 0))
	{
		strcpy(fdfValues->sequence, fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "*studyid") == 0))
	{
		strcpy(fdfValues->studyid, fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "*position1") == 0))
	{
		strncpy(fdfValues->position1, &fieldValue[1], strlen(fieldValue)-2);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "*position2") == 0))
	{
		strncpy(fdfValues->position2, &fieldValue[1], strlen(fieldValue)-2);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "TI") == 0))
	{
		fdfValues->TI = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "ti") == 0))
	{
		fdfValues->ti = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "array_index") == 0))
	{
		fdfValues->array_index = atoi(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "array_dim") == 0))
	{
		fdfValues->array_dim = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "image") == 0))
	{
		fdfValues->image = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "imagescale") == 0))
	{
		fdfValues->imagescale = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "display_order") == 0))
	{
		fdfValues->display_order = atoi(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "bigendian") == 0))
	{
		fdfValues->bigendian = atoi(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "psi") == 0))
	{
		fdfValues->psi = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "phi") == 0))
	{
		fdfValues->phi = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "theta") == 0))
	{
		fdfValues->theta = atof(fieldValue);
	}
	else if((strcmp(fieldName, fdfInput[i].name) == 0) && (strcmp(fieldName, "checksum") == 0))
	{

	}
	else
	{
		// If we didn't find the name of the field passed in, the field is currently
		// unknown and an error is returned.
		printf("Unable to find matching field, ignoring...\n");
		return 1;
	}
//	printf("Field: %s  Name: %s  Value:  %s\n", fieldType, fieldName, fieldValue);
	return 0;
}

