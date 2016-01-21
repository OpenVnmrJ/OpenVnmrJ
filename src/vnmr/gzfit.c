/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* gzfit.c  - fit the z shims for gradient shimming */
/*					  b. fetler */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>              /*  to define iswspace macro */
#include <unistd.h>             /*  bits for 'access' in SVR4 */
#include <string.h>

#include "vnmrsys.h"		/* has MAXPATHL = 128 */
#include "data.h"
#include "group.h"
#include "tools.h"
#include "variables.h"
#include "group.h"		/* contains CURRENT, GLOBAL for P_getreal */
#include "pvars.h"
#include "wjunk.h"

#ifdef UNIX
#include <sys/file.h>           /*  bits for `access', e.g. F_OK */
#else 
#define F_OK            0       /* does file exist */
#define X_OK            1       /* is it executable by caller */
#define W_OK            2       /* writable by caller */
#define R_OK            4       /* readable by caller */
#endif 


#ifdef  DEBUG
extern int debug1;
extern int Eflag;
#define EPRINT0(level, str) \
        if (Eflag >= level) fprintf(stderr,str)
#define EPRINT1(level, str, arg1) \
	if (Eflag >= level) fprintf(stderr,str,arg1)
#define EPRINT2(level, str, arg1, arg2) \
        if (Eflag >= level) fprintf(stderr,str,arg1,arg2)
#define EPRINT(level, str) \
        if (debug1) Wscrprintf(str)
#else 
#define EPRINT(level, str)	/* ??? */
#define EPRINT0(level, str)
#define EPRINT1(level, str, arg1)
#define EPRINT2(level, str, arg1, arg2)
#endif 

#define MAX_PARAMS		64	/* number of shims plus one */
#define MAX_ITER		300	/* maximum number of its iterations in svdcmp */
#define TOL			1.0e-12
#define MAX_CURVES		MAX_PARAMS /* maximum number of curves, replaces gzmaxsize */
#define GZMINSIZE		8

#define sqrtf(axx)  ((float) sqrt((double)(axx)))

extern int Bnmr;

static int svdcmp(float *a, int m, int n, float *w, float *v);
static int fcossin(float x, float *p, int np);
static int fsechcos(float x, float *p, int np);
static int gzfit_output_files(char output_list2[], char mapname[], char shimsetnum[], int ma_list,
              float sw_edge, float chisq, float rmsfit, float rmserr, char cmd_name[], int btrigger,
              char shimname[][16], float shimcal[], float shimoldv[], float shimval[], float shimadd[],
              float shimerr[], char output_name[], int ma, int ndata, float x[], float y[], float wt[],
              float fbase[], float a[], float da[], char weight_output[], char verbose_on[]);
static int greadfunc_settype(int *readfunc_type, char *readfunc_name );

/* to run as a separate process, convert to system(s) call */
/* Comments about algorithm speed and size of algorithm elements. 
						-b.fetler, 9/14/95 */

static int svdfit(y,wt,ndata,a,da,ma,chisq,fbase)
float *y,*wt,*a,*da,*chisq,*fbase; /* fbase is basis file function */
int ndata,ma;	
/* pass fewer variables, remove u,v,w to local, pass da instead of v (smaller) */
/* no need to pass x */
/* wt replaces 1/sig, mult faster than div, no div 0 error, also sig may be infinite */
/* afunc and (*funcs) removed, replaced by fbase, precalculated function points,
	faster for poly (measured for Gradient Shimming) */
{
	int j, i, jj;
	float wmax,wrat,tmp,thresh,sum,*b; /* *afunc removed */
	float *u, *v, *w, chijunk;
	float s;

	if ((u = (float *)malloc(sizeof(float) * ndata * ma))==NULL) {
		Werrprintf("gzfit: error allocating curve fit buffer");
		return(-1);
		}
	if ((v = (float *)malloc(sizeof(float) * ma * ma))==NULL) {
		Werrprintf("gzfit: error allocating curve fit buffer");
		free((char *)u);
		return(-1);
		}
	if ((b = (float *)malloc(sizeof(float) * ndata))==NULL) {
		Werrprintf("gzfit: error allocating curve fit buffer");
		free((char *)u); free((char *)v);
		return(-1);
		}
	if ((w = (float *)malloc(sizeof(float) * ma))==NULL) {
		Werrprintf("gzfit: error allocating curve fit buffer");
		free((char *)u); free((char *)v); free((char *)b);
		return(-1);
		}

	for(j=0; j<ma; w[j]=0.0, j++) {
		for(jj=0; jj<ma; v[j*ma+jj] = 0.0, jj++); }
	for (i=0; i<ndata; b[i]=y[i] * wt[i], i++) {
		for (j=0; j<ma; u[i*ma+j]=fbase[i+ndata*j] * wt[i], j++); }

	if (svdcmp(u,ndata,ma,w,v) != 0) { 
		free((char *)w);
		free((char *)b);
		free((char *)u);
		free((char *)v);
		return(-1);
		} 

	wmax=0.0;		/* set singular values to zero, calculate wrat */
	for (j=0; j<ma; j++) if (w[j] > wmax) wmax=w[j];
	wrat=wmax;
	thresh=TOL*wmax;
	for (j=0; j<ma; j++) { 
		if (w[j] < wrat) wrat=w[j];
		if (w[j] < thresh) w[j]=0.0;
		}
	wrat = wmax/wrat; /* condition number of w matrix */
	if (wrat > 1/TOL) { Werrprintf( "WARNING: gradient shimming curve fit ill-conditioned\n" ); } 
	EPRINT1(1,"\n    curve fit matrix condition number %g \n", wrat );

/* 	back substitution - eliminate tmp[], saves memory, number of steps within for */
	for (j=0; j<ma; a[j]=0.0, j++); 
	for (j=0; j<ma; j++) {
		s = 0.0;
		if (w[j] != 0.0) {
			for (i=0; i<ndata; s += u[i*ma+j] * b[i], i++);
			s /= w[j];
			for (jj=0; jj<ma; a[jj] += v[jj*ma+j] * s, jj++);
			}
		} 
/* 	end back sub */

/*	define da, error in a */
/*	for (j=0; j<ma; da[j]=0.0, j++); */
	if (da[0] < TOL)
	    {
	    for (j=0; j<ma; j++)
		if (w[j] != 0.0)
		    for (jj=0; jj<ma; da[jj] += v[jj*ma+j] * v[jj*ma+j]/(w[j] * w[j]),jj++);
	    for (j=0; j<ma; da[j]=sqrtf(da[j]), j++); 
	    }

	free((char *)u);
	free((char *)v);
	free((char *)w);
	free((char *)b);

	chijunk=0.0;	/* could put into gzfit main */
	for (i=0; i<ndata; i++) {
		for (sum=0.0, j=0; j<ma; sum += a[j] * fbase[i+ndata*j], j++);
		tmp = (y[i] - sum) * wt[i];  chijunk += tmp * tmp; 
		} 	/* chisq sum implicit, fbase function values precalculated (faster) */

	EPRINT1(2,"    chisq %f\n", chijunk );
	*chisq = chijunk; 

	return(0);

}


static float atmp,btmp,ctmp; 
#define SMALLRT(a,b) (ctmp=b/a, a*sqrt(1.0+ctmp*ctmp))		
	/* ctmp must be different name than atmp or btmp */
#define PYTHAG(a,b) (atmp=fabs(a), btmp=fabs(b), atmp > btmp ? \
SMALLRT(atmp,btmp) : (btmp ? SMALLRT(btmp,atmp) : 0.0)) 
#define MAX(a,b) (a > b ? a : b)  	/* use fewer static float variables */
#define SIGN(a,b) (atmp=fabs(a), b >= 0.0 ? atmp : -atmp)  /* use fewer calls to fabs */

#define CMPLXMULT(a,b,c,s) (atmp=a, a=atmp*c+b*s, b=b*c-atmp*s)   /* (a+ib) -> (a+ib) * (c-is) */
	/* fewer substitution variables, see where used below */

/* a,w matrices; v vector */
static int svdcmp(float *a, int m, int n, float *w, float *v)
{
	int flag,nm,n1,m1;
	register int i,j,jj,k,its,l=0;
	float c,f,h,s,x,y,z;
	float anorm=0.0,g=0.0,scale=0.0;
	float ai,gi,hi,si;	/* ai=1/a[][], gi=1/g, hi=1/h, si=1/scale, inverse avoids divides */
	float *rv1;

/*	int cm1=0,cm2=0,cm3=0,cm4=0,cm5=0,sg1=0,sg2=0,sg3=0,p1=0,p2=0,p3=0,p4=0;
	int si1=0,hi1=0,si2=0,hi2=0,gi1=0,ai1=0,ai2=0,mm1=0,fab1=0,fab2=0;  */
					/* instruction counters, count number of operations */

	n1=n-1; m1=m-1;		/* note that n < m always */
	if (n > m) {
		Werrprintf("gradient shimming curve fit failure");
		return(-1);
		}

/*	rv1=(float *)malloc(n * sizeof(float)); */
        if ((rv1 = (float *)malloc(sizeof(float) * n))==NULL) {
                Werrprintf("gzfit: error allocating curve fit buffer");
                return(-1);
                }

/* Householder reduction */
	for (i=0;i<n;i++) {
		l=i+1;
		rv1[i]=scale*g;
		g=s=scale=0.0;	/* eliminate condition if(i<m1), always true */
			for (k=i;k<m;k++) scale += fabs(a[k*n+i]);
			if (scale) {
				si = 1/scale;
				for (k=i;k<m;k++) {
/*					si1++; */
					a[k*n+i] *= si;		/* note: a[k*n+i]=a[k][i] */
					s += a[k*n+i]*a[k*n+i];
				}
				f=a[i*n+i];
				g = -SIGN(sqrt(s),f);
/*				sg1++; */
				h=f*g-s;
				hi = 1/h;
				a[i*n+i]=f-g;
				if (i != n1) {
					for (j=l;j<n;j++) {
					    for (s=0.0,k=i;k<m;k++) s += a[k*n+i]*a[k*n+j];
					    f = s * hi;
/*					    hi1++; */
					    for (k=i;k<m;k++) a[k*n+j] += f*a[k*n+i];
					}
				}
				for (k=i;k<m;k++) a[k*n+i] *= scale;
			}
		w[i]=scale*g;
		g=s=scale=0.0;
		if (i != n1) {	/* eliminate condition && i<m1, always true */
			for (k=l;k<n;k++) scale += fabs(a[i*n+k]);
			if (scale) {
				si = 1/scale;
				for (k=l;k<n;k++) {
					a[i*n+k] *= si;
/*					si2++; */
					s += a[i*n+k]*a[i*n+k];
				}
				f=a[i*n+l];
				g = -SIGN(sqrt(s),f);
/*				sg2++; */
				h=f*g-s;
				hi = 1/h;
				a[i*n+l]=f-g;
				for (k=l;k<n;k++) rv1[k]=a[i*n+k] * hi;
/*				hi2 += n; */
				if (i != m1) {
					for (j=l;j<m;j++) {
					    for (s=0.0,k=l;k<n;k++) s += a[j*n+k]*a[i*n+k];
					    for (k=l;k<n;k++) a[j*n+k] += s*rv1[k];
					}
				}
				for (k=l;k<n;k++) a[i*n+k] *= scale;
			}
		}
		anorm=MAX(anorm,(fabs(w[i])+fabs(rv1[i])));
/*		mm1++; */
	}

/* Accumulation of lh and rh transforms */
	for (i=n1;i>=0;i--) {
		if (i < n1) {
			if (g) {
				gi = 1/g;
				ai = 1/a[i*n+l];
				for (j=l;j<n;j++)
					v[j*n+i]= a[i*n+j] * ai * gi;
/*					gi1 += n;
					ai1 += n; */
				for (j=l;j<n;j++) {
					for (s=0.0,k=l;k<n;k++) s += a[i*n+k]*v[k*n+j];
					for (k=l;k<n;k++) v[k*n+j] += s*v[k*n+i];
				}
			}
			for (j=l;j<n;j++) v[i*n+j]=v[j*n+i]=0.0;
		}
		v[i*n+i]=1.0;
		g=rv1[i];
		l=i;
	}
	for (i=n1;i>=0;i--) {
		l=i+1;
		g=w[i];
		if (i < n1)
			for (j=l;j<n;j++) a[i*n+j]=0.0;
		if (g) {
			g=1.0/g;
			if (i != n1) {
				ai = 1/a[i*n+i];
				for (j=l;j<n;j++) {
					for (s=0.0,k=l;k<m;k++) s += a[k*n+i]*a[k*n+j];
					f = s * ai * g;
/*					ai2++; */
					for (k=i;k<m;k++) a[k*n+j] += f*a[k*n+i];
				}
			}
			for (j=i;j<m;j++) a[j*n+i] *= g;
		} else {
			for (j=i;j<m;j++) a[j*n+i]=0.0;
		}
		++a[i*n+i];
	}

/* Diagonalization */
	for (k=n1;k>=0;k--) {
		for (its=0;its<MAX_ITER;its++) {
			flag=1;
			for (l=k;l>=0;l--) {
				nm=l-1;
/*				fab1++; */
				if (fabs(rv1[l])+anorm == anorm) {
					flag=0;
					break;
				}
/*				fab1++; */
				if (fabs(w[nm])+anorm == anorm) break;
			}
			if (flag) {
				c=0.0;
				s=1.0;
				for (i=l;i<k;i++) {
					f=s*rv1[i];
/*					fab2++; */
					if (fabs(f)+anorm != anorm) {
						g=w[i];
						h=PYTHAG(f,g);
/*						p1++; */
						w[i]=h;
						h=1.0/h;
						c=g*h;
						s=(-f*h);
						for (j=0;j<m;j++) {
							CMPLXMULT(a[j*n+nm],a[j*n+i],c,s); 
/*							cm1++; */
						}
					}
				}
			}
			z=w[k];
			if (l == k) {
				if (z < 0.0) {
					w[k] = -z;
					for (j=0;j<n;j++) v[j*n+k]=(-v[j*n+k]);
				}
				break;
			}
			if (its >= (MAX_ITER-1)) { 	/* MAX_ITER = 30, max number of iterations */
				Werrprintf("gradient shimming curve fit failed");
				free((char *)rv1);
				return(-1);
				}

/* Start of QR transforms */
			x=w[l];
			nm=k;
			y=w[nm];
			g=rv1[nm];
			h=rv1[k];
			f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
			g=PYTHAG(f,1.0);
/*			p2++;
			sg3++; */
			f=((x-z)*(x+z)+h*((y/(f+SIGN(g,f)))-h))/x;
			c=s=1.0;
			for (j=l;j<nm;j++) {
				i=j+1;
				g=rv1[i];
				y=w[i];
				h=s*g;
				g=c*g;
				z=PYTHAG(f,h);
/*				p3++; */
				rv1[j]=z;
				c=f/z;
				s=h/z;

/*				f=x*c+g*s;
				g=g*c-x*s; */
				f = x;
				CMPLXMULT( f, g, c, s ); 
/*				cm2++; */

				h=y*s;
				y=y*c;
				for (jj=0;jj<n;jj++) {
					CMPLXMULT( v[jj*n+j], v[jj*n+i], c, s ); 
/*					cm3++; */
				}
				z=PYTHAG(f,h);
/*				p4++; */
				w[j]=z;
				if (z) {
					z=1.0/z;
					c=f*z;
					s=h*z;
				}

/*				f=(c*g)+(s*y);
				x=(c*y)-(s*g);		*/
				f = g; x = y;
				CMPLXMULT( f, x, c, s ); 
/*				cm4++; */

				for (jj=0;jj<m;jj++) {
					CMPLXMULT( a[jj*n+j], a[jj*n+i], c, s ); 
/*					cm5++; */
				}
			}
			rv1[l]=0.0;
			rv1[k]=f;
			w[k]=x;
		}
/*	printf("its %d, k %d; ",its,k); */
	}
/*	printf("\ncm1 %d, cm2 %d, cm3 %d, cm4 %d, cm5 %d, sg1 %d, sg2 %d, sg3 %d\n",
			cm1,cm2,cm3,cm4,cm5,sg1,sg2,sg3 );
	printf("p1 %d, p2 %d, p3 %d, p4 %d, si1 %d, hi1 %d, si2 %d, hi2 %d\n",p1,p2,p3,p4,si1,hi1,si2,hi2);
	printf("gi1 %d, ai1 %d, ai2 %d, mm1 %d, fab1 %d, fab2 %d\n", gi1,ai1,ai2,mm1,fab1,fab2); 
*/

	free((char *)rv1);
	return(0);
}

