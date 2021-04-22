/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* Pbox_bio.h  - Pbox include file for BioPack, Requires Vnmr 6.1C or later  
                 Eriks Kupce, 27.09.2002  
                 modified by Eriks Kupce, 21 Jan 2003. 
*/


#ifndef FIRST_FID
#include <Pbox_psg.h>
#endif


shape pbox_sh()
{
    shape  tmpsh;

     pbox_get();
     strcpy(tmpsh.name, pbox_name);
     tmpsh.pwr = pbox_pwr;
     tmpsh.pw = pbox_pw;
     tmpsh.pwrf = pbox_pwrf;
     tmpsh.dmf = pbox_dmf;
     tmpsh.dres = pbox_dres;

     return tmpsh;
}


shape pbox(shn, wvstr, parstr, xfrq, rfpw, rfpwr)
char *shn, *wvstr, *parstr;
double xfrq, rfpw, rfpwr;
{
   opx(shn);
   setwave(wvstr);
   pboxpar("sfrq", xfrq);
   pbox_par("2", parstr);
   cpx(rfpw, rfpwr);
   
   return  pbox_sh();
}

shape pbox_dec(shn, wvstr, dec_pwr, xfrq, rfpw, rfpwr)
char *shn, *wvstr;
double dec_pwr, xfrq, rfpw, rfpwr;
{
   char pwrlvl[MAXSTR];
  
   sprintf(pwrlvl, "%.0fd", dec_pwr);
   
   opx(shn);
   setwave(wvstr);
   pboxpar("sfrq", xfrq);
   pbox_par("attn", pwrlvl);
   cpx(rfpw, rfpwr);
   
   return  pbox_sh();
}

static char    autocal[MAXSTR], checkofs[MAXSTR], nwaves[MAXSTR];
static double  tofH, tofC, tofN, tofD;
static int     tof_is_set;

void setautocal()         /* if autocal flag exists, use it,   */
{                         /* otherwise, set it to 'yes'        */
  int setofs();          /* do the same for the checkofs flag */

  if(FIRST_FID)
  {
    if (find("autocal")>0)
      getstr("autocal",autocal);
    else
      autocal[0] = 'y';
      
    if (find("checkofs")>0)
      getstr("checkofs",checkofs);      /* is it used ??? */
    else
      checkofs[0] = 'y';

    tof_is_set = setofs();
  }
}

/* make a shape using pre-set parameters: maxincr = 10 and attn = E */

shape pbox_make(shn, wvn, bw, ofs, rf_pw90, rf_pwr)
  
char   *shn, *wvn;
double bw, ofs, rf_pwr, rf_pw90;
{
  double reps = 0.0;
  shape  sh;


  if (autocal[0] == 'r')                         /* read from file */
    sh = getRsh(shn);
  else 
  {
    if(autocal[0] == 'y')                      /* show Pbox output */ 
      reps = 2.0;    
        
    opx("Pbox");                                      /* open Pbox */
    pboxpar("maxincr", 10.0);  /* set max phase incr to 10 degrees */ 
    pboxpar("reps", reps);         /* used to control Pbox output  */
    pboxUpar("attn", rf_pwr, "E"); /* set course attenuation level */ 
    sh = pbox_shape(shn, wvn, bw, ofs, rf_pw90, rf_pwr);
  }
  
  return sh;
}

/* make adiabatic waveform using pre-set parameters: maxincr = 10 
   and attn = E */

shape pbox_makeA(shn, wvn, bw, pws, ofs, rf_pw90, rf_pwr, nst)
char   *shn, *wvn;
double bw, pws, ofs, rf_pw90, rf_pwr, nst;
{
  double reps = 0.0;
  shape  sh;


  if (autocal[0] == 'r')                         /* read from file */
    sh = getRsh(shn);
  else 
  {
    if(autocal[0] == 'y')                      /* show Pbox output */ 
      reps = 2.0;    
        
    opx("Pbox");                                      /* open Pbox */
    pboxpar("reps", reps);         /* used to control Pbox output  */
    if (nst > 0.5)
      pboxpar("steps", nst);
    pboxUpar("attn", rf_pwr, "E"); /* set course attenuation level */ 
    sh = pboxAshape(shn, wvn, bw, pws, ofs, rf_pw90, rf_pwr);
  }
  
  return sh;
}


/* make a time-reversed shape using pre-set parameters: maxincr = 10 
   and attn = E */

shape pbox_makeR(shn, wvn, bw, ofs, rf_pw90, rf_pwr)
  
