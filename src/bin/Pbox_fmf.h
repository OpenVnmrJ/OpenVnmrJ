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
 /* Pbox_fmf.h - Pbox FM functions. Return the pw*b1 product.
   The user functions (f1, f2 & f3) are dummy functions which can be used
   for testing new functions before they are implemented. Any modification 
   of this file will need the main (Pbox) programm be recompiled.
   *********************************************************************** */

void f1(s)		/* user #1 */
Shape *s;
{
  int j;

  for (j = 0; j < s->np; j++)
    Pha[j] = 0.0;
}

void f2(s)		/* user #2 */
Shape *s;
{
  int j;

  for (j = 0; j < s->np; j++)
    Pha[j] = 0.0;
}


void f3(s)		/* user #3 */
Shape *s;
{
  int j;

  for (j = 0; j < s->np; j++)
    Pha[j] = 0.0;
}


void ls(s)				/* linear freq sweep (chirp) */
Shape *s;
{
  int j = 0;
  double ko = 0.0;

  ko = 0.125 * pi2 * s->pwsw;		/* ko/2 */
  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Pha[j] = -ko * tm * tm;
  }
  s->pb1 = sqrt(fabs(s->pwsw) * s->adb);
}


void lzs(s)				/* CA Lorentzian sweep */
Shape *s;
{
  int     j;
  double  b, lambda;

  b = sqrt(1.0 / s->min - 1.0);
  lambda = 0.5 * M_PI * s->pwsw / (b * s->min + atan(b));   /* lambda/2 */
  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Pha[j] = -lambda * tm * atan(b * tm);
  }
  s->pb1 = 2.0 * sqrt(fabs(lambda) * b * s->adb / M_PI);
}


void ht(s)							/* tanh */
Shape *s;
{
  int     j;
  double  tb, b, lambda; 

  b = 1.0 / s->min;
  b = log(b + sqrt(b*b - 1.0));              

  lambda = -0.5 * M_PI * s->pwsw / tanh(b);
  tb = lambda / b;
  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Pha[j] = tb * log(1.0/cosh(b * tm)); 
  }
  s->pb1 = sqrt(2.0 * fabs(lambda) * b * s->adb / M_PI);
}


void cs(s)					/* cos frequency sweep */
Shape *s;
{
  int j;
  double b;

  b = 0.5 * M_PI;
  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Pha[j] = -s->pwsw * (1.0 - cos(b * tm));
  }
  s->pb1 = sqrt(s->adb * fabs(s->pwsw));
}


void ccs(s)					/* ca cos frequency sweep */
Shape *s;
{
  int j;
  double b, b2, c;

  b = 0.5 * M_PI;
  b2 = b * b;
  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    c = cos(b * tm);
    Pha[j] = -s->pwsw * (1.0 + b2*tm*tm - c*c) / M_PI;
  }
  s->pb1 = sqrt(2.0 * s->adb * fabs(s->pwsw));
}

void cs2(s)					/* ca cos^2 frequency sweep */
Shape *s;
{
  int j;
  double b, b2, c, c2;

  b = 0.5 * M_PI;
  b2 = 24.0 * b * b;
  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    c = 16.0 * cos(M_PI * tm);
    c2 = cos(pi2 * tm);
    Pha[j] = -s->pwsw * (17.0 + b2*tm*tm - c - c2) / (24.0 * M_PI);
  }
  s->pb1 = 2.0 * sqrt(2.0 * s->adb * fabs(s->pwsw) / 3.0);
}


