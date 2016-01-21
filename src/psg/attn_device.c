/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*  All Attenuator functions are in this file  */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>

#include "acodes.h"
#include "oopc.h"
#include "acqparms.h"

extern int      newacq;
#ifndef INTERACT
extern codeint rt_alc_tab[];
#endif

#ifdef DEBUG
extern int      bgflag;

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

extern char *ObjError(),*ObjCmd(); 
#ifndef INTERACT
extern int    safety_check();         /* safety checking for max atten value */
#endif

#define DPRTLEVEL 2

#define STD_APBUS_DELAY  32			/* 400 nanosecs */

typedef struct {
#include "attn_device.p"
} Attn_Object;

/* base class dispatcher */
#define Base(this,msg,par,res)    AP_Device(this,msg,par,res)

extern int      curfifocount;
extern int	ap_interface;

static int init_atten(Msg_New_Result *result);
static int get_attr(Attn_Object *this, Msg_Set_Param *param, Msg_Set_Result *result);
static int set_attn(Attn_Object *this, Msg_Set_Param *param, Msg_Set_Result *result);

/*-------------------------------------------------------------
| Attn_Device()/4 - Message Handler for Attn_devices.
|			Author: Greg Brissey  8/18/88
+-------------------------------------------------------------*/
int Attn_Device(this,msg,param,result)
Attn_Object*	this;
Message		msg;
caddr_t		param;
caddr_t		result;
{
   int error = 0;
   switch (msg)
   {
      case MSG_NEW_DEV_r:
	error = init_atten((Msg_New_Result *) result);
	break;
      case MSG_SET_ATTN_ATTR_pr:
	error = set_attn(this, (Msg_Set_Param *) param, (Msg_Set_Result *) result);
	break;
      case MSG_GET_ATTN_ATTR_pr:
	error = get_attr(this, (Msg_Set_Param *) param, (Msg_Set_Result *) result);
	break;
      default:
	error = Base(this,msg,param,result);
	break;
   }
   return(error);
}

/*----------------------------------------------------------
|  init_atten()/1 - create a new attenuator device structure
|			Author: Greg Brissey
+----------------------------------------------------------*/
static int init_atten(Msg_New_Result *result)
{
   result->object = (Object) malloc(sizeof(Attn_Object));
   if (result->object == (Object) 0)
   {
      fprintf(stderr,"Insuffient memory for New Attn Device!!");
      return(NO_MEMORY);
   }
   ClearTable(result->object, sizeof(Attn_Object));       /* besure all clear */
   result->object->dispatch = (Functionp) Attn_Device;
   return(0);
}

