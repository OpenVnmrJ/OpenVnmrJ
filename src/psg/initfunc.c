/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include "oopc.h"
#include "group.h"
#include "rfconst.h"
#include "acqparms.h"
#include "acodes.h"
#include "pvars.h"
#include "abort.h"

#define NOT_OBSERVE	-1
#define NOT_SAME_BAND	-1

extern double getval(const char *name);
extern int SetRFChanAttr(Object obj, ...);
extern int SetAPAttr(Object attnobj, ...);
extern int SetAPBit(Object obj, ...);
extern int whatrftype(int chan);
extern int getFirstActiveRcvr();
extern int numOfActiveRcvr();
extern void formXLwords(double value, int num, int digoffset,
                 int device, int *words);
extern void formXL16words(double value, int num, int digoffset,
                 int device, int *words);
extern int getlkfreqapword(double lockfreq, int h1freq );
extern void oneLongToTwoShort(int lval, short *sval );

extern char amptype[MAXSTR];
extern char systemdir[];

extern int  newacq;
extern int  bgflag;	/* debug flag */
extern int  curfifocount;

/* extern Object RF_Rout,RF_Opts,RF_Opts2; */
extern int rfchan_getrfband();
extern int rfchan_getampband();
extern int ap_interface;
extern char    *ObjError();
// extern int getline();
extern int getlineFIO(FILE *fd,char *targ,int mxline,int *p2eflag);

/* static where the AP words are stored */
static int decmodfreq_apwords[MAX_RFCHAN_NUM + 1][10];
static int rfband_apwrds;
static int ampfilt_apwrds[2];
static int lkfqflt_apwrds[10];
static int decattn_apwrds[2];
static int recfilt_apwrds[6];
static int wlfilt_apwrds[2];

static void set_ampbits();
void set_ampcw();
void set_atgbit();
void setPSELviaMatrix();
void set_magleg_routing();
void setrf_routing();

/*-------------------------------------------------------------------
|
|	initdecmodfreq()/1 
|	set the decoupler modulation frequency 
|       200-400 0-9900 Hz for 500 has been increase 99900 Hz
|				Author Greg Brissey  6/13/86
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/22/89    Greg B.	   1. added mode so that ap words can be calc once
+------------------------------------------------------------------*/
#define TWO_20	1048576		/* 2^20 */
void initdecmodfreq(double freq, int chan, int mode)
{
   int      i,hbyte,lbyte,ubyte,tbyte,num_apwords,ap_reg;
   double   dmf_flt;

   if (bgflag)
     fprintf(stderr,"initdecmodfreq: chan: %d, freq: %lf, mode: %d\n",
	chan,freq,mode);
   if (mode == INIT_APVAL)
   {
      if (ap_interface == 4)
      {
         ap_reg = 0x98 + ((chan-1) * 16);
	 dmf_flt = freq * (float)TWO_20 / 5e6;
         tbyte  = (int)(dmf_flt + 0.5);

         ubyte  = ( tbyte >> 16)  & 0xff;
         hbyte  = ( tbyte >>  8 ) & 0xff;
         lbyte  = ( tbyte       ) & 0xff;

          decmodfreq_apwords[chan][0] = 6;
          decmodfreq_apwords[chan][1] = APBOUT;
          decmodfreq_apwords[chan][2] = 3;
          decmodfreq_apwords[chan][3] = APSELECT   | 0xb00 | ap_reg ;
          decmodfreq_apwords[chan][4] = APWRITE    | 0xb00 | lbyte;
          decmodfreq_apwords[chan][5] = APPREINCWR | 0xb00 | hbyte;
          decmodfreq_apwords[chan][6] = APPREINCWR | 0xb00 | ubyte;
	  if (bgflag)
          {
            fprintf(stderr,"initdecmodfreq: apreg: 0x%x, lbyte: 0x%x, hbyte: 0x%x, ubyte: 0x%x\n",
		decmodfreq_apwords[chan][3],decmodfreq_apwords[chan][4],decmodfreq_apwords[chan][5],
		decmodfreq_apwords[chan][6]);
          }
      }
      else
      {
         if (mode == INIT_APVAL)
         {
            formXLwords(freq,5,DMFOFFSET,DMFDEV,&decmodfreq_apwords[chan][1]);
            decmodfreq_apwords[chan][0] = 5;
         }
      }
   }
   else
   {
      num_apwords = decmodfreq_apwords[chan][0];
      for (i=1; i<=num_apwords; i++)
      {
          putcode((codeint) (decmodfreq_apwords[chan][i]));
/*
          fprintf(stderr,">>>  initdecmodfreq: apword: 0x%lx\n",
	     decmodfreq_apwords[chan][i]);
*/
      }
      curfifocount += num_apwords;
   }
}

/*-------------------------------------------------------------------
|
|	rfbandselect()/1 
|	set the transmitter to proper band (high or low) old style RF
|				Author Greg Brissey  6/13/86
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/22/89    Greg B.	   1. added mode so that ap words can be calc once
+------------------------------------------------------------------*/
void rfbandselect(band,mode)
char band;
int mode;
{
    double rf;
    int             error;
    int             result;
    Msg_Set_Param   param;
    Msg_Set_Result  obj_result;

   if (bgflag)
       fprintf(stderr,"rfbandselect(): band = '%c' \n",band);
   if (mode == INIT_APVAL)
   {
     if (ap_interface < 2)
     {
       result = rfchan_getrfband(OBSch);  /* Acting Observe Channel */
       rf = (result == RF_HIGH_BAND) ? 1.0 : 0.0;
     }
     else
     {
        /* rf band not quit right, use preamp setting instead*/
        /* but this object does not exist for ap_interface < 2 */
        param.setwhat = GET_PREAMP_SELECT;
        error = Send(RF_Opts2, MSG_GET_APBIT_MASK_pr, &param, &obj_result);
        if (error < 0)
        {
           text_error("%s : %s\n", RF_Opts2->objname, ObjError(error));
        }
        rf = (double) (obj_result.reqvalue);
     }
     formXLwords(rf,1,OBSOFFSET,OBSDEV,&rfband_apwrds);
   }
   else
   {
     putcode((codeint) (rfband_apwrds));
     curfifocount++;
   }

}

/*-------------------------------------------------------------------
|
|	pulseampfilter() 
|	set the transmitter's filters and amp power (old style RF)
|				Author Greg Brissey  6/13/86
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/22/89    Greg B.	   1. added mode so that ap words can be calc once
+------------------------------------------------------------------*/
void pulseampfilter(int mode)
{
   if (bgflag)
       fprintf(stderr,"pulseampfilter(): \n");
   if (mode == INIT_APVAL)
    formXLwords(filter,2,0,PAFDEV,ampfilt_apwrds);
   else
   {
     putcode((codeint) (ampfilt_apwrds[0]));
     putcode((codeint) (ampfilt_apwrds[1]));
     curfifocount += 2;
   }
}

static void report_convert_lockfreq_error()
{
   char type_of_console[ 256 ];

   text_error("PSG: can't convert lockfreq to AP word.\n");
   if (P_getstring(GLOBAL, "Console", type_of_console, 1, sizeof(type_of_console)) == 0)
   {
      if (strcmp( type_of_console, "inova" ) != 0 &&
          strcmp( type_of_console, "mercury" ) != 0)
      {
         text_error("This version of PSG not suitable for a %s console\n",
                  &type_of_console[ 0 ]);
      }
   }
   psg_abort(1);
}

/*-------------------------------------------------------------------
|
|	setlkfrqflt() 
|	set the lock transmitter  frequency and filter 
|       rate = 1 slow rate; rate = 0 fast rate.
|				Author Greg Brissey  6/13/86
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/22/89    Greg B.	   1. added mode so that ap words can be calc once
|   05/1997    Robert L.   2. reworked setlkfrqflt to use SETLOCKFREQ A-code
+------------------------------------------------------------------*/
#define	TWO_24	16777216.0	/* 2 to_the_power 24 */
void setlkfrqflt(rate,mode)
double rate;
int mode;     /* mode instructs the program to perform computations or output A-codes */
{
int	i,num_apwords;
double	lockfreq,lkof;
   if (bgflag)
       fprintf(stderr,"setlkfrqflt(): rate = %3.0f \n",rate);
   if (mode == INIT_APVAL)
   {  if (ap_interface<4)
      {  formXLwords(rate,1,LOCKOFFSET,LOCKDEV,&lkfqflt_apwrds[1]);
         lkfqflt_apwrds[0] = 1; 		    /* 1 apword in array */
      }
      else
      {  if ( P_getreal(GLOBAL,"lockfreq",&lockfreq,1) < 0 )
         {  text_error("PSG: lockfreq not found.\n");
            psg_abort(1);
         }
         else
         { 
            short sval[ 2 ];
            int apval;

/*  Adjust lock frequency by the lock offset parameter  */

            if ( !newacq || P_getreal(GLOBAL, "lkof", &lkof, 1) < 0)
              lkof = 0.0;

            lockfreq += (lkof * 1.0e-6);

/*  This is the old way to set the lock frequency  */

#if 0
	    synthif = 0.0;
	    lockref = 0.0;
	    lksense = '\0';
	    strcpy(lockfreqtab,systemdir); /* $vnmrsystem */
	    strcat(lockfreqtab,"/nuctables/lockfreqtab");
	    if (lockfreqtab_read(lockfreqtab,H1freq,&synthif,&lksense,
								&lockref) == 0)
	    {
		if (bgflag)
		{
		   fprintf(stderr,
			"lockfreqtab: synthif=%g lksense= %c lockref=%g\n",
				synthif,lksense,lockref);
		}
		if (lksense == '+')
		   tmpfreq =  ((lockfreq + synthif) - lockref);
		else
		   tmpfreq = ((lockfreq - synthif) - lockref);

		if (tmpfreq < 0.0) tmpfreq = (-1.0)*tmpfreq;
		if (bgflag)
       		   fprintf(stderr,"setlkfrqflt(): locksynth = %g \n",tmpfreq);
	    }
	    else
	    {
		text_error("PSG: No H1freq setting in lockfreqtab.\n");
        	psg_abort(1);
	    }
            ap_addr = 0xb<<8; ap_reg = 0x54;
	    itmp =  ( (((int)(tmpfreq*TWO_24 + 0.5))/40) );

            lkfqflt_apwrds[1]=APSELECT   | ap_addr | ap_reg;
            lkfqflt_apwrds[2]=APWRITE    | ap_addr | ( (itmp    ) & 0xff);
            lkfqflt_apwrds[3]=APPREINCWR | ap_addr | ( (itmp>>8 ) & 0xff);
            lkfqflt_apwrds[4]=APPREINCWR | ap_addr | ( (itmp>>16) & 0xff);
            lkfqflt_apwrds[0] = 4;
#endif

/*  This is the new way  */

            apval = getlkfreqapword( lockfreq, H1freq );
            if (apval < 0)
            {
                report_convert_lockfreq_error();
            }
            oneLongToTwoShort( apval, &sval[ 0 ] );

	/*  The AP Bus observes the DEC convention of the low order
	    byte/word first, the high order byte/word second, opposite
	    the SPARC/Motorola convention.				*/

            lkfqflt_apwrds[0] = 3;
            lkfqflt_apwrds[1] = SETLOCKFREQ;
            lkfqflt_apwrds[2] = sval[ 1 ];
            lkfqflt_apwrds[3] = sval[ 0 ];
         }
      }
   }
   else
   {  num_apwords = lkfqflt_apwrds[0];
      for (i=1; i<=num_apwords; i++)
          putcode((codeint) (lkfqflt_apwrds[i]));
      curfifocount += num_apwords;
   }
}