char   *shn, *wvn;
double bw, ofs, rf_pwr, rf_pw90;
{
  char   wvnbw[MAXSTR];                 /* put the wvn and bw here */
  double reps = 0.0, st = 1.0;
  shape  sh;


  if (autocal[0] == 'r')                         /* read from file */
    sh = getRsh(shn);
  else 
  {
    if(autocal[0] == 'y')                      /* show Pbox output */ 
      reps = 2.0;   
       
    sprintf(wvnbw, "%s %.7f", wvn, bw);
        
    opx("Pbox");                                      /* open Pbox */
    pboxpar("maxincr", 10.0);  /* set max phase incr to 10 degrees */ 
    pboxpar("reps", reps);         /* used to control Pbox output  */
    pboxUpar("attn", rf_pwr, "E"); /* set course attenuation level */ 
    sh = pbox_shape(shn, wvnbw, ofs, st, rf_pw90, rf_pwr);
  }
  
  return sh;
}


/* make a shape using pre-set parameters: maxincr = 10 */

shape pbox_Rsh(shn, wvn, bw, ofs, rf_pw90, rf_pwr)
  
char   *shn, *wvn;
double bw, ofs, rf_pwr, rf_pw90;
{
  char str[MAXSTR];
  double reps = 0.0;
  shape  sh;

  if(bw < 1.0e-7)
  {
    sh.pw = 0.0;
    sh.pwr = 0.0;
    return sh;
  }

  if (autocal[0] == 'r')                         /* read from file */
    sh = getRsh(shn);
  else 
  {
    if(autocal[0] == 'y')                      /* show Pbox output */ 
      reps = 2.0;    
        
    opx("Pbox");                                      /* open Pbox */
    pboxpar("maxincr", 10.0);  /* set max phase incr to 10 degrees */ 
    pboxpar("reps", reps);         /* used to control Pbox output  */
    pboxSpar("type", "r");               /* set shape type to .RF  */
    pboxSpar("attn", "i");             /* use internal attenuation */ 
    sh = pbox_shape(shn, wvn, bw, ofs, rf_pw90, rf_pwr);
  
    if (sh.pwr > rf_pwr) 
    {
      int ret __attribute__((unused));
      printf("pbox_Rsh: Warning! Insufficient power for %s\n ", sh.name);
      printf("%.0f dB needed, %.0f dB available\n ", sh.pwr, rf_pwr);

      sprintf(str, " -attn %.0f%c\n", rf_pwr, 'd');

      ret = system((char *)strcat(px_cmd,str));                      

      sh = getRsh(shn);
      if (sh.pwrf > 4095.0) 
      {
        printf("pbox_Rsh : power error for %s, pwrf = %.0f\n ", sh.name, sh.pwrf);
        psg_abort(1);
      }
    }
  }

  return sh;
}

shape pbox_Dsh(shn, wvn, bw, ofs, rf_pw90, rf_pwr)
  
char   *shn, *wvn;
double bw, ofs, rf_pwr, rf_pw90;
{
  char str[MAXSTR];
  double reps = 0.0;
  shape  sh;

  if(bw < 1.0e-7)
  {
    sh.pw = 0.0;
    sh.pwr = -16.0;
    sh.pwrf = 0.0;
    sh.dres = 1.0;
    sh.dmf = 1000.0;
    return sh;
  }

  if (autocal[0] == 'r')                         /* read from file */
    sh = getDsh(shn);
  else 
  {
    if(autocal[0] == 'y')                      /* show Pbox output */ 
      reps = 2.0;    
        
    opx(shn);                                         /* open Pbox */
    putwave(wvn, bw, ofs, 0.0, 0.0, 0.0);
    pboxpar("maxincr", 10.0);  /* set max phase incr to 10 degrees */ 
    pboxpar("reps", reps);         /* used to control Pbox output  */
    pboxSpar("type", "d");              /* set shape type to .DEC  */
    pboxSpar("attn", "i");             /* use internal attenuation */
    cpx(rf_pw90, rf_pwr);
    sh = getDsh(shn);
  
    if (sh.pwr > rf_pwr) 
    {
      int ret __attribute__((unused));
      printf("\n\n !!!!!!!!!!!!!!!!!!!!!!!! \n\n ");
      printf("pbox_Dsh: Warning! Insufficient power for %s\n ", sh.name);
      printf("%.0f dB needed, %.0f dB available and will be used !\n\n ", sh.pwr, rf_pwr);

      sprintf(str, " -attn %.0f%c\n", rf_pwr, 'd');

      ret = system((char *)strcat(px_cmd,str));                      

      sh = getDsh(shn);
      if (sh.pwrf > 4095.0) 
      {
        printf("pbox_Dsh : power error for %s, pwrf = %.0f\n ", sh.name, sh.pwrf);
        psg_abort(1);
      }
    }
  }

  return sh;
}


