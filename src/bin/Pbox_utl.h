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
/* Pbox_utl.h - Pbox utilities */
/************************************************************************/

void setglo()			/* set Pbox globals */
{
  FILE *fnm;
  
  strcpy(g.shdir, "/vnmrsys/shapelib/");
  strcpy(g.wvdir, "/vnmrsys/wavelib/");
  g.maxst = 65500;
  g.minpw = 0.2;	/* in us */
  g.minpwg = 2.0;	/* in us */
  g.drmin = 1.0;
  g.maxamp = 1024.0;
  g.maxgr = 32767.0;
  g.amres = 0.001;
  g.phres = 0.001;
  g.tmres = 0.05;
  g.dres = 9.0; 
  g.maxpwr = 63;
  g.minpwr = -16;
  g.maxincr = 30.0;  	/* - */
  g.defnp = 100;
  g.maxitr = 5;      	/* - */
  g.maxdev = 2.0;
  g.cmpr = 1;
  g.minstps = 64;
  g.simtime = 60;  	/* - */
  g.gr = 0;
  g.pw = 0.005;		/* in sec */
  g.gpw = 0.001;	/* in sec */
  g.ipdel = 0.0;	/* in us */
  if (access("/vnmr/psg/psgmain.cpp", 0) == 0)          /* VnmrS */
  {
    g.maxst = 512000;
    g.Console = 'v';
  }
  else if (access("/vnmr/psg/PboxM_psg.h", 0) == 0)     /* Must be a Mercury */
  {
    g.maxst = 700;
    g.maxgr = 2000.0;
    g.Console = 'm';
  }
  else g.Console = 'i';
}

void Usetglo()
{
  if (Usetdir == 1)
  {
    strcpy(g.shdir, "/shapelib/");
    strcpy(g.wvdir, "/wavelib/");
  }
}

void getglo(fil)
FILE *fil;
{
  char val[MAXSTR];
  
  val[0] = '\0';
  if ((findpar(fil, "maxst", val) > 0) && (isdigit(val[0]))) 
    g.maxst = stoi(val);
  if ((findpar(fil, "minpw", val) > 0) && (isdigit(val[0]))) 
    g.minpw = stod(val);
  if ((findpar(fil, "pw", val) > 0) && (isdigit(val[0]))) 
    g.pw = stod(val);
  if ((findpar(fil, "pwg", val) > 0) && (isdigit(val[0]))) 
    g.gpw = stod(val);
  if ((findpar(fil, "minpwg", val) > 0) && (isdigit(val[0]))) 
    g.minpwg = stod(val);
  if ((findpar(fil, "drmin", val) > 0) && (isdigit(val[0]))) 
    g.drmin = stod(val);
  if ((findpar(fil, "maxamp", val) > 0) && (isdigit(val[0]))) 
    g.maxamp = stod(val);
  if ((findpar(fil, "maxgr", val) > 0) && (isdigit(val[0]))) 
    g.maxgr = stod(val);
  if ((findpar(fil, "amres", val) > 0) && (isdigit(val[0]))) 
    g.amres = stod(val);
  if ((findpar(fil, "phres", val) > 0) && (isdigit(val[0]))) 
    g.phres = stod(val);
  if ((findpar(fil, "tmres", val) > 0) && (isdigit(val[0]))) 
    g.tmres = stod(val);
  if ((findpar(fil, "dres", val) > 0) && (isdigit(val[0]))) 
    g.dres = stod(val);
  if ((findpar(fil, "maxpwr", val) > 0) && (isdigit(val[0]))) 
    g.maxpwr = stoi(val);
  if ((findpar(fil, "minpwr", val) > 0) && (isdigit(val[0]))) 
    g.minpwr = stoi(val);
  if ((findpar(fil, "maxincr", val) > 0) && (isdigit(val[0]))) 
    g.maxincr = stod(val);
  if ((findpar(fil, "defnp", val) > 0) && (isdigit(val[0]))) 
    g.defnp = stoi(val);
  if ((findpar(fil, "maxitr", val) > 0) && (isdigit(val[0]))) 
    g.maxitr = stoi(val);
  if ((findpar(fil, "maxdev", val) > 0) && (isdigit(val[0]))) 
    g.maxdev = stod(val);
  if (findpar(fil, "cmpr", val) > 0)
  { 
    if ((val[0] == 'y') || (val[0] == '1')) g.cmpr = 1;
    else g.cmpr = 0;
  }
  if ((findpar(fil, "minstps", val) > 0) && (isdigit(val[0]))) 
    g.minstps = stoi(val);
  if ((findpar(fil, "simtime", val) > 0) && (isdigit(val[0]))) 
    g.simtime = stoi(val);
  if (findpar(fil, "shdir", val) > 0) { fixdir(val); strcpy(g.shdir, val); }
  if (findpar(fil, "wvdir", val) > 0) { fixdir(val); strcpy(g.wvdir, val); }
  if ((findpar(fil, "xvgr", val) > 0) && (isdigit(val[0]))) 
    g.gr = stoi(val);
  if ((findpar(fil, "ipdel", val) > 0) && (isdigit(val[0]))) 
  {
    g.ipdel = stod(val);
    if(g.ipdel < g.minpw) 
    {
      g.ipdel = 0.0;
      if (h.reps > 1)
        pxout("\n Warning ! Pbox_globals : ipdel too short, ignored...\n", -1);
    }
  }
}


char slrp(plw, r2)	        /* which SLURP */
double plw, r2;
{
  if(plw < (g.minpw*0.000001)) 
    plw = g.pw;
  else if (plw > 1.0)
    plw = 4.3 / plw;

  r2 /= plw;
  if ((r2 > 7.5) || (r2 < 0.0001)) return '1';
  else if (r2 > 3.5) return '2';
  else if (r2 > 1.5) return '3';
  else if (r2 > 0.875) return '4';
  else if (r2 > 0.625) return '5';
  else return '6';
}


void clip(nn, clev)		/* clip amplitude */
int nn;
double clev;
{
  int     j;
  double  mxl;

  mxl = maxval(nn, Amp);
  for (j = 0; j < nn; j++) 
  { 
    tm = fabs(Amp[j] /= mxl); 
    if (tm > clev) Amp[j] *= clev/tm; 
  }
}

void rmdc(nn)
int  nn;
{
int i;
double dclev;
  
  dclev = MIN(Amp[0], Amp[nn-1]);
  for (i = 0; i < nn; i++) Amp[i] -= dclev;
}

double calib(ofs, nn)		/* pulse calibration */
double ofs;
int nn;
{
  double c, s, re, im, re0 = 0.0, im0 = 0.0;
  int j;

  ofs *= M_PI / (double) nn;
  for (j = 0; j < nn; j++)
  {
    re = Amp[j] * cos(Pha[j]);
    im = Amp[j] * sin(Pha[j]);
    c = cos(ofs * j);
    s = sin(ofs * j);
    re0 += re * c + im * s;
    im0 += im * c - re * s;
  }
  re = (double) nn / sqrt(re0 * re0 + im0 * im0);
  return re;
}

int getext(str1, str2)
char *str1, *str2;
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

FILE *openshape(fname)
char *fname;
{
FILE *inpf;

  if ((inpf = fopen(fname, "r")) == NULL)     /* read file */
  {
    if (fname[0] == '/')
    {
      flerr(fname); return NULL;
    }
    else
    {
      if(home[0]=='\0') 
        (void) strcpy(home, (char *) getenv("HOME"));
      (void) strcat(strcpy(shapes, home), g.shdir);
      (void) strcat(strcpy(ifn, shapes), fname);
      if ((inpf = fopen(ifn, "r")) == NULL)     /* read file */
      {
        (void) strcpy(ifn, SYSSHAPELIB);
        (void) strcat(ifn, fname);
        if ((inpf = fopen(ifn, "r")) == NULL)     /* read file */
        {
          flerr(fname); return NULL;
        }
      }
    }
  }
  sprintf(e_str, "Reading shapefile %s ...\n", ifn);
  pxout(e_str, 0);

return inpf;
}

