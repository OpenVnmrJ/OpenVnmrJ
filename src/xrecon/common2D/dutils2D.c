/* dutils2D.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* dutils2D.c: 2D data utilities                                             */
/*                                                                           */
/* Copyright (C) 2012 Paul Kinchesh                                          */
/*               2012 Martyn Klassen                                         */
/*               2012 Margaret Kritzer                                       */
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

int check2Dref(struct data *d,struct data *ref)
{
  /* The data sets must have the same FOV orientation and slices */

  /* Check FOV */
  if (!checkequal(d,ref,"lro","RO length")) return(FALSE);
  if (!checkequal(d,ref,"lpe","PE length")) return(FALSE);
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

void copy2Ddata(struct data *d1,struct data *d2)
{
  int dim1,dim2,dim3,nr;
  int i,j,k;
  double *dp1,*dp2;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d1->np/2; dim2=d1->nv; dim3=d1->endpos-d1->startpos; nr=d1->nr;

  /* Clear any data that is there */
  if (d2->data) clear2Ddata(d2);

  /* Allocate memory according to nr */
  if ((d2->data = (fftw_complex ***)fftw_malloc(nr*sizeof(fftw_complex **))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for (i=0;i<nr;i++) { /* loop over receiver blocks */
    if ((d2->data[i] = (fftw_complex **)fftw_malloc(dim3*sizeof(fftw_complex *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    for (j=0;j<dim3;j++) /* loop over slices */
      if ((d2->data[i][j] = (fftw_complex *)fftw_malloc(dim2*dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  }

  for (i=0;i<nr;i++) {
    for (j=0;j<dim3;j++) {
      dp1 = *d1->data[i][j];
      dp2 = *d2->data[i][j];
      for(k=0;k<dim2*d1->np;k++) *dp2++ = *dp1++;
    }
  }

  /* Set dimensions and some detail as well */
  d2->np=d1->np; d2->nv=d1->nv; d2->ns=d1->ns; d2->nr=d1->nr;
  d2->fn=d1->fn; d2->fn1=d1->fn1;

  /* Copy status of data */
  d2->ndim=d1->ndim;
  if (d1->dimstatus) {
     free(d2->dimstatus);
     if ((d2->dimstatus = malloc(d2->ndim*sizeof(*(d2->dimstatus)))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
     for (i=0; i<d2->ndim; i++)
       d2->dimstatus[i] = d1->dimstatus[i];
  } else
    d2->dimstatus = NULL;

  d2->datamode=d1->datamode;
  d2->vol=d1->vol;
  d2->nblocks=d1->nblocks; d2->block=d1->block;
  d2->startpos=d1->startpos; d2->endpos=d1->endpos;

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Copying data: took  %f secs\n",t2-t1);
  fflush(stdout);
#endif

}

void clear2Dall(struct data *d)
{
  clear2Ddata(d);
  clear2Dmask(d);
  clearnoise2D(d);
  clearCSIdata(d);
  cleardimorder(d);
  clearstatus(d);
  clearpars(&d->p);
  clearpars(&d->s);
  cleararray(&d->a);
}

void clear2Ddata(struct data *d)
{
  int dim3;
  int i,j;
  if (d->data) {
    dim3=d->endpos-d->startpos;
    for (i=0;i<d->nr;i++) {
      for (j=0;j<dim3;j++) fftw_free(d->data[i][j]);
      fftw_free(d->data[i]);
    }
    fftw_free(d->data);
    d->data = NULL;
  }
}

void clearCSIdata(struct data *d)
{
  int dim3;
  int i;
  if (d->data) {
    dim3=d->endpos-d->startpos;
    for (i=0;i<d->nr;i++) {
      fftw_free(d->csi_data[i]);
    }
    fftw_free(d->csi_data);
    d->csi_data = NULL;
  }
}

void clearnoise2D(struct data *d)
{
  int dim3;
  int j;
  if (d->noise.data) {
    free(d->noise.M);
    free(d->noise.M2);
    free(d->noise.Re);
    free(d->noise.Im);
    d->noise.M=NULL;
    d->noise.M2=NULL;
    d->noise.Re=NULL;
    d->noise.Im=NULL;
  }
  if (d->noise.mat) {
    dim3=d->endpos-d->startpos;
    for (j=0;j<dim3;j++) gsl_matrix_complex_free(d->noise.mat[j]);
    free(d->noise.mat);
    d->noise.mat=NULL;
  }
  /* Set flags */
  d->noise.data=FALSE;
  d->noise.zero=FALSE;
  d->noise.equal=FALSE;
  d->noise.samples=NONE;
}

void clear2Dmask(struct data *d)
{
  int dim3;
  int i;
  if (d->mask) {
    dim3=d->endpos-d->startpos;
    for (i=0;i<dim3;i++) free(d->mask[i]);
    free(d->mask);
    d->mask=NULL;
  }
}

void checkCrop(struct data *d)
{
	int fn1, fn2, startd1, startd2, cropd1, cropd2;

	  fn1=d->fn1;
	  fn2=d->fn2;

	  /* Check that either fn or fn1 or fn2 are active */
	  /* If so, make sure they are exactly divisible by 4 */

	  if (!fn1) fn1 = d->nv*2;
	  else { fn1 /=4; fn1 *=4; }
	  if (!fn2) fn2 = d->nv2*2;
	  else { fn2 /=4; fn2 *=4; }
	  fn1 /= 2;
	  fn2 /= 2;

	startd1 = d->startd1;
	startd2 = d->startd2;
	cropd1 = d->cropd1;
	cropd2 = d->cropd2;

	if((startd1 < 0) || (startd1 > fn1-2)){
		d->startd1=0;
		fprintf(stderr,"Warning: setting start of crop 1 to 1\n");
	}
	if((startd2 < 0) || (startd2 > fn2-2)){
		d->startd2=0;
		fprintf(stderr,"Warning: setting start of crop 2 to 1\n");
	}
	if(cropd1 < 1) {
		d->cropd1=fn1;
		fprintf(stderr,"Warning: setting crop 1 to %d \n",d->cropd1);
	}
	if(cropd2 < 1) {
		d->cropd2=fn2;
		fprintf(stderr,"Warning: setting crop 2 to %d \n",d->cropd2);
	}
	if(d->startd1 + d->cropd1 > fn1){
		d->cropd1 = fn1-d->startd1;
		fprintf(stderr,"Warning: setting crop 1 to %d  \n",d->cropd1);
	}
	if(d->startd1 + d->cropd2 > fn2){
		d->cropd2 = fn2-d->startd2;
		fprintf(stderr,"Warning: setting crop 2 to %d \n",d->cropd2);
	}


}

void checkCrop3D(struct data *d)
{
	int fn3, startd3, cropd3;

	  checkCrop(d);  // set 2D parameters

	  fn3=d->fn3;


	  /* Check that either fn3 is active */
	  /* If so, make sure it is divisible by 4 */

	  if (!fn3) fn3 = d->nv3*2;
	  else { fn3 /=4; fn3 *=4; }

	  fn3 /= 2;

	startd3 = d->startd3;
	cropd3 = d->cropd3;

	if((startd3 < 0) || (startd3 > fn3-2)){
		d->startd3=0;
		fprintf(stderr,"Warning: setting start of crop 3 to 1\n");
	}
	if(cropd3 < 1) {
		d->cropd3=fn3;
		fprintf(stderr,"Warning: setting crop 3 to %d \n",d->cropd3);
	}
	if(d->startd3 + d->cropd3 > fn3){
		d->cropd3 = fn3-d->startd3;
		fprintf(stderr,"Warning: setting crop 3 to %d  \n",d->cropd3);
	}



}
