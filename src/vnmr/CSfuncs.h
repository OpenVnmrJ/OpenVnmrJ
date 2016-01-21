/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*-------------------------------------------------------
|							|
|   This header file is to contain the structure	|
|   definitions for all CS parameters.			|
|							|
+------------------------------------------------------*/

#ifndef CSFUNCS_H
#define CSFUNCS_H

#define CSMAXDIM 10

#define CSAUTO  1        /* Automatically calculate an indexed scheduling scheme */
#define CSINDEX 2        /* Read a scheduling scheme of indexes                  */
#define CSDELAY 3        /* Read a scheduling scheme of delays                   */
#define CSAUTOPOISSON 4  /* Automatically calculate an indexed Poisson Gap schedule */

struct _csInfo
{
    int         *sch[CSMAXDIM]; /* schedule */
    double      density[CSMAXDIM];        /* sparse density */
    char        *arr;           /* vector of random numbers */
    int         *rsch;           /* vector of random numbers */
    int         xdim;           /* dimension of schedule (number of columns) */
    int         sdim[CSMAXDIM];  /* sparse size of each dimension */
    int         dim[CSMAXDIM];  /* full size of each dimension (ni, ni2, ni3, etc) */
    char        dimPar[CSMAXDIM][32]; /* parameter for each dimension (d2, d3, d4, etc) */
    int         seed;           /* seed used for call to random */
    int         rand;           /* random or sequential scheduler */
    int         type;           /* type of schedule  a-automatic, i-indexed, d-delay */
    int         inflate;        /* auto inflate flag (currently unused) */
    int         num;            /* number of elements (rows) in schedule (arraydim) */
    int         total;          /* total points in completely sampled matrix */
    int         write;          /* flag for writing schedule to curexp */
    int         res;            /* result of reading schedule */
    char        name[256];      /* name of schedule (always in curexp) */
};

typedef struct _csInfo	csInfo;

#ifdef __cplusplus
extern "C" {
#endif
  int    CSinit(const char *str, const char *dir);
  int    CSmakeSched();
  int    CSreadSched(const char *dir);
  void   CSwriteSched(const char *dir);
  int    getCSdimension();
  char  *getCSpar(int dim);
  char  *getCSname();
  int    getCSparIndex(const char *dimPar);
  int    getCSnum();
  int    getCStype();
  double getCSval(int xdim, int index);
  void   CSprint(const char *str); 
  int    CSgetSched(const char *dir);
  int    CStypeIndexed();
  void   CSprint(const char *str);
#ifdef __cplusplus
}
#endif

#endif
