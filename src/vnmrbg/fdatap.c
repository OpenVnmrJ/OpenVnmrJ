/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/***/
/* fdatap: tools for setting and getting NMR parameters.
 ***/

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifndef VNMRJ
#define APLIST
#define ALTLIST
#define AQLIST
#define FTLIST
#define MODELIST

#include "fdatap.h"
#include "apod.h"
#include "prec.h"

#include "namelist.h"
#include "dataio.h"

#define ISQUAD( D ) (1 != (int)getQuad( fdata, NDQUADFLAG, (D) ))
#define FPR     (void)fprintf
#define FLTSIZE 4

static char axisNames[]  = "XYZABC";
static char defLabel[]   = "X_AXIS";

#else
#include "fdatap.h"
#endif

/***/
/* getParm: extract a numeric parameter by parameter code and
 *          optional dimension code.
 ***/

float getParm( float fdata[], int parmCode, int origDimCode )
{
    int dimCode, tpFlag;

/***/
/* Abort on negative parmCode:
 ***/

    if (parmCode < 0)
       {
        (void) fprintf( stderr,
                        "GetParm Error: bad code %d.\n",
                        parmCode );

        return( 0.0 );
       }

/***/
/* If parmCode is a direct FDATA address, return the value:
 ***/

    if (parmCode < FDATASIZE)
       {
        return( (float) fdata[parmCode] );
       }

/***/
/* Otherwise, interpret the generalized ND parameter:
 *  Translate dimension code according to current axis order.
 *  Translate generalized parameter code.
 ***/

    if (!(dimCode = getDim( fdata, origDimCode )))
       {
        (void) fprintf( stderr,
                        "GetParm Error: bad dim %d.\n",
                        dimCode );

        return( 0.0 );
       }

    if (fdata[FDTRANSPOSED] == 0.0)
       tpFlag = 0;
    else
       tpFlag = 1;

/***/
/* Nota Bene:
 *   NDSIZE is extracted according to original dim code and transpose.
 ***/
 
    switch( parmCode )
       {
        case NDSIZE:
           switch( origDimCode )
              {
               case -1:
                 if (tpFlag)
                    return( fdata[FDSPECNUM] );
                 else
                    return( fdata[FDSIZE] );
               case -2:
                 if (tpFlag)
                    return( fdata[FDSIZE] );
                 else
                    return( fdata[FDSPECNUM] );
               case -3:
                 return( fdata[FDF3SIZE] );
               case -4:
                 return( fdata[FDF4SIZE] );
               case 1:
                 return( fdata[FDSIZE] ); 
               case 2:
                 return( fdata[FDSPECNUM] ); 
               case 3:
                 return( fdata[FDF3SIZE] ); 
               case 4:
                 return( fdata[FDF4SIZE] ); 
              }
           break;

        case NDAPOD:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2APOD] ); 
               case 2:
                 return( fdata[FDF1APOD] ); 
               case 3:
                 return( fdata[FDF3APOD] ); 
               case 4:
                 return( fdata[FDF4APOD] ); 
              }
           break;

        case NDTDSIZE:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2TDSIZE] );
               case 2:
                 return( fdata[FDF1TDSIZE] );
               case 3:
                 return( fdata[FDF3TDSIZE] );
               case 4:
                 return( fdata[FDF4TDSIZE] );
              }
           break;

        case NDFTSIZE:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2FTSIZE] );
               case 2:
                 return( fdata[FDF1FTSIZE] );
               case 3:
                 return( fdata[FDF3FTSIZE] );
               case 4:
                 return( fdata[FDF4FTSIZE] );
              }
           break;

        case NDSW:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2SW] ); 
               case 2:
                 return( fdata[FDF1SW] ); 
               case 3:
                 return( fdata[FDF3SW] ); 
               case 4:
                 return( fdata[FDF4SW] ); 
              }
           break;

        case NDORIG:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2ORIG] ); 
               case 2:
                 return( fdata[FDF1ORIG] ); 
               case 3:
                 return( fdata[FDF3ORIG] ); 
               case 4:
                 return( fdata[FDF4ORIG] ); 
              }
           break;

        case NDCAR:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2CAR] );
               case 2:
                 return( fdata[FDF1CAR] );
               case 3:
                 return( fdata[FDF3CAR] );
               case 4:
                 return( fdata[FDF4CAR] );
              }
           break;

