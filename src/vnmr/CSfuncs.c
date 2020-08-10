/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vnmrsys.h"
#include "CSfuncs.h"
#include "allocate.h"
#include "group.h"
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"

static csInfo cs;

int getCSdimension()
{
   return(cs.xdim);
}

char *getCSpar(int dim)
{
   return(cs.dimPar[dim]);
}

char *getCSname()
{
   return(cs.name);
}

int getCSnum()
{
   return(cs.num);
}

int CStypeIndexed()
{
   return(cs.type != CSDELAY);
}

double getCSval(int xdim, int index)
{
   return((double)cs.sch[xdim][index]);
}

// ===============================================================================
// int getCSparIndex(const char *dimPar)
// ===============================================================================
//  search for input parameter string dimPar in csInfo array (cs.dimPar[i])
//  return value: index of matching element if found else -1
// ===============================================================================
int getCSparIndex(const char *dimPar)
{
	int i;
	for (i = 0; i < cs.xdim; i++) {
		if(strcmp(dimPar,cs.dimPar[i])==0)
			return i;
	}
#ifdef OLDCODE
	if ( ! strcmp(dimPar,"d2"))
	{
		return( (cs.density[0] < 1.0) ? 0 : -1);
	}
	else if ( ! strcmp(dimPar,"d3"))
	{
		return( (cs.density[1] < 1.0) ? 1 : -1);
	}
	else if ( ! strcmp(dimPar,"d4"))
	{
		return( (cs.density[2] < 1.0) ? 2 : -1);
	}
#endif
   return(-1);
}

// ===============================================================================
// int initIndex(int i)
// ===============================================================================
//  initialize csInfo array element i
//  uses values of current parameters ni,ni2..,CSdensity,CSdensity2.. to
//  determine if element dimension is sparse, full or not active
//  sparse criteria:
//    1. expected index parameters (ni[j],CSdensity[j]) exist and are active
//    2. ni[j] > 1
//    3. CSdensity[j]>0 && < 100
//  if all sparse criteria are met function sets:
//    1. cs.dim[cs.dim]=ni[j]
//    2. cs.density[cs.dim]=CSdensity[j]
//    3. cs.sdim[cs.dim]=CSdensity[j]*ni[j]
//    4. cs.dimPar[cs.dim]="d%i" i=index+2
//  also modified:
//    1. cs.dim++                 number of schedule dimensions
//    2. cs.num *= cs.sdim[i]     number of schedule elements
//    3. cs.total *= cs.dim[i]    total size of (non-sparse) matrix
//  return value:
//    0 if all sparse criteria met else failure code (1->7)
// ===============================================================================
static int initIndex(int i){
	double val, density=0;
	int nmax,sdim;
	char nstr[16]="ni";
	char dstr[16]="CSdensity";

	cs.sch[i] = NULL;
	cs.dim[i] = 1;
	cs.density[i] = 0.0;

	if(i>0){
		sprintf(nstr,"ni%d",i+1);
		sprintf(dstr,"CSdensity%d",i+1);
	}
	if (P_getreal(CURRENT, nstr, &val, 1))
		return (1);
	if (!P_getactive(CURRENT,nstr))
		return (2);
	if (val <= 1)
		return (3);
	if (P_getreal(CURRENT, dstr, &density, 1))
		return (4);
	if (!P_getactive(CURRENT, dstr))
		return (5);
	density /= 100.0;
	if ((density <=0) || (density >1.0))
		return (6);
	nmax = (int) (val + 0.1);
	sdim = (int) (val * density + 0.1);
        if (sdim < 1)
           sdim = 1;
	if ((nmax < sdim) || (sdim < 1))
		return (7);

	cs.dim[cs.xdim]=nmax;
	cs.density[cs.xdim] = density;
	cs.sdim[cs.xdim] = sdim;
	sprintf(cs.dimPar[cs.xdim], "d%d",i+2);
	cs.xdim++;
	cs.total *= nmax;
	cs.num *=sdim;

	return 0;
}

