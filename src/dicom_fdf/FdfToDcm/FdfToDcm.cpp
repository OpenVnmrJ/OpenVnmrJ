/*
 * Copyright (C) 2017  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
//============================================================================
// Name        : FdfToDcm.cpp
// Author      : Varian, Inc.
// Version     : 1.0
// Description : This program will process a series of one or more FDF
// 				 image files and convert them to DICOM compliant image
//				 files.
//				 The program will take either a single file as input or
//				 a directory.  In the case of a directory, the application
//				 will determine which files should be processed for a given
//				 dataset.
//============================================================================

#include <fstream>
#include <iostream>
using namespace std;

#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

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

#define PROCPARTAGHEADER	"\n***** BEGIN PROCPAR INCLUSION *****\n"
#define PROCPARTAGFOOTER	"\n***** END PROCPAR INCLUSION *****\n"
#define NSHA1 10

static 	fdfEntries_t		fdfValues;

static void makesha1uid(char *filepath, char *filename, char *uid);

extern double   localqrsdelay;
extern double   localidelay;

void 	usage(void);

int main(int argc,char **argv)
{
	bool 				bad=false;
	bool				haveProcpar=false;
	bool				haveTagFile=false;
	bool				isfdf=true;
	bool 				verbose = false;
	bool				insertProcpar = true;
	unsigned char		c = 0xFF;
	char				scratch[256];
	char                            patient_pos[256];
	char				dicomOutputFilename[PATH_MAX];
	char				procparPath[PATH_MAX];
	char				customtagPath[PATH_MAX];
	char				inputfilename[PATH_MAX];
	char				outdcmfilename[PATH_MAX];
	char				uidSuffix[64];
	char				studyUID[128];
	char				seriesUID[128];
	char 				*pixelBuf;
        char                            syscmd[256];
        char                            stdid[256];
        char                            sha1[64];
        char                            uidpbuf[NSHA1];
        char                            uidfbuf[NSHA1];
        char                            tempbuf[256];

	int				i,j;
	int 				count;
	int				pixelBufSize;
	int				numPixels;
	int 				pixels;
	float				minPixel;
	float				maxPixel;
	float				globalMin = 10000000000.0;;
	float				globalMax = 0.0;
	float				*fptr;
	short 				intPixel;
	time_t				curTime;
	struct timeval                  tvp;
        struct timezone                 tzp;
	float 				ctr[3];
	float				R[9];
	float				dc[6];
	float               dctmp;
	float				loc[3];
	float				scaleFactor = 1.0;
	double				psi = 0.0;
	double 				phi = 0.0;
	double 				theta = 0.0;
	double				lro = 0.0;
	double				lpe = 0.0;
	
	const Uint16 		bitsAllocated=BITS_ALLOCATED;
	const Uint16 		bitsStored=BITS_STORED;
	const Uint16 		highBit=HIGH_BIT;

	struct dirent 		**namelist;
	int 				nFiles;
	FILE				*fd, *procparfd, *customfd, *fdf_fd, *fsha;

	GetNamedOptions 		options(argc,argv);
	InputOptions			input_options(options);
	DicomOutputOptions 		dicom_output_options(options);
	TextOutputStream 		log(cerr);
	verbose = options.get("verbose") || options.get("v");
	if(options.get("np"))
	{
		insertProcpar = false;
	}

	// Read and validate required options
	// Complete path to input file(s)
	const char	*inputpath=input_options.filename;
	if(inputpath == NULL)
	{
		cerr << EMsgDC(NeedOption) << " -input-file | -if" << endl;
		bad = true;
	}

	// Full path to the desired output file.
	const char 	*outputpath=dicom_output_options.filename;
	if(outputpath == NULL)
	{
		cerr << EMsgDC(NeedOption) << " -output-path | -of" << endl;
		bad = true;
	}

	if(!bad)
	{
		nFiles = scandir(inputpath, &namelist, 0, alphasort);
		if(nFiles < 0)
		{
			perror("scandir");
			bad = true;
		}
	}

	if(bad)
	{
		usage();
		exit(1);
	}

	// Read all image files and determine the min and max pixel value of the entire
	// study or series. These values will be used to calculate a global scaling
	// factor that will be applied to all images in the specified directory.
	GetScaleFactor(inputpath, &globalMin, &globalMax);
	printf("Global min/max: %f  %f\n", globalMin, globalMax);

	// Save the current time in case there's no procpar and we need to construct UID's
	time(&curTime);

	// Root input directory.  Contains images, procpar and custom tags
	printf("Input Directory = %s\n", inputpath);

	// Create the file path for the procpar file
	strcpy(procparPath, inputpath);
	strcat(procparPath, PROCPAR_FILENAME);
	printf("Procpar path: %s\n", procparPath);

	// Open the procpar file which will be used for all images in the
	// user specified directory.
	procparfd = fopen(procparPath, "r");
	if(procparfd != NULL)
	{
	  (void)strcpy(tempbuf,inputpath);
	  (void)makesha1uid(tempbuf,PROCPAR_FILENAME, uidpbuf);

	   haveProcpar = true;
	   fclose(procparfd);
	   sprintf(uidSuffix, "%s", uidpbuf);
	}
	else
	{
		printf("Procpar file not found in specified input directory, ignoring...\n");
		sprintf(uidSuffix, "%s",inputpath);    // use input path for studyid
	}

	// Create unique series and study instance UID values
	// Use the base string combined with the system time, hostid and pid
	//Example: <1.3.6.1.4.1.670589.1.2.0.127440.31032.8323329>
	// sprintf(uidSuffix, "%d.%d.%d", (unsigned int)(time(0) / 10000),
	//	(unsigned int)getpid(), (unsigned int)gethostid());
	//	sprintf(uidSuffix, "%s.%d.%d", sha1,
	//	(unsigned int)getpid(), (unsigned int)gethostid());



	// sprintf(studyUID, "1.3.6.1.4.1.%s.1.2.0.%s", VARIAN_COMPANY_ID, uidSuffix);

	//UI Series Instance UID 	 VR=<UI>   VL=<0x0032>
	//Example: <1.3.6.1.4.1.670589.1.3.0.123.127440.31032.8323329>
	sprintf(seriesUID, "1.3.6.1.4.1.%s.1.3.0.123.%s", VARIAN_COMPANY_ID, uidSuffix);

	while(nFiles--)
	{
		// See if the next entry in the directory is an FDF file
		if(strstr(namelist[nFiles]->d_name, ".fdf") != NULL)
		{
			// An FDF file was found, so we will process the file
			printf("Processing file: %s\n", namelist[nFiles]->d_name);
			isfdf = true;
		}
		else if(strcmp(namelist[nFiles]->d_name, "fid") == 0)
		{
			// An FID file was found, so we will process the file
			printf("Processing file: %s\n", namelist[nFiles]->d_name);
			isfdf = false;
		}
		else
		{
			// Skip the file if it's not an FDF or FID file
			continue;
		}

		// Get the input FDF or FID filename
		sprintf(inputfilename, "%s/%s", inputpath, namelist[nFiles]->d_name);

		// Create the DICOM output path using the user supplied path and the
		// base filename of the FDF file being processed
		if(isfdf)
		{
			sprintf(outdcmfilename, "%s/%s", outputpath, namelist[nFiles]->d_name);
			strcpy(&outdcmfilename[strlen(outdcmfilename) - 3], "dcm");
		}
		else
		{
			sprintf(outdcmfilename, "%s/fid.dcm", outputpath);
		}

		// Open the FDF or FID file for processing
		fd = fopen(inputfilename, "rb");
		if(fd == NULL)
		{
			printf("Error opening file: %s\n", inputfilename);
			exit(1);
		}

		// get sha1 for SOP UID
		(void)strcpy(tempbuf,inputpath);
		(void)makesha1uid(tempbuf,namelist[nFiles]->d_name, uidfbuf);

		
		ManagedAttributeList	list;
		// Insert the Study and Series instance UIDs
		//UI Study Instance UID 	 VR=<UI>   VL=<0x002e>
		//<1.3.6.1.4.1.670589.1.2.0.127440.31032.8323329>
		//		list+=new UIStringAttribute(TagFromName(StudyInstanceUID), studyUID);

		//UI Series Instance UID 	 VR=<UI>   VL=<0x0032>
		//<1.3.6.1.4.1.670589.1.3.0.123.127440.31032.8323329>
		list+=new UIStringAttribute(TagFromName(SeriesInstanceUID), seriesUID);

		// Read the procpar file if provided by the user and convert the
		// available values to DICOM tags
		procparfd = fopen(procparPath, "r");
		if(procparfd == NULL)
		{
			fprintf(stderr, "Unable to open procpar file...ignoring.\n");
			haveProcpar = false;
		}
		else
		{
			if(ProcessProcpar(procparfd, list, &psi, &phi, &theta, &lro, &lpe, &fdfValues) != 0)
			{
				fprintf(stderr, "Error processing procpar file!!\n");
			}
			fclose(procparfd);

			if(!isfdf) fdfValues.matrix[0] *= 2;		// TODO: Verify
			haveProcpar = true;
		}

		// Insert the Study and Series instance UIDs
		if(isfdf)
		{
			if(ProcessFdfHeader(fd, &fdfValues) != 0)
			{
				printf("Error processing FDF header!!\n");
				fclose(fd);
				exit(1);
			}

			// For FDF files...
			// Find the beginning of the pixel data by detecting the first
			// NULL byte in the file.  Convert the image pixels from 32-bit
			// floating point to 16-bit grayscale signed integers and apply
			// a reasonable scaling factor
			c = 0xFF;
			while(c != '\0')
			{
				if(fread(&c, 1, 1, fd) != 1)
				{
					fprintf(stderr, "Error locating end of FDF header!!", errno);
				}
			}

			if(fdfValues.echoes < 1)fdfValues.echoes=1;
			if(fdfValues.rank == 2)
			{
				numPixels = fdfValues.matrix[0] * fdfValues.matrix[1];
			}
			else
			{
				numPixels = fdfValues.matrix[0] * fdfValues.matrix[1] *
						   fdfValues.matrix[2] * fdfValues.echoes;
			}
			int		pixelBufSize = numPixels * sizeof(float);
			char 	*pixelBuf = (char *)malloc(pixelBufSize);
			if(pixelBuf == NULL)
			{
				printf("Error allocating pixel buffer: %d\n", errno);
				fclose(fd);
				exit(1);
			}
			pixels = fread(pixelBuf, 1, pixelBufSize, fd);
			printf("Pixel bytes read: %d\n", pixels);
			// leave fdf file open and save descriptor for checking custom tags 
			// fclose(fd);
			fdf_fd=fd; fd=NULL; 
			fptr = (float *)pixelBuf;
	
			// if 3D we need to rotate each 2D slice to mimic 2D data
			if(fdfValues.rank == 3)
			  {
			    int iz, ix, iy;
			    int nx=fdfValues.matrix[0];
			    int ny=fdfValues.matrix[1];
			    int nz=fdfValues.matrix[2];
			    float *pixelFrame=(float *)malloc(sizeof(float)*nx*ny);
			    float *pptr=pixelFrame;

			    for(iz=0;iz<nz;iz++)
			      {
				pptr=pixelFrame;
				for(ix=nx-1;ix>-1;ix--)
				  {
				    for(iy=ny-1;iy>-1;iy--)
				      {
					*pptr++ = *(fptr + iz*nx*ny + iy*nx + ix);
				      }
				  }
				for(ix=0;ix<nx*ny;ix++)
				  *(fptr + iz*nx*ny + ix) = *(pixelFrame + ix);
			      }
			    fdfValues.matrix[0]=ny;
			    fdfValues.matrix[1]=nx;
			    (void)free(pixelFrame);
				    
			  }
				

			// Find the min and max pixel values to calculate the appropriate
			// scaling factor to apply
/*%^%			minPixel = 10000000000.0;
			maxPixel = 0.0;

			for(i = 0 ; i < numPixels ; i++)
			{
				if(fptr[i] < minPixel) minPixel = fptr[i];
				if(fptr[i] > maxPixel) maxPixel = fptr[i];
			}
%^%*/


			printf("Study Min Pixel: %f\n", globalMin);
			printf("Study Max Pixel: %f\n", globalMax);
			minPixel = globalMin;
			maxPixel = globalMax;

			// Calculate the scaling factor based on the read pixels
			scaleFactor = MAX_PIXEL_VALUE / maxPixel;
			scaleFactor /= 2.0;      // allow for signed data

			// Store the global scaling factor
			/*
			list+=new DecimalStringAttribute(TagFromName(RescaleSlope),
					scaleFactor * (fdfValues.imagescale == 0.0 ? 1.0 : fdfValues.imagescale));
			*/

			// Now write out the raw pixels
			fd = fopen(outpixfilename, "w");
			if(fd == NULL)
			{
				printf("Error opening pixel output file: %d\n", errno);
				exit(1);
			}
			printf("Output file %s opened for writing...\n", outpixfilename);

			for(i = 0 ; i < (pixels / sizeof(float)) ; i++)
			{
				intPixel = (short)(fptr[i] * scaleFactor);
				fwrite(&intPixel, 1, sizeof(unsigned short), fd);
			}
			printf("Scaling factor: %f\n", scaleFactor);
			fclose(fd);
			printf("Pixel bytes written: %d\n", pixels);
		}	// End if(isfdf)
		else
		{
			ProcessFidData(fd, list, &fdfValues);
		}
		
		// Setup the variables used to process the individual tags from the basic DICOM
		// set, the procpar file and the FDF header
		InputOpenerFromOptions input_opener(options,outpixfilename,cin);
		DicomOutputOpenerFromOptions output_opener(options,outdcmfilename,cout);

		bool 		littleendian=true;
		const char 	*photometricInterpretation="MONOCHROME2";
		unsigned 	planarConfiguration=0;

		Assert(photometricInterpretation);

		// Now all the pixel processing has been done, complete filling in
		// and finalizing the DICOM option objects
		input_options.done();
		dicom_output_options.done();
		options.done();

		cerr << input_options.errors();
		cerr << dicom_output_options.errors();
		cerr << options.errors();
		cerr << input_opener.errors();
		cerr << output_opener.errors();

		if (!input_options.good() || !dicom_output_options.good() ||
			!options.good() || !input_opener.good() || !output_opener.good() ||
			!options || bad)
		{
			usage();
			exit(1);
		}

		istream in(input_opener);
		DicomOutputStream dout(*(ostream *)output_opener,
			dicom_output_options.transfersyntaxuid,
			dicom_output_options.usemetaheader,
			dicom_output_options.useimplicitmetaheader,
			dicom_output_options.addtiff);

		TransferSyntax transfersyntax(
			dicom_output_options.transfersyntaxuid
			? dicom_output_options.transfersyntaxuid
			: DefaultTransferSyntaxUID);

		list+=new UnsignedShortAttribute(TagFromName(BitsAllocated), bitsAllocated);
		list+=new UnsignedShortAttribute(TagFromName(BitsStored), bitsStored);
		list+=new UnsignedShortAttribute(TagFromName(HighBit), highBit);

		if(!haveProcpar)
		{
			// TODO: Fill in SOPClassUID?
			list+=new UIStringAttribute(TagFromName(SOPClassUID), "1.2.840.10008.5.1.4.1.1.4");
		}

		char sopString[128] = "";
		int ttime;
		time(&curTime);
		gettimeofday(&tvp,&tzp);
		ttime=(int)curTime + (int)(tvp.tv_usec);
		//	printf("time is %d \n",ttime);

		// printf("PP UID is %s\n",uidpbuf);
		// printf("FILE UID is %s\n",uidfbuf);


		//		sprintf(sopString, "1.3.6.1.4.1.670589.1.1.0.123.0.128105.6430.%d", ttime);
		sprintf(sopString, "1.3.6.1.4.1.670589.1.1.0.123.0.128105.6430.%s%s", uidpbuf,uidfbuf);
		list+=new UIStringAttribute(TagFromName(SOPInstanceUID), sopString);

		list+=new UnsignedShortAttribute(TagFromName(SamplesPerPixel), 1);
		if(isfdf)
		{
		  //			list+=new UnsignedShortAttribute(TagFromName(PixelRepresentation), 0);
			list+=new UnsignedShortAttribute(TagFromName(PixelRepresentation), 1);
		}
		else
		{
			list+=new UnsignedShortAttribute(TagFromName(PixelRepresentation), 1);
		}
		list+=new CodeStringAttribute(TagFromName(PhotometricInterpretation), "MONOCHROME2");
		list+=new CodeStringAttribute(TagFromName(Laterality), "");
		list+=new CodeStringAttribute(TagFromName(ImageType),"ORIGINAL\\PRIMARY\\OTHER");
		list+=new LongStringAttribute(TagFromName(PositionReferenceIndicator), "");
		list+=new CodeStringAttribute(TagFromName(ScanOptions), "");

		list+=new CodeStringAttribute(TagFromName(MRAcquisitionType), "");

		unsigned frames = isfdf ? 1 : fdfValues.slices;		//%^%fdfValues.echoes * fdfValues.slices;

	    
	       	
		list+=new UnsignedShortAttribute(TagFromName(Columns), fdfValues.matrix[0]);
		list+=new UnsignedShortAttribute(TagFromName(Rows), fdfValues.matrix[1]);
		  
		list+=new IntegerStringAttribute(TagFromName(EchoNumber), (Int32)fdfValues.echo_no);

		if(strlen(fdfValues.studyid) < 3)
		{
			strcpy(fdfValues.studyid, "12345");
		}
		if(strlen(fdfValues.oper) == 0)
		{
			strcpy(fdfValues.oper, "");
		}
		if(strlen(fdfValues.pname) == 0)
		{
			strcpy(fdfValues.pname, "Anon");
		}

		// get rid of non-digits
		j=0;
		for(i=0;i<strlen(fdfValues.studyid);i++)
		  {
		    if((isdigit(fdfValues.studyid[i])) && (fdfValues.studyid[i] != '0'))
		      stdid[j++]=fdfValues.studyid[i];
		  }
		stdid[j]='\0';

		//sprintf(studyUID, "1.3.6.1.4.1.%s.1.2.0.%d", stdid, (unsigned int)gethostid());
	
		 sprintf(studyUID, "1.3.6.1.4.1.%s.1.2.0.%s_%s", strtok(fdfValues.studyid,"\""), strtok(fdfValues.oper,"\""),strtok(fdfValues.pname,"\""));

		list+=new UIStringAttribute(TagFromName(StudyInstanceUID), studyUID);


		list+=new ShortStringAttribute(TagFromName(StudyID), strtok(fdfValues.studyid,"\""));
		list+=new IntegerStringAttribute(TagFromName(SeriesNumber), "123");
		sprintf(scratch, "%d", fdfValues.slice_no * fdfValues.echo_no);
		list+=new IntegerStringAttribute(TagFromName(InstanceNumber), scratch);
		list+=new CodeStringAttribute(TagFromName(Modality),"MR");
		list+=new LongStringAttribute(TagFromName(Manufacturer), "Agilent Technologies");
		list+=new ShortTextAttribute(TagFromName(DerivationDescription),
					"Converted to DICOM by FdfToDcm utility from Agilent proprietary format");
		sprintf(scratch, "%d", fdfValues.slices * fdfValues.echoes);
		list+=new IntegerStringAttribute(TagFromName(ImagesInAcquisition), scratch);

		sprintf(scratch, "%f", fdfValues.location[2]);
		list+=new DecimalStringAttribute(TagFromName(SliceLocation), scratch);

		if((localqrsdelay > -1.0) || (localidelay > -1.0))
		  {
		    if(localqrsdelay < 0.0) localqrsdelay=0.0;
		    if(localidelay < 0.0) localidelay=0.0;

		    sprintf(scratch,"%f",(localqrsdelay + (fdfValues.echo_no - 1)*localidelay)*1000.0);
		    list+=new DecimalStringAttribute(TagFromName(TriggerTime), scratch);
		  }


		switch(fdfValues.position1[0])
		{
		case 'f':
			strcpy(scratch, "FF");
			break;
		case 'h':
			strcpy(scratch, "HF");
			break;
		default:
			strcpy(scratch, "");
		}
		if(scratch[0] != '\0')
		{

			switch(fdfValues.position2[0])
			{
			case 'p':
				strcat(scratch, "P");
				break;
			case 's':
				strcat(scratch, "S");
				break;
			case 'l':
				strcat(scratch, "DL");
				break;
			case 'r':
				strcat(scratch, "DR");
				break;
	
			default:
				break;
			}
		}

		list+=new CodeStringAttribute(TagFromName(PatientPosition), scratch);
		strcpy(patient_pos,scratch);

		// Calculate PixelSpacing based on procpar values lro and lpe
		lpe = lpe * 10.0 / (double)fdfValues.matrix[0];
		lro = lro * 10.0 / (double)fdfValues.matrix[1];
		sprintf(scratch, "%f\\%f", lpe, lro);
		list+=new DecimalStringAttribute(TagFromName(PixelSpacing), scratch);

		// Calculate and process the positional information for 
		// the ImageOrientationPatient tag
		if((fdfValues.rank == 3) || (!isfdf))
		  //	if((fdfValues.slices > 1) || (!isfdf))
		{
			double	cosphi, sinphi;
			double	cospsi, sinpsi;
			double	costheta, sintheta;
			// 3D case
			// Get psi, phi and theta from the procpar file
			// Convert the angles from degrees to radians by multiplying by (PI/180.)
			// Compute sin() and cos() of all three angles
			// Compute the following to fill in the 6 element field:
			//
			//    or0= -1*cosphi*cospsi - sinphi*costheta*sinpsi;
			//    or1= cosphi*sinpsi -  sinphi*costheta*cospsi;
			//    or2= sinphi*sintheta;
			//    or3= -1*sinphi*cospsi + cosphi*costheta*sinpsi;
			//    or4= sinphi*sinpsi  + cosphi*costheta*cospsi;
			//    or5= -1*cosphi*sintheta;
			psi *= (M_PI / 180.0);
			phi *= (M_PI / 180.0);
			theta *= (M_PI / 180.0);
			cospsi = cos(psi);
			cosphi = cos(phi);
			costheta = cos(theta);
			sinpsi = sin(psi);
			sinphi = sin(phi);
			sintheta = sin(theta);
			dc[0] = ((-1) * cosphi * cospsi) - (sinphi * costheta * sinpsi);
			dc[1] = (cosphi * sinpsi) - (sinphi * costheta * cospsi);
			dc[2] = (sinphi * sintheta);
			dc[3] = ((-1) * sinphi * cospsi) + (cosphi * costheta * sinpsi);
			dc[4] = (sinphi * sinpsi) + (cosphi * costheta * cospsi);
			dc[5] = ((-1) * cosphi * sintheta);
		}
		else
		{


			// 2D case
			// 1) Get “orientation” matrix form the FDF header, which is has 9 elements
			// 2) Copy the first 6 of these to this field
			// 3) Change the sign of elements 1, 2, 4, and 5 (with numbering starting at 0)
			dc[0] = fdfValues.orientation[0];
			dc[1] = fdfValues.orientation[1] * (-1.0);
			dc[2] = fdfValues.orientation[2] * (-1.0);
			dc[3] = fdfValues.orientation[3];
			dc[4] = fdfValues.orientation[4] * (-1.0);
			dc[5] = fdfValues.orientation[5] * (-1.0);
		}

		// Factor in the patient position here
		if(strstr(patient_pos,"FF"))
		  {
		    dc[0] *= -1;
		    dc[2] *= -1;
		    dc[3] *= -1;
		    dc[5] *= -1;
		  }
		if(strstr(patient_pos,"P"))
		  {
		    dc[0] *= -1;
		    dc[1] *= -1;
		    dc[3] *= -1;
		    dc[4] *= -1;
		  }
		if(strstr(patient_pos,"DR") || strstr(patient_pos,"DL"))
		  {
		    dctmp = dc[0];
		    dc[0] = dc[1];
		    dc[1] = dctmp;

		    dctmp = dc[3];
		    dc[3] = dc[4];
		    dc[4] = dctmp;

		    if(strstr(patient_pos,"DR"))
		      {
			dc[0] *= -1;
			dc[1] *= -1;
			dc[3] *= -1;
			dc[4] *= -1;
		      }
		    else
		      {
			dc[1] *= -1;
			dc[4] *= -1;
		      }
		  }



		sprintf(scratch, "%f\\%f\\%f\\%f\\%f\\%f", dc[0], dc[1], dc[2], dc[3], dc[4], dc[5]);
		list+=new DecimalStringAttribute(TagFromName(ImageOrientationPatient), scratch);

		// Now calculate the values for ImagePositionPatient based on the orientation
		// values from above and the values contained in the fdf span field. 

		// Calculations are done differently depending on whether we're converting a
		// 2D acquisition or a 3D acquisition.
		if(fdfValues.rank == 2)
		{
			// 1) Figure out distance from center of image to upper left corner:
			// make unrotated x, y, z coords. (I am calling it “ctr[3]” for this pseudo-code)
			// from fdf header, get the 2 element field called “span”. Change sign
			// of both elements and divide them by 2 AND MULT. BY 10.
			// This is the x and y coordinates of the upper left corner:

			// ctr(0)= -5.*span(0); ctr(1)= -5.*span(1); ctr(2)= 0;
			ctr[0] = (-5.0) * fdfValues.span[0];
			ctr[1] = (-5.0) * fdfValues.span[1];
			ctr[2] = 0;

			// 2) add the slice-dependent location offset
			// from fdf header, get "location" (a 3 element vector).
			// Multiply all 3 elements by 10 to convert to millimeters, then add to the ctr vector:

			// ctr(0) += 10*location(0); ctr(1) += 10*location(1); ctr(2) += 10*location(2);
			ctr[0] += 10.0 * fdfValues.location[0];
			ctr[1] += 10.0 * fdfValues.location[1];
			ctr[2] += 10.0 * fdfValues.location[2];
		}
		// Use the 2D calculations if the fdf matrix field only has 2 values.
		else if(fdfValues.rank == 3)
		{
			// 1) Figure out distance from center of image to upper left corner:
			// For “ctr”, get 3 element “origin” from fdf header and set ctr(0->2) to this vector.

			// ctr(0)= origin(0); ctr(1)= origin(1); ctr(2)= origin(2);
			
			ctr[0] = fdfValues.origin[0];
			ctr[1] = fdfValues.origin[1];
			ctr[2] = fdfValues.origin[2];



			// 2) add the slice-dependent location offset (DO THIS ONLY IF GENERATING SEPARATE SLIICES!!)
			// If you are outputting separate slices do the following:
			// Get the 3 element “span” vector from the 3d fdf header
			// For each slice add to the 3rd element of ctr the quantity:
			// (slice_no)*thk/(slices)
			// where thk = span(2) (total slab thickness)
			// and slice_no starts from 0, and “slices” comes from fdf header.

			// ctr(0) += islice * span(2) /slices <--- islice is the current slice being worked on


			// We are outputting the entire slab into 1 dcm file, just skip the above and go to step 3
			
		}
		// 3) do the rotation
		// For this you need the values of_* Image Orientation (patient) (0020,0037) .*_
		// I think you have worked on these already.
		// (/Please make sure that they depend on the patient position
		// (head first, supine, etc.) that came out of the procpar.)
		// Get those direction cosines (the values of (0020,0037)) , which I will call “dc”.
		// Copy to a 3X3 matrix (“R”) and compute the cross products of first 2 rows and put result in the 3rd row, as follows:

		// R(0) = dc(0); <---------which is first element of tag(0020,0037)
		// R(1) = dc(1);
		// R(2) = dc(2);
		// R(3) = dc(3);
		// R(4) = dc(4);
		// R(5) = dc(5);
		R[0] = dc[0];
		R[1] = dc[1];
		R[2] = dc[2];
		R[3] = dc[3];
		R[4] = dc[4];
		R[5] = dc[5];
		
		// For the last 3, do the cross product using the previous elements as follows:
		// R(6)=R(1)*R(5) - R(2)*R(4)
		// R(7)=R(2)*R(3) - R(0)*R(5)
		// R(8)=R(0)*R(4) – R(1)*R(3)
		R[6] = (R[1] * R[5]) - (R[2] * R[4]);
		R[7] = (R[2] * R[3]) - (R[0] * R[5]);
		R[8] = (R[0] * R[4]) - (R[1] * R[3]);
		
		// Now do the matrix multiplication of R*ctr, with ctr being a column vector, and the result will be the location of the upper left pixel (“loc”).
		// loc(0)=R(0)*ctr(0) + R(3)*ctr(1) + R(6)*ctr(2)
		// loc(1)=R(1)*ctr(0) + R(4)*ctr(1) + R(7)*ctr(2)
		// loc(2)=R(2)*ctr(0) + R(5)*ctr(1) + R(8)*ctr(2)
		loc[0] = (R[0] * ctr[0]) + (R[3] * ctr[1]) + (R[6] * ctr[2]);
		loc[1] = (R[1] * ctr[0]) + (R[4] * ctr[1]) + (R[7] * ctr[2]);
		loc[2] = (R[2] * ctr[0]) + (R[5] * ctr[1]) + (R[8] * ctr[2]);
		
		// Done! "loc" should be copied to the Image Position (patient) tag (0020,0032).				
		sprintf(scratch, "%f\\%f\\%f", loc[0], loc[1], loc[2]);
		list+=new DecimalStringAttribute(TagFromName(ImagePositionPatient), scratch);

		// If there are 3 dimension values in the FDF matrix field, add the number of frames
		// and slices based on that value.  If there are only two fields in the matrix, use
		// the value of slices.
		if(fdfValues.rank == 3)
		{
			frames = (int)fdfValues.matrix[2];
			sprintf(scratch, "%d", (int)fdfValues.matrix[2]);
			list+=new IntegerStringAttribute(TagFromName(NumberOfFrames), scratch);
			list+=new UnsignedShortAttribute(TagFromName(NumberOfSlices), fdfValues.matrix[2]);

			float lpe2;
			lpe2 = 10.0*fdfValues.roi[2];
			if(fdfValues.matrix[2] != 0)
			  lpe2 /= (float)fdfValues.matrix[2];
			sprintf(scratch,"%f", lpe2);
			list+=new DecimalStringAttribute(TagFromName(SliceThickness), scratch);
			list+=new DecimalStringAttribute(TagFromName(SpacingBetweenSlices), scratch);

		}
		else if(!isfdf)
		{
			frames = fdfValues.slices;
			sprintf(scratch, "%d", frames);
			list+=new IntegerStringAttribute(TagFromName(NumberOfFrames), scratch);
			list+=new UnsignedShortAttribute(TagFromName(NumberOfSlices), fdfValues.slices);
		}

		// Process the custom tag file if it exists.
		// Check the input directory first, then the other paths defined by CUSTOMTAG_DIRS
		// Example of how to add tag from the group,element pair
		// list+=new DecimalStringAttribute(Tag(0x0018, 0x0083), "10");

		// Check and see if the custom tag file exists in the input directory
		// Create the file path for the procpar file

		// Try finding the custom tag file in the input image directory first
		struct stat		sb;
		haveTagFile = false;
		strcpy(customtagPath, inputpath);
		strcat(customtagPath, CUSTOM_FILENAME);
		if(stat(customtagPath, &sb) != -1)
		{
			haveTagFile = true;
			printf("Found custom tag file: %s\n", customtagPath);
		}
		else if(errno != ENOENT)
		{
			fprintf(stderr, "Unexpected error from stat() call: %d\n", errno);
		}
		// If it wasn't in the image directory, look in ~/vnmrsys/
		if(!haveTagFile)
		{
			char *homeDir = getenv("HOME");
			if(homeDir != NULL)
			{
				sprintf(customtagPath, "%s/vnmrsys/%s", homeDir, CUSTOM_FILENAME);
				if(stat(customtagPath, &sb) != -1)
				{
					haveTagFile = true;
					printf("Found custom tag file: %s\n", customtagPath);
				}
				else if(errno != ENOENT)
				{
					fprintf(stderr, "Unexpected error from stat() call: %d\n", errno);
				}
			}
		}
		// Finally, if we haven't found the custom file, check /vnmr/user_templates/plot
		if(!haveTagFile)
		{
			sprintf(customtagPath, "/vnmr/user_templates/plot/%s", CUSTOM_FILENAME);
			if(stat(customtagPath, &sb) != -1)
			{
				haveTagFile = true;
				printf("Found custom tag file: %s\n", customtagPath);
			}
			else if(errno != ENOENT)
			{
				fprintf(stderr, "Unexpected error from stat() call: %d\n", errno);
			}
		}

		// If we found the file, process the custom tags
		if(haveTagFile)
		{
		  ProcessCustomTags(customtagPath, procparPath, fdf_fd, list);
		}
		else
		{
			printf("Custom tag file not found, ignoring...\n");
		}
		fclose(fdf_fd); // now done with fdf file

		// Add procpar file as a custom tag
