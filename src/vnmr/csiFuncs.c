/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

#include "vnmrsys.h"
#include "data.h"
#include "disp.h"
#include "init2d.h"
#include "group.h"
#include "tools.h"
#include "allocate.h"
#include "variables.h"
#include "pvars.h"
#include "iplan.h"
#include "wjunk.h"
#include "mfileObj.h"
#include "aipSpecStruct.h"
#include "aipCInterface.h"

#define FALSE           0
#define TRUE            1
#define CALIB            40000.0

// MAXSPEC 128
typedef struct {
  char key[MAXSTR];
  int yoff; // pixels
  char color[16];
  double scale; // additional scale to vs
} mspecInfo;
static mspecInfo specList[MAXSPEC];
static int mspecShow = 0;
static int mspecProcessing = 0;

extern double yppmm;
int getProcMspec() {return mspecProcessing;}
int getSpecYoff(int i) {
     return specList[i].yoff;
}
void clearMspec() {
     int i;
     mspecShow = 0;
     for(i=0;i<MAXSPEC;i++) strcpy(specList[i].key,"");
}

extern void set_anno_color(const char *colorName);
extern int colorindex(char *colorname, int *index);
extern int getColorByName(char *colorName);
extern planParams *getCurrPlanParams(int planType);
extern float *get_one_fid(int curfid, int *np, dpointers *c_block, int dcflag);
extern float *calc_user(int trace, int fpoint, int dcflag, int normok, int *newspec, int file_id);
extern void integ(register float  *fptr, register float  *tptr, register int npnt);
extern void getNearestPeak(float *spectrum, int fp, int np, float *max, double *freq);
double ppm2Hz(int direction, double val);
double Hz2ppm(int direction, double val);
int getNtraces();

void calc_li_map(char *path, double freq, double freq1, double freq2);
void calc_li_maps( char *path);
void calc_ll_map( char *path, int nearest, double freq);
void calc_ll_maps( char *path, int nearest);
void csi_getCSIDims(int *nx, int *ny, int *nz, int *ns);
void writeMMap(char *path, float *data, int numtraces, int sliceInd, int mapInd, int *dims, int rank, planParams *tag);
int  csi_getInd(int ix, int iy, int iz, int nx, int ny, int nz);
float *aip_GetFid(int trace);

double aip_getVsFactor() {
   double d;
   if(P_getreal(GLOBAL, "csiSpecVS", &d, 1)) d=0.0;
   if(d <= 0.0) d = 1.0;
   return d; 
}

// write fdf header for csi spectra or arrayed 1D spectra
// dataType: spectrum, baseline, fittings etc...
// dims: dimensions for the data, last dimension is direct dimension (fn/2)
// spatial_rank: csi spatial rank (0 for non-csi data)
// spectral_rank: spectral rank (1 for csi data, 2 for non-csi data) 
// total rank = spatial_rank + spectral_rank
void getSpecHeaderStr(char *appType, char *dataType, int *dims, int spatial_rank, int spectral_rank, char *hdr) {

   double d,d2;
   char str[MAXSTR],tmpstr[MAXSTR];
   float orientation[9];
   int rank = spatial_rank +  spectral_rank;

// magic string
   sprintf(hdr,"#!/usr/local/fdf/startup\n");

// generic data info
   sprintf(tmpstr,"float rank = %d;\n",rank);
   strcat(hdr,tmpstr);
   sprintf(tmpstr,"char *storage = \"float\";\n");
   strcat(hdr,tmpstr);
   sprintf(tmpstr,"float bits = 32;\n");
   strcat(hdr,tmpstr);
   sprintf(tmpstr,"char *type = \"phased\";\n");
   strcat(hdr,tmpstr);

#ifdef LINUX
   sprintf(tmpstr,"int    bigendian = 0;\n");
   strcat(hdr,tmpstr);
#endif

   if(rank == 1) {
      sprintf(tmpstr,"float  matrix[] = {%d};\n",dims[0]);
      strcat(hdr,tmpstr);
   } else if(rank == 2) {
      sprintf(tmpstr,"float  matrix[] = {%d,%d};\n",dims[0],dims[1]);
      strcat(hdr,tmpstr);
   } else if(rank == 3) {
      sprintf(tmpstr,"float  matrix[] = {%d,%d,%d};\n",dims[0],dims[1],dims[2]);
      strcat(hdr,tmpstr);
   } else if(rank == 4) {
      sprintf(tmpstr,"float  matrix[] = {%d,%d,%d,%d};\n",dims[0],dims[1],dims[2],dims[3]);
      strcat(hdr,tmpstr);
   }

   sprintf(tmpstr,"char *apptype = \"%s\";\n",appType);
   strcat(hdr,tmpstr);

   if(P_getstring(CURRENT, "seqfil", str, 1, MAXSTR)) strcpy(str,"");
   sprintf(tmpstr,"char *sequence = \"%s\";\n",str);
   strcat(hdr,tmpstr);

   if(P_getstring(CURRENT, "studyid_", str, 1, MAXSTR)) strcpy(str,"");
   sprintf(tmpstr,"char *studyid = \"%s\";\n",str);
   strcat(hdr,tmpstr);

   if(P_getstring(CURRENT, "file", str, 1, MAXSTR)) strcpy(str,"");
   sprintf(tmpstr,"char  *fidpath = \"%s\";\n",str);
   strcat(hdr,tmpstr);

// spatial info (this is for CSI only, will be used to layout spectra)
   if(spatial_rank > 0) {
      planParams *tag = NULL;

      if(spatial_rank == 2) { 
       tag = getCurrPlanParams(CSI2D);
       sprintf(tmpstr,"float  location[] = {%f,%f,%f};\n",
	-1.0*tag->pos1.value,tag->pos2.value,tag->pos3.value);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  roi[] = {%f,%f,%f};\n",
	tag->dim1.value,tag->dim2.value,tag->dim3.value);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"int  slices = %d;\n",(int)tag->ns.value);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  gap = %f;\n",tag->gap.value);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  thk = %f;\n",tag->thk.value);
       strcat(hdr,tmpstr);
      } else if(spatial_rank == 3) {
       tag = getCurrPlanParams(CSI3D);
       sprintf(tmpstr,"float  location[] = {%f,%f,%f};\n",
	-1.0*tag->pos1.value,tag->pos2.value,tag->pos3.value);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  roi[] = {%f,%f,%f};\n",
	tag->dim1.value,tag->dim2.value,tag->dim3.value);
       strcat(hdr,tmpstr);
      }
  
      sprintf(tmpstr,"float  psi = %f;\n",tag->psi.value);
      strcat(hdr,tmpstr);
      sprintf(tmpstr,"float  phi = %f;\n",tag->phi.value);
      strcat(hdr,tmpstr);
      sprintf(tmpstr,"float  theta = %f;\n",tag->theta.value);
      strcat(hdr,tmpstr);

      euler2tensorView(tag->theta.value,tag->psi.value,tag->phi.value,orientation);
      sprintf(tmpstr,"float  orientation[] = {%f,%f,%f,%f,%f,%f,%f,%f,%f};\n",
	orientation[0],orientation[1],orientation[2],
	orientation[3],orientation[4],orientation[5],
	orientation[6],orientation[7],orientation[8]);
      strcat(hdr,tmpstr);

      if(P_getstring(CURRENT, "position1", str, 1, MAXSTR)) strcpy(str,"");
      sprintf(tmpstr,"char *position1 = \"%s\";\n",str);
      strcat(hdr,tmpstr);

      if(P_getstring(CURRENT, "position2", str, 1, MAXSTR)) strcpy(str,"");
      sprintf(tmpstr,"char *position2 = \"%s\";\n",str);
      strcat(hdr,tmpstr);
   }