// ===============================================================================
// int CSinit(const char *sampling)
// ===============================================================================
//  initialize csInfo array using current parameters
//  return value:
//    0   on success
//   -1   "sampling" parameter not in CURRENT tree
// ===============================================================================
int CSinit(const char *sampling, const char *dir) {
	int i;
	double val;
	char tstr[MAXPATH];

	cs.xdim = 0;
	cs.num = 0;
	cs.total = 0;
        cs.res = 0;
	strcpy(cs.name, "sampling.sch");

	if (sampling) {
		if (!P_getstring(CURRENT, "sampling", tstr, 1, MAXPATH - 1)) {
			if (strcmp(tstr, "sparse"))
				return (-1);
		} else {
			return (-1);
		}
	}
	cs.total = 1;
	cs.num = 1;
	cs.type = CSAUTO;
	if (!P_getstring(CURRENT, "CStype", tstr, 1, MAXPATH - 1)) {
		if (!strcmp(tstr, "d"))
			cs.type = CSDELAY;
		else if (!strcmp(tstr, "i"))
			cs.type = CSINDEX;
                else if ( (!strcmp(tstr, "p1")) || (!strcmp(tstr, "p2")) )
                        cs.type = CSAUTOPOISSON;
	}

	for (i = 0; i < CSMAXDIM; i++) {
		initIndex(i);
	}
        if ( (cs.type != CSAUTO) && (cs.type != CSAUTOPOISSON) && (dir != NULL) )
        {
           cs.res = CSreadSched(dir);
           // Subsequent call to CSreadSched will return cs.res
           if (cs.res)
              return(-1);
           cs.res = 1;
        }
	cs.total = (cs.xdim) ? cs.total : 1;
	cs.num = (cs.xdim) ? cs.num  : 1;

	cs.seed = 169;
	cs.rand = 0;
	cs.inflate = 0;
	cs.write = 0;
	if (!P_getreal(CURRENT, "CSseed", &val, 1)) {
		if (val < 0.0) {
			cs.seed = (int) (val - 0.5);
			cs.rand = 1;
		} else {
			cs.seed = (int) (val + 0.5);
		}
	}
	cs.arr = NULL;
	cs.rsch = NULL;
	return (0);
}

// ===============================================================================
// int CSreadSched(const char *dir)
// ===============================================================================
//  assumes csInfo info initialized using current parameters or CSschedule arguments
//  populates sparse indexes by reading in an existing schedule file
//  return value:
//    0   success
//   -1   schedule file not found
//   -2   something wrong with file format
//   -3   number of columns in file does not match expected number of dimensions
// ===============================================================================
int CSreadSched(const char *dir) {
	char path[MAXPATH+16];
	char line[MAXPATH];
	FILE *fd;
	int val[CSMAXDIM];
	int dim;
	int i, j;
        int count = 0;

        // Read as part of CSinit()
        if (cs.res < 0)
           return(cs.res);
        if (cs.res == 1)
           return(0);

	if (cs.name[0] == '/')
		strcpy(path, cs.name);
	else
		sprintf(path, "%s/%s", dir, cs.name);
	if ((fd = fopen(path, "r")) == NULL ) {
		return (-1);
	}
	while ((count == 0) && fgets(line, sizeof(line), fd) )
        {
           if (line[0] != '#')
              count++;
        }
        if (count == 0)
        {
		fclose(fd);
		return -2;
	}
	dim = sscanf(line, "%d %d %d", &val[0], &val[1], &val[2]);
	if (dim < 1) {
		fclose(fd);
		return -2;
	}
	if (dim != cs.xdim) {
		fclose(fd);
		return -3;
	}
	if ( (cs.type != CSAUTO)&&(cs.type != CSAUTOPOISSON) )
        {
           long fdTell;

           fdTell = ftell(fd);
	   while (fgets(line, sizeof(line), fd) )
           {
              if (line[0] != '#')
                 count++;
           }
           cs.num = count;
           fseek(fd,fdTell,SEEK_SET);
        }
	for (j=0; j < cs.xdim; j++) {
		cs.sch[j] = (int *) allocateWithId(cs.num * sizeof(int), "1_CS");
		memset((void *) cs.sch[j], 0, cs.num * sizeof(int));
		cs.sch[j][0] = val[j];
	}
	i = 1;
	while ((fgets(line, sizeof(line), fd)) && (i < cs.num))
        {
           if (line[0] != '#')
           {
		sscanf(line, "%d %d %d", &val[0], &val[1], &val[2]);
		for (j=0; j < cs.xdim; j++) {
			cs.sch[j][i] = val[j];
		}
		i++;
           }
	}
	fclose(fd);
	return (0);
}

