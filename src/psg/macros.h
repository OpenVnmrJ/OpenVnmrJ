/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef DPS

/*  if oopc.h not included then  define Object type */
#ifndef OBJECTDEFINED
#define OBJECTDEFINED

/* Object Handle Structure */
typedef int (*Functionp)();
typedef struct {Functionp dispatch; char *objname; } *Object;

#endif

#define PULSE_WIDTH		1
#define PULSE_PRE_ROFF		2
#define PULSE_POST_ROFF		3
#define PULSE_DEVICE		4
#define PULSE_PHASE		5
#define PULSE_PHASE_TABLE	6

#define DELAY_TIME		21

#define OFFSET_DEVICE		31
#define OFFSET_FREQ		32

#define RTDELAY_MODE		40
#define RTDELAY_INCR		41
#define RTDELAY_TBASE		42
#define RTDELAY_COUNT		43
#define RTDELAY_HSLINES		44

#define FREQSWEEP_CENTER	50
#define FREQSWEEP_WIDTH		51
#define FREQSWEEP_NP		52
#define FREQSWEEP_MODE		53
#define FREQSWEEP_INCR		54
#define FREQSWEEP_DEVICE	55

#define SLIDER_LABEL		101
#define SLIDER_SCALE		102
#define SLIDER_MAX		103
#define SLIDER_MIN		104
#define SLIDER_UNITS		105

#define TYPE_PULSE		111
#define TYPE_DELAY		111

#define TYPE_TOF		121
#define TYPE_DOF		122
#define TYPE_DOF2		123
#define	TYPE_DOF3		124

#define TYPE_FREQSWPWIDTH	131
#define TYPE_FREQSWPCENTER	132

#define TYPE_RTPARAM		141

#define  DELAY1 0
#define  DELAY2 1
#define  DELAY3 2
#define  DELAY4 3
#define  DELAY5 4

#define NSEC 1
#define USEC 2
#define MSEC 3
#define SEC  4
/* Note: defines duplicated in acodes.h */
#define         TCNT 1
#define         HSLINE 2
#define         TCNT_HSLINE 3
#define         TWRD 4
#define         TWRD_HSLINE 5
#define         TWRD_TCNT 6

/*------ SISCO defines	---------------------------------------*/
#define MAX_POWER		127.0

/* ADC Overflow Checking Keywords for ADC_overflow_check() */
#define ADC_CHECK_ENABLE	201	/* Enable/Disable checking */
#define ADC_CHECK_NPOINTS	202	/* Number of points to check */
#define ADC_CHECK_OFFSET	203	/* Number of points to skip at	*/
					/*  beginning of buffer		*/

/* Round to nearest integer--for internal use */
#define IRND(x)		((x) >= 0 ? ((int)((x)+0.5)) : (-(int)(-(x)+0.5)))
/* Round to nearest positive integer */
#define URND(x)		((x) > 0 ? ((unsigned)((x)+0.5)) : 0)

/*------  End SISCO defines	--------------------------------*/

/*---------------------------------------------------------------*/
#define initval(value,index)					\
			G_initval((double)(value),index)

#define F_initval(value,index)					\
			G_initval((double)(value),index)

#define	obsblank()						\
			blankon(OBSch)

#define	obsunblank()						\
			blankoff(OBSch)

#define	decblank()						\
			blankon(DECch)

#define	decunblank()						\
			blankoff(DECch)

#define	dec2blank()						\
			blankon(DEC2ch)

#define	dec2unblank()						\
			blankoff(DEC2ch)

#define	dec3blank()						\
			blankon(DEC3ch)

#define	dec3unblank()						\
			blankoff(DEC3ch)

#define decpulse(decpulse,phaseptr)				\
			G_Pulse(PULSE_DEVICE,	DECch,		\
			      PULSE_WIDTH,	decpulse,	\
			      pulse_phase_type(phaseptr),	phaseptr,	\
			      PULSE_PRE_ROFF,	0.0,		\
			      PULSE_POST_ROFF,	0.0,		\
			      0)

#define decrgpulse(pulsewidth, phaseptr, rx1, rx2)		\
			G_Pulse(PULSE_DEVICE,	DECch,		\
			      PULSE_WIDTH,	pulsewidth,	\
			      pulse_phase_type(phaseptr),	phaseptr,	\
			      PULSE_PRE_ROFF,	rx1,		\
			      PULSE_POST_ROFF,	rx2,		\
			      0)

#define dec2rgpulse(pulsewidth, phaseptr, rx1, rx2)		\
			G_Pulse(PULSE_DEVICE,	DEC2ch,		\
			      PULSE_WIDTH,	pulsewidth,	\
			      pulse_phase_type(phaseptr),	phaseptr,	\
			      PULSE_PRE_ROFF,	rx1,		\
			      PULSE_POST_ROFF,	rx2,		\
			      0)

#define dec3rgpulse(pulsewidth, phaseptr, rx1, rx2)		\
			G_Pulse(PULSE_DEVICE,	DEC3ch,		\
			      PULSE_WIDTH,	pulsewidth,	\
			      pulse_phase_type(phaseptr),	phaseptr,	\
			      PULSE_PRE_ROFF,	rx1,		\
			      PULSE_POST_ROFF,	rx2,		\
			      0)

#define dec4rgpulse(pulsewidth, phaseptr, rx1, rx2)		\
			G_Pulse(PULSE_DEVICE,	DEC4ch,		\
			      PULSE_WIDTH,	pulsewidth,	\
			      pulse_phase_type(phaseptr),	phaseptr,	\
			      PULSE_PRE_ROFF,	rx1,		\
			      PULSE_POST_ROFF,	rx2,		\
			      0)

#define delay(time)						\
			G_Delay(DELAY_TIME,	time,		\
			      0)

#define idecpulse(decpulse,phaseptr,string)			\
			G_Pulse(PULSE_DEVICE,	DECch,		\
			      PULSE_WIDTH,	decpulse,	\
			      pulse_phase_type(phaseptr),	phaseptr,	\
			      PULSE_PRE_ROFF,	0.0,		\
			      PULSE_POST_ROFF,	0.0,		\
			      SLIDER_LABEL,	string,		\
			      0)

#define idecrgpulse(pulsewidth, phaseptr, rx1, rx2,string)	\
			G_Pulse(PULSE_DEVICE,	DECch,		\
			      PULSE_WIDTH,	pulsewidth,	\
			      pulse_phase_type(phaseptr),	phaseptr,	\
			      PULSE_PRE_ROFF,	rx1,		\
			      PULSE_POST_ROFF,	rx2,		\
			      SLIDER_LABEL,	string,		\
			      0)

