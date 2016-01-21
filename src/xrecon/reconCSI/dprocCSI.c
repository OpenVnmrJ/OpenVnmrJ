/* dprocCSI.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* dprocCSI.c: CSI Data processing routines                                  */
/*                                                                           */
/* Copyright (C) 2012 Margaret Kritzer                                       */
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

void dimorderCSI2D(struct data *d)
{
	int l1,l2;


		/* read in the tables */
		/* these are all equal length = total number of acqs */
		d->synclist[0]=FALSE;
		d->synclist[1]=FALSE;

		if (parindex("pelist",&d->p) > -1)
			l1=nvals("pelist",&d->p);
		else
			l1=-1;
		if (parindex("pe2list",&d->p) > -1)
				l2=nvals("pe2list",&d->p);
			else
				l2=-1;

		if((l1 < 0) || (l1 == d->nv))  // tables are independent
		{
			d->korder = FALSE;
			  /* Fill dim2order with the nv phase encode order */
			  d->dim2order=phaseorder(d,d->nv,d->nseg*d->etl,"pelist");
			  /* Fill dim3order with the nv2 phase encode order */
			  d->dim3order=phaseorder(d,d->nv2,d->nv2,"pe2list");
		}
		else
		{
			if (l1 == l2) {
			d->korder = TRUE;
			d->synclist[0] = d->synclist[1] = TRUE;
			d->dim2order = phaselist(d, "pelist");
			d->dim3order = phaselist(d, "pe2list");
		}

	}


  /* Fill pssorder with the slice order */
  d->pssorder=sliceorder(d,d->ns,"pss");

}

void dimorderCSI3D(struct data *d)
{
	int l1,l2,l3;
	if (d->korder) {
		/* read in the tables */
		/* these are all equal length = total number of acqs */
		d->synclist[0]=FALSE;
		d->synclist[1]=FALSE;
		d->synclist[2]=FALSE;

		if (parindex("pelist",&d->p) > -1)
			l1=nvals("pelist",&d->p);
		else
			l1=-1;
		if (parindex("pe2list",&d->p) > -1)
				l2=nvals("pe2list",&d->p);
			else
				l2=-1;
		if (parindex("pe3list",&d->p) > -1)
			l3=nvals("pe3list",&d->p);
		else
			l3=-1;

		// some warped logic to figure out which dimensions are sync'ed
		if((l1>-1) && (l1 != d->nv))
		{
			if(l1==l2)
			{
				if(l1==l3)
					d->synclist[0]=d->synclist[1]=d->synclist[2]=TRUE;
				else
					d->synclist[0]=d->synclist[1]=TRUE;
			}
			else if(l1==l3)
				d->synclist[0]=d->synclist[2]=TRUE;
		}
		if((l2>-1) && (l2 != d->nv2))
		{
			if(l2==l3)
				d->synclist[1]=d->synclist[2]=TRUE;
		}

		if(d->synclist[0])
			d->dim2order = phaselist(d,"pelist");
		else
			d->dim2order=phaseorder(d,d->nv,d->nseg*d->etl,"pelist");

		if(d->synclist[1])
			d->dim3order = phaselist(d, "pe2list");
		else
			d->dim3order=phaseorder(d,d->nv2,d->nv2,"pe2list");

		if(d->synclist[2])
			d->dim4order = phaselist(d, "pe3list");
		else
			d->dim4order=phaseorder(d,d->nv3,d->nv3,"pe3list");

	}
	else{
		  /* Fill dimXorder with the nvX phase encode order */
		  d->dim2order=phaseorder(d,d->nv,d->nseg*d->etl,"pelist");
		  d->dim3order=phaseorder(d,d->nv2,d->nv2,"pe2list");
		  d->dim4order=phaseorder(d,d->nv3,d->nv3,"pe3list");
	}
}