// ===============================================================================
// void mk_rsch()
// - use psuedo-random number generator to populate schedule indexes (sequential order)
// ===============================================================================
static void mk_sch()
{
   register int i, j;

   i=1; cs.arr[0] = 1; // explicitly set the first sampling point
   while(i < cs.num)
   {
      j = (int) (0.5 + (double) cs.total * ( (double) random() / (double) RAND_MAX));
      if(cs.arr[j] == 0)
      {
         cs.arr[j] = 1;
         i++;
      }
   }
}

// ===============================================================================
// void mk_rsch()
// - use psuedo-random number generator to populate schedule indexes (random order)
// ===============================================================================
static void mk_rsch()
{
   register int i, j;

   i=1; cs.arr[0] = 1; cs.rsch[0]=0; // explicitly set the first sampling point
   while(i < cs.num)
   {
      j = (int) (0.5 + (double) cs.total * ( (double) random() / (double) RAND_MAX));
      if(cs.arr[j] == 0)
      {
         cs.arr[j]++;
         cs.rsch[i]=j;
         i++;
      }
   }
}


// ===============================================================================
// static int poisson_pdf(double lamda)
// calculate Poisson probability distribution values
// ===============================================================================

        //               Knuth Algorithm                // 
        // algorithm poisson random number (Knuth):     //
        // init:                                        //
        //    Let L ? exp(-lambda), k ? 0 and p ? 1.    //
        // do:                                          //
        //    k ? k + 1.                                //
        //    Generate uniform random number u in [0,1] //
        //    and let p ? p × u.                        //
        // while p ? L.                                 //
        // return k ? 1.                                //
        //////////////////////////////////////////////////

static int poisson_pdf( double lambda )
{
        double  L = exp( -lambda );
        int     k = 0;
        double  p = 1;
        int iter  = 0;

        do {
                double  u = drand48();
                p *= u;
                k += 1;
                if (++iter > 10000)
                   return (-1);
        } while ( p >= L );

        return( k-1 );
}


// ===============================================================================
// static int mk_poissongap_sch()
//   use poisson gap sampling to populate schedule indexes (sequential order)
// ===============================================================================

