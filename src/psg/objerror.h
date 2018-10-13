/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------------
|
|       static struct codeWithMsg objerrMsgeTable[]
|
|       contains the Object error messages.
+-----------------------------------------------------------------------*/

struct codeWithMsg {
    int      code;
    char    *errmsg;
};

static struct codeWithMsg objerrMsgTable[] = {
    { UNKNOWN_MSG,   "Unknown Message to device." },
    { UNKNOWN_DEV_ATTR,  "Unknown Device Attribute." },
    { UNKNOWN_AP_ATTR,    "Unknown AP Device Attribute" },
    { UNKNOWN_ATTN_ATTR,   "Unknown Attenuator Device Attribute" },
    { UNKNOWN_APBIT_ATTR,  "Unknown AP Bit Device Attribute" },
    { NOT_PRESENT,   	   "Specifed Device Not Present." },
    { NO_MEMORY,   "Insuffient Memory for New Device!!." },
    { UNKNOWN_EVENT_ATTR,   "Unknown Event Device Attribute." },
    { UNKNOWN_EVENT_TYPE,   "Unknown Type of Event Device." },
    { UNKNOWN_EVENT_TYPE_ATTR,   "Unknown Event Device Type Attribute." },
    { NO_RTPAR_MEMORY, "Insuffient memory for Valid RT Parameter List."	},
    { PAST_MAX_RTPARS, "Exceeded Maximum Entries for Valid RT Parameter List."},
    { UNKNOWN_FREQ_ATTR,   "Unknown Frequency Device Attribute." },
    { UNKNOWN_RFCHAN_ATTR,   "Unknown RF Channel Device Attribute." },
    { ERROR_ABORT,   "Error Resulted in an Abort." },
    { 0,                   " " }
};

/*------------------------------------------------------------------------
|
|       static struct codeWithMsg objCmdMsgeTable[]
|
|       contains the Command messages.
+-----------------------------------------------------------------------*/

