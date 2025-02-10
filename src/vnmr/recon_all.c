/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*****************************************************************************
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
#include <sys/statvfs.h>
#include <fcntl.h>
#include <stdlib.h>

#include <arpa/inet.h>
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
#include "graphics.h"
#include "init2d.h"
#include "buttons.h"



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
static int *skview_order;
static int *sskview_order;
static int *sview_order;
static int *slice_order;
static int *blockreps;
static int *blockrepeat;
static int *repeat;
static int *nts;
static short *pc_done, *pcneg_done, *pcd;
static reconInfo rInfo;
static int reconInfoSet = 0;
static int realtime_block;
static int recon_aborted;

static double *ro_frqP, *pe_frqP;

int nmice;
float *pss;
float *upss;

/* prototypes */
void recon_abort();
char *recon_allocate(int, char *);
int write_fdf(int, float *, fdfInfo *, int *, int, char *, int, int);
int write_3Dfdf(float *, fdfInfo *, char *, int);
int svcalc(int, int, svInfo *, int *, int *, int *, int *, int *);
int psscompare(const void *, const void *);
int upsscompare(const void *, const void *);
int pcompare(const void *, const void *);
void threeD_ft(float *, int, int, int, float *, float *, int, int,
        int, int, float *, double *, double *);
static int recon_space_check();
static int generate_images(char *arstr);
static int generate_images3D(char *arstr);
static void phase_ft(float *xkydata, int nx, int ny, float *win,
        float *absdata, int phase_rev, int zeropad_ro, int zeropad_ph,
        float ro_frq, float ph_frq, float *phsdata, int output_type);
static void filter_window(int window_type, float *window, int npts);
static void fftshift(float *data, int npts);
static void nav_correct(int method, float *data, double *ref_phase, int nro,
        int npe, float *navdata, svInfo *svI);
int arraycheck(char *param, char *arraystr);
int arrayparse(char *arraystring, int *nparams, arrayElement **arrayelsPP, int phasesize, int sviewsize);
int smartphsfit(float *input, double *inphs, int npts, double *outphs);
int pc_pick(char *pc_str);
int arrayfdf(int block, int np, arrayElement *arrayels, char *fdfstring);
extern void rotate_fid(float *fptr,double p0, double p1,int np, int dt);
extern int pc_calc(float *d, float *r, int es, int etl, int m, int tr);
extern int getphase(int ie, int el, int nv, double *ph, float *data, int tr);
extern int phaseunwrap(double *i, int n, double *o);
extern int polyfit(double *i, int n, double *o, int or);
extern int svprocpar(int ff, char *fp);


