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
/* "@(#)sis_geometry.c 9.1 4/16/93  (c) 1991 Spectroscopy Imaging Systems"; */
/***************************************************************************
                       sis_geometry.c

        Header file for SEQD standalone interface programs
        which manage geometric and oblique angle computations.

***************************************************************************/

        
#include <stdio.h>
#include <math.h>

#define BIG 1.0e12              /*BIG=1/x when x=0*/


/************************************************************************
                          transmat

        Return nine doubles holding the values of the direction cosines
             for the logical to magnet frame transform matrix

        This procedure should have an identical selection mechanism
        to the obliker code used by the acquisition system except
        that the calculated transform matrix IS NOT transposed
        at the end of the computations.

************************************************************************/
transmat(psi,phi,theta,t11,t12,t13,t21,t22,t23,t31,t32,t33)

double psi,phi,theta;
double *t11,*t12,*t13,*t21,*t22,*t23,*t31,*t32,*t33;
{
    double D_R;
    double sinpsi,cospsi,sinphi,cosphi,sintheta,costheta;
    double m11,m12,m13,m21,m22,m23,m31,m32,m33;
    double im11,im12,im13,im21,im22,im23,im31,im32,im33;

    /*Calculate the core transform matrix*********************/
    D_R=atan(BIG)/90.0;

    cospsi=cos(D_R*psi);
    sinpsi=sin(D_R*psi);
        
    cosphi=cos(D_R*phi);
    sinphi=sin(D_R*phi);
        
    costheta=cos(D_R*theta);
    sintheta=sin(D_R*theta);

    m11=(sinphi*cospsi-cosphi*costheta*sinpsi);
    m12=(-1.0*sinphi*sinpsi-cosphi*costheta*cospsi);
    m13=(sintheta*cosphi);

    m21=(-1.0*cosphi*cospsi-sinphi*costheta*sinpsi);
    m22=(cosphi*sinpsi-sinphi*costheta*cospsi);
    m23=(sintheta*sinphi);

    m31=(sinpsi*sintheta);
    m32=(cospsi*sintheta);
    m33=(costheta);


    /*Generate the transform matrix for the patient case******/

    /*HEAD SUPINE*/
    im11=m11;       im12=m12;       im13=m13;
    im21=m21;       im22=m22;       im23=m23;
    im31=m31;       im32=m32;       im33=m33;


    /*Return the intermediate matrix without transpose*******/

    *t11=im11;      *t12=im12;      *t13=im13;
    *t21=im21;      *t22=im22;      *t23=im23;
    *t31=im31;      *t32=im32;      *t33=im33;
} 

/***************************************************************************
                        sign_direction_cosines

        Return the true sign (+/-1) of a set of direction cosines
                        as an integer

        Procedure uses a tolerance value to determine values of
             zero in any of the three direction cosines. 

****************************************************************************/

sign_direction_cosines(l,m,n)
double l,m,n;
{
    double  tolerance;
    int     sign;

    tolerance=0.97*atan(BIG)/90.0;

    if (fabs(n) > tolerance) {
        if (n > 0.0)
            sign = 1;
        else
	    sign = -1;
    }
    else {
        if (fabs(m) > tolerance) {
            if (m > 0.0)
                sign = 1;
            else
		sign = -1;
        }
        else {
            if (fabs(l) > tolerance) {
                if (l > 0.0)
                    sign = 1;
                else
		    sign = -1;
            }
        }
    }

    return(sign);
}

/*****************************************************************************
                        axis_decoder()

        Return the values of the euler angles for the plane whose slice
        select axis has direction cosines l,m,n in the magnet frame.

*****************************************************************************/
axis_decoder(l,m,n,e1,e2,e3)
double l,m,n,*e1,*e2,*e3;
{
    double psi,phi,theta;
    double sign,D_R;
    double a,b,c;
    double tolerance,test,tangent,tangent1,tangent2;

    /*Decode Operation*********************************************/

    D_R=atan(BIG)/90.0;
    tolerance=0.97*D_R;
        
    a=  l; b=  m; c=  n;

    /*Determine the true sign of a,b,c********/
    sign=(double)sign_direction_cosines(a,b,c);


    /*Correct the signs of a,b,c*******************/
    /*Note if thet are correct this has NO effect**/
    a=sign*a;
    b=sign*b;
    c=sign*c;


    /*Decode Euler Angle: psi****************************/
    test = 1 - c*c;
        
    if (test > tolerance*tolerance) {
        if (fabs(b) > tolerance)
            tangent=a/b;
        else
	    tangent=a*BIG;

        psi=atan(tangent);
    }
    else {
        psi=0.0;
    }


    /*Decode Euler Angle: theta**************************/
         
    if (fabs(psi) < (90.0*D_R - tolerance)) {
        if (test < tolerance*tolerance) {
            theta=0.0;
        }
        else {
            if ( fabs(c) > tolerance )
                tangent1= b/(c*cos(psi));
            else
		tangent1= b*BIG/(cos(psi));

            theta=atan(tangent1);
        }
    }
    else {
        if ( fabs (c) > tolerance )
            tangent2= a/(c*sin(psi));
        else
	    tangent2= a*BIG/(sin(psi));
       
        theta=atan(tangent2);
    }

    /*Set The Value Of phi***********************************/
    phi=0.0;

    /*Return Values*******************************************/
    *e1=psi/D_R;
    *e2=phi/D_R;
    *e3=theta/D_R;

}
