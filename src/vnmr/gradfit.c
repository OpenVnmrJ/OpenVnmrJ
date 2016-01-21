/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**********************************************************************************
*	gradfit									  *
*	powerfit								  *
*	profile_int								  *
*	decay_gen								  *
*										  *
*  decay_gen    calculates the form of diffusional attenuation expected for	  *
*              	the measured gradient and signal maps in non-uniform gradient	  *
*               calibration							  *
*										  *
*  gradfit      calculates fit coefficients describing the variation of		  *
*               gradient strength with position in calibration of non-		  *
*               uniform pulsed field gradients					  *
*										  *
*  powerfit     calculates fit coefficients describing the variation of	  	  *
*               signal strength with gradient squared in calibration of non-	  *
*               uniform pulsed field gradients					  *
*										  *
*  profile_int	normalises the integral of the signal profile			  *
*										  *
**********************************************************************************/

/**********************************************************************************
*	        gradfit.c - Paul Bowyer, University of Manchester xi99		  *
*      	Calculates fit coefficients for DOSY experiments involving non-uniform    *
*      field gradients. The modules contained in this source code can be invoked  *
*      on the Vnmr command line or using the Magical macro 'calc_fitcoeff'. 	  *	
*										  *
**********************************************************************************/


/* Neatened up by Dan Iverson for VnmrJ compatibility 25 vii 03					*/

/*
	GAM 15i04	Use dosygamma parameter, don't assume proton calibration
	GAM 9i04 	Add DAC_to_G to head of fit_coefficients file
	GAM 9i04 	Correct reading/writing of Gradient_coefficients, correct no of coefficients in decay_gen
	GAM 9i04 	Fit gradient not gradient squared 
	GAM 9i04 	Add standard error to listing of diffusion coefficient fit in gradfit
	GAM 6i04 	Correct calculation of coefficients in powerfit/fitfunc:  
			exclude gamma, dosytimecubed etc from exponent, expand as function of grel=gzlvl/32767 squared
			change decay_gen to tabulate grel squared
	GAM 13v03 	correct coefficients in powerfit/fitfunc;  scale by X2SCALE=2e-9
	GAM 8v03 	profile_int:  reverse sign of frequency, and normalise integral not peak amplitude of profile 
	GAM 8v03 	profile_int:  allow for change of sign between display and wrspec
	GAM 7v03 	use 1st point of decay as initial guess for amplitude
	61CM version 3ix02 GAM
	Change to using directory Dosy/NUG
	i03	Enables Gradfit to fit to between 2 and 8 coefficients and Powerfit to between 
			3 and 5 coeffs.

	MC 13ii03	Prints out rms values for both fits.
	MC 14ii03 	Prints out reliance of fit for both gradfit and powerfit.
	MC 24ii03	Now calculates correct asymptotic standard error for both Gradfit() and Powerfit().	
	MC 27ii03	Gradfit() writes all data to a file called "write_file" in Dosy/NUG.

	GAM 13v03	powerfit now pads out fit_coefficient file to 8 lines  
	GAM 13v03	decay_gen now uses more precision in output  
	GAM 13v03	decay_gen now calculates its own endpoint at 10000-fold attenuation, so uses only 2 parameters   
	GAM 13v03	pad gradfit output to 9 coefficients    
	MC 12vii04      remove dosyfactor from all exponentials and derivatives 
	MC 12vii04      scale x[i] by dosyfactor before fitting process 
	MC 5v05         gradfit.c now compatible with LMN version
*/
/*	NEW VnmrJ VERSION	*/
/* 	9iv08 Change Dosy to dosy, DAC_to_G to gcal_, nugcal to nugcal_		*/
/* 	11iv08 Change from using userdir to curexpdir				*/
/* 	16ii09		GM change initial guess for 10,000-fold attenuation to 10/gcal_		*/
/* 			to allow for exotic values of gcal_ (e.g. from other manufacturers)	*/
/* 	16ii09		GM change output format of decay_gen ditto				*/
/* 	16ii09		GM change output format fit_coefft_stats_expl to show nominal gradient squared 	*/
/* 	17ii09		GM Neaten up displayed output				*/
/*	9iii09		GM close files properly if initial error in powerfit			*/
/*	9iii09		GM close files properly if initial error in decaygen, and check dosyfactor nonzero	*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "group.h"
#include "tools.h"
#include "data.h"
#include "variables.h"
#include "vnmrsys.h"
#include "pvars.h"
#include "wjunk.h"
#include "vnmr_gradfit_lm.h"

#define NPTD 12000
#define SPREAD 0.001
#define NR_END 1
#define FREE_ARG char*
#define ERROR 1
#define MAXLENGTH 128
#define MAXPROF_PTS 100000
#define MAX_POLY 8
#define MAXSIG_PTS 10000 
#define MAX_PROFILE_PTS 100000
#define MAX_ITERS 60
#define X2SCALE 2e-9  /* scaling factor for powerfit coefficients */
#define MAX_COEFS 9

/*  #define DEBUG_GRADFIT  1 */	

static double *vector(long nl, long nh);
static void free_vector(double *v, long nl, long nh);
static void free_matrix(double **m, long nrl, long nrh, long ncl, long nch);
static double **matrix(long nrl, long nrh, long ncl, long nch);
static void polyfunc(double x, double a[], double *y, double dyda[], int na);
static void fitfunc(double x, double a[], double *y, double dyda[], int na);
extern int interuption;
/* extern char userdir[]; */
static int MAD,MA;
static double dosygamma,gcal_,dosytimecubed,dosyfactor,Dcoeff;
static int abortflag;

