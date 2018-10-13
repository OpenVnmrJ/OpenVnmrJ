/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <fcntl.h>
#include "group.h"
#include "symtab.h"
#include "variables.h"
#include "acqparms.h"
#include "aptable.h"
#include "macros.h"
#include "expDoneCodes.h"
#include "abort.h"
#include "pvars.h"
#include "cps.h"

#define FALSE 0
#define TRUE  1
#define NOTFOUND (-1)
#define STDVAR 70

#define MAXSTR 256

extern void nextscan();
extern void postInitScanCodes();
extern void preacqdelay(int padactive);
extern void preNextScanDuration();
extern int getExpNum();
extern int deliverMessageSuid(char *interface, char *message );
extern void ra_inovaacqparms(unsigned int fidn);
extern void set_counters();
extern void new_lcinit_arrayvars();
extern int getStartFidNum();
extern void setvt();
extern void wait4vt();
extern void setspin();
extern void wait4spin();
extern void inittablevar();
extern void initparms_img();
extern void pulsesequence();
extern void reset_table(Tableinfo *tblinfo);
extern void getlockmode(char *alock, int *mode);
extern int setshimflag(char *wshim, int *flag);

extern int bgflag;
extern int ap_interface;	/* ap bus interface type 1=500style, 2=amt style */
extern int newacq;
extern int acqiflag;
extern int checkflag;
extern int dps_flag;
extern int ok2bumpflag;
extern int lockfid_mode;
extern int initializeSeq;

extern char rfwg[];
extern char gradtype[];

extern char   **cnames;	/* pointer array to variable names */
extern int      nnames;		/* number of variable names */

extern int      ntotal;		/* total number of variable names */
extern int      ncvals;		/* NUMBER OF VARIABLE  values */
extern double **cvals;	/* pointer array to variable values */
extern double relaxdelay;
extern int grad_flag;	/* in gradient.c */
extern int scanflag,initflagrt,ilssval,ilctss; /* inova realtime vars */
extern int tmprt; /* inova realtime vars */

extern int traymax;

int lkflt_fast;	/* value to set lock loop filter to when fast */
int lkflt_slow;	/* value to set lock loop filter to when slow */
/* static int ra_initcnt = 0; */  /* If lc has been updated in an ra the 1st fid not
			 * acquired yet (no data) lc must be reset, after that
			 * lc need not be reset, hence this count flag */
int HSrotor;
int HS_Dtm_Adc;		/* Flag to convert.c to select INOVA 5MHz High Speed DTM/ADC */
int activeRcvrMask;	/* Which receivers to use */

double d0; 	      /* defined array element overhead time */
double interfidovhd;  /* calculated array element overhead time */

int locActive;
int zerofid=0;

extern int d2_index;
extern int d3_index;
#define SAMPLING_STANDARD  0
#define SAMPLING_ELLIPTICAL 1
static int sampling = SAMPLING_STANDARD;

static double samplingScale = 1.01;
static double samplingAngle = 0.0;
static double samplingTransX = 0.0;
static double samplingTransY = 0.0;
static double samplingRows = 1.0;
static double samplingCols = 1.0;

int samplingElliptical(int irow, int jcol)
{
   double row;
   double col;
   double res;
   double xAxis;
   double yAxis;

   xAxis = (double) (samplingCols-1) / 2.0;
   yAxis = (double) (samplingRows-1) / 2.0;

   row = (double) irow - yAxis;
   col = (double) jcol - xAxis;
   col -= samplingTransY;
   row -= samplingTransX;
   if (samplingAngle != 0.0)
   {
      double cosx, sinx;
      double newx, newy;
      double radians;
      radians = samplingAngle * M_PI / 180.0;
      cosx = cos(radians);
      sinx = sin(radians);
      newx = col*cosx - row*sinx;
      newy = col*sinx + row*cosx;
      col = newx;
      row = newy;
   }
   res = (row*row)/(yAxis*yAxis) +  (col*col)/(xAxis*xAxis);
   return ((res <= samplingScale) ? 1 : 0);
}

static int doSampling()
{
   if (sampling == SAMPLING_STANDARD)
      return(1);
   if (sampling == SAMPLING_ELLIPTICAL)
      return( samplingElliptical(d2_index, d3_index) );
   return(1);
}


