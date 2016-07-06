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

// #define TIMING_DIAG_ON    /* compile in timing diagnostics */

#define SEND_APARSE_EARLY   // uncomment or comment both defines in AParser.c & A32Interp.c

#include <stdio.h>
#ifndef VXWORKS
  #include <stdlib.h>
#else
  #include <memLib.h>
#endif
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#undef V7
#include "ACode32.h"
#include "FFKEYS.h"
#include "errno.h"
#include "errorcodes.h"     /* from Vnmr */
#include "expDoneCodes.h"
#include "Lock_Cmd.h"
#include "cntlrStates.h"
#include "gradient_fifo.h"
#include "nvhardware.h"
#include "instrWvDefines.h"

#ifdef VXWORKS
#ifdef MASTER_CNTLR
   #include "Console_Stat.h"
   #include "Cntlr_Comm.h"
   #include "masterAux.h"
#endif

   #include "logMsgLib.h"
   #include "AParser.h"
   #include "dataObj.h"
   #include "A32BridgeFuncs.h"
   #include "Cntlr_Comm.h"
   #include "nexus.h"
   #include "sysUtils.h"

#else
   #include "FrontEnd.h"
#endif
#include "lc.h"
#include "ddr_symbols.h"
#include "upLink.h"

double unpackd(int *buffer,int *index);

extern MSG_Q_ID pDataTagMsgQ;	    /* dspDataXfer task  -> dataPublisher task */
extern int warningAsserted;
extern int readuserbyte;

void SendRollCallViaParsCom();
void Send2LockViaParsCom(int Lk_Cmd, long value, long value2, double arg3, double arg4 ) ;
void send2AllCntlrsViaParsCom(int parse_cmd,  long num_acodes,  long num_tables, long cur_acode,  char *Id);
void sendExceptionViaParsCom(int error, int errorcode,  int d1, int d2, char *str);

#define ONE_TIME 1
#define NO_REMAINDER 0
#define START_FIFO_ON_PEND_YES 1
#define START_FIFO_ON_PEND_NO 0

#ifdef  DURATION_DECODE
extern int decodeFifoWordsFlag;
#endif

// if below send as fifo word not pattern to avoid small dma blocks
// dma blocks are more efficient so use them aggressively... 
int PATTERN_LOW_LIMIT = 2500;
int DecThreshold = 200;

#define COMPILEKEY (0)

#ifdef MASTER_CNTLR
  extern Console_Stat	*pCurrentStatBlock;
  struct _nsr {
	int	word1;
	int	word2;
	int	rcvrgain;
	int	lkattn;
	int	lkhi_lo;
   };
   struct _nsr 	nsr;
   int        nsrMixerBand[10];
   int        XYZgradflag;
   extern int auxLockByte;
#endif

#ifdef DDR_CNTLR
#include <rngLib.h>
extern RING_ID  pSyncActionArgs;
#endif

int codesOK;
int mripricnt = 50;   /* for use use MRIUSERBYTE & priority change */
int il_flag = 0;
int zgradcorrhi = 0;    /* gradient correction for L350 deadband */
int zgradcorrlo = 0;    /* gradient correction for L350 deadband */

extern int residentFlag;
extern int BrdType;   /* board type */
extern int AbortingParserFlag;
extern int failAsserted;

extern DATAOBJ_ID pTheDataObject; /* FID statblock, etc. */
extern ACODE_ID  pTheAcodeObject; /* Acode object */

extern int *pGradWfg;             /* malloc-ed in AParser.c */

extern int prepflag;		  /* imaging prep flag */

int icatFail=-1;

long long xgateCount = 0LL;       /* counter for total number of xgate executions */
int wvlogAcodes = 0;		  /* flag for wvEvent logging of each acode parsed */
int dmpRtVarsFlg = 0;

unsigned int *p2CurrentWord;
unsigned int *p2EndOfCodes;
int *p2ReferenceLocation = 0;

static unsigned int *p2CurrentWordStart;
#define MAX_FIDSHIMNT 256       /* max value of scans to average for fidshim */

/* Tables */
#define TABLEHEADER     0x40000000
#define TABLE_HDR_SIZE  8
#define MAXTABLES       60

/* array of pointers to hold addrs of inline t1-60 tables */
unsigned int *p2TableLocation[MAXTABLES];
int TableSizes[MAXTABLES];

// amplifier corrrection

#ifdef RF_LinearizationControl

volatile unsigned int *fpga_linearization_enable = (volatile unsigned int *)(FPGA_BASE_ADR + RF_LinearizationControl);
volatile unsigned int *fpga_atten_mapping = (volatile unsigned int *)(FPGA_BASE_ADR + RF_AttenMappingTable);
volatile unsigned int *fpga_amp_table = (volatile unsigned int *)(FPGA_BASE_ADR + RF_AmpLinearizationTable);
volatile unsigned int *fpga_phase_table = (volatile unsigned int *)(FPGA_BASE_ADR + RF_PhaseLinearizationTable);
volatile unsigned int *fpga_tpwr_map_tbl = (volatile unsigned int *)(FPGA_BASE_ADR + RF_TPowerMappingTable);
volatile unsigned int *fpga_tpwr_scale_tbl = (volatile unsigned int *)(FPGA_BASE_ADR + RF_LinearizationScaleTable);

#else // for testing
unsigned static volatile int   fpga_ampreg=0;
unsigned static volatile int   fpga_phsreg=0;
#endif

int use_tbls = 0;
int max_tpwr = 63;
int min_tpwr = 31;
int num_tbls = 0;
int tbl_size = 0;
int tbl_dflt = 2;

/* the rt var system */
static Acqparams *pAcqReferenceData;
static int *rtVar = NULL; /* (int *) &(acqReferenceData);  */
int initvalFlag = 1;
// just for prints but needs to match lc
char *rt_descr[] = { "np","nt","ct","dpts","autop","il_incr","il_incrBsCnt",
     /* 7 - 12 */    "ra_fidnum","ra_ctnum","rtvptr","idver","o2auto","dpf",
     /* 13 - 20 */   "oph","bs","bsct","ss","ssct","cpf","cfct","tablert",
     /* 21 - 28 */   "rttmp","spare1","id2","id3","id4","zero","one","two",
                     "three","v1","v2","v3","v4","v5","v6","v7","v8","v9",
                     "v10","v11","v12","v13","v14","v15","v16","v17","v18","v19",
                     "v20","v21","v22","v23","v24","v25","v26","v27","v28","v29",
                     "v30","v31","v32","v33","v34","v35","v36","v37","v38","v39",
                     "v40","v41","v42","hdec_cntr","hdec_lcnt","private1","private2",
                     "rtonce","private4","private5","private6","private7","private8" };

void
askRTId(int i)
{
  printf("rt[%d] is id'd as %s\n",i,rt_descr[i]);
}

void prtRTval(int rtindex)
{
 printf("%s = %d\n",rt_descr[rtindex],  rtVar[rtindex]);
}

void dumpRtVars()
{
  // 0 - 29 = np - three
  // 30 - 71 = v1-v42
  // 72 - 81 = hdec_cntr - private8

  int i;
  printf("%s = %d, %s = %d,  %s = %d,  %s = %d,  %s = %d,  %s = %d,  %s = %d\n", 
         rt_descr[0], rtVar[0],   // np
         rt_descr[1], rtVar[1],   // nt
         rt_descr[2], rtVar[2],   // ct
         rt_descr[14], rtVar[14],   // bs
         rt_descr[15], rtVar[15],   // bsct
         rt_descr[16], rtVar[16],   // ss
         rt_descr[17], rtVar[17]   // ssct
       );

  printf("%s = %d, %s = %d,  %s = %d,  %s = %d,  %s = %d,  %s = %d,  %s = %d\n", 
         rt_descr[5], rtVar[5],   // il_incr
         rt_descr[6], rtVar[6],   // il_incrBsCnt
         rt_descr[13], rtVar[13],   // oph
         rt_descr[17], rtVar[18],   // cpf
         rt_descr[17], rtVar[17],   // cfct
         rt_descr[17], rtVar[17],   // tablert
         rt_descr[3], rtVar[3]   // dpts
        );

  i = 30;
  while (i < 71) // v1-v42
  {

    if ( (i+1) % 10  ==  0)
       printf("%s = %d\n",  rt_descr[i], rtVar[i]);
    else
       printf("%s = %d, ",  rt_descr[i], rtVar[i]);
    i++;
  }
  printf("\n");

  printf("%s = %d, %s = %d,  %s = %d,  %s = %d,  %s = %d,  %s = %d,  %s = %d\n", 
         rt_descr[4], rtVar[4],   // autop
         rt_descr[7], rtVar[7],   // ra_fidnum
         rt_descr[8], rtVar[8],   // ra_ctnum
         rt_descr[9], rtVar[9],   // rtvptr
         rt_descr[10], rtVar[10],   // idver
         rt_descr[11], rtVar[11],   // o2auto
         rt_descr[12], rtVar[12]   // dpf
       );

  printf("%s = %d, %s = %d,  %s = %d,  %s = %d,  %s = %d,  %s = %d,  %s = %d\n", 
         rt_descr[21], rtVar[21],   // rttmp
         rt_descr[22], rtVar[22],   // spare1
         rt_descr[23], rtVar[23],   // id2
         rt_descr[24], rtVar[24],   // id3
         rt_descr[25], rtVar[25],   // id4
         rt_descr[26], rtVar[26],   // zero
         rt_descr[27], rtVar[27]   // one
       );

  printf("%s = %d, %s = %d,  %s = %d,  %s = %d,  %s = %d\n",
         rt_descr[77], rtVar[77],   // pri 4
         rt_descr[78], rtVar[78],   // pri 5
         rt_descr[79], rtVar[79],   // pri 6
         rt_descr[80], rtVar[80],   // pri 7
         rt_descr[81], rtVar[81]   // pri 8
       );
  printf("\n");

}


/* Gradient  & PFG ontroller related */
#if defined(PFG_CNTLR) || defined(GRADIENT_CNTLR)
extern int XYZshims[];

/* standard rotation matrix */
volatile int rotm_11,rotm_12,rotm_13;
volatile int rotm_21,rotm_22,rotm_23;
volatile int rotm_31,rotm_32,rotm_33;

/* scaled   rotation matrix */
volatile int srotm_11,srotm_12,srotm_13;
volatile int srotm_21,srotm_22,srotm_23;
volatile int srotm_31,srotm_32,srotm_33;

volatile int d_srotm_11,d_srotm_12,d_srotm_13;
volatile int d_srotm_21,d_srotm_22,d_srotm_23;
volatile int d_srotm_31,d_srotm_32,d_srotm_33;

/* Phase Encode Variables */
int peR_step, peP_step, peS_step;
int peR_vind, peP_vind, peS_vind;
int peR_lim,  peP_lim,  peS_lim;

int d_peR_step, d_peP_step, d_peS_step;
int d_peR_vind, d_peP_vind, d_peS_vind;
int d_peR_lim,  d_peP_lim,  d_peS_lim;

int sim_gradient;

/* real-time gradient angle rotation */
double ang_psi, ang_phi, ang_theta;
int gxflip, gyflip, gzflip;

#endif

/*  DDR controller related NF parameters */
#ifdef DDR_CNTLR

/* NF variables, nf and cf */
int srctag, dsttag;
extern int fid_count;
extern int fid_ct;

#endif

char secretword[MAX_ROLLCALL_STRLEN] = "bogus not there";

/* a simple trick.. */
union {
  long long lword;
  double dword;
  int  nword[2];
 }  lscratch1, lscratch2, lscratch3;

/* for tn=Lk ew set lockpower=0 to stop lock pulser */
/* set a flag to remember we did                    */
/* and remember the true lockpower                  */
int tnlk_flag;
int tnlk_power;
void setTnEqualsLkState();

/* under development... */
void resetRTVars()
{
    pAcqReferenceData->v1 = 0;
    pAcqReferenceData->v2 = 0;
    pAcqReferenceData->v3 = 0;
    pAcqReferenceData->v4 = 0;
    pAcqReferenceData->v5 = 0;
    pAcqReferenceData->v6 = 0;
    pAcqReferenceData->v7 = 0;
    pAcqReferenceData->v8 = 0;
    pAcqReferenceData->v9 = 0;
    pAcqReferenceData->v10 = 0;
    pAcqReferenceData->v11 = 0;
    pAcqReferenceData->v12 = 0;
    pAcqReferenceData->v13 = 0;
    pAcqReferenceData->v14 = 0;
    pAcqReferenceData->v15 = 0;
    pAcqReferenceData->v16 = 0;
    pAcqReferenceData->v17 = 0;
    pAcqReferenceData->v18 = 0;
    pAcqReferenceData->v19 = 0;
    pAcqReferenceData->v20 = 0;
    pAcqReferenceData->oph = 0;
}

int getLcSS()
{
   return(pAcqReferenceData->ss);
}
int getLcSSCT()
{
   return(pAcqReferenceData->ssct);
}

int endofScan(int nt, int bs, int *p2ssct, int *p2ct, int *p2cbs, int fsval)
{
    int state, temp;
    state = -1;
    if (*p2ssct > 0)
    {
        (*p2ssct)--;
    }
    else
    {
        (*p2ct)++;

        if ((fsval > 0) && (fsval <= MAX_FIDSHIMNT))
        {
          if ((*p2ct) >= fsval) *p2ct = 0;
        }

        DPRINT2(+5, "endofScan(): ct: %lu , nt: %lu \n",*p2ct,nt);
        if ( *p2ct == nt)  /* we are done!! */   /* *p2ct */
            state = 2;
        if (*p2ct  > nt)    /* *p2ct */
            state = -3;
        if (*p2ct  < nt)   /* *p2ct */
        {
            state = 0;
            if ((bs > 0) && (*p2ct  % bs == 0))    /* *p2ct */
            {
                *p2cbs = *p2ct / bs;  /* update number of completed blocksize */
                if (il_flag != 1)  /* test for interleave acquisition */
                    state = 1;
		else
                    state = 2;   /* Il @ BS, go to next code set */
            }
        }
    }
    if (state == 2) {
        DPRINT(+1, "nt done - ddr would send data\n");
        return (state);
    }
    if (state == 1) {
        DPRINT1(+1, "bs done ct=%d - ddr would send data\n", pAcqReferenceData->ct  /* *p2ct */);
    }
    if (state == -1)
    {
        DPRINT(+1, "ss active... \n");
    }

    if (state == -3)
    {
    	errLogRet(LOGIT,debugInfo,
			"endofScan: State of -3 is an ERROR.\n");
        /* sendException(HARD_ERROR, HDWAREERROR + CNTLR_ISYNC_ERR, 0,0,NULL); */
    }

    p2CurrentWord = p2ReferenceLocation;    /* this is a JUMP */
    return (state);
}


#ifdef DDR_CNTLR

/* at blocksize, after sig. ave. C67 copies to next buffer
*  if nf=1 then for bs=1 src and dst tags go:
*  src dst
*   0   1    1st block
*   1   2    2nd block
*   2   3    3rd block
*   3   4    4th block
*   ....
*   ct  ct   if ct=nt, dst=src
*
* if nf > 1 allocation of tags jumps (e.g. nt=4, bs=1)
*  cf   0           1         2          3
*    src  dst   src  dst   src  dst   src  dst
*     0    4     1    5     2    6     3    7
*     4    8     5    9     6    10    7    11
*     8    12    9    13    10   14    11   15
*    12    12    13   13    14   14    15   15 <- last row dst=src
*
*/
int nextScan(ACODE_ID pAcodeId, int fidnum, int nf, int nfmod, int cf, int dsize, int *srctag, int *dsttag)
{
    int endct,ctmod;
    unsigned long scan_data_adr;
    FID_STAT_BLOCK *p2statb;
    int newblock=0;
    int nfidnum;
    int tagId;
    int ct,bs,nt,np,smax;

    ct=pAcqReferenceData->ct;
    bs=pAcqReferenceData->bs;
    nt=pAcqReferenceData->nt;
    np=pAcqReferenceData->np;

    smax=pTheDataObject->maxFidBlkBuffered;

    DPRINT5(0,"    nextScan: np: %d nt:%d ct:%d bs:%d fidnum:%d\n", np, nt, ct, bs,fidnum);
    DPRINT5(0,"    nextScan: cf: %d nf:%d ct:%d nt:%d nfmod:%d\n",cf,nf,ct,nt,nfmod);

    if (bs > 0)
        ctmod = bs;
    else
        ctmod = nt;

    nfidnum = cf/nfmod + (fidnum - 1) * nf/nfmod + 1;
    // DPRINT7(-7,"    nextScan: cf: %d / nfmod: %d = %d, nf: %d / nfmod: %d = %d  elemID: %d\n",cf,nfmod,(cf/nfmod), nf, nfmod, (nf/nfmod),nfidnum);
    DPRINT2(0,"    nextScan: fidnum: %d,  trace/echo#: %d \n",fidnum,nfidnum);

    // if blocksize or an nf then may need to alloc a new block
    DPRINT3(0,"    nextScan: ct:%d %% ctmod:%d = %d\n",ct,ctmod,ct%ctmod);
    if (ct % ctmod== 0)
    {
        if ((bs < 1) || ((nt-ct) < bs))
            endct = nt;
        else
            endct = ct + bs;

        // Only get new buffer if start of scan, blocksize or nfmod
        DPRINT3(0,"    nextScan: cf:%d %% nfmod:%d = %d\n",cf,nfmod,cf%nfmod);
        if ((cf % nfmod) == 0)
        {
            /* Now if dataAllocAcqBlk() going to block, we need to decide
             * if the experiment has not really been started, if not we
             * should start the FIFO now before we pend.
             * Otherwise everbody will just wait forever.
             */

            if ( allocDataWillBlock() == 1 ) {
                /* Probable only need to start the fifo if:
                   1. No Task is pended on pAcodeId->pSemParseSuspend
                      semaphore
                      Note : not an issue since the parser would not be
                      here if it was pended.
                   2. No has done the initial start of the fifo for
                      this Exp.
                 */
                DPRINT(+0, "    nextscan: about to block, starting FIFO\n");
#ifdef INSTRUMENT
                wvEvent(96,NULL,NULL);
#endif
                flushCntrlFifoRemainingWords();
                startFifo4Exp();
            }
             p2statb = (FID_STAT_BLOCK *) allocAcqDataBlock(
                (ulong_t) nfidnum,       // must be same id for all bs blks
                (ulong_t) np,            // adjusted for last nfmod
                (ulong_t) ct,
                (ulong_t) endct,
                (ulong_t) nt,
                (ulong_t) dsize,         // adjusted for last nfmod
                (long *) &tagId,         // no longer used
                (long *) &scan_data_adr  // no longer used
                );
            newblock = 1; // allocated new block

            // if new FID, or interleave set the source buf equal to tag
            if ((cf==0 && ct==0) || (il_flag == 1))
            {
                pAcodeId->dspSrcBuf = pAcodeId->tag2snd = tagId;
            }

            DPRINT6(+2,"    nextScan: FID_STAT_BLOCK: 0x%lx fidnum:%ld trace:%ld tagId:%d dsize:%d maxid:%d\n",
                                p2statb,fidnum,nfidnum,tagId,dsize,smax-1);
            scan_data_adr = nfidnum;
        }

        //  code move into if/else to handle nfmod interleave bug 7760
        if (il_flag != 1)
        {
           *srctag = pAcodeId->dspSrcBuf+cf/nfmod+(ct/ctmod)*nf/nfmod;
           DPRINT6(+5,"    nextScan: fidnum: %d, srctag %d = dspSrcBuf %d + (cf/nfmod) %d + (ct/ctmod) %d * (nf/nfmod) %d\n",
		              fidnum, *srctag, pAcodeId->dspSrcBuf, (cf/nfmod), (ct/ctmod), (nf/nfmod));
        }
        else
        {
           *srctag = pAcodeId->dspSrcBuf;
           DPRINT3(+5,"    nextScan: fidnum: %d, srctag %d = dspSrcBuf %d \n",
		              fidnum, *srctag, pAcodeId->dspSrcBuf);
        }
    }
    else // within bs averaging
    {
        // additions to handle nfmod interleave bug 7760  GMB
        if (il_flag != 1)
        {
           *srctag = pAcodeId->dspSrcBuf+cf/nfmod+(ct/ctmod)*nf/nfmod;
           DPRINT6(5,"    nextScan: fidnum: %d, srctag %d = dspSrcBuf %d + (cf/nfmod) %d + (ct/ctmod) %d * (nf/nfmod) %d\n",
		              fidnum, *srctag, pAcodeId->dspSrcBuf, (cf/nfmod), (ct/ctmod), (nf/nfmod));
        }
        else
        {
          if (nfmod == nf)
          {
             *srctag = pAcodeId->dspSrcBuf;
             DPRINT3(5,"    nextScan: fidnum: %d, srctag %d = dspSrcBuf %d \n",
		              fidnum, *srctag, pAcodeId->dspSrcBuf);
          }
          else // if (nfmod < nf ) 
          {
           // cf = 0 through (nf-1)
            int traceNum = cf/nfmod;
            // nfidnum = cf/nfmod + (fidnum - 1) * nf/nfmod + 1;
            // nfidnum // trace/echo we are on, nf/fnmod = max traces/echos
            // tracesPerFid = nf/nfmod;
            // cf  nfmod  ct ctmod
            *srctag = pAcodeId->dspSrcBuf - ((nf/nfmod) - 1) + traceNum;
           DPRINT5(5,"    nextScan: fidnum: %d, srctag %d = dspSrcBuf %d - (tracesPerFid - 1) %d + trace#  %d\n",
		              fidnum, *srctag, pAcodeId->dspSrcBuf, ((nf/nfmod) - 1), traceNum );
          }
        }
    }

//  Origin code now moved into if/else above to fix the nfmod interleave bug 7760
//    if (il_flag != 1)
//       *srctag = pAcodeId->dspSrcBuf+cf/nfmod+(ct/ctmod)*nf/nfmod;
//    else
//       *srctag = pAcodeId->dspSrcBuf;
//
//
//    DPRINT6(5,"fidnum: %d, srctag %d = dspSrcBuf %d + (cf/nfmod) %d + (ct/ctmod) %d * (nf/nfmod) %d\n",
//		fidnum, *srctag, pAcodeId->dspSrcBuf, (cf/nfmod), (ct/ctmod), (nf/nfmod));


    if(*srctag>=smax)
        *srctag %= smax;

    if (ctmod == bs){
        if ((ct % ctmod) == (ctmod-1) && pAcqReferenceData->ct < (nt-1)){
            *dsttag = *srctag+nf/nfmod;
            if(*dsttag >= smax)
                *dsttag %= smax;
        }
        else
            *dsttag = *srctag;
    }
    else
        *dsttag = *srctag;

    DPRINT5(1,"    nextScan: fid:%d cf/nfmod:%d ct/ctmod:%d src:%d dst:%d\n",
         nfidnum,cf/nfmod,ct/ctmod,*srctag,*dsttag);

    return (newblock);
}
#endif  // DDR_CNTLR

void RT1op(int opcode, int rtindex)
{
    switch (opcode) {
        case CLR:
        rtVar[rtindex] = 0;
        break;
    case INC:
        rtVar[rtindex]++;
        break;
    case DEC:
        rtVar[rtindex]--;
        break;
    default:
    	errLogRet(LOGIT,debugInfo,
			"RT1op: Math Option of %d is an Invalid Option.\n",opcode);
    }
}

void RT2op(int opcode, int rtsource, int rtdest)
{
    int temp;
    temp = rtVar[rtsource];
    switch (opcode) {
    case SET:
        break;
    case MOD2:
        temp %= 2;
        break;
    case MOD4:
        temp %= 4;
        break;
    case HLV:
        temp /= 2;
        break;
    case DBL:
        temp *= 2;
        break;
    case NOT:
        temp = ~temp;
        break;
    case NEG:
        temp = -temp;
        break;
    default:
    	errLogRet(LOGIT,debugInfo,
			"RT2op: Math Option of %d is an Invalid Option.\n",opcode);
    }
    rtVar[rtdest] = temp;
}

void RT3op(int opcode, int rtsource1, int rtsource2 , int rtdest)
{
    int temp, arg1, arg2;
    arg1 = rtVar[rtsource1];
    arg2 = rtVar[rtsource2];
    switch (opcode) {
    case ADD:
        temp = arg1 + arg2;
        break;
    case SUB:
        temp = arg1 - arg2;
        break;
    case MUL:
        temp = arg1 * arg2;
        break;
    case DIV:
        if (arg2 == 0)
            complain();
        temp = arg1 / arg2;
        break;
    case MOD:
        if (arg2 < 1)
            complain();
        temp = arg1 % arg2;
        break;
    case OR:
        temp = arg1 | arg2;
        break;
    case AND:
        temp = arg1 & arg2;
        break;
    case XOR:
        temp = arg1 ^ arg2;
        break;
    case LSL:
        temp = arg1 << arg2;
        break;
    case LSR:
        temp = arg1 >> arg2;
        break;
    case LT:
        temp = arg1 < arg2;
        break;
    case GT:
        temp = arg1 > arg2;
        break;
    case GE:
        temp = arg1 >= arg2;
        break;
    case LE:
        temp = arg1 <= arg2;
        break;
    case EQ:
        temp = arg1 == arg2;
        break;
    case NE:
        temp = arg1 != arg2;
        break;
    default:
    	errLogRet(LOGIT,debugInfo,
			"RT3op: Math Option of %d is an Invalid Option.\n",opcode);
    }
    rtVar[rtdest] = temp;
}


/*
   get table element values for TASSIGN acode
   table_index = 0-59  for tables t1-t60
*/

int getTableElement(int table_index, int element, int *errorcode, char *emssg)
{
   unsigned int *tableList, *p2TableWord;
   int tableSize,autoIncrFlag,divNFactor,elemSize;
   int index;
   int i;

   *errorcode = 0;

   if ( (table_index < 0) || (table_index >= MAXTABLES) )
   {
      errLogRet(LOGIT,debugInfo,"getP2TableElement: invalid table index %d\n", table_index);
      *errorcode = SYSTEMERROR+NOACODETBL;
      return(0);
   }

   p2TableWord = p2TableLocation[table_index];       /* table t1 is in array index 0 */

   if ( p2TableWord == NULL )
   {
     errLogRet(LOGIT,debugInfo,"TABLE: table not defined\n");
     *errorcode = SYSTEMERROR+NOACODETBL;
     return(0);
   }

   if ( (*(p2TableWord+1) & 0xF0000000 ) != TABLEHEADER )
   {
     errLogRet(LOGIT,debugInfo,"TABLE: invalid header in table\n");
     *errorcode = SYSTEMERROR+NOACODETBL;
     return(0);
   }

   if ( (*(p2TableWord+1) &  0xFFFFFFF ) != table_index )
   {
     errLogRet(LOGIT,debugInfo,"TABLE: invalid index for table\n");
     *errorcode = SYSTEMERROR+NOACODETBL;
     return(0);
   }

   if ( *(p2TableWord+7) != TableSizes[table_index] )
   {
     errLogRet(LOGIT,debugInfo,"TABLE: invalid size for table\n");
     *errorcode = SYSTEMERROR+NOACODETBL;
     return(0);
   }

   if (element < 0) element = 0;

   tableSize    = *(p2TableWord + 2);
   autoIncrFlag = *(p2TableWord + 3);
   divNFactor   = *(p2TableWord + 4);
   elemSize     = *(p2TableWord + 6);

   /* tableList points to the first table value */
   tableList    = p2TableWord + TABLE_HDR_SIZE ;

   if ( !autoIncrFlag)
   {
      index = element % tableSize;
   }
   else
   {
      /* an autoincrement table does divs then mods and increments. */
      index = *(p2TableWord + 5);
      ( *(p2TableWord+5) )++ ;     /* auto increment the index */
      index /= divNFactor;
      index /= tableSize;
   }

   if (elemSize == 32)
      return(tableList[index]);
   if (elemSize == 1)
   {
      int val;
      val = tableList[index/32];
      index %= 32;
      if ( val & (1 << index))
         return(1);
      else
         return(0);
   }
}

void putTableElement(int table_index, int element, int val, int *errorcode, char *emssg)
{
   unsigned int *tableList, *p2TableWord;
   int tableSize,autoIncrFlag,divNFactor,elemSize;
   int index;
   int i;

   *errorcode = 0;

   if ( (table_index < 0) || (table_index >= MAXTABLES) )
   {
      errLogRet(LOGIT,debugInfo,"getP2TableElement: invalid table index %d\n", table_index);
      *errorcode = SYSTEMERROR+NOACODETBL;
      return;
   }

   p2TableWord = p2TableLocation[table_index];       /* table t1 is in array index 0 */

   if ( p2TableWord == NULL )
   {
     errLogRet(LOGIT,debugInfo,"TABLE: table not defined\n");
     *errorcode = SYSTEMERROR+NOACODETBL;
     return;
   }

   if ( (*(p2TableWord+1) & 0xF0000000 ) != TABLEHEADER )
   {
     errLogRet(LOGIT,debugInfo,"TABLE: invalid header in table\n");
     *errorcode = SYSTEMERROR+NOACODETBL;
     return;
   }

   if ( (*(p2TableWord+1) &  0xFFFFFFF ) != table_index )
   {
     errLogRet(LOGIT,debugInfo,"TABLE: invalid index for table\n");
     *errorcode = SYSTEMERROR+NOACODETBL;
     return;
   }

   if ( *(p2TableWord+7) != TableSizes[table_index] )
   {
     errLogRet(LOGIT,debugInfo,"TABLE: invalid size for table\n");
     *errorcode = SYSTEMERROR+NOACODETBL;
     return;
   }

   if (element < 0) element = 0;

   tableSize    = *(p2TableWord + 2);
   autoIncrFlag = *(p2TableWord + 3);
   divNFactor   = *(p2TableWord + 4);
   elemSize     = *(p2TableWord + 6);

   if ( elemSize != 32 )
   {
     errLogRet(LOGIT,debugInfo,"TABLE: invalid elemsize for table\n");
     *errorcode = SYSTEMERROR+NOACODETBL;
     return;
   }
   /* tableList points to the first table value */
   tableList    = p2TableWord + TABLE_HDR_SIZE ;

   if ( !autoIncrFlag)
   {
      index = element % tableSize;
   }
   else
   {
      /* an autoincrement table does divs then mods and increments. */
      index = *(p2TableWord + 5);
      ( *(p2TableWord+5) )++ ;     /* auto increment the index */
      index /= divNFactor;
      index /= tableSize;
   }
   tableList[index] = val;
}