#define idec2rgpulse(pulsewidth, phaseptr, rx1, rx2,string)	\
			G_Pulse(PULSE_DEVICE,	DEC2ch,		\
			      PULSE_WIDTH,	pulsewidth,	\
			      pulse_phase_type(phaseptr),	phaseptr,	\
			      PULSE_PRE_ROFF,	rx1,		\
			      PULSE_POST_ROFF,	rx2,		\
			      SLIDER_LABEL,	string,		\
			      0)

#define idec3rgpulse(pulsewidth, phaseptr, rx1, rx2,string)	\
			G_Pulse(PULSE_DEVICE,	DEC3ch,		\
			      PULSE_WIDTH,	pulsewidth,	\
			      pulse_phase_type(phaseptr),	phaseptr,	\
			      PULSE_PRE_ROFF,	rx1,		\
			      PULSE_POST_ROFF,	rx2,		\
			      SLIDER_LABEL,	string,		\
			      0)

#define idelay(time,string)					\
			G_Delay(DELAY_TIME,	time,		\
			      SLIDER_LABEL,	string,		\
			      0)

#define iobspulse(string)					\
			G_Pulse(PULSE_DEVICE,	OBSch,		\
			      SLIDER_LABEL,	string,		\
			      0)

#define ipulse(pulsewidth,phaseptr,string)			\
			G_Pulse(PULSE_DEVICE,	OBSch,		\
			      PULSE_WIDTH,	pulsewidth,	\
			      pulse_phase_type(phaseptr),	phaseptr,	\
			      SLIDER_LABEL,	string,		\
			      0)

#define obsstepsize(stepval)                                  \
                        stepsize((double)(stepval),OBSch)

#define decstepsize(stepval)                                 \
                        stepsize((double)(stepval),DECch)

#define dec2stepsize(stepval)                                 \
                        stepsize((double)(stepval),DEC2ch)

#define dec3stepsize(stepval)                                 \
                        stepsize((double)(stepval),DEC3ch)

#define obspower(reqpower)                                  \
                        rlpower((double)(reqpower),OBSch)

#define decpower(reqpower)                                 \
                        rlpower((double)(reqpower),DECch)

#define dec2power(reqpower)                                 \
                        rlpower((double)(reqpower),DEC2ch)

#define dec3power(reqpower)                                 \
                        rlpower((double)(reqpower),DEC3ch)

#define dec4power(reqpower)                                 \
                        rlpower((double)(reqpower),DEC4ch)

#define obspwrf(reqpower)					\
                        rlpwrf((double)(reqpower),OBSch)

#define decpwrf(reqpower)					\
                        rlpwrf((double)(reqpower),DECch)

#define dec2pwrf(reqpower)					\
                        rlpwrf((double)(reqpower),DEC2ch)

#define dec3pwrf(reqpower)					\
                        rlpwrf((double)(reqpower),DEC3ch)

#define ipwrf(value,device,string)				\
			G_Power(POWER_VALUE,	value,		\
			      POWER_DEVICE,	device,		\
			      SLIDER_LABEL,	string,		\
			      0)

#define ipwrm(value,device,string)				\
			G_Power(POWER_VALUE,	value,		\
			      POWER_DEVICE,	device,		\
			      SLIDER_LABEL,	string,		\
			      0)

#define irgpulse(pulsewidth, phaseptr, rx1, rx2,string)		\
			G_Pulse(PULSE_DEVICE,	OBSch,		\
			      PULSE_WIDTH,	pulsewidth,	\
			      pulse_phase_type(phaseptr),	phaseptr,	\
			      PULSE_PRE_ROFF,	rx1,		\
			      PULSE_POST_ROFF,	rx2,		\
			      SLIDER_LABEL,	string,		\
			      0)


/*  offset is still a subroutine because defining it as a Macro
    wrecks havoc on any program that uses `offset' as a symbol.  */

#define obsoffset( value )			\
			G_Offset( OFFSET_DEVICE,OBSch,		\
				  OFFSET_FREQ,  value,		\
				  0 )

#define decoffset( value )			\
			G_Offset( OFFSET_DEVICE,DECch,		\
				  OFFSET_FREQ,  value,		\
				  0 )

#define dec2offset( value )			\
			G_Offset( OFFSET_DEVICE,DEC2ch,		\
				  OFFSET_FREQ,  value,		\
				  0 )

#define dec3offset( value )			\
			G_Offset( OFFSET_DEVICE,DEC3ch,		\
				  OFFSET_FREQ,  value,		\
				  0 )

#define dec4offset( value )			\
			G_Offset( OFFSET_DEVICE,DEC4ch,		\
				  OFFSET_FREQ,  value,		\
				  0 )

#define ioffset( value, device, string )			\
			G_Offset( OFFSET_DEVICE,device,		\
				  OFFSET_FREQ,  value,		\
				  SLIDER_LABEL, string,		\
				  0 )

#define obspulse()						\
			G_Pulse(PULSE_DEVICE,	OBSch,		\
			      0)

#define pulse(pulsewidth,phaseptr)				\
			G_Pulse(PULSE_DEVICE,	OBSch,		\
			      PULSE_WIDTH,	pulsewidth,	\
			      pulse_phase_type(phaseptr),	phaseptr,	\
			      0)

#define rgpulse(pulsewidth, phaseptr, rx1, rx2)			\
			G_Pulse(PULSE_DEVICE,	OBSch,		\
			      PULSE_WIDTH,	pulsewidth,	\
			      pulse_phase_type(phaseptr),	phaseptr,	\
			      PULSE_PRE_ROFF,	rx1,		\
			      PULSE_POST_ROFF,	rx2,		\
			      0)

#define pwrm(rtparam,device)					\
			pwrf(rtparam,device)

#define rlpwrm(value,device)					\
			rlpwrf(value,device)

#define rotorperiod(rtparam)					\
			hardware_get(HS_Rotor,			\
			      GET_RTVALUE,rtparam,		\
			      0)

#define rotorsync(rtparam)					\
			sync_on_event(HS_Rotor,			\
			      SET_RTPARAM,	rtparam,	\
			      0)

#define setautoincrement(tname)					\
			Table[tname - BASEINDEX]->auto_inc =	\
			TRUE

#define setdivnfactor(tname, dfactor) 				\
			Table[tname - BASEINDEX]->divn_factor =	\
			dfactor

#define tsadd(t1name, sval, mval)                               \
                        tablesop(TADD, t1name, sval, mval)

