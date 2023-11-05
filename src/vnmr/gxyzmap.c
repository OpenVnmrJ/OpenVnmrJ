/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* gxyzmap.c (was calcq);  includes gxyzfit                                                     */

/* gxyzmap/gxyzmapo (temporary, old algorithm)                                                  */

/* usage : gxyzmap('filename')                                                                  */

/* Neatened up by Dan Iverson for VnmrJ compatibility 25 vii 03                                 */

/* Speed up FT slightly, but still v slow - better to buffer data  GAM 6ix02                    */
/* Display status GAM  5ix02                                                                    */
/* original before barcorrection                                                                */
/* Increase MAX to 256 GAM 4ix02                                                                */
/* Include sw from parameters GAM 4ix02                                                         */
/* Add display of gxyzcode;  make gxyzstr an array;  check length  4ix02 GAM                    */
/* Free data at end of programme   GAM 27ix02                                                   */
/* Replace matrix routines with NR versions, free all properly, use NR 3D tensors  GAM 30ix02   */
/* Ensure memory freed in the event of an abort because of missing data                         */
/* Add optional 2D sinebell weighting;  currently fixed on with weight2D    GAM 17x02           */
/* Add output of map parameters in <filename>.dat  GAM  11xi02                                  */
/* Correct error in sign of shim correction                                                     */
/* Use difference between 1st and second elements of tau, not just 2nd element                  */
/* include nrerror for 6.1B                                                                     */
/* New version uses separate 3D unwrapping algorithm, and removes correction for map distortion */
/* Correct sign of frequency offset                                                             */
/* Add splitting of shimmaps into individual maps   GAM  10i03                                  */
/* Suppress debugging output    GAM  13i03                                                      */
/* Output amplitude to file argv[1].amp                                                         */
/* Remove printing of Hzpp etc. GAM 30v03                                                       */
/* Increase 2nd dimension of shname as requested by PJB  GAM 11vi03                             */
/* Output amplitudes of all maps into shimamps and .amp file   GAM 13viii03                     */

/* gxyzfit                                                                                      */

/* usage : gxyzfit     or   gxyzfit(ncycles)                                                    */
/*         gxyz0fit    or   gxyz0fit(ncycles)                                                   */

/* convert float to double in gxyzfit                                                           */
/* use files in userdir                                                                         */
/* omit summary at end of xyshim.fit, for compatibility with disp3Dmap                          */
/* insist gzwin leaves 20 points clear                                          GAM 14i03       */
/* close datout as soon as finished                                             GAM 6ii03       */
/* make mapshims 256 long not MAXPATHL to allow large shim sets to be used      GAM 6ii03       */
/* add gxyz0fit alias GAM 17v03                                                                 */
/* fix rounding of shim change GAM 19vi03                                                       */

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include "vnmrsys.h"
#include "group.h"
#include "data.h"
#include "tools.h"
#include "variables.h"
#include "init2d.h"
#include "pvars.h"
#include "wjunk.h"
#include "shims.h"
#include "vfilesys.h"
#include "vnmr_svd.h"
#include "vnmr_fdist.h"

#define weight2D  1
#define debug  1
#define debug2  1
#define debug3  1
#define Pi 3.14159265358979323846 
#define twoPi 2.0*Pi
#define phtol Pi
#define MAX 256

#define SHIM_NAME_LEN 10

FILE *gxyzfitout;
FILE *gxyzfitpwr;
FILE *bug,*bug2,*bug3;
FILE *out,*outs,*ampout,*ampout2,*datout;
static int npxx,oldversion;
static dfilehead fidhead;
static char shimname[MAXPATH],mapshims[MAXPATH],mapname[MAXPATH],path[MAXPATH],ampname[MAXPATH];
static int Dres,ns,ncb;

static  double *v,*rbars;
static  double **cormp,**barcor;
static  double **data,**lstpnt,***corr,**lstpnt2,***corr2,**lstpnt3,***corr3;
static  double ***phd,***amp,***ci;
static  double ***sqrre,***sqrim,***sqrre2,***sqrim2;

static double sum1,sum2,rmsdiff,rmserr;
static double FixFlags[MAX_SHIMS+1];
static int npoints,nshims;
static FILE *mapfile,*listfile,*fitfile;
static double **shimmap;
static double *fy,*angle,*shoff,*shold,*shnew,*shdiff,*sherr,*y;
static char str[80],maptitle[80],dumch[80];
static char shname[MAX_SHIMS+1][SHIM_NAME_LEN];

//static double getChi2Gradient(double p, double p_opt, double deltaChi2);
static double getPwrForGradient(double dx2_dp, double p_opt, double deltaChi2);
static double getChi2ByPwr(double p, double Popt, double deltaChi2);
static double getTotalPwrForGradient(double dChiSqr_dP, double *chi2ByTerm,
                                        double *pOptByTerm, int m);
static double getTotalChi2ForGradient(double dChiSqr_dP, double *chi2ByTerm,
                                      double *pOptByTerm, int m);
static double getMinChi2ForPower(double *chi2ByTerm, double *pOptByTerm, int m,
                                 double p, double *dChiSqr_dP);

#ifdef GXYZFIT
//////////////////////  for gxyzfit standalone program   /////////////////////
char userdir[MAXPATH];
static int ShimSet;
#define Wscrprintf printf

/**
 *
 */
int main(int argc, char **argv)
{
    char *str;

    str = getenv("vnmruser");
    if (str) {
        strcpy(userdir, str);
    } else if ((str = getenv("HOME")) != 0) {
        sprintf(userdir, "%s/vnmrsys", str);
    } else {
        fprintf(stderr,"No home directory!\n");
        exit(-2);
    }
    //fprintf(stderr,"Calling gxyzfit\n");/*CMP*/
    gxyzfit(argc, argv, 0, 0);
    return 0;
}
//////////////////////  end gxyzfit standalone program section   /////////////
#endif // def GXYZFIT

/* ###### memory allocation functions ###### */

static void memerror(error_text)
     char error_text[];
{
    printf("Memory allocation error %s\n",error_text);
    exit(1);
}


static double ***d3tensor(long nrl, long nrh, long ncl, long nch, long ndl, long ndh)
/* allocate a double 3tensor with range t[nrl..nrh][ncl..nch][ndl..ndh] */
{
    long i,j,nrow=nrh-nrl+1,ncol=nch-ncl+1,ndep=ndh-ndl+1;
    double ***t;
    /* allocate pointers to pointers to rows */
    t=(double ***) malloc((size_t)((nrow+1)*sizeof(double**)));
    if (!t) memerror("allocation failure 1 in f3tensor()");
    t += 1;
    t -= nrl;
    /* allocate pointers to rows and set pointers to them */
    t[nrl]=(double **) malloc((size_t)((nrow*ncol+1)*sizeof(double*)));
    if (!t[nrl]) memerror("allocation failure 2 in f3tensor()");
    t[nrl] += 1;
    t[nrl] -= ncl;
    /* allocate rows and set pointers to them */
    t[nrl][ncl]=(double *) malloc((size_t)((nrow*ncol*ndep+1)*sizeof(double)));
    if (!t[nrl][ncl]) memerror("allocation failure 3 in f3tensor()");
    t[nrl][ncl] += 1;
    t[nrl][ncl] -= ndl;
    for(j=ncl+1;j<=nch;j++) t[nrl][j]=t[nrl][j-1]+ndep;
    for(i=nrl+1;i<=nrh;i++) {
        t[i]=t[i-1]+ncol;
        t[i][ncl]=t[i-1][ncl]+ncol*ndep;
        for(j=ncl+1;j<=nch;j++) t[i][j]=t[i][j-1]+ndep;
    }
    /* return pointer to array of pointers to rows */
    return t;
}


void free_d3tensor(double ***t, long nrl, long nrh, long ncl, long nch,
                   long ndl, long ndh)
/* free a double d3tensor allocated by d3tensor() */
{
    free((char *) (t[nrl][ncl]+ndl-1));
    free((char *) (t[nrl]+ncl-1));
    free((char *) (t+nrl-1));
}


static double **dmatrix(int nrl, int nrh, int ncl, int nch)
/* allocate a double matrix with subscript range m[nrl..nrh][ncl..nch] */
{
    int i, nrow=nrh-nrl+1,ncol=nch-ncl+1;
    double **m;
    /* allocate pointers to rows */
    m=(double **) malloc((unsigned)((nrow+1)*sizeof(double*)));
    if (!m) memerror("allocation failure 1 in matrix()");
    m += 1;
    m -= nrl;
    /* allocate rows and set pointers to them */
    m[nrl]=(double *) malloc((unsigned)((nrow*ncol+1)*sizeof(double)));
    if (!m[nrl]) memerror("allocation failure 2 in matrix()");
    m[nrl] += 1;
    m[nrl] -= ncl;
    for(i=nrl+1;i<=nrh;i++) m[i]=m[i-1]+ncol;
    /* return pointer to array of pointers to rows */
    return m;
}

void free_dmatrix(double **m, int nrl, int nrh, int ncl, int nch)
/* free a double matrix allocated by dmatrix() */
{
    free((char *) (m[nrl]+ncl-1));
    free((char *) (m+nrl-1));
}

double *dvector(int nl, int nh)
{
    double *v;
    v=(double *)malloc((unsigned) (nh-nl+1)*sizeof(double));
    if (!v) memerror("allocation failure in vector()");
    return v-nl;
}

void free_dvector(double *v, int nl, int nh)
{
    free((char*) (v+nl));
}

void freeall()
{
    fclose(out);
    fclose(ampout);
    fclose(ampout2);
    if (debug)  fclose(bug);
    if (debug2)  fclose(bug2);
    if (debug3)  fclose(bug3);
    free_d3tensor(sqrre,0,ncb,0,nshims+1,0,npxx/2-1);
    free_d3tensor(sqrre2,0,ncb,0,nshims+1,0,npxx/2-1);
    free_d3tensor(sqrim,0,ncb,0,nshims+1,0,npxx/2-1);
    free_d3tensor(sqrim2,0,ncb,0,nshims+1,0,npxx/2-1);
    free_d3tensor(phd,0,ncb,0,nshims+1,0,npxx/2-1);
    free_d3tensor(amp,0,ncb,0,nshims+1,0,npxx/2-1);
    free_d3tensor(ci,0,ncb,0,nshims+1,0,npxx/2-1);
    free_d3tensor(corr,0,ncb,0,nshims+1,0,npxx/2-1);
    free_d3tensor(corr2,0,ncb,0,nshims+1,0,npxx/2-1);
    free_d3tensor(corr3,0,ncb,0,nshims+1,0,npxx/2-1);
    free_dvector(v,0,npxx/2-1);
    free_dvector(rbars,1,ncb);
    free_dmatrix(data,0,ns-1,0,npxx-1);
    free_dmatrix(lstpnt,0,ncb,0,nshims+1);
    free_dmatrix(lstpnt2,0,ncb,0,nshims+1);
    free_dmatrix(lstpnt3,0,ncb,0,nshims+1);
    free_dmatrix(cormp,0,ncb,0,nshims+1);
    free_dmatrix(barcor,0,ncb,0,nshims+1);
}
/* ###### end of memory allocation functions ###### */

void vnmr_svd_covar(double **v, int ma, double w[], double **cvm)
{
    int k,j,i;
    double sum,*wti;
    wti=dvector(1,ma);
    for (i=1;i<=ma;i++) {
        wti[i]=0.0;
        if (w[i]) wti[i]=1.0/(w[i]*w[i]);
    }
    for (i=1;i<=ma;i++) {
        for (j=1;j<=i;j++) {
            for (sum=0.0,k=1;k<=ma;k++) sum += v[i][k]*v[j][k]*wti[k];
            cvm[j][i]=cvm[i][j]=sum;
        }
    }
    free_dvector(wti,1,ma);
}


#ifndef GXYZFIT
/* ###### main function starts here ###### */