//		OtherByteLargeNonPixelAttribute(Tag t,BinaryInputStream &stream,OurStreamPos pos)
//		(0x0029,0x0010) LO PrivateCreator 	 VR=<LO>   VL=<0x0016>  <SIEMENS CSA NON-IMAGE >
//		(0x0029,0x0011) LO PrivateCreator 	 VR=<LO>   VL=<0x0012>  <SIEMENS CSA HEADER>
//		(0x0029,0x0012) LO PrivateCreator 	 VR=<LO>   VL=<0x0016>  <SIEMENS MEDCOM HEADER2>
		list+=new LongStringAttribute(Tag(0x00E1, 0x0010), "Agilent Technologies NMR Non-Image");
		list+=new LongStringAttribute(Tag(0x00E1, 0x0011), "Agilent Technologies NMR Header");
		if(insertProcpar)
		{
			struct stat sb;
			if(stat(procparPath, &sb) == -1)
			{
				perror("stat");
			}
			int tagSize = strlen(PROCPARTAGHEADER);
			tagSize += strlen(PROCPARTAGFOOTER);
			tagSize += sb.st_size;
			char *privateBuf = (char *)malloc(tagSize + 4);
			bzero(privateBuf, tagSize + 4);

			int 	offset = 0;
			sprintf(privateBuf, PROCPARTAGHEADER);
			offset += strlen(privateBuf);

			FILE	*ppfd;
			ppfd = fopen(procparPath, "r");
			if(ppfd != NULL)
			{
				if(fread(&privateBuf[offset], sb.st_size, 1, ppfd) == 1)
				{
					offset += sb.st_size;
					sprintf(&privateBuf[offset], PROCPARTAGFOOTER);
					if((strlen(privateBuf) % 4) != 0)
					{
						strncat(privateBuf, "    ", (strlen(privateBuf) % 4));
					}
					list+=new UnlimitedTextAttribute(Tag(0x00E1, 0x1000), privateBuf);
				}
				else
				{
					fprintf(stderr, "Error reading procpar, private tag will not be included!!\n");
				}
				fclose(ppfd);
			}
			else
			{
				fprintf(stderr, "Error opening procpar, private tag will not be included!!\n");
			}
			free(privateBuf);
		}

		if(isfdf)
		{
			// Place the global scaling factor into a custom tag
			list+=new DecimalStringAttribute(Tag(0x00E1, 0x1001),
					scaleFactor * (fdfValues.imagescale == 0.0 ? 1.0 : fdfValues.imagescale));
		}

		// All of the individual fields have been processed, now handle the pixel data
		Raw_PixelDataSourceBase *pixeldatasrc=0;
		pixeldatasrc=new Raw_PixelDataSource_LittleEndian(in, (long int)0, fdfValues.matrix[0],
							fdfValues.matrix[1],
							isfdf ? frames : fdfValues.slices, 1);
		Assert(pixeldatasrc);

		printf("Adding pixels for %s, frames: %d\n", outdcmfilename, frames);
		list+=new OtherUnspecifiedLargeAttribute(
			TagFromName(PixelData),
			pixeldatasrc,
			fdfValues.matrix[0],
			fdfValues.matrix[1],
			isfdf ? frames : fdfValues.slices,
			1,
			&transfersyntax,
			0, bitsAllocated, bitsStored, highBit);

		if (!usualManagedAttributeListWrite(list,dout,
			dicom_output_options,log,verbose)) return 1;

		free(namelist[nFiles]);
		delete pixeldatasrc;
	}
	free(namelist);
	if(!isfdf) remove(outpixfilename);
	return 0;
}

