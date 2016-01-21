/* dhead.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* dhead.c: Data header routines                                             */
/*                                                                           */
/* Copyright (C) 2011 Paul Kinchesh                                          */
/*               2011 Martyn Klassen                                         */
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

void getdfh(struct data *d)
{

  /* Assume we don't have to reverse byte order to start with */
  reverse_byte_order = FALSE;

  /* The file pointer might not be at the start of the file */
  fseek(d->fp, 0, SEEK_SET);

  /* Read the data file header */
  fread(&d->fh,1,sizeof(d->fh),d->fp);
  /* Check to see if we should reverse byte order */
  if (!(d->fh.status & S_DATA) || (d->fh.nblocks < 1) || (d->fh.nbheaders < 1)
    || ((d->fh.ebytes != 2) && (d->fh.ebytes != 4) && (d->fh.ebytes != 8))
    || (d->fh.tbytes != d->fh.np*d->fh.ebytes)
    || (d->fh.bbytes != d->fh.tbytes*d->fh.ntraces + d->fh.nbheaders*sizeof(d->bh)))
  {
    /* Reverse byte order of file header */
    reversedfh(&d->fh);
    reverse_byte_order = TRUE;
  }
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  printfileheader(&d->fh);
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  printfilestatus(&d->fh);
  fflush(stdout);
#endif

  /* If we still can't make sense of the header something is wrong */
  if (!(d->fh.status & S_DATA) || (d->fh.nblocks < 1) || (d->fh.nbheaders < 1)
    || ((d->fh.ebytes != 2) && (d->fh.ebytes != 4) && (d->fh.ebytes != 8))
    || (d->fh.tbytes != d->fh.np*d->fh.ebytes)
    || (d->fh.bbytes != d->fh.tbytes*d->fh.ntraces + d->fh.nbheaders*sizeof(d->bh)))
  {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Problem deciphering data file header\n");
    fprintf(stderr,"  Aborting ...\n\n");
    fflush(stderr);
    fclose(d->fp);
    exit(1);
  }
  if (d->buf.st_size == sizeof(d->fh)) { 
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  data file only contains a file header\n");
    fflush(stderr);
  }
  if (d->fh.bbytes > 0) {
    if (((d->buf.st_size - sizeof(d->fh))%d->fh.bbytes) != 0) {
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Problem deciphering data file header\n");
      fprintf(stderr,"  Aborting ...\n\n");
      fflush(stderr);
      fclose(d->fp);
      exit(1);
    }
  }

}

void getdbh(struct data *d,int blockindex)
{
  long offset;

#ifdef DEBUG
  char seqcon[6];
#endif

  offset=sizeof(d->fh)+(long)blockindex*d->fh.bbytes;
  if (d->buf.st_size >= offset+sizeof(d->bh)) {
    fseek(d->fp,offset,SEEK_SET);
    fread(&d->bh,1,sizeof(d->bh),d->fp);
  }
  else {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Problem reading block header %d in %s\n",blockindex,d->file);
    fflush(stderr);
  }
  if (reverse_byte_order) reversedbh(&d->bh);

#ifdef DEBUG
  strcpy(seqcon,*sval("seqcon",&d->p));
//  if ( ( (seqcon[2] == 's') && (blockindex%d->nv == 0) )
//    || (!(seqcon[2] == 's')) ) { /* too many blocks will slow us down */
//    fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
//    printblockheader(&d->bh,blockindex);
//  }
#endif
}

void gethcbh(struct data *d,int blockindex)
{
  long offset;

  offset=sizeof(d->fh)+(long)blockindex*d->fh.bbytes+sizeof(d->bh);
  if ((d->fh.nbheaders > 1) 
    && (d->buf.st_size >= offset+sizeof(d->hcbh))) {
    fseek(d->fp,offset,SEEK_SET);
    fread(&d->hcbh,1,sizeof(d->hcbh),d->fp);
  }
  else {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Problem reading hypercomplex block header %d in %s\n",blockindex,d->file);
    fflush(stderr);
  }
  if (reverse_byte_order) reversehcbh(&d->hcbh);

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  printhcblockheader(&d->hcbh,blockindex);
  fflush(stdout);
#endif
}

