/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* cft3d.h - include files for cln3d.c */

short      wm_flg;
double   **w2d;
COMPLX  ***gfid, **fre, **fim, **bre, **bim, **fx1, **fx2, **f2d;


double Q8(c1, c2, c3, c4)
COMPLX c1, c2, c3, c4;
{
  return sqrt(c1.re*c1.re + c1.im*c1.im + c2.re*c2.re + c2.im*c2.im + 
              c3.re*c3.re + c3.im*c3.im + c4.re*c4.re + c4.im*c4.im);
}


void copy_HCplane(ix, xpp)  
int ix;
PROCPAR xpp;
{
  int      i, j, k;

  k = 2*xpp.fn2;
  for(i=0; i<k; i++)
  {  
    for(j=0; j<xpp.fn1; j++)
    {
      fre[i][j].re = 0.0;  
      fre[i][j].im = 0.0;
      fim[i][j].re = 0.0;
      fim[i][j].im = 0.0;
    }
  }
  
  k = 2*xpp.ni2;   
  for(i=0; i<k; i++)
  {  
    for(j=0; j<xpp.ni; j++)
    {
      fre[i][j].re = gfid[2*ix][i][j].re; 
      fre[i][j].im = gfid[2*ix][i][j].im;
      fim[i][j].re = gfid[2*ix+1][i][j].re;
      fim[i][j].im = gfid[2*ix+1][i][j].re;
    }
  }

  return;
}
   

