/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "apdelay.h"
extern char gradtype[];
/*----------------------------------------------------------------------------
|  Event OverHead delay constants
+----------------------------------------------------------------------------*/

struct _event_overhead {
	double ap_power_delay;		/*
					    power()
					    rlpower()
					*/

	double ap_pwrf_delay;		/*  pwrf() */
	double ap_saps_delay; 		/*
			    			xmtrphase()
			    			dcplrphase()
			    			dcplr2phase()
			    			gensaphase()
		     			*/

	double ap_offset_delay;
	double ap_offset_ltch_delay; 	/*
					    offset()
					*/

	double ap_wfg_start_delay;
	double ap_wfg_offset_delay;
	double ap_wfg_stop_delay; 	/*
					    shaped_pulse()
					    decshaped_pulse()
					    dec2shaped_pulse()
					    genshaped_pulse()
					*/
	double ap_wfg_vstart_delay;	/*  shapedvgradient() */
	double ap_wfg_istart_delay;	/*  shapedincgradient() */

	double ap_wfg2_start_delay;
	double ap_wfg2_offset_delay;
	double ap_wfg2_stop_delay;	/*
					    sim2shaped_pulse()
					    gensim2shaped_pulse()
					*/

	double ap_wfg3_start_delay;
	double ap_wfg3_offset_delay;
	double ap_wfg3_stop_delay;	/*
					   sim3shaped_pulse()
					   gensim3shaped_pulse()
					*/

	double ap_prg_start_delay;	/*
					    obsprgon()
					    decprgon()
					    dec2prgon()
					    prg_dec_on()
					*/

	double ap_prg_stop_delay; 			/*
					    obsprgoff()
					    decprgoff()
					    dec2prgoff()
					    prg_dec_off()
					*/

	double ap_spnlck_start_delay;
	double ap_spnlck_stop_delay;	/*
					    spinlock()
					    decspinlock()
					    dec2spinlock()
					    genspinlock()
					*/
	double ap_spn2lck_start_delay;
	double ap_spn2lck_stop_delay;	/*
					    gen2spinlock()
					*/
        double gradient_delay;		/* rgradient/vgradient/incgradient */
	double vagradient_delay;
	double oblique_gradient_delay;

	double acquire_start_delay;
	double acquire_stop_delay;	/* acquire() */
	double ap_setdecmod_delay;	/*  set_channel_modulation()
					    setdecmodulation()
					*/
	double ap_decmodfreq_delay;	/*  initdecmodfreq() */
	double prg_offset_delay;

  };

