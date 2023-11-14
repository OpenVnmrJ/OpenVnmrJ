/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* gxyzcalib.c   GAM   27 v 03 */
/* input left and right profile edge frequencies */
/* fit difference (profile width) and average (profile centre) */
/* adapted from calibxy2 27 v 03 */

  #include <stdlib.h>
  #include <string.h>
  #include <math.h>

  #define NR_END 1
  #define FREE_ARG char*
  #define MAXLENGTH     512
  #define ERROR 1
  #define MA 5

/*  #define DEBUG  */ 
#ifdef DEBUG
  #include <stdio.h>
#endif

#include "tools.h"
#include "wjunk.h"
#include "vnmr_gxyzcalib_lm.h"

  extern char  curexpdir[];
  extern int abortflag;

  static void free_vector(double *v, long nl, long nh);
  static double *vector(long nl, long nh);
  static int *ivector(long nl, long nh);
  static void free_ivector(int *v, long nl, long nh);
  static double **matrix(long nrl, long nrh, long ncl, long nch);
  static void free_matrix(double **m, long nrl, long nrh, long ncl, long nch);
  static double a[MA+1]= {0,0,0,0,0,0};
  static double gues[MA+1]= {0,0.1,1.0,0.1,0.01,0.01};
  static double root=0;
  static double a1,a2,a3,a4,a5;

  static void widthmodel(double x1, double y1, 
                         double a[], double *w, double dyda[], int na)
  {
   root=sqrt((-a[5]+y1)*(-a[5]+y1)*a[2]*a[2]*cos(3.14159265358979323846*a[3]/180)*
        cos(3.14159265358979323846*a[3]/180)+
        ((-a[4]+x1)*a[1]+(-a[5]+y1)*a[2]*sin(3.14159265358979323846*a[3]/180))*
        ((-a[4]+x1)*a[1]+(-a[5]+y1)*a[2]*sin(3.14159265358979323846*a[3]/180)));
    
   *w=sqrt((a[1]*(x1-a[4])+a[2]*(y1-a[5])*sin(a[3]*3.14159265358979323846/180))*
            (a[1]*(x1-a[4])+a[2]*(y1-a[5])*sin(a[3]*3.14159265358979323846/180))+
             a[2]*a[2]*(y1-a[5])*(y1-a[5])*cos(a[3]*3.14159265358979323846/180)*
             cos(a[3]*3.14159265358979323846/180)); 

    dyda[1]=(-a[4]+x1)*((-a[4]+x1)*a[1]+
            (-a[5]+y1)*a[2]*sin(3.14159265358979323846*a[3]/180))/root;


    dyda[2]=(2*(-a[5]+y1)*(-a[5]+y1)*a[2]*cos(3.14159265358979323846*a[3]/180)*
            cos(3.14159265358979323846*a[3]/180)+
            2*(-a[5]+y1)*sin(3.14159265358979323846*a[3]/180)*((-a[4]+x1)*a[1]+
            (-a[5]+y1)*a[2]*sin(3.14159265358979323846*a[3]/180)))/(2*root);

    dyda[3]=((-3.14159265358979323846/90)*(-a[5]+y1)*(-a[5]+y1)*a[2]*a[2]*
            cos(3.14159265358979323846*a[3]/180)*sin(3.14159265358979323846*a[3]/180)+
            (3.14159265358979323846/90)*(-a[5]+y1)*a[2]*cos(3.14159265358979323846*a[3]/180)*
            ((-a[4]+x1)*a[1]+(-a[5]+y1)*a[2]*
            sin(3.14159265358979323846*a[3]/180)))/(2*root);

    dyda[4]=-(a[1]*((-a[4]+x1)*a[1]+(-a[5]+y1)*
              a[2]*sin(3.14159265358979323846*a[3]/180)))/root;

    dyda[5]=-((-a[5]+y1)*a[2]*a[2]*cos(3.14159265358979323846*a[3]/180)*
             cos(3.14159265358979323846*a[3]/180)+
             a[2]*sin(3.14159265358979323846*a[3]/180)*((-a[4]+x1)*a[1]+
             (-a[5]+y1)*a[2]*sin(3.14159265358979323846*a[3]/180)))/root;

  }

  static void midpointmodel(double x1, double y1, 
                         double a[], double *f, double dyda[], int na)
  {
    
   *f=a[1]+a[2]*(a1*(x1-a4)+a2*(y1-a5)*sin(a3*3.14159265358979323846/180))+a[3]*a2*(y1-a5)*cos(a3*3.14159265358979323846/180);

    dyda[1]=1.0;
    dyda[2]=(a1*(x1-a4)+a2*(y1-a5)*sin(a3*3.14159265358979323846/180));
    dyda[3]=a2*(y1-a5)*cos(a3*3.14159265358979323846/180);
  }

   int calibxy(int argc, char *argv[],int retc, char *retv[])
  {
   FILE *in,*out,*outreg;	
   char in_file[MAXLENGTH];
   char out_file[MAXLENGTH];
   int i,*ia,itst,k,mfit=MA,npt,xmax,xmin;
   double gammaD2O,int_diam,chisq,ochisq,*x,*x1,*y1,
          *w,*sig,**covar,**alpha,*dummydyda,cwidth,cmidpoint,*m;
   char fname[MAXLENGTH];

   gammaD2O=653.5;
   int_diam=0.427;

strcpy(in_file,curexpdir);
strcat(in_file,"/xydata");
strcpy(out_file,curexpdir);
strcat(out_file,"/output");

if ((in=fopen(in_file,"r"))==NULL)
        {Werrprintf("Error opening xydata file.\n");
         return(ERROR);
        }
if ((out = fopen(out_file,"w")) == NULL) {
        Werrprintf("Error opening output file.\n");
	fclose(in);
        return(ERROR);
        }
   strcpy(fname,curexpdir);
   strcat(fname,"/regression.inp");
if ((outreg = fopen(fname,"w")) == NULL) {
        Werrprintf("Error opening regression.inp file.\n");
	fclose(in); fclose(out);
        return(ERROR);
        }
 
        fprintf(out,"Output from calibxy\n");

        fscanf(in,"%d  \n", &npt);
   if ((npt>257)||(npt<2))
   {
        Werrprintf("Number of points %d  is unreasonable\n",npt);
	fclose(in); fclose(out); fclose(outreg);
        return(ERROR);
   }

#ifdef DEBUG
printf("Number of data points:  %d\n",npt);
#endif
   fprintf(outreg,"Fitting of transverse signal profiles\n");
   fprintf(outreg,"profile widths in Hz\n");
   fprintf(outreg,"4\t%d\n",npt);
        
        abortflag = 0;
        ia=ivector(1,MA);
	dummydyda=vector(1,MA);
	x=vector(1,npt);
	w=vector(1,npt);  /* width of profile */
	sig=vector(1,npt);
	covar=matrix(1,MA,1,MA);
	alpha=matrix(1,MA,1,MA);
        x1=vector(1,npt); 
        y1=vector(1,npt); 
	m=vector(1,npt);  /* midpoint of profile */
        if (abortflag)
        {
		free_ivector(ia,1,MA);
		free_vector(dummydyda,1,npt);
		free_vector(x,1,npt);
		free_vector(w,1,npt);
		free_vector(sig,1,npt);
		free_matrix(covar,1,MA,1,MA);
        	free_matrix(alpha,1,MA,1,MA);
		free_vector(x1,1,npt);
		free_vector(y1,1,npt);
		free_vector(m,1,npt);
       	 	fclose(in);
      		fclose(out);
     	   	fclose(outreg);
        	return(ERROR);
        }

/* read in profile widths */

	for (i=1;i<=npt;i++)
	{
        if ((fscanf(in,"%lf %lf %lf \n", &x1[i],&y1[i],&w[i]))==EOF)
		{
		Werrprintf("Insufficient data in input file\n");
		free_ivector(ia,1,MA);
		free_vector(dummydyda,1,npt);
		free_vector(x,1,npt);
		free_vector(w,1,npt);
		free_vector(sig,1,npt);
		free_matrix(covar,1,MA,1,MA);
        	free_matrix(alpha,1,MA,1,MA);
		free_vector(x1,1,npt);
		free_vector(y1,1,npt);
		free_vector(m,1,npt);
       	 	fclose(in);
      		fclose(out);
     	   	fclose(outreg);
        	return(ERROR);
		}
	}

/* read in profile midpoints */

	for (i=1;i<=npt;i++)
	{
        if ((fscanf(in,"%lf %lf %lf \n", &x1[i],&y1[i],&m[i]))==EOF)
		{
		Werrprintf("Insufficient data in input file\n");
		free_ivector(ia,1,MA);
		free_vector(dummydyda,1,npt);
		free_vector(x,1,npt);
		free_vector(w,1,npt);
		free_vector(sig,1,npt);
		free_matrix(covar,1,MA,1,MA);
        	free_matrix(alpha,1,MA,1,MA);
		free_vector(x1,1,npt);
		free_vector(y1,1,npt);
		free_vector(m,1,npt);
       	 	fclose(in);
      		fclose(out);
     	   	fclose(outreg);
        	return(ERROR);
		}
	}

#ifdef DEBUG
printf("Number of data points found:  %d\n",i-1);
#endif
        
        xmax=x1[1];
        xmin=x1[1]; 
        for(i=1;i<npt;i++)
        {
          if(xmax < x1[i+1])
            xmax=x1[i+1];
          else if(xmin > x1[i+1])
            xmin=x1[i+1];  
        }
        gues[1]=2*w[1]/(xmax-xmin);
        gues[2]=2*w[1]/(xmax-xmin);
	
        for (i=1;i<=MA;i++) a[i]=gues[i];

        for (i=1;i<npt+1;i++){ 
                  sig[i]=1.0;
          	}

#ifdef DEBUG
	printf("X1\tY1\twidth\tmidpoint\n\n");
        for(i=1;i<=npt;i++)
        {
	printf("%f\t%f\t%f\t%f\n",x1[i],y1[i],w[i],m[i]);
        }
#endif

/* start fitting widths */

	for (i=1;i<=mfit;i++) ia[i]=1;
	for (i=1;i<=MA;i++) a[i]=gues[i];
        lm_gxyz_init(x1,y1,w,sig,npt,a,ia,MA,covar,alpha,&chisq,widthmodel);
		k=1;
		itst=0;
		for (;;){
			k++;
#ifdef DEBUG
printf("chi squared:  %f\n",chisq);
#endif
			ochisq=chisq;
        lm_gxyz_iterate(x1,y1,w,sig,npt,a,ia,MA,covar,alpha,&chisq,widthmodel);

			if (chisq > ochisq)
			itst=0;
			else if (fabs(ochisq-chisq) < 0.1)
			itst++;
			if (itst < 4) continue;
	lm_gxyz_covar(x1,y1,w,sig,npt,a,ia,MA,covar,alpha,&chisq,widthmodel);
			break;
	}

 
   /* Calculate and output linewidths */
   fprintf(out,"Calculated fitting parameters: \n");
   fprintf(out,"%f\t%f\t%f\t%f\t%f\n",a[1], a[2], a[3], a[4], a[5]);
   fprintf(out,"*****************************************************\n");
   fprintf(out,"x1 shim value: \t y1 shim value: \t Profile width, Hz:\n");

   for(i=1;i<=npt; i++){
   widthmodel(x1[i],y1[i], a, &cwidth, dummydyda, MA);
   fprintf(out,"%f\t%f\t%f\n ",x1[i],y1[i],cwidth); 
   }
   
   printf("Fitting of transverse profile width data \n");
   printf("======================================== \n");
   printf("X1 gives %f Hz per DAC point\n", a[1]);
   printf("Y1 gives %f Hz per DAC point\n", a[2]);
   printf("Deviation from orthogonality of X1 and Y1 is %6.2f degrees\n", a[3]);
   printf("Estimated error in X1 shim is %6.0f DAC points\n", a[4]);
   printf("Estimated error in Y1 shim is %6.0f DAC points\n", a[5]);
   printf("Final chi-squared is %7.3f Hz squared\n", chisq);
   fprintf(out,"\n\n\n\n chi-squared=%f\n", chisq);

   for(i=1;i<=npt; i++){
   widthmodel(x1[i],y1[i], a, &cwidth, dummydyda, MA);
   fprintf(outreg,"%i\t%f\n ",i,cwidth);
   }
   for(i=1;i<=npt; i++){
   fprintf(outreg,"%i\t%f\n ",i,w[i]);
   }

/* now fix fitted profile width parameters ready to fit midpoints */

a1=a[1]; a2=a[2]; a3=a[3]; a4=a[4]; a5=a[5];

/* and start fitting midpoints */

	for (i=1;i<=3;i++) ia[4]=0; ia[5]=0;
	gues[1]=0.0; gues[2]=0.0; gues[3]=0.0;
	for (i=1;i<=3;i++) a[i]=gues[i];
        lm_gxyz_init(x1,y1,m,sig,npt,a,ia,3,covar,alpha,&chisq,midpointmodel);
		k=1;
		itst=0;
		for (;;){
			k++;
#ifdef DEBUG
printf("chi squared:  %f\n",chisq);
#endif
			ochisq=chisq;
        lm_gxyz_iterate(x1,y1,m,sig,npt,a,ia,3,covar,alpha,&chisq,midpointmodel);

			if (chisq > ochisq)
			itst=0;
			else if (fabs(ochisq-chisq) < 0.1)
			itst++;
			if (itst < 4) continue;
	lm_gxyz_covar(x1,y1,m,sig,npt,a,ia,3,covar,alpha,&chisq,midpointmodel);
			break;
	}

 
   /* Calculate and output linewidths */
   fprintf(out,"Calculated fitting parameters: \n");
   fprintf(out,"%f\t%f\t%f\n",a[1], a[2], a[3]);
   fprintf(out,"*****************************************************\n");
   fprintf(out,"x1 shim value: \t y1 shim value: \t Profile midpoint, Hz:\n");

   for(i=1;i<=npt; i++){
   midpointmodel(x1[i],y1[i], a, &cmidpoint, dummydyda, 3);
   fprintf(out,"%f\t%f\t%f\n ",x1[i],y1[i],cmidpoint); 
   }
   
   printf("\nFitting of transverse profile midpoint data\n");
   printf("=============================================\n");
   printf("Mean centre frequency is %f Hz\n", a[1]);
   printf("Estimated X offset of sample from shim centre is %f sample diameters\n", a[2]);
   printf("Estimated Y offset of sample from shim centre is %f sample diameters\n", a[3]);
   printf("Final chi-squared is %7.3f Hz squared\n", chisq);
   fprintf(out,"\n\n\n\n chi-squared=%f\n", chisq);

   for(i=1;i<=npt; i++){
   midpointmodel(x1[i],y1[i], a, &cmidpoint, dummydyda, 3);
   fprintf(outreg,"%i\t%f\n ",i,cmidpoint);
   }
   for(i=1;i<=npt; i++){
   fprintf(outreg,"%i\t%f\n ",i,m[i]);
   }
   if (retc>0)
   {
    retv[0]=realString((double) a1);
   }
    if (retc>1)
   {
    retv[1]=realString((double) a2);
   }
    if (retc>2)
   {
    retv[2]=realString((double) a3);   
   }
    if (retc>3)
   {
    retv[3]=realString((double) a4);   
   }
    if (retc>4)
   {
    retv[4]=realString((double) a5);   
   }
    if (retc>5)
   {
    retv[5]=realString((double) a[1]);   
   }
    if (retc>6)
   {
    retv[6]=realString((double) a[2]);   
   }
    if (retc>7)
   {
    retv[7]=realString((double) a[3]);   
   }
        free_matrix(alpha,1,MA,1,MA);
	free_matrix(covar,1,MA,1,MA);
	free_vector(sig,1,npt);
	free_vector(w,1,npt);
	free_vector(dummydyda,1,npt);
	free_vector(x1,1,npt);
	free_vector(y1,1,npt);
	free_vector(x,1,npt);
	free_ivector(ia,1,MA);
	free_vector(m,1,npt);
        fclose(in);
        fclose(out);
        fclose(outreg);
	return 0;
   }

