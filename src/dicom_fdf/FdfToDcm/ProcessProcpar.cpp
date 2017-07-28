/*
 * Copyright (C) 2017  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
//============================================================================
// Name        : ProcessProcpar.cpp
// Author      : Varian, Inc.
// Version     : 1.0
// Description : This module contains code that will process the procpar file
//               associated with the FDF image(s) being processed.
//============================================================================

#include <fstream>
#include <iostream>
using namespace std;

#include <unistd.h>
#include <errno.h>
#include <strings.h>

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

static	int		optVals = 0;
#define OPTIONAL_ECHOTRAINLENGTH	0x01

static const char *months[13] =
{
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
		""
};

static char		name[1024];
static char		stringpar[1024];

static char		str1[1024];
static char		str2[1024];

// Used to calculate patient orientation values
static double	localpsi = 0.0;
static double	localphi = 0.0;
static double 	localtheta = 0.0;

static int is3D=0;
// Values used to determins ScanningSequence and SequenceVariant
static char		localseqfil[32] = "";
static char		localir[4] = "";
static char		localmt[4] = "";
static char		localrfspoil[4] = "";
static char		localspinecho[4] = "";
static char		localspoilflag[4] = "";
static double	locallpe = 0.0;
static double	locallro = 0.0;
static double	localti = 0.0;
static double	localthk = 0.0;
static double	localgap = 0.0;

double   localqrsdelay = -1.0;
double   localidelay = -1.0;

// Values used to determine rows and columns
static double	localfn1 = 0.0;
static double	localfn = 0.0;
static double	localnv = 0.0;
static double	localnp = 0.0;

// Used to compute Pixel Bandwidth
static double	localsw = 0.0;

int ProcessProcpar(FILE *procparfd, class ManagedAttributeList& list,
		double *psi, double *phi, double *theta, double *lro, double *lpe,
		fdfEntries_t *fdfValues)
{
	int			i, j, k, l;
	int			ret;
	int			type = 0;
	int			active = 0;
	int			nvals = 0;
	int 		ivalue;
	float		fvalue;
	
	optVals = 0;

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

//		printf("Processing procpar value: %s\n", name);
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
				InsertField(type, name, (void *)&ivalue, list, fdfValues);
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
				InsertField(type, name, (void *)&fvalue, list, fdfValues);
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
				InsertField(type, name, (void *)str2, list, fdfValues);
				break;
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
				break;
		}
	}

	// Use the seqfil field, amongh others, to determine the values to insert for the
	// ScanningSequence and SequenceVariant tags.  The rules of determining these fields are:
	//
	// Algorighm for Scanning Sequence:
	// Look at procpar parameter "seqfil".
	//  If it is "sems" or "mems" or "se3d" or "fse3d" or "fsems"  use SE.
	//  If it is "gems" or "mgems" or "ge3d"or "me3d" or "mprage3d"  or "mge3d" use GR.
	//  If it is "gemsir" use GR and IR.
	//  If it is "flair" use SE and IR.
	//  If it is "epi" or "epip" use EP.
	// Look at procpar parameter "ir". If set to "y'" then add IR to the above.
	// Default to RM if none of the above descriptions match.

	//For Sequence Variant:  (this will likely have combinations, too):
	// As above, based off of "seqfil"
	// If it is "mprage3d" use SK
	// If it is "fssfp" or "essfp"  use SS
	// If it is "tssfp" use TRSS
	// Look at procpar param "mt". If it is 'y' then use MTC
	// Look at procpar param "spoilflag", If it is 'y' use SP
	// Look at procpar param "rfspoil", If it is 'y' use SP
	// Look at procpar param "spinecho", If it is 'y' use MP
	// Default to NONE if none of the above descriptions match.

	// Determine the value to use for ScanningSequence which is based on the value of seqfil
	// If it is "sems" or "mems" or "se3d" or "fse3d" or "fsems"  use SE.
	if((strcmp(localseqfil, "sems") == 0) || (strcmp(localseqfil, "mems") == 0) ||
	   (strcmp(localseqfil, "se3d") == 0) || (strcmp(localseqfil, "fse3d") == 0) ||
	   (strcmp(localseqfil, "fsems") == 0))
	{
		strcpy(str1, "SE");
		if(strcmp(localir, "y") == 0)
		{
			strcat(str1, "\\IR");
		}
	}
	//  If it is "gems" or "mgems" or "ge3d"or "me3d" or "mprage3d"  or "mge3d" use GR.
	else if((strcmp(localseqfil, "gems") == 0) || (strcmp(localseqfil, "mgems") == 0) ||
		    (strcmp(localseqfil, "ge3d") == 0) || (strcmp(localseqfil, "me3d") == 0) ||
		    (strcmp(localseqfil, "mprage3d") == 0) || (strcmp(localseqfil, "mge3d") == 0))
	{
		strcpy(str1, "GR");
		if(strcmp(localir, "y") == 0)
		{
			strcat(str1, "\\IR");
		}
	}
	//  If it is "gemsir" use GR and IR.
	else if(strcmp(localseqfil, "gemsir") == 0)
	{
		strcpy(str1, "GR\\IR");
	}
	//  If it is "flair" use SE and IR.
	else if(strcmp(localseqfil, "flair") == 0)
	{
		strcpy(str1, "SE\\IR");
	}
	//  If it is "epi" or "epip" use EP.
	else if((strcmp(localseqfil, "epi") == 0) || strcmp(localseqfil, "epip"))
	{
		strcpy(str1, "EP");
		if(strcmp(localir, "y") == 0)
		{
			strcat(str1, "\\IR");
		}
	}
	// We didn't find a match, so use the default value
	else
	{
		strcpy(str1, "RM");
	}
	list+=new CodeStringAttribute(TagFromName(ScanningSequence), str1);

	// Now determine the proper SequenceVariant value, also based on seqfil
	// If it is "mprage3d" use SK
	// If it is "fssfp" or "essfp"  use SS
	// If it is "tssfp" use TRSS
	// Look at procpar param "mt". If it is 'y' then use MTC
	// Look at procpar param "spoilflag", If it is 'y' use SP
	// Look at procpar param "rfspoil", If it is 'y' use SP
	// Look at procpar param "spinecho", If it is 'y' use MP
	// Default to NONE if none of the above descriptions match.
	if(strcmp(localseqfil, "mprage3d") == 0)
	{
		strcpy(str1, "SK");
	}
	else if(strcmp(localseqfil, "fssfp") == 0)
	{
		strcpy(str1, "SS");
	}
	else if(strcmp(localseqfil, "tssfp") == 0)
	{
		strcpy(str1, "TRSS");
	}
	else if(strcmp(localmt, "y") == 0)
	{
		strcpy(str1, "MTC");
	}
	else if((strcmp(localspoilflag, "y") == 0) || (strcmp(localrfspoil, "y") == 0))
	{
		strcpy(str1, "SP");
	}
	else if(strcmp(localspinecho, "y") == 0)
	{
		strcpy(str1, "MP");
	}
	else
	{
		strcpy(str1, "NONE");
	}
	list+=new CodeStringAttribute(TagFromName(SequenceVariant), str1);

	// If the IR modifier is used, capture InversionTime as well
	if(strcmp(localir, "y") == 0)
	{
		// InversionTime: 60
		// 1 0.06  <-----X 10^3 again...
		sprintf(str1, "%f", localti * 1000.0);
		list+=new DecimalStringAttribute(TagFromName(InversionTime), str1);
	}
	// Save the positional information provided by psi, phi and theta and pass them back
	// to the caller.
	*psi = localpsi;
	*phi = localphi;
	*theta = localtheta;

	// Save the values of lpe and lro for calculating pixel spacing
	*lro = locallro;
	*lpe = locallpe;

	// Calculate the rows and columns if they were not read from an FDF file
	/*
	 * float  matrix[] = {128, 128};
     * {fn1, fn} – if they are “active”
     *     o If “fn1” is not active use “nv in its place
     *     o If “fn” is not active use 0.5*”np” in its place
	 *
	 */
	if(fdfValues->matrix[0] == 0)
	{
		if(localfn1 != 0.0)
		{
			fdfValues->matrix[0] = localfn1;
		}
		else
		{
			fdfValues->matrix[0] = localnv;
		}
	}
	if(fdfValues->matrix[1] == 0)
	{
		if(localfn != 0.0)
		{
			fdfValues->matrix[1] = localfn;
		}
		else
		{
			fdfValues->matrix[1] = 0.5 * localnp;
		}
	}

	fdfValues->ro_size = (int)(localnp * 0.5);
	fdfValues->pe_size = (int)localnv;

	fdfValues->roi[0] = locallpe;
	fdfValues->roi[1] = locallro;
	fdfValues->roi[2] = localthk * 0.1;

	fdfValues->psi = localpsi;
	fdfValues->phi = localphi;
	fdfValues->theta = localtheta;

	// Fill in any fields that weren't present in the procpar file
	// but should have default values
	if((optVals & OPTIONAL_ECHOTRAINLENGTH) == 0)
	{
		list+=new IntegerStringAttribute(TagFromName(EchoTrainLength), "1");
	}

	// Compute the pixel bandwidth field sw/(0.5*np)
	str1[0] = '\0';
	if(localsw != 0.0)
	{
		sprintf(str1, "%f", localsw / (localnp * 0.5));
	}
	list+=new DecimalStringAttribute(TagFromName(PixelBandwidth), str1);




	if(!is3D)
	  {
	    // Compute SpacingBetweenSlices using thk + gap
	    sprintf(str1, "%f", localthk + 10.0*localgap);
	    // sprintf(str1, "%f",  localgap);
	    list+=new DecimalStringAttribute(TagFromName(SpacingBetweenSlices), str1);
	    
	    // slice thickness
	    sprintf(str1, "%f", localthk);
	    list+=new DecimalStringAttribute(TagFromName(SliceThickness), str1);
	  }

	
	

	return(0);
}