int getsimdata(bl, fil)
Blodata *bl;
FILE    *fil;
{
int i;
char s[7][32], val[MAXSTR], str[MAXSTR];

  if (findpar2(fil, "Pxsim", val) > 0)
  {
    i = sscanf(val, "%s %s %s %s %s %s %s %s",str,s[0],s[1],s[2],s[3],s[4],s[5],s[6]);
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
  if (!i) 
    pxerr("Blsim : simulation data error in shapefile"); 

return i; 
}

double scale(phase)		/* scale if 0 > phase > 360 deg */
double phase;
{
  if (phase >= 360)
    phase -= 360 * ((int) (phase / 360.0));
  else if (phase < 0)
    phase += 360 * (1 - (int) (phase / 360.0));
  return (phase);
}


int npsqw(s)		/* estimate np in sq wave */
Shape *s;
{
  double sum = 0.0, ax;
  int i, j;

  if(s->n < 1) return s->dnp = (s->fla/s->dres);
  
  j = s->n * s->m;
  for (i = 1; i < j; i+=s->n)
  {
    ax = pboxRound(s->f[i], s->dres); 
    if (ax < 0)
      ax = -ax;
    sum += ax;
  }

  return pboxRound(sum / s->dres, 1.0);
}


int adjsqw(s)	/* adjust np in sq wave */
Shape *s;
{
  double sum = 0.0;
  int i, j;

  if(s->n < 1) return s->dnp = (s->fla/s->dres);
  
  j = s->n * s->m;
  for (i = 1; i < j; i+=s->n)
  {
    if (s->f[i] < 0)
    {
      s->f[i-1] += 180.0;
      s->f[i] = -s->f[i];
    }
    s->f[i] = pboxRound(s->f[i], s->dres);
    sum += s->f[i];
  }
  return pboxRound(sum / s->dres, 1.0);
}

void shapeinfo(sh, opt)			/* not used */
char  *sh, *opt;
{
FILE  *fil;
char   str[MAXSTR];

  (void) strcat(strcpy(str, waves), sh);
  if ((fil = fopen(str, "r")) == NULL)
  {
    (void) strcat(strcpy(str, SYSWAVELIB), sh); 
    if ((fil = fopen(str, "r")) == NULL) err(sh); 
  }

  fscanf(fil, "%s", str);
  if (opt[0] == 't')
  {
    strcpy(opt, str);
    return;
  }
  printf("\n"); 
  while (str[0] == '#')
  {
    printf("%s", str);
    fgets(str, MAXSTR, fil);
    printf("%s", str);
    fscanf(fil, "%s", str);
  }

  if (opt[0] == 'i')
  {
    printf("%s", str);
    while (fgets(str, MAXSTR, fil) != NULL)
      printf("%s", str);
  }
    fclose(fil);
    exit(1);
}

int mkinp(fname, fnm)
FILE *fnm;
char *fname;
{
FILE *fil;
char str[MAXSTR], str1[MAXSTR], str2[MAXSTR];
int  i;

  if ((fil = fopen(fname, "r")) == NULL)
  {
    flerr(fname);
    return 0;
  }

  while (strcmp(str2, "Pbox.inp"))
  {
    fgets(str, MAXSTR, fil);
    i = sscanf(str, "%s %s", str1, str2);
  }

  while ((str[0] == '#') && (str[1] != '#'))
  {
    if (i > 1);
    { str[0] = ' '; fprintf(fnm, "%s", str); }
    fgets(str, MAXSTR, fil);
    i = sscanf(str, "%s %s", str1, str2);
  }
  fclose(fil);

  return 1;
}

int findcom(fnm, str)
FILE *fnm;
char *str;
{
  char str1[MAXSTR], str2[MAXSTR];
  int i = 0;

  fseek(fnm, 0, 0);
  while (getc(fnm) == '#')
  {
    fgets(str1, MAXSTR, fnm);
    sscanf(str1, "%s", str2); 
      if (strcmp(str, str2) == 0)
      {
        i = 1;
        break;
      }
  }
  return i;
}

int getnf(fil)
FILE *fil;
{
int  itnf = 0, i = 0;

  itnf = getnm(fil, '{');
  i = getnm(fil, '}');
  if (!itnf) 
  {
    fclose(fil);
    pxerr("No waves found !");
    return 0;
  }
  else if (i < itnf)
  {
    fclose(fil);
    pxerr("Missing \"}\" delimiter in Pbox.inp file !\n");
    return 0;
  }

  return itnf;
}


int setattn(str)
char   *str;
{
  int i;
  char str1[16];

   h.attn.flg = 0, h.attn.mgn = 0.0;
   if (str[0] == 'i') h.attn.u = 'i';
   else if (str[0] == 'e') h.attn.u = 'e';
   else if (str[0] == 'h') h.attn.u = 'h';
   else if ((str[0] == 'd') || (str[0] == 'n')) h.attn.u = 'd';
   else
   {
     i = strlen(str); i--;
     if (str[i] == 'i') 
       h.attn.u = 'i';
     else if (str[i] == 'I') 
       h.attn.u = 'I';       
     else if (str[i] == 'e') 
       h.attn.u = 'e';
     else if (str[i] == 'E') 
       h.attn.u = 'E';
     else if (str[i] == 'k') 		/* attn to 4.5 kHz, as 'e' but output in Hz */
       h.attn.u = 'k';
     else if (str[i] == 'H') 		/* attn to 45 dB, as 'd' but output in Hz */
       h.attn.u = 'H';
     else if (str[i] == 'd') 
       h.attn.u = 'd';
     else if (isdigit(str[i]))
     {
       strcpy(str1, str);
       h.attn.mgn = stod(str1);
       h.attn.u = 'E';
       return 1;
     }
     else
     {
       pxerr("Pbox: attn parameter - unrecognized units. Use e.g. d, 49d, 5.0i, 4.5e, etc.");
       return 0;
     }
     if (isdigit(str[i-1]))
     {
       strcpy(str1, str);
       str1[i] = '\0';
       h.attn.mgn = stod(str1);
       h.attn.flg = 1;
       if ((h.attn.u == 'E') || (h.attn.u == 'I'))
         h.attn.flg = 0;
     }
     else
     {
       pxerr("Pbox: attn parameter - unrecognized units. Use e.g. d, 49d, 5.0i, 4.5e, etc.");
       return 0;
     }

     if ((h.attn.mgn < 0.001) && ((h.attn.u == 'i') || (h.attn.u == 'e') || 
         (h.attn.u == 'h') || (h.attn.u == 'H') || (h.attn.u == 'k')))   /* NULL pulses */
     {
       h.attn.flg = -1;
       if ((h.attn.u == 'h') || (h.attn.u == 'H') || (h.attn.u == 'k'))
         h.attn.u = 'i';
     }
   }
   return 1;
}

char ofres(str)
char   *str;
{
int i;
char ch;

     i = strlen(str); i--;
     ch = str[i];
     if ((ch == 't') || (ch == 'T'))
     { 
       str[i] = '\0';
       if ((stod(str) == 90.0) || (stod(str) == 270.0))
         return '0';
       else
         return ch;
     }
     else if (ch == 'r') 
     { 
       str[i] = '\0';
       return ch;
     }
     else
       return 'n';
}

void reportheader()
{
  printf("Input Data :\n");
  printf("~~~~~~~~~~~~\n");

  printf("  name = %s, type = %s, mod = %s,\n", h.name, h.type, h.mod); 
  printf("  steps = %6d,    maxincr = %4.1f,   stepsize = %.2f\n", 
            h.np, h.maxincr, h.stepsize); 
  if ((h.attn.flg) || (h.attn.u == 'E') || (h.attn.u == 'I')) 
    printf("  attn = %6.3f%c, refofs = %7.1f,  sucyc =   %s,\n", 
              h.attn.mgn, h.attn.u, h.offset, h.su);
  else 
    printf("  attn = %c,      refofs = %7.1f,  sucyc =   %s,\n", 
              h.attn.u, h.offset, h.su);
  printf("  reps = %d,      header =     %s,  wrap = %.4f\n", 
            h.reps, h.coms, h.wrp);

  printf("  bsim = %s,      T1 = %9.6f,    T2 = %9.6f (s)\n", h.bls, h.T1, h.T2);
  printf("  dcyc = %6.4f, sw = %9.2f,    sfrq = %.6f\n", 
            h.dcyc, h.sw, h.sfrq);
  printf("  bscor = %s,      pad1 = %d,      pad2 = %d\n", h.bscor, h.pad1, h.pad2); 
  printf("  rdc = %.2f\n", h.rdc); 

  printf("  ref_pw90 = %9.2f (us),        ref_pwr = %d (dB)\n\n", h.pw90, h.rfpwr);
}

void writeheader(fil, ch)
FILE *fil;
char ch;
{
  int i = 0;

  fprintf(fil, "%c Pbox.inp data :\n", ch);
  fprintf(fil, "%c ~~~~~~~~~~~~~~~~~~~~~~~~~~~Waves~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", ch);
  fprintf(fil, "%c   shape   bw(/pw)   ofs  st  ph  fla  trev  d1  d2   d0   wrp  php\n", ch);
  fprintf(fil, "%c ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n", ch);
  for(i=0; i<h.itnf; i++)
    fprintf(fil, "%c %s\n", ch, Wv[i].str);
  fprintf(fil, "%c\n%c", ch, ch);
  if (h.type[0] !='\0') { fprintf(fil, " type      = %-10s", h.type); i++; }
  if (h.mod[0] !='\0') { fprintf(fil, " mod       = %-10s", h.mod); i++; }
  if (h.dres_[0] !='\0') { fprintf(fil, " dres      = %-10s", h.dres_); i++; }
  if (h.np_[0] !='\0')                    { 
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " steps     = %-10s", h.np_); i++; }
  if (h.stepsize_[0] !='\0')                     {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " stepsize  = %-10s", h.stepsize_); i++; }
  else if (h.maxincr_[0] !='\0')                     {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " maxincr   = %-10s", h.maxincr_); i++; }
  if (h.attn_[0] !='\0')                  {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " attn      = %-10s", h.attn_); i++; }
  if (h.offset_[0] !='\0')                    {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " refofs    = %-10s", h.offset_); i++; }
  if (h.wrp_[0] !='\0')                    {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " wrap      = %-10s", h.wrp_); i++; }
  if (h.sfrq_[0] !='\0')                  {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " sfrq      = %-10s", h.sfrq_); i++; }
  if (h.su[0] !='\0')                   {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " sucyc     = %-10s", h.su); i++; }
  if (h.reps_[0] !='\0')                  {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " reps      = %-10s", h.reps_); i++; }
  if (h.coms[0] !='\0')                    {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " header    = %-10s", h.coms); i++; }
  if (h.bscor[0] !='\0')                    {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " bscor     = %-10s", h.bscor); i++; }
  if (h.pad1_[0] !='\0')                  {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " pad1      = %-10s", h.pad1_); i++; }
  if (h.pad2_[0] !='\0')                  {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " pad2      = %-10s", h.pad2_); i++; }
  if (h.bls[0] !='\0')                  {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " bsim      = %-10s", h.bls); i++; }
  if (h.T1_[0] !='\0')                {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " T1        = %-10s", h.T1_); i++; }
  if (h.T2_[0] !='\0')                {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " T2        = %-10s", h.T2_); i++; }
  if (h.rdc_[0] !='\0')                {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " rdc       = %-10s", h.rdc_); i++; }
  if (h.dcyc_[0] !='\0')                        {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " dcyc      = %-10s", h.dcyc_); i++; }
  if (h.sw_[0] !='\0')                        {
    if (i>2) { fprintf(fil, "\n%c", ch); i=0; }
    fprintf(fil, " sw        = %-10s", h.sw_); i++; }
  if (strlen(col)) fprintf(fil, "\n%c %s", ch, col);

  if (h.pw90 > 0.01)                    
  {
    fprintf(fil, "\n%c\n%c RF Calibration Data : ", ch, ch);
    fprintf(fil, " ref_pwr   = %-10d", h.rfpwr); 
    fprintf(fil, " ref_pw90  = %.2f\n", h.pw90);  
    fprintf(fil, "%c ~~~~~~~~~~~~~~~~~~~~~\n", ch);
  }
  else
    fprintf(fil, "\n%c\n", ch);
  
  return;
}  

void sethdrstr()
{
  h.type[0]='n', h.type[1]='\0';
  sprintf(h.name, "Pbox");
  h.dres_[0]='n', h.dres_[1]='\0';  
  h.stepsize_[0]='n', h.stepsize_[1]='\0';
  h.np_[0]='n', h.np_[1]='\0';
  h.maxincr_[0]='n', h.maxincr_[1]='\0';
  h.attn_[0]='i', h.attn_[1]='\0'; setattn(h.attn_);
  h.sfrq_[0]='n', h.sfrq_[1]='\0';
  h.offset_[0]='n', h.offset_[1]='\0';
  h.wrp_[0]='n', h.wrp_[1]='\0';
  h.su[0]='d', h.su[1]='\0';
  h.reps_[0]='2', h.reps_[1]='\0';
  h.coms[0]='y', h.coms[1]='\0';
  h.pad1_[0]='n', h.pad1_[1]='\0';
  h.pad2_[0]='n', h.pad2_[1]='\0';
  h.bls[0]='n', h.bls[1]='\0';
  h.T1_[0]='n', h.T1_[1]='\0';
  h.T2_[0]='n', h.T2_[1]='\0';
  h.rdc_[0]='n', h.rdc_[1]='\0';
  h.dcyc_[0]='n', h.dcyc_[1]='\0';
  h.sw_[0]='n', h.sw_[1]='\0';
  h.pw90_[0]='n', h.pw90_[1]='\0';
  h.rfpwr_[0]='n', h.rfpwr_[1]='\0';
  sprintf(h.ptype, "selective"); 
  if (g.Console == 'm') 
    h.mod[0] = 'c', h.mod[1] = '\0';
  else
    h.mod[0] = 'n', h.mod[1] = '\0';
  h.bscor[0] = 'n', h.bscor[1] = '\0';
  
  h.itnw = 0, h.itnd = 0, h.itns = 0, h.np   = 0; 
  h.npst = 1, h.cal = 10, h.posflg  = 1;
  h.pwr  = 0, h.pwrf = 4095, h.imax  = 0, h.calflg = 0, h.dnp = 1.0;
  h.Bref = 0.0, h.maxtof = 0.0, h.dmf = 0.0, h.pw = g.pw;
  Pfact=1.0, MaxIncr=0.0, B1rms=0.0, MaxAmp=0.0; RmsPwr=0;
}

