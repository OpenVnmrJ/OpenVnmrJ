/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>		/* for caddr_t definition */

#include "oopc.h"
#include "acodes.h"
#include "rfconst.h"

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
#define DPRINT5(level, str, arg1, arg2, arg3, arg4,arg5) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2,arg3,arg4,arg5)
#else
#define DPRINT(level, str)
#define DPRINT1(level, str, arg2)
#define DPRINT2(level, str, arg1, arg2)
#define DPRINT3(level, str, arg1, arg2, arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4)
#define DPRINT5(level, str, arg1, arg2, arg3, arg4, arg5)
#endif

extern char *ObjError(),*ObjCmd(); 

#define DPRTLEVEL 1
#define	HYDRA_ROTO_HS 0x80000000L

typedef struct
{
#include "event_device.p"
}               Event_Object;

/* base class dispatcher */
#define Base(this,msg,par,res)    Device(this,msg,par,res)

extern int	ap_interface;
extern int      curfifocount;
extern int      HSlines;
extern int      fifolpsize;	/* size in words of looping fifo */
extern int	newacq;		/* INOVA Flag */

static int init_event(Msg_New_Result *result);
static int get_attr(Event_Object *this, Msg_Set_Param *param, Msg_Set_Result *result);
static int get_value(Event_Object *this, Msg_Set_Param *param, Msg_Set_Result *result);
static int set_attr(Event_Object *this, Msg_Set_Param *param, Msg_Set_Result *result);
static int set_value(Event_Object *this, Msg_Set_Param *param, Msg_Set_Result *result);
static int valid_rtpar(Event_Object *this, register c68int rtpar);
static int valid_tbase(Event_Object *this, int base);

/*-------------------------------------------------------------
| Event_Device()/4 - Message Handler for apbit_devices.
|                       Author: Greg Brissey  8/18/88
+-------------------------------------------------------------*/
int 
Event_Device(this, msg, param, result)
Event_Object  *this;
Message         msg;
caddr_t         param;
caddr_t         result;
{
   int             error = 0;

   switch (msg)
   {
      case MSG_NEW_DEV_r:
	 error = init_event( (Msg_New_Result *) result);
	 break;
      case MSG_SET_EVENT_ATTR_pr:
	 error = set_attr(this, (Msg_Set_Param *) param, (Msg_Set_Result *) result);
	 break;
      case MSG_SET_EVENT_VAL_pr:
	 error = set_value(this, (Msg_Set_Param *) param, (Msg_Set_Result *) result);
	 break;
      case MSG_GET_EVENT_VAL_pr:
	 error = get_value(this, (Msg_Set_Param *) param, (Msg_Set_Result *) result);
	 break;
      case MSG_GET_EVENT_ATTR_pr:
	 error = get_attr(this, (Msg_Set_Param *) param, (Msg_Set_Result *) result);
	 break;
      default:
	 error = Base(this, msg, param, result);
	 break;
   }
   return(error);
}

/*----------------------------------------------------------
|  init_event()/1 - create a new event device structure
|			Author: Greg Brissey
+----------------------------------------------------------*/
static int init_event(Msg_New_Result *result)
{
   result->object = (Object) malloc(sizeof(Event_Object));
   if (result->object == (Object) 0)
   {
      fprintf(stderr,"Insuffient memory for New Attn Device!!");
      return(NO_MEMORY);
   }
   ClearTable(result->object, sizeof(Event_Object));/* besure all clear */
   result->object->dispatch = (Functionp) Event_Device;
   return(0);
}

