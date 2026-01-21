/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/**********************************************************************
*
*   W A R N I N G, linked into both Vnmr and acqi
*
**********************************************************************/

/*  All Frequency functions are in this file  */
/*-------------------------------------------------------------
| freq_device.c - All Frequency functions are in this file 
+-------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

#include "rfconst.h"
#include "acodes.h"
#include "oopc.h"

#define UNDERRANGE 1
#define OVERRANGE  2
#define RANGEOFF   3
#define TXMIXSELFRQ 150.0e6     /* Tx mix select switchover for NMR freq 150 Mhz */

extern int  H1freq;		/* Proton Freq. of instrument 200,300,400,500 */
extern char systemdir[];	/* for lock/decoupler hardware setting */

extern int      bgflag;

#ifdef DEBUG
#define DPRINT(level, str) \
        if (bgflag >= level) fprintf(stdout,str)
#define DPRINT1(level, str, arg1) \
        if (bgflag >= level) fprintf(stdout,str,arg1)
#define DPRINT2(level, str, arg1, arg2) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2)
#define DPRINT3(level, str, arg1, arg2, arg3) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2,arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2,arg3,arg4)
#else 
#define DPRINT(level, str)
#define DPRINT1(level, str, arg2)
#define DPRINT2(level, str, arg1, arg2)
#define DPRINT3(level, str, arg1, arg2, arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4)
#endif 

extern char *ObjError(int wcode);
extern char *ObjCmd(int wcode);
extern void abort_message(const char *format, ...) __attribute__((format(printf,1,2),noreturn));
extern void text_error(const char *format, ...) __attribute__((format(printf,1,2)));
extern int AP_Device();
extern double round_freq(double baseMHz, double offsetHz,
                  double init_offsetHz, double stepsizeHz);
extern int Device(void *this, Message msg, void *param, void *result);
extern void putcode(c68int arg);
extern void putgtab(int table, c68int  word);
extern int mapRF(int index );
extern int is_psg4acqi();
extern int ClearTable(void *ptr, size_t tablesize);
extern void tune_from_freq_obj(void *FreqObj, int dev_channel );
extern int validate_imaging_config(char *callname);
extern void formXLwords();
extern void formPTSwords();
extern int lockfreqtab_read(char *lkfilename, int h1freq, double *synthif,
                     char *lksense, double *lockref);

#define DPRTLEVEL 1

typedef struct
{
#include "freq_device.p"
}               Freq_Object;

/* base class dispatcher */
#define Base(this,msg,par,res)    AP_Device(this,msg,par,res)

extern int      curfifocount;
static int init_attr(Msg_New_Result *result);
static int get_attr(Freq_Object *this, Msg_Set_Param *param, Msg_Set_Result *result);
static int set_attr(Freq_Object *this, Msg_Set_Param *param, Msg_Set_Result *result);
static int select_rfband(int rfband, double freq, double max_lowband_frq);
static void calc_nondbl_OffsetStorage(Freq_Object *device, int spare);
static void calc_nondbl_Offset(Freq_Object *device);
static void set_nondbl_freq_fromstorage(Freq_Object *device, int spare);
static void calcdirectOffset(Freq_Object *device);
static int PTSrange(Freq_Object *device);
static void calcoffsetsyn(Freq_Object *device);
static void setPTS(double ptsvalue, Freq_Object *device);
static void calcfixedoffset(Freq_Object *device);
static int initswpfreq(Freq_Object *device);
static int incrswpfreq(Freq_Object *device);
static void calcsisoffset(Freq_Object *device);
static void setSISPTS(int base, int offset, Freq_Object *device, int setoop);
void apcodes(int boardadd, int breg, int longw, int *words);
void setlkdecfrq(Freq_Object *device);

/*-------------------------------------------------------------
| Freq_Device()/4 - Message Handler for Freq_devices.
|			Author: Greg Brissey  6/14/89
+-------------------------------------------------------------*/
int Freq_Device(Freq_Object *this, Message msg, void *param, void *result)
{
   int             error = 0;

   switch (msg)
   {
      case MSG_NEW_DEV_r:
	 error = init_attr( (Msg_New_Result *) result);
	 break;
      case MSG_SET_FREQ_ATTR_pr:
	 error = set_attr(this, (Msg_Set_Param *) param, (Msg_Set_Result *) result);
	 break;
      case MSG_GET_FREQ_ATTR_pr:
	 error = get_attr(this, (Msg_Set_Param *) param, (Msg_Set_Result *) result);
	 break;
      default:
	 error = Base(this, msg, param, result);
	 break;
   }
   return (error);
}

/*----------------------------------------------------------
|  init_attr()/1 - create a new Frequency device structure
|			Author: Greg Brissey
+----------------------------------------------------------*/
static int init_attr(Msg_New_Result *result)
{
   result->object = (Object) malloc(sizeof(Freq_Object));
   if (result->object == (Object) 0)
   {
      fprintf(stderr, "Insuffient memory for New Attn Device!!");
      return (NO_MEMORY);
   }
   ClearTable(result->object, sizeof(Freq_Object));	/* besure all clear */
   result->object->dispatch = (Functionp) Freq_Device;
   return (0);
}

void fixup_after_tune(Freq_Object *this, int chan )
{
	this->dev_channel = chan;
	calc_nondbl_Offset(this);
}

int
do_tune_acodes(Freq_Object *this )
{
	int	cnt, iter;
        double  amphbmin, whatamphibandmin();
        double ifval, ptsval, nmrfreq;

	calc_nondbl_Offset(this);

	cnt = this->codecnt;
	putcode(cnt - 1);
	for (iter = 0; iter < cnt; iter++)
	  putcode(this->frq_codes[iter]);
       
	amphbmin = whatamphibandmin(OBSERVE,(double)(this->h1freq));

        ifval  =(this->iffreq)*1.0e6;
        ptsval =this->pts_freq;
        nmrfreq=ptsval-ifval;

        if ((this->base_freq) > amphbmin)	/* pass info for Rcvr Mixer HB/LB */

        /* High Band Rcvr Mixer but need to check for correct Tx Mixer selection  */
        {
          if (nmrfreq <= TXMIXSELFRQ)
          {
            putcode(0x10);
          }
          else
          {
            putcode(0);
          }
        }

        else
        /* Low  Band Rcvr mixer but need to check for correct Tx Mixer selection  */
        {
          if (nmrfreq <= TXMIXSELFRQ) 
          {
            putcode(0x10 + 1);
          }
          else
          {
            putcode(1);
          }
        }

	return( 0 );
}

