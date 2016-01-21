/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*****************************************************************************
* File recon_mm.c:  
*  
* reconstruction for single-slab 3D multi-mouse
* note: all variables referring to slice mean slice encoding!
* 
******************************************************************************/


#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include "data.h"
#include "group.h"
#include "mfileObj.h"
#include "symtab.h"
#include "vnmrsys.h"
#include "variables.h"
#include "fft.h"
#include "ftpar.h"
#include "process.h"
#include "epi_recon.h"
#include "phase_correct.h"
#include "mfileObj.h"
#include "allocate.h"
#include "pvars.h"
#include "wjunk.h"


#ifdef VNMRJ
#include "aipCInterface.h"
#endif 

extern int interuption;
extern void Vperror(char *);
extern int specIndex;
extern double getfpmult(int fdimname, int ddr);

static float *slicedata, *pc, *magnitude, *mag2;
static float *rawmag, *rawphs;
static float *navdata;
static double *nav_refphase;
static int *view_order, *nav_list;
static int *sview_order;
#ifdef MULTISLAB
static int *slice_order;
#endif
static int *blockreps;
static int *blockrepeat;
static int *repeat;
static int *nts;
static int *pc_done;
static reconInfo rInfo; 
static int realtime_block;

static int pe2_size;
static int arraydim;
static int miceperchannel;
static double *ro_frq;

float *pss;
float *upss;
int nmice;

/* prototype */
void recon_mmabort();
extern int write_fdf(int,float *,fdfInfo *,int *, int,char *, int, int);
extern int upsscompare(const void *, const void *);
static void generate_images(int slices, int views, int nro, int nmice,
           int slabs, float *window, fdfInfo *pInfo, int image_order,
           int multi_shot, int *dispcntptr, int dispint, int zeropad,
			    int zeropad2, char *arstr,double *frq1, double *frq2);
int phaseslice_ft(float *, int, int, int, float *, float *, int, int, int, int, float *, double *, double *);
#ifdef MULTISLAB
static  int psscompare(const void *p1, const void *p2);
static  int pcompare(const void *p1, const void *p2);
#endif


/*******************************************************************************/
/*******************************************************************************/
/*           recon_mm                      (multi-mouse!)                             */
/*******************************************************************************/
/*******************************************************************************/
int recon_mm (int argc, char *argv[], int retc, char *retv[] )

/* 

Purpose:
-------
reconstructs 3D multi-mouse data sets and produces fdf images

Arguments:
---------
argc  :  (   )  Argument count.
argv  :  (   )  Command line arguments.
retc  :  (   )  Return argument count.  Not used here.
retv  :  (   )  Return arguments.  Not used here

*/

{
/*   Local Variables: */
    FILE *f1;
    FILE *table_ptr;

    char filepath[MAXPATHL];
    char tablefile[MAXPATHL];
    char petable[MAXSTR];
    char sequence[MAXSTR];
    char studyid[MAXSTR];
    char pos1[MAXSTR];
    char pos2[MAXSTR];
    char apptype[MAXSTR];
    char msintlv[MAXSTR];
    char fdfstr[MAXSTR];
    char rcvrs_str[MAXSTR];
    char rscfilename[MAXPATHL];
    char epi_pc[MAXSTR];
    char nav_type[MAXSTR];
    char recon_windowstr[MAXSTR];
    char aname[SHORTSTR];
    char nav_str[SHORTSTR];
    char raw_str[SHORTSTR];
    char dcrmv[SHORTSTR];
    char arraystr[MAXSTR];
    char *ptr;
    char *imptr;
    char str[MAXSTR];
    char systr[MAXSTR];
    char acqdir[MAXPATHL];
    char fidtemp[MAXPATHL];
    char fidtemp2[MAXPATHL];
    char fidtemp3[MAXPATHL];
    short  *sdataptr;
    short s1;
    int l1;
    vInfo info;

    double fpointmult;
    double rnseg, rfc, retl, rnv, rnv2;
    double recon_force;
    double rimflag;
    double rechoes, ni, repi_rev;
    double rslices,rnf;
    double dtemp, dt2;
    double frq;
    double m1,m2;
    double acqtime;
    double *pe2_frq;
    double *pe_frq;

    dpointers inblock;
    dblockhead *bhp;
    dfilehead *fid_file_head;
#ifdef LINUX
    dfilehead tmpFileHead;
    dblockhead tmpBhp;
#endif

    float *fptr, *nrptr;
    float *datptr;
    float *wptr;
    float *read_window;
    float *phase_window;
    float *fdataptr;
    float real_dc,imag_dc;
    float a,b,c,d;
    float t1, t2, *pt1, *pt2, *pt3, *pt4, *pt5, *pt6;
    float ftemp;

    int status;
    int nshifts;
    int echoes;
    int tstch;
    int *idataptr;
    int imglen,dimfirst,dimafter,imfound;
    int imzero;
    int icnt;
    int nv;
    int k;
    int iad;
    int uslice;
    int ispb, itrc, it;
    int blockctr;
    int pc_offset;
    int soffset;
    int iro;
    int ipc;
    int ntlen;
    int nt;
    int index;
    int tablen, itablen;
    int within_nt;
    int dsize,nsize;
    int views, nro;
    int nro2;
    int ro_size, pe_size;
    int ro2;
    int slab=0;
    int slabs, slabsperblock;
    int slab_reps;
    int slices, etl, nblocks,ntraces;
    int viewsperblock;
    int uslabsperblock;
    int error,error1;
    int npts;
    int npts3d;
    int im_slice;
    int pc_slice;
    int itab;
    int nch;
    int view, slice, echo;
    int oview;
    int oslice;
    int nchannels=1;
    int within_views=1;
    int within_slices=1;
    int rep;
    int i, min_view, iview;
    int min_sview;
    int narg;
    int zeropad=0;
    int zeropad2=0;
    int n_ft;
    int ctcount;
    int pwr,level;
    int fnt;
    int nshots;
    int nsize_ref;
    int nnav, nav_pts;
    int nav;
    int fn,fn1;
    int wtflag;
    int nfids;
    int ifid;
    int itemp;
    int imouse;

    /*    useconds_t snoozetime; */
    struct wtparams  wtp;
 
    /* flags */
    int options=FALSE;
    int pc_option=OFF;
    int nav_option=OFF;
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
    int phase_compressed=TRUE;
    int flash_converted=FALSE;
    int epi_me=FALSE;
    int navflag=FALSE;
    int rawflag=FALSE;
    int halfF_ro=FALSE;
    int image_jnt=FALSE;
    int sort_each_echo=FALSE;

    symbol **root;

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
	Werrprintf("recon_mm: aborting by request");	
	(void)recon_mmabort();
	ABORT;
      }

    /* what sequence is this ? */
    error=P_getstring(PROCESSED, "seqfil", sequence, 1, MAXSTR);  
    if(error)
      {
	Werrprintf("recon_mm: Error getting seqfil");	
	(void)recon_mmabort();
	ABORT;
      }		

    /* aggregate separate fid files if necessary */
    nfids=1;
    error=P_getreal(PROCESSED,"numpetables",&dtemp,1);
    if(!error)
      nfids=(int)dtemp;    
    if(nfids>1)
      {
	(void)strcpy(acqdir,curexpdir);
	(void)strcat(acqdir,"/acqfil/");
	(void)strcpy(fidtemp3,acqdir);
	(void)strcat(fidtemp3,"fidtmp");
	for(ifid=nfids-1;ifid>0;ifid--)
	  {
	    /* remove fid file header */
	    (void)strcpy(fidtemp,acqdir);
	    (void)sprintf(str,"fid%d",ifid);
	    (void)strcat(fidtemp,str);
	    (void)sprintf(systr,"tail +%dc %s > %s_tmp \n",
			  (int) sizeof(dfilehead)+1,fidtemp,fidtemp);
	    (void)system(systr);

	    /* cat onto the next fid file */
	    (void)strcpy(fidtemp2,acqdir);
	    (void)sprintf(str,"fid%d",ifid-1);
	    (void)strcat(fidtemp2,str);
	    (void)sprintf(systr,"cat %s %s_tmp > %s \n",
			  fidtemp2,fidtemp,fidtemp3);
	    (void)system(systr);

	    /* copy result of cat back */
	    (void)sprintf(systr,"cp %s %s \n",
			  fidtemp3,fidtemp2);
	    (void)system(systr);
	  }
	(void)strcpy(fidtemp,acqdir);
	(void)strcat(fidtemp,"fid");
	(void)sprintf(systr,"cp %s %s \n",
		      fidtemp2,fidtemp);
	(void)system(systr);

	/* clean up */
	(void)sprintf(systr,"\rm %s/*tmp  \n",
		      acqdir);
	(void)system(systr);
      }
	    
    /* get studyid */
    error=P_getstring(PROCESSED, "studyid_", studyid, 1, MAXSTR);  
    if(error)
      (void)strcpy(studyid,"unknown");

    /* get patient positions */
    error=P_getstring(PROCESSED, "position1", pos1, 1, MAXSTR);  
    if(error)
      (void)strcpy(pos1,"");
    error=P_getstring(PROCESSED, "position2", pos2, 1, MAXSTR);  
    if(error)
      (void)strcpy(pos2,"");

    /* get apptype */
    error=P_getstring(PROCESSED, "apptype", apptype, 1, MAXSTR);  
    /*
    if(error)
      {
	Werrprintf("recon_mm: Error getting apptype");	
	(void)recon_mmabort();
	ABORT;
      }		
    if(!strlen(apptype))
      {
	Winfoprintf("recon_mm: WARNING:apptype unknown!");	
	Winfoprintf("recon_mm: Set apptype in processed tree");	
      }
    */
    if(!error)
      {
	if(strstr(apptype,"imEPI"))
	  epi_seq=TRUE;
      }

    error=P_getstring(PROCESSED,"epi_pc", epi_pc, 1, MAXSTR);
    if(!error)
      {
	epi_seq=TRUE;
	epi_rev=TRUE;
	pc_option=pc_pick(epi_pc);
	if((pc_option>MAX_OPTION)||(pc_option<OFF))
	  {
	    Werrprintf("recon_mm: Invalid phase correction option in epi_pc");
	    (void)recon_mmabort();
	    ABORT;
	  }
      }

    
    if(epi_seq)
      pc_option=POINTWISE;

