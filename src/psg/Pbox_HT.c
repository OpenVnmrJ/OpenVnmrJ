/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* Pbox_HT.c - Pbox based Hadamard Encoding and Decoding EXperiments, HEDEX 
                Revision 2 - extended to 4d applications and HT dimension.

   optional flags:
   bscor - Bloch-Siegert shift correction, default is 'yyy'
   repflg - Pbox reports, default is 'yyy', allowed values 'y'|'h'|'n'
   htbitrev - bit reversal flag, default is 'nnn'
   htpwr1, htpwr2, htpwr3, htpwr - power to be calibrated
   htbw1, htbw2, htbw3, htbw - pulseshape linewidth (default 20.0)
   htofs1, htofs2, htofs3 and htofs - shift in ni, ni2, ni3 and niHT, 
                                     default is 0
   htss1, htss2, htss3 and htss - stepsize in F1, F2, F3 and HT shapes, 
                               default is 0 (set by Pbox)
   htcal1, htcal2, htcal3 and htcal - calibration flags, # of excitations, 
                                   default is 0 ('no')
   htCAdec, htCOdec - CC homo-decoupling flags in F1 only, default is 'n'.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "group.h"
#include "variables.h"
#include "rfconst.h"
#include "acqparms.h"
#include "aptable.h"
#include "Pbox_psg.h"
#include "Pbox_HT.h"  /* contains Hadamard matrices */
#include "abort.h"
#include "pvars.h"
#include "cps.h"

#define MAXLL 512
#define FNF0 "HT"         /* line list in HT */
#define FNF1 "F1"         /* line list in F1 */
#define FNF2 "F2"         /* line list in F2 */
#define FNF3 "F3"         /* line list in F3 */

extern int dps_flag;
extern char curexp[];

static double *f0, *f1, *f2, *f3, *b0, *b1, *b2, *b3;
static int    H4x, H2x, H41, H21, H42, H22, H43, H23, nlHT, nlF1, nlF2, nlF3;
static char   repflg[MAXSTR], bscor[MAXSTR], bitrev[MAXSTR];

static int priv_getline(path, line, limit) /* read a line from a file */
int  limit;
char line[];
FILE *path;
{
  int i, ch=0;

  line[0] = '\0';
  i = 0;
  while ((i < limit -1) && ((ch = getc(path)) != EOF) && (ch != '\n'))
    line[i++] = ch;
  line[i] = '\0';
  if ((i == 0) && (ch == EOF))
     return(0);
  else
     return(1);
}

/* retrieve pwbw from pbox.info file */
static double getpwbw(const char *pshape)
{
  FILE  *inpf;
  int    j;
  double pwbw = 0.9;
  char   fname[MAXSTR], cmd[MAXSTR];

  strcpy(fname, userdir);
  strcat(fname, "/shapelib/pbox.info");
  sprintf(cmd, "Pbox -i %s > %s", pshape, fname);
  system(cmd);

  if ((inpf = fopen(fname, "r")) != NULL)
  {
    while (priv_getline(inpf, cmd, MAXSTR))
    {
      j = sscanf(cmd, "pwbw %s %s", fname, fname);
      if ((j > 1) && (j != EOF))
      {
        if (atof(fname))
          pwbw = atof(fname);
        break;
      }
    }
    fclose(inpf);
    sprintf(cmd, "rm -f %s/shapelib/pbox.info", userdir);
    system(cmd);
  }
  return(pwbw);
}

static int is_hm4(nij)  
int nij;
{
  if(!nij) return 0;
  
  if(!(nij%12)) return 12;
  else if(!(nij%20)) return 20;
  else if(!(nij%28)) return 28;
  else return 0; 
}

static int not_p2(nij)  
int nij;
{
  int i=1;

  if(!nij) return 1;
  
  i=1; 
  while (i<nij) i<<=1;

  if(i==nij) return 0;
  else return 1;
}

static int not_hm4(nij, xd)  
int nij, xd;
{
  int i, j;
  
  if (not_p2(nij))
  {
    if((i = is_hm4(nij)) > 0) 
    {
      j = nij/i;
      if(not_p2(j)) return 1;
      else if (xd == 0) H2x = j, H4x = i;
      else if (xd == 1) H21 = j, H41 = i;
      else if (xd == 2) H22 = j, H42 = i;
      else if (xd == 3) H23 = j, H43 = i;
      return 0;
    }
    else return 1;
  } 
  else 
  {
    if (xd == 0) H2x = nij, H4x = 2;
    else if (xd == 1) H21 = nij, H41 = 2;
    else if (xd == 2) H22 = nij, H42 = 2;
    else if (xd == 3) H23 = nij, H43 = 2;
    return 0;
  }
  
  return 1; 
}

static int  br(jx, nx, p2)
int  jx, nx, p2;
{
  int i=1, j=0, k=0;
  
  jx--;
  if(p2 == 2)
  {
    k = nx >> 1;
    while(i<k)       
    {
      j = i + k;
      if(((jx&j) != j) && ((jx&i) || (jx&k)))
        jx = jx ^ j;
      i<<=1, k>>=1;
    }
  }
  jx++;

  return (nx - jx + 1);
}  

