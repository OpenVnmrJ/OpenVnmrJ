/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/******************************************************************************
* File recon_all.c:  
*
* main  reconstruction for all sequences (fid data --> images)* note: no conversion of fid file to standard or compressed format is performed
*
* made into generalized recon including multi-channel 3-28-2002 M. Kritzer
* 
******************************************************************************/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include "data.h"
#include "group.h"
#include "mfileObj.h"
#include "symtab.h"
#include "vnmrsys.h"
#include "variables.h"
#include "process.h"
#include "epi_recon.h"
#include "phase_correct.h"
#include "mfileObj.h"

struct _recon_info
{
  svInfo svinfo;
  int do_setup;
  int pc_slicecnt;
  int slicecnt;
  int first_image;
  int dispflag;
  fdfInfo picInfo;
  MFILE_ID fidMd;
  int nchannels;
  int tbytes;
  int ebytes;
  int imglen;
  int ntlen;
  int epi_rev;
};
typedef struct  _recon_info reconInfo;

#define DEG_TO_RAD M_PI/180.
#define MM_TO_CM 0.10
#define SEC_TO_USEC 1000000.0
#define MSEC_TO_USEC 1000.0

#ifdef VNMRJ
#include "aipCInterface.h"
#endif

extern int interuption;
extern symbol **getTreeRoot();
extern varInfo *createVar();
extern void Vperror(char *);
extern int specIndex;

static float *slicedata, *pc, *sorted, *magnitude, *mag2;
static int *view_order;
static int *repeat;
static int *nts;
static int *pc_done;
static reconInfo rInfo; 
static int realtime_block;


/* prototype */
void recon_abort();
int write_fdf(int,float *,fdfInfo *,int *, int);
int svcalc( int, int, svInfo *,  int *, int *, int *);

/******************************************************************************/

/* the following courtesy of Numerical Recipes in C, Press, Teukolsy, et. al. */
#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr
static void four1(data, nn, isign)
    float *data; unsigned long nn; int isign;
{
    unsigned long n,mmax,m,j,istep,i;
    double wtemp,wr,wpr,wpi,wi,theta;
    float tempr,tempi;
    
    n=nn << 1;
    j=1;
    for (i=1;i<n;i+=2) {
	if (j > i) {
	    SWAP(data[j],data[i]);
	    SWAP(data[j+1],data[i+1]);
	}
	m=n >> 1;
	while (m >= 2 && j > m) {
	    j -= m;
	    m >>= 1;
	}
	j += m;
    }
    mmax=2;
    while (n > mmax) {
	istep=mmax << 1;
	theta=isign*(6.28318530717959/mmax);
	wtemp=sin(0.5*theta);
	wpr = -2.0*wtemp*wtemp;
	wpi=sin(theta);
	wr=1.0;
	wi=0.0;
	for (m=1;m<mmax;m+=2) {
	    for (i=m;i<=n;i+=istep) {
		j=i+mmax;
		tempr=wr*data[j]-wi*data[j+1];
		tempi=wr*data[j+1]+wi*data[j];
		data[j]=data[i]-tempr;
		data[j+1]=data[i+1]-tempi;
		data[i] += tempr;
		data[i+1] += tempi;
	    }
	    wr=(wtemp=wr)*wpr-wi*wpi+wr;
	    wi=wi*wpr+wtemp*wpi+wi;
	}
	mmax=istep;
    }	
    /* do a dc correction of sorts */
    data[1]=data[3];
    data[2]=data[4];
    /* shift zero frequency to center */
        for (i=0;i<nn/2;i++)
	{
	    j=nn/2+i;
	    SWAP(data[2*j+1],data[2*i+1]);
	    SWAP(data[2*j+2],data[2*i+2]);
	}

    return;
}
#undef SWAP

/*******************************************************************************/
/*******************************************************************************/
/*            RECON_ALL                                                        */
/*******************************************************************************/
/*******************************************************************************/
recon_all ( argc, argv, retc, retv )

/* 

Purpose:
-------
Routine recon_all is the program for epi reconstruction.  

Arguments:
---------
argc  :  (   )  Argument count.
argv  :  (   )  Command line arguments.
retc  :  (   )  Return argument count.  Not used here.
retv  :  (   )  Return arguments.  Not used here

*/

int argc, retc;
char *argv[], *retv[];
/*ARGSUSED*/

