/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <unistd.h>

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <sys/mman.h>

#ifdef LINUX
#include <sys/time.h>
#endif

#ifdef __INTERIX
#include <arpa/inet.h>
#else
#ifdef SOLARIS
#include <sys/types.h>
#endif
#include <netinet/in.h>
#ifdef SOLARIS
#include <inttypes.h>
#endif
#endif

#include "aipCommands.h"
using namespace aip;
#include "aipDataInfo.h"
#include "aipDataManager.h"
#include "aipVolData.h"
#include "aipOrthoSlices.h"

extern "C" {
#include "group.h"
#include "pvars.h"
#include "params.h"
#include "data.h"
#include "vnmrsys.h"
#include "variables.h"
#include "wjunk.h"
//#include "aipCInterface.h"

extern int fpnt,npnt,specIndex;
extern int graphToVnmrJ( char *message, int messagesize );
extern float *get_one_fid(int curfid, int *np, dpointers *c_block, int dcflag);
extern float *calc_spec(int trace, int fpoint, int dcflag, int normok, int* newspec);
extern float *calc_user(int trace, int fpoint, int dcflag, int normok, int* newspec, int dtype);
extern float *get_data_buf(int);
extern float *gettrace(int trace, int fpoint);
void g3d_graphicsUpdate(char* str);
extern int init2d(int get_rev, int dis_setup);
extern int check2d(int get_rev);
}

extern float *spectrum; // dsww.c
extern int nblocks; // init2d.h
extern int dnpnt,npnt1,fpnt1,interuption,d2flag;
extern dfilehead	datahead,phasehead;

#define USE_MMAP

#define FALSE           0
#define TRUE            1

#define	BIT_ON(a,b)		(a) |= (b)
#define	BIT_OFF(a,b)	(a )&= ~(b)
#define	BIT_TST(a,b)	a&b?1:0
#define	BIT_SET(a,b,f)	if(f) BIT_ON(a,b); else BIT_OFF(a,b)
#define	MSK_SET(a,b,f)	BIT_OFF(a,b); BIT_ON(a,f)int check2d(int get_rev);

// jFunc id code

#define G3D   	100

// command codes sent from vnmrj via jFunc(100,key,value)

// key codes

#define SETACTIVE   0   // set active
#define SETSHOW     1   // set active and open window

// value codes

#define TRACK_DATA  1
#define TRACK_FDF   2
#define TRACK_ALL   3

// command return codes sent to vnmrj via graphToVnmrJ

#define READFDF     21
#define BGNXFER     22
#define GETDATA	    23
#define GETFDF      24
#define SETPOINT	25
#define SETSEM      26
#define MMAPTEST    30

// POINT variables
    
#define G3DPNT        0 // g3dpnt
#define G3DROT        1 // g3drot

#define D1        0x00000000
#define D2        0x00000001
#define D3        0x00000002
#define DIM       0x00000003
#define DCOMPLEX  0x00000008
#define FID       0x00000008
#define SPECTRUM  0x0000000C
#define IMAGE     0x00000000
#define DCONI     0x00000004
#define DTYPE     0x0000000C

#define NODTYPE   0x00000010
#define FID2D     0x00000028
#define SPEC2D    0x0000002C
#define MMAPPED   0x00000040
#define OTHER     0x00000080

#define DGETBUF  1
#define GETTRACE 2
#define GET1FID  3
#define GET1SPEC 4
#define GETSPEC  5

#define GETERROR	100

static int initialized=0;
static dfilehead filehdr;
static int notopen=0;
static int file_type=0;
static int data_source=0;
static int ntraces=1;
static int numblks=0;
static int ebytes=4;
static int numpts=0;
static int first_trace=0;
static int show=0;
static int dtype=OTHER;
static int xfersize;
static int datasize;
static float *xferdata=NULL;
static int status=0;

static int debug=0;

#ifndef MAXPATH
#define MAXPATH 512
#endif