static int hm4(nij)
char nij;
{    
  if (nij=='+') return 1;
  else if (nij=='-') return -1;
  else
    text_error("hmtx4 - error in Hadamard Matrix \n");
  return 0;
}


static int hm(nij, i1, i2)
int nij, i1, i2;
{
  int  i, j, k, ij, jj, nii, icf=0;

  if(i1 < 2) return 1;  
  ij=i2-1; jj=i1-1;           /* reset the indexes */
  for(k=1; k<nij; k*=2);
  nii=k;

  j=1; i=1;
  while((j*=2)<=jj) i=j; 
   
  icf = 2*((ij/i)%2);
 
  j=i>>1;  
  while(j>0)  		
  {
    if((i+j) < i1)
    {
      icf = (icf + 2*((ij/j)%2))%4;
      i+=j; 
    }
    j >>= 1;  
  }
  return 1-icf; 
}


static int gethtfrq(fn, bw, nl, fx, bx)		/* set the list of frequencies */
char    *fn, *bw;
int      nl;
double  *fx[], *bx[];
{
  int      j, k, nn, bb;
  double   am=0.0, bws=20.0;
  vInfo info;

  if(nl > MAXLL)
  {
    nl = MAXLL;
    if(FIRST_FID)
      text_error("WARNING: Maximum number of lines 512 reached !\n");
  }
  else if (nl < 1) 
  {
    text_error("No frequencies found. Aborting...\n"); 
    exit(1);
  }

  nn = P_getVarInfo(CURRENT,fn,&info);
  if (nn == 0)
    nn = info.size;
  if (nn < 1)
  {
    text_error("No frequencies found. Aborting...\n"); 
    exit(1);
  }

  bb = P_getVarInfo(CURRENT,bw,&info);
  if (bb == 0)
    bb = info.size;
  if (bb < 1)
    if(FIRST_FID)
      text_message("Using default %s of %g.\n",bw,bws);

  if (nn > nl) nn = nl;
  if (bb > nl) bb = nl;

  for (j=0; j<nn; j++)
  {
    if ((k = P_getreal(CURRENT, fn, &am, j+1)) < 0)
    {
      exit(1);
    }
    if (bb > j)
    {
      k = P_getreal(CURRENT, bw, &bws, j+1);
    }
    (*fx)[j] = am;
    (*bx)[j] = bws;
  }

  return nn; 
}

/* keep same matrix elements for H12, H20, H28 */
static int hm_Fx(nix, i1, i2, h2, h4, brev)   /* ni, ni2, i */
int nix, i1, i2, h2, h4, brev;
{
  int  i, j, ii, jj;

  if(brev)
  {
    i1=br(i1, nix, h4);
    i2=br(i2, nix, h4);
  }
  
  if(h4 == 2) 
  {
    if(i1 < 2) return 1;  
    else return hm(nix, i1, i2);
  }
  else if(h4==12) 
  {
    if(h2>1)
    {
      i = (i1-1)%12;
      j = (i2-1)%12;
      ii= 1 + (i1-1)/12;
      jj= 1 + (i2-1)/12;
      return (hm(h2, ii, jj) * hm4(H12[i][j]));
    }
    else
      return hm4(H12[i1-1][i2-1]);
  }   
  else if(h4==20) 
  {
    if(h2>1)
    {
      i = (i1-1)%20;
      j = (i2-1)%20;
      ii= 1 + (i1-1)/20;
      jj= 1 + (i2-1)/20;
      return (hm(h2, ii, jj) * hm4(H20[i][j]));
    }
    else
      return hm4(H20[i1-1][i2-1]);
  }   
  else if(h4==28) 
  {
    if(h2>1)
    {
      i = (i1-1)%28;
      j = (i2-1)%28;
      ii= 1 + (i1-1)/28;
      jj= 1 + (i2-1)/28;
      return (hm(h2, ii, jj) * hm4(H28[i][j]));
    }
    else
      return hm4(H28[i1-1][i2-1]);
  }   
  return(0);
}