/*******************************************************************************/
/*******************************************************************************/
/*            RECON_ALL                                                        */
/*******************************************************************************/
/*******************************************************************************/

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
int recon_all(int argc, char *argv[], int retc, char *retv[]) {
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
    char tnstring[MAXSTR];
    char dnstring[MAXSTR];
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
    char *serr; 
    vInfo info;

    double fpointmult;
    double rnseg, rfc, retl, rnv, rnv2;
    double recon_force;
    double rimflag;
    double a1, b1;
    double dt2;
    double rechoes, repi_rev;
    double rslices, rnf;
    double dtemp;
    double m1, m2;
    double acqtime;

    dpointers inblock;
    dblockhead *bhp;
    dfilehead *fid_file_head;
#ifdef LINUX
    dfilehead tmpFileHead;
    dblockhead tmpBhp;
#endif
    
    float pss0;
    float *pss2;
    float *fptr, *nrptr;
    float *datptr=NULL;
    float *wptr;
    float *read_window=NULL;
    float *phase_window=NULL;
    float *phase2_window=NULL;
    float *nav_window=NULL;
    float *fdataptr;
    float *pc_temp;
    float real_dc = 0.0, imag_dc = 0.0;
    double ro_frq, pe_frq;
    double ro_ph, pe_ph;
    float a, b, c, d;
    float t1, t2, *pt1, *pt2, *pt3, *pt4, *pt5, *pt6;
    float ftemp;
    float *fdptr;
    float nt_scale=1.0;
    
    int ierr;
    int floatstatus;
    int status=0;
    int echoes=1;
    int tstch;
    int slab;
    int *idataptr;
    int imglen=0;
    int dimfirst,  imfound;
    int dimafter =1;
    int imzero, imneg1, imneg2;
    int icnt;
    int nv=1;
    int uslice;
    int ispb, itrc, it;
    int ip;
    int iecho;
    int blockctr;
    int pc_offset;
    int soffset;
    int tablen, itablen;
    int skiplen = 0;
    int skipilen;
    int skipitlen;
    int magoffset;
    int roffset;
    int iro;
    int ipc;
    int ibr;
    int ntlen=0;
    int phase_correct;
    int nt;
    int nshifts;
    int within_nt=1;
    int dsize=1;
    int nsize=0;
    int views=1;
    int nro=1;
    int nro2=1;
    int ro_size=1;
    int pe_size=1;
    int pe2_size=1;
    int ro2;
    int slices=1;
    int etl=1;
    int nblocks=1;
    int ntraces=1;
    int slice_reps=1;
    int slab_reps=1;
    int slicesperblock=1;
    int viewsperblock=1;
    int sviewsperblock=1;
    int slabsperblock=1;
    int uslicesperblock=1;
    int error, error1;
    int ipt;
    int npts=1;
    int npts3d=1;
    int im_slice=0;
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
    int rep=0;
    int urep;
    int i, min_view, min_view2, iview;
    int pcslice;
    int min_sview=0;
    int j;
    int narg;
    int zeropad=0;
    int zeropad2=0;
    int n_ft;
    int irep;
    int ctcount;
    int pwr, level;
    int fnt;
    int nshots=1;
    int nsize_ref=0;
    int nnav=0;
    int nav_pts=0;
    int nav;
    int fn, fn1, fn2;
    int wtflag;
    int itemp;
    int ch0=0;
    int in1, in2;
    int temp1, temp2, temp3, temp4;
    int d2array, d3array;
    int *skiptab;
    int inum;
    unsigned *skipint;
    unsigned skipd, iskip, ione;

    short s1;
    int l1;

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
    int display=TRUE;
    int image_jnt=FALSE;
    int lastchan=FALSE;
    int reallybig=FALSE;
    int variableNT=FALSE;
    int look_locker=FALSE;
    
    symbol **root;

    /*********************/
    /* executable code */
    /*********************/

    /* default :  do complete setup and mallocs */
    rInfo.do_setup=TRUE;

    rInfo.narray=0; // init this

    /* look for command line arguments */
    narg=1;
    if (argc>narg) /* first argument is acq or a dummy string*/
    {
        if (strcmp(argv[narg], "acq")==0) /* not first time through!!! */
        {
            rInfo.do_setup=FALSE;
        }
        narg++;
    }
    if (rInfo.do_setup) {
        if (reconInfoSet)
        {
           (void)releaseAllWithId("recon_all");
        
           if(rInfo.fidMd)
           {
               mClose(rInfo.fidMd);
           }
        
        }
        reconInfoSet = 1;
        rInfo.fidMd=NULL;
        sview_order=NULL;
        view_order=NULL;
        skview_order=NULL;
        sskview_order=NULL;
        recon_aborted=FALSE;
    }

    if (interuption || recon_aborted) {
        error=P_setstring(CURRENT, "wnt", "", 0);
        error=P_setstring(PROCESSED, "wnt", "", 0);
        Werrprintf("recon_all: ABORTING ");
        (void)recon_abort();
        ABORT;
    }

    /* what sequence is this ? */
    error=P_getstring(PROCESSED, "seqfil", sequence, 1, MAXSTR);
    if (error) {
        Werrprintf("recon_all: Error getting seqfil");
        (void)recon_abort();
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

    /* get transmit and decouple nucleii */
    error=P_getstring(PROCESSED, "tn", tnstring, 1, MAXSTR);
    if (error)
        (void)strcpy(tnstring, "H1");
    error=P_getstring(PROCESSED, "dn", dnstring, 1, MAXSTR);
    if (error)
        (void)strcpy(dnstring, "H1");

    /* get apptype */
    error=P_getstring(PROCESSED, "apptype", apptype, 1, MAXSTR);
    /*
     if(error)
     {
     Werrprintf("recon_all: Error getting apptype");	
     (void)recon_abort();
     ABORT;
     }		
     if(!strlen(apptype))
     {
     Winfoprintf("recon_all: WARNING:apptype unknown!");	
     Winfoprintf("recon_all: Set apptype in processed tree");	
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
            Werrprintf("recon_all: Invalid phase correction option in epi_pc");
            (void)recon_abort();
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
                        Werrprintf("recon_all: Invalid phase correction option in command line");
                        (void)recon_abort();
                        ABORT;
                    }
                }
            }
        } else {
            if (epi_seq) {
                (void)strcpy(epi_pc, argv[narg]);
                pc_option=pc_pick(epi_pc);
                if ((pc_option>MAX_OPTION)||(pc_option<OFF)) {
                    Werrprintf("recon_all: Invalid phase correction option in command line");
                    (void)recon_abort();
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
                Werrprintf("recon_all: Invalid phase correction option in epi_pc");
                (void)recon_abort();
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
        (void)strcat(rscfilename, "/recon_all.rsc");
        f1=fopen(rscfilename, "r");
        if (f1) {
            options=TRUE;
            serr=fgets(str, MAXSTR, f1);
            if (strstr(str, "image directory")) {
                (void)sscanf(str, "image directory=%s", rInfo.picInfo.imdir);
                rInfo.picInfo.fullpath=FALSE;
                if (epi_seq) {
                    serr=fgets(epi_pc, MAXSTR, f1);
                    pc_option=pc_pick(epi_pc);
                    if ((pc_option>MAX_OPTION)||(pc_option<OFF)) {
                        Werrprintf("recon_all: Invalid phase correction option in recon_all.rsc file");
                        (void)recon_abort();
                        ABORT;
                    }

                    serr=fgets(str, MAXSTR, f1);
                    (void)sscanf(str, "reverse=%d", &epi_rev);
                }
            } else /* no image directory specifed */
            {
                if (epi_seq) {
                    (void)strcpy(epi_pc, str);
                    pc_option=pc_pick(epi_pc);
                    if ((pc_option>MAX_OPTION)||(pc_option<OFF)) {
                        Werrprintf("recon_all: Invalid phase correction option in recon_all.rsc file");
                        (void)recon_abort();
                        ABORT;
                    }

                    serr=fgets(str, MAXSTR, f1);
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

        /* open the fid file */
        (void)strcpy (filepath, curexpdir );
        (void)strcat (filepath, "/acqfil/fid");
        (void)strcpy(rInfo.picInfo.fidname, filepath);
        
        
        rInfo.fidMd=mOpen(filepath, 0, O_RDONLY);
        if ( ! rInfo.fidMd ) {
            Werrprintf("recon_all: Failed to access data at %s",filepath);
            (void)recon_abort();
            ABORT;
        }
        (void)mAdvise(rInfo.fidMd, MF_SEQUENTIAL);
     
   
        /* start out fresh */
        /*
        D_close(D_USERFILE);
        fid_file_head=&tmpFileHead;
        error = D_gethead(D_USERFILE, fid_file_head);
        if (error) {
            error = D_open(D_USERFILE, filepath, fid_file_head);
            if (error) {
                Werrprintf("recon_all: Error opening FID file");
                (void)recon_abort();
                ABORT;
            }
        }
        */
         
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
            Werrprintf("recon_all: Error getting seqcon");
            (void)recon_abort();
            ABORT;
        }
        if (str[3] != 'n')
             threeD=TRUE;
        if (str[1] != 'c')
        	slice_compressed=FALSE;       
        if (str[2] != 'c')
            phase_compressed=FALSE;      
        if (threeD)
            if (str[3] != 'c')
                sliceenc_compressed=FALSE;

        /* display images or not? */
        rInfo.dispint=1;
        if (threeD)
            rInfo.dispint=0;
        error=P_getreal(CURRENT,"recondisplay",&dtemp,1);
        if (!error)
            rInfo.dispint=(int)dtemp;

        error=P_getreal(PROCESSED,"ns",&rslices,1);
        if (error) {
            Werrprintf("recon_all: Error getting ns");
            (void)recon_abort();
            ABORT;
        }
        slices=(int)rslices;

        error=P_getreal(PROCESSED,"nf",&rnf,1);
        if (error) {
            Werrprintf("recon_all: Error getting nf");
            (void)recon_abort();
            ABORT;
        }

        error=P_getreal(PROCESSED,"nv",&rnv,1);
        if (error) {
            Werrprintf("recon_all: Error getting nv");
            (void)recon_abort();
            ABORT;
        }
        nv=(int)rnv;
        views=nv;

        if (nv<MINPE) {
            Werrprintf("recon_all:  nv too small");
            (void)recon_abort();
            ABORT;
        }
  
#ifdef LINUX
        memcpy( &tmpFileHead, (dfilehead *)(rInfo.fidMd->offsetAddr),
                sizeof(dfilehead));
        fid_file_head = &tmpFileHead;
#else
        fid_file_head = (dfilehead *)(rInfo.fidMd->offsetAddr);
#endif
        /* byte swap if necessary */
        DATAFILEHEADER_CONVERT_NTOH(fid_file_head);
        ntraces=fid_file_head->ntraces;
        nblocks=fid_file_head->nblocks;
        nro=fid_file_head->np/2;
        rInfo.fidMd->offsetAddr+= sizeof(dfilehead);
      
        rInfo.tbytes=fid_file_head->tbytes;
        rInfo.ebytes=fid_file_head->ebytes;
        rInfo.bbytes=fid_file_head->bbytes;        
 
        if (nro<MINRO) {
            Werrprintf("recon_all:  np too small");
            (void)recon_abort();
            ABORT;
        }

        /* check for power of 2 in readout */
        zeropad=0;
        fn=0;
        error=P_getVarInfo(CURRENT,"fn",&info);
        if (info.active) {
            error=P_getreal(CURRENT,"fn",&dtemp,1);
            if (error) {
                Werrprintf("recon_all: Error getting fn");
                (void)recon_abort();
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
                Werrprintf("recon_all: phase correction and fract_kx are incompatible");
                (void)recon_abort();
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
                Werrprintf("recon_all: Error getting lro");
                (void)recon_abort();
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
                Werrprintf("recon_all: Error getting lpe");
                (void)recon_abort();
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
                    Werrprintf("recon_all: length of lsfrq does not equal nmice ");
                    (void)recon_abort();
                    ABORT;
                }
                pe_frqP=(double *)recon_allocate(nmice*sizeof(double),
                        "recon_all");
                error=P_getreal(PROCESSED,"sw1",&dt2,1);
                if (error) {
                    Werrprintf("recon_all: Error getting sw1 ");
                    (void)recon_abort();
                    ABORT;
                }
                if (nshifts==1) {
                    error=P_getreal(CURRENT,"lsfrq1",&dtemp,1);
                    if (error) {
                        Werrprintf("recon_all: Error getting lsfrq1 ");
                        (void)recon_abort();
                        ABORT;
                    }
                    for (i=0; i<nmice; i++)
                        pe_frqP[i]=180.0*dtemp/dt2;
                } else {
                    for (i=0; i<nmice; i++) {
                        error=P_getreal(CURRENT,"lsfrq1",&dtemp,(i+1));
                        if (error) {
                            Werrprintf("recon_all: Error getting lsfrq1 element");
                            (void)recon_abort();
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
                    Werrprintf("recon_all: length of lsfrq does not equal nmice ");
                    (void)recon_abort();
                    ABORT;
                }
                ro_frqP=(double *)recon_allocate(nmice*sizeof(double),
                        "recon_all");
                error=P_getreal(PROCESSED,"sw",&dt2,1);
                if (error) {
                    Werrprintf("recon_all: Error getting sw ");
                    (void)recon_abort();
                    ABORT;
                }
                if (nshifts==1) {
                    error=P_getreal(CURRENT,"lsfrq",&dtemp,1);
                    if (error) {
                        Werrprintf("recon_all: Error getting lsfrq ");
                        (void)recon_abort();
                        ABORT;
                    }
                    for (i=0; i<nmice; i++)
                        ro_frqP[i]=180.0*dtemp/dt2;
                } else {
                    for (i=0; i<nmice; i++) {
                        error=P_getreal(CURRENT,"lsfrq",&dtemp,(i+1));
                        if (error) {
                            Werrprintf("recon_all: Error getting lsfrq element");
                            (void)recon_abort();
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

        look_locker=0;
        error=P_getstring(PROCESSED, "recontype", str, 1, MAXSTR);
	if(!error && (strstr(str,"ook")))
	  {
	    error=P_getreal(PROCESSED,"nti",&dtemp,1);
            if (error) {
	      Werrprintf("recon_all: Error getting nti for Look-Locker");
	      (void)recon_abort();
	      ABORT;
	    }
	    else{   
	    	look_locker=1;
	    	if(strstr(str,"2"))
	    		look_locker=2;
	      echoes = (int)dtemp;
	      }     
	  }

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
            Werrprintf("recon_all: Error getting array");
            (void)recon_abort();
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
                    Werrprintf("recon_all: Error getting image element");
                    (void)recon_abort();
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
            Werrprintf("recon_all: Error getting nt info");
            (void)recon_abort();
            ABORT;
        }
        ntlen=info.size; /* read nt values*/
        nts=(int *)recon_allocate(ntlen*sizeof(int), "recon_all");
        for (i=0; i<ntlen; i++) {
            error=P_getreal(PROCESSED,"nt",&dtemp,(i+1));
            if (error) {
                Werrprintf("recon_all: Error getting nt element");
                (void)recon_abort();
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
                      Werrprintf("recon_all: nseg equal to zero");
                      (void)recon_abort();
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


        if (slice_compressed) {
            slices=(int)rslices;
            slicesperblock=slices;
        } else {
            P_getVarInfo(PROCESSED, "pss", &info);
            slices = info.size;
            slicesperblock=1;

            if (slices>nblocks) {
                Werrprintf("recon_all: slices greater than fids for standard mode");
                (void)recon_abort();
                ABORT;
            }

            if (slices>1) {
                viewsperblock=1;
                if(etl < views)
                	viewsperblock=etl;
                if (phase_compressed)
                    viewsperblock=views;
                within_slices=(nblocks/nchannels);
                /* get array info */
                rInfo.narray=0;
                if (strlen(arraystr)) {

                    (void)arrayparse(arraystr, &(rInfo.narray),
                                      &(rInfo.arrayelsP), (views/viewsperblock),slices);


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
                Werrprintf("recon_all: slices equal to zero");
                (void)recon_abort();
                ABORT;
            }
        }

  

        if (phase_compressed) {
            if (slices)
                slice_reps=nblocks*slicesperblock/slices;
            else {
                Werrprintf("recon_all: slices equal to zero");
                (void)recon_abort();
                ABORT;
            }
            slice_reps -= (imzero+imneg1+imneg2);
            if (slicesperblock)
                views=ntraces/slicesperblock;
            else {
                Werrprintf("recon_all: slicesperblock equal to zero");
                (void)recon_abort();
                ABORT;
            }
            views=nv;
            viewsperblock=views;
        } else {  // standard in phase dimension
            views=nv;
            d2array=FALSE;
            if (rInfo.narray == 0) // evaluation array string if not done yet
			{
				if (strlen(arraystr)) {
					(void) arrayparse(arraystr, &(rInfo.narray),
								&(rInfo.arrayelsP), (views / viewsperblock), slices);
				}
			}
             /* look for d2 and determine number of blocks within each view */
             ip = 0;
             if (strlen(arraystr)) {
				while ((!strstr((char *) rInfo.arrayelsP[ip].names, "d2"))
						&& (ip < rInfo.narray))
					ip++;
				if (ip < rInfo.narray) {
					within_views = rInfo.arrayelsP[ip].denom;
					within_views *= nchannels;
					d2array = TRUE;
				}
			}

            if (slices)
                slice_reps=nblocks*slicesperblock/slices;
            else {
                Werrprintf("recon_all: slices equal to zero");
                (void)recon_abort();
                ABORT;
            }
            if (nshots)
                slice_reps/=nshots;
            else {
                Werrprintf("recon_all: nshots equal to zero");
                (void)recon_abort();
                ABORT;
            }
            slice_reps -= (imzero+imneg1+imneg2);
            viewsperblock=1;
            if (nshots) {
            	if(!d2array)
            		within_views=(nblocks)/nshots; /* views are outermost */
           //    if (!slice_compressed&&!flash_converted)
             //       within_slices/=nshots;
            } else {
                Werrprintf("recon_all: nshots equal to zero");
                (void)recon_abort();
                ABORT;
            }
        }

        if (threeD) {
        	d2array=d3array=FALSE;  // phase encoding explicitly arrayed
            error = P_getreal(PROCESSED, "nv2", &rnv2, 1);
			if (error) {
				Werrprintf("recon3D: Error getting nv2");
				(void) recon_abort();
				ABORT;
			}
			slices = (int) rnv2;

            if (phase_compressed) {
                slab_reps=nblocks*slabsperblock/slabs;
                slab_reps -= (imzero+imneg1+imneg2);
                viewsperblock=views;
            } else {
                slab_reps=nblocks*slabsperblock/slabs;
                slab_reps/=views;
                slab_reps -= (imzero+imneg1+imneg2);
                viewsperblock=1;

                /* look for d2 and determine number of blocks within each view */
                // evaluation array string with correct value of slices
                ip=0;
    			if (strlen(arraystr)) {
    				(void) arrayparse(arraystr, &(rInfo.narray),
    				    							&(rInfo.arrayelsP), (views / viewsperblock), slices);


                  while ((!strstr((char *) rInfo.arrayelsP[ip].names, "d2"))
                 		 && (ip < rInfo.narray))
     						ip++;
    			}

                  if((strlen(arraystr)) && (ip < rInfo.narray)){
     					 within_views=rInfo.arrayelsP[ip].denom;
     					 within_views *= nchannels;
     					 d2array=TRUE;
                  }

                  else{
						  within_views=nblocks/views; /* views are outermost */
						 if(!sliceenc_compressed)
						   within_views /= slices;
                  }

                if (!slab_compressed&&!flash_converted)
                    within_slabs/=views;
            }

            if (sliceenc_compressed) {
                sviewsperblock=slices;
            } else {
                slab_reps/=slices;
				sviewsperblock = 1;

				within_sviews = nblocks / slices;  // implicit arrays


				if(strlen(arraystr))
				{
				/* look for d3 and determine number of blocks within each sview */
				if (rInfo.narray == 0) // evaluation array string if not done yet
					{
							(void) arrayparse(arraystr, &(rInfo.narray),
									&(rInfo.arrayelsP),
									(views / viewsperblock), slices);
					}

					ip = 0;
					while ((!strstr((char *) rInfo.arrayelsP[ip].names, "d3"))
							&& (ip < rInfo.narray))
						ip++;
					if (ip < rInfo.narray) {
						within_sviews = rInfo.arrayelsP[ip].denom;
						within_sviews *= nchannels;
						d3array = TRUE;
						if (!phase_compressed && !d2array)
							within_views *= slices;

					}
				}

            }

            /* check profile flag and fake a 2D if need be */
            error=P_getstring(PROCESSED, "profile", profstr, 1, SHORTSTR);
            if (!error) {
                if ((profstr[0] == 'y') && (profstr[1] == 'n')) {
                    threeD=FALSE;
                    slices=1;
                    slice_reps=nchannels;
                    slicesperblock=1;
                    within_slices=1;
                    views=(int)rnv2;
                    rInfo.dispint=1;
                    /* phase encode inherits properties of slice encode */
                    phase_compressed=sliceenc_compressed;
                    viewsperblock=sviewsperblock;
                    within_views=within_sviews;
                } else if ((profstr[0] == 'n') && (profstr[1] == 'y')) {
                    threeD=FALSE;
                    rInfo.dispint=1;
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
                    Werrprintf("recon_all: Error getting fn2");
                    (void)recon_abort();
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
		if (strlen(arraystr)) {
			(void) arrayparse(arraystr, &(rInfo.narray), &(rInfo.arrayelsP),
					(views / viewsperblock), slices);
		}

        
        /* figure out if nt and d2 are jointly arrayed */
        variableNT=FALSE;
        i=0;
        while(i<rInfo.narray){
        	if(rInfo.arrayelsP[i].nparams > 1)
        	{
        		for(in1=0;in1<rInfo.arrayelsP[i].nparams;in1++)
        			if(!strcmp(rInfo.arrayelsP[i].names[in1],"d2"))
        			{
        	     		for(in2=++in1;in2<rInfo.arrayelsP[i].nparams;in2++)
        	        			if(!strcmp(rInfo.arrayelsP[i].names[in2],"nt"))
        	        				variableNT=TRUE;
        			}
        			else if(!strcmp(rInfo.arrayelsP[i].names[in1],"nt"))
        			{
        	     		for(in2=++in1;in2<rInfo.arrayelsP[i].nparams;in2++)
        	        			if(!strcmp(rInfo.arrayelsP[i].names[in2],"d2"))
        	        				variableNT=TRUE;
        			}
        	}
        	i++;
        }

        	
        zeropad2=0;
        fn1=0;
        error=P_getVarInfo(CURRENT,"fn1",&info);
        if (info.active) {
            error=P_getreal(CURRENT,"fn1",&dtemp,1);
            if (error) {
                Werrprintf("recon_all: Error getting fn1");
                (void)recon_abort();
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
                Werrprintf("recon_all: fract_kx and fract_ky are incompatible");
                (void)recon_abort();
                ABORT;
            }
        }

        /* initialize phase & read reversal flags */
        rInfo.phsrev_flag=FALSE;
        rInfo.alt_phaserev_flag=FALSE;
        rInfo.alt_readrev_flag=FALSE;
        if (echoes>1) {
            error=P_getstring(CURRENT,"altecho_reverse", tmp_str, 1, SHORTSTR);
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
                    nav_list = (int *)recon_allocate(nnav*sizeof(int),
                            "recon_all");
                    for (i=0; i<nnav; i++) {
                        error=P_getreal(PROCESSED,"nav_echo",&dtemp,i+1);
                        nav_list[i]=(int)dtemp - 1;
                        if ((nav_list[i]<0)||(nav_list[i]>=(etl+nnav))) {
                            Werrprintf("recon_all: navigator value out of range");
                            (void)recon_abort();
                            ABORT;
                        }
                    }
                }

                if (!nnav) {
                    /* figure out navigators per echo train */
                    nnav=((ntraces/slicesperblock)-views)/nshots;
                    if (nnav>0) {
                        nav_list = (int *)recon_allocate(nnav*sizeof(int),
                                "recon_all");
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
                    Werrprintf("recon_all: Invalid navigator option in nav_type parameter!");
                    (void)recon_abort();
                    ABORT;
                }
            }
            if ((nav_option==PAIRWISE)&&(nnav<2)) {
                Werrprintf("recon_all: 2 navigators required for PAIRWISE nav_type");
                (void)recon_abort();
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
            Werrprintf("recon_all: slice reps less than 1");
            (void)recon_abort();
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
        


        skipint = NULL;
		skiptab = NULL;
		error =P_getstring(PROCESSED, "skiptab", str, 1, MAXSTR);
        if (strstr(str, "y")) {
			// read skipint
			error = P_getVarInfo(CURRENT, "skipint", &info);
			if (!error && (info.size > 1)) {
				skipilen = info.size;
				skipint = (unsigned *) recon_allocate(skipilen * sizeof(unsigned),
						"recon_all");
				for (itab = 0; itab < skipilen; itab++) {
					error = P_getreal(CURRENT, "skipint", &dtemp, itab + 1);
					if (error) {
						Werrprintf("recon_all: Error getting skipint element");
						(void) recon_abort();
						ABORT;
					}
					skipint[itab] = (unsigned)dtemp;
				}
			}

			// read skiptabvals
			skiplen = 0;
			error = P_getVarInfo(CURRENT, "skiptabvals", &info);
			if (!error && (info.size > 1)) {
				itablen = info.size;

				if ((itablen = tablen) && (itablen != views) && (itablen
						!= slices * views)) {
					Werrprintf("recon_all: skiptabvals is wrong size");
					(void) recon_abort();
					ABORT;
				}
				tablen = itablen;

				skiptab = (int *) recon_allocate(itablen * sizeof(int),
						"recon_all");
				for (itab = 0; itab < itablen; itab++) {
					error = P_getreal(CURRENT, "skiptabvals", &dtemp, itab + 1);
					if (error) {
						Werrprintf(
								"recon_all: Error getting skiptabvals element");
						(void) recon_abort();
						ABORT;
					}
					if (dtemp > 0.0) {
						skiplen++;
					}
					skiptab[itab] = dtemp;
				}
			}
		}


        error=P_getVarInfo(CURRENT,"pelist",&info);
        if (!error && (info.size>1)) {
            itablen=info.size;
            if ((itablen != tablen)&&(itablen != views)) {
                Werrprintf("recon_all: pelist is wrong size");
                (void)recon_abort();
                ABORT;
            }
            tablen=itablen;
            view_order = (int *)recon_allocate(itablen*sizeof(int), "recon_all");
            for (itab=0; itab<itablen; itab++) {
                error=P_getreal(CURRENT,"pelist",&dtemp,itab+1);
                if (error) {
                    Werrprintf("recon_all: Error getting pelist element");
                    (void)recon_abort();
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
                    Werrprintf("recon_all: Error getting petable");
                    (void)recon_abort();
                    ABORT;
                }
                table_ptr = NULL;
                if (appdirFind(petable, "tablib", tablefile, NULL, R_OK))
                    table_ptr = fopen(tablefile, "r");
                if (!table_ptr) {
                    Werrprintf("recon_all: Error opening petable file %s",
                            petable);
                    (void)recon_abort();
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
                    Werrprintf("recon_all: Error with fseek, petable");
                    (void)recon_abort();
                    ABORT;
                }
                if ((itablen!=views)&&(itablen!=tablen)) {
                    Werrprintf("recon_all: Error wrong phase sorting table size");
                    (void)recon_abort();
                    ABORT;
                }

                /* read in table for view sorting */
                view_order = (int *)recon_allocate(tablen*sizeof(int),
                        "recon_all");
                while ((tstch=fgetc(table_ptr) != '=') && (tstch != EOF))
                    ;
                if (tstch == EOF) {
                    Werrprintf("recon_all: EOF while reading petable file");
                    (void)recon_abort();
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
                    sview_order = (int *)recon_allocate(tablen*sizeof(int),
                            "recon_all");
                    while ((tstch=fgetc(table_ptr) != '=') && (tstch != EOF))
                        ;
                    if (tstch == EOF) {
                        Werrprintf("recon_all: EOF while reading petable file for t2");
                        (void)recon_abort();
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
            if (FALSE && (min_view<min_view2)) {
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
        
        if (skipint) {  // this is the compressed version of skiptab
    			multi_shot = TRUE;
    			ione=1;
    			if (threeD) {  // these may be oversized but that's ok
    				skipitlen=slices*views;
    				sskview_order = (int *) recon_allocate(skipitlen
    						* sizeof(int), "recon_all");
    				skview_order = (int *) recon_allocate(skipitlen
    						* sizeof(int), "recon_all");
    			}
    			else {
    				skipitlen=views;
    				skview_order = (int *) recon_allocate(skipitlen * sizeof(int),
    						"recon_all");
    			}

    			if (view_order) {
    				j = 0;
    				for (i = 0; i < skipitlen; i++) {
    					inum=i/32;  // which skipint to read
    					skipd = (int)skipint[inum]; // get the double
    					iskip = ione & (skipd >> (i%32));
    					if (iskip > 0)
    						skview_order[j++] = view_order[i];
    				}
    			}
    			if (sview_order) {
    				j = 0;
    				for (i = 0; i < skipitlen; i++) {
    					inum=i/32;  // which skipint to read
    					skipd = (int)skipint[inum]; // get the double
    					iskip = ione & (skipd >> (i%32));
    					if (iskip > 0)
    						sskview_order[j++] = sview_order[i];

    				}
    			}
    			else // no sort table so assume linear
    			{
    				j = 0;
    				for (i = 0; i < skipitlen; i++) {
    					inum=i/32;  // which skipint to read
    					skipd = (int)skipint[inum]; // get the double
    					iskip = ione & (skipd >> (i%32));
    					if (iskip > 0)
    					{
    						skview_order[j] = i % views;
    						if(threeD)
    							sskview_order[j] = (i / views) % slices;
    						j++;
    					}
    				}
    			}

    			if (j < ntraces) {
    				Werrprintf("recon_all: Error - mismatch between traces and elliptical table values");
    				(void) recon_abort();
    				ABORT;
    			}
            }
        else if (skiptab) {
			multi_shot = TRUE;
			skview_order = (int *) recon_allocate(skiplen * sizeof(int),
					"recon_all");
			if (threeD)
				sskview_order = (int *) recon_allocate(skiplen * sizeof(int),
						"recon_all");
			if (view_order) {
				j = 0;
				for (i = 0; i < tablen; i++) {
					if (skiptab[i] > 0)
						skview_order[j++] = view_order[i];
				}
			}
			if (sview_order) {
				j = 0;
				for (i = 0; i < tablen; i++) {
					if (skiptab[i] > 0)
						sskview_order[j++] = sview_order[i];
				}
			} else // no sort table so assume linear
			{
				j = 0;
				for (i = 0; i < tablen; i++) {
					if (skiptab[i] > 0) {
						skview_order[j] = i % views;
						j++;
					}
				}

				if (threeD) {
					j = 0;
					for (i = 0; i < tablen; i++) {
						if (skiptab[i] > 0) {
							sskview_order[j] = (i / views) % slices;
							j++;
						}
					}
				}
			}
        }




        if (flash_converted)
            multi_shot=FALSE;



        blockreps = NULL;
        blockrepeat = NULL;

        if (!threeD) {
            /* get pss array and make slice order array */
            pss=(float*)recon_allocate(slices*sizeof(float), "recon_all");
            slice_order=(int*)recon_allocate(slices*sizeof(int), "recon_all");
            for (i=0; i<slices; i++) {
                error=P_getreal(PROCESSED,"pss",&dtemp,i+1);
                if (error) {
                    Werrprintf("recon_all: Error getting slice offset");
                    (void)recon_abort();
                    ABORT;
                }
                pss[i]=(float)dtemp;
                slice_order[i]=i+1;
            }

            uslicesperblock=slicesperblock;
            if (slice_compressed) {
                /* count repetitions of slice positions */
                blockreps=(int *)recon_allocate(slicesperblock*sizeof(int),
                        "recon_all");
                blockrepeat=(int *)recon_allocate(slicesperblock*sizeof(int),
                        "recon_all");
                pss2=(float *)recon_allocate(slicesperblock*sizeof(float),
                        "recon_all");
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
                upss=(float *)recon_allocate(uslicesperblock*sizeof(float),
                        "recon_all");
                j=0;
                for (i=0; i<uslicesperblock; i++) {
                    upss[i]=pss[j];
                    j+=blockreps[i];
                }
                (void)qsort(slice_order, uslicesperblock, sizeof(int),
                        upsscompare);
		
		/* undo all this and report repeat slices as unique */
		uslicesperblock=slicesperblock;

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
                    Werrprintf("recon_all: Error getting something");
                    (void)recon_abort();
                    ABORT;
                }

                if (imfound) {
                    if (ptr != imptr)
                        dimafter *= info.size; /* get dimension of this variable */
                } else
                    dimfirst *= info.size;

                ptr = strtok(NULL, ",");
			}
			dimafter *= nchannels;
		} else {
			if (nchannels > 0)
				dimfirst = nblocks / nchannels;
			else
				dimfirst = nblocks;
		}

        if (flash_converted)
            dimafter*=slices;

     //   dimafter *= nchannels;

        /* set up fdf header structure */

        error=P_getreal(PROCESSED,"lro",&dtemp,1);
        if (error) {
            Werrprintf("recon_all: Error getting lro");
            (void)recon_abort();
            ABORT;
        }
        rInfo.picInfo.fovro=dtemp;
        error=P_getreal(PROCESSED,"lpe",&dtemp,1);
        if (error) {
            Werrprintf("recon_all: Error getting lpe");
            (void)recon_abort();
            ABORT;
        }
        rInfo.picInfo.fovpe=dtemp;
        error=P_getreal(PROCESSED,"thk",&dtemp,1);
        if (error) {
            Werrprintf("recon_all: Error getting thk");
            (void)recon_abort();
            ABORT;
        }
        rInfo.picInfo.thickness=MM_TO_CM*dtemp;

        error=P_getreal(PROCESSED,"gap",&dtemp,1);
        if (error && !threeD) {
            Werrprintf("recon_all: Error getting gap");
            (void)recon_abort();
            ABORT;
        }
        rInfo.picInfo.gap=dtemp;

        if (threeD) {
            error=P_getreal(PROCESSED,"lpe2",&dtemp,1);
            if (error) {
                Werrprintf("recon_all: Error getting lpe2");
                (void)recon_abort();
                ABORT;
            }
            rInfo.picInfo.thickness=dtemp;
        }

        rInfo.picInfo.slices=slices;
        rInfo.picInfo.echo=1;
        rInfo.picInfo.echoes=echoes;

        error=P_getVarInfo(PROCESSED,"psi",&info);
        if (error) {
            Werrprintf("recon_all: Error getting psi info");
            (void)recon_abort();
            ABORT;
        }
        rInfo.picInfo.npsi=info.size;
        error=P_getVarInfo(PROCESSED,"phi",&info);
        if (error) {
            Werrprintf("recon_all: Error getting phi info");
            (void)recon_abort();
            ABORT;
        }
        rInfo.picInfo.nphi=info.size;
        error=P_getVarInfo(PROCESSED,"theta",&info);
        if (error) {
            Werrprintf("recon_all: Error getting theta info");
            (void)recon_abort();
            ABORT;
        }
        rInfo.picInfo.ntheta=info.size;

        error=P_getreal(PROCESSED,"te",&dtemp,1);
        if (error) {
            Werrprintf("recon_all: recon_all: Error getting te");
            (void)recon_abort();
            ABORT;
        }
        rInfo.picInfo.te=(float)SEC_TO_MSEC*dtemp;
        error=P_getreal(PROCESSED,"tr",&dtemp,1);
        if (error) {
            Werrprintf("recon_all: recon_all: Error getting tr");
            (void)recon_abort();
            ABORT;
        }
        rInfo.picInfo.tr=(float)SEC_TO_MSEC*dtemp;
        (void)strcpy(rInfo.picInfo.seqname, sequence);
        (void)strcpy(rInfo.picInfo.studyid, studyid);
        (void)strcpy(rInfo.picInfo.position1, pos1);
        (void)strcpy(rInfo.picInfo.position2, pos2);
        (void)strcpy(rInfo.picInfo.tn, tnstring);
        (void)strcpy(rInfo.picInfo.dn, dnstring);

        error=P_getreal(PROCESSED,"ti",&dtemp,1);
        if (error) {
            Werrprintf("recon_all: recon_all: Error getting ti");
            (void)recon_abort();
            ABORT;
        }
        rInfo.picInfo.ti=(float)SEC_TO_MSEC*dtemp;
        rInfo.picInfo.image=dimafter*dimfirst;
        if (image_jnt)
            rInfo.picInfo.image/=imglen;
        rInfo.picInfo.image*=(imglen-imzero-imneg1-imneg2);

        if (!phase_compressed)
        {
				if (multi_shot)
				rInfo.picInfo.image /= nshots;
			else
				rInfo.picInfo.image /= views;
		}

        if (!slice_compressed)
            rInfo.picInfo.image/=slices;
        rInfo.picInfo.array_index=1;

        error=P_getreal(PROCESSED,"sfrq",&dtemp,1);
        if (error) {
            Werrprintf("recon_all: recon_all: Error getting sfrq");
            (void)recon_abort();
            ABORT;
        }
        rInfo.picInfo.sfrq=dtemp;

        error=P_getreal(PROCESSED,"dfrq",&dtemp,1);
        if (error) {
            Werrprintf("recon_all: recon_all: Error getting dfrq");
            (void)recon_abort();
            ABORT;
        }
        rInfo.picInfo.dfrq=dtemp;

        /* estimate time for block acquisition */
        error = P_getreal(PROCESSED,"at",&acqtime,1);
        if (error) {
            Werrprintf("recon_all: Error getting at");
            (void)recon_abort();
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
        phase2_window=NULL;
        nav_window=NULL;
        recon_window=NOFILTER; /* turn mine off */

        if (nnav) /* make a window for navigator echoes */
        {
            nav_window=(float*)recon_allocate(ro_size*sizeof(float), "recon_all");
            (void)filter_window(GAUSS_NAV,nav_window, ro_size);
        }

        error=P_getreal(CURRENT,"ftproc",&dtemp,1);
        if (error) {
            Werrprintf("recon_all: Error getting ftproc");
            (void)recon_abort();
            ABORT;
        }
        wtflag=(int)dtemp;

        if (wtflag) {
            recon_window=MAX_FILTER;

            if (init_wt1(&wtp, S_NP)) {
                Werrprintf("recon_all: Error from init_wt1");
                (void)recon_abort();
                ABORT;
            }
            wtp.wtflag=TRUE;
	    //            fpointmult = getfpmult(S_NP, fid_file_head->status & S_DDR);
            fpointmult = getfpmult(S_NP, 0);
            read_window=(float*)recon_allocate(ro_size*sizeof(float),
                    "recon_all");
            if (init_wt2(&wtp, read_window, ro_size, FALSE, S_NP, fpointmult,
                    FALSE)) {
                Werrprintf("recon_all: Error from init_wt2");
                (void)recon_abort();
                ABORT;
            }

            phase_window=(float*)recon_allocate(views*sizeof(float),
                    "recon_all");
            if (phase_compressed) {
                if (init_wt1(&wtp, S_NF)) {
                    Werrprintf("recon_all: Error from init_wt1");
                    (void)recon_abort();
                    ABORT;
                }
                wtp.wtflag=TRUE;
                fpointmult = getfpmult(S_NF,0);
                if (init_wt2(&wtp, phase_window, views, FALSE, S_NF,
                        fpointmult, FALSE)) {
                    Werrprintf("recon_all: Error from init_wt2");
                    (void)recon_abort();
                    ABORT;
                }
            } else {
                if (init_wt1(&wtp, S_NI)) {
                    Werrprintf("recon_all: Error from init_wt1");
                    (void)recon_abort();
                    ABORT;
                }
                wtp.wtflag=TRUE;
                fpointmult = getfpmult(S_NI,0);
                if (init_wt2(&wtp, phase_window, views, FALSE, S_NI,
                        fpointmult, FALSE)) {
                    Werrprintf("recon_all: Error from init_wt2");
                    (void)recon_abort();
                    ABORT;
                }
            }

	    if(threeD)
	      {
		phase2_window=(float*)recon_allocate(slices*sizeof(float),
						     "recon_all");

		  if (init_wt1(&wtp, S_NI2)) {
                    Werrprintf("recon_all: Error from init_wt1");
                    (void)recon_abort();
                    ABORT;
		  }
		  wtp.wtflag=TRUE;
		  fpointmult = getfpmult(S_NI2,0);
		  if (init_wt2(&wtp, phase2_window, slices, FALSE, S_NI2,
			       fpointmult, FALSE)) {
                    Werrprintf("recon_all: Error from init_wt2");
                    (void)recon_abort();
                    ABORT;
		  }
	      }

        }

        error=P_getstring(CURRENT, "rcvrout",tmp_str, 1, SHORTSTR);
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
	
	/* get scaling for images if available */
        error=P_getreal(CURRENT,"aipScale",&dtemp,1);
        if (!error)
	  rInfo.image_scale=dtemp;
	else
	  rInfo.image_scale=IMAGE_SCALE;
  
	if(rInfo.image_scale == 0.0)
		rInfo.image_scale=1.0;


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
            if (chkSize > 65536.0*32768.0)
            {
               Werrprintf("recon_all: Data too large (> 2 Gbytes)");
               (void)recon_abort();
               ABORT;
            }
            dsize=slabsperblock*slab_reps*echoes*2*npts3d;
            dsize*=coilfactor;
            
 
         //   magnitude=(float *)recon_allocate(dsize/2*sizeof(float),
           //           "recon_all");
           // mag2=(float *)recon_allocate(npts3d*sizeof(float), "recon_all");
            if (nnav) {
                nsize=nshots*nnav*echoes*nro*slices*slice_reps;
                nsize_ref=echoes*nro*slices;
                navdata=(float *)recon_allocate(nsize*sizeof(float),
                        "recon_all");
                nav_refphase=(double *)recon_allocate(nsize_ref*sizeof(double),
                        "recon_all");
            }
        } /* 2D cases below */
        else if (phase_compressed) {
            dsize=slicesperblock*echoes*2*npts;
            if (threeD)
                dsize=slabsperblock*echoes*2*npts3d;
            magnitude=(float *)recon_allocate((dsize/2)*sizeof(float),
                    "recon_all");
            mag2=(float *)recon_allocate((dsize/2)*sizeof(float), "recon_all");
            if (nnav) {
                nsize=nshots*nnav*echoes*nro*2*slicesperblock;
                nsize_ref=echoes*nro*slicesperblock;
                navdata=(float *)recon_allocate(nsize*sizeof(float),
                        "recon_all");
                nav_refphase=(double *)recon_allocate(nsize_ref*sizeof(double),
                        "recon_all");
            }
        } else {
            dsize=slices*slice_reps*echoes*2*npts;
            dsize*=coilfactor;
            magnitude=(float *)recon_allocate(npts*sizeof(float), "recon_all");
            mag2=(float *)recon_allocate(npts*sizeof(float), "recon_all");
            if (nnav) {
                nsize=nshots*nnav*echoes*2*nro*slices*slice_reps;
                nsize_ref=echoes*2*nro*slices;
                navdata=(float *)recon_allocate(nsize*sizeof(float),
                        "recon_all");
                nav_refphase=(double *)recon_allocate(nsize_ref*sizeof(double),
                        "recon_all");
            }
        }

   
        
        slicedata=(float *)recon_allocate(dsize*sizeof(float), "recon_all");
        if (slicedata == NULL) {
            (void)recon_abort();
            ABORT;
        }
        if ((rawflag==RAW_MAG)||(rawflag==RAW_MP))
            rawmag
                    =(float *)recon_allocate((dsize/2)*sizeof(float),
                            "recon_all");
        if ((rawflag==RAW_PHS)||(rawflag==RAW_MP))
            rawphs
                    =(float *)recon_allocate((dsize/2)*sizeof(float),
                            "recon_all");
        if (sense||phsflag)
            imgphs
                    =(float *)recon_allocate((dsize/2)*sizeof(float),
                            "recon_all");
        if (phsflag)
            imgphs2=(float *)recon_allocate((dsize/2)*sizeof(float),
                    "recon_all");

        if (pc_option!=OFF) {
            pc=(float *)recon_allocate(slices*echoes*nchannels*2*npts*sizeof(float),
                    "recon_all");
            pc_done=(short *)recon_allocate(slices*nchannels*echoes*sizeof(int),
                    "recon_all");
            for (ipc=0; ipc<slices*nchannels*echoes; ipc++)
                *(pc_done+ipc)=FALSE;

            if (epi_dualref && (pc_option==TRIPLE_REF)) {
                pc_neg=(float *)recon_allocate(slices*nchannels*echoes*2*npts
                        *sizeof(float), "recon_all");
                slicedata_neg=(float *)recon_allocate(dsize*sizeof(float),
                        "recon_all");
                imdata
                        =(float *)recon_allocate(dsize*sizeof(float),
                                "recon_all");
                imdata_neg=(float *)recon_allocate(dsize*sizeof(float),
                        "recon_all");
                if (nchannels) {
                    imdata2=(float *)recon_allocate(dsize*sizeof(float),
                            "recon_all");
                    imdata2_neg=(float *)recon_allocate(dsize*sizeof(float),
                            "recon_all");
                }
            }
            if (epi_dualref) {
                pcneg_done=(short *)recon_allocate(slices*nchannels*echoes*sizeof(int),
                        "recon_all");
                for (ipc=0; ipc<slices*nchannels*echoes; ipc++)
                    *(pcneg_done+ipc)=FALSE;
            }
        }


	if (ntlen)
	  {
	    if (strstr(arraystr,"nt")) 
	      {
		ip = 0;
		while ((ip < rInfo.narray) && (!strstr((char *) rInfo.arrayelsP[ip].names, "nt")))
		  ip++;
		within_nt = rInfo.arrayelsP[ip].denom;
		within_nt *= nchannels;
	      }
	    else 
	      {
		if (phase_compressed)
		  within_nt = nblocks / ntlen;
		else if (variableNT)
		  within_nt = etl * nblocks / (nv);
		else
		  within_nt = etl * nblocks / (nv * ntlen);
	      }
	  }
	else
	  {
	    Werrprintf("recon_all: ntlen equal to zero");
	    (void)recon_abort();
	    ABORT;
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
        rInfo.svinfo.within_slices = within_slices;
		rInfo.svinfo.within_views = within_views;
		rInfo.svinfo.within_sviews = within_sviews;
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
        rInfo.imglen=imglen;
        rInfo.ntlen=ntlen;
        rInfo.within_nt=within_nt;
        rInfo.epi_rev=epi_rev;
        rInfo.rwindow=read_window;
        rInfo.pwindow=phase_window;
        rInfo.swindow=phase2_window;
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
        rInfo.variableNT = variableNT;
        rInfo.svinfo.look_locker=look_locker;

        
        if (recon_space_check()) {
			Werrprintf("recon_all: space check failed ");
			(void)recon_abort();
			ABORT;
		}
        

        /* zero everything important */

        (void)memset(slicedata, 0, dsize * sizeof(*slicedata));

        if(pc_option!=OFF)
        {
            (void)memset(pc, 0, slices * nchannels * echoes * 2 * npts * sizeof(*pc));
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
            (void)memset(pc_neg, 0, slices * nchannels * echoes * 2 * npts * sizeof(*pc_neg));
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
            repeat=(int *)recon_allocate(slabsperblock*slices*views*echoes*sizeof(int),"recon_all");
            for(i=0;i<slabsperblock*slices*echoes*views;i++)
            *(repeat+i)=-1;
        }
        else
        {
            repeat=(int *)recon_allocate(slices*views*echoes*sizeof(int),"recon_all");
            for(i=0;i<slices*echoes*views;i++)
            *(repeat+i)=-1;
        }

        /* remove extra directories */
        (void)strcpy(tempdir,curexpdir);
        (void)strcat(tempdir,"/reconphs");
        (void)sprintf(str,"rm -rf  %s \n",tempdir);
        ierr=system(str);

        (void)strcpy(tempdir,curexpdir);
        (void)strcat(tempdir,"/rawphs");
        (void)sprintf(str,"rm -rf  %s \n",tempdir);
        ierr=system(str);

        (void)strcpy(tempdir,curexpdir);
        (void)strcat(tempdir,"/rawmag");
        (void)sprintf(str,"rm -rf  %s \n",tempdir);
        ierr=system(str);

        (void)strcpy(tempdir,curexpdir);
        (void)strcat(tempdir,"/recon");
        (void)sprintf(str,"rm -rf  %s \n",tempdir);
        ierr=system(str);

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
        within_nt=rInfo.within_nt;
        epi_rev=rInfo.epi_rev;
        read_window=rInfo.rwindow;
        phase_window=rInfo.pwindow;
        phase2_window=rInfo.swindow;
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
        variableNT=rInfo.variableNT;
        look_locker=rInfo.svinfo.look_locker;
    }
    
    reallybig=FALSE;
    if(threeD)
        if(slices*ro_size*pe_size >= BIG3D)
            reallybig=TRUE;
    
    if(realtime_block>=nblocks)
    {
        (void)recon_abort();
        ABORT;
    }

    slice=0;
    view=0;
    echo=0;
    nshots=views/etl;
    nshots=pe_size/etl;
    if(threeD &&(pe2_size<slices))
    sview_zf=TRUE;


     if(mFidSeek(rInfo.fidMd, (realtime_block+1), sizeof(dfilehead), rInfo.bbytes))
     {
     (void)sprintf(str,"recon_all: mFidSeek error for block %d\n",realtime_block);
     Werrprintf(str);
     (void)recon_abort();
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
  
     /*
    if(error = D_getbuf(D_USERFILE, nblocks, realtime_block, &inblock))
    {
        Werrprintf("recon_all: error reading block 0");
        (void)recon_abort();
        ABORT;
    }
    bhp = inblock.head;
    cblockdata=inblock.data;
    */
 
    ctcount = (int) (bhp->ctcount);

    if(within_nt){
      nt=nts[(realtime_block/within_nt)%ntlen];
    }
    else
    {
        Werrprintf("recon_all: within_nt equal to zero");
        (void)recon_abort();
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
            Werrprintf("recon_all: aborting by request");
            (void)recon_abort();
            ABORT;
        }
        
  //      cblockdata=inblock.data; /* copy pointer to data */
        
        blockctr=realtime_block++;
        if(realtime_block>=nblocks)
            acq_done=TRUE;
        
        if(reallybig) {
            sprintf(str,"reading block %d\n",blockctr);
            Winfoprintf(str);
        }
  

        nt_scale=rInfo.image_scale;
	if(variableNT)
	  nt_scale /= (ctcount);
	else
	  nt_scale=rInfo.image_scale/nt;
         

        /* what channel is this? */
        ichan= blockctr%rInfo.nchannels;
        if(ichan==(rInfo.nchannels-1))
        lastchan=TRUE;
        else
        lastchan=FALSE;

        /* reset nt */
        if(within_nt)
	  {
	    nt=nts[(realtime_block/within_nt)%ntlen];
	  }
        else
        {
            Werrprintf("recon_all: within_nt equal to zero");
            (void)recon_abort();
            ABORT;
        }


        /* point to data for this block */
        inblock.head = bhp;
        floatstatus = bhp->status & S_FLOAT;
        rInfo.fidMd->offsetAddr += sizeof(dblockhead);
        inblock.data = (float *)(rInfo.fidMd->offsetAddr);
        


        /* get dc offsets if necessary */
        if(rInfo.dc_flag)
        {
            real_dc=nt_scale*(bhp->lvl);
            imag_dc=nt_scale*(bhp->tlt);
        }
        
 
        if(epi_seq)
        {
            if(dimafter)
            icnt=(blockctr/dimafter)%(imglen)+1;
            else
            {
                Werrprintf("recon_all:dimafter equal to zero");
                (void)recon_abort();
                ABORT;
            }
            error=P_getreal(PROCESSED,"image",&rimflag,icnt);
            if(error)
            {
                Werrprintf("recon_all: Error getting image element");
                (void)recon_abort();
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
            sdataptr=(short *)inblock.data;
            else if (floatstatus == 0)
            idataptr=(int *)inblock.data;
            else
            fdataptr=(float *)inblock.data;

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
                {
                	if(rInfo.nchannels >0)
                		(void)arrayfdf((blockctr/rInfo.nchannels), rInfo.narray, rInfo.arrayelsP, fdfstr);
                	else
                  		(void)arrayfdf(blockctr, rInfo.narray, rInfo.arrayelsP, fdfstr);
                }
            }

            for(itrc=0;itrc<ntraces;itrc++)
	      {

				if (interuption) {
					error = P_setstring(CURRENT, "wnt", "", 0);
					error = P_setstring(PROCESSED, "wnt", "", 0);
					Werrprintf("recon_all: aborting by request");
					(void) recon_abort();
					ABORT;
				}
        


                /* figure out where to put this echo */
                navflag=FALSE;
                status=svcalc(itrc,blockctr,&(rInfo.svinfo),&view,&slice,&echo,&nav,&slab);
                if(status)
                {
                    (void)recon_abort();
                    ABORT;
                }

                oview=view;
                slab=0;

                if(nav>-1)
                navflag=TRUE;

                if((imflag_neg||imflag)&&multi_shot&&!navflag)
                {
                	if(skview_order)
                		view=skview_order[itrc];
                	else
                		view=view_order[view];
                	
                    if(threeD)
                    {
                    	if(sskview_order)
                    		slice=sskview_order[itrc];
                    	else if(sview_order)
                    		slice=sview_order[oview];
                    }
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
                        if(!navflag&&lastchan&&(pc_option!=OFF))
                        *(pcd+slice)=(*(pcd+ichan+ nchannels*slice)) | IMG_DONE;
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
                    if (sdataptr)
                    for(iro=0;iro<2*ro2;iro++)
                    {
                        s1=ntohs(*sdataptr++);
                        *(datptr+soffset+iro)=nt_scale*s1;
                    }
                    else if(idataptr)
                    for(iro=0;iro<2*ro2;iro++)
                    {
                        l1=ntohl(*idataptr++);
                        *(datptr+soffset+iro)=nt_scale*l1;
                    }
                    else /* data is float */
                    for(iro=0;iro<2*ro2;iro++)
                    {
                        memcpy(&l1, fdataptr, sizeof(int));
                        fdataptr++;
                        l1=ntohl(l1);
                        memcpy(&ftemp,&l1, sizeof(int));
                        *(datptr+soffset+iro)=nt_scale*ftemp;
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
                        pt1=datptr+soffset+(nro2-ro2);
                        wptr=read_window;
                        for(iro=0;iro<ro2;iro++)
                        {
                            *pt1 *= *wptr;
                            pt1++;
                            *pt1 *= *wptr;
                            pt1++;
                            wptr++;
                        }
                    }

                    if(navflag&&nav_window)
                    {
                        pt1=datptr+soffset+(nro2-ro2);
                        wptr=nav_window;
                        for(iro=0;iro<ro2;iro++)
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
                        (void)fftshift(nrptr,nro2);
                        (void)fft(nrptr, nro2,pwr, level, COMPLEX,COMPLEX,-1.0,1.0,nro2);

                    }

                    /* phase correction application */
                    if(!navflag&&(imflag||imflag_neg)&&(pc_option!=OFF))
                    {
                        pc_temp=pc;
                        if(imflag_neg)
                        pc_temp=pc_neg;
                        if((pc_option!=OFF)&&(*(pcd+im_slice*nchannels + ichan) & PHS_DONE))
                        {
                            /* pc_offset=im_slice*echoes*npts*2+echo*npts*2+view*nro*2; */
                            pc_offset=im_slice*nchannels*echoes*npts*2+
                            		echo*nchannels*npts*2+ichan*npts*2 + oview*nro*2;
                            if(threeD)
                            pc_offset=im_slice*echoes*nchannels*npts3d*2+
								echo*nchannels*npts3d*2+ichan*npts3d*2+oview*nro*2;
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
            
    //        D_release(D_USERFILE,(realtime_block-1));
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
                            *(pcd+nchannels*(rInfo.pc_slicecnt)+ichan)=(*(pcd+nchannels*(rInfo.pc_slicecnt)+ichan)) | PHS_DONE;
                            pc_offset=rInfo.pc_slicecnt*nchannels*npts*2 + ichan*npts*2;
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
                            *(pcd+nchannels*(rInfo.pc_slicecnt)+ichan)=(*(pcd+nchannels*(rInfo.pc_slicecnt)+ichan)) | PHS_DONE;
                             pc_offset=rInfo.pc_slicecnt*nchannels*npts*2 + ichan*npts*2;

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
            if(pcflag_neg && (pc_option==TRIPLE_REF) && ( *(pcneg_done+nchannels*pcslice+ichan) & IMG_DONE))
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

                        pc_offset=(pcslice+ispb)*nchannels*echoes*npts*2+
                        		echo*nchannels*npts*2+ichan*2*npts+oview*nro*2;

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

            /* do 2nd transform if possible */
            if(phase_compressed&&(imflag||(imflag_neg&&(pc_option==TRIPLE_REF)))&&(!threeD))
            {

                magoffset=0;
                roffset=0;
                fptr=slicedata;
                if(imflag_neg)
                fptr=slicedata_neg;
                if(nnav)
                datptr=navdata;
                if(slice_compressed)
                for(ispb=0;ispb<slicesperblock;ispb++)
                blockrepeat[ispb]=-1;

                for(ispb=0;ispb<slicesperblock;ispb++)
                {
                    /* which unique slice is this? */
                    uslice=slice;
                    if(slice_compressed)
                    uslice=ispb;
                    urep=0;
                    ibr=1;
                    if(uslicesperblock != slicesperblock)
                    {
                        i=-1;
                        j=-1;
                        while((j<ispb)&&(i<uslicesperblock))
                        {
                            i++;
                            j+=blockreps[i];
                            uslice=i;
                        }
                        urep= blockrepeat[uslice];
                        urep++;
                        blockrepeat[uslice]=urep;
                        ibr=blockreps[uslice];
                    }

                    /* bail out  if haven't done phase correction for triple reference */
                    if(!((imflag_neg)&&!(*(pcd+ispb*nchannels+ichan) & PHS_DONE)))
                    {
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

                            if(ichan&&(!sense))
                            {
                                if(phsflag)
                                {
                                    (void)phase_ft(fptr,nro,views,phase_window,mag2+magoffset,phase_reverse,
                                    zeropad,zeropad2,ro_frq,pe_frq, imgphs2+magoffset,MAGNITUDE);
                                    for(ipt=0;ipt<npts;ipt++)
                                    {
                                        magnitude[magoffset+ipt]+=mag2[magoffset+ipt];
                                        imgphs[magoffset+ipt]+=imgphs2[magoffset+ipt];
                                    }
                                }
                                else
                                {
                                    if(epi_dualref && (pc_option==TRIPLE_REF))
                                    {
                                        if(imflag)
                                        {
                                            (void)phase_ft(fptr,nro,views,phase_window,imdata2+2*magoffset,phase_reverse,
                                            zeropad,zeropad2,ro_frq,pe_frq, NULL, COMPLEX);
                                            for(ipt=0;ipt<2*npts;ipt++)
                                            imdata[2*magoffset+ipt]+=imdata2[2*magoffset+ipt];
                                        }
                                        if(imflag_neg)
                                        {
                                            (void)phase_ft(fptr,nro,views,phase_window,imdata2_neg+2*magoffset,phase_reverse,
                                            zeropad,zeropad2,ro_frq,pe_frq, NULL, COMPLEX);
                                            for(ipt=0;ipt<2*npts;ipt++)
                                            imdata_neg[2*magoffset+ipt]+=imdata2_neg[2*magoffset+ipt];
                                        }
                                    }
                                    else /* this is the usual case */
                                    {
                                        (void)phase_ft(fptr,nro,views,phase_window,mag2+magoffset,phase_reverse,
                                        zeropad,zeropad2,ro_frq,pe_frq, NULL, MAGNITUDE);
                                        for(ipt=0;ipt<npts;ipt++)
                                        magnitude[magoffset+ipt]+=mag2[magoffset+ipt];
                                    }
                                }
                            }
                            else if(sense)
                            (void)phase_ft(fptr,nro,views,phase_window,magnitude+magoffset,phase_reverse,
                            zeropad,zeropad2,ro_frq,pe_frq,imgphs+magoffset, MAGNITUDE);
                            else
                            {
                                if(phsflag)
                                (void)phase_ft(fptr,nro,views,phase_window,magnitude+magoffset,phase_reverse,
                                zeropad,zeropad2,ro_frq,pe_frq,imgphs+magoffset, MAGNITUDE);
                                else
                                {
                                    if(epi_dualref && (pc_option==TRIPLE_REF))
                                    {
                                        if(imflag)
                                        (void)phase_ft(fptr,nro,views,phase_window,imdata+2*magoffset,phase_reverse,
                                        zeropad,zeropad2,ro_frq,pe_frq, NULL, COMPLEX);
                                        if(imflag_neg)
                                        (void)phase_ft(fptr,nro,views,phase_window,imdata_neg+2*magoffset,phase_reverse,
                                        zeropad,zeropad2,ro_frq,pe_frq, NULL, COMPLEX);
                                    }
                                    else /* this is the usual case */
                                    (void)phase_ft(fptr,nro,views,phase_window,magnitude+magoffset,phase_reverse,
                                    zeropad,zeropad2,ro_frq,pe_frq,NULL, MAGNITUDE);
                                }
                            }

                            /* write out images and/or raw data */
                            if(lastchan||sense||smash)
                            {
                                if(rInfo.nchannels)
                                irep=(rep/(rInfo.nchannels));
                                else
                                {
                                    Werrprintf("recon_all: nchannels equal to zero");
                                    (void)recon_abort();
                                    ABORT;
                                }

                                irep=irep*ibr + urep +1;

                                if((rInfo.nchannels>1)&&(lastchan)&&(!sense))
                                {
                                    for(ipt=0;ipt<npts;ipt++)
                                    magnitude[magoffset+ipt]/=(rInfo.nchannels);
                                }

                                if(epi_dualref&&(pc_option==TRIPLE_REF)&&imflag&&lastchan&&(!sense))
                                {
                                    /* scale the complex sum of channels and pos & neg readout images */
                                    for(ipt=0;ipt<2*npts;ipt++)
                                    {
                                        imdata[2*magoffset+ipt] += imdata_neg[2*magoffset+ipt];
                                        imdata[2*magoffset+ipt]/=(2*(rInfo.nchannels));
                                    }

                                    /* compute the magnitude */
                                    fdptr=imdata+2*magoffset;
                                    for(ipt=0;ipt<npts;ipt++)
                                    {
                                        a1=*fdptr++;
                                        b1=*fdptr++;
                                        magnitude[magoffset+ipt] = (float)sqrt(a1*a1 + b1*b1);
                                    }
                                }

                                rInfo.picInfo.echo=iecho+1;
                                rInfo.picInfo.slice=rInfo.slicecnt+1;
                                rInfo.picInfo.slice=uslice+1;

                                if(rawflag)
                                {
                                    (void)strcpy(tempdir,rInfo.picInfo.imdir);
                                    itemp=rInfo.picInfo.fullpath;
                                    rInfo.picInfo.fullpath=FALSE;
                                    if((rawflag==RAW_MAG)||(rawflag==RAW_MP))
                                    {
                                        (void)strcpy(rInfo.picInfo.imdir,"rawmag");
                                        display=DISPFLAG(rInfo.dispcnt, rInfo.dispint);
                                        rInfo.dispcnt++;
                                        if(smash)
                                        status=write_fdf(irep,rawmag+magoffset, &(rInfo.picInfo),
                                        &(rInfo.rawmag_order),display,fdfstr,threeD, (ichan+1));
                                        else if(lastchan)
                                        status=write_fdf(irep,rawmag+magoffset, &(rInfo.picInfo),
                                        &(rInfo.rawmag_order),display,fdfstr,threeD, ch0);
                                        if(status)
                                        {
                                            (void)recon_abort();
                                            ABORT;
                                        }
                                    }
                                    if((rawflag==RAW_PHS)||(rawflag==RAW_MP))
                                    {
                                        (void)strcpy(rInfo.picInfo.imdir,"rawphs");
                                        display=DISPFLAG(rInfo.dispcnt, rInfo.dispint);
                                        rInfo.dispcnt++;
                                        if(smash)
                                        status=write_fdf(irep,rawphs+magoffset, &(rInfo.picInfo),
                                        &(rInfo.rawphs_order),display,fdfstr,threeD, (ichan+1));
                                        else if(lastchan)
                                        status=write_fdf(irep,rawphs+magoffset, &(rInfo.picInfo),
                                        &(rInfo.rawphs_order),display,fdfstr,threeD, ch0);
                                        if(status)
                                        {
                                            (void)recon_abort();
                                            ABORT;
                                        }
                                    }
                                    (void)strcpy(rInfo.picInfo.imdir,tempdir);
                                    rInfo.picInfo.fullpath=itemp;
                                }
                                display=DISPFLAG(rInfo.dispcnt, rInfo.dispint);
                                rInfo.dispcnt++;
                                if(sense)
                                {
                                    /* write phase of image data */
                                    (void)strcpy(tempdir,rInfo.picInfo.imdir);
                                    itemp=rInfo.picInfo.fullpath;
                                    rInfo.picInfo.fullpath=FALSE;
                                    (void)strcpy(rInfo.picInfo.imdir,"reconphs");
                                    display=FALSE;
                                    status=write_fdf(irep,imgphs+magoffset, &(rInfo.picInfo),
                                    &(rInfo.phase_order),display,fdfstr,threeD,(ichan+1));
                                    (void)strcpy(rInfo.picInfo.imdir,tempdir);
                                    rInfo.picInfo.fullpath=itemp;

                                    /* write magnitude image data */
                                    display=TRUE;
                                    status=write_fdf(irep,magnitude+magoffset, &(rInfo.picInfo),
                                    &(rInfo.phase_order),display,fdfstr,threeD,(ichan+1));
                                    if(status)
                                    {
                                        (void)recon_abort();
                                        ABORT;
                                    }
                                }
                                else if(lastchan&&imflag)
                                {
                                    if(phsflag)
                                    {
                                        (void)strcpy(tempdir,rInfo.picInfo.imdir);
                                        itemp=rInfo.picInfo.fullpath;
                                        rInfo.picInfo.fullpath=FALSE;
                                        (void)strcpy(rInfo.picInfo.imdir,"reconphs");
                                        status=write_fdf(irep,imgphs+magoffset, &(rInfo.picInfo),
                                        &(rInfo.phase_order),display,fdfstr,threeD,ch0);
                                        if(status)
                                        {
                                            (void)recon_abort();
                                            ABORT;
                                        }
                                        (void)strcpy(rInfo.picInfo.imdir,tempdir);
                                        rInfo.picInfo.fullpath=itemp;
                                    }

                                    if(look_locker){
                                    	temp1=irep; temp2=rInfo.picInfo.echo;
                                    	irep=(temp1-1)*rInfo.picInfo.echoes+temp2;
                                    	rInfo.picInfo.echo=1;
                                    	temp3=rInfo.picInfo.echoes;
                                    	rInfo.picInfo.echoes=1;
                                    	temp4=rInfo.picInfo.image;
                                    	rInfo.picInfo.image=temp3*temp4;
                                    	
                                    	// get corresponding inversion time
                                        error=P_getreal(PROCESSED,"ti",&dtemp,temp2);
                                           if (error) {
                                               Werrprintf("recon_all: recon_all: Error getting ti");
                                               (void)recon_abort();
                                               ABORT;
                                           }
                                        rInfo.picInfo.ti=(float)SEC_TO_MSEC*dtemp;
                              
                                    	 status=write_fdf(irep,magnitude+magoffset, &(rInfo.picInfo),
                                    	                                    &(rInfo.image_order),display,fdfstr,threeD,ch0);
                                    	 irep=temp1; rInfo.picInfo.echo=temp2; rInfo.picInfo.echoes=temp3;rInfo.picInfo.image=temp4;
                                    }
                                    else	
                                    	status=write_fdf(irep,magnitude+magoffset, &(rInfo.picInfo),
															&(rInfo.image_order),display,fdfstr,threeD,ch0);
                                    if(status)
                                    {
                                        (void)recon_abort();
                                        ABORT;
                                    }
                                }
                            }
                            fptr+=2*npts;
                            magoffset+=npts;
                            datptr+=2*nshots*nnav*nro;
                        } /* end of echo loop */
                        rInfo.slicecnt++;
                        rInfo.slicecnt=rInfo.slicecnt%slices;
                    } /* end of if bail out due to image_neg and no phase correction */
                } /* end of slice loop */
            }
        } /* end if imflag or doing phase correction */

        /* see if recon has been forced */
        error=P_getreal(CURRENT,"recon_force",&recon_force,1);
        if(!error)
        {
            if(recon_force>0.0)
            {
                error=P_setreal(CURRENT,"recon_force",0.0,1); /* turn it back off */
                if(!phase_compressed)
                (void)generate_images(fdfstr);
            }
        }

        /* point to header of next block and get new ctcount */
        if(!acq_done)
        {
            if(mFidSeek(rInfo.fidMd, (realtime_block+1), sizeof(dfilehead), rInfo.bbytes))
             {
                 (void)sprintf(str,"recon_all: mFidSeek error for block %d\n",realtime_block);
                 Werrprintf(str);
                 (void)recon_abort();
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
             
             /*
            if(error = D_getbuf(D_USERFILE, nblocks, realtime_block, &inblock))
            {
                (void)sprintf(str,"recon_all: D_getbuf error for block %d\n",realtime_block);
                Werrprintf(str);
                (void)recon_abort();
                ABORT;
            }
            bhp = inblock.head;
            */
        }

    }
    /**************************/
    /* end loop on blocks    */
    /**************************/
    if(acq_done)
    {
 //       D_close(D_USERFILE);        
        *fdfstr='\0';
        if((!phase_compressed) && (!threeD)) /* process data for each slice now */
            (void)generate_images(fdfstr);

        if(threeD) /* process all 3D data */
        (void)generate_images3D(fdfstr);

#ifdef VNMRJ		
//        (void)aipUpdateRQ();

	/* sort by slices */
//        (void)execString("rqsort2=1 aipRQcommand('display','',1,1)\n");

#endif
        (void)releaseAllWithId("recon_all");
        
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
static int generate_images(char *arrstr) {
    float ro_frq, pe_frq;
    float *fptr;
    float *window;
    double dtemp;
    int slices, views, nro, slice_reps, nchannels;
    int multi_shot, *dispcntptr, dispint, zeropad, zeropad2;
    int phsflag;
    int threeD=FALSE;
    int itemp;
    int rawflag, smash, sense;
    int i, j;
    int blkreps;
    int ibr;
    int uslices;
    int uslice;
    int echoes;
    int urep;
    int slice, rep, ipt;
    int ichan, irep;
    int npts;
    int echo;
    int phase_reverse;
    int display;
    int status;
    int lastchan;
    int ch0=0;
    int temp1, temp2, temp3, temp4;
    char tempdir[MAXPATHL];
    char arstr[MAXSTR];

    fptr=slicedata;

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

    uslices=slices;
    npts=views*nro;



    if (blockreps) {
        for (i=0; i<slices; i++) {
            if (blockreps[i]>0) {
                blkreps=TRUE;
                uslices=i;
            }
        }
    }

    if (blockrepeat)
        for (slice=0; slice<slices; slice++)
            blockrepeat[slice]=-1;

    
   
    
    for (slice=0; slice<slices; slice++) {
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
                    
                    fptr=slicedata + slice*slice_reps*echoes*2*npts + 
                        rep*nchannels*echoes*2*npts + ichan*echoes*2*npts
                        + echo*2*npts; 

                    if (nmice&&pe_frqP)
                        pe_frq=pe_frqP[ichan];

                    if (ichan) {
                        if (sense)
                            (void)phase_ft(fptr, nro, views, window, magnitude,
                                    phase_reverse, zeropad, zeropad2, ro_frq,
                                    pe_frq, imgphs, MAGNITUDE);
                        else if (phsflag) {
                            (void)phase_ft(fptr, nro, views, window, mag2,
                                    phase_reverse, zeropad, zeropad2, ro_frq,
                                    pe_frq, imgphs2, MAGNITUDE);
                            for (ipt=0; ipt<npts; ipt++) {
                                magnitude[ipt]+=mag2[ipt];
                                imgphs[ipt]+=imgphs2[ipt];
                            }
                        } else {
                            (void)phase_ft(fptr, nro, views, window, mag2,
                                    phase_reverse, zeropad, zeropad2, ro_frq,
                                    pe_frq, NULL, MAGNITUDE);
                            for (ipt=0; ipt<npts; ipt++)
                                magnitude[ipt]+=mag2[ipt];
                        }
                    } else {
                        if (sense)
                            (void)phase_ft(fptr, nro, views, window, magnitude,
                                    phase_reverse, zeropad, zeropad2, ro_frq,
                                    pe_frq, imgphs, MAGNITUDE);

                        else if (phsflag)
                            (void)phase_ft(fptr, nro, views, window, magnitude,
                                    phase_reverse, zeropad, zeropad2, ro_frq,
                                    pe_frq, imgphs, MAGNITUDE);
                        else
                            (void)phase_ft(fptr, nro, views, window, magnitude,
                                    phase_reverse, zeropad, zeropad2, ro_frq,
                                    pe_frq, NULL, MAGNITUDE);
                    }
                    // fptr+=2*npts;

                    rInfo.picInfo.echo=echo+1;
                    rInfo.picInfo.slice=slice+1;
                    rInfo.picInfo.slice=uslice+1;
                    irep=rep*ibr + urep +1;
                    
                    arstr[0]='\0';
                    if(rInfo.narray)
                    {
                    		(void)arrayfdf(irep-1, rInfo.narray, rInfo.arrayelsP, arstr);
                    }

                       
                    if (rawflag) {
                        (void)strcpy(tempdir, rInfo.picInfo.imdir);
                        itemp=rInfo.picInfo.fullpath;
                        rInfo.picInfo.fullpath=FALSE;
                        status=0;
                        if ((rawflag==RAW_MAG)||(rawflag==RAW_MP)) {
                            (void)strcpy(rInfo.picInfo.imdir, "rawmag");
                            display=DISPFLAG(rInfo.dispcnt, rInfo.dispint);
                            rInfo.dispcnt++;
                            if (smash)
                                status=write_fdf(irep, rawmag,
                                        &(rInfo.picInfo),
                                        &(rInfo.rawmag_order), display, arstr,
                                        threeD, (ichan+1));
                            else if (lastchan)
                                status=write_fdf(irep, rawmag,
                                        &(rInfo.picInfo),
                                        &(rInfo.rawmag_order), display, arstr,
                                        threeD, ch0);
                            if (status) {
                                (void)recon_abort();
                                ABORT;
                            }
                        }
                        if ((rawflag==RAW_PHS)||(rawflag==RAW_MP)) {
                            (void)strcpy(rInfo.picInfo.imdir, "rawphs");
                            display=DISPFLAG(rInfo.dispcnt, rInfo.dispint);
                            rInfo.dispcnt++;
                            if (smash)
                                status=write_fdf(irep, rawphs,
                                        &(rInfo.picInfo),
                                        &(rInfo.rawphs_order), display, arstr,
                                        threeD, (ichan+1));
                            else if (lastchan)
                                status=write_fdf(irep, rawphs,
                                        &(rInfo.picInfo),
                                        &(rInfo.rawphs_order), display, arstr,
                                        threeD, ch0);
                            if (status) {
                                (void)recon_abort();
                                ABORT;
                            }
                        }
                        /* restore imgdir */
                        (void)strcpy(rInfo.picInfo.imdir, tempdir);
                        rInfo.picInfo.fullpath=itemp;
                    }

                    if (sense) {
                        (void)strcpy(tempdir, rInfo.picInfo.imdir);
                        itemp=rInfo.picInfo.fullpath;
                        rInfo.picInfo.fullpath=FALSE;
                        (void)strcpy(rInfo.picInfo.imdir, "reconphs");
                        display=FALSE;
                        status=write_fdf(irep, imgphs, &(rInfo.picInfo),
                                &(rInfo.phase_order), display, arstr, threeD,
                                (ichan+1));
                        (void)strcpy(rInfo.picInfo.imdir, tempdir);
                        rInfo.picInfo.fullpath=itemp;

                        /* write magnitude image data */
                        display=TRUE;
                        status=write_fdf(irep, magnitude, &(rInfo.picInfo),
                                &(rInfo.phase_order), display, arstr, threeD,
                                (ichan+1));
                        if (status) {
                            (void)recon_abort();
                            ABORT;
                        }
                        /* restore imgdir */
                        (void)strcpy(rInfo.picInfo.imdir, tempdir);
                        rInfo.picInfo.fullpath=itemp;
                    } else if (lastchan) {
                        if (nchannels>1) {
                            for (ipt=0; ipt<npts; ipt++)
                                magnitude[ipt]/=nchannels;
                        }
                        display=DISPFLAG(rInfo.dispcnt, rInfo.dispint);
                        rInfo.dispcnt++;
                        
                        
                        if(rInfo.svinfo.look_locker){
                         	temp1=irep; temp2=rInfo.picInfo.echo;
                         	irep=(temp1-1)*rInfo.picInfo.echoes+temp2;
                         	rInfo.picInfo.echo=1;
                         	temp3=rInfo.picInfo.echoes;
                         	rInfo.picInfo.echoes=1;
                         	temp4=rInfo.picInfo.image;
                         	rInfo.picInfo.image=temp3*temp4;
                         	
                         	// get corresponding inversion time
                             status=P_getreal(PROCESSED,"ti",&dtemp,temp2);
                                if (status) {
                                    Werrprintf("recon_all: recon_all: Error getting ti");
                                    (void)recon_abort();
                                    ABORT;
                                }
                             rInfo.picInfo.ti=(float)SEC_TO_MSEC*dtemp;
                   
                         	 status=write_fdf(irep,magnitude, &(rInfo.picInfo),
                         	                                    &(rInfo.image_order),display,arstr,FALSE,ch0);
                         	 irep=temp1; rInfo.picInfo.echo=temp2; rInfo.picInfo.echoes=temp3;rInfo.picInfo.image=temp4;
                         }
                        else {  // this is the standard case
							status = write_fdf(irep, magnitude,
									&(rInfo.picInfo), &(rInfo.image_order),
									display, arstr, FALSE, ch0);
						}
                        
                        if (status) {
                            (void)recon_abort();
                            ABORT;
                        }

                        if (phsflag) {
                            (void)strcpy(tempdir, rInfo.picInfo.imdir);
                            itemp=rInfo.picInfo.fullpath;
                            rInfo.picInfo.fullpath=FALSE;
                            (void)strcpy(rInfo.picInfo.imdir, "reconphs");
                            status=write_fdf(irep, imgphs, &(rInfo.picInfo),
                                    &(rInfo.phase_order), display, arstr,
                                    threeD, ch0);
                            (void)strcpy(rInfo.picInfo.imdir, tempdir);
                            rInfo.picInfo.fullpath=itemp;
                            if (status) {
                                (void)recon_abort();
                                ABORT;
                            }
                            /* restore imgdir */
                            (void)strcpy(rInfo.picInfo.imdir, tempdir);
                            rInfo.picInfo.fullpath=itemp;
                        }

                    }
                } /* end of channel loop */
            }
        }
    }
    RETURN;
}

/******************************************************************************/
/* this is the 3D version                                                                */
/******************************************************************************/
static int generate_images3D(char *arstr) {
    vInfo info;
    float ro_frq, pe_frq;
    float *fptr;
    float *window;
    float *window2;
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
    int nomouse=0;
    char tempdir[MAXPATHL];
    char str[200];
    double *pe2_frq;
    double dtemp;
    double dfov;
    double pefrq;

    fptr=slicedata;
    blkreps=FALSE;
    slice = 0;

    /* get relevant parameters from struct */
    slices=rInfo.svinfo.slices;
    views=rInfo.picInfo.npe;
    nro=rInfo.picInfo.nro;
    slice_reps=rInfo.svinfo.slice_reps;
    nchannels=rInfo.nchannels;
    window=rInfo.pwindow;
    window2=rInfo.swindow;
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
    npts2d=views*nro;
    npts3d=npts2d*slices;
    fnchan=(smash|sense) ? nchannels:1;
    choffset=(smash|sense) ? 1:nchannels;
    
    if (blockreps) {
        for (i=0; i<slices; i++) {
            if (blockreps[i]>0) {
                blkreps=TRUE;
                uslices=i;
            }
        }
    }

    if (blockrepeat)
        for (slice=0; slice<slices; slice++)
            blockrepeat[slice]=-1;
    

    pe2_frq=NULL;
    error=P_getVarInfo(CURRENT,"ppe2",&info);
    if (!error && info.active) {
        nshifts=info.size;
        if (nshifts != slabs) {
            Werrprintf("recon_all: length of ppe2 less than slabs ");
            (void)recon_abort();
            ABORT;
        }
        pe2_frq=(double *)allocateWithId(slabs*sizeof(double), "recon_all");

        error=P_getreal(PROCESSED,"lpe2",&dfov,1);
        if (error) {
            Werrprintf("recon_all: Error getting lpe2");
            (void)recon_abort();
            ABORT;
        }

        for (islab=0; islab<slabs; islab++) {
            error=P_getreal(CURRENT,"ppe2",&dtemp,islab+1);
            if (error) {
                Werrprintf("recon_all: Error getting ppe2 ");
                (void)recon_abort();
                ABORT;
            }
            pe2_frq[islab]=-180.0*dtemp/dfov;
        }
    }

    offset=0;
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
                    if (ichan) {
                        if (phsflag) {
                            (void)threeD_ft(fptr, nro, views, slices, window, window2,
                                    phase_reverse, zeropad, zeropad2, nomouse,
                                    imgphs2, &pefrq, pe2_frq);

                            for (ipt=0; ipt<npts3d; ipt++)
                                imgphs[ipt]+=imgphs2[ipt];

                        } else
                            (void)threeD_ft(fptr, nro, views, slices, window, window2,
                                    phase_reverse, zeropad, zeropad2, nomouse, 
                                    NULL, &pefrq, pe2_frq);

                    } else {
                        if (phsflag)
			  (void)threeD_ft(fptr, nro, views, slices, window, window2,
                                    phase_reverse, zeropad, zeropad2, nomouse,
                                    imgphs+offset, &pefrq, pe2_frq);
                        else           
			  (void)threeD_ft(fptr, nro, views, slices, window,window2,
                                    phase_reverse, zeropad, zeropad2, nomouse,
                                    NULL, &pefrq, pe2_frq);
                    }
                    fptr+=2*npts3d;
                    offset += npts3d;
                } /* end ichan loop */


                /* never output any 2D fdf files for 3D */
                if (FALSE&&(dispint||sense||rawflag||phsflag)) {
                    for (islice=0; islice<slices; islice++) { /* slice loop */
                        rInfo.picInfo.echo=echo+1;
                        threeDslab=islab + 1;
                        rInfo.picInfo.slice=islab*slices + islice + 1;
                        /* 		  rInfo.picInfo.slice=uslice+1;*/
                        irep=rep*ibr + urep +1;

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
                                    (void)recon_abort();
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
                                    (void)recon_abort();
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
                                (void)recon_abort();
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
                                (void)recon_abort();
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

                        status=write_fdf(irep, magnitude+offset,
                                &(rInfo.picInfo), &(rInfo.image_order),
                                display, arstr, threeDslab, ch0);
                        if (status) {
                            (void)recon_abort();
                            ABORT;
                        }
                        offset+=npts2d;
                    } /* end slice loop */
                }/* end if dispint */
            } /* end of echo loop */
        } /* end of rep loop */
    }/* end of slab loop */

    
    /* get rid of any fdf files first */
    (void)sprintf(str,
            "%s/datadir3d/data/",curexpdir);
    (void)unlinkFilesWithSuffix(str, ".fdf");

    
    /* create a big fdf containing all data  for aipExtract */
    /* need to reverse readout and phase directions for this */
    rInfo.picInfo.echoes=echoes;
    rInfo.picInfo.datatype=MAGNITUDE;
    fptr=slicedata;
    for (islab=0; islab<slabs; islab++) {
        rInfo.picInfo.slice=islab+1;
        for (rep=0; rep<(slice_reps/nchannels); rep++) {

            arstr[0]='\0';
            if (rInfo.narray) {
					(void)arrayfdf(rep, rInfo.narray, rInfo.arrayelsP, arstr);
			}


            rInfo.picInfo.image=(float)(rep+1.0);
            for (echo=0; echo<echoes; echo++) {
                rInfo.picInfo.echo=echo+1;
                for (ichan=0; ichan<fnchan; ichan++) {
                    status=write_3Dfdf(fptr, &(rInfo.picInfo), arstr,ichan);
                    if (status) {
                        (void)recon_abort();
                        ABORT;
                    }
                    fptr+=2*npts3d*choffset;
                }
            }
        }
    }    
    
    
    /* copy first image 3D fdf to data.fdf in case someone needs it there */
    /*
    (void)sprintf(str,
            "cp %s/datadir3d/data/img_slab001image001echo001.fdf %s/datadir3d/data/data.fdf \n",
            curexpdir, curexpdir);
    ierr=system(str);
    */
 
    /* do all that for raw magnitude if needed */
    if ((rawflag==RAW_MAG) || (rawflag==RAW_MP)) {
        rInfo.picInfo.datatype=RAW_MAG;
        fptr=rawmag;
        for (islab=0; islab<slabs; islab++) {
            rInfo.picInfo.slice=islab+1;
            for (rep=0; rep<(slice_reps/nchannels); rep++) {
                arstr[0]='\0';
                if(rInfo.narray)
    					  (void)arrayfdf(rep, rInfo.narray, rInfo.arrayelsP, arstr);
                rInfo.picInfo.image=(float)(rep+1.0);
                for (echo=0; echo<echoes; echo++) {
                    rInfo.picInfo.echo=echo+1;
                    for (ichan=0; ichan<fnchan; ichan++) {
                        status=write_3Dfdf(fptr, &(rInfo.picInfo), arstr, ichan);
                        if (status) {
                            (void)recon_abort();
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
                arstr[0]='\0';
                if(rInfo.narray)
    					  (void)arrayfdf(rep, rInfo.narray, rInfo.arrayelsP, arstr);
                rInfo.picInfo.image=(float)(rep+1.0);
                for (echo=0; echo<echoes; echo++) {
                    rInfo.picInfo.echo=echo+1;
                    for (ichan=0; ichan<fnchan; ichan++) {
                        status=write_3Dfdf(fptr, &(rInfo.picInfo), arstr, ichan);
                        if (status) {
                            (void)recon_abort();
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
                arstr[0]='\0';
                if(rInfo.narray)
    					  (void)arrayfdf(rep, rInfo.narray, rInfo.arrayelsP, arstr);
                for (echo=0; echo<echoes; echo++) {
                    rInfo.picInfo.echo=echo+1;
                    for (ichan=0; ichan<fnchan; ichan++) {
                        status=write_3Dfdf(fptr, &(rInfo.picInfo), arstr, ichan);
                        if (status) {
                            (void)recon_abort();
                            ABORT;
                        }
                        fptr+=2*npts3d*choffset;
                    }
                }
            }
        }
    }             
        
    
    
    RETURN;
}

/********************************************************************************/
/* halfFourier                                                                  */
/********************************************************************************/
void halfFourier(float *data, int nsamp, int nfinal, int method)
{
    int i;
    int offset;
    int nsmall;
    int nfill;
    int nf2;
    int pwr, fnt;

    float *fptr;
    float *fptr2;
    float *cdata;
    float *tdata;
    float t1, t2;

    double a, b;
    double d1, d2;
    double *phstmp;
    double *dptr;

    nf2=nfinal/2;
    nsmall=nsamp-nf2;
    nsmall*=2;
    offset=nfinal-nsamp;
    offset*=2; /* complex data */
    pwr = 4;
    fnt = 32;
    while (fnt < 2*nfinal) {
        fnt *= 2;
        pwr++;
    }

    cdata=(float *)recon_allocate(2*nfinal*sizeof(float), "halfFourier");
    tdata=(float *)recon_allocate(2*nfinal*sizeof(float), "halfFourier");
    phstmp=(double *)recon_allocate(nfinal*sizeof(double), "halfFourier");

    /* zero these buffers */
    for (i=0; i<2*nfinal; i++) {
        cdata[i]=0.0;
        tdata[i]=0.0;
    }

    /* copy data into tdata and perform  shift  as data is zerofilled at back*/

    fptr=tdata+offset;
    fptr2=data;
    for (i=0; i<2*nsamp; i++)
        *fptr++=*fptr2++;

    /* get FT of center data and find phase */
    fptr=tdata+offset;
    for (i=0; i<2*nsmall; i++)
        cdata[i+offset]=*fptr++;

    (void)fft(cdata, nfinal, pwr, 0, COMPLEX, COMPLEX, -1.0, 1.0, nfinal);
    (void)getphase(0, nfinal, 1, phstmp, cdata, FALSE);
    if (method==LINEAR) {
        (void)phaseunwrap(phstmp, nfinal, phstmp);
        (void)polyfit(phstmp, nfinal, phstmp, LINEAR);
    }

    /* FT data, phase correct, then IFT */
    (void)fft(tdata, nfinal, pwr, 0, COMPLEX, COMPLEX, -1.0, 1.0, nfinal);

    fptr=tdata;
    dptr=phstmp;
    for (i=0; i<nfinal; i++) {
        d1=*fptr;
        d2=*(fptr+1);
        a=cos(*dptr);
        b=-sin(*dptr++);
        *fptr++=(float) (d1*a-d2*b);
        *fptr++=(float)(a*d2+b*d1);
    }

    fftshift(tdata, nfinal);
    (void)fft(tdata, nfinal, pwr, 0, COMPLEX, COMPLEX, 1.0, -1.0, nfinal);
    fftshift(tdata, nfinal);

    /* copy data back to original location*/
    /* data is shifted for some reason */
    /*
     fptr=data+2;
     fptr2=tdata+2*nfinal-1;
     for(i=0;i<2*(nfinal-1);i++)
     *fptr++=*fptr2--;
     *data=*tdata;
     *(data+1)=*(tdata+1);
     */

    fptr=data;
    fptr2=tdata;
    for (i=0; i<2*nfinal; i++)
        *fptr++=*fptr2++;

    /* now synthesize missing data */
    nfill=nfinal-nsamp;
    fptr=data;
    fptr2=data+2*nfinal-1;
    for (i=0; i<nfill; i++) {
        t1=*fptr++;
        t2=-1.0*(*fptr++);
        *fptr2--=t2;
        *fptr2--=t1;
    }

    releaseAllWithId("halfFourier");

    return;
}

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

    nav_phase=(double *)recon_allocate(tnav*npts*sizeof(double), "nav_correct");
    uphs = (double *)recon_allocate(npts*sizeof(double), "nav_correct");

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
/* phase_ft is also responsible for reversing magnitude data in read & phase directions (new fft)  */
/* int output_type  MAGNITUDE or COMPLEX */
/********************************************************************************************************/
static void phase_ft(float *xkydata, int nx, int ny, float *win,
        float *absdata, int phase_rev, int zeropad_ro, int zeropad_ph,
        float ro_frq, float ph_frq, float *phsdata, int output_type) {
    float a, b;
    float *fptr, *pptr, *pt1, *pt2;
    int ix, iy;
    int np;
    int pwr, fnt;
    int offset;
    int halfR=FALSE;
    int halfP=FALSE;
    float *nrptr;
    float templine[2*MAXPE];
    float *tempdat = NULL;
    fptr=absdata;
    pptr=phsdata;

    np=nx*ny;

    if (zeropad_ph > HALFF*ny)
        halfP=TRUE;

    if (zeropad_ro > HALFF*nx) {
        halfR=TRUE;
        tempdat=(float *)recon_allocate(2*np*sizeof(float), "phase_ft");
    }

    /* do phase direction ft */
    for (ix=nx-1; ix>-1; ix--) {

		if (interuption) {
			(void)P_setstring(CURRENT, "wnt", "", 0);
			(void)P_setstring(PROCESSED, "wnt", "", 0);
			Werrprintf("recon_all: aborting by request");
			//(void) recon_abort();
			return;
		}
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

        if (win) {
            for (iy=0; iy<ny; iy++) {
                templine[2*iy]*= *(win+iy);
                templine[2*iy+1]*= *(win+iy);
            }
        }

        if (halfP) {
            /* perform half Fourier data estimation */
            (void)halfFourier(templine, (ny-zeropad_ph), ny, POINTWISE);
            if (phase_rev) {
                offset=2*(ny-1);
                /* reverse after doing the processing */
                for (iy=0; iy<ny; iy++) {
                    a=templine[2*iy];
                    b=templine[2*iy+1];
                    templine[2*iy]=templine[offset-2*iy];
                    templine[2*iy+1]=templine[offset-2*iy+1];
                    templine[offset-2*iy]=a;
                    templine[offset-2*iy+1]=b;
                }
            }
        }

        /* apply frequency shift */
        if (ph_frq!=0.0) {
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
        (void)fftshift(nrptr,ny);
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
        } else /*  write result */
        {
            if (output_type==MAGNITUDE) {
                nrptr=templine+2*ny-1;
                for (iy=0; iy<ny; iy++) {
                    a=*nrptr--;
                    b=*nrptr--;
                    *fptr++=(float)sqrt((double)(a*a+b*b));
                    if (phsdata)
                        *pptr++=(float)atan2(b, a);
                }
            } else if (output_type==COMPLEX) {
                nrptr=templine+2*ny-1;
                for (iy=0; iy<ny; iy++) {
                    a=*nrptr--;
                    b=*nrptr--;
                    *fptr++=(float)a;
                    *fptr++=(float)b;
                    if (phsdata)
                        *pptr++=(float)atan2(b, a);
                }
            }

        }
    }

    if (halfR) {
        /* perform half Fourier in readout */
        nrptr=tempdat;
        for (iy=0; iy<ny; iy++) {
            (void)halfFourier(nrptr, (nx-zeropad_ro), nx, POINTWISE);
            pwr = 4;
            fnt = 32;
            while (fnt < 2*ny) {
                fnt *= 2;
                pwr++;
            }
            /* apply frequency shift */
            if (ro_frq!=0.0)
                (void)rotate_fid(nrptr, 0.0, ro_frq, 2*nx, COMPLEX);
            (void)fft(nrptr, nx, pwr, 0, COMPLEX, COMPLEX, -1.0, 1.0, nx);
            nrptr+=2*nx;
        }
        /* write out result */
        if (output_type==MAGNITUDE) {
            for (ix=nx-1; ix>-1; ix--) {
                for (iy=ny-1; iy>-1; iy--) {
                    nrptr=tempdat+2*ix+iy*2*nx;
                    a=*nrptr++;
                    b=*nrptr++;
                    *fptr++=(float)sqrt((double)(a*a+b*b));
                    if (phsdata)
                        *pptr++=(float)atan2(b, a);
                }
            }
        } else if (output_type==COMPLEX) {
            for (ix=nx-1; ix>-1; ix--) {
                for (iy=ny-1; iy>-1; iy--) {
                    nrptr=tempdat+2*ix+iy*2*nx;
                    a=*nrptr++;
                    b=*nrptr++;
                    *fptr++=(float)a;
                    *fptr++=(float)b;
                    if (phsdata)
                        *pptr++=(float)atan2(b, a);
                }
            }
        }
    }

    (void)releaseAllWithId("phase_ft");

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
int pc_pick(char *pc_str)
{
    int pc_opt;

    /* decipher phase correction option */
    if (strstr(pc_str, "OFF")||strstr(pc_str, "off"))
        pc_opt=OFF;
    else if (strstr(pc_str, "POINTWISE")||strstr(pc_str, "pointwise"))
        pc_opt=POINTWISE;
    else if (strstr(pc_str, "LINEAR")||strstr(pc_str, "linear"))
        pc_opt=LINEAR;
    else if (strstr(pc_str, "QUADRATIC")||strstr(pc_str, "quadratic"))
        pc_opt=QUADRATIC;
    else if (strstr(pc_str, "CENTER_PAIR")||strstr(pc_str, "center_pair"))
        pc_opt=CENTER_PAIR;
    else if (strstr(pc_str, "PAIRWISE")||strstr(pc_str, "pairwise"))
        pc_opt=PAIRWISE;
    else if (strstr(pc_str, "FIRST_PAIR")||strstr(pc_str, "first_pair"))
        pc_opt=FIRST_PAIR;
    else if (strstr(pc_str, "TRIPLE_REF")||strstr(pc_str, "triple_ref"))
        pc_opt=TRIPLE_REF;
    else
        pc_opt=10000;

    return (pc_opt);
}

/******************************************************************************/
/******************************************************************************/
/* returns slice and view number given trace and block number */
int svcalc(trace, block, svI, view, slice, echo, nav, slab)
    int trace, block;svInfo *svI;int *view;int *slice;int *echo;int *nav;int
            *slab; {
    int v, s, sl, sv;
    int d;
    int i;
    int nav1 = 0;
    int nec, ec;
    int netl;
    int n;
    int seg;
    int nnavs;
    int navflag;
    int viewsize, slabsize, sviewsize, segsize;

    netl=svI->etl;
    nnavs=(svI->pe_size)/(netl);
    nnavs*=(svI->nnav);
    nec=0;
    *nav=-1;
    *slab=0;
    navflag=FALSE;

    if (svI->threeD) {
        if (svI->phase_compressed)
            viewsize=svI->etl;
        else
            viewsize=1;
        

        if (svI->multi_shot)
            segsize=svI->pe_size/(svI->etl);
        else
            segsize=1;

        if(svI->phase_compressed)
            segsize=svI->pe_size/(svI->etl);

        if (svI->slab_compressed)
            slabsize=svI->slabs;
        else
            slabsize=1;

        if (svI->sliceenc_compressed)
            sviewsize=svI->pe2_size;
        else
            sviewsize=1;

        ec=trace%((svI->etl)*(svI->echoes));

        if (svI->multi_slice) {
            sl=trace/(viewsize*svI->echoes); /* slab number */
            seg=trace/(viewsize*slabsize*svI->echoes); /* segment number */
            sv=trace/(viewsize*slabsize*segsize*svI->echoes); /* slice encode view */

            /* sv=trace/(viewsize);              *//* slice encode number */
            /* sl=trace/(viewsize*sviewsize);         *//* slab number */
            /* seg=trace/(viewsize*sviewsize*slabsize);  *//* segments are outermost */

        } else {
            seg=trace/(viewsize*svI->echoes); /* segment number */
            sl=trace/(viewsize*segsize*svI->echoes); /* slab number */
            sv=trace/(viewsize*slabsize*segsize*svI->echoes); /* slice encode view */

            sv=trace/(viewsize*svI->echoes); /* slice encode number */
            sl=trace/(viewsize*sviewsize*svI->echoes); /* slab number */
            seg=trace/(viewsize*sviewsize*slabsize*svI->echoes); /* segments are outermost */
        }

        v=seg*viewsize+ec;

        if (!svI->slab_compressed)
            sl=(block/svI->within_slabs);
        if (!svI->phase_compressed)
            v=(block/svI->within_views);
        if (!svI->sliceenc_compressed)
            sv=(block/svI->within_sviews);

        *view=v%(svI->pe_size);
        *slab=sl%(svI->slabs);
        *slice=sv%(svI->pe2_size);
    } else /* ***************** 2D case below this point ****** */
    {
        if (svI->nnav) {
            netl+=svI->nnav;
            /* figure out how many navigators preceed this */
            d=(netl);
            if (d)
                ec=(trace%d);
            else {
                Werrprintf("recon_all:svcalc: divide by zero");
                (void)recon_abort();
                ABORT;
            }
            i=0;
            while ((i<svI->nnav)&&(!navflag)) {
                if (nav_list[i]<ec)
                    nec++;
                else if (nav_list[i]==ec) {
                    nav1=i;
                    navflag=TRUE;
                }
                i++;
            }
            if (navflag) {
                /* kludge to make view even or odd for echo reversal in epi */
                if ((nav_list[0]==0)&&((svI->nnav)%2))
                    nec=-1;
                else
                    nec=0;

                if (svI->multi_slice) {
                    d=(netl)*(svI->slicesperblock)*(svI->echoes);
                    if (d)
                        n=trace/d; /* segment */
                    else {
                        Werrprintf("recon_all:svcalc: divide by zero");
                        (void)recon_abort();
                        ABORT;
                    }
                    n*=svI->nnav; /* times navigators per segment */
                    /*	      n+=ec;  *//* plus echo  */
                    n+=nav1; /* plus echo  */
                    *nav=n;
                } else
                    *nav=trace;
                *nav=(*nav)%(nnavs);
            }
        } /* end if navigators */

        if (svI->look_locker||svI->multi_shot||svI->epi_seq||svI->nnav) {
            if (svI->phase_compressed) {
                if (svI->multi_slice)
                    d=(netl)*(svI->slicesperblock)*(svI->echoes);
                else
                    d=(netl)*(svI->echoes);
                if (d)
                    v=trace/d; /* segment */
                else {
                    Werrprintf("recon_all:svcalc: divide by zero");
                    (void)recon_abort();
                    ABORT;
                }
                v*=svI->etl; /* times etl */

               if (svI->look_locker==1)
					d = 1.0; // views change fastest
               else if (svI->look_locker==2)
            	   d = (svI->echoes) * (svI->slicesperblock);
				else
					//	d=(svI->slicesperblock)*(svI->echoes);
					d = (svI->echoes);
                if (d)
                    v+=((trace/d)%(netl)); /* plus echo  */
                else {
                    Werrprintf("recon_all:svcalc: divide by zero");
                    (void)recon_abort();
                    ABORT;
                }
                v=v-nec;
            } else /* not phase compressed but still multi-shot (like FSE) */
            {
                d=svI->within_views;
                if (d)
                    v=(block/d);
                else {
                    Werrprintf("recon_all:svcalc: divide by zero");
                    (void)recon_abort();
                    ABORT;
                }
                v*=svI->etl; /* times etl */

                if ((svI->multi_slice) && (svI->etl==1))
                    d=(svI->slicesperblock)*(svI->echoes);
                else if(svI->look_locker)
                	d=1; // views change innermost for look locker
                else if (svI->look_locker==2)
                    d = (svI->echoes) * (svI->slicesperblock);
                else
                    d=(svI->echoes);
                if (d)
                    v+=((trace/d)%(netl)); /* plus echo  */
                else {
                    Werrprintf("recon_all:svcalc: divide by zero");
                    (void)recon_abort();
                    ABORT;
                }
                v=v-nec;
            }
            if (svI->slice_compressed) {
                if (svI->multi_slice) {
                    d=netl;
                   if(svI->look_locker)d *= (svI->echoes);
                   if(svI->look_locker == 2)d =1;  // slices innermost
                    if (d)
                        s=trace/d;
                    else {
                        Werrprintf("recon_all:svcalc: divide by zero");
                        (void)recon_abort();
                        ABORT;
                    }
                } else {
                    d=svI->viewsperblock;
                    if (d)
                        s=trace/d;
                    else {
                        Werrprintf("recon_all:svcalc: divide by zero");
                        (void)recon_abort();
                        ABORT;
                    }
                }
            } else /* not slice compressed */
            {
                d=(svI->within_slices);
                if (d)
                    s=(block/d);
                else {
                    Werrprintf("recon_all:svcalc: divide by zero");
                    (void)recon_abort();
                    ABORT;
                }
            }

        } else /* not multi_shot */
        {
            if (svI->phase_compressed) {
                if (svI->multi_slice)
                    d=(svI->slicesperblock)*(svI->echoes);
                else
                    d=(svI->echoes);
                if (d)
                    v=trace/d;
                else {
                    Werrprintf("recon_all:svcalc: divide by zero");
                    (void)recon_abort();
                    ABORT;
                }
            } else {
                d=(svI->within_views);
                if (d)
                    v=(block/d);
                else {
                    Werrprintf("recon_all:svcalc: divide by zero");
                    (void)recon_abort();
                    ABORT;
                }
            }
            if (svI->slice_compressed) {
                if (svI->multi_slice)
                    d=(svI->echoes);
                else
                    d=(svI->viewsperblock)*(svI->echoes);
                if (d)
                    s=trace/d;
                else {
                    Werrprintf("recon_all:svcalc: divide by zero");
                    (void)recon_abort();
                    ABORT;
                }
            } else {
                d=(svI->within_slices);
                if (d)
                    s=(block/d);
                else {
                    Werrprintf("recon_all:svcalc: divide by zero");
                    (void)recon_abort();
                    ABORT;
                }
            }
        }
        *view=v%(svI->pe_size);
        *slice=s%(svI->slices);
    } /* end if not threeD */

    if (svI->epi_seq) {
		d = (netl) * (svI->slicesperblock);
		if (d)
			*echo = trace / d;
		else {
			Werrprintf("recon_all:svcalc: divide by zero");
			(void) recon_abort();
			ABORT;
		}
		*echo = (*echo) % (svI->echoes);
	} else if (svI->look_locker == 1) {
		d = netl;
		*echo = (trace / d) % (svI->echoes); // echoes outside etl loop
	} else if (svI->look_locker == 2) {
		d = (svI->slicesperblock);
		*echo = (trace / d) % (svI->echoes); // echoes outside etl loop
	} else
		*echo = trace % (svI->echoes);

    return (0);
}

/******************************************************************************/
/******************************************************************************/
char *recon_allocate(nsize, caller_id)
    int nsize;char *caller_id; {
    char *ptr;
    char str[MAXSTR];

    ptr=NULL;
    ptr=allocateWithId(nsize, caller_id);
    if (!ptr) {
        (void)sprintf(str, "recon_all: Error mallocing size of %d \n", nsize);
        Werrprintf(str);
        (void)recon_abort();
        return (NULL);
    }

    return (ptr);
}

/******************************************************************************/
/******************************************************************************/
void recon_abort() {
  //  D_close(D_USERFILE);
    
    if (rInfo.fidMd) {
        mClose(rInfo.fidMd);
        rInfo.fidMd=NULL;
    }
    recon_aborted=TRUE;
    (void)releaseAllWithId("recon_all");
    return;
}

/******************************************************************************/
/******************************************************************************/
int svib_image(pname, image)
    char *pname;int image; {
    char arraystr[MAXSTR];
    char errstr[MAXSTR];
    char *imptr=NULL;
    char *lp=NULL;
    char *rp=NULL;
    char *nmptr=NULL;
    int i=0;
    int error;
    int imcnt=0;
    double rimflag;

    error=P_getstring(PROCESSED,"array", arraystr, 1, MAXSTR);
    if (error) {
        Werrprintf("recon_all: svib_image: Error getting array");
        (void)recon_abort();
        ABORT;
    }

    lp=strstr(arraystr, "(");
    imptr=strstr(arraystr, "image");
    nmptr=strstr(arraystr, pname);
    rp=strstr(arraystr, ")");

    if (((imptr>lp)&&(nmptr>lp)&&(imptr<rp)&&(nmptr<rp)) || ( rInfo.svinfo.epi_seq)){
        /* how many when image=1? */
        i=0;
        while(imcnt<image)  
        {
            error=P_getreal(PROCESSED,"image",&rimflag,(i+1));
            if (error) {
                sprintf(errstr,"recon_all: svib_image: Error getting image element %d \n",i+1);
                Werrprintf(errstr);
                (void)recon_abort();
                return(-1);
            }
            if (rimflag==1.0)
                imcnt++;
            i++;
        }
    }
    else
        i=image;

    return (i);
}

/******************************************************************************/
/******************************************************************************/
int write_fdf(imageno, datap, fI, image_orderP, display, arstr, threeD, channel)
    int imageno;fdfInfo *fI;float *datap;int *image_orderP;int display;char
            *arstr;int threeD;int channel; {
    MFILE_ID fdfMd;
    vInfo info;
    char *ptr;
    char *ptr2;
    char sviblist[MAXSTR];
    char filename[MAXPATHL];
    char dirname[MAXPATHL];
    char svbname[SHORTSTR];
    char ctemp[SHORTSTR];
    char str[100];
    char hdr[2000];
    int i, error;
    int ierr; 
    int pad_cnt, align;
    int hdrlen;
    int filesize;
    int ior;
    int slice_acq, slice_pos;
    int nimage;
    int id;
    int w1, w2, w3;
    double rppe, rpro, rpss;
    double orppe, orpro, orpss;
    double psi, phi, theta;
    double cospsi, cosphi, costheta;
    double sinpsi, sinphi, sintheta;
    double or0, or1, or2, or3, or4, or5, or6, or7, or8;
    double te;
    double dtemp;

    if (fI->fullpath)
        (void)strcpy(dirname, fI->imdir);
    else {
        (void)strcpy(dirname, curexpdir);
        (void)strcat(dirname, "/");
        (void)strcat(dirname, fI->imdir);
    }
    (void)strcpy(filename, dirname);

    if ((*image_orderP==0)&&!channel) {
        (void)unlinkFilesWithSuffix(dirname, ".fdf");
    }

    slice_acq=fI->slice;
    if (!threeD) {
        i=0;
        while (slice_acq != slice_order[i])
            i++;
        slice_pos=i+1;
    } else {
        slice_pos=fI->slice;
        slice_acq=threeD;
    }


        w1= (fI->slices)> 999 ? 1 + (int)log10(fI->slices) : 3;
        w2= (fI->image)> 999 ? 1 + (int)log10(fI->image) : 3;
        w3= (fI->echoes)> 999 ? 1 + (int)log10(fI->echoes) : 3;
      
       
       (void)sprintf(str, "/slice%0*dimage%0*decho%0*d", w1,slice_pos, w2, imageno,
                       w3, fI->echo);
       

    (void)strcat(filename, str);
    if (channel) {
        (void)sprintf(str, "coil%03d", channel);
        (void)strcat(filename, str);
    }
    (void)strcat(filename, ".fdf");

    /* get some slice or image specific information */
    rpss=0.0;
    if (nmice) {
        rpss=0.0;
        error=P_getreal(PROCESSED,"mpss",&rpss,channel);
        if (threeD)
            rpss += (slice_pos- (fI->slices/2))*fI->thickness;
        else {
            error=P_getreal(PROCESSED,"pss",&dtemp,slice_acq);
            if (error) {
                Werrprintf("recon_all: write_fdf: Error getting slice offset pss");
                (void)recon_abort();
                ABORT;
            }
            rpss+=dtemp;
        }

        rppe=0.0;
        error=P_getreal(PROCESSED,"mppe",&rppe,channel);
        /* account for pe direction shift in recon */
        rppe += (fI->fovpe)*(rInfo.pe_frq/180.);

        rpro=0.0;
        error=P_getreal(PROCESSED,"mpro",&rpro,channel);

        orppe=rppe;
        orpro=rpro;
        orpss=rpss;
        error=P_getreal(GLOBAL,"moriginx",&orpro,channel);
        error=P_getreal(GLOBAL,"moriginy",&orppe,channel);
        error=P_getreal(GLOBAL,"moriginz",&orpss,channel);
    } else {
        if (threeD)
            rpss += (slice_pos- (fI->slices/2))*fI->thickness;
        else {
            error=P_getreal(PROCESSED,"pss",&rpss,slice_acq);
            if (error) {
                Werrprintf("recon_all: write_fdf: Error getting slice offset");
                (void)recon_abort();
                ABORT;
            }
        }

        error=P_getreal(PROCESSED,"ppe",&rppe,1);
        if (error) {
            Werrprintf("recon_all: write_fdf: Error getting phase encode offset");
            (void)recon_abort();
            ABORT;
        }
        /* account for pe direction shift in recon */
        rppe = (fI->fovpe)*(rInfo.pe_frq/180.);

        error=P_getreal(PROCESSED,"pro",&rpro,1);
        if (error) {
            Werrprintf("recon_all: write_fdf: Error getting readout offset");
            (void)recon_abort();
            ABORT;
        }
        orppe=rppe;
        orpro=rpro;
    }

    /* get orientation for this image */
    ior=(imageno-1)%(fI->npsi)+1;
    error=P_getreal(PROCESSED,"psi",&psi,ior);
    if (error) {
        Werrprintf("recon_all: write_fdf: Error getting psi");
        (void)recon_abort();
        ABORT;
    }
    ior=(imageno-1)%(fI->nphi)+1;
    error=P_getreal(PROCESSED,"phi",&phi,ior);
    if (error) {
        Werrprintf("recon_all: write_fdf: Error getting phi");
        (void)recon_abort();
        ABORT;
    }
    ior=(imageno-1)%(fI->ntheta)+1;
    error=P_getreal(PROCESSED,"theta",&theta,ior);
    if (error) {
        Werrprintf("recon_all: write_fdf: Error getting theta");
        (void)recon_abort();
        ABORT;
    }

    cospsi=cos(DEG_TO_RAD*psi);
    sinpsi=sin(DEG_TO_RAD*psi);
    cosphi=cos(DEG_TO_RAD*phi);
    sinphi=sin(DEG_TO_RAD*phi);
    costheta=cos(DEG_TO_RAD*theta);
    sintheta=sin(DEG_TO_RAD*theta);

    or0=-1*cosphi*cospsi - sinphi*costheta*sinpsi;
    or1=-1*cosphi*sinpsi + sinphi*costheta*cospsi;
    or2=-1*sinphi*sintheta;
    or3=-1*sinphi*cospsi + cosphi*costheta*sinpsi;
    or4=-1*sinphi*sinpsi - cosphi*costheta*cospsi;
    or5=cosphi*sintheta;
    or6=-1*sintheta*sinpsi;
    or7=sintheta*cospsi;
    or8=costheta;

    /* get te if necessary */
    te=fI->te;
    te*=fI->echo;
    if (fI->echoes > 1) {
        error=P_getVarInfo(PROCESSED,"TE",&info);
        if (!error) {
            if (info.size > 1) {
                error=P_getreal(PROCESSED,"TE",&te,(fI->echo -1)%info.size + 1);
                if (error) {
                    Werrprintf("recon_all: write_fdf: Error getting particular TE");
                    (void)recon_abort();
                    ABORT;
                }
            }
        }
    }

    (void)sprintf(hdr, "#!/usr/local/fdf/startup\n");
    (void)strcat(hdr, "float  rank = 2;\n");
    (void)strcat(hdr, "char  *spatial_rank = \"2dfov\";\n");
    (void)strcat(hdr, "char  *storage = \"float\";\n");
    (void)strcat(hdr, "float  bits = 32;\n");
    if(rInfo.phsflag)
      (void)strcat(hdr, "char  *type = \"phase\";\n");
    else
      (void)strcat(hdr, "char  *type = \"absval\";\n");
    (void)sprintf(str, "float  matrix[] = {%d, %d};\n", fI->npe, fI->nro);
    (void)strcat(hdr, str);
    (void)strcat(hdr, "char  *abscissa[] = {\"cm\", \"cm\"};\n");
    (void)strcat(hdr, "char  *ordinate[] = { \"intensity\" };\n");
    (void)sprintf(str, "float  span[] = {%.6f, %.6f};\n", fI->fovpe, fI->fovro);
    (void)strcat(hdr, str);
    (void)sprintf(str, "float  origin[] = {%.6f,%.6f};\n", orppe, orpro);
    (void)strcat(hdr, str);
    if (nmice) {
        /*       (void)sprintf(str,"int    coils = %d;\n",rInfo.nchannels); */
        (void)sprintf(str, "int    coils = %d;\n", nmice);
        (void)strcat(hdr, str);
        (void)sprintf(str, "int    coil = %d;\n", channel);
        (void)strcat(hdr, str);
        (void)sprintf(str, "float  morigin = {%.6f,%.6f,%.6f};\n", orpro,
                orppe, orpss);
        /*       (void)sprintf(str,"float  morigin = {%.6f,%.6f,%.6f};\n",orppe,orpro,orpss); */
        (void)strcat(hdr, str);
        rppe *= (-1);
    }
    (void)sprintf(str, "char  *nucleus[] = {\"%s\",\"%s\"};\n",fI->tn,fI->dn);
    (void)strcat(hdr, str);
    (void)sprintf(str, "float  nucfreq[] = {%.6f,%.6f};\n", fI->sfrq, fI->dfrq);
    (void)strcat(hdr, str);
    (void)sprintf(str, "float  location[] = {%.6f,%.6f,%.6f};\n", rppe, rpro,
            rpss);
    (void)strcat(hdr, str);
    (void)sprintf(str, "float  roi[] = {%.6f,%.6f,%.6f};\n", fI->fovpe,
            fI->fovro, fI->thickness);
    (void)strcat(hdr, str);
    (void)sprintf(str, "float  gap = %.6f;\n", fI->gap);
    (void)strcat(hdr, str);
    (void)sprintf(str, "char  *file = \"%s\";\n", fI->fidname);
    (void)strcat(hdr, str);
    /*   (void)sprintf(str,"int    slice_no = %d;\n",slice_acq); */
    (void)sprintf(str, "int    slice_no = %d;\n", slice_pos);
    (void)strcat(hdr, str);
    (void)sprintf(str, "int    slices = %d;\n", fI->slices);
    (void)strcat(hdr, str);
    (void)sprintf(str, "int    echo_no = %d;\n", fI->echo);
    (void)strcat(hdr, str);
    (void)sprintf(str, "int    echoes = %d;\n", fI->echoes);
    (void)strcat(hdr, str);
    if (!arraycheck("te", arstr)) {
        (void)sprintf(str, "float  TE = %.3f;\n", te);
        (void)strcat(hdr, str);
        (void)sprintf(str, "float  te = %.6f;\n", MSEC_TO_SEC*te);
        (void)strcat(hdr, str);
    }
    if (!arraycheck("tr", arstr)) {
        (void)sprintf(str, "float  TR = %.3f;\n", fI->tr);
        (void)strcat(hdr, str);
        (void)sprintf(str, "float  tr = %.6f;\n", MSEC_TO_SEC*fI->tr);
        (void)strcat(hdr, str);
    }
    (void)sprintf(str, "int ro_size = %d;\n", fI->ro_size);
    (void)strcat(hdr, str);
    (void)sprintf(str, "int pe_size = %d;\n", fI->pe_size);
    (void)strcat(hdr, str);
    (void)sprintf(str, "char *sequence = \"%s\";\n", fI->seqname);
    (void)strcat(hdr, str);
    (void)sprintf(str, "char *studyid = \"%s\";\n", fI->studyid);
    (void)strcat(hdr, str);
    (void)sprintf(str, "char *position1 = \"%s\";\n", fI->position1);
    (void)strcat(hdr, str);
    (void)sprintf(str, "char *position2 = \"%s\";\n", fI->position2);
    (void)strcat(hdr, str);
    if (!arraycheck("ti", arstr)) {
        (void)sprintf(str, "float  TI =  %.3f;\n", fI->ti);
        (void)strcat(hdr, str);
        (void)sprintf(str, "float  ti =  %.6f;\n", MSEC_TO_SEC*fI->ti);
        (void)strcat(hdr, str);
    }
    /*   (void)sprintf(str,"int    array_index = %d;\n",fI->array_index); */
    (void)sprintf(str, "int    array_index = %d;\n", imageno);
    (void)strcat(hdr, str);
    (void)sprintf(str, "float  array_dim = %.4f;\n", fI->image);
    (void)strcat(hdr, str);
    /*  (void)sprintf(str,"float  image = %.4f;\n",fI->image); */
    (void)sprintf(str, "float  image = 1.0;\n");
    (void)strcat(hdr, str);
    /*   id = (slice_pos-1)*fI->image*fI->echoes + (imageno-1)*fI->echoes + (fI->echo-1); */
    id = (imageno-1)*fI->slices*fI->echoes+ (slice_pos-1)*fI->echoes+ (fI->echo-1);
    //if(rInfo.svinfo.look_locker)
    	// id=(slice_pos-1)*fI->image+ (imageno-1);
    (void)sprintf(str, "int    display_order = %d;\n", id);
    (void)strcat(hdr, str);
#ifdef LINUX
    (void)sprintf(str, "int    bigendian = 0;\n");
    (void)strcat(hdr, str);
#endif
    (void)sprintf(str, "float  imagescale = %.9f;\n",rInfo.image_scale);
    (void)strcat(hdr, str);
    if (!arraycheck("psi", arstr)) {
        (void)sprintf(str, "float  psi = %.4f;\n", psi);
        (void)strcat(hdr, str);
    }
    if (!arraycheck("phi", arstr)) {
        (void)sprintf(str, "float  phi = %.4f;\n", phi);
        (void)strcat(hdr, str);
    }
    if (!arraycheck("theta", arstr)) {
        (void)sprintf(str, "float  theta = %.4f;\n", theta);
        (void)strcat(hdr, str);
    }

    /* check for sviblist stuff */
    error=P_getstring(PROCESSED,"sviblist", sviblist, 1, MAXSTR);
    if (!error) {
        ptr=strtok(sviblist, ",");
        while (ptr != NULL) {
            strcpy(svbname, ptr);
            /* strip off any blanks */
            ptr2=ptr;
            while ((*ptr2==' ')&&(ptr2<(ptr+strlen(svbname))))
                ptr2++;
            strcpy(svbname, ptr2);
            if (!strstr(svbname, "TE")) /* TE handled separately */
            {
                error=P_getVarInfo(PROCESSED,svbname,&info);
                if (error) {
                    (void)sprintf(str,
                            "recon_all: write_fdf: Error getting %s", svbname);
                    /* Werrprintf("recon_all: write_fdf: Error getting something"); */
                    Winfoprintf(str);
                } else {
                    nimage=svib_image(svbname, imageno);
                    if (nimage<0) {
                        Werrprintf("recon_all: write_fdf: Problem with sviblist");
                        (void)recon_abort();
                        ABORT;
                    }
                    nimage=(nimage-1)%info.size+1;
                    switch (info.basicType) {
                    case T_REAL: {
                        error=P_getreal(PROCESSED,svbname,&dtemp,nimage);
                        if (error) {
                            Werrprintf("recon_all: write_fdf: Error getting sviblist variable");
                            (void)recon_abort();
                            ABORT;
                        }
                        (void)sprintf(str, "float  %s = %.6f;\n", svbname,
                                dtemp);
                        (void)strcat(hdr, str);
                    }
                        break;
                    case T_STRING: {
                        error
                                =P_getstring(PROCESSED,svbname,ctemp,nimage, SHORTSTR);
                        if (error) {
                            Werrprintf("recon_all: write_fdf: Error getting sviblist variable");
                            (void)recon_abort();
                            ABORT;
                        }
                        (void)sprintf(str, "char  *%s = \"%s\";\n", svbname,
                                ctemp);
                        (void)strcat(hdr, str);
                    }
                        break;
                    default: {
                        Werrprintf("recon_all: write_fdf: Error getting sviblist variable");
                        (void)recon_abort();
                        ABORT;
                    }
                        break;
                    } /* end switch */
                }
            }
            ptr=strtok(NULL,",");
        }
    }

    (void)sprintf(str,
            "float  orientation[] = {%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f};\n",
            or0, or1, or2, or3, or4, or5, or6, or7, or8);
    (void)strcat(hdr, str);
    (void)strcat(hdr, arstr); /* add array based parameters */
    (void)strcat(hdr, "int checksum = 1291708713;\n");
    (void)strcat(hdr, "\f\n");

    align = sizeof(float);
    hdrlen=strlen(hdr);
    hdrlen++; /* include NULL terminator */
    pad_cnt = hdrlen % align;
    pad_cnt = (align - pad_cnt) % align;
    pad_cnt = pad_cnt + align -1;

    /* Put in padding */
    (void)sprintf(str, " ");
    for (i=0; i<pad_cnt-1; i++)
        (void)strcat(str, " ");

    (void)strcat(str, "\n");
    (void)strcat(hdr, str);

    hdrlen+=strlen(str);

    *(hdr + hdrlen) = '\0';

    /* open the fdf file */
    filesize=hdrlen*sizeof(char)+ fI->nro*fI->npe*sizeof(float);
    if (!access(filename, W_OK)) {
        fdfMd=mOpen(filename, filesize, O_RDWR | O_CREAT);
        (void)mAdvise(fdfMd, MF_SEQUENTIAL);
    } else {
        /* create the output directory and try again */
        if (access(dirname, F_OK)) {
            (void)sprintf(str, "mkdir %s \n", dirname);
            ierr=system(str);
        }
        fdfMd=mOpen(filename, filesize, O_RDWR | O_CREAT);
        (void)mAdvise(fdfMd, MF_SEQUENTIAL);
        if (!fdfMd) {
            Werrprintf("recon_all: write_fdf: Error opening image file");
            (void)recon_abort();
            ABORT;
        }
    }

    if ((*image_orderP==0)&&!channel) {
        if (svprocpar(TRUE, dirname)) {
            Werrprintf("recon_all: write_fdf: Error creating procpar");
            (void)recon_abort();
            ABORT;
        }
    }

    if (!channel)
        *image_orderP=*image_orderP + 1;

    (void)memcpy(fdfMd->offsetAddr, hdr, hdrlen*sizeof(char));
    fdfMd->newByteLen += hdrlen*sizeof(char);
    fdfMd->offsetAddr += hdrlen*sizeof(char);
    (void)memcpy(fdfMd->offsetAddr, datap, fI->nro*fI->npe*sizeof(float));
    fdfMd->newByteLen += fI->nro*fI->npe*sizeof(float);
    (void)mClose(fdfMd);

#ifdef VNMRJ	
    if (display) {
        (void)aipDisplayFile(filename, -1);
        //aipAutoScale();
    }
#endif 

    return (0);
}

/******************************************************************************/
/******************************************************************************/
int write_3Dfdf(datap, fI, arstr, channel)
    fdfInfo *fI;float *datap;char *arstr;int channel; {
    MFILE_ID fdfMd;
    vInfo info;
    char *ptr;
    char *ptr2;
    char sviblist[MAXSTR];
    char filestr[MAXSTR];
    char filename[MAXPATHL];
    char dirname[MAXPATHL];
    char svbname[SHORTSTR];
    char ctemp[SHORTSTR];
    char str[100];
    char hdr[2000];
    int ierr;
    int i, error;
    int pad_cnt, align;
    int hdrlen;
    int filesize;
    int ior;
    int slice_acq, slice_pos;
    int nimage;
    int npts2d;
    int npts3d;
    int reallybig=FALSE;
    int id;
    int nch, ich;
    int iro, iview, islice;
    double rppe, rpro, rpss;
    double orro, orpe, orpe2;
    double psi, phi, theta;
    double cospsi, cosphi, costheta;
    double sinpsi, sinphi, sintheta;
    double or0, or1, or2, or3, or4, or5, or6, or7, or8;
    double te;
    double dtemp;
    float *aptr, *bptr;
    float a, b;
    float *fp2;
    float *mag2D;

    (void)strcpy(dirname, curexpdir);
    (void)strcat(dirname, "/datadir3d/data");
    (void)strcpy(filename, dirname);
    switch (fI->datatype) {
    case RAW_MAG:
        (void)strcat(filename, "/rawmag");
        break;
    case RAW_PHS:
        (void)strcat(filename, "/rawphs");
        break;
    case PHS_IMG:
        (void)strcat(filename, "/imgphs");
        break;
    case MAGNITUDE:
    default:
        (void)strcat(filename, "/img");
        break;
    }

    if ((rInfo.smash)||(rInfo.sense))
        (void)sprintf(str, "_slab%03dimage%03decho%03dchan%02d.fdf", fI->slice,
                (int)(fI->image), fI->echo, (channel+1));
    else
        (void)sprintf(str, "_slab%03dimage%03decho%03d.fdf", fI->slice,
                (int)(fI->image), fI->echo);
    (void)strcat(filename, str);

    
    slice_acq=0;
    slice_pos=0;

    /* get some slice or image specific information */
    error=P_getreal(PROCESSED,"pss",&rpss,1);
    if (error) {
        Werrprintf("recon_all: write_3Dfdf: Error getting slab offset");
        (void)recon_abort();
        ABORT;
    }

    error=P_getreal(PROCESSED,"ppe",&rppe,1);
    if (error) {
        Werrprintf("recon_all: write_3Dfdf: Error getting phase encode offset");
        (void)recon_abort();
        ABORT;
    }

    /* account for pe direction shift in recon */
    rppe = (fI->fovpe)*(rInfo.pe_frq/180.);
    rppe *= -1.0; /* make compatible with ft3d output */
    error=P_getreal(PROCESSED,"pro",&rpro,1);
    if (error) {
        Werrprintf("recon_all: write_3Dfdf: Error getting readout offset");
        (void)recon_abort();
        ABORT;
    }
    rpro *= -1.0; /* make compatible with planning */

    if (nmice) {
        if (pe_frqP)
            rppe += (fI->fovpe)*(*(pe_frqP+channel-1)/180.);
        if (ro_frqP)
            rpro += (fI->fovro)*(*(ro_frqP+channel-1)/180.);
    }

    /* get orientation for this image */
    ior=1;
    error=P_getreal(PROCESSED,"psi",&psi,ior);
    if (error) {
        Werrprintf("recon_all: write_3Dfdf: Error getting psi");
        (void)recon_abort();
        ABORT;
    }
    error=P_getreal(PROCESSED,"phi",&phi,ior);
    if (error) {
        Werrprintf("recon_all: write_3Dfdf: Error getting phi");
        (void)recon_abort();
        ABORT;
    }
    error=P_getreal(PROCESSED,"theta",&theta,ior);
    if (error) {
        Werrprintf("recon_all: write_3Dfdf: Error getting theta");
        (void)recon_abort();
        ABORT;
    }

    cospsi=cos(DEG_TO_RAD*psi);
    sinpsi=sin(DEG_TO_RAD*psi);
    cosphi=cos(DEG_TO_RAD*phi);
    sinphi=sin(DEG_TO_RAD*phi);
    costheta=cos(DEG_TO_RAD*theta);
    sintheta=sin(DEG_TO_RAD*theta);

    /*
     or0=-1*cosphi*cospsi - sinphi*costheta*sinpsi;
     or1=-1*cosphi*sinpsi + sinphi*costheta*cospsi;
     or2=-1*sinphi*sintheta;
     or3=-1*sinphi*cospsi + cosphi*costheta*sinpsi;
     or4=-1*sinphi*sinpsi - cosphi*costheta*cospsi;
     or5=cosphi*sintheta;
     or6=-1*sintheta*sinpsi;
     or7=sintheta*cospsi;
     or8=costheta;
     */

    or0=-1*sinphi*sinpsi - cosphi*costheta*cospsi;
    or1=sinphi*cospsi - cosphi*costheta*sinpsi;
    or2=sintheta*cosphi;
    or3=cosphi*sinpsi - sinphi*costheta*cospsi;
    or4=-1*cosphi*cospsi - sinphi*costheta*sinpsi;
    or5=sintheta*sinphi;
    or6=cospsi*sintheta;
    or7=sinpsi*sintheta;
    or8=costheta;

    /* get te if necessary */
    te=fI->te;
    te*=fI->echo;
    if (fI->echoes > 1) {
        error=P_getVarInfo(PROCESSED,"TE",&info);
        if (!error) {
            if (info.size > 1) {
                error=P_getreal(PROCESSED,"TE",&te,(fI->echo -1)%info.size + 1);
                if (error) {
                    Werrprintf("recon_all: write_3Dfdf: Error getting particular TE");
                    (void)recon_abort();
                    ABORT;
                }
            }
        }
    }

    error=P_getstring(CURRENT, "file", filestr, 1, MAXSTR);
    if (error) {
        Werrprintf("recon_all: write_3Dfdf Error getting file");
        (void)recon_abort();
        ABORT;
    }

    orro=rpro;
    orpe=rppe;
    orpe2=rpss;
    if (!nmice) {
        orro -= .5*(fI->fovro);
        orpe -= .5*(fI->fovpe);
        orpe2 -= .5*(fI->thickness);
    }

    (void)sprintf(hdr, "#!/usr/local/fdf/startup\n");
    (void)strcat(hdr, "float  rank = 3;\n");
    (void)strcat(hdr, "char  *spatial_rank = \"3dfov\";\n");
    (void)strcat(hdr, "char  *storage = \"float\";\n");
    (void)strcat(hdr, "float  bits = 32;\n");
    if(rInfo.phsflag)
      (void)strcat(hdr, "char  *type = \"phase\";\n");
    else
      (void)strcat(hdr, "char  *type = \"absval\";\n");
    (void)sprintf(str, "float  matrix[] = {%d, %d, %d};\n", fI->nro, fI->npe,
            fI->slices);
    (void)strcat(hdr, str);
    (void)strcat(hdr, "char  *abscissa[] = {\"cm\", \"cm\", \"cm\"};\n");
    (void)strcat(hdr, "char  *ordinate[] = { \"intensity\" };\n");
    (void)sprintf(str, "float  span[] = {%.6f, %.6f, %.6f};\n", fI->fovro,
            fI->fovpe, fI->thickness);
    (void)strcat(hdr, str);
    (void)sprintf(str, "float  origin[] = {%.6f,%.6f,%.6f};\n", orro, orpe,
            orpe2);
    (void)strcat(hdr, str);
    (void)sprintf(str, "char  *nucleus[] = {\"%s\",\"%s\"};\n",fI->tn,fI->dn);
    (void)strcat(hdr, str);
    (void)sprintf(str, "float  nucfreq[] = {%.6f,%.6f};\n", fI->sfrq, fI->dfrq);
    (void)strcat(hdr, str);
    (void)sprintf(str, "float  location[] = {%.6f,%.6f,%.6f};\n", rpro, rppe,
            rpss);
    (void)strcat(hdr, str);
    (void)sprintf(str, "float  roi[] = {%.6f,%.6f,%.6f};\n", fI->fovro,
            fI->fovpe, fI->thickness);
    (void)strcat(hdr, str);
    (void)sprintf(
            str,
            "float  orientation[] = {%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f};\n",
            or0, or1, or2, or3, or4, or5, or6, or7, or8);
    (void)strcat(hdr, str);
    (void)sprintf(str, "char *array_name = \"%s\"; \n", "none");
    (void)strcat(hdr, str);
    (void)sprintf(str, "char  *file = \"%s\";\n", fI->fidname);
    (void)strcat(hdr, str);

    /* THIS STUFF IS NOw APPRECIATED!! */
//     (void)sprintf(str,"int    slice_no = %d;\n",slice_pos);
//     (void)sprintf(str,"int    slice_no = 1;\n");
//     (void)strcat(hdr,str);
//     (void)sprintf(str,"int    slices = %d;\n",fI->slices);
//     (void)strcat(hdr,str);
     (void)sprintf(str,"int    slab_no = 1;\n");
     (void)strcat(hdr,str);
     (void)sprintf(str,"int    slabs = %d;\n",rInfo.svinfo.slabs);
     (void)strcat(hdr,str);
     (void)sprintf(str,"int    echo_no = %d;\n",fI->echo);
     (void)strcat(hdr,str);
     (void)sprintf(str,"int    echoes = %d;\n",fI->echoes);
     (void)strcat(hdr,str);
     if(!arraycheck("te",arstr))
     {
     (void)sprintf(str,"float  TE = %.3f;\n",te);
     (void)strcat(hdr,str);
     (void)sprintf(str,"float  te = %.6f;\n",MSEC_TO_SEC*te);
     (void)strcat(hdr,str);
     }
     if(!arraycheck("tr",arstr))
     {
     (void)sprintf(str,"float  TR = %.3f;\n",fI->tr);
     (void)strcat(hdr,str);
     (void)sprintf(str,"float  tr = %.6f;\n",MSEC_TO_SEC*fI->tr);
     (void)strcat(hdr,str);
     }
     (void)sprintf(str,"int ro_size = %d;\n",fI->ro_size);
     (void)strcat(hdr,str);
     (void)sprintf(str,"int pe_size = %d;\n",fI->pe_size);
     (void)strcat(hdr,str);
     (void)sprintf(str,"char *sequence = \"%s\";\n",fI->seqname);
     (void)strcat(hdr,str);
     (void)sprintf(str,"char *studyid = \"%s\";\n",fI->studyid);
     (void)strcat(hdr,str);
     
     (void)sprintf(str,"char *file = \"%s\";\n",filestr);
     (void)strcat(hdr,str);
     (void)sprintf(str,"char *position1 = \"%s\";\n",fI->position1);
     (void)strcat(hdr,str);
     (void)sprintf(str,"char *position2 = \"%s\";\n",fI->position2);
     (void)strcat(hdr,str);
     if(!arraycheck("ti",arstr))
     {
     (void)sprintf(str,"float  TI =  %.3f;\n",fI->ti);
     (void)strcat(hdr,str);
     (void)sprintf(str,"float  ti =  %.6f;\n",MSEC_TO_SEC*fI->ti);
     (void)strcat(hdr,str);
     }
     
//     (void)sprintf(str,"int    array_index = 1;\n");
//     (void)strcat(hdr,str);
     (void)sprintf(str,"int  array_index = %d;\n",(int)(fI->image));
     (void)strcat(hdr,str);
     
     (void)sprintf(str,"float  image = 1.0;\n");
     (void)strcat(hdr,str);
     id=1;
     (void)sprintf(str,"int    display_order = %d;\n",id);
     (void)strcat(hdr,str);
     #ifdef LINUX
     (void)sprintf(str,"int    bigendian = 0;\n");
     (void)strcat(hdr,str);
     #endif
    (void)sprintf(str, "float  imagescale = %.9f;\n",rInfo.image_scale);
    (void)strcat(hdr, str);

     
     if (!arraycheck("psi", arstr)) {
		(void)sprintf(str, "float  psi = %.4f;\n", psi);
		(void)strcat(hdr, str);
	}
	if (!arraycheck("phi", arstr)) {
		(void)sprintf(str, "float  phi = %.4f;\n", phi);
		(void)strcat(hdr, str);
	}
	if (!arraycheck("theta", arstr)) {
		(void)sprintf(str, "float  theta = %.4f;\n", theta);
		(void)strcat(hdr, str);
	}

	/* check for sviblist stuff */

	error=P_getstring(PROCESSED, "sviblist", sviblist, 1, MAXSTR);
	if (!error) {
		ptr=strtok(sviblist, ",");
		while (ptr != NULL) {
			strcpy(svbname, ptr);
			ptr2=ptr;
			while ((*ptr2==' ')&&(ptr2<(ptr+strlen(svbname))))
				ptr2++;
			strcpy(svbname, ptr2);
			if (!strstr(svbname, "TE")) {
				error=P_getVarInfo(PROCESSED, svbname, &info);
				if (error) {
					(void)sprintf(str,
							"recon_all: write_3Dfdf: Error getting %s", svbname);
					Winfoprintf(str);
				} else {
					nimage=svib_image(svbname, 0);
					if (nimage<0) {
						(void)recon_abort();
						ABORT;
					}
					//					nimage=(nimage-1)%info.size+1;
					nimage=(nimage)%info.size+1;
					switch (info.basicType) {
					case T_REAL: {
						error=P_getreal(PROCESSED, svbname, &dtemp, nimage);
						if (error) {
						 
						  sprintf(str,"recon_all: write_3Dfdf: Error getting sviblist variable %s for nimage %d",svbname,nimage);
							Werrprintf(str);
							//   	Werrprintf("recon_all: write_3Dfdf: Error getting sviblist variable");
							(void)recon_abort();
							ABORT;
						}
						(void)sprintf(str, "float  %s = %.6f;\n", svbname,
								dtemp);
						(void)strcat(hdr, str);
					}
						break;
					case T_STRING: {
						error=P_getstring(PROCESSED, svbname, ctemp, nimage,
								SHORTSTR);
						if (error) {
							Werrprintf("recon_all: write_3Dfdf: Error getting sviblist variable");
							(void)recon_abort();
							ABORT;
						}
						(void)sprintf(str, "char  *%s = %s;\n", svbname, ctemp);
						(void)strcat(hdr, str);
					}
						break;
					default: {
						Werrprintf("recon_all: write_3Dfdf: Error getting sviblist variable");
						(void)recon_abort();
						ABORT;
					}
						break;
					}
				}
			}
			ptr=strtok(NULL, ",");
		}
	}

	(void)strcat(hdr, arstr); 
     
    (void)strcat(hdr, "int checksum = 0;\n");
    (void)strcat(hdr, "\f\n");

    align = sizeof(float);
    hdrlen=strlen(hdr);
    hdrlen++; /* include NULL terminator */
    pad_cnt = hdrlen % align;
    pad_cnt = (align - pad_cnt) % align;

    /* Put in padding */
    for (i=0; i<pad_cnt; i++) {
        (void)strcat(hdr, "\n");
        hdrlen++;
    }
    *(hdr + hdrlen) = '\0';

    
    /* open the fdf file */
    filesize=hdrlen*sizeof(char)+ fI->nro*fI->npe*fI->slices*sizeof(float);
    if (!access(filename, W_OK)) {
         fdfMd=mOpen(filename,filesize,O_RDWR | O_CREAT);
         (void)mAdvise(fdfMd, MF_SEQUENTIAL);
        // fptr=fopen(filename, "w");
    } else {
        /* create the output directory and try again */
        if (access(dirname, F_OK)) {
            (void)strcpy(dirname, curexpdir);
            (void)strcat(dirname, "/datadir3d");
            (void)sprintf(str, "mkdir %s \n", dirname);
            ierr=system(str);
            (void)strcpy(dirname, curexpdir);
            (void)strcat(dirname, "/datadir3d/data");
            (void)sprintf(str, "mkdir %s \n", dirname);
            ierr=system(str);
        }

        
         fdfMd=mOpen(filename,filesize,O_RDWR | O_CREAT);
         (void)mAdvise(fdfMd, MF_SEQUENTIAL);
         
        if (!fdfMd) {
            Werrprintf("recon_all: write_3Dfdf: Error opening image file");
            (void)recon_abort();
            ABORT;
        }
    }
         
    
    (void)memcpy(fdfMd->offsetAddr, hdr, hdrlen*sizeof(char));
    fdfMd->newByteLen += hdrlen*sizeof(char);
    fdfMd->offsetAddr += hdrlen*sizeof(char);
    
    npts2d=(fI->nro)*(fI->npe);
    npts3d=npts2d*(fI->slices);
    nch=(rInfo.sense || rInfo.smash) ? 1: rInfo.nchannels; 
    if (npts3d >= BIG3D)
        reallybig=TRUE;
    reallybig=FALSE;
    
    if ((fI->datatype == MAGNITUDE)||(fI->datatype == PHS_IMG)) {
        mag2D=(float *)malloc(npts2d*sizeof(float));
        /* this will combine channels if necessary */
        for (islice=0; islice<fI->slices; islice++) {
            if (reallybig) {
                (void)sprintf(str, "writing slice %d \n", islice);
                Winfoprintf(str);
            }
            (void)memset(mag2D, 0, npts2d*sizeof(*mag2D));    
            for (ich=0; ich<nch; ich++) {
                fp2=mag2D;               
                aptr=datap + ich*2*npts3d + islice*2*npts2d;
                bptr=aptr+1;
                for (iview=0; iview<fI->npe; iview++) {
                    for (iro=0; iro<fI->nro; iro++) {
                        a=*aptr;
                        b=*bptr;
                        aptr+=2;
                        bptr+=2;
                        if(fI->datatype == MAGNITUDE)
                            *fp2++ += (float)sqrt((double)(a*a+b*b));     
                        else if(fI->datatype == PHS_IMG)
                            *fp2++ += (float)atan2(b,a);
                    }
                }
            }
            (void)memcpy(fdfMd->offsetAddr, mag2D, npts2d*sizeof(float));
            fdfMd->newByteLen += npts2d*sizeof(float);
            fdfMd->offsetAddr += npts2d*sizeof(float);
        }
        (void)free(mag2D);
    }
    else
    {  
        /* just dump the data */
        (void)memcpy(fdfMd->offsetAddr, datap, npts3d*sizeof(float));
        fdfMd->newByteLen += npts3d*sizeof(float);
        fdfMd->offsetAddr += npts3d*sizeof(float);
    }
    
    (void)mClose(fdfMd);
  
    
    if (svprocpar(TRUE, dirname)) {
        Werrprintf("recon_all: write_3Dfdf: Error creating procpar");
        (void)recon_abort();
        ABORT;
    }
 
    return (0);
}

/*****************************************************************/
int psscompare(p1, p2)
    const void *p1;const void *p2; {
    int i= *((int *)p1);
    int j= *((int *)p2);

    i=i-1;
    j=j-1;

    if (pss[i]>pss[j])
        return (1);
    else if (pss[i]<pss[j])
        return (-1);
    else
        return (0);
}
/*****************************************************************/
int pcompare(p1, p2)
    const void *p1;const void *p2; {
    float f1= *((float *)p1);
    float f2= *((float *)p2);

    if (f1>f2)
        return (1);
    else if (f1<f2)
        return (-1);
    else
        return (0);
}

/*****************************************************************/
int upsscompare(p1, p2)
    const void *p1;const void *p2; {
    int i= *((int *)p1);
    int j= *((int *)p2);

    i=i-1;
    j=j-1;

    if (upss[i]>upss[j])
        return (1);
    else if (upss[i]<upss[j])
        return (-1);
    else
        return (0);
}

/******************************************************************************/
/*            arraycheck                                                                   */
/******************************************************************************/
/* returns TRUE if param is a member of the arrayed elements */
int arraycheck(char *param, char *arraystr)
{
    int match;
    char str[MAXSTR];

    match=FALSE;
    (void)sprintf(str, " %s =", param);
    if (strstr(arraystr, str))
        match=TRUE;

    return (match);
}

/******************************************************************************/
/*            arrayparse                                                                    */
/******************************************************************************/
/* returns names and sizes from array string*/
int arrayparse(char *arraystring, int *nparams, arrayElement **arrayelsPP, int phasesize, int p2size)
{
    arrayElement *ap;
    char *ptr;
    char arraystr[MAXSTR];
    char aname[MAXSTR];
    int np=0;
    int i, ipar, denom;
    vInfo info;

    (void)strcpy(arraystr, arraystring);
    /* count the array elements */
    ptr=strtok(arraystr, ",");
    while (ptr != NULL) {
        /* is this a jointly arrayed thing? */
        if (strstr(ptr, "(")) {
            while (strstr(ptr, ")")==NULL) /* move to end of jointly arrayed list */
            {
                ptr=strtok(NULL,",");
            }
        }
        np++;
        ptr=strtok(NULL,",");
    }

    (void)strcpy(arraystr, arraystring); /* restore this */
    ap = (arrayElement *)recon_allocate(np*sizeof(arrayElement), "recon_all");

    /* find info about each array element */
    ptr=strtok(arraystr, ",");
    ipar=0;
    while (ptr != NULL) {
        /* is this a jointly arrayed thing? */
        if (strstr(ptr, "(")) {
            i=0;
            ptr++; /* move past the ( */
            while (strstr(ptr, ")")==NULL) /* move to end of jointly arrayed list */
            {
                strcpy(ap[ipar].names[i], ptr);
                i++;
                ptr=strtok(NULL,",");
            }
            *(ptr+strlen(ptr)-1)='\0'; /* overwrite the ) */
            strcpy(ap[ipar].names[i], ptr);
            i++;
            strcpy(aname, ptr);
            (void)P_getVarInfo(PROCESSED,aname,&info);
            ap[ipar].size=info.size;
            ap[ipar].nparams=i;
        } else {
            ap[ipar].nparams=1;
            strcpy(ap[ipar].names[0], ptr);
            strcpy(aname, ptr);
            (void)P_getVarInfo(PROCESSED,aname,&info);
            ap[ipar].size=info.size;
        }
        ptr=strtok(NULL,",");
        ipar++;
    }

    /* compute denominator for block to element conversion */
    denom=phasesize; /* account for phase loop being outermost if not compressed */
    denom=1;
    for (ipar=np-1; ipar>=0; ipar--) {
        ap[ipar].denom=denom;
        if(!strcmp(ap[ipar].names[0], "d2"))
        	denom *= phasesize;
        else if(!strcmp(ap[ipar].names[0], "d3"))
         	denom *= p2size;
        else
        	denom *= ap[ipar].size;
    }


     denom=1;
     for (ipar=np-1; ipar>=0; ipar--) {
         ap[ipar].idenom=denom;
         if(!strcmp(ap[ipar].names[0], "d2"))
         	denom *= 1;
         else if(!strcmp(ap[ipar].names[0], "d3"))
          	denom *= 1;
         else if(!strcmp(ap[ipar].names[0], "pss"))
                  	denom *= 1;
         else
         	denom *= ap[ipar].size;
     }

    *nparams=np;
    *arrayelsPP=ap;

    return (0);
}

/******************************************************************************/
/*            arrayfdf                                                                       */
/******************************************************************************/
int arrayfdf(int block, int np, arrayElement *arrayels, char *fdfstring)
{
    char str[MAXSTR];
    char name[MAXSTR];
    int ip, ij;
    int error;
    int image;
    int arrayno;
    vInfo info;
    double dtemp;
    char ctemp[MAXSTR];

    /* construct string for fdf header */
    (void)strcpy(fdfstring, "");
    for (ip=0; ip<np; ip++) {
        for (ij=0; ij<arrayels[ip].nparams; ij++) {
            image=FALSE;
            strcpy(name, arrayels[ip].names[ij]);
            if (strstr(name, "image"))
                image=TRUE;
            arrayno=block/(arrayels[ip].idenom);
            arrayno=arrayno%(arrayels[ip].size);
            arrayno=arrayno+1;
            error=P_getVarInfo(PROCESSED,name,&info);
            if (strstr(name, "d2"))
                        	break;
            if (strstr(name, "d3"))
                        	break;
            if (strstr(name, "pss"))
                        	break;

            if (error) {
                Werrprintf("recon_all: arrayfdf: Error getting arrayed parameter info");
                (void)recon_abort();
                ABORT;
            }
            switch (info.basicType) {
            case T_REAL: {
                error=P_getreal(PROCESSED,name,&dtemp,arrayno);
                if (error) {
                    Werrprintf("recon_all: arrayfdf: Error getting arrayed variable");
                    (void)recon_abort();
                    ABORT;
                }
                if (image&&(dtemp<1.0)) {
                    (void)sprintf(fdfstring, "\n");
                    return (0); /* nothing to do */
                }
                (void)sprintf(str, "float  %s = %.6f;\n", name, dtemp);
                (void)strcat(fdfstring, str);
            }
                break;
            case T_STRING: {
                error=P_getstring(PROCESSED,name,ctemp,arrayno, SHORTSTR);
                if (error) {
                    Werrprintf("recon_all: arrayfdf: Error getting arrayed variable");
                    (void)recon_abort();
                    ABORT;
                }
                (void)sprintf(str, "char  *%s = %s;\n", name, ctemp);
                (void)strcat(fdfstring, str);
            }
                break;
            default: {
                Werrprintf("recon_all: arrayfdf: Error getting arrayed variable");
                (void)recon_abort();
                ABORT;
            }
                break;
            } /* end switch */
        }
    }
    return (0);
}

/*******************************************************/
/* fits phase over region of reasonable amplitude */
/* input is complex data, output is fitted  phase   */
/* inphs is input phase data,  unwrapped            */
/* fit is linear only                                             */
/********************************************************/
int smartphsfit(float *input, double *inphs, int npts, double *outphs)
{
    int i;
    int width;
    int nfit;
    int nogood=FALSE;
    int maxloc;
    int startpt, endpt;
    float *dp1, *dp2;
    double a, b;
    double maxfactor;
    double sx=0.0;
    double sy=0.0;
    double ssx=0.0;
    double sxy=0.0;
    double x, y;
    double b0, b1;
    double maxamp;
    double *amp;

    amp=(double *)recon_allocate(npts*sizeof(double), "smartphsfit");

    width=PC_FIT_WIDTH*npts/100;
    nfit=2*width;

    /* find max amplitude and its location */
    dp1=input;
    dp2=dp1+1;
    maxloc=0;
    a=*dp1;
    b=*dp2;
    maxamp=sqrt(a*a+b*b);
    for (i=0; i<npts; i++) {
        a=*dp1;
        b=*dp2;
        *(amp+i)=sqrt(a*a+b*b);
        if (*(amp+i) > maxamp) {
            maxamp=*(amp+i);
            maxloc=i;
        }
        dp1+=2;
        dp2=dp1+1;
    }

    /* find a region of good amplitude */
    maxfactor=.8;
    startpt=maxloc;
    startpt=npts/2;
    while ((*(amp+startpt)>maxfactor*maxamp) && (startpt>=0))
        startpt--;
    endpt=maxloc;
    endpt=npts/2;
    while ((*(amp+endpt)>maxfactor*maxamp) && (endpt<npts))
        endpt++;

    if (endpt-startpt<10) {
        /*
         startpt=npts/2-5;
         endpt=npts/2+5;
         */
        nogood=TRUE;
    }

    /* fit the phase from startpt to endpt */
    nfit=0;
    for (i=startpt; i<endpt; i++) {
        x=(double)i;
        y=inphs[i];
        sx += x;
        sy += y;
        ssx += x*x;
        sxy += x*y;
        nfit++;
    }
    b1=(nfit*sxy-sx*sy)/(nfit*ssx-sx*sx);
    b0=(sy-b1*sx)/nfit;

    if (nogood) {
        b0=0;
        b1=0;
    }

    /* generate evaluation */
    for (i=0; i<npts; i++)
        outphs[i]=b0+b1*i;

    return (0);
}
    /********************************************************************************************************/
    /* threeD_ft is also responsible for reversing magnitude data in read & phase directions (new fft)  */
    /********************************************************************************************************/
    void threeD_ft(xkydata,nx,ny,nz,win,win2, phase_rev, zeropad_ro, zeropad_ph, imouse, phsdata,pe_frq, frq_pe2)
         float *xkydata;
         float *win;
	 float *win2;
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
      int reallybig=FALSE;
      int halfR=FALSE;
      float *nrptr;
      float *pptr;
      float templine[2*MAXPE];
      float *tempdat;
      double frq;
      char str[100];
      
     // fptr=absdata;
      pptr=phsdata;
      np=nx*ny;
      if(np*nz>=BIG3D)
          reallybig=TRUE;
      reallybig=FALSE;
       
      if(zeropad_ro > HALFF*nx)
        {
          halfR=TRUE;
          tempdat=(float *)allocateWithId(2*np*sizeof(float),"threeD_ft");
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
          if (reallybig) {
            sprintf(str, "slice ft for y=%d\n", iy);
            Winfoprintf(str);
        }
          
          for (ix = 0; ix < nx; ix++) {

			if (interuption) {
				(void)P_setstring(CURRENT, "wnt", "", 0);
				(void)P_setstring(PROCESSED, "wnt", "", 0);
				Werrprintf("recon_all: aborting by request");
				//(void) recon_abort();
				return;
			}

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

          if(win2)
            {
              for(iz=0;iz<nz;iz++)
            {
              templine[2*iz]*= *(win2+iz);
              templine[2*iz+1]*= *(win2+iz);
            }
	    }
          /* apply frequency shift */
         
          if(frq_pe2!=NULL)
            {
              frq=*(frq_pe2+imouse);
                  (void)rotate_fid(templine, 0.0, frq, 2*nz, COMPLEX);
            }

          nrptr=templine;
          (void)fftshift(nrptr,nz);
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
          if(reallybig){
              sprintf(str,"phase ft for z=%d\n",iz);
              Winfoprintf(str);       
          }

          for (ix = nx - 1; ix > -1; ix--) {
			if (interuption) {
				(void)P_setstring(CURRENT, "wnt", "", 0);
				(void)P_setstring(PROCESSED, "wnt", "", 0);
				Werrprintf("recon_all: aborting by request");
				//(void) recon_abort();
				return;
			}

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
          
          nrptr = templine;
          (void) fftshift(nrptr, ny);
          (void) fft(nrptr, ny, pwr, 0, COMPLEX, COMPLEX, -1.0, 1.0, ny);


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
          /* NOT!! */
          if(!halfR && FALSE)
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
      if(halfR && FALSE)
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
      
      (void)releaseAllWithId("threeD_ft");
          
      return;
    }

    
    static int recon_space_check()
    {
         struct  statvfs freeblocks_buf;
         double  free_kbytes;	/* number of free kbytes available */
         double  req_kbytes;		/* kbytes required for images */
         char    tmpdir[MAXSTR];
         double  req_bytes;
         int headerlen = 2048;  // a generous estimate of fdf header size
         int narray;
        
  /* For a VnmrS or 400-MR the check is done in PSG 
   * (to cover multiple receivers, etc
   */
      if ( P_getstring(SYSTEMGLOBAL, "Console", tmpdir, 1, MAXPATHL) < 0)
      {   Werrprintf("Cannot find the 'Console' parameter");
          return(-1);
      }
      if (strcmp(tmpdir,"vnmrs") == 0) 
         return(0);
      

         sprintf(tmpdir,"%s/.",curexpdir);
         statvfs( tmpdir, &freeblocks_buf);
         free_kbytes = (double) freeblocks_buf.f_bavail;
     
    	
        req_bytes = 4 * rInfo.svinfo.ro_size * rInfo.svinfo.pe_size;
    	
    	if(rInfo.svinfo.threeD){
    		req_bytes *= rInfo.svinfo.pe2_size;
    		req_bytes += headerlen;
    		req_bytes *= rInfo.svinfo.echoes;
    		
    	}
    	else
    	{
    		req_bytes += headerlen;
    		req_bytes *= rInfo.svinfo.echoes;
    		req_bytes *= rInfo.svinfo.slices;
    	}
         
    	// arrayed elements
    	narray = rInfo.narray;
			if (!rInfo.svinfo.phase_compressed)
				narray /= rInfo.picInfo.npe;

			if (rInfo.svinfo.threeD) {
				if (!rInfo.svinfo.sliceenc_compressed)
					narray /= rInfo.svinfo.pe2_size;
			} else {
				if (!rInfo.svinfo.slice_compressed)
					narray /= rInfo.svinfo.slices;
			}
			
			if(narray > 1)
				req_bytes *= narray;
			
			req_kbytes = req_bytes / 1024;
			
            		  
        if(req_kbytes > free_kbytes)
		{
			Werrprintf("recon_all : Insufficient disk space available to save images!!");
			return(1);
		}
    	
                    
                    
    return(0);
    }