#define tsdiv(tname, sval, mval)                                \
                        tablesop(TDIV, tname, sval, mval)
                               
#define tsmult(tname, sval, mval)                               \
                        tablesop(TMULT, tname, sval, mval)
 
#define tssub(tname, sval, mval)                                \
                        tablesop(TSUB, tname, sval, mval)
 
#define ttadd(t1name, t2name, mval)                             \
                        tabletop(TADD, t1name, t2name, mval)

#define ttdiv(t1name, t2name, mval)                             \
                        tabletop(TDIV, t1name, t2name, mval)
 
#define ttmult(t1name, t2name, mval)                            \
                        tabletop(TMULT, t1name, t2name, mval)
 
#define ttsub(t1name, t2name, mval)				\
			tabletop(TSUB, t1name, t2name, mval)

#define xgate(value)						\
			sync_on_event(Ext_Trig,			\
			      SET_DBVALUE, (double)(value),	\
			      0)

#define genqdphase(phaseptr, rf_device)				\
	if ( (rf_device > 0) && (rf_device <= NUMch) ) 		\
	{							\
	   SetRFChanAttr(RF_Channel[rf_device],			\
			SET_RTPHASE90, phaseptr,		\
			0);					\
	}							\
	else							\
	{							\
	   char msge[128];					\
           sprintf(msge,"genqdphase: device #%d is not within bounds 1 - %d\n", \
			rf_device, NUMch);			\
	   text_error(msge);					\
      	   psg_abort(1);						\
	}

#define txphase(phaseptr)					\
	SetRFChanAttr(RF_Channel[OBSch],			\
			 phase_var_type(phaseptr), phaseptr,		\
			 0)

#define decphase(phaseptr)					\
	SetRFChanAttr(RF_Channel[DECch],			\
			 phase_var_type(phaseptr), phaseptr,		\
			 0)

#define dec2phase(phaseptr)					\
	SetRFChanAttr(RF_Channel[DEC2ch],			\
			 phase_var_type(phaseptr), phaseptr,		\
			 0)

#define dec3phase(phaseptr)					\
	SetRFChanAttr(RF_Channel[DEC3ch],			\
			 phase_var_type(phaseptr), phaseptr,		\
			 0)

#define dec4phase(phaseptr)					\
	SetRFChanAttr(RF_Channel[DEC4ch],			\
			 phase_var_type(phaseptr), phaseptr,		\
			 0)

#define genpulse(pulsewidth, phaseptr, rf_device)		\
	if ( (rf_device > 0) && (rf_device <= NUMch) )		\
	{							\
	   G_Pulse(PULSE_DEVICE,	rf_device,		\
		      PULSE_WIDTH,	pulsewidth,		\
		      PULSE_PHASE,	phaseptr,		\
		      0);					\
	}							\
	else							\
	{							\
	   char msge[128];					\
           sprintf(msge,"genpulse: device #%d is not within bounds 1 - %d\n", \
			rf_device, NUMch);			\
	   text_error(msge);					\
      	   psg_abort(1);						\
	}

#define genrgpulse(pulsewidth, phaseptr, rx1, rx2, rf_device)	\
	if ( (rf_device > 0) && (rf_device <= NUMch) ) 	\
	{							\
	   G_Pulse(PULSE_DEVICE,	rf_device,		\
		      PULSE_WIDTH,	pulsewidth,		\
		      PULSE_PHASE,	phaseptr,		\
		      PULSE_PRE_ROFF,	rx1,			\
		      PULSE_POST_ROFF,	rx2,			\
		      0);					\
	}							\
	else							\
	{							\
	   char msge[128];					\
           sprintf(msge,"genrgpulse: device #%d is not within bounds 1 - %d\n", \
			rf_device, NUMch);			\
	   text_error(msge);					\
      	   psg_abort(1);						\
	}

#define xmtrphase(phaseptr)					\
	SetRFChanAttr(RF_Channel[OBSch],			\
			 SET_RTPHASE,	phaseptr,		\
			 0)

#define dcplrphase(phaseptr)					\
	SetRFChanAttr(RF_Channel[DECch],			\
			 SET_RTPHASE,	phaseptr,		\
			 0)

#define dcplr2phase(phaseptr)					\
	SetRFChanAttr(RF_Channel[DEC2ch],			\
			 SET_RTPHASE,	phaseptr,		\
			 0)

#define dcplr3phase(phaseptr)					\
	SetRFChanAttr(RF_Channel[DEC3ch],			\
			 SET_RTPHASE,	phaseptr,		\
			 0)

#define gensaphase(phaseptr, rf_device)				\
	if ( (rf_device > 0) && (rf_device <= NUMch) ) 		\
	{							\
	   SetRFChanAttr(RF_Channel[rf_device],			\
			SET_RTPHASE, phaseptr,			\
			0);					\
	}							\
	else							\
	{							\
	   char msge[128];					\
           sprintf(msge,"gensaphase: device #%d is not within bounds 1 - %d\n", \
			rf_device, NUMch);			\
	   text_error(msge);					\
      	   psg_abort(1);						\
	}

#define simpulse(pw1, pw2, phs1, phs2, rx1, rx2)		\
		G_Simpulse(PULSE_DEVICE,	(int)(OBSch),	\
						(int)(DECch),	\
						(int)(0),	\
			   PULSE_PHASE,		(int)(phs1),	\
						(int)(phs2),	\
						(int)(0),	\
			   PULSE_WIDTH,		(double)(pw1),	\
						(double)(pw2),	\
						(double)(0.0),	\
			   PULSE_PRE_ROFF,	(double)(rx1),	\
			   PULSE_POST_ROFF,	(double)(rx2),	\
			   NULL)

#define sim3pulse(pw1, pw2, pw3, phs1, phs2, phs3, rx1, rx2)	\
		G_Simpulse(PULSE_DEVICE,	(int)(OBSch),	\
						(int)(DECch),	\
						(int)(DEC2ch),	\
						(int)(0),	\
			   PULSE_PHASE,		(int)(phs1),	\
						(int)(phs2),	\
						(int)(phs3),	\
						(int)(0),	\
			   PULSE_WIDTH,		(double)(pw1),	\
						(double)(pw2),	\
						(double)(pw3),	\
						(double)(0.0),	\
			   PULSE_PRE_ROFF,	(double)(rx1),	\
			   PULSE_POST_ROFF,	(double)(rx2),	\
			   NULL)