#undef SIGN
#undef MAX
#undef PYTHAG
#undef CMPLXMULT
#undef SMALLRT


static int gzfit_read_header(infile, input_name, b_ndata, b_ma, cmd_name) 
int 	 *b_ndata, *b_ma;
FILE	*infile;
char	 cmd_name[], input_name[];
{
	int i;
	char jstr[MAXPATHL];

	*b_ndata = 0; *b_ma = 0;
	for (i=0; i<4; i++) {
		if ( fscanf( infile, "%s", jstr) == EOF) {
			Werrprintf("%s error: invalid format for file %s", cmd_name, input_name);
			return(-1);
			}
		switch (i) {
			case 2 : if (isReal(jstr)) *b_ma = atoi( jstr ); break;
			case 3 : if (isReal(jstr)) *b_ndata = atoi( jstr ); break;
			default : ;
			}
		}
	if (*b_ndata < 1 || *b_ndata > 256 * 1024) {	/* max fn 512k; does not apply to poly fit */
		Werrprintf("gzfit error: invalid number of points in file %s", input_name);
		return(-1);
		}
	do {
		if (fscanf( infile, "%s", jstr) == EOF) strcpy(jstr,"errflag");
		} 	/* fscanf for string until data start */
		while ( (strcmp(jstr,"1") != 0) && (strcmp(jstr,"errflag") != 0) );
	if (strcmp(jstr,"errflag") == 0) {
		Werrprintf("%s error: invalid format for file %s", cmd_name, input_name );
		return(-1);
		}
	for (i=0; i<3; i++) {
		if ((fscanf( infile, "%s", jstr) == EOF) || (strcmp(jstr,"0") != 0)) {
			Werrprintf("%s error: invalid format for file %s", cmd_name, input_name);
			return(-1);
			}
		}
	return(0);
}


static int gzfit_readstore_data(i_file, ndata, index, pfloat)
FILE	*i_file;
int	 ndata, index;
float	*pfloat;
{
	int i;
	float freq, ph;

	for( i=0; i < ndata; i++) {
	  	if ((fscanf( i_file, "%f", &freq)==EOF) || (fscanf( i_file, "%f", &ph)==EOF)) 
			return(-1); 
		else pfloat[index + i] = ph; 
		} 
	return(0);
}


static int gzfit_read_input_body(infile, input_name, cmd_name, x, y, ndata, sw_edge) 
FILE	*infile;
char	 input_name[], cmd_name[];
float	*x, *y, *sw_edge;
int	 ndata;
{
	int i;
	float freq, ph, tmp;

	for( i=0; i < ndata; i++) {
	  	if ((fscanf( infile, "%f", &freq)==EOF) || (fscanf( infile, "%f", &ph)==EOF)) {
			Werrprintf("%s error: end of file %s reached", cmd_name, input_name);
			return(-1);
		  	}
		else {
			x[i] = freq; 
			y[i] = ph; 
			}
		} 

	tmp = fabs(x[0]);
	if (fabs(freq) > tmp) tmp = fabs(freq);
	tmp *= ((float)ndata) / ((float)(ndata-1));
	if ((tmp > 1.0) && (tmp < 1.0e7)) *sw_edge = tmp; 
	else *sw_edge = 25000; 
	*sw_edge = 1.0 / *sw_edge;
	for (i=0; i<ndata; i++) x[i] *= *sw_edge;
	*sw_edge = 1.0 / *sw_edge;

	EPRINT2(1, "  Input file %s read, %d points.\n", input_name, ndata ); 
	EPRINT0(2, "    x-axis normalized\n");
	EPRINT2(2, "    first data point x[0] %f, y[0] %f\n", x[0], y[0] ); 

	return(0);
}


static int gzfit_read_wt_body(wtfile, wt_name, cmd_name, wt, ndata, w_ndata) 
FILE	*wtfile;
char	 wt_name[], cmd_name[];
float	*wt;
int	 ndata, w_ndata;
{
	int i, wn_skip;
	float freq, ph;

        if (w_ndata == ndata) {
	if (gzfit_readstore_data(wtfile, w_ndata, 0, wt) != 0) {
		Werrprintf("%s error: end of file %s reached", cmd_name, wt_name);
		return(-1); 
		}
		} 

	else if (w_ndata > ndata) {
		wn_skip = (w_ndata - ndata)/2;
/*	skipping w_ndata-ndata endpoints */
                if (wn_skip > 0) {
                for( i = 1; i <= wn_skip; i++ ) {
	  	  if ((fscanf( wtfile, "%f", &freq)==EOF) || (fscanf( wtfile, "%f", &ph)==EOF)) {
                	Werrprintf("%s warning: end of file %s reached", cmd_name, wt_name);
			return(-1);
			} } }
		if (gzfit_readstore_data(wtfile, ndata, 0, wt) != 0) {
                	Werrprintf("%s warning: end of file %s reached", cmd_name, wt_name);
			return(-1);
			}
                if ((w_ndata - ndata - wn_skip) > 0) {
                        for(i=1; i<=(w_ndata - ndata - wn_skip); i++) {		/* could skip read here */
	  	  if ((fscanf( wtfile, "%f", &freq)==EOF) || (fscanf( wtfile, "%f", &ph)==EOF)) {
                	Werrprintf("%s warning: end of file %s reached", cmd_name, wt_name);
			return(-1);
			}
			}
		}
		}

	else { 		/* w_ndata < ndata */
		wn_skip = (ndata - w_ndata)/2;
	EPRINT1(1,"  Too few points in weighting file, zero-filling %d endpoints.\n", ndata-w_ndata); 

	  	if ((fscanf( wtfile, "%f", &freq)==EOF) || (fscanf( wtfile, "%f", &ph)==EOF)) {
                	Werrprintf("%s warning: end of file %s reached", cmd_name, wt_name);
			return(-1);
			}
		wt[0] = 0.0; 
		if (wn_skip > 0) {
			for(i=1; i<wn_skip; i++)  wt[i] = 0.0; 
			}
		wt[ wn_skip ] = ph;
		if (gzfit_readstore_data(wtfile, w_ndata-1, wn_skip+1, wt) != 0) {
                	Werrprintf("%s warning: end of file %s reached", cmd_name, wt_name);
			return(-1);
			}
		if ((ndata - w_ndata - wn_skip) > 0) {
			for(i=0; i<(ndata - w_ndata - wn_skip); i++) 
				{ wt[ i + wn_skip + w_ndata] = 0.0; } /* =ph; */
			}
		}

	EPRINT1(1, "  Weight file %s read.\n", wt_name ); 
	EPRINT1(2, "    first data point wt[0] %f\n", wt[0] ); 

	return(0);
}


static int gzfit_read_b_body(b_file, basis_name, cmd_name, ndata, b_ndata, b_ma, fbase) 
FILE	*b_file;
char	 basis_name[], cmd_name[];
int	 ndata, b_ndata, b_ma;
float	*fbase;
{
	int i, j, bn_skip;
	float freq, ph;
	char jstr[MAXPATHL];

	if (b_ndata == ndata) {

	for (i = 0; i < b_ndata; i++ )  fbase[i] = 1.0;   /* z^0 base vector */
	for( j = 1; j <= b_ma; j++ )
		{
		if (j > 1) for (i = 0; i < 4; i++) {
			if (fscanf( b_file, "%s", jstr ) == EOF) {
				Werrprintf("%s error: end of file %s reached", cmd_name, basis_name);
				return(-1);
				}
			}
		if (gzfit_readstore_data(b_file, b_ndata, (b_ndata * j), fbase) != 0) {
			Werrprintf("%s error: end of file %s reached", cmd_name, basis_name);
			return(-1);
			}
		}
		} 

	if (b_ndata > ndata) {
	bn_skip = (b_ndata - ndata)/2;

        for (i = 0; i < ndata; i++ )  fbase[i] = 1.0;   /* z^0 base vector */
        for( j = 1; j <= b_ma; j++ )
                {
		if (j > 1) for (i = 0; i < 4; i++) {
			if (fscanf( b_file, "%s", jstr ) == EOF) {
				Werrprintf("%s error: end of file %s reached", cmd_name, basis_name);
				return(-1);
				}
			}

		if (bn_skip > 0) {
		for( i = 1; i <= bn_skip; i++ ) {
			if ((fscanf( b_file, "%f", &freq)==EOF) || (fscanf(b_file,"%f",&ph)==EOF)) {
				Werrprintf("%s error: end of file %s reached", cmd_name, basis_name);
				return(-1);
				}
			}
		if (gzfit_readstore_data(b_file, ndata, (ndata * j), fbase) != 0) {
			Werrprintf("%s error: end of file %s reached", cmd_name, basis_name);
			return(-1);
			}
		if ((b_ndata - ndata - bn_skip) > 0) {
			for(i=1; i<=(b_ndata - ndata - bn_skip); i++) {
			if ((fscanf( b_file, "%f", &freq)==EOF) || (fscanf(b_file,"%f",&ph)==EOF)) {
				Werrprintf("%s error: end of file %s reached", cmd_name, basis_name);
				return(-1);
				}
			}
                }
                }
	}
	}

	return(0);
}


static int gzfit_wt_calc(weight_on, weight_fit, wt, x, ndata)	/* weight calculation */
char	 weight_on[],
	 weight_fit[];
