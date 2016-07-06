%/* 
%* Copyright (c) 1999-2000 Varian,Inc. All Rights Reserved. 
%* This software contains proprietary and confidential
%* information of Varian, Inc. and its contributors.
%* Use, disclosure and reproduction is prohibited without
%* prior consent.
%*
%*/
/* nddsgen.C.NDDSType Data_Upload; */
/* nddsgen.C.Output.Extension c; */
#ifdef RPC_HDR
%
%#include "ndds/ndds_c.h"
%#include "NDDS_Obj.h"
%
#endif


#ifdef RPC_HDR
%
%/* cmd types */
%
#endif
const C_UPLOAD = 1;
const C_RECVPROC_READY = 10;
const C_RECVPROC_DONE =  20;
const C_RECVPROC_CONTINUE_UPLINK = 30;

const NO_DATA = 0;
const ERROR_BLK = 11;
const COMPLETION_BLK = 22;
const DATA_BLK = 42;

#ifdef RPC_HDR
%
%/* topic name form */
%/* topic names form: h/rf1/dwnld/strm, rf1/h/dwnld/reply */
%
#endif
const CNTLR_PUB_UPLOAD_TOPIC_FORMAT_STR = "%s/h/upload/strm";
const HOST_SUB_UPLOAD_TOPIC_FORMAT_STR = "%s/h/upload/strm";

const CNTLR_SUB_UPLOAD_TOPIC_FORMAT_STR = "h/%s/upload/reply";
const HOST_PUB_UPLOAD_TOPIC_FORMAT_STR = "h/%s/upload/reply";

const CNTLR_SUB_UPLOAD_MCAST_TOPIC_FORMAT_STR = "h/ddr/upload/reply";
const HOST_PUB_UPLOAD_MCAST_TOPIC_FORMAT_STR = "h/ddr/upload/reply";
 
#ifdef RPC_HDR
%
%/* download types */
%
%#ifndef VXWORKS
%#include "rcvrDesc.h"
%#include "workQObj.h"
%#include "errLogLib.h"
%#include "expDoneCodes.h"
%#include "memorybarrier.h"
%#include "rcvrDesc.h"
%
%extern membarrier_t TheMemBarrier;
%
%#endif
%
%
#endif
const  DATAUPLOAD_FIDSTATBLK = 1;
const  DATAUPLOAD_FID =  2;
const  DATA_FID =  1;

const MAX_IPv4_UDP_SIZE_BYTES  =  65535;   /* IPv4 UDP max Packet size */
const NDDS_MAX_Size_Serialize = 64512;    /* serialization for NDDS  63KB */
const MAX_FIXCODE_SIZE = 64000;		 /* Max data size bytes */
const MAX_STR_SIZE = 512;

struct Data_Upload {
        long type;
        long sn;
        unsigned long elemId;
        unsigned long totalBytes;
        unsigned long  dataOffset;  /* for multi parts, offset into the buffer */
	unsigned long crc32chksum;  /* CRC-32 checksum */
        long          deserializerFlag;		    /* use this so that deserializer can mark this issue */
        unsigned long pPrivateIssueData;	    /* private per Issue data, Deserializer use only */
        opaque data<MAX_FIXCODE_SIZE>;
};

