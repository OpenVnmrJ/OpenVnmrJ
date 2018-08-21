/* dutils.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* dutils.c: Data utilities                                                  */
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

void opendata(char *datafile,struct data *d)
{
  /* Initialize data defaults */
  initdata(d);

  /* Check status of data file */
  if (stat(datafile,&d->buf) == -1) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Unable to access %s\n",datafile);
    fprintf(stderr,"  Aborting ...\n\n");
    fflush(stderr);
    exit(1);
  }
  /* Check to see if input file can be opened */
  if ((d->fp=fopen(datafile,"r")) == NULL) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Unable to open %s\n\n",datafile);
    fprintf(stderr,"  Aborting ...\n\n");
    fflush(stderr);
    exit(1);
  }
  /* Exit if input file size is less than the VNMR file header size */
  if (d->buf.st_size < sizeof(d->fh)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Incomplete file header (?)\n");
    fprintf(stderr,"  Aborting ...\n\n");
    fflush(stderr);
    exit(1);
  }
  /* Copy filename */
  if ((d->file = (char *)malloc((strlen(datafile)+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  strcpy(d->file,datafile);

  /* Get file header */
  getdfh(d);

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  %s contains %d bytes\n",d->file,(int)d->buf.st_size);
  fflush(stdout);
#endif
}

void setnvols(struct data *d)
{
  int nvols;
  int listlen;
  char seqcon[6];

  /* To figure the number of volumes insist we must have the following */
  if ((ptype("np",&d->p) < 0) || (ptype("nv",&d->p) < 0)
    || (ptype("nv2",&d->p) < 0) || (ptype("pss",&d->p) < 0) 
    || (ptype("rcvrs",&d->p) < 0) || (ptype("seqcon",&d->p) < 0) 
    || (ptype("arraydim",&d->p) < 0) || (ptype("nf",&d->p) < 0)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  The following values are required from a 'procpar':\n");
    fprintf(stderr,"  np nv nv2 pss rcvrs seqcon arraydim nf\n\n");
    fprintf(stderr," Aborting ...\n\n");
    fflush(stderr);
    exit(1);
  }

  if (d->seqmode>=IM4D) {  /* 4D or 3D CSI */
    if(ptype("nv3",&d->p) < 0)
      {
	fprintf(stderr,"  Invalid value for nv3 in procpar \n\n");
	fprintf(stderr,"  Aborting ...\n\n");
	fflush(stderr);
	exit(1);
      }
  }

  /* Sanity check */
  if ((d->np != d->fh.np) || ((int)*val("nf",&d->p) != d->fh.ntraces)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Check data file and procpar file are compatible\n");
    fprintf(stderr,"  procpar np = %d and nf = %d\n",d->np,(int)*val("nf",&d->p));
    fprintf(stderr,"  fid     np = %d and ntraces= %d\n",d->fh.np,d->fh.ntraces);
    fprintf(stderr,"  Aborting ...\n\n");
    fflush(stderr);
    exit(1);
  }

  /* Use value of arraydim to calculate the total number of volumes */
  nvols=(int)*val("arraydim",&d->p);
  /* Multiple echoes generate 'ne' volumes that are not arrayed */
  nvols*=d->ne;
  /* Multiple receivers generate 'nr' volumes included in arraydim */
  if (nvols%d->nr != 0) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Problem calculating number of volumes\n");
    fprintf(stderr,"  Check data file and procpar file are compatible\n");
    fprintf(stderr,"  Aborting ...\n\n");
    fflush(stderr);
    exit(1);
  }
  nvols/=d->nr;

  /* Check for 'standard' slice and phase loops as these
     generate volumes included in arraydim */
  strcpy(seqcon,*sval("seqcon",&d->p));
  if (d->seqmode<IM3D) {  /* 2D */
    /* 'standard' slice loop has pss arrayed */
    if (seqcon[1] == 's') nvols/=d->ns;
  } else { /* 3D */
    /* 'compressed' slice loop generates 'ns' extra volumes  */
    if (seqcon[1] == 'c') nvols*=d->ns;
  }

  if (seqcon[2] == 's') /* 'standard' phase loop */
    nvols/=d->nseg;
  if (seqcon[3] == 's') /* 'standard' phase loop */
    nvols/=d->nv2;
  if (seqcon[4] == 's') /* volumes = slice encodes for 3D csi */
    nvols/=d->nv3;

  /* Allow for 2D LookLocker */
  if (im2DLL(d)) nvols*=nvals("ti",&d->p);

  /* special case for CSI and elliptical acquires NOT using DI's functions */
  if(imCSI(d) && nvols < 1)
  {
	  d->korder = TRUE;
	  nvols=(int)*val("arraydim",&d->p);
	  nvols*=d->ne;
	  nvols /= d->nr;
	  listlen = nvals("pelist",&d->p);
	  if(listlen > 0)
		  nvols /= listlen;

	 if(im4D(d))  // 3D csi
		  nvols *= d->nv3;  // for 3d nvols is number of slices
	  else
		  nvols *= d->ns;
  }
  else if(imCSI(d) && (im4D(d)))
  {
	  nvols *= d->nv3;
  }

  /* We now have the total number of volumes (nvols) according to
     the input parameter set */

  /* Set nvols */
  d->nvols=nvols;

  /* Set start volume and end volume */
  setstartendvol(d);

  /* Initialise volume counter */
  d->vol=d->startvol;

  /* Initialise block counter */
  d->block=0;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Total number of volumes (d->nvols) = %d\n",d->nvols);
  fprintf(stdout,"  Start volume (d->startvol+1) = %d\n",d->startvol+1);
  fprintf(stdout,"  End volume (d->endvol) = %d\n",d->endvol);
  fflush(stdout);
#endif
}

void setstartendvol(struct data *d)
{
  if (spar(d,"allvolumes","n")) {  /* Don't process all volumes */
    d->startvol=(int)*val("startvol",&d->p)-1;
    d->endvol=(int)*val("endvol",&d->p);
    if (d->startvol > d->endvol) {
      /* Swap them */
      d->vol=d->endvol; d->endvol=d->startvol; d->startvol=d->vol;
    }
    if (d->startvol < 0) d->startvol=0;
    if (d->endvol > d->nvols) d->endvol=d->nvols;
  } else { /* Process all volumes */
    d->startvol=0;
    d->endvol=d->nvols;
  }
}

void setdatapars(struct data *d)
{
  int i;
  char rcvrs[MAXRCVRS];

  /* Set the sequence mode from seqcon and apptype parameters */
  setseqmode(d);

  /* Set data dimensions */
  setdim(d);

  /* Number of segments and echo train length */
  /* Number of segments takes precidence, as in the setloop macro */
  d->nseg=(int)*val("nseg",&d->p);
  if (d->nseg>0) {
    d->etl=d->nv/d->nseg;
    if (d->nv%d->nseg>0) d->etl++;
  } else {
    d->etl=(int)*val("etl",&d->p);
    if (d->etl>0) {
      d->nseg=d->nv/d->etl;
      if (d->nv%d->etl>0) d->nseg++;
    }
  }
  if (d->nseg<1) {
    d->nseg=d->nv;
    d->etl=1;
  }

  /* Number of echoes */
  d->ne=(int)*val("ne",&d->p);
  if (d->ne < 1) d->ne=1; /* Set ne to 1 if 'ne' does not exist */

  /* Number of receivers */
  strcpy(rcvrs,*sval("rcvrs",&d->p));
  d->nr=0;
  for (i=0;i<strlen(rcvrs);i++)
    if (rcvrs[i] == 'y') d->nr++;

  /* Number of pss values = slices */
  d->ns=nvals("pss",&d->p);

  /* There must be at least one block per volume */
  d->nblocks=(int)*val("nblocks",&d->p);
  if (!d->nblocks) d->nblocks++;

  /* Number of points and views for resizing data */
  d->fn=(int)*val("fn",&d->p);
  d->fn1=(int)*val("fn1",&d->p);
  d->fn2=(int)*val("fn2",&d->p);
  d->fn3=(int)*val("fn3",&d->p);

  /* for cropping csi result */
  d->startd1 =(int)*val("snv",&d->p);
  d->startd1 += -1;
  d->startd2 =(int)*val("snv2",&d->p);
  d->startd2 += -1;
  d->cropd1 =(int)*val("cnv",&d->p);
  d->cropd2 =(int)*val("cnv2",&d->p);

  // for reversal of dimensions in csi
  d->d1rev = (int)*val("d1rev",&d->p);
  d->d2rev = (int)*val("d2rev",&d->p);
  d->d3rev = (int)*val("d3rev",&d->p);

  /* Navigators */
  d->nav=FALSE; /* Default is no navigator */
  if (spar(d,"navigator","y")) {
    d->nav=TRUE;
    d->nnav=nvals("nav_echo",&d->p);
    if (d->nnav>0) {
      if ((d->navpos = (int *)malloc(d->nnav*sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      for (i=0;i<d->nnav;i++) d->navpos[i]=(int)val("nav_echo",&d->p)[i];
    }
  }

  /* Set profile flag */
  d->profile=FALSE;
  if (spar(d,"profile","y") && im2D(d)) d->profile=TRUE;
  if (spar(d,"profile","yy") && im3D(d)) d->profile=TRUE;

  /* Set proj2D flag */
  d->proj2D=FALSE;
  if (im3D(d)) {
    if (spar(d,"profile","yn") || spar(d,"profile","ny")) d->proj2D=TRUE;
  }

  /* Set number of dimensions */
  /* 1Ds are just 1D */
  if (im1D(d)) d->ndim=1;
  /* Multislice 2Ds and 3Ds both work on volumes, i.e. 3D */
  else if (im2D(d)) d->ndim=3;
  else if (im3D(d)) d->ndim=3;

  /* Set default status of each dimension to flag no data */
  if ((d->dimstatus = (int *)malloc(d->ndim*sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for (i=0;i<d->ndim;i++) d->dimstatus[i]=NONE;

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  d->nseg    = %d\n",d->nseg);
  fprintf(stdout,"  d->etl     = %d\n",d->etl);
  fprintf(stdout,"  d->ne      = %d\n",d->ne);
  fprintf(stdout,"  d->nr      = %d\n",d->nr);
  fprintf(stdout,"  d->ns      = %d\n",d->ns);
  fprintf(stdout,"  d->fn      = %d\n",d->fn);
  fprintf(stdout,"  d->fn1     = %d\n",d->fn1);
  fprintf(stdout,"  d->fn2     = %d\n",d->fn2);
  if (d->nav) {
    fprintf(stdout,"  d->nnav    = %d",d->nnav);
    fprintf(stdout,", d->navpos = %d",d->navpos[0]);
    for (i=1;i<d->nnav;i++) fprintf(stdout,",%d",d->navpos[i]);
    fprintf(stdout,"\n");
  }
  if (d->profile) fprintf(stdout,"  d->profile = TRUE\n");
  if (d->proj2D)  fprintf(stdout,"  d->proj2D  = TRUE\n");
  fflush(stdout);
#endif
}

void setseqmode(struct data *d)
{
  char seqcon[6];

  /* To figure the sequence mode insist we must have the following */
  if ((ptype("seqcon",&d->p) < 0) || (ptype("apptype",&d->p) < 0)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  The 'procpar' must contain 'seqcon' and 'apptype' parameters:\n\n");
    fprintf(stderr,"  Aborting ...\n\n");
    fflush(stderr);
    exit(1);
  }

  /* Set sequence mode */
  d->seqmode=0;
  /* Standard cases */
  strcpy(seqcon,*sval("seqcon",&d->p));
  if ((seqcon[3] == 'n') && (seqcon[4] == 'n')) { /* standard 2D multislice */
    if ((seqcon[1] == 'c') && (seqcon[2] == 'c')) d->seqmode=IM2DCC;
    if ((seqcon[1] == 'c') && (seqcon[2] == 's')) d->seqmode=IM2DCS;
    if ((seqcon[1] == 's') && (seqcon[2] == 'c')) d->seqmode=IM2DSC;
    if ((seqcon[1] == 's') && (seqcon[2] == 's')) d->seqmode=IM2DSS;
  } else { /* standard 3D */
    if ((seqcon[2] == 'c') && (seqcon[3] == 'c')) d->seqmode=IM3DCC;
    if ((seqcon[2] == 'c') && (seqcon[3] == 's')) d->seqmode=IM3DCS;
    if ((seqcon[2] == 's') && (seqcon[3] == 'c')) d->seqmode=IM3DSC;
    if ((seqcon[2] == 's') && (seqcon[3] == 's')) d->seqmode=IM3DSS;
  }
  /* Special cases */
  if (spar(d,"apptype","im1Dglobal")) d->seqmode=IM1D;
  if (spar(d,"apptype","im1D")) d->seqmode=IM1D;
  if (spar(d,"apptype","im2Dfse")) {
    if ((seqcon[1] == 'c') && (seqcon[2] == 'c')) d->seqmode=IM2DCCFSE;
    if ((seqcon[1] == 'c') && (seqcon[2] == 's')) d->seqmode=IM2DCSFSE;
    if ((seqcon[1] == 's') && (seqcon[2] == 'c')) d->seqmode=IM2DSCFSE;
    if ((seqcon[1] == 's') && (seqcon[2] == 's')) d->seqmode=IM2DSSFSE;
  }
  if (spar(d,"apptype","im2Depi")) d->seqmode=IM2DEPI;
  if (im2DLL(d)) { /* 2D LookLocker */
    if ((seqcon[1] == 'c') && (seqcon[2] == 'c')) d->seqmode=IM2DCCLL;
    if ((seqcon[1] == 'c') && (seqcon[2] == 's')) d->seqmode=IM2DCSLL;
    if ((seqcon[1] == 's') && (seqcon[2] == 'c')) d->seqmode=IM2DSCLL;
    if ((seqcon[1] == 's') && (seqcon[2] == 's')) d->seqmode=IM2DSSLL;
  }
  if (spar(d,"apptype","im3Dfse")) {
    if (seqcon[3] == 'c') d->seqmode=IM3DCFSE;
    if (seqcon[3] == 's') d->seqmode=IM3DSFSE;
  }
  if (spar(d,"apptype","im2Dcsi")) {
	  if(seqcon[2] == 'c'){
		  if(seqcon[3] == 's')
			  d->seqmode=IM2DCSCSI;
		  if(seqcon[3] == 'c')
			  d->seqmode=IM2DCCCSI;
	  }
	  if(seqcon[2] == 's'){
		  if(seqcon[3] == 'c')
			  d->seqmode=IM2DSCCSI;
		  if(seqcon[3] == 's')
			  d->seqmode=IM2DSSCSI;
	  }
  }
  if (spar(d,"apptype","im3Dcsi")) {
	    if ((seqcon[2] == 'c') && (seqcon[3] == 'c')) {
					if (seqcon[4] == 's')d->seqmode=IM3DCCSCSI;
					else d->seqmode=IM3DCCCCSI;
	    }
	    if ((seqcon[2] == 'c') && (seqcon[3] == 's')) {
			if (seqcon[4] == 's')d->seqmode=IM3DCSSCSI;
			else d->seqmode=IM3DCSCCSI;
	    }
	    if ((seqcon[2] == 's') && (seqcon[3] == 'c')) {
			if (seqcon[4] == 's')d->seqmode=IM3DSCSCSI;
			else d->seqmode=IM3DSCCCSI;
	    }
	    if ((seqcon[2] == 's') && (seqcon[3] == 's'))  {
			if (seqcon[4] == 's')d->seqmode=IM3DSSSCSI;
			else d->seqmode=IM3DSSCCSI;
	    }
  }
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  Sequence mode set to ");
  switch (d->seqmode) {
    case IM1D:      fprintf(stdout,"1D\n"); break;
    case IM2DCC:    fprintf(stdout,"2D with seqcon='*ccnn'\n"); break;
    case IM2DCS:    fprintf(stdout,"2D with seqcon='*csnn'\n"); break;
    case IM2DSC:    fprintf(stdout,"2D with seqcon='*scnn'\n"); break;
    case IM2DSS:    fprintf(stdout,"2D with seqcon='*ssnn'\n"); break;
    case IM2DCCFSE: fprintf(stdout,"2D with appmode='im2Dfse' and seqcon='nccnn'\n"); break;
    case IM2DCSFSE: fprintf(stdout,"2D with appmode='im2Dfse' and seqcon='ncsnn'\n"); break;
    case IM2DSCFSE: fprintf(stdout,"2D with appmode='im2Dfse' and seqcon='nscnn'\n"); break;
    case IM2DSSFSE: fprintf(stdout,"2D with appmode='im2Dfse' and seqcon='nssnn'\n"); break;
    case IM2DEPI:   fprintf(stdout,"2D with appmode='im2Depi'\n"); break;
    case IM2DCCLL:  fprintf(stdout,"2D with recontype='LookLocker' and seqcon='nccnn'\n"); break;
    case IM2DCSLL:  fprintf(stdout,"2D with recontype='LookLocker' and seqcon='ncsnn'\n"); break;
    case IM2DSCLL:  fprintf(stdout,"2D with recontype='LookLocker' and seqcon='nscnn'\n"); break;
    case IM2DSSLL:  fprintf(stdout,"2D with recontype='LookLocker' and seqcon='nssnn'\n"); break;
    case IM3DCC:    fprintf(stdout,"3D with seqcon='**ccn'\n"); break;
    case IM3DCS:    fprintf(stdout,"3D with seqcon='**csn'\n"); break;
    case IM3DSC:    fprintf(stdout,"3D with seqcon='**scn'\n"); break;
    case IM3DSS:    fprintf(stdout,"3D with seqcon='**ssn'\n"); break;
    case IM3DCFSE:  fprintf(stdout,"3D with appmode='im3Dfse' and seqcon='ncccn'\n"); break;
    case IM3DSFSE:  fprintf(stdout,"3D with appmode='im3Dfse' and seqcon='nccsn'\n"); break;
    default: break;
  }
  fflush(stdout);
#endif
}

void setdim(struct data *d)
{
  /* Number of points and views */
  d->np=(int)*val("np",&d->p);
  d->nv=(int)*val("nv",&d->p);
  d->nv2=(int)*val("nv2",&d->p);
  d->nv3=(int)*val("nv3",&d->p);
  if (d->nv == 0) d->nv=1;
  if (d->nv2 == 0) d->nv2=1;
  if (d->nv3 == 0) d->nv3=1;
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  d->np      = %d\n",d->np);
  fprintf(stdout,"  d->nv      = %d\n",d->nv);
  fprintf(stdout,"  d->nv2     = %d\n",d->nv2);
#endif
}

int spar(struct data *d,char *par,char *str)
{
  if (!(strcmp(*sval(par,&d->p),str))) return(TRUE);
  return(FALSE);
}

int im1D(struct data *d)
{
  if ((IM1D <= d->seqmode) && (d->seqmode < IM2D)) return(1);
  else return(0);
}

int im2D(struct data *d)
{
  if ((IM2D <= d->seqmode) && (d->seqmode < IM3D)) return(1);
  else return(0);
}

int im2DLL(struct data *d)
{
  if (spar(d,"recontype","LookLocker")) return(1);
  else return(0);
}

int im3D(struct data *d)
{
  if ((IM3D <= d->seqmode) && (d->seqmode < IM4D)) return(1);
  else return(0);
}

int im4D(struct data *d)
{
  if ((IM4D <= d->seqmode) && (d->seqmode < MAXDIM)) return(1);
  else return(0);
}

int imCSI(struct data *d)
{
	 if ((strstr(*sval("apptype",&d->p),"csi"))) return(TRUE);
	 else  return(FALSE);
}

double getelem(struct data *d,char *par,int image)
{
  double *dbl;
  int cycle,n,id;
  dbl=val(par,&d->p);
  if (arraycheck(par,&d->a)) { /* parameter is arrayed */
    cycle=getcycle(par,&d->a);
    n=nvals(par,&d->p);
    id=(image/cycle)%n;
    return(dbl[id]);
  } else { /* par is not in array string */
    n=nvals(par,&d->p);
    if (n>1) {
      id=image%n; /* assume cycle=1 */
      return(dbl[id]);
    } else {
      return(dbl[0]);
    }
  }
}

int wprocpar(struct data *d,char *filename)
{
  FILE *fp1,*fp2;
  struct stat buf;
  char chardata;

  /* Return if a procpar file already exists */
  if (!stat(filename,&buf)) return(0);

  if ((fp1=fopen(d->procpar,"r")) == NULL) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Unable to open procpar file %s\n",d->procpar);
    fflush(stderr);
    return(1);
  }
  if ((fp2=fopen(filename,"w")) == NULL) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Unable to write procpar file %s\n",filename);
    fflush(stderr);
    return(1);
  }

  while ((fscanf(fp1,"%c",&chardata)) != EOF) fprintf(fp2,"%c",chardata);

  fclose(fp1);
  fclose(fp2);

  return(0);
}

void copydbh(struct datablockhead *dbh1,struct datablockhead *dbh2)
{
   dbh2->scale   = dbh1->scale;   /* scaling factor */
   dbh2->status  = dbh1->status;  /* status of data in block */
   dbh2->index   = dbh1->index;   /* block index */
   dbh2->mode    = dbh1->mode;    /* mode of data in block */
   dbh2->ctcount = dbh1->ctcount; /* ct value for FID */
   dbh2->lpval   = dbh1->lpval;   /* F2 left phase in phasefile */
   dbh2->rpval   = dbh1->rpval;   /* F2 right phase in phasefile */
   dbh2->lvl     = dbh1->lvl;     /* F2 level drift correction */
   dbh2->tlt     = dbh1->tlt;     /* F2 tilt drift correction */
}

void setfile(struct file *datafile,char *datadir)
{
  datafile->nfiles=1;

  if ((datafile->fid = (char **)malloc(sizeof(char *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((datafile->procpar = (char **)malloc(sizeof(char *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Force '*.fid' and '*.fid/' input names to be '*.fid/fid'
     and guess 'procpar' location */
  if ((strcmp(&datadir[strlen(datadir)-4],".fid") == 0)) {
    if ((datafile->fid[0] = (char *)malloc((strlen(datadir)+5)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    if ((datafile->procpar[0] = (char *)malloc((strlen(datadir)+9)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    strcpy(datafile->fid[0],datadir);
    strcat(datafile->fid[0],"/fid");
    strcpy(datafile->procpar[0],datadir);
    strcat(datafile->procpar[0],"/procpar");
  }
  else if ((strcmp(&datadir[strlen(datadir)-5],".fid/") == 0)) {
    if ((datafile->fid[0] = (char *)malloc((strlen(datadir)+4)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    if ((datafile->procpar[0] = (char *)malloc((strlen(datadir)+8)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    strcpy(datafile->fid[0],datadir);
    strcat(datafile->fid[0],"fid");
    strcpy(datafile->procpar[0],datadir);
    strcat(datafile->procpar[0],"procpar");
  }
  else if ((strcmp(&datadir[strlen(datadir)-8],".fid/fid") == 0)) {
    if ((datafile->fid[0] = (char *)malloc((strlen(datadir)+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    if ((datafile->procpar[0] = (char *)malloc((strlen(datadir)+5)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    strcpy(datafile->fid[0],datadir);
    strcpy(datafile->fid[0],datadir);
    datafile->procpar[0][strlen(datafile->fid[0])-3]=0; /* NULL terminate after '/' */
    strcat(datafile->procpar[0],"procpar");
  }
}

void setreffile(struct file *datafile,struct data *d,char *refpar)
{
  char filename[MAXPATHLEN],refname[MAXPATHLEN];
  int i,filenamectr=0,refnamectr=0;

  /* Get the reference name from the reference parameter */
  strcpy(refname,*sval(refpar,&d->p));

  /* If recon is straight after acquisition ... */
  if (vnmrj_recon && spar(d,"file","exp")) {
      /* Use the reference name as is */
      setfile(datafile,refname);
  } else {
    if (vnmrj_recon) { /* Recon from within VnmrJ */
      /* Use the path as defined in the parameter "file" */
      strcpy(filename,*sval("file",&d->p));
    } else { /* Recon outside of VnmrJ */
      /* Use the path as defined in d->file */
      strcpy(filename,d->file);
      filename[strlen(filename)-8]=0;  /* NULL terminate to remove .fid/fid */
    }
    /* Now use the path as defined in filename */
    for (i=0;i<strlen(filename);i++)
      if (filename[i] == '/') filenamectr=i;
    if (filenamectr>0) filenamectr++;
    for (i=0;i<strlen(refname);i++)
      if (refname[i] == '/') refnamectr=i;
    if (refnamectr>0) refnamectr++;
    for (i=refnamectr;i<strlen(refname);i++) {
      filename[filenamectr]=refname[i];
      filenamectr++;
    }
    filename[filenamectr]=0; /* NULL terminate */
    setfile(datafile,filename);
  }

}

void setfn(struct data *d1,struct data *d2,double multiplier)
{
  int fn;
  if (d2->fn==0) fn=d2->np;
  else fn=d2->fn;
  fn=round2int(multiplier*fn);
  setval(&d1->p,"fn",fn);
  d1->fn=fn;
}

void setfn1(struct data *d1,struct data *d2,double multiplier)
{
  int fn1;
  if (d2->fn1==0) fn1=d2->nv*2;
  else fn1=d2->fn1;
  fn1=round2int(multiplier*fn1);
  setval(&d1->p,"fn1",fn1);
  d1->fn1=fn1;
}

void setdimstatus(struct data *d,struct dimstatus *status,int block)
{
  int i;

  /* If it's the first block store dimstatus, otherwise refresh it */
  if (block>0) { /* Refresh */
    if (status->dimstatus && (d->ndim == status->ndim)) {
      for (i=0; i<status->ndim; i++)
        d->dimstatus[i] = status->dimstatus[i];
    }
  } else {
    status->ndim=d->ndim;
    if (d->dimstatus) {
      free(status->dimstatus);
      if ((status->dimstatus = malloc(status->ndim*sizeof(*(status->dimstatus)))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      for (i=0; i<status->ndim; i++)
        status->dimstatus[i] = d->dimstatus[i];
    }
    else
      status->dimstatus = NULL;
  }

}

void copynblocks(struct data *d1,struct data *d2)
{
  copypar("nblocks",&d1->p,&d2->p);
  d2->nblocks=d1->nblocks;
}

void copymaskpars(struct data *d1,struct data *d2)
{
  copypar("imMK",&d1->p,&d2->p);
  copypar("masknoisefrac",&d1->p,&d2->p);
  copypar("maskstartslice",&d1->p,&d2->p);
  copypar("maskendslice",&d1->p,&d2->p);
  copypar("maskrcvrs",&d1->p,&d2->p);
  copypar("maskwlb",&d1->p,&d2->p);
  copypar("maskwgf",&d1->p,&d2->p);
  copypar("maskwsb",&d1->p,&d2->p);
  copypar("masklb",&d1->p,&d2->p);
  copypar("masklb1",&d1->p,&d2->p);
  copypar("maskgf",&d1->p,&d2->p);
  copypar("maskgf1",&d1->p,&d2->p);
  copypar("masksb",&d1->p,&d2->p);
  copypar("masksb1",&d1->p,&d2->p);
  copypar("maskfn",&d1->p,&d2->p);
  copypar("maskfn1",&d1->p,&d2->p);
  copypar("masklvlmode",&d1->p,&d2->p);
  copypar("masklvlmax",&d1->p,&d2->p);
  copypar("masklvlnoise",&d1->p,&d2->p);
  copypar("masklvlnoisefrac",&d1->p,&d2->p);
  copypar("dfill",&d1->p,&d2->p);
  copypar("dfilldim",&d1->p,&d2->p);
  copypar("dfillfrac",&d1->p,&d2->p);
  copypar("dfillloops",&d1->p,&d2->p);
  copypar("maskeqnoise",&d1->p,&d2->p);
}

void copysmappars(struct data *d1,struct data *d2)
{
  copypar("imSM",&d1->p,&d2->p);
  copypar("smapref",&d1->p,&d2->p);
  copypar("vcoilref",&d1->p,&d2->p);
  copypar("smapmask",&d1->p,&d2->p);
  copypar("smapnoisefrac",&d1->p,&d2->p);
  copypar("smapwlb",&d1->p,&d2->p);
  copypar("smapwgf",&d1->p,&d2->p);
  copypar("smapwsb",&d1->p,&d2->p);
  copypar("smaplb",&d1->p,&d2->p);
  copypar("smaplb1",&d1->p,&d2->p);
  copypar("smapgf",&d1->p,&d2->p);
  copypar("smapgf1",&d1->p,&d2->p);
  copypar("smapsb",&d1->p,&d2->p);
  copypar("smapsb1",&d1->p,&d2->p);
  copypar("smapeqnoise",&d1->p,&d2->p);
}

void copysensepars(struct data *d1,struct data *d2)
{
  copypar("accelread",&d1->p,&d2->p);
  copypar("accelphase",&d1->p,&d2->p);
  copypar("rmapread",&d1->p,&d2->p);
  copypar("rmapphase",&d1->p,&d2->p);
  copypar("noisematrix",&d1->p,&d2->p);
  copypar("printNM",&d1->p,&d2->p);
}

int checkequal(struct data *d1,struct data *d2,char *par,char *comment)
{
  int i;
  double *val1,*val2;

  if (nvals(par,&d1->p) != nvals(par,&d2->p)) {
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  %s and %s have different number of %s\n",d2->procpar,d1->procpar,comment);
  fprintf(stdout,"  Aborting recon of %s ...\n",d1->file);
  fflush(stdout);
#endif
    return(FALSE);
  }
  val1=val(par,&d1->p);
  val2=val(par,&d2->p);
  for (i=0;i<nvals(par,&d1->p);i++) {
    if (val1[i] != val2[i]) {
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  %s and %s have different %s\n",d2->procpar,d1->procpar,comment);
  fprintf(stdout," Aborting recon of %s ...\n",d1->file);
  fflush(stdout);
#endif
      return(FALSE);
    }
  }
  return(TRUE);
}

void initdatafrom(struct data *d1, struct data *d2)
{
  int i, ndim;

  // Copy file pointers
  d2->fp      = d1->fp;
  d2->datafp  = d1->datafp;
  d2->phasfp  = d1->phasfp;

  // Copy file names
  if (d1->file) {
    if ((d2->file= (char *)malloc((strlen(d1->file)+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    strcpy(d2->file,d1->file);
  } else
     d2->file = NULL;
  if (d1->procpar) {
    if ((d2->procpar = (char *)malloc((strlen(d1->procpar)+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    strcpy(d2->procpar,d1->procpar);
  } else
      d2->procpar = NULL;

  // Copy data structure
  d2->fh       = d1->fh;
  d2->bh       = d1->bh;
  d2->hcbh     = d1->hcbh;
  d2->buf      = d1->buf;

  d2->nvols    = d1->nvols;
  d2->startvol = d1->startvol;
  d2->endvol   = d1->endvol;
  d2->vol      = d1->vol;
  d2->outvol   = d1->outvol;

  d2->nblocks  = d1->nblocks;
  d2->startpos = d1->startpos;
  d2->endpos   = d1->endpos;
  d2->block    = d1->block;

  d2->np       = d1->np;
  d2->nv       = d1->nv;
  d2->nv2      = d1->nv2;
  d2->nseg     = d1->nseg;
  d2->etl      = d1->etl;
  d2->ne       = d1->ne;
  d2->fn       = d1->fn;
  d2->fn1      = d1->fn1;
  d2->fn2      = d1->fn2;
  d2->nr       = d1->nr;
  d2->ns       = d1->ns;
  d2->korder = d1->korder;

  d2->profile  = d1->profile;
  d2->proj2D   = d1->proj2D;

  d2->nav      = d1->nav;
  d2->nnav     = d1->nnav;
  if (d1->navpos) {
     if ((d2->navpos = malloc(d2->nnav*sizeof(*(d2->navpos)))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
     for (i=0; i<d2->nnav; i++)
        d2->navpos[i] = d1->navpos[i];
  } else
     d2->navpos = NULL;

  d2->seqmode  = d1->seqmode;

  if (d1->dim2order) {
     if (d1->dim2order[0] == -1) ndim = 1; else ndim = d1->nv;
     if ((d2->dim2order = malloc(ndim*sizeof(*(d2->dim2order)))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
     for (i=0; i<ndim; i++)
        d2->dim2order[i] = d1->dim2order[i];
  } else
     d2->dim2order = NULL;

  if (d1->dim3order) {
     if (d1->dim3order[0] == -1) ndim = 1; else ndim = d1->nv2;
     if ((d2->dim3order = malloc(ndim*sizeof(*(d2->dim3order)))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
     for (i=0; i<ndim; i++)
        d2->dim3order[i] = d1->dim3order[i];
  } else
     d2->dim3order = NULL;

  if (d1->pssorder) {
     if (d1->pssorder[0] == -1) ndim = 1; else ndim = d1->ns;
     if ((d2->pssorder = malloc(ndim*sizeof(*(d2->pssorder)))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
     for (i=0; i<ndim; i++)
        d2->pssorder[i] = d1->pssorder[i];
  } else
     d2->pssorder = NULL;

  /* NOTE *dim2orderR, *dim3orderR not yet done */

  d2->dim2R    = d1->dim2R;
  d2->dim3R    = d1->dim3R;

  d2->datamode = d1->datamode;

  d2->max      = d1->max;

  d2->p.npars  = 0;
  d2->a.npars  = 0;
  d2->s.npars  = 0;

  copypars(&d1->p,&d2->p);
  copypars(&d1->s,&d2->s);

  copypars(&d1->a,&d2->a);

  if (d2->a.npars>0) {
    if ((d2->a.d = (double **)malloc(sizeof(double *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    if ((d2->a.d[0] = (double *)malloc(*(d2->a.nvals)*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    /* Copy cycles to a->d */
    for (i=0;i<*(d2->a.nvals);i++) d2->a.d[0][i]=d1->a.d[0][i];
  }

  if (d1->fdfhdr) {
     if ((d2->fdfhdr= (char *)malloc((strlen(d1->fdfhdr)+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
     strcpy(d2->fdfhdr,d1->fdfhdr);
  } else
     d2->fdfhdr = NULL;

  // NOTE THAT NO DATA IS COPIED
  d2->ndim= d1->ndim;
  if (d1->dimstatus) {
     if ((d2->dimstatus = malloc(d2->ndim*sizeof(*(d2->dimstatus)))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
     for (i=0; i<d2->ndim; i++)
       d2->dimstatus[i] = NONE;
  } else
    d2->dimstatus = NULL;

  initnoise(d2);

  d2->data     = NULL;
  d2->mask     = NULL;

  // USER should copy the required data manually
  // ie copy2Ddata(d1,d2);
}

void initdata(struct data *d)
{
  /* Set flags for no data */
  d->nvols      = NONE;
  d->profile    = FALSE;
  d->proj2D     = FALSE;
  d->nav        = FALSE;
  d->korder     = FALSE;
  d->seqmode    = NONE;
  d->ndim       = NONE;
  d->datamode   = NONE;
  /* Set data pointers other than d->procpar to NULL */
  d->fp         = NULL;
  d->datafp     = NULL;
  d->phasfp     = NULL;
  d->file       = NULL;
  d->dim2order  = NULL;
  d->dim3order  = NULL;
  d->pssorder   = NULL;
  d->dim2orderR = NULL;
  d->dim3orderR = NULL;
  d->data       = NULL;
  d->navpos     = NULL;
  d->mask       = NULL;
  d->dimstatus  = NULL;
  d->fdfhdr     = NULL;
  /* NULL noise and zero max */
  nullnoise(d);
  zeromax(d);
}

void nulldata(struct data *d)
{
  /* Set pointers to NULL */
  d->fp         = NULL;
  d->datafp     = NULL;
  d->phasfp     = NULL;
  d->file       = NULL;
  d->procpar    = NULL;
  d->dim2order  = NULL;
  d->dim3order  = NULL;
  d->pssorder   = NULL;
  d->dim2orderR = NULL;
  d->dim3orderR = NULL;
  d->data       = NULL;
  d->csi_data   = NULL;
  d->navpos     = NULL;
  d->mask       = NULL;
  d->dimstatus  = NULL;
  d->fdfhdr     = NULL;
  nullnoise(d);
  nullpars(&d->p);
  nullpars(&d->a);
  nullpars(&d->s);
  /* Set some flags */
  d->nvols      = NONE;
  d->profile    = FALSE;
  d->proj2D     = FALSE;
  d->nav        = FALSE;
  d->korder = FALSE;
  d->seqmode    = NONE;
  d->ndim       = NONE;
  d->datamode   = NONE;
}

void cleardimorder(struct data *d)
{
  free(d->dim2order);
  free(d->dim3order);
  free(d->pssorder);
  d->dim2order  = NULL;
  d->dim3order  = NULL;
  d->pssorder   = NULL;
}

void clearstatus(struct data *d)
{
  free(d->dimstatus);
  d->dimstatus  = NULL;
}

void zeromax(struct data *d)
{
  d->max.Mval=0.0;
  d->max.Rval=0.0;
  d->max.Ival=0.0;
  d->max.np=-1;
  d->max.nv=-1;
  d->max.nv2=-1;
  /* Set data flag */
  d->max.data=FALSE;
}

void zeronoise(struct data *d)
{
  int i;
  d->noise.samples = 0;
  if (!d->noise.data) {
    if ((d->noise.M = (double *)malloc(d->nr*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    if ((d->noise.M2 = (double *)malloc(d->nr*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    if ((d->noise.Re = (double *)malloc(d->nr*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    if ((d->noise.Im = (double *)malloc(d->nr*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  }
  for (i=0;i<d->nr;i++) {
    d->noise.M[i] = 0.0;
    d->noise.M2[i] = 0.0;
    d->noise.Re[i] = 0.0;
    d->noise.Im[i] = 0.0;
  }
  d->noise.avM = 0.0;
  d->noise.avM2 = 0.0;
  d->noise.avRe = 0.0;
  d->noise.avIm = 0.0;
  /* Set data flag */
  d->noise.data=TRUE;
  /* Set data is zero flag */
  d->noise.zero=TRUE;
  d->noise.equal=FALSE;
  d->noise.samples=NONE;
}

void initnoise(struct data *d)
{
  nullnoise(d);
}

void nullnoise(struct data *d)
{
  /* Set pointers to NULL */
  d->noise.M  = NULL;
  d->noise.M2 = NULL;
  d->noise.Re = NULL;
  d->noise.Im = NULL;
  d->noise.mat = NULL;
  /* Set flags */
  d->noise.data=FALSE;
  d->noise.zero=FALSE;
  d->noise.equal=FALSE;
  d->noise.samples=NONE;
}

void closedata(struct data *d)
{
  fclose(d->fp);
  free(d->file);
  free(d->procpar);
}
