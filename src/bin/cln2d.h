/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* cln2d.h - 2D Compressive Sensing processing routines, 
             Eriks Kupce, Agilent technologies, Oxford, Jan 2011 */ 

double Qval(c1, c2)
COMPLX c1, c2;
{
  return sqrt(c1.re*c1.re + c1.im*c1.im + c2.re*c2.re + c2.im*c2.im);
}


void print_sched(sch, np)   /*  _sched functions to be phased out */
int *sch, np;
{
  int    i;
  
  for(i=0; i<np; i++)
    printf("%4d %8d\n", i, sch[i]);
  
  return;
}

int *get_sched(xfnm, np, rep)
char *xfnm;
int  *np;
short rep;
{
  FILE  *src;
  char   str[MAXSTR];
  int    i, j, *sch;

  strcpy(str, xfnm);
  strcat(str, "/sampling.sch");  if(rep) printf("reading schedule %s ... \n", str); 
  if((src = open_file(str, "r"))==NULL) exit(0);
  
  j=0;
  while(fgets(str, MAXSTR, src))  /* count the number of peaks */
  {
    if((str[0] != '#') && (sscanf(str, "%d", &i))) j++;
  }         
  
  sch = (int *) calloc(j, sizeof(int));
  
  j=0;    
  fseek(src, 0, 0);  
  while(fgets(str, MAXSTR, src))           
  {
    if((str[0] != '#') && (sscanf(str, "%d", &sch[j]))) j++;      
  }  
  fclose(src);
  
  *np = j;  
  if(rep) print_sched(sch, j);
  
  return sch;
}


int Cpeak_1d(xfid, yfid, xcf, jx, xnp, reps)  
COMPLX  *xfid, *yfid;
double   xcf;
int     *jx, xnp;
short    reps;
{
  int    i, k;
  double mx, mn, tm;
  
  k = 0;
  mx = 0.0; mn = 0.0;
  for(i=0; i<xnp; i++)
  {
    tm = Qval(xfid[i], yfid[i]); 
    if(tm > mx) mx = tm, k = i;
    mn += tm;
  }
  mn /= (double) xnp;  
  tm = xcf*mn; 
  if(reps) printf("pos = %4d, amp = %.6f, noise = %.6f, thr = %.6f\n", k, mx, mn, tm);  
  if(mx < tm) 
    return 0;

  *jx = k;  

  return 1;
}

// simple complex data format
int Cpeak_1d_cmplx(xfid, xcf, jx, xnp, reps)
COMPLX *xfid;
double xcf;
int *jx, xnp;
short reps;
{
    int i, k;
    double mx, mn, tm;

    k = 0;
    mx = 0.0; mn = 0.0;
    for(i=0; i<xnp; i++)
    {
        tm=sqrt(xfid[i].re*xfid[i].re + xfid[i].im*xfid[i].im );
        if(tm > mx) mx = tm, k = i;
        mn += tm;
    }
    mn /= (double) xnp;
    tm = xcf*mn;
    if(reps) printf("pos = %4d, amp = %.6f, noise = %.6f, thr = %.6f\n", k, mx, mn, tm);
    if(mx < tm)
    return 0;

    *jx = k;

    return 1;
}

COMPLX *trace_F1(xfid, xni, xnp, xtr)
int       xni, xnp, xtr;
COMPLX  **xfid[xni][xnp];
{
  int      i;
  COMPLX  *f1d;
  
  f1d = calloc_C1d(xni);
  for(i=0; i<xni; i++)
  {
    f1d[i].re = (**xfid)[2*i][xtr].re;
    f1d[i].im = (**xfid)[2*i+1][xtr].re; 
  }
  
  return f1d;
}

// simple complex data format
COMPLX *trace_F1_cmplx(xfid, xni, xnp, xtr)
int xni, xnp, xtr;
COMPLX **xfid[xni][xnp];
{
    int i;
    COMPLX *f1d;

    f1d = calloc_C1d(xni);
    for(i=0; i<xni; i++)
    {
        f1d[i].re = (**xfid)[i][xtr].re;
        f1d[i].im = (**xfid)[i][xtr].im;
    }
    return f1d;
}

COMPLX *trace_F1i(xfid, xni, xnp, xtr)
int       xni, xnp, xtr;
COMPLX  **xfid[xni][xnp];
{
  int      i;
  COMPLX  *f1d;
  
  f1d = calloc_C1d(xni);
  for(i=0; i<xni; i++)
  {
    f1d[i].re = (**xfid)[2*i][xtr].im;
    f1d[i].im = (**xfid)[2*i+1][xtr].im; 
  }
  
  return f1d;
}


