/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*      Copyright 2009 Varian, Inc. and The University of Manchester    */

/****************************************************************************
* fiddle   perform reference deconvolution                                  *
*****************************************************************************/


/*
 */

/*	VERSION Manchester OpenVnmrJ 2.1 from OpenVnmrJ 2.1 release	*/
/*	DI fixes for crash if space used in writefid filename 10viii21 	*/
/*	Apply to writecf and readf too 10viii21 			*/
/*	GAM 3viii21 Implement correction of every element of an array	*/
/*		using the first element of an array of correction 	*/
/*		functions, using keyword "readsinglecf" instead of	*/
/*	 	"readcf"						*/
/*	GAM 2viii21 Correct effect of autophase on ref region when      */
/*	         writing out correction function                        */
/*	GAM 2viii21 Correct bug in 1st point of corrected FID           */
/*	GAM 30vii21 Use same endian conversion in writeout, readincf    */
/*	VERSION Manchester VNMRJ 4.2 from DI 2014.10.07 from 2.2C	*/
/*	GAM 27x15 Add endian conversion for readincf 			*/
/*	GAM 27x15 Neaten up: pad correction function with zeroes	*/
/*	GAM 27x15 Neaten up: correct number of points to correct	*/
/*	GAM 27x15 Neaten up: only write out filenames if verbose	*/
/*	GAM 27x15 Neaten up: don't create reference or calculate	*/
/*		  correction function if reading in latter		*/
/*	Version for Linux corrected by Dan Iverson 6i09			*/
/*	Split lines rejoined 7i09					*/
/*	Correct phasfile to phasefile  1ix99  GAM/PBC			*/
/*	Revision 21 May 1998						*/
/*	Don't average 1st and last points of corrected fid 21v98	*/
/*	Major reorganisation 4iii97					*/
/*	Add read/write of correction function 4iii97			*/
/*	Neatened up for circulation 10 vi 97				*/
/*	Correct error in application of dc correction 27ii97		*/
/*	Set f1 reference frequency by rfl1 7xi96			*/
/*	ct=0 bug fixed 7xi96						*/
/*	Fix quantflag with new ideal calculation GAM 24x96		*/
/*	Only use satellites inside reference region			*/
/*	New version 2 vii 96 - ideal signal is now calculated directly 	*/
/*	Add automatic phasing 12 xi 96					*/
/*	Add alternating phase for hypercomplex with new ideal calc  	*/
/*	Make autophasing of reference the default			*/


/*      Copyright 1996,1997

	This software was produced using contributions from G.A. Morris, 
	A. Gibbs, H. Barjat and C. England, using source material from 
	Varian Associates 



	Usage: 
fiddle('option'[,'filename',][,'option',['filename']][,startno][,finishno][,incr
ement])

	Aliases:

		fiddled		difference spectra: subtract alternate fids 
				if option writefid is used, the arraydim of the
				written fid halves 

		fiddleu		difference spectra: subtract 2nd and subsequent
				fids from 1st
				if option writefid is used, the arraydim of the
				written fid decreases by 1 

		fiddle2D )	2D 
		fiddle2d )	
 
		fiddle2Dd )	2D subtracting alternate fids
		fiddle2dd )	 

	Options:

		alternate	alternate reference phase +- (for 
				phase sensitive gradient 2D data)
		autophase	automatically adjust phase
		displaycf	stop at display of correction function
	 	fittedbaseline	use cubic spline baseline correction 
				defined by the choice of integral regions
 		invert		invert the corrected difference spectrum/spectra
		noaph		do not automatically adjust zero order phase 
				of reference region
		nodc		do not use dc correction of reference region
		nohilbert	do not use Hilbert transform algorithm; 
				use extrapolated dispersion mode reference 
				signal unless option
		noextrap	is also used
		normalise	keep the corrected spectrum integrals equal to 
				that of the first spectrum
		readcf		read correction function from file '<filename>';
				the argument 'filename' must immediately 
				follow 'readcf'
		satellites	use satellites defined in '<filename>' in ideal 
				reference region; '<filename>' should be in
				/vnmr/satellites
 		stop1		stop at display of experimental reference fid
 		stop2		stop at display of correction function
 		stop3		stop at display of corrected fid
 		stop4		stop at display of first corrected fid
 		verbose		display information about the course of the 
				processing in the main window
		writecf		write correction function tofile '<filename>';
				the argument 'filename' must immediately 
				follow 'writecf'
		writefid	write out corrected fid to '<filename>'; if
				'<filename>' does not begin with / it is assumed
				to be in the current working directory

References:

  FT   		J. Taquin, Rev. Physique App., 14 669 (1979).
    		G.A. Morris, JMR 80 547 (1988).
  Difference 	G.A. Morris & D. Cowburn, MRC 27 1085 (1989).
  Hilbert  	A. Gibbs & G.A. Morris JMR 91 77 (1991).
  General	G.A. Morris, H. Barjat and T.J. Horne, 
		Prog. NMR Spectrosc., in press (1977).

******************************************************************/

#include "group.h"
#include "tools.h"
#include "data.h"
#include "variables.h"
#include "vnmrsys.h"
#include "pvars.h"
#include "wjunk.h"
#include "fft.h"
#include "ftpar.h"
#include "displayops.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define ERROR 1
#define TRUE 1
#define FALSE 0
#define FTNORM 5.0e-7   /* this lot from ft2d.c */
#define CMPLX_DC  8
#define COMPLETE  0
#define APODIZE   8

static int np0,fn0;
static dfilehead phasehead,fidhead,writehead,cfhead,swaphead;
static dblockhead writebhead,cfbhead,swapbhead;
extern char *get_cwd();
extern int init_wt1(struct wtparams *wtpar, int fdimname);
extern int init_wt2(struct wtparams *wtpar, register float  *wtfunc,
             register int n, int rftflag, int fdimname, double fpmult, int 
rdwtflag);
extern int fnpower(int fni);
extern void weightfid(float *wtfunc, float *outp, int n, int rftflag, int 
dtype);


extern int interuption;
extern int start_from_ft;
struct wtparams wtp;

static double ct,refposfr,hzpp,sw,cr,rfl,rfp,delta,fn,rp,rp0,lp,sfrq;
static double arraydim,sw1,rpinc,rfl1,refintegral,firstrefint,rp2nd;
static int  print,npi,leftpos,rightpos,refpos,satposn,quantflag;
static int  dccorr,corfunc,noftflag,noift,apod;
static int  solvent,stopflag,baseline,hilbert,extrap,verbose;
static float *data1,*data2,*data3,*data4,*wtfunc;
static int  startno,finishno,stepno,incno;
static int  halffg,difffg,udifffg,firstfg,secondfg,nosub,invert;
static int  
/* GAM 3viii21 */
aphflg,altfg,oflag,flag2d,writefg,makereffg,ldcflag,readsinglecfflg,readcfflg,writecfflg;
static int writescalefg;
static float scaleVal;
static double *spg,*spy,*resets;
static int  *spx,*intbuf,numresets;
static float satpos[10],satint[10],satish[10];
static int  np0w,numsats,cfcount,count;
static char  *pathptr;
static char  writename[MAXPATH],writecfname[MAXPATH],readcfname[MAXPATH];
static FILE  *fidout = NULL,*writecffile,*readcffile;
static float odat[4]={
	180.0,-90.0,-180.0,90.0};
static double omega1,omega2,t1,t2,ph2,pi=3.141592654;
static float 	finalph,degtorad;
/* moved up from fiddle	*/
static	float   *inp,*p,*pp;
static	double   x1,x2,d2,d3,dg,spa,spb,spc,spd;
static	float   scale,max,phasetweek;
static	float   x;
static	double   tmp;
/* moved up from i_fiddle	*/
static	char path[MAXPATH],satname[MAXPATH],sysstr[2*MAXPATH];
static int r;
typedef	int	Boolean;	/* This is for the string comparison */
typedef char	*String;	/* This is for the string comparison */