int gxyzmap(argc,argv,retc,retv)
     int argc;
     int retc;
     char *argv[];
     char *retv[];
{
    char gxyzstr[MAX];
    char datoutname[MAX];
    char progress[8] = "XYFT    ";
    char digits[10] = "0123456789";
    int ii,jj,kk;
    int spc,i,j,k,q,r,c;
    int npt,nji,nki,tni,alt,ncbout,ncycles;
    int nfieldpts,shim,gzwin,cb;
    double zwrapcorr,sph,cph,qd,rd,jd,kd,tnid,sw,sw1,sw2,tau,tau1,tau2,Hzpp;
    double wt2D,phi,lsfrq1,lsfrq2,rcycles;
    double ftol,lastpnt,grwin,rni,arraydim,rfn,phase2,phase3,phase4;
    double alpha,beta,wt;
    dpointers  inblock;

    disp_status("IN      ");

    /* ###### check input arguments ###### */

    oldversion= 0;
    if (argv[0][7]=='o')
    {
        oldversion= 1;
    }

    if (argc != 2)
    {
        Werrprintf("wrong number of input arguments: argc only %d passed",argc);
        ABORT;
    }

    /* ###### read relevant parameters ###### */

    P_getreal(CURRENT,"fn",&rfn,1);
    P_getreal(CURRENT,"gzwin",&grwin,1);
    P_getreal(CURRENT,"arraydim",&arraydim,1);
    P_getreal(CURRENT,"ni",&rni,1);
    P_getreal(CURRENT,"lsfrq1",&lsfrq1,1);
    P_getreal(CURRENT,"lsfrq2",&lsfrq2,1);
    P_getreal(CURRENT,"sw",&sw,1);
    P_getreal(CURRENT,"sw1",&sw1,1);
    P_getreal(CURRENT,"sw2",&sw2,1);
    P_getreal(CURRENT,"tau",&tau1,1);
    P_getreal(CURRENT,"tau",&tau2,2);
    tau=tau2-tau1;
    if (  P_getreal(CURRENT,"cycles",&rcycles,1) ) rcycles=1.0;
    P_getstring(CURRENT,"gxyzcode",gxyzstr,1,MAX-1);
    P_getstring(CURRENT,"mapname",mapname,1,MAXPATH-1);
    P_getstring(CURRENT,"mapshims",mapshims,1,MAXPATH-1);
    if (((int) strlen(gxyzstr)) != ((int) (rni*rni)))
    {
        Werrprintf("gxyzcode length is %d, not %d;  command cancelled \n",(int) strlen(gxyzstr),(int) (rni*rni));
        ABORT;
    }
    /* ###### open output files ###### */


    if ((out = fopen(argv[1],"w")) == NULL)
    {
        Werrprintf("Cannot open shimmap output file");
        ABORT;
    }
    strcpy(ampname,argv[1]);
    strcat(ampname, ".amp");
    if ((ampout = fopen(ampname,"w")) == NULL)
    {
        Werrprintf("Cannot open shimmap amplitude output file");
        fclose(out);
        ABORT;
    }
    if ((ampout2 = fopen("shimamps","w")) == NULL)
    {
        Werrprintf("Cannot open shimmap output file");
        fclose(out);
        fclose(ampout);
        ABORT;
    }
    strcpy(datoutname, argv[1]);
    strcat(datoutname, ".dat");
    if ((datout = fopen(datoutname,"w")) == NULL)
    {
        Werrprintf("Cannot open shimmap output data file");
        fclose(out);
        fclose(ampout);
        fclose(ampout2);
        ABORT;
    }
    if (debug)  bug = fopen("debug","w");
    if (debug2) bug2 = fopen("debug2","w");
    if (debug3) bug3 = fopen("debug3","w");

    /* ###### calculate other variables ###### */

    nji = (int)rni;
    tni = nji;
    nki = nji;
    tnid = (double)tni;
    ncb = nji*nki;
    npxx = (int)rfn;
    nshims = (int)((arraydim/(2.0*rni*rni))-1);
    ncycles = (int)rcycles;
    Hzpp = sw/(npxx/2);
    wt=0.0;

    gzwin = (int)(0.005*npxx*(100.0 - grwin));
    if (gzwin%2) gzwin--;
    if (gzwin<10)
    {
        gzwin=10;
        Werrprintf("gzwin reduced to leave necessary margin for phase correction \n");
    }

    npt = (npxx - gzwin*2)/2;

    if (debug)
    {
        printf("Plan of z bars included in map \n");
        printf("\n");
        kk=0;
        for (ii=1;  ii<= nji;  ii++)
        {
            for (jj=1;  jj<= nji;  jj++)
            {
                printf("%c  ",gxyzstr[kk]);
                kk++;
            }
            printf("\n");
        }
        printf("\n");
    }

    if ( (ncycles < 1) || (ncycles > 10) ) {
        Werrprintf("cycles reset to 1");
        ncycles = 1;
    }


    /* ###### output map parameters to <filename>.dat ##### */

    fprintf(datout,"ni =  %d\n",nji);
    fprintf(datout,"gxyzcode =  %s\n",gxyzstr);
    fprintf(datout,"Hzpp =  %f\n",Hzpp);
    fprintf(datout,"npt =  %d\n",npt);
    fclose(datout);



    /* ###### set up vectors and matrices ###### */



    v = dvector(0,npxx/2-1);
    rbars = dvector(1,ncb);
    lstpnt = dmatrix(0,ncb,0,nshims+1);
    lstpnt2 = dmatrix(0,ncb,0,nshims+1);
    lstpnt3 = dmatrix(0,ncb,0,nshims+1);
    cormp = dmatrix(0,ncb,0,nshims+1);
    barcor = dmatrix(0,ncb,0,nshims+1);

    sqrre=d3tensor(0,ncb,0,nshims+1,0,npxx/2-1);
    sqrre2=d3tensor(0,ncb,0,nshims+1,0,npxx/2-1);
    sqrim=d3tensor(0,ncb,0,nshims+1,0,npxx/2-1);
    sqrim2=d3tensor(0,ncb,0,nshims+1,0,npxx/2-1);
    phd=d3tensor(0,ncb,0,nshims+1,0,npxx/2-1);
    amp=d3tensor(0,ncb,0,2*nshims+2,0,npxx/2-1);
    ci=d3tensor(0,ncb,0,nshims+1,0,npxx/2-1);
    corr=d3tensor(0,ncb,0,nshims+1,0,npxx/2-1);
    corr2=d3tensor(0,ncb,0,nshims+1,0,npxx/2-1);
    corr3=d3tensor(0,ncb,0,nshims+1,0,npxx/2-1);

    /* ###### initialise vectors and matrices ###### */

    for (i=0;i<npxx/2;i++)
        v[i] = sw - i*sw/(npxx/2 - 1) - rflrfp;

    for (cb=1;cb<=ncb;cb++){
        for (shim=1;shim<=nshims+1;shim++){
            lstpnt[cb][shim] = 0.0;
            lstpnt2[cb][shim] = 0.0;
            lstpnt3[cb][shim] = 0.0;
            cormp[cb][shim] = 0.0;
            barcor[cb][shim] = 0.0;
            for (i=0;i<npxx/2;i++){
                sqrre[cb][shim][i] = 0.0;
                sqrim[cb][shim][i] = 0.0;
                sqrre2[cb][shim][i] = 0.0;
                sqrim2[cb][shim][i] = 0.0;
                phd[cb][shim][i] = 0.0;
                amp[cb][shim][i] = 0.0;
                amp[cb][shim+nshims+1][i] = 0.0;
                ci[cb][shim][i] = 0.0;
                corr[cb][shim][i] = 0.0;
                corr2[cb][shim][i] = 0.0;
                corr3[cb][shim][i] = 0.0;
            } /* end of i loop */
        } /* end of shim loop */
    } /* end of cb loop */


    ncbout=0;
    for (cb=1;cb<=ncb;cb++){
        if (gxyzstr[cb-1] == '0')
            rbars[cb] = 0.0;
        else
        {
            rbars[cb] = 1.0;
            ncbout++;
        }
    }
    nfieldpts = ncbout*((npxx - gzwin*2)/2);


    /* Load buffer with spectral data */


    disp_status("        ");
    disp_status("READ    ");
    ns=nji*nki*2*(nshims+1);
    spc=0;
    data=dmatrix(0,ns-1,0,npxx-1);

    for (j=1;j<=nji;j++)
    {
        for (k=1;k<=nki;k++)
        {
            for (shim=1;shim<=(nshims+1);shim++)
            {
                for (alt=0;alt<=1;alt++)
                {

                    /* get spectrum */
                    if ( (Dres = D_gethead(D_DATAFILE, &fidhead)) )
                    {
                        if (Dres == D_NOTOPEN)
                        {
                            Wscrprintf("Original Spectrum had to be re-opened \n");
                            strcpy(path, curexpdir);
                            strcat(path, "/datdir/data");
                            Dres = D_open(D_DATAFILE, path, &fidhead); /* open the file */
                        }

                        if (Dres)
                        {
                            D_error(Dres);
                            Werrprintf("gxyzmap : spectrum %d not found",spc);
                            freeall();
                            ABORT;
                        }
                    }

                    if ( (Dres = D_getbuf(D_DATAFILE, fidhead.nblocks, spc, &inblock)) )
                    {
                        D_error(Dres);
                        freeall();
                        ABORT;
                    }

                    if ( (Dres = D_release(D_DATAFILE, spc)) )
                    {
                        D_error(Dres);
                        freeall();
                        ABORT;
                    }
                    for (i=0;i<npxx;i+=1)
                    {
                        data[spc][i]=inblock.data[i];
                    }
                    spc++;
                } /* end of tau loop */
            } /* end of shims loop */
        } /* end of k loop */
    } /* end of j loop */

    /* ###### perform discrete 2Dft ###### */

    disp_status("        ");
    disp_status("XYFT    ");
    phi=0.0;
    cb=0;
    for (q=1;q<=nji;q++)
    {
        for (r=1;r<=nki;r++)
        {
            cb++;
            progress[5]=digits[q];
            progress[7]=digits[r];
            disp_status(progress);
            for (i=0;i<npxx;i+=2)
            {
                spc=0;
                for (j=1;j<=nji;j++)
                {
                    for (k=1;k<=nki;k++)
                    {
                        qd = (double)q;
                        rd = (double)r;
                        jd = (double)j;
                        kd = (double)k;
                        phi = 2.0*Pi*(((jd-1.0)*(qd-1.0) + (kd-1.0)*(rd-1.0))/tnid);
                        phi += 2.0*Pi*((jd+kd)*(0.5 + (1.0/(2.0*tnid))) - jd*(lsfrq2/sw2) - kd*(lsfrq1/
                                                                                                sw1));
                        cph=cos(phi);
                        sph=sin(phi);
                        wt2D=1.0;
                        if (weight2D)
                        {
                            wt2D=cos((Pi/tnid)*(jd-0.5-tnid/2.0))*cos((Pi/tnid)*(kd-0.5-tnid/
                                                                                 2.0));
                        }
                        for (shim=1;shim<=(nshims+1);shim++)
                        {
                            for (alt=0;alt<=1;alt++)
                            {
                                if (alt == 0) { /* alt = 0 for 1st tau value */
                                    /* code folded from here */
                                    sqrre[cb][shim][i/2] += data[spc][i]*cph*wt2D + data[spc][i+1]*
                                            sph*wt2D;
                                    sqrim[cb][shim][i/2] += data[spc][i+1]*cph*wt2D - data[spc][i]*sph*wt2D;
                                    /* unfolding */
                                }

                                if (alt == 1) { /* alt = 1 for 2nd tau value */
                                    /* code folded from here */
                                    sqrre2[cb][shim][i/2] += data[spc][i]*cph*wt2D +
                                            data[spc][i+1]*sph*wt2D;
                                    sqrim2[cb][shim][i/2] += data[spc][i+1]*cph*wt2D - data[spc][i]*sph*wt2D;
                                    /* unfolding */
                                }

                                spc++;
                            } /* end of tau loop */
                        } /* end of shims loop */
                    } /* end of inner summation loop */
                } /* end of summation loops */

            } /* end of i loop */
        } /* end of r loop */
    } /* end of q loop */

    if (oldversion)
    {
        /* USE OLD ALGORITHM */

        /* ################### calculate phases ################### */
        /* use i for sqrre and sqrim for tau = 0                    */
        /* use j for sqrre2 and sqrim2 for tau = inc                */
        /* where j is the corrected index value, when taking into   */
        /* account the effect of the field inhomogeneity and the    */
        /* shim gradient increment on second profile                */

        disp_status("3Dmap   ");

        for (k=1;k<=ncycles;k++){ /* start of cycle loop for phase calculations */

            for (cb=1;cb<=ncb;cb++)
            {
                if (rbars[cb] > 0.0) {
                    for (i=(gzwin/2)-5;i<((npxx-gzwin)/2)+5;i++) /* calc phases +- 10 pts more than w
                                                                    indow */
                    {
                        for (shim=1;shim<=nshims+1;shim++)
                        {
                            if (shim == 1) { /* calculate field map only */

                                j = (int)(ci[cb][shim][i]);
                                if (ci[cb][shim][i] < 0.0)
                                {
                                    /* code folded from here */
                                    wt = -(ci[cb][shim][i] - j);
                                    q=1;
                                    /* unfolding */
                                }
                                else
                                {
                                    /* code folded from here */
                                    wt = ci[cb][shim][i] - j;
                                    q=-1;
                                    /* unfolding */
                                }

                                /* phase difference for first point */
                                alpha = sqrre[cb][shim][i-j]*sqrre2[cb][shim][i-j] + sqrim[cb][shim][i-j]*
                                        sqrim2[cb][shim][i-j];
                                beta = sqrim[cb][shim][i-j]*sqrre2[cb][shim][i-j] - sqrre[cb][shim][i-j]*
                                        sqrim2[cb][shim][i-j];
                                phase2 = -atan2(beta,alpha);   /* Correct sign:  phase(ta
                                                                  u) - phase(0) */

                                if (debug) fprintf(bug,"%d\t%f\t",i,phase2);

                                /* calculate unwrapping correction */
                                if (k == 1) {
                                    /* code folded from here */
                                    corr[cb][shim][i] = corr[cb][shim][i-1];
                                    if ((phase2 - lstpnt[cb][shim]) > phtol) corr[cb][shim][i] -= twoPi;
                                    if ((phase2 - lstpnt[cb][shim]) < -phtol) corr[cb][shim][i] += twoPi;
                                    lstpnt[cb][shim] = phase2;
                                    /* unfolding */
                                }
                                phase2 += corr[cb][shim][i-j];
                                if (debug) fprintf(bug,"%f\t%f\n",corr[cb][shim][i],phase2);

                                /* phase difference for second point */
                                alpha = sqrre[cb][shim][i-j+q]*sqrre2[cb][shim][i-j+q] +
                                        sqrim[cb][shim][i-j+q]*sqrim2[cb][shim][i-j+q];
                                beta = sqrim[cb][shim][i-j+q]*sqrre2[cb][shim][i-j+q] - sqrre[cb][shim][i-j+q]*
                                        sqrim2[cb][shim][i-j+q];
                                phase3 = -atan2(beta,alpha);   /* Correct sign:  phase(ta
                                                                  u) - phase(0) */

                                phase3 += corr[cb][shim][i-j+q];

                                /* calculate phase difference field - weighted average of
                                   shim phase differences */
                                phase4 = phase2*(1.0 - wt) + phase3*wt;
                                phd[cb][shim][i] = phase4;

                                amp[cb][1][i] = sqrt((sqrre[cb][1][i]*sqrre[cb][1][i] + sqrim[cb][1][i]*
                                                      sqrim[cb][1][i]));
                                amp[cb][2][i] = sqrt((sqrre2[cb][1][i]*sqrre2[cb][1][i] + sqrim2[cb][1][i]*
                                                      sqrim2[cb][1][i]));
                                if (i == (npxx/4 - 5)) cormp[cb][1] = corr[cb][1][i]; /*
                                                                                        store corr at midpoint */

                            }
                            else
                            { /* calculate shimmaps */

                                j = (int)(ci[cb][shim][i]);
                                if (ci[cb][shim][i] < 0.0)
                                {
                                    /* code folded from here */
                                    wt = -(ci[cb][shim][i] - j);
                                    q=1;
                                    /* unfolding */
                                }
                                else
                                {
                                    /* code folded from here */
                                    wt = ci[cb][shim][i] - j;
                                    q=-1;
                                    /* unfolding */
                                }

                                /* phase difference for first point */
                                alpha = sqrre[cb][shim][i-j]*sqrre2[cb][shim][i-j] + sqrim[cb][shim][i-j]*
                                        sqrim2[cb][shim][i-j];
                                beta = sqrim[cb][shim][i-j]*sqrre2[cb][shim][i-j] - sqrre[cb][shim][i-j]*
                                        sqrim2[cb][shim][i-j];
                                phase2 = -atan2(beta,alpha);   /* Correct sign:  phase(ta
                                                                  u) - phase(0) */

                                if (debug3 && shim == 2) fprintf(bug3,"%d\t%f\t",i,phase2);

                                /* calculate unwrapping correction on first cycle */
                                if (k == 1) {
                                    /* code folded from here */
                                    corr2[cb][shim][i] = corr2[cb][shim][i-1];
                                    if ((phase2 - lstpnt2[cb][shim]) > phtol) corr2[cb][shim][i] -= twoPi;
                                    if ((phase2 - lstpnt2[cb][shim]) < -phtol) corr2[cb][shim][i] += twoPi;
                                    lstpnt2[cb][shim] = phase2;
                                    /* unfolding */
                                }
                                phase2 += corr2[cb][shim][i-j];
                                if (debug3 && shim == 2) fprintf(bug3,"%f\t%f\n",corr2[cb][shim][i],
                                                                 phase2);

                                /* phase difference for second point */
                                alpha = sqrre[cb][shim][i-j+q]*sqrre2[cb][shim][i-j+q] +
                                        sqrim[cb][shim][i-j+q]*sqrim2[cb][shim][i-j+q];
                                beta = sqrim[cb][shim][i-j+q]*sqrre2[cb][shim][i-j+q] - sqrre[cb][shim][i-j+q]*
                                        sqrim2[cb][shim][i-j+q];
                                phase3 = -atan2(beta,alpha);   /* Correct sign:  phase(ta
                                                                  u) - phase(0) */

                                phase3 += corr2[cb][shim][i-j+q];

                                /* calculate phase difference field - weighted average of
                                   shim phase differences */
                                phase4 = phase2*(1.0 - wt) + phase3*wt;
                                phd[cb][shim][i] = phase4; /* field error not subtracted
                                                              yet */

                                amp[cb][shim][i] = sqrt((sqrre[cb][shim][i]*sqrre[cb][shim][i]
                                                         + sqrim[cb][shim][i]*sqrim[cb][shim][i]));
                                amp[cb][shim+1+nshims][i] = sqrt((sqrre2[cb][1][i]*sqrre2[cb][1][i] + sqrim2[cb][1][i]*
                                                                  sqrim2[cb][1][i]));
                                if (i == (npxx/4 - 5)) cormp[cb][shim] = corr2[cb][shim][i];
                            }

                        } /* end of shim loop */
                    } /* end of i loop */
                }/* end of if rbars condition */
            } /* end of cb loop */

            /* ###### calculate phase index corrections, ci ###### */
            /* ###### bring phd maps into correct range, i.e. subtract cormp's ###### */
            /*        apply the same unwrapping correction at the midpoint of each bar       */

            r=0;
            for (shim=1;shim<=(nshims+1);shim++)
            {
                if (debug2) fprintf(bug2,"\n");
                for (cb=1;cb<=ncb;cb++)
                {
                    if (rbars[cb] > 0.0) {
                        for (i=(gzwin/2);i<((npxx-gzwin)/2);i++)
                        {
                            r++;
                            if (shim == 1)
                            {
                                phd[cb][shim][i] = (phd[cb][shim][i] - cormp[cb][shim])/(twoPi*
                                                                                         tau);
                                ci[cb][shim][i] = phd[cb][shim][i]/Hzpp;

                                if (i == (npxx/4 - 5)) {
                                    /* code folded from here */
                                    if (phd[cb][shim][i] > Pi/tau) barcor[cb][shim] = -1.0/tau;
                                    if (phd[cb][shim][i] < -Pi/tau) barcor[cb][shim] = 1.0/tau;
                                    /* unfolding */
                                }
                                if (debug2) fprintf(bug2,"%d\t%f\t%f\n",r,phd[cb][shim][i],
                                                    barcor[cb][shim]);
                            }
                            else
                            {
                                phd[cb][shim][i] = (phd[cb][shim][i] - cormp[cb][shim])/tau;
                                ci[cb][shim][i] = phd[cb][shim][i]/Hzpp; /* calculate cor
                                                                            rection before subtract fieldmap */
                                phd[cb][shim][i] = phd[cb][shim][i]  - phd[cb][1][i];

                                if (i == (npxx/4 - 5)) {
                                    /* code folded from here */
                                    if (phd[cb][shim][i] > Pi/tau) barcor[cb][shim] = -1.0/tau;
                                    if (phd[cb][shim][i] < -Pi/tau) barcor[cb][shim] = 1.0/tau;
                                    /* unfolding */
                                }
                            }

                        } /* end of i loop */
                    }/* end of if rbars condition */
                } /* end of cb loop */
            } /* end of shim loop */

        } /* end of cycles loop */
    }
    else
    {

        /* USE SIMPLIFIED ALGORITHM - STRIP OUT CORRECTIONS INITIALLY */

        /* ################### calculate phases ################### */

        disp_status("3Dmap   ");

        for (cb=1;cb<=ncb;cb++)
        {
            if (rbars[cb] > 0.0) {
                for (i=(gzwin/2)-5;i<((npxx-gzwin)/2)+5;i++) /* calc phases +- 10 pts more than window */
                {
                    for (shim=1;shim<=nshims+1;shim++)
                    {
                        if (shim == 1) { /* calculate field map only */

                            /* phase difference */
                            alpha = sqrre[cb][shim][i]*sqrre2[cb][shim][i] + sqrim[cb][shim][i]*
                                    sqrim2[cb][shim][i];
                            beta = sqrim[cb][shim][i]*sqrre2[cb][shim][i] - sqrre[cb][shim][i]*
                                    sqrim2[cb][shim][i];
                            phd[cb][shim][i] = -atan2(beta,alpha)/(twoPi*tau);  /* Correct si
                                                                                   gn:  phase(tau) - phase(0) */


                            amp[cb][1][i] = sqrt((sqrre[cb][1][i]*sqrre[cb][1][i] + sqrim[cb][1][i]*
                                                  sqrim[cb][1][i]));
                            amp[cb][2][i] = sqrt((sqrre2[cb][1][i]*sqrre2[cb][1][i] + sqrim2[cb][1][i]*
                                                  sqrim2[cb][1][i]));

                        }
                        else
                        { /* calculate shimmaps */

                            /* phase difference */
                            alpha = sqrre[cb][shim][i]*sqrre2[cb][shim][i] + sqrim[cb][shim][i]*
                                    sqrim2[cb][shim][i];
                            beta = sqrim[cb][shim][i]*sqrre2[cb][shim][i] - sqrre[cb][shim][i]*
                                    sqrim2[cb][shim][i];
                            phd[cb][shim][i] = -atan2(beta,alpha)/(twoPi*tau);  /* Correct si
                                                                                   gn:  phase(tau) - phase(0) */


                            amp[cb][shim][i] = sqrt((sqrre[cb][shim][i]*sqrre[cb][shim][i] +
                                                     sqrim[cb][shim][i]*sqrim[cb][shim][i]));
                            amp[cb][shim+1+nshims][i] = sqrt((sqrre2[cb][1][i]*sqrre2[cb][1][i] + sqrim2[cb][1][i]*
                                                              sqrim2[cb][1][i]));
                        }

                    } /* end of shim loop */
                } /* end of i loop */
            }/* end of if rbars condition */
        } /* end of cb loop */

        /* z unwrapping:  for each bar, work outwards from the middle correcting any jumps > pi radians */

        ftol=0.5/tau;

        for (cb=1;cb<=ncb;cb++)
        {
            if (rbars[cb] > 0.0)
            {
                for (shim=1;shim<=nshims+1;shim++)
                {
                    zwrapcorr=0.0;
                    lastpnt=phd[cb][shim][npxx/4];
                    for (i=1+(npxx/4);i<((npxx-gzwin)/2)+5;i++)
                    {
                        if ((phd[cb][shim][i]-lastpnt) > ftol) zwrapcorr -= 1.0/tau;
                        if ((phd[cb][shim][i]-lastpnt) < -ftol) zwrapcorr += 1.0/tau;
                        if (debug) fprintf(bug,"%d\t%f\t%f\t%f\n",i,lastpnt,phd[cb][shim][i],zwrapcorr);
                        lastpnt=phd[cb][shim][i];
                        phd[cb][shim][i]=phd[cb][shim][i]+zwrapcorr;
                    } /* end of i loop */
                    zwrapcorr=0.0;
                    lastpnt=phd[cb][shim][npxx/4];
                    for (i=(npxx/4)-1;i>=((gzwin/2)-5);i--)
                    {
                        if ((phd[cb][shim][i]-lastpnt) > ftol) zwrapcorr -= 1.0/tau;
                        if ((phd[cb][shim][i]-lastpnt) < -ftol) zwrapcorr += 1.0/tau;
                        if (debug) fprintf(bug,"%d\t%f\t%f\t%f\n",i,lastpnt,phd[cb][shim][i],zwrapcorr);
                        lastpnt=phd[cb][shim][i];
                        phd[cb][shim][i]=phd[cb][shim][i]+zwrapcorr;
                    } /* end of i loop */
                } /* end of shim loop */
            }/* end of if rbars condition */
        } /* end of cb loop */

        /* xy unwrapping:  for each bar, work outwards from the middle correcting any jumps > pi radians        */
        /* Assume middle of middle bar is within +- ftol, first work left and right correcting any sudden steps */
        /* then work up and down from each of the corrected bars to correct the rest of the map                 */

        for (shim=1;shim<=nshims+1;shim++) /* Correct middle line of bars */
        {
            j=nji/2;
            i=nji/2;
            cb=(i-1)+(j-1)*nji+1;
            lastpnt=phd[cb][shim][npxx/4];
            if (debug) fprintf(bug,"%d\t%f\t%f\n",cb,lastpnt,barcor[cb][shim]);

            for (i=(nji/2+1);i<=nji;i++)
            {
                cb=(i-1)+(j-1)*nji+1;
                if (rbars[cb] > 0.0)
                {
                    if ((phd[cb][shim][npxx/4]-lastpnt) > ftol) barcor[cb][shim] = -1.0/tau;
                    if ((phd[cb][shim][npxx/4]-lastpnt) < -ftol) barcor[cb][shim] = +1.0/tau;
                    if (debug) fprintf(bug,"%d\t%f\t%f\n",cb,(phd[cb][shim][npxx/4]-lastpnt),barcor[cb][shim]);
                    lastpnt=phd[cb][shim][npxx/4]+barcor[cb][shim];
                }
            } /* end of middle x bar right sweep */

            j=nji/2;
            i=nji/2;
            cb=(i-1)+(j-1)*nji+1;
            lastpnt=phd[cb][shim][npxx/4];
            if (debug) fprintf(bug,"%d\t%f\t%f\n",cb,lastpnt,barcor[cb][shim]);
            for (i=(nji/2-1);i>0;i--)
            {
                cb=(i-1)+(j-1)*nji+1;
                if (rbars[cb] > 0.0)
                {
                    if ((phd[cb][shim][npxx/4]-lastpnt) > ftol) barcor[cb][shim] = -1.0/tau;
                    if ((phd[cb][shim][npxx/4]-lastpnt) < -ftol) barcor[cb][shim] = +1.0/tau;
                    if (debug) fprintf(bug,"%d\t%f\t%f\n",cb,(phd[cb][shim][npxx/4]-lastpnt),barcor[cb][shim]);
                    lastpnt=phd[cb][shim][npxx/4]+barcor[cb][shim];
                }
            } /* end of middle x bar left sweep */

            for (i=nji/2;i<=nji;i++) /* Now correct each vertical line of bars */
            {
                j=nji/2;
                cb=(i-1)+(j-1)*nji+1;
                lastpnt=phd[cb][shim][npxx/4]+barcor[cb][shim];
                if (debug) fprintf(bug,"%d\t%f\t%f\n",cb,lastpnt,barcor[cb][shim]);
                for (j=(nji/2-1);j>0;j--)
                {
                    cb=(i-1)+(j-1)*nji+1;
                    if (rbars[cb] > 0.0)
                    {
                        if ((phd[cb][shim][npxx/4]-lastpnt) > ftol) barcor[cb][shim] = -1.0/tau;
                        if ((phd[cb][shim][npxx/4]-lastpnt) < -ftol) barcor[cb][shim] = +1.0/tau;
                        if (debug) fprintf(bug,"%d\t%f\t%f\n",cb,(phd[cb][shim][npxx/4]-lastpnt),
                                           barcor[cb][shim]);
                        lastpnt=phd[cb][shim][npxx/4]+barcor[cb][shim];
                    }
                } /* end of downward sweep */
                j=nji/2;
                cb=(i-1)+(j-1)*nji+1;
                lastpnt=phd[cb][shim][npxx/4]+barcor[cb][shim];
                if (debug) fprintf(bug,"%d\t%f\t%f\n",cb,lastpnt,barcor[cb][shim]);
                for (j=(nji/2+1);j<=nji;j++)
                {
                    cb=(i-1)+(j-1)*nji+1;
                    if (rbars[cb] > 0.0)
                    {
                        if ((phd[cb][shim][npxx/4]-lastpnt) > ftol) barcor[cb][shim] = -1.0/tau;
                        if ((phd[cb][shim][npxx/4]-lastpnt) < -ftol) barcor[cb][shim] = +1.0/tau;
                        if (debug) fprintf(bug,"%d\t%f\t%f\n",cb,(phd[cb][shim][npxx/4]-lastpnt),
                                           barcor[cb][shim]);
                        lastpnt=phd[cb][shim][npxx/4]+barcor[cb][shim];
                    }
                } /* end of upward sweep */
            } /* end of sweep right */

            for (i=nji/2-1;i>0;i--)
            {
                j=nji/2;
                cb=(i-1)+(j-1)*nji+1;
                lastpnt=phd[cb][shim][npxx/4]+barcor[cb][shim];
                if (debug) fprintf(bug,"%d\t%f\t%f\n",cb,lastpnt,barcor[cb][shim]);
                for (j=(nji/2-1);j>0;j--)
                {
                    cb=(i-1)+(j-1)*nji+1;
                    if (rbars[cb] > 0.0)
                    {
                        if ((phd[cb][shim][npxx/4]-lastpnt) > ftol) barcor[cb][shim] = -1.0/tau;
                        if ((phd[cb][shim][npxx/4]-lastpnt) < -ftol) barcor[cb][shim] = +1.0/tau;
                        if (debug) fprintf(bug,"%d\t%f\t%f\n",cb,(phd[cb][shim][npxx/4]-lastpnt),
                                           barcor[cb][shim]);
                        lastpnt=phd[cb][shim][npxx/4]+barcor[cb][shim];
                    }
                } /* end of downward sweep */
                j=nji/2;
                cb=(i-1)+(j-1)*nji+1;
                lastpnt=phd[cb][shim][npxx/4]+barcor[cb][shim];
                if (debug) fprintf(bug,"%d\t%f\t%f\n",cb,lastpnt,barcor[cb][shim]);
                for (j=(nji/2+1);j<=nji;j++)
                {
                    cb=(i-1)+(j-1)*nji+1;
                    if (rbars[cb] > 0.0)
                    {
                        if ((phd[cb][shim][npxx/4]-lastpnt) > ftol) barcor[cb][shim] = -1.0/tau;
                        if ((phd[cb][shim][npxx/4]-lastpnt) < -ftol) barcor[cb][shim] = +1.0/tau;
                        if (debug) fprintf(bug,"%d\t%f\t%f\n",cb,(phd[cb][shim][npxx/4]-lastpnt),
                                           barcor[cb][shim]);
                        lastpnt=phd[cb][shim][npxx/4]+barcor[cb][shim];
                    }
                } /* end of upward sweep */
            } /* end of sweep left */

        } /* end of shim loop */

        /* Subtract unwrapped field map from unwrapped shim map, correcting if middle points differ by more than
           unfolded range */

        for (shim=2;shim<=nshims+1;shim++)
        {
            j=nji/2;
            i=nji/2;
            cb=(i-1)+(j-1)*nji+1;
            alpha=phd[cb][shim][(npxx/4)-1]+barcor[cb][shim] - phd[cb][1][(npxx/4)-1]-barcor[cb][1];
            j=(int) ((fabs(alpha/ftol)+1.0)/2.0);
            if (alpha<0.0) j=-j;
            beta=-j*2.0*ftol;  /* Correction for map centre difference being out of range */
            for (cb=1;cb<=ncb;cb++)
            {
                if (rbars[cb] > 0.0) {
                    for (i=(gzwin/2)-5;i<((npxx-gzwin)/2)+5;i++)
                    {
                        if (debug2) fprintf(bug2,"%d\t%d\t%d\t%f\t%f\t%f\n",shim,cb,i,phd[cb][shim][i],
                                            phd[cb][1][i],phd[cb][shim][i]-phd[cb][1][i]);
                        phd[cb][shim][i] = beta + phd[cb][shim][i]+barcor[cb][shim]  - phd[cb][1][i]-
                                barcor[cb][1];
                    } /* end of i loop */
                }/* end of if rbars condition */
            } /* end of cb loop */
        } /* end of shim loop */

    } /* end of choice between old and new algorithms */

    /* ###### Output results to shimmap file ###### */

    if (nshims == 0){ /* output field map */
        fprintf(out,"exp 4\n");
        fprintf(out,"  1  %d\n",nfieldpts);
        fprintf(out,"field map\n");
        fprintf(out,"\n1  0  0  0\n");

        c=0;
        for (cb=1;cb<=ncb;cb++)
        {
            if (rbars[cb] > 0.0)
            {
                for (i=(gzwin/2);i<((npxx-gzwin)/2);i++)
                {
                    c++;
                    fprintf(out,"%d\t%f\n",c,phd[cb][1][i] + barcor[cb][1]);
                }
            }
        }
    }
    else /* output shimmaps without field map */
    {
        fprintf(out,"exp 4\n");
        fprintf(out,"  %d  %d\n",nshims,nfieldpts);
        fprintf(out,"shim maps\n");

        c=0;
        for (shim=2;shim<=nshims+1;shim++)
        {
            fprintf(out,"\n1  0  0  0\n");
            for (cb=1;cb<=ncb;cb++){
                if (rbars[cb] > 0.0){
                    for (i=(gzwin/2);i<((npxx-gzwin)/2);i++){
                        c++;
                        if (oldversion)
                        {
                            fprintf(out,"%d\t%f\n",c,phd[cb][shim][i] + barcor[cb][shim]);
                        }
                        else
                        {
                            fprintf(out,"%d\t%f\n",c,phd[cb][shim][i]);
                        }
                    }
                }
            }
        }
        fprintf(ampout2,"exp 4\n");
        fprintf(ampout2,"  %d  %d\n",2*nshims+2,nfieldpts);
        fprintf(ampout2,"shimmap amplitude\n");

        c=0;
        for (shim=1;shim<=2*nshims+2;shim++)
        {
            fprintf(ampout2,"\n1  0  0  0\n");
            for (cb=1;cb<=ncb;cb++){
                if (rbars[cb] > 0.0){
                    for (i=(gzwin/2);i<((npxx-gzwin)/2);i++){
                        c++;
                        fprintf(ampout2,"%d\t%f\n",c,amp[cb][shim][i]);
                    }
                }
            }
        }
    }

    /* Output individual shimmaps */

    c=0;
    for (shim=2;shim<=nshims+1;shim++)
    {
        sprintf(path,"%s/3Dshimlib/shimmaps/%s/shimmap%d",userdir,mapname,shim-1);
        j=1;
        k=0;
        strcpy(shimname,"        ");
        for (i=0; i<strlen(mapshims); i++)
        {
            if (isspace(mapshims[i])) j++;
            if ((j==(shim-1))&&!(isspace(mapshims[i])))
            {
                shimname[k]=mapshims[i];
                k++;
            }
        }
        strcat(shimname,"\0");
        if ((outs = fopen(path,"w")) == NULL)
        {
            Werrprintf("Cannot open shimmap output file");
            freeall();
            disp_status("        ");
            ABORT;
        }
        disp_status(shimname);
        fprintf(outs,"exp 4\n");
        fprintf(outs,"  %d  %d\n",1,nfieldpts);
        fprintf(outs,"%s\n",shimname);
        fprintf(outs,"\n1  0  0  0\n");
        for (cb=1;cb<=ncb;cb++){
            if (rbars[cb] > 0.0){
                for (i=(gzwin/2);i<((npxx-gzwin)/2);i++){
                    c++;
                    if (oldversion)
                    {
                        fprintf(outs,"%d\t%f\n",c,phd[cb][shim][i] + barcor[cb][shim]);
                    }
                    else
                    {
                        fprintf(outs,"%d\t%f\n",c,phd[cb][shim][i]);
                    }
                }
            }
        }
        if (fclose(outs) == -1)
        {
            Werrprintf("Cannot close shimmap output file for %s\n",shimname);
            freeall();
            ABORT;
        }
        disp_status("        ");
    }




    /* output amplitude file */

    fprintf(ampout,"exp 4\n");
    fprintf(ampout,"  2  %d\n",nfieldpts);
    fprintf(ampout,"index\n");
    for (j=1;j<3;j++)
    {
        fprintf(ampout,"\n1  0  0  0\n");
        cb=0;
        c=0;
        for (q=1;q<=nji;q++){
            for (r=1;r<=nki;r++){
                cb++;
                if (rbars[cb] > 0.0){
                    for (i=(gzwin/2);i<((npxx-gzwin)/2);i++){
                        c++;
                        fprintf(ampout,"%d\t%f\n",c,amp[cb][j][i]);
                    }
                }
            }
        }
    }


    /* ###### final remarks ###### */

    freeall();
    disp_status("        ");

    RETURN;

}/* ############### end of gxyzmap main ############## */
#endif // ndef GXYZFIT