{
/*   Local Variables: */
    FILE *f1;
    FILE *table_ptr;

    char filepath[MAXPATHL];
    char tablefile[MAXPATHL];
    char petable[MAXSTR];
    char sequence[MAXSTR];
    char msintlv[MAXSTR];
    char rcvrs_str[MAXSTR];
    char rscfilename[MAXPATHL];
    char epi_pc[MAXSTR];
    char recon_windowstr[MAXSTR];
    char aname[SHORTSTR];
    char arraystr[MAXSTR];
    char *ptr;
    char *imptr;
    char str[MAXSTR];
    short  *sdataptr;
    vInfo info;
    double rnseg, rfc, retl, rnv;
    double rcelem;
    double recon_force;
    double rimflag;
    double rnt;
    double wc, wc2;
    double ncomp,rechoes, nf, ni, repi_rev;
    double rslices,rnf;
    double *dptr;
    double dtemp;
    double acqtime;
    double psi, phi, theta;
    double cospsi,cosphi,costheta;
    double sinpsi, sinphi, sintheta;
    double *window;
    dpointers inblock;
    dblockhead *bhp;
    dfilehead *fid_file_head;
    float *fptr, *nrptr;
    float *fdataptr;
    float a,b,c,d;
    float t1, t2, *pt1, *pt2, *pt3, *pt4, *pt5, *pt6;
    int echoes;
    int *idataptr;
    int imglen,dimfirst,dimafter,imfound;
    int imzero;
    int icnt;
    int nv;
    int ispb, itrc;
    int iecho;
    int blockctr;
    int pc_offset, sort_offset;
    int soffset, soff;
    int magoffset;
    int iro;
    int ipc;
    int ntlen;
    int nt;
    int within_nt;
    int dsize;
    int views, nro;
    int slices, etl, nblocks,ntraces, slice_reps;
    int slicesperblock, viewsperblock;
    int error,error1,error2;
    int ipt, npts;
    int im_slice;
    int pc_slice, pc_view;
    int itab;
    int ichan;
    int view, slice, echo;
    int nchannels=1;
    int within_views=1;
    int within_slices=1;
    int rep;
    int i, min_view, iview;
    int narg;
    int zeropad=0;
    int nro_ft;
    int irep;
    int ctcount;
    short index;
    useconds_t snoozetime;

    /* flags */
    int pc_option=OFF;
    int recon_window=NOFILTER;
    int done=FALSE;
    int imflag=TRUE;
    int multi_shot=FALSE;
    int transposed=FALSE;
    int epi_seq=FALSE;
    int multi_slice=FALSE;
    int epi_rev=TRUE;
    int recon_done=FALSE;
    int recon_skip=FALSE;
    int slice_compressed=TRUE; 
    int phase_compressed=TRUE; /* a good assumption  */
    int flash_converted=FALSE;
    int epi_me=FALSE;
    int recon_forced=FALSE;

    symbol **root;

    /*******************/
    /* executable code */
    /*******************/

   /* default :  do complete setup and mallocs */
    rInfo.do_setup=TRUE;
    
    if(interuption)
      {
	Vperror("recon_all: aborting by request");	
	(void)recon_abort();
	ABORT;
      }
    
    /* what sequence is this ? */
    error=P_getstring(CURRENT, "seqfil", sequence, 1, MAXSTR);  
    if(error)
      {
	Vperror("recon_all: Error getting seqfil");	
	(void)recon_abort();
	ABORT;
      }		
    if(strstr(sequence,"epi"))
      epi_seq=TRUE;

   /* look for command line arguments */
    narg=1;
    if (argc>narg)
      {
	if(strcmp(argv[narg],"acq")==0)  /* not first time through!!! */
	  rInfo.do_setup=FALSE;         
	narg++;

	if(argc>narg)
	  {
	    (void)strcpy(rInfo.picInfo.imdir,argv[narg++]);
	    rInfo.picInfo.fullpath=TRUE;
	    if(epi_seq)
	      {
		if(argc>narg)
		  (void)strcpy(epi_pc,argv[narg++]);
		else
		  (void)strcpy(epi_pc,"POINTWISE");
	      }
	  }
	else
	  {
	    (void)strcpy(epi_pc,"POINTWISE");
	    (void)strcpy(rInfo.picInfo.imdir,"recon");
	    rInfo.picInfo.fullpath=FALSE;
	    epi_rev=TRUE;
	  }
      }
    else
      {
	/* try vnmr parameters */
	error=P_getstring(CURRENT, "epi_images", rInfo.picInfo.imdir, 1, MAXSTR);  
	if(!error)
	  { 
	    rInfo.picInfo.fullpath=FALSE;
	    if(epi_seq)
	      {
		error=P_getstring(CURRENT,"epi_pc", epi_pc, 1, MAXSTR);
		if(error)
		  (void)strcpy(epi_pc,"POINTWISE");
		
		error=P_getreal(CURRENT,"epi_rev", &repi_rev, 1);
		if(repi_rev>0.0)
		  epi_rev=TRUE;
		else
		  epi_rev=FALSE;
	      }
	  }
	else  /* try the resource file to get recon particulars */
	  {
	    (void)strcpy(rscfilename,curexpdir);
	    (void)strcat(rscfilename,"/recon_all.rsc");
	    f1=fopen(rscfilename,"r");
	    if(f1)
	      {
		(void)fgets(str,MAXSTR,f1);
		(void)sscanf(str,"image directory=%s",rInfo.picInfo.imdir);
		rInfo.picInfo.fullpath=FALSE;
		if(epi_seq)
		  {
		    (void)fgets(epi_pc,MAXSTR,f1);
		    (void)fgets(str,MAXSTR,f1);
		    (void)sscanf(str,"reverse=%d",&epi_rev);
		  }
		(void)fclose(f1);
	      }
	    else /* defaults */
	      {
		(void)strcpy(epi_pc,"POINTWISE");
		(void)strcpy(rInfo.picInfo.imdir,"recon");
		rInfo.picInfo.fullpath=FALSE;
		epi_rev=TRUE;
	      }
	  }
      }

if(rInfo.do_setup)
      {

	if(strstr(sequence,"ms"))
	  multi_slice=TRUE;

	if(strstr(sequence,"epidw"))
	  {
	    error=P_getstring(CURRENT, "ms_intlv", msintlv, 1, MAXSTR);  
	    if(error)
	      {
		Vperror("recon_all: Error getting ms_intlv");	
		(void)recon_abort();
		ABORT;
	      }
	    if(msintlv[0]=='y')
	      multi_slice=TRUE;
	  }	
	if(epi_seq)
	  {
	    /* decipher phase correction option */
	    if(strstr(epi_pc,"OFF")||strstr(epi_pc,"off"))
	  pc_option=OFF; 
	    else if(strstr(epi_pc,"POINTWISE")||strstr(epi_pc,"pointwise"))
	      pc_option=POINTWISE; 
	    else if(strstr(epi_pc,"LINEAR")||strstr(epi_pc,"linear"))
	      pc_option=LINEAR; 
	    else if(strstr(epi_pc,"QUADRATIC")||strstr(epi_pc,"quadratic"))
	      pc_option=QUADRATIC; 
	    else if(strstr(epi_pc,"CENTER_PAIR")||strstr(epi_pc,"center_pair")) 
	      pc_option=CENTER_PAIR; 
	    else if(strstr(epi_pc,"PAIRWISE")||strstr(epi_pc,"pairwise")) 
	      pc_option=PAIRWISE;
	    else if(strstr(epi_pc,"FIRST_PAIR")||strstr(epi_pc,"first_pair")) 
	      pc_option=FIRST_PAIR;
	    if(pc_option!=OFF)
	      {
		if((pc_option>MAX_OPTION)||(pc_option<MIN_OPTION))
		  {
		    Vperror("recon_all: Invalid phase correction option");
		    (void)recon_abort();
		    ABORT;
		  }
	      }
	  }
	else
	  {
	    epi_rev=FALSE;
	    pc_option=OFF;
	  }
    
	/* display images or not? */
        rInfo.dispflag=TRUE;
        error=P_getreal(CURRENT,"recondisplay",&dtemp,1);
        if(!error)
          rInfo.dispflag=(int)dtemp;

	/* get choice of filter, if any */
	error=P_getstring(CURRENT,"recon_window", recon_windowstr, 1, MAXSTR);
	if(error)
	  (void)strcpy(recon_windowstr,"NOFILTER");
	
	if(strstr(recon_windowstr,"OFF")||strstr(recon_windowstr,"off"))
	  recon_window=NOFILTER; 
	else if(strstr(recon_windowstr,"NOFILTER")||strstr(recon_windowstr,"nofilter"))
	  recon_window=NOFILTER; 
	else if(strstr(recon_windowstr,"BLACKMANN")||strstr(recon_windowstr,"blackmann"))
	  recon_window=BLACKMANN;
	else if(strstr(recon_windowstr,"HANN")||strstr(recon_windowstr,"hann"))
	  recon_window=HANN;
	else if(strstr(recon_windowstr,"HAMMING")||strstr(recon_windowstr,"hamming"))
	  recon_window=HAMMING;
	else if(strstr(recon_windowstr,"GAUSSIAN")||strstr(recon_windowstr,"gaussian"))
	  recon_window=GAUSSIAN;

	/* open the fid file */
	(void)strcpy ( filepath, curexpdir );
	(void)strcat ( filepath, "/acqfil/fid" );  
	(void)strcpy(rInfo.picInfo.fidname,filepath);
	rInfo.fidMd=mOpen(filepath,0,O_RDONLY);
	(void)mAdvise(rInfo.fidMd, MF_SEQUENTIAL);
	
	/* how many receivers? */
	error=P_getstring(CURRENT, "rcvrs", rcvrs_str, 1, MAXSTR);  
	if(!error)
	  {
	    nchannels=strlen(rcvrs_str);
	    for(i=0;i<strlen(rcvrs_str);i++)
	      if(*(rcvrs_str+i)!='y')
		nchannels--;
	  }
	rInfo.nchannels=nchannels;
	
	/* figure out if compressed in slice dimension */   
	error=P_getstring(CURRENT, "seqcon", str, 1, MAXSTR);  
	if(error)
	  {
	    Vperror("recon_all: Error getting seqcon");	
	    (void)recon_abort();
	    ABORT;
	  }
	if(str[1] != 'c')slice_compressed=FALSE;
	if(str[2] != 'c')phase_compressed=FALSE;
  
  
	error=P_getreal(CURRENT,"ns",&rslices,1);
	if(error)
	  {
	    Vperror("recon_all: Error getting ns");
	    (void)recon_abort();
	    ABORT;
	  }
	slices=(int)rslices;

	error=P_getreal(PROCESSED,"nf",&rnf,1);
	if(error)
	  {
	    Vperror("recon_all: Error getting nf");
	    (void)recon_abort();
	    ABORT;
	  }
  

		
	error=P_getreal(CURRENT,"nv",&rnv,1); 
	if(error)
	  {
	    Vperror("recon_all: Error getting nv");
	    (void)recon_abort();
	    ABORT;
	  }
	nv=(int)rnv;
  
	fid_file_head = (dfilehead *)(rInfo.fidMd->offsetAddr);
	ntraces=fid_file_head->ntraces;
	nblocks=fid_file_head->nblocks;
	nro=fid_file_head->np/2;
	rInfo.fidMd->offsetAddr+= sizeof(dfilehead);

	/* check for power of 2 in readout*/
	nro_ft=1;
	while(nro_ft<nro)
	  nro_ft*=2;
	if(nro<nro_ft)
	  {
	    zeropad=nro_ft-nro;
	    nro=nro_ft;
	  }
	
	error=P_getreal(CURRENT,"nseg",&rnseg,1); 
	if(error)
	  rnseg=1.0;
	
	error=P_getreal(CURRENT,"ne",&rechoes,1); 
	if(error)
	  rechoes=1.0;
	echoes=(int)rechoes;
	if(echoes>1&&epi_seq)
	  epi_me=echoes;
	
	error=P_getreal(CURRENT,"etl",&retl,1); 
	if(error)
	  retl=1.0;
	
	/* set up filter window if necessary */
	if((recon_window>NOFILTER)&&(recon_window<=MAX_FILTER))
	  {
	    if(!window)
	      window=(double*)allocateWithId(nro*sizeof(double),"recon_all");
	    (void)filter_window(recon_window, window, nro);
	  }
	else
	  window=NULL;
	
	
	/* get some epi related parameters */
	error1=P_getreal(PROCESSED,"flash_converted",&rfc,1);
	if(!error1)
	  flash_converted=TRUE;  
	
	/* sanity checks */
	if(flash_converted)
	  {
	    if(ntraces==nv*slices)
	      {
		phase_compressed=TRUE;
		slice_compressed=TRUE;
	      }
	    else if(ntraces==nv)
	      {
		phase_compressed=TRUE;
		slice_compressed=FALSE;
	      }	  
	    else if(ntraces==slices)
	      {
		phase_compressed=FALSE;
		slice_compressed=TRUE;
	      }	  
	    else if(ntraces==1)
	      {
		phase_compressed=FALSE;
		slice_compressed=FALSE;
	      }	  
	  }
	
	if(epi_seq)
	  {
	    error=P_getVarInfo(CURRENT,"image",&info); 
	    if(error)
	      {
		Vperror("recon_all: Error getting image");
		(void)recon_abort();
		ABORT;
	      }
	    imglen=info.size;
	  }
	else
	  imglen=1;
	
	error=P_getstring(CURRENT,"array", arraystr, 1, MAXSTR); 
	if(error)
	  {
	    Vperror("recon_all: Error getting array");
	    (void)recon_abort();
	    ABORT;
	  }
	/* locate image within array to see if reference scan was acquired */
	imzero=0;
	imptr=strstr(arraystr,"image");
	if(!imptr)
	  imglen=1;  /* image was not arrayed */
	else
	  {
	    /* how many when image=1? */
	    for(i=0;i<imglen;i++)
	      {
		error=P_getreal(CURRENT,"image",&rimflag,(i+1));
		if(error)
		  {
		    Vperror("recon_all: Error getting image element");
		    (void)recon_abort();
		    ABORT;
		  }
		if(rimflag<=0.0)
		  imzero++;
	      }
	  }

	/* get nt  array  */
	error=P_getVarInfo(CURRENT,"nt",&info); 
	if(error)
	  {
	    Vperror("recon_all: Error getting nt info");
	    mClose(rInfo.fidMd);
	    rInfo.fidMd=NULL;
	    (void)recon_abort();
	    ABORT;
	  }
	ntlen=info.size;	    /* read nt values*/
	nts=(int *)allocateWithId(ntlen*sizeof(int),"recon_all");
	for(i=0;i<ntlen;i++)
	  {
	    error=P_getreal(CURRENT,"nt",&dtemp,(i+1));
	    if(error)
	      {
		Vperror("recon_all: Error getting nt element");
		(void)recon_abort();
		ABORT;
	      }
	    nts[i]=(int)dtemp;
	  }

	if(slice_compressed)
	  {
	    slices=(int)rslices;
	    slicesperblock=slices;
	  }	
	else
	  {
	    P_getVarInfo(CURRENT, "pss", &info);
	    slices = info.size;
	    slicesperblock=1;
	    within_slices=nblocks/slices;
	  }	
	if(phase_compressed)
	  {
	    slice_reps=nblocks*slicesperblock/slices;
	    slice_reps -= imzero;
	    views=ntraces/slicesperblock;
	    views=nv;
	    viewsperblock=views;
	  }
	else
	  {
	    error=P_getreal(PROCESSED,"ni",&ni,1);
	    if(error)
	      {
		Vperror("recon_all: Error getting ni");
		(void)recon_abort();
		ABORT;
	      }
	    views=(int)ni;
	    views=nv;
	    slice_reps=nblocks*slicesperblock/slices;
	    slice_reps/=views;
	    slice_reps -= imzero;
	    viewsperblock=1;
	    within_views=nblocks/views;  /* views are outermost */
	    if(!slice_compressed&&!flash_converted)
	      within_slices/=views;
	  }
	
	if(slice_reps<1)
	  {
	    Vperror("recon_all: slice reps less than 1");
	    (void)recon_abort();
	    ABORT;
	  }
	
	etl=views; /* assuming single shot */
	multi_shot=FALSE;
	if(epi_seq)
	  {
	    if(rnseg>1.0)
	      {
		multi_shot=TRUE;
		etl=(int)views/rnseg;
	      }
	  }
	else 
	  {
	    if(retl > 1.0)
	      {
		multi_shot=TRUE;
		etl=(int)retl;
	      }
	  }
	
	if(flash_converted)
	  multi_shot=FALSE;
	
	if(multi_shot&&rInfo.do_setup)
	  {
	    /* open the table file */
	    error=P_getstring(GLOBAL, "userdir", tablefile, 1, MAXPATHL);  
	    if(error)
	      {
		Vperror("recon_all: Error getting userdir");	
		(void)recon_abort();
		ABORT;
	      }
	    (void)strcat(tablefile,"/tablib/" );  
	    error=P_getstring(CURRENT, "petable", petable, 1, MAXSTR);  
	    if(error)
	      {
		Vperror("recon_all: Error getting petable");	
		(void)recon_abort();
		ABORT;
	      }
	    (void)strcat(tablefile,petable);  
	    table_ptr = fopen(tablefile,"r");
	    if(!table_ptr)
	      {	
		Vperror("recon_all: Error opening table file");	
		(void)recon_abort();
		ABORT;
	      }	
	    
	    /* read in table for view sorting */
	    view_order = (int *)allocateWithId(views*sizeof(int), "recon_all");
	    while (fgetc(table_ptr) != '=') ;
	    itab = 0;
	    min_view=0;
	    while ((itab<views) && !done)
	      {
		if(fscanf(table_ptr, "%d", &iview) == 1)
		  {
		    view_order[itab]=iview;
		    if(iview<min_view)	
		      min_view=iview;
		    itab++;	
		  }
		else	
		  done = TRUE;
	      }
	    (void)fclose(table_ptr);
	    if(itab != views)
	      {
		Vperror("recon_all: Error wrong phase sorting table size");	
		(void)recon_abort();
		ABORT;
	      }
	    /* make it start from zero */
	    for(i=0;i<views;i++)
	      view_order[i]-=min_view;
	  }	
	
    
	/***********************************************************************************/ 
	/* get array to see how reference scan data was interleaved with acquisition */
	/***********************************************************************************/ 
	dimfirst=1;
	dimafter=1;

	if(imptr != NULL)  /* don't bother if no image */
	  {
	    imfound=0;
	    /* interrogate array to find position of 'image' */	
	    ptr=strtok(arraystr,",");
	    while(ptr != NULL)
	      {
		if(ptr == imptr)
		  {
		    imfound=1;
		  }
		/* is this a jointly arrayed thing? */
		if(strstr(ptr,"("))
		  {
		    while(strstr(ptr,")")==NULL) /* move to end of jointly arrayed list */
		      ptr=strtok(NULL,",");
		    
		    *(ptr+strlen(ptr)-1)='\0';
		  }
		strcpy(aname,ptr);
		error=P_getVarInfo(CURRENT,aname,&info);
		if(error)
		  {
		    Vperror("recon_all: Error getting something");
		    (void)recon_abort();
		    ABORT;
		  }
		
		if(imfound)
		  {
		    if(ptr != imptr)
		      dimafter *= info.size;	 /* get dimension of this variable */	    
		  }
		else
		  dimfirst *= info.size;
		
		ptr=strtok(NULL,",");
	      }
	  }
	else
	  dimfirst=nblocks;
	
	if(flash_converted)
	  dimafter*=slices;
	
	/* set up fdf header structure */
	rInfo.picInfo.nro=nro;
	rInfo.picInfo.npe=views;
	
	error=P_getreal(CURRENT,"lro",&dtemp,1); 
	if(error)
	  {
	    Vperror("recon_all: Error getting lro");
	    (void)recon_abort();
	    ABORT;
	  }
	rInfo.picInfo.fovro=dtemp;
	error=P_getreal(CURRENT,"lpe",&dtemp,1); 
	if(error)
	  {
	    Vperror("recon_all: Error getting lpe");
	    (void)recon_abort();
	    ABORT;
	  }
	rInfo.picInfo.fovpe=dtemp;
	error=P_getreal(CURRENT,"thk",&dtemp,1); 
	if(error)
	  {
	    Vperror("recon_all: Error getting thk");
	    (void)recon_abort();
	    ABORT;
	  }
	rInfo.picInfo.thickness=MM_TO_CM*dtemp;
	rInfo.picInfo.slices=slices;	
	rInfo.picInfo.echo=1;
	rInfo.picInfo.echoes=echoes;
	error=P_getreal(CURRENT,"te",&dtemp,1);
	if(error)
	  {
	    Vperror("recon_all: recon_all: Error getting te");
	    (void)recon_abort();
	    ABORT;
	  }
	rInfo.picInfo.te=(float)SEC_TO_MSEC*dtemp;
	error=P_getreal(CURRENT,"tr",&dtemp,1);
	if(error)
	  {
	    Vperror("recon_all: recon_all: Error getting tr");
	    (void)recon_abort();
	    ABORT;
	  }
	rInfo.picInfo.tr=(float)SEC_TO_MSEC*dtemp;
	rInfo.picInfo.ro_size=nro;
	rInfo.picInfo.pe_size=views;
	(void)strcpy(rInfo.picInfo.seqname, sequence);
	error=P_getreal(CURRENT,"ti",&dtemp,1);
	if(error)
	  {
	    Vperror("recon_all: recon_all: Error getting ti");
	    (void)recon_abort();
	    ABORT;
	  }
	rInfo.picInfo.ti=(float)SEC_TO_MSEC*dtemp;
	rInfo.picInfo.image=1.0;
	rInfo.picInfo.array_index=1;
	
	error=P_getreal(CURRENT,"sfrq",&dtemp,1);
	if(error)
	  {
	    Vperror("recon_all: recon_all: Error getting sfrq");
	    (void)recon_abort();
	    ABORT;
	  }  
	rInfo.picInfo.sfrq=dtemp;
	
	error=P_getreal(CURRENT,"dfrq",&dtemp,1);
	if(error)
	  {
	    Vperror("recon_all: recon_all: Error getting dfrq");
	    (void)recon_abort();
	    ABORT;
	  }  
	rInfo.picInfo.dfrq=dtemp;
	error=P_getreal(CURRENT,"psi",&psi,1);
	if(error)
	  {
	    Vperror("recon_all: recon_all: Error getting psi");
	    (void)recon_abort();
	    ABORT;
	  }
	error=P_getreal(CURRENT,"phi",&phi,1);
	if(error)
	  {
	    Vperror("recon_all: recon_all: Error getting phi");
	    (void)recon_abort();
	    ABORT;
	  }
	error=P_getreal(CURRENT,"theta",&theta,1);
	if(error)
	  {
	    Vperror("recon_all: recon_all: Error getting theta");
	    (void)recon_abort();
	    ABORT;
	  }
	
	/* estimate time for block acquisition */
	error = P_getreal(CURRENT,"at",&acqtime,1);
	if(error)
	  {
	    Vperror("recon_all: Error getting at");
	    (void)recon_abort();
	    ABORT;
	  }
	snoozetime=(useconds_t)(acqtime*ntraces*nt*SEC_TO_USEC);
	snoozetime=(useconds_t)(0.25*rInfo.picInfo.tr*MSEC_TO_USEC);

	cospsi=cos(DEG_TO_RAD*psi);
	sinpsi=sin(DEG_TO_RAD*psi);
	cosphi=cos(DEG_TO_RAD*phi);
	sinphi=sin(DEG_TO_RAD*phi);
	costheta=cos(DEG_TO_RAD*theta);
	sintheta=sin(DEG_TO_RAD*theta);
	
	rInfo.picInfo.orientation[0]=-1*cosphi*cosphi - sinphi*costheta*sintheta;
	rInfo.picInfo.orientation[1]=-1*cosphi*sinpsi + sinphi*costheta*cospsi;
	rInfo.picInfo.orientation[2]=-1*sinphi*sintheta;
	rInfo.picInfo.orientation[3]=-1*sinphi*cospsi + cosphi*costheta*sinpsi;
	rInfo.picInfo.orientation[4]=-1*sinphi*sinpsi - cosphi*costheta*cospsi;
	rInfo.picInfo.orientation[5]=cosphi*sintheta;
	rInfo.picInfo.orientation[6]=-1*sintheta*sinpsi;
	rInfo.picInfo.orientation[7]=sintheta*cospsi;
	rInfo.picInfo.orientation[8]=costheta;
	
	/* bundle these for convenience */
	rInfo.svinfo.nro=nro;
	rInfo.svinfo.slice_reps=slice_reps;
	rInfo.svinfo.nblocks=nblocks;
	rInfo.svinfo.ntraces=ntraces;
	rInfo.svinfo.dimafter=dimafter;
	rInfo.svinfo.dimfirst=dimfirst;
	rInfo.svinfo.multi_shot=multi_shot;
	rInfo.svinfo.multi_slice=multi_slice;
	rInfo.svinfo.etl=etl;
	rInfo.svinfo.slicesperblock=slicesperblock;
	rInfo.svinfo.viewsperblock=viewsperblock;
	rInfo.svinfo.within_slices=within_slices;
	rInfo.svinfo.within_views=within_views;
	rInfo.svinfo.phase_compressed=phase_compressed;
	rInfo.svinfo.slice_compressed=slice_compressed;
	rInfo.svinfo.views=views;
	rInfo.svinfo.slices=slices;
	rInfo.svinfo.echoes=echoes;
	rInfo.svinfo.epi_seq=epi_seq;
	rInfo.svinfo.flash_converted=flash_converted;
	rInfo.svinfo.pc_option=pc_option;
	rInfo.tbytes=fid_file_head->tbytes;
	rInfo.ebytes=fid_file_head->ebytes;
	rInfo.imglen=imglen;
	rInfo.ntlen=ntlen;
	rInfo.epi_rev=epi_rev;

      } /* end if doing setup */

  /* initialize  and malloc */
    npts=views*nro; /* number of pixels per slice */
    slice=0;
    view=0;
    echo=0;

 if(rInfo.do_setup)
   {
     realtime_block=0;
     rInfo.first_image=TRUE;
     rInfo.pc_slicecnt=0;
     rInfo.slicecnt=0;
     
     if(phase_compressed)
       {
	 dsize=slicesperblock*echoes*2*npts;
	 slicedata=(float *)allocateWithId(dsize*sizeof(float),"recon_all");
	 magnitude=(float *)allocateWithId(slicesperblock*echoes*npts*sizeof(float),"recon_all");
	 mag2=(float *)allocateWithId(slicesperblock*echoes*npts*sizeof(float),"recon_all");
       }
      else
	{      
	  dsize=slices*slice_reps*echoes*2*npts;
	  slicedata=(float *)allocateWithId(dsize*sizeof(float),"recon_all");
	  magnitude=(float *)allocateWithId(npts*sizeof(float),"recon_all");
	  mag2=(float *)allocateWithId(npts*sizeof(float),"recon_all");
	}
     if(pc_option!=OFF)
       {
	 pc=(float *)allocateWithId(slices*echoes*2*npts*sizeof(float),"recon_all");
	 pc_done=(int *)allocateWithId(slices*echoes*sizeof(int),"recon_all");
	 for(ipc=0;ipc<slices*echoes;ipc++)
	   *(pc_done+ipc)=FALSE;
       }
     if(multi_shot)
       sorted=(float *)allocateWithId(dsize*sizeof(float),"recon_all");
      
     /* zero everything important */
     for(i=0;i<dsize;i++)
       *(slicedata+i)=0.0;
     if(multi_shot)
       for(i=0;i<dsize;i++)
	 *(sorted+i)=0.0;
     if(pc_option!=OFF)
       for(i=0;i<slices*echoes*2*npts;i++)
	 *(pc+i)=0.0;
     
     repeat=(int *)allocateWithId(slices*views*echoes*sizeof(int),"recon_all");
     for(i=0;i<slices*echoes*views;i++)
	*(repeat+i)=-1;
   }
 
 
 /* turn off manual forced recon */
 root = getTreeRoot ( "current" );
 (void)RcreateVar ( "recon_force", root, T_REAL);
 error=P_getreal(CURRENT,"recon_force",&recon_force,1);
 if(!error)
   error=P_setreal(CURRENT,"recon_force",0.0,1);
 
 if(!rInfo.do_setup)
   {
     /* unpack the structure for convenience */
     pc_option=rInfo.svinfo.pc_option;
     slice_reps=rInfo.svinfo.slice_reps;
     nblocks=rInfo.svinfo.nblocks;
     ntraces=rInfo.svinfo.ntraces;
     dimafter=rInfo.svinfo.dimafter;
     dimfirst=rInfo.svinfo.dimfirst;
     multi_shot=rInfo.svinfo.multi_shot;
     multi_slice=rInfo.svinfo.multi_slice;
     etl=rInfo.svinfo.etl;
     slicesperblock=rInfo.svinfo.slicesperblock;
     viewsperblock=rInfo.svinfo.viewsperblock;
     within_slices=rInfo.svinfo.within_slices;
     within_views=rInfo.svinfo.within_views;
     phase_compressed=rInfo.svinfo.phase_compressed;
     slice_compressed=rInfo.svinfo.slice_compressed;
     views=rInfo.svinfo.views;
     slices=rInfo.svinfo.slices;
     echoes=rInfo.svinfo.echoes;
     epi_seq=rInfo.svinfo.epi_seq;
     flash_converted=rInfo.svinfo.flash_converted;
     imglen=rInfo.imglen;
     ntlen=rInfo.ntlen;
     epi_rev=rInfo.epi_rev;
   }

 if(realtime_block>=nblocks)
   {
     (void)recon_abort();
     ABORT;
   }

 within_nt=nblocks/ntlen;

 /* loop on  blocks  - initialize these */
 bhp = (dblockhead *)(rInfo.fidMd->offsetAddr);
 ctcount = (int) (bhp->ctcount);
 nt=nts[realtime_block/within_nt];

 while((!recon_done)&&(ctcount==nt))
   {
     if(interuption)
       {
	 Vperror("recon_all: aborting by request");	
	 (void)recon_abort();
	 ABORT;
       }

     blockctr=realtime_block++;
     if(realtime_block>=nblocks)
       recon_done=TRUE;       
     
     /* reset nt */
     nt=nts[realtime_block/within_nt];
     
     /* point to data for this block */
     inblock.head = bhp;
     rInfo.fidMd->offsetAddr += sizeof(dblockhead);  
     inblock.data = (float *)(rInfo.fidMd->offsetAddr);
     rInfo.fidMd->offsetAddr += (ntraces)*(rInfo.tbytes);
     
     /* point to header of next block and get new ctcount */
     bhp = (dblockhead *)(rInfo.fidMd->offsetAddr);
     ctcount = (int) (bhp->ctcount);
     
     if(epi_seq)
       {
	 icnt=(blockctr/dimafter)%(imglen)+1;
	 error=P_getreal(CURRENT,"image",&rimflag,icnt);
	 if(error)
	   {
	     Vperror("recon_all: Error getting image element");
	     (void)recon_abort();
	     ABORT;
	   }
	 imflag=(int)rimflag;
       }
     else
       imflag=TRUE;

      if((imflag)||(pc_option!=OFF))
      {
	  /* start of raw data */
	  sdataptr=NULL;
	  idataptr=NULL;
	  fdataptr=NULL;
	  if (rInfo.ebytes==2)
	    sdataptr=(short *)inblock.data;  
	  else if ((inblock.head->status & S_FLOAT) == 0)
	    idataptr=(int *)inblock.data;  
	  else
	    fdataptr=(float *)inblock.data;

	  for(itrc=0;itrc<ntraces;itrc++) 
	  {	
	    /* figure out where to put this echo */
	      (void)svcalc(itrc,blockctr,&(rInfo.svinfo),&view,&slice,&echo);

	      /* evaluate repetition number */
	      if(imflag)
		{
		  rep= *(repeat+slice*views*echoes+view*echoes+echo);
		  rep++;
		  rep=rep%slice_reps;
		  *(repeat+slice*views+view)=rep;
		  *(repeat+slice*views*echoes+view*echoes+echo)=rep;
		}
	      else
		rep=0;

	      if(imflag)
		  im_slice=slice;
	      else
		  pc_slice=slice;

		  
	      /* scale & convert the echo */
	      if(phase_compressed)
		  soffset=view*2*nro+echo*npts*2+(slice%slicesperblock)*echoes*npts*2;          
	      else
		  soffset=view*2*nro+echo*npts*2+rep*echoes*npts*2+slice*slice_reps*echoes*2*npts; 


	      /* scale and convert to float */
	      if(zeropad==8586)
		{
		  for(iro=0;iro<2*nro;iro++)
		    *(slicedata+soffset+iro)=0.0;
		}
	      if (sdataptr)
		for(iro=0;iro<2*(nro-zeropad);iro++)
		  *(slicedata+zeropad+soffset+iro)=IMAGE_SCALE*(*sdataptr++);
	      else if(idataptr)
		for(iro=0;iro<2*(nro-zeropad);iro++)
		  *(slicedata+zeropad+soffset+iro)=IMAGE_SCALE*(*idataptr++);
	      else
		for(iro=0;iro<2*(nro-zeropad);iro++)
		  *(slicedata+zeropad+soffset+iro)=IMAGE_SCALE*(*fdataptr++);

	      if(view%2 && epi_rev) /* time reverse data */
	      {
		  pt1=slicedata+soffset;
		  pt2=pt1+1;
		  pt3=pt1+2*(nro-1);
		  pt4=pt3+1;
		  for(iro=0;iro<nro/2;iro++)
		  {
		      t1=*pt1;
		      t2=*pt2;
		      *pt1=*pt3;
		      *pt2=*pt4;
		      *pt3=t1;
		      *pt4=t2;
		      pt1+=2;
		      pt2+=2;
		      pt3-=2;
		      pt4-=2;
		  }	
	      }
		  
	      if((recon_window>NOFILTER)&&(recon_window<=MAX_FILTER))
		{
		  pt1=slicedata+soffset;
		  dptr=window;
		  for(iro=0;iro<nro;iro++)
		    {
		      *pt1++=(float)*pt1 * (*dptr);
		      *pt1++=(float)*pt1 * (*dptr++);
		    }		  
		}

	      /* read direction ft */
	      nrptr=slicedata+soffset;
	      nrptr=nrptr-1;  /* four1 counts arrays as 1->n */
	      (void)four1(nrptr, (unsigned long)nro, 1);
	      
	      /* phase correction application and/or sort data*/
	      if(imflag&&(pc_option!=OFF||multi_shot))
	      {	
		  if(multi_shot)
		      sort_offset=soffset + (view_order[view] - view)*2*nro; 

		  if(pc_option!=OFF)
		  {
		      if(*(pc_done+im_slice))
		      {
			  pc_offset=im_slice*echoes*npts*2+echo*npts*2+view*nro*2;
			  pt1=slicedata+soffset;
			  pt2=pt1+1;
			  pt3=pc+pc_offset;
			  pt4=pt3+1;
			  pt5=pt1;
			  if(multi_shot)
			      pt5=sorted+sort_offset;
			  pt6=pt5+1;
			  for(iro=0;iro<nro;iro++)	
			  {
			      a=*pt1;	
			      b=*pt2;
			      c=*pt3;
			      d=*pt4;
			      *pt5=a*c-b*d;	
			      *pt6=a*d+b*c;
			      pt1+=2;
			      pt2+=2;
			      pt3+=2;
			      pt4+=2;
			      pt5+=2;
			      pt6+=2;
			  }
		      }
		  }	
		  else   /* only sort data, no phase correction */
		  {
		      pt1=slicedata+soffset;
		      pt2=pt1+1;
		      pt5=sorted+sort_offset;
		      pt6=pt5+1;
		      for(iro=0;iro<nro;iro++)
		      {
			  *pt5=*pt1;	
			  *pt6=*pt2;
			  pt1+=2;
			  pt2+=2;
			  pt5+=2;
			  pt6+=2;
		      }
		  }
	      }
	  }	/* end of loop on traces */

	  /* compute phase correction if not pointwise */
	  if((!imflag)&&(pc_option!=OFF))
	    {
	      if(phase_compressed)
		{
		  fptr=slicedata;
		  for (ispb=0;ispb<slicesperblock*echoes;ispb++)
		    {
		      *(pc_done+rInfo.pc_slicecnt)=TRUE;
		      pc_offset=rInfo.pc_slicecnt*npts*2;
		      (void)pc_calc(fptr, (pc+pc_offset),
				    nro, views, pc_option, transposed);
		      rInfo.pc_slicecnt++;
		      rInfo.pc_slicecnt=rInfo.pc_slicecnt%(slices*echoes);
		      fptr+=2*npts;
		    }
		}
	    }	
	  
	  /* do 2nd transform if possible */
	  if(phase_compressed&&imflag)
	    {
	      /* what channel is this? */
	      ichan= blockctr%rInfo.nchannels;
	      
	      magoffset=0;
	      fptr=slicedata;
	      if(multi_shot)
		fptr=sorted;

	      for (ispb=0;ispb<slicesperblock;ispb++)
		{
		  for(iecho=0;iecho<echoes;iecho++)
		    {
		      if(ichan)
			{
			  (void)phase_ft(fptr,nro,views,window,mag2+magoffset);
			  for(ipt=0;ipt<npts;ipt++)
			    magnitude[magoffset+ipt]+=mag2[magoffset+ipt];
			}
		      else
			(void)phase_ft(fptr,nro,views,window,magnitude+magoffset);
		      
		      /* write out images if on last channel */
		      if(ichan==(rInfo.nchannels-1))
			{
			  if(rInfo.nchannels>1)
			    {
			      for(ipt=0;ipt<npts;ipt++)
				magnitude[magoffset+ipt]/=(rInfo.nchannels);	
			    }
		

			  irep=(rep/(rInfo.nchannels))+1;
			  rInfo.picInfo.echo=iecho+1;
			  rInfo.picInfo.slice=rInfo.slicecnt+1;
			  /* 		      irep=rInfo.picInfo.echo + (irep-1)*echoes; */

			  (void)write_fdf(irep,magnitude+magoffset, &(rInfo.picInfo),
					  &(rInfo.first_image),rInfo.dispflag);
			}

		      fptr+=2*npts;
		      magoffset+=npts;
		    }   /* end of echo loop */
		  
		  rInfo.slicecnt++;
		  rInfo.slicecnt=rInfo.slicecnt%slices;
		}      /* end of slice loop */
	    }
      }            /* end if imflag or doing phase correction */

      /* see if recon has been forced */
      if(!recon_forced)
	{
	  error=P_getreal(CURRENT,"recon_force",&recon_force,1);
	  if(!error)
	    {
	      if(recon_force>0.0)
		{
		  recon_forced=TRUE;
		  if(!phase_compressed)
		    (void)generate_images(slices, views, nro, slice_reps, rInfo.nchannels,
					  window, &(rInfo.picInfo), rInfo.first_image, multi_shot, rInfo.dispflag);		
		}
	    }
	}

  }   	/* end loop on blocks */
 
 if(!phase_compressed)     /* process data for each slice now */
   (void)generate_images(slices, views, nro, slice_reps, rInfo.nchannels, window, &(rInfo.picInfo), 
			 rInfo.first_image, multi_shot, rInfo.dispflag);

  if(recon_done)
    {
      (void)releaseAllWithId("recon_all");
      (void)free(&realtime_block);
      if(rInfo.fidMd)
	{
	  mClose(rInfo.fidMd);
	  rInfo.fidMd=NULL;
	}
    }

  return(0);
}



