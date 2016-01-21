%/*
%# 
%# Copyright (c) 1999-2000 Varian,Inc. All Rights Reserved. 
%# This software contains proprietary and confidential
%# information of Varian, Inc. and its contributors.
%# Use, disclosure and reproduction is prohibited without
%# prior consent.
%#
%*/
%/*
%*  Author: Greg Brissey  5/06/2004
%*/
/* nddsgen.C.NDDSType Lock_Cmd; */
/* nddsgen.C.Output.Extension c; */
#ifdef RPC_HDR
%
%#include "ndds/ndds_c.h"
%#include "NDDS_Obj.h"
%
#endif


#ifdef RPC_HDR
%
%/* Lock Case Actions */
%
#endif
const LK_SET_GAIN  =   21;
const LK_SET_POWER =   22;
const LK_SET_PHASE =   23;
const LK_ON        =   24;
const LK_OFF       =   25;
const LK_SET_RATE  =   26;
const LK_SET_FREQ  =   27;
const LKRATE       =   37;

const LK_AUTOLOCK  = 504;   /* Acode value */
 

#ifdef RPC_HDR
%
%/* topic name form */
%/* topic names form: ?/lock/cmdstrm, lock/?/cmdstrm */
%
#endif
const LOCK_CMDS_TOPIC_FORMAT_STR = "%s/lock/cmds";
const LOCK_SUB_CMDS_TOPIC_FORMAT_STR = "%s/lock/cmds";
const LOCK_SUB_CMDS_PATTERN_TOPIC_STR = "*/lock/cmds";

/* const HOST_PUB_CMDS_TOPIC_FORMAT_STR = "h/master/cmdstrm";
const CNTLR_SUB_CMDS_TOPIC_FORMAT_STR = "h/master/cmdstrm";
*/

/*
union UnionArg switch
  (int argType) {
	case 0: int intArg;
	case 1: float floatArg;
        case 2:  double dblArg;
  };


struct Lock_Cmd {
        long cmd;
	UnionArg arg1;
	UnionArg arg2;
	UnionArg arg3;
	unsigned long crc32chksum; 
};
*/

struct Lock_Cmd {
        long cmd;
        long arg1;
        long arg2;
        double arg3;
        double arg4;
	unsigned long crc32chksum;  /* CRC-32 checksum */
};

#ifdef RPC_HDR
%
%#ifdef __cplusplus
%    extern "C" {
%#endif
%
%extern void getLock_CmdInfo(NDDS_OBJ *pObj);
%
%#ifdef __cplusplus
%}
%#endif
%
#endif
#ifdef RPC_CDR
%
% /* Direct Code from Codes_Downld.x */
%void getLock_CmdInfo(NDDS_OBJ *pObj)
%{
%    strcpy(pObj->dataTypeName,Lock_CmdNDDSType);
%
%    pObj->TypeRegisterFunc = Lock_CmdNddsRegister;
%    pObj->TypeAllocFunc = (DataTypeAllocate) Lock_CmdAllocate;
%    pObj->TypeSizeFunc = Lock_CmdMaxSize;
%}
%
#endif

