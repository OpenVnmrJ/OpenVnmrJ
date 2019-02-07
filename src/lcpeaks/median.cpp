/* 
 * Varian,Inc. All Rights Reserved.
 * This software contains proprietary and confidential
 * information of Varian, Inc. and its contributors.
 * Use, disclosure and reproduction is prohibited without
 * prior consent.
 */
/* DISCLAIMER :
     * ------------
     *
 * This is a beta version of the GALAXIE integration library.
 * This code is under development and is provided for information purposes.
 * The classes names and interfaces as well as the file names and
 * organization is subject to changes. Moreover, this code has not been
 * fully tested.
 *
 * For any bug report, comment or suggestion please send an email to
 * gilles.orazi@varianinc.com
 *
 * Copyright Varian JMBS (2002)
 */

#include "common.h"
#include "median.h"
#include <assert.h>
#include <cmath>

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

/*
  Author: Wirth, Niklaus
  Title: Algorithms + data structures = programs
  Publisher: Englewood Cliffs: Prentice-Hall, 1976
  Physical description: 366 p.
  Series: Prentice-Hall Series in Automatic Computation
  
  find the kth smallest element in the array, and thus the median
  array a is modified by this procedure
*/
double ComputeMedian(double*  a, int n)
{
    assert(n>0);

    int k = n / 2 ;
    int l = 0     ;
    int m = n-1   ;

    while (l<m)
    {
        double x = a[k] ;
        int i = l ;
        int j = m ;
        do{
            while (a[i]<x) i++ ;
            while (x<a[j]) j-- ;
            if (i <= j)
            {
                double tmp = a[i] ;
                a[i] = a[j] ;
                a[j] = tmp ;
                i++ ;
                j-- ;
            }
        } while (i<=j) ;
        if (j<k) l=i ;
        if (k<i) m=j ;
    }
    return a[k] ;
}