/******************************************************************************/
/******************************************************************************/
static generate_images(slices, views, nro, slice_reps, nchannels, window, pInfo, first_image, multi_shot, dispflag)
     int slices, views, nro, slice_reps, nchannels, first_image;
     int  multi_shot, dispflag;
     fdfInfo *pInfo;
     double *window;
{
  float *fptr;
  int slice, rep, ipt;
  int ichan;
  int npts=views*nro;
  int echo;

  fptr=slicedata;
  if(multi_shot)    
    fptr=sorted;
  for(slice=0;slice<slices;slice++)
    {
      for(rep=0;rep<(slice_reps/nchannels);rep++)
	{
	  for(echo=0;echo<pInfo->echoes;echo++)
	    {
	      for(ichan=0;ichan<nchannels;ichan++)
		{
		  if(ichan)
		    {
		      (void)phase_ft(fptr,nro,views,window,mag2);
		      for(ipt=0;ipt<npts;ipt++)
			magnitude[ipt]+=mag2[ipt];
		    }
		  else
		    (void)phase_ft(fptr,nro,views,window,magnitude);
		  fptr+=2*npts;
		}
	      pInfo->echo=echo+1;
	      pInfo->slice=slice+1;
	      if(nchannels>1)
		{
		  for(ipt=0;ipt<npts;ipt++)
		    magnitude[ipt]/=nchannels;		
		}
	      (void)write_fdf(rep+1,magnitude, pInfo, &first_image, dispflag);
	  }
	}
    }
  return;
}