shape  pboxHT_F1(shp, refpw90, refpwr, tp)       /* make shape in F1 */
char    *shp, tp;
double  refpw90, refpwr;
{
  FILE     *inpf;
  long     jx;
  double   CCppm,
           pha = 0.0,
           bws = 20.0,
           htss1 = 0.0,
           reps = 0.0,
           bwCC = 25.0;       /* width of CC decoupling band, in ppm */
  int      i, k, ni, ia, bb, htcal1=0, iofs=0, brev=0, phm=0;
  char     ch=' ', str[MAXSTR], cmd[MAXSTR], COdec[MAXSTR], CAdec[MAXSTR];
  vInfo    info;

  if(tp == 's') ch = '+';
  else if(tp == 'e') pha = 180.0;
  else if(tp == 'r') pha = 90.0, tp = 'e';

  ni = (int) (0.5 + getval("ni"));  
  if (find("htofs1") > 0) /* trying to check active state? */
    iofs = (int) (0.5 + getval("htofs1"));
  if (find("htcal1") > 0)
    htcal1 = (int) (0.5 + getval("htcal1"));
  
  if(ni > 0) htcal1=ni-iofs;  
  else ni=1;

  if (find("htss1") > 0)
    htss1 = getval("htss1");

  jx = 1;
  if(find("ni2") > 0)  
    jx = (long) (0.5 + getval("ni2"));
  if(jx == 0) jx = 1;

  ia = 1;
  if(find("ni3") > 0)  
  ia = (int) (0.5 + getval("ni3"));
  if(ia == 0) ia = 1;
  jx = jx*ia*ni;
  
  ia = (int) (0.5 + getval("arraydim"));
  ia = ia/jx;               /* inner loop */  
  if (ia == 0)
     abort_message("Pbox_HT : arraydim is too small\n");
  jx = 1 + (ix-1)/ia;
  if(((jx-1) < ni) && (((ix-1)%ia) == 0))
  {
    if (FIRST_FID)          /* GET THE FREQUENCY LIST */
    {
      if(not_hm4(ni, 1))
      { 
        text_error("ni must be N, N*12, N*20 or N*28, where N is a power of 2\n");
        psg_abort(1);
      } 
      if(((f1 = (double *) calloc(htcal1, sizeof(double))) == NULL) ||
         ((b1 = (double *) calloc(htcal1, sizeof(double))) == NULL))
      {
        text_error("Pbox_HT F1: problems allocationg memory. Aborting...\n");
        psg_abort(1);
      }
      nlF1 = gethtfrq("htfrq1", "htbw1", htcal1, &f1, &b1);

      if (find("repflg") > 0) getstr("repflg", repflg);
      if (find("htbitrev") > 0) getstr("htbitrev", bitrev);
      if (find("bscor") > 0) getstr("bscor", bscor);
      if (bscor[A] != 'n') bscor[A] = 'y';       
    } 
       
    if (repflg[A] == 'y') reps = 1.0, phm = 1;
    else if (repflg[A] == 'h') phm = 1;
    if (bitrev[A] == 'y') brev = 1;

    (void) sprintf(str, "%s/shapelib/Pbox.inp" , userdir);

    if ((inpf = fopen(str, "w")) == NULL)
    {
      text_error("Cannot open \"%s\"\n", str);
      exit(1);
    }
    
    str[0] = '\0';
    jx = 1 + (jx-1)%ni;    
    for(i=0, k=0; i<nlF1; i++)
    {
      if(hm_Fx(ni, i+1+iofs, (int) jx, H21, H41, brev) > 0)  
      {
        fprintf(inpf, "{ %s %.6f %.2f } %c\n", shp, b1[i], f1[i], ch);
        if (phm) strcat(str,"+");  
        k++;      
      }
      else 
      {
        if(tp == 'e') 
        {
          fprintf(inpf, "{ %s %.6f %.2f n %.1f }\n", shp, b1[i], f1[i], pha);            
          k++;
        }
        if (phm) strcat(str,"-");
      }  
    }
    str[nlF1] = '\0';
    if((reps<1.0) && (phm)) text_message("%s",str);
    strcpy(str,""); str[0]='\0';
    bb = P_getVarInfo(CURRENT,"htbw1",&info);
    if (bb == 0) { bws = getval("htbw1"); } /* htbw1 arrayed? */
    if(k==0) fprintf(inpf, "{ %s %.6f }\n", shp, bws);
    
    if (find("htCOdec") > 0) 
    {
      getstr("htCOdec", COdec);
      if(COdec[0] == 'y')  
      {
        CCppm = getval("dfrq");
        fprintf(inpf, "{ wurst2i %.1f/5ms %.1f n t5 }\n", bwCC*CCppm, 118.0*CCppm);  
      }
    }  
    if (find("htCAdec") > 0) 
    {
      getstr("htCAdec", CAdec);
      if(CAdec[0] == 'y')  
      {
        CCppm = getval("dfrq");
        fprintf(inpf, "{ wurst2i %.1f/5ms %.1f n t5 }\n", bwCC*CCppm, -118.0*CCppm); 
      } 
    }  
    fclose(inpf);

    sprintf(str, "%s_%ld" , FNF1, jx);
 
    if (dps_flag)
    {
       if(k==0) 
         sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -attn 0i -%.0f -u %s\n",
              str, bscor[A], htss1, refpwr, 1e6*refpw90, reps, userdir);
       else    
         sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -%.0f -u %s\n",
              str, bscor[A], htss1, refpwr, 1e6*refpw90, reps, userdir);
    }
    else
    {
       if(k==0) /* could use curexp instead of userdir */
         sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -attn 0i -%.0f -u %s\n",
              str, bscor[A], htss1, refpwr, 1e6*refpw90, reps, userdir);
       else    
         sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -%.0f -u %s\n",
              str, bscor[A], htss1, refpwr, 1e6*refpw90, reps, userdir);
    }
    system(cmd);                                  /* execute Pbox */
    if(reps > 0) text_message("  cmd : %s", cmd);
  }
  else
  {
    jx = 1 + (jx-1)%ni;
    (void) sprintf(str, "%s_%ld" , FNF1, jx);  
  }

  return getRsh(str);
}

