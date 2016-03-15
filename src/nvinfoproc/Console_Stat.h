/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
  WARNING: THIS FILE IS AUTO-GENERATED. DO NOT MODIFY.

  This file was generated from Console_Stat.idl using "rtiddsgen".
  The rtiddsgen tool is part of the RTI Data Distribution Service distribution.
  For more information, type 'rtiddsgen -help' at a command shell
  or consult the RTI Data Distribution Service manual.
*/

#ifndef Console_Stat_h
#define Console_Stat_h

#ifndef NDDS_STANDALONE_TYPE
    #ifdef __cplusplus
        #ifndef ndds_cpp_h
            #include "ndds/ndds_cpp.h"
        #endif
    #else
        #ifndef ndds_c_h
            #include "ndds/ndds_c.h"
        #endif
    #endif
#else
    #include "ndds_standalone_type.h"
#endif


/*

*






*/

/*

*  Author: Greg Brissey  4/20/2004

*/

#include "NDDS_Obj.h"

/*-------------------------------------------------------------------

|   acquisition status codes for status display on Host

|    values assigned to 'Acqstate' in structure CONSOLE_STATUS;

|    symbols and values match definitions in STAT_DEFS.h,

|    SCCS category xracq

+-------------------------------------------------------------------*/
                
#define ACQ_INACTIVE (0)                
                        
#define ACQ_REBOOT (5)                
                        
#define ACQ_IDLE (10)                
                        
#define ACQ_PARSE (15)                
                        
#define ACQ_PREP (16)                
                        
#define ACQ_SYNCED (17)                
                        
#define ACQ_ACQUIRE (20)                
                        
#define ACQ_PAD (25)                
                        
#define ACQ_VTWAIT (30)                
                        
#define ACQ_SPINWAIT (40)                
                        
#define ACQ_AGAIN (50)                
                        
#define ACQ_HOSTGAIN (55)                
                        
#define ACQ_ALOCK (60)                
                        
#define ACQ_AFINDRES (61)                
                        
#define ACQ_APOWER (62)                
                        
#define ACQ_APHASE (63)                
                        
#define ACQ_FINDZ0 (65)                
                        
#define ACQ_SHIMMING (70)                
                        
#define ACQ_HOSTSHIM (75)                
                        
#define ACQ_SMPCHANGE (80)                
                        
#define ACQ_RETRIEVSMP (81)                
                        
#define ACQ_LOADSMP (82)                
                        
#define ACQ_INTERACTIVE (90)                
                        
#define ACQ_TUNING (100)                
                        
#define ACQ_PROBETUNE (105)                
                        
#define LSDV_EJECT (0x40)                
                        
#define LSDV_DETECTED (0x4000)                
                        
#define LSDV_LKREGULATED (0x04)                
                        
#define LSDV_LKNONREG (0x08)                
                        
#define LSDV_LKMASK (0x0C)                
                        
#define CONSOLE_VNMRS_ID (0)                
                        
#define CONSOLE_400MR_ID (1)                
                        
#define CONSOLE_SILKVNMRS_ID (2)                
                        
#define CONSOLE_SILK400MR_ID (3)                
        
/* topic name form */

/* topic names form: h/rf1/dwnld/strm, rf1/h/dwnld/reply */
                
#define CNTLR_PUB_STAT_TOPIC_FORMAT_STR ("master/h/constat")                
                        
#define HOST_SUB_STAT_TOPIC_FORMAT_STR ("master/h/constat")                
                        
#define CNTLR_SUB_STAT_TOPIC_FORMAT_STR ("h/master/constat")                
                        
#define HOST_PUB_STAT_TOPIC_FORMAT_STR ("h/master/constat")                
        
/* download types */
                
#define DATA_FID (1)                
                        
#define MAX_IPv4_UDP_SIZE_BYTES (65535)                
                        
#define NDDS_MAX_Size_Serialize (64512)                
        