#define sim4pulse(pw1,pw2,pw3,pw4,phs1,phs2,phs3,phs4,rx1,rx2)	\
		G_Simpulse(PULSE_DEVICE,	(int)(OBSch),	\
						(int)(DECch),	\
						(int)(DEC2ch),	\
						(int)(DEC3ch),	\
						(int)(0),	\
			   PULSE_PHASE,		(int)(phs1),	\
						(int)(phs2),	\
						(int)(phs3),	\
						(int)(phs4),	\
						(int)(0),	\
			   PULSE_WIDTH,		(double)(pw1),	\
						(double)(pw2),	\
						(double)(pw3),	\
						(double)(pw4),	\
						(double)(0.0),	\
			   PULSE_PRE_ROFF,	(double)(rx1),	\
			   PULSE_POST_ROFF,	(double)(rx2),	\
			   NULL)
			

#define rfon(rf_device)						\
	if ( (rf_device > 0) && (rf_device <= NUMch) ) 		\
	{							\
	   SetRFChanAttr(RF_Channel[rf_device],			\
			SET_XMTRGATE, ON,			\
			0);					\
	}							\
	else							\
	{							\
	   char msge[128];					\
           sprintf(msge,"rfon: device #%d is not within bounds 1 - %d\n", \
			rf_device, NUMch);			\
	   text_error(msge);					\
      	   psg_abort(1);						\
	}

#define rfoff(rf_device)						\
	if ( (rf_device > 0) && (rf_device <= NUMch) ) 		\
	{							\
	   SetRFChanAttr(RF_Channel[rf_device],			\
			SET_XMTRGATE, OFF,			\
			0);					\
	}							\
	else							\
	{							\
	   char msge[128];					\
           sprintf(msge,"rfoff: device #%d is not within bounds 1 - %d\n", \
			rf_device, NUMch);			\
	   text_error(msge);					\
      	   psg_abort(1);						\
	}

#define xmtron()						\
	SetRFChanAttr(RF_Channel[OBSch], SET_XMTRGATE, ON, 0)

#define decon()							\
	SetRFChanAttr(RF_Channel[DECch], SET_XMTRGATE, ON, 0)

#define dec2on()                                                \
        SetRFChanAttr(RF_Channel[DEC2ch], SET_XMTRGATE, ON, 0)

#define dec3on()                                                \
        SetRFChanAttr(RF_Channel[DEC3ch], SET_XMTRGATE, ON, 0)

#define dec4on()                                                \
        SetRFChanAttr(RF_Channel[DEC4ch], SET_XMTRGATE, ON, 0)

#define xmtroff()						\
	SetRFChanAttr(RF_Channel[OBSch], SET_XMTRGATE, OFF, 0)

#define decoff()						\
	SetRFChanAttr(RF_Channel[DECch], SET_XMTRGATE, OFF, 0)

#define dec2off()                                               \
        SetRFChanAttr(RF_Channel[DEC2ch], SET_XMTRGATE, OFF, 0)
 
#define dec3off()                                               \
        SetRFChanAttr(RF_Channel[DEC3ch], SET_XMTRGATE, OFF, 0)

#define dec4off()                                               \
        SetRFChanAttr(RF_Channel[DEC4ch], SET_XMTRGATE, OFF, 0)

#define sp3on()		sp_on(3)
#define sp3off()	sp_off(3)
#define sp4on()		sp_on(4)
#define sp4off()	sp_off(4)
#define sp5on()		sp_on(5)
#define sp5off()	sp_off(5)
 
 
#define is_y(target)   ((target == 'y') || (target == 'Y'))
#define is_w(target)   ((target == 'w') || (target == 'W'))
#define is_r(target)   ((target == 'r') || (target == 'R'))
#define is_c(target)   ((target == 'c') || (target == 'C'))
#define is_d(target)   ((target == 'd') || (target == 'D'))
#define is_p(target)   ((target == 'p') || (target == 'P'))
#define is_q(target)   ((target == 'q') || (target == 'Q'))
#define is_t(target)   ((target == 't') || (target == 'T'))
#define is_u(target)   ((target == 'u') || (target == 'U'))
#define is_porq(target)((target == 'p') || (target == 'P') || (target == 'q') || (target == 'Q'))

#define anyrfwg   ( (is_y(rfwg[0])) || (is_y(rfwg[1])) || (is_y(rfwg[2])) || (is_y(rfwg[3])) )
#define anygradwg ((is_w(gradtype[0])) || (is_w(gradtype[1])) || (is_w(gradtype[2])))
#define anygradcrwg ((is_r(gradtype[0])) || (is_r(gradtype[1])) || (is_r(gradtype[2])))
#define anypfga   ((is_p(gradtype[0])) || (is_p(gradtype[1])) || (is_p(gradtype[2])))
#define anypfgw   ((is_q(gradtype[0])) || (is_q(gradtype[1])) || (is_q(gradtype[2])))
#define anypfg3 ((is_c(gradtype[0])) || (is_c(gradtype[1])) || (is_c(gradtype[2])) || (is_t(gradtype[0])) || (is_t(gradtype[1])) || (is_t(gradtype[2]))) 
#define anypfg3w ((is_d(gradtype[0])) || (is_d(gradtype[1])) || (is_d(gradtype[2])) || (is_u(gradtype[0])) || (is_u(gradtype[1])) || (is_u(gradtype[2])))  
#define anypfg	(anypfga || anypfgw || anypfg3 || anypfg3w)
#define anywg  (anyrfwg || anygradwg || anygradcrwg || anypfgw || anypfg3w)

#define iscp(a)  ((cpflag) ? zero : (a))


#define shaped_pulse(name, width, phase, rx1, rx2)		\
	newtrans ? genshaped_pulse(name, width, phase, rx1, rx2,\
			  0.0, 0.0, OBSch)			\
		:  S_shapedpulse(name,(double)(width), \
				phase,(double)(rx1),(double)(rx2))

#define decshaped_pulse(name, width, phase, rx1, rx2)		\
	newdec ? genshaped_pulse(name, width, phase, rx1, rx2,	\
			  0.0, 0.0, DECch)			\
		: S_decshapedpulse(name,(double)(width),	\
				phase,(double)(rx1),(double)(rx2))

#define dec2shaped_pulse(name, width, phase, rx1, rx2)		\
	genshaped_pulse(name, width, phase, rx1, rx2,		\
			  0.0, 0.0, DEC2ch)

#define dec3shaped_pulse(name, width, phase, rx1, rx2)		\
	genshaped_pulse(name, width, phase, rx1, rx2,		\
			  0.0, 0.0, DEC3ch)

