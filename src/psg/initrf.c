/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "oopc.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "chanstruct.h"
#include "group.h"
#include "apdelay.h"

#ifdef RFCHANNELS
#include "macros.h"
#endif

/*-----------------------------------------------------------------
|
|	initialRF()
|	initialize basic RF hardware  
|		
|				Author Greg Brissey  5/13/86
|   Modified   Author     Purpose
|   --------   ------     -------
|   12/13/88   Greg B.    1. Added Code to setup linear attenuators
|			     for ap_interface >= 3, using tpwrf,dpwrf.
|			  2. Course Decoupler Attenuators controlled
|			     by dpwr not dhp now.
|			  3. Change atten. setup to use an absolute value
|			     instead of the dedicated real-time variable
|   6/29/89   Greg B.     1. Seperated apbus word calculating and setting
|			     initialRF() - calculates & stores apbus words 
|			     setRF() - sets the apbus words (apbout).
|   7/27/89   Greg B.     1. Added Frequency Objects, SetFreqAttr()
+---------------------------------------------------------------*/
extern codeint lkflt_fast;
extern codeint lkflt_slow;
extern codeint initflagrt;

extern rfvarinfo RFInfo[];

extern int newacq;
extern int acqiflag;
extern int ap_interface;
extern int bgflag;
extern int putcode();

extern Object RF_Rout,RF_Opts,RF_Opts2;

initialRF()
{
int rfchan;
   for (rfchan=1; rfchan <= NUMch; rfchan++)
   {  SetRFChanAttr(RF_Channel[rfchan],
		SET_SPECFREQ,	*RFInfo[rfchan].rfsfrq,
		SET_INIT_OFFSET,	*RFInfo[rfchan].rftof,
		SET_PHASEATTR,	NULL,
		NULL);
   }
   /* we have to set all freqs before we set gate-mode */
   if (ap_interface == 4)	 	/* init dec mod freq */
   {  
      for (rfchan=1; rfchan <= NUMch; rfchan++)
      {
         initdecmodfreq(*(ModInfo[rfchan].MI_dmf),rfchan,INIT_APVAL);
      }
   }
   if (ap_interface < 4)
      initdecmodfreq(dmf,2,INIT_APVAL);/* init decoupler modulation freq  */

   if (ap_interface < 4)
   {  pulseampfilter(INIT_APVAL);       /* set trans, filter & pulse amp. */
      dowlfiltercontrol(INIT_APVAL);    /* set Wideline recvr broad,apbchip*/
   }
   dofiltercontrol(INIT_APVAL);		/* init wl and audio filter + cntl */
   if (!newdecamp)
        decouplerattn(INIT_APVAL);
   setlkfrqflt( (acqiflag ? 0.0 : 1.0), INIT_APVAL );
   if (ap_interface > 1)
   {
     set_observech(OBSch);		/* done after freq-s have been set */
   }
   if (ap_interface < 4)
      rfbandselect(rfband[0], INIT_APVAL); /* select proper preamp */
}

