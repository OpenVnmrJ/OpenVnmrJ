/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */

#ifndef nddsfuncs_h
#define nddsfuncs_h

#ifndef RTI_NDDS_4x

#ifndef ndds_cdr_h
#include "ndds/ndds_cdr.h"
#endif

#endif /* RTI_NDDS_4x */

#ifndef ndds_c_h
#include "ndds/ndds_c.h"
#endif

#ifndef NDDS_Obj_h
#include "NDDS_Obj.h"
#endif

#ifndef threadfuncs_h
#include "threadfuncs.h"
#endif


/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

#ifndef RTI_NDDS_4x
extern RTIBool Codes_DownldCallback(const NDDSRecvInfo *issue, NDDSInstance *instance, void *callBackRtnParam);
extern void initiateNDDS(void);
extern int createCodeDownldSubscription(cntlr_t *pCntlrThr,char *subName);
extern int createCodeDownldPublication(cntlr_t *pCntlrThr,char *pubName);
extern int createAppHB_BESubscription(cntlr_t *pCntlrThr,char *subName);
extern int sendCmd(int cmdtype,int buftype,char *buft,char *label);
/* extern int getXferSize(NDDS_ID pubId, int *pipeFd); */
extern int getXferSize(NDDS_ID pubId, NDDSBUFMNGR_ID pNddsBufMngr);
extern int getXferdAck(NDDS_ID pubId, int *pipeFd);
extern int wait4ConsoleSub(NDDS_ID pubId);
extern int writeToConsole(char *cntlrId, NDDS_ID pubId, char *name, char* bufAdr,int size, int serialNum, int ackItr );
extern int readBlkingMsgePipe(int *pipeFd, char* msgeBuffer);
#else  /* RTI_NDDS_4x */
extern void initiateNDDS(int debugLevel);
extern int createCodeDownldSubscription(cntlr_t *pCntlrThr,char *subName);
extern int createCodeDownldPublication(cntlr_t *pCntlrThr,char *pubName);
extern int createAppHB_BESubscription(cntlr_t *pCntlrThr,char *subName);
extern int sendCmd(int cmdtype,int buftype,char *buft,char *label);
extern int getXferSize(NDDS_ID pubId, NDDSBUFMNGR_ID pNddsBufMngr);
extern int getXferdAck(NDDS_ID pubId, int *pipeFd);
extern int wait4ConsoleSub(NDDS_ID pubId);
extern int writeToConsole(char *cntlrId, NDDS_ID pubId, char *name, char* bufAdr,int size, int serialNum, int ackItr );
#endif /* RTI_NDDS_4x */
 
#ifdef __cplusplus
}
#endif
#endif

