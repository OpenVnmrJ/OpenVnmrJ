/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*-*- Mode: C++ -*-*/

static char *Sid() {
    return "@(#)ddlfile.c 18.1 03/21/08 (c)1991 SISCO";
}


 /*
  *
  *  ddlfile.c:     contains routines to read data from new ddl file format
  *                 and build appropriate sisfile data structures.
  *
  *
  */

#include <stdio.h>
// #include <stream.h>
#ifdef LINUX
// #include <strstream>
#else
// #include <strstream.h>
#endif
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <values.h>

#include "ddllib.h"
#include "graphics.h"
#include "gtools.h"
#include "imginfo.h"
#include "stderr.h"
#include "ddlfile.h"
#include "params.h"
#include "gframe.h"
#include "msgprt.h"
#include "file_format.h"

/* temporary defines until a defaults panel is constructed	*/
#define	CHAR_MAX	255
#define SHORT_MAX	65535
#define INT_MAX		2147483647.0

int
Imginfo::memoryCheck(DDLSymbolTable *symtab, char *errmsg, int *mbytesShort)
{
    int i;

    *mbytesShort = 0;			// Amount of additional memory needed

    int rank;
    if ( ! symtab->GetValue("rank", rank) ) {
	sprintf(errmsg,"No \"rank\" FDF entry.");
	return FALSE;
    }
    int bits;
    if ( ! symtab->GetValue("bits", bits) ) {
	sprintf(errmsg,"No \"bits\" FDF entry.");
	return FALSE;
    }
    char *storage;
    if ( ! symtab->GetValue("storage", storage) ) {
	sprintf(errmsg,"No \"storage\" FDF entry.");
	return FALSE;
    }
    int side;
    double words = 1;
    for (i=0; i<rank; i++){
	if ( ! symtab->GetValue("matrix", side, i) ) {
	    sprintf(errmsg,"No \"matrix\" FDF entry for dimension %d.", i+1);
	    return FALSE;
	}
	words *= side;
    }

    /* Do we have enough memory to read this in? */
    double srcbytes = (bits / 8) * words;
    double totbytes = srcbytes;
    if (bits != 32 || (strcasecmp(storage,"float") != 0)) {
	/* Extra memory for dealing with data conversion */
	totbytes += 2 * (srcbytes * 32 / bits);
    }
    int meg = 1 << 20;
    int mbytesReq = (int)((totbytes + (meg - 1) ) / meg);
    if (!memoryCheck(mbytesReq, errmsg, mbytesShort)){
	return FALSE;
    }
    
    return TRUE;
}

int
Imginfo::memoryCheck(int MbytesRequested, char *errmsg, int *MbytesShort)
{
    int meg = 1 << 20;
    if (MbytesRequested > MAXINT/meg){
	sprintf(errmsg,"Need %g bytes memory: too big for integer!",
		(double)MbytesRequested * meg);
	*MbytesShort = -1;
	return FALSE;
    }
    int bytes = MbytesRequested * meg;
    int tst = bytes;
    char *buf;
    if/*while*/ ( tst > 0 && !(buf = (char *)malloc(tst)) ){
	tst -= meg;
    }
    if (buf){
	free(buf);
    }
    if (tst == bytes){
	return TRUE;
    }else{
	/**MbytesShort = (bytes - tst) / meg;
	sprintf(errmsg,"Insufficient memory: need additional %dMbytes.",
		*MbytesShort);*/
	*MbytesShort = -1;
	sprintf(errmsg,"Out of memory. Unload some data, exit other programs, or add more swap space.");
	return FALSE;
    }
}

