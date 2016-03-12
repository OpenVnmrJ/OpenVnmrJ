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
/*
DESCRIPTION

   These functions Handler internal Data acquisition and/or Processing


*/

#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */
#include <string.h>
#include <vxWorks.h>
#include <stdioLib.h>
#include <semLib.h>
#include <msgQLib.h>
#include "hostAcqStructs.h"
#include "hostMsgChannels.h"
#include "expDoneCodes.h"
#include "errorcodes.h"
#include "logMsgLib.h"
#include "namebufs.h"
#include "hardware.h"
#include "fifoObj.h"
#include "stmObj.h"
#include "adcObj.h"
#include "AParser.h"
#include "autoObj.h"
#include "acodes.h"
#include "A_interp.h"
#include "timeconst.h"
#include "lock_interface.h"	/* MERCURY */

extern MSG_Q_ID pUpLnkHookMsgQ; /* MsgQ used between Data process & STM Obj */
extern MSG_Q_ID pMsgesToPHandlr;/* MsgQ for Msges to Problem Handler */
extern MSG_Q_ID pMsgesToHost;	/* MsgQ used for Msges to routed upto Expproc */

/* Hardware Objects */
extern FIFO_ID		pTheFifoObject;
extern STMOBJ_ID	pTheStmObject;
extern ADC_ID		pTheAdcObject;
extern AUTO_ID		pTheAutoObject;

/* Exception Msges to Phandler, e.g. FOO, etc. */
extern EXCEPTION_MSGE HardErrorException;
extern EXCEPTION_MSGE GenericException;

/*
static struct _fidparms  {
	short  fidntindx;
	short  fidnpindx;
	short  fiddpflag;
	ushort_t  fidac_offset;
	ACODE_ID  fidpAcodeId;
			 } fidparms;
*/

static short	fidntindx, fidnpindx;
static ushort_t	fidac_offset;
static int	fiddpf;
static ulong_t  fidnp;
static ACODE_ID fidpAcodeId;

void
datanextscan(ulong_t np, int dp, ulong_t nt, ulong_t ct, int bs, int *dtmcntrl, ulong_t fidnum);
extern  int  get_acqstate();                  /*  in /sysvwacq/monitor.c  */
extern  ushort  getRcvrmode();

