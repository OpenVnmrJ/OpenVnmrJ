/* sense2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* sense2D.c: SENSE data processing routines                                 */
/*                                                                           */
/* Copyright (C) 2012 Paul Kinchesh                                          */
/*                                                                           */
/* This file is part of Xrecon.                                              */
/*                                                                           */
/* Xrecon is free software: you can redistribute it and/or modify            */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* Xrecon is distributed in the hope that it will be useful,                 */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with Xrecon. If not, see <http://www.gnu.org/licenses/>.            */
/*                                                                           */
/*---------------------------------------------------------------------------*/
/**/

#include "../Xrecon.h"


/*------------------------------------------------*/
/*---- Reduction factors for sensitivity maps ----*/
/*------------------------------------------------*/
static int rmapread=1;  /* read */
static int rmapphase=1; /* phase */


/*---------------------------*/
/*---- For SVD in memory ----*/
/*---------------------------*/
static gsl_matrix ****gslmAm;  /* gsl_matrix Am */
static gsl_matrix ****gslmVm;  /* gsl_matrix Vm */
static gsl_vector ****gslvSm;  /* gsl_vector Sm (=Wm) */
static int ***calcm;           /* calculation flag */


/*
   NRC p. 59
   Singular Value Decomposition, or SVD, is the method of choice for solving
   most linear least-squares problems.
   Any M × N matrix A whose number of rows M is greater than or equal to
   its number of columns N (the case of an overdetermined set of equations),
   can be written as the product of an M × N column-orthogonal matrix U, an
   N × N diagonal matrix W with positive or zero elements (the singular values),
   and the transpose of an N × N orthogonal matrix V.

   NRC has routines svdcmp to perform the SVD and svbksb to solve a set of
   simultaneous linear equations Ax = b by "back-substitution", given the
   U, W, and V returned by svdcmp.

   NRC pp. 63-64
   Note that a typical use of svdcmp and svbksb superficially
   resembles the typical use of ludcmp and lubksb: In both cases, you
   decompose the left-hand matrix (A) just once, and then can use the
   decomposition either once or many times with different right-hand
   sides (b). The crucial difference is the 'editing' of the singular
   values before svbksb is called:
   Zeroing the small singular values of W is very often better than
   both the direct-method solution and the SVD solution where the
   small singular values of W are left nonzero. It may seem
   paradoxical that this can be so, since zeroing a singular value
   corresponds to throwing away one linear combination of the set of
   equations that we are trying to solve. The resolution of the
   paradox is that we are throwing away precisely a combination of
   equations that is so corrupted by roundoff error as to be at best
   useless; usually it is worse than useless since it 'pulls' the
   solution vector way off towards infinity along some direction that
   is almost a nullspace vector. SVD cannot be applied blindly, then.
   You have to exercise some discretion in deciding at what threshold
   to zero the small singular values of W.
   svbksb presumes that you have already zeroed the small singular
   values of W. It does not do this for you. If you haven't zeroed
   the small singular values of W, then this routine is just as
   ill-conditioned as any direct method, and you are misusing SVD.

   GSLs gsl_linalg_SV_decomp and gsl_linalg_SV_solve behave likewise.

   NRC p. 49 (LU decomposition):
   If the input matrix is complex, so that you want to solve the
   system (A + iC) · (x + iy) = (b + id) then there are two possible
   ways to proceed. The best way is to rewrite ludcmp and lubksb
   as complex routines. A quick-and-dirty way to solve complex
   systems is to take the real and imaginary parts, giving
   A · x - C · y = b  and  C · x + A · y = d  which can be written
   as a 2N × 2N set of real equations,
       [ A -C ] · [ x ] = [ b ]
       [ C  A ]   [ y ]   [ d ]
   and then solved with ludcmp and lubksb in their present forms.
   This scheme is a factor of 2 inefficient in storage, since A and C
   are stored twice. It is also a factor of 2 inefficient in time,
   since the complex multiplies in a complexified version of the
   routines would each use 4 real multiplies, while the solution of
   a 2N × 2N problem involves 8 times the work of an N × N one.
   If you can tolerate these factor-of-two inefficiencies, then
   the quick-and-dirty way is an easy way to proceed.

   NRCs svdcmp and svbksb are written for real systems.
   GSLs gsl_linalg_SV_decomp and gsl_linalg_SV_solve are also written for
   real systems, so, as a first pass, we just pull the same trick with these.

   NB. gsl_linalg_SV_decomp puts W into vector S
       LAPACK has a complex SVD
*/

#define ZEROSV 1.0e-10