void getblockCSI2D(struct data *d,int volindex,int DCCflag)
{
	int dim1, dim2, dim3, nr, dim4;
	int i, j, k, l;
	int jj, kk;

  /* Check d->nvols has been set */
  if (d->nvols == 0) setnvols(d); /* Set the number of data volumes */

  /* Set start and end position of block which is one slice for CSI */
  setblock(d,d->nv2);

  /* Get the block */
  getblock(d,volindex,DCCflag);



  /* Set data dimensions */
   dim1=d->np/2; dim3=d->endpos-d->startpos; nr=d->nr;dim4=d->nv3;
   dim2=d->nv;

   /* Allocate memory according to nr */
   if ((d->csi_data = (fftw_complex **)fftw_malloc(nr*sizeof(fftw_complex *))) == NULL)
	   nomem(__FILE__,__FUNCTION__,__LINE__);

   /* Allocate memory according to block size */
   for (i=0;i<nr;i++)  /* loop over receivers */
   {
	   d->csi_data[i] = NULL;
     d->csi_data[i] = (fftw_complex *)fftw_malloc(dim3*dim2*dim1*sizeof(fftw_complex));
				if(d->csi_data[i] == NULL)nomem(__FILE__,__FUNCTION__,__LINE__);
   }
   // copy data to csi_data
   for (i=0;i<nr;i++){
		   for(j=0;j<dim3;j++){
			   jj=j*dim2*dim1;
			   for(k=0;k<dim2;k++){
				   kk=jj+k*dim1;
				   for(l=0;l<dim1;l++){
					   d->csi_data[i][kk+l][0] = d->data[i][j][k*dim1+l][0];
					   d->csi_data[i][kk+l][1] = d->data[i][j][k*dim1+l][1];
				   }
			   }
		   }
   }

	   clear2Ddata(d);
}
void getblockCSI3D(struct data *d,int volindex ,int DCCflag)  // reads nv*nv2 "in-plane" raw data block to process
{
	int dim1, dim2, dim3, nr, dim4;
	int i, j, k, l;
	int jj, kk, mm;
	int d4index = volindex;
	int fn0,fn1,fn2,fn3;
	int df0, df1, df2, df3;

/* Check d->nvols has been set */
	if (d->nvols == 0)
		setnvols(d); /* Set the number of data volumes */

	/* Set start and end position of block which is one slice for CSI */
	setblock(d, d->nv2);

	/* Get the block */
	getblock(d, volindex, DCCflag);


	/* Set data dimensions */
	dim1 = d->np / 2;
	dim2 = d->nv;
	dim3 = d->nv2;
	dim4 = d->nv3;
	nr = d->nr;

	// account for zero filling
	if(spar(d, "epsi", "y")){
		if(d->fn1) fn1=d->fn1/2; else fn1=dim2;   // nv orig in dim2
		if(d->fn2) fn0=d->fn2/2; else fn0=dim1;   // nv2 orig in dim1
		if(d->fn3)fn2=d->fn3/2; else fn2=dim3;    // nv3 orig in dim3
		fn3=dim4;  // spectral orig in dim4 but we don't zero fill spectral
	}
	else{
		if(d->fn1) fn1=d->fn1/2; else fn1=dim2;
		if(d->fn2) fn2=d->fn2/2; else fn2=dim3;
		if(d->fn3) fn3=d->fn3/2; else fn3=dim4;
		fn0=dim1; // spectral dimension
	}

	df0=(fn0-dim1)/2;
	df1=(fn1-dim2)/2;
	df2=(fn2-dim3)/2;
	df3=(fn3-dim4)/2;

	mm = (df3+d4index)*fn0*fn1*fn2;

	// copy data to csi_data
	for (i = 0; i < nr; i++) {
		for (j = 0; j < dim3; j++) {
			jj = mm + (df2+j) * fn1 * fn0;
			for (k = 0; k < dim2; k++) {
				kk = jj + (df1+k) * fn0;

				for (l = 0; l < dim1; l++) {
					d->csi_data[i][kk + l + df0][0] = d->data[i][j][k * dim1 + l][0];
					d->csi_data[i][kk + l + df0][1] = d->data[i][j][k * dim1 + l][1];
				}
			}
		}
	}

	d->np=2*fn0;
	d->nv=fn1;
	d->nv2=fn2;
	d->nv3=fn3;

  clear2Ddata(d);

}


