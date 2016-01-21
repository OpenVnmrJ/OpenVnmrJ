/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*****************************************************************************
 * File recon3D.c:  
 *
 * main  reconstruction for large 3D acquisitions (fid data --> images)
 * stolen from recon_all.c 9/2/2008   M.R. Kritzer
 * 
 ******************************************************************************/

#define _FILE_OFFSET_BITS 64


#include <math.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <inttypes.h>

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
#include "vfilesys.h"
#include "allocate.h"
#include "pvars.h"
#include "wjunk.h"
#include "buttons.h"
#include "init2d.h"

#ifdef VNMRJ
#include "aipCInterface.h"
#endif 


extern int interuption;
extern void Vperror(char *);
extern int specIndex;
extern double getfpmult(int fdimname, int ddr);
extern int unlinkFilesWithSuffix(char *dirpath, char *suffix);

static float *slicedata, *pc, *magnitude, *mag2;
static float *slicedata_neg, *pc_neg;
static float *imdata, *imdata2, *imdata_neg, *imdata2_neg;
static float *rawmag, *rawphs, *imgphs, *imgphs2;
static float *navdata;
static double *nav_refphase;
static int *view_order, *nav_list;
static int *sview_order;
static int *slice_order;
static int *blockreps;
static int *blockrepeat;
static int *repeat;
static int *nts;
static short *pc_done, *pcneg_done, *pcd;
static reconInfo rInfo;
static int realtime_block;
static int recon3D_aborted;
static FILE *temp3D_file; 

static double *ro_frqP, *pe_frqP;

int nmice;
float *pss;
float *upss;

/* prototype */
void recon3D_abort();
char *recon3Dallocate(int, char *);
extern int write_fdf(int, float *, fdfInfo *, int *, int, char *, int, int);
extern int write_3Dfdf(float *, fdfInfo *, char *, int);
extern int svcalc(int, int, svInfo *, int *, int *, int *, int *, int *);
extern int psscompare(const void *, const void *);
extern int upsscompare(const void *, const void *);
extern int pcompare(const void *, const void *);
static int generate_imagesBig3D(char *arstr);
static void temp_ft(float *xkydata, int nx, int ny, float *win,
        float *absdata, int phase_rev, int zeropad_ro, int zeropad_ph,
        float ro_frq, float ph_frq, float *phsdata, int flag, int blk);
static void filter_window(int window_type, float *window, int npts);
#ifdef XXX
static void fftshift(float *data, int npts);
#endif
static void nav_correct(int method, float *data, double *ref_phase, int nro,
        int npe, float *navdata, svInfo *svI);

/******************************************************************************/
/******************************************************************************/


/*******************************************************************************/
/*******************************************************************************/
/*            recon3D                                                        */
/*******************************************************************************/
/*******************************************************************************/

/* 

 Purpose:
 -------
 Routine recon3D is the program for epi reconstruction.  

 Arguments:
 ---------
 argc  :  (   )  Argument count.
 argv  :  (   )  Command line arguments.
 retc  :  (   )  Return argument count.  Not used here.
 retv  :  (   )  Return arguments.  Not used here

 */