/*----------------------------------------------------------
| set_attr()/3 - static routine that has access to Freq device
|		 attributes, address,register,bytes,mode
|			Author: Greg Brissey  6/14/89
+----------------------------------------------------------*/
static int set_attr(Freq_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;
   extern void   calcoffsetsyn();
   extern void   calcfixedoffset();

   result->resultcode = result->genfifowrds = result->reqvalue = 0;
   result->DBreqvalue = 0.0;
   switch (param->setwhat)
   {
      case SET_PTSVALUE:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->ptsval = (int) param->DBvalue;
	 break;
      case SET_OVERRANGE:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->overrange = (int) param->DBvalue;
	 break;
      case SET_PTSOPTIONS:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->ptsoptions = (int) param->DBvalue;

         /*--- set overunderflag here since this is an initialization */
         this->overunderflag = 0;
	 break;
      case SET_IFFREQ:		/* observe, decoupler, etc */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->iffreq = (double) param->DBvalue;
	 break;
      case SET_RFTYPE:		/* observe, decoupler, etc */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->rftype = (int) param->DBvalue;
	 break;
      case SET_H1FREQ:		/* observe, decoupler, etc */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->h1freq = (int) param->DBvalue;
	 break;
      case SET_RFBAND:		/* observe, decoupler, etc */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->rfband = (int) param->DBvalue;
	 break;
      case SET_FREQSTEP:	/* frequency step size (Hz) */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->freq_stepsize =  param->DBvalue; /* changed to double 7/17/90 */
	 break;
      case SET_OSYNBFRQ:	/* offset syn base freq  for rftype 'b' */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->ofsyn_basefrq = (double) param->DBvalue;
	 break;
      case SET_OSYNCFRQ:	/* offset syn constant freq  for rftype 'b' */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->ofsyn_constfrq = (double) param->DBvalue;
	 break;
      case SET_SPECFREQ:	/* sfrq or dfrq  */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->base_freq = (double) param->DBvalue;
	 /* set overunderflag to 0 so first offset which will set	*/
	 /* frequency will not try to apply overranging			*/
	 this->overunderflag = 0;
	 break;
      case SET_INIT_OFFSET:	/* tof or dof  */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->init_offset_freq = (double) param->DBvalue;
         /* NOTE:  Fall through to next case */
      case SET_OFFSETFREQ:		/* tof or dof frequency */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->offset_freq = (double) param->DBvalue;
         switch (this->rftype)
         {
	    case DIRECT_NON_DBL:
	       calc_nondbl_Offset(this);
	       break;
            case DIRECTSYN:
               /* init_ptsfreq setting moved to calcdirectOffset */
	       calcdirectOffset(this);
	       break;
            case OFFSETSYN:
	       calcoffsetsyn(this);
	       break;
            case FIXED_OFFSET:
	       calcfixedoffset(this);
	       break;
            case IMAGE_OFFSETSYN:
	       calcsisoffset(this);
	       break;
            case LOCK_DECOUP_OFFSETSYN:
	       setlkdecfrq(this);
	       break;
   	 }
	 /* Set so next offset will check for overranging	*/
	 this->overunderflag = RANGEOFF;
	 break;

      case SET_OFFSETFREQ_STORE:		/* tof or dof frequency */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->offset_freq = (double) param->DBvalue;
         switch (this->rftype)
         {
	    case DIRECT_NON_DBL: /* Must be Unity+ for success */
	       calc_nondbl_OffsetStorage(this,1);
	       break;
            case LOCK_DECOUP_OFFSETSYN:
	       setlkdecfrq(this);
	       break;
	    default:
	       error = UNKNOWN_FREQ_ATTR;	
	       result->resultcode = UNKNOWN_FREQ_ATTR;	
	       break;
   	 }
	 break;

      case SET_FREQ_FROMSTORAGE:		/* tof or dof frequency */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
         switch (this->rftype)
         {
	    case DIRECT_NON_DBL: /* Must be Unity+ for success */
	       set_nondbl_freq_fromstorage(this,(int) param->DBvalue); 
	       break;
	    default:
	       error = UNKNOWN_FREQ_ATTR;	
	       result->resultcode = UNKNOWN_FREQ_ATTR;	
	       break;
	 }
	 break;

	/*  Used by PSG (su, go, etc.) */

      case SET_VALUE:
       {
         int             cnt;
         int             i;

	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
         /* setfrequency(this); */	/* generate appropriate acodes */

         if (this->ptsoptions & (1 << USE_SETPTS)) /* gen SETPTS or apwords */
         {
	    putcode(SETPTS);
	    for (i = 0; i < 7; i++)
	    {
	       putcode((c68int) this->frq_codes[i]);
	    }
	    curfifocount += 9;
         }
         else
         {
	    cnt = this->codecnt;
	    putcode(APBOUT);
	    putcode(cnt - 1);
	    for (i = 0; i < cnt; i++)
	    {
	       putcode(this->frq_codes[i]);
	    }
	    curfifocount += cnt;

	    /* Output the Tune Freq Initialization acode for UNITYPLUS HARDWARE */
	    /* (but only if not done previously for this frequency object */
#ifndef INTERACT
            if ( (this->rftype) == DIRECT_NON_DBL && (this->channel_is_tuned) == 0 &&
		 (is_psg4acqi() == 0) )
	    {
	      putcode(TUNE_FREQ);
	      putcode( mapRF(this->dev_channel) );

	/*  Tune requires use of PTS 1.  Set dev_channel to use this PTS.
	    Call calc nondoubling offset to compute new APBOUT words.
	    Output the resulting frequency codes.  Then restore the
	    original value for dev_channel.  Restore the original
	    frequency codes by calling calc nondoubling offset again.	*/

	      tune_from_freq_obj( this, this->dev_channel );
	      this->channel_is_tuned = 131071;
	    }
#endif 
          }
       }
	 break;


      case SET_GTAB:
       {
         int             cnt;
         int             i;

	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
         /* setfrequency(this); */	/* generate appropriate acodes */

	 validate_imaging_config("Freq list");

         if (this->ptsoptions & (1 << USE_SETPTS)) /* gen SETPTS or apwords */
         {
	    putgtab(param->value,SETPTS);
	    for (i = 0; i < 7; i++)
	    {
	       putgtab(param->value,(c68int) this->frq_codes[i]);
	    }
	    curfifocount += 9;
         }
         else
         {
	    cnt = this->codecnt;
	    putgtab(param->value,APBOUT);
	    putgtab(param->value,cnt - 1);
	    for (i = 0; i < cnt; i++)
	    {
	       putgtab(param->value,this->frq_codes[i]);
	    }
	    curfifocount += cnt;
          }
       }
       break;
	 

/*  If channel is Direct Non-Doubling, this case is
    identical to SET_VALUE following putcode(TUNE_FREQ).

    Used by VNMR (tune command)				*/

      case SET_TUNE_FREQ:
	 if ( (this->rftype) != DIRECT_NON_DBL ) {		/* Must be Hydra for  */
		error = UNKNOWN_FREQ_ATTR;			/* this message to be */
		result->resultcode = UNKNOWN_FREQ_ATTR;		/* successful */
	 }
	 else {
		int	chan;

		putcode(this->dev_channel);

		chan = this->dev_channel;
		if (chan != OBSERVE) {
			this->dev_channel = OBSERVE;
		}

		do_tune_acodes( this );

		if (chan != OBSERVE) {
			this->dev_channel = chan;
			calc_nondbl_Offset(this);
		}
	 }

	 break;

      case SET_OVRUNDRFLG:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
         this->overunderflag = (int) param->DBvalue;
	 break;

      case SET_SWEEPCENTER:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
         this->sweepcenter = param->DBvalue;
	 break;

      case SET_SWEEPWIDTH:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
         this->sweepwidth = param->DBvalue;
         if (this->sweepnp > 0)
	   this->sweepincr = this->sweepwidth / this->sweepnp;
	 break;

      case SET_SWEEPNP:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
         this->sweepnp = param->DBvalue;
         this->maxswpwidth = 
	       this->sweepnp * ((double) ((unsigned int) 0xffff));
         if (this->sweepwidth > 0)
	   this->sweepincr = this->sweepwidth / this->sweepnp;
	 break;

      case SET_SWEEPMODE:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
         this->sweeprtptr = (int) (param->DBvalue + 0.05);
	 if (this->sweeprtptr <= 0)
	    this->sweeprtptr = -1;
	 break;

      case SET_INITSWEEP:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
          error = initswpfreq(this);
	 break;

      case SET_INCRSWEEP:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
         error = incrswpfreq(this);
	 break;

      default:
	 error = UNKNOWN_FREQ_ATTR;
	 result->resultcode = UNKNOWN_FREQ_ATTR;
	 break;
   }
   return (error);
}

