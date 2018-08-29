/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* it may be necessary to define ISOTRPIC by concatnation */

/*rgs1atan.c */

/*modified from RGS1ATAN.PAS for c syntax */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define maxsize 65536
#define maxshim 8

#define SQR(x)(x*x)
#define scal_z2_z0 1.0e-12

#define DEBUG 1


typedef float poly_coeff[12];
typedef struct { int typ; float coeff; int dcur; } shims;

int ns;

float dphi(float x1,float x2,float y1,float y2);
void getinfo(int *si, int *nproj, float *wbuff, char infile_name1[]);
void calpolycoeff(int np,int nf,int si,float fov,float dt,float *wbuff1,float *wbuff0,float *a0,float *a,float *b,float *c,float *chisq, int nn);
void getphase(int si, int idx, int wd2, float *wbuff1, float *wbuff, int ni, int nf);

void getcal(char *calfile, float *scal);
void cal_shim(char *pos[3], poly_coeff a1,poly_coeff a2,shims *sval,float *scal);


main(int argc, char *argv[])
{ 

extern int ns;

float   delta_t,wd,b0,b1,b2,b3,chisq,fov;
int     size,nf,ne,wd2,nord,maxproj;
int     j;
float    wrkbuff0[maxsize],wrkbuff1[maxsize],wrkbuff2[maxsize],wrkbuff3[maxsize];
FILE    fil_out;
shims   shim_val[maxshim];
float   shim_cal[(maxshim+1)*maxshim];
poly_coeff a1,a2,a3,chi;
char    ch; 
char 	string[100], fil_in[100], logfile[255], errfile[255], calfile[255];
FILE    *fp, *fp1;

 fprintf(stderr," %d arguments passed to %s\n",argc,argv[0]);
 if (argc == 13) 
 {
   ns=3;
 }
 if (argc == 16) 
 {
   ns=6;
 }

  strcpy(fil_in,argv[1]);

  getdata(&size,&maxproj,&wrkbuff2[0],fil_in);

  delta_t = (float)atof(argv[2]);
  nord = atoi(argv[3]);
  fov = atof(argv[4]);

  strcpy(logfile,getenv("vnmruser")); 
  strcat(logfile,"/shims/fastmap.log");

  fp = fopen(logfile,"a");
  fprintf(fp," the following units are in Hz/cm^n \n");
  fprintf(fp,"nf j   a0    a1     a2     a3    (RMSD)\n");
  fclose(fp);

  fprintf(stderr," the following units are in Hz/cm^n \n");
  fprintf(stderr,"nf j   a0    a1     a2     a3    (RMSD)\n");

#ifdef DEBUG
  fprintf(stderr,"Before the first for instruction : nf = %d and ns = %d\n", nf, ns);
#endif

  for (nf = 0; nf<ns; nf++) 
  {
    a1[nf] = 0.0;
    a2[nf] = 0.0;
    a3[nf] = 0.0;
    chi[nf] = 0.0;

 fprintf(stderr,"ne value : %d and maxproj value : %d \n",ne,maxproj);

    for (ne=1; ne<maxproj/ns; ne++)
    {
      wd = atof(argv[5+nf]);
      wd2 = (int)(size*wd*0.25);
      getphase(size,nf,wd2,&wrkbuff1[0],&wrkbuff2[0],(int)(nf*maxproj/ns),(int)(nf*maxproj/ns+ne));

#ifdef DEBUG
  fprintf(stderr,"%s %f %d %d \n",fil_in,delta_t,wd2,nord);
#endif
  
      fprintf(stderr,"%d %+7.4f ",nf,ne*delta_t);         
#ifdef DEBUG
  fprintf(stderr,"Before calpolycoeff : wd2=%d , nf=%d, size=%d, fov=%f, nord=%d \n",wd2, nf, size, fov, nord);
#endif
      calpolycoeff(wd2,(nf+1),size,fov,delta_t*ne,wrkbuff1,&wrkbuff0[0],&b0,&b1,&b2,&b3,&chisq,nord);
#ifdef DEBUG
  fprintf(stderr,"After calpolycoeff and before the if\n");
#endif
      if (ne>0)
      {
        for (j = 0; j<2*wd2; j++)
        {
          wrkbuff3[(nf*maxproj/ns+ne)*size+2*j] = wrkbuff0[(nf*2+1)*size+2*j];
          wrkbuff3[(nf*maxproj/ns+ne)*size+2*j+1] = wrkbuff0[(nf*2)*size+2*j];
        }
      }
      chisq*=chisq;
      a1[nf]=a1[nf]+b1/chisq;
      a2[nf]=a2[nf]+b2/chisq;
      a3[nf]=a3[nf]+b3/chisq;
      chi[nf]=chi[nf]+1/chisq;
    if (((delta_t<0.0001) || (chisq>2.0)) && (ne == 1))
    {
      strcpy(errfile,getenv("vnmruser"));
      strcat(errfile,"/shims/fastmap.err");
      fp1 = fopen(errfile,"a");
      if (chisq>1.0E04)
      {
        fprintf(fp1,"ERROR: Not enough Sensitivity in the %dth projection \n Check position, Rx gain and tpwr\n",nf+1);
      }
      if ((delta_t<0.0001) && (nf<1))
      {
        fprintf(fp1,"ERROR: Not enough B0 encoding : Check tau \n");
      }
      if ((chisq>2.25) && (chisq<1.000001E04))
      {
        fprintf(fp1,"ERROR: RMSD Phase error in the %dth projection is above 1.5 Hz\n Check tpwr, tau, sensitivity and B0 homogeneity \n",nf+1);
      }
      fclose(fp1);
    }
    }
    a1[nf]=a1[nf]/chi[nf];
    a2[nf]=a2[nf]/chi[nf];
    a3[nf]=a3[nf]/chi[nf];
  }



 fprintf(stderr," AFTER the FOR loop ...\n");


    if (maxproj/ns>2) 
    {
      for (nf = 0; nf<ns; nf++) fprintf(stderr," %d %+6.3f %+6.3f %+6.3f\n",nf, a1[nf],a2[nf],a3[nf]);
    }
  putdata(&size,&wrkbuff3[0],fil_in);
  printf("\n");
  strcpy(calfile,getenv("FASTMAP"));
  strcat(calfile,"/calib/.");
  strcat(calfile,argv[5+ns]);
  getcal(calfile,&shim_cal[0]);
  cal_shim(&argv[7+ns],a1,a2,&shim_val[0],&shim_cal[0]);

  printf("\n");
}
  
    float k_x(poly_coeff a1)
    {
      extern int ns;
      if (ns == 3) return a1[0];
      if (ns == 6) return (a1[0]+a1[1]+a1[4]-a1[5])*0.35355;
    }

    float k_y(poly_coeff a1)
    {
      extern int ns;
      if (ns == 3) return a1[1];
      if (ns == 6) return (a1[0]-a1[1]+a1[2]-a1[3])*0.35355;
    }

    float k_z(poly_coeff a1)
    {
      extern int ns;
      if (ns == 3) return a1[2];
      if (ns == 6) return (a1[2]+a1[3]+a1[4]+a1[5])*0.35355;
    }

    float k_zx(poly_coeff a2)
    {
      extern int ns;
      if (ns == 3) return 0.0;
      if (ns == 6) return (a2[4]-a2[5])*1.000;
    }
   
    float k_zy(poly_coeff a2) 
    {
      extern int ns;
      if (ns == 3) return 0.0;
      if (ns == 6) return (a2[2]-a2[3])*1.000;
    }

    float k_xy(poly_coeff a2)
    {
      extern int ns;
      if (ns == 3) return 0.0;
      if (ns == 6) return (a2[0]-a2[1])*0.500;
    }

    float k_x2y2(poly_coeff a2)
    {
      extern int ns;
      if (ns == 3) return (a2[0]-a2[1])*0.5;
      if (ns == 6) return (-a2[2]-a2[3]+a2[4]+a2[5])*0.500;
    }

    float k_z2(poly_coeff a2)  
    {
      extern int ns;
      if (ns == 3) return (-a2[0]-a2[1]+2.0*a2[2])*0.33333;
      if (ns == 6) return (-2*a2[0]-2*a2[1]+a2[2]+a2[3]+a2[4]+a2[5])*0.33333;
    }

#include "cal_shim.c"

/********************************************************************************************
		Modification History

20080904 (ss) - calib file path changed, fastmap/calib/

********************************************************************************************/
