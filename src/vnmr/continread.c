/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*****************************************************************************
* continread         processes the output file from the CONTIN programme and *
*                    prepares an input file for ddif                         *
*		     specific to dosy processing                             *
*****************************************************************************/

/* 9iv08 remove spurious DAC_to_G	*/
/* 9iv08 change Dosy directory to dosy	*/
/* 15v08 Change to new format of dosy_in file                      */
/* 15iv09 Output to diffusion_spectrum is unused, because of 2048 point limit in dsp 	*/
/* 28x09 Use correct curexpdir/dosy filename in any error messages, not userdir               */




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
#include "wjunk.h"

#define ERROR			1
#define TRUE			1
#define FALSE			0
#define MAXLENGTH		512
#define MAXPEAKS		512
#define MAXPOINTS		600
#define MAX_ERROR   	30.0	/* Maximum error permitted on the diffusion value, in per cent */
#define MAXNUGCOEF		8
#define DSCALE			2e-9
#define NR_END 			1
#define FREE_ARG 		char*
/*#define DEBUG_CONTIN 		1 */ /*Comment out not to compile the debugging code*/


/*}}}*/


/*Declarations*/
/*{{{*/
/*}}}*/


/*}}}*/


int continread(int argc, char *argv[], int retc, char *retv[])