/*----------------------------------------------------------
| set_attr()/3 - static routine that has access to attn device 
|		 attributes, address,register,bytes,mode
|			Author: Greg Brissey  8/18/88
+----------------------------------------------------------*/
static int set_attn(Attn_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int error = 0;
   c68int encode_word;

   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case SET_MAXVAL:
   	DPRINT3(DPRTLEVEL,
	  "set_attn: Cmd: '%s' to  %d, for device: '%s'\n",
	      ObjCmd(param->setwhat),param->value,this->objname);
	this->maxattn = (c68long) param->value;
	break;
      case SET_MINVAL:	/* obsserve, decoupler, etc */
   	DPRINT3(DPRTLEVEL,
	  "set_attn: Cmd: '%s' to %d, for device: '%s'\n",
	      ObjCmd(param->setwhat),param->value,this->objname);
	this->minattn = (c68long) param->value;
	break;
      case SET_OFFSET:	/* obsserve, decoupler, etc */
   	DPRINT3(DPRTLEVEL,
	  "set_attn: Cmd: '%s' to %d, for device: '%s'\n",
	      ObjCmd(param->setwhat),param->value,this->objname);
	this->offsetattn = (c68long) param->value;
	break;
      case SET_RFBAND:	/* For SIS Unity systems */
   	DPRINT3(DPRTLEVEL,
	  "set_attn: Cmd: '%s' to %d, for device: '%s'\n",
	      ObjCmd(param->setwhat),param->value,this->objname);
	this->rfband = (c68long) param->value;
	this->setselect = (c68int) RFBAND_SEL;
	break;
      case SET_VALUE:
   	DPRINT3(DPRTLEVEL,
	  "set_attn: Cmd: '%s' to %d, for device: '%s'\n",
	      ObjCmd(param->setwhat),param->value,this->objname);

#ifndef INTERACT
        if (safety_check("MAXATTEN", "set_attn():SET_VALUE", (double)(param->value), -1, this->objname) == 0 )
        {
          psg_abort(1);
        }
#endif

        if (this->ap_mode == 3 /* LOCK/DECOUP Attn */)
	{
	  this->rtptr4attn = (c68long) (param->value);
	  /* Power for Lock/Decoup (rftype='l') is like the Lock Power and must be handled in the console */
          putcode((c68int) LOCKDEC_ATTN);
	  putcode(((this->ap_adr << 8) & 0xff00) |
                          (this->ap_reg & 0xff) );
	  putcode((c68int) this->rtptr4attn);  /* absolute value */
	  putcode((c68int) STD_APBUS_DELAY);  /* 32 == apbus delay 400 nsec */

          DPRINT2(DPRTLEVEL,"LOCKDEC_ATTN: %d, 0x%x\n",LOCKDEC_ATTN,LOCKDEC_ATTN);
          DPRINT1(DPRTLEVEL,"apadr|apreg = 0x%x \n",
		     ((this->ap_adr << 8)&0xff00) | (this->ap_reg & 0xff));
          DPRINT1(DPRTLEVEL,"value: %d\n", this->rtptr4attn);
          DPRINT1(DPRTLEVEL,"apdelay: %d\n", STD_APBUS_DELAY);
	  this->curattn = this->rtptr4attn;
        }
	else if (this->maxattn != (c68long)255)  
	if (this->maxattn != (c68long)255)  
	{
        /*** NMR Systems ***/
	this->rtptr4attn = (c68long) (param->value);
        encode_word = 0;
        DPRINT1(DPRTLEVEL,"Channel=%d\n",this->dev_channel);
        if (this->ap_mode == -1)
           encode_word |= NEGLOGIC_BIT;
        if (this->ap_mode == 2)
           encode_word |= PWR_NOT_ATTN;
        encode_word |= (0x00ff & this->ap_bytes);
        DPRINT2(DPRTLEVEL,"APCOUT: %d, 0x%x\n",APCOUT,APCOUT);
        DPRINT1(DPRTLEVEL,"apadr|apreg = 0x%x \n",
		   ((this->ap_adr << 8)&0xff00) | (this->ap_reg & 0xff));
        DPRINT1(DPRTLEVEL,"encode word=0x%x\n",encode_word);
        DPRINT1(DPRTLEVEL,"offset=%d\n",this->offsetattn);
        DPRINT1(DPRTLEVEL,"Value = %d\n",this->rtptr4attn);
        putcode((c68int)APCOUT);
	putcode(((this->ap_adr << 8) & 0xff00) |
                          (this->ap_reg & 0xff) );
	putcode((c68int) encode_word);
	/* val = abs maxattn - (rtval + offset) */
	/* e.g. val = abs 79 - (63 + 16) */
        putcode((c68int) this->maxattn);
        putcode((c68int) this->offsetattn);
	putcode((c68int) this->rtptr4attn);  /* absolute value */
        result->genfifowrds = 2 * this->ap_bytes; /* words stuffed in fifo */
	curfifocount += (2 * this->ap_bytes);
	this->curattn = this->rtptr4attn;
	}
	else
	{
	/*** SISCO Unity Only ...				***/
	/*** - RF double bit high order bit of attenuator chip.	***/
	validate_imaging_config("SIS attn ");
	if (this->setselect != (c68int) RFBAND_SEL) 
	   this->curattn = (c68long) (param->value);
	if (this->rfband == 0)
	   this->rtptr4attn = (c68long) (this->curattn | 0x080);
	else if (this->rfband == 1)
	   this->rtptr4attn = (c68long) (this->curattn & 0x07f);
	else
	   text_error("ERROR: Invalid rfband in setting attn value.\n");
        encode_word = 0;
        DPRINT1(DPRTLEVEL,"Channel=%d\n",this->dev_channel);
	encode_word = encode_word | this->setselect;
        if (this->ap_mode == -1)
           encode_word |= NEGLOGIC_BIT;
	if (this->ap_reg < 0) 
	   encode_word |= DIRECTWRITE_BIT; 
        encode_word |= (0x00ff & this->ap_bytes);
        DPRINT2(DPRTLEVEL,"APSOUT: %d, 0x%x\n",APCOUT,APCOUT);
        DPRINT1(DPRTLEVEL,"apadr|apreg = 0x%x \n",
		   ((this->ap_adr << 8)&0xff00) | (this->ap_reg & 0xff));
        DPRINT1(DPRTLEVEL,"encode word=0x%x\n",encode_word);
        DPRINT1(DPRTLEVEL,"offset=%d\n",this->offsetattn);
        DPRINT1(DPRTLEVEL,"Value = %d\n",this->rtptr4attn);
        putcode((c68int)APSOUT);
	putcode(((this->ap_adr << 8) & 0xff00) |
                          (this->ap_reg & 0xff) );
	putcode((c68int) encode_word);
        putcode((c68int) this->maxattn);
	putcode((c68int) this->rtptr4attn);  /* absolute value */
        result->genfifowrds = 2 * this->ap_bytes; /* words stuffed in fifo */
	curfifocount += (2 * this->ap_bytes);
	this->setselect = 0;
	/*** end SISCO ***/
	}
        break;

      case SET_RTPARAM:
   	DPRINT3(DPRTLEVEL,
	  "set_attn: Cmd: '%s' to %d, for device: '%s'\n",
	      ObjCmd(param->setwhat),param->value,this->objname);
	if (this->maxattn != (c68long)255)
	{
        /*** NMR Systems ***/
	this->rtptr4attn = (c68long) param->value;

/*  Get current value from the PSG copy of the Low Core variable.  The value
 *  is the offset (in units of short words) from the Base of Low Core.  */

#ifndef INTERACT
        if (newacq)
           this->curattn = *(lc_stadr + rt_alc_tab[param->value]);
        else
#endif /* not INTERACT */
           this->curattn = *(lc_stadr + param->value);
 	DPRINT3( DPRTLEVEL,
	  "lc_stadr in the Attenuator Object: 0x%x, offset: 0x%x, value: %d\n",
	   lc_stadr, param->value, this->curattn );
        DPRINT1(DPRTLEVEL,"Channel=%d\n",this->dev_channel);

           encode_word = RTVAR_BIT;
           if (this->ap_mode == -1)
              encode_word |= NEGLOGIC_BIT;
           if (this->ap_mode == 2)
              encode_word |= PWR_NOT_ATTN;
           encode_word |= (0x00ff & this->ap_bytes);
           DPRINT2(DPRTLEVEL,"APCOUT: %d, 0x%x\n",APCOUT,APCOUT);
           DPRINT1(DPRTLEVEL,"apadr|apreg = 0x%x \n",
		   ((this->ap_adr << 8)&0xff00) | (this->ap_reg & 0xff));
           DPRINT1(DPRTLEVEL,"encode word=0x%x\n",encode_word);
           DPRINT1(DPRTLEVEL,"offset=%d\n",this->offsetattn);
           DPRINT1(DPRTLEVEL,"RT Ptr=%d\n",this->rtptr4attn);
           putcode((c68int)APCOUT);
	   putcode(((this->ap_adr << 8) & 0xff00) |
                             (this->ap_reg & 0xff) );
	   putcode((c68int) encode_word);
	   /* val = abs maxattn - (rtval + offset) */
	   /* e.g. val = abs 79 - (63 + 16) */
           putcode((c68int) this->maxattn);
           putcode((c68int) this->offsetattn);
	   putcode((c68int) this->rtptr4attn); /*val = abs maxattn - (rtval + offset)*/
           result->genfifowrds = 2 * this->ap_bytes; /* words stuffed in fifo */
	   curfifocount += (2 * this->ap_bytes);
	}
	else
	{
	/*** SISCO Unity Only ...				***/
	/*** - RF double bit high order bit of attenuator chip.	***/
	  validate_imaging_config("SIS attn");

	   this->rtptr4attn = (c68long) param->value;
/*  Get current value from the PSG copy of the Low Core variable.  The value
 *  is the offset (in units of short words) from the Base of Low Core.  */

#ifndef INTERACT
           if (newacq)
              this->curattn = *(lc_stadr + rt_alc_tab[param->value]);
           else
#endif /* not INTERACT */
              this->curattn = *(lc_stadr + param->value);
           DPRINT2(DPRTLEVEL,"Channel=%d val=%d\n",this->dev_channel, this->curattn);
           encode_word = RTVAR_BIT;
	   encode_word = encode_word | this->setselect;
	   if (this->ap_reg < 0) 
	   	encode_word |= DIRECTWRITE_BIT; 
           if (this->ap_mode == -1)
              encode_word |= NEGLOGIC_BIT;
           encode_word |= (0x00ff & this->ap_bytes);
           DPRINT2(DPRTLEVEL,"APSOUT: %d, 0x%x\n",APCOUT,APCOUT);
           DPRINT1(DPRTLEVEL,"apadr|apreg = 0x%x \n",
		   ((this->ap_adr << 8)&0xff00) | (this->ap_reg & 0xff));
           DPRINT1(DPRTLEVEL,"encode word=0x%x\n",encode_word);
           DPRINT1(DPRTLEVEL,"offset=%d\n",this->offsetattn);
           DPRINT1(DPRTLEVEL,"RT Ptr=%d\n",this->rtptr4attn);
           putcode((c68int)APSOUT);
	   putcode(((this->ap_adr << 8) & 0xff00) |
                             (this->ap_reg & 0xff) );
	   putcode((c68int) encode_word);
	   putcode((c68int) this->maxattn);
	   putcode((c68int) this->rtptr4attn);
           result->genfifowrds = 2 * this->ap_bytes; /* words stuffed in fifo */
	   curfifocount += (2 * this->ap_bytes);
	   this->setselect = 0;
	}
        break;

      case SET_DEFAULTS:
   	DPRINT2(DPRTLEVEL,
	  "set_attn: Cmd: '%s' for device: '%s'\n",
	      ObjCmd(param->setwhat),this->objname);
	this->dev_state=ACTIVE;
	this->dev_cntrl=VALUEONLY;
	this->dev_channel=OBSERVE;
	this->ap_adr=OBS_ATTN_AP_ADR;
	this->ap_reg=OBS_ATTN_AP_REG;
	this->ap_bytes=OBS_ATTN_BYTES;
	this->ap_mode=OBS_ATTN_MODE;
	this->maxattn=OBS_ATTN_MAXVAL;	/* max value of device possible */
	this->minattn=OBS_ATTN_MINVAL;	/* min value of device possible */
	this->offsetattn=OBS_ATTN_OFSET;/* Zero */
	this->rtptr4attn=0;	/* present Value/offset device is set to */
	this->curattn=0;	/* present value for attenuator. */
        break;			/* see attn_device.p for description of */
				/* rtptr4attn and curattn. */

	/*** SISCO Unity Only ...				***/
	/*** - RF double bit high order bit of attenuator chip.	***/
      case SET_GTAB:
   	DPRINT3(DPRTLEVEL,
	  "set_value: Cmd: '%s' to %d, for device: '%s'\n",
	      ObjCmd(param->setwhat),param->value,this->objname);
	/* we invert the bits here because the acquisition computer	*/
	/* will subtract the rtptr4attn from max value to obtain an	*/
	/* attenuation value.  At this point rtptr4attn is a power 	*/
	/* value.							*/
	validate_imaging_config("SIS attn list");
	if (this->setselect != (c68int) RFBAND_SEL) 
	   text_error("ERROR: SET_GTAB not used with attn value.\n");
	if (this->rfband == 0)
	   this->rtptr4attn = (c68long) (this->curattn | 0x080);
	else if (this->rfband == 1)
	   this->rtptr4attn = (c68long) (this->curattn & 0x07f);
	else
	   text_error("ERROR: Invalid rfband in setting attn value.\n");
        encode_word = 0;
        DPRINT1(DPRTLEVEL,"Channel=%d\n",this->dev_channel);
	encode_word = encode_word | this->setselect;
        if (this->ap_mode == -1)
           encode_word |= NEGLOGIC_BIT;
	if (this->ap_reg < 0)
	   encode_word |= DIRECTWRITE_BIT;
        encode_word |= (0x00ff & this->ap_bytes);
        DPRINT2(DPRTLEVEL,"APSOUT: %d, 0x%x\n",APCOUT,APCOUT);
        DPRINT1(DPRTLEVEL,"apadr|apreg = 0x%x \n",
		   ((this->ap_adr << 8)&0xff00) | (this->ap_reg & 0xff));
        DPRINT1(DPRTLEVEL,"encode word=0x%x\n",encode_word);
        DPRINT1(DPRTLEVEL,"offset=%d\n",this->offsetattn);
        DPRINT1(DPRTLEVEL,"Value = %d\n",this->rtptr4attn);
        putgtab(param->value,(c68int)APSOUT);
	putgtab(param->value,((this->ap_adr << 8) & 0xff00) |
                          (this->ap_reg & 0xff)) ;
	putgtab(param->value,(c68int) encode_word);
        putgtab(param->value,(c68int) this->maxattn);
	putgtab(param->value,(c68int) this->rtptr4attn);  /* absolute */
								/* value */
        result->genfifowrds = 2 * this->ap_bytes; /* words stuffed in fifo */
	/* not incremented here */
	/* curfifocount += (2 * this->ap_bytes); */
	this->setselect = 0;
        break;

      default:
	error = UNKNOWN_ATTN_ATTR;
	result->resultcode = UNKNOWN_ATTN_ATTR;
	break;
   }
   return(error);
}