int *getP2Table(int table_index, int *errorcode, char *emssg)
{
   unsigned int *p2TableWord;

   *errorcode = 0;

   if ( (table_index < 0) || (table_index >= MAXTABLES) )
   {
      errLogRet(LOGIT,debugInfo,"getP2TableElement: invalid table index %d\n", table_index);
      *errorcode = SYSTEMERROR+NOACODETBL;
      return(NULL);
   }

   p2TableWord = p2TableLocation[table_index];       /* table t1 is in array index 0 */

   if ( p2TableWord == NULL )
   {
     errLogRet(LOGIT,debugInfo,"TABLE: table not defined\n");
     *errorcode = SYSTEMERROR+NOACODETBL;
     return(NULL);
   }

   if ( (*(p2TableWord+1) & 0xF0000000 ) != TABLEHEADER )
   {
     errLogRet(LOGIT,debugInfo,"TABLE: invalid header in table\n");
     *errorcode = SYSTEMERROR+NOACODETBL;
     return(NULL);
   }

   if ( (*(p2TableWord+1) &  0xFFFFFFF ) != table_index )
   {
     errLogRet(LOGIT,debugInfo,"TABLE: invalid index for table\n");
     *errorcode = SYSTEMERROR+NOACODETBL;
     return(NULL);
   }

   if ( *(p2TableWord+7) != TableSizes[table_index] )
   {
     errLogRet(LOGIT,debugInfo,"TABLE: invalid size for table\n");
     *errorcode = SYSTEMERROR+NOACODETBL;
     return(NULL);
   }
   return(p2TableWord);
}

#ifdef RF_CNTLR
#include "rfinfo.h"

struct _RFInfo rfInfo;  // was defined in rfTests.c, moved here , defined extern there now

int *pRFWfg = NULL;
int spareState = 0;  // for real time spare control

struct _DecInfo
{
  int patId;
  int policy;
  int wordsperstate;
  int startOffset;
  long long timeperState;
  long long patTicks;
  long long remTicks;
  long long vTicks;
  long long skipTicks;
  int *PatternList;
  int PatternSize;
  int numReps;
  int nBitsRev;
  int progDecRunning;
  int savedPhase;
  int startingPhase;
  int phasePerPass;
} DecInfo;

int progDecOn(int patid, long long pat_ticks, long long timePerState, int wordsPerState, int patoffset, long long patSkipTicks, int reps)
{
  /* check of patid */
  int errorCode;
  DecInfo.patId = patid;
  errorCode = getPattern(patid, &(DecInfo.PatternList), &(DecInfo.PatternSize));
  if (errorCode != 0) {
      return (errorCode);
  }
  if (pat_ticks < 4LL) pat_ticks=4LL; /* protect divide by zero */
  DecInfo.patTicks = pat_ticks;
  DecInfo.startOffset = patoffset;
  DecInfo.timeperState = timePerState;
  DecInfo.wordsperstate = wordsPerState;
  DecInfo.vTicks = 0LL;
  DecInfo.skipTicks = patSkipTicks;
  DecInfo.numReps = reps;             /* number of replications */
  DecInfo.progDecRunning = 1;
  DPRINT4(+0,"progDecOn: ticks: %lld, offset: %d, TimePerState: %lld, WordsPerState: %ld\n",
		DecInfo.patTicks, DecInfo.startOffset, DecInfo.timeperState, DecInfo.wordsperstate);
  return(0);
  /*  if (ix == 0) DecInfo.remTicks = 0LL; */
}

/* this is a guess */
#define MOSTLOOPS (200)
#define MOSTLOOPS_LL (200LL)

int isIncr(int w1)
{
   if ((w1&RFPHASEKEY) == RFPHASEKEY)           return(1);
   if ((w1&RFPHASECYCLEKEY) == RFPHASECYCLEKEY) return(1);
   if ((w1&RFAMPKEY) == RFAMPKEY)               return(1);
   if ((w1&RFAMPSCALEKEY)==RFAMPSCALEKEY)       return(1);
   return(0);
}
int progDecOff(int patid, int policy, long long runTicks)
{
  long long loopcount, temp, totalTicks;
  int loops,status,index, offsetFlag;
  int outstates;
  long long endTicks;
  int w1,w2;
  int runc = 1;
  int wce= 0;
  int wcs = 0;
  int runningPhase;
  index = 0;
  offsetFlag = (DecInfo.startingPhase & 0x10000000) != 0;
  DPRINT2(-1,"Starting Phase = 0x%x  %x\n",offsetFlag,(DecInfo.startingPhase)&0xffff);

  /* policy DECZEROTIME handles cases when decoupling has zero duration */
  if (policy == DECZEROTIME)
  {
    /* turn off the gates restore the defaults */
    writeCntrlFifoWord(encode_RFSetGates(0,0x3e,RF_TR_GATE));
    /* NO LATCH!!! leave in T R_LO_NOT unaltered */
    writeCntrlFifoWord(RFAMPKEY   | 0xffff );         /* DEFAULT NO LATCH!!! */
    writeCntrlFifoWord(RFPHASEKEY | 0);               /* DEFAULT NO LATCH!!! */

    /* clear DecInfo structure */
    DecInfo.remTicks       = 0LL;
    DecInfo.progDecRunning = 0;
    DecInfo.vTicks         = 0LL;
    DecInfo.policy         = DECSTOPEND;

    return(0);
  }

  DecInfo.policy = policy;
  totalTicks = (runTicks + DecInfo.remTicks + DecInfo.vTicks);
  DecInfo.remTicks = 0LL;
  loopcount = totalTicks / (DecInfo.patTicks);
  temp = loopcount;
  /* if abort or fail just return */
  if ((AbortingParserFlag == 1) || ( failAsserted == 1))
	  return(0);

  /* FIFO WORD WRITE - only necessary at start!! */
  writeCntrlFifoWord(encode_RFSetDuration(0,DecInfo.timeperState)); /* NO LATCH!!! */
  writeCntrlFifoWord(encode_RFSetGates(0,0x3e,0x3e) );                  /* NO LATCH!!! */
  /* this can send an enormous number of loops */
  DPRINT5(-1,"PROG DEC OFF; tickPerState: %lld,  total %lld  cycle %lld loops %lld  policy %d\n",\
	  DecInfo.timeperState,totalTicks,DecInfo.patTicks,loopcount,DecInfo.policy);
  runningPhase = DecInfo.savedPhase<<15; /* only used offsetter - 2^31 precision */

  while (temp > 0LL)
  {
      if ((AbortingParserFlag == 1) || ( failAsserted == 1))
      {
          DPRINT(-1,"PROG DEC OFF aborted\n");
	  return(0);  /* don't mask cause */
      }
      if (temp > MOSTLOOPS_LL)
        loops = MOSTLOOPS;
      else
        loops = (int) temp;
      /* SEND A BUNCH OF PATTERNS */

      if (offsetFlag)
      {

           for (index=0; index < loops; index++)
           {
        	 writeCntrlFifoWord(RFPHASECYCLEKEY | ((runningPhase >> 15)) & 0xffff );
             runningPhase += DecInfo.phasePerPass;
             writeCntrlFifoBuf(DecInfo.PatternList,DecInfo.PatternSize);
           }
      }
      else
      {
    	// small pattern guard...
    	if (DecInfo.PatternSize < DecThreshold)
    	{
    	   for (index=0; index < loops; index++)
    		  writeCntrlFifoBuf(DecInfo.PatternList,DecInfo.PatternSize);
    	}
    	else
    	{
           status = sendCntrlFifoList((DecInfo.PatternList), (DecInfo.PatternSize),loops);
           if (status == -1)
           {
             errLogRet(LOGIT,debugInfo,"progDecOff() : sendCntrlFifoList error!! \n");
             return(HDWAREERROR+PROG_DEC_LOAD_ERR);
           }
    	}
      }
      temp -= loops;
  }
  if (offsetFlag)
	  writeCntrlFifoWord(RFPHASECYCLEKEY | ((runningPhase >> 15)) & 0xffff );
  totalTicks -= (DecInfo.patTicks)*loopcount;
  DPRINT1(+1, "REMAINING TICKS %lld\n", totalTicks);


  /* change mode for the LAST scan */
  if ( (DecInfo.policy == DECCONTINUE) && (pAcqReferenceData->ct == (pAcqReferenceData->nt - 1)) )
  {
    DecInfo.policy = DECSTOPEND;
  }

  /* we have totalTicks time left - what do we do??*/

  if (DecInfo.policy == DECCONTINUE)
  { /* we know we're continuing */
    DPRINT1(+1, "DECONTINUE PUSHS TICKS %lld to next PROG DEC CYCLE\n", totalTicks);
    DecInfo.remTicks = totalTicks;  /* done */
  }
  /* leave gates alone and exit */


  if (DecInfo.policy == DECSTOPEND)
  {
    /* this depends upon a constant #words/ state */
    outstates = (int) (totalTicks/(DecInfo.timeperState));
    endTicks = totalTicks - outstates*(DecInfo.timeperState);
    pRFWfg = DecInfo.PatternList;

    /*  guard against time 0 to 3, if endTicks=0-3, merge it with the last outstate */
    if ((endTicks > 0LL) && (endTicks < 4LL))
    {
        if (outstates > 0) 
        {
        outstates--;
        endTicks += DecInfo.timeperState;
        }
        /* else clause under test */
    }

    /* send remaining full states */
    DPRINT1(-1, "STOPEND states %d\n", outstates);

    while ((runc > 0) && (outstates > 0))
	{
        w1 = *pRFWfg;
        w2 = 1;

	    if (isIncr(w1))
	    {
	      w2 = (w1 >> 16) & 0xff;
        }
        if (w2 == 0) w2 = 1;
        if (w2 > outstates)
        {
          runc=0;
          DPRINT(-3,"runc set to zero");
        }
	    else
	    {
	      wce++;
	      pRFWfg++;
          if ((wce-wcs) >= 8000)
	      {
                status = sendCntrlFifoList(DecInfo.PatternList+wcs, wce-wcs, ONE_TIME);
                DPRINT1(-1, "blaster sent %d\n", wce-wcs);
                wcs = wce;
	      }
	      if (w1 & LATCHKEY) outstates -= w2;
        }
   }
   DPRINT2(-1,"Final STATE %d + %d\n",wcs,wce);

   if ((wce-wcs) > 0)
	{
           status = sendCntrlFifoList(DecInfo.PatternList+wcs, wce-wcs, ONE_TIME);
           DPRINT1(-1, "FINAL blaster send %d\n", wce-wcs);
	}
   while (outstates > 0)
	{
        w1 = *pRFWfg;
        w2 = 1;
	    if (isIncr(w1))
	    {
	      w2 = (w1 >> 16) & 0xff;
        }
        if (w2 == 0) w2 = 1;
        if (w2 > outstates)
	    {
               w2 = outstates;
               w1 = w1 & 0xff00ffff | ((w2 & 0xff) << 16);
	       pRFWfg--;
	    }

	  if (w1 & LATCHKEY) outstates -= w2;
          writeCntrlFifoWord(w1 );
	  pRFWfg++;
   }
      /* DPRINT1(-1,"outstates done outstates = %d\n",outstates); */
      /* done with whole states, handle partial, and close down */
      /* read word if increment set to one, if latch, remove latch, add endTicks
             with latch - clean up gates. */
          while (endTicks > 0LL)
	  {
            if (pRFWfg > (DecInfo.PatternList + DecInfo.PatternSize))
	      {
                DPRINT(-3,"progdec off CYCLED - may be BAD\n");
	        pRFWfg = DecInfo.PatternList;
              }
            w1 = *pRFWfg++;
	    if (isIncr(w1))
	    {
	      w2 = (w1 >> 16) & 0xff;
            }
	    // there is less than out state of constraint to one
            if (w2 > 0)
	      w1 = w1 & 0xff01ffff; /* one increment */
            if (w1 & LATCHKEY) /* finish up */
	      {
                writeCntrlFifoWord(encode_RFSetDuration(0, (int) endTicks)); /* no Latch */
                writeCntrlFifoWord(w1);
                endTicks = 0LL;   /* break out of the loop */
	      }
	  }
        /* safe the gates */
        writeCntrlFifoWord(encode_RFSetGates(0,0x3e,RF_TR_GATE));  /* T on DEFAULT NO LATCH!!! */
        writeCntrlFifoWord(RFAMPKEY   | 0xffff );       /* DEFAULT NO LATCH!!! */
        writeCntrlFifoWord(RFPHASEKEY | 0);             /* DEFAULT NO LATCH!!! */
        if (offsetFlag) writeCntrlFifoWord(RFPHASECYCLEKEY | DecInfo.savedPhase );
        DecInfo.remTicks = 0LL;
        DecInfo.progDecRunning = 0;
        endTicks = 0LL;

  }
  if ((DecInfo.policy != DECCONTINUE) && (DecInfo.policy != DECSTOPEND))
      errLogRet(LOGIT,debugInfo, "progDecOff: BAD POLICY!!!\n");

  DecInfo.vTicks = 0LL;
  return(0);
}



int progDecSkipOff(int patid, int policy, long long runTicks, long long skipTicks)
{
  long long loopcount, temp, totalTicks;
  int  loops, status;
  int  outstates, skipfull_states, fract_ticks;
  long long endTicks;

  /* policy DECZEROTIME handles cases when decoupling has zero duration */
  if (policy == DECZEROTIME)
  {
    /* turn off the gates restore the defaults */
    writeCntrlFifoWord(encode_RFSetGates(0,0x3e,RF_TR_GATE));    /* leave in T NO LATCH!!! */
    writeCntrlFifoWord(RFAMPKEY   | 0xffff );         /* DEFAULT NO LATCH!!! */
    writeCntrlFifoWord(RFPHASEKEY | 0);               /* DEFAULT NO LATCH!!! */

    /* clear DecInfo structure */
    DecInfo.remTicks       = 0LL;
    DecInfo.progDecRunning = 0;
    DecInfo.vTicks         = 0LL;
    DecInfo.policy         = DECSTOPEND;

    return(0);
  }

  /* check for error conditions */

  if (skipTicks >= DecInfo.patTicks)
  {
     errLogRet(LOGIT,debugInfo, "error in programmable decoupling async skip time too large\n");
     return(HDWAREERROR+PROG_DEC_PAT_ERR);
  }

  DecInfo.policy   = policy;
  totalTicks       = (runTicks + DecInfo.remTicks + DecInfo.vTicks);
  DecInfo.remTicks = 0LL;


  /* figure out ticks for first stage (skip) and second stage (remainder) */

  lscratch2.lword = (DecInfo.patTicks/DecInfo.numReps);


  /* STAGE A                                                                     */
  /* compute the number of full states to skip and then start decoupler waveform */
  /* for e.g., if skipfull_states=2, then skip first 2 states, i.e. state 0 - state 1 */

  skipfull_states = (int)( skipTicks/DecInfo.timeperState );

  /* the first fractional state to execute (if any) */
  fract_ticks = (int) (DecInfo.timeperState - (skipTicks - skipfull_states*DecInfo.timeperState) );
  if ( (fract_ticks < 0) || (fract_ticks > DecInfo.timeperState) )
  {
     errLogRet(LOGIT,debugInfo, "error in programmable decoupling async skip calculation 1\n");
     return(HDWAREERROR+PROG_DEC_PAT_ERR);
  }

  /* if fract_ticks is <= 3, then merge it with the next state */
  if ( (fract_ticks >= 0) && (fract_ticks <= 3) )
  {
     fract_ticks += (int)DecInfo.timeperState;  /* add ticks per state  */
     skipfull_states++;                         /* and skip one more    */
  }
  /* temp is the number of ticks left over in one rep of the waveform after skip part */
  /*      if runTicks is smaller than temp, then set temp to runTicks                 */

  temp = lscratch2.lword - (skipfull_states * DecInfo.timeperState);
  if (temp > runTicks)
     temp = runTicks;

  /* now set this first fractional state into fifo. fract_ticks >= 4 */
  /* compute the correct pointer into waveform pattern               */

  /* DURATION WORD  NO LATCH            */
  writeCntrlFifoWord( encode_RFSetDuration(0, (int) fract_ticks));
  writeCntrlFifoWord( encode_RFSetGates(0,0x3e,0x3e));       /* NO LATCH!!! */

  /* compute the address after skipping */

  pRFWfg = (DecInfo.PatternList) + (skipfull_states*(DecInfo.wordsperstate));

  if (  (pRFWfg+(DecInfo.wordsperstate)) > ((DecInfo.PatternList) + (DecInfo.PatternSize)) )
  {
     errLogRet(LOGIT,debugInfo, "error in programmable decoupling async skip calculation 2\n");
     return(HDWAREERROR+PROG_DEC_PAT_ERR);
  }

  /* if words per state = 3 then GATEWORD NO LATCH */
  if (DecInfo.wordsperstate == 3)
  {
     if ( ((*pRFWfg) & 0x7C000000) == GATEKEY )
     {
        writeCntrlFifoWord( *pRFWfg );
        pRFWfg++;
     }
     else
        errLogRet(LOGIT,debugInfo,"error in executing first fractional waveform state\n");
  }

  /* if words per state >= 2 then AMPWORD  NO LATCH */
  if (DecInfo.wordsperstate >= 2)
  {
     if ( ((*pRFWfg) & 0x7C000000) == RFAMPKEY )
     {
        writeCntrlFifoWord( *pRFWfg );
        pRFWfg++;
     }
     else
        errLogRet(LOGIT,debugInfo,"error in executing first fractional waveform state\n");
  }


  /* PHASE WORD  (HAS LATCH ALREADY FROM PSG) */
  if ( ((*pRFWfg) & 0x7C000000) == RFPHASEKEY )
  {
     writeCntrlFifoWord( *pRFWfg );
  }
  else
     errLogRet(LOGIT,debugInfo,"error in executing first fractional waveform state\n");

  temp        -= fract_ticks;
  totalTicks  -= fract_ticks;

  /* STAGE B  NEED TO SEND REMAINING FULL STATES OF THE SKIP PART HERE */

  outstates = (int)(temp/DecInfo.timeperState);
  if (outstates > 0)
  {
     /* set the standard DURATIONKEY word */
    writeCntrlFifoWord( encode_RFSetDuration(0,DecInfo.timeperState));
    /* protect AGAINST SMALL PATTERNS */
     status = sendCntrlFifoList( (DecInfo.PatternList+((skipfull_states+1)*DecInfo.wordsperstate)), outstates*DecInfo.wordsperstate, ONE_TIME);
     temp       -= (DecInfo.timeperState)*outstates;
     totalTicks -= (DecInfo.timeperState)*outstates;
     DPRINT1(-1,"progDecSkipOff stage B has %lld ticks left over ???\n",temp);
  }

  /* STAGE C  now start with regular decProgOff at the beginning of the waveform */

  loopcount = totalTicks / (DecInfo.patTicks);
  temp      = loopcount;

   if ((AbortingParserFlag == 1) || ( failAsserted == 1))
      return(0);

  /* FIFO WORD WRITE */
  writeCntrlFifoWord(encode_RFSetDuration(0,DecInfo.timeperState)); /* NO LATCH!!! */
  writeCntrlFifoWord(encode_RFSetGates(0,0x3e,0x3e) );                  /* NO LATCH!!! */

  /* this can send an enormous number of loops */
  DPRINT5(-1,"PROG DEC SKIP OFF; tickPerState: %lld,  total %lld  cycle %lld loops %lld  policy %d\n",\
	  DecInfo.timeperState,totalTicks,DecInfo.patTicks,loopcount,DecInfo.policy);

  while (temp > 0LL)
  {
      if ((AbortingParserFlag == 1) || ( failAsserted == 1))
      {
          DPRINT(-1,"PROG DEC (SKIP) OFF aborted\n");
	  return(0);
      }
      if (temp > MOSTLOOPS_LL)
        loops = MOSTLOOPS;
      else
        loops = (int) temp;
      /* SEND A BUNCH OF PATTERNS */
      /* protect against small patterns */
      status = sendCntrlFifoList((DecInfo.PatternList), (DecInfo.PatternSize),loops);
      temp -= loops;
  }
  totalTicks -= (DecInfo.patTicks)*loopcount;
  DPRINT1(+1, "REMAINING TICKS %lld\n", totalTicks);


  /* change mode for the LAST scan */
  if ( (DecInfo.policy == DECCONTINUE) && (pAcqReferenceData->ct == (pAcqReferenceData->nt - 1)) )
  {
    DecInfo.policy = DECSTOPEND;
  }

  /* we have totalTicks time left - what do we do??*/

  if (DecInfo.policy == DECCONTINUE)
  { /* we know we're continuing */
    DPRINT1(+1, "DECONTINUE PUSHS TICKS %lld to next PROG DEC CYCLE\n", totalTicks);
    DecInfo.remTicks = totalTicks;  /* done */
  }
  /* leave gates alone and exit */


  if (DecInfo.policy == DECSTOPEND)
  {
    /* this depends upon a constant #words/ state */
    outstates = (int) (totalTicks/(DecInfo.timeperState));
    endTicks = totalTicks - outstates*(DecInfo.timeperState);

    /*  guard against time 0 to 3, if endTicks=0-3, merge it with the last outstate */
    if ((endTicks > 0LL) && (endTicks < 4LL))
    {
        outstates--;
        endTicks += DecInfo.timeperState;
    }

    /* STAGE D    send remaining full states */
    DPRINT2(+1, "STOPEND states %d, %d\n", outstates,outstates*(DecInfo.wordsperstate));
    if (outstates > 0)
    {
      status = sendCntrlFifoList((DecInfo.PatternList), outstates*DecInfo.wordsperstate,\
        ONE_TIME);
      /* protect against small patterns */
      DPRINT1(+1, "sending outstates %d\n", outstates);
    }


    /* STAGE E   send remaining fractional state */
    if (endTicks > 0LL)
    {

       /* compute addr of Pattern Word to be sent */
       pRFWfg = (DecInfo.PatternList) + (outstates*(DecInfo.wordsperstate));

       if (  (pRFWfg+(DecInfo.wordsperstate)) > ((DecInfo.PatternList) + (DecInfo.PatternSize)) )
           pRFWfg = (DecInfo.PatternList);

       /* if words per state = 3 then GATEWORD NO LATCH */
       if (DecInfo.wordsperstate == 3)
       {
           if ( ((*pRFWfg) & 0x7C000000) == GATEKEY )
           {
              writeCntrlFifoWord( *pRFWfg );
              pRFWfg++;
           }
           else
              errLogRet(LOGIT,debugInfo,"error in executing last fractional waveform state\n");
       }

       /* if words per state >= 2 then AMPWORD  NO LATCH */
       if (DecInfo.wordsperstate >= 2)
       {
           if ( ((*pRFWfg) & 0x7C000000) == RFAMPKEY )
           {
              writeCntrlFifoWord( *pRFWfg );
              pRFWfg++;
           }
           else
              errLogRet(LOGIT,debugInfo,"error in executing last fractional waveform state\n");
       }

       /* DURATION WORD  NO LATCH */
       writeCntrlFifoWord( DURATIONKEY | (int) endTicks & 0x3fffff);

       /* PHASE WORD  (HAS LATCH ALREADY FROM PSG) */
       if ( ((*pRFWfg) & 0x7C000000) == RFPHASEKEY )
       {
          writeCntrlFifoWord( *pRFWfg );
       }
       else
          errLogRet(LOGIT,debugInfo,"error in executing last fractional waveform state\n");

    }

    /* turn off the gates restore the defaults */
    /* GATES GO OFF! */
    writeCntrlFifoWord(encode_RFSetGates(0,0x3e,RF_TR_GATE));  /* DEFAULT NO LATCH!!! */
    writeCntrlFifoWord(RFAMPKEY   | 0xffff );       /* DEFAULT NO LATCH!!! */
    writeCntrlFifoWord(RFPHASEKEY | 0);             /* DEFAULT NO LATCH!!! */

    DecInfo.remTicks = 0LL;
    DecInfo.progDecRunning = 0;
  }

  if ((DecInfo.policy != DECCONTINUE) && (DecInfo.policy != DECSTOPEND))
      errLogRet(LOGIT,debugInfo, "progDecSkipOff: BAD POLICY!!!\n");

  DecInfo.vTicks = 0LL;

  return(0);
}


#endif


#if defined(PFG_CNTLR) || defined(GRADIENT_CNTLR)

void calc_obl_matrix(double ang1,double ang2,double ang3,double *tm11,double *tm12,double *tm13,   \
                  double *tm21,double *tm22,double *tm23,double *tm31,double *tm32,double *tm33)
{
    double D_R;
    double sinang1,cosang1,sinang2,cosang2,sinang3,cosang3;
    double m11,m12,m13,m21,m22,m23,m31,m32,m33;
    double im11,im12,im13,im21,im22,im23,im31,im32,im33;
    double tol = 1.0e-14;

    /* Convert the input to the basic mag_log matrix */
    D_R = 1.745329252e-2;      /* PI/180 */

    cosang1 = cos(D_R*ang1);
    sinang1 = sin(D_R*ang1);

    cosang2 = cos(D_R*ang2);
    sinang2 = sin(D_R*ang2);

    cosang3 = cos(D_R*ang3);
    sinang3 = sin(D_R*ang3);

    m11 = (sinang2*cosang1 - cosang2*cosang3*sinang1);
    m12 = (-1.0*sinang2*sinang1 - cosang2*cosang3*cosang1);
    m13 = (sinang3*cosang2);

    m21 = (-1.0*cosang2*cosang1 - sinang2*cosang3*sinang1);
    m22 = (cosang2*sinang1 - sinang2*cosang3*cosang1);
    m23 = (sinang3*sinang2);

    m31 = (sinang1*sinang3);
    m32 = (cosang1*sinang3);
    m33 = (cosang3);

    if (fabs(m11) < tol) m11 = 0;
    if (fabs(m12) < tol) m12 = 0;
    if (fabs(m13) < tol) m13 = 0;
    if (fabs(m21) < tol) m21 = 0;
    if (fabs(m22) < tol) m22 = 0;
    if (fabs(m23) < tol) m23 = 0;
    if (fabs(m31) < tol) m31 = 0;
    if (fabs(m32) < tol) m32 = 0;
    if (fabs(m33) < tol) m33 = 0;

    /* Generate the transform matrix for mag_log ******************/

    /*HEAD SUPINE*/
    im11 = m11;       im12 = m12;       im13 = m13;
    im21 = m21;       im22 = m22;       im23 = m23;
    im31 = m31;       im32 = m32;       im33 = m33;

    /*Transpose intermediate matrix and return***********/
    *tm11 = im11;     *tm21 = im12;     *tm31 = im13;
    *tm12 = im21;     *tm22 = im22;     *tm32 = im23;
    *tm13 = im31;     *tm23 = im32;     *tm33 = im33;
}
#endif

unsigned AdvisedFreq = 0;