/***/
/* Nota Bene: since 0.0 indicates unset value here, "ZERO_EQUIV"
 *            is an unlikely value used to represent a set value
 *            which really is zero.
 ***/

        case NDCENTER:
           switch( dimCode )
              {
               case 1:
                 if (fdata[FDF2CENTER] == ZERO_EQUIV)
                    return( (float) 0.0 );
                 else
                    return( fdata[FDF2CENTER] );
               case 2:
                 if (fdata[FDF1CENTER] == ZERO_EQUIV)
                    return( (float) 0.0 );
                 else
                    return( fdata[FDF1CENTER] );
               case 3:
                 if (fdata[FDF3CENTER] == ZERO_EQUIV)
                    return( (float) 0.0 );
                 else
                    return( fdata[FDF3CENTER] );
               case 4:
                 if (fdata[FDF4CENTER] == ZERO_EQUIV)
                    return( (float) 0.0 );
                 else
                    return( fdata[FDF4CENTER] );
              }
           break;

        case NDOBS:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2OBS] ); 
               case 2:
                 return( fdata[FDF1OBS] ); 
               case 3:
                 return( fdata[FDF3OBS] ); 
               case 4:
                 return( fdata[FDF4OBS] ); 
              }
           break;

        case NDOFFPPM:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2OFFPPM] );
               case 2:
                 return( fdata[FDF1OFFPPM] );
               case 3:
                 return( fdata[FDF3OFFPPM] );
               case 4:
                 return( fdata[FDF4OFFPPM] );
              }
           break;

        case NDFTFLAG:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2FTFLAG] ); 
               case 2:
                 return( fdata[FDF1FTFLAG] ); 
               case 3:
                 return( fdata[FDF3FTFLAG] ); 
               case 4:
                 return( fdata[FDF4FTFLAG] ); 
              }
           break;

        case NDP0:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2P0] );
               case 2:
                 return( fdata[FDF1P0] );
               case 3:
                 return( fdata[FDF3P0] );
               case 4:
                 return( fdata[FDF4P0] );
              }
           break;

        case NDP1:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2P1] );
               case 2:
                 return( fdata[FDF1P1] );
               case 3:
                 return( fdata[FDF3P1] );
               case 4:
                 return( fdata[FDF4P1] );
              }
           break;

        case NDZF:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2ZF] );
               case 2:
                 return( fdata[FDF1ZF] );
               case 3:
                 return( fdata[FDF3ZF] );
               case 4:
                 return( fdata[FDF4ZF] );
              }
           break;

        case NDX1:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2X1] );
               case 2:
                 return( fdata[FDF1X1] );
               case 3:
                 return( fdata[FDF3X1] );
               case 4:
                 return( fdata[FDF4X1] );
              }
           break;
 
        case NDXN:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2XN] );
               case 2:
                 return( fdata[FDF1XN] );
               case 3:
                 return( fdata[FDF3XN] );
               case 4:
                 return( fdata[FDF4XN] );
              }
           break;
 
        case NDAPODCODE:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2APODCODE] );
               case 2:
                 return( fdata[FDF1APODCODE] );
               case 3:
                 return( fdata[FDF3APODCODE] );
               case 4:
                 return( fdata[FDF4APODCODE] );
              }
           break;

        case NDAPODQ1:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2APODQ1] );
               case 2:
                 return( fdata[FDF1APODQ1] );
               case 3:
                 return( fdata[FDF3APODQ1] );
               case 4:
                 return( fdata[FDF4APODQ1] );
              }
           break;

        case NDAPODQ2:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2APODQ2] );
               case 2:
                 return( fdata[FDF1APODQ2] );
               case 3:
                 return( fdata[FDF3APODQ2] );
               case 4:
                 return( fdata[FDF4APODQ2] );
              }
           break;

        case NDAPODQ3:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2APODQ3] );
               case 2:
                 return( fdata[FDF1APODQ3] );
               case 3:
                 return( fdata[FDF3APODQ3] );
               case 4:
                 return( fdata[FDF4APODQ3] );
              }
           break;

        case NDC1:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2C1] );
               case 2:
                 return( fdata[FDF1C1] );
               case 3:
                 return( fdata[FDF3C1] );
               case 4:
                 return( fdata[FDF4C1] );
              }
           break;

        case NDAQSIGN:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2AQSIGN] );
               case 2:
                 return( fdata[FDF1AQSIGN] );
               case 3:
                 return( fdata[FDF3AQSIGN] );
               case 4:
                 return( fdata[FDF4AQSIGN] );
              }
           break;

        case NDQUADFLAG:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2QUADFLAG] ); 
               case 2:
                 return( fdata[FDF1QUADFLAG] ); 
               case 3:
                 return( fdata[FDF3QUADFLAG] ); 
               case 4:
                 return( fdata[FDF4QUADFLAG] ); 
              }
           break;

        case NDUNITS:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2UNITS] ); 
               case 2:
                 return( fdata[FDF1UNITS] ); 
               case 3:
                 return( fdata[FDF3UNITS] ); 
               case 4:
                 return( fdata[FDF4UNITS] ); 
              }
           break;

        case NDLABEL1:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2LABEL] ); 
               case 2:
                 return( fdata[FDF1LABEL] ); 
               case 3:
                 return( fdata[FDF3LABEL] ); 
               case 4:
                 return( fdata[FDF4LABEL] ); 
              }
           break;

        case NDLABEL2:
           switch( dimCode )
              {
               case 1:
                 return( fdata[FDF2LABEL+1] ); 
               case 2:
                 return( fdata[FDF1LABEL+1] ); 
               case 3:
                 return( fdata[FDF3LABEL+1] ); 
               case 4:
                 return( fdata[FDF4LABEL+1] ); 
              }
           break;

        default:
           (void) fprintf( stderr,
                           "GetParm Error: bad ND code %d.\n",
                           parmCode );
       }

    return( (float) 0.0 );
}

#ifndef VNMRJ

int getParmI( fdata, parmCode, origDimCode )

   float fdata[FDATASIZE];
   int   parmCode, origDimCode;
{
    int ip;

    ip = (int)getParm( fdata, parmCode, origDimCode );

    return( ip );
}

/***/
/* setParm: set a header location by generalized parameter.
 ***/

