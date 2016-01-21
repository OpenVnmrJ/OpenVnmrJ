/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**********************************************************************************
*                                                                                 *
*  splmodprepare    prepares the input data file for the program SPLMOD		  *
*                                                                                 *
**********************************************************************************/

/*	9iv08 change DAC_to_G to gcal_					*/
/*	9iv08 change nugcal to nugcal_					*/
/*	9iv08 change Dosy directory to dosy				*/
/*	15v08 Change to new format of dosy_in file			*/
/*	MN read in ncomp from parameter set				*/
/*	3iv09 disable change in gradient values if nugflag='y'		*/
/*	5v09  increase MAXPEAKS to 32768 to match dosyfit		*/
/*    28x09 use correct curexpdir/dosy filename in any error messages */
/*      14iii11 change fit limits pnmnmx(1),(2) to 0.002,3              */



/*Includes and defines*/
/*{{{*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <unistd.h>


#include "vnmrsys.h"
#include "data.h"
#include "disp.h"
#include "pvars.h"
#include "init2d.h"
#include "graphics.h"
#include "group.h"
#include "wjunk.h"

#define ERROR			1
#define TRUE			1
#define FALSE			0
#define MAXLENGTH		512
#define MAXPEAKS		32768
#define MAXPOINTS		600
#define MAXFITCOEFFS		4
#define DSCALE			2e-9
#define NR_END 			1
#define FREE_ARG 		char*
/* #define DEBUG_SPLMOD 	1 */	/*Comment out not to compile the debugging code*/


/*}}}*/

/*Declarations*/




int splmodprepare(int argc, char *argv[], int retc, char *retv[])