/*-------------------------------------------------------------------
|
|	decouplerattn() 
|	set the old style decoupler attenuator 
|       dhp = 0 - 255
|	dlp = 0 - 60 db.
|				Author Greg Brissey  6/13/86
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/22/89    Greg B.	   1. added mode so that ap words can be calc once
|   9/19/90    Greg B.	   1. added transient mode for decpwr usage 
|			      (ap vals not stored, but codes gen. directly ) 
+------------------------------------------------------------------*/
void decouplerattn(int mode)
{
   if (bgflag)
       fprintf(stderr,"decouplerattn(): \n");
   if (mode == INIT_APVAL)
   {
      decattn_apwrds[0] = 0;
      if (dhpflag)
      {
	  if (automated)
	      formXL16words(dhp,2,DLPOFFSET,DLPDEV,decattn_apwrds);
      }
      else
	  formXLwords(dlp,2,DLPOFFSET,DLPDEV,decattn_apwrds);
   }
   else if (mode == SET_APVAL)
   {
     if (decattn_apwrds[0] != 0)
     {
         putcode((codeint) (decattn_apwrds[0]));
         putcode((codeint) (decattn_apwrds[1]));
         curfifocount += 2;
     }
   }
   else if (mode == TRANSIENT_APVAL) /* calc & set, but do not remember setting */
   {
      int tmpwrds[2];
      if (dhpflag)
      {
	  if (automated)
	      formXL16words(dhp,2,DLPOFFSET,DLPDEV,tmpwrds);
      }
      else
	  formXLwords(dlp,2,DLPOFFSET,DLPDEV,tmpwrds);
      putcode((codeint) (tmpwrds[0]));
      putcode((codeint) (tmpwrds[1]));
   }
   else
   {
     abort_message("DECOUPLERATTN(): Illegal mode argument value: %d .\n",mode);
   }
}

int
get_filter_max_bandwidth()
{
    char tbuf[10];
    int fbmax;

    if (P_getstring(GLOBAL, "audiofilter", tbuf, 1, sizeof(tbuf))){
	strcpy(tbuf, "e");
    }
    switch (tbuf[0]){
      case 'e':
      case 'b':
	fbmax = FBMAX;
	break;
      case '2':
	fbmax = 2 * FBMAX;
	break;
      case '5':
	fbmax = 5 * FBMAX;
	break;
      default:
	abort_message("Illegal 'audiofilter' parameter setting: %s", tbuf);
    }
    return fbmax;
}

/*----------------------------------------------------------------------
|	dofiltercontrol()   establish receiver filter 
|       Correct filter bandwidth is established by parameter entry 'fb'
|	The filters are located on the receiver board, controlled via
|	the AP-chip. In addition the liq/wl IF and RF as well as
|	obs/diag bits are set. A total of 4 apb-words are created.
|	Range 100  Hz to 51200  Hz in 100  Hz steps.
|	       50 kHz to  1600 kHz in  50 kHz steps.
|	      If fb='n' then the bypass is set.
+---------------------------------------------------------------------*/
#define	RCVR_ADDR	0x0b
#define RVCR_GAIN	0x42
#define	RCVR_FILTERS	0x40
void dofiltercontrol(mode)
int mode;
{
int	apfilter;
int	apb_byte;
double	fbmax;
double	fbstep;
int	i;
int	num_apbytes;

    if (bgflag)
      fprintf(stderr,"dofiltercontrol(): \n");

    /* Determine fb limit */
    fbmax = get_filter_max_bandwidth();
    fbstep = fbmax / 256;

   if ( mode == INIT_APVAL)
   {  if (ap_interface < 4)
      {
	 float xfb;
	 if (fbmax > FBMAX){
	     if (fb <fbmax){
		 xfb =  fb / (100 * fbmax / FBMAX);
	     }else{
		 xfb = 512;
	     }
	 }else{
	     if ( fb <= 49901.0){
		 xfb =fb / 100.0;
	     }else{
		 xfb = 499.0;
	     }
	 }
	 formXLwords(xfb, 3, 0, FILTERDEV, &recfilt_apwrds[1]);

	 recfilt_apwrds[0] = 3;
         if (fb <= fbmax)
         {
	   recfilt_apwrds[4] = 0xa300;
           /* (b3xx) xx = ((fb * 256) / fbmax) - 1    */
	   apfilter = (int) ( ( (fb / fbstep) - 1.0) + .005 );
	   recfilt_apwrds[5] = (0xb300 + (0x00ff & apfilter)); 
           recfilt_apwrds[0] += 2;
         }
         else
            recfilt_apwrds[4] = 0;
      }
      else
      {  recfilt_apwrds[0] = 3;
         recfilt_apwrds[1] = APSELECT | (RCVR_ADDR<<8) | RCVR_FILTERS;
         if (fb > fbmax)
            apb_byte = (int) ( ((fbmax / fbstep) - 1)  + 0.5 );
	 else
            apb_byte = (int) ( ((fb / fbstep) - 1)  + 0.5 );
         recfilt_apwrds[2] = APWRITE    | (RCVR_ADDR<<8) | (apb_byte&0xff);
         recfilt_apwrds[3] = APPREINCWR | (RCVR_ADDR<<8) | 0x00;
      }
   }
   else
   {
      num_apbytes = recfilt_apwrds[0];
     for (i=1; i<=num_apbytes; i++)
         putcode((codeint) (recfilt_apwrds[i]));
     curfifocount += num_apbytes;
   }
}

/*-------------------------------------------------------------------
|
|	dowlfiltercontrol() 
|        establish receiver filter for new style RF for Wideline Acq.
|        correct filter bandwidth is established by parameter entry 'fb'
|	 
|				Author Greg Brissey  6/13/86
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/22/89    Greg B.	   1. added mode so that ap words can be calc once
+------------------------------------------------------------------*/
void dowlfiltercontrol(mode)
int mode;
{
    double bw;
    int apfilter;

    if (bgflag)
       fprintf(stderr,"dowlfiltercontrol(): \n");

    if (mode == INIT_APVAL)
    {
      bw = fb / 100.0;/* min. filter step 100Hz */

      if (bw > 10000.0)
	  apfilter = 0xb302;		/* 2400kHz */
      else
	  if(bw > 7000.0)
	      apfilter = 0xb303;		/* 1000kHz */
	  else
	      if (bw > 3000.0)
	          apfilter = 0xb304;	/* 700kHz */
	      else
		  if (bw > 1700.0)
	              apfilter = 0xb305;	/* 300kHz */
		  else
		      apfilter = 0xb306;	/* 170kHz */
      wlfilt_apwrds[0] = 0xa301;
      wlfilt_apwrds[1] = apfilter;
    }
   else
   {
     putcode((codeint) (wlfilt_apwrds[0]));
     putcode((codeint) (wlfilt_apwrds[1]));
     curfifocount += 2;
   }
}

/*-------------------------------------------------------------------
|
|	set_observech()/1 
|	set a channel to be the Observe Channel. The channel displaced
|       assumes the function of the displacer.
|				Author Greg Brissey  1/24/90
+------------------------------------------------------------------*/
void set_observech(int channel)
{
    if ((channel < 1) || (channel > NUMch))
    {
      abort_message("set_observech: channel #%d does not exist\n",
	        channel);
    }
    if (bgflag)
	fprintf(stderr,"New Obsch: %d, OBSch: %d, DECch: %d, DEC2ch: %d\n",
		channel,OBSch,DECch,DEC2ch);
   /* reset relays, etc. since they depend on who is Observe channel */
   if (ap_interface != 1)
   {
      if (ap_interface < 4)
      {
         setrf_routing();
         set_ampcw();
      }
      else
      {
         if (TODEV != channel)
           SetAPBit(RF_MLrout, SET_TRUE, MAGLEG_RELAY_2, NULL);
         set_magleg_routing();
         set_atgbit();
         set_ampbits();
      }
   }
}
/*-------------------------------------------------------------------
|
|	setrf_routing()/0 
|	set RF Relays for new RF channel implimentation 
|				Author Greg Brissey  1/22/90
+------------------------------------------------------------------*/
void setrf_routing()
{
    char stmpval[20];
    char legrelay;
    int error;
    int obsamphiband;
    int decamphiband,dec2amphiband;

    /* amp bandmask for all channels */
    /* ampbandmask = rfchan_getampbandmask(TODEV); */

    obsamphiband = rfchan_getampband(OBSch);
    decamphiband = rfchan_getampband(DECch);

    /* ----  Amplifier Output Routing ---- */
    if ( (OBSch == TODEV) || (OBSch == DODEV) )
    {
	int amprelay,decrelay5;

        amprelay  = (obsamphiband) ?  SET_TRUE : SET_FALSE;
        decrelay5 = (decamphiband) ?  SET_FALSE : SET_TRUE;

        error = SetAPBit(RF_Rout, amprelay,AMP_RELAY,
				  decrelay5,DEC_RELAY5,
			NULL);
    }
    else if (OBSch == DO2DEV)  /* Observe 3rd channel */
    {
       dec2amphiband = rfchan_getampband(DEC2ch);

	if ( !decamphiband && !dec2amphiband)
	{
           error = SetAPBit(RF_Rout, SET_TRUE,AMP_RELAY,
				     SET_TRUE,DEC_RELAY5,
			NULL);
	}
	else
	{
           error = SetAPBit(RF_Rout, SET_FALSE,AMP_RELAY,
				     SET_FALSE,DEC_RELAY5,
			NULL);
	}
    }
    else
    {
           abort_message("OBSch has invalid value: %d\n",OBSch);
    }
    /* if any error occurred print error message */
    if (error < 0)
    {
       abort_message("%s : %s\n",RF_Rout->objname,ObjError(error));
    }
    /*-------------------------------------------------------------------
    | LO Routing to Mixer/Preamp
    +--------------------------------------------------------------------*/
    if (OBSch == TODEV)  /* Obs channel the normal Observe channel ? */
    {
       error = SetAPBit(RF_Opts,SET_FALSE,LO_RELAY, NULL); /* LO to Chan1 */
    }
    else
    {
       error = SetAPBit(RF_Opts,SET_TRUE,LO_RELAY, NULL); /* LO to where ever */
    }
    if (error < 0)
    {
      abort_message("%s : %s\n",RF_Opts->objname,ObjError(error));
    }

    /*-------------------------------------------------------------------
    | Preamplifier Routing
    +--------------------------------------------------------------------*/
    if (obsamphiband)
    {
       error = SetAPBit(RF_Opts2,SET_FALSE,PREAMP_SELECT, NULL);
    }
    else
    {
       error = SetAPBit(RF_Opts2,SET_TRUE,PREAMP_SELECT, NULL);
    }
    if (error < 0)
    {
       abort_message("%s : %s\n",RF_Opts2->objname,ObjError(error));
    }
    /*-------------------------------------------------------------------
    | Obs T/R Switch-Magnetic Leg Routing 
    | check for override parameter, if present and has a valid value use it
    | else use stanard logic for setting.
    +--------------------------------------------------------------------*/
    if (P_getstring(CURRENT, "legrelay", stmpval, 1, 18) < 0)
    {
       legrelay='n';
    } 
    else
       legrelay = stmpval[0];

    if ( (legrelay == 'h') || (legrelay == 'H') )
    {
	obsamphiband = 1;  /* force to hi band */
    }
    else if ( (legrelay == 'l') || (legrelay == 'L') ) 
    {
	obsamphiband = 0;  /* force to low band */
    }
    else if ( (legrelay != 'n') && (legrelay != 'N') )
    {
       abort_message("setrf_routing(): legrelay='%c' is invalid. (=l,h,n only)\n",
	  legrelay);
    }

    if (obsamphiband)
    {
       error = SetAPBit(RF_Opts2, SET_TRUE,LEG_RELAY, NULL);
    }
    else
    {
       error = SetAPBit(RF_Opts2, SET_FALSE,LEG_RELAY, NULL);
    }
    if (error < 0)
    {
       abort_message("%s : %s\n",RF_Opts2->objname,ObjError(error));
    }
}

/****************************************************************/
/* This routine determines whether the particular transmitter	*/
/* is a observe transmitter, decoupler, homo or hetero. It the 	*/
/* returns the appropriate bit setting for gate mode		*/
/****************************************************************/
int set_gate_mode(int rfchan)
{
   char	tmpchr;
   char	xmtr_gmode[MAX_RFCHAN_NUM + 1];
   int	gmode;
   int arraymode = 0;
   char mrarray[8];

   if ( (P_getstring(CURRENT, "mrarray", mrarray, 1, 7) == 0) &&
        (mrarray[0] == 'y' || mrarray[0] == 'Y') )
   {
       arraymode = 1;
       /* RF_PICrout array mode set in set_magleg_routing() */
       /* SetAPBit(RF_PICrout, SET_TRUE, PIC_ARRAY, NULL);  PIC chnged */
   }
   if (rfchan == OBSch || arraymode)
   {  if (P_getstring(CURRENT,"logate",xmtr_gmode,1,MAX_RFCHAN_NUM)<0)
	strcpy(xmtr_gmode,"l");
      
      if (xmtr_gmode[0] != 's' && xmtr_gmode[0] != 'S')
	 gmode = 160;		/* liquids observe mode */
      else 
         gmode = 64;		/* solids  observe mode */
   }
   else
   { tmpchr = ModInfo[rfchan].MI_homo[0];
     if ( (tmpchr == 'y') || (tmpchr == 'Y') )
     {  gmode = 192;		/* homo decoupling mode */
     }
     else
     {
       gmode = 96;		/* hetero decoupling mode */
     }
   }
   if (bgflag)
      fprintf(stderr,"chan=%d, OBSch=%d, gmode=%d\n",rfchan,OBSch,gmode);
   return(gmode);
}

