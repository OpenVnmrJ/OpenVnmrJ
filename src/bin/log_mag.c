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
/* "@(#)log_mag.c 9.1 4/16/93  (c) 1991 Spectroscopy Imaging Systems";     */
/****************************************************************************
               
               log_mag.c: General Purpose Coordinate Transformer


        The log_mag program provides coordinate conversions between
        the logical reference frame and the magnet reference frame. 

        The program is for use from the UNIX command line:


                log_mag  psi phi theta x1 x2 x3


        The arguments have the following interpretations:

        psi:            Euler angle in degrees, usually defined -90 to +90.

        phi:            Euler angle in degrees, usually defined -180 to 180.

        theta:          Euler angle in degrees, usually defined -90 to +90.

        x1,x2,x3:       Values of coordinates (cm) along the x1,x2,x3
                        axes of the logical frame.      


        The command returns three coordinate values and an information
        string to the standard output as follows:

        log_mag:        x    y    z

        If the program is misused it returns: 0.000000 0.000000 0.000000

**************************************************************************/
        
#include <stdio.h>
#include <math.h>


main(argc,argv)
int  argc;
char *argv[];
{
    double psi,phi,theta;
    double x,y,z,x1,x2,x3;
    double t11,t12,t13,t21,t22,t23,t31,t32,t33;

    if (argc != 7) {
        printf("%f\n%f\n%f",0.0,0.0,0.0);
        exit(1);
    }

    /****
    * Initialize variables
    ****/
    sscanf(argv[1],"%lf",&psi);
    sscanf(argv[2],"%lf",&phi);
    sscanf(argv[3],"%lf",&theta);
    sscanf(argv[4],"%lf",&x1);
    sscanf(argv[5],"%lf",&x2);
    sscanf(argv[6],"%lf",&x3);

    /****
    * Get coordinate transformation matrix elements
    ****/
    transmat(psi,phi,theta,&t11,&t12,&t13,&t21,&t22,&t23,&t31,&t32,&t33);

    x=t11*x1 + t21*x2 + t31*x3;
    y=t12*x1 + t22*x2 + t32*x3;
    z=t13*x1 + t23*x2 + t33*x3;
                
    printf("%f\n%f\n%f",x,y,z);
}
