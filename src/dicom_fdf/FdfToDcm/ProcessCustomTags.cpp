/*
 * Copyright (C) 2017  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
//============================================================================
// Name        : ProcessFidData.cpp
// Author      : Varian, Inc.
// Version     : 1.0
// Description : The functions contained in this source file will read an FID
//				 raw data file and convert the pixel data into a form that can
//				 be saved into a DICOM file.
//				 Following the processing done in this file, the normal procpar

#include <fstream>
#include <iostream>
using namespace std;

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

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

static char		str1[1024];
static char		str2[1024];

static int InsertCustomElement(unsigned short group, unsigned short element, char *vrstr,
						char *valuestr, class ManagedAttributeList& list);
static int FindProcparValue(char *procparPath, char *fieldName, char *retstr);

int ProcessCustomTags(char *filepath, char *procparPath, FILE *fdf_file, class ManagedAttributeList& list)
{
	unsigned short	group;
	unsigned short	element;
	char			*groupstr, *elementstr, *vrstr, *valuestr;
	char			*cptr;
	char			*savestr1;
	char			str[1024];
	char			str1[1024];
	FILE			*fd, *procparfd;

	fd = fopen(filepath, "r");
	if(fd == NULL)
	{
		return(1);
	}

	//(2001,00fc) LO    [VARIAN MRI SYSTEM]                    #    22,  1  PrivateCreator

	while(fgets(str, sizeof(str), fd) != NULL)
	{
		// Skip blank lines and comments
		if((str[0] == '#') || (isspace(str[0]))) continue;

		groupstr = strchr(str, '(');
		if(groupstr == NULL) continue;
		groupstr++;
		group = strtol(groupstr, NULL, 16);

		elementstr = strchr(str, ',');
		if(elementstr == NULL) continue;
		elementstr++;
		element = strtol(elementstr, NULL, 16);

		vrstr = strchr(str, ' ');
		if(vrstr == NULL) continue;
		vrstr++;
		vrstr[2] = 0;

		valuestr = vrstr + 3;
		while(isspace(*valuestr)) valuestr++;

		// Figure out which type of value string we have
		switch(valuestr[0])
		{
		case '[':
			// Look for the closing bracket
			cptr = strchr(valuestr, ']');
			if(cptr == NULL)
			{
				valuestr = NULL;
				break;
			}

			// Null terminate and isolate the value string
			*cptr = '\0';
			valuestr++;

			// Check for values that are tied to procpar.  Indicated by '=' leading char
			if(valuestr[0] == '=')
			  {
			    if(FindFDFValue(fdf_file, ++valuestr, str1) ==0)
			      {
				strcpy(valuestr, str1);
			      }
			    else if(FindProcparValue(procparPath, ++valuestr, str1) == 0)
			      {					
				strcpy(valuestr, str1);
			      }
			    else
			      {
				// If the value was not found, don't do anymore processing of this tag
				valuestr = NULL;
			      }
			    break;
			}
			break;
		case '(':
			// Values in parens indicate they are not used, so skip them
			valuestr = NULL;
			break;
		default:
			cptr = valuestr;
			while(!isspace(*cptr++));
			*(cptr - 1) = '\0';
			break;
		}
		if(valuestr == NULL) continue;

		// Skip DICOM header fields that will be filled in by the dicom3tools libraries
		if((group == 0x0000) ||
		   ((group == 0x0008) && (element == 0x0012)) ||
		   ((group == 0x0008) && (element == 0x0013)) ||
		   ((group == 0x0008) && (element == 0x0014)) ||
		   ((group == 0x0028) && (element == 0x0100)) ||
		   ((group == 0x0028) && (element == 0x0101)) ||
		   ((group == 0x0028) && (element == 0x0102)) ||
		   ((group == 0x0028) && (element == 0x0103)))
		{
			continue;
		}

		// All fields have been parsed, now insert the element into the DICOM header
//		printf("Processing value: '(0x%04x, 0x%04x)' %2.2s '%s'\n", group, element, vrstr, valuestr);
		InsertCustomElement(group, element, vrstr, valuestr, list);
	}
	fclose(fd);
	return(0);
}

int InsertCustomElement(unsigned short group, unsigned short element, char *vrstr,
						char *valuestr, class ManagedAttributeList& list)
{

	// Determine the VR (Value Representation) of the value being inserted and add
	// using the appropriate ManagedAttributeList call

	// First, remove the entry from the header if it's already there
	list-=Tag(group, element);

	// AE Application Entity
	if(strcmp(vrstr, "AE") == 0)
	{
		list+=new ApplicationEntityAttribute(Tag(group, element), valuestr);
	}
	// AS Age String Example - "018M" would represent an age of 18 months.
	else if (strcmp(vrstr, "AS") == 0)
	{
		list+=new AgeStringAttribute(Tag(group, element), valuestr);
	}
	// AT Attribute Tag Ordered pair of 16-bit unsigned integers that is the value of a Data Element Tag.
	else if (strcmp(vrstr, "AT") == 0)
	{
		fprintf(stderr, "VR value 'AT' not yet supported, ignoring...\n");
	}
	// CS Code String A string of characters with leading or trailing spaces (20H) being non-significant.
	else if (strcmp(vrstr, "CS") == 0)
	{
		list+=new CodeStringAttribute(Tag(group, element), valuestr);
	}
	// DA Date Example -"19930822" would represent August 22, 1993.
	else if (strcmp(vrstr, "DA") == 0)
	{
		list+=new DateStringAttribute(Tag(group, element), valuestr);
	}
	// DL Delimitation
	else if (strcmp(vrstr, "DL") == 0)
	{
		fprintf(stderr, "VR value 'DL' not yet supported, ignoring...\n");
	}
	//DS Decimal String A string of characters representing either a fixed point number or a floating point number. DT DATE TIME The Date Time common data type.
	else if (strcmp(vrstr, "DS") == 0)
	{
		list+=new DecimalStringAttribute(Tag(group, element), valuestr);
	}
	// FL Floating Point Single
	else if (strcmp(vrstr, "FL") == 0)
	{
		list+=new FloatSingleAttribute(Tag(group, element), atof(valuestr));
	}
	// FD Floating Point Double
	else if (strcmp(vrstr, "FD") == 0)
	{
		list+=new FloatDoubleAttribute(Tag(group, element), atof(valuestr));
	}
	// IS Integer String
	else if (strcmp(vrstr, "IS") == 0)
	{
		if(strchr(valuestr, '\\') == NULL)
		{
			int ival = atoi(valuestr);
			sprintf(valuestr, "%d", ival);
		}
		list+=new IntegerStringAttribute(Tag(group, element), valuestr);
	}
	// LO Long String
	else if (strcmp(vrstr, "LO") == 0)
	{
		list+=new LongStringAttribute(Tag(group, element), valuestr);
	}
	// LT Long Text
	else if (strcmp(vrstr, "LT") == 0)
	{
		list+=new LongTextAttribute(Tag(group, element), valuestr);
	}
	// OB Other Byte String
	else if (strcmp(vrstr, "OB") == 0)
	{
		fprintf(stderr, "VR value 'OB' not yet supported, ignoring...\n");
//		list+=new OtherByteLargeNonPixelAttribute(Tag(group, element), valuestr);
	}
	// OF Other Float String
	else if (strcmp(vrstr, "OF") == 0)
	{
		fprintf(stderr, "VR value 'OF' not yet supported, ignoring...\n");
//		list+=new OtherFloatSmallAttribute(Tag(group, element), valuestr);
	}
	// OW Other Word String
	else if (strcmp(vrstr, "OW") == 0)
	{
		fprintf(stderr, "VR value 'OW' not yet supported, ignoring...\n");
//		list+=new OtherWordSmallNonPixelAttribute(Tag(group, element), valuestr);
	}
	// PN Person Name The five components in their order of occurrence are: family name complex, given name complex, middle name, name prefix, name suffix.PN
	// Examples:
	// Rev. John Robert Quincy Adams, B.A. M.Div. "Adams^John Robert Quincy^^Rev.^B.A. M.Div." [One family name; three given names; no middle name; one prefix; two suffixes.]
	// Susan Morrison-Jones, Ph.D., Chief Executive Officer "Morrison-Jones^Susan^^^Ph.D., Chief Executive Officer" [Two family names; one given name; no middle name; no prefix; two suffixes.]
	// John Doe "Doe^John" [One family name; one given name; no middle name, prefix, or suffix. Delimiters have been omitted for the three trailing null components.] (for examples of the encoding of Person Names using multi-byte character sets see Annex H of Digital Imaging and Communications in Medicine (DICOM) - Part 5: Data Structures and Encoding)
	else if (strcmp(vrstr, "PN") == 0)
	{
		list+=new PersonNameAttribute(Tag(group, element), valuestr);
	}
	// SH Short String
	else if (strcmp(vrstr, "SH") == 0)
	{
		list+=new ShortStringAttribute(Tag(group, element), valuestr);
	}
	// SL Signed Long Represents an integer, n, in the range: - 2 31 <= n <= (2 31 - 1)
	else if (strcmp(vrstr, "SL") == 0)
	{
		list+=new SignedLongAttribute(Tag(group, element), atoi(valuestr));
	}
	// SQ Sequence of Items
	else if (strcmp(vrstr, "SQ") == 0)
	{
		fprintf(stderr, "VR value 'SQ' not yet supported, ignoring...\n");
	}
	// SS Signed Short Represents an integer n in the range: -2 15 <= n <= (2 15 - 1)
	else if (strcmp(vrstr, "SS") == 0)
	{
		list+=new SignedShortAttribute(Tag(group, element), atoi(valuestr));
	}
	// ST Short Text
	else if (strcmp(vrstr, "ST") == 0)
	{
		list+=new ShortTextAttribute(Tag(group, element), valuestr);
	}
	// TM Time A string of characters of the format hhmmss.frac
	else if (strcmp(vrstr, "TM") == 0)
	{
		list+=new TimeStringAttribute(Tag(group, element), valuestr);
	}
	// UI Unique Identifier A character string containing a UID that is used to uniquely identify a wide variety of items.
	else if (strcmp(vrstr, "UI") == 0)
	{
		list+=new UIStringAttribute(Tag(group, element), valuestr);
	}
	// UL Unsigned Long: Unsigned binary integer 32 bits long. Represents an integer n in the range: 0 <= n < 2 32
	else if (strcmp(vrstr, "UL") == 0)
	{
		list+=new UnsignedLongAttribute(Tag(group, element), strtoul(valuestr, NULL, 10));
	}
	// UN Unknown
	else if (strcmp(vrstr, "UN") == 0)
	{
		fprintf(stderr, "VR value 'UN' not yet supported, ignoring...\n");
	}
	// US Unsigned Short Unsigned binary integer 16 bits long. Represents integer n in the range: 0 <= n < 2 16
	else if (strcmp(vrstr, "US") == 0)
	{
		list+=new UnsignedShortAttribute(Tag(group, element), atoi(valuestr));
	}
	// UT Unlimited Text
	else if (strcmp(vrstr, "UT") == 0)
	{
		list+=new UnlimitedTextAttribute(Tag(group, element), valuestr);
	}
	// The VR passed in was not recognized, so just ignore and go on to the next value.
	else
	{
		fprintf(stderr, "WARNING: Unknown VR in custom tag file '%s' - Ignoring...\n", vrstr);
	}
	return(0);
}

int	FindProcparValue(char *procparPath, char *fieldName, char *retptr)
{
	int			i, j, k, l;
	int			ret;
	int			type = 0;
	int			active = 0;
	int			nvals = 0;
	int 		ivalue;
	float		fvalue;
	char		name[128];

	bool		found = false;
	FILE		*procparfd;

	procparfd = fopen(procparPath, "r");
	if(procparfd == NULL)
	{
		fprintf(stderr, "Unable to open procpar file for insertion of custom tag...\n");
		return(1);
	}

	/* Get the parameter name */
	while(fscanf(procparfd, "%s", name) == 1)
	{
		/* Read the flag variables, looking for type and value of 'active',
		   and get the number of values */
		if (fscanf(procparfd, "%*f %d %*f %*f %*f %*f %*f %*f %d %*f %d",
		  &type,&active,&nvals) != 3)
		{
			fprintf(stderr, "Error reading procpar values: %s\n", strerror(errno));
		}

		/* Now get the values */
		switch(type)
		{
			case 0:
				for(j = 0 ; j < nvals ; j++)
				{
					if (fscanf(procparfd, "%d", &ivalue) != 1)
					{
						fprintf(stderr, "Error reading int values...\n");
					}
				}
				// Skip any values that aren't active
				if(active == 0) break;
				break;
			case 1:
				for(j = 0 ; j < nvals ; j++)
				{
					if (fscanf(procparfd, "%f", &fvalue) != 1)
					{
						fprintf(stderr, "Error reading float values...\n");
					}
				}
				// Skip any values that aren't active
				if(active == 0) break;
				break;
			case 2:
				for (j = 0 ; j < nvals ; j++)
				{
					/* A single string can contain white space so we check for the final " */
					l=0;
					do
					{
						if(fscanf(procparfd, "%s", str1) != 1)
						{
							fprintf(stderr, "Error reading string values...\n");
						}
						k = 0;
						if(str1[0] == '"') k++; /* skip leading quote */

						for(k = k ; k < strlen(str1) ; k++)
						{
							str2[l++]=str1[k]; /* copy to str2 */
						}

						if(str1[strlen(str1)-1] != '"') str2[l++]=' '; /* add white space */
					} while(str1[strlen(str1)-1] != '"');

					str2[--l]=0; /* NULL terminate */
				}
				// Skip any values that aren't active
				if(active == 0) break;
				break;
		}

		// See if we found the parameter we are looking for
		if(strcmp(name, fieldName) == 0)
		{
			// Copy the found field into the return string
			switch(type)
			{
			case 0:
				sprintf(retptr, "%d", ivalue);
				break;
			case 1:
				sprintf(retptr, "%f", fvalue);
				break;
			case 2:
				strcpy(retptr, str2);
				break;
			}
			return(0);
		}
		// Read the number of enumerable values
		if (fscanf(procparfd,"%d",&nvals) != 1)
		{
			fprintf(stderr, "Error reading int values...\n");
		}

		// Move on to the next field if there are no enumerable values left
		if(nvals == 0) continue;

		// Skip any enumerable values
		switch(type)
		{
			case 0:
				for(i = 0 ; i < nvals ; i++)
				{
					if(fscanf(procparfd, "%*d") != 1)
					{
						fprintf(stderr, "Error reading procpar value: %s\n", strerror(errno));
						break;
					}
				}
				break;
			case 1:
				for(i = 0 ; i < nvals ; i++)
				{
					if(fscanf(procparfd, "%*f") != 1)
					{
						fprintf(stderr, "Error reading procpar float value: %s\n", strerror(errno));
						break;
					}
				}
				break;
			case 2:
				// A single string can contain white space so we check for the final '"'
				i = 0;
				while(i < nvals)
				{
					ret = fscanf(procparfd,"%s",str1);
					if(ret != 1)
					{
						fprintf(stderr, "Error reading procpar string value: %d %s\n", ret, strerror(errno));
						break;
					}
					if(str1[strlen(str1) - 1] == '"') i++;
				}

				if(strcmp(name, fieldName) == 0)
				{
					found = true;
				}
				break;
		}
	}
}