/*-------------------------------------------------------------------
|	homo_decouple_routing()/0 
|	Sets lines for homo decoupling when receiver gating line is not
|            tied to amplifier blanking line.
+------------------------------------------------------------------*/
int homo_decouple_routing()
{
int	gmode;
char	tmpchr;

    gmode = 0xd | (0xd << 4);		/* no homo decoupling */
    tmpchr = ModInfo[DODEV].MI_homo[0];
    if (OBSch == TODEV)  /* Obs channel the normal Observe channel ? */
    {
	if ( (tmpchr == 'y') || (tmpchr == 'Y') )
	{
	   if ( rfchan_getampband(OBSch) )
	      gmode = 0x1 | (0x1 << 4);	/* homo decoupling mode for chan 1*/
	   else
	      gmode = 0x2 | (0x2 << 4);	/* homo decoupling mode for chan 2*/
	}
    }
    else if (OBSch == DODEV)
    {
	if ( (tmpchr == 'y') || (tmpchr == 'Y') )
	{
	   if ( rfchan_getampband(OBSch) )
	      gmode = 0x1 | (0x1 << 4);	/* homo decoupling mode for chan 1*/
	   else
	      gmode = 0x2 | (0x2 << 4);	/* homo decoupling mode for chan 2*/
	}
    }
    else if (OBSch == DO2DEV)  /* Observe 3rd channel */
    {
	if ( (tmpchr == 'y') || (tmpchr == 'Y') )
	   gmode = 0x3 | (0x3 << 4);	/* homo decoupling mode for chan 3*/
    }
    else if (OBSch == DO3DEV)  /* Observe 4th channel */
    {
	if ( (tmpchr == 'y') || (tmpchr == 'Y') )
	   gmode = 0x4 | (0x4 << 4);	/* homo decoupling mode for chan 4*/
    }
    else
    {
         abort_message("OBSch has invalid value: %d\n",OBSch);
    }
    return(gmode);
}

/*-------------------------------------------------------------------
|	set_magleg_routing()/0 
|	set the four realys in the magnet leg.
|            and also preamp select, and mixer
|	7/97 - preamp select removed and put int set_preamp_mask called
|		in setRF() routine.
+------------------------------------------------------------------*/
void set_magleg_routing()
{
   char mrarray[8];
   char stmpval[20];
   char tn[20];
   char highq;
   int error __attribute__((unused));

   /* set MagLeg Lock/Normal Observe to Normal */
   SetAPBit(RF_MLrout, SET_FALSE, MAGLEG_MIXER, NULL);

   /* Set Channels 1 and 2 for Broadband (special SIS case)	*/
   if (amptype[0] == 'b')
   {
      if ( (amptype[1] != 'b') && (amptype[1] != 'n') && (NUMch>=2))
      {
      text_error("Warning: Chl 1 and Chl 2 Amp Types are mixed. Check config.");
      }
      SetAPBit(RF_MLrout, SET_FALSE, MAGLEG_HILO, NULL);
      /* SetAPBit(RF_MLrout, SET_FALSE, MAGLEG_MIXER, NULL); */

      /* switch for systems with hi/lo observe amp, broadband decouple amp */
      if ( !rfchan_getampband(OBSch) )
	SetAPBit(RF_MLrout, SET_TRUE, MAGLEG_HILO, NULL);
      else
	SetAPBit(RF_MLrout, SET_FALSE, MAGLEG_HILO, NULL);
      /* return;    removed 12/19/02 for 4 channel imager PIC magnet leg, gmb */
   }
   else	/* added else cause 12/19/02 for 4 channel imager PIC magnet leg, gmb */
   {
      if ( !rfchan_getampband(OBSch) )
         SetAPBit(RF_MLrout, SET_TRUE, MAGLEG_HILO, NULL);
      else
         SetAPBit(RF_MLrout, SET_FALSE, MAGLEG_HILO, NULL);
   }

/* we switch the mixer here, although it isn't conected to anything */
/* the mixer is actually switched with the preamp (HILO) select     */
/* but if it is used in the future with another mixer box, well..   */
/* then here it is 						*/
/* 								*/
/*   if ( !rfchan_getampband(OBSch) ) 				*/
/*      SetAPBit(RF_MLrout, SET_TRUE, MAGLEG_MIXER, NULL); 	*/
/*  else 							*/
/*      SetAPBit(RF_MLrout, SET_FALSE, MAGLEG_MIXER, NULL); 	*/
/* 								*/
/* ------------------------------------------------------------------ */
/*  9/23/96 - Now the mixer bit is used for a set of relays to switch */
/*   the Normal Observe and Lock Observe so that the Transmitter and  */
/*   Receiver can be used on the Lock channel */
/*   when tn='LK' then Trans & Recvr are switched to the lock */
/* ------------------------------------------------------------------ */
   if ( (P_getstring(CURRENT, "tn", tn, 1, 9) == 0) && 
	 (strcmp(tn,"lk") == 0) )
      SetAPBit(RF_MLrout, SET_TRUE, MAGLEG_MIXER, NULL);
   else
      SetAPBit(RF_MLrout, SET_FALSE, MAGLEG_MIXER, NULL);

   mrarray[0]='i'; /* inactive, overriden if parameter is present */
   if ( (P_getstring(CURRENT, "mrarray", mrarray, 1, 7) == 0) &&
        (mrarray[0] == 'y' || mrarray[0] == 'Y') )
   {
       SetAPBit(RF_MLrout, SET_TRUE, MAGLEG_ARRAY_MODE, NULL);
   } else {
       SetAPBit(RF_MLrout, SET_FALSE, MAGLEG_ARRAY_MODE, NULL);
       if (OBSch == TODEV) {
           SetAPBit(RF_MLrout, SET_FALSE, MAGLEG_PREAMP_SEL_0, NULL);
           SetAPBit(RF_MLrout, SET_FALSE, MAGLEG_PREAMP_SEL_1, NULL);
       } else if (OBSch == DODEV) {
           SetAPBit(RF_MLrout, SET_TRUE, MAGLEG_PREAMP_SEL_0, NULL);
           SetAPBit(RF_MLrout, SET_FALSE, MAGLEG_PREAMP_SEL_1, NULL);
       } else if (OBSch == DO2DEV) {
           SetAPBit(RF_MLrout, SET_FALSE, MAGLEG_PREAMP_SEL_0, NULL);
           SetAPBit(RF_MLrout, SET_TRUE, MAGLEG_PREAMP_SEL_1, NULL);
       } else {
           SetAPBit(RF_MLrout, SET_TRUE, MAGLEG_PREAMP_SEL_0, NULL);
           SetAPBit(RF_MLrout, SET_TRUE, MAGLEG_PREAMP_SEL_1, NULL);
       }
   }

   /* 4 channel PIC additions for highq, mrarray and PSEL0,PSEL1  9/13/02  GMB */
   /* Other changes in initrf.c  */

    if (P_getstring(CURRENT, "highq", stmpval, 1, 18) < 0)
    {
       highq='n';
    } 
    else
       highq = stmpval[0];

    if (bgflag)
       fprintf(stderr,"set_magleg_routing(): highq = '%c'\n", highq);
    if ( (highq == 'y') || (highq == 'Y') )
    {
       error = SetAPBit(RF_PICrout, SET_TRUE,PIC_DELAY, NULL);
    }
    else
    {
       error = SetAPBit(RF_PICrout, SET_FALSE,PIC_DELAY, NULL);
    }

    if (bgflag)
        fprintf(stderr,"set_magleg_routing(): mrarray[0]='%c'\n",mrarray[0]);
    setPSELviaMatrix();

    if ((mrarray[0]=='y') || (mrarray[0]=='Y'))
    {
       SetAPBit(RF_PICrout, SET_TRUE, PIC_ARRAY, NULL);
    }
    else if ((mrarray[0]=='n') || (mrarray[0]=='N'))
    {
       SetAPBit(RF_PICrout, SET_FALSE, PIC_ARRAY, NULL);
    }
    else
    {
       int numActiveRcvrs;
       numActiveRcvrs = numOfActiveRcvr();
       if (bgflag)
           fprintf(stderr,"set_magleg_routing(): numActiveRcvrs: %d\n",numActiveRcvrs);
       if (numActiveRcvrs > 1)
           SetAPBit(RF_PICrout, SET_TRUE, PIC_ARRAY, NULL);
    }
}
/*
ARM: (or the Active Receiver Matrix)

 For Andy's 4 channel PIC mleg board
 PSEL0 and PSEL1 are set base on the 1st receiver to be used, which is
 base on the value of the rcvrs parameter string value.
 e.g. rcvrs='nynn'      PSEL0=1, PSEL1=0  (matrix line 2)

 default is 1st rcvr active

 These bits are not modified for tune, unlike IN2SEL0 & IN2SEL1.
 
           Receivers (rcvrs='yny')
        1       2       3       4       PSEL0   PSEL1
    -----------------------------------------------------
  1     y       x       x       x       0       0
  2     n       y       x       x       1       0
  3     n       n       y       x       0       1
  4     n       n       n       y       1       1
*/
void setPSELviaMatrix()
{
     int fristActive;

     fristActive = getFirstActiveRcvr();
     switch(fristActive)
     {
	case 0:
       		SetAPBit(RF_PICrout, SET_FALSE, PIC_PSEL0, NULL);
       		SetAPBit(RF_PICrout, SET_FALSE, PIC_PSEL1, NULL);
		break;
	case 1:
       		SetAPBit(RF_PICrout, SET_TRUE, PIC_PSEL0, NULL);
       		SetAPBit(RF_PICrout, SET_FALSE, PIC_PSEL1, NULL);
		break;
	case 2:
       		SetAPBit(RF_PICrout, SET_FALSE, PIC_PSEL0, NULL);
       		SetAPBit(RF_PICrout, SET_TRUE, PIC_PSEL1, NULL);
		break;
	case 3:
       		SetAPBit(RF_PICrout, SET_TRUE, PIC_PSEL0, NULL);
       		SetAPBit(RF_PICrout, SET_TRUE, PIC_PSEL1, NULL);
		break;
	default:  /* default 1st rcvr as active */
       		SetAPBit(RF_PICrout, SET_FALSE, PIC_PSEL0, NULL);
       		SetAPBit(RF_PICrout, SET_FALSE, PIC_PSEL1, NULL);
		break;
     }
}

/*-------------------------------------------------------------------
|	set_preamp_mask()/0 
|            sets homo decoupling and preamp select.
+------------------------------------------------------------------*/
void set_preamp_mask()
{
   int	preamp_mask;

   /* Set Channels 1 and 2 for Broadband (special SIS case)	*/
   if (amptype[0] == 'b')
   {
      if ( (amptype[1] != 'b') && (amptype[1] != 'n') && (NUMch>=2))
      {
      text_error("SPM Warning: Chl 1 and 2 Amp Types are mixed. Check config.");
      }
      preamp_mask = 1;
      if (newacq)	/*receiver gate line not tied to amp blanking */
      	preamp_mask =  homo_decouple_routing();
      SetAPAttr(RF_TR_PA, SET_MASK, preamp_mask, NULL);

      return;
   }

   if ( rfchan_getampband(OBSch) )
      preamp_mask = 1;
   else
      preamp_mask = 2;
   if (newacq)		/*receiver gate line not tied to amp blanking */
   	preamp_mask = homo_decouple_routing();
   SetAPAttr(RF_TR_PA, SET_MASK, preamp_mask, NULL);
}

