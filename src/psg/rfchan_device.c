/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  All Channel functions are in this file  */
/*-------------------------------------------------------------
| rfchan_device.c - RF Channel functions are referenced in this file
|                       Author: Greg Brissey  12/07/89
|   Modified   Author     Purpose
|   --------   ------     -------
|
+-------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "rfconst.h"
#include "acodes.h"
#include "oopc.h"

#include "acqparms.h"
#include "aptable.h"
#include "macros.h"
#include "abort.h"

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

extern char    *ObjError(), *ObjCmd();
extern int SetAttnAttr(Object attnobj, ...);
extern int attr_valtype(int attribute);
extern void HSgate(int ch, int state);
extern void settable90(int device, char arg[]);
extern int Device();
extern int	ap_interface;
extern int 	SkipHSlineTest;

#define DPRTLEVEL 1

#define NO_TABLE_USE 0
#define OK_TO_USE_TABLE 1
#define RTVAL 1
#define ABSVAL 0

typedef struct
{
#include "rfchan_device.p"
}               RFChan_Object;

/* bad form to cheat like this */
typedef struct
{
#include "freq_device.p"
}               Freq_Object;

/* base class dispatcher */
#define Base(this,msg,par,res)    Device(this,msg,par,res)

extern int      curfifocount;
extern int      presHSlines;
extern int      fifolpsize;

static void setphase90();
static void setphase();
static int init_attr();
static int set_attr();
static int get_attr();
static void setlkdecphase90(int device, int value);
static void setattn(Object obj, Msg_Set_Param *param, int use_table,
                    Msg_Set_Result *result, char *objname, int *ap_ovrride);
static void setlkdecphase90(int device, int value);

static int      hibandbitmask = 0;	/* dev_channel bit is set if in
					                 * highband */
static void set_sisunity_rfband(int setwhat, int value, c68int dev_channel);
static void select_sisunity_rfband(RFChan_Object *this);
static void set_xmtrx2bit(Object obj, RFChan_Object *this);
static void set_mixerbit(Object obj, RFChan_Object *this);
static void set_ampbandrelay(Object obj, RFChan_Object *this, double basefreq);

/*-------------------------------------------------------------
| RFChan_Device()/4 - Message Handler for RFChan_devices.
|                       Author: Greg Brissey  12/07/89
+-------------------------------------------------------------*/
int
RFChan_Device(RFChan_Object *this, Message msg, void *param, void *result)
{
   int             error = 0;

   switch (msg)
   {
      case MSG_NEW_DEV_r:
	 error = init_attr(result);
	 break;
      case MSG_SET_RFCHAN_ATTR_pr:
	 error = set_attr(this, param, result);
	 break;
      case MSG_GET_RFCHAN_ATTR_pr:
	 error = get_attr(this, param, result);
	 break;
      default:
	 error = Base(this, msg, param, result);
	 break;
   }
   return (error);
}

/*----------------------------------------------------------
|  init_attr()/1 - create a new RFChannel device structure
|                       Author: Greg Brissey
+----------------------------------------------------------*/
static int init_attr(Msg_New_Result *result)
{
   result->object = (Object) malloc(sizeof(RFChan_Object));
   if (result->object == (Object) 0)
   {
      fprintf(stderr, "Insuffient memory for New RF Channel Device!!");
      return (NO_MEMORY);
   }
   ClearTable(result->object, sizeof(RFChan_Object));	/* besure all clear */
   result->object->dispatch = (Functionp) RFChan_Device;
   return (0);
}

