/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* direct discrete FT */
#ifndef __DDFT_H
#define __DDFT_H

double xx;
#ifndef SWAP
#define SWAP(a, b) xx=(a); (a)=(b); (b)=xx
#endif

#ifndef SWAP2
#define SWAP2(a, b, c) (c)=(a); (a)=(b); (b)=(c)
#endif

void  dft(np, Re, Im)
int   np;
double  *Re[], *Im[];
{
  int      n, i, j, k;
  double   *re, *im, cs, sn, cnst0, cnst1, cnst2;
  float    dnp = np;

  re = (double *)calloc(np, sizeof(double));
  im = (double *)calloc(np, sizeof(double));

  cnst0 = 2.0*M_PI/np;
  for(n=0; n<np; n++)           
  {
    if (n<np/2) j=np/2+n;
    else        j=n-np/2;
    cnst1 = cnst0*n;
    for(k=0; k<np; k++)         
    {
      cnst2 = cnst1*k;
      cs = cos(cnst2);
      sn = sin(cnst2);
      re[j] += (*Re)[k]*cs + (*Im)[k]*sn;
      im[j] += (*Im)[k]*cs - (*Re)[k]*sn; 
    } 
  }
  
  for(i=0; i<np; i++)            /* scale */
  {
    (*Re)[i] = re[i]/dnp;
    (*Im)[i] = im[i]/dnp;   
  }
}

/* -------------------- Pbox FT routines -------------------- */

void re_order(xRe, xIm, np)
int   np;
double *xRe[], *xIm[];
{
  int i, j, k;
  double *tRe, *tIm;

  tRe = (double *) calloc(np, sizeof(double));
  tIm = (double *) calloc(np, sizeof(double));

  k = np/2 + np%2; 
  for(i=0; i<np; i++) 
  {
    j = (i + k)%np; 
    tRe[i] = (*xRe)[j]; tIm[i] = (*xIm)[j];  
  }
  for(i=0; i<np; i++) 
    (*xRe)[i] = tRe[i], (*xIm)[i] = tIm[i];
    
  free(tRe); free(tIm);
}

void re_orderi(xRe, xIm, nn)	        /* wrap 12345000 into 34500012 */
double *xRe[], *xIm[];			/* replaces ft_wrap + re_order for iFFT */
int    nn;
{
int    i, j;
double re0, im0, tm;

  i = nn/2;
  re0 = (*xRe)[i], im0 = (*xIm)[i];
  for(i--, nn--, j=nn-i-1; i>=0; i--, j--, nn--)
  {
    tm = (*xRe)[i], (*xRe)[j] = (*xRe)[nn], (*xRe)[nn] = tm;
    tm = (*xIm)[i], (*xIm)[j] = (*xIm)[nn], (*xIm)[nn] = tm;
  }
  (*xRe)[0] = re0, (*xIm)[0] = im0; 
}

void f_ft(xRe, xIm, np, sign)
int  np;
double *xRe[], *xIm[], sign;
{
  int	  i, j, k, m, istep;	
  double  tRe, tIm, cs, sn, wr, wi, teta, tm;

  j = 0;					/* bit-reversal */
  for (i=0; i<np; i++)		
  {
  
    if (j > i) 	  
    {
      SWAP((*xRe)[j], (*xRe)[i]);
      SWAP((*xIm)[j], (*xIm)[i]); 
    }
    
    m = np >> 1;
    while (m>=2 && j>=m)
    {
      j-=m; 
      m>>=1;
    }
    j += m;
  }
  k = 1;					/* Danielson - Lanczos lemma */
  while (np > k)  
  {
    istep = 2*k;
    teta = sign*M_PI/k;
    cs = cos(teta);
    sn = sin(teta);
    wr = 1.0, wi = 0.0;
    for (m=0; m<k; m++) 
    {
      for (i=m; i<np; i+=istep) 
      {
        j = i + k;
        tRe = wr*(*xRe)[j] + wi*(*xIm)[j];
        tIm = wr*(*xIm)[j] - wi*(*xRe)[j];
        (*xRe)[j] = (*xRe)[i] - tRe;	 
        (*xIm)[j] = (*xIm)[i] - tIm;
        (*xRe)[i] += tRe;
        (*xIm)[i] += tIm;
      }
    wr = (tm=wr)*cs + wi*sn;
    wi = wi*cs - tm*sn;
    }
    k = istep;
  }
}