void sethdr()
{
  h.rfpwr = 0, h.itnf = 0; 
  h.tfl   = 0, h.sfl  =-1, h.reps  = 2, h.pad1 = 0, h.pad2 = 0;  
  
  h.T1 = 0.0, h.T2 = 0.0, h.dres = 0.0, h.pw90 = 0.0, h.offset = 0.0; 
  h.sw = 0.0, h.rdc = 0.0, h.wrp = 0.0, h.sfrq = 0.0, h.dcyc = 1.0;    
  h.maxincr = g.maxincr, h.stepsize=0.0;  
}

void set_type(tp)
char tp;
{
    switch (tp) 
    {
      case 'R': case 'r':
        h.tfl = 0; h.type[0] = 'r'; h.type[1] = '\0';  
        break;
      case 'D': case 'd':
        h.tfl = -1; h.type[0] = 'd'; h.type[1] = '\0';    
        break;
      case 'G': case 'g':
        h.tfl = 1; h.type[0] = 'g'; h.type[1] = '\0';  
        break;  
      case 'W': case 'w':
        h.tfl = 2; h.type[0] = 'w'; h.type[1] = '\0';  
        break;  
      case 'n':
        break;
      default :
        pxerr("Unrecognized file format or type");
    }
}

void setname(nm)
char *nm;
{
  int  i=0;

  i = strlen(nm); 

  switch (nm[i-1]) 
  {
    case 'F': 
      if((nm[i-3] == '.') && (nm[i-2] == 'R'))
      {
        set_type('r');
        nm[i-3]='\0';
      }
      break;
    case 'C': 
      if((nm[i-4] == '.') && (nm[i-3] == 'D') && (nm[i-2] == 'E'))
      {
        set_type('d');
        nm[i-4]='\0';
      }
      break;
    case 'D': 
      if((nm[i-4] == '.') && (nm[i-3] == 'G') && (nm[i-2] == 'R'))
      {
        set_type('g');
        nm[i-4]='\0';
      }
  }
  strcpy(h.name, nm); 
}

void gethdr(fil)
FILE *fil;
{
  char val[MAXSTR];
  int  i;

  val[0] = '\0';
  if (findpar(fil, "type", val) > 0) 				    /* type */
  { set_type(val[0]); val[0]='\0'; }
  if (findpar(fil, "name", val) > 0)				    /* name */ 
  { setname(val); val[0]='\0'; }	        
  if ((findpar(fil, "dres", h.dres_) > 0) && (isdigit(h.dres_[0]))) /* dres */
    h.dres = stod(h.dres_); ;
  if ((findpar(fil, "stepsize", h.stepsize_) > 0) &&                /* stepsize */ 
      (setunits(&h.stepsize, h.stepsize_) > 0)) 
      h.stepsize *= 1000000.0;  /* convert to us */ 
  if ((findpar(fil, "steps", h.np_) > 0) && (isdigit(h.np_[0])))   /* # of steps */
      h.np = stoi(h.np_); 
  if (h.stepsize > 0.0001)
  { 
    h.np = 0; h.np_[0]='n', h.np_[1]='\0';
  }
  if ((findpar(fil, "maxincr", h.maxincr_) > 0) && 
      (isdigit(h.maxincr_[0])))					    /* max incr */
    h.maxincr = stod(h.maxincr_);
  if (fabs(h.maxincr) < 0.001) h.maxincr = g.maxincr;
  if (findpar(fil, "attn", h.attn_) > 0) setattn(h.attn_);          /* attenuation */
  if ((findpar(fil, "sfrq", h.sfrq_) > 0) && (isdigit(h.sfrq_[0]))) /* sfrq */
    h.sfrq = stod(h.sfrq_);
  if ((findpar(fil, "wrap", h.wrp_) > 0) && (isdigit(h.wrp_[0])))  /* wrap */
    h.wrp = stod(h.wrp_);
  if (findpar(fil, "refofs", h.offset_) > 0)   	  	           /* offset */
    setunits(&h.offset, h.offset_); 
  if (findpar(fil, "sucyc", val) > 0) 	   			   /* supercyc */
  { strcpy(h.su, val); val[0]='\0'; }
  if (h.su[0] == 'd' && h.su[1] == '\0') h.sfl = -1;
  else if (h.su[0] == 'n' && h.su[1] == '\0') h.sfl = 0;
  else h.sfl = 1;
  if ((findpar(fil, "reps", h.reps_) > 0) && (isdigit(h.reps_[0]))) /* reports */
    h.reps = stoi(h.reps_);
  if (findpar(fil, "header", val) > 0) 
  { strcpy(h.coms, val); val[0]='\0'; }                 	     /* comments */
  if ((findpar(fil, "pad1", h.pad1_) > 0) && (isdigit(h.pad1_[0])))  /* pad1 */
      h.pad1 = stoi(h.pad1_); 
  if ((findpar(fil, "pad2", h.pad2_) > 0) && (isdigit(h.pad2_[0])))  /* pad2 */
      h.pad2 = stoi(h.pad2_); 
  if (findpar(fil, "bscor", val) > 0) 
  { strcpy(h.bscor, val); val[0]='\0'; }                 	     /* BS compensation */
  if (findpar(fil, "bsim", val) > 0) 
  { strcpy(h.bls, val);	 val[0]='\0'; }                              /* simulator */
  if (findpar(fil, "T1", h.T1_) > 0)                                 /* T1 */ 
    setunits(&h.T1, h.T1_);
  if (findpar(fil, "T2", h.T2_) > 0)                                 /* T2 */ 
    setunits(&h.T2, h.T2_);
  if ((findpar(fil, "rdc", h.rdc_) > 0) && (isdigit(h.rdc_[0])))    /* rdc */ 
    setunits(&h.rdc, h.rdc_);
  if ((findpar(fil, "dcyc", h.dcyc_) > 0) && (isdigit(h.dcyc_[0])))  /* dcyc */
  {
    h.dcyc = stod(h.dcyc_);
    if (h.dcyc > 1.0 || h.dcyc <= 0.0) h.dcyc = 1.0;
  }
  if (findpar(fil, "sw", h.sw_) > 0)		     		     /* sw */
    setunits(&h.sw, h.sw_);
  if ((findpar(fil, "ref_pw90", h.pw90_) > 0) && (isdigit(h.pw90_[0])))	/* pw90 */
    h.pw90 = stod(h.pw90_);						/* ref_pwr */
  if ((findpar(fil, "ref_pwr", h.rfpwr_) > 0) && 
      ((isdigit(h.rfpwr_[0])) || (h.rfpwr_[0] == '-')))
    h.rfpwr = stoi(h.rfpwr_);
  else h.pw90 = 0.0;
  if (findpar(fil, "ptype", val) > 0) 
  { strcpy(h.ptype, val); val[0]='\0'; }                        	/* ptype  */
  if (findpar(fil, "mod", val) > 0) 		/* modulation mode */
  {
    if (val[0] == 's') h.mod[0] = 's';		/* DANTE + SAPS */
    else if (val[0] == 'c') h.mod[0] = 'c';	/* interleaved DANTE */
    else h.mod[0] = 'n';			/* WFG and AP pulses */
  }
  if (h.pw90 > 0.01) h.Bref = 250.0 / h.pw90;
}


int ispar(prm)					/* is it a parameter */
char *prm;
{
char str[10];
int  i, k;

  if ((i = strlen(prm)) > 9)
    return (0);

  k=1;
  while (prm[k]!='\0')				/* copy & remove '-' */
  {
    str[k-1]=prm[k];
    k++;
  }
  str[k-1]='\0';
  k=0;

  switch (str[0])                                       /* k(max) = 26, ptype 9->8 */
  {
    case 'a' :						/* attenuation */
      if (strcmp("attn", str) == 0) k=1;
      break;
    case 'b' :						/* simulator */	
      if (strcmp("bsim", str) == 0) k=2; 
      else if (strcmp("bscor", str) == 0) k=24;        /* BS compensator */
      break;
    case 'd' :
      if (strcmp("dres", str) == 0) k=3;  	/* dres */
      else if (strcmp("dcyc", str) == 0) k=4; 	/* dcyc */
      break;
    case 'h' : 						/* comments */
      if (strcmp("header", str) == 0) k=5;  
      break;
    case 'm' : 						/* max incr */
      if (strcmp("maxincr", str) == 0) k=6; 
      else if (strcmp("mod", str) == 0) k=23;           /* mod */
      break;
    case 'n' :						/* name */
      if (strcmp("name", str) == 0) k=7;
      break;
    case 'p' :
      if (strcmp("ptype", str) == 0) k=8;    	/* ptype */
      else if (strcmp("pad1", str) == 0) k=25;  /* pad1 */
      else if (strcmp("pad2", str) == 0) k=26;  /* pad2 */
      break;
    case 'r' :						
      if (strcmp("rdc", str) == 0) k=9;      /* Radiation damping */
      else if (strcmp("refofs", str) == 0) k=10;   	   /* ref. offset */
      else if (strcmp("reps", str) == 0) k=11;     /* reports */
      else if (strcmp("ref_pw90", str) == 0) k=12; /* pw90 */
      else if (strcmp("ref_pwr", str) == 0) k=13;  /* ref_pwr */
      break;
    case 's' :
      if (strcmp("stepsize", str) == 0) k=14;  	/* stepsize */                     
      else if (strcmp("steps", str) == 0) k=15; /* # of steps */
      else if (strcmp("sfrq", str) == 0) k=16;  /* sfrq */
      else if (strcmp("sucyc", str) == 0) k=17; /* supercyc */
      else if (strcmp("sw", str) == 0) k=18;    /* sw */
      break;
    case 't' :						/* type */
      if (strcmp("type", str) == 0) k=19; 
      break;
    case 'T' :
      if (strcmp("T1", str) == 0) k=20;  		/* T1 */
      else if (strcmp("T2", str) == 0) k=21;   	        /* T2 */
      break;
    case 'w' :						/* wrap */
      if (strcmp("wrap", str) == 0) k=22; 
      break;
    default :
      k=0;
  }
  
  return (k);
}

