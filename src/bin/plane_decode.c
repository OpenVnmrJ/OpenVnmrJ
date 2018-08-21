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
/*"@(#)plane_decode.c 9.1 4/16/93  (c) 1991 Spectroscopy Imaging Systems"; */
/****************************************************************************

                plane_decode.c: Two Point Plane Decoder

        The plane_decode program provides a means to convert the
        coordinates of two points (given in the logical frame)
        into the Euler angles and slice offset for the target
        plane.

        The program is for use from the UNIX command line:


              plane_decode  psi phi theta kr1 kp1 ks1 kr2 kp2 ks2


        The arguments have the following interpretations:

        psi:            Euler angle in degrees, usually defined -90 to +90.

        phi:            Euler angle in degrees, usually defined -180 to 180.

        theta:          Euler angle in degrees, usually defined -90 to +90.
        
        kr1,kp1,ks1:    Logical frame coordinates for the first point

        kr2,kp2,ks2:    Logical frame coordinates for the second point


        The command returns three angles (degrees), and a slice offset

                psi     phi     theta   pss

*****************************************************************************/
        
#include <stdio.h>
#include <math.h>


main(argc,argv)
int  argc;
char *argv[];
{
    double psi,phi,theta,kr1,kp1,ks1,kr2,kp2,ks2;
    double t11,t12,t13,t21,t22,t23,t31,t32,t33;
    double x1,x2,x3,x4,y1,y2,y3,y4,z1,z2,z3,z4;
    double lr,mr,nr,ls,ms,ns,lt,mt,nt;
    double sos,sot,leng,trav,sign;
    double tpsi,tphi,ttheta;

    if (argc != 10) {
        printf("%f\n%f\n%f\n%f",0.0,0.0,0.0,0.0);
        exit(1);
    }

    sscanf(argv[1],"%lf",&psi);
    sscanf(argv[2],"%lf",&phi);
    sscanf(argv[3],"%lf",&theta);
    sscanf(argv[4],"%lf",&kr1);
    sscanf(argv[5],"%lf",&kp1);
    sscanf(argv[6],"%lf",&ks1);
    sscanf(argv[7],"%lf",&kr2);
    sscanf(argv[8],"%lf",&kp2);
    sscanf(argv[9],"%lf",&ks2);

    if ((kr1 == kr2) && (kp1 == kp2) && (ks1 == ks2)) {
        printf("%f\n%f\n%f",0.0,0.0,0.0);
        exit(1);
    }

    transmat(psi,phi,theta,&t11,&t12,&t13,&t21,&t22,&t23,&t31,&t32,&t33);

    /****
    * Convert coordinates to magnet frame.  This defines two
    * points p1 & p2 in the magnet frame
    *****/
    x1 = t11*kr1 + t21*kp1 + t31*ks1;
    y1 = t12*kr1 + t22*kp1 + t32*ks1;
    z1 = t13*kr1 + t23*kp1 + t33*ks1;

    x2 = t11*kr2 + t21*kp2 + t31*ks2;
    y2 = t12*kr2 + t22*kp2 + t32*ks2;
    z2 = t13*kr2 + t23*kp2 + t33*ks2;

    /****
    * Compute the equation of ray p1 -> p2.  The direction cosines
    * are sign corrected because we want a vector product.
    *****/
    leng = sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1)+(z2-z1)*(z2-z1));
        
    lr = (x2 - x1)/leng; 
    mr = (y2 - y1)/leng; 
    nr = (z2 - z1)/leng;

    sign = (double)sign_direction_cosines(lr,mr,nr);
    lr = lr*sign; 
    mr = mr*sign; 
    nr = nr*sign;

    /****
    * Compute direction cosines of target.  We compute these by
    * taking a vector product with the ray direction cosines.
    *****/
    sign = (double)sign_direction_cosines(t31,t32,t33);
    ls = t31*sign;
    ms = t32*sign;
    ns = t33*sign;

    lt = (mr*ns - ms*nr);
    mt = (ls*nr - lr*ns);
    nt = (lr*ms - ls*mr);

    sign = (double)sign_direction_cosines(lt,mt,nt);
    lt = lt*sign;
    mt = mt*sign;
    nt = nt*sign;
    sot = lt*x2 + mt*y2 + nt*z2;

    /****
    * Axis Decode the Target Direction Cosines
    *****/

    axis_decoder(lt,mt,nt,&tpsi,&tphi,&ttheta);

    /****
    * Find polarity of target slice axis.  We need the transform
    * matrix for the target plane.  This lets us settle the sign
    * for sot
    *****/

    transmat(tpsi,tphi,ttheta,&t11,&t12,&t13,&t21,&t22,&t23,&t31,&t32,&t33);

    sign = (double)sign_direction_cosines(t31,t32,t33);
    sot = sot*sign;

    /****
    * Print out the results
    *****/
    printf("%f\n%f\n%f\n%f",tpsi,tphi,ttheta,sot);
}
