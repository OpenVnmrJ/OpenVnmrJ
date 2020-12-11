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
#include "macros.h"
#include "apdelay.h"
#include "group.h"
#include "pvars.h"
#include "abort.h"

#define CURRENT	1

extern int      bgflag;		/* debug flag */
extern int     	ap_interface;
extern int	ap_ovrride;
extern char     dseq[MAXSTR];
extern void	prg_dec_off();
extern int	pgd_is_running();
extern char *ObjError(int wcode);
extern int SetAPBit(Object obj, ...);
extern void HSgate(int ch, int state);
extern void initdecmodfreq(double freq, int chan, int mode);

#define MODA_TRUE  SET_TRUE
#define MODA_FALSE SET_FALSE
#define MODB_TRUE  SET_TRUE
#define MODB_FALSE SET_FALSE

#define SW_MODE 12
#define SG_MODE 13
#define SM_MODE 14
#define SX_MODE 15
#define N_MODE  8
#define X_MODE  7
#define M_MODE  6
#define G_MODE  5
#define E_MODE  4
#define W_MODE  3
#define S_MODE  2
#define F_MODE  1
#define C_MODE  0
#define R_MODE  -1      /* reset mode */
#define ILLEGAL	-2

static char	dm_tmp;
static int      decstatus[MAX_RFCHAN_NUM + 1] =
                         { R_MODE, R_MODE, R_MODE,
                           R_MODE, R_MODE, R_MODE,
                           R_MODE};
static double	save_scale[MAX_RFCHAN_NUM + 1] =
			 { 1.0,    1.0,    1.0,
			   1.0,    1.0,    1.0,
			   1.0 };
       int	sync_flag;

static void set_dmm_mode(int channel, int on, char mod_mode, int sync, double local_dmf, double *delaytime);
static void setdecmodulation(int status_a, int status_b);
static void set_channel_modulation(int mode, int channel);
void gatedmmode(int statindex, int channel, double *delaytime);


/*------------------------------------------------------------------
|
|	gatedecoupler()/1
|	set the High Speed lines for the decoupler functions
|       the HS lines set (or pattern) is governed by dm,dmm status
|	(i.e., status: A,B,C,D)
|	dm[chan]  - chan# decoupler is on or off
|	dmm[chan] - chan# type of decoupler modulation
|       Note: HSlines is passed to acquisition in EVENT1 & EVENT2
|	      Acodes, only then are the lines actually set
|				Author Greg Brissey 6/26/86
|   Modified:   10/25/88  Greg Brissey
|	        Do not set DECPP if linear amp otherwise dec signal
|		goes to non-existent 25watt instead the 1 watt amp.
|	        The high speed line DECPP is being used	to switch a
|	        mechanical relay in 300 & 400s.
|   Note: On newer RF backplanes 1/26/90, dd
+-----------------------------------------------------------------*/
void gatedecoupler(int statindex, double delaytime)
{
   int		chan;
   int          index;
   int		MINch;
   int		nodelay;
   char		tbuf[10];

   /* ---  check for delay --- */
   if (delaytime > 0.0)
	nodelay = FALSE;
   else
	nodelay = TRUE;

   /* ------ type of RF backplane */
   if (ap_interface < 3)
   {
      if ((dhpflag == TRUE) && (newdecamp != TRUE))	/* dhp & not lin. amp */
	 HSgate(DECPP, TRUE);	/* set decoupler High Power bit On */
      else
	 HSgate(DECPP, FALSE);	/* set decoupler High Power bit Off */
   }

   MINch = (ap_interface < 4) ? DODEV : TODEV;
   for (chan=MINch; chan<=NUMch; chan++)
   {
      if (bgflag)
      {
         fprintf(stderr, "gatedecoupler(): initial HSlines: 0x%x \n", HSlines);
         fprintf(stderr, "gatedecoupler(): Transmitter RFchan%d\n", chan);
      }

      if (*(ModInfo[chan].MI_dmsize) > 0)
      {
	 index = statindex;	/* keep orginal index, dmm[chan] maybe
				 * different size */
	 if ( index >= *(ModInfo[chan].MI_dmsize) )
	    index = *(ModInfo[chan].MI_dmsize) - 1;

         dm_tmp = ModInfo[chan].MI_dm[index];
	 if ((dm_tmp == 'Y') || (dm_tmp == 'S') || (dm_tmp == 'A') ||
	     (dm_tmp == 'y') || (dm_tmp == 's') || (dm_tmp == 'a'))
         { /* set modulation mode and turn xmtr on */
            gatedmmode(statindex, chan, &delaytime);
	    SetRFChanAttr(RF_Channel[chan], SET_XMTRGATE, ON, 0);
            if (P_getstring(GLOBAL, "cwblanking", tbuf, 1, sizeof(tbuf)) == 0
                && *tbuf == 'y')
            {
                SetRFChanAttr(RF_Channel[chan], SET_RCVRGATE, OFF, 0);
            }
         }
	 else
         { /* turn xmtr off and set modulation mode */
	    SetRFChanAttr(RF_Channel[chan], SET_XMTRGATE, OFF, 0);
            gatedmmode(statindex, chan, &delaytime);
            if (P_getstring(GLOBAL, "cwblanking", tbuf, 1, sizeof(tbuf)) == 0
                && *tbuf == 'y')
            {
                SetRFChanAttr(RF_Channel[chan], SET_RCVRGATE, ON, 0);
            }
         }
      }
      else
      { /*default to off */
	 SetRFChanAttr(RF_Channel[chan], SET_XMTRGATE, OFF, 0);
         gatedmmode(statindex, chan, &delaytime);
	 if (pgd_is_running(chan))
            delaytime = delaytime - PRG_STOP_DELAY;
	 prg_dec_off(2, chan);
      }
   }
   if (delaytime > 0.0)
      delay(delaytime);
   else
   {
      if ((nodelay == FALSE) && (delaytime < 0.0))
	text_error("Warning:  statusdelay time not long enough for requested actions.");
   }
}