static struct codeWithMsg objCmdMsgTable[] = {
    { SET_ALL, "SET_ALL" },
    { SET_DEVSTATE, "SET_DEVSTATE" },
    { SET_DEVCNTRL, "SET_DEVCNTRL" },
    { SET_DEVCHANNEL, "SET_DEVCHANNEL" },
    { SET_APADR, "SET_APADR" },
    { SET_APREG, "SET_APREG" },
    { SET_APBYTES, "SET_APBYTES" },
    { SET_APMODE, "SET_APMODE" },
    { SET_MAXVAL, "SET_MAXVAL" },
    { SET_MINVAL, "SET_MINVAL" },
    { SET_OFFSET, "SET_OFFSET" },
    { SET_VALUE, "SET_VALUE" },
    { SET_DEFAULTS, "SET_DEFAULTS" },
    { SET_BIT, "SET_BIT" },
    { CLEAR_BIT, "CLEAR_BIT" },
    { SET_TRUE_EQ_ONE, "SET_TRUE_EQ_ONE" },
    { SET_TRUE_EQ_ZERO, "SET_TRUE_EQ_ZERO" },
    { SET_TRUE, "SET_TRUE" },
    { SET_FALSE, "SET_FALSE" },
    { SET_RTPARAM, "SET_RTPARAM" },
    { SET_TYPE, "SET_TYPE" },
    { SET_MAXRTPARS, "SET_MAXRTPARS" },
    { SET_VALID_RTPAR, "SET_VALID_RTPAR" },
    { SET_DBVALUE, "SET_DBVALUE" },
    { SET_H1FREQ, "SET_H1FREQ" },
    { SET_PTSVALUE, "SET_PTSVALUE" },
    { SET_IFFREQ, "SET_IFFREQ" },
    { SET_OVERRANGE, "SET_OVERRANGE" },
    { SET_INIT_OFFSET, "SET_INIT_OFFSET" },
    { SET_RFTYPE, "SET_RFTYPE" },
    { SET_RFBAND, "SET_RFBAND" },
    { SET_FREQSTEP, "SET_FREQSTEP" },
    { SET_OSYNBFRQ, "SET_OSYNBFRQ" },
    { SET_OSYNCFRQ, "SET_OSYNCFRQ" },
    { SET_SPECFREQ, "SET_SPECFREQ" },
    { SET_OFFSETFREQ, "SET_OFFSETFREQ" },
    { SET_PTSOPTIONS, "SET_PTSOPTIONS" },
    { SET_FREQVALUE, "SET_FREQVALUE" },
    { SET_RTPWR, "SET_RTPWR" },
    { SET_RTPWRF, "SET_RTPWRF" },
    { SET_PWR, "SET_PWR" },
    { SET_PWRF, "SET_PWRF" },
    { SET_RTPHASE90, "SET_RTPHASE90" },
    { SET_RTPHASE, "SET_RTPHASE" },
    { SET_PHASE90, "SET_PHASE90" },
    { SET_PHASE, "SET_PHASE" },
    { SET_PHLINE0, "SET_PHLINE0" },
    { SET_PHLINE90, "SET_PHLINE90" },
    { SET_PHLINE180, "SET_PHLINE180" },
    { SET_PHLINE270, "SET_PHLINE270" },
    { SET_PHASEBITS, "SET_PHASEBITS" },
    { SET_PHASEAPADR, "SET_PHASEAPADR" },
    { SET_PHASESTEP, "SET_PHASESTEP" },
    { SET_PHASEATTR, "SET_PHASEATTR" },
    { SET_HSLADDR, "SET_HSLADDR" },
    { SET_XMTRGATEBIT, "SET_XMTRGATEBIT" },
    { SET_XMTRGATE, "SET_XMTRGATE" },
    { SET_XMTRTYPE, "SET_XMTRTYPE" },
    { SET_AMPTYPE, "SET_AMPTYPE" },
    { SET_FREQOBJ, "SET_FREQOBJ" },
    { SET_ATTNOBJ, "SET_ATTNOBJ" },
    { SET_LNATTNOBJ, "SET_LNATTNOBJ" },
    { SET_APOVRADR, "SET_APOVRADR" },
    { SET_XMTR2AMPBDBIT, "SET_XMTR2AMPBDBIT" },
    { SET_AMPHIMIN, "SET_AMPHIMIN" },
    { SET_XMTRX2BIT, "SET_XMTRX2BIT" },
    { SET_HLBRELAYOBJ, "SET_HLBRELAYOBJ" },
    { SET_XMTRX2OBJ, "SET_XMTRX2OBJ" },
    { SET_WGGATEBIT, "SET_WGGATEBIT" },
    { SET_WGGATE, "SET_WGGATE" },
    { SET_OVRUNDRFLG, "SET_OVRUNDRFLG" },
    { SET_SWEEPCENTER, "SET_SWEEPCENTER" },
    { SET_SWEEPWIDTH, "SET_SWEEPWIDTH" },
    { SET_SWEEPNP, "SET_SWEEPNP" },
    { SET_SWEEPMODE, "SET_SWEEPMODE" },
    { SET_INITSWEEP, "SET_INITSWEEP" },
    { SET_INCRSWEEP, "SET_INCRSWEEP" },
    { SET_INITINCR, "SET_INITINCR" },
    { SET_ABSINCR, "SET_ABSINCR" },
    { SET_RTINCR, "SET_RTINCR" },
    { GET_ALL,	"GET_ALL" },
    { GET_STATE, "GET_STATE" },
    { GET_CNTRL, "GET_CNTRL" },
    { GET_CHANNEL, "GET_CHANNEL" },
    { GET_ADR,	"GET_ADR" },
    { GET_REG,	"GET_REG" },
    { GET_BYTES,	"GET_BYTES" },
    { GET_MODE,	"GET_MODE" },
    { GET_MAXVAL,	"GET_MAXVAL" },
    { GET_MINVAL,	"GET_MINVAL" },
    { GET_OFFSET,	"GET_OFFSET" },
    { GET_VALUE,	"GET_VALUE" },
    { GET_RTVALUE,	"GET_RTVALUE" },
    { GET_OF_FREQ,	"GET_OF_FREQ" },
    { GET_PTS_FREQ,	"GET_PTS_FREQ" },
    { GET_SPEC_FREQ,	"GET_SPEC_FREQ" },
    { GET_BASE_FREQ,	"GET_BASE_FREQ" },
    { GET_RFBAND,	"GET_RFBAND" },
    { GET_XMTRTYPE,	"GET_XMTRTYPE" },
    { GET_AMPTYPE,	"GET_AMPTYPE" },
    { GET_HIBANDMASK,	"GET_HIBANDMASK" },
    { GET_AMPHIBAND,	"GET_AMPHIBAND" },
    { GET_PHASEBITS, 	"GET_PHASEBITS" },
    { GET_PREAMP_SELECT, 	"GET_PREAMP_SELECT" },
    { GET_FREQSTEP,     "GET_FREQSTEP" },
    { GET_SWEEPCENTER,     "GET_SWEEPCENTER" },
    { GET_SWEEPWIDTH,     "GET_SWEEPWIDTH" },
    { GET_SWEEPNP,     "GET_SWEEPNP" },
    { GET_SWEEPINCR,     "GET_SWEEPINCR" },
    { GET_SWEEPMODE,     "GET_SWEEPMODE" },
    { GET_SWEEPMAXWIDTH,     "GET_SWEEPMAXWIDTH" },
    { GET_LBANDMAX,     "GET_LBANDMAX" },
    { GET_MAXXMTRFREQ,     "GET_MAXXMTRFREQ" },
    { GET_XMTRGATE,	"GET_XMTRGATE" },
    { GET_XMTRGATEBIT,	"GET_XMTRGATEBIT" },
    { GET_PWR,		"GET_PWR" },
    { GET_SIZETUNE,	"GET SIZETUNE" },
    { GET_PTSOPTIONS,	"GET PTSOPTIONS" },
    { GET_IFFREQ,	"GET IFFREQ" },
    { GET_PTSVALUE,	"GET PTSVALUE" },
    { GET_RFTYPE,	"GET RFTYPE" },
    { GET_OSYNBFRQ,	"GET OSYNBFRQ" },
    { GET_OSYNCFRQ,	"GET OSYNCFRQ" },
    { GET_PWRF,         "GET_PWRF" },
    { -1,                   " " }
};