/*-----------------------------------------------------------------------
|  createPS(int arrayDim)
|	create the acode for the Pulse Sequence.
+-----------------------------------------------------------------------*/
void createPS(int arrayDim)
{
    int i;

    if (ra_flag)
    {
      ra_inovaacqparms(ix);
    }

    set_counters();

    /* code for HSrotor? */

    new_lcinit_arrayvars();

    if ( ix == getStartFidNum() )
    {
       /*
        *  We may have a monitor system (ILI, ISI, ...) and
        *  we might have an MTS Grad Power Amp attached
        *
        *  SAFETY CHECK & setupcode
        */

    }

    /*
         Do initialization for RF , DDR & Gradient that happens for each FID
         setRF();
    */
    postInitScanCodes();


    /*
      if (ix == getStartFidNum())
      {
        ifzero(initflagrt);
          reset_pgdflag();
          initHSlines();
        endif(initflagrt);
      }
    */

    arrayedshims();	/* if shims arrayed, set them again, load will be  */
			/* set to 'y' in arrayfuncs  */

    if (vttemp != oldvttemp)	/* change vt setting only if it changed */
    {				/* why?, (because it takes time) */
	    setvt();		/* And it includes fifoStop/Sync */
        wait4vt();		/* And this includes fifoStop/Sync */
    }
    oldvttemp = vttemp;		/* if arrayed, vttemp will get new value */

    if (spin != oldspin) 	/* chag spin setting only if it changed */
    {				/* why?, (because it takes time) */
       setspin();		/* And it includes fifoStop/Sync */
       wait4spin();		/* And this includes fifoStop/Sync */
    }
    oldspin = spin;

    /* capture elapsed time prior to pad and nt looping */
    preNextScanDuration();
    ifzero((oph-8));
    preacqdelay(padactive);
    endif((oph-8));

    /* ###################################################################### */

    nextscan();  /* nt looping reset to this point.  */

    /* ###################################################################### */


    loadtablecall = -1; /* initializes loadtablecall variable       */
    inittablevar();     /* initialize all internal table variables  */
    initparms_img();    /* does getval on many variables            */
    enableHDWshim();    /* config done internally */
    setAmpBlanking();   /* if unblank mode, restore unblank */
    if ( doSampling() )
    {
       zerofid=0;
       pulsesequence();	/* generate Acodes from USER Pulse Sequence */
       initializeSeq = 0;
    }
    else
    {
       zerofid=1;
       sendzerofid(arrayDim);
    }

    /* reset tables between array elements */
    for (i = 0; i < MAXTABLE; i++)
    {
       tmptable_order[i] = table_order[i];
       if (Table[i]->reset)
          reset_table(Table[i]);
    }

    return;
}

