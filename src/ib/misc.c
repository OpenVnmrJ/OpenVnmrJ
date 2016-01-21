/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
static char *sccsID(){
    return "@(#)misc.c 18.1 03/21/08 (c)1991 SISCO";
}
/*
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <iostream>
#include "ddllib.h"


double errcheck( double d, char *s)
{
  if (errno == EDOM) {
    errno = 0;
    execerror(s, "argument out of range");
  }
  else if (errno == ERANGE) {
    errno = 0;
    execerror(s, "result out of range");
  }
  return d;
}
  
double Log(double x)
{
  return errcheck(log(x), "log");
}

double Log10(double x)
{
  return errcheck(log10(x), "log10");
}

double Exp(double x)
{
  return errcheck(exp(x), "exp");
}

double Sqrt(double x)
{
  return errcheck(sqrt(x), "sqrt");
}


double Pow(double x, double y)
{
  return errcheck(pow(x, y), "exponentiation");
}

double integer(double x)
{
  return (double)(long) x;
}

char *emalloc(unsigned int n)
{
  char *p;

  p = (char*) new char[n];
  if (p == 0) execerror("out of memory", 0);
  return p;
}