/* make a multi-selective shape using pre-set parameters */

void newwave(sh, bw, ofs, st, pha, fla)       /* start a new wave */
char *sh;
double bw, ofs, st, pha, fla;
{
  sprintf(nwaves, "%s %.7f %.2f %.2f %.2f %.2f", 
          sh, bw, ofs, st, pha, fla); 
  
  return; 
}

void addwave(sh, bw, ofs, st, pha, fla)  
char *sh;
double bw, ofs, st, pha, fla;
{
  char tmp[MAXSTR*2];
  sprintf(tmp, "%s\" \"%s %.7f %.2f %.2f %.2f %.2f", 
          nwaves, sh, bw, ofs, st, pha, fla); 
  strcpy(nwaves,tmp);
  
  return; 
}

/* make a multi-selective shape using pre-set parameters */

void newAwave(sh, bw, pws, ofs, st, pha, fla)       /* start a new wave */
char *sh;
double bw, pws, ofs, st, pha, fla;
{
  sprintf(nwaves, "%s %.2f/%.7f %.2f %.2f %.2f %.2f", 
          sh, bw, pws, ofs, st, pha, fla); 
  
  return; 
}

void addAwave(sh, bw, pws, ofs, st, pha, fla)  
char *sh;
double bw, pws, ofs, st, pha, fla;
{
  char tmp[MAXSTR*2];
  sprintf(tmp, "%s\" \"%s %.2f/%.7f %.2f %.2f %.2f %.2f", 
          nwaves, sh, bw, pws, ofs, st, pha, fla); 
  strcpy(nwaves,tmp);
  
  return; 
}

shape pbox_Rshn(shn, rf_pw90, rf_pwr)
char   *shn;
double rf_pwr, rf_pw90;
{
  double reps = 0.0;
  shape  sh;

  if (autocal[0] == 'r')                         /* read from file */
    sh = getRsh(shn);
  else 
  {
    if(autocal[0] == 'y')                      /* show Pbox output */ 
      reps = 2.0;    
        
    opx(shn);                                      /* open Pbox */
    setwave(nwaves);
    pboxpar("maxincr", 10.0);  /* set max phase incr to 10 degrees */ 
    pboxpar("reps", reps);          /* used to control Pbox output */
    pboxSpar("type", "r");                /* set shape type to .RF */
    pboxSpar("attn", "i");             /* use internal attenuation */ 
    cpx(rf_pw90, rf_pwr);
    sh = getRsh(shn);
  }

  if(sh.pwr > 63.0) 
  {
    printf("Pbox : insufficient power for %s pulse.\n", shn);
    sh.pwr = 63.0;
  }
  
  return sh;
}

shape pbox_Dshn(shn, rf_pw90, rf_pwr)
char   *shn;
double rf_pwr, rf_pw90;
{
  double reps = 0.0;
  shape  sh;

  if (autocal[0] == 'r')                         /* read from file */
    sh = getDsh(shn);
  else 
  {
    if(autocal[0] == 'y')                      /* show Pbox output */ 
      reps = 2.0;    
        
    opx(shn);                                      /* open Pbox */
    setwave(nwaves);
    pboxpar("maxincr", 10.0);  /* set max phase incr to 10 degrees */ 
    pboxpar("reps", reps);          /* used to control Pbox output */
    pboxSpar("type", "d");                /* set shape type to .RF */
    pboxSpar("attn", "i");             /* use internal attenuation */ 
    cpx(rf_pw90, rf_pwr);
    sh = getDsh(shn);
  }

  if(sh.pwr > 63.0) 
  {
    printf("Pbox : insufficient power for %s pulse.\n", shn);
    sh.pwr = 63.0;
  }
  
  return sh;
}