Boolean equal ();		/* This is for the string comparison */
static int i_fiddle(int argc, char *argv[]);
int setupreadcf();
int getfidblockheadforwrite();
int getfidblockheadforcf();
int freall(int flag);
void readincf();
void setupwritefile();
void setupwritecf();
void writeoutresult();
void writeoutcf();
void printdata(float *data, int i1, int i2);
void faph(float *data, int i1, int i2, float *finalph);
void findratio(float *data, int i1, int i2, float *ratio);
void phase(float *data, float *tmpdata, int i1, int i2, float phch);
void transpmove(float *f, float *t);
void submem(float *t, float *f, int n);
void invertmem(float *t, int n);
void incrementrp();
void baseline_correct(float *inp, int leftpos, int rightpos);
void solventextract();
void extrapolate();
void fiddle_zeroimag();
void makeideal();

/*************************************

   fiddle()

**************************************/
int fiddle(int argc, char *argv[], int retc, char *retv[])
{
//	int    pwr,cblock,res,dc_correct=TRUE;
	int    pwr,cblock,res;
	register int i,ntval;
	dpointers  inblock;
	float   a,b,c,d,denom;
	int    ocount;

	/* initialization bits */
	if (i_fiddle(argc,argv))
		ABORT;

//	dc_correct=dccorr;
	pwr = fnpower(fn0);
	max=0.0;
	cfcount=0;
	count=0;
	firstrefint=0.0;
	phasetweek=0.0;
	degtorad=3.141592654/180.0;
	ocount=0;
	ntval = 1;
	if (!P_getreal(PROCESSED, "nt", &tmp, 1))
	{
		ntval = (int) (tmp + 0.5);
		if (ntval < 1)
			ntval = 1;
	}
	disp_status("IN3 ");

	/* check range of transforms */
	if (startno>=fidhead.nblocks) startno=fidhead.nblocks-1;
	if (startno<0) {
		startno=0; 
		finishno=fidhead.nblocks; 
		stepno=1;
	}
	if (finishno>fidhead.nblocks) finishno=fidhead.nblocks;
	if (stepno==0) stepno=1;

	/* setup destination fidfile and/or correction function file if requested 
*/
	if (writefg) setupwritefile();
	if (writecfflg) setupwritecf();

	/* start of main loop */
	incno=0;
	t1=0;
	rp0=rp;
	for (cblock = startno; cblock < finishno; cblock+=stepno)
	{
		if ( (res = D_getbuf(D_DATAFILE, fidhead.nblocks, cblock, &inblock)) )
		{
			D_error(res);
			freall(TRUE);
		}
		if ( (inblock.head->status & (S_DATA|S_SPEC|S_FLOAT|S_COMPLEX)) ==
		    (S_DATA|S_SPEC|S_FLOAT|S_COMPLEX) )
		{
			disp_index(cblock+1);
			incno+=1;
			if (verbose) Wscrprintf("Increment no. %d \n",incno);

			if (!aphflg) incrementrp();

			inp = (float *)inblock.data;
			/* DEBUGGING ONLY
			  if (verbose) Wscrprintf("Real part of 1st point of original fid 
%f \n",
			      inp[0]);
			  if (verbose) Wscrprintf("Imag part of 1st point of original fid 
%f \n",
			      inp[1]);
			*/
			/* dc (not normally used!) 
			if (dc_correct)
			  {
			  disp_status("DC ");
			  cmplx_dc(inp, &lvl_re, &lvl_im, &tlt_re, &tlt_im, np0/2, 
CMPLX_DC);
			  vvrramp(inp, 2, inp, 2, lvl_re, tlt_re, np0/2);
			  vvrramp(inp+1, 2, inp+1, 2, lvl_im, tlt_im, np0/2);
			  } */

			/* phase correct spectrum */
			finalph=0.0;
			rotate2(inp,np0/2,lp,rp);
/* GAM 27x15 */		if ((aphflg)&&(!readcfflg))
			{
				faph(inp,leftpos,rightpos,&finalph);
				rotate2(inp,np0/2,0.0,finalph);
			}

			/* if baseline then zero imag. */
			if (baseline||hilbert) fiddle_zeroimag();

			/* move first to data1, ready for ift, n.b. fn0==np0 */
			transpmove(inp,data1);

			/* do zeroing for reference region */
			if (solvent) solventextract();
			else
			{
				for (i=0;i<leftpos;i++)
				{
					inp[i]=0.0;
				}
				for (i=rightpos+2;i<np0;i++)
				{
					inp[i]=0.0;
				}

				if (ldcflag) baseline_correct(inp,leftpos,rightpos);
				if (extrap) extrapolate();
			} /* !solvent */
			transpmove(inp,data2);

			/* now do the ift's */
			if (!noift)
			{
				disp_status("IFT1 ");
			
	fft(data1,fn0/2,pwr,0,COMPLEX,COMPLEX,1.0,FTNORM/ntval,np0/2);
				disp_status("IFT2 ");
			
	fft(data2,fn0/2,pwr,0,COMPLEX,COMPLEX,1.0,FTNORM/ntval,np0/2);
			}
/* GAM 27x15 */ 	if (!readcfflg)
			{
			if (makereffg) makeideal();

			/* need to weight data3 to create ideal fid */
			if (makereffg&&!noift)
			{
				disp_status("WT ");
				weightfid(wtfunc,data3,np0w/2,FALSE,COMPLEX);
			}
/* GAM 27x15 */		}
			if (stopflag<1||stopflag>3)
			{
				/* DEBUGGING ONLY
				  if (verbose) Wscrprintf("Real part of 1st point of 
original fid %f \n",
				      data1[0]);
				  if (verbose) Wscrprintf("Imag part of 1st point of 
original fid %f \n",
				      data1[1]);
				*/
/* GAM 27x15 */ 		if (!readcfflg) 
				{
				/* divide (3) by (2) */
				disp_status("DIV ");
				for (i=0;i<npi;i+=2)
				{
					a=data3[i];
					b=data3[i+1];
					c=data2[i];
					d=data2[i+1];
					denom=c*c+d*d;
					data2[i]=(a*c+b*d)/denom;
					data2[i+1]=(b*c-a*d)/denom;
				}

/* GAM 27x15 */			for (i=npi;i<fn0;i+=1)
/* GAM 27x15 */                 {
/* GAM 27x15 */                        data2[i]=0.0;
/* GAM 27x15 */			}

				if (writecfflg) writeoutcf();
/* GAM 27x15 */			}
				if (readcfflg) readincf();

				/* and multiply by (1) */

				/* DEBUGGING ONLY	
				  if (verbose) Wscrprintf("Real part of 1st point of 
correction function %f \n",
				      data2[0]);
				  if (verbose) Wscrprintf("Imag part of 1st point of 
correction function %f \n",
				      data2[1]);
				*/
				disp_status("MUL ");
				if (!corfunc)
				{
					for (i=0;i<npi;i+=2)
					{
						a=data2[i];
						b=data2[i+1];
						c=data1[i];
						d=data1[i+1];
						inp[i]=(a*c-b*d);
						inp[i+1]=(b*c+a*d);
					}
				}
				/* DEBUGGING ONLY
				  if (verbose) Wscrprintf("Real part of 1st point of 
corrected fid %f \n",
				      inp[0]);
				  if (verbose) Wscrprintf("Imag part of 1st point of 
corrected fid %f \n",
				      inp[1]);
				*/
				/* Halve first point of corrected fid */
/* GAM 2viii21 */
/* Correct halving of first point before FT */
				if (halffg) /* default true */
				{
					inp[0]=0.5*inp[0];
					inp[1]=0.5*inp[1];
				}

				if (npi<np0)
					for (i=npi;i<np0;i++)
						inp[i]=0.0;
			} /* stopflag not 1 - 3 */

			if (firstfg&&!nosub)
				movmem((char *)inp,(char *)data4,sizeof(float)*np0,1,4);
			if (secondfg&&!nosub)
			{
				disp_status("SUB ");
				submem(inp,data4,np0);
				if (invert) invertmem(inp,np0);
			}

			/* inp contains result fid - write out and/or FT! */
			if (writefg&&!firstfg) writeoutresult();
			if (!noftflag)
			{
				disp_status("FT ");
				fft(inp,fn0/2,pwr,0,COMPLEX,COMPLEX,-1.0,FTNORM/ct,np0/2);
			}

			/* move intermediate result for display if requested */
			if (stopflag||corfunc)
			{
				disp_status("MOVE ");
				switch (stopflag)
				{
				case 1: 
					p=data1; 
					break;
				case 2: 
					p=data2; 
					break;
				case 4: 
					p=data4; 
					break;
				default:
					p=data3; 
					break;
				}
				if (corfunc) p=data2;
				if (difffg&&(stopflag<4))
				{
					if (firstfg) movmem((char *)p,(char
					*)data4,sizeof(float)*np0,1,4);
					if (!firstfg)
					{
						submem(p,data4,np0);
						if (invert) invertmem(p,np0);
					}
				}
				if (noift)
					transpmove(p,inp);
					else
					movmem((char *)p,(char *)inp,sizeof(float)*np0,1,4);
			}

			/* re-phase back to rp,lp */
			disp_status("PHASE ");
			if (!(writefg&&!firstfg)) /* if not both writing and subsequent 
fid */
			{
				rotate2(inp,np0/2,-lp,-rp);
			}
			else rotate2(inp,np0/2,-lp,0.0);

			if (difffg)
			{
				secondfg=!secondfg;
				firstfg=!firstfg;
			}
			if (udifffg)
			{
				secondfg=TRUE;
				firstfg=FALSE;
			}
			if (secondfg||oflag)
				makereffg=FALSE;
			else makereffg=TRUE;

			/* oflag increment */
			if (oflag)
			{
				rp+=odat[ocount];
				ocount++;
				if (ocount>3) ocount=0;
			}
			/* release result */
			if ( (res=D_markupdated(D_DATAFILE,cblock)) )
			{
				D_error(res);
				disp_status("  ");
				freall(TRUE);
			}
			if ( (res = D_release(D_DATAFILE, cblock)) )
			{
				D_error(res);
				disp_status("  ");
				freall(TRUE);
			}
			if (interuption) /* ? not fully working ? */
			{
				Werrprintf("Fiddle processing halted");
				freall(TRUE);
				ABORT;
			}

			if (flag2d)
			{
				if (altfg)
				{
					if ((incno % 2)==0) t1=t1+1/sw1;
				}
				else
				{
					t1=t1+1/sw1;
				}
			}
		}  /* end of if ( (inblock.head->status &c at start of main loop */
	}	/* end of main loop */
	start_from_ft=TRUE;
	releasevarlist();
	appendvarlist("cr");
	Wsetgraphicsdisplay("ds");
	/* free memory */
	freall(FALSE);

	disp_status("  ");
	disp_index(0);
	RETURN;
}

