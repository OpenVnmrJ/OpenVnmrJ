/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include "imagemath.h"
//#include <stdio.h>
//#include <stdlib.h>
#include <stdarg.h>
//#include <math.h>

float gammln(float xx)
{
	double x,tmp,ser;
	static double cof[6]={76.18009173,-86.50532033,24.01409822,
		-1.231739516,0.120858003e-2,-0.536382e-5};
	int j;

	x=xx-1.0;
	tmp=x+5.5;
	tmp -= (x+0.5)*log(tmp);
	ser=1.0;
	for (j=0;j<=5;j++) {
		x += 1.0;
		ser += cof[j]/x;
	}
	return -tmp+log(2.50662827465*ser);
}


#define ITMAX 100
#define EPS 3.0e-7

void gcf(float *gammcf, float a, float x, float *gln)
{
	int n;
	float gold=0.0,g,fac=1.0,b1=1.0;
	float b0=0.0,anf,ana,an,a1,a0=1.0;

	*gln=gammln(a);
	a1=x;
	for (n=1;n<=ITMAX;n++) {
		an=(float) n;
		ana=an-a;
		a0=(a1+a0*ana)*fac;
		b0=(b1+b0*ana)*fac;
		anf=an*fac;
		a1=x*a0+anf*a1;
		b1=x*b0+anf*b1;
		if (a1) {
			fac=1.0/a1;
			g=b1*fac;
			if (fabs((g-gold)/g) < EPS) {
				*gammcf=exp(-x+a*log(x)-(*gln))*g;
				return;
			}
			gold=g;
		}
	}
	fprintf(stderr, "a too large, ITMAX too small in routine GCF");
}

#undef ITMAX
#undef EPS


#define ITMAX 100
#define EPS 3.0e-7

void gser(float *gamser, float a, float x, float *gln)
{
	int n;
	float sum,del,ap;

	*gln=gammln(a);
	if (x <= 0.0) {
            if (x < 0.0) fprintf(stderr, "x less than 0 in routine GSER");
		*gamser=0.0;
		return;
	} else {
		ap=a;
		del=sum=1.0/a;
		for (n=1;n<=ITMAX;n++) {
			ap += 1.0;
			del *= x/ap;
			sum += del;
			if (fabs(del) < fabs(sum)*EPS) {
				*gamser=sum*exp(-x+a*log(x)-(*gln));
				return;
			}
		}
		fprintf(stderr, "a too large, ITMAX too small in routine GSER");
		return;
	}
}

#undef ITMAX
#undef EPS


float gammq(float a, float x)
{
	float gamser,gammcf,gln;

	if (x < 0.0 || a <= 0.0) fprintf(stderr, "Invalid arguments in routine GAMMQ");
	if (x < (a+1.0)) {
		gser(&gamser,a,x,&gln);
		return 1.0-gamser;
	} else {
		gcf(&gammcf,a,x,&gln);
		return gammcf;
	}
}


float chisqComp(float chisq, int n)
{
    return gammq(n / 2.0, chisq / 2);
}


float chisqCompInv(float p, int n)
{
    float tol = 1e-3; // fractional tolerance
    float diff;
    float x;
    float x1;
    float x2;
    float y;

    if (p < 0 || p > 1 || n < 1) {
        fprintf(stderr, "Invalid arguments in routine chisqComInv");
        n = n < 1 ? 1 : n;
        p = p < 0 ? 0 : (p > 1 ? 1 : p);
    }
    x1 = n;
    x2 = 2 * n;
    while ((y = chisqComp(x2, n)) > p) {
        x1 = x2;
        x2 *= 2;
    }
    while ((y = chisqComp(x1, n)) < p) {
        x1 /= 2;
    }
    x = (x1 + x2) / 2;
    y = chisqComp(x, n);
    while ((diff = fabs(1 - y / p)) > tol) {
        if (y < p) {
            x2 = x;
        } else {
            x1 = x;
        }
        x = (x1 + x2) / 2;
        y = chisqComp(x, n);
    }
    return x;
}


#define NPS 13
int main()
{
    float a;
    float x;
    int i;
    int n;
    float p;
    float pvals[] = {.005,.01,.025,.05,.1,.25,.5,.75,.9,.95,.975,.99,.999};

    a = 10.0;
/*     for (x = 1; x <= 20; x += 0.25) { */
/*         // q = gammq(a, x); */
/*         // printf("Gamma: q(%.1f,%.1f)=%f\n", a, x, q); */
/*         q = chisqComp(x, (int)a); */
/*         printf("ChisqComp: q(%.2f, %d) = %f\n", (int)a, x, q); */
/*     } */

    for (n = 1; n < 30; n += 5) {
        for (i = 0; i < NPS; i++) {
            p = pvals[i];
            x = chisqCompInv(p, n);
            printf("ChisqCompInv: X2(%.2g, %d) = %f\n", p, n, x);
        }
    }
    return 0;
}