/* to set the post exp decoupler state: leaves decoupler on only for dm='a' */

void setPostExpDecState(statindex,delaytime)
int             statindex;      /* status index (i.e.,index into dm,dmm) */
double          delaytime;
{
   int          chan;
   int          index;
   int          MINch;
   int          nodelay;
   char         dmm_char;

   /* ---  check for delay --- */
   if (delaytime > 0.0)
        nodelay = FALSE;
   else
        nodelay = TRUE;

   /* ------ type of RF backplane */
   if (ap_interface < 3)
   {
      if ((dhpflag == TRUE) && (newdecamp != TRUE))     /* dhp & not lin. amp */
         HSgate(DECPP, TRUE);   /* set decoupler High Power bit On */
      else
         HSgate(DECPP, FALSE);  /* set decoupler High Power bit Off */
   }

   MINch = (ap_interface < 4) ? DODEV : TODEV;
   for (chan=MINch; chan<=NUMch; chan++)
   {
      if (bgflag)
      {
         fprintf(stderr, "setPostExpDecState(): initial HSlines: 0x%x \n", HSlines);
         fprintf(stderr, "setPostExpDecState(): Transmitter RFchan%d\n", chan);
      }

      if (*(ModInfo[chan].MI_dmsize) > 0)
      {
         index = statindex;     /* keep orginal index, dmm[chan] maybe
                                 * different size */
         if ( index >= *(ModInfo[chan].MI_dmsize) )
            index = *(ModInfo[chan].MI_dmsize) - 1;

         dm_tmp = ModInfo[chan].MI_dm[index];

         dmm_char='c';
         if ( *(ModInfo[chan].MI_dmmsize) > 0)
         {
           dmm_char = ModInfo[chan].MI_dmm[0];
         }

         if ( ((dm_tmp == 'A') && (dmm_char != 'p')) || ((dm_tmp == 'a') && (dmm_char != 'p')) )
         { /* set modulation mode and turn xmtr on */
            gatedmmode(statindex, chan, &delaytime);
            SetRFChanAttr(RF_Channel[chan], SET_XMTRGATE, ON, 0);
         }
         else
         { /* turn xmtr off and set modulation mode */
            SetRFChanAttr(RF_Channel[chan], SET_XMTRGATE, OFF, 0);
            gatedmmode(statindex, chan, &delaytime);
            if (pgd_is_running(chan))
              delaytime = delaytime - PRG_STOP_DELAY;
            prg_dec_off(2, chan);
         }
      }
      else
      { /*default to off */
         SetRFChanAttr(RF_Channel[chan], SET_XMTRGATE, OFF, 0);
         gatedmmode(statindex, chan, &delaytime);
         if (pgd_is_running(chan))
            delaytime = delaytime - PRG_STOP_DELAY;
         prg_dec_off(2, chan);
      }
   }
   if (delaytime > 0.0)
      delay(delaytime);
   else
   {
      if ((nodelay == FALSE) && (delaytime < 0.0))
        text_error("Warning:  statusdelay time not long enough for requested actions.");
   }
}