float 	*wt, *x;
int	 ndata;
{
	float tmp, tmp2, x_offset, ph;
	int i, itmp1, itmp2;

	float *a, *da, chisq, *fbase = NULL, *w;	/* "extra" variables for wt curve fit */
	int b_ma, j, jj;

	if (strcmp(weight_on,"f") == 0) {

	if (strcmp(weight_fit,"a")==0) {

/*	define TRAPEZOID by height tmp, 50% cutoffs itmp1, itmp2, calculate cutoffs */
	tmp=0.0;
	for (i=(ndata/4); i<(ndata-(ndata/4)); i++) tmp += wt[i];
	tmp /= (float)(ndata - 2*(ndata/4)); 	/* wt-avg in middle half */

	tmp /= 2.0;
	itmp1=0; itmp2=0;
	jj = 1;
	for (i=0; (jj==1 && i<ndata/2); i++) 
		if (wt[i] > tmp) { itmp1 = i; jj = 0; }
	jj = 1;
	for (i=ndata-1; (jj==1 && i>(ndata/2-1)); i--) 
		if (wt[i] > tmp) { itmp2 = i; jj = 0; }
	tmp *= 2.0;
	if ((itmp1 < (ndata/20)) || (itmp2 > (19*ndata/20))) {
		itmp1 = ndata/20; itmp2 = ndata-itmp1; 
		}
	itmp1 *= 2;
	itmp2 = ndata - ((ndata-itmp2)*2); 

	for (i=0; i<itmp1; i++) wt[i] = ((float)i)/((float)itmp1);
	for (i=itmp1; i<(itmp2+1); i++) wt[i] = 1.0;
	for (i=(itmp2+1); i<ndata; i++) wt[i] = ((float)(ndata-1-i)/(float)(ndata-1-itmp2));

	} /* end if weight_fit=="a" */

	else if (strcmp(weight_fit,"s")==0) {	/* 14641 data smoothing - do it twice */

        if ((a = (float *)malloc(sizeof(float) * ndata))==NULL) {
                Werrprintf("gzfit: error allocating curve fit buffer");
                return(-1);
                }
	a[0] = 6.0*wt[0] + 4.0*wt[1] + wt[2];
	a[1] = 4.0*wt[0] + 6.0*wt[1] + 4.0*wt[2] + wt[3];
	for (i=2; i<(ndata-2); i++) 
		a[i] = (wt[i-2]+wt[i+2]) + 4.0 * (wt[i-1]+wt[i+1]) + 6.0 * wt[i];
	a[ndata-2] = 4.0*wt[ndata-1] + 6.0*wt[ndata-2] + 4.0*wt[ndata-3] + wt[ndata-4];
	a[ndata-1] = 6.0*wt[ndata-1] + 4.0*wt[ndata-2] + wt[ndata-3];
	for (i=0; i<ndata; i++) wt[i] = a[i]/16.0;

	a[0] = 6.0*wt[0] + 4.0*wt[1] + wt[2];
	a[1] = 4.0*wt[0] + 6.0*wt[1] + 4.0*wt[2] + wt[3];
	for (i=2; i<(ndata-2); i++) 
		a[i] = (wt[i-2]+wt[i+2]) + 4.0 * (wt[i-1]+wt[i+1]) + 6.0 * wt[i];
	a[ndata-2] = 4.0*wt[ndata-1] + 6.0*wt[ndata-2] + 4.0*wt[ndata-3] + wt[ndata-4];
	a[ndata-1] = 6.0*wt[ndata-1] + 4.0*wt[ndata-2] + wt[ndata-3];
	for (i=0; i<ndata; i++) wt[i] = a[i]/16.0;

	free((char *)a);

	}  /* end if weight_fit=="s" */
	else if ((strcmp(weight_fit,"c")==0) || (strcmp(weight_fit,"j")==0)) { 
		 	/* could include other types of curve fit */
			/* could do data smoothing, 14641 style */

	  b_ma = 16;
	  if (strcmp(weight_fit,"c")==0) {
	b_ma = 16;
	if ((fbase = (float *)malloc(sizeof(float) * ndata * b_ma))==NULL) {
		Werrprintf("gzfit: error allocating curve fit buffer");
		return(-1);
		}
	for(i = 0; i < b_ma * ndata; i++) fbase[i]=0.0;
       	if ((w = (float *)malloc(sizeof(float) * b_ma))==NULL) {
               	Werrprintf("gzfit: error allocating curve fit buffer");
		free((char *)fbase);
               	return(-1);
               	}

	for( i=0; i<ndata; i++ ) {
		if( fcossin(x[i],w,b_ma) != 0) { 
			Werrprintf( "gzfit: polynomial calc failed" ); 
			free((char *)fbase); free((char *)w);
			ABORT;
			}
		for( j=0; j<b_ma; j++ ) { fbase[i+ndata*j] = w[j]; }
		}
	free((char *)w);
	EPRINT1(1, "  Polynomial basis function defined, order %d.\n", b_ma-1 ); 
	  } /* end if weight_fit=="c" */

	  else if (strcmp(weight_fit,"j")==0) {
/*	svdfit wt with SECH * polynomial */  /* ma -> b_ma */
	b_ma = 3;
	if ((fbase = (float *)malloc(sizeof(float) * ndata * b_ma))==NULL) {
		Werrprintf("gzfit: error allocating curve fit buffer");
		return(-1);
		}
	for(i = 0; i < b_ma * ndata; i++) fbase[i]=0.0;
       	if ((w = (float *)malloc(sizeof(float) * b_ma))==NULL) {
               	Werrprintf("gzfit: error allocating curve fit buffer");
		free((char *)fbase); 
               	return(-1);
               	}

	for( i=0; i<ndata; i++ ) {
		if( fsechcos(x[i],w,b_ma) != 0) { 
			Werrprintf( "gzfit: polynomial calc failed" ); 
			free((char *)fbase); free((char *)w);
			ABORT;
			}
		for( j=0; j<b_ma; j++ )  fbase[i+ndata*j] = w[j]; 
		}
	free((char *)w);
	EPRINT1(1, "  SECH * Polynomial basis function defined, order %d.\n", b_ma-1 ); 
	  } /* end if weight_fit=="j" */

        if ((w = (float *)malloc(sizeof(float) * ndata))==NULL) {
                Werrprintf("gzfit: error allocating curve fit buffer");
                return(-1);
                }
        if ((a = (float *)malloc(sizeof(float) * b_ma))==NULL) {
                Werrprintf("gzfit: error allocating curve fit buffer");
		free((char *)w); free((char *)fbase);
                return(-1);
                }
        if ((da = (float *)malloc(sizeof(float) * b_ma))==NULL) {
                Werrprintf("gzfit: error allocating curve fit buffer");
		free((char *)w); free((char *)a); free((char *)fbase);
                return(-1);
                }
	for( i = 0; i < b_ma; i++ ) { a[i] = 0.0; da[i] = 0.0; }
	for (i=0; i<ndata; i++) w[i]=1.0; 	/* weighting function */
	EPRINT0(1, "  calling svdfit...  " ); 
	if( svdfit(wt,w,ndata,a,da,b_ma,&chisq,fbase) != 0)
		{
		free((char *)w); free((char *)a); free((char *)da); free((char *)fbase);
		return( -1 );
		} 

/*	calculate new wt from a, fbase */
	for( i = 0; i < ndata; i++ ) {
		ph = 0.0;
		for ( jj = 0; jj < b_ma; jj++) 
			ph += a[jj] * fbase[i+ndata*jj];
		wt[i] = ph;
		} 

	free((char *)w); free((char *)a); free((char *)da); free((char *)fbase);
	} /* end if weight_fit=="c" || "j" */

	EPRINT0(1, "  Fitting field, profile weighting...  " ); 
	/* normalize wt[i] */
	tmp = 0.0;
	for (i = 0; i < ndata; i++) {
		if (wt[i] >= 0.0)  tmp2 = wt[i]; 
		else  tmp2 = -wt[i]; 
		if (tmp2 > tmp)  tmp = tmp2; 
		} 
	for (i = 0; i < ndata; i++)  wt[i] /= tmp; 
	} /* end if weight_on=="f" */

	else if (strcmp(weight_on,"t") == 0) {	/* Tcheby. loop */

		EPRINT0(1, "  Fitting field, Tchebyshev weighting...  " ); 

		/* already normalized to one with sw_edge */
		x_offset = x[0]; if (x_offset < 0.0) { x_offset = -x_offset; }
		tmp = x[ndata-1]; if (tmp < 0.0) { tmp = -tmp; }
		if (tmp > x_offset) { x_offset = tmp; }
/*		x_offset *= ((float)(ndata)/(float)(ndata - 1)); */
/*		x_offset *= ((float)(ndata-3)/(float)(ndata)); */

                for( i = 0; i < ndata; i++)
                        {
			if (( 1.0 - (x[i]/x_offset)*(x[i]/x_offset) ) < 0.0) {
			EPRINT1(1, "gzfit: point %d Tcheby. sqrt error ignored\n", i); wt[i] = 0.0; }
       			else {
/*		tmp = (float) (sqrt(sqrt((double) (1.0 - (x[i]/x_offset)*(x[i]/x_offset)) ))); */
/*		tmp = (float) (sqrt((double) (1.0 - (x[i]/x_offset)*(x[i]/x_offset)) )); */
		tmp = 1.0 - (x[i]/x_offset)*(x[i]/x_offset);
		if (tmp < 0.0) tmp=0.0;
		else tmp = (float) (sqrt(sqrt((double) (1.0 - (x[i]/x_offset)*(x[i]/x_offset)) ))); 
			wt[i] = tmp;
				}
                        }
		}	/* end Tcheby. loop */

	else if (strcmp(weight_on,"r")==0) {	/* TRAPEZOID, assuming gzwin correct */

	itmp1 = (int)(0.2 * (float)ndata);
	itmp2 = (int)(0.8 * (float)ndata);
	for (i=0; i<itmp1; i++) wt[i] = ((float)i)/((float)itmp1);
	for (i=itmp1; i<(itmp2+1); i++) wt[i] = 1.0;
	for (i=(itmp2+1); i<ndata; i++) wt[i] = ((float)(ndata-1-i)/(float)(ndata-1-itmp2));

	} /* end if weight_on=="r" */

	else if (strcmp(weight_on,"h")==0) {
	tmp  = 1.0;
	tmp2 = 16.0;
	for (i=0; i<ndata; i++) { 
		ph = exp(tmp2 * (x[i]-0.68)); 
		ph += 1.0/ph;
		ph = 2.0 / ph;
		wt[i] = ph;
		} 
	for (i=0; i<ndata; i++) { 
		ph = exp(tmp2 * (x[i]-0.51)); 
		ph += 1.0/ph;
		ph = 2.0 / ph;
		wt[i] += ph;
		} 
	for (i=0; i<ndata; i++) { 
		ph = exp(tmp2 * (x[i]-0.34)); 
		ph += 1.0/ph;
		ph = 2.0 / ph;
		wt[i] += ph;
		} 
	for (i=0; i<ndata; i++) { 
		ph = exp(tmp2 * (x[i]-0.17)); 
		ph += 1.0/ph;
		ph = 2.0 / ph;
		wt[i] += ph;
		} 
	for (i=0; i<ndata; i++) { 
		ph = exp(tmp2 * x[i]); 
		ph += 1.0/ph;
		ph = 2.0 / ph;
		wt[i] += ph;
		} 
	for (i=0; i<ndata; i++) { 
		ph = exp(tmp2 * (x[i]+0.17)); 
		ph += 1.0/ph;
		ph = 2.0 / ph;
		wt[i] += ph;
		} 
	for (i=0; i<ndata; i++) { 
		ph = exp(tmp2 * (x[i]+0.34)); 
		ph += 1.0/ph;
		ph = 2.0 / ph;
		wt[i] += ph;
		} 
	for (i=0; i<ndata; i++) { 
		ph = exp(tmp2 * (x[i]+0.51)); 
		ph += 1.0/ph;
		ph = 2.0 / ph;
		wt[i] += ph;
		} 
	for (i=0; i<ndata; i++) { 
		ph = exp(tmp2 * (x[i]+0.68)); 
		ph += 1.0/ph;
		ph = 2.0 / ph;
		wt[i] += ph;
		} 
	tmp = 0.0;
	for (i = 0; i < ndata; i++) {
		if (wt[i] >= 0.0)  tmp2 = wt[i]; 
		else  tmp2 = -wt[i]; 
		if (tmp2 > tmp)  tmp = tmp2; 
		} 
	for (i = 0; i < ndata; i++)  wt[i] /= tmp; 

	}  /* end if weight_on=="h" */

	else if (strcmp(weight_on,"i")==0) {
	tmp=0.0;
	for (i=(ndata/4); i<(ndata-(ndata/4)); i++) tmp += wt[i];
	tmp /= (float)(ndata - 2*(ndata/4)); 	/* wt-avg in middle half */

	itmp1 = (int)(0.15 * (float)ndata);
	itmp2 = (int)(0.85 * (float)ndata);
	tmp2 = 5.0;
	tmp2 /= (x[0] - x[itmp1-1]);
	for (i=0; i<itmp1; i++) { 
		ph = exp(tmp2 * (x[i] - x[itmp1-1])); 
		ph += 1.0/ph;
		ph = 2.0 / ph;
		wt[i] = ph;
		}
	for (i=itmp1; i<(itmp2+1); i++) wt[i] = 1.0;
	for (i=(itmp2+1); i<ndata; i++) { 
		ph = exp(-tmp2 * (x[i] - x[itmp2+1])); 
		ph += 1.0/ph;
		ph = 2.0 / ph;
		wt[i] = ph;
		}

	}  /* end if weight_on=="i" */

	else if (strcmp(weight_on,"k")==0) {
	tmp=0.0;
	for (i=(ndata/4); i<(ndata-(ndata/4)); i++) tmp += wt[i];
	tmp /= (float)(ndata - 2*(ndata/4)); 	/* wt-avg in middle half */

	itmp1 = (int)(0.15 * (float)ndata);
	itmp2 = (int)(0.85 * (float)ndata);
	tmp2 = 5.0;
	tmp2 /= (x[0] - x[itmp1-1]);
	for (i=0; i<itmp1; i++) { 
		ph = exp(tmp2 * (x[i] - x[itmp1-1])); 
		ph += 1.0/ph;
		ph = 2.0 / ph;
		wt[i] = ph;
		}
	for (i=itmp1; i<(itmp2+1); i++) wt[i] = 1.0;
	for (i=(itmp2+1); i<ndata; i++) { 
		ph = exp(-tmp2 * (x[i] - x[itmp2+1])); 
		ph += 1.0/ph;
		ph = 2.0 / ph;
		wt[i] = ph;
		}
	tmp = 2.0 * M_PI / (x[itmp2+1] - x[itmp1]);
	for (i=itmp1; i<(itmp2+1); i++) 
		wt[i] = 0.93 - 0.07 * cos(tmp * x[i]);

	}  /* end if weight_on=="k" */

	else { 		/* (strcmp(weight_on,"n") == 0) */
		EPRINT0(1, "  Fitting field, no weighting...  " ); 
		} 
	return(0);
}


/* real fourier decomp, cos + sin */
static int fcossin(float x, float *p, int np)
{
  int    j;
  float  ppc, pps;
	p[0] = 1.0;
	ppc = cos(M_PI * x);
	pps = sin(M_PI * x);
	p[1] = pps;
	p[2] = ppc;
	for (j = 2; j < np; j += 2)  p[j] = ppc * p[j-2];
	for (j = 3; j < np; j += 2)  p[j] = pps * p[j-2];
	return(0);
}


#ifdef NOT_USED
static int fcos2( x, p, np)	/* cos fourier decomp */
float	 x,
	*p;
int	 np;
{
  int    j;
  float  ppc;
	p[0] = 1.0;
	ppc = cos(M_PI * x);
	p[1] = ppc;
	for (j = 2; j < np; j++)  p[j] = ppc * p[j-1];
	return(0);
}
#endif


/* SECH * cos */
static int fsechcos(float x, float *p, int np)
{
  float	 ae = 6.0, ppc;
  int	 j;

	ppc = cos(M_PI * x);
	p[0] = exp(ae * x);
	p[0] += 1.0 / p[0];
	p[0] = 2.0 / p[0];
	p[1] = ppc * p[0];
	for (j = 2; j < np; j++)  p[j] = ppc * p[j-1];
	return(0);
}


#ifdef NOT_USED
static int fsech( x, p, np)	/* SECH * polynomial */
float	 x,
	*p;
int	 np;
{
  float	 ae = 20.0;
  int	 j;

	p[0] = exp(ae * x);
	p[0] += 1.0 / p[0];
	p[0] = 0.5 / p[0];
        for (j = 1; j < np; j++)  p[j] = p[j-1] * x;
	return(0);
}
#endif


static int fpoly( x, p, np )  /* for use with svdfit */ /* polynomial function */
float    x,
	*p;
int      np;            /* powers up to np-1 */
{
  int    j;
        p[0] = 1.0;
        for (j = 1; j < np; j++)  p[j] = p[j-1] * x;
	return(0);
}


static int gzfit_args(argc, argv, cmd_name, gspath, 
	weight_on, weight_fit, verbose_on, weight_output, btrigger_ad)
