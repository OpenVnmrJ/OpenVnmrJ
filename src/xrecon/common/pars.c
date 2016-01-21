/* pars.c */
/*---------------------------------------------------------------------------*/
/*                                                                           */
/* pars.c: Parameter read routines                                           */
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

void getpars(char *procparfile,struct data *d)
{

  /* Null the data structure */
  nulldata(d);

  /* Copy procpar filename */
  if ((d->procpar = (char *)malloc((strlen(procparfile)+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  strcpy(d->procpar,procparfile);

  /* Get the parameters, some attributes, and their values */
  getprocparpars(procparfile,&d->p);

  /* Set and fill the 'pars' array structure.
     We convert array to an array of pars in the 'pars' array structure.
     We also figure how often each par cycles as the expt is run */
  setarray(&d->p,&d->a);

  /* Convert sviblist to an array of pars in the 'pars' sviblist structure */
  list2array("sviblist",&d->p,&d->s);

}

void getprocparpars(char *procparfile,struct pars *p)
{
  FILE *fp;
  int i,j,k;
  int type;
  int nvals;
  char name[1024];
  char str[1024]; /* NB. 'dg' type strings can be long */
  char nextChar;
  int keepString;
  int keepNext;

  /* Open procpar for reading */
  if ((fp=fopen(procparfile,"r")) == NULL) {
    fprintf(stderr,"\n%s: %s()\n",__FILE__,__FUNCTION__);
    fprintf(stderr,"  Unable to open procpar file %s\n",procparfile);
    if ((fp=fopen("procpar","r")) == NULL) {
      fprintf(stderr,"  Unable to open procpar file %s\n","procpar");
      fprintf(stderr,"  Aborting ...\n\n");
      fflush(stderr);
      exit(1);
    }
  }

  /* Initialise number of pars */
  p->npars=0;

  /* Scan procpar and count the parameters */
  while (fscanf(fp,"%s",name) != EOF) {
    /* Read type from the flag variables and read the number of values */
    if (fscanf(fp,"%*f %d %*f %*f %*f %*f %*f %*f %*f %*f %d",
      &type,&nvals) != 2)
      procparerror(__FUNCTION__,name,"(attributes)");

    /* Skip the values */
    skipprocparvals(fp,p,type,nvals);

    /* Read the number of enumerable values */
    if (fscanf(fp,"%d",&nvals) != 1)
      procparerror(__FUNCTION__,name,"(number of enumerable values)");

    /* Skip the enumerable values */
    skipprocparvals(fp,p,type,nvals);

    /* Increment number of parameters */
    p->npars++;
  }

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  fprintf(stdout,"  %s contains %d parameters\n",procparfile,p->npars);
  fflush(stdout);
#endif

  /* Allocate dynamic memory according to the number of parameters */
  mallocnpars(p);

  /* Rewind pointer to beginning of file */
  rewind(fp);

  /* Loop over the counted parameters reading data as we go */
  for (i=0;i<p->npars;i++) {

    /* Get the parameter name */
    fscanf(fp,"%s",name);

    /* Allocate and set parameter name */
    if ((p->name[i] = (char *)malloc((strlen(name)+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    strcpy(p->name[i],name);

    /* Read the flag variables, looking for type and value of 'active',
       and get the number of values */
    if (fscanf(fp, "%*f %d %*f %*f %*f %*f %*f %*f %d %*f %d",
      &p->type[i],&p->active[i],&p->nvals[i]) != 3)
      procparerror(__FUNCTION__,name,"(attributes)");

    /* Now get the values */
    switch(p->type[i]) {
      case 0:
        if ((p->i[i] = (int *)malloc(p->nvals[i]*sizeof(int *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
        for (j=0;j<p->nvals[i];j++) {
          if (fscanf(fp, "%d", &p->i[i][j]) != 1)
            procparerror(__FUNCTION__,name,"(values)");
        }
        break;
      case 1:
        if ((p->d[i] = (double *)malloc(p->nvals[i]*sizeof(double *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
        for (j=0;j<p->nvals[i];j++) {
          if (fscanf(fp, "%lf", &p->d[i][j]) != 1)
            procparerror(__FUNCTION__,name,"(values)");
        }
        break;
      case 2:
        if ((p->s[i] = (char **)malloc(p->nvals[i]*sizeof(char *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
        for (j=0;j<p->nvals[i];j++) {
          /* A single string can contain white space so we check for the final ".
           * Quotes can be escaped using a backslash, so this must be accounted for
           * when reading strings.
           * WARNING: svp does not generate correct escape characters. This code will
           * mangle strings containing backslash in the same manner as rtp, ie it
           * behaves as well/poorly as VnmrJ software */
          k=0;
          keepNext   = 0;
          keepString = 0;
          while (1) {
            if (fscanf(fp,"%c",&nextChar) != 1)
               procparerror(__FUNCTION__,name,"(values)");
            /* Discard characters until the first quote is reached */
            if (!keepString) {
              if (nextChar == '"')
                keepString = 1;
              continue;
            }
            /* Check for the escape character which means the next character is used literally */
            if (nextChar == '\\') {
              keepNext = 1;
              continue;
            }
            /* Record the character regardless of value */
            if (keepNext) {
              str[k++] = nextChar;
              keepNext = 0;
              continue;
            }
            /* Check for terminating double quote */
            if (nextChar == '"') {
              /* Null terminate the string */
              str[k] = 0;
              /* Add string to the parameter object */
              if ((p->s[i][j] = (char *)malloc((strlen(str)+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
              strcpy(p->s[i][j],str);
              break;
            }
            else
              str[k++] = nextChar;
          }
        }
        break;
    }

    /* Read the number of enumerable values */
    if (fscanf(fp,"%d",&nvals) != 1)
      procparerror(__FUNCTION__,name,"(number of enumerable values)");

    /* Skip the enumerable values */
    skipprocparvals(fp,p,p->type[i],nvals);

  }

  /* Close the procpar */
  fclose(fp);

/*
#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  printprocpar(p);
  fflush(stdout);
#endif
*/
}

void skipprocparvals(FILE *fp,struct pars *p,int type,int nvals)
{
  int i;
  char str[1024]; /* NB. 'dg' type strings can be long */

  switch(type) {
    case 0:
      for (i=0;i<nvals;i++) fscanf(fp,"%*d");
      break;
    case 1:
      for (i=0;i<nvals;i++) fscanf(fp,"%*f");
      break;
    case 2:
      /* A single string can contain white space so we check for the final " */
      i=0;
      while (i<nvals) {
        fscanf(fp,"%s",str);
        if (str[strlen(str)-1] == '"') i++;
      }
      break;
  }
}

void procparerror(const char *function,char *par,char *str)
{
  fprintf(stderr,"\n%s: %s()\n",__FILE__,function);
  fprintf(stderr,"  Problem reading parameter '%s' %s\n",par,str);
  fflush(stderr);
}

void setarray(struct pars *p,struct pars *a)
{
  int i,j,k,n;
  int joint;
  double *cycles;
  char str[MAXPATHLEN];

  if (!list2array("array",p,a)) return;

  /* Take a copy of the array string */
  strcpy(str,*sval("array",p));

  /* Fill with parameter names from the array string */
  /* 'array' is a string, so we will use member 'd' to hold a 'joint' index
     with which we can track parameters that are jointly arrayed */
  /* Allocate memory for a->d */
  if ((a->d = (double **)malloc(sizeof(double *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((a->d[0] = (double *)malloc(*a->nvals*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  /* Initialise a->d */
  for (i=0;i<*a->nvals;i++) a->d[0][i]=0;
  joint=0; j=0; k=0;
  for (i=0;i<strlen(str);i++) {
    if (str[i] == '(') {
      i++; a->d[0][j]=1; joint=1;
    }
    if (str[i] == ')') {
      i++; joint=0;
    }
    if (str[i] == ',') {
      if (joint) {
        a->d[0][j]++;
        a->d[0][j+1]=a->d[0][j]; /* for a ',' j+1 will always exist */
      }
      a->s[0][j++][k]=0; /* NULL terminate */
      k=0;
    }
    else
      a->s[0][j][k++]=str[i];
  }
  a->s[0][j][k]=0; /* NULL terminate */

  /* As such:
     The 'joint' index is 0 for parameters that are not jointly arrayed.
     Arrayed parameters generate indices as follows:

       array                     'joint' index
                     parameter   a  b  c  d  e
     (a,b),c,d,e   :             2  2  0  0  0
     a,(b,c),d,e   :             0  2  2  0  0
     (a,b,c),d,e   :             2  3  3  0  0
     a,(b,c,d),e   :             0  2  3  3  0
     a,(b,c,d,e)   :             0  2  3  4  4
     a,(b,c),(d,e) :             0  2  2  2  2
     (a,b,c,d,e)   :             2  3  4  5  5

     Starting from the last parameter (which cycles the quickest) we can
     now easily figure how many parameters are jointly arrayed
  */

  /* Convert the 'joint' index to 'cycles' which describes the number of
     volumes over which the parameter cycles */
  /* Allocate for a->nvals 'cycles' */
  if ((cycles = (double *)malloc(*a->nvals*sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Initialise cycles and joint for the last parameter */
  cycles[*a->nvals-1]=1;
  if (a->d[0][*a->nvals-1] > 0) joint=1;
  else joint=0;

  for (i=*a->nvals-2;i>=0;i--) {
    if (joint) {
      n=a->d[0][i+1]-1;
      for (j=0;j<n;j++) { /* was for (j=0;j<a->d[0][i+1]-1;j++) { */
        cycles[i]=cycles[i+1];
        i--;
      }
      i++; joint=0;
    } else {
      cycles[i]=cycles[i+1]*nvals(a->s[0][i+1],p);
      if (a->d[0][i] > 0) joint=1;
    }
  }

  /* Copy cycles to a->d */
  for (i=0;i<*a->nvals;i++) a->d[0][i]=cycles[i];

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  printarray(a,*sval("array",p));
#endif

}

int list2array(char *par,struct pars *p1,struct pars *p2)
{
  int i,j,k;
  char str[MAXPATHLEN];

  /* We only care about the single parameter here */
  /* Initialise the p2 'pars' structure */
  p2->npars=0;

  /* Copy par to the p2 'pars' structure */
  copypar(par,p1,p2);

  /* Return 0 if the par does not exist */
  if (p2->npars == 0) return(0);

  /* Return if par is an empty string */
  if (strlen(**p2->s) == 0) {
    free(*p2->name); free(**p2->s); free(*p2->s);
    freenpars(p2);
    /* Use npars to flag the sviblist parameter is empty */
    p2->npars=0;
    return(0);
  }

  *p2->nvals=1; /* there is at least one parameter in the list */
  /* Count the ,s to see how many parameters there are */
  for (i=1;i<strlen(**p2->s);i++) {
    if (p2->s[0][0][i] == ',') p2->nvals[0]++;
  }

  /* Free the list string */
  free(**p2->s); free(*p2->s); free(p2->s);

  /* Allocate for nvals */
  if ((p2->s = (char ***)malloc(sizeof(char **))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((p2->s[0] = (char **)malloc(*p2->nvals*sizeof(char *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  for (i=0;i<*p2->nvals;i++)
    if ((p2->s[0][i] = (char *)malloc((strlen(*sval(par,p1))+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);

  /* Return if the par is "array" since this is a special case where the
     string can additionally contain "(" and ")" */
  if (!strcmp(par,"array")) return(1);

  /* Take a copy of the par list string */
  strcpy(str,*sval(par,p1));

  /* Fill with parameter names from the list string */
  j=0; k=0;
  for (i=0;i<strlen(str);i++) {
    if (str[i] == ',') {
      p2->s[0][j++][k]=0; /* NULL terminate */
      k=0;
    }
    else
      p2->s[0][j][k++]=str[i];
  }
  p2->s[0][j][k]=0; /* NULL terminate */

#ifdef DEBUG
  fprintf(stdout,"\n%s: %s()\n",__FILE__,__FUNCTION__);
  printprocpar(p2);
  fflush(stdout);
#endif

  return(1);
}

void setval(struct pars *p,char *par,double value)
{
  struct pars p2;

  /* Set the value of the parameter in pars struct p2 */
  p2.npars = 1;     /* One parameter */
  mallocnpars(&p2); /* Malloc for one parameter */
  p2.type[0] = 1;   /* Value is real */
  p2.active[0] = 1; /* Parameter is active */
  p2.nvals[0]  = 1; /* There is one parameter value */
  /* Malloc and fill parameter name */
  if ((p2.name[0] = (char *)malloc((strlen(par)+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  strcpy(p2.name[0],par);
  /* Malloc and fill the parmaeter value */
  if ((p2.d[0] = (double *)malloc(sizeof(double *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  *p2.d[0] = value;
  /* Now copy the value from p2 to p */
  copypar(par,&p2,p);
  /* Clear p2 */
  clearpars(&p2);
}

void setsval(struct pars *p,char *par,char *svalue)
{
  struct pars p2;

  /* Set the value of the parameter in pars struct p2 */
  p2.npars = 1;     /* One parameter */
  mallocnpars(&p2); /* Malloc for one parameter */
  p2.type[0] = 2;   /* Value is real */
  p2.active[0] = 1; /* Parameter is active */
  p2.nvals[0]  = 1; /* There is one parameter value */
  /* Malloc and fill parameter name */
  if ((p2.name[0] = (char *)malloc((strlen(par)+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  strcpy(p2.name[0],par);
  /* Malloc and fill the parmaeter value */
  if ((p2.s[0] = (char **)malloc(sizeof(char *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((p2.s[0][0] = (char *)malloc((strlen(svalue)+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  strcpy(p2.s[0][0],svalue);
  /* Now copy the value from p2 to p */
  copypar(par,&p2,p);
  /* Clear p2 */
  clearpars(&p2);
}

void copypars(struct pars *p1, struct pars *p2)
{
  int i;

  /* Delete existing parameters */
  clearpars(p2);

  /* Copy the current p1 set to p2 */
  p2->npars = p1->npars;
  if (p2->npars > 0) mallocnpars(p2);
  for (i=0;i<p2->npars;i++) {
    if ((p2->name[i] = (char *)malloc((strlen(p1->name[i])+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    strcpy(p2->name[i],p1->name[i]);
    /* Copy values from p1 to p2 */
    copyvalues(p1,i,p2,i);
  }
}

void cppar(char *par1,struct pars *p1,char *par2,struct pars *p2)
{
  int i;
  int ix;
  struct pars p3;

  /* Find the index of par1 in the p1 set otherwise return */
  ix=parindex(par1,p1);
  if (ix<0) return;

  /* Search for par2 in the p2 set to modify an exisiting parameter */
  for (i=0;i<p2->npars;i++) {
    if (!strcmp(p2->name[i],par2)) { /* There is a match */
      /* Free existing values */
      switch(p2->type[i]) {
        case 0:
          free(p2->i[i]);
          break;
        case 1:
          free(p2->d[i]);
          break;
        case 2:
          free(p2->s[i]);
          break;
      } /* end p->type switch */
      /* Copy values from p1 to p2 */
      copyvalues(p1,ix,p2,i);
      return;
    }
  }

  /* If we have not already returned then add the parameter */

  /* Copy the current p2 set to p3 */
  p3.npars = p2->npars;
  if (p3.npars > 0) mallocnpars(&p3);
  for (i=0;i<p3.npars;i++) {
    if ((p3.name[i] = (char *)malloc((strlen(p2->name[i])+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    strcpy(p3.name[i],p2->name[i]);
    /* Copy values from p2 to p3 */
    copyvalues(p2,i,&p3,i);
  }

  /* Clear the current p2 set */
  clearpars(p2);

  /* Restore the p2 set but allocate for an extra parameter */
  p2->npars = p3.npars+1;
  mallocnpars(p2);
  for (i=0;i<p3.npars;i++) {
    if ((p2->name[i] = (char *)malloc((strlen(p3.name[i])+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
    strcpy(p2->name[i],p3.name[i]);
    /* Copy values back from p3 to p2 */
    copyvalues(&p3,i,p2,i);
  }

  /* Clear the p3 set */
  clearpars(&p3);

  /* Now add the parameter */
  if ((p2->name[i] = (char *)malloc((strlen(par2)+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  strcpy(p2->name[i],par2);
  /* Copy values from p1 to p2 */
  copyvalues(p1,ix,p2,i);

}

void copypar(char *par,struct pars *p1,struct pars *p2)
{
  cppar(par,p1,par,p2);
  return;
}

void copyvalues(struct pars *p1,int ix1,struct pars *p2,int ix2)
{
  int j;

  /* Copy type, value of 'active' and number of values */
  p2->type[ix2]   = p1->type[ix1];
  p2->active[ix2] = p1->active[ix1];
  p2->nvals[ix2]  = p1->nvals[ix1];
  /* Allocate and copy values */
  switch(p2->type[ix2]) {
    case 0:
      if ((p2->i[ix2] = (int *)malloc(p2->nvals[ix2]*sizeof(int *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      for (j=0;j<p2->nvals[ix2];j++) p2->i[ix2][j] = p1->i[ix1][j];
      break;
    case 1:
      if ((p2->d[ix2] = (double *)malloc(p2->nvals[ix2]*sizeof(double *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      for (j=0;j<p2->nvals[ix2];j++) p2->d[ix2][j] = p1->d[ix1][j];
      break;
    case 2:
      if ((p2->s[ix2] = (char **)malloc(p2->nvals[ix2]*sizeof(char *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
      for (j=0;j<p2->nvals[ix2];j++) {
        if ((p2->s[ix2][j] = (char *)malloc((strlen(p1->s[ix1][j])+1)*sizeof(char))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
        strcpy(p2->s[ix2][j],p1->s[ix1][j]);
      }
      break;
  } /* end p->type switch */

}

void mallocnpars(struct pars *p)
{
  if ((p->name = (char **)malloc(p->npars*sizeof(char *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((p->type = malloc(p->npars*sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((p->active = malloc(p->npars*sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((p->nvals = malloc(p->npars*sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((p->i = (int **)malloc(p->npars*sizeof(int *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((p->d = (double **)malloc(p->npars*sizeof(double *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((p->s = (char ***)malloc(p->npars*sizeof(char **))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
}

void freenpars(struct pars *p)
{
  free(p->name);
  free(p->type);
  free(p->active);
  free(p->nvals);
  free(p->i);
  free(p->d);
  free(p->s);
}

void printarray(struct pars *a,char *array)
{
  int i;

  /* Return if there are no arrayed parameters */
  if (a->npars == 0) return;

  fprintf(stdout,"  -----\n");
  fprintf(stdout,"  ARRAY\n");
  fprintf(stdout,"  -----\n");
  fprintf(stdout,"    array = %s\n",array);
  fprintf(stdout,"    Number of pars arrayed = %d\n",*a->nvals);
  for (i=0;i<*a->nvals;i++) {
    fprintf(stdout,"    par %d = %s",i+1,a->s[0][i]);
    if (a->d[0][i] > 0) fprintf(stdout,", cycle = %d\n",(int)a->d[0][i]);
    else fprintf(stdout,"\n");
  }
  fflush(stdout);
}

void printprocpar(struct pars *p)
{
  int i,j;
  fprintf(stdout,"  -------\n");
  fprintf(stdout,"  PROCPAR\n");
  fprintf(stdout,"  -------\n");
  for (i=0;i<p->npars;i++) {
    if (p->active[i]) {
      fprintf(stdout,"  par: %s = ",p->name[i]);
      switch(p->type[i]) {
        case 0:
          if (p->nvals[i] > 1) {
            fprintf(stdout,"arrayed:\n");
            for (j=0;j<p->nvals[i];j++)
              fprintf(stdout,"    %d\n",p->i[i][j]);
          }
          else
            fprintf(stdout,"%d\n",*p->i[i]);
          break;
       case 1:
          if (p->nvals[i] > 1) {
            fprintf(stdout,"arrayed:\n");
            for (j=0;j<p->nvals[i];j++)
              fprintf(stdout,"    %f\n",p->d[i][j]);
          }
          else
            fprintf(stdout,"%f\n",*p->d[i]);
          break;
        case 2:
          if (p->nvals[i] > 1) {
            fprintf(stdout,"arrayed:\n");
            for (j=0;j<p->nvals[i];j++)
              fprintf(stdout,"    %s\n",p->s[i][j]);
          }
          else
            fprintf(stdout,"%s\n",*p->s[i]);
          break;
        default:
          break;
      }
    }
  }
  fflush(stdout);
}

int ptype(char *par,struct pars *p)
{
  int i;
  for (i=0;i<p->npars;i++)
    if ((!strcmp(p->name[i],par)) && p->active[i])
      return(p->type[i]);
  return(-1);
}

int nvals(char *par,struct pars *p)
{
  int i;
  for (i=0;i<p->npars;i++)
    if ((!strcmp(p->name[i],par)) && p->active[i])
      return(p->nvals[i]);
  return(0);
}

int *ival(char *par,struct pars *p)
{
  int i;
  int *in;
  for (i=0;i<p->npars;i++)
    if ((!strcmp(p->name[i],par)) && p->active[i] && (p->type[i] == 0))
      return(p->i[i]);
  if ((in = (int *)malloc(sizeof(int))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  *in=0;
  return(in);
}

double *val(char *par,struct pars *p)
{
  int i;
  double *d;
  for (i=0;i<p->npars;i++)
    if ((!strcmp(p->name[i],par)) && p->active[i] && (p->type[i] == 1))
      return(p->d[i]);
  if ((d = (double *)malloc(sizeof(double))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  *d=0.0;
  return(d);
}

char **sval(char *par,struct pars *p)
{
  int i;
  char **s;

  for (i=0;i<p->npars;i++)
    if ((!strcmp(p->name[i],par)) && p->active[i] && (p->type[i] == 2))
      return(p->s[i]);
  if ((s = (char **)malloc(sizeof(char *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  if ((*s = (char *)malloc(sizeof(char *))) == NULL) nomem(__FILE__,__FUNCTION__,__LINE__);
  **s=(int)0;
  return(s);
}

int parindex(char *par,struct pars *p)
{
  int i;
  for (i=0;i<p->npars;i++)
    if (!strcmp(p->name[i],par))
      return(i);
  return(-1);
}

int arraycheck(char *par,struct pars *a)
{
  int i;
  /* Return if there are no arrayed parameters */
  if (a->npars == 0) return(0);
  for (i=0;i<*a->nvals;i++)
    if (!strcmp(a->s[0][i],par)) return(1);
  return(0);
}

int getcycle(char *par,struct pars *a)
{
  int i;
  /* Return if there are no arrayed parameters */
  if (a->npars == 0) return(0);
  for (i=0;i<*a->nvals;i++)
    if (!strcmp(a->s[0][i],par)) return((int)a->d[0][i]);
  return(0);
}

int cyclefaster(char *par1,char *par2,struct pars *a)
{
  int cycle1,cycle2;
  int i;

  /* If d2 or d3 are arrayed the cycle value can't be used to test.
     It doesn't make sense that d2,d3 are jointly arrayed with another parameter.
     In this instance just check the position of parameters  in array string */
  if (!strcmp(par1,"d2") || !strcmp(par1,"d3") || !strcmp(par2,"d2") || !strcmp(par2,"d3")) {
    /* Parameters cycle faster as the index increases */
    for (i=0;i<*a->nvals;i++) {
      if (!strcmp(a->s[0][i],par1)) return(FALSE);
      if (!strcmp(a->s[0][i],par2)) return(TRUE);
    }
  } else {
    /* Compare the cycle */
    cycle1=getcycle(par1,a);
    cycle2=getcycle(par2,a);
    if (cycle1<1) return(FALSE); /* cycle=0 if parameter is not arrayed */
    if (cycle2<1) return(FALSE); /* cycle=0 if parameter is not arrayed */
    if (cycle1<cycle2) return(TRUE);
  }
  return(FALSE);
}

void clearpars(struct pars *p)
{
  int i,j;
  if (p->npars>0) {
    for (i=0;i<p->npars;i++) {
      if ((p->type[i] == 2) && p->active[i]) {
        for (j=0;j<p->nvals[i];j++) free(p->s[i][j]);
      }
      if ((p->type[i] == 1) && p->active[i]) free(p->d[i]);
      if ((p->type[i] == 0) && p->active[i]) free(p->i[i]);
      free(p->name[i]);
    }
    freenpars(p);
  }
}

void cleararray(struct pars *a)
{
  int i;
  if (a->npars>0) {
    for (i=0;i<*a->nvals;i++) free(a->s[0][i]);
    free(a->s[0]);
    free(a->d[0]);
    free(*a->name);
    freenpars(a);
  }
}

void nullpars(struct pars *p)
{
  p->npars   = 0;
  p->name    = NULL;
  p->type    = NULL;
  p->active  = NULL;
  p->nvals   = NULL;
  p->i       = NULL;
  p->d       = NULL;
  p->s       = NULL;
}