void setpar(pid, val)					/* set h parameter */
int pid;
char *val;
{
  int  i;

  switch (pid)
  {
    case 1 :						/* attenuation */
      strcpy(h.attn_, val);
      setattn(h.attn_);	
      break;
    case 2 :						/* Bloch simulator */	    
      strcpy(h.bls, val);
      break;
    case 3 :						/* dres */
      strcpy(h.dres_, val);
      if (isdigit(h.dres_[0])) 
        h.dres = stod(h.dres_);
      break;
    case 4 :						/* dcyc */
      strcpy(h.dcyc_, val);
      if (isdigit(h.dcyc_[0]))
      {
        h.dcyc = stod(h.dcyc_);
        if (h.dcyc > 1.0 || h.dcyc <= 0.0) h.dcyc = 1.0;
      }
      break;
    case 5 : 						/* comments */
      strcpy(h.coms, val);
      break;
    case 6 : 						/* max incr */
      strcpy(h.maxincr_, val);
      if (isdigit(h.maxincr_[0]))                  
      {
        h.maxincr = stod(h.maxincr_);
        if (fabs(h.maxincr) < 0.001)
          h.maxincr = g.maxincr;
      }                       
      break;
    case 7 :						/* name */
      setname(val);
      break;
    case 8 :
      strcpy(h.ptype, val);				/* ptype */
      break;
    case 9 :
      strcpy(h.rdc_, val);
      if (isdigit(h.rdc_[0]))
        setunits(&h.rdc, h.rdc_);	
      break;
    case 10 :						/* offset */	
      strcpy(h.offset_, val);
      setunits(&h.offset, h.offset_);
      break;
    case 11 :						/* reports */
      strcpy(h.reps_, val);
      if (isdigit(h.reps_[0]))
        h.reps = stoi(h.reps_);
      break;
    case 12 :						/* ref_pw90 */
      strcpy(h.pw90_, val);
      if (isdigit(h.pw90_[0]))
      {
        h.pw90 = stod(h.pw90_);
        h.Bref = 250.0 / h.pw90;
      }
      break;
    case 13 :						/* ref_pwr */
      strcpy(h.rfpwr_, val);
      if ((isdigit(h.rfpwr_[0])) || (h.rfpwr_[0] == '-')) 
        h.rfpwr = stoi(h.rfpwr_);
      else 
        h.pw90 = 0.0;
      break;
    case 14 :						/* stepsize */
      strcpy(h.stepsize_, val);
      i = setunits(&h.stepsize, h.stepsize_);
      if (i > 0)
        h.stepsize *= 1000000.0;  /* convert to us */
      break;
    case 15 :						/* # of steps */
      strcpy(h.np_, val);
      if (isdigit(h.np_[0])) 
        h.np = stoi(h.np_);
      break;
    case 16 :						/* sfrq */
      strcpy(h.sfrq_, val);
      if (isdigit(h.sfrq_[0])) 
        h.sfrq = stod(h.sfrq_);	
      break;
    case 17 :						/* supercyc */
      strcpy(h.su, val);
      if (h.su[0] == 'd' && h.su[1] == '\0') h.sfl = -1;
      else if (h.su[0] == 'n' && h.su[1] == '\0') h.sfl = 0;
      else h.sfl = 1;
      break;
    case 18 :						/* sw */
      strcpy(h.sw_, val);
      setunits(&h.sw, h.sw_);
      break;
    case 19 :						/* type */
      set_type(val[0]);
      break;
    case 20 :						/* T1 */
      strcpy(h.T1_, val);
      setunits(&h.T1, h.T1_);	
      break;
    case 21 :						/* T2 */
      strcpy(h.T2_, val);
      setunits(&h.T2, h.T2_);	
      break;
    case 22 :						/* wrap */
      strcpy(h.wrp_, val);
      if (isdigit(h.wrp_[0])) 
        h.wrp = stod(h.wrp_);	
      break;
    case 23 : 						/* mod */
      strcpy(h.mod, val);
      break;
    case 24 : 						/* BS comp */
      strcpy(h.bscor, val);
      break;
    case 25 :						/* pad1 */
      strcpy(h.pad1_, val);
      if (isdigit(h.pad1_[0])) 
        h.pad1 = stoi(h.pad1_);
      break;
    case 26 :						/* pad2 */
      strcpy(h.pad2_, val);
      if (isdigit(h.pad2_[0])) 
        h.pad2 = stoi(h.pad2_);
      break;
    default :
      pxerr("setpar - wrong parameter ID");
  }
}


void getcal(fil, tp)
FILE *fil;
char tp;
{
  char val[MAXSTR];
   
  val[0] = '\0';
  if (tp == 'd') 
  {
    set_type(tp);
    h.su[0] = 'd', h.su[1] = '\0'; h.sfl = -1;
    setattn("e");	       				     /* attenuation */
  }
  else if (tp == 'c')
  {
    set_type('r');
    h.su[0] = 'n', h.su[1] = '\0'; h.sfl = 0;
    setattn("e");	       				     /* attenuation */
  }
  if ((findpar(fil, "ref_pw90", val) > 0) && (isdigit(val[0])))	/* ref_pw90 */
  { h.pw90 = stod(val);	val[0]='\0'; }				/* ref_pwr */
  if ((findpar(fil, "ref_pwr", val) > 0) && ((isdigit(val[0])) || (val[0] == '-')))
    h.rfpwr = stoi(val);
  else h.pw90 = 0.0;
}


void resetwave(Wave *wa)
{

  wa->sh[0] = '\0'; wa->dir[0] = '\0';
  wa->su[0] = 'n', wa->oflg = 'n', wa->su[1] = '\0'; 
  wa->str[0] = '{', wa->str[1] = ' ', wa->str[2] = '\0';
  wa->fla = 0.0;
  wa->np = 0, wa->n1 = 0, wa->n2 = 0, wa->apflg = 0, wa->nps = 1, wa->npst = 1;
  wa->trev = 0, wa->sfl = 0, wa->stflg = 0;
  wa->bw = 0.0, wa->of = 0.0, wa->pw = 0.0, wa->d0 = 0.0, wa->d1 = 0.0, wa->d2 = 0.0;
  wa->st = 0.0, wa->ph = 0.0, wa->pwb = 0.0, wa->incr = 0.0, wa->wrp = 0.0, wa->dash = -1.0;
  wa->php = -1.0;
}

int setwave(wa, tfl)
Wave *wa;
int  tfl;
{
char ch, wstr[512], str[256], prm[256];
int i=0, j, k=1;

    if (wa->str[0] != '{')
    {
      sprintf(e_str, "Pbox: error in wave format");
      return 0;
    }
    else
      strcpy(wstr, wa->str);
    wstr[0]=' ';
    while ((k) && (i < 14) && (k=getprm(prm, wstr)) && (prm[0] != '}'))
    {
      j = strlen(prm)-1; 
      if (prm[j] == '}') 
        prm[j] = '\0', k = 0, j--;
      if ((prm[0] != 'n') || (i == 0))
      {
        switch (i)
        {
          case 0 :						/* shape */
            cutstr2(wa->dir, prm, '/');
            if ((j = cutstr2(wa->sh, prm, '-')) > 0)
              wa->dash = stod(prm);
            else strcpy(wa->sh, prm);
            break;
          case 1 :						/* pw*bw */
            if(prm[j]=='/') prm[j]='\0';
            if(prm[0]=='/') prm[0]=' '; 
            if (((cutstr2(str, prm, '/') != 0) && 
            (setpwbw(&wa->pw, &wa->bw, &wa->of, str) == 0)) ||
            (setpwbw(&wa->pw, &wa->bw, &wa->of, prm) == 0))
              return 0;
            break;
          case 2 :						/* offset */
            if (setunits(&wa->of, prm) == 0) return 0;    
            break;
          case 3 :						/* status */
            wa->stflg = 1;
            if ((wa->st = stod(prm)) != 0.0)
            { 
              if (wa->st < 0.0) 
              {
                wa->st = 0.0;
                pxout("\nWarning !!! Illegal status. Reset to 0.0\n", 1);
              }
              wa->trev += (int) (wa->st + 0.5);
              wa->trev = abs(wa->trev%2);
            }
            break;
          case 4 :						/* phase & sucyc */
            strcpy(wa->su, prm);
            if ((isdigit(prm[0])) || ((prm[0] == '-') && (isdigit(prm[1]))))
            { cutstr(str, wa->su); wa->ph = stod(str); }
            if ((wa->su[0] == 'n' && wa->su[1] == '\0') || (wa->su[0] == '\0')) 
              wa->sfl = 0;
            else if (wa->su[0] == 'd' && wa->su[1] == '\0') 
              wa->sfl = -1;
            else 
              wa->sfl = 1;	
            break;
          case 5 :					       /* flipangle */
              wa->oflg = ofres(prm);
              wa->fla = stod(prm); 
              if ((tfl > 0) && (fabs(wa->fla) > 100.0))
                wa->fla = 100.0;
            break;
          case 6 :					       /* t-rev flg */
            if (prm[0] == 'y') wa->trev += 1;
            wa->trev = abs(wa->trev%2);
            break;
          case 7 :				   	       /* rof1 */
            if (setunits(&wa->d1, prm) == 0) return 0; 
            break;
          case 8 :				   	       /* rof2 */
            if (setunits(&wa->d2, prm) == 0) return 0;
            break;
          case 9 :				   	       /* rof0 */
            if (prm[0] == 'a') wa->apflg = 1;
            else if (setunits(&wa->d0, prm) == 0) return 0;
            break;
          case 10 :					       /* wrap */
            wa->wrp = stod(prm);
            break;
          case 11 :					       /* php */
            wa->php = stod(prm);
            break;
          default :
            pxerr("Pbox: wave-string format error."); return 0;
        }
      }
      prm[0] = '\0';
      i++;
    }
    return i;
}

int getwave(fil, wa, tfl)
FILE *fil;
Wave *wa;
int  tfl;
{
char str[256], prm[256];
int i=0, j=0, k=1;

  resetwave(wa); 

  while ((j = getc(fil)) != EOF)
  {
    if (j == '+')
      wa->apflg = 1;
    else if (j == '{')
      break;
  }

  while ((k) && (i < 12) && (k = fscanf(fil, "%s", prm)) && (prm[0] != '}'))
  {
    j = strlen(prm);
    if (prm[j-1] == '}') prm[j-1] = '\0', k = 0; 
    strcat(strcat(wa->str, prm), " ");
    i++;
  }

  strcat(wa->str, "}");
  i = setwave(wa, tfl);
  return i;
}


void reportwave(wa, reps)
Wave *wa;
int   reps;
{
    printf("  sh = %s,\n  su = %s,\n", wa->sh, wa->su);
    printf("  bw = %5.1f,    pw = %9.7f, ofs = %5.1f,  st = %5.1f\n", 
              wa->bw, wa->pw, wa->of, wa->st);
    printf("  fla = %5.1f,    ph = %5.1f,     trev = %5.1f\n", 
              wa->fla, wa->ph, wa->trev);
    printf("  d0 = %7.6f,  d1 = %7.6f,  d2 = %7.6f\n", 
              wa->d0, wa->d1, wa->d2);
    printf("  wrp = %.4f,     php = %.4f\n", 
              wa->wrp, wa->php);
    if (reps > 4)
    {
      printf("  np = %d,  n1 = %d,  n2 = %d\n", wa->np, wa->n1, wa->n2);
      printf("  nps = %d,  sfl = %d\n", wa->nps, wa->sfl);
      printf("  dash = %7.2f,  pwb = %6.3f\n", wa->dash, wa->pwb);
    }
}


int nsuc(str)
char *str;
{
  char **str1;
  int i, j, nsu;

  if(!strlen(str)) return 0;

  i=0, nsu=1;
  while(str[i] != '\0')
  { 
    if (str[i] == ',') nsu++;
    i++;
  }

  if (nsu == 1) return 1;
  else if (nsu > 1)
  {
    str1 = (char **) calloc(nsu, sizeof(char *)); 
    for(i=0; i<nsu; i++)
    {
      str1[i] = (char *) calloc(NAMLEN, sizeof(char));
      cutstr(str1[i], str);
    }

    i = 0, j = 0;
    while(i < nsu)
    {
      while((i < nsu) && (str1[i][0] =='\0')) i++;
      j = i+1;
      while(j < nsu)
      {
        if ((i < j) && (str1[j][0] !='\0') && (strcmp(str1[i], str1[j]) == 0))
          str1[j][0] = '\0';
        j++;
      }
      i++;
    }

    i=1;
    strcpy(str, str1[0]);
    for(j=1; j<nsu; j++) 
    {
      if(str1[j][0] != '\0')
      {
        strcat(str, ",");
        strcat(str, str1[j]); 
        i++;
      }
    }
  }
  return i;
}