/* move AB -> BA */
void transpmove(float *f, float *t)
{
	movmem((char *)(f+np0/2),(char *)t,sizeof(float)*np0/2,1,4);
	movmem((char *)f,(char *)(t+np0/2),sizeof(float)*np0/2,1,4);
}

/* free all */
int freall(int flag)
{
	free(wtfunc);
	free(data1);
	free(data2);
	free(data3);
	if (difffg) free(data4);
	if (writefg||writecfflg||readcfflg) free(intbuf);
	if (fidout)
        {
           fclose(fidout);
           fidout = NULL;
        }
	if (writecfflg) fclose(writecffile);
	if (readcfflg) fclose(readcffile);
	if (solvent) { 
		free(resets); 
		free(spx); 
		free(spg); 
		free(spy);
	}
	if (flag) ABORT;
        RETURN;
}

void submem(float *t, float *f, int n)
{
	register int i;
	for (i=0;i<n;i++)
		*t++ -= *f++;
}

void invertmem(float *t, int n)
{
	register int i;
	for (i=0;i<n;i++)
		*t++*= -1;
}


/*---------------------------------------
|    				        |
|    i_fiddle()    			|
|          				|
+--------------------------------------*/
static int i_fiddle(int argc, char *argv[])
{
	short status_mask;
	int  i,j;
	double v,np;
	vInfo info;
	int  flag;
	FILE *satfile;

	int	intcount;

	char	nohilbert[] 		= "nohilbert";		/* These are to */
	char	noextrap[] 		= "noextrap";		/* define the */
	char	verbos[] 		= "verbose";		/* strings used */
	char	fittedbaseline[] 	= "fittedbaseline";	/* for the 	*/
	char	stop1[] 		= "stop1";		/* fiddle 	*/
	char	stop2[] 		= "stop2";		/* command line	*/
	char	stop3[] 		= "stop3";
	char	stop4[] 		= "stop4";
	char	displaycf[] 		= "displaycf";
	char	nodc[] 			= "nodc";
	char	inver[] 		= "invert";
	char	satellites[] 		= "satellites";
	char	writefid[]		= "writefid";
	char	normalise[]		= "normalise";
	char	alternate[]		= "alternate";
	char	noaph[]			= "noaph";
	char	readcf[]		= "readcf";
/* GAM 3viii21 */
	char	readsinglecf[]		= "readsinglecf";
	char	writecf[]		= "writecf";
	char	writescale[]   	        = "writescaledfid";


/* GAM 3viii21 */
	readsinglecfflg=readcfflg=writecfflg=corfunc=noift=noftflag=invert=solvent=FALSE;
	dccorr=baseline=verbose=numsats=FALSE;
	apod=udifffg=difffg=firstfg=secondfg=nosub=FALSE;
	stopflag=0;
	halffg=hilbert=makereffg=ldcflag=extrap=aphflg=TRUE;
	altfg=flag2d=writefg=oflag=quantflag=FALSE;
        writescalefg=FALSE;
        scaleVal=1.0;

	intcount=0;
	print=0;
	startno= -1;
	finishno=startno+1;
	stepno=1;
	sw1=0.0;
	rfl1=0.0;

	disp_status("IN");
	if ((P_getreal(PROCESSED, "ni", &v, 1) == 0&&(v>1.0))||
	    (P_getreal(PROCESSED, "ni2", &v, 1) == 0&&(v>1.0)) )
	{
		if (verbose) Wscrprintf("2D data\n");
		if 
(P_getreal(CURRENT,"sw1",&sw1,1)||P_getreal(CURRENT,"rfl1",&rfl1,1))
		{
			Werrprintf("Error accessing parameters\n");
			return(ERROR);
		}
	}


	if ((int)strlen(argv[0])>(int)6)
	{
		if (argv[0][6] == 'd')
		{
			Wscrprintf("Calculating FIDDLE'd difference between spectra n and n-1.\nds(1) for corrected 1st spectrum, ds(n) for difference.\n");
			difffg=TRUE;
			firstfg=TRUE;
		}
		if (argv[0][6] == 'u')
		{
			Wscrprintf("Calculating FIDDLE'd difference between spectra n and 1.\nds(1) for corrected 1st spectrum, ds(n) for difference \n");
			udifffg=TRUE;
			difffg=TRUE;
			firstfg=TRUE;
		}
		if (argv[0][6] == '2')
		{
			Wscrprintf("2D FIDDLE processing\n");
			flag2d=TRUE;
			quantflag=TRUE;
			extrap=FALSE;
			if (argv[0][8] == 'd')
			{
				Wscrprintf("... taking the difference between alternate fids\n");
				firstfg=TRUE;
				difffg=TRUE;
			}
		}
	}


	if (argc>1)		/* This is to check for the options used */
	{
		for (i=1;i<argc;i++)
		{
			if (atoi(argv[i])>0)
			{
				intcount++;
				if (intcount==1)
				{
					startno=atoi(argv[i])-1; 
					finishno=startno+1; 
					stepno=1;
				}
				if (intcount==2)
				{
					finishno=atoi(argv[i]); 
					stepno=1;
				}
				if (intcount==3)
				{
					stepno=atoi(argv[i]);
				}
			}
			else	if (equal(argv[i],nohilbert))
			{
				if (verbose) Wscrprintf("Not using Hilbert algorithm \n");
				hilbert=FALSE; 
				extrap=TRUE;
			}
			else if (equal(argv[i],noextrap))
			{
				if (verbose) Wscrprintf("Not using extrapolation of dispersion mode reference signal \n");
				extrap=FALSE;
			}
			else if (equal(argv[i],verbos))
			{
				verbose=TRUE;
				Wscrprintf("Verbose: displaying information during FIDDLE.\n");
			}
			else if (equal(argv[i],fittedbaseline))
			{
				if (verbose) Wscrprintf("Fitted baseline used.\n");
				baseline=TRUE;
			}
			else if (equal(argv[i],stop1))
			{
				if (verbose) Wscrprintf("Display experimental reference fid.\n");
				stopflag=1; 
				noftflag=TRUE;
			}
			else if (equal(argv[i],stop2))
			{
				if (verbose) Wscrprintf("Display correction function.\n");
				stopflag=2; 
				noftflag=TRUE;
			}
			else if (equal(argv[i],stop3))
			{
				if (verbose) Wscrprintf("Display ideal reference fid.\n");
				stopflag=3; 
				noftflag=TRUE;
			}
			else if (equal(argv[i],stop4))
			{
				if (verbose) Wscrprintf("Display first corrected fid.\n");
				stopflag=4;
			}
			else if (equal(argv[i],alternate))
			{
				if (verbose) Wscrprintf("Alternating phases of increments for hypercomplex 2D\n");
				altfg=TRUE;
			}
			else if (equal(argv[i],displaycf))
			{
				if (verbose) Wscrprintf("Display correction function in place of corrected spectrum\n");
				corfunc=TRUE; 
				noftflag=TRUE;
			}
			else if (equal(argv[i],nodc))
			{
				if (verbose) Wscrprintf("No dc correction of reference region \n");
				ldcflag=FALSE;
			}
			else if (equal(argv[i],inver))
			{
				if (verbose) Wscrprintf("Corrected difference spectrum will be inverted \n");
				invert=TRUE;
			}
			else if (equal(argv[i],satellites))
			{
				if (verbose) Wscrprintf("Satellites will be included in the ideal reference \n");
				numsats=TRUE;
				i++;
				if ((argc-1)<i)
				{
					Werrprintf("Satellite filename not specified");
					return(ERROR);
				}
				sprintf(satname,"/vnmr/satellites/%s",argv[i]);
				if (verbose) Wscrprintf("Satellite file name is %s \n",argv[i]);
			}
			else if (equal(argv[i],writefid) || equal(argv[i],writescale))
			{
				writefg=TRUE;
                                if (equal(argv[i],writescale))
                                {
                                   writescalefg=TRUE;
				   if (verbose)
                                     Wscrprintf("Scaled corrected fid will be written to disk \n");
                                }
                                else
                                {
				   if (verbose)
                                     Wscrprintf("Corrected fid will be written to disk \n");
                                }
				i++;
				if ((argc-1)<i)
				{
					Werrprintf("Destination filename not specified\n");
					return(ERROR);
				}
				if ( writescalefg && ((argc-1)<i+1) )
				{
					Werrprintf("FID scaling factor not provided\n");
					return(ERROR);
				}
				strcpy(writename,argv[i]);
				if (argv[i][0]!='/')
				{
					pathptr = get_cwd();
					strcpy(writename,pathptr);
					strcat(writename,"/");
					strcat(writename,argv[i]);
				}
				if
				(((int) 										
	(strlen(writename))<5)||(strcmp(&writename[strlen(writename)-4],".fid")))
				    strcat(writename,".fid");
				sprintf(sysstr,"mkdir \"%s\"",writename); /* DI 10viii21 */
				if(system(sysstr))
				{
					sprintf(sysstr,"cd \"%s\"",writename); /* DI 10viii21 */
					if(system(sysstr))
					{
						Werrprintf("System error: %s",sysstr);
						return(ERROR);
					}
				}
				Wscrprintf("Writing corrected fid out; destination file: %s\n",writename);
				if ( writescalefg )
				{
                                   i++;
				   scaleVal = atof(argv[i]);
                                   if (scaleVal == 0.0)
				   {
				      Werrprintf("FID scaling factor not provided\n");
						return(ERROR);
				   }
		                   if (verbose)
                                      Wscrprintf("FID scaling factor %f\n",scaleVal);
				}
			}
			else if (equal(argv[i],writecf))
			{
				if (verbose) Wscrprintf("Correction function will be written to disk \n");
				writecfflg=TRUE;
				i++;
				if ((argc-1)<i)
				{
					Werrprintf("Destination filename not specified\n");
					return(ERROR);
				}
				strcpy(writecfname,argv[i]);
				if (argv[i][0]!='/')
				{
					pathptr = get_cwd();
					strcpy(writecfname,pathptr);
					strcat(writecfname,"/");
					strcat(writecfname,argv[i]);
				}
				if
				(((int) 										
	(strlen(writecfname))<5)||(strcmp(&writecfname[strlen(writecfname)-4],
				    ".fid")))
				    strcat(writecfname,".fid");
				sprintf(sysstr,"mkdir \"%s\"",writecfname); /* GAM/DI 10viii21 */
				if(system(sysstr))
				{
					sprintf(sysstr,"cd \"%s\"",writecfname); /* GAM/DI 10viii21 */
					if(system(sysstr))
					{
						Werrprintf("System error: %s",sysstr);
						return(ERROR);
					}
				}
/* GAM 27x15 */			if (verbose) Wscrprintf("Writing correction function out; destination file: %s\n",writecfname);
			}
/* GAM 3viii21 */
			else if (equal(argv[i],readcf)||equal(argv[i],readsinglecf))
			{
				if (verbose) Wscrprintf("Correction function will be read from disk \n");
				readcfflg=TRUE;
				if (equal(argv[i],readsinglecf)) readsinglecfflg=TRUE;
				i++;
				if ((argc-1)<i)
				{
					Werrprintf("Source filename not specified\n");
					return(ERROR);
				}
				strcpy(readcfname,argv[i]);
				if (argv[i][0]!='/')
				{
					pathptr = get_cwd();
					strcpy(readcfname,pathptr);
					strcat(readcfname,"/");
					strcat(readcfname,argv[i]);
				}
				if
				(((int) 										
	(strlen(readcfname))<5)||(strcmp(&readcfname[strlen(readcfname)-4],
				    ".fid")))
				   strcat(readcfname,".fid");
                                sprintf(sysstr,"cd \"%s\"",readcfname); /* GAM/DI 10viii21 */
				if(system(sysstr))
				{
					Werrprintf("System error: %s",sysstr);
					return(ERROR);
				}

/* GAM 27x15 */                 if (verbose) Wscrprintf("Reading correction function in; source file: %s\n",readcfname);
			}
			else if (equal(argv[i],noaph))
			{
				if (verbose) Wscrprintf("Reference region(s) will not be automatically phased\n");

				aphflg=FALSE;
			}
			else if (equal(argv[i],normalise))
			{
				if (verbose) Wscrprintf("Corrected spectra will be normalised to 1st spectrum\n");

				quantflag=TRUE;
			}

			else {
				Werrprintf("Option %s was not recognised ",argv[i]);
			}
		}
	}

	if (verbose&&numsats) printf("%s\n",satname);
	if (noift&&!stopflag) stopflag=2;
	if (stopflag==4&&!difffg) stopflag=3;
	if (difffg) finishno++;
/* GAM 4viii21 */
	if (readsinglecfflg) aphflg = FALSE; /* No need to adjust reference region phase since reference region not used */

	D_allrelease();


	/* get parameters */
	if  (
	    P_getreal(PROCESSED,"fn",&fn,1)||
	    P_getreal(CURRENT,"ct",&ct,1)||
	    P_getreal(CURRENT,"sw",&sw,1)||
	    P_getreal(CURRENT,"cr",&cr,1)||
	    P_getreal(CURRENT,"delta",&delta,1)||
	    P_getreal(CURRENT,"rfl",&rfl,1)||
	    P_getreal(CURRENT,"rfp",&rfp,1)||
	    P_getreal(CURRENT,"rp",&rp,1)||
	    P_getreal(CURRENT,"lp",&lp,1)||
	    P_getreal(PROCESSED,"sfrq",&sfrq,1)||
	    P_getreal(PROCESSED,"np",&np,1)
	    )
	{
		Werrprintf("Error accessing parameters\n");
		return(ERROR);
	}

	if (ct==0)
	{
		ct=1;
	    if (verbose)
		   Werrprintf("ct was zero; assumed to be 1\n");
	    P_setreal(CURRENT,"ct",1.0,0);
	    P_setreal(PROCESSED,"ct",1.0,0);
	}
	if (verbose)
		Wscrprintf("sw %lf, cr %lf, delta %lf, rfl %lf, rfp %lf, rp %lf, lp %lf\n",
		    sw,cr,delta,rfl,rfp,rp,lp);
/* GAM 27x15 */	if (((cr<rfp)||(cr-delta>rfp))&&!readcfflg)
	{
		Werrprintf("Please place cursors either side of reference and try again\n");
		return(ERROR);
	}
	if (!aphflg)
	{
		if (flag2d)
		{
			if (P_getreal(CURRENT,"phinc",&rpinc,1))
			{
				/*  parameter phinc not created, so set phase increment to zero	*/
				rpinc=0.0;
			}
			if (P_getreal(CURRENT,"rp2nd",&rp2nd,1))
			{
				rp2nd=rp;
			}
		}
	}

	npi= (int)np;
	np0 = (int)fn;
	fn0 = np0;    /* ! */
/* GAM 27x15 */	if (hilbert&&((int)np>fn0/2))
	{
		if (verbose) Wscrprintf("np = %f ",np);
		if (verbose) Wscrprintf("fn = %f\n",fn);
/* GAM 27x15 */		Werrprintf("Data should be zerofilled so that fn>=2*np for hilbert transform");
	}
	np0w=np0;
/* GAM 27x15 */	if (hilbert) npi=fn0/2; 
	if (hilbert) np0w=fn0/2; 

	/* calculate rest of params */
	hzpp=sw/np0*2;
	refpos=2*(int)((sw-rfl)/hzpp);
	refposfr=((sw-rfl)/hzpp)-0.5*(double)refpos;
	leftpos=refpos-2*(int)((cr-rfp)/hzpp);
	rightpos=leftpos+2*(int)(delta/hzpp);
	if (verbose) Wscrprintf(" refposfr %g, hzpp %g, left %d, ref %d, right %d\n",
	    refposfr,hzpp,leftpos,refpos,rightpos);

	/* read satellite file if used */
	if (numsats)
	{
		if ((satfile=fopen(satname,"r"))==NULL)
		{
			Werrprintf("Error opening satellite file\n");
			return(ERROR);
		}
		numsats=0;
		while(fscanf(satfile,"%f %f %f\n",&satpos[numsats],
		    &satint[numsats],&satish[numsats])!=EOF)
		{
			if (verbose) printf("%f %f %f\n",
			    satpos[numsats],satint[numsats],satish[numsats]);
			numsats++;
		}
		fclose(satfile);
	}

	/* access & check the file containing the original spectrum */
	if ( (r = D_gethead(D_DATAFILE, &fidhead)) )
	{
		if (r == D_NOTOPEN)
		{
			if (verbose) Wscrprintf("Original Spectrum had to be re-opened \n");
			strcpy(path, curexpdir);
			strcat(path, "/datdir/data");
			r = D_open(D_DATAFILE, path, &fidhead); /* open the file */
		}

		if (r)
		{
			D_error(r);
			return(ERROR);
		}
	}
	status_mask = (S_DATA|S_SPEC|S_FLOAT|S_COMPLEX);
	if ( (fidhead.status & status_mask) != status_mask )
	{
		Werrprintf("No spectrum in file: use ft before fiddle. fidhead.status = %d", fidhead.status);
		return(ERROR);
	}
	if (np0 != fidhead.np)
	{
		Werrprintf("Size of data inconsistent, np0 = %d, head.np = %d",
		    np0, fidhead.np);
		return(ERROR);
	}

	if(writefg) getfidblockheadforwrite();
	if(writecfflg) getfidblockheadforcf();

	/* Set PHASFILE status to !S_DATA - this is required to
	 force a recalculation of the display from the new data
	 in DATAFILE (in the ds routine, see proc2d.c)   */
	if ( (r=D_gethead(D_PHASFILE,&phasehead)) )
	{
		if (r==D_NOTOPEN)
		{
			//Wscrprintf("phas NOTOPEN\n"); /* DI 10viii21 */
			strcpy(path,curexpdir);
			strcat(path,"/datdir/phasefile");
			r=D_open(D_PHASFILE,path,&phasehead);
		}
		if (r)
		{
			D_error(r);
			return(ERROR);
		}
	}
	phasehead.status=0;
	if ( (r=D_updatehead(D_PHASFILE,&phasehead)) )
	{
		D_error(r);
		Wscrprintf("PHAS updatehead\n");
		return(ERROR);
	}

	/* allocate memory for the temp buffers & weight func. */
	if ( (data1=(float *)malloc(sizeof(float)*fn0))==0
	    ||(data2=(float *)malloc(sizeof(float)*fn0))==0
	    ||(data3=(float *)malloc(sizeof(float)*fn0))==0
	    ||(wtfunc=(float *)malloc(sizeof(float)*np0/2))==0)
	{
		Werrprintf("could not allocate memory\n");
		difffg=FALSE;
		freall(TRUE);
	}
	if (difffg)
	{
		if ((data4=(float *)malloc(sizeof(float)*fn0))==0)
		{
			Werrprintf("could not allocate memory\n");
			freall(TRUE);
		}
	}
	if (writefg||writecfflg||readcfflg)
	{
		if ((intbuf=(int *)malloc(sizeof(float)*fn0))==0)
		{
			Werrprintf("could not allocate memory\n");
			freall(TRUE);
		}
	}
	if (readcfflg) setupreadcf();

	/* setup weighting function for ideal fid */
	if (init_wt1(&wtp,S_NP))
	{
		Wscrprintf("ERROR: init_wt1(S_NP)\n");
		freall(TRUE);
	}
	if (init_wt2(&wtp,wtfunc,np0/2,FALSE,S_NP,1.0,FALSE))
	{
		Wscrprintf("ERROR: init_wt2()\n");
		freall(TRUE);
	}

	/* setup for solvent rd */
	if (solvent)
	{
		if (P_getVarInfo(CURRENT,"lifrq",&info))
		{
			Werrprintf("Error accessing resets");
			freall(TRUE);
		}
		numresets=(int)info.size;
		if (numresets<=1)
		{
			Werrprintf("Please provide some integral resets");
			freall(TRUE);
		}
		if ((resets=(double *)malloc(sizeof(double)*(numresets+4)))==0
		    ||(spy=(double *)malloc(sizeof(double)*(numresets+4)))==0
		    ||(spx=(int *)malloc(sizeof(int)*(numresets+4)))==0
		    ||(spg=(double *)malloc(sizeof(double)*(numresets+4)))==0 )
		{
			Werrprintf("error allocating memory");
			freall(TRUE);
		}
		i=1;
		flag=0;
		for (j=1;j<numresets;j++)
		{
			if (P_getreal(CURRENT,"lifrq",&resets[i],j))
			{
				Werrprintf("Error accessing lifrq(%d)\n",i);
				freall(TRUE);
			}
			if (resets[i]<(rfl-rfp+cr)&&flag==0)
			{
				resets[i+1]=resets[i];
				resets[i]=rfl-rfp+cr;
				flag=1;
				i++;
			}
			if (resets[i]<rfl&&flag==1)
			{
				resets[i+1]=resets[i];
				resets[i]=rfl;
				flag=2;
				i++;
			}
			if (resets[i]<(rfl-rfp+cr-delta)&&flag==2)
			{
				resets[i+1]=resets[i];
				resets[i]=rfl-rfp+cr-delta;
				flag=3;
				i++;
			}
			/*
				if (resets[i]>(rfl-rfp+cr-delta)&&flag==TRUE)
					i--;
			*/
			if (resets[i]==resets[i-1])
				i--;
			i++;
		}
		resets[0]=sw;
		numresets=i;
		resets[numresets]=hzpp;
	}

	disp_status(" ");
	return(COMPLETE);
}