static char imagedir[MAXPATH];
static char path[MAXPATH];
typedef union d32 {
	int     i; 
    float   f;
} d32;

static int getGraphicsType(char *n);

#define PACKFLT(dst,src) ((d32*)(dst))->i=htonl(((d32*)(src))->i)

static struct  {
	int	id;
	int	code;
	int	value;
} jvnmrj;

static struct  {
    int id;
    int code;
    int value;
    int frame;
    int x;
    int y;
    int z;
} pdata;

static struct  {
	int	id;
	int	code;
	int dtype; 	// dtype
	int	np;     // np size
	int	trace;  // current trace
	int	traces; // number of traces
	int	slices; // number of slices
	int	pts;    // number of xfer pts (floats)
	int rp; 	// rp
	int lp; 	// lp
	int ymin; 	// ymin
	int ymax; 	// ymax
	int mean; 	// average
	int stdev;  // variance
	int sx;     // x-span
	int sy;     // y-span
	int sz;     // z-span
} jdata;

static void setVPntVar(float x, float y, float z, float f){
    if(!initialized)
        return;
    double vx=0;
    if(P_getreal(GLOBAL,"g3dpnt",&vx,1)>=0){
        P_setreal(GLOBAL,"g3dpnt",x,1);
        P_setreal(GLOBAL,"g3dpnt",y,2);
        P_setreal(GLOBAL,"g3dpnt",z,3);
        P_setreal(GLOBAL,"g3dpnt",f,4);
       // writelineToVnmrJ("pnew", "4 g3dpnt aipXYlast aipXZlast aipYZlast");
    }
}

void sendVpntToVnmrj(float x, float y, float z, int frame){
    if(!initialized)
        return;
    pdata.id = htonl(G3D);
    pdata.code = htonl(SETPOINT);
    pdata.value = htonl(G3DPNT);
    pdata.frame = htonl(frame);//
    PACKFLT(&(pdata.x),&x);
    PACKFLT(&(pdata.y),&y);
    PACKFLT(&(pdata.z),&z);
    graphToVnmrJ((char*)(&pdata), sizeof(pdata));
}

void sendG3drot(float x, float y, float z){
    if(!initialized)
       return;
    pdata.id = htonl(G3D);
    pdata.code = htonl(SETPOINT);
    pdata.value = htonl(G3DROT);
    PACKFLT(&(pdata.x),&x);
    PACKFLT(&(pdata.y),&y);
    PACKFLT(&(pdata.z),&z);
    graphToVnmrJ((char*)(&pdata), sizeof(pdata));
}

static void sendvnmrj(int code, int value){
     jvnmrj.code=htonl(code);
     jvnmrj.value=htonl(value);
     graphToVnmrJ((char*)(&jvnmrj), sizeof(jvnmrj));    
}
static void sendError(int id){
    if(!initialized)
       return;
     jvnmrj.code=htonl(GETERROR);
     jvnmrj.value=htonl(id);
     graphToVnmrJ((char*)(&jvnmrj), sizeof(jvnmrj));    
}

static void initialize(int show){
    if(!initialized){
        jvnmrj.id = htonl(G3D);
		jvnmrj.code = htonl(SETACTIVE);
		jvnmrj.value = htonl(0);

		jdata.id = htonl(G3D);
		jdata.code = htonl(GETDATA);
		graphToVnmrJ((char*)(&jvnmrj), sizeof(jvnmrj));
		//execString("g3dinit(2)\n");
	   	execString("vnmrjcmd('g3dinit')\n");  // initialize vnmrj
		execString("g3dinit(0)\n"); // initialize parameters
		initialized=1;
    }
    //else
    switch(show){
    case 1:
    	execString("vnmrjcmd('g3d','window','true')\n"); // set window show true
    	break;
    case 2:
    	execString("vnmrjcmd('g3d','window','false')\n"); // set window show false
    	break;
    case 3:
	    execString("vnmrjcmd('g3d','window','toggle')\n"); // toggle window show
	    break;
    }
}

