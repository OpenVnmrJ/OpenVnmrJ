/* utils.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* utils.c: Utilities                                                        */
/*                                                                           */
/* Copyright (C) 2012 Paul Kinchesh                                          */
/*               2012 Martyn Klassen                                         */
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

int check_CPU_type()
{
  union {
    unsigned char ch[2];
    short ss;
  } cpu;
  cpu.ch[0] = 1;
  cpu.ch[1] = 0;
  /* If we have least significant byte at the first address we are
     little-endian (little end comes first) */
  if (cpu.ss == 1) return(TRUE);
  return(FALSE);
}

void complexmatrixmultiply(gsl_matrix_complex *A,gsl_matrix_complex *B,gsl_matrix_complex *C)
{
  int i,j,k;
  gsl_complex cx;

  /* Check Dimensions */
  if (B->size1 != A->size2) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Input matrices (args 1&2) do not have compatible dimensions\n");
    fprintf(stderr,"  Aborting ...\n\n");
    fflush(stderr);
    return;
  }
  if ((C->size1 != A->size1) && (C->size2 != B->size2)) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Resultant matrix (arg 3) does not have correct dimensions\n");
    fprintf(stderr,"  Aborting ...\n\n");
    fflush(stderr);
    return;
  }

  for (i=0;i<C->size1;i++) {
    for (j=0;j<C->size2;j++) {
      GSL_SET_COMPLEX(&cx,0.0,0.0);
      for (k=0;k<A->size2;k++) {
        cx=gsl_complex_add(cx,gsl_complex_mul(gsl_matrix_complex_get(A,i,k),gsl_matrix_complex_get(B,k,j)));
      }
      gsl_matrix_complex_set(C,i,j,cx);
    }
  }

}

void complexmatrixconjugate(gsl_matrix_complex *A)
{
  int i,j;
  gsl_complex cx;

  for (i=0;i<A->size1;i++) {
    for (j=0;j<A->size2;j++) {
      cx=gsl_matrix_complex_get(A,i,j);
      GSL_SET_IMAG(&cx,-GSL_IMAG(cx));
      gsl_matrix_complex_set(A,i,j,cx);
    }
  }
}

int round2int(double dval)
{
  int ival;
  ival=(int)floor(dval);
  if (dval-(double)ival >= 0.5) ival++;
  return(ival);
}

void reverse2ByteOrder(int nele,char *ptr)
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

void reverse4ByteOrder(int nele,char *ptr)
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

void reverse8ByteOrder(int nele,char *ptr)
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

int doublecmp(const void *double1,const void *double2)
{
  /* Explicitly cast to get the correct type */
  const double *dbl1 = (const double *)double1;
  const double *dbl2 = (const double *)double2;
  /* Compare */
  if (*dbl1 > *dbl2) return 1;        /* first item is bigger */
  else if (*dbl1 == *dbl2) return  0; /* equality */
  else return -1;                     /* second item is bigger */
}

int nomem(char *file,const char *function,int line)
{
  fprintf(stderr,"\n%s: %s() line %d\n",file,function,line);
  fprintf(stderr,"  Insufficient memory for program!\n\n");
  fflush(stderr);
  exit(1);
}

int createdir(char *dirname)
{
  struct stat buf;
  int type;

  /* If dirname doesn't exist make it */
  if (stat(dirname,&buf) == -1) { /* Check to see if it exists */
    if (mkdir(dirname, 00755) != 0) { /* If not then create */
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Unable to create directory %s\n",dirname);
      fflush(stderr);
      return(1);
    }
    stat(dirname,&buf);
  }
  /* dirname could of course exist as a file or symbolic link */
  type = buf.st_mode & S_IFMT;
  switch (type) {
    case S_IFDIR:
      return(0);
      break;
    default:
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  %s is NOT a directory\n",dirname);
      fflush(stderr);
      return(1);
  } /* end type switch */

  return(0);
}

int checkdir(char *dirname)
{
  struct stat buf;
  int type;

  /* Return if dirname doesn't exist */
  if (stat(dirname,&buf) == -1) { /* Check to see if it exists */
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  %s does not exist\n",dirname);
    fflush(stderr);
    return(1);
  }

  /* dirname could of course exist as a file or symbolic link */
  type = buf.st_mode & S_IFMT;
  switch (type) {
    case S_IFDIR:
      return(0);
      break;
    default:
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  %s is NOT a directory\n",dirname);
      fflush(stderr);
      return(1);
  } /* end type switch */

  return(0);
}

int checkfile(char *filename)
{
  struct stat buf;
  int type;

  /* Return if filename doesn't exist */
  if (stat(filename,&buf) == -1) { /* Check to see if it exists */
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  %s does not exist\n",filename);
    fflush(stderr);
    return(1);
  }

  /* filename could of course exist as a directory or symbolic link */
  type = buf.st_mode & S_IFMT;
  switch (type) {
    case S_IFREG:
      return(0);
      break;
    default:
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  %s is NOT a regular file\n",filename);
      fflush(stderr);
      return(1);
  } /* end type switch */

  return(0);
}