/*-------------------------------------------------------------------
|
|	set_ampcw()/0 
|	set Amp bands to Pulse or CW
|				Author Greg Brissey  1/26/90
+------------------------------------------------------------------*/
void set_ampcw()
{
    int error,error2;
    int obsamphiband;
    int amphiband1,amphiband2,amphiband3;
    int cw_bit,bothonband;

    /* Set all decoupler channels to CW, then set Observe channel to Pulse */
    /* Unused Channels are set to Pulse mode */
    if (NUMch > 2)  /* set 2nd Amp if present, base on having more than 2 channels */
    {
      if (OBSch != DO2DEV)
      {
        amphiband3 = rfchan_getampband(DO2DEV);
        if (amphiband3)
        {
          error = SetAPBit(RF_Opts2, SET_TRUE,HIGHBAND2_CW, NULL);
          error2 = SetAPBit(RF_Opts, SET_FALSE, LOWBAND2_CW, NULL);
        }
	else
        {
          error = SetAPBit(RF_Opts2, SET_FALSE,HIGHBAND2_CW, NULL);
          error2 = SetAPBit(RF_Opts, SET_TRUE, LOWBAND2_CW, NULL);
        }
      }
      else  /* if observe, both bands to pulse mode, OK to set, */
      {	    /*	 chan 1,2 do not effect the 2nd amp */
        error = SetAPBit(RF_Opts2, SET_FALSE,HIGHBAND2_CW, NULL);
        error2 = SetAPBit(RF_Opts, SET_FALSE, LOWBAND2_CW, NULL);
      }

      /* ---------- check for errors ------------- */
      if (error < 0)
      {
         abort_message("%s : %s\n",RF_Opts2->objname,ObjError(error));
      }
      if (error2 < 0)
      {
         abort_message("%s : %s\n",RF_Opts->objname,ObjError(error));
      }
    }

    /* set both amp bands to CW to start */
    error = SetAPBit(RF_Opts2, SET_TRUE,HIGHBAND_CW,
    		       SET_TRUE, LOWBAND_CW,
	 NULL);
    if (error < 0)
    {
       abort_message("%s : %s\n",RF_Opts2->objname,ObjError(error));
    }

    amphiband1 = rfchan_getampband(TODEV);
    amphiband2 = rfchan_getampband(DODEV);
    obsamphiband = ( ((OBSch == TODEV) || (OBSch == DODEV)) ?
			rfchan_getampband(OBSch) : NOT_OBSERVE );

    bothonband = ( (amphiband1 == amphiband2) ? amphiband1 : NOT_SAME_BAND );

    if (obsamphiband != NOT_OBSERVE)  /* Channel 1 or 2 is Observe */
    {
      cw_bit = (obsamphiband) ? HIGHBAND_CW : LOWBAND_CW;
      error = SetAPBit(RF_Opts2, SET_FALSE,cw_bit, NULL);
				/* set band to pulse mode */
      if (error < 0)
      {
         abort_message("%s : %s\n",RF_Opts2->objname,ObjError(error));
      }
    }

    /* check if one band not used and set to pulse */	
    if (bothonband != NOT_SAME_BAND)
    {
        cw_bit = (bothonband) ? LOWBAND_CW : HIGHBAND_CW;
        error = SetAPBit(RF_Opts2, SET_FALSE, cw_bit, NULL);
				/* set the other band to Pulse mode too! */
        if (error < 0)
        {
           abort_message("%s : %s\n",RF_Opts2->objname,ObjError(error));
        }
    }
}

/*---------------------------------------------------
+---------------------------------------------------*/
void set_atgbit()
{
int	band;
int	bit;

   band = rfchan_getampband(RFCHAN1);
   {  if (band || (amptype[RFCHAN1-1] == 'b'))
	 bit = HS_SEL4;
      else
         bit = HS_SEL5;
      SetRFChanAttr(RF_Channel[RFCHAN1],
		SET_HSSEL_TRUE,	bit,
		NULL);
   }
   band = rfchan_getampband(RFCHAN2);
   {  if (band && (amptype[RFCHAN2-1] != 'b'))
	 bit = HS_SEL4;
      else
         bit = HS_SEL5;
      SetRFChanAttr(RF_Channel[RFCHAN2],
		SET_HSSEL_TRUE,	bit,
		NULL);
   }
}


/*---------------------------------------------------
|  determine if RF channel is being by the following criteria
|  1. if base freq is one the channel is not used
|  2. if base freq > 1.0 and nucleus name is tn then always used
|  3. if base freq > 1.0 and nucleus name is dnX, and dnX is not null then  used.
|  Return:  0 - Not Being Used
|	    1 - Being Used but Not Observe Channel	(dn,dn2,dn3,etc..)
|	    2 - Being Used and Observe Channel		(tn)
+---------------------------------------------------*/
int rfchan_used(int channel)
{
   char *nucname;
   char  dnname[6], tmpstr[15];
   int used = 0;
   extern char *getnucname();
   extern double rfchan_getbasefreq();

   /* 1st if channel base freq is 1.0 , then dn, dn2, etc.. not defined therefore not used */
   if (rfchan_getbasefreq(channel) != 1.0)
   {
       /* next obtain the nucleus name,
          then find out if it is set to any thing
       */
       nucname = getnucname(channel);  /* e.g. tn, dn, dn2, dn3 */
       strcpy(dnname,nucname);
       /* printf("chan: %d, offset name: '%s'\n",channel,nucname); */
       if (strcmp(nucname,"tn") == 0)  /* if Observe it is used ! */
       {
          used = 2;
       }
       else
       {
         if ( P_getstring(CURRENT, dnname, tmpstr, 1, 9) >= 0)
	 {
	  /* printf(" %s = '%s'\n",dnname,tmpstr); */
          used = (tmpstr[0]!='\000');  /* if it's not blank then it's being used */
	 }
         else
	 {
          used = FALSE;  /* dnX doesn't exist then it's not used */
	 }
       }
   }
   else
   {
       used = FALSE;
   }
   return( used );
}


/****************************************************************/
/* This routine checks for exceptions for RF_hilo byte. If the 	*/
/* console has more than two channels, the three bits in	*/
/* RF_hilo must be set, AMT2_LL, AMT3_HL, AMT2_HL.		*/
/* These bits determine which channel each band in the		*/
/* amplifier follows for CW and blanking. Also the routing bits	*/
/* on the attenuator board may have to be reset, default drives	*/
/* high band to OUT3, low band to OUT4. Sometimes this has to	*/
/* go straight, i.e. high and/or  low channel3 to OUT3, high 	*/
/* and/or low channel4 to OUT4.	The set_ampbandrelay() routine	*/
/* assumes a single H/L band AMT on the first/second channel,	*/
/*  a single H/L band AMT on the third and fourth cahnnel.	*/
/* This then also serves a single Low band on the third/fourth	*/
/* channel. Anything else must be reset, and some warning msg	*/
/* might be in order.						*/
/*								*/
/* Finally the ATG must be selected correctly			*/
/*								*/
/* 'amptype' is of subtype "flag". It is enumerated:		*/
/*	"c"	Class C		Never on Hydra			*/
/*	"a"	Linear Full					*/
/*	"l"	Linear Low	If Only low band		*/
/*	"h"	Linear High	If only high band, not sold	*/
/*	"n"	None		If two channels share AMT	*/
/*	"b"	Linear Broad	SIS, no switching bands		*/
/* Most of the time the letters are decoded in pairs, for	*/
/* channel 1/2 (always "aa") for channel 3/4 they can be:	*/
/*     "ln"	One low band 	set_ampbandrelay ok		*/
/*     "ll"	Dual low band	go straight, unless dfrq2=dfrq3	*/
/*     "an"	Single H/L AMT	set_ampbandrelay ok		*/
/*     "aa"	Two H/L AMTs	go straight, set external realy */
/****************************************************************/

/* AMP_CW_GATED supports the special Amp and Route Board on the Minnesota CMRR system. */
#define AMP_CW_STD    0x88
#define AMP_CW_GATED  0xdd
#define AMP_PULSE 0x55
#define AMP_IDLE  0x0
#define SOL_CW    0xee
#define SOL_PULSE 0x21
#define MASK_AMP1 0x0f
#define MASK_AMP2 0xf0
#define MASK_AMP3 0x0f
#define MASK_AMP4 0xf0
#define AMP_NOT_USED -1
#define LOW_NONE 1		/* amptype = xxln */
#define HIGH_NONE 2     	/* amptype = xxhn */
#define LOW_LOW 3       	/* amptype = xxll */
#define HIGH_LOW 4      	/* amptype = xxan */
#define HIGH_LOW_HIGH_LOW 5	/* amptype = xxaa, 2nd and 3rd Amp chassie present, 750s */
#define BROADBAND 6		/* amptype = xxbb,  SISCO */
#define TRUE	1
#define FALSE	0
#define NOT_SET_YET -1