/*----------------------------------------------------------
| set_attr()/3 - static routine that has access to Channel device
|                attributes, address,register,bytes,mode
|                       Author: Greg Brissey  6/14/89
+----------------------------------------------------------*/
static int set_attr(RFChan_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;

   extern double   calcoffsetsyn();
   extern double   calcfixedoffset();

   result->resultcode = result->genfifowrds = result->reqvalue = 0;
   result->DBreqvalue = 0.0;
   switch (param->setwhat)
   {
      case SET_XMTRTYPE:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 /* this->newtrans =  *( (int *) (param->argptr)); */
	 this->newxmtr = (int) param->value;
	 break;
      case SET_AMPTYPE:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->newamp = param->value;
	 break;
      case SET_XMTR2AMPBDBIT:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->xmtr2ampbit = param->value;	/* relay bit for amp hi/low
						 * band */
	 break;
      case SET_XMTRX2BIT:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->xmtrx2bit = param->value;	/* relay bit for amp hi/low
						 * band */
	 break;
      case SET_AMPHIMIN:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);
	 this->amphibandmin = param->DBvalue;
	 break;
      case SET_FREQOBJ:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  0x%lx, for device: '%s'\n",
	 ObjCmd(param->setwhat), *((Object *) param->valptr), this->objname);
	 this->FreqObj = *((Object *) param->valptr);
	 break;
      case SET_ATTNOBJ:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  0x%lx, for device: '%s'\n",
	 ObjCmd(param->setwhat), *((Object *) param->valptr), this->objname);
	 this->AttnObj = *((Object *) param->valptr);
	 break;
      case SET_HS_OBJ:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  0x%lx, for device: '%s'\n",
	 ObjCmd(param->setwhat), *((Object *) param->valptr), this->objname);
	 this->HSObj = *((Object *) param->valptr);
	 break;
      case SET_MIXER_OBJ:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  0x%lx, for device: '%s'\n",
	 ObjCmd(param->setwhat), *((Object *) param->valptr), this->objname);
	 this->MixerObj = *((Object *) param->valptr);
	 break;
      case SET_MOD_OBJ:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  0x%lx, for device: '%s'\n",
	 ObjCmd(param->setwhat), *((Object *) param->valptr), this->objname);
	 this->ModulObj = *((Object *) param->valptr);
	 break;
      case SET_XGMODE_OBJ:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  0x%lx, for device: '%s'\n",
	 ObjCmd(param->setwhat), *((Object *) param->valptr), this->objname);
	 this->GateObj = *((Object *) param->valptr);
	 break;
      case SET_LNATTNOBJ:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  0x%lx, for device: '%s'\n",
	 ObjCmd(param->setwhat), *((Object *) param->valptr), this->objname);
	 this->LnAttnObj = *((Object *) param->valptr);
	 break;
      case SET_HLBRELAYOBJ:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  0x%lx, for device: '%s'\n",
	 ObjCmd(param->setwhat), *((Object *) param->valptr), this->objname);
	 this->HLBrelayObj = *((Object *) param->valptr);
	 break;
      case SET_XMTRX2OBJ:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  0x%lx, for device: '%s'\n",
	 ObjCmd(param->setwhat), *((Object *) param->valptr), this->objname);
	 this->XmtrX2Obj = *((Object *) param->valptr);
	 break;
      case SET_MOD_MODE:
         DPRINT1(DPRTLEVEL,"SET_MOD_MODE for %s\n",this->objname);
         param->setwhat = SET_MASK;
         error = Send(this->ModulObj, MSG_SET_AP_ATTR_pr, param, result);
         break;
      case SET_GATE_MODE:
         DPRINT1(DPRTLEVEL,"SET_MOD_MODE for %s\n",this->objname);
         param->setwhat = SET_MASK;
         error = Send(this->GateObj, MSG_SET_AP_ATTR_pr, param, result);
         break;
      case SET_HSSEL_FALSE:
         param->setwhat = SET_FALSE;
         error = Send(this->HSObj, MSG_SET_APBIT_MASK_pr, param, result);
	 break;
      case SET_HSSEL_TRUE:
         param->setwhat = SET_TRUE;
         error = Send(this->HSObj, MSG_SET_APBIT_MASK_pr, param, result);
	 break;
      case SET_HS_VALUE:
         param->setwhat = SET_VALUE;
         error = Send(this->HSObj, MSG_SET_AP_ATTR_pr, param, result);
	 break;
      case SET_MIXER_VALUE:
	 if (param->value < 0)
	 {
	    /* standard creation of acodes */
            param->setwhat = SET_VALUE;
            error = Send(this->MixerObj, MSG_SET_AP_ATTR_pr, param, result);
	 }
	 else
	 {
	    /* create acodes in global table given by param->value */
            param->setwhat = SET_GTAB;
            error = Send(this->MixerObj, MSG_SET_AP_ATTR_pr, param, result);
	 }
         break;
      case SET_MOD_VALUE:
         param->setwhat = SET_VALUE;
         error = Send(this->ModulObj, MSG_SET_AP_ATTR_pr, param, result);
         break;
      case SET_XGMODE_VALUE:
         param->setwhat = SET_VALUE;
         error = Send(this->GateObj, MSG_SET_AP_ATTR_pr, param, result);
         break;
      case SET_SPECFREQ:	/* sfrq or dfrq  */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);

	 /* set base frequency */
	 error = Send(this->FreqObj, MSG_SET_FREQ_ATTR_pr, param, result);
	 if (error < 0)
	 {
	    abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		    ObjCmd(param->setwhat));
	 }

	 set_ampbandrelay(this->HLBrelayObj, this,param->DBvalue);
	 if (this->XmtrX2Obj != (Object) NULL)
            set_xmtrx2bit(this->XmtrX2Obj, this);
	 break;

      case SET_INIT_OFFSET:	/* initial setting of tof or dof frequency */
      case SET_OFFSETFREQ:	/* subsequent setting of tof or dof frequency */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);

	 /* set base frequency */
	 error = Send(this->FreqObj, MSG_SET_FREQ_ATTR_pr, param, result);
	 if (error < 0)
	 {
	    abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		    ObjCmd(param->setwhat));
	 }

	 /* select the high or low band of the transmitter for SIS Unity*/
	 select_sisunity_rfband(this);

	 if (this->XmtrX2Obj != (Object) NULL)
	    set_xmtrx2bit(this->XmtrX2Obj, this);
	 if (this->MixerObj != (Object) NULL)
            set_mixerbit(this->MixerObj, this);
	 break;


      case SET_PTSOPTIONS:
      case SET_IFFREQ:
      case SET_RFTYPE:
      case SET_H1FREQ:
      case SET_RFBAND:
      case SET_FREQSTEP:
      case SET_OSYNBFRQ:
      case SET_OSYNCFRQ:
      case SET_OVRUNDRFLG:
      case SET_SWEEPCENTER:
      case SET_SWEEPWIDTH:
      case SET_SWEEPNP:
      case SET_SWEEPMODE:
      case SET_INITSWEEP:
      case SET_INCRSWEEP:
      case SET_OFFSETFREQ_STORE:
      case SET_FREQ_FROMSTORAGE:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);

	 error = Send(this->FreqObj, MSG_SET_FREQ_ATTR_pr, param, result);
	 if (error < 0)
	 {
	    abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		    ObjCmd(param->setwhat));
	 }
	 break;

      case SET_FREQVALUE:	/* generate acode for frequency */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);

	 param->setwhat = SET_VALUE;
	 error = Send(this->FreqObj, MSG_SET_FREQ_ATTR_pr, param, result);
	 if (error < 0)
	 {
	    abort_message("%s : %s  '%s'\n", this->FreqObj->objname, ObjError(error),
		    ObjCmd(param->setwhat));
	 }

	 /* If SIS unity system this will set rfband acodes */
	 set_sisunity_rfband(SET_VALUE,0,this->dev_channel);
	 break;

      case SET_FREQLIST:	/* gen acode for frequency table list */
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);

	 param->setwhat = SET_GTAB;
	 error = Send(this->FreqObj, MSG_SET_FREQ_ATTR_pr, param, result);
	 if (error < 0)
	 {
	    abort_message("%s : %s  '%s'\n", this->FreqObj->objname, ObjError(error),
		    ObjCmd(param->setwhat));
	 }
	 /* If SIS unity system this will set rfband acodes */
	 set_sisunity_rfband(SET_GTAB,param->value,this->dev_channel);
	 break;


      case SET_RTPWR:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 if (this->AttnObj != NULL)	/* no attenuator */
	 {
	    param->setwhat = SET_RTPARAM;	/* use attn object command */
	    setattn(this->AttnObj, param, OK_TO_USE_TABLE, result, this->objname,
		    this->ap_ovrride);
	 }
	 else
	 {
	    if (ix == 1)
	       fprintf(stdout,
	       "%s:  '%s' statement ignored due to system configuration.\n",
		       this->objname, ObjCmd(param->setwhat));
	    return(error);
	 }
	 break;

      case SET_RTPWRF:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 if (this->LnAttnObj != NULL)	/* no fine attenuator */
	 {
	    param->setwhat = SET_RTPARAM;	/* use attn object command */
	    setattn(this->LnAttnObj, param, OK_TO_USE_TABLE, result, this->objname,
		    this->ap_ovrride);
	 }
	 else
	 {
	    if (ix == 1)
	       fprintf(stdout,
	       "%s:  '%s' statement ignored due to system configuration.\n",
		       this->objname, ObjCmd(param->setwhat));
	    return(error);
	 }
	 break;

      case SET_PWR:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 if (this->AttnObj != NULL)	/* no coarse attenuator */
	 {
	    param->setwhat = SET_VALUE;	/* use attn object command */
	    setattn(this->AttnObj, param, NO_TABLE_USE, result, this->objname,
		    this->ap_ovrride);
	 }
	 else
	 {
	    if (ix == 1)
	       fprintf(stdout,
	       "%s:  '%s' statement ignored due to system configuration.\n",
		       this->objname, ObjCmd(param->setwhat));
	    return(error);
	 }
	 break;

      case SET_PWRF:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 if (this->LnAttnObj != NULL)	/* no fine attenuator */
	 {
	    param->setwhat = SET_VALUE;	/* use attn object command */
            if (param->value < 0)
               param->value = 0;
            else if (param->value > 4095)
               param->value = 4095;
	    setattn(this->LnAttnObj, param, NO_TABLE_USE, result, this->objname,
		    this->ap_ovrride);
	 }
	 else
	 {
	    if (ix == 1)
	       fprintf(stdout,
	       "%s:  '%s' statement ignored due to system configuration.\n",
		       this->objname, ObjCmd(param->setwhat));
	    return(error);
	 }
	 break;
      case SET_GATE_PHASE:
         {
	 putcode(APBOUT);
	 putcode(1);
	 putcode(APSELECT | 0xb00 | ( 0x92 + (this->dev_channel - 1) * 16)  );
	 putcode(APWRITE  | 0xb00 | param->value );
	 curfifocount += 2;
	 break;
	 }

      case SET_PHLINE0:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to 0x%x, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 this->phasetbl[0] = param->value;
	 break;

      case SET_PHLINE90:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to 0x%x, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 this->phasetbl[1] = param->value;
	 break;

      case SET_PHLINE180:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to 0x%x, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 this->phasetbl[2] = param->value;
	 break;

      case SET_PHLINE270:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to 0x%x, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 this->phasetbl[3] = param->value;
	 break;
      case SET_PHASEBITS:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to 0x%x, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 this->phasebits = param->value;
	 break;

      case SET_PHASESTEP:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %lf, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->DBvalue, this->objname);

	 if (!this->newxmtr)
	 {
	    abort_message("%s: requires direct synthesis RF on %s\n",
		    ObjCmd(param->setwhat), this->objname);
	 }
	 notinhwloop("stepsize");
         if (ap_interface < 4)
	    this->phasestep = (int) ((param->DBvalue * 2.0) + .50005);
         else
	    this->phasestep = (int) ((param->DBvalue * 4.0) + .50005);
	 putcode(PHASESTEP);
	 putcode(this->dev_channel);	/* TRans = 1, Dec = 2 */
	 putcode(this->phasestep);
	 break;

      case SET_PHASEAPADR:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 this->sphaseaddr = param->value;
	 break;

      case SET_PHASE_REG:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->sphasereg = param->value;
	 break;

      case SET_PHASE_MODE:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 this->sphasemode = param->value;
	 break;

      case SET_PHASEATTR:
	 DPRINT2(DPRTLEVEL,
		 "set_attr: Cmd: '%s' for device: '%s'\n",
		 ObjCmd(param->setwhat), this->objname);
	 putcode(SETPHATTR);	/* new acode */
	 putcode(this->dev_channel);

         {
	    putcode( (((this->phasetbl[0]) >> 16) & 0xffff) );
	    putcode( ((this->phasetbl[0]) & 0xffff) );
	    putcode( (((this->phasetbl[1]) >> 16) & 0xffff) );
	    putcode( ((this->phasetbl[1]) & 0xffff) );
	    putcode( (((this->phasetbl[2]) >> 16) & 0xffff) );
	    putcode( ((this->phasetbl[2]) & 0xffff) );
	    putcode( (((this->phasetbl[3]) >> 16) & 0xffff) );
	    putcode( ((this->phasetbl[3]) & 0xffff) );
	    putcode( (((this->phasebits)>>16) & 0xffff) );
	    putcode( ((this->phasebits) & 0xffff) );
         }
	 putcode(this->sphaseaddr);
         if (ap_interface == 4) putcode(this->sphasereg);
	 break;

      case SET_RTPHASE90:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

         if (this->newxmtr == TRUE)  /* not for lock/decoup brd this->newxmtr == TRUE+1 */
         {
	   setphase90(this->dev_channel, param->value);
         }
         else if (this->newxmtr == TRUE+1)
         {
	   setlkdecphase90(this->dev_channel, param->value);
         }
	 break;

      case SET_PHASE90:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %s, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->valptr, this->objname);

         if (param->valptr[0] != '\0')
	    settable90(this->dev_channel, param->valptr);
	 break;

      case SET_RTPHASE:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 /* Check RF consistency. */
	 if (!this->newxmtr)
	 {
	    abort_message("%s: requires direct synthesis RF on %s\n",
		    ObjCmd(param->setwhat), this->objname);
	 }

         if (this->newxmtr == TRUE)  /* not for lock/decoup brd this->newxmtr == TRUE+1 */
         {
	   setphase(this->dev_channel, param->value, RTVAL, OK_TO_USE_TABLE,
		  this->ap_ovrride,this->sphasemode);
         }
	 break;

      case SET_PHASE:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 /* Check RF consistency. */
	 if (!this->newxmtr)
	 {
	    abort_message("%s: requires direct synthesis RF on %s\n",
		    ObjCmd(param->setwhat), this->objname);
	 }

	 setphase(this->dev_channel, param->value, ABSVAL, NO_TABLE_USE,
		  this->ap_ovrride,this->sphasemode);
	 break;

      case SET_APOVRADR:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to 0x%lx, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->valptr, this->objname);

	 this->ap_ovrride = ((int *) param->valptr);
	 break;

      case SET_HSLADDR:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to 0x%lx, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->valptr, this->objname);

	 this->HSlineptr = ((int *) param->valptr);
	 break;

      case SET_RCVRGATEBIT:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to 0x%x, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 /* HSline bit to gate xmtr with */
	 this->rcvrgateHSbit = param->value;	/* create bit mask for gate */
	 break;

      case SET_XMTRGATEBIT:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to 0x%x, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 /* HSline bit to gate xmtr with */
	 this->xmtrgateHSbit = param->value;	/* create bit mask for gate */
	 break;

      case SET_RCVRGATE:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);
	 /* Receiver gate is inverted polarity */
	 if (param->value)
	    HSgate(this->rcvrgateHSbit,FALSE);
	 else
	    HSgate(this->rcvrgateHSbit,TRUE);

	 break;

      case SET_XMTRGATE:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 if (param->value)
	    HSgate(this->xmtrgateHSbit,TRUE);
	 else
	    HSgate(this->xmtrgateHSbit,FALSE);

         if (this->newxmtr > TRUE )  /* (this->newxmtr == TRUE+1). i.e. lock/dec rftype  */
         {
           /* lock/dec on/off must be handled in the console because the lockpower must be
              re-established, since the decoupler power will effect the lock power from
              altering the 20 db atten in the magnet leg
           */
           /* bit 0 - LO 0=on,1=off (off for decoupling */
           /* bit 1 - Lk/Dec Xmtr Gate 0=off,1=on */
           /* bit 7 - Lk/Dec Decoupler enable 1=on 0=off */
	   Msg_Set_Param  decparam;
	   Msg_Set_Result decresult;
           int error;
	   int apadr,apreg;
           
	  decparam.setwhat = GET_ADR;
	  error = Send(this->HSObj, MSG_GET_AP_ATTR_pr, &decparam, &decresult);
	  if (error < 0)
	  {
	    abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		    ObjCmd(param->setwhat));
	  }
          apadr = decresult.reqvalue;

	  decparam.setwhat = GET_REG;
   	  decresult.reqvalue = 0;
	  error = Send(this->HSObj, MSG_GET_AP_ATTR_pr, &decparam, &decresult);
	  if (error < 0)
	  {
	    abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		    ObjCmd(param->setwhat));
	  }
          apreg = decresult.reqvalue;
	   if (param->value)
	   {
     	      /* turn on Lk/Dec Decoupler Xmtr Gate */
              putcode((c68int) LOCKDEC_ON_OFF);
	      putcode((c68int) (apadr << 8) | (0xff & apreg));
              putcode((c68int) param->value);	/* 1 = On */
              putcode((c68int) 32);	/* 32 = STD AP delay 400 nsec  */

#ifdef XXX
              decparam.setwhat = SET_MASK;
              decparam.value = 0x83;       /* 0x80 dec enable, 2 - Dec Xmtr Gate On, 1 LO off */
              error = Send(this->HSObj, MSG_SET_AP_ATTR_pr, &decparam, &decresult);
              decparam.setwhat = SET_VALUE;
              error = Send(this->HSObj, MSG_SET_AP_ATTR_pr, &decparam, &decresult);
#endif

	      if ( (ix == 1) && (bgflag) )
                fprintf(stderr,"lock/dec: Turn-On Xmtr: apaddr:  0x%x\n",(apadr << 8) | (0xff & apreg));
	   }
	   else 
	   {
     	      /* turn Off Lk/Dec Decoupler Xmtr Gate */
              putcode((c68int) LOCKDEC_ON_OFF);
	      putcode((c68int) (apadr << 8) | (0xff & apreg));
              putcode((c68int) param->value);	/* 1 = On */
              putcode((c68int) 32);	/* 32 = STD AP delay 400 nsec  */

#ifdef XXX
              decparam.setwhat = SET_MASK;
              decparam.value = 0x01;      /* 0x00 dec disable, 1 - Dec Xmtr Gate Off, LO off */
              error = Send(this->HSObj, MSG_SET_AP_ATTR_pr, &decparam, &decresult);
              decparam.setwhat = SET_VALUE;
              error = Send(this->HSObj, MSG_SET_AP_ATTR_pr, &decparam, &decresult);
#endif

	      if ( (ix == 1) && (bgflag) )
                fprintf(stderr,"lock/dec: Turn-Off Xmtr: apaddr:  0x%x\n",(apadr << 8) | (0xff & apreg));
	   }
	 }
	 break;

      case SET_WGGATEBIT:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to  %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 /* HSline bit to gate wg with */
	 this->wg_gateHSbit = param->value;	/* relay bit for amp hi/low
						 * band */
	 break;

      case SET_WGGATE:
	 DPRINT3(DPRTLEVEL,
		 "set_attr: Cmd: '%s' to %d, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 if (param->value)
	    HSgate(this->wg_gateHSbit,TRUE);
	 else
	    HSgate(this->wg_gateHSbit,FALSE);

	 break;
      default:
	 error = UNKNOWN_RFCHAN_ATTR;
	 result->resultcode = UNKNOWN_RFCHAN_ATTR;
	 break;
   }
   return (error);
}