static void checkParams(int mode){
    double value=0; 
    status=0;
    
    if(P_getstring(GLOBAL,"imagedir",imagedir,1,MAXPATH) <0){
        (void)strcpy(imagedir,curexpdir);
        (void)strcat(imagedir,"/datadir3d/data/data.fdf");
        (void)P_creatvar(GLOBAL,"imagedir",T_STRING);
        (void)P_setstring(GLOBAL,"imagedir",imagedir,1);
    }
    if(P_getreal(GLOBAL,"g3dtype",&value,1)<0){
        status |= NODTYPE;
    }
}

static void setDtype(int type){
    if(!initialized)
        return;  
    double value=0;
	bool newdtype=false;

    if (P_getreal(GLOBAL,"g3dtype",&value,1)<0) {
		(void)P_creatvar (GLOBAL,"g3dtype", T_REAL);
		newdtype=true;
	}
	if (newdtype || type!=value) {
		P_setreal(GLOBAL,"g3dtype",(double)type,1);
		if(debug)
			Winfoprintf("setDtype: %d\n",type);
		execString("g3drestore\n"); // initialize vj controls
		//execString("g3dinit(1)\n"); // initialize vj controls
		// writelineToVnmrJ("pnew", "2 g3di g3dtype");
                appendJvarlist("g3di g3dtype");
	}
}

void setVrot(float x, float y, float z){
    double value;
    if(P_getreal(GLOBAL,"g3drot",&value,1)>=0){
        P_setreal(GLOBAL,"g3drot",x,1);
        P_setreal(GLOBAL,"g3drot",y,2);
        P_setreal(GLOBAL,"g3drot",z,3);
        sendvnmrj(SETSEM,1);
    }
}

void setVpnt(float x, float y, float z, float f){
    if(!initialized)//
        return;  
    setVPntVar(x,y,z,f);
    sendvnmrj(SETSEM,0);
}

void setVflt(int index, float value){
    if(!initialized)
        return;
    double oldval;
    if(P_getreal(GLOBAL,"g3df",&oldval,1)>=0){
        P_setreal(GLOBAL,"g3df",value,index);
        sendvnmrj(SETSEM,2);
    }
}
static int open_data_file()
{
	int err;
	notopen=0;		
	path[0]=0;
	err = D_gethead(file_type, &filehdr);
	
	if (err == D_NOTOPEN){
		err = D_getfilepath(file_type, path, curexpdir);
		if(err){
			Werrprintf("g3d: could not open data file\n");
			D_error(err);
			return 0;
		}
		Wscrprintf("g3d: %s had to be re-opened\n",path);
		err = D_open(file_type, path, &filehdr);
		notopen=1;
	}
	if (err){
		Werrprintf("g3d : could not open %s\n",path);
		D_error(err);
		return 0;
	}
	ntraces=filehdr.ntraces;
	numblks=filehdr.nblocks;
	numpts=filehdr.np;
	ebytes=filehdr.ebytes;
//	// there seems to be a bug where filehdr.ebytes is now being set to 0 for live data
    ebytes=(ebytes==0)?4:ebytes;
    if(debug)
	Winfoprintf("open_data_file: type=%d ntraces=%d, nblocks=%d np=%d ebytes=%d\n",
			file_type,filehdr.ntraces,filehdr.nblocks,filehdr.np,filehdr.ebytes);

    return 1;
}

void g3d_graphicsUpdate(char *n){
	int newtype=getGraphicsType(n);
	int valid=WgraphicsdisplayValid(n);
	if(debug)
		Winfoprintf("graphics: %d %d %s\n",valid,newtype,n);
	if(newtype==OTHER)
		return;
	//if(initialized && newtype !=OTHER && newtype !=dtype){
	if(initialized && newtype !=dtype){
		char tmp[256];
		tmp[0]=0;
		sprintf(tmp,"2 g3dup:%02d %s",newtype,n);
		writelineToVnmrJ("pnew", tmp);
	}
	dtype=newtype;
}