#define simshaped_pulse(n1, n2, w1, w2, ph1, ph2, r1, r2)	\
	newtrans ? gensim3shaped_pulse(n1, n2, "", w1, w2, 0.0, ph1, ph2, \
			zero, r1, r2,0.0, 0.0, OBSch, DECch, DEC2ch)	\
			 : S_simshapedpulse(n1, n2,(double)(w1),	\
				(double)(w2), ph1, ph2,	\
				(double)(r1),(double)(r2))

#define sim3shaped_pulse(n1, n2, n3, w1, w2, w3, ph1, ph2, ph3, r1, r2)	\
	gensim3shaped_pulse(n1, n2, n3, w1, w2, w3, ph1, ph2,	\
			  ph3, r1, r2, 0.0, 0.0, OBSch, DECch,	\
			  DEC2ch)

#define shapedvpulse(name, width, rtamp, phase, rx1, rx2)		\
	genshaped_rtamppulse(name, (double)(width), rtamp, phase, \
		(double)(rx1),(double)(rx2), 0.0, 0.0, OBSch)			\

#define apshaped_pulse(name, width, phase, tbl1, tbl2, rx1, rx2) \
	gen_apshaped_pulse(name, width, phase, tbl1, tbl2, rx1, rx2, OBSch)

#define apshaped_decpulse(name, width, phase, tbl1, tbl2, rx1, rx2) \
	gen_apshaped_pulse(name, width, phase, tbl1, tbl2, rx1, rx2, DECch)

#define apshaped_dec2pulse(name, width, phase, tbl1, tbl2, rx1, rx2) \
	gen_apshaped_pulse(name, width, phase, tbl1, tbl2, rx1, rx2, DEC2ch)

#define spinlock(   name, pp_90, pp_res, phase, nloops)		\
	genspinlock(name, pp_90, pp_res, phase, nloops, OBSch)

#define decspinlock(name, pp_90, pp_res, phase, nloops)		\
	genspinlock(name, pp_90, pp_res, phase, nloops, DECch)

#define dec2spinlock(name, pp_90, pp_res, phase, nloops)	\
	genspinlock(name, pp_90, pp_res, phase, nloops, DEC2ch)

#define dec3spinlock(name, pp_90, pp_res, phase, nloops)	\
	genspinlock(name, pp_90, pp_res, phase, nloops, DEC3ch)

#define obsprgon(name, pp_90, pp_res)				\
	prg_dec_on(name, pp_90, pp_res, OBSch)

#define decprgon(name, pp_90, pp_res)				\
	prg_dec_on(name, pp_90, pp_res, DECch)

#define dec2prgon(name, pp_90, pp_res)				\
	prg_dec_on(name, pp_90, pp_res, DEC2ch)

#define dec3prgon(name, pp_90, pp_res)				\
	prg_dec_on(name, pp_90, pp_res, DEC3ch)

#define obsprgoff()						\
	prg_dec_off(2, OBSch)

#define decprgoff()						\
	prg_dec_off(2, DECch)

#define dec2prgoff()						\
	prg_dec_off(2, DEC2ch)

#define dec3prgoff()						\
	prg_dec_off(2, DEC3ch)

#define initdelay(incrtime,index) 				\
	G_RTDelay(RTDELAY_MODE,SET_INITINCR,RTDELAY_TBASE,index,\
		  RTDELAY_INCR,incrtime,0)

#define incdelay(multparam,index) \
	G_RTDelay(RTDELAY_MODE,SET_RTINCR,RTDELAY_TBASE,index,	\
		  RTDELAY_COUNT,multparam,0)

#define vdelay(base,rtcnt) \
	G_RTDelay(RTDELAY_MODE,TCNT,RTDELAY_TBASE,(int)(base),	\
		  RTDELAY_COUNT,rtcnt,0)

#define statusdelay(index,delaytime) \
	S_statusdelay((int)(index), (double)(delaytime)) 

/*---------------------------------------------------------------*/
/*- INOVA defines 						-*/
/*---------------------------------------------------------------*/

#define i_power(rtparam,device,string)				\
			G_Power(POWER_RTVALUE,	rtparam,	\
			      POWER_DEVICE,	device,		\
			      POWER_TYPE,	COARSE,		\
			      SLIDER_LABEL,	string,		\
			      0)

#define i_pwrf(value,device,string)				\
			G_Power(POWER_RTVALUE,	rtparam,	\
			      POWER_DEVICE,	device,		\
			      SLIDER_LABEL,	string,		\
			      0)

#define setuserap(value,reg)						\
	setBOB((int)(value),(int)(reg))

#define vsetuserap(rtparam,reg)					\
	vsetBOB(rtparam,(int)(reg))

#define readuserap(rtparam)					\
	vreadBOB(rtparam,3)

#define setExpTime(duration)	g_setExpTime( (double) duration)
/*---------------------------------------------------------------*/
/*- SISCO defines (gradients)					-*/
/*---------------------------------------------------------------*/


#define gradient(gid,gamp)					\
		rgradient(gid, (double)(gamp))

#define vgradient(gid,gamp0,gampi,vmult)				\
		S_vgradient(gid, IRND(gamp0), IRND(gampi), vmult)

#define incgradient(gid,gamp0,gamp1,gamp2,gamp3,mult1,mult2,mult3)	\
		S_incgradient(gid, IRND(gamp0),		\
				IRND(gamp1), IRND(gamp2), IRND(gamp3),	\
				mult1, mult2, mult3)

#define shapedpulse(pulsefile,pulsewidth,phaseptr,rx1,rx2)	\
		newtrans ? genshaped_pulse(pulsefile,(double)(pulsewidth), \
				phaseptr,(double)(rx1),(double)(rx2),  \
				0.0,0.0,OBSch)	\
			 :  S_shapedpulse(pulsefile,(double)(pulsewidth), \
				phaseptr,(double)(rx1),(double)(rx2))

#define decshapedpulse(pulsefile,pulsewidth,phaseptr,rx1,rx2)		\
		newdec ? genshaped_pulse(pulsefile,(double)(pulsewidth), \
				phaseptr,(double)(rx1),(double)(rx2),  \
				0.0,0.0,DECch)	\
		       : S_decshapedpulse(pulsefile,(double)(pulsewidth), \
				phaseptr,(double)(rx1),(double)(rx2))

#define simshapedpulse(fno,fnd,transpw,decpw,transphase,decphase,rx1,rx2) \
		newtrans ? gensim2shaped_pulse(fno,fnd,(double)(transpw),	\
				(double)(decpw),transphase,decphase,	\
				(double)(rx1),(double)(rx2),		\
				0.0,0.0,OBSch,DECch) \
			 : S_simshapedpulse(fno,fnd,(double)(transpw),	\
				(double)(decpw),transphase,decphase,	\
				(double)(rx1),(double)(rx2))