Boolean equal (stringa, stringb)	/* This is for string comparison */
String stringa, stringb;
{
	while (*stringa == *stringb && *stringa != '\0')
		stringa++, stringb++;

	if (*stringa == '\0' && *stringb == '\0')
		return (TRUE);
		else
		return (FALSE);
}






void faph(float *data, int i1, int i2, float *finalph)
{
	float oldbestphase,bestphase,maxratio;
	int j;
	float phch,ratio;
	for (j=i1;j<=i2;j+=2)
	{
		data2[j]=data[j];
		data2[j+1]=data[j+1];
	}
	bestphase=0.0;
	maxratio=0.0;
	for (phch=0.0; phch<359.0; phch+=10.0)
	{
		phase(data,data2,i1,i2,phch);
		baseline_correct(data2,i1,i2);
		findratio(data2,i1,i2,&ratio);
		if (print==1)	printf(" phase and ratio : %f %f \n ",phch,ratio);
		if (ratio>maxratio)
		{
			maxratio=ratio;
			bestphase=phch;
		}
	}
	oldbestphase=bestphase;
	ratio=0.0;
	for (phch=oldbestphase-10.0; phch<=oldbestphase+10.0; phch+=1.0)
	{
		phase(data,data2,i1,i2,phch);
		baseline_correct(data2,i1,i2);
		findratio(data2,i1,i2,&ratio);
		if (print==1)	printf(" phase and ratio : %f %f \n ",phch,ratio);
		if (ratio>maxratio)
		{
			maxratio=ratio;
			bestphase=phch;
		}
	}
	oldbestphase=bestphase;
	ratio=0.0;
	for (phch=oldbestphase-1.0; phch<=oldbestphase+1.0; phch+=0.1)
	{
		phase(data,data2,i1,i2,phch);
		baseline_correct(data2,i1,i2);
		findratio(data2,i1,i2,&ratio);
		if (print==1)	printf(" phase and ratio : %f %f \n ",phch,ratio);
		if (ratio>maxratio)
		{
			maxratio=ratio;
			bestphase=phch;
		}
	}
	if (verbose) printf(" Best phase : %f \n\n ",bestphase);
	ratio=maxratio;
	*finalph=bestphase;
	return;
}