static struct _event_overhead  event_ovrhead;
/*-------------------------------------------------------------------
|  initeventovrhd() - initializes the overhead of the various events
|
|
+-------------------------------------------------------------------*/
void initeventovrhd(int type)
{
   char grt;
   int i,ngrad;
   /* default to a pfg gradient */
   grt='p';
   ngrad=0;
   for (i=0; i < 3; i++)
   {
     if (gradtype[i] != 'n') 
     {
	ngrad++;
	grt = gradtype[i];
     }
   }
   switch(type)
   {
   case VXR:
		event_ovrhead.ap_power_delay = VXR_POWER_OVERHEAD;
		event_ovrhead.ap_pwrf_delay = VXR_PWRF_OVERHEAD;
		event_ovrhead.ap_saps_delay = VXR_SAPS_OVERHEAD;
		event_ovrhead.ap_offset_delay = VXR_OFFSET_OVERHEAD;
		event_ovrhead.ap_offset_ltch_delay = VXR_OFFSET_LTCH_OVERHEAD;
		event_ovrhead.ap_wfg_start_delay = VXR_WFG_START_OVERHEAD;
		event_ovrhead.ap_wfg_vstart_delay = VXR_WFG_VSTART_OVERHEAD;
		event_ovrhead.ap_wfg_istart_delay = VXR_WFG_ISTART_OVERHEAD;
		event_ovrhead.ap_wfg_offset_delay = VXR_WFG_OFFSET_OVERHEAD;
		event_ovrhead.ap_wfg_stop_delay = VXR_WFG_STOP_OVERHEAD;
		event_ovrhead.ap_wfg2_start_delay = VXR_WFG2_START_OVERHEAD;
		event_ovrhead.ap_wfg2_offset_delay = VXR_WFG2_OFFSET_OVERHEAD;
		event_ovrhead.ap_wfg2_stop_delay = VXR_WFG2_STOP_OVERHEAD;
		event_ovrhead.ap_wfg3_start_delay = VXR_WFG3_START_OVERHEAD;
		event_ovrhead.ap_wfg3_offset_delay = VXR_WFG3_OFFSET_OVERHEAD;
		event_ovrhead.ap_wfg3_stop_delay = VXR_WFG3_STOP_OVERHEAD;
		event_ovrhead.ap_prg_start_delay = VXR_PRG_START_OVERHEAD;
		event_ovrhead.ap_prg_stop_delay = VXR_PRG_STOP_OVERHEAD;
		event_ovrhead.ap_spnlck_start_delay = VXR_SPNLCK_START_OVERHEAD;
		event_ovrhead.ap_spnlck_stop_delay = VXR_SPNLCK_STOP_OVERHEAD;
		event_ovrhead.ap_spn2lck_start_delay = VXR_SPN2LCK_START_OVERHEAD;
		event_ovrhead.ap_spn2lck_stop_delay = VXR_SPN2LCK_STOP_OVERHEAD;
		if ((grt == 'p') || (grt == 'q'))
		{
		  event_ovrhead.gradient_delay = UNITY_P_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITY_P_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = UNITY_P_GRADIENT*ngrad;
		}
		else if (grt == 'l')
		{
		  event_ovrhead.gradient_delay = UNITY_L_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITY_L_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = UNITY_L_GRADIENT*ngrad;
		}
		else if ((grt == 'w') || (grt == 's'))
		{
		  event_ovrhead.gradient_delay = UNITY_W_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITY_W_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = UNITY_W_GRADIENT*ngrad;
		}
		else if ((grt == 'c') || (grt == 'd') || (grt == 't') || (grt == 'u'))
		{
		  event_ovrhead.gradient_delay = UNITYPLUS_T_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITYPLUS_T_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = 
						UNITYPLUS_T_GRADIENT*ngrad;
		}
		else
		{
		  event_ovrhead.gradient_delay = 0.0;
		  event_ovrhead.vagradient_delay = 0.0;
		  event_ovrhead.oblique_gradient_delay = 0.0;
		}
		event_ovrhead.acquire_start_delay = VXR_ACQUIRE_START_OVERHEAD;
		event_ovrhead.acquire_stop_delay = VXR_ACQUIRE_STOP_OVERHEAD;
		event_ovrhead.ap_setdecmod_delay = 0.0;
		event_ovrhead.ap_decmodfreq_delay = 0.0;
		event_ovrhead.prg_offset_delay = VXR_PRG_OFFSET_OVERHEAD;
	break;
   case UNITY:
		event_ovrhead.ap_power_delay = UNITY_POWER_OVERHEAD;
		event_ovrhead.ap_pwrf_delay = UNITY_PWRF_OVERHEAD;
		event_ovrhead.ap_saps_delay = UNITY_SAPS_OVERHEAD;
		event_ovrhead.ap_offset_delay = UNITY_OFFSET_OVERHEAD;
		event_ovrhead.ap_offset_ltch_delay = UNITY_OFFSET_LTCH_OVERHEAD;
		event_ovrhead.ap_wfg_start_delay = UNITY_WFG_START_OVERHEAD;
		event_ovrhead.ap_wfg_vstart_delay = UNITY_WFG_VSTART_OVERHEAD;
		event_ovrhead.ap_wfg_istart_delay = UNITY_WFG_ISTART_OVERHEAD;
		event_ovrhead.ap_wfg_offset_delay = UNITY_WFG_OFFSET_OVERHEAD;
		event_ovrhead.ap_wfg_stop_delay = UNITY_WFG_STOP_OVERHEAD;
		event_ovrhead.ap_wfg2_start_delay = UNITY_WFG2_START_OVERHEAD;
		event_ovrhead.ap_wfg2_offset_delay = UNITY_WFG2_OFFSET_OVERHEAD;
		event_ovrhead.ap_wfg2_stop_delay = UNITY_WFG2_STOP_OVERHEAD;
		event_ovrhead.ap_wfg3_start_delay = UNITY_WFG3_START_OVERHEAD;
		event_ovrhead.ap_wfg3_offset_delay = UNITY_WFG3_OFFSET_OVERHEAD;
		event_ovrhead.ap_wfg3_stop_delay = UNITY_WFG3_STOP_OVERHEAD;
		event_ovrhead.ap_prg_start_delay = UNITY_PRG_START_OVERHEAD;
		event_ovrhead.ap_prg_stop_delay = UNITY_PRG_STOP_OVERHEAD;
		event_ovrhead.ap_spnlck_start_delay = UNITY_SPNLCK_START_OVERHEAD;
		event_ovrhead.ap_spnlck_stop_delay = UNITY_SPNLCK_STOP_OVERHEAD;
		event_ovrhead.ap_spn2lck_start_delay = UNITY_SPN2LCK_START_OVERHEAD;
		event_ovrhead.ap_spn2lck_stop_delay = UNITY_SPN2LCK_STOP_OVERHEAD;
		if ((grt == 'p') || (grt == 'q'))
		{
		  event_ovrhead.gradient_delay = UNITY_P_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITY_P_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = UNITY_P_GRADIENT*ngrad;
		}
		else if (grt == 'l')
		{
		  event_ovrhead.gradient_delay = UNITY_L_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITY_L_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = UNITY_L_GRADIENT*ngrad;
		}
		else if ((grt == 'w') || (grt == 's'))
		{
		  event_ovrhead.gradient_delay = UNITY_W_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITY_W_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = UNITY_W_GRADIENT*ngrad;
		}
		else if ((grt == 'c') || (grt == 'd') || (grt == 't') || (grt == 'u'))
		{
		  event_ovrhead.gradient_delay = UNITYPLUS_T_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITYPLUS_T_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = 
						UNITYPLUS_T_GRADIENT*ngrad;
		}
		else
		{
		  event_ovrhead.gradient_delay = 0.0;
		  event_ovrhead.vagradient_delay = 0.0;
		  event_ovrhead.oblique_gradient_delay = 0.0;
		}
		event_ovrhead.acquire_start_delay = 
						UNITY_ACQUIRE_START_OVERHEAD;
		event_ovrhead.acquire_stop_delay = UNITY_ACQUIRE_STOP_OVERHEAD;
		event_ovrhead.ap_setdecmod_delay = UNITY_SETDECMOD_OVERHEAD;
		event_ovrhead.ap_decmodfreq_delay = UNITY_DECMODFREQ_OVERHEAD;
		event_ovrhead.prg_offset_delay = UNITY_PRG_OFFSET_OVERHEAD;
	break;
   case UNITYPLUS:
		event_ovrhead.ap_power_delay = UNITYPLUS_POWER_OVERHEAD;
		event_ovrhead.ap_pwrf_delay = UNITYPLUS_PWRF_OVERHEAD;
		event_ovrhead.ap_saps_delay = UNITYPLUS_SAPS_OVERHEAD;
		event_ovrhead.ap_offset_delay = UNITYPLUS_OFFSET_OVERHEAD;
		event_ovrhead.ap_offset_ltch_delay = UNITYPLUS_OFFSET_LTCH_OVERHEAD;
		event_ovrhead.ap_wfg_start_delay = UNITYPLUS_WFG_START_OVERHEAD;
	      event_ovrhead.ap_wfg_vstart_delay = UNITYPLUS_WFG_VSTART_OVERHEAD;
	      event_ovrhead.ap_wfg_istart_delay = UNITYPLUS_WFG_ISTART_OVERHEAD;
	      event_ovrhead.ap_wfg_offset_delay = UNITYPLUS_WFG_OFFSET_OVERHEAD;
		event_ovrhead.ap_wfg_stop_delay = UNITYPLUS_WFG_STOP_OVERHEAD;
		event_ovrhead.ap_wfg2_start_delay = UNITYPLUS_WFG2_START_OVERHEAD;
	      event_ovrhead.ap_wfg2_offset_delay = UNITYPLUS_WFG2_OFFSET_OVERHEAD;
		event_ovrhead.ap_wfg2_stop_delay = UNITYPLUS_WFG2_STOP_OVERHEAD;
		event_ovrhead.ap_wfg3_start_delay = UNITYPLUS_WFG3_START_OVERHEAD;
	      event_ovrhead.ap_wfg3_offset_delay = UNITYPLUS_WFG3_OFFSET_OVERHEAD;
		event_ovrhead.ap_wfg3_stop_delay = UNITYPLUS_WFG3_STOP_OVERHEAD;
		event_ovrhead.ap_prg_start_delay = UNITYPLUS_PRG_START_OVERHEAD;
		event_ovrhead.ap_prg_stop_delay = UNITYPLUS_PRG_STOP_OVERHEAD;
		event_ovrhead.ap_spnlck_start_delay = UNITYPLUS_SPNLCK_START_OVERHEAD;
		event_ovrhead.ap_spnlck_stop_delay = UNITYPLUS_SPNLCK_STOP_OVERHEAD;
		event_ovrhead.ap_spn2lck_start_delay = UNITYPLUS_SPN2LCK_START_OVERHEAD;
		event_ovrhead.ap_spn2lck_stop_delay = UNITYPLUS_SPN2LCK_STOP_OVERHEAD;
		if ((grt == 'p') || (grt == 'q'))
		{
		  event_ovrhead.gradient_delay = UNITYPLUS_P_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITYPLUS_P_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = 
					UNITYPLUS_P_GRADIENT*ngrad;
		}
		else if (grt == 'l')
		{
		  event_ovrhead.gradient_delay = UNITYPLUS_L_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITYPLUS_L_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = 
					UNITYPLUS_L_GRADIENT*ngrad;
		}
		else if ((grt == 'w') || (grt == 's'))
		{
		  event_ovrhead.gradient_delay = UNITYPLUS_W_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITYPLUS_W_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = 
						UNITYPLUS_W_GRADIENT*ngrad;
		}
		else if ((grt == 'c') || (grt == 'd') || (grt == 't') || (grt == 'u'))
		{
		  event_ovrhead.gradient_delay = UNITYPLUS_T_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITYPLUS_T_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = 
						UNITYPLUS_T_GRADIENT*ngrad;
		}
		else
		{
		  event_ovrhead.gradient_delay = 0.0;
		  event_ovrhead.vagradient_delay = 0.0;
		  event_ovrhead.oblique_gradient_delay = 0.0;
		}
		event_ovrhead.acquire_start_delay = 
					UNITYPLUS_ACQUIRE_START_OVERHEAD;
		event_ovrhead.acquire_stop_delay = 
					UNITYPLUS_ACQUIRE_STOP_OVERHEAD;
		event_ovrhead.ap_setdecmod_delay = UNITYPLUS_SETDECMOD_OVERHEAD;
		event_ovrhead.ap_decmodfreq_delay = 
					UNITYPLUS_DECMODFREQ_OVERHEAD;
		event_ovrhead.prg_offset_delay = UNITYPLUS_PRG_OFFSET_OVERHEAD;
	break;
   case INOVA:
		event_ovrhead.ap_power_delay = INOVA_POWER_OVERHEAD;
		event_ovrhead.ap_pwrf_delay = INOVA_PWRF_OVERHEAD;
		event_ovrhead.ap_saps_delay = INOVA_SAPS_OVERHEAD;
		event_ovrhead.ap_offset_delay = INOVA_OFFSET_OVERHEAD;
		event_ovrhead.ap_offset_ltch_delay = INOVA_OFFSET_LTCH_OVERHEAD;
		event_ovrhead.ap_wfg_start_delay = INOVA_WFG_START_OVERHEAD;
		event_ovrhead.ap_wfg_vstart_delay = INOVA_WFG_VSTART_OVERHEAD;
		event_ovrhead.ap_wfg_istart_delay = INOVA_WFG_ISTART_OVERHEAD;
	      event_ovrhead.ap_wfg_offset_delay = INOVA_WFG_OFFSET_OVERHEAD;
		event_ovrhead.ap_wfg_stop_delay = INOVA_WFG_STOP_OVERHEAD;
		event_ovrhead.ap_wfg2_start_delay = INOVA_WFG2_START_OVERHEAD;
	      event_ovrhead.ap_wfg2_offset_delay = INOVA_WFG2_OFFSET_OVERHEAD;
		event_ovrhead.ap_wfg2_stop_delay = INOVA_WFG2_STOP_OVERHEAD;
		event_ovrhead.ap_wfg3_start_delay = INOVA_WFG3_START_OVERHEAD;
	      event_ovrhead.ap_wfg3_offset_delay = INOVA_WFG3_OFFSET_OVERHEAD;
		event_ovrhead.ap_wfg3_stop_delay = INOVA_WFG3_STOP_OVERHEAD;
		event_ovrhead.ap_prg_start_delay = INOVA_PRG_START_OVERHEAD;
		event_ovrhead.ap_prg_stop_delay = INOVA_PRG_STOP_OVERHEAD;
		event_ovrhead.ap_spnlck_start_delay = INOVA_SPNLCK_START_OVERHEAD;
		event_ovrhead.ap_spnlck_stop_delay = INOVA_SPNLCK_STOP_OVERHEAD;
		event_ovrhead.ap_spn2lck_start_delay = INOVA_SPN2LCK_START_OVERHEAD;
		event_ovrhead.ap_spn2lck_stop_delay = INOVA_SPN2LCK_STOP_OVERHEAD;
		if ((grt == 'p') || (grt == 'q'))
		{
		  event_ovrhead.gradient_delay = INOVA_P_GRADIENT;
		  event_ovrhead.vagradient_delay = INOVA_P_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = INOVA_P_GRADIENT*ngrad;
		}
		else if (grt == 'l')
		{
		  event_ovrhead.gradient_delay = INOVA_L_GRADIENT;
		  event_ovrhead.vagradient_delay = INOVA_L_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = INOVA_L_GRADIENT*ngrad;
		}
		else if ((grt == 'w') || (grt == 's'))
		{
		  event_ovrhead.gradient_delay = INOVA_W_GRADIENT;
		  event_ovrhead.vagradient_delay = INOVA_W_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = INOVA_W_GRADIENT*ngrad;
		}
		else if ((grt == 'c') || (grt == 'd') || (grt == 't') || (grt == 'u'))
		{
		  event_ovrhead.gradient_delay = INOVA_T_GRADIENT;
		  event_ovrhead.vagradient_delay = INOVA_T_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = INOVA_T_GRADIENT*ngrad;
		}
		else
		{
		  event_ovrhead.gradient_delay = 0.0;
		  event_ovrhead.vagradient_delay = 0.0;
		  event_ovrhead.oblique_gradient_delay = 0.0;
		}
		event_ovrhead.acquire_start_delay = 
					INOVA_ACQUIRE_START_OVERHEAD;
		event_ovrhead.acquire_stop_delay = INOVA_ACQUIRE_STOP_OVERHEAD;
		event_ovrhead.ap_setdecmod_delay = INOVA_SETDECMOD_OVERHEAD;
		event_ovrhead.ap_decmodfreq_delay = INOVA_DECMODFREQ_OVERHEAD;
		event_ovrhead.prg_offset_delay = INOVA_PRG_OFFSET_OVERHEAD;
	break;
   default:
		event_ovrhead.ap_power_delay = VXR_POWER_OVERHEAD;
		event_ovrhead.ap_pwrf_delay = VXR_PWRF_OVERHEAD;
		event_ovrhead.ap_saps_delay = VXR_SAPS_OVERHEAD;
		event_ovrhead.ap_offset_delay = VXR_OFFSET_OVERHEAD;
		event_ovrhead.ap_offset_ltch_delay = VXR_OFFSET_LTCH_OVERHEAD;
		event_ovrhead.ap_wfg_start_delay = VXR_WFG_START_OVERHEAD;
		event_ovrhead.ap_wfg_vstart_delay = VXR_WFG_VSTART_OVERHEAD;
		event_ovrhead.ap_wfg_stop_delay = VXR_WFG_STOP_OVERHEAD;
		event_ovrhead.ap_prg_start_delay = VXR_PRG_START_OVERHEAD;
		event_ovrhead.ap_prg_stop_delay = VXR_PRG_STOP_OVERHEAD;
		event_ovrhead.ap_spnlck_start_delay = VXR_SPNLCK_START_OVERHEAD;
		event_ovrhead.ap_spnlck_stop_delay = VXR_SPNLCK_STOP_OVERHEAD;
		event_ovrhead.ap_spn2lck_start_delay = VXR_SPN2LCK_START_OVERHEAD;
		event_ovrhead.ap_spn2lck_stop_delay = VXR_SPN2LCK_STOP_OVERHEAD;
		if ((grt == 'p') || (grt == 'q'))
		{
		  event_ovrhead.gradient_delay = UNITY_P_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITY_P_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = UNITY_P_GRADIENT*ngrad;
		}
		else if (grt == 'l')
		{
		  event_ovrhead.gradient_delay = UNITY_L_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITY_L_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = UNITY_L_GRADIENT*ngrad;
		}
		else if ((grt == 'w') || (grt == 's'))
		{
		  event_ovrhead.gradient_delay = UNITY_W_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITY_W_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = UNITY_W_GRADIENT*ngrad;
		}
		else if ((grt == 'c') || (grt == 'd') || (grt == 't') || (grt == 'u'))
		{
		  event_ovrhead.gradient_delay = UNITYPLUS_T_GRADIENT;
		  event_ovrhead.vagradient_delay = UNITYPLUS_T_GRADIENT*ngrad;
		  event_ovrhead.oblique_gradient_delay = 
						UNITYPLUS_T_GRADIENT*ngrad;
		}
		else
		{
		  event_ovrhead.gradient_delay = 0.0;
		  event_ovrhead.vagradient_delay = 0.0;
		  event_ovrhead.oblique_gradient_delay = 0.0;
		}
		event_ovrhead.acquire_start_delay = VXR_ACQUIRE_START_OVERHEAD;
		event_ovrhead.acquire_stop_delay = VXR_ACQUIRE_STOP_OVERHEAD;
		event_ovrhead.ap_setdecmod_delay = 0.0;
		event_ovrhead.ap_decmodfreq_delay = 0.0;
		event_ovrhead.prg_offset_delay = VXR_PRG_OFFSET_OVERHEAD;
	  break;
   }
}