/******************************************************************************/
/******************************************************************************/
 static phase_ft(xkydata,nx,ny,win,absdata)
 float *xkydata;
 float *absdata;
 double *win;
 int nx,ny;
 {
     float a,b;
     float *fptr,*pt1,*pt2;
     int ix,iy;
     int i,np;
     int offset;
     float *nrptr;
     float templine[2*MAXPE];
     fptr=absdata;

     np=nx*ny;
     if(win)
       {
	 offset=0.5*(nx-ny);
	 win+=offset;
       }
     /* do phase direction ft */
     for(ix=0;ix<nx;ix++)
     {
	 /* get the ky line */
	 pt1=xkydata+2*ix;
	 pt2=pt1+1;
	 nrptr=templine;
	 for(iy=0;iy<ny;iy++)
	 {
	     *nrptr++=*pt1;
	     *nrptr++=*pt2;
	     pt1+=2*nx;	
	     pt2=pt1+1;
	 }
	 if(win)
	   {
	     for(iy=0;iy<ny;iy++)
	       {
		 templine[2*iy]*= *(win+iy);
		 templine[2*iy+1]*= *(win+iy);
	       }
	   }
	 nrptr=templine;
	 nrptr=nrptr-1;
	 (void)four1(nrptr, (unsigned long)ny, 1); 
	 /* write it back */
	 pt1=xkydata+2*ix;
	 pt2=pt1+1;
	 nrptr=templine;
	 for(iy=0;iy<ny;iy++)
	 {
	     *pt1=*nrptr++;
	     *pt2=*nrptr++;
	     a=*pt1;
	     b=*pt2;
	     *fptr++=(float)sqrt((double)(a*a+b*b));
	     pt1+=2*nx;	
	     pt2=pt1+1;
	 }
     }
     return;
 }

