/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */
/* Px_util.h - utility functions for Pbox Px-routines */

#include <ctype.h>

static char shapes[MAXSTR];

typedef struct {
int     np, nstp, reps, time, sign;
double *Mx, *My, *Mz, *Scf;
double  T1, T2, rdc, B1max, pw, st, sw, ofs;
} Blodata;

double stod(char str[16])  /* string to double conversion */
{
double dn;
(void) sscanf(str, "%lf", &dn);
return dn;
}

int stoi(char str[16])  /* string to integer conversion */
{
int in;
double dn;
(void) sscanf(str, "%lf", &dn);
in = (int) (dn + 0.001);
return in;
}

double *arry(int nl)
{
  double *ar;

  ar = (double *) calloc(nl, sizeof(double));
  if (!ar)
  {
    printf("allocation failure in arry()");
    exit(1);
  }
  return ar;
}

int getext(char *str1, char *str2)
{
int i, j, k;

  if ((j = strlen(str2)) == 0) return 0;
  i = j - 1;
  while((i > 0) && (str2[i] != '.')) i--;
  if (i == 0) return 0;
  for (k = 0, i++; i < j; i++, k++) { str1[k] = str2[i]; }
  str1[k] = '\0'; 
  return k;
}


int findpar2(FILE *fnm, char *str, char *val)
{
  char ch, str1[MAXSTR];
  int i = 0;

  fseek(fnm, 0, 0);
  while ((ch = getc(fnm)) == '#')
  {
    if (fgets(val, MAXSTR, fnm) == NULL) break;
    sscanf(val, "%255s", str1);
    if ((str[0] == str1[0]) && (strcmp(str,str1) == 0))
    {
      i++;
      break;
    }
  }

  return i;
}


int getsimdata(Blodata *bl, FILE *fil)
{
int i;
char s[7][32], val[MAXSTR], str[MAXSTR];

  if (findpar2(fil, "Pxsim", val) > 0)
  {
    i = sscanf(val, "%255s %31s %31s %31s %31s %31s %31s %31s",str,s[0],s[1],s[2],s[3],s[4],s[5],s[6]);
    if (i < 4) i = 0;
    else
    {
      if (isdigit(s[0][0])) bl->pw = stod(s[0]); else i = 0;     /* pw */
      if (isdigit(s[1][0])) bl->B1max = stod(s[1]); else i = 0;  /* B1max */
      if (isdigit(s[2][0])) bl->st = stod(s[2]); else i = 0;     /* status */
      if (isdigit(s[3][0])) bl->sw = stod(s[3]); else i = 0;     /* sw */
      if ((i > 5) && (isdigit(s[4][0]))) bl->T1 = stod(s[4]);    /* T1 */
      if ((i > 6) && (isdigit(s[5][0]))) bl->T2 = stod(s[5]);    /* T2 */
      if ((i > 7) && (isdigit(s[6][0]))) bl->rdc = stod(s[6]);   /* rdc */
    }
  }
  else i = 0; 
  if (!i) { printf("Insufficient information in shapefile header\n"); exit(1); }

return i; 
}

void skipcomms(FILE *fnm)
{
  long i;
  char str[MAXSTR];

    str[0] = '\0';
    i = ftell(fnm);
    if (fscanf(fnm, "%511s", str) == EOF) return;
    while (str[0] == '#')
    {
      if (fgets(str, MAXSTR, fnm) == NULL) return;
      i = ftell(fnm);
      if (fscanf(fnm, "%511s", str) == EOF) return;
    }
    fseek(fnm, i-1, 0);
}

double maxval(int nn, double *am)
{
int   j;
double mxl = 0.0;

  for (j = 0; j < nn; j++) mxl = MAX(mxl, fabs(am[j]));
  return mxl;
}