void ca(s)					/* CA phase modulation */
Shape *s;
{
int    i, j, k;
double ph=0.0, ko=0.0;

  if(s->c2 < 0.001) s->c2 = 2.0;               /* set default to 2.0 */

  if (imx < 0) k = s->np / 2;
  else
  {
    for (j = 0, k = 0; j < s->np; j++)
    {
      if (Amp[j] > ko) k = j, ko = Amp[j];      /* find the maximum */
    }
  }
  Pha[0] = Amp[0]*Amp[0];	   		/* find k(t) */
  for(j = 1; j < s->np; j++) 
  {
    Pha[j] = pow(Amp[j],s->c2);
    Amp[j-1] = Amp[j]; 
  }
  Pha[0] -= (ph = Pha[0]);                      /* remove dc-offset */
  for (i = 1; i < s->np; i++)	   		/* integrate */
    Pha[i] = Pha[i-1] + (Pha[i] - ph);
  s->np--;
  ko = (double) s->np * s->pwsw * (Pha[k] - Pha[k-1]) / Pha[s->np-1];
  tm = 0.5 * Pha[s->np-1];			/* max frequency, not scaled */
  j = 0;
  while (tm > Pha[j]) j++;
  ph = Pha[j-1];                                /* freq correction (offset) */
  for (i = 1; i < s->np; i++)			/* integrate */	
    Pha[i] = Pha[i-1] + (Pha[i] - ph);
  s->np--;
  ph = Pha[j-1];                                /* phase correction (offset) */
  tm = s->pwsw * M_PI / ((double) s->np * tm);
  for(i = 0; i < s->np; i++) 
    Pha[i] = -tm * (Pha[i] - ph);
  s->pb1 = sqrt(fabs(ko) * s->adb); 
}


void osm(s)					/* Optimized Spin Mixing */
Shape *s;
{
  int     i, j, k;
  double  ko=0.0, ph = 0.0;
  
  if(imx < 0) k = s->np / 2, ko = Amp[k]; 
  else
  {
    for (j = 0, k = 0; j < s->np; j++)
    {
      if (Amp[j] > ko) k = j, ko = Amp[k];      /* find the maximum */
    }
  }  
  
  for(j = 0; j < k; j++) 
    Pha[j] = Amp[j]-ko;
  for(j = k; j < s->np; j++) 
    Pha[j] = ko-Amp[j];
                       
  tm = Pha[s->np] - Pha[0];			/* max frequency, not scaled */
  ko = (double) s->np * s->pwsw * (Pha[k] - Pha[k-1]) / tm;
  s->pb1 = sqrt(fabs(ko * s->adb)); 
  if(s->pb1 < 2.0)
    s->pb1 = 0.0, s->adb = 0.0;
  for (i = 1; i < s->np; i++)			/* integrate */	
    Pha[i] = Pha[i-1] + Pha[i];
  s->np--; 
  ph = Pha[k];                                /* phase correction (offset) */
  tm = -2.0*(s->pwsw * M_PI) / ((double) s->np * tm);
  for(i = 0; i < s->np; i++) 
    Pha[i] = tm * (Pha[i] - ph);
}


void fm2pm(s)					/* FM to PM conversion */
Shape *s;
{
int     i, j, k;
double  ko=0, ph = 0.0;
  
  if((s->adb > 0.0) && (s->pwsw > 0.0))
  {
    if(imx < 0) k = s->np / 2; 
    else
    {
      for (j = 0, k = 0; j < s->np-1; j++)
      {
        if (Amp[j] > ko) k = j, ko = Amp[j];    /* find the maximum */
      }
    }                         
    tm = Pha[s->np-1] - Pha[0];			/* max frequency, not scaled */
    ko = (double) s->np * s->pwsw * (Pha[k] - Pha[k-1]) / tm;
    s->pb1 = sqrt(fabs(ko * s->adb)); 
    tm = tm*0.5 + Pha[0];			/* middle of sweep */
    j = 0;
    while (tm > Pha[j]) j++;
    tm = Pha[s->np-1] - Pha[0];			/* max frequency, not scaled */
  }
  for (i = 1; i < s->np; i++)			/* integrate */	
    Pha[i] = Pha[i-1] + Pha[i];
  s->np--;
  
  if((s->adb > 0.0) && (s->pwsw > 0.0))
  {
    ph = Pha[j-1];                                /* phase correction (offset) */
    tm = -2.0*(s->pwsw * M_PI) / ((double) s->np * tm);
  }
  else
    ph = 0.0, tm = (double) -2.0/s->np;
    
  for(i = 0; i < s->np; i++) 
    Pha[i] = tm * (Pha[i] - ph);
}


void tns(s)				/* tan freq sweep */
Shape *s;
{
  int j;
  double lambda, b, c;

  b = 0.5 * M_PI * s->c1;
  lambda = 0.5 * M_PI * s->pwsw / tan(b);
  c = -lambda / b; 
  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Pha[j] = c * log(cos(b * tm));
  }
  s->pb1 = sqrt(2.0 * fabs(lambda) * b * s->adb / M_PI);
}