static double *vector(long nl, long nh)
  { double *v;
   v=(double *)malloc((size_t)((nh-nl+1+NR_END)*sizeof(double)));
   if (!v) {
        Werrprintf("allocation failure in vector() of indices  %ld  and  %ld\n",nl,nh);
        abortflag=1;
        return(v);
    }
      return v-nl+NR_END;
   }

static int *ivector(long nl, long nh)
    {
        int *v;
        v=(int *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(int)));
        if (!v)
        {
            Werrprintf("allocation failure in ivector()\n");
            abortflag=1;
            return(v);
        }
        return v-nl+NR_END;
    }

static void free_vector(double *v, long nl, long nh)
{
   if (v)
      free((FREE_ARG) (v+nl-NR_END));
}

static void free_ivector(int *v, long nl, long nh)
{
   if (v)
        free((FREE_ARG) (v+nl-NR_END));
}

static double **matrix(long nrl, long nrh, long ncl, long nch)
{
        long i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
        double **m;
        m=(double **) malloc((size_t)((nrow+NR_END)*sizeof(double*)));
        if (!m)
        {
            Werrprintf("allocation failure 1 in matrix()");
            abortflag=1;
            return(m);
        }
        m += NR_END;
        m -= nrl;
        m[nrl]=(double *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double)));
        if (!m[nrl])
        {
            Werrprintf("allocation failure 2 in matrix()");
            abortflag=1;
            return(m);
        }
        m[nrl] += NR_END;
        m[nrl] -= ncl;
        for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;
        return m;
}

static void free_matrix(double **m, long nrl, long nrh, long ncl, long nch)
{
   if (m && m[nrl])
        free((FREE_ARG) (m[nrl]+ncl-NR_END));
   if (m)
        free((FREE_ARG) (m+nrl-NR_END));
} 