getlkfid(long *data,int nt,int filter)
{
  ITR_MSG itrmsge;
  ACODE_OBJ AcodeId;
  FID_STAT_BLOCK *pStatBlk;
  MSG_Q_ID pPrevMsgQ;
  STMOBJ_ID  pPrevStmObj;
  int np,ssct,ct,bs,cbs,dp,ss,maxsum;
  unsigned long npleft,dstaddr;
  unsigned long adccntrl;
  int dtmcntrl;
  short ntag;
  int dwell,Tag,stat;
  long *dataAddr;

  np=512;    /* number of points for the scan                 */
  ss = ssct = 0;  /* Pointer to Steady State Completed Transients  */
  nt=1;      /* Number of Transients.                         */
  ct = 0;    /* Pointer to Completed Transients               */
  bs = 0;	   /* blocksize                                     */
  cbs = 0;   /* Completed blocksizes                          */
  dp = 4;    /* bytes per data point                          */
  maxsum = 0x7fffffff;
  dwell = CNT312_5USEC<<16;
  adccntrl = ADC_AP_ENABLE_CTC | (LOCK_CHAN << ADC_AP_CHANSELECT_POS);


  pPrevStmObj = pTheStmObject;  /* Save this STM Object, to allow us to restore when done */
  /* If HS STM/ADC selected then switch back to standard STM to acquire lock fid */
  if (stmIsHsStmObj(pTheStmObject))  
  {
     DPRINT(-1,"Switch Back to Standard STM\n");
     pTheStmObject = stmGetStdStmObj();
  }

  AcodeId.interactive_flag = 0;

  UpLnkHookMsgQClear();

  init_fifo(&AcodeId, 0, 0);    /* OK */

  pPrevMsgQ = stmChgMsgQ(pTheStmObject, pUpLnkHookMsgQ);

  init_adc(pTheAdcObject, 0, &adccntrl, 1);

   /*  Set the lock systems duty cycle to 20 Hz, */
 
  fifoStuffCmd(pTheFifoObject,CL_DELAY,CNT5_USEC);  /* 5 us delay */

  if (filter <= 16)
  {
     setLockPar(SET_LK20HZ,0,FALSE); /* MERCURY, 20 Hz mode, 400 Hz audio */
     dwell = CNT312_5USEC<<16;
  }
  else
  {
     setLockPar(SET_LK2KCF,0,FALSE); /* MERCURY, 5 kHz mode, 4 Hz audio */
     dwell = CNT12_5USEC<<16;
  }
  fifoStuffCmd(pTheFifoObject,CL_DELAY,CNT5_USEC);  /* ~5usec for duty cycle change */
     
  initscan(&AcodeId, np, ss, &ssct, nt, ct, bs, cbs, maxsum);
/*  writeapword(0x0e86, 0x0,AP_MIN_DELAY_CNT ); /* put DSP in pass through mode */

  /* fidnum,ct,np,nt,dp */
  /* nextscan(int np,int dp,int nt,int ct,int bs,int *dtmcntrl,int fidnum) */
  /* datanextscan(ulong_t np, int dp, ulong_t nt, ulong_t ct,
  /* 		 int bs, int *dtmcntrl, ulong_t fidnum) */
  datanextscan((ulong_t) np, dp, (ulong_t) nt, (ulong_t) ct, bs, &dtmcntrl, 1L);

  fifoStuffCmd(pTheFifoObject,CL_DELAY,CNT40_MSEC);  /* 40 ms delay */

/*  enableOvrLd(ssct, ct, &adccntrl); */
  acquire(0, np, dwell);
/*  disableOvrLd(&adccntrl); */

  /* ss == 0, nt == 1, bs == 0, cbs == 0 */
  /* endofscan(int ss, int *ssct, int nt, int *ct, int bs, int *cbs) */
  endofscan(ss, &ssct, nt, &ct, bs, &cbs);  /* give RTZ STM Intrp */

  fifoStuffCmd(pTheFifoObject,HALTOP,0);

/*
/*  memset(0x10000000,0,np*dp);
/*  memset(data,0,np*dp);
*/

   /* taskDelay(120);  */
   /* fifoStart(pTheFifoObject); */

  fifoStartSync(pTheFifoObject);
  fifoWait4StopItrp(pTheFifoObject);

/*msgQReceive(pUpLnkHookMsgQ,(char*) &itrmsge,sizeof(ITR_MSG), WAIT_FOREVER);*/
  DPRINT(1,"Wait for Message\n");
  if (msgQReceive(pUpLnkHookMsgQ,(char*)&itrmsge,sizeof(ITR_MSG), 120) != ERROR)
  {
     Tag = stmGetNxtFid(pTheStmObject,&itrmsge,&pStatBlk,&dataAddr,&stat);

     DPRINT1(1,"Got Messge, Tag: %d\n",Tag);
     if (itrmsge.msgType != INTERRUPT_OCCURRED )
       DPRINT(0,"Big Trouble not an interrupt");
  }
  else
  {
    errLogSysRet(LOGIT,debugInfo,"getlkfid: Did Not Receive data.");
    return(1);
  }

  /* read some status from STM */
  npleft = stmNpCntReg(pTheStmObject);
/*   dstaddr = stmDstAddrReg(pTheStmObject); NOMERCURY */
  ntag = stmTagReg(pTheStmObject);

  DPRINT4(1,"Tag: %d, npleft: %ld, dstaddr: 0x%lx (0x%lx)\n",
	ntag,npleft,dstaddr,(dataAddr + (long)(np*dp)));

  if (npleft != 0)
    DPRINT(0,"NP != to Acquired Points\n");

  DPRINT3(1,"data: 0x%lx, dataAddr: 0x%lx, bytes: %d\n",data,dataAddr,np*dp);
  /* copy data into memory location given */
  memcpy((char*)data,(char*)dataAddr,np*dp);

  stmFreeFidBlk(pTheStmObject,Tag);

  /* reset lock duty cycle back to 2K from 20 Hz */
  setLockPar(SET_LK2KCS, 0, FALSE);

  fifoStuffCmd(pTheFifoObject,HALTOP,0);
 /*   taskDelay(120);  */
  fifoStart(pTheFifoObject);
  fifoWait4Stop(pTheFifoObject);
  stmChgMsgQ(pTheStmObject, pPrevMsgQ);
  pTheStmObject = pPrevStmObj;  /* Restore Previous STM Object */

  return( 0 );
}