int minnmb(s, maxincr)
Shape *s;
double maxincr;
{
int nn;

      if (strcmp(s->am, "sc") == 0)				/* sinc */
        nn = (int) (360.0 * (s->c1 + 1.0) / maxincr);
      else if (strcmp(s->fm, "ls") == 0)
        nn = (int) (180.0 * fabs(s->c1) / maxincr); 		/* lin sweep */
      else if ((strcmp(s->fm, "htan") == 0) || (strcmp(s->fm, "cs") == 0))
        nn = (int) (180.0 * fabs(s->pwb) / maxincr);		/* sech & sin/cos */
      else if (strcmp(s->am, "ft") == 0)	/* ft */
        nn = (int) (90.0 * s->m / maxincr);
      else if (strcmp(s->am, "fs") == 0)	/* fser */
        nn = (int) (90.0 * (2 * s->m - 1) / maxincr);
      else
        nn = s->dnp;		/* other */

return nn;
}

void reset_shn(s)
Shape *s;
{
  s->co = 0, s->dc = 0, s->np = 0, s->dnp = 0;
  s->of = 0.0, s->dres = 0.0, s->srev = 0.0, s->fla = 0.0;
  s->c1 = 0.0, s->c2 = 0.0, s->c3 = 0.0; 
  s->pb1 = 0.0, s->pwb = 0.0, s->pwsw = 0.0;
  s->max = 1.0, s->min = 0.0, s->lft = 0.0, s->rgt = 1.0;
  s->wr = 0.0, s->stch = 0.0;
  s->duty = 1.0;
  s->trev = 0;
  s->st = 0.0;
  s->oflg = 'n';
}


void resetshape(s)
Shape *s;
{
  reset_shn(s);

  s->n = 0, s->m = 0;
  s->rfl = 0.0, s->adb = 0.0; 
  s->set = 0, s->afl = 0, s->ffl = 0, s->sqw = 0; 
  strcpy(s->sh, "n"); strcpy(s->am, "sq");
  strcpy(s->fm, "n"); strcpy(s->su, "n");
  s->d1 = 0.0, s->d2 = 0.0;
  s->wf[0] = 'n',  s->df[0] = 'n',   s->dash[0] = 'n';
  s->wf[1] = '\0', s->df[1] = '\0',  s->dash[1] = '\0';
  s->pwb_[0]='n',  s->pwb_[1]='\0';  s->pwsw_[0]='n', s->pwsw_[1]='\0'; 
  s->dres_[0]='n', s->dres_[1]='\0'; s->of_[0]='n',   s->of_[1]='\0';
  s->duty_[0]='n', s->duty_[1]='\0'; s->fla_[0]='n',  s->fla_[1]='\0';
  s->pb1_[0]='n',  s->pb1_[1]='\0';  s->adb_[0]='n',  s->adb_[1]='\0';
  s->dnp_[0]='n',  s->dnp_[1]='\0';  s->c1_[0]='n',   s->c1_[1]='\0';
  s->c2_[0]='n',   s->c2_[1]='\0';   s->c3_[0]='n',   s->c3_[1]='\0';
  s->min_[0]='n',  s->min_[1]='\0';  s->d1_[0]='n',   s->d1_[1]='\0';
  s->d2_[0]='n',   s->d2_[1]='\0';   s->max_[0]='n',  s->max_[1]='\0';
  s->lft_[0]='0',  s->lft_[1]='\0';  s->rgt_[0]='1',  s->rgt_[1]='\0';
  s->wr_[0]='n',   s->wr_[1]='\0';   s->stch_[0]='n', s->stch_[1]='\0';
  s->dc_[0]='n',   s->dc_[1]='\0';   s->co_[0]='n',   s->co_[1]='\0';
  s->trev_[0]='n', s->trev_[1]='\0'; s->srev_[0]='n', s->srev_[1]='\0';
  s->rfl_[0]='n',  s->rfl_[1]='\0';
  s->st_[0]='n',   s->st_[1]='\0';
}

void resetwindow(wn)
Windw *wn;
{
  wn->n = 0, wn->m = 0;
  wn->np = 0, wn->dnp = 0.0; 
  strcpy(wn->am, "sq");
  wn->sh[0]='\0';
  wn->c1 = 0.0, wn->c2 = 0.0, wn->c3 = 0.0, wn->rfl = 0.0;
  wn->max = 1.0, wn->min = 0.0, wn->lft = 0.0, wn->rgt = 1.0;
}

void reflect(s)					/* reflect shape */
Shape *s;
{
  int i, j;

  for (i = 1, j = 2; j < s->np; i++, j+=2)
    Amp[i] = Amp[j];
  for (i = 0, j = s->np-1; i < s->np/2; i++, j--)
    Amp[j] = s->rfl*Amp[i];
}

void tmrev(a, np)					/* time reverse */
double *a[];
int     np;
{
  int i, j;
  double b;

  for (j = 0, i = np - 1; j < np / 2; j++, i--)  /* did not work with SWAP ! */
  {
    b = (*a)[j]; 
    (*a)[j] = (*a)[i]; 
    (*a)[i] = b;
  }
}

void swrev(srev, np)					/* reverse the sweep */
int    np;
double srev;
{
  int j, k;

  if (srev > 1.0) 
    srev = 1.0;
  else if (srev < 0.0)					/* find the maximum */
  {
    for (j = 0, k = 0, tm = 0.0; j < np; j++)
    {
      if (Amp[j] > tm) 
        k = j, tm = Amp[j]; 
    }
    j = k;
  }
  else
    j = np - (int) (srev * (double) np);
  for ( ; j < np; j++) 
    Pha[j] = -Pha[j];
}

int ampmod(s)
Shape *s;
{
  int i = 1, ee();

  if (strcmp(s->am, "sq") == 0) { sq(s); i=0; }	/* square */
  else if (strcmp(s->am, "sqa") == 0) sqa(s);	/* square wave AM */
  else if (strcmp(s->am, "gs") == 0)  gs(s);	/* gauss */
  else if (strcmp(s->am, "lz") == 0)  lz(s);	/* Lorentzian */
  else if (strcmp(s->am, "sch") == 0) sch(s);   /* sech */
  else if (strcmp(s->am, "hta") == 0) hta(s);   /* tanh */
  else if (strcmp(s->am, "sc") == 0)  sc(s);	/* sinc */
  else if (strcmp(s->am, "csp") == 0) csp(s);	/* cos powr */
  else if (strcmp(s->am, "wr") == 0)  wr(s);	/* wurst */
  else if (strcmp(s->am, "sed") == 0) sed(s);   /* seduce-1 */ 
  else if (strcmp(s->am, "qp") == 0)  qp(s);    /* quadrupolar */ 
  else if (strcmp(s->am, "ata") == 0) ata(s);   /* CA atan */ 
  else if (strcmp(s->am, "tra") == 0) tra(s);	/* triangle */
  else if (strcmp(s->am, "elp") == 0) elp(s);	/* ellipse */
  else if (strcmp(s->am, "exa") == 0) exa(s);	/* exp amp */
  else if (strcmp(s->am, "tna") == 0) tna(s);	/* tan amp */
  else if (strcmp(s->am, "fs") == 0)  fser(s);	/* fser */
  else if (strcmp(s->am, "ft") == 0)  ftr(s);	/* ft */
  else if (strcmp(s->am, "a1") == 0)  a1(s);	/* user #1 */
  else if (strcmp(s->am, "a2") == 0)  a2(s);	/* user #2 */
  else if (strcmp(s->am, "a3") == 0)  a3(s);	/* user #3 */
  else if (ee(s->am, s->fnm, s->np, &Amp) == 0)  /* call eqn eval-or */
    return -1;	        

  if (s->trev) tmrev(&Amp, s->np);		/* trev */
  if (fabs(s->rfl) > 0.01) reflect(s);          /* reflect */
    
  return i;
}

int dfil(wn)					/* Digital Filters */
Windw *wn;
{
  int i, j, k, m, zf;
  double  *re0, *im0, *re, *im, x;

  zf = 2;
  while (zf < wn->np) 			/* make np a power of 2 */
    zf *= 2;                    
  re0 = (double *) calloc(zf, sizeof(double));
  im0 = (double *) calloc(zf, sizeof(double));
  re = (double *) calloc(zf, sizeof(double));
  im = (double *) calloc(zf, sizeof(double));

  for (j = 0; j < zf; j++)
    re[j] = 0.0, im[j] = 0.0;	

  if(wn->dnp < 2)       /* default the number of filter coef-s */
    wn->dnp = (int) (25.0*wn->c1) + 1; 
  if ((strcmp(wn->am, "sq") == 0) && (wn->c1>0.1))  /* square filter */
    wn->dnp = (int) ((double) zf/wn->c1);
  if(wn->dnp > wn->np-1) 
    wn->dnp = wn->np-1;
  j = m = k = wn->dnp/2; 	/* force the # of coef be odd */
  wn->dnp = 2*k;
  wn->dnp++;  		

  if (strcmp(wn->am, "sq") == 0)
    sqf(&re, k);   				     /* square */
  else
  {
    if (strcmp(wn->am, "hnw") == 0) hnw(&re, k);  /* Hanning window */
    else if (strcmp(wn->am, "hmw") == 0) hmw(&re, k);  /* Hamming window */
    else if (strcmp(wn->am, "blw") == 0) blw(&re, k);  /* Blackman window */
    else if (strcmp(wn->am, "d1") == 0)  d1(&re, k);   /* user window #1  */
    else if (strcmp(wn->am, "d2") == 0)  d2(&re, k);   /* user window #2  */
    else if (strcmp(wn->am, "d3") == 0)  d3(&re, k);   /* user window #3  */
    else 
    {
      sprintf(e_str, "%s - Unrecognized dsp function...", wn->am);
      pxerr(e_str);
      return 0;
    }
    re[k] = 1.0; 
    x = wn->c1/M_PI; 
    for (i=0, j++, m--; i < k; i++, j++, m--)  
    {
      tm = (double) (i + 1)/x; 
      re[m] *= sin(tm)/tm; 
      re[j] = re[m];
    }
  }
  for (j = 0; j < wn->np; j++)			/* convert into re0 and im0 */
  {
    re0[j] = Amp[j] * cos(Pha[j]);
    im0[j] = Amp[j] * sin(Pha[j]);
  }

  shift(&re, &im, wn->dnp, zf); 
  reorder(&re, &im, zf); 
  if (strcmp(wn->am, "sq") != 0)
    fft(&re, &im, zf, -1.0);
  fft(&re0, &im0, zf, 1.0);			/* Filtering in freq domain */
  for (j = 0; j < zf; j++)	
  {
    re0[j] *= re[j];
    im0[j] *= re[j];
  }
  fft(&re0, &im0, zf, -1.0);

  for (j = 0; j < zf; j++)				/* find Amp and Pha */
  {
    re0[j] /= (double) wn->np;
    im0[j] /= (double) wn->np; 
    if ((re0[j] != 0.0) || (im0[j] != 0.0))
    {
      Amp[j] = sqrt(re0[j] * re0[j] + im0[j] * im0[j]);
      Pha[j] = atan2(im0[j], re0[j]);
    }
    else
      Amp[j] = re0[j], Pha[j] = im0[j];
  }
  return 1;
}