{
	/*Declarations*/
/*{{{*/
	int		i,j,l,lr,					/*standard iterators*/	
			bd[MAXPOINTS],				/*array for amplitude levels deleted from analysis (prune)*/
			badinc,						/*number of pruned levels*/
			nPeaks,						/*number of peaks*/
			ncomp,					/*max number of components for the fit*/
			dosyversion,					/*version number*/
			mfit,						/*obsolete - for HR-DOSY fitting in dosyfit*/
			calibflag,					/*obsolete - here for historical reasons*/
			nugflag,					/*whether to use pure exponentials or corrected*/
			badflag,					/*for removing data from pruned amplitudes*/
			mtry,						/*number of starting values for each D*/
			nugsize,					/*length of array nugcal_*/
			nPoints;					/*number of points per peak (amplitude levels)*/

	char	rubbish[MAXLENGTH], 		/* string for various purposes*/
			jstr[MAXLENGTH]; 			/* string for various purposes*/

	double	dosyconstant,				/*calculated from the pulse sequence */
			gcal_,			/*conversion factor from DAC points to Gauss - from nugcal_*/
			gcal__conv,			/*conversion factor from DAC points to Gauss - to convert gradAmp using nugcal_*/
			gradAmp[MAXPOINTS],					/*gradient amplitudes */
			frq[MAXPEAKS],						/*frequencies for each peak */
			ampl[MAXPOINTS],						/*amplitudes for each peak */
			nnl_splmod,					/*SPLMOD parameter - number of analyses for each data set*/
			tmp,					/*for temporary usage*/
			ngam_splmod,				/*SPLMOD parameter - value of background (baseline)*/
			iwt_splmod,					/*SPLMOD parameter - statistical weighting*/
			nerfit_splmod,				/*SPLMOD parameter - for statistical weighting*/
			plmnmx_1_splmod,			/*SPLMOD parameter - minimum peak amplitude*/
			plmnmx_2_splmod,			/*SPLMOD parameter - maximum peak amplitude*/
			pnmnmx_1_splmod,			/*SPLMOD parameter - controls minimum D */
			pnmnmx_2_splmod,			/*SPLMOD parameter - controls maximum D */
			nug[MAXFITCOEFFS+1];			/*coefficients for NUG correction*/

	FILE 	*in,						/*for dosy_in file*/
		*splmodin;				/*for splmod.in format*/	

#ifdef DEBUG_SPLMOD
	FILE 	*debug;
#endif


/*}}}*/

/* Setting some flags to default values */
	/*{{{*/
/*}}}*/
	

#ifdef DEBUG_SPLMOD
	//strcpy(rubbish,userdir);
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/debug_splmodprepare");
	debug =fopen(rubbish,"w");		/* file for debugging information */
	fprintf(debug,"Start of splmodprepare\n");
	fprintf(debug,"Before reading in of data\n");
#endif



	
	/*Read data from dosy_in*/
/*{{{*/
	//strcpy(rubbish,userdir);
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/dosy_in");
	in = fopen(rubbish,"r");		/* input file for 2D DOSY */
	if (!in) { 
		Werrprintf("splmodprepare: could not open file %s",rubbish);
		return(ERROR);
	}

	//strcpy(rubbish,userdir);
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/dosy_splmod.in");
	splmodin = fopen(rubbish,"w");	/*  output - input for splmod */
	if (!splmodin) {
		Werrprintf("splmodprepare: could not open file %s",rubbish);
		fclose(in);
		return(ERROR);
	}


	strcpy(jstr,curexpdir);
	strcat(jstr,"/dosy/dosy_in");
	if (fscanf(in,"DOSY version %d\n",&dosyversion) == EOF)
	{
		Werrprintf("splmodprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(splmodin);
		return(ERROR);
	}
	if (fscanf(in,"%d spectra will be deleted from the analysis :\n",&badinc) == EOF) {
		Werrprintf("splmodprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(splmodin);
		return(ERROR);
	}
	for (i=0;i<badinc;i++) {
		if (fscanf(in,"%d\n",&bd[i]) == EOF) {
			Werrprintf("splmodprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(splmodin);
			return(ERROR);
		}
	}

	if ((fscanf(in,"Analysis on %d peaks\n",&nPeaks) == EOF) /* read in calibration parameters */
		|| (fscanf(in,"%d points per peaks\n",&nPoints) == EOF)
		|| (fscanf(in,"%d parameters fit\n",&mfit) == EOF)
		|| (fscanf(in,"dosyconstant = %lf\n",&dosyconstant) == EOF)
		|| (fscanf(in,"gradient calibration flag :  %d\n",&calibflag) == EOF))
	{
		Werrprintf("splmodprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(splmodin);
		return(ERROR);
	}
	calibflag=0; /*obsolete*/

	
/*}}}*/
#ifdef DEBUG_SPLMOD
	fprintf(debug,"Before reading in of parameters from Vnmr\n");
#endif

	/*Read in parameters from Vnmr */
/*{{{*/

 /* MN2Mar Reading in ncomp - number of components to be fixed (i.e ncomp=2 is a biexponential fit */
ncomp=1;
  if (!P_getreal (CURRENT, "ncomp", &tmp, 1))
    {
	 ncomp = (int) (tmp + 0.5);
      if (ncomp < 1) 
	ncomp = 1;
    }
/* fetching nugflag as a parameter */
			for(i=0;i<=MAXFITCOEFFS;i++)
                                        nug[i]=0.0;
                        nugflag=0;
                        if (!P_getstring(CURRENT, "nugflag",rubbish,1,2)) {
                                if (!strcmp("y",rubbish)) {
                                        nugflag=1;
                                }
                                else {
                                        nugflag=0;
                                }
                        }
                        if(nugflag){
                                P_getsize(CURRENT,"nugcal_",&nugsize);
                                if (nugsize>5) nugsize=5;
                                if (nugsize<2){
                                        Wscrprintf("splmodread: nugcal_ must contain at least 2 values");
                                        Werrprintf("splmodread: nugcal_ must contain at least 2 values");
			 		fclose(in);
			 		fclose(splmodin);
                                        return(ERROR);
                                        }
                                if (P_getreal(CURRENT, "nugcal_", &gcal_, 1))
                                        {
                                        Werrprintf("splmodread: cannot read gcal_ from nugcal_");
                                        Wscrprintf("splmodread: cannot read gcal_ from nugcal_");
			 		fclose(in);
			 		fclose(splmodin);
                                        return(ERROR);
                                        }
                         for (i=1;i<nugsize;i++){
                                        if (P_getreal(CURRENT, "nugcal_", &nug[i], i+1))
                                                {
                                                Werrprintf("splmodread: cannot read coefficients from nugcal_");
                                                Wscrprintf("splmodread: cannot read coefficients from nugcal_");
			 			fclose(in);
			 			fclose(splmodin);
                                                return(ERROR);
                                                }
                                        }
#ifdef DEBUG_SPLMOD
                                        fprintf(debug,"nugsize:  %d\n",nugsize);
                                        fprintf(debug,"gcal_ %e :\n",gcal_);
                                        for (i=0;i<MAXFITCOEFFS;i++)
                                        fprintf(debug,"nug %e :\n",nug[i+1]);
                                        fprintf(debug,"ncomp:  %d\n",ncomp);
#endif
                        }















	
/*}}}*/


/* Check for sensible values of n and np */
/*{{{*/
	if (nPeaks > MAXPEAKS)
	{
		fprintf(splmodin,"Too many peaks for the analysis !\n");
		Werrprintf("splmodprepare: Too many peaks for the analysis !\n");
		fclose(in);
		fclose(splmodin);
		return(ERROR);
	}
	if (nPoints > MAXPOINTS)
	{
		Werrprintf("splmodprepare: Too many increments for analysis, max. nunmber of gzlvl1 is %d\n", MAXPOINTS);
		fclose(in);
		fclose(splmodin);
		return(ERROR);
	}/*}}}*/

	

	/*Read in gradient amplitudes values from file*/

	for (i=0,j=0;i<(nPoints+badinc);i++){         /*scan through all the input values, discarding the unwanted ones */ 
		badflag = 0;
		for (l=0;l<badinc;l++)  if (i == bd[l]-1) badflag = TRUE;
		strcpy(jstr,curexpdir);
		strcat(jstr,"/dosy/dosy_in"); /* read in gradient array */
		if (badflag) {
			if (fscanf(in,"%s",rubbish) == EOF) {
				Werrprintf("splmodprepare: reached end of file %s on increment %d", jstr, i);
		fclose(in);
		fclose(splmodin);
				return(ERROR);
			}
		}
		else     {
			if (fscanf(in,"%lf",&gradAmp[j]) == EOF) {
				Werrprintf("splmodprepare: reached end of file %s on increment %d", jstr, i);
		fclose(in);
		fclose(splmodin);
				return(ERROR);
			}
			j++; 
		}
	}
/* MN11Feb08 if NUG is used, we need to correct the gradAmp values for the (potentially) different gcal_
if (P_getreal (CURRENT, "gcal_", &gcal__orig, 1))
    {
      Werrprintf ("Error accessing parameter gcal_\n");
      fclosetest (in);
      fclose(splmodin);
      return (ERROR);
    }
if (nugflag){
  gcal__conv = gcal_/gcal__orig;
}
else{
  gcal__conv = 1.0;
}
	for (i=0;i<nPoints;i++){       
	gradAmp[i]=gradAmp[i]*gcal__conv;
}

*/

  gcal__conv = 1.0;

	/*Read in frequencies from dosy_in*/
/*{{{*/
		strcpy(jstr,curexpdir);
		strcat(jstr,"/dosy/dosy_in"); 
		if (fscanf(in,"%s",rubbish) == EOF) {
			Werrprintf("splmodprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(splmodin);
			return(ERROR);
		}
		while (strcmp(rubbish,"(Hz)") != 0)
			if (fscanf(in,"%s",rubbish) == EOF) {
				Werrprintf("splmodprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(splmodin);
				return(ERROR);
			}
		for (i=0;i<nPeaks;i++)
			if ((fscanf(in,"%s",rubbish) == EOF) || (fscanf(in,"%lf",&frq[i]) == EOF)) {
				Werrprintf("splmodprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(splmodin);
				return(ERROR);
			}
/*}}}*/


	/* Loop over resonances begins */
		
	for (lr=0;lr<nPeaks;lr++){					

 			/* read in amplitudes from dosy_in */
/*{{{*/
			if (lr == 0)       {
				if (fscanf(in,"%s",rubbish) == EOF) {
					Werrprintf("splmodprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(splmodin);
					return(ERROR);
				}
				while(strcmp(rubbish,"(mm)") != 0) {
					if (fscanf(in,"%s",rubbish) == EOF) {
						Werrprintf("splmodprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(splmodin);
						return(ERROR);
					}
				}
			}        

		
			for (i=0,j=0;i<(nPoints+badinc);i++) {
				badflag = 0;
				for (l=0;l<badinc;l++)         if (i == bd[l]-1) badflag = 1;
				fscanf(in,"%s",rubbish);
				fscanf(in,"%s",rubbish);
				if (badflag)	fscanf(in,"%s",rubbish);
				else		fscanf(in,"%lf",&ampl[j++]);
			}


/*}}}*/	
			/*Testing some parameters*/
		nnl_splmod=(double) ncomp;							/*max number of components per decay*/	
		ngam_splmod=0.0;							/*assume base line corrected*/
		iwt_splmod=1.0;								/*all points have the same statistical weight*/	
		nerfit_splmod=0.0;							/*must be 0 if iwt is 1*/	
		plmnmx_1_splmod=0.0;						/*minimum amplitude - non negativity*/	
		plmnmx_2_splmod=5.0;						/*maximum amplitude value - 5 times the max y value*/	
		pnmnmx_1_splmod=2.0e-2;						/*default value*/	
		pnmnmx_2_splmod=2.08;						/*default value*/	
		pnmnmx_1_splmod=0.002;                                          /* conservative value */
		pnmnmx_2_splmod=3.0;                                            /* conservative value */
		mtry=20;									/*number of starting values for each D*/	
		
		fprintf(splmodin," ***DOSY EXPERIMENT*** peak number %d   at %lf Hz\n",(lr+1),frq[lr]);
		fprintf(splmodin," NNL       %d    %1.1f\n",0,nnl_splmod);
		fprintf(splmodin," NGAM      %d    %1.1f\n",0,ngam_splmod);
		fprintf(splmodin," IWT       %d    %1.1f\n",0,iwt_splmod);
		fprintf(splmodin," NERFIT    %d    %1.1f\n",0,nerfit_splmod);
		fprintf(splmodin," CONVRG    %d    %1.1E\n",1,5e-7);
		fprintf(splmodin," DOSPL\nFFFFFFFFFFFFFFFFFFFF\n");				/*use exact model - no bsplines */
		fprintf(splmodin," MTRY\n");									/*number of starting values for each D*/	
		for (i=1;i<=20;i++){
			fprintf(splmodin," %3d",mtry);
		}
		fprintf(splmodin," \n");	
		fprintf(splmodin," PLMNMX    %d    %1.1E\n",1,plmnmx_1_splmod);
		fprintf(splmodin," PLMNMX    %d    %1.1E\n",2,plmnmx_2_splmod);
		fprintf(splmodin," PNMNMX    %d    %1.1E\n",1,pnmnmx_1_splmod);
		fprintf(splmodin," PNMNMX    %d    %1.1E\n",2,pnmnmx_2_splmod);
		fprintf(splmodin," MXITER    %d    %3.1f\n",1,2000.0);
		fprintf(splmodin," MXITER    %d    %3.1f\n",2,4000.0);
		fprintf(splmodin," IFORMY\n (1E11.5)\n");
		fprintf(splmodin," IFORMT\n (1E11.5)\n");
		if (nugflag){
			fprintf(splmodin," RUSER     %d    %1.4E\n",1,nug[1]);	
			fprintf(splmodin," RUSER     %d    %1.4E\n",2,nug[2]);
			fprintf(splmodin," RUSER     %d    %1.4E\n",3,nug[3]);
			fprintf(splmodin," RUSER     %d    %1.4E\n",4,nug[4]);
		}
		fprintf(splmodin," IUSER     %d    %1.1f\n",1,1.0);	
		fprintf(splmodin," IPRITR    %d    %1.1f\n",1,0.0);	
		fprintf(splmodin," IPRITR    %d    %1.1f\n",2,0.0);	
		fprintf(splmodin," IPLFIT    %d    %1.1f\n",1,0.0);	
		fprintf(splmodin," IPLFIT    %d    %1.1f\n",2,0.0);	
		fprintf(splmodin," IPLRES    %d    %1.1f\n",1,1.0);	
		fprintf(splmodin," IPLRES    %d    %1.1f\n",2,1.0);	
		if (lr==(nPeaks-1)){
			fprintf(splmodin," LAST      %d    %1.1f\n",0,1.0);			
		}else{
			fprintf(splmodin," LAST      %d    %1.1f\n",0,-1.0);		
		}
		fprintf(splmodin," END\n");	
		if (nPoints<10){				/*for right justify*/
			fprintf(splmodin," NY        %d\n",nPoints);		
		}else if (nPoints<100){
			fprintf(splmodin," NY       %d\n",nPoints);		
		}else{
			fprintf(splmodin," NY      %d\n",nPoints);		
		}
		for (i=0;i<nPoints;i++){
			fprintf(splmodin," %10.4E\n",(gradAmp[i]*gradAmp[i]));	
		}


		for (i=0;i<nPoints;i++){
			fprintf(splmodin," %10.4E\n",ampl[i]);	
		}


	}
			
#ifdef DEBUG_SPLMOD
	fprintf(debug,"End of splmodprepare\n");
	fclose(debug);
#endif

	fclose(in);
	fclose(splmodin);

		RETURN;
}

/*FUNCTIONS*/