/*--------------------------------------------------------------------*/
/* datanextscan
/* 	Sends stm information to the apbus: source, destination addr 	*/
/*	Sets phase to 0  						*/
/*	zeroes source memory.						*/
/*	Arguments:							*/
/*		tag	: Tag number to use 				*/
/*		data	: where to put lock fid 			*/
/*		dp	: precion dp=2 - single  			*/
/*----------------------------------------------------------------------*/
void
datanextscan(ulong_t np, int dp, ulong_t nt, ulong_t ct, int bs, int *dtmcntrl, ulong_t fidnum)
{
  unsigned long Codes[50];
  unsigned short stm_control_word;
  long tag2snd;
  int cnt,endct,modct;
  unsigned long scan_data_adr;
  FID_STAT_BLOCK *p2statb;

   p2statb = stmAllocAcqBlk(pTheStmObject, 
	   	(ulong_t) fidnum, (ulong_t) np, (ulong_t) ct, (ulong_t) nt, 
		(ulong_t) nt,
	   	(ulong_t) (np * dp), (long *)&tag2snd, &scan_data_adr );

   /* Send tag word */
   cnt = stmGenTagCodes(pTheStmObject, tag2snd, Codes);
 
   /* set source Address of Data */
   /* cnt += stmGenSrcAdrCodes(pTheStmObject, scan_data_adr, &Codes[cnt]);
   /* set Destination Address of Data */
   /* cnt += stmGenDstAdrCodes(pTheStmObject, scan_data_adr, &Codes[cnt]);
   /* MERCURY */

   /* generate source and destination fifo words */
   cnt += stmGenSrcDstAdrCodes(pTheStmObject, scan_data_adr,
                        scan_data_adr, &Codes[cnt]);

   fifoStuffIt(pTheFifoObject, Codes, cnt);  /* into Fifo Buffer */
         
   /* reset num_points */
   /* stm_control_word =  STM_AP_RELOAD_NP | STM_AP_RELOAD_ADDRS; */
   stm_control_word =  STM_AP_RELOAD_NP_ADDRS;
   cnt = stmGenCntrlCodes(pTheStmObject, stm_control_word, Codes);
         
   /* set phase */
   /* stm_control_word = 0 & STM_AP_PHASE_CYCLE_MASK; */
   /* Enable adc and stm */
   /* stm_control_word = 
   /*                stm_control_word | STM_AP_ENABLE_ADC1 | STM_AP_ENABLE_STM; 
   /* set precision if single precision (2 bytes) */
   /* if (dp == 2)
   /*      stm_control_word = stm_control_word | STM_AP_SINGLE_PRECISION;
                
   /* zero source data if steady state first transient (ct == 0) */
   stm_control_word = STM_AP_MEM_ZERO;

   /* save stm control word */
   *dtmcntrl = stm_control_word;

   cnt += stmGenCntrlCodes(pTheStmObject, stm_control_word, &Codes[cnt]);

   fifoStuffIt(pTheFifoObject, Codes, cnt);  /* into Fifo Buffer */

   DPRINT2(1,"datanextscan: Tag: %d, Src*dst: 0x%lx\n",tag2snd, scan_data_adr);

   return;
}

tstlk()
{
    long data[1025],nt;
    int filter, i;
    nt=1;
    filter = 16;
    getlkfid(data,nt,filter);
    taskDelay(120);
    for (i=0; i < 30;i++)
      printf("data[%d]= %ld\n",i,data[i]);
}