static int mk_poissongap_sch(double sinefraction)
{
        int     i;
        float   ld = (float) cs.total / (float) cs.num;
        float   w = 2.0;                   //  inital weight 
        int     k = 0;                     //  currently found sampling points 
        int     l = 0;

        if ((1.0/ld) < 0.01)
        {
          Werrprintf("CSdensity too low (<1%%) in Poisson gap sequential sampling\n");
          ABORT;
        }
        else if (cs.num > cs.total)
        {
          Werrprintf("CSdensity greater than 100%% in Poisson gap sequential sampling\n");
          ABORT;
        }

        /* int *vec  = ( int* ) malloc( cs.total*sizeof( cs.total ) ); */
        int *vec  = (int *) allocateWithId(cs.total * sizeof(int), "1_CS");
        memset((void *) vec, 0, cs.total * sizeof(int));

        /* Winfoprintf("mk_poissongap_sch: sine fraction=%2.1f  num=%d  total=%d\n",sinefraction,cs.num,cs.total); */

        int tempi;
        do {
               for(l=0; l<cs.total; l++)
               {
                 vec[l] =-1;
               }
               vec[0]   = 0;            // first point is always acquired
               i = 0; k = 0;         
               while ( i < cs.total ) {
                   vec[k] = i;   //  store point to be acquired 
                   i += 1;           //  put pointer at next point 
                   k += 1;           //  next index of acqured point

                   //  calculate the gap

                   tempi = poisson_pdf( (ld-1.0)*w*sin((float)i/(float)(cs.total+1)*(M_PI/sinefraction)) );
                   if (tempi < 0)
                   {
                      Werrprintf("Poisson gap calculation error- no convergance. Increase CSdensity or change CStype.\n");
                      ABORT;
                   }
                   else
                     i += tempi;
                }

// if more  points calculated than wanted: try again with weight 2% larger 
// if fewer points calculated than wanted: try again with weight 2% smaller 

                if ( k > cs.num ) w *= 1.02;
                if ( k < cs.num ) w /= 1.02;

        } while ( k != cs.num );          // try until correct number points to be acquired found

    i = 0;
    for (k=0; k<cs.num; k++)
    {
       if (vec[k]>-1)
       {
          cs.arr[vec[k]]=1;
          i++;
       }
    }
    if (i != cs.num)
    {
       Werrprintf("CS poisson gap points calculation error\n");
       ABORT;
    }

/*
        Winfoprintf("points req=%d  points calcd=%d  total points=%d  deviation=%d\n", \
            cs.num, i, cs.total, (cs.num-i));
        printf("Poisson gap sampling points in sequential order:\n\n"); 

        i = cs.num; int l2;
        for ( k = 0 ; k < cs.num ; k=k+10)
        {
           if (i < 10) 
             l2=i;
           else
             l2=10;
           for (l=0; l<l2; l++) printf( "%3d ", vec[k+l]);
           i -= 10;
           printf("\n");
        }
        printf("\n");
       
        i = cs.total;
        for ( k = 0 ; k < cs.total ; k=k+10)
        {
           if (i<10)
             l2=i;
           else
             l2=10;
           for (l=0; l<l2; l++) printf( "%3d ", cs.arr[k+l]);
           i -= 10;
           printf("\n");
        }
*/
    RETURN;
}


// ===============================================================================
// static int mk_poissongap_rsch()
//   use poisson gap sampling to populate schedule indexes (random order)
// ===============================================================================

