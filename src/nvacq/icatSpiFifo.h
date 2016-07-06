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

/* RF FPGA SPI FIFO interface to the iCAT SPI */
/* Author: Greg Brissey  1/20/2010 */

/*
DESCRIPTION

   functions to handle RF FPGA SPI FIFO to the iCAT SPI

*/

#include <vxWorks.h>


#ifndef  icatSpiFiFoh
#define  icatSpiFiFoh

#include "logMsgLib.h"

extern int icatDebug;

#define IPRINT(level, str) \
        if (icatDebug > level) diagPrint(debugInfo,str)

#define IPRINT1(level, str, arg1) \
        if (icatDebug > level) diagPrint(debugInfo,str,arg1)

#define IPRINT2(level, str, arg1, arg2) \
        if (icatDebug > level) diagPrint(debugInfo,str,arg1,arg2)
 
#define IPRINT3(level, str, arg1, arg2, arg3) \
        if (icatDebug > level) diagPrint(debugInfo,str,arg1,arg2,arg3)
 
#define IPRINT4(level, str, arg1, arg2, arg3, arg4) \
        if (icatDebug > level) diagPrint(debugInfo, str,arg1,arg2,arg3,arg4)
 
#define IPRINT5(level, str, arg1, arg2, arg3, arg4, arg5 ) \
        if (icatDebug > level) diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5)
 
#define IPRINT6(level, str, arg1, arg2, arg3, arg4, arg5, arg6 ) \
        if (icatDebug > level) diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5,arg6)
 
#define IPRINT7(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7 ) \
   	if (icatDebug > level)  \
	  diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5,arg6,arg7)
   
#define IPRINT8(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 ) \
   	if (icatDebug > level)  \
	  diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8)
 
#define IPRINT9(level, str, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9 ) \
        if (icatDebug > level)  \
	  diagPrint(debugInfo,str,arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8,arg9)

int icatDebugOn();
int icatDebugOff();
int getIcatId();
int resetIcat();
int rebootIcat();
int getIcatSpiStatus();
int writeIcatFifo(UINT32 *buffer, UINT32 bytes);
int readIcatFifo(UINT32 cmd, UINT32 *buffer, UINT32 bytes);

#endif  /* icatSpiFiFo */