// spectral info
   if(spectral_rank > 0) { // for now, spectra of any rank will be treated as arrayed 1D.

      int i, ntraces = 0;
      for(i=1; i<rank; i++) ntraces +=dims[i];

      sprintf(tmpstr,"float spec_data_rank = 2;\n");
      strcat(hdr,tmpstr);
      sprintf(tmpstr,"float spec_display_rank = 1;\n");
      strcat(hdr,tmpstr);
      sprintf(tmpstr,"float spec_matrix[] = {%d,%d};\n",dims[0],ntraces);
      strcat(hdr,tmpstr);
     
      sprintf(tmpstr,"char *dataType = \"%s\";\n",dataType); 
      strcat(hdr,tmpstr);

      if(P_getstring(CURRENT, "tn", str, 1, MAXSTR)) strcpy(str,"");
      sprintf(tmpstr,"char  *nucleus[] = {\"%s\"};\n",str);
      strcat(hdr,tmpstr);

      if(P_getreal(CURRENT, "sfrq", &d, 1)) d=0.0;
      sprintf(tmpstr,"float sfreq[] = {%f};\n",d);
      strcat(hdr,tmpstr);

      if(P_getreal(CURRENT, "reffrq", &d, 1)) d=0.0;
      sprintf(tmpstr,"float reffrq[] = {%f};\n",d);
      strcat(hdr,tmpstr);

      if(P_getreal(CURRENT, "sw", &d, 1)) d=0.0;
      sprintf(tmpstr,"float sw[] = {%f};\n",d);
      strcat(hdr,tmpstr);

      if(P_getreal(CURRENT, "rfl", &d, 1)) d=0.0;
      if(P_getreal(CURRENT, "rfp", &d2, 1)) d2=0.0;
      sprintf(tmpstr,"float upfield[] = {%f};\n",d2-d);
      strcat(hdr,tmpstr);

      if(P_getreal(CURRENT, "wp", &d, 1)) d=0.0;
      sprintf(tmpstr,"float wp[] = {%f};\n",d);
      strcat(hdr,tmpstr);

      if(P_getreal(CURRENT, "sp", &d, 1)) d=0.0;
      sprintf(tmpstr,"float sp[] = {%f};\n",d);
      strcat(hdr,tmpstr);

      if(P_getreal(CURRENT, "rp", &d, 1)) d=0.0;
      sprintf(tmpstr,"float rp[] = {%f};\n",d);
      strcat(hdr,tmpstr);

      if(P_getreal(CURRENT, "lp", &d, 1)) d=0.0;
      sprintf(tmpstr,"float lp[] = {%f};\n",d);
      strcat(hdr,tmpstr);
   }

}

void getDataDim(char *appType, int *dims, int *ntraces, int *spatial_rank, int *spectral_rank) {

   if(strstr(appType,"csi") != NULL || strstr(appType,"CSI") != NULL) {
     int nx=1,ny=1,nz=1,ns=1;
     csi_getCSIDims(&nx,&ny,&nz,&ns); // ns > 1 only if nz==1
     nz = nz*ns;  // for this purpose, multi-slice is the same as 3D.
     if(nx > 1 && ny > 1 && nz > 1) {
       *spatial_rank = 3;
       *spectral_rank = 1;
       *ntraces = nx*ny*nz;
       dims[0] = fn/2;
       dims[1] = nx;
       dims[2] = ny;
       dims[3] = nz;
     } else if(nx > 1 && ny > 1) {
       *spatial_rank = 2;
       *spectral_rank = 1;
       *ntraces = nx*ny;
       dims[0] = fn/2;
       dims[1] = nx;
       dims[2] = ny;
     } else if(nx > 1) {
       *spatial_rank = 1;
       *spectral_rank = 1;
       *ntraces = dims[1] = nx;
       dims[0] = fn/2;
     }
   } else {
       *spatial_rank = 0;
       *spectral_rank = 2;
       *ntraces = dims[1] = nblocks * specperblock;
       dims[0] = fn/2;
   } 
}

int writeFDFSpecfile(char *path, char *dataType, float *data, int numtraces, int datasize, int msg) {
   int dims[4] = {0,0,0,0};
   int spatial_rank = 0;
   int spectral_rank = 0;
   int ntraces = 0;
   int np, filesize;
   MFILE_ID fileID;
   char hdr[2000];
   int i, hdrlen, align, pad_cnt;
   char appType[MAXSTR];
   if(P_getstring(CURRENT, "apptype", appType, 1, MAXSTR)) strcpy(appType,"");

   getDataDim(appType, dims, &ntraces, &spatial_rank, &spectral_rank);

   np = dims[0]; 
   if(ntraces != numtraces) {
     Werrprintf("Error: number of traces %d and parameter value %d don't match.",numtraces,ntraces);
     return 0;
   }
   if(np != datasize) {
     Werrprintf("Error: data size %d and parameter value %d don't match.",datasize,np);
     return 0;
   }

   getSpecHeaderStr(appType, dataType, dims, spatial_rank, spectral_rank, hdr);

   hdrlen = strlen(hdr);
   align = sizeof(float);
   hdrlen=strlen(hdr);
   hdrlen++; // include NULL terminator 
   pad_cnt = hdrlen % align;
   pad_cnt = (align - pad_cnt) % align;

   // Put in padding 
   for (i=0; i<pad_cnt; i++) {
        (void)strcat(hdr, "\n");
        hdrlen++;
   }
   *(hdr + hdrlen) = '\0';

   filesize = hdrlen*sizeof(char)+ntraces*np*sizeof(float);
   if(access(path, W_OK) == 0) unlink(path);
   fileID = mOpen(path, filesize, O_RDWR | O_CREAT);
   if(!fileID) {
      Werrprintf("Cannot open %s.", path);
      return 0; 
   }
   (void)mAdvise(fileID, MF_SEQUENTIAL);

   (void)memcpy(fileID->offsetAddr, hdr, hdrlen*sizeof(char));
    fileID->newByteLen += hdrlen*sizeof(char);
    fileID->offsetAddr += hdrlen*sizeof(char);
    (void)memcpy(fileID->offsetAddr, data, ntraces*np*sizeof(float));
    fileID->newByteLen += ntraces*np*sizeof(float);
    (void)mClose(fileID);

   if(msg>0) Winfoprintf("%s is saved (data size = %d x %d).", path,ntraces,np);
   return 1;
}

int writeFDFSpecfile_spectrum(char *path, int msg) {
   
   int ind = 0, updateflag = 0;
   int numtraces;
   int datasize;
   float *data, *ptr, *spectrum;
   int ret=0;

   init2d(GET_REV,GRAPHICS);
   numtraces = nblocks * specperblock;
   datasize = fn/2;

   if(numtraces < 1) {
        Werrprintf("No data.");
        return 0;
   }

   data = (float *) allocateWithId(datasize * numtraces * sizeof(float), "fdfdata");
   if(data == 0) {
	Werrprintf("cannot allocate fdfdata buffer");
        return 0;
   }
   
   ptr = data;
   while(ind < numtraces && (spectrum = calc_spec(ind,0,FALSE,TRUE,&updateflag)) != NULL) {

      memcpy(ptr, spectrum, datasize * sizeof(float) );
      ptr += datasize;
      ind++;
   }

   numtraces = ind;

   ret = writeFDFSpecfile(path, "spectrum", data, numtraces, datasize, msg); 
   releaseWithId("fdfdata");
   return ret;
}

int savefdfspec(int argc, char *argv[], int retc, char *retv[]) {

  int ret;
  int msg = (retc > 0) ? 0:1;
  if(argc > 1 && argv[1][0] == '/') {
    ret = writeFDFSpecfile_spectrum(argv[1], msg);
  } else {
    char datdirPath[MAXSTR]; 
    sprintf(datdirPath, "%s/datdir/spec.fdf",curexpdir);
    ret = writeFDFSpecfile_spectrum(datdirPath, msg);
  }
  if(retc>0) retv[0] = realString((double)ret);
  RETURN; 
}

int getShowMspec() {
   int i, count = 0;
   if(!mspecShow) return 0;

   for(i=0;i<MAXSPEC;i++) {
      if(strlen(specList[i].key) == 0) continue;
      count++;
   }
   return count; 
}

int setSpecColor(int i) { 
      int color;
      if(strlen(specList[i].color)==0) return SPEC_COLOR;
      else if(colorindex(specList[i].color, &color)) return color;
      else {
        set_anno_color(specList[i].color);
        return -1;
      }
}