static int mk_poissongap_rsch(double sinefraction)
{
        int     i;
        float   ld = (float) cs.total / (float) cs.num;
        float   w = 2.0;                   //  inital weight 
        int     k = 0;                     //  currently found sampling points 
        int     l = 0;

        if ((1.0/ld) < 0.01)
        {
          Werrprintf("CSdensity too low (<1%%) in Poisson gap sequential sampling\n");
          ABORT;
        }
        else if (cs.num > cs.total)
        {
          Werrprintf("CSdensity greater than 100%% in Poisson gap sequential sampling\n");
          ABORT;
        }

        /* int *vec  = ( int* ) malloc( cs.total*sizeof( cs.total ) ); */
        int *vec  = (int *) allocateWithId(cs.total * sizeof(int), "1_CS");
        memset((void *) vec, 0, cs.total * sizeof(int));

        /* Winfoprintf("mk_poissongap_rsch: sine fraction=%2.1f  num=%d  total=%d\n",sinefraction,cs.num,cs.total); */
        int tempi;
        do {
               for(l=0; l<cs.total; l++)
               {
                 vec[l] =-1;
               }
               vec[0]   = 0;            // first point is always acquired
               i = 0; k = 0;         
               while ( i < cs.total ) {
                   vec[k] = i;   //  store point to be acquired 
                   i += 1;           //  put pointer at next point 
                   k += 1;           //  next index of acqured point

                   //  calculate the gap

                   tempi = poisson_pdf( (ld-1.0)*w*sin((float)i/(float)(cs.total+1)*(M_PI/sinefraction)) );
                   if (tempi < 0)
                   {
                      Werrprintf("Poisson gap calculation error- no convergance. Increase CSdensity or change CStype.\n");
                      ABORT;
                   }
                   else
                     i += tempi;
                }

// if more  points calculated than wanted: try again with weight 2% larger 
// if fewer points calculated than wanted: try again with weight 2% smaller 

                if ( k > cs.num ) w *= 1.02;
                if ( k < cs.num ) w /= 1.02;

        } while ( k != cs.num );          // try until correct number points to be acquired found

    i = 0;
    for (k=0; k<cs.num; k++)
    {
       if (vec[k]>-1)
       {
          cs.arr[vec[k]]=1;
          i++;
       }
    }
    if (i != cs.num)
    {
        Werrprintf("CS poisson gap points calculation error\n");
        ABORT;
    }

/*
        Winfoprintf("points req=%d  points calcd=%d  total points=%d  deviation=%d\n", \
            cs.num, i, cs.total, (cs.num-i));
        printf("Poisson gap sampling points in sequential order:\n\n");
        i = cs.num; int l2;
        for ( k = 0 ; k < cs.num ; k=k+10)
        {
           if (i < 10) 
             l2=i;
           else
             l2=10;
           for (l=0; l<l2; l++) printf( "%3d ", vec[k+l]);
           i -= 10;
           printf("\n");
        }
        printf("\n");
       
        i = cs.total;
        for ( k = 0 ; k < cs.total ; k=k+10)
        {
           if (i<10)
             l2=i;
           else
             l2=10;
           for (l=0; l<l2; l++) printf( "%3d ", cs.arr[k+l]);
           i -= 10;
           printf("\n");
        }
*/

// now randomize the order in poisson gap vector vec 

    int sum1 = 0, sum2 = 0, noswap = 0, l2 = 0;
    for (k=0; k<cs.num; k++)
       sum1 += vec[k];

    for (k=0; k<2*(cs.num); k++)
    {
       noswap = 0;
       // compute two indices which should be swapped with each other

       i = (int) (0.5 + (double) ((cs.num)-1) * ( (double) random() / (double) RAND_MAX));
       if ((i<=0) || (i>=(cs.num)))
          i = (int) (0.5 + (double) ((cs.num)-1) * ( (double) random() / (double) RAND_MAX));

       l = (int) (0.5 + (double) ((cs.num)-1) * ( (double) random() / (double) RAND_MAX));
       if ((l<=0) || (l>=(cs.num)) || (l==i))
          l = (int) (0.5 + (double) ((cs.num)-1) * ( (double) random() / (double) RAND_MAX));

       if ((i<=0) || (i>=(cs.num)))
          noswap = 1;
       if ((l<=0) || (l>=(cs.num)))
          noswap = 1;

       if (noswap) 
          continue;

       l2     = vec[i];
       vec[i] = vec[l];
       vec[l] = l2;
    }

    sum2=0;
    for (k=0; k<cs.num; k++)
    {
       cs.rsch[k] = vec[k];
       sum2      += cs.rsch[k];
    }

    if (cs.rsch[0] != 0)
    {
       Werrprintf("error (type 1) in randomizing the poisson gap schedule! abort!\n");
       ABORT;
    }
     
/*
        printf("\nPoisson gap sampling points in random order:\n\n");
        i = cs.num; 
        for ( k = 0 ; k < cs.num ; k=k+10)
        {
           if (i < 10)
             l2=i;
           else
             l2=10;
           for (l=0; l<l2; l++) printf( "%3d ", cs.rsch[k+l]);
           i -= 10;
           printf("\n");
        }
*/

    if (sum1 != sum2)
    {
        Werrprintf("error (type 2) in randomizing the poisson gap schedule! abort!\n");
        ABORT;
    }
        
    RETURN;
}