/*----------------------------------------------------------
| get_attr()/3 - static routine that has access to freq device
|			Author: Greg Brissey  7/23/89
+----------------------------------------------------------*/
static int get_attr(Freq_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;

   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case GET_BASE_FREQ:
	 result->DBreqvalue = this->base_freq;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %lf for device: '%s'\n",
		 ObjCmd(param->setwhat), result->DBreqvalue, this->objname);
	 break;
      case GET_OF_FREQ:
	 result->DBreqvalue = this->of_freq;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %lf for device: '%s'\n",
		 ObjCmd(param->setwhat), result->DBreqvalue, this->objname);
	 break;
      case GET_PTS_FREQ:	/* observe, decoupler, etc */
	 result->DBreqvalue = this->pts_freq;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %lf for device: '%s'\n",
		 ObjCmd(param->setwhat), result->DBreqvalue, this->objname);
	 break;
      case GET_SPEC_FREQ:
	 result->DBreqvalue = this->spec_freq;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %lf for device: '%s'\n",
		 ObjCmd(param->setwhat), result->DBreqvalue, this->objname);
	 break;
      case GET_FREQSTEP:
         result->DBreqvalue = this->freq_stepsize;
         DPRINT3(DPRTLEVEL,
                 "get_attr: Cmd: '%s', is %lf for device: '%s'\n",
                 ObjCmd(param->setwhat), result->DBreqvalue, this->objname);
         break;
      case GET_RFBAND:  /* obtain rf band to use */
         {
           int highband;
	   double lbandmax_freq;

           if (this->rftype == IMAGE_OFFSETSYN)
           {
              lbandmax_freq = 130.0;                              /* MHz */
           }
	   else if (this->rftype == FIXED_OFFSET)

           {
              lbandmax_freq = 0;                                  /* MHz */
           }
	   else
           {
              lbandmax_freq = ((double) this->ptsval) - this->iffreq; /* MHz */
           }
	   highband = select_rfband(this->rfband, this->spec_freq * 1.0E-6, lbandmax_freq);
           if (highband)
           {
	     result->reqvalue = RF_HIGH_BAND;
	     result->DBreqvalue = RF_HIGH_BAND;
           }
	   else
           {
	     result->reqvalue = RF_LOW_BAND;
	     result->DBreqvalue = RF_LOW_BAND;
           }
	   DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %d for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
         }
	 break;

      case GET_SWEEPCENTER:
         result->DBreqvalue = this->sweepcenter;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %lf for device: '%s'\n",
		 ObjCmd(param->setwhat), result->DBreqvalue, this->objname);
	 break;

      case GET_SWEEPWIDTH:
         result->DBreqvalue = this->sweepwidth;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %lf for device: '%s'\n",
		 ObjCmd(param->setwhat), result->DBreqvalue, this->objname);
	 break;

      case GET_SWEEPNP:
         result->DBreqvalue = this->sweepnp;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %lf for device: '%s'\n",
		 ObjCmd(param->setwhat), result->DBreqvalue, this->objname);
	 break;

      case GET_SWEEPINCR:
         result->DBreqvalue = this->sweepincr;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %lf for device: '%s'\n",
		 ObjCmd(param->setwhat), result->DBreqvalue, this->objname);
	 break;

      case GET_SWEEPMODE:
         result->DBreqvalue = this->sweeprtptr;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %lf for device: '%s'\n",
		 ObjCmd(param->setwhat), result->DBreqvalue, this->objname);
	 break;

      case GET_SWEEPMAXWIDTH:
         result->DBreqvalue = this->maxswpwidth;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %lf for device: '%s'\n",
		 ObjCmd(param->setwhat), result->DBreqvalue, this->objname);
	 break;

      case GET_LBANDMAX:  /* obtain low band max frequency */
         {
	   double lbandmax_freq;

           if (this->rftype != FIXED_OFFSET)
           {
              lbandmax_freq = ((double) this->ptsval) - this->iffreq;	/* MHz */
           }
           else  /* fix freq lbandmax_freq = 0 */
           {
              lbandmax_freq = 0; 				/* MHz */
           }
	   result->reqvalue = (int) lbandmax_freq;
	   result->DBreqvalue = lbandmax_freq;
	   DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %lf for device: '%s'\n",
		 ObjCmd(param->setwhat), result->DBreqvalue, this->objname);
         }
	 break;

      case GET_MAXXMTRFREQ:  /* obtain maximum xmtr frequency */
         {			/* HYDRA */
	   double lbandmax_freq;
           int	  highband;

           if (this->rftype != FIXED_OFFSET)
           {
              lbandmax_freq = ((double) this->ptsval) - this->iffreq;	/* MHz */
           }
           else  /* fix freq lbandmax_freq = 0 */
           {
              lbandmax_freq = 0; 				/* MHz */
           }
           highband = select_rfband(this->rfband, 
			    this->spec_freq * 1.0E-6, lbandmax_freq);
           if (highband && (this->rftype != DIRECT_NON_DBL) )
           {
             result->DBreqvalue  = (((double) this->ptsval) * 2.0) - this->iffreq;
           }
           else
           {
	     result->DBreqvalue = lbandmax_freq;
           }
	   DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %lf for device: '%s'\n",
		 ObjCmd(param->setwhat), result->DBreqvalue, this->objname);
         }
	 break;

      case GET_SIZETUNE:
	 result->reqvalue = this->codecnt;
	 result->DBreqvalue = (double) (this->codecnt);
	 break;

/*  Tune on UnityPlus imposes some special requirements.  The tune frequency for
    any channel is synthesized with PTS 1.  Thus when sending the A_codes for the
    alternate channels, one must save all the attributes relating to the PTS, set
    those attributes based on PTS 1 and then restore the original attributes.

    Here we obtain the original attributes.					*/

      case GET_PTSOPTIONS:
	 result->reqvalue = this->ptsoptions;
	 result->DBreqvalue = (double) (this->ptsoptions);
	 break;

      case GET_IFFREQ:
	 result->DBreqvalue = this->iffreq;
	 break;

      case GET_PTSVALUE:
	 result->reqvalue = this->ptsval;
	 result->DBreqvalue = (double) (this->ptsval);
	 break;

      case GET_RFTYPE:
	 result->reqvalue = this->rftype;
	 result->DBreqvalue = (double) (this->rftype);
	 break;

      case GET_OSYNBFRQ:
	 result->DBreqvalue = this->ofsyn_basefrq;
	 break;

      case GET_OSYNCFRQ:
	 result->DBreqvalue = this->ofsyn_constfrq;
	 break;

      default:
	 error = UNKNOWN_FREQ_ATTR;
	 result->resultcode = UNKNOWN_FREQ_ATTR;
	 break;
   }
   return (error);
}


/*---------------------------------------------------------------------
| SetFreqAttr  -  Set Frequency Device to List of Attributes
|			Author: Greg Brissey  8/18/88
+---------------------------------------------------------------------*/
int SetFreqAttr(Object obj, ...)
{
   va_list         vargs;
   int             error = 0;
   int             error2 = 0;
   Msg_Set_Param   param;
   Msg_Set_Result  result;

   if(obj == NULL)
   {
	abort_message("SetFreqtAttr: Device is not present.\n");
        return(-1);
   }
   va_start(vargs, obj);

/*  VMS C has a problem in that when fixed arguments preceed
    variable argument list, the fixed arguments are the first
    ones in the variable argument list.  So one must throw out
    those variable arguments that correspond to the fixed
    arguments.							*/
#ifdef VMS
   error = va_arg(vargs, int );  /* Corresponds to Object obj;	*/
#endif 

   /* Options allways follow the format 'Option,(value),Option,(value),etc' */
   while ((param.setwhat = va_arg(vargs, int)) != 0)
   {
      if (param.setwhat != SET_DEFAULTS)	/* default has no value
						 * parameter */
	 param.DBvalue = (double) va_arg(vargs, double);
      error = Send(obj, MSG_SET_FREQ_ATTR_pr, &param, &result);
      if (error < 0)
      {
	 param.value = (int) (param.DBvalue + 0.45);
	 error2 = Send(obj, MSG_SET_AP_ATTR_pr, &param, &result);
	 if (error2 < 0)
	 {
	    error2 = Send(obj, MSG_SET_DEV_ATTR_pr, &param, &result);
	    if (error2 < 0)
	    {
	       text_error("%s : %s  '%s'\n", obj->objname, ObjError(error),
		       ObjCmd(param.setwhat));
	       error = error2;
	       break;
	    }
	    else
	     error = error2;
	 }
	 else
	  error = error2;
      }
   }
   va_end(vargs);
   return (error);
}

/*-------------------------------------------------------------------
|  select_rfband() - select proper rf band
|			Author: Greg Brissey  7/14/89
|  returns 1 for highband, 0 for low band.
+-------------------------------------------------------------------*/
static int select_rfband(int rfband, double freq, double max_lowband_frq)
{
   if (rfband == RF_BAND_AUTOSELECT)
   {
      if (freq > max_lowband_frq)
      {
	 return (1);		/* high band */
      }
      else
      {
	 return (0);		/* low band */
      }
   }
   else if (rfband == RF_HIGH_BAND)
   {
      return (1);		/* high band */
   }
   else
   {
      return (0);		/* low band */
   }
}

/*-------------------------------------------------------------------
|  chk4heterodecmode() - select proper special hetero decoupler mode
|			Author: Greg Brissey  7/14/89
+-------------------------------------------------------------------*/
static int
chk4heterodecmode()
{
   extern double   sfrq,
                   dfrq;
   double          frqdif;

   /* Determine if Obs & Dec freq are within 1MHz */
   frqdif = dfrq - sfrq;
   if (frqdif < 0.0)
      frqdif = -frqdif;		/* absolute value */
   if (frqdif < 1.0)
      return (1);
   else
      return (0);
}