#ifdef VXWORKS
int A32_interp(ACODE_ID pAcodeId)
#else
int A32_interp()
#endif
{

#if defined(PFG_CNTLR) || defined(GRADIENT_CNTLR)
    register int G_r, G_p, G_s, Gx, Gy, Gz;
    register int d_G_r, d_G_p, d_G_s;
    int statGx,  statGy,  statGz;
    int d_statGx,  d_statGy,  d_statGz;
    int tempGx1, tempGy1, tempGz1;
    int tempGx2, tempGy2, tempGz2;
    int tempGx3, tempGy3, tempGz3;
    int pat1_index, pat2_index, pat3_index;
    int pat4_index, pat5_index, pat6_index;
    int *patternList1, *patternList2, *patternList3;
    int *patternList4, *patternList5, *patternList6;
    int patternSize1 , patternSize2 , patternSize3 ;
    int patternSize4 , patternSize5 , patternSize6 ;
    int ii, jj, kk, ll, mm, nn, jtmp, tempv;
    int *pIntW1, *pIntW2, *pIntW3, *pgwfg;
    int *pIntW4, *pIntW5, *pIntW6;
    int gradgridword[10];
    long long lltemp;
    double tmp_m11, tmp_m12, tmp_m13;
    double tmp_m21, tmp_m22, tmp_m23;
    double tmp_m31, tmp_m32, tmp_m33;
#endif

    int priorityInversionCntDwn;   /* for use use MRIUSERBYTE & prority change */

    int actionloc;
    int i, j, k, l, m, n, skipped, pat_index, temp;
    int action, tmp, parseCount;
    int numargs;
    int npasses, nrem, nextra;
    unsigned int *ipntr, *ac_base;
    int ss, sscnt, ct, nt, bs, flag;
    /********TEMPORARY ************/
    int smallPhaseWord;
    int quadPhaseWord;
    /******************************/
    char expName[129];
    int curAcodeNumber;
    int *patternList;
    int patternSize;
    int *pIntW;
    int status;
    int errorcode;
    int newFid;
    int ddrstarted=0;
    double lastp=0;
    double lastf=0;
    int lastsrc=0;
    int lastdst=0;
    int lastnt=0;
    int skip=0;
    int avar=-1;
    int pvar=-1;
    int dbytes=4;
    unsigned long fidDim=0;
    unsigned long fidNF=0;
    unsigned long fidNFMod=0;
    unsigned long fidBuffs=0;
    unsigned long fidBuffSize=0;
/* loop stacks & variables */
#define NESTEDLOOP_DEPTH 10
    int nvLoopCountInd[NESTEDLOOP_DEPTH];
    int nvLoopCntrInd[NESTEDLOOP_DEPTH];
    int *p2NvLoopBeginPos[NESTEDLOOP_DEPTH];
    int loopStkPos = -1;

/* ifzero stacks & variables */
#define MAXIFZ 5           /* max number of nested ifzeros */
    int ifzLvl = -1;
    int ifzTrueBranchDone[MAXIFZ];

#ifdef MASTER_CNTLR
    int MixedRFError = 0;
    int channelBitsSet = 0;
#endif

#ifdef RF_CNTLR
    int itemp, jtemp;
    long long decSkipTicks=0LL;
#endif

    skipped   = 0;
    tnlk_flag = 0;
    codesOK   = 1;        /* true */
    readuserbyte = 0;

    priorityInversionCntDwn = -1;   /* for use use MRIUSERBYTE & prority change */

#ifdef VXWORKS
    strcpy(expName, pAcodeId->id);
    ac_base = p2CurrentWord = pAcodeId->cur_acode_base;
    pAcqReferenceData = pAcodeId->pLcStruct;
    rtVar = (int *) pAcodeId->pLcStruct;    /* (int *)
                         * &(acqReferenceData);  */

    curAcodeNumber = pAcodeId->cur_acode_set;
    newFid = 1;

    cntrlClearInstrCountTotal();  /* clear Instruction FIFO counter register */
    cntrlClrCumDmaCnt();  /* zero sw cumlutive DMA count to FIFO */

    /*
     * e.g. from FrontEnd.c p2EndOfCodes = globalAcodePntr +
     * j/sizeof(int);
     */
    DPRINT1(-1,"A32Interp(): pAcodeId->cur_acode_size: %d bytes\n",pAcodeId->cur_acode_size);
    p2EndOfCodes = pAcodeId->cur_acode_base + (pAcodeId->cur_acode_size / sizeof(int));

#ifdef RF_CNTLR
    /* determine if there was any previous iCAT RF failures during bootup */
    /* once set these are none recoverable, i.e. system must be rebooted, but that alone */
    /* will unlikey correct the iCAT problem.    */
    DPRINT1(-1, " icatFail = %d\n",icatFail);
    if (icatFail == -1)
    {
       if ( chk4IcatImageCpFail() != 0)
       {
          icatFail = ICAT_ERROR + IMAGE_COPY;   // Failure in iCAT RF update.
       }
       else if ( chk4IcatImageLdFail() != 0)
       {
          icatFail = ICAT_ERROR + IMAGE_LOAD;  // iCAT RF did not initialize properly
       }
       else if ( chk4IcatConfigFail() != 0 )
       {
          icatFail = ICAT_ERROR + REG_CONFIG;  // iCAT RF configuration failed
       }
       else
          icatFail = 0;
    }
    if (icatFail > 0)
    {
	       // taskDelay(calcSysClkTicks(500));   un-needed  5/25/2010  GMB
          return icatFail;
    }
#endif

#ifdef MASTER_CNTLR
    // throwing the error condition is left to the masters ROLLCALL Acode
    // this Acode sets what controllers are being used,  and thus what controllers
    // that are expected to send a exception complete message
    DPRINT1(1, " mixedRFs: %d \n",chk4MixedRF());
    if (chk4MixedRF() != 0)
    {
        MixedRFError = ICAT_ERROR + MIXED_RFS;
    }

#endif 

#else

    curAcodeNumber = 1;
    strcpy(expName, "BOGUS");
    ac_base = p2CurrentWord = getFirstAcodeSet(expName, curAcodeNumber, 0, 0);
    pAcqReferenceData = &acqReferenceData;
    rtVar = (int *) &(acqReferenceData);

#endif

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
    TSPRINT("Start to Parse Acodes:");
#endif
    while (codesOK)
    {
        DPRINT1(+1,"----------  Acode: 0x%lx\n",*p2CurrentWord);

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
        {
            char msg[128];
            sprintf(msg,"Acode: 0x%lx: ",*p2CurrentWord);
            TSPRINT(msg);
        }
#endif

        /* for use use MRIUSERBYTE & prority change */
        if (priorityInversionCntDwn > 0)
        {
#ifdef ENABLE_BUFFER_DURATION_CALC
*           unsigned long duration, wordsinfifo;
*           unsigned long bufDurationGet();
#endif
           unsigned long wordsinfifo;
           priorityInversionCntDwn--;
           wvEvent( (EVENT_MRISUERBYTE_CNTDWN + (0xFF & priorityInversionCntDwn)) ,NULL,NULL);
           DPRINT1(+4,"priorityInversionCntDwn: = %d\n",priorityInversionCntDwn);
           wordsinfifo =  wordsInFifoBuf();
           /* Without some sort of incrmental flushing of the FIFO buffers,
            * FIFO underflows may occur during the READ User Byte */
           if ( ((priorityInversionCntDwn % 10) == 0) && (wordsinfifo > 0))
           {
              flushCntrlFifoRemainingWords();
              DPRINT2(+4,"priorityInversionCntDwn: = %d, fifowords: %d\n",
			priorityInversionCntDwn, wordsinfifo);
           }

        }
        if (priorityInversionCntDwn == 0)   /* for use use MRIUSERBYTE & prority change */
        {
           resetParserPriority();
           priorityInversionCntDwn--;  /* now we won't keep reseting priority when unnecessary */
        }

        if ((AbortingParserFlag == 1) || ( failAsserted == 1))
        {
            if (priorityInversionCntDwn <= 0)   /* trial */
              resetParserPriority();
            errorcode = 0;
            return (errorcode); /* for aborting parser 'AA' */
        }

        /* stupid logic for end of buffer */
        if (p2CurrentWord >= p2EndOfCodes)
        {
            DPRINT1(-1,"----------  Acode: 0x%lx\n",*p2CurrentWord);
            DPRINT(-1, "---->  Ran Off End of Acodes <-------------\n");
            markAcodeSetDone(expName, curAcodeNumber);
            errorcode = HDWAREERROR + INVALIDACODE;
            return (errorcode); /* for the READER ONLY */
        }
        if ((*p2CurrentWord & 0xFFF00000) != 0xacd00000)
        {
            DPRINT1(-1,"---------- Previous Good  Acode: 0x%lx\n",action);
            DPRINT1(-1,"----------  Acode: 0x%lx\n",*p2CurrentWord);
            DPRINT1(-1, "Data Alignment Error! %x\n", action);
            markAcodeSetDone(expName, curAcodeNumber);
            errorcode = HDWAREERROR + INVALIDACODE;
            return (errorcode); /* for the READER ONLY */
        }

        if (!codesOK)
            break;

        action = *p2CurrentWord++;

        /* since this can be very verbose added a flag to enable/disable this Windview logging */
        if (wvlogAcodes == 1)
           wvEvent((1000 + (action & 0xFFF)),NULL,NULL);

        switch (action)
        {

        case INTER_REV_CHK:
            DPRINT(+1, " INTER_REV_CHK\n");
            if (*p2CurrentWord++ != COMPILEKEY) {
                /* complain(); goIdle(); break; */
                errorcode = HDWAREERROR + PSGIDERROR;
                /*
                 * errLogRet(LOGIT,debugInfo, "INTERP_REV_CHK
                 * Error: %d expected: %d.",(int)(*ac_cur),
                 * InterpRevId);
                 */
                return (errorcode);
            }
	    break;

        case FIFOSTART:
            DPRINT(-1, " FIFO_START\n");
            startFifo4Exp();   /* startCntrlFifo(); */
            break;

        case FIFOHALT:
            DPRINT(-1, " FIFO_HALT\n");
            writeCntrlFifoWord(LATCHKEY | DURATIONKEY | 0);
            break;

        case WAIT4XSYNC:
            DPRINT(-1, " WAIT4XSYNC\n");
            temp = *p2CurrentWord++;
#ifdef EXTERNALGATEKEY_DEFINED
            writeCntrlFifoWord(LATCHKEY | EXTERNALGATEKEY | temp);
#endif
            break;

        case SENDSYNC:	/* only the master should get this one */
            DPRINT2(-1,"SENDSYNC: post delay %d ticks, prepScan: %d\n",*p2CurrentWord, *(p2CurrentWord+1));
            temp = *p2CurrentWord++;    /* post delay in ticks, 8 = 100ns */
            prepflag = *p2CurrentWord++;    /* 0-no prep, 1- is prep so pPrepSem is taken in Syste,Syncup */

#ifdef  DURATION_DECODE
            if (decodeFifoWordsFlag > 0)
            {
              /* zero cumlutive duration calc for decoders */
              flushCntrlFifoRemainingWords();  /* force DMA of buffer, so that these words are not counted */
              execFunc("clrCumTime",NULL, NULL, NULL, NULL, NULL,NULL,NULL,NULL);
            }
#endif

            if (pAcqReferenceData->il_incr == 0)  /* not on an IL cycle */
            {
	       SystemSync(temp,prepflag);  /* just the 1st time, not on additional IL cycles */
            }
            break;

        case WAIT4ISYNC:
            temp = *p2CurrentWord++;  /* post delay in ticks, 8 = 100ns */
            DPRINT1(-1, " WAIT4ISYNC, postdelay ticks: %d\n",temp);

#ifdef  DURATION_DECODE
            if (decodeFifoWordsFlag > 0)
            {
               /* zero cumlutive duration calc for decoders */
               flushCntrlFifoRemainingWords();  /* force DMA of buffer, so that these words are not counted */
               execFunc("clrCumTime",NULL, NULL, NULL, NULL, NULL,NULL,NULL,NULL);
            }
#endif

            if (pAcqReferenceData->il_incr == 0)  /* not on an IL cycle */
            {
	        SystemSync(temp,0);  /* just the 1st time, not on additional IL cycles */
            }
            break;

        case WAIT4STOP:
            DPRINT(-1, "WAIT4STOP\n");
            wait4CntrlFifoStop();
            /* cntrlFifoWait4StopItrp(); */
            /* wait4NormalTermSignal(); */
            break;

	/* 
         * XGATE acode for correcting tick counts on RFController for iCAT (DD2)
	 * This acode is not invloved with the functionality of xgate; rather it is for correcting DD2 RF tick counts
         */
	case SYNC_XGATE_COUNT:
	     DPRINT(-1, "SYNC_XGATE_COUNT\n");
             i=*p2CurrentWord++;
             j=*p2CurrentWord++;
             xgateCount++;
             break;

        /*
         * force the working buffer of acodes to be queued for DMA
         */
	case FLUSHFFBUF:
             DPRINT(-1,"FLUSHFFBUF\n");
	     flushCntrlFifoRemainingWords();
             break;


            /*
             * DLOAD embedded stream stuffs directly n words, n
             * words of data
             */
        case DLOAD:
            j = *p2CurrentWord++;
            writeCntrlFifoBuf(p2CurrentWord, j);
            p2CurrentWord += j;
            break;

            /*
             * TLOAD stream table number,  rt var from table
             * number, choose data at rt var table contents are
             * number, number*data
             */
#ifdef TLOADDEFINED
        case TLOAD:
            i = *p2CurrentWord++;
            j = *p2CurrentWord++;
            k = rtVar[j];
            break;
#endif
            /*
             * execute pattern dispatches a pre-set fifo word set
             * to the FIFO via DMA engine.
             */
        case EXECUTEPATTERN:
            pat_index = *p2CurrentWord++;
            DPRINT1(-1, "EXECUTE PATTERN %d\n", pat_index);
            errorcode = getPattern(pat_index, &patternList, &patternSize);
            if (errorcode == 0)
            {
		if (patternSize > 0)
		{
                  if (patternSize < PATTERN_LOW_LIMIT)
		     writeCntrlFifoBuf(patternList,patternSize);
                  else
                     sendCntrlFifoList(patternList, patternSize, ONE_TIME);
	        }
		else
		  DPRINT1(-2,"Zero or Negative Pattern Size - Be Very Alarmed %d",pat_index);
            }
            else
            {
                return (errorcode);
            }
            break;

            /*
             * execute pattern based on Real Time Control using a
             * pattern index comprised of a base index + rtvalue
             */
        case EXECRTPATTERN:
            i = *p2CurrentWord++;
            j = *p2CurrentWord++;
            pat_index = i + rtVar[j];
            DPRINT1(-1, "EXECRTPATTERN %d\n", pat_index);
            errorcode = getPattern(pat_index, &patternList, &patternSize);
            if (errorcode == 0)
            {
		if (patternSize > 0)
		{
                  if (patternSize < PATTERN_LOW_LIMIT)
		     writeCntrlFifoBuf(patternList,patternSize);
                  else
                     sendCntrlFifoList(patternList, patternSize, ONE_TIME);
	        }
		else
		  DPRINT1(-2,"Zero or Negative Pattern Size - Be Very Alarmed %d",pat_index);
            }
            else
            {
                return (errorcode);
            }
            break;

            /*
             * dispatch a number of whole and part of patterns a
             * delay is added at the end to align timing
             */
        case MULTIPATTERN:
            DPRINT(-1, " MULTIPATTERN\n");
            pat_index = *p2CurrentWord++;
            npasses = *p2CurrentWord++; /* npasses argument */
            nrem = *p2CurrentWord++;    /* seqment thru the
                             * pattern where we
                             * quit.. */
            DPRINT3(-1, "Multi PATTERN patIndex: %d, passes of pat: %d, remaining portion size:%d \n",
		    pat_index, npasses, nrem);

            errorcode = getPattern(pat_index, &patternList, &patternSize);
            if (errorcode != 0) {
    	        errLogRet(LOGIT,debugInfo,
			"MULTIPATTERN: Pattern not found.\n");
                return (errorcode);
            }

            /* this needs to be double checked,  GMB 10/19/04  */
	    /* protect against small patterns */
            sendCntrlFifoList(patternList, patternSize, npasses );
            if ( nrem > 0)
            {
	       if (nrem < 200)
	       writeCntrlFifoBuf(patternList, nrem);
	       else
	       sendCntrlFifoList(patternList,nrem,1);
	    }
            /* form a remticks delay. reuse nrem */
            nextra = *p2CurrentWord++;
            if (nextra > 3)
                writeCntrlFifoWord(LATCHKEY | DURATIONKEY | nextra);
            else if (nextra != 0)
            {
    	        errLogRet(LOGIT,debugInfo,
			"MULTIPATTERN: BAD DELAY.\n");
                return (HDWAREERROR + INVALIDACODE);
            }
            break;
        case RTFREQ: 
            i = *p2CurrentWord++;  // rt var id
            j = *p2CurrentWord++;  // pattern number
            k = *p2CurrentWord++;  // size of one transfer
            // 
            l = rtVar[i];
            DPRINT3(-1," RTFREQ: %s dereferenced to %d, tsize =%d\n", rt_descr[i],l,k);
            errorcode = getPattern(j, &patternList, &patternSize);
            if (errorcode != 0) return(errorcode);
            if (patternSize < 1)
               DPRINT(-1,"rtfreq 0 length pattern");
            //
            j = patternSize / k;
            l %= j;
            DPRINT1(-1,"RTFREQ base pointer = %lx",patternList);
            patternList += l*k; // check pointer MATH
            DPRINT1(-1,"RTFREQ base pointer = %lx",patternList);
            for (i=0; i < k; i++)
               DPRINT1(-1,"value = %x",patternList[i]);
	    writeCntrlFifoBuf(patternList,k);
            break;
            /* TABLEDEF, TBD */
        case SAFESTATE:
             // currently only RF uses..
             setSafeGate(*p2CurrentWord++);
             // IMPORTANT this sets the conditions AFTER AN ABORT
             resetSafeVals();
             break;

        case SOFTDELAY:
            DPRINT(-1, " SOFTDELAY\n");
            taskDelay(*p2CurrentWord++);
            break;

        case BIGDELAY:
            DPRINT(-1, " BIGDELAY\n");
            lscratch1.nword[0] = *p2CurrentWord++; /* pattern ticks high word*/
            lscratch1.nword[1] = *p2CurrentWord++; /* pattern ticks low word */
            while (lscratch1.lword >= 4)
            {
              if (lscratch1.lword < 0x3ffffff)
                l = lscratch1.lword;
              else
	        l = 0x3F00000; /* ensure a residual delay */
              writeCntrlFifoWord(LATCHKEY | DURATIONKEY | l );
              lscratch1.lword -= l;  /* what's left */
            }
            break;

            /* RTINIT  rt[i] = j user*/
        case RTINIT:
            i = *p2CurrentWord++;   /* sub code or operation */
            j = *p2CurrentWord++;   /* result unary op an RT var */
            if (initvalFlag == 1)
            {
              rtVar[j] = i;        /* 1st arg value;  second arg rtvar destination */
            }
            break;

            /* FRTINIT  rt[i] = j always*/
        case FRTINIT:
            i = *p2CurrentWord++;   /* sub code or operation */
            j = *p2CurrentWord++;   /* result unary op an RT var */
            rtVar[j] = i;           /* 1st arg value;  second arg rtvar destination */
            break;

            /* RT operations group...  sub code, index */
            /* RT operations group...  sub code, index */
            /* rt[j] = op(i) rt[j] */
        case RTOP:
            i = *p2CurrentWord++;   /* sub code or operation */
            j = *p2CurrentWord++;   /* result unary op an RT var */
            RT1op(i, j);
            break;

            /* rt[k] = op(i) rt[j] */
        case RT2OP:
            i = *p2CurrentWord++;   /* sub code or operation */
            j = *p2CurrentWord++;   /* source variable */
            k = *p2CurrentWord++;   /* result in k */
            RT2op(i, j, k);
            break;

            /* rt[l] = rt[j]  op(i) rt[k] */
        case RT3OP:
            i = *p2CurrentWord++;   /* sub code or operation */
            j = *p2CurrentWord++;   /* upper bound */
            k = *p2CurrentWord++;   /* lower bound */
            l = *p2CurrentWord++;   /* result in l */
            RT3op(i, j, k, l);
            break;

            /*
             * These are really debugging tools but cause ERRORS
             * iff the limits are violated. RTASSERT   test word
             * against upper and lower bounds - 32 bit range only
             * RTASSERTH  test word against upper bound
             * - 32 bit range only RTASSERTL  test word against
             * lower bound            - 32 bit range only
             * LRTASSERT  test word against upper bound
             * - 64 bit range
             */
        case RTASSERT:
            DPRINT(-1, " RTASSERT\n");
            i = *p2CurrentWord++;   /* index */
            j = *p2CurrentWord++;   /* upper bound */
            k = *p2CurrentWord++;   /* lower bound */
            if ((rtVar[i] > j) || (rtVar[i] < k))
    	        errLogRet(LOGIT,debugInfo,
			"RTASSERT: THIS IS VERY WRONG.\n");
            break;

        case RTASSERTH:
            DPRINT(-1, " RTASSERTH\n");
            i = *p2CurrentWord++;   /* index */
            j = *p2CurrentWord++;   /* upper bound */
            if (rtVar[i] > j)
    	        errLogRet(LOGIT,debugInfo,
			"RTASSERTH: THIS IS VERY WRONG.\n");
            break;

        case RTASSERTL:
            DPRINT(-1, " RTASSERTL\n");
            i = *p2CurrentWord++;   /* index */
            j = *p2CurrentWord++;   /* lower bound */
            if (rtVar[i] < j)
    	        errLogRet(LOGIT,debugInfo,
			"RTASSERTL: THIS IS VERY WRONG.\n");
            break;

#ifdef LONGLONGCASE
        case LRTASSERT:
            i = *p2CurrentWord++;
            lscratch1.nword[0] = *p2CurrentWord++;
            lscratch1.nword[1] = *p2CurrentWord++;
            if (rtVar[i] > lscratch1)
                ERROR;
            break;
#endif
            /*
             * these are range bounding acodes RTCLIP   test word
             * and clip against upper and lower bounds - 32 bit
             * range only RTCLIPH  test word and clip against
             * upper bound            - 32 bit range only RTCLIPL
             * test word and clip against lower bound
             * - 32 bit range only LRTASSERT  test word against
             * upper bound                   - 64 bit range
             */
        case RTCLIP:
            DPRINT(+1, " RTCLIP\n");
            i = *p2CurrentWord++;   /* index */
            j = *p2CurrentWord++;   /* upper bound */
            k = *p2CurrentWord++;   /* lower bound */
            if (rtVar[i] > j)
                rtVar[i] = j;
            if (rtVar[i] < k)
                rtVar[i] = k;
            break;

        case RTCLIPH:
            DPRINT(+1, " RTCLIPh\n");
            i = *p2CurrentWord++;   /* index */
            j = *p2CurrentWord++;   /* upper bound */
            if (rtVar[i] > j)
                rtVar[i] = j;
            break;

        case RTCLIPL:
            DPRINT(+1, " RTCLIPL\n");
            i = *p2CurrentWord++;   /* index */
            j = *p2CurrentWord++;   /* upper bound */
            if (rtVar[i] < j)
                rtVar[i] = j;
            break;

#ifdef LONGLONGCASE
        case LRTCLIP:
            i = *p2CurrentWord++;   /* index */
            lscratch.nword[0] = *p2CurrentWord++;   /* upper bound top word */
            lscratch.nword[1] = *p2CurrentWord++;   /* upper bound bottom
                                 * word */
            if (rtVar[i] > lscratch1)
                rtVar[i] = lscratch1;
            break;
#endif
            /*
             * Control Flow commands are reference to
             * p2ReferenceLocation. JUMP  - unconditional move to
             * computed value JMP_LT JTM_NE  conditional moves.
             */
        case JUMP:
            DPRINT(-1, " JUMP\n");
            i = *p2CurrentWord++;
            p2CurrentWord = p2ReferenceLocation + i;
            break;

        case JMP_LT:
            DPRINT(-1, " JMP_LT\n");
            i = *p2CurrentWord++;
            j = *p2CurrentWord++;
            k = *p2CurrentWord++;
            if (rtVar[i] < rtVar[j])
                p2CurrentWord = p2ReferenceLocation + k;
            break;

        case JMP_NE:
            DPRINT(-1, " JMP_NE\n");
            i = *p2CurrentWord++;
            j = *p2CurrentWord++;
            k = *p2CurrentWord++;
            if (rtVar[i] != rtVar[j])
                p2CurrentWord = p2ReferenceLocation + k;
            break;

        case NVLOOP:
            /* DPRINT(-1, " NVLOOP\n"); */
            loopStkPos++;
            if(loopStkPos>= NESTEDLOOP_DEPTH)
                return (SYSTEMERROR+NVLOOP_ERR);

            nvLoopCountInd[loopStkPos]    = *p2CurrentWord++;      // loop count

            if (rtVar[nvLoopCountInd[loopStkPos]] <= 0)            // if loop count is 0
            {
              p2CurrentWord++;                                     // loop index
              i = *p2CurrentWord++;                                // num words to skip to bypass loop
              p2CurrentWord += i;                                  // jump to 1st word after ENDNVLOOP
              loopStkPos--;                                        // unwind stack
              break;
            }

            nvLoopCntrInd[loopStkPos]     = *p2CurrentWord++;      // loop index
            p2CurrentWord++;                                       // num words to skip to bypass loop
            rtVar[nvLoopCntrInd[loopStkPos]] = 0;
            p2NvLoopBeginPos[loopStkPos]  =  p2CurrentWord;
            break;

        case ENDNVLOOP:
            /* DPRINT(-1, " ENDNVLOOP\n"); */
            p2CurrentWord++;
            p2CurrentWord++;
            rtVar[nvLoopCntrInd[loopStkPos]]++;
            if (rtVar[nvLoopCntrInd[loopStkPos]] < rtVar[nvLoopCountInd[loopStkPos]])
            {
              p2CurrentWord = p2NvLoopBeginPos[loopStkPos];
              /*
              DPRINT2(-1," ENDNVLOOP: loop iterating current index=%d  limit=%d\n",     \
                 (rtVar[nvLoopCntrInd[loopStkPos]]-1),rtVar[nvLoopCountInd[loopStkPos]]);
              */
            }
            else if(loopStkPos>=0)
            {
              /* DPRINT1(-1," ENDNVLOOP: out of loop %d \n",nvLoopCntrInd[loopStkPos]); */
              loopStkPos--;                         /* pop the stacks */
            }
            else
                return (SYSTEMERROR+NVLOOP_ERR);
            break;

        case IFZERO:
            DPRINT(-1, " IFZERO\n");

            i = *p2CurrentWord++;    /* rtVar                          */
            j = *p2CurrentWord++;    /* else branch exists 0 or 1      */
            k = *p2CurrentWord++;    /* num words to skip to ELSENZ    */
            l = *p2CurrentWord++;    /* num words to skip to ENDIFZERO */

            ifzLvl++;                /* increment nested ifzero level  */
            if(ifzLvl>=MAXIFZ)
                return (SYSTEMERROR+IFZERO_ERR);
            if ( rtVar[i] == 0 )
            {
               /* ifzero TRUE branch                */
               ifzTrueBranchDone[ifzLvl] = 1;
               /* break & go to interpret next word */
            }
            else
            {
               /* ifzero ELSENZ branch                       */
               ifzTrueBranchDone[ifzLvl] = 0;
               if ( j )                /* else branch exists */
               {
                 p2CurrentWord += k ;    /* num words to skip to ELSENZ acode */
                 /* break & go to interpret next word */
               }
               else
               {
                 p2CurrentWord += l ;    /* num words to skip to ENDIFZERO acode */
                /* break & go to interpret next word */
               }
            }

            break;

        case IFMOD2ZERO:
            DPRINT(-1, " IFMOD2ZERO\n");

            i = *p2CurrentWord++;    /* rtVar                          */
            j = *p2CurrentWord++;    /* else branch exists 0 or 1      */
            k = *p2CurrentWord++;    /* num words to skip to ELSENZ    */
            l = *p2CurrentWord++;    /* num words to skip to ENDIFZERO */

            ifzLvl++;                /* increment nested ifzero level  */
            if(ifzLvl>=MAXIFZ)
                return (SYSTEMERROR+IFZERO_ERR);
            if ( (rtVar[i] % 2) == 0 )
            {
               /* ifmod2zero TRUE branch                */
               ifzTrueBranchDone[ifzLvl] = 1;
               /* break & go to interpret next word */
            }
            else
            {
               /* ifzero ELSENZ branch                       */
               ifzTrueBranchDone[ifzLvl] = 0;
               if ( j )                /* else branch exists */
               {
                 p2CurrentWord += k ;    /* num words to skip to ELSENZ acode */
                 /* break & go to interpret next word */
               }
               else
               {
                 p2CurrentWord += l ;    /* num words to skip to ENDIFZERO acode */
                /* break & go to interpret next word */
               }
            }

            break;

        case ELSENZ:
            DPRINT(-1, " ELSENZ\n");

            i = *p2CurrentWord++;    /* rtVar                         */
            j = *p2CurrentWord++;    /* num words to skip to ENDIFZERO*/

            /* can arrive at this acode w/ or w/o executing code in IFZERO TRUE
               branch. hence need to differentiate                           */

            if ( ifzTrueBranchDone[ifzLvl] == 1 )
            {
              /* ifzero TRUE branch was already done, skip to endif   */
              p2CurrentWord += j ;      /* num words to skip to endif */
              /* break & go to interpret next word */
            }

            /*
            else
            {
                 do ifzero ELSENZ branch
                 p2CurrentWord now points to the first word of elsenz
                 break & go to interpret next word
            }
            */

            break;


        case ENDIFZERO:
            DPRINT(-1, " ENDIFZERO\n");

            i = *p2CurrentWord++;    /* rtVar */

            /* decrement ifzero depth pointer. out of ifzero-endif block */
            if(ifzLvl<0)
                return (SYSTEMERROR+IFZERO_ERR);

            ifzLvl--;

            break;


       case VDELAY:

            i = *p2CurrentWord++;
            j = *p2CurrentWord++;
            lscratch1.lword = ((long long) rtVar[i])*((long long) j);

#ifdef RF_CNTLR
          if ( DecInfo.progDecRunning == 1 )
          {
            DecInfo.vTicks += lscratch1.lword;
            lscratch1.lword = 0;
          }
          else
          {
            while (lscratch1.lword >= 4)
            {
              if (lscratch1.lword < 0x3ffffff)
                l = lscratch1.lword;
              else
                l = 0x3f00000; /* ensure a residual delay */
              writeCntrlFifoWord(LATCHKEY | DURATIONKEY | l );
              lscratch1.lword -= l;  /* what's left */
            }
          }
#endif

#if defined(MASTER_CNTLR) || defined(DDR_CNTLR) || defined(PFG_CNTLR)

            while (lscratch1.lword >= 4)
            {
              if (lscratch1.lword < 0x3ffffff)
                l = lscratch1.lword;
              else
                l = 0x3f00000; /* ensure a residual delay */
              writeCntrlFifoWord(LATCHKEY | DURATIONKEY | l );
              lscratch1.lword -= l;  /* what's left */
            }
#endif

#ifdef GRADIENT_CNTLR
{
            int repeats;
            while (lscratch1.lword >= 4L)
            {
              if (lscratch1.lword < 640L)
                {/* 8 usec or less */
                  l = lscratch1.lword;
                  writeCntrlFifoWord(LATCHKEY | DURATIONKEY | l );
                  /* DPRINT1(2,"output normal = %lx\n",l); */
                }
              else
              {  /* always leave a little residual room  */
                 if (lscratch1.lword > 320*65534)
                 {
                     l = 320*65530;
                     /* DPRINT1(0,"output repeated clip = %lx\n",l); */
                     writeCntrlFifoWord(LATCHKEY | (17<< 26) |(65530 << 10) | 320 );
                 }
                 else
                 {  /* it is smaller than 320*65530 but larger than 640 */
                    repeats = lscratch1.lword / 320;

                    repeats--; /* leave at least 320 to avoid the 4usec grid */
                    /* DPRINT1(0,"output repeats = %x\n",repeats); */
                    writeCntrlFifoWord(LATCHKEY | (17<< 26) | (repeats << 10) | 320 );
                    l = repeats*320;
                 }
              }

              lscratch1.lword -= l;  /* what's left */
              /* DPRINT1(0,"left to do repeats = %lx\n",lscratch1.lword); */
            }
}
#endif
            break;


        case VDELAY_LIST:
	    DPRINT(-1, " VDELAY_LIST\n");
            pat_index = *p2CurrentWord++;   /* pattern id for vdelay list sets */
            i         = *p2CurrentWord++;   /* v-index */

            errorcode = getPattern(pat_index, &patternList, &patternSize);
            if (errorcode != 0)
            {
                errLogRet(LOGIT,debugInfo,
                        "VDELAY_LIST: vdelay_list not found.\n");
                return (errorcode);
            }
            pIntW = patternList;

            j = rtVar[i];
            if (j < 0)
            {
               /* error */
            }
            while (j >= patternSize/2)   // loop index starts at 0; gets reset if it exceeds vdelays
            {
              j -= patternSize/2;
            }

            lscratch1.nword[0] =  *( pIntW + (2*j+0) ) ;
            lscratch1.nword[1] =  *( pIntW + (2*j+1) ) ;

#ifdef RF_CNTLR
          if ( DecInfo.progDecRunning == 1 )
          {
            DecInfo.vTicks += lscratch1.lword;
            lscratch1.lword = 0;
          }
          else
          {
            while (lscratch1.lword >= 4)
            {
              if (lscratch1.lword < 0x3ffffff)
                l = lscratch1.lword;
              else
                l = 0x3f00000; /* ensure a residual delay */
              writeCntrlFifoWord(LATCHKEY | DURATIONKEY | l );
              lscratch1.lword -= l;  /* what's left */
            }
          }
#endif

#if defined(MASTER_CNTLR) || defined(DDR_CNTLR) || defined(PFG_CNTLR)

            while (lscratch1.lword >= 4)
            {
              if (lscratch1.lword < 0x3ffffff)
                l = lscratch1.lword;
              else
                l = 0x3f00000; /* ensure a residual delay */
              writeCntrlFifoWord(LATCHKEY | DURATIONKEY | l );
              lscratch1.lword -= l;  /* what's left */
            }
#endif

#ifdef GRADIENT_CNTLR
{
            int repeats;
            while (lscratch1.lword >= 4L)
            {
              if (lscratch1.lword < 640L)
                {/* 8 usec or less */
                  l = lscratch1.lword;
                  writeCntrlFifoWord(LATCHKEY | DURATIONKEY | l );
                  /* DPRINT1(2,"output normal = %lx\n",l); */
                }
              else
              {  /* always leave a little residual room  */
                 if (lscratch1.lword > 320*65534)
                 {
                     l = 320*65530;
                     /* DPRINT1(0,"output repeated clip = %lx\n",l); */
                     writeCntrlFifoWord(LATCHKEY | (17<< 26) |(65530 << 10) | 320 );
                 }
                 else
                 {  /* it is smaller than 320*65530 but larger than 640 */
                    repeats = lscratch1.lword / 320;

                    repeats--; /* leave at least 320 to avoid the 4usec grid */
                    /* DPRINT1(0,"output repeats = %x\n",repeats); */
                    writeCntrlFifoWord(LATCHKEY | (17<< 26) | (repeats << 10) | 320 );
                    l = repeats*320;
                 }
              }

              lscratch1.lword -= l;  /* what's left */
              /* DPRINT1(0,"left to do repeats = %lx\n",lscratch1.lword); */
            }
}
#endif
            break;


            /* PADDELAY <- OMITTED */
        case PAD_DELAY:
            DPRINT(-1, " PAD_DELAY\n");
            i = *p2CurrentWord++;
            j = *p2CurrentWord++;
            if (i != 0)
                writeCntrlFifoWord(LATCHKEY | DURATIONKEY | (0x3ffffff & rtVar[i]));
            else
                writeCntrlFifoWord(LATCHKEY | DURATIONKEY | (0x3ffffff & j));
            break;


            /*
             * RT Table  (in line)
             */

        case TABLE:
            DPRINT(-1, " TABLE\n");
            /* i = Table Index  0-59 for t1-t60    j = Table Size */

            if (( *p2CurrentWord & 0xF0000000 ) != TABLEHEADER )
               errLogRet(LOGIT,debugInfo,"TABLE: invalid header in table\n");

            i = ( (*p2CurrentWord) & 0xFFFFFFF ) ; /* strip the TABLEHEADER ID */

            /* DPRINT3(-1,"i=%d   p2CurrentWord=0x%x  *p2CurrentWord=%d\n",i,p2CurrentWord,(*p2CurrentWord)); */

            if ( (i >= 0) && (i <= 59) )
            {
              /* p2TableLocation points to the position of TABLE acode */
              p2TableLocation[i] =  (p2CurrentWord - 1) ;

              j = *(p2CurrentWord + 6) ;
              if ( j <= 0 )
                errLogRet(LOGIT,debugInfo,"TABLE: invalid size for table\n");

              TableSizes[i] = j ;

              /* Now skip to end of TABLE acode+data section */
              p2CurrentWord += j;

              /* DPRINT2(-1,"TableSizes[i]=%d   p2CurrentWord=0x%x\n",TableSizes[i],p2CurrentWord); */
            }
            else
              errLogRet(LOGIT,debugInfo,"TABLE: invalid index for table\n");

            break;


            /*
             * RT table assign  - two forms... if Table is an
             * autoincrement table arg2 (j) is IGNORED index is
             * most often ct...
             */
        case TASSIGN:

            i = *p2CurrentWord++;
            j = *p2CurrentWord++;
            k = *p2CurrentWord++;
            l = rtVar[j];
            rtVar[k] = getTableElement(i, l, &errorcode, "tassign");
DPRINT5(-1, " TASSIGN table i %d; j= %d k= %d l= %d val= %d\n", i, j,k,l, rtVar[k]);
            if (errorcode)
                return (errorcode);
            break;

        case TPUT:

            i = *p2CurrentWord++;
            j = *p2CurrentWord++;
            k = *p2CurrentWord++;
            l = rtVar[j];
DPRINT5(-1, " TPUT    table i %d; j= %d k= %d l= %d val= %d\n", i, j,k,l, rtVar[k]);
            putTableElement(i, l, rtVar[k], &errorcode, "tassign");
            if (errorcode)
                return (errorcode);
            break;

        case TCOUNT:
         {
            int *p2TableWord, *ptr;
            int tableSize, elemSize, index, count;
            i = *p2CurrentWord++;
            j = *p2CurrentWord++;
            k = *p2CurrentWord++;
            l = rtVar[j];
            p2TableWord = getP2Table(i, &errorcode, "tcount");
            if (errorcode)
                return (errorcode);
            if (l < 0)
               l = 0;
            tableSize    = *(p2TableWord + 2);
            elemSize     = *(p2TableWord + 6);
            count = 0;
            if (elemSize == 32)
            {
               ptr = p2TableWord + TABLE_HDR_SIZE + l;
               while ( (l++ < tableSize) && ( *ptr++ == 0 ) )
               {
                  count++;
               }
            }
            if (elemSize == 1)
            {
               ptr = p2TableWord + TABLE_HDR_SIZE + l/32;
               while ( (l < tableSize) && (( *ptr & ( 1 << (l % 32)))  == 0 )  )
               {
                  count++;
                  l++;
                  if ( l % 32 == 0)
                  {
                     ptr++;
                  }
               }
            }
            rtVar[k] = count;
         }
            break;

#ifdef OLD
        case TSETPNTR:
            DPRINT(-1, " TSETPNTR\n");
            i = *p2CurrentWord++;
            j = *p2CurrentWord++;
            l = rtVar[j];
            k = *p2CurrentWord++;
            rtVar[k] = (unsigned int) getP2TableElement(i, l, &errorcode, "tassign");
            if (errorcode)
                return (errorcode);
            break;
#endif

            /*
             * INITSCAN This element initializes counter for a
             * new increment.  The np data in the old acode moves
             * to ARMDDR.  This also 1. sets the base for the
             * jump codes 2. pulls in a current low core -> pops
             * the default il='n' or pops from somewhere? 3.
             * tbd... ss, nt, bs should be in the low core
             * already ... fid num should be passed. instead...
             * space allocation is necessary on ddr..  there is
             * an SS2 parameter...
             */
        case INITSCAN:
            DPRINT(+1, " INIT_SCAN\n");
            /* RF ONLY USE */
#ifdef RF_CNTLR
            DecInfo.startOffset = 0;
            DecInfo.remTicks = 0L;
            DecInfo.patTicks = 0L;
            DecInfo.vTicks   = 0L;
            DecInfo.skipTicks= 0L;
            DecInfo.policy   = 0;
            DecInfo.patId    = 0;
            DecInfo.numReps  = 1;
            i = 0;
            j = 1;
            while (j < pAcqReferenceData->nt)
            {
              j = j*2;
              i++;
            }
            if (i > 0)
              DecInfo.nBitsRev = i;
            else
              DecInfo.nBitsRev = 1;
#endif
            smallPhaseWord = 0;


            i = *p2CurrentWord++;
            pAcqReferenceData->np = i;
            j = *p2CurrentWord++;
            pAcqReferenceData->nt = j;
            k = *p2CurrentWord++;
            pAcqReferenceData->ss   = k;
            pAcqReferenceData->ssct = k;
            l = *p2CurrentWord++;
            pAcqReferenceData->bs = l;
            /* interleave acquisition test */
            if (pAcqReferenceData->il_incr == 0)
            {
              pAcqReferenceData->ct = 0;      /* <--- is this right? should it not be startct ?? */
              pAcqReferenceData->bsct = 0;
            }
            else
            {
              /* for interleave cycle need to set the bsct & ct to the proper values */
              pAcqReferenceData->bsct = pAcqReferenceData->il_incrBsCnt;
              pAcqReferenceData->ct = pAcqReferenceData->il_incrBsCnt * pAcqReferenceData->bs;
              pAcqReferenceData->ssct = 0;  /* no ss on the il cycles, except the 1st time thought */
            }
            m = *p2CurrentWord++;
            pAcqReferenceData->cpf = m;     /* phase cycling 1=no cycling,  0=quad detect*/

            if (pAcqReferenceData->il_incr == 0)
            {
              initvalFlag = 1;
            }
            resetRTVars();

            for(i=0; i<MAXTABLES; i++)
            {
              p2TableLocation[i]=NULL;  TableSizes[i]=0;
            }

            pAcodeId->initial_scan_num = pAcqReferenceData->ct;
            pAcodeId->cur_scan_data_adr = 0;

            if (pAcqReferenceData->rtonce == 0)    /* reset the xgate count only once during the entire scan */
               xgateCount = 0LL;

#ifdef MASTER_CNTLR
            pCurrentStatBlock->AcqTickCountError = -1;
#endif

            /* popInLowCore(args); */
            break;

        case NEXTSCAN:
            DPRINT4(+0, " NEXTSCAN +++++++++++++++++++++++ For ct: %lu, nt: %lu, bsct: %lu ssct: %ld   +++++++++++++++++++ \n",
				pAcqReferenceData->ct,pAcqReferenceData->nt,pAcqReferenceData->bsct,pAcqReferenceData->ssct);
            p2ReferenceLocation = p2CurrentWord - 1;

            smallPhaseWord = 0;

            /* calculation of ctss index rtvptr */
            i = pAcqReferenceData->nt;
            j = pAcqReferenceData->ssct;
            k = pAcqReferenceData->ct;
            pAcqReferenceData->rtvptr = ( i - (j % i) + k ) % i;


            /* oph phase cycling   1=no cycling,  0=quad detect */

            if (pAcqReferenceData->cpf)
              pAcqReferenceData->oph = 0 ;
            else
              pAcqReferenceData->oph = pAcqReferenceData->rtvptr ;

            DPRINT5(+0," Arg0: %d, Arg1: %d, Arg2: %d, Arg3: %d, Arg4: %d\n",
		*p2CurrentWord,*(p2CurrentWord+1),*(p2CurrentWord+2),*(p2CurrentWord+3),*(p2CurrentWord+4));

            p2CurrentWord += 5;

            if ((pAcqReferenceData->ct == 0) && (pAcqReferenceData->ssct <= 0) ) initvalFlag = 1;

#if defined(PFG_CNTLR) || defined(GRADIENT_CNTLR)

            peR_step=0;
            peP_step=0;
            peS_step=0;
            d_peR_step=0;
            d_peP_step=0;
            d_peS_step=0;

#endif

            /* popInLowCore(args); */
            break;

        case ENDOFSCAN:
            DPRINT4(+0, " ENDOFSCAN ++++++++++++++++++++ For ct: %lu, nt: %lu, bsct: %lu, ssct: %ld ++++++++++++++++ \n",
			pAcqReferenceData->ct, pAcqReferenceData->nt, pAcqReferenceData->bsct, pAcqReferenceData->ssct);
            /* flushCntrlFifoRemainingWords(); */
            i = *p2CurrentWord++;    /* ss */
            j = *p2CurrentWord++;    /* nt */
            k = *p2CurrentWord++;    /* bs */
            l = *p2CurrentWord++;    /* fsval */

            initvalFlag = 0;

            pAcqReferenceData->rtonce = 1;     /* rtonce rtvar set to 1 to disable further actions */

            /*  j is the arrayed valued of nt. we need this value to be sent to endofScan */

#ifdef MASTER_CNTLR
           /* set the Console_Stat structure with configured & active channnel bits */
           if (! channelBitsSet)
           {
              pCurrentStatBlock->AcqChannelBitsConfig1 = *p2CurrentWord++;
              pCurrentStatBlock->AcqChannelBitsConfig2 = *p2CurrentWord++;
              pCurrentStatBlock->AcqChannelBitsActive1 = *p2CurrentWord++;
              pCurrentStatBlock->AcqChannelBitsActive2 = *p2CurrentWord++;
              channelBitsSet = 1;
           }
           else
              p2CurrentWord += 4;
#else
           p2CurrentWord += 4;
#endif

            endofScan( j, pAcqReferenceData->bs,
                  &(pAcqReferenceData->ssct), &(pAcqReferenceData->ct),
                  &(pAcqReferenceData->bsct), l);

            if (dmpRtVarsFlg > 0)
            {
               printf("ENDOFSCAN: for fid: %lu, ct: %lu \n",  curAcodeNumber, pAcqReferenceData->ct);
               dumpRtVars();
            }

            if (BrdType != DDR_BRD_TYPE_ID)  /* Master */
            {
               parseAheadDecrement(WAIT_FOREVER);   /* pend parser max of 10 sec then times out */
            }
            break;

#ifndef DDR_CNTLR
       case SEND_ZERO_FID:
            i = *p2CurrentWord++;   
            i = *p2CurrentWord++;   
            break;
#endif

            /*
             * NEXTCODESET removes the last acode set and
             * acquires the next one. There is some interleave
             * management. If il is on popd lc to storage.
             */
        case NEXTCODESET:
            DPRINT2(+0, " NEXTCODESET: present: %d, next: %d\n",curAcodeNumber,(curAcodeNumber+1));
            /*
             * release this code buffer - cannot pend if il on
             * pop the lc to pool -> could pend (better not??)
             */
//#ifdef DDR_CNTLR
            scheduleCntrlFifoRemainingWords();
            /* flushCntrlFifoRemainingWords(); can cause fifo filling interrupts to happen too quickly if # in buf too small */
//#endif
            DPRINT2(+2,"A32Interp(): il_flag: %d , Acode residentFlag: %d \n",il_flag, residentFlag);
            /* if interleaving and all the Acodes are resident in memory, do NOT delete them,
               otherwise delete them since they will be sent again */
            if ((il_flag == 1) && (residentFlag == 1) )
            {
                DPRINT2(+2,"NEXTCODESET: ct: %ld , nt: %ld \n",pAcqReferenceData->ct, pAcqReferenceData->nt );
                if ( pAcqReferenceData->ct == pAcqReferenceData->nt)
                {
                   markAcodeSetDone(expName, curAcodeNumber);
                   rmAcodeSet(expName, curAcodeNumber);
                   newFid = 1;   /* results in interrupt taht increments FID number to status */
                }
            }
            else
            {
                 DPRINT2(+2,"NEXTCODESET: delete: '%s': fid %d \n",expName, curAcodeNumber );
                 markAcodeSetDone(expName, curAcodeNumber);
                 rmAcodeSet(expName, curAcodeNumber);
                 newFid = 1;   /* results in interrupt taht increments FID number to status */
            }
            curAcodeNumber++;   /* needs il handlers */

            pAcodeId->cur_acode_set = curAcodeNumber;
            pAcodeId->cur_acode_base = pAcodeId->cur_jump_base = ac_base = p2CurrentWord =
		getAcodeSet(pAcodeId->id,pAcodeId->cur_acode_set, &(pAcodeId->cur_acode_size), 1 /* timeout sec */);
            if (ac_base == NULL)
            {
                markAcodeSetDone(expName, curAcodeNumber);
                errorcode = SYSTEMERROR+NOACODESET;
                return (errorcode); /* for the READER ONLY */
            }

            DPRINT1(+0,"A32Interp(): pAcodeId->cur_acode_size: %d bytes\n",pAcodeId->cur_acode_size);
                    p2EndOfCodes = pAcodeId->cur_acode_base + (pAcodeId->cur_acode_size / sizeof(int));

            if (curAcodeNumber == 1)  /* start of sequence, 0 = presequence stuff */
            {
              /* zero FIFO instruction Instruction count register */
              cntrlClearInstrCountTotal();  /* clear Instruction FIFO counter register */
              /* execFunc("clrFifoInstrCount",NULL, NULL, NULL, NULL, NULL); */
              cntrlClrCumDmaCnt();  /* zero sw cumlutive DMA count to FIFO */
            }

#ifdef  DURATION_DECODE
            if (decodeFifoWordsFlag > 0)
            {
              execFunc("prtFifoTime","NEXTCODESET", NULL, NULL, NULL, NULL,NULL,NULL,NULL);
              execFunc("clrFidTime",NULL, NULL, NULL, NULL, NULL,NULL,NULL,NULL);
            }
#endif

            initvalFlag = 1;
            break;

            /*
             * END_PARSE
             *
             * Causes the parser to return to idle or start
             * function.
             */
        case END_PARSE:
             // dumpStatBlocks( 1 );
	          i = *p2CurrentWord++;
            DPRINT1(-1, " END_PARSE %d\n",i);
            if ( i != 0 )
               signal_syncop(SETUP_CMPLT,-1L, -1L);
            else
               signal_syncop(EXP_COMPLETE,-1L, -1L);

            /* writeCntrlFifoWord(LATCHKEY | DURATIONKEY | 0); */
            flushCntrlFifoRemainingWords();
            if ( ! cntrlFifoRunning() )
            {
              // TSPRINT("Not running so start fifo, 166 ms delay:");
	           taskDelay(calcSysClkTicks(166));   /* taskDelay(10) */     // original delay
               startFifo4Exp();   /* startCntrlFifo(); */
            }
            markAcodeSetDone(expName, curAcodeNumber);
            rmAcodeSet(expName, curAcodeNumber);
            errorcode = 0;
#ifdef  DURATION_DECODE
            if (decodeFifoWordsFlag > 0)
            {
               execFunc("prtFifoTime","END_PARSE", NULL, NULL, NULL, NULL,NULL,NULL,NULL);
            }
#endif
            // TSPRINT("Returning to AParse:");
            return (errorcode); /* for the READER ONLY */
            break;


	    /* indicate that acquisition is in the Interleave mode */
       case IL_MODE:
	    il_flag = *p2CurrentWord++;
            pAcqReferenceData->il_incr = 0;
            DPRINT1(+2,"IL_MODE: mode: %d\n",il_flag);
            break;

       case ILCJMP:
	         /* must reset fid count and ct for acodes interpreted already */
                 /* for the status panel */
                 /* fid_count = ??; */
                 /* fid_ct = ?? ; */
                 /* p2CurrentWord = p2ReferenceLocation;    this is a JUMP */

                /* removed for now, fixed with change in PSG using FRTINIT for */
                 /* peloop & msloop imaging issues */
                /* initvalFlag = 1;   /* make sure RTINIT does set rt vals */

	             i = *p2CurrentWord++;
                DPRINT2(+2,"ILCJMP: ct: %ld , nt: %ld \n",pAcqReferenceData->ct, pAcqReferenceData->nt );
                if ( pAcqReferenceData->ct == pAcqReferenceData->nt)
	                 break;

                pAcqReferenceData->il_incrBsCnt = pAcqReferenceData->bsct;

                /* for interleave acquisition when all the Acodes are resident in memory, do NOT delete them,
                   otherwise delete them since they will be sent again */
                if (residentFlag == 0)
                {
                    DPRINT2(+2,"ILCJMP: delete: '%s': fid %d \n",expName, curAcodeNumber );
                    markAcodeSetDone(expName, curAcodeNumber);
                    rmAcodeSet(expName, curAcodeNumber);
                    /* newFid = 1;   /* results in interrupt that increments FID number to status */
                }
	             curAcodeNumber = 1;

                pAcodeId->cur_acode_set = curAcodeNumber;
                pAcodeId->cur_acode_base = pAcodeId->cur_jump_base = ac_base = p2CurrentWord =
		              getAcodeSet(pAcodeId->id,pAcodeId->cur_acode_set, &(pAcodeId->cur_acode_size), 1 /* timeout sec */);
                if (ac_base == NULL)
                {
                  markAcodeSetDone(expName, curAcodeNumber);
                  errorcode = SYSTEMERROR+NOACODESET;
                  return (errorcode); /* for the READER ONLY */
                }

                DPRINT1(+2,"A32Interp(): pAcodeId->cur_acode_size: %d bytes\n",pAcodeId->cur_acode_size);
                    p2EndOfCodes = pAcodeId->cur_acode_base + (pAcodeId->cur_acode_size / sizeof(int));

                /* increment the parsed interleave cycle count */
		          pAcqReferenceData->il_incr++;

            break;


            /*
             * SETVT  mode, temp,  options
             *
             * does not cause the parsing to pend
             * It is put in the general part because master fifo must stop,
             * temp can be arrayed, so all others must stop as well.
             * All controllers get the same SETVT acode, this way we can tell
             * *why* a xmtr or rcvr stopped
             */
        case SETVT:
            queueSetVT(p2CurrentWord, pAcodeId);
            p2CurrentWord += 5;
            break;

            /*
             * WAIT4VT errorcode, time to wait
             *
             * This element waits a variable time for the VT unit to
             * regulate and stabilize. It may be given a fixed time to
             * succeed. If it fails, errorcode tells which class of
             * error to send.
             */
        case WAIT4VT:
            queueWait4VT(p2CurrentWord, pAcodeId);
            p2CurrentWord += 2;
            break;

            /*
             * SETSPIN speed, option1, option2
             *
             * Set the spin speed in Hz with options.TBD.
             */
	case SETSPIN:
	    queueSetSpin(p2CurrentWord, pAcodeId);
            p2CurrentWord += 3;
            break;
            /*
             * CHECKSPIN delta, errodes, flag
             *
             * Verify spin is within delta of set point.TBD.
             */
	case CHECKSPIN:
	    queueCheckSpin(p2CurrentWord, pAcodeId);
            p2CurrentWord += 3;
            break;



            /*
             * LOCKAUTO    mode,  maxpower, maxgain
             *
             * Does the auto lock functionality.  Mode contains
             * switches for the algorithm. Maxpower and maxgain
             * restrict the ranges the algorithm may use. The
             * auto lock suspends all parsers until complete.
             */
        case LOCKAUTO:
            {
                 DPRINT(-1, "auto lock ...\n");
                 /* vwacq was: if ( (int) rt_tbl[ *ac_cur ] == 0) */
                 /* arg1 = lockmode
                  * arg2 = max lock power
                  * arg3 = max lock gain
                 */
                 if ( pAcqReferenceData->ct == 0)
                 {
                     queueAutoLock(p2CurrentWord,pAcodeId);
                 }
                 p2CurrentWord += 3;
            }
           break;

        case SHIMAUTO:
           {
             int whenshim,shimmode,len;
             int tmpSampleHasChanged = 1;

             DPRINT(-1, "auto shim ...\n");
             whenshim = *p2CurrentWord++;
             shimmode = *p2CurrentWord++;
             len = *p2CurrentWord++;
             if ( ( shimmode & 1 ) &&
                chkshim((int)whenshim,(int) pAcqReferenceData->ct, &tmpSampleHasChanged) )
             {
#ifdef MASTER_CNTLR
                setShimMethod(p2CurrentWord, len);
#endif
                queueAutoShim(pAcodeId);
             }
             p2CurrentWord += len;
           }
           break;

#ifdef DDR_CNTLR

        case INITDDR:
            {
              unsigned int *start=p2CurrentWord;
              int i = *p2CurrentWord++;  /* number of integer arguments to follow */

              DPRINT1(0,"INITDDR: args = %d\n",*p2CurrentWord);

              fidDim = (unsigned long)  *p2CurrentWord++;
              fidBuffs = (unsigned long)  *p2CurrentWord++;
              fidBuffSize = (unsigned long) *p2CurrentWord++;
              fidNF=(unsigned long) *p2CurrentWord++;
              fidNFMod=(unsigned long) *p2CurrentWord++;
              DPRINT5(-1,"INITDDR: fids:%d fidBuffs:%lu fidBuffSize:%lu NF:%lu NFMOD:%lu\n",
                  fidDim,fidBuffs,fidBuffSize,fidNF,fidNFMod);

// TSPRINT("dataInitial() start:");
	          /* initialize the dataObj, to asllocat the fidstablock for acquisition and uplink */
              if (dataInitial(pTheDataObject, fidBuffs, fidBuffSize)  == ERROR)
              {
                   errorcode = HDWAREERROR + INVALIDACODE;
                   return (errorcode);
              }
// TSPRINT("dataInitial() cmplt:");
              ddr_set_exp(start);
// TSPRINT("dataInitial() ddr_set_exp:");
              p2CurrentWord = start + i;
            }
            break;

        case NEXTSCANDDR:
        {
            int np,nt,ct,bs,ix,cf,newblock=0,args;
            int mode,clrData;
            int endct=0;
            int rem_ct,ddr_oph;
            int psonTicks,cnt;
            int mask,gates,pinSyncGates;
            double recvPhase,freqOffset,phaseStep;
            int instwords[10];

            args = *p2CurrentWord++;  /* number of integer arguments to follow */

            /* -------------------------  nextscan params -------------------*/
            ix = *p2CurrentWord++;
            /* ------------------------  DDR params ------------------ */
            mode = *p2CurrentWord++;
            pinSyncGates = *p2CurrentWord++;
            psonTicks = *p2CurrentWord++;
            i = 0;
            freqOffset = unpackd(p2CurrentWord, &i);
            phaseStep = unpackd(p2CurrentWord, &i);
            p2CurrentWord += i;

            cf=pAcqReferenceData->cfct;  // init to 0 in lc. update here
            ct=pAcqReferenceData->ct;    // init to 0 in lc. update ENDOFSCAN
            bs=pAcqReferenceData->bs;    // init in lc
            nt=pAcqReferenceData->nt;    // set in INITSCAN
            np=pAcqReferenceData->np;    // set in INITSCAN

            parseAheadDecrement(WAIT_FOREVER);   /* pend parser max of 10 sec then times out */

            /* ------------------------------------------------------------*/

            skip = ( pAcqReferenceData->ssct > 0); /* true if ss active */

            if(!skip && avar>=0)
                skip=((int)rtVar[avar]>0);

            if(avar>=0){
                DPRINT1(-1,"NEXTSCANDDR:  vvvvvvvvvvvvvvvvvvvvvv acqval=%d\n",(int)rtVar[avar]);
            }
            else if(skip){
                DPRINT1(-1,"NEXTSCANDDR:  vvvvvvvvvvvvvvvvvvvvvv sscnt=%d\n",pAcqReferenceData->ssct);
            }
            else{
            	DPRINT1(-1,"NEXTSCANDDR:  vvvvvvvvvvvvvvvvvvvvvv args=%d\n",args);
            }

           // PINSYNC_DDR

            if (skip){ // just put out a delay for pinsync and exit
                mask = DDR_PINS|DDR_SCAN|DDR_FID|DDR_ACQ; // make sure these are off
                gates = pinSyncGates&(~mask);
                cnt = fifoEncodeGates(0, mask, gates, &(instwords[0]));
                cnt = cnt + fifoEncodeDuration(1, psonTicks,&(instwords[cnt]));
                cnt = cnt + fifoEncodeGates(0, mask, 0, &(instwords[cnt]));
                writeCntrlFifoBuf((int*) instwords, cnt);
                DPRINT4(+1,"mask: 0x%lx gates: 0x%lx, cnt: %d, instwrd: 0x%lx\n",
                	mask,gates,cnt,instwords[0]);

                if(pAcqReferenceData->ssct > 0){
                    DPRINT(-1,"NEXTSCANDDR: ^^^^^^^^^^^^^^^ early exit <sscnt>\n");
                }
                else{
                    DPRINT(-1,"NEXTSCANDDR: ^^^^^^^^^^^^^^^ early exit <acqvar>\n");
                }
                break;
            }
            pAcqReferenceData->cfct = cf = (cf % fidNF) ? cf : 0;

            DPRINT6(0,"NEXTSCANDDR:  np:%d nt:%d ct:%d bs:%d ix:%d cf:%d\n",
			      np,nt,ct,bs,ix,cf);
            newblock = nextScan(pAcodeId, ix, fidNF, fidNFMod, cf, fidBuffSize, &srctag, &dsttag);
            if ( (mode & EXP_CLR_DATA) || (il_flag == 1))
                dsttag=srctag;

            DPRINT1(0,"NEXTSCANDDR:  newblock: %d\n",newblock);

            //{   // Diagnostic for SEND_ZERO_FID
            //   FID_STAT_BLOCK *pStatBlk;
            //  pStatBlk =  dataGetStatBlk(pTheDataObject, srctag);
            //  DPRINT4(-7,"NEXTSCANDDR:  ix: %ld, Fid: %ld, Tag: %ld, dsttag: %d\n", ix, pStatBlk->elemId, srctag,dsttag);
            //}
            /* ------------------------------------------------------------*/

	        /* this will work for C but not JPSG */
            /* will need totalfids passed to console */

            DPRINT2(0," ct: %d, nt: %d\n",pAcqReferenceData->ct+1,nt);

            mode |= ( (ix == fidDim)
                    && ( pAcqReferenceData->ct == (nt-1) )
                    && (cf == fidNF-1) ) ? EXP_LAST_SCAN : 0 ;

            DPRINT6(0,"NEXTSCANDDR: ix %d:%d ct %d:%d cf %d:%d\n",
                               ix,fidDim,pAcqReferenceData->ct,nt-1,cf,fidNF-1);

            if(cf==0 && pAcqReferenceData->ct==0)
                mode|=EXP_NEWFID;

            DPRINT4(-1,"NEXTSCANDDR: mode: 0x%x src: %d dst: %d nt: %d\n",
                               mode,srctag,dsttag,nt);

            rem_ct = nt - pAcqReferenceData->ct;

            if (newblock){
                DPRINT3(+0,"NEXTSCANDDR: nt: %ld, ct: %ld, remaining ct: %ld\n",
                    nt,pAcqReferenceData->ct,rem_ct);
                if (bs > 0)
                    endct = (rem_ct < bs) ? rem_ct : bs;
                else
                    endct = nt;
            }
            if(!ddrstarted || lastf!=freqOffset){
                ddr_set_rf(freqOffset);
                lastf=freqOffset;
            }

            if(cf == 0)
                DPRINT3(-1," NEXTSCANDDR:  ct %d started src:%ld dst:%ld\n",
                pAcqReferenceData->ct,srctag,dsttag);

            DPRINT3(-1," NEXTSCANDDR:  FidNum: %ld,  srctag: %ld, dsttag: %ld \n", ix, srctag, dsttag);
            if(!ddrstarted || lastsrc!=srctag || lastdst!=dsttag || endct!=lastnt){
                ddr_set_acm(srctag, dsttag, endct);
                lastsrc=srctag;
                lastdst=dsttag;
                lastnt=endct;
                DPRINT3(+0," NEXTSCANDDR:  +++>> ddr_set_acm(%ld,%ld,%ld)\n",srctag,dsttag,endct);
            }

            // calculate receiver phase

            ddr_oph=(pAcqReferenceData->oph)%4;

            recvPhase = ddr_oph * 90.0;
            DPRINT1(+1," NEXTSCANDDR:  ddr_oph=%g\n",recvPhase);

            if(pvar>=0){
                 recvPhase+=rtVar[pvar]*phaseStep;
                 DPRINT3(+1," NEXTSCANDDR:  pvar(%d)*pstep(%g)=%g\n",rtVar[pvar],phaseStep,rtVar[pvar]*phaseStep);
            }

            if(!ddrstarted || lastp!=recvPhase){
                ddr_set_rp(recvPhase);
                lastp=recvPhase;
                DPRINT1(+0," NEXTSCANDDR:  +++>> ddr_set_rp(%lf)\n",recvPhase);
            }
            DPRINT2(+0," NEXTSCANDDR:  +++>> ddr_push_scan(%ld,0x%0.8X)\n",srctag,mode);
            ddr_push_scan(srctag,mode);
            pAcqReferenceData->cfct++;

            // PINSYNC_DDR
            // add the cf == 0 to test, so that when nfmod is less than nf only the 1st of the multiple blocks is counted
            // as a new FID,  otherwise the FID & CT count go weird.  BUG 7755  GMB 11/12/09
            if ( (newFid && pAcqReferenceData->cfct==1) || ((newblock == 1) && (il_flag == 1) && (cf == 0)) )
            {
                mask = gates = (DDR_FID | pinSyncGates);
                // DPRINT(-10," NEXTSCANDDR:  Assert DDR_FID\n");
                // DPRINT5(-10," NEXTSCANDDR: if( newFid: %d == 1 &&  cfct %d == 1) || ( newblock %d == 1 && il_flag %d == 1 && cf %d == 0) \n",
                //       newFid, pAcqReferenceData->cfct, newblock, il_flag, cf );
                // DPRINT3(-10,"NEXTSCANDDR: fidnum:%d, nf: %d. nfmod: %d\n", ix, fidNF,fidNFMod);
                // DPRINT6(-10,"NEXTSCANDDR: np:%d nt:%d ct:%d bs:%d ix:%d cf:%d\n",
			       //     np,nt,ct,bs,ix,cf);
                // DPRINT6(-10,"NEXTSCANDDR: ix %d:%d ct %d:%d cf %d:%d\n",
                //                ix,fidDim,pAcqReferenceData->ct,nt-1,cf,fidNF-1);

            }
            else {
                if (pAcqReferenceData->cfct == 1)
                {
                    mask = gates = (DDR_SCAN | pinSyncGates);
                    // DPRINT(-10," NEXTSCANDDR:  Assert DDR_SCAN\n");
                    // DPRINT3(-10,"NEXTSCANDDR: fidnum:%d, nf: %d. nfmod: %d\n", ix, fidNF,fidNFMod);
                    // DPRINT6(-10,"NEXTSCANDDR:  np:%d nt:%d ct:%d bs:%d ix:%d cf:%d\n",
			           //         np,nt,ct,bs,ix,cf);
                    // DPRINT6(-10,"NEXTSCANDDR: ix %d:%d ct %d:%d cf %d:%d\n",
                    //             ix,fidDim,pAcqReferenceData->ct,nt-1,cf,fidNF-1);
                }
                else
                    mask = gates = pinSyncGates;
	        }
            DPRINT3(+0,"NEXTSCANDDR: newFid: %d, gates: 0x%lx, pson delay: %d ticks\n",
                      newFid,pinSyncGates,psonTicks);
            cnt = fifoEncodeGates(0, mask, gates, &(instwords[0]));
            DPRINT3(+1,"mask & gates: 0x%lx, cnt: %d, instwrd: 0x%lx\n",gates,cnt,instwords[0]);
            cnt = cnt + fifoEncodeDuration(1, psonTicks,&(instwords[cnt]));
            DPRINT2(+1,"cnt: %d, dur instwrd: 0x%lx\n",cnt,instwords[1]);
            cnt = cnt + fifoEncodeGates(0, mask, 0, &(instwords[cnt]));
            writeCntrlFifoBuf((int*) instwords, cnt);
            newFid = 0; // clear newFid flag after first pinsync in fid

            // ARMDDR
            if(!ddrstarted){
                ddrstarted=1;
                DPRINT1(+0,"NEXTSCANDDR:  +++>> ddr_start_exp(0x%X)\n",mode);
                ddr_start_exp(mode);
            }
            DPRINT(-1," NEXTSCANDDR:  ^^^^^^^^^^^^^^^^^^^ exit\n");
        }
	    break;

       case SEND_ZERO_FID:
            {
               int np,nt,ct,bs,ix,cf,newblock=0,args;
               int skip,syncFlag;
               int rem_ct,ddr_oph;
                int endct=0;
               FID_STAT_BLOCK *pStatBlk;

	            ix = *p2CurrentWord++;  
	            syncFlag = *p2CurrentWord++;  

               cf=pAcqReferenceData->cfct;  // init to 0 in lc. update here
               ct=pAcqReferenceData->ct;    // init to 0 in lc. update ENDOFSCAN
               bs=pAcqReferenceData->bs;    // init in lc
               nt=pAcqReferenceData->nt;    // set in INITSCAN
               np=pAcqReferenceData->np;    // set in INITSCAN

               skip = ( pAcqReferenceData->ssct > 0); /* true if ss active */

               if(!skip && avar>=0)
                   skip=((int)rtVar[avar]>0);

               if(avar>=0){
                   DPRINT1(-1,"SEND_ZERO_FID:  vvvvvvvvvvvvvvvvvvvvvv acqval=%d\n",(int)rtVar[avar]);
                   break;
               }
               else if(skip){
                   DPRINT1(-1,"SEND_ZERO_FID:  vvvvvvvvvvvvvvvvvvvvvv sscnt=%d\n",pAcqReferenceData->ssct);
                   break;
               }
               else{
            	   DPRINT2(-1,"SEND_ZERO_FID:  vvvvvvvvvvvvvvvvvvvvvv ix=%d, syncFlag=%d\n",ix,syncFlag);
               }

                
               pAcqReferenceData->cfct = cf = (cf % fidNF) ? cf : 0;
               newblock = nextScan(pAcodeId, ix, fidNF, fidNFMod, cf, fidBuffSize, &srctag, &dsttag);
               DPRINT1(-1,"SEND_ZERO_FID:  newblock %d\n", newblock);
               if (newblock == 1)
               {
                  int signal4_syncop(int acode, int tagword);

                  rem_ct = nt - pAcqReferenceData->ct;
                  if (bs > 0)
                      endct = (rem_ct < bs) ? rem_ct : bs;
                  else
                      endct = nt;

                  if( lastsrc!=srctag || lastdst!=dsttag || endct!=lastnt)
                  {
                      lastsrc=srctag;
                      lastdst=dsttag;
                      lastnt=endct;
                      DPRINT3(+0," SEND_ZERO_FID:  +++>> srctag:%ld, dsttag: %ld, endct: %ld\n",srctag,dsttag,endct);
                  }
                  pAcqReferenceData->cfct++;

                  pStatBlk =  dataGetStatBlk(pTheDataObject, srctag);
                  pStatBlk->doneCode = EXP_FIDZERO_CMPLT;  // If recieve proc needs an indicator
                  pStatBlk->errorCode = 0;
                  // pStatBlk->dataSize = 0;
                  pStatBlk->fidAddr =  (int *) -1;
                  DPRINT4(-1,"SEND_ZERO_FID:  ix: %ld, Fid: %ld, Tag: %ld, sync: %d\n", ix, pStatBlk->elemId, srctag,syncFlag);
                  if (syncFlag)
                  {
                     signalz_syncop(SEND_ZERO_FID,srctag);  // send it Sync'd with DDR FIFO time-line
                  }
                  else
                  {
                     SendZeroFid(srctag);  // just send it now
                  }
               }

            }
            break;

            /*
             * NEXTCODESET removes the last acode set and
             * acquires the next one. There is some interleave
             */
    case PINSYNC_DDR:  // called by inactive ddrchannels
	    j = *p2CurrentWord++;  /* channel id */
	    DPRINT1(-1,"NEXTSCANDDR:  inactive channel %d\n",j);
        parseAheadDecrement(WAIT_FOREVER);   /* pend parser max of 10 sec then times out */
        break;

    case SETACQ_DDR:  // set or clear DDR_ACQ gate
        {
        int instwords[4];
	    int state,mask=DDR_ACQ,gate,cnt=0;
	    state = *p2CurrentWord++;  /* on or off */
	    if(j==0){
	        DPRINT(0,"SETACQ_DDR: clearing DDR_ACQ\n");
	        gate=0;
	    }
	    else{
	        if(skip){
	            DPRINT(0,"SETACQ_DDR: set DDR_ACQ called in dummy scan <aborting>\n");
	            break;
	        }
	        DPRINT(0,"SETACQ_DDR:  setting DDR_ACQ\n");
	        gate=DDR_ACQ;
	    }
	    cnt = fifoEncodeGates(0, mask, gate, &(instwords[0]));
        writeCntrlFifoBuf((int*) instwords, cnt);
        DPRINT4(1,"mask: 0x%lx gates: 0x%lx, cnt: %d, instwrd: 0x%lx\n",mask,gate,cnt,instwords[0]);
	    }
        break;

     case SETPVAR_DDR:  /* rcvrphase(phaseptr)  */
	    pvar = *p2CurrentWord++;
	    DPRINT1(-1,"SETPVAR_DDR:  rcvrphase(var) var=%d\n",pvar);
        break;

     case SETAVAR_DDR:  /* set ddr acquire variable */
	    avar = *p2CurrentWord++;
	    DPRINT1(-1,"SETAVAR_DDR:  setacqvar(var) var=%d\n",avar);
        break;

	case RG_MOD_DDR:
	    j = *p2CurrentWord++;  /* nloops */
        k = *p2CurrentWord++;  /* gate on command */
        l = *p2CurrentWord++;  /* gate on duration */
        m = *p2CurrentWord++;  /* gate off command */
        n = *p2CurrentWord++;  /* gate off duration */
        for (i = 0; i < j; i++)
	    {
               writeCntrlFifoWord(k);
               writeCntrlFifoWord(l);
               writeCntrlFifoWord(m);
               writeCntrlFifoWord(n);
	    }
	  break;

#endif

#ifdef RF_CNTLR
        case TEMPCOMP:
                p2CurrentWordStart=p2CurrentWord;
        	i = *p2CurrentWord++;
                DPRINT1(-1,"TEMPCOMP: AUX value: %d\n",i);
        	if ( getRfType() == 1){  // am I an ICAT ?
		     j = (LATCHKEY | AUX | AUXTCOMP | (0x3 & i));
		     DPRINT2(-1,"ICAT TEMPCOMP=%d  0x%X\n",i,j);
		     writeCntrlFifoBuf(&j,1);
        	}
                break;

        case AMPTBLS:
        	p2CurrentWordStart=p2CurrentWord;
        	use_tbls = *p2CurrentWord++;  // tells the fpga whether or not to do the correction
        	if(use_tbls>0){
				tbl_size = *p2CurrentWord++;
				num_tbls = *p2CurrentWord++; // new tables to download
				DPRINT2(-1, "AMPTBLS num:%d length:%d\n",num_tbls,tbl_size);
				// 1. enable fpga amplifier correction
		  		*fpga_linearization_enable = 1;
		  		// 2. download and set fpga active tpwr mapping table
		  		for (i=0;i<256; i++){
		  			j=*p2CurrentWord++;
		  			fpga_atten_mapping[i]=j;
		  			DPRINT2(2, "LUT[%d] = %d\n",i,j);
		  		}
		  		// 2. download and set tpwr->tpwr mapping table
		  		for (i=0;i<256; i++){
		  			j=*p2CurrentWord++;
		  			fpga_tpwr_map_tbl[i]=j;
		  			DPRINT2(2, "TPWR[%d] = %d\n",i,j);
		  		}
		  		// 3. download and set tpwr scale table
		  		for (i=0;i<256; i++){
		  			j=*p2CurrentWord++;
		  			fpga_tpwr_scale_tbl[i]=j;
		  			DPRINT2(2, "SCALE[%d] = %g\n",i,((float)j)/0x10000);
		  		}
				// 4. download and set fpga correction tables
				//  - tables range from tpwr_min*2 to tpwr_max*2 with:
				//    tpwr=31.5:63 mapping to tables 0:63  tpwr*2=63:126
				//  - map all tpwr values <= 32 to table 0  (tpwr*2 values <= 63)
				//  - map all tpwr values between 32 and 63 to tables 0..63
				//
				for(i=0;i<num_tbls;i++){
					unsigned int word,amp;
					int phs;
					for(j=0;j<tbl_size;j++){
						word=(unsigned int)*p2CurrentWord++;
						amp=word>>16;
						amp=amp&0xffff;
						phs=word&0xffff;// 0..65535
						phs=phs-0x8000; // convert to signed value +-32768
						fpga_amp_table[i*tbl_size+j] =(unsigned int)amp; // write to the fpga ampl correction buffer
						fpga_phase_table[i*tbl_size+j]=(int)phs;         // write to the fpga phase correction buffer
					}
					DPRINT5(1, "AMPTBL[%d][%d] word:0x%X ampl:%g phase:%g\n",
						i,j-1,word,((double)amp)/65535,((double)phs*180.0)/32768);
				}
		}
    		else
    		  *fpga_linearization_enable = 0; // disable amplifier linearization
                  DPRINT1(-1, "AMPTBLS rcvd %d acode values\n",
			  (unsigned long)(p2CurrentWord-p2CurrentWordStart));

                break;

        case ADVISE_FREQ:
                AdvisedFreq++;
                i = *p2CurrentWord++;
                DPRINT1(2,"ADVISE_FREQ: %d tuning words expected\n", i);
                j = adviseFreq(p2CurrentWord, i); // adjust using synthesizer calibrations
                p2CurrentWord += j;
                DPRINT1(0,"ADVISE_FREQ: %d tuning words received\n", i);
                break;

        case FORTH:
           /* 
	     * Evaluate a forth expression 
	     */
           {
                char *cmd = (char*) p2CurrentWord;
                p2CurrentWord += strlen(cmd);
                DPRINT1(0,"FORTH: %s\n", i);
                forth(cmd);
                break;
           }

           /*
             * VRFAMP         RTvar, scalefactor VRFAMPS
             * RTvar, scalefactor VRFPHASE       RTvar,
             * scalefactor VRFPHASEC      RTvar, scalefactor
             *
             * These elements compute the value of RTvar *
             * scalefactor and place the result in the amplitude,
             * amplitude scale, phase, or phase cycle register.
             * The value is clipped at 16 bits.
             */
        case VRFAMP:
            DPRINT(-1, " VRFAMP\n");
            i = *p2CurrentWord++;
            k = *p2CurrentWord++;
            DPRINT1(-4, "VRFAMP TPWRF = %d\n", ((rtVar[k] * i) & 0xffff));
            writeCntrlFifoWord(RFAMPKEY | ((rtVar[k] * i) & 0xffff));
            break;

        case VRFAMPS:
            DPRINT(-1, " VRFAMPS\n");
            i = *p2CurrentWord++;
            k = *p2CurrentWord++;
            DPRINT1(-4, "VRFAMPS TPWR = 0x%x\n", ((rtVar[k] * i) & 0xffff));
            writeCntrlFifoWord(RFAMPSCALEKEY | ((rtVar[k] * i) & 0xffff));
            break;

            /*
             * vphase group - compute the phase from a real time
             * variable vphase/vphaseq are present for
             * completeness only since the RFPHASEKEY is the
             * "pattern input" to the math layer. vphaseC.
             * vphasecq are the standard usages. vphasec,
             * vphasecq are the active elements for small angle
             * and quadrature respectively.
             */

        case VPHASE:
            i = *p2CurrentWord++;
            k = *p2CurrentWord++;
            DPRINT3(+1, " VPHASE resolved to v[%d]*0x%x = 0x%x\n", k, i, (rtVar[k] * i) & 0xffff);
            writeCntrlFifoWord(RFPHASEKEY | ((rtVar[k] * i) & 0xffff));
            break;

            /* multiplies an rtVar by 90 degrees.  */
        case VPHASEQ:
            i = *p2CurrentWord++;
            l = rtVar[i] * 0x4000;
            DPRINT2(+1, " VPHASE resolved to v[%d]*0x4000 = 0x%x\n", k, l & 0xffff);
            writeCntrlFifoWord(RFPHASEKEY | ((rtVar[k] * i) & 0xffff));
            break;
            /* now with 2 ^ 31 precision */
        case VPHASEC:
            i = *p2CurrentWord++;
            k = *p2CurrentWord++;
            /* need a 64 bit accumulator */
            lscratch1.lword = (long long) i;
            lscratch1.lword *= (long long) rtVar[k];
	        lscratch1.lword += 0x7FFF; /* round */
            lscratch1.lword >>= 15; /* back to 360 == 65536 */
            l = lscratch1.nword[1] & 0xffff; /* modulo 360 */
            DPRINT3(1,"VPHASEC resolved to %s *0x%x = 0x%x\n",rt_descr[k],i,l);
            smallPhaseWord = l;
            DPRINT1(+1,"VPHASEC: phase word = %g\n", l*0.00549316);
            writeCntrlFifoWord(RFPHASECYCLEKEY | l );
            break;

        case VPHASECQ:
            i = *p2CurrentWord++;
            l = (rtVar[i] * 0x4000) & 0xffff;
            quadPhaseWord = l;
            l += smallPhaseWord;
            DPRINT2(1,"VPHASECQ: resolved to %s * 0x4000 = 0x%x\n",rt_descr[i],l);
            DPRINT1(1,"VPHASECQ: phase word = %g\n", l*0.00549316);
            writeCntrlFifoWord(RFPHASECYCLEKEY | (l & 0xffff));
            break;

        case SELECTRFAMP:
            i = *p2CurrentWord++;
            selectRFAmp(i);
            DPRINT1(-1, "SELECTRFAMP: called selectRFAmp() with arg %d\n",i);
            break;
            
      /*
          DECPROGON
        - pattern ID, duration of a cycle, duration of state,
          int wordsperstate, int offset
      */
        case DECPROGON:
	      i = *p2CurrentWord++;  /* patternID */
          lscratch1.nword[0] = *p2CurrentWord++; /* pattern ticks high word*/
          lscratch1.nword[1] = *p2CurrentWord++; /* pattern ticks low word */
          lscratch2.nword[0] = *p2CurrentWord++; /* time per state */
          lscratch2.nword[1] = *p2CurrentWord++;
          j = *p2CurrentWord++; /* words per state */
          k = *p2CurrentWord++; /* patoffset - unused?? */
          lscratch3.nword[0] = *p2CurrentWord++; /* skip value in ticks for single rep */
          lscratch3.nword[1] = *p2CurrentWord++;
          l = *p2CurrentWord++; /* number of replications */
          DecInfo.phasePerPass = *p2CurrentWord++;
          DecInfo.startingPhase = *p2CurrentWord++;
          DecInfo.savedPhase = (smallPhaseWord + quadPhaseWord) & 0xffff;
          progDecOn(i,(lscratch1.lword),(lscratch2.lword),j,k,(lscratch3.lword),l);

          break;

        case DECPROGOFF:
      /*
          DECPROGOFF
        - pattern id, policy, long long cycleInTicks
      */
          i = *p2CurrentWord++;  /* Pattern Id */
          j = *p2CurrentWord++;  /* Dec Policy */
          lscratch1.nword[0] = *p2CurrentWord++;
          lscratch1.nword[1] = *p2CurrentWord++;
          k = *p2CurrentWord++;  /* Sync Type */
          l = *p2CurrentWord++;  /* Async Scheme */

          if (k == 1)                   /* sync type */
          {
             status = progDecOff(i,j,(lscratch1.lword));
             if (status) return(status);  /* handle error */
          }
          else if (k == 2)              /* async type */
          {

             /* definitions:
                DecInfo.patTicks = no of ticks in the total pattern including reps
                DecInfo.skipTicks= (total ticks/(nt*numreps)) , so for a single rep
                lscratch1.lword  = no of ticks in decoupling duration
                lscratch2.lword  = no of ticks in pattern (in a single rep)
                decSkipTicks     = no of ticks to be skipped for i-th scan (in terms of single rep)
             */

             lscratch2.lword = (DecInfo.patTicks/DecInfo.numReps);

             if (l == 1)                /* progressive skip scheme */
             {
                decSkipTicks = DecInfo.skipTicks * pAcqReferenceData->ct;
             }
             else if (l == 2)           /* random scheme */
             {
                decSkipTicks = (lscratch2.lword * rand()) / 32767LL  ;
             }
             else if (l == 3)           /* bit reversal order */
             {
                /* use itemp=no of bits to reverse, n=ct, jtemp = bit reversed int */

                n     = pAcqReferenceData->ct;
                itemp = DecInfo.nBitsRev;

                jtemp = n&1;
                while( --itemp )
                {
                   n     >>= 1;
                   jtemp <<= 1;
                   jtemp  += n&1;
                }
                /* jtemp is the bit reversed int */
/*
                diagPrint(debugInfo,"nt=%d  nBitsRev=%d   ct=%d    BIT REVERSED INT = %d\n",(pAcqReferenceData->nt),DecInfo.nBitsRev,(pAcqReferenceData->ct),jtemp);
*/
                decSkipTicks = DecInfo.skipTicks * jtemp;
            }
            else
                errLogRet(LOGIT,debugInfo,"invalid decoupler asynchroziation scheme\n");


                if (decSkipTicks >= DecInfo.patTicks)
                   decSkipTicks = decSkipTicks % DecInfo.patTicks;

                DPRINT4(1,"DECPROGOFF async: %d  DecInfo.skipTicks=%lld  decSkipTicks=%lld  DecInfo.patTicks=%lld\n",l,DecInfo.skipTicks,decSkipTicks,DecInfo.patTicks);
                status = progDecSkipOff(i, j, (lscratch1.lword), decSkipTicks);

                if (status) return(status);
          }
          else
            errLogRet(LOGIT,debugInfo,"invalid decoupler synchronization type\n");
          break;

      /*
       *
          VLOOPTICKS
          long long ticksPerLoop
          this acode adds vTicks to obs/decprog decoupling when loops are
          enclosed in obs/decprog statements
      */

        case VLOOPTICKS:
          lscratch1.nword[0] = *p2CurrentWord++;
          lscratch1.nword[1] = *p2CurrentWord++;
          i = rtVar[nvLoopCntrInd[loopStkPos]];
          DPRINT1(+1,"VLOOPTICKS: args = %lld ticks\n",lscratch1.lword);
          if ( (i >= 1) && (DecInfo.progDecRunning == 1) )
          {
            DecInfo.vTicks += lscratch1.lword;
            DPRINT3(+1,"VLOOPTICKS: loopcounter=%d   added %lld ticks to DecInfo.vTicks total=%lld\n",i, lscratch1.lword, DecInfo.vTicks);
          }
          break;

        /*
         * TINFO   (#2) for RF_CNTLR
         * see TINFO below
         */
	case TINFO:
            rfInfo.ampHiLow  = *p2CurrentWord++;
            rfInfo.xmtrHiLow = *p2CurrentWord++;
            rfInfo.tunePwr   = *p2CurrentWord++;
	    DPRINT3(-1,"RFINFO band=%d, xmtr=%d, power=%d",
			rfInfo.ampHiLow,rfInfo.xmtrHiLow,rfInfo.tunePwr);
            break;

        case RFSPARELINE:
	    k = *p2CurrentWord++;  // get argument
	    DPRINT2(1,"RF GATE state = %x  command = %x\n",spareState,k);
	    l = k & 0x7;
	    m = (k >> 3) & 0x7;
	    spareState &= (~m);  // wipe out current bit(s)
	    spareState |= l;     // new state
	    DPRINT1(1,"RF GATE exit state = %x\n",spareState);
            writeCntrlFifoWord(RFUSERKEY | spareState);
	    break;
#endif


#ifdef PFG_CNTLR
            /*
             * PFGVGRADIENT arg1 - which one encoded as an FFKEY
             * value arg2 - rtvar to multiply arg3 - multiplier
             */

        case PFGVGRADIENT:
            DPRINT(-1, "PFG VGRADIENT\n");
            i = *p2CurrentWord++;
            j = *p2CurrentWord++;
            k = *p2CurrentWord++;
            writeCntrlFifoWord(i | (((rtVar[j]) * k) & 0xffff));
            break;

	case PFGENABLE:
	    DPRINT(-1,"PFG ENABLE");
	    /* reset too */
	    resetPfgAmps(); /* this might hang??? */
            pfg_AMP_enable(*p2CurrentWord++);
            break;


        case SETGRDROTATION:
            DPRINT(-1, "SETGRDROTATION");
            rotm_11 = *p2CurrentWord++;
            rotm_12 = *p2CurrentWord++;
            rotm_13 = *p2CurrentWord++;

            rotm_21 = *p2CurrentWord++;
            rotm_22 = *p2CurrentWord++;
            rotm_23 = *p2CurrentWord++;

            rotm_31 = *p2CurrentWord++;
            rotm_32 = *p2CurrentWord++;
            rotm_33 = *p2CurrentWord++;
            break;


        case SETVGRDROTATION:
            DPRINT(-1, "SETVGRDROTATION");
            pat_index = *p2CurrentWord++;   /* pattern id for rotation angle sets */
            j         = *p2CurrentWord++;   /* v-index */
            k         = rtVar[j];

            if ( k < 0 )
            {
               k = 0;
            }

            errorcode = getPattern(pat_index, &patternList, &patternSize);
            if (errorcode != 0)
            {
                errLogRet(LOGIT,debugInfo,
                        "SETVGRDROTATION: rotation list not found.\n");
                return (errorcode);
            }

            k = k % 262144; /* 262144 is the max number of rot sets in a single list id */

            /* check validity of the rtvar-index */
            if ((k*9+9) > patternSize)  /* 9 matrix elements */
            {
              errLogRet(LOGIT,debugInfo,
                   "SETVGRDROTATION: error in rotation list or list index.\n");
              return(SYSTEMERROR+GRD_ROTATION_LIST_ERR);
            }

            diagPrint(debugInfo,"\n");
            diagPrint(debugInfo,"pattern id = %d   rtvar name=%d   rtvar value=%d   index=%d\n",pat_index,j,rtVar[j],k);

            pIntW1 = patternList + k*9;  /* start addr of rtVar[j]-th rotation set */

            rotm_11 = *pIntW1++;
            rotm_12 = *pIntW1++;
            rotm_13 = *pIntW1++;

            rotm_21 = *pIntW1++;
            rotm_22 = *pIntW1++;
            rotm_23 = *pIntW1++;

            rotm_31 = *pIntW1++;
            rotm_32 = *pIntW1++;
            rotm_33 = *pIntW1++;

            diagPrint(debugInfo,"11=%d  12=%d  13=%d\n",rotm_11,rotm_12,rotm_13);
            diagPrint(debugInfo,"21=%d  22=%d  23=%d\n",rotm_21,rotm_22,rotm_23);
            diagPrint(debugInfo,"31=%d  32=%d  33=%d\n",rotm_31,rotm_32,rotm_33);
            break;


        case SETPEVALUE:
           DPRINT(-1, "SETPEVALUE");

           i = *p2CurrentWord++;
           if (i == 1)
           {
	    DPRINT4(-1,"SASSETPE %x %x  %x  %x\n",i,peR_step,peR_vind,peR_lim);
              peR_step = *p2CurrentWord++;
              peR_vind = *p2CurrentWord++;
              peR_lim  = *p2CurrentWord++;
           }
           if (i == 2)
           {
	    DPRINT4(-1,"SASSETREAD %x %x  %x  %x\n",i,peR_step,peR_vind,peR_lim);
              peP_step = *p2CurrentWord++;
              peP_vind = *p2CurrentWord++;
              peP_lim  = *p2CurrentWord++;
           }
           if (i == 3)
           {
	    DPRINT4(-1,"SASSETSLICE %x %x  %x  %x\n",i,peR_step,peR_vind,peR_lim);
              peS_step = *p2CurrentWord++;
              peS_vind = *p2CurrentWord++;
              peS_lim  = *p2CurrentWord++;
           }
           if (i == 4)
           {
	    DPRINT4(-1,"SASSETMYPE %x %x  %x  %x\n",i,d_peR_step,d_peR_vind,d_peR_lim);
              d_peR_step = *p2CurrentWord++;
              d_peR_vind = *p2CurrentWord++;
              d_peR_lim  = *p2CurrentWord++;
           }
           if (i == 5)
           {
	    DPRINT4(-1,"SASSETMYPHASE %x %x  %x  %x\n",i,d_peP_step,d_peP_vind,d_peP_lim);
              d_peP_step = *p2CurrentWord++;
              d_peP_vind = *p2CurrentWord++;
              d_peP_lim  = *p2CurrentWord++;
           }
           if (i == 6)
           {
	    DPRINT4(-1,"SASSETMYSLICE %x %x  %x  %x\n",i,d_peS_step,d_peS_vind,d_peS_lim);
              d_peS_step = *p2CurrentWord++;
              d_peS_vind = *p2CurrentWord++;
              d_peS_lim  = *p2CurrentWord++;
           }

           break;


	case OBLPEGRD:
           DPRINT(-1, "OBLPEGRD");

           i = *p2CurrentWord++;           // type

           G_r = *p2CurrentWord++;         // static part for Gr
           G_p = *p2CurrentWord++;         // static part for Gp
           G_s = *p2CurrentWord++;         // static part for Gs



           // do the phase encode & oblique rotation


           tempGx1=0;  tempGy1=0;  tempGz1=0;
           tempGx2=0;  tempGy2=0;  tempGz2=0;
           tempGx3=0;  tempGy3=0;  tempGz3=0;
           srotm_11=0; srotm_12=0; srotm_13=0;
           srotm_21=0; srotm_22=0; srotm_23=0;
           srotm_31=0; srotm_32=0; srotm_33=0;

           srotm_11 = (rotm_11*32767)>>16;
           srotm_21 = (rotm_21*32767)>>16;
           srotm_31 = (rotm_31*32767)>>16;

           srotm_12 = (rotm_12*32767)>>16;
           srotm_22 = (rotm_22*32767)>>16;
           srotm_32 = (rotm_32*32767)>>16;

           srotm_13 = (rotm_13*32767)>>16;
           srotm_23 = (rotm_23*32767)>>16;
           srotm_33 = (rotm_33*32767)>>16;

           /* Do the rotation of the static gradient part */

           statGx  = srotm_11*G_r + srotm_12*G_p + srotm_13*G_s ;
           statGy  = srotm_21*G_r + srotm_22*G_p + srotm_23*G_s ;
           statGz  = srotm_31*G_r + srotm_32*G_p + srotm_33*G_s ;

           statGx >>= 15;  statGy >>= 15;  statGz >>= 15;


           /* First: Gradient RO */

           if (peR_step != 0)
           {
               tempGx1 = ((srotm_11*peR_step +0x3fff)>>15)*rtVar[peR_vind];
               tempGy1 = ((srotm_21*peR_step +0x3fff)>>15)*rtVar[peR_vind];
               tempGz1 = ((srotm_31*peR_step +0x3fff)>>15)*rtVar[peR_vind];
           }

           /* Second: Gradient PE */

           if (peP_step != 0)
           {
               tempGx2 = ((srotm_12*peP_step +0x3fff)>>15)*rtVar[peP_vind];
               tempGy2 = ((srotm_22*peP_step +0x3fff)>>15)*rtVar[peP_vind];
               tempGz2 = ((srotm_32*peP_step +0x3fff)>>15)*rtVar[peP_vind];
           }

           /* Third: Gradient SS */

           if (peS_step != 0)
           {
               tempGx3 = ((srotm_13*peS_step +0x3fff)>>15)*rtVar[peS_vind];
               tempGy3 = ((srotm_23*peS_step +0x3fff)>>15)*rtVar[peS_vind];
               tempGz3 = ((srotm_33*peS_step +0x3fff)>>15)*rtVar[peS_vind];
           }

           Gx = tempGx1 + tempGx2 + tempGx3 + statGx;
           Gy = tempGy1 + tempGy2 + tempGy3 + statGy;
           Gz = tempGz1 + tempGz2 + tempGz3 + statGz;

           diagPrint(debugInfo,"Gx=%8d  Gy=%8d  Gz=%8d\n",Gx,Gy,Gz);

           if ( (Gx > 32767) || (Gx < -32767) )
           {
              errLogRet(LOGIT,debugInfo,"X gradient value %d is outside +/-32767 range\n",Gx);
              markAcodeSetDone(expName, curAcodeNumber);
              errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
              return (errorcode);
           }

           if ( (Gy > 32767) || (Gy < -32767) )
           {
              errLogRet(LOGIT,debugInfo,"Y gradient value %d is outside +/-32767 range\n",Gy);
              markAcodeSetDone(expName, curAcodeNumber);
              errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
              return (errorcode);
           }

           if ( (Gz > 32767) || (Gz < -32767) )
           {
              errLogRet(LOGIT,debugInfo,"Z gradient value %d is outside +/-32767 range\n",Gz);
              markAcodeSetDone(expName, curAcodeNumber);
              errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
              return (errorcode);
           }

            Gx &= 0xffff;  Gy &= 0xffff;  Gz &= 0xffff;

            writeCntrlFifoWord(XPFGAMPKEY | Gx);
            writeCntrlFifoWord(YPFGAMPKEY | Gy);
            writeCntrlFifoWord(ZPFGAMPKEY | Gz);


            peR_step = 0; peP_step = 0; peS_step = 0;

           break;



        case SHAPEDGRD:
           DPRINT(-1,"SHAPEDGRD");

           pat1_index = *p2CurrentWord++;// shape ID
           i = *p2CurrentWord++;         // axis  KEYS are same for PFG and GRADIENT Controllers
           j = *p2CurrentWord++;         // amplitude multiplier
           k = *p2CurrentWord++;         // DURATIONKEY word
           l = *p2CurrentWord++;         // loops currently set to 1

           errorcode = getPattern(pat1_index, &patternList1, &patternSize1);

           if ( (errorcode == 0) && (patternSize1 > 0) )
           {
               writeCntrlFifoWord(DURATIONKEY | k);

               pIntW1 = patternList1;

               for (m=0; m < patternSize1; m++)
               {
                   Gx   = j*(*pIntW1);
                   Gx >>= 15;

                   if (m == patternSize1/2)
                   {
                      diagPrint(debugInfo,"SHAPEDGRD: Gw=%d\n",Gx);
                   }

                   if ( (Gx > 32767) || (Gx < -32767) )
                   {
                     errLogRet(LOGIT,debugInfo,"gradient value %d is outside +/-32767 range in shapedgradient\n",Gx);
                     markAcodeSetDone(expName, curAcodeNumber);
                     errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
                     return (errorcode);
                   }

                   Gx  &= 0xffff;
                   writeCntrlFifoWord(i | Gx | LATCHKEY);

                   pIntW1++;
               }
           }

           break;



	case OBLPESHAPEDGRD:
           DPRINT(-1, "OBLPESHAPEDGRD");

           i = *p2CurrentWord++;           // type

           pat1_index = *p2CurrentWord++;
           pat2_index = *p2CurrentWord++;
           pat3_index = *p2CurrentWord++;
                DPRINT3(-1,"pat1index: %d pat2index: %d pat3index: %d\n",pat1_index,pat2_index,pat3_index);

           G_r = *p2CurrentWord++;         // static part for Gr
           G_p = *p2CurrentWord++;         // static part for Gp
           G_s = *p2CurrentWord++;         // static part for Gs
                DPRINT3(-1,"SAS G_r: %d G_p: %d G_s\n",G_r,G_p,G_s);

           k   = *p2CurrentWord++;         // Duration Count
           l   = *p2CurrentWord++;         // Loop Count
                DPRINT2(-1,"duration count: %d loop count: %d\n",k,l);

           if ((i & 0x8) || (i & 0x10) || (i & 0x20)){  // POP off the extra codes
                pat4_index = *p2CurrentWord++;
                pat5_index = *p2CurrentWord++;
                pat6_index = *p2CurrentWord++;
                DPRINT3(-1,"SAS pat4index: %d pat5index: %d pat6index: %d\n",pat4_index,pat5_index,pat6_index);

                d_G_r = *p2CurrentWord++;         // static part for d_Gr
                d_G_p = *p2CurrentWord++;         // static part for d_Gp
                d_G_s = *p2CurrentWord++;         // static part for d_Gs
                DPRINT3(-1,"SAS d_G_r: %d d_G_p: %d d_G_s\n",d_G_r,d_G_p,d_G_s);
           }

           /* Now do the rotation with waveform pattern streams */

              errorcode = 0;
              if (i & 0x1)
              {
                 errorcode = getPattern(pat1_index, &patternList1, &patternSize1);
                 if (errorcode == 0)
                 {
                   pIntW1 = patternList1;
                   ii     = patternSize1;
                   /*
                   if (rtVar[peR_vind] > peR_lim)
                      errLogRet(LOGIT,debugInfo,
                          "real time value exceeds limit %d in ReadOut.\n", peR_lim);
                   */
                 }
              }
              if ((errorcode == 0) && (i & 0x2))
              {
                 errorcode = getPattern(pat2_index, &patternList2, &patternSize2);
                 if (errorcode == 0)
                 {
                   pIntW2 = patternList2;
                   ii     = patternSize2;
                   /*
                   if (rtVar[peP_vind] > peP_lim)
                      errLogRet(LOGIT,debugInfo,
                          "real time value exceeds limit %d in PhaseEncode.\n", peP_lim);
                   */
                 }
              }
              if ((errorcode == 0) && (i & 0x4))
              {
                 errorcode = getPattern(pat3_index, &patternList3, &patternSize3);
                 if (errorcode == 0)
                 {
                   pIntW3 = patternList3;
                   ii     = patternSize3;
                   /*
                   if (rtVar[peS_vind] > peS_lim)
                      errLogRet(LOGIT,debugInfo,
                          "real time value exceeds limit %d in SliceSelect.\n", peS_lim);
                   */
                 }
              }

           if ((i & 0x8) || (i & 0x10) || (i & 0x20)){  // POP off the extra codes
              if ((errorcode == 0) && (i & 0x8))
              {
                 errorcode = getPattern(pat4_index, &patternList4, &patternSize4);
                 if (errorcode == 0)
                 {
                   pIntW4 = patternList4;
                   ii     = patternSize4;
                 }
              }
              if ((errorcode == 0) && (i & 0x10))
              {
                 errorcode = getPattern(pat5_index, &patternList5, &patternSize5);
                 if (errorcode == 0)
                 {
                   pIntW5 = patternList5;
                   ii     = patternSize5;
                 }
              }
              if ((errorcode == 0) && (i & 0x20))
              {
                 errorcode = getPattern(pat6_index, &patternList6, &patternSize6);
                 if (errorcode == 0)
                 {
                   pIntW6 = patternList6;
                   ii     = patternSize6;
                 }
              }
           }




              if (errorcode == 0)
              {
                 if (ii > 0)   // pattern size
                 {
                    // do the oblique rotation in a loop here for the waveform pattern elements

                    writeCntrlFifoWord(DURATIONKEY   | k);

                    pgwfg = pGradWfg;     /* array ptr for gwfg array  */
                    if (pgwfg == NULL)
                       errLogRet(LOGIT,debugInfo,"OBLPESHAPED waveform array allocation is null\n");

                    mm = 1;               /* no of amp over range error messages to be sent */

                    for (m=0; m < ii; m++)
                    {

                       tempGx1=0;  tempGy1=0;  tempGz1=0;
                       tempGx2=0;  tempGy2=0;  tempGz2=0;
                       tempGx3=0;  tempGy3=0;  tempGz3=0;
                       srotm_11=0; srotm_12=0; srotm_13=0;
                       srotm_21=0; srotm_22=0; srotm_23=0;
                       srotm_31=0; srotm_32=0; srotm_33=0;

                       d_srotm_11=0; d_srotm_12=0; d_srotm_13=0;
                       d_srotm_21=0; d_srotm_22=0; d_srotm_23=0;
                       d_srotm_31=0; d_srotm_32=0; d_srotm_33=0;


                      if (i & 0x1)
                      {
                       srotm_11 = (rotm_11*(*pIntW1))>>16;
                       srotm_21 = (rotm_21*(*pIntW1))>>16;
                       srotm_31 = (rotm_31*(*pIntW1))>>16;
                       pIntW1++;
                      }

                      if (i & 0x2)
                      {
                       srotm_12 = (rotm_12*(*pIntW2))>>16;
                       srotm_22 = (rotm_22*(*pIntW2))>>16;
                       srotm_32 = (rotm_32*(*pIntW2))>>16;
                       pIntW2++;
                      }

                      if (i & 0x4)
                      {
                       srotm_13 = (rotm_13*(*pIntW3))>>16;
                       srotm_23 = (rotm_23*(*pIntW3))>>16;
                       srotm_33 = (rotm_33*(*pIntW3))>>16;
                       pIntW3++;
                      }

           if ((i & 0x8) || (i & 0x10) || (i & 0x20)){  // POP off the extra codes

                      if (i & 0x8)
                      {
                       d_srotm_11 = (rotm_11*(*pIntW4))>>16;
                       d_srotm_21 = (rotm_21*(*pIntW4))>>16;
                       d_srotm_31 = (rotm_31*(*pIntW4))>>16;
                       pIntW4++;
                      }

                      if (i & 0x10)
                      {
                       d_srotm_12 = (rotm_12*(*pIntW5))>>16;
                       d_srotm_22 = (rotm_22*(*pIntW5))>>16;
                       d_srotm_32 = (rotm_32*(*pIntW5))>>16;
                       pIntW5++;
                      }

                      if (i & 0x20)
                      {
                       d_srotm_13 = (rotm_13*(*pIntW6))>>16;
                       d_srotm_23 = (rotm_23*(*pIntW6))>>16;
                       d_srotm_33 = (rotm_33*(*pIntW6))>>16;
                       pIntW6++;
                      }
	}



                       /* Do the rotation of the static gradient part */

                       statGx  = srotm_11*G_r + srotm_12*G_p + srotm_13*G_s ;
                       statGy  = srotm_21*G_r + srotm_22*G_p + srotm_23*G_s ;
                       statGz  = srotm_31*G_r + srotm_32*G_p + srotm_33*G_s ;

                       statGx >>= 15;  statGy >>= 15;  statGz >>= 15;

           if ((i & 0x8) || (i & 0x10) || (i & 0x20)){  // POP off the extra codes
                       d_statGx  = d_srotm_11*d_G_r + d_srotm_12*d_G_p + d_srotm_13*d_G_s ;
                       d_statGy  = d_srotm_21*d_G_r + d_srotm_22*d_G_p + d_srotm_23*d_G_s ;
                       d_statGz  = d_srotm_31*d_G_r + d_srotm_32*d_G_p + d_srotm_33*d_G_s ;

                       d_statGx >>= 15;  d_statGy >>= 15;  d_statGz >>= 15;
	}




                       /* First: Gradient RO */

                       if (peR_step != 0)
                       {
                         tempGx1 = ((srotm_11*peR_step +0x3fff)>>15)*rtVar[peR_vind];
                         tempGy1 = ((srotm_21*peR_step +0x3fff)>>15)*rtVar[peR_vind];
                         tempGz1 = ((srotm_31*peR_step +0x3fff)>>15)*rtVar[peR_vind];
                       }

                       /* Second: Gradient PE */

                       if (peP_step != 0)
                       {
                         tempGx2 = ((srotm_12*peP_step +0x3fff)>>15)*rtVar[peP_vind];
                         tempGy2 = ((srotm_22*peP_step +0x3fff)>>15)*rtVar[peP_vind];
                         tempGz2 = ((srotm_32*peP_step +0x3fff)>>15)*rtVar[peP_vind];
                       }

                       /* Third: Gradient SS */

                       if (peS_step != 0)
                       {
                         tempGx3 = ((srotm_13*peS_step +0x3fff)>>15)*rtVar[peS_vind];
                         tempGy3 = ((srotm_23*peS_step +0x3fff)>>15)*rtVar[peS_vind];
                         tempGz3 = ((srotm_33*peS_step +0x3fff)>>15)*rtVar[peS_vind];
                       }

                       Gx = tempGx1 + tempGx2 + tempGx3 + statGx;
                       Gy = tempGy1 + tempGy2 + tempGy3 + statGy;
                       Gz = tempGz1 + tempGz2 + tempGz3 + statGz;

           if ((i & 0x8) || (i & 0x10) || (i & 0x20)){  // POP off the extra codes
                       tempGx1=0;  tempGy1=0;  tempGz1=0;
                       tempGx2=0;  tempGy2=0;  tempGz2=0;
                       tempGx3=0;  tempGy3=0;  tempGz3=0;

                       /* 4th: Gradient d_RO */

                       if (d_peR_step != 0)
                       {
                         tempGx1 = ((d_srotm_11*d_peR_step +0x3fff)>>15)*rtVar[d_peR_vind];
                         tempGy1 = ((d_srotm_21*d_peR_step +0x3fff)>>15)*rtVar[d_peR_vind];
                         tempGz1 = ((d_srotm_31*d_peR_step +0x3fff)>>15)*rtVar[d_peR_vind];
                       }

                       /* 5th: Gradient d_PE */

                       if (d_peP_step != 0)
                       {
                         tempGx2 = ((d_srotm_12*d_peP_step +0x3fff)>>15)*rtVar[d_peP_vind];
                         tempGy2 = ((d_srotm_22*d_peP_step +0x3fff)>>15)*rtVar[d_peP_vind];
                         tempGz2 = ((d_srotm_32*d_peP_step +0x3fff)>>15)*rtVar[d_peP_vind];
                       }

                       /* 6th: Gradient d_SS */

                       if (d_peS_step != 0)
                       {
                         tempGx3 = ((d_srotm_13*d_peS_step +0x3fff)>>15)*rtVar[d_peS_vind];
                         tempGy3 = ((d_srotm_23*d_peS_step +0x3fff)>>15)*rtVar[d_peS_vind];
                         tempGz3 = ((d_srotm_33*d_peS_step +0x3fff)>>15)*rtVar[d_peS_vind];
                       }

                       Gx = Gx + tempGx1 + tempGx2 + tempGx3 + d_statGx;
                       Gy = Gy + tempGy1 + tempGy2 + tempGy3 + d_statGy;
                       Gz = Gz + tempGz1 + tempGz2 + tempGz3 + d_statGz;
	}



/*
                       if (m == ii/2)
                       {
                          diagPrint(debugInfo,"Gx1=%d  Gx2=%d  Gx3=%d  statGx=%d\n",tempGx1,tempGx2,tempGx3,statGx);
                          diagPrint(debugInfo,"Gy1=%d  Gy2=%d  Gy3=%d  statGy=%d\n",tempGy1,tempGy2,tempGy3,statGy);
                          diagPrint(debugInfo,"Gz1=%d  Gz2=%d  Gz3=%d  statGz=%d\n",tempGz1,tempGz2,tempGz3,statGz);
                          diagPrint(debugInfo,"Gx=%d   Gy=%d   Gz=%d\n\n",Gx,Gy,Gz);
                       }

                       if ( (Gx > 32767) || (Gx < -32767) )
                       {
                          if (mm > 0)
                          {
                            errLogRet(LOGIT,debugInfo,"X gradient value %d is outside +/-32767 range\n",Gx);
                            mm--;
                          }
                       }

                       if ( (Gy > 32767) || (Gy < -32767) )
                       {
                          if (mm > 0)
                          {
                            errLogRet(LOGIT,debugInfo,"Y gradient value %d is outside +/-32767 range\n",Gy);
                            mm--;
                          }
                       }

                       if ( (Gz > 32767) || (Gz < -32767) )
                       {
                          if (mm > 0)
                          {
                            errLogRet(LOGIT,debugInfo,"Z gradient value %d is outside +/-32767 range\n",Gz);
                            mm--;
                          }
                       }
*/

                       if ( (Gx > 32767) || (Gx < -32767) )
                       {
                          errLogRet(LOGIT,debugInfo,"X gradient value %d is outside +/-32767 range\n",Gx);
                          markAcodeSetDone(expName, curAcodeNumber);
                          errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
                          return (errorcode);
                       }

                       if ( (Gy > 32767) || (Gy < -32767) )
                       {
                          errLogRet(LOGIT,debugInfo,"Y gradient value %d is outside +/-32767 range\n",Gy);
                          markAcodeSetDone(expName, curAcodeNumber);
                          errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
                          return (errorcode);
                       }

                       if ( (Gz > 32767) || (Gz < -32767) )
                       {
                          errLogRet(LOGIT,debugInfo,"Z gradient value %d is outside +/-32767 range\n",Gz);
                          markAcodeSetDone(expName, curAcodeNumber);
                          errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
                          return (errorcode);
                       }

                       Gx &= 0xffff;  Gy &= 0xffff; Gz &= 0xffff;

                       *pgwfg = (XPFGAMPKEY | Gx);
                        pgwfg++;
                       *pgwfg = (YPFGAMPKEY | Gy);
                        pgwfg++;
                       *pgwfg = (ZPFGAMPKEY | Gz | LATCHKEY);
                        pgwfg++;


                       if ( (pgwfg - pGradWfg) >= 1000)
                       {
                         writeCntrlFifoBuf(pGradWfg,(pgwfg-pGradWfg));
                         /* errLogRet(LOGIT,debugInfo,"pgwfg is re-set to 0x%x   %d words written\n",pgwfg,(pgwfg-pGradWfg));  */
                         pgwfg = pGradWfg;
                       }
                       else if (m == (ii-1))
                       {
                         writeCntrlFifoBuf(pGradWfg,(pgwfg-pGradWfg));
                         /* errLogRet(LOGIT,debugInfo,"end of for loop: %d words written\n",(pgwfg-pGradWfg)); */
                       }

                    }
                 }
              }
              else
              {
                 return (errorcode);
              }

           peR_step = 0; peP_step = 0; peS_step = 0;
           d_peR_step = 0; d_peP_step = 0; d_peS_step = 0;


           break;


#endif



#ifdef GRADIENT_CNTLR
    case GRAD_DELAYS:
		{
	    extern void resetClearReg();
    	extern void resetDelays();
    	extern void setXDelay(int ticks);
       	extern void setYDelay(int ticks);
       	extern void setZDelay(int ticks);
       	extern void seB0Delay(int ticks);
        i = *p2CurrentWord++;
        j = *p2CurrentWord++;
        k = *p2CurrentWord++;
        l = *p2CurrentWord++;
#ifdef GRADIENT_CoarseXDelay
        DPRINT4(-1, " GRAD_DELAYS %d %d %d %d\n",i,j,k,l);
        resetClearReg();
        resetDelays();
        setXDelay(i);
        setYDelay(j);
        setZDelay(k);
        setB0Delay(l);
#endif
		}
        break;
            /*
             * VGRADIENT arg1 - which one FFKEY value arg2 -
             * rtvar to multiply arg3 - multiplier
             */
   case VGRADIENT:
            DPRINT(-1, " VGRADIENT\n");
            i = *p2CurrentWord++;
            j = *p2CurrentWord++;
            k = *p2CurrentWord++;
            writeCntrlFifoWord((((rtVar[j]) * k) & 0xffff) | i);
            break;
	case SETSHIMS:
	   {int i,num,time,v,fWord[2];
            time = *p2CurrentWord++;
            num  = *p2CurrentWord++;
            DPRINT2(-1," SETSHIM:  num=%d, time=0x%x\n",num,time);
            for (i=0; i<num; i++)
            {
               switch(*p2CurrentWord++)
               {
               case 2:		// Z1 for Varian shims
               case 3:		// Z1C for RRI shims
                    v = (*p2CurrentWord + 0x8000) & 0xFFFF;
                    XYZshims[2]=v;
                    fWord[0] = encode_GRADIENTSetShim(1,((6<<16) | v) );
                    fWord[1] = (LATCHKEY | DURATIONKEY | time);
                    p2CurrentWord++;
   // DPRINT2(-1," SETSHIM:  num=2,3, word=0x%x(0x%x)\n",fWord[0],v);
                    writeCntrlFifoBuf(fWord,2);
                    break;
               case 16:		// X shim
                    v = (*p2CurrentWord + 0x8000) & 0xFFFF;
                    XYZshims[0]=v;
                    fWord[0] = encode_GRADIENTSetShim(1,((2<<16) | v) );
                    fWord[1] = (LATCHKEY | DURATIONKEY | time);
                    p2CurrentWord++;
   // DPRINT2(-1," SETSHIM:  num=16, word=0x%x(0x%x)\n",fWord[0],v);
                    writeCntrlFifoBuf(fWord,2);
                    break;
               case 17:		// Y shim
                    v = (*p2CurrentWord + 0x8000) & 0xFFFF;
                    XYZshims[1]=v;
                    fWord[0] = encode_GRADIENTSetShim(1,((4<<16) | v) );
                    fWord[1] = (LATCHKEY | DURATIONKEY | time);
                    p2CurrentWord++;
   // DPRINT2(-1," SETSHIM:  num=17, word=0x%x(0x%x)\n",fWord[0],v);
                    writeCntrlFifoBuf(fWord,2);
                    break;
               default:
                    errLogRet(LOGIT,debugInfo,"Illegal Shim DAC\n");
                    p2CurrentWord++;
                    break;
               }
            }
           }
	    break;
        case ECC_AMPS:
            DPRINT(-1,"ECC_AMPS \n");
            loadEccAmpTerms(p2CurrentWord);
            p2CurrentWord += 52;
	    break;

        case ECC_TIMES:
            DPRINT1(-1,"ECC_TIMES p2CurrentWord=0x%x\n",p2CurrentWord);
            loadEccTimeConstTerms(p2CurrentWord);
            p2CurrentWord += 104;
	    break;

	case SDAC_VALUES:
          { int n;
            DPRINT(-1,"SDAC_VALUES\n");
            n = *p2CurrentWord++;
            loadSdacScaleNLimits(p2CurrentWord);
            p2CurrentWord += n;
          }
	    break;

        case MRIUSERGATES:
            i = *p2CurrentWord++;
            DPRINT1( 1,"MRIUSERGATES  %d", rtVar[i]);
	    writeCntrlFifoWord(encode_GRADIENTSetUser(0,rtVar[i]));
            break;

	case SETGRDROTATION:
            DPRINT(-1, "SETGRDROTATION");
            rotm_11 = *p2CurrentWord++;
            rotm_12 = *p2CurrentWord++;
            rotm_13 = *p2CurrentWord++;

            rotm_21 = *p2CurrentWord++;
            rotm_22 = *p2CurrentWord++;
            rotm_23 = *p2CurrentWord++;

            rotm_31 = *p2CurrentWord++;
            rotm_32 = *p2CurrentWord++;
            rotm_33 = *p2CurrentWord++;
            break;


        case SETVGRDROTATION:
            DPRINT(-1, "SETVGRDROTATION");
            pat_index = *p2CurrentWord++;   /* pattern id for rotation angle sets */
            j         = *p2CurrentWord++;   /* v-index */
            k         = rtVar[j];

            if ( k < 0 )
            {
               k = 0;
            }

            errorcode = getPattern(pat_index, &patternList, &patternSize);
            if (errorcode != 0)
            {
                errLogRet(LOGIT,debugInfo,
                        "SETVGRDROTATION: rotation list not found.\n");
                return (errorcode);
            }

            k = k % 262144; /* 262144 is the max number of rot sets in a single list id */

            /* check validity of the rtvar-index */
            if ((k*9+9) > patternSize)  /* 9 matrix elements */
            {
              errLogRet(LOGIT,debugInfo,
                   "SETVGRDROTATION: error in rotation list or list index.\n");
              return(SYSTEMERROR+GRD_ROTATION_LIST_ERR);
            }

            diagPrint(debugInfo,"\n");
            diagPrint(debugInfo,"pattern id = %d   rtvar name=%d   rtvar value=%d   index=%d\n",pat_index,j,rtVar[j],k);

            pIntW1 = patternList + k*9;  /* start addr of rtVar[j]-th rotation set */

            rotm_11 = *pIntW1++;
            rotm_12 = *pIntW1++;
            rotm_13 = *pIntW1++;

            rotm_21 = *pIntW1++;
            rotm_22 = *pIntW1++;
            rotm_23 = *pIntW1++;

            rotm_31 = *pIntW1++;
            rotm_32 = *pIntW1++;
            rotm_33 = *pIntW1++;

            diagPrint(debugInfo,"11=%d  12=%d  13=%d\n",rotm_11,rotm_12,rotm_13);
            diagPrint(debugInfo,"21=%d  22=%d  23=%d\n",rotm_21,rotm_22,rotm_23);
            diagPrint(debugInfo,"31=%d  32=%d  33=%d\n",rotm_31,rotm_32,rotm_33);
            break;



	case SETVGRDANGLIST:
            DPRINT(-1, "SETVGRDANGLIST");

            pat_index = *p2CurrentWord++;   /* pattern id for rotation angle   */
            i         = *p2CurrentWord++;   /* angle type: 0=psi 1=phi 2=theta */
            j         = *p2CurrentWord++;   /* v-index */

            errorcode = getPattern(pat_index, &patternList, &patternSize);
            if (errorcode != 0)
            {
                errLogRet(LOGIT,debugInfo,
                        "SETVGRDANGLIST: rotation angle list not found.\n");
                return (errorcode);
            }
     
            k = rtVar[j] % patternSize;    /* modulo or should it warn about out of array access? */

            if (k >= patternSize)
            {
                errorcode =  SYSTEMERROR+GRD_ROTATION_LIST_ERR ;
                errLogRet(LOGIT,debugInfo,
                        "SETVGRDANGLIST: invalid rotation angle specified.\n");
                return (errorcode);
            }

            /* angles in integer form preserved 0.01 deg resolution */
            if (i == 0) 
               ang_psi   = (patternList[k])/100.0;
            else if (i == 1)
               ang_phi   = (patternList[k])/100.0;
            else if (i == 2)
               ang_theta = (patternList[k])/100.0;
            else
            {
                errorcode =  SYSTEMERROR+GRD_ROTATION_LIST_ERR ;
                errLogRet(LOGIT,debugInfo,
                        "SETVGRDANGLIST: invalid rotation angle specified.\n");
                return (errorcode);
            }
            diagPrint(debugInfo,"SETVGRDANGLIST: pat_index=%d pat_size=%d i=%d j=%d k=%d executed\n",pat_index,patternSize,i,j,k);
	    break;



	case EXEVGRDROTATION:
           DPRINT(-1, "EXEVGRDROTATION");
 
           gxflip = *p2CurrentWord++;       /* gxflip */
           gyflip = *p2CurrentWord++;       /* gyflip */
           gzflip = *p2CurrentWord++;       /* gzflip */
 
           /* check to make sure that all angles are set once */
           /* code to check here                              */

           calc_obl_matrix(ang_psi,ang_phi,ang_theta,&tmp_m11,&tmp_m12,&tmp_m13, \
                           &tmp_m21,&tmp_m22,&tmp_m23,&tmp_m31,&tmp_m32,&tmp_m33);
           
           rotm_11 = gxflip * (int)(tmp_m11*65536.0);
           rotm_12 = gxflip * (int)(tmp_m12*65536.0);
           rotm_13 = gxflip * (int)(tmp_m13*65536.0);

           rotm_21 = gyflip * (int)(tmp_m21*65536.0);
           rotm_22 = gyflip * (int)(tmp_m22*65536.0);
           rotm_23 = gyflip * (int)(tmp_m23*65536.0);
           
           rotm_31 = gzflip * (int)(tmp_m31*65536.0);
           rotm_32 = gzflip * (int)(tmp_m32*65536.0);
           rotm_33 = gzflip * (int)(tmp_m33*65536.0);

           diagPrint(debugInfo,"EXEVGRDROTATION:\n");
           diagPrint(debugInfo,"11=%d  12=%d  13=%d\n",rotm_11,rotm_12,rotm_13);
           diagPrint(debugInfo,"21=%d  22=%d  23=%d\n",rotm_21,rotm_22,rotm_23);
           diagPrint(debugInfo,"31=%d  32=%d  33=%d\n",rotm_31,rotm_32,rotm_33);

	   break;



        case SETPEVALUE:
           DPRINT(-1, "SETPEVALUE");

           i = *p2CurrentWord++;
           if (i == 1)
           {
              peR_step = *p2CurrentWord++;
              peR_vind = *p2CurrentWord++;
              peR_lim  = *p2CurrentWord++;
           }
           if (i == 2)
           {
              peP_step = *p2CurrentWord++;
              peP_vind = *p2CurrentWord++;
              peP_lim  = *p2CurrentWord++;
           }
           if (i == 3)
           {
              peS_step = *p2CurrentWord++;
              peS_vind = *p2CurrentWord++;
              peS_lim  = *p2CurrentWord++;
           }
           if (i == 4)
           {
           /*  DPRINT4(-1,"SASSETMYPE %x %x  %x  %x\n",i,d_peR_step,d_peR_vind,d_peR_lim); */
              d_peR_step = *p2CurrentWord++;
              d_peR_vind = *p2CurrentWord++;
              d_peR_lim  = *p2CurrentWord++;
           }
           if (i == 5)
           {
            /* DPRINT4(-1,"SASSETMYPHASE %x %x  %x  %x\n",i,d_peP_step,d_peP_vind,d_peP_lim); */
              d_peP_step = *p2CurrentWord++;
              d_peP_vind = *p2CurrentWord++;
              d_peP_lim  = *p2CurrentWord++;
           }
           if (i == 6)
           {
            /* DPRINT4(-1,"SASSETMYSLICE %x %x  %x  %x\n",i,d_peS_step,d_peS_vind,d_peS_lim); */
              d_peS_step = *p2CurrentWord++;
              d_peS_vind = *p2CurrentWord++;
              d_peS_lim  = *p2CurrentWord++;
           }

           break;



	case OBLPEGRD:
           DPRINT(-1, "OBLPEGRD");

           i = *p2CurrentWord++;           // type

           G_r = *p2CurrentWord++;         // static part for Gr
           G_p = *p2CurrentWord++;         // static part for Gp
           G_s = *p2CurrentWord++;         // static part for Gs



           // do the phase encode & oblique rotation


           tempGx1=0;  tempGy1=0;  tempGz1=0;
           tempGx2=0;  tempGy2=0;  tempGz2=0;
           tempGx3=0;  tempGy3=0;  tempGz3=0;
           srotm_11=0; srotm_12=0; srotm_13=0;
           srotm_21=0; srotm_22=0; srotm_23=0;
           srotm_31=0; srotm_32=0; srotm_33=0;

           srotm_11 = (rotm_11*32767)>>16;
           srotm_21 = (rotm_21*32767)>>16;
           srotm_31 = (rotm_31*32767)>>16;

           srotm_12 = (rotm_12*32767)>>16;
           srotm_22 = (rotm_22*32767)>>16;
           srotm_32 = (rotm_32*32767)>>16;

           srotm_13 = (rotm_13*32767)>>16;
           srotm_23 = (rotm_23*32767)>>16;
           srotm_33 = (rotm_33*32767)>>16;

           /* Do the rotation of the static gradient part */

           statGx  = srotm_11*G_r + srotm_12*G_p + srotm_13*G_s ;
           statGy  = srotm_21*G_r + srotm_22*G_p + srotm_23*G_s ;
           statGz  = srotm_31*G_r + srotm_32*G_p + srotm_33*G_s ;

           statGx >>= 15;  statGy >>= 15;  statGz >>= 15;


           /* First: Gradient RO */

           if (peR_step != 0)
           {
               tempGx1 = ((srotm_11*peR_step +0x3fff)>>15)*rtVar[peR_vind];
               tempGy1 = ((srotm_21*peR_step +0x3fff)>>15)*rtVar[peR_vind];
               tempGz1 = ((srotm_31*peR_step +0x3fff)>>15)*rtVar[peR_vind];
           }

           /* Second: Gradient PE */

           if (peP_step != 0)
           {
               tempGx2 = ((srotm_12*peP_step +0x3fff)>>15)*rtVar[peP_vind];
               tempGy2 = ((srotm_22*peP_step +0x3fff)>>15)*rtVar[peP_vind];
               tempGz2 = ((srotm_32*peP_step +0x3fff)>>15)*rtVar[peP_vind];
           }

           /* Third: Gradient SS */

           if (peS_step != 0)
           {
               tempGx3 = ((srotm_13*peS_step +0x3fff)>>15)*rtVar[peS_vind];
               tempGy3 = ((srotm_23*peS_step +0x3fff)>>15)*rtVar[peS_vind];
               tempGz3 = ((srotm_33*peS_step +0x3fff)>>15)*rtVar[peS_vind];
           }

           Gx = tempGx1 + tempGx2 + tempGx3 + statGx;
           Gy = tempGy1 + tempGy2 + tempGy3 + statGy;
           Gz = tempGz1 + tempGz2 + tempGz3 + statGz;

           diagPrint(debugInfo,"Gx=%8d  Gy=%8d  Gz=%8d\n",Gx,Gy,Gz);

           if ( (Gx > 32767) || (Gx < -32767) )
           {
              errLogRet(LOGIT,debugInfo,"X gradient value %d is outside +/-32767 range\n",Gx);
              markAcodeSetDone(expName, curAcodeNumber);
              errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
              return (errorcode);
           }

           if ( (Gy > 32767) || (Gy < -32767) )
           {
              errLogRet(LOGIT,debugInfo,"Y gradient value %d is outside +/-32767 range\n",Gy);
              markAcodeSetDone(expName, curAcodeNumber);
              errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
              return (errorcode);
           }

           if ( (Gz > 32767) || (Gz < -32767) )
           {
              errLogRet(LOGIT,debugInfo,"Z gradient value %d is outside +/-32767 range\n",Gz);
              markAcodeSetDone(expName, curAcodeNumber);
              errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
              return (errorcode);
           }

            Gx &= 0xffff;  Gy &= 0xffff;  Gz &= 0xffff;

            writeCntrlFifoWord(XGRADKEY | Gx);
            writeCntrlFifoWord(YGRADKEY | Gy);
            writeCntrlFifoWord(ZGRADKEY | Gz);


            peR_step = 0; peP_step = 0; peS_step = 0;

           break;


	case SHAPEDGRD:
           DPRINT(-1,"SHAPEDGRD");

           pat1_index = *p2CurrentWord++;// shape ID
           i = *p2CurrentWord++;         // axis
           j = *p2CurrentWord++;         // amplitude multiplier
           k = *p2CurrentWord++;         // DURATIONKEY word
           l = *p2CurrentWord++;         // loops currently set to 1

           errorcode = getPattern(pat1_index, &patternList1, &patternSize1);

           if ( (errorcode == 0) && (patternSize1 > 0) )
           {

               n  = k/320;         /* repeats */
               ii = k%320;         /* extra   */
               if (ii > 0)
               {
                  n--;
                  ii += 320;
               }
               kk = 0;             /* index into grid array start at 0 */

               if (n == 1)
               {
                 gradgridword[kk++] = (LATCHKEY | DURATIONKEY | 320);
               }
               else
               {
                  while (n > 0)
                  {
                     if (n > 65530)
                       jj = 65530;
                     else
                       jj = n;

                     gradgridword[kk++] = (LATCHKEY | (17 << 26) | (jj << 10) | 320) ;
                     n -= jj;
                   }
               }

               /* now set the extra remaining ticks                     */
               if ( ii > 0 )
                   gradgridword[kk++] = (LATCHKEY | DURATIONKEY | ii);

               /* kk is the number of words written into gradgrid array */

               DPRINT1(-1,"SHAPEDGRD: no of words in gradgrid array = %d\n",kk);

               pIntW1 = patternList1;

               pgwfg  = pGradWfg;                    /* array ptr for gwfg array */
               if (pgwfg == NULL)
                  errLogRet(LOGIT, debugInfo, "SHAPED waveform array allocation is null\n");


               for (m=0; m < patternSize1; m++)
               {
                   Gx   = j*(*pIntW1);
                   Gx >>= 15;

                  /*
                   if (m == ((patternSize1/2)-1) )
                   {
                      diagPrint(debugInfo,"SHAPEDGRD: Gw = %d\n",Gx);
                   }
                  */

                   if ( (Gx > 32767) || (Gx < -32767) )
                   {
                      errLogRet(LOGIT,debugInfo,"X gradient value %d is outside +/-32767 range\n",Gx);
                      markAcodeSetDone(expName, curAcodeNumber);
                      errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
                      return (errorcode);
                   }

                   Gx  &= 0xffff;

                   *pgwfg = ( i | Gx ) ;            /*  No latch, latch provided by REPEAT writes */
                    pgwfg++;
                   for (ll=0; ll<kk; ll++)
                   {
                      *pgwfg = gradgridword[ll];    /* REPEAT writes */
                       pgwfg++;
                   }

                   if ( (pgwfg - pGradWfg) >= 1010 )
                   {
                      writeCntrlFifoBuf(pGradWfg, (pgwfg-pGradWfg));
                      pgwfg = pGradWfg;
                   }
                   else if (m == (patternSize1-1))
                   {
                      writeCntrlFifoBuf(pGradWfg, (pgwfg-pGradWfg));
                   }

                   pIntW1++;
               }
           }

          /* commented out
           writeCntrlFifoWord(XGRADKEY | 0);
           writeCntrlFifoWord(YGRADKEY | 0);
           writeCntrlFifoWord(ZGRADKEY | 0);
          */

           break;


	case OBLPESHAPEDGRD:

           DPRINT(-1, "OBLPESHAPEDGRD");

           i = *p2CurrentWord++;           // type

           pat1_index = *p2CurrentWord++;
           pat2_index = *p2CurrentWord++;
           pat3_index = *p2CurrentWord++;

           G_r = *p2CurrentWord++;         // static part for Gr
           G_p = *p2CurrentWord++;         // static part for Gp
           G_s = *p2CurrentWord++;         // static part for Gs

           k   = *p2CurrentWord++;         // Duration Count
           l   = *p2CurrentWord++;         // Loop Count

           sim_gradient = 0;

           if ((i & 0x8) || (i & 0x10) || (i & 0x20))   // pop off the extra codes for 2nd overlapping gradient
           {
                pat4_index = *p2CurrentWord++;
                pat5_index = *p2CurrentWord++;
                pat6_index = *p2CurrentWord++;
                DPRINT3(-1,"SAS pat4index: %d pat5index: %d pat6index: %d\n",pat4_index,pat5_index,pat6_index);

                d_G_r = *p2CurrentWord++;         // static part for d_Gr
                d_G_p = *p2CurrentWord++;         // static part for d_Gp
                d_G_s = *p2CurrentWord++;         // static part for d_Gs

                sim_gradient = 1;
                DPRINT3(-1,"SAS d_G_r: %d d_G_p: %d d_G_s\n",d_G_r,d_G_p,d_G_s);
           }


           /* Now do the rotation with waveform pattern streams */

              errorcode = 0;
              if (i & 0x1)
              {
                 errorcode = getPattern(pat1_index, &patternList1, &patternSize1);
                 if (errorcode == 0)
                 {
                   pIntW1 = patternList1;
                   ii     = patternSize1;
                   /*
                   if (rtVar[peR_vind] > peR_lim)
                      errLogRet(LOGIT,debugInfo,
                          "real time value exceeds limit %d in ReadOut.\n", peR_lim);
                   */
                 }
              }
              if ((errorcode == 0) && (i & 0x2))
              {
                 errorcode = getPattern(pat2_index, &patternList2, &patternSize2);
                 if (errorcode == 0)
                 {
                   pIntW2 = patternList2;
                   ii     = patternSize2;
                   /*
                   if (rtVar[peP_vind] > peP_lim)
                      errLogRet(LOGIT,debugInfo,
                          "real time value exceeds limit %d in PhaseEncode.\n", peP_lim);
                   */
                 }
              }
              if ((errorcode == 0) && (i & 0x4))
              {
                 errorcode = getPattern(pat3_index, &patternList3, &patternSize3);
                 if (errorcode == 0)
                 {
                   pIntW3 = patternList3;
                   ii     = patternSize3;
                   /*
                   if (rtVar[peS_vind] > peS_lim)
                      errLogRet(LOGIT,debugInfo,
                          "real time value exceeds limit %d in SliceSelect.\n", peS_lim);
                   */
                 }
              }

           if (sim_gradient)  // get patterns for 2nd overlapping gradient
           {
              if ((errorcode == 0) && (i & 0x8))
              {
                 errorcode = getPattern(pat4_index, &patternList4, &patternSize4);
                 if (errorcode == 0)
                 {
                   pIntW4 = patternList4;
                   ii     = patternSize4;
                 }
              }
              if ((errorcode == 0) && (i & 0x10))
              {
                 errorcode = getPattern(pat5_index, &patternList5, &patternSize5);
                 if (errorcode == 0)
                 {
                   pIntW5 = patternList5;
                   ii     = patternSize5;
                 }
              }
              if ((errorcode == 0) && (i & 0x20))
              {
                 errorcode = getPattern(pat6_index, &patternList6, &patternSize6);
                 if (errorcode == 0)
                 {
                   pIntW6 = patternList6;
                   ii     = patternSize6;
                 }
              }
           }



              if (errorcode == 0)
              {
                 if (ii > 0)   // pattern size
                 {
                    // do the oblique rotation in a loop here for the waveform pattern elements

                    nn = k/320;       /* repeats */
                    jj = k%320;       /* extra   */
                    if (jj > 0)
                    {
                       nn--;
                       jj += 320;
                    }
                    kk = 0;             /* index into grid array start at 0 */

                    if (nn == 1)
                    {
                       gradgridword[kk++] = (LATCHKEY | DURATIONKEY | 320);
                    }
                    else
                    {
                       while (nn > 0)
                       {
                          if (nn > 65530)
                             jtmp = 65530;
                          else
                             jtmp = nn;

                          gradgridword[kk++] = (LATCHKEY | (17 << 26) | (jtmp << 10) | 320) ;
                          nn -= jtmp;
                       }
                    }

                    /* now set the extra remaining ticks           */
                    if (jj > 0)
                        gradgridword[kk++] = (LATCHKEY | DURATIONKEY | jj);

                    /* kk is the number of words written into gradgrid array */

                    DPRINT1(-1,"OBLPESHAPEDGRD: no of words in gradgrid array = %d\n",kk);

                    pgwfg = pGradWfg;     /* array ptr for gwfg array  */
                    if (pgwfg == NULL)
                       errLogRet(LOGIT,debugInfo,"OBLPESHAPED waveform array allocation is null\n");

                    mm = 1;               /* no of amp over range error messages to be sent */
//SAS moved these out of the loop
                       srotm_11=0; srotm_12=0; srotm_13=0;
                       srotm_21=0; srotm_22=0; srotm_23=0;
                       srotm_31=0; srotm_32=0; srotm_33=0;
                       d_srotm_11=0; d_srotm_12=0; d_srotm_13=0;
                       d_srotm_21=0; d_srotm_22=0; d_srotm_23=0;
                       d_srotm_31=0; d_srotm_32=0; d_srotm_33=0;
//ENDSAS moved these out of the loop

                    for (m=0; m < ii; m++)
                    {

                       tempGx1=0;  tempGy1=0;  tempGz1=0;
                       tempGx2=0;  tempGy2=0;  tempGz2=0;
                       tempGx3=0;  tempGy3=0;  tempGz3=0;


                      if (i & 0x1)
                      {
                       srotm_11 = (rotm_11*(*pIntW1))>>16;
                       srotm_21 = (rotm_21*(*pIntW1))>>16;
                       srotm_31 = (rotm_31*(*pIntW1))>>16;
                       pIntW1++;
                      }

                      if (i & 0x2)
                      {
                       srotm_12 = (rotm_12*(*pIntW2))>>16;
                       srotm_22 = (rotm_22*(*pIntW2))>>16;
                       srotm_32 = (rotm_32*(*pIntW2))>>16;
                       pIntW2++;
                      }

                      if (i & 0x4)
                      {
                       srotm_13 = (rotm_13*(*pIntW3))>>16;
                       srotm_23 = (rotm_23*(*pIntW3))>>16;
                       srotm_33 = (rotm_33*(*pIntW3))>>16;
                       pIntW3++;
                      }

                    if (sim_gradient)  // define rotation elements for 2nd overlapping gradient
                    {

                      if (i & 0x8)
                      {
                       d_srotm_11 = (rotm_11*(*pIntW4))>>16;
                       d_srotm_21 = (rotm_21*(*pIntW4))>>16;
                       d_srotm_31 = (rotm_31*(*pIntW4))>>16;
                       pIntW4++;
                      }

                      if (i & 0x10)
                      {
                       d_srotm_12 = (rotm_12*(*pIntW5))>>16;
                       d_srotm_22 = (rotm_22*(*pIntW5))>>16;
                       d_srotm_32 = (rotm_32*(*pIntW5))>>16;
                       pIntW5++;
                      }

                      if (i & 0x20)
                      {
                       d_srotm_13 = (rotm_13*(*pIntW6))>>16;
                       d_srotm_23 = (rotm_23*(*pIntW6))>>16;
                       d_srotm_33 = (rotm_33*(*pIntW6))>>16;
                       pIntW6++;
                      }
                    }


                       /* Do the rotation of the static gradient part */

                       statGx  = srotm_11*G_r + srotm_12*G_p + srotm_13*G_s ;
                       statGy  = srotm_21*G_r + srotm_22*G_p + srotm_23*G_s ;
                       statGz  = srotm_31*G_r + srotm_32*G_p + srotm_33*G_s ;

                       statGx >>= 15;  statGy >>= 15;  statGz >>= 15;

                       if (sim_gradient)  
                       {
                         d_statGx  = d_srotm_11*d_G_r + d_srotm_12*d_G_p + d_srotm_13*d_G_s ;
                         d_statGy  = d_srotm_21*d_G_r + d_srotm_22*d_G_p + d_srotm_23*d_G_s ;
                         d_statGz  = d_srotm_31*d_G_r + d_srotm_32*d_G_p + d_srotm_33*d_G_s ;

                         d_statGx >>= 15;  d_statGy >>= 15;  d_statGz >>= 15;
                       }



                       /* First: Gradient RO */

                       if (peR_step != 0)
                       {

                         if ( (*(pIntW1-1) == 32767) || (*(pIntW1-1) == -32767) )     // original mode for PE truncate for constant part of WFG
                         {
                           tempGx1 = ((srotm_11*peR_step +0x3fff)>>15)*rtVar[peR_vind];
                           tempGy1 = ((srotm_21*peR_step +0x3fff)>>15)*rtVar[peR_vind];
                           tempGz1 = ((srotm_31*peR_step +0x3fff)>>15)*rtVar[peR_vind];
                         }
                         else                                                         // new round off before PE mult for varying part of WFG
                         {
                           tempv  = peR_step*rtVar[peR_vind];

                           lltemp = (srotm_11*tempv +0x3fff)>>15;
                           tempGx1= (int)lltemp;

                           lltemp = (srotm_21*tempv +0x3fff)>>15;
                           tempGy1= (int)lltemp;

                           lltemp = (srotm_31*tempv +0x3fff)>>15;
                           tempGz1= (int)lltemp;
                         }
                       }

                       /* Second: Gradient PE */

                       if (peP_step != 0)
                       {
                         if ( (*(pIntW2-1) == 32767) || (*(pIntW2-1) == -32767) )   // original mode for PE truncate for constant part of WFG
                         {
                           tempGx2 = ((srotm_12*peP_step +0x3fff)>>15)*rtVar[peP_vind];
                           tempGy2 = ((srotm_22*peP_step +0x3fff)>>15)*rtVar[peP_vind];
                           tempGz2 = ((srotm_32*peP_step +0x3fff)>>15)*rtVar[peP_vind];
                         }
                         else                                                       // new round off before PE mult for varying part of WFG
                         {
                           tempv   = peP_step*rtVar[peP_vind];

                           lltemp  = (srotm_12*tempv +0x3fff)>>15;
                           tempGx2 = (int)lltemp;

                           lltemp  = (srotm_22*tempv +0x3fff)>>15;
                           tempGy2 = (int)lltemp;

                           lltemp  = (srotm_32*tempv +0x3fff)>>15;
                           tempGz2 = (int)lltemp;
                         }
                       }

                       /* Third: Gradient SS */

                       if (peS_step != 0)
                       {
                         if ( (*(pIntW3-1) == 32767) || (*(pIntW3-1) == -32767) )   // original mode for PE truncate for constant part of WFG
                         {
                           tempGx3 = ((srotm_13*peS_step +0x3fff)>>15)*rtVar[peS_vind];
                           tempGy3 = ((srotm_23*peS_step +0x3fff)>>15)*rtVar[peS_vind];
                           tempGz3 = ((srotm_33*peS_step +0x3fff)>>15)*rtVar[peS_vind];
                         }
                         else                                                       // new round off before PE mult for varying part of WFG
                         {
                           tempv   = peS_step*rtVar[peS_vind];

                           lltemp  = (srotm_13*tempv +0x3fff)>>15;
                           tempGx3 = (int)lltemp;

                           lltemp  = (srotm_23*tempv +0x3fff)>>15;
                           tempGy3 = (int)lltemp;

                           lltemp  = (srotm_33*tempv +0x3fff)>>15;
                           tempGz3 = (int)lltemp;
                         }
                       }

                       Gx = tempGx1 + tempGx2 + tempGx3 + statGx;
                       Gy = tempGy1 + tempGy2 + tempGy3 + statGy;
                       Gz = tempGz1 + tempGz2 + tempGz3 + statGz;

                    if (sim_gradient)
                    {

                       tempGx1=0; tempGy1=0; tempGz1=0;
                       tempGx2=0; tempGy2=0; tempGz2=0;
                       tempGx3=0; tempGy3=0; tempGz3=0;

                       /* Fourth: Gradient RO of 2nd overlapping gradient */

                       if (d_peR_step != 0)
                       {

                         if ( (*(pIntW4-1) == 32767) || (*(pIntW4-1) == -32767) )     // original mode for PE truncate for constant part of WFG
                         {
                           tempGx1 = ((d_srotm_11*d_peR_step +0x3fff)>>15)*rtVar[d_peR_vind];
                           tempGy1 = ((d_srotm_21*d_peR_step +0x3fff)>>15)*rtVar[d_peR_vind];
                           tempGz1 = ((d_srotm_31*d_peR_step +0x3fff)>>15)*rtVar[d_peR_vind];
                         }
                         else                                                         // new round off before PE mult for varying part of WFG
                         {
                           tempv  = d_peR_step*rtVar[d_peR_vind];

                           lltemp = (d_srotm_11*tempv +0x3fff)>>15;
                           tempGx1= (int)lltemp;

                           lltemp = (d_srotm_21*tempv +0x3fff)>>15;
                           tempGy1= (int)lltemp;

                           lltemp = (d_srotm_31*tempv +0x3fff)>>15;
                           tempGz1= (int)lltemp;
                         }
                       }

                       /* Fifth: Gradient PE of 2nd overlapping gradient */

                       if (d_peP_step != 0)
                       {
                         if ( (*(pIntW5-1) == 32767) || (*(pIntW5-1) == -32767) )   // original mode for PE truncate for constant part of WFG
                         {
                           tempGx2 = ((d_srotm_12*d_peP_step +0x3fff)>>15)*rtVar[d_peP_vind];
                           tempGy2 = ((d_srotm_22*d_peP_step +0x3fff)>>15)*rtVar[d_peP_vind];
                           tempGz2 = ((d_srotm_32*d_peP_step +0x3fff)>>15)*rtVar[d_peP_vind];
                         }
                         else                                                       // new round off before PE mult for varying part of WFG
                         {
                           tempv   = d_peP_step*rtVar[d_peP_vind];

                           lltemp  = (d_srotm_12*tempv +0x3fff)>>15;
                           tempGx2 = (int)lltemp;

                           lltemp  = (d_srotm_22*tempv +0x3fff)>>15;
                           tempGy2 = (int)lltemp;

                           lltemp  = (d_srotm_32*tempv +0x3fff)>>15;
                           tempGz2 = (int)lltemp;
                         }
                       }

                       /* Sixth: Gradient SS of 2nd overlapping gradient */

                       if (d_peS_step != 0)
                       {
                         if ( (*(pIntW6-1) == 32767) || (*(pIntW6-1) == -32767) )   // original mode for PE truncate for constant part of WFG
                         {
                           tempGx3 = ((d_srotm_13*d_peS_step +0x3fff)>>15)*rtVar[d_peS_vind];
                           tempGy3 = ((d_srotm_23*d_peS_step +0x3fff)>>15)*rtVar[d_peS_vind];
                           tempGz3 = ((d_srotm_33*d_peS_step +0x3fff)>>15)*rtVar[d_peS_vind];
                         }
                         else // new round off before PE mult for varying part of WFG
                         {
                           tempv   = d_peS_step*rtVar[d_peS_vind];

                           lltemp  = (d_srotm_13*tempv +0x3fff)>>15;
                           tempGx3 = (int)lltemp;

                           lltemp  = (d_srotm_23*tempv +0x3fff)>>15;
                           tempGy3 = (int)lltemp;

                           lltemp  = (d_srotm_33*tempv +0x3fff)>>15;
                           tempGz3 = (int)lltemp;
                         }
                       }

                       Gx += tempGx1 + tempGx2 + tempGx3 + d_statGx;
                       Gy += tempGy1 + tempGy2 + tempGy3 + d_statGy;
                       Gz += tempGz1 + tempGz2 + tempGz3 + d_statGz;

                    }

                       if ( (Gx > 32767) || (Gx < -32767) )
                       {
                          errLogRet(LOGIT,debugInfo,"X gradient value %d is outside +/-32767 range\n",Gx);
                          markAcodeSetDone(expName, curAcodeNumber);
                          errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
                          return (errorcode);
                       }

                       if ( (Gy > 32767) || (Gy < -32767) )
                       {
                          errLogRet(LOGIT,debugInfo,"Y gradient value %d is outside +/-32767 range\n",Gy);
                          markAcodeSetDone(expName, curAcodeNumber);
                          errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
                          return (errorcode);
                       }

                       if ( (Gz > 32767) || (Gz < -32767) )
                       {
                          errLogRet(LOGIT,debugInfo,"Z gradient value %d is outside +/-32767 range\n",Gz);
                          markAcodeSetDone(expName, curAcodeNumber);
                          errorcode = HDWAREERROR + GRADIENT_AMPL_OUTOFRANGE;
                          return (errorcode);
                       }

                       Gx &= 0xffff;  Gy &= 0xffff; Gz &= 0xffff;

                       *pgwfg = (XGRADKEY | Gx);
                        pgwfg++;
                       *pgwfg = (YGRADKEY | Gy);
                        pgwfg++;
                       *pgwfg = (ZGRADKEY | Gz);        /*  No latch, latch provided by REPEAT writes */
                        pgwfg++;

                       for (ll=0; ll<kk; ll++)
                       {
                          *pgwfg = gradgridword[ll];    /* REPEAT writes */
                           pgwfg++;
                       }

                       if ( (pgwfg - pGradWfg) >= 1000)
                       {
                         writeCntrlFifoBuf(pGradWfg,(pgwfg-pGradWfg));
                         pgwfg = pGradWfg;
                       }
                       else if (m == (ii-1))
                       {
                         writeCntrlFifoBuf(pGradWfg,(pgwfg-pGradWfg));
                       }
                    }
                 }
              }
              else
              {
                 return (errorcode);
              }

           peR_step = 0; peP_step = 0; peS_step = 0;
           break;
#endif

#ifdef LOCK_CNTLR
/* the lock no longer has a interpreter phase */
#endif

        case RECEIVERGAIN:
            DPRINT1(-1, " RECEIVERGAIN: gain = %d\n",*p2CurrentWord);
            i = *p2CurrentWord++;
            /* receivergain */
            break;

#ifdef MASTER_CNTLR

         case DUTYCYCLE_VALUES:
            {
               int n;
               DPRINT( 1,"DUTYCYCLE_VALUES\n");
               n = *p2CurrentWord++;
               sibSendDutyCycle(n, p2CurrentWord);
               p2CurrentWord += n;
             }
             break;

	case ROLLCALL:
            {

               char mcntlrs[MAX_ROLLCALL_STRLEN];    
               int nmissing;
               int j,ok;
               int retryCntDwn = 3;
               ok = 0;
	       i = *p2CurrentWord++;
               DPRINT(-1,"ROLLCALL\n");
               /* DPRINT2(-1,"size of string: %d words, %d bytes\n",i,i*4); */
               /* DPRINT1(-1,"used str: '%s'\n",(char*) *p2CurrentWord); */
               strncpy(secretword,(char*) p2CurrentWord,i*4);
               DPRINT1(-2,"ROLLCALL: Controllers used in PS: '%s'\n",(char*) secretword);

               /* remove all controllers that have respond to past rollcalls */
               /* so we can start with a blank slate and see who comes back  */
               /* cntlrStatesRemoveAll();  /* remove all controllers */
               cntlrStatesAdd("master1",CNTLR_READYnIDLE,MASTER_CONFIG_STD);

               /* rollcall(); /* invoke rollcall to get current reponding cntlrs */
	       /*  taskDelay(calcSysClkTicks(17));  taskDelay(1); */

               /* forget the above rollcal , rollcall was issue prior to intering the intrepeter so
                  lets assume rollcall has completed, if not we do catch it below , GMB 7/26/05  */

               p2CurrentWord += i;

               for(retryCntDwn = 3; retryCntDwn > 0; retryCntDwn--)
               {
                 mcntlrs[0] = 0;
                 nmissing = cntlrSet2UsedInPS(secretword, mcntlrs);
                 if (nmissing == 0)
                 {
		               ok = 1;
                    /* p2CurrentWord += i; */
		               break;
                 }
                 DPRINT2(-1,"ROLLCALL: %d missing, MIAs: '%s'\n",
			           nmissing,mcntlrs);
                 SendRollCallViaParsCom();
		           // rollcall();
	              taskDelay(calcSysClkTicks(166));   /* taskDelay(10) */
               }
               if (!ok)
               {
                  DPRINT(-1, "ROLLCALL: FAILED to Find required Controllers\n");
                  DPRINT2(-1,"ROLLCALL: %d missing, MIAs: '%s'\n", nmissing,mcntlrs);
                  sendExceptionViaParsCom(WARNING_MSG, WARNINGS+MISSINGCNTLRS, 0, 0, NULL);
               }
               else
               {
                    DPRINT(-1,"ROLLCALL: All required Controllers present.\n");
               }

#ifndef SEND_APARSE_EARLY
               /* previously APARSE was sent to all the other controller here in the Acode ROLLCALL
                * this delayed the start of parsing by the other controller by ~140 msec delaying the start (bug 4420)
                * the message was moved into AParser.c to send the message early  (delay ~ 6 msec)
                * [ all time can vary depending on sequence ] 
                * We ifdef this just incase problems are run into, an easy reverting back to the old is possible
                *    GMB   8/6/2010
                */
                DPRINT4(2,"real send2AllCntlrs: CNTLR_CMD_APARSER, numAcodes: %d, nTables: %d, startFid: %d, expid: '%s'\n",
                  pAcodeId->num_acode_sets, pAcodeId->num_tables,pAcodeId->cur_acode_set,pAcodeId->id);

#ifdef TIMING_DIAG_ON    /* compile in timing diagnostics */
               TSPRINT("Send APARSER to other cntrls:");
#endif
               // send2AllCntlrs(CNTLR_CMD_APARSER, pAcodeId->num_acode_sets,
				   //                pAcodeId->num_tables,pAcodeId->cur_acode_set,pAcodeId->id);
               send2AllCntlrsViaParsCom(CNTLR_CMD_APARSER, pAcodeId->num_acode_sets,
				                  pAcodeId->num_tables,pAcodeId->cur_acode_set,pAcodeId->id);

#endif   // SEND_APARSE_EARLY

              if (MixedRFError > 0) {
                  // sendException(HARD_ERROR, MixedRFError, 0, 0, NULL); // ICAT_ERROR + MIXED_RFS;
                  sendExceptionViaParsCom(HARD_ERROR, MixedRFError, 0, 0, NULL); // ICAT_ERROR + MIXED_RFS;
              }

            }
            break;

	case MASTER_CHECK:
            // see sibFuncs.c for more details.
	    XYZgradflag = (*p2CurrentWord++ >> 16);   /*XYZ thru grad amps? */
            i = sibGet('S') & 0xFBC0;  // eliminate 0x400, no data yet
                                       // eliminate 0x020,  IP Ready

	    if (XYZgradflag) {	// gradtype='rrr', must have green led on ISI
                if (i != 0) sibShowG(1);
            }
            if ( *p2CurrentWord++ )
               sibAbortEnable();
            else
               sibAbortDisable();

            // Reset the active channel bits
            pCurrentStatBlock->AcqChannelBitsActive1 = 0;
            pCurrentStatBlock->AcqChannelBitsActive2 = 0;
            channelBitsSet = 0;

	    break;

	case PNEUTEST:
            {  int pneuErr, pneuLock;
               DPRINT1(-1,"PNEUTEST pin=%d\n",*p2CurrentWord);
               pneuErr = getPneuFault();
               setPneuFaultInterlock(*p2CurrentWord);  // as 'pin' is set
               DPRINT2(-1,"pneuErr=%d, pneuLock=%d\n",pneuErr,*p2CurrentWord);
               if ( getPneuFault() )
               {  if ( *p2CurrentWord ) //if != 0, then HARD_ERROR or WARNING_MSG
                  // sendException(*p2CurrentWord,getPneuFault(),0,0,NULL);
                  sendExceptionViaParsCom(*p2CurrentWord,getPneuFault(),0,0,NULL);
               }
               *p2CurrentWord++;
            }
            break;
	case NSRVSET:

            i = *p2CurrentWord++;
            pCurrentStatBlock->AcqRecvGain = (short) ((i>>16) & 0xff);
            i = i & 0xff; /* retain only a few words */
            for (j=0; j < i; j++)
	      {
              k = *p2CurrentWord++;  //
              if ((k & 0xf000) == 0x700)
		{
                  k &= (~0x200);
                  k |= nsr.lkattn;
		}
	      hsspi(1, k);
              }
            break;

	case SETNSR:

            nsr.word1 = *p2CurrentWord;
            i = *p2CurrentWord++;
            hsspi(1,i);
            //----
            nsr.word2 = *p2CurrentWord & (~0x8); // remove lkattn bit
            j = *p2CurrentWord++ | nsr.lkattn;   //  | nsr.lkhi_lo;
            hsspi(1,j);
            //----
            k = *p2CurrentWord++;
            hsspi(1,k);
            //----
	    DPRINT3(-1,"SET NSR %x  %x  %x\n",i,j,k);
	    l = (*p2CurrentWord++) & 0xff;
	    pCurrentStatBlock->AcqRecvGain = (short) l;
            break;

            /*
             * READHSROTOR  RTvar
             *
             * Read the measured rotor period and place in RTvar.
             */
        case READHSROTOR:
           {
              int rtindex,delay;
              int rstat;
              i = *p2CurrentWord++;
              DPRINT(-1, "READHSROTOR \n");

              rtindex = *p2CurrentWord++;
              rtVar[rtindex] = masterRotorPhasedRead(); /* units of 100 nsec */
              DPRINT2(1,">>>  rotor speed (rtVar[%d]) = %d\n",
                rtindex,rtVar[rtindex]);
           }

            break;

            /*
             * MASTERLOCKSETTC  int
             *
             * Directly sets the lock servo time constants,  TBD
             * KEYS ETC...
             *
             */
        case MASTERLOCKSETTC:
            i = *p2CurrentWord++;
            DPRINT(-1, "Set Master Lock TC, a NOOP at present.\n");
            break;

            /*
             * MASTERLOCKHOLD  flag
             *
             * If flag is true,  hold the lock servo else it can
             * track. May implement lock on / off too... TBD
             */
        case MASTERLOCKHOLD:
            i = *p2CurrentWord++;
            DPRINT(-1, "Master Lock Hold,  a NOOP at present.\n");
            break;

	case SETSHIMS:
	   {
	     int i,num,time;
             int oneDac[5];
	     int zero;
             time = *p2CurrentWord++;
             setShimTimeDelay(time);
             num = *p2CurrentWord++;
             DPRINT2(-1," SETSHIM:  num=%d, time=0x%x\n",num,time);
             for (i=2; i<num; i++)
             {
//                printf("%d is %d\n",i,*p2CurrentWord);
		zero=0;
	        oneDac[0] = 13;
	        oneDac[1] = i;
	        oneDac[2] = *p2CurrentWord++;
		shimHandler(oneDac,&zero,3,1);   // count=1, fifoFlag=false
             }
           }
           break;

            /* SYNCSETSHIMS num(dacID, value) x num
             *
             * Set shims via the synchronous interface.Set num dacs with dacID,
             * value pairs.
             */
        case SYNCSETSHIMS:
           {  int i,num;
              int oneDac[5];
	      int zero;
              DPRINT(-1,"SYNCSETSHIM\n");
              num = *p2CurrentWord++;
              for (i=0; i<num; i++)
              {
//                printf("%d is %d\n",i,*p2CurrentWord);
		zero=0;
	        oneDac[0] = 13;
	        oneDac[1] = *p2CurrentWord++;
	        oneDac[2] = *p2CurrentWord++;
		shimHandler(oneDac,&zero,3, 1);   // count=1, fifoFlag=true
              }
           }
           break;
        case SMPL_HOLD:
           {  int fWord[5];
              if (*p2CurrentWord++)
                 auxLockByte |= AUX_LOCK_HOLD;
              else
                 auxLockByte &= ~AUX_LOCK_HOLD;
              auxLockByte &= 0xFF;		// 8 bits only;
              //now stuff it in the fifo
              DPRINT1(-1,"SMPL_HOLD byte=%x\n",auxLockByte);
              fWord[0] = (LATCHKEY | AUX | (1<<8) |  auxLockByte);
              writeCntrlFifoBuf(fWord,1);
           }
           break;
        case HOMOSPOIL:
           {  int fWord[5];
              if (*p2CurrentWord++)
                 auxLockByte |= AUX_LOCK_HOMOSPOIL;
              else
                 auxLockByte &= ~AUX_LOCK_HOMOSPOIL;
              auxLockByte &= 0xFF;		// 8 bits only;
              //now stuff it in the fifo
              DPRINT1(-1,"HOMOSPOIL byte=%x\n",auxLockByte);
              fWord[0] = (LATCHKEY | AUX | (1<<8) |  auxLockByte);
              writeCntrlFifoBuf(fWord,1);
           }
           break;
	case TNLK:
           {
              setTnEqualsLkState();
           }
           break;
        /*
         * LOCKCHECK errormode
         *
         * If loss of lock, send errormode.
         *
         */
        case LOCKCHECK:
	   DPRINT(-1,"LOCKCHECK");
	   setLkInterLk(*p2CurrentWord++);
	   break;
	// we get here if tn="lk" to set lockpower=0,
	// to stop the lock from pulsing
	// but LOCKINFO will record the true lockpower, to be reset later
	case LOCKINFO:
           set_shimset( *p2CurrentWord  & 0xFFFF); /*for autolock or autoshim*/
           XYZgradflag = (*p2CurrentWord >> 16);   /*XYZ thru grad amps? */
           establishShimType( *p2CurrentWord  & 0xFFFF);
           *p2CurrentWord++;
           { int oneDac[4];
             int zero=0;
             DPRINT1(-1,"z0 = 0x%x\n",*p2CurrentWord);
             oneDac[0] = 13;
             oneDac[1] = 1;
             oneDac[2] = *p2CurrentWord;
	     pCurrentStatBlock->AcqShimValues[1] = *p2CurrentWord++;
             shimHandler(oneDac, &zero, 3, 0);
           }

	   DPRINT1(-1,"lockgain = 0x%x\n",*p2CurrentWord);
           // send2Lock(LK_SET_GAIN, *p2CurrentWord++, 0, 0.0, 0.0);
           Send2LockViaParsCom(LK_SET_GAIN, *p2CurrentWord++, 0, 0.0, 0.0);

           DPRINT1(-1,"lockpower = 0x%x\n",*p2CurrentWord);
           if (*p2CurrentWord > 48) {
              /* clear the lock preamp attn bit */
	      hsspi(1, ((0xd<<11) | 0x1) );
           }
           else {
              /* set the lock preamp attn bit */
              hsspi(1, ((0xd<<11) | 0x0) );
           }
           /* the lock cntlr will deduct the 20 dB as needed */
           /* it sends the status back, so needs the true value */
           /* the lock cntlt will set pw=0 if power=0 */
           /* if tnlk_flag is set we need to keep power=0,  */
           /* but remember power for reset */
           tnlk_power = *p2CurrentWord;
           if ( ! tnlk_flag) {
              // send2Lock(LK_SET_POWER, *p2CurrentWord, 0, 0.0, 0.0);
              Send2LockViaParsCom(LK_SET_POWER, *p2CurrentWord, 0, 0.0, 0.0);
           }
           *p2CurrentWord++;

           DPRINT1(-1,"lockphase = 0x%x\n",*p2CurrentWord);
           // send2Lock(LK_SET_PHASE, *p2CurrentWord++, 0, 0.0, 0.0);
           Send2LockViaParsCom(LK_SET_PHASE, *p2CurrentWord++, 0, 0.0, 0.0);

           pCurrentStatBlock->AcqLockFreq1 = lscratch1.nword[0] = *p2CurrentWord++;
           pCurrentStatBlock->AcqLockFreq2 = lscratch1.nword[1] = *p2CurrentWord++;
           DPRINT3(-1,"lockfreq = %f 0x%x 0x%x\n",lscratch1.dword, lscratch1.nword[0], lscratch1.nword[1]);
           //send2Lock(LK_SET_FREQ, lscratch1.nword[0], lscratch1.nword[1],
           //                       lscratch1.dword, 0.0);
           Send2LockViaParsCom(LK_SET_FREQ, lscratch1.nword[0], lscratch1.nword[1],
                                  lscratch1.dword, 0.0);

           DPRINT1(-1,"lock mode word = 0x%x\n",*p2CurrentWord);
           saveTcValues( (*p2CurrentWord & 0xFF),((*p2CurrentWord>>8) & 0xFF) );
           temp = (*p2CurrentWord>>16) & 0xFF;
           if (temp & AUX_LOCK_LOCKON)	// if alock='u'
              auxLockByte = temp;	// unlocked, as set in temp
           else				// else use as is
           {  if (auxLockByte & AUX_LOCK_LOCKON)
                 auxLockByte = temp | AUX_LOCK_LOCKON;
              else
                 auxLockByte = (temp & (~AUX_LOCK_LOCKON) ) & 0xFF;
           }
           {  int fWord[5];
              fWord[0] = (LATCHKEY | AUX | (1<<8) |  auxLockByte);
              writeCntrlFifoBuf(fWord,1);
           }
           p2CurrentWord++;
           break;

        /*
         * TINFO
         * The ultimate oo. There are two case with TINFO, the compiler will
         * separate these. One is for the Master, one for all RF controlers.
         * Each object in PSG writes info under this acode, although
         * different info, different format.
         * The master gets the NSR mixer band (0=hi, 1=low) for all channels.
         * The RF get xmtr mixer band and tune power setting for one channel.
         * It all end up in the right place.
         *
         * Also pass the bearing air on level.
         * This case may become the catch all in the future
         */
        case TINFO:
          {   int numrfch;
              numrfch = *p2CurrentWord++;
              for (i=0; i<numrfch; i++)
              {
                  nsrMixerBand[i] = *p2CurrentWord++;
              DPRINT2(-1,"TINFO %d=%d\n",i,nsrMixerBand[i]);
              }
              setBearingLevel(*p2CurrentWord++);
              newVTAirFlow(*p2CurrentWord++);
              setVTAirLimits(*p2CurrentWord++);
              setSpinnerType(*p2CurrentWord++);
              testAndClearPneuFault();
          }
          break;
	  /* SSHA - NOW BLAF - only involves Master1 */
        case SSHA:
	   i = *p2CurrentWord++;
           j = *p2CurrentWord++;
           k = *p2CurrentWord++;
           send2Blaf(i,j,k); /* command, shims, options */
        break;
        case SETACQSTATE:
	   i = *p2CurrentWord++;
           setAcqState( i );
        break;



#endif

	case MRIUSERBYTE:
	   {
	    int rw, i, ticks, auxVal;
	    rw = *p2CurrentWord++;	// read or write?
	    i  = *p2CurrentWord++;	// rtVar index
            DPRINT3( 1,"MRIUSERBYTE - rw=%d rtVar[%d]=%d", rw, i, rtVar[i]);
	    if (rw==1)	// 0 = read, 1 = write 2=fast resd
	    {
              auxVal = (LATCHKEY | AUX | (3<<8) |  (rtVar[i] & 0xFF) );
              writeCntrlFifoWord(auxVal); 	// synchronous
	    }
            else if (rw==2)
            {
                readuserbyte=rtVar[i];  //THIS IS A READUSERBIT CALL
              priorityInversionCntDwn = mripricnt; /* interprete 10 Acodes then switch back to normal priority */
              ticks = *p2CurrentWord++;
              queueMRIRead(i, ticks, pAcodeId);
if(BrdType != MASTER_BRD_TYPE_ID){
if(warningAsserted==1) {rtVar[i]=1; warningAsserted=0;}
else {rtVar[i]=0; warningAsserted=0;}
}
              DPRINT2( -2,"MRIUSERBYTE - rtVar[%d]=%d",  i, rtVar[i]);
            }
	    else if (rw==0)  //normal MRIBYTE
	    {
                readuserbyte=0;
              priorityInversionCntDwn = mripricnt; /* interprete 10 Acodes then switch back to normal priority */
              ticks = *p2CurrentWord++;
	      queueMRIRead(i, ticks, pAcodeId);

            }
	   }
	    break;
            /*
             * ROTOSYNC_TRIG  flag,  count or RTvar
             *
             * If flag = 0,  load the next value as a rotosync
             * period else load the contents the real time
             * variable.
             */
        case ROTORSYNC_TRIG:
            {
              int rtparmflg, rtindex, count;
              int num,postdelay;

              DPRINT(-1, " ROTORSYNC_TRIG\n");
              rtparmflg = *p2CurrentWord++;
              rtindex = *p2CurrentWord++;  /* index or count depending on flag rtparmflg */
              postdelay = 8;  /* 100 nsec */

              DPRINT1(-1,"ROTORSYNC: rtflag = %d\n",rtparmflg);
              if (rtparmflg)
                 count = rtVar[rtindex];
              else
                 count = rtindex;

              DPRINT1(-1,"ROTORSYNC: count = %d\n",count);
              if ( count > 0)
              {
                 num = RotorSync(count,postdelay);
              }
            }
            break;

/* -- now all controller see GETSAMP & LOADSAMP -- so su/change  works properly */

        case GETSAMP:
          {
              int bumpFlag;
              int ret,skipsampdetect;
              unsigned long sample2ChgTo;
              /* the sample number is now 2 shorts and encodes Gilson psotional & type information that
                 roboproc needs
                 long  100 - 1    Location
                       10k - 1k   Sample Type
                      1m - 100k   Rack Type
                            10m   Rack Position
                           100m   Zone of rack
              */

              sample2ChgTo = *p2CurrentWord++;
              skipsampdetect = *p2CurrentWord++;
              bumpFlag = *p2CurrentWord++;

              queueSamp(GETSAMP,sample2ChgTo,skipsampdetect,pAcodeId,bumpFlag);
              DPRINT(0,"==========> GETSAMP: Resume Parser\n");
           }
           break;

        case LOADSAMP:
          {
              int bumpFlag,spinActive;
              int ret,skipsampdetect;
              unsigned long sample2ChgTo;

              /* the sample number is now 2 shorts and encodes Gilson psotional & type information that
                 roboproc needs sampleloc is an encoded number

                 long  100 - 1    Location
                       10k - 1k   Sample Type
                      1m - 100k   Rack Type
                            10m   Rack Position
                           100m   Zone of rack

              */

              sample2ChgTo = *p2CurrentWord++;
              skipsampdetect = *p2CurrentWord++;
              spinActive =  *p2CurrentWord++;
              bumpFlag = *p2CurrentWord++;
              queueLoadSamp(LOADSAMP,sample2ChgTo,skipsampdetect,pAcodeId,
                              spinActive,bumpFlag);
              DPRINT(0,"==========> LOADSAMP: Resume Parser\n");
          }
          break;

        case PFGSETZLVL:
          {
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
              int encoded = *p2CurrentWord++;
              int key = encoded >> 26;
              int value = (int)((short) (encoded & 0xFFFF));
              int corrected = value < 0 ? value + zgradcorrlo : value > 0 ? zgradcorrhi + value : 0;
              int capped = corrected < 0 ? MAX(corrected, SHRT_MIN) : MIN(corrected, SHRT_MAX);
              int reencoded = (key << 26) | (capped & 0xFFFF);
              
              DPRINT3(-1, "setting PFGZLVL[%d] to %d corrected to %d\n", key, value, capped);
              writeCntrlFifoBuf(&reencoded, 1);
          } 
          break;

        case PFGSETZCORR:
           DPRINT(-1,"PFGSETZCORR\n");
           zgradcorrhi = *p2CurrentWord++;
           zgradcorrlo = *p2CurrentWord++;
           DPRINT2(-1, "setting PFGZCORR to <%d,%d>\n", zgradcorrlo, zgradcorrhi);
           break;

        default:
           if ((action & 0xFFF00000) == 0xacd00000)
           DPRINT2(-1, "unrecognized acode: 0x%x, %d\n", action,(action & 0xfffff));
	   break;
        }
    }
    errLogRet(LOGIT,debugInfo,
		"Fell out of Interpreter 'while'. This is an ERROR.\n");
    markAcodeSetDone(expName, curAcodeNumber);
    errorcode = HDWAREERROR + INVALIDACODE;
    return (errorcode); /* for the READER ONLY */
}

