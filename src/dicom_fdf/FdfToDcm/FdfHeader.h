/*
 * Copyright (C) 2017  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
//============================================================================
// Name        : FdfHeader.h
// Author      : Varian, Inc.
// Version     : 1.0
// Description : This header file contains definitions of the fields contained
//				 in an FDF image file.  There are declarations that describe
//				 the input fields and the data types they contain as well as
//				 structures that will hold the values derived from the FDF
//				 input strings.
//============================================================================

#ifndef FDFHEADER_H
#define FDFHEADER_H

#define DATASET_2D 	1
#define DATASET_3D 	2

#define BITS_STORED			16
#define BITS_ALLOCATED		16
#define HIGH_BIT			15
#define	MAX_PIXEL_VALUE		(float)((1 << BITS_STORED) - 1)
#define VARIAN_COMPANY_ID	"670589"
#define UID_PREFIX			"1.3.6.1.4.1."

#define	outpixfilename 		"rawPixels.dat"
#define PROCPAR_FILENAME	"/procpar"
#define CUSTOM_FILENAME		"/dicom.default"

// List of the data types that may be contained in FDF fields
enum fdfTypes
{
	FDF_INT = 1,
	FDF_FLOAT,
	FDF_CHARPTR,
	FDF_CHARARRAY1,
	FDF_CHARARRAY2,
	FDF_CHARARRAY3,
	FDF_FILENAME,
	FDF_FLOATARRAY2,
	FDF_FLOATARRAY3,
	FDF_FLOATARRAY9
};

// Structure defining the fields in the FDF header fields
typedef struct fdfInputEntry
{
	char	type[8];
	char	name[24];
	int		datatype;
} fdfInputEntry_t;

// Structure use to lookup an individual field and its associated
// data type
const fdfInputEntry_t	fdfInput[] =
{
		{ "float", 	"rank", 			FDF_FLOAT },
		{ "char", 	"*spatial_rank", 	FDF_CHARPTR },
		{ "char", 	"*storage", 		FDF_CHARPTR },
		{ "float", 	"bits", 			FDF_FLOAT },
		{ "char", 	"*type", 			FDF_CHARPTR },
		{ "float", 	"matrix[]", 		FDF_FLOATARRAY3 },
		{ "char", 	"*abscissa[]", 		FDF_CHARARRAY3 },
		{ "char", 	"*ordinate[]", 		FDF_CHARARRAY1 },
		{ "float", 	"span[]", 			FDF_FLOATARRAY3 },
		{ "float", 	"origin[]", 		FDF_FLOATARRAY3 },
		{ "char", 	"*nucleus[]", 		FDF_CHARARRAY2 },
		{ "float", 	"nucfreq[]", 		FDF_FLOATARRAY2 },
		{ "float", 	"location[]", 		FDF_FLOATARRAY3 },
		{ "float", 	"roi[]", 			FDF_FLOATARRAY3 },
		{ "float", 	"orientation[]", 	FDF_FLOATARRAY9 },
		{ "flota",	"gap",				FDF_FLOAT },
		{ "char", 	"*array_name", 		FDF_CHARPTR },
		{ "char", 	"*file", 			FDF_CHARPTR },
		{ "int", 	"slice_no", 		FDF_INT },
		{ "int", 	"slices", 			FDF_INT },
		{ "int",    "echo_no", 			FDF_INT },
		{ "int",    "echoes", 			FDF_INT },
		{ "float",  "TE", 				FDF_FLOAT },
		{ "float",  "te", 				FDF_FLOAT },
		{ "float",  "TR", 				FDF_FLOAT },
		{ "float",  "tr", 				FDF_FLOAT },
		{ "int", 	"ro_size", 			FDF_INT },
		{ "int", 	"pe_size", 			FDF_INT },
		{ "char", 	"*sequence", 		FDF_CHARPTR },
		{ "char", 	"*studyid", 		FDF_CHARPTR },
		{ "char", 	"*pname", 		FDF_CHARPTR },
		{ "char", 	"*oper", 		FDF_CHARPTR },
		{ "char", 	"*file", 			FDF_FILENAME },
		{ "char", 	"*position1", 		FDF_CHARPTR },
		{ "char", 	"*position2", 		FDF_CHARPTR },
		{ "float",  "TI", 				FDF_FLOAT },
		{ "float",  "ti", 				FDF_FLOAT },
		{ "int",    "array_index", 		FDF_INT },
		{ "float",  "array_dim", 		FDF_FLOAT },
		{ "float",  "image", 			FDF_FLOAT },
		{ "float",	"imagescale", 		FDF_FLOAT },
		{ "int",    "display_order", 	FDF_INT },
		{ "int",    "bigendian", 		FDF_INT },
		{ "float",	"psi", 				FDF_FLOAT },
		{ "float", 	"phi", 				FDF_FLOAT },
		{ "float",	"theta",			FDF_FLOAT },
		{ "float",  "orientation[]",	FDF_FLOATARRAY9 },
		{ "int", 	"checksum", 		FDF_INT },
		{ "char",	"appdirs",			FDF_CHARPTR	},
		{ NULL, NULL, 0 }
};

// Structure to hold FDF fields that have been converted to internal
// binary or string representations
typedef struct fdfEntries
{
	float  	rank;
	char  	spatial_rank[16];
	char  	storage[16];
	float  	bits;
	char  	type[16];
	float  	matrix[3];
	char  	abscissa[3][16];
	char  	ordinate[1][16];
	float  	span[2];
	float  	origin[3];
	char  	nucleus[2][16];
	float  	nucfreq[2];
	float  	location[3];
	float  	roi[3];
	float	gap;
	float  	orientation[9];
	char 	array_name[16];
	char  	file[PATH_MAX];
	int    	slice_no;
	int    	slices;
	int    	echo_no;
	int    	echoes;
	float  	TE;
	float  	te;
	float  	TR;
	float  	tr;
	int 	ro_size;
	int 	pe_size;
	char 	sequence[64];
	char 	studyid[64];
	char 	pname[64];
        char    oper[64];
	char 	file2[PATH_MAX];
	char 	position1[64];
	char 	position2[64];
	float  	TI;
	float  	ti;
	int    	array_index;
	float  	array_dim;
	float  	image;
	float 	imagescale;
	int    	display_order;
	float  	psi;
	float  	phi;
	float  	theta;
	int    	bigendian;
	int 	checksum;
	char	appdirs[16][PATH_MAX];
} fdfEntries_t;

// Function prototypes shared by various C++ source files
// In ProcessFdfHeader.cpp
extern int ProcessFdfHeader(FILE *fd, fdfEntries_t *fdfValues);
extern int FindFDFValue(FILE *file, char *fieldName, char *retstr);

// In ConvertFdfFieldToTag.cpp
extern int convertFdfFieldToTag(char *fieldType, char *fieldName,
			char *fieldValue, fdfEntries_t *fdfValues);

// In ProcessProcpar.cpp
extern int ProcessProcpar(FILE *infile, class ManagedAttributeList& list,
		double *psi, double *phi, double *theta, double *lro, double *lpe,
		fdfEntries_t *fdfValues);
void InsertField(int type, char *name, void *valptr, class ManagedAttributeList& list,
		fdfEntries_t *fdfValues);

// In ProcessFidData.cpp
int ProcessFidData(FILE *fidfd, class ManagedAttributeList& list, fdfEntries_t *fdfValues);

// In ProcessCustomTags.cpp
int ProcessCustomTags(char *filepath, char *procparPath, FILE *fdf_file,  class ManagedAttributeList& list);

// In GetScaleFactor.cpp
int GetScaleFactor(const char *path, float *minPixel, float *maxPixel);

#endif	// #ifndef FDFHEADER_H