PROCPAR get_par3d(xdir, xdir2, xfhd, xbhd, xncyc, iph, ift, xdim)
char        *xdir, *xdir2;
FILEHEADER  *xfhd;
BLOCKHEADER *xbhd;
int         *xncyc, *iph, ift, xdim;
{
  FILE   *src;
  PROCPAR pp;
  int     i, j, k, nimax;
  double  tm;
  double csd=0;
  char    str[MAXSTR];

  if((src = open_fid(xdir, "r")) == NULL) exit(0);    /* open 2D source file */
  if((!get_fheader(src, xfhd, 1)) || (!get_bheader(src, xbhd, 0)))
    vn_abort(src, "cln3d: Failed to read file header."); 
  fclose(src);      
 
  (void) reset_procpar(&pp); 

  src = open_procpar(xdir, "r");
  if(getval(src, "at", &tm)>0) pp.at = tm;      
  else vn_abort(src, "cln3d: failed to get at. "); 
  if(getval(src, "sw", &tm)>0) pp.sw = tm;      
  else vn_abort(src, "cln3d: failed to get sw. "); 
  if(getval(src, "sw1", &tm)>0) pp.sw1 = tm;      
  if(getval(src, "sw2", &tm)>0) pp.sw2 = tm;    
  if(getval(src, "sw3", &tm)>0) pp.sw3 = tm;      
  if(getval_i(src, "np", &i)>0)  pp.np = i; pp.np = xfhd->np/2; // np is compex
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
      if(nimax){
          pp.at1= pp.ni/pp.sw1;
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
      if(nimax){
          pp.at2= pp.ni2/pp.sw2;
      }
  }
  if (getval_i(src, "ni3", &i) > 0) {
      nimax=0;
      if (getval_i(src, "ni3max", &nimax)>0){
          pp.ni3=i;
          pp.ni3max=nimax;
      }
      else if (getval(src, "CSdensity3", &csd) > 0) {
          pp.ni3max = i;
          pp.ni3 = i * csd / 100.0;
      }
      if(nimax){
          pp.at3= pp.ni3/pp.sw3;
      }
  }

  if(getval_i(src, "fn", &i)>0) pp.fn = i; pp.fn=pow2(pp.np); // not as in VJ ! 
  if(getval_i(src, "fn1", &i)>0) pp.fn1 = i/2;                // not as in VJ !
  if(getval_i(src, "fn2", &i)>0) pp.fn2 = i/2;                // not as in VJ !
  if(getval(src, "celem", &tm)>0) pp.ct = (int) (tm + 0.5);     
  if(getval(src, "lb1", &tm)>0) pp.lb1 = tm;      
  if(getval(src, "sb1", &tm)>0) pp.sb1 = tm;    
  if(getval(src, "sbs1", &tm)>0) pp.sbs1 = tm;    
  if(getval(src, "gf1", &tm)>0) pp.gf1 = tm;    
  if(getval(src, "gfs1", &tm)>0) pp.gfs1 = tm;    
  if(getval(src, "lb2", &tm)>0) pp.lb2 = tm;    
  if(getval(src, "sb2", &tm)>0) pp.sb2 = tm;    
  if(getval(src, "sbs2", &tm)>0) pp.sbs2 = tm;    
  if(getval(src, "gf2", &tm)>0) pp.gf2 = tm;    
  if(getval(src, "gfs2", &tm)>0) pp.gfs2 = tm;    
  if(getval_s(src, "array", str)>0)
  {
    if(strcmp(str, "phase,phase2") == 0) *iph=12;
    else if(strcmp(str, "phase2,phase") == 0) *iph=21;
    else vn_abort(src, "Unexpected array. ");
  }
  else vn_abort(src, "Problem retrieving array."); 
  fclose(src);  
     
  k=0; i=0;                            // check if the data is inflated 
  if((pp.ni>1)  && (pp.nimax>pp.ni))   i=1, pp.at = pp.nimax/pp.sw1;   
  if((pp.ni2>1) && (pp.ni2max>pp.ni2)) i=1, pp.at2 = pp.ni2max/pp.sw2;
  if((pp.ni3>1) && (pp.ni3max>pp.ni3)) i=1, pp.at3 = pp.ni3max/pp.sw3;
  //if(i) vn_error("cln3d: data not inflated. ");

  if(pp.ni >1) k++; else pp.ni=1;   
  if(pp.ni2>1) k++; else pp.ni2=1;
  if(pp.ni3>1) k++; else pp.ni3=1; 
  
  if(pp.fn1 < pp.ni)  pp.fn1=pow2(pp.ni);     // fn1 is complex
  if(pp.fn2 < pp.ni2) pp.fn2=pow2(pp.ni2);    // fn2 is complex
  pp.nimax=pow2(pp.nimax);                    // make sure nimax is pow2
  pp.ni2max=pow2(pp.ni2max);
  if(!k) vn_error("cln3d: No sparse dimensions found."); 
  if(k > 2)
  {
    char str2[MAXSTR];
    sprintf(str2,"cln3d: Dimensionality > 3. %d %d %d", pp.ni, pp.ni2, pp.ni3);
    vn_error(str2);
  }
  if(k != xdim) vn_error("cln3d: Inconsistent sampling schedule"); 

  i = 1; if(pp.ni>1) i*=2; if(pp.ni2>1) i*=2; if(pp.ni3>1) i*=2; // assuming all HC
  *xncyc = i;

  if(pp.ct < (*xncyc*pp.ni*pp.ni2)) pp.ni2 = pp.ct/(*xncyc*pp.ni);

  i = -1; k = 2*pp.np;                     
  if(ift) k = 2*pp.fn;                                // fix fhd here   
  if(xfhd->np != k) i = k; 
  if(ift) k =  *xncyc*pp.nimax*pp.ni2max;   
  else k =  *xncyc*pp.fn1*pp.fn2; 
  if(k == xfhd->nblocks) k = -1;
  if((i>0) || (k>0)) set_fhd(xfhd, i, k);  
                 
  mk_dir(xdir2); 
  copy_par(xdir, xdir2);                   

  if(ift) j = pp.nimax, k = pp.ni2max;
  else j = pp.fn1, k = pp.fn2;
  if(j == pp.ni) j = -1;
  if(k == pp.ni2) k = -1;
  if((i>0) || (j>0) || (k>0)) 
    fix_np(xdir2, i, j, k, 0);       // fix procpar here  np, ni, ni2
                                          
  return pp;
}