double eventovrhead(int event_type)
{
  double overheadtime;

   switch(event_type)
   {
   case POWER_EVENT:
	overheadtime = event_ovrhead.ap_power_delay;
	break;
   case PWRF_EVENT:
	overheadtime = event_ovrhead.ap_pwrf_delay;
	break;
   case SAPS_EVENT:
	overheadtime = event_ovrhead.ap_saps_delay;
	break;
   case OFFSET_FREQ_EVENT:
	overheadtime = event_ovrhead.ap_offset_delay;
	break;
   case OFFSET_LATCH_EVENT:
	overheadtime = event_ovrhead.ap_offset_ltch_delay;
	break;
   case WFG_START_EVENT:
	overheadtime = event_ovrhead.ap_wfg_start_delay;
	break;
   case WFG_VSTART_EVENT:
	overheadtime = event_ovrhead.ap_wfg_vstart_delay;
	break;
   case WFG_ISTART_EVENT:
	overheadtime = event_ovrhead.ap_wfg_istart_delay;
	break;
   case WFG_OFFSET_EVENT:
	overheadtime = event_ovrhead.ap_wfg_offset_delay;
	break;
   case WFG_STOP_EVENT:
	overheadtime = event_ovrhead.ap_wfg_stop_delay;
	break;
   case WFG2_START_EVENT:
	overheadtime = event_ovrhead.ap_wfg2_start_delay;
	break;
   case WFG2_OFFSET_EVENT:
	overheadtime = event_ovrhead.ap_wfg2_offset_delay;
	break;
   case WFG2_STOP_EVENT:
	overheadtime = event_ovrhead.ap_wfg2_stop_delay;
	break;
   case WFG3_START_EVENT:
	overheadtime = event_ovrhead.ap_wfg3_start_delay;
	break;
   case WFG3_OFFSET_EVENT:
	overheadtime = event_ovrhead.ap_wfg3_offset_delay;
	break;
   case WFG3_STOP_EVENT:
	overheadtime = event_ovrhead.ap_wfg3_stop_delay;
	break;
   case PRG_START_EVENT:
	overheadtime = event_ovrhead.ap_prg_start_delay;
	break;
   case PRG_STOP_EVENT:
	overheadtime = event_ovrhead.ap_prg_stop_delay;
	break;
   case SPNLCK_START_EVENT:
	overheadtime = event_ovrhead.ap_spnlck_start_delay;
	break;
   case SPNLCK_STOP_EVENT:
	overheadtime = event_ovrhead.ap_spnlck_stop_delay;
	break;
   case SPN2LCK_START_EVENT:
	overheadtime = event_ovrhead.ap_spn2lck_start_delay;
	break;
   case SPN2LCK_STOP_EVENT:
	overheadtime = event_ovrhead.ap_spn2lck_stop_delay;
	break;
   case GRADIENT_EVENT:
	overheadtime = event_ovrhead.gradient_delay;
	break;
   case VAGRADIENT_EVENT:
	overheadtime = event_ovrhead.vagradient_delay;
	break;
   case OBLIQUEGRADIENT_EVENT:
	overheadtime = event_ovrhead.oblique_gradient_delay;
	break;
   case ACQUIRE_START_EVENT:
	overheadtime = event_ovrhead.acquire_start_delay;
	break;
   case ACQUIRE_STOP_EVENT:
	overheadtime = event_ovrhead.acquire_stop_delay;
	break;
   case SETDECMOD_EVENT:
	overheadtime = event_ovrhead.ap_setdecmod_delay;
	break;
   case DECMODFREQ_EVENT:
	overheadtime = event_ovrhead.ap_decmodfreq_delay;
	break;
   case PRG_OFFSET_EVENT:
	overheadtime = event_ovrhead.prg_offset_delay;
	break;

   default:
	overheadtime = 0.0;
	break;
   }
   return( overheadtime );
}