void rm_Cpt(bre, bim, f1dx, f1dy, sch, nsp)
COMPLX *bre[], *bim[], *f1dx, *f1dy;
int    *sch, nsp;
{
  int      i, j;

  for(i=0; i<nsp; i++)   /* remove the new component from the exp-FID */
  {
    j = sch[i];        
    (*bre)[j].re -= f1dx[j].re; 
    (*bre)[j].im -= f1dx[j].im; 
    (*bim)[j].re -= f1dy[j].re; 
    (*bim)[j].im -= f1dy[j].im; 
  }    
}

// simple complex data format
void rm_Cpt_cmplx(bre, f1dx, sch, nsp)
COMPLX *bre[], *f1dx;
int *sch, nsp;
{
    int i, j;
    for(i=0; i<nsp; i++) /* remove the new component from the exp-FID */
    {
        j = sch[i];
        (*bre)[j].re -= f1dx[j].re;
        (*bre)[j].im -= f1dx[j].im;
    }
}

void add_pts(f1dr, f1di, fre, fim, xnp)  
COMPLX *f1dr[], *f1di[], *fre, *fim;
int     xnp;
{
  int      i;

  for(i=0; i<xnp; i++)   /* copy the sampled fid-s */
  {
    (*f1dr)[i].re += fre[i].re;
    (*f1dr)[i].im += fre[i].im;
    (*f1di)[i].re += fim[i].re;
    (*f1di)[i].im += fim[i].im;
  }
}

// simple complex data format
void add_pts_cmplx(f1dr, fre, xnp)
COMPLX *f1dr[], *fre;
int xnp;
{
    int i;

    for(i=0; i<xnp; i++) /* copy the sampled fid-s */
    {
        (*f1dr)[i].re += fre[i].re;
        (*f1dr)[i].im += fre[i].im;
    }
}


void add_Cpt(f1dr, f1di, fre, fim)
COMPLX *f1dr, *f1di, fre, fim;
{
  f1dr->re += fre.re; 
  f1dr->im += fre.im; 
  f1di->re += fim.re; 
  f1di->im += fim.im;
}

// simple complex data format
void add_Cpt_cmplx(f1dr, fre)
COMPLX *f1dr, fre;
{
    f1dr->re += fre.re;
    f1dr->im += fre.im;
}

void put_tr_F1(xfid, fre, fim, jx, ni, np)
int       jx, ni, np;
COMPLX  **xfid[2*ni][np], *fre, *fim;
{
  int  i;
  double  scf;
  
  scf = (double) np;

  for(i=0; i<ni; i++)   /* copy the sampled fid-s */
  {
    (**xfid)[2*i][jx].re = fre[i].re/scf; 
    (**xfid)[2*i+1][jx].re = fre[i].im/scf; 
    (**xfid)[2*i][jx].im = fim[i].re/scf; 
    (**xfid)[2*i+1][jx].im = fim[i].im/scf; 
  }
}

// simple complex data format
void put_tr_F1_cmplx(xfid, fre, jx, ni, np)
int       jx, ni, np;
COMPLX  **xfid[2*ni][np], *fre;
{
  int  i;
  double  scf;

  scf = (double) np;

  for(i=0; i<ni; i++)   /* copy the sampled fid-s */
  {
    (**xfid)[i][jx].re = fre[i].re/scf;
    (**xfid)[i][jx].im = fre[i].im/scf;
  }
}

// hyper-complex data format
void zf_C1d(f1dx, f1dy, jx, xnp)
COMPLX  *f1dx[], *f1dy[];
int      jx, xnp;
{
  double a0, a1, b0, b1;

  a0 = (*f1dx)[jx].re; a1 = (*f1dx)[jx].im;
  b0 = (*f1dy)[jx].re; b1 = (*f1dy)[jx].im;

  reset_C1d(f1dx, xnp);
  reset_C1d(f1dy, xnp);

  (*f1dx)[jx].re=a0; (*f1dx)[jx].im=a1;
  (*f1dy)[jx].re=b0; (*f1dy)[jx].im=b1;

  return;
}
// simple complex data format
void zf_C1d_cmplx(f1dx, jx, xnp)
COMPLX  *f1dx[];
int      jx, xnp;
{
  double a0, a1;

  a0 = (*f1dx)[jx].re;
  a1 = (*f1dx)[jx].im;

  reset_C1d(f1dx, xnp);

  (*f1dx)[jx].re=a0;
  (*f1dx)[jx].im=a1;

  return;
}

