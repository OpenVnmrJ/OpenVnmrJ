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
/* Pbox_def.h - Pbox definitions and elementary utilities.
                mod and ipdel added to globals */


#define REV	"VERSION VJ4.2 "
#define SYSSHAPELIB "/vnmr/shapelib/"
#define SYSWAVELIB "/vnmr/wavelib/"
#define FN_INP "Pbox.inp"
#define FN_TMP "Pbox.tmp"
#define FN_GR  "pbox.gr"
#define FN_CAL "pbox.cal"
#define FN_FID "pbox.fid"
#define FN_SIM "pbox.sim"
#define FN_XY  "pbox.xy"
#define FN_Z   "pbox.mz"
#define FN_GLO "/.Pbox_globals"

#ifndef M_PI
#define M_PI	3.14159265358979323846
#endif

#ifndef M_LN10
#define M_LN10  2.30258509299404568402
#endif

#define MAXSTR  256
#define MAXLIN  81		/* max length of comment line     */
#define NAMLEN  16		/* max numb of char in shapename  */

#define ADD  1
#define MULT 2
#define POW  3
#define FUNC 4
#define PRN  5
#define EQ   100
#define DBLE 200
#define VAR  300

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define SWAP(a, b) tm=(a); (a)=(b); (b)=tm
#define ATTN(db)  exp(0.05 * db * M_LN10)
#define DCB(b, ref) 20.0 * log(b / ref) / M_LN10 

typedef struct   {
  int     flg;
  double  mgn;
  char    u;
} Var;

typedef struct   {
char    shdir[MAXSTR], wvdir[MAXSTR], demodir[MAXSTR], Console;
int     maxst, maxpwr, minpwr, defnp, cmpr, maxitr, simtime, minstps, gr;
double  minpw, minpwg, drmin, tmres, phres, amres, dres, maxamp, maxgr, maxincr,
        maxdev, pw, gpw, ipdel;
} Global;

typedef struct   {
int     np, npst, pwr, pwrf, rfpwr, tfl, sfl, reps, cal, pad1, pad2,
        imax, itns, itnf, itnw, itnd, calflg, posflg;
double  T1, T2, rdc, sw, offset, maxincr, pw90, dres, dmf, dcyc, pw, dnp, Bref, 
        sfrq, maxtof, stepsize, wrp;
char    name[128], type[16], bls[8], su[128], coms[8], ptype[32], mod[16], bscor[8], 
        fname[MAXSTR];
char    T1_[16], T2_[16], sw_[16], offset_[16], maxincr_[16], pw90_[16], dres_[16], 
        dcyc_[16], sfrq_[16], np_[16], attn_[16], reps_[16], rfpwr_[16], wrp_[16],
        pad1_[16], pad2_[16], stepsize_[16], rdc_[16];
Var     attn;
} Header;

typedef struct   {
double  fla, ph, ph0, pwb, dash, incr;
double  bw, of, pw, pwt, d0, d1, d2, st, wrp, php;
int     trev, np, n1, n2, npt, nps, npst, sfl, apflg, stflg;
char    sh[NAMLEN], dir[NAMLEN], su[MAXSTR], str[MAXSTR], oflg;
} Wave;

typedef struct {
int    np, dnp, n, m, co, trev, afl, ffl, sqw, dc, set;
char   sh[NAMLEN], fnm[MAXSTR], am[MAXSTR], fm[MAXSTR], su[NAMLEN], wf[NAMLEN],  
       df[NAMLEN], dash[16], pwb_[8], pwsw_[8], dres_[8], of_[8], duty_[8],  
       fla_[8], pb1_[8], adb_[8], dnp_[8], c1_[8], c2_[8], c3_[8], min_[8], 
       d1_[8], d2_[8], max_[4], lft_[8], rgt_[4], wr_[4], stch_[8], dc_[4],  
       co_[8], trev_[4], rfl_[4], srev_[8], st_[8], oflg, modpar[512];
double fla, pb1, pwb, pwsw, adb, of, dres, c1, c2, c3, d1, d2, duty, st;
double *f, max, min, lft, rgt, wr, stch, rfl, srev;
} Shape;

