/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif
/* 
 */
/* Pxspy.c - Pbox spy, 8 Dec 1994, Eriks Kupce
   Converts a shaped pulse (*.RF, *.DEC and *.GRD) into Fourier series and
   produces an output file which can be included directly into the Pbox 
   library (wavelib). For example, this program can be used to convert hard  
   pulse decoupling sequences into soft ("cool") decoupling waveforms. 
   The Fourier coefficients may slightly depend on the number of points
   in the waveform. 
   Ref.: E.Kupce, J.Boyd & I.Campbell, JMR 110A (1994) 109.
   Updated for Pbox 5.2 on 29.10.96.
*/

/* modified, 1994/12/12, R.Kyburz:
	- adjusted for VNMR pathnames and installation in bin directory
	- adjusted for full VNMR shape/pattern file variability
	- made interactive input safer (defaults upon return)
   modified, 2009/06/24, E. Kupce:
        - corrected the Fourier analysis.
*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#define WAVELIB "/vnmrsys/wavelib/usr/"
#define SHAPELIB "/vnmrsys/shapelib/"
#define SYSSHAPELIB "/vnmr/shapelib/"

#define MAXSTR 512
#define PI M_PI
#define SWAP(a,b) dm=(a); (a)=(b); (b)=dm
#define MAX(A, B) ((A) > (B) ? (A) : (B))

main(argc, argv)
int argc;
char *argv[];
{
  FILE *inpf, *outf;
  char ch, ext[10], str[MAXSTR], name[MAXSTR], ifn[MAXSTR], ofn[MAXSTR];
  int i, j, k, ip, nc, nr, np, gt, nm, ofs, tokens;
  double am, ph, ln, re, im, cs, sn, ax, bx, dm, fgt, fla;
  double *Re, *Im, *an, *bn;
  double rd = PI / 180.0;
  void   d_ft(), f_ft(), re_order();

  /*-----------------------------+   
  | check for filename extension |
  +-----------------------------*/
  if ((argc < 2) || (strchr(argv[1], '.') == NULL))     /* check for extension */
  {
    printf("Usage: Pxspy filename.extension\n");
    exit(1);
  }
  nm = strlen(argv[1]);
  for (j = 0; j < 3; j++)
  {
    k = nm - 3 + j;
    ext[j] = argv[1][k];
  }
  ext[j] = '\0';

  /*-------------------------+
  | find and open input file |
  +-------------------------*/
  if ((inpf = fopen(argv[1], "r")) == NULL)     /* read file */
  {
    if (argv[1][0] == '/')
    {
      printf("Pxspy: cannot open input file %s\n", argv[1]);
      exit(1);
    }
    else
    {
      (void) strcpy(ifn, (char *) getenv("HOME"));
      (void) strcat(ifn, SHAPELIB);
      (void) strcat(ifn, argv[1]);
      if ((inpf = fopen(ifn, "r")) == NULL)     /* read file */
      {
        (void) strcpy(ifn, SYSSHAPELIB);
        (void) strcat(ifn, argv[1]);
        if ((inpf = fopen(ifn, "r")) == NULL)     /* read file */
        {
          printf("Pxspy: cannot find input file %s\n", argv[1]);
          exit(1);
        }
      }
    }
  }
  printf("Looking into %s ...\n", argv[1]);

  /*---------------------+
  | find number of steps |
  +---------------------*/
  nr = 0;
  k = 0;			/* find number of steps */
  while ((ch = getc(inpf)) != EOF)
  {
    if ((ch != ' ') && (ch != '\n') && (ch != '\t') && (ch != '#'))
      k++;
    if (ch == '#')
      fgets(str, MAXSTR, inpf);
    else if ((ch == '\n') && (k > 0))
    {
      nr++;
      k = 0;
    }
  }
  if (nr == 0)
  {
    printf("Pxspy: no shape / pattern data found in %s;\n", ifn);
    printf("   no output produced.\n");
    fclose(inpf);               /* close the INfile */
    exit(1);
  }


  /*---------------------------+
  | FIND NUMBER OF TIME SLICES |
  +---------------------------*/
 
  /*---------------------------------------+
  |  .RF files (r.f. pulse shapes)         |
  |  column 3: duration count (default: 1  |
  +---------------------------------------*/
  fseek(inpf, 0, 0);
  np = 0;
  if (strcmp(ext, ".RF") == 0)
  {
    while(fgets(str, MAXSTR, inpf))
    {
      if((str[0] != '#') && (sscanf(str, "%lf", &ph) > 0)) 
      {
        tokens = sscanf(str, "%lf %lf %lf %lf", &ph, &am, &ln, &fgt);       
        if (tokens < 3) ln = 1.0;
        np += (int) ln;   
      }     
    } 
  }  
  else if (strcmp(ext, "DEC") == 0)
  {
  
  /*---------------------------------------+
  |  .DEC files (modulation pattern)       |
  |  column 1: flip angle                  |
  +---------------------------------------*/

    while(fgets(str, MAXSTR, inpf))
    {
      if((str[0] != '#') && (sscanf(str, "%lf", &ln) > 0)) 
        np += (int) ln;
    }
  }  
  else if (strcmp(ext, "GRD") == 0)
  {

  /*---------------------------------+
  |  .GRD files (gradient pattern)   |
  |  column 2: duration (default: 1) |
  +---------------------------------*/

    while(fgets(str, MAXSTR, inpf))
    {
      if((str[0] != '#') && (sscanf(str, "%lf", &am) > 0)) 
      tokens = sscanf(str, "%lf %lf", &am, &ln);       
      if (tokens < 2)
        ln = 1.0;
      np += (int) ln;
    }
  }  
  else
  {
    printf("Pxspy: %s - unrecognized file name extension\n", ext);
    fclose(inpf);
    exit(1);
  }

  /*----------------------------------------+
  | allocate space for Fourier coefficients |
  +----------------------------------------*/
  Re = (double *) calloc(np, sizeof(double));
  Im = (double *) calloc(np, sizeof(double));


  /*----------------------------------------+
  | COLLECT DATA FROM SHAPE / PATTERN FILES |
  +----------------------------------------*/

  /*---------------------------------------+
  |  .RF files (r.f. pulse shapes)         |
  |  column 1: phase                       |
  |  column 2: amplitude (default: 1023)   |
  |  column 3: duration count (default: 1  |
  |  column 4: r.f. gate (default: on = 1) |
  +---------------------------------------*/
  fseek(inpf, 0, 0);
  ax = bx = dm = 0.0; j = 0;
  if (strcmp(ext, ".RF") == 0)
  {
    while((fgets(str, MAXSTR, inpf)) && (j < np))
    {
      if((str[0] != '#') && (sscanf(str, "%lf", &ph) > 0)) 
      {
        tokens = sscanf(str, "%lf %lf %lf %lf", &ph, &am, &ln, &fgt);       
        if (tokens < 4)
        gt = 1;
        
        if (tokens < 4)
          gt = 1;
        else
        {   
          gt = (int) fgt;
          gt &= 1;                /* don't pick spare gates (2/4) */
        }
        if (tokens < 3)
          ln = 1.0;
        if (tokens < 2)
          am = 1023.0;
 
        if (gt == 0)
          am = 0.0;
        else
          am += 1.0;

        ph *= rd;
        cs = cos(ph);
        sn = sin(ph);
        re = am * cs;
        im = am * sn;  
        ax += fabs(cs) * ln;
        bx += fabs(sn) * ln;

        for (k = 0; k < (int) ln; k++)
        {
	  Re[j] = re;
	  Im[j] = im;
	  j++;
        }
      }
    }
  }
  else if (strcmp(ext, "DEC") == 0)
  {
  
  /*---------------------------------------+
  |  .DEC files (modulation pattern)       |
  |  column 1: flip angle                  |
  |  column 2: phase                       |
  |  column 3: amplitude (default: 1023)   |
  |  column 4: r.f. gate (default: on = 1) |
  +---------------------------------------*/

    while((fgets(str, MAXSTR, inpf)) && (j < np))
    {
      if((str[0] != '#') && (sscanf(str, "%lf", &ln) > 0)) 
      {
        tokens = sscanf(str, "%lf %lf %lf %lf", &ln, &ph, &am, &fgt);
        if (tokens < 4)
          gt = 1;
        else
        {   
          gt = (int) fgt;
          gt &= 1;                /* don't pick spare gates (2/4) */
        }
        if (tokens < 3)
          am = 1023.0;
        if (tokens < 2)
          ph = 0.0;

        if (gt == 0)
          am = 0.0;
        else
          am += 1.0;

        ph *= rd;
        cs = cos(ph);
        sn = sin(ph);
        re = am * cs;
        im = am * sn;
        ax += fabs(cs) * ln;
        bx += fabs(sn) * ln;

        for (k = 0; k < ln; k++)
        {
	  Re[j] = re;
	  Im[j] = im;
	  j++;
        }
      }
    }
  }
  else if (strcmp(ext, "GRD") == 0)
  {

  /*---------------------------------+
  |  .GRD files (gradient pattern)   |
  |  column 1: amplitude             |
  |  column 2: duration (default: 1) |
  +---------------------------------*/

    while((fgets(str, MAXSTR, inpf)) && (j < np))
    {
      if((str[0] != '#') && (sscanf(str, "%lf", &am) > 0))
      { 
        tokens = sscanf(str, "%lf %lf", &am, &ln);       
      
        if (tokens < 2)
          ln = 1.0;

        for (k = 0; k < ln; k++)
        {
	  Re[j] = am;
	  Im[j] = 0.0;
	  j++;
        }
      }
    }
  }
  fclose(inpf);			/* close the INfile */

  if (strcmp(ext, "GRD") == 0)
    gt = 0;
  else
  {
    ax /= (double) np;
    bx /= (double) np;
    if (bx < 0.001)
      gt = 0;			/* real only, imaginary part = 0 */
    else if (ax < 0.001)
      gt = 1;			/* imaginary only, real part = 0 */
    else
      gt = -1;
  }


  /*----------------------------------+
  | User input for output file header |
  | and Fourier analysis:	      |
  +----------------------------------*/

  /*----------------------------------------+
  | pattern name			    |
  | can also be entered with comment, like  |
  |   xyz : xyz shape for refocusing pulses |
  +----------------------------------------*/
  printf("\n  Enter the new shape/pattern name: ");
  i = 0; j = 0;
  while ((i < MAXSTR) && ((ch = getchar()) != '\n'))
  {
    name[i++] = ch;
    if (((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) ||
        ((ch >= '0') && (ch <= '9')))
      j++;
  }
  name[i] = '\0';
  if (j == 0)
  {
    strcpy(name, "new");
    printf("     shape/pattern name assumed to be 'new'.\n");
  }

  /*--------------------------------------+
  | on-resonance flip angle		  |
  | used for scaling Fourier coefficients |
  | 0 selects default of 90 degrees, as   |
  |    this would lead to zero coefs	  |
  +--------------------------------------*/
  printf("  on-resonance flip angle in degees (default = 90) : ");
  i = 0;
  while ((i < MAXSTR) && ((ch = getchar()) != '\n'))
    str[i++] = ch;
  str[i] = '\0';
  i = sscanf(str, "%lf", &am);
  if ((i == 0) || (i == EOF) || (am == 0.0))
  { 
    am = 90.0;
    printf("     assuming default of %3.1f degrees.\n", am);
  }
  am = (fla=am)/360.0;

  /*---------------------------+
  | pw*bw product              |
  | used for definition header |
  +---------------------------*/
  printf("  product of pulsewidth and bandwidth (default = 5.0) : ");
  i = 0;
  while ((i < MAXSTR) && ((ch = getchar()) != '\n'))
    str[i++] = ch;
  str[i] = '\0';
  i = sscanf(str, "%lf", &ln); 
  if ((i == 0) || (i == EOF))
  {    
    ln = 5.0;
    printf("     assuming pw*bw = 5.0\n"); 
  } 

  /*-------------------------------+
  | number of Fourier coefficients |
  | default: 10; 0 selects default |
  +-------------------------------*/
  printf("  number of Fourier coefficients (default = 10) : ");
  i = 0;
  while ((i < MAXSTR) && ((ch = getchar()) != '\n'))
    str[i++] = ch;
  str[i] = '\0';
  i = sscanf(str, "%d", &nc); 
  if ((i == 0) || (i == EOF) || (nc < 1))
  {    
    nc = 10;
    printf("     assuming %d Fourier coefficients\n", nc); 
  } 

  printf("  Relative offset in units of pw*bw (default = 0) : ");
  i = 0;
  while ((i < MAXSTR) && ((ch = getchar()) != '\n'))
    str[i++] = ch;
  str[i] = '\0';
  i = sscanf(str, "%lf", &dm); 
  ofs = (int) dm;
  if ((i == 0) || (i == EOF))
  {    
    ofs = 0;
    printf("     assuming %d offset\n", ofs); 
  } 

  an = (double *) calloc(nc, sizeof(double));  
  bn = (double *) calloc(nc, sizeof(double));

  /*-----------------+
  | Fourier analysis |
  +-----------------*/
  if (gt < 0)    
  {
    i = 1;
    while(i<nc) i*=2;  
    if(i-nc) d_ft(&Re, &Im, nc);
    else     f_ft(&Re, &Im, nc);     
    re_order(&Re, &Im, nc);     
    for(i=0; i<nc; i++) an[i] = Re[i], bn[i] = Im[i];    
  }
  else
  {
    bx = 2.0 * M_PI / (double) np;
    for (i = 0; i < nc; i++)	/* Fourier Series */
    {
      re = bx * (double) i;  
      for (j = 0; j < np; j++)
      {
        ph = re * (double) j;
        cs = cos(ph);
        sn = sin(ph);
        an[i] += Re[j] * cs + Im[j] * sn;
        bn[i] += Im[j] * cs - Re[j] * sn;
      }
      an[i] /= (double) np;
      bn[i] /= (double) np;
    }
  }

  dm = 0.0;
  for(i=0; i<nc; i++)  /* scale the waveform */
  {
    ax = sqrt(an[i]*an[i] + bn[i]*bn[i]);
    if (dm < ax) dm = ax;
  }
    
  for(i=0; i<nc; i++) an[i] /= dm, bn[i] /= dm;

  if (gt > 0)
  {
    for (i = 0; i < nc; i++)
    {
      SWAP(an[i], bn[i]);
      bn[i] = -bn[i];
    }
    gt = 0;
    i = 0;
  }
  else i = nc/2;

  i += ofs;
  if ((dm = sqrt(an[i]*an[i] + bn[i]*bn[i])) > 0.001)
    am /= dm;
  else
    am = 1.0;

  for (i = 0; i < nc; i++)
  {
    an[i] *= 2.0 * am;
    bn[i] *= -2.0 * am;  
  }

  if (gt == 0)
    an[0] /= 2.0;

  /*-----------------+  
  | open output file |
  +-----------------*/
  (void) strcpy(ofn, (char *) getenv("HOME"));
  (void) strcat(ofn, WAVELIB);
  (void) strcat(ofn, name);
  
  if ((outf = fopen(ofn, "w")) == NULL)
  {
    (void) strcpy(ifn, (char *) getenv("HOME"));
    sprintf(str, "mkdir %s/vnmrsys/wavelib\n", ifn);
    system(str);  
    sprintf(str, "mkdir %s/vnmrsys/wavelib/usr\n", ifn);
    system(str);     
    if ((outf = fopen(ofn, "w")) == NULL)
    {
      printf("Pxspy: cannot open output file %s\n", ofn);
      exit(1);
    }
  }
  
  fprintf(outf, "# %s\n", name);
  fprintf(outf, "%c \n", '%');
  fprintf(outf, "#	Shape Parameters \n");
  fprintf(outf, "#       ---------------- \n");
  if (gt == 0)
    fprintf(outf, "amf 	= fs	(amplitude modulation function) \n");
  else
    fprintf(outf, "amf 	= ft	(amplitude modulation function) \n");  
  fprintf(outf, "fla 	= %.1f	(default flipangle on resonance) \n", fla);
  fprintf(outf, "pwbw 	= %.3f	(pulsewidth to bandwidth product) \n", ln);
  if(ofs)
    fprintf(outf, "ofs     = %d	(calibration offset, in rel. units)\n", ofs);
  fprintf(outf, "steps 	= %d	(default number of steps) \n", np);
  fprintf(outf, " \n");
  fprintf(outf, "#	Fourier Coefficients \n");
  fprintf(outf, "#       -------------------- \n");
  fprintf(outf, "cols 	= 2 (columns) \n");
  fprintf(outf, "rows 	= %d (rows) \n", nc);
  fprintf(outf, "   \n");
  fprintf(outf, "#        a(n)    b(n) \n");
  fprintf(outf, "#        ------------ \n");
  for (i = 0; i < nc; i++)
    fprintf(outf, "       %8.4f %8.4f\n", an[i], bn[i]);  
  fclose(outf);			
  printf("\nShape definition written in\n");
  printf("        %s\n\n", ofn);
  return(0);
}