/*----------------------------------------------------------
| get_attr()/3 - static routine that has access to attn device 
|		 attributes, address,register,bytes,mode
|			Author: Greg Brissey  8/18/88
+----------------------------------------------------------*/
static int get_attr(Attn_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int error = 0;
   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case GET_MAXVAL:
	result->reqvalue = this->maxattn;
   	DPRINT3(DPRTLEVEL,
	  "get_attn: Cmd: '%s', is %d for device: '%s'\n",
	      ObjCmd(param->setwhat),result->reqvalue,this->objname);
	break;
      case GET_MINVAL:  /* observe, decoupler, etc */
	result->reqvalue = this->minattn;
   	DPRINT3(DPRTLEVEL,
	  "get_attn: Cmd: '%s', is %d for device: '%s'\n",
	      ObjCmd(param->setwhat),result->reqvalue,this->objname);
	break;
      case GET_OFFSET:
	result->reqvalue = this->offsetattn;
   	DPRINT3(DPRTLEVEL,
	  "get_attn: Cmd: '%s', is %d for device: '%s'\n",
	      ObjCmd(param->setwhat),result->reqvalue,this->objname);
	break;
      case GET_VALUE:
	result->reqvalue = this->curattn;
   	DPRINT3(DPRTLEVEL,
	  "get_attn: Cmd: '%s', is %d for device: '%s'\n",
	      ObjCmd(param->setwhat),result->reqvalue,this->objname);
	break;
      default:
	error = UNKNOWN_ATTN_ATTR;
	result->resultcode = UNKNOWN_ATTN_ATTR;
   }
   return(error);
}