/*      derived from
**      3Dshimfit       Fitting program for 3D automated phase shimming
**                      October 1998 : PBChilvers
*/


void fshimmap(double x, double p[], int np)
{
    int i,j;
    i=(int) x;
    for (j=1;  j<=np; j++)
    {
        p[j]=shimmap[i][j];
        if (FixFlags[j]!=0) p[j]=0.0;
        /*      printf("%d\t%d\t%f\n",i,j,p[j]); */
    }
}

void writefitfile()
{
    int i;
    strcpy(path, userdir);
    strcat(path, "/3Dshimlib/data/xyshim.fit");
    if ((fitfile=fopen(path,"w"))==0)
    {
        printf("Cannot open file xyshim.fit \n\n");
    }
    fprintf(fitfile,"exp 4\n");
    fprintf(fitfile,"    2\t %d\n",npoints);
    fprintf(fitfile,"%s\n",maptitle);
    fprintf(fitfile," \n");
    fprintf(fitfile,"1 0 0 0\n");
    for (i=1; i<=npoints; i++)
    {
        fprintf(fitfile,"%f\t%f\n",angle[i],y[i]);
    }
    fprintf(fitfile," \n");
    fprintf(fitfile,"1 0 0 0\n");
    for (i=1; i<=npoints; i++)
    {
        fprintf(fitfile,"%f\t%f\n",angle[i],fy[i]);
    }
    /*
      fprintf(fitfile," \n");
      fprintf(fitfile," \n");
      fprintf(fitfile,"mapname %s\n",maptitle);
      fprintf(fitfile,"shimset 10  nshims\t%d   rmsdiff  %4.2f   rmsdiff/rmserr   %3.2f \n\n",nshims,rmsdiff,rmsdiff/r
      mserr);
      fprintf(fitfile,"shim\toffset\t\told\t\tnew\t\tdiff\t\terror\n");
      fprintf(fitfile,"-----------------------------------------------------------------------------\n");
      for (i=1;i<=nshims; i++)
      {
      fprintf(fitfile,"%s\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n",shname[i],(int) shoff[i], (int) shold[i], (int) shnew[i], (i
      nt) shdiff[i], (int) sherr[i]);
      }
      fprintf(fitfile,"-----------------------------------------------------------------------------\n");
    */
    fclose(fitfile);
}