/*----------------------------------------------------------------------
|  calc_nondbl_OffsetStorage() - calc Base Frequncy  & PTS value for frequency
|			  This routine supports the synthesizers for
|			  hardware released in 6/92, with XMTRs that
|			  no logner support doubling
+----------------------------------------------------------------------*/
/* spare flag to specify if the frequency should 	*/
/* be put in the spare freq storage. 1=yes 0=standard */
static void calc_nondbl_OffsetStorage(Freq_Object *device, int spare)
{
   double	ptsfreq;
   int		pts_mode;
//   int		ptsbit;
   int		reg,apadr;

   device->spec_freq = round_freq(device->base_freq, device->offset_freq,
                                  device->init_offset_freq,
                                  device->freq_stepsize);
   ptsfreq = (device->spec_freq + ( (device->iffreq) * 1.0e6 ));
   device->pts_freq = ptsfreq;

   if (bgflag)
   {
     fprintf(stderr,
     "nondbl_Offset: %11.2lf(SpecFreq) = %11.2lf(Base) + (%11.2lf(offset) - %11.2f(init_offset)\n",
       device->spec_freq, device->base_freq*1e6, device->offset_freq, device->init_offset_freq);
     fprintf(stderr, "nondbl_Offset: Channel stepsize= %11.2lf\n", device->freq_stepsize);
   }

   if (device->overunderflag == 0)
   {  device->init_ptsfreq = device->pts_freq;
   }
   if ( ( !(device->ptsoptions & (1 << OVR_UNDR_RANGE)) ) ||
             ( device->overunderflag == 0) )
   {  pts_mode = RANGEOFF;
   }
   else
   {  pts_mode = PTSrange(device);	/* use over or underrange or neither */
   }					/* recalc pts freq if needed */

   spare = spare & 0x1;			/* make sure spare is 0 or 1 */
   if (spare)
      device->spareoverunderflag = pts_mode;
   else
      device->overunderflag = pts_mode;
   device->codecnt = 0;

   reg    = device->ap_reg;
   apadr  = device->ap_adr << 8;
   // ptsbit = 1 << (device->dev_channel-1);	/* e.g. 00001000 */
   // enable = ~ptsbit & 0x1e;			/* e.g. xxx1011x */
   device->frq_codes[device->codecnt]   = APSELECT|apadr|(reg-1);
   if (( device->ptsoptions & (1 << OVR_UNDR_RANGE)) && !spare)
   { if ( pts_mode == UNDERRANGE )
     {  device->frq_codes[device->codecnt+1] = APWRITE |apadr|0x0c;
     }
     else if ( pts_mode == OVERRANGE )
     {  device->frq_codes[device->codecnt+1] = APWRITE |apadr|0x04;
     }
     else	/* use_neither */
     {  device->frq_codes[device->codecnt+1] = APWRITE |apadr;
     }
   }
   else		/* only latching */
   {  device->frq_codes[device->codecnt+1] = APWRITE |apadr|spare;
   }
   device->codecnt += 2;
   setPTS(device->pts_freq,device);	/* 5 more AP bus words */
}

/*----------------------------------------------------------------------
|  calc_nondbl_Offset() - calls calc_nondbl_OffsetStorage for main
|			  frequency storage location zero to calc 
|			  Base Frequncy  & PTS value for frequency.
|			  Then calcultates the acodes to set the PTS
|			  with that frequency.
+----------------------------------------------------------------------*/
static void calc_nondbl_Offset(Freq_Object *device)
{
   int		ptsbit;
   int		apadr;

   calc_nondbl_OffsetStorage(device,0);

   // reg    = device->ap_reg;
   apadr  = device->ap_adr << 8;
   ptsbit = 1 << (device->dev_channel-1);	/* e.g. 00001000 */
   // enable = ~ptsbit & 0x1e;			/* e.g. xxx1011x */
   device->frq_codes[device->codecnt]   = APPREINCWR |apadr|ptsbit;
   device->frq_codes[device->codecnt+1] = APWRITE    |apadr;
   device->codecnt += 2;
}

/*----------------------------------------------------------------------
|  set_nondbl_freq_fromstorage() - Sets whatever frequency value is in the
|			  stored or standard AP&F register.
+----------------------------------------------------------------------*/
/* spare flag to specify if the frequency should 	*/
/* be set from spare or std storage. 1=spare 0=std */
static void set_nondbl_freq_fromstorage(Freq_Object *device, int spare)
{
   int		pts_mode;
   int		ptsbit;
   int		reg,apadr;

   spare = spare & 0x1;			/* make sure spare is 0 or 1 */
   if (spare)
      pts_mode = device->spareoverunderflag;
   else
      pts_mode = device->overunderflag;

   device->codecnt = 0;
   reg    = device->ap_reg;
   apadr  = device->ap_adr << 8;
   ptsbit = 1 << (device->dev_channel-1);	/* e.g. 00001000 */
   // enable = ~ptsbit & 0x1e;			/* e.g. xxx1011x */
   device->frq_codes[device->codecnt]   = APSELECT|apadr|(reg-1);
   if ( device->ptsoptions & (1 << OVR_UNDR_RANGE) )
   { if ( pts_mode == UNDERRANGE )
     {  device->frq_codes[device->codecnt+1] = APWRITE |apadr|0x0c|spare;
     }
     else if ( pts_mode == OVERRANGE )
     {  device->frq_codes[device->codecnt+1] = APWRITE |apadr|0x04|spare;
     }
     else	/* use_neither with spare freq*/
     {  device->frq_codes[device->codecnt+1] = APWRITE |apadr|spare;
     }
   }
   else		
   {  device->frq_codes[device->codecnt+1] = APWRITE |apadr|spare;
   }
   device->codecnt += 2;

   device->frq_codes[device->codecnt]   = APSELECT|apadr|(reg+1);
   device->frq_codes[device->codecnt+1] = APWRITE |apadr|ptsbit;
   device->frq_codes[device->codecnt+2] = APWRITE |apadr;
   device->codecnt += 3;
}

/*----------------------------------------------------------------------
|  calcdirectOffset() - calc Base Frequncy  & PTS value for frequency
|			Author: Greg Brissey  7/14/89
+----------------------------------------------------------------------*/
static void calcdirectOffset(Freq_Object *device)
{
   double          ptsfreq;
   double          lbandmax_freq;
   int            freqKHz;
   int            freqtHz;
   int             highband;
   int             within1MHz;
   int             mode;
   int		   pts_mode;

   lbandmax_freq = ((double) device->ptsval) - device->iffreq;	/* MHz */

   device->spec_freq = round_freq(device->base_freq, device->offset_freq,
                                  device->init_offset_freq,
                                  device->freq_stepsize);
   if (bgflag)
   {
     fprintf(stderr,
     "calcdirectOffset: %11.2lf(SpecFreq) = %11.2lf(Base) + (%11.2lf(offset) - %11.2f(init_offset)\n",
       device->spec_freq, device->base_freq*1e6, device->offset_freq, device->init_offset_freq);
     fprintf(stderr, "calcdirectOffset: Channel stepsize= %11.2lf\n", device->freq_stepsize);
   }

   /* Low or High Band */
   highband = select_rfband(device->rfband, (device->spec_freq)*1.0e-6, lbandmax_freq);
   if (highband && device->rftype!=DIRECT_NON_DBL)	/* high band */
      ptsfreq = (device->spec_freq + ( (device->iffreq) * 1.0e6 ) ) / 2.0;
   else
      ptsfreq = (device->spec_freq + ( (device->iffreq) * 1.0e6 ));

   device->pts_freq = ptsfreq;

   if (device->ptsoptions & (1 << USE_SETPTS)) /* gen SETPTS or apwords */
   {
     freqKHz = (int) ((ptsfreq / 1000.0) +.0005);	/* freq KHz */
     freqtHz = (int) ((ptsfreq - ((double) freqKHz * 1000.0)) * 10.0);	/* .1Hz */

     device->frq_codes[0] = (int) (freqKHz >> 8);
     device->frq_codes[1] = (int) (freqKHz & 0x00ff);
     device->frq_codes[2] = (int) (freqtHz >> 8);
     device->frq_codes[3] = (int) (freqtHz & 0x00ff);

     if (bgflag)
      fprintf(stderr,
	  "directsyn(): PTSfreq = %13.1lf, freqKHz = %d, freqtHz = %d \n",
	      ptsfreq, freqKHz, freqtHz);

     /* Determine if Obs & Dec freq are within 1MHz */
     within1MHz = chk4heterodecmode();
     /* setup High or Low band and also the special heterodecoupler mode */
     /* bit0 0=High band 1=Low Band */
     /* bit1 0=not special HOMO Decoupling mode 1=It is. */
     mode = 0;
     if (within1MHz)
        mode |= 2;		/* decoupler mode bit ON */
     if (device->dev_channel == OBSERVE)	/* transmitter */
     {
        if (!highband)
	   mode = 3;		/* low band set bit0 and bit1 to 1 */
        else
	   mode = 0;		/* no homodecoupling mode for transmitter */
     }
     else
     {
        if (!highband)
	   mode |= 1;		/* low band set bit-0 to 1 */
     }
     device->frq_codes[4] = mode;
     device->frq_codes[5] = device->dev_channel - 1;
     device->frq_codes[6] = (device->ptsval == 160);
     device->codecnt = 7;
     if (bgflag)
      fprintf(stderr,
	      "directsyn(): mode = %d\n", mode);
   }
   else
   {
     if (device->ptsoptions & (1 << LATCH_PTS))
     {
        int reg,apadr,apselect;
	

        reg = device->ap_reg - 5;
        apadr = (device->ap_adr) << 8;
	apselect = APSELECT | apadr | (reg & 0xff);
       /*----------------------------------------
       | if latch & no over/under then    5, 15, 11
       |  else if latch & overrange then  4, 14, 10
       |  else if latch & underrange then 6, 16, 12
       | Skip if init_ptsfreq has not been esablished yet
       |  as determined by overunderflag being 0
       +----------------------------------------*/
        if (device->overunderflag == 0)
        {
           device->init_ptsfreq = device->pts_freq;
        }
	if ( ( !(device->ptsoptions & (1 << OVR_UNDR_RANGE)) ) ||
             ( device->overunderflag == 0) )
	{
	   pts_mode = RANGEOFF;
        }
	else
	{
	  pts_mode = PTSrange(device); /* use over or underrange or neither */
				       /* recalc pts freq if needed */
        }

        device->overunderflag = pts_mode;
        device->codecnt = 0;
        setPTS(device->pts_freq,device);

	if ( pts_mode == OVERRANGE )
        {
            device->frq_codes[device->codecnt] = apselect;
            device->frq_codes[device->codecnt+1] = APWRITE | apadr | 0x004;
            device->frq_codes[device->codecnt+2] = APWRITE | apadr | 0x014;
            device->frq_codes[device->codecnt+3] = APWRITE | apadr | 0x010;
            device->codecnt += 4;
        }
        else if ( pts_mode == UNDERRANGE )
        {
            device->frq_codes[device->codecnt] = apselect;
            device->frq_codes[device->codecnt+1] = APWRITE | apadr | 0x006;
            device->frq_codes[device->codecnt+2] = APWRITE | apadr | 0x016;
            device->frq_codes[device->codecnt+3] = APWRITE | apadr | 0x012;
            device->codecnt += 4;
        }
        else  /* use_neither */
        {
           device->frq_codes[device->codecnt] = apselect;
           device->frq_codes[device->codecnt+1] = APWRITE | apadr | 0x005;
           device->frq_codes[device->codecnt+2] = APWRITE | apadr | 0x015;
           device->frq_codes[device->codecnt+3] = APWRITE | apadr | 0x011;
           device->codecnt += 4;
        }
     }
     else
     {
       device->codecnt = 0;
       setPTS(device->pts_freq,device);
     }
   }

}
/*-----------------------------------------------------------------------
| int  PTSrange()/1  - determine if over or under ranging PTS is required.
|
|      compare requested PTS frequency with the initial PTS frequency,
|      if the don't differ from 100K up then no ranging required.
|      if requested freq is greater than the initial then OVERRANGE
|      else UNDERRANGE....
|				Author:  Greg Brissey   8-9-89
|
|      Modified to allow different freq. resolutions 
|	1.0 hz resolution 1Mhz boundary - LATER -
+------------------------------------------------------------------------*/
static int PTSrange(Freq_Object *device)
{
   int initptsfreq;
   int newptsfreq;
   double level,tmpoverrange;

   tmpoverrange = (device->overrange*10.0)/device->freq_stepsize;
   level= tmpoverrange * (double) device->freq_stepsize; /* 1 Hz units */
   if ( (device->ptsval >= 500) &&
        (device->pts_freq  > (double) (device->ptsval * 1e6 / 2)) )
   {
      level *= 2.0;
   }
   DPRINT1(DPRTLEVEL,"level = %f\n",level);
   initptsfreq = (int) (device->init_ptsfreq / level);
   newptsfreq = (int)  (device->pts_freq / level);

   if ( newptsfreq != initptsfreq )
   {
      if ( newptsfreq > initptsfreq)
      {
           device->pts_freq -= level;
           DPRINT2(DPRTLEVEL,"init = %f   new = %f \n",
		    device->init_ptsfreq,device->pts_freq);
           DPRINT(DPRTLEVEL,"over range required\n");
           return(OVERRANGE);
      }
      else  /* newptsfreq < initptsfreq */
      {
           device->pts_freq += level;
           DPRINT2(DPRTLEVEL,"init = %f   new = %f \n",
		   device->init_ptsfreq,device->pts_freq);
           DPRINT(DPRTLEVEL,"under range required\n");
           return(UNDERRANGE);
      }
   }
   else
   {
      return(RANGEOFF);
   }
}