/*******************************************************************
* do-autogain
*    preform autogain procedure
*    1. create private copies of Acode Object and RT parameter table
*    2. Copy orignals into the private ones
*    3. COnfigure for autogain
*    4. Call APint()
*    5. Get messages (typical a. NOISE acquire, b. ADC/Recv Overflow (if occurs)
*			      c. FID acquire )
*    6. calc new gain
*    7. if delta <= 4 then done, else keep going
*/
do_autogain(ACODE_ID pAcodeId, short ntindx, short npindx, short dpfindx,
	    short recvgindx, short apaddr, short apdly, short maxval,
	    short minval, short stepval, short adcbits,
	    ushort_t ac_offset)
{
  int Tag,stat;
  long *dataAddr;
  ITR_MSG itrmsge;
  FID_STAT_BLOCK *pStatBlk;
  MSG_Q_ID pPrevMsgQ;
  long fidAbsMaxDP(long *dataAddr,long np);
  long fidAbsMaxSP(short *dataAddr,long np);
  long *rt_base, *rt_tbl; /* pointer to realtime buffer addresses */
  unsigned int	*tmp_rtvar_base;
  long fidmax;
  unsigned long targetlevel;
  unsigned short *ac_cur,*ac_base; /* acode pointers */
  int delta,gain,maxgain,overload,rem,dpf;
  int rstatus = 0;
  int old_stat;
  int fifoexpstartstate;

  ACODE_OBJ AutoGainAcodeId;    /* private autogain acode object */

  UpLnkHookMsgQClear();

  old_stat = get_acqstate();
  fifoexpstartstate = fifoGetStart4Exp(pTheFifoObject); /* save state of fifo started for Exp flag */
  update_acqstate(ACQ_AGAIN);

  maxgain = maxval;

  DPRINT(0,"do_autogain: =================================== \n");
  DPRINT3(0,"Orig. Acode Obj: 0x%lx, ac_base = 0x%lx, rt_base = 0x%lx\n",
	pAcodeId, pAcodeId->cur_acode_base, pAcodeId->cur_rtvar_base);

  ac_base = pAcodeId->cur_acode_base;
  ac_cur = ac_base + ac_offset;

  rt_base = (long *) pAcodeId->cur_rtvar_base;
  rt_tbl=rt_base+1;
  DPRINT4(0,">>>  nt (rt_tbl[%d]) = %d,  np (rt_tbl[%d]) = %d\n",
	ntindx,rt_tbl[ntindx], npindx, rt_tbl[npindx]);
  DPRINT4(0,">>>  dpf (rt_tbl[%d]) = %d, recvgain (rt_tbl[%d]) = %d\n",
	dpfindx,rt_tbl[dpfindx],recvgindx,rt_tbl[recvgindx]);
  DPRINT6(0,">>>  apaddr: 0x%x, apdly: %d, rgain max: %d, min: %d, step: %d, adcbits: %d\n",
	apaddr,apdly,maxval,minval,stepval,adcbits);
  DPRINT2(0,"FID Acquire Acode location: 0x%lx, Acode = %d\n",
            ac_cur+1,*(ac_cur+1));
/*
  DPRINT2(1,"Current Acode location: 0x%lx, Acode = %d\n",
            ac_cur,*ac_cur);
*/
  DPRINT1(1,"RT Table size: %d\n", pAcodeId->cur_rtvar_size);

  DPRINT2(0,">>> ADC Effective Bits: %d, Max Pos Value: %ld\n",adcbits,((long) (1L << (adcbits-1))));
  targetlevel = (((long) (1L << (adcbits-1)))) / 2L ;	/* calc 2 to adcbits  / 2, 16bits/2 */
  DPRINT2(0,">>> FID target level [(1 << %d)/2]: %lu\n",adcbits-1,targetlevel);

  /* make private rt variable table */
  tmp_rtvar_base = (unsigned int *)malloc((pAcodeId->cur_rtvar_size) * sizeof(long));

  /* copy present rt variable table into private one */
  memcpy(tmp_rtvar_base,pAcodeId->cur_rtvar_base,(pAcodeId->cur_rtvar_size * sizeof(long)));

  /* copy present Acode Object into private one */
  memcpy(&AutoGainAcodeId,pAcodeId,sizeof(ACODE_OBJ));
  DPRINT3(1,"Copy of Acode Obj: 0x%lx, ac_base = 0x%lx, rt_base = 0x%lx\n",
	&AutoGainAcodeId, AutoGainAcodeId.cur_acode_base, 
	AutoGainAcodeId.cur_rtvar_base);
   
  /* setup Private Acode object for Autogainning */
  AutoGainAcodeId.interactive_flag = ACQ_AUTOGAIN;

  AutoGainAcodeId.cur_acode_base = ac_cur; /* just after AUTOGAIN */
  AutoGainAcodeId.cur_rtvar_base = tmp_rtvar_base; /* set to private table */
  AutoGainAcodeId.cur_acode_set = AutoGainAcodeId.num_acode_sets = 1;
  DPRINT3(1,"Private Acode Obj: 0x%lx, ac_base = 0x%lx, rt_base = 0x%lx\n",
	&AutoGainAcodeId, AutoGainAcodeId.cur_acode_base, 
	AutoGainAcodeId.cur_rtvar_base);
  rt_base = (long *) tmp_rtvar_base;
  rt_tbl=rt_base+1;
  DPRINT4(1,">>>  nt (rt_tbl[%d]) = %d,  np (rt_tbl[%d]) = %d\n",
	ntindx,rt_tbl[ntindx], npindx, rt_tbl[npindx]);
  DPRINT2(1,">>>  recvgain (rt_tbl[%d]) = %d\n",
         recvgindx,rt_tbl[recvgindx]);


  /* set STM & ADC Intrp MsgQ to Our  private msgQ */
   pPrevMsgQ = stmChgMsgQ(pTheStmObject, pUpLnkHookMsgQ);

   dpf =  rt_tbl[dpfindx];
   gain = rt_tbl[recvgindx];
   while(rstatus == 0)
   {

     /* copy original rt variable table into private one */
     memcpy(tmp_rtvar_base,pAcodeId->cur_rtvar_base,(pAcodeId->cur_rtvar_size * sizeof(long)));
     rt_tbl[ntindx] = 1;  /* set nt = 1 */
     rt_tbl[recvgindx] = gain;  /* new gain value */

     adcItrpDisable(pTheAdcObject,ADC_ALLITRPS);
     stmItrpDisable(pTheFifoObject,  MAX_SUM_ITRP_MASK );

     DPRINT(0,"-------------- Calling APint() --------------------- \n");
     fifoClrStart4Exp(pTheFifoObject);
     overload = 0;
     adcOvldClear(pTheAdcObject); /* clr any overload & reenable ADC intrps */
     APint(&AutoGainAcodeId);  /* Ahh, the miracle of re-entrent code */
     fifoWait4Stop( pTheFifoObject );

     while(1)
     {
       DPRINT1(0,"=========   Wait for Message  Q'd: %d  =============\n",
		  msgQNumMsgs(pUpLnkHookMsgQ));
       msgQReceive(pUpLnkHookMsgQ,(char*) &itrmsge,sizeof(ITR_MSG), 
			WAIT_FOREVER);
       Tag = stmGetNxtFid(pTheStmObject,&itrmsge,&pStatBlk,&dataAddr,&stat);
       DPRINT5(0,
	"itrmge: donecode %d(warn-%d), errorcode %d(adcov-%d), msgType %d\n",
		  itrmsge.donecode, WARNING_MSG, itrmsge.errorcode, 
		  WARNINGS + ADCOVER,itrmsge.msgType);

       DPRINT1(1,"Got '%s' FID\n",
		((pStatBlk->elemId == 0L) ? "Noise" : "Autogain"));
       DPRINT5(1,
       "AutoGain Tag: %ld, FID: %ld, CT: %ld, doneCode: %d, errorCOde: %d\n",
	    Tag,pStatBlk->elemId,pStatBlk->ct,pStatBlk->doneCode,
		pStatBlk->errorCode);
       DPRINT4(1,
       "         FID: %ld, CT: %ld, NP: %ld, fid Size: %ld\n",
                pStatBlk->elemId,pStatBlk->ct, pStatBlk->np,
                pStatBlk->dataSize);
       DPRINT1(1,
       "    dataAddr: 0x%lx\n",dataAddr);
       if ( (Tag != -1) && (pStatBlk->elemId != 0L))
       {
         break;
       }
       if ((Tag != -1))
       {
         DPRINT1(1,"stmFreeFidBlk: tag %d\n",Tag);
         stmFreeFidBlk(pTheStmObject,Tag);
       }
       if ((Tag == -1) && (itrmsge.donecode == WARNING_MSG) && 
	    ( (itrmsge.errorcode == WARNINGS + ADCOVER) ||
	      (itrmsge.errorcode == WARNINGS + RECVOVER) )
	  )
       {
          overload = 1;
          /* if overload and gain is already zero then report autogain
	      failure */
	  /* if (gain == 0)  */
	  if (gain == minval) 
	  {
	     /* the non-zero rstatus will terminate while() */
	     rstatus = AGAINERROR + AGAINFAIL;  
	  }
          /* maxgain = gain - 12; */
          maxgain = gain - 2;    /* slp change, 12 to 2 9/27/95 */
          if (maxgain < minval) maxgain = minval;
          gain = minval;
          DPRINT3(0,"ADC/Recv OverFlow: tag %d, maxgain: %d, gain: %d\n",
			Tag,maxgain,gain);
       }
     }

     if (!overload)
     {
       DPRINT1(2,"Msg in Q: %d\n",msgQNumMsgs(pUpLnkHookMsgQ));
       DPRINT(0,">>>>>>>>>   Calc Gain Delta  <<<<<<<<<<<<<\n");
       if (dpf == 4)
       {
	  DPRINT2(0,"call fidAbsMaxDP(0x%lx,%ld)\n",dataAddr,rt_tbl[npindx]);
          fidmax = fidAbsMaxDP(dataAddr,rt_tbl[npindx]);
       }
       else
       {
	  DPRINT2(0,"call fidAbsMaxSP(0x%lx,%ld)\n",dataAddr,rt_tbl[npindx]);
          fidmax = fidAbsMaxSP((short*)dataAddr,rt_tbl[npindx]);
       }
       DPRINT5(0,"FID Max: %ld, gain: %d, maxgain: %d, overload: %d, target level: %ld\n",
		  fidmax,rt_tbl[recvgindx],maxgain,overload,targetlevel);

       delta = calcgain(fidmax,targetlevel /*32768/2*/);   /* maybe 32768 * 3/4 = 24576 */
       /* adjust delta if exceeds maxgain result */
       if ((gain + (delta)) > maxgain)
       {
          delta = maxgain - gain;
       }

       if (abs(delta) <= 4)   /* if delta just 4 then just use this gain */
       {
          DPRINT1(1,"stmFreeFidBlk: tag %d\n",Tag);
          stmFreeFidBlk(pTheStmObject,Tag);
	  /* clr any overload & reenable ADC intrps */
          adcOvldClear(pTheAdcObject); 
	  /* clr any High Speed STM/ADC ADC overload */
          stmAdcOvldClear(pTheStmObject); 
          stmItrpEnable(pTheFifoObject, MAX_SUM_ITRP_MASK );
          break; 
       }

       if ( (gain == minval) && (delta<0) )
       {
          /* the non-zero rstatus will terminate while() */
          rstatus = AGAINERROR + AGAINFAIL;
       }

       gain = gain + delta;
       if (gain < minval)
          gain = minval;
       if (gain > maxgain) 
          gain = maxgain;

       /* round to step value of gain */
       rem = gain % stepval;
       DPRINT3(0,"Gain: %d, step: %d, modulo: %d\n",gain,stepval,rem);
       gain = (rem >= (stepval/2)) ? gain + (stepval - rem) : gain - rem;
       DPRINT1(0,"Corrected Gain: %d\n",gain);

       DPRINT3(0,"Delta: %d, gain: %d, maxgain: %d\n",
		delta,gain,maxgain);
     }
     DPRINT1(1,"stmFreeFidBlk: tag %d\n",Tag);
     stmFreeFidBlk(pTheStmObject,Tag);
     /* receivergain(0xb42, AP_HW_MIN_DELAY_CNT, (int) gain); */
     DPRINT1(0,"set receiver gain to %d\n",gain);
     receivergain(apaddr, getRcvrmode(), (int) gain);
     fifoStuffCmd(pTheFifoObject,CL_DELAY,CNT40_MSEC);
     fifoStuffCmd(pTheFifoObject,HALTOP,0);
     fifoStart( pTheFifoObject );
     fifoWait4Stop( pTheFifoObject );
     adcOvldClear(pTheAdcObject); /* clr any overload & reenable ADC intrps */
     stmAdcOvldClear(pTheStmObject); /* clr any High Speed STM/ADC ADC overload */
     stmItrpEnable(pTheFifoObject, MAX_SUM_ITRP_MASK );
     fifoSetStart4Exp(pTheFifoObject,fifoexpstartstate);
   }

  free(tmp_rtvar_base);

  /* Set the Original gain value to new autogain value */
  rt_base = (long *) pAcodeId->cur_rtvar_base;
  rt_tbl=rt_base+1;
  rt_tbl[recvgindx] = gain;

  stmChgMsgQ(pTheStmObject, pPrevMsgQ);

  update_acqstate(old_stat);
  return(rstatus);

}