shape  pboxHT_F1e(shp, refpw90, refpwr)       /* make excitation shape in F1 */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT_F1(shp, refpw90, refpwr, 'e');
}

shape  pboxHT_F1i(shp, refpw90, refpwr)       /* make inversion shape in F1 */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT_F1(shp, refpw90, refpwr, 'i');
}

shape  pboxHT_F1s(shp, refpw90, refpwr)       /* make sequential inv shape in F1 */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT_F1(shp, refpw90, refpwr, 's');
}

shape  pboxHT_F1r(shp, refpw90, refpwr)       /* make refocusing shape in F1 */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT_F1(shp, refpw90, refpwr, 'r');
}


shape  pboxHT_F2(shp, refpw90, refpwr, tp)       /* make shape in F2 */
char    *shp, tp;
double  refpw90, refpwr;
{
  FILE     *inpf;
  long     jx;
  double   bws = 20.0,
           pha = 0.0,
           htss2 = 0.0,
           reps = 0.0;
  int      i, k, ni2, ia, bb, htcal2=0, iofs=0, brev=0, phm=0;
  char     ch=' ', str[MAXSTR], cmd[MAXSTR];
  vInfo    info;

  if(tp == 's') ch = '+';
  else if(tp == 'e') pha = 180.0;
  else if(tp == 'r') pha = 90.0, tp = 'e';

  ni2 = (int) (0.5 + getval("ni2")); 
   
  if (find("htofs2") > 0)
    iofs = (int) (0.5 + getval("htofs2"));
  if (find("htcal2") > 0)
    htcal2 = (int) (0.5 + getval("htcal2"));
  
  if(ni2 > 0) htcal2=ni2-iofs;  
  else ni2=1;
  
  if (find("htss2") > 0)
    htss2 = getval("htss2");
  
  ia = 1;
  if(find("ni3") > 0)  
  ia = (int) (0.5 + getval("ni3"));
  if(ia == 0) ia = 1;
  jx = ia*ni2;
  
  ia = (int) (0.5 + getval("arraydim"));  
  ia = ia/jx;               /* inner loop */  
  jx = 1 + (ix-1)/ia;  
  if(((jx-1) < ni2) && (((ix-1)%ia) == 0))
  {
    if (FIRST_FID)          /* GET THE FREQUENCY LIST */
    {
      if(not_hm4(ni2, 2))
      { 
        text_error("ni2 must be N, N*12, N*20 or N*28, where N is a power of 2\n");
        psg_abort(1);
      } 
      if(((f2 = (double *) calloc(htcal2, sizeof(double))) == NULL) ||
         ((b2 = (double *) calloc(htcal2, sizeof(double))) == NULL))
      {
        text_error("Pbox_HT F2: problems allocationg memory. Aborting...\n");
        psg_abort(1);
      }
      nlF2 = gethtfrq("htfrq2", "htbw2", htcal2, &f2, &b2);

      if (find("repflg") > 0) getstr("repflg", repflg);
      if (find("htbitrev") > 0) getstr("htbitrev", bitrev);
      if (find("bscor") > 0) getstr("bscor", bscor);
      if (bscor[B] != 'n') bscor[B] = 'y';            
    }
    
    if (repflg[B] == 'y') reps = 1.0, phm = 1;
    else if (repflg[B] == 'h') phm = 1;
    if (bitrev[B] == 'y') brev = 1;
   
    (void) sprintf(str, "%s/shapelib/Pbox.inp" , userdir);

    if ((inpf = fopen(str, "w")) == NULL)
    {
      text_error("Cannot open \"%s\"\n", str);
      exit(1);
    }

    str[0] = '\0';
    jx = 1 + (jx-1)%ni2;    
    for(i=0, k=0; i<nlF2; i++)
    {
      if(hm_Fx(ni2, i+1+iofs, (int) jx, H22, H42, brev) > 0) 
      {
        fprintf(inpf, "{ %s %.6f %.2f } %c\n", shp, b2[i], f2[i], ch);
        if (phm) strcat(str,"+");  
        k++;      
      }
      else 
      {
        if(tp == 'e') 
        {
          fprintf(inpf, "{ %s %.6f %.2f n %.1f }\n", shp, b2[i], f2[i], pha);
          k++;
        }
        if (phm) strcat(str,"-");  
      }  
    }
    if((reps<1.0) && (phm)) text_message("%s",str);
    strcpy(str,""); str[0]='\0';
    bb = P_getVarInfo(CURRENT,"htbw2",&info);
    if (bb == 0) { bws = getval("htbw2"); } /* htbw2 arrayed? */
    if(k==0) fprintf(inpf, "{ %s %.6f }\n", shp, bws);
    fclose(inpf);
  
    (void) sprintf(str, "%s_%ld", FNF2, jx);

    if (dps_flag)
    {
      if(k==0) 
        sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -attn 0i -%.0f -u %s\n",
              str, bscor[B], htss2, refpwr, 1e6*refpw90, reps, userdir);
      else    
        sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -%.0f -u %s\n",
            str, bscor[B], htss2, refpwr, 1e6*refpw90, reps, userdir);
    }
    else
    {
      if(k==0) /* could use curexp instead of userdir */
        sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -attn 0i -%.0f -u %s\n",
              str, bscor[B], htss2, refpwr, 1e6*refpw90, reps, userdir);
      else    
        sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -%.0f -u %s\n",
            str, bscor[B], htss2, refpwr, 1e6*refpw90, reps, userdir);
    }
    system(cmd);                                  /* execute Pbox */
    if(reps > 0) text_message("  cmd : %s", cmd);
  }
  else
  {
    jx = 1 + (jx-1)%ni2; 
    (void) sprintf(str, "%s_%ld", FNF2, jx);  
  }
  
  return getRsh(str);
}

