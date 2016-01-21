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

/*
    Varian NMR Palo Alto (SVR4 vs. SunOS):
              Changed UNIX include file <sys/file.h> to <fcntl.h>
              Changed L_INCR to SEEK_CUR
                                                                     */

#include <fcntl.h>

#include "starsprog.h"
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>

extern FILE *resfile;
int quadflag,qcsaflag,csaflag;
int *cq,*etaq,*csa,*etas,*psi,*chi,*xi,*viso,*amp;
int lb=parmax+1,gf=parmax+1,rfw=parmax+1,theta=parmax+1;
int pulsestep,rmsflag,sites;

float ival,sw,sp,gb1,pw,vrot,sfrq;
float gb1r,pwt,vrotr,sfrqr;
int nt,npar,totalpar,ntrans,fn,np,pft,pft2,ndata,progorder,maxiter;
int cflag,sflag;
char pulse[6];
float digres;
float *cc,*ss,*cg,*sg,*c2g,*s2g;
float *cfi,*c2fi,*sfi,*s2fi,*c3fi,*s3fi,*c4fi,*s4fi;

float mval[7];   /* for I<=11/2 */
float **expint,*expspec;
float qcpar[parmax+1],qcinc[parmax+1],tran[7];
char spectype[5];

void readexp_1(argv2)
char argv2[];

/* get experimental ssb integrals*/
{
FILE *expfile;
float dum,ampmax,x1,x2,x3;
char str[81],c;
float x;
int i,nform,sitectr;
if ((expfile=fopen(argv2,"r")) == NULL) 
  nrerror("Error: Can not open file of experimental ssb integrals");
expint = matrix(1,sites,0,pft);
for (sitectr=1;sitectr<=sites;sitectr++) 
  for (i=0; i<pft; expint[sitectr][i++] = 0.0);
ndata = 0;
ampmax = 0.0;
/* Read file of experimental ssb integrals*/
/* First check which format is used, i.e. whether a site nuber is given*/
for (i=0;i<80;i++) str[i] = ' ';
str[80] = '\0';
i = 0;
while ((c=getc(expfile))!='\n' && c!=EOF) str[i++] = c;
str[i] = '\0';
nform = sscanf(str,"%f %f %f",&x1,&x2,&x3);
if ((nform<2 || nform>3) || (sites>1 && nform !=3))
  nrerror("Error: Wrong format in file of experimental ssb intensities");
if (nform==2) { 
  i = x1<0.0 ? (int)(x1-0.01) : (int)(x1+0.01);
  sitectr = 1; dum = x2; 
  } else {
  i = x2<0.0 ? (int)(x2-0.01) : (int)(x2+0.01);
  sitectr = (int)(x1+0.01); dum = x3;
  }
if (abs(i)>=pft2) nrerror("Error: Assigned ssb-order too high for current csa or cq parameters");
x = (float)i*vrot + qcpar[viso[sitectr]];
if (x>sw || x<sp) nrerror("Error: Assigned ssb outside simulated spectralwidth");
if (dum<0.0) nrerror("Error: Can not iterate on negative ssbs");
if (ampmax < dum) ampmax = dum;
expint[sitectr][pft2-i] = dum;
ndata++;
if (nform == 2) {
  while (fscanf(expfile,"%d %f",&i,&dum) != EOF) { /* read ssb order and intensity*/
    if (abs(i)>=pft2) nrerror("Error: Assigned ssb-order too high for current csa or cq parameters");
    x = (float)i*vrot + qcpar[viso[sitectr]];
    if (x>sw || x<sp) nrerror("Error: Assigned ssb outside simulated spectralwidth");
    if (dum<0.0) nrerror("Error: Can not iterate on negative ssbs");
    if (ampmax < dum) ampmax = dum;
    expint[1][pft2-i] = dum;
    ndata++;
    }
  } else {
  while (fscanf(expfile,"%d %d %f",&sitectr,&i,&dum) != EOF) { /* read ssb order and intensity*/
    if (abs(i)>=pft2) nrerror("Error: Assigned ssb-order too high for current csa or cq parameters");
    x = (float)i*vrot + qcpar[viso[sitectr]];
    if (x>sw || x<sp) nrerror("Error: Assigned ssb outside simulated spectralwidth");
    if (dum<0.0) nrerror("Error: Can not iterate on negative ssbs");
    if (ampmax < dum) ampmax = dum;
    expint[sitectr][pft2-i] = dum;
    ndata++;
    }
  }
if (ampmax == 0.0) nrerror("Error: Experimental data consists of zeroes");
expint[sites][pft] = 0.0;
for (sitectr=1;sitectr<=sites;sitectr++) {
  for (i=0; i<pft; i++) {
    expint[sitectr][i] /= ampmax;  /* normalize (largest ssb set to 1) */
    expint[sites][pft] += expint[sitectr][i];  /* total experimental ssb intensity */
    }
  }
fclose(expfile);
}