static void set_ampbits()
{
char	tmpstr[10];
char	ampmode[10];
int     amp1mode;
int     amp2mode;
int     ampoverride = 0;
int     solmode;
int     chan;
int 	amphiband[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int	ampsmode[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int	solispulse[2] = { NOT_SET_YET, NOT_SET_YET };  /* 1 is low-0/high-1 band Solids amp been set to pulse */
int	ampmask[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
int     Amp34Type = BROADBAND;
int     MaskAmp[5] = { 0, MASK_AMP1, MASK_AMP2, MASK_AMP3, MASK_AMP4 };
int     HiBandMask[5] = { 0, MASK_AMP1, MASK_AMP1, MASK_AMP3, MASK_AMP3 };
int     LowBandMask[5] = { 0, MASK_AMP2, MASK_AMP2, MASK_AMP4, MASK_AMP4 };
double	diff_basefrq;
int	amp_cw;                 /* AMP_CW_STD or AMP_CW_GATED */

extern double rfchan_getbasefreq();
extern int rfchan_getampband();
extern char *getfreqname();
extern char *getnucname();

/* -- use the settings of the 1st two physical channels
      which are connected to the 1st amp chassis, not the logical channels
      which was done prior to this.
*/

/*

1. setting ampfiler to Pulse, CW or Idle
2. Make sure amplifer is OK for Freq range (e.g. highband freq isn't using a lowband amp)

 Differs from old way
   a. chan 1 (Obs)  & 2  (Dec) are both in high band both are set to Pulse, new 1st Pulse, 2nd CW 
*/
   
   solmode = AMP_IDLE;	/* 0 */
   if (P_getstring(GLOBAL, "cwblanking", tmpstr, 1, sizeof(tmpstr)) == 0
       && *tmpstr == 'y')
   {
       amp_cw = AMP_CW_GATED;
   } else {
       amp_cw = AMP_CW_STD;
   }
   

/*
   fprintf(stderr,"\n");
   amptype[5] = '\0';
   fprintf(stderr,"Amptype: '%s'\n",amptype);
   fprintf(stderr,"\n");
*/

/* first the no-can-do cases */
/* This routine (and many others) don't */
   if ( (NUMch > 4) &&	(whatrftype(5) != LOCK_DECOUP_OFFSETSYN))
   {  text_error("set_ampbits(): No support for 'numrfch'>4");
      psg_abort(1);
   }

   /* determine configuration of 3rd & 4th channnel amp settings */
   if ( (amptype[RFCHAN3 - 1]=='l') && (amptype[RFCHAN4 - 1]=='n') )
	Amp34Type = LOW_NONE;
   else if ( (amptype[RFCHAN3 - 1]=='h') && (amptype[RFCHAN4 - 1]=='n') )
	Amp34Type = HIGH_NONE;
   else if ( (amptype[RFCHAN3 - 1]=='l') && (amptype[RFCHAN4 - 1]=='l') )
	Amp34Type = LOW_LOW;
   else if ( (amptype[RFCHAN3 - 1]=='a') && (amptype[RFCHAN4 - 1]=='n') )
	Amp34Type = HIGH_LOW;
   else if ( (amptype[RFCHAN3 - 1]=='a') && (amptype[RFCHAN4 - 1]=='a') )
	Amp34Type = HIGH_LOW_HIGH_LOW;
   else if ( (amptype[RFCHAN3 - 1]=='b') && (amptype[RFCHAN4 - 1]=='b') )	/* SIS */
	Amp34Type = BROADBAND;


   /* check all Channels for usage */
   for (chan = 1; chan <= 4; chan++)
   {  
      int chantype;

      if( chan <= NUMch)
      {
        if ((chantype = rfchan_used(chan)) != 0)
        {
           amphiband[chan] = rfchan_getampband(chan);

           if (bgflag)
           {
             fprintf(stderr,"set_ampbits: Channel - %d  Nuc: '%s'  Basefreq : %lf  RFBand: %s \n",
		chan,getnucname(chan),rfchan_getbasefreq(chan), ((amphiband[chan]) ? "HI" : "LOW "));
           }

         /* determine which Pulse/CW mask to use, chan 1 & 2 low band 0xf, hi band 0xf0,
						  chan 3 & 4 low band 0xf, hi band 0xf0
	    remember the channel may be hi/low band but the amp brick is specific
	    The High order nible controls the Low Band brick  (0x50 - pulse or 0x80 - CW)
	    the Low order nible controls the High Band brick  (0x05 - pulse or 0x08 - CW)
	    so if chan 1 is high have to set Hi band amp brick to Pulse 0x58  (hb-5, lb-8 (CW))
               if chan 1 is low have to set Low band amp brick to Pulse 0x85  (hb-8 (CW), lb-5 )

	    However, if amptype is of the form: xxll, xxaa or xxbb the usage of the 
  	    High & Low order nibles changes from Low/High Band brick specific to actual AMP
	    specific. For amptype = aall the 2nd & 3rd Amps Pulse/CW are control by the
	    High order nible and Low order nible respectivily.
         */

         ampmask[chan] = (amphiband[chan]) ? HiBandMask[chan] : LowBandMask[chan];
   	 if (amptype[chan-1] == 'b') ampmask[chan] = HiBandMask[chan];

	 /* 
	    if amptype = xxll, xxaa or xxbb then usage of ampbytes changes from low/high band
	    to 2nd/3rd AMP, i.e.  amp34mode = 0x88 - low/hi band CW, to 2nd/3rd Amp CW
	    which typically dedicated to the 3rd & 4th channel respectivily
         */
         if ( (chan == RFCHAN3) || (chan == RFCHAN4) )
         {
	    if ( (Amp34Type == LOW_LOW ) ||   			/* xxll */
		 (Amp34Type == HIGH_LOW_HIGH_LOW ) ||		/* xxaa */
		 (Amp34Type == BROADBAND ) 			/* xxbb */
  	       ) 
	    {
	      ampmask[RFCHAN3] = HiBandMask[RFCHAN3];  /* normally low band is now 3rd channel */
	      ampmask[RFCHAN4] = LowBandMask[RFCHAN4];  /* normally Hi band is now 4th channel */
	    }
         }

	 /* check the obvious error, freq is in range of amp band */
         /* however the varients of 3rd & 4th channel amptype  will be tested later */
         if ( ((amptype[chan-1] == 'l') && amphiband[chan]) ||
	      ((amptype[chan-1] == 'h') && (!amphiband[chan])) )
         {
            abort_message("'%s' is out of range for amplifier %d %s band",
		getfreqname(chan), ((chan<3)?1:2), ((amptype[chan-1] == 'l')?"low":"high"));
         }

	 /* Is 4th chan sharing low, but set to hi */
         /* Is 4th chan sharing Hi, but set to Low */
         if ((chan == RFCHAN4) && ( 
	     ((Amp34Type == LOW_NONE) && (amphiband[RFCHAN4] == TRUE)) ||  
             ((Amp34Type == HIGH_NONE) && (amphiband[RFCHAN4] == FALSE))  ) )
         {
            abort_message("'%s' is out of range for amplifier %d shared %s band",
		getfreqname(chan), ((chan<3)?1:2), ((Amp34Type == LOW_NONE)?"low":"high"));
         }


	 /* time to set Pulse/CW mode bits */
         if (chan == OBSch)
	 {
	     ampsmode[chan] = AMP_PULSE;
             if ((chan == RFCHAN1) || (chan == RFCHAN2))
             {
		 /* Since the bit patterns for Pulse/CW for Solids are not unique
		    we have to use this hacked up logic to be sure Pulse override CW */ 
		 if (solispulse[amphiband[chan]] == NOT_SET_YET)
		 {
                    solispulse[amphiband[chan]] = TRUE;
		    solmode |= SOL_PULSE & ampmask[chan];  /* SOL_PULSE & MASK_AMP1 */
		 }
		 else if (solispulse[amphiband[chan]] == 0) /* CW */
		 {
                    solispulse[amphiband[chan]] = TRUE;
		    solmode &= ~(ampmask[chan]);
		    solmode |= SOL_PULSE & ampmask[chan];  /* SOL_PULSE & MASK_AMP1 */
		 }
	     }
	 }
	 else
	 {
	     ampsmode[chan] = amp_cw;
             if ((chan == RFCHAN1) || (chan == RFCHAN2))
	     {
	        if ( solispulse[amphiband[chan]] != TRUE)
		{
                  solispulse[amphiband[chan]] = FALSE;
		  solmode |= SOL_CW & ampmask[chan];  /* SOL_CW & MASK_AMP1 */
		}
	     }
	 }
        }
        else   /* Amp not used */
        {
	 ampsmode[chan] = AMP_IDLE;
	 amphiband[chan] = AMP_NOT_USED; /* -1 */
   
         if (bgflag)
         {
            fprintf(stderr,"set_ampbits: Channel - %d, Unused \n",chan);
         }
        }
       }
       else  /* amp not present */
       {
	   ampsmode[chan] = AMP_IDLE;
	   amphiband[chan] = AMP_NOT_USED; /* -1 */
           if (bgflag)
           {
              fprintf(stderr,"set_ampbits: Channel - %d, Not Present\n",chan);
           }
	}

   }
   
   /* amp1mode crontol 1st amp low/hi band bricks */
   amp1mode = (ampsmode[RFCHAN1] & ampmask[RFCHAN1]) | (ampsmode[RFCHAN2] & ampmask[RFCHAN2]);

   /* if two amps going to same band amp then Pulse mode takes priority of CW */
   /* e.g. if 0xd0 or 0x0d the d shows both Pulse & CW has been selected */
   /* do high band amp,	            Pulse/CW both set  ?  mask out CW Bits  :  use present value  */
   amp1mode = ( (amp1mode & MaskAmp[RFCHAN1]) == 0x0d) ? (amp1mode & 0xf7 ) :  amp1mode;

   /* do low band amp,	            Pulse/CW both set  ?  mask out CW Bits  :  use present value  */
   amp1mode = ( (amp1mode & MaskAmp[RFCHAN2]) == 0xd0) ? (amp1mode & 0x7f ) :  amp1mode;

   /* If SISCO Broadband amp then set to Pulse mode , Period... */
   solmode = (amptype[RFCHAN1-1] == 'b') ? SOL_PULSE : solmode;

/*
   printf("NEW - ampmode1: 0x%x, ampmode2: 0x%x, ampmode12: 0x%x, solmode: 0x%x\n",
		ampsmode[RFCHAN1], ampsmode[RFCHAN2],amp1mode,solmode);
*/
   /* amp2mode controls the 2nd low/hi band bricks, or 2nd amp & 3rd amp */
   amp2mode = (ampsmode[RFCHAN3] & ampmask[RFCHAN3]) | (ampsmode[RFCHAN4] & ampmask[RFCHAN4]);

   /* if two amps going to same band amp then Pulse mode takes priority of CW */
   /* e.g. if 0xd0 or 0x0d the d shows both Pulse & CW has been selected */
   /* do high band amp */
   amp2mode = ( (amp2mode & MaskAmp[RFCHAN3]) == 0x0d) ? (amp2mode & 0xf7 ) :  amp2mode;
   amp2mode = ( (amp2mode & MaskAmp[RFCHAN4]) == 0xd0) ? (amp2mode & 0x7f ) :  amp2mode;

/*
   printf("NEW - ampmode3: 0x%x, ampmode4: 0x%x, ampmode34: 0x%x\n",
		ampsmode[RFCHAN3], ampsmode[RFCHAN4], amp2mode);
*/


   /* get amp override parameter, reset amps to overrides if any */
   if ( P_getstring(CURRENT, "ampmode", ampmode, 1, 9) >= 0)
   {
         ampoverride = strlen(ampmode);
   }
   for (chan=1; chan <= ampoverride; chan++)
   {
      switch(ampmode[chan-1])
      {
	case 'p':
         if (chan < 3)
	 {
           amp1mode = (amp1mode & ~(MaskAmp[chan])) |  (AMP_PULSE & MaskAmp[chan]);   /* channel to override mode */
           solmode = (solmode & ~(MaskAmp[chan])) |  (SOL_PULSE & MaskAmp[chan]);   /* channel to override mode */
	 }
	 else
           amp2mode = (amp2mode & ~(MaskAmp[chan])) |  (AMP_PULSE & MaskAmp[chan]);   /* channel to override mode */

	break;

	case 'c':
         if (chan < 3)
	 {
           amp1mode = (amp1mode & ~(MaskAmp[chan])) |  (amp_cw & MaskAmp[chan]);   /* channel to override mode */
           solmode = (solmode & ~(MaskAmp[chan])) |  (SOL_CW & MaskAmp[chan]);   /* channel to override mode */
	 }
	 else
           amp2mode = (amp2mode & ~(MaskAmp[chan])) |  (amp_cw & MaskAmp[chan]);   /* channel to override mode */

	break;

	case 'i':
         if (chan < 3)
	 {
           amp1mode = (amp1mode & ~(MaskAmp[chan])) |  (AMP_IDLE & MaskAmp[chan]);   /* channel to override mode */
           solmode = (solmode & ~(MaskAmp[chan])) |  (AMP_IDLE & MaskAmp[chan]);   /* channel to override mode */
	 }
	 else
           amp2mode = (amp2mode & ~(MaskAmp[chan])) |  (AMP_IDLE & MaskAmp[chan]);   /* channel to override mode */
	break;

      }
   }

 /*   printf("NDATA: 12 0x%x 34 0x%x sol 0x%x\n",amp1mode,amp2mode,solmode); */

/* Set cw vs. pulse for two amplifier bands, i.e. one chassis */
   SetAPAttr(RF_Amp1_2, SET_MASK, amp1mode, NULL);

 /* Solids Amp always on 1st/2nd amp, 1st chassie */
   SetAPAttr(RF_Amp_Sol, SET_MASK, solmode, NULL);


 /* Set cw vs. pulse for two amplifier bands, i.e. 2nd chassis */
   SetAPAttr(RF_Amp3_4, SET_MASK, amp2mode, NULL);

/*--------------------------------------------------------------*/
/* Set Channels 1 and 2 for Broadband	(special SIS case)	*/
/*--------------------------------------------------------------*/
   if (amptype[0] == 'b')
   {
      SetAPBit(RF_hilo,
		SET_FALSE,	HILO_CH1,	 /* Default High band */
		NULL);
   }
   if ( (amptype[1] == 'b') || (amptype[1] == 'n') || (NUMch<2) )
   {
      SetAPBit(RF_hilo,
		SET_TRUE,	HILO_CH2,	 /* Default Low band */
		NULL);
   }

   /* Only two channels then no need to go farther */
   if (NUMch < 3)
     return;
   
   if (NUMch < 4)	/* deal with simple case right here */
   {
         int	hiband,
		ch3_lb;

         /* if ((amphiband3) || (amptype[2] == 'b')) */
         if ((amphiband[RFCHAN3] == TRUE) || (amptype[RFCHAN3 - 1] == 'b'))
         { /* must use ATG-3 */
            SetRFChanAttr(RF_Channel[RFCHAN3],
	   	   SET_HSSEL_TRUE,	HS_SEL4,
	   	   SET_HSSEL_TRUE,	HS_SEL5,
		   NULL);
            SetAPBit(RF_MLrout,
		   SET_FALSE,	MAGLEG_RELAY_0,
		   SET_FALSE,	MAGLEG_RELAY_1,
		   NULL);
	    hiband = SET_TRUE;
	    ch3_lb = SET_FALSE;
         }
         else
         { /* must use ATG-4 */
            SetRFChanAttr(RF_Channel[RFCHAN3],
	   	   SET_HSSEL_TRUE,	HS_SEL6,
		   NULL);
            SetAPBit(RF_MLrout,
		   SET_TRUE,	MAGLEG_RELAY_0,
		   SET_TRUE,	MAGLEG_RELAY_1,
		   NULL);
	    hiband = SET_FALSE;
	    ch3_lb = SET_TRUE;
         }

         SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* must be 0  */
		hiband,		HILO_AMT2_HL,	/* LB = 0; HB = 1 */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		ch3_lb,		HILO_CH3,	/* HB = 0; LB = 1 */
		NULL);

      return;
   }



/* now we "only" have the 4 channel cases left */
   diff_basefrq = rfchan_getbasefreq(RFCHAN3) - rfchan_getbasefreq(RFCHAN4);

   switch(Amp34Type)
   {
      case LOW_NONE:
        /* hi/lo relay on attn brd ok if both freqs are < 245MHz, */
        /* but dfrq2 should equal dfrq3 				*/
        /* if ( (ix < 2) && getchan3 && getchan4 && */
        /* if channel is not used then amphiband[channel] is set to -1 */

        SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 3 must use ATG4 */
		NULL);
        SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 4 must use ATG4 */
		NULL);
        SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* must be 0  */
		SET_FALSE,	HILO_AMT2_HL,	/* must be 0  */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		NULL);

                /*
                   0x50 = amp3 is idle, amp4 follows ATG4
                   0x80 = amp3 is idle, amp4 is cw
                */
	break;

      case HIGH_NONE:   /* xxhn */

        SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 3 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
        SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 4 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
        SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* don't care */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1  */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		SET_FALSE,	HILO_CH3,	/* HB = 0     */
		SET_FALSE,	HILO_CH4,	/* HB = 0     */
		NULL);

                /*
                   0x50 = amp3 follows ATG3, amp4 is idle
                   0x80 = amp3 is cw, amp4 is idle
                */
	break;


      case LOW_LOW:	/* xxll */


        if ( (diff_basefrq > -1.0) && (diff_basefrq < 1.0) )
        {  
	   SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 3 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
           SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 4 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
           SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* don't care */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1  */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		SET_FALSE,	HILO_CH3,	/* chan3 goes straight  */
		SET_FALSE,	HILO_CH4,	/* chan4 goes to band 3 */
		NULL);

                   /*
                      0x05 = amp3 follows ATG3, amp4 is idle
                      0x08 = amp3 is cw, amp4 is idle
                   */
        }
        else
        {  
	   SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan3 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
           SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 4 must use ATG4 */
		NULL);
           SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* must be 0  */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1  */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		SET_FALSE,      HILO_CH3,	/* chan 3 goes straight */
		SET_TRUE,       HILO_CH4,	/* chan 4 goes straight */
		NULL);

                   /*
                      0x55 = amp3 follows ATG3, amp4 follows ATG4
                      0x88 = amp3 is cw, amp4 is cw
                   */
        }
	break;

      case HIGH_LOW:	/* amptype = xxan */

           /* even if not used (AMP_NOT_USED) need to set the following base on 
	      high/low band of channel 3 & 4 */

	   if ( amphiband[RFCHAN3] == AMP_NOT_USED) 
              amphiband[RFCHAN3] = rfchan_getampband(RFCHAN3);
	   if ( amphiband[RFCHAN4] == AMP_NOT_USED) 
              amphiband[RFCHAN4] = rfchan_getampband(RFCHAN4);


        /* if (!amphiband3 && !amphiband4)		both are low band */
        if ((amphiband[RFCHAN3] == FALSE) && (amphiband[RFCHAN4] == FALSE)) /* both are low band */
        {  SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 3 must use ATG4 */
		NULL);
           SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 4 must use ATG4 */
		NULL);
           SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* must be 0  */
		SET_FALSE,	HILO_AMT2_HL,	/* must be 0  */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		NULL);
           SetAPBit(RF_MLrout,
		SET_TRUE,	MAGLEG_RELAY_0,
		SET_TRUE,	MAGLEG_RELAY_1,
		NULL);

                   /*
                      0x50 = amp3 is idle, amp4 follows ATG4
                      0x80 = amp3 is idle, amp4 is cw
                   */

        }
      /* else if (amphiband3 && amphiband4)	 both are high band */
        else if ( (amphiband[RFCHAN3] == TRUE) && 
		  (amphiband[RFCHAN4]== TRUE)) /* both are high band */
        {  SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 3 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
           SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 4 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
           SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* don't care */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1  */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		SET_FALSE,	HILO_CH3,	/* HB = 0     */
		SET_FALSE,	HILO_CH4,	/* HB = 0     */
		NULL);
           SetAPBit(RF_MLrout,
		SET_FALSE,	MAGLEG_RELAY_0,
		SET_FALSE,	MAGLEG_RELAY_1,
		NULL);

                   /*
                      0x05 = amp3 follows ATG3, amp4 is idle
                      0x08 = amp3 is cw, amp4 is idle
                   */
        }
        else
        {
           /* if (amphiband3) */
           if (amphiband[RFCHAN3] == TRUE)
           {  
	      SetRFChanAttr(RF_Channel[RFCHAN3],
		   SET_HSSEL_TRUE,	HS_SEL4,  /* chan 3 must use ATG3 */
		   SET_HSSEL_TRUE,	HS_SEL5,
		   NULL);
              SetRFChanAttr(RF_Channel[RFCHAN4],
		   SET_HSSEL_TRUE,	HS_SEL6,  /* chan 4 must use ATG4 */
		   NULL);
              SetAPBit(RF_hilo,
		   SET_FALSE,	HILO_CH3,	  /* HB = 0 */
		   SET_TRUE,	HILO_CH4,	  /* LB = 1 */
		   NULL);
           }
           else	/* RFCHAN4 must be in highband */
           {  
	      SetRFChanAttr(RF_Channel[RFCHAN3],
		   SET_HSSEL_TRUE,	HS_SEL6,  /* chan 3 must use ATG4 */
		   NULL);
              SetRFChanAttr(RF_Channel[RFCHAN4],
		   SET_HSSEL_TRUE,	HS_SEL4,  /* chan 4 must use ATG3 */
		   SET_HSSEL_TRUE,	HS_SEL5,
		   NULL);
              SetAPBit(RF_hilo,
		   SET_TRUE,	HILO_CH3,	  /* LB = 1 */
		   SET_FALSE,	HILO_CH4,	  /* HB = 0 */
		   NULL);
           }

           SetAPBit(RF_hilo,
		   SET_FALSE,	HILO_AMT2_LL,	/* must be 0  */
		   SET_TRUE,	HILO_AMT2_HL,	/* must be 1  */
		   SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		   NULL);
        }
	break;


      case HIGH_LOW_HIGH_LOW:   /* amptype = xxaa */

           /* even if not used (AMP_NOT_USED) need to set the following base on 
	      high/low band of channel 3 & 4 */

	   if ( amphiband[RFCHAN3] == AMP_NOT_USED) 
              amphiband[RFCHAN3] = rfchan_getampband(RFCHAN3);
	   if ( amphiband[RFCHAN4] == AMP_NOT_USED) 
              amphiband[RFCHAN4] = rfchan_getampband(RFCHAN4);


        SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 3 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
        SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 4 must use ATG4 */
		NULL);

                        /*
                           0x08 now sets band of Amp3 to cw
                           0x80 now sets band of Amp4 to cw
                        */

        SetAPBit(RF_hilo,
		SET_FALSE,	HILO_CH3,	/* force straight */
		SET_TRUE,	HILO_CH4,	/* force straight */
		NULL);

        if ((amphiband[RFCHAN3] == TRUE) && 
	    (amphiband[RFCHAN4] == TRUE))	/* both 3 &  4 are high band */
        {  
	   SetAPBit(RF_hilo,
		SET_TRUE,	HILO_AMT2_LL,	/* must be 1 */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1 */
		SET_TRUE,	HILO_AMT3_HL,	/* must be 1 */
		NULL);
           SetAPBit(RF_MLrout,
		SET_FALSE,	MAGLEG_RELAY_0,
		SET_FALSE,	MAGLEG_RELAY_1,
		NULL);
        }
        else if ((amphiband[RFCHAN3] == FALSE) && 
		 (amphiband[RFCHAN4] == FALSE))/* both 3 &  4 are low band */
        {  
	   SetAPBit(RF_hilo,
		SET_TRUE,	HILO_AMT2_LL,	/* must be 1  */
		SET_FALSE,	HILO_AMT2_HL,	/* 0 prefered */
		SET_FALSE,	HILO_AMT3_HL,	/* must be 0  */
		NULL);
           SetAPBit(RF_MLrout,
		SET_TRUE,	MAGLEG_RELAY_0,
		SET_TRUE,	MAGLEG_RELAY_1,
		NULL);
        }
        else if ((amphiband[RFCHAN3] == TRUE) && 
		 (amphiband[RFCHAN4] == FALSE)) /* ch 3 is hi, ch 4 is low */
        {  
	   SetAPBit(RF_hilo,
		SET_TRUE,	HILO_AMT2_LL,	/* must be 1 */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1 */
		SET_FALSE,	HILO_AMT3_HL,	/* must be 0 */
		NULL);
           SetAPBit(RF_MLrout,
		SET_FALSE,	MAGLEG_RELAY_0,
		SET_TRUE,	MAGLEG_RELAY_1,
		NULL);
        }
        else if ((amphiband[RFCHAN3]== FALSE) && 
		 (amphiband[RFCHAN4] == TRUE)) /* ch 3 is low, ch 4 is hi */
        {  
	   SetAPBit(RF_hilo,
		SET_TRUE,	HILO_AMT2_LL,	/* must be 1  */
		SET_FALSE,	HILO_AMT2_HL,	/* 0 prefered */
		SET_TRUE,	HILO_AMT3_HL,	/* must be 1  */
		NULL);
           SetAPBit(RF_MLrout,
		SET_TRUE,	MAGLEG_RELAY_0,
		SET_FALSE,	MAGLEG_RELAY_1,
		NULL);
        }
	break;


      case BROADBAND:	/* amptype = xxbb, SIS */

        SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 3 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
        SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 4 must use ATG4 */
		NULL);

                        /*
                           0x08 now sets band of Amp3 to cw
                           0x80 now sets band of Amp4 to cw
                        */
  
        SetAPBit(RF_hilo,
		SET_FALSE,	HILO_CH3,	/* force straight */
		SET_TRUE,	HILO_CH4,	/* force straight */
		NULL);

      /* It is questionable if these need to be set, but will set */
      /* for Hi/Lo situation */
        SetAPBit(RF_hilo,
		SET_TRUE,	HILO_AMT2_LL,	/* must be 1 */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1 */
		SET_FALSE,	HILO_AMT3_HL,	/* must be 0 */
		NULL);

        SetAPBit(RF_MLrout,
		SET_FALSE,	MAGLEG_RELAY_0,
		SET_TRUE,	MAGLEG_RELAY_1,
		NULL);
	break;

      default:
          text_error("invalid amplifier configuration for 4 RF channels");
          psg_abort(1);
	  break;
    }
}