/******************************************************************************

 	gradfit() - fitting of profile DOSY results using Levenberg Marquardt  
	Reads in data from diffusion_display.inp, normalises the data and fits
	to an 8th order polynomial. Coefficients written to a file in the current
	 directory called 'Gradient_coefficients' describe the variation of
	relative gradient with frequency displacement from the centre
	of the spectral width
	PB - 7vii99	

******************************************************************************/
int gradfit(int argc, char *argv[])
{
	FILE 	*in,
		*out,
		*fp,
		*plotgrads,
		*outgrads;
	int 	i,
		itst,
		j,
		k,
		mfit,
		np;
        int     ia[MAX_COEFS+1];
	double	rms,chisq,
		dum1,dum2,dum3,dum4,dum5,dum6,dum7,frscale,
		ochisq,z,
		*x,
		*y,*yerr,
		*yc,
		*sig,
		**covar,
		**alpha,
		scale,
		max_y,	
		lowfrq,
		highfrq,
		D_value;
	char    rubbish[MAXLENGTH];
	static double *a;
		
#ifdef DEBUG_GRADFIT
	        FILE    *debug;
	        strcpy(rubbish,curexpdir);
                strcat(rubbish,"/dosy/debug_gradfit");
	        debug = fopen(rubbish,"w");             /* file for debugging information */
		fprintf(debug,"Start of gradfit\n");
#endif
			
	
disp_status("gradfit ");

	if ((argc!=4)&&(argc!=5)){
		Werrprintf("gradfit: number of arguments incorrect\n");
		printf("Usage : gradfit(<lowfreq>,<highfreq>,<D_value>)");
		return(ERROR);
		}

	lowfrq = atof(argv[1]);
	highfrq = atof(argv[2]);
	D_value = atof(argv[3]);

	MA=MAX_COEFS;
	if (argc==5) MA=(int) atof(argv[4]);
	if ((MA>MAX_COEFS)||(MA<2))
	{
		Werrprintf("gradfit requires between 2 and %d coefficients to do fit.\n",MAX_COEFS);
		return(ERROR);
	}

	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/NUG/write_file");
	if ((fp = fopen(rubbish,"w")) == NULL) {
        Werrprintf("Error opening write_file.\n");
        return(ERROR);
        }


	mfit=MA;
	Wscrprintf("Diffusion coefficient for calibration = %e m2/s\n", D_value*1e-10);
	Wscrprintf("Fitting of relative gradient as a function of frequency\n");
	fprintf(fp,"Data discarded below %f\n",lowfrq);
	fprintf(fp,"Data discarded above %f\n",highfrq);
	fprintf(fp,"D_value : %e\n",D_value);

	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/diffusion_display.inp");
	if ((in = fopen(rubbish,"r")) == NULL) {
        Werrprintf("Error opening diffusion_display.inp.\n");
           fclose(fp);
        return(ERROR);
        }
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/NUG/Gradient_coefficients");
	if ((out = fopen(rubbish,"w")) == NULL) {
        Werrprintf("Error opening gradient coeff file.\n");
           fclose(in);
           fclose(fp);
        return(ERROR);
        }
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/NUG/Gradient_fit_stats");
	if ((outgrads = fopen(rubbish,"w")) == NULL) {
        Werrprintf("Error opening gradient file.\n");
           fclose(out);
           fclose(in);
           fclose(fp);
        return(ERROR);
        }
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/NUG/Gradient_fit_stats_expl");
	if ((plotgrads = fopen(rubbish,"w")) == NULL) {
        Werrprintf("Error opening expl file.\n");
           fclose(outgrads);
           fclose(out);
           fclose(in);
           fclose(fp);
        return(ERROR);
        }
	i=1;
	while ((fscanf(in,"%lf %lf %lf %lf %lf %lf %lf\n",&dum1,&dum2,&dum3,&dum4,&dum5,&dum6,&dum7)) !=EOF){
		if((dum1 >= lowfrq) && (dum1 <= highfrq)) i++;	/* calculate number of points from diffusion_display.inp	*/
		}
	np=i-1;
	fclose(in);
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/diffusion_display.inp");
	if ((in = fopen(rubbish,"r")) == NULL) {
        Werrprintf("Error opening diffusion_display.inp.\n");
           fclose(plotgrads);
           fclose(outgrads);
           fclose(out);
           fclose(fp);
        return(ERROR);
        }
		fprintf(fp,"MA = %d;  np = %d\n",MA,np);

        abortflag = 0;
	x=vector(1,np);
	a=vector(1,MA);
	y=vector(1,np);
	yerr=vector(1,np);
	yc=vector(1,np);
	sig=vector(1,np);
	covar=matrix(1,MA,1,MA);
	alpha=matrix(1,MA,1,MA);
        if (abortflag)
        {
	   free_matrix(alpha,1,MA,1,MA);
	   free_matrix(covar,1,MA,1,MA);
	   free_vector(sig,1,np);
   	   free_vector(yc,1,np);
	   free_vector(yerr,1,np);
	   free_vector(y,1,np);
	   free_vector(a,1,MA);
	   free_vector(x,1,np);
	   fclose(plotgrads);
	   fclose(outgrads);
           fclose(out);
	   fclose(fp);
	   fclose(in);
#ifdef DEBUG_GRADFIT
	   fclose(debug);
#endif
           disp_status("        ");
           return(ERROR);
        }

	if (MA==9) {  a[1]=0.0; a[2]=0.92; a[3]=4.7e-06; a[4]=-3.3e-06; a[5]=6.6e-11; a[6]=-1.4e-11; a[7]=0.0; a[8]=0.0; a[9]=0.0;
	}
	if (MA==8) {  a[1]=0.0; a[2]=0.92; a[3]=4.7e-06; a[4]=-3.3e-06; a[5]=6.6e-11; a[6]=-1.4e-11; a[7]=0.0; a[8]=0.0;
	}
	if (MA==7) {  a[1]=0.0; a[2]=0.92; a[3]=4.7e-06; a[4]=-3.3e-06; a[5]=6.6e-11; a[6]=-1.4e-11; a[7]=0.0;
	}
	if (MA==6) {  a[1]=0.0; a[2]=0.92; a[3]=4.7e-06; a[4]=-3.3e-06; a[5]=6.6e-11; a[6]=-1.4e-11;
	}
	if (MA==5) {  a[1]=0.0; a[2]=0.92; a[3]=4.7e-06; a[4]=-3.3e-06; a[5]=6.6e-11;
	}
	if (MA==4) {  a[1]=0.0; a[2]=0.92; a[3]=4.7e-06; a[4]=-3.3e-06;
	}
	if (MA==3) {  a[1]=0.0;	a[2]=0.92; a[3]=4.7e-06;
	}
	if (MA==2) {  a[1]=0.0; a[2]=0.92;
	}

#ifdef DEBUG_GRADFIT
		fprintf(debug,"Values of lowfrq and highfrq are %lf and %lf  \n",lowfrq,highfrq);
		fclose(debug);
	        strcpy(rubbish,curexpdir);
                strcat(rubbish,"/dosy/debug_gradfit");
	        debug = fopen(rubbish,"a");             /* file for debugging information */
#endif
		fprintf(fp,"Value of low freq = %f\n",lowfrq);
		fprintf(fp,"Value of high freq = %f\n",highfrq);
	i=1;
	max_y=0;
	frscale=fabs(lowfrq);
	if (fabs(highfrq)>frscale) frscale=fabs(highfrq);
	while ((fscanf(in,"%lf %lf %lf %lf %lf %lf %lf\n",&dum1,&dum2,&dum3,&dum4,&dum5,&dum6,&dum7 )) !=EOF){

		if((dum1 >= lowfrq) && (dum1 <= highfrq))	/* read in values from diffusion_display.inp, only use freq,D and std err	*/
		{
			x[i]=dum1/frscale; y[i]=dum3; yerr[i]=dum4;
			if(y[i] >= max_y) max_y=y[i];
	#ifdef DEBUG_GRADFIT
			fprintf(debug,"Values of i, x and y are %d, %f and %f  \n",i,x[i],y[i]);
	#endif
			 i++;
		}
		}
	#ifdef DEBUG_GRADFIT
			fprintf(debug,"Value of max_y is %f  \n",max_y);
			fprintf(debug,"Value of dum1 is %f  \n",dum1);
			fprintf(debug,"Value of dum2 is %f  \n",dum2);
			fprintf(debug,"Value of dum3 is %f  \n",dum3);
			fprintf(debug,"Value of dum4 is %f  \n",dum4);
			fprintf(debug,"Value of dum5 is %f  \n",dum5);
			fprintf(debug,"Value of dum6 is %f  \n",dum6);
			fprintf(debug,"Value of dum7 is %f  \n",dum7);
			fprintf(debug,"Number of points to fit to : %d\n",np);
			fprintf(debug,"%s\t\t%s\t%s\t%s\n","freq","diff coeff","normalised D","std error");
		fclose(debug);
	        strcpy(rubbish,curexpdir);
                strcat(rubbish,"/dosy/debug_gradfit");
	        debug = fopen(rubbish,"a");             /* file for debugging information */
	#endif
		fprintf(fp,"Number of points to fit to : %d\n",np);
		fprintf(fp,"%s\t\t%s\t%s\t%s\n","freq","diff coeff","normalised D","std error");
	
	for(i=1;i<=np;i++){
			y[i]/= D_value;	/* divide diffusion coeffs by lit value for D of H2O */
/* GAM 9i04 fit gradient not gradient squared */
			y[i]=sqrt(y[i]); 
			sig[i]= 1.0;
		#ifdef DEBUG_GRADFIT
				fprintf(debug,"%f\t%f\t%f\t%f\n",x[i],y[i]*D_value,y[i],yerr[i]); 
		#endif
				fprintf(fp,"%f\t%f\t%f\t%f\n",x[i],y[i]*D_value,y[i],yerr[i]);
			}
		fprintf(fp,"Value of np is = %d\n",np);
	for (i=1;i<=mfit;i++) ia[i]=1;

	#ifdef DEBUG_GRADFIT
		fprintf(debug,"About to start fitting!\n");
		fclose(debug);
	        strcpy(rubbish,curexpdir);
                strcat(rubbish,"/dosy/debug_gradfit");
	        debug = fopen(rubbish,"a");             /* file for debugging information */
	#endif
		lm_g_init(x,y,sig,np,a,ia,MA,covar,alpha,&chisq,polyfunc);
		k=1;
		itst=0;
		for (;;) {
			if(interuption)
			{
				Werrprintf("Gradfit halted\n");
				return(ERROR);
			}	
		
			#ifdef DEBUG_GRADFIT
				fprintf(debug,"\n%s %2d %17s %10.4f\n","Iteration #",k,
					"chi-squared:",chisq);
				fprintf(debug,"%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\t%8s\n",
					"a[1]","a[2]","a[3]","a[4]","a[5]","a[6]","a[7]","a[8]","a[9]");
				for (i=1;i<=MA;i++) Wscrprintf("%6.5e\t",a[i]);
				fprintf(debug,"\n");
			#endif
			k++;
			ochisq=chisq;
			lm_g_iterate(x,y,sig,np,a,ia,MA,covar,alpha,&chisq,polyfunc);
			if (chisq > ochisq)
				itst=0;
			else if (fabs(ochisq-chisq) < 0.1)
				itst++;
			if (itst < 100) continue;
			lm_g_covar(x,y,sig,np,a,ia,MA,covar,alpha,&chisq,polyfunc);

                        Wscrprintf("\nUncertainties for gradient fit parameters:\n");
			scale = sqrt((np-MA)/chisq);
                        for (i=1;i<=MA;i++) Wscrprintf("Coefficient no. %d   %9.4g\tstd. error \t%9.4g , %9.4g %% \n",i,a[i],(sqrt(covar[i][i]))/scale,fabs(100.0*sqrt(covar[i][i])/(scale*a[i])));
			Wscrprintf("\n");

			#ifdef DEBUG_GRADFIT
				Wscrprintf("\nUncertainties for gradient fit parameters:\n");
				for (i=1;i<=MA;i++) Wscrprintf("a[%d]\t%9.4e\n",i,(sqrt(covar[i][i]))/scale);
			#endif
			break;
		}
				Wscrprintf("Value of chi squared is %f \n",chisq);
				fprintf(fp,"chi squared is %f\n",chisq);
				fprintf(fp,"Value of max_y is %f\n",max_y);

		rms=sqrt(((chisq)/(double) np));
		Wscrprintf("RMS error is %f percent \n",100.0*rms/max_y);
				fprintf(fp,"RMS error is %f percent\n",100.0*rms/max_y);

	for(i=1;i<=MA;i++) fprintf(out,"%1.8e\tx%d\n", a[i]*pow(frscale,1.0-i),i-1);
	if (MA<9)
	{
	for(i=MA+1;i<=9;i++)
		{
		fprintf(out,"0.0\tx%d\n",i-1);
		}
	}
	fprintf(outgrads,"Fit of diffusion display to polynomial of order %d\n",MA);
	fprintf(outgrads,"Chi-squared for fit : %f\n",chisq);
	fprintf(outgrads,"%8s\t%8s\t%8s\n","Freq.","Exp. G","Calc. G");
	fprintf(plotgrads,"exp 5\n");
	fprintf(plotgrads,"  2  %d\n",np);
	fprintf(plotgrads,"Frequency\n");
	fprintf(plotgrads,"1  0  0  0\n");
/*
for (i+=MA;i<=8;i++)
{
a[i]=0.0;
}
*/
	for (i=1; i<=np; i++)	
	{ 
		yc[i]=a[1]; z=x[i];
		for (j=2;j<=MA;j++)
		{
			yc[i]=yc[i]+z*a[j];
			z=z*x[i];
		}
	   fprintf(outgrads,"%f\t%e\t%e\n",x[i],y[i],yc[i]);
	   fprintf(plotgrads,"%f\t%e\t\n",x[i],y[i]);
	}
	   fprintf(plotgrads,"\n");
	   fprintf(plotgrads,"2  0  0  0\n");
	for (i=1; i<=np; i++)	
	{ 
		yc[i]=a[1]; z=x[i];
		for (j=2;j<=MA;j++)
		{
			yc[i]=yc[i]+z*a[j];
			z=z*x[i];
		}
	   fprintf(plotgrads,"%f\t%e\t\n",x[i],yc[i]);
	}

	free_matrix(alpha,1,MA,1,MA);
	free_matrix(covar,1,MA,1,MA);
	free_vector(yc,1,np);
	free_vector(sig,1,np);
	free_vector(a,1,MA);
	free_vector(y,1,np);
	free_vector(yerr,1,np);
	free_vector(x,1,np);
	fclose(out);
	fclose(outgrads);
	fclose(plotgrads);
	fclose(fp);
	fclose(in);
disp_status("        ");
#ifdef DEBUG_GRADFIT
	fprintf(debug,"End of gradfit \n");
	fclose(debug);
#endif
	return 0;
}