Imginfo*
Imginfo::ddldata_load(char *filename, char *errmsg, DDLSymbolTable *ddlst)
{

    if (ddlst){
	st = ddlst;
    }else{
	/* Create a DDL symbol table and parse the input file */
	st = ParseDDLFile(filename);// Get header
	int mbytes;
	if (!memoryCheck(st, errmsg, &mbytes)){
	    return NULL;
	}
	if (!st->MallocData()){ // Get data
	    msgerr_print("Cannot read data from %s.", filename);
	    delete st;
	    return NULL;
	}
    }
    if (filename){
	st->SetValue("dirpath", "/");
	st->SetValue("filename", filename);
    }

    /*st->PrintSymbols();/*CMP*/

    int rank = 1;
    if ( ! st->GetValue("rank", rank) ) {
	sprintf(errmsg,"No \"rank\" FDF entry.");
	return NULL;
    }
    
    char *spatial_rank;
    if ( ! st->GetValue("spatial_rank", spatial_rank)){
	if (st->GetValue("subrank", spatial_rank)){
	    st->SetValue("spatial_rank", spatial_rank);
	}else{
	    sprintf(errmsg,"No \"spatial_rank\" entry.");
	    return NULL;
	}
    }

    int bits=0;
    st->GetValue("bits", bits);
    int pixelmax = G_Get_Sizecms2(Gframe::gdev);
    vs = 4200.0 * pixelmax / 127;	// Default vscale value

    char* storage = "";
    st->GetValue("storage", storage);
    if ((strcmp(storage, "float") == 0) && (bits == 32)) {
	type = TYPE_FLOAT;
    } else 
    {
	if (bits < 8) {
	    sprintf(errmsg,"Invalid \"bits\" field.");
	    return NULL;
	}
  	int datasize = st->DataLength() / (bits/8);  
  	float* dataptr = new float[datasize];
	if (dataptr == 0) {
	    sprintf(errmsg,"Unable to allocate memory.");
	    return NULL;
	}

  	if (strcmp(storage, "float") == 0) {
	    if (bits == 64) {
  	  	convertdata( (double *)st->GetData(), dataptr, datasize);
	    }
	    else {
		sprintf(errmsg,"Unsupported bits/storage parameters.");
		return NULL;
	    }
  	} else if (strcmp(storage, "char") == 0) {
	    if (bits == 8) {
		vs =  pixelmax / CHAR_MAX;
  	  	convertdata( (unsigned char *)st->GetData(), dataptr, datasize);
	    }
	    else {
		sprintf(errmsg,"Unsupported bits/storage parameters.");
		return NULL;
	    }
  	} else if (strcmp(storage, "short") == 0) {
	    if (bits == 16) {
		vs = pixelmax / SHORT_MAX;
  	  	convertdata((unsigned short *)st->GetData(),dataptr,datasize);
	    }
	    else {
		sprintf(errmsg,"Unsupported bits/storage parameters.");
		return NULL;
	    }
  	} else if (strcmp(storage, "int") == 0) {
	    if (bits == 8) {
		vs =  pixelmax / CHAR_MAX;
		convertdata( (unsigned char *)st->GetData(),dataptr,datasize);
	    } else if (bits == 16) {
		vs = pixelmax / SHORT_MAX;
		convertdata((unsigned short *)st->GetData(),dataptr,datasize);
	    } else if (bits == 32) {
		vs = pixelmax / INT_MAX;
		convertdata( (int *)st->GetData(), dataptr, datasize);
	    }else {
		sprintf(errmsg,"Unsupported bits/storage parameters.");
		return NULL;
	    }
  	} else if (strcmp(storage, "integer") == 0) {
	    if (bits == 8) {
		vs =  (float)pixelmax / CHAR_MAX;
		convertdata((unsigned char *)st->GetData(),dataptr,datasize);
	    } else if (bits == 16) {
		vs = (float)pixelmax / SHORT_MAX;
		convertdata((unsigned short *)st->GetData(),dataptr,datasize);
	    } else if (bits == 32) {
		vs = (float)pixelmax / INT_MAX;
		convertdata( (int *)st->GetData(), dataptr, datasize);
	    }else {
		sprintf(errmsg,"Unsupported bits/storage parameters.");
		return NULL;
	    }
  	} else {
	    sprintf(errmsg,"Unsupported bits/storage parameters.");
	    return NULL;
  	}
  	type = TYPE_FLOAT;
	st->SetData( (char *)dataptr, datasize*(sizeof(float)));
	st->SetValue("bits", 32);
	st->SetValue("storage", "float");
	delete [] dataptr;
    }

    AutoVscale();/*CMP*/
    double vscale;		// Not currently used
    st->GetValue("vs", vscale);
    //cout << "vs = " << vscale << endl;
  
    return this;
}