int wmult(s, wn)
Shape  *s;
Windw *wn;
{
  int j;
  double mx=0.0;

  for (j = 0; j < s->np; j++)
    Pha[j] = Amp[j];
  resetshape(&swn);
  swn.np = s->np; 
  strcpy(swn.sh, wn->sh), strcpy(swn.am, wn->am);
  swn.c1 = wn->c1,   swn.c3 = wn->c2,   swn.c3 = wn->c3;
  swn.dnp = wn->dnp, swn.min = wn->min, swn.max = wn->max;
  swn.lft = 1.0 - 2.0*wn->lft; swn.rgt = wn->rgt*(1.0 + swn.lft);
  swn.rfl = wn->rfl;
  if(ampmod(&swn) < 0) return 0;
  for (j = 0; j < s->np; j++)
    Amp[j] *= Pha[j];
  imx=0;		/* reset this parameter for asymmetric shapes */

  return 1;
}

void setsw(s)
Shape *s;
{
int    j = 0;
double b, c;

  if ((fabs(s->pwsw) < 0.01) && (fabs(s->pwb) > 0.001))
  {
    if (strcmp(s->fm, "ls") == 0)       		  /* chirp */
    {
      if (s->adb > 1.01) 
        c = sqrt(1.0 / s->adb);
      else 
        c = sqrt(0.8);	
      while (c > fabs(Amp[j])) j++;
      s->pwsw = 0.5 * s->pwb / (0.5 - (double) j / s->np); 
    }
    else if (strcmp(s->fm, "tns") == 0) 		 /* tan sweep */
    {
      if (s->adb > 1.251) c = acos(sqrt(1.25 / s->adb));
      else c = acos(sqrt(0.8)); 
      s->pwsw = s->pwb * tan(0.5 * M_PI * s->c1) / tan(c); 
    }
    else if (strcmp(s->fm, "cas") == 0)  		/* CAP */ 
    {
      b = 0.0; c = 2.0;
      while (b < s->pwb)
      {
        c += 1.0;
        b = cbrt(c);
        b = 2.0 * c * sqrt(b * b - 1.0) / M_PI;
      }
      s->adb = c;
    }
    else s->pwsw = s->pwb;
  }
}

int frqmod(s)
Shape *s;
{
  int ee();
 
  setsw(s);
  s->adb /= pi2; 

  if (s->ffl < 0)				/* PM functions */
  {
    if (ee(s->fm, s->fnm, s->np, &Pha) == 0) /* call eqn eval */
    return 0;
  }
  else
  {
    if (strcmp(s->fm, "ls") == 0)       ls(s);  /* chirp */
    else if (strcmp(s->fm, "tns") == 0) tns(s); /* tan sweep */
    else if (strcmp(s->fm, "ht") == 0)  ht(s);  /* tanh */
    else if (strcmp(s->fm, "ats") == 0) ats(s); /* atan sweep */
    else if (strcmp(s->fm, "lzs") == 0) lzs(s); /* CA Lorentzian */
    else if (strcmp(s->fm, "ca") == 0)  ca(s);  /* CA sweep */
    else if (strcmp(s->fm, "osm") == 0) osm(s); /* OSM sweep */
    else if (strcmp(s->fm, "cas") == 0) cas(s); /* CAP */ 
    else if (strcmp(s->fm, "cs") == 0)  cs(s);  /* cos sweep*/
    else if (strcmp(s->fm, "cs2") == 0) cs2(s); /* CA cos^2 sweep*/
    else if (strcmp(s->fm, "ccs") == 0) ccs(s); /* CA cos sweep*/
    else if (strcmp(s->fm, "fsw") == 0) fsw(s); /* freq switch */
    else if (strcmp(s->fm, "fslg") == 0) fslg(s); /* freq switch FSLG */
    else if (strcmp(s->fm, "sqw") == 0) sqw(s);   /* sq wave */
    else if (ee(s->fm, s->fnm, ++s->np, &Pha) != 0) /* call eqn eval */
      fm2pm(s);                                 /* convert FM to PM */
    else
      return 0;          
  }
  if (s->trev) tmrev(&Pha, s->np);		/* trev */
         
  return 1;
}

void stretch(s, nn)
Shape *s;
int   nn;
{
  int i, j, k, nn2;
  double pwsw, ph0, phincr, ko;
  
  if (Sh->adb < 0.001)
  {
    if (strcmp(Sh->fm,"n") != 0)
      pxout("\nWarning ! Adiabaticity not specified, assuming adb = 4\n", 1);
    Sh->adb = 4.0;
  }
  pwsw = Sh->pb1 * Sh->pb1 * Sh->stch * Sh->stch / Sh->adb;
  ko = 0.125 * pi2 * pwsw;

  ph0 = ko; 
  nn2 = Sh->np / 2;
  phincr = M_PI * pwsw / ((double) Sh->np * Sh->stch);

  for (j = 0, i = nn2 - 1; j < nn2; j++, i--)   /* first half */
    Pha[i] += (double) j * phincr + phincr + ph0;

  ph0 += (phincr * (Sh->np - nn2));              /* second half */
  for (i = 0, j = Sh->np - 1, k = nn + Sh->np - 1; i < Sh->np - nn2; i++, j--, k--) 
  {
    Pha[k] = Pha[j] - (double) i * phincr + ph0;
    Amp[k] = Amp[j];
  }

  for (j = 0, k = nn2; j < nn; j++, k++)	/* middle */
  {
    tm = 1.0 - 2.0 * (double) j / (nn - 1);
    Pha[k] = ko * tm * tm; 
    Amp[k] = 1.0;
  }
}

int wrap(wrf, nn)
int    nn;
double wrf;
{
  int i, j, nn1;
  double *tmpa, *tmph;

  wrf -= floor(wrf);        /* make sure wrf < 1.0 */
  tm = (double) nn * wrf;
  nn1 = (int) (tm+0.5);  
  if ((nn1 >= nn) || (nn1 <= 0)) 
    return 0;

  tmpa = arry(nn1);
  tmph = arry(nn1);
  for(i = 0, j = nn - nn1; i < nn1; i++, j++)
  {
    tmpa[i] = Amp[j]; 
    tmph[i] = Pha[j];
  }
  for(i = nn - 1, j = nn - nn1 - 1; j > -1; i--, j--)
  {
    Amp[i] = Amp[j]; 
    Pha[i] = Pha[j];
  }
  for(i = 0; i < nn1; i++)
  {
    Amp[i] = tmpa[i]; 
    Pha[i] = tmph[i];
  }
  free(tmpa);
  free(tmph);

  return 1;
}


int getsu(s, sam, sph)					/* get supercycle */
Shape *s;
double *sam[], *sph[];
{
  int i, j;

    if (s->np < 1) s->np = s->dnp;
    if (s->np < 1) 
    { 
      sprintf(e_str, "Undefined number of steps in %s", s->sh);
      pxerr(e_str);
      return 0;
    }
    if (strcmp(s->fm, "ca") == 0) s->np += 2; 

    for (i = 0; i < s->np; i++)		  	  /* initialize */
      (*sam)[i] = 1.0, (*sph)[i] = 0.0;

    if (s->afl > 0) 					/* AM */
    {
      Amp = arry(s->np);
      j = ampmod(s);
      if (j<0) return 0;
      imod += j;  
      for (i = 0; i < s->np; i++)			/* copy */
        (*sam)[i] = Amp[i];
      free(Amp);
    }

    if (s->ffl) 			 		/* FM & PM */
    {
      Pha = arry(s->np);
      if(frqmod(s) == 0) return 0; 
      for (i = 0; i < s->np; i++)			/* copy */
          (*sph)[i] = Pha[i]; 
      free(Pha);
    } 
  return 1; 
}

void reportshape(s)
Shape *s;
{
  int i, j, k;
    printf("\n                  Shape Parameters\n");
    printf("  fla = %7.2f,  pwb1 = %5.1f,   amf = %s\n", s->fla, s->pb1, s->am);
    if (s->ffl < 0)
      printf("  pwbw = %5.1f,   pwsw = %5.1f,   pmf = %s\n", s->pwb, s->pwsw, s->fm);
    else
      printf("  pwbw = %5.1f,   pwsw = %5.1f,   fmf = %s\n", s->pwb, s->pwsw, s->fm);
    printf("  adb = %6.1f,   ofs = %6.1f,   su =  %s\n", s->adb, s->of, s->su);
    printf("  dres = %5.1f,   steps = %4d,   dash = %s\n", s->dres, s->dnp, s->dash);
    printf("  srev = %5.1f,   st = %6.1f\n", s->srev, s->st);
    printf("  c1 = %8.2f,  c2 = %8.2f,  c3 = %8.2f\n", s->c1, s->c2, s->c3);
    printf("  d1 = %8.6f,  d2 = %8.6f,  dutyc = %5.2f\n", s->d1, s->d2, s->duty);

    printf("\n                  Truncation Parameters \n");
    printf("       min =   %6.3f,       max =   %6.3f\n", s->min, s->max);
    printf("       left =  %6.3f,       right = %6.3f\n", s->lft, s->rgt);
    printf("       cmplx = %2d,           wrap =  %6.3f\n", s->co, s->wr);
    printf("       trev = %3d,           stretch = %6.3f\n", s->trev, s->stch);
    printf("       dcflag = %d,           reflect = %.2f\n", s->dc, s->rfl);
  if ((k = s->n * s->m) > 0)
  {
    printf("\n              Additional Data Matrix (%d * %d)\n", s->n, s->m);
    k=0;     
    for(i = 0; i< s->m; i++) {
      for(j = 0; j< s->n; j++) 	{
	printf("      %8.4f", s->f[k]);
	k++;			}
      printf("\n");
        		     }
  }
}

void reportwindow(wn)
Windw *wn;
{
  int i, j, k;
    printf("  name = %s\n", wn->sh);
    printf("  steps = %4d,   wf = %s\n", wn->dnp, wn->am);
    printf("  c1 = %8.2f,  c2 = %8.2f,  c3 = %8.2f\n", wn->c1, wn->c2, wn->c3);
    printf("       min =   %6.3f,       max =   %6.3f\n", wn->min, wn->max);
    printf("       left =  %6.3f,       right = %6.3f\n", wn->lft, wn->rgt);
    printf("  reflect = %8.2f\n", wn->rfl);
  if ((k = wn->n * wn->m) > 0)
  {
    printf("\n              Additional Data Matrix (%d * %d)\n", wn->n, wn->m);
    k=0;     
    for(i = 0; i< wn->m; i++) {
      for(j = 0; j< wn->n; j++) 	{
	printf("      %8.4f", wn->f[k]);
	k++;			}
      printf("\n");
        		     }
  }
}