/******************************************************************************

 	powerfit() - fitting of signal decay to an exponential of a power 
	series. Reads in data from Signal_atten_file in directory
	'~/vnmrsys/dosy/NUG'
	Fit coefficients written to a file in the same directory called
	'fit_coefficients'
	PB - 7vii99	

******************************************************************************/
int powerfit(argc, argv, retc, retv)
        char           *argv[], *retv[];
        int             argc, retc;
{
	FILE 	*in,
		*outstats,
		*plotstats,
		*outcoeff,
		*fp;
	char	dum1ch[80],
		rubbish[MAXLENGTH];
	int 	i, 
		r,
		itst,
		j,
		k,
		mfit,
		pad,
		np;
        int     ia[MAX_COEFS+1];
	double 	xx,
		dum2,dum3,
		rms,chisq,
		ochisq,z,
		*x,
		scale,
		*y,
		*yc,
		*x2,
		*sig,
		**covar,
		**alpha;
	static double *a,*gues;


#ifdef DEBUG_GRADFIT
	        FILE    *debug;
	        strcpy(rubbish,curexpdir);
                strcat(rubbish,"/dosy/debug_powerfit");
	        debug = fopen(rubbish,"w");             /* file for debugging information */
		fprintf(debug,"Start of powerfit\n");
#endif
	if ((argc!=1)&&(argc!=2))
	{
		Werrprintf("powerfit requires at most one argument\n");
#ifdef DEBUG_GRADFIT
		fclose(debug);
#endif
		return(ERROR);
	}
disp_status("powerfit");

	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/NUG/write_file");
	if ((fp = fopen(rubbish,"a")) == NULL) {
        Werrprintf("Error opening write_file.\n");
#ifdef DEBUG_GRADFIT
		fclose(debug);
#endif
        return(ERROR);
        }

		
	if (P_getreal(CURRENT,"dosytimecubed",&dosytimecubed,1)||P_getreal(CURRENT,"gcal_",&gcal_,1))
	{
#ifdef DEBUG_GRADFIT
		fclose(debug);
#endif
		fclose(fp);
		Werrprintf("Error accessing parameter dosytimecubed or gcal_\n");
		return(ERROR);
	}
	if (P_getreal(CURRENT,"dosygamma",&dosygamma,1))
	{
		Werrprintf("Error accessing parameter dosygamma\n");
#ifdef DEBUG_GRADFIT
		fclose(debug);
#endif
		fclose(fp);
		return(ERROR);
	}
	MAD=MAX_COEFS;
        pad=0;
/* GAM 13v03 */
	if (argc==2) 
	{
		MAD=(int) atof(argv[1]);
		pad=9-MAD;
	}

	if ((MAD>MAX_COEFS)||(MAD<2))
	{
		Werrprintf("powerfit requires between 2 and %d coefficients to do fit.\n", MAX_COEFS);
#ifdef DEBUG_GRADFIT
		fclose(debug);
#endif
		fclose(fp);
		return(ERROR);
	}
	
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/NUG/Signal_atten_file");
	if ((in = fopen(rubbish,"r")) == NULL) {
        	Werrprintf("Error opening Signal_atten_file.\n");
#ifdef DEBUG_GRADFIT
		fclose(debug);
#endif
		fclose(fp);
        	return(ERROR);
        	}
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/NUG/fit_coefficients");
	if ((outcoeff = fopen(rubbish,"w")) == NULL) {
        	Werrprintf("Error fit_coefficients file.\n");
#ifdef DEBUG_GRADFIT
		fclose(debug);
#endif
		fclose(in);
		fclose(fp);
        	return(ERROR);
        	}
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/NUG/fit_coeff_stats");
	if ((outstats = fopen(rubbish,"w")) == NULL) {
        	Werrprintf("Error fit_coeff_stats file.\n");
#ifdef DEBUG_GRADFIT
		fclose(debug);
#endif
		fclose(outcoeff);
		fclose(in);
		fclose(fp);
        	return(ERROR);
        	}
	strcpy(rubbish,curexpdir);
	strcat(rubbish,"/dosy/NUG/fit_coeff_stats_expl");
	if ((plotstats = fopen(rubbish,"w")) == NULL) {
        	Werrprintf("Error fit_coeff_stats file_expl.\n");
#ifdef DEBUG_GRADFIT
		fclose(debug);
#endif
		fclose(outstats);
		fclose(outcoeff);
		fclose(in);
		fclose(fp);
        	return(ERROR);
        	}
	mfit=MAD;
		#ifdef DEBUG_GRADFIT
			fprintf(debug,"powerfit :  mfit is %i  \n",mfit);
			fprintf(debug,"Value of dosytimecubed is %e  \n",dosytimecubed);
			fclose(debug);
		        strcpy(rubbish,curexpdir);
       		        strcat(rubbish,"/dosy/debug_powerfit");
		        debug = fopen(rubbish,"a");             /* file for debugging information */
		#endif
				fprintf(fp,"Value of mfit is %i\n",mfit);
				

/* GAM 13v03 */
	i=1;
	fgets(dum1ch,80,in);
	while ((fscanf(in,"%lf %lf \n",&dum2,&dum3)) !=EOF) i++;
	np=i-1;
			#ifdef DEBUG_GRADFIT
				fprintf(debug,"Value of np is %d  \n",np);
				fprintf(debug,"Value of dum1ch is %s  \n",dum1ch);
				fprintf(debug,"Value of dum2 is %f  \n",dum2);
				fprintf(debug,"Value of dum3 is %f  \n",dum3);
			fclose(debug);
		        strcpy(rubbish,curexpdir);
       		        strcat(rubbish,"/dosy/debug_powerfit");
		        debug = fopen(rubbish,"a");             /* file for debugging information */
			#endif
	(void)rewind(in);

        abortflag = 0;
	a=vector(1,MAD+1);
	gues=vector(1,MAD+1);
	x=vector(1,np);
	x2=vector(1,np);
	y=vector(1,np);
	yc=vector(1,np);
	sig=vector(1,np);
	covar=matrix(1,MAD,1,MAD);
	alpha=matrix(1,MAD,1,MAD);
        if (abortflag)
        {
	   free_matrix(alpha,1,MAD,1,MAD);
	   free_matrix(covar,1,MAD,1,MAD);
	   free_vector(sig,1,np);
	   free_vector(yc,1,np);
	   free_vector(y,1,np);
	   free_vector(x2,1,np);
	   free_vector(x,1,np);
	   free_vector(gues,1,MAD);
	   free_vector(a,1,MAD);
#ifdef DEBUG_GRADFIT
	   fclose(debug);
#endif
	   fclose(plotstats);
	   fclose(outstats);
	   fclose(outcoeff);
	   fclose(in);
	   fclose(fp);
           disp_status("        ");
           return(ERROR);
        }

	for(i=0;i<=MAD;i++)
	{
		a[i]=0.0; gues[i]=0.0;
	}
	gues[1]=1;
	fscanf(in,"Diffusion coefficient : %lf\n",&Dcoeff);
	#ifdef DEBUG_GRADFIT
		fprintf(debug,"Diffusion coefficient : %e\n", Dcoeff);
	#endif
	fprintf(fp,"Diffusion coefficient :  %e\n",Dcoeff);
/* GAM 6i04  x2 is now (gzlvl/32767)^2 ;  dosyfactor is separate */
	dosyfactor=Dcoeff*dosygamma*dosygamma*0.0001*dosytimecubed*32767*32767*gcal_*gcal_;
	i=1;
	while ((fscanf(in,"%lf\t%lf\n",&y[i],&x2[i])) !=EOF) i++;
			#ifdef DEBUG_GRADFIT
				fprintf(debug,"dosyfactor: %e\n",dosyfactor);
				fprintf(debug,"Number of points to fit to : %d\n",np);
				fprintf(debug,"%s\t\t%s\n","x2","y");
				for(i=1;i<=np;i++) fprintf(debug,"%f\t%f\n",x2[i],y[i]);
				fprintf(debug,"%s\t\t%s\t\t%s\n","y","x","s.d.");
			fclose(debug);
		        strcpy(rubbish,curexpdir);
       		        strcat(rubbish,"/dosy/debug_powerfit");
		        debug = fopen(rubbish,"a");             /* file for debugging information */
			#endif
				fprintf(fp,"Number of points to fit to : %d\n",np);
		
	for (i=1;i<=np;i++) {
/* GAM 13v03 */
/* MC 12vii04 : scale by dosyfactor here */
			x[i]=x2[i]*dosyfactor;
			sig[i]=1.0;
			#ifdef DEBUG_GRADFIT
				fprintf(debug,"%f\t%f\t%f\n",y[i],x[i],sig[i]);
			#endif
				fprintf(fp,"%f\t%f\t%f\n",y[i],x[i],sig[i]);
		}
		
	for (i=1;i<=mfit;i++) ia[i]=1;
	for (i=1;i<=MAD;i++) a[i]=gues[i];
/* GAM 7v03 */
	a[1]=y[1];

	#ifdef DEBUG_GRADFIT
		fprintf(debug,"About to start fitting!\n");
			fclose(debug);
		        strcpy(rubbish,curexpdir);
       		        strcat(rubbish,"/dosy/debug_powerfit");
		        debug = fopen(rubbish,"a");             /* file for debugging information */
	#endif
	Wscrprintf("Fitting of calculated signal attenuation to the exponential of a power series\n");
	lm_g_init(x,y,sig,np,a,ia,MAD,covar,alpha,&chisq,fitfunc);
	k=1;
	itst=0;
	for (;;) {
		if(interuption)
			{
				Werrprintf("Powerfit halted\n");
				return(ERROR);
			}	
		#ifdef DEBUG_GRADFIT	
			fprintf(debug,"\n%s %2d %17s %10.4f\n","Iteration #",k,
				"chi-squared:",chisq);
			fprintf(debug,"%8s %8s %8s %8s %8s %8s \n",
				"a[1]","a[2]","a[3]","a[4]","a[5]","a[6]");
			for (i=1;i<=MAD;i++) fprintf(debug,"%9.8e\t",a[i]);
			fprintf(debug,"\n");
			fclose(debug);
		        strcpy(rubbish,curexpdir);
       		        strcat(rubbish,"/dosy/debug_powerfit");
		        debug = fopen(rubbish,"a");             /* file for debugging information */
		#endif
		k++;
		ochisq=chisq;
		lm_g_iterate(x,y,sig,np,a,ia,MAD,covar,alpha,&chisq,fitfunc);
		if (chisq > ochisq)
			itst=0;
		else if (fabs(ochisq-chisq) < 0.1)
			itst++;
		if (itst < MAX_ITERS) continue;
		lm_g_covar(x,y,sig,np,a,ia,MAD,covar,alpha,&chisq,fitfunc);
                        Wscrprintf("\nUncertainties for signal decay fit parameters:\n");
			scale = sqrt((np-MAD)/chisq);	
                        for (i=2;i<=MAD;i++) Wscrprintf("Coefficient no. %d   %9.4g\tstd. error \t%9.4g , %9.4g %% \n",i-1,a[i],(sqrt(covar[i][i]))/scale,fabs(100.0*sqrt(covar[i][i])/(scale*a[i])));
	Wscrprintf("\n");

		#ifdef DEBUG_GRADFIT
			fprintf(debug,"\nUncertainties for signal decay fit parameters:\n");
			for (i=1;i<=MAD;i++) fprintf(debug,"a[%d]\t%9.4f\n",i,sqrt(covar[i][i]));
			fprintf(debug,"\n");
			fclose(debug);
		        strcpy(rubbish,curexpdir);
       		        strcat(rubbish,"/dosy/debug_powerfit");
		        debug = fopen(rubbish,"a");             /* file for debugging information */
		#endif
		break;
	}
	fprintf(outstats,"Fit of signal decay to exponential of a power series\n");
	fprintf(plotstats,"exp 5\n");
	fprintf(plotstats,"  2  %d\n",np);
	fprintf(plotstats,"natural log of signal vs (nominal gradient in G/cm) squared\n");
	fprintf(plotstats,"1  0  0  0\n");

		Wscrprintf("Chi squared is %e  \n",chisq);
				fprintf(fp,"value of y[1] is %e  \n",y[1]);
				fprintf(fp,"value of np is %d  \n",np);
				fprintf(fp,"Chi squared is %e  \n",chisq);

	rms=sqrt(((chisq)/(double) np));
		Wscrprintf("RMS error is %f   percent  \n",100.0*rms/y[1]);
				fprintf(fp,"RMS error is %f   percent  \n",100.0*rms/y[1]);

/* GAM 9i04 */
	fprintf(outcoeff,"gcal_  %e\n",gcal_);
       if ( (r = (P_setreal(CURRENT,"nugcal_",gcal_,0))) )
                Wscrprintf("Error setting parameter nugcal_\n");

	for(i=2;i<=MAD;i++) 
	{
	fprintf(outcoeff,"%e\n",a[i]);
       if ( (r = (P_setreal(CURRENT,"nugcal_",a[i],i))) )
                Wscrprintf("Error setting parameter nugcal_\n");
	}

/* GAM 13v03 */
	if (pad>0)
	{
		for (i=1;i<=pad;i++)
		{
			fprintf(outcoeff,"0.0\n");
		}
	}
	fprintf(outstats,"Chi-squared for fit : %e\n",chisq);
	fprintf(outstats,"%8s\t%8s\t%8s\n","Grad. area","Exp. S","Calc. S");
	for(i=1;i<=np;i++)
	{
		xx=x[i]; z=0.0;
		for (j=1;j<=MAD-1;j++)
/* GAM 13v03 */
/* MC 12vii04 : remove dosyfactor  */
/* GAM 16ii09 make horizontal axis of plot nominal gradient in G/cm squared  */
		{
			z=z+a[j+1]*pow(xx,(double)j);
		}
		yc[i]= a[1]*exp(-(z));
   		fprintf(outstats,"%f\t%e\t%e\n",x[i]/X2SCALE,y[i],yc[i]);
   		fprintf(plotstats,"%f\t%e\t\n",x[i]*32767.0*gcal_*32767.0*gcal_/dosyfactor,log(y[i]));
	}
		fprintf(plotstats,"\n");
		fprintf(plotstats,"2  0  0  0\n");

	for(i=1;i<=np;i++)
	{
		xx=x[i]; z=0.0;
		for (j=1;j<=MAD-1;j++)
		{
			z=z+a[j+1]*pow(xx,(double)j);
		}
		yc[i]= a[1]*exp(-z);
   		fprintf(plotstats,"%f\t%e\t\n",x[i]*32767.0*gcal_*32767.0*gcal_/dosyfactor,log(yc[i]));
	}

	free_matrix(alpha,1,MAD,1,MAD);
	free_matrix(covar,1,MAD,1,MAD);
	free_vector(x2,1,np);
	free_vector(yc,1,np);
	free_vector(sig,1,np);
	free_vector(y,1,np);
	free_vector(x,1,np);
	free_vector(a,1,MAD);
	free_vector(gues,1,MAD);
	fclose(in);
	fclose(fp);
	fclose(plotstats);
	fclose(outcoeff);
	fclose(outstats);
disp_status("        ");
	return 0;

}
/******************************************************************************

 	decay_gen() - Calculates a signal attenuation based upon an 
	unattenuated profile and a polynomial expression for the 
	gradient squared as a function of frequency for a specified read gradient
	Paul Bowyer - 9vii99	

******************************************************************************/
int decay_gen(int argc, char *argv[])
{
register int	i,
		j;
	int	nprof,
		niter,
		ngrads;
	FILE 	*in,
		*out,
		*gradfile;
	double 	d,
		maxGz,
		minGz,
		avGz,
		D,
	       	x[MAXSIG_PTS],
		x2[MAXSIG_PTS],
		grel2[MAXSIG_PTS],
	       	profile[MAXPROF_PTS][2],
		Gz[MAXPROF_PTS],
		Gz_squared[MAXPROF_PTS],
		S[MAXSIG_PTS],
		pcf[MAX_POLY+1],
		lowest_grad,
		highest_grad,
		gradinc,
		x2guess,
		prevsig;
	char 	rubbish[MAXLENGTH];

#ifdef DEBUG_GRADFIT
	        FILE    *debug;
#endif


#ifdef DEBUG_GRADFIT
	        strcpy(rubbish,curexpdir);
                strcat(rubbish,"/dosy/debug_decay_gen");
	        debug = fopen(rubbish,"w");             /* file for debugging information */
		fprintf(debug,"Start of decay_gen\n");
#endif
			
	if(argc !=3)
		{
		Werrprintf("decay_gen : number of arguments is incorrect!\n");
		Werrprintf("Usage : decay_gen(<diffcoeff>,<number of gradient values>)	\n");
	#ifdef DEBUG_GRADFIT
		fclose(debug);
	#endif
		return(ERROR);
		}
	#ifdef DEBUG_GRADFIT
        fprintf(debug,"Starting decay_gen\n");
	#endif

disp_status("decaygen");

        strcpy(rubbish,curexpdir);
        strcat(rubbish,"/dosy/NUG/Normalised_profile");
	if ((in = fopen(rubbish,"r")) == NULL) {
        Wscrprintf("Error opening profile file.\n");
	#ifdef DEBUG_GRADFIT
		fclose(debug);
	#endif
        return(ERROR);
        }
        strcpy(rubbish,curexpdir);
        strcat(rubbish,"/dosy/NUG/Gradient_coefficients");
	if ((gradfile = fopen(rubbish,"r")) == NULL) {
        Wscrprintf("Error opening Gradient_coefficients.\n");
	#ifdef DEBUG_GRADFIT
		fclose(debug);
	#endif
		fclose(in);
        return(ERROR);
        }
        strcpy(rubbish,curexpdir);
        strcat(rubbish,"/dosy/NUG/Signal_atten_file");
	if ((out = fopen(rubbish,"w")) == NULL) {
        Wscrprintf("Error opening signal attenuation file.\n");
	#ifdef DEBUG_GRADFIT
		fclose(debug);
	#endif
		fclose(in);
		fclose(gradfile);
        return(ERROR);
        }

	if (P_getreal(CURRENT,"dosytimecubed",&dosytimecubed,1)||P_getreal(CURRENT,"gcal_",&gcal_,1))
	{
		Werrprintf("Error accessing parameter dosytimecubed or gcal_\n");
	#ifdef DEBUG_GRADFIT
		fclose(debug);
	#endif
		fclose(in);
		fclose(out);
		fclose(gradfile);
		return(ERROR);
	}

	D = atof(argv[1]);
	ngrads = atoi(argv[2]);
/* GAM 13v03 */
	lowest_grad = 0.0;
	if(ngrads > MAXSIG_PTS) ngrads = MAXSIG_PTS;
	if (P_getreal(CURRENT,"dosygamma",&dosygamma,1))
	{
		Werrprintf("Error accessing parameter dosygamma\n");
	#ifdef DEBUG_GRADFIT
		fclose(debug);
	#endif
		fclose(in);
		fclose(out);
		fclose(gradfile);
		return(ERROR);
	}
	dosyfactor=gcal_*gcal_*0.0001*dosygamma*dosygamma*dosytimecubed;
	if (dosyfactor==0.0) 
	{
		Werrprintf("Product of gcal_, dosygamma and dosytimecubed is zero - decay generation halted\n");
	#ifdef DEBUG_GRADFIT
		fclose(debug);
	#endif
		fclose(in);
		fclose(out);
		fclose(gradfile);
		return(ERROR);
	}
	avGz=0.0;
	maxGz=0.0;
	minGz=1.0;
	i=0;
	while ((fscanf(in,"%lf \t %lf\n",&profile[i][0],&profile[i][1])) !=EOF) i++;
	nprof=i;

	for(i=0;i<MAX_POLY;i++) fscanf(gradfile,"%lf\t %s\n",&pcf[i],rubbish);
	#ifdef DEBUG_GRADFIT
		for(i=0;i<MAX_POLY;i++) fprintf(debug,"pcf[%d] : %e\n",i,pcf[i]);
	#endif
	d=profile[1][0]-profile[0][0];

	for(j=0;j<nprof;j++){
				Gz[j]=pcf[0]+profile[j][0]*pcf[1]+pow(profile[j][0],2.0)*pcf[2]+
					pow(profile[j][0],3.0)*pcf[3]+pow(profile[j][0],4.0)*pcf[4]+
					pow(profile[j][0],5.0)*pcf[5]+pow(profile[j][0],6.0)*pcf[6]+
					pow(profile[j][0],7.0)*pcf[7];
				avGz+=Gz[j];
				if(Gz[j]>maxGz) maxGz=Gz[j];
				if(Gz[j]<minGz) minGz=Gz[j];
				if (Gz[j]<0.0) 
				{
				Gz[j]=0.0;
	printf("Warning:  fitted gradient shape leads to negative apparent gradient\n");
				}
				}
	avGz/=nprof;
		Wscrprintf("Average value of relative gradient : %f\n",avGz);
	#ifdef DEBUG_GRADFIT
		fprintf(debug,"Number of profile points: %d\n\n",nprof);
		fprintf(debug,"Number of gradient values %d\n\n",ngrads);
		fprintf(debug,"dosytimecubed : %e\n",dosytimecubed);
		fprintf(debug,"gcal_ : %f\n",gcal_);
		fprintf(debug,"max G : %f   min G : %f \n",maxGz,minGz);
	#endif
/* GAM 13v03  Hunt for 10000-fold attenuation */
	i=0; 
	x2guess=10.0/gcal_;
	prevsig=1.0;
	for(niter=1;niter<11;niter++)
	{
	x2[0]=x2guess*x2guess*dosyfactor;
	S[i]=0.0;
	for(j=0;j<nprof;j++)
		{
/* GAM 9i04 coefficients read in are now for relative gradient not relative gradient squared */
		Gz_squared[j]=Gz[j]*Gz[j]*x2[i];
		if(j==0 || j==(nprof-1)) {
					S[i]+=(profile[j][1]*exp(-D*Gz_squared[j])*3.0/8.0);
					}
		else if((j==1) || j==(nprof-2)) {
					S[i]+=(profile[j][1]*exp(-D*Gz_squared[j])*7.0/6.0);
					}
		else if((j==2) || j==(nprof-3)) {
					S[i]+=(profile[j][1]*exp(-D*Gz_squared[j])*23.0/24.0);
					}
		else			S[i]+=(profile[j][1]*exp(-D*Gz_squared[j]));
		}
	x2guess=x2guess/sqrt(log(S[i])/log(0.0001));
	if (S[i]>prevsig)
	{
	printf("Warning:  fitted gradient shape leads to calculated attenuation curve minimum at around %f DAC points\n",x2guess);
	break;
	}
	prevsig=S[i];
	}

	highest_grad = x2guess;  /* in DAC points */
	#ifdef DEBUG_GRADFIT
        fprintf(debug,"highest_grad = %f\n",highest_grad);
	#endif
	gradinc = (highest_grad - lowest_grad)/(double) (ngrads-1);

	for(i=0;i<ngrads;i++){
			 x[i] = lowest_grad + i*gradinc;
			 grel2[i] = x[i]*x[i]/(32767*32767);
			 x[i] *= (gcal_ * 1e-2);
		 	 x2[i] = x[i]*x[i]*dosygamma*dosygamma*dosytimecubed;
			}
	for(i=0;i<ngrads;i++){
			if(interuption)
			{
				Werrprintf("decay_gen halted\n");
				return(ERROR);
			}	
			S[i]=0.0;
			for(j=0;j<nprof;j++)
				{
/* GAM 9i04 coefficients read in are now for relative gradient not relative gradient squared */
		                Gz_squared[j]=Gz[j]*Gz[j]*x2[i];
				if(j==0 || j==(nprof-1)) {
							S[i]+=(profile[j][1]*exp(-D*Gz_squared[j])*3.0/8.0);
							}
				else if((j==1) || j==(nprof-2)) {
							S[i]+=(profile[j][1]*exp(-D*Gz_squared[j])*7.0/6.0);
							}
				else if((j==2) || j==(nprof-3)) {
							S[i]+=(profile[j][1]*exp(-D*Gz_squared[j])*23.0/24.0);
							}
				else			S[i]+=(profile[j][1]*exp(-D*Gz_squared[j]));
				}
	
/* GAM 8v03 don't scale integral by no of Hz per point  */
/*			S[i] *= d;			*/
			if(i==0){
				fprintf(out,"Diffusion coefficient : %e\n",D);
				}
		
			fprintf(out,"%10.8f\t%20.18f\n",S[i],grel2[i]);
			}
	#ifdef DEBUG_GRADFIT
		for(i=0;i<MAX_POLY;i++) Wscrprintf("pcf[%d] : %f\n",i,pcf[i]);
		fprintf(debug,"Signal attenuation and x2 written to file 'Signal_atten_file'\n");
		fprintf(debug,"Diffusion coefficient for decay : %e\n",D);
		fclose(debug);
	#endif
	fclose(in);
	fclose(out);
	fclose(gradfile);

disp_status("        ");
	return 0;
}