void d_ft(xRe, xIm, np, sign)				/* discrete FT */
double   *xRe[], *xIm[], sign;
int      np;
{
  int     i, j;
  double   *tRe, *tIm, cs, sn;
  double  cnst0, cnst1, cnst2; 

  tRe = (double *) calloc(np, sizeof(double));
  tIm = (double *) calloc(np, sizeof(double));

  cnst0 = 2.0 * M_PI / (double) np;
  for(i=0; i<np; i++) 	
  { 
    cnst1 = cnst0 * (double) i;
    for(j=0; j<np; j++) 
    { 
      cnst2 = cnst1 * (double) j;
      cs = cos(cnst2);
      sn = sin(cnst2);
      tRe[i] = tRe[i] + (*xRe)[j]*cs + (*xIm)[j]*sn;
      tIm[i] = tIm[i] + (*xIm)[j]*cs - (*xRe)[j]*sn;
    }
  }

    for(i=0; i<np; i++) 
      (*xRe)[i] = tRe[i], (*xIm)[i] = tIm[i];

  if(sign<0.0) 
  {
    for(i=0; i<np; i++) 
      (*xRe)[i] = tRe[i], (*xIm)[i] = tIm[i];
  }
  else
  {
    for(i=0; i<np; i++) 
      (*xRe)[i] = tRe[i], (*xIm)[i] = -tIm[i];
  }

  return;
}

void ft(Re, Im, opt, np)
double   *Re[], *Im[];
char      opt; 
int       np;
{
  if (np < 1) return;
  switch (opt)
  {
    case 'i' :   	     				/* discrete iFT */
      re_orderi(Re, Im, np);
      d_ft(Re, Im, np, -1.0);
      break;
    case 'd' :       			        	/* discrete dFT */
      d_ft(Re, Im, np, 1.0);
      re_order(Re, Im, np);
      break;
    case 'f' : 		 				/* fast dFT */
      f_ft(Re, Im, np, 1.0); 
      re_order(Re, Im, np); 
      break;
    default  :  					/* fast iFT */
      re_orderi(Re, Im, np);
      f_ft(Re, Im, np, -1.0);
  }
}


void ft2d(xrr, xri, xir, xii, xni, xnp, opt)
char     opt;
int      xni, xnp;
double **xrr[xni][xnp], **xri[xni][xnp], **xir[xni][xnp], **xii[xni][xnp];
{
  int i, j;
  double *tRe, *tIm;

  i=xnp;
  if(xni>i) i=xni;
  tRe = (double *)calloc(i, sizeof(double));
  tIm = (double *)calloc(i, sizeof(double));  
  
  for(i=0; i<xnp; i++)  /* iFT in F1 */
  { 
    for(j=0; j<xni; j++) 
      tRe[j] = (**xrr)[j][i], tIm[j] = (**xri)[j][i];  /* F1 traces, rr and ri */
    ft(&tRe, &tIm, opt, xni); 
    for(j=0; j<xni; j++) 
      (**xrr)[j][i] = tRe[j], (**xri)[j][i] = tIm[j];  /* F1 traces, rr and ri */
    
    for(j=0; j<xni; j++) 
      tRe[j] = (**xir)[j][i], tIm[j] = (**xii)[j][i];  /* F1 traces, ir and ii */
    ft(&tRe, &tIm, opt, xni); 
    for(j=0; j<xni; j++) 
      (**xir)[j][i] = tRe[j], (**xii)[j][i] = tIm[j];  /* F1 traces, ir and ii */
  } 
  for(i=0; i<xni; i++)  /* iFT in F2 */
  {
    for(j=0; j<xnp; j++) 
      tRe[j] = (**xrr)[i][j], tIm[j] = (**xir)[i][j];   /* F2 traces, rr and ir */
    ft(&tRe, &tIm, opt, xnp); 
    for(j=0; j<xnp; j++) 
      (**xrr)[i][j] = tRe[j], (**xir)[i][j] = tIm[j];   /* F2 traces, rr and ir */
    
    for(j=0; j<xnp; j++) 
      tRe[j] = (**xri)[i][j], tIm[j] = (**xii)[i][j];   /* F2 traces, ri and ii */
    ft(&tRe, &tIm, opt, xnp); 
    for(j=0; j<xnp; j++) 
      (**xri)[i][j] = tRe[j], (**xii)[i][j] = tIm[j];   /* F2 traces, ri and ii */
  }

  return;
}


void cft1d(xfid, xnp, opt)                  /* xnp # complex pts */
COMPLX *xfid[];
int      xnp;
char     opt;
{
  int     j;
  double *tRe, *tIm;
      
  tRe = (double *)calloc(xnp, sizeof(double));
  tIm = (double *)calloc(xnp, sizeof(double));
  
  for(j=0; j<xnp; j++) 
    tRe[j] = (*xfid)[j].re, tIm[j] = (*xfid)[j].im;     
    
  ft(&tRe, &tIm, opt, xnp);
   
  for(j=0; j<xnp; j++) 
    (*xfid)[j].re = tRe[j], (*xfid)[j].im = tIm[j];     
    
  free(tRe); free(tIm);

  return;
}