typedef struct {
int   n, m, np, dnp;
char  sh[NAMLEN], fnm[MAXSTR], am[MAXSTR];
double *f, max, min, lft, rgt, c1, c2, c3, rfl; 
} Windw;

typedef struct {
int     np, nstp, reps, time, sign;
double *Mx, *My, *Mz, *Scf;
double  T1, T2, rdc, B1max, pw, st, sw, ofs;
} Blodata;

typedef struct {
int     ndir;
char    **dir;
} Wlib;

static char home[MAXSTR], shapes[MAXSTR], waves[MAXSTR], ifn[MAXSTR], col[MAXSTR];

double *Amp, *Pha, *Re, *Im, *Cs, *Sn, tm;
double  rd = M_PI / 180.0, dg = 180.0 / M_PI, pi2 = 2.0 * M_PI, 
        MaxIncr, Pfact, B1rms, MaxAmp;
int     imod, Stype=0, RmsPwr, imx = -1;
int     Usetdir = 0;
Wlib    wv;
Global  g;
Header  h;
Wave   *Wv;
Shape  *Sh, *su, swn;
Windw  *wf, *df;
Blodata bl;

/* -------------------------- house-keeping ---------------------------- */

void setwavelib()	
{
int i;

  wv.ndir = 11;
  wv.dir = (char **) calloc(wv.ndir, sizeof(char *));
  for (i = 0; i < wv.ndir; i++)
    wv.dir[i] = (char *) calloc(NAMLEN, sizeof(char *));
  strcpy(wv.dir[0], "excitation/");
  strcpy(wv.dir[1], "inversion/");
  strcpy(wv.dir[2], "refocusing/");
  strcpy(wv.dir[3], "flipback/");
  strcpy(wv.dir[4], "decoupling/");
  strcpy(wv.dir[5], "mixing/");
  strcpy(wv.dir[6], "composite/");
  strcpy(wv.dir[7], "notch/");
  strcpy(wv.dir[8], "supercycles/");
  strcpy(wv.dir[9], "gradients/");
  strcpy(wv.dir[10],"usr/");
}

double *arry(nl)
int nl;
{
  double *ar;

  ar = (double *) calloc(nl, sizeof(double)); 
  if (!ar)
    pxerr("allocation failure in arry()");
  return ar;
}

double stod(str)		/* string to double conversion */
char *str;
{
  double dn;
  
  if (sscanf(str, "%lf", &dn))
    return dn;
  else
    return 0.0;
}

int stoi(str)		/* string to integer conversion */
char *str;
{
  double dn;
  
  dn = stod(str); 
  return (int) (dn);
}

int findch(fnm, chr)
FILE *fnm;
char chr;
{
  int i = 0, j;

  while ((j = getc(fnm)) != EOF)
  {
    if (j == chr)
    {
      i = 1;
      break;
    }
  }
  return i;
}

double maxval(nn, am)
int nn;
double *am;
{
int   j;
double mxl = 0.0;

  for (j = 0; j < nn; j++) mxl = MAX(mxl, fabs(am[j]));
  return mxl;
}


double pboxRound(a, b)
double a, b;
{
  double sign = 1.0;

  if (a < 0.0)
    sign = -1.0;
  if (a == 0.0)
    return (0.0);
  if (b == 0.0)
    b = 1.0;
  if (b < 0.0)
    b = -b;
  else
    a = sign * b * ((int) (0.1e-5 + 0.5 + fabs(a / b)));
  return (a);
}

int cutstr(str1, str2)
char *str1, *str2;

{
  int i, j, k;

  if ((j = strlen(str2)) == 0) return 0;
  i = 0;
  while((i < j) && (str2[i] != ',')) 
  { 
    str1[i] = str2[i]; 
    i++; 
  }
  str1[i] = '\0';
  k = 0; i++;
  while(i < j) 
  { 
    str2[k] = str2[i]; 
    i++, k++; 
  }
  str2[k] = '\0';
  return j;
}