int clean_1d(xfid, thr, sch, nsp, xni, xnp, jx, dnse, reps)
int    *sch, nsp, xni, xnp, jx;
COMPLX **xfid[xni][xnp];
double  thr;
short   dnse, reps;
{
  COMPLX *fre, *fim, *f1dx, *f1dy, *xre, *xim;
  int     ix, j, np;
  double  a, b;

  a = 1.0/nsp; b = (double) 1.0/xni;  // fn-s ? 

  np = pow2(xni);        /* make sure it is a power of 2 */ 
  fre = calloc_C1d(np);  f1dx = calloc_C1d(np); 
  fim = calloc_C1d(np);  f1dy = calloc_C1d(np);     
  reset_C1d(&fre, np);   reset_C1d(&fim, np);  
    
  xre = trace_F1(xfid, xni, xnp, jx);      // extract Re & Im traces from the sampled data 
  xim = trace_F1i(xfid, xni, xnp, jx);     // *xre[], *xim[]
  j=0;                                          
  while(j < nsp)                      
  { 
    reset_C1d(&f1dx, np);    
    reset_C1d(&f1dy, np);    
    (void) copy_C1d(&f1dx, &xre, xni);   /* make a new copy */
    (void) copy_C1d(&f1dy, &xim, xni);   

    (void) cft1d(&f1dx, np, 'f');  
    (void) cft1d(&f1dy, np, 'f');  

    if(!Cpeak_1d(f1dx, f1dy, thr, &ix, np, 0)) break;  

    (void) add_Cpt(&fre[ix], &fim[ix], f1dx[ix], f1dy[ix]); 
    (void) zf_C1d(&f1dx, &f1dy, ix, np);           

    (void) cft1d(&f1dx, np, 'x');   
    (void) cft1d(&f1dy, np, 'x');         
    scale1dC(&f1dx, np, a);
    scale1dC(&f1dy, np, a);           /* scale here  */

    (void) rm_Cpt(&xre, &xim, f1dx, f1dy, sch, nsp);  /* remove from data */ 

    j++; 
  } 

  (void) cft1d(&xre, np, 'f');    // FT the residuals
  (void) cft1d(&xim, np, 'f');   

  if(!dnse) add_pts(&fre, &fim, xre, xim, np);

  (void) cft1d(&fre, np, 'x'); // invert frq in F1  
  (void) cft1d(&fim, np, 'x');   
  scale1dC(&fre, np, b);        // scale here  
  scale1dC(&fim, np, b); 

  put_tr_F1(xfid, fre, fim, jx, xni, xnp);   // replace the processed FID 

  return j;
}

// clean_1d modified for simple complex data format
int clean_1d_cmplx(xfid, thr, sch, nsp, xni, xnp, jx, dnse, reps)
int *sch, nsp, xni, xnp, jx;
COMPLX **xfid[xni][xnp];
double thr;
short dnse, reps;
{
    COMPLX *fre, *f1dx, *xre;
    int ix, j, np;
    double a, b;

    a = 1.0/nsp; b = (double) 1.0/xni;  // fn-s ?

    np = pow2(xni); /* make sure it is a power of 2 */
    fre = calloc_C1d(np);
    f1dx = calloc_C1d(np);
    reset_C1d(&fre, np);
    xre = trace_F1_cmplx(xfid, xni, xnp, jx); // extract Re & Im traces from the sampled data

    j=0;
    while(j < nsp)
    {
        reset_C1d(&f1dx, np);
        copy_C1d(&f1dx, &xre, xni); /* make a new copy */

        cft1d(&f1dx, np, 'f');
        if(!Cpeak_1d_cmplx(f1dx, thr, &ix, np, 0)) break;

        add_Cpt_cmplx(&fre[ix], f1dx[ix]);

        zf_C1d_cmplx(&f1dx, ix, np);

        cft1d(&f1dx, np, 'x');
        scale1dC(&f1dx, np, a);

        rm_Cpt_cmplx(&xre, f1dx, sch, nsp); /* remove from data */

        j++;
    }

    (void) cft1d(&xre, np, 'f');    // FT the residuals

    if(!dnse) add_pts_cmplx(&fre, xre, np);

    (void) cft1d(&fre, np, 'x');// invert frq in F1
    scale1dC(&fre, np, b);// scale here

    put_tr_F1_cmplx(xfid, fre, jx, xni, xnp);// replace the processed FID

    return j;
}
void ftrs_f2(dfid, xfhd, sch, nsp, rep)  /* FT in F2 */
FILEHEADER   xfhd;
COMPLX      **dfid[xfhd.nblocks][xfhd.np/2];
int          sch[], nsp;
short        rep;
{
  COMPLX      **smp;
  int i, j, k, ni, np;
  
  if(rep)  printf("ft in F2 ... "); 

  ni = xfhd.nblocks/2;
  np = xfhd.np/2;           /* np must be pow2 ! */
  
  if(np != pow2(np)) vn_error("Error: np is not a power of 2 !\n");
  
  smp = calloc_C2d(2*nsp, np);
    
  for(i=0; i<nsp; i++)   /* copy the sampled fid-s */
  {
    k = 2*sch[i];        /* complex data point in F1 */
    for(j=0; j<np; j++)
    {
      smp[2*i][j].re = (**dfid)[k][j].re;
      smp[2*i][j].im = (**dfid)[k][j].im;
      smp[2*i+1][j].re = (**dfid)[k+1][j].re;
      smp[2*i+1][j].im = (**dfid)[k+1][j].im;
    }
    cft1dx(&smp[2*i], np, 'f');  /* direct FT in F2 */
    cft1dx(&smp[2*i+1], np, 'f');
  }
  
  
  k=2*ni;
  for(i=0; i<k; i++)   /* zero all data */
  {
    for(j=0; j<np; j++) (**dfid)[i][j].re = 0.0, (**dfid)[i][j].im = 0.0;
  }
  
  for(i=0; i<nsp; i++)   /* replace the sampled fid-s */
  {
    k = 2*sch[i];        /* complex data point in F1 */
    for(j=0; j<np; j++)
    {
      (**dfid)[k][j].re = smp[2*i][j].re;
      (**dfid)[k][j].im = smp[2*i][j].im;
      (**dfid)[k+1][j].re = smp[2*i+1][j].re;
      (**dfid)[k+1][j].im = smp[2*i+1][j].im;
    }
  }
  if(rep) printf("done...\n");
  
  return;
}  