shape  pboxHT_F2e(shp, refpw90, refpwr)       /* make excitation shape in F2 */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT_F2(shp, refpw90, refpwr, 'e');
}

shape  pboxHT_F2i(shp, refpw90, refpwr)       /* make inversion shape in F2 */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT_F2(shp, refpw90, refpwr, 'i');
}

shape  pboxHT_F2s(shp, refpw90, refpwr)       /* make sequential inv shape in F2 */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT_F2(shp, refpw90, refpwr, 's');
}

shape  pboxHT_F2r(shp, refpw90, refpwr)       /* make refocusing shape in F2 */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT_F2(shp, refpw90, refpwr, 'r');
}


shape  pboxHT_F3(shp, refpw90, refpwr, tp)       /* make shape in F3 */
char    *shp, tp;
double  refpw90, refpwr;
{
  FILE     *inpf;
  long     jx;
  double   bws = 20.0,
           pha = 0.0,
           htss3 = 0.0,
           reps = 0.0;
  int      i, k, ia, ni3, bb, htcal3=0, iofs=0, brev=0, phm=0;
  char     ch=' ', str[MAXSTR], cmd[MAXSTR];
  vInfo    info;

  if(tp == 's') ch = '+';
  else if(tp == 'e') pha = 180.0;
  else if(tp == 'r') pha = 90.0, tp = 'e';

  ni3 = (int) (0.5 + getval("ni3")); 
 
  if (find("htofs3") > 0)
    iofs = (int) (0.5 + getval("htofs3"));
  if (find("htcal3") > 0)
    htcal3 = (int) (0.5 + getval("htcal3"));

  if(ni3 > 0) htcal3=ni3-iofs;  
  else ni3=1;
  
  if (find("htss3") > 0)
    htss3 = getval("htss3");
  
  ia = (int) (0.5 + getval("arraydim"));
  ia = ia/ni3;               /* inner loop */  
  jx = 1 + (ix-1)/ia;
  if(((jx-1) < ni3) && (((ix-1)%ia) == 0))
  {
    if (FIRST_FID)          /* GET THE FREQUENCY LIST */
    {
      if(not_hm4(ni3, 3))
      { 
        text_error("ni3 must be N, N*12, N*20 or N*28, where N is a power of 2\n");
        psg_abort(1);
      }       
      if(((f3 = (double *) calloc(htcal3, sizeof(double))) == NULL) ||
         ((b3 = (double *) calloc(htcal3, sizeof(double))) == NULL))
      {
        text_error("Pbox_HT F3: problems allocationg memory. Aborting...\n");
        psg_abort(1);
      }
      nlF3 = gethtfrq("htfrq3", "htbw3", htcal3, &f3, &b3);

      if (find("repflg") > 0) getstr("repflg", repflg);
      if (find("htbitrev") > 0) getstr("htbitrev", bitrev);
      if (find("bscor") > 0) getstr("bscor", bscor);
      if (bscor[C] != 'n') bscor[C] = 'y';       
    }
    
    if (repflg[C] == 'y') reps = 1.0, phm = 1;
    else if (repflg[C] == 'h') phm = 1;
    if (bitrev[C] == 'y') brev = 1;
   
    (void) sprintf(str, "%s/shapelib/Pbox.inp" , userdir);

    if ((inpf = fopen(str, "w")) == NULL)
    {
      text_error("Cannot open \"%s\"\n", str);
      exit(1);
    }

    str[0] = '\0';
    jx = 1 + (jx-1)%ni3;    
    for(i=0, k=0; i<nlF3; i++)
    {
      if(hm_Fx(ni3, i+1+iofs, (int) jx, H23, H43, brev) > 0) 
      {
        fprintf(inpf, "{ %s %.6f %.2f } %c\n", shp, b3[i], f3[i], ch);
        if (phm) strcat(str,"+");  
        k++;      
      }
      else 
      {
        if(tp == 'e') 
        {
          fprintf(inpf, "{ %s %.6f %.2f n %.1f }\n", shp, b3[i], f3[i], pha);
          k++;
        }
        if (phm) strcat(str,"-");
      }  
    }
    if((reps<1.0) && (phm)) text_message("%s",str);
    strcpy(str,""); str[0]='\0';
    bb = P_getVarInfo(CURRENT,"htbw3",&info);
    if (bb == 0) { bws = getval("htbw3"); } /* htbw3 arrayed? */
    if(k==0) fprintf(inpf, "{ %s %.6f }\n", shp, bws);
    fclose(inpf);
  
    (void) sprintf(str, "%s_%ld", FNF3, jx);  

    if (dps_flag)
    {
      if(k==0) 
        sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -attn 0i -%.0f -u %s\n",
              str, bscor[C], htss3, refpwr, 1e6*refpw90, reps, userdir);
      else    
        sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -%.0f -u %s\n",
            str, bscor[C], htss3, refpwr, 1e6*refpw90, reps, userdir);
    }
    else
    {
      if(k==0) /* could use curexp instead of userdir */
        sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -attn 0i -%.0f -u %s\n",
              str, bscor[C], htss3, refpwr, 1e6*refpw90, reps, userdir);
      else    
        sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -%.0f -u %s\n",
            str, bscor[C], htss3, refpwr, 1e6*refpw90, reps, userdir);
    }
    system(cmd);                                  /* execute Pbox */
    if(reps > 0) text_message("  cmd : %s", cmd);
  }
  else
  {
    jx = 1 + (jx-1)%ni3;                         
    (void) sprintf(str, "%s_%ld" , FNF3, jx);     
  }
  
  return getRsh(str);
}