void cft1dy(xfid, xnp, opt)  /* xnp # complex pts, rev sign after FT */
COMPLX *xfid[];
int      xnp;
char     opt;
{
  int     j;
  double *tRe, *tIm;
      
  tRe = (double *)calloc(xnp, sizeof(double));
  tIm = (double *)calloc(xnp, sizeof(double));
  
  for(j=0; j<xnp; j++) 
    tRe[j] = (*xfid)[j].re, tIm[j] = (*xfid)[j].im;     
    
  ft(&tRe, &tIm, opt, xnp);
   
  for(j=0; j<xnp; j++) 
    (*xfid)[j].re = tRe[j], (*xfid)[j].im = -tIm[j];  // inverts frq   
    
  free(tRe); free(tIm);

  return;
}

void cft1dx(xfid, xnp, opt)  /* xnp # complex pts, rev sign before FT */
COMPLX *xfid[];
int      xnp;
char     opt;
{
  int     j;
  double *tRe, *tIm;
      
  tRe = (double *)calloc(xnp, sizeof(double));
  tIm = (double *)calloc(xnp, sizeof(double));
  
  for(j=0; j<xnp; j++) 
    tRe[j] = (*xfid)[j].re, tIm[j] = -(*xfid)[j].im;   // inverts time  
    
  ft(&tRe, &tIm, opt, xnp);
   
  for(j=0; j<xnp; j++) 
    (*xfid)[j].re = tRe[j], (*xfid)[j].im = tIm[j];     
    
  free(tRe); free(tIm);

  return;
}

void cft1dz(xfid, xnp, opt)  /* rev sign both before and after FT */       
COMPLX *xfid[];
int      xnp;
char     opt;
{
  int     j;
  double *tRe, *tIm;
      
  tRe = (double *)calloc(xnp, sizeof(double));
  tIm = (double *)calloc(xnp, sizeof(double));
  
  for(j=0; j<xnp; j++) 
    tRe[j] = (*xfid)[j].re, tIm[j] = -(*xfid)[j].im;     
    
  ft(&tRe, &tIm, opt, xnp);
   
  for(j=0; j<xnp; j++) 
    (*xfid)[j].re = tRe[j], (*xfid)[j].im = -tIm[j];     
    
  free(tRe); free(tIm);

  return;
}


void cft2d(xfid, xni, xnp, opt, dim)  /* xni # complx pts; xnp # real pts - as in Vnmr */
char     opt;
int      xni, xnp;
COMPLX **xfid[xni][xnp];      
{
  int i, j, np;
  double *tRe, *tIm;
      
  if((xni==0) || (xnp==0)) 
  {
    printf(" ni = %d np = %d, cft2d aborted.\n", xni, xnp);
    return;
  }
  
  np=0;  
  j = xnp/2;
  if(xni>j) j=xni;   
  tRe = (double *)calloc(j, sizeof(double));  
  tIm = (double *)calloc(j, sizeof(double));
 
  if(dim < 2)
  {
    np = xnp/2; 
    for(i=0; i<np; i++)  /* FT in F1 */
    {   
      for(j=0; j<xni; j++) 
        tRe[j] = (**xfid)[2*j][i].re, tIm[j] = -(**xfid)[2*j+1][i].re;    /* F1 traces, rr and ri */
      ft(&tRe, &tIm, opt, xni); 
      for(j=0; j<xni; j++) 
        (**xfid)[2*j][i].re = tRe[j], (**xfid)[2*j+1][i].re = tIm[j];   /* F1 traces, rr and ri */
      for(j=0; j<xni; j++) 
        tRe[j] = (**xfid)[2*j][i].im, tIm[j] = -(**xfid)[2*j+1][i].im;    /* F1 traces, ir and ii */
      ft(&tRe, &tIm, opt, xni); 
      for(j=0; j<xni; j++) 
        (**xfid)[2*j][i].im = tRe[j], (**xfid)[2*j+1][i].im = tIm[j];   /* F1 traces, ir and ii */
    }
  }
  if((dim==0) || (dim==2))
  {
    for(i=0; i<xni; i++)  /* FT in F2 */
    {
      for(j=0; j<np; j++) 
        tRe[j] = (**xfid)[2*i][j].re, tIm[j] = -(**xfid)[2*i][j].im;     /* F2 traces, rr and ir */
      ft(&tRe, &tIm, opt, np); 
      for(j=0; j<np; j++) 
        (**xfid)[2*i][j].re = tRe[j], (**xfid)[2*i][j].im = tIm[j];     /* F2 traces, rr and ir */    
      for(j=0; j<np; j++) 
        tRe[j] = (**xfid)[2*i+1][j].re, tIm[j] = -(**xfid)[2*i+1][j].im; /* F2 traces, ri and ii */
      ft(&tRe, &tIm, opt, np); 
      for(j=0; j<np; j++) 
        (**xfid)[2*i+1][j].re = tRe[j], (**xfid)[2*i+1][j].im = tIm[j]; /* F2 traces, ri and ii */
    }
  } 
  free(tRe); free(tIm);  

  return;
}