SendZeroFid(int srctag)
{
    PUBLSH_MSG pubMsg;

    // FID_STAT_BLOCK *pStatBlk;
    // pStatBlk =  dataGetStatBlk(pTheDataObject, srctag);
    // pStatBlk->doneCode = EXP_FIDZERO_CMPLT;  // If recieve proc needs an indicator
    // pStatBlk->errorCode = 0;
    // pStatBlk->dataSize = 0;
    // pStatBlk->fidAddr =  (int *) -1;

    DPRINT1(-1,"SEND_ZERO_FID:  Tag: %d\n", srctag);
    pubMsg.tag = srctag; 
    pubMsg.dspIndex = srctag;
    pubMsg.crc32chksum = 0;
    // start transfer immediately, just the stat block would be sent, Recvproc must know what to do.
    msgQSend(pDataTagMsgQ,(char*) &pubMsg, sizeof(pubMsg), WAIT_FOREVER, MSG_PRI_NORMAL);
    // taskDelay(30);
    return OK;
}


#ifndef MASTER_CNTLR

#ifdef DDR_CNTLR
void syncAction(long signaltype)
{
   int tag;
   DPRINT1(-1,"DDR syncAction\n",signaltype);
   switch (signaltype) 
   {
       case SEND_ZERO_FID:
	         rngBufGet(pSyncActionArgs,(char*) &tag,sizeof(int));
            DPRINT1(-1,"DDR syncAction: SEND_ZERO_FID: tag=%d\n",tag);
            taskDelay(120); // delay 2 sec
            SendZeroFid(tag);
       break;
case MRIUSERBYTE:
if((readuserbyte&0xff)!=0){
   semGive(pTheAcodeObject->pSemParseSuspend);
resetShandlerPriority();
}
 break;

       default:
             break;
   }
   return;
}
#else
void syncAction(long signaltype)
{
switch (signaltype)
 {
     case MRIUSERBYTE:
if((readuserbyte&0xff)!=0){
       semGive(pTheAcodeObject->pSemParseSuspend);
       resetShandlerPriority();
}
     break;
     default:
     break;
}
 return;
}
#endif