/******************************************************************************

	Profile_integrate() 
	Integrates a profile, normalises it, and writes it to a file 
	Paul Bowyer - 17xi99	

******************************************************************************/

int profile_int(int argc, char *argv[]) {

int 	i,
	nprof,
	expnum,
	profnum;
double 	profile[MAX_PROFILE_PTS][2];
double 	maxamp,
	sum,
	one_fifthamp_frq,
	integ,
	tmp;
double	lowfrq,
	highfrq;
char	rubbish[MAXLENGTH];

FILE 	*profilein,
	*profileout;
#ifdef DEBUG_GRADFIT
double  fmax = 0.0;
#endif

disp_status("integr. ");
if(argc!=3)
	{
	Werrprintf("Profile integrate: number of arguments incorrect\n");
	Werrprintf("Usage : <lowfreq> <highfreq>\n");
        return(ERROR);
	}
lowfrq = atof(argv[1]);
highfrq = atof(argv[2]);

        strcpy(rubbish,curexpdir);
        strcat(rubbish,"/dosy/NUG/Signal_profile");
	if ((profilein = fopen(rubbish,"r")) == NULL) {
        Werrprintf("Error opening Signal_profile in profile_int.\n");
        return(ERROR);
        }

        strcpy(rubbish,curexpdir);
        strcat(rubbish,"/dosy/NUG/Normalised_profile");
	if ((profileout = fopen(rubbish,"w")) == NULL) {
        Werrprintf("Error opening Normalised_profile in profile_int.\n");
        return(ERROR);
        }


if (lowfrq > highfrq)
		{ tmp = lowfrq;
		  lowfrq = highfrq;
		  highfrq = tmp;
		}
#ifdef DEBUG_GRADFIT
	Wscrprintf("Data discarded below %f Hz\n", lowfrq);
	Wscrprintf("Data discarded above %f Hz\n", highfrq);
#endif

maxamp = 0.0;
sum = 0.0;
if((fscanf(profilein,"exp %d \n",&expnum) == EOF)
   || (fscanf(profilein," 1  %d\n",&profnum) == EOF)
   || (fscanf(profilein,"Frequency (hz) vs Amplitude\n") == EOF)
   || (fscanf(profilein,"\n") == EOF)
   || (fscanf(profilein,"1  0  0  0\n") == EOF))	
	{
	Werrprintf("Reached end of Signal profile file\n");
	return(ERROR);
	}
i=0;
/* GAM 8v03 allow for change of sign of frequency between display and wrspec */
while ((fscanf(profilein,"%lf\t%lf\n",&profile[i][0],&profile[i][1]))!=EOF) 
	{
	profile[i][0]=-profile[i][0];
	 if((profile[i][0] >= lowfrq) && (profile[i][0] <= highfrq)) i++;
	}
nprof=i-1;
#ifdef DEBUG_GRADFIT
	Wscrprintf("number of points in profile: %d\n",nprof);
#endif
for(i=0;i<nprof;i++){
	if(profile[i][1] > maxamp) {
		maxamp = profile[i][1];
#ifdef DEBUG_GRADFIT
		fmax = profile[i][0];
#endif
		}
	}
#ifdef DEBUG_GRADFIT
	Wscrprintf("Peak profile amplitude %f at %f Hz\n",maxamp,fmax);
#endif

i=0;
while  (profile[i++][1] <= (maxamp/5.0));   /* Empty while loop. Just setting i */
one_fifthamp_frq = profile[i-1][0];
#ifdef DEBUG_GRADFIT
	Wscrprintf("Frequency at one fifth peak amplitude : %f\n", one_fifthamp_frq);
#endif
/* GAM 8v03 don't scale integral by no of Hz per point */
integ=0.0;
for(i=0;i<nprof;i++){
	if((i==0) || (i==(nprof-1))) integ+=profile[i][1]*3.0/8.0;
	else if ((i==1) || (i==(nprof-2))) integ+=profile[i][1]*7.0/6.0;
	else if ((i==2) || (i==(nprof-3)))  integ+=profile[i][1]*23.0/24.0;
	else integ+=profile[i][1];
	}
#ifdef DEBUG_GRADFIT
	Wscrprintf("Integral of profile using extended formula : %f\n", integ);
#endif
/* GAM 8v03 normalise integral not peak amplitude of profile;  write out profile with frequencies ascending */
for(i=nprof-1;i>=0;i--){		/* normalise profile */
		profile[i][1]/= integ;
		fprintf(profileout,"%f \t %e\n",profile[i][0],profile[i][1]);
		}
integ=0.0;
for(i=0;i<nprof;i++){			/* check integral again!!	*/
	if((i==0) || (i==(nprof-1))) integ+=profile[i][1]*3.0/8.0;
	else if ((i==1) || (i==(nprof-2))) integ+=profile[i][1]*7.0/6.0;
	else if ((i==2) || (i==(nprof-3))) integ+=profile[i][1]*23.0/24.0;
	else integ+=profile[i][1];
	}
#ifdef DEBUG_GRADFIT
	Wscrprintf("Integral of profile after normalisation: %f\n", integ);
#endif

fclose(profilein);
fclose(profileout);

					
disp_status("        ");
return 0;

}