#define shapedincgradient(axis,pattern,width,a0,a1,a2,a3,x1,x2,x3,loops,wait) \
		shaped_INC_gradient(axis, pattern, (double)(width), \
			(double)(a0), (double)(a1), (double)(a2), (double)(a3), \
			x1, x2, x3, IRND(loops), IRND(wait))

#define shapedgradient(pulsefile,pulsewidth,gamp0,which,loops,wait_4_me) \
		S_shapedgradient(pulsefile,(double)(pulsewidth),	\
				(double)(gamp0),which,(int)(loops+0.5),	\
				(int)(wait_4_me+0.5))

#define shaped2Dgradient(pulsefile,pulsewidth,gamp0,which,loops,wait_4_me,tag) \
		shaped_2D_gradient(pulsefile,(double)(pulsewidth),	\
				(double)(gamp0),which,(int)(loops+0.5),	\
				(int)(wait_4_me+0.5),(int)(tag+0.5))

#define shapedvgradient(pfile,pwidth,gamp0,gampi,vmult,which,vloops,wait,tag) \
		shaped_V_gradient(pfile,(double)(pwidth),(double)(gamp0), \
				(double)(gampi),vmult,which,vloops,	\
				(int)(wait+0.5),(int)(tag+0.5))

#define oblique_gradient(level1,level2,level3,ang1,ang2,ang3)          \
		S_oblique_gradient((double)(level1),(double)(level2),  \
	                        (double)(level3),(double)(ang1),       \
			        (double)(ang2),(double)(ang3))

#define oblique_imaging_shapedgradient(pat,width,lvl1,lvl2,lvl3,ang1,ang2,ang3,loops,wait)  \
          S_oblique_shapedgradient(pat,(double)(width), \
		                (double)(lvl1),(double)(lvl2),(double)(lvl3), \
				(double)(ang1),(double)(ang2),(double)(ang3), \
				(int)(loops+0.5),(int)(wait+0.5))

#define oblique_shapedgradient(pat1,pat2,pat3,width,lvl1,lvl2,lvl3,ang1,ang2,ang3,loops,wait)  \
          S_oblique_shaped3gradient(pat1,pat2,pat3,(double)(width), \
		                (double)(lvl1),(double)(lvl2),(double)(lvl3), \
				(double)(ang1),(double)(ang2),(double)(ang3), \
				(int)(loops+0.5),(int)(wait+0.5))

#define phase_encode_gradient(stat1,stat2,stat3,step2,vmult2,lim2,ang1,ang2,ang3)  \
                S_phase_encode_gradient((double)(stat1),(double)(stat2),      \
				(double)(stat3),(double)(step2),              \
				(codeint)vmult2,(double)(lim2),(double)(ang1), \
				(double)(ang2),(double)(ang3))

#define phase_encode_shapedgradient(pat,width,stat1,stat2,stat3,step2,vmult2,lim2,ang1,ang2,ang3,vloops,wait,tag)   \
                S_phase_encode_shapedgradient(pat,(double)(width),        \
				(double)(stat1),(double)(stat2),          \
				(double)(stat3),(double)(step2),          \
				vmult2,(double)(lim2),(double)(ang1),     \
				(double)(ang2),(double)(ang3),vloops,     \
				(int)(wait+0.5),(int)(tag+0.5))

#define pe_oblique_shapedgradient(pat1,pat2,pat3,width,lvl1,lvl2,lvl3,step2,vmult2,lim2,ang1,ang2,ang3,wait,tag)   \
           S_pe_oblique_shaped3gradient(pat1,pat2,pat3,(double)(width),   \
		                (double)(lvl1),(double)(lvl2),(double)(lvl3), \
				(double)(step2),vmult2,(double)(lim2),    \
				(double)(ang1),(double)(ang2),(double)(ang3), \
				(int)(wait+0.5),(int)(tag+0.5))

#define pe3_oblique_shaped3gradient(pat1,pat2,pat3,width,stat1,stat2,stat3,step1,step2,step3,vmult1,vmult2,vmult3,lim1,lim2,lim3,ang1,ang2,ang3,loops,wait) \
                S_pe3_oblique_shaped3gradient(pat1,pat2,pat3,(double)(width), \
				(double)(stat1),(double)(stat2), \
				(double)(stat3),(double)(step1),          \
				(double)(step2),(double)(step3),vmult1,   \
				vmult2,vmult3,(double)(lim1),(double)(lim2), \
				(double)(lim3),(double)(ang1),(double)(ang2), \
				(double)(ang3), IRND(loops), IRND(wait))

#define phase_encode3_gradient(stat1,stat2,stat3,step1,step2,step3,vmult1,vmult2,vmult3,lim1,lim2,lim3,ang1,ang2,ang3) \
                S_phase_encode3_gradient((double)(stat1),(double)(stat2), \
				(double)(stat3),(double)(step1),          \
				(double)(step2),(double)(step3),vmult1,   \
				vmult2,vmult3,(double)(lim1),(double)(lim2), \
				(double)(lim3),(double)(ang1),(double)(ang2), \
				(double)(ang3))

#define phase_encode3_shapedgradient(pat,width,stat1,stat2,stat3,step1,step2,step3,vmult1,vmult2,vmult3,lim1,lim2,lim3,ang1,ang2,ang3,loops,wait) \
                S_phase_encode3_shapedgradient(pat,(double)(width), \
				(double)(stat1),(double)(stat2), \
				(double)(stat3),(double)(step1),          \
				(double)(step2),(double)(step3),vmult1,   \
				vmult2,vmult3,(double)(lim1),(double)(lim2), \
				(double)(lim3),(double)(ang1),(double)(ang2), \
				(double)(ang3), IRND(loops), IRND(wait))

#define position_offset(pos,grad,resfrq,device)                 \
                S_position_offset((double)(pos),(double)(grad), \
				(double)(resfrq),(int)(device))

#define position_offset_list(posarray,grad,nslices,resfrq,device,listno,apv1) \
                S_position_offset_list(posarray,(double)(grad),      \
				(double)(nslices),(double)(resfrq),  \
				(int)(device),(int)(listno),apv1)

#define oblique_gradpulse(level1,level2,level3,ang1,ang2,ang3,gradtime)   \
		S_oblique_gradpulse((double)(level1),(double)(level2),  \
	                (double)(level3),(double)(ang1),(double)(ang2),  \
			(double)(ang3),(double)(gradtime))