static int getGraphicsType(char *n){
	if(strcmp(n,"dconi")==0||strcmp(n,"ds2d")==0)
		return DCONI;
	if(strcmp(n,"ds")==0)
		return SPECTRUM;
	if((n[0]=='d') && (n[1]=='s')) // ds, dssh, dssa, dss
		return SPEC2D;
	if(strcmp(n,"df")==0)
		return FID;
	if((n[0]=='d') && (n[1]=='f')) // df, dfsh, dfsa, dfs
		return FID2D;
	return OTHER;
}

static float *get_trace(int id)
{
	int err;
	float *data=NULL;
	
	switch(data_source){
	case GET1FID: // This code started failing after vj 3.2
//		{
//			dpointers	c_block;	/* pointer to PHASFILE data		*/
//
//			if ((spectrum = get_one_fid(first_trace+id,&numpts,&c_block, FALSE)) == 0){
//			   sendError(1); // data file error
//			}
//			data=spectrum;
//		}
//		break;
	case DGETBUF:
	    {
			int block=id/ntraces;
			int trace=id-block*ntraces;
			int dl=numpts*ebytes;
		    dpointers  inblock;
			err = D_getbuf(file_type, numblks, block, &inblock);

			if(err){
				D_error(err);
				return NULL;
			}
		    data = (float *)(inblock.data+trace*dl);

            // mark buffer as releasable
		    if ( (err = D_release(file_type, block)) ){
		    	D_error(err);
				return NULL;
		    }
	    }
		break;
	case GETSPEC:
		{
			if ((spectrum = gettrace(fpnt1+id,fpnt)) == 0){
				sendError(1); // data file error
				return NULL;
			}
			data=spectrum;
		}
		break;
	case GET1SPEC:
		{
			if ((spectrum = get_data_buf(first_trace+id)) == 0){
			   sendError(1); // data file error
			   return NULL;
			}
			data=spectrum;
		}
		break;

	case GETTRACE:
		data=gettrace(fpnt1+id,fpnt);
		break;
    }
    return data;
}