void reversedfh(struct datafilehead *dfh) 
{
  reverse4ByteOrder(1,(char *)&dfh->nblocks);
  reverse4ByteOrder(1,(char *)&dfh->ntraces);
  reverse4ByteOrder(1,(char *)&dfh->np);
  reverse4ByteOrder(1,(char *)&dfh->ebytes);
  reverse4ByteOrder(1,(char *)&dfh->tbytes);
  reverse4ByteOrder(1,(char *)&dfh->bbytes);
  reverse2ByteOrder(1,(char *)&dfh->vers_id);
  reverse2ByteOrder(1,(char *)&dfh->status);
  reverse4ByteOrder(1,(char *)&dfh->nbheaders);
}

void reversedbh(struct datablockhead *dbh)
{
  reverse2ByteOrder(1,(char *)&dbh->scale);
  reverse2ByteOrder(1,(char *)&dbh->status);
  reverse2ByteOrder(1,(char *)&dbh->index);
  reverse2ByteOrder(1,(char *)&dbh->mode);
  reverse4ByteOrder(1,(char *)&dbh->ctcount);
  reverse4ByteOrder(1,(char *)&dbh->lpval);
  reverse4ByteOrder(1,(char *)&dbh->rpval);
  reverse4ByteOrder(1,(char *)&dbh->lvl);
  reverse4ByteOrder(1,(char *)&dbh->tlt);
}

void reversehcbh(struct hypercmplxbhead *hcbh)
{
  reverse2ByteOrder(1,(char *)&hcbh->s_spare1);
  reverse2ByteOrder(1,(char *)&hcbh->status);
  reverse2ByteOrder(1,(char *)&hcbh->s_spare2);
  reverse2ByteOrder(1,(char *)&hcbh->s_spare3);
  reverse4ByteOrder(1,(char *)&hcbh->l_spare1);
  reverse4ByteOrder(1,(char *)&hcbh->lpval1);
  reverse4ByteOrder(1,(char *)&hcbh->rpval1);
  reverse4ByteOrder(1,(char *)&hcbh->f_spare1);
  reverse4ByteOrder(1,(char *)&hcbh->f_spare2);
}

void printfileheader(struct datafilehead *dfh)
{
  fprintf(stdout,"  -----------\n");
  fprintf(stdout,"  FILE HEADER\n");
  fprintf(stdout,"  -----------\n");
  fprintf(stdout,"  Number of blocks in file: \t\t\t%d\n",dfh->nblocks);
  fprintf(stdout,"  Number of traces per block: \t\t\t%d\n",dfh->ntraces);
  fprintf(stdout,"  Number of elements (np) per trace: \t\t%d\n",dfh->np);
  fprintf(stdout,"  Number of bytes per element: \t\t\t%d\n",dfh->ebytes);
  fprintf(stdout,"  Number of bytes per trace: \t\t\t%d\n",dfh->tbytes);
  fprintf(stdout,"  Number of bytes per block: \t\t\t%d\n",dfh->bbytes);
  fprintf(stdout,"  Software version, file_id status bits: \t%d\n",(int)dfh->vers_id);
  fprintf(stdout,"  Status of whole file: \t\t\t%d\n",(int)dfh->status);
  fprintf(stdout,"  Number of block headers per block: \t\t%d\n",dfh->nbheaders);
}