/*----------------------------------------------------------
| get_attr()/3 - static routine that has access to RF Chan device
|                       Author: Greg Brissey  12/07/89
+----------------------------------------------------------*/
static int get_attr(RFChan_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;
   Msg_Set_Param   forAttn;	/* Used to get current attenuator value */
   Msg_Set_Result  xresult;	/* Used to get maximum attenuator value */

   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case GET_XMTRTYPE:
	 result->reqvalue = this->newxmtr;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %d for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
	 break;
      case GET_AMPTYPE:	/* obsserve, decoupler, etc */
	 result->reqvalue = this->newamp;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is %d for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
	 break;

      case GET_PHASEBITS:
	 result->reqvalue = this->phasebits;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is 0x%x for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
	 break;

      case GET_HIBANDMASK:	/* obsserve, decoupler, etc */
	 result->reqvalue = hibandbitmask;	/* static param */
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is 0x%x for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
	 break;

      case GET_AMPHIBAND:	/* amplifier band for this channel */
	 result->reqvalue = (hibandbitmask & (1 << this->dev_channel)) ?
			     1 : 0;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is 0x%x for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
	 break;

      case GET_XMTRGATE:
	 result->reqvalue = *this->HSlineptr;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s', is 0x%x for device: '%s'\n",
		 ObjCmd(param->setwhat), result->reqvalue, this->objname);
	 break;

      case GET_XMTRGATEBIT:	/* HSline bit to gate xmtr with */
	 result->reqvalue = this->xmtrgateHSbit;
	 DPRINT3(DPRTLEVEL,
		 "get_attr: Cmd: '%s' to 0x%x, for device: '%s'\n",
		 ObjCmd(param->setwhat), param->value, this->objname);

	 break;