int readlistfile()
{
    int i;
    strcpy(path, userdir);
    strcat(path, "/3Dshimlib/data/xyshim.list");
    if ((listfile=fopen(path,"r"))==0)
    {
        printf("Cannot open file xyshim.list \n\n");
        return(-1);
    }
    fscanf(listfile,"%s %s\n",dumch,maptitle);
    fprintf(gxyzfitout,"Map title: %s\n",maptitle);
    fgets(dumch,80,listfile);
    fgets(dumch,80,listfile);
    do {
        if (fscanf(listfile,"%s",str) == EOF) strcpy(str,"errflag");
    }   /* fscanf for string until data start */                while ((strncmp(str,"-",1)!=0)&&(strcmp(str,"errflag")!= 0));
    if (strcmp(str,"errflag")==0) {
        printf("error: invalid format for xyshim.list");
        fclose(listfile);
        return(-1);
    }
    for (i=1; i<=nshims; i++)
    {
        fscanf(listfile,"%s %lf %lf %lf %lf %lf\n",shname[i],&shoff[i],&shold[i],&shnew[i],&shdiff[i],&sherr[i]);
        fprintf(gxyzfitout,"%s\t%8.0f %8.0f %8.0f %8.0f %8.0f\n",shname[i],shoff[i],shold[i],shnew[i],shdiff[i],sherr[i]);
    }
    fclose(listfile);
    return(0);
}

static void writelistfile(int shimset)
{
    int i;
    strcpy(path, userdir);
    strcat(path, "/3Dshimlib/data/xyshim.list");
    if ((listfile=fopen(path,"w"))==0)
    {
        printf("Cannot open file xyshim.list \n\n");
        return;
    }
    fprintf(listfile,"mapname %s\n",maptitle);
    fprintf(listfile,"shimset %d  nshims\t%d   rmsdiff  %4.2f   rmsdiff/rmserr   %6.4f \n\n",
            shimset, nshims,rmsdiff,rmsdiff/rmserr);
    fprintf(listfile,"shim\toffset\t\told\t\tnew\t\tdiff\t\terror\n");
    fprintf(listfile,"-----------------------------------------------------------------------------\n");
    for (i=1;i<=nshims; i++)
    {
        fprintf(listfile,"%s\t%d\t\t%d\t\t%d\t\t%d\t\t%d\n",shname[i],(int) shoff[i], (int) shold[i], (int) shnew[i],
                (int) shdiff[i], (int) sherr[i]);
    }
    fprintf(listfile,"-----------------------------------------------------------------------------\n");
    fclose(listfile);
}

//double getFitChisq(double *a, double **basisFunctions, double *y, double *sigma, int nparams, int npoints)
//{
//    double sum2;
//    double ycalc;
//    int i;
//    int j;
//
//    sum2 = 0.0;
//    for (i=1; i <= npoints; i++)
//    {
//        ycalc = 0;
//        for (j=1; j<=nparams; j++)
//        {
//            ycalc += a[j] * basisFunctions[i][j];
//        }
//        sum2 += SQR((ycalc - y[i]) / sigma[i]);
//    }
//    return sum2;
//}


static int getNumberOfPositiveTerms(double *w, int n)
{
    int i;
    int rtn;

    for (i = 1, rtn = 0; i <= n; i++) {
        if (w[i] > 0) {
            rtn++;
        }
    }
    return rtn;
}

///**
// * Gets the smallest of the w[i] (i in range 1 to n) that is >0.
// * Returns 0 if there is no w[i] > 0.
// */
//static double getSmallestPositiveValue(double *w, int n)
//{
//    int i;
//    double minw = 0;
//
//    for (i = 1; i <= n; i++) {
//        if (w[i] > 0) {
//            if (minw > w[i] || minw == 0) {
//                minw = w[i];
//            }
//        }
//    }
//    return minw;
//}

static double getMaxValue(double *w, int n)
{
    int i;
    double maxw = 0;

    for (i = 1; i <= n; i++) {
        if (maxw < w[i]) {
            maxw = w[i];
        }
    }
    return maxw;
}


/**
 * Compares the chi-square statistics of 2 fits to see if the first is
 * a significantly worse fit than the second.  The first fit is
 * assumed to use a subset of the parameters used in the second fit.
 * @return The probability that the fit is worse (0 < p < 1).
 */
static double isFitWorse(double chisq1, int npars1, double chisq2, int npars2, int npoints)
{
    double fstat;
    double p;

    //fprintf(gxyzfitpwr,
    //        "  isFitWorse: chisq1=%.15g, npars1=%d, chisq2=%.15g, npars2=%d, npoints=%d\n",
    //        chisq1, npars1, chisq2, npars2, npoints);
    p = 0;
    if (chisq2 > 0) {
        fstat = ((chisq1 - chisq2) / (npars2 - npars1))
                / (chisq2 / (npoints - npars2));
/**
 * Get the probability that an F-statistic will, by chance, be smaller than a given value.
 * @param f The f-value; assumed to be drawn from an F-distribution.
 * @param n1 Degrees of freedom for numerator.
 * @param n2 Degrees of freedom for denominator.
 * @return The probability that a random f would be smaller than the given f.
 */
        p = f_distribution(fstat, (double) (npars2 - npars1), (double) (npoints - npars2));
    }
    return p;
}

static double getChisq(double *y, double *sig, int npoints, double *a, int nShims)
{
    double chisq = 0;
    double sum = 0;
    double tmp;
    double *afunc;
    int i;
    int j;

    afunc = dvector(1, npoints);
    chisq = 0;
    for (i = 1; i <= npoints; i++) {
        fshimmap((double)i, afunc, nShims);
        for (sum = 0, j = 1; j <= nShims; j++) {
            sum += a[j] * afunc[j];
        }
        tmp = (y[i] - sum) / sig[i];
        chisq += tmp * tmp;
    }
    free_dvector(afunc, 1, nShims);
    return chisq;
}

//static double calculatePower(double *a, int m, int shimset, char *shimtype)
//{
//    int i;
//    double power;
//    char shimsetPath[MAXPATH];
//    char line[81];
//    FILE *infd;
//
//    sprintf(shimsetPath, "/vnmr/gxyzshim/shimtab/shimset%d_%s", shimset, shimtype);
//    infd = fopen(shimsetPath, "r");
//    if (infd == 0) {
//        printf("Cannot open shimset file \"%s\"\n", shimsetPath);
//        return -1;
//    }
//    fgets(line, 80, infd); // Skip irrelevant lines
//    fgets(line, 80, infd);
//    fgets(line, 80, infd);
//
//    for (i = 1, power = 0; i <= m && fgets(line, 80, infd) != 0; i++) {
//        char name[81];
//        double daclimit;
//        double maxCurrent;
//        double coilR;
//        double current;
//        sscanf(line,"%s %lf %lf %lf", name, &daclimit, &maxCurrent, &coilR);
//        //fprintf(gxyzfitpwr,"%s %f %.3f %.2f\n", name, daclimit, maxCurrent, coilR);/*CMP*/
//        current = maxCurrent * a[i] / daclimit;
//        power += current * current * coilR;
//    }
//    return power;
//}