int argc;
char *argv[], cmd_name[], gspath[];
char weight_on[], weight_fit[], verbose_on[], weight_output[];
int *btrigger_ad;
{
	int btrigger;
	char userpath[MAXPATHL]; 
/*	 ARGUMENTS:
	 (1) filename prefix, i.e. 'gshim'
			OR
	     keyword for type of curve fit, i.e. 'poly'
	 (2) keyword for curve fit weighting if desired
	 (3) keyword for output type: normal, weight function, diff; weighted or unweighted output 
*/

/*	if ( argc > 5 ) 
		{
		Werrprintf( "Usage: %s('gshim')", argv[0] );
		return(-1);
		} */
	if (argc < 2 || argc > 5)
		{
/*		Werrprintf("Usage: %s(file_prefix<,'f|t|r|a|c|h|i|j|k|s|n'<,'n|w|d|c|b<n>'>>)", argv[0] ); */
		Werrprintf("Usage: %s('gshim')", argv[0] );
                return( 1 );
		}
	strcpy( cmd_name, argv[0] );

	if (argc > 1)
		{
		  if (argv[1][0] != '/')
		  {
		    strcpy(gspath, userdir);
		    strcat(gspath, "/");
		  }
		  else
		    strcpy(gspath, "");
		  strcat(gspath, argv[1] );
		}
	else {
		if (P_getstring( GLOBAL, "userdir", userpath, 1, MAXPATHL )!=0) {
			Werrprintf("WARNING: parameter 'userdir' does not exist");
			strcpy(gspath, userdir); /* from main() getenv */
			strcat(gspath, "/gshimlib/data/gshim" );  /* default value */
		} else {
			strcpy(gspath, userpath);
			strcat(gspath, "/gshimlib/data/gshim");  /* default value */
		}
		}

	btrigger = 0; /* trigger that we are reading basis functions from file */
	if (argc > 1) { 
                int len;
		if (strcmp(argv[1],"poly")==0)
		  btrigger = 4; 	/* polynomial fit, keep hidden */
                len = strlen(gspath) - 5;
		if ( (len > 0) && !strcmp(gspath+len, "/vmix") )
		  btrigger = 1; 	/* give extra output for vmix file */
		}

	strcpy( weight_on, "f"); 	/* default values */
	strcpy( weight_fit, "n");
	if (argc > 2) {	
		if (strcmp(argv[2],"t") == 0)  strcpy(weight_on, "t");
		else if (strcmp(argv[2],"n") == 0) { strcpy(weight_on,"n"); }
		else if (strcmp(argv[2],"c") == 0) { strcpy(weight_on,"f"); strcpy(weight_fit,"c"); }
		else if (strcmp(argv[2],"r") == 0) { strcpy(weight_on,"r"); }
		else if (strcmp(argv[2],"a") == 0) { strcpy(weight_on,"f"); strcpy(weight_fit,"a"); }
		else if (strcmp(argv[2],"h") == 0) { strcpy(weight_on,"h"); }
		else if (strcmp(argv[2],"i") == 0) { strcpy(weight_on,"i"); }
		else if (strcmp(argv[2],"j") == 0) { strcpy(weight_on,"f"); strcpy(weight_fit,"j"); }
		else if (strcmp(argv[2],"k") == 0) { strcpy(weight_on,"k"); }
		else if (strcmp(argv[2],"s") == 0) { strcpy(weight_on,"f"); strcpy(weight_fit,"s"); }
		}

	strcpy(verbose_on, "n");		/* default values */
	strcpy(weight_output, "y");
	if (argc > 3) { 			/* 1st char verbose_on, 2nd char weight_output */
		if (strcmp(argv[3],"n") == 0)  strcpy(verbose_on,"n");  		/* fit output */
		else if (strcmp(argv[3],"w") == 0)  { strcpy(verbose_on, "w"); }	/* show weighting function */
		else if (strcmp(argv[3],"d") == 0)  { strcpy(verbose_on, "d"); } 	/* residual difference */
		else if (strcmp(argv[3],"c") == 0)  { strcpy(verbose_on, "c"); }	/* show fit components */
		else if (strcmp(argv[3],"b") == 0)  { strcpy(verbose_on, "b"); }	/* show basis functions */
		else if (strcmp(argv[3],"e") == 0)  { strcpy(verbose_on, "e"); }	/* show components plus orig */
		else if (strcmp(argv[3],"nn") == 0) { strcpy(verbose_on, "n"); strcpy(weight_output,"n"); } 
		else if (strcmp(argv[3],"wn") == 0) { strcpy(verbose_on, "w"); }	/* weight_output irrelevant */
		else if (strcmp(argv[3],"dn") == 0) { strcpy(verbose_on, "d"); strcpy(weight_output,"n"); } 
		else if (strcmp(argv[3],"cn") == 0) { strcpy(verbose_on, "c"); strcpy(weight_output,"n"); } 
		else if (strcmp(argv[3],"bn") == 0) { strcpy(verbose_on, "b"); strcpy(weight_output,"n"); } 
		else if (strcmp(argv[3],"en") == 0) { strcpy(verbose_on, "e"); strcpy(weight_output,"n"); } 
		}
	*btrigger_ad = btrigger;

	return(0);
}


static int gzfit_read_listfile(listfile, mapname, shimsetnum, ma_list_ad, rmserr, btrigger,
	shimcal, shimoldv, shimval, shimadd, shimerr, shimname, cmd_name, output_list)
FILE *listfile;
char mapname[], shimsetnum[], shimname[][16], cmd_name[], output_list[];
int *ma_list_ad, btrigger;
float *rmserr;
float	 shimcal[],	/* calibration values or offsets */
	 shimoldv[],	/* old shim values */
	 shimval[],	/* new shim values */
	 shimadd[],	/* fitted shim values, shimval = shimoldv - shimadd, diff */
	 shimerr[];	/* error in each shim */
{
	int gzmaxsize=MAX_CURVES;
	int ma_list;
	int i, j, bflag, doread;
	float tmp = 0.0;
	char jstr[MAXPATHL];

        if (btrigger < 2)
	  for (i=0; i<gzmaxsize; i++)  sprintf(shimname[i], "z%d", i+1);
        else
	  for (i=0; i<gzmaxsize; i++)  sprintf(shimname[i], "z%d", i);
	*rmserr = 0.0;
	for (j = 0; j < 6; j++) {
		if ( fscanf(listfile, "%s", jstr) == EOF) {
			Werrprintf("%s: wrong header format for file '%s'", cmd_name, output_list );
                	return( 1 );
			}
		switch (j) {
			case 1 : strcpy(mapname, jstr); break;
			case 3 : strcpy(shimsetnum, jstr); break;
			case 5 : if (isReal( jstr ))  ma_list = atoi( jstr ); 
				 else ma_list = 4; break;
			default : break; 	/* 0 2 4 */
			}
		}
/*	if (ma_list > gzmaxsize) gzmaxsize = ma_list; */ /* for poly of order > 7 */
	for (i=0; i<gzmaxsize; i++) 
		{
		shimcal[i]=1.0;
		shimoldv[i]=shimval[i]=shimadd[i]=shimerr[i]=0.0;
		}

      if (btrigger < 2) { /* big loop, else do not read in list */

/*
	for (j = 0; j < 7; j++) {
		if ( fscanf(listfile, "%s", jstr) == EOF) {
			Werrprintf("%s: wrong header format for file %s", cmd_name, output_list );
                	return( 1 );
			}
		}
*/
	doread = 0;
	while (doread == 0)
	{
		if ( fscanf(listfile, "%s", jstr) == EOF) {
			Werrprintf("%s: wrong header format for file %s", cmd_name, output_list );
                	return( 1 );
			}
		if (strcmp(jstr,"err") == 0)
		{
		  if ( fscanf(listfile, "%s", jstr) == EOF) {
			Werrprintf("%s: wrong header format for file %s", cmd_name, output_list );
                	return( 1 );
			}
		  if (isReal( jstr ))   *rmserr = atof( jstr );
		  else			*rmserr = 0.0;
		}
		else if (strcmp(jstr,"Error") == 0)
		{
		  doread = 1;
		  if ( fscanf(listfile, "%s", jstr) == EOF) { 	/* read one extra word */
			Werrprintf("%s: wrong header format for file %s", cmd_name, output_list );
                	return( 1 );
			}
		}
	}

	for (i = 0; i < ma_list; i++)	/* dont need full structure in fscanf, but it shows format */
		{			/* should scan one at a time to catch EOF */
		if ( fscanf(listfile, "%s", jstr) == EOF) 	/* shim name */
			{
			Werrprintf("%s: row %d does not exist in file %s", cmd_name, i+1, output_list );
                	return( 1 );
			} 
		if ((jstr[0] == '-') && (jstr[1] == '-'))
			{
			Werrprintf("%s: row %d does not exist in file %s", cmd_name, i+1, output_list );
                	return( 1 );
			} 
		strcpy(shimname[i], jstr);

		bflag = 0;
		for (j=0; j<5; j++) {
			if ( fscanf( listfile, "%s", jstr) == EOF)
				{
				Werrprintf("%s: wrong format for shim '%s' in file %s", 
					cmd_name, shimname[i], output_list );
                		return( 1 );
				} 
			if (isReal( jstr )) { tmp = (float)(atof( jstr )); }
			else {		/* isReal doesn't catch if starts with digit */
				Werrprintf("%s: wrong format for shim '%s' in file %s", 
					cmd_name, shimname[i], output_list );
                		return( 1 );
				}
			switch (j) { 
				case 0 : shimcal[i] = tmp; 
/*					if (fabs(shimcal[i]) < TOL) {
					  Werrprintf("%s: shim offset '%s' is zero in %s\n",
						cmd_name, shimname[i], output_list);
					  bflag = 1;
					  } */
					break;
				case 1 : shimoldv[i] = tmp; break;
				case 2 : shimval[i] = tmp; break;
				case 3 : shimadd[i] = tmp; break;
				case 4 : shimerr[i] = tmp; break;
				default : ; /* break; */	/* could use shimcal++, shimcal[0] */
				}
			if (bflag == 1) {
                		return( 1 );
				}
			}
		}
			/* could fscanf(jstr), use (float)(atof(jstr)), declare atof as double */

	bflag = 0;
	if (ma_list < GZMINSIZE)
	    {
	    for (i = ma_list; i < GZMINSIZE; i++) 
		{
		if ( fscanf( listfile, "%s", jstr) == EOF)
			{
			printf("\n%s warning: row %d does not exist in file %s", cmd_name, i+1, output_list);
			break;
			}
		if ((jstr[0] == '-') && (jstr[1] == '-'))
			{
			printf("\n%s warning: row %d does not exist in file %s", cmd_name, i+1, output_list);
			break;
			} 
		strcpy(shimname[i], jstr);

		for (j=0; j<5; j++) {
			if ( fscanf( listfile, "%s", jstr) == EOF)
				{ 
				if (j == 0) {
			  printf("\ngzfit warning: no value read for shim offset '%s' in %s", shimname[i], output_list);
				  bflag = 1; break;
				  }
				else if (j == 1) {
			  printf("\ngzfit warning: no value read for shim '%s' in %s, assumed zero", 
					shimname[i], output_list);
				  bflag = 1; break;
				  }
				}
			if (isReal( jstr )) { tmp = (float)(atof( jstr )); }
			else if (j==0) {	/* isReal doesn't catch if starts with digit */
			  printf("\ngzfit warning: no value read for shim offset '%s' in %s", shimname[i], output_list);
				  bflag = 1; break;
			  }
			else if (j==1) {
			  printf("\ngzfit warning: no value read for shim '%s' in %s, assumed zero", 
					shimname[i], output_list);
				  bflag = 1; break;
				}
			switch (j) {
				case 0 : shimcal[i] = tmp; break;
				case 1 : shimoldv[i] = tmp; break;
				case 2 : shimval[i] = shimoldv[i]; break;
				default : ; 	/* break; */
				}
			}
		if (bflag == 1) break; /* need to break two loops */
		}
	    }
	} 	/* end btrigger<2 */

	*ma_list_ad = ma_list;
	return(0);
}


