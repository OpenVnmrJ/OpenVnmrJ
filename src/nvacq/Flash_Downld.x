%/* 
%* Copyright (c) 1999-2000 Varian,Inc. All Rights Reserved. 
%* This software contains proprietary and confidential
%* information of Varian, Inc. and its contributors.
%* Use, disclosure and reproduction is prohibited without
%* prior consent.
%*
%*/
/* nddsgen.C.NDDSType Flash_Downld; */
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
const FLASH_DOWNLD_FILE = 1;
const FLASH_DWNLD_COMMIT = 2;
const FLASH_DELETE_FILEPAT = 3;
const FLASH_REPLY_ACK = 4;
const FLASH_XFER_NUMBER = 5;
const FLASH_XFER_ACK = 6;
const FLASH_XFER_ABORT = 7;
const FLASH_NAMEBUF_QUERY = 8;
const FLASH_QUERY_ACK = 9;
const FLASH_DWNLD_START = 10;
const FLASH_DWNLD_CMPLT = 11;
 

#ifdef RPC_HDR
%
%/* topic name form */
%
#endif
const FLASH_HOST_PUB_TOPIC_STR = "h/flashdwnld/strm";
const FLASH_CNTLR_SUB_TOPIC_STR = "h/flashdwnld/strm";
const FLASH_CNTLR_SUB_TOPIC_PATTERN = "h/flashdwnld/*";

const FLASH_HOST_PUB_TOPIC_FORMAT_STR = "h/%s/flashdwnld/strm";
const FLASH_CNTLR_SUB_TOPIC_FORMAT_STR = "h/%s/flashdwnld/strm";
const FLASH_CNTLR_PUB_TOPIC_FORMAT_STR = "%s/h/flashdwnld/request";
const FLASH_HOST_SUB_TOPIC_FORMAT_STR = "%s/h/flashdwnld/request";
const FLASH_HOST_SUB_TOPIC_PATTERN     = "*/h/flashdwnld/request";

const FLASH_CNTLR_PUB_Q_SIZE = 1;	/* h/%s/flashdwnld/request */
const FLASH_CNTLR_PUB_HIWATERMARK = 1;	/* h/%s/flashdwnld/request */
const FLASH_CNTLR_PUB_LOWWATERMARK = 0;	/* h/%s/flashdwnld/request */
const FLASH_CNTLR_PUB_ACKS_PER_SENDQ = 1; /* h/%s/flashdwnld/request */
const FLASH_HOST_SUB_Q_SIZE = 2;	/* h/%s/flashdwnld/request */

const FLASH_HOST_PUB_Q_SIZE = 10;	/* h/%s/flashdwnld/strm */
const FLASH_HOST_PUB_HIWATERMARK = 2;
const FLASH_HOST_PUB_LOWWATERMARK = 0;
const FLASH_HOST_PUB_ACKS_PER_SENDQ = 10;
const FLASH_CNTLR_SUB_Q_SIZE = 10;
 
const MAX_IPv4_UDP_SIZE_BYTES  =  65535;   /* IPv4 UDP max Packet size */
const NDDS_MAX_Size_Serialize = 64512;    /* serialization for NDDS  63KB */
const MAX_FLASHDATA_SIZE = 16384;	  /* Max data size bytes */
const FLASH_MAX_NAME_SIZE = 64;
const FLASH_MAX_STR_SIZE = 512;
const FLASH_MAX_CNTLRID_SIZE = 12;
const FLASH_MAX_MD5_SIZE = 34;

struct Flash_Downld {
        char           cntlrId[FLASH_MAX_CNTLRID_SIZE];
        int            cmd;     /* download command, 'downLoad', etc. */
        int            status;     /* status */
        unsigned long  totalBytes;  /* total size of data being transferred */
        unsigned long  dataOffset;  /* for multi parts, offset into the buffer */
        char           md5digest[FLASH_MAX_MD5_SIZE];  /* md5 digest of file */
        char           filename[FLASH_MAX_NAME_SIZE];  /* reserved */
        char           msgstr[FLASH_MAX_STR_SIZE];  /* reserved */
        opaque         data<MAX_FLASHDATA_SIZE>;
};


#ifdef RPC_HDR
%
%#ifdef __cplusplus
%    extern "C" {
%#endif
%
%extern void getFlash_DownldInfo(NDDS_OBJ *pObj);
%
%#ifdef __cplusplus
%}
%#endif
%
#endif
#ifdef RPC_CDR
%
% /* Direct Code from Codes_Downld.x */
%void getFlash_DownldInfo(NDDS_OBJ *pObj)
%{
%    strcpy(pObj->dataTypeName,Flash_DownldNDDSType);
%
%    pObj->TypeRegisterFunc = Flash_DownldNddsRegister;
%    pObj->TypeAllocFunc = (DataTypeAllocate) Flash_DownldAllocate;
%    pObj->TypeSizeFunc = Flash_DownldMaxSize;
%}
%
#endif