void findratio(float *data, int i1, int i2, float *ratio)
{
	int j;
	float pos,neg;
	pos=0.0;
	neg=-0.00000001;
	for (j=i1;j<=i2;j+=2)
	{
		if (data[j]>0) pos+=data[j];
		if (data[j]<0) neg+=data[j];
	}
	*ratio=-pos/neg;
	return;
}

void phase(float *data, float *tmpdata, int i1, int i2, float phch)
{
	int j;
	float rephch,imphch;
	rephch=(float) cos((double) (phch*degtorad));
	imphch= -sin((double) (phch*degtorad));
	for (j=i1;j<=i2;j+=2)
	{
		tmpdata[j]=data[j]*rephch-imphch*data[j+1];
		tmpdata[j+1]=data[j]*imphch+rephch*data[j+1];
	}
	if (print==1) printf("\n \n Phased spectrum with baseline \n\n ");
	if (print==1) printdata(tmpdata,i1,i2);
	return;
}


void baseline_correct(float *inp, int leftpos, int rightpos)
{
	float npdc, nprr, ldcr, rdcr, a1, a2, b1, b2;
	int i,npdci;

	nprr=(float)(rightpos-leftpos+1);
	npdc=nprr/20;     /* Average 1st and last tenth */
	npdci=(int) npdc;
	if (npdci<1) npdci=1;   /* Actual number of points to use for one tenth */
	npdc=(float) npdci;
	ldcr=0.0;
	for (i=leftpos;i<=leftpos+2*npdci;i+=2)
	{
		ldcr+=inp[i];
	}
	ldcr=ldcr/(npdc+1.0);
	rdcr=0.0;
	for (i=rightpos-2*npdci;i<=rightpos;i+=2)
	{
		rdcr+=inp[i];
	}
	rdcr=rdcr/(1.0+npdc);
	b1=ldcr;
	b2=rdcr;
	a1=(float)leftpos+npdc-1;
	a2=(float)rightpos-npdc+1;
	for (i=leftpos;i<=rightpos;i+=2)
	{
		inp[i]-=b1+(b2-b1)*(((float)i-a1)/(a2-a1));
	}
}