FILE *findwave(lib, dir, sh, shf)
char  *lib, *dir, *sh, *shf;
{
  FILE *fil;
  int  i = 0, j;
  char str[MAXSTR], shape[MAXSTR];	/* open wave file */

  j = fixdir(dir);
  (void) strcat(strcpy(shape, dir), sh);
  (void) strcat(strcpy(str, lib), shape);
  while (((fil = fopen(str, "r")) == NULL) && (i < wv.ndir))
  {
    (void) strcat(strcat(strcpy(str, lib), wv.dir[i]), shape);
    i++; 
  }   
  if (fil != NULL)
  {
    i--;
    strcpy(shf, str); 
    if (j == 0) sprintf(dir, "%s", wv.dir[i]);
    else if (i > 0) strcpy(dir, strcat(strcpy(str, dir), wv.dir[i]));
  }  
  return fil;
}


int printcoms(dir, sh, opt)
char  *dir, *sh, opt;
{
  FILE *fil;
  char shape[MAXSTR], str[MAXSTR];


  if (((fil = findwave(waves, dir, sh, e_str)) == NULL) &&
      ((fil = findwave(SYSWAVELIB, dir, sh, e_str)) == NULL)) 
  {
    err(sh); return 0;
  }

  fgets(shape, MAXSTR, fil);
  sscanf(shape, "%s", str);
  if (opt == 't')
  {
    if (str[0] == '#')
      sprintf(dir, "%s", shape);
  }
  else
  {
    while (str[0] == '#')
    {
      printf("%s", shape);
      fgets(shape, MAXSTR, fil);
      sscanf(shape, "%s", str);
    }
    if (opt == 'i')
    {
      printf("%s", shape);
      while (fgets(shape, MAXSTR, fil))
        printf("%s", shape);
    }
  }
  fclose(fil);
  return 1;
}


int getshape(dir, sh, s, reps)
char  *dir, *sh;
Shape *s;
int  reps;
{
  FILE *fil;
  char str[MAXSTR], str1[MAXSTR];
  int i, j, k;

  if ((sh[0]=='a' || sh[0]=='A' || sh[0]=='r') && (sh[1]=='-'))  /* internal phase cycles */
  {
    strcpy(str1, sh); 
    cutstr2(str,str1,'-'); 
    strcpy(s->fm, "sqw");
    s->ffl = 1;
    s->n = 1;
    s->m = 2;
    tm=stod(str1);
    if(str[0]=='a')
    {
      s->f = arry(s->m); 
      s->f[0]=tm;  
      s->f[1]=-tm; 
    }
    else if(str[0]=='A')
    {
      s->f = arry(s->m); 
      s->f[0]=0.0;  
      s->f[1]=tm; 
    }
    else
    {
      s->m = stoi(str1); 
      s->f = arry(s->m);    
      for(i = 0; i< s->m; i++)
	  s->f[i]=0.0;
    }
    s->dnp = s->m;  
    return s->sqw = 1;
  }

  if (((fil = findwave(waves, dir, sh, s->fnm)) == NULL) &&
      ((fil = findwave(SYSWAVELIB, dir, sh, s->fnm)) == NULL))
  {
    err(sh); return 0;
  }
  fscanf(fil, "%s", str);
  while (str[0] == '#')
  {
    fgets(str, MAXSTR, fil);
    fscanf(fil, "%s", str);
  }
		/* Shape parameters */  
  strcpy(s->sh, sh); 
  if (findeqn(fil, "amf", str)) strcpy(s->am, str);		/* AM funct */
  if ((s->am[0] == 'n' && s->am[1] == '\0') || 
     (s->am[0] == '\0')) strcpy(s->am, "sq");
  if (s->am[0] == 's' && s->am[1] == 'q' && s->am[2] == '\0')
  s->afl = 0; 
  else s->afl = 1;
  if (findeqn(fil, "fmf", str)) strcpy(s->fm, str);		/* FM funct */
  if ((s->fm[0] == 'n' && s->fm[1] == '\0') || 
     (s->fm[0] == '\0')) s->ffl = 0;
  else s->ffl = 1;
  if (findeqn(fil, "pmf", str))                                 /* PM funct */
  {
    strcpy(s->fm, str);		
    if ((s->fm[0] == 'n' && s->fm[1] == '\0') || 
       (s->fm[0] == '\0')) s->ffl = 0;
    else s->ffl = -1;
  }
  if (findpar(fil, "su", str) > 0) strcpy(s->su, str);		/* supercycle */
  if (findpar(fil, "wf", str) > 0) strcpy(s->wf, str);		/* window fun */
  if (findpar(fil, "dsp", str) > 0) strcpy(s->df, str);		/* dig filter */
  if (findpar(fil, "fla", str) > 0) strcpy(s->fla_, str);	/* flipangle  */
  {
    s->oflg = ofres(s->fla_);
    s->fla = stod(s->fla_); 
  }
  if (findpar(fil, "pwbw", str) > 0) s->pwb = stod(strcpy(s->pwb_, str));
  if (findpar(fil, "pwb1", str) > 0) s->pb1 = stod(strcpy(s->pb1_, str));
  if (findpar(fil, "pwsw", str) > 0) s->pwsw = stod(strcpy(s->pwsw_, str));
  if (findpar(fil, "adb", str) > 0) s->adb = stod(strcpy(s->adb_, str));
  if (findpar(fil, "ofs", str) > 0) s->of = stod(strcpy(s->of_, str));
  if (findpar(fil, "dres", str) > 0) s->dres = stod(strcpy(s->dres_, str)); 
  if (findpar(fil, "st", str) > 0) s->st = stod(strcpy(s->st_, str));
  if (findpar(fil, "dash", str) > 0) strcpy(s->dash, str);
  if (findpar(fil, "c1", str) > 0) s->c1 = stod(strcpy(s->c1_, str));
  if (findpar(fil, "c2", str) > 0) s->c2 = stod(strcpy(s->c2_, str));
  if (findpar(fil, "c3", str) > 0) s->c3 = stod(strcpy(s->c3_, str));
  if (findpar(fil, "d1", str) > 0) s->d1 = stod(strcpy(s->d1_, str));
  if (findpar(fil, "d2", str) > 0) s->d2 = stod(strcpy(s->d2_, str));
  if (findpar(fil, "dutyc", str) > 0) 
  {
    s->duty = stod(strcpy(s->duty_, str));
    if ((s->duty > 0.0001) && (s->duty < 1.0))
      s->d1 = s->d2 = (0.5 / s->duty) - 0.5;
    else 
      s->duty = 1.0;
  }
  if (findpar(fil, "steps", str) > 0) s->dnp = stoi(strcpy(s->dnp_, str));

		/* Truncation Parameters */
  if (findpar(fil, "min", str) > 0) s->min = stod(strcpy(s->min_, str));
  if (findpar(fil, "max", str) > 0) s->max = stod(strcpy(s->max_, str));
  if (findpar(fil, "left", str) > 0) s->lft = stod(strcpy(s->lft_, str));
  if (findpar(fil, "right", str) > 0) s->rgt = stod(strcpy(s->rgt_, str));
  if (findpar(fil, "cmplx", str) > 0) s->co = stoi(strcpy(s->co_, str));
  if (findpar(fil, "wrap", str) > 0) s->wr = stod(strcpy(s->wr_, str));
  if (findpar(fil, "stretch", str) > 0) s->stch = stod(strcpy(s->stch_, str));
  if (findpar(fil, "trev", str) > 0) s->trev = stoi(strcpy(s->trev_, str));
  if (findpar(fil, "srev", str) > 0) s->srev = stod(strcpy(s->srev_, str));
  if (findpar(fil, "reflect", str) > 0) s->rfl = stod(strcpy(s->rfl_, str));
  if ((findpar(fil, "dcflag", str) > 0) && (str[0] == 'y')) 
    { strcpy(s->dc_, "y"); s->dc = 1; }
		/* Additional Data Matrix */
  if (findpar(fil, "cols", str) > 0) s->n = stoi(str);
  if (findpar(fil, "rows", str) > 0) s->m = stoi(str);
  if ((k = s->n * s->m) > 0)
  {
    fgets(str, MAXSTR, fil);
    skipcomms(fil);
    s->f = arry(k); k=0;     
    for(i = 0; i< s->m; i++) {
      for(j = 0; j< s->n; j++) 	{
	fscanf(fil, "%lf", &s->f[k]); 
	k++;			}
        		     }
  }
  if ((strcmp(s->fm, "sqw") == 0) || (strcmp(s->fm, "fsw") == 0))
    s->sqw = 1;
  return 1;
}


int getwindow(dir, sh, wn, reps)
char  *dir, *sh;
Windw *wn;
int  reps;
{
  FILE *fil;
  char shape[MAXSTR], str[MAXSTR];
  int i, j, k;

  if (((fil = findwave(waves, dir, sh, wn->fnm)) == NULL) &&
      ((fil = findwave(SYSWAVELIB, dir, sh, wn->fnm)) == NULL))
  {
    err(sh); return 0;
  }

  fscanf(fil, "%s", str);
  while (str[0] == '#')
  {
    fgets(str, MAXSTR, fil);
    fscanf(fil, "%s", str);
  }
		/* Window parameters */

  strcpy(wn->sh, sh);
  if (findeqn(fil, "amf", str) > 0) strcpy(wn->am, str);		/* AM funct */
  if ((wn->am[0] == 'n' && wn->am[1] == '\0') || 
     (wn->am[0] == '\0')) strcpy(wn->am, "sq");

  if (findpar(fil, "c1", str) > 0) wn->c1 = stod(str);
  if (findpar(fil, "c2", str) > 0) wn->c2 = stod(str);
  if (findpar(fil, "c3", str) > 0) wn->c3 = stod(str);
  if (findpar(fil, "steps", str) > 0) wn->dnp = stoi(str);

		/* Truncation Parameters */
  if (findpar(fil, "min", str) > 0) wn->min = stod(str);
  if (findpar(fil, "max", str) > 0) wn->max = stod(str);
  if (findpar(fil, "left", str) > 0) wn->lft = stod(str);
  if (findpar(fil, "right", str) > 0) wn->rgt = stod(str);
  if (findpar(fil, "reflect", str) > 0) wn->rfl = stod(str);

		/* Additional Data Matrix */
  if (findpar(fil, "cols", str) > 0) wn->n = stoi(str);
  if (findpar(fil, "rows", str) > 0) wn->m = stoi(str);
  if ((k = wn->n * wn->m) > 0)
  {
    fgets(str, MAXSTR, fil);
    skipcomms(fil);
    wn->f = arry(k); k=0;     
    for(i = 0; i< wn->m; i++) {
      for(j = 0; j< wn->n; j++) 	{
	fscanf(fil, "%lf", &wn->f[k]);
	k++;			}
        		     }
  }
  return 1;
}


int findparm(fnm, str, val, ip)
FILE *fnm;
char *val, *str;
int  ip;
{
  char chr[MAXSTR];
  
  chr[0] = '\0';
  fseek(fnm, ip, 0); 
  while ((fscanf(fnm, "%s", chr) != EOF) && (chr[0] != '$'))
  {
    if ((str[0] == chr[0]) && (strcmp(chr,str) == 0))
    {
      fscanf(fnm, "%s", chr);
      if (chr[0] == '=') 
      {
        fscanf(fnm, "%s", val); 
        return 1;
      }
    }
  }

  return (0);
}