static double *vector(long nl, long nh)
/* allocate a float vector with subscript range v[nl..nh] */
{
	double *v;
	v=(double *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(double)));
	if (!v)
        {
           Werrprintf("allocation failure in vector()");
           abortflag = 1;
           return(v);
        }
	return v-nl+NR_END;
}

double **matrix(long nrl, long nrh, long ncl, long nch)
/* allocate a float matrix with subscript range m[nrl..nrh][ncl..nch] */
{
	long i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
	double **m;
	/* allocate pointers to rows */
	m=(double **) malloc((size_t)((nrow+NR_END)*sizeof(double*)));
	if (!m)
        {
           Werrprintf("allocation failure 1 in matrix()");
           abortflag = 1;
           return(m);
        }
	m += NR_END;
	m -= nrl;
	/* allocate rows and set pointers to them */
	m[nrl]=(double *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double)));
	if (!m[nrl])
        {
           Werrprintf("allocation failure 2 in matrix()");
           abortflag = 1;
           return(m);
        }
	m[nrl] += NR_END;
	m[nrl] -= ncl;
	for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;
	/* return pointer to array of pointers to rows */
	return m;
}
void free_vector(double *v, long nl, long nh)
/* free a float vector allocated with vector() */
{
   if (v)
	free((FREE_ARG) (v+nl-NR_END));
}
void free_matrix(double **m, long nrl, long nrh, long ncl, long nch)
/* free a float matrix allocated by matrix() */
{
   if (m && m[nrl])
	free((FREE_ARG) (m[nrl]+ncl-NR_END));
   if (m)
	free((FREE_ARG) (m+nrl-NR_END));
}
void polyfunc(double x, double a[], double *y, double dyda[], int na)
{
	if (MA==9) {
		*y= a[1]+a[2]*x+a[3]*pow(x,2.0)+a[4]*pow(x,3.0)+a[5]*pow(x,4.0)+a[6]*pow(x,5.0)+a[7]*pow(x,6.0)+a[8]*pow(x,7.0)+a[9]*pow(x,8.0);
		dyda[1] = 1.0;
		dyda[2] = x;
		dyda[3] = pow(x,2.0);
		dyda[4] = pow(x,3.0);
		dyda[5] = pow(x,4.0);
		dyda[6] = pow(x,5.0);
		dyda[7] = pow(x,6.0);
		dyda[8] = pow(x,7.0);
		dyda[9] = pow(x,8.0);
		}

	if (MA==8) {
		*y= a[1]+a[2]*x+a[3]*pow(x,2.0)+a[4]*pow(x,3.0)+a[5]*pow(x,4.0)+a[6]*pow(x,5.0)+a[7]*pow(x,6.0)+a[8]*pow(x,7.0);
		dyda[1] = 1.0;
		dyda[2] = x;
		dyda[3] = pow(x,2.0);
		dyda[4] = pow(x,3.0);
		dyda[5] = pow(x,4.0);
		dyda[6] = pow(x,5.0);
		dyda[7] = pow(x,6.0);
		dyda[8] = pow(x,7.0);
		}

	if (MA==7) {
		*y= a[1]+a[2]*x+a[3]*pow(x,2.0)+a[4]*pow(x,3.0)+a[5]*pow(x,4.0)+a[6]*pow(x,5.0)+a[7]*pow(x,6.0);
		dyda[1] = 1.0;
		dyda[2] = x;
		dyda[3] = pow(x,2.0);
		dyda[4] = pow(x,3.0);
		dyda[5] = pow(x,4.0);
		dyda[6] = pow(x,5.0);
		dyda[7] = pow(x,6.0);
		}

	if (MA==6) {
		*y= a[1]+a[2]*x+a[3]*pow(x,2.0)+a[4]*pow(x,3.0)+a[5]*pow(x,4.0)+a[6]*pow(x,5.0);
		dyda[1] = 1.0;
		dyda[2] = x;
		dyda[3] = pow(x,2.0);
		dyda[4] = pow(x,3.0);
		dyda[5] = pow(x,4.0);
		dyda[6] = pow(x,5.0);
		}

	if (MA==5) {
		*y= a[1]+a[2]*x+a[3]*pow(x,2.0)+a[4]*pow(x,3.0)+a[5]*pow(x,4.0);
		dyda[1] = 1.0;
		dyda[2] = x;
		dyda[3] = pow(x,2.0);
		dyda[4] = pow(x,3.0);
		dyda[5] = pow(x,4.0);
		}

	if (MA==4) {
		*y= a[1]+a[2]*x+a[3]*pow(x,2.0)+a[4]*pow(x,3.0);
		dyda[1] = 1.0;
		dyda[2] = x;
		dyda[3] = pow(x,2.0);
		dyda[4] = pow(x,3.0);
		}

	if (MA==3) {
		*y= a[1]+a[2]*x+a[3]*pow(x,2.0);
		dyda[1] = 1.0;
		dyda[2] = x;
		dyda[3] = pow(x,2.0);
		}

	if (MA==2) {
		*y= a[1]+a[2]*x;
		dyda[1] = 1.0;
		dyda[2] = x;
		}
}