long fidAbsMaxDP(long *dataAddr,long np)
{
  register long i,npcnt;
  register long *dptr;
  register long max = 0L;

  for (dptr=dataAddr,npcnt=np,i=0;i<npcnt;i++,dptr++)
  {
      if ( (( *dptr < 0 ) ? (-(*dptr)) : *dptr ) > max )
      {
         max = (( *dptr < 0 ) ? (-(*dptr)) : *dptr );
      }
      /* DPRINT3(3,"data[%d]: %ld, max : %ld\n",i,*dptr,max); */
  }
  return(max);
}

long fidAbsMaxSP(short *dataAddr,long np)
{
  register long i,npcnt;
  register short *dptr;
  register long max = 0L;

  for (dptr=dataAddr,npcnt=np,i=0;i<npcnt;i++,dptr++)
  {
      if ( (( *dptr < 0 ) ? (-(*dptr)) : *dptr ) > max )
      {
         max = (( *dptr < 0 ) ? (-(*dptr)) : *dptr );
      }
      /* DPRINT3(3,"data[%d]: %d, max : %ld\n",i,*dptr,max); */
  }
  return(max);
}

   /* if recvoverload then start at a gain of 0 and max gain to present
      value.
      if adcoverflow then start at a gain of 0 and max gain to present
      value. 
      if maxvalue about 50% of max then OK.
      Otherwise adjust for 50%  
   */ 