shape   pbox_Dec(shn, wvn, bw, decpwr, ref90, refpwr)
char    *shn, *wvn;                                   /* parameter names */
double  bw, decpwr, ref90, refpwr;
{
  double n = 0.0,
         reps = 0.0;
  shape  sh;

  if (autocal[0] == 'r')                         /* read from file */
    sh = getDsh(shn);
  else 
  {
    if(autocal[0] == 'y')                      /* show Pbox output */ 
      reps = 2.0;    
    
    opx(shn);                                         /* open Pbox */
    putwave(wvn, bw, n, n, n, n); 
    pboxpar("reps", reps);           /* used to control Pbox output  */
    if (decpwr > 0)                  /* set course attenuation level */
      pboxUpar("attn", decpwr, "d");
    else
      pboxSpar("attn", "d");    /* attenuate to the nearest dB level */
    pboxSpar("type", "d");                       /* set type to .DEC */
    cpx(ref90, refpwr);
    sh = getDsh(shn);
  }
  
  return sh;
}

/* Adiabatic decoupling */

shape   pbox_Adec(shn, wvn, bw, pws, ofs, ref90, refpwr)  
char    *shn, *wvn;                                   /* parameter names */
double  bw, pws, ofs, ref90, refpwr;
{
  char   wvstr[MAXSTR];
  double reps = 0.0;
  shape  sh;

  if (autocal[0] == 'r')                         /* read from file */
    sh = getDsh(shn);
  else 
  {
    if(autocal[0] == 'y')                      /* show Pbox output */ 
      reps = 2.0;    

    sprintf(wvstr, "%s %.2f/%.7f %.2f", wvn, bw, pws, ofs);   
    opx(shn);  
    setwave(wvstr);                                       
    pboxpar("reps", reps);           /* used to control Pbox output  */
    pboxSpar("type", "d");                       /* set type to .DEC */
    cpx(ref90, refpwr);
    sh = getDsh(shn);
  }
  
  return sh;
}

/* decoupling calibrations - no shapefile is produced in the output
   if decpwr = 0, use bw and attenuate to the nearest dB level, 
   else calculate dmf for the given decpwr.                        */

shape   pbox_Dcal(wvn, bw, decpwr, ref90, refpwr)
char    *wvn;                                   /* parameter names */
double  bw, decpwr, ref90, refpwr;
{
  double n = 0.0,
         reps = 0.0;
  shape  sh;

  if(autocal[0] == 'y')                        /* show Pbox output */ 
    reps = 2.0;    
    
  opx("cal");                                         /* open Pbox */ 
  putwave(wvn, bw, n, n, n, n); 
  pboxpar("reps", reps);           /* used to control Pbox output  */
  if (decpwr > 0)                  /* set course attenuation level */
    pboxUpar("attn", decpwr, "d");
  else
    pboxSpar("attn", "d");    /* attenuate to the nearest dB level */
  cpx(ref90, refpwr);
  pbox_get();
  
  sh.pw = pbox_pw;
  sh.pwr = pbox_pwr;
  sh.pwrf = 4095.0;
  sh.dmf = pbox_dmf;
  sh.dres = pbox_dres; 
 
  return sh;
}


/* shaped pulse calibrations - calculate pw and power settings  
   from the given bw; no shapefile is produced in the output */

shape   pbox_Rcal(shn, bw, ref90, refpwr)
char    *shn;                                   /* parameter names */
double  bw, ref90, refpwr;
{
  double n = 0.0,
         reps = 0.0;
  shape  sh;

  if(autocal[0] == 'y')                        /* show Pbox output */ 
    reps = 2.0;    
    
  opx("cal");                                         /* open Pbox */ 
  pboxpar("reps", reps);           /* used to control Pbox output  */
  pboxUpar("attn", refpwr, "E");
      /* use only fine attenuators */
  putwave(shn, bw, n, n, n, n); 
  cpx(ref90, refpwr);
  pbox_get();
  
  sh.pw = pbox_pw;
  sh.pwr = pbox_pwr;
  sh.pwrf = pbox_pwrf;
 
  return sh;
}


/* ofs_check() - check offsets - tof, dof, dof2 and dof3. 
   0.0 ppm offsets are deemed unlikely and are used as on/off switches. 
   Only D2O, DMSO, CDCl3 and CD3OH are considered as viable solvents.

   Syntax: ofs_check(H1ppm, C13ppm, N15ppm, H2ppm)
   where H1ppm, C13ppm, N15ppm and H2ppm are expected offsets in ppm
   for channels 1, 2, 3 and 4.
   Example: ofs_check(4.75, 56.0, 120.0, 0.0)
   0.0 for channel 4 means that dof3 is not verified.
*/

#define gammaH_D 6.51439977668257645737  /* gammaH/gammaD ratio */
#define gammaC_D 1.63804673571948579067  /* gammaC/gammaD ratio */
#define gammaN_D 0.66009695364263226058  /* gammaN/gammaD ratio */