/*-----------------------------------------------------------------
|	initparms()/
|	initializes the main variables used
+------------------------------------------------------------------*/
void initparms()
{
    double tmpval;
    char   tmpstr[36];
    int    tmp, getchan;

    sw = getval("sw");
    np = getval("np");

    if ( P_getreal(CURRENT,"nf",&nf,1) < 0 )
    {
        nf = 0.0;                /* if not found assume 0 */
    }
    if (nf < 2.0)
	nf = 1.0;

    nt = getval("nt");
    sfrq = getval("sfrq");

    filter = getval("filter");		/* pulse Amp filter setting */
    tof = getval("tof");
    bs = getval("bs");
    if (!var_active("bs",CURRENT))
	bs = 0.0;
    pw = getval("pw");
    pw90 = getval("pw90");
    p1 = getval("p1");

    pwx = getvalnwarn("pwx");
    pwxlvl = getvalnwarn("pwxlvl");
    tau = getvalnwarn("tau");
    satdly = getvalnwarn("satdly");
    satfrq = getvalnwarn("satfrq");
    satpwr = getvalnwarn("satpwr");
    getstrnwarn("satmode",satmode);

    /* ddr */
    roff=getvalnwarn("roff");

    /* --- delays --- */
    d1 = getval("d1"); 		/* delay */
    d2 = getval("d2"); 		/* a delay: used in 2D experiments */
    d3 = getvalnwarn("d3");	/* a delay: used in 3D experiments */
    d4 = getvalnwarn("d4");	/* a delay: used in 4D experiments */
    phase1 = (int) sign_add(getvalnwarn("phase"),0.005);
    phase2 = (int) sign_add(getvalnwarn("phase2"),0.005);
    phase3 = (int) sign_add(getvalnwarn("phase3"),0.005);
    rof1 = getval("rof1"); 	/* Time receiver is turned off before pulse */
    rof2 = getval("rof2");	/* Time after pulse before receiver turned on */
    alfa = getval("alfa"); 	/* Time after rec is turned on that acqbegins */
    pad = getval("pad"); 	/* Pre-acquisition delay */
    padactive = var_active("pad",CURRENT);
    hst = getval("hst"); 	/* HomoSpoil delay */


    tpwr = getval("tpwr");
    if ( P_getreal(CURRENT,"tpwrm",&tpwrf,1) < 0 )
       if ( P_getreal(CURRENT,"tpwrf",&tpwrf,1) < 0 )
          tpwrf = 4095.0;

    //getstr("rfband",rfband);	/* RF band, high or low */

    getstr("hs",hs);
    hssize = strlen(hs);
    /* setlockmode(); */		/* set up lockmode variable,homo bits */
    if (bgflag)
    {
      fprintf(stderr,"sw = %lf, sfrq = %10.8lf\n",sw,sfrq);
      fprintf(stderr,"hs='%s',%d\n",hs,hssize);
    }
    gain = getval("gain");

    gainactive = var_active("gain",CURRENT); /* non arrayable */
    /* InterLocks is set by go.  It will have three chars.
     * char 0 is for lock
     * char 1 is for spin
     * char 2 is for temp
     */
    getstr("interLocks",interLock); /* non arrayable */
    spin = (int) sign_add(getval("spin"),0.005);
    spinactive = var_active("spin",CURRENT); /* non arrayable */
    HSrotor = 0;  /* high speed spinner selected */
    /* spinTresh is created and set by go.c inside Vnmrbg */
    if (spin >= (int) sign_add(getval("spinThresh"),0.005))
    {
       /* Selected Solids spinner */
       HSrotor = 1;
    }
    vttemp = getval("temp");	/* get vt temperature */
    tempactive = var_active("temp",CURRENT); /* non arrayable */
    vtwait = getval("vtwait");	/* get vt timeout setting */
    vtc = getval("vtc");	/* get vt timeout setting */
    if (getparm("traymax","real",GLOBAL,&tmpval,1))
    {
      traymax=0;
    }
    else
    {
      traymax= (int) (tmpval + 0.5);
    }

    if (getparm("loc","real",GLOBAL,&tmpval,1))
	psg_abort(1);
    loc = (int) sign_add(tmpval,0.005);
    if (!var_active("loc",GLOBAL) || (loc<0) )
    {
        locActive = 0;
        loc = 0;
        tmpval = 0.0;
        if (setparm("loc","real",GLOBAL,&tmpval,1))
          psg_abort(1);
    }
    else
      locActive = 1;

    /* if using Gilson Liquid Handler Racks then gilpar is defined
     * and is an array of 4 values */
    if ((traymax == 96) || (traymax == (8*96)))  /* test for Gilson/Hermes */
    {
       int trayloc=1;
       int trayzone=1;

       if ( P_getreal(GLOBAL, "vrack", &tmpval, 1) >= 0 )
          trayloc = (int) (tmpval + 0.5);

       if ( P_getreal(GLOBAL, "vzone", &tmpval, 1) >= 0 )
          trayzone = (int) (tmpval + 0.5);

       /* rrzzllll */
       loc = loc + (10000 * trayzone) + (1000000 * trayloc);
       if (bgflag)
          fprintf(stderr,"GILSON: ----- vrack: %d, vzone: %d, Encoded Loc = %d\n",trayloc,trayzone,loc);
    }

    getstr("alock",alock);
    getstr("wshim",wshim);
    getlockmode(alock,&lockmode);		/* type of autolocking */
    whenshim = setshimflag(wshim,&shimatanyfid); /* when to shim */
    if ( ( tmp=P_getstring(CURRENT, "sampling", tmpstr, 1, 20)) >= 0)
    {
       samplingScale = 1.01;
       samplingAngle = 0.0;
       samplingTransX = 0.0;
       samplingTransY = 0.0;
       if (tmpstr[0] == 'e')
       {
          double rtmp = 0.0;
          sampling = SAMPLING_ELLIPTICAL;
          P_getreal(CURRENT, "ni", &rtmp, 1);
          samplingRows = (int) rtmp;
          rtmp = 0.0;
          P_getreal(CURRENT, "ni2", &rtmp, 1);
          samplingCols = (int) rtmp;
          if ((samplingRows < 2) || (samplingCols < 2))
             sampling = SAMPLING_STANDARD;
          if (sampling == SAMPLING_ELLIPTICAL)
          {
             if ( P_getreal(CURRENT, "samplingEScale", &rtmp, 1) >= 0)
                samplingScale = rtmp;
             if ( P_getreal(CURRENT, "samplingEAngle", &rtmp, 1) >= 0)
                samplingAngle = rtmp;
             if ( P_getreal(CURRENT, "samplingETransX", &rtmp, 1) >= 0)
                samplingTransX = rtmp;
             if ( P_getreal(CURRENT, "samplingETransY", &rtmp, 1) >= 0)
                samplingTransY = rtmp;
          }
       }
       else
          sampling = SAMPLING_STANDARD;
    }
    else
       sampling = SAMPLING_STANDARD;
// this looks REDUNDANT to instantiation...
    // but is not completely dpwrf
    if ( ( tmp=P_getstring(CURRENT, "dn", tmpstr, 1, 9)) >= 0)
       getchan = TRUE;
    else
       getchan = FALSE;
    /* if "dn" does not exist, don't bother with the rest of channel 2 */
    getchan = (NUMch > 1) && getchan && (tmpstr[0]!='\000');
    if (getchan)		/* variables associated with 2nd channel */
    {
       dfrq = getval("dfrq");
       dmf  = getval("dmf");		/* 1st decoupler modulation freq */
       dof  = getval("dof");
       dres = getvalnwarn("dres");	/* prg decoupler digital resolution */
       if (dres < 1.0) dres = 1.0;
       getstrnwarn("dseq",dseq);

          dpwr = getval("dpwr");
          dhp     = 0.0;
          dlp     = 0.0;

       if ( P_getreal(CURRENT,"dpwrm",&dpwrf,1) < 0 )
          if ( P_getreal(CURRENT,"dpwrf",&dpwrf,1) < 0 )
             dpwrf = 4095.0;
       getstr("dm",dm);
       dmsize = strlen(dm);
       getstr("dmm",dmm);
       dmmsize = strlen(dmm);
       getstr("homo",homo);
       homosize = strlen(homo);
    }
    else
    {
       dfrq    = 1.0;
       dmf     = 1000;
       dof     = 0.0;
       dres    = 1.0;
       dseq[0] = '\000';
       dhp     = 0.0;
       dlp     = 0.0;
       dpwr    = 0.0;
       dpwrf   = 0.0;
       strcpy(dm,"n");
       dmsize  = 1;
       strcpy(dmm,"c");
       dmmsize = 1;
       strcpy(homo,"n");
       homosize= 1;
    }
    if (bgflag)
    {
       if (!getchan)
          fprintf(stderr,"next line are default values for chan 2\n");
       fprintf(stderr,"dm='%s',%d, dmm='%s',%d\n",dm,dmsize,dmm,dmmsize);
       fprintf(stderr,"homo='%s',%d\n",homo,homosize);
    }

    if ( (tmp=P_getstring(CURRENT, "dn2", tmpstr, 1, 9)) >= 0)
       getchan = TRUE;
    else
       getchan = FALSE;
    /* if "dn2" does not exist, don't bother with the rest of channel 3 */
    getchan = (NUMch > 2) && getchan && (tmpstr[0]!='\000');
    if (getchan)			/* variables associated with 3rd channel */
    {
      dfrq2 = getval("dfrq2");
      dmf2  = getval("dmf2");		/* 2nd decoupler modulation freq */
      dof2  = getval("dof2");
      dres2 = getvalnwarn("dres2");	/* prg decoupler digital resolution */
      if (dres2 < 1.0) dres2 = 1.0;
      getstrnwarn("dseq2",dseq2);
      dpwr2 = getval("dpwr2");
      if ( P_getreal(CURRENT,"dpwrm2",&dpwrf2,1) < 0 )
         if ( P_getreal(CURRENT,"dpwrf2",&dpwrf2,1) < 0 )
            dpwrf2 = 4095.0;
      getstr("dm2",dm2);
      dm2size = strlen(dm2);
      getstr("dmm2",dmm2);
      dmm2size = strlen(dmm2);
      getstr("homo2",homo2);
      homo2size = strlen(homo2);
    }
    else
    {
       dfrq2    = 1.0;
       dmf2     = 1000;
       dof2     = 0.0;
       dres2    = 1.0;
       dseq2[0] = '\000';
       dpwr2    = 0.0;
       dpwrf2   = 0.0;
       strcpy(dm2,"n");
       dm2size  = 1;
       strcpy(dmm2,"c");
       dmm2size = 1;
       strcpy(homo2,"n");
       homo2size= 1;
    }
    if (bgflag)
    {
       if (!getchan)
          fprintf(stderr,"next two lines are default values for chan 3\n");
       fprintf(stderr,"dfrq2 = %10.8lf, dof2 = %10.8lf, dpwr2 = %lf\n",
	   dfrq2,dof2,dpwr2);
       fprintf(stderr,"dmf2 = %10.8lf, dm2='%s',%d, dmm2='%s',%d\n",
	   dmf2,dm2,dm2size,dmm2,dmm2size);
       fprintf(stderr,"homo2='%s',%d\n",homo2,homo2size);
    }

    if ( (tmp=P_getstring(CURRENT, "dn3", tmpstr, 1, 9)) >= 0)
       getchan = TRUE;
    else
       getchan = FALSE;
    /* if "dn3" does not exist, don't bother with the rest of channel 3 */
    getchan = (NUMch > 3) && getchan && (tmpstr[0]!='\000');
    if (getchan)			/* variables associated with 3rd channel */
    {
      dfrq3 = getval("dfrq3");
      dmf3  = getval("dmf3");		/* 3nd decoupler modulation freq */
      dof3  = getval("dof3");
      dres3 = getvalnwarn("dres3");	/* prg decoupler digital resolution */
      if (dres3 < 1.0) dres3 = 1.0;
      getstrnwarn("dseq3",dseq3);
      dpwr3 = getval("dpwr3");
      if ( P_getreal(CURRENT,"dpwrm3",&dpwrf3,1) < 0 )
         if ( P_getreal(CURRENT,"dpwrf3",&dpwrf3,1) < 0 )
            dpwrf3 = 4095.0;
      getstr("dm3",dm3);
      dm3size = strlen(dm3);
      getstr("dmm3",dmm3);
      dmm3size = strlen(dmm3);
      getstr("homo3",homo3);
      homo3size = strlen(homo3);
    }
    else
    {
       dfrq3    = 1.0;
       dmf3     = 1000;
       dof3     = 0.0;
       dres3    = 1.0;
       dseq3[0] = '\000';
       dpwr3    = 0.0;
       dpwrf3   = 0.0;
       strcpy(dm3,"n");
       dm3size  = 1;
       strcpy(dmm3,"c");
       dmm3size = 1;
       strcpy(homo3,"n");
       homo3size= 1;
    }

    if (bgflag)
    {
       if (!getchan)
          fprintf(stderr,"next two lines are default values for chan 3\n");
       fprintf(stderr,"dfrq3 = %10.8lf, dof3 = %10.8lf, dpwr3 = %lf\n",
	   dfrq3,dof3,dpwr3);
       fprintf(stderr,"dmf3 = %10.8lf, dm3='%s',%d, dmm3='%s',%d\n",
	   dmf3,dm3,dm3size,dmm3,dmm3size);
       fprintf(stderr,"homo3='%s',%d\n",homo3,homo3size);
    }

    if ( (tmp=P_getstring(CURRENT, "dn4", tmpstr, 1, 9)) >= 0)
       getchan = TRUE;
    else
       getchan = FALSE;
    /* if "dn4" does not exist, don't bother with the rest of channel 4 */
    getchan = (NUMch > 4) && getchan && (tmpstr[0]!='\000');
    if (getchan)			/* variables associated with 4th channel */
    {
      dfrq4 = getval("dfrq4");
      dmf4  = getval("dmf4");		/* 4nd decoupler modulation freq */
      dof4  = getval("dof4");
      dres4 = getvalnwarn("dres4");	/* prg decoupler digital resolution */
      if (dres4 < 1.0) dres4 = 1.0;
      getstrnwarn("dseq4",dseq4);
      dpwr4 = getval("dpwr4");
      if ( P_getreal(CURRENT,"dpwrm4",&dpwrf4,1) < 0 )
         if ( P_getreal(CURRENT,"dpwrf4",&dpwrf4,1) < 0 )
            dpwrf4 = 4095.0;
      getstr("dm4",dm4);
      dm4size = strlen(dm4);
      getstr("dmm4",dmm4);
      dmm4size = strlen(dmm4);
      getstr("homo4",homo4);
      homo4size = strlen(homo4);
    }
    else
    {
       dfrq4    = 1.0;
       dmf4     = 1000;
       dof4     = 0.0;
       dres4    = 1.0;
       dseq4[0] = '\000';
       dpwr4    = 0.0;
       dpwrf4   = 0.0;
       strcpy(dm4,"n");
       dm4size  = 1;
       strcpy(dmm4,"c");
       dmm4size = 1;
       strcpy(homo4,"n");
       homo4size= 1;
    }

    if (bgflag)
    {
       if (!getchan)
          fprintf(stderr,"next two lines are default values for chan 4\n");
       fprintf(stderr,"dfrq4 = %10.8lf, dof4 = %10.8lf, dpwr4 = %lf\n",
	   dfrq4,dof4,dpwr4);
       fprintf(stderr,"dmf4 = %10.8lf, dm4='%s',%d, dmm4='%s',%d\n",
	   dmf4,dm4,dm4size,dmm4,dmm4size);
       fprintf(stderr,"homo4='%s',%d\n",homo4,homo4size);
    }
// end of REDUNDANT ???
}