void readexp_2(argv2,inpfile)
char argv2[];
FILE *inpfile;

/* get experimental datapoints */
{
int expfile;
float expsw,expsp,lbval,gfval,llim,rlim,ampmax;
int i,j,nregion,expfn;
int explp,offset,npoints,simlp,nread;
struct datafilehead fhd;
struct datablockhead bh;
int *iexpspec;


/* experimental sw, sp, fn*/
fscanf(inpfile,"%f %f %f %f %d",&expsw,&expsp,&lbval,&gfval,&expfn);
if ((expsw/expfn) != (sw/(float)fn)) 
     nrerror("Error: Digital resolution in simulation must match experimental data");
fscanf(inpfile,"%d",&nregion);  /* number of assigned regions*/
qcpar[lb] = lbval; /* lb to apply in the simulations*/
if (lb<npar) {
  if (qcpar[lb]<0.01*digres) nrerror("Error: Please set initial estimate for lb");
  qcinc[lb] = 0.3*lbval;
  }
qcpar[gf] = gfval; /* gf to apply in the simulations*/
if (gf<npar) {
  if (qcpar[gf] ==0.0 ) nrerror("Error: Please set initial estimate for gf");
  qcinc[gf] = 0.3*gfval;
  }

expfile = open(argv2,O_RDONLY);
if (!expfile) nrerror("Error: Can not open experiment phase file");
expspec = vector(0,fn);
for (i=0; i<fn; expspec[i++] = 0.0);
nread = read(expfile,&fhd,sizeof(struct datafilehead)); /*read file_header */
if (nread != sizeof(struct datafilehead)) nrerror("error reading datafilehead");
nread = read(expfile,&bh,sizeof(struct datablockhead)); /* block_header */
if (nread != sizeof(struct datablockhead)) nrerror("error reading datablockhead");
for (i=1,ndata=0; i<=nregion; i++) { /* loop through all regions*/
  fscanf(inpfile,"%f %f",&llim,&rlim);
  explp = floor((expsw+expsp-llim)*expfn/expsw+0.1); /* first exp. data point*/
  offset = sizeof(float)*explp;
  simlp = floor((sw+sp-llim)*fn/sw+0.1); /* corresponding point in simulated spec*/
  if (explp < 0 || simlp <0 || simlp>(fn-1)) 
           nrerror("Error: Region outside simulated spectral window");
  npoints = floor( (llim-rlim)*expfn/expsw+0.1);
  if ((simlp+npoints) >= fn)
        nrerror("Error: Region outside simulated spectral window");
  lseek(expfile,offset,SEEK_CUR);
  j = (read(expfile,&expspec[simlp],sizeof(float)*npoints)); /* read data*/
  if (j != (sizeof(float)*npoints)) nrerror("error while reading phasefile");
  lseek(expfile,-(offset+j),SEEK_CUR);
#ifdef LINUX
  iexpspec = (int *) &expspec[simlp];
  for (j=0; j<npoints; j++)
  {
     *iexpspec = ntohl( *iexpspec );
      iexpspec++;
  }
#endif
  ndata += npoints;
  } 
ampmax = 0.0;
for (i=0; i<fn; i++)
  if (ampmax<expspec[i]) ampmax = expspec[i];
if (ampmax==0.0) nrerror("Error: Experimental data consists of zeroes");
expspec[fn] = 0.0;
for (i=0; i<fn; i++) {
  expspec[i] /= ampmax;  /* normalize data*/
  expspec[fn] += expspec[i];  /* total experimental intensity*/
  }
close(expfile);
}

FILE* macfile;

void setup(argv3)
char argv3[];

