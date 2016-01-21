%/* 
%* Copyright (c) 1999-2000 Varian,Inc. All Rights Reserved. 
%* This software contains proprietary and confidential
%* information of Varian, Inc. and its contributors.
%* Use, disclosure and reproduction is prohibited without
%* prior consent.
%*
%*/
/* nddsgen.C.NDDSType Codes_Downld; */
/* nddsgen.C.NDDSType Codes_Downld_Status; */
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
const C_DOWNLOAD = 1;
const C_REPLY_ACK = 2;
const C_XFER_NUMBER = 3;
const C_XFER_ACK = 4;
const C_XFER_ABORT = 5;
const C_NAMEBUF_QUERY = 6;
const C_QUERY_ACK = 7;
const C_DWNLD_START = 8;
const C_DWNLD_CMPLT = 9;

#ifdef RPC_HDR
%
%/* topic name form */
%/* topic names form: h/rf1/dwnld/strm, rf1/h/dwnld/reply */
%
#endif
const HOST_PUB_TOPIC_FORMAT_STR = "h/%s/dwnld/strm";
const CNTLR_SUB_TOPIC_FORMAT_STR = "h/%s/dwnld/strm";

/* The Robart's & 128x128 EPI Settings */
const HOST_CODES_DOWNLD_PUB_QSIZE = 2;   /* was 1 */
const HOST_CODES_DOWNLD_PUB_HIWATER = 1;
const HOST_CODES_DOWNLD_PUB_LOWWATER = 0;
const HOST_CODES_DOWNLD_PUB_ACKSPERQ = 1;

const HOST_CODES_DOWNLD_SUB_QSIZE = 4;   /* was 1 */
const CTLR_CODES_DOWNLD_SUB_QSIZE = 4;   /* was 1 */

const CTLR_CODES_DOWNLD_PUB_QSIZE = 2;
const CTLR_CODES_DOWNLD_PUB_HIWATER = 1;
const CTLR_CODES_DOWNLD_PUB_LOWWATER = 0;
const CTLR_CODES_DOWNLD_PUB_ACKSPERQ = 1;

const CNTLR_PUB_TOPIC_FORMAT_STR = "%s/h/dwnld/reply";
const HOST_SUB_TOPIC_FORMAT_STR = "%s/h/dwnld/reply";

 
#ifdef RPC_HDR
%
%/* download types */
%
#endif
const  DYNAMIC =  1;
const TABLES = 2;
const FIXED = 3;
const VME = 4;


const MAX_IPv4_UDP_SIZE_BYTES  =  65535;   /* IPv4 UDP max Packet size */
const NDDS_MAX_Size_Serialize = 64512;    /* serialization for NDDS  63KB */

/* const MAX_FIXCODE_SIZE = 64000;	  /* Max data size bytes */
/* lowered size, that help the no-acode problem at Robarts & a 128x128 EPI , GMB 5/5/06 */
const MAX_FIXCODE_SIZE = 32000;		 /* Max data size bytes */
const MAX_STR_SIZE = 512;
/* const MAX_SIZE = 32600; */
/* const MAX_FIXCODE_SIZE = 16384; */

struct Codes_Downld {
        short           cmdtype;     /* download command, 'downLoad', etc. */
        short           status;     /* status */
        unsigned long  totalBytes;  /* total size of data being transferred */
        unsigned long  dataOffset;  /* for multi parts, offset into the buffer */
        unsigned int  sn;
        unsigned int  ackInterval;  /* interval to which ack OK or Error in receiving codes */
	unsigned long crc32chksum;  /* CRC-32 checksum */
        string label<MAX_STR_SIZE>;  /* name buffer label */
        string msgstr<MAX_STR_SIZE>;  /* reserved */
        opaque data<MAX_FIXCODE_SIZE>;
};

struct Codes_Downld_Status {
        unsigned int    dynBufsTotal; /*Total  # of dynamic buffers */
        unsigned int    dynBufsFree; /* # of dynamic buffers free */
        unsigned int    fixBufsTotal; /*Total  # of dynamic buffers */
        unsigned int    fixBufsFree; /* # of Fixed buffers free */
        unsigned int    fixMaxBufSize; 
};

#ifdef RPC_HDR
%
%#ifdef __cplusplus
%    extern "C" {
%#endif
%
%extern void getCodes_DownldInfo(NDDS_OBJ *pObj);
%extern void getCodes_DownldStatusInfo(NDDS_OBJ *pObj);
%
%#ifdef __cplusplus
%}
%#endif
%
#endif
#ifdef RPC_CDR
%
% /* Direct Code from Codes_Downld.x */
%void getCodes_DownldInfo(NDDS_OBJ *pObj)
%{
%    strcpy(pObj->dataTypeName,Codes_DownldNDDSType);
%
%    pObj->TypeRegisterFunc = Codes_DownldNddsRegister;
%    pObj->TypeAllocFunc = (DataTypeAllocate) Codes_DownldAllocate;
%    pObj->TypeSizeFunc = Codes_DownldMaxSize;
%}
%
%void getCodes_DownldStatusInfo(NDDS_OBJ *pObj)
%{
%    strcpy(pObj->dataTypeName,Codes_Downld_StatusNDDSType);
%
%    pObj->TypeRegisterFunc = Codes_Downld_StatusNddsRegister;
%    pObj->TypeAllocFunc = (DataTypeAllocate) Codes_Downld_StatusAllocate;
%    pObj->TypeSizeFunc = Codes_Downld_StatusMaxSize;
%}
%
#endif