int  check_mem_size(xpp, xnpl, xnplmin, xnplmax, ncyc, reps)
PROCPAR  xpp;
unsigned long  *xnpl, *xnplmin, *xnplmax;
int             ncyc, reps;
{
  unsigned long  np, nb, npl, nplmin, nplmax;
  short          main_buffers, buf2d, wm_buf, ncyc_buf;
  double         pl_size;

  main_buffers = 1; buf2d = 6; wm_buf = 1; ncyc_buf = ncyc; np = 2*xpp.np; // np is real
  pl_size =  main_buffers*ncyc*xpp.fn1*xpp.fn2*sizeof(COMPLX);  
                      // printf("pl_size = %.0f \n", pl_size);                                                                                 
  npl = (unsigned long) (0.8*available_RAM()/pl_size) - buf2d - ncyc_buf - wm_buf; 
                      // printf("can accommodate @ 80% RAM complx npl = %lu\n", npl/2);
  nb = (unsigned long) (0.5 + ceil((double) np/npl));  
                      // printf(" required number of blocks = %lu\n", nb);
  npl = (unsigned long) np/nb;        
                      // printf(" # of complex planes per block = %lu\n", npl/2);
  npl/=2; npl *=2;    // make sure npl is an even number as we need both .re and .im
  if(npl<1) vn_error("Insufficient memory. ");  

  nplmax = npl;    
  nplmin = np - nb*npl;   
  if(nplmin > 0) nb++;  
  else nplmin += npl;  

  *xnpl = npl/2; *xnplmax = nplmax/2; *xnplmin = nplmin/2; // to use complex data 
                                                           // .re and .im 
  gfid = calloc_C3d(2*npl, 2*xpp.fn2, xpp.fn1);
  bre = calloc_C2d(2*xpp.fn2, xpp.fn1); bim = calloc_C2d(2*xpp.fn2, xpp.fn1);  // pow 2  
  fre = calloc_C2d(2*xpp.fn2, xpp.fn1); fim = calloc_C2d(2*xpp.fn2, xpp.fn1);  // pow 2  
  fx1 = calloc_C2d(2*xpp.fn2, xpp.fn1); fx2 = calloc_C2d(2*xpp.fn2, xpp.fn1);  // pow 2  
  f2d = calloc_C2d(ncyc, xpp.fn);                                              // pow 2  

  if(reps) 
  {
    printf("n_b = %lu, npl = %lu, nplmin = %lu, nplmax = %lu\n", nb, npl/2, nplmin/2, nplmax/2);  
    printf(" %lu blocks of %lu complex planes each\n", nb, npl/2);  
  }

  return (int) nb;
}


short **get_sched(xfnm, xnb, xdim, reps)
char   *xfnm;
int    *xnb, *xdim, reps;
{
  FILE    *src;
  char     cmd[MAXSTR];
  short  **CSsch, s[4];
  int      i, j, k, nb, xd;

  if(reps) printf("reading schedule %s ... \n", xfnm);

  strcpy(cmd, xfnm);
  strcat(cmd, "/sampling.sch"); 
  if((src = fopen(cmd, "r")) == NULL)
    vn_error("Error: failed to read the sampling schedule. "); 

  fgets(cmd, 512, src);  
  xd = sscanf(cmd, "%hd %hd %hd", &s[0], &s[1], &s[2]);
  if(xd < 1) 
    vn_abort(src, "Error: failed to read the sampling schedule. ");

  k=1; j=xd;  
  while((fgets(cmd, 512, src)) && (j==xd)) 
  { 
    j = sscanf(cmd, "%hd %hd %hd", &s[0], &s[1], &s[2]);  
    k++;
  }
  *xnb = k; nb = k;  

  CSsch = (short **) calloc(xd, sizeof(short *));
  for(i=0; i<xd; i++)
    CSsch[i] = (short *) calloc(nb+1, sizeof(short ));
  
  i=0; k=xd;
  fseek(src, 0, 0);
  while((fgets(cmd, 512, src)) && (i<nb) && (k==xd))
  { 
    k = sscanf(cmd, "%hd %hd %hd", &s[0], &s[1], &s[2]);
    for(j=0; j<xd; j++) CSsch[j][i] = s[j];
    i++;
  }
  fclose(src);
  
  *xdim = xd;
  if(reps) printf("number of sheduled points : %d\n", i);

  return CSsch;
}


void xtr_f3(xnp, ix, jx, iph)   // xnp is complex np
int         xnp, ix, jx, iph;
{
  int  i;   
                
  if(iph==12)
  {
    for(i=0; i<xnp; i++)
    {
      f2d[0][i].re = gfid[2*i][2*ix][jx].re;   f2d[0][i].im = gfid[2*i+1][2*ix][jx].re;   /* ph = 1, ph2 = 1 */
      f2d[1][i].re = gfid[2*i][2*ix+1][jx].re; f2d[1][i].im = gfid[2*i+1][2*ix+1][jx].re; /* ph = 1, ph2 = 2 */
      f2d[2][i].re = gfid[2*i][2*ix][jx].im;   f2d[2][i].im = gfid[2*i+1][2*ix][jx].im;   /* ph = 2, ph2 = 1 */
      f2d[3][i].re = gfid[2*i][2*ix+1][jx].im; f2d[3][i].im = gfid[2*i+1][2*ix+1][jx].im; /* ph = 2, ph2 = 2 */
    }
  }
  else       /* phase2, phase */
  {
    for(i=0; i<xnp; i++)
    {
      f2d[0][i].re = gfid[2*i][2*ix][jx].re;   f2d[0][i].im = gfid[2*i+1][2*ix][jx].re;   /* ph = 1, ph2 = 1 */  
      f2d[1][i].re = gfid[2*i][2*ix][jx].im;   f2d[1][i].im = gfid[2*i+1][2*ix][jx].im;   /* ph = 2, ph2 = 1 */
      f2d[2][i].re = gfid[2*i][2*ix+1][jx].re; f2d[2][i].im = gfid[2*i+1][2*ix+1][jx].re; /* ph = 1, ph2 = 2 */
      f2d[3][i].re = gfid[2*i][2*ix+1][jx].im; f2d[3][i].im = gfid[2*i+1][2*ix+1][jx].im; /* ph = 2, ph2 = 2 */
    }
  }

  return;
}


