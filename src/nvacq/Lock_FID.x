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
/* nddsgen.C.NDDSType Lock_FID; */
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
const LOCK_PUB_FID_TOPIC_FORMAT_STR = "lock/fid";
const SUB_FID_TOPIC_FORMAT_STR = "lock/fid";


const LOCK_MAX_DATA = 5000;

struct Lock_FID {
        short  lkfid<LOCK_MAX_DATA>;
};

#ifdef RPC_HDR
%
%#ifdef __cplusplus
%    extern "C" {
%#endif
%
%extern void getLock_FIDInfo(NDDS_OBJ *pObj);
%
%#ifdef __cplusplus
%}
%#endif
%
#endif
#ifdef RPC_CDR
%
% /* Direct Code from Codes_Downld.x */
%void getLock_FIDInfo(NDDS_OBJ *pObj)
%{
%    strcpy(pObj->dataTypeName,Lock_FIDNDDSType);
%
%    pObj->TypeRegisterFunc = Lock_FIDNddsRegister;
%    pObj->TypeAllocFunc = (DataTypeAllocate) Lock_FIDAllocate;
%    pObj->TypeSizeFunc = Lock_FIDMaxSize;
%}
%
#endif

