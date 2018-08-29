/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

FILE *inpfile,*macfile;
float fun();
void nrerror();

/* program for estimation of cq, etaq, and viso from three singularities
 of the MAS central transition lineshape of a half-integer nucleus

H. Bildsoe, Dept. of Chem., Aarhus University, DK-8000 Aarhus C, Denmark

march 1995 */



main(argc,argv)
int argc;
char *argv[];
{
int i,j,icase,sites;
float f[5],freq[3];
float ival,cq,viso,eta,cur,delta,r;
float a,v0;
char *sing[3];
char *sinname[5];
/* initialize array sinname with allowed singularity names */
sinname[0] = "f2";
sinname[1] = "p2";
sinname[2] = "p1";
sinname[3] = "s1";
sinname[4] = "f1";
sing[0] = "f2"; 
sing[1] = "p2"; 
sing[2] = "p1";
for (i=0;i<5;i++) f[i] = 0.0;
if ((inpfile = fopen(argv[1],"r")) == NULL) nrerror("can not open inputfile");
 /* read singularity labels */
fscanf(inpfile,"%d",&sites);
for (i=0;i<=2;i++) fscanf(inpfile,"%f %s",&freq[i],sing[i]);
fscanf(inpfile,"%f %f",&ival,&v0);
fclose(inpfile);
if (fabs((float)((int)(ival+0.1))-ival)<0.1 || ival<1.1) 
  nrerror("no central transition for this ival");
a = (18.0*ival*(ival+1.0)-13.5)/(ival*ival*(2.0*ival-1.0)*(2.0*ival-1.0)*32.0); 

/* convert names to indices into array sinname and sort into order f2,p2,p1,s1,f1 */
for (i=0, icase=0;i<=4;i++) { 
  for (j=0;j<3; j++) if (strncmp(sinname[i],sing[j],2)==0) break;
  if (j<3) {
    f[i] = freq[j];
    sing[j] = "  ";   /* remove label from list */
    icase += 1<<i;
    }
  }

for (i=0;i<=2;i++)    /* check for remaining labels */
 if (strncmp(sing[i],"  ",2) != 0) 
    nrerror("unknown singularity name");
 

switch (icase)
{ 
case 7: /* case of f2,p2,p1 */
if (f[0]<f[1] || f[1]<f[2]) nrerror("frequencies and labels are inconsistent");
r=(f[0]-f[1])/(f[0]-f[2]);
if (r<0.0 || r>1.1) nrerror("impossible data");
  else if (r>=1.0) eta=1.0;
  else if (r==0.0) eta=0.0;
  else if (r>7.0/15.0) eta = 1.0 - 2.0/r + 2.0*sqrt(1.0-r+r*r)/r;
  else eta = -6.0/50.0 + (96.0 - 3.0*sqrt(128.0*(8.0+7.0*r)*(1.0-r)))/(50.0*r);
cq = sqrt((f[0]-f[2])*v0/a/fun(eta,2)/1.0e6);
break;

case 11:  /* f2,p2,s1 */
if (f[0]<f[1] || f[1]<f[3]) nrerror("frequencies and labels are inconsistent");
r = (f[0]-f[1])/(f[1]-f[3]); 
if (r<0.0 || r>26.0/25.0) nrerror("impossible data");
  else if (r>=24.0/25.0) eta = 1.0;
  else if (r==0.0) eta = 0.0;
  else eta = 3.0/25.0 + (96.0-3.0*sqrt(128.0*(8.0-7.0*r)*(1.0+r)))/(50.0*r);
cq = sqrt((f[0]-f[3])*v0/a/fun(eta,3)/1.0e6);
break;

case 19:  /* f2,p2,f1 */
if (f[0]<f[1] || f[1]<f[4]) nrerror("frequencies and labels are inconsistent");
r = (f[0]-f[1])/(f[1]-f[4]);
if (r<0.0 || r>26.0/25.0) nrerror("impossible data");
  else if (r>=24.0/25.0) eta = 1.0;
  else if (r==0.0) eta = 0.0;
  else eta = 6.0 + 12.0*(1.0-sqrt(1.0+r))/r;
cq = sqrt((f[0]-f[4])*v0/a/fun(eta,4)/1.0e6);
break;

case 13: /* f2,p1,s1 */
if (f[0]<f[2] || f[2]<f[3]) nrerror("frequencies and labels are inconsistent");
r = (f[2]-f[3])/(f[0]-f[3]);
if (r<0.0 || r>27.0/49.0) nrerror("impossible data");
  else if (r>=25.0/49.0) eta = 1.0;
  else if (r==0.0) eta = 0.0;
  else if (r>49.0/169.0) eta = -9.0/5.0+(336.0/5.0 +48.0*sqrt(r))/(49.0-25.0*r);
  else eta = -9.0/5.0 + (84.0-12.0*sqrt(7.0*(7.0-15.0*r)))/(50.0*r);
cq = sqrt((f[0]-f[3])*v0/a/fun(eta,3)/1.0e6);
break;

case 21: /* f2,p1,f1 */
if (f[0]<f[2] || f[2]<f[4]) nrerror("frequencies and labels are inconsistent");
r = (f[0]-f[2])/(f[2]-f[4]);
if (r<22.0/25.0 || r>10.0/7.0) nrerror("impossible data");
  else if (r>=9.0/7.0) eta = 0.0;
  else if (r<=24.0/25.0) eta = 1.0;
  else if (r>8.0/7.0)
    eta = 1.0 - (28.0+6.0*sqrt(7.0*(7.0*r-8.0)*(1.0+r)))/(25.0+21.0*r);
  else eta = (84.0 + 21.0*sqrt(8.0*(8.0-7.0*r)*(1.0+r)))/(14.0*(6.0+7.0*r));
cq = sqrt((f[0]-f[4])*v0/a/fun(eta,4)/1.0e6);
break;

case 25: /* f2,s1,f1 */
if (f[0]<f[3] || f[3]<f[4]) nrerror("frequencies and labels are inconsistent");
r = (f[3]-f[4])/(f[0]-f[3]); 
if (r<0.0 || r>8.0/9.0) nrerror("impossible data");
  else if (r>=7.0/9.0) eta = 0.0;
  else eta = -9.0/5.0 + (84.0/5.0 + 42.0*sqrt(1.0+r))/(21.0+25.0*r);
cq = sqrt((f[0]-f[4])*v0/a/fun(eta,4)/1.0e6);
break;

case 14: /* p2,p1,s1 */
if (f[1]<f[2] || f[2]<f[3]) nrerror("frequencies and labels are inconsistent");
r = (f[1]-f[2])/(f[1]-f[3]); 
if (r<0.0 || r>1.1) nrerror("impossible data");
  else if (r>=1.0) eta = 0.0;
  else if (r<64.0/113.0) 
    eta = 3.0/25.0-(1344.0-75.0*sqrt(128.0*(8.0+7.0*r)*(1.0-r)))/(50.0*(24.0+25.0*r));
  else eta = 3.0/25.0 +(84.0 - 12.0*sqrt(7.0*r*(15.0-8.0*r)))/(50.0*(1.0-r));
cq = sqrt((f[1]-f[3])*v0/a/fun(eta,6)/1.0e6);
break;

case 22: /* p2,p1,f1 */
if (f[1]<f[2] || f[2]<f[4]) nrerror("frequencies and labels are inconsistent");
r = (f[1]-f[2])/(f[2]-f[4]);
if (r<0.0 || r>10.0/7.0) nrerror("impossible data");
  else if (r>=9.0/7.0) eta = 0.0;
  else if (r>64.0/105.0) eta = 1.0 +(20.0-42.0*sqrt(r*(1.0+r)))/(25.0+21.0*r);
  else eta = (-84.0+21.0*sqrt(8.0*(8.0-7.0*r)*(1.0+r)))/(14.0*(6.0+7.0*r));
cq = sqrt((f[1]-f[4])*v0/a/fun(eta,7)/1.0e6);
break;

case 26: /* p2,s1,f1 */
if (f[1]<f[3] || f[3]<f[4]) nrerror("frequencies and labels are inconsistent");
r = (f[3]-f[4])/(f[1]-f[4]); 
if (r<0.0 || r>8.0/16.0) nrerror("impossible data");
  else if (r>=7.0/16.0) eta = 0.0;
  else eta = 6.0 - (147 - 6.0*sqrt(7.0*(7.0-15.0*r)))/(21.0+4.0*r);
cq = sqrt((f[1]-f[4])*v0/a/fun(eta,7)/1.0e6);
break;

case 28: /* p1,s1,f1 */
if (f[2]<f[3] || f[3]<f[4]) nrerror("frequencies and labels are inconsistent");
r = (f[3]-f[4])/(f[2]-f[4]); 
if (r<0.0 || r>1.1) nrerror("impossible data");
  else if (r>=1.0) eta = 0.0;
  else if (r>8.0/15.0) eta = 1.0 - 2.0*(1.0 - sqrt(1.0-r+r*r))/(1.0-r);
  else  eta = (-42.0 + 12.0*sqrt(7.0*(1.0-r)*(7.0+8.0*r)))/(14.0*(3.0+4.0*r));
cq = sqrt((f[2]-f[4])*v0/a/fun(eta,9)/1.0e6);
break;
}


a *= cq*cq/v0*1.0e6;
/* calculate positions of remaining two singularities */
switch (icase)
{

case 7:
  cur = f[3]= f[0]-fun(eta,3)*a;
  f[4] = f[0]-a*fun(eta,4);
  delta = f[3]-f[4];
  break;
case 11:
  cur = f[2] = f[0]-a*fun(eta,2);
  f[4] = f[0]-a*fun(eta,4);
  delta = f[2]-f[4];
  break;
case 19: 
  cur = f[2] = f[0]-a*fun(eta,2);
  f[3] = f[0]-a*fun(eta,3);
  delta = f[2]-f[3];
  break;
case 13: 
  cur = f[1] = f[0]-a*fun(eta,1);
  f[4] = f[0]-a*fun(eta,4);
  delta = f[1]-f[4];
  break;
case 21: 
  cur = f[1] = f[0]-a*fun(eta,1);
  f[3] = f[0]-a*fun(eta,3);
  delta = f[1]-f[3];
  break;
case 25:
  cur = f[1] = f[0]-a*fun(eta,1);
  f[2] = f[0]-a*fun(eta,2);
  delta = f[1]-f[2];
  break;
case 14: 
  cur = f[0] = f[1]+a*fun(eta,1);
  f[4] = f[1]-a*fun(eta,7);
  delta = f[0]-f[4];
  break;
case 22: 
  cur = f[0] = f[1]+a*fun(eta,1);
  f[3] = f[1]-a*fun(eta,6);
  delta = f[0]-f[3];
  break;
case 26:
  cur = f[0] = f[1]+a*fun(eta,1);
  f[2] = f[1]-a*fun(eta,5);
  delta = f[0]-f[2];
  break;
case 28:
  cur = f[0] = f[2]+a*fun(eta,2);
  f[1] = f[2]+a*fun(eta,5);
  delta = f[0]-f[1];
  break;
}
a = (2.0*ival*(ival+1.0)-1.5)/(ival*ival*(2.0*ival-1.0)*(2.0*ival-1.0));
a *= 3.0*cq*cq/32.0/7.0/v0;
a *= (1.0-eta)*(1.0-eta)*1.0e6;
viso = f[0]+a;
if ((macfile = fopen(argv[2],"w")) == NULL) nrerror("can't open macrofile");
if (sites==1) fprintf(macfile,"cq=%f etaq=%f viso=%f\n",cq,eta,viso);
  else fprintf(macfile,"cq%1d=%f etaq%1d=%f viso%1d=%f\n",sites,cq,sites,eta,sites,viso);
fprintf(macfile,"r1=%f r2=%f r3=%f r4=%f r5=%f\n",f[0],f[1],f[2],f[3],f[4]);
fprintf(macfile,"cr=%f delta=%f\n",cur,delta);
if (icase==21) 
  printf("warning: etas can not be determined accurately from f2,p1,f1\n");
}