void printdata(float *data, int i1, int i2)
{
	int j;
	for (j=i1;j<=i2;j+=2)
	{
		printf(" %f %f \n ",data[j],data[j+1]);
	}
}

void setupwritefile()
{
	if ((fidout=fopen(writename,"w+"))==0)
	{
		Werrprintf("cannot open output fid file\n");
		freall(TRUE);
	}
	/* tweak file header */
	writehead.nblocks=((finishno-startno)/stepno);
	if (difffg) writehead.nblocks/=2;
	writehead.np=np0w;
	writehead.ebytes=4; /* should also allow 16 bit ! */
	writehead.tbytes=writehead.ebytes*writehead.np;
	writehead.bbytes=writehead.tbytes+sizeof(dblockhead);
	/* ntraces and nbheaders should be 1 */
	writehead.ntraces=1;
	writehead.nbheaders=1;
	writehead.status= S_DATA | S_FLOAT | S_COMPLEX; /* see ebytes ! */
	/* make up blockheader */
	writebhead.scale=(short)0;
	writebhead.status=S_DATA | S_FLOAT | S_COMPLEX;
	writebhead.mode=(short)1; /* mode ? */
	writebhead.lpval=0.0;
	writebhead.rpval=0.0;
	writebhead.lvl=0.0;
	writebhead.tlt=0.0;
	writebhead.ctcount=(int)ct; /* should be ct */
#ifdef LINUX
        movmem((char *)(&writehead), (char *) (&swaphead),
                        sizeof(dfilehead), 1, 1);
        DATAFILEHEADER_CONVERT_HTON(&swaphead);
	fwrite((char *)&(swaphead),sizeof(dfilehead),1,fidout);
#else
	fwrite((char *)&(writehead),sizeof(dfilehead),1,fidout);
#endif
}