#define vagradient(gradlvl,theta,phi)				\
    	oblique_gradient(0.0,0.0,(double)(gradlvl),		\
			(double)(phi),0.0,(double)(theta))

#define vagradpulse(gradlvl,gradtime,theta,phi)		\
    	S_oblique_gradpulse(0.0,0.0,(double)(gradlvl),		\
			(double)(phi),0.0,(double)(theta),	\
			(double)(gradtime))

#define vashapedgradient(pat,gradlvl,gradtime,theta,phi,loops,wait)	\
	S_oblique_shapedgradient(pat,(double)(gradtime),0.0, \
			0.0,(double)(gradlvl),(double)(phi), \
			0.0,(double)(theta),(int)(loops+0.5),(int)(wait+0.5))

#define vashapedgradpulse(pat,gradlvl,gradtime,theta,phi)		\
	S_oblique_shapedgradient(pat,(double)(gradtime),0.0, \
			0.0,(double)(gradlvl),(double)(phi), \
			0.0,(double)(theta),(int)(0),WAIT)

#define magradient(gradlvl)				\
    	oblique_gradient((double)(gradlvl),(double)(gradlvl),	\
		(double)(gradlvl),90.0,0.0,90.0)

#define magradpulse(gradlvl,gradtime)		\
    	S_oblique_gradpulse((double)(gradlvl),(double)(gradlvl),  \
		(double)(gradlvl),90.0,0.0,90.0,	\
			(double)(gradtime))

#define mashapedgradient(pat,gradlvl,gradtime,loops,wait)		\
	S_oblique_shapedgradient(pat,(double)(gradtime),(double)(gradlvl), \
			(double)(gradlvl),(double)(gradlvl),90.0, \
			0.0,90.0,(int)(loops+0.5),(int)(wait+0.5))

#define mashapedgradpulse(pat,gradlvl,gradtime)		\
	S_oblique_shapedgradient(pat,(double)(gradtime),(double)(gradlvl), \
			(double)(gradlvl),(double)(gradlvl),90.0, \
			0.0,90.0,(int)(0),WAIT)

/*---------------------------------------------------------------*/
/*- SISCO defines (Misc)					-*/
/*---------------------------------------------------------------*/
#define observepower(reqpower)                                  \
                        rlpower((double)(reqpower),OBSch)

#define decouplepower(reqpower)                                 \
                        rlpower((double)(reqpower),DECch)

#define getarray(paramname,arrayname)                           \
			S_getarray(paramname,arrayname,sizeof(arrayname))

#define create_offset_list(list,nvals,device,list_no)			\
			Create_offset_list(list,(int)(nvals+0.5), 	\
				(int)(device+0.5), IRND(list_no))
#define create_freq_list(list,nvals,device,list_no)			\
			Create_freq_list(list,(int)(nvals+0.5), 	\
				(int)(device+0.5), IRND(list_no))
#define create_delay_list(list,nvals,list_no)				\
			Create_delay_list(list,(int)(nvals+0.5), 	\
				IRND(list_no))
#define voffset(table,vindex)					\
			 setHSLdelay(); vget_elem((int)(table+0.5),vindex)
#define vfreq(table,vindex)					\
			 setHSLdelay(); vget_elem((int)(table+0.5),vindex)
#define vdelay_list(table,vindex)					\
			vget_elem((int)(table+0.5),vindex)
#define init_vscan(rtvar,npts)					\
			Init_vscan(rtvar, (double)(npts))

/*---------------------------------------------------------------*/
/*- SISCO defines (Imaging Sequence Developement)		-*/
/*---------------------------------------------------------------*/


#define  rotate() \
    rotate_angle(psi,phi,theta,offsetx,offsety,offsetz,gxdelay,gydelay,gzdelay)

#define  rot_angle(psi,phi,theta) \
    rotate_angle(psi,phi,theta,offsetx,offsety,offsetz,gxdelay,gydelay,gzdelay)

#define  poffset(POS,GRAD)  position_offset(POS,GRAD,resto,OBSch)

#define  poffset_list(POSARRAY,GRAD,NS,APV1) \
    position_offset_list(POSARRAY,GRAD,NS,resto,OBSch,0,APV1)

#define obl_gradient(LEVEL1,LEVEL2,LEVEL3) \
    oblique_gradient(LEVEL1,LEVEL2,LEVEL3,psi,phi,theta)

#define obl_imaging_shapedgradient(PAT,WIDTH,LVL1,LVL2,LVL3,LOOPS,WAIT) \
    oblique_imaging_shapedgradient(PAT,WIDTH,LVL1,LVL2,LVL3,psi,phi,theta, \
								LOOPS,WAIT)

#define obl_shapedgradient(PAT1,PAT2,PAT3,WIDTH,LVL1,LVL2,LVL3,LOOPS,WAIT) \
    oblique_shapedgradient(PAT1,PAT2,PAT3,WIDTH,LVL1,LVL2,LVL3,psi,phi,theta, \
								LOOPS,WAIT)

#define pe_oblshapedgradient(PAT1,PAT2,PAT3,WIDTH,LVL1,LVL2,LVL3,STEP2,VMULT2,WAIT,TAG) \
    pe_oblique_shapedgradient(PAT1,PAT2,PAT3,WIDTH,LVL1,LVL2,LVL3,STEP2, \
			VMULT2,nv/2,psi,phi,theta,WAIT,TAG)

#define pe2_oblshapedgradient(PAT1,PAT2,PAT3,WIDTH,STAT1,STAT2,STAT3,STEP2,STEP3,VMULT2,VMULT3) \
    pe3_oblique_shaped3gradient(PAT1,PAT2,PAT3, WIDTH, STAT1, STAT2, STAT3, 0.0, \
			STEP2, STEP3, zero, VMULT2, VMULT3,  \
        		0.0, nv/2, nv2/2, psi, phi, theta, 1.0, WAIT)

#define pe3_oblshapedgradient(PAT1,PAT2,PAT3,WIDTH,STAT1,STAT2,STAT3,STEP1,STEP2,STEP3,VMULT1,VMULT2,VMULT3) \
    pe3_oblique_shaped3gradient(PAT1,PAT2,PAT3, WIDTH ,STAT1, STAT2, STAT3, \
			STEP1, STEP2, STEP3, VMULT1, VMULT2, VMULT3, \
        		nv/2, nv2/2, nv3/2, psi, phi, theta, 1.0, WAIT)

#define pe_gradient(STAT1,STAT2,STAT3,STEP2,VMULT2) \
    phase_encode_gradient(STAT1,STAT2,STAT3,STEP2,VMULT2,nv/2,psi,phi,theta)