/* read and transpose a 3D block of planes */
          
int get3d_f1f2(src, xnp, xni, ipos, iph, rep)  
FILE        *src; 
int          xnp, xni, ipos, iph, rep;
{
  int         i, j, k, m, np, nb, ncyc;
  FILEHEADER  fhd;
  BLOCKHEADER bhd;
  
  fseek(src, 0, 0);
  if(!get_fheader(src, &fhd, rep)) 
    vn_abort(src, "cln3d: Failed to read file header."); 

  ncyc = 4;                    /* assuming ncyc = 4 & hyper-complex in both dimensions */
  nb = (int) fhd.nblocks/ncyc;  
  np = (int) fhd.np/2;

  for(i=0; i<nb; i++)      
  {   
    for(j=0; j<ncyc; j++)
    {
      get_bheader(src, &bhd, 0);      
      if(fhd.nbheaders > 1) get_bheader(src, &bhd, 0);       
      if((get_fid(src, fhd, &f2d[j])) < np) break;             
    }
    if(j<ncyc) vn_abort(src, "\nfailed to get fid file header \n"); 

    j = i%xni; m = i/xni; 

    if(iph==12)  
    {
      for(k=0; k<xnp; k++)
      {  
        gfid[2*k][2*m][j].re   = f2d[0][ipos+k].re; gfid[2*k+1][2*m][j].re   = f2d[0][ipos+k].im; /* ph=1, ph2=1 */
        gfid[2*k][2*m+1][j].re = f2d[1][ipos+k].re; gfid[2*k+1][2*m+1][j].re = f2d[1][ipos+k].im; /* ph=1, ph2=2 */
        gfid[2*k][2*m][j].im   = f2d[2][ipos+k].re; gfid[2*k+1][2*m][j].im   = f2d[2][ipos+k].im; /* ph=2, ph2=1 */
        gfid[2*k][2*m+1][j].im = f2d[3][ipos+k].re; gfid[2*k+1][2*m+1][j].im = f2d[3][ipos+k].im; /* ph=2, ph2=2 */
      } 
    }
    else
    {
      for(k=0; k<xnp; k++)
      {  
        gfid[2*k][2*m][j].re   = f2d[0][ipos+k].re; gfid[2*k+1][2*m][j].re   = f2d[0][ipos+k].im; /* ph=1, ph2=1 */
        gfid[2*k][2*m][j].im   = f2d[1][ipos+k].re; gfid[2*k+1][2*m][j].im   = f2d[1][ipos+k].im; /* ph=2, ph2=1 */
        gfid[2*k][2*m+1][j].re = f2d[2][ipos+k].re; gfid[2*k+1][2*m+1][j].re = f2d[2][ipos+k].im; /* ph=1, ph2=2 */
        gfid[2*k][2*m+1][j].im = f2d[3][ipos+k].re; gfid[2*k+1][2*m+1][j].im = f2d[3][ipos+k].im; /* ph=2, ph2=2 */
      }        
    }  
    printf("blocks read\r%-5d ", 4*i+4);     /* bhd.iblock */
  }
  printf("blocks read\n");

  if(i<nb) return 0;
     
  return 1;
}  


