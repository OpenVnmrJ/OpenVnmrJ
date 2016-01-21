/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "oopc.h"
/*-----------------------------------------------------------------
|
|       attr_valtype()
|       returns type of value associated with Attribute of Object
|
|                               Author Greg Brissey  1/10/89
|   Modified   Author     Purpose
|   --------   ------     -------
|
+---------------------------------------------------------------*/
int
attr_valtype(attribute)
int             attribute;
{
   switch (attribute)
   {
      case SET_DEFAULTS:
      case SET_HS_VALUE:
      case SET_MOD_VALUE:
      case SET_XGMODE_VALUE:
	 return (NO_VALUE);
	 break;

      case CLEAR_BIT:
      case SET_ALL:
      case SET_AMPTYPE:
      case SET_APADR:
      case SET_APBYTES:
      case SET_APMODE:
      case SET_APREG:
      case SET_BIT:
      case SET_DEVCHANNEL:
      case SET_DEVCNTRL:
      case SET_DEVSTATE:
      case SET_FALSE:
      case SET_FREQLIST:
      case SET_GATE_MODE:
      case SET_GATE_PHASE:
      case SET_GTAB:
      case SET_HSSEL_FALSE:
      case SET_HSSEL_TRUE:
      case SET_MAXRTPARS:
      case SET_MOD_MODE:
      case SET_MASK:
      case SET_MAXVAL:
      case SET_MINVAL:
      case SET_MIXER_VALUE:
      case SET_OFFSET:
      case SET_PHASE:
      case SET_PHASEAPADR:
      case SET_PHASE_REG:
      case SET_PHASE_MODE:
      case SET_PHASEATTR:
      case SET_PHASEBITS:
      case SET_PHLINE0:
      case SET_PHLINE90:
      case SET_PHLINE180:
      case SET_PHLINE270:
      case SET_PWR:
      case SET_PWRF:
      case SET_RCVRGATEBIT:
      case SET_RTPARAM:
      case SET_RTPHASE90:
      case SET_RTPHASE:
      case SET_RTPWR:
      case SET_RTPWRF:
      case SET_TRUE:
      case SET_TRUE_EQ_ONE:
      case SET_TRUE_EQ_ZERO:
      case SET_TYPE:
      case SET_VALID_RTPAR:
      case SET_VALUE:
      case SET_WGGATE:
      case SET_WGGATEBIT:
      case SET_XMTR2AMPBDBIT:
      case SET_RCVRGATE:
      case SET_XMTRGATE:
      case SET_XMTRGATEBIT:
      case SET_XMTRTYPE:
      case SET_XMTRX2BIT:
	 return (INTEGER);
	 break;

      case SET_AMPHIMIN:
      case SET_DBVALUE:
      case SET_FREQ_FROMSTORAGE:
      case SET_FREQSTEP:
      case SET_FREQVALUE:
      case SET_H1FREQ:
      case SET_IFFREQ:
      case SET_INCRSWEEP:
      case SET_INITSWEEP:
      case SET_OFFSETFREQ:
      case SET_OFFSETFREQ_STORE:
      case SET_OSYNBFRQ:
      case SET_OSYNCFRQ:
      case SET_OVERRANGE:
      case SET_OVRUNDRFLG:
      case SET_PHASESTEP:
      case SET_PTSOPTIONS:
      case SET_PTSVALUE:
      case SET_RFBAND:
      case SET_RFTYPE:
      case SET_INIT_OFFSET:
      case SET_SPECFREQ:
      case SET_SWEEPCENTER:
      case SET_SWEEPMODE:
      case SET_SWEEPNP:
      case SET_SWEEPWIDTH:
	 return (DOUBLE);
	 break;

      case SET_APOVRADR:
      case SET_ATTNOBJ:
      case SET_FREQOBJ:
      case SET_HLBRELAYOBJ:
      case SET_HSLADDR:
      case SET_HS_OBJ:
      case SET_LNATTNOBJ:
      case SET_MIXER_OBJ:
      case SET_MOD_OBJ:
      case SET_PHASE90:
      case SET_XGMODE_OBJ:
      case SET_XMTRX2OBJ:
	 return (POINTER);
	 break;

      case GET_ALL:
      case GET_AMPHIBAND:
      case GET_AMPTYPE:
      case GET_ADR:
      case GET_BYTES:
      case GET_CHANNEL:
      case GET_CNTRL:
      case GET_HIBANDMASK:
      case GET_MAXRTPARS:
      case GET_MAXVAL:
      case GET_MINVAL:
      case GET_MODE:
      case GET_OFFSET:
      case GET_PHASEBITS:
      case GET_PTSOPTIONS:
      case GET_PTSVALUE:
      case GET_PWR:
      case GET_PWRF:
      case GET_REG:
      case GET_RFTYPE:
      case GET_RTVALUE:
      case GET_SIZETUNE:
      case GET_STATE:
      case GET_TYPE:
      case GET_VALID_RTPAR:
      case GET_VALUE:
      case GET_XMTRGATE:
      case GET_XMTRGATEBIT:
      case GET_XMTRTYPE:
	 return (INTEGER);
	 break;

      case GET_BASE_FREQ:
      case GET_IFFREQ:
      case GET_LBANDMAX:
      case GET_MAXXMTRFREQ:
      case GET_OF_FREQ:
      case GET_OSYNBFRQ:
      case GET_OSYNCFRQ:
      case GET_PTS_FREQ:
      case GET_RFBAND:
      case GET_SPEC_FREQ:
      case GET_SWEEPCENTER:
      case GET_SWEEPINCR:
      case GET_SWEEPMODE:
      case GET_SWEEPNP:
      case GET_SWEEPWIDTH:
	 return (DOUBLE);
	 break;

      default:
	 return (UNKNOWN_TYPE);
	 break;
   }
}