static void sendData(int code, int mode){

	float *data=NULL;
	float rp=0;
	float lp=0;
	int *dst,*src;
	int i,j,k;
	int traces=1;
	int slices=1;
	int image_h, image_w, image_slices;
	float sx=1,sy=1,sz=1;
	double value=0;
	float y;
	int step=1;
	int pts=0;
	
	float ymin=1e20f;
	float ymax=-1e20f;
	float sum=0;
	float ss=0;
	float mean;
	float stdev;

	if(P_getreal(CURRENT,"np",&value,1)<0){
		sendError(code);
		return;
	}	
	numpts=(int)value;	
	
	data_source=0;
	int xtype=dtype;

	if(xtype==DCONI){ // dconi
		data_source=GETTRACE;
		file_type=D_DATAFILE;
		if(!open_data_file()){
			sendError(1); // data file error
			return;
		}

		ntraces=npnt1;
		numpts=npnt;
		ebytes=4;
		numblks=1;
		if(debug)
			Winfoprintf("DCONI: npnt1=%d, npnt=%d\n",npnt1,npnt);
	}
	else if(xtype==SPEC2D){ // dssh, dssa, dss
		if(debug)
			Winfoprintf("SPEC2D: twod=%d, npnt=%d\n",d2flag,npnt);
		file_type=D_DATAFILE;
		if(!open_data_file()){
			sendError(1); // data file error
			return;
		}
		if(!d2flag){ // data has imaginary component
		    data_source=DGETBUF;
		}
		else{
			xtype=DCONI;
		    data_source=GETTRACE;
			ntraces=npnt1;
			numpts=npnt;
		}
		first_trace=0;
	}
	else if(xtype==SPECTRUM){ // ds special case
		file_type=D_DATAFILE;
		data_source=GET1SPEC;
		if(debug)
			Winfoprintf("SPECTRUM: specIndex=%d dnpnt=%d\n",specIndex,dnpnt);
		if(!open_data_file()){
			sendError(1); // data file error
			return;
		}
		ntraces=1;
		numblks=1;
		first_trace=specIndex-1;
	}
	else if(xtype==FID){ // df special case
		data_source=GET1FID;
		file_type=D_PHASFILE;
		if(debug)
			Winfoprintf("FID: specIndex=%d dnpnt=%d\n",specIndex,dnpnt);
		if(!open_data_file()){
			sendError(1); // data file error
			return;
		}
		ntraces=1;
		numblks=1;
		first_trace=specIndex;
	}
	else if(xtype==FID2D){ // dfsh, dfsa, dfs
		file_type=D_PHASFILE;
		data_source=DGETBUF;	
		if(!open_data_file()){
			sendError(1); // data file error
			return;
		}
		first_trace=0;
	}
    if(code==GETFDF){
    	xtype=IMAGE; // fdf files are inverted
    }   	
	else if(data_source==0){ // try getting default fdf file if all else fails
		code=GETFDF;
		xtype=IMAGE;
		//sendError(1); // data file error
		//return;
	}
	if(P_getreal(CURRENT,"rp",&value,1)>=0)
		rp=value;
	if(P_getreal(CURRENT,"lp",&value,1)>=0)
		lp=value;
	
	VolData *vdat=VolData::get();
	switch(code){
	case GETDATA: // return all fid or spectrum traces
		
		vdat->showObliquePlanesPanel(false);
		
		traces=numblks*ntraces;
		if(traces==0)
			traces=1;
	    
	    datasize=slices*traces*numpts*ebytes*step;
		for(i=0,k=0;i<traces;i++){
			data=get_trace(i);	
			if(data==NULL){
				sendError(2);
				return; // bad data error	
			}			
			for(j=0;j<numpts*step;j+=step,k++){
				y=data[j];
		        ymax=y>ymax?y:ymax;
		        ymin=y<ymin?y:ymin;
		        sum += y;
		        ss +=y*y;
		        pts++;
			}
		}
		break;
		
	case GETFDF: // return data from fdf file
		if(initialized)
			sendvnmrj(READFDF, show);
		specIndex=1;
		numpts=ntraces=0;
		//(void)aipJavaFile(imagedir, &image_w, &image_h, &image_slices, &sx, &sy, &sz, &data);
		{
		  DataManager::get()->javaFile(imagedir, &image_w, &image_h, &image_slices, &sx, &sy, &sz, &data);
		}
		if(!initialized)
			return;
        sendvnmrj(BGNXFER, show);

		numpts=image_w;
		traces=image_h;
		slices=image_slices;

		if(data==NULL){
			sendError(1);
			sendError(numpts);
			sendError(traces);
			return;	 // data error
		}
		pts=traces*slices*numpts;
		datasize=pts*4;
		for(i=0;i<traces*numpts*slices;i++){
			y=data[i];	
	        ymax=y>ymax?y:ymax;
	        ymin=y<ymin?y:ymin;
		    sum += y;
		    ss +=y*y;
		}
		break;
	default:
		sendError(code);
		return;	
	}
	xtype &=DTYPE;
	if(slices>1)
		BIT_ON(xtype,D3);
	else if(traces>1)
		BIT_ON(xtype,D2);
    xtype |=status;
	mean=sum/pts;
	stdev=sqrt((ss-sum*mean)/(pts-1));
	
	setDtype(xtype);
	
	jdata.code = htonl(code);
	if(slices>1 && vdat->volDataIsMapped())
		jdata.dtype = htonl(xtype|MMAPPED);
	else
		jdata.dtype = htonl(xtype);
	jdata.np = htonl(numpts);
	jdata.trace = htonl(specIndex-1);
	jdata.traces = htonl(traces);
	jdata.slices = htonl(slices);
	jdata.pts = htonl(pts);
	PACKFLT(&(jdata.rp),&rp);
	PACKFLT(&(jdata.lp),&lp);
	PACKFLT(&(jdata.ymin),&ymin);	
	PACKFLT(&(jdata.ymax),&ymax);	
	PACKFLT(&(jdata.ymin),&ymin);	
	PACKFLT(&(jdata.mean),&mean);	
	PACKFLT(&(jdata.stdev),&stdev);	
	PACKFLT(&(jdata.sx),&sx);	
	PACKFLT(&(jdata.sy),&sy);	
	PACKFLT(&(jdata.sz),&sz);	

	if(mode==0){
		if(slices>1 && vdat->volDataIsMapped()){
			xferdata=(float*)(&jdata);
			xfersize=sizeof(jdata);
			graphToVnmrJ((char*)xferdata, xfersize);
		}
		else{
			xfersize=sizeof(jdata)+datasize;
			xferdata=(float*)malloc(xfersize);
			if(xferdata==NULL){
				sendError(3);
				return; // malloc error	
			}		
			// copy and format the data
			dst=(int*)xferdata+sizeof(jdata)/4;
			switch(code){
			case GETDATA: // return all fid or spectrum traces
				for(i=0,k=0;i<traces;i++){
					data=get_trace(i);
					if(data==NULL){
						sendError(2);
						return;
					}
					for(j=0;j<numpts*step;j+=step,k++){
						PACKFLT((dst+k),(data+j));
					}
				}
				break;
			case GETFDF:
				for(i=0;i<pts;i++){
					PACKFLT((dst+i),(data+i));
				}
				//free(data);
				break;
			}
	    	dst=(int*)xferdata;	
			// copy the xfer structure
			src=(int*)(&jdata);
			for(i=0;i< (int) sizeof(jdata)/4;i++){
				*dst++ = *src++;
			}    	
			graphToVnmrJ((char*)xferdata, xfersize);
			free(xferdata);
		}
	}
	else{
		xferdata=(float*)(&jdata);
		xfersize=sizeof(jdata);
		graphToVnmrJ((char*)xferdata, xfersize);
	}
}