int  HCpeak_2d(xfre, xfim, xcf, xni, xnp, xscf, i1, i2, rep) 
COMPLX    **xfre, **xfim;
double    xcf, xscf;  
int       xni, xnp, *i1, *i2, rep;
{
  int    i, j, ix, jx;
  double mx, tm, mn;
  
  mx=0.0; mn=0.0; 
  j=0; ix=0; jx=0;
  for(i=0; i<xni; i++)
  {
    for(j=0; j<xnp; j++)
    {
      tm = Q8(xfre[2*i][j], xfre[2*i+1][j], xfim[2*i][j], xfim[2*i+1][j]); 
      mn += tm;
      if(tm > mx) mx=tm, ix=i, jx=j;
    }
  } 
  mn = mn/xnp/xni; *i1=ix, *i2=jx;
  tm = mn*xcf;
        
  if(rep) 
    printf("%4d pk at %4d %4d, amp = %.6f, noise = %.6f, thr = %12.6f\n", rep, ix, jx, mx, mn, tm); 
     
  if(mx < tm) 
    return 0;

  return 1;
}


void rm_HCpt(sch, nsp)  /* xni as in VJ */
short  **sch;
int      nsp;
{
  int      i, ii, jj; 

  for(i=0; i<nsp; i++)   /* remove the new component from the exp-FID */
  {                                               
    jj = sch[0][i]; ii = sch[1][i];       
    fre[2*ii][jj].re -= fx1[2*ii][jj].re; 
    fre[2*ii][jj].im -= fx1[2*ii][jj].im; 
    fre[2*ii+1][jj].re -= fx1[2*ii+1][jj].re; 
    fre[2*ii+1][jj].im -= fx1[2*ii+1][jj].im; 

    fim[2*ii][jj].re -= fx2[2*ii][jj].re; 
    fim[2*ii][jj].im -= fx2[2*ii][jj].im; 
    fim[2*ii+1][jj].re -= fx2[2*ii+1][jj].re; 
    fim[2*ii+1][jj].im -= fx2[2*ii+1][jj].im; 
  }    
  
  return;
}


void add_buff(xni, xnp, dnse)
int   xni, xnp;
short dnse;
{
  int      i, j;

  for(i=0; i<xni; i++)   /* add the residual signal to the clead-FID */
  {
    for(j=0; j<xnp; j++)   
    {
      bre[2*i][j].re += fre[2*i][j].re; 
      bre[2*i][j].im += fre[2*i][j].im; 
      bre[2*i+1][j].re += fre[2*i+1][j].re; 
      bre[2*i+1][j].im += fre[2*i+1][j].im; 

      bim[2*i][j].re += fim[2*i][j].re; 
      bim[2*i][j].im += fim[2*i][j].im; 
      bim[2*i+1][j].re += fim[2*i+1][j].re; 
      bim[2*i+1][j].im += fim[2*i+1][j].im; 
    }
  }    
    
  return;
}


void put_tr_F1F2(xni, xnp, ix) 
int  xni, xnp, ix;
{
  int      i, j;

  for(i=0; i<xni; i++)   /* replace the data with the cleaned FID */
  {
    for(j=0; j<xnp; j++)   
    {
      gfid[2*ix][2*i][j].re = bre[2*i][j].re; 
      gfid[2*ix][2*i][j].im = bre[2*i][j].im; 
      gfid[2*ix][2*i+1][j].re = bre[2*i+1][j].re; 
      gfid[2*ix][2*i+1][j].im = bre[2*i+1][j].im; 

      gfid[2*ix+1][2*i][j].re = bim[2*i][j].re; 
      gfid[2*ix+1][2*i][j].im = bim[2*i][j].im; 
      gfid[2*ix+1][2*i+1][j].re = bim[2*i+1][j].re; 
      gfid[2*ix+1][2*i+1][j].im = bim[2*i+1][j].im; 
    }
  }    
  
  return;
}


void add_HCpt(b1, b2, a1, a2)
COMPLX *b1, *b2, a1, a2;
{
  b1->re += a1.re;  b1->im += a1.im;
  b2->re += a2.re;  b2->im += a2.im;
}


void add_HCpts(i1, i2)
int i1, i2;
{
  int k;

  k=2*i1;   add_HCpt(&bre[k][i2], &bim[k][i2], fx1[k][i2], fx2[k][i2]);
  k=2*i1+1; add_HCpt(&bre[k][i2], &bim[k][i2], fx1[k][i2], fx2[k][i2]);

}

