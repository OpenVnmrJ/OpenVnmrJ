/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "byteswap_linux.h"


/*---------------------------*/
/*---- For byte swapping ----*/
/*---------------------------*/
extern int reverse_byte_order;


/*---------------------------*/
/*---- Check file header ----*/
/*---------------------------*/
int check_file_header(dfh)
struct datafilehead *dfh;
{
  /* Check to see if we should reverse byte order */
 /*
 if (!(dfh.status & S_DATA) || (dfh.nblocks < 1) || (dfh.nbheaders < 1)
    || ((dfh.ebytes != 2) && (dfh.ebytes != 4) && (dfh.ebytes != 8))
    || (dfh.tbytes != dfh.np*dfh.ebytes))
  {
    fprintf(stdout,"\n%s: %s()\n",SOURCEFILE,function);
    fprintf(stdout,"  Problem deciphering data file header\n");
    fprintf(stdout,"  Aborting ...\n\n");
    fclose(d->fp);
    exit(1);
  }
*/

return(1);
}



/*---------------------------*/
/*---- Reverse file header byte order ----*/
/*---------------------------*/
void reversedfh(dfh) 
struct datafilehead *dfh;
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

/*---------------------------*/
/*---- Reverse block header byte order ----*/
/*---------------------------*/
void reversedbh(dbh)
struct datablockhead *dbh;
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


void reverse2ByteOrder(nele,ptr)
  int nele;                  /* specify number of elements to convert */
  char *ptr;                 /* specify and return conversion data    */
{
  register short *ip;
  register char *ca;
  register int m;
  union {
    char ch[2];
    short i;
  } bs;
  ca = (char *)ptr;
  ip = (short *)ptr;
  for (m=0;m<nele;m++) {
    bs.ch[1] = *ca++;
    bs.ch[0] = *ca++;
    *ip++ = bs.i;
  }
}

void reverse4ByteOrder(nele,ptr)
  int nele;                  /* specify number of elements to convert */
  char *ptr;                 /* specify and return conversion data    */
{
  register int *ip;
  register char *ca;
  register int m;
  union {
    char ch[4];
    long i;
  } bs;
  ca = (char *)ptr;
  ip = (int *)ptr;
  for (m=0;m<nele;m++) {
    bs.ch[3] = *ca++;
    bs.ch[2] = *ca++;
    bs.ch[1] = *ca++;
    bs.ch[0] = *ca++;
    *ip++ = bs.i;
  }
}

void reverse8ByteOrder(nele,ptr)
  int nele;                  /* specify number of elements to convert */
  char *ptr;                 /* specify and return conversion data    */
{
  register double *ip;
  register char *ca;
  register int m;
  union {
    char ch[8];
    double i;
  } bs;
  ca = (char *)ptr;
  ip = (double *)ptr;
  for (m=0;m<nele;m++) {
    bs.ch[7] = *ca++;
    bs.ch[6] = *ca++;
    bs.ch[5] = *ca++;
    bs.ch[4] = *ca++;
    bs.ch[3] = *ca++;
    bs.ch[2] = *ca++;
    bs.ch[1] = *ca++;
    bs.ch[0] = *ca++;
    *ip++ = bs.i;
  }
}