static double calculatePower(double *a, double *fixedvalues, double *powerFactors, int nShims)
{
    int i;
    double tmp;
    double power;

    for (i = 1, power = 0; i <= nShims; i++) {
        if (fixedvalues[i] != 0) {
            tmp = fixedvalues[i] * powerFactors[i];
        } else {
            tmp = a[i];// * powerFactors[i];
        }
        power += tmp * tmp;
    }
    return power;
}

static void getshimsetname(char *shimsetName, int shimset)
{
   sprintf(shimsetName,"shimset%d",shimset);
   if (shimset == 9)
   {
      double H1freq;
      if (P_getreal(SYSTEMGLOBAL, "h1freq", &H1freq, 1) == 0)
      {
         int h1freq;
         h1freq = (int) (H1freq+0.5);
         if ( (h1freq == 700) || (h1freq > 750) )
            strcat(shimsetName,"_UHF");
      }
   }
}

/**
 * Gets the factors used to turn DacUnits into Watts: sqrt(Ohms)*Amps/Dacunit.
 * P(watts) = (Dacunits * factor)^2
 * "names" is an ordered list of the "nShims" shim names (1:nShims).
 * "factors" is a dvector(1:nShims) to receive the factor for each shim.
 * Returns 1 on success, or 0 if it could not find the scaling info.
 */
static int getPowerFactors(int shimset, int nShims,
                           char names[][SHIM_NAME_LEN], double *factors)
{
    int i;
    char shimsetPath[MAXPATH];
    char shimsetName[MAXPATH];
    char line[81];
    FILE *infd;
    int ok = 1;

    // Initialize w/ default values
    for (i = 1; i <= nShims; i++) {
        factors[i] = 1e-5;
    }
    //return 0;/*CMP*/

    // File containing shim coil info:
    sprintf(shimsetPath, "%s/3Dshimlib/data/shimpower", userdir);
    infd = fopen(shimsetPath, "r");
    if (infd == 0) {
        // No shim coil info in 3Dshimlib/data; look for shimtab directory
        getshimsetname(shimsetName,shimset);
#ifdef GXYZFIT
        i = 1;
        sprintf(shimsetPath, "/vnmr/gxyzshim/shimtab/%s", shimsetName);
#else
        i = appdirFind(shimsetName, "gxyzshim/shimtab", shimsetPath, "", R_OK);
#endif
        if (i > 0) {
            infd = fopen(shimsetPath, "r");
        }
        if (infd == 0) {
            fprintf(gxyzfitpwr, "CANNOT OPEN SHIMSET FILE: \"%s\"\n\n",
                    shimsetName);
            return 0;
        }
    }
    fprintf(gxyzfitpwr,"getPowerFactors: shimsetPath=%s\n", shimsetPath);
    fprintf(gxyzfitpwr,"(DAC * PowerFactor)**2 = Watts\n");
    fprintf(gxyzfitpwr,"Shim\t\tMax I\tCoil R\tPowerFactor\n");

    for (i = 1; i <= nShims; i++) {
        char name[81];
        double daclimit;
        double maxCurrent;
        double coilR;
        int nRewinds; // counts rewinds of the file
        int done = 0; // whether we got data for this shim

        // Read lines until we find the one for this shim
        // May need to rewind once to find the line
        for (nRewinds = 0; !done && nRewinds < 2; ) {
            if (fgets(line, 80, infd) == 0) {
                // End-of-file
                rewind(infd);
                nRewinds++;
            } else {
                int n;
                n = sscanf(line,"%s %lf %lf %lf",
                           name, &daclimit, &maxCurrent, &coilR);
                if (n == 4) {
                    //fprintf(gxyzfitpwr,"%s\n", name);/*CMP*/
                    if (strcasecmp(name, names[i]) == 0) {
                        //fprintf(gxyzfitpwr,"%s %f %.3f %.2f\n",
                        //        name, daclimit, maxCurrent, coilR);/*CMP*/
                        factors[i] = sqrt(coilR) * maxCurrent / daclimit;
                        done = 1;
                    }
                }
            }
        }
        if (!done) {
            factors[i] = 1;
            ok = 0;
        }
        fprintf(gxyzfitpwr, "%2d    %-7s %7.2f %8.2f %9.3fe-6\n",
                i, name, maxCurrent, coilR, factors[i] * 1e6);
    }
    fprintf(gxyzfitpwr, "\n");
    fclose(infd);
    return ok;
}

/**
 * Determines the sort order (smallest to largest absolute value) of the given
 * array of "nvals" "values". No actual sorting is done. The "order" array is
 * populated with the indices of the "values" array in order of smallest to
 * largest.
 * The caller must allocate space for the "order" array.
 */
/*
static void getSortOrder(double *values, int nvals, int *order)
{
    int i;
    int j;
    double vmin;
    double vmax;
    double vdone;

    // Find largest value
    vmax = values[1];
    for (i = 2; i <= nvals; i++) {
        double absv = fabs(values[i]);
        if (absv > vmax) {
            vmax = absv;
        }
    }

    vdone = -1;
    for (j = 1; j <= nvals; ) {
        // Find lowest unhandled value
        vmin = vmax; // initialize to max value
        for (i = 1; i <= nvals; i++) {
            double absv = fabs(values[i]);
            if (absv < vmin && absv > vdone) {
                vmin = absv;
            }
        }

        // Handle all cases with that value
        for (i = 1; i <= nvals; i++) {
            double absv = fabs(values[i]);
            if (absv == vmin) {
                order[j++] = i;
            }
        }
        vdone = vmin;
    }

    //for (i = 1; i <= nvals; i++) {
    //    j = order[i];
    //    printf("order[%02d]=%d ==> %.6g\n", i, j, values[j]);
    //}
}
*/

/*
static void printRmsVsPowerByWeight(double *w, double *chi2ByTerm, double *pwrByTerm, int n, int m)
{
    int i;
    char path[MAXPATH];
    FILE *outfile;
    int order[MAX_SHIMS + 1];
    double chi2;
    double pwr;

    sprintf(path,"%s/3Dshimlib/data/RmsVsPwrByWgt", userdir);
    outfile = fopen(path, "w");
    if (outfile != 0) {
        chi2 = chi2ByTerm[0]; // Init to chi2 with full power
        pwr = pwrByTerm[0]; // Init to full power
        printf("Get Sort Order for printRmsVsPowerByWeight\n");
        getSortOrder(w, m, order);
        for (i = 0; i <= m; i++) {
            double rms;
            if (i > 0) {
                int j = order[i];
                chi2 += chi2ByTerm[j];
                pwr -= pwrByTerm[j];
            }
            rms = sqrt(chi2 / (n - m + i));
            fprintf(outfile, "%g %g\n", pwr, rms);
        }
        fclose(outfile);
    }
}
*/

/*
static void printRmsVsPowerByMinRms(double *chi2ByTerm, double *pwrByTerm, int n, int m)
{
    int i;
    char path[MAXPATH];
    FILE *outfile;
    int order[MAX_SHIMS + 1];
    double *drms_dpwr;
    double chi2;
    double pwr;

    sprintf(path,"%s/3Dshimlib/data/RmsVsPwrByMinRms", userdir);
    outfile = fopen(path, "w");
    if (outfile != 0) {
        chi2 = chi2ByTerm[0]; // Init to chi2 with full power
        pwr = pwrByTerm[0]; // Init to full power

        drms_dpwr = dvector(1, m);
        for (i = 1; i <= m; i++) {
            drms_dpwr[i] = sqrt(chi2ByTerm[i]) / pwrByTerm[i];
        }
        printf("Get Sort Order for printRmsVsPowerByMinRms\n");
        getSortOrder(drms_dpwr, m, order);
        free_dvector(drms_dpwr, 1, m);

        for (i = 0; i <= m; i++) {
            double rms;
            if (i > 0) {
                int j = order[i];
                chi2 += chi2ByTerm[j];
                pwr -= pwrByTerm[j];
            }
            rms = sqrt(chi2 / (n - m + i));
            fprintf(outfile, "%g %g\n", pwr, rms);
        }
        fclose(outfile);
    }
}
*/

/*
static void printRmsVsPowerByMinChisq(double *chi2ByTerm, double *pwrByTerm, int n, int m)
{
    int i;
    char path[MAXPATH];
    FILE *outfile;
    int order[MAX_SHIMS + 1];
    double *drms_dpwr;
    double chi2;
    double pwr;

    sprintf(path,"%s/3Dshimlib/data/RmsVsPwrByMinChisq", userdir);
    outfile = fopen(path, "w");
    if (outfile != 0) {
        chi2 = chi2ByTerm[0]; // Init to chi2 with full power
        pwr = pwrByTerm[0]; // Init to full power

        //drms_dpwr = dvector(1, m);
        //for (i = 1; i <= m; i++) {
        //    drms_dpwr[i] = chi2ByTerm[i] / pwrByTerm[i];
        //}
        printf("Get Sort Order for printRmsVsPowerByMinChisq\n");
        //getSortOrder(drms_dpwr, m, order);
        getSortOrder(chi2ByTerm, m, order);
        //free_dvector(drms_dpwr, 1, m);

        for (i = 0; i <= m; i++) {
            double rms;
            if (i > 0) {
                int j = order[i];
                chi2 += chi2ByTerm[j];
                pwr -= pwrByTerm[j];
            }
            rms = sqrt(chi2 / (n - m + i));
            fprintf(outfile, "%g %g\n", pwr, rms);
        }
        fclose(outfile);
    }
}
*/

/*
static void printRmsVsPowerByAllTerms(double *w, double *chi2ByTerm, double *pwrByTerm, int n, int m)
{
    char path[MAXPATH];
    FILE *outfile;

    sprintf(path,"%s/3Dshimlib/data/RmsVsPwrByAllTerms", userdir);
    outfile = fopen(path, "w");
    if (outfile != 0) {
        int order[MAX_SHIMS + 1];
        long ncombos;
        int i;
        double maxChi2 = chi2ByTerm[0];
        for (i = 1; i <= m; i++) {
            maxChi2 += chi2ByTerm[i];
        }

        getSortOrder(w, m, order);
        i = m > 20 ? 20 : m;
        for (ncombos = (1 << i) - 1; ncombos >= 0; ncombos--) {
            long combo;
            int j;
            double rms;
            double chi2 = chi2ByTerm[0]; // Init to chi2 with full power
            double pwr = pwrByTerm[0]; // Init to full power
            if (ncombos % 100000 == 0) {
                printf("ncombos=0x%08x\n", ncombos);
            }
            for (i = 1, combo = ncombos; combo > 0; i++, combo >>= 1) {
                if (combo & 1) {
                    int j = order[i];
                    chi2 += chi2ByTerm[j];
                    pwr -= pwrByTerm[j];
                }
            }
            rms = sqrt(chi2 / (n - m + i));
            fprintf(outfile, "%g %g\n", pwr, rms);

            //chi2 = maxChi2;
            //pwr = 0;
            //for (i = 1, combo = ncombos; combo > 0; i++, combo >>= 1) {
            //    if (combo & 1) {
            //        int j = order[m + 1 - i];
            //        chi2 -= chi2ByTerm[j];
            //        pwr += pwrByTerm[j];
            //    }
            //}
            //rms = sqrt(chi2 / (n - m + i));
            //fprintf(outfile, "%g %g\n", pwr, rms);
        }
        fclose(outfile);
    }
}
*/

static void getChi2ByTerm(double **u, double *w, double **v, double *y,
                          double *sig, int n, int m,
                          double *chi2ByTerm)
{
    int i;
    double wt0;
    double a[MAX_SHIMS+1];

    // Get calculated coefficients (into vector a)
    vnmr_svd_solve(u, w, v, n, m, y, a);
    // Calculate chisq of fit with all terms at full power
    chi2ByTerm[0] = getChisq(y, sig, n, a, m);
    //printf("chi2[0]=%g\n", chi2ByTerm[0]);/*CMP*/

    // Calculate delta chi2 omitting each term individually: m values
    for (i = 1; i <= m; i++) {
        wt0 = w[i];
        w[i] = 0;
        // Get calculated coefficients (into vector a)
        vnmr_svd_solve(u, w, v, n, m, y, a);
        w[i] = wt0;
        // Calculate delta-chisq of fit
        chi2ByTerm[i] = getChisq(y, sig, n, a, m) - chi2ByTerm[0];
        //printf("chi2[%d]=%g\n", i, chi2ByTerm[i]);/*CMP*/
    }
}

static void getPwrByTerm(double **u, double *w, double **v, double *y,
                         double *sig, int n, int m,
                         double *fixflags, double *pwrFactors,
                         double *pwrByTerm)
{
    int i;
    double wt0;
    double a[MAX_SHIMS+1];

    // Get calculated coefficients (into vector a)
    vnmr_svd_solve(u, w, v, n, m, y, a);

    // Calculate power with all terms full on
    pwrByTerm[0] = calculatePower(a, fixflags, pwrFactors, m);
    //printf("power[0]=%g\n", pwrByTerm[0]);/*CMP*/

    // Calculate delta power omitting each term individually: m values
    for (i = 1; i <= m; i++) {
        wt0 = w[i];
        w[i] = 0;
        // Get calculated coefficients (into vector a)
        vnmr_svd_solve(u, w, v, n, m, y, a);
        w[i] = wt0;
        // Calculate power
        pwrByTerm[i] = pwrByTerm[0] - calculatePower(a, fixflags, pwrFactors, m);
        //printf("power[%d]=%g\n", i, pwrByTerm[i]);/*CMP*/
    }
}

static void printMinRmsVsPower(int n, int m,
                               double *chi2ByTerm, double *pwrByTerm)
{
    int i;
    char path[MAXPATH];
    FILE *outfile;
    int npts = 100; // Number of RMS points to print out
    double chi2Grad = -1000;
    double maxPwr; // The sum of the max powers of all the variable terms
    double fixedPwr; // Total power from the fixed terms

    //printf("printMinRmsVsPower: START\n");/*CMP*/
    // Write out min RMS deviation vs. power
    sprintf(path,"%s/3Dshimlib/data/MinRmsVsPwr", userdir);
    outfile = fopen(path, "w");
    maxPwr = getTotalPwrForGradient(0, chi2ByTerm, pwrByTerm, m);
    fixedPwr = pwrByTerm[0] - maxPwr;
    for (i = 1; i < npts; i++) {
        double p = i * maxPwr / npts;
        //printf("chi2Grad=%g\n", chi2Grad);/*CMP*/
        double chi2 = getMinChi2ForPower(chi2ByTerm, pwrByTerm, m, p, &chi2Grad);

        if (outfile != 0) {
            double rms = sqrt(chi2 / (n - m)); // Estimate of actual RMS scatter
            fprintf(outfile, "%g %g\n", p + fixedPwr, rms);
        }
    }
    if (outfile != 0) {
        // Calculate and print RMS scatter w/ full power
        double rms = sqrt(chi2ByTerm[0] / (n - m));
        fprintf(outfile, "%g %g\n", pwrByTerm[0], rms);
        fclose(outfile);
    }
    //printf("printMinRmsVsPower: DONE\n");/*CMP*/
}

