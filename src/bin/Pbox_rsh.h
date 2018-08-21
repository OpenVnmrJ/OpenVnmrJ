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
/* Pbox_rsh.h - Pbox readshape function, returns number of steps */

#define MIN(A, B) ((A) < (B) ? (A) : (B))

double *Amp, *Pha;

readshape(inpf, ext, tpa)
FILE     *inpf;
char     *ext;
double   tpa;
{
  int j, k, gt, nn, tokens;
  char str[MAXSTR];
  double am, ph, ln, fgt, tmp;

  fseek(inpf, 0, 0);
  nn = 0;
  if (strcmp(ext, "RF") == 0)
  {
    while (fgets(str, MAXSTR, inpf))
    {
      if ((str[0] == '#') ||
          ((tokens = sscanf(str, "%lf %lf %lf %lf", &ph, &am, &ln, &fgt)) == 0))
        continue;
      if (tokens < 3)
        ln = 1.0;
      nn += (int) ln; 
    }
  }  
  else if (strcmp(ext, "GRD") == 0)
  {
    while (fgets(str, MAXSTR, inpf))
    {
      if ((str[0] == '#') ||
          ((tokens = sscanf(str, "%lf %lf", &am, &ln)) == 0))
        continue;
      if (tokens < 2)
        ln = 1.0;
      nn += (int) ln;
    }
  }  
  else if (strcmp(ext, "DEC") == 0)
  {
    tmp = 3600.0;
    while (fgets(str, MAXSTR, inpf))
    {
      if ((str[0] == '#') ||
          ((tokens = sscanf(str, "%lf %lf %lf %lf", &ln, &ph, &am, &fgt)) == 0))
        continue;
      if (tpa < 1.0)
        tmp = MIN(tmp, ln);
      nn += (int) ln;
    }
    if (tpa < 1.0) nn /= (int) (tpa = tmp + 0.1);
    else nn /= (int) tpa;
  }  
  else
  {
    printf("readshape: Unrecognized file name extension %s\n", ext);
    fclose(inpf);
    exit(1);
  }

  Amp = arry(nn); 
  Pha = arry(nn);

  j = 0;
  fseek(inpf, 0, 0);
  if (strcmp(ext, "RF") == 0)
  {
    while (fgets(str, MAXSTR, inpf))
    {
      if ((str[0] == '#') ||
          ((tokens = sscanf(str, "%lf %lf %lf %lf", &ph, &am, &ln, &fgt)) == 0))
        continue;
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

      for (k = 0; k < (int) ln; k++)
      {
	Amp[j] = am; 
	Pha[j] = ph; 
	j++;
      }
    }
  }
  else if (strcmp(ext, "DEC") == 0)
  {
    while (fgets(str, MAXSTR, inpf))
    {
      if ((str[0] == '#') ||
           ((tokens = sscanf(str, "%lf %lf %lf %lf", &ln, &ph, &am, &fgt)) == 0))
        continue;
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

      ln /= (int) tpa;
      for (k = 0; k < ln; k++)
      {
	Amp[j] = am;
	Pha[j] = ph;
	j++;
      }
    }
  }
  else if (strcmp(ext, "GRD") == 0)
  {
    while (fgets(str, MAXSTR, inpf))
    {
      if ((str[0] == '#') ||
           ((tokens = sscanf(str, "%lf %lf", &am, &ln)) == 0))
        continue;
      if (tokens < 2)
        ln = 1.0;

      for (k = 0; k < ln; k++)
      {
	Amp[j] = am;
	Pha[j] = 0.0;
	j++;
      }
    }
  }
  fclose(inpf);			/* close the INfile */

  return(nn);
}