shape  pboxHT_F3e(shp, refpw90, refpwr)       /* make excitation shape in F3 */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT_F3(shp, refpw90, refpwr, 'e');
}

shape  pboxHT_F3i(shp, refpw90, refpwr)       /* make inversion shape in F3 */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT_F3(shp, refpw90, refpwr, 'i');
}

shape  pboxHT_F3s(shp, refpw90, refpwr)       /* make refocusing shape in F3 */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT_F3(shp, refpw90, refpwr, 's');
}

shape  pboxHT_F3r(shp, refpw90, refpwr)       /* make refocusing shape in F3 */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT_F3(shp, refpw90, refpwr, 'r');
}


static long HTarr()
{
  long  jx;
  char  array[MAXSTR];
  int  i, j, k;
  
  j = (int) (0.5 + getval("arrayelemts"));
  getstr("array", array);
  i = strlen(array);  
  if(j == 1)
  {
    if(i == 4 && array[0] == 'i' && array[1] == 'x' && 
       array[2] == 'H' && array[3] == 'T')
      return 1;
    else
      return 0;
  }
  else if (j > 1)
  {
    if(i == 4 && array[i-1] == 'T' && array[i-2] == 'H' && 
      array[i-3] == 'x' && array[i-4] == 'i')
      return 1;
    else if(i > 4 && array[i-1] == 'T' && array[i-2] == 'H' && 
      array[i-3] == 'x' && array[i-4] == 'i' && array[i-5] == ',')
      return 1;
    else if(i > 4 && array[0] == 'i' && array[1] == 'x' && 
      array[2] == 'H' && array[3] == 'T' && array[4] == ',')
    {
      k = 1;
      if(find("ni") > 0)  
        k = (int) (0.5 + getval("ni"));
      if(k == 0) k = 1;

      jx = k; k = 1;
      if(find("ni2") > 0)  
        k = (int) (0.5 + getval("ni2"));
      if(k == 0) k = 1;

      jx = jx*k; k = 1;
      if(find("ni3") > 0)  
        k = (int) (0.5 + getval("ni3"));
      if(k == 0) k = 1;
      
      return (long) k*jx;    
    }
    else
    {
      text_error("Pbox_HT : ixHT index must be 1-st or last in the array\n");
      psg_abort(1);
    }
  }
  
  return 0;
}