/* for non master AParser or interperters setting the status does nothing
   only the master changes these states */
setAcqState( int acqstate)
{
}

setFidCtState(int fidnum, int ct)
{
}
void setVTtype(int type)
{
}
void spinvalueSet(int speed, int bump)
{
}
void setLkInterLk(int in) {}
void setVTinterLk(int in) {}
void setSpinInterlk(int in) {}
#endif

#ifndef DDR_CNTLR
  /* DDR has a special version of this function */
zeroFidCtState() {}
#endif

/*----------------------------------------------------------------------*/
/*  This routine can be used for deciding when to perform either        */
/*   autoshim or resetting DACs since the mask has the same meaning in  */
/*   both cases.							*/
/*									*/
/*  chkshim(mask,ct,sampchg):	 					*/
/*		Check Shim  or DAC Mask for when to do auto shimming  	*/
/*		or RESETTING the Shim DACs				*/
/*	        should be performed 					*/
/*	  Return 0 - if don't perform operation				*/
/*		 1 - to perform operation  				*/
/*----------------------------------------------------------------------*/

 chkshim(mask,ct,sampchg)
 int mask,*sampchg;
 long ct;
  {
    int doit;
    doit = FALSE;
    switch (mask)
      {
	case 1:
	case 2:
		if (ct == 0L)  /* If New Exp. or FID, Autoshim */
	         {
/*s*/             DPRINT(0,"Shim EXP or FID\n");
		   doit = TRUE;
	     	 }
		break;
	case 4:
		/* if (lgostat == BSCODE)  /* If Done Block Size, Autoshim */
	         {
/*s*/             DPRINT(0,"Shim BS\n");
		   doit = TRUE;
	     	 }
		break;
	case 8:
		if (*sampchg) 		/* if sample changed, Autoshim */
		 {
/*s*/             DPRINT(0,"Shim SAMP\n");
		   doit = TRUE;
		   *sampchg = 0;  /* only one autoshim in a go*/
                 }
		break;

	default: break;
      } /* end mask switch */

    return(doit);
  } /* end of chkmask */