/*---------------------------------------------------------------------
| SetAttnAttr  -  Set Attenuator Device to List of Attributes
|			Author: Greg Brissey  8/18/88
+---------------------------------------------------------------------*/
SetAttnAttr(Object attnobj, ...)
{
  va_list vargs;
  char msge[128];
  int error = 0;
  int error2 = 0;
  Msg_Set_Param param;
  Msg_Set_Result result;

   va_start(vargs, attnobj);

   /* Options allways follow the format 'Option,(value),Option,(value),etc' */
   while ( (param.setwhat = va_arg(vargs,int)) != 0)
   {
     if (param.setwhat != SET_DEFAULTS)    /* default has no value parameter */
	param.value = (c68int) va_arg(vargs,int) ;
     error = Send(attnobj,MSG_SET_ATTN_ATTR_pr,&param,&result);
      if (error < 0)
      {
	 error2 = Send(attnobj, MSG_SET_AP_ATTR_pr, &param, &result);
	 if (error2 < 0)
	 {
	    error2 = Send(attnobj, MSG_SET_DEV_ATTR_pr, &param, &result);
	    if (error2 < 0)
	    {
	       sprintf(msge, "%s : %s  '%s'\n", attnobj->objname, ObjError(error),
		       ObjCmd(param.setwhat));
	       text_error(msge);
	       psg_abort(1);
	    }
	 }
      }
   }
   va_end(vargs);
   return(error);
}
