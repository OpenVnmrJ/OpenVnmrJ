/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*polycoeff.c */

/*modified from RGS1ATAN.PAS for c syntax */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define maxsize 8192
#define maxshim 8
#define minpoints 3

#define SQR(x)(x*x)

float newphase(float wi, float wf);
float newphase_old(float wi, float wf);
void calpolycoeff(int np,int nf,int si,float fov,float dt,float *wbuff1,float *wbuff0,float *a0,float *a,float *b,float *c,float *chisq, int nn);


float newphase(float wi, float wf)
  {
  double ia;
  float temp;
      temp = wf-wi;
      ia = (double)(temp/fabs(temp));
      temp=fabs(temp);
      if (temp>30.0e-3) 
      {
      temp=fmod(temp,36.0e-3);
      if ((temp<6.0e-3) || (temp>30.0e-3))
      {
      temp=(int)(fabs(wf-wi)/36.0e-3+0.5);
      wf = wf - temp*ia*36.0e-3;
      }
      }
  return wf;
  }  

void calpolycoeff(int np,int nf,int si,float fov,float dt,float *wbuff1,float *wbuff0,float *a0,float *a,float *b,float *c, float *chisq, int nn)
  {
  double x, ia, sum_xy,sum_xx,sum_y,sum_x2y,sum_x4, sum_x3y, sum_x6;
  int   offset,off1,i,j;
  float  temp;
  char logfile[255];
  FILE *fp;

    offset = 2*np+2+2*(nf-1)*si+1; /*+2 for writing in imaginary part +1 for real part of spc*/   
    off1 = (2*nf-1)*(si/2);

    sum_xy = 0;
    sum_xx = 0;
    for (i = -np; i<np+1; i++) 
    {
      wbuff0[2*i-1+offset] = wbuff1[2*i+off1];
      wbuff0[2*i+offset] = wbuff1[2*i+off1+1];
    }
    i = 0;
    x = (1.0*i)/np;

fprintf(stderr,"Value of np : %d\n", np);

    sum_y = (double)wbuff0[2*i-1+offset];
    sum_xy = (double)(x*(wbuff0[2*i-1+offset]));
    sum_xx = (x*x);
    sum_x2y = x*sum_xy;
    sum_x4 = (sum_xx*sum_xx);
    sum_x6 = sum_xx*sum_x4;
    sum_x3y = x*sum_x2y;
    j=1;
    while ((j<np+1) && (wbuff0[2*j+offset]>0) && (wbuff0[-2*j+offset]>0))
    {
      i = j;
      x = (1.0*i)/np;
      wbuff0[2*i-1+offset] = newphase(wbuff0[2*(i-1)-1+offset],wbuff0[2*i-1+offset]);
      sum_y = (double)wbuff0[2*i-1+offset]+sum_y;
      sum_xy = (double)(x*(wbuff0[2*i-1+offset]))+sum_xy;
      sum_xx = x*x+sum_xx;
      sum_x2y = (double)(x*x*(wbuff0[2*i-1+offset]))+sum_x2y;
      sum_x4 = x*x*x*x+sum_x4;
      sum_x3y = (double)(x*x*x*wbuff0[2*i-1+offset])+sum_x3y;
      sum_x6 = x*x*x*x*x*x+sum_x6;
      i=-j;
      x = (1.0*i)/np;
      wbuff0[2*i-1+offset] = newphase(wbuff0[2*(i+1)-1+offset],wbuff0[2*i-1+offset]);
      sum_y = (double)wbuff0[2*i-1+offset]+sum_y;
      sum_xy = (double)(x*(wbuff0[2*i-1+offset]))+sum_xy;
      sum_xx = x*x+sum_xx;
      sum_x2y = (double)(x*x*(wbuff0[2*i-1+offset]))+sum_x2y;
      sum_x4 = x*x*x*x+sum_x4;
      sum_x3y = (double)(x*x*x*wbuff0[2*i-1+offset])+sum_x3y;
      sum_x6 = x*x*x*x*x*x+sum_x6;
      j=j+1;
    }
    np=j-1;
    *a0 = (float)((sum_y)/((2*np+1)));
    *a = 0.0;
    *b = 0.0;
    *c = 0.0;

    if (np<minpoints)
    {
      *chisq = 1000.0;
    }
    else
    {
    if (nn>1)
    {
    *a0 = (float)((sum_xx*sum_x2y-sum_y*sum_x4)/((sum_xx*sum_xx)-(2*np+1)*sum_x4));
    *b = (float)((sum_xx*sum_y-(2*np+1)*sum_x2y)/((sum_xx*sum_xx)-(2*np+1)*sum_x4));
     }
    if ((nn<3) && (nn>0))
    {
     *a = (float)(sum_xy)/(sum_xx);
    *c = 0.0;
     }
    if (nn>2) 
    {
     *a = (float)(sum_x3y*sum_x4-sum_xy*sum_x6)/(sum_x4*sum_x4-sum_xx*sum_x6);
     *c = (float)((sum_xy*sum_x4-sum_x3y*sum_xx)/(sum_x4*sum_x4-sum_x6*sum_xx));
    }
    *chisq=0.0;

    for (i = -np; i<np+1; i++) 
    {
      wbuff0[2*i-1+offset+si] = wbuff0[2*i-1+offset]-(*a0+i*(*a)/np+SQR(1.0*i/np)*(*b)+i*(*c)/np*SQR(1.0*i/np));
      *chisq = *chisq+(wbuff0[2*i-1+offset+si]*wbuff0[2*i-1+offset+si]);
/*wbuff0[2*i+offset+si] for writing in imaginary part wbuff0[2*i-1+offset+si] for real part of spc*/
    }
  /*Scaling:
    =======
    for x=-1..1; 
    x:  ((np/si)*SW/(1000*G)) for G = Gradient in [1000Hz/cm], e.g. 1
        i.e. (np*FOV/si)
    nu: 1/(360*T) for T = extra delay in [10ms], hence dt*1e2 e.g. 
  */

    *a = (*a)*(si/(2*fov*np*360e-6*dt*1.0e2));
    *b = (*b)*SQR(si/(2*fov*np))/(360e-6*dt*1.0e2);
    *c = (*c)*si/(2*fov*np*360e-6*dt*1.0e2)*SQR(si/(2*fov*np));
    *a0 = *a0/(360e-6*dt*1.0e2);
    *chisq = sqrt(*chisq)/((360e-6*dt*1.0e2)*(2.0*np+1));
    }

    strcpy(logfile,getenv("vnmruser"));
    strcat(logfile,"/shims/fastmap.log");
    fp = fopen(logfile,"a");
    fprintf(fp,"%+5.1f %+5.2f %+4.2f %+5.3f (%+3.2f) \n",*a0,*a,*b,*c,*chisq);
    fclose(fp);
    fprintf(stderr,"%+5.1f %+5.2f %+4.2f %+5.3f (%+3.2f) \n",*a0,*a,*b,*c,*chisq);
  }/*calpolycoeff*/
  

