/* dread.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* dread.c: Data read routines                                               */
/*                                                                           */
/* Copyright (C) 2012 Paul Kinchesh                                          */
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

int synctablesort(struct data *d, int d2, int d3, int d4, int *p2, int *p3, int *p4);

static int currentblockhead=-1;

void setblock(struct data *d,int dim)
{
  int blockdim;

  /* Figure the dim required per block */
  blockdim=dim/d->nblocks;
  if (dim%d->nblocks) blockdim++;

  /* Correct the number of blocks */
  d->nblocks=dim/blockdim;
  if (dim%blockdim) d->nblocks++;

  /* Set start position and end position */
  d->startpos=d->block*blockdim;
  d->endpos=(d->block+1)*blockdim;
  if (d->endpos>dim) d->endpos=dim;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Block %d (of %d)\n",d->block+1,d->nblocks);
  fprintf(stdout,"  Dim = %d\n",dim);
  fprintf(stdout,"  Block dim = %d (from %d to %d)\n",d->endpos-d->startpos,d->startpos,d->endpos-1);
  fflush(stdout);
#endif
}

void getblock(struct data *d,int volindex,int DCCflag)
{
  int dim1,dim2,dim3,nr;
  int i,j;
  int datatype;

  /* Check d->nvols has been set */
  if (d->nvols<1) return;

  /* Set datatype */
  datatype=0;
  if (d->fh.status & S_FLOAT) /* 32-bit float */
    datatype=FLT32;
  else if (d->fh.status & S_32) /* 32-bit int */
    datatype=INT32;
  else /* 16-bit int */
    datatype=INT16;

  /* If a previous block has been zero filled matrix size will be incorrect */
  /* Refresh from procpar values */
  for (i=0;i<d->ndim;i++) {
    if (d->dimstatus[i] & ZEROFILL) {
      setdim(d);
      break;
    }
  }

  /* Set data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;


  /* Allocate memory according to nr */
  if ((d->data = (fftw_complex ***)fftw_malloc(nr*sizeof(fftw_complex **))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Allocate memory according to block size */
  for (i=0;i<nr;i++) { /* loop over receivers */
    if ((d->data[i] = (fftw_complex **)fftw_malloc(dim3*sizeof(fftw_complex *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    for (j=0;j<dim3;j++) /* loop over dim3 (slices or 2nd phase encodes) */
      if ((d->data[i][j] = (fftw_complex *)fftw_malloc(dim2*dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  }

  /* Read according to d->type */
  switch(datatype) {
    case FLT32: /* 32-bit float */
      readfblock(d,volindex,DCCflag);
      break;
    case INT32: /* 32-bit int */
      readlblock(d,volindex,DCCflag);
      break;
    case INT16: /* 16-bit int */
      readsblock(d,volindex,DCCflag);
      break;
    default:
      break;
  } /* end datatype switch */
  /* Reset blockhead read for next volume */
  currentblockhead=-1;
  /* Flag data status */
  for (i=0;i<d->ndim;i++) d->dimstatus[i]=DATA;

}

void getnavblock(struct data *d,int volindex,int DCCflag)
{
  int dim1,dim3,dim4,nr;
  int i,j;
  int datatype;

  /* Check there are navigator scans */
  if (!d->nav) return;

  /* Check d->nvols has been set */
  if (d->nvols<1) return;

  /* Set datatype */
  datatype=0;
  if (d->fh.status & S_FLOAT) /* 32-bit float */
    datatype=FLT32;
  else if (d->fh.status & S_32) /* 32-bit int */
    datatype=INT32;
  else /* 16-bit int */
    datatype=INT16;

  /* If a previous block has been zero filled matrix size will be incorrect */
  /* Refresh from procpar values */
  for (i=0;i<d->ndim;i++) {
    if (d->dimstatus[i] & ZEROFILL) {
      setdim(d);
      break;
    }
  }

  /* Set data dimensions */
  dim1=d->np/2; dim3=d->endpos-d->startpos; nr=d->nr;dim4=d->nv3;

  /* Allocate memory according to nr */
  if ((d->data = (fftw_complex ***)fftw_malloc(nr*sizeof(fftw_complex **))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Allocate memory according to block size */
  for (i=0;i<nr;i++) { /* loop over receivers */
    if ((d->data[i] = (fftw_complex **)fftw_malloc(dim3*sizeof(fftw_complex *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    for (j=0;j<dim3;j++) /* loop over dim3 (slices or 2nd phase encodes) */
      if ((d->data[i][j] = (fftw_complex *)fftw_malloc(d->nnav*d->nseg*dim1*sizeof(fftw_complex))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  }

  /* Read according to d->type */
  switch(datatype) {
    case FLT32: /* 32-bit float */
      readnavfblock(d,volindex,DCCflag);
      break;
    case INT32: /* 32-bit int */
      readnavlblock(d,volindex,DCCflag);
      break;
    case INT16: /* 16-bit int */
      readnavsblock(d,volindex,DCCflag);
      break;
    default:
      break;
  } /* end datatype switch */
  /* Reset blockhead read for next volume */
  currentblockhead=-1;
  /* Flag data status */
  for (i=0;i<d->ndim;i++) d->dimstatus[i]=DATA;

}

int readfblock(struct data *d,int volindex,int DCCflag)
{
  float *fdata;        /* Pointer for 32-bit floating point data */
  int dim1,dim2,dim3,dim4,nr;
  int startpos,dim3index,scale;
  int i,j,k,l;
  int ioff = FALSE;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Volume %d:\n",volindex+1);
  fprintf(stdout,"  Reading 32-bit floating point data\n");
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;dim4=d->nv3;

  /* Start position */
  startpos=d->startpos;

  /* Allocate memory */
  if ((fdata = (float *)malloc(d->fh.np*d->fh.ebytes)) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

#ifdef DEBUG
  if (DCCflag) {
    fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stdout,"  Volume %d:\n",volindex);
    fprintf(stdout,"  DC correction using dbh.lvl and dbh.tlt\n");
    fflush(stdout);
  }
#endif

 /* Read nr consecutive blocks for the 'volume' */
  for (i=0;i<nr;i++) { /* loop over receivers */
   for (j=0;j<dim3;j++) { /* loop over dim3 (slices, 2nd phase encodes or traces) */
	dim3index=startpos+j;
	for (k=0;k<dim2;k++) { /* loop over phase */
	  /* Set data pointer */
	  ioff = setoffset(d, volindex, i, dim3index, k);
	  if (ioff == TRUE) {
			free(fdata);
			return (1);
	  }
	  else if (ioff == NACQ){  // just zero fill
		  for (l = 0; l < dim1; l++) {
				d->data[i][j][k * dim1 + l][0] = 0.0;
				d->data[i][j][k * dim1 + l][1] = 0.0;
		  }
	  } else {
			fread(fdata, d->fh.ebytes, d->fh.np, d->fp); // read one trace or echo
			if (reverse_byte_order)
				reverse4ByteOrder(d->fh.np, (char *) fdata);
			scale = d->bh.ctcount;
			if (scale < 1)
				scale = 1; /* don't want to divide by 0 if there's no data */
			for (l = 0; l < dim1; l++) {
				d->data[i][j][k * dim1 + l][0] = (double) fdata[2 * l]/ scale;
				d->data[i][j][k * dim1 + l][1] = (double) fdata[2 * l + 1] / scale;
				}
			if (DCCflag) {
				for (l = 0; l < dim1; l++) {
					d->data[i][j][k * dim1 + l][0]
							-= (double) d->bh.lvl / scale;
					d->data[i][j][k * dim1 + l][1]
							-= (double) d->bh.tlt / scale;
					}
				}
	  }  // end of if(NACQ) or error

	} /* end phase loop */
   } /* end dim3 loop (slice or 2nd phase encode) */
  } /* end receiver loop */

  /* Free memory */
  free(fdata);
  return(0);
}

int readlblock(struct data *d,int volindex,int DCCflag)
{
  long int *ldata;     /* Pointer for 32-bit integer data */
  int dim1,dim2,dim3,dim4,nr;
  int startpos,dim3index,scale;
  int i,j,k,l,m;
  int ioff;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Volume %d:\n",volindex);
  fprintf(stdout,"  Reading 32-bit integer data\n");
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr;dim4=d->nv3;

  /* Start position */
  startpos=d->startpos;
  m=0;
  /* Allocate memory */
  if ((ldata = (long *)malloc(d->fh.np*d->fh.ebytes)) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

#ifdef DEBUG
  if (DCCflag) {
    fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stdout,"  Volume %d:\n",volindex);
    fprintf(stdout,"  DC correction using dbh.lvl and dbh.tlt\n");
    fflush(stdout);
  }
#endif

  /* Read nr consecutive blocks for the 'volume' */
  for (i = 0; i < nr; i++) { /* loop over receivers */
		for (j = 0; j < dim3; j++) { /* loop over dim3 (slices or 2nd phase encodes) */
			dim3index = startpos + j;
			for (k = 0; k < dim2; k++) { /* loop over phase */
				/* Set data pointer */
				ioff = setoffset(d, volindex, i, dim3index, k);
				if (ioff == TRUE) {
					free(ldata);
					return (1);
				} else if (ioff == NACQ) { // just zero fill
					for (l = 0; l < dim1; l++) {
						d->data[i][j][k * dim1 + l][0] = 0;
						d->data[i][j][k * dim1 + l][1] = 0;
					}
				} else {
					fread(ldata, d->fh.ebytes, d->fh.np, d->fp); // read one trace or echo
					if (reverse_byte_order)
						reverse4ByteOrder(d->fh.np, (char *) ldata);
					scale = d->bh.ctcount;
					if (scale < 1)
						scale = 1; /* don't want to divide by 0 if there's no data */
					for (l = 0; l < dim1; l++) {
						d->data[i][j][k * dim1 + l][0] = (double) ldata[2 * l]/ scale;
						d->data[i][j][k * dim1 + l][1] = (double) ldata[2 * l + 1] / scale;
					}
					if (DCCflag) {
						for (l = 0; l < d->fh.np / 2; l++) {
							d->data[i][j][k * dim1 + l][0]
									-= (double) d->bh.lvl / scale;
							d->data[i][j][k * dim1 + l][1]
									-= (double) d->bh.tlt / scale;
						}
					}
				}
			}/* end phase loop */
		} /* end dim3 loop (slice or 2nd phase encode) */
	} /* end receiver loop */

  /* Free memory */
  free(ldata);
  return(0);
}


int readsblock(struct data *d,int volindex,int DCCflag)
{
  short int *sdata;    /* Pointer for 16-bit integer data */
  int dim1,dim2,dim3,dim4,nr;
  int startpos,dim3index,scale;
  int i,j,k,l,m;
  int ioff;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Volume %d:\n",volindex);
  fprintf(stdout,"  Reading 16-bit integer data\n");
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; dim3=d->endpos-d->startpos; nr=d->nr; dim4=d->nv3;

  /* Start slice */
  startpos=d->startpos;
  m=0;
  /* Allocate memory */
  if ((sdata = (short *)malloc(d->fh.np*d->fh.ebytes)) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

#ifdef DEBUG
  if (DCCflag) {
    fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stdout,"  Volume %d:\n",volindex);
    fprintf(stdout,"  DC correction using dbh.lvl and dbh.tlt\n");
    fflush(stdout);
  }
#endif

  /* Read nr consecutive blocks for the 'volume' */
  for (i=0;i<nr;i++) { /* loop over receivers */
	  for (j=0;j<dim3;j++) { /* loop over dim3 (slices or 2nd phase encodes) */
	dim3index=startpos+j;
	for (k = 0; k < dim2; k++) { /* loop over phase */
				/* Set data pointer */
				ioff = setoffset(d, volindex, i,  dim3index, k);
				if (ioff == TRUE) {
					free(sdata);
					return (1);
				} else if (ioff == NACQ) { // just zero fill
					for (l = 0; l < dim1; l++) {
						d->data[i][j][k * dim1 + l][0] = 0;
						d->data[i][j][k * dim1 + l][1] = 0;
					}
				} else{
					fread(sdata, d->fh.ebytes, d->fh.np, d->fp);
				if (reverse_byte_order)
					reverse2ByteOrder(d->fh.np, (char *) sdata);
				scale = d->bh.ctcount;
				if (scale < 1)
					scale = 1; /* don't want to divide by 0 if there's no data */
				for (l = 0; l < dim1; l++) {
					d->data[i][j][k * dim1 + l][0] = (double) sdata[2 * l]/ scale;
					d->data[i][j][k * dim1 + l][1] = (double) sdata[2 * l + 1]/ scale;
				}
				if (DCCflag) {
					for (l = 0; l < dim1; l++) {
						d->data[i][j][k * dim1 + l][0] -= (double) d->bh.lvl
								/ scale;
						d->data[i][j][k * dim1 + l][1] -= (double) d->bh.tlt
								/ scale;
					}
				}
			}
		} /* end phase loop */
	} /* end dim3 loop (slice or 2nd phase encode) */
} /* end receiver loop */

  /* Free memory */
  free(sdata);
  return(0);
}

int setoffset(struct data *d,int volindex,int receiver, int dim3index,int dim2index)
{
  int dim1,dim2,ns,nv2,nv3,nr;
  int index=0,trace=0,phase=0,slice=0,phase2=0,phase3=0;
  int arraydim,psscycle,d2cycle,ni,d3cycle,ni2;
  long offset=0;
  int blockindex=0;
  int i;
  int nti; /* For LookLocker */

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nseg*d->etl; ns=d->ns; nv2=d->nv2; nr=d->nr;
  nv3=d->nv3;

  /* 1D scans */
  if (im1D(d)) {
    index=volindex;
    trace=dim3index;
  }

  /* 2D scans */
  if (im2D(d)) {
    /* Allow for compressed multi-echo loop */
    index=volindex/d->ne;
    trace=volindex%d->ne;
    /* Set phase according to its actual index */
    phase=phaseindex(d,dim2index);
    /* Set slice according to its actual index */
    slice=sliceindex(d,dim3index);
  }

  /* 3D scans */
  if (im3D(d)) {
    /* For 3D scans multiple slices always give different volumes */
    /* and for 2D CSI slices are equivalent to volumes */
    ns=(int)*val("ns",&d->p); /* We can use the value of ns */
    /* Allow for compressed multi-echo loop */
    index=volindex/(d->ne*ns);
    trace=volindex%(d->ne*ns); // compressed slices


    if(d->korder){

	   if(synctablesort(d, dim2index, dim3index, -1, &phase, &phase2, NULL))
		   return(NACQ);

	   if (!d->synclist[0])
				phase = phaseindex(d, dim2index);
	   if (!d->synclist[1])
				phase2 = phase2index(d, dim3index);
    }
    else
    {
    	  /* Set phase according to its actual index */
    	    phase=phaseindex(d,dim2index);
    	    /* Set phase2 according to its actual index */
    	    phase2=phase2index(d,dim3index);
    }
  }

  /* 4D scans or 3D csi */
  if (im4D(d)) {
    /* For 3D scans multiple slices always give different volumes */
    ns=(int)*val("ns",&d->p); /* We can use the value of ns */
    /* Allow for compressed multi-echo loop */
    index=volindex/(d->ne*ns);
    trace=volindex%(d->ne*ns);

    // 3D csi
    //if((d->seqmode >= IM3DCCCCSI) && (d->seqmode <= IM3DSSSCSI))
    //   if(imCSI(d) && (im4D(d)))
    if(d->korder){

	   if(synctablesort(d, dim2index, dim3index, volindex, &phase, &phase2, &phase3))
		   return(NACQ);

	   if (!d->synclist[0])
				phase = phaseindex(d, dim2index);

	   if (!d->synclist[1])
				phase2 = phase2index(d, dim3index);

	   if (!d->synclist[2])
				phase3 = phase3index(d, volindex);

    }
    else
    {
			/* Set phase according to its actual index */
			phase = phaseindex(d, dim2index);
			/* Set phase2 according to its actual index */
			phase2 = phase2index(d, dim3index);
			phase3 = phase3index(d, volindex); /* if a table was used */
		}
  }

  /* Calculate offset according to how the expt is run */
  switch(d->seqmode) {
    case IM1D: /* 1D scans */
      blockindex=index*nr+receiver;
      /* Calculate offset to the required trace */
      offset=sizeof(d->fh)+(long)(blockindex)*d->fh.bbytes+sizeof(d->bh)+d->fh.tbytes*(long)(trace);
      break;
    case IM2DCC: /* seqcon = *ccnn */
      blockindex=index*nr+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(phase*ns*d->ne+slice*d->ne+trace);
      if (d->nav) {
        offset+=d->fh.tbytes*(long)(phase*ns*d->nnav+slice*d->nnav);
        for (i=0;i<d->nnav;i++) if (d->navpos[i]-1<=trace) offset+=d->fh.tbytes;
      }
      break;
    case IM2DCS: /* seqcon = *csnn */
      /* The standard phase encode loop cycles more slowly than any
         array elements unless d2 is in the array string */
      d2cycle=getcycle("d2",&d->a);
      if (d2cycle>0) { /* d2 is in the array string */
        ni=(int)*val("ni",&d->p);
        blockindex=(index/d2cycle)*d2cycle*ni + index%d2cycle;
        blockindex=blockindex*nr+phase*d2cycle*nr+receiver;
      } else { /* d2 is not in array string */
        arraydim=(int)*val("arraydim",&d->p);
        blockindex=index*nr+phase*arraydim/dim2+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(slice*d->ne+trace);
      if (d->nav) {
        offset+=d->fh.tbytes*(long)(slice*d->nnav);
        for (i=0;i<d->nnav;i++) if (d->navpos[i]-1<=trace) offset+=d->fh.tbytes;
      }
      break;
    case IM2DSC: /* seqcon = *scnn */
      /* Somewhat trickier as we must use 'psscycle' to account for where
         and how 'pss' appears amongst the array elements */
      if (ns > 1) { /* there is more than one slice */
        psscycle=getcycle("pss",&d->a);
        blockindex=(index/psscycle)*psscycle*ns + index%psscycle;
        blockindex=blockindex*nr+slice*psscycle*nr+receiver;
      } else { /* there's a single slice */
        blockindex=index*nr+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(phase*d->ne+trace);
      if (d->nav) {
        offset+=d->fh.tbytes*(long)(phase*d->nnav);
        for (i=0;i<d->nnav;i++) if (d->navpos[i]-1<=trace) offset+=d->fh.tbytes;
      }
      break;
    case IM2DSS: /* seqcon = *ssnn */
      /* Even trickier. The standard phase encode loop cycles more slowly
         than any array elements unless d2 is in the array string and we
         must use 'psscycle' to account for where and how 'pss' appears
         amongst the array elements */
      arraydim=(int)*val("arraydim",&d->p);
      if (ns > 1) { /* there is more than one slice */
        psscycle=getcycle("pss",&d->a);
        d2cycle=getcycle("d2",&d->a);
        if (d2cycle>0) { /* d2 is in the array string */
          ni=(int)*val("ni",&d->p);
          if (cyclefaster("d2","pss",&d->a)) {
            psscycle*=ni;
            blockindex=(index/d2cycle)*d2cycle*ni + index%d2cycle;
            blockindex=(blockindex/psscycle)*psscycle*ns + blockindex%psscycle;
          } else {
            blockindex=(index/psscycle)*psscycle*ns + index%psscycle;
            blockindex=(blockindex/d2cycle)*d2cycle*ni + blockindex%d2cycle;
          }
          blockindex=blockindex*nr+slice*psscycle*nr+phase*d2cycle*nr+receiver;
        } else { /* d2 is not in array string */
          blockindex=(index/psscycle)*psscycle*ns + index%psscycle;
          blockindex=blockindex*nr+slice*psscycle*nr+phase*arraydim/dim2+receiver;
        }
      } else { /* there's a single slice */
        d2cycle=getcycle("d2",&d->a);
        if (d2cycle>0) { /* d2 is in array string */
          ni=(int)*val("ni",&d->p);
          blockindex=(index/d2cycle)*d2cycle*ni + index%d2cycle;
          blockindex=blockindex*nr+phase*d2cycle*nr+receiver;
        } else { /* d2 is not in array string */
          blockindex=index*nr+phase*arraydim/dim2+receiver;
        }
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(trace);
      if (d->nav) {
        for (i=0;i<d->nnav;i++) if (d->navpos[i]-1<=trace) offset+=d->fh.tbytes;
      }
      break;
    case IM2DCCFSE: /* im2Dfse with seqcon = nccnn */
      blockindex=index*nr+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)((phase/d->etl)*ns*d->etl+slice*d->etl+phase%d->etl);
      break;
    case IM2DCSFSE: /* im2Dfse with seqcon = ncsnn */
      arraydim=(int)*val("arraydim",&d->p);
      d2cycle=getcycle("d2",&d->a);
      if (d2cycle>0) { /* d2 is in array string */
        ni=(int)*val("ni",&d->p);
        blockindex=(index/d2cycle)*d2cycle*ni + index%d2cycle;
        blockindex=blockindex*nr+(phase/d->etl)*d2cycle*nr+receiver;
      } else { /* d2 is not in array string */
        blockindex=index*nr+(phase/d->etl)*(arraydim*d->etl)/dim2+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(slice*d->etl+phase%d->etl);
      break;
    case IM2DSCFSE: /* im2Dfse with seqcon = nscnn */
      /* Somewhat trickier as we must use 'psscycle' to account for where
         and how 'pss' appears amongst the array elements */
      if (ns > 1) { /* there is more than one slice */
        psscycle=getcycle("pss",&d->a);
        blockindex=(index/psscycle)*psscycle*ns + index%psscycle;
        blockindex=blockindex*nr+slice*psscycle*nr+receiver;
      } else { /* there's a single slice */
        blockindex=index*nr+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(phase);
      break;
    case IM2DSSFSE: /* im2Dfse with seqcon = nssnn */
      /* Even trickier. The standard phase encode loop cycles more slowly
         than any array elements unless d2 is in the array string and we
         must use 'psscycle' to account for where and how 'pss' appears
         amongst the array elements */
      arraydim=(int)*val("arraydim",&d->p);
      if (ns > 1) { /* there is more than one slice */
        psscycle=getcycle("pss",&d->a);
        d2cycle=getcycle("d2",&d->a);
        if (d2cycle>0) { /* d2 is in the array string */
          ni=(int)*val("ni",&d->p);
          if (cyclefaster("d2","pss",&d->a)) {
            psscycle*=ni;
            blockindex=(index/d2cycle)*d2cycle*ni + index%d2cycle;
            blockindex=(blockindex/psscycle)*psscycle*ns + blockindex%psscycle;
          } else {
            blockindex=(index/psscycle)*psscycle*ns + index%psscycle;
            blockindex=(blockindex/d2cycle)*d2cycle*ni + blockindex%d2cycle;
          }
          blockindex=blockindex*nr+slice*psscycle*nr+(phase/d->etl)*d2cycle*nr+receiver;
        } else { /* d2 is not in array string */
          blockindex=(index/psscycle)*psscycle*ns + index%psscycle;
          blockindex=blockindex*nr+slice*psscycle*nr+(phase/d->etl)*(arraydim*d->etl)/dim2+receiver;
        }
      } else { /* there's a single slice */
        d2cycle=getcycle("d2",&d->a);
        if (d2cycle>0) { /* d2 is in array string */
          ni=(int)*val("ni",&d->p);
          blockindex=(index/d2cycle)*d2cycle*ni + index%d2cycle;
          blockindex=blockindex*nr+(phase/d->etl)*d2cycle*nr+receiver;
        } else { /* d2 is not in array string */
          blockindex=index*nr+(phase/d->etl)*(arraydim*d->etl)/dim2+receiver;
        }
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(phase%d->etl);
      break;
    case IM2DEPI: /* im2Depi */
      blockindex=index*nr+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(ns*phase+slice);
      break;
    case IM2DCCLL: /* LookLocker with seqcon = nccnn */
      nti=nvals("ti",&d->p);
      index=volindex/nti;
      trace=volindex%nti;
      blockindex=index*nr+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)((phase/d->etl)*nti*ns*d->etl+slice*nti*d->etl+trace*d->etl+phase%d->etl);
      break;
    case IM2DCSLL: /* LookLocker with seqcon = ncsnn */
      nti=nvals("ti",&d->p);
      index=volindex/nti;
      trace=volindex%nti;
      arraydim=(int)*val("arraydim",&d->p);
      d2cycle=getcycle("d2",&d->a);
      if (d2cycle>0) { /* d2 is in array string */
        ni=(int)*val("ni",&d->p);
        blockindex=(index/d2cycle)*d2cycle*ni + index%d2cycle;
        blockindex=blockindex*nr+(phase/d->etl)*d2cycle*nr+receiver;
      } else { /* d2 is not in array string */
        blockindex=index*nr+(phase/d->etl)*(arraydim*d->etl)/dim2+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(slice*nti*d->etl+trace*d->etl+phase%d->etl);
      break;
    case IM2DSCLL: /* LookLocker with seqcon = nscnn */
      /* Somewhat trickier as we must use 'psscycle' to account for where
         and how 'pss' appears amongst the array elements */
      nti=nvals("ti",&d->p);
      index=volindex/nti;
      trace=volindex%nti;
      if (ns > 1) { /* there is more than one slice */
        psscycle=getcycle("pss",&d->a);
        blockindex=(index/psscycle)*psscycle*ns + index%psscycle;
        blockindex=blockindex*nr+slice*psscycle*nr+receiver;
      } else { /* there's a single slice */
        blockindex=index*nr+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)((phase/d->etl)*nti*d->etl+trace*d->etl+phase%d->etl);
      break;
    case IM2DSSLL: /* LookLocker with seqcon = nssnn */
      /* Even trickier. The standard phase encode loop cycles more slowly
         than any array elements unless d2 is in the array string and we
         must use 'psscycle' to account for where and how 'pss' appears
         amongst the array elements */
      nti=nvals("ti",&d->p);
      index=volindex/nti;
      trace=volindex%nti;
      arraydim=(int)*val("arraydim",&d->p);
      if (ns > 1) { /* there is more than one slice */
        psscycle=getcycle("pss",&d->a);
        d2cycle=getcycle("d2",&d->a);
        if (d2cycle>0) { /* d2 is in the array string */
          ni=(int)*val("ni",&d->p);
          if (cyclefaster("d2","pss",&d->a)) {
            psscycle*=ni;
            blockindex=(index/d2cycle)*d2cycle*ni + index%d2cycle;
            blockindex=(blockindex/psscycle)*psscycle*ns + blockindex%psscycle;
          } else {
            blockindex=(index/psscycle)*psscycle*ns + index%psscycle;
            blockindex=(blockindex/d2cycle)*d2cycle*ni + blockindex%d2cycle;
          }
          blockindex=blockindex*nr+slice*psscycle*nr+(phase/d->etl)*d2cycle*nr+receiver;
        } else { /* d2 is not in array string */
          blockindex=(index/psscycle)*psscycle*ns + index%psscycle;
          blockindex=blockindex*nr+slice*psscycle*nr+(phase/d->etl)*(arraydim*d->etl)/dim2+receiver;
        }
      } else { /* there's a single slice */
        d2cycle=getcycle("d2",&d->a);
        if (d2cycle>0) { /* d2 is in array string */
          ni=(int)*val("ni",&d->p);
          blockindex=(index/d2cycle)*d2cycle*ni + index%d2cycle;
          blockindex=blockindex*nr+(phase/d->etl)*d2cycle*nr+receiver;
        } else { /* d2 is not in array string */
          blockindex=index*nr+(phase/d->etl)*(arraydim*d->etl)/dim2+receiver;
        }
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(trace*d->etl+phase%d->etl);
      break;
    case IM3DCC: /* seqcon = **cc* */
      blockindex=index*nr+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(phase2*dim2*ns*d->ne+phase*ns*d->ne+trace);
      if (d->nav) {
        offset+=d->fh.tbytes*(long)(phase2*dim2*ns*d->nnav+phase*ns*d->nnav);
        for (i=0;i<d->nnav;i++) if (d->navpos[i]-1<=trace) offset+=d->fh.tbytes;
      }
      break;
    case IM3DCS: /* seqcon = **cs* */
      /* The standard 2nd phase encode loop cycles more slowly than any
         array elements unless d3 is in the array string */
      d3cycle=getcycle("d3",&d->a);
      if (d3cycle>0) { /* d3 is in the array string */
        ni2=(int)*val("ni2",&d->p);
        blockindex=(index/d3cycle)*d3cycle*ni2 + index%d3cycle;
        blockindex=blockindex*nr+phase2*d3cycle*nr+receiver;
      } else { /* d3 is not in array string */
        arraydim=(int)*val("arraydim",&d->p);
        blockindex=index*nr+phase2*arraydim/nv2+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(phase*ns*d->ne+trace);
      if (d->nav) {
        offset+=d->fh.tbytes*(long)(phase*ns*d->nnav);
        for (i=0;i<d->nnav;i++) if (d->navpos[i]-1<=trace) offset+=d->fh.tbytes;
      }
      break;
    case IM3DSC: /* seqcon = **sc* */
      /* The standard phase encode loop cycles more slowly than any
         array elements unless d2 is in the array string */
      d2cycle=getcycle("d2",&d->a);
      if (d2cycle>0) { /* d2 is in the array string */
        ni=(int)*val("ni",&d->p);
        blockindex=(index/d2cycle)*d2cycle*ni + index%d2cycle;
        blockindex=blockindex*nr+phase*d2cycle*nr+receiver;
      } else { /* d2 is not in array string */
        arraydim=(int)*val("arraydim",&d->p);
        blockindex=index*nr+phase*arraydim/dim2+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(phase2*ns*d->ne+trace);
      if (d->nav) {
        offset+=d->fh.tbytes*(long)(phase2*ns*d->nnav);
        for (i=0;i<d->nnav;i++) if (d->navpos[i]-1<=trace) offset+=d->fh.tbytes;
      }
      break;
    case IM3DSS: /* seqcon = **ss* */
      /* The standard 2nd phase encode loop cycles more slowly than
         the first which cycles more slowly than any array elements
         unless d2 and/or d3 are in the array string */
      d2cycle=getcycle("d2",&d->a);
      d3cycle=getcycle("d3",&d->a);
      if (d2cycle>0 && d3cycle>0) { /* d2 and d3 are in the array string */
        ni=(int)*val("ni",&d->p);
        ni2=(int)*val("ni2",&d->p);
        if (cyclefaster("d2","d3",&d->a)) {
          d3cycle*=ni;
          blockindex=(index/d2cycle)*d2cycle*ni + index%d2cycle;
          blockindex=(blockindex/d3cycle)*d3cycle*ni2 + blockindex%d3cycle;
        } else {
          d2cycle*=ni2;
          blockindex=(index/d3cycle)*d3cycle*ni2 + index%d3cycle;
          blockindex=(blockindex/d2cycle)*d2cycle*ni + blockindex%d2cycle;
        }
        blockindex=blockindex*nr+phase2*d3cycle*nr+phase*d2cycle*nr+receiver;
      }
      else if (d2cycle>0) { /* d2 is in the array string */
        arraydim=(int)*val("arraydim",&d->p);
        ni=(int)*val("ni",&d->p);
        blockindex=(index/d2cycle)*d2cycle*ni + index%d2cycle;
        blockindex=blockindex*nr+phase2*arraydim/nv2+phase*d2cycle*nr+receiver;
      }
      else if (d3cycle>0) { /* d3 is in the array string */
        arraydim=(int)*val("arraydim",&d->p);
        ni2=(int)*val("ni2",&d->p);
        blockindex=(index/d3cycle)*d3cycle*ni2 + index%d3cycle;
        blockindex=blockindex*nr+phase2*d3cycle*nr+phase*arraydim/dim2+receiver;
      }
      else { /* Neither d2 or d3 are in the array string */
        arraydim=(int)*val("arraydim",&d->p);
        blockindex=index*nr+phase2*arraydim/nv2+phase*arraydim/(nv2*dim2)+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)trace;
      if (d->nav) {
        offset+=d->fh.tbytes*(long)(ns*d->nnav);
        for (i=0;i<d->nnav;i++) if (d->navpos[i]-1<=trace) offset+=d->fh.tbytes;
      }
      break;
    case IM3DCFSE: /* im3Dfse with seqcon = ncccn */
      blockindex=index*nr+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(phase2*dim2*ns*d->ne+(phase/d->etl)*ns*d->etl+(volindex%ns)*d->etl+phase%d->etl);
      break;
    case IM3DSFSE: /* im3Dfse with seqcon = nccsn */
      /* The standard 2nd phase encode loop cycles more slowly than any
         array elements unless d3 is in the array string */
      d3cycle=getcycle("d3",&d->a);
      if (d3cycle>0) { /* d3 is in the array string */
        ni2=(int)*val("ni2",&d->p);
        blockindex=(index/d3cycle)*d3cycle*ni2 + index%d3cycle;
        blockindex=blockindex*nr+phase2*d3cycle*nr+receiver;
      } else { /* d3 is not in array string */
        arraydim=(int)*val("arraydim",&d->p);
        blockindex=index*nr+phase2*arraydim/nv2+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)((phase/d->etl)*ns*d->etl+(volindex%ns)*d->etl+phase%d->etl);
      break;
    case IM2DSSCSI: /* im2DCSI, seqcon = "nnssn" */
        arraydim=(int)*val("arraydim",&d->p);
        blockindex=phase2*arraydim/nv2+phase*arraydim/(nv2*(d->nv))+receiver;
     //   blockindex=index*nr+phase2*arraydim/nv2+phase*arraydim/nv + receiver;
        /* Calculate offset to end of the required block */
        offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
        /* Adjust for required trace */
        offset-=d->fh.ntraces*d->fh.tbytes;
        offset+=d->fh.tbytes*(long)(trace);
        break;
    case IM2DCSCSI: /* im2DCSI, seqcon = "nncsn" */
         arraydim=(int)*val("arraydim",&d->p);
         blockindex=index*nr+phase2*arraydim/nv2+receiver;
         /* Calculate offset to end of the required block */
         offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
         /* Adjust for required trace */
         offset-=d->fh.ntraces*d->fh.tbytes;
         offset+=d->fh.tbytes*(long)(phase*ns*d->ne+trace);
         break;
    case IM2DSCCSI: /* im2DCSI, seqcon = "nnscn" */
          arraydim=(int)*val("arraydim",&d->p);
          blockindex=index*nr+phase*arraydim/dim2+receiver;
          /* Calculate offset to end of the required block */
          offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
          /* Adjust for required trace */
          offset-=d->fh.ntraces*d->fh.tbytes;
          offset+=d->fh.tbytes*(long)(phase2*ns*d->ne+trace);
          break;
    case IM2DCCCSI: /* im2DCSI, seqcon = "nnccn" */
         arraydim=(int)*val("arraydim",&d->p);
         blockindex=index*nr+receiver;
         /* Calculate offset to end of the required block */
         offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
         /* Adjust for required trace */
         offset-=d->fh.ntraces*d->fh.tbytes;
         if(d->korder)
             offset+=d->fh.tbytes*(long)(phase+trace);
         else
             offset+=d->fh.tbytes*(long)(phase2*(d->nv)*ns*d->ne+phase*ns*d->ne+trace);
         break;
    case IM3DSSSCSI: /* im3DCSI, seqcon = "nnsss" */
        arraydim=(int)*val("arraydim",&d->p);
        blockindex=index*nr+phase2*arraydim/nv2+phase*arraydim/(nv2*(d->nv))+receiver;
        if(d->korder)
        	blockindex=phase3 + receiver; // synctablesort returns the block as phase, phase2, phase3
        /* Calculate offset to end of the required block */
        offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
        /* Adjust for required trace */
        offset-=d->fh.ntraces*d->fh.tbytes;
        offset+=d->fh.tbytes*(long)trace;
        break;
    case IM3DSSCCSI: /* im3DCSI, seqcon = "nnssc" */
         arraydim=(int)*val("arraydim",&d->p);
      //   blockindex=index*nr+phase2*arraydim/nv2+phase*arraydim/(nv2*(d->nv))+receiver;
         blockindex=phase2*arraydim/nv2+phase*arraydim/(nv2*(d->nv))+receiver;
         if(d->korder)
        	 blockindex=phase2 + receiver;  // synctablesort returns the block as phase, phase2, phase3
         /* Calculate offset to end of the required block */
         offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
         /* Adjust for required trace */
         trace=trace+phase3; // it is compressed in nv3
         offset-=d->fh.ntraces*d->fh.tbytes;
         offset+=d->fh.tbytes*(long)trace;
         break;
    default:
      break;
  } /* end seqmode switch */

  /* Get block header if required */
  if (blockindex != currentblockhead) {
    if (blockindex<d->fh.nblocks) {
      getdbh(d,blockindex);
      currentblockhead=blockindex;
    } else {
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Unable to read block header %d\n",blockindex);
  fflush(stdout);
#endif
    }
  }

  /* Set offset */
  if (d->buf.st_size >= offset+d->fh.tbytes){
	  fseek(d->fp,offset,SEEK_SET);
	  // if (im4D(d)) fprintf(stderr,"trace %d block %d index %d \n",trace,blockindex, index);
	  // if (im4D(d)) fprintf(stderr,"offset %d \n",offset);
  }
  else {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Unable to set offset for ");
    if (im1D(d)) fprintf(stderr,"block %d receiver %d trace %d\n",volindex,receiver,dim3index);
    if (im2D(d)) fprintf(stderr,"volume %d receiver %d slice %d phase %d\n",volindex,receiver,dim3index,dim2index);
    if (im3D(d)) fprintf(stderr,"volume %d receiver %d phase2 %d phase %d\n",volindex,receiver,dim3index,dim2index);
    if (im4D(d)) fprintf(stderr,"volume %d receiver %d phase 3 %d phase2 %d phase %d\n",volindex,receiver,volindex, dim3index,dim2index);
  if (im4D(d)) fprintf(stderr,"trace %d block %d index %d \n",trace,blockindex, index);
    fflush(stderr);
    return(1);
  }
  return(0);
}

int readnavfblock(struct data *d,int volindex,int DCCflag)
{
  float *fdata;        /* Pointer for 32-bit floating point data */
  int dim1,dim2,dim3,nr;
  int startpos,dim3index,scale;
  int i,j,k,l,m,n;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Volume %d:\n",volindex+1);
  fprintf(stdout,"  Reading 32-bit floating point data\n");
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nseg*d->etl; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Start position */
  startpos=d->startpos;

  /* Allocate memory */
  if ((fdata = (float *)malloc(d->fh.np*d->fh.ebytes)) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

#ifdef DEBUG
  if (DCCflag) {
    fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stdout,"  Volume %d:\n",volindex);
    fprintf(stdout,"  DC correction using dbh.lvl and dbh.tlt\n");
    fflush(stdout);
  }
#endif

 /* Read nr consecutive blocks for the 'volume' */
  for (i=0;i<nr;i++) { /* loop over receivers */
    for (j=0;j<dim3;j++) { /* loop over dim3 (slices or 2nd phase encodes) */
      dim3index=startpos+j;
      for (m=0;m<d->nseg;m++) { /* loop over all segments */
        for (n=0;n<d->nnav;n++) { /* loop over navigators */
          k=m*d->nnav+n;
          /* Set data pointer */
          if (setnavoffset(d,volindex,i,dim3index,m,n)) {
            free(fdata);
            return(1);
          }
          fread(fdata,d->fh.ebytes,d->fh.np,d->fp);
          if (reverse_byte_order)
            reverse4ByteOrder(d->fh.np,(char *)fdata);
          scale=d->bh.ctcount;
          if (scale<1) scale=1; /* don't want to divide by 0 if there's no data */
          for (l=0;l<dim1;l++) {
            d->data[i][j][k*dim1+l][0]=(double)fdata[2*l]/scale;
            d->data[i][j][k*dim1+l][1]=(double)fdata[2*l+1]/scale;
          }
          if (DCCflag) {
            for (l=0;l<dim1;l++) {
              d->data[i][j][k*dim1+l][0]-=(double)d->bh.lvl/scale;
              d->data[i][j][k*dim1+l][1]-=(double)d->bh.tlt/scale;
            }
          }
        }
      }
    } /* end slice loop */
  } /* end receiver block loop */

  /* Free memory */
  free(fdata);
  return(0);
}

int readnavlblock(struct data *d,int volindex,int DCCflag)
{
  long int *ldata;     /* Pointer for 32-bit integer data */
  int dim1,dim2,dim3,nr;
  int startpos,dim3index,scale;
  int i,j,k,l,m,n;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Volume %d:\n",volindex+1);
  fprintf(stdout,"  Reading 32-bit integer data\n");
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nseg*d->etl; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Start position */
  startpos=d->startpos;

  /* Allocate memory */
  if ((ldata = (long *)malloc(d->fh.np*d->fh.ebytes)) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

#ifdef DEBUG
  if (DCCflag) {
    fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stdout,"  Volume %d:\n",volindex);
    fprintf(stdout,"  DC correction using dbh.lvl and dbh.tlt\n");
    fflush(stdout);
  }
#endif

 /* Read nr consecutive blocks for the 'volume' */
  for (i=0;i<nr;i++) { /* loop over receivers */
    for (j=0;j<dim3;j++) { /* loop over dim3 (slices or 2nd phase encodes) */
      dim3index=startpos+j;
      for (m=0;m<d->nseg;m++) { /* loop over all segments */
        for (n=0;n<d->nnav;n++) { /* loop over navigators */
          k=m*d->nnav+n;
          /* Set data pointer */
          if (setnavoffset(d,volindex,i,dim3index,m,n)) {
            free(ldata);
            return(1);
          }
          fread(ldata,d->fh.ebytes,d->fh.np,d->fp);
          if (reverse_byte_order)
            reverse4ByteOrder(d->fh.np,(char *)ldata);
          scale=d->bh.ctcount;
          if (scale<1) scale=1; /* don't want to divide by 0 if there's no data */
          for (l=0;l<dim1;l++) {
            d->data[i][j][k*dim1+l][0]=(double)ldata[2*l]/scale;
            d->data[i][j][k*dim1+l][1]=(double)ldata[2*l+1]/scale;
          }
          if (DCCflag) {
            for (l=0;l<dim1;l++) {
              d->data[i][j][k*dim1+l][0]-=(double)d->bh.lvl/scale;
              d->data[i][j][k*dim1+l][1]-=(double)d->bh.tlt/scale;
            }
          }
        }
      }
    } /* end slice loop */
  } /* end receiver block loop */

  /* Free memory */
  free(ldata);
  return(0);
}

int readnavsblock(struct data *d,int volindex,int DCCflag)
{
  short int *sdata;    /* Pointer for 16-bit integer data */
  int dim1,dim2,dim3,nr;
  int startpos,dim3index,scale;
  int i,j,k,l,m,n;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Volume %d:\n",volindex+1);
  fprintf(stdout,"  Reading 16-bit integer data\n");
  fflush(stdout);
#endif

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nseg*d->etl; dim3=d->endpos-d->startpos; nr=d->nr;

  /* Start position */
  startpos=d->startpos;

  /* Allocate memory */
  if ((sdata = (short *)malloc(d->fh.np*d->fh.ebytes)) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

#ifdef DEBUG
  if (DCCflag) {
    fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stdout,"  Volume %d:\n",volindex);
    fprintf(stdout,"  DC correction using dbh.lvl and dbh.tlt\n");
    fflush(stdout);
  }
#endif

 /* Read nr consecutive blocks for the 'volume' */
  for (i=0;i<nr;i++) { /* loop over receivers */
    for (j=0;j<dim3;j++) { /* loop over dim3 (slices or 2nd phase encodes) */
      dim3index=startpos+j;
      for (m=0;m<d->nseg;m++) { /* loop over all segments */
        for (n=0;n<d->nnav;n++) { /* loop over navigators */
          k=m*d->nnav+n;
          /* Set data pointer */
          if (setnavoffset(d,volindex,i,dim3index,m,n)) {
            free(sdata);
            return(1);
          }
          fread(sdata,d->fh.ebytes,d->fh.np,d->fp);
          if (reverse_byte_order)
            reverse2ByteOrder(d->fh.np,(char *)sdata);
          scale=d->bh.ctcount;
          if (scale<1) scale=1; /* don't want to divide by 0 if there's no data */
          for (l=0;l<dim1;l++) {
            d->data[i][j][k*dim1+l][0]=(double)sdata[2*l]/scale;
            d->data[i][j][k*dim1+l][1]=(double)sdata[2*l+1]/scale;
          }
          if (DCCflag) {
            for (l=0;l<dim1;l++) {
              d->data[i][j][k*dim1+l][0]-=(double)d->bh.lvl/scale;
              d->data[i][j][k*dim1+l][1]-=(double)d->bh.tlt/scale;
            }
          }
        }
      }
    } /* end slice loop */
  } /* end receiver block loop */

  /* Free memory */
  free(sdata);
  return(0);
}

int setnavoffset(struct data *d,int volindex,int receiver,int dim3index,int seg,int nav)
{
  int dim1,dim2,ns,nv2,nr;
  int index=0,trace=0,slice=0,phase2=0;
  int arraydim,psscycle;
  long offset=0;
  int blockindex=0;

  /* Data dimensions */
  dim1=d->np/2; dim2=d->nv; ns=d->ns; nv2=d->nv2; nr=d->nr;

  /* 1D scans */
  if (im1D(d)) {
    index=volindex;
  }

  /* 2D scans */
  if (im2D(d)) {
    /* Allow for compressed multi-echo loop */
    index=volindex/d->ne;
    /* Set slice according to its actual index */
    slice=sliceindex(d,dim3index);
  }

  /* 3D scans */
  if (im3D(d)) {
    /* For 3D scans multiple slices always give different volumes */
    ns=(int)*val("ns",&d->p); /* We can use the value of ns */
    /* Allow for compressed multi-echo loop */
    index=volindex/(d->ne*ns);
    trace=volindex%(d->ne*ns);
    /* Set phase2 according to its actual index */
    phase2=phase2index(d,dim3index);
  }

  /* Calculate offset according to how the expt is run */
  switch(d->seqmode) {
    case IM2DCC: /* seqcon = *ccnn */
      blockindex=index*nr+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)((seg*ns+slice)*(d->etl*d->ne+d->nnav));
      offset+=d->fh.tbytes*(long)(d->navpos[nav]-1+nav);
      break;
    case IM2DCS: /* seqcon = *csnn */
      /* The standard phase encode loop cycles more slowly than any
         array elements */
      arraydim=(int)*val("arraydim",&d->p);
      blockindex=index*nr+seg*arraydim/d->nseg+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(slice*(d->etl*d->ne+d->nnav));
      offset+=d->fh.tbytes*(long)(d->navpos[nav]-1+nav);
      break;
    case IM2DSC: /* seqcon = *scnn */
      /* Somewhat trickier as we must use 'psscycle' to account for where
         and how 'pss' appears amongst the array elements */
      if (ns > 1) { /* there is more than one slice */
        psscycle=getcycle("pss",&d->a);
        blockindex=(index/psscycle)*psscycle*ns + index%psscycle;
        blockindex=blockindex*nr+slice*psscycle*nr+receiver;
      } else { /* there's a single slice */
        blockindex=index*nr+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(seg*(d->etl*d->ne+d->nnav));
      offset+=d->fh.tbytes*(long)(d->navpos[nav]-1+nav);
      break;
    case IM2DSS: /* seqcon = *ssnn */
      /* Even trickier. The standard phase encode loop cycles more slowly
         than any array elements and we must use 'psscycle' to account for
         where and how 'pss' appears amongst the array elements */
      arraydim=(int)*val("arraydim",&d->p);
      if (ns > 1) { /* there is more than one slice */
        psscycle=getcycle("pss",&d->a);
        blockindex=(index/psscycle)*psscycle*ns + index%psscycle;
        blockindex=blockindex*nr+slice*psscycle*nr+seg*arraydim/d->nseg+receiver;
      } else { /* there's a single slice */
        blockindex=index*nr+seg*arraydim/d->nseg+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(d->navpos[nav]-1+nav);
      break;
    case IM2DCCFSE: /* im2Dfse with seqcon = nccnn */
      /* The same as IM2DCC ... */
      blockindex=index*nr+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)((seg*ns+slice)*(d->etl*d->ne+d->nnav));
      offset+=d->fh.tbytes*(long)(d->navpos[nav]-1+nav);
      break;
    case IM2DCSFSE: /* im2Dfse with seqcon = ncsnn */
      /* The same as IM2DCS ... */
      arraydim=(int)*val("arraydim",&d->p);
      blockindex=index*nr+seg*arraydim/d->nseg+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(slice*(d->etl*d->ne+d->nnav));
      offset+=d->fh.tbytes*(long)(d->navpos[nav]-1+nav);
      break;
    case IM2DSCFSE: /* im2Dfse with seqcon = nscnn */
      /* Somewhat trickier as we must use 'psscycle' to account for where
         and how 'pss' appears amongst the array elements */
      if (ns > 1) { /* there is more than one slice */
        psscycle=getcycle("pss",&d->a);
        blockindex=(index/psscycle)*psscycle*ns + index%psscycle;
        blockindex=blockindex*nr+slice*psscycle*nr+receiver;
      } else { /* there's a single slice */
        blockindex=index*nr+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(seg*(d->etl*d->ne+d->nnav));
      offset+=d->fh.tbytes*(long)(d->navpos[nav]-1+nav);
      break;
    case IM2DSSFSE: /* im2Dfse with seqcon = nssnn */
      /* Even trickier. The standard phase encode loop cycles more slowly
         than any array elements and we must use 'psscycle' to account for
         where and how 'pss' appears amongst the array elements */
      arraydim=(int)*val("arraydim",&d->p);
      if (ns > 1) { /* there is more than one slice */
        psscycle=getcycle("pss",&d->a);
        blockindex=(index/psscycle)*psscycle*ns + index%psscycle;
        blockindex=blockindex*nr+slice*psscycle*nr+seg*arraydim/d->nseg+receiver;
      } else { /* there's a single slice */
        blockindex=index*nr+seg*arraydim/d->nseg+receiver;
      }
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(d->navpos[nav]-1+nav);
      break;
    case IM3DCC: /* seqcon = **cc* */
      blockindex=index*nr+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)((phase2*dim2*ns+seg*ns+trace)*(d->etl*d->ne+d->nnav));
      offset+=d->fh.tbytes*(long)(d->navpos[nav]-1+nav);
      break;
    case IM3DCS: /* seqcon = **cs* */
      arraydim=(int)*val("arraydim",&d->p);
      blockindex=index*nr+phase2*arraydim/nv2+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)((seg*ns+trace)*(d->etl*d->ne+d->nnav));
      offset+=d->fh.tbytes*(long)(d->navpos[nav]-1+nav);
      break;
    case IM3DSC: /* seqcon = **sc* */
      arraydim=(int)*val("arraydim",&d->p);
      blockindex=index*nr+seg*arraydim/d->nseg+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)((phase2*ns+trace)*(d->etl*d->ne+d->nnav));
      offset+=d->fh.tbytes*(long)(d->navpos[nav]-1+nav);
      break;
    case IM3DSS: /* seqcon = **ss* */
      arraydim=(int)*val("arraydim",&d->p);
      blockindex=index*nr+phase2*arraydim/nv2+seg*arraydim/d->nseg+receiver;
      /* Calculate offset to end of the required block */
      offset=sizeof(d->fh)+(long)(blockindex+1)*d->fh.bbytes;
      /* Adjust for required trace */
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset-=d->fh.ntraces*d->fh.tbytes;
      offset+=d->fh.tbytes*(long)(trace*(d->etl*d->ne+d->nnav));
      offset+=d->fh.tbytes*(long)(d->navpos[nav]-1+nav);
      break;
    default:
      break;
  } /* end seqmode switch */

  /* Get block header if required */
  if (blockindex != currentblockhead) {
    if (blockindex<d->fh.nblocks) {
      getdbh(d,blockindex);
      currentblockhead=blockindex;
    } else {
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Unable to read block header %d\n",blockindex);
  fflush(stdout);
#endif
    }
  }

  /* Set offset */
  if (d->buf.st_size >= offset+d->fh.tbytes) fseek(d->fp,offset,SEEK_SET);
  else {
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Unable to set offset for volume %d receiver %d phase2/slice %d segment %d navigator %d\n",
    volindex,receiver,dim3index,seg,nav);
  fflush(stdout);
#endif
    return(1);
  }
  return(0);
}

int sliceindex(struct data *d,int slice)
{
  int i;

  if (d->pssorder[0] == -1) return(slice);

  for (i=0;i<d->ns;i++)
    if (d->pssorder[i] == slice) return(i);

  return(-1);
}

int segindex(struct data *d,int phase)
{
  int i;

  if (d->dim2order[0] == -1) return(phase/d->etl);

  for (i=0;i<d->nseg*d->etl;i++)
    if (d->dim2order[i] == phase) return(i/d->etl);

  return(-1);
}

int phaseindex(struct data *d,int phase)
{
  int i;

  if (d->dim2order[0] == -1) return(phase);

  for (i=0;i<d->nseg*d->etl;i++)
    if (d->dim2order[i] == phase) return(i);

  return(-1);
}

int phase2index(struct data *d,int phase2)
{
  int i;

  if (d->dim3order[0] == -1) return(phase2);

  for (i=0;i<d->nv2;i++)
    if (d->dim3order[i] == phase2) return(i);

  return(-1);
}

int phase3index(struct data *d,int phase3)
{
  int i;

  if (d->dim4order[0] == -1) return(phase3);

  for (i=0;i<d->nv3;i++)
    if (d->dim4order[i] == phase3) return(i);

  return(-1);
}

int synctablesort(struct data *d, int d2, int d3, int d4, int *p2, int *p3, int *p4)
{
	  int i;
	  int tablen=0;
	  int threeD=FALSE;
	  int d2match = FALSE;
	  int d3match = FALSE;
	  int d4match = FALSE;


	#ifdef DEBUG
	  struct timeval tp;
	  double t1;
	  int rtn;
	  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
	  rtn=gettimeofday(&tp, NULL);
	  t1=(double)tp.tv_sec+(1.e-6)*tp.tv_usec;
	  fflush(stdout);
	#endif

	  if(d4 >-1)
		  threeD=TRUE;


	  if(d->synclist[0])
		  tablen=nvals("pelist",&d->p);
	  else if(d->synclist[1])
		  tablen=nvals("pe2list",&d->p);
	  else if(threeD && (d->synclist[2]))
		  tablen=nvals("pe3list",&d->p);

	  i = -1;
	  while( !(d2match && d3match && d4match) && (i++ < tablen))
	  {

		  d2match=(d->synclist[0]) ? FALSE:TRUE;
		  d3match=(d->synclist[1]) ? FALSE:TRUE;
		  d4match=(d->synclist[2]) ? FALSE:TRUE;
		  if(!threeD)d4match=TRUE;  // satisfy this condition

		  if(d->dim2order[i] == d2)
		  {
			  d2match = TRUE;
			  if(d->dim3order[i] == d3)
				  d3match = TRUE;
			  if(threeD){
				  if(d->dim4order[i] == d4)
				  	d4match = TRUE;
			  }
		  }
	  }
	  if(i < tablen)
	  {
		  if(d->synclist[0])
			  *p2 = i;
		  if(d->synclist[1])
			  *p3 = i;
		  if (threeD) {
			if (d->synclist[2])
				*p4 = i;
		}
	  }
	  else
		  return(1); // this means we didn't find it


	return(0);

}