/************************************************************************/
int gzfit(int argc, char *argv[], int retc, char *retv[] )
/************************************************************************/
/* Fit the phase map with the basis functions		*/
{

	extern int Bnmr;

	char	 cmd_name[MAXPATHL],
		 input_name[MAXPATHL],
		 basis_name[MAXPATHL],
		 gspath[MAXPATHL];
	char	 output_name[MAXPATHL],
		 output_list[MAXPATHL],
		 output_list2[MAXPATHL],
		 wt_name[MAXPATHL],
		 weight_on[MAXPATHL],	/* triggers weighting function, read from file */
		 weight_fit[MAXPATHL],	/* triggers fitting weight file */
		 verbose_on[MAXPATHL],  /* display weighting function or residual diff */
		 weight_output[MAXPATHL]; /* triggers weighted output */

	FILE	*listfile, *infile, *b_file, *wtfile;

	int	 jint;

	int	 i, j, btrigger;
	float	*fbase; 	/* basis function data; size ma * ndata */
	float	 tmp, sw_edge;
/*	float	 syy, sy, sxy, sxx, sx, sig, soffset, sslope; */
	float	 shimcal[MAX_PARAMS],	/* calibration values or offsets */
		 shimoldv[MAX_PARAMS],	/* old shim values */
		 shimval[MAX_PARAMS],	/* new shim values */
		 shimadd[MAX_PARAMS],	/* fitted shim values, shimval = shimoldv - shimadd, diff */
		 shimerr[MAX_PARAMS];	/* error in each shim */
	char	 shimname[MAX_PARAMS][16];	/* shim names */
	int	 ma_list; /* if poly then >8 */
	char	 mapname[MAXPATHL], shimsetnum[4];
	float	 rmsfit, rmserr, a1tmp;

/* variables from svdfit
( x, y, sig, ndata, a, ma, u, v, w, chisq, funcs ) */
	float   *x, 		/* data set to be fitted, size ndata */
	        *y,		/* data set to be fitted, size ndata */
		*wt;		/* weighting function, size w_ndata */	/* remove u,v,sig */
	int      ndata;		/* number of data points */
	float   *a;		/* output fit parameters, size ma, maximum MAX_PARAMS */
	float	*da;		/* error in a, size ma */
	int      ma = 1;	/* number of parameters, number of shims plus one */
	float   *w, 		/* tmp data, size ma */
		 chisq;		/* chi-squared of fit */
	int	 b_ma, b_ndata;		/* number of basis sets, number of points per basis set */
	int	 w_ndata; 


/* RETURN values: retv[0]=0 ok; retv[0]=1 general failure; retv[0]=2 curve fit failure */
        EPRINT0(1,"Starting gzfit...  \n"); 

/* translate arguments */
	if ( gzfit_args(argc, argv, cmd_name, gspath,
		weight_on, weight_fit, verbose_on, weight_output, &btrigger) !=0 )
		{
                if (retc>0) retv[0] = intString(1);
		return(0);
		}

/* set file names */
	if (strcmp( weight_on, "f") == 0) 
		{
		strcpy( wt_name, gspath );
		strcat( wt_name, ".amp" );
		} 
	strcpy( basis_name, gspath ); strcat( basis_name, ".bas" );
	strcpy( input_name, gspath ); strcat( input_name, ".phase" );
	strcpy( output_name, gspath ); strcat( output_name, ".fit" );
	strcpy( output_list, gspath ); strcat( output_list, ".list" );
	strcpy( output_list2, output_list );
/*
printf("cmd=%s path=%s weight_on=%s weight_fit=%s verbose_on=%s weight_output=%s btrigger=%d\n",
	cmd_name, gspath, weight_on, weight_fit, verbose_on, weight_output, btrigger);
*/

/* read output_list as input, use to set up, if wrong format then abort */
	if ( access(output_list, F_OK) != 0 )
		{
		if (!Bnmr) {
			Winfoprintf( "%s:  file %s not found", cmd_name, output_list );
                	if (retc>0) retv[0] = intString(1);
                	return( 0 );
			}
		}
	listfile = fopen( output_list, "r" );
	if (listfile == NULL) {
		Werrprintf( "%s: problem opening %s", cmd_name, output_list);
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		}
	if (gzfit_read_listfile(listfile, mapname, shimsetnum, &ma_list, &rmserr, btrigger,
		shimcal, shimoldv, shimval, shimadd, shimerr, shimname, cmd_name, output_list) != 0)
		{
		fclose(listfile);
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		}
	for (j=0; j<ma_list; j++) /* for (j=0; j<gzmaxsize; j++) */
	    if (fabs(shimcal[j]) < TOL)
	    {
		Werrprintf("%s: offset for shim '%s' in 'gshim.list' is zero, aborting",cmd_name,shimname[j]);
                if (retc>0) retv[0] = intString(1);
                return( 0 );
	    }
/*
	printf("mapname=%s  shimset=%s  ma_list=%d\n", mapname, shimsetnum, ma_list);
	for (j=0; j<gzmaxsize; j++) 
		printf("%-8s %8.0f %11.0f %11.0f %11.0f %11.0f\n", shimname[j],
			shimcal[j], shimoldv[j], shimval[j], shimadd[j], shimerr[j] );
	printf("--------------------------------------------\n");
*/
	fclose( listfile );


/* read data to be fitted from file; 
	contains x[], y[], ndata; format as wrspec (expl) */

	if (access( input_name, F_OK ) != 0) 	
		{
		if (!Bnmr) {
			Winfoprintf( "%s:  file %s not found", cmd_name, input_name );
                	if (retc>0) retv[0] = intString(1);
                	return( 0 );
			} 
		}
	infile = fopen( input_name, "r" );
	if (infile == NULL) {
		Werrprintf( "%s: problem opening %s", cmd_name, input_name );
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		} 
	if ( gzfit_read_header(infile, input_name, &ndata, &jint, cmd_name) != 0)  { 
		fclose( infile );
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		}
        if ((x = (float *)malloc(sizeof(float) * ndata))==NULL) {
		fclose( infile );
                Werrprintf("gzfit: error allocating curve fit buffer");
                if (retc>0) retv[0] = intString(1);
                return( 0 );
                }
        if ((y = (float *)malloc(sizeof(float) * ndata))==NULL) {
		free((char *)x); fclose( infile );
                Werrprintf("gzfit: error allocating curve fit buffer");
                if (retc>0) retv[0] = intString(1);
                return( 0 );
                }
/* initialize arrays, set dimensions to max */
	for( i = 0; i < ndata; i++ ) x[i] = 0.0;
	for( i = 0; i < ndata; i++ ) y[i] = 0.0;
	if (gzfit_read_input_body(infile, input_name, cmd_name, x,y,ndata,&sw_edge) != 0) {
		fclose( infile );
		free((char *)x); free((char *)y);
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		} 
	fclose( infile ); 

/* set weight array wt, read from file if asked */
        if ((wt = (float *)malloc(sizeof(float) * ndata))==NULL) {	/* wt = 1/sig */
		free((char *)x); free((char *)y);
                Werrprintf("gzfit: error allocating curve fit buffer");
                if (retc>0) retv[0] = intString(1);
                return( 0 );
                }
	for( i = 0; i < ndata; i++ ) wt[i] = 1.0; 
	if (strcmp(weight_on,"f")==0) {		/* read weight file */

        if (access( wt_name, F_OK ) != 0)    /* access file */
                {
                if (!Bnmr) {
                        Winfoprintf( "%s: file not found %s", cmd_name, wt_name );
			strcpy( weight_on, "r");
			}
                }
	  if (strcmp(weight_on,"f")==0) {		/* read weight file */
        wtfile = fopen( wt_name, "r" );      /* open file */
        if (wtfile == NULL) {
                Werrprintf( "%s: problem opening %s", cmd_name, wt_name );
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		}
	if ( gzfit_read_header(wtfile, wt_name, &w_ndata, &jint, cmd_name) != 0)  { 
		fclose( wtfile );
		free((char *)x); free((char *)y); free((char *)wt);
		Werrprintf("%s: error reading %s", cmd_name, wt_name);
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		}
	if (gzfit_read_wt_body(wtfile, wt_name, cmd_name, wt, ndata, w_ndata) != 0) {
		fclose( wtfile );
		free((char *)x); free((char *)y); free((char *)wt);
		Werrprintf("%s: error reading %s", cmd_name, wt_name);
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		} 
	fclose( wtfile );

	  } /* end if weight_on == "f" */ /* inner loop */
	} /* end if weight_on == "f" */

/* call weight calc - for Tchebyshev fit, or to normalize wt file) */
	if( gzfit_wt_calc(weight_on, weight_fit, wt, x, ndata) != 0)  {
		free((char *)wt); free((char *)x); free((char *)y);
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		}

/* DO NOT USE THIS SECTION */
/* could weight all by chisq after fit, or chisq over a central region (weights norm to 1.0) */
/* weight by input data, avg. middle six points */
#ifdef DO_NOT_USE
	syy = 0; sy = 0; sxy = 0; sxx = 0; sx = 0; sig = 0; soffset = 0; sslope = 0;
 	for( i = (ndata/2) - 3; i < (ndata/2) + 3; i++ ) { 
 		sy += y[i]; sxy = x[i] * y[i]; sxx += x[i] * x[i]; sx += x[i]; syy += y[i] * y[i];
 		}
 		printf( "\nweighting point using slope\n" );
 		soffset = sy / 6;
 		sslope = sxy / sxx;


/*              severe roundoff error in this one */
		sig = syy - (sy * sy / 6) - (sxy * sxy / sxx);
 		for( i = (ndata/2) - 3; i < (ndata/2) + 3; i++ ) {
 			tmp = (y[i] - soffset - sslope * x[i]);
 			sig += tmp * tmp;
 			printf(" sig = %g, ", sig ); 
 			} 
 		sig = sqrtf( sig/6 ); 
     printf( "sy %g, syy %g, sxy %g, sx %g, sxx %g, d %g, sig %g\n", sy,syy,sxy,sx,sxx,(sxx-sx*sx), sig );
 	for( i=0; i<ndata; i++) wt[i] /= sig; 
#endif
/* weighting for actual errors taken care of with chisq instead of measured sig */


/* xxxxxxxxxxxxxxxxxxxx */

/* read phase maps of basis functions
	OR define basis functions as polynomial; 
	contains ma number of basis functions */

	if (btrigger > 1) 	/* see basis_name == "poly" or "p" */
	{
		/* take ma from output_list */
		if (ma_list < 2) {
			Werrprintf("%s warning: gzsize out of bounds", cmd_name );
			ma_list = 2;
			}
		if (ma_list > (ndata - 1)) {
			Werrprintf("%s warning: gzsize out of bounds", cmd_name );
			ma_list = ndata - 1;
			}
		ma = ma_list;

        	if ((fbase = (float *)malloc(sizeof(float) * ndata * ma))==NULL) {
                	Werrprintf("gzfit: error allocating curve fit buffer");
			free((char *)wt); free((char *)x); free((char *)y);
                	if (retc>0) retv[0] = intString(1);
                	return( 0 );
                	}

		for(i=0; i<ma*ndata; i++) fbase[i]=1.0;

/*		*** assign fbase values from fpoly or (*funcs)() ***/
        	if ((w = (float *)malloc(sizeof(float) * ma))==NULL) {
                	Werrprintf("gzfit: error allocating curve fit buffer");
                        free((char *)x); free((char *)y); free((char *)wt); free((char *)fbase);
                	if (retc>0) retv[0] = intString(1);
                	return( 0 );
                	}

		if (btrigger == 4) {
		for( i=0; i<ndata; i++ ) {
			if( fpoly(x[i],w,ma) != 0) { 
				Werrprintf( "%s: polynomial calc failed", cmd_name ); 
				free((char *)wt); free((char *)x); free((char *)y);
                		if (retc>0) retv[0] = intString(1);
                		return( 0 );
				}
			for( j=0; j<ma; j++ ) { fbase[i+ndata*j] = w[j]; }
			}
		}
		free((char *)w);
		EPRINT1(1, "  Polynomial basis function defined, order %d.\n", ma-1 ); 

	}

	else if (btrigger < 2) 	/* read basis file */
	{
		if (access( basis_name, F_OK ) != 0)    /* access file */
			{
			if (!Bnmr) {
			Winfoprintf( "%s: file %s not found", cmd_name, input_name );
                	if (retc>0) retv[0] = intString(1);
                	return( 0 );
			}
			}
		b_file = fopen( basis_name, "r" );      /* open file */
		if (b_file == NULL) {
			Werrprintf( "%s: problem opening %s", cmd_name, basis_name );
                	if (retc>0) retv[0] = intString(1);
                	return( 0 );
			}
	if ( gzfit_read_header(b_file, basis_name, &b_ndata, &b_ma, cmd_name) != 0)  { 
		fclose( b_file );
		free((char *)x); free((char *)y); free((char *)wt);
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		}
	if (b_ma < 1 || b_ma > b_ndata) {
		Werrprintf( "%s error: %d data sets in file %s", cmd_name, b_ma, basis_name );
		free((char *)x); free((char *)y); free((char *)wt);
		fclose( b_file );
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		}
	if (b_ndata < ndata) {
		Werrprintf( "%s error: gzwin too large!", cmd_name );
		free((char *)x); free((char *)y); free((char *)wt);
		fclose( b_file ); 
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		} 
	if (ma_list > b_ma)  ma_list = b_ma;

/*	if ( P_getreal(CURRENT, "gzsize", &gcut, 1) == 0 ) { 
		s_ma = (int) (gcut); 
		if (s_ma > b_ma) {
			gcut = (double) b_ma;
			P_setreal(CURRENT, "gzsize", gcut, 1);
			}
		} */

	ma = ma_list + 1;

        if ((fbase = (float *)malloc(sizeof(float) * ndata * ma))==NULL) {
                Werrprintf("gzfit: error allocating curve fit buffer");
                free((char *)x); free((char *)y); free((char *)wt);
                if (retc>0) retv[0] = intString(1);
                return( 0 );
                }
	if ( gzfit_read_b_body(b_file, basis_name, cmd_name, ndata, b_ndata, ma_list, fbase) != 0) { 
                free((char *)x); free((char *)y); free((char *)wt); free((char *)fbase);
		fclose( b_file );
                if (retc>0) retv[0] = intString(1);
                return( 0 );
		} 
	fclose( b_file );
	EPRINT2(1, "  Basis file %s read, %d data sets.\n", basis_name, b_ma ); 

	} /* end if (btrigger < 2) */


/* calculate fit - 
	given all variables above, 
	calculate ma coefficients a[] */

        if ((a = (float *)malloc(sizeof(float) * ma))==NULL) {
                Werrprintf("gzfit: error allocating curve fit buffer");
                free((char *)x); free((char *)y); free((char *)wt); free((char *)fbase); 
		if (retc>0) retv[0] = intString(1);
		return( 0 );
                }
        if ((da = (float *)malloc(sizeof(float) * ma))==NULL) {
                Werrprintf("gzfit: error allocating curve fit buffer");
                free((char *)x); free((char *)y); free((char *)wt); free((char *)fbase);
		free((char *)a);
		if (retc>0) retv[0] = intString(1);
		return( 0 );
                }

	for( i = 0; i < ma; i++ ) { a[i] = 0.0; da[i] = 0.0; }

	EPRINT0(1, "  calling svdfit...  " ); 
	if( svdfit(y,wt,ndata,a,da,ma,&chisq,fbase) != 0)
		{
/* 		errors reported by svdfit 			*/
		free((char *)x); free((char *)y); free((char *)wt);
		free((char *)a); free((char *)da); free((char *)fbase);
		if (retc>0) retv[0] = intString(2);
		return( 0 );
		} 

	if (btrigger<2 && ma>2 && (strcmp(shimname[0],"z1c")==0) /* && (strcmp(shimsetnum,"5")==0) */ ) 
		{
/* keep a[1] as int, subtract, redo curve fit */ 
/* need to subtract z1 from other basis functions?? otherwise wrong da */
/* keep old da[i], so long as a[0]>0, see svdfit */
		a1tmp = (float)((int)(a[1]*shimcal[0])) / shimcal[0];
		ma -= 1;
		for (i=0; i<ndata; i++)
			{
			y[i] -= a1tmp * fbase[i+ndata]; 
			tmp = fbase[i+ndata];
			fbase[i+ndata] = fbase[i+ndata*ma];
			fbase[i+ndata*ma] = tmp;
			}
		EPRINT0(1, "  calling svdfit twice...  " ); 
		if( svdfit(y,wt,ndata,a,da,ma,&chisq,fbase) != 0)
			{
			free((char *)x); free((char *)y); free((char *)wt);
			free((char *)a); free((char *)da); free((char *)fbase);
			if (retc>0) retv[0] = intString(2);
			return( 0 );
			} 
		for (i=0; i<ndata; i++)
			{
			tmp = fbase[i+ndata];
			fbase[i+ndata] = fbase[i+ndata*ma];
			fbase[i+ndata*ma] = tmp;
			y[i] += a1tmp * fbase[i+ndata]; 
			}
		a[ma] = a[1];
		a[1] = a1tmp;
		ma += 1;
		}

	if ((ndata > ma) && (chisq > TOL)) { tmp = sqrtf( chisq / (ndata - ma) ); }
	else { tmp = 1.0; }
	for (i=0; i<ma; i++) da[i] *= tmp;	/* normalized error */

/* output values of a[] */

        EPRINT0(2,"  Fit values: ");
        for (j=0; j<ma; j++) EPRINT2(2,"a[%d] %f, ",j,a[j]);
        EPRINT0(2,"\n");
	EPRINT0(2,"  Fit errors: ");
	for (j=0; j<ma; j++) EPRINT2(2,"da[%d] %f, ",j,da[j]);
	EPRINT0(2,"\n");

	if (btrigger < 2) {
		for (j=1; j<=ma_list; j++) {
			shimadd[j-1] = a[j] * shimcal[j-1];
			shimerr[j-1] = da[j] * fabs(shimcal[j-1]);
			shimval[j-1] = shimoldv[j-1] - shimadd[j-1];
			}
		}
	else {
		for (j=1; j<=ma_list; j++) {
			shimadd[j-1] = a[j-1]; 
			shimerr[j-1] = da[j-1];
			}
		}

	rmsfit = 0; rmserr = 0;
	for (j=0; j<ma_list; j++) { 
		rmsfit += shimadd[j] * shimadd[j];
		if (fabs(shimerr[j]) > TOL) {
		  tmp = shimadd[j] / shimerr[j];
		  rmserr += tmp * tmp;
		} 
		}
	rmsfit = sqrtf( rmsfit ) / ma_list;  
	rmserr = sqrtf( rmserr ) / ma_list;	
	EPRINT1(2, "    rms ratio of fit to std dev %g\n", rmserr ); 
	EPRINT0(1, "    ...svdfit done.\n" );

/* write output files gshim.list, gshim.fit */
	if (gzfit_output_files(output_list2, mapname, shimsetnum, ma_list, sw_edge, chisq,
	  rmsfit, rmserr, cmd_name, btrigger, shimname, shimcal, shimoldv, shimval, shimadd, shimerr,
	  output_name, ma, ndata, x, y, wt, fbase, a, da, weight_output, verbose_on) != 0)
	{
		free((char *)x); free((char *)y); free((char *)wt);
		free((char *)a); free((char *)da); free((char *)fbase);
               	if (retc>0) retv[0] = intString(1);
               	return( 0 );
	}

/* could output singularities found, non-orthogonal basis vectors,
	overdetermined system - possibly redo
	with smaller basis set	?????????????????  */

	free((char *)x);
	free((char *)y);
	free((char *)wt);
	free((char *)a);
	free((char *)da);
        free((char *)fbase);

	EPRINT0(1,"  ...gzfit done!\n");

	if (retc>0) retv[0] = intString(0);
	return( 0 );

}