// ===============================================================================
// int CSmakeSched()
// - generate a schedule from current csInfo array
// ===============================================================================
int CSmakeSched() {
	int i,j,k;
        int ret = 1;
	int modulus=1;
	int divisor=1;
        char tstr[MAXPATH];

	if (!cs.total)
		RETURN;
        /* Winfoprintf("CSmakeSched CS.type=%d  CS.rand=%d cs.xdim=%d\n",cs.type,cs.rand,cs.xdim); */

	srandom(cs.seed);
	cs.arr = (char *) allocateWithId(cs.total, "1_CS");
	memset((void *) cs.arr, 0, cs.total);
	for (i = 0; i < cs.xdim; i++) {
		cs.sch[i] = (int*) allocateWithId(cs.num * sizeof(int),"1_CS");
		memset((void *) cs.sch[i], 0, cs.num * sizeof(int));
	}
	if (cs.rand) {
		cs.rsch = (int *) allocateWithId(cs.num * sizeof(int), "1_CS");
		memset((void *) cs.rsch, 0, cs.num * sizeof(int));
                if (cs.type == CSAUTOPOISSON)
                {
                  double sinefraction = 2.0;
                  if (!P_getstring(CURRENT, "CStype", tstr, 1, MAXPATH - 1))
                  {
                     if (!strcmp(tstr, "p1"))
                        sinefraction = 1.0;
                     else 
                        sinefraction = 2.0;
                  }
                  /* Winfoprintf("mk_poissongap_rsch with sinefraction %3.1f random acquisition\n",sinefraction); */
                  ret = mk_poissongap_rsch(sinefraction);
                  if (ret)
                     ABORT;
                }
                else
                {
		  mk_rsch();
                }

		for (i = 0; i < cs.num; i++) {
			modulus=1;
			divisor=1;
			for (k = 0; k < cs.xdim; k++) {
				modulus=cs.dim[k];
				cs.sch[k][i] = (int) ((cs.rsch[i]/divisor) % modulus);
				divisor*=modulus;
			}
		}
	} else {
                if (cs.type == CSAUTOPOISSON)
                {
                  double sinefraction = 2.0;
                  if (!P_getstring(CURRENT, "CStype", tstr, 1, MAXPATH - 1))
                  {
                     if (!strcmp(tstr, "p1"))
                        sinefraction = 1.0;
                     else
                        sinefraction = 2.0;
                  }
                  /* Winfoprintf("mk_poissongap_sch with sinefraction %3.1f sequential acquisition\n",sinefraction); */
                  ret = mk_poissongap_sch(sinefraction);
                  if (ret)
                     ABORT;
                }
                else
                {
		  mk_sch();
                }

		j=0;
		for (i = 0; i < cs.total; i++){
			if (cs.arr[i]){
				modulus=1;
				divisor=1;
				for (k = 0; k < cs.xdim; k++) {
					modulus=cs.dim[k];
					cs.sch[k][j] = (int) ((i/divisor) % modulus);
					divisor*=modulus;
				}
				j++;
			}
		}
		divisor*=modulus;
	}
        RETURN;
}

int CSgetSched(const char *dir)
{
   int ret=1;

   if ((cs.type == CSAUTO) || (cs.type == CSAUTOPOISSON))
   {
      ret = CSmakeSched();
      if (ret) 
         ABORT;
      CSwriteSched(dir);

   }
   return(CSreadSched(dir));
}

// ===============================================================================
// void CSprint(const char *msg)
// - print info for active schedule
// ===============================================================================
void CSprint(const char *msg) {
	int i;

	fprintf(stderr, "%s\n", msg);
	fprintf(stderr, "CS name:    %s\n", cs.name);
	fprintf(stderr, "CS seed:    %d\n", cs.seed);
	fprintf(stderr, "CS rand:    %d\n", cs.rand);
	fprintf(stderr, "CS total:   %d\n", cs.total);
	fprintf(stderr, "CS num:     %d\n", cs.num);
	fprintf(stderr, "CS xdim:    %d\n", cs.xdim);
	if (cs.xdim == 0)
		return;
	for (i = 0; i < cs.xdim; i++) {
		fprintf(stderr, "CS[%d] par:%s dim:%d density:%g sdim:%d \n", i,
					cs.dimPar[i], cs.dim[i], cs.density[i], cs.sdim[i]);
	}
}

// ===============================================================================
// void CSprintSched()
// - print elements of active schedule
// ===============================================================================
void CSprintSched() {
	int i, j;

	for (i = 0; i < cs.num; i++) {
		fprintf(stderr, "CS_%-4d", i);
		for (j = 0; j < cs.xdim; j++) {
			if (cs.sch[j])
				fprintf(stderr, " %-4d", (int)cs.sch[j][i]);
		}
		fprintf(stderr, "\n");
	}
}