setFidParms(short ntindx,short npindx,short dpfindx, 
	    ushort_t ac_offset, ACODE_ID pAcodeId)
{
    long *rt_base, *rt_tbl; /* pointer to realtime buffer addresses */
    fidntindx = ntindx;
    fidnpindx = npindx;
    fidac_offset = ac_offset;
    fidpAcodeId = pAcodeId;
    DPRINT3(0,"setFidParms: Acode Obj: 0x%lx, ac_base = 0x%lx, rt_base = 0x%lx\n",
	fidpAcodeId, fidpAcodeId->cur_acode_base, fidpAcodeId->cur_rtvar_base);
    if (fidpAcodeId != NULL)
    {
      rt_base = (long *) fidpAcodeId->cur_rtvar_base;
      rt_tbl=rt_base+1;
      fiddpf = rt_tbl[dpfindx];
      fidnp = rt_tbl[npindx];
      DPRINT4(0,">>>  nt (rt_tbl[%d]) = %d,  np (rt_tbl[%d]) = %d\n",
	    ntindx,rt_tbl[ntindx], npindx, rt_tbl[npindx]);
      DPRINT2(0,">>>  dpf (rt_tbl[%d]) = %d\n",
	    dpfindx,rt_tbl[dpfindx]);
    }
    else	/* reasonable defaults */
    {
      fiddpf = 4;
      fidnp = 1024;
    }
}