static int greadfunc_args(argv, argc, userpath, readfunc_name, readfunc_type, input_list, mapname, cmd_name, jval)
int argc;
char *argv[];
char input_list[], mapname[], cmd_name[], userpath[];
char readfunc_name[];
int *readfunc_type, *jval;
{
	int argnum = 1; /* cmd_name = argv[0] already used */

	*readfunc_type = 0;
	*jval = 0;
	if (argc > argnum)
	{
		strcpy( readfunc_name, argv[argnum] );
		argnum++;
	}
	else 
		strcpy( readfunc_name, "Z" );
	greadfunc_settype( readfunc_type, readfunc_name );
	if (*readfunc_type == -1)
	{
		if (readfunc_name[0] != 'Z')
		  Werrprintf("%s( '%s' ): invalid argument\n", cmd_name, readfunc_name );
                return( -1 );
	}
	if ((*readfunc_type == 0) || (*readfunc_type == 1))
	{
		if (argc > argnum)
		{
		    if (isReal( argv[argnum] )) *jval = atoi( argv[argnum] );
                    else *jval = 0;
		    argnum++;
		}
	}
	else if (*readfunc_type == 5)
	{
		if (argc > argnum)
		{
			strcpy( mapname, argv[argnum] );
			argnum++;
		}
		else
			strcpy( mapname, "standard" );
	}
	if (P_getstring( GLOBAL, "userdir", userpath, 1, MAXPATHL )!=0)
	{
		Werrprintf( "%s: parameter 'userdir' not found", cmd_name );
		return( -1 );
	}
	if (argc > argnum)
	{
		strcpy( input_list, argv[argnum] );
		argnum++;
	}
	else 
	{
		strcpy(input_list, userpath);
		strcat(input_list, "/gshimlib/data/gshim.list"); 
	}
	return(0);
}

static int greadfunc_settype(int *readfunc_type, char *readfunc_name )
{
	if (strcmp(readfunc_name, "setoldv") == 0)
		*readfunc_type =  0;
	else if (strcmp(readfunc_name, "setnew") == 0)
		*readfunc_type =  1;
	else if (strcmp(readfunc_name, "setzshims") == 0)
		*readfunc_type =  2;
	else if (strcmp(readfunc_name, "readzshims") == 0)
		*readfunc_type =  3;
	else if (strcmp(readfunc_name, "unarray") == 0)
		*readfunc_type =  4;
	else if (strcmp(readfunc_name, "wrshimcal") == 0)
		*readfunc_type =  5;
	else if (strcmp(readfunc_name, "chkshimcal") == 0)
		*readfunc_type =  6;
	else
		*readfunc_type = -1;

	return(0);
}

static int gfunc_setoldv(gzsize, gzlist, shimname, shimoldv, shimval, shimadd, shimerr, 
		userpath, cmd_name, rmserr, shimsetnum, gzmaxsize)
int *gzsize, *gzlist, gzmaxsize;
char shimname[][16], userpath[], cmd_name[], shimsetnum[];
float shimoldv[], shimval[], shimadd[], shimerr[], *rmserr;
{
	int j, gzbase;
	double dtmp;
	char basename[MAXPATHL], jstr[MAXPATHL];
	FILE *bfile;

	*rmserr = 0;
	strcpy(basename, userpath);
	strcat(basename, "/gshimlib/data/gshim.bas"); 
	if (access(basename, F_OK) != 0)
	{
		if (!Bnmr)
			Werrprintf( "%s:  file '%s' not found", cmd_name, basename );
               	return( -1 );
	}
	bfile = fopen( basename, "r" );
	if (bfile == NULL) {
		Werrprintf( "%s: problem opening %s", cmd_name, basename);
                return( -1 );
		}
	for (j = 0; j < 3; j++) {
		if ( fscanf(bfile, "%s", jstr) == EOF) {
			Werrprintf("%s: wrong header format for file %s", cmd_name, basename );
                        fclose( bfile );
                	return( -1 );
			}
		switch (j) {
			case 2 : if (isReal( jstr ))  gzbase = atoi( jstr ); 
				 else gzbase = gzmaxsize; break;
			default : break; 	/* 0 1 */
			}
		}
	fclose( bfile );
	if (*gzsize < 1)         *gzsize = *gzlist;
	if (*gzsize > gzbase)    *gzsize = gzbase;
	if (*gzsize > gzmaxsize) *gzsize = gzmaxsize;

	for (j = 0; j < GZMINSIZE; j++)
	{
/*		if (P_getreal(PROCESSED, shimname[j], &dtmp, 1) != 0) */ /* for real use?? */
		if (P_getreal(CURRENT, shimname[j], &dtmp, 1) != 0)
		{
			Werrprintf("%s: parameter '%s' not found\n", cmd_name, shimname[j]);
			return( -1 );
			break;
		}
		else
		{
			shimoldv[j] = (float) dtmp;
			shimval[j]  = (float) dtmp;
			shimadd[j]  = 0.0;
			shimerr[j]  = 0.0;
		}
	}
	if (P_getreal(SYSTEMGLOBAL, "shimset", &dtmp, 1) != 0)
	{
		Werrprintf( "%s: parameter 'shimset' not found", cmd_name );
		return( -1 );
	}
	else sprintf(shimsetnum, "%g", dtmp);
	return( 0 );
}

static int gfunc_setnew(gzlist, gzsize, shimname, shimoldv, shimval, shimadd,
	shimerr, shimsetnum, execstr, rmserr, userpath, cmd_name)
int gzlist, *gzsize;
char shimname[][16], shimsetnum[], execstr[], userpath[], cmd_name[];
float shimoldv[], shimval[], shimadd[], shimerr[], *rmserr;
{
	float shimlimit;
	double dtmp;
	int i, toobig = -1, retval = 0;
	char jstr[MAXPATHL], recname[MAXPATHL];
	FILE *bfile;

	strcpy(recname, userpath);
	strcat(recname, "/gshimlib/data/gshim.rec"); 
	bfile = fopen( recname, "a" );
	if (bfile == NULL)
		Werrprintf( "%s: problem opening %s", cmd_name, recname);
	if (bfile != NULL)
		fprintf(bfile,"Updating shims %s-%s.\n",shimname[0],shimname[gzlist-1]);
	Winfoprintf("Updating shims %s-%s.",shimname[0],shimname[gzlist-1]);

	*gzsize = gzlist;
	shimlimit = 32767;
	if ((strcmp(shimsetnum,"1")==0) || (strcmp(shimsetnum,"2")==0) ||
	    (strcmp(shimsetnum,"10")==0) || (strcmp(shimsetnum,"11")==0))
		shimlimit = 2047;
	for (i=0; i<gzlist; i++)
	{
		if (shimval[i] > shimlimit)
		{
			toobig = i;
			shimval[i] = shimlimit;
		}
		if (shimval[i] < -shimlimit)
		{
			toobig = i;
			shimval[i] = -shimlimit;
		}
	}
	if (toobig > -1)
	{
		*gzsize = toobig;
		if (toobig == 0)
		{
			retval = 2; /* stop acq. */
			Werrprintf("shim %s out-of-range, shimming aborted", shimname[toobig] );
			if (bfile != NULL)
			    fprintf(bfile,"shim %s out-of-range, shimming aborted\n", shimname[toobig] );
		}
		else
		{
			retval = 1; /* do not stop acquisition */
			Werrprintf("shim %s out-of-range", shimname[toobig] );
			if (bfile != NULL)
			    fprintf(bfile,"shim %s out-of-range\n", shimname[toobig] );
		}
	}

	strcpy( execstr, "\0" );
        if (retval < 2)
        {
	    strcpy( execstr, "sethw(" );
	    if (shimsetnum[0] == '5') strcat(execstr, "'nowait'," );
	    dtmp = (double)shimval[0];
/*	    if (P_setreal(PROCESSED, shimname[0], dtmp, 1) != 0) */ /* for real use?? */
	    if (P_setreal(CURRENT, shimname[0], dtmp, 1) != 0)
	    {
		Werrprintf("parameter '%s' not found\n", shimname[0]);
		retval = -1;
	    }
	    strcat( execstr, "\'" ); strcat( execstr, shimname[0] ); strcat( execstr, "\'," );
	    sprintf( jstr, "%g", dtmp ); strcat( execstr, jstr );
	    if (gzlist > 1)
	        for (i=1; i<gzlist; i++)
	        {
		    dtmp = (double)shimval[i];
/*		    if (P_setreal(PROCESSED, shimname[i], dtmp, 1) != 0) */ /* for real use?? */
		    if (P_setreal(CURRENT, shimname[i], dtmp, 1) != 0) 
		    {
			Werrprintf("parameter '%s' not found\n", shimname[i]);
			retval = -1;
			break;
		    }
		    strcat( execstr, ",\'" ); strcat( execstr, shimname[i] ); strcat( execstr, "\'," );
		    sprintf( jstr, "%g", dtmp ); strcat( execstr, jstr );
	        }
	    strcat( execstr, ")\0" );
        }
	if (retval == 1)
	{
		*rmserr = 0;
		for (i=0; i<gzlist; i++)
		{
			shimoldv[i] = shimval[i];
			shimadd[i]  = 0.0;
			shimerr[i]  = 0.0;
		}
	}
	if (bfile != NULL) fclose(bfile);
	return(retval);
}

static int gfunc_setzshims(shimname, gzlist, shimsetnum, execstr)
char shimname[][16], shimsetnum[], execstr[];
int gzlist;
{
	int j;
	strcpy( execstr, "sethw(" );
	if (shimsetnum[0] == '5') strcat(execstr, "\'nowait\'," );
	strcat( execstr, "\'" ); strcat( execstr, shimname[0] ); strcat( execstr, "\'," );
	strcat( execstr, shimname[0] );
        if (gzlist > 1)
	    for (j = 1; j < gzlist; j++)
	    {
		strcat( execstr, ",\'" ); strcat( execstr, shimname[j] ); strcat( execstr, "\'," );
		strcat( execstr, shimname[j] );
	    }
/*	strcat( execstr, ")\0" ); */
	strcat( execstr, ")" );
	return(0);
}

static int gfunc_readzshims(shimname, gzlist, execstr)
char shimname[][16], execstr[];
int gzlist;
{
	int j;
	char addstr[MAXPATHL];
	strcpy( execstr, "readhw(" );
	strcpy( addstr,  "\0" );
	strcat( execstr, "\'" ); strcat( execstr, shimname[0] ); strcat( execstr, "\'" );
	strcat( addstr, shimname[0] );
        if (gzlist > 1)
	    for (j = 1; j < gzlist; j++)
	    {
		strcat( execstr, ",\'" ); strcat( execstr, shimname[j] ); strcat( execstr, "\'" );
		strcat( addstr, "," ); strcat( addstr, shimname[j] );
	    }
	strcat( execstr, "):" );
	strcat( execstr, addstr );
	return(0);
}

static int gfunc_unarrayshims(shimname, gzlist, gzmaxsize, jval, execstr)
char shimname[][16], execstr[];
int gzlist, gzmaxsize, jval;
{
	int j;
	char jstr[MAXPATHL];

	strcpy( execstr, "\0" );
	if (jval == 1) /* gshim.list not found */
	{
	    gzlist = GZMINSIZE;
	    for (j = 0; j < gzlist; j++)
	    {
		sprintf( jstr, "z%d", j+1 );
		strcat( execstr, jstr ); strcat( execstr, "=" );
		strcat( execstr, jstr ); strcat( execstr, "[1] " );
	    }
	    for (j = 0; j < gzlist/2; j++)
	    {
		sprintf( jstr, "z%dc", j+1 );
		strcat( execstr, jstr ); strcat( execstr, "=" );
		strcat( execstr, jstr ); strcat( execstr, "[1] " );
	    }
	}
	else /* use shimname[] from gshim.list */
	{
	    if (gzlist > gzmaxsize) gzlist = gzmaxsize;
	    for (j = 0; j < gzlist; j++)
	    {
		strcat( execstr, shimname[j] ); strcat( execstr, "=" );
		strcat( execstr, shimname[j] ); strcat( execstr, "[1] " );
	    }
	}
	strcat( execstr, "\0" );
	return(0);
}

static int gfunc_wrshimcal(cmd_name,shimname,shimcal,shimoldv,shimval,shimadd,shimerr,
  gzsize,gzmaxsize,shimsetnum,rmserr)
char	 shimsetnum[],
	 cmd_name[],
	 shimname[][16];	/* shim names */
float	 shimcal[],	/* calibration values or offsets */
	 shimoldv[],	/* old shim values */
	 shimval[],	/* new shim values */
	 shimadd[],	/* fitted shim values, shimval = shimoldv - shimadd, diff */
	 shimerr[],	/* error in each shim */
	 *rmserr;