setRF()
{
codeint		*apbcntadr;
codeint		 apbcount;
int		 rfchan;
int		 gmode;

   if ((newacq) ) ifzero(initflagrt);
      putcode(LKFILTER);
      putcode(lkflt_fast);
      putcode(lkflt_slow);
   if ((newacq) ) endif(initflagrt);

   putcode(APBOUT);
   apbcntadr = Codeptr;			/* save location for count - 1 */ 
   putcode(0);				/* put zero in for now */

   if (ap_interface < 4)
      initdecmodfreq(dmf,2,SET_APVAL);/* set decoupler modulation freq */
   if (ap_interface < 4)
   {  rfbandselect(rfband[0],SET_APVAL);/* select high or low rfband for trans*/
      pulseampfilter(SET_APVAL);        /* set trans, filter & pulse amp. */
      dowlfiltercontrol(SET_APVAL);     /* set Wideline reciever broad,apbchip*/
   }
   else 	/* Init homo decoupling rcvr mask for ap_interface == 4 */
   {
       /* Init with gain for 2nd rcvr; bit 0 = homo bit = 0 */
      SetAPAttr(RCVR_homo, SET_MASK, getGainBits(1), NULL);
   }

   dofiltercontrol(SET_APVAL);		/* set wl and audio filter + cntl */
   updt_interfiddelay(2.0*INOVA_STD_APBUS_DELAY);

   if (!newdecamp)
        decouplerattn(SET_APVAL);

   updt_interfiddelay(3.0*INOVA_STD_APBUS_DELAY);

   apbcount = ( (codeint) (Codeptr - apbcntadr)) - 2;    /* count - 1 */
   *apbcntadr = apbcount;

   if (bgflag)
      fprintf(stderr," apbcount = %d, cntadr = %lx \n",apbcount,apbcntadr);

/*  Since the lock frequency is set with a separate A-code, rather
    than in connection with the massive APBOUT instruction above,
    the call to setlkfrqflt has to be moved past the end of the APBOUT.	*/

   setlkfrqflt( (acqiflag ? 0.0 : 1.0), SET_APVAL );

   /* For Each RF Channel set Freq,Phase,Attn */
   for (rfchan = 1; rfchan <= NUMch; rfchan++)
   { 
      /* Set its PTS and phase  */
      SetRFChanAttr(RF_Channel[rfchan],
			SET_FREQVALUE,	*RFInfo[rfchan].rftof,
    			SET_RTPHASE90,	zero,
			NULL);
      updt_interfiddelay(8.0*INOVA_STD_APBUS_DELAY);

      /* newtrans,newdec */
      if ((rftype[rfchan-1] == 'c') || (rftype[rfchan-1] == 'C') ||
          (rftype[rfchan-1] == 'd') || (rftype[rfchan-1] == 'D') || 
          (rftype[rfchan-1] == 'l') || (rftype[rfchan-1] == 'L') ) 
      {  SetRFChanAttr(RF_Channel[rfchan],
			SET_PHASESTEP,	90.0, 
			SET_PHASE,	0,
	   		NULL);
	 updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);
      }

      /* coarse attenuator */
      if (cattn[rfchan] != 0.0)
      {  SetRFChanAttr(RF_Channel[rfchan],
			SET_PWR,	(int)(*RFInfo[rfchan].rftpwr),
	   		NULL);
	 updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);
      }

      /* fine attenuator */
      if (fattn[rfchan] != 0.0)
      {  SetRFChanAttr(RF_Channel[rfchan],
			SET_PWRF,	(int)(*RFInfo[rfchan].rftpwrf),
	   		NULL);
	 updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);
      }

      /* set gate and phase select lines to default values */
      if (ap_interface == 4)
      {  initdecmodfreq(*(ModInfo[rfchan].MI_dmf),rfchan,SET_APVAL);
         updt_interfiddelay(3.0*INOVA_STD_APBUS_DELAY);

	 /* if Lock/Dec don't set it here, HS is being used as HSline, XMTRGATE so don't set it */
	 /* XMTRGATE handled the difference, (HSlines are apbus setting & phase is apbus also ) */
         /* seperate acode to handle phase */
         if ((rftype[rfchan-1] != 'l') && (rftype[rfchan-1] != 'L') )
	 {
            SetRFChanAttr(RF_Channel[rfchan],
			SET_HS_VALUE,		/* no param->value */
			NULL);
            updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);
         }
         SetRFChanAttr(RF_Channel[rfchan],
			SET_MOD_VALUE,		/* no param->value */
			NULL);
         updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);
         SetRFChanAttr(RF_Channel[rfchan],
			SET_MIXER_VALUE, -1,	/* no param->value */
			NULL);
         updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);
         SetRFChanAttr(RF_Channel[rfchan],
			SET_GATE_MODE,		set_gate_mode(rfchan),
			SET_XGMODE_VALUE,	0,/* no param->value */
			NULL);
         updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);

	 /* init RCVR_homo bit to be set later */
         if (ModInfo[rfchan].MI_homo[0]=='y')
            SetAPAttr(RCVR_homo, SET_MASK, getGainBits(1) | 1, NULL);
      }
   }

   /* New RF Interface Board Code */
   if ( (ap_interface > 1) && (ap_interface < 4) )
   {  SetAPBit(RF_Rout, SET_VALUE,NULL);	/* generate APBOUT */
      SetAPBit(RF_Opts, SET_VALUE,NULL);	/* generate APBOUT */
      SetAPBit(RF_Opts2,SET_VALUE,NULL);	/* generate APBOUT */
   }
   if (ap_interface == 4)
   {
      set_preamp_mask();	/* init homo decoupling bits in RF_TR_PA */
      SetAPAttr(RF_TR_PA,  SET_VALUE,NULL);	/* generate APBOUT */
      updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);
      SetAPAttr(RF_Amp1_2, SET_VALUE,NULL);	/* generate APBOUT */
      updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);
      SetAPAttr(RF_Amp3_4, SET_VALUE,NULL);	/* generate APBOUT */
      updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);
      SetAPAttr(RF_Amp_Sol, SET_VALUE,NULL);	/* generate APBOUT */
      updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);
      SetAPAttr(RCVR_homo, SET_VALUE,NULL);	/* generate APBOUT */
      updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);
/*      SetAPBit (RF_Amp_AP, SET_VALUE,NULL);	/* generate APBOUT */

      /* set up 4t channel magleg PIC register values base on 0xb49 register value */
      setPICreg();
      SetAPBit (RF_PICrout, SET_VALUE,NULL);	/* generate APBOUT */

      SetAPBit (RF_MLrout, SET_VALUE,NULL);	/* generate APBOUT */
      updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);
      SetAPBit (RF_hilo,   SET_VALUE,NULL);	/* generate APBOUT */
      updt_interfiddelay(1.0*INOVA_STD_APBUS_DELAY);
   }
}