void printfilestatus(struct datafilehead *dfh)
{
  fprintf(stdout,"  File Status:");
  if (dfh->status & S_DATA)
    fprintf(stdout,"\t\t\t\t\tFile contains data\n");
  else {
    fprintf(stdout,"\t\t\t\t\tFile contains no data\n");
    return;
  }
  if (dfh->status & S_SPEC)
    fprintf(stdout,"\t\t\t\t\t\tSpectrum\n");
  else
    fprintf(stdout,"\t\t\t\t\t\tFid\n");
  if (dfh->status & S_FLOAT) {
    fprintf(stdout,"\t\t\t\t\t\t32 bit\n");
    fprintf(stdout,"\t\t\t\t\t\tFloating point\n");
  }
  else {
    if (dfh->status & S_32)
      fprintf(stdout,"\t\t\t\t\t\t32 bit\n");
    else
      fprintf(stdout,"\t\t\t\t\t\t16 bit\n");
    fprintf(stdout,"\t\t\t\t\t\tInteger\n");
  }
  if (dfh->status & S_COMPLEX)
    fprintf(stdout,"\t\t\t\t\t\tComplex\n");
  else
    fprintf(stdout,"\t\t\t\t\t\tReal\n");
  if (dfh->status & S_HYPERCOMPLEX)
    fprintf(stdout,"\t\t\t\t\t\tHypercomplex\n");
  fprintf(stdout,"  Other status bits:");
  if (dfh->status & S_DDR)
    fprintf(stdout,"\t\t\t\tDDR acq\n");
  else
    fprintf(stdout,"\t\t\t\tNot DDR acq\n");
  if (dfh->status & S_SECND)
    fprintf(stdout,"\t\t\t\t\t\tSecond FT\n");
  else
    fprintf(stdout,"\t\t\t\t\t\tFirst FT\n");
  if (dfh->status & S_TRANSF)
    fprintf(stdout,"\t\t\t\t\t\tTransposed\n");
  else
    fprintf(stdout,"\t\t\t\t\t\tRegular\n");
  if (dfh->status & S_3D)
    fprintf(stdout,"\t\t\t\t\t\t3D data\n");
  if (dfh->status & S_NP)
    fprintf(stdout,"\t\t\t\t\t\tnp dimension is active\n");
  if (dfh->status & S_NF)
    fprintf(stdout,"\t\t\t\t\t\tnf dimension is active\n");
  if (dfh->status & S_NI)
    fprintf(stdout,"\t\t\t\t\t\tni dimension is active\n");
  if (dfh->status & S_NI2)
    fprintf(stdout,"\t\t\t\t\t\tni2 dimension is active\n");
}

void printblockheader(struct datablockhead *dbh,int blockindex)
{
  fprintf(stdout,"  ------------\n");
  fprintf(stdout,"  BLOCK HEADER (block %d)\n",blockindex);
  fprintf(stdout,"  ------------\n");
  fprintf(stdout,"  Scaling factor: \t\t\t%d\n",(int)dbh->scale);
  fprintf(stdout,"  Status of data in block: \t\t%d\n",(int)dbh->status);
  fprintf(stdout,"  Block index: \t\t\t\t%d\n",(int)dbh->index);
  fprintf(stdout,"  Mode of data in block: \t\t%d\n",(int)dbh->mode);
  fprintf(stdout,"  ct value for FID: \t\t\t%d\n",(int)dbh->ctcount);
  fprintf(stdout,"  f2 (2D-f1) left phase in phasefile: \t%f\n",dbh->lpval);
  fprintf(stdout,"  f2 (2D-f1) right phase in phasefile: \t%f\n",dbh->rpval);
  fprintf(stdout,"  Level drift correction: \t\t%f\n",dbh->lvl);
  fprintf(stdout,"  Tilt drift correction: \t\t%f\n",dbh->tlt);
}

void printhcblockheader(struct hypercmplxbhead *hcbh,int blockindex)
{
  fprintf(stdout,"  -------------------------\n");
  fprintf(stdout,"  HYPERCOMPLEX BLOCK HEADER (block %d)\n",blockindex);
  fprintf(stdout,"  -------------------------\n");
  fprintf(stdout,"  Status of data in block: \t\t%d\n",(int)hcbh->status);
  fprintf(stdout,"  f1 left phase in phasefile: \t\t%f\n",hcbh->lpval1);
  fprintf(stdout,"  f1 right phase in phasefile: \t\t%f\n",hcbh->rpval1);
}
