%/*
%# 
%# Copyright (c) 1999-2004 Varian,Inc. All Rights Reserved. 
%# This software contains proprietary and confidential
%# information of Varian, Inc. and its contributors.
%# Use, disclosure and reproduction is prohibited without
%# prior consent.
%#
%# 
%*/
/* nddsgen.C.NDDSType FidCt_Stat; */
/* nddsgen.C.Output.Extension c; */
#ifdef RPC_HDR
%
%#include "ndds/ndds_c.h"
%#include "NDDS_Obj.h"
%
#endif


#ifdef RPC_HDR
%
%/* topic name form */
%/* topic names form: ?/lock1/cmdstrm, lock1/?/cmdstrm */
%
#endif
const FIDCT_PUB_STAT_TOPIC_FORMAT_STR = "%s/fid/status";
const FIDCT_SUB_PATTERN_TOPIC_STR = "*/fid/status";

struct FidCt_Stat {
        long FidCt;
        long Ct;
};

#ifdef RPC_HDR
%
%#ifdef __cplusplus
%    extern "C" {
%#endif
%
%extern void getFidCt_StatInfo(NDDS_OBJ *pObj);
%
%#ifdef __cplusplus
%}
%#endif
%
#endif
#ifdef RPC_CDR
%
% /* Direct Code from Codes_Downld.x */
%void getFidCt_StatInfo(NDDS_OBJ *pObj)
%{
%    strcpy(pObj->dataTypeName,FidCt_StatNDDSType);
%
%    pObj->TypeRegisterFunc = FidCt_StatNddsRegister;
%    pObj->TypeAllocFunc = (DataTypeAllocate) FidCt_StatAllocate;
%    pObj->TypeSizeFunc = FidCt_StatMaxSize;
%}
%
#endif