int setParm( fdata, parmCode, parmVal, origDimCode )

   float parmVal, fdata[FDATASIZE];
   int   parmCode, origDimCode;
{
    int dimCode, tpFlag;

/***/
/* Abort on negative parmCode:
 ***/

    if (parmCode < 0)
       {
        (void) fprintf( stderr,
                        "SetParm Error: bad code %d.\n",
                        parmCode );

        return( 1 );
       }

/***/
/* If parmCode is a direct FDATA address, set the value:
 ***/

    if (parmCode < FDATASIZE)
       {
        fdata[parmCode] = parmVal;
        return( 0 );
       }

/***/
/* Otherwise, interpret the generalized ND parameter:
 *   Translate dimension code according to current axis order.
 *   Interpret the generalized parameter.
 ***/

    if (!(dimCode = getDim( fdata, origDimCode )))
       {
        (void) fprintf( stderr,
                        "SetParm Error: bad dim %d.\n",
                        dimCode );

        return( 0.0 );
       }

    if (fdata[FDTRANSPOSED] == 0.0)
       tpFlag = 0;
    else
       tpFlag = 1;

/***/
/* Nota Bene: NDSIZE is set according to original dim code and transpose:
 ***/

    switch( parmCode )
       {
        case NDSIZE:
           switch( origDimCode )
              {
               case -1:
                 if (tpFlag)
                    fdata[FDSPECNUM] = parmVal;
                 else
                    fdata[FDSIZE]    = parmVal;
                 break;
               case -2:
                 if (tpFlag)
                    fdata[FDSIZE]    = parmVal;
                 else
                    fdata[FDSPECNUM] = parmVal;
                 break;
               case -3:
                 fdata[FDF3SIZE]  = parmVal;
                 break;
               case -4:
                 fdata[FDF4SIZE]  = parmVal;
                 break;
               case 1:
                 fdata[FDSIZE]    = parmVal;
                 break;
               case 2:
                 fdata[FDSPECNUM] = parmVal;
                 break;
               case 3:
                 fdata[FDF3SIZE]  = parmVal;
                 break;
               case 4:
                 fdata[FDF4SIZE]  = parmVal;
                 break;
              }
           break;
        
        case NDAPOD:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2APOD]  = parmVal;
                 break;
               case 2:
                 fdata[FDF1APOD]  = parmVal;
                 break;
               case 3:
                 fdata[FDF3APOD]  = parmVal;
                 break;
               case 4:
                 fdata[FDF4APOD]  = parmVal;
                 break;
              }
           break;

        case NDTDSIZE:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2TDSIZE]  = parmVal;
                 break;
               case 2:
                 fdata[FDF1TDSIZE]  = parmVal;
                 break;
               case 3:
                 fdata[FDF3TDSIZE]  = parmVal;
                 break;
               case 4:
                 fdata[FDF4TDSIZE]  = parmVal;
                 break;
              }
           break;
       
        case NDFTSIZE:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2FTSIZE]  = parmVal;
                 break;
               case 2:
                 fdata[FDF1FTSIZE]  = parmVal;
                 break;
               case 3:
                 fdata[FDF3FTSIZE]  = parmVal;
                 break;
               case 4:
                 fdata[FDF4FTSIZE]  = parmVal;
                 break;
              }
           break;
 
        case NDSW:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2SW]  = parmVal;
                 break;
               case 2:
                 fdata[FDF1SW]  = parmVal;
                 break;
               case 3:
                 fdata[FDF3SW]  = parmVal;
                 break;
               case 4:
                 fdata[FDF4SW]  = parmVal;
                 break;
              }
           break;
        
        case NDORIG:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2ORIG]  = parmVal;
                 break;
               case 2:
                 fdata[FDF1ORIG]  = parmVal;
                 break;
               case 3:
                 fdata[FDF3ORIG]  = parmVal;
                 break;
               case 4:
                 fdata[FDF4ORIG]  = parmVal;
                 break;
              }
           break;
       
        case NDCAR:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2CAR]  = parmVal;
                 break;
               case 2:
                 fdata[FDF1CAR]  = parmVal;
                 break;
               case 3:
                 fdata[FDF3CAR]  = parmVal;
                 break;
               case 4:
                 fdata[FDF4CAR]  = parmVal;
                 break;
              }
           break;

        case NDCENTER:
           switch( dimCode )
              {
               case 1:
                 if (parmVal == 0.0)
                    fdata[FDF2CENTER]  = ZERO_EQUIV;
                 else
                    fdata[FDF2CENTER]  = parmVal;
                 break;
               case 2:
                 if (parmVal == 0.0)
                    fdata[FDF1CENTER]  = ZERO_EQUIV;
                 else
                    fdata[FDF1CENTER]  = parmVal;
                 break;
               case 3:
                 if (parmVal == 0.0)
                    fdata[FDF3CENTER]  = ZERO_EQUIV;
                 else
                    fdata[FDF3CENTER]  = parmVal;
                 break;
               case 4:
                 if (parmVal == 0.0)
                    fdata[FDF4CENTER]  = ZERO_EQUIV;
                 else
                    fdata[FDF4CENTER]  = parmVal;
                 break;
              }
           break;

        case NDOBS:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2OBS]    = parmVal;
                 break;
               case 2:
                 fdata[FDF1OBS]    = parmVal;
                 break;
               case 3:
                 fdata[FDF3OBS]    = parmVal;
                 break;
               case 4:
                 fdata[FDF4OBS]    = parmVal;
                 break;
              }
           break;
       
        case NDOFFPPM:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2OFFPPM]    = parmVal;
                 break;
               case 2:
                 fdata[FDF1OFFPPM]    = parmVal;
                 break;
               case 3:
                 fdata[FDF3OFFPPM]    = parmVal;
                 break;
               case 4:
                 fdata[FDF4OFFPPM]    = parmVal;
                 break;
              }
           break;
 
        case NDFTFLAG:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2FTFLAG] = parmVal;
                 break;
               case 2:
                 fdata[FDF1FTFLAG] = parmVal;
                 break;
               case 3:
                 fdata[FDF3FTFLAG] = parmVal;
                 break;
               case 4:
                 fdata[FDF4FTFLAG] = parmVal;
                 break;
              }
           break;

        case NDAPODCODE:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2APODCODE] = parmVal;
                 break;
               case 2:
                 fdata[FDF1APODCODE] = parmVal;
                 break;
               case 3:
                 fdata[FDF3APODCODE] = parmVal;
                 break;
               case 4:
                 fdata[FDF4APODCODE] = parmVal;
                 break;
              }
           break;

        case NDAPODQ1:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2APODQ1] = parmVal;
                 break;
               case 2:
                 fdata[FDF1APODQ1] = parmVal;
                 break;
               case 3:
                 fdata[FDF3APODQ1] = parmVal;
                 break;
               case 4:
                 fdata[FDF4APODQ1] = parmVal;
                 break;
              }
           break;

        case NDAPODQ2:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2APODQ2] = parmVal;
                 break;
               case 2:
                 fdata[FDF1APODQ2] = parmVal;
                 break;
               case 3:
                 fdata[FDF3APODQ2] = parmVal;
                 break;
               case 4:
                 fdata[FDF4APODQ2] = parmVal;
                 break;
              }
           break;

        case NDAPODQ3:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2APODQ3] = parmVal;
                 break;
               case 2:
                 fdata[FDF1APODQ3] = parmVal;
                 break;
               case 3:
                 fdata[FDF3APODQ3] = parmVal;
                 break;
               case 4:
                 fdata[FDF4APODQ3] = parmVal;
                 break;
              }
           break;

        case NDC1:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2C1] = parmVal;
                 break;
               case 2:
                 fdata[FDF1C1] = parmVal;
                 break;
               case 3:
                 fdata[FDF3C1] = parmVal;
                 break;
               case 4:
                 fdata[FDF4C1] = parmVal;
                 break;
              }
           break;

        case NDP0:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2P0] = parmVal;
                 break;
               case 2:
                 fdata[FDF1P0] = parmVal;
                 break;
               case 3:
                 fdata[FDF3P0] = parmVal;
                 break;
               case 4:
                 fdata[FDF4P0] = parmVal;
                 break;
              }
           break;

        case NDP1:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2P1] = parmVal;
                 break;
               case 2:
                 fdata[FDF1P1] = parmVal;
                 break;
               case 3:
                 fdata[FDF3P1] = parmVal;
                 break;
               case 4:
                 fdata[FDF4P1] = parmVal;
                 break;
              }
           break;

        case NDZF:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2ZF] = parmVal;
                 break;
               case 2:
                 fdata[FDF1ZF] = parmVal;
                 break;
               case 3:
                 fdata[FDF3ZF] = parmVal;
                 break;
               case 4:
                 fdata[FDF4ZF] = parmVal;
                 break;
              }
           break;

        case NDX1:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2X1] = parmVal;
                 break;
               case 2:
                 fdata[FDF1X1] = parmVal;
                 break;
               case 3:
                 fdata[FDF3X1] = parmVal;
                 break;
               case 4:
                 fdata[FDF4X1] = parmVal;
                 break;
              }
           break;
 
        case NDXN:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2XN] = parmVal;
                 break;
               case 2:
                 fdata[FDF1XN] = parmVal;
                 break;
               case 3:
                 fdata[FDF3XN] = parmVal;
                 break;
               case 4:
                 fdata[FDF4XN] = parmVal;
                 break;
              }
           break;
 
        case NDAQSIGN:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2AQSIGN] = parmVal;
                 break;
               case 2:
                 fdata[FDF1AQSIGN] = parmVal;
                 break;
               case 3:
                 fdata[FDF3AQSIGN] = parmVal;
                 break;
               case 4:
                 fdata[FDF4AQSIGN] = parmVal;
                 break;
              }
           break;

        case NDQUADFLAG:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2QUADFLAG] = parmVal;
                 break;
               case 2:
                 fdata[FDF1QUADFLAG] = parmVal;
                 break;
               case 3:
                 fdata[FDF3QUADFLAG] = parmVal;
                 break;
               case 4:
                 fdata[FDF4QUADFLAG] = parmVal;
                 break;
              }
           break;

        case NDUNITS:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2UNITS]  = parmVal;
                 break;
               case 2:
                 fdata[FDF1UNITS]  = parmVal;
                 break;
               case 3:
                 fdata[FDF3UNITS]  = parmVal;
                 break;
               case 4:
                 fdata[FDF4UNITS]  = parmVal;
                 break;
              }
           break;
        
        case NDLABEL1:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2LABEL]  = parmVal;
                 break;
               case 2:
                 fdata[FDF1LABEL]  = parmVal;
                 break;
               case 3:
                 fdata[FDF3LABEL]  = parmVal;
                 break;
               case 4:
                 fdata[FDF4LABEL]  = parmVal;
                 break;
              }
           break;
        
        case NDLABEL2:
           switch( dimCode )
              {
               case 1:
                 fdata[FDF2LABEL+1]  = parmVal;
                 break;
               case 2:
                 fdata[FDF1LABEL+1]  = parmVal;
                 break;
               case 3:
                 fdata[FDF3LABEL+1]  = parmVal;
                 break;
               case 4:
                 fdata[FDF4LABEL+1]  = parmVal;
                 break;
              }
           break;
        
        default:
           (void) fprintf( stderr,
                           "SetParm Error: bad ND code %d.\n",
                           parmCode );
       
           return( 1 );
       }

    return( 0 );
}