int	 *gzsize, gzmaxsize;
{
	int bsize = 4, jsize, ishimset, j;
	double dtmp;
        char calFile[MAXPATH];
        FILE *fd;

	if (P_getreal(SYSTEMGLOBAL, "shimset", &dtmp, 1) != 0)
	{
		Werrprintf( "%s: parameter 'shimset' not found", cmd_name );
		return( -1 );
	}
	else sprintf(shimsetnum, "%g", dtmp);
	ishimset = (int)(dtmp + 0.5);
	for (j=0; j<gzmaxsize; j++) sprintf( shimname[j], "z%d", j+1 );
	if ((ishimset == 1) || (ishimset == 2) || (ishimset == 10))
		for (j=0; j<2; j++) sprintf( shimname[j], "z%dc", j+1 );
	else if (ishimset == 5)
		for (j=0; j<4; j++) sprintf( shimname[j], "z%dc", j+1 );
        sprintf(calFile,"%s/acq/shimcal%d",systemdir,ishimset);
        if ( (fd = fopen( calFile, "r" )) )
        {
           int val;
           char str[MAXPATH];

           if ( fscanf(fd,"%d%[^\n]\n", &val, str) >= 1)
              bsize = val;
           j = 0;
           val=200;
           while ( (fscanf(fd,"%d%[^\n]\n", &val, str) >= 1) && (j < gzmaxsize) )
           {
              shimcal[j] = val;
              j++;
           }
           fclose(fd);
           while ( j < gzmaxsize )
           {
              shimcal[j] = val;
              j++;
           }
        }
        else
        {
	   switch (ishimset)
	   {
		case 1:
			bsize = 4;
			shimcal[0] = shimcal[1] = 50;
			shimcal[2] = shimcal[3] = 200;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 800;
		  break;
		case 2:
			bsize = 5;
			shimcal[0] = shimcal[1] = 50;
			shimcal[2] = shimcal[3] = 200;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 800;
		  break;
		case 3:
			bsize = 6;
			shimcal[0] = 400; shimcal[1] = shimcal[2] = 800;
			shimcal[3] = 3200;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 6400;
		  break;
		case 4:  /* Agilent 28 shims */
                case 19: /* Agilent 28 Thin (51mm)  */
                case 20: /* Agilent 32 shims */
                case 21: /* Agilent 24 shims */
                case 22: /* Oxford  28 shims */
                case 23: /* Agilent 28 Thin (54mm) */
                case 24: /* PC+     27 shims */
                case 25: /* PC+     28 shims */
                case 26: /* Agilent 32 (28 shims) */
                case 27: /* Agilent 40 (28 shims) */
			bsize = 7;
			shimcal[0] = shimcal[1] = 800;
			shimcal[2] = shimcal[3] = 3200;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 6400;
		  break;
		case 5:
			bsize = 8;
			shimcal[0] = 200;
			shimcal[1] = shimcal[2] = shimcal[3] = 800;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 3200;
		  break;
		case 6:
			bsize = 5;
			shimcal[0] = shimcal[1] = 800;
			shimcal[2] = shimcal[3] = 3200;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 4800;
		  break;
		case 7:   /* Agilent 21 shims */
			bsize = 6;
			shimcal[0] = shimcal[1] = 800;
			shimcal[2] = shimcal[3] = 3200;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 6400;
		  break;
		case 8:
			bsize = 4;
			shimcal[0] = shimcal[1] = 800;
			for (j=2; j<gzmaxsize; j++) shimcal[j] = 3200;
		  break;
		case 9:
			bsize = 7;
			shimcal[0] = shimcal[1] = 800;
			shimcal[2] = shimcal[3] = 3200;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 4800;
		  break;
		case 10:
			bsize = 5;
			shimcal[0] = shimcal[1] = 50;
			shimcal[2] = shimcal[3] = 200;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 1200;
		  break;
		case 11:
			bsize = 2;
			shimcal[0] = shimcal[1] = 50;
			shimcal[2] = shimcal[3] = 200;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 800;
		  break;
		case 12:
			bsize = 5;
			shimcal[0] = shimcal[1] = 800;
			shimcal[2] = shimcal[3] = 3200;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 4800;
		  break;
		case 13:
			bsize = 6;
			shimcal[0] = shimcal[1] = 800;
			shimcal[2] = shimcal[3] = 3200;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 4800;
		  break;
		case 14:
			bsize = 6;
			shimcal[0] = shimcal[1] = 800;
			shimcal[2] = shimcal[3] = 3200;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 4800;
		  break;
		case 17:  /* Agilent 27 shims */
			bsize = 6;
			shimcal[0] = shimcal[1] = 800;
			shimcal[2] = 2000;
			shimcal[3] = 3200;
			for (j=4; j<gzmaxsize; j++) shimcal[j] = 6400;
		  break;
		default:
                        bsize = 4;
                        shimcal[0] = shimcal[1] = 200;
                        for (j=2; j<gzmaxsize; j++) shimcal[j] = 800;

		  break;
	   }
        }

	jsize = bsize;
	if (jsize < GZMINSIZE) jsize = GZMINSIZE;
	for (j = 0; j < jsize; j++)
	{
/*		if (P_getreal(PROCESSED, shimname[j], &dtmp, 1) != 0) */ /* for real use?? */
		if (P_getreal(CURRENT, shimname[j], &dtmp, 1) != 0)
		{
			Werrprintf("%s: parameter '%s' not found\n", cmd_name, shimname[j]);
			return( -1 );
			break;
		}
		else
		{
			shimadd[j] = 0.0;
			shimerr[j] = 0.0;
			shimoldv[j] = (float) dtmp;
			shimval[j]  = (float) dtmp;
			if ((int)(dtmp + 0.5) > 0) shimcal[j] = -shimcal[j];
		}
	}
	*gzsize = bsize;
	if (P_getreal(CURRENT, "gzsize", &dtmp, 1) != 0)
		Werrprintf("parameter 'gzsize' not found");
	else
	{
		j = (int)(dtmp + 0.5);
		if (j > bsize)
		{
			P_setreal(CURRENT, "gzsize", (double)bsize, 1);
			Werrprintf("gzsize reset to max of %d", bsize);
			j = bsize;
		}
		bsize = j;
		*gzsize = bsize;
	}
	*rmserr = 0.0;
	return(0);
}

static int gfunc_chkshimcal(cmd_name,shimname,shimcal,shimoldv,shimval,shimadd,shimerr,
gzsize,gzmaxsize,shimsetnum,rmserr)
char	 shimsetnum[],
	 cmd_name[],
	 shimname[][16];	/* shim names */
float	 shimcal[],	/* calibration values or offsets */
	 shimoldv[],	/* old shim values */
	 shimval[],	/* new shim values */
	 shimadd[],	/* fitted shim values, shimval = shimoldv - shimadd, diff */
	 shimerr[],	/* error in each shim */
	 *rmserr;
int	 *gzsize, gzmaxsize;
{
	int bsize, ishimset, j;
	double dtmp;
	float shimlimit;

	bsize = *gzsize;
	if (P_getreal(CURRENT, "gzsize", &dtmp, 1) == 0)
		bsize = (int)(dtmp + 0.5);
	if (bsize > gzmaxsize)
		bsize = gzmaxsize;
	*gzsize = bsize;
	*rmserr = 0.0;

	if (P_getreal(SYSTEMGLOBAL, "shimset", &dtmp, 1) != 0)
	{
		Werrprintf( "%s: parameter 'shimset' not found", cmd_name );
		return( -1 );
	}
	else sprintf(shimsetnum, "%g", dtmp);
	ishimset = (int)(dtmp + 0.5);
	switch (ishimset)
	{
		case 1: case 2: case 10: case 11:
			shimlimit = 2047;
			break;
		default:
			shimlimit = 32767;
			break;
	}
	for (j=0; j < bsize; j++)
	{
/*		if (P_getreal(PROCESSED, shimname[j], &dtmp, 1) != 0) */ /* for real use?? */
		if (P_getreal(CURRENT, shimname[j], &dtmp, 1) != 0)
		{
			Werrprintf("error in 'gshim.list': parameter '%s' does not exist", shimname[j]);
			return( -1 );
			break;
		}
		else
		{
			shimadd[j] = 0.0;
			shimerr[j] = 0.0;
			shimoldv[j] = (float) dtmp;
			shimval[j]  = (float) dtmp;
			if (fabs(shimcal[j]) < TOL)
			{
				shimcal[j] = 100;
				Werrprintf("warning: offset for shim '%s' in 'gshim.list' set to %g",shimname[j],shimcal[j]);
			}
			if ((shimoldv[j] + shimcal[j]) > shimlimit)
			{
			    if ((shimoldv[j] - shimcal[j]) < -shimlimit)
			    {
				shimcal[j] = shimlimit - shimoldv[j];
				Werrprintf("warning: offset for shim '%s' in 'gshim.list' clipped to %g",shimname[j],shimcal[j]);
			    }
			    else shimcal[j] = -shimcal[j];
			}
			else if ((shimoldv[j] + shimcal[j]) < -shimlimit)
			{
			    if ((shimoldv[j] - shimcal[j]) > shimlimit)
			    {
				shimcal[j] = -(shimlimit + shimoldv[j]);
				Werrprintf("warning: offset for shim '%s' in 'gshim.list' clipped to %g",shimname[j],shimcal[j]);
			    }
			    else shimcal[j] = -shimcal[j];
			}
		}
	}
	return(0);
}

/**********************
 * FILE 	*listfile;
 * char	 mapname[], shimsetnum[];
 * char	 cmd_name[];
 * float shimcal[],	 calibration values or offsets
 * 	 shimoldv[],	 old shim values
 * 	 shimval[],	 new shim values
 * 	 shimadd[],	 fitted shim values, shimval = shimoldv - shimadd, diff
 * 	 shimerr[];	 error in each shim
 * char	 shimname[][16];	 shim names
 * int	 gzmaxsize, gzsize, btrigger;  if poly then gzsize>8
 * float	 rmserr, chisq;
 */
static int gzfit_write_listfile(FILE *listfile, char mapname[], char shimsetnum[], char cmd_name[],
                 int btrigger, int gzmaxsize, int gzsize, float rmserr,
                 char shimname[][16],
                 float shimcal[], float shimoldv[], float shimval[], float shimadd[], float shimerr[], float chisq)
{
	int j;
	char jstr[MAXPATHL];

	fprintf(listfile, "mapname %s\nshimset %-2s   gzsize %d", mapname,shimsetnum,gzsize);
	fprintf(listfile, "                         rms err %11g\n", rmserr );

	if (gzsize < GZMINSIZE) gzsize = GZMINSIZE;
/* what it really should do is only fit the shims up to gzsize or ma_list,
   but write out values up to the number of shims in the basis set */

	fprintf(listfile, "\nShim       Offset         Old         New        Diff       Error\n"); 
		 strcpy(jstr,"-----------------------------------------------------------------");
	fprintf(listfile, "%s\n", jstr);
	if (btrigger == 0) {
		for (j=0; j<gzsize; j++) /* for (j=0; j<gzmaxsize; j++) */
			fprintf(listfile,"%-8s %8.0f %11.0f %11.0f %11.0f %11.0f\n", shimname[j],
				shimcal[j], shimoldv[j], shimval[j], shimadd[j], shimerr[j] );
		}
	else if (btrigger == 1) {
		for (j=0; j<gzsize; j++) /* for (j=0; j<gzmaxsize; j++) */
			fprintf(listfile,"%-8s %8g %11g %11g %11g %11g\n", shimname[j],
				shimcal[j], shimoldv[j], shimval[j], shimadd[j], shimerr[j] );
		}
	else { 
		for (j=0; j<gzsize; j++) /* for (j=0; j<gzmaxsize; j++) */
			fprintf(listfile,"%-8s %8.0f %11.0f %11.0f %11g %11g\n", shimname[j],
				shimcal[j], shimoldv[j], shimval[j], shimadd[j], shimerr[j] );
		}
	fprintf(listfile, "%s\n", jstr);
	return(0);
}

