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
//				 file will be processed and its contents added to the DICOM
//				 output file.
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

#ifndef LINUX
#define LINUX
#endif
#include "FidData.h"
#include "FdfHeader.h"

#define FLOATPIXELFILE	"floatPixels.dat"

datafileheadSwapUnion	inhdr;
datafilehead			filehdr;
datablockheadSwapUnion	indata;
datablockhead			blkhdr;
dpointers				dataptrs;
hycmplxhead				hycomplxhdr;

int ProcessFidData(FILE *fidfd, class ManagedAttributeList& list, fdfEntries_t *fdfValues)
{
	int		i, x;
	int		ret;
	int		pixels;
	int		totalPixels;
	int		numPixels;
	int		pixelSize;
	int		pixelBufSize;
	int		floatstatus = 0;
	short 	intPixel;
	bool	readerror = false;
	float	scale = 0.0;
	float	pix;
	float	minPixel = 1000000.0;
	float	maxPixel = 0.0;

	char	*pixelBuf;
	short	*spixel;
	int		*ipixel;
	float	*fpixel;

	FILE	*fd;
	FILE	*floatfd;

	/*
	 * The reading of "fid" all hinges on the contents of "fidData.h", which defines
	 * the structures of the headers. The format looks like
	 * {File header} {block1 header} {block1 data} {block2 header} {block2 data} .....
	 * {blockN header}{blockN data}.
	 *
	 * So it is a single file header followed by pairs of block header + block data.
	 * The file header and the data block headers are fixed structures, as defined in fidData.h.
	 *
	 * Here is some matlab code, which also serves as pseudo-code, that I use to read fid files.
	 *
	 * Matlab Example
	 * % input data
	 * fid=fopen(filename,'r','n');
	 * % read file header
	 * fileheader.nblocks=fread(fid,1,'int32');
	 * fileheader.ntraces=fread(fid,1,'int32');
	 * fileheader.np=fread(fid,1,'int32');
	 * fileheader.ebytes=fread(fid,1,'int32');
	 * fileheader.tbytes=fread(fid,1,'int32');
	 * fileheader.bbytes=fread(fid,1,'int32');
	 * fileheader.vers_id=fread(fid,1,'int16');
	 * fileheader.status=fread(fid,1,'int16');
	 * fileheader.nbheaders=fread(fid,1,'int32');
	 */

	if(fread(&filehdr, sizeof(filehdr), 1, fidfd) != 1)
	{
		fprintf(stderr, "Error reading FID file header: %d, %s", errno, strerror(errno));
		return(1);
	}
	// Byte swap the header if necessary
	DATAFILEHEADER_CONVERT_NTOH(&filehdr);

/*%^%	printf("File Header: \n");
	printf("\tbbytes:    %d\n", filehdr.bbytes);
	printf("\tebytes:    %d\n", filehdr.ebytes);
	printf("\tnbheaders: %d\n", filehdr.nbheaders);
	printf("\tnblocks:   %d\n", filehdr.nblocks);
	printf("\tnp:        %d\n", filehdr.np);
	printf("\tntraces:   %d\n", filehdr.ntraces);
	printf("\tstatus:    %d\n", filehdr.status);
	printf("\ttbytes:    %d\n", filehdr.tbytes);
	printf("\tvers_id:   %d\n", filehdr.vers_id);
%^%*/

	/*
	 * The bit-wise encoded word in each data block header called "status" is useful
	 * to determine data type. File header ebytes tells us how big each element is,
	 * status can tell us float vs. int. This is defined in fidData.h.
	 *
	 * Decide if data is short, int or float using floatstatus and ebytes:
	 *
	 * Also, note that status (or anything else) does NOT have to be checked for each block.
	 * There can NOT be a change from block to block in data type, data size,
	 * number of data points in the block, etc. Only the data changes from block to block.
	 */

	// Figure out how many pixels and bytes are in each block
	floatstatus = filehdr.status & S_FLOAT;
	if(filehdr.ebytes == 2)
	{
		numPixels = (filehdr.bbytes - sizeof(blkhdr)) / sizeof(short);
		pixelBufSize = numPixels * sizeof(short);
		pixelSize = sizeof(short);
	}
	else if(floatstatus == 0)
	{
		numPixels = (filehdr.bbytes - sizeof(blkhdr)) / sizeof(int);
		pixelBufSize = numPixels * sizeof(int);
		pixelSize = sizeof(int);
	}
	else
	{
		numPixels = (filehdr.bbytes - sizeof(blkhdr)) / sizeof(float);
		pixelBufSize = numPixels * sizeof(float);
		pixelSize = sizeof(float);
	}

	// Store the rows and columns from the file header
	// TODO: Verify
	fdfValues->matrix[0] = filehdr.np;
	fdfValues->matrix[1] = filehdr.ntraces;

	/*
	 * % loop on blocks
	 * for iblock=1:fileheader.nblocks
	 *   dataheader(iblock).scale=fread(fid,1,'int16');
	 *   dataheader(iblock).status=fread(fid,1,'int16');
	 *   dataheader(iblock).index=fread(fid,1,'int16');
	 *   dataheader(iblock).mode=fread(fid,1,'int16');
	 *   dataheader(iblock).ctcount=fread(fid,1,'int32');
	 *   dataheader(iblock).lpval=fread(fid,1,'float32');
	 *   dataheader(iblock).rpval=fread(fid,1,'float32');
	 *   dataheader(iblock).lvl=fread(fid,1,'float32');
	 *   dataheader(iblock).tlt=fread(fid,1,'float32');
	 *    is_float_block = bitand(dataheader(iblock).status, hex2dec('8')) <-------------Note: use of "status" to tell if data is float (almost always is)
	 *     scale=1.0/(dataheader(iblock).ctcount);
	 *
	 *   if fileheader.ebytes==2
	 *       rbuff=fread(fid,blockpts,'int16');
	 *   else
	 *       if (bitand(fileheader.status, hex2dec('8')) > 0)    <-------------same thing here.
	 *         rbuff=fread(fid,blockpts,'float32');
	 *       else
	 *           rbuff=fread(fid,blockpts,'int32');
	 *       end
	 *   end
	 *
	 */

	fd = fopen(outpixfilename, "w");
	if(fd == NULL)
	{
		fprintf(stderr, "Error opening pixel output file: %d\n", errno);
		return(1);
	}

	floatfd = fopen(FLOATPIXELFILE, "w+");
	if(floatfd == NULL)
	{
		fprintf(stderr, "Error opening raw float file for write: %d - %s\n", errno, strerror(errno));
		return(1);
	}

	FILE *pixfd = fopen("textPixels.dat", "w");

	totalPixels = 0;
	fdfValues->slices = filehdr.nblocks;			//%^% TODO: verify
	for(i = 0 ; i < filehdr.nblocks ; i++)
	{
		// First, read the data block header
		if(fread(&blkhdr, sizeof(blkhdr), 1, fidfd) != 1)
		{
			fprintf(stderr, "Error reading FID data header: %d, %s", errno, strerror(errno));
			return(1);
		}
		DATABLOCKHEADER_CONVERT_NTOH(&blkhdr);

		// Read and convert the pixel data for the current block
		scale = 1.0 / (float)(blkhdr.ctcount);
		pixelBuf = (char *)malloc(pixelBufSize);
		if(pixelBuf == NULL)
		{
			fprintf(stderr, "Error allocating pixel buffer: %d\n", errno);
			return(1);
		}
		pixels = fread(pixelBuf, pixelSize, numPixels, fidfd);
		totalPixels += pixels;

		// Find the min and max pixel values to calculate the appropriate
		// scaling factor to apply
		minPixel = 10000000000.0;
		maxPixel = 0.0;

		// square both numbers and sum then take square root
		// Convert the pixel data to 16-bit integers
/*		 -0.3396
		  1.1885
		 -1.6995
		  1.2959
		  1.6328
		  2.8730
		  1.7778
		  2.0483
		 -0.3248
		  0.4938
		 -0.7499
		 -0.4147
		  1.7842
		  3.3172
		  0.7514
		 -1.3876
		 -1.9200
		 -3.9479
		 -2.0409
		 -2.5850
                  for(iro=0;iro<2*ro2;iro++)
                  {
                      memcpy(&l1, fdataptr, sizeof(int));
                      fdataptr++;
                      l1=ntohl(l1);  <----------------- byte swap for longs (where I copied float data)
                      memcpy(&ftemp,&l1, sizeof(int));
                      *(datptr+soffset+iro)=nt_scale*ftemp;
                  }
*/
		uint32_t		l1;
		for(x = 0 ; x < numPixels ; x++)
		{
			memcpy(&l1, &pixelBuf[x * sizeof(int32_t)], sizeof(int32_t));
			l1 = ntohl(l1);
			memcpy(&pix, &l1, sizeof(int));
			if(pix < minPixel) minPixel = pix;
			if(pix > maxPixel) maxPixel = pix;
			fwrite(&pix, 1, sizeof(float), floatfd);
			fprintf(pixfd, "%f\n", pix);
		}
	}
	fclose(pixfd);
	rewind(floatfd);

	// Calculate the scaling factor based on the read pixels
	float		scaleFactor = MAX_PIXEL_VALUE / maxPixel;
	printf("Scale Factor: %f\n", scaleFactor);

	// Read the floating point pixels we just wrote and apply the scaling factor
	// calculated above.  Then write out the converted pixels into the file that
	// will be added to the DICOM data
	FILE *intfd = fopen("intPixels.dat", "w");
	for(x = 0 ; x < totalPixels ; x++)
	{
		int ret = fread(&pix, sizeof(float), 1, floatfd);
		if(ret != 1)
		{
			fprintf(stderr, "Error reading raw float pixels: %d - %s\n", errno, strerror(errno));
			return(1);
		}
		intPixel = (short)(pix * scaleFactor);
		fwrite(&intPixel, 1, sizeof(unsigned short), fd);
		fprintf(intfd, "%d\n", intPixel);
	}
	fclose(intfd);
	fclose(floatfd);
	remove(FLOATPIXELFILE);

	fclose(fd);
	return(0);
}