/**
 * @param chi2ByTerm ChiSq reduction given by each term at full power [1:m].
 *        chi2ByTerm[0] is the net ChiSq at full power.
 * @param pwrByTerm The power in each basis vector at full power (best fit)
 *        in Watts. pwmByTerm[0] is the total power (sum of terms 1 - m).
 * @param m The number of terms or basis vectors.
 * @param n The number of data points in the fit (used for calculating the
 *        rms error in Hz for a given chi2).
 * @param minSlope The target slope (value of [d log(rms) / d log(power)]).
 *        The power is reduced until the slope is at this value unless one
 *        of the other limits is reached first.
 * @param maxErr The power is reduced until the rms field error in Hz reaches
 *        this value, unless one of the other limits is reached first.
 * @param maxDeltaErr The power is reduced until the relative rms error
 *        reaches this value, unless another limit is reached first.
 *        E.g., 0.1 means a 10% increase in rms error.
 * @return The minimum power level that satisfies the above limits (W).
 *         The actual value is 0% to 1% less than the returned value.
 */
static double getPowerForErrorGradient(double *chi2ByTerm, double *pwrByTerm,
                                       int m, int n,
                                       double minSlope,
                                       double maxErr,
                                       double maxDeltaErr)
{
    double chi2Grad = -1000;
    //double p1 = pwrByTerm[0];
    double p1 = getTotalPwrForGradient(0, chi2ByTerm, pwrByTerm, m);
    double fixedPwr = pwrByTerm[0] - p1;
    double p2 = p1;
    double chiSq2 = chi2ByTerm[0];
    double slope = 0;
    double rms2 = sqrt(chiSq2 / (n - m));
    double rms0 = rms2;

    if (minSlope < 0) {
        fprintf(gxyzfitpwr,
                "Optimizing power to give (d log(err) / d log(P)) = %g\n",
                minSlope);
    }
    while (slope > minSlope) {
        //fprintf(gxyzfitpwr, "p1=%g, p2=%g, rms2=%g\n", p1, p2, rms2);/*CMP*/
        p2 = p1;
        p1 = p2 * 0.99;
        double chiSq1 = getMinChi2ForPower(chi2ByTerm, pwrByTerm, m, p1,
                                           &chi2Grad);
        double rms1 = sqrt(chiSq1 / (n - m));
        // Calc [d log(rms) / d log(power)]
        slope = ((rms2 - rms1) * p2) / (rms2 * (p2 - p1));
        if (rms1 > maxErr) {
            fprintf(gxyzfitpwr,
                    "Power optimization hit max absolute error limit at %f W\n",
                    p2 + fixedPwr);
            break;
        }
        if ((rms1 - rms0) / rms0 > maxDeltaErr) {
            fprintf(gxyzfitpwr,
                    "Power optimization hit max error increase limit at %f W\n",
                    p2 + fixedPwr);
            break;
        }
        rms2 = rms1;
    }
    return p2 + fixedPwr;
}

/**
 * Find the minimum chi2 for a given total shim power.
 * "dChiSqr_dP" is a guess at the critical chi2Gradient; on return
 * it contains the actual gradient used.
 */
#define GRAD_INCR (1.618)
#define GRAD_MIN (-1e-8)
#define CHI2_TOL (1e-5)
#define MAX_ITRS (100)
/**
 * @param chi2ByTerm ChiSq reduction given by each term at full power [1:m].
 *        chi2ByTerm[0] is the net ChiSq at full power.
 * @param pOptByTerm The full power value of each term in Watts [1:m].
 *        pOptByTerm[0] is the total power (sum of the m terms).
 * @param m The number of terms.
 * @param p The given power.
 * @param dChiSqr_dP On input: Guess for the partial dChiSq/dP for each term.
 *        On output: The per-term partial dChiSq/dP at the given power.
 * @return The minimum ChiSq at the given power.
 */
static double getMinChi2ForPower(double *chi2ByTerm, double *pOptByTerm, int m,
                                 double p, double *dChiSqr_dP)
{
    // "chi2Gradient" is the partial derivative dChi2/dP for a single term.
    // At min chi2, all terms use power set to give the same chi2Gradient.
    // Find the critical chi2Gradient that gives the specified power.

    double dChiSqr_dP0; // Bracket the critical gradient - lower pwr
    double dChiSqr_dP1; // Bracket the critical gradient - upper pwr
    double dChiSqr_dP3; // New guess at critical gradient
    double chi0; // chiSqr at lower power gradient
    double chi1; // chiSqr at upper power gradient
    double chi3; // chiSqr at new guess
    double p0; // Lower power guess
    double p1; // Upper power guess
    double p3; // New power guess
    int i;

    if (*dChiSqr_dP >= 0) {
        *dChiSqr_dP = -1; // Only negative values make sense
    }

    /// Bracket the critical gradient.
    dChiSqr_dP0 = dChiSqr_dP1 = *dChiSqr_dP;
    p0 = p1 = getTotalPwrForGradient(dChiSqr_dP0, chi2ByTerm, pOptByTerm, m);
    chi0 = chi1 = getTotalChi2ForGradient(dChiSqr_dP0, chi2ByTerm, pOptByTerm, m);
    if (p0 == p) {
        return chi0;
    } else if (p0 < p) {
        for (i = 0; i < MAX_ITRS && p1 < p; i++) {
            dChiSqr_dP1 = dChiSqr_dP1 / GRAD_INCR;
            if (i >= MAX_ITRS - 1) {
                dChiSqr_dP1 = 0;
            }
            p1 = getTotalPwrForGradient(dChiSqr_dP1, chi2ByTerm, pOptByTerm, m);
            chi1 = getTotalChi2ForGradient(dChiSqr_dP1, chi2ByTerm, pOptByTerm, m);
            if (p1 < p) {
                dChiSqr_dP0 = dChiSqr_dP1;
                p0 = p1;
                chi0 = chi1;
            }
        }
        // p0 < p <= p1
    } else { // (p0 > p)
        for (i = 0; i < MAX_ITRS && p0 > p; i++) {
            if (dChiSqr_dP0 == 0) {
                dChiSqr_dP0 = GRAD_MIN;
            } else {
                dChiSqr_dP0 *= GRAD_INCR;
            }
            p0 = getTotalPwrForGradient(dChiSqr_dP0, chi2ByTerm, pOptByTerm, m);
            chi0 = getTotalChi2ForGradient(dChiSqr_dP0, chi2ByTerm, pOptByTerm, m);
            if (p0 > p) {
                dChiSqr_dP1 = dChiSqr_dP0;
                p1 = p0;
                chi1 = chi0;
            }
        }
        // p0 <= p < p1
    }
    // NB: chi0 > chi1

    /// Use bisection to find critical gradient for chi2 within tolerance.
    for (i = 0; i < MAX_ITRS && (chi0 - chi1) / (chi0 + chi1) > CHI2_TOL / 2; i++) {
        dChiSqr_dP3 = (dChiSqr_dP0 + dChiSqr_dP1) / 2;
        p3 = getTotalPwrForGradient(dChiSqr_dP3, chi2ByTerm, pOptByTerm, m);
        chi3 = getTotalChi2ForGradient(dChiSqr_dP3, chi2ByTerm, pOptByTerm, m);
        if (p3 < p) {
            dChiSqr_dP0 = dChiSqr_dP3;
            p0 = p3;
            chi0 = chi3;
        } else { // (p3 >= p)
            dChiSqr_dP1 = dChiSqr_dP3;
            p1 = p3;
            chi1 = chi3;
        }
    }
    //fprintf(stderr, "chi2 err=%g\n", (chi0 - chi1) / (chi0 + chi1));/*CMP*/
    // Update critical gradient and return the calculated min chi-sqr.
    if (p1 - p < p - p0) {
        chi3 = chi1;
        *dChiSqr_dP = dChiSqr_dP1;
    } else {
        chi3 = chi0;
        *dChiSqr_dP = dChiSqr_dP0;
    }
    return chi3;
}
#undef GRAD_INCR
#undef GRAD_MIN
#undef CHI2_TOL

/**
 *
 * @param w On INPUT: The weights in the SVD (w[1:m]);
 *          On OUTPUT: adjusted weights.
 */
static void getWeightsForGradient(double dChiSqr_dP, double *chi2ByTerm,
                                  double *pOptByTerm, int m, double *w)
{
    int i;
    for (i = 1; i <= m; i++) {
        double p = getPwrForGradient(dChiSqr_dP, pOptByTerm[i], chi2ByTerm[i]);
        if (pOptByTerm[i] > 0 && p > 1e-12 * pOptByTerm[i]) {
            w[i] *= sqrt(pOptByTerm[i] / p);
        } else {
            w[i] = 0;
        }
        //fprintf(gxyzfitpwr,"w[%d]=%g, pOptByTerm=%g, p=%g, chi2ByTerm=%g\n",
        //        i, w[i], pOptByTerm[i], p, chi2ByTerm[i]);/*CMP*/
    }
}

/**
 * This calculates the partial derivative dChi2/dP for a single term
 * with given p_opt and deltaChi2, at the given power level, p.
 * Uses the formula y = (1 - sqrt(x))**2.
 * @param p The power to use in the basis vector.
 * @param p_opt The power in the basis vector for the optimum fit.
 * @param deltaChi2 The Chi-Square reduction produced by this basis vector.
 * @return The partial derivative, dChi2/dP, at the given power.
 */
/*
static double getChi2Gradient(double p, double p_opt, double deltaChi2)
{
    return deltaChi2 * ((1 / p_opt) - (1 / (sqrt(p_opt) * sqrt(p))));
}
*/

/**
 * Calculate the chi2 increase for a term when its power is reduced
 * from the optimum.
 * Uses the formula y = (1 - sqrt(x))**2.
 * @param p The power to use in the basis vector.
 * @param p_opt The power in the basis vector for the optimum fit.
 * @param deltaChi2 The Chi-Square reduction produced by this basis vector
 * at optimum power.
 * @return The Chi-Square increase produced by reducing the power in
 * this basis vector to the given power level.
 */
static double getChi2ByPwr(double p, double p_opt, double deltaChi2)
{
    double pdiff = 1 - sqrt(p / p_opt);
    return deltaChi2 * pdiff * pdiff;
}

/**
 * Calculate the power for a single term to give the specified chi2Gradient.
 * Uses the formula y = (1 - sqrt(x))**2.
 * @param dChiSqr_dP The desired chi2Gradient (partial derivative of
 * Chi-Square w.r.t. power in this basis vector).
 * @param p_opt The power in the basis vector for the optimum fit.
 * @param deltaChi2 The Chi-Square reduction produced by this basis vector
 * at optimum power.
 * @return The power level in this basis vector that gives the
 * specified gradient.
 */
static double getPwrForGradient(double dChiSqr_dP, double p_opt, double deltaChi2)
{
    double b = sqrt(p_opt);
    double pwr = deltaChi2 / ((deltaChi2 / b) - (b * dChiSqr_dP));
    pwr *= pwr;
    return pwr;
}

/**
 * Calculate the total power for the specified chi2Gradient.
 * @param dChiSqr_dP The desired chi2Gradient (partial derivative of
 * Chi-Square w.r.t. power in each basis vector).
 * @param chi2ByTerm Array of Chi-Square reductions produced by each basis vector
 * at optimum power.
 * @param pOptByTerm Array of powers in the basis vectors for the optimum fit.
 * @param m The number of basis vectors.
 * @return The sum of the powers put into all of the basis vectors.
 */
static double getTotalPwrForGradient(double dChiSqr_dP, double *chi2ByTerm, double *pOptByTerm, int m)
{
    int i;
    double p = 0;
    for (i = 1; i <= m; i++) {
        if (pOptByTerm[i] != 0 &&  chi2ByTerm[i] != 0) { // Skip fixed (railed) terms
            p += getPwrForGradient(dChiSqr_dP, pOptByTerm[i], chi2ByTerm[i]);
        }
    }
    return p;
}

/**
 * Calculate the total Chi-Square for the specified chi2Gradient.
 * @param dChiSqr_dP The desired chi2Gradient (partial derivative of
 * Chi-Square w.r.t. power in each basis vector).
 * @param chi2ByTerm Array of Chi-Square reductions produced by each basis vector
 * at optimum power.
 * @param pOptByTerm Array of powers in the basis vectors for the optimum fit.
 * @param m The number of basis vectors.
 * @return The Chi-Square of the resulting fit.
 */
static double getTotalChi2ForGradient(double dChiSqr_dP, double *chi2ByTerm, double *pOptByTerm, int m)
{
    int i;
    double p;
    double chi2;

    chi2 = chi2ByTerm[0];
    //fprintf(stderr,"--- chi2[0]=%g, dChiSqr_dP=%g\n", chi2, dChiSqr_dP);/*CMP*/
    for (i = 1; i <= m; i++) {
        if (pOptByTerm[i] != 0 &&  chi2ByTerm[i] != 0) {
            p = getPwrForGradient(dChiSqr_dP, pOptByTerm[i], chi2ByTerm[i]);
            chi2 += getChi2ByPwr(p, pOptByTerm[i], chi2ByTerm[i]);
            //fprintf(stderr,"--- p[%d]=%g, chi2[%d]=%g, chi2=%g\n", i, p, i, getChi2ByPwr(p, pOptByTerm[i], chi2ByTerm[i]), chi2);/*CMP*/
        }
    }
    return chi2;
}

/**
 * Gets the key part of a "key=value" argument, converted to lower case.
 * @param key On return: the "key" part in lower case.
 * @param arg The argument to look at.
 */
static void getArgKey(char *key, char *arg) {
    int i;
    int n;
    char *p = strchr(arg, '=');
    n = (p == NULL) ? 0 : p - arg;
    strncpy(key, arg, n);
    key[n] = '\0';
    for (i = 0; i < n; i++) {
        if (isupper(key[i])) {
            key[i] = tolower(key[i]);
        }
    }
}

/**
 * Gets the value part of a "key=value" argument as a String.
 * @param value On return: the "value" part of the argument.
 * @param arg The argument to look at.
 */
static void getArgStringValue(char *value, char *arg) {
    char *p = strchr(arg, '=');
    if (p == 0) {
        strcpy(value, "");
    } else {
        strcpy(value, p + 1);
    }
}

/**
 * Returns the value part of a "key=value" argument as an int.
 * @param arg The argument to look at.
 * @return The "value" interpreted as an int.
 */
static int getArgIntValue(char *arg) {
    char strValue[MAXPATH];
    getArgStringValue(strValue, arg);
    return atoi(strValue);
}

/**
 * Returns the value part of a "key=value" argument as a double.
 * @param arg The argument to look at.
 * @return The "value" interpreted as a double.
 */