void zf_HC2d(xni, xnp, ix, jx)           
int  xni, xnp, ix, jx;
{
  double a1, a2, a3, a4, b1, b2, b3, b4;

  a1 = fx1[2*ix][jx].re; 
  a2 = fx1[2*ix][jx].im; 
  a3 = fx1[2*ix+1][jx].re; 
  a4 = fx1[2*ix+1][jx].im; 

  b1 = fx2[2*ix][jx].re; 
  b2 = fx2[2*ix][jx].im; 
  b3 = fx2[2*ix+1][jx].re; 
  b4 = fx2[2*ix+1][jx].im; 

  reset_C2d(&fx1, 2*xni, xnp); 
  reset_C2d(&fx2, 2*xni, xnp); 

  fx1[2*ix][jx].re = a1; 
  fx1[2*ix][jx].im = a2; 
  fx1[2*ix+1][jx].re = a3; 
  fx1[2*ix+1][jx].im = a4; 

  fx2[2*ix][jx].re = b1; 
  fx2[2*ix][jx].im = b2; 
  fx2[2*ix+1][jx].re = b3; 
  fx2[2*ix+1][jx].im = b4; 
  
  return;
}


void wm_2d(xni, xnp, xfid)
int xni, xnp;
COMPLX  **xfid[2*xni][xnp];
{
  int i, j;

  for(i=0; i<xni; i++)
  {
    for(j=0; j<xnp; j++)
    {
      (**xfid)[2*i][j].re *= w2d[i][j];
      (**xfid)[2*i][j].im *= w2d[i][j];
      (**xfid)[2*i+1][j].re *= w2d[i][j];
      (**xfid)[2*i+1][j].im *= w2d[i][j];
    }
  }

  return;
}


int clean_2d(xpp, xcf, sch, nsp, ix, ift, dnse, rep) 
PROCPAR  xpp;
double   xcf;
short  **sch, dnse, rep;
int      nsp, ix, ift; 
{
  int     jj, i1, i2;
  double  a, b;

  a = 1.0/nsp; b = (double) 1.0/(xpp.ni2*xpp.ni);  // fn-s ? 

  reset_C2d(&bre, 2*xpp.fn2, xpp.fn1); 
  reset_C2d(&bim, 2*xpp.fn2, xpp.fn1); 
  reset_C2d(&fre, 2*xpp.fn2, xpp.fn1); 
  reset_C2d(&fim, 2*xpp.fn2, xpp.fn1); 

  copy_HCplane(ix, xpp); 

  jj=0;  
  while(jj < nsp)                    
  { 
    reset_C2d(&fx1, 2*xpp.fn2, xpp.fn1); 
    reset_C2d(&fx2, 2*xpp.fn2, xpp.fn1); 
    copy_C2d(&fx1, fre, 2*xpp.ni2max, xpp.nimax); 
    copy_C2d(&fx2, fim, 2*xpp.ni2max, xpp.nimax);  

    cft2dx(&fx1, xpp.ni2max, 2*xpp.nimax, 'f', 0);  // Re
    cft2dx(&fx2, xpp.ni2max, 2*xpp.nimax, 'f', 0);  // Im

    if(!HCpeak_2d(fx1, fx2, xcf, xpp.ni2max, xpp.nimax, a, &i1, &i2, 0)) break;   

    (void) add_HCpts(i1, i2);
    (void) zf_HC2d(xpp.ni2max, xpp.nimax, i1, i2);           
    
    (void) cft2dx(&fx1, xpp.ni2max, 2*xpp.nimax, 'x', 0);  
    (void) cft2dx(&fx2, xpp.ni2max, 2*xpp.nimax, 'x', 0); 
    (void) scale2dC(&fx1, 2*xpp.ni2max, xpp.nimax, a);
    (void) scale2dC(&fx2, 2*xpp.ni2max, xpp.nimax, a);

    rm_HCpt(sch, nsp); 
     
    jj++;
  }

  cft2dx(&fre, xpp.ni2max, 2*xpp.nimax, 'f', 0); // FT the residuals
  cft2dx(&fim, xpp.ni2max, 2*xpp.nimax, 'f', 0);  

  add_buff(xpp.ni2max, xpp.nimax, dnse); 

  cft2dx(&bre, xpp.ni2max, 2*xpp.nimax, 'x', 0);  
  cft2dx(&bim, xpp.ni2max, 2*xpp.nimax, 'x', 0); 
  scale2dC(&bre, 2*xpp.ni2max, xpp.nimax, b);
  scale2dC(&bim, 2*xpp.ni2max, xpp.nimax, b);
  
  if(ift)
    put_tr_F1F2(xpp.ni2max, xpp.nimax, ix);   
  else                                           // reprocess 
  {
    if(wm_flg) 
    {
      wm_2d(xpp.ni2max, xpp.nimax, &bre); 
      wm_2d(xpp.ni2max, xpp.nimax, &bim);
    }
    cft2d(&bre, xpp.fn2, 2*xpp.fn1, 'f', 0);    // zero fill and FT
    cft2d(&bim, xpp.fn2, 2*xpp.fn1, 'f', 0);  
    put_tr_F1F2(xpp.fn2, xpp.fn1, ix);   
  }
 
  return jj;
}