void re_order(xRe, xIm, np)
int   np;
double *xRe[], *xIm[];
{
  int i, j, k;
  double *tRe, *tIm;

  tRe = (double *) calloc(np, sizeof(double));
  tIm = (double *) calloc(np, sizeof(double));

  k = np/2 + np%2; 
  for(i=0; i<np; i++) 
  {
    j = (i + k)%np; 
    tRe[i] = (*xRe)[j]; tIm[i] = (*xIm)[j];  
  }
  for(i=0; i<np; i++) 
    (*xRe)[i] = tRe[i], (*xIm)[i] = tIm[i];
    
  free(tRe); free(tIm);
}


void f_ft(xRe, xIm, np)
int  np;
double *xRe[], *xIm[];
{
  int	  i, j, k, m, istep;	
  double  tRe, tIm, cs, sn, wr, wi, teta, tm, dm;

  j = 0;					/* bit-reversal */
  for (i=0; i<np; i++)		
  {
  
    if (j > i) 	  
    {
      SWAP((*xRe)[j], (*xRe)[i]);
      SWAP((*xIm)[j], (*xIm)[i]); 
    }
    
    m = np >> 1;
    while (m>=2 && j>=m)
    {
      j-=m; 
      m>>=1;
    }
    j += m;
  }
  k = 1;					/* Danielson - Lanczos lemma */
  while (np > k)  
  {
    istep = 2*k;
    teta = M_PI/k;
    cs = cos(teta);
    sn = sin(teta);
    wr = 1.0, wi = 0.0;
    for (m=0; m<k; m++) 
    {
      for (i=m; i<np; i+=istep) 
      {
        j = i + k;
        tRe = wr*(*xRe)[j] + wi*(*xIm)[j];
        tIm = wr*(*xIm)[j] - wi*(*xRe)[j];
        (*xRe)[j] = (*xRe)[i] - tRe;	 
        (*xIm)[j] = (*xIm)[i] - tIm;
        (*xRe)[i] += tRe;
        (*xIm)[i] += tIm;
      }
    wr = (tm=wr)*cs + wi*sn;
    wi = wi*cs - tm*sn;
    }
    k = istep;
  }
}

void d_ft(xRe, xIm, np)				/* discrete FT */
double   *xRe[], *xIm[];
int      np;
{
  int     i, j;
  double   *tRe, *tIm, cs, sn;
  double  cnst0, cnst1, cnst2; 

  tRe = (double *) calloc(np, sizeof(double));
  tIm = (double *) calloc(np, sizeof(double));

  for(i=0; i<np; i++) tRe[i] = 0.0, tIm[i] = 0.0;
  
  cnst0 = 2.0 * M_PI / (double) np;
  for(i=0; i<np; i++) 	
  { 
    cnst1 = cnst0 * (double) i;
    for(j=0; j<np; j++) 
    { 
      cnst2 = cnst1 * (double) j;
      cs = cos(cnst2);
      sn = sin(cnst2);
      tRe[i] += (*xRe)[j]*cs + (*xIm)[j]*sn;
      tIm[i] += (*xIm)[j]*cs - (*xRe)[j]*sn;
    }
  }

  for(i=0; i<np; i++) 
    (*xRe)[i] = tRe[i], (*xIm)[i] = -tIm[i];
    
  return;
}