void InsertField(int type, char *name, void *valptr, class ManagedAttributeList& list,
		fdfEntries_t *fdfValues)
{
	int			i;
	int			x;
	char		month[4] = "00";
	int			day = 0;
	char		year[5] = "0000";
	char		*tokptr;
	char		*cptr = (char *)valptr;
	int			*iptr = (int *)valptr;
	float		*fptr = (float *)valptr;
	char		str[256];
	char		str2[256];

	if(strcmp(name, "date") == 0)
	{
		// StudyDate (YYYYMMDD)
		// 1 "Mar  5 2010"
		for(i = 0 ; months[i][0] != '\0' ; i++)
		{
			if(strncmp(months[i], cptr, strlen(months[i])) == 0)
			{
				sprintf(month, "%02d", i+1);
				break;
			}
		}
		tokptr = strtok(cptr, " ");
		if(tokptr != NULL) tokptr = strtok(NULL, " ");
		if(tokptr != NULL) day = atoi(tokptr);
		tokptr = strtok(NULL," ");
		if(tokptr != NULL) strcpy(year, tokptr);
		sprintf(str, "%4.4s%2.2s%02d", year, month, day);
		list+=new DateStringAttribute(TagFromName(StudyDate), str);
		list+=new DateStringAttribute(TagFromName(ContentDate), str);
	}
	else if(strcmp(name, "time_submitted") == 0)
	{
		// StudyTime
		// 1 "20100305T173753"
		if(strlen(cptr) == 15)
		{
			list+=new TimeStringAttribute(TagFromName(StudyTime), &cptr[9]);
			list+=new TimeStringAttribute(TagFromName(ContentTime), &cptr[9]);
		}
	}
	else if(strcmp(name, "investigator") == 0)
	{
		// ReferringPhysicianName
		// 1 ""
	  //		sprintf(str, "^%s^^^", cptr);
		list+=new PersonNameAttribute(TagFromName(ReferringPhysicianName), cptr);
	}
	else if(strcmp(name, "operator_") == 0)
	{
		// OperatorsName
		// 1 "vnmr1"
		strcpy(fdfValues->oper, cptr);
       		sprintf(str, "^%s^^^", cptr);
		list+=new PersonNameAttribute(TagFromName(OperatorName), cptr);
	}
	else if(strcmp(name, "console") == 0)
	{
		// ManufacturerModelName
		// 1 "vnmrs"
		list+=new LongStringAttribute(TagFromName(ManufacturerModelName), cptr);
	}
	else if(strcmp(name, "studyid_") == 0)
	{
		strcpy(fdfValues->studyid, cptr);
	}
	else if(strcmp(name, "name") == 0)
	{
		// PatientName
		// 1 ""
		if(strlen(cptr) == 0)
		{
			sprintf(str, "Anonymized");
		}
		else
		{
			sprintf(str, "%s", cptr);
		}
		strcpy(fdfValues->pname,cptr);
		list+=new PersonNameAttribute(TagFromName(PatientName), str);
	}
	else if(strcmp(name, "ident") == 0)
	{
		// PatientID
		// 1 ""
		if(strlen(cptr) == 0) strcpy(cptr, "Patient ID Unknown");
		list+=new LongStringAttribute(TagFromName(PatientID), cptr);
	}
	else if(strcmp(name, "birthday") == 0)
	{
		// PatientBirthDate
		// 4 ""
		if(strlen(cptr) != 0)
		{
			for(i = 0 ; months[i][0] != '\0' ; i++)
			{
				if(strncmp(months[i], cptr, strlen(months[i])) == 0)
				{
					sprintf(month, "%02d", i+1);
					break;
				}
			}
			tokptr = strtok(cptr, " ");
			if(tokptr != NULL) tokptr = strtok(NULL, " ");
			if(tokptr != NULL) day = atoi(tokptr);
			tokptr = strtok(NULL," ");
			if(tokptr != NULL) strcpy(year, tokptr);
			sprintf(str, "%4.4s%2.2s%02d", year, month, day);
			list+=new DateStringAttribute(TagFromName(PatientBirthDate), str);
		}
		else
		{
			list+=new DateStringAttribute(TagFromName(PatientBirthDate), "");
		}
	}
	else if(strcmp(name, "gender") == 0)
	{
		// PatientSex
		// 1 ""
		if(cptr[0] == 'f')
		{
			strcpy(str, "F");
		}
		else if(cptr[0] == 'm')
		{
			strcpy(str, "M");
		}
		else
		{
			strcpy(str, "");
		}
		list+=new CodeStringAttribute(TagFromName(PatientSex), str);
	}
	else if(strcmp(name, "weight") == 0)
	{
		if(*fptr != 0.0)
		{
			sprintf(str, "%.2f", *fptr);
			list+=new DecimalStringAttribute(TagFromName(PatientWeight), str);
		}
	}
	else if(strcmp(name, "seqfil") == 0)
	{
		// SequenceName
		// 1 "gems"
		list+=new ShortStringAttribute(TagFromName(SequenceName), cptr);
		strcpy(localseqfil, cptr);
	}
	else if(strcmp(name, "ir") == 0)
	{
		strcpy(localir, cptr);
	}
	else if(strcmp(name, "mt") == 0)
	{
		strcpy(localmt, cptr);
	}
	else if(strcmp(name, "rfspoil") == 0)
	{
		strcpy(localrfspoil, cptr);
	}
	else if(strcmp(name, "spinecho") == 0)
	{
		strcpy(localspinecho, cptr);
	}
	else if(strcmp(name, "spoilflag") == 0)
	{
		strcpy(localspoilflag, cptr);
	}
	else if(strcmp(name, "thk") == 0)
	{
		// SliceThickness
		// 1 2
		localthk = *fptr;
	}
	else if(strcmp(name, "tr") == 0)
	{
		// RepetitionTime:
		// 1 0.05  <----- multiply by 10^3 to get milliseconds
		sprintf(str, "%d", (int)(*fptr * 1000.0));
		list+=new DecimalStringAttribute(TagFromName(RepetitionTime), str);
	}
	else if(strcmp(name, "te") == 0)
	{
		// EchoTime: 10
		// 1 0.01  <---- X 10^3 to get millisecs
		sprintf(str, "%f", (*fptr * 1000.0));
		list+=new DecimalStringAttribute(TagFromName(EchoTime), str);
	}
	else if(strcmp(name, "heartrate") == 0)
	{
		// for cine
		sprintf(str, "%f", (*fptr));
		list+=new DecimalStringAttribute(TagFromName(HeartRate), str);
	}

	else if(strcmp(name, "idelay") == 0)
	{
		// for cine
	        localidelay = *fptr;
 	}

	else if(strcmp(name, "qrsdelay") == 0)
	{
		// for cine
	  localqrsdelay = *fptr;
	}
	else if(strcmp(name, "sfrq") == 0)
	{
		// ImagingFrequency: 399.4158
		// 1 399.415784
		sprintf(str, "%f", *fptr);
		list+=new DecimalStringAttribute(TagFromName(ImagingFrequency), str);
	}
	else if(strcmp(name, "tn") == 0)
	{
		// ImagedNucleus: 'H1'
		// 1 "H1"
		list+=new ShortStringAttribute(TagFromName(ImagedNucleus), cptr);
	}
	else if(strcmp(name, "flip1") == 0)
	{
		// Flip angle
		sprintf(str, "%f", *fptr);
		list+=new DecimalStringAttribute(TagFromName(FlipAngle), str);
	}
	else if(strcmp(name, "nt") == 0)
	{
		// Number of averages
		sprintf(str, "%f", *fptr);
		list+=new DecimalStringAttribute(TagFromName(NumberOfAverages), str);
	}
	else if(strcmp(name, "gap") == 0)
	{
		// Half of SpacingBetweenSlices.  Will be added with thk
		localgap = *fptr;
	}
	else if(strcmp(name, "etl") == 0)
	{
		// EchoTrainLength: 1  <---- 1 should be default, then if etl exists in procpar use that but it won;t always be there!
		// 1 4
		sprintf(str, "%d", (int)*fptr);
		list+=new IntegerStringAttribute(TagFromName(EchoTrainLength), str);
		optVals |= OPTIONAL_ECHOTRAINLENGTH;
	}
	else if(strcmp(name, "rfcoil") == 0)
	{
		// TransmitCoilName: 'H1_40mm'
		// 1 "H1_40mm"
		list+=new ShortStringAttribute(TagFromName(TransmitCoilName), cptr);
	}
	else if(strcmp(name, "scantime") == 0)
	{
		// AcquisitionDuration: 6
		// 1 "6.4s"
		strcpy(str, cptr);
		str[strlen(str) - 1] = '\0';
		list+=new FloatDoubleAttribute(TagFromName(AcquisitionDuration), atof(str));
	}
	else if(strcmp(name, "sense") == 0)
	{
		// ParallelAcquisition: 'NO'  <--- make this yes if sense = 'y'
		// ParallelAcquisitionTechnique: 'SENSE'
		// 1 "n"
		if(strcmp(cptr, "y") == 0)
		{
			strcpy(str, "YES");
			strcpy(str2, "SENSE");
		}
		else
		{
			strcpy(str, "NO");
			strcpy(str2, "");
		}
		list+=new CodeStringAttribute(TagFromName(ParallelAcquisition), str);
		list+=new CodeStringAttribute(TagFromName(ParallelAcquisitionTechnique), str2);
	}
	else if(strcmp(name, "ti") == 0)
	{
		localti = *fptr;
	}
	else if(strcmp(name, "psi") == 0)
	{
		localpsi = *fptr;
	}
	else if(strcmp(name, "phi") == 0)
	{
		localphi = *fptr;
	}
	else if(strcmp(name, "theta") == 0)
	{
		localtheta = *fptr;
	}
	else if(strcmp(name, "lpe") == 0)
	{
		locallpe = *fptr;
	}
	else if(strcmp(name, "lro") == 0)
	{
		locallro = *fptr;
	}
	else if(strcmp(name, "fn1") == 0)
	{
		localfn1 = *fptr;
	}
	else if(strcmp(name, "fn") == 0)
	{
		localfn = *fptr;
	}
	else if(strcmp(name, "nv") == 0)
	{
		localnv = *fptr;
	}
	else if(strcmp(name, "nv2") == 0)
	{
	  if(*fptr> 0.0)
	    is3D=1;
	}
	else if(strcmp(name, "np") == 0)
	{
		localnp = *fptr;
	}
	else if(strcmp(name, "sw") == 0)
	{
		localsw = *fptr;
	}
	else if(strcmp(name, "ppe") == 0)
	{
		fdfValues->origin[0] = atof(cptr);
		fdfValues->location[0] = atof(cptr);
	}
	else if(strcmp(name, "pro") == 0)
	{
		fdfValues->origin[1] = atof(cptr);
		fdfValues->location[1] = atof(cptr);
	}
	else if(strcmp(name, "pss0") == 0)
	{
		fdfValues->location[2] = atof(cptr);
	}
	else if(strcmp(name, "ns") == 0)
	{
		fdfValues->slices = (int)*fptr;
	}
	else if(strcmp(name, "ne") == 0)
	{
		fdfValues->echoes = atoi(cptr);
	}
	else if(strcmp(name, "appdirs") == 0)
	{
		int		i = 0;
		char	*str1;
		char	*str2;

		fdfValues->appdirs[0][0] = '\0';
		str1 = cptr;
		str2 = strchr(str1, ';');
		while(str1 != NULL)
		{
			if(str2 != NULL) *str2++ = '\0';
			strcpy(fdfValues->appdirs[i++], str1);
			str1 = str2;
			if(str1 != NULL)
			{
				str2 = strchr(str1, ';');
			}
		}
		strcpy(fdfValues->appdirs[i++], "/vnmr/imaging/dicom");
		strcpy(fdfValues->appdirs[i++], "/vnmr/dicom");
		fdfValues->appdirs[i][0] = '\0';
	}
}