#define ARRAY_SIZE 10
static void mmaptest() {
    int i;
    static const char* test_map_file="/export/home/tmp/test";
    int fd = open(test_map_file, O_CREAT | O_TRUNC| O_RDWR, 0666 );
	if (fd == 0) {
		Werrprintf("Could not fopen file: %.900s", test_map_file);
		return;
	}
	// test if the file with sufficient size can be created
	(void) lseek( fd, 4*ARRAY_SIZE-1, SEEK_SET);
	int bytesWritten = write(fd, " ", 1 );
	if (bytesWritten != 1 ) {
		Werrprintf("Could not write mmap file: %.900s", test_map_file);
		close(fd);
	    return;
	}
	float *float_array = (float*)mmap((void*)0, ARRAY_SIZE*4, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if ((caddr_t)float_array == (caddr_t)(-1)) {
		Werrprintf("Could not mmap file: %.900s", test_map_file);
		return;
	}
	for(i=0;i<ARRAY_SIZE;i++){
		float_array[i]=(float)i;
	}
	msync(float_array, ARRAY_SIZE*4, MS_SYNC);
	close(fd);
	jvnmrj.code=htonl(MMAPTEST);
	jvnmrj.value=htonl(1);
    graphToVnmrJ((char*)(&jvnmrj), sizeof(jvnmrj));	
}

extern "C" {
void g3dcmd(int code, int value){

	switch(code){
	case MMAPTEST:
		mmaptest();
		break;
	case SETACTIVE:
	case SETSHOW:
		//tracking=value;
		initialize(value);
		break;
	case GETDATA:
	case GETFDF:
		checkParams(1);
		sendData(code,value);
		break;
	default:
		sendvnmrj(code, value); // echo message back
		break;		
	}
 }
}