void modpars(fil, s, ip)
FILE *fil;
Shape *s;
int  ip;
{
  char str[MAXSTR];
  int i, j, k=0;

		/* Modify shape parameters */

  if (findeqnm(fil, "amf", str, ip)) 
  { 
    strcpy(s->am, str);	                                    /* AM funct */
    sprintf(s->modpar, "%s#    amf = %s\n", s->modpar, str);
  }
  if (s->am[0] == 'n' && s->am[1] == '\0') strcpy(s->am, "sq");
  if (s->am[0] == 's' && s->am[1] == 'q' && s->am[2] == '\0')
    s->afl = 0; 
  else s->afl = 1;
  if (findeqnm(fil, "fmf", str, ip)) 
  {
    strcpy(s->fm, str);                                     /* FM funct */
    sprintf(s->modpar, "%s#    fmf = %s\n", s->modpar, str);
  }
  if (s->fm[0] == 'n' && s->fm[1] == '\0') s->ffl = 0;
  else s->ffl = 1;
  if (findeqnm(fil, "pmf", str, ip)) 	                    /* PM funct */
  {
    strcpy(s->fm, str);
    sprintf(s->modpar, "%s#    pmf = %s\n", s->modpar, str);
    if (s->fm[0] == 'n' && s->fm[1] == '\0') s->ffl = 0;
    else s->ffl = -1;
  }
  if (findparm(fil, "su", str, ip)) 
  {
    strcpy(s->su, str);	                                  /* supercycle */
    sprintf(s->modpar, "%s#    su = %s\n", s->modpar, str);
  }
  if (findparm(fil, "fla", str, ip))                      /* flipangle */
  {
    sprintf(s->modpar, "%s#    fla = %s\n", s->modpar, str);
    s->oflg = ofres(str);
    s->fla = stod(str); 
  }
  if (findparm(fil, "pwbw", str, ip)) 
  {
    s->pwb = stod(str);
    sprintf(s->modpar, "%s#    pwbw = %s\n", s->modpar, str);    
  }
  if (findparm(fil, "pwb1", str, ip)) 
  {
    s->pb1 = stod(str);
    sprintf(s->modpar, "%s#    pb1 = %s\n", s->modpar, str);
  }    
  if (findparm(fil, "pwsw", str, ip)) 
  {
    s->pwsw = stod(str);
    sprintf(s->modpar, "%s#    pwsw = %s\n", s->modpar, str);    
  }
  if (findparm(fil, "adb", str, ip)) 
  {
    s->adb = stod(str);
    sprintf(s->modpar, "%s#    adb = %s\n", s->modpar, str);
  }
  if (findparm(fil, "ofs", str, ip)) 
  {
    s->of = stod(str);
    sprintf(s->modpar, "%s#    ofs = %s\n", s->modpar, str);
  }
  if (findparm(fil, "dres", str, ip)) 
  {
    s->dres = stod(str);
    sprintf(s->modpar, "%s#    dres = %s\n", s->modpar, str);
  }
  if (findparm(fil, "st", str, ip)) 
  {
    s->st = stod(str);
    sprintf(s->modpar, "%s#    st = %s\n", s->modpar, str);
  }
  if (findparm(fil, "dash", str, ip)) 
  {
    strcpy(s->dash, str);
    sprintf(s->modpar, "%s#    dash = %s\n", s->modpar, str);
  }
  if (findparm(fil, "c1", str, ip)) 
  {
    s->c1 = stod(str);
    sprintf(s->modpar, "%s#    c1 = %s\n", s->modpar, str);
  }
  if (findparm(fil, "c2", str, ip)) 
  {
    s->c2 = stod(str);
    sprintf(s->modpar, "%s#    c2 = %s\n", s->modpar, str);
  }
  if (findparm(fil, "c3", str, ip)) 
  {
    s->c3 = stod(str);
    sprintf(s->modpar, "%s#    c3 = %s\n", s->modpar, str);
  }
  if (findparm(fil, "d1", str, ip)) 
  {
    s->d1 = stod(str);
    sprintf(s->modpar, "%s#    d1 = %s\n", s->modpar, str);
  }
  if (findparm(fil, "d2", str, ip)) 
  {
    s->d2 = stod(str);
    sprintf(s->modpar, "%s#    d2 = %s\n", s->modpar, str);
  }
  if (findparm(fil, "dutyc", str, ip)) 
  {
    s->duty = stod(str);
    sprintf(s->modpar, "%s#    dutyc = %s\n", s->modpar, str);
    if ((s->duty > 0.0001) && (s->duty < 1.0))
      s->d1 = s->d2 = (0.5 / s->duty) - 0.5;
    else 
      s->duty = 1.0;
  }
  if (findparm(fil, "steps", str, ip)) 
  {
    s->dnp = stoi(str); 
    sprintf(s->modpar, "%s#    steps = %s\n", s->modpar, str);
  }

		/* Truncation Parameters */
  if (findparm(fil, "min", str, ip)) 
  {
    s->min = stod(str); 
    sprintf(s->modpar, "%s#    min = %s\n", s->modpar, str);
  }
  if (findparm(fil, "max", str, ip)) 
  {
    s->max = stod(str); 
    sprintf(s->modpar, "%s#    max = %s\n", s->modpar, str);
  }
  if (findparm(fil, "left", str, ip)) 
  {
    s->lft = stod(str);
    sprintf(s->modpar, "%s#    left = %s\n", s->modpar, str);
  }
  if (findparm(fil, "right", str, ip)) 
  {
    s->rgt = stod(str);
    sprintf(s->modpar, "%s#    right = %s\n", s->modpar, str);
  }
  if (findparm(fil, "cmplx", str, ip)) 
  {
    s->co = stoi(str);
    sprintf(s->modpar, "%s#    cmplx = %s\n", s->modpar, str);
  }
  if (findparm(fil, "wrap", str, ip)) 
  {
    s->wr = stod(str);
    sprintf(s->modpar, "%s#    wrap = %s\n", s->modpar, str);
  }
  if (findparm(fil, "stretch", str, ip)) 
  {
    s->stch = stod(str);
    sprintf(s->modpar, "%s#    stretch = %s\n", s->modpar, str);
  }
  if (findparm(fil, "trev", str, ip)) 
  {
    s->trev = stoi(str);
    sprintf(s->modpar, "%s#    trev = %s\n", s->modpar, str);
  }
  if (findparm(fil, "srev", str, ip)) 
  {
    s->srev = stod(str);
    sprintf(s->modpar, "%s#    srev = %s\n", s->modpar, str);
  }
  if (findparm(fil, "reflect", str, ip)) 
  {
    s->rfl = stod(str);
    sprintf(s->modpar, "%s#    reflect = %s\n", s->modpar, str);
  }
  if ((findparm(fil, "dcflag", str, ip)) && (str[0] == 'y')) 
  {
    s->dc = 1;
    sprintf(s->modpar, "%s#    dcflag = y\n", s->modpar);
  }
		/* Additional Data Matrix */
  if (i=findparm(fil, "cols", str, ip)) 
  {
    s->n = stoi(str);
    sprintf(s->modpar, "%s#    cols = %s\n", s->modpar, str);
  }    
  if (j=findparm(fil, "rows", str, ip)) 
  {
    s->m = stoi(str);
    sprintf(s->modpar, "%s#    rows = %s\n", s->modpar, str);
  }
  if ((i) || (j))
  {
    fgets(str, MAXSTR, fil);
    s->f = arry(k); k=0;     
    for(i = 0; i< s->m; i++) 
    {
      sprintf(s->modpar, "%s#      ", s->modpar);
      for(j = 0; j< s->n; j++) 	
      {
	fscanf(fil, "%lf", &s->f[k]);
	sprintf(s->modpar, "%s %12.6f  ", s->modpar, s->f[k]);
	k++;			
      }
      sprintf(s->modpar, "%s \n", s->modpar);
    }
  }
  if ((strcmp(s->fm, "sqw") == 0) || (strcmp(s->fm, "fsw") == 0))
     s->sqw = 1;

  return;
}


void setdash(s, dash)
Shape *s;
double dash;
{
  if (strcmp(s->dash, "fla") == 0) s->fla = dash;
  else if (strcmp(s->dash, "pwbw") == 0) s->pwb = dash;
  else if (strcmp(s->dash, "pwb1") == 0) s->pb1 = dash;
  else if (strcmp(s->dash, "pwsw") == 0) s->pwsw = dash;
  else if (strcmp(s->dash, "adb") == 0) s->adb = dash;
  else if (strcmp(s->dash, "ofs") == 0) s->of = dash;
  else if (strcmp(s->dash, "dres") == 0) s->dres = dash;
  else if (strcmp(s->dash, "c1") == 0) s->c1 = dash;
  else if (strcmp(s->dash, "c2") == 0) s->c2 = dash;
  else if (strcmp(s->dash, "c3") == 0) s->c3 = dash;
  else if (strcmp(s->dash, "dutyc") == 0) s->duty = dash;
  else if (strcmp(s->dash, "min") == 0) s->min = dash;
  else if (strcmp(s->dash, "max") == 0) s->max = dash;
  else if (strcmp(s->dash, "left") == 0) s->lft = dash;
  else if (strcmp(s->dash, "right") == 0) s->rgt = dash;
  else if (strcmp(s->dash, "wrap") == 0) s->wr = dash;
  else if (strcmp(s->dash, "trev") == 0) s->trev = (int) dash;
  else if (strcmp(s->dash, "stretch") == 0) s->stch = dash;
}

/* Stype - shaped pulse type

  Pulse Types :
  ============
  1 - Constant Amplitude & Constant Phase	square pulse
  2 - Constant Amplitude & Phase Alternated	e.g. WALTZ-16
  3 - Amplitude Modulated & Constant Phase	e.g. Gaussian
  4 - Amplitude Modulated & Phase Alternated	e.g. BURPs
  5 - Constant Amplitude & Phase Modulated	e.g. chirp
  6 - Amplitude Modulated & Phase Modulated	e.g. WURST
*/

void set_Stype()
{
  int     i=0, j=0, ics=0, isn=0;
  double  minph=rd*g.phres; 

  Cs = arry(h.np);
  Sn = arry(h.np);

  for (j = 0; j < h.np; j++)
  {
    tm = rd*Pha[j];
    Cs[j] = cos(tm);
    Sn[j] = sin(tm);
    if (fabs(Amp[j]) > 0.0)			/* skip if Amp = 0 */
    {
      if (minph > fabs(Sn[j])) isn++;		/* 0 or 180 */
      else if (minph > fabs(Cs[j])) ics++; 	/* 90 or 270 */
    }
    else isn++, ics++;
  }
  ics -= h.np, isn -= h.np; 
  
  i=1;
  while((Amp[i] == Amp[i-1]) && (i < h.np)) i++;
  j=1;
  while((Pha[j] == Pha[j-1]) && (j < h.np)) j++;

  if ((i == h.np) && (j == h.np)) 
    Stype = 1;                        /* no AM, no PM */
  else if (j == h.np) 
  {
    Stype = 3;                        /* AM (no PM) */
  }
  else
  {
    if (!isn) 			/* Phase alternated, 0 & 180 */
    {
      if (i == h.np)  		
        Stype = 2;              /* no AM */
      else
        Stype = 4; 		/* AM */
    }
    else if (!ics) 		/* Phase alternated, 90 & 270 */ 
    {
      if (i == h.np)  		
        Stype = 2;		/* no AM */
      else
        Stype = 4; 		/* AM */
    }
    else if (i == h.np) 
      Stype=5; 			/* Phase Modulated (no AM) */
    else Stype=6; 		/* PM + AM */
  } 
}