#endif

/***/
/* getDim: interpret absolute or relative dimension parameter.
 ***/

int getDim( fdata, dimCode )

   float fdata[FDATASIZE];
   int   dimCode;
{

    if (dimCode < 0)
       {
        dimCode = -dimCode;

        switch( dimCode )
           {
            case 1:
               return( 1 );
            case 2:
               return( 2 );
            case 3:
               return( 3 );
            case 4:
               return( 4 );
            default:
               (void) fprintf( stderr,
                               "GetDim Error: bad dim %d.\n",
                               -dimCode );
           }
       }
    else
       {
        if (dimCode == CUR_HDIM)
           {
            if (fdata[FDTRANSPOSED] == 0.0)
               dimCode = CUR_XDIM;
            else
               dimCode = CUR_YDIM;
           }

        if (dimCode == CUR_VDIM)
           {
            if (fdata[FDTRANSPOSED] == 0.0)
               dimCode = CUR_YDIM;
            else
               dimCode = CUR_XDIM;
           }

        dimCode = fdata[FDDIMORDER+dimCode-1];

        switch( dimCode )
           {
            case 1:
               return( 2 );
            case 2:
               return( 1 );
            case 3:
               return( 3 );
            case 4:
               return( 4 );
            default:
               (void) fprintf( stderr,
                               "GetDim Error: bad dim %d.\n",
                               dimCode );
           }
       }

    return( 0 );
}

#ifndef VNMRJ

/***/
/* setParmStr: sets a string parameter in fdata,
 ***/