/*------------------------------------------------------------------
|
|	gatedmmode()/3
|	Check if 'dmm' has a field for statindex, else default
|	to last index, then call set_dmm_mode' to select modualtion
+-----------------------------------------------------------------*/
void gatedmmode(int statindex, int channel, double *delaytime)
{
   int	sync;
   int	on;


   if (bgflag)
      fprintf(stderr, "gatedmmode(): initial HSlines: 0x%x \n", HSlines);

/* --- gate decoupler modulation mode according to field(statindex) --- */
/* 1st Decoupler Channel:  if 'w','c', 's', or 'n' not found, then
   dmm is set to external */

   if ( *(ModInfo[channel].MI_dmmsize) <= 0) return;

   if ( statindex >= *(ModInfo[channel].MI_dmmsize) )
      statindex = *(ModInfo[channel].MI_dmmsize) - 1;

   sync = ( (dm_tmp == 's') || (dm_tmp == 'S') );
   on   = ((dm_tmp == 'Y') || (dm_tmp == 'S') || (dm_tmp == 'A') ||
	   (dm_tmp == 'y') || (dm_tmp == 's') || (dm_tmp == 'a'));

   set_dmm_mode(channel, on, ModInfo[channel].MI_dmm[statindex],
			 sync, (*(ModInfo[channel].MI_dmf)), delaytime );

   if (bgflag)
      fprintf(stderr, "gatedmmode(): final HSlines: 0x%x \n", HSlines);
}

/*------------------------------------------------------------------
|	setstatus()/4
|	allows the user to program modulation modes independent
|	of dm and dmm and status().
|   int	channel 	 the channel to reprogram
|   int	on	 	     xmtr on (TRUE/FALSE)
|   char mod_mode	 modulation type
|   int	sync		 flag, only used if apinterface=4 (hydra)
|   double	set_dmf	 set this dmf value
+-----------------------------------------------------------------*/
void setstatus(int channel, int on, char mod_mode, int sync, double set_dmf)
{
   double delaytime = 0.0;

   if ( (channel<1) || (channel>NUMch) )
   {
      abort_message("setstatus(): system not configured for channel %d",
                    channel);
   }
   if (ap_interface!=4 && sync && mod_mode!='p' && mod_mode!='P')
   {  text_error("This system supports synchrounous modulation only with PPM");
      psg_abort(1);
   }

   /* ------ type of RF backplane */
   if (ap_interface < 3)
   {
      if ((dhpflag == TRUE) && (newdecamp != TRUE))	/* dhp & not lin. amp */
	 HSgate(DECPP, TRUE);	/* set decoupler High Power bit On */
      else
	 HSgate(DECPP, FALSE);	/* set decoupler High Power bit Off */
   }

   if ( on )
   {  /* set modulation mode and turn xmtr on */
      set_dmm_mode(channel,on,mod_mode,sync,set_dmf,&delaytime);
      SetRFChanAttr(RF_Channel[channel], SET_XMTRGATE, ON, 0);
   }
   else
   {  /* turn xmtr off and set modulation mode */
      SetRFChanAttr(RF_Channel[channel], SET_XMTRGATE, OFF, 0);
      set_dmm_mode(channel,on,mod_mode,sync,set_dmf,&delaytime);
   }

}