int cutstr2(str1, str2, ch)
char *str1, *str2, ch;
{
  int i, j, k;

  if ((j = strlen(str2)) == 0) return 0;
  i = j - 1;
  while((i > 0) && (str2[i] != ch)) i--;
  if (i == 0) return 0;
  strcpy(str1, str2); 
  str1[i] = '\0';
  for (k = 0, i++; i < j; i++, k++) { str2[k] = str2[i]; }
  str2[k] = '\0'; 
  return k;
}

void skipcomms(fnm)
FILE *fnm;
{
  int i;
  char str[MAXSTR];

    i = ftell(fnm);
    fscanf(fnm, "%s", str); 
    while (str[0] == '#')
    {
      fgets(str, MAXSTR, fnm);
      i = ftell(fnm);
      fscanf(fnm, "%s", str); 
    }
    fseek(fnm, i-1, 0);
}

int findpar(fnm, str, val)
FILE *fnm;
char *val, *str;
{
  char chr[MAXSTR];

  fseek(fnm, 0, 0); 
  while (fscanf(fnm, "%s", chr) != EOF)
  {
    while ((chr[0] == '#') || (chr[0] == '('))
    {
      fgets(chr, MAXSTR, fnm); fscanf(fnm, "%s", chr);
    }
    if (chr[0] == str[0])
    {
      if (strcmp(chr,str) == 0)
      {
        fscanf(fnm, "%s", chr); 
        if (chr[0] == '=') 
        {
          fscanf(fnm, "%s", val); 
          return 1;
        }
      } 
      else if ((cutstr2(val, chr, '=')) && (strcmp(val, str) == 0))
      {
        strcpy(val, chr); 
        return 1;
      } 
    }
  } 
  return (0);
}

int findpar2(fnm, str, val)
FILE *fnm;
char *val, *str;
{
  char ch, str1[MAXSTR];
  int i = 0;

  fseek(fnm, 0, 0);
  while ((ch = getc(fnm)) == '#')
  {
    fgets(val, MAXSTR, fnm);
    sscanf(val, "%s", str1);
    if ((str[0] == str1[0]) && (strcmp(str,str1) == 0))
    {
      i++;
      break;
    }
  }

  return i;
}

int isname(nstr)
char *nstr;
{

  if((nstr[0] == 'n' && nstr[1] == '\0') || (nstr[0] == '\0'))
    return 0;
  else
    return 1;
}

int setunits(dpar, param) 
double  *dpar;
char    *param;
{
  int  i=0;
  char units[16], val[16];

  if(!isname(param)) return 1;

  units[0]='\0'; val[0]='\0';
  i=sscanf(param, " %[-+0-9.]%s", val, units); 

  if ((i == 1) && (val[0] != '\0'))
  {
    *dpar = stod(val);
    return (-1);
  }
  else if (i == 2)
  {
    *dpar = stod(val);
    switch (units[0])
    {
      case 'p':				/* ppm */
        if (h.sfrq) 
          *dpar *= h.sfrq;
        else
        {
          pxerr("Spectrometer Frequency (sfrq) not specified");
          return 0;
        }
        break;
      case 'k':				/* kilo */
          *dpar *= 1000.0;
        break;
      case 'm':				/* mili */
          *dpar *= 0.001;
        break;
      case 'u':				/* micro */
          *dpar *= 0.000001;
        break;
      case 'H':				/* Hz */
      case 'h':				/* Hz */
        break;
      case 's':				/* seconds */
        break;
      default :
          sprintf(e_str, "%s - Unrecognized units", units);
          pxerr(e_str);
          return 0;
    }
    return 1;
  }
  sprintf(e_str, "%s - Unrecognized units", units);
  pxerr(e_str);
  return 0;
}