void save_3dblock(xfnm, xfhd, xnp, xni, iph)     // get inflated F1F2 plane for phase,phase2 
char        *xfnm; 
FILEHEADER   xfhd;
int          xnp, xni, iph;
{
  FILE  *trg;
  int    i, j, k, m, nb, ncyc;
  float  tmp;
  
  if((trg = open_file(xfnm, "w")) == NULL) exit(1);

  ncyc = 4;                 /* assuming hyper-complex in both dimensions */
  nb = (int) xfhd.nblocks/ncyc;
  for(i=0; i<nb; i++)      
  {   
    j = i%xni; m = i/xni; 
    for(k=0; k<xnp; k++)
    {  
      tmp = (float) gfid[2*k][2*m][j].re;   fwrite(&tmp, sizeof(float), 1, trg);   /* ph=1, ph2=1 */
      tmp = (float) gfid[2*k+1][2*m][j].re; fwrite(&tmp, sizeof(float), 1, trg);
    }
    if(iph==12)
    {
      for(k=0; k<xnp; k++)
      {  
        tmp = (float) gfid[2*k][2*m+1][j].re;   fwrite(&tmp, sizeof(float), 1, trg); /* ph=1, ph2=2 */
        tmp = (float) gfid[2*k+1][2*m+1][j].re; fwrite(&tmp, sizeof(float), 1, trg);
      }
      for(k=0; k<xnp; k++)
      {  
        tmp = (float) gfid[2*k][2*m][j].im;   fwrite(&tmp, sizeof(float), 1, trg);   /* ph=2, ph2=1 */
        tmp = (float) gfid[2*k+1][2*m][j].im; fwrite(&tmp, sizeof(float), 1, trg);
      }
    }
    else
    {
      for(k=0; k<xnp; k++)
      {  
        tmp = (float) gfid[2*k][2*m][j].im;   fwrite(&tmp, sizeof(float), 1, trg);   /* ph=2, ph2=1 */
        tmp = (float) gfid[2*k+1][2*m][j].im; fwrite(&tmp, sizeof(float), 1, trg);
      }
      for(k=0; k<xnp; k++)
      {  
        tmp = (float) gfid[2*k][2*m+1][j].re;   fwrite(&tmp, sizeof(float), 1, trg); /* ph=1, ph2=2 */
        tmp = (float) gfid[2*k+1][2*m+1][j].re; fwrite(&tmp, sizeof(float), 1, trg);
      }
    }
    for(k=0; k<xnp; k++)
    {  
      tmp = (float) gfid[2*k][2*m+1][j].im;   fwrite(&tmp, sizeof(float), 1, trg); /* ph=2, ph2=2 */
      tmp = (float) gfid[2*k+1][2*m+1][j].im; fwrite(&tmp, sizeof(float), 1, trg);
    }
  }  
  fclose(trg);
   
  return;
}  


