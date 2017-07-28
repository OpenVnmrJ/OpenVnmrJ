/*
 * Copyright (C) 2017  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
//============================================================================
// Name        : ProcessFdfHeader.cpp
// Author      : Varian, Inc.
// Version     : 1.0
// Description : The functions contained in this source file will read an FDF
//				 header and convert the fields into their DICOM counterparts.
//				 Output from these functions will be a DICOM header that is
//				 complete and ready for pixel data to be appended.
//				 These functions will validate the header for all required
//				 fields and will issue a warning if all fields are not present.
//
//				 Future versions of the software may be modified to strictly
//				 enforce DICOM compliance and return an error rather than just
//				 print a warning.
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

int ProcessFdfHeader(FILE *fd, 	fdfEntries_t *fdfValues)
{
	int			i;
	int			line = 1;
	char		*pch;
	char		tagBuf[512];
	char		fieldType[PATH_MAX];
	char		fieldName[PATH_MAX];
	char		fieldValue[PATH_MAX];

	// Read each field out of the FDF file and convert them to the appropriate
	// DICOM tag and add to the list of tags that will be written to the converted
	// file by the caller.
	do
	{
		bzero(tagBuf, sizeof(tagBuf));
		bzero(fieldType, sizeof(fieldType));
		bzero(fieldName, sizeof(fieldName));
		bzero(fieldValue, sizeof(fieldValue));

		if(fgets(tagBuf, sizeof(tagBuf), fd) == NULL)
		{
			printf("Error reading FDF header: %d\n", errno);
			return(-1);
		}
		if(tagBuf[0] == '\0') break;
		if(isspace(tagBuf[0])) continue;
		if(tagBuf[0] == '#') continue;
		if(tagBuf[0] == '\f') break;
		if(tagBuf[0] == '/') continue;

		// Find the field value and trim any white space or special
		// characters from both ends
		pch = strchr(tagBuf, '=');
		if(pch == NULL) break;
		pch++;

		// Trim the end of the string of white space and the trailing
		// colon and newline
		for(i = strlen(pch) ; pch[i] != ';' ; i--);
		pch[i] = 0;

		// Trim the white space from the front of the string
		while(isspace(pch[0])) pch++;
		strcpy(fieldValue, pch);

		pch = strtok(tagBuf," ");
		while(pch != NULL)
		{
			strcpy(fieldType, pch);

			pch = strtok(NULL, " ");
			if(pch == NULL) break;
			for(i = 0 ; fdfInput[i].datatype != 0 ; i++)
			{
				if(strncmp(pch, fdfInput[i].name,
						strlen(fdfInput[i].name)) == 0)
				{
					strcpy(fieldName, pch);
					// Found an expected field, break out and process the tag
					break;
				}
			}
			if(fdfInput[i].datatype == 0)
			{
				printf("Unexpected field found, ignoring...\n");
				break;
			}

			line++;
			break;
		}
		convertFdfFieldToTag(fieldType, fieldName, fieldValue, fdfValues);
	} while(tagBuf[0] >= ' ');

	return 0;
}

int	FindFDFValue(FILE *fd, char *searchName, char *retptr)
{
	int			i, j, k, l;
	int			ret;
	int			type = 0;
	int			active = 0;
	int			nvals = 0;
	int 		ivalue;
	float		fvalue;
	char		name[128];
	char		*pch;
	char		tagBuf[512];
	char		fieldType[PATH_MAX];
	char		fieldName[PATH_MAX];
	char		fieldValue[PATH_MAX];

	bool		found = false;

	if(fd == NULL)
	{
		fprintf(stderr, "Unable to open FDF file for insertion of custom tag...\n");
		return(1);
	}
	/* start at beginning of fdf file */
	(void)rewind(fd);


	// Read each field out of the FDF file until we find one matching searchName
	do
	{
		bzero(tagBuf, sizeof(tagBuf));
		bzero(fieldType, sizeof(fieldType));
		bzero(fieldName, sizeof(fieldName));
		bzero(fieldValue, sizeof(fieldValue));

		if(fgets(tagBuf, sizeof(tagBuf), fd) == NULL)
		{
			printf("Error reading FDF header: %d\n", errno);
			return(-1);
		}

		if(tagBuf[0] == '\0') break;
		if(isspace(tagBuf[0])) continue;
		if(tagBuf[0] == '#') continue;
		if(tagBuf[0] == '\f') break;
		if(tagBuf[0] == '/') continue;

		// Find the field value and trim any white space or special
		// characters from both ends
		pch = strchr(tagBuf, '=');
		if(pch == NULL) break;
		pch++;

		// Trim the end of the string of white space and the trailing
		// colon and newline
		for(i = strlen(pch) ; pch[i] != ';' ; i--);
		pch[i] = 0;

		// Trim the white space from the front of the string
		while(isspace(pch[0])) pch++;
		strcpy(fieldValue, pch);

		pch = strtok(tagBuf," ");
		strcpy(fieldType, pch);
		pch = strtok(NULL, " ");
		strcpy(fieldName, pch);


		// printf("fieldType is %s fieldName is %s Value is %s \n",fieldType, fieldName, fieldValue);

		//	convertFdfFieldToTag(fieldType, fieldName, fieldValue, fdfValues);
		// See if we found the parameter we are looking for
		if(strcmp(fieldName, searchName) == 0)
		{
		  strcpy(retptr, fieldValue);
		  found=1;

		}

	} while(!found && (tagBuf[0] >= ' '));

	if(found)
	  return(0);
	else
	  return(1);


}