COMPLX **read_2dfid_zf(dir, xfhd, xbhd1, xbhd2, xzf1, xzf2, p2, rep)   // read 2d fid 
char        *dir;                        // zero fill to the nearest pow 2 if xzf = 1 
FILEHEADER  *xfhd;                       // do not zero fill if xzf < 1 
BLOCKHEADER *xbhd1, *xbhd2;              // otherwise fill to xzf 
int          xzf1, xzf2, *p2; 
short        rep;
{
  FILE     *src;  
  char      fname[MAXSTR];
  int       i, j, zf1, zf2;
  COMPLX  **dfid;
  FILEHEADER   fhd;
  BLOCKHEADER  bhd1, bhd2;

  i=0;
  sprintf(fname, "%s", dir); 
  if((src = open_fid(fname, "r")) == NULL)
    exit(0);

  if(get_fheader(src, &fhd, rep) == 0)        /* read source file header */
    vn_abort(src, "\nfailed to get fid file header \n");

  zf2 = fhd.np/2;
  if(xzf2 == 1) zf2 = pow2(zf2);
  else if(xzf2 > 1) zf2 = pow2(xzf2);

  zf1 = fhd.nblocks*fhd.ntraces;
  if(xzf1 == 1) zf1 = pow2(zf1);
  else if(xzf1 > 1) zf1 = pow2(xzf1);

 *xfhd = fhd;
  dfid = calloc_C2d(zf1, zf2);  
  for(i=0; i<zf1; i++)
  {
    for(j=0; j<zf2; j++)
      dfid[i][j].re=0.0, dfid[i][j].im=0.0;
  } 
   
  get_bheader(src, xbhd1, rep);   /* return the first block headers */ 
  if (fhd.nbheaders > 1)
    get_bheader(src, xbhd2, 0);       
  j = get_fid(src, fhd, &dfid[0]);       /* get the first fid here */
   
  for (i=1; i<fhd.nblocks; i++)
  {
    get_bheader(src, &bhd1, 0);   
    if (fhd.nbheaders > 1)
      get_bheader(src, &bhd2, 0);      
    j = get_fid(src, fhd, &dfid[i]); 
  }        
  fclose(src);  

  j = pow2(fhd.np); *p2 = fhd.np;
  if(j != fhd.np) 
  {
    (void) set_fhd(&fhd, j, 0);
    *p2 = 0;
  }
      
  return dfid;
}  

 
void write_2d(xfn1, xfn2, xfid, rep)  
char     *xfn1, *xfn2;
COMPLX  **xfid;
short     rep;
{
  FILE   *fil;
  int     i;
  FILEHEADER  fhd;
  BLOCKHEADER bhd;

        
  if ((fil = open_fid(xfn1, "r")) == NULL) exit(0);           /* open 2D source file */
  if(!get_fheader(fil, &fhd, 0)) vn_abort(fil, "write_2d: failed to get fid file header.");
  if(!get_bheader(fil, &bhd, 0)) vn_abort(fil, "write_2d: failed to get fid block header.");
  fclose(fil);
  
  fhd.nbheaders = 1;
  if ((fil = open_fid(xfn2, "w")) == NULL) exit(0);           /* open 2D source file */
  if(!put_fheader(fil, &fhd, rep)) vn_abort(fil, "write_2d: failed to write fid file header.");
  
  bhd.lvl = 0.0;  bhd.tlt = 0.0; 	                      /* write out file */ 	
  for (i=0; i<fhd.nblocks; i++)                              
  {  
    bhd.iblock = (long) i + 1; 
    if(!put_bheader(fil, &bhd, 0)) vn_abort(fil, "write_2d: failed to write fid block header.");
    if(!put_fid(fil, fhd, xfid[i])) vn_abort(fil, "write_2d: failed to write fid data."); 
  }  
  fclose(fil);

  return;
}
 

