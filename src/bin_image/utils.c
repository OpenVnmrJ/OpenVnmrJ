/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*---------------------------------------------------------------------------*/
/* This is free software: you can redistribute it and/or modify              */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, either version 3 of the License, or         */
/* (at your option) any later version.                                       */
/*                                                                           */
/* This is distributed in the hope that it will be useful,                   */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* If not, see <http://www.gnu.org/licenses/>.                               */
/*---------------------------------------------------------------------------*/
/**/

void qs(double *items, int left, int right);

void nomem() {
  fprintf(stdout,"\nInsufficient memory for program!\n\n");
  exit(1);
}

void debug_msg(char *msg) {
  fprintf(stdout,"%s",msg);
  fflush(stdout);

}

double min(double a, double b) {
  return(a < b ? a : b);
}

double max(double a, double b) {
  return(a > b ? a : b);
}

void quicksort(double *items,int n) {
  qs(items,0,n-1);
}

void qs(double *items, int left, int right) {
  register int i, j;
  double x, y;

  i = left; j = right;
  x = items[(left+right)/2];

  do {
    while((items[i] < x) && (i < right)) i++;
    while((x < items[j]) && (j > left)) j--;

    if(i <= j) {
      y = items[i];
      items[i] = items[j];
      items[j] = y;
      i++; j--;
    }
  } while(i <= j);

  if (left < j)  qs(items, left, j);
  if (i < right) qs(items, i, right);
}

void createdir(char *name, char *path) {
  char dirname[MAXPATHLEN];
  char listfile[MAXPATHLEN];
  struct stat buf;
  int type;
  FILE *fp;

  sprintf(dirname,"%s/%s",path,name);

  /* If dirname doesn't exist make it */
  if (stat(dirname,&buf) == -1) { /* Check to see if it exists */
    if (mkdir(dirname, 00755) != 0) { /* If not then create */
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  Unable to create directory %s\n",dirname);
      fflush(stderr);
      exit(0);
    }
    stat(dirname,&buf);
  }
  /* dirname could of course exist as a file or symbolic link */
  type = buf.st_mode & S_IFMT;
  switch (type) {
    case S_IFDIR:
      sprintf(listfile,"%s/%s",path,"aipList");
      if ((fp = fopen(listfile,"a")) == NULL) {
        fprintf(stderr,"Can't open file %s for writing\n",listfile);
        exit(0);
      }
      fprintf(fp,"%s\n",name);
      fclose(fp);
      return;
      break;
    default:
      fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
      fprintf(stderr,"  %s is NOT a directory\n",dirname);
      fflush(stderr);
      exit(0);
  } /* end type switch */

}

/* Output value if vnmruser exists */
void val2file(char *filename,double data,int mode) {
  char file[MAXPATHLEN];
  FILE *fp;
  
  if (getenv("vnmruser") != NULL) {
    sprintf(file,"%s/%s",getenv("vnmruser"),filename);
    if ((fp = fopen(file,"a")) == NULL) {
      fprintf(stderr,"Can't open file %s for writing\n",file);
      exit(0);
    }
    switch (mode) {
      case INT32:
        fprintf(fp,"%d\n",(int)data);
        break;
      case FLT32:
        fprintf(fp,"%f\n",(float)data);
        break;
      default:
        fprintf(fp,"%f\n",data);
        break;
    }
    fclose(fp);
  }
  return;
}

/* Output string if vnmruser exists */
void str2file(char *filename,char *data) {
  char file[MAXPATHLEN];
  FILE *fp;
  
  if (getenv("vnmruser") != NULL) {
    sprintf(file,"%s/%s",getenv("vnmruser"),filename);
    if ((fp = fopen(file,"a")) == NULL) {
      fprintf(stderr,"Can't open file %s for writing\n",file);
      exit(0);
    }
    fprintf(fp,"%s\n",data);
    fclose(fp);
  }
  return;
}