float fun(x,ifn)
float x;
int ifn;
{
float z;

/* calculate "reduced" distance between two singularities for eta=x */
switch (ifn)
{  
case 1: /* distance f2-p2 */
  z = 4.0*x/21.0;
  break;
case 2: /* f2-p1 */
  if (x>(3.0/7.0)) 
     z = (3.0-x)*(1.0+x)/21.0;
     else z = (9.0+2.0*x/3.0+25.0*x*x/9.0)/56.0;
  break;
case 3: /* f2-s1 */
  z = (3.0+5.0*x/3.0)*(3.0+5.0*x/3.0)/56.0;
  break;
case 4:  /* f2-f1 */
  z = (2.0+x/3.0)*(2.0+x/3.0)/14.0;
  break;
case 5:  /* p2-p1 */
  if (x>(3.0/7.0)) 
    z = -(x-1.0)*(x+3.0)/21.0;
    else z = (3-5.0*x/3.0)*(3.0-5.0*x/3.0)/56.0;
  break;
case 6:  /* p2-s1 */
  z = (9.0-2.0*x/3.0+25.0*x*x/9.0)/56.0;
  break;
case 7:  /* p2-f1 */
  z = (2.0-x/3.0)*(2.0-x/3.0)/14.0;
  break;
case 8:  /* p1-s1 */
  if (x>(3.0/7.0)) 
    z = 7.0*(3.0/7.0+x)*(3.0/7.0+x)/72.0;
    else  z = x/6.0;
  break;
case 9:  /* p1-f1 */
  if (x>(3.0/7.0)) 
    z = 1.0/7.0+x*x/18.0;
    else z = -(x+1.0)*(x-3.0)/24.0;
  break;
case 10:  /* s1-f1 */
  z = -(x-1.0)*(x+3.0)/24.0;
  break;
}
return z;
}

void nrerror(error_text)
char error_text[];
{
printf("%s\n",error_text);
fflush(stdout);
exit(-1);
}





