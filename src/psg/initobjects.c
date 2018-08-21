/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef PSG_LC
#define PSG_LC 
#endif
#include <stdio.h>
#include "group.h"
#include "variables.h"
#include "params.h"
#include "acodes.h"
#include "rfconst.h"
#include "oopc.h"	/* Object Oriented Programing Package */
#include "acqparms.h"
#include "chanstruct.h"
extern int ap_ovrride;
extern int    ap_interface;	/* AP Interface Type */
extern int RFChan_Device();
extern char amptype[];
extern struct _chanconst rfchanconst[];
extern int bgflag;
initObjects()
{
    extern Object ObjectNew();
    extern int    Attn_Device(),AP_Device(),APBit_Device(),Event_Device();
    extern int    Freq_Device();
    extern double whatamphibandmin();
    extern int 	  rotorSync;
    extern int RFCHCONSTsize;
    extern void inittimerconst();
    int rfchan;
    char msge[128];

    /* ----- Object Programing, What'a Concept! ------------- */
    /* Gradual introduce of Object Oriented Program into PSG */
    /*     Another Great Advancement? by Greg Brissey 8/18/88 */

    /* Create Hardware Objects to be controlled	*/

    Ext_Trig = ObjectNew(Event_Device,"External Trigger Control");
    SetEventAttr(Ext_Trig,SET_DEFAULTS,SET_MAXRTPARS,14,
			    SET_VALID_RTPAR,v1, SET_VALID_RTPAR,v2,
                            SET_VALID_RTPAR,v3, SET_VALID_RTPAR,v4,
                            SET_VALID_RTPAR,v5, SET_VALID_RTPAR,v6,
                            SET_VALID_RTPAR,v7, SET_VALID_RTPAR,v8,
                            SET_VALID_RTPAR,v9, SET_VALID_RTPAR,v10,
                            SET_VALID_RTPAR,v11, SET_VALID_RTPAR,v12,
                            SET_VALID_RTPAR,v13, SET_VALID_RTPAR,v14,
                 NULL);

    RT_Delay = ObjectNew(Event_Device,"Real-Time Delay Control");
    SetEventAttr(RT_Delay,SET_DEFAULTS,SET_MAXRTPARS,19,
			    SET_TYPE,		EVENT_RT_DELAY,
			    SET_VALID_RTPAR,ct, 
			    SET_VALID_RTPAR,bsctr, SET_VALID_RTPAR,ssctr,
			    SET_VALID_RTPAR,fidctr, SET_VALID_RTPAR,tablert,
			    SET_VALID_RTPAR,v1, SET_VALID_RTPAR,v2,
                            SET_VALID_RTPAR,v3, SET_VALID_RTPAR,v4,
                            SET_VALID_RTPAR,v5, SET_VALID_RTPAR,v6,
                            SET_VALID_RTPAR,v7, SET_VALID_RTPAR,v8,
                            SET_VALID_RTPAR,v9, SET_VALID_RTPAR,v10,
                            SET_VALID_RTPAR,v11, SET_VALID_RTPAR,v12,
                            SET_VALID_RTPAR,v13, SET_VALID_RTPAR,v14,
                 NULL);

    inittimerconst(); /* initialize timerword constants, tests fifolpsize */

    /* create relay and signal routine objects */
    if ( (ap_interface > 1) && (ap_interface < 4) )
    {
      RF_Rout = ObjectNew(APBit_Device,"RF Routing Relays"); /*ap bit cntl obj*/
      RF_Opts = ObjectNew(APBit_Device,"RF Options");
      RF_Opts2 = ObjectNew(APBit_Device,"RF Options 2");
      /* set the appropriate logical-to-bit translation */
      SetAPBit(RF_Rout,SET_DEFAULTS,
		     SET_TRUE_EQ_ZERO,		DEC_RELAY5, 	   /* BIT 0 */
		     SET_TRUE_EQ_ZERO,		AMP_RELAY, 	   /* BIT 1 */
		     SET_TRUE_EQ_ZERO,		DEC2_RELAY, 	   /* BIT 2 */
		     SET_TRUE_EQ_ZERO,		OBS_RELAY, 	   /* BIT 3 */
		     SET_TRUE_EQ_ZERO,		DEC_RELAY, 	   /* BIT 4 */
		     SET_TRUE_EQ_ZERO,		HPAMPHB_RELAY, 	   /* BIT 5 */
		     SET_TRUE_EQ_ZERO,		HPAMPLB_RELAY, 	   /* BIT 6 */
		     SET_TRUE_EQ_ZERO,		TUNE_RELAY, 	   /* BIT 7 */
		NULL);	/* reg 14 */

      SetAPBit(RF_Opts,SET_DEFAULTS,
		     SET_TRUE_EQ_ZERO,		DEC_XMTR_HI_LOW_BAND, /* BIT0 */
		     SET_TRUE_EQ_ONE,		HOMO2, 	   	      /* BIT1 */
		     SET_TRUE_EQ_ZERO,		OBS_XMTR_HI_LOW_BAND, /* BIT2 */
		     SET_TRUE_EQ_ZERO,		LO_RELAY, 	      /* BIT3 */
		     SET_TRUE_EQ_ZERO,		LOWBAND2_CW,          /* BIT4 */
		     SET_TRUE_EQ_ZERO,		DEC2_XMTR_HI_LOW_BAND,/* BIT5 */
		     SET_TRUE_EQ_ZERO,		MODB, 	   	      /* BIT6 */
		     SET_TRUE_EQ_ZERO,		MODA, 	   	      /* BIT7 */
		NULL);
      SetAPAttr(RF_Opts,SET_APREG,RF_OPTS_AP_REG /* 17 */,NULL);

      SetAPBit(RF_Opts2,SET_DEFAULTS,
		     SET_TRUE_EQ_ZERO,		HIGHBAND_CW, 	   /* BIT 0 */
		     SET_TRUE_EQ_ZERO,		LOWBAND_CW, 	   /* BIT 1 */
		     SET_TRUE_EQ_ZERO,		PREAMP_GAIN_SELECT,/* BIT 2 */
		     SET_TRUE_EQ_ZERO,		TR_SWITCH, 	   /* BIT 3 */
		     SET_TRUE_EQ_ZERO,		MIXER_SELECT, 	   /* BIT 4 */
		     SET_TRUE_EQ_ZERO,		PREAMP_SELECT, 	   /* BIT 5 */
		     SET_TRUE_EQ_ZERO,		HIGHBAND2_CW, 	   /* BIT 6 */
		     SET_TRUE_EQ_ZERO,		LEG_RELAY, 	   /* BIT 7 */
		NULL);
      SetAPAttr(RF_Opts2,SET_APREG,RF_OPTS2_AP_REG /* 13 */,NULL);

      /* initialize hardware objects */
      SetAPBit(RF_Rout,
		     SET_FALSE,		DEC_RELAY5, 
		     SET_FALSE,		AMP_RELAY, 
		     SET_FALSE,		DEC2_RELAY, 
		     SET_FALSE,		OBS_RELAY, 
		     SET_FALSE,		DEC_RELAY, 
		     SET_FALSE,		HPAMPHB_RELAY, 
		     SET_FALSE,		HPAMPLB_RELAY, 
		     SET_FALSE,		TUNE_RELAY, 
		NULL);
      SetAPBit(RF_Opts,
		     SET_FALSE,		DEC_XMTR_HI_LOW_BAND, 
		     SET_FALSE,		HOMO2, 
		     SET_FALSE,		OBS_XMTR_HI_LOW_BAND, 
		     SET_FALSE,		LO_RELAY, 
		     SET_FALSE,		LOWBAND2_CW, 
		     SET_FALSE,		DEC2_XMTR_HI_LOW_BAND, 
		     SET_FALSE,		MODB, 
		     SET_FALSE,		MODA, 
		NULL);
      SetAPBit(RF_Opts2,
		     SET_FALSE,		HIGHBAND_CW, 
		     SET_FALSE,		LOWBAND_CW, 
		     SET_FALSE,		PREAMP_GAIN_SELECT, 
		     SET_FALSE,		TR_SWITCH, 
		     SET_FALSE,		MIXER_SELECT, 
		     SET_FALSE,		PREAMP_SELECT, 
		     SET_FALSE,		HIGHBAND2_CW, 
		     SET_FALSE,		LEG_RELAY, 
		NULL);
    }
    else
    {
       RF_Rout  = (Object) NULL;	/* So we know the object isn't there */
       RF_Opts  = (Object) NULL;	/* So we know the object isn't there */
       RF_Opts2 = (Object) NULL;	/* So we know the object isn't there */
    } /* endif ( (ap_interface > 1) && (ap_interface < 4) ) */

    if (ap_interface == 4) 
    {
       RF_TR_PA  = ObjectNew(AP_Device,   "RF T/R and PreAmp enables");
       RF_Amp1_2 = ObjectNew(AP_Device,   "Amp 1/2 cw vs. pulse");
       RF_Amp3_4 = ObjectNew(AP_Device,   "Amp 3/4 cw vs. pulse");
       RF_Amp_Sol= ObjectNew(AP_Device,   "Amp Solids cw vs. pulse");
       RF_MLrout = ObjectNew(APBit_Device,"RF Routing Relays in Magnet Leg");
       RF_PICrout = ObjectNew(APBit_Device,"RF Routing Relays in 4 channel type  (PIC) Magnet Leg");
       RF_hilo   = ObjectNew(APBit_Device,"ATTNs hi/lo band relays");
       RCVR_homo = ObjectNew(AP_Device,   "RCVR homo decoupling bit");
       /* set the appropriate logical-to-bit translation */

      SetAPAttr(RF_TR_PA,
		     SET_APADR,			0xb,
		     SET_APREG,			0x48,
		     SET_APBYTES,		1,
		     SET_APMODE,		1,
		NULL);
      SetAPAttr(RF_Amp1_2,
		     SET_APADR,			AMP1_2_APADDR,
		     SET_APREG,			AMP1_2_APREG,
		     SET_APBYTES,		1,
		     SET_APMODE,		1,
		NULL);

      SetAPAttr(RF_Amp3_4,
		     SET_APADR,			AMP3_4_APADDR,
		     SET_APREG,			AMP3_4_APREG,
		     SET_APBYTES,		1,
		     SET_APMODE,		1,
		NULL);

      SetAPAttr(RF_Amp_Sol,
		     SET_APADR,			AMPSOL_APADDR,
		     SET_APREG,			AMPSOL_APREG,
		     SET_APBYTES,		1,
		     SET_APMODE,		1,
		NULL);

      SetAPBit(RF_MLrout,SET_DEFAULTS,
		     SET_TRUE_EQ_ONE,		MAGLEG_HILO,	   /* BIT 0 */
		     SET_TRUE_EQ_ONE,		MAGLEG_MIXER,	   /* BIT 1 */
		     SET_TRUE_EQ_ONE,		MAGLEG_RELAY_0,	   /* BIT 2 */
		     SET_TRUE_EQ_ONE,		MAGLEG_RELAY_1,	   /* BIT 3 */
		     SET_TRUE_EQ_ONE,		MAGLEG_RELAY_2,	   /* BIT 4 */
		     SET_TRUE_EQ_ONE,		MAGLEG_ARRAY_MODE, /* BIT 5 */
		     SET_TRUE_EQ_ONE,		MAGLEG_PREAMP_SEL_0, /* BIT 6 */
		     SET_TRUE_EQ_ONE,		MAGLEG_PREAMP_SEL_1, /* BIT 7 */
		NULL);
      SetAPAttr(RF_MLrout,
		     SET_APADR,			MAGLEG_APADDR,
		     SET_APREG,			MAGLEG_APREG,
		     SET_APBYTES,		1,
		     SET_APMODE,		1,
		NULL);

      /* PIC add. 9/5/02 */
      SetAPBit(RF_PICrout,SET_DEFAULTS,
		     SET_TRUE_EQ_ONE,		PIC_MIX1SEL,	   /* BIT 0 */
		     SET_TRUE_EQ_ONE,		PIC_MIX2SEL,	   /* BIT 1 */
		     SET_TRUE_EQ_ONE,		PIC_PSEL0,	   /* BIT 2 */
		     SET_TRUE_EQ_ONE,		PIC_PSEL1,	   /* BIT 3 */
		     SET_TRUE_EQ_ONE,		PIC_IN2SEL0,	   /* BIT 4 */
		     SET_TRUE_EQ_ONE,		PIC_IN2SEL1, 	   /* BIT 5 */
		     SET_TRUE_EQ_ONE,		PIC_ARRAY, 	   /* BIT 6 */
		     SET_TRUE_EQ_ONE,		PIC_DELAY, 	   /* BIT 7 */
		NULL);
      SetAPAttr(RF_PICrout,
		     SET_APADR,			MAGLEG_APADDR,  /* 0xB00 */
		     SET_APREG,			PIC_AP_APREG,	/* 0x4E */
		     SET_APBYTES,		1,
		     SET_APMODE,		1,
		NULL);

      SetAPBit(RF_hilo,SET_DEFAULTS,
		     SET_TRUE_EQ_ONE,		HILO_CH1,	   /* BIT 0 */
		     SET_TRUE_EQ_ZERO,		HILO_CH2,	   /* BIT 1 */
		     SET_TRUE_EQ_ONE,		HILO_CH3,	   /* BIT 2 */
		     SET_TRUE_EQ_ZERO,		HILO_CH4, 	   /* BIT 3 */
		     SET_TRUE_EQ_ONE,		HILO_AMT2_HL, 	   /* BIT 4 */
		     SET_TRUE_EQ_ONE,		HILO_AMT3_HL, 	   /* BIT 5 */
		     SET_TRUE_EQ_ONE,		HILO_AMT2_LL, 	   /* BIT 6 */
		     SET_TRUE_EQ_ONE,		HILO_NC, 	   /* BIT 7 */
		NULL);
      SetAPAttr(RF_hilo,
		     SET_APADR,			HILO_APADDR,
		     SET_APREG,			HILO_APREG,
		NULL);
      SetAPBit(RF_MLrout,
		     SET_FALSE,		MAGLEG_HILO,	   /* BIT 0 */
		     SET_FALSE,		MAGLEG_MIXER,	   /* BIT 1 */
		     SET_FALSE,		MAGLEG_RELAY_0,	   /* BIT 2 */
		     SET_FALSE,		MAGLEG_RELAY_1,	   /* BIT 3 */
		     SET_FALSE,		MAGLEG_RELAY_2,	   /* BIT 4 */
		     SET_FALSE,		MAGLEG_ARRAY_MODE, /* BIT 5 */
		     SET_FALSE,		MAGLEG_PREAMP_SEL_0, /* BIT 6 */
		     SET_FALSE,		MAGLEG_PREAMP_SEL_1, /* BIT 7 */
		NULL);
      SetAPBit(RF_PICrout,
		     SET_TRUE,		PIC_MIX1SEL,	   /* BIT 0 */
		     SET_TRUE,		PIC_MIX2SEL,	   /* BIT 1 */
		     SET_FALSE,		PIC_PSEL0,	   /* BIT 2 */
		     SET_FALSE,		PIC_PSEL1,	   /* BIT 3 */
		     SET_FALSE,		PIC_IN2SEL0,	   /* BIT 4 */
		     SET_FALSE,		PIC_IN2SEL1, 	   /* BIT 5 */
		     SET_FALSE,		PIC_ARRAY, 	   /* BIT 6 */
		     SET_FALSE,		PIC_DELAY, 	   /* BIT 7 */
		NULL);
      SetAPBit(RF_hilo,
		     SET_TRUE,		HILO_CH1,	   /* BIT 0 */
		     SET_TRUE,		HILO_CH2,	   /* BIT 1 */
		     SET_TRUE,		HILO_CH3,	   /* BIT 2 */
		     SET_TRUE,		HILO_CH4, 	   /* BIT 3 */
		     SET_FALSE,		HILO_AMT2_HL, 	   /* BIT 4 */
		     SET_FALSE,		HILO_AMT3_HL, 	   /* BIT 5 */
		     SET_FALSE,		HILO_AMT2_LL, 	   /* BIT 6 */
		     SET_FALSE,		HILO_NC, 	   /* BIT 7 */
		NULL);
      SetAPAttr(RCVR_homo,
		     SET_APADR,			0xb,
		     SET_APREG,			0x43,
		     SET_APBYTES,		1,
		     SET_APMODE,		1,
		NULL);

       /* Initialize User AP Objects for inova BOB board */
       inituserapobjects();
    }
    else
    {
       RF_TR_PA  = (Object) NULL;	/* Just so we know it isn't there */
       RF_Amp1_2 = (Object) NULL;	/* Just so we know it isn't there */
       RF_Amp3_4 = (Object) NULL;	/* Just so we know it isn't there */
       RF_Amp_Sol= (Object) NULL;	/* Just so we know it isn't there */
       RF_MLrout = (Object) NULL;	/* Just so we know it isn't there */
       RF_PICrout = (Object) NULL;	/* Just so we know it isn't there */
       RF_hilo   = (Object) NULL;	/* Just so we know it isn't there */
       RCVR_homo = (Object) NULL;	/* Just so we know it isn't there */
    } /* endif (ap_interface == 4) */

    if (NUMch > MAX_RFCHAN_NUM)
    {
       sprintf(msge,
	"Number of Channels configured (%d) beyond the max (%d)\n",NUMch,
	   MAX_RFCHAN_NUM);
       text_error(msge);
       psg_abort(1);
    }
    if (NUMch > RFCHCONSTsize)
    {
       sprintf(msge,
	"Number of Channels configured (%d) beyond the channel constants provided (%d)\n",NUMch,RFCHCONSTsize);
       text_error(msge);
       psg_abort(1);
    }

    for (rfchan=1; rfchan <= MAX_RFCHAN_NUM; rfchan++)
      RF_Channel[rfchan] = NULL;
/*
    RF_Channel[0] = RF_Channel[TODEV] = NULL;
    RF_Channel[DODEV] = RF_Channel[DO2DEV] = RF_Channel[DO3DEV] = NULL;
*/
    OBSch = TODEV; DECch = DODEV; DEC2ch = DO2DEV; DEC3ch = DO3DEV; DEC4ch = DO4DEV;

    /* For Each RF Channel create its Hardware Objects */
    for (rfchan=1; rfchan <= NUMch; rfchan++)
    {
       Object RFChan,FreqObj,CattnObj,FattnObj;
       char rfchlabel[MAXSTR];

       /* For Now only permit the 5th Channel to be rftype 'l' or lock/2H dec  */
       if ((rfchan > 4) &&  (whatrftype(rfchan) != LOCK_DECOUP_OFFSETSYN))
       {
         sprintf(msge,
	  "5th channel can only be 2H Decoupler, rftype: 'l' \n");
         text_error(msge);
         psg_abort(1);
       }

       if (P_getstring(GLOBAL, "rfchlabel", rfchlabel, rfchan, MAXSTR) < 0)
       {
         sprintf(rfchlabel,"RF Channel %d",rfchan);
       }
       RFChan = ObjectNew(RFChan_Device,rfchlabel);
       RF_Channel[rfchan] = RFChan;
       sprintf(msge,"Channel %d Transmitter",rfchan);
       FreqObj = ObjectNew(Freq_Device,msge);
       setup_freq_obj( FreqObj, rfchan );

      if ( cattn[rfchan] != 0.0 )
      {
	if (cattn[rfchan] == SIS_UNITY_ATTN_MAX)
	{
           sprintf(msge,"Channel %d  SISCO Style Attenuator",rfchan);
           CattnObj = ObjectNew(Attn_Device,msge);
	}
        else if ( ap_interface == 1 )
	{
           sprintf(msge,"Channel %d  Early Style Attenuator",rfchan);
           CattnObj = ObjectNew(Attn_Device,msge);
	}
	else
	{
           sprintf(msge,"Channel %d  Attenuator",rfchan);
           CattnObj = ObjectNew(Attn_Device,msge);
	}

        /*----------------------------------------------------------------------
		 	     adr reg 	bytes 	mode 	max 	min 	offset
 	chan 1 cattn,ap<2    5  0  	1 	-1 	63|79 	0 	max-63 
 	chan 2 cattn,ap<2    5  4  	1 	-1 	63|79	0 	max-63 
 	chan 1 cattn,ap=>2   5  12  	1 	-1 	63|79 	0 	max-63 
 	chan 2 cattn,ap=>2   5  16  	1 	-1 	63|79	0 	max-63 
 	chan 3 cattn,ap=>2   5  15  	1 	-1 	63|79	0 	max-63 

        Note: 9/11/91 maxvalue is based on cattn, however offset is base on 
                      the DEFINED max power value of 63 and the max attn value 
		      (63 or 79) i.e. offset = maxval - 63
	+---------------------------------------------------------------------*/
        SetAttnAttr(CattnObj,SET_DEVSTATE,	ACTIVE,
			 SET_DEVCNTRL,		VALUEONLY,
			 SET_DEVCHANNEL,	rfchan,
			 SET_APADR,		rfchanconst[rfchan].catapadr,
			 SET_APREG,		rfchanconst[rfchan].catapreg,
			 SET_APBYTES,		rfchanconst[rfchan].catapbytes,
			 SET_APMODE,		rfchanconst[rfchan].catapmode,
			 SET_MAXVAL,		(int) (cattn[rfchan]),
					/* rfchanconst[rfchan].cathwmax, */
			 SET_MINVAL,		rfchanconst[rfchan].cathwmin,
			 SET_OFFSET,		(((int) cattn[rfchan]) - 63),
					/* rfchanconst[rfchan].cathwofset, */
		NULL); 

        if ( ap_interface == 1 )
        {
          SetAttnAttr(CattnObj,
                         SET_APREG,             rfchanconst[rfchan].catapreg5,
                         SET_OFFSET,            rfchanconst[rfchan].cathwofset5,
			 SET_APMODE,		1,
		NULL);
        }
        if (ap_interface == 4 )		/* U+ or INOVA */
        {
          char type;

	/*
	   fprintf(stderr,">> >>> chan: %d, Cattn:  %d, apinterface=4, apreg: 0x%x\n",
		rfchan,cattn[rfchan],0x33 - (rfchan-1)*16);
	*/

          SetAttnAttr(CattnObj,
			SET_APADR,		0xb,
			SET_APREG,		0x33-(rfchan-1),
			SET_APMODE,		2,
		NULL);

          if (whatrftype(rfchan) == LOCK_DECOUP_OFFSETSYN)
          {
           SetAttnAttr(CattnObj,
			 SET_DEVCNTRL,		VALUEONLY,
			 SET_DEVCHANNEL,	rfchan,
			 SET_APADR,		0xb,
			 SET_APREG,		0x96 + (rfchan-1)*16,
			 SET_APBYTES,		1,
			 SET_APMODE,		3,	/* for attn_device for lk/dec attn control */
			 SET_MAXVAL,		63,	/* max,min,offset all overriden by apmode 3 */
			 SET_MINVAL,		0,
			 SET_OFFSET,		(((int) cattn[rfchan]) - 63),
		NULL); 
           if (bgflag)
	     fprintf(stderr,">>>>> chan: %d, Cattn:  %d, lock/dec apreg: 0x%x\n",
		rfchan,cattn[rfchan],0x96 + (rfchan-1)*16);

          }
	}

	if (cattn[rfchan] == SIS_UNITY_ATTN_MAX)
	{
	   fprintf(stderr," >>> Cattn: ack! SIS_UNITY_ATTN_MAX: %d\n",SIS_UNITY_ATTN_MAX);
	   /* Set SIS Unity Attenuators */
	   if (rfchan == TODEV)
	   {
	      if (newtrans == TRUE) {
      		SetAttnAttr(CattnObj,
			 SET_DEVSTATE,		ACTIVE,
			 SET_DEVCNTRL,		VALUEONLY,
			 SET_DEVCHANNEL,	rfchan,
			 SET_APADR,		12,   	    /* 12 */
			 SET_APREG,		-1,	    /*  0 */
			 SET_APBYTES,		1,	    /*  1 */
			 SET_APMODE,		0,          /* 0 */
			 SET_MAXVAL,		255,        /* incl rf bit */
			 SET_MINVAL,		0,          /*  0 */
			 SET_OFFSET,		0,          /*  0 */
		         NULL); 
	      }
	      else {
      		SetAttnAttr(CattnObj,
			 SET_DEVSTATE,		ACTIVE,
			 SET_DEVCNTRL,		VALUEONLY,
			 SET_DEVCHANNEL,	rfchan,
			 SET_APADR,		12,	/* 12 */
			 SET_APREG,		3,	/*  3 */
			 SET_APBYTES,		1,	/*  1 */
			 SET_APMODE,		0,	/*  0 */
			 SET_MAXVAL,		255,   	/* incl rf bit unused */
			 SET_MINVAL,		0,   /*  0 */
			 SET_OFFSET,		63,	/* 63 */
		         NULL); 
	      }
	      ToAttn = CattnObj;
	   }
	   if (rfchan == DODEV)
	   {
	      if (newtrans == TRUE) {
      		SetAttnAttr(CattnObj,
			 SET_DEVSTATE,		ACTIVE,
			 SET_DEVCNTRL,		VALUEONLY,
			 SET_DEVCHANNEL,	rfchan,
			 SET_APADR,		13,   	    /* 12 */
			 SET_APREG,		-1,	    /*  0 */
			 SET_APBYTES,		1,	    /*  1 */
			 SET_APMODE,		0,          /* 0 */
			 SET_MAXVAL,		255,        /* incl rf bit */
			 SET_MINVAL,		0,          /*  0 */
			 SET_OFFSET,		0,          /*  0 */
		         NULL); 
	      }
	      else {
		SetAttnAttr(CattnObj,
			 SET_DEVSTATE,		ACTIVE,
			 SET_DEVCNTRL,		VALUEONLY,
			 SET_DEVCHANNEL,	rfchan,
			 SET_APADR,		12,	   /* 12 */
			 SET_APREG,		11,	   /* 11 */
			 SET_APBYTES,		1,	   /*  1 */
			 SET_APMODE,		0,	   /*  0 */
			 SET_MAXVAL,		255,   /* incl rf bit unused */
			 SET_MINVAL,		0,	   /*  0 */
			 SET_OFFSET,		63,	   /* 63 */
		         NULL); 
	      }
	      DoAttn = CattnObj;
	    }
	}
      }
      else
      {
        CattnObj = NULL;
      }

      /* If channel a LOCK_DECOUP_OFFSETSYN then no fine atten no matter what, (will overwrite cattn apaddr)*/
      if ( (fattn[rfchan] != 0.0) && (whatrftype(rfchan) != LOCK_DECOUP_OFFSETSYN) )
      {
         sprintf(msge,"Channel %d  Fine Attenuator",rfchan);
         FattnObj = ObjectNew(Attn_Device,msge);
	/*----------------------------------------------------------------------
			    adr reg 	bytes 	mode 	max 	min 	offset
 	chan 1 fattn,ap=>2   5  22  	2 	1 	4095 	0 	0 
 	chan 2 fattn,ap=>2   5  20  	2 	1 	4095 	0 	0 
	+---------------------------------------------------------------------*/
         SetAttnAttr(FattnObj,
			SET_DEVSTATE,		ACTIVE,
			SET_DEVCNTRL,		VALUEONLY,
			SET_DEVCHANNEL,		rfchan,
			SET_APADR,		rfchanconst[rfchan].fatapadr,
			SET_APREG,		rfchanconst[rfchan].fatapreg,
			SET_APBYTES,		rfchanconst[rfchan].fatapbytes,
			SET_APMODE,		rfchanconst[rfchan].fatapmode,
			SET_MAXVAL,		(int) fattn[rfchan],
					/* rfchanconst[rfchan].fathwmax, */
			SET_MINVAL,		rfchanconst[rfchan].fathwmin,
			SET_OFFSET,		rfchanconst[rfchan].fathwofset,
		NULL);
        if (ap_interface == 4 )
        {
          SetAttnAttr(FattnObj,
			SET_APADR,		0xb,
			SET_APREG,		0x96 + (rfchan-1)*16,
			SET_APMODE,		-1,
		NULL);
        }

      }
      else
      {
        FattnObj = NULL;
      }
      if (ap_interface == 4)
      {  RF_Mod    = ObjectNew(AP_Device,    "XMTRs modulation mode byte");
         RF_mixer  = ObjectNew(AP_Device,    "XMTRs hi/lo mixer relay");
         X_gmode   = ObjectNew(AP_Device,    "XMTRs gating mode byte");
         HS_select = ObjectNew(APBit_Device, "XMTRs HS line select:");
         SetAPAttr(RF_Mod,
			SET_APADR,		0xb,
			SET_APREG,		0x9b + (rfchan-1)*16,
			SET_APBYTES,		1,
			SET_APMODE,		1,
		NULL);


         SetAPAttr(RF_mixer,
			SET_APADR,		0xb,
			SET_APREG,		0x91 + (rfchan-1)*16,
			SET_APBYTES,		1,
			SET_APMODE,		1,
		NULL);

         SetAPAttr(X_gmode,
			SET_APADR,		0xb,
			SET_APREG,		0x92 + (rfchan-1)*16,
			SET_APBYTES,		1,
			SET_APMODE,		1,
		NULL);

         SetAPBit(HS_select,SET_DEFAULTS,
			SET_TRUE_EQ_ONE,	HS_SEL0,	   /* BIT 0 */
			SET_TRUE_EQ_ONE,	HS_SEL1,	   /* BIT 1 */
			SET_TRUE_EQ_ONE,	HS_SEL2,	   /* BIT 2 */
			SET_TRUE_EQ_ONE,	HS_SEL3,	   /* BIT 3 */
			SET_TRUE_EQ_ONE,	HS_SEL4,	   /* BIT 4 */
			SET_TRUE_EQ_ONE,	HS_SEL5,	   /* BIT 5 */
			SET_TRUE_EQ_ONE,	HS_SEL6,	   /* BIT 6 */
			SET_TRUE_EQ_ONE,	HS_SEL7,	   /* BIT 7 */
		NULL);
         SetAPAttr(HS_select,
			SET_APADR,		0xb,
			SET_APREG,		0x90 + (rfchan-1)*16,
		NULL);
         SetAPAttr(HS_select,
			SET_MASK,		rfchan,
		NULL);
         if (whatrftype(rfchan) == LOCK_DECOUP_OFFSETSYN)
         {
             /* bit 0 - LO gate 			0=on,1=off */
             /* bit 1 - Decoupler Transmitter Gate 	1=on,0=off */
             /* bit 7 - Decoupler Enable 		1=on,0=off */
             SetAPAttr(HS_select,
			SET_MASK,		1,  /* LO gate off, Txgate-0ff, DecEnable - Off */
		NULL);
         }
      }
      else
      {  RF_Mod    = (Object) NULL;	/* Just so we know it isn't there */
         RF_mixer  = (Object) NULL;	/* Just so we know it isn't there */
         X_gmode   = (Object) NULL;	/* Just so we know it isn't there */
         HS_select = (Object) NULL;	/* Just so we know it isn't there */
      }

      SetRFChanAttr(RFChan,SET_DEVSTATE, 	ACTIVE, 
			SET_DEVCHANNEL, 	rfchan,
	/* 2/21/97 huum? shouldn't these be rfchan not OBSERVE ?, no change for now. Greg B. */
			SET_XMTRTYPE,		whatxmtrtype(OBSERVE), 
			SET_AMPTYPE,		whatamptype(OBSERVE),
			SET_AMPHIMIN,		whatamphibandmin(OBSERVE,(double)H1freq),
			SET_FREQOBJ,		&FreqObj, 
			SET_ATTNOBJ,		&CattnObj,
			SET_LNATTNOBJ,		&FattnObj,
			SET_MOD_OBJ,		&RF_Mod,
			SET_MIXER_OBJ,		&RF_mixer,
			SET_HS_OBJ,		&HS_select,
			SET_XGMODE_OBJ,		&X_gmode,
			SET_HLBRELAYOBJ,	&RF_Rout,
			SET_XMTR2AMPBDBIT,	rfchanconst[rfchan].xmtr2amprelaybit,
			SET_XMTRX2OBJ,		&RF_Opts,
			SET_XMTRX2BIT,		rfchanconst[rfchan].xmtrx2bit,
			SET_PHLINE0,		rfchanconst[rfchan].phline0, 
			SET_PHLINE90,		rfchanconst[rfchan].phline90,
			SET_PHLINE180,		rfchanconst[rfchan].phline180,
			SET_PHLINE270,		rfchanconst[rfchan].phline270,
			SET_PHASEBITS,		rfchanconst[rfchan].phasebits,
			SET_APOVRADR, 		&ap_ovrride,
			SET_PHASEAPADR, 	rfchanconst[rfchan].smphaseapadr,
			SET_PHASE_REG,		0,
			SET_PHASE_MODE,		1,
			SET_HSLADDR, 		&HSlines,
			SET_XMTRGATEBIT, 	rfchanconst[rfchan].xmtrgatebit,
			SET_RCVRGATEBIT,	RXOFF,
			SET_WGGATEBIT, 		rfchanconst[rfchan].wggatebit,
		NULL);

      if (ap_interface == 4)
      {  SetRFChanAttr(RFChan,
			SET_HLBRELAYOBJ,	&RF_hilo,
			SET_XMTR2AMPBDBIT,	rfchan-1,
			SET_PHASEAPADR, 	0xb,
			SET_PHASE_REG,		0x94 + (rfchan-1)*16,
			SET_PHASE_MODE,		2,
			SET_XMTRX2BIT,		-1,
			SET_RCVRGATEBIT,	(1 << ((rfchan-1)*5 + 0)),
			SET_XMTRGATEBIT, 	(1 << ((rfchan-1)*5 + 1)),
			SET_WGGATEBIT, 		(1 << ((rfchan-1)*5 + 2)),
			SET_PHLINE90,		(1 << ((rfchan-1)*5 + 3)), 
			SET_PHLINE180,		(1 << ((rfchan-1)*5 + 4)),
			SET_PHLINE270,		(3 << ((rfchan-1)*5 + 3)),
			SET_PHASEBITS,		(3 << ((rfchan-1)*5 + 3)),
		NULL);
         if (whatrftype(rfchan) == LOCK_DECOUP_OFFSETSYN)
         {  SetRFChanAttr(RFChan,
		
			SET_XMTRTYPE,		TRUE+1,
		/* Thes are no longer HSlines but an apbus register values for phase */
			SET_PHLINE0,		0x00, /*   0 */
			SET_PHLINE90,		0x40, /*  90 */
			SET_PHLINE180,		0x80, /* 180 */
			SET_PHLINE270,		0xC0, /* 270 */
		NULL);
         }
      }

      /* for backwards compatibility */
      switch(rfchan)
      {
	case TODEV:
		RFChan1 = RFChan;
        	ToAttn = CattnObj;
	 	ToLnAttn = FattnObj;
		break;
	case DODEV:
		RFChan2 = RFChan;
        	DoAttn = CattnObj;
	 	DoLnAttn = FattnObj;
		break;
	case DO2DEV:
		RFChan3 = RFChan;
        	Do2Attn = CattnObj;
		Do2LnAttn = FattnObj;
		break;
	case DO3DEV:
		RFChan4 = RFChan;
        	Do3Attn = CattnObj;
		Do3LnAttn = FattnObj;
		break;
	case DO4DEV:
		RFChan5 = RFChan;
        	Do4Attn = CattnObj;
		Do4LnAttn = FattnObj;
		break;
	default:
		break;
      }
    }

/*---------------------------------------------------------------------------*/

    if (rotorSync == 1)    /* solids linear attn & rotor sync */
    {
         HS_Rotor = ObjectNew(Event_Device,"High Speed Rotor Control");
    	 SetEventAttr(HS_Rotor,SET_DEFAULTS,
			    SET_TYPE,		EVENT_HS_ROTOR,
			    SET_MAXRTPARS,	14,
			    SET_VALID_RTPAR,	v1, SET_VALID_RTPAR,	v2,
                            SET_VALID_RTPAR,	v3, SET_VALID_RTPAR,	v4,
                            SET_VALID_RTPAR,	v5, SET_VALID_RTPAR,	v6,
                            SET_VALID_RTPAR,	v7, SET_VALID_RTPAR,	v8,
                            SET_VALID_RTPAR,	v9, SET_VALID_RTPAR,	v10,
                            SET_VALID_RTPAR,	v11,SET_VALID_RTPAR,	v12,
                            SET_VALID_RTPAR,	v13,SET_VALID_RTPAR,	v14,
                      NULL);
    }
    return(OK);
}

whatxmtrtype(device)
{
    if ( (rftype[device-1] == 'c')  || (rftype[device-1] == 'C') ||
         (rftype[device-1] == 'd')  || (rftype[device-1] == 'D') )
    {
	return(TRUE);
    }
    else if ( (rftype[device-1] == 'l')  || (rftype[device-1] == 'L') )
    {
	return(TRUE+1);  /* Lock/Dec Board , for rfchan_device in SET_XMTRGATE */
    }
    else
    {
	return(FALSE);
    }
}

whatamptype(device)
{
    if ( (amptype[device-1] == 'c')  || (amptype[device-1] == 'C') )
    {
	return(FALSE);
    }
    else
    {
	return(TRUE);
    }
}