void cft2dx(xfid, xni, xnp, opt, dim)  /* frequency signs reversed */
char     opt;
int      xni, xnp;
COMPLX **xfid[xni][xnp];      
{
  int i, j, np;
  double *tRe, *tIm;
      
  if((xni==0) || (xnp==0)) 
  {
    printf(" ni = %d np = %d, cft2d aborted.\n", xni, xnp);
    return;
  }

  np = 0;    
  j = xnp/2;
  if(xni>j) j=xni;   
  tRe = (double *)calloc(j, sizeof(double));  
  tIm = (double *)calloc(j, sizeof(double));
 
  if(dim < 2)
  {
    np = xnp/2; 
    for(i=0; i<np; i++)  /* FT in F1 */
    {   
      for(j=0; j<xni; j++) 
        tRe[j] = (**xfid)[2*j][i].re, tIm[j] = (**xfid)[2*j+1][i].re;    /* F1 traces, rr and ri */
      ft(&tRe, &tIm, opt, xni); 
      for(j=0; j<xni; j++) 
        (**xfid)[2*j][i].re = tRe[j], (**xfid)[2*j+1][i].re = tIm[j];   /* F1 traces, rr and ri */
      for(j=0; j<xni; j++) 
        tRe[j] = (**xfid)[2*j][i].im, tIm[j] = (**xfid)[2*j+1][i].im;    /* F1 traces, ir and ii */
      ft(&tRe, &tIm, opt, xni); 
      for(j=0; j<xni; j++) 
        (**xfid)[2*j][i].im = tRe[j], (**xfid)[2*j+1][i].im = tIm[j];   /* F1 traces, ir and ii */
    }
  }
  if((dim==0) || (dim==2))
  {
    for(i=0; i<xni; i++)  /* FT in F2 */
    {
      for(j=0; j<np; j++) 
        tRe[j] = (**xfid)[2*i][j].re, tIm[j] = (**xfid)[2*i][j].im;     /* F2 traces, rr and ir */
      ft(&tRe, &tIm, opt, np); 
      for(j=0; j<np; j++) 
        (**xfid)[2*i][j].re = tRe[j], (**xfid)[2*i][j].im = tIm[j];     /* F2 traces, rr and ir */    
      for(j=0; j<np; j++) 
        tRe[j] = (**xfid)[2*i+1][j].re, tIm[j] = (**xfid)[2*i+1][j].im; /* F2 traces, ri and ii */
      ft(&tRe, &tIm, opt, np); 
      for(j=0; j<np; j++) 
        (**xfid)[2*i+1][j].re = tRe[j], (**xfid)[2*i+1][j].im = tIm[j]; /* F2 traces, ri and ii */
    }
  } 
  free(tRe); free(tIm);  

  return;
}


void aft2d(xfid, xni, xnp, opt)  /* xni # complx pts; xnp # real pts */
char     opt;
int      xni, xnp;
float  **xfid[xni][xnp];      
{
  int i, j, np;
  double *tRe, *tIm;

  np = xni/2;   
  tRe = (double *)calloc(np, sizeof(double));  
  tIm = (double *)calloc(np, sizeof(double));
  
  for(i=0; i<xnp; i++)  /* FT in F1 */
  {
    for(j=0; j<np; j++)   /* ni */
      tRe[j] = (**xfid)[2*j][i], tIm[j] = (**xfid)[2*j+1][i];    /* F1 traces, rr and ri */
    ft(&tRe, &tIm, opt, np); 
    for(j=0; j<np; j++) 
      (**xfid)[2*j][i] = tRe[j], (**xfid)[2*j+1][i] = tIm[j];    /* F1 traces, rr and ri */    
  } 
  free(tRe); free(tIm);  

  return;
}



