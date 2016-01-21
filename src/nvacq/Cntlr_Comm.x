%/*
%*
%* Varian,Inc. All Rights Reserved.
%* This software contains proprietary and confidential
%* information of Varian, Inc. and its contributors.
%* Use, disclosure and reproduction is prohibited without
%* prior consent.
%*/
%/*
%*  Author: Greg Brissey  4/28/2004
%*/
/* nddsgen.C.NDDSType Cntlr_Comm; */
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
const CNTLR_CMD_ROLLCALL = 1;    /* master initiated roll call */
const CNTLR_CMD_HERE = 2;        /* controller responce to roll call */
const CNTLR_CMD_SET_DEBUGLEVEL = 3;
const CNTLR_CMD_REBOOT = 4;
const CNTLR_CMD_APARSER = 5;
const CNTLR_CMD_READY4SYNC = 6;
const CNTLR_CMD_STATE_UPDATE = 7;
const CNTLR_CMD_FFS_UPDATE = 8;
const CNTLR_CMD_FFS_COMMIT  = 9;
const CNTLR_CMD_FFS_DELETE  = 10;
const CNTLR_CMD_TUNE_QUIET  = 11;
const CNTLR_CMD_TUNE_ENABLE  = 12;
const CNTLR_CMD_TUNE_FINI  = 13;

const CNTLR_CMD_SET_ACQSTATE = 14;
const CNTLR_CMD_SET_SHIMDAC = 15;
const CNTLR_CMD_SET_FIFOTICK = 16;

const CNTLR_CMD_DDR_AT_BARRIER = 20;
const CNTLR_CMD_SET_NUM_ACTIVE_DDRS = 21;
const CNTLR_CMD_SET_UPLINK_CNTDWN = 22;

const CNTLR_RTVAR_UPDATE = 23;

const CNTLR_CMD_SET_WRKING_GRP = 24;
const CNTLR_CMD_NEXT_WRKING_GRP = 25;

#ifdef RPC_HDR
%
%/* topic name form */
%/* topic names form: h/rf1/dwnld/strm, rf1/h/dwnld/reply */
%
#endif

const CNTLR_COMM_MULTICAST_IP = "225.0.0.10";
const EXCEPTION_COMM_MULTICAST_IP = "225.0.0.11";
const DDRSYNC_COMM_MULTICAST_IP = "225.0.0.12";

const MASTER_PUB_COMM_TOPIC_FORMAT_STR = "master/cntlr/cmd";
const CNTLR_SUB_COMM_TOPIC_FORMAT_STR = "master/cntlr/cmd";

const CNTLR_PUB_COMM_TOPIC_FORMAT_STR = "%s/master/cmd";
const MASTER_SUB_COMM_PATTERN_TOPIC_STR = "*/master/cmd";
const MASTER_SUB_COMM_TOPIC_FORMAT_STR = "%s/master/cmd";

const EXCEPTION_PUB_COMM_TOPIC_FORMAT_STR = "cntlr/excp";
const EXCEPTION_SUB_COMM_TOPIC_FORMAT_STR = "cntlr/excp";
 
const MASTERBARRIER_PUB_COMM_TOPIC_FORMAT_STR = "master/ddr_barrier";
/* const DDRBARRIER_PUB_COMM_TOPIC_FORMAT_STR = "cntlr/ddr_barrier"; */
const DDRBARRIER_SUB_COMM_TOPIC_FORMAT_STR = "cntlr/ddr_barrier";
 
const DDRBARRIER_PUB_COMM_TOPIC_FORMAT_STR = "%s/ddr_barrier";
/* const DDRBARRIER_SUB_COMM_TOPIC_FORMAT_STR = "%s/ddr_barrier"; */
const DDRBARRIER_SUB_PATTERN_TOPIC_FORMAT_STR = "*/ddr_barrier";

const DDRGRPBARRIER_PUB_COMM_TOPIC_FORMAT_STR = "%s/ddr_grpbarrier";
const DDRGRPBARRIER_SUB_PATTERN_TOPIC_FORMAT_STR = "*/ddr_grpbarrier";

/* DDR query to Recvproc on number of FID buffers available */
const CNTLR_PUB_UPLOAD_BUFREQ_TOPIC_FORMAT_STR = "%s/h/upload/query";
const HOST_SUB_UPLOAD_BUFREQ_TOPIC_FORMAT_STR  = "%s/h/upload/query";

#ifdef RPC_HDR
%
%/* download types */
%
#endif
const  DATA_FID =  1;


const MAX_IPv4_UDP_SIZE_BYTES  =  65535;   /* IPv4 UDP max Packet size */
const COMM_MAX_STR_SIZE = 256;

struct Cntlr_Comm {
        char cntlrId[16];    /* controller ID String, e.g. rf1. Gradient1, ddr1, etc... */
        int cmd;
        int errorcode;
        int warningcode;
        long arg1;
        long arg2;
        long arg3;
	unsigned long crc32chksum;  /* CRC-32 checksum */
        string msgstr<COMM_MAX_STR_SIZE>;  /* reserved */
};

#ifdef RPC_HDR
%
%#ifdef __cplusplus
%    extern "C" {
%#endif
%
%extern void getCntlr_CommInfo(NDDS_OBJ *pObj);
%
%#ifdef __cplusplus
%}
%#endif
%
#endif
#ifdef RPC_CDR
%
% /* Direct Code from Codes_Downld.x */
%void getCntlr_CommInfo(NDDS_OBJ *pObj)
%{
%    strcpy(pObj->dataTypeName,Cntlr_CommNDDSType);
%
%    pObj->TypeRegisterFunc = Cntlr_CommNddsRegister;
%    pObj->TypeAllocFunc = (DataTypeAllocate) Cntlr_CommAllocate;
%    pObj->TypeSizeFunc = Cntlr_CommMaxSize;
%}
%
#endif