double get_ofs(Xsfrq, Xtof, Xfrq)  
char   *Xsfrq, *Xtof;
double  Xfrq;
{
  double Xofs;
  
  if(find(Xsfrq) > 0)
  {
    Xofs = getval(Xsfrq);
    return (Xofs - Xfrq)*1000000.0/Xfrq;    /* current offset in ppm */
  }
  else
    return 0.0;
}

int setofs()
{
  extern  int dps_flag;
  char    solv[MAXSTR];
  double  CS_DMSO = 2.49,      /* solvent H-1 & H-2 chemical shifts */
          CS_CDCl3 = 7.25,
          CS_CD3OD = 3.30; 
         
  double  H2frq, H2ofs, solvppm;

  getstr("solvent", solv);
 
  switch (solv[0]) 
  {
    case 'A': case 'a': case 'B': case 'b': case 'E': case 'e':
    case 'M': case 'm': case 'P': case 'p': case 'O': case 'o': 
    case 'T': case 't':
      printf("Pbox_bio: unregistred solvent: %s; offsets not checked.\n", solv);
      return 0;
    case 'D':                               /* check for D2O and DMSO */
      if((solv[1] == '2') && (solv[2] == 'O'))
        solvppm = 7.83 - (273.0 + getval("temp"))/96.9;
      else if ((solv[1] == 'M') && (solv[2] == 'S') && (solv[3] == 'O'))
        solvppm = CS_DMSO;
      else
      {
        printf("Pbox_bio: unregistred solvent: %s; offsets not checked.\n", solv);
        return 0;
      }
    break;
    case 'd' :                               /* check for d2o and dmso */
      if((solv[1] == '2') && (solv[2] == 'o'))
        solvppm = 7.83 - (273.0 + getval("temp"))/96.9;
      else if ((solv[1] == 'm') && (solv[2] == 's') && (solv[3] == 'o'))
        solvppm = CS_DMSO;
      else
      {
        printf("Pbox_bio: unregistred solvent: %s; offsets not checked.\n", solv);
        return 0;
      }
      break;
  case 'C' :                       /* check for CD3OH, CD3OD and CDCl3 */
      if ((solv[1] == 'D') && (solv[2] == '3') && (solv[3] == 'O') && 
           (solv[4] == 'H')) solvppm = CS_CD3OD;
      else if ((solv[1] == 'D') && (solv[2] == '3') && (solv[3] == 'O') && 
           (solv[4] == 'D')) solvppm = CS_CD3OD;
      else if ((solv[1] == 'D') && (solv[2] == 'C') && (solv[3] == 'l') && 
           (solv[4] == '3')) solvppm = CS_CDCl3;
      else
      {
        printf("Pbox_bio: unregistred solvent: %s; offsets not checked.\n", solv);
        return 0;
      }
      break;
  case 'c' :                       /* check for cd3oh, cd3od and cdcl3 */
      if ((solv[1] == 'd') && (solv[2] == '3') && (solv[3] == 'o') && 
           (solv[4] == 'h')) solvppm = CS_CD3OD;
      else if ((solv[1] == 'd') && (solv[2] == '3') && (solv[3] == 'o') && 
           (solv[4] == 'd')) solvppm = CS_CD3OD;
      else if ((solv[1] == 'd') && (solv[2] == 'c') && (solv[3] == 'l') && 
           (solv[4] == '3')) solvppm = CS_CDCl3;
      else
      {
        printf("Pbox_bio: unregistred solvent: %s; offsets not checked.\n", solv);
        return 0;
      }
      break;
  case 'N' :                                         /* check for NONE */
      if ((solv[1] == 'O') && (solv[2] == 'N') && (solv[3] == 'E'))
           solvppm = 0.0;
      else
      {
        printf("Pbox_bio: unregistred solvent: %s; offsets not checked.\n", solv);
        return 0;
      }
      break;
  case 'n' :                                         /* check for NONE */
      if ((solv[1] == 'o') && (solv[2] == 'n') && (solv[3] == 'e'))
           solvppm = 0.0;
      else
      {
        printf("Pbox_bio: unregistred solvent: %s; offsets not checked.\n", solv);
        return 0;
      }
      break;
  default : 
      printf("Pbox_bio: unregistred solvent: %s; assuming 0 ppm.\n", solv);
      solvppm = 0.0;
  }
  
  if((dps_flag) && (find("lockfreq_") > 0))          /* use lockfreq_ in dps mode */
    H2ofs = getval("lockfreq_");
  else if (P_getreal(0, "lockfreq", &H2ofs, 1) < 0) 
    return 0;

  H2frq = 1000.0*H2ofs/(1000.0 + 0.001*solvppm);
  
  tofH = get_ofs("sfrq", "tof", H2frq*gammaH_D); 
  if((tofH < -100.0) || (tofH > 100.0)) 
    return 0;                                        /* check against bad lockfreq_ */   
  tofC = get_ofs("dfrq", "dof", H2frq*gammaC_D);  
  tofN = get_ofs("dfrq2", "dof2", H2frq*gammaN_D); 
  tofD = get_ofs("dfrq3", "dof3", H2frq); 
  
  return 1;
}