/*-----------------------------------------------------------------------
|
| calcoffsetsyn()/3
|	calculate the offset synthesizer frequency of the old style RF
|	return calculated tbo or dbo (offset freq Hz)
|
|				Author Greg Brissey  6/16/86
|------------  NOTE ------------------------------------------------
| changed frequency calc, lock & solvent corrections are applied before
|                         tof or dof are added. This correction is also
|			  rounded up to the larger of the tof,dof stepsize
|			  prior to tof or dof addition.  Thus tof & dof are
|			  not adjusted & give control to the stepsize of that
|			  device. tbo & dbo are not used any more.
|					Greg Brissey  8/20/87
|			Author: Greg Brissey  7/14/89
|  Modified 7/17/90
|	           No Longer round up to the larger of the tof,dof stepsize
|		   round to stepsize of channel (tof,dof,dof2)
|		   device->freq_stepsize now contains the actual stepsize
+----------------------------------------------------------------------*/
static void calcoffsetsyn(Freq_Object *device)
{
   int             highband;
   int             cnt;
   double          baseMHz;	/* base freq, (i.e., tbo,dbo ) */
   double          specfreq;
   double          lbandmax_freq;
   double          pts;		/* PTS syn value */

   lbandmax_freq = ((double) device->ptsval) - device->iffreq;	/* MHz */

   device->spec_freq = round_freq(device->base_freq, device->offset_freq,
                                  device->init_offset_freq,
                                  device->freq_stepsize);
   if (bgflag)
   {
     fprintf(stderr,
     "calcoffsetsyn: %11.2lf(SpecFreq) = %11.2lf(Base) + (%11.2lf(offset) - %11.2f(init_offset)\n",
       device->spec_freq, device->base_freq*1e6, device->offset_freq, device->init_offset_freq);
     fprintf(stderr, "calcoffsetsyn: Channel stepsize= %11.2lf\n", device->freq_stepsize);
   }

   specfreq = device->spec_freq * 1e-6;   /* MHz */

   /* obtain exact pts value from SpecFreq */
   /* pts = iffreq + baseMHz - specfreq; */
   pts = device->ofsyn_constfrq + device->ofsyn_basefrq - specfreq;

   if (bgflag)
   {
      fprintf(stderr,
      " %11.7lf(pts) = %7.3lf(constfrq) + %11.7lf(base) - %11.7lf(specfreq)\n",
	      pts, device->ofsyn_constfrq, device->ofsyn_basefrq, specfreq);
   }

   if (pts < 0.0)
      pts = -pts;		/* absolute value */

   /* round exact pts to 100KHz, PTS hardware good only to 100KHz */
   pts = (double) ((long) ((pts * 10.0) +.5)) / 10.0;
   if (bgflag)
      fprintf(stderr, "offsetsyn: %7.3lf(pts) rounded to 100KHz\n", pts);

   baseMHz = specfreq - device->ofsyn_constfrq;

   /* rfband High or Low ? */
   highband = select_rfband(device->rfband, specfreq, lbandmax_freq);
   if (highband)
      baseMHz -= pts;
   else
      baseMHz += pts;

   if (bgflag)
      fprintf(stderr,
	      " %11.7lf(base) = (+/-)%11.7lf(SpecFreq) (+/-) %11.7lf(pts) - %6.2lf(constfrq)\n",
	      baseMHz, specfreq, pts, device->ofsyn_constfrq);


   if (device->pts_freq != pts)
   {
     device->codecnt = 0;
     setPTS(pts, device);
     device->pts_freq = pts;
   }
   else
   {
     device->codecnt = (device->ptsval == 160) ? 3 : 4; /* skip past PTS settings */
   }

   baseMHz = round_freq(baseMHz, (double) 0.0, (double) 0.0,
                        device->freq_stepsize);
   device->of_freq = baseMHz;
   baseMHz = (baseMHz * 10.0 + 0.45);

   if (bgflag)
      fprintf(stderr,
	      "offsetsyn(): tbo = %11.2lf, tof= %7.2lf, freq=%11.7lf\n",
	      baseMHz, device->offset_freq, specfreq);

   cnt = device->codecnt;
   formXLwords(baseMHz, 8, 0, device->dev_channel, &(device->frq_codes[cnt]));
   device->codecnt += 8;
}