/*-----------------------------------------------------------------
|	getval()/1
|	returns value of variable
+------------------------------------------------------------------*/
double getval(const char *variable)
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index == NOTFOUND)
    {
        fprintf(stdout,"'%s': not found, value assigned to zero.\n",variable);
	return(0.0);
    }
    if (bgflag)
        fprintf(stderr,"GETVAL(): Variable: %s, value: %lf \n",
     	    variable,*( (double *) cvals[index]) );
    return( *( (double *) cvals[index]) );
}
/*-----------------------------------------------------------------
|	getstr()/1
|	returns string value of variable
+------------------------------------------------------------------*/
void getstr(const char *variable, char buf[])
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index != NOTFOUND)
    {
	char *content;

	content = ((char *) cvals[index]);
    	if (bgflag)
            fprintf(stderr,"GETSTR(): Variable: %s, value: '%s' \n",
     	    	variable,content);
    	strncpy(buf,content,MAXSTR-1);
	buf[MAXSTR-1] = 0;
    }
    else
    {
        fprintf(stdout,"'%s': not found, value assigned to null.\n",variable);
	buf[0] = 0;
    }
}
/*-----------------------------------------------------------------
|	getvalnwarn()/1
|	returns value of variable
+------------------------------------------------------------------*/
double getvalnwarn(const char *variable)
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index == NOTFOUND)
    {
	return(0.0);
    }
    if (bgflag)
        fprintf(stderr,"GETVAL(): Variable: %s, value: %lf \n",
     	    variable,*( (double *) cvals[index]) );
    return( *( (double *) cvals[index]) );
}
/*-----------------------------------------------------------------
|	getstrnwarn()/1
|	returns string value of variable
+------------------------------------------------------------------*/
void getstrnwarn(const char *variable, char buf[])
{
    int index;

    /* index = findsname(variable,cnames,nnames); */
    index = find(variable);   /* hash table find */
    if (index != NOTFOUND)
    {
	char *content;

	content = ((char *) cvals[index]);
    	if (bgflag)
            fprintf(stderr,"GETSTR(): Variable: %s, value: '%s' \n",
     	    	variable,content);
    	strncpy(buf,content,MAXSTR-1);
	buf[MAXSTR-1] = 0;
    }
    else
    {
	buf[0] = 0;
    }
}