// mspec('on/off/clear')
// mspec('processing','on/off')
// mspec(1,key1,yoff1,color1)
// mspec(2,key2,yoff2,color2)
int mspec(int argc, char *argv[], int retc, char *retv[]) {
  int i, ind, iColor, yoff=0;
  double voff, scale=1.0;
  char str[16];
  char color[16];
  if(argc < 2) {
    if(retc > 0) {
        if(retc > 1) { // return # of spec
          int count = 0;
          for(i=0;i<MAXSPEC;i++) {
	    if(strlen(specList[i].key) == 0) continue;
      	    count++;
          }
	  retv[1] = (char *)realString((double)count); 
        }
 	// return mspec mode
	retv[0] = (char *)realString((double)mspecShow);
	RETURN;
    } else {
	Winfoprintf("Usage: mspec(1,...) or mspec('clear')");
    	ABORT;
    }
  }
   
  if(strcasecmp(argv[1],"on") == 0) {
     mspecShow=1;
  } else if(strcasecmp(argv[1],"off") == 0) {
     mspecShow=0;
  } else if(strcasecmp(argv[1],"clear") == 0) {
     mspecShow=0;
     for(i=0;i<MAXSPEC;i++) strcpy(specList[i].key,"");
  } else if(strcasecmp(argv[1],"processing") == 0) {
     if(argc>2 && strcasecmp(argv[2],"off") == 0) mspecProcessing = 0;
     else mspecProcessing = 1;
  } else if(strcasecmp(argv[1],"all") == 0) { // all in the array up to MAXSPEC=128
     int ntraces = getNtraces();
     mspecShow=1;
     for(i=0;i<MAXSPEC;i++) strcpy(specList[i].key,"");
     if(!P_getreal(CURRENT,"vo", &voff, 1) && voff >= 0.0) {
       if(yppmm>0) voff *= yppmm;
       yoff = (int)voff;
     } else {
       yoff = dnpnt2/(argc);
     }
     for(i=0;i<MAXSPEC && i<ntraces;i++) {
/* 123456789 987654321 123456789
        ind = floor(i/9);
        if(ind%2 == 0) ind = i % 9 + 1;
	else ind = 9 - i % 9;
*/
// 12345678 98765432 12345678 98765432
        ind = floor(i/8);
        if(ind%2 == 0) ind = i % 8 + 1;
	else ind = 9 - i % 8;

        sprintf(color,"Spectrum%d",ind); 

	sprintf(specList[i].key,"SPEC:%d",i+1); 
        specList[i].yoff=i*yoff;
        strcpy(specList[i].color,color);
        specList[i].scale=scale;
     }
 
     if(ntraces>MAXSPEC) 
       Winfoprintf("%d of %d spectra are displayed.",MAXSPEC,ntraces);

     argv++;
     argc--;
     argv++;
     argc--;
     iColor = 0;
     while(argc && iColor < i) {
	if(!isdigit(argv[0][0]) || strstr(argv[0],"0x")==argv[0] || 
	strstr(argv[0],",") != NULL) {
          strcpy(specList[iColor].color,argv[0]);
	  iColor++;
	}
        argv++;
        argc--;
     }
 
     if(iColor > 0 && iColor < i) {
        int j;
        ind = i;
        for(i=iColor; i<ind; i+=iColor) {
	   for(j=0; j<iColor; j++) {
	      strcpy(specList[i+j].color,specList[j].color); 
	   }
	}
     }
  } else if(isdigit(argv[1][0]) && strstr(argv[1],"-") != NULL) {
  // mspec('1-') - all
  // mspec('6-') - 6 to last trace
  // mspec('2-10')
  // mspec('2-10:2') - 2 to 10 with step size 2
  // mspec('2-:2') - 2 to last trace, with step size 2
     int ntraces = getNtraces();
     int first=1, last=ntraces, step=1, count=0;
     char *strptr, *tokptr;
     strptr = argv[1];
     if((tokptr = (char*) strtok(strptr, ":")) != (char *) 0) {
        strcpy(str,tokptr);
	strptr = (char *) 0;
	if((tokptr = (char*) strtok(strptr, ":")) != (char *) 0) step = atoi(tokptr); 
     } else strcpy(str,argv[1]);
     strptr = str;
     if((tokptr = (char*) strtok(strptr, "-")) != (char *) 0) {
	first = atoi(tokptr);
	strptr = (char *) 0;
	if((tokptr = (char*) strtok(strptr, "-")) != (char *) 0) last = atoi(tokptr); 
     }
     mspecShow=1;
     for(i=0;i<MAXSPEC;i++) strcpy(specList[i].key,"");
     if(!P_getreal(CURRENT,"vo", &voff, 1) && voff >= 0.0) {
       if(yppmm>0) voff *= yppmm;
       yoff = (int)voff;
     } else {
       yoff = dnpnt2/(argc);
     }
     if(first<1) first=1;
     if(last<first) last=first;
     if(last>ntraces) last=ntraces;
     count=0;
     for(i=first;i<MAXSPEC && i<=last;i+=step) {
/* 123456789 123456789
        ind = count % 9 + 1;
*/
        ind = floor(count/8);
        if(ind%2 == 0) ind = count % 8 + 1;
	else ind = 9 - count % 8;

        sprintf(color,"Spectrum%d",ind); 

	sprintf(specList[count].key,"SPEC:%d",i); 
        specList[count].yoff=count*yoff;
        strcpy(specList[count].color,color);
        specList[count].scale=scale;
	count++;
     }

     if(ntraces>MAXSPEC || last>MAXSPEC) 
       Winfoprintf("%d of %d spectra are displayed.",MAXSPEC,ntraces);

     argv++;
     argc--;
     argv++;
     argc--;
     iColor = 0;
     while(argc && iColor < count) {
	if(!isdigit(argv[0][0]) || strstr(argv[0],"0x")==argv[0] || 
	strstr(argv[0],",") != NULL) {
          strcpy(specList[iColor].color,argv[0]);
	  iColor++;
	}
        argv++;
        argc--;
     }
 
     if(iColor > 0 && iColor < count) {
        int j;
        ind = count;
        for(i=iColor; i<ind; i+=iColor) {
	   for(j=0; j<iColor; j++) {
	      strcpy(specList[i+j].color, specList[j].color); 
	   }
	}
     }
  } else if(argc > 2 && isdigit(argv[1][0]) && !isdigit(argv[2][0]) && strstr(argv[2],":") != NULL) { 
     // e.g., spec(1,'SPEC:3',40,'red')
     mspecShow=1;
     i = atoi(argv[1]);
     if(i<1 || i>MAXSPEC) {
        Werrprintf("Invalid spec # %d (range: 1-128).",i);
        ABORT;
     } else i--;
     strcpy(specList[i].key,argv[2]);
     if(argc>3) yoff=atoi(argv[3]);
     else if(!P_getreal(CURRENT,"vo", &voff, 1) && voff >= 0.0) {
       if(yppmm>0) voff *= yppmm;
       yoff = (int)voff*i;
     }
     if(argc>4) {
       strcpy(color, argv[4]);
     } else {
/*
        ind = floor(i/9);
        if(ind%2 == 0) ind = i % 9 + 1;
	else ind = 9 - i % 9;
*/
        ind = floor(i/8);
        if(ind%2 == 0) ind = i % 8 + 1;
	else ind = 9 - i % 8;

        sprintf(color,"Spectrum%d",ind); 
     }
     if(argc>5) scale=atof(argv[5]);
     specList[i].yoff=yoff;
     strcpy(specList[i].color, color);
     specList[i].scale=scale;
 
  } else if(argc > 2 && isdigit(argv[1][0])) { // mspec(1,3,4,...,color,...) 
     int trace;
     int ntraces = getNtraces();
     iColor = 0;
     if(!P_getreal(CURRENT,"vo", &voff, 1) && voff >= 0.0) {
       if(yppmm>0) voff *= yppmm;
       yoff = (int)voff;
     } else {
       yoff = dnpnt2/(argc);
     }
     for(i=0;i<MAXSPEC;i++) strcpy(specList[i].key,"");
     mspecShow=1;
     i=0;
     argv++;
     argc--;
     while(argc) {
        if(i>=MAXSPEC) {
               Werrprintf("Invalid spec # %d (range: 1-128).",i);
               ABORT;
        }
        specList[i].yoff=i*yoff;
        specList[i].scale=scale;
/*
        ind = floor(i/9);
        if(ind%2 == 0) ind = i % 9 + 1;
	else ind = 9 - i % 9;
*/
        ind = floor(i/8);
        if(ind%2 == 0) ind = i % 8 + 1;
	else ind = 9 - i % 8;

	sprintf(color,"Spectrum%d",ind); 
        strcpy(specList[i].color, color);
        if(isdigit(argv[0][0]) && strstr(argv[0],"0x")==NULL &&
		strstr(argv[0],",") == NULL) {
          trace = atoi(argv[0]);
          if(trace < 1 || trace > ntraces) {
            Winfoprintf("Index %d out of bounds (ignored)",trace);
            argv++;
            argc--;
            continue;
	  }
	  sprintf(specList[i].key,"SPEC:%d",trace); 
          i++;
        } else if(strstr(argv[0],":") != NULL) {
          strcpy(specList[i].key,argv[0]);
          i++;
        } else if(!isdigit(argv[0][0]) || strstr(argv[0],"0x")==argv[0] || 
		strstr(argv[0],",") != NULL) {
          strcpy(specList[iColor].color, argv[0]);
	  iColor++;
        }
        argv++;
        argc--;
     }
 
     if(iColor > 0 && iColor < i) {
        int j;
        ind = i;
        for(i=iColor; i<ind; i+=iColor) {
	   for(j=0; j<iColor; j++) {
	      strcpy(specList[i+j].color, specList[j].color); 
           }
	      
	}
     }
  } else if(argc>1) { // mspec('SPEC:1','BASE:1',...)
     if(!P_getreal(CURRENT,"vo", &voff, 1) && voff >= 0.0) {
       if(yppmm>0) voff *= yppmm;
       yoff = (int)voff;
     } else {
       yoff = dnpnt2/(argc);
     }
     for(i=0;i<MAXSPEC;i++) strcpy(specList[i].key,"");
     mspecShow=1;
     i=0;
     argv++;
     argc--;
     while(argc) {
        if(i>=MAXSPEC) {
               Werrprintf("Invalid spec # %d (range: 1-128).",i);
               ABORT;
        }
        if(isdigit(argv[0][0])) {
	  sprintf(specList[i].key,"SPEC:%d",atoi(argv[0])); 
        } else {
          strcpy(specList[i].key,argv[0]);
        }
        specList[i].yoff=i*yoff;
        specList[i].scale=scale;
/*
        ind = floor(i/9);
        if(ind%2 == 0) ind = i % 9 + 1;
	else ind = 9 - i % 9;
*/
        ind = floor(i/8);
        if(ind%2 == 0) ind = i % 8 + 1;
	else ind = 9 - i % 8;

	sprintf(str,"Spectrum%d",ind); 
        strcpy(specList[i].color, color);
        argv++;
        argc--;
        i++;
     }
  }

  RETURN;
}

