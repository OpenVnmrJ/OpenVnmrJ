/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* 
	Add fitting for nonlinear gradients 3ix02
	Only close file if exists 3ix02
	Try to open cfile whether or not calibflag
	Only calculate gradient calibration data if > 1 peak
	Initialise nonlinflag to 0 to allow backward compatibility with dosy macro
	Try adding facility to correct for baseplane noise in absolute value 3D

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vnmrsys.h"
#include "data.h"
#include "disp.h"
#include "init2d.h"
#include "graphics.h"
#include "group.h"
#include "pvars.h"
#include "wjunk.h"

static double	sqrarg;

#define ERROR	1
#define TRUE	1
#define FALSE	0
#define MAXLENGTH	250
#define MAXPEAKS_2D	512
#define MAXPEAKS_3D	8192
#define MAXPOINTS	200
#define NPARAMS		4
#define MAXITERS	4
#define MAXFITCOEFFS    5
#define SQR(a)		(sqrarg=(a),sqrarg*sqrarg)
#define SWAP(a,b)       {float temp=(a);(a)=(b);(b)=temp;}
#define LABEL_LEN	20
#define COMMENT_LEN	80
#define MAX_ERROR	50.0	/* Maximum error permitted on the diffusion value, in per cent */

static double	alpha[NPARAMS][NPARAMS],beta[NPARAMS],big,covar[NPARAMS][NPARAMS],oneda[NPARAMS][NPARAMS],yt1,yt2,yt3,ochisq,fc[MAXFITCOEFFS];
static int	icol,irow,lr,mfit,n,ma,np,abortflag,nonlinflag;
static double	prevchisq;
extern char     curexpdir[];
extern char     userdir[];

extern void fclosetest(FILE *file);
/*****************************************************************************
*  structure for a peak record
*****************************************************************************/
typedef	struct pk_struct {
        double          f1, f2;
        double          amp;
        double          f1_min, f1_max,
                        f2_min, f2_max;
        double          fwhh1, fwhh2;
        double          vol;
        int             key;
        struct pk_struct *next;
        char            label[LABEL_LEN+1];
        char            comment[COMMENT_LEN+1];
        } peak_struct;
 

/*****************************************************************************
*  structure for peak table
*       First entry in header is the number of peaks in the table, next
*       entries tell whether the corresponding key is in use (PEAK_FILE_FULL)
*       or not (PEAK_FILE_EMPTY). (i.e. header[20] tells whether a peak with
*       key 20 is currently in existence.)
*****************************************************************************/
typedef	struct {
        int   num_peaks;
        FILE *file;
        float version;
        peak_struct     *head;
        short   header[MAXPEAKS_3D+1];
        char            f1_label,f2_label;
        int experiment;
        int planeno;
        } peak_table_struct;

peak_table_struct	**peak_table;

	/*---------------------------------------
	|					|
	|	        dosyfit()		|
	|					|
	|--------------------------------------*/

int dosyfit3D(int argc, char *argv[])

