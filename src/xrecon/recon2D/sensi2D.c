/* sensi2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* sensi2D.c: Sensibility data processing routines                           */
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

/*
   K.P.Pruessmann, M.Weiger, M.B.Scheidegger, P.Boesiger, MRM 42,952-962(1999):
   The geometry factor (g) describes the ability with the used coil
   configuration to separate pixels superimposed by aliasing. In practice
   it allows a priori SNR estimates and provides an important criterion for
   the design of dedicated coil arrays.
*/

#include "../Xrecon.h"

void sensibility2D(struct data *d)
{
  struct file sfref,fref;
  struct data sref,ref,d1;
  double aread,aphase;
  int OK;
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

  /* First we must calculate sensitvity maps */
  /* Check if a suitable reference file is defined */
  /* If it is then we will use the appropriate reference data */
  if (strlen(*sval("senseref",&d->p)) > 0) { /* Sense reference defined */
    setreffile(&sfref,d,"senseref"); /* Set reference file */
    getpars(sfref.procpar[0],&sref); /* Get pars from reference procpar */
    opendata(sfref.fid[0],&sref);    /* Open reference data file fid */
    setdatapars(&sref);              /* Set data structure parameters */
    copypar("file",&d->p,&sref.p);   /* Copy file parameter to reference data */
    copymaskpars(d,&sref);           /* Copy masking parameters to reference data */
    copysmappars(d,&sref);           /* Copy sensitivity map parameters to reference data */
    copysensepars(d,&sref);          /* Copy sense parameters to reference data */
    copynblocks(d,&sref);            /* Copy nblocks parameter to reference data */
    aread=*val("initaread",&d->p);   /* Get acceleration factor in read */
    aphase=*val("initaphase",&d->p); /* Get acceleration factor in phase */
    setfn(&sref,d,aread);            /* Set reference fn */
    setfn1(&sref,d,aphase);          /* Set reference fn1 */
    d=&sref;
  }

  setnvols(d);                   /* Set the number of data volumes */
  dimorder2D(d);                 /* Sort ascending slice and phase order */

  /* Output will be masked so we need to figure how */
  /* If we are using volume coil data we will mask with that */
  if (spar(d,"smapref","vcoil")) {   /* Volume coil reference */
    OK=setvcoil2D(d,&ref,&fref); /* Set up volume coil data parameters */
    if (!OK) return;             /* If all is not OK bail out */
    vcoilref=TRUE;
  }

  /* Loop over data blocks */
  for (d->block=0;d->block<d->nblocks;d->block++) {

    if (interupt) return;        /* Interupt/cancel from VnmrJ */

    if (vcoilref) {

      ref.block=d->block;        /* Set reference processing block */
      getblock2D(&ref,0,NDCC);   /* Get block without applying dbh.lvl and dbh.tlt */
      shiftdata2D(&ref,STD);     /* Shift FID data for fft */
      getblock2D(d,0,NDCC);      /* Get block without applying dbh.lvl and dbh.tlt */
      shiftdata2D(d,STD);        /* Shift FID data for fft */
      zeronoise(d);              /* Zero any noise measurement */
      equalizenoise(d,SM);       /* Scale for equal noise in all receivers (smap pars) */
      get2Dnoisematrix(d,SM);    /* Get noise matrix */
      zeronoise(&ref);           /* Zero reference noise data */
      getnoise(&ref,SM);         /* Get noise of volume coil data */
      scaledata(&ref,d->noise.avM/ref.noise.avM); /* Scale volume coil data */
      phaseramp2D(d,PHASE);      /* Phase ramp the data to correct for phase encode offset ppe */
      phaseramp2D(&ref,PHASE);   /* Phase ramp the data to correct for phase encode offset ppe */
      if (!spar(d,"smapmask","calculated")) {  /* Not pre-calculated mask */
        initdata(&d1);           /* Initialize d1 struct */
        copy2Ddata(&ref,&d1);    /* Store a copy of unweighted data */
        weightdata2D(&ref,MK);   /* Apply mask weighting to data */
        zerofill2D(&ref,STD);    /* Zero fill data according to fn, fn1 */
        fft2D(&ref,STD);         /* 2D fft */
        zeromax(&ref);           /* Clear max data */
        zeronoise(&ref);         /* Clear noise data */
        shiftdata2D(&ref,STD);   /* Shift data to get images */
        get2Dmask(&ref,SM);      /* Get mask */
        fill2Dmask(&ref,SM);     /* Density filter the mask */
        copy2Ddata(&d1,&ref);    /* Restore unweighted data */
      }
      weightdata2D(&ref,SM);     /* Apply sensitivity map weighting to data */
      zerofill2D(&ref,STD);      /* Zero fill data according to fn, fn1 */
      fft2D(&ref,STD);           /* 2D fft */
      shiftdata2D(&ref,STD);     /* Shift data to get images */
      if (spar(d,"smapmask","calculated")) {  /* Calculated mask */
        if (!read2Dmask(&ref,MK)) {
          fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
          fprintf(stderr,"  Problem reading mask\n");
          fflush(stderr);
          return;
        }
      }
      weightdata2D(d,SM);        /* Apply sensitivity map weighting to data */
      zerofill2D(d,STD);         /* Zero fill data according to fn, fn1 */
      fft2D(d,STD);              /* 2D fft */
      shiftdata2D(d,STD);        /* Shift data to get images */
      w2Dfdfs(d,VJ,FLT32,0);     /* Generate output in recon directory */
      gen2Dsmapvcoil(d,&ref);    /* Generate sensitivity map using volume coil */
      mask2Ddata(d,&ref);        /* Mask the data */
      w2Dfdfs(d,SM,FLT32,0);     /* Write 2D sensitivity map data from volume 0 */
      gmap2D(d);                 /* Calculate geometry factor and relative SNR */
      mask2Ddata(d,&ref);        /* Mask the data */
      w2Dfdfs(d,GF,FLT32,0);     /* Write geometry factor */
      w2Dfdfs(d,RS,FLT32,0);     /* Write Relative SNR */

    } else { /* Not volume coil reference */

      getblock2D(d,0,NDCC);      /* Get block without applying dbh.lvl and dbh.tlt */
      shiftdata2D(d,STD);        /* Shift FID data for fft */
      zeronoise(d);              /* Zero any noise measurement */
      equalizenoise(d,SM);       /* Scale for equal noise in all receivers (smap pars) */
      phaseramp2D(d,PHASE);      /* Phase ramp the data to correct for phase encode offset ppe */
      get2Dnoisematrix(d,SM);    /* Get noise matrix */
      if (!spar(d,"smapmask","calculated")) {  /* Not pre-calculated mask */
        initdata(&ref);          /* Initialize ref struct */
        copy2Ddata(d,&ref);      /* Store a copy of unweighted data */
        weightdata2D(d,MK);      /* Apply mask weighting to data */
        zerofill2D(d,STD);       /* Zero fill data according to fn, fn1 */
        fft2D(d,STD);            /* 2D fft */
        zeromax(d);              /* Clear max data */
        zeronoise(d);            /* Clear noise data */
        shiftdata2D(d,STD);      /* Shift data to get images */
        get2Dmask(d,SM);         /* Get mask */
        fill2Dmask(d,SM);        /* Density filter the mask */
        copy2Ddata(&ref,d);      /* Restore unweighted data */
      }
      weightdata2D(d,SM);        /* Apply sensitivity map weighting to data */
      zerofill2D(d,STD);         /* Zero fill data according to fn, fn1 */
      fft2D(d,STD);              /* 2D fft */
      shiftdata2D(d,STD);        /* Shift data to get images */
      w2Dfdfs(d,VJ,FLT32,0);     /* Generate output in recon directory */
      if (spar(d,"smapmask","calculated")) {  /* Calculated mask */
        /* Read the mask */
        if (!read2Dmask(d,MK)) {
          fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
          fprintf(stderr,"  Problem reading mask\n");
          fflush(stderr);
          return;
        }
      }
      if (spar(d,"smapref","sos")) /* SOS reference */
        gen2Dsmapsos(d);         /* Generate sensitivity map using SOS */
      else if (spar(d,"smapref","super")) /* SUPER reference */
        /* FORCE SOS until SUPER fixed */
        gen2Dsmapsos(d);         /* Generate sensitivity map using SUPER combination */
      else
        gen2Dsmapsos(d);         /* Generate sensitivity map using SOS by default */
      mask2Ddata(d,d);	         /* Mask the data */
      w2Dfdfs(d,SM,FLT32,0);     /* Write 2D sensitivity map data from volume 0 */
      gmap2D(d);                 /* Calculate geometry factor and relative SNR */
      mask2Ddata(d,d);           /* Mask the data */
      w2Dfdfs(d,GF,FLT32,0);     /* Write geometry factor */
      w2Dfdfs(d,RS,FLT32,0);     /* Write Relative SNR */

    }

    clear2Ddata(d);                    /* Clear data from memory */
    if (vcoilref) clear2Ddata(&ref);   /* Clear data from memory */

  }

  if (vcoilref) {
    closedata(&ref);             /* Close fid file */
    clear2Dall(&ref);            /* Clear Reference data */
  }
  clear2Dall(d);                 /* Clear data */

}

