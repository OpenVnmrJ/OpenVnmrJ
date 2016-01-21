/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*****************************************************************************
* File phase_fid.c:  
*
* finds phase at peak of each line of input file
* creates output "fid" format file containing phases  
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
#include <stdlib.h>
#include "buttons.h"
#include "data.h"
#include "graphics.h"
#include "group.h"
#include "init2d.h"
#include "mfileObj.h"
#include "symtab.h"
#include "vnmrsys.h"
#include "variables.h"
#include "process.h"
#include "epi_recon.h"
#include "phase_correct.h"
#include "mfileObj.h"
#include "allocate.h"
#include "pvars.h"
#include "variables.h"
#include "wjunk.h"


#ifdef VNMRJ
#include "aipCInterface.h"
#endif 

extern int interuption;
extern void Vperror(char *);
extern int specIndex;

static float *polardata;
static float *cartdata;
static float *rawdata;
static int *nts;
static int iphs;
static reconInfo rInfo; 
static int realtime_block;

/* prototype */
void phase_fidabort();

/*******************************************************************************/
/*******************************************************************************/
/*           phase_fid                                                                        */
/*******************************************************************************/
/*******************************************************************************/
int phase_fid (int argc, char *argv[], int retc, char *retv[] )
/* 

Arguments:
---------
argc  :  (   )  Argument count.
argv  :  (   )  Command line arguments.
retc  :  (   )  Return argument count.  Not used here.
retv  :  (   )  Return arguments.  Not used here

*/

/*ARGSUSED*/