void write_tr3d(xdir, xfhd, xbhd, pp, nb, ncyc, ift, iph, nplmin, nplmax, reps)
char         *xdir;
FILEHEADER    xfhd;
BLOCKHEADER  *xbhd;
PROCPAR       pp;
int           nb, ncyc, ift, iph, reps;
unsigned long nplmin, nplmax;
{
  FILE  **s, *trg;
  COMPLX *tr;
  char    str[MAXSTR];
  int     i, j, k, ni, ni2;
  float   tre, tim;
  unsigned long npl, nn, ix;

 
  printf(" cln3d: Saving data in %s...\n", xdir); 
  if((trg = open_fid(xdir, "w")) == NULL) exit(1);
  if(!put_fheader(trg, &xfhd, 1)) 
    vn_abort(trg, "cln3d: Failed to write file header."); 

  if(ift) ni = pp.nimax, ni2 = pp.ni2max;  
  else    ni = pp.fn1,   ni2 = pp.fn2;

  ix = 1;
  npl = nplmax;  
  if(nb > 1)
  {
    tr = calloc_C1d(pp.fn);  
    s = (FILE **) calloc(nb, sizeof(FILE *));
    for(i=0; i<nb; i++)            /* the loop starts here */
    {
      sprintf(str, "%s/tmp_%d", xdir, i);
      if((s[i] = open_file(str, "r")) == NULL) vn_abort(trg, "...");   
    }

    nn = ni*ni2*ncyc;      
    for(i=0; i<nn; i++)
    {    
      npl=nplmax;                        
      for(j=0; j<nb; j++)
      {                 
        if((j==(nb-1)) && (npl > nplmin))  /* last block */
          npl = nplmin;
        for(k=0; k<npl; k++)
        {
          if((!(fread(&tre, sizeof(float), 1, s[j]))) || 
             (!(fread(&tim, sizeof(float), 1, s[j]))))
            break;
          tr[j*nplmax+k].re = (double) tre;
          tr[j*nplmax+k].im = (double) tim;
        }
        if(k<npl) break;
      }
      if(j<nb) break;
      if(ift) cft1dz(&tr, pp.fn, 'x');    /* must be a pow2 */

      if((!put_bheader(trg, xbhd, 0)) || (!put_fid(trg, xfhd, tr))) break;
      printf("blocks saved\r%-5ld ", ix);      
      xbhd->iblock++; ix++;    
    }
    if(i<nn) printf("cln3d: failed to complete.\n");
    for(i=0; i<nb; i++) 
    {
      fclose(s[i]); 
      sprintf(str, "%s/tmp_%d", xdir, i); 
      remove(str);
    }
  }
  else
  {
    for(i=0; i<ni2; i++)
    {
      for(j=0; j<ni; j++)
      {
        (void) xtr_f3(pp.np, i, j, iph);    // all complex  
        if(ift)
        {
          for(k=0; k<ncyc; k++)
            cft1dz(&f2d[k], pp.fn, 'x');   // fn is complex
        } 
        for(k=0; k<ncyc; k++)
        {     
          if((!put_bheader(trg, xbhd, 0)) ||
             (!put_fid(trg, xfhd, f2d[k]))) break;
          xbhd->iblock++;
        }
      }
    }
  } 
  fclose(trg);   

}


void wm_mtx(xpp)
PROCPAR  xpp;
{
  int      i, j;
  double  *w2, tm; 
  
  tm  = fabs(xpp.lb1) + fabs(xpp.gf1) + fabs(xpp.sb1);
  tm += fabs(xpp.lb2) + fabs(xpp.gf2) + fabs(xpp.sb2);  
  wm_flg = 1; 
  if(tm==0.0) 
  {
    wm_flg=0; 
    return;
  }                    

  w2 = calloc_1d(xpp.ni2max);
  w2d = calloc_2d(xpp.ni2max, xpp.nimax);
  for(i=0; i<xpp.ni2max; i++) w2[i] = 1.0;
  for(j=0; j<xpp.nimax;  j++) w2d[0][j] = 1.0;

  if(xpp.lb1 != 0.0) lbmult(xpp.nimax, xpp.nimax, xpp.at1, xpp.lb1, &w2d[0]);
  if(xpp.gf1 != 0.0) gfmult(xpp.nimax, xpp.nimax, xpp.at1, xpp.gf1, xpp.gfs1, &w2d[0]);
  if(xpp.sb1 != 0.0) sbmult(xpp.nimax, xpp.nimax, xpp.at1, xpp.sb1, xpp.sbs1, &w2d[0]);

  if(xpp.lb2 != 0.0) lbmult(xpp.ni2max, xpp.ni2max, xpp.at2, xpp.lb2, &w2);
  if(xpp.gf2 != 0.0) gfmult(xpp.ni2max, xpp.ni2max, xpp.at2, xpp.gf2, xpp.gfs2, &w2);
  if(xpp.sb2 != 0.0) sbmult(xpp.ni2max, xpp.ni2max, xpp.at2, xpp.sb2, xpp.sbs2, &w2);

  for(i=1; i< xpp.ni2max; i++)
  {
    for(j=0; j<xpp.nimax; j++)
    {
      w2d[i][j] = w2d[0][j];
    }
  }

  for(i=0; i< xpp.ni2max; i++)
  {
    for(j=0; j<xpp.nimax; j++)
    {
      w2d[i][j] *= w2[i];
    }
  }

  return;
}