/*----------------------------------------------------------
| set_value()/3 - static routine that has access to event device
|
|                       Author: Greg Brissey  8/18/88
+----------------------------------------------------------*/
static int set_value(Event_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;

   result->resultcode = result->genfifowrds = result->reqvalue = 0;
   switch (this->event_type)
   {
      case EVENT_HS_ROTOR:
	 switch (param->setwhat)
	 {
	    case SET_RTPARAM:
	       if (fifolpsize < 512)
	       {
		  abort_message("Warning: Incompatible Output Board for Rotor Sync.");
	       }
   	       DPRINT3(DPRTLEVEL,
	       "Event_Device: EVENT_HS_ROTOR, Cmd: '%s' to %d, for device: '%s'\n",
		      ObjCmd(param->setwhat),param->value,this->objname);
 	       if ( ! valid_rtpar(this,param->value) )
	       {
		   char msge[128];
		   sprintf(msge,
			"%s: Invalid Real Time Parameter Specified.\n",
			 this->objname);
		   abort_message(msge);
	       }

               DPRINT2(DPRTLEVEL,"RT_EVENT1: %d, 0x%x\n",RT_EVENT1,RT_EVENT1);
               DPRINT1(DPRTLEVEL,"TCNT: %d\n",TCNT);
               DPRINT1(DPRTLEVEL,"EXT_TRIGGER_TIMEBASE: 0x%x\n",EXT_TRIGGER_TIMEBASE);
               DPRINT1(DPRTLEVEL,"RT Param: %d \n", param->value);
               DPRINT1(DPRTLEVEL,"HSlines: 0x%x\n",HSlines);
 
	       if (!newacq)
	       {
                 if (ap_interface < 4)
	           HSgate(SP2, TRUE);
                 else
                   HSgate(HYDRA_ROTO_HS,TRUE);
	         putcode((c68int) RT_EVENT1);
	         putcode((c68int) TCNT);
	         putcode((c68int) EXT_TRIGGER_TIMEBASE);
	         putcode((c68int) param->value);	/* rt parameter */
	         putcode((c68int) HSlines);
                 if (ap_interface < 4)
	            HSgate(SP2, FALSE);
                 else
                    HSgate(HYDRA_ROTO_HS,FALSE);
	       }
	       else	/* INOVA */
	       {
	         putcode((c68int) ROTORSYNC);
	         putcode((c68int) 1);
	         putcode((c68int) param->value);	/* rt parameter */
	       }

/*
 *  Remove output board differences
 *
 *       if ( fifolpsize < 65)
 *       {
 *         DPRINT2(DPRTLEVEL,"EVENT1: %d, 0x%x\n",EVENT1,EVENT1);
 *         DPRINT1(DPRTLEVEL,"Rotor Tbase: 0x%x\n",ROTORSYNC_TIMEBASE);
 *         DPRINT1(DPRTLEVEL,"HSlines: 0x%x\n",HSlines);
 *
 *         putcode((c68int) EVENT1);
 *         putcode((c68int) ROTORSYNC_TIMEBASE);
 *         putcode((c68int) HSlines);
 *       }
 *       else
 */
	       {
                 DPRINT2(DPRTLEVEL,"EVENT1_TWRD: %d, 0x%x\n",EVENT1_TWRD,EVENT1_TWRD);
                 DPRINT1(DPRTLEVEL,"Rotor Tbase: 0x%x\n",ROTORSYNC_TIMEBASE);
                 DPRINT1(DPRTLEVEL,"HSlines: 0x%x\n",HSlines);

	         if (!newacq)
	         {
	           putcode((c68int) EVENT1_TWRD);
	           putcode((c68int) ROTORSYNC_TIMEBASE);/*TBase5,trigger after cntdwn*/
	         }
	       }

	       result->genfifowrds = 2;	/* words stuffed in acquisition acode */
	       curfifocount += 2;
	       break;

	    case SET_VALUE:
	       if (param->value > 0)
	       {
		  int count;

		  if (fifolpsize < 512)		/* determine fifo type */
		  {
		     abort_message("Warning: Incompatible Output Board for Rotor Sync.");
		  }
	          if (!newacq)
	          {
		    /* correct count for fifo type */
		    count = crtfifo_cnt(param->value);/* output bd correction */
                    if (ap_interface < 4)
	               HSgate(SP2, TRUE);
                    else
                       HSgate(HYDRA_ROTO_HS,TRUE);
	          }
		  else   /* INOVA */
	          {
	            count = param->value;
	          }

   	          DPRINT3(DPRTLEVEL,
	           "Event_Device: EVENT_HS_ROTOR, Cmd: '%s' to %d, for device: '%s'\n",
		      ObjCmd(param->setwhat),param->value,this->objname);

                  DPRINT2(DPRTLEVEL,"EVENT1_TWRD: %d, 0x%x\n",EVENT1_TWRD,EVENT1_TWRD);
        	  DPRINT1(DPRTLEVEL,"EXT. Tbase + Cnt: 0x%x\n",
			(EXT_TRIGGER_TIMEBASE | (count & 0xfff)));
        	  DPRINT1(DPRTLEVEL,"HSlines: 0x%x\n",HSlines);

	          if (!newacq)
	          {
		    putcode((c68int) EVENT1_TWRD);
		    putcode((c68int) (EXT_TRIGGER_TIMEBASE | (count & 0xfff)));

                    if (ap_interface < 4)
                       HSgate(SP2, FALSE);    /* start rotor sync count down */
                    else
                       HSgate(HYDRA_ROTO_HS,FALSE);
		  }
		  else
		  {
	            putcode((c68int) ROTORSYNC);
	            putcode((c68int) 0);
	            putcode((c68int) count);	/* parameter value */
		  }

                  DPRINT2(DPRTLEVEL,"EVENT1_TWRD: %d, 0x%x\n",EVENT1_TWRD,EVENT1_TWRD);
        	  DPRINT1(DPRTLEVEL,"Rotor Tbase: 0x%x\n",ROTORSYNC_TIMEBASE);
        	  DPRINT1(DPRTLEVEL,"HSlines: 0x%x\n",HSlines);

	          if (!newacq)
	          {
		    putcode((c68int) EVENT1_TWRD);
		    putcode((c68int) ROTORSYNC_TIMEBASE);	/* TBase5, trigger after
								 * countdown */
		  }
		  result->genfifowrds = 2;	/* words stuffed in
						 * acquisition acode */
		  curfifocount += 2;
	       }
	       break;
	    case GET_RTVALUE:
   	       DPRINT3(DPRTLEVEL,
	       "Event_Device: EVENT_HS_ROTOR, Cmd: '%s', is %d for device: '%s'\n",
		      ObjCmd(param->setwhat),param->value,this->objname);
 	       if ( ! valid_rtpar(this,param->value) )
	       {
		   char msge[128];
		   sprintf(msge,
			"%s: Invalid Real Time Parameter Specified.\n",
			 this->objname);
		   abort_message(msge);
	       }
                DPRINT2(DPRTLEVEL,"RD_HSROTOR: %d, 0x%x\n",
			 RD_HSROTOR,RD_HSROTOR);
        	DPRINT1(DPRTLEVEL,"RT Param: %d \n",param->value);
	       putcode((c68int) RD_HSROTOR);
	       putcode((c68int) param->value);	/* rt parameter to put value
						 * into */
	       break;
	    default:
	        error = UNKNOWN_EVENT_TYPE_ATTR;
		result->resultcode = UNKNOWN_EVENT_TYPE_ATTR;
	       break;
	 }
	 break;
      case EVENT_EXT_TRIGGER:
	 switch (param->setwhat)
	 {
	    case SET_RTPARAM:
   	       DPRINT3(DPRTLEVEL,
	       "Event_Device: EVENT_EXT_TRIGGER, Cmd: '%s' to %d, for device: '%s'\n",
		      ObjCmd(param->setwhat),param->value,this->objname);
 	       if ( ! valid_rtpar(this,param->value) )
	       {
		   char msge[128];
		   sprintf(msge,
			"%s: Invalid Real Time Parameter Specified.\n",
			 this->objname);
		   abort_message(msge);
	       }
               DPRINT2(DPRTLEVEL,"RT_EVENT1: %d, 0x%x\n",RT_EVENT1,RT_EVENT1);
               DPRINT1(DPRTLEVEL,"TCNT: %d\n",TCNT);
               DPRINT1(DPRTLEVEL,"EXT_TRIGGER_TIMEBASE: 0x%x\n",EXT_TRIGGER_TIMEBASE);
               DPRINT1(DPRTLEVEL,"RT Param: %d \n", param->value);
               DPRINT1(DPRTLEVEL,"HSlines: 0x%x\n",HSlines);

	       if (!newacq)
	       {
	         putcode(RT_EVENT1);
	         putcode(TCNT);
	         putcode(EXT_TRIGGER_TIMEBASE);
	         putcode((c68int) param->value);	/* rt parameter */
	         putcode((c68int) HSlines);
	       }
	       else	/* INOVA */
	       {
	         putcode((short)EXTGATE);
	         putcode((short)1);
	         putcode((c68int) param->value);	/* rt parameter */
	       }
	       result->genfifowrds = 1;	/* words stuffed in acquisition acode */
	       curfifocount++;
	       break;

	    case SET_VALUE:
	       if (param->value > 0)
	       {
 		  int count;

		  /* correct count for fifo type */
		  if (!newacq)
		    count = crtfifo_cnt(param->value); 
		  else
		    count = param->value;

   	          DPRINT3(DPRTLEVEL,
	          "Event_Device: EVENT_EXT_TRIGGER, Cmd: '%s' to %d, for device: '%s'\n",
		      ObjCmd(param->setwhat),param->value,this->objname);

/*
 *  Remove output board differences
 *
 *  	if ( fifolpsize < 512 )
 *	{
 *        DPRINT2(DPRTLEVEL,"EVENT1: %d, 0x%x\n",EVENT1,EVENT1);
 *        DPRINT1(DPRTLEVEL,"EXT. Tbase + Cnt: 0x%x\n",
 *	          (EXT_TRIGGER_TIMEBASE | (count & 0xfff)));
 *        DPRINT1(DPRTLEVEL,"HSlines: 0x%x\n",HSlines);
 *
 *	  putcode(EVENT1);
 *	  putcode((c68int) EXT_TRIGGER_TIMEBASE | (count & 0xfff));
 *	  putcode((c68int) HSlines);
 *	}
 *	else
 */
		{
                  DPRINT2(DPRTLEVEL,"EVENT1_TWRD: %d, 0x%x\n",
			  EVENT1_TWRD,EVENT1_TWRD);
        	  DPRINT1(DPRTLEVEL,"EXT. Tbase + Cnt: 0x%x\n",
			(EXT_TRIGGER_TIMEBASE | (count & 0xfff)));
        	  DPRINT1(DPRTLEVEL,"HSlines: 0x%x\n",HSlines);

		  if (!newacq)
		  {
		    putcode(EVENT1_TWRD);
		    putcode((c68int) EXT_TRIGGER_TIMEBASE | (count & 0xfff));
		  }
		  else  /* INOVA */
		  {
		    putcode((short)EXTGATE);
		    putcode((short)0);
		    putcode((short)(count & 0xffff));
		  }
		}
		result->genfifowrds = 1;	/* words stuffed in
						 * acquisition acode */
		curfifocount++;

	       }
	       break;
	    default:
	        error = UNKNOWN_EVENT_TYPE_ATTR;
		result->resultcode = UNKNOWN_EVENT_TYPE_ATTR;
	       break;
	 }
	 break;

      case EVENT_RT_DELAY:
       {
	 RT_Event *rtevnt;
         int rtbase,absbase;
         int rtcount,abscount;
         int rthslines,abshslines;
	 rtevnt = (RT_Event *) param->valptr; /* init rt event struct ptr */
   	 DPRINT3(DPRTLEVEL,
	    "Event_Device: EVENT_RT_DELAY, Cmd: '%s' to struct: 0x%x, for device: '%s'\n",
	     ObjCmd(param->setwhat),param->valptr,this->objname);
	 switch (rtevnt->rtevnt_type)
	 {
	    case TCNT:
		 absbase = rtevnt->timerbase;
		 rtcount = rtevnt->timercnt;
		 abshslines = HSlines;
   	         DPRINT4(DPRTLEVEL,
	         "EVENT_RT_DELAY: TCNT, TimeBase: %d, Cnt: 0x%x(%d) HSlines: 0x%x\n",
		 absbase,rtcount,rtcount,HSlines);
                 valid_tbase(this,absbase); /* returns if OK, else aborts */
 	         if ( ! valid_rtpar(this,rtcount) )
	         {
		   char msge[128];
		   sprintf(msge,
			"%s: Invalid Real Time Parameter Specified for Count.\n",
			 this->objname);
		   abort_message(msge);
	         }
		 putcode(RT_EVENT1);
		 putcode(TCNT);
		 putcode(absbase);
		 putcode(rtcount);
		 putcode(HSlines);
		 break;
	    case TWRD_TCNT:
		 rtbase = rtevnt->timerbase;
		 rtcount = rtevnt->timercnt;
		 abshslines = HSlines;
   	         DPRINT5(DPRTLEVEL,
	         "EVENT_RT_DELAY: TWRD_TCNT, TimeBase: 0x%x(%d), Cnt: 0x%x(%d) HSlines: 0x%x\n",
		 rtbase,rtbase,rtcount,rtcount,HSlines);
 	         if ( ! valid_rtpar(this,rtbase) )
	         {
		   char msge[128];
		   sprintf(msge,
			"%s: Invalid Real Time Parameter Specified for Time Base.\n",
			 this->objname);
		   abort_message(msge);
	         }
 	         if ( ! valid_rtpar(this,rtcount) )
	         {
		   char msge[128];
		   sprintf(msge,
			"%s: Invalid Real Time Parameter Specified for Count.\n",
			 this->objname);
		   abort_message(msge);
	         }
		 putcode(RT_EVENT1);
		 putcode(TWRD_TCNT);
		 putcode(rtbase);
		 putcode(rtcount);
		 putcode(HSlines);
		 break;
	    case SET_INITINCR:
		{
		 double timeincr;
		 int twords[5],ovrhdtbases,i;

		 timeincr = rtevnt->incrtime;
		 abscount = rtevnt->timerbase;
   	         DPRINT2(DPRTLEVEL,
	         "EVENT_RT_DELAY: INCRDELAY, incrtime: %lf, index: %d \n",
		 timeincr,abscount);
		 rttimerwords(timeincr,twords);
		 ovrhdtbases = ((twords[0]-1) << 8) & 0xff00;
		 putcode(INITDELAY);
		 putcode(ovrhdtbases | rtevnt->timerbase);
		 putcode(3);
		 /* order is important!, nsec,usec,msec,sec */
		 if (newacq)
		 {
		    putcode(twords[1]);
		    putcode(twords[2]);
		    putcode(twords[3]);
		    putcode(twords[4]);
		 }
		 else
		 {
                    if (twords[1])
		       putcode(absolutecnt(twords[1]));
		    else
		       putcode(0);
                    if (twords[2])
		       putcode(absolutecnt(twords[2]));
		    else
		       putcode(0);
                    if (twords[3])
		       putcode(absolutecnt(twords[3]));
		    else
		       putcode(0);
                    if (twords[4])
		       putcode(absolutecnt(twords[4]));
		    else
		       putcode(0);
		 }
		}
		 break;
	    case SET_ABSINCR:
		 putcode(INCRDELAY);
		 putcode(0x0000 | rtevnt->timerbase); /* Abs. value & RT delay index*/
		 putcode(rtevnt->timercnt);  /* multipler value */
		break;
	    case SET_RTINCR:
		 putcode(INCRDELAY);
		 putcode(0x0100 | rtevnt->timerbase); /* RT value & RT delay index */
		 putcode(rtevnt->timercnt);  /* RT parameter */
		break;
	     }
        }
	break;
      default:
	 error = UNKNOWN_EVENT_TYPE;
	 result->resultcode = UNKNOWN_EVENT_TYPE;
	 break;
   }
   return (error);
}

