/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */

/***********************************************************************
EPI sequence
************************************************************************/

#include <standard.h>
#ifndef DPS
#include <sys/time.h>
#endif
#include "sgl.c"

/* Additional GRADIENT/RF structures for Standard Gradient Echo */
RF_PULSE_T sge_rf;
READOUT_GRADIENT_T sgero_grad;
PHASE_ENCODE_GRADIENT_T sgepe_grad;
REFOCUS_GRADIENT_T sgeror_grad;
GENERIC_GRADIENT_T sgespoil_grad;

GENERIC_GRADIENT_T tmcrush_grad,tmspoil_grad;

/* GRADIENT kernel nodes, summary and info */
struct SGL_GRAD_NODE_T *epipk=NULL,*epirk=NULL,*sgek=NULL;
struct SGL_KERNEL_SUMMARY_T *epipkSummary=NULL,*epirkSummary=NULL;
SGL_KERNEL_INFO_T epipR,epipP,epipS,epirR,epirP,epirS,sgeR,sgeP,sgeS;

/* External storage for variables that are likely only calculated for ix=1 */
double *epipeamp; 
char   **epipename;
double *flipscale;
double epipTime,epirTime,delayToAcq;
double blipm0,perm0;
double sgegTime,sgedToAcq,sgedFromAcq,sgetr_delay;

struct timeval tp;
double time1,time2,time3,time4,time5;
int    rtn;