int setParmStr( fdata, parmCode, value, origDimCode )

   float fdata[FDATASIZE];
   int   parmCode, origDimCode;
   char  *value;
{
    int i, dimCode, itemp;

/***/
/* Abort on negative parmCode:
 ***/

    if (parmCode < 0)
       {
        (void) fprintf( stderr,
                        "SetParmStr Error: bad code %d.\n",
                        parmCode );

        return( 1 );
       }

/***/
/* If parmCode is a direct FDATA address, set the value:
 ***/

    if (parmCode < FDATASIZE)
       {
        switch( parmCode )
           {
            case FDF2LABEL: case FDF1LABEL: case FDF3LABEL: case FDF4LABEL:
               (void) txt2Flt( value, fdata + parmCode, SIZE_NDLABEL );
               break;

            case FDSRCNAME:
               (void) txt2Flt( value, fdata + parmCode, SIZE_SRCNAME );
               break;

            case FDUSERNAME:
               (void) txt2Flt( value, fdata + parmCode, SIZE_USERNAME );
               break;

            case FDOPERNAME:
               (void) txt2Flt( value, fdata + parmCode, SIZE_OPERNAME );
               break;

            case FDCOMMENT:
               (void) txt2Flt( value, fdata + parmCode, SIZE_COMMENT );
               break;

            case FDTITLE:
               (void) txt2Flt( value, fdata + parmCode, SIZE_TITLE );
               break;

            default:
               (void) txt2Flt( value, fdata + parmCode, (int)(strlen(value)) );
               break;
           }

        return( 0 );
       }

/***/
/* Otherwise, interpret the generalized ND parameter:
 *  Translate dimension code according to current axis order.
 *  Translate generalized parameter code.
 ***/

    if (!(dimCode = getDim( fdata, origDimCode )))
       {
        (void) fprintf( stderr,
                        "SetParmStr Error: bad dim %d.\n",
                        dimCode );

        return( 0 );
       }

    switch( parmCode )
       {
        case NDLABEL:
           if (dimCode == 1)
              (void) txt2Flt( value, fdata + FDF2LABEL, SIZE_NDLABEL );
           else if (dimCode == 2)
              (void) txt2Flt( value, fdata + FDF1LABEL, SIZE_NDLABEL );
           else if (dimCode == 3)
              (void) txt2Flt( value, fdata + FDF3LABEL, SIZE_NDLABEL );
           else if (dimCode == 4) 
              (void) txt2Flt( value, fdata + FDF4LABEL, SIZE_NDLABEL );

           break;

        default:
           break;
       }

    return( 0 );
}

/***/
/* getParmStr: extracts a string parameter from fdata; value
 *             will be overwritten on next call.
 ***/