void cft3d(xfid, xni2, xni, xnp, opt)  /* frequency signs reversed */
char      opt;
int       xni2, xni, xnp;
COMPLX ***xfid[xni2][xni][xnp];      
{
  int i, j, k, np, ni, ni2;
  double *tRe, *tIm;
      
  if((xni2==0) || (xni==0) || (xnp==0)) 
  {
    printf("ni2 = %d, ni = %d np = %d, cft3d aborted.\n", xni2, xni, xnp);
    return;
  }

  np = 0;    
  j = xnp/2;
  if(xni>j) j=xni;   
  if(xni2>j) j=xni2;
  tRe = (double *)calloc(j, sizeof(double));  
  tIm = (double *)calloc(j, sizeof(double));
 
  np = xnp/2; ni=2*xni; ni2=2*xni2;
  for(k=0; k<ni2; k++)
  {
    for(i=0; i<np; i++)  /* FT in F2 */
    {   
      for(j=0; j<xni; j++) 
        tRe[j] = (***xfid)[k][2*j][i].re, tIm[j] = (***xfid)[k][2*j+1][i].re;    
      ft(&tRe, &tIm, opt, xni); 
      for(j=0; j<xni; j++) 
        (***xfid)[k][2*j][i].re = tRe[j], (***xfid)[k][2*j+1][i].re = tIm[j];  
      for(j=0; j<xni; j++) 
        tRe[j] = (***xfid)[k][2*j][i].im, tIm[j] = (***xfid)[k][2*j+1][i].im;    
      ft(&tRe, &tIm, opt, xni); 
      for(j=0; j<xni; j++) 
        (***xfid)[k][2*j][i].im = tRe[j], (***xfid)[k][2*j+1][i].im = tIm[j];   
    }
  }

  for(k=0; k<ni2; k++)
  {
    for(i=0; i<ni; i++)  /* FT in F3 */
    {
      for(j=0; j<np; j++) 
        tRe[j] = (***xfid)[k][i][j].re, tIm[j] = (***xfid)[k][i][j].im;     
      ft(&tRe, &tIm, opt, np); 
      for(j=0; j<np; j++) 
        (***xfid)[k][i][j].re = tRe[j], (***xfid)[k][i][j].im = tIm[j];        
    }
  }

  for(i=0; i<ni; i++)  /* FT in F1 */
  {
    for(j=0; j<np; j++)  
    {
      for(k=0; k<xni2; k++) 
        tRe[j] = (***xfid)[2*k][i][j].re, tIm[j] = (***xfid)[2*k+1][i][j].re;     
      ft(&tRe, &tIm, opt, np); 
      for(k=0; k<xni2; k++) 
        (***xfid)[2*k][i][j].re = tRe[j], (***xfid)[2*k+1][i][j].re = tIm[j];       
      for(k=0; k<xni2; k++) 
        tRe[j] = (***xfid)[2*k][i][j].im, tIm[j] = (***xfid)[2*k+1][i][j].im;     
      ft(&tRe, &tIm, opt, np); 
      for(k=0; k<xni2; k++) 
        (***xfid)[2*k][i][j].im = tRe[j], (***xfid)[2*k+1][i][j].im = tIm[j];     
    }    
  }
 
  free(tRe); free(tIm);  

  return;
}


void lbmult(np, fn, at, lb, wx)    /* np and fn is complex np */
int     np, fn;
double  at, lb;
double   *wx[];
{
  int    i, j;
  double step;

  lb *= at;
  if(np>fn) np=fn;                 /* ! make sure fn > 0 */
    
  step = -lb*M_PI/(double) (np-1);
  if(lb > 0.0)
  {
    for(j=0; j<np; j++)
      (*wx)[j] *= exp(j*step); 
  }
  else
  {
    for(j=0, i=np-1; j<np; j++, i--)
      (*wx)[i] *= exp(-j*step); 
  }
  for(j=np; j<fn; j++)  /* zero fill outside the window */
    (*wx)[j] = 0.0;
}

void gfmult(np, fn, at, gf, gfs, wx)   /* exp(-((t-gfs)/gf)2)  */
int    np, fn;
double at, gf, gfs;
double *wx[];
{
  int    j;
  double step, x;

  gfs /= at;  gf/=at;
  if(np>fn) np=fn;                 /* ! make sure fn > 0 */
  if(gfs < 0.0)
    gfs = 0.0;
  if(gfs > 1.0)
    gfs = 1.0;

  gf = 1.0/gf;
  gfs *= gf;
  step = gf/(double) (np-1);
  x = 0.0;
  for(j=0; j<np; j++)
  {
    x = (double) j/(np-1);
    x = gfs - x*gf;
    (*wx)[j] *= exp(-x*x); 
  }
  for(j=np; j<fn; j++)  /* zero fill outside the window */
    (*wx)[j] = 0.0;      
}