/*  Respond to these requests by passing them to the Frequency Object
    that is part of each RF Channel Object.				*/

      case GET_OF_FREQ:
      case GET_PTS_FREQ:
      case GET_SPEC_FREQ:
      case GET_BASE_FREQ:
      case GET_RFBAND:
      case GET_FREQSTEP:
      case GET_SWEEPCENTER:
      case GET_SWEEPWIDTH:
      case GET_SWEEPNP:
      case GET_SWEEPINCR:
      case GET_SWEEPMODE:
      case GET_SWEEPMAXWIDTH:
      case GET_LBANDMAX:
      case GET_MAXXMTRFREQ:
	 DPRINT2(DPRTLEVEL,
		 "get_attr: Cmd: '%s' is forwarded  to device: '%s'\n",
		 ObjCmd(param->setwhat), this->FreqObj->objname);

	 error = Send(this->FreqObj, MSG_GET_FREQ_ATTR_pr, param, result);
	 if (error < 0)
	 {
	    abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		    ObjCmd(param->setwhat));
	 }
	 break;

/*  Respond to this request by asking the Attenuator Object for its value.  */
/*  Find out its maximum value, if 127 or 255 (for 255 high bit is rf	    */
/*  band bit) divide value by 2 and round up.				    */

      case GET_PWR:
	 forAttn.setwhat = GET_VALUE;
	 error = Send(this->AttnObj, MSG_GET_ATTN_ATTR_pr, &forAttn, result);
	 if (error < 0)
	 {
	    abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		    ObjCmd(param->setwhat));
	 }
	 forAttn.setwhat = GET_MAXVAL;
   	 xresult.reqvalue = 0;
	 error = Send(this->AttnObj, MSG_GET_ATTN_ATTR_pr, &forAttn, &xresult);
	 if (error < 0)
	 {
	    abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		    ObjCmd(param->setwhat));
	 }
	 if ((xresult.reqvalue == SIS_UNITY_ATTN_MAX) ||
		(xresult.reqvalue == 255) )
	 {
	    result->reqvalue = (result->reqvalue/2) + (result->reqvalue%2);
	 }
	 break;

      case GET_PWRF:
	 if (this->LnAttnObj != NULL)	/* no fine attenuator */
         {
	    forAttn.setwhat = GET_VALUE;
	    error = Send(this->LnAttnObj, MSG_GET_ATTN_ATTR_pr,
                          &forAttn, result);
	    if (error < 0)
	    {
	       abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		    ObjCmd(param->setwhat));
	    }
         }
         else
            error = -1;

	 break;
      default:
	 error = UNKNOWN_RFCHAN_ATTR;
	 result->resultcode = UNKNOWN_RFCHAN_ATTR;
   }
   return (error);
}