shape  pboxHT(shp, refpw90, refpwr, tp)       /* make shape in HT dimension */
char    *shp, tp;
double  refpw90, refpwr;
{
  FILE     *inpf;
  long     jx;
  double   bws = 20.0,
           pha = 0.0,
           htss = 0.0,
           reps = 0.0;
  int      i, k, niHT, ia, bb, htcal=0, iofs=0, brev=0, phm=0;
  char     ch=' ', str[MAXSTR], cmd[MAXSTR];
  vInfo    info;

  ia = 1;
  if(tp == 's') ch = '+';
  else if(tp == 'e') pha = 180.0;
  else if(tp == 'r') pha = 90.0, tp = 'e';

  niHT = (int) (0.5 + getval("htni"));  
   
  if (find("htofs") > 0)
    iofs = (int) (0.5 + getval("htofs"));
  if (find("htss") > 0)
    htss = getval("htss");
  
  if (find("htcal") > 0)
    htcal = (int) (0.5 + getval("htcal"));
  
  if(niHT > 0) htcal=niHT-iofs;  
  else niHT=1;
  
  jx = HTarr();
  if(jx == 1)
    ia = 1;
  else if(jx > 1)
  {
    jx = jx*niHT;  
    ia = (int) (0.5 + getval("arraydim"));
    ia = ia/jx;               /* inner loop */ 
  }
  else if((htcal > 0) && (jx == 0))
    ia = 1;
  else 
  {
    text_error("Pbox_HT : must array ixHT index from 1 to niHT\n");
    psg_abort(1); 
  }
 
  jx = 1 + (ix-1)/ia;
  if(((jx-1) < niHT) && (((ix-1)%ia) == 0))
  {
    if (FIRST_FID)          /* GET THE FREQUENCY LIST */
    {
      if(not_hm4(niHT, 0))
      { 
        text_error("niHT must be N, N*12, N*20 or N*28, where N is a power of 2\n");
        psg_abort(1);
      } 
      if(((f0 = (double *) calloc(htcal, sizeof(double))) == NULL) ||
         ((b0 = (double *) calloc(htcal, sizeof(double))) == NULL))
      {
        text_error("Pbox_HT: problems allocationg memory. Aborting...\n");
        psg_abort(1);
      }
      nlHT = gethtfrq("htfrq", "htbw", htcal, &f0, &b0);

      if (find("repflg") > 0) getstr("repflg", repflg);
      if (find("htbitrev") > 0) getstr("htbitrev", bitrev);
      if (find("bscor") > 0) getstr("bscor", bscor);
      if (bscor[A] != 'n') bscor[A] = 'y';       
    }
    
    if (repflg[A] == 'y') reps = 1.0, phm = 1;
    else if (repflg[A] == 'h') phm = 1;
    if (bitrev[A] == 'y') brev = 1;
   
    (void) sprintf(str, "%s/shapelib/Pbox.inp" , userdir);

    if ((inpf = fopen(str, "w")) == NULL)
    {
      text_error("Cannot open \"%s\"\n", str);
      exit(1);
    }
 
    str[0] = '\0';
    jx = 1 + (jx-1)%niHT; 
    for(i=0, k=0; i<nlHT; i++)
    {
      if(hm_Fx(niHT, i+1+iofs, (int) jx, H2x, H4x, brev) > 0) 
      {
        fprintf(inpf, "{ %s %.6f %.2f } %c\n", shp, b0[i], f0[i], ch);
        if (phm) strcat(str,"+");
        k++;      
      }
      else 
      {
        if(tp == 'e') 
        {
          fprintf(inpf, "{ %s %.6f %.2f n %.1f }\n", shp, b0[i], f0[i], pha);
          k++;
        }
        if (phm) strcat(str,"-");
      }  
    }
    if((reps<1.0) && (phm)) text_message("%s",str);
    strcpy(str,""); str[0]='\0';
    bb = P_getVarInfo(CURRENT,"htbw",&info);
    if (bb == 0) { bws = getval("htbw"); } /* htbw arrayed? */
    if(k==0) fprintf(inpf, "{ %s %.6f }\n", shp, bws);
    fclose(inpf);
      
    (void) sprintf(str, "%s_%ld", FNF0, jx);

    if (dps_flag)
    {
       if(k==0) 
         sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -attn 0i -%.0f -u %s\n",
              str, bscor[A], htss, refpwr, 1e6*refpw90, reps, userdir);
       else    
         sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -%.0f -u %s\n",
            str, bscor[A], htss, refpwr, 1e6*refpw90, reps, userdir);
    }
    else
    {
       if(k==0) /* could use curexp instead of userdir */
         sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -attn 0i -%.0f -u %s\n",
              str, bscor[A], htss, refpwr, 1e6*refpw90, reps, userdir);
       else    
         sprintf(cmd, "Pbox %s -bscor %c -s %.2f -p %.0f -l %.2f -%.0f -u %s\n",
            str, bscor[A], htss, refpwr, 1e6*refpw90, reps, userdir);
    }
    system(cmd);                                  /* execute Pbox */
    if(reps > 0) text_message("  cmd : %s", cmd);
  }
  else
  {
    jx = 1 + (jx-1)%niHT; 
    (void) sprintf(str, "%s_%ld" , FNF0, jx);  
  }
  
  return getRsh(str);
}

shape  pboxHTe(shp, refpw90, refpwr)       /* make excitation shape in HT */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT(shp, refpw90, refpwr, 'e');
}

shape  pboxHTi(shp, refpw90, refpwr)       /* make inversion shape in HT */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT(shp, refpw90, refpwr, 'i');
}

shape  pboxHTr(shp, refpw90, refpwr)       /* make refocusing shape in HT */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT(shp, refpw90, refpwr, 'r');
}

shape  pboxHTs(shp, refpw90, refpwr)       /* make refocusing shape in HT */
char    *shp;
double  refpw90, refpwr;
{
  return pboxHT(shp, refpw90, refpwr, 's');
}