static double getArgDoubleValue(char *arg) {
    char strValue[MAXPATH];
    getArgStringValue(strValue, arg);
    return atof(strValue);
}

/*--- start of main function ---*/
int gxyzfit(argc,argv,retc,retv)
     int argc;
     int retc;
     char *argv[];
     char *retv[];
{
    int flag,i,j,nsets;
    double shimset,range,dum,chisq,*x,*z,*sig,*a,*w,**cvm,**u,**v;
    double *powerFactors;
    double *yOrig;
    int nterms;
    int prevNterms;
    double prevChisq;
    double origChisq;
    double power;
    double prevPower;
    double cvmax;
    double chi2ByTerm[MAX_SHIMS + 1]; // Chi2 increase from omitting i'th term
    double pwrByTerm[MAX_SHIMS + 1]; // Optimum power for i'th term
    double signifByTerm[MAX_SHIMS + 1]; // Probability i'th term is significant

    // Parameters settable by caller:
    int dcf = 1; // Subtract out DC offset of field/shim maps
    int nShimsUsed = 0; // Number of shims to fit (0 ==> all)
    double tol = 1e-6; // Tolerance for zeroing singular values.
    double pThresh = 0.95; // Threshold probability for using term in fit.
    double chi2Thresh = 0.0; // Mininum fractional chi2 change to include term.
    int zeroBased = 1; // Fit to calculated zero-shim-power field map.
    double powerLimit = 100; // Max allowed total shim power (Watts)
    double maxErrorVsPowerLimit = -0.1; // d(log(rmsError)) / d(log(P))
    double maxErrorLimit = 10; // Max RMS field variation in Hz
    double maxErrorIncreaseLimit = 1; // Max error increase to reduce pwr
                                      // 1 ==> error can double (increase 100%)

    // Open log files
    strcpy(path, userdir);
    strcat(path, "/3Dshimlib/data/mapdata");
    if ((mapfile=fopen(path,"r"))==0)
    {
        printf("Cannot open file mapdata \n\n");
        ABORT;
    }
    strcpy(path, userdir);
    strcat(path, "/3Dshimlib/data/gxyzfitout");
    if ((gxyzfitout=fopen(path,"w"))==0)
    {
        printf("Cannot open file gxyzfitout \n\n");
        fclose(mapfile);
        ABORT;
    }
    strcpy(path, userdir);
    strcat(path, "/3Dshimlib/data/gxyzfitpwr");
    if ((gxyzfitpwr=fopen(path,"w"))==0)
    {
        printf("Cannot open file gxyzfitpwr \n\n");
        fclose(mapfile);
        fclose(gxyzfitout);
        ABORT;
    }

    fprintf(gxyzfitpwr, "gxyzfit starting\n\n");/*CMP*/

    // Read arguments
    for (i = 1; i < argc; i++) {
        char key[MAXPATH];
        fprintf(gxyzfitpwr, "arg[%d]=\"%s\"\n", i, argv[i]);/*CMP*/
        getArgKey(key, argv[i]);
        if (strcmp("nshims", key) == 0) {
            nShimsUsed = getArgIntValue(argv[i]);
#ifdef GXYZFIT
        } else if (strcmp("shimset", key) == 0) {
            // This argument only used by standalone version
            ShimSet = getArgIntValue(argv[i]);
#endif
        } else if (strcmp("zerobased", key) == 0) {
            zeroBased = getArgIntValue(argv[i]);
        } else if (strcmp("minweight", key) == 0) {
            tol = getArgDoubleValue(argv[i]);
        } else if (strcmp("minprobability", key) == 0) {
            pThresh = getArgDoubleValue(argv[i]);
        } else if (strcmp("minchi2", key) == 0) {
            chi2Thresh = getArgDoubleValue(argv[i]);
        } else if (strcmp("maxpower", key) == 0) {
            powerLimit = getArgDoubleValue(argv[i]);
        } else if (strcmp("maxerrorincreasepct", key) == 0) {
            maxErrorIncreaseLimit = getArgDoubleValue(argv[i]) / 100;
        } else if (strcmp("maxerror", key) == 0) {
            maxErrorLimit = getArgDoubleValue(argv[i]);
        } else if (strcmp("maxdeltaerrorvspower", key) == 0) {
            // It's the max absval of a negative quantity...
            maxErrorVsPowerLimit = -fabs(getArgDoubleValue(argv[i]));
        } else if (i == 1 && strspn(argv[i], "0123456789") == strlen(argv[i])) {
            // Special, legacy case; bare integer is number of shims to use
            nShimsUsed = atoi(argv[i]);
        } else {
            fprintf(stderr,"Bad argument to gxyzfit: \"%s\"\n", argv[i]);
        }
    }
    // Check if called as "gxyz0fit"
    if (argv[0][4]=='0') {
        dcf=0;
    }
    fprintf(gxyzfitpwr, "dcOffsetsRemoved=%d\n\n", dcf);/*CMP*/
 
    for (i=0;i<MAX_SHIMS+1; i++)
    {
        FixFlags[i]=0;
    }
    //fprintf(gxyzfitpwr,"\nStart gxyzfit\n\n");/*CMP*/

    range=32767.0;
#ifndef GXYZFIT
    if (P_getreal(SYSTEMGLOBAL, "shimset", &shimset, 1) == 0)
#else
        shimset = ShimSet;
#endif
    {
        if ((shimset<3.0) || ((shimset>9.5) && (shimset<11.5))) range = 2047;
    }

    fgets(dumch,80,mapfile);
    fscanf(mapfile,"%d %d\n",&nsets,&npoints);
    fgets(dumch,80,mapfile);
    nshims= nsets-1;
    fprintf(gxyzfitout,"No. of shims available :%d\t   No. of points:  %d\n\n",
            nshims,npoints);
    if (nShimsUsed > 0 && nShimsUsed < nshims) {
        nshims = nShimsUsed;
    }
    fprintf(gxyzfitout,"No. of shims to fit :%d\n\n",nshims);
    x=dvector(1,npoints);
    y=dvector(1,npoints);
    z=dvector(1,npoints);
    angle=dvector(1,npoints);
    fy=dvector(1,npoints);
    sig=dvector(1,npoints);
    yOrig = dvector(1, npoints);
    a=dvector(1,nshims);
    w=dvector(1,nshims);
    powerFactors = dvector(1, nshims);
    cvm=dmatrix(1,nshims,1,nshims);
    shimmap=dmatrix(1,npoints,1,nshims);
    u=dmatrix(1,npoints,1,nshims);
    v=dmatrix(1,nshims,1,nshims);
    shoff=dvector(1,nshims);
    shold=dvector(1,nshims);
    shnew=dvector(1,nshims);
    shdiff=dvector(1,nshims);
    sherr=dvector(1,nshims);
    /*
      fprintf(gxyzfitout,"memory allocated, hit return\n");
      (void) getchar();
    */

    // Initialize maptitle, shname, shoff, shold, shnew, shdiff, sherr
    readlistfile();

    // Initialize "powerFactors"
    getPowerFactors(shimset, nshims, shname, powerFactors);

    /*  Read in field map data */
    // Initializes "x", "y", "sig"
    fgets(dumch,80,mapfile);
    // Look for start of first data block in mapfile (the field map)
    // (past the next "1" token -- line is "1  0  0  0")
    do {
        if (fscanf(mapfile,"%s",str) == EOF)
            strcpy(str,"errflag");
    }  while ((strcmp(str,"1")!= 0)&&(strcmp(str,"errflag")!= 0));
    if (strcmp(str,"errflag")==0) {
        printf("error: invalid format for mapdata file");
        fclose(mapfile);
        fclose(gxyzfitout);
        ABORT;
    }
    // Skip next 3 tokens ("0  0  0")
    for (i=0;i<3;i++) {
        if ((fscanf(mapfile,"%s",str)==EOF)||(strcmp(str,"0")!= 0)) {
            printf("error: invalid format for mapdata file");
            fclose(mapfile);
            fclose(gxyzfitout);
            ABORT;
        }
    }
    sum1=0.0;
    for (i=1;i<=npoints;i++) {
        // NB: "angle" is only used (trivially) by writefitfile()
        fscanf(mapfile,"%lf %lf\n",&angle[i],&y[i]);
        sum1+=y[i];
        sig[i]=1.0;
        x[i]=(double) i;
    }
    if (dcf) {
        // Subtract observed DC offset from field map data
        for (i=1;i<=npoints;i++) {
            y[i] -= sum1 / ((double) npoints);
            /* fprintf(gxyzfitout,"%d %f \n",i,y[i]);  */
        }
    }

    // If y[i] will be modified to reflect field at 0 power,
    // remember original values
    if (zeroBased) {
        for (i = 1; i <= npoints; i++) {
            yOrig[i] = y[i];
        }
    }

    /*  Read in first "nshims" shim maps from "mapfile" */
    // Initializes "shimmap"
    for (j=1;j<=nshims;j++) {
        // Look for start of next data block in mapfile
        // (past the next "1" token -- line is "1  0  0  0")
        do {
            if (fscanf(mapfile,"%s",str) == EOF)
                strcpy(str,"errflag");
        }  while ((strcmp(str,"1")!= 0)&&(strcmp(str,"errflag")!= 0));
        if (strcmp(str,"errflag")==0) {
            printf("error: invalid format for mapdata file");
            fclose(mapfile);
            fclose(gxyzfitout);
            ABORT;
        }
        // Skip next 3 tokens ("0  0  0")
        for (i=0;i<3;i++) {
            if ((fscanf(mapfile,"%s",str)==EOF)||(strcmp(str,"0")!= 0)) {
                printf("error: invalid format for mapdata file");
                fclose(mapfile);
                fclose(gxyzfitout);
                ABORT;
            }
        }
        sum1=0.0;
        //fprintf(gxyzfitpwr,"shoff[%d]=%.0f\n", j, shoff[j]);/*CMP*/
        for (i=1;i<=npoints;i++) {
            fscanf(mapfile,"%lf %lf\n",&dum,&shimmap[i][j]);
            shimmap[i][j] /= powerFactors[j] * shoff[j];
            //shimmap[i][j] /= shoff[j];
            sum1+=shimmap[i][j];
        }
        sum1 /= npoints; // Mean map value = DC offset
        for (i=1;i<=npoints;i++) {
            if (dcf) {
                // Subtract observed DC offset from shim map data
                shimmap[i][j] -= sum1;
            }
            /* fprintf(gxyzfitout,"%d %lf \n",i,shimmap[i][j]);  */
            if (zeroBased) {
                // Modify field map to what it would have been w/o this shim
                // (NB: If dcf==1, this preserves the mean DC offset of 0)
                y[i] -= shold[j] * shimmap[i][j] * powerFactors[j];
            }
        }

    }


    /*  All data are now assembled */
    /*  shimmap[i][j] contains the i'th point of the j'th shim */
    /*  function fshimmap(x,p,nshims) returns array p = shimmap[1:nshims] for point x */
    /*  Now do the fitting */

    flag=1;
    do
    { /* loop mark 1 */
        double maxSingularValue;
        double *afunc;
        double deltaChisq;

        // Solve, in least-squares sense, [A] * [x] = [b].
        // b is vector of npoints observations;
        // A is (npoints by nshims) matrix: npoints basis vector
        // values for each of nshims basis vectors;
        // x is the solution vector giving the coefficient of each
        // basis vector to fit the data.

        // Decomposing [A] into [V] * [diag(W)] * [U]'  (where ' ==> transpose)
        // Construct [A] in u array; it will be replaced by [U].
        afunc = dvector(1, nshims);
        for (i = 1; i <= npoints; i++) {
            // Get all the shimmap values for the i'th point into afunc
            fshimmap((double)i, afunc, nshims);
            //tmp=1.0/sig[i];
            for (j = 1; j <= nshims; j++) {
                //u[i][j]=afunc[j]*tmp;
                u[i][j] = afunc[j];
            }
            //b[i]=y[i]*tmp;
        }
        free_dvector(afunc, 1, nshims);
        vnmr_svd(u, npoints, nshims, w, v); // Given A (in u) get U, W, and V.

        // Get the maximum weight
        maxSingularValue = getMaxValue(w, nshims);

        // Print out all the weights
        //fprintf(gxyzfitpwr,"Relative weights:");
        //for (i = 1; i <= nshims; i++) {
        //    fprintf(gxyzfitpwr," %.3g", w[i] / maxSingularValue);
        //}
        //fprintf(gxyzfitpwr,"\n");

        // ELIMINATE TERMS with numerically singular values
        for (j = 1; j <= nshims; j++) {
            if (w[j] < 1e-12 * maxSingularValue) {
                w[j] = 0;
            }
        }
        nterms = getNumberOfPositiveTerms(w, nshims);
        fprintf(gxyzfitpwr,
                "Eliminated %d numerically singular basis vectors\n",
                nshims - nterms);


        // Initialize "chi2ByTerm"
        // NB: w is, at this point, only edited for numerically singular values.
        //getChi2ByTerm(u, w, v, y, sig, npoints, nshims, chi2ByTerm);
        // Initialize "pwrByTerm" for diagnostic print
        //getPwrByTerm(u, w, v, y, sig, npoints, nshims,
        //             FixFlags, powerFactors,
        //             pwrByTerm);
        //fprintf(gxyzfitpwr,"ChiSq=%g, Power=%g W\n",
        //        chi2ByTerm[0], pwrByTerm[0]);

        // Get calculated coefficients (into vector a)
        vnmr_svd_solve(u, w, v, npoints, nshims, y, a);
        // Calculate chisq of fit
        chisq = getChisq(y, sig, npoints, a, nshims);
        power = calculatePower(a, FixFlags, powerFactors, nshims);
        fprintf(gxyzfitpwr,"ChiSq=%g, Power=%g W\n\n", chisq, power);

        // For each basis vector print the % of power in each shim coil
        fprintf(gxyzfitpwr,"SINGULAR VALUE DECOMPOSITION\n");
        fprintf(gxyzfitpwr,
                "Weights and %% of each shim-map in basis vectors:\n");
        fprintf(gxyzfitpwr,"\t\t\t  ");
        for (i = 1; i <= nshims; i++) {
            int width = strlen(shname[i]) + 1;
            width = width < 4 ? 4 : width;
            fprintf(gxyzfitpwr,"%*s", width, shname[i]);
        }
        fprintf(gxyzfitpwr,"\n");
        for (j = 1; j <= nshims; j++) {
            fprintf(gxyzfitpwr,"w[%02d]=%12.2f  v[%02d]=", j, w[j], j);
            // Print the corresponding v COLUMN
            for (i = 1; i <= nshims; i++) {
                int pwr = (int)rint(100 * (v[i][j]));
                int width = strlen(shname[i]) + 1;
                width = width < 4 ? 4 : width;
                fprintf(gxyzfitpwr,"%*d", width, pwr);
            }
            fprintf(gxyzfitpwr,"\n");
        }
        fprintf(gxyzfitpwr,"\n");


        // Print out all the coefficients
        // Get calculated coefficients (into vector a)
        //vnmr_svd_solve(u, w, v, npoints, nshims, y, a);
        //fprintf(gxyzfitpwr,"Solution vector:");
        //for (i = 1; i <= nshims; i++) {
        //    fprintf(gxyzfitpwr," %.3g", a[i]);
        //}
        //fprintf(gxyzfitpwr,"\n");/*CMP*/
        //fprintf(gxyzfitpwr, "minChisq=%g, power=%g\n",
        //        chi2ByTerm[0], pwrByTerm[0]);/*CMP*/

        // ELIMINATE TERMS with too-small weights
        for (j = 1; j <= nshims; j++) {
            if (w[j] < tol * maxSingularValue) {
                fprintf(gxyzfitpwr,"Vector %d weight=%g (%.3g%%)\n",
                        j,  w[j], 100 * w[j] / maxSingularValue);
                w[j] = 0;
            }
        }
        prevNterms = nterms;
        nterms = getNumberOfPositiveTerms(w, nshims);
        fprintf(gxyzfitpwr,"Eliminated %d low-weight basis vectors\n",
                prevNterms - nterms);

        // Get calculated coefficients (into vector a)
        vnmr_svd_solve(u, w, v, npoints, nshims, y, a);
        // Calculate chisq of fit
        chisq = getChisq(y, sig, npoints, a, nshims);
        power = calculatePower(a, FixFlags, powerFactors, nshims);
        fprintf(gxyzfitpwr,"ChiSq=%g, Power=%g W\n", chisq, power);

        //chisq = 0;
        //for (i = 1; i <= npoints; i++) {
        //    fshimmap((double)i, afunc, nshims);
        //    for (sum = 0, j = 1; j <= nshims; j++) {
        //        sum += a[j] * afunc[j];
        //    }
        //    tmp = (y[i] - sum) / sig[i];
        //    chisq += tmp * tmp;
        //}

        //svdfit(x,y,sig,npoints,a,nshims,tol,u,v,w,zeros,&chisq,fshimmap);


        // ELIMINATE TERMS with too little effect on Chi2
        prevNterms = nterms;
        prevChisq = origChisq = chisq;
        prevPower = power;
        //for (i = 1; i <= nshims; i++) {
        getChi2ByTerm(u, w, v, y, sig, npoints, nshims, chi2ByTerm);
        fprintf(gxyzfitpwr,"\n\tChi2 Reduction\tSignif Probability\n");
        for (i = 1; i <= nshims; i++) {
            double chi2 = chi2ByTerm[i] + chi2ByTerm[0]; // Chi2 w/o this term
            signifByTerm[i] = isFitWorse(chi2, nterms - 1,
                                         chi2ByTerm[0], nterms,
                                         npoints);
            fprintf(gxyzfitpwr,"v[%2d]\t%11.0f\t%.6f\n",
                    i, chi2ByTerm[i], signifByTerm[i]);
        }
        for (i = nshims; i > 0; --i) {
            if (w[i] > 0) {
                int isMax = (w[i] == maxSingularValue);
                deltaChisq = chi2ByTerm[i] / chi2ByTerm[0];
                if (!isMax && (signifByTerm[i] < pThresh
                               || deltaChisq < chi2Thresh))
                {
                    //fprintf(gxyzfitpwr,"DISCARD vector #%d\n", i);
                    fprintf(gxyzfitpwr,
                            "Discard vector %d: delta ChiSq=%.1f%%, p=%g\n",
                            i, deltaChisq * 100, signifByTerm[i]);
                    w[i] = 0;
                    --nterms;
                }
            }
        }
        // Get calculated coefficients (into vector a)
        vnmr_svd_solve(u, w, v, npoints, nshims, y, a);
        // Initialize "chi2ByTerm"
        getChi2ByTerm(u, w, v, y, sig, npoints, nshims, chi2ByTerm);
        // Initialize "pwrByTerm"
        getPwrByTerm(u, w, v, y, sig, npoints, nshims,
                     FixFlags, powerFactors,
                     pwrByTerm);
        fprintf(gxyzfitpwr,"Eliminated %d insignificant basis vectors\n",
                prevNterms - nterms);
        fprintf(gxyzfitpwr,"ChiSq=%g, Power=%g W\n\n",
                chi2ByTerm[0], pwrByTerm[0]);

        // Write out table of min RMS field variation vs. total shim power
        printMinRmsVsPower(npoints, nshims, chi2ByTerm, pwrByTerm);

        // Find power at optimum error/power point
        double pOpt = getPowerForErrorGradient(chi2ByTerm, pwrByTerm,
                                               nshims, npoints,
                                               maxErrorVsPowerLimit,
                                               maxErrorLimit,
                                               maxErrorIncreaseLimit);
        if (powerLimit > pOpt) {
            powerLimit = pOpt;
            if (pOpt > pwrByTerm[0]) {
                fprintf(gxyzfitpwr, "Power reduced to limit of %.3g W\n", pOpt);
            }
        } else {
            fprintf(gxyzfitpwr, "Power reduced to hard limit of %.3g W\n",
                    powerLimit);
        }

        // Apply limit on total power
        if (pwrByTerm[0] > powerLimit) {
            // Total power too big
            double ww[MAX_SHIMS + 1];
            double aa[MAX_SHIMS + 1];
            double origChi2;
            double chi2Grad = -1000;
            // Save original weights & shim amplitudes for reporting diagnostics
            for (i = 1; i <= nshims; i++) {
                aa[i] = a[i];
                ww[i] = w[i];
            }
            origChi2 = chisq;

            // Determine the Chi2 gradient for desired power
            getMinChi2ForPower(chi2ByTerm, pwrByTerm, nshims, powerLimit,
                               &chi2Grad);
            // Calculate scaled weights corresponding to this gradient
            // Weights are increased to reduce power in each term
            getWeightsForGradient(chi2Grad, chi2ByTerm, pwrByTerm, nshims, w);

            // For fun, check the fit and power (should == powerLimit)
            getPwrByTerm(u, w, v, y, sig, npoints, nshims,
                     FixFlags, powerFactors,
                     pwrByTerm);
            // Get calculated coefficients (into vector a)
            vnmr_svd_solve(u, w, v, npoints, nshims, y, a);
            // Calculate chisq of fit
            chisq = getChisq(y, sig, npoints, a, nshims);

            fprintf(gxyzfitpwr,
                    "\nBasis Vector Reduction Factors      |\t\tShim Factors\n");
            for (i = 1; i <= nshims; i++) {
                double shimPwr = a[i] * a[i];
                fprintf(gxyzfitpwr,
                        "vec-factor[%2d]=%6.4f\tpwr=%.4f  |\t%12s%10.3f\tpwr=%.4f\tDAC=%6.0f\n",
                        i, ww[i] / w[i], pwrByTerm[i], shname[i],
                        a[i] / aa[i], shimPwr, -a[i] / powerFactors[i]);
            }
            fprintf(gxyzfitpwr,"\nChiSq=%g, Power=%g W\n\n",
                    chisq,  pwrByTerm[0]);
        }

        // Make sure solution is up-to-date by recalculating it
        // Get calculated coefficients (into vector a)
        vnmr_svd_solve(u, w, v, npoints, nshims, y, a);
        // Calculate chisq of fit
        chisq = getChisq(y, sig, npoints, a, nshims);
        power = calculatePower(a, FixFlags, powerFactors, nshims);

        vnmr_svd_covar(v,nshims,w,cvm);


        sum1 = sqrt(chisq / (npoints - nshims)); // RMS freq err

        fprintf(gxyzfitpwr,"ChiSq=%g, RMS Err=%g Hz, Power=%.2f W\n",
                chisq, sum1, power);

        // Print fit data in gxyzfitout file
        fprintf(gxyzfitout,"\nFit of field map with shim maps\n\n");
        for (i=1;i<=nshims;i++)
            fprintf(gxyzfitout,"%12.6f %s %10.6f\n",a[i],"  +-",sqrt(cvm[i][i]));
        fprintf(gxyzfitout,"\nChi-squared %7.2f\n",chisq);
        fprintf(gxyzfitout,"\nRMS frequency error %2.2f Hz\n\n\n", sum1);
        cvmax = 0;
        for (i=1;i<=nshims;i++)
        {
            for (j=1;j<=nshims;j++)
            {
                if (fabs(cvm[i][j]) > cvmax) {
                    cvmax = fabs(cvm[i][j]);
                }
            }
        }
        fprintf(gxyzfitout,"\nCovariance matrix (covariance = value * %.6g)\n",
                fabs(cvmax) / 100);
        for (i=1;i<=nshims;i++)
        {
            for (j=1;j<=nshims;j++)
            {
                fprintf(gxyzfitout,"%9.3f", 100 * cvm[i][j] / fabs(cvmax));
            }
            fprintf(gxyzfitout,"\n");
        }

        fprintf(gxyzfitout,"\n");
        fprintf(gxyzfitout,"\n");

        sum1=0.0;
        sum2=0.0;
        for (i=1;i<=nshims; i++)
        {
            if (FixFlags[i]==0)
            {
                shdiff[i] = -rint(a[i] / powerFactors[i]);
                sherr[i] = sqrt(cvm[i][i]) / powerFactors[i]; /* not truncated yet*/

                if (zeroBased) {
                    shnew[i] = shdiff[i];
                    shdiff[i] = shnew[i] - shold[i];
                } else {
                    shnew[i]=shold[i]+shdiff[i];
                }
                sum1=sum1+shdiff[i]*shdiff[i];
                sum2=sum2+sherr[i]*sherr[i];

                fprintf(gxyzfitout,"shoff = %f\n",shoff[i]);
                fprintf(gxyzfitout,"shdiff = %f\n",shdiff[i]);
                fprintf(gxyzfitout,"cvm = %f\n",sqrt(cvm[i][i]));
                fprintf(gxyzfitout,"sherr = %f\n",sherr[i]);
                fprintf(gxyzfitout,"shnew = %f\n",shnew[i]);
                fprintf(gxyzfitout,"sum1 = %f\n",sum1);
                fprintf(gxyzfitout,"sum2 = %f\n",sum2);
                fprintf(gxyzfitout,"\n");

            }
            else
            {
                shdiff[i]=0.0;
                sherr[i]=0.0;
                shnew[i]=FixFlags[i];
            }
        }
        rmsdiff=sqrt(sum1/((double)nshims));
        rmserr=sqrt(sum2/((double)nshims));

        // See if any shim's current exceeds maximum allowed
        // If it does, set current to max and repeat fit w/ remaining shims
        flag=0;
        for (i=1;((i<=nshims)&&(flag==0));i++)
        {
            if (FixFlags[i]==0)
            {
                if (shnew[i]<-range)
                {
                    flag=1;
                    fprintf(gxyzfitpwr,"\n\nShim no. %d has clamped at -%.0f (from %.0f)\n",
                            i,range,shnew[i]);//CMP
                    FixFlags[i]=-range;
                    shnew[i]=-range;
                    fprintf(gxyzfitout,"Shim no. %d has been set to -%f\n",i,range);
                    Wscrprintf("Shim no. %d has been set to its limit of -%d\n",i,(int) range);
                    Wscrprintf("Fitting is now being repeated with this shim fixed at its limit\n");
                }
                if (shnew[i]>range)
                {
                    flag=1;
                    fprintf(gxyzfitpwr,"\n\nShim no. %d has clamped at %.0f (from %.0f)\n",
                            i,range,shnew[i]);//CMP
                    FixFlags[i]=range;
                    shnew[i]=range;
                    fprintf(gxyzfitout,"Shim no. %d has been set to %f\n",i,range);
                    Wscrprintf("Shim no. %d has been set to its limit of +%d\n",i,(int) range);
                    Wscrprintf("Fitting is now being repeated with this shim fixed at its limit\n");
                }
                if (FixFlags[i] != 0) {
                    // Just turned on fixflag for this shim
                    // Modify "zero" shim field map to include effect of this fixed shim
                    double correction;
                    if (zeroBased) {
                        // Correction to field is full shim value
                        correction = shnew[i];
                    } else {
                        // Correction to field is diff in shim from when field was measured
                        correction = shnew[i] - shold[i];
                    }
                    for (j = 1; j <= npoints; j++) {
                        y[j] += powerFactors[i] * correction * shimmap[j][i];
                    }
                }
            }
        }
        for (i=1;i<=nshims;i++) fprintf(gxyzfitout,"i\t%d\tfixflag\t%.0f\n",i,FixFlags[i]);
    } /* end of loop mark 1 */  while (flag==1);

    // Construct fit to data for printout by writefitfile()
    for (i = 1; i <= npoints; i++) {
        fy[i] = 0;
        for (j = 1; j <= nshims; j++) {
            fy[i] += a[j] * shimmap[i][j];
        }
    }
    if (zeroBased) {
        for (i = 1; i <= npoints; i++) {
            fy[i] += yOrig[i] - y[i];
            y[i] = yOrig[i];
        }
    }

    fprintf(gxyzfitout,"mapname %s\n",maptitle);
    fprintf(gxyzfitout,"\n");
    fprintf(gxyzfitout,"shimset %d  nshims\t%d   rmsdiff  %4.2f   rmsdiff/rmserr   %6.4f \n\n",
            (int) shimset, nshims,rmsdiff,rmsdiff/
            rmserr);
    fprintf(gxyzfitout,"shim\toffset\t\told\t\tnew\t\tdiff\t\terror\n");
    fprintf(gxyzfitout,"-----------------------------------------------------------------------------\n");
    for (i=1;i<=nshims; i++)
    {
        fprintf(gxyzfitout,"%s\t%6d\t\t%6d\t\t%6d\t\t%6d\t\t%5d\n",shname[i],(int) shoff[i], (int) shold[i], (int) shnew[i],
                (int) shdiff[i], (int) sherr[i]);
    }
    fprintf(gxyzfitout,"-----------------------------------------------------------------------------\n");
    writelistfile((int) shimset);
    writefitfile();
    free_dmatrix(v,1,nshims,1,nshims);
    free_dmatrix(u,1,npoints ,1,nshims);
    free_dmatrix(cvm,1,nshims ,1,nshims);
    free_dvector(w,1,nshims);
    free_dvector(powerFactors, 1, nshims);
    free_dvector(shoff,1,nshims);
    free_dvector(shold,1,nshims);
    free_dvector(shnew,1,nshims);
    free_dvector(shdiff,1,nshims);
    free_dvector(sherr,1,nshims);
    free_dvector(a,1,nshims);
    free_dvector(yOrig, 1, npoints);
    free_dvector(sig,1,npoints);
    free_dvector(z,1,npoints);
    free_dvector(y,1,npoints);
    free_dvector(x,1,npoints);
    free_dvector(fy,1,npoints);
    free_dvector(angle,1,npoints);
    fclose(mapfile);
    fclose(gxyzfitout);
    //fprintf(gxyzfitpwr,"End gxyzfit\n\n");/*CMP*/
    fclose(gxyzfitpwr);
    RETURN;
}