/*---------------------------------------------------------------------
| SetRFChanAttr  -  Set Channel Device to List of Attributes
|                       Author: Greg Brissey  8/18/88
+---------------------------------------------------------------------*/
/*VARARGS2*/
int SetRFChanAttr(Object obj, ...)
{
   va_list         vargs;
   int             error = 0;
   int             error2 = 0;
   Msg_Set_Param   param;
   Msg_Set_Result  result;

   va_start(vargs, obj);
   /* Options allways follow the format 'Option,(value),Option,(value),etc' */
   while ((param.setwhat = va_arg(vargs, int)) != 0)
   {
      switch (attr_valtype(param.setwhat))
      {
	 case NO_VALUE:
	    break;
	 case INTEGER:
	    param.value = (int) va_arg(vargs, int);
	    break;
	 case DOUBLE:
	    param.DBvalue = (double) va_arg(vargs, double);
	    break;
	 case POINTER:
	    param.valptr = (char *) va_arg(vargs, long);
	    break;
	 default:
	    abort_message("Value Type unknown for Object.");
      }
      error = Send(obj, MSG_SET_RFCHAN_ATTR_pr, &param, &result);
      if (error < 0)
      {
	 error2 = Send(obj, MSG_SET_DEV_ATTR_pr, &param, &result);
	 if (error2 < 0)
	 {
	    abort_message("%s : %s  '%s'\n", obj->objname, ObjError(error),
		    ObjCmd(param.setwhat));
	 }
      }
   }
   va_end(vargs);
   return (error);
}

