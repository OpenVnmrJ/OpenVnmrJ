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
/* Pxfid.c - convert pulse file to fid text file
   Eriks Kupce / Rolf Kyburz, Dec 9, 1994 */

/* modified, 1994/12/09, R.Kyburz:
        - adjusted for VNMR pathnames and installation in bin directory
        - adjusted for full VNMR shape/pattern file variability
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <ctype.h>
#include <string.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

#define FNOUT "pbox.fid"
#define SHAPELIB "/vnmrsys/shapelib/"
#define SYSSHAPELIB "/vnmr/shapelib/"

#define PI M_PI
#define MAXSTR 512

int main(int argc, char *argv[])
{
  FILE *inpf, *outf;
  int i, j, k, nm, tokens, np, gt;
  float am, ph, fgt, re, im, ln, numb, tpa;
  double rd = PI / 180.0;
  char ch, str[128];
  char ifn[MAXSTR], ofn[MAXSTR];
  static char ext[10];

  /*----------------------------+
  | check for filename argument |
  +----------------------------*/
  if (argc < 2)
  {
    printf("Usage: Pxfid filename.extension\n");
    exit(1);
  }

  /*-----------------------------+
  | check for filename extension |
  +-----------------------------*/
  if (strchr(argv[1], '.') == NULL)	/* check for extension */
  {
    printf("Usage: Pxfid filename.extension\n");
    exit(1);
  }
  nm = strlen(argv[1]);
  for (j = 0; j < 3; j++)
  {
    k = nm - 3 + j;
    ext[j] = argv[1][k];
  }
  ext[3] = '\0';

  /*-------------------------+
  | find and open input file |
  +-------------------------*/
  if ((inpf = fopen(argv[1], "r")) == NULL)	/* read file */
  {
    if (argv[1][0] == '/')
    {
      printf("Pxfid: cannot open input file %s\n", argv[1]);
      exit(1);
    }
    else
    {
      (void) strcpy(ifn, (char *) getenv("HOME"));
      (void) strcat(ifn, SHAPELIB);
      (void) strcat(ifn, argv[1]);
      if ((inpf = fopen(ifn, "r")) == NULL)	/* read file */
      {
        (void) strcpy(ifn, SYSSHAPELIB);
        (void) strcat(ifn, argv[1]);
        if ((inpf = fopen(ifn, "r")) == NULL)     /* read file */
        {
          printf("Pxfid: cannot find input file %s\n", argv[1]);
          exit(1);
        }
      }
    }
  }

  /*---------------------+
  | find number of steps |
  +---------------------*/
  np = 0;
  k = 0;
  while ((ch = getc(inpf)) != EOF)
  {
    if ((ch != ' ') && (ch != '\n') && (ch != '\t') && (ch != '#'))
      k++;
    if (ch == '#')
      fgets(str, MAXSTR, inpf);
    else if ((ch == '\n') && (k > 0))
    {
      np++;
      k = 0;
    }
  }
  if (np == 0)
  {
    printf("Pxfid: no shape / pattern data found in %s;\n", ifn);
    printf("   no output produced.\n");
    fclose(inpf);		/* close the INfile */
    exit(1);
  }
  else
    printf("   number of slices in shape: %d\n", np);
  fseek(inpf, 0, 0);

  /*-----------------+
  | open output file |
  +-----------------*/
  (void) strcpy(ofn, (char *) getenv("HOME"));
  (void) strcat(ofn, SHAPELIB);
  (void) strcat(ofn, FNOUT);
  if ((outf = fopen(ofn, "w")) == NULL)
  {
    printf("Pxfid: cannot open output file %s\n", ofn);
    fclose(inpf);
    exit(1);
  }


  /*--------------------------------------+
  | CONVERT INPUT FILE INTO FID TEXT FILE |
  +--------------------------------------*/

  /*---------------------------------------+
  |  .RF files (r.f. pulse shapes)	   |
  |  column 1: phase			   |
  |  column 2: amplitude (default: 1023)   |
  |  column 3: duration count (default: 1  |
  |  column 4: r.f. gate (default: on = 1) |
  +---------------------------------------*/
  if (strcmp(ext, ".RF") == 0)
  {
    numb = 0;
    for (i = 0; i < np; i++)
    {
      fgets(str, MAXSTR, inpf);
      while ((str[0] == '#') ||
             ((tokens = sscanf(str, "%f %f %f %f", &ph, &am, &ln, &fgt)) == 0))
        fgets(str, MAXSTR, inpf);
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
      if (tokens == 0)
      {
        printf("Pxfid: file format error in %s\n", ifn);
        fclose(inpf);
        fclose(outf);
	unlink(ofn);
        exit(1);
      }

      if (gt == 0)
	am = 0.0;
      else
	am += 1.0;
      re = am * cos(rd * ph);
      im = am * sin(rd * ph);

      for (j = 0; j < (int) ln; j++)
	fprintf(outf, "%5.0f %5.0f\n", re, im);
      numb += (int) ln;
    }
  }

  /*---------------------------------------+
  |  .DEC files (modulation pattern)	   |
  |  column 1: flip angle		   |
  |  column 2: phase			   |
  |  column 3: amplitude (default: 1023)   |
  |  column 4: r.f. gate (default: on = 1) |
  +---------------------------------------*/
  else if (strcmp(ext, "DEC") == 0)
  {
    j = fscanf(inpf, "%c %s %f %f %f %f %f\n", &ch, str, &ln, &ln, &ln, &ln, &am);
    if ((j == 7) && (ch == '#') && (str[0] == 'P') && (str[1] == 'b') && 
        (str[2] == 'o') && (str[3] == 'x'))
      tpa = ln;
    else
      tpa = 1.0;
    numb = 0;
    for (i = 0; i < np; i++)
    {
      fgets(str, MAXSTR, inpf);
      while ((str[0] == '#') ||
	     ((tokens = sscanf(str, "%f %f %f %f", &ln, &ph, &am, &fgt)) == 0))
        fgets(str, MAXSTR, inpf);
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
      if (tokens == 0)
      {
        printf("Pxfid: file format error in %s\n", ifn);
        fclose(inpf);
        fclose(outf);
	unlink(ofn);
        exit(1);
      }

      if (gt == 0)
	am = 0.0;
      else
	am += 1.0;
      re = am * cos(rd * ph);
      im = am * sin(rd * ph);

      ln = 0.001 + ln/tpa;
      for (j = 0; j < (int) ln; j++)
	fprintf(outf, "%8.0f %8.0f\n", re, im);
      numb += (int) ln;
    }
  }

  /*---------------------------------+
  |  .GRD files (gradient pattern)   |
  |  column 1: amplitude	     |
  |  column 2: duration (default: 1) |
  +---------------------------------*/
  else if (strcmp(ext, "GRD") == 0)
  {
    numb = 0;
    for (i = 0; i < np; i++)
    {
      fgets(str, MAXSTR, inpf);
      while ((str[0] == '#') ||
	     ((tokens = sscanf(str, "%f %f", &am, &ln)) == 0))
        fgets(str, MAXSTR, inpf);
      if (tokens < 2)
        ln = 1.0;
      if (tokens == 0)
      {
        printf("Pxfid: file format error in %s\n", ifn);
        fclose(inpf);
        fclose(outf);
	unlink(ofn);
        exit(1);
      }

      for (j = 0; j < (int) ln; j++)
	fprintf(outf, "%8.0f %8.0f\n", am, 0.0);
      numb += (int) ln;
    }
  }
  else
  {
    printf("Pxfid: Unrecognized file name extension\n");
    fclose(outf);
    fclose(inpf);
    exit(1);
  }

  /*-------------------------------------------------+
  | close files, report number of output data points |
  +-------------------------------------------------*/
  fclose(outf);		/* close the OUTfile */
  fclose(inpf);		/* close the INfile */
  if (numb == 0)
  {
    unlink(ofn);	/* don't produce output file if no data present */
    printf("   no output produced.\n");
  }
  else
    printf("   created output file with %6.0f complex points.\n", numb);
  return(0);
}
