/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*****************************************************************************
* continprepare		prepares a file to be read by the programme CONTIN   *
*                       specific to dosy processing                          *
*****************************************************************************/




/*	9iv08 remove spurious reference to DAC_to_G	*/
/*	9iv08 change Dosy directory to dosy		*/
/*	15v08 Change to new dosy_in file format		*/
/*    28x09 use curexpdir/dosy filename in any error messages  */

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
#include "init2d.h"
#include "graphics.h"
#include "group.h"
#include "pvars.h"
#include "wjunk.h"

#define ERROR			1
#define TRUE			1
#define FALSE			0
#define MAXLENGTH               512
#define MAXPEAKS		512
#define MAXPOINTS		600
#define MAXNUGCOEF		8
#define DSCALE			2e-9
#define DC_SCALE		1e14	/*scaling factor for dosyconstant*/
#define NR_END 			1
#define FREE_ARG 		char*
/* #define DEBUG_CONTIN 	1 */	/*Comment out not to compile the debugging code*/


/*}}}*/


/*Declarations*/
/*{{{*/
/*}}}*/


/*}}}*/


int continprepare(int argc, char *argv[], int retc, char *retv[])

{
	/*Declarations*/
/*{{{*/
	int		i,j,l,lr,					/*standard iterators*/	
			bd[MAXPOINTS],				/*array for amplitude levels deleted from analysis (prune)*/
			badinc,						/*number of pruned levels*/
			nPeaks,						/*number of peaks*/
			dosyversion,					/*dosy version */
			mfit,						/*obsolete - for HR-DOSY fitting in dosyfit*/
			calibflag,					/*obsolete - here for historical reasons*/
			nonlinflag,					/*whether to use pure exponentials or corrected*/
			badflag,					/*for removing data from pruned amplitudes*/
			fn1,					    /*fn1 from Vnmr*/
			nPoints;					/*number of points per peak (amplitude levels)*/

	char	rubbish[MAXLENGTH], 		/* string for various purposes*/
			jstr[MAXLENGTH]; 			/* string for various purposes*/

	double	dosyconstant,				/*calculated from the pulse sequence */
			gradAmp[MAXPOINTS],			/*gradient amplitudes */
			frq[MAXPEAKS],				/*frequencies for each peak */
			ampl[MAXPOINTS],			/*amplitudes for each peak */
			iwt_contin,					/*contin parameter - statistical weighting*/
			diff_min,					/*min D to be fitted*/
			diff_max,					/*max D to be fitted*/
			doubletmp,					/*temporary variable*/
			nerfit_contin;				/*contin parameter - for statistical weighting*/

	FILE 		*in,						/*for dosy_in file*/
			*continin;					/*for contin.in format*/

#ifdef DEBUG_CONTIN
	FILE 	*debug;
#endif


/*}}}*/

/* Setting some flags to default values */
	/*{{{*/
/*}}}*/
	

#ifdef DEBUG_CONTIN
	//strcpy(rubbish,userdir);
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/debug_continprepare");
	debug = fopen(rubbish,"w");		/* file for debugging information */
	
	fprintf(debug,"Start of continprepare\n");
	fprintf(debug,"Before reading in of data\n");
#endif

	
	/*Read data from dosy_in*/
/*{{{*/
	//strcpy(rubbish,userdir);
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/dosy_in");
	in = fopen(rubbish,"r");		/* input file for 2D DOSY */
	if (!in) { 
		Werrprintf("continprepare: could not open file %s",rubbish);
		return(ERROR);
	}

	//strcpy(rubbish,userdir);
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/dosy_contin.in");
	continin = fopen(rubbish,"w");	/*  output - input for contin */
	if (!continin) {
		Werrprintf("continprepare: could not open file %s",rubbish);
		fclose(in);
		return(ERROR);
	}


	strcpy(jstr,curexpdir);
	strcat(jstr,"/dosy/dosy_in");
        if (fscanf(in,"DOSY version %d\n",&dosyversion) == EOF)
        {
                Werrprintf("continprepare: reached end of file %s", jstr);
                fclose(in);
                fclose(continin);
                return(ERROR);
        }
        if (fscanf(in,"%d spectra will be deleted from the analysis :\n",&badinc) == EOF) {
                Werrprintf("continprepare: reached end of file %s", jstr);
                fclose(in);
                fclose(continin);
                return(ERROR);
        }
	for (i=0;i<badinc;i++) {
		if (fscanf(in,"%d\n",&bd[i]) == EOF) {
			Werrprintf("continprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(continin);
			return(ERROR);
		}
	}

	if ((fscanf(in,"Analysis on %d peaks\n",&nPeaks) == EOF) /* read in calibration parameters */
		|| (fscanf(in,"%d points per peaks\n",&nPoints) == EOF)
		|| (fscanf(in,"%d parameters fit\n",&mfit) == EOF)
		|| (fscanf(in,"dosyconstant = %lf\n",&dosyconstant) == EOF)
		|| (fscanf(in,"gradient calibration flag :  %d\n",&calibflag) == EOF)
		|| (fscanf(in,"non-linear gradient flag :  %d\n",&nonlinflag) == EOF))
	{
		Werrprintf("continprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(continin);
		return(ERROR);
	}
	calibflag=0; /*obsolete*/

	
/*}}}*/
#ifdef DEBUG_CONTIN
	fprintf(debug,"Before reading in of parameters from Vnmr\n");
#endif

	/*Read in parameters from Vnmr */
/*{{{*/

	if (!P_getreal(CURRENT,"fn1",&doubletmp,1)) {
		fn1= (int) (doubletmp +0.5);
	}
	else{
		Werrprintf("Error accessing parameter fn1\n");
		fclose(in);
		fclose(continin);
		return(ERROR);
	}

	

/* Check for sensible values of n and np */
/*{{{*/
	if (nPeaks > MAXPEAKS)
	{
		fprintf(continin,"Too many peaks for the analysis !\n");
		Werrprintf("continprepare: Too many peaks for the analysis !\n");
		fclose(in);
		fclose(continin);
		return(ERROR);
	}
	if (nPoints > MAXPOINTS)
	{
		Werrprintf("continprepare: Too many increments for analysis, max. nunmber of gzlvl1 is %d\n", MAXPOINTS);
		fclose(in);
		fclose(continin);
		return(ERROR);
	}

	/*Read in gradient amplitudes values from file*/
/*{{{*/
	for (i=0,j=0;i<(nPoints+badinc);i++){         /*scan through all the input values, discarding the unwanted ones */ 
		badflag = 0;
		for (l=0;l<badinc;l++)  if (i == bd[l]-1) badflag = TRUE;
		strcpy(jstr,curexpdir);
		strcat(jstr,"/dosy/dosy_in"); /* read in gradient array */
		if (badflag) {
			if (fscanf(in,"%s",rubbish) == EOF) {
				Werrprintf("continprepare: reached end of file %s on increment %d", jstr, i);
		fclose(in);
		fclose(continin);
				return(ERROR);
			}
		}
		else     {
			if (fscanf(in,"%lf",&gradAmp[j]) == EOF) {
				Werrprintf("continprepare: reached end of file %s on increment %d", jstr, i);
		fclose(in);
		fclose(continin);
				return(ERROR);
			}
			j++; 
		}
	}




/*}}}*/

	/*Read in frequencies from dosy_in*/
/*{{{*/
		strcpy(jstr,curexpdir);
		strcat(jstr,"/dosy/dosy_in"); 
		if (fscanf(in,"%s",rubbish) == EOF) {
			Werrprintf("continprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(continin);
			return(ERROR);
		}
		while (strcmp(rubbish,"(Hz)") != 0)
			if (fscanf(in,"%s",rubbish) == EOF) {
				Werrprintf("continprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(continin);
				return(ERROR);
			}
		for (i=0;i<nPeaks;i++)
			if ((fscanf(in,"%s",rubbish) == EOF) || (fscanf(in,"%lf",&frq[i]) == EOF)) {
				Werrprintf("continprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(continin);
				return(ERROR);
			}
/*}}}*/


	/* Loop over resonances begins */
		
	for (lr=0;lr<nPeaks;lr++){					

 			/* read in amplitudes from dosy_in */
/*{{{*/
			if (lr == 0)       {
				if (fscanf(in,"%s",rubbish) == EOF) {
					Werrprintf("continprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(continin);
					return(ERROR);
				}
				while(strcmp(rubbish,"(mm)") != 0) {
					if (fscanf(in,"%s",rubbish) == EOF) {
						Werrprintf("continprepare: reached end of file %s", jstr);
		fclose(in);
		fclose(continin);
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
		iwt_contin=1.0;								/*all points have the same statistical weight*/	
		nerfit_contin=0.0;							/*must be 0 if iwt is 1*/	
		if (argc==3){
			diff_min=atof(argv[1]);
			diff_max=atof(argv[2]);
		}
		else{
			Werrprintf("continprepare: exactly 2 arguments must be passed\n");
		fclose(in);
		fclose(continin);
			return(ERROR);
		}
	
			
		
		fprintf(continin," ***DOSY EXPERIMENT*** peak number %d   at %lf Hz\n",(lr+1),frq[lr]);
		fprintf(continin," IWT       %d    %1.1f\n",0,iwt_contin);
		fprintf(continin," NERFIT    %d    %1.1f\n",0,nerfit_contin);
		fprintf(continin," GMNMX    %d    %1.1E\n",1,diff_min);
		fprintf(continin," GMNMX    %d    %1.1E\n",2,diff_max);
		fprintf(continin," NG       %d    %1.3E\n",1,((double) fn1)/2.0);
		fprintf(continin," IGRID    %d    %1.3E\n",1,1.0);
		fprintf(continin," IFORMY\n (1E11.5)\n");
		fprintf(continin," IFORMT\n (1E11.5)\n");
		fprintf(continin," NINTT    %d    %1.4E\n",0,0.0);	
		fprintf(continin," RUSER    %d    %1.4E\n",16,1.0);	
		fprintf(continin," RUSER    %d    %1.4E\n",17,180.0);
		fprintf(continin," RUSER    %d    %1.4E\n",15,sqrt( (dosyconstant/DC_SCALE) )/(4.0e7*3.14159265358979323846) );
		fprintf(continin," IUSER    %d    %1.1f\n",10,2.0);	
		fprintf(continin," IPLFIT    %d    %1.1f\n",1,0.0);	
		fprintf(continin," IPLFIT    %d    %1.1f\n",2,0.0);	
		fprintf(continin," IPLRES    %d    %1.1f\n",1,2.0);	
		fprintf(continin," IPLRES    %d    %1.1f\n",2,2.0);	
		if (lr==(nPeaks-1)){
			fprintf(continin," LAST      %d    %1.1f\n",0,1.0);			
		}else{
			fprintf(continin," LAST      %d    %1.1f\n",0,-1.0);		
		}
		fprintf(continin," END\n");	
		if (nPoints<10){				/*for right justify*/
			fprintf(continin," NY        %d\n",nPoints);		
		}else if (nPoints<100){
			fprintf(continin," NY       %d\n",nPoints);		
		}else{
			fprintf(continin," NY      %d\n",nPoints);		
		}
		for (i=0;i<nPoints;i++){
			fprintf(continin," %10.4E\n",(gradAmp[i]*gradAmp[i]));	
		}


		for (i=0;i<nPoints;i++){
			fprintf(continin," %10.4E\n",ampl[i]);	
		}


	}
			
#ifdef DEBUG_CONTIN
	fprintf(debug,"End of continprepare\n");
	fclose(debug);
#endif 

		fclose(in);
		fclose(continin);
		RETURN;
}

/*FUNCTIONS*/