{
/*   Local Variables: */
    FILE *f1;

    char filepath[MAXPATHL];
    char outfilepath[MAXPATHL];
    char sequence[MAXSTR];
    char studyid[MAXSTR];
    char rcvrs_str[MAXSTR];
    char apptype[MAXSTR];
    char dcrmv[SHORTSTR];
    char str[MAXSTR];
    short  *sdataptr;
    vInfo info;

    double dtemp;
    double mag;
    double maxmag;
    double acqtime;
    double amax=0.0, bmax=0.0;

    dpointers inblock;
    dblockhead *bhp;
    dblockhead *bhpp;
    dfilehead *fid_file_head = NULL;
    dfilehead *phs_file_head;

    float *fptr;
    float *datptr;
    float *fdataptr;
    float real_dc = 0.0,imag_dc = 0.0;
    float a,b;

    int *idataptr;
    int itrc;
    int blockctr;
    int iro;
    int ntlen;
    int nt;
    int nro;
    int within_nt;
    int nblocks,ntraces;
    int error;
    int nchannels=1;
    int maxpt;
    int i;
    int narg;
    int zeropad=0;
    int ctcount;
 
    /* flags */
    int acq_done=FALSE;

    /*********************/
    /* executable code */
    /*********************/
    
    /* default :  do complete setup and mallocs */
    rInfo.do_setup=TRUE;
    
    /* look for command line arguments */
    narg=1;
    if (argc>narg)  /* first argument is acq or a dummy string*/
      {
	if(strcmp(argv[narg],"acq")==0)  /* not first time through!!! */
	  rInfo.do_setup=FALSE;         
	narg++;
      }
    if(rInfo.do_setup)
      rInfo.fidMd=NULL;

    if(interuption)
      {
	error=P_setstring(CURRENT, "wnt", "", 0);  
	error=P_setstring(PROCESSED, "wnt", "", 0);  
	Werrprintf("phase_fid: aborting by request");	
	(void)phase_fidabort();
	ABORT;
      }

    /* what sequence is this ? */
    error=P_getstring(PROCESSED, "seqfil", sequence, 1, MAXSTR);  
    if(error)
      {
	Werrprintf("phase_fid: Error getting seqfil");	
	(void)phase_fidabort();
	ABORT;
      }		

    /* get studyid */
    error=P_getstring(PROCESSED, "studyid_", studyid, 1, MAXSTR);  
    if(error)
      (void)strcpy(studyid,"unknown");

    /* get apptype */
    error=P_getstring(PROCESSED, "apptype", apptype, 1, MAXSTR);  
    /*
    if(error)
      {
	Werrprintf("phase_fid: Error getting apptype");	
	(void)phase_fidabort();
	ABORT;
      }		
    if(!strlen(apptype))
      {
	Winfoprintf("phase_fid: WARNING:apptype unknown!");	
	Winfoprintf("phase_fid: Set apptype in processed tree");	
      }
    */

    
    /* always write to recon */
    (void)strcpy(rInfo.picInfo.imdir,"recon");
    rInfo.picInfo.fullpath=FALSE;


    /******************************/
    /* setup                      */
    /******************************/
    if(rInfo.do_setup)
      {
#ifdef VNMRJ
	if(!aipOwnsScreen()) {
	  setdisplay();
	  Wturnoff_buttons();
	  sunGraphClear();
	  aipDeleteFrames();
	}
#else
	setdisplay();
	Wturnoff_buttons();
	sunGraphClear();
#endif
	(void)init2d_getfidparms(FALSE);

	
	/* display images or not? */
	rInfo.dispint=1;
	error=P_getreal(PROCESSED,"recondisplay",&dtemp,1);
	if(!error)
	  rInfo.dispint=(int)dtemp;
	
	/* open the fid file */
	(void)strcpy ( filepath, curexpdir );
	(void)strcat ( filepath, "/acqfil/fid" );  
	(void)strcpy(rInfo.picInfo.fidname,filepath);
	rInfo.fidMd=mOpen(filepath,0,O_RDONLY);
	(void)mAdvise(rInfo.fidMd, MF_SEQUENTIAL);
	
	/* how many receivers? */
	error=P_getstring(PROCESSED, "rcvrs", rcvrs_str, 1, MAXSTR);  
	if(!error)
	  {
	    nchannels=strlen(rcvrs_str);
	    for(i=0;i<strlen(rcvrs_str);i++)
	      if(*(rcvrs_str+i)!='y')
		nchannels--;
	  }
	rInfo.nchannels=nchannels;

	fid_file_head = (dfilehead *)(rInfo.fidMd->offsetAddr);
	ntraces=fid_file_head->ntraces;
	nblocks=fid_file_head->nblocks;
	nro=fid_file_head->np/2;
	rInfo.fidMd->offsetAddr+= sizeof(dfilehead);
	
	if(nro<MINRO)
	  {
	    Werrprintf("phase_fid:  np too small");
	    (void)phase_fidabort();
	    ABORT;
	  }

	/* get nt  array  */
	error=P_getVarInfo(PROCESSED,"nt",&info); 
	if(error)
	  {
	    Werrprintf("phase_fid: Error getting nt info");
	    (void)phase_fidabort();
	    ABORT;
	  }
	ntlen=info.size;	    /* read nt values*/
	nts=(int *)allocateWithId(ntlen*sizeof(int),"phase_fid");
	for(i=0;i<ntlen;i++)
	  {
	    error=P_getreal(PROCESSED,"nt",&dtemp,(i+1));
	    if(error)
	      {
		Werrprintf("phase_fid: Error getting nt element");
		(void)phase_fidabort();
		ABORT;
	      }
	    nts[i]=(int)dtemp;
	  }

	/* dc correct or not? */
	rInfo.dc_flag=FALSE;
	error=P_getstring(CURRENT, "dcrmv", dcrmv, 1, SHORTSTR);
	if(!error)
	  {
	    if ((dcrmv[0] == 'y') &&(nts[0]==1))
	      rInfo.dc_flag = TRUE;
	  }

	/* estimate time for block acquisition */
	error = P_getreal(PROCESSED,"at",&acqtime,1);
	if(error)
	  {
	    Werrprintf("phase_fid: Error getting at");
	    (void)phase_fidabort();
	    ABORT;
	  }

	realtime_block=0;
	rInfo.image_order=0;
	rInfo.pc_slicecnt=0;
	rInfo.slicecnt=0;
	rInfo.dispcnt=0;
	iphs=0;
	
	polardata=(float *)allocateWithId(2*nblocks*ntraces*sizeof(float),"phase_fid");
	cartdata=(float *)allocateWithId(2*nblocks*ntraces*sizeof(float),"phase_fid");

	rawdata=(float *)allocateWithId(2*nro*sizeof(float),"phase_fid");
	      
	/* bundle these for convenience */
	rInfo.picInfo.nro=nro;
	rInfo.svinfo.nblocks=nblocks;
	rInfo.svinfo.ntraces=ntraces;
	rInfo.tbytes=fid_file_head->tbytes;
	rInfo.ebytes=fid_file_head->ebytes;
	rInfo.bbytes=fid_file_head->bbytes;
	rInfo.ntlen=ntlen;
	rInfo.zeropad = zeropad;

	/* zero everything important */
	for(i=0;i<2*ntraces*nblocks;i++)
	  {
	    *(polardata+i)=0.0;
	    *(cartdata+i)=0.0;
	  }
	for(i=0;i<2*nro;i++)
	  *(rawdata+i)=0.0;
      }
    /******************************/
    /* end setup                    */
    /******************************/

    if(!rInfo.do_setup)
      {
	/* unpack the structure for convenience */
	nro=rInfo.picInfo.nro;
	nblocks=rInfo.svinfo.nblocks;
	ntraces=rInfo.svinfo.ntraces;
	ntlen=rInfo.ntlen;
	zeropad=rInfo.zeropad;
      }

    if(realtime_block>=nblocks)
      {
	(void)phase_fidabort();
	ABORT;
      }

    if(ntlen)
      within_nt=nblocks/ntlen;
    else
      {
	Werrprintf("phase_fid: ntlen equal to zero");
	(void)phase_fidabort();
	ABORT;
      }			

    if(mFidSeek(rInfo.fidMd, (realtime_block+1), sizeof(dfilehead), rInfo.bbytes))
      {
	(void)sprintf(str,"phase_fid: mFidSeek error for block %d\n",realtime_block);
	Werrprintf(str);
	(void)phase_fidabort();
	ABORT;
      }
    bhp = (dblockhead *)(rInfo.fidMd->offsetAddr);
    ctcount = (int) (bhp->ctcount);
    if(within_nt)
      nt=nts[realtime_block/within_nt];
    else
      {
	Werrprintf("phase_fid: within_nt equal to zero");
	(void)phase_fidabort();
	ABORT;
      }

			    
    /*********************/
    /* loop on blocks    */
    /*********************/
    while((!acq_done)&&(ctcount==nt))
      {
	if(interuption)
	  {
	    error=P_setstring(CURRENT, "wnt", "", 0);  
	    error=P_setstring(PROCESSED, "wnt", "", 0);  
	    Werrprintf("phase_fid: aborting by request");	
	    (void)phase_fidabort();
	    ABORT;
	  }

	blockctr=realtime_block++;
	if(realtime_block>=nblocks)
	  acq_done=TRUE;       
	
	/* reset nt */
	if(within_nt)
	  nt=nts[realtime_block/within_nt];
	else
	  {
	    Werrprintf("phase_fid: within_nt equal to zero");
	    (void)phase_fidabort();
	    ABORT;
	  }

	/* point to data for this block */
	inblock.head = bhp;
	rInfo.fidMd->offsetAddr += sizeof(dblockhead);  
	inblock.data = (float *)(rInfo.fidMd->offsetAddr);

	/* get dc offsets if necessary */
	if(rInfo.dc_flag)
	  {
	    real_dc=IMAGE_SCALE*(bhp->lvl);
	    imag_dc=IMAGE_SCALE*(bhp->tlt);
	  }

	/* point to header of next block and get new ctcount */
	if(!acq_done)
	  {
	    if(mFidSeek(rInfo.fidMd, (realtime_block+1), sizeof(dfilehead), rInfo.bbytes))
	      {
		(void)sprintf(str,"phase_fid: mFidSeek error for block %d\n",realtime_block);
		Werrprintf(str);
		(void)phase_fidabort();
		ABORT;
	      }
	    bhp = (dblockhead *)(rInfo.fidMd->offsetAddr);
	    ctcount = (int) (bhp->ctcount);
	  }

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
	
	/* be on the safe side */
	for(i=0;i<2*nro;i++)
	  *(rawdata+i)=0.0;	
	
	datptr=rawdata;
	for(itrc=0;itrc<ntraces;itrc++) 
	  {	
	    /* scale and convert to float */
	    if (sdataptr)
	      for(iro=0;iro<2*nro;iro++)
		*(datptr+iro)=IMAGE_SCALE*(*sdataptr++);
	    else if(idataptr)
	      for(iro=0;iro<2*nro;iro++)
		*(datptr+iro)=IMAGE_SCALE*(*idataptr++);
	    else
	      for(iro=0;iro<2*nro;iro++)
		*(datptr+iro)=IMAGE_SCALE*(*fdataptr++);
	    
	    if(rInfo.dc_flag)
	      {
		for(iro=0;iro<2*nro;iro++)
		  {
		    if(iro%2)
		      *(datptr+iro) -= imag_dc;
		    else
		      *(datptr+iro) -= real_dc;
		  }
	      }
	    /* find max signal and its phase */
	    maxmag=0.0;
	    fptr=rawdata;
	    for(iro=0;iro<nro;iro++)
	      {
		a=*(fptr++);
		b=*(fptr++);
		mag=sqrt((double)(a*a+b*b));
		if(mag>=maxmag)
		  {
		    maxmag=mag;
		    maxpt=iro;
		    amax=(double)a;
		    bmax=(double)b;
		  }
	      }
	    polardata[iphs]=(float)maxmag;
	    cartdata[iphs++]=(float)amax;
	    polardata[iphs]=(float)(atan2(bmax,amax));
	    cartdata[iphs++]=(float)bmax;
	  }	/* end of loop on traces */
      }   	  
    /**************************/
    /* end loop on blocks    */
    /**************************/
 
    if(acq_done)
      {
	/* create pseudo-fid file containing phases */
	bhpp=(dblockhead *)allocateWithId(sizeof(dblockhead),"phase_fid");
	phs_file_head=(dfilehead *)allocateWithId(sizeof(dfilehead),"phase_fid");
	(void)strcpy (outfilepath, curexpdir );
	(void)strcat (outfilepath, "/acqfil/pseudofid_polar" );  
	f1=fopen(outfilepath, "w+");
	if(f1==NULL)
	  {
	    (void)sprintf(str,"phase_fid: cannot create file %s\n",outfilepath);
	    Werrprintf(str);
	    (void)phase_fidabort();
	    ABORT;
	  }
	phs_file_head->nblocks=1;
	phs_file_head->ntraces=1;
	phs_file_head->np=ntraces*nblocks;
	phs_file_head->ebytes=4;
	phs_file_head->tbytes=(phs_file_head->np)*(phs_file_head->ebytes);
	phs_file_head->bbytes=(phs_file_head->tbytes)*(phs_file_head->ntraces);
	phs_file_head->vers_id=fid_file_head->vers_id;
	phs_file_head->status=S_FLOAT&S_COMPLEX&S_DATA&S_32&S_NP;
	phs_file_head->nbheaders=phs_file_head->nblocks;
	(void)fwrite(phs_file_head, sizeof(dfilehead),1,f1);

	bhpp->scale=1.0;
	bhpp->status=S_FLOAT&S_COMPLEX&S_DATA&S_32&NP_CMPLX;
	bhpp->index=1;
	bhpp->mode=NP_DSPLY;
	bhpp->ctcount=1;
	bhpp->lpval=0.0;
	bhpp->rpval=0.0;
	bhpp->lvl=0.0;
	bhpp->tlt=0.0;
	(void)fwrite(bhpp, sizeof(dblockhead),1,f1);

	(void)fwrite(polardata, sizeof(float),2*nblocks*ntraces,f1);
	(void)fclose(f1);

	/* create pseudo-fid file containing magnitudes */
	(void)strcpy (outfilepath, curexpdir );
	(void)strcat (outfilepath, "/acqfil/pseudofid_cart" );  
	f1=fopen(outfilepath, "w+");
	if(f1==NULL)
	  {
	    (void)sprintf(str,"phase_fid: cannot create file %s\n",outfilepath);
	    Werrprintf(str);
	    (void)phase_fidabort();
	    ABORT;
	  }
	(void)fwrite(phs_file_head, sizeof(dfilehead),1,f1);
	(void)fwrite(bhpp, sizeof(dblockhead),1,f1);
	(void)fwrite(cartdata, sizeof(float),2*nblocks*ntraces,f1);
	(void)fclose(f1);

	(void)releaseAllWithId("phase_fid");
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
void phase_fidabort()
{
  if(rInfo.fidMd)
    {
      mClose(rInfo.fidMd);
      rInfo.fidMd=NULL;
    }
  (void)releaseAllWithId("phase_fid");
  return;
}
   