/******************************************************************************/
/******************************************************************************/
static filter_window(window_type,window, npts)
     int window_type;
     double *window;
     int npts;
{
  double f, alpha;
  int i;
  switch(window_type)
    {
    case NOFILTER:
      return;
    case BLACKMANN:
      {
	for(i=0;i<npts;i++)
	  window[i]=0.42-0.5*cos(2*M_PI*(i-1)/(npts-1))+0.08*cos(4*M_PI*(i-1)/(npts-1)); 
	return;
      }
    case HANN:
      {
	for(i=0;i<npts;i++)
	  window[i]=0.5*(1-cos(2*M_PI*(i-1)/(npts-1)));
	return;
      }
    case HAMMING:
      {
	for(i=0;i<npts;i++)
	  window[i]=0.54-0.46*cos(2*M_PI*(i-1)/(npts-1));
	return;
      }
    case GAUSSIAN:
      {
	alpha=2.5;
	f=(-2*alpha*alpha)/(npts*npts);
	for(i=0;i<npts;i++)
	  window[i]=exp(f*(i-1-npts/2)*(i-1-npts/2));
	return;
      }
    default:
      return;
    }
 }

/******************************************************************************/
/******************************************************************************/
/* returns slice and view number given trace and block number */
int svcalc(trace,block,svI,view,slice, echo)
int trace, block;
svInfo *svI; 
int *view;
int *slice;
int *echo;
{
  int v,s;

  if(svI->multi_shot||svI->epi_seq)
    {
      if(svI->phase_compressed)
	{
	  if(svI->multi_slice)
	    {
	      v=trace/((svI->etl)*(svI->slicesperblock)*(svI->echoes));  /* segment */
	      v*=svI->etl;                                                      /* times etl */
	      v+=(trace%(svI->etl));                                  /* plus echo  */
	    }
	  else
	    v=trace;
	}
      else
	v=(block/svI->within_views);
      if(svI->slice_compressed)
	{
	  if(svI->multi_slice)
	    s=trace/(svI->etl);
	  else  
	    s=(trace/svI->viewsperblock); 
	}
      else
	s=(block/svI->within_slices);
    }
  else   /* not multi_shot */
    {
      if(svI->phase_compressed)
	{
	  if(svI->multi_slice)
	    v=trace/(svI->slicesperblock)*(svI->echoes);
	  else
	    v=trace/(svI->echoes);
	}
      else
	v=(block/svI->within_views);
      if(svI->slice_compressed)
	{
	  if(svI->multi_slice)
	    s=trace/(svI->echoes);
	  else  
	    s=trace/(svI->viewsperblock)*(svI->echoes); 
	}
      else
	s=(block/svI->within_slices);
    }
  
  *view=v%(svI->views);
  *slice=s%(svI->slices);
  if(svI->epi_seq)
    {
      *echo=trace/((svI->etl)*(svI->slicesperblock));
      *echo=(*echo)%(svI->echoes);
    }
  else
    *echo=trace%(svI->echoes);


  return;
}