void
Imginfo::ddldata_write(char *filename)
{
    DDLSymbolTable *cp = (DDLSymbolTable *)this->st->CloneList();
    int outbits = 32;
    char *storage = "float";
    if (fileformat){
	outbits = fileformat->data_size();
	storage = fileformat->data_type();
    }
    double vscale;
    double voffset = 0;
    int inbits;
    cp->GetValue("bits", inbits);

    cp->SetValue("storage", storage);
    cp->SetValue("bits", outbits);
    cp->SetValue("filename", filename);
    int datawords = cp->DataLength() / (inbits/8);
    if (strcasecmp(storage, "integer") == 0){
	int pixelmax = G_Get_Sizecms2(Gframe::gdev);
	switch (outbits){
	  case 8:
	    {
		unsigned char* dataptr = new unsigned char[datawords];
		vscale = vs * CHAR_MAX / pixelmax;
		convertdata((float *)cp->GetData(),
			    dataptr,
			    datawords,
			    vscale,
			    voffset);
		cp->SetData((char *)dataptr, (outbits/8) * datawords);
		cp->SaveSymbolsAndData(filename);
		delete[] dataptr;
		break;
	    }
	  case 16:
	    {
		unsigned short* dataptr = new unsigned short[datawords];
		vscale = vs * SHORT_MAX / pixelmax;
		convertdata((float *)cp->GetData(),
			    dataptr,
			    datawords,
			    vscale,
			    voffset);
		cp->SetData((char *)dataptr, (outbits/8) * datawords);
		cp->SaveSymbolsAndData(filename);
		delete[] dataptr;
		break;
	    }
	  case 32:
	    {
		int* dataptr = new int[datawords];
		vscale = vs * INT_MAX / pixelmax;
		convertdata((float *)cp->GetData(),
			    dataptr,
			    datawords,
			    vscale,
			    voffset);
		cp->SetData((char *)dataptr, (outbits/8) * datawords);
		cp->SaveSymbolsAndData(filename);
		delete[] dataptr;
		break;
	    }
	  default:
	    msgerr_print("Illegal data size for integer FDF data: %d", outbits);
	    break;
	}
    }else if (strcasecmp(storage, "float") == 0){
	switch (outbits){
	  case 32:
	    {
		cp->SaveSymbolsAndData(filename);
		break;
	    }
	  default:
	    msgerr_print("Illegal data size for float FDF data: %d", outbits);
	    break;
	}
    }else{
	msgerr_print("Illegal data type: %s", storage);
    }
    cp->Delete();
}

void
Imginfo::convertdata(unsigned char *indata, float *cdata, int datasize)
{
   while (datasize-- > 0)
   	*cdata++ = (float) *indata++;
}

void
Imginfo::convertdata(unsigned short *indata, float *cdata, int datasize)
{
   while (datasize-- > 0)
   	*cdata++ = (float) *indata++;
}

void
Imginfo::convertdata(int *indata, float *cdata, int datasize)
{
   while (datasize-- > 0)
   	*cdata++ = (float) *indata++;
}

void
Imginfo::convertdata(double *indata, float *cdata, int datasize)
{
   while (datasize-- > 0)
   	*cdata++ = (float) *indata++;
}

void
Imginfo::convertdata(float *cdata, unsigned char *outdata, int datasize,
		     double scale_factor, double offset)
{
   double temp;
   while (datasize-- > 0)
   {
	temp = (*cdata++)*scale_factor + offset;
	if ( temp > CHAR_MAX) temp = CHAR_MAX;
	if ( temp < 0.0 ) temp = 0.0;
   	*outdata++ = (unsigned char) temp;
   }
}

void
Imginfo::convertdata(float *cdata, unsigned short *outdata, int datasize,
		     double scale_factor, double offset)
{
   double temp;
   while (datasize-- > 0)
   {
	temp = (*cdata++)*scale_factor + offset;
	if ( temp > SHORT_MAX) temp = SHORT_MAX;
	if ( temp < 0.0 ) temp = 0.0;
   	*outdata++ = (unsigned short) temp;
   }
}

void
Imginfo::convertdata(float *cdata, short *outdata, int datasize,
		     double scale_factor, double offset)
{
   double temp;
   double max = 0x7fff;
   double min = -max;

   while (datasize-- > 0)
   {
	temp = (*cdata++)*scale_factor + offset;
	if ( temp > max){
	    temp = max;
	}else if ( temp < min ){
	    temp = min;
	}
   	*outdata++ = (short)temp;
   }
}

void
Imginfo::convertdata(float *cdata, int *outdata, int datasize,
		     double scale_factor, double offset)
{
   double temp;
   double max = 0x7fffffff;
   double min = -max;

   while (datasize-- > 0)
   {
	temp = (*cdata++)*scale_factor + offset;
	if ( temp > max){
	    temp = max;
	}else if ( temp < min ){
	    temp = min;
	}
   	*outdata++ = (int)temp;
   }
}
