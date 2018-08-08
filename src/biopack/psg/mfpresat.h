/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*  mfpresat - multi-frequency presat functions. Eriks Kupce, Oxford, June 2005.
 
               Pbox utility functions for multi-frequency presat and multi-frequency 
               CW homo-decoupling. The mfpresat.ll and mfhomodec.ll line lists are
               expected in the current experiment. This mfhdec() method is appropriate
               for INOVA or UNITYplus. Band-selective homodecoupling is standard on
               all sequences on the VNMRS console.
*/               

#ifndef FIRST_FID
#include <Pbox_psg.h>
#endif

#define FNLL "mfpresat.ll"         /* line list for mfpresat */
#define FNHD "mfhomodec.ll"        /* line list for mfhomodecoupling */
#define MAXN  128                  /* maximum number of frequencies */

static shape  mfshape, mfhdshape;

int getmfll(fnm, fx)		   /* get the list of frequencies */
double  *fx[];
char    *fnm;
{
  FILE     *inpf;
  int      j, k, nn;
  char     str[MAXSTR];
  double   tm=0.0;
  extern   char curexp[];
  char *ret __attribute__((unused));

  
  (void) sprintf(str, "%s/%s" , curexp, fnm);
  
  if ((inpf = fopen(str, "r")) == NULL)
  {
    printf("Cannot find \"%s\"\n", str);
    psg_abort(1);
  }

  while ((getc(inpf)) == '#')
     ret = fgets(str, MAXSTR, inpf);  /* ignore com-s */
  k = ftell(inpf); fseek(inpf, k-1, 0);

  j = 0; nn = 0; 
  while ((fgets(str, MAXSTR, inpf)) && (nn < MAXN)) 
  {
    k = sscanf(str, "%d %lf\n", &j, &tm); 
    if(k==2)
      (*fx)[nn] = tm;   
    nn++; 
  }
  fclose(inpf);						/* close the INfile */

  return nn; 
}


shape  pbox_mfcw()       /* make mfcw shape */
{
  FILE     *inpf;
  double   *f1, reps = 0.0;
  int       i, nl;
  char      str[MAXSTR], cmd[MAXSTR], repflg[MAXSTR];
  extern char userdir[];
  int ret __attribute__((unused));

  getstr("repflg", repflg);

  /* GET THE FREQUENCY LIST */
  
  if((f1 = (double *) calloc(MAXN, sizeof(double))) == NULL)
  {
    printf("pbox_mfcw: problems allocationg memory. Aborting...\n");
    psg_abort(1);
  }
  nl = getmfll(FNLL, &f1);   
       
  if (repflg[A] == 'y') reps = 1.0;

  (void) sprintf(str, "%s/shapelib/Pbox.inp" , userdir);
  if ((inpf = fopen(str, "w")) == NULL)
  {
    printf("Cannot open \"%s\"\n", str);
    psg_abort(1);
  }
        
  for(i=0; i<nl; i++)
    fprintf(inpf, "{ square90 %.6fs %.2f } \n", satdly, f1[i]);
  fclose(inpf);

  sprintf(cmd, "Pbox mfpresat.DEC -%.0f\n", reps);
  ret = system(cmd);                                  /* execute Pbox */
  if(reps > 0) printf("  cmd : %s", cmd);
  
  return getDsh("mfpresat");
}


shape  pbox_mfdec(dcyc)       /* make mfdec shape */
double dcyc;
{
  FILE     *inpf;
  double   *f1, reps = 0.0, stepsize=2.0*getval("at")/getval("np");
  int       i, nl;
  char      str[MAXSTR], cmd[MAXSTR], repflg[MAXSTR];
  extern char userdir[];
  int ret __attribute__((unused));

  getstr("repflg", repflg);

  /* GET THE FREQUENCY LIST */
  
  if((f1 = (double *) calloc(MAXN, sizeof(double))) == NULL)
  {
    printf("pbox_mfcwdec: problems allocationg memory. Aborting...\n");
    psg_abort(1);
  }
  nl = getmfll(FNHD, &f1);   
       
  if (repflg[A] == 'y') reps = 1.0;

  (void) sprintf(str, "%s/shapelib/Pbox.inp" , userdir);
  if ((inpf = fopen(str, "w")) == NULL)
  {
    printf("Cannot open \"%s\"\n", str);
    psg_abort(1);
  }
        
  for(i=0; i<nl; i++)
    fprintf(inpf, "{ square90 %.6fs %.2f } \n", getval("at"), f1[i]);
  fclose(inpf);

  sprintf(cmd, "Pbox mfhdec.DEC -s %.2f -dcyc %.3f, -%.0f\n", 1.0e6*stepsize, dcyc, reps);
  ret = system(cmd);                                  /* execute Pbox */
  if(reps > 0) printf("  cmd : %s", cmd);
  
  return getDsh("mfhdec");
}


void mfpresat() /* use satfrq */
{
  setlimit("satpwr", satpwr, 30.0);
  obsoffset(tof);
  
  if(FIRST_FID)
    mfshape = pbox_mfcw();

  if ((satmode[0] == 'y') && (satdly > 1.0e-5))
  {
    obspower(satpwr);
    delay(d1);
    spinlock(mfshape.name, 1.0/mfshape.dmf, mfshape.dres, zero, 1);
  }
  else
    delay(d1);
}


void mfhomodec(dcyc) 
double dcyc;
{
  setlimit("dcyc", dcyc, 1.0);
  
  if(FIRST_FID)
    mfhdshape = pbox_mfdec(dcyc);
    
  mfhdshape.pwr = getval("hhdpwr");
  mfhdshape.pwrf = 4095.0;
    
  homodec(&mfhdshape);
}


void mfpresat_on() 
{
  setlimit("satpwr", satpwr, 30.0);
  obsoffset(tof);
  
  if(FIRST_FID)
    mfshape = pbox_mfcw();

  mfshape.pwr = satpwr;
  mfshape.pwrf = 0.0;
  (void) pbox_xmtron(&mfshape);

  return;
}

void mfpresat_off() /* use satfrq */
{
  (void) pbox_xmtroff();
  
  return;
}