char *getParmStr( fdata, parmCode, origDimCode )

   float fdata[FDATASIZE];
   int   parmCode, origDimCode;
{
    static char buffer[NAMELEN+1];
    int i, dimCode, itemp;

    for( i = 0; i < NAMELEN; i++ ) buffer[i] = '\0';

/***/
/* Abort on negative parmCode:
 ***/

    if (parmCode < 0)
       {
        (void) fprintf( stderr,
                        "GetParmStr Error: bad code %d.\n",
                        parmCode );

        return( buffer );
       }

/***/
/* If parmCode is a direct FDATA address, return the value:
 ***/

    if (parmCode < FDATASIZE)
       {
        switch( parmCode )
           {
            case FDF2LABEL: case FDF1LABEL: case FDF3LABEL: case FDF4LABEL:
               (void) flt2Txt( fdata + parmCode, buffer, SIZE_NDLABEL );
               break;

            case FDSRCNAME:
               (void) flt2Txt( fdata + parmCode, buffer, SIZE_SRCNAME );
               break;

            case FDUSERNAME:
               (void) flt2Txt( fdata + parmCode, buffer, SIZE_USERNAME );
               break;

            case FDOPERNAME:
               (void) flt2Txt( fdata + parmCode, buffer, SIZE_OPERNAME );
               break;

            case FDCOMMENT:
               (void) flt2Txt( fdata + parmCode, buffer, SIZE_COMMENT );
               break;

            case FDTITLE:
               (void) flt2Txt( fdata + parmCode, buffer, SIZE_TITLE );
               break;

            case FDTRANSPOSED:
               if (1 == (int)getParm( fdata, parmCode, 0 ))
                  (void) strcpy( buffer, "Transposed" );
               else
                  (void) strcpy( buffer, "Not Transposed" );

               break;

            case FD2DPHASE:
               itemp = getParm( fdata, parmCode, 0 );

               if (itemp == FD_MAGNITUDE)
                  (void) strcpy( buffer, "Magnitude" );
               else if (itemp == FD_TPPI)
                  (void) strcpy( buffer, "TPPI" );
               else if (itemp == FD_STATES)
                  (void) strcpy( buffer, "States" );
               else if (itemp == FD_IMAGE)
                  (void) strcpy( buffer, "Image" );
               else
                  (void) sprintf( buffer, "%d=Unknown", itemp );

               break;

            case FDQUADFLAG:
               itemp = getParm( fdata, parmCode, 0 );

               if (itemp == 0)
                  (void) strcpy( buffer, "Complex" );
               else if (itemp == 1)
                  (void) strcpy( buffer, "Real" );
               else if (itemp == 2)
                  (void) strcpy( buffer, "Redfield" );
               else
                  (void) sprintf( buffer, "%d=Unknown", itemp );

               break;

            case FDF2QUADFLAG: case FDF1QUADFLAG:
            case FDF3QUADFLAG: case FDF4QUADFLAG:

               itemp = getParm( fdata, parmCode, 0 );

               if (itemp == FD_COMPLEX)
                  {
                   itemp = getParm( fdata, FDF2AQSIGN, 0 );

                   if (itemp == ALT_STATES)
                      (void) strcpy( buffer, "States-TPPI" );
                   else if (itemp == ALT_STATES_NEG)
                      (void) strcpy( buffer, "States-TPPI-N" );
                   else if (itemp == ALT_NONE_NEG)
                      (void) strcpy( buffer, "Complex-N" );
                   else
                      (void) strcpy( buffer, "Complex" );
                  }
               else if (itemp == FD_REAL) 
                  {
                   itemp = getParm( fdata, FDF2AQSIGN, 0 );

                   if (itemp == ALT_SEQUENTIAL)
                      (void) strcpy( buffer, "Sequential" );
                   else if (itemp == ALT_SEQUENTIAL_NEG)
                      (void) strcpy( buffer, "Sequential-N" );
                   else
                      (void) strcpy( buffer, "Real" );
                  }
               else if (itemp == FD_PSEUDOQUAD)
                  {
                   itemp = getParm( fdata, FDF2AQSIGN, 0 );

                   if (itemp == ALT_SEQUENTIAL_NEG)
                      (void) strcpy( buffer, "Redfield-N" );
                   else
                      (void) strcpy( buffer, "Redfield" );
                  }
               else
                  {
                   (void) sprintf( buffer, "%d=Unknown", itemp );
                  }

               break;

            case FDF2FTFLAG: case FDF1FTFLAG: case FDF3FTFLAG: case FDF4FTFLAG:
               if (1 == (int)getParm( fdata, parmCode, 0 ))
                  (void) strcpy( buffer, "Freq" );
               else
                  (void) strcpy( buffer, "Time" );
        
               break;

            case FDF2APODCODE: case FDF1APODCODE:
            case FDF3APODCODE: case FDF4APODCODE:
               itemp = getParm( fdata, parmCode, 0 );

               if (itemp >= 0 && itemp < APOD_COUNT)
                  (void) strcpy( buffer, apList[itemp].name );
               else
                  (void) sprintf( buffer, "%d=Unknown", itemp );

               break;

            default:
               (void) flt2Txt( fdata + parmCode, buffer, FDATASIZE-parmCode );
               break;
           }

        return( buffer );
       }

/***/
/* Otherwise, interpret the generalized ND parameter:
 *  Translate dimension code according to current axis order.
 *  Translate generalized parameter code.
 ***/

    if (!(dimCode = getDim( fdata, origDimCode )))
       {
        (void) fprintf( stderr,
                        "GetParmStr Error: bad dim %d.\n",
                        dimCode );

        return( buffer );
       }

    switch( parmCode )
       {
        case NDLABEL:
           if (dimCode == 1)
              (void) flt2Txt( fdata + FDF2LABEL, buffer, SIZE_NDLABEL );
           else if (dimCode == 2)
              (void) flt2Txt( fdata + FDF1LABEL, buffer, SIZE_NDLABEL );
           else if (dimCode == 3)
              (void) flt2Txt( fdata + FDF3LABEL, buffer, SIZE_NDLABEL );
           else if (dimCode == 4)
              (void) flt2Txt( fdata + FDF4LABEL, buffer, SIZE_NDLABEL );
           else
              (void) strcpy( buffer, "Unknown" );

           break;

        case NDQUADFLAG:
           itemp = getParm( fdata, parmCode, origDimCode );

           if (itemp == FD_COMPLEX)
              {
               itemp = getParm( fdata, NDAQSIGN, origDimCode );

               if (itemp == ALT_STATES)
                  (void) strcpy( buffer, "States-TPPI" );
               else if (itemp == ALT_STATES_NEG)
                  (void) strcpy( buffer, "States-TPPI-N" );
               else if (itemp == ALT_NONE_NEG)
                  (void) strcpy( buffer, "Complex-N" );
               else
                  (void) strcpy( buffer, "Complex" );
              }
           else if (itemp == FD_REAL)
              {
               itemp = getParm( fdata, NDAQSIGN, origDimCode );

               if (itemp == ALT_SEQUENTIAL)
                  (void) strcpy( buffer, "Sequential" );
               else if (itemp == ALT_SEQUENTIAL_NEG)
                  (void) strcpy( buffer, "Sequential-N" );
               else
                  (void) strcpy( buffer, "Real" );
              }
           else if (itemp == FD_PSEUDOQUAD)
              {
               itemp = getParm( fdata, NDAQSIGN, origDimCode );

               if (itemp == ALT_SEQUENTIAL_NEG)
                  (void) strcpy( buffer, "Redfield-N" );
               else
                  (void) strcpy( buffer, "Redfield" );
              }
           else
              (void) sprintf( buffer, "%d=Unknown", itemp );

           break;
  
        case NDFTFLAG:
           if (1 == (int)getParm( fdata, parmCode, origDimCode ))
              (void) strcpy( buffer, "Freq" );
           else
              (void) strcpy( buffer, "Time" );

           break;

        case NDAPODCODE:
           itemp = getParm( fdata, parmCode, origDimCode );

           if (itemp >= 0 && itemp < APOD_COUNT)
              (void) strcpy( buffer, apList[itemp].name );
           else
              (void) sprintf( buffer, "%d=Unknown", itemp );

           break;

        default:
           break;
       }

    return( buffer );
}

/***/
/* flt2Txt: unpacks text from float array.
 ***/

int flt2Txt( array, text, maxChar )

    char  *text;
    float *array;
    int   maxChar;
{
    int  i, count;
    char *cPtr;

    cPtr = (char *) array;

    for( i = 0; i < maxChar + 1; i++ ) text[i] = '\0';

    for( i = 0; i < maxChar && *cPtr; i++ )
       {
        text[i] = *cPtr++;
        if (!isprint( text[i] )) text[i] = '.';
       }

    return( 0 );
}

/***/
/* txt2Flt: packs text into float array without trailing null.
 ***/

int txt2Flt( text, array, maxChar )

    char  *text;
    float *array;
    int   maxChar;
{
    int  i, count;
    char *cPtr;

    cPtr = (char *) array;

    for( i = 0; i < maxChar;          i++ ) cPtr[i] = '\0';
    for( i = 0; i < maxChar && *text; i++ ) cPtr[i] = *text++;

    return( 0 );
}

/***/
/* getQuad: utility for extracting a given quadState.
 ***/

int getQuad( fdata, parmCode, origDimCode )

   float fdata[FDATASIZE];
   int   parmCode, origDimCode;
{
    int thisDim, dimCount;

    dimCount = getParm( fdata, FDDIMCOUNT, 0 );
    thisDim  = origDimCode < 0 ? -origDimCode : origDimCode;

    if (thisDim > dimCount) return( 1 );

    if (1 == (int)getParm( fdata, parmCode, origDimCode ))
       return( 1 );
    else
       return( 2 );
}