void setupwritecf()
{
	if ((writecffile=fopen(writecfname,"w+"))==0)
	{
/* GAM 28vii21 */
		Werrprintf("cannot open correction function file\n");
		freall(TRUE);
	}
	/* tweak file header */
	cfhead.nblocks=((finishno-startno)/stepno);
	if (difffg) cfhead.nblocks/=2;
	cfhead.np=np0w;
	cfhead.ebytes=4; /* should also allow 16 bit ! */
	cfhead.tbytes=cfhead.ebytes*cfhead.np;
	cfhead.bbytes=cfhead.tbytes+sizeof(dblockhead);
	/* ntraces and nbheaders should be 1 */
	cfhead.ntraces=1;
	cfhead.nbheaders=1;
	cfhead.status= S_DATA | S_FLOAT | S_COMPLEX; /* see ebytes ! */
	/* make up blockheader */
	cfbhead.scale=(short)0;
	cfbhead.status=S_DATA | S_FLOAT | S_COMPLEX;
	cfbhead.mode=(short)1; /* mode ? */
	cfbhead.lpval=0.0;
	cfbhead.rpval=0.0;
	cfbhead.lvl=0.0;
	cfbhead.tlt=0.0;
	cfbhead.ctcount=(int)ct; /* should be ct */
#ifdef LINUX
        movmem((char *)(&cfhead), (char *) (&swaphead),
                        sizeof(dfilehead), 1, 1);
        DATAFILEHEADER_CONVERT_HTON(&swaphead);
        fwrite((char *)&(swaphead),sizeof(dfilehead),1,writecffile);
#else
	fwrite((char *)&(cfhead),sizeof(dfilehead),1,writecffile);
#endif
}

void writeoutresult()
{
   register int i;
   union u_tag {
      float fval;
      int   ival;
   } uval;

	writebhead.index=(short)count;
#ifdef LINUX
        movmem((char *)(&writebhead), (char *) (&swapbhead),
                        sizeof(dblockhead), 1, 1);
        DATABLOCKHEADER_CONVERT_HTON(&swapbhead);
        fwrite((char *)&(swapbhead),sizeof(dblockhead),1,fidout);;
#else
	fwrite((char *)&(writebhead),sizeof(dblockhead),1,fidout);
#endif
	/* no lp correction on fid data! */
	if (!flag2d) rotate2(inp,np0w/2,0.0,-rp);
	disp_status("WRITE ");
    if (writescalefg)
    {
        max=0.0;
		for (i=0;i<np0w;i++)
			if (fabs(inp[i])>max) max=fabs(inp[i]);
		if (verbose) Wscrprintf("Maximum corrected fid signal %f\n",max);
        scaleVal=scaleVal/max;
        writescalefg = FALSE;
    }
#ifndef LINUX
    scale=1.0;
    
	if (count==0)
	{
		for (i=0;i<np0w;i++)
			if (fabs(inp[i])>max) max=fabs(inp[i]);
		if (verbose) Wscrprintf("Maximum corrected fid signal %f\n",max);
		if (max>2e9) /* 2e9 = 2^31, 32bit max */
		{
			scale=1e9/max;
			Wscrprintf("WARNING: Corrected fid scaled down to fit 32 bits.\n");
		}
	}
#endif
	if (verbose) Wscrprintf("FID scaling factor %f\n",scaleVal);
	for (i=0;i<np0w;i++)
    {
#ifdef LINUX
        uval.fval = inp[i]*scaleVal;
        intbuf[i]=ntohl(uval.ival);
#else
		intbuf[i]=(int)(scale*inp[i]);
#endif
    }

	fwrite(intbuf,4,np0w,fidout);
	count++;
}

void writeoutcf()

{
	register int i;
   union u_tag {
      float fval;
      int   ival;
   } uval;
	cfbhead.index=(short)cfcount;
#ifdef LINUX
        movmem((char *)(&cfbhead), (char *) (&swapbhead),
                        sizeof(dblockhead), 1, 1);
        DATABLOCKHEADER_CONVERT_HTON(&swapbhead);
        fwrite((char *)&(swapbhead),sizeof(dblockhead),1,writecffile);
#else
	fwrite((char *)&(cfbhead),sizeof(dblockhead),1,writecffile);
#endif
	disp_status("WRITE CF");
/* GAM 30vii21 */
/* Scale correction function up by 10^6 so that it displays with sensible vf, then scale down again when read in */
	scale=1000000.0;
/* GAM 2viii21 */
/* Apply the autophase correction found for the reference region ... */
	rotate2(data2,np0/2,0.0,finalph);
/* Remove scaling now saving as floating point */
	for (i=0;i<np0w;i++)
/* GAM 30vii21 */
	{
	#ifdef LINUX
        uval.fval = data2[i]*scale;
        intbuf[i]=ntohl(uval.ival);
#else
	intbuf[i]=(int)(scale*data2[i]);
#endif
	}
	fwrite(intbuf,4,np0w,writecffile); 
/* ... then reverse */
	rotate2(data2,np0/2,0.0,-finalph);
	cfcount++;
}

void incrementrp()
/* increment rp if appropriate */
{
	if (flag2d)
	{
		if (!altfg) rp=rp0+(incno-1)*rpinc;
		if (altfg)
		{
			if (((incno/2)*2)!=incno) rp=rp0+((float) (incno-1))*rpinc/2.0;
			/* if odd increment, add phinc ... */
			if (((incno/2)*2)==incno) rp=rp2nd-((float) (incno-2))*rpinc/2.0;
			/* ... if even, subtract */
		}
	}
	if (verbose) Wscrprintf("rp %f \n",rp);
}

void solventextract()
{
	register int i,j;
	disp_status("SPLINE");
	movmem((char *)(&inp[leftpos]),(char
	*)data2,sizeof(float)*(rightpos-leftpos+2),1,4);
	for (i=0;i<=numresets;i++)
	{
		spx[i]=2*(int)((sw-resets[i])/hzpp);
		spy[i]=(double)inp[spx[i]];
	}
	for (i=1;i<numresets;i++)
	{
		spg[i]=tan(.5*(atan((spy[i]-spy[i-1])/(double)(spx[i]-spx[i-1]))+
		    atan((spy[i+1]-spy[i])/(double)(spx[i+1]-spx[i]))));
	}
	spg[0]=tan(2.0*atan(spg[1])-atan(spg[2]));
	spg[numresets]=tan(2.0*atan(spg[numresets-1])-atan(spg[numresets-2]));
	if (verbose) for (i=0;i<=numresets;i++) printf("%d: r=%f,x=%d,y=%f,g=%f\n",
	    i,resets[i],spx[i],spy[i],spg[i]);
	pp=inp;
	for (i=0;i<numresets;i++)
	{
		x1=(double)spx[i];
		x2=(double)spx[i+1];
		x2=x2-x1;
		x1=0.0;
		d2=x2*x2;
		d3=d2*x2;
		dg=spg[i+1]-spg[i];
		spd=dg/d2-2.0*(spy[i+1]-spy[i]-spg[i]*x2)/d3;
		spc=(dg-3.0*spd*d2)/(2.0*x2);
		spb=spg[i];
		spa=spy[i];
		for (j=0;j<(int)x2;j+=2)
		{
			tmp=(double)j;
			pp[j]=(float)(spa+tmp*(spb+tmp*(spc+tmp*spd)));
			pp[j+1]=0.0;
		}
		pp+=(int)x2;
	}
	movmem((char *)data2,(char
	*)(&inp[leftpos]),sizeof(float)*(rightpos-leftpos+2),1,4);
	for (i=leftpos+1;i<rightpos+2;i+=2)
	{
		inp[i]=0.0;
	}
}

void extrapolate()
{
	register int i;
	x=inp[rightpos+1]*(rightpos-refpos);
	for (i=(rightpos-refpos);i<(np0-refpos);i+=2)
	{
		inp[refpos+i+1]=x/i;
	}
	x=inp[leftpos+1]*(refpos-leftpos);
	for (i=(refpos-leftpos+2);i<refpos;i+=2)
	{
		inp[refpos-i+1]=x/i;
	}
}

void fiddle_zeroimag()
{
	register int i;
	disp_status("ZERO ");
	if (verbose) Wscrprintf("zeroing all imag.\n");
	for (i=1;i<np0;i+=2)
		inp[i]=0.0;
}