/* convenience function
   tree (current etc)
   name
   buf (value of paramter)
   def if it does not exist set it to def
   this version allows arrayed use in the current tree.
*/

void getStringSetDefault(int tree, const char *name, char *buf, const char *def)
{
	int index;
	char *content;
	if (tree != CURRENT)
	{
		if (P_getstring(tree, name,buf,1,MAXSTR) < 0)
		strcpy(buf,def);
	}
	else
	{
	    index = find(name);   /* hash table find */
	    if (index != NOTFOUND)
	    {
		   content = ((char *) cvals[index]);
	       strncpy(buf,content,MAXSTR-1);
		   buf[MAXSTR-1] = 0;
	    }
	    else
		   strcpy(buf,def);
		}
}
/* gets real variable from correct tree
 * if current tree,  value may be arrayed
 */
void getRealSetDefault(int tree, const char *name, double *buf, double def)
{
	int index;
	if (tree != CURRENT)
	{
		if (P_getreal(tree, name,buf,1) < 0)
		*buf = def;
	}
	else
	{
		index = find(name);   /* hash table find */
	    if (index == NOTFOUND)
	       *buf = def;
	    else
	       *buf = *( (double *) cvals[index]);
	}
}
/*-----------------------------------------------------------------
|	sign_add()/2
|  	 uses sign of first argument to decide to add or subtract
|			second argument	to first
|	returns new value (double)
+------------------------------------------------------------------*/
double sign_add(double arg1, double arg2)
{
    if (arg1 >= 0.0)
	return(arg1 + arg2);
    else
	return(arg1 - arg2);
}