/* leave it just for a while for reference */
#ifdef SET_AMP_OLDWAY  
set_ampbitsold()
{
char	tmpstr[10];
int	amphiband1;
int	amphiband2;
int	amphiband3;
int	amphiband4;
char	ampmode[10];
int     amp1mode;
int     amp2mode;
int     amp3mode;
int     amp4mode;
int     ampoverride = 0;
int     solmode;
int	getchan3, getchan4;
int	mask;
int     chan;
double	diff_basefrq;
int	amp_cw;                 /* AMP_CW_STD or AMP_CW_GATED */

extern double rfchan_getbasefreq();
extern int rfchan_getampband();
extern char *getfreqname();
extern char *getnucname();

/* -- use the settings of the 1st two physical channels
      which are connected to the 1st amp chassis, not the logical channels
      which was done prior to this.
*/
/*   wrong way 
   amphiband1 = rfchan_getampband(OBSch);
   amphiband2 = rfchan_getampband(DECch);
   printf("rfchan_getbasefreq((OBSch[%d]): %lf\n",OBSch,rfchan_getbasefreq(OBSch));
   printf("rfchan_getbasefreq(DECch[%d]): %lf\n",DECch,rfchan_getbasefreq(DECch));
   printf("OBSch[%d]: High Band: %d, DECch[%d]: High Band: %d\n",
    OBSch,amphiband1, DECch,amphiband2);
*/

   if (P_getstring(GLOBAL, "cwblanking", tmpstr, 1, sizeof(tmpstr)) == 0
       && *tmpstr == 'y')
   {
       amp_cw = AMP_CW_GATED;
   } else {
       amp_cw = AMP_CW_STD;
   }

   amphiband1 = rfchan_getampband(RFCHAN1);
   amphiband2 = rfchan_getampband(RFCHAN2);
   if (bgflag)
   {
       fprintf(stderr,"set_ampbits: Basefreq Pyshical RFCHAN1: %lf\n",rfchan_getbasefreq(RFCHAN1));
       fprintf(stderr,"set_ampbits: basefreq Pyshical RFCHAN2: %lf\n",rfchan_getbasefreq(RFCHAN2));
       fprintf(stderr,"set_ampbits: Channel 1: High Band: '%s', Channel 2: High Band: '%s'\n",
    		((amphiband1) ? "TRUE" : "FALSE"),((amphiband2) ? "TRUE" : "FALSE"));
   }

   if ((amphiband1==amphiband2) || (amptype[RFCHAN1-1] == 'b') )
   {
      amp1mode = AMP_PULSE;
      amp2mode = AMP_PULSE;
      solmode = (SOL_PULSE & MASK_AMP1) | (SOL_PULSE & MASK_AMP2);
   }
   else
   {  if (amphiband1)
      {
         amp1mode = AMP_PULSE;
         amp2mode = amp_cw;
         solmode = (SOL_PULSE & MASK_AMP1) | (SOL_CW & MASK_AMP2);
      }
      else
      {
         amp1mode = amp_cw;
         amp2mode = AMP_PULSE;
         solmode = (SOL_CW & MASK_AMP1) | (SOL_PULSE & MASK_AMP2);
      }
   }
/*
   printf("OLD - ampmode1: 0x%x, ampmode2: 0x%x, ampmode12: 0x%x, solmode: 0x%x\n",
		amp1mode, amp2mode,
                (amp1mode & MASK_AMP1) | (amp2mode & MASK_AMP2),
		solmode);
*/

   if ( P_getstring(CURRENT, "ampmode", ampmode, 1, 9) >= 0)
   {
         ampoverride = strlen(ampmode);
   }
   if (ampoverride)
   {
      if (ampmode[0] == 'p')
      {
         amp1mode = AMP_PULSE;
         solmode = (SOL_PULSE & MASK_AMP1) | (solmode & MASK_AMP2);
      }
      else if (ampmode[0] == 'c')
      {
         amp1mode = amp_cw;
         solmode = (SOL_CW & MASK_AMP1) | (solmode & MASK_AMP2);
      }
      else if (ampmode[0] == 'i')
      {
         amp1mode = AMP_IDLE;
      }
      if (ampoverride > 1)
      {
         if (ampmode[1] == 'p')
         {
            amp2mode = AMP_PULSE;
            solmode = (solmode & MASK_AMP1) | (SOL_PULSE & MASK_AMP2);
         }
         else if (ampmode[1] == 'c')
         {
            amp2mode = amp_cw;
            solmode = (solmode & MASK_AMP1) | (SOL_CW & MASK_AMP2);
         }
         else if (ampmode[1] == 'i')
         {
            amp2mode = AMP_IDLE;
         }
      }
   }
/*
   printf("Ovr - ampmode1: 0x%x, ampmode2: 0x%x, ampmode12: 0x%x, solmode: 0x%x\n",
		amp1mode, amp2mode,
                (amp1mode & MASK_AMP1) | (amp2mode & MASK_AMP2),
		solmode);
*/

/* Set cw vs. pulse for two amplifier bands, i.e. one chassis */
   SetAPAttr(RF_Amp1_2, SET_MASK,	
             (amp1mode & MASK_AMP1) | (amp2mode & MASK_AMP2)
              , NULL);
   SetAPAttr(RF_Amp_Sol, SET_MASK,	solmode, NULL);

/* first the no-can-do cases */
   if (NUMch > 4)	/* This routine (and many others) don't */
   {  text_error("set_ampbits(): No support for 'numrfch'>4");
      psg_abort(1);
   }

/*--------------------------------------------------------------*/
/* Set Channels 1 and 2 for Broadband	(special SIS case)	*/
/*--------------------------------------------------------------*/
   if (amptype[0] == 'b')
   {
      SetAPBit(RF_hilo,
		SET_FALSE,	HILO_CH1,	 /* Default High band */
		NULL);
   }
   if ( (amptype[1] == 'b') || (amptype[1] == 'n') || (NUMch<2) )
   {
      SetAPBit(RF_hilo,
		SET_TRUE,	HILO_CH2,	 /* Default Low band */
		NULL);
   }

      
/* if we only have (or want to use) two channels, set amp 3/4 to idle */
/*
   if ( P_getstring(CURRENT, "dn2", tmpstr, 1, 9) >= 0)
      getchan3 = (tmpstr[0]!='\000');
   else
      getchan3 = FALSE;
   if ( P_getstring(CURRENT, "dn3", tmpstr, 1, 9) >= 0)
      getchan4 = (tmpstr[0]!='\000');
   else
      getchan4 = FALSE;
*/
   /* if base freq (dfrq,dfrq2,etc) is 1.0 then channel is being used (cps.c) */
   if (NUMch >= 3)
   {
      if (bgflag)
         fprintf(stderr,"set_ampbits: Basefreq Pyshical RFCHAN3: %lf\n",rfchan_getbasefreq(RFCHAN3));
      if (rfchan_getbasefreq(RFCHAN3) != 1.0) 
         getchan3 = TRUE;
      else
         getchan3 = FALSE;
   }
   else
     getchan3 = FALSE;

   if (NUMch >= 4)
   {
      if (bgflag)
         fprintf(stderr,"set_ampbits: Basefreq Pyshical RFCHAN4: %lf\n",rfchan_getbasefreq(RFCHAN4));
      if (rfchan_getbasefreq(RFCHAN4) != 1.0) 
         getchan4 = TRUE;
      else
         getchan4 = FALSE;
   }
   else
     getchan4 = FALSE;

   if (NUMch<3 || (!getchan3 && !getchan4) ) /* deal with trivial case  here */
   {  SetAPAttr(RF_Amp3_4,
		SET_MASK,	AMP_IDLE,	/* set amp 3/4 to idle */
 		NULL);
      return;
   }

/*
   If we have 3 channels, all types of a single-chassis amplifier
   must be supported, e.g., 'l', 'h', and 'a'.
*/

   amphiband3 = rfchan_getampband(RFCHAN3);
   if (bgflag)
       fprintf(stderr,"set_ampbits: Channel 3: High Band: '%s'\n",
    		((amphiband3) ? "TRUE" : "FALSE"));

   if (NUMch < 4)	/* deal with simple case right here */
   {
      if ( ((amptype[2] == 'l') && amphiband3) ||
	   ((amptype[2] == 'h') && (!amphiband3)) )
      {
         text_error("dfrq2 is out of range for 3rd amplifier band");
         psg_abort(1);
      }
      else
      {  
         int	hiband,
		ch3_lb;

         if ((amphiband3) || (amptype[2] == 'b'))
         { /* must use ATG-3 */
            SetRFChanAttr(RF_Channel[RFCHAN3],
	   	   SET_HSSEL_TRUE,	HS_SEL4,
	   	   SET_HSSEL_TRUE,	HS_SEL5,
		   NULL);
            SetAPBit(RF_MLrout,
		   SET_FALSE,	MAGLEG_RELAY_0,
		   SET_FALSE,	MAGLEG_RELAY_1,
		   NULL);
	    hiband = SET_TRUE;
	    ch3_lb = SET_FALSE;
         }
         else
         { /* must use ATG-4 */
            SetRFChanAttr(RF_Channel[RFCHAN3],
	   	   SET_HSSEL_TRUE,	HS_SEL6,
		   NULL);
            SetAPBit(RF_MLrout,
		   SET_TRUE,	MAGLEG_RELAY_0,
		   SET_TRUE,	MAGLEG_RELAY_1,
		   NULL);
	    hiband = SET_FALSE;
	    ch3_lb = SET_TRUE;
         }

         SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* must be 0  */
		hiband,		HILO_AMT2_HL,	/* LB = 0; HB = 1 */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		ch3_lb,		HILO_CH3,	/* HB = 0; LB = 1 */
		NULL);

         mask = (amphiband3) ? MASK_AMP3 : MASK_AMP4;
         amp3mode = (amphiband3 == amphiband1) ? AMP_PULSE : amp_cw;
	 if (!getchan3)
            amp3mode = AMP_IDLE;
/*
         printf("OLD - ampmode3: 0x%x, ampmode4: Not Set, ampmode34: 0x%x \n",
		amp3mode, (amp3mode & mask));
*/
         if (ampoverride >= 3)
         {
            if (ampmode[2] == 'p')
               amp3mode = AMP_PULSE;
            else if (ampmode[2] == 'c')
               amp3mode = amp_cw;
            else if (ampmode[2] == 'i')
               amp3mode = AMP_IDLE;
         }
         SetAPAttr(RF_Amp3_4,
		SET_MASK,	(amp3mode & mask),
		NULL);
/*
         printf("OLD after Override - ampmode3: 0x%x, ampmode4: Not Set, ampmode34: 0x%x \n",
		amp3mode, (amp3mode & mask));
*/

	 
      }

      printf("\nODATA: 12 0x%x 34 0x%x sol 0x%x\n",
	(amp1mode & MASK_AMP1) | (amp2mode & MASK_AMP2),(amp3mode & mask),solmode);

      return;
   }

