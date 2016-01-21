%/*
%* 
%* Copyright (c) 1999-2000 Varian,Inc. All Rights Reserved. 
%* This software contains proprietary and confidential
%* information of Varian, Inc. and its contributors.
%* Use, disclosure and reproduction is prohibited without
%* prior consent.
%*/
%/*
%*  Author: Greg Brissey  4/20/2004
%*/
/* nddsgen.C.NDDSType Console_Stat; */
/* nddsgen.C.Output.Extension c; */
#ifdef RPC_HDR
%
%#include "ndds/ndds_c.h"
%#include "NDDS_Obj.h"
%
#endif


#ifdef RPC_HDR
%
%/*-------------------------------------------------------------------
%|   acquisition status codes for status display on Host
%|    values assigned to 'Acqstate' in structure CONSOLE_STATUS;
%|    symbols and values match definitions in STAT_DEFS.h,
%|    SCCS category xracq
%+-------------------------------------------------------------------*/
%
#endif
const ACQ_INACTIVE = 0;
const ACQ_REBOOT = 5;
const ACQ_IDLE 	= 10;
const ACQ_PARSE	= 15;
const ACQ_PREP 	= 16;
const ACQ_SYNCED = 	17;
const ACQ_ACQUIRE = 	20;
const ACQ_PAD	= 	25;
const ACQ_VTWAIT = 	30;
const ACQ_SPINWAIT = 	40;
const ACQ_AGAIN = 	50;
const ACQ_HOSTGAIN = 	55;
const ACQ_ALOCK = 	60;
const ACQ_AFINDRES = 	61;
const ACQ_APOWER = 	62;
const ACQ_APHASE = 	63;
const ACQ_FINDZ0 = 	65;
const ACQ_SHIMMING = 	70;
const ACQ_HOSTSHIM = 	75;
const ACQ_SMPCHANGE = 	80;
const ACQ_RETRIEVSMP = 	81;
const ACQ_LOADSMP = 	82;
const ACQ_INTERACTIVE=  90;
const ACQ_TUNING= 	100;
const ACQ_PROBETUNE= 	105;

const LSDV_EJECT=       0x40;
const LSDV_DETECTED=    0x4000;
const LSDV_LKREGULATED= 0x04;
const LSDV_LKNONREG=    0x08;
const LSDV_LKMASK=      0x0C;

#ifdef RPC_HDR
%
%/* topic name form */
%/* topic names form: h/rf1/dwnld/strm, rf1/h/dwnld/reply */
%
#endif
const CNTLR_PUB_STAT_TOPIC_FORMAT_STR = "master/h/constat";
const HOST_SUB_STAT_TOPIC_FORMAT_STR = "master/h/constat";

const CNTLR_SUB_STAT_TOPIC_FORMAT_STR = "h/master/constat";
const HOST_PUB_STAT_TOPIC_FORMAT_STR = "h/master/constat";

 
#ifdef RPC_HDR
%
%/* download types */
%
#endif
const  DATA_FID =  1;


const MAX_IPv4_UDP_SIZE_BYTES  =  65535;   /* IPv4 UDP max Packet size */
const NDDS_MAX_Size_Serialize = 64512;    /* serialization for NDDS  63KB */
/* const MAX_FIXCODE_SIZE = 64000;		 /* Max data size bytes */
%#ifndef MAX_SHIMS_CONFIGURED
const MAX_SHIMS_CONFIGURED = 48;
%#endif

/* see hostAcqStructs.h for defined member that are replicated in the publication */

struct Console_Stat {
		int     dataTypeID;	/* from STATUS_BLOCK structure, set to STATBLK (3) */
                int     AcqCtCnt;       /* reset from CONSOLE_STATUS structure */
                int     AcqFidCnt;
                int     AcqSample;
                int     AcqLockFreqAP;
                int     AcqLockFreq1;
                int     AcqLockFreq2;
                int     AcqNpErr;
                int     AcqSpinSet;
                int     AcqSpinAct;
		int	AcqSpinSpeedLimit;
                int     AcqPneuBearing;
                int     AcqPneuStatus;
                int     AcqPneuVtAir;
                int     AcqTickCountError;
                int     AcqChannelBitsConfig1;
                int     AcqChannelBitsConfig2;
                int     AcqChannelBitsActive1;
                int     AcqChannelBitsActive2;
                short   AcqRcvrNpErr;
                short   Acqstate;
                short   AcqOpsComplCnt;
                short   AcqLSDVbits;
                short   AcqLockLevel;
                short   AcqRecvGain;
		short   AcqSpinSpan;
		short	AcqSpinAdj;
		short	AcqSpinMax;
		short	AcqSpinActSp;
		short	AcqSpinProfile;
                short   AcqVTSet;
                short   AcqVTAct;
                short   AcqVTC;
                short   AcqPneuVTAirLimits;
                short   AcqPneuSpinner;
                short   AcqLockGain;
                short   AcqLockPower;
                short   AcqLockPhase;
                short   AcqShimValues[MAX_SHIMS_CONFIGURED];
                short   AcqShimSet;
                short   AcqOpsComplFlags;
                short   rfMonError;     /* RF Monitor error code */
                short   rfMonitor[8];   /* Avg RF power on up to 6 channels:
                                         * Bits 0-5: power (63 = trip level)
                                         * 6-10: knob position
                                         */
                short   statblockRate;  /* Time between statblocks in ms */
                short   gpaTuning[9];   /* 3 parameters per axis; order is:
                                         *  x-proportional, x-integral, x-spare,
                                         *  y-p, y-i, y-s, z-p, z-i, z-s */
                short   gpaError;       /* Grad Power Amp error code:
                                         * Bit 0: Amp Fault,  1: Amp Warning
                                         * 2: Amp Comm Fault, 3: Power Fault
                                         * 4: Power Warning,  5: PS Comm Fault
                                         * 6: NIC Fault,      7: External Fault
                                         * 8: No Comm w/ GPA
                                         */
                char  probeId1[20];
                char  gradCoilId[12];

};

#ifdef RPC_HDR
%
%#ifdef __cplusplus
%    extern "C" {
%#endif
%
%extern void getConsole_StatInfo(NDDS_OBJ *pObj);
%
%#ifdef __cplusplus
%}
%#endif
%
#endif
#ifdef RPC_CDR
%
% /* Direct Code from Codes_Downld.x */
%void getConsole_StatInfo(NDDS_OBJ *pObj)
%{
%    strcpy(pObj->dataTypeName,Console_StatNDDSType);
%
%    pObj->TypeRegisterFunc = Console_StatNddsRegister;
%    pObj->TypeAllocFunc = (DataTypeAllocate) Console_StatAllocate;
%    pObj->TypeSizeFunc = Console_StatMaxSize;
%}
%
#endif