/* further arguments for phase correction & image directory */
    if(argc>narg) 
      {
	options=TRUE;
	pc_option=pc_pick(argv[narg]);
	if((pc_option>MAX_OPTION)||(pc_option<OFF))
	  {
	    (void)strcpy(rInfo.picInfo.imdir,argv[narg++]);
	    rInfo.picInfo.fullpath=TRUE;
	    if(argc>narg)
	      {
		if(epi_seq)
		  {
		    (void)strcpy(epi_pc,argv[narg]);
		    pc_option=pc_pick(epi_pc);
		    if((pc_option>MAX_OPTION)||(pc_option<OFF))
		      {
			Werrprintf("recon_mm: Invalid phase correction option in command line");
			(void)recon_mmabort();
			ABORT;
		      }
		  }
	      }
	  }
	else
	  {
	    if(epi_seq)
	      {
		(void)strcpy(epi_pc,argv[narg]);		  
		pc_option=pc_pick(epi_pc);
		if((pc_option>MAX_OPTION)||(pc_option<OFF))
		  {
		    Werrprintf("recon_mm: Invalid phase correction option in command line");
		    (void)recon_mmabort();
		    ABORT;
		  }
	      }
	  }
      }
    if(!options)  /* try vnmr parameters */
      {
	error=P_getstring(PROCESSED,"epi_pc", epi_pc, 1, MAXSTR);
	if(!error && epi_seq)
	  {
	    options=TRUE;
	    pc_option=pc_pick(epi_pc);
	    if((pc_option>MAX_OPTION)||(pc_option<OFF))
	      {
		Werrprintf("recon_mm: Invalid phase correction option in epi_pc");
		(void)recon_mmabort();
		ABORT;
	      }
	    error=P_getreal(PROCESSED,"epi_rev", &repi_rev, 1);
	    if(!error)
	      {
		if(repi_rev>0.0)
		  epi_rev=TRUE;
		else
		  epi_rev=FALSE;
	      }
	  }
      }
    if(!options)  /* try the resource file to get recon particulars */
      {
	(void)strcpy(rscfilename,curexpdir);
	(void)strcat(rscfilename,"/recon_mm.rsc");
	f1=fopen(rscfilename,"r");
	if(f1)
	  {
	    options=TRUE;
	    (void)fgets(str,MAXSTR,f1);
	    if(strstr(str,"image directory"))
	      {
		(void)sscanf(str,"image directory=%s",rInfo.picInfo.imdir);
		rInfo.picInfo.fullpath=FALSE;
		if(epi_seq)
		  {
		    (void)fgets(epi_pc,MAXSTR,f1);
		    pc_option=pc_pick(epi_pc);
		    if((pc_option>MAX_OPTION)||(pc_option<OFF))
		      {
			Werrprintf("recon_mm: Invalid phase correction option in recon_mm.rsc file");
			(void)recon_mmabort();
			ABORT;
		      }

		    (void)fgets(str,MAXSTR,f1);
		    (void)sscanf(str,"reverse=%d",&epi_rev);
		  }
	      }
	    else  /* no image directory specifed */
	      {
		if(epi_seq)
		  {
		    (void)strcpy(epi_pc,str);		  
		    pc_option=pc_pick(epi_pc);
		    if((pc_option>MAX_OPTION)||(pc_option<OFF))
		      {
			Werrprintf("recon_mm: Invalid phase correction option in recon_mm.rsc file");
			(void)recon_mmabort();
			ABORT;
		      }

		    (void)fgets(str,MAXSTR,f1);
		    (void)sscanf(str,"reverse=%d",&epi_rev);
		  }		  
	      }
	    (void)fclose(f1);
	  }
      }

    if(epi_seq)
      {
	if((pc_option>MAX_OPTION)||(pc_option<OFF))
	  pc_option=POINTWISE;
      }
    
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

	if(!epi_seq)
	  {
	    epi_rev=FALSE;
	    pc_option=OFF;
	  }
	
	/* display images or not? */
	rInfo.dispint=1;
	error=P_getreal(PROCESSED,"recondisplay",&dtemp,1);
	if(!error)
	  rInfo.dispint=(int)dtemp;
	
	/* get choice of filter, if any */
	error=P_getstring(PROCESSED,"recon_window", recon_windowstr, 1, MAXSTR);
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
	error=P_getstring(PROCESSED, "rcvrs", rcvrs_str, 1, MAXSTR);  
	if(!error)
	  {
	    nchannels=strlen(rcvrs_str);
	    for(i=0;i<strlen(rcvrs_str);i++)
	      if(*(rcvrs_str+i)!='y')
		nchannels--;
	  }
	rInfo.nchannels=nchannels;

	/* how many mousies? */
	error=P_getreal(PROCESSED,"nmice",&dtemp,1); 
	if(!error)
	  nmice=(int)dtemp;
	else
	  nmice=nchannels;  /* default if not set */

	nch=0;
	while(nmice>nch)
	  nch+=nchannels;
	miceperchannel=nch/nchannels; /* multiplexed mice per channel */
	
	if(nmice<nchannels)
	  {
	    nchannels=nmice;
	    rInfo.nchannels=nchannels;
	  }

	/* figure out if compressed in slice dimension */   
	error=P_getstring(PROCESSED, "seqcon", str, 1, MAXSTR);  
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting seqcon");	
	    (void)recon_mmabort();
	    ABORT;
	  }
	/* 	if(str[1] != 'c')slice_compressed=FALSE; */
	if(str[2] != 'c')phase_compressed=FALSE;
	if(str[3] != 'c')slice_compressed=FALSE;
    
	error=P_getreal(PROCESSED,"ns",&rslices,1);
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting ns");
	    (void)recon_mmabort();
	    ABORT;
	  }
	/* 	slices=(int)rslices; */
	
	rnv2=0.0;
	error=P_getreal(PROCESSED,"nv2",&rnv2,1);
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting nv2");
	    (void)recon_mmabort();
	    ABORT;
	  }
	slices=(int)rnv2;

	error=P_getreal(PROCESSED,"nf",&rnf,1);
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting nf");
	    (void)recon_mmabort();
	    ABORT;
	  }
  
	error=P_getreal(PROCESSED,"nv",&rnv,1); 
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting nv");
	    (void)recon_mmabort();
	    ABORT;
	  }
	nv=(int)rnv;

	if(nv<MINPE)
	  {
	    Werrprintf("recon_mm:  nv too small");
	    (void)recon_mmabort();
	    ABORT;
	  }

#ifdef LINUX
        memcpy( & tmpFileHead, (dfilehead *)(rInfo.fidMd->offsetAddr), sizeof(dfilehead) );
        fid_file_head = &tmpFileHead;
#else
	fid_file_head = (dfilehead *)(rInfo.fidMd->offsetAddr);
