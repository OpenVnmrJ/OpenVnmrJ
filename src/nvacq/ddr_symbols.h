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
#ifndef DDR_SYMBOLS_H
#define DDR_SYMBOLS_H

// ------------- Make C header file C++ compliant -------------------

#ifdef __cplusplus
extern "C" {
#endif

#define BS_POST_COPY   // compilation switch back to old bs mode

#define DDR_DATA_SIZE     (0x04000000)       // 64 MBytes
#define DDR_ACQ_SIZE      (0x03000000)
//======================= NMR data info =====================

enum {
    DATA_FLOAT       = 0x00000000,   // float data buffer format
    DATA_SHORT       = 0x00000001,   // short data buffer format 
    DATA_DOUBLE      = 0x00000002,   // double data buffer format 
    DATA_PACK        = 0x00000004,   // pack return data as complex short
    DATA_FREQ        = 0x00000008,   // domain type (0=time,1=frequency)  
    DATA_TYPE        = 0x00000003    // data type mask
};

enum {
    FILTER_BLACKMAN  = 0x00000000,  // Blackman filter 
    FILTER_HAMMING   = 0x00000010,  // Hamming filter
    FILTER_NONE      = 0x00000020,  // decimation only 
    FILTER_BLCKH     = 0x00000030,  // Blackman - Harris filter
    FILTER_TYPE      = 0x00000030   // filter type mask
};

enum {
    PF_FIRST         = 0x00000000,  // first point only
    PF_QUAD          = 0x00000040,  // quadratic fit
    PF_TYPE          = 0x00000040   // prefid phase method mask
};

enum {
    AD_FILTER1       = 0x00000000,  // AD filter mode 1
    AD_FILTER2       = 0x00000080,  // AD filter mode 2
    AD_FILTER        = 0x00000080,  // AD filter mode mask
    AD_SKIP1         = 0x00000100,  // AD acq trigger auto-skip (min)
    AD_SKIP2         = 0x00000200,  // AD acq trigger auto-skip (mid)
    AD_SKIP3         = 0x00000300,  // AD acq trigger auto-skip (max)
    AD_SKIP          = 0x00000300,  // AD filter skip mask
    AD_TYPE          = 0x00000380   // AD filter mask
};

enum exp_mode_bits {
    MODE_DTYPE       = 0x0000000F,   // DATA_TYPE
    MODE_FILTER      = 0x00000030,   // FILTER_TYPE  
    MODE_PFMODE      = 0x00000040,   // PF_TYPE
    MODE_ADMODE      = 0x00000380,   // AD_TYPE
    MODE_NOCS        = 0x00000400,   // disable checksum calculation  
    MODE_SS          = 0x00000800,   // allocate snapshot buffer  
    MODE_ADC_DEBUG   = 0x00001000,   // fake ADC data (saw tooth)
    MODE_DITHER_OFF  = 0x00002000,   // disable ddr-a dithering
    MODE_SOFT_SYNC   = 0x00004000,   // soft-sync reset (test mode)
    MODE_FSHIFT      = 0x00008000,   // numerical frequency shift 
    MODE_HWPF        = 0x00010000,   // use NCO regs. for const freq&phase  
    MODE_PHASE_CONT  = 0x00020000,   // don't clear NCO accumulator (phase-continuous)  
    MODE_NOPC        = 0x00040000,   // no receiver phase cycling  
    MODE_NOPS        = 0x00080000,   // no phase synchronization 
    MODE_CWPR        = 0x00100000,   // clockwise rcvr phase rotation (Infinity) 
    MODE_FIDSIM      = 0x00200000,   // FID simulation mode 
    MODE_CFLOAT      = 0x00400000,   // use floating point filter coeffs. 
    MODE_DMA         = 0x00800000,   // use dma for data transfers 
    MODE_PACKED      = 0x01000000,   // packed data ADC FIFO xfer 
    MODE_L1          = 0x02000000,   // half-integer decimation (stage 1) 
    MODE_L2          = 0x04000000,   // half-integer decimation (stage 2)
    MODE_ATAQ        = 0x08000000,   // constrain aqtm to al*dwell 
    MODE_ATAQ_DR     = 0x10000000,   // reflect missing data in ataq mode 
    MODE_ADC_OVF     = 0x20000000    // enable new adc ovf warning for each fid 
};

enum exp_info_bits {
    EXP_DTYPE       = 0x00000003,   // acquisition data type  
    EXP_READY       = 0x00000004,   // ddr ready 
    EXP_INIT        = 0x00000008,   // experiment initialized 
    EXP_REBUILD     = 0x00000010,   // new filter needed flag 
    EXP_NEWSIM      = 0x00000020,   // changed simfid flag 
    EXP_END_SCAN    = 0x00000040,   // end of scan flag 
    EXP_IN_SCAN     = 0x00000080,   // in scan flag 
    EXP_LAST_SCAN   = 0x00000100,   // last scan flag 
    EXP_RUNNING     = 0x00000200,   // exp status (1=running 0=stopped) 
    EXP_TEST        = 0x00000400,   // test ready  
    EXP_SS          = 0x00000800,   // snapshot buffer available
    EXP_WACQ        = 0x00001000,   // set MODE_WACQ scan bit
    EXP_WIEN        = 0x00002000,   // set MODE_WIEN scan bit
    EXP_DATA        = 0x00004000,   // all data transferred flag
    EXP_ERROR       = 0x00008000,   // fatal error 
    EXP_COMPLETED   = 0x00010000,   // exp complete flag 
    EXP_CLR_DATA    = 0x00020000,   // clear data buffer
    EXP_NEWFID      = 0x00040000    // new fid flag
};

// accumulator status bits

enum {
    ACM_DTYPE   	= 0x00000003,   // data type mask
    ACM_STATUS  	= 0x000000F0,   // status mask
    ACM_LOCKED  	= 0x00000010,   // buffer is locked by host 
    ACM_LAST    	= 0x00000020,   // last scan flag
    ACM_LOCK_ERR 	= 0x00000080,   // write to unlocked buffer
    ACM_CLIP_ERR 	= 0x00000100,   // fid data was clipped
    ACM_SS      	= 0x00000200    // snap-shot id 
};

enum SS_MODE {
    SS_ACM,     // accumulation buffer snapshot 
    SS_ACQ      // acquisition buffer snapshot 
};  

enum {
    STOP_END,     // end of experiment (normal) 
    STOP_USER,    // user stop request 
    STOP_ERROR    // stop on fatal error 
};  

enum {  // C67 test options
    DDR_TEST_ACTIVE,    // watchdog timer test 
    DDR_TEST_FIFO,    	// data fifo read test
    DDR_TEST_DATA,    	// data message test 
    DDR_TEST_BRAM,    	// fpga bram test 
    DDR_TEST_LIMITS    	// data min max test
};  

enum {  // debug output codes
    DDR_DEBUG_HMSGS = 0x01,    	// show ddr->host messages 
    DDR_DEBUG_DMSGS = 0x02,    	// show host->ddr messages 
    DDR_DEBUG_DATA  = 0x04,    	// show ddr->host data messages 
    DDR_DEBUG_SCANS = 0x08,    	// show scan messages 
    DDR_DEBUG_INTS  = 0x10,    	// show interrupt calls 
    DDR_DEBUG_ERRS  = 0x20,     // show error messages 
    DDR_DEBUG_MPOST = 0x40,     // show msg post activity 
    DDR_DEBUG_XFER  = 0x80      // test data transfer 
};  

enum {
    INIT_MSGS = 0x01,     		// end of exp msgs 
    DATA_MSGS = 0x02,     		// end of exp msgs 
    RUN_MSGS  = 0x04,     		// end of exp msgs 
    END_MSGS  = 0x08,     		// end of exp msgs 
    ERR_MSGS  = 0x10,     		// user stop request 
    ALL_MSGS  = 0xff     		// all messages 
};

enum {  // debug mode codes
    DDR_NONE    = 0x00,        // clear all debug modes
    DDR_HMSGS   = 0x01,        // set host message enable mask
    DDR_BLINK   = 0x02,        // start/stop led blinker
    DDR_AUTOP   = 0x04         // auto-parse
};  

#define MSG_VALID 0x10000000   // set in id by msg sender cleared after read

// opcodes for host->ddr messages
enum HOST_TO_ACQ_MSGS 
{
    // -------------------------------------------------------------------
    // opcode          description                     arg1    arg2    arg3 
    // -------------------------------------------------------------------
    DDR_NOINIT,     // 0 uninitialized message         ---     ---     ---  
    DDR_INIT,       // 1 acknowledge active ddr        ---     ---     --- 
             
    // experiment initialization                                            

    DDR_SET_MODE,   // 2  set mode options             MODE    BKP1    BKP2
    DDR_SET_DIMS,   // 3  set accumulation buffers     NACMS   DL      AL
    DDR_SET_XP,     // 4  set transfer policy          NF      NFMOD   ASCALE     
    DDR_SET_SR,     // 5  set sampling rate            SR      ---     ---     
    DDR_SET_SD,     // 6  set sampling delay           SD      ---     ---     
    DDR_SET_SW,     // 7  set filter window width      SW      stage   ---     
    DDR_SET_OS,     // 8  set filter window ovs        OS      stage   ---     
    DDR_SET_XS,     // 9  set filter window bc         XS      stage   ---     
    DDR_SET_FW,     // 10 set filter width factor      FW      stage   ---     
    DDR_SET_CP,     // 11 set constant phase  p        ---     ---     
    DDR_SET_CA,     // 12 set constant ampl scale      a       ---     ---     
    DDR_SET_CF,     // 13 set constant freq            f       ---     ---     
              
    // experiment control 
    
    DDR_INIT_EXP,   // 14  initialize experiment       nd      ---     ---     
    DDR_START_EXP,  // 15 start experiment             mode    ---     ---     
    DDR_RESUME_EXP, // 16 resume experiment            ---     ---     ---
    DDR_STOP_EXP,   // 17 stop after N scans           N       ---     ---     

    // fid frame commands                                                  

    DDR_PUSH_FID,   // 18 push fid frame               ---     ---     --- 
    
    // scan frame commands                                                  
    
    DDR_SET_ACM,    // 19 set accum buffer index       src     dst     nt     
    DDR_SET_RP,     // 20 set receiver phase           p       ---     ---     
    DDR_SET_RF,     // 21 set receiver freq            f       ---     ---     
 
    DDR_PUSH_SCAN,  // 22 push scan frame              src     flags   cf      
        
    // data transfer control                                                

    DDR_SS_REQ,     // 23 snap shot request            mode     ---     ---    
    DDR_UNLOCK,     // 24 free data buffer             acm      opt     ---    

    // misc. commands                                                       
    
    DDR_DEBUG,      // 25 set debug mode               id      arg1    arg2    
    DDR_TEST,       // 26 test function 1              id      arg1    arg2    
    DDR_VERSION,    // 27 get version and checksums    ---     ---     --- 
    DDR_SETFID,     // 28 set fid function             arg1    arg2    arg3    
    DDR_PFAVES,     // 29 set prefid averages          aves    ---     ---    
    DDR_XIN_INFO,   // 30 request xin info             ---     ---     ---    

    // spectrum simulation

    DDR_SIM_PEAKS,  // 31 simulated fid peaks          npks    ---     ---    
    DDR_SIM_FSCALE, // 32 simulated fid sw             fscale  ---     ---    
    DDR_SIM_ASCALE, // 33 simulated fid sw             Ascale  ---     ---    
    DDR_SIM_RDLY,   // 34 simulated fid rd             rdly    ---     ---    
    DDR_SIM_NOISE,  // 35 simulated fid noise          noise   ---     ---    
    DDR_SIM_LW,     // 36 simulated fid lw             lw      ---     ---    
    DDR_SIM_CYCLE,  // 37 simulated duty cycle         on      off     ---    

    DDR_PEAK_FREQ,  // 38 simulated peak frequency     id      freq    ---
    DDR_PEAK_AMPL,  // 39 simulated peak ampl          id      ampl    ---
    DDR_PEAK_WIDTH  // 40 simulated peak lw            id      width   ---   
};

// opcodes for ddr->host messages 

enum DDR_TO_HOST_MSGS 
{
    // -------------------------------------------------------------------
    // opcode          description                  arg1    arg2    arg3    
    //--------------------------------------------------------------------- 
    HOST_NOINIT,     // 0  unitialized message         index   ---     ---     
    
    // DDR command acknowledgments
     
    HOST_INIT,       // 1  DDR_INIT                    status  ---     ---     
    HOST_RESET,      // 2  DDR_RESET                   status  ---     ---     
    HOST_INIT_EXP,   // 3  DDR_INIT_EXP                status  ---     ---     
    HOST_START_EXP,  // 4  DDR_START                   status  ---     --- 
    HOST_STOP_EXP,   // 5  DDR_STOP                    status  ---     ---     
    HOST_RESUME_EXP, // 6  DDR_RESUME                  status  ---     ---     
    HOST_SS_REQ,     // 7  DDR_SS_REQ                  id      cs      ct      
    
     // asynchronous messages 
                                                               
    HOST_DATA,       // 8  data ready                  acm     id      cs  
    HOST_END_DATA,   // 9  data ready (last fid)       acm     id      cs  
    HOST_END_SCAN,   // 10 end of scan                 acm     ct      flags
    HOST_END_EXP,    // 11 end of exp                  cpu     tm      tt
    HOST_DATA_INFO,  // 12 additional data info        tt      scale   ---
   
    // misc. commands
    
    HOST_XIN_INFO,   // 13 return xin info             X       B        N  
    HOST_ERROR,      // 14 send error message          id      [opt]   [opt]   
    HOST_TEST,       // 15 test function 1 reply       id      arg1    arg2    
    HOST_VERSION     // 16 version and checksums       rev     ddr.h   fpga    
};

//=======================  ddr error codes =================================
//                           arg1.i    arg2.i    arg3.i
#define DDR_ERROR_INIT          1   //
#define DDR_ERROR_INIT_STR      "<C67> DSP not initialized"

#define DDR_ERROR_ACQ           2   // acm.tt            
#define DDR_ERROR_ACQ_STR       "<C67> acquisition error"

#define DDR_ERROR_SCAN_QUE1     3  //  id     
#define DDR_ERROR_SCAN_QUE1_STR "<C67> scan queue failure [push]"

#define DDR_ERROR_SCAN_QUE2     4  //  id     
#define DDR_ERROR_SCAN_QUE2_STR "<C67> scan queue failure [pop]"

#define DDR_ERROR_FID_QUE       5  //  id     
#define DDR_ERROR_FID_QUE_STR   "<C67> fid queue failure"

#define DDR_ERROR_DATA_QUE      6  //  acm.tt            
#define DDR_ERROR_DATA_QUE_STR  "<C67> data queue failure"

#define DDR_ERROR_NP            7  //  acm      acm.nt    
#define DDR_ERROR_NP_STR        "<C67> scan overrun (repetition rate too high)"

#define DDR_ERROR_LOCK          8  //  acm      acm.nt  
#define DDR_ERROR_LOCK_STR      "<C67> locked buffer overwrite"

#define DDR_ERROR_DCLIP         9  //  id (warning)
#define DDR_ERROR_DCLIP_STR     "<C67> NMR signal clipped"

#define DDR_ERROR_MAXD          10  //  id (warning)
#define DDR_ERROR_MAXD_STR      "<C67> max data value exceeded in averaging"

#define DDR_ERROR_ADF_OVF       11  //  id
#define DDR_ERROR_ADF_OVF_STR   "<C67> DATA FIFO full"

#define DDR_ERROR_BADID         12  //  id
#define DDR_ERROR_BADID_STR     "<C67> fpga version incompatible with DSP os"

#define DDR_ERROR_MEM           13  //  mem error code
#define DDR_ERROR_MEM_STR       "<C67> out of memory"

#define DDR_ERROR_PROC          14  
#define DDR_ERROR_PROC_STR      "<C67> post-processing error"

#define DDR_ERROR_SS            15 
#define DDR_ERROR_SS_STR        "<C67> snapshot request (no buffer allocated)"

#define DDR_ERROR_DMQ_C67       16
#define DDR_ERROR_DMQ_C67_STR   "<C67> unknown MSG id"

#define DDR_ERROR_DMSG          17  //  id
#define DDR_ERROR_DMSG_STR      "<405> C67->405 MSG queue full"

#define DDR_ERROR_HMSG          18  //  id
#define DDR_ERROR_HMSG_STR      "<405> 405->C67 MSG queue full"

#define DDR_ERROR_OVF           19  
#define DDR_ERROR_OVF_STR       "<405> DDR output fifo full"

#define DDR_ERROR_UNF           20  
#define DDR_ERROR_UNF_STR       "<405> DDR output fifo empty"

#define DDR_ERROR_SW            21  
#define DDR_ERROR_SW_STR        "<405> unexpected software behavior trap"

#define DDR_ERROR_HPI           22  
#define DDR_ERROR_HPI_STR       "<405> HPI timeout"


// memory malloc error codes
enum {
   MEM_ERR_BMEM = 1,    // BIOS structures (scan queue etc)
   MEM_ERR_IMEM = 2,    // acm buffer structure array
   MEM_ERR_DMEM = 3,    // exp. data buffers
   MEM_ERR_CMEM = 4,    // filter coefficient vectors 
   MEM_ERR_AMEM = 5,    // acquisition buffer 
   MEM_ERR_SMEM = 6     // simulated FID buffer
};

typedef union u32 
{
    float   f;
    int     i;
} u32;

typedef struct i64 
{
    unsigned int h;
    unsigned int l;
} i64;

typedef union u64 
{
    double   d;
    i64      i;
} u64;

typedef union siu {
  unsigned int    i;
  short  s[2];
} siu;

typedef struct data_msg {
    unsigned int  src;       // acm id
    unsigned int  adrs;      // EMIF address of data 
    unsigned int  np;        // number of points 
    unsigned int  id;        // user id  
    unsigned int  cs;        // checksum 
    unsigned int  status;    // status bits (data
} data_msg;

typedef struct scan_msg 
{
    int         status;   // status code (EXP_LAST_SCAN)
    int         acm;      // id of scan buffer 
    int         ct;       // scan count in buffer
} scan_msg;

#ifdef __cplusplus
}
#endif
#endif