/*-------------------------------------------------------------------
* 4 channel magnet leg PIC routing  new register 0xB4E
* 	PIC_MIX1SEL             0        = ! MAGLEG_HILO 
*	PIC_MIX2SEL             1        = ! MAGLEG_HILO 
*	PIC_PSEL0               2        = (MAGLEG_HILO && ! MAGLEG_MIXER) || Tune 
*	PIC_PSEL1               3        = (MAGLEG_MIXER) || Tune 
*	PIC_IN2SEL0             4        = (MAGLEG_HILO && ! MAGLEG_MIXER) || Tune 
*	PIC_IN2SEL1             5        = (MAGLEG_MIXER) || Tune 
*	PIC_ARRAY               6        = mrarray='y' 
*	PIC_DELAY               7        = highq='y' 
*	
*   GMB   8/16/02
*/
setPICreg()
{
    int             error;
    int             result;
    Msg_Set_Param   param;
    Msg_Set_Result  obj_result;
    int mlregvalue, hilo, mixer,sel0, notHilo,sel0Cmd,mixerCmd;
    int tmpregval;

   /* obtain the register value of RF_MLrout, since the IN2SEL0,IN2SEL1,MIX1SEL and MIX2SEL
      are defined by the lower 2 bits
      of this register (0xb49, MAGLEG_HILO & MAGLEG_MIXER)
   */
    param.setwhat = GET_VALUE;
    error = Send(RF_PICrout, MSG_GET_APBIT_MASK_pr, &param, &obj_result);
    if (error < 0)
    {  char    msge[128];

       sprintf(msge, "%s : %s\n", RF_MLrout->objname, ObjError(error));
           text_error(msge);
    }
    tmpregval = (int) (obj_result.reqvalue);
    if (bgflag)
       fprintf(stderr,"setPICreg(): RF_PICrout - present val: 0x%x\n",tmpregval);

    param.setwhat = GET_VALUE;
    error = Send(RF_MLrout, MSG_GET_APBIT_MASK_pr, &param, &obj_result);
    if (error < 0)
    {  char    msge[128];

       sprintf(msge, "%s : %s\n", RF_MLrout->objname, ObjError(error));
           text_error(msge);
    }
    mlregvalue = (int) (obj_result.reqvalue);
    if (bgflag)
       fprintf(stderr,"setPICreg(): RF_MLrout -  val: 0x%x\n",mlregvalue);

    /* determine state of MAGLEG_HILO and MAGLEG_MIXER bits, and create command to set the
       PIC bits
    */
    hilo = (mlregvalue >> MAGLEG_HILO) & 0x1;
    mixer = (mlregvalue  >> MAGLEG_MIXER) & 0x1;
    sel0 = hilo & (!mixer);

    if (bgflag)
       fprintf(stderr,"setPICreg(): hilo: %d, mixer: %d, sel0: %d\n",hilo,mixer,sel0);

    notHilo = (hilo == 0 ) ? SET_TRUE : SET_FALSE;   /* ! hilo */
    sel0Cmd = (sel0 == 1) ? SET_TRUE : SET_FALSE;
    mixerCmd = (mixer == 1) ? SET_TRUE : SET_FALSE;

    /* Note: ARRAY, HIGHQ, PSEL0 & PSEL1 are set in initfunc.c */
    SetAPBit(RF_PICrout, notHilo, 	PIC_MIX1SEL, 
    			 notHilo, 	PIC_MIX2SEL, 
			 /* sel0Cmd, 	PIC_PSEL0, */
			 /* mixerCmd,  	PIC_PSEL1, */
			 sel0Cmd,	PIC_IN2SEL0,
			 mixerCmd,   	PIC_IN2SEL1,
	     NULL);
    param.setwhat = GET_VALUE;
    error = Send(RF_PICrout, MSG_GET_APBIT_MASK_pr, &param, &obj_result);
    if (error < 0)
    {  char    msge[128];

       sprintf(msge, "%s : %s\n", RF_MLrout->objname, ObjError(error));
           text_error(msge);
    }
    tmpregval = (int) (obj_result.reqvalue);
    if (bgflag)
       fprintf(stderr,"setPICreg(): RF_PICrout - new val: 0x%x\n",tmpregval);
}

mapRF(index)
{
   int rfchan;
   int index2;

   index2 = index;

   if (ap_interface == 4)
      switch (index)
      { 
         case RFCHAN1: index2=OBSch;
                       break;
         case RFCHAN2: index2=DECch;
                       break;
         case RFCHAN3: index2=DEC2ch;
                       break;
         case RFCHAN4: index2=DEC3ch;
                       break;
         default:      index2=index;
                       break;
      }
   return(index2);
}