// TODO
// recalculate if update=1
float *getFitting(int trace, int npts, int update) {
   return NULL; 
}

extern float *getBcdata(int trace, int npts, int update);

// curSpec start from 0
float *calc_mspec(int curSpec, int trace, int fpoint, int dcflag, int normok, int *newspec)
{
    char *key;
    if(curSpec<0 || curSpec>=MAXSPEC) {
	Werrprintf("Index %d out of range. Return NULL.",curSpec);
	return NULL;
    }

    key = specList[curSpec].key;
    if(strlen(key) < 1) {
      return NULL;

    } else if(strcmp(key,"SPEC") == 0) { 
      return calc_user(trace, fpoint, dcflag, normok, newspec, D_PHASFILE);

    } else if(strcmp(key,"BASE") == 0) { 
      return getBcdata(trace, fn/2, mspecProcessing);

    } else if(strcmp(key,"FIT") == 0) {
      return getFitting(trace, fn/2, mspecProcessing);

    } else { 
      float *data=aipGetTrace(key, trace, specList[curSpec].scale, fn/2);
      if(data!=NULL) return data+fpoint;
      else return NULL;
    }

}

// when return NULL, phasefile buffer will be used.
float *mspec_getSpec(int trace, int npts, int firstpnt) {
    int curSpec = 0;
    if(trace >=0 && trace < MAXSPEC) curSpec = trace;
    else return NULL;
    if(mspecProcessing || strlen(specList[curSpec].key)==0 || 
		strcmp(specList[curSpec].key,"SPEC") == 0) { 
      return NULL; 
    } else { 
      float *data=aipGetTrace(specList[curSpec].key, trace, specList[curSpec].scale,npts);
      if(data!=NULL) return data+firstpnt;
      else return NULL;
    }
}

// The following functions will be used by llMMap and liMMap commands.
// these command will called init2d if needed. So global parameters like
// nblocks, fn, sw, rflrfp etc... should already be set correctly. 

// llfrq and lifrq are in absolute frequency.
// whereas cr or dpf is in referenced frequency (i.e. checmical shift).
// cs = freq - rflrfp
// rflrfp = rfl - rfp
double freq_to_cs(double freq) {
   return freq - rflrfp;
}

// convert cr (cursor in chemical shift, in Hz) to frequency
double cs_to_freq(double cs) {
   return cs + rflrfp;
}

double freq_to_csPPM(double freq) {
   return Hz2ppm(HORIZ, freq - rflrfp);
}

int freq_to_dp(double freq) {
   return((int) ((sw - freq) * (double)fn*0.5 / sw));
}

double dp_to_freq(int dp) {
   return((double) (sw * ( 1.0 - ((double) dp / (double) fn*0.5))));
}

// given a integral region defined by freq1 and freq2 (from lifrq), 
// find corresponding peak in llfrq.
// in the case where no peak in llfrq falls in the region, freq is 
// set to the center of the region.
// this is used to get the peak freq, which is used to identify or
// label a peak.
int getFreq4Region(double *freq, double freq1, double freq2) {
  double d;
  int i = 0, found = 0;
  int n = P_getsize(CURRENT,"llfrq",NULL);
  
  if(n > 1) { // llfrq contains freqs n peaks.
     for(i=0; i<n; i++) {
	if(!P_getreal(CURRENT,"llfrq", &d, i+1)) { 
          if(d <= freq1 && d >= freq2) {
	     found = i+1;
	     *freq = d;
	     break;
          }
        }
     }
  }
  if(!found) {
     *freq = 0.5*(freq1+freq2);
  }
  return found;
}

// given a peak freq (from llfrq), find integral region fron lifrq.
// if peak does not fall into any lifrq region, use freq+/200Hz. 
int getRegion4Freq(double freq, double *freq1, double *freq2) {
  double d1, d2;
  int i = 0, found = 0;
  int n = P_getsize(CURRENT,"lifrq",NULL);
  if(n > 1) { // lifrq contains n*2+1 freqs for n "partial" intergals.
     n = (n-1)/2;
     for(i=0; i<n; i++) {
	P_getreal(CURRENT,"lifrq", &d1, i*2+1);	
	P_getreal(CURRENT,"lifrq", &d2, i*2+2);	
        if(freq <=d1 && freq >= d2) {
	   found = i+1;
	   *freq1 = d1;
	   *freq2 = d2;
	   break;
        }
     }
  } 
  if(!found) { // return the entire spec region
     found=2;
     *freq1 = sw;
     *freq2 = 0;
  }
  return found;
}

// get full path for mmap file (without .fdf extension).
void getDefaultMapPath(char *path) {
   char *ptr;
   int n;
   if(P_getstring(CURRENT, "file", path, 1, MAXSTR)) {
     sprintf(path, "%s/datdir/maps",curexpdir);
   } else {
     ptr = strstr(path,".csi/");
     if(ptr != NULL) {
	n = strlen(path) - strlen(ptr) + 4;
        path[n]='\0';
        strcat(path,"/maps");
     } else {
        sprintf(path, "%s/datdir/maps",curexpdir);
     }
   }
}

// create directory if needed.
void checkDir(char *path) {
   int n = strlen(path);
   // remove ending '/' 
   if(path[n-1] == '/') {
     n--;
     path[n] = '\0';
   }
   while(n>1) {
     if(path[n-1] == '/') {n--; break;}
     n--;
   }
   if(n>1) {
     char tmp[1024];
     strcpy(tmp,"");
     strncat(tmp,path,n); // tmp is directory path
     if(access(tmp, W_OK) != 0) {
        if(mkdir(tmp,0777 )) fprintf(stderr, "Error make %s directory.\n",tmp);
     }
   }
}

// remove .fdf, create dir if needed
void checkMapPath(char *path) {
   char *ptr;
   ptr = strstr(path,".fdf");
   if(ptr != NULL) {
     int n = strlen(path) - strlen(ptr);
     path[n]='\0';
   }
   checkDir(path);
}

