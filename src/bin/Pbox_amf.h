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
/* Pbox_amf.h - Pbox AM functions 
   The user functions (a1, a2 & a3) are dummy functions which can be used
   for testing new functions before they are implemented. Any modification 
   of this file will need the main (Pbox) programm be recompiled.
   *********************************************************************** */

/* - - - - - - - - - - - -  user functions a1, a2 & a3 - - - - - - - - - */

void a1(s)		/* user #1 */
Shape *s;
{
  int j;

  for (j = 0; j < s->np; j++)
    Amp[j] = 1.0;
}

void a2(s)		/* user #2 */
Shape *s;
{
  int j;

  for (j = 0; j < s->np; j++)
    Amp[j] = 1.0;
}

void a3(s)		/* user #3 */
Shape *s;
{
  int j;

  for (j = 0; j < s->np; j++)
    Amp[j] = 1.0;
}

/* - - - - - - - - - - Simple AM functions - - - - - - - - */

void sq(s)					/* constant amplitude */
Shape *s;
{
  int j;

  for (j = 0; j < s->np; j++)
    Amp[j] = 1.0;
}

void gs(s)					/* Gaussian */
Shape *s;
{
  int j;
  double c;
	
  c = log(s->min); 
  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Amp[j] = exp(c * tm * tm); 
  }
}


void lz(s)					/* Lorentzian */
Shape *s;
{
  int     j;
  double  c;

  c = 1.0/s->min - 1.0;
  for (j = 0; j < s->np; j++)
  { 
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Amp[j] = 1.0 / (1.0 + c * tm * tm);
  }
}


void sch(s)					/* sech */
Shape *s;
{
  int     j;
  double  c;   

  if (s->c1 == 0.0) s->c1 = 1.0;
  c = 1.0 / s->min;
  c = log(c + sqrt(c*c - 1.0));              
  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Amp[j] = 1.0/cosh(c * pow(tm, s->c1)); 
  }
}


void hta(s)					/* tanh amp */
Shape *s;
{
  int     j;
  double  a, c;   

  c = 1.0 / s->min;
  c = log(c + sqrt(c*c - 1.0));              
  a = s->c1*tanh(c*s->lft);
  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Amp[j] = a - tanh(c * tm); 
  }
}


void csp(s)					/* cosine power */
Shape *s;
{
  int j;
  double c;

  c = M_PI/2.0;
  for (j = 0; j < s->np; j++)
  {
    if (s->c1 != 0.0)
    {
      tm = s->lft - s->rgt * (double) j / (s->np - 1);
      Amp[j] = pow(cos(c * tm), s->c1);
    }
    else
      Amp[j] = 1.0;
  }
}


void sc(s)					/* sinc */
Shape *s;
{
  int j;
  double c;

  c = s->c1 * M_PI;
  for (j = 0; j < s->np; j++)
  {
    tm = c * (s->lft - s->rgt * (double) j / (s->np - 1));
    if (tm != 0.0)
      Amp[j] = sin(tm) / tm;
    else
      Amp[j] = 1.0;
  }
}


void  wr(s) 					/* WURST */
Shape *s;
{
  int     j;
  double  c;

  c = M_PI/2.0;
  for(j = 0; j < s->np; j++)       
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Amp[j] = 1.0 - fabs(pow(sin(c*tm), s->c1));
  }
}


void    sed(s)                                  /* seduce-1 */          
Shape *s;
{
  int     j;
  double  c;
 
  for (j = 0; j < s->np; j++) 
  {             
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    c = tanh(0.04*fabs(tm));
    c = 1.0/cosh(2500*tm*c*c);
    Amp[j] = c * (1.0 + cos(M_PI*tm));
  }
}

