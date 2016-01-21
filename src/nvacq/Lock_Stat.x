%/*
%# 
%# Copyright (c) 1999-2000 Varian,Inc. All Rights Reserved. 
%# This software contains proprietary and confidential
%# information of Varian, Inc. and its contributors.
%# Use, disclosure and reproduction is prohibited without
%# prior consent.
%#
%*/
/* nddsgen.C.NDDSType Lock_Stat; */
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
const LOCK_PUB_STAT_TOPIC_FORMAT_STR = "lock/status";
const SUB_STAT_TOPIC_FORMAT_STR = "lock/status";

struct Lock_Stat {
        long lkon;
        long locked;
        long lkpower;
        long lkgain;
        long lkphase;
        long  lkLevelR;
        long  lkLevelI;
        long  lkSlopeR;
        long  lkSlopeI;
        double lkfreq;
};

#ifdef RPC_HDR
%
%#ifdef __cplusplus
%    extern "C" {
%#endif
%
%extern void getLock_StatInfo(NDDS_OBJ *pObj);
%
%#ifdef __cplusplus
%}
%#endif
%
#endif
#ifdef RPC_CDR
%
% /* Direct Code from Codes_Downld.x */
%void getLock_StatInfo(NDDS_OBJ *pObj)
%{
%    strcpy(pObj->dataTypeName,Lock_StatNDDSType);
%
%    pObj->TypeRegisterFunc = Lock_StatNddsRegister;
%    pObj->TypeAllocFunc = (DataTypeAllocate) Lock_StatAllocate;
%    pObj->TypeSizeFunc = Lock_StatMaxSize;
%}
%
#endif

