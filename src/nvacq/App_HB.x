%/*
%*
%* Varian,Inc. All Rights Reserved.
%* This software contains proprietary and confidential
%* information of Varian, Inc. and its contributors.
%* Use, disclosure and reproduction is prohibited without
%* prior consent.
%*/
%/*
%*  Author: Greg Brissey  11/15/2004
%*/
/* nddsgen.C.NDDSType App_HB; */
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
%/* topic names form: h/rf1/dwnld/strm, rf1/h/dwnld/reply */
%
#endif

#define HB_USE_MULTICAST 
#ifdef HB_USE_MULTICAST 
const APP_HB_MULTICAST_IP = "225.0.0.15";
#else
const APP_HB_MULTICAST_IP = "0";   /* unicast */
#endif

const PUB_AppHB_TOPIC_FORMAT_STR = "%s/h/AppHB";
const SUB_AppHB_TOPIC_FORMAT_STR = "%s/h/AppHB";
const AppHB_PATTERN_FORMAT_STR  = "*/h/AppHB";
const PUB_NodeHB_TOPIC_FORMAT_STR = "%s/cntlr/AppHB";
const SUB_NodeHB_TOPIC_FORMAT_STR = "%s/cntlr/AppHB";
const nodeHB_PATTERN_FORMAT_STR  = "*/cntlr/AppHB";

struct App_HB {
        char AppIdStr[16];    /* controller ID String, e.g. rf1. Gradient1, ddr1, etc... */
	unsigned long HBcnt;
        int    ThreadId;
        int    AppId;
};

#ifdef RPC_HDR
%
%#ifdef __cplusplus
%    extern "C" {
%#endif
%
%extern void getApp_HBInfo(NDDS_OBJ *pObj);
%
%#ifdef __cplusplus
%}
%#endif
%
#endif
#ifdef RPC_CDR
%
% /* Direct Code from Codes_Downld.x */
%void getApp_HBInfo(NDDS_OBJ *pObj)
%{
%    strcpy(pObj->dataTypeName,App_HBNDDSType);
%
%    pObj->TypeRegisterFunc = App_HBNddsRegister;
%    pObj->TypeAllocFunc = (DataTypeAllocate) App_HBAllocate;
%    pObj->TypeSizeFunc = App_HBMaxSize;
%}
%
#endif