// liMMap(fullpath, freq1, freq2)
// liMMap(freq1, freq2, fullpath)
// liMMap(freq1, freq2)
// liMMap(fullpath, cs)
// liMMap(cs, fullpath)
// liMMap(fullpath)
// liMMap(cs)
// fullpath is full path to map file without .fdf file extension. 
// peak freq (or label) will be attached to file name.
// freq1, freq2 are absolute frequencies (in lifrq)
// cs is chemical shift of a peak, such as cr (cursor) 
int liMMap(int argc, char *argv[], int retc, char *retv[]) {
   char path[MAXSTR];
   double freq=0, freq1=0,freq2=0;
   int sel = 0;
   
   strcpy(path,"");
   if(argc>3 && argv[1][0] == '/') {
	sel = 1;
	strcpy(path,argv[1]);
	freq1=atof(argv[2]);
	freq2=atof(argv[3]);
        getFreq4Region(&freq, freq1,freq2); 
   } else if(argc>3) {
	sel = 1;
	freq1=atof(argv[1]);
	freq2=atof(argv[2]);
	strcpy(path,argv[3]);
        getFreq4Region(&freq, freq1,freq2); 
   } else if(argc>2 && argv[1][0] == '/') {
	sel = 1;
	strcpy(path,argv[1]);
	freq=cs_to_freq(atof(argv[2]));
        getRegion4Freq(freq, &freq1, &freq2);
   } else if(argc>2 && argv[2][0] == '/') {
	sel = 1;
	freq=cs_to_freq(atof(argv[1]));
        getRegion4Freq(freq, &freq1, &freq2);
	strcpy(path,argv[2]);
   } else if(argc>2) {
	sel = 1;
	freq1=atof(argv[1]);
	freq2=atof(argv[2]);
        getFreq4Region(&freq, freq1,freq2); 
   } else if(argc>1 && argv[1][0] == '/') {
	strcpy(path,argv[1]);
   } else if(argc>1 && strcmp(argv[1],"all")==0) {
        sel = 0;
   } else if(argc>1) {
	sel = 1;
	freq=cs_to_freq(atof(argv[1]));
        getRegion4Freq(freq, &freq1, &freq2);
   } else {
	sel = 1;
	freq=cs_to_freq(cr);
        freq1=sw;
        freq2=0;
   }

   if(strlen(path) == 0) {
     getDefaultMapPath(path);
     if(!sel || (freq1==sw && freq2==0)) { 
       strcat(path,"/csi_image");
     } else {
       char tmpstr[MAXSTR];
       sprintf(tmpstr, "/mmap_li_%.2f", freq_to_csPPM(freq));
       strcat(path,tmpstr);
     } 
   } else {
     char dir[MAXSTR];
     if(path[0] != '/') {
       getDefaultMapPath(dir);
	strcat(dir,"/");
	strcat(dir,path);
     } else {
       sprintf(dir,"%s",path);
     }
     if(path[strlen(path)-1]=='_') {
       if(freq1==sw && freq2==0)
	 sprintf(path, "%simage",dir);
       else 
         sprintf(path, "%s%.2f",dir, freq_to_csPPM(freq));
     } else {
       sprintf(path, "%s",dir);
     } 
   }

   if(sel) calc_li_map(path, freq, freq1, freq2);
   else calc_li_maps(path);

   RETURN;
}

// llMMap - calculates mmaps for all peaks in llfrq, and save to default path.
// llMMap(cr) - calculates mmap for pecified peak (e.g. by cr), 
//	and save to default path
// llMMap(path) - calculates mmaps for all peaks in llfrq, and save to path.
// llMMap(path, cr) - calculates map for specified peak (e.g by cr).
// llMMap(cr, path) - the same as llMMap(path, cr).
// path is full path to the directory where maps will be saved
// cr is in chemical shift, needs to be converted to absolute freq 
//	(as in llfrq or lifrq)
// maps will be named according to freq or label
// by default "nearest line" will be used, unless is specified by argv[3].
int llMMap(int argc, char *argv[], int retc, char *retv[]) {
   char path[MAXSTR];
   double cs=0;
   int sel = 0;
   
   int nearest = 1;
   if(argc > 3) nearest = atoi(argv[3]);

   strcpy(path,"");
   if(argc>2 && argv[1][0] == '/') {
	sel = 1;
	strcpy(path,argv[1]);
	cs=atof(argv[2]);
   } else if(argc>2) {
	sel = 1;
	cs=atof(argv[1]);
	strcpy(path,argv[2]);
   } else if(argc>1 && argv[1][0] == '/') {
	strcpy(path,argv[1]);
   } else if(argc>1 && strcmp(argv[1],"all")==0) {
        sel = 0;
   } else if(argc>1) {
	sel = 1;
	cs=atof(argv[1]);
   } else {
	sel = 1;
	cs=cr;
   }

   double freq = cs_to_freq(cs);
   if(strlen(path) == 0) { 
     getDefaultMapPath(path);
     if(!sel) {
       strcat(path,"/mmap_ll");
     } else {
	char tmpstr[MAXSTR];
       sprintf(tmpstr, "/mmap_ll_%.2f", freq_to_csPPM(freq));
       strcat(path,tmpstr);
     }
   } else {
     char dir[MAXSTR];
     if(path[0] != '/') {
       getDefaultMapPath(dir);
	strcat(dir,"/");
	strcat(dir,path);
     } else {
       sprintf(dir,"%s",path);
     }
     sprintf(path, "%s_%.2f", dir,freq_to_csPPM(freq));
   }

   if(sel) calc_ll_map(path, nearest, freq); 
   else calc_ll_maps(path, nearest); 

   RETURN;
}

// given a spec of np points, and a start point (e.g. cr) near a peak,
// find peak amp and freq. 
void getNearest(float *spec, int np, int start, float *amp, double *freq) {
  int le;
  le = np / 40;
  if (le > 400)
    le = 400;
  else if (le < 10)
    le = 10;
  start -= le / 2 - 1;
  if (start < 0)
    start = 0;
  if (start + le >= np)
    start = np - le;
  getNearestPeak(spec,start,le,amp,freq);
}