int getFidDpf()
{
   return(fiddpf);
}
long getFidNp()
{
   return(fidnp);
}


/* call in stubio.c for shimming on the fid via Getfid() */
ulong_t getshimfid()
{
   ulong_t fidaddr;
   int getfiddata(ACODE_ID pAcodeId, short ntindx, short npindx, ushort_t ac_cur, ulong_t *addr );
   DPRINT(1,"getshimfid: call getfiddata \n");
   getfiddata(fidpAcodeId, fidntindx, fidnpindx, fidac_offset, &fidaddr );
   DPRINT1(1,"getshimfid: returned from getfiddata, fid addr: 0x%lx \n",
		fidaddr);

   return( fidaddr );
}

/*******************************************************************
* getfiddata
*    obtain FID via re-entrant call to APint() with apropriate pointers
*    into a set of Acodes
*    1. create private copies of Acode Object and RT parameter table
*    2. Copy orignals into the private ones
*    3. COnfigure for nt = 1 acquire
*    4. Call APint()
*    5. Get messages (typical a. NOISE acquire, b. ADC/Recv Overflow (if occurs)
*			      c. FID acquire )
*    6. return FID data
*/
int getfiddata(ACODE_ID pAcodeId, short ntindx, short npindx, ushort_t ac_offset, ulong_t *addr )
{
  int Tag,stat;
  long *dataAddr;
  ITR_MSG itrmsge;
  FID_STAT_BLOCK *pStatBlk;
  MSG_Q_ID pPrevMsgQ;
  long fidAbsMax(long *,long );
  long *rt_base, *rt_tbl; /* pointer to realtime buffer addresses */
  unsigned int	*tmp_rtvar_base;
  long fidmax;
  int rstatus = 0;
  unsigned short *ac_cur,*ac_base; /* acode pointers */

  ACODE_OBJ fidAcodeId;    /* private getfid acode object */

  DPRINT(0,"getfiddata: =================================== \n");
  DPRINT3(1,"Orig. Acode Obj: 0x%lx, ac_base = 0x%lx, rt_base = 0x%lx\n",
	pAcodeId, pAcodeId->cur_acode_base, pAcodeId->cur_rtvar_base);
  ac_base = pAcodeId->cur_acode_base;
  ac_cur = ac_base + ac_offset;
  rt_base = (long *) pAcodeId->cur_rtvar_base;
  rt_tbl=rt_base+1;
  DPRINT4(1,">>>  nt (rt_tbl[%d]) = %d,  np (rt_tbl[%d]) = %d\n",
	ntindx,rt_tbl[ntindx], npindx, rt_tbl[npindx]);
  DPRINT2(1,"FID Acquire Acode location: 0x%lx, Acode = %d\n",
            ac_cur+1,*(ac_cur+1));
  DPRINT1(1,"RT Table size: %d\n", pAcodeId->cur_rtvar_size);

  /* make private rt variable table */
  tmp_rtvar_base = (unsigned int *)malloc((pAcodeId->cur_rtvar_size) * sizeof(long));

  /* copy present rt variable table into private one */
  memcpy(tmp_rtvar_base,pAcodeId->cur_rtvar_base,(pAcodeId->cur_rtvar_size * sizeof(long)));

  /* copy present Acode Object into private one */
  memcpy(&fidAcodeId,pAcodeId,sizeof(ACODE_OBJ));
  DPRINT3(1,"Copy of Acode Obj: 0x%lx, ac_base = 0x%lx, rt_base = 0x%lx\n",
	&fidAcodeId, fidAcodeId.cur_acode_base, 
	fidAcodeId.cur_rtvar_base);
   
  /* setup Private Acode object for getfid */
  fidAcodeId.interactive_flag = ACQ_AUTOGAIN;
  fidAcodeId.cur_acode_base = ac_cur; /* just before INIT */
  fidAcodeId.cur_rtvar_base = tmp_rtvar_base; /* set to private table */
  fidAcodeId.cur_acode_set = fidAcodeId.num_acode_sets = 1;
  DPRINT3(1,"Private Acode Obj: 0x%lx, ac_base = 0x%lx, rt_base = 0x%lx\n",
	&fidAcodeId, fidAcodeId.cur_acode_base, 
	fidAcodeId.cur_rtvar_base);
  rt_base = (long *) tmp_rtvar_base;
  rt_tbl=rt_base+1;
  DPRINT4(1,">>>  nt (rt_tbl[%d]) = %d,  np (rt_tbl[%d]) = %d\n",
	ntindx,rt_tbl[ntindx], npindx, rt_tbl[npindx]);

  /* set STM & ADC Intrp MsgQ to Our  private msgQ */
   pPrevMsgQ = stmChgMsgQ(pTheStmObject, pUpLnkHookMsgQ);


     /* copy original rt variable table into private one */
     memcpy(tmp_rtvar_base,pAcodeId->cur_rtvar_base,(pAcodeId->cur_rtvar_size * sizeof(long)));
     rt_tbl[ntindx] = 1;  /* set nt = 1 */

     adcItrpDisable(pTheAdcObject,ADC_ALLITRPS);

     DPRINT(0,"-------------- Calling APint() --------------------- \n");
     APint(&fidAcodeId);  /* Ahh, the miracle of re-entrent code */
     fifoStart( pTheFifoObject );
     fifoWait4Stop( pTheFifoObject );

     while(1)
     {
       DPRINT1(1,"=========   Wait for Message  Q'd: %d  =============\n",
		  msgQNumMsgs(pUpLnkHookMsgQ));
       msgQReceive(pUpLnkHookMsgQ,(char*) &itrmsge,sizeof(ITR_MSG), 
			WAIT_FOREVER);
       Tag = stmGetNxtFid(pTheStmObject,&itrmsge,&pStatBlk,&dataAddr,&stat);
       DPRINT5(1,
	"itrmge: donecode %d(warn-%d), errorcode %d(adcov-%d), msgType %d\n",
		  itrmsge.donecode, WARNING_MSG, itrmsge.errorcode, 
		  WARNINGS + ADCOVER,itrmsge.msgType);

       DPRINT1(1,"Got %s FID\n",
		((pStatBlk->elemId == 0L) ? "Noise" : "FID"));
       DPRINT5(1,
       "getfiddata Tag: %ld, FID: %ld, CT: %ld, doneCode: %d, errorCOde: %d\n",
	    Tag,pStatBlk->elemId,pStatBlk->ct,pStatBlk->doneCode,
		pStatBlk->errorCode);
       DPRINT4(1,
       "         FID: %ld, CT: %ld, NP: %ld, fid Size: %ld\n",
                pStatBlk->elemId,pStatBlk->ct, pStatBlk->np,
                pStatBlk->dataSize);
       DPRINT1(1,
       "    dataAddr: 0x%lx\n",dataAddr);
       if ( (Tag != -1) && (pStatBlk->elemId != 0L))
       {
         *addr = (ulong_t) dataAddr;	/* return address of data */
         break;
       }
       if ((Tag != -1))
       {
         DPRINT1(1,"stmFreeFidBlk: tag %d\n",Tag);
         stmFreeFidBlk(pTheStmObject,Tag);
       }
       if ((Tag == -1) && (itrmsge.donecode == WARNING_MSG) && 
	    ( (itrmsge.errorcode == WARNINGS + ADCOVER) ||
	      (itrmsge.errorcode == WARNINGS + RECVOVER) )
	  )
       {
	  rstatus = AGAINERROR + AGAINFAIL;  /* the non-zero rstatus will terminate while() */
          DPRINT1(1,"ADC/Recv OverFlow: tag %d\n",Tag);

       }
     }

  DPRINT1(1,"Msg in Q: %d\n",msgQNumMsgs(pUpLnkHookMsgQ));
  DPRINT(1,">>>>>>>>>   Return data address <<<<<<<<<<<<<\n");
  DPRINT1(1,"stmFreeFidBlk: tag %d\n",Tag);
  stmFreeFidBlk(pTheStmObject,Tag);
  adcOvldClear(pTheAdcObject); /* clr any overload & reenable ADC intrps */
  stmAdcOvldClear(pTheStmObject); /* clr any High Speed STM/ADC ADC overload */

  free(tmp_rtvar_base);

  stmChgMsgQ(pTheStmObject, pPrevMsgQ);

  return(rstatus);

}

UpLnkHookMsgQClear()
{
  ITR_MSG itrmsge;
  while(msgQNumMsgs(pUpLnkHookMsgQ))
  {
       msgQReceive(pUpLnkHookMsgQ,(char*) &itrmsge,sizeof(ITR_MSG), 
			NO_WAIT);
       DPRINT5(0,
        "UpLnkHookMsgQClear: removed msge: Tag: %d, Donecode: %d, Errorcode: %d, msgType: %d, Count: %lu\n",
          itrmsge.tag, itrmsge.donecode, itrmsge.errorcode,
	  itrmsge.msgType, itrmsge.count);
  }
}