#ifndef MAX_SHIMS_CONFIGURED
                
#define MAX_SHIMS_CONFIGURED (48)                
        
#endif

#ifdef __cplusplus
extern "C" {
#endif

        
extern const char *Console_StatTYPENAME;
        

#ifdef __cplusplus
}
#endif

typedef struct Console_Stat
{
    DDS_Long  dataTypeID;
    DDS_Long  AcqCtCnt;
    DDS_Long  AcqFidCnt;
    DDS_Long  AcqSample;
    DDS_Long  AcqLockFreqAP;
    DDS_Long  AcqLockFreq1;
    DDS_Long  AcqLockFreq2;
    DDS_Long  AcqNpErr;
    DDS_Long  AcqSpinSet;
    DDS_Long  AcqSpinAct;
    DDS_Long  AcqSpinSpeedLimit;
    DDS_Long  AcqPneuBearing;
    DDS_Long  AcqPneuStatus;
    DDS_Long  AcqPneuVtAir;
    DDS_Long  AcqTickCountError;
    DDS_Long  AcqChannelBitsConfig1;
    DDS_Long  AcqChannelBitsConfig2;
    DDS_Long  AcqChannelBitsActive1;
    DDS_Long  AcqChannelBitsActive2;
    DDS_Short  AcqRcvrNpErr;
    DDS_Short  Acqstate;
    DDS_Short  AcqOpsComplCnt;
    DDS_Short  AcqLSDVbits;
    DDS_Short  AcqLockLevel;
    DDS_Short  AcqRecvGain;
    DDS_Short  AcqSpinSpan;
    DDS_Short  AcqSpinAdj;
    DDS_Short  AcqSpinMax;
    DDS_Short  AcqSpinActSp;
    DDS_Short  AcqSpinProfile;
    DDS_Short  AcqVTSet;
    DDS_Short  AcqVTAct;
    DDS_Short  AcqVTC;
    DDS_Short  AcqPneuVTAirLimits;
    DDS_Short  AcqPneuSpinner;
    DDS_Short  AcqLockGain;
    DDS_Short  AcqLockPower;
    DDS_Short  AcqLockPhase;
    DDS_Short  AcqShimValues[(MAX_SHIMS_CONFIGURED)];
    DDS_Short  AcqShimSet;
    DDS_Short  AcqOpsComplFlags;
    DDS_Short  rfMonError;
    DDS_Short  rfMonitor[8];
    DDS_Short  statblockRate;
    DDS_Short  gpaTuning[9];
    DDS_Short  gpaError;
    DDS_Char  probeId1[20];
    DDS_Char  gradCoilId[12];
    DDS_Short  consoleID;

} Console_Stat;
    
                            
#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, start exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport __declspec(dllexport)
#endif

    

DDS_SEQUENCE(Console_StatSeq, Console_Stat);
        
NDDSUSERDllExport
RTIBool Console_Stat_initialize(
        Console_Stat* self);
        
NDDSUSERDllExport
RTIBool Console_Stat_initialize_ex(
        Console_Stat* self,RTIBool allocatePointers);

NDDSUSERDllExport
void Console_Stat_finalize(
        Console_Stat* self);
                        
NDDSUSERDllExport
void Console_Stat_finalize_ex(
        Console_Stat* self,RTIBool deletePointers);
        
NDDSUSERDllExport
RTIBool Console_Stat_copy(
        Console_Stat* dst,
        const Console_Stat* src);

#if (defined(RTI_WIN32) || defined (RTI_WINCE)) && defined(NDDS_USER_DLL_EXPORT)
  /* If the code is building on Windows, stop exporting symbols.
   */
  #undef NDDSUSERDllExport
  #define NDDSUSERDllExport
#endif


#ifdef __cplusplus

    extern "C" {

#endif



extern void getConsole_StatInfo(NDDS_OBJ *pObj);



#ifdef __cplusplus

}

#endif


#endif /* Console_Stat_h */