{
register int	i,j,k,l;
int	itst,lista[NPARAMS],errorflag,badinc,badflag,ct,calibflag,bd[MAXPOINTS],ll2dflg;
double	alamda,y[MAXPOINTS],x[MAXPOINTS],frq[MAXPEAKS_2D],x2[MAXPOINTS],a[NPARAMS],sig[MAXPOINTS],chisq;
char	rubbish[MAXLENGTH],expname[MAXLENGTH],fname[MAXLENGTH],nonlinname[MAXLENGTH];
char	str='\0',str1[LABEL_LEN+1],jstr[MAXLENGTH];
void	mrqmindosy(double *,int *,double *,double *,double *,double *,double *,double *,int *);
double	fac,dosyconstant,DAC_to_Gauss;
double	xc[MAXPOINTS],yc[MAXPOINTS],xdiff[MAXPOINTS],peak_height_sum[MAXPOINTS];
double	y_diff_wa[MAXPOINTS],y_sum[MAXPOINTS],y_dd[MAXPOINTS],y_std[MAXPOINTS];
FILE	*table,*fit_results,*D_spectrum,*cfile,*errorfile,*in,*old_ptr,*fitfile;
int     rej_pk[MAXPOINTS];
int	io,jo,ko,lo,xo,xt,xthigh,xtlow,maxpeaks,systematic_dev_flg;
float	xtf,dlow,dhigh;
double	**y_diff,**y_fitted;
double  avnoise;

peak_struct	**peak = NULL;
extern peak_struct *	create_peak(/*f1, f2, amp*/);
extern int	read_peak_file();
extern void	write_peak_file_record(/*peak_table,peak,record*/);
extern void	delete_peak_table(/*peak_table*/);

abortflag = 0;
nonlinflag = 0;
systematic_dev_flg = FALSE;
ll2dflg = FALSE;
fitfile = NULL;
old_ptr = NULL;
if (argc > 1)
	{
	if (!strcmp(argv[1],"3D")) 
		{
		ll2dflg = TRUE;
		maxpeaks = MAXPEAKS_3D;
		}
	else
		{
		Werrprintf("dosyfit: unexpected argument %s\n",argv[1]);
		return(ERROR);
		}
	}
else	maxpeaks = MAXPEAKS_2D;
if (ll2dflg)
	{
	avnoise=0.0;
	if (argc>2)
		{
		avnoise=atof(argv[2]);
		printf("Average noise per Hz squared is %f\n",avnoise);
		}
	peak_table = (peak_table_struct **) malloc(MAXPOINTS*sizeof(peak_table_struct *));
	if (!peak_table)
		{
		Werrprintf("dosyfit: Could not allocate memory for peak_table array!");
		return(ERROR);
		}
	for (i=0;i<MAXPOINTS;i++)	peak_table[i] = NULL;
	peak = (peak_struct **) malloc(MAXPOINTS*sizeof(peak_struct *));
	if (!peak)
		{
		Werrprintf("dosyfit: Could not allocate memory for peak_struct array!");
		free(peak_table);
		return(ERROR);
		}
	for (i=0;i<MAXPOINTS;i++)
		{
		peak[i] = create_peak(0,0,0.0);
		}
	}
strcpy(rubbish,userdir);
strcat(rubbish,"/Dosy/dosy_in");
in = fopen(rubbish,"r");		/* input file for 2D DOSY */
if (!in)
{
        Werrprintf("dosyfit: could not open file %s",rubbish);
        if (ll2dflg) { free(peak); free(peak_table); }
        return(ERROR);
}

strcpy(rubbish,userdir);
if (!ll2dflg)	strcat(rubbish,"/Dosy/diffusion_display.inp");
else		strcat(rubbish,"/Dosy/diffusion_display_3D.inp");
table = fopen(rubbish,"w");	/* output file for 2D DOSY */
if (!table)
        {
                Werrprintf("dosyfit: could not open file %s",rubbish);
                fclosetest(in);
                return(ERROR);
        }
	 
strcpy(rubbish,userdir);
strcat(rubbish,"/Dosy/general_dosy_stats");
fit_results = fopen(rubbish,"w");	/* detailed fitting output */

if (!fit_results)
        {
                Werrprintf("dosyfit: could not open file %s",rubbish);
                fclosetest(in);
                return(ERROR);
        }
strcpy(rubbish,userdir);
strcat(rubbish,"/Dosy/diffusion_spectrum");
D_spectrum = fopen(rubbish,"w");	/* diffusion spectrum, input for dsp */
if (!D_spectrum)
        {
                Werrprintf("dosyfit: could not open file %s",rubbish);
                fclosetest(in);
                return(ERROR);
        }

strcpy(rubbish,userdir);
strcat(rubbish,"/Dosy/fit_errors");
errorfile = fopen(rubbish,"w");		/* failed fits signaled in this file */
if (!errorfile)
        {
                Werrprintf("dosyfit: could not open file %s",rubbish);
                fclosetest(in);
                return(ERROR);
        }

/* Manchester 6.1B:  use older version of dosy_in read, with error checking   */

strcpy(jstr,userdir);
strcat(jstr,"/Dosy/dosy_in");
if (fscanf(in,"%d spectra will be deleted from the analysis :\n",&badinc) == EOF)
{
        Werrprintf("dosyfit: reached end of file %s", jstr);
        if (ll2dflg) { free(peak); free(peak_table); }
        else fclosetest(table);
        fclosetest(in);
        return(ERROR);
}
for (i=0;i<badinc;i++)
{
        if (fscanf(in,"%d\n",&bd[i]) == EOF)
        {
                Werrprintf("dosyfit: reached end of file %s", jstr);
                if (ll2dflg) { free(peak); free(peak_table); }
                else fclosetest(table);
                fclosetest(in);
                return(ERROR);
        }
}

if ((fscanf(in,"Analysis on %d peaks\n",&n) == EOF) /* read in calibration parameters */
        || (fscanf(in,"%d points per peaks\n",&np) == EOF)
        || (fscanf(in,"%d parameters fit\n",&mfit) == EOF)
        || (fscanf(in,"dosyconstant = %lf\n",&dosyconstant) == EOF)
        || (fscanf(in,"gradient calibration flag :  %d\n",&calibflag) == EOF)
        || (fscanf(in,"non-linear gradient flag :  %d\n",&nonlinflag) == EOF))
{
        Werrprintf("dosyfit: reached end of file %s", jstr);
        if (ll2dflg) { free(peak); free(peak_table); }
        else fclosetest(table);
        fclosetest(in);
        return(ERROR);
}

DAC_to_Gauss=1.0; /* Not implemented */

 
strcpy(rubbish,userdir);
strcat(rubbish,"/Dosy/calibrated_gradients");
        cfile = fopen(rubbish,"r");
	if (!cfile)
		{
		Werrprintf("dosyfit: could not open file %s",rubbish);
	        if (ll2dflg) { free(peak); free(peak_table); } else fclosetest(table);
       	 	fclosetest(in);
        	return(ERROR);
		}
strcpy(rubbish,userdir); 
strcat(rubbish,"/Dosy/NUG/");
if(P_getstring(CURRENT,"nonlinfile",nonlinname,1,MAXLENGTH))
        {
        strcpy(nonlinname,"fit_coefficients");
        }
strcat(rubbish,nonlinname);
if (nonlinflag)
        {
        fitfile = fopen(rubbish,"r");
        if (!fitfile)
                {
                Werrprintf("dosyfit: could not open file %s",rubbish);
                if (ll2dflg) { free(peak); free(peak_table); } else fclosetest(table);
                fclosetest(in);
                return(ERROR);
                }
        }

if (ll2dflg)
        {
        strcpy(expname,curexpdir);
        for (i=1,l=0;i<=(np+badinc);i++,l++)
                {
                strcpy(fname,expname);
                strcat(fname,"/ll2d/peaks.bin.");
                j = 0;
                if (i >= 10)
                        {   
                        j = i/10;
                        str = j+'0';
                        strncat(fname,&str,1);
                        }
                str = (i-j*10)+'0';
                strncat(fname,&str,1);
                if (read_peak_file(&peak_table[i-1],fname))
                        {
                        Werrprintf("dosyfit: Could not read peak file n %d !\n",i);
                        free(peak_table);
                        free(peak);
                        return(ERROR);
                        }
                if (i == 1)
                        {
                        n = peak_table[i-1]->num_peaks;
                        }
                peak[i-1] = peak_table[i-1]->head;
                }
i=1;
        }

if (n > maxpeaks)
        {
        if (!ll2dflg)   fprintf(table,"Too many peaks for the analysis !\n");
        Werrprintf("dosyfit: Too many peaks for the analysis !\n");
        if (ll2dflg) { free(peak_table); free(peak); }
        else fclosetest(table);
        fclosetest(in);
	fclosetest(cfile);
        return(ERROR);
        }
if (np > MAXPOINTS)
        {
        Werrprintf("dosyfit: Too many increments for analysis, max. nunmber of gzlvl1 is %d\n", MAXPOINTS);
        if (ll2dflg) { free(peak_table); free(peak); }
        else fclosetest(table);
        fclosetest(in);
	fclosetest(cfile);
        return(ERROR);
        }



/*	allocate memory */
y_diff = (double **) malloc(np*sizeof(double *));
if (!y_diff)
	{
	Werrprintf("Can not allocate memory !");
	return(ERROR);
	}
for (i=0;i<np;i++)
	{
	y_diff[i] = (double *) malloc(n*sizeof(double));
	if (!y_diff[i])
		{
		Werrprintf("Can not allocate memory !");
		return(ERROR);
		}
	}
y_fitted = (double **) malloc(np*sizeof(double *));
if (!y_fitted)
	{
	Werrprintf("Can not allocate memory !");
	return(ERROR);
	}
for (i=0;i<np;i++)
	{
	y_fitted[i] = (double *) malloc(n*sizeof(double));
	if (!y_fitted[i])
		{
		Werrprintf("Can not allocate memory !");
		return(ERROR);
		}
	}


ma = NPARAMS;
if (mfit == 4)
	{
	for (i=0;i<NPARAMS;i++)	lista[i] = i;
	}
else if (mfit == 3)
	{
	lista[0] = 0; lista[1] = 2; lista[2] = 3;
	}
else
	{
	lista[0] = 2; lista[1] = 3;
	}

for (i=0;i<np;i++)      sig[i] = 1.0;   /*temp*/
for (i=0,j=0;i<(np+badinc);i++)         /*scan through all the input values, discarding the unwanted ones */
        {
        badflag = 0;
        for (l=0;l<badinc;l++)  if (i == bd[l]-1) badflag = TRUE;
        strcpy(jstr,userdir);
        strcat(jstr,"/Dosy/dosy_in"); /* read in gradient array */
        if (badflag)
                {
                if (fscanf(in,"%s",rubbish) == EOF)
                {
                        Werrprintf("dosyfit: reached end of file %s on increment %d", jstr, i);
                        if (ll2dflg) { free(peak); free(peak_table); }
                        else fclosetest(table);
                        fclosetest(in); fclosetest(cfile);
                        return(ERROR);
                }
                }
        else     
                {
                if (fscanf(in,"%lf",&x[j]) == EOF)
                {
                        Werrprintf("dosyfit: reached end of file %s on increment %d", jstr, i);
                        if (ll2dflg) { free(peak); free(peak_table); }
                        else fclosetest(table);
                        fclosetest(in); fclosetest(cfile);
                        return(ERROR);
                }
                x[j] *= DAC_to_Gauss;  /* change to gauss/cm */
                x[j++] *= 1e-2;  /* change from gauss/cm to tesla/m */
                }
        }

if (!calibflag)         /* calibrate gradient array */
        {
                for (i=0;i<np;i++)
                        {
                        x2[i] = x[i]*x[i]*dosyconstant;
                        }
        }
if (calibflag)
        for (i=0;i<np;i++)
        {
                if (fscanf(cfile,"%lf",&x2[i]) == EOF)
                {
                        strcpy(jstr,userdir);
                        strcat(jstr,"/Dosy/calibrated_gradients");
                        Werrprintf("dosyfit: reached end of file %s", jstr);
                        Wscrprintf("dosyfit: reached end of file %s", jstr);
                        if (ll2dflg) { free(peak); free(peak_table); }
                        else fclosetest(table);
                        fclosetest(in); fclosetest(cfile);
                        return(ERROR);
                }
        }
if(nonlinflag){
        for (i=0;i<MAXFITCOEFFS;i++)
             {
              if (fscanf(fitfile,"%lf",&fc[i]) == EOF)
              {  
                      strcpy(jstr,userdir);
                      strcat(jstr,"/Dosy/fit_coefficients");
                      Werrprintf("dosyfit: reached end of file %s", jstr);
                      Wscrprintf("dosyfit: reached end of file %s", jstr);
                      if (ll2dflg) { free(peak); free(peak_table); }
                      else fclosetest(table);
                      fclosetest(in); fclosetest(fitfile);
                      return(ERROR);
              }  
        }
        }



strcpy(jstr,userdir);
strcat(jstr,"/Dosy/dosy_in");   /* read in frequencies from dll, could read from file fp.out */
if (!ll2dflg)
        {
        if (fscanf(in,"%s",rubbish) == EOF)
        {
                Werrprintf("dosyfit: reached end of file %s", jstr);
                if (ll2dflg) { free(peak); free(peak_table); }
                else fclosetest(table);
                fclosetest(in); fclosetest(cfile);
                return(ERROR);
        }
        while (strcmp(rubbish,"(Hz)") != 0)
                if (fscanf(in,"%s",rubbish) == EOF)
                {
                        Werrprintf("dosyfit: reached end of file %s", jstr);
                        if (ll2dflg) { free(peak); free(peak_table); }
                        else fclosetest(table);
                        fclosetest(in); fclosetest(cfile);
                        return(ERROR);
                }
        for (i=0;i<n;i++)
                if ((fscanf(in,"%s",rubbish) == EOF) || (fscanf(in,"%lf",&frq[i]) == EOF))
                {
                        Werrprintf("dosyfit: reached end of file %s", jstr);
                        if (ll2dflg) { free(peak); free(peak_table); }
                        else fclosetest(table);
                        fclosetest(in); fclosetest(cfile);
                        return(ERROR);
                }
        }


if (!ll2dflg)	fprintf(fit_results,"\t\t2D data set\n\n");
else		fprintf(fit_results,"\t\t3D data set\n\n");
if(nonlinflag) {
                fprintf(fit_results,"\tExponential power series fit:\n\tpeak height =  a1 * exp(-(k1*a2*gradient_area\n\t\t\t+k2*(a2*gradient area)^2\n\t\t\t+k3*(a2*gradient area)^3\n\t\t\t+k4*(a2*gradient area)^4\n\t\t\t+k5*(a2*gradient area)^5))\n");
                fprintf(fit_results,"\n\tFit coefficients file : '%s'\n\n",nonlinname);
                }
else fprintf(fit_results,"\tExponential fit:\npeak height =  a1 * exp(-a2*gradient_area)\n");
ct = 0;
for (i=0;i<np;i++)
	{
	peak_height_sum[i] = 0.0;
	xdiff[i] = 0.0;
	for (j=0;j<n;j++)
		{
		y_diff[i][j] = 0.0;
		}
	}
disp_status("dosyfit");

for (lr=0;lr<n;lr++)	/* loop over resonances begins */
	{

     if (lr == 0 && !ll2dflg)        /* read in amplitudes from dll */
                {
                if (fscanf(in,"%s",rubbish) == EOF)
                {
                        Werrprintf("dosyfit: reached end of file %s", jstr);
                        if (ll2dflg) { free(peak); free(peak_table); }
                        else fclosetest(table);
                        fclosetest(in); fclosetest(cfile); fclosetest(fit_results); fclosetest(D_spectrum);
                        return(ERROR);
                }
                while(strcmp(rubbish,"(mm)") != 0)
                        {
                        if (fscanf(in,"%s",rubbish) == EOF)
                        {
                                Werrprintf("dosyfit: reached end of file %s", jstr);
                                if (ll2dflg) { free(peak); free(peak_table); }
                                else fclosetest(table);
                                fclosetest(in); fclosetest(cfile); fclosetest(fit_results); fclosetest(D_spectrum);
                                return(ERROR);
                        }
                        }
                }        

	if(lr%32 == 0)	disp_index(lr);
	for (i=0,j=0;i<(np+badinc);i++)
		{
		badflag = 0;
		for (l=0;l<badinc;l++)         if (i == bd[l]-1) badflag = 1;
		if (!ll2dflg)
			{
			fscanf(in,"%s",rubbish);
			fscanf(in,"%s",rubbish);
			if (badflag)	fscanf(in,"%s",rubbish);
			else		fscanf(in,"%lf",&y[j++]);
			}
		else
			{
			if (!badflag)
				{
				if (avnoise>0.0)
					{
					y[j++]=peak[i]->vol-avnoise*(peak[i]->f1_max-peak[i]->f1_min)*(peak[i]->f2_max-peak[i]->f2_min);
printf("peak %d   point %d\ncorners \t%f \t%f \t%f \t%f\nraw volume %f\tcorrected vol %f\n",i,j,peak[i]->f1_min,peak[i]->f1_max,peak[i]->f2_min,peak[i]->f2_max,peak[i]->vol,y[j-1]);
					}
				else
					{
					y[j++] = peak[i]->vol;
					}
				}
			}
		}
	/* set initial guesses for the fitting parameters */
	a[0] = 0.0;
	a[1] = 0.0;
	a[2] = y[0]-(y[1]-y[0])*x2[0]/(x2[1]-x2[0]);
	i = 0;
	while (y[++i] >= 0.5*a[2] && i < np-1)	;
	fac = x2[i-1]+(a[2]-y[i])/(y[i-1]-y[i])*(x2[i]-x2[i-1]);
	a[3] = 0.693/fac;
/* Manchester 6.1B   16xi00   */
if (a[2]==0.0) a[2]=1.0;
if (!(a[3]>0.0 && a[3]<1.0e-5)) a[3]=1.0e-10;

	alamda = -1.0;
	errorflag = 0;
	mrqmindosy(&alamda,lista,x,x2,y,a,sig,&chisq,&errorflag);
	if (abortflag)
        {
           if (ll2dflg) { free(peak); free(peak_table); }
           else fclosetest(table);
           fclosetest(in); fclosetest(cfile); fclosetest(fit_results); fclosetest(D_spectrum);
           return(ERROR);
        }
	if (errorflag) continue;
	k = 1;
	itst = 0;
	for (;;)
		{
		k++;
		ochisq = chisq;
		mrqmindosy(&alamda,lista,x,x2,y,a,sig,&chisq,&errorflag);
	        if (abortflag)
                {
                   if (ll2dflg) { free(peak); free(peak_table); }
                   else fclosetest(table);
                   fclosetest(in); fclosetest(cfile);
                   fclosetest(fit_results); fclosetest(D_spectrum);
                   return(ERROR);
                }
/* Manchester 6.1B   16xi00   */
                if (k>19) Wscrprintf("Warning:  corrupt input data");
                if (k>19) break;

		if (errorflag) break;
		if (chisq > ochisq)	itst = 0;
		else
			{
			if (fabs(ochisq-chisq) < 0.1)	itst++;
			else itst = 0;
			}
		if (itst < MAXITERS) continue;
		alamda = 0.0;
		mrqmindosy(&alamda,lista,x,x2,y,a,sig,&chisq,&errorflag);
	        if (abortflag)
                {
                   if (ll2dflg) { free(peak); free(peak_table); }
                   else fclosetest(table);
                   fclosetest(in); fclosetest(cfile);
                   fclosetest(fit_results); fclosetest(D_spectrum);
                   return(ERROR);
                }
		if (errorflag) break;
		if (!errorflag)
			{
			fac = chisq/(double)(np-mfit);
			fac = sqrt(fac);
			fac *= sqrt(covar[mfit-1][mfit-1]);
	/* if 2D dosy write out to diffusion_display.inp */
			if (!ll2dflg)
				{
				if (a[3] > 0.0 && ((100.0*fac/a[3]) < MAX_ERROR))
					{
					fprintf(table,"%10.2f%10.4f%10.4f\n",frq[lr],1.0e10*a[3],1.0e10*fac);
						/* The linewidths in 'diffusion_spectrum' file are no longer multiplied by 2 */
					if (a[2] > 0.0) fprintf(D_spectrum,"%10.2f,%10.4f,%10.4f,%10.2f\n",1.0e10*a[3],a[2]/(1.0e10*fac),1.0*1.0e10*fac,1.0);
					}
				else	fprintf(errorfile,"Fit failed on line %d \n",lr+1);
				}
	/* else update the ll2d labels and write out to diffusion_display_3D.inp */
			else
				{
				if (a[3] > 0.0 && ((100.0*fac/a[3]) < MAX_ERROR))
					{
					fprintf(table,"%5d%10.4f%10.4f\n",peak[0]->key,1.0e10*a[3],1.0e10*fac);
/* DON'T change following line to add 0.05 to standard error */
					fprintf(D_spectrum,"%10.2f,%10.4f,%10.4f,%10.2f\n",1.0e10*a[3],a[2]/(1.0e10*fac),1.0e10*fac+0.0,1.0);
		/* display the results of 3D analysis using the label facility of the 'll2d' program */
					i = lo = 0;
		/* First loop to copy diff. coef., second loop to copy estimated error */
						while (lo < 2)
						{
						jo = ko = 0;
					/* look for the correct rounded value of xt */
						xtf = (lo == 0) ? (1.0e12*a[3]) : (1.0e12*fac);
						xtlow = xtf;
						xthigh = xtlow+1;
						dhigh = (float)xthigh - xtf;
						dlow = xtf - (float)xtlow;
						if (dhigh <= dlow)	xt = xo = xthigh;
						else			xt = xo = xtlow;
						xtf = (float)xt;
						io = 1;
						while (xt >= 1.0)
							{
							xt = xo/pow(10.0,io++);
							}
						io -= 2;
						ko = TRUE;
						xt = xo;
						/* now copy the number into the string 'str1' */
						while (io >= 0)
							{
							if (io <= 1 && ko == TRUE)
								{
								str1[i++] = '0';
								str1[i++] = '.';
								if (io == 0)	str1[i++] = '0';
								}
							else if (io == 1) str1[i++] = '.';
							xt = xt-(jo*pow(10.0,io+1));
							jo = xt/pow(10.0,io--);
							str = jo+'0';
							if (ko == TRUE)
								{
								str1[i++] = str;
								ko = FALSE;
								}
							else	str1[i++] = str;
							}
						if (lo == 0)
							{
							str1[i++] = ' ';
							str1[i++] = '(';
							}
						if (lo == 1)
							{
							if (xtf < 1.0)
								{
								str1[i++] = '0';
								}
							str1[i++] = ')';
							}
						lo++;
						}
					while (i < LABEL_LEN-1)		str1[i++] = '\0';
					}
			/* if not a good diffusion coefficient signal it by label 'x' */
				else
					{
					i = 0;
					while (i < LABEL_LEN-1)         str1[i++] = '\0';
/*
					str1[0] = 'x';
*/
					}
				strcpy(peak[0]->label,str1);
				if (lr == 0)
					{
					strcpy(fname,curexpdir);
                			strcat(fname,"/ll2d/peaks.bin");
					old_ptr = peak_table[0]->file;
					if ((peak_table[0]->file = fopen(fname,"r+")) == NULL)
						{
						Werrprintf("dosyfit: unable to open %s\n",fname);
						return(ERROR);
						}
					}
				write_peak_file_record(peak_table[0],peak[0],peak[0]->key);
				}
			if (!ll2dflg)	fprintf(fit_results,"Frequency %f\n",frq[lr]);
			else		fprintf(fit_results,"\n\tPeak number %d :\n",peak[0]->key);
			fprintf(fit_results,"a0 %4.3f       a1 %4.3f       a2 %4.2f (%5.2f)\n",a[0],a[2],1.0e10*a[3],1.0e10*fac);
			fprintf(fit_results,"Gradient area         exp. height    calc. height          Diff\n");
                        for (i=0;i<np;i++)
                                {
                                if(nonlinflag) yc[i] = a[0]+a[2]*exp(-(fc[0]*a[3]*x2[i]+
                                                                fc[1]*pow(x2[i]*a[3],2.0)+
                                                                fc[2]*pow(x2[i]*a[3],3.0)+
                                                                fc[3]*pow(x2[i]*a[3],4.0)+
                                                                fc[4]*pow(x2[i]*a[3],5.0)));
                                else yc[i] = a[0]+a[2]*exp(-a[3]*x2[i]);

                                y_diff[i][lr] = y[i]-yc[i];
                                y_fitted[i][lr] = yc[i];
                                fprintf(fit_results,"%15.1f %15.2f %15.2f %15.2f\n",x2[i],y[i],yc[i],y[i]-yc[i]);
                                }
        /* push the pointers to the next peak */
                        if (ll2dflg && lr < (n-1))
                                for (i=0;i<(np+badinc);i++)     peak[i] = peak[i]->next;
                        if (fac/a[3] < 0.075)
                                {
                                rej_pk[lr]=0;
                                fac = -1.0/a[3];
                                for (i=0;i<np;i++)
                                        {
/*      calculate the estimated value of x from the fit coefficients and the experimental y value */
/*      peak height sum represents total signal for a given gzlvl1 value */
/*      only do calculation if experimental signal greater than 1% of initial */

                                        if (y[i]>0.01*a[2])
                                                {
                                                peak_height_sum[i] += y[i];
                                                xc[i] = fac*log((y[i]-a[0])/a[2]);
                                                xdiff[i] += xc[i]*y[i];
                                                }
                                        }
                                ct++;
                                } else rej_pk[lr]=1;

                        }
                break;
		}
	if (errorflag)
		{
		fprintf(errorfile,"Fit failed on line %d \n",lr+1);
		}
	}	/* end of loop for the various resonances */

/* Finally compile the statistics necessary for gradient calibration */
/* if there is more than one peak				     */

if (ct > 1)
	{
for (i=0;i<np;i++)
        {
        y_diff_wa[i] = 0.0;
        y_sum[i] = 0.0; 
        y_dd[i] = 0.0;
        for (j=0;j<n;j++)
                {
                if (rej_pk[j]==0)
                        {
                        y_diff_wa[i] += y_diff[i][j];
                        y_sum[i] += y_fitted[i][j];
                        }
                }
        y_diff_wa[i] /= y_sum[i];
        }
/* y_diff_wa[i] contains the weighted average difference for gzlvl1[i] value, */
/* still need to calculate standard deviation */
for (i=0;i<np;i++)
        {
        y_dd[i] = 0.0;
        for (j=0;j<n;j++)
                {
                if (rej_pk[j]==0)
                        {
                        y_dd[i] += SQR((y_diff[i][j]/y_fitted[i][j])-y_diff_wa[i])*(y_fitted[i][j]/y_sum[i]);

                        }
                }
        y_dd[i] /= (double)(n-1);
        y_std[i] = sqrt(y_dd[i]);
        }
	fprintf(fit_results,"\nThe average percentage differences between experimental and\n");
	fprintf(fit_results,"calculated data points are, for the %d values of gzlvl1:\n\n",np);
	fprintf(fit_results,"Percentage           Standard deviation (per cent)\n");
	for (i=0;i<np;i++)
		{
		fprintf(fit_results," %2.5f			     %2.5f\n",y_diff_wa[i]*100.0,y_std[i]*100.0);			
		if (!calibflag && (fabs(y_diff_wa[i]/y_std[i]) > 3.0))	systematic_dev_flg = TRUE;
		}
	if (!ll2dflg && systematic_dev_flg == TRUE)	Winfoprintf("Systematic deviations found:  may be worth trying \" undosy calibflag='y' dosy \"\n");
	if (!calibflag)
		{
		fclosetest(cfile);
		strcpy(rubbish,userdir);
		strcat(rubbish,"/Dosy/calibrated_gradients");
                cfile = fopen(rubbish,"w");
if (!cfile)
{
        Werrprintf("dosyfit: could not open file %s",rubbish);
        if (ll2dflg) { free(peak); free(peak_table); }
        else fclosetest(table);
        fclosetest(in);
        return(ERROR);
}

		for (i=0;i<np;i++)
		{
		if (xdiff[i]==0.0) xdiff[i]=x2[i]*peak_height_sum[i];
		fprintf(cfile,"%.3f\n",xdiff[i]/peak_height_sum[i]);
		}
                fflush(cfile);
		}

for (i=(np-1);i>=0;i--)
	{
	free((char *) (y_diff[i]));
	free((char *) (y_fitted[i]));
	}
free((char *) (y_diff));
free((char *) (y_fitted));
	}

fclosetest(in);
fclosetest(D_spectrum);
fclosetest(table);
if (ll2dflg)
	{
	fclosetest(old_ptr);
	for (i=0;i<(np+badinc);i++)
		{
		fflush(peak_table[i]->file);
		delete_peak_table(&peak_table[i]);
		}
	}
fclosetest(fit_results);
fclosetest(cfile);
fclosetest(errorfile);
disp_index(0);
disp_status("       ");
RETURN;
}