void pulsesequence()
{
  /* Internal variable declarations */
  double  freq90[MAXNSLICE],freq180[MAXNSLICE];
  double  dw,te_delay,tr_delay,*tc_delay,tcmax=0.0,tc0=0.0,xtime;
  double  tau1=0.0,tau2=0.0,del1=0.0,del2=0.0,thk2fact,minDELTA=0.0,maxDELTA,grora;
  double  episw,*si,delacq,samplem0,rampm0,skipm0,adjm0,rstadj,rsm0adj,tgrid;
  double  lread,*peamp,roamp,roramp,sign=1.0,ropad,readstretch;
  double  segtr,segtr_delay=0.0,*Mz,*Mxy,*flip,csegt1;
  double  volumes,nrefs,nsges;
  char    segtc[MAXSTR],altread[MAXSTR],cseg[MAXSTR],rseg[MAXSTR],name[MAXSTR],platnpmin[MAXSTR];
  char    pescheme[MAXSTR];
  int     nread,nphase,nnav,fulletl,maxkzero,ntraces,seg0;
  int     samplenp,rampnp,platnp,minplatnp,dwint,dwmod,oversample,delint;
  int     ncseg=1,autoscale,SCALEFLIP=FALSE,ixone,tstartup,EPIPK=FALSE;
  int     GE,SE,DSE,SESTE,DW,IR,RS,EPI,TC,ALT,CSEG,RSEG,ASL,VS,CPE;
  int     shape90,shape180=0;
  int     i,j,k,l;

  /* Diffusion variables */
  double Gss;                // "gdiff" for slice/slice refocus
  double dgss,dcrush,dgss2;  // "delta" for slice/slice refocus, SE refocus pulse crusher and slice select
  double Dgss,Dcrush,Dgss2;  // "DELTA" for slice/slice refocus, SE refocus pulse crusher and slice select
  double dtmcrush,Dtmcrush;  // "delta" and "DELTA" for stimulated echo crusher

  /* Spin-echo variables */
  char   p2flipadjust[MAXSTR];
  double p2flipmult;

  /* Double Spin-echo variables */
  double dsete,dsetemin=0.0,del3=0.0;
  char   dblspinecho[MAXSTR],mindsete[MAXSTR];

  /* Spin-Stimulated echo variables */
  double sete,stete,setemin,stetemin,ttmspoil,gtmspoil,ste_delay=0.0,tm_delay=0.0;
  double ttmcrush,gtmcrush,gtmcrushr=0.0,gtmcrushs=0.0;
  char   spinstimecho[MAXSTR],minsete[MAXSTR],minstete[MAXSTR],exrf[MAXSTR];

  /* Reference standard gradient echo variables */
  double  sgesw,sgedw,*sgesi,sgete,sgetr,flipsge,sgedacq;
  char    minsgete[MAXSTR],minsgetr[MAXSTR],sgecentric[MAXSTR];
  int     *petab,inv;
  int     SGE;

  /* Real-time variables used in this sequence */
  int  vms_slices = v1;   /* Number of slices */
  int  vms_ctr    = v2;   /* Slice loop counter */
  int  vpe_steps  = v3;   /* Actual number of steps through peloop */
  int  vpe_ctr    = v4;   /* PE loop counter */
  int  vpe_mult   = v5;   /* PE multiplier, ranges from -PE/2 to PE/2 */
  int  vss        = v6;   /* Compressed steady-states */
  int  vssc       = v7;   /* Compressed steady-states */
  int  vtrimage   = v8;   /* Counts down from nt, trimage delay when 0 */
  int  vacquire   = v9;   /* Argument for setacqvar, to skip steady state acquires */
  int  vtrigblock = v10;  /* Number of slices per trigger block */
  int  vtraces    = v11;  /* Number of traces per 'fid' */
  int  vtrace_ctr = v12;  /* Trace counter */
  int  vnphase    = v13;  /* Requested number of PE steps */
  int  vphase90   = v14;  /* Phase of 2nd two SESTE 90 pulses */
  int  vphase180  = v15;  /* Phase of SE 180 pulse */

  if (ix==1) ixone=TRUE; else ixone=FALSE;

tstartup=(int)getval("tstartup");

if (tstartup) {
  if (ixone) {
    rtn=gettimeofday(&tp, NULL);
    time1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    time3=time1;
  }
  if (ix<10) {
    rtn=gettimeofday(&tp, NULL);
    time2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    if (ixone) fprintf(stdout,"%s: prep %d took %f ms\n",seqfil,ix-1,1000*(time2-time1));
    else fprintf(stdout,"%s: prep %d took %f ms\n",seqfil,ix-1,1000*(time2-time3));
    time3=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  }
  if (ix==1000) {
    rtn=gettimeofday(&tp, NULL);
    time2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    fprintf(stdout,"%s: 1000 preps took %f secs\n",seqfil,time2-time1);
  }
}

  /* Enable variable rate sampling */
  setacqmode(WACQ|NZ);

  /* Initialize paramaters */
  get_parameters();
  if(FP_EQ(trampfixed,0.0)) euler_test(); /* increase ramp time by sqrt(3) if oblique */

  nread=(int)getval("nread");          /* Number of readout points */
  nphase=(int)getval("nphase");        /* Number of phase encode points */
  nnav=(int)getval("nnav");            /* Number of navigator echoes */
  episw=getval("episw");               /* EPI spectral width */
  getstr("platnpmin",platnpmin);       /* Flag to set minimum # readout plateau points in ramp sampling */
  minplatnp=(int)getval("minplatnp");  /* Minimum # points the readout plateau must have in ramp sampling */
  rstadj=getval("rstadj");             /* Ramp sampling timing adjustment */
  rsm0adj=getval("rsm0adj");           /* Ramp sampling moment adjustment */
  tgrid=getval("tgrid");               /* Timing grid, default is 50.0 ns */
  ropad=getval("ropad");               /* Pad time for readout gradient when no ramp sampling */
  readstretch=getval("readstretch");   /* Number of pixels to stretch/compress image in readout */
  grora=getval("grora");               /* Tweak of refocus moment to move zipper up/down (shouldn't be required) */
  volumes=getval("volumes");           /* Number of EPI volumes */
  nrefs=getval("nrefs");               /* Number of EPI reference volumes */
  nsges=getval("nsges");               /* Number of standard gradient echoes */
  getstr("segtc",segtc);               /* Flag for T2* time correction in segmented acquisitions */
  getstr("altread",altread);           /* Alternate read gradient on alternate segments (only nseg odd and >1) */
  getstr("cseg",cseg);                 /* Flag for compressed segments w.r.t. imaging slices */
  segtr=getval("segtr");               /* Compressed segment TR */
  autoscale=(int)getval("autoscale");  /* Flag to automatically scale the flips for compressed segments */
  csegt1=getval("csegt1");             /* The T1 to use in automatic scaling of flips for compressed segments */
  getstr("rseg",rseg);                 /* Flag to reverse compressed reference segments */
  getstr("pescheme",pescheme);         /* Phase encode order scheme, 'l' for linear, 'c' for centric-out (even # shots only) */

  /* Spin-echo parameters */
  thk2fact=getval("thk2fact");         /* Thickness factor for refocusing pulse */
  getstr("p2flipadjust",p2flipadjust); /* Flag to set double spin echo */
  p2flipmult=getval("p2flipmult");     /* Multiplier for refocusing pulses */

  /* Double Spin-echo parameters */
  getstr("dblspinecho",dblspinecho);   /* Flag to set double spin echo */
  dsete=getval("dsete");               /* Double spin echo TE1, first echo time */
  getstr("mindsete",mindsete);         /* Flag for minimum double spin echo TE1 */

  /* Spin-Stimulated echo parameters */
  getstr("spinstimecho",spinstimecho); /* Flag to set Spin-Stimulated echo */
  sete=getval("sete");                 /* sete = echo time for spin echo (te = sete+stete) */
  stete=getval("stete");               /* stete = echo time for stimulated echo (te = sete+stete) */
  getstr("minsete",minsete);           /* Flag for minimum echo time in spin echo */
  getstr("minstete",minstete);         /* Flag for minimum echo time in stimulated echo */
  getstr("exrf",exrf);                 /* Flag for excitation RF on ('y') or off ('n') */
  ttmcrush=getval("ttmcrush");         /* Crusher duration about mixing time */
  gtmcrush=getval("gtmcrush");         /* Crusher amplitude about mixing time */
  ttmspoil=getval("ttmspoil");         /* Mixing time spoil duration */
  gtmspoil=getval("gtmspoil");         /* Mixing time spoil amplitude */

  /* Reference standard gradient echo parameters */
  sgesw=getval("sgesw");               /* Standard gradient echo sw */
  sgete=getval("sgete");               /* Standard gradient echo te */
  sgetr=getval("sgetr");               /* Standard gradient echo tr */
  flipsge=getval("flipsge");           /* Standard gradient echo flip */
  getstr("minsgete",minsgete);         /* Flag for minimum standard gradient echo te */
  getstr("minsgetr",minsgetr);         /* Flag for minimum standard gradient echo tr */
  getstr("sgecentric",sgecentric);     /* Flag for standard gradient echo centric-out phase encode */

  GE    = (spinecho[0] == 'n');        /* EPI Gradient Echo */
  SE    = (spinecho[0] == 'y');        /* EPI Spin Echo */
  DSE   = (dblspinecho[0] == 'y');     /* EPI Double Spin Echo ... */
  if (!SE) DSE=FALSE;                  /* ... only if Spin Echo */
  SESTE = (spinstimecho[0] == 'y');    /* EPI Spin-Stimulated Echo ... */
  if (SESTE) GE=FALSE;                 /* ... is not Gradient Echo */
  DW    = (diff[0] == 'y');            /* Diffusion */
  IR    = (ir[0] == 'y');              /* Inversion Recovery */
  RS    = (rampsamp[0] == 'y');        /* Ramp sampling */
  TC    = (segtc[0] == 'y');           /* Segement time correction ... */
  if (nseg<2) TC=FALSE;                /* ... only if there is more than one segment */
  ALT   = (altread[0] == 'y');         /* Alternate read gradient on alternate segments ... */
  if (nseg<2) ALT=FALSE;               /* ... only if there is more than one segment ... */
  if ((int)nseg%2 == 0) ALT=FALSE;     /* ... and an odd # segments */
  CSEG  = (cseg[0] == 'y');            /* Compressed segments ... */
  if (SE || nseg<2) CSEG=FALSE;        /* ... not if spin echo or one segment */
  RSEG  = (rseg[0] == 'y');            /* Reverse compressed reference segments */
  ASL   = (asl[0] == 'y');             /* Arterial Spin Labelling */
  VS    = (vascsup[0] == 'y' && ASL);  /* Vascular Suppression */
  CPE   = (pescheme[0] == 'c');        /* Centric-out phase encoding ... */
  if ((int)nseg%2 == 1) CPE=FALSE;     /* ... for an even # segments only */

  switch ((int)image) {
    case -3: /* Reference standard gradient echo scan */
      EPI=FALSE; SGE=TRUE;
      break;
    default: /* Normal EPI scan */
      EPI=TRUE; SGE=FALSE;
      break;
  }

if (tstartup) {
  if (ix<10) {
    rtn=gettimeofday(&tp, NULL);
    time4=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    fprintf(stdout,"      Parameter read took %f ms\n",1000*(time4-time3));
  }
}

  /* nread must be a multiple of 4 and nphase a multiple of 2 */
  if (nread%4) { nread +=(4-nread%4); putvalue("nread",nread); }
  if (nphase%2) { nphase++; putvalue("nphase",nphase); }

  /* Check nseg, kzero, nnav and set echo train length */
  if (nseg<1) { nseg=1; putvalue("nseg",nseg); }
  if (nseg>nphase) { nseg=nphase; putvalue("nseg",nseg); }
  fulletl=nphase/(int)nseg;
  if (nphase%(int)nseg) fulletl++;
  maxkzero=nphase/2/(int)nseg;
  if (nphase/2%(int)nseg) maxkzero++;
  if (kzero<1) { kzero=1; putvalue("kzero",kzero); }
  if (kzero>maxkzero) { kzero=maxkzero; putvalue("kzero",kzero); }
  if (nnav<0) { nnav=0; putvalue("nnav",nnav); }
  if (CPE) etl=fulletl; /* For centric-out phase encoding the full echo train is sampled */
  else etl=fulletl-maxkzero+kzero;
  putvalue("etl",etl);  

  /* Calculate dwell time and spectral width for EPI */
  dwint=(int)floor(2.5e+6/episw);                      /* requested dw in units of 0.4 us */
  if (2.5e+6/episw-(double)dwint >=0.5) dwint++;       /* correct for requested sw roundoff */
  if (dwint<1) dwint=1;                                /* maximum sw is 2.5 MHz (dw = 0.4 us) */
  oversample=1;                                        /* set no oversample as default */
  dwmod=1;                                             /* default dw is multiple of 0.4 us */
  if (pro != 0.0) {                                    /* oversample if readout FOV is offset */
    oversample=2;                                      /* set for oversample */
    dwmod *=2;                                         /* for oversample dw must be a multiple of 0.8 us */
  }
  if (dwint>25*oversample) dwmod *=2;                  /* for sw<100kHz we need an additional x2 (why??) */
  while (dwint%dwmod != 0) dwint++;                    /* set dwint to a suitable multiple of 0.4 us */
  episw=2.5e+6/dwint;                                  /* calculate suitable sw */
  dw=dwint*0.4e-6/oversample;                          /* dwell time */
  sw=episw*oversample;                                 /* oversampled sw */
  putvalue("episw",episw);                             /* return value to parameter set */
  putvalue("oversample",oversample);                   /* return value to parameter set */
  putvalue("sw",sw);                                   /* return value to parameter set */

  /* Calculate dwell time and spectral width for standard gradient echo */
  dwint=(int)floor(2.5e+6/sgesw);                      /* requested dw in units of 0.4 us */
  if (2.5e+6/sgesw-(double)dwint >=0.5) dwint++;       /* correct for requested sw roundoff */
  if ((oversample>1) && (dwint%2)) dwint++;            /* if oversampling make dwint a multiple of 0.8 us */
  sgedw=dwint*0.4e-6/oversample;                       /* standard gradient echo dwell time */
  sgesw=2.5e+6/dwint;                                  /* standard gradient echo sweep width */
  if (FP_GT(sgesw,episw)) sgesw=episw;                 /* sweep width can't be greater than for EPI */
  putvalue("sgesw",sgesw);                             /* return value to parameter set */

  /* Calculate number of samples and set defaults for linear sampling */
  samplenp=oversample*nread/2;                         /* # samples in each readout lobe */
  platnp=samplenp;                                     /* # samples on the readout plateau */
  rampnp=0;                                            /* # samples on each readout ramp */
  if ((si=(double *)malloc(samplenp*sizeof(double))) == NULL) nomem(); /* allocate for sampling intervals */

  /* Calculate readout gradient */
  lread=lro*nread/(nread+2*readstretch);               /* allow readout FOV to be scaled */
  init_readout(&ro_grad,"ro",lread,nread,episw);       /* initialize readout gradient structure */
  ro_grad.pad1=ropad; ro_grad.pad2=ropad;              /* pad the readout gradient according to ropad */
  calc_readout(&ro_grad,NOWRITE,"gro","","");          /* calculate gradient */
  for (i=0;i<samplenp;i++) si[i]=dw;                   /* sampling interval for linear sampling */
  delacq=0.5*(ro_grad.duration-samplenp*dw);           /* from start of readout gradient to acquisition */

  /* Allow ramp sampling for EPI */
  if (RS) {                                            /* ramp sampling */
    samplem0=dw*oversample*ro_grad.roamp;              /* the moment of each sample without oversampling */
    rampm0=0.5*ro_grad.tramp*ro_grad.roamp;            /* the moment of the ramp */
    rampnp=(int)floor(rampm0/samplem0);                /* max # non linear samples we can put on ramp */
    if (platnpmin[0] == 'y') minplatnp=2;              /* min # plateau points if min flag is selected */
    /* Shorten plateau to include ramp sampling equally from ramp up and ramp down */
    platnp=nread/2;
    while ((platnp+2*rampnp > nread/2) && (platnp>minplatnp)) {
      platnp -=2;
    }
    putvalue("minplatnp",platnp);                      /* return min (or selected) # plateau points */
    if (platnp+2*rampnp == nread/2) putCmd("platnpmin = 'y'"); /* if using the min # plateau points check the min box */
    if (platnp == nread/2) {
      platnp *=oversample;                             /* if there are no ramp samples set platnp for oversampling */
      rampnp=0;                                        /* there are no ramp samples */
    } else {                                           /* if there are ramp samples calculate the timings */
      init_readout(&ro_grad,"ro",lread,2*platnp,episw); /* reinitialize readout gradient structure */
      calc_readout(&ro_grad,NOWRITE,"gro","","");      /* recalculate gradient */
      platnp *=oversample;                             /* set platnp for oversampling */
      rampnp=samplenp-platnp;                          /* actual # samples on the ramps */
      rampnp/=2;                                       /* actual # samples on a single ramp */
      samplem0 /=oversample;                           /* correct sample moment for oversampling */
      skipm0=0.5*(ro_grad.m0-samplem0*samplenp);       /* readout moment to skip before sampling */
      if (skipm0<0.0) skipm0=0.0;                      /* small -ve imprecision for dw = 2us without middle of first sample */
      delacq=sqrt(2*skipm0/ro_grad.slewRate);          /* delay to start of first sample for an ideal readout gradient */
      delacq+=rstadj;                                  /* allow the start time of sampling to be tweaked by rstadj */
      if (delacq<0.0) {                                /* it can't be less than 0.0 */
        rstadj -=delacq; putvalue("rstadj",rstadj);    /* reset rstadj to minimum */
        delacq=0.0;   
      }
      if (delacq>ro_grad.tramp) delacq=ro_grad.tramp;  /* it can't be more than the readout ramp */
      adjm0=(skipm0-delacq*delacq*ro_grad.slewRate/2)/rampnp; /* m0 adjustment to preserve the # ramp points */
      skipm0=delacq*delacq*ro_grad.slewRate/2;         /* adjust readout moment to skip before sampling */
      adjm0 += rsm0adj*samplem0;                       /* allow m0 adjustment of ramp points */
      if (adjm0<-samplem0) adjm0=-samplem0;            /* it can't be adjusted down by more than a sample m0 */
      for (i=0;i<samplenp/2;i++) {
        skipm0+=samplem0+adjm0;                        /* include moment to next sample */
        si[i]=sqrt(2*skipm0/ro_grad.slewRate)-delacq;  /* time between samples */
        delacq+=si[i];                                 /* adjust delay from start to next sample */
        if (delacq>ro_grad.tramp) {                    /* recalculate if we have reached the plateau */
          delacq-=si[i];                               /* reset the delay */
          si[i]=ro_grad.tramp-delacq;                  /* add time to end of ramp */
          si[i]+=(samplem0+adjm0-0.5*ro_grad.tramp*ro_grad.roamp+delacq*delacq*ro_grad.slewRate/2)/ro_grad.roamp;
          break;
        }
      }
      /* Grid the delays */
      if (tgrid == 0.0) tgrid = 50.0;                  /* default timing grid is 50 ns */
      tgrid=1e9/tgrid;                                 /* scalar to put times on integer grid */
      for (i=0;i<samplenp/2;i++) {                     /* granulate timings to tgrid */
        delint=(int)floor(tgrid*si[i]);
        if (tgrid*si[i]-(double)delint >= 0.5) delint++;
        si[i]=delint/tgrid;
        if (si[i]<dw) si[i]=dw;                        /* trap for any -ve delays */
      }
      for (i=0;i<samplenp/2;i++) si[samplenp-1-i]=si[i]; /* the ramp down is the same */
      delacq=0;
      for (i=0;i<samplenp;i++) delacq +=si[i];         /* sum the sampling intervals */
      delacq=0.5*(ro_grad.duration-delacq);            /* figure the delay to start of acquisition */
      if (delacq<0.0) {                                /* -ve delay due to granulation or m0 adjustment of ramp points */
        if (delacq>-1e-6 && si[0]>dw-delacq) {         /* shouldn't be out by more than 1 us for normal granulation */
          si[0]+=delacq; si[samplenp-1]+=delacq;       /* adjust first and last samples */
        } else {
          if (si[0]>dw-2*delacq) {
            si[samplenp-1]+=2*delacq;                  /* adjust just the last sample */
            warn_message("%s warning: Moment tweak of ramp points is severe!",seqfil);
          } else abort_message("%s error: Moment tweak of ramp points is too severe!",seqfil);
        }
        delacq=0.0;                                    /* set delay to zero */
      }
    } 
  }

  /* Set sampling intervals */
  for (i=0;i<samplenp;i++) si[i] -=dw;                 /* subtract dw from the sampling intervals */
  putvalue("platnp",platnp/oversample);                /* # samples on the readout plateau */
  putvalue("rampnp",rampnp/oversample);                /* # samples on each readout ramp */
  putvalue("delacq",delacq);                           /* delay from start of readout to first sample */

  /* Calculate np for the acquisition */
  ntraces=nnav+1+(int)etl;
  np=2*samplenp*ntraces;
  putvalue("np",np);

  /* EPI echo spacing */
  esp = ro_grad.duration; 
  putvalue("esp",esp);

  /* Calculate readout dephase gradient */
  init_readout_refocus(&ror_grad,"ror");               /* readout dephase */
  ro_grad.m0ref*=grora; /* allow tweak of refocus moment to move zipper up/down (shouldn't be required) */
  calc_readout_refocus(&ror_grad,&ro_grad,WRITE,"gror");
  roramp=ror_grad.amp;

  /* RF Power & Bandwidth Calculations */
  shape_rf(&p1_rf,"p1",p1pat,p1,flip1,rof1,rof1);
  calc_rf(&p1_rf,"tpwr1","tpwr1f");
  shape_rf(&sge_rf,"sge",p1pat,p1,flipsge,rof1,rof1);
  calc_rf(&sge_rf,"sgetpwr","sgetpwrf");  

  /* Calculate slice select gradient */
  init_slice(&ss_grad,"ss",thk);                       /* slice select */
  init_slice_refocus(&ssr_grad,"ssr");                 /* slice refocus */
  calc_slice(&ss_grad,&p1_rf,WRITE,"gss");
  calc_slice_refocus(&ssr_grad,&ss_grad,WRITE,"gssr");

  /* Calculate phase encode gradient */
  init_phase(&per_grad,"per",lpe,nphase);              /* phase encode offset */
  calc_phase(&per_grad,NOWRITE,"","");                 /* calculate phase encode as usual */

  /* Calculate phase encode blip to be a multiple of 8 usec */ 
  if (ixone) blipm0=per_grad.m0*2/nphase; /* only do for ix=1 as we may not recalculate per_grad */
  init_dephase(&pe_grad,"pe");
  if (CPE) calc_dephase(&pe_grad,NOWRITE,blipm0*nseg/2,"",""); /* Centric-out phase encoding */
  else calc_dephase(&pe_grad,NOWRITE,blipm0*nseg,"","");
  pe_grad.duration=granularity(pe_grad.duration,GRADIENT_RES*2);
  pe_grad.calcFlag=AMPLITUDE_FROM_MOMENT_DURATION;
  if (CPE) calc_dephase(&pe_grad,NOWRITE,blipm0*nseg/2,"",""); /* Centric-out phase encoding */
  else calc_dephase(&pe_grad,NOWRITE,blipm0*nseg,"gpe","tpe");
  /* Adjust phase encode offset for kzero */
  if (CPE) { /* Centric-out phase encoding */
    if (ixone) per_grad.m0 = blipm0*nseg/2;      /* only for ix=1 */
    calc_phase(&per_grad,NOWRITE,"gper","tper"); /* gradient blip for max number of k-space lines to be stepped */
    per_grad.calcFlag=AMPLITUDE_FROM_MOMENT_DURATION_RAMP; /* use the calculated duration */
    per_grad.m0 = blipm0;                                  /* supply moment */
    calc_phase(&per_grad,NOWRITE,"","");         /* gradient blip for a single line of k-space */
  } else {
    if (ixone) per_grad.m0 *= (1-(maxkzero-kzero)*nseg*2/nphase); /* only for ix=1 */
    calc_phase(&per_grad,NOWRITE,"gper","tper");
    per_grad.calcFlag=AMPLITUDE_FROM_MOMENT_DURATION_RAMP;
    if (ixone) perm0 = per_grad.m0; /* only for ix=1 */
  }

if (tstartup) {
  if (ix<10) {
    rtn=gettimeofday(&tp, NULL);
    time5=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    fprintf(stdout,"      EPI Grad calcs took %f ms\n",1000*(time5-time4));
    time4=time5;
  }
}

  /* Intra-TE gradients */
  if (SE || SESTE) { /* EPI Spin Echo or Spin-Stimulated Echo */
    shape_rf(&p2_rf,"p2",p2pat,p2,flip2,rof1,rof1);
    if (p2flipadjust[0] == 'y') p2_rf.flipmult=p2flipmult;
    calc_rf(&p2_rf,"tpwr2","tpwr2f");
    init_slice(&ss2_grad,"ss2",thk*thk2fact);
    calc_slice(&ss2_grad,&p2_rf,WRITE,"gss2");
    init_generic(&crush_grad,"crush",gcrush,tcrush);
    calc_generic(&crush_grad,WRITE,"gcrush","tcrush");
    offsetlist(pss,ss2_grad.ssamp,0,freq180,ns,'c');
    shape180 = shapelist(p2_rf.pulseName,ss2_grad.rfDuration,freq180,ns,ss2_grad.rfFraction,'c');
  }

  if (SESTE) { /* EPI Spin-Stimulated Echo */
    /* Combine TM crusher with slice refocus */
    init_dephase(&tmcrush_grad,"tmcrush");
    if (FP_GTE(gtmcrush*ttmcrush,ssr_grad.m0)) {
      calc_dephase(&tmcrush_grad,WRITE,gtmcrush*ttmcrush,"","");
      gtmcrushr= tmcrush_grad.amp;
      gtmcrushs= tmcrush_grad.amp*(ssr_grad.m0/tmcrush_grad.m0-1);
    } else { 
      calc_dephase(&tmcrush_grad,WRITE,ssr_grad.m0,"","");
      gtmcrushr= tmcrush_grad.amp*gtmcrush*ttmcrush/ssr_grad.m0;
      gtmcrushs= tmcrush_grad.amp*(1-gtmcrush*ttmcrush/ssr_grad.m0);
    }
    if (ttmspoil>0) {
      init_generic(&tmspoil_grad,"tmspoil",gtmspoil,ttmspoil);
      calc_generic(&tmspoil_grad,WRITE,"","");
    }
  }
  if (DW) init_diffusion(&diffusion,&diff_grad,"diff",gdiff,tdelta); /* Diffusion */

if (tstartup) {
  if (ix<10) {
    rtn=gettimeofday(&tp, NULL);
    time5=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    fprintf(stdout,"      Other intra-TE grad calcs took %f ms\n",1000*(time5-time4));
    time4=time5;
  }
}

  /* Segment time correction */
  tc_delay=0;
  if (TC) {                                            
    tc_delay = (double *)malloc(nseg*sizeof(double));  /* Time correction delays */
    if (CPE) { /* Centric-out phase encoding */
      for (i=0;i<nseg;i++) {
        j=(i+1)/2;
        tc_delay[i]=granularity(2*j*ro_grad.duration/nseg,GRADIENT_RES);
      }
      seg0=0;                                          /* The segment kzero is in */
    } else {
      for (i=0;i<nseg;i++) tc_delay[i]=granularity(i*ro_grad.duration/nseg,GRADIENT_RES);
      seg0=(nphase/2-1)%(int)nseg;                     /* The segment kzero is in */
    }
    tcmax=tc_delay[(int)nseg-1];                       /* Maximum time correction delay */
    tc0=tc_delay[seg0];                                /* Time correction for the segment kzero is in */
  }

  /* If possible include all EPI preparation gradients in a kernel for minimum timing */
  /* Set EPIPK flag in this instances (basic GE EPI) */
  if (GE && !(DW || VS)) {
    EPIPK = TRUE;
    xtime = ss_grad.tramp+ssr_grad.duration;
    xtime = xtime>ror_grad.duration ? xtime : ror_grad.duration;
    xtime -= ss_grad.tramp+ssr_grad.duration;
    start_kernel(&epipk); /* start epi prep kernel */
    add_gradient((void *)&ss_grad,"slice",SLICE,START_TIME,"",0.0,PRESERVE);
    add_gradient((void *)&ssr_grad,"sliceReph",SLICE,BEHIND_LAST,"slice",0.0,INVERT);
    add_gradient((void *)&ror_grad,"readDeph",READ,SAME_END,"sliceReph",xtime,INVERT);
  }

if (tstartup) {
  if (ix<10) {
    rtn=gettimeofday(&tp, NULL);
    time5=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    fprintf(stdout,"      EPI preparation kernel took %f ms\n",1000*(time5-time4));
    time4=time5;
  }
}

  /* Min TE and TE delays */
  if (CPE) kzero=1;  /* Centric-out phase encoding */

  if (GE) { /* gradient echo EPI */
    if (EPIPK) { /* EPI preparation kernel (basic gradient echo EPI) */
      temin = get_timing(FROM_RF_CENTER_OF,"slice",TO_END_OF,"readDeph")+tc0+(nnav+1+kzero)*esp-esp/2;
    } else {
      temin = ss_grad.rfCenterBack+ssr_grad.duration+ror_grad.duration+tc0+(nnav+1+kzero)*esp-esp/2;
      if (VS) temin += vsTime;
      if (DW) {
        temin += GRADIENT_RES;                         /* Minimum delay time */
        taudiff=0.0;                                   /* The duration of events between diffusion gradients */
        set_diffusion(&diffusion,taudiff,tDELTA,te,minte[0]);
        calc_diffTime(&diffusion,&temin);
      }
    }
    if (minte[0] == 'y') {
      te = temin;
      putvalue("te",te);
    }
    if (FP_LT(te,temin)) {
      abort_message("TE too short, minimum TE= %.3f ms\n",temin*1000);
    }
    if (EPIPK) { /* EPI preparation kernel (basic gradient echo EPI) */
      te_delay = granularity(te-temin-tc0,GRADIENT_RES);
      if (te_delay>0.0) change_timing("readDeph",te_delay);
    } else {
      if (VS && !DW) del1 = te-temin-tc0;
      else del1=0.0;
    }
  }

  /* Write the EPI preparation kernel */
  if (EPIPK) { /* EPI preparation kernel (basic gradient echo EPI and spin-stimulated EPI) */
    delayToAcq = get_timing(FROM_RF_PULSE_OF,"slice",TO_END_OF,"readDeph");
    if (evaluateSummary(epipk,&epipkSummary)) {        /* check if the preparation kernel has been modified */
      set_comp_info(&epipR,"epipro");                  /* set read shape name */
      set_comp_info(&epipP,"epippe");                  /* set PE shape name */
      set_comp_info(&epipS,"epipsl");                  /* set slice shape name */
      epipTime=write_comp_grads(&epipR,&epipP,&epipS); /* write the gradient shapes */
    }
  }

  if (SE) { /* spin echo EPI */
    te = granularity(te,2*GRADIENT_RES);
    tau1 = ss_grad.rfCenterBack+ssr_grad.duration+crush_grad.duration+ss2_grad.rfCenterFront+2*GRADIENT_RES;
    if (DSE) { /* double spin-echo */
      dsete = granularity(dsete,2*GRADIENT_RES);
      dsetemin = 2*(ss_grad.rfCenterBack+ssr_grad.duration+GRADIENT_RES+crush_grad.duration)+ss2_grad.duration;
      if (mindsete[0] == 'y') {
        dsete = dsetemin;
        putvalue("dsete",dsete);
      }
      if (FP_LT(dsete,dsetemin)) {
        abort_message("TE1 too short, minimum TE1= %.3f ms\n",dsetemin*1000);
      }
      del3 = granularity((dsete-dsetemin)/2.0,GRADIENT_RES); /* delay first half first echo TE1 */
      tau1 = crush_grad.duration+ss2_grad.rfCenterFront+GRADIENT_RES; /* tau1 for calculation of temin for second echo */
    } else {
      dsete=0.0; /* the double spin-echo (DSE) TE1 is 0.0 unless running DSE */
    }
    if (VS) tau1 += vsTime;
    tau2 = ss2_grad.rfCenterBack+crush_grad.duration+ror_grad.duration+tc0+(nnav+1+kzero)*esp-esp/2+GRADIENT_RES;
    temin = 2*MAX(tau1,tau2);
    if (DSE) { /* double spin-echo */
      /* Adjust tau1 to include the additional time that is available for diffusion encoding in second half of TE1 */
      tau1 -= ss_grad.rfCenterBack+ssr_grad.duration+del3+GRADIENT_RES;
    }
    if (DW) {
      taudiff=2*crush_grad.duration+ss2_grad.duration; /* The duration of events between diffusion gradients */
      set_diffusion(&diffusion,taudiff,tDELTA,te-dsete,minte[0]);
      set_diffusion_se(&diffusion,tau1,tau2);
      calc_diffTime(&diffusion,&temin);
    }
    if (DSE) temin += dsetemin+2*del3; /* for double spin-echo set temin appropriately */
    if (minte[0] == 'y') {
      te = temin;
      putvalue("te",te);
    }
    if (FP_LT(te,temin)) {
      abort_message("TE too short, minimum TE= %.3f ms\n",temin*1000);
    }
    if (DW) {
      del1=0.0; del2=0.0; 
    } else { 
      del1 = (te-dsete)/2.0-tau1+GRADIENT_RES;
      del2 = (te-dsete)/2.0-tau2+GRADIENT_RES; 
    }
  }

  if (SESTE) { /* spin-stimulated echo EPI */
    /* spin echo TE */
    sete = granularity(sete,2*GRADIENT_RES);
    tau1 = ss_grad.rfCenterBack+ssr_grad.duration+crush_grad.duration+ss2_grad.rfCenterFront+2*GRADIENT_RES;
    if (VS) tau1 += vsTime;
    tau2 = ss2_grad.rfCenterBack+crush_grad.duration;
    setemin = 2*MAX(tau1,tau2);
    if (DW) {
      taudiff=2*crush_grad.duration+ss2_grad.duration+GRADIENT_RES;         /* The duration of events between diffusion gradients */
      set_diffusion(&diffusion,taudiff,tDELTA,sete,minsete[0]);
      set_diffusion_se(&diffusion,tau1,tau2);
      calc_diffTime(&diffusion,&setemin);
    }
    if (minsete[0] == 'y') {
      sete = setemin;
      putvalue("sete",sete);
    }
    if (FP_LT(sete,setemin)) {
      abort_message("SE TE too short, minimum SE TE = %.3f ms\n",setemin*1000);
    }
    if (DW) { 
      del1=0.0; del2=0.0; 
    } else { 
      del1 = sete/2.0-tau1+GRADIENT_RES; 
      del2 = sete/2.0-tau2+GRADIENT_RES; 
    }
    /* stimulated echo TE */
    stete = granularity(stete,2*GRADIENT_RES);
    tau1 = tmcrush_grad.duration+ss_grad.rfCenterFront;
    tau2 = ss_grad.rfCenterBack+tmcrush_grad.duration+ror_grad.duration+tc0+(nnav+1+kzero)*esp-esp/2;
    stetemin = 2*MAX(tau1,tau2);
    if (minstete[0] == 'y') {
      stete = stetemin;
      putvalue("stete",stete);
    }
    if (FP_LT(stete,stetemin)) {
      abort_message("STE TE too short, minimum STE TE = %.3f ms\n",stetemin*1000);
    }
    putvalue("te",sete+stete);
    del1 = stete/2.0-tau2-tc0;
    ste_delay = stete/2.0-tau1;
  }

  /* spin-stimulated echo EPI mixing time */
  if (SESTE) {
    /* Min TM */  
    tm = granularity(tm,GRADIENT_RES);
    /* tmmin includes a GRADIENT_RES as this is minimum delay time */
    tmmin  = ss_grad.duration+GRADIENT_RES;  /* one half of ss_grad for each RF pulse */
    if (ttmspoil>0) tmmin += tmspoil_grad.duration;
    /* TM delay */
    if (mintm[0] == 'y') {
      tm = tmmin;
      putvalue("tm",tm);
    }
    if (FP_LT(tm,tmmin)) {
      abort_message("TM too short, minimum TM = %.3f ms\n",tmmin*1000);
    }
    tm_delay = tm - tmmin + GRADIENT_RES;
  }

if (tstartup) {
  if (ix<10) {
    rtn=gettimeofday(&tp, NULL);
    time5=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    fprintf(stdout,"      EPI delays took %f ms\n",1000*(time5-time4));
    time4=time5;
  }
}

  /* Put all readout gradients in a kernel */
  if (ixone || sglarray) {
    start_kernel(&epirk); /* start epi read kernel */
    if (nnav>0) {
      add_gradient((void *)&ro_grad,"readNav",READ,START_TIME,"",0.0,PRESERVE);
      for (i=1;i<nnav;i++)
        add_gradient((void *)&ro_grad,"readNav",READ,BEHIND_LAST,"",0.0,i%2);
      add_gradient((void *)&ro_grad,"read",READ,BEHIND_LAST,"",0.0,i++%2);
    } else {
      i=0;
      add_gradient((void *)&ro_grad,"read",READ,START_TIME,"",0.0,i++%2);
    }
    if (!CPE) /* If not centric-out phase encoding */
      add_gradient((void *)&per_grad,"phaseDeph",PHASE,SAME_END,"read",0.0,PRESERVE);
    add_gradient((void *)&ro_grad,"read",READ,BEHIND_LAST,"",0.0,i++%2);
    for (j=1;j<etl;j++) {
      add_gradient((void *)&pe_grad,"phase",PHASE,BEHIND_LAST,"",-pe_grad.duration/2,INVERT); 
      add_gradient((void *)&ro_grad,"read",READ,BEHIND_LAST,"",-pe_grad.duration/2,i++%2);
    }
  }

if (tstartup) {
  if (ix<10) {
    rtn=gettimeofday(&tp, NULL);
    time5=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    fprintf(stdout,"      EPI read kernel took %f ms\n",1000*(time5-time4));
    time4=time5;

  }
}

  /* Write the composite gradients */ 
  peamp  = (double *)malloc(nseg*sizeof(double));        /* PE amplitudes for each shot */
  if (ixone) { /* first array element */
    epipeamp  = (double *)malloc(nseg*sizeof(double));   /* external storage of PE amplitudes */
    epipename = (char **)malloc(nseg*sizeof(char *));    /* external storage of PE names */
    for (i=0;i<nseg;i++) epipename[i] = (char *)malloc(20*sizeof(char));
  }
  for (i=0;i<nseg;i++) { /* loop over segments */
    if (ixone || sglarray) {
      if (CPE) { /* Centric-out phase encoding */
        if (i%2) add_gradient((void *)&per_grad,"phaseDeph",PHASE,SAME_END,"read",0.0,INVERT);
      } else {
        per_grad.m0 = perm0-(i+1)*blipm0;                /* set PE dephase moment for the shot */
        calc_phase(&per_grad,NOWRITE,"","");             /* calculate PE dephase */
      }
      sprintf(name,"%s%d","epirpe",i+1);                 /* set PE name for the shot */
      /* Used to check if the read kernel has been modified to see if shapes need writing
         but for centric-out phase encoding we sometimes use same PE shape and just invert it.
         The kernel summary is just the same, so for now just always write the shapes */
//    if (evaluateSummary(epirk,&epirkSummary)) {        /* check if the read kernel has been modified */
        set_comp_info(&epirR,"epirro");                  /* set read shape name */
        set_comp_info(&epirP,name);                      /* set PE shape name */
        set_comp_info(&epirS,"epirsl");                  /* set slice shape name */
        epirTime=write_comp_grads(&epirR,&epirP,&epirS); /* write the gradient shapes */
        epipeamp[i]=epirP.amp;                           /* store PE amplitude externally */
        if (CPE && i%2) epipeamp[i] *=-1;                /* For centric-out phase encoding the gradient is reversed for odd shots */
        strcpy(epipename[i],epirP.name);                 /* store PE name externally */
//    }
    }
    peamp[i]=epipeamp[i]; /* reset local PE amplitude (can get set to 0.0 for reference scans) */
  }
  roamp=epirR.amp; /* reset local read amplitude (can get reversed for reference scan) */

if (tstartup) {
  if (ix<10) {
    rtn=gettimeofday(&tp, NULL);
    time5=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    fprintf(stdout,"      Analyze/writing EPI read kernel took %f ms\n",1000*(time5-time4));
    time4=time5;
  }
}

  switch ((int)image) {
    case 1: /* Real image scan */
      break;
    case 0: /* Normal reference scan */
      for (i=0;i<nseg;i++) peamp[i]=0.0;
      break;
    case -1: /* Inverted image scan */
      roamp=-epirR.amp; roramp=-ror_grad.amp;
      break;
    case -2: /* Inverted reference scan */
      for (i=0;i<nseg;i++) peamp[i]=0.0;
      roamp=-epirR.amp; roramp=-ror_grad.amp;
      break;
   }

  /* Standard Gradient Echo */
  if ((sgesi=(double *)malloc(samplenp*sizeof(double))) == NULL) nomem(); /* allocate for sampling intervals */
  init_readout(&sgero_grad,"sgero",lro,nread,sgesw);     /* initialize readout gradient structure */
  calc_readout(&sgero_grad, NOWRITE,"","","");           /* calculate gradient */
  for (i=0;i<samplenp;i++) sgesi[i]=sgedw-dw;            /* sampling interval for linear sampling */
  sgedacq=0.5*(sgero_grad.duration-samplenp*sgedw);      /* from start of readout gradient to acquisition */
  init_readout_refocus(&sgeror_grad,"sgeror");           /* readout dephase */
  calc_readout_refocus(&sgeror_grad,&sgero_grad,NOWRITE,"");
  init_phase(&sgepe_grad,"sgepe",lpe,nphase);            /* phase encode */
  calc_phase(&sgepe_grad,NOWRITE,"","");                 /* calculate phase encode as usual */
  pe_steps=ntraces*nseg;                                 /* actual number of steps through peloop */

  /* Add gradient structures to a kernel */
  if (SGE || ixone) { 
    xtime = ss_grad.tramp+ssr_grad.duration;
    xtime = xtime>sgeror_grad.duration ? xtime : sgeror_grad.duration;
    xtime = xtime>sgepe_grad.duration ? xtime : sgepe_grad.duration;
    xtime -= ss_grad.tramp+ssr_grad.duration;
    start_kernel(&sgek); /* start sge kernel */
    add_gradient((void *)&ss_grad,"slice",SLICE,START_TIME,"",0.0,PRESERVE);
    add_gradient((void *)&ssr_grad,"sliceReph",SLICE,BEHIND_LAST,"slice",0.0,INVERT);
    add_gradient((void *)&sgeror_grad,"readDeph",READ,SAME_END,"sliceReph",xtime,INVERT);
    init_dephase(&sgespoil_grad,"sgespoil");
    calc_dephase(&sgespoil_grad,NOWRITE,sgedw*nread*oversample*sgero_grad.amp-sgero_grad.m0def,"","");
    xtime = sgero_grad.tramp;
    xtime = xtime<sgespoil_grad.tramp ? xtime : sgespoil_grad.tramp;
    add_gradient((void *)&sgepe_grad,"phase",PHASE,SAME_END,"readDeph",0.0,PRESERVE);
    add_gradient((void *)&sgero_grad,"read",READ,BEHIND,"readDeph",0.0,PRESERVE);
    add_gradient((void *)&sgepe_grad,"phaseRew",PHASE,BEHIND,"read",0.0,INVERT);
    add_gradient((void*)&sgespoil_grad,"spoil",READ,BEHIND,"read",-xtime,PRESERVE);
    /* Minimum TE */
    temin = get_timing(FROM_RF_CENTER_OF,"slice",TO_ECHO_OF,"read");
    if (minsgete[0] == 'y') {
      sgete = temin;
      putvalue("sgete",sgete);
    }
    if (FP_LT(sgete,temin)) {
      abort_message("Standard Gradient Echo TE too short, minimum TE= %.3f ms\n",temin*1000);   
    }
    /* Calculate delays */
    te_delay = granularity(sgete-temin,GRADIENT_RES);
    if (te_delay>0.0) change_timing("readDeph",te_delay);
    sgedToAcq = get_timing(FROM_RF_PULSE_OF,"slice",TO_START_OF,"read");
    sgedFromAcq = get_timing(FROM_END_OF,"read",TO_END_OF,"phaseRew");
    xtime = get_timing(FROM_END_OF,"read",TO_END_OF,"spoil");
    sgedFromAcq = sgedFromAcq>xtime ? sgedFromAcq : xtime;
    /* Write the composite gradients */ 
    set_comp_info(&sgeR,"sgeread");
    set_comp_info(&sgeP,"sgephase");
    set_comp_info(&sgeS,"sgeslice");
    sgegTime=write_comp_grads(&sgeR,&sgeP,&sgeS);
    /* Min TR */
    trmin = sgegTime + GRADIENT_RES; /* ensure that sgetr_delay is at least 4us */
    trmin *=ns;
    if (minsgetr[0] == 'y') {
      sgetr = trmin;
      putvalue("sgetr",sgetr);
    }
    if (FP_LT(sgetr,trmin)) {
      abort_message("Standard Gradient Echo TR too short, minimum TR = %.3f ms\n",trmin*1000);
    }
    sgetr_delay = granularity((sgetr-trmin)/ns,GRADIENT_RES);
  }

  /* Set phase encode table for standard gradient echo */
  if (SGE) { /* Standard gradient echo */
    if ((petab=(int *)malloc(nphase*sizeof(int))) == NULL) nomem();
    putCmd("sgepelist = 0");    /* Re-initialize sgepelist */
    if (sgecentric[0] == 'y') { /* centric-out phase encode */
      inv=1; petab[0]=0;
      for (i=1;i<nphase;i++) {
        inv = -inv;
        petab[i] = petab[i-1] + inv*i;
        putCmd("sgepelist[%d] = %d",i+1,petab[i]);
      }
    } else {
      for (i=0;i<nphase;i++) petab[i]=i-nphase/2;
    }
    settable(t1,nphase,petab);
  }

if (tstartup) {
  if (ix<10) {
    rtn=gettimeofday(&tp, NULL);
    time5=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    fprintf(stdout,"      To RF offsetlist/shapelist took took %f ms\n",1000*(time5-time4));
    time4=time5;
  }
}

  /* Set up frequency offset pulse shape list */   	
  offsetlist(pss,ss_grad.ssamp,0,freq90,ns,'c');
  shape90 = shapelist(p1_rf.pulseName,ss_grad.rfDuration,freq90,ns,ss_grad.rfFraction,'c');

if (tstartup) {
  if (ix<10) {
    rtn=gettimeofday(&tp, NULL);
    time5=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    fprintf(stdout,"      RF offsetlist/shapelist took %f ms\n",1000*(time5-time4));
    time4=time5;
  }
}

  /* Calculate spoiler */
  if (tspoil>0.0) {
    init_generic(&spoil_grad,"spoil",gspoil,tspoil);
    calc_generic(&spoil_grad,WRITE,"","");
  }

  /* Create optional prepulse events */
  if (sat[0] == 'y')  create_satbands();
  if (fsat[0] == 'y') create_fatsat();
  if (mt[0] == 'y')   create_mtc();
  if (ir[0] == 'y')   create_inversion_recovery();
  /* The ASL module can not be used with the IR module */
  /* The ASL module uses the IR module real time variables, real time variables v41,v42 and real time tables t57-t60 */
  if (asl[0] == 'y')  create_arterial_spin_label();

  /* Check nsblock, the number of slices blocked together
     (used for triggering and/or inversion recovery) */
  check_nsblock();

  /* Min TR */   	
  trmin = epirTime+tcmax+tspoil+3*GRADIENT_RES; /* ensure that tr_delay is at least GRADIENT_RES */
  if (EPIPK) trmin += epipTime;
  else trmin += ss_grad.duration+ssr_grad.duration+del1;
  if (SE) trmin += 2*crush_grad.duration+ss2_grad.duration+del2;
  if (DW) trmin += diffusion.Time;
  if (VS) trmin += vsTime;
  if (SESTE) trmin = ss_grad.duration+ssr_grad.duration+del1+epirTime+tcmax+tspoil+3*GRADIENT_RES;
  if (CSEG) {                     /* allow for compressed EPI segments ... */
    trmin -= GRADIENT_RES;        /* only one scope trigger delay for all compressed segments */
    if (segtr<=trmin) { segtr_delay=0.0; segtr=trmin; putvalue("segtr",trmin); } 
    else segtr_delay=segtr-trmin; /* calculate the segment delay */
    trmin += segtr_delay;         /* add the segment delay ... */
    trmin *= nseg;                /* ... multiply by the number of segments ... */
    trmin += GRADIENT_RES;        /* ... and add one scope trigger delay */
  }
  if (SESTE)         trmin += tm+stete/2+sete;
  if (sat[0] == 'y') trmin += satTime;
  if (mt[0] == 'y')  trmin += mtTime;
  if (fsat[0]=='y')  trmin += fsatTime;
  if (ticks > 0)     trmin += GRADIENT_RES;

  /* Adjust for all slices */
  trmin *= ns;

  /* Inversion recovery */
  if (IR) {
    /* tauti is the additional time beyond IR component to be included in ti */
    /* satTime, fsatTime and mtTime all included as those modules will be after IR */
    tauti = satTime + fsatTime + mtTime + GRADIENT_RES + ss_grad.rfCenterFront;
    /* calc_irTime checks ti and returns the time of all IR components */
    trmin += calc_irTime(tauti,trmin,mintr[0],tr,&trtype);
  }

  /* Arterial Spin Labelling */
  if (asl[0] == 'y') {
    /* The ASL module can not be used with the IR module */
    /* The ASL module uses the IR module real time variables, real time variables v41,v42 and real time tables t57-t60 */
    /* tauasl is the additional time beyond ASL component to be included in inflow time */
    /* satTime, fsatTime and mtTime all included as those modules will be after ASL */
    tauasl = satTime + fsatTime + mtTime + GRADIENT_RES + ss_grad.rfCenterFront;
    /* calc_aslTime checks inflow time and returns the time of all ASL components */
    trmin += calc_aslTime(tauasl,trmin,&trtype);
  }

  if (mintr[0] == 'y') {
    tr = trmin;
    putvalue("tr",tr);
  }
  if (FP_LT(tr,trmin)) {
    abort_message("TR too short, minimum TR = %.3f ms\n",trmin*1000);
  }

  /* Calculate tr delay */
  tr_delay = granularity((tr-trmin)/ns,GRADIENT_RES);

  /* For NIFTI slice duration */
  if (trtype) putvalue("niftitslice",trmin/ns);
  else putvalue("niftitslice",trmin/ns+tr_delay);

  sgl_error_check(sglerror);

  /* Return some gradient durations */
  putvalue("tror",ror_grad.duration);
  putvalue("tss",ss_grad.duration);
  putvalue("tssr",ssr_grad.duration);

  /* Calculate B values */
  if (ixone) {
    /* Calculate bvalues according to main diffusion gradients */
    calc_bvalues(&diffusion,"dro","dpe","dsl");
    /* Add components from additional diffusion encoding imaging gradients peculiar to this sequence */
    /* Initialize variables */
    dgss = 0.5*(ss_grad.rfCenterBack+ssr_grad.duration);
    Gss = ss_grad.m0ref/dgss; Dgss = dgss;
    dcrush=0.0; Dcrush=0.0; dgss2=0.0; Dgss2=0.0; dtmcrush=0.0; Dtmcrush=0.0;
    if (SE || SESTE) {
      dgss2 = ss2_grad.duration/2; Dgss2 = dgss2;
      dcrush = crush_grad.duration-crush_grad.tramp; Dcrush = crush_grad.duration+ss2_grad.duration;
      if (SESTE) {
        dtmcrush = tmcrush_grad.duration-tmcrush_grad.tramp; 
        Dtmcrush = tmcrush_grad.duration+tm+ss_grad.duration;
        /* For compressed segments Dtmcrush depends on the segment that k=0 is in */
        if (CSEG) Dtmcrush += ((nphase/2-1)%(int)nseg)*segtr;
      }
    }
    /* Calculate the additional values */
    for (i = 0; i < diffusion.nbval; i++)  {
      /* For all sequence modes we have additional readout and slice encoding */
      diffusion.bro[i] += (nnav+1+kzero)*bval(ro_grad.roamp,esp/2.0,esp/2.0);
      diffusion.bsl[i] += bval(Gss,dgss,Dgss);
      if (SE || SESTE) {
        /* set droval, dpeval and dslval */
        set_dvalues(&diffusion,&droval,&dpeval,&dslval,i);
        /* Readout */
        diffusion.bro[i] += bval(-gcrush,dcrush,Dcrush);
        diffusion.bro[i] += bval_nested(gdiff*droval,tdelta,tDELTA,-gcrush,dcrush,Dcrush);
        /* Slice */
        diffusion.bsl[i] += bval(gcrush,dcrush,Dcrush);
        diffusion.bsl[i] += bval(ss2_grad.ssamp,dgss2,Dgss2);
        diffusion.bsl[i] += bval_nested(gdiff*dslval,tdelta,tDELTA,gcrush,dcrush,Dcrush);
        diffusion.bsl[i] += bval_nested(gdiff*dslval,tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
        diffusion.bsl[i] += bval_nested(gcrush,dcrush,Dcrush,ss2_grad.ssamp,dgss2,Dgss2);
        /* Readout/Phase Cross-terms */
        diffusion.brp[i] += bval_cross(gdiff*dpeval,tdelta,tDELTA,-gcrush,dcrush,Dcrush);
        /* Readout/Slice Cross-terms */
        diffusion.brs[i] += bval2(-gcrush,gcrush,dcrush,Dcrush);
        diffusion.brs[i] += bval_cross(gdiff*droval,tdelta,tDELTA,gcrush,dcrush,Dcrush);
        diffusion.brs[i] += bval_cross(gdiff*dslval,tdelta,tDELTA,-gcrush,dcrush,Dcrush);
        diffusion.brs[i] += bval_cross(gdiff*droval,tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
        diffusion.brs[i] += bval_cross(-gcrush,dcrush,Dcrush,ss2_grad.ssamp,dgss2,Dgss2);
        /* Slice/Phase Cross-terms */
        diffusion.bsp[i] += bval_cross(gdiff*dpeval,tdelta,tDELTA,gcrush,dcrush,Dcrush);
        diffusion.bsp[i] += bval_cross(gdiff*dpeval,tdelta,tDELTA,ss2_grad.ssamp,dgss2,Dgss2);
        if (SESTE) {
          /* Readout */
          diffusion.bro[i] += bval(-gtmcrushr,dtmcrush,Dtmcrush);
          if (FP_GTE(gtmcrush*ttmcrush,ssr_grad.m0)) {
            /* Slice */
            diffusion.bsl[i] += bval(-gtmcrushs,dtmcrush,Dtmcrush);
            diffusion.bsl[i] += bval(ss_grad.amp,ss_grad.duration/2.0,tm);
            diffusion.bsl[i] += bval_nested(-gtmcrushs,dtmcrush,Dtmcrush,ss_grad.amp,ss_grad.duration/2.0,tm);
            /* Readout/Slice Cross-terms */
            diffusion.brs[i] += bval2(-gtmcrushr,-gtmcrushs,dtmcrush,Dtmcrush);
            diffusion.brs[i] += bval_cross(-gtmcrushr,dtmcrush,Dtmcrush,ss_grad.amp,ss_grad.duration/2.0,tm);
          } else {
            dgss = (1-gtmcrush*ttmcrush/ssr_grad.m0)*ss_grad.duration/2.0;
            /* Slice */
            diffusion.bsl[i] += 2*bval(-gtmcrushs,tmcrush_grad.duration,tmcrush_grad.duration);
            diffusion.bsl[i] += bval(ss_grad.amp,dgss,tm);
            /* Readout/Slice Cross-terms */
            diffusion.brs[i] += bval_cross(-gtmcrushr,dtmcrush,Dtmcrush,ss_grad.amp,dgss,tm);
          }
        }
        if (DSE) {
          /* Readout */
          diffusion.bro[i] += bval(-gcrush,dcrush,Dcrush);
          /* Phase */
          diffusion.bpe[i] += bval(gcrush,dcrush,Dcrush);
          /* Slice */
          diffusion.bsl[i] += bval(ss2_grad.ssamp,dgss2,Dgss2);
          /* Readout/Phase Cross-terms */
          diffusion.brp[i] += bval2(-gcrush,gcrush,dcrush,Dcrush);
          /* Readout/Slice Cross-terms */
          diffusion.brs[i] += bval_cross(-gcrush,dcrush,Dcrush,ss2_grad.ssamp,dgss2,Dgss2);
          /* Slice/Phase Cross-terms */
          diffusion.bsp[i] += bval_cross(gcrush,dcrush,Dcrush,ss2_grad.ssamp,dgss2,Dgss2);
        }
      }
    }  /* End for-all-directions */
    /* Write the values */
    write_bvalues(&diffusion,"bval","bvalue","max_bval");
  }

  /* Set DDR offset */   	
  roff=0; /* we oversample if we want pro !=0 */

  /* Phase cycle: Alternate SESTE 90 and SE 180 phase to cancel residual FID */
  mod2(ct,vphase90);                 /* 0101 */
  dbl(vphase90,vphase90);            /* 0202 */
  add(vphase90,oph,vphase90);        /* 0202 with respect to oph */
  add(vphase90,one,vphase180);       /* 1313 with respect to oph */

  /* Compressed segments */
  if (CSEG) {
    ncseg=nseg; nseg=1; /* set compressed and standard segment loop size */
    if (ixone) {
      if ((flipscale=(double *)malloc(ncseg*sizeof(double))) == NULL) nomem();
      if ((Mz=(double *)malloc(ncseg*sizeof(double))) == NULL) nomem();
      if ((Mxy=(double *)malloc(ncseg*sizeof(double))) == NULL) nomem();
      if ((flip=(double *)malloc(ncseg*sizeof(double))) == NULL) nomem();
      if (autoscale) { /* if autoscaling of flips is selected, calculate them */
        flipscale[ncseg-1]=1;
        flip[ncseg-1]=flip1*M_PI/180.0;
        Mz[ncseg-1]=cos(flip1*M_PI/180.0);
        Mxy[ncseg-1]=sin(flip1*M_PI/180.0);
        for (i=1;i<ncseg;i++) {
          Mz[ncseg-1-i]=1;
          Mxy[ncseg-1-i]=0;
          flip[ncseg-1-i]=0;
          while (Mxy[ncseg-1-i]<Mxy[ncseg-i]) {
            flip[ncseg-1-i] += M_PI/1800.0;
            Mz[ncseg-1-i]=cos(flip[ncseg-1-i]);
            Mxy[ncseg-1-i]=sin(flip[ncseg-1-i]);
            Mz[ncseg-i]=1-2*exp(-(-csegt1*log(0.5-0.5*Mz[ncseg-1-i])+segtr)/csegt1);
            Mxy[ncseg-i]=Mz[ncseg-i]*sin(flip[ncseg-i]);
          }
        }
        for (i=0;i<ncseg;i++) flipscale[i]=flip[i]/flip[ncseg-1];
        putvalue("snrpenalty",100*(sin(flip1*M_PI/180.0)-Mxy[0])/sin(flip1*M_PI/180.0));
        putCmd("flipscale = 1");   /* re-initialize flipscale */
        for (i=0;i<ncseg;i++) putCmd("flipscale[%d] = %f",i+1,flipscale[i]);
      } else { /* otherwise just get the values from flipscale */
        S_getarray("flipscale",flipscale,ncseg*sizeof(double)); /* get values */
      }
    }
    SCALEFLIP=TRUE; /* set flag to scale flips */
  }

  if (ixone) { /* Attempt to set reasonable experiment time */
    /* Standard gradient echoes */
    if (ssc<0) xtime = nsges*(ns*tr_delay+ns*(pe_steps-ssc)*(sgegTime+sgetr_delay+100e-6+2*GRADIENT_RES));
    else {
      xtime = ns*(tr_delay+(pe_steps+ssc)*(sgegTime+sgetr_delay+100e-6+2*GRADIENT_RES));
      xtime += (nsges-1)*(ns*tr_delay+ns*pe_steps*(sgegTime+sgetr_delay+100e-6+2*GRADIENT_RES));
    }
    /* EPI */
    if (ss<0) xtime += ((nt-ss)*tr*nseg*nrefs+(ntmean-ss)*trmean*nseg*volumes);
    else xtime += (tr*nseg*ss+tr*ntmean*nseg*nrefs+trmean*ntmean*nseg*volumes);
    /* Inter image delay */
    xtime += trimage*(nsges+volumes+nrefs);
    xtime += 1.0; /* add 1 sec for startup */
    g_setExpTime(xtime);
  }

if (tstartup) {
  if (ix<10) {
    rtn=gettimeofday(&tp, NULL);
    time5=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
    fprintf(stdout,"      To sequence took %f ms\n",1000*(time5-time4));
    time4=time5;
  }
}

  /* PULSE SEQUENCE *****************************************/
  status(A);
  rotate();
  triggerSelect(trigger);
  obsoffset(resto);
  delay(GRADIENT_RES);

  /* Trigger */
  if (ticks > 0) F_initval((double)nsblock,vtrigblock);

  /* Set grad_advance for first array, first acquired transient */
  if (ixone) { ifzero(rtonce); grad_advance(tep); endif(rtonce); }


  /**********************************************************/
  /* EPI (as opposed to Standard [non EPI] Gradient Echo)   */
  /**********************************************************/
  if (EPI) {

    /* Loop over standard segments */
    for (i=0;i<nseg;i++) {

      if (trtype) delay(ns*tr_delay); /* EPI relaxation delay */

      /* Begin compressed multislice loop */       
      msloop('c',ns,vms_slices,vms_ctr);

        if (!trtype) delay(tr_delay); /* EPI relaxation delay */

        if (ticks > 0) {
          modn(vms_ctr,vtrigblock,vtest);
          ifzero(vtest);              /* if the beginning of an trigger block */
            xgate(ticks);
            grad_advance(tep);
            delay(GRADIENT_RES);
          elsenz(vtest);
            delay(GRADIENT_RES);
          endif(vtest);
        }

        sp1on(); delay(GRADIENT_RES); sp1off(); /* scope trigger */

        /* Prepulse options */
        /* The ASL module can not be used with the IR module */
        /* The ASL module uses the IR module real time variables, real time variables v41,v42 and real time tables t57-t60 */
        if (asl[0] == 'y')  arterial_spin_label();
        if (ir[0] == 'y')   inversion_recovery();
        if (sat[0] == 'y')  satbands();
        if (fsat[0] == 'y') fatsat();
        if (mt[0] == 'y')   mtc();

        if (SESTE) { /* spin-stimulated echo */
          obspower(p1_rf.powerCoarse);
          obspwrf(p1_rf.powerFine);
          delay(GRADIENT_RES);
          /* Excitation pulse */
          obl_shapedgradient(ss_grad.name,ss_grad.duration,0.0,0.0,ss_grad.amp,NOWAIT);
          if (exrf[0] == 'y') {
            delay(ss_grad.rfDelayFront);
            shapedpulselist(shape90,ss_grad.rfDuration,oph,rof1,rof1,'c',vms_ctr);
            delay(ss_grad.rfDelayBack);
          } else 
            delay(ss_grad.duration);
          obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);
          /* Vascular Suppression */
          if (VS) vascular_suppress();
          /* Diffusion dephase */
          if (DW) {
            delay(diffusion.d1);
            diffusion_dephase(&diffusion,dro,dpe,dsl);
            delay(diffusion.d2);
          } 
          else delay(del1);
          obspower(p2_rf.powerCoarse);
          obspwrf(p2_rf.powerFine);
          delay(GRADIENT_RES);
          /* Spin echo refocus pulse */
          obl_shapedgradient(crush_grad.name,crush_grad.duration,-crush_grad.amp,0.0,crush_grad.amp,WAIT);
          obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0.0,0.0,ss2_grad.amp,NOWAIT);
          delay(ss2_grad.rfDelayFront);
          shapedpulselist(shape180,ss2_grad.rfDuration,vphase180,rof1,rof1,'c',vms_ctr);
          delay(ss2_grad.rfDelayBack);
          obl_shapedgradient(crush_grad.name,crush_grad.duration,-crush_grad.amp,0.0,crush_grad.amp,WAIT);
          /* Diffusion rephase */
          if (DW) {
            delay(diffusion.d3);
            diffusion_rephase(&diffusion,dro,dpe,dsl);
            delay(diffusion.d4);
          } 
          else delay(del2);
          /* Spin echo forms here (signal as after first 90 of standard stimulated echo) */
          obspower(p1_rf.powerCoarse);
          obspwrf(p1_rf.powerFine);
          delay(ste_delay);
          obl_shapedgradient(tmcrush_grad.name,tmcrush_grad.duration,-gtmcrushr,0,-gtmcrushs,WAIT);
          /* Store magnetization along z */
          obl_shapedgradient(ss_grad.name,ss_grad.duration,0.0,0.0,ss_grad.amp,NOWAIT);
          delay(ss_grad.rfDelayFront);
          shapedpulselist(shape90,ss_grad.rfDuration,vphase90,rof1,rof1,'c',vms_ctr);
          delay(ss_grad.rfDelayBack);
          /* Mixing period */
          if (ttmspoil>0.0) 
            obl_shapedgradient(tmspoil_grad.name,tmspoil_grad.duration,tmspoil_grad.amp,tmspoil_grad.amp,tmspoil_grad.amp,WAIT);
          delay(tm_delay);
        }

        /* Loop over compressed segments */
        for (j=0;j<ncseg;j++) {

          if (CSEG && RSEG) { /* Reverse reference segment order */
            switch ((int)image) {
              case -1:        /* Inverted image scan */
                l=ncseg-1-j;
                break;
              case -2:        /* Inverted reference scan */
                l=ncseg-1-j;
                break;
              default:
                l=j;
                break;
            }
          } else l=j;

          if (ALT) {          /* Alternate read gradient on alternate segments */
            if ((i+j)%2) sign = -1.0;
            else sign = 1.0;
          }

          obspower(p1_rf.powerCoarse);
          if (SCALEFLIP) obspwrf(p1_rf.powerFine*flipscale[j]);
          else obspwrf(p1_rf.powerFine);
          delay(GRADIENT_RES);

          if (GE) { /* gradient echo EPI */
            if (EPIPK) { /* EPI preparation kernel (basic gradient echo EPI) */
              obl_shaped3gradient(epipR.name,epipP.name,epipS.name,epipTime,roramp*sign,epipP.amp,epipS.amp,NOWAIT);
              delay(ss_grad.rfDelayFront);
              shapedpulselist(shape90,ss_grad.rfDuration,oph,rof1,rof1,'c',vms_ctr);
              delay(delayToAcq-alfa);
            }
            if (!EPIPK) { /* gradient echo EPI with vascular suppression or diffusion */
              obl_shapedgradient(ss_grad.name,ss_grad.duration,0.0,0.0,ss_grad.amp,NOWAIT);
              delay(ss_grad.rfDelayFront);
              shapedpulselist(shape90,ss_grad.rfDuration,oph,rof1,rof1,'c',vms_ctr);
              delay(ss_grad.rfDelayBack);
              obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);
              if (VS) vascular_suppress();
              if (DW) {
                delay(diffusion.d1);
                diffusion_dephase(&diffusion,dro,dpe,dsl);
                delay(diffusion.d2);
                delay(diffusion.d3);
                diffusion_rephase(&diffusion,dro,dpe,dsl);
                delay(diffusion.d4);
              } 
              else delay(del1);
              obl_shapedgradient(ror_grad.name,ror_grad.duration,-roramp*sign,0,0,NOWAIT);
              delay(ror_grad.duration-alfa);
            }
          }

          if (SE) { /* spin echo EPI */
            obl_shapedgradient(ss_grad.name,ss_grad.duration,0.0,0.0,ss_grad.amp,NOWAIT);
            delay(ss_grad.rfDelayFront);
            shapedpulselist(shape90,ss_grad.rfDuration,oph,rof1,rof1,'c',vms_ctr);
            delay(ss_grad.rfDelayBack);
            obl_shapedgradient(ssr_grad.name,ssr_grad.duration,0,0,-ssr_grad.amp,WAIT);
            obspower(p2_rf.powerCoarse);
            obspwrf(p2_rf.powerFine);
            delay(GRADIENT_RES);
            if (DSE) { /* double spin echo EPI */
              delay(del3);
              obl_shapedgradient(crush_grad.name,crush_grad.duration,-crush_grad.amp,crush_grad.amp,0.0,WAIT);
              obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0.0,0.0,ss2_grad.amp,NOWAIT);
              delay(ss2_grad.rfDelayFront);
              shapedpulselist(shape180,ss2_grad.rfDuration,vphase180,rof1,rof1,'c',vms_ctr);
              delay(ss2_grad.rfDelayBack);
              obl_shapedgradient(crush_grad.name,crush_grad.duration,-crush_grad.amp,crush_grad.amp,0.0,WAIT);
            }
            if (VS) vascular_suppress();
            if (DW) {
              delay(diffusion.d1);
              diffusion_dephase(&diffusion,dro,dpe,dsl);
              delay(diffusion.d2);
            } 
            else delay(del1);
            obl_shapedgradient(crush_grad.name,crush_grad.duration,-crush_grad.amp,0.0,crush_grad.amp,WAIT);
            obl_shapedgradient(ss2_grad.name,ss2_grad.duration,0.0,0.0,ss2_grad.amp,NOWAIT);
            delay(ss2_grad.rfDelayFront);
            shapedpulselist(shape180,ss2_grad.rfDuration,vphase180,rof1,rof1,'c',vms_ctr);
            delay(ss2_grad.rfDelayBack);
            obl_shapedgradient(crush_grad.name,crush_grad.duration,-crush_grad.amp,0.0,crush_grad.amp,WAIT);

            if (DW) {
              delay(diffusion.d3);
              diffusion_rephase(&diffusion,dro,dpe,dsl);
              delay(diffusion.d4);
            }
            else delay(del2);
            obl_shapedgradient(ror_grad.name,ror_grad.duration,-roramp*sign,0,0,NOWAIT);
            delay(ror_grad.duration-alfa);
          }

          if (SESTE) { /* spin-stimulated echo */
            obl_shapedgradient(ss_grad.name,ss_grad.duration,0.0,0.0,ss_grad.amp,NOWAIT);
            delay(ss_grad.rfDelayFront);
            shapedpulselist(shape90,ss_grad.rfDuration,vphase90,rof1,rof1,'c',vms_ctr);
            delay(ss_grad.rfDelayBack);
            obl_shapedgradient(tmcrush_grad.name,tmcrush_grad.duration,-gtmcrushr,0,-gtmcrushs,WAIT);
            delay(del1);
            obl_shapedgradient(ror_grad.name,ror_grad.duration,-roramp*sign,0,0,NOWAIT);
            delay(ror_grad.duration-alfa);
          }

          /* EPI Readout */
          if ((asl[0] == 'y') && (volumercv[0] == 'n')) asl_xmtoff(); /* PDD switching only for dedicated ASL tagging coil configuration */
          startacq(alfa);   /* prepare for acquisition (includes standard PDD switching) */
          if (TC) delay(tc_delay[i+l]); /* segment time correction */
          obl_shaped3gradient(epirR.name,epipename[i+l],epirS.name,epirTime,roamp*sign,peamp[i+l],epirS.amp,NOWAIT);
          nwloop(ntraces,vtraces,vtrace_ctr);
            delay(delacq);
            for(k=0;k<samplenp;k++) {
              sample(dw);				
              delay(si[k]);
            }
            delay(delacq);
          endnwloop(vtrace_ctr);
          endacq();           /* finalise the acquisition (includes standard PDD switching) */
          if ((asl[0] == 'y') && (volumercv[0] == 'n')) asl_xmton(); /* PDD switching only for dedicated ASL tagging coil configuration */
          delay(GRADIENT_RES); /* for very large sw it seems there can be a slight timing error ?? */
          obl_shapedgradient(spoil_grad.name,spoil_grad.duration,spoil_grad.amp,spoil_grad.amp,spoil_grad.amp,WAIT);
          if (TC) delay(tcmax-tc_delay[i+l]);  /* segment time correction */
          if (CSEG) delay(segtr_delay); /* compressed segment delay */

        } /* end of compressed EPI segment loop */

      endmsloop('c',vms_ctr);

    } /* end of standard EPI segment loop */

  } /* end of EPI (as opposed to standard non EPI gradient echo) */


  /**********************************************************/
  /* Standard [non EPI] Gradient Echo                       */
  /**********************************************************/
  if (SGE) { 

    F_initval((double)nphase,vnphase);  /* required number of PE steps */
    F_initval((double)ntraces,vtraces); /* required number of traces per 'fid' */
    initval(fabs(ssc),vssc);            /* compressed steady-states */
    assign(zero,vtrace_ctr);            /* set trace counter */
    assign(one,vacquire);               /* set real-time acquire flag */

    delay(ns*tr_delay);                 /* delay to standard gradient echo */

    /* Begin phase-encode loop */       
    peloop('c',pe_steps,vpe_steps,vpe_ctr);

      sub(vpe_ctr,vssc,vpe_ctr);        /* vpe_ctr counts up from -ssc */
      assign(zero,vssc);                /* for next pass through peloop */
      /* Start acquiring and counting traces when vpe_ctr reaches zero */
      ifzero(vpe_ctr); assign(zero,vacquire); endif(vpe_ctr);
      /* Set PE mult according to vpe_ctr or hold at initial value for steady states */
      ifrtLT(vpe_ctr,vnphase,vtest); getelem(t1,vpe_ctr,vpe_mult); endif(vtest);

      /* Begin compressed multislice loop */       
      msloop('c',ns,vms_slices,vms_ctr);

        delay(sgetr_delay);             /* standard gradient echo relaxation delay */

        if (ticks > 0) {
          modn(vms_ctr,vtrigblock,vtest);
          ifzero(vtest);                /* if the beginning of an trigger block */
            xgate(ticks); grad_advance(tep); delay(GRADIENT_RES);        
          elsenz(vtest);
            delay(GRADIENT_RES);
          endif(vtest);
        }

        sp1on(); delay(GRADIENT_RES); sp1off(); /* scope trigger */

        obspower(sge_rf.powerCoarse); 
        obspwrf(sge_rf.powerFine);
        delay(GRADIENT_RES);

        ifzero(vacquire); /* acquire data */
          ifzero(vtrace_ctr); /* if the first trace of a 'fid' */
            if ((asl[0] == 'y') && (volumercv[0] == 'n')) asl_xmtoff(); /* PDD switching only for dedicated ASL tagging coil configuration */
            startacq(alfa);   /* prepare for acquisition */
          elsenz(vtrace_ctr); /* else */
            delay(alfa);      /* delay by alfa */ 
          endif(vtrace_ctr);
          if ((asl[0] == 'y') && (volumercv[0] == 'n')) asl_xmton();    /* PDD switching only for dedicated ASL tagging coil configuration */
          pdd_postacq(); /* startacq essentially performs pdd_preacq() so pdd_postacq() required for transmit pulse */
          pe_shaped3gradient(sgeR.name,sgeP.name,sgeS.name,sgegTime,sgeR.amp,0,sgeS.amp,-sgepe_grad.increment,vpe_mult,NOWAIT);
          delay(ss_grad.rfDelayFront);
          shapedpulselist(shape90,ss_grad.rfDuration,oph,rof1,rof1,'c',vms_ctr);
          if ((asl[0] == 'y') && (volumercv[0] == 'n')) asl_xmtoff();   /* PDD switching only for dedicated ASL tagging coil configuration */
          pdd_preacq(); /* switch PDD pre acquisition (if config requires) */
          delay(sgedToAcq);
          rcvron(); /* a delay between recvron() and sample() eliminates artefact */
          delay(sgedacq);
          for(k=0;k<samplenp;k++) { sample(dw); delay(sgesi[k]); }
          rcvroff();
          delay(sgedacq);
          pdd_postacq(); /* switch PDD post acquisition (if config requires) */
          delay(sgedFromAcq);
          incr(vtrace_ctr); /* increment trace counter */
          delay(GRADIENT_RES);
        elsenz(vacquire); /* steady states */
          delay(alfa);
          if ((asl[0] == 'y') && (volumercv[0] == 'n')) asl_xmton();    /* PDD switching only for dedicated ASL tagging coil configuration */
          pdd_postacq(); /* just to be sure PDD config is set for transmit pulse */
          pe_shaped3gradient(sgeR.name,sgeP.name,sgeS.name,sgegTime,sgeR.amp,0,sgeS.amp,-sgepe_grad.increment,vpe_mult,NOWAIT);
          delay(ss_grad.rfDelayFront);
          shapedpulselist(shape90,ss_grad.rfDuration,oph,rof1,rof1,'c',vms_ctr);
          if ((asl[0] == 'y') && (volumercv[0] == 'n')) asl_xmtoff();   /* PDD switching only for dedicated ASL tagging coil configuration */
          pdd_preacq(); /* switch PDD pre acquisition (if config requires) */
          delay(sgedToAcq);
          delay(sgero_grad.duration);
          pdd_postacq(); /* switch PDD post acquisition (if config requires) */
          delay(sgedFromAcq);
          delay(GRADIENT_RES);
        endif(vacquire); /* end of steady states */
        sub(vtraces,vtrace_ctr,vtest);    /* test if the last trace of a 'fid' has been acquired */
        ifzero(vtest);                    /* if so */
          endacq();                       /* finalise the acquisition */
          if ((asl[0] == 'y') && (volumercv[0] == 'n')) asl_xmton(); /* PDD switching only for dedicated ASL tagging coil configuration */
          assign(zero,vtrace_ctr);        /* reset trace counter */
        endif(vtest);
        delay(100e-6); /* a delay is required between an endacq and the next startacq */

      endmsloop('c',vms_ctr);           /* end slice loop */

    endpeloop('c',vpe_ctr);             /* end PE loop */

  }

  /* Inter-image delay */
  sub(ntrt,ct,vtrimage);
  decr(vtrimage);
  ifzero(vtrimage);
    delay(trimage);
  endif(vtrimage);

}