void gmap2D(struct data *d)
{
  int dim1,dim2,dim3,nr;
  int h,i,j,k,l,m,n;
  double aread,aphase;
  int flaread,flaphase;
  int np,np1,nv,nv1;
  int degen1,degen2,degen3,degen4,rdegen;
  double *dp1;
  double re,im,M;
  int signum;
  fftw_complex *gslice,*rsnrslice;
  gsl_permutation *gslpN,*gslpR;
  gsl_matrix_complex *gslmN,*gslmS,*gslmINS,*gslmST,*gslmSTCINS,*gslmR,*gslmR2; 
  gsl_vector_complex *gslvS,*gslvINS,*gslvR,*gslvIRR;
  gsl_complex cx;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Return if data is not from multiple receivers */
  if (nr<2) {
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Data is only from a single receiver\n");
  fprintf(stdout,"  Aborting recon of %s ...\n",d->file);
#endif
    return;
  }

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  aread=*val("accelread",&d->p);
  aphase=*val("accelphase",&d->p);
  flaread=(int)floor(aread);
  flaphase=(int)floor(aphase);

  np=round2int(dim1/aread);
  np1=dim1-np*flaread;
  nv=round2int(dim2/aphase);
  nv1=dim2-nv*flaphase;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Folded read = %d (%d %d-fold, %d %d-fold)\n",np,np1,flaread+1,np-np1,flaread);
  fprintf(stdout,"  Folded phase = %d (%d %d-fold, %d %d-fold)\n",nv,nv1,flaphase+1,nv-nv1,flaphase);
  fflush(stdout);
#endif

  degen1=(flaread+1)*(flaphase+1);
  degen2=flaread*(flaphase+1);
  degen3=(flaread+1)*flaphase;
  degen4=flaread*flaphase;

  /* Allocate memory for geometry factor (g) and relative SNR maps */
  if ((gslice = (fftw_complex *)fftw_malloc(dim2*dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((rsnrslice = (fftw_complex *)fftw_malloc(dim2*dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Allocate memory for matices permutation and vectors that will be used
     to calculate INS, the product of the inverse noise matrix (IN) and
     the sensitivity matrix S */
  gslmN=(gsl_matrix_complex *)gsl_matrix_complex_calloc(nr,nr); /* Noise matrix */
  gslpN=(gsl_permutation *)gsl_permutation_calloc(nr); /* Noise permutation */
  gslvS=(gsl_vector_complex *)gsl_vector_complex_alloc(nr); /* Sensitivity vector */
  gslvINS=(gsl_vector_complex *)gsl_vector_complex_alloc(nr); /* INS vector */

  for (j=0;j<dim3;j++) {

    /* The geometry factor equation requires inversion of square matrices.
       We therefore perform LU decomposition. We take a copy of the
       Noise matrix as LU decomposition overwrites it with L and U
       GSL manual:
       It is preferable to avoid direct use of the inverse whenever possible,
       as the linear solver functions can obtain the same result more
       efficiently and reliably (consult any introductory textbook on
       numerical linear algebra for details). */
    gsl_matrix_complex_memcpy(gslmN,d->noise.mat[j]);
    gsl_linalg_complex_LU_decomp(gslmN,gslpN,&signum);

    for(k=0;k<nv1;k++) {

      gslmS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(nr,degen1);
      gslmINS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(nr,degen1);
      gslmST=(gsl_matrix_complex *)gsl_matrix_complex_calloc(degen1,nr);
      gslmSTCINS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(degen1,degen1);

      for(l=0;l<np1;l++) {
        /* Construct gslmS from degenerate pixels in sensitivity maps */
        for (m=0;m<flaphase+1;m++) {
          for (n=0;n<flaread+1;n++) {
            for (i=0;i<nr;i++) {
              dp1 = d->data[i][j][(k+m*nv)*dim1+n*np+l];
              GSL_SET_REAL(&cx,*dp1++);
              GSL_SET_IMAG(&cx,*dp1);
              gsl_matrix_complex_set(gslmS,i,m*(flaread+1)+n,cx);
	    }
          }
        }

        /* Calculate only if gslmS is not empty */
	if (!gsl_matrix_complex_isnull(gslmS)) {
          /* Calculate gslmINS */
          for (i=0;i<degen1;i++) {
            gsl_matrix_complex_get_col(gslvS,gslmS,i);
            gsl_linalg_complex_LU_solve(gslmN,gslpN,gslvS,gslvINS);
            gsl_matrix_complex_set_col(gslmINS,i,gslvINS);
          }

          /* gslmSTCINS is the product of the transposed complex conjugate
             of the sensitivity matrix gslmS (gslmSTC) and gslmINS */
          gsl_matrix_complex_transpose_memcpy(gslmST,gslmS);
          complexmatrixconjugate(gslmST);
          complexmatrixmultiply(gslmST,gslmINS,gslmSTCINS);

          /* See if gslmSTCINS can be reduced */
          rdegen=0;
          for (i=0;i<degen1;i++)
            if (gsl_complex_abs(gsl_matrix_complex_get(gslmSTCINS,i,i)) > 0.0)
              rdegen++;

          /* Reduce gslmSTCINS into gslmR */
          gslmR=(gsl_matrix_complex *)gsl_matrix_complex_calloc(rdegen,rdegen);
          m=0; n=0;
          for (h=0;h<degen1;h++) {
            for (i=0;i<degen1;i++) {
              if (gsl_complex_abs(gsl_matrix_complex_get(gslmSTCINS,h,i)) > 0.0) {
                gsl_matrix_complex_set(gslmR,m,n,gsl_matrix_complex_get(gslmSTCINS,h,i));
                n++; m+=n/rdegen; n=n%rdegen;
              }
            }
          }
          gslmR2=(gsl_matrix_complex *)gsl_matrix_complex_alloc(rdegen,rdegen);
          gslpR=(gsl_permutation *)gsl_permutation_alloc(rdegen);
          gslvR=(gsl_vector_complex *)gsl_vector_complex_alloc(rdegen);
          gslvIRR=(gsl_vector_complex *)gsl_vector_complex_alloc(rdegen);

          /* LU decompose gslmR and solve by picking out the required elements
             as we go */
          gsl_matrix_complex_memcpy(gslmR2,gslmR);
          gsl_linalg_complex_LU_decomp(gslmR2,gslpR,&signum);
          GSL_SET_COMPLEX(&cx,0.0,0.0);
          for (i=0;i<rdegen;i++) {
            gsl_vector_complex_set_zero(gslvR);
            gsl_vector_complex_set(gslvR,i,gsl_matrix_complex_get(gslmR,i,i));
            gsl_linalg_complex_LU_solve(gslmR2,gslpR,gslvR,gslvIRR);
            cx=gsl_complex_add(cx,gsl_vector_complex_get(gslvIRR,i));
          }
          cx=gsl_complex_sqrt(cx);

          gsl_vector_complex_free(gslvR);
          gsl_vector_complex_free(gslvIRR);
          gsl_permutation_free(gslpR);
          gsl_matrix_complex_free(gslmR);
          gsl_matrix_complex_free(gslmR2);

          for (m=0;m<flaphase+1;m++) {
            for (n=0;n<flaread+1;n++) {
              dp1 = gslice[(k+m*nv)*dim1+n*np+l];
              *dp1++ = GSL_REAL(cx);
              *dp1 = GSL_IMAG(cx);
              rsnrslice[(k+m*nv)*dim1+n*np+l][0]=1/(gsl_complex_abs(cx)*sqrt(rdegen));
            }
          }
        }
      }

      gsl_matrix_complex_free(gslmS);
      gsl_matrix_complex_free(gslmINS);
      gsl_matrix_complex_free(gslmST);
      gsl_matrix_complex_free(gslmSTCINS);

      gslmS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(nr,degen2);
      gslmINS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(nr,degen2);
      gslmST=(gsl_matrix_complex *)gsl_matrix_complex_calloc(degen2,nr);
      gslmSTCINS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(degen2,degen2);

      for(l=np1;l<np;l++) {
        /* Construct gslmS from degenerate pixels in sensitivity maps */
        for (m=0;m<flaphase+1;m++) {
          for (n=0;n<flaread;n++) {
            for (i=0;i<nr;i++) {
              dp1 = d->data[i][j][(k+m*nv)*dim1+n*np+l];
              GSL_SET_REAL(&cx,*dp1++);
              GSL_SET_IMAG(&cx,*dp1);
              gsl_matrix_complex_set(gslmS,i,m*flaread+n,cx);
            }
          }
        }

        /* Calculate only if gslmS is not empty */
        if (!gsl_matrix_complex_isnull(gslmS)) {
          /* Calculate gslmINS */
          for (i=0;i<degen2;i++) {
            gsl_matrix_complex_get_col(gslvS,gslmS,i);
            gsl_linalg_complex_LU_solve(gslmN,gslpN,gslvS,gslvINS);
            gsl_matrix_complex_set_col(gslmINS,i,gslvINS);
          }

          /* gslmSTCINS is the product of the transposed complex conjugate
             of the sensitivity matrix gslmS (gslmSTC) and gslmINS */
          gsl_matrix_complex_transpose_memcpy(gslmST,gslmS);
          complexmatrixconjugate(gslmST);
          complexmatrixmultiply(gslmST,gslmINS,gslmSTCINS);

          /* See if gslmSTCINS can be reduced */
          rdegen=0;
          for (i=0;i<degen2;i++)
            if (gsl_complex_abs(gsl_matrix_complex_get(gslmSTCINS,i,i)) > 0.0)
              rdegen++;

          /* Reduce gslmSTCINS into gslmR */
          gslmR=(gsl_matrix_complex *)gsl_matrix_complex_calloc(rdegen,rdegen);
          m=0; n=0;
          for (h=0;h<degen2;h++) {
            for (i=0;i<degen2;i++) {
              if (gsl_complex_abs(gsl_matrix_complex_get(gslmSTCINS,h,i)) > 0.0) {
                gsl_matrix_complex_set(gslmR,m,n,gsl_matrix_complex_get(gslmSTCINS,h,i));
                n++; m+=n/rdegen; n=n%rdegen;
              }
            }
          }
          gslmR2=(gsl_matrix_complex *)gsl_matrix_complex_alloc(rdegen,rdegen);
          gslpR=(gsl_permutation *)gsl_permutation_alloc(rdegen);
          gslvR=(gsl_vector_complex *)gsl_vector_complex_alloc(rdegen);
          gslvIRR=(gsl_vector_complex *)gsl_vector_complex_alloc(rdegen);

          /* LU decompose gslmR and solve by picking out the required elements
             as we go */
          gsl_matrix_complex_memcpy(gslmR2,gslmR);
          gsl_linalg_complex_LU_decomp(gslmR2,gslpR,&signum);
          GSL_SET_COMPLEX(&cx,0.0,0.0);
          for (i=0;i<rdegen;i++) {
            gsl_vector_complex_set_zero(gslvR);
            gsl_vector_complex_set(gslvR,i,gsl_matrix_complex_get(gslmR,i,i));
            gsl_linalg_complex_LU_solve(gslmR2,gslpR,gslvR,gslvIRR);
            cx=gsl_complex_add(cx,gsl_vector_complex_get(gslvIRR,i));
          }
          cx=gsl_complex_sqrt(cx);

          gsl_vector_complex_free(gslvR);
          gsl_vector_complex_free(gslvIRR);
          gsl_permutation_free(gslpR);
          gsl_matrix_complex_free(gslmR);
          gsl_matrix_complex_free(gslmR2);

          for (m=0;m<flaphase+1;m++) {
            for (n=0;n<flaread;n++) {
              dp1 = gslice[(k+m*nv)*dim1+n*np+l];
              *dp1++ = GSL_REAL(cx);
              *dp1 = GSL_IMAG(cx);
              rsnrslice[(k+m*nv)*dim1+n*np+l][0]=1/(gsl_complex_abs(cx)*sqrt(rdegen));
            }
          }
        }
      }

      gsl_matrix_complex_free(gslmS);
      gsl_matrix_complex_free(gslmINS);
      gsl_matrix_complex_free(gslmST);
      gsl_matrix_complex_free(gslmSTCINS);

    }

    for(k=nv1;k<nv;k++) {

      gslmS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(nr,degen3);
      gslmINS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(nr,degen3);
      gslmST=(gsl_matrix_complex *)gsl_matrix_complex_calloc(degen3,nr);
      gslmSTCINS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(degen3,degen3);

      for(l=0;l<np1;l++) {
        /* Construct gslmS from degenerate pixels in sensitivity maps */
        for (m=0;m<flaphase;m++) {
          for (n=0;n<flaread+1;n++) {
            for (i=0;i<nr;i++) {
              dp1 = d->data[i][j][(k+m*nv)*dim1+n*np+l];
              GSL_SET_REAL(&cx,*dp1++);
              GSL_SET_IMAG(&cx,*dp1);
              gsl_matrix_complex_set(gslmS,i,m*(flaread+1)+n,cx);
            }
          }
        }

        /* Calculate only if gslmS is not empty */
        if (!gsl_matrix_complex_isnull(gslmS)) {
          /* Calculate gslmINS */
          for (i=0;i<degen3;i++) {
            gsl_matrix_complex_get_col(gslvS,gslmS,i);
            gsl_linalg_complex_LU_solve(gslmN,gslpN,gslvS,gslvINS);
            gsl_matrix_complex_set_col(gslmINS,i,gslvINS);
          }

          /* gslmSTCINS is the product of the transposed complex conjugate
             of the sensitivity matrix gslmS (gslmSTC) and gslmINS */
          gsl_matrix_complex_transpose_memcpy(gslmST,gslmS);
          complexmatrixconjugate(gslmST);
          complexmatrixmultiply(gslmST,gslmINS,gslmSTCINS);

          /* See if gslmSTCINS can be reduced */
          rdegen=0;
          for (i=0;i<degen3;i++)
            if (gsl_complex_abs(gsl_matrix_complex_get(gslmSTCINS,i,i)) > 0.0)
              rdegen++;

          /* Reduce gslmSTCINS into gslmR */
          gslmR=(gsl_matrix_complex *)gsl_matrix_complex_calloc(rdegen,rdegen);
          m=0; n=0;
          for (h=0;h<degen3;h++) {
            for (i=0;i<degen3;i++) {
              if (gsl_complex_abs(gsl_matrix_complex_get(gslmSTCINS,h,i)) > 0.0) {
                gsl_matrix_complex_set(gslmR,m,n,gsl_matrix_complex_get(gslmSTCINS,h,i));
                n++; m+=n/rdegen; n=n%rdegen;
              }
            }
          }
          gslmR2=(gsl_matrix_complex *)gsl_matrix_complex_alloc(rdegen,rdegen);
          gslpR=(gsl_permutation *)gsl_permutation_alloc(rdegen);
          gslvR=(gsl_vector_complex *)gsl_vector_complex_alloc(rdegen);
          gslvIRR=(gsl_vector_complex *)gsl_vector_complex_alloc(rdegen);

          /* LU decompose gslmR and solve by picking out the required elements
             as we go */
          gsl_matrix_complex_memcpy(gslmR2,gslmR);
          gsl_linalg_complex_LU_decomp(gslmR2,gslpR,&signum);
          GSL_SET_COMPLEX(&cx,0.0,0.0);
          for (i=0;i<rdegen;i++) {
            gsl_vector_complex_set_zero(gslvR);
            gsl_vector_complex_set(gslvR,i,gsl_matrix_complex_get(gslmR,i,i));
            gsl_linalg_complex_LU_solve(gslmR2,gslpR,gslvR,gslvIRR);
            cx=gsl_complex_add(cx,gsl_vector_complex_get(gslvIRR,i));
          }
          cx=gsl_complex_sqrt(cx);

          gsl_vector_complex_free(gslvR);
          gsl_vector_complex_free(gslvIRR);
          gsl_permutation_free(gslpR);
          gsl_matrix_complex_free(gslmR);
          gsl_matrix_complex_free(gslmR2);

          for (m=0;m<flaphase;m++) {
            for (n=0;n<flaread+1;n++) {
              dp1 = gslice[(k+m*nv)*dim1+n*np+l];
              *dp1++ = GSL_REAL(cx);
              *dp1 = GSL_IMAG(cx);
              rsnrslice[(k+m*nv)*dim1+n*np+l][0]=1/(gsl_complex_abs(cx)*sqrt(rdegen));
            }
          }
        }
      }

      gsl_matrix_complex_free(gslmS);
      gsl_matrix_complex_free(gslmINS);
      gsl_matrix_complex_free(gslmST);
      gsl_matrix_complex_free(gslmSTCINS);

      gslmS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(nr,degen4);
      gslmINS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(nr,degen4);
      gslmST=(gsl_matrix_complex *)gsl_matrix_complex_calloc(degen4,nr);
      gslmSTCINS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(degen4,degen4);

      for(l=np1;l<np;l++) {

        /* Construct gslmS from degenerate pixels in sensitivity maps */
        for (m=0;m<flaphase;m++) {
          for (n=0;n<flaread;n++) {
            for (i=0;i<nr;i++) {
              dp1 = d->data[i][j][(k+m*nv)*dim1+n*np+l];
              GSL_SET_REAL(&cx,*dp1++);
              GSL_SET_IMAG(&cx,*dp1);
              gsl_matrix_complex_set(gslmS,i,m*flaread+n,cx);
            }
          }
        }

        /* Calculate only if gslmS is not empty */
        if (!gsl_matrix_complex_isnull(gslmS)) {
          /* Calculate gslmINS */
          for (i=0;i<degen4;i++) {
            gsl_matrix_complex_get_col(gslvS,gslmS,i);
            gsl_linalg_complex_LU_solve(gslmN,gslpN,gslvS,gslvINS);
            gsl_matrix_complex_set_col(gslmINS,i,gslvINS);
          }

          /* gslmSTCINS is the product of the transposed complex conjugate
             of the sensitivity matrix gslmS (gslmSTC) and gslmINS */
          gsl_matrix_complex_transpose_memcpy(gslmST,gslmS);
          complexmatrixconjugate(gslmST);
          complexmatrixmultiply(gslmST,gslmINS,gslmSTCINS);

          /* See if gslmSTCINS can be reduced */
          rdegen=0;
          for (i=0;i<degen4;i++)
            if (gsl_complex_abs(gsl_matrix_complex_get(gslmSTCINS,i,i)) > 0.0)
              rdegen++;

          /* Reduce gslmSTCINS into gslmR */
          gslmR=(gsl_matrix_complex *)gsl_matrix_complex_calloc(rdegen,rdegen);
          m=0; n=0;
          for (h=0;h<degen4;h++) {
            for (i=0;i<degen4;i++) {
              if (gsl_complex_abs(gsl_matrix_complex_get(gslmSTCINS,h,i)) > 0.0) {
                gsl_matrix_complex_set(gslmR,m,n,gsl_matrix_complex_get(gslmSTCINS,h,i));
                n++; m+=n/rdegen; n=n%rdegen;
              }
            }
          }
          gslmR2=(gsl_matrix_complex *)gsl_matrix_complex_alloc(rdegen,rdegen);
          gslpR=(gsl_permutation *)gsl_permutation_alloc(rdegen);
          gslvR=(gsl_vector_complex *)gsl_vector_complex_alloc(rdegen);
          gslvIRR=(gsl_vector_complex *)gsl_vector_complex_alloc(rdegen);

          /* LU decompose gslmR and solve by picking out the required elements
             as we go */
          gsl_matrix_complex_memcpy(gslmR2,gslmR);
          gsl_linalg_complex_LU_decomp(gslmR2,gslpR,&signum);
          GSL_SET_COMPLEX(&cx,0.0,0.0);
          for (i=0;i<rdegen;i++) {
            gsl_vector_complex_set_zero(gslvR);
            gsl_vector_complex_set(gslvR,i,gsl_matrix_complex_get(gslmR,i,i));
            gsl_linalg_complex_LU_solve(gslmR2,gslpR,gslvR,gslvIRR);
            cx=gsl_complex_add(cx,gsl_vector_complex_get(gslvIRR,i));
          }
          cx=gsl_complex_sqrt(cx);

          gsl_vector_complex_free(gslvR);
          gsl_vector_complex_free(gslvIRR);
          gsl_permutation_free(gslpR);
          gsl_matrix_complex_free(gslmR);
          gsl_matrix_complex_free(gslmR2);

          for (m=0;m<flaphase;m++) {
            for (n=0;n<flaread;n++) {
              dp1 = gslice[(k+m*nv)*dim1+n*np+l];
              *dp1++ = GSL_REAL(cx);
              *dp1 = GSL_IMAG(cx);
              rsnrslice[(k+m*nv)*dim1+n*np+l][0]=1/(gsl_complex_abs(cx)*sqrt(rdegen));
            }
          }
        }
      }

      gsl_matrix_complex_free(gslmS);
      gsl_matrix_complex_free(gslmINS);
      gsl_matrix_complex_free(gslmST);
      gsl_matrix_complex_free(gslmSTCINS);

    }

    /* Calculate Relative SNR */
    if (spar(d,"noisematrix","y")) { /* Use noise matrix */
      gslmST=(gsl_matrix_complex *)gsl_matrix_complex_calloc(1,nr);
      gslmINS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(nr,1);
      gslmSTCINS=(gsl_matrix_complex *)gsl_matrix_complex_calloc(1,1);
      for (k=0;k<dim2*dim1;k++) {
        for(i=0;i<nr;i++) {
          dp1 = d->data[i][j][k];
          GSL_SET_REAL(&cx,*dp1++);
          GSL_SET_IMAG(&cx,*dp1);
          gsl_vector_complex_set(gslvS,i,cx);
          gsl_matrix_complex_set(gslmST,0,i,cx);
        }
        complexmatrixconjugate(gslmST);
        gsl_linalg_complex_LU_solve(gslmN,gslpN,gslvS,gslvINS);
        gsl_matrix_complex_set_col(gslmINS,0,gslvINS);
        complexmatrixmultiply(gslmST,gslmINS,gslmSTCINS);
        cx=gsl_matrix_complex_get(gslmSTCINS,0,0);
        /* A unity noise matrix would give the same result as SOS
           so we must take the square root */
        d->data[1][j][k][0]=sqrt(gsl_complex_abs(cx));
        d->data[1][j][k][1]=0.0;
      }
      gsl_matrix_complex_free(gslmST);
      gsl_matrix_complex_free(gslmINS);
      gsl_matrix_complex_free(gslmSTCINS);
    } else { /* Just use the square root of SOS */
      for (k=0;k<dim2*dim1;k++) {
        M=0.0;
        for(i=0;i<nr;i++) {
          re=fabs(d->data[i][j][k][0]);
          im=fabs(d->data[i][j][k][1]);
          M+=re*re+im*im;
        }
        d->data[1][j][k][0]=sqrt(M);
        d->data[1][j][k][1]=0.0;
      }
    }

    /* Fill data with geometry factor and adjust relative SNR appropriately */
    for (k=0;k<dim2*dim1;k++) {
      d->data[0][j][k][0]=gslice[k][0];
      d->data[0][j][k][1]=gslice[k][1];
      d->data[1][j][k][0]*=rsnrslice[k][0];
    }

  } /* end dim3 for loop */

  gsl_vector_complex_free(gslvS);
  gsl_vector_complex_free(gslvINS);
  gsl_matrix_complex_free(gslmN);
  gsl_permutation_free(gslpN);

  fftw_free(gslice);
  fftw_free(rsnrslice);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Took %f secs\n",t2-t1);
  fflush(stdout);
#endif

}