void fftdim3CSI(struct data *d)
{
  fftw_complex *data;
  fftw_plan p;
  int dim1,dim2,dim3,dim4,nr;
  int i,j,k;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Starting 1D FT in dim3 ...\n");
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->nv2; nr=d->nr; dim4=d->nv3;

  /* Allocate memory for in-plane "slice"  */
  if ((data = (fftw_complex *)fftw_malloc(dim4*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Measured plan for 1D FT */
  p=fftw_plan_dft_1d(dim3,data,data,FFTW_FORWARD,FFTW_MEASURE);

  /* FT  dim3 ... */
  for (i=0;i<nr;i++) {
    for (k=0;k<dim1*dim2;k++) {
      for (j=0;j<dim3;j++) {
        data[j][0]=d->data[i][j][k][0];
        data[j][1]=d->data[i][j][k][1];
      }
      /* ft */
      fftw_execute(p);
      for (j=0;j<dim3;j++) {
        d->data[i][j][k][0]=data[j][0];
        d->data[i][j][k][1]=data[j][1];
      }
    }
  }
  /* ... and tidy up */
  fftw_destroy_plan(p);
  fftw_free(data);

  /* Set status for dimension */
  d->dimstatus[2]+=FFT;
  if (d->dimstatus[2] & SHIFT) d->dimstatus[2]-=SHIFT;

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  1D FT of %d x %d x %d data: took %f secs\n",dim1,dim2,dim3,t2-t1);
  fflush(stdout);
#endif

}

void zerofill2DCSI(struct data *d, int mode)
{
  int i,j,k;
  int ix1,ix2;
  int iy1, iy2;
  int dim1,dim2,dim3,nr;
  int fn=0,fn1=0, fn2=0;
  int off2, off3, noff2, noff3;
  int ndim1,ndim2,range2,shft1,shft2,nshft2;
  int ndim3, range3, shft3, nshft3;
  double oversample;
  fftw_complex *data;


#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  switch(mode) {
    case STD: /* spatial dimensions */
      if (d->dimstatus[1] & FFT || d->dimstatus[2] & FFT) return;
      oversample=*val("oversample",&d->p);
      if (oversample == 0.0) oversample=1;
      fn=d->fn*oversample;
      fn1=d->fn1;
      fn2=d->fn2;
      break;
    case READ: /* Standard parameter */
      if (d->dimstatus[0] & FFT) return;
      oversample=*val("oversample",&d->p);
      if (oversample == 0.0) oversample=1;
      fn=d->fn*oversample;
      break;
    case PHASE: /* Standard parameter */
      if (d->dimstatus[1] & FFT) return;
      fn1=d->fn1;
      break;
    case PHASE2: /* Standard parameter */
      if (d->dimstatus[2] & FFT) return;
      fn2=d->fn2;
      break;
    default:
      /* Invalid mode */
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Invalid 2nd argument %s(*,'mode')\n",__FUNCTION__);
      fflush(stderr);
      return;
      break;
  } /* end mode switch */

  /* Check that either fn or fn1 or fn2 are active */
  /* If so, make sure they are exactly divisible by 4 */
  if (!fn && !fn1 && !fn2) return;
  if (!fn) fn = d->np;
  else { fn /=4; fn *=4; }
  if (!fn1) fn1 = d->nv*2;
  else { fn2 /=4; fn2 *=4; }
  if (!fn2) fn2 = d->nv2*2;
  else { fn2 /=4; fn2 *=4; }

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Initial data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->nv2; nr=d->nr;

  nr=1; // channels combined already

  /* New data dimensions */
  ndim2=fn1/2;
  ndim3=fn2/2;
  ndim1=dim1; // not zero filling in fid dimension

  if((ndim1 == dim1)&&(ndim2 == dim2)&&(ndim3 == dim3)) return;

  /* Set up for new matrix size to be larger than original */
  range2=dim2; range3=dim3;  /* data range */
  shft1=0; shft2=0; shft3=0;        /* shifts required for original data */
  off2 = off3 = 0;
  noff2 = noff3 = 0;

  nshft2=abs(ndim2-dim2) ;   /* shift for new matrix */
  nshft3=abs(ndim3-dim3) ;

  /* Correct new matrix shifts if original data is not shifted */
  if (!(d->dimstatus[1] & SHIFT))
		  nshft2/=2;
  else
	  {
		  noff2=nshft2;
		  nshft2=0;
	  }
  if (!(d->dimstatus[2] & SHIFT))
	  nshft3/=2;
  else
	  {
		  noff3=nshft3;
		  nshft3=0;
	  }

  /* Now allow new matrix size to be smaller than original */
  if (ndim2<dim2) {
    range2=ndim2; shft2=nshft2; nshft2=0; off2=noff2; noff2=0; /* reset range and swap shifts */
  }
  if (ndim3<dim3) {
     range3=ndim3; shft3=nshft3; nshft3=0; off3=noff3; noff3=0; /* reset range and swap shifts */
   }

  data = (fftw_complex *)fftw_malloc(dim3*dim2*dim1*sizeof(fftw_complex));
  if(data == NULL)nomem(__FILE__,__FUNCTION__,__LINE__);

  // only handling shifting in spatial dimensions
      for (i = 0; i < nr; i++) {
		// copy original data
		(void) memcpy(data, d->csi_data[i], dim3 * dim2 * dim1
				* sizeof(fftw_complex));

		/* free d->csi_data,  reallocate and copy data back in its place */
		fftw_free(d->csi_data[i]);
		d->csi_data[i] = NULL;

		// calloc zeroes the memory
		d->csi_data[i] = (fftw_complex *) calloc(ndim3 * ndim2 * ndim1, sizeof(fftw_complex));
		if(d->csi_data[i] == NULL)nomem(__FILE__, __FUNCTION__, __LINE__);

      /* Copy back to larger matrix */
		for (j = 0; j < range3/2; j++) {
			iy1 = (nshft3 + j) * ndim2 * ndim1;
			iy2 = (shft3 + j) * dim2 * dim1;
			for (k = 0; k < range2/2; k++) {
				ix1 = (nshft2 + k) * ndim1;
				ix2 = (shft2 + k) * dim1;
				(void) memcpy(&(d->csi_data[i][iy1+ix1]), &data[iy2+ix2],  dim1 * sizeof(fftw_complex));
			}
			for (; k < range2; k++) {
				ix1 = (nshft2 + noff2 + k) * ndim1;
				ix2 = (shft2 + off2 + k) * dim1;
				(void) memcpy(&(d->csi_data[i][iy1+ix1]), &data[iy2+ix2],  dim1 * sizeof(fftw_complex));
			}
		}
		for (;j < range3; j++) {
			iy1 = (nshft3 + noff3 + j) * ndim2 * ndim1;
			iy2 = (shft3 + off3 + j) * dim2 * dim1;
			for (k = 0; k < range2/2; k++) {
				ix1 = (nshft2 + k) * ndim1;
				ix2 = (shft2 + k) * dim1;
				(void) memcpy(&(d->csi_data[i][iy1+ix1]), &data[iy2+ix2],  dim1 * sizeof(fftw_complex));
			}
			for (; k < range2; k++) {
				ix1 = (nshft2 + noff2 + k) * ndim1;
				ix2 = (shft2 + off2 + k) * dim1;
				(void) memcpy(&(d->csi_data[i][iy1+ix1]), &data[iy2+ix2],  dim1 * sizeof(fftw_complex));
			}
		}
    } // end of rcvr loop



  /* Free data */
  fftw_free(data);

  /* Set zerofill flags */
  if (d->nv != ndim2) d->dimstatus[1]+=ZEROFILL;
  if (d->nv2 != ndim3) d->dimstatus[2]+=ZEROFILL;

  /* Update parameters to reflect new data size */
// d->np=fn;
  d->nv=ndim2;
  d->nv2=ndim3;

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Zero filling to give %d x %d data: took %f secs\n",ndim1,ndim2,t2-t1);
  fflush(stdout);
#endif

}
void shiftdim3dataCSI(struct data *d,int dim4shft)
{
  int dim1, dim2, dim3, dim4,nr;
  fftw_complex *slice;
  int i,j;
  int offseta, offsetb;
  int ss, slicesize;
  int ds4;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Set dim3 and receivers for 3D CSI data */
  dim1=d->np/2; dim2=d->nv; dim3=d->nv2; dim4=d->nv3; nr=d->nr;
  ss = dim1*dim2*dim3;
  slicesize = ss*sizeof(fftw_complex);
  ds4=dim4shft-dim4;

  /* Allocate memory for in-plane "slice" */
  slice = NULL;
  slice = (fftw_complex *)fftw_malloc(slicesize);
  if(slice == NULL)nomem(__FILE__,__FUNCTION__,__LINE__);

  for (j = 0; j < dim4 - dim4shft; j++) {
		offseta = j * ss;
		offsetb = (j + dim4shft) * ss;
		for (i = 0; i < nr; i++) {
			(void) memcpy(slice, d->csi_data[i][offseta], slicesize);
			(void) memcpy(d->csi_data[i][offseta], d->csi_data[i][offsetb],
					slicesize);
			(void) memcpy(d->csi_data[i][offsetb], slice, slicesize);
		}
	}
	for (; j < dim4; j++) {
		offseta = j * ss;
		offsetb = (j + ds4) * ss;
		for (i = 0; i < nr; i++) {
			(void) memcpy(slice, d->csi_data[i][offseta], slicesize);
			(void) memcpy(d->csi_data[i][offseta], d->csi_data[i][offsetb],
					slicesize);
			(void) memcpy(d->csi_data[i][offsetb], slice, slicesize);

		}
	}

  fftw_free(slice);

#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Took %f secs\n",t2-t1);
  fflush(stdout);
#endif
}

void phaserampdim3CSI(struct data *d,int mode)
{
  int dim1,dim2,dim3,dim4,nr;
  int i,j,k;
  int slicesize;
  double re,im,M,theta,factor;
  double ppe, fov;
  double phi;
  int offset;

#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->nv2; dim4=d->nv3; nr=d->nr;
  slicesize = dim1*dim2*dim3;

  switch (mode) {
    case PHASE3:
      /* Return if there is no phase ramp to apply */
      ppe=*val("ppe3",&d->p);
      if (ppe == 0.0) return;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif
      fov=*val("lpe3",&d->p);
      /* Set phase ramp factor to correct for offset */
      factor=-2*M_PI*ppe/fov;
      /* Apply phase ramp to generate frequency shift */
      offset=0;
      for (i = 0; i < nr; i++) {
			for (j = 0; j < dim4 / 2; j++) {
				phi = factor * j;
				for (k = 0; k < slicesize; k++) {
					re = d->csi_data[i][offset + k][0];
					im = d->csi_data[i][offset + k][1];
					M = sqrt(re * re + im * im);
					theta = atan2(im, re) + phi;
					d->csi_data[i][offset + k][0] = M * cos(theta);
					d->csi_data[i][offset + k][1] = M * sin(theta);
				}
				offset += slicesize;
			}
			for (; j < dim4; j++) {
				phi = factor * (j - dim4);
				for (k = 0; k < slicesize; k++) {
					re = d->csi_data[i][offset + k][0];
					im = d->csi_data[i][offset + k][1];
					M = sqrt(re * re + im * im);
					theta = atan2(im, re) + factor * (j);
					theta = atan2(im, re) + factor * (j - dim3);
					d->csi_data[i][offset + k][0] = M * cos(theta);
					d->csi_data[i][offset + k][1] = M * sin(theta);
				}
				offset += slicesize;
			}
		}


#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"  Phase encode phase ramp (%d traces): took %f secs\n",dim1,t2-t1);
  fflush(stdout);
#endif
      break;
  }

}
void w2DCSI(struct data *d, int receiver,int d4, int d3, int d2,int fileid)
{
  FILE *fp=0;
  float *floatdata;
  int dim1,dim2,dim3,nr;
  int j,k;
  int offset;

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->nv2; nr=d->nr;

  if(d->fn1)dim2 = (d->fn1)/2;
  if(d->fn2)dim3 = (d->fn2)/2;


  offset = d4*dim1*dim2*dim3 + d3*dim1*dim2 + d2*dim1;

  switch(fileid) {
  case FID_FILE:
    fp=d->fidfp;
    if ((floatdata = (float *) malloc(d->np * sizeof(float))) == NULL)
			nomem(__FILE__, __FUNCTION__, __LINE__);
		k = 0;
		for (j = 0; j < dim1; j++) {
			floatdata[2 * k] = (float) d->csi_data[receiver][offset+j][0];
			floatdata[2 * k + 1] = (float) d->csi_data[receiver][offset+j][1];
			k++;
		}

    if (reverse_byte_order)
      reverse4ByteOrder(d->np,(char *)floatdata);
    fwrite(floatdata,sizeof(float),d->np,fp);
    break;
    default:
      break;
  }
}

void w3DCSI(struct data *d,int receiver,int d4, int d3, int d2,int fileid)
{
  FILE *fp=0;
  float *floatdata;
  int dim1,dim2,dim3,nr;
  int j,k;
  int offset;

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->nv2; nr=d->nr;
  offset = d3*dim1*dim2+d2*dim1;
  offset += d4*dim1*dim2*dim3;

  switch(fileid) {
  case FID_FILE:
    fp=d->fidfp;
    if ((floatdata = (float *) malloc(d->np * sizeof(float))) == NULL)
			nomem(__FILE__, __FUNCTION__, __LINE__);
		k = 0;
		for (j = 0; j < dim1; j++) {
			floatdata[2 * k] = (float) d->csi_data[receiver][offset+j][0];
			floatdata[2 * k + 1] = (float) d->csi_data[receiver][offset+j][1];
			k++;
		}

    if (reverse_byte_order)
      reverse4ByteOrder(d->np,(char *)floatdata);
    fwrite(floatdata,sizeof(float),d->np,fp);
    break;
    default:
      break;
  }
}
void wdfhCSI(struct data *d,int fileid)
{
  FILE *fp=0;
  struct datafilehead fh;
  int vers_id;
  int fn, fn1, fn2;

/* d->fh.vers_id seems to be zero for fids ??
  vers_id=d->fh.vers_id-FID_FILE; */
  vers_id=1;

  /* account for possible zero filling in final fid size */
  fn=d->fn;
  fn1=d->fn1;
  fn2=d->fn2;

  /* Check that either fn or fn1 or fn2 are active */
  /* If so, make sure they are exactly divisible by 4 */
  if (!fn) fn = d->np;
  else { fn /=4; fn *=4; }
  if (!fn1) fn1 = d->nv*2;
  else { fn2 /=4; fn2 *=4; }
  if (!fn2) fn2 = d->nv2*2;
  else { fn2 /=4; fn2 *=4; }
  fn1 /= 2;
  fn2 /= 2;

  // However we are going to crop ultimately
  fn1 =d->cropd1;
  fn2 =d->cropd2;


  switch(fileid) {
    case FID_FILE:
     if (im3D(d))
    	 fh.nblocks   = fn1*fn2*d->nvols;           /* 2D csi */
     if (im4D(d))
     	 fh.nblocks   = fn1*fn2*d->nv3;           /* 2D csi */
      fh.ntraces   = 1;    /* number of traces per block */
      fh.np        = d->np;            /* number of elements per trace */
      fh.ebytes    = 4;                /* number of bytes per element */
      fh.tbytes    = fh.np*fh.ebytes;  /* number of bytes per trace */
      fh.bbytes    = fh.ntraces*fh.tbytes+28;  /* number of bytes per block */
      fh.vers_id   = d->fh.vers_id;    /* software version and file_id status bits */
      fh.status    = S_DATA            /* there will be data */
                   + S_FLOAT           /* the data will be 32-bit float */
                   + S_COMPLEX         /* the data will be complex */
                   + (d->fh.status & S_DDR);   /* take the DDR flag from the fid */
      fh.nbheaders = 1;
      fp=d->fidfp;
      break;
    case DATA_FILE:
      fh.nblocks   = d->fh.nblocks*d->fh.ntraces; /* number of blocks in file */
      fh.ntraces   = 1;                /* number of traces per block */
      if (d->fn > 0)
        fh.np      = d->fn;            /* number of elements per trace */
      else
        fh.np      = d->np;            /* number of elements per trace */
      fh.ebytes    = 4;                /* number of bytes per element */
      fh.tbytes    = fh.np*fh.ebytes;  /* number of bytes per trace */
      fh.bbytes    = fh.tbytes+28;     /* number of bytes per block */
      fh.vers_id   = vers_id+DATA_FILE;  /* software version and file_id status bits */
      fh.status    = S_DATA            /* there will be data */
                   + S_SPEC            /* the data will be a spectrum */
                   + S_FLOAT           /* the data will be 32-bit float */
                   + S_COMPLEX         /* the data will be complex */
                   + (d->fh.status & S_DDR)    /* take the DDR flag from the fid */
                   + S_NP;             /* the data will have np dimension active */
      fh.nbheaders = 1;
      fp=d->datafp;
      break;
    case PHAS_FILE:
      fh.nblocks   = d->fh.nblocks*d->fh.ntraces; /* number of blocks in file */
      fh.ntraces   = 1;                /* number of traces per block */
      if (d->fn > 0)
        fh.np      = d->fn/2;          /* number of elements per trace */
      else
        fh.np      = d->np/2;          /* number of elements per trace */
      fh.ebytes    = 4;                /* number of bytes per element */
      fh.tbytes    = fh.np*fh.ebytes;  /* number of bytes per trace */
      fh.bbytes    = fh.tbytes+28;     /* number of bytes per block */
      fh.vers_id   = vers_id+PHAS_FILE;  /* software version and file_id status bits */
      fh.status    = S_DATA            /* there will be data */
                   + S_SPEC            /* the data will be a spectrum */
                   + S_FLOAT           /* the data will be 32-bit float */
/*                   + (d->fh.status & S_DDR)  // take the DDR flag from the fid */
                   + S_NP;             /* the data will have np dimension active */
      fh.nbheaders = 1;
      fp=d->phasfp;
      break;
  }

  if (reverse_byte_order) reversedfh(&fh);
  fwrite(&fh,1,sizeof(d->fh),fp);

}

void regridEPSI(struct data *d)
{
  fftw_complex *data;
  double sw, bs, w, a, b, re, im;
  int dim1,dim2,dim3,dim4,nr;
  int ndim1,ndim2,ndim3,ndim4;
  int i,j,k,l,m;
  int offset, offsetj, offsetk;
  int noffset;



#ifdef DEBUG
  struct timeval tp;
  double t1,t2;
  int rtn;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Starting regridding of CSI ...\n");
  rtn=gettimeofday(&tp, NULL);
  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fflush(stdout);
#endif

  /* Data dimensions */
 dim1=d->np/2; dim2=d->nv; dim3=d->nv2; dim4 = d->nv3; nr=d->nr;

  /* already zero filled
 if(d->fn1)dim2=(d->fn1)/2;
 if(d->fn2)dim3=(d->fn2)/2;
 if(d->fn3){
	// dim4=(d->fn3)/2;
	 dim1=(d->fn3)/2;
 }
 */

 // for epsi, np is slice and nv3 is spectral dimension but we will reorder here


  if(!spar(d,"epsi","y"))
	  return;

  sw=*val("sw",&d->p);
  bs=*val("sws",&d->p);
  w= 2.0*M_PI*bs/(sw*dim4);

  // the whole thing!
  if((data = (fftw_complex *)fftw_malloc(dim1*dim2*dim3*dim4*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
 (void) memcpy(data,d->csi_data[0], dim4 * dim3 * dim2 * dim1
				* sizeof(fftw_complex));

 ndim1=dim4;  // new spectral dim
 ndim2=dim2;
 ndim3=dim3;
 ndim4=dim1; // new spatial dim
  for(i=0;i<nr;i++){  // really better be just one
	  for(j=0;j<dim4;j++) {  // loop over spect direction
		  offsetj = j*dim3*dim2*dim1;
		  for(k=0;k<dim3;k++) {
			  offsetk = k*dim2*dim1;
			  for(l=0;l<dim2;l++){
				  offset = offsetj+offsetk+l*dim1;
				  for(m=0;m<dim1;m++){ // loop over read direction
					  noffset = m*ndim3*ndim2*ndim1 + l*ndim1 + k*ndim1*ndim2 + j;
					  a = cos(w * m * j);
					  b = sin(w * m * j);
					  re = data[offset + m][0];
					  im = data[offset + m][1];
					  d->csi_data[i][noffset][0] = re * a - im * b;
					  d->csi_data[i][noffset][1] = re * b + im * a;
				  }
			  }
		  }
	  }
  }

  d->np = 2 * ndim1;
  d->nv3 = ndim4; // swap these to reflect standard 3d csi ordering
 // if(d->fn3)
 	// d->fn3 = 2*ndim4;

  // adjust this since slices have changed
  d->nvols *= ndim4;
  d->nvols /= dim4;

// now swap nv2 and nv3 dimensions because ...?
  (void) memcpy(data,d->csi_data[0], dim4 * dim3 * dim2 * dim1
  				* sizeof(fftw_complex));

  // reset current dimensions
  dim1=ndim1;
  dim2=ndim2;
  dim3=ndim3;
  dim4=ndim4;
  // these are new dimensions post-swap
  ndim3=dim4;
  ndim4=dim3;
  for (i = 0; i < nr; i++) { // really better be just one
		for (j = 0; j < dim4; j++) {
			offsetj = j * dim3 * dim2 * dim1;
			for (k = 0; k < dim3; k++) {
				offsetk = offsetj + k * dim2 * dim1;
				noffset = k * ndim3 * ndim2 * ndim1 + j * ndim2 * ndim1;
				for (l = 0; l < dim2 * dim1; l++) {
					d->csi_data[i][noffset + l][0] = data[offsetk + l][0];
					d->csi_data[i][noffset + l][1] = data[offsetk + l][1];
				}
			}
		}
	}

	// since we swapped dim 2 and 3
    d->nv2 = ndim3;
	d->nv3 = ndim4;

	/*
   if(d->fn2)
		d->fn2 = 2*ndim3;
	if(d->fn3)
		d->fn3 = 2*ndim4;
		*/

	// adjust this since slices have changed
	d->nvols *= ndim4;
	d->nvols /= dim4;

  fftw_free(data);

  /* Set status for dimension */


#ifdef DEBUG
  rtn=gettimeofday(&tp, NULL);
  t2=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  regrid of %d x %d x %d data: took %f secs\n",dim1,dim2,dim3,t2-t1);
  fflush(stdout);
#endif

}