// TODO:
// currently this does not work for ROI: 
// dim1,2,3 and pos1,2,3 need to be calculated for ROI.  
// this also does not work for planes of 3D data because orientation 
// should be based on base image, not theta, psi, phi parameters.
void getMapHeaderStr(char *appType, int *dims, int rank, int sliceInd, planParams *tag, char *hdr) {
   double d, d2;
   char str[MAXSTR], str2[MAXSTR],tmpstr[MAXSTR];
   float orientation[9];

// magic string
   sprintf(hdr,"#!/usr/local/fdf/startup\n");

// generic data info
   sprintf(tmpstr,"float rank = %d;\n",rank);
   strcat(hdr,tmpstr);
   sprintf(tmpstr,"char *storage = \"float\";\n");
   strcat(hdr,tmpstr);
   sprintf(tmpstr,"float bits = 32;\n");
   strcat(hdr,tmpstr);
   sprintf(tmpstr,"char *type = \"absval\";\n");
   strcat(hdr,tmpstr);
   sprintf(tmpstr,"float image = 1;\n");
   strcat(hdr,tmpstr);

#ifdef LINUX
   sprintf(tmpstr,"int    bigendian = 0;\n");
   strcat(hdr,tmpstr);
#endif

   sprintf(tmpstr,"char *apptype = \"%s\";\n",appType);
   strcat(hdr,tmpstr);

   if(P_getstring(CURRENT, "seqfil", str, 1, MAXSTR)) strcpy(str,"");
   sprintf(tmpstr,"char *sequence = \"%s\";\n",str);
   strcat(hdr,tmpstr);

   if(P_getstring(CURRENT, "studyid_", str, 1, MAXSTR)) strcpy(str,"");
   sprintf(tmpstr,"char *studyid = \"%s\";\n",str);
   strcat(hdr,tmpstr);

   if(P_getstring(CURRENT, "file", str, 1, MAXSTR)) strcpy(str,"");
   sprintf(tmpstr,"char  *fidpath = \"%s\";\n",str);
   strcat(hdr,tmpstr);

      if(rank == 2) { 
       // determine pss for sliceInd
       int ns = 1;
       float pss;
       vInfo paraminfo;
       if(!P_getVarInfo(CURRENT, tag->pss.name, &paraminfo) ) ns = paraminfo.size;
       //sliceInd += 1; // sliceInd now starts from 1
       if(sliceInd > ns) {
	 Winfoprintf("Warning: slice index %d out of bound (1 to %d). Set to 1",sliceInd,ns); 
	 sliceInd = 1;
       }
       if(!P_getreal(CURRENT,tag->pss.name,&d,sliceInd)) pss=d;
       else pss = 0;

       sprintf(tmpstr,"char  *spatial_rank = \"2dfov\";\n");
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float matrix[] = {%d,%d};\n",dims[0],dims[1]);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"char *abscissa[] = {\"cm\", \"cm\"};\n");
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"char *ordinate[] = {\"intensity\"};\n");
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  span[] = {%f,%f};\n", tag->dim1.value,tag->dim2.value);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  origin[] = {%f,%f};\n",-1.0*tag->pos1.value,tag->pos2.value);
       strcat(hdr,tmpstr);
       if(P_getstring(CURRENT, "tn", str, 1, MAXSTR)) strcpy(str,"H1");
       if(P_getstring(CURRENT, "dn", str2, 1, MAXSTR)) strcpy(str2,"H1");
       sprintf(tmpstr,"char  *nucleus[] = {\"%s\",\"%s\"};\n",str,str2);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  location[] = {%f,%f,%f};\n",
	  -1.0*tag->pos1.value,tag->pos2.value,pss);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  roi[] = {%f,%f,%f};\n",
	  tag->dim1.value,tag->dim2.value,tag->thk.value);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"int  slices = %d;\n",ns);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"int  slice_no = %d;\n",sliceInd);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  gap = %f;\n",tag->gap.value);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  thk = %f;\n",tag->thk.value);
       strcat(hdr,tmpstr);
       
       if(P_getreal(CURRENT, "sfrq", &d, 1)) d=0.0;
       if(P_getreal(CURRENT, "dfrq", &d2, 1)) d2=0.0;
       sprintf(tmpstr,"float  nucfreq[] = {%f,%f};\n",d,d2);
       strcat(hdr,tmpstr);
      } else if(rank == 3) { 
       sprintf(tmpstr,"char  *spatial_rank = \"3dfov\";\n");
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float matrix[] = {%d,%d,%d};\n",dims[0],dims[1],dims[2]);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"char *abscissa[] = {\"cm\", \"cm\", \"cm\"};\n");
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"char *ordinate[] = {\"intensity\"};\n");
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  span[] = {%f,%f,%f};\n", tag->dim1.value,tag->dim2.value,tag->dim3.value);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  origin[] = {%f,%f,%f};\n",-1.0*tag->pos1.value,tag->pos2.value,tag->pos3.value);
       strcat(hdr,tmpstr);
       if(P_getstring(CURRENT, "tn", str, 1, MAXSTR)) strcpy(str,"H1");
       if(P_getstring(CURRENT, "dn", str2, 1, MAXSTR)) strcpy(str2,"H1");
       sprintf(tmpstr,"char  *nucleus[] = {\"%s\",\"%s\"};\n",str,str);
       strcat(hdr,tmpstr);
       if(P_getreal(CURRENT, "sfrq", &d, 1)) d=0.0;
       if(P_getreal(CURRENT, "dfrq", &d2, 1)) d2=0.0;
       sprintf(tmpstr,"float  nucfreq[] = {%f,%f};\n",d,d2);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  location[] = {%f,%f,%f};\n",
	  -1.0*tag->pos1.value,tag->pos2.value,tag->pos3.value);
       strcat(hdr,tmpstr);
       sprintf(tmpstr,"float  roi[] = {%f,%f,%f};\n",
	  tag->dim1.value,tag->dim2.value,tag->dim3.value);
       strcat(hdr,tmpstr);
      }
  
      sprintf(tmpstr,"float  psi = %f;\n",tag->psi.value);
      strcat(hdr,tmpstr);
      sprintf(tmpstr,"float  phi = %f;\n",tag->phi.value);
      strcat(hdr,tmpstr);
      sprintf(tmpstr,"float  theta = %f;\n",tag->theta.value);
      strcat(hdr,tmpstr);

      if(rank>2)
      euler2tensorView_3D(tag->theta.value,tag->psi.value,tag->phi.value,orientation);
      else
      euler2tensorView(tag->theta.value,tag->psi.value,tag->phi.value,orientation);
      sprintf(tmpstr,"float  orientation[] = {%f,%f,%f,%f,%f,%f,%f,%f,%f};\n",
	orientation[0],orientation[1],orientation[2],
	orientation[3],orientation[4],orientation[5],
	orientation[6],orientation[7],orientation[8]);
      strcat(hdr,tmpstr);

      if(P_getstring(CURRENT, "position1", str, 1, MAXSTR)) strcpy(str,"");
      sprintf(tmpstr,"char *position1 = \"%s\";\n",str);
      strcat(hdr,tmpstr);

      if(P_getstring(CURRENT, "position2", str, 1, MAXSTR)) strcpy(str,"");
      sprintf(tmpstr,"char *position2 = \"%s\";\n",str);
      strcat(hdr,tmpstr);
}

// for 2D csi, numtraces is traces for one slice 
void writeCSIMap(char *path, float *data, int numtraces, int sliceInd) {

   int dims[3] = {0,0,0};
   int rank, ntraces;
   int nx=1,ny=1,nz=1,ns=1; // ns is irrelevant here 
   csi_getCSIDims(&nx,&ny,&nz,&ns);
   if(nx > 1 && ny > 1 && nz > 1) {
       rank=3;
       dims[0] = nx;
       dims[1] = ny;
       dims[2] = nz;
   } else {
       rank=2;
       dims[0] = nx;
       dims[1] = ny;
       dims[2] = 1;
   }
   ntraces = nx*ny*nz;
   if(ntraces != numtraces) {
        Werrprintf("ntraces %d mismatch number of traces %d",ntraces,numtraces);
        return;
   }

   planParams *tag = NULL;
   if(rank == 2) tag = getCurrPlanParams(CSI2D);
   else tag = getCurrPlanParams(CSI3D);

   writeMMap(path, data, numtraces, sliceInd, sliceInd, dims, rank, tag);

}

void writeMMap(char *path, float *data, int numtraces, int sliceInd, int mapInd, int *dims, int rank, planParams *tag) {
   int filesize;
   MFILE_ID fileID;
   char hdr[2000];
   int i, hdrlen, align, pad_cnt;
   char appType[MAXSTR];
   if(P_getstring(CURRENT, "apptype", appType, 1, MAXSTR)) strcpy(appType,"");

   getMapHeaderStr(appType, dims, rank, sliceInd, tag, hdr);

   hdrlen = strlen(hdr);
   align = sizeof(float);
   hdrlen=strlen(hdr);
   hdrlen++; // include NULL terminator 
   pad_cnt = hdrlen % align;
   pad_cnt = (align - pad_cnt) % align;

   // Put in padding 
   for (i=0; i<pad_cnt; i++) {
        (void)strcat(hdr, "\n");
        hdrlen++;
   }
   *(hdr + hdrlen) = '\0';

   filesize = hdrlen*sizeof(char)+numtraces*sizeof(float);
   if(access(path, W_OK) == 0) unlink(path);
   fileID = mOpen(path, filesize, O_RDWR | O_CREAT);
   if(!fileID) {
      Werrprintf("Cannot open %s.", path);
      return; 
   }
   (void)mAdvise(fileID, MF_SEQUENTIAL);

   (void)memcpy(fileID->offsetAddr, hdr, hdrlen*sizeof(char));
    fileID->newByteLen += hdrlen*sizeof(char);
    fileID->offsetAddr += hdrlen*sizeof(char);
    (void)memcpy(fileID->offsetAddr, data, numtraces*sizeof(float));
    fileID->newByteLen += numtraces*sizeof(float);
    (void)mClose(fileID);

   Winfoprintf("%s is saved (data size = %d x %d x %d).", path,dims[0],dims[1],dims[2]);

   // now set csiMapSel parameter
   char name[MAXSTR];
   getDefaultMapPath(name);
   P_setstring(GLOBAL,"csiMapSel", path+strlen(name)+1,mapInd);
}