PROCPAR  get_params(xdir)
char  *xdir;
{
  FILE   *src;
  PROCPAR pp;
  char    str[MAXSTR];
  int     i, j,nimax;
  double  tm;
  double csd=0.0;

  (void) reset_procpar(&pp);  
  if((src = open_curpar(xdir, "r")) == NULL) return pp;  
  if(getval_i(src, "np", &j)) pp.np = j;
  if (getval_i(src, "ni", &i) > 0) {
      nimax=0;
      if (getval_i(src, "nimax", &nimax)>0){
          pp.ni=i;
          pp.nimax=nimax;
      }
      else if (getval(src, "CSdensity", &csd) > 0) {
          pp.nimax = i;
          pp.ni = i * csd / 100.0;
      }
  }
  if (getval_i(src, "ni2", &i) > 0) {
      nimax=0;
      if (getval_i(src, "ni2max", &nimax)>0){
          pp.ni2=i;
          pp.ni2max=nimax;
      }
      else if (getval(src, "CSdensity2", &csd) > 0) {
          pp.ni2max = i;
          pp.ni2 = i * csd / 100.0;
      }
  }

  /*
  if(getval(src, "CSdensity", &csd1)>0){
	  if(getval_i(src, "ni", &i)>0)  {
		  csd2=csd1;
		  pp.nimax = i;
		  pp.ni=i*csd1/100.0;
	  }
	  if(getval_i(src, "ni2", &i)>0)  {
		  getval(src, "CSdensity2", &csd2);
		  pp.ni2max = i;
		  pp.ni2=pp.ni2max*csd2/100.0;
	  }
  } else if(getval_i(src, "nimax", &j)>0){
	  if(getval_i(src, "ni", &j)) pp.ni = j;
	  if(getval_i(src, "ni2", &j)) pp.ni2 = j;
	  if(getval_i(src, "nimax", &j)) pp.nimax = j;
	  if(getval_i(src, "ni2max", &j)) pp.ni2max = j;
  }
  else
      vn_error("cln2d: neither CSdensity nor nimax defined");
  */

  if(getval_i(src, "fn", &j)) pp.fn = j;    
  if(getval_i(src, "fn1", &j)) pp.fn1 = j;        
  if(getval_i(src, "fn2", &j)) pp.fn2 = j;        
  if(getval_i(src, "celem", &j)) pp.ct = j;  /* potential problem if ct > 32k */
  if(getval(src, "at", &tm))  pp.at = tm;
  if(getval(src, "sw", &tm))  pp.sw = tm;
  if((getval_s(src, "array", str) < 5) ||
    ((!strcmp(str, "phase")) && (!strcmp(str, "phase2")))) 
    vn_abort(src, "Unexpected array. ");
  fclose(src);

  i=0; j=0;
  if((pp.nimax > pp.ni)   && (pp.ni >1)) j++; else pp.nimax=0; 
  if((pp.ni2max > pp.ni2) && (pp.ni2>1)) j++; else pp.ni2max=0;

  if((pp.ni > 1) && (pp.ni2 > 1)) 
    vn_error("cln2d: does not work with 3D data sets. "); 

  if((pp.ni < 2) && (pp.ni2 < 2)) 
    vn_error("cln2d: does not work with 1D data sets. "); 

  if(pp.fn1 < (2*pp.nimax))  pp.fn1 = (2*pp.nimax);  /* assuming ncyc = 2 */
  if(pp.fn2 < (2*pp.ni2max)) pp.fn2 = (2*pp.ni2max);

  return pp;
}