// ===============================================================================
// void CSwriteSched(const char *dir)
// - save current schedule in a file
// ===============================================================================
void CSwriteSched(const char *dir) {
	char path[MAXPATH+16];
	FILE *fd;

	if (cs.name[0] == '/')
		strcpy(path, cs.name);
	else
		sprintf(path, "%s/%s", dir, cs.name);
	fd = fopen(path, "w");
	if (fd) {
		int i, j;
		for (i = 0; i < cs.num; i++) {
			for (j = 0; j < cs.xdim; j++) {
				if (cs.sch[j])
					fprintf(fd, "%-4d ", (int) cs.sch[j][i]);
			}
			fprintf(fd, "\n");
		}
		fclose(fd);
	}
}
#ifdef VNMRJ
// ===============================================================================
// CSargs(int argc, char *argv[])
// ===============================================================================
//  - called by CSschedule
//  - optional input arguments allow override of schedule set by vnmr parameters
//  - argument syntax:
//    1. "filename"              sets schedule file name (default=sampling.sch)
//    2. "seed real"             sets random seed (default=169, if <0 set random schedule)
//    3. "real int",["real int ",["real int"]]
//                                sets sampling density & size (order sequentially assigned)
//    example:
//    CSschedule('15.625','64','15.625','32')
//    Equivalent to  mkCSsch 10 64 5 32
// ===============================================================================
static void CSargs(int argc, char *argv[]) {
	int i;
	int ival;
	int xdim;

	double density;

	xdim = 0;
	cs.write = 1;
	strcpy(cs.name, "sampling.sch");
	if (argc == 1)
		return;
	i = 1;
	while (i < argc) {
		fprintf(stderr, "1: %s\n", argv[i]);
		if (!strncmp(argv[i], "read", 4)) {
			cs.write = 0;
			if ((i + 1 < argc) && !isReal(argv[i + 1])) {
				fprintf(stderr, "CSargs[%d]\n", argc);
				strcpy(cs.name, argv[i + 1]);
				i++;
			}
		} else if (!strncmp(argv[i], "seed", 4)) {
			if (1 == sscanf(argv[i], "seed %d", &ival)) {
				if (ival < 0) {
					cs.seed = -ival;
					cs.rand = 1;
				} else {
					cs.seed = ival;
				}
				P_setreal(CURRENT, "CSseed", (double) cs.seed, 1);
			}
		}
		else if (!isReal(argv[i])) {
			strcpy(cs.name, argv[i]);
		} else
		{
			density = atof(argv[i++]);
			density/=100;
			ival= atoi(argv[i]);
			if(ival>1 && density>0 && density <1){
				cs.density[xdim] = density;
				cs.dim[xdim]=ival;
				cs.sdim[xdim] = ival * density+0.1;
				sprintf(cs.dimPar[xdim], "d%d",xdim+2);
				xdim++;
			}
		}
		i++;
	}
	if (xdim) {
		if (xdim > CSMAXDIM-1) {
			Winfoprintf("%s: CS schedule limited to %d dimensions", argv[0],
					CSMAXDIM-1);
			xdim = CSMAXDIM-1;
		}
		else
			cs.xdim=xdim;
		cs.total = 1;
		cs.num=1;
		for (i = 0; i < cs.xdim; i++) {
			if((cs.dim[i]>1) && (cs.sdim[i]<cs.dim[i])){
				cs.total *= cs.dim[i];
				cs.num *= cs.sdim[i];
			}
		}
	}
}

int CSschedule(int argc, char *argv[], int retc, char *retv[]) {
	int ret;

	//P_setstring(CURRENT, "sampling", "sparse", 1);
	//P_setstring(CURRENT, "CStype", "a", 1);
	CSinit(NULL,NULL);
	CSprint("After init");
	CSargs(argc, argv);
	CSprint("After args");
	if (cs.write) {
		if ( (ret = CSmakeSched()) ) 
                   ABORT;
		CSprintSched();
		CSwriteSched(curexpdir);
		releaseAllWithId("1_CS");
	} else {
		if ((ret = CSreadSched(curexpdir)))
			fprintf(stderr, "CS read error: %d\n", ret);
		else{
			CSprint("After read");
			CSprintSched();
		}
	}
	RETURN;
}

#endif // VNMRJ