static int gzfit_output_files(char output_list2[], char mapname[], char shimsetnum[], int ma_list,
              float sw_edge, float chisq, float rmsfit, float rmserr, char cmd_name[], int btrigger,
              char shimname[][16], float shimcal[], float shimoldv[], float shimval[], float shimadd[],
              float shimerr[], char output_name[], int ma, int ndata, float x[], float y[], float wt[],
              float fbase[], float a[], float da[], char weight_output[], char verbose_on[])
{
	int	 ival, i, j, jj, gzmaxsize=MAX_CURVES;
	float	 ph, freq;
	FILE	*listfile2, *outfile;
	char	 over_write_prompt[MAXPATHL+20], over_write_ans[4];

/* output list file */
        if (access( output_list2, F_OK ) == 0) {	/* change output_list2 to output_list ?? */
                ival = unlink( output_list2 );
                if (ival != 0) {
                        Werrprintf( "%s:  cannot remove %s", cmd_name, output_list2 );
                	return( 1 );
			} 
                }

        listfile2 = fopen( output_list2, "w" );      /* open file */
        if (listfile2 == NULL) {
                Werrprintf( "%s:  problem opening %s", cmd_name, output_list2 );
                return( 1 );
		}

		/* need to fix whether ma_list is used or gzsize read in from parameter */
	gzfit_write_listfile(listfile2, mapname, shimsetnum, cmd_name, btrigger, gzmaxsize, ma_list, 
	  rmserr, shimname, shimcal, shimoldv, shimval, shimadd, shimerr, chisq);
	fclose( listfile2 );

/* output fit file */
        if (access( output_name, F_OK ) == 0) {
                if (!Bnmr) {
                        sprintf(
                                &over_write_prompt[ 0 ],
                                "OK to overwrite %s? ",
                                 output_name
                        );
                        W_getInputCR(
                                &over_write_prompt[ 0 ],
                                &over_write_ans[ 0 ],
                                 sizeof( over_write_ans ) - 1
                        );
                        if (over_write_ans[ 0 ] != 'Y' && over_write_ans[ 0 ] != 'y') {
                                Winfoprintf( "%s:  operation aborted", cmd_name );
                		return( 1 );
                        	}
			}
                printf( "overwriting output file %s\n", output_name );
                ival = unlink( output_name );
                if (ival != 0) {
                        Werrprintf( "%s:  cannot remove %s", cmd_name, output_name );
                	return( 1 );
			} 
                }

        outfile = fopen( output_name, "w" );      /* open file */
        if (outfile == NULL) {
                Werrprintf( "%s:  problem opening %s", cmd_name, output_name );
                return( 1 );
		}

	if (strcmp(verbose_on, "n")==0) {	/* default */
	fprintf( outfile, "exp 4 \n  2  %d \nFit (Frequency vs Field)\n\n%d  %d  %d  %d\n", 
		ndata, 1, 0, 0, 0 ); 
	if (strcmp(weight_output, "y") == 0) 			/* weighted output */
		for (i=0; i<ndata; i++) y[i] *= wt[i];
	for( i = 0; i < ndata; i++ ) {
		freq = x[i] * sw_edge;
		fprintf( outfile, "%g  %g\n", freq, y[i] ); 
		}
	fprintf( outfile, "\n%d  %d  %d  %d\n", 2, 0, 0, 0 ); 
	if (strcmp(weight_output, "y") == 0) {
        for( i = 0; i < ndata; i++ ) {
                freq = x[i] * sw_edge;
                ph = 0.0;
                for ( jj = 0; jj < ma; jj++) 
			ph += a[jj] * fbase[i+ndata*jj];
		fprintf( outfile, "%g  %g\n", freq, wt[i]*ph );		/* weighted fitted function */ 
		}
	} else {
        for( i = 0; i < ndata; i++ ) {
                freq = x[i] * sw_edge;
                ph = 0.0;
                for ( jj = 0; jj < ma; jj++) 
			ph += a[jj] * fbase[i+ndata*jj]; 
		fprintf( outfile, "%g  %g\n", freq, ph ); 		/* fitted function */ 
		}
		}
		} /* end if verbose = n */

	else if (strcmp(verbose_on, "d")==0) {
	fprintf( outfile, "exp 4 \n  1  %d \nFit Difference (Freq vs Field)\n\n%d  %d  %d  %d\n", 
		ndata, 1, 0, 0, 0 ); 
	if (strcmp(weight_output, "y") == 0) {
        for( i = 0; i < ndata; i++ ) {
                freq = x[i] * sw_edge;
                ph = 0.0;
                for ( jj = 0; jj < ma; jj++) {
			ph += a[jj] * fbase[i+ndata*jj];
			}
		fprintf( outfile, "%g  %g\n", freq, wt[i]*(y[i] - ph) );  	/* residual difference */ 
		}
	} else {
        for( i = 0; i < ndata; i++ ) {
                freq = x[i] * sw_edge;
                ph = 0.0;
                for ( jj = 0; jj < ma; jj++) {
			ph += a[jj] * fbase[i+ndata*jj];
			}
		fprintf( outfile, "%g  %g\n", freq, y[i] - ph );  	/* residual difference */ 
		}
		}
		} /* end if verbose = d */

	else if (strcmp(verbose_on, "w")==0) {
	fprintf( outfile, "exp 4 \n  1  %d \nWeight Function (Freq vs Field)\n\n%d  %d  %d  %d\n", 
		ndata, 1, 0, 0, 0 ); 
        for( i = 0; i < ndata; i++ ) {
                freq = x[i] * sw_edge;
		fprintf( outfile, "%g  %g\n", freq, wt[i] );  		/* weight function */ 
		}
		} /* end if verbose = w */

	else if (strcmp(verbose_on, "c")==0) {
	fprintf( outfile, "exp 4 \n  %d  %d \nFit Components (Freq vs Field)\n",
		ma, ndata );
	if (strcmp(weight_output, "y") == 0) {
      for (j=0; j<ma; j++) {
	fprintf( outfile, "\n%d  %d  %d  %d\n", j+1, 0, 0, 0 ); 
        for( i = 0; i < ndata; i++ ) {
                freq = x[i] * sw_edge;
		ph = a[j] * fbase[i+ndata*j] * wt[i];
		fprintf( outfile, "%g  %g\n", freq, ph );		/* weighted fitted function */ 
		}
      }
	} else {
      for (j=0; j<ma; j++) {
	fprintf( outfile, "\n%d  %d  %d  %d\n", j+1, 0, 0, 0 ); 
        for( i = 0; i < ndata; i++ ) {
                freq = x[i] * sw_edge;
		ph = a[j] * fbase[i+ndata*j];
		fprintf( outfile, "%g  %g\n", freq, ph ); 		/* fitted function */ 
		}
      }
		}
		} /* end if verbose = c */

	else if (strcmp(verbose_on, "b")==0) {
	fprintf( outfile, "exp 4 \n  %d  %d \nFit Functions (Freq vs Field)\n",
		ma, ndata );
	if (strcmp(weight_output, "y") == 0) {
      for (j=0; j<ma; j++) {
	fprintf( outfile, "\n%d  %d  %d  %d\n", j+1, 0, 0, 0 ); 
        for( i = 0; i < ndata; i++ ) {
                freq = x[i] * sw_edge;
		ph = fbase[i+ndata*j] * wt[i];
		fprintf( outfile, "%g  %g\n", freq, ph );		/* weighted fitted function */ 
		}
      }
	} else {
      for (j=0; j<ma; j++) {
	fprintf( outfile, "\n%d  %d  %d  %d\n", j+1, 0, 0, 0 ); 
        for( i = 0; i < ndata; i++ ) {
                freq = x[i] * sw_edge;
		ph = fbase[i+ndata*j];
		fprintf( outfile, "%g  %g\n", freq, ph ); 		/* fitted function */ 
		}
      }
		}
		} /* end if verbose = b */

	else if (strcmp(verbose_on, "e")==0) {
	fprintf( outfile, "exp 4 \n  %d  %d \nFit Components (Freq vs Field)\n",
		ma+1, ndata );
	if (strcmp(weight_output, "y") == 0) {
	fprintf( outfile, "\n%d  %d  %d  %d\n", 1, 0, 0, 0 ); 
        for( i = 0; i < ndata; i++ ) {
                freq = x[i] * sw_edge;
		ph = 0.0;
		for (jj=0; jj<ma; jj++) 
			ph += a[jj] * fbase[i+ndata*jj] * wt[i];
		fprintf( outfile, "%g  %g\n", freq, ph );		/* weighted fitted function */ 
		}
      for (j=0; j<ma; j++) {
	fprintf( outfile, "\n%d  %d  %d  %d\n", j+2, 0, 0, 0 ); 
        for( i = 0; i < ndata; i++ ) {
                freq = x[i] * sw_edge;
		ph = a[j] * fbase[i+ndata*j] * wt[i];
		fprintf( outfile, "%g  %g\n", freq, ph );		/* weighted fitted function */ 
		}
      }
	} else {
	fprintf( outfile, "\n%d  %d  %d  %d\n", 1, 0, 0, 0 ); 
        for( i = 0; i < ndata; i++ ) {
                freq = x[i] * sw_edge;
		ph = 0.0;
		for (jj=0; jj<ma; jj++) 
			ph += a[jj] * fbase[i+ndata*jj];
		fprintf( outfile, "%g  %g\n", freq, ph );		/* fitted function */ 
		}
      for (j=0; j<ma; j++) {
	fprintf( outfile, "\n%d  %d  %d  %d\n", j+2, 0, 0, 0 ); 
        for( i = 0; i < ndata; i++ ) {
                freq = x[i] * sw_edge;
		ph = a[j] * fbase[i+ndata*j];
		fprintf( outfile, "%g  %g\n", freq, ph ); 		/* fitted function */ 
		}
      }
		}
		} /* end if verbose = e */

	gzfit_write_listfile(outfile, mapname, shimsetnum, cmd_name, btrigger, gzmaxsize, ma_list, 
	  rmserr, shimname, shimcal, shimoldv, shimval, shimadd, shimerr, chisq);

	fclose( outfile );
	EPRINT1(1, "  Output file %s written.\n", output_name ); 

	return(0);
}

/************************************************************************/
int gmaplistfunc(int argc, char *argv[], int retc, char *retv[] )
/************************************************************************/
{
	FILE 	*listfile;
	char	 mapname[MAXPATHL], shimsetnum[4];
	char	 cmd_name[MAXPATHL], input_list[MAXPATHL], execstr[MAXPATHL];
	char	 userpath[MAXPATHL], readfunc_name[MAXPATHL];
	float	 shimcal[MAX_PARAMS],	/* calibration values or offsets */
		 shimoldv[MAX_PARAMS],	/* old shim values */
		 shimval[MAX_PARAMS],	/* new shim values */
		 shimadd[MAX_PARAMS],	/* fitted shim values, shimval = shimoldv - shimadd, diff */
		 shimerr[MAX_PARAMS];	/* error in each shim */
	char	 shimname[MAX_PARAMS][16];	/* shim names */
	int	 gzmaxsize=MAX_CURVES, gzlist, btrigger=0; /* if poly then >8 */
	int	 readfunc_type, gzsize, ival, jval=0;
	float	 rmserr;
	double	 dtmp;

	strcpy( cmd_name, argv[0] );
	if (greadfunc_args(argv, argc, userpath, readfunc_name, &readfunc_type, 
			input_list, mapname, cmd_name, &jval ) != 0)
	{
                if (retc>0) retv[0] = intString(-1);
                return( 0 );
	}
      if (readfunc_type != 5)
      {
/* read input_list as input, use to set up, if wrong format then abort */
	if (access(input_list, F_OK) != 0)
	{
		if (readfunc_type == 4) jval = 1;
		else
		{
		    if (!Bnmr)
			Werrprintf( "%s:  file 'gshim.list' not found", cmd_name );
               	    if (retc>0) retv[0] = intString(-1);
		    return( 0 );
		}
	}
	else
	{
	listfile = fopen( input_list, "r" );
	if (listfile == NULL) {
		if (readfunc_type == 4) jval = 1;
		else
		{
			Werrprintf( "%s: problem opening 'gshim.list'", cmd_name );
                	if (retc>0) retv[0] = intString(-1);
			return( 0 );
		}
		}
	else if (gzfit_read_listfile(listfile, mapname, shimsetnum, &gzlist, &rmserr, btrigger,
		shimcal, shimoldv, shimval, shimadd, shimerr, shimname, cmd_name, input_list) != 0)
		{
		if (readfunc_type == 4) jval = 1;
		else
		{
			fclose(listfile);
                	if (retc>0) retv[0] = intString(-1);
			return( 0 );
		}
		}
	fclose(listfile);
	}
      }

	switch (readfunc_type)
	{
		case 0:
			gzsize = jval;
			if (gfunc_setoldv(&gzsize, &gzlist, shimname, shimoldv, shimval, shimadd,
				shimerr, userpath, cmd_name, &rmserr, shimsetnum, gzmaxsize) != 0)
			{
                		if (retc>0) retv[0] = intString( -1 );
                		return( 0 );
			}
			else
			{
                		if (retc>0) retv[0] = intString( 0 );
				if (retc>1) retv[1] = newString( shimname[0] );
				if (retc>2) retv[2] = newString( shimname[3] );
				if (retc>3)
				{
				  if (P_getreal(CURRENT,"gzsize",&dtmp,1) != 0)
				    retv[3] = newString( shimname[gzsize-1] );
				  else
				    retv[3] = newString( shimname[((int)(dtmp+0.5))-1] );
				}
			}
		  break;
		case 1: /* always (setnew,0) */
			jval = gfunc_setnew(gzlist, &gzsize, shimname, shimoldv, shimval, shimadd,
				shimerr, shimsetnum, execstr, &rmserr, userpath, cmd_name);
                	if (retc>0) retv[0] = intString( jval );
			if (retc>1) retv[1] = newString( execstr );
			if (retc>2) 
			{
			    if (jval>0) retv[2] = newString( shimname[gzsize] );
			    else retv[2] = newString( shimname[gzsize-1] );
			}
		  break;
		case 2:
			if (gfunc_setzshims(shimname, gzlist, shimsetnum, execstr) != 0)
			{
				Werrprintf( "%s( '%s' ) failed",cmd_name,readfunc_name);
                		if (retc>0) retv[0] = intString( -1 );
				if (retc>1) retv[1] = newString( "\0" );
                		return( 0 );
			}
			else 
			{
                		if (retc>0) retv[0] = intString( 0 );
				if (retc>1) retv[1] = newString( execstr );
			}
		  break;
		case 3:
			if (gfunc_readzshims(shimname, gzlist, execstr) != 0)
			{
				Werrprintf( "%s( '%s' ) failed",cmd_name,readfunc_name);
                		if (retc>0) retv[0] = intString( -1 );
				if (retc>1) retv[1] = newString( "\0" );
                		return( 0 );
			}
			else 
			{
                		if (retc>0) retv[0] = intString( 0 );
				if (retc>1) retv[1] = newString( execstr );
			}
		  break;
		case 4:
			if (gfunc_unarrayshims(shimname, gzlist, gzmaxsize, jval, execstr) != 0)
			{
				Werrprintf( "%s( '%s' ) failed",cmd_name,readfunc_name);
                		if (retc>0) retv[0] = intString( -1 );
				if (retc>1) retv[1] = newString( "\0" );
                		return( 0 );
			}
			else 
			{
                		if (retc>0) retv[0] = intString( 0 );
				if (retc>1) retv[1] = newString( execstr );
			}
		  break;
		case 5:
			if (gfunc_wrshimcal(cmd_name,shimname,shimcal,shimoldv,shimval,shimadd,shimerr,
				&gzsize,gzmaxsize,shimsetnum,&rmserr) != 0)
			{
                		if (retc>0) retv[0] = intString( -1 );
                		return( 0 );
			}
			else 
                		if (retc>0) retv[0] = intString( 0 );
		  break;
		case 6:
			gzsize = gzlist;
			if (gfunc_chkshimcal(cmd_name,shimname,shimcal,shimoldv,shimval,shimadd,shimerr,
				&gzsize,gzmaxsize,shimsetnum,&rmserr) != 0)
			{
                		if (retc>0) retv[0] = intString( -1 );
                		return( 0 );
			}
			else 
				if (retc>0) retv[0] = intString( 0 );
		  break;
		default: break;
	}

	if ((readfunc_type==0) || ((readfunc_type==1) && (jval==1)) || 
		(readfunc_type==5) || (readfunc_type==6))
	{
            if (access( input_list, F_OK ) == 0) {
                ival = unlink( input_list );
                if (ival != 0) {
                        Werrprintf( "%s:  cannot remove %s", cmd_name, input_list );
                	if (retc>0) retv[0] = intString( -1 );
                	return( 0 );
			} 
                }
            listfile = fopen( input_list, "w" );
            if (listfile == NULL) {
                Werrprintf( "%s:  problem opening %s", cmd_name, input_list );
                if (retc>0) retv[0] = intString( -1 );
                return( 0 );
		}
	    gzfit_write_listfile(listfile, mapname, shimsetnum, cmd_name, btrigger, gzmaxsize,
		gzsize, rmserr, shimname, shimcal, shimoldv, shimval, shimadd, shimerr,0.0);
	    fclose(listfile);
	}
	return( 0 );
}