void sense2D(struct data *d)
{
  struct file fref,fvref;
  struct data ref,vref;
  int OK;
  double aread,aphase;
  int npshft,nvshft;
  int np,nv,nr,fn,fn1;
  int vcoilref=FALSE;

  /* Return if data is not from multiple receivers */
  if (d->nr<2) {
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Data is only from a single receiver\n");
  fprintf(stdout,"  Aborting recon of %s ...\n",d->file);
  fflush(stdout);
#endif
    return;
  }

  /* Acceleration factors */
  aread=*val("accelread",&d->p);
  aphase=*val("accelphase",&d->p);

  setreffile(&fref,d,"senseref"); /* Set reference file */
  getpars(fref.procpar[0],&ref); /* Get pars from reference procpar */
  opendata(fref.fid[0],&ref);    /* Open reference data file fid */
  OK=checksenseref(d,&ref);      /* Check against folded parameters */
  if (!OK) return;               /* If all is not OK bail out */
  setdatapars(&ref);             /* Set data structure parameters */
  copypar("file",&d->p,&ref.p);  /* Copy file parameter to reference data */
  copymaskpars(d,&ref);          /* Copy masking parameters to reference data */
  copysmappars(d,&ref);          /* Copy sensitivity map parameters to reference data */
  copysensepars(d,&ref);         /* Copy sense parameters to reference data */
  copynblocks(d,&ref);           /* Copy nblocks parameter to reference data */
  setref2Dmatrix(&ref,d);        /* Set reference fn and fn1 */

  setnvols(&ref);                /* Set the number of data volumes */
  setnvols(d);                   /* Set the number of data volumes */
  dimorder2D(&ref);              /* Sort ascending slice and phase order */
  dimorder2D(d);                 /* Sort ascending slice and phase order */

  /* Set shifts so that the data is nicely aligned for unfolding */
  /* Read */
  if (d->fn == 0) np=d->np/2;
  else np=d->fn/2;
  fn=round2int(aread*np);
  npshft=fn/2;
  /* Phase */
  if (d->fn1 == 0) nv=d->nv;
  else nv=d->fn1/2;
  fn1=round2int(aphase*nv);
  nvshft=fn1/2;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Full FOV read = %d, phase = %d, npshft = %d, nvshft = %d\n",
    fn,fn1,npshft,nvshft);
  fflush(stdout);
#endif

  /* Store some initial parameters */
  np=d->np; nv=d->nv; nr=d->nr; fn=d->fn; fn1=d->fn1;

  /* Output will be masked so we need to figure how */
  /* If we are using volume coil data we will mask with that */
  if (spar(d,"smapref","vcoil")) { /* Volume coil reference */
    OK=setvcoil2D(&ref,&vref,&fvref); /* Set up volume coil data parameters */
    if (!OK) return;               /* If all is not OK bail out */
    vcoilref=TRUE;
  }

  /* Loop over data blocks */
  for (d->block=0;d->block<d->nblocks;d->block++) {

    if (interupt) return;            /* Interupt/cancel from VnmrJ */

    /* Set reference processing block */
    ref.block=d->block;

    /* Calculate sensitivity maps */
    if (vcoilref) OK=smap2Dvcoil(&ref,&vref,STD); /* volume coil reference */
    else OK=smap2Dacoil(&ref,STD);   /* array coil reference */
    if (!OK) return;                 /* If all is not OK bail out */
    svd2Dinmem(&ref);                /* SVD the maps */

    /* Loop over requested volumes in data file */
    for (d->vol=d->startvol;d->vol<d->endvol;d->vol++) {
      getblock2D(d,d->vol,NDCC);     /* Get block without applying dbh.lvl and dbh.tlt */
      shiftdata2D(d,STD);            /* Shift FID data for fft */
      zeronoise(d);                  /* Zero any noise measurement */
      equalizenoise(d,STD);          /* Scale for equal noise in all receivers (standard pars) */
      phaseramp2D(d,PHASE);          /* Phase ramp the data to correct for phase encode offset ppe */
      weightdata2D(d,STD);           /* Apply weighting to data */
      zerofill2D(d,STD);             /* Zero fill data according to fn, fn1 and fn2 */
      fft2D(d,STD);                  /* 2D fft */
      shift2Ddata(d,npshft,nvshft);  /* Shift data for unfolding */
      sense2Dunfold(d,&ref);         /* Unfold the data */
      w2Dfdfs(d,VJ,FLT32,d->vol);    /* Write 2D fdf image data from volume */
      clear2Ddata(d);                /* Clear data from memory */
      d->np=np; d->nv=nv; d->nr=nr;  /* Restore initial np, nv and nr */
      d->fn=fn; d->fn1=fn1;          /* Restore initial fn and fn1 */
    }
    clear2Ddata(&ref);               /* Clear data from memory */
    if (vcoilref) clear2Ddata(&vref);  /* Clear data from memory */
  }

  if (vcoilref) closedata(&vref);    /* Close fid file */
  closedata(&ref);                   /* Close fid file */
  if (vcoilref) clear2Dall(&vref);   /* Clear Vcoil data */
  clear2Dall(&ref);                  /* Clear Reference data */
  clear2Dall(d);                     /* Clear data */

}