int recon3D(int argc, char *argv[], int retc, char *retv[]) {
    /*   Local Variables: */
    FILE *f1;
    FILE *table_ptr;

    char filepath[MAXPATHL];
    char tablefile[MAXPATHL];
    char tempdir[MAXPATHL];
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
    char tmp_str[SHORTSTR];
    char dcrmv[SHORTSTR];
    char profstr[SHORTSTR];
    char arraystr[MAXSTR];
    char *ptr;
    char *imptr;
    char str[MAXSTR];
    short *sdataptr;
    vInfo info;

    double fpointmult;
    double rnseg, rfc, retl, rnv, rnv2;
    double recon_force;
    double rimflag;
    double dt2;
    double rechoes, repi_rev;
    double rslices, rnf;
    double dtemp;
    double m1, m2;
    double acqtime;

    dpointers inblock;
    dblockhead *bhp;
    dfilehead *fid_file_head;
    dfilehead ffh;
    
    float *cblockdata;
    float pss0;
    float *pss2;
    float *fptr, *nrptr;
    float *datptr;
    float *wptr;
    float *read_window;
    float *phase_window;
    float *nav_window;
    float *fdataptr;
    float *pc_temp;
    float real_dc, imag_dc;
    double ro_frq, pe_frq;
    double ro_ph, pe_ph;
    float a, b, c, d;
    float t1, t2, *pt1, *pt2, *pt3, *pt4, *pt5, *pt6;

    int floatstatus;
    int status;
    int echoes;
    int tstch;
    int slab;
    int *idataptr;
    int imglen, dimfirst, dimafter, imfound;
    int imzero, imneg1, imneg2;
    int icnt;
    int nv;
    int uslice;
    int ispb, itrc, it;
    int ip;
    int iecho;
    int ndir2;
    int blockctr;
    int pc_offset;
    int soffset;
    int tablen, itablen;
    int magoffset;
    int roffset;
    int iro;
    int ipc;
    int ntlen;
    int phase_correct;
    int nt;
    int nshifts;
    int within_nt;
    int dsize, nsize;
    int views, nro;
    int nro2;
    int ro_size, pe_size;
    int pe2_size;
    int ro2;
    int slices, etl, nblocks, ntraces, slice_reps;
    int slab_reps;
    int slicesperblock, viewsperblock;
    int sviewsperblock;
    int slabsperblock=1;
    int uslicesperblock;
    int error, error1;
    int npts, npts3d;
    int im_slice;
    int pc_slice;
    int itab;
    int ichan;
    int view, slice, echo;
    int oview;
    int coilfactor;
    int nchannels=1;
    int slabs=1;
    int within_slabs=1;
    int within_sviews=1;
    int within_views=1;
    int within_slices=1;
    int rep;
    int i, min_view, min_view2, iview;
    int pcslice;
    int min_sview;
    int j;
    int narg;
    int zeropad=0;
    int zeropad2=0;
    int n_ft;
    int ctcount;
    int pwr, level;
    int fnt;
    int nshots;
    int nsize_ref;
    int nnav, nav_pts;
    int nav;
    int fn, fn1, fn2;
    int wtflag;
    int itemp;
  
    /*    useconds_t snoozetime; */
    struct wtparams wtp;

    /* flags */
    int threeD=FALSE;
    int sview_zf=FALSE;
    int options=FALSE;
    int pc_option=OFF;
    int epi_dualref=FALSE;
    int nav_option=OFF;
    int recon_window=NOFILTER;
    int done=FALSE;
    int imflag=TRUE;
    int imflag_neg=FALSE;
    int pcflag_neg=FALSE;
    int pcflag=FALSE;
    int multi_shot=FALSE;
    int transposed=FALSE;
    int epi_seq=FALSE;
    int multi_slice=FALSE;
    int epi_rev=TRUE;
    int reverse=TRUE;
    int acq_done=FALSE;
    int sliceenc_compressed=TRUE;
    int slice_compressed=TRUE;
    int slab_compressed=TRUE;
    int phase_compressed=TRUE; /* a good assumption  */
    int flash_converted=FALSE;
    int tab_converted=FALSE;
    int epi_me=FALSE;
    int phase_reverse=FALSE;
    int navflag=FALSE;
    int rawflag=FALSE;
    int sense=FALSE;
    int smash=FALSE;
    int phsflag=FALSE;
    int halfF_ro=FALSE;
    int image_jnt=FALSE;
    int lastchan=FALSE;
    int reallybig=FALSE;
    
    symbol **root;

    /*********************/
    /* executable code */
    /*********************/

    /* default :  do complete setup and mallocs */
    rInfo.do_setup=TRUE;

    fid_file_head=&ffh;
    
    /* look for command line arguments */
    narg=1;
    if (argc>narg) /* first argument is acq or a dummy string*/
    {
        if (strcmp(argv[narg], "acq")==0) /* not first time through!!! */
            rInfo.do_setup=FALSE;
        narg++;
    }
    if (rInfo.do_setup) {
        rInfo.fidMd=NULL;
        sview_order=NULL;
        view_order=NULL;
        recon3D_aborted=FALSE;
    }

    if (interuption || recon3D_aborted) {
        error=P_setstring(CURRENT, "wnt", "", 0);
        error=P_setstring(PROCESSED, "wnt", "", 0);
        Werrprintf("recon3D: ABORTING ");
        (void)recon3D_abort();
        ABORT;
    }

    /* what sequence is this ? */
    error=P_getstring(PROCESSED, "seqfil", sequence, 1, MAXSTR);
    if (error) {
        Werrprintf("recon3D: Error getting seqfil");
        (void)recon3D_abort();
        ABORT;
    }

    /* get studyid */
    error=P_getstring(PROCESSED, "studyid_", studyid, 1, MAXSTR);
    if (error)
        (void)strcpy(studyid, "unknown");

    /* get patient positions */
    error=P_getstring(PROCESSED, "position1", pos1, 1, MAXSTR);
    if (error)
        (void)strcpy(pos1, "");
    error=P_getstring(PROCESSED, "position2", pos2, 1, MAXSTR);
    if (error)
        (void)strcpy(pos2, "");

    /* get apptype */
    error=P_getstring(PROCESSED, "apptype", apptype, 1, MAXSTR);
    /*
     if(error)
     {
     Werrprintf("recon3D: Error getting apptype");	
     (void)recon3D_abort();
     ABORT;
     }		
     if(!strlen(apptype))
     {
     Winfoprintf("recon3D: WARNING:apptype unknown!");	
     Winfoprintf("recon3D: Set apptype in processed tree");	
     }
     */
    if (!error) {
        if (strstr(apptype, "imEPI"))
            epi_seq=TRUE;
    }

    error=P_getstring(CURRENT,"epi_pc", epi_pc, 1, MAXSTR);
    if (!error) {
        epi_seq=TRUE;
        epi_rev=TRUE;
        pc_option=pc_pick(epi_pc);
        if ((pc_option>MAX_OPTION)||(pc_option<OFF)) {
            Werrprintf("recon3D: Invalid phase correction option in epi_pc");
            (void)recon3D_abort();
            ABORT;
        }
    }

    if (epi_seq)
        pc_option=POINTWISE;

    /* further arguments for phase correction & image directory */
    if (argc>narg) {
        options=TRUE;
        pc_option=pc_pick(argv[narg]);
        if ((pc_option>MAX_OPTION)||(pc_option<OFF)) {
            (void)strcpy(rInfo.picInfo.imdir, argv[narg++]);
            rInfo.picInfo.fullpath=TRUE;
            if (argc>narg) {
                if (epi_seq) {
                    (void)strcpy(epi_pc, argv[narg]);
                    pc_option=pc_pick(epi_pc);
                    if ((pc_option>MAX_OPTION)||(pc_option<OFF)) {
                        Werrprintf("recon3D: Invalid phase correction option in command line");
                        (void)recon3D_abort();
                        ABORT;
                    }
                }
            }
        } else {
            if (epi_seq) {
                (void)strcpy(epi_pc, argv[narg]);
                pc_option=pc_pick(epi_pc);
                if ((pc_option>MAX_OPTION)||(pc_option<OFF)) {
                    Werrprintf("recon3D: Invalid phase correction option in command line");
                    (void)recon3D_abort();
                    ABORT;
                }
            }
        }
    }
    if (!options) /* try vnmr parameters */
    {
        error=P_getstring(CURRENT,"epi_pc", epi_pc, 1, MAXSTR);
        if (!error && epi_seq) {
            options=TRUE;
            pc_option=pc_pick(epi_pc);
            if ((pc_option>MAX_OPTION)||(pc_option<OFF)) {
                Werrprintf("recon3D: Invalid phase correction option in epi_pc");
                (void)recon3D_abort();
                ABORT;
            }
            error=P_getreal(CURRENT,"epi_rev", &repi_rev, 1);
            if (!error) {
                if (repi_rev>0.0)
                    epi_rev=TRUE;
                else
                    epi_rev=FALSE;
            }
        }
    }
    if (!options) /* try the resource file to get recon particulars */
    {
        (void)strcpy(rscfilename, curexpdir);
        (void)strcat(rscfilename, "/recon3D.rsc");
        f1=fopen(rscfilename, "r");
        if (f1) {
            options=TRUE;
            (void)fgets(str, MAXSTR, f1);
            if (strstr(str, "image directory")) {
                (void)sscanf(str, "image directory=%s", rInfo.picInfo.imdir);
                rInfo.picInfo.fullpath=FALSE;
                if (epi_seq) {
                    (void)fgets(epi_pc, MAXSTR, f1);
                    pc_option=pc_pick(epi_pc);
                    if ((pc_option>MAX_OPTION)||(pc_option<OFF)) {
                        Werrprintf("recon3D: Invalid phase correction option in recon3D.rsc file");
                        (void)recon3D_abort();
                        ABORT;
                    }

                    (void)fgets(str, MAXSTR, f1);
                    (void)sscanf(str, "reverse=%d", &epi_rev);
                }
            } else /* no image directory specifed */
            {
                if (epi_seq) {
                    (void)strcpy(epi_pc, str);
                    pc_option=pc_pick(epi_pc);
                    if ((pc_option>MAX_OPTION)||(pc_option<OFF)) {
                        Werrprintf("recon3D: Invalid phase correction option in recon3D.rsc file");
                        (void)recon3D_abort();
                        ABORT;
                    }

                    (void)fgets(str, MAXSTR, f1);
                    (void)sscanf(str, "reverse=%d", &epi_rev);
                }
            }
            (void)fclose(f1);
        }
    }

    if (epi_seq) {
        if ((pc_option>MAX_OPTION)||(pc_option<OFF))
            pc_option=POINTWISE;
    }

    /* always write to recon */
    (void)strcpy(rInfo.picInfo.imdir, "recon");
    rInfo.picInfo.fullpath=FALSE;

    /******************************/
    /* setup                      */
    /******************************/
    if (rInfo.do_setup) {
#ifdef VNMRJ
        if (!aipOwnsScreen()) {
            setdisplay();
            Wturnoff_buttons();
            sunGraphClear();
            aipDeleteFrames();
        } else
            aipSetReconDisplay(-1);
#else
        setdisplay();
        Wturnoff_buttons();
        sunGraphClear();
#endif
        (void)init2d_getfidparms(FALSE);

        if (!epi_seq) {
            epi_rev=FALSE;
            pc_option=OFF;
        }
        
        // get rid of 3D cursors if they are there
        (void)execString("aipShow3Planes(0)\n"); 
  
        temp3D_file=NULL;
        
        /* get choice of filter, if any */
        error=P_getstring(PROCESSED,"recon_window", recon_windowstr, 1, MAXSTR);
        if (error)
            (void)strcpy(recon_windowstr, "NOFILTER");

        if (strstr(recon_windowstr, "OFF")||strstr(recon_windowstr, "off"))
            recon_window=NOFILTER;
        else if (strstr(recon_windowstr, "NOFILTER")||strstr(recon_windowstr,
                "nofilter"))
            recon_window=NOFILTER;
        else if (strstr(recon_windowstr, "BLACKMANN")||strstr(recon_windowstr,
                "blackmann"))
            recon_window=BLACKMANN;
        else if (strstr(recon_windowstr, "HANN")||strstr(recon_windowstr,
                "hann"))
            recon_window=HANN;
        else if (strstr(recon_windowstr, "HAMMING")||strstr(recon_windowstr,
                "hamming"))
            recon_window=HAMMING;
        else if (strstr(recon_windowstr, "GAUSSIAN")||strstr(recon_windowstr,
                "gaussian"))
            recon_window=GAUSSIAN;

      
        /* start out fresh */
        D_close(D_USERFILE);

      /* open the fid file */
      (void)strcpy ( filepath, curexpdir );
      (void)strcat ( filepath, "/acqfil/fid" );  
      (void)strcpy(rInfo.picInfo.fidname,filepath);
      error = D_gethead(D_USERFILE, fid_file_head);
      if (error)
      {
          error = D_open(D_USERFILE, filepath, fid_file_head);   
          if(error)
          {
    	  Vperror("recon3D: Error opening fid file");
    	  ABORT;
          }      
      }
    
         
        /* how many receivers? */
        error=P_getstring(PROCESSED, "rcvrs", rcvrs_str, 1, MAXSTR);
        if (!error) {
            nchannels=strlen(rcvrs_str);
            for (i=0; i<strlen(rcvrs_str); i++)
                if (*(rcvrs_str+i)!='y')
                    nchannels--;
        }
        rInfo.nchannels=nchannels;

        /* figure out if compressed in slice dimension */
        error=P_getstring(PROCESSED, "seqcon", str, 1, MAXSTR);
        if (error) {
            Werrprintf("recon3D: Error getting seqcon");
            (void)recon3D_abort();
            ABORT;
        }
        if (str[3] != 'n')
             threeD=TRUE;
        if (str[1] != 'c')
        {
            slice_compressed=FALSE;
            if(threeD)
                slab_compressed=FALSE;
        }
        if (str[2] != 'c')
            phase_compressed=FALSE;
      
        if (threeD)
            if (str[3] != 'c')
                sliceenc_compressed=FALSE;
        
        /* check acquisition is compressed in 1 and only 1 dimension */
        if ((!phase_compressed) && (!sliceenc_compressed)) {
			Werrprintf("recon3D: seqcon must be ncscn or nccsn");
			(void)recon3D_abort();
			ABORT;
		}
        
        if ((phase_compressed) && (sliceenc_compressed)) {
			Werrprintf("recon3D: seqcon must be ncscn or nccsn");
			(void)recon3D_abort();
			ABORT;
		}

        
        /* display images or not? */
        rInfo.dispint=1;
        if (FALSE && threeD)
            rInfo.dispint=0;
        error=P_getreal(CURRENT,"recondisplay",&dtemp,1);
        if (!error)
            rInfo.dispint=(int)dtemp;

        error=P_getreal(PROCESSED,"ns",&rslices,1);
        if (error) {
            Werrprintf("recon3D: Error getting ns");
            (void)recon3D_abort();
            ABORT;
        }
        slices=(int)rslices;

        error=P_getreal(PROCESSED,"nf",&rnf,1);
        if (error) {
            Werrprintf("recon3D: Error getting nf");
            (void)recon3D_abort();
            ABORT;
        }

        error=P_getreal(PROCESSED,"nv",&rnv,1);
        if (error) {
            Werrprintf("recon3D: Error getting nv");
            (void)recon3D_abort();
            ABORT;
        }
        nv=(int)rnv;
        views=nv;

        if (nv<MINPE) {
            Werrprintf("recon3D:  nv too small");
            (void)recon3D_abort();
            ABORT;
        }
  

        /* byte swap if necessary */
/*        DATAFILEHEADER_CONVERT_NTOH(fid_file_head); */
        ntraces=fid_file_head->ntraces;
        nblocks=fid_file_head->nblocks;
        nro=fid_file_head->np/2;
        // rInfo.fidMd->offsetAddr+= sizeof(dfilehead);
        
  
        if (nro<MINRO) {
            Werrprintf("recon3D:  np too small");
            (void)recon3D_abort();
            ABORT;
        }

        /* check for power of 2 in readout */
        zeropad=0;
        fn=0;
        error=P_getVarInfo(CURRENT,"fn",&info);
        if (info.active) {
            error=P_getreal(CURRENT,"fn",&dtemp,1);
            if (error) {
                Werrprintf("recon3D: Error getting fn");
                (void)recon3D_abort();
                ABORT;
            }
            fn=(int)(dtemp/2);
            ro_size=fn;
        } else {
            n_ft=1;
            while (n_ft<nro)
                n_ft*=2;
            ro_size=n_ft;
        }

        error=P_getreal(PROCESSED, "fract_kx", &dtemp, 1);
        if (!error) {
            ro_size=2*(nro-(int)dtemp);
            zeropad=ro_size-nro;
            n_ft=1;
            while (n_ft<ro_size)
                n_ft*=2;
            ro_size=n_ft;
            if (n_ft<fn)
                ro_size=fn;

            if ((zeropad > HALFF*nro)&&(pc_option<=MAX_OPTION)&&(pc_option>OFF)) {
                Werrprintf("recon3D: phase correction and fract_kx are incompatible");
                (void)recon3D_abort();
                ABORT;
            }
        }

        if (nro<ro_size) {
            itemp=ro_size-nro;
            nro=ro_size;
            ro_size-=itemp;
        } else
            ro_size=nro;

        /* are there mousies? */
        nmice=0;
        error=P_getreal(PROCESSED,"nmice",&dtemp,1);
        if (!error)
            nmice=(int)dtemp;

        /* get pro and ppe for image shifting */
        ro_frq=0.0;
        ro_ph=0.0;
        /*
         (void)ls_ph_fid("lsfid", &itemp, "phfid", &ro_ph, "lsfrq", &ro_frq);
         */
        error=P_getreal(CURRENT,"pro",&ro_frq,1);
        if (!error) {
            error=P_getreal(PROCESSED,"lro",&dtemp,1);
            if (error) {
                Werrprintf("recon3D: Error getting lro");
                (void)recon3D_abort();
                ABORT;
            }
            ro_frq *= (180/dtemp);
        }
        ro_frq=0.0; /* disable this -recon has no business doing readout shifts! */
        pe_frq=0.0;
        pe_ph=0.0;
        /*
         (void)ls_ph_fid("lsfid1", &itemp, "phfid1", &pe_ph, "lsfrq1", &pe_frq);
         */
        error=P_getreal(CURRENT,"ppe",&pe_frq,1);
        if (!error) {
            error=P_getreal(PROCESSED,"lpe",&dtemp,1);
            if (error) {
                Werrprintf("recon3D: Error getting lpe");
                (void)recon3D_abort();
                ABORT;
            }
            pe_frq *= (-180/dtemp);
        }

        ro_frqP=NULL;
        pe_frqP=NULL;
        /* for multi-mouse 2D */
        if (nmice) {
            error=P_getVarInfo(CURRENT,"lsfrq1",&info);
            if (!error && info.active) {
                nshifts=info.size;
                if (((nshifts > 1) &&nshifts != nmice)) {
                    Werrprintf("recon3D: length of lsfrq does not equal nmice ");
                    (void)recon3D_abort();
                    ABORT;
                }
                pe_frqP=(double *)recon3Dallocate(nmice*sizeof(double),
                        "recon3D");
                error=P_getreal(PROCESSED,"sw1",&dt2,1);
                if (error) {
                    Werrprintf("recon3D: Error getting sw1 ");
                    (void)recon3D_abort();
                    ABORT;
                }
                if (nshifts==1) {
                    error=P_getreal(CURRENT,"lsfrq1",&dtemp,1);
                    if (error) {
                        Werrprintf("recon3D: Error getting lsfrq1 ");
                        (void)recon3D_abort();
                        ABORT;
                    }
                    for (i=0; i<nmice; i++)
                        pe_frqP[i]=180.0*dtemp/dt2;
                } else {
                    for (i=0; i<nmice; i++) {
                        error=P_getreal(CURRENT,"lsfrq1",&dtemp,(i+1));
                        if (error) {
                            Werrprintf("recon3D: Error getting lsfrq1 element");
                            (void)recon3D_abort();
                            ABORT;
                        }
                        pe_frqP[i]=180.0*dtemp/dt2;
                    }
                }
            }

            error=P_getVarInfo(CURRENT,"lsfrq",&info);
            if (!error && info.active) {
                nshifts=info.size;
                if (((nshifts > 1) &&nshifts != nmice)) {
                    Werrprintf("recon3D: length of lsfrq does not equal nmice ");
                    (void)recon3D_abort();
                    ABORT;
                }
                ro_frqP=(double *)recon3Dallocate(nmice*sizeof(double),
                        "recon3D");
                error=P_getreal(PROCESSED,"sw",&dt2,1);
                if (error) {
                    Werrprintf("recon3D: Error getting sw ");
                    (void)recon3D_abort();
                    ABORT;
                }
                if (nshifts==1) {
                    error=P_getreal(CURRENT,"lsfrq",&dtemp,1);
                    if (error) {
                        Werrprintf("recon3D: Error getting lsfrq ");
                        (void)recon3D_abort();
                        ABORT;
                    }
                    for (i=0; i<nmice; i++)
                        ro_frqP[i]=180.0*dtemp/dt2;
                } else {
                    for (i=0; i<nmice; i++) {
                        error=P_getreal(CURRENT,"lsfrq",&dtemp,(i+1));
                        if (error) {
                            Werrprintf("recon3D: Error getting lsfrq element");
                            (void)recon3D_abort();
                            ABORT;
                        }
                        ro_frqP[i]=180.0*dtemp/dt2;
                    }
                }
            }
        }

        error=P_getreal(PROCESSED,"nseg",&rnseg,1);
        if (error)
            rnseg=1.0;

        error=P_getreal(PROCESSED,"ne",&rechoes,1);
        if (error)
            rechoes=1.0;
        echoes=(int)rechoes;
        if (echoes>1&&epi_seq)
            epi_me=echoes;

        error=P_getreal(PROCESSED,"etl",&retl,1);
        if (error)
            retl=1.0;

        /* get some epi related parameters */
        error1=P_getreal(PROCESSED,"flash_converted",&rfc,1);
        if (!error1)
            flash_converted=TRUE;

        /* sanity checks */
        if (flash_converted) {
            if (ntraces==nv*slices) {
                phase_compressed=TRUE;
                slice_compressed=TRUE;
            } else if (ntraces==nv) {
                phase_compressed=TRUE;
                slice_compressed=FALSE;
            } else if (ntraces==slices) {
                phase_compressed=FALSE;
                slice_compressed=TRUE;
            } else if (ntraces==1) {
                phase_compressed=FALSE;
                slice_compressed=FALSE;
            }
        }

        if (slice_compressed&&!epi_seq&&!threeD)
            multi_slice=TRUE;
        else if(threeD && slab_compressed)
            multi_slice=TRUE;
   

        error=P_getstring(PROCESSED, "ms_intlv", msintlv, 1, MAXSTR);
        if (!error) {
            if (msintlv[0]=='y')
                multi_slice=TRUE;
        }

        imglen=1;
        error=P_getVarInfo(PROCESSED,"image",&info);
        if (!error)
            imglen=info.size;

        error=P_getstring(PROCESSED,"array", arraystr, 1, MAXSTR);
        if (error) {
            Werrprintf("recon3D: Error getting array");
            (void)recon3D_abort();
            ABORT;
        }

        /* locate image within array to see if reference scan was acquired */
        imzero=0;
        imneg1=0;
        imneg2=0;
        imptr=strstr(arraystr, "image");
        if (!imptr)
            imglen=1; /* image was not arrayed */
        else if (imglen>1) {
            /* how many when image=1? */
            /* also check for image = -2 or -1  */
            for (i=0; i<imglen; i++) {
                error=P_getreal(PROCESSED,"image",&rimflag,(i+1));
                if (error) {
                    Werrprintf("recon3D: Error getting image element");
                    (void)recon3D_abort();
                    ABORT;
                }
                if (rimflag==0.0)
                    imzero++;
                else if (rimflag==-1)
                    imneg1++;
                else if (rimflag==-2)
                    imneg2++;
            }
            if ((imneg1>0) && (imneg2>0))
                epi_dualref=TRUE;
        }
        if ((!imzero)&&(!epi_dualref))
            pc_option=OFF;

        /* get nt  array  */
        error=P_getVarInfo(PROCESSED,"nt",&info);
        if (error) {
            Werrprintf("recon3D: Error getting nt info");
            (void)recon3D_abort();
            ABORT;
        }
        ntlen=info.size; /* read nt values*/
        nts=(int *)recon3Dallocate(ntlen*sizeof(int), "recon3D");
        for (i=0; i<ntlen; i++) {
            error=P_getreal(PROCESSED,"nt",&dtemp,(i+1));
            if (error) {
                Werrprintf("recon3D: Error getting nt element");
                (void)recon3D_abort();
                ABORT;
            }
            nts[i]=(int)dtemp;
        }

        /* dc correct or not? */
        rInfo.dc_flag=FALSE;
        error=P_getstring(CURRENT, "dcrmv", dcrmv, 1, SHORTSTR);
        if (!error) {
            if ((dcrmv[0] == 'y') &&(nts[0]==1))
                rInfo.dc_flag = TRUE;
        }

        if (slice_compressed) {
            slices=(int)rslices;
            slicesperblock=slices;
        } else {
            P_getVarInfo(PROCESSED, "pss", &info);
            slices = info.size;
            slicesperblock=1;

            if (slices>nblocks) {
                Werrprintf("recon3D: slices greater than fids for standard mode");
                (void)recon3D_abort();
                ABORT;
            }

            if (slices>1) {
                viewsperblock=1;
                if (phase_compressed)
                    viewsperblock=views;
                within_slices=(nblocks/nchannels);
                /* get array info */
                rInfo.narray=0;
                if (strlen(arraystr)) {
                    (void)arrayparse(arraystr, &(rInfo.narray),
                            &(rInfo.arrayelsP), (views/viewsperblock));
                    /* find pss and determine number of blocks within each slice */
                    ip=0;
                    while ((!strstr((char *) rInfo.arrayelsP[ip].names, "pss"))
                            &&(ip<rInfo.narray))
                        ip++;
                    within_slices=rInfo.arrayelsP[ip].denom;
                }
                within_slices *= nchannels;
            } else
                within_slices=(nblocks/nchannels);

            if (!slices) {
                Werrprintf("recon3D: slices equal to zero");
                (void)recon3D_abort();
                ABORT;
            }
        }

        views=nv;
        etl=1;
        nshots=views;
        multi_shot=FALSE;
        if (epi_seq) {
            /* default for EPI is single shot */
            etl=views;
            nshots=1;
            if (rnseg>1.0) {
                nshots=(int)rnseg;
                multi_shot=TRUE;
                if (rnseg)
                    etl=(int)views/nshots;
                else {
                    Werrprintf("recon3D: nseg equal to zero");
                    (void)recon3D_abort();
                    ABORT;
                }
            }
        } else {
            if (retl > 1.0) {
                multi_shot=TRUE;
                etl=(int)retl;
                nshots=views/etl;
            }
        }

        if (phase_compressed) {
            if (slices)
                slice_reps=nblocks*slicesperblock/slices;
            else {
                Werrprintf("recon3D: slices equal to zero");
                (void)recon3D_abort();
                ABORT;
            }
            slice_reps -= (imzero+imneg1+imneg2);
            if (slicesperblock)
                views=ntraces/slicesperblock;
            else {
                Werrprintf("recon3D: slicesperblock equal to zero");
                (void)recon3D_abort();
                ABORT;
            }
            views=nv;
            viewsperblock=views;
        } else {
            views=nv;
            if (slices)
                slice_reps=nblocks*slicesperblock/slices;
            else {
                Werrprintf("recon3D: slices equal to zero");
                (void)recon3D_abort();
                ABORT;
            }
            if (nshots)
                slice_reps/=nshots;
            else {
                Werrprintf("recon3D: nshots equal to zero");
                (void)recon3D_abort();
                ABORT;
            }
            slice_reps -= (imzero+imneg1+imneg2);
            viewsperblock=1;
            if (nshots) {
                within_views=(nblocks)/nshots; /* views are outermost */
                if (!slice_compressed&&!flash_converted)
                    within_slices/=nshots;
            } else {
                Werrprintf("recon3D: nshots equal to zero");
                (void)recon3D_abort();
                ABORT;
            }
        }

        if (threeD) {
            if (phase_compressed) {
                slab_reps=nblocks*slabsperblock/slabs;
                slab_reps -= (imzero+imneg1+imneg2);
                viewsperblock=views;
            } else {
                slab_reps=nblocks*slabsperblock/slabs;
                slab_reps/=views;
                slab_reps -= (imzero+imneg1+imneg2);
                viewsperblock=1;
                within_views=nblocks/views; /* views are outermost */
                if (!slab_compressed&&!flash_converted)
                    within_slabs/=views;
            }
            error=P_getreal(PROCESSED,"nv2",&rnv2,1);
            if (error) {
                Werrprintf("recon3D: Error getting nv2");
                (void)recon3D_abort();
                ABORT;
            }
 
            slices=(int)rnv2;
            if (sliceenc_compressed) {
                sviewsperblock=slices;
            } else {
                slab_reps/=slices;
                within_sviews=nblocks/slices; /* rubbish! */
                if (!phase_compressed)
                    within_sviews/=views; /* further rubbish! */
            }

            /* check profile flag and fake a 2D if need be */
            error=P_getstring(PROCESSED, "profile", profstr, 1, SHORTSTR);
            if (!error) {
                if ((profstr[0] == 'y') && (profstr[1] == 'n')) {
                    threeD=FALSE;
                    slices=1;
                    slice_reps=1;
                    slicesperblock=1;
                    within_slices=1;
                    views=(int)rnv2;
                    /* phase encode inherits properties of slice encode */
                    phase_compressed=sliceenc_compressed;
                    viewsperblock=sviewsperblock;
                    within_views=within_sviews;
                } else if ((profstr[0] == 'n') && (profstr[1] == 'y')) {
                    threeD=FALSE;
                    slices=1;
                    views=(int)rnv;
                }
            }

            /* slice encode direction zero fill or not? */
            fn2=0;
            error=P_getVarInfo(CURRENT,"fn2",&info);
            if (info.active) {
                error=P_getreal(CURRENT,"fn2",&dtemp,1);
                if (error) {
                    Werrprintf("recon3D: Error getting fn2");
                    (void)recon3D_abort();
                    ABORT;
                }
                fn2=(int)(dtemp/2);
                pe2_size=fn2;
            } else {
                n_ft=1;
                while (n_ft<slices)
                    n_ft*=2;
                pe2_size=n_ft;
            }
        } /* end if 3D */

        /* get array info */
        rInfo.narray=0;
        if (strlen(arraystr))
            (void)arrayparse(arraystr, &(rInfo.narray), &(rInfo.arrayelsP),
                    (views/viewsperblock));

        zeropad2=0;
        fn1=0;
        error=P_getVarInfo(CURRENT,"fn1",&info);
        if (info.active) {
            error=P_getreal(CURRENT,"fn1",&dtemp,1);
            if (error) {
                Werrprintf("recon3D: Error getting fn1");
                (void)recon3D_abort();
                ABORT;
            }
            fn1=(int)(dtemp/2);
            pe_size=fn1;
        } else {
            n_ft=1;
            while (n_ft<views)
                n_ft*=2;
            pe_size=n_ft;
        }

        error=P_getreal(PROCESSED, "fract_ky", &dtemp, 1);
        if (!error) {
            pe_size=views;
            views=views/2+ (int)dtemp;
            zeropad2=pe_size-views;
            n_ft=1;
            while (n_ft<pe_size)
                n_ft*=2;
            pe_size=n_ft;
            if (n_ft<fn1)
                pe_size=fn1;

            /* adjust etl OR nshots, depending on VJ parameter */
            if (epi_seq) {
                if (nshots)
                    etl=(int)views/nshots;
            } else {
                if (etl)
                    nshots=views/etl;
            }

            if (phase_compressed)
                viewsperblock=views;

            if ((zeropad > HALFF*nro)&&(zeropad2 > HALFF*views)) {
                Werrprintf("recon3D: fract_kx and fract_ky are incompatible");
                (void)recon3D_abort();
                ABORT;
            }
        }

        /* initialize phase & read reversal flags */
        rInfo.phsrev_flag=FALSE;
        rInfo.alt_phaserev_flag=FALSE;
        rInfo.alt_readrev_flag=FALSE;
        if (echoes>1) {
            error=P_getstring(CURRENT,"altecho_reverse", tmp_str, 1, MAXSTR);
            if (!error) {
                if (tmp_str[0]=='y')
                    rInfo.alt_readrev_flag=TRUE;
                if (strlen(tmp_str)>1)
                    if (tmp_str[1]=='y')
                        rInfo.alt_phaserev_flag=TRUE;
            }
        }

        /* navigator stuff */
        nnav=0;
        error=P_getstring(PROCESSED, "navigator", nav_str, 1, SHORTSTR);
        if (!error) {
            if (nav_str[0]=='y') {
                error=P_getVarInfo(PROCESSED,"nav_echo",&info);
                if (!error) {
                    nnav=info.size;
                    nav_list = (int *)recon3Dallocate(nnav*sizeof(int),
                            "recon3D");
                    for (i=0; i<nnav; i++) {
                        error=P_getreal(PROCESSED,"nav_echo",&dtemp,i+1);
                        nav_list[i]=(int)dtemp - 1;
                        if ((nav_list[i]<0)||(nav_list[i]>=(etl+nnav))) {
                            Werrprintf("recon3D: navigator value out of range");
                            (void)recon3D_abort();
                            ABORT;
                        }
                    }
                }

                if (!nnav) {
                    /* figure out navigators per echo train */
                    nnav=((ntraces/slicesperblock)-views)/nshots;
                    if (nnav>0) {
                        nav_list = (int *)recon3Dallocate(nnav*sizeof(int),
                                "recon3D");
                        for (i=0; i<nnav; i++)
                            nav_list[i]=i;
                        nav_pts=ro_size;
                    } else
                        nnav=0;
                }

                error=P_getreal(PROCESSED,"navnp",&dtemp,1);
                if (!error) {
                    nav_pts=(int)dtemp;
                    nav_pts/=2;
                } else
                    nav_pts=ro_size;
            }
        }

        nav_option=OFF;
        if (nnav) {
            nav_option=POINTWISE;
            if (nnav>1)
                nav_option=PAIRWISE;

            error=P_getstring(CURRENT,"nav_type", nav_type, 1, MAXSTR);
            if (!error) {
                if (strstr(nav_type, "OFF")||strstr(nav_type, "off"))
                    nav_option=OFF;
                else if (strstr(nav_type, "POINTWISE")||strstr(nav_type,
                        "pointwise"))
                    nav_option=POINTWISE;
                else if (strstr(nav_type, "LINEAR")||strstr(nav_type, "linear"))
                    nav_option=LINEAR;
                else if (strstr(nav_type, "PAIRWISE")||strstr(nav_type,
                        "pairwise"))
                    nav_option=PAIRWISE;
                else {
                    Werrprintf("recon3D: Invalid navigator option in nav_type parameter!");
                    (void)recon3D_abort();
                    ABORT;
                }
            }
            if ((nav_option==PAIRWISE)&&(nnav<2)) {
                Werrprintf("recon3D: 2 navigators required for PAIRWISE nav_type");
                (void)recon3D_abort();
                ABORT;
            }

            /* adjust etl and views if necessary */
            if (views == (ntraces/slicesperblock)) {
                views-=nshots*nnav;
                etl=views/nshots;
                if (phase_compressed)
                    viewsperblock=views;
            }
        } /* end of navigator setup */

        if (threeD)
            slice_reps=slab_reps;

        if (slice_reps<1) {
            Werrprintf("recon3D: slice reps less than 1");
            (void)recon3D_abort();
            ABORT;
        }

        error=P_getreal(PROCESSED,"tab_converted",&rfc,1);
        if (!error)
            tab_converted=TRUE;

        tablen=views;
        if (threeD)
            tablen*=slices;

        multi_shot=FALSE;
        min_view=min_view2=0;

        error=P_getVarInfo(CURRENT,"pelist",&info);
        if (!error && (info.size>1)) {
            itablen=info.size;
            if ((itablen != tablen)&&(itablen != views)) {
                Werrprintf("recon3D: pelist is wrong size");
                (void)recon3D_abort();
                ABORT;
            }
            tablen=itablen;
            view_order = (int *)recon3Dallocate(itablen*sizeof(int), "recon3D");
            for (itab=0; itab<itablen; itab++) {
                error=P_getreal(CURRENT,"pelist",&dtemp,itab+1);
                if (error) {
                    Werrprintf("recon3D: Error getting pelist element");
                    (void)recon3D_abort();
                    ABORT;
                }
                view_order[itab]=dtemp;
                if (dtemp<min_view)
                    min_view=dtemp;
                if (-1*dtemp<min_view2)
                    min_view2=-1*dtemp;
            }
            multi_shot=TRUE;
        } else {
            error=P_getstring(PROCESSED, "petable", petable, 1, MAXSTR);
            /* open table for sorting */
            if ((!error)&&strlen(petable)&&strcmp(petable, "n")&&strcmp(
                    petable, "N")&&(!tab_converted)) {
                multi_shot=TRUE;
                /*  try user's directory*/
                error=P_getstring(PROCESSED, "petable", petable, 1, MAXSTR);
                if (error) {
                    Werrprintf("recon3D: Error getting petable");
                    (void)recon3D_abort();
                    ABORT;
                }
                table_ptr = NULL;
                if (appdirFind(petable, "tablib", tablefile, NULL, R_OK))
                    table_ptr = fopen(tablefile, "r");
                if (!table_ptr) {
                    Werrprintf("recon3D: Error opening petable file %s",
                            petable);
                    (void)recon3D_abort();
                    ABORT;
                }

                /* first figure out size of sorting table(s) */
                while ((tstch=fgetc(table_ptr) != '=') && (tstch != EOF))
                    ;
                itablen = 0;
                done=FALSE;
                while ((itablen<tablen) && !done) {
                    if (fscanf(table_ptr, "%d", &iview) == 1)
                        itablen++;
                    else
                        done = TRUE;
                }
                /* reposition to start of file */
                error=fseek(table_ptr, 0, SEEK_SET);
                if (error) {
                    Werrprintf("recon3D: Error with fseek, petable");
                    (void)recon3D_abort();
                    ABORT;
                }
                if ((itablen!=views)&&(itablen!=tablen)) {
                    Werrprintf("recon3D: Error wrong phase sorting table size");
                    (void)recon3D_abort();
                    ABORT;
                }

                /* read in table for view sorting */
                view_order = (int *)recon3Dallocate(tablen*sizeof(int),
                        "recon3D");
                while ((tstch=fgetc(table_ptr) != '=') && (tstch != EOF))
                    ;
                if (tstch == EOF) {
                    Werrprintf("recon3D: EOF while reading petable file");
                    (void)recon3D_abort();
                    ABORT;
                }
                itab = 0;
                min_view=0;
                min_view2=0;
                done=FALSE;
                while ((itab<tablen) && !done) {
                    if (fscanf(table_ptr, "%d", &iview)==1) {
                        view_order[itab]=iview;
                        if (iview<min_view)
                            min_view=iview;
                        if (-1*iview<min_view2)
                            min_view2=-1*iview;
                        itab++;
                    } else
                        done = TRUE;
                }

                /* check for t2 table describing slice encode ordering */
                tstch=fgetc(table_ptr);
                while ((tstch != 't') && (tstch != EOF))
                    tstch=fgetc(table_ptr);

                sview_order=NULL;
                if ((tstch != EOF)&&threeD) {
                    sview_order = (int *)recon3Dallocate(tablen*sizeof(int),
                            "recon3D");
                    while ((tstch=fgetc(table_ptr) != '=') && (tstch != EOF))
                        ;
                    if (tstch == EOF) {
                        Werrprintf("recon3D: EOF while reading petable file for t2");
                        (void)recon3D_abort();
                        ABORT;
                    }
                    itab = 0;
                    min_sview=0;
                    done=FALSE;
                    while ((itab<tablen) && !done) {
                        if (fscanf(table_ptr, "%d", &iview) == 1) {
                            sview_order[itab]=iview;
                            if (iview<min_sview)
                                min_sview=iview;
                            itab++;
                        } else
                            done = TRUE;
                    }
                }
                (void)fclose(table_ptr);
            }
        }

        if (multi_shot) {
            /* make it start from zero */
            if (min_view<min_view2) {
                if (sview_order) {
                    for (i=0; i<tablen; i++) {
                        view_order[i]=-1*view_order[i]-min_view2;
                        sview_order[i]-=min_sview;
                    }
                } else {
                    for (i=0; i<tablen; i++)
                        view_order[i]=-1*view_order[i]-min_view2;
                }
                rInfo.phsrev_flag=TRUE;
            } else {
                if (sview_order) {
                    for (i=0; i<tablen; i++) {
                        view_order[i]-=min_view;
                        sview_order[i]-=min_sview;
                    }
                } else {
                    for (i=0; i<tablen; i++)
                        view_order[i]-=min_view;
                }
            }
        } /* end if multishot*/

        if (flash_converted)
            multi_shot=FALSE;

        blockreps = NULL;
        blockrepeat = NULL;

        if (!threeD) {
            /* get pss array and make slice order array */
            pss=(float*)recon3Dallocate(slices*sizeof(float), "recon3D");
            slice_order=(int*)recon3Dallocate(slices*sizeof(int), "recon3D");
            for (i=0; i<slices; i++) {
                error=P_getreal(PROCESSED,"pss",&dtemp,i+1);
                if (error) {
                    Werrprintf("recon3D: Error getting slice offset");
                    (void)recon3D_abort();
                    ABORT;
                }
                pss[i]=(float)dtemp;
                slice_order[i]=i+1;
            }

            uslicesperblock=slicesperblock;
            if (slice_compressed) {
                /* count repetitions of slice positions */
                blockreps=(int *)recon3Dallocate(slicesperblock*sizeof(int),
                        "recon3D");
                blockrepeat=(int *)recon3Dallocate(slicesperblock*sizeof(int),
                        "recon3D");
                pss2=(float *)recon3Dallocate(slicesperblock*sizeof(float),
                        "recon3D");
                for (i=0; i<slicesperblock; i++) {
                    blockreps[i]=0;
                    blockrepeat[i]=-1;
                    pss2[i]=pss[i];
                }
                (void)qsort(pss2, slices, sizeof(float), pcompare);
                pss0=pss2[0];
                i=0;
                j=0;
                while (i<slicesperblock) {
                    if (pss2[i]!=pss0) {
                        pss0=pss2[i];
                        j++;
                    }
                    blockreps[j]++;
                    i++;
                }
                uslicesperblock=j+1;
            }

            /* sort the slice order based on pss */
            if (uslicesperblock==slicesperblock)
                (void)qsort(slice_order, slices, sizeof(int), psscompare);
            else {
                upss=(float *)recon3Dallocate(uslicesperblock*sizeof(float),
                        "recon3D");
                j=0;
                for (i=0; i<uslicesperblock; i++) {
                    upss[i]=pss[j];
                    j+=blockreps[i];
                }
                (void)qsort(slice_order, uslicesperblock, sizeof(int),
                        upsscompare);
            }
        }

        /***********************************************************************************/
        /* get array to see how reference scan data was interleaved with acquisition */
        /***********************************************************************************/
        dimfirst=1;
        dimafter=1;

        if (imptr != NULL) /* don't bother if no image */
        {
            /* interrogate array to find position of 'image' */
            imfound=FALSE;

            ptr=strtok(arraystr, ",");
            while (ptr != NULL) {
                if (ptr == imptr)
                    imfound=TRUE;

                /* is this a jointly arrayed thing? */
                if (strstr(ptr, "(")) {
                    while (strstr(ptr, ")")==NULL) /* move to end of jointly arrayed list */
                    {
                        if (strstr(ptr, "image"))
                            image_jnt=TRUE;
                        ptr=strtok(NULL,",");
                    }
                    *(ptr+strlen(ptr)-1)='\0';
                }
                strcpy(aname, ptr);
                error=P_getVarInfo(PROCESSED,aname,&info);
                if (error) {
                    Werrprintf("recon3D: Error getting something");
                    (void)recon3D_abort();
                    ABORT;
                }

                if (imfound) {
                    if (ptr != imptr)
                        dimafter *= info.size; /* get dimension of this variable */
                } else
                    dimfirst *= info.size;

                ptr=strtok(NULL,",");
            }
        } else
            dimfirst=nblocks;

        if (flash_converted)
            dimafter*=slices;

        dimafter *= nchannels;

        /* set up fdf header structure */

        error=P_getreal(PROCESSED,"lro",&dtemp,1);
        if (error) {
            Werrprintf("recon3D: Error getting lro");
            (void)recon3D_abort();
            ABORT;
        }
        rInfo.picInfo.fovro=dtemp;
        error=P_getreal(PROCESSED,"lpe",&dtemp,1);
        if (error) {
            Werrprintf("recon3D: Error getting lpe");
            (void)recon3D_abort();
            ABORT;
        }
        rInfo.picInfo.fovpe=dtemp;
        error=P_getreal(PROCESSED,"thk",&dtemp,1);
        if (error) {
            Werrprintf("recon3D: Error getting thk");
            (void)recon3D_abort();
            ABORT;
        }
        rInfo.picInfo.thickness=MM_TO_CM*dtemp;

        error=P_getreal(PROCESSED,"gap",&dtemp,1);
        if (error && !threeD) {
            Werrprintf("recon3D: Error getting gap");
            (void)recon3D_abort();
            ABORT;
        }
        rInfo.picInfo.gap=dtemp;

        if (threeD) {
            error=P_getreal(PROCESSED,"lpe2",&dtemp,1);
            if (error) {
                Werrprintf("recon3D: Error getting lpe2");
                (void)recon3D_abort();
                ABORT;
            }
            rInfo.picInfo.thickness=dtemp;
        }

        rInfo.picInfo.slices=slices;
        rInfo.picInfo.echo=1;
        rInfo.picInfo.echoes=echoes;

        error=P_getVarInfo(PROCESSED,"psi",&info);
        if (error) {
            Werrprintf("recon3D: Error getting psi info");
            (void)recon3D_abort();
            ABORT;
        }
        rInfo.picInfo.npsi=info.size;
        error=P_getVarInfo(PROCESSED,"phi",&info);
        if (error) {
            Werrprintf("recon3D: Error getting phi info");
            (void)recon3D_abort();
            ABORT;
        }
        rInfo.picInfo.nphi=info.size;
        error=P_getVarInfo(PROCESSED,"theta",&info);
        if (error) {
            Werrprintf("recon3D: Error getting theta info");
            (void)recon3D_abort();
            ABORT;
        }
        rInfo.picInfo.ntheta=info.size;

        error=P_getreal(PROCESSED,"te",&dtemp,1);
        if (error) {
            Werrprintf("recon3D: recon3D: Error getting te");
            (void)recon3D_abort();
            ABORT;
        }
        rInfo.picInfo.te=(float)SEC_TO_MSEC*dtemp;
        error=P_getreal(PROCESSED,"tr",&dtemp,1);
        if (error) {
            Werrprintf("recon3D: recon3D: Error getting tr");
            (void)recon3D_abort();
            ABORT;
        }
        rInfo.picInfo.tr=(float)SEC_TO_MSEC*dtemp;
        (void)strcpy(rInfo.picInfo.seqname, sequence);
        (void)strcpy(rInfo.picInfo.studyid, studyid);
        (void)strcpy(rInfo.picInfo.position1, pos1);
        (void)strcpy(rInfo.picInfo.position2, pos2);
        error=P_getreal(PROCESSED,"ti",&dtemp,1);
        if (error) {
            Werrprintf("recon3D: recon3D: Error getting ti");
            (void)recon3D_abort();
            ABORT;
        }
        rInfo.picInfo.ti=(float)SEC_TO_MSEC*dtemp;
        rInfo.picInfo.image=dimafter*dimfirst;
        if (image_jnt)
            rInfo.picInfo.image/=imglen;
        rInfo.picInfo.image*=(imglen-imzero-imneg1-imneg2);

        if (!phase_compressed)
            rInfo.picInfo.image/=views;
        if (!slice_compressed)
            rInfo.picInfo.image/=slices;
        rInfo.picInfo.array_index=1;

        error=P_getreal(PROCESSED,"sfrq",&dtemp,1);
        if (error) {
            Werrprintf("recon3D: recon3D: Error getting sfrq");
            (void)recon3D_abort();
            ABORT;
        }
        rInfo.picInfo.sfrq=dtemp;

        error=P_getreal(PROCESSED,"dfrq",&dtemp,1);
        if (error) {
            Werrprintf("recon3D: recon3D: Error getting dfrq");
            (void)recon3D_abort();
            ABORT;
        }
        rInfo.picInfo.dfrq=dtemp;

        /* estimate time for block acquisition */
        error = P_getreal(PROCESSED,"at",&acqtime,1);
        if (error) {
            Werrprintf("recon3D: Error getting at");
            (void)recon3D_abort();
            ABORT;
        }

        /* 	snoozetime=(useconds_t)(acqtime*ntraces*nt*SEC_TO_USEC);*/
        /*	snoozetime=(useconds_t)(0.25*rInfo.picInfo.tr*MSEC_TO_USEC); */

        if (views<pe_size) {
            itemp=pe_size-views;
            views=pe_size;
            pe_size-=itemp;
        } else
            pe_size=views;

        if (threeD) {
            if (slices<pe2_size) {
                itemp=pe2_size-slices;
                slices=pe2_size;
                pe2_size-=itemp;
            } else
                pe2_size=slices;

            rInfo.picInfo.slices=slices;
        }

        /* set up filter window if necessary */
        read_window=NULL;
        phase_window=NULL;
        nav_window=NULL;
        recon_window=NOFILTER; /* turn mine off */

        if (nnav) /* make a window for navigator echoes */
        {
            nav_window=(float*)recon3Dallocate(nro*sizeof(float), "recon3D");
            (void)filter_window(GAUSS_NAV,nav_window, nro);
        }

        error=P_getreal(CURRENT,"ftproc",&dtemp,1);
        if (error) {
            Werrprintf("recon3D: Error getting ftproc");
            (void)recon3D_abort();
            ABORT;
        }
        wtflag=(int)dtemp;

        if (wtflag) {
            recon_window=MAX_FILTER;
            if (init_wt1(&wtp, S_NP)) {
                Werrprintf("recon3D: Error from init_wt1");
                (void)recon3D_abort();
                ABORT;
            }
            wtp.wtflag=TRUE;
            fpointmult = getfpmult(S_NP, fid_file_head->status & S_DDR);
            read_window=(float*)recon3Dallocate(ro_size*sizeof(float),
                    "recon3D");
            if (init_wt2(&wtp, read_window, ro_size, FALSE, S_NP, fpointmult,
                    FALSE)) {
                Werrprintf("recon3D: Error from init_wt2");
                (void)recon3D_abort();
                ABORT;
            }
            phase_window=(float*)recon3Dallocate(views*sizeof(float),
                    "recon3D");
            if (phase_compressed) {
                if (init_wt1(&wtp, S_NF)) {
                    Werrprintf("recon3D: Error from init_wt1");
                    (void)recon3D_abort();
                    ABORT;
                }
                wtp.wtflag=TRUE;
                fpointmult = getfpmult(S_NF,0);
                if (init_wt2(&wtp, phase_window, views, FALSE, S_NF,
                        fpointmult, FALSE)) {
                    Werrprintf("recon3D: Error from init_wt2");
                    (void)recon3D_abort();
                    ABORT;
                }
            } else {
                if (init_wt1(&wtp, S_NI)) {
                    Werrprintf("recon3D: Error from init_wt1");
                    (void)recon3D_abort();
                    ABORT;
                }
                wtp.wtflag=TRUE;
                fpointmult = getfpmult(S_NI,0);
                if (init_wt2(&wtp, phase_window, views, FALSE, S_NI,
                        fpointmult, FALSE)) {
                    Werrprintf("recon3D: Error from init_wt2");
                    (void)recon3D_abort();
                    ABORT;
                }
            }
        }

        error=P_getstring(CURRENT, "rcvrout",tmp_str, 1, MAXSTR);
        if ((!error)&& (tmp_str[0]=='i')) {
            smash=TRUE;
            sense=TRUE;
        }

        error=P_getstring(CURRENT, "raw", tmp_str, 1, SHORTSTR);
        if (!error) {
            if (tmp_str[0]=='m')
                rawflag=RAW_MAG;
            if (tmp_str[0]=='p')
                rawflag=RAW_PHS;
            if (tmp_str[0]=='b')
                rawflag=RAW_MP;
        }

        error=P_getstring(CURRENT, "dmg", tmp_str, 1, SHORTSTR);
        if (!error) {
            if (strstr(tmp_str, "pa"))
                phsflag=TRUE;
        }

        if (nmice) {
            sense=TRUE;
            phsflag=FALSE;
            rawflag=FALSE;
        }

        /* turn off  recon force */
        root = getTreeRoot ("current");
        (void)RcreateVar ("recon_force", root, T_REAL);
        error=P_getreal(CURRENT,"recon_force",&recon_force,1);
        if (!error)
            error=P_setreal(CURRENT,"recon_force",0.0,1);

        realtime_block=0;
        rInfo.image_order=0;
        rInfo.phase_order=0;
        rInfo.rawmag_order=0;
        rInfo.rawphs_order=0;
        rInfo.pc_slicecnt=0;
        rInfo.slicecnt=0;
        rInfo.dispcnt=0;

        coilfactor=1;
        if (smash||sense||threeD)
            coilfactor=nchannels;

        npts=views*nro; /* number of pixels per slice */
        npts3d=npts*slices; /* number of pixels per slab */
        
        
        if (threeD) {
			double chkSize;

			chkSize = (double) (slabsperblock*slab_reps*echoes*2*coilfactor);
			chkSize *= (double) npts3d * (double) sizeof(float);
			chkSize=fid_file_head->bbytes;
			if (chkSize > 100000000000.*65536.0*32768.0) {
				Werrprintf("recon3D: block size too large (> 2 Gbytes)");
				(void)recon3D_abort();
				ABORT;
			}


			/* only a  block at a time */
			if (phase_compressed)
				dsize=npts*2;
			if (sliceenc_compressed)
				dsize=nro*2*slices;

			dsize*=coilfactor;
			magnitude=(float *)recon3Dallocate(dsize*sizeof(float), "recon3D");
		//	mag2=(float *)recon3Dallocate(npts3d*sizeof(float), "recon3D");
			if (nnav) {
				nsize=nshots*nnav*echoes*nro*slices*slice_reps;
				nsize_ref=echoes*nro*slices;
				navdata=(float *)recon3Dallocate(nsize*sizeof(float), "recon3D");
				nav_refphase=(double *)recon3Dallocate(nsize_ref*sizeof(double), "recon3D");
			}
		} /* 2D cases below */
        else if (phase_compressed) {
            dsize=slicesperblock*echoes*2*npts;
            if (threeD)
                dsize=slabsperblock*echoes*2*npts3d;
            magnitude=(float *)recon3Dallocate((dsize/2)*sizeof(float),
                    "recon3D");
            mag2=(float *)recon3Dallocate((dsize/2)*sizeof(float), "recon3D");
            if (nnav) {
                nsize=nshots*nnav*echoes*nro*2*slicesperblock;
                nsize_ref=echoes*nro*slicesperblock;
                navdata=(float *)recon3Dallocate(nsize*sizeof(float),
                        "recon3D");
                nav_refphase=(double *)recon3Dallocate(nsize_ref*sizeof(double),
                        "recon3D");
            }
        } else {
            dsize=slices*slice_reps*echoes*2*npts;
            dsize*=coilfactor;
            magnitude=(float *)recon3Dallocate(npts*sizeof(float), "recon3D");
            mag2=(float *)recon3Dallocate(npts*sizeof(float), "recon3D");
            if (nnav) {
                nsize=nshots*nnav*echoes*nro*slices*slice_reps;
                nsize_ref=echoes*nro*slices;
                navdata=(float *)recon3Dallocate(nsize*sizeof(float),
                        "recon3D");
                nav_refphase=(double *)recon3Dallocate(nsize_ref*sizeof(double),
                        "recon3D");
            }
        }
    

        slicedata=(float *)recon3Dallocate(dsize*sizeof(float), "recon3D");
        if (slicedata == NULL) {
            (void)recon3D_abort();
            ABORT;
        }
        if ((rawflag==RAW_MAG)||(rawflag==RAW_MP))
			rawmag=(float *)recon3Dallocate((dsize/2)*sizeof(float),"recon3D");
		if ((rawflag==RAW_PHS)||(rawflag==RAW_MP))
			rawphs=(float *)recon3Dallocate((dsize/2)*sizeof(float),"recon3D");
		if (sense||phsflag)
			imgphs=(float *)recon3Dallocate((dsize/2)*sizeof(float),"recon3D");
		if (phsflag)
			imgphs2=(float *)recon3Dallocate((dsize/2)*sizeof(float),"recon3D");

        if (pc_option!=OFF) {
            pc=(float *)recon3Dallocate(slices*echoes*2*npts*sizeof(float),
                    "recon3D");
            pc_done=(short *)recon3Dallocate(slices*echoes*sizeof(int),
                    "recon3D");
            for (ipc=0; ipc<slices*echoes; ipc++)
                *(pc_done+ipc)=FALSE;

            if (epi_dualref && (pc_option==TRIPLE_REF)) {
                pc_neg=(float *)recon3Dallocate(slices*echoes*2*npts
                        *sizeof(float), "recon3D");
                slicedata_neg=(float *)recon3Dallocate(dsize*sizeof(float),
                        "recon3D");
                imdata=(float *)recon3Dallocate(dsize*sizeof(float),"recon3D");
                imdata_neg=(float *)recon3Dallocate(dsize*sizeof(float),"recon3D");
                if (nchannels) {
                    imdata2=(float *)recon3Dallocate(dsize*sizeof(float),"recon3D");
                    imdata2_neg=(float *)recon3Dallocate(dsize*sizeof(float),"recon3D");
                }
            }
            if (epi_dualref) {
                pcneg_done=(short *)recon3Dallocate(slices*echoes*sizeof(int),"recon3D");
                for (ipc=0; ipc<slices*echoes; ipc++)
                    *(pcneg_done+ipc)=FALSE;
            }
        }

        /* bundle these for convenience */
        rInfo.picInfo.ro_size=ro_size;
        rInfo.picInfo.pe_size=pe_size;
        rInfo.picInfo.nro=nro;
        rInfo.picInfo.npe=views;
        rInfo.svinfo.ro_size=ro_size;
        rInfo.svinfo.pe_size=pe_size;
        rInfo.svinfo.pe2_size=pe2_size;
        rInfo.svinfo.slice_reps=slice_reps;
        rInfo.svinfo.nblocks=nblocks;
        rInfo.svinfo.ntraces=ntraces;
        rInfo.svinfo.dimafter=dimafter;
        rInfo.svinfo.dimfirst=dimfirst;
        rInfo.svinfo.multi_shot=multi_shot;
        rInfo.svinfo.multi_slice=multi_slice;
        rInfo.svinfo.etl=etl;
        rInfo.svinfo.uslicesperblock=uslicesperblock;
        rInfo.svinfo.slabsperblock=slabsperblock;
        rInfo.svinfo.slicesperblock=slicesperblock;
        rInfo.svinfo.viewsperblock=viewsperblock;
        rInfo.svinfo.within_slices=within_slices;
        rInfo.svinfo.within_views=within_views;
        rInfo.svinfo.within_sviews=within_sviews;
        rInfo.svinfo.slabs=slabs;
        rInfo.svinfo.within_slabs=within_slabs;
        rInfo.svinfo.phase_compressed=phase_compressed;
        rInfo.svinfo.slab_compressed=slab_compressed;
        rInfo.svinfo.slice_compressed=slice_compressed;
        rInfo.svinfo.sliceenc_compressed=sliceenc_compressed;
        rInfo.svinfo.slices=slices;
        rInfo.svinfo.echoes=echoes;
        rInfo.svinfo.epi_seq=epi_seq;
        rInfo.svinfo.threeD=threeD;
        rInfo.svinfo.flash_converted=flash_converted;
        rInfo.svinfo.pc_option=pc_option;
        rInfo.svinfo.epi_dualref=epi_dualref;
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
        rInfo.nwindow=nav_window;
        rInfo.zeropad = zeropad;
        rInfo.zeropad2 = zeropad2;
        rInfo.ro_frq = ro_frq;
        rInfo.pe_frq = pe_frq;
        rInfo.dsize = dsize;
        rInfo.nsize = nsize;
        rInfo.rawflag = rawflag;
        rInfo.sense = sense;
        rInfo.smash = smash;
        rInfo.phsflag = phsflag;

        /* zero everything important */

        (void)memset(slicedata, 0, dsize * sizeof(*slicedata));

        if(pc_option!=OFF)
        {
            (void)memset(pc, 0, slices * echoes * 2 * npts * sizeof(*pc));
        }
        if(epi_dualref && (pc_option==TRIPLE_REF))
        {
            (void)memset(slicedata_neg, 0, dsize * sizeof(*slicedata_neg));
            (void)memset(imdata, 0, dsize * sizeof(*imdata));
            (void)memset(imdata_neg, 0, dsize * sizeof(*imdata_neg));
            if(nchannels)
            {
                (void)memset(imdata2, 0, dsize * sizeof(*imdata2));
                (void)memset(imdata2_neg, 0, dsize * sizeof(*imdata2_neg));
            }
            (void)memset(pc_neg, 0, slices * echoes * 2 * npts * sizeof(*pc_neg));
        }
        if(nnav)
        {
            (void)memset(navdata, 0, nsize * sizeof(*navdata));
            (void)memset(nav_refphase, 0, nsize_ref * sizeof(*nav_refphase));
        }

        if((rawflag==RAW_MAG)||(rawflag==RAW_MP))
        {
            (void)memset(rawmag, 0, (dsize/2) * sizeof(*rawmag));
        }
        if((rawflag==RAW_PHS)||(rawflag==RAW_MP))
        {
            (void)memset(rawphs, 0, (dsize/2) * sizeof(*rawphs));
        }
        if(sense||phsflag)
        {
            (void)memset(imgphs, 0, (dsize/2) * sizeof(*imgphs));
        }

        if(threeD)
        {
            repeat=(int *)recon3Dallocate(slabsperblock*slices*views*echoes*sizeof(int),"recon3D");
            for(i=0;i<slabsperblock*slices*echoes*views;i++)
            *(repeat+i)=-1;
        }
        else
        {
            repeat=(int *)recon3Dallocate(slices*views*echoes*sizeof(int),"recon3D");
            for(i=0;i<slices*echoes*views;i++)
            *(repeat+i)=-1;
        }

        /* remove extra directories */
        (void)strcpy(tempdir,curexpdir);
        (void)strcat(tempdir,"/reconphs");
        (void)sprintf(str,"rm -rf  %s \n",tempdir);
        (void)system(str);

        (void)strcpy(tempdir,curexpdir);
        (void)strcat(tempdir,"/rawphs");
        (void)sprintf(str,"rm -rf  %s \n",tempdir);
        (void)system(str);

        (void)strcpy(tempdir,curexpdir);
        (void)strcat(tempdir,"/rawmag");
        (void)sprintf(str,"rm -rf  %s \n",tempdir);
        (void)system(str);

        (void)strcpy(tempdir,curexpdir);
        (void)strcat(tempdir,"/recon");
        (void)sprintf(str,"rm -rf  %s \n",tempdir);
        (void)system(str);

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
        epi_dualref=rInfo.svinfo.epi_dualref;
        slice_reps=rInfo.svinfo.slice_reps;
        nblocks=rInfo.svinfo.nblocks;
        ntraces=rInfo.svinfo.ntraces;
        dimafter=rInfo.svinfo.dimafter;
        dimfirst=rInfo.svinfo.dimfirst;
        multi_shot=rInfo.svinfo.multi_shot;
        multi_slice=rInfo.svinfo.multi_slice;
        etl=rInfo.svinfo.etl;
        slicesperblock=rInfo.svinfo.slicesperblock;
        slabs=rInfo.svinfo.slabs;
        slabsperblock=rInfo.svinfo.slabsperblock;
        uslicesperblock=rInfo.svinfo.uslicesperblock;
        viewsperblock=rInfo.svinfo.viewsperblock;
        within_slices=rInfo.svinfo.within_slices;
        within_views=rInfo.svinfo.within_views;
        within_sviews=rInfo.svinfo.within_sviews;
        within_slabs=rInfo.svinfo.within_slabs;
        phase_compressed=rInfo.svinfo.phase_compressed;
        slice_compressed=rInfo.svinfo.slice_compressed;
        slab_compressed=rInfo.svinfo.slab_compressed;
        sliceenc_compressed=rInfo.svinfo.sliceenc_compressed;
        views=rInfo.picInfo.npe;
        nro=rInfo.picInfo.nro;
        ro_size=rInfo.svinfo.ro_size;
        pe_size=rInfo.svinfo.pe_size;
        pe2_size=rInfo.svinfo.pe2_size;
        slices=rInfo.svinfo.slices;
        echoes=rInfo.svinfo.echoes;
        epi_seq=rInfo.svinfo.epi_seq;
        threeD=rInfo.svinfo.threeD;
        flash_converted=rInfo.svinfo.flash_converted;
        imglen=rInfo.imglen;
        ntlen=rInfo.ntlen;
        epi_rev=rInfo.epi_rev;
        read_window=rInfo.rwindow;
        phase_window=rInfo.pwindow;
        nav_window=rInfo.nwindow;
        zeropad=rInfo.zeropad;
        zeropad2=rInfo.zeropad2;
        ro_frq=rInfo.ro_frq;
        pe_frq=rInfo.pe_frq;
        dsize=rInfo.dsize;
        nsize=rInfo.nsize;        
        rawflag=rInfo.rawflag;
        sense=rInfo.sense;
        smash=rInfo.smash;
        phsflag=rInfo.phsflag;
    }
    
    reallybig=FALSE;
    if(threeD)
        if(slices*ro_size*pe_size >= BIG3D)
            reallybig=TRUE;
    
    if(realtime_block>=nblocks)
    {
        (void)recon3D_abort();
        ABORT;
    }

    if(ntlen)
    {
        if(phase_compressed)
        within_nt=nblocks/ntlen;
        else
        within_nt=etl*nblocks/(nv*ntlen);
    }
    else
    {
        Werrprintf("recon3D: ntlen equal to zero");
        (void)recon3D_abort();
        ABORT;
    }
    slice=0;
    view=0;
    echo=0;
    nshots=views/etl;
    nshots=pe_size/etl;
    if(threeD &&(pe2_size<slices))
    sview_zf=TRUE;


     
     if ( (error = D_getbuf(D_USERFILE, nblocks, realtime_block, &inblock)) )
        {
		Werrprintf("recon3D: error reading block 0");
		(void)recon3D_abort();
		ABORT;
	}
	bhp = inblock.head;
	ctcount = (int) (bhp->ctcount);
	
	if (within_nt)
		nt=nts[(realtime_block/within_nt)%ntlen];
	else {
		Werrprintf("recon3D: within_nt equal to zero");
		(void)recon3D_abort();
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
            Werrprintf("recon3D: aborting by request");
            (void)recon3D_abort();
            ABORT;
        }
        
        cblockdata=inblock.data; /* save pointer to data */
        
        blockctr=realtime_block++;
        if(realtime_block>=nblocks)
            acq_done=TRUE;
        
        if(reallybig) {
            sprintf(str,"reading block %d\n",blockctr);
            Werrprintf(str);
        }
  
        
        /* reset nt */
        if(within_nt)
        nt=nts[(realtime_block/within_nt)%ntlen];
        else
        {
            Werrprintf("recon3D: within_nt equal to zero");
            (void)recon3D_abort();
            ABORT;
        }

        floatstatus = bhp->status & S_FLOAT;
      
        /* get dc offsets if necessary */
        if(rInfo.dc_flag)
        {
            real_dc=IMAGE_SCALE*(bhp->lvl);
            imag_dc=IMAGE_SCALE*(bhp->tlt);
        }

        /* point to header of next block and get new ctcount */
	if (!acq_done) {
	  if ( (error = D_getbuf(D_USERFILE, nblocks,
                                 realtime_block, &inblock)) )
                {
				(void)sprintf(str, "recon3D: D_getbuf error for block %d\n",
						realtime_block);
				Werrprintf(str);
				(void)recon3D_abort();
				ABORT;
		}
			bhp = inblock.head;
			ctcount = (int) (bhp->ctcount);
		}

        if(epi_seq)
        {
            if(dimafter)
            icnt=(blockctr/dimafter)%(imglen)+1;
            else
            {
                Werrprintf("recon3D:dimafter equal to zero");
                (void)recon3D_abort();
                ABORT;
            }
            error=P_getreal(PROCESSED,"image",&rimflag,icnt);
            if(error)
            {
                Werrprintf("recon3D: Error getting image element");
                (void)recon3D_abort();
                ABORT;
            }
            /* set flag to describe data block */
            imflag=imflag_neg=pcflag=pcflag_neg=FALSE;
            if(rimflag==1.0)
            imflag=TRUE;
            else
            imflag=FALSE;
            if(rimflag==-1.0)
            imflag_neg=TRUE;
            else
            imflag_neg=FALSE;
            if(rimflag==-2.0)
            pcflag_neg=TRUE;
            else
            pcflag_neg=FALSE;
            if(rimflag==0.0)
            pcflag=TRUE;
            else
            pcflag=FALSE;
        }
        else
        imflag=TRUE;

        /* point to phase & image indicator */
        if(pc_option!=OFF)
        {
            pcd=pc_done;
            if(epi_dualref && (imflag_neg || pcflag_neg))
            pcd=pcneg_done;
        }
       
        if((imflag)||(pc_option!=OFF))
        {
            /* start of raw data */
            sdataptr=NULL;
            idataptr=NULL;
            fdataptr=NULL;
            if (rInfo.ebytes==2)
            sdataptr=(short *)cblockdata;
            else if (floatstatus == 0)
            idataptr=(int *)cblockdata;
            else
            fdataptr=(float *)cblockdata;

            if(((ro_size<nro)||(pe_size<views)||sview_zf)&&(phase_compressed)&&(!threeD))
            {
                (void)memset(slicedata, 0, (dsize) * sizeof(*slicedata));
                if(epi_dualref && (pc_option==TRIPLE_REF))
                (void)memset(slicedata_neg, 0, (dsize) * sizeof(*slicedata_neg));
                if(nnav)
                (void)memset(navdata, 0, (nsize) * sizeof(*navdata));
                if((rawflag==RAW_MAG)||(rawflag==RAW_MP))
                (void)memset(rawmag, 0, (dsize/2) * sizeof(*rawmag));
                if((rawflag==RAW_PHS)||(rawflag==RAW_MP))
                (void)memset(rawphs, 0, (dsize/2) * sizeof(*rawphs));
                if(sense||phsflag)
                (void)memset(imgphs, 0, (dsize/2) * sizeof(*imgphs));
            }

            if(imflag)
            {
                *fdfstr='\0';
                if(rInfo.narray)
                (void)arrayfdf(blockctr, rInfo.narray, rInfo.arrayelsP, fdfstr);
            }

            for(itrc=0;itrc<ntraces;itrc++)
            {
                /* figure out where to put this echo */
                navflag=FALSE;
                status=svcalc(itrc,blockctr,&(rInfo.svinfo),&view,&slice,&echo,&nav,&slab);
                if(status)
                {
                    (void)recon3D_abort();
                    ABORT;
                }

                oview=view;
                slab=0;

                if(nav>-1)
                navflag=TRUE;

                if((imflag_neg||imflag)&&multi_shot&&!navflag)
                {
                    view=view_order[view];
                    if(sview_order&&threeD)
                    slice=sview_order[view];
                }

                if(navflag&&!imflag&&!imflag_neg)
                {
                    /* skip this echo and advance data pointer */
                    if (sdataptr)
                    sdataptr+=2*nav_pts;
                    else if(idataptr)
                    idataptr+=2*nav_pts;
                    else
                    fdataptr+=2*nav_pts;
                }
                else if((imflag_neg || pcflag_neg)&& (pc_option != TRIPLE_REF))
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
                    if(!threeD)
                    {
                        /* which unique slice is this? */
                        uslice=slice;
                        if(uslicesperblock != slicesperblock)
                        {
                            i=-1;
                            j=-1;
                            while((j<slice)&&(i<uslicesperblock))
                            {
                                i++;
                                j+=blockreps[i];
                                uslice=i;
                            }
                        }
                    }

                    /* evaluate repetition number */
                    if(imflag&&!navflag)
                    {
                        if(threeD)
                        {
                            rep= *(repeat+slab*slices*views*echoes+slice*views*echoes+view*echoes+echo);
                            rep++;
                            rep=rep%(slab_reps);
                            *(repeat+slab*slices*views*echoes+slice*views*echoes+view*echoes+echo)=rep;
                        }
                        else
                        {
                            rep= *(repeat+slice*views*echoes+view*echoes+echo);
                            rep++;
                            rep=rep%(slice_reps);
                            *(repeat+slice*views*echoes+view*echoes+echo)=rep;
                        }
                    }

                    if(imflag||imflag_neg)
                    {
                        im_slice=slice;
                        if(!navflag&&(pc_option!=OFF))
                        *(pcd+slice)=(*(pcd+slice)) | IMG_DONE;
                    }
                    else
                    pc_slice=slice;

                    if(threeD)
                    {
                        if(imflag||imflag_neg)
                        im_slice=slab;
                        else
                        pc_slice=slab;
                    }

                    /* set up pointers & offsets  */
                    if(!navflag)
                    {
                        nro2=nro;
                        ro2=ro_size;
                        npts=views*nro2;
                        npts3d=npts*slices;
                        datptr=slicedata;
                        if(imflag_neg)
                        datptr=slicedata_neg;
                        if(threeD)
                        {
                            soffset=rep;
                            soffset*=echoes;
                            soffset+=echo;
                            soffset*=slices;
                            soffset+=slice;
                            soffset*=views;
                            soffset+=view;
                            soffset*=(2*nro);
                            /*
                             soffset = rep*echoes*2*npts3d +
                             echo*2*npts3d + slice*2*npts + view*2*nro;
                             
                             
                             soffset=view*2*nro+echo*npts*2+rep*echoes*npts*2+slice*slice_reps*echoes*2*npts; 			
                             */
                            
                            /* soffset into this block only! */
                            if(phase_compressed)
                             soffset=view*2*nro+echo*npts*2;
                             else  if(sliceenc_compressed)
                             soffset=slice*2*nro+echo*npts*2;
                        }
                        else
                        {
                            if(phase_compressed)
                            soffset=view*2*nro+echo*npts*2+(slice%slicesperblock)*echoes*npts*2;
                            else
                            soffset=view*2*nro+echo*npts*2+rep*echoes*npts*2+slice*slice_reps*echoes*2*npts;
                        }
                    }
                    else
                    {
                        nro2=nro;
                        ro2=nav_pts;
                        datptr=navdata;
                        if(phase_compressed)
                        soffset=nav*2*nro2+echo*nro2*nshots*nnav*2+
                        (slice%slicesperblock)*echoes*nro2*nshots*nnav*2;
                        else
                        soffset=nav*2*nro2+echo*nshots*nnav*nro2*2+rep*echoes*nnav*nshots*nro2*2+
                        slice*slice_reps*echoes*nnav*nshots*2*nro2;
                    }
                    

                    /* scale and convert to float */
                    soffset+=(nro2-ro2);

					if (sdataptr) {
						for (iro=0; iro<2*ro2; iro++)
							*(datptr+soffset+iro)=IMAGE_SCALE*(*sdataptr++);
					} else if (idataptr) {
						for (iro=0; iro<2*ro2; iro++)
							*(datptr+soffset+iro)=IMAGE_SCALE*(*idataptr++);
					} else {
						for (iro=0; iro<2*ro2; iro++)
							*(datptr+soffset+iro)=IMAGE_SCALE*(*fdataptr++);
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
                    if(epi_rev || rInfo.alt_readrev_flag)
                    {
                        reverse=FALSE;
                        if((rInfo.alt_readrev_flag)&&(echo%2))
                        reverse=TRUE;
                        else if(epi_rev)
                        {
                            if((imflag||pcflag)&&(oview%2))
                            reverse=TRUE;
                            if((imflag_neg||pcflag_neg)&&((oview%2)==0))
                            reverse=TRUE;
                        }
                        /* reverse if even and positive readout OR if odd and negative readout */
                        if(reverse)
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
                    }

                    if(rawflag&&imflag&&(!navflag))
                    {
                        /* raw data must be written transposed */
                        roffset=soffset/2-view*nro2+view;
                        switch(rawflag)
                        {
                            case RAW_MAG:
                            for(iro=0;iro<nro2;iro++)
                            {
                                m1=(double)*(datptr+soffset+2*iro);
                                m2=(double)*(datptr+soffset+2*iro+1);
                                dtemp=sqrt(m1*m1+m2*m2);
                                rawmag[roffset+iro*views]=(float)dtemp;
                            }
                            break;
                            case RAW_PHS:
                            for(iro=0;iro<nro2;iro++)
                            {
                                m1=(double)*(datptr+soffset+2*iro);
                                m2=(double)*(datptr+soffset+2*iro+1);
                                dtemp=atan2(m2,m1);
                                rawphs[roffset+iro*views]=(float)dtemp;
                            }
                            break;
                            case RAW_MP:
                            for(iro=0;iro<nro2;iro++)
                            {
                                m1=(double)*(datptr+soffset+2*iro);
                                m2=(double)*(datptr+soffset+2*iro+1);
                                dtemp=sqrt(m1*m1+m2*m2);
                                rawmag[roffset+iro*views]=(float)dtemp;
                                dtemp=atan2(m2,m1);
                                rawphs[roffset+iro*views]=(float)dtemp;
                            }
                            break;
                            default:
                            break;
                        }
                    }

                    /*
                     if((recon_window>NOFILTER)&&(recon_window<=MAX_FILTER))
                     */
                    if(read_window)
                    {
                        pt1=datptr+soffset+(nro2-nro);
                        wptr=read_window;
                        for(iro=0;iro<nro;iro++)
                        {
                            *pt1 *= *wptr;
                            pt1++;
                            *pt1 *= *wptr;
                            pt1++;
                        }
                    }

                    if(navflag&&nav_window)
                    {
                        pt1=datptr+soffset+(nro2-nro);
                        wptr=nav_window;
                        for(iro=0;iro<nro;iro++)
                        {
                            *pt1 *= *wptr;
                            pt1++;
                            *pt1 *= *wptr;
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
                        if(ro_frqP&&nmice)
                        {
                            ichan= blockctr%nmice;
                            if((rInfo.alt_readrev_flag)&&echo%2)
                            (void)rotate_fid(nrptr, 0.0, -1*ro_frqP[ichan], 2*nro2, COMPLEX);
                            else
                            (void)rotate_fid(nrptr, 0.0, ro_frqP[ichan], 2*nro2, COMPLEX);
                        }
                        /*
                         else if(ro_frq!=0.0)
                         {
                         if((rInfo.alt_readrev_flag)&&echo%2)
                         (void)rotate_fid(nrptr, 0.0, -1*ro_frq, 2*nro2, COMPLEX);
                         else
                         (void)rotate_fid(nrptr, 0.0, ro_frq, 2*nro2, COMPLEX);
                         }
                         */

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
                    if(!navflag&&(imflag||imflag_neg)&&(pc_option!=OFF))
                    {
                        pc_temp=pc;
                        if(imflag_neg)
                        pc_temp=pc_neg;
                        if((pc_option!=OFF)&&(*(pcd+im_slice) & PHS_DONE))
                        {
                            /* pc_offset=im_slice*echoes*npts*2+echo*npts*2+view*nro*2; */
                            pc_offset=im_slice*echoes*npts*2+echo*npts*2+oview*nro*2;
                            if(threeD)
                            pc_offset=im_slice*echoes*npts3d*2+echo*npts3d*2+oview*nro*2;
                            pt1=datptr+soffset;
                            pt2=pt1+1;
                            pt3=pc_temp+pc_offset;
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
                } /* end if image or not navigator */
            } /* end of loop on traces */
            
            
            if ( (error = D_release(D_USERFILE, (realtime_block-1))) ) {
     				(void)sprintf(str, "recon3D: D_release error for block %d\n",
     						realtime_block-1);
     				Werrprintf(str);
     				(void)recon3D_abort();
     				ABORT;
     			}
			
            pcslice=rInfo.pc_slicecnt;
            /* compute phase correction */
            if((pcflag||(pcflag_neg&&(pc_option==TRIPLE_REF)))&&(pc_option!=OFF))
            {
                phase_correct=pc_option;
                if(pc_option==TRIPLE_REF)
                phase_correct=POINTWISE;
                pc_temp=pc;
                if(pcflag_neg)
                pc_temp=pc_neg;

                if(phase_compressed)
                {
                    fptr=slicedata;
                    if(multi_shot&&((pc_option==CENTER_PAIR)||(pc_option==FIRST_PAIR)))
                    {
                        for (ispb=0;ispb<slicesperblock*echoes;ispb++)
                        {
                            *(pcd+rInfo.pc_slicecnt)=(*(pcd+rInfo.pc_slicecnt)) | PHS_DONE;
                            pc_offset=rInfo.pc_slicecnt*npts*2;
                            /* 	pc_offset+=(views-pe_size)*nro2; */
                            for(it=0;it<(pe_size/etl);it++)
                            {
                                (void)pc_calc(fptr, (pc_temp+pc_offset),
                                nro2, etl, phase_correct, transposed);
                                pc_offset += 2*nro2*etl;
                            }
                            rInfo.pc_slicecnt++;
                            rInfo.pc_slicecnt=rInfo.pc_slicecnt%(slices*echoes);
                            fptr+=2*npts;
                        }
                    }
                    else
                    {
                        for (ispb=0;ispb<slicesperblock*echoes;ispb++)
                        {
                            *(pcd+rInfo.pc_slicecnt)=(*(pcd+rInfo.pc_slicecnt)) | PHS_DONE;
                            pc_offset=rInfo.pc_slicecnt*npts*2;
                            /* 	pc_offset+=(views-pe_size)*nro2; */
                            (void)pc_calc(fptr, (pc_temp+pc_offset),
                            nro2, pe_size, phase_correct, transposed);
                            rInfo.pc_slicecnt++;
                            rInfo.pc_slicecnt=rInfo.pc_slicecnt%(slices*echoes);
                            fptr+=2*npts;
                        }
                    }
                }
            }

            /* if triple reference negative phase data and already have corresponding image data, do phase correction and continue */
            if(pcflag_neg && (pc_option==TRIPLE_REF) && ( *(pcneg_done+pcslice) & IMG_DONE))
            {
                pcflag_neg=FALSE;
                imflag_neg=TRUE;
                pc_temp=pc_neg; /* location of phase correction data */

                pt1=slicedata_neg; /* location of phase encoded negative readout reference data */
                pt2=pt1+1;
                pt5=pt1; /* where to put result */
                pt6=pt5+1;

                for (ispb=0;ispb<slicesperblock*echoes;ispb++)
                {
                    for (iview=0;iview<pe_size;iview++)
                    {
                        oview=iview;
                        /* figure out which unsorted view it is */
                        if(multi_shot && view_order)
                        {
                            oview=0;
                            while(iview != view_order[oview])
                            oview++;
                        }
                        pc_offset=(pcslice+ispb)*echoes*npts*2+echo*npts*2+oview*nro*2;

                        pt3=pc_temp+pc_offset;
                        pt4=pt3+1;
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
            }

            if(threeD && (phase_compressed || sliceenc_compressed)) /* transform, then write to temp file */
            {
                ichan= blockctr%rInfo.nchannels;
				if(ichan==(rInfo.nchannels-1))
				lastchan=TRUE;
				else
					lastchan=FALSE;

				magoffset=0;
				roffset=0;
				fptr=slicedata;
				if (imflag_neg)
					fptr=slicedata_neg;
				if (nnav)
					datptr=navdata;
		
				if (phase_compressed)
					ndir2=views;
				else
					ndir2=slices;
				
                for(iecho=0;iecho<echoes;iecho++)
                 {
                     if(nnav&&(nav_option!=OFF))
                     {
                         if(rep==0)
                         (void)getphase(0,nro,nshots*nnav,nav_refphase+roffset,datptr,FALSE);

                         nav_correct(nav_option,fptr,nav_refphase+roffset,nro,pe_size,datptr,
                         &(rInfo.svinfo));

                         roffset+=nro;
                     }

                     phase_reverse=FALSE;
                     if(rInfo.phsrev_flag)
                     phase_reverse=TRUE;
                     if((rInfo.alt_phaserev_flag)&&iecho%2)
                     {
                         if(phase_reverse)
                         phase_reverse=FALSE;
                         else
                         phase_reverse=TRUE;
                     }

                     if(nmice&&pe_frqP)
                     pe_frq=pe_frqP[ichan];

     
                     /* this is the usual case */
                     (void)temp_ft(fptr,nro,ndir2,phase_window,magnitude,phase_reverse,
                             zeropad,zeropad2,ro_frq,pe_frq,NULL, phase_compressed, blockctr);
                     
   
                 } /* end of echo loop */
                 rInfo.slicecnt++;
                 rInfo.slicecnt=rInfo.slicecnt%slices;

            }
 
        } /* end if imflag or doing phase correction */

  
    }
    /**************************/
    /* end loop on blocks    */
    /**************************/
    if(acq_done)
    {
      D_close(D_USERFILE);        
        *fdfstr='\0';
 

        if(threeD) /* process third dimension and write out images */
        	  (void)generate_imagesBig3D(fdfstr);

        (void)releaseAllWithId("recon3D");
        
#ifdef VNMRJ		
        (void)aipUpdateRQ();
#endif
       
        
  
        
    }

    return(0);
}


/******************************************************************************/
/* this is the "Big" 3D version                                                */
/* it will do the 3rd direction FT, then write out separate 2D fdfs            */
/******************************************************************************/
static int generate_imagesBig3D(char *arstr) {
    vInfo info;
    float ro_frq, pe_frq;
    float *fptr;
    float *window;
    int slices, views, nro, slice_reps, nchannels;
    int multi_shot, *dispcntptr, dispint, zeropad, zeropad2;
    int phsflag;
    int threeDslab;
    int itemp;
    int rawflag, smash, sense;
    int i, j;
    int blkreps;
    int ibr;
    int uslices;
    int uslice;
    int echoes;
    int urep;
    int nshifts;
    int slice, rep, ipt;
    int ichan, irep;
    int fnchan;
    int echo;
    int phase_reverse;
    int display;
    int slabs;
    int status;
    int npts2d, npts3d;
    int islab, islice;
    int error;
    int choffset;
    int offset;
    int lastchan;
    int ch0=0;
    int ix, iy, iz;
    int nx;
    int ny, nz, pwr, fnt;
    int imouse=0;
    int reallybig=FALSE;
    int toffset;
    long long lptr;
    char tempdir[MAXPATHL];
    char str[200];
    double *pe2_frq;
    double dtemp;
    double dfov;
    double pefrq;
    double a, b, c;
    float *nrptr;
    float *fptr1, *fptr2;
	float templine[2*MAXPE];
   	float *imagedata, *magdata;
   	float *frq_pe2=NULL;
   	float frq;
   	float *inplane;
   	float *rotplane;
      
   
    blkreps=FALSE;

    /* get relevant parameters from struct */
    slices=rInfo.svinfo.slices;
    views=rInfo.picInfo.npe;
    nro=rInfo.picInfo.nro;
    slice_reps=rInfo.svinfo.slice_reps;
    nchannels=rInfo.nchannels;
    window=rInfo.pwindow;
    multi_shot=rInfo.svinfo.multi_shot;
    dispcntptr=&(rInfo.dispcnt);
    dispint=rInfo.dispint;
    zeropad=rInfo.zeropad;
    zeropad2=rInfo.zeropad2;
    ro_frq=rInfo.ro_frq;
    pe_frq=rInfo.pe_frq;
    rawflag=rInfo.rawflag;
    sense=rInfo.sense;
    smash=rInfo.smash;
    phsflag=rInfo.phsflag;
    echoes=rInfo.svinfo.echoes;
    slabs=rInfo.svinfo.slabs;
    pefrq=pe_frq;

    uslices=slices;
    nx=nro;
    npts2d=views*nro;
    npts3d=npts2d*slices;
    fnchan=(smash|sense) ? nchannels:1;
    choffset=(smash|sense) ? 1:nchannels;
    
    imagedata=(float *)recon3Dallocate(2*npts2d*sizeof(float), "generate_imagesBig3D");
    magdata=(float *)recon3Dallocate(npts2d*sizeof(float), "generate_imagesBig3D");
    
    if (temp3D_file == NULL) {
		Werrprintf("recon3D: cannot open temp file ");
		(void)recon3D_abort();
		ABORT;
	}

	if (rInfo.svinfo.phase_compressed) {
		ny=views;
		nz=slices;
	} else {
		ny=slices;
		nz=views;
	}
	
	if (slices*views*nro >= BIG3D)
		reallybig=TRUE;
         
    if (blockreps) {
        for (i=0; i<slices; i++) {
            if (blockreps[i]>0) {
                blkreps=TRUE;
                uslices=i;
            }
        }
    }


	if (rInfo.svinfo.sliceenc_compressed) {
		inplane=(float *)allocateWithId(2*nx*nz*sizeof(float),
				"generate_imagesBig3D");
		rotplane=(float *)allocateWithId(2*nx*nz*sizeof(float),
				"generate_imagesBig3D");
	}
	
    if (blockrepeat)
        for (slice=0; slice<slices; slice++)
            blockrepeat[slice]=-1;
    
    pe2_frq=NULL;
    error=P_getVarInfo(CURRENT,"ppe2",&info);
    if (!error && info.active) {
        nshifts=info.size;
        if (nshifts != slabs) {
            Werrprintf("recon3D: length of ppe2 less than slabs ");
            (void)recon3D_abort();
            ABORT;
        }
        pe2_frq=(double *)allocateWithId(slabs*sizeof(double), "generate_imagesBig3D");

        error=P_getreal(PROCESSED,"lpe2",&dfov,1);
        if (error) {
            Werrprintf("recon3D: Error getting lpe2");
            (void)recon3D_abort();
            ABORT;
        }

        for (islab=0; islab<slabs; islab++) {
            error=P_getreal(CURRENT,"ppe2",&dtemp,islab+1);
            if (error) {
                Werrprintf("recon3D: Error getting ppe2 ");
                (void)recon3D_abort();
                ABORT;
            }
            pe2_frq[islab]=-180.0*dtemp/dfov;
        }
    }

    offset=0;
    lptr=0;
    (void)rewind(temp3D_file);
    
    for (islab=0; islab<slabs; islab++) {
        /* which unique slice is this? */
        uslice=slice;
        urep=0;
        ibr=1;
        if (blkreps) {
            i=-1;
            j=-1;
            while ((j<slice)&&(i<uslices)) {
                urep=slice-j-1;
                i++;
                j+=blockreps[i];
                uslice=i;
            }
            urep= blockrepeat[uslice];
            urep++;
            blockrepeat[uslice]=urep;
            ibr=blockreps[uslice];
        }
        for (rep=0; rep<(slice_reps/nchannels); rep++) {
            for (echo=0; echo<echoes; echo++) {
                phase_reverse=FALSE;
                if (rInfo.phsrev_flag)
                    phase_reverse=TRUE;
                if ((rInfo.alt_phaserev_flag)&&echo%2) {
                    if (phase_reverse)
                        phase_reverse=FALSE;
                    else
                        phase_reverse=TRUE;
                }

                for (ichan=0; ichan<nchannels; ichan++) {
                    if (ichan==(rInfo.nchannels-1))
                    lastchan=TRUE;
                    else
                    lastchan=FALSE;
                    if (lastchan) {
                    	/* most basic case: do 3rd dimension ft on data stored in temp3D_file */
                    	pwr = 4;
						fnt = 32;
						while (fnt < 2*nz) {
							fnt *= 2;
							pwr++;
						}
						for (iy=0; iy<ny; iy++) {
							if (reallybig) {
								sprintf(str, "third ft number %d\n", iy);
								Werrprintf(str);
							}
							
							if(rInfo.svinfo.sliceenc_compressed) {
								/* read in 2D plane */
								lptr = iy*2*nro*views;
								lptr *= sizeof(float);
								(void)fseek(temp3D_file,lptr,SEEK_SET);
								(void)fread(inplane, sizeof(float), 2*views*nro, temp3D_file);	
							}

							for (ix=0; ix<nx; ix++) {
								nrptr=templine;
								if (rInfo.svinfo.phase_compressed) {
									/* get the kz line */
									offset = 2*ix + 2*nx*iy;
									for (iz=0; iz<nz; iz++) {
										lptr=offset + iz*2*npts2d;
										lptr *= sizeof(float);
										(void)fseek(temp3D_file, lptr, SEEK_SET);
										(void)fread(nrptr, sizeof(float), 2,
												temp3D_file);
										nrptr += 2;
									}
								} else if (rInfo.svinfo.sliceenc_compressed) {
									toffset=ix;
									for (iz=0; iz<nz; iz++) {
										templine[2*iz]=*(inplane+2*toffset);
										templine[2*iz+1]=*(inplane+2*toffset+1);
										toffset += nx;
									}
								}
								
								/* apply frequency shift */
								if (frq_pe2!=NULL) {
									frq=*(frq_pe2+imouse);
									(void)rotate_fid(templine, 0.0, frq, 2*nz,
											COMPLEX);
								}
								nrptr=templine;
								(void)fft(nrptr, nz, pwr, 0, COMPLEX, COMPLEX,
										-1.0, 1.0, nz);
								/* write it back */
								if (rInfo.svinfo.phase_compressed) {
									(void)fseek(temp3D_file, lptr, SEEK_SET);
									nrptr=templine;
									for (iz=0; iz<nz; iz++) {
										lptr=offset + iz*2*npts2d;
										lptr *= sizeof(float);
										(void)fseek(temp3D_file, lptr, SEEK_SET);
										(void)fwrite(nrptr, sizeof(float), 2,
												temp3D_file);
										nrptr += 2;
									}
								} else if (rInfo.svinfo.sliceenc_compressed) {
									toffset=ix*nz;
									for (iz=0; iz<nz; iz++) {
										*(rotplane+2*toffset) = templine[2*(nz-iz-1)];
										*(rotplane+2*toffset+1) = templine[2*(nz-iz-1) + 1];
										toffset++;
									}
								}
							} /* ix loop */

							if (rInfo.svinfo.sliceenc_compressed) {
								/* write back the 2D plane */
								lptr = iy*2*nro*views;
								lptr *= sizeof(float);
								(void)fseek(temp3D_file, lptr, SEEK_SET);
								(void)fwrite(rotplane, sizeof(float), 2*views
										*nro, temp3D_file);
							}		
	
						} /* iy loop */
                    }  /* last channel */            
                } /* end ichan loop */
               
                (void)rewind(temp3D_file);        
				/*  output 2D fdf files for 3D */
				if (TRUE||sense||rawflag||phsflag) {
                    for (islice=0; islice<slices; islice++) { /* slice loop */
                        rInfo.picInfo.echo=echo+1;
                        threeDslab=islab + 1;
                        rInfo.picInfo.slice=islab*slices + islice + 1;
                        /* 		  rInfo.picInfo.slice=uslice+1;*/
                        irep=rep*ibr + urep +1;
                        
                        
                        arstr[0]='\0';
                        if(rInfo.narray)
                              (void)arrayfdf(irep-1, rInfo.narray, rInfo.arrayelsP, arstr);
                        
                        /* create the magnitude data */
                        (void)fread(imagedata, sizeof(float), 2*npts2d, temp3D_file);
                        fptr1=imagedata;
                        fptr2=magdata;
                        for(ipt=0;ipt<npts2d;ipt++)
                        {
                        		a=(double)*fptr1++;
                        		b=(double)*fptr1++;
                        		c=sqrt(a*a+b*b);
                        		*fptr2++=(float)c;
                        }

                        if (rawflag) {
                            (void)strcpy(tempdir, rInfo.picInfo.imdir);
                            itemp=rInfo.picInfo.fullpath;
                            rInfo.picInfo.fullpath=FALSE;
                            if ((rawflag==RAW_MAG)||(rawflag==RAW_MP)) {
                                (void)strcpy(rInfo.picInfo.imdir, "rawmag");
                                display=DISPFLAG(rInfo.dispcnt, rInfo.dispint);
                                rInfo.dispcnt++;
                                if (smash)
                                    status=write_fdf(irep, rawmag+offset,
                                            &(rInfo.picInfo),
                                            &(rInfo.rawmag_order), display,
                                            arstr, threeDslab, (ichan+1));
                                else if (lastchan)
                                    status=write_fdf(irep, rawmag+offset,
                                            &(rInfo.picInfo),
                                            &(rInfo.rawmag_order), display,
                                            arstr, threeDslab, ch0);
                                if (status) {
                                    (void)recon3D_abort();
                                    ABORT;
                                }
                            }
                            if ((rawflag==RAW_PHS)||(rawflag==RAW_MP)) {
                                (void)strcpy(rInfo.picInfo.imdir, "rawphs");
                                display=DISPFLAG(rInfo.dispcnt, rInfo.dispint);
                                rInfo.dispcnt++;
                                if (smash)
                                    status=write_fdf(irep, rawphs+offset,
                                            &(rInfo.picInfo),
                                            &(rInfo.rawphs_order), display,
                                            arstr, threeDslab, (ichan+1));
                                else if (lastchan)
                                    status=write_fdf(irep, rawphs+offset,
                                            &(rInfo.picInfo),
                                            &(rInfo.rawphs_order), display,
                                            arstr, threeDslab, ch0);
                                if (status) {
                                    (void)recon3D_abort();
                                    ABORT;
                                }
                            }
                            (void)strcpy(rInfo.picInfo.imdir, tempdir);
                            rInfo.picInfo.fullpath=itemp;
                        } /* end if raw flag */

                        if (sense) {
                            (void)strcpy(tempdir, rInfo.picInfo.imdir);
                            itemp=rInfo.picInfo.fullpath;
                            rInfo.picInfo.fullpath=FALSE;
                            (void)strcpy(rInfo.picInfo.imdir, "reconphs");
                            status=write_fdf(irep, imgphs+offset,
                                    &(rInfo.picInfo), &(rInfo.phase_order),
                                    display, arstr, threeDslab, (ichan+1));
                            (void)strcpy(rInfo.picInfo.imdir, tempdir);
                            rInfo.picInfo.fullpath=itemp;

                            /* write magnitude image data */
                            status=write_fdf(irep, magnitude+offset,
                                    &(rInfo.picInfo), &(rInfo.phase_order),
                                    display, arstr, threeDslab, (ichan+1));
                            if (status) {
                                (void)recon3D_abort();
                                ABORT;
                            }
                            /* restore imgdir */
                            (void)strcpy(rInfo.picInfo.imdir, tempdir);
                            rInfo.picInfo.fullpath=itemp;
                        }

                        if (phsflag && lastchan) {
                            (void)strcpy(tempdir, rInfo.picInfo.imdir);
                            itemp=rInfo.picInfo.fullpath;
                            rInfo.picInfo.fullpath=FALSE;
                            (void)strcpy(rInfo.picInfo.imdir, "reconphs");
                            status=write_fdf(irep, imgphs+offset,
                                    &(rInfo.picInfo), &(rInfo.phase_order),
                                    display, arstr, threeDslab, ch0);
                            (void)strcpy(rInfo.picInfo.imdir, tempdir);
                            rInfo.picInfo.fullpath=itemp;
                            if (status) {
                                (void)recon3D_abort();
                                ABORT;
                            }
                            /* restore imgdir */
                            (void)strcpy(rInfo.picInfo.imdir, tempdir);
                            rInfo.picInfo.fullpath=itemp;
                        }

                        if (nchannels>1) {
                            for (ipt=0; ipt<npts2d; ipt++)
                                magnitude[offset+ipt]/=nchannels;
                        }
                        display=DISPFLAG(rInfo.dispcnt, rInfo.dispint);
                        rInfo.dispcnt++;

                        status=write_fdf(irep, magdata,
                                &(rInfo.picInfo), &(rInfo.image_order),
                                display, arstr, threeDslab, ch0);
                        if (status) {
                            (void)recon3D_abort();
                            ABORT;
                        }
                        
                    } /* end slice loop */
                }/* end if dispint */
            } /* end of echo loop */
        } /* end of rep loop */
    }/* end of slab loop */

    (void)fclose(temp3D_file);
	(void)sprintf(str, "rm -f %s/recon/temp3D \n", curexpdir);
	(void)system(str);
      
      
    /* get rid of any fdf files first */
    (void)sprintf(str,
            "%s/datadir3d/data/",curexpdir);
    (void)unlinkFilesWithSuffix(str, ".fdf");

    
    /* create a big fdf containing all data  for aipExtract */
    /* need to reverse readout and phase directions for this */
    rInfo.picInfo.echoes=echoes;
    rInfo.picInfo.datatype=MAGNITUDE;
    fptr=slicedata;
    for (islab=0; islab<0*slabs; islab++) {
        rInfo.picInfo.slice=islab+1;
        for (rep=0; rep<(slice_reps/nchannels); rep++) {
            rInfo.picInfo.image=(float)(rep+1.0);
            for (echo=0; echo<echoes; echo++) {
                rInfo.picInfo.echo=echo+1;
                for (ichan=0; ichan<fnchan; ichan++) {
                    status=write_3Dfdf(fptr, &(rInfo.picInfo), arstr,ichan);
                    if (status) {
                        (void)recon3D_abort();
                        ABORT;
                    }
                    fptr+=2*npts3d*choffset;
                }
            }
        }
    }    
    
    
    /* copy first image 3D fdf to data.fdf in case someone needs it there */
    (void)sprintf(str,
            "cp %s/datadir3d/data/img_slab001image001echo001.fdf %s/datadir3d/data/data.fdf \n",
            curexpdir, curexpdir);
    (void)system(str);

 
    /* do all that for raw magnitude if needed */
    if ((rawflag==RAW_MAG) || (rawflag==RAW_MP)) {
        rInfo.picInfo.datatype=RAW_MAG;
        fptr=rawmag;
        for (islab=0; islab<slabs; islab++) {
            rInfo.picInfo.slice=islab+1;
            for (rep=0; rep<(slice_reps/nchannels); rep++) {
                rInfo.picInfo.image=(float)(rep+1.0);
                for (echo=0; echo<echoes; echo++) {
                    rInfo.picInfo.echo=echo+1;
                    for (ichan=0; ichan<fnchan; ichan++) {
                        status=write_3Dfdf(fptr, &(rInfo.picInfo), arstr, ichan);
                        if (status) {
                            (void)recon3D_abort();
                            ABORT;
                        }
                        fptr+=2*npts3d*choffset;
                    }
                }
            }
        }
    }
        
        
    /* do all that for raw phase if needed */
    if ((rawflag==RAW_PHS) || (rawflag==RAW_MP)) {
        rInfo.picInfo.datatype=RAW_PHS;
        fptr=rawphs;
        for (islab=0; islab<slabs; islab++) {
            rInfo.picInfo.slice=islab+1;
            for (rep=0; rep<(slice_reps/nchannels); rep++) {
                rInfo.picInfo.image=(float)(rep+1.0);
                for (echo=0; echo<echoes; echo++) {
                    rInfo.picInfo.echo=echo+1;
                    for (ichan=0; ichan<fnchan; ichan++) {
                        status=write_3Dfdf(fptr, &(rInfo.picInfo), arstr, ichan);
                        if (status) {
                            (void)recon3D_abort();
                            ABORT;
                        }
                        fptr+=2*npts3d*choffset;
                    }
                }
            }
        }
    }        
        
        
    /* do all that for phase if needed */
    if (phsflag) {
        rInfo.picInfo.datatype=PHS_IMG;
        fptr=slicedata;
        for (islab=0; islab<slabs; islab++) {
            rInfo.picInfo.slice=islab+1;
            for (rep=0; rep<(slice_reps/nchannels); rep++) {
                rInfo.picInfo.image=(float)(rep+1.0);
                for (echo=0; echo<echoes; echo++) {
                    rInfo.picInfo.echo=echo+1;
                    for (ichan=0; ichan<fnchan; ichan++) {
                        status=write_3Dfdf(fptr, &(rInfo.picInfo), arstr, ichan);
                        if (status) {
                            (void)recon3D_abort();
                            ABORT;
                        }
                        fptr+=2*npts3d*choffset;
                    }
                }
            }
        }
    }             
        
    releaseAllWithId("generate_imagesBig3D");
    
    RETURN;
}

#ifdef XXX
/********************************************************************************************************/
/*  fftshift routine                                                                                                                                                */
/********************************************************************************************************/
static void fftshift(float *data, int npts) {
    float *fptr;
    float t1, t2;
    int i;
    int n2;

    n2=npts/2;
    /*perform the shift */
    fptr=data;
    for (i=0; i<n2; i++) {
        t1=*(fptr+npts);
        t2=*(fptr+1+npts);
        *(fptr+npts)=*fptr;
        *(fptr+1+npts)=*(fptr+1);
        *fptr++=t1;
        *fptr++=t2;
    }
    return;
}
#endif

/********************************************************************************************************/
/* nav_correct performs phase correction for raw data based on phase of navigators                      */
/********************************************************************************************************/
static void nav_correct(int method, float *data, double *ref_phase, int nro,
        int npe, float *navdata, svInfo *svI) {
    int i;
    int tnav;
    int nsegs;
    int ipt;
    int iseg;
    int iv;
    int npts;
    int offset;
    int netl;
    int nmod;

    float *fptr;
    double *dptr;
    double *dptr2;
    double a, b, c;
    double d1, d2;
    double *uphs;
    double *nav_phase;
    double phi;

    netl=svI->nnav+svI->etl;
    npts=nro;
    nsegs=(svI->pe_size)/(svI->etl);
    tnav=(svI->nnav)*nsegs;
    nmod=svI->nnav;

    nav_phase=(double *)recon3Dallocate(tnav*npts*sizeof(double), "nav_correct");
    uphs = (double *)recon3Dallocate(npts*sizeof(double), "nav_correct");

    /* get phase of each navigator */
    offset=0;
    for (i=0; i<tnav; i++) {
        (void)getphase(i, npts, tnav, nav_phase+offset, navdata, FALSE);
        offset+=npts;
    }

    dptr=nav_phase;
    if (method==PAIRWISE) {
        nmod=2; /* for picking out correction echo */
        /* compute even/odd phase differences */
        for (i=0; i<nsegs; i++) {
            dptr=nav_phase+i*npts*(svI->nnav);
            dptr2=dptr+npts;
            for (ipt=0; ipt<npts; ipt++) {
                a=*dptr;
                b=*dptr2;

                /*
                 c=0.5*(a-b);
                 *dptr++ = c;
                 *dptr2++ = -c;
                 *dptr++ = a;
                 *dptr2++ = b;
                 */

                c=(b-a);
                *dptr++ = 0.;
                *dptr2++ = c;
            }
        }
    }

    /* correct with reference navigator */
    dptr=nav_phase;
    for (i=0; i<tnav; i++)
        for (ipt=0; ipt<npts; ipt++)
            *dptr++ -= ref_phase[ipt];

    /* perform least squares fit if needed */
    if (method==LINEAR) {
        dptr=nav_phase;
        fptr=navdata;
        for (i=0; i<tnav; i++) {
            (void)phaseunwrap(dptr, npts, uphs);
            /* fit to polynomial and return polynomial evaluation */
            (void)smartphsfit(fptr, uphs, npts, dptr);
            dptr+=npts;
            fptr+=2*npts;
        }
    }

    /* now correct imaging data */
    fptr=data;
    for (i=0; i<npe; i++) {
        if (svI->multi_shot) {
            /* find unsorted view */
            iv=0;
            while (view_order[iv] != i)
                iv++;
        } else
            iv=i;

        iseg=iv/(svI->etl);
        if (nav_list[0]==0)
            iv+=svI->nnav;
        offset=iseg*(svI->nnav)+ iv%nmod;
        offset=offset%tnav;
        offset*=npts;

        for (ipt=0; ipt<nro; ipt++) {
            phi=nav_phase[offset+ipt];
            a=cos(phi);
            b=-sin(phi);
            d1=*fptr;
            d2=*(fptr+1);
            *fptr++=(float)(d1*a-d2*b);
            *fptr++=(float)(a*d2+b*d1);
        }
    }

    releaseAllWithId("nav_correct");

    return;
}

/********************************************************************************************************/
/* temp_ft will do either phase or slice direction FT on a block of 3D data, then append to a temp file */
/* temp_ft is also responsible for reversing magnitude data in read & phase directions (new fft)  */
/* int output_type  MAGNITUDE or COMPLEX */
/********************************************************************************************************/
static void temp_ft(float *xkydata, int nx, int ny, float *win,
        float *absdata, int phase_rev, int zeropad_ro, int zeropad_ph,
        float ro_frq, float ph_frq, float *phsdata, int phase_comp_flag, int block) {
    float a, b;
    float *fptr, *pptr, *pt1, *pt2;
    int ix, iy;
    int np;
    int pwr, fnt;
    int offset;
    int toffset;
    int halfR=FALSE;
    int halfP=FALSE;
    float *nrptr;
    float templine[2*MAXPE];
    float *tempdat;
    float *rotplane;
    char dirname[200];
    char str[200];
    long long lptr;
    
	int views=rInfo.picInfo.npe;
	int nro=rInfo.picInfo.nro; 
      
    fptr=absdata;
    pptr=phsdata;

    np=nx*ny;

    if (zeropad_ph > HALFF*ny)
        halfP=TRUE;

    if (zeropad_ro > HALFF*nx) {
        halfR=TRUE;
        tempdat=(float *)recon3Dallocate(2*np*sizeof(float), "temp_ft");
    }

    if (rInfo.svinfo.phase_compressed) {
    		rotplane=(float *)allocateWithId(2*views*nro*sizeof(float),
    				"temp_ft");
    }
    		
    /* do 2nd direction ft */
    for (ix=nx-1; ix>-1; ix--) {
		/* get the ky line */
		nrptr=templine;

		if (phase_rev && (!halfP)) {
			pt1=xkydata+2*ix;
			pt1+=(ny-1)*2*nx;
			pt2=pt1+1;
			for (iy=0; iy<ny; iy++) {
				*nrptr++=*pt1;
				*nrptr++=*pt2;
				pt1-=2*nx;
				pt2=pt1+1;
			}
		} else {
			pt1=xkydata+2*ix;
			pt2=pt1+1;
			nrptr=templine;
			for (iy=0; iy<ny; iy++) {
				*nrptr++=*pt1;
				*nrptr++=*pt2;
				pt1+=2*nx;
				pt2=pt1+1;
			}
		}


		if (win && rInfo.svinfo.phase_compressed) {
			for (iy=0; iy<ny; iy++) {
				templine[2*iy]*= *(win+iy);
				templine[2*iy+1]*= *(win+iy);
			}
		}

		/* apply frequency shift */
		
		if (rInfo.svinfo.phase_compressed && (ph_frq!=0.0)) {
			if (phase_rev)
				(void)rotate_fid(templine, 0.0, -1*ph_frq, 2*ny, COMPLEX);
			else
				(void)rotate_fid(templine, 0.0, ph_frq, 2*ny, COMPLEX);
		}
		

		nrptr=templine;
		pwr = 4;
		fnt = 32;
		while (fnt < 2*ny) {
			fnt *= 2;
			pwr++;
		}
		(void)fft(nrptr, ny, pwr, 0, COMPLEX, COMPLEX, -1.0, 1.0, ny);

		/* write it back */
		if (halfR) {
			pt1=tempdat+2*ix;
			pt2=pt1+1;
			nrptr=templine;
			for (iy=0; iy<ny; iy++) {
				*pt1=*nrptr++;
				*pt2=*nrptr++;
				pt1+=2*nx;
				pt2=pt1+1;
			}
		} else if(rInfo.svinfo.phase_compressed) /*  write result */
		{
			nrptr=templine;
			toffset=(nx-1-ix)*views;
			for (iy=0; iy<ny; iy++) {
				*(rotplane+2*toffset) = templine[2*(views-iy-1)];
				*(rotplane+2*toffset+1) = templine[2*(views-iy-1) + 1];
				toffset++;				
			}
		}
		 else if(rInfo.svinfo.sliceenc_compressed) /*  write result */
				{
					nrptr=templine;
					for (iy=0; iy<ny; iy++) {
						a=*nrptr++;
						b=*nrptr++;
						*fptr++=(float)a;
						*fptr++=(float)b;
						if (phsdata)
							*pptr++=(float)atan2(b, a);
					}
				}
	}
    
    
    /* open the temp file if need be */
    if(temp3D_file==NULL)
    {
              (void)strcpy(dirname, curexpdir);
              (void)strcat(dirname, "/recon");
              if (access(dirname, F_OK)) {              
            	  (void)sprintf(str, "mkdir %s \n", dirname);
            	  (void)system(str);
              }
              (void)strcat(dirname, "/temp3D");
              temp3D_file=fopen(dirname,"w+");
              if(!temp3D_file)
              {
                  Werrprintf("recon3D: temp_ft: Error opening temporary file");
                    (void)recon3D_abort();
                    return;
              }
    }

   if(phase_comp_flag)  //this is same as rInfo.svinfo.phase_compressed
	   (void)fwrite(rotplane,sizeof(float), 2*nx*ny, temp3D_file);
   else 			/* write it back along z axis for slice compressed case */
   {
	    fptr=absdata;
		for (ix=0; ix<nx; ix++) {
			offset=block*nx*2;
			offset += 2*ix;
			for (iy=0;iy<ny;iy++){	
				lptr=offset+iy*2*nro*views;
				lptr *= sizeof(float);
				(void)fseek(temp3D_file,lptr, SEEK_SET);
				(void)fwrite(fptr,sizeof(float), 2, temp3D_file);
				fptr += 2;
			}	
		}
   }
	  

    (void)releaseAllWithId("temp_ft");

    return;
}



/******************************************************************************/
/******************************************************************************/
static void filter_window(int window_type, float *window, int npts) {
    double f, alpha;
    int i;
    switch (window_type) {
    case NOFILTER:
        return;
    case BLACKMANN: {
        for (i=0; i<npts; i++)
            window[i]=0.42-0.5*cos(2*M_PI*(i-1)/(npts-1))+0.08*cos(4*M_PI*(i-1)
                    /(npts-1));
        return;
    }
    case HANN: {
        for (i=0; i<npts; i++)
            window[i]=0.5*(1-cos(2*M_PI*(i-1)/(npts-1)));
        return;
    }
    case HAMMING: {
        for (i=0; i<npts; i++)
            window[i]=0.54-0.46*cos(2*M_PI*(i-1)/(npts-1));
        return;
    }
    case GAUSSIAN: {
        alpha=2.5;
        f=(-2*alpha*alpha)/(npts*npts);
        for (i=0; i<npts; i++)
            window[i]=exp(f*(i-1-npts/2)*(i-1-npts/2));
        return;
    }
    case GAUSS_NAV: {
        alpha=20;
        f=(-2*alpha*alpha)/(npts*npts);
        for (i=0; i<npts; i++)
            window[i]=exp(f*(i-1-npts/2)*(i-1-npts/2));
        return;
    }
    default:
        return;
    }
}


/******************************************************************************/
/******************************************************************************/
char *recon3Dallocate(nsize, caller_id)
    int nsize;char *caller_id; {
    char *ptr;
    char str[MAXSTR];

    ptr=NULL;
    ptr=allocateWithId(nsize, caller_id);
    if (!ptr) {
        (void)sprintf(str, "recon3D: Error mallocing size of %d \n", nsize);
        Werrprintf(str);
        (void)recon3D_abort();
        return (NULL);
    }

    return (ptr);
}

/******************************************************************************/
/******************************************************************************/
void recon3D_abort() {
    D_close(D_USERFILE);
    recon3D_aborted=TRUE;
    (void)releaseAllWithId("recon3D");
    return;
}