void makeideal()
{
	register int i,j;
	disp_status("IDEAL ");
	/* calculate reference centreband signal ... */
	refintegral=sqrt(data2[0]*data2[0]+data2[1]*data2[1]);
	if (firstrefint==0.0)
	{
		firstrefint=refintegral;
	}
	if (quantflag)
	{
		refintegral=firstrefint;
	}
	t2=0.0;
	omega2=0.5*sw-rfl;
	omega1=-0.5*sw1+rfl1;
	if (altfg)
	{
		if ((incno % 2)==1) omega1=-omega1;
	}
	if (verbose) Wscrprintf("omega1 %f \n",omega1);
	if (verbose) Wscrprintf("omega2 %f \n",omega2);
	for (i=0; i<np0/2; i+=2)
	{
		ph2=2.0*pi*(omega2*t2+omega1*t1);
		data3[i]=refintegral*cos(ph2);
		data3[i+1]=refintegral*sin(ph2);
		t2+=1/sw;
	}
	/* ... and add satellites */
	for (j=0; j<numsats; j++)
	{
		omega2=0.5*sw-rfl+satish[j]*sfrq+satpos[j];
		omega1=-(0.5*sw1-rfl1+satish[j]*sfrq+satpos[j]);
		if (altfg)
		{
			if ((incno % 2)==1) omega1=-omega1;
		}
		t2=0.0;
		satposn=2*(int)((sw-rfl+satpos[j]+satish[j]*sfrq)/hzpp);
		if (satposn<=rightpos)
		{
			for (i=0; i<np0/2; i+=2)
			{
				ph2=2.0*pi*(omega2*t2+omega1*t1);
				data3[i]+=satint[j]*refintegral*cos(ph2);
				data3[i+1]+=satint[j]*refintegral*sin(ph2);
				t2+=1/sw;
			}
		}
		omega2=0.5*sw-rfl+satish[j]*sfrq-satpos[j];
		omega1=-(0.5*sw1-rfl1+satish[j]*sfrq-satpos[j]);
		if (altfg)
		{
			if ((incno % 2)==1) omega1=-omega1;
		}
		t2=0.0;
		satposn=2*(int)((sw-rfl-satpos[j]+satish[j]*sfrq)/hzpp);
		if (satposn>=leftpos)
		{
			for (i=0; i<np0/2; i+=2)
			{
				ph2=2.0*pi*(omega2*t2+omega1*t1);
				data3[i]+=satint[j]*refintegral*cos(ph2);
				data3[i+1]+=satint[j]*refintegral*sin(ph2);
				t2+=1/sw;
			}
		}
	}
	if (verbose) Wscrprintf("1st pt of ideal fn %f \n",data3[0]);
	if (verbose) Wscrprintf("2nd pt of ideal fn %f \n",data3[1]);
}

int getfidblockheadforwrite()
{
	/* get fid blockhead for writefid option */
	if ( (r = D_gethead(D_USERFILE,&writehead)) )
	{
		if (r == D_NOTOPEN)
		{
			if (verbose) Wscrprintf("reopening fid header\n");
			strcpy(path,curexpdir);
			strcat(path,"/acqfil/fid");
			r = D_open(D_USERFILE, path, &writehead);
		}
		if (r)
		{
			if (verbose) Wscrprintf("No way, it's the reopen bit!\n");
			D_error(r);
			return(ERROR);
		}
	}
	sprintf(sysstr,"cp %s/text \"%s\"",curexpdir,writename); /* DI 10viii21 */
	if (system(sysstr))
	{
		Werrprintf("could not copy text from file?\n");
	}
	sprintf(sysstr,"%s/procpar",writename);
	if ( (r = P_copy(CURRENT,TEMPORARY)) ) {
		Wscrprintf("P_copy error\n");
		return(ERROR);
	}
	if ( (r = P_setreal(TEMPORARY,"np",(double)np0w,0)) )
		Wscrprintf("P_setreal error\n");
	P_setreal(TEMPORARY,"lp",0.0,1);
	P_setstring(TEMPORARY,"dp","y",1);
	/* adjust arraydim if writing less fids */
	if (difffg)
	{
		P_getreal(CURRENT,"arraydim",&arraydim,1);
		if (udifffg) P_setreal(TEMPORARY,"arraydim",arraydim-1.0,1);
		else P_setreal(TEMPORARY,"arraydim",arraydim/2.0,1);
	}
	if ( (r = P_save(TEMPORARY,sysstr)) ) Wscrprintf("P_save error %s\n",sysstr);
	strcat(writename,"/fid"); /* set name to fid! */
	return(COMPLETE);
}

int getfidblockheadforcf()
{
	/* get fid blockhead for writecf option */
	if ( (r = D_gethead(D_USERFILE,&cfhead)) )
	{
		if (r == D_NOTOPEN)
		{
			if (verbose) Wscrprintf("reopening fid header\n");
			strcpy(path,curexpdir);
			strcat(path,"/acqfil/fid");
			r = D_open(D_USERFILE, path, &cfhead);
		}
		if (r)
		{
			if (verbose) Wscrprintf("No way, it's the reopen bit!\n");
			D_error(r);
			return(ERROR);
		}
	}
	sprintf(sysstr,"cp %s/text \"%s\"",curexpdir,writecfname); /* GAM/DI 10viii21 */
	if (system(sysstr))
	{
		Werrprintf("could not copy text from file?\n");
	}
	sprintf(sysstr,"%s/procpar",writecfname);
	if ( (r = P_copy(CURRENT,TEMPORARY)) )
	{
		Wscrprintf("P_copy error\n");
		return(ERROR);
	}
	if ( (r = P_setreal(TEMPORARY,"np",(double)np0w,0)) )
		Wscrprintf("P_setreal error\n");
	P_setreal(TEMPORARY,"lp",0.0,1);
	P_setstring(TEMPORARY,"dp","y",1);
	/* adjust arraydim if writing less fids */
	if (difffg)
	{
		P_getreal(CURRENT,"arraydim",&arraydim,1);
		if (udifffg) P_setreal(TEMPORARY,"arraydim",arraydim-1.0,1);
		else P_setreal(TEMPORARY,"arraydim",arraydim/2.0,1);
	}
	if ( (r = P_save(TEMPORARY,sysstr)) ) Wscrprintf("P_save error %s\n",sysstr);
	strcat(writecfname,"/fid"); /* set name to fid! */
	return(COMPLETE);
}

void readincf()
{
	register int i;
/* GAM 30vii21 */
   union u_tag {
      float fval;
      int   ival;
   } uval;
	int bytes __attribute__((unused));
/* GAM 2viii21 */ 
/* Always read in the first correction function in any array */
	if (readsinglecfflg)
	{
	 	fseek(readcffile,32,SEEK_SET);
	}
	bytes=fread(intbuf,sizeof(dblockhead),1,readcffile);
/*	Should really test for compatibility here, and report errors	*/
	bytes=fread((char *)(intbuf),4,np0w,readcffile);
	if (verbose)
	{
		if (feof(readcffile)) Wscrprintf("End of file detected");
	}
/* GAM 30vii21 */
	for (i=0;i<np0w;i++)
	{
/* Scale down by 10^6 as scaled up when written, so that correction function displays with normal vf */
#ifdef LINUX
        uval.ival = htonl(intbuf[i]);
        data2[i]=uval.fval/1000000.0;
#else
	data2[i]=(float)intbuf[i]/1000000.0;
#endif
	}
/* GAM 27x15 */	for (i=np0w;i<np0;i++) data2[i]=0.0;
}

int setupreadcf()
{  
	int bytes __attribute__((unused));
		strcat(readcfname,"/fid"); /* set name to /fid! */
		if ((readcffile=fopen(readcfname,"r"))==NULL)
		{
			Werrprintf("Error opening correction function file\n");
			return(ERROR);
		}
	bytes=fread((intbuf),sizeof(dfilehead),1,readcffile);
        return(COMPLETE);
}