/*-----------------------------------------------------------------
|       getmaxval()/1
|       Gets the maximum value of an arrayed or list real parameter.
+------------------------------------------------------------------*/
int getmaxval(const char *parname )
{
    int      size,r,i,tmpval,maxval;
    double   dval;
    vInfo    varinfo;

    if ( (r = P_getVarInfo(CURRENT, parname, &varinfo)) ) {
        abort_message("getmaxval: could not find the parameter \"%s\"\n",parname);
    }
    if ((int)varinfo.basicType != 1) {
        abort_message("getmaxval: \"%s\" is not an array of reals.\n",parname);
    }

    size = (int)varinfo.size;
    maxval = 0;
    for (i=0; i<size; i++) {
        if ( P_getreal(CURRENT,parname,&dval,i+1) ) {
	    abort_message("getmaxval: problem getting array element %d.\n",i+1);
	}
	tmpval = (int)(dval+0.5);
	if (tmpval > maxval)	maxval = tmpval;
    }
    return(maxval);
}


/*-----------------------------------------------------------------
|	putval()/2
|	Sets a vnmr parmeter to a given value.
+------------------------------------------------------------------*/
void putval(char *paramname, double paramvalue)
{
   int stat,expnum;
   char	message[STDVAR];
   char addr[MAXSTR];

   expnum = getExpNum();
   stat = -1;
   if (getparm("vnmraddr","string",GLOBAL,addr,MAXSTR))
   {
	text_error("putval: cannot get Vnmr address for %s.\n",paramname);
   }
   else
   {
   	sprintf(message,"sysputval(%d,'%s',%g)\n",
			expnum,paramname,paramvalue);
   	stat = deliverMessageSuid(addr,message);
	if (stat < 0)
	{
	   text_error("putval: Error in parameter: %s.\n",paramname);
	}
   }

}
/*-----------------------------------------------------------------
|	putstr()/2
|	Sets a vnmr parmeter to a given string.
+------------------------------------------------------------------*/
void putstr(char *paramname, char *paramstring)
{
   int stat,expnum;
   char	message[STDVAR];
   char addr[MAXSTR];

   expnum = getExpNum();
   stat = -1;
   if (getparm("vnmraddr","string",GLOBAL,addr,MAXSTR))
   {
	text_error("putval: cannot get Vnmr address for %s.\n",paramname);
   }
   else
   {
   	sprintf(message,"sysputval(%d,'%s','%s')\n",
					expnum,paramname,paramstring);
   	stat = deliverMessageSuid(addr,message);
	if (stat < 0)
	{
	   text_error("putval: Error in parameter: %s.\n",paramname);
	}
   }

}