int setpwbw(wpw, wbw, ofs, param) 
char    *param;
double *wpw, *wbw, *ofs;
{
  int  i=0, j=0;
  char units[16], val[16], param1[512];

  if(!isname(param)) return 1;
  param1[0] = '\0';
  
  if(cutstr2(param1, param, '-') != 0)
  {
    i = strlen(param) - 1; 
    if ((param1[0] == '(') && (param[i] == ')'))
      param1[0] = ' ', param[i] = '\0';
    else 
      pxerr("Pbox: wave-string: pw/bw format error.");
       
    units[0]='\0'; val[0]='\0';
    i=sscanf(param1, " %[-+0-9.]%s", val, units);
    if ((i == 1) && (val[0] != '\0'))
    {
      tm = stod(val); j=1;
      if (fabs(tm) > 1.0) *ofs = tm;
      else j=0;
    }
    else if (i == 2)
    {
      switch (units[0])
      {
        case 'p': case 'k': case 'h': case 'H':
          j = setunits(ofs, param1);  
          break;
        case 'm': case 'u': case 's': case 'n':  
          j = 0; 
          break;
      }
    }
    if (j == 0) 
    {
      printf("Problem setting pw*bw\n"); 
      return 0;
    }
    else 
      j = 0;
  }

  units[0]='\0'; val[0]='\0';
  i=sscanf(param, " %[-+0-9.]%s", val, units);
  if ((i == 1) && (val[0] != '\0'))
  {
    tm = stod(val);
    if (fabs(tm) > 1.0) *wbw = tm;
    else *wpw = tm;
    j = 1;
  }
  else if (i == 2)
  {
    switch (units[0])
    {
      case 'p':	case 'k': case 'h': case 'H':
        j = setunits(wbw, param); 
        break;
      case 'm': case 'u': case 's':  
        j = setunits(wpw, param); 
        break;
      case 'n': 
        j = 1;
        break;  
    }
  }
  
  if (j == 0) 
    printf("Problem setting pw*bw\n");
  else if(strlen(param1))
  {
    tm = *ofs - *wbw; 
    *ofs = *wbw + (0.5*tm);
    *wbw = tm;
  }
  
  return j;
}


int getnm(fil, ch)
FILE *fil;
char ch;
{
  int i = 0, j=0;

  fseek(fil,0,0);
  while (findch(fil, ch) > 0) j++;
  return j;
}

int fixdir(dir)
char *dir;
{
  int i;

  if ((i = strlen(dir)) && (dir[i-1] != '/')) 
    strcat(dir, "/");
  return i;
}


int sethome(dir)
char *dir;
{
  int i;

  if((dir[0] != '/') || ((i = strlen(dir)) == 0)) return 0;
  if (dir[--i] == '/') dir[i--] = '\0';
  if (i > 0)
  {
    strcpy(home, dir);
    Usetdir = 1;
  }

  return i;
}


int getprm(prm, wstr)
char *prm, *wstr;
{
  int i=0, j=0;

  while (((wstr[i] == ' ') || (wstr[i] == '\t')) && (wstr[i] != '\0'))
    i++;
  if (wstr[i] == '\0') 
    return 0;
  else
  {
    while ((wstr[i] != ' ') && (wstr[i] != ' ') && (wstr[i] != '\0'))
    {
      prm[j] = wstr[i];
      wstr[i] = ' ';
      i++, j++;
    }
  }
  prm[j] = '\0'; 

  return j;
}

int is_reserved(prm)            /* check whether the shape name is reserved */
char *prm;
{
  int j=0;

  switch (prm[0])
  {
      case 'r':				/* rdc */
        if (strcmp(prm, "rdc") == 0)
          j = -1;
        break;
      case 'R':				/* rdc */
        if (strcmp(prm, "RDC") == 0)
          j = -1;
        break;
      case 'n':				/* null */
        if (strcmp(prm, "null") == 0)
          j = 2;
        break;
      case 'N':				/* NULL */
        if (strcmp(prm, "NULL") == 0)
          j = 2;
        break;
      default :
          j = 0;
  }
  return j;
}
