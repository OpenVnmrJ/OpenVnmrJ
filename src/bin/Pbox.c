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
/* Pbox.c. - Pandora's (pulse) box, v.6.1.97, Varian format output
   E. Kupce, Varian Ltd, Oxford
   waveform mixing as in J. Magn. Reson. (A) 105 (1993) 234. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "Pbox_err.h"           /* Pbox errors */
#include "Pbox.h"
#include "Pbox_sim.h"           /* Pbox simulator */

int main(int argc, char *argv[])
{
  FILE *fil;
  char opt = ' ', verb = 'n', un, val[10], fn_inp[MAXSTR], h_name[MAXSTR];
  int  i, j, k, ip, narg, inm;

/* ------------------------------- Check the Options -----------------------*/

  if ((argc>1) && (argv[1][0] != '-'))
    j=2;
  else
    j=1;

  home[0]='\0';
  while(j<argc)	
  {
    if (argv[j][0] != '-')
      k=1;
    else if(ispar(argv[j]))
      k=2;
    else
    {
      for(k=1, i=1; argv[j][i] != '\0'; i++)
      {
        switch(argv[j][i])
        {
          case 'f': if ((argc > j+1) && (argv[j+1][0] != '-'))
                      (void) strcpy(fn_inp, argv[j+1]); 
                    else
                      pxerr("Option -f : filename is missing");
                    opt='f';
                    break;
          case 'h': opt='n'; break;
          case 'i': opt='n'; break;
          case 'o': opt='n'; break;
          case 'r': if ((argc > j+1) && (argv[j+1][0] != '-'))
                      strcpy(h_name, argv[j+1]);
                    else
                      pxerr("Option -r : filename is missing");
                    opt='r'; 
                    break;
          case 't': opt='n'; break; 
          case 'u': if ((argc > j+1) && (argv[j+1][0] != '-') &&
                        (sethome(argv[j+1]) == 0))
                      pxerr("Option -u : invalid filename");
                    break;
          case 'v': verb='y';
		    if (opt == ' ') opt = 'n'; break; 
          case 'w': opt='n'; break; 
          case 'x': opt='n'; break; 
        }
      }
    }
    j+=k;
  }

  imod = 0; 
  setpbox(0); 

/* ------------------------------- Open Input File -------------------------*/

  if (opt != 'n')
  {
    if (opt == 'f')
      (void) strcat(strcpy(ifn, shapes), fn_inp);
    else if (opt == 'r')
    {
      (void) strcat(strcpy(ifn, shapes), FN_TMP);
      if ((fil = fopen(ifn, "w")) == NULL)
        err(ifn);
      (void) strcat(strcpy(h.fname, shapes), h_name);
      mkinp(h.fname, fil); 
      fclose(fil);
      setname(h.name);
    }                    
    else 
      (void) strcat(strcpy(ifn, shapes), FN_INP);

    if ((fil = fopen(ifn, "r")) == NULL)		/* open input file */
    {
      (void) strcat(strcpy(ifn, SYSSHAPELIB), FN_INP);
      if ((fil = fopen(ifn, "r")) == NULL)
        err(FN_INP);
    }
    h.itnf = getnf(fil);
    gethdr(fil);
  } 

/* ------------------------------- Evaluate Options -----------------------*/

  if ((argc>1) && (argv[1][0] != '-'))
  {
    (void) setname(argv[1]); 		/* set shape filename if provided */
    j=2;
  }
  else
    j=1;

  while(j<argc)
  {
    if (argv[j][0] != '-')
    {
      printf("\nWarning : %s - unrecognized argument\n", argv[j]);
      k=1;
    }
    else if ((ip=ispar(argv[j])) > 0)
    {
      setpar(ip, argv[j+1]);
      k=2;
    }
    else
    {
      for(i=1,k=1; argv[j][i] != '\0'; i++)
      {
        switch(argv[j][i])
        {
        case 'b': if(verb=='y') printf("Bloch simulator activated"); 
                  narg=k;
                  h.bls[0] = 'y', h.bls[1] = '\0';
                  while((k<=narg) && (j+k<argc) && (argv[j+k][0] != '-'))
                  {
                    if(verb=='y') printf(", option = %s, ", argv[j+k]); 
                    (void) strcpy(h.bls, argv[j+k]);
                    k++; 
                  }
                  if(verb=='y') printf("\n"); 
                  break;
        case 'c': if(verb=='y') printf("Calibrate only\n");
                  h.calflg = 1;
                  break;
        case 'f': if(verb=='y') printf("Input file %s\n", argv[j+k]); 
                  k++; 
                  break;
        case 'h': if(verb=='y') printf("Print wave header :\n"); 
                  while((j+k<argc) && (argv[j+k][0] != '-'))
                  {
                    e_str[0]='\0';
                    if(verb=='y') printf("%s \n", argv[j+k]); 
                    printcoms(e_str, argv[j+k], 'h');
                    printf("\n");
                    k++; 
                  }
                  exit(1);
        case 'i': if(verb=='y') printf("Print wave parameters :\n"); 
                  while((j+k<argc) && (argv[j+k][0] != '-'))
                  {
                    e_str[0]='\0';
                    if(verb=='y') printf("%s \n", argv[j+k]); 
                    printcoms(e_str, argv[j+k], 'i');
                    printf("\n");
                    k++; 
                  }
                  exit(1);
        case 'l': if(verb=='y') printf("ref_pw90 = "); 
                  narg=k;
                  while((k<=narg) && (j+k<argc) && (argv[j+k][0] != '-'))
                  {
                    if(verb=='y') printf("%s, ", argv[j+k]); 
                    (void) strcpy(h.pw90_ , argv[j+k]);
                    h.pw90=stod(argv[j+k]);
                    h.Bref = 250.0 / h.pw90;
                    k++; 
                  }
                  if(verb=='y') printf("\n"); 
                  break;
        case 'm': if(verb=='y') printf("maxincr = "); 
                  narg=k;
                  while((k<=narg) && (j+k<argc) && (argv[j+k][0] != '-'))
                  {
                    if(verb=='y') printf("%s, ", argv[j+k]); 
                    (void) strcpy(h.maxincr_, argv[j+k]);
                    if (isdigit(h.maxincr_[0]))
                    {
                      h.maxincr = stod(h.maxincr_);
                      if (fabs(h.maxincr) < 0.001)
                      h.maxincr = g.maxincr;
                    }                       
                    k++; 
                  }
                  if(verb=='y') printf("\n"); 
                  break;
        case 'o': printf("\n\tOptions :\n\t=======\n");
                  printf(" -b time (s)  - activate Bloch simulator, set simtime (sec)\n");
                  printf(" -c           - calibrate only, no shapefile created\n");
                  printf(" -f filename  - input filename\n");
                  printf(" -h wave      - print wave header\n");
                  printf(" -i wave      - print wave parameters\n");
                  printf(" -l ref_pw90  - length (us) of reference pw90 pulse\n");
                  printf(" -o           - list options\n");
                  printf(" -p ref_pwr   - reference power level (dB)\n");
                  printf(" -r filename  - re-shape Pbox pulse\n");
                  printf(" -s stepsize  - stepsize (us)\n");
                  printf(" -t wave      - print wave title\n");
                  printf(" -u userdir   - set userdir directory\n");
                  printf(" -w \"wavestr\" - wave data string\n");
                  printf(" -v           - print Pbox version; verbose\n");
                  printf(" -number      - sets reps = number (0-4)\n");
                  printf(" -x           - print Pbox parameters\n");
                  printf(" -param value - parameter name followed by its value\n");
                  exit(1);
        case 'p': if(verb=='y') printf("ref_pwr = "); 
                  narg=k;
                  while((k<=narg) && (j+k<argc) && (argv[j+k][0] != '-'))
                  {
                    if(verb=='y') printf("%s, ", argv[j+k]); 
                    (void) strcpy(h.rfpwr_ , argv[j+k]);
                    h.rfpwr=stoi(argv[j+k]);
                    k++; 
                  }
                  if(verb=='y') printf("\n"); 
                  break;
        case 'r': if(verb=='y') printf("Reshape file %s\n", argv[j+k]); 
                  k++; 
                  break;
        case 's': if(verb=='y') printf("stepsize = "); 
                  narg=k;
                  while((k<=narg) && (j+k<argc) && (argv[j+k][0] != '-'))
                  {
                    if(verb=='y') printf("%s, ", argv[j+k]);
                    (void) strcpy(h.stepsize_, argv[j+k]);
                    ip=sscanf(h.stepsize_, "%[0-9.]%c", val, &un);
                    if ((ip==1) && (isdigit(val[0])))
                      h.stepsize = stod(val);		/* default is us */
                    else if (ip > 1)
                    {
                      h.stepsize = setunits(h.stepsize_);
                      h.stepsize *= 1000000.0;  	/* convert to us */
                    }
                    if (h.stepsize > g.tmres) 
                    {
                      sprintf(h.maxincr_,"180.0");
                      h.maxincr = 180.0;
                    }
                    k++; 
                  }
                  if(verb=='y') printf("\n"); 
                  break;
        case 't': if(verb=='y') printf("Print wave title :\n"); 
                  while((j+k<argc) && (argv[j+k][0] != '-'))
                  {
                    e_str[0]='\0';
                    if(verb=='y') printf("%s \n", argv[j+k]); 
                    printcoms(e_str, argv[j+k], 't');
                    printf("\n");
                    k++; 
                  }
                  exit(1);
        case 'u': if(verb=='y') printf("userdir %s\n", argv[j+k]); 
                  k++; 
                  break;
        case 'v': if(argc == 2) 
                  {
                    printf("Pbox %s\n", REV);
                    exit(1); 
                  }
                  break;
        case 'w': if(verb=='y') printf("Wave(s) : \n"); 
                  opt='w';
                  h.itnf = 0;
                  inm=j+k; 
                  while((j+k<argc) && (argv[j+k][0] != '-') && (argv[j+k][0] != '\0'))
                  {
                    if(verb=='y') printf("%s, \n", argv[j+k]); 
                    h.itnf++;
                    k++; 
                  }
                  break;
        case 'x': printf("\n     Pbox Parameters :\n");
                  printf("     =================\n\n");
                  printf("The input parameters :\n");
                  printf("~~~~~~~~~~~~~~~~~~~~~~\n");
                  printf("name -     shape filename<.EXT>\n"); 	
                  printf("type -     shape type, r - RF, d - DEC, g - GRD\n");
                  printf("dres -     as in VNMR, deg\n");
                  printf("steps -    number of steps (< 64k)\n");
                  printf("stepsize - stepsize (us)\n");
                  printf("maxincr -  max phase incr, deg (<<180)\n");  
                  printf("attn -     attenuation, i (int-l), e (ext-l) or d (nearest dB)\n");
                  printf("sfrq -     spectrometer frequency, MHz\n");    
                  printf("refofs -   reference offset, Hz (/ppm)\n");
                  printf("sucyc -    supercycle, d (default), n (no), name\n");   	
                  printf("reps -     amount of reports (0-4) \n");
                  printf("header -   shape header, y (yes) n (no) i (imaging)\n");     
                  printf("bsim -     Bloch simulation, y (yes), n (no), a (add), s (sub),\n");
                  printf("           200 (time in sec)\n");
                  printf("T1 -       relax-n time T1 (sec)\n");
                  printf("T2 -       relax-n time T2 (sec) \n");
                  printf("dcyc -     duty cycle (0 - 1) \n");
                  printf("sw -       spectral width (Hz) \n"); 
                  printf("bscor -    Bloch-Siegert shift correction (y/n)\n"); 
                  printf("ptype -    pulse type (imaging only)\n");	
                  printf(" \n");
                  printf("Wavelib parameters : \n");
                  printf("~~~~~~~~~~~~~~~~~~~~ \n");
                  printf("amf -    (amplitude modulation function)\n");
                  printf("fmf -    (frequency modulation function)\n");
                  printf("pmf -    (phase modulation function)\n");
                  printf("su -     (default supercycle)\n");
                  printf("fla -    (default flipangle on resonance)\n");
                  printf("pwbw -   (pulsewidth to bandwidth product)\n");
                  printf("pwb1 -   (pulsewidth to B1max product)\n");
                  printf("pwsw -   (pulsewidth to sweepwidth product)\n");
                  printf("adb -    (adiabaticity on resonance)\n");
                  printf("ofs -    (offset of excitation bandwidth)\n");
                  printf("dres -   (default tipangle resolution, deg)\n");
                  printf("dash -   (dash variable)\n");
                  printf("c1 -     (constant)\n");
                  printf("c2 -     (constant)\n");
                  printf("c3 -     (constant)\n");
                  printf("steps -  (default number of steps)\n");
                  printf("min -    (minimum truncation threshold, 0-1)\n");
                  printf("max -    (maximum truncation threshold, 0-1)\n");
                  printf("left -   (truncation from left, 0-1)\n");
                  printf("right -  (truncation from right, 0-1)\n");
                  printf("cmplx -  (flag, retain real(1), imag(-1) or complex(0) part of wave)\n");
                  printf("wrap -   (wrap around factor, 0-1)\n");
                  printf("trev -   (time reversal flag, yes = 1, no = 0)\n");
                  printf("srev -   (sweep reversal flag, yes = 1, no = 0)\n");
                  printf("stretch - (stretching factor >= 0)\n");
                  printf("dcflag - (dc correction, y/n)\n");
                  printf(" \n");
                  printf("Additional data matrices are listed without parameter names. \n");
                  printf("One must only define the size of the data matrix given by :\n");
                  printf("cols - (number of columns)\n");
                  printf("raws - (number of rows)\n");
                  printf(" \n");
                  exit(1);
        case '0': h.reps=0; h.reps_[0]='0', h.reps_[1]='\0';
                  if(verb=='y') printf("reps=%d\n", h.reps);
                  break;
        case '1': h.reps=1; h.reps_[0]='1', h.reps_[1]='\0';
                  if(verb=='y') printf("reps=%d\n", h.reps);
                  break;
        case '2': h.reps=2; h.reps_[0]='2', h.reps_[1]='\0';
                  if(verb=='y') printf("reps=%d\n", h.reps);
                  break;
        case '3': h.reps=3; h.reps_[0]='3', h.reps_[1]='\0';
                  if(verb=='y') printf("reps=%d\n", h.reps);
                  break;
        case '4': h.reps=4; h.reps_[0]='4', h.reps_[1]='\0';
                  if(verb=='y') printf("reps=%d\n", h.reps);
                  break;
        default : printf("%s - bad option. \nFor list of options type : Pbox -o \n", 
                  argv[j]); exit(1);
        }
      }
    }
    j+=k;
  }

  Wv = (Wave *) calloc(h.itnf, sizeof(Wave)); 
  Sh = (Shape *) calloc(h.itnf, sizeof(Shape)); 
  for (i=0; i<h.itnf; i++) Sh[i].set = 0; 
  
  if (h.reps > 1)
  {
    printf("\n    Welcome to Pandora's Box!");
    printf("\n    %s\n\n",REV);  
    if (opt!='w')
    {
      printf("Reading input data from : \n \"%s\"...\n", ifn);
      if (h.reps > 3)
        reportheader();
    }
  }

  if (opt=='w')
  {
    if (h.itnf == 0)
      pxerr("Pbox input data error : no waves defined");
    for (i = 0; i < h.itnf; i++, inm++)
    {
      resetwave(&Wv[i]); 
      strcat(strcat(Wv[i].str, argv[inm]), " }"); 
      if (setwave(&Wv[i], h.tfl) == 0)
        pxerr("Pbox wave definition error : unrecognized shapename"); 
    }
    inm=0;
  }
  else
  {
    fseek(fil, 0, 0); 
    for (i = 0; i < h.itnf; i++)
    {
      if (! getwave(fil, &Wv[i], h.tfl))
      {
        printf("\n error in wave %d definition. Aborting...\n", i+1);
        fclose(fil); exit(1);
      }
    }
    inm = getnm(fil, '$'); 
    fclose(fil);				     /* close the .inp file */
  }

/* ------------------------------- Input Closed ----------------------------*/

  if (h.name[0]=='c' && h.name[1]=='a' && h.name[2]=='l' && 
      (h.name[3]=='\0' || h.name[3]=='.'))
    h.calflg = 1;					/* calibration flag */

  if ((h.bls[0] != 'a') && (h.bls[0] != 's'))
    i = remove(strcat(strcpy(e_str, shapes), FN_SIM));

  (void) strcpy(h.fname, shapes); 
  (void) strcat(h.fname, h.name);  
  for(i=0; i<h.itnf; i++)
    loadshape(i);   
  setshapes(); 
  if (inm) resetpars(ifn, inm); 
  col[0] = '\0';
  closepbox(); 

  if (bl.time > 0)
  {
    if (h.reps > 1)
      printf("Bloch simulation ... \n");
    if(bl.rdc > 0.001)
      bloch_rd(&bl);
    else
      bloch(&bl);
  }

  if (h.reps > 1)
    printf("\nDone.\n");

  return (0);
}