// convert freq to pnt, then get the pnt for all traces.
// if nearest > 0, getpeak(spectrum,start,le,&amp,&freq) will be called to get the neast line.
void calc_ll_map( char *path, int nearest, double freq) {
   int ind = 0, updateflag = 0;
   int numtraces=nblocks * specperblock;
   int datasize = fn/2;
   float *data, *spectrum, pamp;
   double pfreq;
   int pnt;
   int nx,ny,nz,i,j,k, count;
   int ns,sliceInd,ntraces,firstSlice,lastSlice,firstPlane,lastPlane;
   double d;
   char path2[MAXSTR];
   double vsFactor = aip_getVsFactor();

   if(numtraces < 1) {
      init2d(GET_REV,GRAPHICS);
      numtraces=nblocks * specperblock;
      if(numtraces < 1) {
        Werrprintf("No data.");
        return;
      }
   }

   csi_getCSIDims(&nx, &ny, &nz, &ns); 
   if(numtraces != (nx*ny*nz*ns)) {
      Werrprintf("Abort: data size %d does not match parameter value %d",numtraces,(nx*ny*nz)); 
      return;
   }

   if(!P_getreal(CURRENT, "csiSlice", &d, 1)) sliceInd=(int)d;
   else sliceInd = 0; // all slices

   // check slice
   if(ns == 1) sliceInd=1; // single slice data
   if(sliceInd > ns) sliceInd=1; // slice out of bound

   if(nz > 1) { // write 3D map (cannot extract 2D map of 3D data
      firstSlice = 1; lastSlice = 1;
      firstPlane = 1; lastPlane=nz;
   } else if(sliceInd <= 0) { // all slices
      firstSlice = 1; lastSlice = ns;
      firstPlane = 1; lastPlane=1;
   } else {
      firstSlice = sliceInd; lastSlice = sliceInd;
      firstPlane = 1; lastPlane=1;
   }

   // Note, for 2D data, ntraces=numtraces/ns;
   // numtraces is total number of traces. 
   // if ns=1 (this is the case for single slice, or 3D data), ntraces=numtraces
   ntraces = nx*ny*nz;

   data = (float *) allocateWithId(ntraces * sizeof(float), "mmap");
   if(data == 0) {
	Werrprintf("cannot allocate mmap buffer");
        return;
   }
 
 clearVar("csiMapSel");
 checkMapPath(path);
 for(sliceInd=firstSlice-1; sliceInd<lastSlice; sliceInd++) {
   // append freq and file extension to path
   if(ns>1) sprintf(path2,"%s_%d.fdf",path,sliceInd+1);
   else sprintf(path2,"%s.fdf",path);

   count=0;
   pnt = freq_to_dp(freq); 
   for(k=firstPlane-1;k<lastPlane;k++)
     for(j=0;j<ny;j++)
       for(i=0;i<nx;i++) { 
	  ind=csi_getInd(j,nx-i-1,k,ny,nx,nz);
	  ind=sliceInd*nz*ny*nx + ind; 
          if(ind < numtraces && (spectrum = calc_spec(ind,0,FALSE,TRUE,&updateflag)) != NULL) {
      	    if(nearest == 1) {
	      getNearest(spectrum, datasize, pnt, &pamp, &pfreq);
            } else {
	      pamp = *(spectrum + pnt);
            }
//Winfoprintf("calc_ll_map %f %f %d %d ",pfreq, vsFactor*pamp,ind,pnt);
            data[count] = pamp*vsFactor;  
	    count++;
	  } else {
            Werrprintf("Error making ll map: trace %d not avaiable.",ind);
	    return;
	  }
   }

   if(count == ntraces) writeCSIMap(path2, data, ntraces, sliceInd+1); 
 }
   releaseWithId("mmap");
}

// convert freq1 and freq2 to pnt1, pnt2, then add up points between 
// pnt1 and pnt2 for all traces.
void calc_li_map(char *path, double freq, double freq1, double freq2) {
   int ind = 0, updateflag = 0;
   int numtraces=nblocks * specperblock;
   float *data, *spectrum, *fptr;
   float integral;
   int pnt1, pnt2, np;
   int nx,ny,nz,i,j,k,count;
   int ns,sliceInd,ntraces,firstSlice,lastSlice,firstPlane,lastPlane;
   double d;
   char path2[MAXSTR];
   double vsFactor = aip_getVsFactor();

   if(numtraces < 1) {
      init2d(GET_REV,GRAPHICS);
      numtraces=nblocks * specperblock;
      if(numtraces < 1) {
        Werrprintf("No data.");
        return;
      }
   }

   csi_getCSIDims(&nx, &ny, &nz, &ns);
   if(numtraces != (nx*ny*nz*ns)) {
      Werrprintf("Abort: data size %d does not match parameter value %d",numtraces,(nx*ny*nz)); 
      return;
   }

   if(!P_getreal(CURRENT, "csiSlice", &d, 1)) sliceInd=(int)d;
   else sliceInd = 0;

   // check slice
   if(ns == 1) sliceInd=1; // single slice data
   if(sliceInd > ns) sliceInd=1; // slice out of bound

   if(nz > 1) {
      firstSlice = 1; lastSlice = 1;
      firstPlane = 1; lastPlane=nz;
   } else if(sliceInd <= 0) {
      firstSlice = 1; lastSlice = ns;
      firstPlane = 1; lastPlane=1;
   } else {
      firstSlice = sliceInd; lastSlice = sliceInd;
      firstPlane = 1; lastPlane=1;
   }

   // Note, for 2D data, ntraces=numtraces/ns;
   // numtraces is total number of traces. 
   // if ns=1 (this is the case for single slice, or 3D data), ntraces=numtraces
   ntraces = nx*ny*nz;

   data = (float *) allocateWithId(ntraces * sizeof(float), "mmap");
   if(data == 0) {
	Werrprintf("cannot allocate mmap buffer");
        return;
   }
   
 clearVar("csiMapSel");
 checkMapPath(path);
 for(sliceInd=firstSlice-1; sliceInd<lastSlice; sliceInd++) {
   // append freq and file extension to path
   if(ns>1) sprintf(path2,"%s_%d.fdf", path, sliceInd+1);
   else sprintf(path2,"%s.fdf", path);

   count=0;
   pnt1 = freq_to_dp(freq1); 
   pnt2 = freq_to_dp(freq2); 
   for(k=firstPlane-1;k<lastPlane;k++)
     for(j=0;j<ny;j++)
       for(i=0;i<nx;i++) { 
	  ind=csi_getInd(j,nx-i-1,k,ny,nx,nz);
	  ind=sliceInd*nz*ny*nx + ind; 
          if(ind < numtraces && (spectrum = calc_spec(ind,0,FALSE,TRUE,&updateflag)) != NULL) {
            fptr = spectrum+pnt1;
            integral = *fptr;
            np = pnt2-pnt1+1;
            while(--np) integral += *fptr++;
            data[count] = integral*vsFactor;  
            count++;
//Winfoprintf("calc_li_map %f %f %f %f %d %d %d",freq, freq1, freq2, vsFactor*integral,ind,pnt1, pnt2);
	  } else {
            Werrprintf("Error making li map: trace %d not avaiable.",ind);
	    return;
	  }
   }

   if(count == ntraces) writeCSIMap(path2, data, ntraces, sliceInd+1); 
 }
   releaseWithId("mmap");
}

void calc_ll_maps( char *path, int nearest) {
  double freq;
  int i = 0;
  int n = P_getsize(CURRENT,"llfrq",NULL);
  char path2[MAXSTR];
  
  if(n > 1) { // llfrq contains freqs n peaks.
     for(i=0; i<n; i++) {
	if(!P_getreal(CURRENT,"llfrq", &freq, i+1)) { 
          sprintf(path2, "%s_%.2f", path,freq_to_csPPM(freq));
	  calc_ll_map(path2, nearest, freq);	
        }
     }
  }
}

void calc_li_maps( char *path) {
  double freq, freq1, freq2;
  int i = 0;
  int n = P_getsize(CURRENT,"llfrq",NULL);
  char path2[MAXSTR];
  
  if(n > 1) { // llfrq contains freqs n peaks.
     for(i=0; i<n; i++) {
	if(!P_getreal(CURRENT,"llfrq", &freq, i+1)) { 
          sprintf(path2, "%s_%.2f", path,freq_to_csPPM(freq));
          getRegion4Freq(freq, &freq1, &freq2);
	  calc_li_map(path2, freq, freq1, freq2);	
        }
     }
  }
}

// the following functions will be called by aipSpecViewList to display 
// fid or spec data, assuming variables are set property because 
// these functions will be called after init2d or initfid. 
//
// first point of zoomed region
int getFistPoint() {
  if(nblocks <1) init2d(GET_REV,GRAPHICS);
  return fpnt;
}

// number of points in zoomed region
int getNumPoints() {
  if(nblocks <1) init2d(GET_REV,GRAPHICS);
  return npnt;
}

// vscale for fid or spec 
double getVScale(int fidflag) {
  double d;
  if(fidflag) {
    if (P_getreal(CURRENT, "vf", &d, 1)) d = wc2max;
    return d/((float)wc2max*CALIB);
  } else {
    if (P_getreal(CURRENT, "vs", &d, 1)) d = 100*wc2max;
    return d / (float)wc2max;  
  }
}

// return yoff (normalized to 1.0)
double getYoff() {
  double d, yoff=0.0;
  if (!P_getreal(CURRENT, "vp", &d, 1)) yoff = d;
  if (!P_getreal(GLOBAL, "wc2max", &d, 1) && d>0) yoff /= d;
  else yoff=0.0;
  return yoff;
}