/* ~~~~~~~~~~~~~~~ Decoupler Functions : Use for simshaped_pulse ~~~~~~~~~~~~~~~~*/

static void pboxHT_makedec(fname, Tp, bw, Jxy, stepsize, ref_pw90, ref_pwr)
char *fname;
double Tp, bw, Jxy, stepsize, ref_pw90, ref_pwr;
{
  char    cmd[MAXSTR],
          str[40],
          su[20] = "n";
  double  ncyc = 1.0,
          nsu = 5.0 * Jxy * Tp;
  getstr("repflg", repflg);

  if (nsu < 2.0)        { strcpy(su, "n");         ncyc = 1.0;   }
  else if (nsu < 4.2)   { strcpy(su, "m4");        ncyc = 4.0;   }
  else if (nsu < 6.0)   { strcpy(su, "t5");        ncyc = 5.0;   }
  else if (nsu < 10.0)  { strcpy(su, "m8");        ncyc = 8.0;   }
  else if (nsu < 17.0)  { strcpy(su, "m16");       ncyc = 16.0;  }
  else if (nsu < 21.0)  { strcpy(su, "t5,m4");     ncyc = 20.0;  }
  else if (nsu < 27.0)  { strcpy(su, "t5,t5");     ncyc = 25.0;  }
  else if (nsu < 45.0)  { strcpy(su, "t5,m8");     ncyc = 40.0;  }
  else if (nsu < 85.0)  { strcpy(su, "t5,m16");    ncyc = 80.0;  }
  else if (nsu < 110.0) { strcpy(su, "t5,t5,m4");  ncyc = 100.0; }
/* else
    text_message("psg pboxHT_makedec: using default shape for %s, no supercycles\n",fname);
*/
  Tp /= ncyc;

  sprintf(cmd, "Pbox %s -u %s -w \"wurst2i %9.7f/%3.1f\" ", fname, userdir, Tp, bw);
  if (stepsize < 0.05)
    sprintf(str, " -s n -sucyc %s", su);
  else
    sprintf(str, " -s %5.2f -sucyc %s", stepsize, su);
  strcat(cmd, str);

  if (ref_pw90 > 0.01)
  {
    sprintf(str, " -p %2.0f -l %5.2f", ref_pwr, ref_pw90);
    strcat(cmd, str);
  }

  if (repflg[A] == 'y') text_message("  cmd : %s\n",cmd);
  system(cmd);
}

/**************************************************************
*                      pboxHT_dec()                           *
* Make shape for H channel for simultaneous decoupling pulse. *
* Use for decoupling the Hadamard encoded nucleus.            *
* 1st indirect dimension only. For other dimensions, create   *
*   pboxHT_dec2() -> htbw2, htss2, dfrq2, etc.                *
**************************************************************/

/*
 * pwpat      decoupling shape name for H channel
 * pshape     waveform name for X channel
 * jXH        coupling constant in Hz
 * bw         decoupling bandwidth in ppm
 * ref_pw     ref pw(H) used to make shape
 * ref_comp   ref compH used to make shape
 * ref_pwr    ref tpwr(H) used to make shape
 */
shape pboxHT_dec(char *pwpat, const char *pshape, double jXH,
              double bw, double ref_pw, double ref_comp, double ref_pwr)
{
    char        tmp[MAXSTR];
    double      htbw = getval("htbw1"),
                htss = getval("htss1"),
                pwbw = 0.9,
                decpw,
                decpwr,
                nsu,
                Tp,
                pwbwXH,
                stepsize,
                pwcalc = ref_pw * ref_comp * 1e6;

    if (jXH <= 0.0) jXH = 150.0; /* could be 150, 90, 20 Hz */
    if (bw < 16.0) bw = 16.0; /* could be 20, 60, 200 ppm */
    if (htbw < 1.0) htbw = 20.0;
    if (htss < 0.0) htss = 0.0;
    strcpy(tmp, pwpat);
    if (strcmp(pshape,"")==0) pshape = "gaus180";

    if (strcmp(pwpat,"") != 0)
    {
      pwbw = getpwbw(pshape);
      decpw = pwbw / htbw;

/*    Make decoupling sequence */
      nsu = 5.0 * jXH * decpw;
      Tp = decpw / nsu;
      bw *= dfrq;
      pwbwXH = bw * Tp;
      if (pwbwXH < 20.0) /* could be 16.0, pass argument adiabat_min? */
        bw = 20.0 / Tp; /* increase if too small for adiabatics */
      stepsize = htss / 2.0;
      if ((stepsize > 10.0) || (stepsize < 1.0))
        stepsize = 0.0;

      strcpy(tmp, pwpat);

      pboxHT_makedec(tmp, decpw, bw, jXH, stepsize, pwcalc, ref_pwr);

      pbox_get();
      decpwr = pbox_pwr;
      if (decpwr > 52.0)
        text_error("Warning: pboxHT_dec decpwr too high\n");
    }
    return (getRsh(tmp));
}