void    qp(s)                               /* quadrupolar pulse */        
Shape *s;
{
  int     j;
  double  c;

    c = -0.5*sqrt(40.0); 
  for (j = 0; j < s->np; j++) 
  { 
    tm = c * (s->lft - s->rgt * (double) j / (s->np - 1));
    Amp[j] = (1.0 - 7.05*tm - 0.225*tm*tm + 11.0*tm*tm*tm) * exp(-tm*tm);
  }
}

void ata(s)					/* CA atan amplitude */
Shape *s;
{
  int     j;
  double  c;

  c = 1.0/(s->min * s->min) - 1.0;
  for (j = 0; j < s->np; j++)
  { 
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Amp[j] = 1.0 / sqrt(1.0 + c * tm * tm);
  }
}

void tra(s)					/* triangle */
Shape *s;
{
  int j;

  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / s->np;
    Amp[j] = s->c1 + 1.0 - fabs(tm);
  }
}

void elp(s)					/* Semi ellipse */
Shape *s;
{
  int j;

  for (j = 0; j < s->np-1; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np-1);
    Amp[j] = sqrt(1.0 - tm*tm);
  }
}

void exa(s)					/* exp ampl mod */
Shape *s;
{
  int j;

  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / s->np;
    Amp[j] = s->c1 + exp(s->c2 * tm);
  }
}

void tna(s)					/* tan ampl mod */
Shape *s;
{
  int j;
  double a, c;

  c = -s->c2 * rd;
  a = -s->c1 * tan(c);
  for (j = 0; j < s->np; j++)
  {
    tm = s->lft - s->rgt * (double) j / (s->np - 1);
    Amp[j] = a + tan(c * tm);
  }
}



void fser(s)					/* fser */
Shape *s;
{
  int i, j, k;
  double step, coef, lft, rgt;

  k = s->n * s->m;
  rgt = -0.5 * M_PI * s->rgt;
  lft = -0.5 * M_PI * (s->lft + 1.0);
  for (i = 0; i < s->np; i++)
  {
    Amp[i] = s->f[0];
    coef = lft - rgt * (double) i / (s->np-1);
    for (j = 2; j < k; j+=2)
    {
      step = (double) j * coef;
      Amp[i] += s->f[j] * cos(step) + s->f[j+1] * sin(step);
    }
  }
}

void ftr(s)					/* inv DFT */
Shape *s;
{
  int i, j, k, m, mm, ni, ni2;
  double cs, sn, re0, im0, step, cf1, cf2;

  cf1 = pi2 / (double) s->np;
  ni = s->m / 2;
  ni2 = ni*2;
  mm = 2 * (s->m - ni);
  for (i = 0; i < s->np; i++)
  {
    cf2 = cf1 * (double) i;
    re0 = 0.0, im0 = 0.0;
    for (j = 0; j < mm; j+=2)
    {
	step = 0.5 * cf2 * (double) j;
	k = ni2 + j;
	cs = cos(step);
	sn = sin(step);
	re0 += s->f[k] * cs - s->f[k+1] * sn;
	im0 += s->f[k+1] * cs + s->f[k] * sn;
    }
    m = s->np - ni; 
    for (j = 0; j < ni2; j+=2)
    {
	k = m + j/2;
	step = cf2 * (double) k;
	cs = cos(step);
	sn = sin(step);
	re0 += s->f[j] * cs - s->f[j+1] * sn;
	im0 += s->f[j+1] * cs + s->f[j] * sn;
    }

    if((re0) && (im0))
    {
      Pha[i] = atan2(im0, re0);
      Amp[i] = sqrt(re0 * re0 + im0 * im0);
    }
    else
      Pha[i] = im0, Amp[i] = re0;
  }
}


void sqa(s)					/* sq wave AM */
Shape *s;
{
  int i, j, k;

  k = s->m * s->n; 
  for (j = 0, i = 0; i < k; i+=s->n)
  {
    tm = 0.0;
    while (tm < s->f[i+1])
    {
      Amp[j] = s->f[i+2];  
      tm += s->dres;
      j++;
    }
  }
}