{
	/*Declarations*/
/*{{{*/
	int		i,j,k,l,				/*standard iterators*/	
    			dosyversion, 		                /*version number*/
			nPeaks,					/*number of peaks*/
			badinc,				     	/*number of pruned levels*/	
			nonlinflag,				/*whether to use pure exponentials or corrected*/
			badflag,					/*for removing data from pruned amplitudes*/
			bd[MAXPOINTS],				/*array for amplitude levels deleted from analysis (prune)*/
			peaknr,						/*current peak*/
			inttmp,						/*temporary variable*/
			fn1,						/*resolution in the diffusion dimension*/
			nPoints;					/*number of points per peak (amplitude levels)*/

	char	rubbish[MAXLENGTH], 		/* string for various purposes*/
	    	*pptemp,			 		/* temprary pointer to pointer*/
			strOne[MAXLENGTH],			/* Have to read in data as a string*/
			strTwo[MAXLENGTH],			/* Have to read in data as a string*/
			strThree[MAXLENGTH],			/* Have to read in data as a string*/
			jstr[MAXLENGTH]; 			/* string for various purposes*/

	double	dosyconstant,				/*calculated from the pulse sequence */
			doubletmp,					/*temporary variable */
			doubletmp2,					/*temporary variable */
			doubletmp3 = 0.0,				/*temporary variable */
			doubletmp4 = 0.0,				/*temporary variable */
			doubletmp5 = 0.0,				/*temporary variable */
			gradAmp[MAXPOINTS],			/*gradient amplitudes */
			frq[MAXPEAKS],				/*frequencies for each peak */
			ampl[MAXPOINTS];			/*amplitudes for each peak */

	FILE 	*infile,						/*for dosy_contin.out file*/
	    	*dosyfile,						/*for dosy_in file*/
	    	*fit_residuals,					/*for general_dosy_stats*/
    	    *ddiffile_contin,				/*for diffusion_display.contin*/
    	    *ddiffile,						/*for diffusion_display.inp*/
    	    *D_spectrum;						/*for diffusion_spectrum*/
	

#ifdef DEBUG_CONTIN
	FILE 	*debug;
#endif


/*}}}*/

	

#ifdef DEBUG_CONTIN
	//strcpy(rubbish,userdir);
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/debug_continread");
	debug = fopen(rubbish,"w");		/* file for debugging information */
	fprintf(debug,"Start of continread\n");
	fprintf(debug,"Before reading in of data\n");
#endif
	
	/*Read data from dosy_in*/



	//strcpy(rubbish,userdir);
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/dosy_in");
	dosyfile = fopen(rubbish,"r");	
	if (!dosyfile) { 
		Werrprintf("continread: could not open file %s",rubbish);
		return(ERROR);
	}
	strcpy(jstr,curexpdir);
	strcat(jstr,"/dosy/dosy_in");
       if (fscanf(dosyfile,"DOSY version %d\n",&dosyversion) == EOF)
        {
                Werrprintf("continread: reached end of file %s", jstr);
                fclose(dosyfile);
                return(ERROR);
        }
        if (fscanf(dosyfile,"%d spectra will be deleted from the analysis :\n",&badinc) == EOF) {
                Werrprintf("continread: reached end of file %s", jstr);
                fclose(dosyfile);
                return(ERROR);
        }
	for (i=0;i<badinc;i++) {
		if (fscanf(dosyfile,"%d\n",&bd[i]) == EOF) {
			Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
			return(ERROR);
		}
	}

	if ((fscanf(dosyfile,"Analysis on %d peaks\n",&nPeaks) == EOF) /* read in calibration parameters */
		|| (fscanf(dosyfile,"%d points per peaks\n",&nPoints) == EOF)
		|| (fscanf(dosyfile,"%d parameters fit\n",&inttmp) == EOF)
		|| (fscanf(dosyfile,"dosyconstant = %lf\n",&dosyconstant) == EOF)
		|| (fscanf(dosyfile,"gradient calibration flag :  %d\n",&inttmp) == EOF)
		|| (fscanf(dosyfile,"non-linear gradient flag :  %d\n",&nonlinflag) == EOF))
	{
		Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		return(ERROR);
	}

	for (i=0,j=0;i<(nPoints+badinc);i++){         /*scan through all the input values, discarding the unwanted ones */ 
		badflag = 0;
		for (l=0;l<badinc;l++)  if (i == bd[l]-1) badflag = TRUE;
		strcpy(jstr,curexpdir);
		strcat(jstr,"/dosy/dosy_in"); /* read in gradient array */
		if (badflag) {
			if (fscanf(dosyfile,"%s",rubbish) == EOF) {
				Werrprintf("continread: reached end of file %s on increment %d", jstr, i);
		fclose(dosyfile);
				return(ERROR);
			}
		}
		else     {
			if (fscanf(dosyfile,"%lf",&gradAmp[j]) == EOF) {
				Werrprintf("continread: reached end of file %s on increment %d", jstr, i);
		fclose(dosyfile);
				return(ERROR);
			}
			j++; 
		}
	}

	if (fscanf(dosyfile,"%s",rubbish) == EOF) {
					Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
					return(ERROR);
				}
				while(strcmp(rubbish,"(mm)") != 0) {
					if (fscanf(dosyfile,"%s",rubbish) == EOF) {
						Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
						return(ERROR);
					}
				}


    /*open the fit_residuals*/
	//strcpy(rubbish,userdir);
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/general_dosy_stats");
	fit_residuals = fopen(rubbish,"w");	
	if (!fit_residuals) { 
		Werrprintf("continread: could not open file %s",rubbish);
		fclose(dosyfile);
		return(ERROR);
	}

	fprintf(fit_residuals,"\t\t2D data set\n");
	fprintf(fit_residuals,"\t\tsee dosy_contin.out for more statistical information.\n");
	fprintf(fit_residuals,"Fitted for distributions of pure exponentials using CONTIN\n\n\n\n");
	

	/*Read data from dosy_contin.out*/
	//strcpy(rubbish,userdir);
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/dosy_contin.out");
	infile = fopen(rubbish,"r");	
	if (!infile) { 
		Werrprintf("continread: could not open file %s",rubbish);
		fclose(dosyfile);
		fclose(fit_residuals);
		return(ERROR);
	}
    	//strcpy(rubbish,userdir);
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/diffusion_display.inp");
	ddiffile = fopen(rubbish,"w");	
	if (!ddiffile) {
		Werrprintf("continread: could not open file %s",rubbish);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		return(ERROR);
	}
	//strcpy(rubbish,userdir);
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/diffusion_display.contin");
	ddiffile_contin = fopen(rubbish,"w");	
	if (!ddiffile_contin) {
		Werrprintf("continread: could not open file %s",rubbish);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		return(ERROR);
	}
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/diffusion_spectrum");
	D_spectrum = fopen(rubbish,"w");	
	if (!D_spectrum) {
		Werrprintf("continread: could not open file %s",rubbish);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(ddiffile_contin);
		return(ERROR);
	}
	strcpy(jstr,curexpdir);
	strcat(jstr,"/dosy/dosy_contin.out");

	for (k=1;k<=nPeaks;k++){
	
		for (i=0,j=0;i<(nPoints+badinc);i++) {
			badflag = 0;
			for (l=0;l<badinc;l++)         if (i == bd[l]-1) badflag = 1;
			fscanf(dosyfile,"%s",rubbish);
			fscanf(dosyfile,"%s",rubbish);
			if (badflag)	fscanf(dosyfile,"%s",rubbish);
			else		fscanf(dosyfile,"%lf",&ampl[j++]);
		}

	
		if (fscanf(infile,"%s",rubbish) == EOF) {
			Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
			return(ERROR);
		}

		while (strcmp(rubbish,"EXPERIMENT***") != 0)
			if (fscanf(infile,"%s",rubbish) == EOF) {
				Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
				return(ERROR);
			}

		if (fscanf(infile,"%s",rubbish) == EOF) {
			Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
			return(ERROR);
			}
		if (fscanf(infile,"%s",rubbish) == EOF) {
			Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
			return(ERROR);
			}
		if (fscanf(infile,"%d",&peaknr) == EOF) {
			Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
			return(ERROR);
		}
		if (fscanf(infile,"%s",rubbish) == EOF) {
			Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
			return(ERROR);
			}
		if (fscanf(infile,"%lf",&frq[k]) == EOF) {
			Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
			return(ERROR);
		}

		printf("peak nr %d\n",k);
		printf("%d %lf\n",peaknr,frq[k]);
	
		while (strcmp(rubbish,"NG") != 0)
			if (fscanf(infile,"%s",rubbish) == EOF) {
				Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
				return(ERROR);
			}
			if (fscanf(infile,"%s",rubbish) == EOF) {
				Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
				return(ERROR);
				}
			if (fscanf(infile,"%s",rubbish) == EOF) {
				Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
				return(ERROR);
				}
			fn1= (int) strtod(rubbish,&pptemp); /*number of grid points (fn1 from Vnmr /2)*/
			if (fscanf(infile,"%s",rubbish) == EOF) {
				Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
				return(ERROR);
			}
		while (strcmp(rubbish,"CHOSEN") != 0)
			if (fscanf(infile,"%s",rubbish) == EOF) {
				Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
				return(ERROR);
			}
		while (strcmp(rubbish,"RESIDUALS") != 0)
			if (fscanf(infile,"%s",rubbish) == EOF) {
				Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
				return(ERROR);
			}
		fprintf(fit_residuals,"\nFrequency %lf\n",frq[k]);
		fprintf(fit_residuals,"Gradient area         exp. height    calc. height          Diff\n");
		for (i=0;i<nPoints;i++) {
			if (fscanf(infile,"%s",strOne)==EOF){
			Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
			return(ERROR);
			}
			doubletmp=strtod(strOne,&pptemp);
			/*Print to general_dosy_stats*/
			fprintf(fit_residuals,"%10.6lf %15.6lf %15.6lf %15.6lf\n",dosyconstant*0.0001*(1.0e-9)*gradAmp[i]*gradAmp[i],ampl[i],ampl[i]-doubletmp,doubletmp);
		}

		while (strcmp(rubbish,"ABSCISSA") != 0)
			if (fscanf(infile,"%s",rubbish) == EOF) {
				Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
				return(ERROR);
			}
#ifdef UNDEF
			if (fscanf(infile,"%s",rubbish) == EOF) {
				Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
				return(ERROR);
			}
			if (fscanf(infile,"%s",rubbish) == EOF) {
				Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
				return(ERROR);
			}
			if (fscanf(infile,"%s",rubbish) == EOF) {
				Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
				return(ERROR);
			}
#endif

			fprintf(ddiffile,"%10.3lf%10.4lf%10.4lf%10.6lf%10.4lf%10.4lf%10.6lf\n", frq[k],0.0,0.0,0.0,0.0,0.0,0.0);

		for (i=1;i<=fn1;i++) {
			fscanf(infile,"%s  %s  %s\n",strOne,strTwo,strThree);
			doubletmp=strtod(strOne,&pptemp);
			doubletmp2=strtod(strThree,&pptemp);
			fprintf(ddiffile_contin,"%3.3e %3.3e \n",doubletmp,doubletmp2);
			if (i==1)
				{
				doubletmp3=doubletmp;
				doubletmp4=doubletmp2;
				}
			if (i==2)
				{
				doubletmp5=2.0*(doubletmp2-doubletmp4);
				fprintf(D_spectrum,"%8.4lf,\t%10.4lf,\t%10.4lf,\t1.0\n",doubletmp4,doubletmp3,doubletmp5);
				fprintf(D_spectrum,"%8.4lf,\t%10.4lf,\t%10.4lf,\t1.0\n",doubletmp2,doubletmp,doubletmp5);
				}
			if (i>2)
				{
				fprintf(D_spectrum,"%8.4lf,\t%10.4lf,\t%10.4lf,\t1.0\n",doubletmp2,doubletmp,doubletmp5);
				}
				if ( (strncmp(pptemp,"",1) ==0)){ 
				if (fscanf(infile,"%s",rubbish)==EOF){
				Werrprintf("continread: reached end of file %s", jstr);
		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);
				return(ERROR);
					
			   	}
		
			}
		}
		
		}





#ifdef DEBUG_CONTIN
	fprintf(debug,"End of continread\n");
	fclose(debug);
#endif 

		fclose(dosyfile);
		fclose(fit_residuals);
		fclose(infile);
		fclose(ddiffile);
		fclose(D_spectrum);
		fclose(ddiffile_contin);

		RETURN;
}

/*FUNCTIONS*/