void sense2Dunfold(struct data *d,struct data *ref)
{
  int dim1,dim2,dim3,nr;
  int i,j,k,l,m,n;
  double aread,aphase;
  int flaread,flaphase;
  int np,np1,nv,nv1;
  int ncoils,degen1,degen2,degen3,degen4;
  int rk,rl;
  fftw_complex **slice;
  gsl_vector *gslvb;
  gsl_vector *gslvx1,*gslvx2,*gslvx3,*gslvx4;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Unfolding data:\n");
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Acceleration factors */
  aread=*val("accelread",&d->p);
  aphase=*val("accelphase",&ref->p);
  flaread=(int)floor((double)aread);
  flaphase=(int)floor((double)aphase);

  /* 2* for quick-and-dirty method */
  ncoils=2*nr;
  degen1=2*(flaread+1)*(flaphase+1);
  degen2=2*flaread*(flaphase+1);
  degen3=2*(flaread+1)*flaphase;
  degen4=2*flaread*flaphase;

  gslvb=gsl_vector_calloc(ncoils);
  gslvx1=gsl_vector_calloc(degen1);
  gslvx2=gsl_vector_calloc(degen2);
  gslvx3=gsl_vector_calloc(degen3);
  gslvx4=gsl_vector_calloc(degen4);

  np=round2int(aread*dim1);
  np1=np-dim1*flaread;
  nv=round2int(aphase*dim2);
  nv1=nv-dim2*flaphase;

#ifdef DEBUG
  fprintf(stdout,"  Folded read  = %d (%d %d-fold, %d %d-fold)\n",dim1,np1,flaread+1,dim1-np1,flaread);
  fprintf(stdout,"  Folded phase = %d (%d %d-fold, %d %d-fold)\n",dim2,nv1,flaphase+1,dim2-nv1,flaphase);
  fflush(stdout);
#endif

  /* Allocate pointers to final unfolded data */
  if ((slice = (fftw_complex **)fftw_malloc(dim3*sizeof(fftw_complex *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  for (j=0;j<dim3;j++) {
    /* Allocate memory for final unfolded slice */
    if ((slice[j] = (fftw_complex *)fftw_malloc(nv*np*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

    for(k=0;k<nv1;k++) {
      rk=k/rmapphase;

      for(l=0;l<np1;l++) {
        rl=l/rmapread;
        /* If gslmAm elements are non-zero unfolded the data ... */
        if (calcm[j][rk][rl]) {
          /* Construct gslvb from folded data */
          for (i=0;i<nr;i++) {
            gsl_vector_set(gslvb,i,d->data[i][j][k*dim1+l][0]);
            gsl_vector_set(gslvb,i+nr,d->data[i][j][k*dim1+l][1]);
          }
          /* Solve */
          gsl_linalg_SV_solve(gslmAm[j][rk][rl],gslmVm[j][rk][rl],gslvSm[j][rk][rl],gslvb,gslvx1);
          /* Fill unfolded image */
          for (m=0;m<flaphase+1;m++) {
            for (n=0;n<flaread+1;n++) {
              slice[j][(k+m*dim2)*np+n*dim1+l][0]=gsl_vector_get(gslvx1,m*(flaread+1)+n);
              slice[j][(k+m*dim2)*np+n*dim1+l][1]=gsl_vector_get(gslvx1,degen1/2+m*(flaread+1)+n);
            }
          }
        /* ... else set the unfolded pixels to zero */
        } else {
          for (m=0;m<flaphase+1;m++) {
            for (n=0;n<flaread+1;n++) {
              slice[j][(k+m*dim2)*np+n*dim1+l][0]=0.0;
              slice[j][(k+m*dim2)*np+n*dim1+l][1]=0.0;
            }
          }
        }
      }

      for(l=np1;l<dim1;l++) {
        rl=l/rmapread;
        /* If gslmAm elements are non-zero unfolded the data ... */
        if (calcm[j][rk][rl]) {
          /* Construct gslvb from folded data */
          for (i=0;i<nr;i++) {
            gsl_vector_set(gslvb,i,d->data[i][j][k*dim1+l][0]);
            gsl_vector_set(gslvb,i+nr,d->data[i][j][k*dim1+l][1]);
          }
          /* Solve */
          gsl_linalg_SV_solve(gslmAm[j][rk][rl],gslmVm[j][rk][rl],gslvSm[j][rk][rl],gslvb,gslvx2);
          /* Fill unfolded image */
          for (m=0;m<flaphase+1;m++) {
            for (n=0;n<flaread;n++) {
              slice[j][(k+m*dim2)*np+n*dim1+l][0]=gsl_vector_get(gslvx2,m*flaread+n);
              slice[j][(k+m*dim2)*np+n*dim1+l][1]=gsl_vector_get(gslvx2,degen2/2+m*flaread+n);
            }
          }
        /* ... else set the unfolded pixels to zero */
        } else {
          for (m=0;m<flaphase+1;m++) {
            for (n=0;n<flaread;n++) {
              slice[j][(k+m*dim2)*np+n*dim1+l][0]=0.0;
              slice[j][(k+m*dim2)*np+n*dim1+l][1]=0.0;
            }
          }
        }
      }

    }

    for(k=nv1;k<dim2;k++) {
      rk=k/rmapphase;

      for(l=0;l<np1;l++) {
        rl=l/rmapread;
        /* If gslmAm elements are non-zero unfolded the data ... */
        if (calcm[j][rk][rl]) {
         /* Construct gslvb from folded data */
          for (i=0;i<nr;i++) {
            gsl_vector_set(gslvb,i,d->data[i][j][k*dim1+l][0]);
            gsl_vector_set(gslvb,i+nr,d->data[i][j][k*dim1+l][1]);
          }
          /* Solve */
          gsl_linalg_SV_solve(gslmAm[j][rk][rl],gslmVm[j][rk][rl],gslvSm[j][rk][rl],gslvb,gslvx3);
          /* Fill unfolded image */
          for (m=0;m<flaphase;m++) {
            for (n=0;n<flaread+1;n++) {
              slice[j][(k+m*dim2)*np+n*dim1+l][0]=gsl_vector_get(gslvx3,m*(flaread+1)+n);
              slice[j][(k+m*dim2)*np+n*dim1+l][1]=gsl_vector_get(gslvx3,degen3/2+m*(flaread+1)+n);
            }
          }
        /* ... else set the unfolded pixels to zero */
        } else {
          for (m=0;m<flaphase;m++) {
            for (n=0;n<flaread+1;n++) {
              slice[j][(k+m*dim2)*np+n*dim1+l][0]=0.0;
              slice[j][(k+m*dim2)*np+n*dim1+l][1]=0.0;
            }
          }
        }
      }

      for(l=np1;l<dim1;l++) {
        rl=l/rmapread;
        /* If gslmAm elements are non-zero unfolded the data ... */
        if (calcm[j][rk][rl]) {
         /* Construct gslvb from folded data */
          for (i=0;i<nr;i++) {
            gsl_vector_set(gslvb,i,d->data[i][j][k*dim1+l][0]);
            gsl_vector_set(gslvb,i+nr,d->data[i][j][k*dim1+l][1]);
          }
          /* Solve */
          gsl_linalg_SV_solve(gslmAm[j][rk][rl],gslmVm[j][rk][rl],gslvSm[j][rk][rl],gslvb,gslvx4);
          /* Fill unfolded image */
          for (m=0;m<flaphase;m++) {
            for (n=0;n<flaread;n++) {
              slice[j][(k+m*dim2)*np+n*dim1+l][0]=gsl_vector_get(gslvx4,m*flaread+n);
              slice[j][(k+m*dim2)*np+n*dim1+l][1]=gsl_vector_get(gslvx4,degen4/2+m*flaread+n);
            }
          }
        /* ... else set the unfolded pixels to zero */
        } else {
          for (m=0;m<flaphase;m++) {
            for (n=0;n<flaread;n++) {
              slice[j][(k+m*dim2)*np+n*dim1+l][0]=0.0;
              slice[j][(k+m*dim2)*np+n*dim1+l][1]=0.0;
            }
          }
        }
      }

    }

    /* Free data for the slice that we no longer need */
    for (i=0;i<nr;i++) fftw_free(d->data[i][j]);
  }

  /* Properly free data */
  for (i=0;i<nr;i++) fftw_free(d->data[i]);
  fftw_free(d->data);

  d->nr=1;
  d->nv=nv;
  d->np=2*np;
  if ((d->data = (fftw_complex ***)fftw_malloc(1*sizeof(fftw_complex **))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((d->data[0] = (fftw_complex **)fftw_malloc(dim3*sizeof(fftw_complex *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for (j=0;j<dim3;j++)
    d->data[0][j] = slice[j];

  /* Update FOV parameters to reflect new field of view */
  copypar("lro",&ref->p,&d->p);
  copypar("lpe",&ref->p,&d->p);

  /* Reset max pars to default values */
  zeromax(d);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

void svd2Dinmem(struct data *ref)
{
  int dim1,dim2,dim3,nr;
  int i,j,k,l,m,n;
  double aread,aphase;
  int flaread,flaphase;
  int np,np1,nv,nv1;
  int ncoils,degen1,degen2,degen3,degen4;
  double *dp1;
  gsl_vector *gslvwork1,*gslvwork2,*gslvwork3,*gslvwork4;

#ifdef DEBUG
  double dval;
  double min6=100.0,min8=100.0,min10=100.0,min12=100.0;
  double min14=100.0,min16=100.0,min18=100.0,min20=100.0;
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  SVD in memory:\n");
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=ref->np/2; dim2=ref->nv; dim3=ref->endpos-ref->startpos; nr=ref->nr;

  aread=*val("accelread",&ref->p);
  aphase=*val("accelphase",&ref->p);
  flaread=(int)floor((double)aread);
  flaphase=(int)floor((double)aphase);

  np=round2int(dim1/aread);
  np1=dim1-np*flaread;
  nv=round2int(dim2/aphase);
  nv1=dim2-nv*flaphase;

#ifdef DEBUG
  fprintf(stdout,"  Folded read = %d (%d %d-fold, %d %d-fold)\n",np,np1,flaread+1,np-np1,flaread);
  fprintf(stdout,"  Folded phase = %d (%d %d-fold, %d %d-fold)\n",nv,nv1,flaphase+1,nv-nv1,flaphase);
  fflush(stdout);
#endif

  /* 2* for quick-and-dirty method */
  ncoils=2*nr;
  degen1=2*(flaread+1)*(flaphase+1);
  degen2=2*flaread*(flaphase+1);
  degen3=2*(flaread+1)*flaphase;
  degen4=2*flaread*flaphase;

  /* Get hardware-determined floating-point machine constants */
/*
  machar(&mach);
  eps=mach.eps*100.0;
*/

  gslvwork1=gsl_vector_calloc(degen1);
  gslvwork2=gsl_vector_calloc(degen2);
  gslvwork3=gsl_vector_calloc(degen3);
  gslvwork4=gsl_vector_calloc(degen4);
  if ((gslmAm = (gsl_matrix ****)malloc(dim3*sizeof(gsl_matrix ***))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((gslmVm = (gsl_matrix ****)malloc(dim3*sizeof(gsl_matrix ***))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((gslvSm = (gsl_vector ****)malloc(dim3*sizeof(gsl_vector ***))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((calcm = (int ***)malloc(dim3*sizeof(int **))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for (j=0;j<dim3;j++) {
    if ((gslmAm[j] = (gsl_matrix ***)malloc(nv*sizeof(gsl_matrix **))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    if ((gslmVm[j] = (gsl_matrix ***)malloc(nv*sizeof(gsl_matrix **))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    if ((gslvSm[j] = (gsl_vector ***)malloc(nv*sizeof(gsl_vector **))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    if ((calcm[j] = (int **)malloc(nv*sizeof(int *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

    for(k=0;k<nv1;k++) {
      if ((gslmAm[j][k] = (gsl_matrix **)malloc(np*sizeof(gsl_matrix *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      if ((gslmVm[j][k] = (gsl_matrix **)malloc(np*sizeof(gsl_matrix *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      if ((gslvSm[j][k] = (gsl_vector **)malloc(np*sizeof(gsl_vector *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      if ((calcm[j][k] = (int *)malloc(np*sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

      for(l=0;l<np1;l++) {
        gslmAm[j][k][l]=(gsl_matrix *)gsl_matrix_calloc(ncoils,degen1);
        gslmVm[j][k][l]=(gsl_matrix *)gsl_matrix_calloc(degen1,degen1);
        gslvSm[j][k][l]=(gsl_vector *)gsl_vector_calloc(degen1);
        /* Construct gslmA from degenerate pixels in sensitivity maps */
        for (m=0;m<flaphase+1;m++) {
          for (n=0;n<flaread+1;n++) {
            for (i=0;i<nr;i++) {
              dp1 = ref->data[i][j][(k+m*nv)*dim1+n*np+l];
              gsl_matrix_set(gslmAm[j][k][l],i,m*(flaread+1)+n,*dp1++);
              gsl_matrix_set(gslmAm[j][k][l],i+nr,m*(flaread+1)+n,*dp1);
              gsl_matrix_set(gslmAm[j][k][l],i,degen1/2+m*(flaread+1)+n,-*dp1--);
              gsl_matrix_set(gslmAm[j][k][l],i+nr,degen1/2+m*(flaread+1)+n,*dp1);
            }
          }
        }
        if (gsl_matrix_isnull(gslmAm[j][k][l])) {
          calcm[j][k][l]=0;
        } else {
          /* Decompose gslmA */
          gsl_linalg_SV_decomp(gslmAm[j][k][l],gslmVm[j][k][l],gslvSm[j][k][l],gslvwork1);
          calcm[j][k][l]=1;
#ifdef DEBUG
  for (i=0;i<degen1;i++) {
    dval=gsl_vector_get(gslvSm[j][k][l],i);
    if ((dval>1.0e-6) && (dval<min6)) min6=dval;
    if ((dval>1.0e-8) && (dval<min8)) min8=dval;
    if ((dval>1.0e-10) && (dval<min10)) min10=dval;
    if ((dval>1.0e-12) && (dval<min12)) min12=dval;
    if ((dval>1.0e-14) && (dval<min14)) min14=dval;
    if ((dval>1.0e-16) && (dval<min16)) min16=dval;
    if ((dval>1.0e-18) && (dval<min18)) min18=dval;
    if ((dval>1.0e-20) && (dval<min20)) min20=dval;
  }
#endif
          /* Zero the small singular values of gslvS */
          for (i=0;i<degen1;i++)
            if (gsl_vector_get(gslvSm[j][k][l],i) < ZEROSV) gsl_vector_set(gslvSm[j][k][l],i,0.0);
        }
      }

      for(l=np1;l<np;l++) {
        gslmAm[j][k][l]=(gsl_matrix *)gsl_matrix_calloc(ncoils,degen2);
        gslmVm[j][k][l]=(gsl_matrix *)gsl_matrix_calloc(degen2,degen2);
        gslvSm[j][k][l]=(gsl_vector *)gsl_vector_calloc(degen2);
        /* Construct gslmA from degenerate pixels in sensitivity maps */
        for (m=0;m<flaphase+1;m++) {
          for (n=0;n<flaread;n++) {
            for (i=0;i<nr;i++) {
              dp1 = ref->data[i][j][(k+m*nv)*dim1+n*np+l];
              gsl_matrix_set(gslmAm[j][k][l],i,m*flaread+n,*dp1++);
              gsl_matrix_set(gslmAm[j][k][l],i+nr,m*flaread+n,*dp1);
              gsl_matrix_set(gslmAm[j][k][l],i,degen2/2+m*flaread+n,-*dp1--);
              gsl_matrix_set(gslmAm[j][k][l],i+nr,degen2/2+m*flaread+n,*dp1);
            }
          }
        }
        if (gsl_matrix_isnull(gslmAm[j][k][l])) {
          calcm[j][k][l]=0;
        } else {
          /* Decompose gslmA */
          gsl_linalg_SV_decomp(gslmAm[j][k][l],gslmVm[j][k][l],gslvSm[j][k][l],gslvwork2);
          calcm[j][k][l]=1;
#ifdef DEBUG
  for (i=0;i<degen2;i++) {
    dval=gsl_vector_get(gslvSm[j][k][l],i);
    if ((dval>1.0e-6) && (dval<min6)) min6=dval;
    if ((dval>1.0e-8) && (dval<min8)) min8=dval;
    if ((dval>1.0e-10) && (dval<min10)) min10=dval;
    if ((dval>1.0e-12) && (dval<min12)) min12=dval;
    if ((dval>1.0e-14) && (dval<min14)) min14=dval;
    if ((dval>1.0e-16) && (dval<min16)) min16=dval;
    if ((dval>1.0e-18) && (dval<min18)) min18=dval;
    if ((dval>1.0e-20) && (dval<min20)) min20=dval;
  }
#endif
          /* Zero the small singular values of gslvS */
          for (i=0;i<degen2;i++)
            if (gsl_vector_get(gslvSm[j][k][l],i) < ZEROSV) gsl_vector_set(gslvSm[j][k][l],i,0.0);
        }
      }

    }

    for(k=nv1;k<nv;k++) {
      if ((gslmAm[j][k] = (gsl_matrix **)malloc(np*sizeof(gsl_matrix *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      if ((gslmVm[j][k] = (gsl_matrix **)malloc(np*sizeof(gsl_matrix *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      if ((gslvSm[j][k] = (gsl_vector **)malloc(np*sizeof(gsl_vector *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      if ((calcm[j][k] = (int *)malloc(np*sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

      for(l=0;l<np1;l++) {
        gslmAm[j][k][l]=(gsl_matrix *)gsl_matrix_calloc(ncoils,degen3);
        gslmVm[j][k][l]=(gsl_matrix *)gsl_matrix_calloc(degen3,degen3);
        gslvSm[j][k][l]=(gsl_vector *)gsl_vector_calloc(degen3);
        /* Construct gslmA from degenerate pixels in sensitivity maps */
        for (m=0;m<flaphase;m++) {
          for (n=0;n<flaread+1;n++) {
            for (i=0;i<nr;i++) {
              dp1 = ref->data[i][j][(k+m*nv)*dim1+n*np+l];
              gsl_matrix_set(gslmAm[j][k][l],i,m*(flaread+1)+n,*dp1++);
              gsl_matrix_set(gslmAm[j][k][l],i+nr,m*(flaread+1)+n,*dp1);
              gsl_matrix_set(gslmAm[j][k][l],i,degen3/2+m*(flaread+1)+n,-*dp1--);
              gsl_matrix_set(gslmAm[j][k][l],i+nr,degen3/2+m*(flaread+1)+n,*dp1);
            }
          }
        }
        if (gsl_matrix_isnull(gslmAm[j][k][l])) {
          calcm[j][k][l]=0;
        } else {
          /* Decompose gslmA */
          gsl_linalg_SV_decomp(gslmAm[j][k][l],gslmVm[j][k][l],gslvSm[j][k][l],gslvwork3);
          calcm[j][k][l]=1;
#ifdef DEBUG
  for (i=0;i<degen3;i++) {
    dval=gsl_vector_get(gslvSm[j][k][l],i);
    if ((dval>1.0e-6) && (dval<min6)) min6=dval;
    if ((dval>1.0e-8) && (dval<min8)) min8=dval;
    if ((dval>1.0e-10) && (dval<min10)) min10=dval;
    if ((dval>1.0e-12) && (dval<min12)) min12=dval;
    if ((dval>1.0e-14) && (dval<min14)) min14=dval;
    if ((dval>1.0e-16) && (dval<min16)) min16=dval;
    if ((dval>1.0e-18) && (dval<min18)) min18=dval;
    if ((dval>1.0e-20) && (dval<min20)) min20=dval;
  }
#endif
          /* Zero the small singular values of gslvS */
          for (i=0;i<degen3;i++)
            if (gsl_vector_get(gslvSm[j][k][l],i) < ZEROSV) gsl_vector_set(gslvSm[j][k][l],i,0.0);
        }
      }

      for(l=np1;l<np;l++) {
        gslmAm[j][k][l]=(gsl_matrix *)gsl_matrix_calloc(ncoils,degen4);
        gslmVm[j][k][l]=(gsl_matrix *)gsl_matrix_calloc(degen4,degen4);
        gslvSm[j][k][l]=(gsl_vector *)gsl_vector_calloc(degen4);
        /* Construct gslmA from degenerate pixels in sensitivity maps */
        for (m=0;m<flaphase;m++) {
          for (n=0;n<flaread;n++) {
            for (i=0;i<nr;i++) {
              dp1 = ref->data[i][j][(k+m*nv)*dim1+n*np+l];
              gsl_matrix_set(gslmAm[j][k][l],i,m*flaread+n,*dp1++);
              gsl_matrix_set(gslmAm[j][k][l],i+nr,m*flaread+n,*dp1);
              gsl_matrix_set(gslmAm[j][k][l],i,degen4/2+m*flaread+n,-*dp1--);
              gsl_matrix_set(gslmAm[j][k][l],i+nr,degen4/2+m*flaread+n,*dp1);
            }
          }
        }
        if (gsl_matrix_isnull(gslmAm[j][k][l])) {
          calcm[j][k][l]=0;
        } else {
          /* Decompose gslmA */
          gsl_linalg_SV_decomp(gslmAm[j][k][l],gslmVm[j][k][l],gslvSm[j][k][l],gslvwork4);
          calcm[j][k][l]=1;
#ifdef DEBUG
  for (i=0;i<degen4;i++) {
    dval=gsl_vector_get(gslvSm[j][k][l],i);
    if ((dval>1.0e-6) && (dval<min6)) min6=dval;
    if ((dval>1.0e-8) && (dval<min8)) min8=dval;
    if ((dval>1.0e-10) && (dval<min10)) min10=dval;
    if ((dval>1.0e-12) && (dval<min12)) min12=dval;
    if ((dval>1.0e-14) && (dval<min14)) min14=dval;
    if ((dval>1.0e-16) && (dval<min16)) min16=dval;
    if ((dval>1.0e-18) && (dval<min18)) min18=dval;
    if ((dval>1.0e-20) && (dval<min20)) min20=dval;
  }
#endif
          /* Zero the small singular values of gslvS */
          for (i=0;i<degen4;i++)
            if (gsl_vector_get(gslvSm[j][k][l],i) < ZEROSV) gsl_vector_set(gslvSm[j][k][l],i,0.0);
        }
      }

    }

    /* Free reference data for the slice that we no longer need */
    for (i=0;i<nr;i++) fftw_free(ref->data[i][j]);
  }

  /* Properly free data */
  for (i=0;i<nr;i++) fftw_free(ref->data[i]);
  fftw_free(ref->data);
  ref->data=NULL;

#ifdef DEBUG
  fprintf(stdout,"  Singular values < %e set to zero\n",ZEROSV);
  fprintf(stdout,"  Minimum singular value > 1.0e-6 = %e\n",min6);
  fprintf(stdout,"  Minimum singular value > 1.0e-8 = %e\n",min8);
  fprintf(stdout,"  Minimum singular value > 1.0e-10 = %e\n",min10);
  fprintf(stdout,"  Minimum singular value > 1.0e-12 = %e\n",min12);
  fprintf(stdout,"  Minimum singular value > 1.0e-14 = %e\n",min14);
  fprintf(stdout,"  Minimum singular value > 1.0e-16 = %e\n",min16);
  fprintf(stdout,"  Minimum singular value > 1.0e-18 = %e\n",min18);
  fprintf(stdout,"  Minimum singular value > 1.0e-20 = %e\n",min20);
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

void setref2Dmatrix(struct data *ref,struct data *d)
{
  double aread,aphase;
  int dim1,dim2;
  int fn,fn1;
  int flaread,flaphase,np,np1,nv,nv1;

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv;

  /* Acceleration */
  aread=*val("accelread",&d->p);
  aphase=*val("accelphase",&d->p);

  /* Make sure acceleration values are sufficiently precise */
  np=round2int(dim1*aread);
  aread=(double)np/dim1;
  nv=round2int(dim2*aphase);
  aphase=(double)nv/dim2;

  /* Sensitivity map reduction factors */
  rmapread=(int)*val("rmapread",&d->p);
  rmapphase=(int)*val("rmapphase",&d->p);

  /* Make sure rmapread and rmapphase are sensible values */
  if (rmapread < 1) rmapread=1;
  if (rmapphase < 1) rmapphase=1;

  /* Set fn and fn1 according to how zero filling or truncation has been selected */
  if (d->fn<1) fn=round2int(aread*dim1); else fn=d->fn/2;
  if (d->fn1<1) fn1=round2int(aphase*dim2); else fn1=d->fn1/2;

  /* fn and fn1 must result in integer folded data matrix size and must be even */
  while (FP_NEQ(fn/aread,round2int(fn/aread)) || (fn%2 !=0) || (round2int(fn/aread)%2 !=0)) fn++;
  while (FP_NEQ(fn1/aphase,round2int(fn1/aphase)) || (fn1%2 !=0) || (round2int(fn1/aphase)%2 !=0)) fn1++;

  /* Folded data matrix protions must be exactly divisible by rmapread and rmapphase */
  flaread=(int)floor(aread);
  np=fn-round2int(fn*flaread/aread);
  np1=round2int(fn/aread)-np;
  while ((np%rmapread != 0) || (np1%rmapread != 0)) rmapread--;
  flaphase=(int)floor(aphase);
  nv=fn1-round2int(fn1*flaphase/aphase);
  nv1=round2int(fn1/aphase)-nv;
  while ((nv%rmapphase != 0) || (nv1%rmapphase != 0)) rmapphase--;

  /* Zero fill to 1/rmapread * 1/rmapphase the size of raw unfolded data */
  ref->fn = (int)2*fn/rmapread;
  ref->fn1 = (int)2*fn1/rmapphase;

  /* Write reference fn and fn1 to the reference parameter set */
  setval(&ref->p,"fn",ref->fn);
  setval(&ref->p,"fn1",ref->fn1);

  /* Set zero filling for folded data */
  d->fn=round2int(2*fn/aread);
  d->fn1=round2int(2*fn1/aphase);

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  if ((rmapread > 1) || (rmapphase > 1)) {
  fprintf(stdout,"  Using reduced sensitivity data, read by %d, phase by %d\n",
    rmapread,rmapphase);
  }
  fprintf(stdout,"  Reference fn = %d, fn1 = %d\n",ref->fn,ref->fn1);
  fprintf(stdout,"  Folded fn = %d, fn1 = %d\n",d->fn,d->fn1);
  fprintf(stdout,"  Folded read = %d (%d %d-fold, %d %d-fold)\n",round2int(fn/aread),np,flaread+1,np1,flaread);
  fprintf(stdout,"  Folded phase = %d (%d %d-fold, %d %d-fold)\n",round2int(fn1/aphase),nv,flaphase+1,nv1,flaphase);
  fprintf(stdout,"  Acceleration, read = %f, phase = %f\n",aread,aphase);
  fflush(stdout);
#endif

}

int checksenseref(struct data *d,struct data *ref)
{

  /* In SENSE reconstruction we use reference data to generate sensitivity maps */
  /* The reference data must have the same FOV orientation and slices as the */
  /* final unfolded data */

  /* Check FOV */
  if (FP_NEQ(*val("accelread",&d->p) * *val("lro",&d->p),*val("lro",&ref->p))
    || FP_NEQ(*val("accelphase",&d->p) * *val("lpe",&d->p),*val("lpe",&ref->p)) ) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Reference %s and data %s have different FOV\n",ref->procpar,d->procpar);
    fprintf(stderr,"  Aborting recon of %s ...\n",d->file);
    fflush(stderr);
    return(FALSE);
  }

  if (!checkequal(d,ref,"pro","RO positions")) return(FALSE);
  if (!checkequal(d,ref,"ppe","PE positions")) return(FALSE);

  /* Check orientations */
  if (!checkequal(d,ref,"psi","orientations")) return(FALSE);
  if (!checkequal(d,ref,"phi","orientations")) return(FALSE);
  if (!checkequal(d,ref,"theta","orientations")) return(FALSE);

  /* Check slices */
  if (!checkequal(d,ref,"pss","slices")) return(FALSE);

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Reference %s compatible with data %s\n",ref->procpar,d->procpar);
  fflush(stdout);
#endif
  return(TRUE);

}