/*----------------------------------------------------------------------
|  setattn()/6  -  generic routine to set an attenutor through its Object
+-----------------------------------------------------------------------*/
static void setattn(Object obj, Msg_Set_Param *param, int use_table,
                    Msg_Set_Result *result, char *objname, int *ap_ovrride)
{
   int             error;

   okinhwloop();

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   if ( fifolpsize < 100 )
   {
     if (*ap_ovrride)
     {
      *ap_ovrride = 0;
     }
     else
     {
      G_Delay(DELAY_TIME, 0.2e-6, 0);	/* allows high-speed lines to be set
					 * prior to addressing the AP bus.
					 * S.F. */
     }
   }

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   /*********************************************
   *  Set up table access for power statement.  *
   *********************************************/

   if (use_table == OK_TO_USE_TABLE)
   {
      if ((param->value >= t1) && (param->value <= t60))
      {
	 param->value = tablertv(param->value);
      }
   }

   error = Send(obj, MSG_SET_ATTN_ATTR_pr, param, result);
   if (error < 0)
   {
      abort_message("%s : %s  '%s'\n", objname, ObjError(error),
	      ObjCmd(param->setwhat));
   }
   return;
}

/*----------------------------------------------------------------------
|  setphase90()/4  -  generic routine to set phase quadrent
+-----------------------------------------------------------------------*/
static void
setphase90(device, value)
int             device;
int             value;
{

   okinhwloop();		/* legal in hardware loop */

   /*********************************************
   *  Set up table access for phase statement.  *
   *********************************************/

   if ((value >= t1) && (value <= t60))
   {
       value = tablertv(value);
   }

   putcode(SETPHAS90);	/* replaces TXPHASE, DECPHASE */
   putcode(device);
   putcode(value);

   return;
}