#endif
	/* byte swap if necessary */
	DATAFILEHEADER_CONVERT_NTOH(fid_file_head);
	ntraces=fid_file_head->ntraces;
	nblocks=fid_file_head->nblocks;
	if(nfids)
	  nblocks*=nfids;
	nro=fid_file_head->np/2;
	rInfo.fidMd->offsetAddr+= sizeof(dfilehead);
	
	if(nro<MINRO)
	  {
	    Werrprintf("recon_mm:  np too small");
	    (void)recon_mmabort();
	    ABORT;
	  }

	/* check for power of 2 in readout */
	zeropad=0;
	fn=0;
	error=P_getVarInfo(CURRENT,"fn",&info); 
	if(info.active)
	  {
	    error=P_getreal(CURRENT,"fn",&dtemp,1); 
	    if(error)
	      {
		Werrprintf("recon_mm: Error getting fn");
		(void)recon_mmabort();
		ABORT;
	      }
	    fn=(int)(dtemp/2);
	    ro_size=fn;
	  }
	else
	  {
	    n_ft=1;
	    while(n_ft<nro)
	      n_ft*=2;
	    ro_size=n_ft;
	  }

	error=P_getreal(PROCESSED, "fract_kx", &dtemp, 1);  
	if(!error)
	  {
	    ro_size=2*(nro-(int)dtemp);
	    zeropad=ro_size-nro;
	    n_ft=1;
	    while(n_ft<ro_size)
	      n_ft*=2;
	    ro_size=n_ft;
	    if(n_ft<fn)
	      ro_size=fn;
	    
	    if((zeropad > HALFF*nro)&&(pc_option<=MAX_OPTION)&&(pc_option>OFF))
	      {
	      	Werrprintf("recon_mm: phase correction and fract_kx are incompatible");
		(void)recon_mmabort();
		ABORT;
	      }	
	  }

	if(nro<ro_size)
	  {
	    itemp=ro_size-nro;
	    nro=ro_size;
	    ro_size-=itemp;
	  }
	else
	  ro_size=nro;
	
	/* get lsfrq and lsfrq1 */
	ro_frq=NULL;
	error=P_getVarInfo(CURRENT,"lsfrq",&info); 
	if(!error && info.active)
	  {
	    nshifts=info.size;	
	    if(((nshifts > 1) &&nshifts != nmice))
	      {
		Werrprintf("recon_mm: length of lsfrq does not equal nmice ");	
		(void)recon_mmabort();
		ABORT;
	      }
	    ro_frq=(double *)allocateWithId(nmice*sizeof(double),"recon_mm");    
	    error=P_getreal(PROCESSED,"sw",&dt2,1); 
	    if(error)
	      {
		Werrprintf("recon_mm: Error getting sw ");	
		(void)recon_mmabort();
		ABORT;
	      }
	    if(dt2==0.0)
	      {
		Werrprintf("recon_mm: sw set to zero in processed tree! ");	
		(void)recon_mmabort();
		ABORT;
	      }	    
	    if(nshifts==1)
	      {
		error=P_getreal(CURRENT,"lsfrq",&dtemp,1);
		if(error)
		  {
		    Werrprintf("recon_mm: Error getting lsfrq ");
		    (void)recon_mmabort();
		    ABORT;
		  }
		for(i=0;i<nmice;i++)
		  ro_frq[i]=-180.0*dtemp/dt2;
	      }
	    else
	      {
		for(i=0;i<nmice;i++)
		  {
		    error=P_getreal(CURRENT,"lsfrq",&dtemp,(i+1));
		    if(error)
		      {
			Werrprintf("recon_mm: Error getting lsfrq element");
			(void)recon_mmabort();
			ABORT;
		      }
		    ro_frq[i]=-180.0*dtemp/dt2;
		  }
	      }
	  }
	    
	pe_frq=NULL;
	error=P_getVarInfo(CURRENT,"lsfrq1",&info); 
	if(!error && info.active)
	  {
	    nshifts=info.size;	
	    if(((nshifts > 1) &&nshifts != nmice))
	      {
		Werrprintf("recon_mm: length of lsfrq does not equal nmice ");	
		(void)recon_mmabort();
		ABORT;
	      }
	    pe_frq=(double *)allocateWithId(nmice*sizeof(double),"recon_mm");    
	    error=P_getreal(PROCESSED,"sw1",&dt2,1); 
	    if(error)
	      {
		Werrprintf("recon_mm: Error getting sw1 ");	
		(void)recon_mmabort();
		ABORT;
	      }
	    if(dt2==0.0)
	      {
		Werrprintf("recon_mm: sw1 set to zero in processed tree! ");	
		(void)recon_mmabort();
		ABORT;
	      }
	    if(nshifts==1)
	      {
		error=P_getreal(CURRENT,"lsfrq1",&dtemp,1);
		if(error)
		  {
		    Werrprintf("recon_mm: Error getting lsfrq1 ");
		    (void)recon_mmabort();
		    ABORT;
		  }
		for(i=0;i<nmice;i++)
		  pe_frq[i]=180.0*dtemp/dt2;
	      }
	    else
	      {
		for(i=0;i<nmice;i++)
		  {
		    error=P_getreal(CURRENT,"lsfrq1",&dtemp,(i+1));
		    if(error)
		      {
			Werrprintf("recon_mm: Error getting lsfrq1 element");
			(void)recon_mmabort();
			ABORT;
		      }
		    pe_frq[i]=180.0*dtemp/dt2;
		  }
	      }
	  }

	pe2_frq=NULL;
	error=P_getVarInfo(CURRENT,"lsfrq2",&info); 
	if(!error && info.active)
	  {
	    nshifts=info.size;	
	    if(((nshifts > 1) &&nshifts <  nmice))
	      {
		Werrprintf("recon_mm: length of lsfrq2 less than nmice ");	
		(void)recon_mmabort();
		ABORT;
	      }
	    pe2_frq=(double *)allocateWithId(nmice*sizeof(double),"recon_mm");    
	    error=P_getreal(PROCESSED,"sw2",&dt2,1); 
	    if(error)
	      {
		Werrprintf("recon_mm: Error getting sw2 ");	
		(void)recon_mmabort();
		ABORT;
	      }
	    if(dt2==0.0)
	      {
		Werrprintf("recon_mm: sw2 set to zero in processed tree! ");	
		(void)recon_mmabort();
		ABORT;
	      }
	    if(nshifts==1)
	      {
		error=P_getreal(CURRENT,"lsfrq2",&dtemp,1);
		if(error)
		  {
		    Werrprintf("recon_mm: Error getting lsfrq2 ");
		    (void)recon_mmabort();
		    ABORT;
		  }
		for(i=0;i<nmice;i++)
		  pe2_frq[i]=180.0*dtemp/dt2;
	      }
	    else
	      {
		k=0;
		for(i=0;i<strlen(rcvrs_str);i++)
		  {
		    if(*(rcvrs_str+i)=='y')
		      {
		      error=P_getreal(CURRENT,"lsfrq2",&dtemp,(i+1));
		      if(error)
			{
			  Werrprintf("recon_mm: Error getting lsfrq2 element");
			  (void)recon_mmabort();
			  ABORT;
			}
		      pe2_frq[k++]=180.0*dtemp/dt2;
		      }
		  }
	      }
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

	/* get some epi related parameters */
	error1=P_getreal(PROCESSED,"flash_converted",&rfc,1);
	if(!error1)
	  flash_converted=TRUE;  
	
	/*
	if (slice_compressed&&!epi_seq)
	  multi_slice=TRUE;
	*/
	
	error=P_getstring(PROCESSED, "ms_intlv", msintlv, 1, MAXSTR);  
	if(!error)
	  {
	    if(msintlv[0]=='y')
	      multi_slice=TRUE;
	  }

	imglen=1;
	error=P_getVarInfo(PROCESSED,"image",&info); 
	if(!error)
	  imglen=info.size;
    
	error=P_getstring(PROCESSED,"array", arraystr, 1, MAXSTR); 
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting array");
	    (void)recon_mmabort();
	    ABORT;
	  }

	/* locate image within array to see if reference scan was acquired */
	imzero=0;
	imptr=strstr(arraystr,"image");
	if(!imptr)
	  imglen=1;  /* image was not arrayed */
	else if(imglen>1)
	  {
	    /* how many when image=1? */
	    for(i=0;i<imglen;i++)
	      {
		error=P_getreal(PROCESSED,"image",&rimflag,(i+1));
		if(error)
		  {
		    Werrprintf("recon_mm: Error getting image element");
		    (void)recon_mmabort();
		    ABORT;
		  }
		if(rimflag<=0.0)
		  imzero++;
	      }
	  }
	if(!imzero)
	  pc_option=OFF;

	/* get nt  array  */
	error=P_getVarInfo(PROCESSED,"nt",&info); 
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting nt info");
	    (void)recon_mmabort();
	    ABORT;
	  }
	ntlen=info.size;	    /* read nt values*/
	nts=(int *)allocateWithId(ntlen*sizeof(int),"recon_mm");
	for(i=0;i<ntlen;i++)
	  {
	    error=P_getreal(PROCESSED,"nt",&dtemp,(i+1));
	    if(error)
	      {
		Werrprintf("recon_mm: Error getting nt element");
		(void)recon_mmabort();
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

#ifdef MULTISLAB
	if(slice_compressed)
	  {
	    slabs=(int)rslices;
	    slabsperblock=slabs;
	  }	
	else
	  {
	    P_getVarInfo(PROCESSED, "pss", &info);
	    slabs = info.size;
	    slabsperblock=1;
	    if(slabs)
	      within_slabs=nblocks/slabs;
	    else
	      {
		Werrprintf("recon_mm: slabs equal to zero");
		(void)recon_mmabort();
		ABORT;
	      }	
	  }
#endif	

	slabs=1;
	slabsperblock=1;
	if(phase_compressed)
	  {
	    if(slabs)
	      slab_reps=nblocks*slabsperblock/slabs;
	    else
	      {
		Werrprintf("recon_mm: slabs equal to zero");
		(void)recon_mmabort();
		ABORT;
	      }	
	    slab_reps -= imzero;
	    if(slabsperblock)
	      views=ntraces/slabsperblock;
	    else
	      {
		Werrprintf("recon_mm: slabsperblock equal to zero");
		(void)recon_mmabort();
		ABORT;
	      }	
	    views=nv;
	    viewsperblock=views;
	  }
	else
	  {
	    error=P_getreal(PROCESSED,"ni",&ni,1);
	    if(error)
	      {
		Werrprintf("recon_mm: Error getting ni");
		(void)recon_mmabort();
		ABORT;
	      }
	    views=(int)ni;
	    views=nv;
	    if(slabs)
	      slab_reps=nblocks*slabsperblock/slabs;
	    else
	      {
		Werrprintf("recon_mm: slabs equal to zero");
		(void)recon_mmabort();
		ABORT;
	      }	
	    if(views)
	      slab_reps/=views;
	    else
	      {
		Werrprintf("recon_mm: views equal to zero");
		(void)recon_mmabort();
		ABORT;
	      }	
	    slab_reps -= imzero;
	    viewsperblock=1;
	    if(views)
	      {
		within_views=nblocks/views;  /* views are outermost */
		if(!slice_compressed&&!flash_converted)
		  within_slices/=views;
	      }
	    else
	      {
		Werrprintf("recon_mm: views equal to zero");
		(void)recon_mmabort();
		ABORT;
	      }	

	  }

	if(!slice_compressed)
	      within_slices=nchannels;

	/* default is single shot */
	etl=views;
	nshots=1;
	multi_shot=FALSE;
	if(epi_seq)
	  {
	    if(rnseg>1.0)
	      {
		nshots=(int)rnseg;
		multi_shot=TRUE;
		if(rnseg)
		  etl=(int)views/nshots;
		else
		  {
		    Werrprintf("recon_mm: nseg equal to zero");
		    (void)recon_mmabort();
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


	/* slice encode direction zero fill or not? */
	n_ft=1;
	while(n_ft<slices)
	  n_ft*=2;
	pe2_size=n_ft;

	/* get array info */
	rInfo.narray=0;
	if(strlen(arraystr))
	  (void)arrayparse(arraystr, &(rInfo.narray), &(rInfo.arrayelsP),
			   (views/viewsperblock));
	
	/* compute product of arrayed element sizes */
	arraydim=1;
	for(iad=0;iad<rInfo.narray;iad++)
	  arraydim*=rInfo.arrayelsP[iad].size;
	arraydim*=nchannels;

	/* figure out if acq was phase compressed and multi--shot or not */
	if(phase_compressed&&multi_shot)
	  {
	    if(!slice_compressed)
	      arraydim*=slices;
	    if(nblocks > arraydim)
	      {
		phase_compressed=FALSE;
		within_views=nblocks/nshots;
		viewsperblock=etl;
		if(!slice_compressed)
		  within_slices=within_views/slices;
	      }
	    if(!slice_compressed) 
	      arraydim/=slices;	      
	  }
	
	zeropad2=0;
	fn1=0;
	error=P_getVarInfo(CURRENT,"fn1",&info); 
	if(info.active)
	  {
	    error=P_getreal(CURRENT,"fn1",&dtemp,1); 
	    if(error)
	      {
		Werrprintf("recon_mm: Error getting fn1");
		(void)recon_mmabort();
		ABORT;
	      }
	    fn1=(int)(dtemp/2);
	    pe_size=fn1;
	  }
	else
	  {
	    n_ft=1;
	    while(n_ft<views)
	      n_ft*=2;
	    pe_size=n_ft;
	  }

	
	error=P_getreal(PROCESSED, "fract_ky", &dtemp, 1);  
	if(!error)
	  {
	    pe_size=views;
	    views=views/2 + (int)dtemp;
	    zeropad2=pe_size-views;
	    n_ft=1;
	    while(n_ft<pe_size)
	      n_ft*=2;
	    pe_size=n_ft;
	    if(n_ft<fn1)
	      pe_size=fn1;

	    /* adjust etl  */
	    if(nshots)
	      etl=views/nshots;
	    if(phase_compressed)
	      viewsperblock=views;

	    if((zeropad > HALFF*nro)&&(zeropad2 > HALFF*views))	   
	      {
	      	Werrprintf("recon_mm: fract_kx and fract_ky are incompatible");
		(void)recon_mmabort();
		ABORT;
	      }	
	  }


	/* navigator stuff */
	nnav=0;
	error=P_getVarInfo(PROCESSED,"navigator",&info); 
	if(!error)
	  {
	    if(info.basicType != T_STRING)
	      {
		nnav=info.size;
		nav_list = (int *)allocateWithId(nnav*sizeof(int), "recon_mm");
		for(i=0;i<nnav;i++)
		  {
		    error=P_getreal(PROCESSED,"navigator",&dtemp,i+1);
		    nav_list[i]=(int)dtemp - 1;
		    if((nav_list[i]<0)||(nav_list[i]>=(etl+nnav)))
		      {
			Werrprintf("recon_mm: navigator value out of range");
			(void)recon_mmabort();
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
		  nav_pts=ro_size;
	      }
	    else
	      {
		error=P_getstring(PROCESSED, "navigator", nav_str, 1, SHORTSTR);  
		if(!error)
		  {
		    if(nav_str[0]=='y')
		      {
			/* figure out navigators per echo train */
			nnav=((ntraces/slabsperblock)-views)/nshots;
			nav_list = (int *)allocateWithId(nnav*sizeof(int), "recon_mm");
			for(i=0;i<nnav;i++)
			  nav_list[i]=i;
			nav_pts=ro_size;
		      }
		  }
	      }
	    
	    nav_option=OFF;
	    if(nnav)
	      {
		nav_option=POINTWISE;
		if(nnav>1) 	  
		  nav_option=PAIRWISE;
		
		error=P_getstring(PROCESSED,"nav_type", nav_type, 1, MAXSTR);
		if(!error)
		  {
		    if(strstr(nav_type,"OFF")||strstr(nav_type,"off"))
		      nav_option=OFF;
		    else if(strstr(nav_type,"POINTWISE")||strstr(nav_type,"pointwise"))
		      nav_option=POINTWISE; 	 
		    else if(strstr(nav_type,"PAIRWISE")||strstr(nav_type,"pairwise"))
		      nav_option=PAIRWISE; 	 
		    else
		      {
			Werrprintf("recon_mm: Invalid navigator option in nav_type parameter!");
			(void)recon_mmabort();
			ABORT;
		      }
		  }
	      }

	    if((nav_option==PAIRWISE)&&(nnav<2))
	      {
		Werrprintf("recon_mm: 2 navigators required for PAIRWISE nav_type");
		(void)recon_mmabort();
		ABORT;	      
	      }
	    /* adjust etl and views if necessary */
	    if(nnav)
	      {
		if(views == (ntraces/slabsperblock))
		  {
		    views-=nshots*nnav;
		    etl=views/nshots;
		    if(phase_compressed)
		      viewsperblock=views;
		  }
	      }
	  }


	if(slab_reps<1)
	  {
	    Werrprintf("recon_mm: slab reps less than 1");
	    (void)recon_mmabort();
	    ABORT;
	  }	
	
	error=P_getstring(PROCESSED, "petable", petable, 1, MAXSTR);  
	/* open table for sorting */
	if((!error)&&strlen(petable)&&strcmp(petable,"n")&&strcmp(petable,"N"))
	  {
	    if(nfids>1)
	      petable[strlen(petable)-1]='\0';
	    
	    multi_shot=TRUE;
	    /*  try user's directory*/
	    error=P_getstring(GLOBAL, "userdir", tablefile, 1, MAXPATHL);  
	    if(error)
	      {
		Werrprintf("recon_mm: Error getting userdir");	
		(void)recon_mmabort();
		ABORT;
	      }
	    (void)strcat(tablefile,"/tablib/" );  
	    error=P_getstring(PROCESSED, "petable", petable, 1, MAXSTR);  
	    if(error)
	      {
		Werrprintf("recon_mm: Error getting petable");	
		(void)recon_mmabort();
		ABORT;
	      }
	    (void)strcat(tablefile,petable);  
	    table_ptr = fopen(tablefile,"r");
	    if(!table_ptr)
	      {	
		/* check system directory */
		(void)strcpy(tablefile,"/vnmr/tablib/");
		(void)strcat(tablefile,petable);  
		table_ptr = fopen(tablefile,"r");
		if(!table_ptr)
		  {
		    Werrprintf("recon_mm: Error opening table file");	
		    (void)recon_mmabort();
		    ABORT;
		  }
	      }	
	    
	    tablen=views;
	    if(rnv2>0.0)
	      tablen*=slices;

	    /* first figure out size of sorting table(s) */
	    done=FALSE;
	    while ((tstch=fgetc(table_ptr) != '=') && (tstch != EOF));
	    itablen = 0;
	    while ((itablen<tablen) && !done)
	      {
		if(fscanf(table_ptr, "%d", &iview) == 1)
		  itablen++;
		else	
		  done = TRUE;
	      }	    
	    /* reposition to start of file */
	    error=fseek(table_ptr,0,SEEK_SET);
	    if(error)
	      {
		Werrprintf("recon_mm: Error with fseek, petable");	
		(void)recon_mmabort();
		ABORT;
	      }
	    if((itablen!=views)&&(itablen!=tablen))
	      {
		Werrprintf("recon_mm: Error wrong phase sorting table size");	
		(void)recon_mmabort();
		ABORT;
	      }

	    /* read in table for view sorting */
	    view_order = (int *)allocateWithId(tablen*sizeof(int), "recon_mm");
	    while ((tstch=fgetc(table_ptr) != '=') && (tstch != EOF));
	    if(tstch == EOF)
	      {
		Werrprintf("recon_mm: EOF while reading petable file");	
		(void)recon_mmabort();
		ABORT;
	      }
	    itab = 0;
	    min_view=0;
	    done=FALSE;
	    while ((itab<tablen) && !done)
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
	    if(itab==views*slices)
	      sort_each_echo=TRUE;

	    /* check for t2 table describing slice encode ordering */
	    tstch=fgetc(table_ptr);
	    while ((tstch != 't') && (tstch != EOF))
	      tstch=fgetc(table_ptr);

	    sview_order=NULL;
	    if((tstch != EOF)&&(rnv2>0.0))
	      {
		sview_order = (int *)allocateWithId(tablen*sizeof(int), "recon_mm");
		while ((tstch=fgetc(table_ptr) != '=') && (tstch != EOF));
		if(tstch == EOF)
		  {
		    Werrprintf("recon_mm: EOF while reading petable file for t2");	
		    (void)recon_mmabort();
		    ABORT;
		  }
		itab = 0;
		min_sview=0;
		done=FALSE;
		while ((itab<tablen) && !done)
		  {
		    if(fscanf(table_ptr, "%d", &iview) == 1)
		      {
			sview_order[itab]=iview;
			if(iview<min_sview)	
			  min_sview=iview;
			itab++;	
		      }
		    else	
		      done = TRUE;
		  }
	      }
	    (void)fclose(table_ptr);

	    /* make it start from zero */
	    for(i=0;i<tablen;i++)
	      {
		view_order[i]-=min_view;
		if(sort_each_echo)
		  sview_order[i]-=min_sview;
	      }

	  }   /* end if petable */	
	else
	  multi_shot=FALSE;
	
	if(flash_converted)
	  multi_shot=FALSE;
	
	blockreps=NULL;
	blockrepeat=NULL;
	uslabsperblock=slabsperblock;

#ifdef MULTISLAB

	/* get pss array and make slab order array */
	pss=(float*)allocateWithId(slabs*sizeof(float),"recon_mm");	
	slice_order=(int*)allocateWithId(slabs*sizeof(int),"recon_mm");
	for(i=0;i<slabs;i++)
	  {
	    error=P_getreal(PROCESSED,"pss",&dtemp,i+1);
	    if(error)
	      {
		Werrprintf("recon_mm: Error getting slice offset");
		(void)recon_mmabort();
		    ABORT;
	      }
	    pss[i]=(float)dtemp;
	    slice_order[i]=i+1;
	  }

	if(slice_compressed)
	  {
	    /* count repetitions of slice positions */
	    blockreps=(int *)allocateWithId(slabsperblock*sizeof(int),"recon_mm");
	    blockrepeat=(int *)allocateWithId(slabsperblock*sizeof(int),"recon_mm");
	    pss2=(float *)allocateWithId(slabsperblock*sizeof(float),"recon_mm");
	    for(i=0;i<slabsperblock;i++)
	      {
		blockreps[i]=0;
		blockrepeat[i]=-1;
		pss2[i]=pss[i];
	      }
	    (void)qsort(pss2, slices, sizeof(float), pcompare);
	    pss0=pss2[0];
	    i=0;
	    j=0;
	    while(i<slabsperblock)
	      {
		if(pss2[i]!=pss0)
		  {
		    pss0=pss2[i];
		    j++;
		  }
		blockreps[j]++;
		i++;
	      }
	    uslabsperblock=j+1;
	  }

	/* sort the slice order based on pss */
	if(uslabsperblock==slabsperblock)
	  (void)qsort(slice_order, slices, sizeof(int), psscompare);
	else
	  {
	    upss=(float *)allocateWithId(uslabsperblock*sizeof(float),"recon_mm");
	    j=0;
	    for(i=0;i<uslabsperblock;i++)
	      {
		upss[i]=pss[j];
		j+=blockreps[i];
	      }
	    (void)qsort(slice_order, uslabsperblock, sizeof(int), upsscompare);
	  }
#endif
	/***********************************************************************************/ 
	/* get array to see how reference scan data was interleaved with acquisition */
	/***********************************************************************************/ 
	dimfirst=1;
	dimafter=1;

	if(imptr != NULL)  /* don't bother if no image */
	  {
	    /* interrogate array to find position of 'image' */	
	    imfound=FALSE;

	    ptr=strtok(arraystr,",");
	    while(ptr != NULL)
	      {
		if(ptr == imptr)
		  imfound=TRUE;
		
		/* is this a jointly arrayed thing? */
		if(strstr(ptr,"("))
		  {
		    while(strstr(ptr,")")==NULL) /* move to end of jointly arrayed list */
		      {
			if(strstr(ptr,"image"))
			  image_jnt=TRUE;
			ptr=strtok(NULL,",");
		      }
		    *(ptr+strlen(ptr)-1)='\0';
		  }
		strcpy(aname,ptr);
		error=P_getVarInfo(PROCESSED,aname,&info);
		if(error)
		  {
		    Werrprintf("recon_mm: Error getting something");
		    (void)recon_mmabort();
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
	
	error=P_getreal(PROCESSED,"lro",&dtemp,1); 
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting lro");
	    (void)recon_mmabort();
	    ABORT;
	  }
	rInfo.picInfo.fovro=dtemp;
	error=P_getreal(PROCESSED,"lpe",&dtemp,1); 
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting lpe");
	    (void)recon_mmabort();
	    ABORT;
	  }
	rInfo.picInfo.fovpe=dtemp;
	error=P_getreal(PROCESSED,"thk",&dtemp,1); 
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting thk");
	    (void)recon_mmabort();
	    ABORT;
	  }
	rInfo.picInfo.thickness=MM_TO_CM*dtemp;

	// put in slab thickness if available
	error=P_getreal(PROCESSED,"lpe2",&dtemp,1);
	if(!error)
		rInfo.picInfo.thickness=dtemp;


	rInfo.picInfo.slices=slices;	
	rInfo.picInfo.echo=1;
	rInfo.picInfo.echoes=echoes;

	error=P_getVarInfo(PROCESSED,"psi",&info); 
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting psi info");
	    (void)recon_mmabort();
	    ABORT;
	  }
	rInfo.picInfo.npsi=info.size;
	error=P_getVarInfo(PROCESSED,"phi",&info); 
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting phi info");
	    (void)recon_mmabort();
	    ABORT;
	  }
	rInfo.picInfo.nphi=info.size;
	error=P_getVarInfo(PROCESSED,"theta",&info); 
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting theta info");
	    (void)recon_mmabort();
	    ABORT;
	  }
	rInfo.picInfo.ntheta=info.size;
	
	error=P_getreal(PROCESSED,"te",&dtemp,1);
	if(error)
	  {
	    Werrprintf("recon_mm: recon_mm: Error getting te");
	    (void)recon_mmabort();
	    ABORT;
	  }
	rInfo.picInfo.te=(float)SEC_TO_MSEC*dtemp;
	error=P_getreal(PROCESSED,"tr",&dtemp,1);
	if(error)
	  {
	    Werrprintf("recon_mm: recon_mm: Error getting tr");
	    (void)recon_mmabort();
	    ABORT;
	  }
	rInfo.picInfo.tr=(float)SEC_TO_MSEC*dtemp;
	(void)strcpy(rInfo.picInfo.seqname, sequence);
	(void)strcpy(rInfo.picInfo.studyid, studyid);
	(void)strcpy(rInfo.picInfo.position1, pos1);
	(void)strcpy(rInfo.picInfo.position2, pos2);
	error=P_getreal(PROCESSED,"ti",&dtemp,1);
	if(error)
	  {
	    Werrprintf("recon_mm: recon_mm: Error getting ti");
	    (void)recon_mmabort();
	    ABORT;
	  }
	rInfo.picInfo.ti=(float)SEC_TO_MSEC*dtemp;
	rInfo.picInfo.image=dimafter*dimfirst;
	if(image_jnt)
	  rInfo.picInfo.image/=imglen;
	rInfo.picInfo.image*=(imglen-imzero);
	  
	if(!phase_compressed)
	  rInfo.picInfo.image/=views;
	if(!slice_compressed)
	  rInfo.picInfo.image/=slices;
	rInfo.picInfo.array_index=1;
	
	error=P_getreal(PROCESSED,"sfrq",&dtemp,1);
	if(error)
	  {
	    Werrprintf("recon_mm: recon_mm: Error getting sfrq");
	    (void)recon_mmabort();
	    ABORT;
	  }  
	rInfo.picInfo.sfrq=dtemp;
	
	error=P_getreal(PROCESSED,"dfrq",&dtemp,1);
	if(error)
	  {
	    Werrprintf("recon_mm: recon_mm: Error getting dfrq");
	    (void)recon_mmabort();
	    ABORT;
	  }  
	rInfo.picInfo.dfrq=dtemp;

	/* estimate time for block acquisition */
	error = P_getreal(PROCESSED,"at",&acqtime,1);
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting at");
	    (void)recon_mmabort();
	    ABORT;
	  }

	/* 	snoozetime=(useconds_t)(acqtime*ntraces*nt*SEC_TO_USEC);*/
	/*	snoozetime=(useconds_t)(0.25*rInfo.picInfo.tr*MSEC_TO_USEC); */
	
	if(views<pe_size)
	  {
	    itemp=pe_size-views;
	    views=pe_size;
	    pe_size-=itemp;
	  }
	else
	  pe_size=views;

	if(slices<pe2_size)
	  {
	    itemp=pe2_size-slices;
	    slices=pe2_size;
	    pe2_size-=itemp;
	  }
	else
	  pe2_size=slices;

	/* set up filter window if necessary */
	read_window=NULL;
	phase_window=NULL;
	recon_window=NOFILTER;  /* turn mine off */
	error=P_getreal(CURRENT,"ftproc",&dtemp,1); 
	if(error)
	  {
	    Werrprintf("recon_mm: Error getting ftproc");
	    (void)recon_mmabort();
	    ABORT;
	  }
	wtflag=(int)dtemp;

	if(wtflag)
	  {
	    recon_window=MAX_FILTER;  
	    if (init_wt1(&wtp, S_NP))
	      { 
		Werrprintf("recon_mm: Error from init_wt1");	
		(void)recon_mmabort();
		ABORT;
	      }
	    wtp.wtflag=TRUE;
	    fpointmult = getfpmult(S_NP, fid_file_head->status & S_DDR );
	    read_window=(float*)allocateWithId(nro*sizeof(float),"recon_mm");
	    if (init_wt2(&wtp, read_window, nro, FALSE, S_NP,
			 fpointmult, FALSE))
	      { 
		Werrprintf("recon_mm: Error from init_wt2");	
		(void)recon_mmabort();
		ABORT;
	      }
	    phase_window=(float*)allocateWithId(views*sizeof(float),"recon_mm");
	    if(phase_compressed)
	      {
		if (init_wt1(&wtp, S_NF))
		  { 
		    Werrprintf("recon_mm: Error from init_wt1");	
		    (void)recon_mmabort();
		    ABORT;
		  }
		wtp.wtflag=TRUE;
		fpointmult = getfpmult(S_NF,0);
		if (init_wt2(&wtp,phase_window, views, FALSE, S_NF,
			     fpointmult, FALSE))
		  { 
		    Werrprintf("recon_mm: Error from init_wt2");	
		    (void)recon_mmabort();
		    ABORT;
		  }
	      }
	    else
	      {
		if (init_wt1(&wtp, S_NI))
		  { 
		    Werrprintf("recon_mm: Error from init_wt1");	
		    (void)recon_mmabort();
		    ABORT;
		  }
		wtp.wtflag=TRUE;
		fpointmult = getfpmult(S_NI,0);
		if (init_wt2(&wtp,phase_window, views, FALSE, S_NI,
			     fpointmult, FALSE))
		  { 
		    Werrprintf("recon_mm: Error from init_wt2");	
		    (void)recon_mmabort();
		    ABORT;
		  }
	      }
	  }

	error=P_getstring(PROCESSED, "raw", raw_str, 1, SHORTSTR);  
	if(!error)
	  {
	    if(raw_str[0]=='m')
	      rawflag=RAW_MAG;
	    if(raw_str[0]=='p')
	      rawflag=RAW_PHS;
	    if(raw_str[0]=='b')
	      rawflag=RAW_MP;
	  }

	/* turn off  recon force */
	root = getTreeRoot ( "current" );
	(void)RcreateVar ( "recon_force", root, T_REAL);
	error=P_getreal(CURRENT,"recon_force",&recon_force,1);
	if(!error)
	  error=P_setreal(CURRENT,"recon_force",0.0,1);
	
	realtime_block=0;
	rInfo.image_order=0;
	rInfo.pc_slicecnt=0;
	rInfo.slicecnt=0;
	rInfo.dispcnt=0;
	
	npts=views*nro; /* number of pixels per slice */
	npts3d=npts*slices; /* number of pixels per slab */

	dsize=nmice*slabsperblock*echoes*2*npts3d;
	magnitude=(float *)allocateWithId((dsize/2)*sizeof(float),"recon_mm");
	mag2=(float *)allocateWithId((dsize/2)*sizeof(float),"recon_mm");
	if(nnav)
	  {
	    nsize=nmice*nshots*nnav*echoes*nro*2*slabsperblock;
	    nsize_ref=echoes*nro*slabsperblock;
	    navdata=(float *)allocateWithId(nsize*sizeof(float),"recon_mm");
	    nav_refphase=(double *)allocateWithId(nsize_ref*sizeof(double),"recon_mm");
	  }

	slicedata=(float *)allocateWithId(dsize*sizeof(float),"recon_mm");
	if((rawflag==RAW_MAG)||(rawflag==RAW_MP))
	  rawmag=(float *)allocateWithId((dsize/2)*sizeof(float),"recon_mm");
	if((rawflag==RAW_PHS)||(rawflag==RAW_MP))
	  rawphs=(float *)allocateWithId((dsize/2)*sizeof(float),"recon_mm");
	      
	if(pc_option!=OFF)
	  {
	    pc=(float *)allocateWithId(nmice*slices*echoes*2*npts*sizeof(float),"recon_mm");
	    pc_done=(int *)allocateWithId(nmice*slices*echoes*sizeof(int),"recon_mm");
	    for(ipc=0;ipc<slices*echoes;ipc++)
	      *(pc_done+ipc)=FALSE;
	  }

	/* bundle these for convenience */
	rInfo.picInfo.ro_size=ro_size;
	rInfo.picInfo.pe_size=pe_size;
	rInfo.picInfo.nro=nro;
	rInfo.picInfo.npe=views;
	rInfo.svinfo.ro_size=ro_size;
	rInfo.svinfo.pe_size=pe_size;
	rInfo.svinfo.slice_reps=slab_reps;
	rInfo.svinfo.nblocks=nblocks;
	rInfo.svinfo.ntraces=ntraces;
	rInfo.svinfo.dimafter=dimafter;
	rInfo.svinfo.dimfirst=dimfirst;
	rInfo.svinfo.multi_shot=multi_shot;
	rInfo.svinfo.multi_slice=multi_slice;
	rInfo.svinfo.etl=etl;
	rInfo.svinfo.uslicesperblock=uslabsperblock;
	rInfo.svinfo.slicesperblock=slabsperblock;
	rInfo.svinfo.viewsperblock=viewsperblock;
	rInfo.svinfo.within_slices=within_slices;
	rInfo.svinfo.within_views=within_views;
	rInfo.svinfo.phase_compressed=phase_compressed;
	rInfo.svinfo.slice_compressed=slice_compressed;
	rInfo.svinfo.slices=slices;
	rInfo.svinfo.echoes=echoes;
	rInfo.svinfo.epi_seq=epi_seq;
	rInfo.svinfo.flash_converted=flash_converted;
	rInfo.svinfo.pc_option=pc_option;
	rInfo.svinfo.nnav=nnav;
	rInfo.svinfo.nav_option=nav_option;
	rInfo.svinfo.nav_pts=nav_pts;
	rInfo.tbytes=fid_file_head->tbytes;
	rInfo.ebytes=fid_file_head->ebytes;
	rInfo.bbytes=fid_file_head->bbytes;
	rInfo.imglen=imglen;
	rInfo.ntlen=ntlen;
	rInfo.epi_rev=epi_rev;
	rInfo.rwindow=read_window;
	rInfo.pwindow=phase_window;
	rInfo.zeropad = zeropad;
	rInfo.zeropad2 = zeropad2;
	rInfo.ro_frq = 0.;
	rInfo.pe_frq = 0.;
	rInfo.dsize = dsize;
	rInfo.nsize = nsize;
	rInfo.rawflag = rawflag;

	/* zero everything important */
	for(i=0;i<dsize;i++)
	  *(slicedata+i)=0.0;
	if(pc_option!=OFF)
	  for(i=0;i<slices*echoes*2*npts;i++)
	    *(pc+i)=0.0;
	if(nnav)
	  {
	    for(i=0;i<nsize;i++)
	      *(navdata+i)=0.0;
	    for(i=0;i<nsize_ref;i++)
	      *(nav_refphase+i)=0.0;
	  }
	if((rawflag==RAW_MAG)||(rawflag==RAW_MP))
	  for(i=0;i<(dsize/2);i++)
	    *(rawmag+i)=0.0;
	if((rawflag==RAW_PHS)||(rawflag==RAW_MP))
	  for(i=0;i<(dsize/2);i++)
	    *(rawphs+i)=0.0;

	repeat=(int *)allocateWithId(nmice*slices*views*echoes*sizeof(int),"recon_mm");
	for(i=0;i<slices*echoes*views;i++)
	  *(repeat+i)=-1;
      }
    /******************************/
    /* end setup                            */
    /******************************/

    if(!rInfo.do_setup)
      {
	/* unpack the structure for convenience */
	nav_option=rInfo.svinfo.nav_option;
	nnav=rInfo.svinfo.nnav;
	nav_pts=rInfo.svinfo.nav_pts;
	pc_option=rInfo.svinfo.pc_option;
	slab_reps=rInfo.svinfo.slice_reps;
	nblocks=rInfo.svinfo.nblocks;
	ntraces=rInfo.svinfo.ntraces;
	dimafter=rInfo.svinfo.dimafter;
	dimfirst=rInfo.svinfo.dimfirst;
	multi_shot=rInfo.svinfo.multi_shot;
	multi_slice=rInfo.svinfo.multi_slice;
	etl=rInfo.svinfo.etl;
	slabsperblock=rInfo.svinfo.slicesperblock;
	uslabsperblock=rInfo.svinfo.uslicesperblock;
	viewsperblock=rInfo.svinfo.viewsperblock;
	within_slices=rInfo.svinfo.within_slices;
	within_views=rInfo.svinfo.within_views;
	phase_compressed=rInfo.svinfo.phase_compressed;
	slice_compressed=rInfo.svinfo.slice_compressed;
	views=rInfo.picInfo.npe;
	nro=rInfo.picInfo.nro;
	ro_size=rInfo.svinfo.ro_size;
	pe_size=rInfo.svinfo.pe_size;
	slices=rInfo.svinfo.slices;
	echoes=rInfo.svinfo.echoes;
	epi_seq=rInfo.svinfo.epi_seq;
	flash_converted=rInfo.svinfo.flash_converted;
	imglen=rInfo.imglen;
	ntlen=rInfo.ntlen;
	epi_rev=rInfo.epi_rev;
	read_window=rInfo.rwindow;
	phase_window=rInfo.pwindow;
	zeropad=rInfo.zeropad;
	zeropad2=rInfo.zeropad2;
	/*
	ro_frq=rInfo.ro_frq;
	pe_frq=rInfo.pe_frq;
	*/
	dsize=rInfo.dsize;
	rawflag=rInfo.rawflag;
      }

    if(realtime_block>=nblocks)
      {
	(void)recon_mmabort();
	ABORT;
      }

    if(ntlen)
      within_nt=nblocks/ntlen;
    else
      {
	Werrprintf("recon_mm: ntlen equal to zero");
	(void)recon_mmabort();
	ABORT;
      }			
    slice=0;
    view=0;
    echo=0;
    nshots=views/etl;
    nshots=pe_size/etl;

    if(mFidSeek(rInfo.fidMd, (realtime_block+1), sizeof(dfilehead), rInfo.bbytes))
      {
	(void)sprintf(str,"recon_mm: mFidSeek error for block %d\n",realtime_block);
	Werrprintf(str);
	(void)recon_mmabort();
	ABORT;
      }
#ifdef LINUX
    memcpy( & tmpBhp, (dblockhead *)(rInfo.fidMd->offsetAddr), sizeof(dblockhead) );
    bhp = &tmpBhp;
#else
    bhp = (dblockhead *)(rInfo.fidMd->offsetAddr);
#endif
    /* byte swap if necessary */
    DATABLOCKHEADER_CONVERT_NTOH(bhp);
    ctcount = (int) (bhp->ctcount);
    if(within_nt)
      nt=nts[realtime_block/within_nt];
    else
      {
	Werrprintf("recon_mm: within_nt equal to zero");
	(void)recon_mmabort();
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
	    Werrprintf("recon_mm: aborting by request");	
	    (void)recon_mmabort();
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
	    Werrprintf("recon_mm: within_nt equal to zero");
	    (void)recon_mmabort();
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
		(void)sprintf(str,"recon_mm: mFidSeek error for block %d\n",realtime_block);
		Werrprintf(str);
		(void)recon_mmabort();
		ABORT;
	      }
#ifdef LINUX
            memcpy( & tmpBhp, (dblockhead *)(rInfo.fidMd->offsetAddr), sizeof(dblockhead) );
            bhp = &tmpBhp;
#else
	    bhp = (dblockhead *)(rInfo.fidMd->offsetAddr);
#endif
	    /* byte swap if necessary */
	    DATABLOCKHEADER_CONVERT_NTOH(bhp);
	    ctcount = (int) (bhp->ctcount);
	  }

	if(epi_seq)
	  {
	    if(dimafter)
	      icnt=(blockctr/dimafter)%(imglen)+1;
	    else
	      {
		Werrprintf("recon_mm:dimafter equal to zero");
		(void)recon_mmabort();
		ABORT;
	      }
	    error=P_getreal(PROCESSED,"image",&rimflag,icnt);
	    if(error)
	      {
		Werrprintf("recon_mm: Error getting image element");
		(void)recon_mmabort();
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

	    if(imflag)
	      {
		*fdfstr='\0';
		if(rInfo.narray)
		  (void)arrayfdf(blockctr, rInfo.narray, rInfo.arrayelsP, fdfstr) ;
	      }

	    for(itrc=0;itrc<ntraces;itrc++) 
	      {	
		/* figure out where to put this echo */
		navflag=FALSE;
		status=svmcalc(itrc,blockctr,&(rInfo.svinfo),&view,&slice,&echo,&nav,&imouse);
		if(status)
		  {
		    (void)recon_mmabort();
		    ABORT;
		  }
		oview=view;
		oslice=slice;
		if(sort_each_echo)
		  {
		    index=blockctr/arraydim;
		    index*=ntraces;
		    index+=itrc;
		  }
		else
		  index=view;
		if(tablen)
		  index=index%tablen;
		slab=0;

		if(nav>-1)
		  navflag=TRUE;

		if(imflag&&multi_shot&&!navflag)
		  {
		    view=view_order[index];
		    if(sview_order)
		      slice=sview_order[index];
		  }
		
		if(navflag&&!imflag)
		  {
		    /* skip this echo and advance data pointer */
		    if (sdataptr)
		      sdataptr+=2*nav_pts;
		    else if(idataptr)
		      idataptr+=2*nav_pts;
		    else
		      fdataptr+=2*nav_pts;
		  }
		else if(imouse<0)
		  {
		    /* skip this echo and advance data pointer */
		    if (sdataptr)
		      sdataptr+=2*nro;
		    else if(idataptr)
		      idataptr+=2*nro;
		    else
		      fdataptr+=2*nro;
		  }
		else 
		  {
		    uslice=slice;
#ifdef MULTISLAB
		    /* which unique slice is this? */
		    if(uslabsperblock != slabsperblock)
		      {
			i=-1;
			j=-1;
			while((j<slice)&&(i<uslabsperblock))
			  {
			    i++;
			    j+=blockreps[i];
			    uslice=i;
			  }
		      }
#endif
		    /* evaluate repetition number */
		    if(imflag&&!navflag)
		      {
			rep= *(repeat+slab*views*echoes+view*echoes+echo); 
			rep++;
			rep=rep%(slab_reps); 
			*(repeat+slab*views*echoes+view*echoes+echo)=rep; 
		      }
		    		    
		    if(imflag)
		      im_slice=slab;
		    else
		      pc_slice=slab;
		    
		    /* set up pointers & offsets  */
		    if(!navflag)
		      {
			nro2=nro;
			ro2=ro_size;
			npts=views*nro2; 
			datptr=slicedata;
			if(phase_compressed&&slice_compressed)
			  {
			    soffset=imouse*slabsperblock;
			    soffset+=slab;
			    soffset*=echoes;
			    soffset+=echo;
			    soffset*=slices;
			    soffset+=slice;
			    soffset*=views;
			    soffset+=view;
			    soffset*=(2*nro);
			  }
			else
			  {
			    soffset=imouse*slabsperblock;
			    soffset+=slab;
			    soffset*=echoes;
			    soffset+=echo;
			    soffset*=slices;
			    soffset+=slice;
			    soffset*=views;
			    soffset+=view;
			    soffset*=(2*nro);
			    soffset=imouse*slabsperblock*echoes*2*npts3d + slab*echoes*2*npts3d +
			      echo*2*npts3d + slice*2*npts + view*2*nro;
			  }
			/* soffset=view*2*nro+echo*npts*2+rep*echoes*npts*2+slice*slab_reps*echoes*2*npts;  */
		      }
		    else
		      {
			nro2=nro;
			ro2=nav_pts;
			datptr=navdata;
			if(phase_compressed)
			  soffset=nav*2*nro2+echo*nro2*nshots*nnav*2+
			    (slice%slabsperblock)*echoes*nro2*nshots*nnav*2;          
			else
			  soffset=nav*2*nro2+echo*nshots*nnav*nro2*2+rep*echoes*nnav*nshots*nro2*2+
			    slice*slab_reps*echoes*nnav*nshots*2*nro2; 
		      }

		    /* scale and convert to float */
		    soffset+=(nro2-ro2);
		    if (sdataptr)
		      for(iro=0;iro<2*ro2;iro++)
			{
			  s1=ntohs(*sdataptr++);
			  *(datptr+soffset+iro)=IMAGE_SCALE*s1;
			}
		    else if(idataptr)
		      for(iro=0;iro<2*ro2;iro++)
			{
			  l1=ntohl(*idataptr++);
			  *(datptr+soffset+iro)=IMAGE_SCALE*l1;
			}
		    else /* data is float */
		      for(iro=0;iro<2*ro2;iro++)
			{
			  memcpy(&l1, fdataptr, sizeof(float));
			  fdataptr++;
			  l1=ntohl(l1);
			  memcpy(&ftemp,&l1, sizeof(float));
			  *(datptr+soffset+iro)=IMAGE_SCALE*ftemp;
			}
		    
		    if(rInfo.dc_flag)
		      {
			for(iro=0;iro<2*ro2;iro++)
			  {
			    if(iro%2)
			      *(datptr+soffset+iro) -= imag_dc;
			    else
			      *(datptr+soffset+iro) -= real_dc;
			  }
		      }
		    soffset-=(nro2-ro2);
		    
		    /* time reverse data */	
		    if(oview%2 && epi_rev) 
		      {
			pt1=datptr+soffset;
			pt2=pt1+1;
			pt3=pt1+2*(nro2-1);
			pt4=pt3+1;
			for(iro=0;iro<nro2/2;iro++)
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
		    
		    if(rawflag&&imflag&&(!navflag))
		      {
			switch(rawflag)
			  {
			  case RAW_MAG:
			    for(iro=0;iro<ro2;iro++)				
		      {
				m1=(double)*(datptr+soffset+2*iro);
				m2=(double)*(datptr+soffset+2*iro+1);
				dtemp=sqrt(m1*m1+m2*m2);
				rawmag[(soffset/2)+iro]=(float)dtemp;
			      }
			    break;
			  case RAW_PHS:
			    for(iro=0;iro<ro2;iro++)			
			      {
				m1=(double)*(datptr+soffset+2*iro);
				m2=(double)*(datptr+soffset+2*iro+1);
				dtemp=atan2(m2,m1);
				rawphs[(soffset/2)+iro]=(float)dtemp;
			      }
			    break;			    
			  case RAW_MP:
			    for(iro=0;iro<ro2;iro++)			
			      {
				m1=(double)*(datptr+soffset+2*iro);
				m2=(double)*(datptr+soffset+2*iro+1);
				dtemp=sqrt(m1*m1+m2*m2);
				rawmag[(soffset/2)+iro]=(float)dtemp;
				dtemp=atan2(m2,m1);
				rawphs[(soffset/2)+iro]=(float)dtemp;
			      }
			    break;			    
			  default:
			    break;
			  }
		      }
		    
		    if((recon_window>NOFILTER)&&(recon_window<=MAX_FILTER))
		      {
			pt1=datptr+soffset;
			wptr=read_window;
			for(iro=0;iro<nro2;iro++)
			  {
			    *pt1 *=  *wptr;
			    pt1++;
			    *pt1 *=  *wptr;
			    pt1++;
			    wptr++;
			  }		  
		      }
		    
		    halfF_ro=FALSE;
		    if(zeropad > HALFF*nro2)
		      halfF_ro=TRUE;

		    if(!halfF_ro)
		      {
			/* read direction ft */
			nrptr=datptr+soffset;

			/* apply frequency shift */
			if(ro_frq!=NULL)
			  {
			    frq=*(ro_frq+imouse);
			    if(strstr(rInfo.picInfo.seqname,"mems")&&echo%2)
			      (void)rotate_fid(nrptr, 0.0, -1*frq, 2*nro2, COMPLEX);
			    else
			      (void)rotate_fid(nrptr, 0.0, frq, 2*nro2, COMPLEX);
			  }

			level = nro2/(2*nro2);
			if (level > 0)
			  {
			    i = 2;
			    while (i <= level)
			      i *= 2;
			    level= i/2;
			  }
			else
			  level = 0;
			
			level=0;
			pwr = 4;
			fnt = 32;
			while (fnt < 2*nro2)
			  {
			    fnt *= 2;
			    pwr++;
			  }
			(void)fft(nrptr, nro2,pwr, level, COMPLEX,COMPLEX,-1.0,1.0,nro2);  
		      }
		    
		    /* phase correction application */
		    if(!navflag&&imflag&&(pc_option!=OFF))
		      {	
			if((pc_option!=OFF)&&(*(pc_done+im_slice)))
			  {
			    /* pc_offset=im_slice*echoes*npts*2+echo*npts*2+view*nro*2; */
			    pc_offset=im_slice*echoes*npts*2+echo*npts*2+oview*nro*2;
			    pt1=datptr+soffset;
			    pt2=pt1+1;
			    pt3=pc+pc_offset;
			    pt4=pt3+1;
			    pt5=pt1;
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
		  }  /* end if image or not navigator */
	      }	/* end of loop on traces */
		
	    /* compute phase correction */
	    if((!imflag)&&(pc_option!=OFF))
	      {
		if(phase_compressed)
		  {
		    fptr=slicedata;
		    if(multi_shot&&((pc_option==CENTER_PAIR)||(pc_option==FIRST_PAIR)))
		      {
			for (ispb=0;ispb<slabsperblock*echoes;ispb++)
			  {
			    *(pc_done+rInfo.pc_slicecnt)=TRUE;
			    pc_offset=rInfo.pc_slicecnt*npts*2;
			    /* 	pc_offset+=(views-pe_size)*nro2; */
			    for(it=0;it<(pe_size/etl);it++)
			      {
				(void)pc_calc(fptr, (pc+pc_offset),
					      nro2, etl, pc_option, transposed);
				pc_offset += 2*nro2*etl;
			      }
			    rInfo.pc_slicecnt++;
			    rInfo.pc_slicecnt=rInfo.pc_slicecnt%(slices*echoes);
			    fptr+=2*npts;
			  }
		      }
		    else
		      {
			for (ispb=0;ispb<slabsperblock*echoes;ispb++)
			  {
			    *(pc_done+rInfo.pc_slicecnt)=TRUE;
			    pc_offset=rInfo.pc_slicecnt*npts*2;
			    /* 	pc_offset+=(views-pe_size)*nro2; */
			    (void)pc_calc(fptr, (pc+pc_offset),
					  nro2, pe_size, pc_option, transposed);
			    rInfo.pc_slicecnt++;
			    rInfo.pc_slicecnt=rInfo.pc_slicecnt%(slices*echoes);
			    fptr+=2*npts;
			  }
		      }
		  }
	      }	
	  }            /* end if imflag or doing phase correction */
	
	/* see if recon has been forced */
	error=P_getreal(CURRENT,"recon_force",&recon_force,1);
	if(!error) 
	  {
	    if(recon_force>0.0) 
	      {
		error=P_setreal(CURRENT,"recon_force",0.0,1);  /* turn it back off */
		if(!phase_compressed)
		  (void)generate_images(slices, views, nro, slab_reps, rInfo.nchannels,
					phase_window, &(rInfo.picInfo), rInfo.image_order, 
					multi_shot, &(rInfo.dispcnt), rInfo.dispint, zeropad, zeropad2, fdfstr,pe_frq, pe2_frq);		
	      }
	  }
      }   	  
    /**************************/
    /* end loop on blocks    */
    /**************************/
 
    if(acq_done)
      {
	*fdfstr='\0';
	(void)generate_images(slices, views, nro, nmice, slabsperblock, phase_window, &(rInfo.picInfo), 
			      rInfo.image_order, multi_shot, &(rInfo.dispcnt), rInfo.dispint, zeropad, zeropad2,fdfstr, pe_frq, pe2_frq);
	
#ifdef VNMRJ		
	(void)aipUpdateRQ();
#endif
	(void)releaseAllWithId("recon_mm");
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
static void generate_images(int slices, int views, int nro, int nmice,
           int slabs, float *window, fdfInfo *pInfo, int image_order,
           int multi_shot, int *dispcntptr, int dispint, int zeropad,
			    int zeropad2, char *arstr,double *pe_frq, double *pe2_frq)
{
  float *fptr, *mag3D, *fp2, *fp3;
  int blkreps;
  int moffset;
  int magoffset;
  int slice;
  int irep;
  int imouse, islab;
  int npts2d;
  int npts3d;
  int echo;
  int phase_reverse;
  int display;
  int status;
  int islice, iview, iro;
 
  fptr=slicedata;
  blkreps=FALSE;
  moffset=0;
  magoffset=0;
  npts2d=views*nro;
  npts3d=npts2d*slices;
  mag3D=(float *)allocateWithId(npts3d*sizeof(float),"recon_mm");    
	
  for(imouse=0;imouse<nmice;imouse++)
    {
      for(islab=0;islab<slabs;islab++)
	{
	  for(echo=0;echo<pInfo->echoes;echo++)
	    {
	      phase_reverse=FALSE;
	      if(strstr(pInfo->seqname,"mems")&&echo%2)
		phase_reverse=TRUE;
	      /*
	      (void)phaseslice_ft(fptr,nro,views,slices,window,magnitude+magoffset,phase_reverse,zeropad,zeropad2,imouse, NULL, pe_frq,  pe2_frq);
	      */
	      (void)phaseslice_ft(fptr,nro,views,slices,window,magnitude,phase_reverse,zeropad,zeropad2,imouse, NULL,pe_frq,  pe2_frq);
	      fptr+=2*npts3d;
	      magoffset+=npts3d;
	      moffset=0;
	      /* write 2D images */
	      for(slice=0;slice<0*slices;slice++)
		{
		  pInfo->echo=echo+1;
		  pInfo->slice=slice+1;
		  display=DISPFLAG(*dispcntptr, dispint);
		  (*dispcntptr)++;
		  
		  /* irep=rep*ibr + urep +1; */
		  irep=imouse + 1;
		  /* 		  status=write_fdf(irep,magnitude+moffset, pInfo, &image_order, display, arstr, islab+1, ch0); */
		  status=write_fdf(irep,magnitude+moffset, pInfo, &image_order, display, arstr, islab+1, imouse+1 );
		  if(status)
		    {
		      (void)recon_mmabort();
		      return;	
		    }
		  moffset+=npts2d;
		}
		/* write 3D FDF file */
		rInfo.picInfo.echoes=pInfo->echoes;
  		rInfo.picInfo.datatype=MAGNITUDE;
  		fp3=magnitude;
		rInfo.picInfo.slice=1;
    	rInfo.picInfo.image=imouse+1;
    	rInfo.picInfo.echo=echo+1;
	  	fp2=mag3D;
    	for(islice=0;islice<slices;islice++)
	    {
	    fp2+=(npts2d-1);
	    for(iview=0;iview<views;iview++)
		{
		  for(iro=0;iro<nro;iro++)
		    *fp2--=*(fp3+iro*views+iview);
		}
	      fp3+=npts2d;	
	      fp2+=(npts2d+1);
	    }						    	 		
	  	status=write_3Dfdf(mag3D, &(rInfo.picInfo), arstr, 0);	  	  
	  	if(status)
	    	{
	      		(void)recon_mmabort();
	      		return;	
	    	}	    	     	
	   }
	}
	
    }
  return;
}



/********************************************************************************************************/
/* phaseslice_ft is also responsible for reversing magnitude data in read & phase directions (new fft)  */
/********************************************************************************************************/
int phaseslice_ft(xkydata,nx,ny,nz,win,absdata, phase_rev, zeropad_ro, zeropad_ph, imouse, phsdata,pe_frq, frq_pe2)
     float *xkydata;
     float *absdata;
     float *win;
     int nx,ny,nz,phase_rev,zeropad_ro,zeropad_ph;
     int imouse;
     float *phsdata;
     double *pe_frq;
     double *frq_pe2;
{
  float a,b;
  float *fptr,*pt1,*pt2;
  int ix,iy,iz;
  int np;
  int pwr,fnt;
  int halfR=FALSE;
  float *nrptr;
  float *pptr;
  float templine[2*MAXPE];
  float *tempdat = NULL;
  double frq;

  fptr=absdata;
  pptr=phsdata;

  np=nx*ny;


  if(zeropad_ro > HALFF*nx)
    {
      halfR=TRUE;
      tempdat=(float *)allocateWithId(2*np*sizeof(float),"phaseslice_ft");
    }


  /* do slice direction ft */
  pwr = 4;
  fnt = 32;
  while (fnt < 2*nz)
    {
      fnt *= 2;
      pwr++;
    }
  for(iy=0;iy<ny;iy++)
    {
      for(ix=0;ix<nx;ix++)
	{
	  /* get the kz line */
	  pt1=xkydata+2*ix + 2*nx*iy;
	  pt2=pt1+1;
	  nrptr=templine;
	  for(iz=0;iz<nz;iz++)
	    {
	      *nrptr++=*pt1;
	      *nrptr++=*pt2;
	      pt1+=2*np;	
	      pt2=pt1+1;
	    }

	  /* apply frequency shift */
	  if(frq_pe2!=NULL)
	    {
	      frq=*(frq_pe2+imouse);
	      (void)rotate_fid(templine, 0.0, frq, 2*nz, COMPLEX);
	    }

	  nrptr=templine;
	  (void)fft(nrptr, nz,pwr, 0, COMPLEX,COMPLEX,-1.0,1.0,nz);   
	  /* write it back */
	  pt1=xkydata+2*ix + 2*nx*iy;
	  pt2=pt1+1;
	  nrptr=templine;
	  for(iz=0;iz<nz;iz++)
	    {
	      *pt1=*nrptr++;
	      *pt2=*nrptr++;
	      pt1+=2*np;	
	      pt2=pt1+1;
	    }
	}
    }


  /* do phase direction ft */
  pwr = 4;
  fnt = 32;
  while (fnt < 2*ny)
    {
      fnt *= 2;
      pwr++;
    }
  for(iz=0;iz<nz;iz++)
    {
      for(ix=nx-1;ix>-1;ix--)
	{
	  /* get the ky line */
	  pt1=xkydata+2*ix;
	  pt1=xkydata+2*ix + 2*np*iz;
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
	  
#ifdef HALFF	  
	    /* perform half Fourier data estimation */
	  if(zeropad_ph > HALFF*ny)
	    (void)halfFourier(templine, (ny-zeropad_ph), ny, POINTWISE);
#endif
	  
	  /* apply frequency shift */
	  if(pe_frq!=NULL)
	    {
	      frq=*(pe_frq+imouse); 

	      if(phase_rev)
		(void)rotate_fid(templine, 0.0, -1*frq, 2*ny, COMPLEX);
	      else
		(void)rotate_fid(templine, 0.0, frq, 2*ny, COMPLEX);
	    }
	  
	  nrptr=templine;
	  (void)fft(nrptr, ny,pwr, 0, COMPLEX,COMPLEX,-1.0,1.0,ny);  
      
	  /* write it back */
	  pt1=xkydata+2*ix + 2*np*iz;
	  pt2=pt1+1;
	  nrptr=templine;
	  for(iy=0;iy<ny;iy++)
	    {
	      *pt1=*nrptr++;
	      *pt2=*nrptr++;
	      pt1+=2*nx;	
	      pt2=pt1+1;
	    }
	  
	  /*  write magnitude */
	  if(!halfR)
	    {
	      if(!phase_rev)
		{
		  nrptr=templine+2*ny-1;
		  for(iy=0;iy<ny;iy++)
		    {
		      a=*nrptr--;
		      b=*nrptr--;
		      *fptr++=(float)sqrt((double)(a*a+b*b));
		      if(phsdata)
			*pptr++=(float)atan2(b,a);
		    }
		}
	      else
		{
		  nrptr=templine;
		  for(iy=0;iy<ny;iy++)
		    {
		      a=*nrptr++;
		      b=*nrptr++;
		      *fptr++=(float)sqrt((double)(a*a+b*b));
		      if(phsdata)
			*pptr++=(float)atan2(b,a);
		    }
		}
	    }
	}
    }
  
#ifdef HALFF
  if(halfR)
    {
      /* perform half Fourier in readout */
      nrptr=tempdat;
      for(iy=0;iy<ny;iy++)
	{
	  (void)halfFourier(nrptr,(nx-zeropad_ro), nx, POINTWISE);
	  pwr = 4;
	  fnt = 32;
	  while (fnt < 2*ny)
	    {
	      fnt *= 2;
	      pwr++;
	    }
	  /* apply frequency shift */
	  /*
	  if(ro_frq!=NULL)
	    (void)rotate_fid(nrptr, 0.0, ro_frq, 2*nx, COMPLEX);
	  (void)fft(nrptr, nx,pwr, 0, COMPLEX,COMPLEX,-1.0,1.0,nx);  
	  */
	  nrptr+=2*nx;
	}
      /* write out magnitude */
      if(!phase_rev)
	{
	  for(ix=nx-1;ix>-1;ix--)
	    {
	      for(iy=ny-1;iy>-1;iy--)
		{
		  nrptr=tempdat+2*ix+iy*2*nx;
		  a=*nrptr++;
		  b=*nrptr++;
		  *fptr++=(float)sqrt((double)(a*a+b*b));
		  if(phsdata)
		    *pptr++=(float)atan2(b,a);
		}
	    }
	}
      else
	{
	  for(ix=nx-1;ix>-1;ix--)
	    {
	      for(iy=0;iy<ny;iy++)
		{
		  nrptr=tempdat+2*ix+iy*2*nx;
		  a=*nrptr++;
		  b=*nrptr++;
		  *fptr++=(float)sqrt((double)(a*a+b*b));
		  if(phsdata)
		    *pptr++=(float)atan2(b,a);
		}
	    }
	}
    }
#endif
  
  (void)releaseAllWithId("phaseslice_ft");
      
  return(0);
}

#ifdef XXX
/******************************************************************************/
/******************************************************************************/
static void filter_window(window_type,window, npts)
     int window_type;
     float *window;
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
#endif

/******************************************************************************/
/******************************************************************************/
void recon_mmabort()
{
  if(rInfo.fidMd)
    {
      mClose(rInfo.fidMd);
      rInfo.fidMd=NULL;
    }
  (void)releaseAllWithId("recon_mm");
  return;
}
   
#ifdef MULTISLAB
/*****************************************************************/
static  int psscompare(const void *p1, const void *p2)
{
  int i= *((int *)p1);
  int j= *((int *)p2);

  i=i-1;
  j=j-1;

  if(pss[i]>pss[j])
    return(1);
  else if(pss[i]<pss[j])
    return(-1);
  else
    return(0);
}
/*****************************************************************/
static  int pcompare(const void *p1, const void *p2)
{
  float f1= *((float *)p1);
  float f2= *((float *)p2);

  if(f1>f2)
    return(1);
  else if(f1<f2)
    return(-1);
  else
    return(0);
}
#endif

/* returns slice and view number given trace and block number */
/* modified for 3D multi mouse */
int svmcalc(trace,block,svI,view,slice, echo, nav, mouse)
     int trace, block;
     svInfo *svI; 
     int *view;
     int *slice;
     int *echo;
     int *nav;
     int *mouse;
{
  int v,s;
  int d;
  int i;
  int m;
  int nav1;
  int nec, ec;
  int netl;
  int n;
  int nnavs;
  int navflag;

  netl=svI->etl;
  nnavs=(svI->pe_size)/(netl);
  nnavs*=(svI->nnav);
  nec=0;
  *nav=-1;
  navflag=FALSE;

  /* which mouse, if any? */
  m=block%rInfo.nchannels;
  m+=(trace%miceperchannel)*rInfo.nchannels;
  /* modify these to look like one mouse per channel */
  trace/=miceperchannel;
  netl/=miceperchannel;

  if(m>=nmice)
    *mouse=-1;
  else
    {
      if(svI->nnav)
	{
	  netl+=svI->nnav;
	  /* figure out how many navigators preceed this */
	  d=(netl);
	  if(d)
	    ec=(trace%d);
	  else
	    {
	      Werrprintf("recon_mm:svmcalc: divide by zero");
	      (void)recon_abort();
	      ABORT;
	    }
	  i=0;
	  while((i<svI->nnav)&&(!navflag))
	    {
	      if(nav_list[i]<ec)
		nec++;
	      else if(nav_list[i]==ec)
		{
		  nav1=i;
		  navflag=TRUE;
		}
	      i++;
	    }
	  if(navflag)
	    {
	      /* kludge to make view even or odd for echo reversal in epi */
	      if((nav_list[0]==0)&&((svI->nnav)%2))
		nec=-1;
	      else
		nec=0;
	      
	      if(svI->multi_slice)
		{
		  d=(netl)*(svI->slicesperblock)*(svI->echoes); 
		  if(d)
		    n=trace/d;  /* segment */
		  else
		    {
		      Werrprintf("recon_mm:svmcalc: divide by zero");
		  (void)recon_abort();
		  ABORT;
		    }			
		  n*=svI->nnav;     /* times navigators per segment */
		  /*	      n+=ec;  */                         /* plus echo  */	      
		  n+=nav1;                         /* plus echo  */	      
		  *nav=n;
		}
	      else
		*nav=trace;
	      *nav=(*nav)%(nnavs);
	    }
	}
      
      if(svI->multi_shot||svI->epi_seq)
	{
	  if(svI->phase_compressed)
	    {
	      if(svI->multi_slice)
		{
		  d=(netl)*(svI->slicesperblock)*(svI->echoes); 
		  if(d)
		    v=trace/d;  /* segment */
		  else
		    {
		      Werrprintf("recon_mm:svmcalc: divide by zero");
		      (void)recon_abort();
		      ABORT;
		    }			
		  v*=svI->etl;                                                 /* times etl */
		  v+=(trace%(netl));                         /* plus echo  */
		  v=v-nec;
		}
	      else
		v=trace-nec;
	    }
	  else
	    {
	      d=svI->within_views;
	      if(d)
		v=(block/d);
	      else
		{
		  Werrprintf("recon_mm:svmcalc: divide by zero");
		  (void)recon_abort();
		  ABORT;
		}			
	      v*=svI->etl;                                                 /* times etl */
	      v+=(trace%(netl));                         /* plus echo  */
	      v=v-nec;
	    }
	  if(svI->slice_compressed)
	    {
	      if(svI->multi_slice)
		{
		  d=netl;
		  if(d)
		    s=trace/d;
		  else
		    {
		      Werrprintf("recon_mm:svmcalc: divide by zero");
		      (void)recon_abort();
		      ABORT;
		    }			
		}
	      else  
		{
		  d=svI->viewsperblock; 
		  if(d)
		    s=trace/d;
		  else
		    {
		      Werrprintf("recon_mm:svmcalc: divide by zero");
		      (void)recon_abort();
		      ABORT;
		    }			
		}
	    }
	  else
	    {
	      d=svI->within_slices;
	      if(d)
		s=(block/d);
	      else
		{
		  Werrprintf("recon_mm:svmcalc: divide by zero");
		  (void)recon_abort();
		  ABORT;
		}			
	    }
	  
	}
      else   /* not multi_shot */
	{
	  if(svI->phase_compressed)
	    {
	      if(svI->multi_slice)
		d=(svI->slicesperblock)*(svI->echoes);
	      else
		d=(svI->echoes);
	      if(d)
		v=trace/d;
	      else
		{
		  Werrprintf("recon_mm:svmcalc: divide by zero");
		  (void)recon_abort();
		  ABORT;
		}			
	    }
	  else
	    {
	      d=(svI->within_views);
	      if(d)
		v=(block/d);
	      else
		{
		  Werrprintf("recon_mm:svmcalc: divide by zero");
		  (void)recon_abort();
		  ABORT;
		}			
	    }
	  if(svI->slice_compressed)
	    {
	      if(svI->multi_slice)
		d=(svI->echoes);
	      else  
		d=(svI->viewsperblock)*(svI->echoes); 
	      if(d)
		s=trace/d;
	      else
		{
		  Werrprintf("recon_mm:svmcalc: divide by zero");
		  (void)recon_abort();
		  ABORT;
		}			
	    }
	  else
	    {
	      d=(svI->within_slices);
	      if(d)
		s=(block/d);
	      else
		{
		  Werrprintf("recon_mm:svmcalc: divide by zero");
		  (void)recon_abort();
		  ABORT;
		}			
	    }
	}
      *view=v%(svI->pe_size);
      *slice=s%(svI->slices);
      if(svI->epi_seq)
	{
	  d=(netl)*(svI->slicesperblock);
	  if(d)
	    *echo=trace/d;
	  else
	    {
	      Werrprintf("recon_mm:svmcalc: divide by zero");
	      (void)recon_abort();
	      ABORT;
	}			
	  *echo=(*echo)%(svI->echoes);
	}
      else
	*echo=trace%(svI->echoes);

      *mouse=m;
    } /* end if actual mouse */

  return(0);
}     