void mrqmindosy(double *flag,int *lista,double *x,double *x2,double *y,double *a,double *sig,double *chisq,int *error)
{
int	k,kk,i,j,l,m,ihit,indxc[NPARAMS],indxr[NPARAMS],ipiv[NPARAMS];
double	pivinv,dum,da[NPARAMS],dy,dyda[NPARAMS],atry[NPARAMS],sig2i,wt,ymod;

(void) x;
if (*flag < 0.0)
	{
	kk = mfit+1;
	for (j=0;j<ma;j++)
		{
		ihit = 0;
		for (k=0;k<mfit;k++)
			{
			if (lista[k] == j)	ihit++;	
			}
		if (ihit == 0)
			{
			lista[kk++] = j;
			}
		else if (ihit > 1) 
			{
			 abortflag = 1;
	                 Werrprintf("Dosyfit program error ...\n");
                         return;
			}
		}
	if (kk != ma+1)
        {
	    abortflag = 1;
	    Werrprintf("Dosyfit program error ...\n");
            return;
        }
	*flag = 0.001;
	/* beginning of first mrqcof() routine */
	for (j=0;j<mfit;j++)
		{
		for (k=0;k<=j;k++)
			{
			alpha[j][k] = 0.0;
			}
		beta[j] = 0.0;
		}
	*chisq = 0.0;
	for (i=0;i<np;i++)
		{
	        /* do function bit */
                if(nonlinflag)
                {
                                yt1 = a[0];
                                if (x2[i] >= 0.0)       yt2 = a[1]*sqrt(x2[i]);
                                else                    yt2 = -1.0*a[1]*sqrt(-1.0*x2[i]);
                                yt3 = -1.0*(fc[0]*a[3]*x2[i]
                                        +fc[1]*pow(x2[i]*a[3],2.0)
                                        +fc[2]*pow(x2[i]*a[3],3.0)
                                        +fc[3]*pow(x2[i]*a[3],4.0)
                                        +fc[4]*pow(x2[i]*a[3],5.0));
                                yt3 = exp(yt3);
                                ymod = yt1+yt2+a[2]*yt3;
         
                                /* partial derivatives wrt parameters   */
                                dyda[0]=1.0;
                                if (x2[i] >= 0.0)       dyda[1] = sqrt(x2[i]);
                                else                    dyda[1] = -1.0*sqrt(-1.0*x2[i]);
                                dyda[2] = yt3;
                                dyda[3] = -a[2]*(fc[0]*x2[i]
                                                +2.0*fc[1]*a[3]*pow(x2[i],2.0)
                                                +3.0*fc[2]*pow(a[3],2.0)*pow(x2[i],3.0)
                                                +4.0*fc[3]*pow(a[3],3.0)*pow(x2[i],4.0)
                                                +5.0*fc[4]*pow(a[3],4.0)*pow(x2[i],5.0)) * yt3;

                        }
                else{

                        yt1 = a[0];
         
                        if (x2[i] >= 0.0)       yt2 = a[1]*sqrt(x2[i]);
                        else                    yt2 = -1.0*a[1]*sqrt(-1.0*x2[i]);
                        yt3 = -1.0*x2[i]*a[3];
                        yt3 = exp(yt3);
                        ymod = yt1+yt2+a[2]*yt3;
                        dyda[0] = 1.0;
         
                        if (x2[i] >= 0.0)       dyda[1] = sqrt(x2[i]);
                        else                    dyda[1] = -1.0*sqrt(-1.0*x2[i]);
                        dyda[2] = yt3;
                        dyda[3] = -1.0*a[2]*x2[i]*yt3;
 
                        }


       		/* end of function bit */
		sig2i = 1.0/(sig[i]*sig[i]);
		dy = y[i]-ymod;
		for (j=0;j<mfit;j++)
			{
			wt = dyda[lista[j]]*sig2i;
			for (k=0;k<=j;k++)
				{
				alpha[j][k] += wt*dyda[lista[k]];
				}
			beta[j] += dy*wt;
			}
		(*chisq) += dy*dy*sig2i;
		}
	for (j=1;j<mfit;j++)
		{
		for (k=0;k<=j-1;k++)
			{
			alpha[k][j] = alpha[j][k];
			}
		}
	/*end of first mrqcof() routine */
	prevchisq = *chisq;
	}
for (j=0;j<mfit;j++)
	{
	for (k=0;k<mfit;k++)
		{
		covar[j][k] = alpha[j][k];
		}
	covar[j][j] = alpha[j][j]*(1.0+(*flag));
	oneda[j][0] = beta[j];
	}
for (i=0;i<mfit;i++) ipiv[i] = 0;	/* beginning of gaussj routine */
for (i=0;i<mfit;i++)
	{
	big = 0.0;
	for (j=0;j<mfit;j++)
		{
		if (ipiv[j] != 1)
			{
			for (k=0;k<mfit;k++)
				{
				if (ipiv[k] == 0)
					{
					if (fabs(covar[j][k]) >= big)
						{
						big = fabs(covar[j][k]);
						irow = j;
						icol = k;
						}
					}
				else
					{
					if (ipiv[k] > 1)
						{
						*error = 1;
						return;
						}
					}
				}
			}
		}
	++(ipiv[icol]);
	if (irow != icol)
		{
		for (l=0;l<mfit;l++)	SWAP(covar[irow][l],covar[icol][l]);
		for (l=0;l<1;l++)	SWAP(oneda[irow][l],oneda[icol][l]);
		}
	indxr[i] = irow;
	indxc[i] = icol;
	if (covar[icol][icol] == 0.0)
		{
		*error = 1;
		return;
		}
	pivinv = 1.0/covar[icol][icol];
	covar[icol][icol] = 1.0;
	for (l=0;l<mfit;l++)	covar[icol][l] *= pivinv;
	for (l=0;l<1;l++)	oneda[icol][l] *= pivinv;
	for (m=0;m<mfit;m++)
		{
		if (m != icol)
			{
			dum = covar[m][icol];
			covar[m][icol] = 0.0;
			for (l=0;l<mfit;l++)	covar[m][l] -= covar[icol][l]*dum;
			for (l=0;l<1;l++)	oneda[m][l] -= oneda[icol][l]*dum;
			}
		}
	}
for (l=mfit-1;l>=0;l--)
	{
	if (indxr[l] != indxc[l])
		{
		for (k=0;k<mfit;k++)
			{
			SWAP(covar[k][indxr[l]],covar[k][indxc[l]]);
			}
		}
	}	/* end of gaussj routine */
for (j=0;j<mfit;j++)
	{
	da[j] = oneda[j][0];
	}
if (*flag == 0.0)
	{
	return;
	}
for (j=0;j<ma;j++) atry[j] = a[j];
for (j=0;j<mfit;j++) atry[lista[j]] = a[lista[j]]+da[j];

/* Beginning of mrqcof routine */
for (j=0;j<mfit;j++)
	{
	for (k=0;k<=j;k++)
		{
		covar[j][k] = 0.0;
		}
	da[j] = 0.0;
	}
*chisq = 0.0;
for (i=0;i<np;i++)
	{
	/* do function bit */
        /* do function bit */
                if(nonlinflag)
                {
                                ymod = 0.0;
                                yt1 = atry[0];
                                if (x2[i] >= 0.0)       yt2 = atry[1]*sqrt(x2[i]);
                                else                    yt2 = -1.0*atry[1]*sqrt(-1.0*x2[i]);
                                yt3 = -1.0*(fc[0]*atry[3]*x2[i]
                                        +fc[1]*pow(x2[i]*atry[3],2.0)
                                        +fc[2]*pow(x2[i]*atry[3],3.0)
                                        +fc[3]*pow(x2[i]*atry[3],4.0)
                                        +fc[4]*pow(x2[i]*atry[3],5.0));
                                yt3 = exp(yt3);
                                ymod = yt1+yt2+atry[2]*yt3;
         
                                /* partial derivatives wrt parameters   */
                                dyda[0]=1.0;
                                if (x2[i] >= 0.0)       dyda[1] = sqrt(x2[i]);
                                else                    dyda[1] = -1.0*sqrt(-1.0*x2[i]);
                                dyda[2] = yt3;
                                dyda[3] = -atry[2]*(fc[0]*x2[i]
                                                +2.0*fc[1]*atry[3]*pow(x2[i],2.0)
                                                +3.0*fc[2]*pow(atry[3],2.0)*pow(x2[i],3.0)
                                                +4.0*fc[3]*pow(atry[3],3.0)*pow(x2[i],4.0)
                                                +5.0*fc[4]*pow(atry[3],4.0)*pow(x2[i],5.0)) *yt3;

                        }
                else
                {
                        ymod = 0.0;
                        yt1 = atry[0];
                
                        if (x2[i] >= 0.0)       yt2 = atry[1]*sqrt(x2[i]);
                        else                    yt2 = -1.0*atry[1]*sqrt(-1.0*x2[i]);
                        yt3 = -1.0*x2[i]*atry[3];
                        yt3 = exp(yt3);
                        ymod = yt1+yt2+atry[2]*yt3;
                        dyda[0] = 1.0;
                
                        if (x2[i] >= 0.0)       dyda[1] = sqrt(x2[i]);
                        else                    dyda[1] = -1.0*sqrt(-1.0*x2[i]);
                        dyda[2] = yt3;
                        dyda[3] = -1.0*atry[2]*x2[i]*yt3;
                }
                /* end of function bit */

	sig2i = 1.0/(sig[i]*sig[i]);
	dy = y[i]-ymod;
	for (j=0;j<mfit;j++)
		{
		wt = dyda[lista[j]]*sig2i;
		for (k=0;k<=j;k++)	covar[j][k] += wt*dyda[lista[k]];
		da[j] += dy*wt;
		}
	*chisq += dy*dy*sig2i;
	}
for (j=1;j<mfit;j++)
	{
	for (k=0;k<=j-1;k++)	covar[k][j] = covar[j][k];
	}
/* End of mrqcof routine */

if (*chisq < prevchisq)
	{
	*flag *= 0.1;
	prevchisq = *chisq;
	for (j=0;j<mfit;j++)
		{
		for (k=0;k<mfit;k++)
			{
			alpha[j][k] = covar[j][k];
			}
		beta[j] = da[j];
		a[lista[j]] = atry[lista[j]];
		}
	}
else
	{
	*flag *= 10.0;
	*chisq = prevchisq;
	}
return;
}