/* now we "only" have the 4 channel cases left */
   diff_basefrq = rfchan_getbasefreq(RFCHAN3) - rfchan_getbasefreq(RFCHAN4);
   amphiband4 = rfchan_getampband(RFCHAN4);
   if (bgflag)
       fprintf(stderr,"set_ampbits: Channel 4: High Band: '%s'\n",
    		((amphiband4) ? "TRUE" : "FALSE"));

   if ( (amptype[2]=='l') && (amptype[3]=='n') )
   {  /* hi/lo relay on attn brd ok if both freqs are < 245MHz, */
      /* but dfrq2 should equal dfrq3 				*/
      if (amphiband4 || amphiband3)
      {  text_error("dfrq2 or dfrq3 is out of range for 3rd amplifier band");
         psg_abort(1);
      }

      SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 3 must use ATG4 */
		NULL);
      SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 4 must use ATG4 */
		NULL);
      SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* must be 0  */
		SET_FALSE,	HILO_AMT2_HL,	/* must be 0  */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		NULL);
      amp3mode = AMP_IDLE;
      amp4mode = ( (amphiband3 == amphiband1) ? AMP_PULSE : amp_cw );
                /*
                   0x50 = amp3 is idle, amp4 follows ATG4
                   0x80 = amp3 is idle, amp4 is cw
                */
   }
   else if ( (amptype[2]=='h') && (amptype[3]=='n') )
   {
      if ( (!amphiband4) || (!amphiband3) )
      {  text_error("dfrq2 or dfrq3 is out of range for 3rd amplifier band");
         psg_abort(1);
      }

      SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 3 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
      SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 4 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
      SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* don't care */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1  */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		SET_FALSE,	HILO_CH3,	/* HB = 0     */
		SET_FALSE,	HILO_CH4,	/* HB = 0     */
		NULL);

      amp3mode = ( (amphiband3 == amphiband1) ? AMP_PULSE : amp_cw );
      amp4mode = AMP_IDLE;
                /*
                   0x50 = amp3 follows ATG3, amp4 is idle
                   0x80 = amp3 is cw, amp4 is idle
                */
   }
   else if ( (amptype[2]=='l') && (amptype[3]=='l') )
   {  if ( amphiband3 || amphiband4 )
      {  text_error("dfrq2 and/or dfrq3 is out of range of amplifier band(s)");
         psg_abort(1);
      }
      if ( (diff_basefrq > -1.0) && (diff_basefrq < 1.0) )
      {  SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 3 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
         SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 4 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
         SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* don't care */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1  */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		SET_FALSE,	HILO_CH3,	/* chan3 goes straight  */
		SET_FALSE,	HILO_CH4,	/* chan4 goes to band 3 */
		NULL);

         amp3mode = ( (amphiband3 == amphiband1) ? AMP_PULSE : amp_cw );
         amp4mode = AMP_IDLE;
                   /*
                      0x05 = amp3 follows ATG3, amp4 is idle
                      0x08 = amp3 is cw, amp4 is idle
                   */
      }
      else
      {  SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan3 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
         SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 4 must use ATG4 */
		NULL);
         SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* must be 0  */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1  */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		SET_FALSE,      HILO_CH3,	/* chan 3 goes straight */
		SET_TRUE,       HILO_CH4,	/* chan 4 goes straight */
		NULL);

         amp3mode = ( (amphiband3 == amphiband1) ? AMP_PULSE : amp_cw );
         amp4mode = ( (amphiband3 == amphiband1) ? AMP_PULSE : amp_cw );
                   /*
                      0x55 = amp3 follows ATG3, amp4 follows ATG4
                      0x88 = amp3 is cw, amp4 is cw
                   */
	if (!getchan3)
           amp3mode = AMP_IDLE;
	if (!getchan4)
           amp4mode = AMP_IDLE;
      }
   }
   else if ( (amptype[2]=='a') && (amptype[3]=='n') )
   {  
      if (!amphiband3 && !amphiband4)		/* both are low band */
      {  SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 3 must use ATG4 */
		NULL);
         SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 4 must use ATG4 */
		NULL);
         SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* must be 0  */
		SET_FALSE,	HILO_AMT2_HL,	/* must be 0  */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		NULL);
         SetAPBit(RF_MLrout,
		SET_TRUE,	MAGLEG_RELAY_0,
		SET_TRUE,	MAGLEG_RELAY_1,
		NULL);

         amp3mode = AMP_IDLE;
         amp4mode = ( (amphiband3 == amphiband1) ? AMP_PULSE : amp_cw );
                   /*
                      0x50 = amp3 is idle, amp4 follows ATG4
                      0x80 = amp3 is idle, amp4 is cw
                   */

      }
      else if (amphiband3 && amphiband4)	/* both are high band */
      {  SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 3 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
         SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 4 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
         SetAPBit(RF_hilo,
		SET_FALSE,	HILO_AMT2_LL,	/* don't care */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1  */
		SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		SET_FALSE,	HILO_CH3,	/* HB = 0     */
		SET_FALSE,	HILO_CH4,	/* HB = 0     */
		NULL);
         SetAPBit(RF_MLrout,
		SET_FALSE,	MAGLEG_RELAY_0,
		SET_FALSE,	MAGLEG_RELAY_1,
		NULL);

         if (amphiband3)
         {
            amp3mode = ( (amphiband3 == amphiband1) ? AMP_PULSE : amp_cw );
            amp4mode = AMP_IDLE;
         }
         else
         {
            amp3mode = AMP_IDLE;
            amp4mode = ( (amphiband3 == amphiband1) ? AMP_PULSE : amp_cw );
         }
                   /*
                      0x05 = amp3 follows ATG3, amp4 is idle
                      0x08 = amp3 is cw, amp4 is idle
                   */
      }
      else
      {
         if (amphiband3)
         {  SetRFChanAttr(RF_Channel[RFCHAN3],
		   SET_HSSEL_TRUE,	HS_SEL4,  /* chan 3 must use ATG3 */
		   SET_HSSEL_TRUE,	HS_SEL5,
		   NULL);
            SetRFChanAttr(RF_Channel[RFCHAN4],
		   SET_HSSEL_TRUE,	HS_SEL6,  /* chan 4 must use ATG4 */
		   NULL);
            SetAPBit(RF_hilo,
		   SET_FALSE,	HILO_CH3,	  /* HB = 0 */
		   SET_TRUE,	HILO_CH4,	  /* LB = 1 */
		   NULL);

            amp3mode = ( (amphiband3 == amphiband1) ? AMP_PULSE : amp_cw );
            amp4mode = ( (amphiband3 == amphiband1) ? amp_cw : AMP_PULSE );
	    if (!getchan3)
               amp3mode = AMP_IDLE;
	    if (!getchan4)
               amp4mode = AMP_IDLE;
         }
         else
         {  SetRFChanAttr(RF_Channel[RFCHAN3],
		   SET_HSSEL_TRUE,	HS_SEL6,  /* chan 3 must use ATG4 */
		   NULL);
            SetRFChanAttr(RF_Channel[RFCHAN4],
		   SET_HSSEL_TRUE,	HS_SEL4,  /* chan 4 must use ATG3 */
		   SET_HSSEL_TRUE,	HS_SEL5,
		   NULL);
            SetAPBit(RF_hilo,
		   SET_TRUE,	HILO_CH3,	  /* LB = 1 */
		   SET_FALSE,	HILO_CH4,	  /* HB = 0 */
		   NULL);

            amp3mode = ( (amphiband3 == amphiband1) ? amp_cw : AMP_PULSE );
            amp4mode = ( (amphiband3 == amphiband1) ? AMP_PULSE : amp_cw );
	    if (!getchan3)
               amp3mode = AMP_IDLE;
	    if (!getchan4)
               amp4mode = AMP_IDLE;
         }

         SetAPBit(RF_hilo,
		   SET_FALSE,	HILO_AMT2_LL,	/* must be 0  */
		   SET_TRUE,	HILO_AMT2_HL,	/* must be 1  */
		   SET_FALSE,	HILO_AMT3_HL,	/* don't care */
		   NULL);
      }
   }
   else if ( (amptype[2]=='a') && (amptype[3]=='a') )
   {
      SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 3 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
      SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 4 must use ATG4 */
		NULL);

      amp3mode = ( (amphiband3 == amphiband1) ? AMP_PULSE : amp_cw );
      amp4mode = ( (amphiband4 == amphiband1) ? AMP_PULSE : amp_cw );
                        /*
                           0x08 now sets band of Amp3 to cw
                           0x80 now sets band of Amp4 to cw
                        */
      if (!getchan3)
         amp3mode = AMP_IDLE;
      if (!getchan4)
         amp4mode = AMP_IDLE;

      SetAPBit(RF_hilo,
		SET_FALSE,	HILO_CH3,	/* force straight */
		SET_TRUE,	HILO_CH4,	/* force straight */
		NULL);
      if (amphiband3 && amphiband4)		/* both 3 &  4 are high band */
      {  SetAPBit(RF_hilo,
		SET_TRUE,	HILO_AMT2_LL,	/* must be 1 */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1 */
		SET_TRUE,	HILO_AMT3_HL,	/* must be 1 */
		NULL);
         SetAPBit(RF_MLrout,
		SET_FALSE,	MAGLEG_RELAY_0,
		SET_FALSE,	MAGLEG_RELAY_1,
		NULL);
      }
      else if (!amphiband3 && !amphiband4)	/* both 3 &  4 are low band */
      {  SetAPBit(RF_hilo,
		SET_TRUE,	HILO_AMT2_LL,	/* must be 1  */
		SET_FALSE,	HILO_AMT2_HL,	/* 0 prefered */
		SET_FALSE,	HILO_AMT3_HL,	/* must be 0  */
		NULL);
         SetAPBit(RF_MLrout,
		SET_TRUE,	MAGLEG_RELAY_0,
		SET_TRUE,	MAGLEG_RELAY_1,
		NULL);
      }
      else if (amphiband3 && !amphiband4)	/* ch 3 is hi, ch 4 is low */
      {  SetAPBit(RF_hilo,
		SET_TRUE,	HILO_AMT2_LL,	/* must be 1 */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1 */
		SET_FALSE,	HILO_AMT3_HL,	/* must be 0 */
		NULL);
         SetAPBit(RF_MLrout,
		SET_FALSE,	MAGLEG_RELAY_0,
		SET_TRUE,	MAGLEG_RELAY_1,
		NULL);
      }
      else if (!amphiband3 && amphiband4)	/* ch 3 is low, ch 4 is hi */
      {  SetAPBit(RF_hilo,
		SET_TRUE,	HILO_AMT2_LL,	/* must be 1  */
		SET_FALSE,	HILO_AMT2_HL,	/* 0 prefered */
		SET_TRUE,	HILO_AMT3_HL,	/* must be 1  */
		NULL);
         SetAPBit(RF_MLrout,
		SET_TRUE,	MAGLEG_RELAY_0,
		SET_FALSE,	MAGLEG_RELAY_1,
		NULL);
      }
   }
   else if ( (amptype[2]=='b') && (amptype[3]=='b') )	/* SIS */
   {
      SetRFChanAttr(RF_Channel[RFCHAN3],
		SET_HSSEL_TRUE,	HS_SEL4,	/* chan 3 must use ATG3 */
		SET_HSSEL_TRUE,	HS_SEL5,
		NULL);
      SetRFChanAttr(RF_Channel[RFCHAN4],
		SET_HSSEL_TRUE,	HS_SEL6,	/* chan 4 must use ATG4 */
		NULL);

      amp3mode = ( (amphiband3 == amphiband1) ? AMP_PULSE : amp_cw );
      amp4mode = ( (amphiband4 == amphiband1) ? AMP_PULSE : amp_cw );
                        /*
                           0x08 now sets band of Amp3 to cw
                           0x80 now sets band of Amp4 to cw
                        */
      if (!getchan3)
         amp3mode = AMP_IDLE;
      if (!getchan4)
         amp4mode = AMP_IDLE;

      SetAPBit(RF_hilo,
		SET_FALSE,	HILO_CH3,	/* force straight */
		SET_TRUE,	HILO_CH4,	/* force straight */
		NULL);
      /* It is questionable if these need to be set, but will set */
      /* for Hi/Lo situation */
      SetAPBit(RF_hilo,
		SET_TRUE,	HILO_AMT2_LL,	/* must be 1 */
		SET_TRUE,	HILO_AMT2_HL,	/* must be 1 */
		SET_FALSE,	HILO_AMT3_HL,	/* must be 0 */
		NULL);
      SetAPBit(RF_MLrout,
		SET_FALSE,	MAGLEG_RELAY_0,
		SET_TRUE,	MAGLEG_RELAY_1,
		NULL);
   }
   else
   {
      text_error("invalid amplifier configuration for 4 RF channels");
      psg_abort(1);
   }

