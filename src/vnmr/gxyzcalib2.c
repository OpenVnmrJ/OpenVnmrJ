/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* source code from /net/gerbil/export/home/vnmr1/Vnmr61B_VVK/calibxy.c */
/* revert using curexp not exp1 4ix02 GAM */
/* uses x1 and y1 input */
/* define initial guesses nonzero for x1err and y1err, otherwise crashes if x1[1]=y1[1]=0 */

  #include <stdio.h>
  #include <string.h>
  #include <math.h>
  #define NR_END 1
  #define FREE_ARG char*
  #include <stddef.h>
  #include <stdlib.h>
  #define SWAP(a,b) {swap=(a);(a)=(b);(b)=swap;}
  #define MAXLENGTH     512
  #define ERROR 1
/*  #define DEBUG  */
#include "tools.h"
#include "wjunk.h"

  extern char  curexpdir[];

  static void nrerror(char error_text[]);
  static void mrqmin2(double x1[],double y1[], double w[], double sig[], 
    int ndata, double a[], int ia[], int ma, double **covar, double **alpha,
    double *chisq, void (*funcs)(double, double, double [], double *,
    double [], int), double *alamda);
  static void free_vector(double *v, long nl, long nh);
  static double *vector(long nl, long nh);
  static int *ivector(long nl, long nh);
  static void free_ivector(int *v, long nl, long nh);
  static double **matrix(long nrl, long nrh, long ncl, long nch);
  static void free_matrix(double **m, long nrl, long nrh, long ncl, long nch);
  static void gaussj(double **a, int n, double **b, int m);
  static void mrqcof2(double x1[], double y1[], double w[],double sig[], int ndata, 
                     double a[],
		int ia[], int ma, double **alpha, double beta[], double *chisq,
		void (*funcs)(double,double,double [], double *, double [], int));
  static void covsrt(double **covar, int ma, int ia[], int mfit);

  #define MA 5
  static double a[MA+1]= {0,0,0,0,0,0};
  static double gues[MA+1]= {0,0.1,1.0,0.1,0.01,0.01};
  static double root=0;

  static void modelfunc2(double x1, double y1, 
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

   void nrerror(char error_text[])
   {  fprintf(stderr,"Numerical Recipes run-time error...\n");
      fprintf(stderr,"%s\n",error_text);
      fprintf(stderr,"..now exiting to system...\n");
      exit(1);
   }

   int calibxy2(int argc, char *argv[],int retc, char *retv[])
  {
   FILE *in,*out,*outreg;	
   char in_file[MAXLENGTH];
   char out_file[MAXLENGTH];
   int i,*ia,itst,k,mfit=MA,npt,xmax,xmin;
   double gammaD2O,int_diam,alamda,chisq,ochisq,*x,*x1,*y1,
          *w,*sig,**covar,**alpha,*dummydyda,cwidth;
   char fname[MAXLENGTH];

   gammaD2O=653.5;
   int_diam=0.427;

strcpy(in_file,curexpdir);
strcat(in_file,"/xydata");
strcpy(out_file,curexpdir);
strcat(out_file,"/output");

        if ((in=fopen(in_file,"r"))==NULL)
        {Werrprintf("Error opening input file.\n");
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
        Werrprintf("Error opening output file.\n");
        fclose(in); fclose(out);
        return(ERROR);
        }
 
        fprintf(out,"Output from calibxy2\n");

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
   fprintf(outreg,"Output from fitXYprofiles\n");
   fprintf(outreg,"profile widths in Hz\n");
   fprintf(outreg,"2\t%d\n",npt);
        
        ia=ivector(1,MA);
	dummydyda=vector(1,MA);
	x=vector(1,npt);
	w=vector(1,npt);
	sig=vector(1,npt);
	covar=matrix(1,MA,1,MA);
	alpha=matrix(1,MA,1,MA);
        x1=vector(1,npt); 
        y1=vector(1,npt); 

        i=1;
        while((fscanf(in,"%lf %lf %lf \n", &x1[i],&y1[i],&w[i]))!=EOF)
               { i++;}
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
            xmin=x1[i+i];  
        }
        gues[1]=2*w[1]/(xmax-xmin);
        gues[2]=2*w[1]/(xmax-xmin);
	
        for (i=1;i<=MA;i++) a[i]=gues[i];

        for (i=1;i<npt+1;i++){ 
                  sig[i]=1.0;
          	}

#ifdef DEBUG
	printf("X1\tY1\twidth\n\n");
        for(i=1;i<=npt;i++)
        {
	printf("%f\t%f\t%f\n",x1[i],y1[i],w[i]);
        }
#endif

/* start fitting widths */

	for (i=1;i<=mfit;i++) ia[i]=1;
	for (i=1;i<=MA;i++) a[i]=gues[i];
		alamda = -1;
/*    modelfunc2(x1[1],y1[1],a,&cwidth,dummydyda,MA);  */
        mrqmin2(x1,y1,w,sig,npt,a,ia,MA,covar,alpha,&chisq,modelfunc2,&alamda);
		k=1;
		itst=0;
		for (;;){
			k++;
#ifdef DEBUG
printf("chi squared:  %f\n",chisq);
#endif
			ochisq=chisq;
        mrqmin2(x1,y1,w,sig,npt,a,ia,MA,covar,alpha,&chisq,modelfunc2,&alamda);

			if (chisq > ochisq)
			itst=0;
			else if (fabs(ochisq-chisq) < 0.1)
			itst++;
			if (itst < 4) continue;
			alamda=0.0;
	mrqmin2(x1,y1,w,sig,npt,a,ia,MA,covar,alpha,&chisq,modelfunc2,&alamda);
			break;
	}

 
   /* Calculate linewidths */
   fprintf(out,"Calculated fitting parameters: \n");
   fprintf(out,"%f\t%f\t%f\t%f\t%f\n",a[1], a[2], a[3], a[4], a[5]);
   fprintf(out,"*****************************************************\n");
   fprintf(out,"x1 shim value: \t y1 shim value: \t Profile width, Hz:\n");

   for(i=1;i<=npt; i++){
   modelfunc2(x1[i],y1[i], a, &cwidth, dummydyda, MA);
   fprintf(out,"%f\t%f\t%f\n ",x1[i],y1[i],cwidth); 
   }
   
   printf("Fitting of transverse profile data \n\n");
   printf(" X1 gives %f Hz per DAC point\n", a[1]);
   printf(" Y1 gives %f Hz per DAC point\n", a[2]);
   printf(" Deviation from orthogonality of X1 and Y1 is %6.2f degrees\n", a[3]);
   printf(" Estimated error in X1 shim is %6.0f DAC points\n", a[4]);
   printf(" Estimated error in Y1 shim is %6.0f DAC points\n", a[5]);
   printf(" Final chi-squared is %7.3f Hz squared\n", chisq);
   fprintf(out,"\n\n\n\n chi-squared=%f\n", chisq);

   for(i=1;i<=npt; i++){
   modelfunc2(x1[i],y1[i], a, &cwidth, dummydyda, MA);
   fprintf(outreg,"%i\t%f\n ",i,cwidth);
   }
   for(i=1;i<=npt; i++){
   fprintf(outreg,"%i\t%f\n ",i,w[i]);
   }

   if (retc>0)
   {
    retv[0]=realString((double) a[1]);
   }
    if (retc>1)
   {
    retv[1]=realString((double) a[2]);
   }
    if (retc>2)
   {
    retv[2]=realString((double) a[3]);   
   }
    if (retc>3)
   {
    retv[3]=realString((double) a[4]);   
   }
    if (retc>4)
   {
    retv[4]=realString((double) a[5]);   
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
        fclose(in);
        fclose(out);
        fclose(outreg);
	return 0;
   }

    static double *vector(long nl, long nh)
  { double *v;
   v=(double *)malloc((size_t)((nh-nl+1+NR_END)*sizeof(double)));
   if (!v) {
   printf("allocation failure in vector() of indices  %ld  and  %ld\n",nl,nh);
	nrerror("allocation failure in vector() ");
    }
      return v-nl+NR_END;
   }

      static int *ivector(long nl, long nh)
    {
        int *v;
        v=(int *)malloc((size_t) ((nh-nl+1+NR_END)*sizeof(int)));
        if (!v) nrerror("allocation failure in ivector()");
        return v-nl+NR_END;
    }

      static void free_vector(double *v, long nl, long nh)
      { free((FREE_ARG) (v+nl-NR_END)); }

      static void free_ivector(int *v, long nl, long nh)
      {
        free((FREE_ARG) (v+nl-NR_END));
      }

      static double **matrix(long nrl, long nrh, long ncl, long nch)
      {
        long i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
        double **m;
        m=(double **) malloc((size_t)((nrow+NR_END)*sizeof(double*)));
        if (!m) nrerror("allocation failure 1 in matrix()");
        m += NR_END;
        m -= nrl;
        m[nrl]=(double *) malloc((size_t)((nrow*ncol+NR_END)*sizeof(double)));
        if (!m[nrl]) nrerror("allocation failure 2 in matrix()");
        m[nrl] += NR_END;
        m[nrl] -= ncl;
        for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;
        return m;
      }

      static void free_matrix(double **m, long nrl, long nrh, long ncl, long nch)
      {
        free((FREE_ARG) (m[nrl]+ncl-NR_END));
        free((FREE_ARG) (m+nrl-NR_END));
      } 

    static void mrqmin2(double x1[],double y1[],double w[], double sig[], 
        int ndata,double a[], int ia[], int ma, double **covar, double **alpha,
        double *chisq, void (*funcs)(double, double, double [], double *,
	double [], int), double *alamda)
      {
	void covsrt(double **covar, int ma, int ia[], int mfit);
	int j,k,l,m;
	static int mfit;
	static double ochisq,*atry,*beta,*da,**oneda;

	if (*alamda < 0.0) {
		atry=vector(1,ma);
		beta=vector(1,ma);
		da=vector(1,ma);
		for (mfit=0,j=1;j<=ma;j++)
			if (ia[j]) mfit++;
		oneda=matrix(1,mfit,1,1);
		*alamda=0.001;
		mrqcof2(x1,y1,w,sig,ndata,a,ia,ma,alpha,beta,chisq,funcs);
		ochisq=(*chisq);
		for (j=1;j<=ma;j++) atry[j]=a[j];
	}
	for (j=0,l=1;l<=ma;l++) {
		if (ia[l]) {
			for (j++,k=0,m=1;m<=ma;m++) {
				if (ia[m]) {
					k++;
					covar[j][k]=alpha[j][k];
				}
			}
			covar[j][j]=alpha[j][j]*(1.0+(*alamda));
			oneda[j][1]=beta[j];
		}
	}
	gaussj(covar,mfit,oneda,1);
	for (j=1;j<=mfit;j++) da[j]=oneda[j][1];
	if (*alamda == 0.0) {
		covsrt(covar,ma,ia,mfit);
		free_matrix(oneda,1,mfit,1,1);
		free_vector(da,1,ma);
		free_vector(beta,1,ma);
		free_vector(atry,1,ma);
		return;
	}
	for (j=0,l=1;l<=ma;l++)
		if (ia[l]) atry[l]=a[l]+da[++j];
	mrqcof2(x1,y1,w,sig,ndata,atry,ia,ma,covar,da,chisq,funcs);
	if (*chisq < ochisq) {
		*alamda *= 0.1;
		ochisq=(*chisq);
		for (j=0,l=1;l<=ma;l++) {
			if (ia[l]) {
				for (j++,k=0,m=1;m<=ma;m++) {
					if (ia[m]) {
						k++;
						alpha[j][k]=covar[j][k];
					}
				}
				beta[j]=da[j];
				a[l]=atry[l];
			}
		}
	} else {
		*alamda *= 10.0;
		*chisq=ochisq;
	}

     }
     static void mrqcof2(double x1[], double y1[],double y[], double sig[], 
                         int ndata, double a[], int ia[],
        int ma, double **alpha, double beta[], double *chisq,
        void (*funcs)(double,double, double [], double *, double [], int))
     {
        int i,j,k,l,m,mfit=0;
        double ymod,wt,sig2i,dy,*dyda;
        dyda=vector(1,ma);
        for (j=1;j<=ma;j++)
                if (ia[j]) mfit++;
        for (j=1;j<=mfit;j++) {
                for (k=1;k<=j;k++) alpha[j][k]=0.0;
                beta[j]=0.0;
        }
        *chisq=0.0;
        for (i=1;i<=ndata;i++) {
                (*funcs)(x1[i],y1[i],a,&ymod,dyda,ma);
                sig2i=1.0/(sig[i]*sig[i]);
                dy=y[i]-ymod;
                for (j=0,l=1;l<=ma;l++) {
                        if (ia[l]) {
                                wt=dyda[l]*sig2i;
                                for (j++,k=0,m=1;m<=l;m++)
                                        if (ia[m]) alpha[j][++k] += wt*dyda[m];
                                beta[j] += dy*wt;
                        }
                }
                *chisq += dy*dy*sig2i;
        }
        for (j=2;j<=mfit;j++)
                for (k=1;k<j;k++) alpha[k][j]=alpha[j][k];
        free_vector(dyda,1,ma);
     }
 
     static void covsrt(double **covar, int ma, int ia[], int mfit)
     {
        int i,j,k;
        double swap;
        for (i=mfit+1;i<=ma;i++)
                for (j=1;j<=i;j++) covar[i][j]=covar[j][i]=0.0;
        k=mfit;
        for (j=ma;j>=1;j--) {
                if (ia[j]) {
                 for (i=1;i<=ma;i++) SWAP(covar[i][k],covar[i][j])
                 for (i=1;i<=ma;i++) SWAP(covar[k][i],covar[j][i])
                        k--;
                }
         }
      }

      static void gaussj(double **a, int n, double **b, int m)
      {
        int *indxc,*indxr,*ipiv;
        int i,icol,irow,j,k,l,ll;
        double big,dum,pivinv;
        double swap;
        indxc=ivector(1,n);
        indxr=ivector(1,n);
        ipiv=ivector(1,n);
        
        icol = irow = 0;
        for (j=1;j<=n;j++) ipiv[j]=0;
        for (i=1;i<=n;i++) {
                big=0.0;
                for (j=1;j<=n;j++)
                        if (ipiv[j] != 1)
                                for (k=1;k<=n;k++) {
                                        if (ipiv[k] == 0) {
                                                if (fabs(a[j][k]) >= big) {
                                                        big=fabs(a[j][k]);
                                                        irow=j;
                                                        icol=k;
                                                }
                    } else if (ipiv[k] > 1) nrerror("gaussj: Singular Matrix-1");
                  }
                ++(ipiv[icol]);
                if (irow != icol) {
                        for (l=1;l<=n;l++) SWAP(a[irow][l],a[icol][l])
                        for (l=1;l<=m;l++) SWAP(b[irow][l],b[icol][l])
                }
                indxr[i]=irow;
                indxc[i]=icol;
                if (a[icol][icol] == 0.0) nrerror("gaussj: Singular Matrix-2");
                pivinv=1.0/a[icol][icol];
                a[icol][icol]=1.0;
                for (l=1;l<=n;l++) a[icol][l] *= pivinv;
                for (l=1;l<=m;l++) b[icol][l] *= pivinv;
                for (ll=1;ll<=n;ll++)
                        if (ll != icol) {
                                dum=a[ll][icol];
                                a[ll][icol]=0.0;
                                for (l=1;l<=n;l++) a[ll][l] -= a[icol][l]*dum;
                                for (l=1;l<=m;l++) b[ll][l] -= b[icol][l]*dum;
                        }
        }
        for (l=n;l>=1;l--) {
                if (indxr[l] != indxc[l])
                        for (k=1;k<=n;k++)
                                SWAP(a[k][indxr[l]],a[k][indxc[l]]);
        }
        free_ivector(ipiv,1,n);
        free_ivector(indxr,1,n);
        free_ivector(indxc,1,n);

     }




