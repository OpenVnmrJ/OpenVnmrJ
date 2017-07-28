/*
 * Copyright (C) 2017  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
//============================================================================
// Name        : GetScaleFactor.cpp
// Author      : Varian Inc.
// Version     : 1.0
// Description : This module will read all FDF or FID files in the specified
//               directory and find the min and max pixel value of all files.
//               These values will be used to calculate the scaling factor
//               to be inserted into the final DICOM output object.  Using n a
//               this scaling factor will result in uniform scaling of all
//               images within a given study or series.
//============================================================================

#include <fstream>
#include <iostream>
using namespace std;

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

// Local prototypes
int GetFDFMinMax(char *path, float *fileMin, float *fileMax);
int GetFIDMinMax(char *path, float *fileMin, float *fileMax);

int GetScaleFactor(const char *path, float *minPixel, float *maxPixel)
{
	int				nFiles;
	int				nImageFiles = 0;
	float			tempMin = 65536.0;
	float			tempMax = 0.0;
	struct dirent 	**namelist;
	char			filename[PATH_MAX];

	nFiles = scandir(path, &namelist, 0, alphasort);
	if(nFiles < 0)
	{
		perror("scandir");
		return(-1);
	}

	while(nFiles--)
	{
		// See if the next entry in the directory is an FDF file
		if(strstr(namelist[nFiles]->d_name, ".fdf") != NULL)
		{
			// An FDF file was found, so we will process the file
			sprintf(filename, "%s/%s", path, namelist[nFiles]->d_name);

			printf("Processing FDF file: %s\n", filename);
			GetFDFMinMax(filename, &tempMin, &tempMax);
			nImageFiles++;
		}
		else if(strcmp(namelist[nFiles]->d_name, "fid") == 0)
		{
			// An FID file was found, so we will process the file
			sprintf(filename, "%s/%s", path, namelist[nFiles]->d_name);

			printf("Processing FID file: %s\n", namelist[nFiles]->d_name);
			GetFIDMinMax(namelist[nFiles]->d_name, &tempMin, &tempMax);
			nImageFiles++;
		}
		else
		{
			// Skip the file if it's not an FDF or FID file
			continue;
		}
		if(tempMin < *minPixel) *minPixel = tempMin;
		if(tempMax > *maxPixel) *maxPixel = tempMax;
	}

	printf("%d Files processed for min and max pixel values...\n", nImageFiles);
}

int GetFDFMinMax(char *path, float *fileMin, float *fileMax)
{
	int				i;
	int				fsize;
	int				pixels;
	int				numPixels;
	unsigned char	c;
	char 			*pixelBuf;
	float			*fptr;
	float			pixel;
	float			minPixel, maxPixel;
	FILE			*fd;
	struct stat		fstat;

	if(stat(path, &fstat) == -1)
	{
		fprintf(stderr, "Error from stat() on %s - errno: %d\n", path, errno);
		return(-1);
	}
	fsize = fstat.st_size;

	fd = fopen(path, "r");
	if(fd == NULL)
	{
		fprintf(stderr, "Error opening FDF file - errno: %d\n", errno);
		return(-1);
	}
	c = 0xFF;
	while(c != '\0')
	{
		if(fread(&c, 1, 1, fd) != 1)
		{
			fprintf(stderr, "Error locating end of FDF header - errno: %d\n", errno);
			fclose(fd);
			return(-1);
		}
	}

	// Find the min and max pixel values to calculate the appropriate
	// scaling factor to apply
	minPixel = 10000000000.0;
	maxPixel = 0.0;

	pixelBuf = (char *)malloc(fsize);
	if(pixelBuf == NULL)
	{
		fprintf(stderr, "Error allocating pixel memory - errno: %d\n", errno);
		fclose(fd);
		return(-1);
	}
	pixels = fread(pixelBuf, 1, fsize, fd);
	printf("Pixel bytes read: %d\n", pixels);
	fclose(fd);

	fptr = (float *)pixelBuf;
	numPixels = pixels / sizeof(float);
	for(i = 0 ; i < numPixels ; i++)
	{
		if(fptr[i] < minPixel) minPixel = fptr[i];
		if(fptr[i] > maxPixel) maxPixel = fptr[i];
	}
	printf("File: %s - Min Pixel: %f\n", path, minPixel);
	printf("File: %s - Max Pixel: %f\n", path, maxPixel);

	if(minPixel < *fileMin) *fileMin = minPixel;
	if(maxPixel > *fileMax) *fileMax = maxPixel;

	printf("Return Min: %f\n", *fileMin);
	printf("Return Max: %f\n", *fileMax);
	return 0;
}

int GetFIDMinMax(char *path, float *fileMin, float *fileMax)
{

}