void sbmult(np, fn, at, sb, sbs, wx)  /* np and fn is complex np */
int    np, fn;
double at, sb, sbs;
double *wx[];
{
  int    j, np1, np2, nptot, sq=0;
  double x, pos, step;

  sb*=2.0; sb /= at; sbs /= at;  
  if(np>fn) np=fn;                 /* ! make sure fn > 0 */
  
  np1 = (int) (0.5 + sbs*np);   
  nptot = (int) (0.5 + sb*np);  

  if(nptot < 0) 
  {
    sq = 1;
    nptot = -nptot;
  }
  
  np2 = np1 + nptot;

  if ((np1 >= np) || (nptot == 0) || (np2 <= 0)) 
    return;

  if(np2 > np)
    np2 = np;
  
  for(j=np2; j<fn; j++)  /* zero fill outside the window */
    (*wx)[j] = 0.0;

  if (np1 > 0)
  {
    for(j=0; j<np1; j++)   /* zero fill outside the window */
    (*wx)[j] = 0.0;
  }
      
  step = M_PI/(nptot-1);  
  if(np1 < 0) 
  {  
    pos = -np1*step;
    np1 = 0;
  }
  else pos = 0.0;
    
  if(sq)
  {
    for(j=np1; j<np2; j++)
    {
      x = sin(pos);
      x *= x;
      (*wx)[j] *= x; 
      pos += step;
    }
  }
  else
  {
    for(j=np1; j<np2; j++)
    {
      (*wx)[j] *= sin(pos);
      pos += step;
    } 
  }     
}


void wm2d(xpp, xfid, opt)  /* 2d window multiplication */
PROCPAR  xpp;
COMPLX **xfid;
char     opt;  
{
  int i, j, np, ni;
  double *tRe, *tIm;
   
  j = xpp.np/2;
  if(xpp.ni>j) j=xpp.ni;
  tRe = (double *)calloc(j, sizeof(double));
  tIm = (double *)calloc(j, sizeof(double));
  
  np = xpp.np/2; 
  ni = xpp.ni*2;
  for(i=0; i<np; i++)  /* wmult in F1 */
  {
    for(j=0; j<ni; j++) 
      tRe[j] = xfid[j][i].re, tIm[j] = xfid[j][i].im;    
      
    switch(opt)
    {
      case 's' :                      /* sb1 */
        sbmult(ni, ni, xpp.at1, xpp.sb1, xpp.sbs1, &tRe);
        sbmult(ni, ni, xpp.at1, xpp.sb1, xpp.sbs1, &tIm);
      break;

      case 'g' :                      /* gf1 */
        gfmult(ni, ni, xpp.at1, xpp.gf1, xpp.gfs1, &tRe);
        gfmult(ni, ni, xpp.at1, xpp.gf1, xpp.gfs1, &tIm);
      break;
      
      case 'l' :                      /* lb1 */
        lbmult(ni, ni, xpp.at1, xpp.lb1, &tRe);
        lbmult(ni, ni, xpp.at1, xpp.lb1, &tIm);
      break;
    }
    
    for(j=0; j<ni; j++) 
      xfid[j][i].re = tRe[j], xfid[j][i].im = tIm[j];        
  }
  for(i=0; i<ni; i++)  /* FT in F2 */
  {
    for(j=0; j<np; j++) 
      tRe[j] = xfid[i][j].re, tIm[j] = xfid[i][j].im;  
        
    switch(opt)
    {
      case 's' :                      /* sb */
        sbmult(np, np, xpp.at, xpp.sb, xpp.sbs, &tRe);
        sbmult(np, np, xpp.at, xpp.sb, xpp.sbs, &tIm);
      break;

      case 'g' :                      /* gf */
        gfmult(np, np, xpp.at, xpp.gf, xpp.gfs, &tRe);
        gfmult(np, np, xpp.at, xpp.gf, xpp.gfs, &tIm);
      break;
      
      case 'l' :                      /* lb */
        lbmult(np, np, xpp.at, xpp.lb, &tRe);
        lbmult(np, np, xpp.at, xpp.lb, &tIm);
      break;
    }
    
    for(j=0; j<np; j++) 
      xfid[i][j].re = tRe[j], xfid[i][j].im = tIm[j];     
  }

  free(tRe); free(tIm);
  return;
}