/*----------------------------------------------------------------------
|  setlkdecphase90()/4  -  routine to set phase quadrent of lock/decouper brd
+-----------------------------------------------------------------------*/
static void setlkdecphase90(int device, int value)
{

   okinhwloop();		/* legal in hardware loop */

   /*********************************************
   *  Set up table access for phase statement.  *
   *********************************************/

   if ((value >= t1) && (value <= t60))
   {
       value = tablertv(value);
   }

   putcode(LOCKDECPHS90);	/* replaces TXPHASE, DECPHASE */
   putcode(device);
   putcode(value);

   return;
}

/*----------------------------------------------------------------------
|  setphase()/5  -  generic routine to set small angle & quadrent phase
+-----------------------------------------------------------------------*/
static void
setphase(device, value, rtflag, use_table, ap_ovrride, mode)
int             device;
int             value;
int             rtflag;
int             use_table;
int            *ap_ovrride;
int             mode;
{
int	flags;
   okinhwloop();		/* legal in hardware loop */

/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   if ( fifolpsize < 100 )	/* Just for old OB board */
   {
     if (*ap_ovrride)
     {
      *ap_ovrride = 0;
     }
     else
     {
      G_Delay(DELAY_TIME, 0.2e-6, 0);	/* allows high-speed lines to be set
					 * prior to addressing the AP bus.
					 * S.F. */
     }
   }
/* ++++++++++++++++++++++++++++++++++++++++++++++++++++++++ */

   /*********************************************
   *  Set up table access for phase statement.  *
   *********************************************/

   flags = 0x0;
   if (use_table == OK_TO_USE_TABLE)
   {
      if ((value >= t1) && (value <= t60))
      {
	 value = tablertv(value);
      }
   }
   else
      flags = 0x0100;
   if (rtflag) flags = 0x0;	/* rt value */
   if (mode == 2)
      flags |= 0x0200;

   putcode(SETPHASE);
   putcode((device) | flags);
   putcode(value);
   if (mode == 2)
     curfifocount += 3;		/* Unity+ gets 3 fifo words */
   else
     curfifocount++;		/* Unity gets 1, increment current fifo count */
   return;
}

/*-----------------------------------------------------
| set transmitter to Amplifier Band Relay
+-----------------------------------------------------*/
static void set_ampbandrelay(Object obj, RFChan_Object *this, double basefreq)
{
   int error = 0;
   Msg_Set_Param   chparam;
   Msg_Set_Result  chresult;

   if (obj != NULL)
   {
      /* based on the base frequency set routing relay to proper band of Amp. */
      /* For Hydra this is double checked in initfunc.c set_ampbit() */
      chparam.setwhat = (basefreq > this->amphibandmin) ?
	 SET_FALSE : SET_TRUE;
      chparam.value = this->xmtr2ampbit; /* OBS_RELAY, DEC_RELAY, DEC2_RELAY */
					 /* HILO_CH1,  HILO_CH2,  HILO_CH3   */
      error = Send(obj, MSG_SET_APBIT_MASK_pr, &chparam, &chresult);
      if (error < 0)
      {
	     abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		               ObjCmd(chparam.setwhat));
      }
      /* set global channel high band bit mask */
      if (chparam.setwhat == SET_FALSE)
	 hibandbitmask |= (1 << this->dev_channel);
      else
	 hibandbitmask &= ~(1 << this->dev_channel);
   }
}