/*-------------------------------------------------------------------
|
|	setPTS()/3
|	set PTS for old style RF scheme
|				Author Greg Brissey  6/30/86
|			Author: Greg Brissey  7/14/89
+------------------------------------------------------------------*/
static void setPTS(double ptsvalue, Freq_Object *device)
{
   int             num;
   int             ptstype;
   int            *iptr;
   double          div,ptsfreq;
   int             digit;
   int             bcd,tmpbcd;
   int             bcdbcd = 0;
   int             reg;
   int             apadr;
   int             paired[5];

   ptsfreq = ptsvalue;
   /* --- create APBUSS words to control PTS --- */
   /* not rounding, making sure 30.0=30 */
   ptstype = device->ptsval;

   if ( (device->rftype != DIRECTSYN) && (device->rftype != DIRECT_NON_DBL) )
   {
      if (ptstype == 160)
	 num = 3;		/* number of apbuss words to create */
      else
	 num = 4;		/* number of apbuss words to create */
      if (device->dev_channel == OBSERVE)
	 formPTSwords((ptsvalue * 10.0), num, PTS1OFFSET, ptstype, device->frq_codes);
      else
	 formPTSwords((ptsvalue * 10.0), num, PTS2OFFSET, ptstype, device->frq_codes);
      device->codecnt = num;
   }
   else
   {
      iptr = device->frq_codes + device->codecnt;
      reg      = device->ap_reg;
      apadr    = device->ap_adr << 8;
      /* PTS320 is set 10 MHz lower then needed */
      if (ptstype == 320) ptsvalue -= 10e6;
      ptsvalue = ptsvalue * 10.0;	/* now in units of 1/10 Hz */
      div = 1.0e9;		/* start at 100 Mhz bcd digit */
      for (digit = 10; digit > 0; digit--)
      {
	 bcd = (int) (ptsvalue / div);
	 DPRINT3(DPRTLEVEL+1,"--> ptsvalue = %lf, digit=%d, bcd=%d\n",
		ptsvalue, digit, bcd);
	 if (digit % 2)
	 {
	    /* check for overranging PTS greater than or equal to 500 Mhz */
	    /* Since the PTS's double their frequency the overranging is  */
	    /* doubled.	 For overranging the 1 bit must be zero. For      */
	    /* underranging the 1 bit must be 1.			  */
	    tmpbcd = bcd;
	    if (device->overrange < 50000.0)	/* 10 Khz overrange */
	    {
	     if ((device->overunderflag != 0) && (ptstype >= 500)  &&
		(digit == 7) &&	(ptsfreq > (double)(ptstype/2)*1.0e6) )
	     {
		if (device->overunderflag == OVERRANGE)
		{
		   /* set 1 bit of the 1 MHz digit to zero */
		   if (bcd % 2) tmpbcd = bcd - 1;
		}
		if (device->overunderflag == UNDERRANGE)
		{
		   /* set 1 bit of the 1 MHz digit to one */
		   if ((bcd % 2) == 0) tmpbcd = bcd + 1;
		}
	     /* fprintf(stderr," digit = %d  bcd = %d\n",digit,tmpbcd); */
	     }
	    }
	    if ( (ptstype == 160) && (digit == 9) )
	    {			/* if pts160, translate the 2 bcd digit to 1
				 * hex digit */
	       bcdbcd = (bcdbcd >> 4) * 10;
	       bcdbcd += bcd;
	       bcdbcd &= 0x0f;
	    }
	    else
	    {
	       bcdbcd |= tmpbcd & 0xf;
	    }
            paired[digit/2]=(~bcdbcd) & 0x0ff;
	 }
	 else
	 {
	    /* check for overranging PTS greater than or equal to 500 Mhz */
	    /* Since the PTS's double their frequency the overranging is  */
	    /* doubled.	 For overranging the 1 bit must be zero. For      */
	    /* underranging the 1 bit must be 1.			  */
	    tmpbcd = bcd;
	    if (device->overrange > 50000.0)	/* 100 Khz overrange */
	    {
	     if ((device->overunderflag != 0) && (ptstype >= 500)  &&
		(digit == 8) &&	(ptsfreq > (double)(ptstype/2)*1.0e6) )
	     {
		if (device->overunderflag == OVERRANGE)
		{
		   /* set 1 bit of the 1 MHz digit to zero */
		   if (bcd % 2) tmpbcd = bcd - 1;
		}
		if (device->overunderflag == UNDERRANGE)
		{
		   /* set 1 bit of the 1 MHz digit to one */
		   if ((bcd % 2) == 0) tmpbcd = bcd + 1;
		}
	        /* fprintf(stderr," digit = %d  bcd = %d\n",digit,bcd); */
	     }
	    }
	    bcdbcd = (tmpbcd << 4) & 0xf0;
	 }
	 ptsvalue -= (double) bcd *div;

	 div /= 10.0;
      }
      if (device->rftype == DIRECTSYN)
      {
         *iptr++ = APSELECT | apadr | ((reg - 3) & 0xff);
         *iptr++ = APWRITE  | apadr | paired[1];
         *iptr++ = APPREINCWR  | apadr | paired[2];
         *iptr++ = APPREINCWR  | apadr | paired[3];
         *iptr++ = APPREINCWR  | apadr | paired[4];
         *iptr++ = APSELECT | apadr | ((reg - 4) & 0x0ff) ;
         *iptr++ = APWRITE  | apadr | paired[0];
         device->codecnt += 7;
      }
      else /* DIRECT_NON_DBL */
      {    /* Note that this starts with PREINCWR, it assunes that 
	    * calc_nondbl_Offset() has already done the first 2 AP bus words
            */
         *iptr++ = APPREINCWR  | apadr | paired[0];
         *iptr++ = APWRITE     | apadr | paired[1];
         *iptr++ = APWRITE     | apadr | paired[2];
         *iptr++ = APWRITE     | apadr | paired[3];
         *iptr++ = APWRITE     | apadr | paired[4];
         device->codecnt += 5;
      }
   }
}