/*----------------------------------------------------------
| get_value()/3 - static routine that has access to event device
|
|                       Author: Greg Brissey  8/18/88
+----------------------------------------------------------*/
static int get_value(Event_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;

   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case GET_RTVALUE:
        switch (this->event_type)
   	{
           case EVENT_HS_ROTOR:
   	       DPRINT3(DPRTLEVEL,
               "Event_Device: EVENT_HS_ROTOR, Cmd: '%s', is %d for device: '%s'\n",
                      ObjCmd(param->setwhat),param->value,this->objname);
               if ( ! valid_rtpar(this,param->value) )
               {
                   char msge[128];
                   sprintf(msge,
                        "%s: Invalid Real Time Parameter Specified.\n",
                         this->objname);
                   abort_message(msge);
               }
               DPRINT2(DPRTLEVEL,"RD_HSROTOR: %d, 0x%x\n",RD_HSROTOR,RD_HSROTOR);
               DPRINT1(DPRTLEVEL,"RT Param: %d \n",param->value);
               putcode((c68int) RD_HSROTOR);
               putcode((c68int) param->value);  /* rt parameter to put value */
                                                /*    into */
               break;
          default:
	     error = UNKNOWN_EVENT_TYPE;
	     result->resultcode = UNKNOWN_EVENT_TYPE;
	     break;
	}
        break;

	default:
	    error = UNKNOWN_EVENT_TYPE_ATTR;
	    result->resultcode = UNKNOWN_EVENT_TYPE_ATTR;
	    break;
    }
   return (error);
}
/*----------------------------------------------------------
| set_attr()/3 - static routine that has access to apbit device
|                attributes,
|                       Author: Greg Brissey  8/18/88
+----------------------------------------------------------*/
static int set_attr(Event_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   int             error = 0;
   int		i;
   c68int	*rtptr;

   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case SET_DEFAULTS:
         DPRINT2(DPRTLEVEL,
            "set_event_attr: Cmd: '%s' for device: '%s'\n",
              ObjCmd(param->setwhat),this->objname);
	 this->dev_state = ACTIVE;
	 this->dev_cntrl = BITMASK;
	 this->dev_channel = OBSERVE;
	 this->event_type = EVENT_EXT_TRIGGER;
         break;
      case SET_TYPE:
        DPRINT3(DPRTLEVEL,
          "set_event_attr: Cmd: '%s' to %d, for device: '%s'\n",
              ObjCmd(param->setwhat),param->value,this->objname);
	 this->event_type = param->value;
	 break;
      case SET_MAXRTPARS:
        DPRINT3(DPRTLEVEL,
          "set_event_attr: Cmd: '%s' to %d, for device: '%s'\n",
              ObjCmd(param->setwhat),param->value,this->objname);
	 this->max_rtpars = param->value;
	 this->valid_rtpars = (c68int *) malloc( ((param->value + 4) * sizeof(c68int)) );
         if (this->valid_rtpars == (c68int *) 0)
         {
            fprintf(stderr,"Insuffient memory for Valid RT Parameter List.");
            error = NO_RTPAR_MEMORY;
         }
         DPRINT2(DPRTLEVEL,
          "set_event_attr: Cmd: '%s',  valid_rtpars ptr = 0x%lx \n",
              ObjCmd(param->setwhat),this->valid_rtpars);
	 for(i=0,rtptr=this->valid_rtpars; i < this->max_rtpars; i++,rtptr++)
	    *rtptr = -1;
	 break;
      case SET_VALID_RTPAR:
        DPRINT3(DPRTLEVEL,
          "set_event_attr: Cmd: '%s' to %d, for device: '%s'\n",
              ObjCmd(param->setwhat),param->value,this->objname);
	 /* be sure ptr is valid & we will not go past the end of malloc space */
         if ((this->n_rtpars + 1 > this->max_rtpars) || 
	     (this->valid_rtpars == (c68int *) 0))
         { 
	     char msge[128];
       	     sprintf(msge,"%s : MAXRTPARS = %d \n",this->objname,this->max_rtpars);
             text_error(msge);
	     error = PAST_MAX_RTPARS;
	     break;
	 }
	 rtptr = this->valid_rtpars;
         rtptr = rtptr + this->n_rtpars;
         *rtptr = param->value;
	 this->n_rtpars += 1;	/* do this after rtptr + this->n_rtpars, */
			        /* address starts at 0  not 1 */
         DPRINT2(DPRTLEVEL,
          "set_event_attr: Cmd: '%s',  valid_rtpars ptr = 0x%lx \n",
              ObjCmd(param->setwhat),rtptr);
	 break;
      default:
	 error = UNKNOWN_EVENT_ATTR;
         result->resultcode = UNKNOWN_EVENT_ATTR;
	 break;
   }
   return (error);
}
/*----------------------------------------------------------
| get_attr()/3 - static routine that has access to apbit device
|                attributes,
|                       Author: Greg Brissey  8/18/88
+----------------------------------------------------------*/
static int get_attr(Event_Object *this, Msg_Set_Param *param, Msg_Set_Result *result)
{
   char		msge[128];
   int          error = 0;
   int		i,index,max;
   c68int	*rtptr;

   result->resultcode = 0;
   result->genfifowrds = 0;
   result->reqvalue = 0;
   switch (param->setwhat)
   {
      case GET_TYPE:
        DPRINT3(DPRTLEVEL,
          "set_event_attr: Cmd: '%s' to %d, for device: '%s'\n",
              ObjCmd(param->setwhat),param->value,this->objname);
	 result->reqvalue = this->event_type;
	 break;
      case GET_MAXRTPARS:
        DPRINT3(DPRTLEVEL,
          "set_event_attr: Cmd: '%s' to %d, for device: '%s'\n",
              ObjCmd(param->setwhat),param->value,this->objname);
	 result->reqvalue = this->max_rtpars;
	 break;
      case GET_VALID_RTPAR:
        DPRINT3(DPRTLEVEL,
          "set_event_attr: Cmd: '%s' index of %d, for device: '%s'\n",
              ObjCmd(param->setwhat),param->value,this->objname);

         index = param->value;
   	 max = this->n_rtpars;
	 if ( index > 0 && index <= max)
	 {
	    rtptr = this->valid_rtpars;
            rtptr = rtptr + (param->value - 1);
	    result->reqvalue = *rtptr;
	 }
	 else
	 {
       	     sprintf(msge,"%s : MAXRTPARS = %d \n",this->objname,this->max_rtpars);
             text_error(msge);
	     error = PAST_MAX_RTPARS;
   	     result->resultcode = PAST_MAX_RTPARS;
	 }

         DPRINT2(DPRTLEVEL,
          "set_event_attr: Cmd: '%s',  valid_rtpars ptr = 0x%lx \n",
              ObjCmd(param->setwhat),rtptr);
	 break;
      default:
	 error = UNKNOWN_EVENT_ATTR;
         result->resultcode = UNKNOWN_EVENT_ATTR;
	 break;
   }
   return (error);
}
/*----------------------------------------------------------

/*---------------------------------------------------------------------
| SetEventAttr  -  Set Event Device to List of Attributes
|			Author: Greg Brissey  11/16/88
+---------------------------------------------------------------------*/
int SetEventAttr(Object obj, ...)
{
  char  msge[128];
  va_list vargs;
  int error = 0;
  Msg_Set_Param param;
  Msg_Set_Result result;

   va_start(vargs, obj);
   /* Options allways follow the format 'Option,(value),Option,(value),etc' */
   while ( (param.setwhat = va_arg(vargs,int)) != 0)
   {
     if (param.setwhat != SET_DEFAULTS)    /* default has no value parameter */
        param.value = (c68int) va_arg(vargs,int) ;
     error = Send(obj,MSG_SET_EVENT_ATTR_pr,&param,&result);
     if (error < 0)
     {
       sprintf(msge,"%s : %s  '%s'\n",obj->objname,ObjError(error),
		ObjCmd(param.setwhat));
       abort_message(msge);
     }
   }
   va_end(vargs);
   return(error);
}
/*---------------------------------------------------------
| valid_rtpar()
|	check if rt pars given is in the valid rt parameter list
|       for that object
|				Greg B.    11/17/88
+----------------------------------------------------------*/
static int valid_rtpar(Event_Object *this, register c68int rtpar)
{
   register c68int *rtptr;
   register int i,max;

   max = this->n_rtpars;

   for(i=0,rtptr=this->valid_rtpars; i < max; i++,rtptr++)
   {
      if (*rtptr == rtpar)
      {
	  return(1);
      }
   }
   return(0);
}
static int valid_tbase(Event_Object *this, int base)
{
   if ( (base < 1) || (base > 4) )
   {
     char msge[128];
     sprintf(msge,
          "%s: Invalid Time Base Specified.\n",
          this->objname);
     abort_message(msge);
   }
   return(0);
}
static
int valid_tcnt(this,cnt)
Event_Object   *this;
int 		cnt;
{
   if (fifolpsize > 63)
   {
     if ( (cnt < 1) || (cnt > 4096) ) /* 0 to 0xfff, except nsec clock 1 to 0xfff */
     {
       char msge[128];
       sprintf(msge,
          "%s: Invalid Time Base Specified.\n",
          this->objname);
       abort_message(msge);
     }
   }
   else
   {
     if ( (cnt < 2) || (cnt > 4095) ) /* 1 to 0xfff */
     {
       char msge[128];
       sprintf(msge,
          "%s: Invalid Time Base Specified.\n",
          this->objname);
       abort_message(msge);
     }
   }
   return(0);
}