/*-----------------------------------------------------
| set transmitter mixer relay 
+-----------------------------------------------------*/
static void set_mixerbit(Object obj, RFChan_Object *this)
{
int		error = 0;
Msg_Set_Param	chparam;
Msg_Set_Result	chresult;

   if (obj != NULL)
   {  chparam.setwhat = GET_SPEC_FREQ;
      error = Send(this->FreqObj, MSG_GET_FREQ_ATTR_pr, &chparam, &chresult);
      if (error < 0)
      {
	     abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		               ObjCmd(chparam.setwhat));
      }

      /* base on the rf band of the frequency set xmtr board doubling line. */
      chparam.setwhat = SET_MASK;
      chparam.value   = (chresult.DBreqvalue > 1.5e8 ) ? 0x80 : 0x0;
      error = Send(obj, MSG_SET_AP_ATTR_pr, &chparam, &chresult);
      if (error < 0)
      {
	     abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		               ObjCmd(chparam.setwhat));
      }
   }
}
/*-----------------------------------------------------
| set transmitter to Amplifier Band Relay
+-----------------------------------------------------*/
static void set_xmtrx2bit(Object obj, RFChan_Object *this)
{
   int error = 0;
   Msg_Set_Param   chparam;
   Msg_Set_Result  chresult;

   if (obj != NULL)
   {
      chparam.setwhat = GET_RFBAND;
      error = Send(this->FreqObj, MSG_GET_FREQ_ATTR_pr, &chparam, &chresult);
      if (error < 0)
      {
	     abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		               ObjCmd(chparam.setwhat));
      }

      /* base on the rf band of the frequency set xmtr board doubling line. */
      chparam.setwhat = (chresult.reqvalue == RF_HIGH_BAND) ? 
			 SET_TRUE : SET_FALSE;
      chparam.value = this->xmtrx2bit;	/* OBS_XMTR_HI_LOW_BAND,DEC_..,DEC2_..
					 *  */
      error = Send(obj, MSG_SET_APBIT_MASK_pr, &chparam, &chresult);
      if (error < 0)
      {
	     abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		                ObjCmd(chparam.setwhat));
      }
   }
}

/*-----------------------------------------------------------------------
| For SIS Unity Systems
|      select_sisunity_rfband 
|		   - Selects a high or low band  
|                    for either the observe or decouple transmitter.
|                  - The setting of the band is done in the same apbus
|                    location as the signal attenuation is selected.
|                    The signal attenuation is in the lower 7 bits the
|                    rf band selection is in the msb.
|                  - Increments frq_codes by ??;
+------------------------------------------------------------------------*/
static void select_sisunity_rfband(RFChan_Object *this)
{
   int error = 0;
   int band_select;
   Msg_Set_Param   chparam;
   Msg_Set_Result  chresult;

   if ((cattn[this->dev_channel] == SIS_UNITY_ATTN_MAX) && 
    ((rftype[this->dev_channel-1] == 'c') || 
				(rftype[this->dev_channel-1] == 'C')))
   {
      chparam.setwhat = GET_RFBAND;
      error = Send(this->FreqObj, MSG_GET_FREQ_ATTR_pr, &chparam, &chresult);
      if (error < 0)
      {
	      abort_message("%s : %s  '%s'\n", this->objname, ObjError(error),
		                ObjCmd(chparam.setwhat));
      }
      band_select = (chresult.reqvalue == RF_HIGH_BAND) ? 
			 1 : 0;

      if (this->dev_channel == OBSERVE) 
	 SetAttnAttr(ToAttn, SET_RFBAND, band_select,NULL);
      if (this->dev_channel == DECOUPLER) 
	 SetAttnAttr(DoAttn, SET_RFBAND, band_select,NULL);
   }
}

/*-----------------------------------------------------------------------
| For SIS Unity Systems
|      set_sisunity_rfband 
|		   - Creates the apwords to select a high or low band.  
|                  - The setting of the band is done in the same apbus
|                    location as the signal attenuation is selected.
|                    The signal attenuation is in the lower 7 bits the
|                    rf band selection is in the msb.
|                  - Increments frq_codes by ??;
+------------------------------------------------------------------------*/
static void set_sisunity_rfband(int setwhat, int value, c68int dev_channel)
{
   if ((cattn[dev_channel] == SIS_UNITY_ATTN_MAX) && 
	((rftype[dev_channel-1] == 'c') || (rftype[dev_channel-1] == 'C')) )
   {
	if (dev_channel == OBSERVE) 
	   SetAttnAttr(ToAttn, setwhat, value,NULL);
	if (dev_channel == DECOUPLER) 
	   SetAttnAttr(DoAttn, setwhat, value,NULL);
   }
}