/*---------------------------------------------------------------
| calcfixdoffset() - calculate setting for fixed frequncy device
|			Author: Greg Brissey  7/14/89
|  Modified 7/17/90
|	           No Longer round up to the larger of the tof,dof stepsize
|		   round to stepsize of channel
|		   device->freq_stepsize now contains the actual stepsize
+---------------------------------------------------------------*/
static void calcfixedoffset(Freq_Object *device)
{
   double          specfreq;
   double          baseMHz;

   device->spec_freq = round_freq(device->base_freq, device->offset_freq,
                                  device->init_offset_freq,
                                  device->freq_stepsize);
   if (bgflag)
   {
     fprintf(stderr,
     "calcfixedoffset: %11.2lf(SpecFreq) = %11.2lf(Base) + (%11.2lf(offset) - %11.2f(init_offset)\n",
       device->spec_freq, device->base_freq*1e6, device->offset_freq, device->init_offset_freq);
     fprintf(stderr, "calcfixedoffset: Channel stepsize= %11.2lf\n", device->freq_stepsize);
   }

   specfreq = device->spec_freq * 1e-6;   /* MHz */

   baseMHz = specfreq - device->ofsyn_constfrq;	/* back calc new base freq */

   if (bgflag)
      fprintf(stderr,
	      "fixsyn: %11.7lf(base) = %11.7lf(SpecFreq) - %6.2lf(const_freq)\n",
	      baseMHz, specfreq, device->ofsyn_constfrq);


/*-------------------------------------------------------------------------*/

   baseMHz = round_freq(baseMHz, (double) 0.0, (double) 0.0, device->freq_stepsize);
   device->of_freq = baseMHz;

   baseMHz = (baseMHz * 10.0 + 0.45);
   /* actually round this one! */
   /* eg. just to be sure 30.0 = 30 */

   if (bgflag)
      fprintf(stderr,
	      "fixsyn(): tbo = %11.2lf, tof= %7.2lf, freq=%11.7lf\n",
	      baseMHz, device->offset_freq, specfreq);

   formXLwords(baseMHz, 8, 0, device->dev_channel, device->frq_codes);
   device->codecnt = 8;
}
/*----------------------------------------------------------------------
|  initswpfreq() - calc starting Frequncy  & PTS value for swept frequency
|			Author: Greg Brissey  10/24/90
+----------------------------------------------------------------------*/
static int initswpfreq(Freq_Object *device)
{
   int i,idx,cnt,mask,highband;
   double lbandmax_freq,dtmp;
   double maxptswidth,maxbandwidth;
   unsigned int ltmp;
   Freq_Object    swpobj;	/* temporary sweep freq object */
   Freq_Object    *swpptr;

   /*bcopy(device,&swpobj,sizeof(Freq_Object));   copy Object info */
   memcpy( &swpobj, device, sizeof(Freq_Object));  /* copy Object info */
   swpptr = &swpobj;

   /* start freq = center - 1/2 width */
   swpptr->base_freq = device->sweepcenter - (device->sweepwidth/2.0e6); 
   swpptr->offset_freq = 0.0;
   swpptr->init_offset_freq = 0.0;
   calcdirectOffset(&swpobj);


   /* determine if trans freq is in high band or low band */
   if ( (swpptr->rftype != DIRECTSYN) && (device->rftype != DIRECT_NON_DBL) )
   {
     text_error("%s : Only Direct synthesis RF supported.\n", device->objname);
     return(ERROR_ABORT);
   }

   maxptswidth = maxbandwidth = device->maxswpwidth;

   /* do following if in absolute mode not rt parameter */
   if (device->sweeprtptr < 1)
   {
      /* determine if trans freq is in high band or low band */
      lbandmax_freq = ((double) swpptr->ptsval) - swpptr->iffreq; /* MHz */
      highband = select_rfband(swpptr->rfband, 
			    swpptr->spec_freq * 1.0E-6, lbandmax_freq);
      if (highband && device->rftype!=DIRECT_NON_DBL)
      {
        device->sweepincr /= 2.0; /* pts incr/2.0, since trans freq = pts*2 */
        maxptswidth  = ( ( ((swpptr->ptsval * 2.0) - swpptr->iffreq)
			    - device->sweepcenter ) * 2.0) * 1.0e6; /*Hz*/
	maxbandwidth = ((device->sweepcenter - lbandmax_freq) * 2.0) * 1.0e6;
      }
      else
      {
        maxptswidth  = ((  device->sweepcenter - 1.0 ) * 2.0) * 1.0e6;
	maxbandwidth = ((lbandmax_freq - device->sweepcenter ) * 2.0) * 1.0e6;
      }

      /* start freq + sweep width >= max pts value then warning */
      if ( swpptr->pts_freq + (device->sweepincr * device->sweepnp) >= 
	   (device->ptsval * 1.0e6))
      {
         text_error(
           "Frequency sweep will exceed PTS, change center frequency or sweep width.\n");
         return(ERROR_ABORT);
      }
   }
   /* determine minimum sweep width base upon np, rfband and ptsval */
   if (maxptswidth < device->maxswpwidth)
   {
	device->maxswpwidth = maxptswidth;
   }
   else if (maxbandwidth < device->maxswpwidth)
   {
	device->maxswpwidth = maxbandwidth;
   }

   putcode(INITFREQ);
   /* must also determine if rt variable to be used. */
   mask = (device->sweeprtptr > 1) ? RTVAR_BIT : 0x0;
   mask |= (device->ptsval == 160) ? PTS160_BIT : 0x0;
   putcode( mask | (device->dev_channel & 0xf) );
   putcode( ((device->ap_adr << 8) & 0x0f00) | (device->ap_reg & 0xff));

   /* if pts 155223964.05  dtmp = 15.0 */
   dtmp = swpptr->pts_freq / 1.0e7; /*  15  100,10 Mhz */
   ltmp = (int) dtmp;
   putcode( ltmp);  /* 100,10 MHz */

   dtmp = swpptr->pts_freq - (double) (ltmp * 10000000);
   ltmp = (int) dtmp;
   putcode( (ltmp >> 16));  /* upper word 5223964 Hz */
   putcode( (ltmp & 0xffff)); /* lower word 5223964 Hz */
   
   cnt = swpptr->codecnt;
   if (swpptr->ptsoptions & (1 << LATCH_PTS))
   {
     for (i = 0, idx = cnt - 4; i < 4; i++)	/* bkup 4 acodes for latching */
     {
        putcode(swpptr->frq_codes[idx]);		/* output latching acodes */
     }
   }
   else
   {
     for (i = 0; i < 4; i++)
     {
        putcode(0);
     }
   }
   putcode(cnt - 1);
   for (i = 0; i < cnt; i++)
   {
      putcode(swpptr->frq_codes[i]);
   }
   return( 0 );
}
/*----------------------------------------------------------------------
|  incrswpfreq() - Increment Frequncy
|			Author: Greg Brissey  10/24/90
+----------------------------------------------------------------------*/
static int incrswpfreq(Freq_Object *device)
{
    putcode(INCRFREQ);
    putcode(device->dev_channel);
    if (device->sweeprtptr > 0)
      putcode(device->sweeprtptr);
    else
    {
      if ((unsigned int)device->sweepincr <= (unsigned int)0xffff)
      {
        putcode((int) (device->sweepincr + 0.5));
      }
      else
      {
        text_error(
        "%s: Sweep increment too large, decrease sweep width or increase np.\n",
	   device->objname);
        return(ERROR_ABORT);
      }
   }
   return( 0 );
}

/*-----------------------------------------------------------------------
|
| 	calcsisoffset()/3
|	Calculates the offset frequency of the modulator 'm' style RF.
|	Includes generating the acodes to set the offset synthesizer.  
|
|				M. Howitt 1/8/89
|------------  NOTE ------------------------------------------------
|	tbo & dbo are not used any more.
|	SIS does not use solvent or lock and therefore does not correct
|	for these.
+----------------------------------------------------------------------*/
#define MAXOFFSETSYN	1.60000000	/* maximum frequency in 0.1 Hz	*/
#define MINOFFSETSYN	1.40000000	/* minimum frequency in 0.1 Hz	*/

static void calcsisoffset(Freq_Object *device)
{
   int             cnt;
   int            baseFreq;	/* base freq */
   int            max_todostep;
   double          base_offset;	/* base freq, (i.e., tbo,dbo ) */
   double          specfreq;
   // double          lbandmax_freq;
   double          pts;		/* PTS syn value */


   max_todostep = device->freq_stepsize;

   // lbandmax_freq = ((double) device->ptsval) - device->iffreq;	/* MHz */

   device->spec_freq = round_freq(device->base_freq, device->offset_freq,
                                  device->init_offset_freq,
                                  device->freq_stepsize);
   if (bgflag)
   {
     fprintf(stderr,
     "calcsisoffset: %11.2lf(SpecFreq) = %11.2lf(Base) + (%11.2lf(offset) - %11.2f(init_offset)\n",
       device->spec_freq, device->base_freq*1e6, device->offset_freq, 
						device->init_offset_freq);
     fprintf(stderr, 
	"calcsisoffset: Channel stepsize= %11.2lf\n", device->freq_stepsize);
   }


   specfreq = device->spec_freq * 1e-6;   /* MHz */

   /* obtain exact pts value from SpecFreq */
   /* pts = specfreq + iffreq + offset_syn_constant; */
   pts = specfreq + device->ofsyn_constfrq + device->ofsyn_basefrq;

   if (bgflag)
   {
      fprintf(stderr,
      " %11.7lf(pts) = %7.3lf(constfrq) + %11.7lf(base) - %11.7lf(specfreq)\n",
	      pts, device->ofsyn_constfrq, device->ofsyn_basefrq, specfreq);
   }

   /* pts = (round exact pts to 100KHz) pts; PTS hardware good only to 100KHz */
   pts = (double) ((long) ((pts * 10.0) +.5)) / 10.0;

   if (bgflag)
      fprintf(stderr, "setsisbasefreq: %7.3lf(pts) rounded to 100KHz\n", pts);

/*----------------------------------------------------------------------*/
/* Calculate offset synthesizer base frequency				*/
/* tbo(or dbo) = MainPTS - sfrq - 20.5					*/
/*----------------------------------------------------------------------*/
   base_offset = pts - specfreq - device->ofsyn_constfrq;

   /*-------------------------------------------------------------------*/
   /* Check PTS frequency to see if it needs to be reset.  If different */
   /* see if offset can be expanded before changing pts freq		*/
   /*-------------------------------------------------------------------*/
   if (device->pts_freq != pts)
   {
      double ptsdiff,offset_syn;
      ptsdiff = device->pts_freq - pts;
      if (bgflag)
	fprintf(stderr,"calcsisoffset: adjust, ptsdiff=%7.3lf offset=%7.4lf\n",
							ptsdiff,base_offset);
      if ((ptsdiff <= 0.10) && (ptsdiff >= -0.10))
      {
	offset_syn = base_offset+ptsdiff;
	if (bgflag)
	  fprintf(stderr,"calcsisoffset: offset_syn =%10.6lf\n",offset_syn);

	/* check to make sure offset is within valid range */
	if ((offset_syn > MAXOFFSETSYN) | (offset_syn < MINOFFSETSYN))
	   text_error("Offset  setting out of range; resetting PTS.\n");
	else
	{
	   base_offset = offset_syn;
	   pts = device->pts_freq;
	}
      }
   }

   baseFreq = (int) ((base_offset * 1.0e7) +.005);
   /* set new tbo,dbo, convert to 1/10Hz */
   if (bgflag)
      fprintf(stderr, "offsetsyn: baseFreq = %d (1/10Hz)\n", baseFreq);

   /* round baseMHz (tbo,dbo) to the larger of the tof & dof stepsize */
   if (max_todostep > 1L)
      baseFreq = ((baseFreq / max_todostep) * max_todostep) +
	 (((baseFreq % max_todostep) >= (max_todostep / 2L)) ?
	  max_todostep : 0L);

   if (bgflag)
      fprintf(stderr, "offsetsyn:round to %d(.1Hz), baseFreq= %d(.1Hz)\n",
	      max_todostep, baseFreq);

   /* set new tbo,dbo, convert to 0.1Hz */
   base_offset = (double) baseFreq;

   device->of_freq = base_offset;

   /* actually round this one! */
   /* eg. just to be sure 30.0 = 30 */

   if (bgflag)
      fprintf(stderr,
	      "offsetsyn(): tbo = %11.2lf, tof= %7.2lf, freq=%11.7lf\n",
	      base_offset, device->offset_freq, specfreq);

/*----------------------------------------------------------------------*/
/* Set PTS (if needed) and offset synthesizer				*/
/*----------------------------------------------------------------------*/
   device->codecnt = 0;
   if (device->ptsoptions & (1 <<  SIS_PTS_OFFSETSYN))
   {
      int ifreq, mainpts;
      int setpts = FALSE;

      if (device->pts_freq != pts) setpts = TRUE;
      mainpts = (int)((pts*10.0) + 0.0005);
      ifreq = (int) (base_offset + 0.45);
      setSISPTS(mainpts,ifreq,device,setpts);	/* device->codecnt set 	*/
						/* in setSISPTS		*/
   }
   else /* Assumed to have varian offset synthesizer */ 
   {
      if (device->pts_freq != pts)
      {
         setPTS(pts, device);
         device->pts_freq = pts;
      }

      cnt = device->codecnt;
      formXLwords((base_offset + 0.45), 8, 0, device->dev_channel, 
						&(device->frq_codes[cnt]));
      device->codecnt += 8;
   }
   return;
}

