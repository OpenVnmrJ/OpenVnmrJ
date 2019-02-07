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

// $History: Wavelets.cpp $
/*  */
/* *****************  Version 8  ***************** */
/* User: Go           Date: 22/10/02   Time: 11:49 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* DIAMIR - 6566 : integration : switch to the C++ dll */
/*  */
/* *****************  Version 7  ***************** */
/* User: Go           Date: 16/10/02   Time: 16:20 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 6  ***************** */
/* User: Go           Date: 9/10/02    Time: 9:53 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */
/* backup */
/*  */
/* *****************  Version 5  ***************** */
/* User: Go           Date: 2/07/02    Time: 14:55 */
/* Updated in $/Orlando_2000/DLL_Integration/Cpp */

#include "common.h"
#include "Wavelets.h"
#include "debug.h"
#include <cmath>
#include "GeneralLibrary.h"

//Looking for memory leaks
#ifdef __VISUAL_CPP__
#include "LeakWatcher.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
#endif

/* --------------------------
     * Wavefilt::Wavefilt
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose : Constructor
 *           Initialize a wavelet filter object
 * History :
 * -------------------------- */
Wavefilt::Wavefilt(int n){
    double sig;
    int k;

    ncof = n ;
    cc = new double[n+1];
    cr = new double[n+1];

//wksp = NULL ;
    switch (n) {
        case 4: {
                cc[0] = 0.0 ;
                cc[1] = 0.4829629131445341 ;
                cc[2] = 0.8365163037378079 ;
                cc[3] = 0.2241438680420134 ;
                cc[4] = -0.1294095225512604 ;
                break;
            }
        case 12: {
                cc[0] = 0.0 ;
                cc[1] = 0.111540743350 ;
                cc[2] = 0.494623890398 ;
                cc[3] = 0.751133908021 ;
                cc[4] = 0.315250351709 ;
                cc[5] = -0.226264693965 ;
                cc[6] = -0.129766867567 ;
                cc[7] = 0.097501605587 ;
                cc[8] =  0.027522865530 ;
                cc[9] = -0.031582039318 ;
                cc[10] = 0.000553842201 ;
                cc[11] = 0.004777257511 ;
                cc[12] = -0.001077301085 ;
                break;
            }
        case 20: {
                cc[0] = 0.0 ;
                cc[1] = 0.026670057901 ;
                cc[2] = 0.188176800078 ;
                cc[3] = 0.527201188932 ;
                cc[4] = 0.688459039454 ;
                cc[5] = 0.281172343661 ;
                cc[6] = -0.249846424327 ;
                cc[7] = -0.195946274377 ;
                cc[8] = 0.127369340336 ;
                cc[9] = 0.093057364604 ;
                cc[10] = -0.071394147166 ;
                cc[11] = -0.029457536822 ;
                cc[12] =  0.033212674059 ;
                cc[13] = 	0.003606553567 ;
                cc[14] = -0.010733175483 ;
                cc[15] =  0.001395351747 ;
                cc[16] = 	0.001992405295 ;
                cc[17] = -0.000685856695 ;
                cc[18] = -0.000116466855 ;
                cc[19] = 	0.000093588670 ;
                cc[20] = -0.000013264203 ;
                break;
            }
        default:{
// assert(false,'pwtset: input should be 4, 12 or 20');
            }
    }
    sig = -1 ;
    for (k=1;k<=n;k++){
        cr[n+1-k] = sig*cc[k] ;
        sig = -sig ;
    }
    ioff = - (n/2) ;
    joff = ioff ;
}

/* --------------------------
     * Wavefilt::~Wavefilt
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose : Destructor
 * History :
 * -------------------------- */
Wavefilt::~Wavefilt(void){
    delete [] cc ;
    delete [] cr ;
}

/* --------------------------
     * Wavefilt::pwt
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose : Performs wavelet transform
 *           Code taken from numerical recipes
 * History :
 * -------------------------- */
void Wavefilt::pwt(double *a, unsigned long n, int isign)
//---------------------
//a indexes are 1-based
//---------------------
{
    double  ai, ai1;
    unsigned long i,ii,j,jf,jr,k,n1,ni,nj,nh,nmod ;

//if n < 4 then exit ;
    wksp = new double[n+1];
    nmod = ncof*n ;
    n1 = n-1 ;
    nh = n>>1 ;
    for(j=1;j<=n;j++) wksp[j] = 0.0 ;

    if (isign > 0) {
        ii = 1 ;
        i = 1 ;
        while (i <=n) {
            ni = i+nmod+ioff ;
            nj = i+nmod+joff ;
            for (k=1;k<=ncof;k++){
                jf = n1 & (ni+k) ;
                jr = n1 & (nj+k) ;
                wksp[ii] += cc[k]*a[jf+1] ;
                wksp[ii+nh] += cr[k]*a[jr+1] ;
            }
            i += 2 ;
            ii++ ;
        }
    }
    else {
        ii = 1 ;
        i = 1 ;
        while (i<=n) {
            ai = a[ii] ;
            ai1 = a[ii+nh] ;
            ni = i+nmod+ioff ;
            nj = i+nmod+joff ;
            for (k=1;k<=ncof;k++) {
                jf = (n1 & (ni+k))+1;
                jr = (n1 & (nj+k))+1;
                wksp[jf] += cc[k]*ai ;
                wksp[jr] += cr[k]*ai1 ;
            }
            i+=2 ;
            ii++ ;
        }
    }

    for (j=1;j<=n;j++) a[j]=wksp[j] ;

    delete [] wksp;

}

//--------------- Wavelet shrinkage functions -------------

/* --------------------------
     * softshrink
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose : Perform denoising by a soft wavelet
 *           shrinkage
 * History :
 * -------------------------- */
INT_FLOAT softshrink(INT_FLOAT x, INT_FLOAT s)
{
    INT_FLOAT _ax = fabs(x) ; //to avoid a double evaluation of abs(x)
    if (_ax < s)
        return 0 ;
    else
        return sign<INT_FLOAT>(x) * (_ax - s);
}

/* --------------------------
     * semisoftshrink
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose : Perform denoising by a semi-soft
 *           wavelets shrinkage
 * History :
 * -------------------------- */
INT_FLOAT semisoftshrink(INT_FLOAT L1,
    INT_FLOAT L2,
    INT_FLOAT x)
{
    INT_FLOAT R12 = L2 / (L2 - L1);
    INT_FLOAT _ax = fabs(x) ;
    if (_ax <= L1)
        return 0;
    else
        if (_ax > L2)
            return x;
    else
        return sign<INT_FLOAT>(x) * (_ax - L1) * R12;
}

/* --------------------------
     *
 * Author  : Bruno Orsier : Original Delphi code
             Gilles Orazi : C++ translation
 * Created : 06/2002
 * Purpose : Perform denoising by a semi-soft
 *           wavelets shrinkage
 * History :
 * -------------------------- */
// based on higher order polynomial
INT_FLOAT semisoftshrink2(INT_FLOAT L1,
    INT_FLOAT L2,
    INT_FLOAT x)
{
    INT_FLOAT _ax = fabs(x) ;
    if (_ax <= L1)
        return 0;
    else
        if (_ax > L2)
            return x ;
    else
        {
        INT_FLOAT tmp = L2 - L1 ;
        INT_FLOAT P3 = tmp * tmp * tmp;
        INT_FLOAT R1 = (L1 + L2) / P3;
        INT_FLOAT R2 = 2 * (L2 * L2) / P3;

        tmp = (_ax - L1) ;
        return sign<INT_FLOAT>(x) * (R2 - R1 * _ax) * tmp * tmp ;
    }
}