void check_ofsH(H1ppm, maxdevH)
double  H1ppm, maxdevH;
{
  double Xdev, dXppm, Xofs, Xfrq;

  if(tof_is_set == 0) return;  /* bad lockfreq_ */
    
  Xfrq = getval("sfrq");

  dXppm = H1ppm - tofH;
  Xdev = dXppm;
  if (Xdev < 0.0) Xdev = -Xdev;
  if(Xdev > maxdevH)
  {
    Xofs = getval("tof");
    Xofs += dXppm*Xfrq;
    printf("tof at %.2f ppm, tof = %.1f (%.2f ppm) expected.\n", 
             tofH, Xofs, H1ppm);
  }
}

void check_ofsC(Cppm, maxdevC)
double  Cppm, maxdevC;
{
  double Xdev, dXppm, Xofs, Xfrq;

  if(tof_is_set == 0) return;  /* bad lockfreq_ */
    
  Xfrq = getval("dfrq");

  dXppm = Cppm - tofC;
  Xdev = dXppm;
  if (Xdev < 0.0) Xdev = -Xdev;
  if(Xdev > maxdevC)
  {
    Xofs = getval("dof");
    Xofs += dXppm*Xfrq;
    printf("dof at %.2f ppm, dof = %.1f (%.2f ppm) expected.\n", 
             tofC, Xofs, Cppm);
  }
}

void check_ofsN(Nppm, maxdevN)
double  Nppm, maxdevN;
{
  double Xdev, dXppm, Xofs, Xfrq;

  if(tof_is_set == 0) return;  /* bad lockfreq_ */
    
  Xfrq = getval("dfrq2");

  dXppm = Nppm - tofN;
  Xdev = dXppm;
  if (Xdev < 0.0) Xdev = -Xdev;
  if(Xdev > maxdevN)
  {
    Xofs = getval("dof2");
    Xofs += dXppm*Xfrq;
    printf("dof2 at %.2f ppm, dof2 = %.1f (%.2f ppm) expected.\n", 
             tofN, Xofs, Nppm);
  }
}

void check_ofsD(H2ppm, maxdevD)
double  H2ppm, maxdevD;
{
  double Xdev, dXppm, Xofs, Xfrq;

  if(tof_is_set == 0) return;  /* bad lockfreq_ */
    
  Xfrq = getval("dfrq3");

  dXppm = H2ppm - tofD;
  Xdev = dXppm;
  if (Xdev < 0.0) Xdev = -Xdev;
  if(Xdev > maxdevD)
  {
    Xofs = getval("dof3");
    Xofs += dXppm*Xfrq;
    printf("dof3 at %.2f ppm, dof3 = %.1f (%.2f ppm) expected.\n", 
             tofD, Xofs, H2ppm);
  }
}

void ofs_check(H1ppm, C13ppm, N15ppm, H2ppm)
double         H1ppm, C13ppm, N15ppm, H2ppm;
{
  double maxdev_H1 = 0.2,   /* maximum difference in ppm for H1  */
         maxdev_H2 = 0.2,   /* maximum difference in ppm for H2  */
         maxdev_C13 = 2.0,  /* maximum difference in ppm for C13 */
         maxdev_N15 = 2.0;  /* maximum difference in ppm for N15 */

  if ((checkofs[0] == 'y') && (tof_is_set)) 
  {
    if (H1ppm > 0.001)
      check_ofsH(H1ppm, maxdev_H1); 
    if (C13ppm > 0.001)
      check_ofsC(C13ppm, maxdev_C13); 
    if (N15ppm > 0.001)
      check_ofsN(N15ppm, maxdev_N15); 
    if (H2ppm > 0.001)
      check_ofsD(H2ppm, maxdev_H2); 
  }
  else                                   /* bad lockfreq_ */
    return;
}

