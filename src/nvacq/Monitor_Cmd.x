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
/* nddsgen.C.NDDSType Monitor_Cmd; */
/* nddsgen.C.Output.Extension c; */
#ifdef RPC_HDR
%
%#include "ndds/ndds_c.h"
%#include "NDDS_Obj.h"
%
#endif


#ifdef RPC_HDR
%
%/* Console Case Actions */
%
#endif
#ifdef RPC_HDR
%#ifndef HOSTACQSTRUCTS_H
#endif
/* Console Case Actions */
const ECHO         =   1;
const XPARSER      =   2;
const APARSER      =   3;
const ABORTACQ     =   4;
const HALTACQ      =   5;
const STATINTERV   =   6;
const STARTLOCK    =   7;
const STARTINTERACT=   8;
const GETINTERACT  =   9;
const STOPINTERACT =   10;
const AUPDT        =   11;
const STOP_ACQ     =   12;
const ACQDEBUG     =   13;
const HEARTBEAT    =   14;
const GETSTATBLOCK =   15;
const ABORTALLACQS =   16;
const OK2TUNE      =   17;
const ROBO_CMD_ACK =   18;
 
#ifdef RPC_HDR
%/* Distinguish this one (console information)
%   from CONF_INFO (configuration information) */
#endif
 
const CONSOLEINFO  =   19;
 
const DOWNLOAD     =   20;
const STARTFIDSCOPE=   21;
const STOPFIDSCOPE =   22;
const GETCONSOLEDEBUG= 23;

const QUERY_CNTLRS_PRESENT = 25;

const FLASH_UPDATE= 30;
const FLASH_COMMIT= 31;
const FLASH_DELETE= 32;
const NDDS_VER_QUERY= 33;

#ifdef RPC_HDR
%#endif
#endif
 
#ifdef RPC_HDR
%#ifndef MAX_SHIMS_CONFIGURED
#endif
const MAX_SHIMS_CONFIGURED =   48;
#ifdef RPC_HDR
%#endif
#endif
 


#ifdef RPC_HDR
%
%/* topic name form */
%/* topic names form: h/master/cmdstrm, master/h/cmdstrm */
%
#endif
const CNTLR_PUB_CMDS_TOPIC_FORMAT_STR = "master/h/cmdstrm";
const HOST_SUB_CMDS_TOPIC_FORMAT_STR = "master/h/cmdstrm";

const HOST_PUB_CMDS_TOPIC_FORMAT_STR = "h/master/cmdstrm";
const CNTLR_SUB_CMDS_TOPIC_FORMAT_STR = "h/master/cmdstrm";

#ifdef RPC_HDR
%
%/* download types */
%
#endif
const  DATA_FID =  1;


const MAX_IPv4_UDP_SIZE_BYTES  =  65535;   /* IPv4 UDP max Packet size */
const NDDS_MAX_Size_Serialize = 64512;    /* serialization for NDDS  63KB */
const MAX_FIXCODE_SIZE = 64000;		 /* Max data size bytes */
const CMD_MAX_STR_SIZE = 512;

struct Monitor_Cmd {
        long cmd;
        long arg1;
        long arg2;
        long arg3;
        long arg4;
        long arg5;
	unsigned long crc32chksum;  /* CRC-32 checksum */
        opaque msgstr<CMD_MAX_STR_SIZE>;
};

#ifdef RPC_HDR
%
%#ifdef __cplusplus
%    extern "C" {
%#endif
%
%extern void getMonitor_CmdInfo(NDDS_OBJ *pObj);
%
%#ifdef __cplusplus
%}
%#endif
%
#endif
#ifdef RPC_CDR
%
% /* Direct Code from Codes_Downld.x */
%void getMonitor_CmdInfo(NDDS_OBJ *pObj)
%{
%    strcpy(pObj->dataTypeName,Monitor_CmdNDDSType);
%
%    pObj->TypeRegisterFunc = Monitor_CmdNddsRegister;
%    pObj->TypeAllocFunc = (DataTypeAllocate) Monitor_CmdAllocate;
%    pObj->TypeSizeFunc = Monitor_CmdMaxSize;
%}
%
#endif