{
float statwidth,statw[3],fi,mmax;
int i,j,index,nssb,pftdim,fndim,npdim;

if ((macfile=fopen(argv3,"w")) == NULL) nrerror("Error: Can not open macro file");

if (vrot<10.0) nrerror("Error: srate too small");

if (nt < ntmin) {
  nt = ntmin;
  fprintf(macfile,"ant=%d\n",nt);
  }
if (nt > ntmax) {
  nt = ntmax;
  fprintf(macfile,"ant=%d\n",nt);
  }
/* convert Hz to rad/sec and usec to sec */
for (i=1;i<=sites;i++) {
  if (cq[i]<totalpar) {
    if (qcpar[cq[i]]>1.0e5) printf("warning: cq in MHz ! ");
    qcpar[cq[i]] *= 2.0*pi*1.0e6;
    if (cq[i]<npar) qcinc[cq[i]] *= 2.0*pi*1.0e6;
   }
  if (csa[i]<totalpar) {
    qcpar[csa[i]] *= 1.0e-6;   /* remember ppm */
    if (csa[i]<npar) qcinc[csa[i]] *= 1.0e-6;
    }
  }
sfrqr = 2.0*pi*1.0e6*sfrq;
pwt = pw*1.0e-6;
vrotr = 2.0*pi*vrot;
gb1r = -gb1*2.0*pi;

/* do not iterate on all amp parameters */
for (j=1,i=0;j<=sites;j++) {
  if (amp[j]<npar) i++ ;}
if (i == sites) nrerror("Error: Do not iterate on all amp-parameters");

/* Round fn (number of complex points) to nearest higher power of 2 */
fndim = ceil(log((float)fn)/log(2.0));
fn = pow(2.0,(float)fndim);
if (fn > fnmax) {
  if (npar>0 && progorder==2) /* disaster for an iteration with lineshape*/ 
    nrerror("Error: Maximum fn value is 65536");
  else fn = fnmax;
  }
if (fn < fnmin) fn = fnmin;
fprintf(macfile," fn=%d\n",2*fn); /* output revised value for fn */
/* Round np (number of complex points) to nearest higher power of 2 */
npdim = ceil(log((float)np)/log(2.0));
np = pow(2.0,(float)npdim);
if (np > npmax) np = npmax;
if (np < npmin) np = npmin;
fprintf(macfile," np=%d\n",2*np); /* output revised value for np */
digres = sw/fn;
/* Check ival: Multiplum of 0.5 and in the range 1/2 to 11/2 */
if (fabs(2.0*ival-(int)(2.0*ival+0.001))>1.0e-3 || ival<0.5) nrerror("Error: Illegal value for ival");
if (ival>5.5) nrerror("Error: Not implemented for ival>11/2");

/* Check mval */
for (i=1 , cflag=FALSE , mmax=0; i<=ntrans; i++) {
  if (mval[i]>mmax) mmax=mval[i];  /* find maximum m-value */
  if (fabs((int)(ival-mval[i]+0.001) - (ival-mval[i])) > 0.001) {
    printf("\nError: Illegal m-value:%f\n",mval[i]);  /* check for proper m-values */
    exit(-1);
    }
  if (fabs(mval[i]-0.5) < 0.01) {
    cflag = TRUE;   /* is the central transition required? */
    index = i;
    }
  }
if (cflag) {
    if (((int)(2.0*ival+0.01))== 2) {
    printf("\nError: No central transition for ival=%f\n",ival);
    exit(-1);
    }
  for (j=index; j<ntrans; j++) mval[j] = mval[j+1]; 
  mval[ntrans] = 0.5;
  sflag = ((ntrans-1) != 0);
  } else sflag = (ntrans != 0);

/* use the width of the static spectrum to estimate the number of sidebands
to calculate in order to avoid aliasing */

statwidth = 0.0;
for (i=1;i<=sites;i++) {
  statw[i] = 0.0;
  if (csa[i]<totalpar) {
    if (fabs(qcpar[etas[i]]) < 1.0) statw[i] = 2.0*fabs(qcpar[csa[i]])*sfrqr;
      else statw[i] = fabs(qcpar[csa[i]])*(1.0+fabs(qcpar[etas[i]]))*sfrqr;
    }
  if (cq[i]<totalpar) {
    if (mmax<1.0) statw[i] += 2.0*9.0/144.0*(25.0+22.0*qcpar[etaq[i]]+qcpar[etaq[i]]*qcpar[etaq[i]])*(ival*(ival+1.0)-0.75)*pow(qcpar[cq[i]]/(2.0*ival*(2.0*ival-1)),2.0)/sfrqr;
      else statw[i] += fabs(3.0*qcpar[cq[i]]*(mmax-0.5))/(ival*(2.0*ival-1.0));
    }
  if (statw[i]>statwidth) statwidth = statw[i];
  }

nssb = ceil(1.2*statwidth/vrotr+9.0); /* allow a few extra sidebands */
/*pft is the number of sidebands we calculate to avoid aliasing */ 
/* round to nearest higher power of 2 */
pftdim = ceil(log((float)nssb)/log(2.0));
pft = pow(2.0,(float)pftdim);
if (pft > pftmax) pft = pftmax;
if (pft < pftmin) pft = pftmin;
if (pft<nssb) printf("warning: ssb's may be aliased ");
pft2 = pft/2;
/* number of integration steps in procedure getrho */
pulsestep = ceil(fabs(vrot*pwt*sqrt(statwidth/(4.0*pi))));
if (pulsestep<1) pulsestep = 1;

/*transition probalities */
for (i=1; i<=ntrans; i++) tran[i] = sqrt(ival*(ival+1.0)-mval[i]*(mval[i]-1.0));
/* For ideal pulses we need the squared transition probability */
if (strncmp(pulse,"ideal",5)==0)
  for (i=1;i<=ntrans;i++) tran[i] *= tran[i];

/* calculate trigonometric functions 
arrays cc and ss are used (interpolated) later on to avoid a lot of time-
consuming cos/sin calculations */
cc = vector(0,3600);
ss = vector(0,3600);
for (i=0; i<=3600; i++) {
  fi = i*rad;
  cc[i] = cos(fi);
  ss[i] = sin(fi);
  }

cg = vector(0,pft);
sg = vector(0,pft);
c2g = vector(0,pft);
s2g = vector(0,pft);
cfi = vector(0,pft);
sfi = vector(0,pft);
c2fi = vector(0,pft);
s2fi = vector(0,pft);
for (i=0; i<=pft; i++) {
  fi = (float)(i)*2.0*pi/(float)(pft);
  cg[i] = cos(fi);
  sg[i] = sin(fi);
  cfi[i] = (cg[i]-1.0)/vrotr;
  sfi[i] = sg[i]/vrotr;
  fi *= 2.0;
  c2g[i] = cos(fi);
  s2g[i] = sin(fi);
  c2fi[i] = (c2g[i]-1.0)/(2.0*vrotr);
  s2fi[i] = s2g[i]/(2.0*vrotr);
  }
if (progorder == 2) {
  c3fi = vector(0,pft);
  s3fi = vector(0,pft);
  c4fi = vector(0,pft);
  s4fi = vector(0,pft);
  for (i=0; i<=pft; i++) {
    fi = (float)(i)*2.0*pi/(float)(pft);
    c3fi[i] = (cos(3.0*fi)-1.0)/(3.0*vrotr);
    s3fi[i] = sin(3.0*fi)/(3.0*vrotr);
    c4fi[i] = (cos(4.0*fi)-1.0)/(4.0*vrotr);
    s4fi[i] = sin(4.0*fi)/(4.0*vrotr);
    }
  }
}