#define pe_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP2,VMULT2,WAIT,TAG) \
    phase_encode_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP2,VMULT2, \
        nv/2,psi,phi,theta,one,WAIT,TAG)

#define pe2_gradient(STAT1,STAT2,STAT3,STEP2,STEP3,VMULT2,VMULT3)   \
    phase_encode3_gradient(STAT1, STAT2, STAT3, 0.0, STEP2, STEP3,  \
        zero, VMULT2, VMULT3, 0.0, nv/2, nv2/2, psi, phi, theta)

#define pe3_gradient(STAT1,STAT2,STAT3,STEP1,STEP2,STEP3,VMULT1,VMULT2,VMULT3) \
    phase_encode3_gradient(STAT1, STAT2, STAT3, STEP1, STEP2, STEP3, \
        VMULT1, VMULT2, VMULT3, nv/2, nv2/2, nv3/2, psi, phi, theta)

#define pe2_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP2,STEP3,VMULT2,VMULT3)   \
    phase_encode3_shapedgradient(PAT, WIDTH, STAT1, STAT2, STAT3, 0.0, \
			STEP2, STEP3, zero, VMULT2, VMULT3,  \
        		0.0, nv/2, nv2/2, psi, phi, theta, 1.0, WAIT)

#define pe3_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP1,STEP2,STEP3,VMULT1,VMULT2,VMULT3) \
    phase_encode3_shapedgradient(PAT, WIDTH ,STAT1, STAT2, STAT3, \
			STEP1, STEP2, STEP3, VMULT1, VMULT2, VMULT3, \
        		nv/2, nv2/2, nv3/2, psi, phi, theta, 1.0, WAIT)

#define init_rfpattern(pattern_name,pulse_struct,steps)  \
        init_RFpattern(pattern_name, pulse_struct, IRND(steps))

#define init_decpattern(pattern_name,pulse_struct,steps)  \
        init_DECpattern(pattern_name, pulse_struct, IRND(steps))

#define init_gradpattern(pattern_name,pulse_struct,steps)  \
        init_Gpattern(pattern_name, pulse_struct, IRND(steps))

/*---------------------------------------------------------------*/
/*- End SISCO defines 						-*/
/*---------------------------------------------------------------*/

extern void incr(int a);
extern void decr(int a);
extern void assign(int a, int b);
extern void dbl(int a, int b);
extern void hlv(int a, int b);
extern void modn(int a, int b, int c);
extern void mod4(int a, int b);
extern void mod2(int a, int b);
extern void add(int a, int b, int c);
extern void sub(int a, int b, int c);
extern void mult(int a, int b, int c);
extern void divn(int a, int b, int c);
extern void orr(int a, int b, int c);
extern void G_initval(double value, int index);
extern void settable(int tablename, int numelements, int tablearray[]);
extern void getelem(int tablename, int indxptr, int dstptr);
extern void getTabSkip(int tablename, int indxptr, int dstptr);
extern codeint tablertv(codeint tablename);

extern void starthardloop(codeint count);
extern void endhardloop();
extern void loop(codeint count, codeint counter);
extern void endloop(codeint counter);
extern void ifzero(codeint rtvar);
extern void ifmod2zero(codeint rtvar);
extern void elsenz(codeint rtvar);
extern void endif(codeint rtvar);
extern int  loadtable(char *infilename);
extern void tablesop(int operationtype, int tablename, int scalarval, int modval);
extern void tabletop(int operationtype, int table1name, int table2name, int modval);

extern void S_statusdelay(int index, double delaytime);
extern void setstatus(int channel, int on, char mod_mode,
                      int sync, double set_dmf);
extern void setreceiver();
extern void rcvroff();
extern void rcvron();
extern void	setprgmode(int mode, int rfdevice);
extern void	prg_dec_off(int stopmode, int rfdevice);
extern int prg_dec_on(char *name, double pw_90, double deg_res, int rfdevice);
extern void zgradpulse(double gval, double gdelay);
extern double gen_shapelistpw(char *, double, int);
extern double gen_poffset(double, double, int);
extern double getTimeMarker(void);
extern double nuc_gamma(void);
extern void parallelstart(const char *chanType);
extern double parallelend();
extern int      getorientation(char *c1,char *c2,char *c3,char *orientname);
extern int      grad_advance(double dur);
extern void     set_rotation_matrix(double ang1, double ang2, double ang3);
extern void lk_sample();
extern void lk_hold();
extern void lk_hslines();
extern void lk_autotrig();
extern void lk_sampling_off();
extern void rlpower(double value, int device);
extern void power(int value, int device);

extern void genshaped_pulse(char *name, double width, codeint phase,
                     double rx1, double rx2, double g1,
                     double g2, int rfdevice);
extern void S_shapedpulse(char *pulsefile, double pulsewidth, codeint phaseptr,
                   double rx1, double rx2);
extern void S_decshapedpulse(char *pulsefile, double pulsewidth,
                   codeint phaseptr, double rx1, double rx2);
extern void S_simshapedpulse(char *fno, char *fnd, double transpw, double decpw,
                 codeint transphase, codeint decupphase,
                 double rx1, double rx2);
extern void S_shapedgradient(char *name, double width, double amp,
                      char which, int loops, int wait_4_me);
extern void gensim3shaped_pulse(char *name1, char *name2, char *name3,
                    double width1, double width2, double width3,
                    codeint phase1, codeint phase2, codeint phase3,
                    double rx1, double rx2, double g1, double g2,
                    int rfdevice1, int rfdevice2, int rfdevice3);
extern void S_oblique_gradpulse(double level1, double level2, double level3,
                double ang1, double ang2, double ang3, double gdelay);
extern int pulse_phase_type(int val);
extern int phase_var_type(int val);
extern void stepsize(double stepsiz, int device);
extern void rlpwrf(double value, int device);
extern int SetRFChanAttr(Object obj, ...);
extern int prg_dec_on(char *name, double pw_90, double deg_res, int rfdevice);
extern void rgradient(char axis, double value);
extern void status(int index);
extern void G_Delay(int firstkey, ...);
extern void G_Pulse(int firstkey, ...);
extern int G_Offset( int firstkey, ... );
extern int hsdelay(double time);
extern void genspinlock(char *name, double pw_90, double deg_res, codeint phsval, int ncycles, int rfdevice);
extern void G_Simpulse(int firstkey, ...);
extern void acquire(double datapts, double dwell);
extern void sp1on();
extern void sp1off();

#endif