/******************************************************************************/
/******************************************************************************/
void recon_abort()
{
  if(rInfo.fidMd)
    {
      mClose(rInfo.fidMd);
      rInfo.fidMd=NULL;
    }
  (void)releaseAllWithId("recon_all");
  (void)free(&realtime_block);

  return;
}
   
/******************************************************************************/
/******************************************************************************/
int write_fdf(imageno,datap,fI,first_imageP, display)
int imageno;
fdfInfo *fI; 
float *datap;
int *first_imageP;
int display;
{
  MFILE_ID  fdfMd;
  char filename[MAXPATHL];
  char dirname[MAXPATHL];
  char str[100];
  char hdr[2000];
  char quote = '"';
  FILE *f1;
  double rppe, rpro, rpss;
  int i,error;
  int pad_cnt, align;
  int hdrlen;
  int filesize;
  
  if(fI->fullpath)
    (void)strcpy(dirname,fI->imdir);
  else
    {
      (void)strcpy(dirname,curexpdir);
      (void)strcat(dirname,"/");
      (void)strcat(dirname,fI->imdir);
    }
  (void)strcpy(filename,dirname);
  
  if(*first_imageP)
    {
      (void)sprintf(str,"rm %s/*.fdf \n",dirname);
      (void)system(str);	
    }
  
  (void)sprintf(str,"/slice%03dimage%03decho%03d",fI->slice,imageno,fI->echo);
  (void)strcat(filename,str);
  (void)strcat(filename,".fdf");
  
  
  error=P_getreal(CURRENT,"pss",&rpss,fI->slice);
  if(error)
    {
      Vperror("recon_all: write_fdf: Error getting slice offset");
      (void)recon_abort();
      ABORT;
    }
  error=P_getreal(CURRENT,"ppe",&rppe,1);
  if(error)
    {
      Vperror("recon_all: write_fdf: Error getting phase encode offset");
      (void)recon_abort();
      ABORT;
    }
  error=P_getreal(CURRENT,"pro",&rpro,1);
  if(error)
    {
      Vperror("recon_all: write_fdf: Error getting phase encode offset");
      (void)recon_abort();
      ABORT;
    }

  (void)sprintf(hdr,"#!/usr/local/fdf/startup\n");
  (void)strcat(hdr,"float  rank = 2;\n");
  (void)strcat(hdr,"char  *spatial_rank = \"2dfov\";\n");
  (void)strcat(hdr,"char  *storage = \"float\";\n");
  (void)strcat(hdr,"float  bits = 32;\n");
  (void)strcat(hdr,"char  *type = \"absval\";\n");
  (void)sprintf(str,"float  matrix[] = {%d, %d};\n",fI->npe,fI->nro);
  (void)strcat(hdr,str);
  (void)strcat(hdr,"char  *abscissa[] = {\"cm\", \"cm\"};\n");
  (void)strcat(hdr,"char  *ordinate[] = { \"intensity\" };\n");
  (void)sprintf(str,"float  span[] = {%.6f, %.6f};\n",fI->fovpe,fI->fovro);
  (void)strcat(hdr,str);
  (void)sprintf(str,"float  origin[] = {%.6f,%.6f};\n",rppe,rpro);
  (void)strcat(hdr,str);
  (void)strcat(hdr,"char  *nucleus[] = {\"H1\",\"H1\"};\n");
  (void)sprintf(str,"float  nucfreq[] = {%.6f,%.6f};\n",fI->sfrq,fI->dfrq);
  (void)strcat(hdr,str);
  (void)sprintf(str,"float  location[] = {%.6f,%.6f,%.6f};\n",rppe,rpro,rpss);
  (void)strcat(hdr,str);
  (void)sprintf(str,"float  roi[] = {%.6f,%.6f,%.6f};\n",fI->fovpe,fI->fovro,fI->thickness);
  (void)strcat(hdr,str);
  (void)sprintf(str,"char  *file = \"%s\";\n",fI->fidname);
  (void)strcat(hdr,str);
  (void)sprintf(str,"int    slice_no = %d;\n",fI->slice);
  (void)strcat(hdr,str);
  (void)sprintf(str,"int    slices = %d;\n",fI->slices);
  (void)strcat(hdr,str);
  (void)sprintf(str,"int    echo_no = %d;\n",fI->echo);
  (void)strcat(hdr,str);
  (void)sprintf(str,"int    echoes = %d;\n",fI->echoes);
  (void)strcat(hdr,str);
  (void)sprintf(str,"float  TE = %.3f;\n",fI->te);
  (void)strcat(hdr,str);
  (void)sprintf(str,"float  te = %.6f;\n",MSEC_TO_SEC*fI->te);
  (void)strcat(hdr,str);
  (void)sprintf(str,"float  TR = %.3f;\n",fI->tr);
  (void)strcat(hdr,str);
  (void)sprintf(str,"float  tr = %.6f;\n",MSEC_TO_SEC*fI->tr);
  (void)strcat(hdr,str);
  (void)sprintf(str,"int ro_size = %d;\n",fI->ro_size);
  (void)strcat(hdr,str);
  (void)sprintf(str,"int pe_size = %d;\n",fI->pe_size);
  (void)strcat(hdr,str);
  (void)sprintf(str,"char *sequence = \"%s\";\n",fI->seqname);
  (void)strcat(hdr,str);
  (void)sprintf(str,"float  TI =  %.3f;\n",fI->ti);
  (void)strcat(hdr,str);
  (void)sprintf(str,"float  ti =  %.6f;\n",MSEC_TO_SEC*fI->ti);
  (void)strcat(hdr,str);
  (void)sprintf(str,"int    array_index = %d;\n",fI->array_index);
  (void)strcat(hdr,str);
  (void)sprintf(str,"float  image = %.4f;\n",fI->image);
  (void)strcat(hdr,str);
  (void)sprintf(str,
		"float  orientation[] = {%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f};\n",
		fI->orientation[0],fI->orientation[1],fI->orientation[2],
		fI->orientation[3],fI->orientation[4],fI->orientation[5],
		fI->orientation[6],fI->orientation[7],fI->orientation[8]);
  (void)strcat(hdr,str);
  (void)strcat(hdr,"int checksum = 1291708713;\n");
  (void)strcat(hdr,"\f\n");
  
  align = sizeof(float);
  hdrlen=strlen(hdr);
  hdrlen++;   /* include NULL terminator */
  pad_cnt = hdrlen % align;
  pad_cnt = (align - pad_cnt) % align;
  
  /* Put in padding */
  for(i=0;i<pad_cnt;i++)
    {
      (void)strcat(hdr,"\n");
      hdrlen++;
    }
  *(hdr + hdrlen) = '\0';
    
  filesize=hdrlen*sizeof(char) +  fI->nro*fI->npe*sizeof(float);
  fdfMd=mOpen(filename,filesize,O_RDWR | O_CREAT);
  (void)mAdvise(fdfMd, MF_SEQUENTIAL);
  if(!fdfMd)
    {
      /* create the output directory and try again */
      (void)sprintf(str,"mkdir %s \n",dirname);
      (void)system(str);
      fdfMd=mOpen(filename,filesize,O_RDWR | O_CREAT);
      (void)mAdvise(fdfMd, MF_SEQUENTIAL);
      if(!fdfMd)
	{
	  Vperror("recon_all: write_fdf: Error opening image file");
	  (void)recon_abort();
	  ABORT;
	}
    }
  
  if(*first_imageP)
    {
      (void)sprintf(str,"cp %s/procpar %s \n",curexpdir,dirname);
      (void)system(str);
      *first_imageP=FALSE;
    }
  
  (void)memcpy(fdfMd->offsetAddr,hdr,hdrlen*sizeof(char));
  fdfMd->newByteLen += hdrlen*sizeof(char);
  fdfMd->offsetAddr += hdrlen*sizeof(char);
  (void)memcpy(fdfMd->offsetAddr,datap,fI->nro*fI->npe*sizeof(float));
  fdfMd->newByteLen += fI->nro*fI->npe*sizeof(float);
  /* 	(void)mClose(fdfMd);  */
  
  
#ifdef VNMRJ	
  if(display)
    (void)aipDisplayFile(filename, -1);
#endif
  
  
  return;
}
	