int checksite(cptr,index)
char *cptr;
int index;
{
int j;
if (cptr[index] == ' ') return 1;
j = cptr[index] - '0';
if (j<=sites) return j;
printf("\nError: Illegal sitenumber in iteration parameter %s\n",cptr); fflush(stdout);
exit(-1);
}


void readinput(argv1, argv2, argv3)
char argv1[],argv2[],argv3[];
{
FILE *inpfile;
int i,l,j;
float *cqval,*etaqval,*csaval,*etasval,rfwval,thetaval;
float *psival,*chival,*xival,*visoval,*ampval;
char c,parname[8];

if ((inpfile = fopen(argv1,"r")) == NULL) nrerror("Error: Can not open input data file");
fscanf(inpfile,"%d %f %d %d %s",&sites,&ival,&ntrans,&progorder,spectype);

if (sites>sitesmax || sites<1) nrerror("Error: Illegal value for 'sites'");
if (progorder<1 || progorder>2) nrerror("Error: Illegal value for 'progorder'");
if (!strcmp(spectype,"quad")) {
  quadflag = TRUE; csaflag = FALSE; qcsaflag = FALSE; }
  else if (!strcmp(spectype,"qcsa")) {
  quadflag = FALSE; csaflag = FALSE; qcsaflag = TRUE; }
  else if (!strcmp(spectype,"csa")) {
  quadflag = FALSE; csaflag = TRUE; qcsaflag = FALSE; }
  else nrerror("Error: Illegal value for 'spectype'");
cq = intvector(1,sites);
etaq = intvector(1,sites);
for (j=1;j<=sites;j++) {
  cq[j] = parmax+1;
  etaq[j] = parmax+1;}

csa = intvector(1,sites);
etas = intvector(1,sites);
for (j=1;j<=sites;j++) {
  csa[j] = parmax+1;
  etas[j] = parmax+1;}

psi = intvector(1,sites);
chi = intvector(1,sites);
xi = intvector(1,sites);
for (j=1;j<=sites;j++) {
  psi[j] = parmax+1;
  chi[j] = parmax+1;
  xi[j] = parmax+1;}

viso = intvector(1,sites);
amp = intvector(1,sites);
for (j=1;j<=sites;j++) {
  viso[j] = parmax+1;
  amp[j] = parmax+1; }

cqval = vector(1,sites);
etaqval = vector(1,sites);
csaval = vector(1,sites);
etasval = vector(1,sites);
psival = vector(1,sites);
chival = vector(1,sites);
xival = vector(1,sites);
visoval = vector(1,sites);
ampval = vector(1,sites);

for (i=1; i<=ntrans; i++)  
  fscanf(inpfile,"%f",&mval[i]);
for (j=1;j<=sites;j++) {
  if (quadflag || qcsaflag) 
    fscanf(inpfile,"%f %f",&cqval[j],&etaqval[j]);
  if (csaflag || qcsaflag) 
    fscanf(inpfile,"%f %f",&csaval[j],&etasval[j]); 
  if (qcsaflag) 
    fscanf(inpfile,"%f %f %f",&psival[j],&chival[j],&xival[j]);
  fscanf(inpfile,"%f %f",&ampval[j],&visoval[j]);
  }
fscanf(inpfile,"%f %f",&vrot,&thetaval);
fscanf(inpfile,"%f %f %f %6s",&gb1,&pw,&rfwval,pulse);

if (rfwval>1.0e5) printf("warning: rfw in MHz! ");

if (strncmp(pulse,"ideal",5) != 0 && strncmp(pulse,"short",5) != 0 && strncmp(pulse,"finit",5) != 0 ) 
  nrerror("Error: Illegal value for 'pulse'");

fscanf(inpfile,"%f %f %f ",&sw,&sp,&sfrq);
fscanf(inpfile,"%d %d %d %d",&nt,&np,&fn,&maxiter);

/* read and check iteration parameters */
npar = 0;
rmsflag = FALSE;
l = 0;
c=getc(inpfile);
do {
  i=0;
  while((c=getc(inpfile)) != EOF && c != '\n' && c != ',') 
    parname[i++]=c;
  if (c==EOF && i == 0) break;
  parname[i++] = ' '; 
  parname[i]='\0';
 if (c==EOF) nrerror("Error: Error in input");
  if (strncmp(parname,"rms",3)==0) {
    rmsflag = TRUE;
    break;
    }
  if (strncmp(parname,"amp",3)==0) {
    j = checksite(parname,3);
    amp[j] = npar++;
    qcpar[amp[j]] = ampval[j];
    qcinc[amp[j]] = 0.2*ampval[j];
    }

/* quadrupole parameters */
  if (quadflag || qcsaflag) {
   
    if (strncmp(parname,"cq",2)==0) {
      j = checksite(parname,2);
      cq[j] = npar++;
      qcpar[cq[j]] = cqval[j];
      if (qcpar[cq[j]] == 0.0) nrerror("Error: Please set non-zero estimate for cq");
      qcinc[cq[j]] = -0.2*cqval[j];
      } else  if (strncmp(parname,"etaq",4)==0) {
      j = checksite(parname,4);
      etaq[j] = npar++;
      qcpar[etaq[j]] = etaqval[j];
      if (qcpar[etaq[j]]>0.7) 
        qcinc[etaq[j]] = -0.2;
        else 
        qcinc[etaq[j]] = 0.2;
      } 
    }

/* csa parameters */
  if (csaflag || qcsaflag) {

    if (strncmp(parname,"csa",3)==0)  {
      j = checksite(parname,3);
      csa[j] = npar++;
      qcpar[csa[j]] = csaval[j];
      if (qcpar[csa[j]] == 0.0) nrerror("Error: Please set non-zero estimate for csa");
      qcinc[csa[j]] = -0.3*csaval[j];
      } else  if (strncmp(parname,"etas",4)==0)  {
      j = checksite(parname,4);
      etas[j] = npar++;
      qcpar[etas[j]] = etasval[j];
      if (qcpar[etas[j]]>0.7) 
        qcinc[etas[j]] = -0.2;
        else 
        qcinc[etas[j]] = 0.2;
      } 
    }

/* orientation parameters */
  if (qcsaflag) {
    if (strncmp(parname,"psi",3)==0) {
      j = checksite(parname,3);
      psi[j] = npar++;
      qcpar[psi[j]] = psival[j];
      qcinc[psi[j]] = 30.0;
      } else  if (strncmp(parname,"chi",3)==0) {
      j = checksite(parname,3);
      chi[j] = npar++;
      qcpar[chi[j]] = chival[j];
      qcinc[chi[j]] = 20.0;
      } else  if (strncmp(parname,"xi",2)==0) {
      j = checksite(parname,2);
      xi[j] = npar++;
      qcpar[xi[j]] = xival[j];
      qcinc[xi[j]] = 30.0;
      } 
    }

  if (strncmp(parname,"theta",5) == 0) {
    theta = npar++;
    qcpar[theta] = thetaval;
    qcinc[theta] = 0.05;
    } else if (strncmp(parname,"rfw",3) == 0) {
    rfw = npar++;
    qcpar[rfw] = rfwval;
    if (rfwval<0.1) nrerror("Error: Please set initial estimate for rfw>0.1");
    qcinc[rfw] = 0.3*rfwval;
    }
 
/* viso and linebroadening parameters */
  if (progorder==2) {
    if (strncmp(parname,"viso",4)==0) {
      j = checksite(parname,4);
      viso[j] = npar++;
      qcpar[viso[j]] = visoval[j];
      qcinc[viso[j]] = 5.0*sw/fn;
      } else if (strncmp(parname,"lb",2) == 0) lb = npar++;
      else if (strncmp(parname,"gf",2) == 0) gf = npar++;
    }
  if (l==npar) { /* check that npar was incremented in this scan */
    printf("\nError: Illegal iteration parameter %s\n",parname);
    fflush(stdout);
    exit(-1);
    } else l = npar; /* save current npar value */

  } while (c != '\n' && l<parmax);

/* non-iteration parameters */
l = npar;
for (j=1;j<=sites;j++) {
  if (amp[j]>parmax) { 
    amp[j] = l++;
    qcpar[amp[j]] = ampval[j];
    }
  if (quadflag || qcsaflag) {
    if (cq[j]>parmax) {
      cq[j] = l++;
      qcpar[cq[j]] = cqval[j];
      }
    if (etaq[j]>parmax) { 
      etaq[j] = l++;
      qcpar[etaq[j]] = etaqval[j];
      }
    }
  if (csaflag || qcsaflag) {
    if (csa[j]>parmax) { 
      csa[j] = l++;
      qcpar[csa[j]] = csaval[j];
      }
    if (etas[j]>parmax) { 
      etas[j] = l++;
      qcpar[etas[j]] = etasval[j];
      }
    }
  if (qcsaflag) {
    if (psi[j]>parmax) { 
      psi[j] = l++;
      qcpar[psi[j]] = psival[j];
      }
    if (chi[j]>parmax) { 
      chi[j] = l++;
      qcpar[chi[j]] = chival[j];
      }
    if (xi[j]>parmax) { 
      xi[j] = l++;
      qcpar[xi[j]] = xival[j];
      }
    }
  if (viso[j]>parmax) { 
    viso[j] = l++;
    qcpar[viso[j]] = visoval[j];
    }
  }
if (theta > parmax) {
  theta = l++;
  qcpar[theta] = thetaval;}
if (rfw > parmax) {
  rfw = l++;
  qcpar[rfw] = rfwval;}
if (progorder == 2 && (npar>0 || rmsflag) && lb > parmax) lb = l++;
if (progorder == 2 && (npar>0 || rmsflag) && gf > parmax) gf = l++;
totalpar = l;
setup(argv3);
if ((npar > 0) || rmsflag) {
  if (progorder == 1) readexp_1(argv2);
    else readexp_2(argv2,inpfile);
  }
fclose(inpfile);
free_vector(cqval,1,sites);
free_vector(etaqval,1,sites);
free_vector(csaval,1,sites);
free_vector(etasval,1,sites);
free_vector(psival,1,sites);
free_vector(chival,1,sites);
free_vector(xival,1,sites);
free_vector(visoval,1,sites);
free_vector(ampval,1,sites);
}