/*------------------------------------------------------------------
|
|	set_dmm_mode()/5
|	For systems with AP_INTERFACE = 1 or 2:
|		mod_mode is selected over HS lines, moda and modb
|		stored in moda, modb
|		Valid modes are:
|		'c'=00=cw 	'n'=01=Noise
|		'w'=10=Waltz	'f'=11=fm-fm
|		'p'=cw+PPM(if present)
|	For systems with AP_INTERFACE=3:
|		stored in dmmode
|		mode_mode is selected over AP bus.
|		Valid modes are:
|		'c'=00=cw 	'n'=01=noise
|		'w'=10=Waltz	'f'=11=fm-fm
|		'p'=cw+PPM(if present)
|       	2nd Decoupler dmm2 can only be 'c' or 'p'
|	For systems with AP_INTERFACE=4 (hydra):
|		mod_mode is selected over the AP bus.
|		stored in h_dmm.
|		Each transmitter has a controller with a modulator.
|		Valid modes are:
|		'c'=cw or PPM	'r'=square wave	'u'=external
|		'f'=fm-fm	'w'=Waltz	'g'=Garp
|		'm'=mlev16	'x'=xy32	'p'=cw+PPM(if present)
+-----------------------------------------------------------------*/
/*  channel	 channel to program */
/*  mod_mode	 mode to select */
/*  sync	 if synchronous (TRUE/FALSE) */
/*  on		 if PPM (TRUE/FALSE) */
/*  local_dmf	 set this dmf */
/*  *delaytime	 given time. the time for requested actions 	*/
/*               will be subtracted from this time.		*/
static void set_dmm_mode(int channel, int on, char mod_mode, int sync, double local_dmf, double *delaytime)
{
   int  decstat;
   int	dmmode;
   int	h_dmm;
   int	moda,modb;
   double dmfscale = 1.0;
   void setHSLdelay();

   switch( mod_mode )
   {
   case 'w':
   case 'W': /* Waltz */
		if (channel == 2)
		{  dmmode = MODMA;
		   moda   = MODA_TRUE;
		   modb   = MODB_FALSE;
		}
		else
		{  dmmode = ILLEGAL;
		   moda   = ILLEGAL;
		   modb   = ILLEGAL;
		}
		h_dmm   = 4;
                decstat = W_MODE;
		break;
   case 'n':
   case 'N': /* Square Wave */
		if (channel == 2)
		{  dmmode = MODMB;
		   moda   = MODA_FALSE;
		   modb   = MODB_TRUE;
		}
		else
		{  dmmode = ILLEGAL;
		   moda   = ILLEGAL;
		   modb   = ILLEGAL;
		}
		h_dmm   = ILLEGAL;
                decstat = N_MODE;
		break;
   case 'f':
   case 'F': /* FM-FM */
		if (channel == 2)
		{  dmmode = MODMA + MODMB;
		   moda   = MODA_TRUE;
		   modb   = MODB_TRUE;
		}
		else
		{  dmmode = ILLEGAL;
		   moda   = ILLEGAL;
		   modb   = ILLEGAL;
		}
		h_dmm   = 2;
                decstat = F_MODE;
		break;
   case 'c':
   case 'C': /* cw */
		dmmode  = 0;
		moda    = MODA_FALSE;
		modb    = MODB_FALSE;
		h_dmm   = 0;
                decstat = C_MODE;
		break;
   case 'p':
   case 'P': /* programmed decoupling mode */
		dmmode  = 0;
		h_dmm   = 0;
		moda    = MODA_FALSE;
		modb    = MODB_FALSE;
                decstat = C_MODE;
		break;
   case 'r':
   case 'R': /* Rectangular, Square Wave */
		dmmode   = ILLEGAL;
		moda     = ILLEGAL;
		modb     = ILLEGAL;
		dmfscale = 0.25;
		h_dmm    = 1;
                decstat  = S_MODE;
		break;
   case 'u':
   case 'U': /* User suplied, External*/
		dmmode  = ILLEGAL;
		moda    = ILLEGAL;
		modb    = ILLEGAL;
		h_dmm   = 3;
                decstat = E_MODE;
		break;
   case 'm':
   case 'M': /* mlev */
		dmmode  = ILLEGAL;
		moda    = ILLEGAL;
		modb    = ILLEGAL;
		h_dmm   = 6;
                decstat = M_MODE;
		break;
   case 'g':
   case 'G': /* garp */
		dmmode   = ILLEGAL;
		moda     = ILLEGAL;
		modb     = ILLEGAL;
		dmfscale = 45.0;
		h_dmm    = 5;
                decstat = G_MODE;
		break;
   case 'x':
   case 'X': /* xy32 */
		dmmode  = ILLEGAL;
		moda    = ILLEGAL;
		modb    = ILLEGAL;
		h_dmm   = 7;
                decstat = X_MODE;
		break;
   default:  /* non existing */
		dmmode = ILLEGAL;
		moda    = ILLEGAL;
		modb    = ILLEGAL;
		h_dmm   = ILLEGAL;
                decstat = C_MODE;
		break;
   }

   if ( (dmmode==ILLEGAL && ap_interface<3) ||
        (  moda==ILLEGAL && modb==ILLEGAL && ap_interface==3) ||
        ( h_dmm==ILLEGAL && ap_interface==4) )
   {
      abort_message("Unsupported modulation mode %c on channel %d\n",
		mod_mode,channel);
   }
   /* initialize dmm HSlines but not AP_bus lines */
   if (ap_interface < 3)
   {  HSgate(MODMA + MODMB, FALSE);	/* initial dmm HSlines */
      HSgate(dmmode,TRUE);
   }
   else if (ap_interface == 3)
   {
      if (decstatus[channel] != decstat)
      {  if (channel == 2)
	 {
	 	setdecmodulation(moda, modb); /* Only do channel 2 */
	  	*delaytime = *delaytime - SETDECMOD_DELAY;
	 }
         decstatus[channel] = decstat;
      }
   }
   else if (ap_interface == 4)
   {  if (!sync)
      {  h_dmm |= 0x8;
         decstat |= 0x8; 
      }
      if ( (int)(*(ModInfo[channel].MI_dmf) + 0.5) != (int)(local_dmf  + 0.5)
 || (int)((4.0 * save_scale[channel]) + 0.5) != (int)((4.0 * dmfscale) + 0.5) )
      {   initdecmodfreq( local_dmf*dmfscale, channel, INIT_APVAL );
          initdecmodfreq( local_dmf*dmfscale, channel, SET_APVAL  );
          *(ModInfo[channel].MI_dmf) = local_dmf;
          save_scale[channel] = dmfscale;
	  *delaytime = *delaytime - DECMODFREQ_DELAY;
      }
      if ( !(sync_flag & (1<<channel)) && sync)
      {  if (mod_mode!='p' && mod_mode!='P')
         {  SetRFChanAttr(RF_Channel[channel], SET_XMTRGATE, OFF, 0);
            set_channel_modulation(8,channel);	/* cw mode sync mod counters */
/*            decstat = C_MODE; */
            sync_flag |= (1<<channel);
	  *delaytime = *delaytime - SETDECMOD_DELAY;
         }
      }
      if (decstatus[channel] != decstat)
      {  set_channel_modulation(h_dmm,channel);
         decstatus[channel] = decstat;
	 *delaytime = *delaytime - SETDECMOD_DELAY;
      }
   }

   if ( ((mod_mode == 'p') || (mod_mode == 'P')) && on )
   {  /* programmable decoupler:  RF channel must have a WFG */
      if (!pgd_is_running(channel))
         *delaytime = *delaytime - PRG_START_DELAY;
      prg_dec_on(   ( (ModInfo[channel].MI_dseq)),
		      1/(*(ModInfo[channel].MI_dmf)),
		      (*(ModInfo[channel].MI_dres)),
		      channel);
   }
   else
   {
      if (pgd_is_running(channel))
         *delaytime = *delaytime - PRG_STOP_DELAY;
      prg_dec_off(2, channel);
   }
}