/*----------------------------------------------------*/
/* for gradient shimming we set lock_hold, and turn   */
/* lockpower to zero, this stops the pulses,          */
/* when exp completes/aborts this routine resets both */
/*----------------------------------------------------*/
void setTnEqualsLkState()
{
#ifdef MASTER_CNTLR
DPRINT(-1,"==>>>>><<<<<TNLK\n");
   tnlk_flag = 1;
   auxLockByte |= AUX_LOCK_HOLD;
   auxLockByte &= 0xFF;		// 8 bits only;
   set2sw_fifo(0);
   auxWriteReg(AUX_LOCK_REG, auxLockByte);
   set2sw_fifo(1);

   /* the lock cntlt will set pw=0 if power=0 */
   setpower(-1);
#endif
}

void resetTnEqualsLkState()
{
#ifdef MASTER_CNTLR
DPRINT2( 1,"resetTnEqualsLkState() %d %d\n",tnlk_flag, tnlk_power);
   if ( ! tnlk_flag) return;	/* nothing doing */
   pCurrentStatBlock->AcqLockPower = tnlk_power; /* prevent a race condition */
   setpower(tnlk_power);	/* reset to original lock pwer */

   taskDelay(calcSysClkTicks(17)); /* give the packet a change to arrive */

   lockHoldOff();		/* clear lock hold bit on aux bus */

   tnlk_flag = 0;		/* we're back to normal */
#endif
}

void sendInfo2ConsoleStat(int tickmismatch)
{
#ifdef MASTER_CNTLR
   pCurrentStatBlock->AcqTickCountError = tickmismatch; 
   pCurrentStatBlock->AcqChannelBitsActive1 = 0;
   pCurrentStatBlock->AcqChannelBitsActive2 = 0;
   DPRINT(1,"sendInfo2ConsoleStat: set active channel bits to 0 & send to ConsoleStat\n");
#endif
}