void ph0(xRe, xIm, xnp, xph)    /* zero order phase correction - rotation; xph in rad ! */
double  *xRe[], *xIm[], xph;
int      xnp;
{
  int i;
  double  cs, sn, a, b;
  
  cs = cos(xph); sn = sin(xph);
  for(i=0; i<xnp; i++) 	    		    
  { 
    a = (*xRe)[i]*cs - (*xIm)[i]*sn; 
    b = (*xIm)[i]*cs + (*xRe)[i]*sn;  
    (*xRe)[i] = a; (*xIm)[i] = b;
  }
	  
  return;  
}

void ph1(xRe, xIm, xnp, phi)    /* first order phase correction - shift; phi in rad ! */
double  *xRe[], *xIm[], phi;
int      xnp;
{
  int i;
  double  cs, sn, a, b, phx;
  
  phx = 0.0;  
  for(i=0; i<xnp; i++) 	    		    
  { 
    cs = cos(phx); sn = sin(phx);
    a = (*xRe)[i]*cs - (*xIm)[i]*sn; 
    b = (*xIm)[i]*cs + (*xRe)[i]*sn;  
    (*xRe)[i] = a; (*xIm)[i] = b;
    phx += phi;
  }
	  
  return;  
}

void cph0(xfid, xnp, xph)    /* zero order phase correction - rotation; xph in rad */
COMPLX   *xfid[];
double   xph;
int      xnp;
{
  int i;
  double  cs, sn, a, b;
  
  cs = cos(xph); sn = sin(xph);
  for(i=0; i<xnp; i++) 	    		    
  { 
    a = (*xfid)[i].re*cs - (*xfid)[i].im*sn; 
    b = (*xfid)[i].im*cs + (*xfid)[i].re*sn;  
    (*xfid)[i].re = a; (*xfid)[i].im = b;
  }
	  
  return;  
}

void cph1(xfid, xnp, phi)    /* first order phase correction - shift; phi in rad ! */
COMPLX  *xfid[];
double   phi;
int      xnp;
{
  int i;
  double  cs, sn, a, b, phx, ph;
  
  phx = 0.0; ph = phi/(double) xnp;  
  for(i=0; i<xnp; i++) 	    		    
  { 
    cs = cos(phx); sn = sin(phx);
    a = (*xfid)[i].re*cs - (*xfid)[i].im*sn; 
    b = (*xfid)[i].im*cs + (*xfid)[i].re*sn;  
    (*xfid)[i].re = a; (*xfid)[i].im = b;
    phx += ph;
  }
	  
  return;  
}

void aph0(xfid, jx, xnp)    /* zero order phase correction - rotation; xph in rad */
COMPLX   *xfid[];
int      jx, xnp;
{
  double  ph;

  ph = atan2((*xfid)[jx].im, (*xfid)[jx].re);   printf("### ph = %12.2f\n", 180.0*ph/M_PI); 
  cph0(xfid, xnp, -ph);
  	  
  return;  
}

void lsfid(xRe, xIm, xnp, xfq, xsw)    /* frequency shift */
double  *xRe[], *xIm[], xfq;
int      xnp;
{
  double  phi;
    
  phi = 2.0*M_PI*xfq/xsw;                /* calculate the phase increment, rad */
  (void) ph1(xRe, xIm, xnp, -phi);
	  
  return;  
}


int peak(xRe, xIm, xnp)    /* find the maximum peak position in a 1d trace */
double  xRe[], xIm[];
int     xnp;
{
  int     i, ipos;
  double  mx, a;
  
  mx = 0.0; ipos = 0;
  for(i=0; i<xnp; i++)
  {
    a = xRe[i]*xRe[i] + xIm[i]*xIm[i];
    if(a > mx) mx = a, ipos = i;
  }

  return ipos;
}

int cpeak(xfid, xnp)    /* find the maximum peak position in a 1d trace */
COMPLX  *xfid;
int     xnp;
{
  int     i, ipos;
  double  mx, a;
  
  mx = 0.0; ipos = 0;
  for(i=0; i<xnp; i++)
  {
    a = xfid[i].re*xfid[i].re + xfid[i].im*xfid[i].im;
    if(a > mx) mx = a, ipos = i;
  }

  return ipos;
}

void cfrq(xfid, xnp)  /* calculate the instant frequency */
COMPLX  *xfid[];
int     xnp;
{
  int     i;
  
  for(i=0; i<xnp; i++)
  {
    (*xfid)[i].re = atan2((*xfid)[i].re, (*xfid)[i].im);    
    (*xfid)[i].im = 0.0;
  }
  for(i=1; i<xnp; i++)
    (*xfid)[i].re -= (*xfid)[i-1].re;   /* phase change in radians per dwell time */
       
  (*xfid)[0].re = (*xfid)[1].re;
  
  return;
}