#ifdef RPC_HDR
%
%#ifdef __cplusplus
%    extern "C" {
%#endif
%
%extern void getData_UploadInfo(NDDS_OBJ *pObj);
%
%#ifndef VXWORKS
%extern int getNewDataValPtr(Data_Upload *objp);
%#endif
%
%#ifdef __cplusplus
%}
%#endif
%
#endif
#ifdef RPC_CDR
%#ifndef VXWORKS
% /* Direct Code from Codes_Downld.x */
%
%static char failsafeAddr[MAX_FIXCODE_SIZE+8];
%
% /* Custom Deserializer routine, ifdef out auto generated one */
%
%RTIBool
%NddsCDRDeserialize_Data_Upload(NDDSCDRStream *cdrs, Data_Upload *objp)
%{
%    RCVR_DESC_ID pRcvrDesc;
%    WORKQ_ID   pWorkQId;
%    WORKQ_ENTRY_ID pWorkQEntry;
%    SHARED_DATA_ID pSharedData;
%    unsigned long dummy;
%    int result;
%
%    struct timeval tp;
%    long usdif;
%
%    pRcvrDesc = (RCVR_DESC_ID) objp->pPrivateIssueData;
%
%#ifdef DIAG_TIMESTAMP
%    usdif = printTimeStamp(pRcvrDesc->cntlrId, "Deserialize_Data -> 1 <-", 
%                   &(pRcvrDesc->p4Diagnostics->tp),500000); 
%    if (usdif > 500000) {
%         DPRINT(-4,"-----------------------------------------------\n");  }
%#endif
%
%
%    if (!NddsCDRDeserialize_long(cdrs, &objp->type)) {
%    	return (RTI_FALSE);
%    }
%    if (!NddsCDRDeserialize_long(cdrs, &objp->sn)) {
%    	return (RTI_FALSE);
%    }
%    if (!NddsCDRDeserialize_u_long(cdrs, &objp->elemId)) {
%    	return (RTI_FALSE);
%    }
%    if (!NddsCDRDeserialize_u_long(cdrs, &objp->totalBytes)) {
%    	return (RTI_FALSE);
%    }
%    if (!NddsCDRDeserialize_u_long(cdrs, &objp->dataOffset)) {
%    	return (RTI_FALSE);
%    }
%    if (!NddsCDRDeserialize_u_long(cdrs, &objp->crc32chksum)) {
%    	return (RTI_FALSE);
%    }
%    /* then next member used internal only and should not be deserialized */
%
%
%    if (!NddsCDRDeserialize_long(cdrs, &objp->deserializerFlag)) {
%    	return (RTI_FALSE);
%    }
%
%    /* if (!NddsCDRDeserialize_u_long(cdrs, &objp->pPrivateIssueData)) { */
%    if (!NddsCDRDeserialize_u_long(cdrs, &dummy)) {
%    	return (RTI_FALSE);
%    }
%
%    /* pRcvrDesc = (RCVR_DESC_ID) objp->pPrivateIssueData; */
%
%#ifdef DIAG_TIMESTAMP 
%    usdif = printTimeStamp(pRcvrDesc->cntlrId, "Deserialize_Data -> 2 <-", 
%                   &(pRcvrDesc->p4Diagnostics->tp),100000); 
%    if (usdif > 100000) {
%         DPRINT(-4,"-----------------------------------------------\n");  }
%#endif
%
%
%    result = getNewDataValPtr(objp);
%
%#ifdef DIAG_TIMESTAMP 
%    usdif = printTimeStamp(pRcvrDesc->cntlrId, "Deserialize_Data -> 3 <-", 
%                   &(pRcvrDesc->p4Diagnostics->tp),100000); 
%    if (usdif > 100000) {
%         DPRINT(-4,"-----------------------------------------------\n");  }
%#endif
%      
%    if (!NddsCDRDeserialize_bytes(cdrs, (char *)objp->data.val, (unsigned int *)&objp->data.len, MAX_FIXCODE_SIZE)) {
%    	return (RTI_FALSE);
%    }
%    DPRINT3(+2,"'%s': Data_Deserializer() - transfer: to addr: 0x%lx, bytes recv: %ld\n",
%			pRcvrDesc->cntlrId,objp->data.val,objp->data.len);
%
%#ifdef DIAG_TIMESTAMP 
%    usdif = printTimeStamp(pRcvrDesc->cntlrId, "Deserialize_Data -> 3b <-", 
%                   &(pRcvrDesc->p4Diagnostics->tp),100000); 
%    if (usdif > 100000) {
%         DPRINT(-4,"-----------------------------------------------\n");  }
%#endif
%
%    pWorkQId = pRcvrDesc->pWorkQObj;
%    pWorkQEntry = pRcvrDesc->activeWrkQEntry;
%
%    objp->deserializerFlag = fillInWorkQ(objp, pWorkQEntry);
%    DPRINT2(-2,"'%s': Data_Deserializer() - fillInWorkQ returned: %d. \n", pRcvrDesc->cntlrId, objp->deserializerFlag);
%
%    if ( (objp->type == DATAUPLOAD_FID) && ((objp->dataOffset+objp->data.len) == objp->totalBytes) )
%       objp->deserializerFlag = DATA_BLK;
%
%#ifdef DIAG_TIMESTAMP 
%    /* -------------- test output ------------------------ */
%    usdif = printTimeStamp(pRcvrDesc->cntlrId, "Deserialize_Data -> 4 <-", 
%                   &(pRcvrDesc->p4Diagnostics->tp),100000); 
%    if (usdif > 100000) {
%         DPRINT(-4,"-----------------------------------------------\n");  }
%#endif
%
%    pSharedData = (SHARED_DATA_ID) lockSharedData(&TheMemBarrier);
%    if (pSharedData == NULL)
%        errLogSysQuit(LOGOPT,debugInfo,"Deserializer: Could not lock memory barrier mutex");
%
%      if ( pSharedData->discardIssues == 1)    /* error, start to discard any pubs receivered */
%          objp->deserializerFlag = NO_DATA;
%
%    unlockSharedData(&TheMemBarrier);
% 
%#ifdef DIAG_TIMESTAMP 
%    /* -------------- test output ------------------------ */
%    usdif = printTimeStamp(pRcvrDesc->cntlrId, "Deserialize_Data -> 5 <-", 
%                   &(pRcvrDesc->p4Diagnostics->tp),100000); 
%    if (usdif > 100000) {
%         DPRINT(-4,"-----------------------------------------------\n");  }
%#endif
%
%
%    if ( objp->deserializerFlag == ERROR_BLK)
%    {
%        pSharedData = (SHARED_DATA_ID) lockSharedData(&TheMemBarrier);
%        if (pSharedData == NULL)
%           errLogSysQuit(LOGOPT,debugInfo,"Deserializer: Could not lock memory barrier mutex");
%
%         pSharedData->AbortFlag = pSharedData->discardIssues = 1;   /* error, start to discard any pubs receivered */
%
%         unlockSharedData(&TheMemBarrier);
%
%    }
%
%    DPRINT2(+2,"'%s': Data_Deserializer() - Finial return code: %d\n", pRcvrDesc->cntlrId, objp->deserializerFlag);
%
%#ifdef DIAG_TIMESTAMP 
%    /* -------------- test output ------------------------ */
%    usdif = printTimeStamp(pRcvrDesc->cntlrId, "Deserialize_Data -> 6 <-", 
%                   &(pRcvrDesc->p4Diagnostics->tp),100000); 
%    if (usdif > 100000) {
%         DPRINT(-4,"-----------------------------------------------\n");  }
%#endif
%
%    return (RTI_TRUE);
%}
%#endif
%
% /* Direct Code from Codes_Downld.x */
%void getData_UploadInfo(NDDS_OBJ *pObj)
%{
%    strcpy(pObj->dataTypeName,Data_UploadNDDSType);
%
%    pObj->TypeRegisterFunc = Data_UploadNddsRegister;
%    pObj->TypeAllocFunc = (DataTypeAllocate) Data_UploadAllocate;
%    pObj->TypeSizeFunc = Data_UploadMaxSize;
%}
%
%
%#ifndef VXWORKS
%
% /* 'NDDS_DataFuncs.c' line: 228, 'ddr1': Data_UpldCallBack() - VVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVVV
% * 'ddr1': Data_UpldCallBack() - Reg. FidStatBlock: elemId: 1, trueElemId: 1, FID transfer @ addr: 0xfe81003c
% * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe81003c, bytes recv: 64000
% * 'ddr1': Data_UpldCallBack() - offset*len: 64000, totalBytes: 409600
% * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe81fa3c, bytes recv: 64000
% * 'ddr1': Data_UpldCallBack() - offset*len: 128000, totalBytes: 409600
% * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe82f43c, bytes recv: 64000
% * 'ddr1': Data_UpldCallBack() - offset*len: 192000, totalBytes: 409600
% * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe83ee3c, bytes recv: 64000
% * 'ddr1': Data_UpldCallBack() - offset*len: 256000, totalBytes: 409600
% * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe84e83c, bytes recv: 64000
% * 'ddr1': Data_UpldCallBack() - offset*len: 320000, totalBytes: 409600
% * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe85e23c, bytes recv: 64000
% * 'ddr1': Data_UpldCallBack() - offset*len: 384000, totalBytes: 409600
% * 'ddr1': Data_UpldCallBack() - FID transfer: new addr: 0xfe86dc3c, bytes recv: 25600
% * 'ddr1': Data_UpldCallBack() - offset*len: 409600, totalBytes: 409600
% * 'ddr1': Data_UpldCallBack() - Fid Xfer Cmplt, Send workQEntry (0x130e1c0) to next stage
% * 'ddr1': Data_UpldCallBack() - ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
% */
%
%int getNewDataValPtr(Data_Upload *objp)
%{
%   RCVR_DESC_ID pRcvrDesc;
%   WORKQ_ID   pWorkQId;
%   WORKQ_ENTRY_ID pWorkQEntry;
%   SHARED_DATA_ID pSharedData;
%   int retval,workQsAv;
%
%
%   pRcvrDesc = (RCVR_DESC_ID) objp->pPrivateIssueData;
%
%   pWorkQId = pRcvrDesc->pWorkQObj;
%   pWorkQEntry = pRcvrDesc->activeWrkQEntry;
%
%   retval = 0;
%
%   if (objp->type == DATAUPLOAD_FIDSTATBLK)
%   {
%         DPRINT1(+2,"'%s': getNewDataValPtr() - Statblock Transfer\n",pRcvrDesc->cntlrId);
%        /* get new workQ entry and set up the issue so NDDS load data into fidstatblock buffer */
%        if ( workQGetWillPend(pWorkQId) )
%        {
%           errLogRet(ErrLogOp,debugInfo,"'%s': getNewDataValPtr() - Going to block getting workQ\n",pRcvrDesc->cntlrId);
%         }
%        pRcvrDesc->activeWrkQEntry = workQGet(pWorkQId);
%
%        objp->data.val = (char*) pRcvrDesc->activeWrkQEntry->pFidStatBlk;
%
%#define INSTRUMENT
%#ifdef INSTRUMENT
%
%     workQsAv = numAvailWorkQs(pWorkQId);
%     
%     if (workQsAv < pRcvrDesc->p4Diagnostics->workQLowWaterMark)
%        pRcvrDesc->p4Diagnostics->workQLowWaterMark = workQsAv;
%
%#endif
%
%        /* retval = 1; */
%    }
%    else if (objp->type == DATAUPLOAD_FID)
%    {
%        if (objp->dataOffset == 0)
%        {
%           /* objp->data.val = getWorkQNewFidBufferPtr(pWorkQId,pRcvrDesc->activeWrkQEntry); */
%           objp->data.val = pRcvrDesc->activeWrkQEntry->pFidData;
%           pWorkQEntry->FidStrtAddr = objp->data.val;
%
%           DPRINT2(+2,"'%s': getNewDataValPtr() - Start of FID Transfer: start addr: 0x%lx\n",
%			pRcvrDesc->cntlrId,objp->data.val);
%        }
%        else
%        {
%           DPRINT3(+2,"'%s': getNewDataValPtr() - FID: offset: %ld, totalBytes: %ld\n",
%			pRcvrDesc->cntlrId,objp->dataOffset,objp->totalBytes);
%           objp->data.val = (pWorkQEntry->FidStrtAddr + objp->dataOffset);
%        }
%   }
%
%   pSharedData = (SHARED_DATA_ID) lockSharedData(&TheMemBarrier);
%   if (pSharedData == NULL)
%       errLogSysQuit(LOGOPT,debugInfo,"getNewDataValPtr: Could not lock memory barrier mutex");
%
%     /* just incase data file not open and NULL was returned */
%     if ( ( objp->data.val == NULL) || (pSharedData->discardIssues == 1) )
%        objp->data.val = failsafeAddr;
%
%    unlockSharedData(&TheMemBarrier);
%
%
%   return retval;
%}
%
%
%int fillInWorkQ(Data_Upload *objp, WORKQ_ENTRY_ID pWorkQEntry)
%{
%      int retcode = 0;
%      if (objp->type == DATAUPLOAD_FIDSTATBLK)
%      {
%         /*This is a Fid Stat Block */
%
%        pWorkQEntry->statBlkCRC = objp->crc32chksum;
%
%#ifdef LINUX
%        FSB_CONVERT_NTOH( pWorkQEntry->pFidStatBlk );
%#endif
%        switch((pWorkQEntry->pFidStatBlk->doneCode & 0xFFFF))
%         {
%          /* any for the following case means there is no data following this statblock 
%	   *	   and it should be pass on for processing 
%           */
%          case EXP_HALTED:
%          case EXP_ABORTED:
%          case HARD_ERROR:
%                  pWorkQEntry->statBlkType = ERRSTATBLK;
%                  DPRINT(+2,"fillInWorkQ: ERRSTATBLK\n");
%                  retcode = ERROR_BLK;
%                  break;
%
%          case STOP_CMPLT:
%          case SETUP_CMPLT:
%          case WARNING_MSG:
%                if ((pWorkQEntry->pFidStatBlk->doneCode & 0xFFFF) == WARNING_MSG)
%                {
%                   pWorkQEntry->statBlkType = WRNSTATBLK;
%                  DPRINT(+2,"fillInWorkQ: WRNSTATBLK\n");
%                }
%                else
%                {
%                   pWorkQEntry->statBlkType = SU_STOPSTATBLK;
%                  DPRINT(+2,"fillInWorkQ: SU_STOPSTATBLK\n");
%                }
%                 retcode = COMPLETION_BLK;
%                 break;
%
%            default:
%                pWorkQEntry->statBlkType = FIDSTATBLK;
%                 DPRINT(+2,"fillInWorkQ: FIDSTATBLK\n");
%                retcode = NO_DATA;
%                break;
%         }
%    }
%    else if (objp->type == DATAUPLOAD_FID)
%    {
%        pWorkQEntry->dataCRC = objp->crc32chksum;
%        retcode = NO_DATA;
%    }
%    return (retcode);
%}
%#endif
#endif