void ats(s)				/* arctan sweep*/
Shape *s;
{
  int     j;
  double  b, lambda, b2, ta;


  b2 = 1.0 / (s->min * s->min) - 1.0;
  b = sqrt(b2);
  lambda = 0.5 * M_PI * s->pwsw / atan(b); 
  ta = 0.5 * lambda / b;
  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Pha[j] = -lambda * tm * atan(b * tm) - ta * log(1.0 + b2 * tm * tm);;
  }
  s->pb1 = sqrt(2.0 * fabs(lambda) * b * s->adb / M_PI);
}


void cas(s)			/* constant adiab-ty sweep (CAP) */
Shape *s;
{
  int j;
  double c;	

  c = pi2 * s->adb;
  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Pha[j] = -c * (1.0 - sqrt(1.0 - tm * tm));
  }
  s->pb1 = 2.0 * s->adb;
}


void fsw(s)				/* freq switched */
Shape *s;
{
  int i, j, k;
  double ph = 0.0, ln = 0.0;

  k = s->m * s->n;
  for (i = 0, j = 0; i < k; i += s->n)
  {
    if (s->n > 2) ph += s->f[i+2];
    for (tm = 0.0; tm < s->f[i+1]; tm += s->dres, j++)
      Pha[j] = rd * (ph + tm * s->f[i]); 
    ph += tm * s->f[i];
    ln += tm;
  }

  if (s->n > 2) ph = Pha[0] - rd*s->f[2];
  else ph = Pha[j/2];

  for (i = 0; i < j; i++)
    Pha[i] -= ph;
  s->pb1 = ln / 360.0;
}

/*
void fslg(s)				 * FSLG * 
Shape *s;
{
  int i, j, k;
  double ph0, phi, phd;	

  phd = pboxRound(360.0*s->pwb/(double) s->np, 1.0);
  phi = rd*phd;
  ph0 = phi/2.0;
  k = s->np / 2;
  for (j = 0; j < k; j++)		  * first half *  
    Pha[j] = ph0 + (double) j*phi;
  ph0 = Pha[j-1] - M_PI;
  for (i = 0, j = k; j < s->np; j++, i++)    * second half *  
    Pha[j] = ph0 - (double) i*phi;

  s->pb1 = (double) s->np * phd * tan(rd*s->fla) / 360.0;
}
*/
  
void fslg(s)				/* FSLG - phase not rounded */
Shape *s;
{
  int i, j, k;
  double ph0, phi;	

  phi = pi2 * s->pwb / (double) s->np;
  ph0 = phi/2.0;
  k = s->np / 2;
  for (j = 0; j < k; j++)		 /* first half */ 
    Pha[j] = ph0 + (double) j*phi;
  ph0 = Pha[j-1] - M_PI;
  for (i = 0, j = k; j < s->np; j++, i++)   /* second half */ 
    Pha[j] = ph0 - (double) i*phi;

  s->pb1 = 2.0 * sin(rd*s->fla);
}
 

void sqw(s)				/* sq waves */
Shape *s;
{
  int i, j, k;
  double  ln = 0.0;

  tm = 0.0;
  if (s->n == 0)
  {
    tm = s->c1 * rd;
    k = s->dnp/2;
    for (i = 0, j = s->dnp-1; i < k; i++, j--) 
      Pha[i] = tm, Pha[j] = -tm;
  }
  if (s->n == 1)
  {
    for (i = 0; i < s->m; i++) Pha[i] = s->f[i] * rd;
  }
  else if (s->n == 2)
  {
    k = s->m * s->n;
    for (j = 0, i = 0; i < k; i+= s->n)
    {
      tm = 0.0;
      while (tm < s->f[i+1])
      { 
        Pha[j] = s->f[i] * rd;
        tm += s->dres; 
        j++;
      }
      ln += tm;
    }
    s->pb1 = ln / 360.0;
  }
  else if (s->n == 3)
  {
    k = s->m * s->n;
    for (j = 0, i = 0; i < k; i += s->n)
    {
      tm = 0.0;
      while (tm < s->f[i+1])
      { 
        Pha[j] = s->f[i] * rd;
        tm += s->dres; 
        j++;
      }
    }
  }
}
