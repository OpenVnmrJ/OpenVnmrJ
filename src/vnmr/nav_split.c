/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*****************************************************************************
* File nav_split.c:  
*
* splits fid file into imaging and navigator datal 4-4-2003 M. Kritzer
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
extern double getfpmult();

static   MFILE_ID fidMd;
static   MFILE_ID newfidMd;
static   MFILE_ID navMd;

/* prototype */
void nav_abort();
int svcalc( int, int, svInfo *,  int *, int *, int *);

/*******************************************************************************/
/*******************************************************************************/
/*            NAV_SPLIT                                                    */
/*******************************************************************************/
/*******************************************************************************/
nav_split ( argc, argv, retc, retv )

/* 

Purpose:
-------
Routine nav_split separates fid data into imaging and navigator data

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
    
    
    svInfo svinfo;
    
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
    char dcrmv[SHORTSTR];
    char nav_str[SHORTSTR];
    char arraystr[MAXSTR];
    char *ptr;
    char *imptr;
    char str[MAXSTR];
    short  *sdataptr;
    vInfo info;


    double rnseg, rfc, retl, rnv;
    double rcelem;
    double rimflag;
    double ncomp,rechoes, nf, ni, repi_rev;
    double rslices,rnf;
    double *dptr;
    double dtemp;
    double acqtime;
    dpointers inblock;
    dblockhead *bhp;
    dfilehead *fid_file_head;
    dfilehead *new_fid_head;
    dfilehead *nav_head;
    float *fptr, *nrptr;
    float *wptr;
    float *fdataptr;
    float a,b,c,d;
    float t1, t2, *pt1, *pt2, *pt3, *pt4, *pt5, *pt6;
    int echoes;
    int iecho;
    int tstch;
    int *idataptr;
    int *nav_list;
    int iblock;
    int imglen,dimfirst,dimafter,imfound;
    int imzero;
    int icnt;
    int nv;
    int ispb, itrc;
    int blockctr;
    int iro;
    int ipc;
    int dsize;
    int views, nro;
    int ro_size, pe_size;
    int slices, etl, nblocks,ntraces, slice_reps;
    int new_etl;
    int new_ntraces;
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
    int i, min_view, iview;
    int narg;
    int n_ft;
    int nshots;
    int nav_first, nnav, nav_pts;
    int nav_traces;
    int navflag;
    short index;
    long tbytes;
    long filesize;

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
    int acq_done=FALSE;
    int slice_compressed=TRUE; 
    int phase_compressed=TRUE; /* a good assumption  */
    int flash_converted=FALSE;
    int epi_me=FALSE;
    int phase_reverse=FALSE;

    symbol **root;

    /*********************/
    /* executable code */
    /*********************/

    /* default :  do complete setup and mallocs */
    
    if(interuption)
      {
	Vperror("nav_split: aborting by request");	
	(void)nav_abort();
	ABORT;
      }
    
    /* what sequence is this ? */
    error=P_getstring(PROCESSED, "seqfil", sequence, 1, MAXSTR);  
    if(error)
      {
	Vperror("nav_split: Error getting seqfil");	
	(void)nav_abort();
	ABORT;
      }		
    if(strstr(sequence,"epi"))
      epi_seq=TRUE;

    /* defaults */
    nnav=0;
    nav_first=0;
    /* look for command line arguments */
    narg=1;
    if(argc>narg)
      {
	(void)sscanf(argv[narg++],"%d",&nnav);
	if(argc>narg)
	  {
	    (void)sscanf(argv[narg++],"%d",&nav_first);
	    if(argc>narg)
	      {
		(void)sscanf(argv[narg++],"%d",&nav_pts);
	      }
	  }
      }

      if(strstr(sequence,"ms"))
      multi_slice=TRUE;
    
    if(strstr(sequence,"epidw"))
      {
	error=P_getstring(PROCESSED, "ms_intlv", msintlv, 1, MAXSTR);  
	if(error)
	  {
	    Vperror("nav_split: Error getting ms_intlv");	
	    (void)nav_abort();
	    ABORT;
	  }
	if(msintlv[0]=='y')
	  multi_slice=TRUE;
      }	

    
    /* open the fid file */
    (void)strcpy ( filepath, curexpdir );
    (void)strcat ( filepath, "/acqfil/fid" );  
    fidMd=mOpen(filepath,0,O_RDONLY);
    (void)mAdvise(fidMd, MF_SEQUENTIAL);
    
    /* how many receivers? */
    error=P_getstring(PROCESSED, "rcvrs", rcvrs_str, 1, MAXSTR);  
    if(!error)
      {
	nchannels=strlen(rcvrs_str);
	for(i=0;i<strlen(rcvrs_str);i++)
	  if(*(rcvrs_str+i)!='y')
	    nchannels--;
      }
    
    /* figure out if compressed in slice dimension */   
    error=P_getstring(PROCESSED, "seqcon", str, 1, MAXSTR);  
    if(error)
      {
	Vperror("nav_split: Error getting seqcon");	
	(void)nav_abort();
	ABORT;
      }
    if(str[1] != 'c')slice_compressed=FALSE;
    if(str[2] != 'c')phase_compressed=FALSE;
    
    
    error=P_getreal(PROCESSED,"ns",&rslices,1);
    if(error)
      {
	Vperror("nav_split: Error getting ns");
	(void)nav_abort();
	ABORT;
      }
    slices=(int)rslices;
    
    error=P_getreal(PROCESSED,"nf",&rnf,1);
    if(error)
      {
	Vperror("nav_split: Error getting nf");
	(void)nav_abort();
	ABORT;
      }
    
    error=P_getreal(PROCESSED,"nv",&rnv,1); 
    if(error)
      {
	Vperror("nav_split: Error getting nv");
	(void)nav_abort();
	ABORT;
      }
    nv=(int)rnv;
    if(!nv)
      {
	Vperror("nav_split:  nv is zero");
	(void)nav_abort();
	ABORT;
      }
    
    error=P_getreal(PROCESSED,"nseg",&rnseg,1); 
    if(error)
      rnseg=1.0;
    
    error=P_getreal(PROCESSED,"ne",&rechoes,1); 
    if(error)
      rechoes=1.0;
    echoes=(int)rechoes;
    if(echoes>1&&epi_seq)
      epi_me=echoes;
    
    error=P_getreal(PROCESSED,"etl",&retl,1); 
    if(error)
      retl=1.0; 
   

    fid_file_head = (dfilehead *)(fidMd->offsetAddr);
    ntraces=fid_file_head->ntraces;
    nblocks=fid_file_head->nblocks;
    nro=fid_file_head->np/2;
    tbytes=fid_file_head->tbytes;
    fidMd->offsetAddr+= sizeof(dfilehead);
    


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
	error=P_getVarInfo(PROCESSED,"image",&info); 
	if(error)
	  {
	    Vperror("nav_split: Error getting image");
	    (void)nav_abort();
	    ABORT;
	  }
	imglen=info.size;
      }
    else
      imglen=1;
    
    error=P_getstring(PROCESSED,"array", arraystr, 1, MAXSTR); 
    if(error)
      {
	Vperror("nav_split: Error getting array");
	(void)nav_abort();
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
	    error=P_getreal(PROCESSED,"image",&rimflag,(i+1));
	    if(error)
	      {
		Vperror("nav_split: Error getting image element");
		(void)nav_abort();
		ABORT;
	      }
	    if(rimflag<=0.0)
	      imzero++;
	  }
      }


    if(slice_compressed)
      {
	slices=(int)rslices;
	slicesperblock=slices;
      }	
    else
      {
	P_getVarInfo(PROCESSED, "pss", &info);
	slices = info.size;
	slicesperblock=1;
	if(slices)
	  within_slices=nblocks/slices;
	else
	  {
	    Vperror("nav_split: slices equal to zero");
	    (void)nav_abort();
	    ABORT;
	  }	
      }
    if(phase_compressed)
      {
	if(slices)
	  slice_reps=nblocks*slicesperblock/slices;
	else
	  {
	    Vperror("nav_split: slices equal to zero");
	    (void)nav_abort();
	    ABORT;
	  }	
	slice_reps -= imzero;
	views=nv;
	viewsperblock=views;
      }
    else
      {
	error=P_getreal(PROCESSED,"ni",&ni,1);
	if(error)
	  {
	    Vperror("nav_split: Error getting ni");
	    (void)nav_abort();
	    ABORT;
	  }
	views=nv;
	if(slices)
	  slice_reps=nblocks*slicesperblock/slices;
	else
	  {
	    Vperror("nav_split: slices equal to zero");
	    (void)nav_abort();
	    ABORT;
	  }	
	if(views)
	  slice_reps/=views;
	else
	  {
	    Vperror("nav_split: views equal to zero");
	    (void)nav_abort();
	    ABORT;
	  }	
	slice_reps -= imzero;
	viewsperblock=1;
	if(views)
	  {
	    within_views=nblocks/views;  /* views are outermost */
	    if(!slice_compressed&&!flash_converted)
	      within_slices/=views;
	  }
	else
	  {
	    Vperror("nav_split: views equal to zero");
	    (void)nav_abort();
	    ABORT;
	  }	

      }

    if(slice_reps<1)
      {
	Vperror("nav_split: slice reps less than 1");
	(void)nav_abort();
	ABORT;
      }	

    etl=views; /* assuming single shot */
    nshots=1;
    multi_shot=FALSE;
    if(epi_seq)
      {
	if(rnseg>1.0)
	  {
	    multi_shot=TRUE;
	    nshots=(int)rnseg;
	    if(rnseg)
	      etl=views/nshots;
	    else
	      {
		Vperror("nav_split: nseg equal to zero");
		(void)nav_abort();
		ABORT;
	      }			
	  }
      }
    else 
      {
	if(retl > 1.0)
	  {
	    multi_shot=TRUE;
	    etl=(int)retl;
	    nshots=views/etl;
	  }
      }
	
    if(flash_converted)
      multi_shot=FALSE;
	
    /* navigator stuff */
    if(!nnav)
      {
	error=P_getVarInfo(PROCESSED,"navigator",&info); 
	if(!error)
	  {
	    if(info.basicType != T_STRING)
	      {
		nnav=info.size;
		nav_list = (int *)allocateWithId(nnav*sizeof(int), "recon_all");
		for(i=0;i<nnav;i++)
		  {
		    error=P_getreal(PROCESSED,"navigator",&dtemp,i+1);
		    nav_list[i]=(int)dtemp - 1;
		    if((nav_list[i]<0)||(nav_list[i]>=(etl+nnav)))
		      {
			Werrprintf("recon_all: navigator value out of range");
			(void)recon_abort();
			ABORT;
		      }
		  }
		error=P_getreal(PROCESSED,"navnp",&dtemp,1);
		if(!error)
		  {
		    nav_pts=(int)dtemp;
		    nav_pts/=2;
		  }
		else
		  nav_pts=nro;
	      }
	    else
	      {
		error=P_getstring(PROCESSED, "navigator", nav_str, 1, SHORTSTR);  
		if(!error)
		  {
		    if(nav_str[0]=='y')
		      {
			/* figure out navigators per echo train */
			n_ft=1;
			while(n_ft<(ntraces/slicesperblock))
			  n_ft*=2;
			n_ft/=2;
			nnav=((ntraces/slicesperblock)-n_ft)/nshots;
			nav_list = (int *)allocateWithId(nnav*sizeof(int), "recon_all");
			for(i=0;i<nnav;i++)
			  nav_list[i]=i;
			nav_pts=nro;
		      }
		  }
	      }	
	  }
      }
    if(nnav)
      {
	if(views == (ntraces/slicesperblock))
	  {
	    views-=nshots*nnav;
	    etl=views/nshots;
	    if(phase_compressed)
	      viewsperblock=views;
	  }
      }
    
    /***********************************************************************************/ 
    /* get array to see how reference scan data was interleaved with acquisition */
    /***********************************************************************************/ 
    dimfirst=1;
    dimafter=1;

    if(imptr != NULL)  /* don't bother if no image */
      {
	imfound=FALSE;
	/* interrogate array to find position of 'image' */	
	ptr=strtok(arraystr,",");
	while(ptr != NULL)
	  {
	    if(ptr == imptr)
	      imfound=TRUE;
		
	    /* is this a jointly arrayed thing? */
	    if(strstr(ptr,"("))
	      {
		while(strstr(ptr,")")==NULL) /* move to end of jointly arrayed list */
		  ptr=strtok(NULL,",");
		    
		*(ptr+strlen(ptr)-1)='\0';
	      }
	    strcpy(aname,ptr);
	    error=P_getVarInfo(PROCESSED,aname,&info);
	    if(error)
	      {
		Vperror("nav_split: Error getting something");
		(void)nav_abort();
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
	
    npts=views*nro; 
    slice=0;
    view=0;
    echo=0;
    
    svinfo.ro_size=nro;
    svinfo.slice_reps=slice_reps;
    svinfo.nblocks=nblocks;
    svinfo.ntraces=ntraces;
    svinfo.dimafter=dimafter;
    svinfo.dimfirst=dimfirst;
    svinfo.multi_shot=multi_shot;
    svinfo.multi_slice=multi_slice;
    svinfo.etl=etl;
    svinfo.slicesperblock=slicesperblock;
    svinfo.viewsperblock=viewsperblock;
    svinfo.within_slices=within_slices;
    svinfo.within_views=within_views;
    svinfo.phase_compressed=phase_compressed;
    svinfo.slice_compressed=slice_compressed;
    svinfo.pe_size=views;
    svinfo.slices=slices;
    svinfo.echoes=echoes;
    svinfo.epi_seq=epi_seq;
    svinfo.flash_converted=flash_converted;
    svinfo.pc_option=pc_option;

    new_etl=etl+nnav;
    nav_traces=nshots*nnav*slices;
    new_ntraces=ntraces-nav_traces;

    /* check some things */
    if(nnav>new_etl)
      {
	Vperror("nav_split: navigators greater than ETL");
	(void)nav_abort();
	ABORT;
      }
    if(new_etl<1)
      {
	Vperror("nav_split:  ETL less than 1");
	(void)nav_abort();
	ABORT;
      }
    if(nav_first>=new_etl)
      {
	Vperror("nav_split: bad first navigator value");
	(void)nav_abort();
	ABORT;
      }
    if((nav_first+nnav)>=new_etl)
      {
	Vperror("nav_split: bad last navigator value");
	(void)nav_abort();
	ABORT;
      }


    /* open the output fid file */
    (void)strcpy ( filepath, curexpdir );
    (void)strcat ( filepath, "/acqfil/nfid" );  
    filesize=sizeof(dfilehead) +  nblocks*(sizeof(dblockhead) + new_ntraces*tbytes);
    newfidMd=mOpen(filepath,filesize,O_RDWR | O_CREAT);
    (void)mAdvise(newfidMd, MF_SEQUENTIAL);
    new_fid_head = (dfilehead *)(newfidMd->offsetAddr);
    /* copy file header */
    (void)memcpy(new_fid_head,fid_file_head,sizeof(dfilehead));
    newfidMd->newByteLen += sizeof(dfilehead);
    newfidMd->offsetAddr += sizeof(dfilehead);
    /* put in new values */
    new_fid_head->ntraces=new_ntraces;
    new_fid_head->bbytes=new_ntraces*tbytes;

    /* open the navigator file */
    (void)strcpy ( filepath, curexpdir );
    (void)strcat ( filepath, "/acqfil/nav" );  
    filesize=sizeof(dfilehead) +  nblocks*(sizeof(dblockhead) + nav_traces*tbytes);
    navMd=mOpen(filepath,filesize,O_RDWR | O_CREAT);
    (void)mAdvise(navMd, MF_SEQUENTIAL);
    nav_head = (dfilehead *)(navMd->offsetAddr);
    /* copy file header */
    (void)memcpy(nav_head,fid_file_head,sizeof(dfilehead));
    navMd->newByteLen += sizeof(dfilehead);
    navMd->offsetAddr += sizeof(dfilehead);
    /* put in new values */
    nav_head->ntraces=nav_traces;
    nav_head->bbytes=nav_traces*tbytes;

			    
    /*********************/
    /* loop on blocks    */
    /*********************/
    iblock=0;
    while(iblock<nblocks)
      {
	if(interuption)
	  {
	    Vperror("nav_split: aborting by request");	
	    (void)nav_abort();
	    ABORT;
	  }

	bhp = (dblockhead *)(fidMd->offsetAddr);
	blockctr=iblock++;

	/* copy block  header to both places*/
	(void)memcpy(newfidMd->offsetAddr,fidMd->offsetAddr,sizeof(dblockhead));
	(void)memcpy(navMd->offsetAddr,fidMd->offsetAddr,sizeof(dblockhead));
	/* update offsets */
	fidMd->offsetAddr += sizeof(dblockhead);  
	newfidMd->newByteLen += sizeof(dblockhead);
	newfidMd->offsetAddr += sizeof(dblockhead);
	navMd->newByteLen += sizeof(dblockhead);
	navMd->offsetAddr += sizeof(dblockhead);

	if(epi_seq)
	  {
	    if(dimafter)
	      icnt=(blockctr/dimafter)%(imglen)+1;
	    else
	      {
		Vperror("nav_split:dimafter equal to zero");
		(void)nav_abort();
		ABORT;
	      }
	    error=P_getreal(PROCESSED,"image",&rimflag,icnt);
	    if(error)
	      {
		Vperror("nav_split: Error getting image element");
		(void)nav_abort();
		ABORT;
	      }
	    imflag=(int)rimflag;
	  }
	else
	  imflag=TRUE;
	
	    for(itrc=0;itrc<ntraces;itrc++) 
	      {	
		/* figure out where to put this echo */
		/* (void)svcalc(itrc,blockctr,&svinfo,&view,&slice,&echo); */

		/* copy the data to new fid file or navigator file */
		iecho=itrc%new_etl;
		
		i=0;
		navflag=FALSE;
		while((i<nnav)&&(!navflag))
		  {
		    if(nav_list[i]==iecho)
		      navflag=TRUE;
		    i++;
		  }
		if(navflag)
		  {
		    (void)memcpy(navMd->offsetAddr,fidMd->offsetAddr,tbytes);
		    navMd->newByteLen += tbytes;
		    navMd->offsetAddr += tbytes;
		  }
		else
		  {
		    (void)memcpy(newfidMd->offsetAddr,fidMd->offsetAddr,tbytes);
		    newfidMd->newByteLen += tbytes;
		    newfidMd->offsetAddr += tbytes;
		  }
		
		fidMd->offsetAddr += tbytes;
		
	      }	/* end of loop on traces */
      }   	  
    /**************************/
    /* end loop on blocks    */
    /**************************/

    mClose(fidMd);
    fidMd=NULL;
    mClose(newfidMd);
    newfidMd=NULL;
    mClose(navMd);
    navMd=NULL;
    (void)releaseAllWithId("nav_split");    

    return(0);
}


/******************************************************************************/
/******************************************************************************/
void nav_abort()
{
  if(fidMd)
    {
      mClose(fidMd);
      fidMd=NULL;
    }  
  if(newfidMd)
    {
      mClose(newfidMd);
      newfidMd=NULL;
    }
  if(navMd)
    {
      mClose(navMd);
      navMd=NULL;
    }
  (void)releaseAllWithId("nav_split");

  return;
}
   