void xft(double *xRe[], double *xIm[], double *xcs, double *xsn, int np, double sign)
{
  int	  i, j, k, m, istep;	
  double  tRe, tIm, wr, wi, tm;

  j = 0;					/* bit-reversal */
  for (i=0; i<np; i++)		
  {
  
    if (j > i) 	  
    {
      SWAP((*xRe)[j], (*xRe)[i]);
      SWAP((*xIm)[j], (*xIm)[i]); 
    }   
    m = np >> 1;
    while (m>=2 && j>=m)
    {
      j-=m; 
      m>>=1;
    }
    j += m;
  }
  k = 1;					/* Danielson - Lanczos lemma */
  while (np > k)  
  {
    istep = 2*k;
    wr = 1.0, wi = 0.0;
    for (m=0; m<k; m++) 
    {
      for (i=m; i<np; i+=istep) 
      {
        j = i + k;
        tRe = wr*(*xRe)[j] + wi*(*xIm)[j];
        tIm = wr*(*xIm)[j] - wi*(*xRe)[j];
        (*xRe)[j] = (*xRe)[i] - tRe;	 
        (*xIm)[j] = (*xIm)[i] - tIm;
        (*xRe)[i] += tRe;
        (*xIm)[i] += tIm;
      }
      wr = (tm=wr)*xcs[k] + wi*xsn[k];
      wi = wi*xcs[k] - tm*xsn[k];
    }
    k = istep;
  }
}


void mk_mtx(xcs, xsn, xnp, opt)
double *xcs[], *xsn[];
int     xnp;
char    opt;
{
  int	  k, istep;	
  double  teta, sign;

  sign = 1.0;
  if(opt == 'i') sign = -1.0;
  
  k=1;
  while(k<xnp)  
  {
    istep = 2*k;  
    teta = sign*M_PI/k;
    (*xcs)[k] = cos(teta);
    (*xsn)[k] = sin(teta);
    k = istep;
  }
}

void xfft(Re, Im, xcs, xsn, np, opt)                      /* matrix FFT */
double   *Re[], *Im[], *xcs[], *xsn[];
int       np;
char      opt; 
{
  
  if (np < 1) return;
        
  if(opt == 'i')
  {
    re_orderi(Re, Im, np);
    xft(Re, Im, xcs, xsn, np, -1.0);
  }
  else
  {
    xft(Re, Im, xcs, xsn, np, 1.0);
    re_order(Re, Im, np); 
  }

  return;
}

void scale1dC(xfid, xnp, scf)
int      xnp;
COMPLX  *xfid[xnp];
double   scf;
{
  int i;
  double scf1;

  scf1 = scf;
  if(scf < 0.0) scf1 = -scf;  /* invert frequency if scf < 0.0 */
  for(i=0; i<xnp; i++)            /* scale */
  {
    (*xfid)[i].re *= scf1;
    (*xfid)[i].im *= scf;   
  }

  return;
}

void invert_frq_2dC(xfid, xni, xnp)
int      xni, xnp;
COMPLX **xfid[xni][xnp];
{
  int i, j; 

  for(i=0; i<xni; i++)            /* scale */
  {
    for(j=0; j<xnp; j++)
    {
      (**xfid)[i][j].im = -(**xfid)[i][j].im; 
    }  
  }

  return;
}


void scale2dC(xfid, xni, xnp, scf)
int      xni, xnp;
COMPLX **xfid[xni][xnp];
double   scf;
{
  int i, j; 

  for(i=0; i<xni; i++)            /* scale */
  {
    for(j=0; j<xnp; j++)
    {
      (**xfid)[i][j].re *= scf;
      (**xfid)[i][j].im *= scf; 
    }  
  }

  return;
}

void scale2dR(xfid, xni, xnp, scf)
int      xni, xnp;
double **xfid[xni][xnp], scf;
{
  int i, j; 

  for(i=0; i<xni; i++)            /* scale */
  {
    for(j=0; j<xnp; j++)
    {
      (**xfid)[i][j] *= scf;
    }  
  }

  return;
}


void scale3dC(xfid, xni2, xni, xnp, scf)
int      xni2, xni, xnp;
COMPLX ***xfid[xni2][xni][xnp];
double   scf;
{
  int i, j, k; 

  for(i=0; i<xni2; i++)            /* scale */
  {
    for(j=0; j<xni; j++)
    {
      for(k=0; k<xnp; k++)
      {
        (***xfid)[i][j][k].re *= scf;
        (***xfid)[i][j][k].im *= scf; 
      }  
    }
  }

  return;
}
#endif