/***/
/* getAxis: utility for extracting dimension code by axis label.
 ***/

int getAxis( fdata, axisLabel )

   float fdata[FDATASIZE];
   char  *axisLabel;
{
    int  dim, dimCount;
    char *dimLabel;

/***/
/* First, look for label in recorded axis names.
 * Then, try generic axis names X_AXIS, Y_AXIS, etc.
 ***/

    dimCount = getParm( fdata, FDDIMCOUNT, 0 );

    for( dim = 1; dim <= dimCount; dim++ )
       {
        dimLabel = getParmStr( fdata, NDLABEL, dim );

        if (!strcasecmp( axisLabel, dimLabel )) return( dim );
       }

    for( dim = 1; dim <= dimCount; dim++ )
       {
        defLabel[0] = axisNames[dim-1];

        if (!strcasecmp( axisLabel, defLabel )) return( dim );
       }

    return( BAD_DIM );
}

/***/
/* getAxisChar: utility for extracting axis letter x y z a from 0,1,2,3.
 ***/

char getAxisChar( dim )

   int dim;
{
    switch( dim )
       {
        case 0:
           return( 'x' );
           break;
        case 1:
           return( 'y' );
           break;
        case 2:
           return( 'z' );
           break;
        case 3:
           return( 'a' );
           break;
        default:
           return( '?' );
           break;
       }

    return( '?' );
}

/***/
/* getAxisCharU: utility for extracting axis letter X Y Z A from 0,1,2,3.
 ***/

char getAxisCharU( dim )

   int dim;
{
    switch( dim )
       {
        case 0:
           return( 'X' );
           break;
        case 1:
           return( 'Y' );
           break;
        case 2:
           return( 'Z' );
           break;
        case 3:
           return( 'A' );
           break;
        default:
           return( '?' );
           break;
       }

    return( '?' );
}

/***/
/* is90_180: utility for testing if dimension PS -90,180.
 ***/

int is90_180( fdata, dimCode )

   float fdata[FDATASIZE];
   int   dimCode;
{
    if (180.0 == getParm( fdata, NDP1, dimCode ) &&
         0.000 == getParm( fdata, NDX1, dimCode ) &&
           0.000 == getParm( fdata, NDXN, dimCode )) return( 1 );

    return( 0 );
}

/***/
/* getFold: return the folding mode for a given dimension.
 ***/

int getFold( fdata, dimCode )

   float fdata[FDATASIZE];
   int   dimCode;
{

    if (0.000 == getParm( fdata, NDX1, dimCode ) &&
         0.000 == getParm( fdata, NDXN, dimCode ))
       {
        if (180.0 == getParm( fdata, NDP1, dimCode ))
           return( FOLD_INVERT );
        else
           return( FOLD_ORDINARY );
       }

    return( FOLD_BAD );
}

/***/
/* isInterleaved: utility for testing if dimension is interleaved complex.
 ***/

int isInterleaved( fdata, dimCode )

   float fdata[FDATASIZE];
   int   dimCode;
{

/***/
/* Complex dimensions 3 and higher are interleaved.
 * Dimension 2 (Y) of hypercomplex planes is interleaved.
 ***/

    if (dimCode > CUR_YDIM && ISQUAD( dimCode ))
       {
        return( 1 ); 
       }
    else if (dimCode == CUR_YDIM && ISQUAD( CUR_XDIM ) && ISQUAD( CUR_YDIM ))
       {
        return( 1 ); 
       }

    return( 0 ); 
}

/***/
/* isHdrStr: returns true if parmLoc is part of a header string.
 ***/

int isHdrStr( parmLoc )

   int parmLoc;
{
    if (parmLoc >= FDF2LABEL &&
          parmLoc < FDF2LABEL + SIZE_NDLABEL/FLTSIZE) return( 1 );

    if (parmLoc >= FDF1LABEL &&
          parmLoc < FDF1LABEL + SIZE_NDLABEL/FLTSIZE) return( 1 );

    if (parmLoc >= FDF3LABEL &&
          parmLoc < FDF3LABEL + SIZE_NDLABEL/FLTSIZE) return( 1 );

    if (parmLoc >= FDF4LABEL &&
          parmLoc < FDF4LABEL + SIZE_NDLABEL/FLTSIZE) return( 1 );

    if (parmLoc >= FDSRCNAME &&
          parmLoc < FDSRCNAME + SIZE_SRCNAME/FLTSIZE) return( 1 );

    if (parmLoc >= FDUSERNAME &&
          parmLoc < FDUSERNAME + SIZE_USERNAME/FLTSIZE) return( 1 );

    if (parmLoc >= FDOPERNAME &&
          parmLoc < FDOPERNAME + SIZE_OPERNAME/FLTSIZE) return( 1 );

    if (parmLoc >= FDTITLE  &&
          parmLoc < FDTITLE + SIZE_TITLE/FLTSIZE) return( 1 );

    if (parmLoc >= FDCOMMENT &&
          parmLoc < FDCOMMENT + SIZE_COMMENT/FLTSIZE) return( 1 );

    if (parmLoc == NDLABEL)  return( 1 );

    if (parmLoc == NDLABEL1) return( 1 );

    if (parmLoc == NDLABEL2) return( 1 );

    return( 0 );
}

/***/
/* isHdrStr0: returns length if parmLoc begins a header string.
 ***/

int isHdrStr0( parmLoc )

   int parmLoc;
{
    switch( parmLoc )
       {
        case NDLABEL:
        case FDF2LABEL:
        case FDF1LABEL:
        case FDF3LABEL:
        case FDF4LABEL:
           return( SIZE_NDLABEL );
           break;
        case FDSRCNAME:
           return( SIZE_SRCNAME );
           break;
        case FDUSERNAME:
           return( SIZE_USERNAME );
           break;
        case FDOPERNAME:
           return( SIZE_OPERNAME );
           break;
        case FDTITLE:
           return( SIZE_TITLE );
           break;
        case FDCOMMENT:
           return( SIZE_COMMENT );
           break;
        default:
           break;
       }

    return( 0 );
}

