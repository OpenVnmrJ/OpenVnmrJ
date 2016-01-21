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
/* nddsgen.C.NDDSType Lock_Status; */
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
const LOCK_PUB_CMDS_TOPIC_FORMAT_STR = "lock/status";
const SUB_CMDS_TOPIC_FORMAT_STR = "lock/status";


const LOCK_MAX_DATA = 5000;

struct Lock_Status {
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
        short  lkfid<LOCK_MAX_DATA>;
};

#ifdef RPC_HDR
%
%#ifdef __cplusplus
%    extern "C" {
%#endif
%
%extern void getLock_StatusInfo(NDDS_OBJ *pObj);
%
%#ifdef __cplusplus
%}
%#endif
%
#endif
#ifdef RPC_CDR
%
% /* Direct Code from Codes_Downld.x */
%void getLock_StatusInfo(NDDS_OBJ *pObj)
%{
%    strcpy(pObj->dataTypeName,Lock_StatusNDDSType);
%
%    pObj->TypeRegisterFunc = Lock_StatusNddsRegister;
%    pObj->TypeAllocFunc = (DataTypeAllocate) Lock_StatusAllocate;
%    pObj->TypeSizeFunc = Lock_StatusMaxSize;
%}
%
#endif