/*----------------------------------------------------------------------*/
/* Program to drive PTS-Interface					*/
/* by Jerry Signer							*/
/*									*/
/*   Modified   Author     Purpose					*/
/*   --------   ------     -------					*/
/*    1/09/89   M. Howitt  1. Updated code to match obj oriented 	*/
/*			      changes to psg.				*/
/*----------------------------------------------------------------------*/
# define	ap_init 0xa000	/*AP S2 Phase*/
# define	ap_seq	0x9000	/*AP S1 Phase*/
# define	obspts	1
# define	obsofs	0
# define	dcplpts	3
# define	dcplofs	2
# define	ptsboar	7

int lngpow(int a, int b)
/* function to calculate power (pow)
*/
{
int respow = 1;
int II;
	if (b==0) return(respow);
	for (II=1; II <= b; II++) 
		respow= respow * 2;
	return(respow);
}

int ptsconv(int value)
/* conversion program to convert a int number (10 <= x =< 4999) into
   the form for the pts output. Returned value is of type long, BCD coded
   32bit coded
   word: 	
		bit	value
		16-19	100kHz
		20-23	1MHz
		24-27	10MHz
		28-31	100MHz
*/
{
int II;
int ptsbcd;
	{
		ptsbcd = 0;
		for (II=0; II<=3; II++) {
		ptsbcd = ptsbcd + (value % 10) * lngpow(2,(16+4*II));
			value = value / 10;
			}
	return(ptsbcd);
}}

int ptsofs(int ofsint)
/* subroutine to convert an integer number in Hz (offset) into a bcd AP
   Bus chip form. Returned value is int.
   The bits have to be set as followed:
		bits	value
		0-3	1Hz
		4-7	10Hz
		8-11	100Hz
		12-15	1kHz
		16-19	10kHz
		20-23	100kHz
		24	1MHz
		25	2MHz
*/
{
int II;
int ptsobcd = 0;
	{
		for (II=0; II<=6; II++) {
		ptsobcd = ptsobcd + (ofsint%10) * lngpow(2,(II*4));
		ofsint = ofsint / 10;
		}
		return(ptsobcd);
	}
}

/*base in 100KHz, offset in .1Hz*/
/*setoop: set offset or both: 0 only offset*/
/*1 Main PTS + Offset*/
/*structure - see freq_device.p */
static void setSISPTS(int base, int offset, Freq_Object *device, int setoop)
{
   int cnt;

    if (bgflag)
      fprintf(stderr, 
      "setSISPTS: mainpts = %d, ifreq = %d , device chl = %d, setpts = %d\n",
	      base , offset, device->dev_channel, setoop);
	cnt = device->codecnt;
	if (setoop == 1)
	{
		if (device->dev_channel == OBSERVE) {
		   apcodes(ptsboar,obspts,~ptsconv(base),
					&(device->frq_codes[cnt]));
		}
		if (device->dev_channel == DECOUPLER) {
		   apcodes(ptsboar,dcplpts,~ptsconv(base),
					&(device->frq_codes[cnt]));
		}
		device->codecnt += 5;
	}
	cnt = device->codecnt;
	if (device->dev_channel == OBSERVE) {
		apcodes(ptsboar,obsofs,ptsofs(offset/10),
					&(device->frq_codes[cnt]));
		}
	if (device->dev_channel == DECOUPLER) {
		apcodes(ptsboar,dcplofs,ptsofs(offset/10),
					&(device->frq_codes[cnt]));
		}
	device->codecnt += 5;
}

void apcodes(int boardadd, int breg, int longw, int *words)
{
int init, seq1, seq2, seq3, seq4;

init = ap_init | (boardadd << 8) | (breg << 2) | 0x0003;
seq1 = ap_seq | (boardadd << 8) | (longw & 0x000000ff);
seq2 = ap_seq | (boardadd << 8) | ((longw >> 8) & 0xff);
seq3 = ap_seq | (boardadd << 8) | ((longw >> 16) & 0xff);
seq4 = ap_seq | (boardadd << 8) | ((longw >> 24) & 0xff);
   if (bgflag)
      fprintf(stderr, "setSISPTSjerry: init = %x, seq1 = %x, seq2 = %x, seq3 = %x, seq4 = %x\n",
	      init,seq1,seq2,seq3,seq4);
*words++ = init;
*words++ = seq1;
*words++ = seq2;
*words++ = seq3;
*words++ = seq4;

return;
}	/*end setacodes*/

/* 1.0	10/7/87 */

/*-------------------------------------------------------------------
|
|	setlkdecfrq() 
|	set the lock decoupler transmitter  frequency 
|				Author Greg Brissey  2/20/97
|
+------------------------------------------------------------------*/

#ifdef PSG_LC	/* if not for PSG then dummy out this routine for acqi & Vnmr see below */

#define	TWO_24	16777216.0	/* 2 to_the_power 24 */
#define	MAXSTR	256
void setlkdecfrq(Freq_Object *device)
{
  int	ap_addr,ap_reg,itmp;
  double	tmpfreq,synthif,lockref;
  char	lockfreqtab[MAXSTR];		/* Filename for lockfreqtab */
  char	lksense;

  synthif = 0.0;
  lockref = 0.0;
  lksense = '\0';
  device->spec_freq = round_freq(device->base_freq, device->offset_freq,
                                  device->init_offset_freq,
                                  device->freq_stepsize);

  strcpy(lockfreqtab,systemdir); /* $vnmrsystem */
  strcat(lockfreqtab,"/nuctables/lockfreqtab");
  if (lockfreqtab_read(lockfreqtab,H1freq,&synthif,&lksense, &lockref) == 0)
  {
    if (bgflag)
    {
       fprintf(stderr,
	"lockfreqtab: synthif=%g lksense= %c lockref=%g\n",
			synthif,lksense,lockref);
    }
    if (lksense == '+')
      tmpfreq =  (((device->spec_freq*1.0e-6) + synthif) - lockref);
    else
      tmpfreq = (((device->spec_freq*1.0e-6) - synthif) - lockref);

    if (tmpfreq < 0.0) tmpfreq = (-1.0)*tmpfreq;

    if (bgflag)
        fprintf(stderr,"setlkdecfrq(): locksynth = %g \n",tmpfreq);
  }
  else
  {
	abort_message("PSG: No H1freq setting in lockfreqtab.\n");
  }
  /* ap_addr = 0xb<<8; ap_reg = 0xbc; */
  ap_reg    = device->ap_reg;
  ap_addr  = device->ap_adr << 8;
  itmp =  ( (((int)(tmpfreq*TWO_24 + 0.5))/40) );
  device->frq_codes[0]=APSELECT   | ap_addr | ap_reg;
  device->frq_codes[1]=APWRITE    | ap_addr | ( (itmp    ) & 0xff);
  device->frq_codes[2]=APPREINCWR | ap_addr | ( (itmp>>8 ) & 0xff);
  device->frq_codes[3]=APPREINCWR | ap_addr | ( (itmp>>16) & 0xff);
  device->codecnt = 4;
  if (bgflag)
  {
     fprintf(stderr,">>>> setlkdecfrq: frq_codes[0]: 0x%x\n",
	device->frq_codes[0]);
     fprintf(stderr,">>>> setlkdecfrq: frq_codes[1]: 0x%x\n",
	device->frq_codes[1]);
     fprintf(stderr,">>>> setlkdecfrq: frq_codes[2]: 0x%x\n",
	device->frq_codes[2]);
     fprintf(stderr,">>>> setlkdecfrq: frq_codes[3]: 0x%x\n",
	device->frq_codes[3]);
  }
}

#else 

void setlkdecfrq(Freq_Object *device)
{
}

#endif 