/*
   printf("OLD - ampmode3: 0x%x, ampmode4: 0x%x, ampmode34: 0x%x\n",
		amp3mode, amp4mode,((amp3mode & MASK_AMP3) | (amp4mode & MASK_AMP4)));
*/

/* Set amplifier Pulse/CW mode for four channels */
   if (ampoverride >= 3)
   {
      if (ampmode[2] == 'p')
         amp3mode = AMP_PULSE;
      else if (ampmode[2] == 'c')
         amp3mode = amp_cw;
      else if (ampmode[2] == 'i')
         amp3mode = AMP_IDLE;
   }
   if (ampoverride >= 4)
   {
      if (ampmode[3] == 'p')
         amp4mode = AMP_PULSE;
      else if (ampmode[3] == 'c')
         amp4mode = amp_cw;
      else if (ampmode[3] == 'i')
         amp4mode = AMP_IDLE;
   }
   SetAPAttr(RF_Amp3_4,
		SET_MASK,   ( (amp3mode & MASK_AMP3) | (amp4mode & MASK_AMP4) ),
		NULL);
/*
   printf("Ovr - ampmode3: 0x%x, ampmode4: 0x%x, ampmode34: 0x%x\n",
		amp3mode, amp4mode,((amp3mode & MASK_AMP3) | (amp4mode & MASK_AMP4)));
*/

   printf("\nODATA: 12 0x%x 34 0x%x sol 0x%x\n",
	(amp1mode & MASK_AMP1) | (amp2mode & MASK_AMP2),
	(amp3mode & MASK_AMP3) | (amp4mode & MASK_AMP4), solmode);
   fprintf(stderr,"\n");

   return;
}
#endif   /*  SET_AMP_OLDWAY   */

/* This program has been moved to lockfreqfunc.c, SCCS category VNMR */

#if 0
#define COMMENTCHAR '#'
#define LINEFEEDCHAR '\n'

int lockfreqtab_read(lkfilename,h1freq,synthif,lksense,lockref)
char *lkfilename;
int h1freq;
double *synthif,*lockref;
char *lksense;
{
 FILE *fd;
 char parse_me[MAXSTR];
 int stat,h1freq_file,eof_flag;
 double tmp_synthif,tmp_lockref;

    fd = fopen(lkfilename,"r");
    if (fd == NULL) 
    	return(-1);

    do
    {
       do				/* until not a comment or blank*/
       {
         stat=getlineFIO(fd,parse_me,MAXSTR,&eof_flag);
       }
       while ( ((parse_me[0] == COMMENTCHAR) || (parse_me[0] == LINEFEEDCHAR)) 	
							    && (stat != EOF) );
       if (stat != EOF) 
       {
           /* now it is not a comment so attempt to parse it */
          stat = sscanf(parse_me,"%d  %lf  %1s  %lf", &h1freq_file,
		&tmp_synthif, lksense, &tmp_lockref);
          if (stat != 4)
          {
		text_error("PSG: Error in lockfreqtab file.");
		fclose(fd); 
		psg_abort(1);
          }
	  if ((h1freq == h1freq_file) && 
			((lksense[0] == '+') || (lksense[0] == '-')))
	  {
		*synthif = tmp_synthif;
		*lockref = tmp_lockref;
   		fclose(fd); 
		return(0);
	  }
       }
    }
    while (stat != EOF);

    return(-1);
}
#endif