// number traces
int getNtraces() {
  init2d(GET_REV,GRAPHICS);
  return nblocks * specperblock;
}

// number of points per trace
int getSpecPoints() {
  if(nblocks <1) init2d(GET_REV,GRAPHICS);
  return fn/2;
}
int getFidPoints() {
  double d;
  if (!P_getreal(CURRENT, "np", &d, 1)) return (int)(d/2);
  else {
    initfid(1);
    return fn/2;
  }
}

// Note, trace starts from 0 for both get_one_fid and calc_user.
// call this only after caller successfully called initfid(1). 
float *aip_GetFid(int trace) {
  if(nblocks <1) initfid(1);
  if ((trace < 0) || (trace >= nblocks * specperblock))
  {
    trace = 0;
  }
  float *data = get_one_fid(trace,&fn,&c_block, FALSE);
  c_buffer = trace;
  c_first  = trace;
  c_last   = trace;
  return data;
}

// get from D_PHASFILE
// call this only after caller successfully called init2d(GET_REV,GRAPHICS). 
float *aip_GetSpec(int trace) {
  int dummy;
  if(nblocks <1) init2d(GET_REV,GRAPHICS);
  if ((trace < 0) || (trace >= nblocks * specperblock))
  {
    trace = 0;
  }
  float *data = calc_user(trace, 0, FALSE,TRUE,&dummy,D_PHASFILE);
  return data;
}

float *aip_GetBaseline(int trace) {
    return getBcdata(trace, fn/2, mspecProcessing); 
}

float *aip_GetFit(int trace) {
   return getFitting(trace, fn/2, mspecProcessing); 
}

// nz > 1 is 3D data. 
// ns > 1 is multi-slice data.
// force ns=1 if nz>1
void csi_getCSIDims(int *nx, int *ny, int *nz, int *ns) {
  int nv=1,nv2=1,nv3=1,nslice=1;
  double d;
  int ntraces = getNtraces();
  if(!P_getreal(CURRENT, "nv", &d, 1) && d > 1) nv=(int)d;
  if(!P_getreal(CURRENT, "nv2", &d, 1) && d > 1) nv2=(int)d;
  if(!P_getreal(CURRENT, "nv3", &d, 1) && d > 1) nv3=(int)d;

  // get # of slices (pss array size)
    planParams *tag = getCurrPlanParams(CSI2D);
    vInfo paraminfo;
    if(tag != NULL) {
      if(!P_getVarInfo(CURRENT, tag->pss.name, &paraminfo) ) nslice = paraminfo.size;
    } else {
      if(!P_getVarInfo(CURRENT, "pss", &paraminfo) ) nslice = paraminfo.size;
    } 

  // overwrite with fn1,fn2,fn3 if active
  if(nv3 > 1 && P_getactive(CURRENT,"fn3")>0 && 
	P_getreal(CURRENT, "fn3", &d, 1)==0 && d > 1) nv3=(int)d/2; 
  if(nv2 > 1 && P_getactive(CURRENT,"fn2")>0 && 
	P_getreal(CURRENT, "fn2", &d, 1)==0 && d > 1) nv2=(int)d/2; 
  if(nv > 1 && P_getactive(CURRENT,"fn1")>0 && 
	P_getreal(CURRENT, "fn1", &d, 1)==0 && d > 1) nv=(int)d/2; 

  // overwrite with fnv,fnv2,fnv3 if active
  if(nv3 > 1 && P_getactive(CURRENT,"fnv3")>0 && 
	P_getreal(CURRENT, "fnv3", &d, 1)==0 && d > 1) nv3=(int)d; 
  else if(nv3 > 1 && P_getreal(CURRENT, "fnv3", &d, 1)==0 && d > 1) {
  	// if fnv3>1, but not active, treat 3D data as multi slice data
	nslice = (int)d/2;
        nv3 = 1;
  }

  if(nv2 > 1 && P_getactive(CURRENT,"fnv2")>0 && 
	P_getreal(CURRENT, "fnv2", &d, 1)==0 && d > 1) nv2=(int)d; 
  if(nv > 1 && P_getactive(CURRENT,"fnv")>0 && 
	P_getreal(CURRENT, "fnv", &d, 1)==0 && d > 1) nv=(int)d; 

  if(nv*nv2*nv3*nslice == ntraces) { // multi 3D
    *nx=nv2;
    *ny=nv;
    *nz=nv3;
    *ns=nslice;
  } else if(nv*nv2*nv3 == ntraces) { // 3D
    *nx=nv2;
    *ny=nv;
    *nz=nv3;
    *ns=1;
  } else if(nv*nv2*nslice == ntraces) { // multi slices
    *nx=nv2;
    *ny=nv;
    *nz=1;
    *ns=nslice;
  } else { // single slice
    *nx=nv;
    *ny=ntraces/nv;
    *nz=1;
    *ns=1;
  } 
  //Winfoprintf("%d traces in %d x %d x %d grid of %d slices",ntraces,*nx,*ny,(*nz),(*ns));
}

// this convert 3D indexes to 1D index
// nx,ny,nz are size of the dimensions,
// ix,iy,iz are indexes of the dimensions, starting from zero. 
// return index also starts from zero.
// loops start from zero, assuming nz is outer loop, and nx is inner loop
// to reverse nx dimension, call csi_getInd(nx-ix-1,iy,iz,nx,ny,nz)
// ny and nz may also be reversed the same way.
int csi_getInd(int ix, int iy, int iz, int nx, int ny, int nz) {
   return iz*ny*nx + iy*nx + ix;
}

void csi_getVoxInd(int *ix, int *iy, int *iz, int ind, int nx, int ny, int nz) {
   int i, ij;
   if(nx<1) nx=1;
   if(ny<1) ny=1;
   ij = (ind+1) % (nx*ny);
   i = ij % nx;
   *iz=(ind+1-ij)/(nx*ny) - 1;
   *iy=(ij-i)/nx - 1;
   *ix=i - 1;
}

#define MIN(x,y) (x < y ? x : y)
#define MAX(x,y) (x > y ? x : y)
#define INSIDE 0
#define OUTSIDE 1

#ifndef AIPPOINTS

typedef struct Dpoint {
    double x;
    double y;
} Dpoint_t;

#endif

int InsidePolygon(Dpoint_t *polygon,int N,Dpoint_t p)
{
  int counter = 0;
  int i;
  double xinters;
  Dpoint_t p1,p2;

  p1 = polygon[0];
  for (i=1;i<=N;i++) {
    p2 = polygon[i % N];
    if (p.y > MIN(p1.y,p2.y)) {
      if (p.y <= MAX(p1.y,p2.y)) {
        if (p.x <= MAX(p1.x,p2.x)) {
          if (p1.y != p2.y) {
            xinters = (p.y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
            if (p1.x == p2.x || p.x <= xinters)
              counter++;
          }
        }
      }
    }
    p1 = p2;
  }

  if (counter % 2 == 0)
    return(OUTSIDE);
  else
    return(INSIDE);
}

void getCursorFreq(double *freq, double *freq1, double *freq2, double *width) {
   double cfreq;
   if(P_getreal(CURRENT, "cr", &cfreq, 1)) cfreq=0.0;

   cfreq = cs_to_freq(cfreq);
   getRegion4Freq(cfreq, freq1, freq2);
   *freq=cfreq;
   *width = sw;
}

void getCursorFreq2(double *freq, double *freq1, double *freq2, double *width) {
   double cfreq,dfreq;
   if(P_getreal(CURRENT, "cr", &cfreq, 1)) cfreq=0.0;
   if(P_getreal(CURRENT, "delta", &dfreq, 1)) dfreq=0.0;
   cfreq=cfreq-dfreq;

   cfreq = cs_to_freq(cfreq);
   getRegion4Freq(cfreq, freq1, freq2);
   *freq=cfreq;
   *width = sw;
}

void aip_writeCSIMap(char *path, float *data, int mapInd, int rows, int cols, double x, double y, double w, double h) {
   int rank = 2;
   int dims[3];
   dims[0] = rows;
   dims[1] = cols;
   dims[2] = 1;
   int numtraces = rows*cols;
   int sliceInd = 1;
   double d;
   if(!P_getreal(CURRENT, "csiSlice", &d, 1)) sliceInd=(int)d;

   planParams *tag = getCurrPlanParams(CSI2D);
   tag->pos1.value=x;
   tag->pos2.value=y;
   tag->dim1.value=w;
   tag->dim2.value=h;

   writeMMap(path, data, numtraces, sliceInd, mapInd, dims, rank, tag);
}