/***/
/* copyHdr: copy srcHdr to destHdr.
 ***/

int copyHdr( destHdr, srcHdr )

   float *destHdr, *srcHdr;
{
    int i;

    for( i = 0; i < FDATASIZE; i++ ) *destHdr++ = *srcHdr++;

    return( 0 );
}

/***/
/* readHdr: read header, test and adjust for byte-swap, set global swap flags.
 ***/

int readHdr( inUnit, fdata, swapDone )

   FILE_UNIT( inUnit );

   float fdata[FDATASIZE];
   int   *swapDone;
{
    float hdr[FDATASIZE];
    int   autoSwapState, byteSwapState, status, error;

    *swapDone = 0;

    autoSwapState = getAutoSwapFlag();
    byteSwapState = getByteSwapFlag();

    (void) setAutoSwapFlag( 0 );
    (void) setByteSwapFlag( 0 );

    error = dataRead( inUnit, fdata, sizeof(float)*FDATASIZE );

    (void) setByteSwapFlag( byteSwapState );
    (void) setAutoSwapFlag( autoSwapState );

    if (!autoSwapState) return( error );

    (void) copyHdr( hdr, fdata );
    
    switch( status = testHdr( hdr ))
       {
        case HDR_BAD:
           error = 2;
           break;

        case HDR_SWAPPED:
           if (autoSwapState > 1)
              {
               FPR( stderr, "Byte Swap Header: %d\n", FDATASIZE );
              }

           (void) copyHdr( fdata, hdr );
           (void) setByteSwapFlag( 1 );

           *swapDone = 1;
           break;

        case HDR_OK:
           break;

        default:
           break;
       }

   return( error );
}

/***/
/* readHdrB: read header, test and adjust for byte-swap, set global swap flags.
 ***/

int readHdrB( inUnit, fdata, maxTries, timeOut, swapDone )

   FILE_UNIT( inUnit );

   float fdata[FDATASIZE];
   int   maxTries, timeOut, *swapDone;
{
    float hdr[FDATASIZE];
    int   autoSwapState, byteSwapState, status, error;

    *swapDone = 0;

    autoSwapState = getAutoSwapFlag();
    byteSwapState = getByteSwapFlag();

    (void) setAutoSwapFlag( 0 );
    (void) setByteSwapFlag( 0 );

    error = 
     dataReadB( inUnit, fdata, sizeof(float)*FDATASIZE, maxTries, timeOut );

    (void) setByteSwapFlag( byteSwapState );
    (void) setAutoSwapFlag( autoSwapState );

    if (!autoSwapState) return( error );

    (void) copyHdr( hdr, fdata );
    
    switch( status = testHdr( hdr ))
       {
        case HDR_BAD:
           error = 2;
           break;

        case HDR_SWAPPED:
           if (autoSwapState > 1)
              {
               FPR( stderr, "Byte Swap Header: %d\n", FDATASIZE );
              }

           (void) copyHdr( fdata, hdr );
           (void) setByteSwapFlag( 1 );

           *swapDone = 1;
           break;

        case HDR_OK:
           break;

        default:
           break;
       }

   return( error );
}


/***/
/* initFDATA: create a blank FDATA, with minimal params so that 
 *            setParm()/getParm() will work.
 ***/

int initFDATA( fdata )

   float fdata[FDATASIZE];
{
    int i;

    for( i = 0; i < FDATASIZE; i++ ) fdata[i] = 0.0;

    fdata[FD2DVIRGIN]  = 1.0;
    fdata[FDDIMORDER1] = 2;
    fdata[FDDIMORDER2] = 1;
    fdata[FDDIMORDER3] = 3;
    fdata[FDDIMORDER4] = 4;

    return( 0 );
}

/***/
/* testHdr: test for valid header; performs byte-swap if needed.
 ***/

int testHdr( fdata )

   float  fdata[FDATASIZE];
{
    int   status;

/***/
/* FDMAGIC must be zero for valid data.
 * FDFLTORDER indicates possible byte-swap.
 ***/

    status = HDR_OK;

#ifndef CRAY

    if (fdata[FDMAGIC] != 0.0)
       {
        return( HDR_BAD );
       }

    if ((fdata[FDFLTORDER] != 0.0) && (fdata[FDFLTORDER] != (float)FDORDERCONS))
       {
        (void) swapHdr( fdata );

        if (fdata[FDFLTORDER] != (float)FDORDERCONS)
           {
            return( HDR_BAD );
           }

        status = HDR_SWAPPED;
       }

    if (fdata[FDSIZE] <= 0.0 || fdata[FDSIZE] != (int)fdata[FDSIZE])
       {
        return( HDR_BAD );
       }
#endif

    return( status );
}

/***/
/* swapHdr: perform byte-swapping of appropriate header locations.
 ***/

int swapHdr( fdata )

   float fdata[FDATASIZE];
{
    int   i;
    union fsw { float f; char s[4]; } in, out;

    for( i = 0; i < FDATASIZE; i++ )
       {
        if (!isHdrStr( i ))
           {
            in.f = fdata[i];

            out.s[0] = in.s[3]; 
            out.s[1] = in.s[2]; 
            out.s[2] = in.s[1]; 
            out.s[3] = in.s[0]; 

            fdata[i] = out.f;
           }
       }

    return( 0 );
}

/***/
/* isNullHdr: return 1 if fdata values are all null.
 ***/

int isNullHdr( fdata )

   float *fdata;
{
    int i, count;

    count = 0;

    for( i = 0; i < FDATASIZE; i++ ) if (fdata[i] == 0.0) count++;

    if (count == FDATASIZE) return( 1 );

    return( 0 );
}

/***/
/* Bottom.
 ***/
#endif