/*------------------------------------------------------------------
|
|	setdecmodulation()/2
|	set the AP Bus register bits for the decoupler modulation type
|				Author Greg Brissey 6/26/86
+------------------------------------------------------------------*/
static void setdecmodulation(int status_a, int status_b)
{
   int             error,
		   param_cnt;
   Msg_Set_Param   param;
   Msg_Set_Result  result;

   if (bgflag)
   {
      fprintf(stderr, "setdecmodulation():  status_a = %d  status_b = %d\n",
	      status_a, status_b);
   }

   param_cnt = 2;
   param.setwhat = status_a;
   param.value = MODA;

   while (param_cnt--)
   {
     error = Send(RF_Opts, MSG_SET_APBIT_MASK_pr, &param, &result);
     if (error < 0)
     {
        text_error("%s : %s\n", RF_Opts->objname, ObjError(error));
     }

     param.value = MODB;
     param.setwhat = status_b;
   }

   SetAPBit(RF_Opts, SET_VALUE, NULL);
}

/*------------------------------------------------------------------
|
|	set_channel_modulation()/2
|	set the AP Bus register bits for the transmitter modulation type
|				Author Greg Brissey 6/26/86
+------------------------------------------------------------------*/
static void set_channel_modulation(int mode, int channel)
{
   if (bgflag)
   {
      fprintf(stderr, "set_channel_modulation():  mode = %d  channel = %d\n",
	      mode, channel);
   }
    mode |= 0x80;		/* enable clock */
   SetRFChanAttr(RF_Channel[channel],
			SET_MOD_MODE,	mode,
			SET_MOD_VALUE,
			NULL);
}
/*-----------------------------------------------
|                                               |
|                setHSLdelay()/0                |
|                                               |
+----------------------------------------------*/
void setHSLdelay()
{
   if (fifolpsize < 100)
   {
      if (ap_ovrride)
      {
         ap_ovrride = 0;
      }
      else
      {
         G_Delay(DELAY_TIME, 0.2e-6, 0);  /* allows HS lines to be set */
      }
   }
}
/*-----------------------------------------------
|                                               |
|             reset_decstatus()/0               |
|                                               |
+----------------------------------------------*/
void reset_decstatus()
{
int   i;
   for (i=1; i<=MAX_RFCHAN_NUM; i++)
   {  decstatus[i]  = R_MODE;
      save_scale[i] = 1;
   }
}