void	usage(void)
{
	printf("\nUsage:  \n        FdfToDcm <-input-file | -if> <-output-file | -of>\n\n");
}

static void makesha1uid(char *filepath, char *filename, char *uidbuf)
{
  int i,j;
  char sha1Path[256];
  char fpath[256];
  char sha1[NSHA1];
  char   syscmd[256];
  FILE *fsha;

  // get its sha1sum to use for UID
  (void)strcpy(sha1Path,filepath);
  (void)strcat(sha1Path,"/sha1.txt");

  (void)strcpy(fpath,filepath);
  (void)strcat(fpath,"/");
  (void)strcat(fpath,filename);

  (void)sprintf(syscmd,"sha1sum %s > %s \n",fpath,sha1Path);
  (void)system(syscmd);

  //read it in
  fsha=fopen(sha1Path,"r");
  if(!fsha)printf("Could not open sha1 for %s \n",fpath);
  fscanf(fsha,"%10s",sha1);
  (void)fclose(fsha);


  // clean up
  //  (void)sprintf(syscmd,"rm %s \n",sha1Path);
  // (void)system(syscmd);

  // get rid of non-digits
  j=0;
  for(i=0;i<NSHA1;i++)
    {
      if((isdigit(sha1[i])) && (sha1[i] != '0'))
	uidbuf[j++]=sha1[i];
    }
  uidbuf[j]='\0';

  return;
}
  