void fitfunc(double x, double a[], double *y, double dyda[], int na)
{
double expval;  
/* GAM 6-9i04 */
/* MC 12vii04 : remove dosyfactor from all exponentials and derivatives */
	if (MAD==9) {
	expval= exp(-(a[2]*x+a[3]*pow(x,2.0)+a[4]*pow(x,3.0)+a[5]*pow(x,4.0)+a[6]*pow(x,5.0)+a[7]*pow(x,6.0)+a[8]*pow(x,7.0)+a[9]*pow(x,8.0)));
	*y= a[1]*expval;
	dyda[1] = expval;
	dyda[2] = -a[1]*x*expval;
	dyda[3] = -a[1]*pow(x,2.0)*expval;
	dyda[4] = -a[1]*pow(x,3.0)*expval;
	dyda[5] = -a[1]*pow(x,4.0)*expval;
	dyda[6] = -a[1]*pow(x,5.0)*expval;
	dyda[7] = -a[1]*pow(x,6.0)*expval;
	dyda[8] = -a[1]*pow(x,7.0)*expval;
	dyda[9] = -a[1]*pow(x,8.0)*expval;
	}
	if (MAD==8) {
	expval= exp(-(a[2]*x+a[3]*pow(x,2.0)+a[4]*pow(x,3.0)+a[5]*pow(x,4.0)+a[6]*pow(x,5.0)+a[7]*pow(x,6.0)+a[8]*pow(x,7.0)));
        *y= a[1]*expval;
        dyda[1] = expval;
	dyda[2] = -a[1]*x*expval;
	dyda[3] = -a[1]*pow(x,2.0)*expval;
	dyda[4] = -a[1]*pow(x,3.0)*expval;
	dyda[5] = -a[1]*pow(x,4.0)*expval;
	dyda[6] = -a[1]*pow(x,5.0)*expval;
	dyda[7] = -a[1]*pow(x,6.0)*expval;
	dyda[8] = -a[1]*pow(x,7.0)*expval;
	}
	if (MAD==7) {
	expval= exp(-(a[2]*x+a[3]*pow(x,2.0)+a[4]*pow(x,3.0)+a[5]*pow(x,4.0)+a[6]*pow(x,5.0)+a[7]*pow(x,6.0)));
        *y= a[1]*expval;
        dyda[1] = expval;
	dyda[2] = -a[1]*x*expval;
	dyda[3] = -a[1]*pow(x,2.0)*expval;
	dyda[4] = -a[1]*pow(x,3.0)*expval;
	dyda[5] = -a[1]*pow(x,4.0)*expval;
	dyda[6] = -a[1]*pow(x,5.0)*expval;
	dyda[7] = -a[1]*pow(x,6.0)*expval;
	}
	if (MAD==6) {
        expval= exp(-(a[2]*x+a[3]*pow(x,2.0)+a[4]*pow(x,3.0)+a[5]*pow(x,4.0)+a[6]*pow(x,5.0)));
        *y= a[1]*expval; 
        dyda[1] = expval; 
	dyda[2] = -a[1]*x*expval;
	dyda[3] = -a[1]*pow(x,2.0)*expval;
	dyda[4] = -a[1]*pow(x,3.0)*expval;
	dyda[5] = -a[1]*pow(x,4.0)*expval;
	dyda[6] = -a[1]*pow(x,5.0)*expval;
	}
	if (MAD==5) {
        expval= exp(-(a[2]*x+a[3]*pow(x,2.0)+a[4]*pow(x,3.0)+a[5]*pow(x,4.0)));
        *y= a[1]*expval; 
        dyda[1] = expval; 
	dyda[2] = -a[1]*x*expval;
	dyda[3] = -a[1]*pow(x,2.0)*expval;
	dyda[4] = -a[1]*pow(x,3.0)*expval;
	dyda[5] = -a[1]*pow(x,4.0)*expval;
	}
	if (MAD==4) {
        expval= exp(-(a[2]*x+a[3]*pow(x,2.0)+a[4]*pow(x,3.0)));
        *y= a[1]*expval; 
        dyda[1] = expval; 
	dyda[2] = -a[1]*x*expval;
	dyda[3] = -a[1]*pow(x,2.0)*expval;
	dyda[4] = -a[1]*pow(x,3.0)*expval;
	}
	if (MAD==3) {
        expval= exp(-(a[2]*x+a[3]*pow(x,2.0)));
        *y= a[1]*expval; 
        dyda[1] = expval; 
	dyda[2] = -a[1]*x*expval;
	dyda[3] = -a[1]*pow(x,2.0)*expval;
	}
	if (MAD==2) {
        expval= exp(-(a[2]*x));
        *y= a[1]*expval; 
        dyda[1] = expval; 
	dyda[2] = -a[1]*x*expval;
	}
}
