/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
* macros.h for Nvpsg
*/

#ifndef DPS

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

/*--------------------------------------------------------------
|  To allow lint to check the for proper parameter type & number
|  passed to macros, each macro is define twice. 
|  For lint the macro is defined as its self but in all capital
|  letters. These are then defined in the lintfile.c
|  The incantation is then:
|   cc -DLINT -P -I. s2pul.c
|   lint -DLINT -a -c -h -u -z -v -n -I. s2pul.i llib-lpsg.ln
|
|					Greg Brissey.
+-------------------------------------------------------------*/

#ifndef LINT
/*---------------------------------------------------------------*/
#define initval(value,index)					\
                              G_initval((double)(value),index)

#define	obsblank()	      \
                              genBlankOnOff(1,OBSch)

#define	obsunblank()	      \
                              genBlankOnOff(0,OBSch)

#define	decblank()						\
                              genBlankOnOff(1,DECch)

#define	decunblank()						\
                              genBlankOnOff(0,DECch)

#define	dec2blank()						\
                              genBlankOnOff(1,DEC2ch)

#define	dec2unblank()						\
                              genBlankOnOff(0,DEC2ch)

#define	dec3blank()						\
                              genBlankOnOff(1,DEC3ch)

#define	dec3unblank()						\
                              genBlankOnOff(0,DEC3ch)

#define	dec4blank()						\
                              genBlankOnOff(1,DEC4ch)

#define	dec4unblank()						\
                              genBlankOnOff(0,DEC4ch)

#define decpulse(decpulse,phaseptr)				\
			genRFPulse(                             \
			      decpulse,	\
			      phaseptr,	\
			      0.0,	\
			      0.0,	\
                              DECch 	\
			      )

#define decrgpulse(pulsewidth, phaseptr, rx1, rx2)		\
                        genRFPulse(                             \
                              pulsewidth, \
                              phaseptr, \
                              rx1,      \
                              rx2,      \
                              DECch     \
                              )

#define dec2rgpulse(pulsewidth, phaseptr, rx1, rx2)		\
                        genRFPulse(                             \
                              pulsewidth, \
                              phaseptr, \
                              rx1,      \
                              rx2,      \
                              DEC2ch     \
                              )

#define dec3rgpulse(pulsewidth, phaseptr, rx1, rx2)		\
                        genRFPulse(                             \
                              pulsewidth, \
                              phaseptr, \
                              rx1,      \
                              rx2,      \
                              DEC3ch     \
                              )

#define dec4rgpulse(pulsewidth, phaseptr, rx1, rx2)		\
                        genRFPulse(      \
                              pulsewidth, \
                              phaseptr, \
                              rx1,      \
                              rx2,      \
                              DEC4ch     \
                              )

#define declvlon()        notImplemented()

#define declvloff()       notImplemented()

#define idecpulse(decpulse,phaseptr,string)			\
                              notImplemented()

#define idecrgpulse(pulsewidth, phaseptr, rx1, rx2,string)	\
                              notImplemented()

#define idec2rgpulse(pulsewidth, phaseptr, rx1, rx2,string)	\
                              notImplemented()

#define idec3rgpulse(pulsewidth, phaseptr, rx1, rx2,string)	\
                              notImplemented()

#define idelay(time,string)					\
                              notImplemented()

#define iobspulse(string)					\
                              notImplemented()

#define ipulse(pulsewidth,phaseptr,string)			\
                              notImplemented()

#define rcvrstepsize(stepval)                                  \
                              setRcvrPhaseStep((double)(stepval))

#define rcvrphase(phaseptr)					\
                              setRcvrPhaseVar((int)(phaseptr))

#define stepsize(stepval, DEVICE)                            \
                              definePhaseStep((double)(stepval), (int)(DEVICE))

#define obsstepsize(stepval)                                  \
                              definePhaseStep((double)(stepval),OBSch)

#define decstepsize(stepval)                                 \
                              definePhaseStep((double)(stepval),DECch)

#define dec2stepsize(stepval)                                 \
                              definePhaseStep((double)(stepval),DEC2ch)

#define dec3stepsize(stepval)                                 \
                              definePhaseStep((double)(stepval),DEC3ch)

#define dec4stepsize(stepval)                                 \
                              definePhaseStep((double)(stepval),DEC4ch)

#define rlpower(reqpower,chan)    \
                              genPower((double)reqpower, (int) chan) 

#define obspower(reqpower)    \
                              genPower((double)reqpower,OBSch) 

#define decpower(reqpower)                                 \
                              genPower((double)reqpower,DECch)

#define dec2power(reqpower)                                 \
                              genPower((double)reqpower,DEC2ch)

#define dec3power(reqpower)                                 \
                              genPower((double)reqpower,DEC3ch)

#define dec4power(reqpower)                                 \
                              genPower((double)reqpower,DEC4ch)

#define rlpwrf(reqpower, chan)                                   \
                              genPowerF((double)reqpower, (int) chan)

#define obspwrf(reqpower)                                   \
                              genPowerF((double)reqpower,OBSch)

#define decpwrf(reqpower)					\
                              genPowerF((double)reqpower,DECch)

#define dec2pwrf(reqpower)					\
                              genPowerF((double)reqpower,DEC2ch)

#define dec3pwrf(reqpower)					\
                              genPowerF((double)reqpower,DEC3ch)

#define dec4pwrf(reqpower)					\
                              genPowerF((double)reqpower,DEC4ch)

#define vobspwrfstepsize(stepsize)              \
							  defineVAmpStep((double)stepsize,OBSch)

#define vdecpwrfstepsize(stepsize)              \
							  defineVAmpStep((double)stepsize,DECch)

#define vdec2pwrfstepsize(stepsize)              \
							  defineVAmpStep((double)stepsize,DEC2ch)

#define vdec3pwrfstepsize(stepsize)              \
							  defineVAmpStep((double)stepsize,DEC3ch)

#define vdec4pwrfstepsize(stepsize)              \
							  defineVAmpStep((double)stepsize,DEC4ch)

#define vobspwrf(vindex)                                   \
                              genVPowerF(vindex,OBSch)

#define vdecpwrf(vindex)                                       \
                              genVPowerF(vindex,DECch)

#define vdec2pwrf(vindex)                                      \
                              genVPowerF(vindex,DEC2ch)

#define vdec3pwrf(vindex)                                      \
                              genVPowerF(vindex,DEC3ch)

#define vdec4pwrf(vindex)                                      \
                              genVPowerF(vindex,DEC4ch)

#define ipwrf(value,device,string)				\
                              notImplemented()

#define ipwrm(value,device,string)				\
                              notImplemented()

#define irgpulse(pulsewidth, phaseptr, rx1, rx2,string)		\
                              notImplemented()


/*  offset is still a subroutine because defining it as a Macro
    wrecks havoc on any program that uses `offset' as a symbol.  */

#define obsoffset( value )                       \
                              genOffset(value, OBSch) 

#define decoffset( value )			\
                              genOffset(value, DECch)

#define dec2offset( value )			\
                              genOffset(value, DEC2ch)

#define dec3offset( value )			\
                              genOffset(value, DEC3ch)

#define dec4offset( value )			\
                              genOffset(value, DEC4ch)

#define ioffset( value, device, string )			\
                              notImplemented()

#define obspulse()						\
                        genRFPulse(                             \
                              pw, \
                              oph, \
                              rof1,      \
                              rof2,      \
                              OBSch     \
                              )

#define pulse(pulsewidth,phaseptr)				\
                        genRFPulse(                             \
                              pulsewidth, \
                              phaseptr, \
                              rof1,      \
                              rof2,      \
                              OBSch     \
                              )

#define rgpulse(pulsewidth, phaseptr, rx1, rx2)			\
                        genRFPulse(                             \
                              pulsewidth, \
                              phaseptr, \
                              rx1,      \
                              rx2,      \
                              OBSch     \
                              )

#define pwrm(rtparam,device)					\
                              notImplemented()

#define rlpwrm(value,device)					\
                              notImplemented()

#define setautoincrement(tname)					\
                              notImplemented()

#define setdivnfactor(tname, dfactor) 				\
                              notImplemented()

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

#define genqdphase(phaseptr, rf_device)				\
                              notImplemented()

#define phaseshift(base, vvar, rf_device)                       \
                              notImplemented()

#define txphase(phaseptr)					\
                              genPhase(phaseptr,OBSch)

#define decphase(phaseptr)					\
                              genPhase(phaseptr,DECch)

#define dec2phase(phaseptr)					\
                              genPhase(phaseptr,DEC2ch)

#define dec3phase(phaseptr)					\
                              genPhase(phaseptr,DEC3ch)

#define dec4phase(phaseptr)					\
                              genPhase(phaseptr,DEC4ch)

#define genpulse(pulsewidth, phaseptr, rf_device)		\
                              notImplemented()

#define genrgpulse(pulsewidth, phaseptr, rx1, rx2, rf_device)	\
                              notImplemented()

#define xmtrphase(phaseptr)					\
                              genFullPhase(phaseptr,OBSch)

#define dcplrphase(phaseptr)					\
                              genFullPhase(phaseptr,DECch)

#define dcplr2phase(phaseptr)					\
                              genFullPhase(phaseptr,DEC2ch)

#define dcplr3phase(phaseptr)					\
                              genFullPhase(phaseptr,DEC3ch)

#define dcplr4phase(phaseptr)					\
                              genFullPhase(phaseptr,DEC4ch)

#define gensaphase(phaseptr, rf_device)				\
                              genFullPhase(phaseptr,rf_device)

#define simpulse(pw1, pw2, phs1, phs2, rx1, rx2)		\
                              gensim_pulse(pw1,pw2, phs1, phs2, rx1, rx2, OBSch, DECch)

#define sim3pulse(pw1, pw2, pw3, phs1, phs2, phs3, rx1, rx2)	\
         gensim3_pulse(pw1, pw2, pw3,  phs1, phs2, phs3, rx1, rx2, OBSch, DECch, DEC2ch)

#define sim4pulse(pw1,pw2,pw3,pw4,phs1,phs2,phs3,phs4,rx1,rx2)	\
         gensim4_pulse(pw1, pw2, pw3, pw4, phs1, phs2, phs3, phs4, rx1, rx2, OBSch, DECch, DEC2ch, DEC3ch)      
			

#define rfon(rf_device)						\
                              notImplemented()

#define rfoff(rf_device)						\
                              notImplemented()

#define xmtron()						\
                              genTxGateOnOff( 1, OBSch )

#define decon()							\
                              genTxGateOnOff( 1, DECch )

#define dec2on()                                                \
                              genTxGateOnOff( 1, DEC2ch )

#define dec3on()                                                \
                              genTxGateOnOff( 1, DEC3ch )

#define dec4on()                                                \
                              genTxGateOnOff( 1, DEC4ch )

#define xmtroff()						\
                              genTxGateOnOff( 0, OBSch )

#define decoff()						\
                              genTxGateOnOff( 0, DECch )

#define dec2off()                                               \
                              genTxGateOnOff( 0, DEC2ch )
 
#define dec3off()                                               \
                              genTxGateOnOff( 0, DEC3ch )

#define dec4off()                                               \
                              genTxGateOnOff( 0, DEC4ch )

#define nwloop(loops, rtvar1, rtvar2)                      \
                    nowait_loop((double)loops, rtvar1, rtvar2)

#define endnwloop(rtvar)                                   \
                    nowait_endloop(rtvar)

#define sp1on()         splineon( 1 )
#define sp2on()         splineon( 2 )
#define sp3on()         splineon( 3 )
#define sp1off()        splineoff( 1 )
#define sp2off()        splineoff( 2 )
#define sp3off()        splineoff( 3 )

#define sp4on()		splineon(4)
#define sp4off()	splineoff(4)
#define sp5on()		splineon(5)
#define sp5off()	splineoff(5)
#define sp6on()		splineon(6)
#define sp6off()	splineoff(6)
 
 
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
        genRFShapedPulse(width, name, phase, rx1, rx2, 0.0, 0.0, OBSch)

#define decshaped_pulse(name, width, phase, rx1, rx2)		\
        genRFShapedPulse(width, name, phase, rx1, rx2, 0.0, 0.0, DECch)

#define dec2shaped_pulse(name, width, phase, rx1, rx2)		\
        genRFShapedPulse(width, name, phase, rx1, rx2, 0.0, 0.0, DEC2ch)

#define dec3shaped_pulse(name, width, phase, rx1, rx2)		\
        genRFShapedPulse(width, name, phase, rx1, rx2, 0.0, 0.0, DEC3ch)

#define simshaped_pulse(n1, n2, w1, w2, ph1, ph2, r1, r2)	\
        gensim2shaped_pulse(n1, n2, w1, w2, ph1, ph2, r1, r2, 0.0, 0.0, OBSch, DECch)

#define sim3shaped_pulse(n1, n2, n3, w1, w2, w3, ph1, ph2, ph3, r1, r2)	\
        gensim3shaped_pulse(n1, n2, n3, w1, w2, w3, ph1, ph2, ph3, r1, r2, 0.0, 0.0, OBSch, DECch, DEC2ch)

#define shapedvpulse(name, width, rtamp, phase, rx1, rx2)		\
        genRFshaped_rtamppulse(name, width, rtamp, phase, rx1, rx2, 0.0, 0.0, OBSch)

#define apshaped_pulse(name, width, phase, tbl1, tbl2, rx1, rx2) \
        genRFShapedPulse(width, name, phase, rx1, rx2, 0.0, 0.0, OBSch)

#define apshaped_decpulse(name, width, phase, tbl1, tbl2, rx1, rx2) \
        genRFShapedPulse(width, name, phase, rx1, rx2, 0.0, 0.0, DECch)

#define apshaped_dec2pulse(name, width, phase, tbl1, tbl2, rx1, rx2) \
        genRFShapedPulse(width, name, phase, rx1, rx2, 0.0, 0.0, DEC2ch)

#define spinlock(   name, pp_90, pp_res, phase, nloops)		\
        genspinlock(name, pp_90, pp_res, phase, nloops, OBSch)

#define decspinlock(name, pp_90, pp_res, phase, nloops)		\
        genspinlock(name, pp_90, pp_res, phase, nloops, DECch)

#define dec2spinlock(name, pp_90, pp_res, phase, nloops)	\
        genspinlock(name, pp_90, pp_res, phase, nloops, DEC2ch)

#define dec3spinlock(name, pp_90, pp_res, phase, nloops)	\
        genspinlock(name, pp_90, pp_res, phase, nloops, DEC3ch)

#define dec4spinlock(name, pp_90, pp_res, phase, nloops)	\
        genspinlock(name, pp_90, pp_res, phase, nloops, DEC4ch)

#define obsprgon(name, pp_90, pp_res)				\
        prg_dec_on(name, pp_90, pp_res, OBSch)

#define decprgon(name, pp_90, pp_res)				\
        prg_dec_on(name, pp_90, pp_res, DECch)

#define dec2prgon(name, pp_90, pp_res)				\
        prg_dec_on(name, pp_90, pp_res, DEC2ch)

#define dec3prgon(name, pp_90, pp_res)				\
        prg_dec_on(name, pp_90, pp_res, DEC3ch)

#define dec4prgon(name, pp_90, pp_res)				\
        prg_dec_on(name, pp_90, pp_res, DEC4ch)

#define obsprgonOffset(name, pp_90, pp_res, frq)				\
        prg_dec_on_offset(name, pp_90, pp_res, frq, OBSch)

#define decprgonOffset(name, pp_90, pp_res, frq)				\
        prg_dec_on_offset(name, pp_90, pp_res, frq, DECch)

#define dec2prgonOffset(name, pp_90, pp_res, frq)				\
        prg_dec_on_offset(name, pp_90, pp_res, frq, DEC2ch)

#define dec3prgonOffset(name, pp_90, pp_res, frq)				\
        prg_dec_on_offset(name, pp_90, pp_res, frq, DEC3ch)

#define dec4prgonOffset(name, pp_90, pp_res, frq )				\
        prg_dec_on_offset(name, pp_90, pp_res, frq, DEC4ch)

#define obsprgoff()						\
        prg_dec_off(OBSch)

#define decprgoff()						\
        prg_dec_off(DECch)

#define dec2prgoff()						\
        prg_dec_off(DEC2ch)

#define dec3prgoff()						\
        prg_dec_off(DEC3ch)

#define dec4prgoff()						\
        prg_dec_off(DEC4ch)

#define initdelay(incrtime,index) 				\
                              notImplemented()

#define incdelay(multparam,index) \
                              notImplemented()

#define statusdelay(index,delaytime) \
               statusDelay((int)(index), (double)(delaytime))

#define create_delay_list(list, nvals ) 			\
		genCreate_delay_list((double *)list, (int) nvals)

/*---------------------------------------------------------------*/
/*- INOVA defines 						-*/
/*---------------------------------------------------------------*/

#define i_power(rtparam,device,string)				\
                              notImplemented()

#define i_pwrf(value,device,string)				\
                              notImplemented()

#define setuserap(value,reg)						\
                              notImplemented()

#define vsetuserap(rtparam,reg)					\
                              notImplemented()

#define readuserap(rtparam)					\
                              notImplemented()

#define setExpTime(duration)	 \
                              notImplemented()
/*---------------------------------------------------------------*/
/*- SISCO defines (gradients)					-*/
/*---------------------------------------------------------------*/


#define gradient(gid,gamp)					\
                              rgradient(gid, (double)(gamp))

#define vgradient(gid,gamp0,gampi,vmult)				\
                              notImplemented()

#define shapedpulse(pulsefile,pulsewidth,phaseptr,rx1,rx2)	\
        genRFShapedPulse(pulsewidth, pulsefile, phaseptr, rx1, rx2, 0.0, 0.0, OBSch)

#define decshapedpulse(pulsefile,pulsewidth,phaseptr,rx1,rx2)		\
        genRFShapedPulse(pulsewidth, pulsefile, phaseptr, rx1, rx2, 0.0, 0.0, DECch)

#define simshapedpulse(fno,fnd,transpw,decpw,transphase,decphase,rx1,rx2) \
        gensim2shaped_pulse(fno, fnd, transpw, decpw, transphase, decphase, rx1, rx2, 0.0, 0.0, OBSch, DECch)

#define shapedvgradient(pfile,pwidth,gamp0,gampi,vmult,which,vloops,wait,tag) \
                              notImplemented()

#define position_offset_list(posarray,grad,nslices,resfrq,device,listno,apv1) \
                              notImplemented()

#define vagradient(gradlvl,theta,phi)				\
                              notImplemented()

#define vagradpulse(gradlvl,gradtime,theta,phi)		\
                              notImplemented()

#define vashapedgradient(pat,gradlvl,gradtime,theta,phi,loops,wait)	\
                              notImplemented()

#define vashapedgradpulse(pat,gradlvl,gradtime,theta,phi)		\
                              notImplemented()

#define magradient(gradlvl)				                               \
         set_rotation_matrix(90.0, 0.0, 90.0) ,                                        \
         phase_encode3_gradient((double)(gradlvl)/1.73,(double)(gradlvl)/1.73,(double)(gradlvl)/1.73, \
            0.0,0.0,0.0, 0,0,0, 0.0,0.0,0.0)

#define magradpulse(gradlvl,gradtime)		                                       \
         set_rotation_matrix(90.0, 0.0, 90.0) ,                                        \
         phase_encode3_gradpulse((double)(gradlvl)/1.73, (double)(gradlvl)/1.73, (double)(gradlvl)/1.73, \
            (double)(gradtime), 0.0,0.0,0.0, 0,0,0,  0.0,0.0,0.0)  

#define mashapedgradient(pat,gradlvl,gradtime,loops,wait)	      	                \
           set_rotation_matrix(90.0, 0.0, 90.0) ,                                       \
           phase_encode3_oblshapedgradient(pat,pat,pat,(double)(gradtime),              \
                 (double)(gradlvl)/1.73,(double)(gradlvl)/1.73,(double)(gradlvl)/1.73,  \
                 0.0,0.0,0.0, 0,0,0, 0.0,0.0,0.0, 1, (int)(wait+0.5), 0)

#define mashapedgradpulse(pat,gradlvl,gradtime)                                         \
           set_rotation_matrix(90.0, 0.0, 90.0) ,                                       \
           phase_encode3_oblshapedgradient(pat,pat,pat,(double)(gradtime),              \
                 (double)(gradlvl)/1.73,(double)(gradlvl)/1.73,(double)(gradlvl)/1.73,  \
                 0.0,0.0,0.0, 0,0,0, 0.0,0.0,0.0, 1, WAIT, 0)

/*---------------------------------------------------------------*/
/*- SISCO defines (Misc)					-*/
/*---------------------------------------------------------------*/
#define observepower(reqpower)                                  \
                              genPower(reqpower,OBSch)

#define decouplepower(reqpower)                                 \
                              genPower(reqpower,DECch)

#define getarray(paramname,arrayname)                           \
             S_getarray(paramname,arrayname,sizeof(arrayname))

#define create_freq_list(list,nvals,device,list_no)			\
                              notImplemented()

#define voffset(table,vindex)					\
                              voffsetch((int) table,(int) vindex,(int) OBSch)

#define vfreq(table,vindex)					\
                              notImplemented()

#define init_vscan(rtvar,npts)					\
                              notImplemented()

/*---------------------------------------------------------------*/
/*- SISCO defines (Imaging Sequence Developement)		-*/
/*---------------------------------------------------------------*/


#define  rotate()                 \
    set_rotation_matrix(psi,phi,theta)

#define rot_angle(psi,phi,theta)     \
    set_rotation_matrix(psi,phi,theta)

#define  poffset(pos,grad)  gen_poffset(pos, grad, OBSch)

#define  poffset_list(POSARRAY,GRAD,NS,APV1) \
                              notImplemented()

#define position_offset(pos,grad,resfrq,device)                 \
                S_position_offset((double)(pos),(double)(grad), \
                                (double)(resfrq),(int)(device))

#define offsetlist(posarray, gssval, resfrq, listarray, nsval, mode)              \
    gen_offsetlist((double *)posarray, (double)gssval, (double)resfrq, (double *)listarray, (double)nsval, mode, OBSch)

#define offsetglist(posarray, gssvalarr, resfrq, listarray, nsval, mode)          \
    gen_offsetglist((double *)posarray, (double *)gssvalarr, (double)resfrq, (double *)listarray, (double)nsval, mode, OBSch)

#define create_rotation_list(nm, angle_set, num_sets)       \
    create_rot_angle_list(nm, (&(angle_set[0][0])), (int)(num_sets))

#define shapelist(pat, width, listarray, nsval, frac, mode)  \
    gen_shapelist_init(pat, (double)width, (double *)listarray, (double)nsval, (double)(1.0-frac), mode, OBSch)

#define decshapelist(pat, width, listarray, nsval, frac, mode)  \
    gen_shapelist_init(pat, (double)width, (double *)listarray, (double)nsval, (double)(1.0-frac), mode, DECch)

#define dec2shapelist(pat, width, listarray, nsval, frac, mode)  \
    gen_shapelist_init(pat, (double)width, (double *)listarray, (double)nsval, (double)(1.0-frac), mode, DEC2ch)

#define dec3shapelist(pat, width, listarray, nsval, frac, mode)  \
    gen_shapelist_init(pat, (double)width, (double *)listarray, (double)nsval, (double)(1.0-frac), mode, DEC3ch)

#define shapelistpw(pat, width)                             \
    gen_shapelistpw(pat, (double)width, OBSch)

#define decshapelistpw(pat, width)                             \
    gen_shapelistpw(pat, (double)width, DECch)

#define dec2shapelistpw(pat, width)                             \
    gen_shapelistpw(pat, (double)width, DEC2ch)

#define dec3shapelistpw(pat, width)                             \
    gen_shapelistpw(pat, (double)width, DEC3ch)

#define shapedpulselist(listid, width, rtvar1, rof1, rof2, mode, rtvar2)          \
    gen_shapedpulselist((int)listid, (double)width, (int)rtvar1, (double)rof1, (double)rof2, mode, (int)rtvar2, OBSch)

#define decshapedpulselist(listid, width, rtvar1, rof1, rof2, mode, rtvar2)          \
    gen_shapedpulselist((int)listid, (double)width, (int)rtvar1, (double)rof1, (double)rof2, mode, (int)rtvar2, DECch)

#define dec2shapedpulselist(listid, width, rtvar1, rof1, rof2, mode, rtvar2)          \
    gen_shapedpulselist((int)listid, (double)width, (int)rtvar1, (double)rof1, (double)rof2, mode, (int)rtvar2, DEC2ch)

#define dec3shapedpulselist(listid, width, rtvar1, rof1, rof2, mode, rtvar2)          \
    gen_shapedpulselist((int)listid, (double)width, (int)rtvar1, (double)rof1, (double)rof2, mode, (int)rtvar2, DEC3ch)

#define shapedpulseoffset(pat, pw, rtvar, rof1, rof2, offset)                    \
    gen_shapedpulseoffset(pat, (double)pw, (int)rtvar, (double)rof1, (double)rof2, (double)offset, OBSch)

#define decshapedpulseoffset(pat, pw, rtvar, rof1, rof2, offset)                    \
    gen_shapedpulseoffset(pat, (double)pw, (int)rtvar, (double)rof1, (double)rof2, (double)offset, DECch)

#define dec2shapedpulseoffset(pat, pw, rtvar, rof1, rof2, offset)                    \
    gen_shapedpulseoffset(pat, (double)pw, (int)rtvar, (double)rof1, (double)rof2, (double)offset, DEC2ch)

#define dec3shapedpulseoffset(pat, pw, rtvar, rof1, rof2, offset)                    \
    gen_shapedpulseoffset(pat, (double)pw, (int)rtvar, (double)rof1, (double)rof2, (double)offset, DEC3ch)

#define obl_gradient(LEVEL1,LEVEL2,LEVEL3)                                        \
         phase_encode3_gradient((double)LEVEL1,(double)LEVEL2,(double)LEVEL3,     \
            0.0,0.0,0.0, 0,0,0, 0.0,0.0,0.0)

#define obl_shapedgradient(PAT,WIDTH,LVL1,LVL2,LVL3,WAIT)                                \
           phase_encode3_oblshapedgradient(PAT,PAT,PAT,(double)(WIDTH),                  \
                                (double)(LVL1),(double)(LVL2),(double)(LVL3),            \
                                0.0, 0.0, 0.0, 0, 0, 0, 0.0, 0.0, 0.0,                   \
                                1, (int)(WAIT+0.5), 0)

#define obl_shaped3gradient(PAT1,PAT2,PAT3,WIDTH,LVL1,LVL2,LVL3,WAIT)                    \
           phase_encode3_oblshapedgradient(PAT1,PAT2,PAT3,(double)(WIDTH),               \
                                (double)(LVL1),(double)(LVL2),(double)(LVL3),            \
                                0.0, 0.0, 0.0, 0, 0, 0, 0.0, 0.0, 0.0,                   \
                                1, (int)(WAIT+0.5), 0)


#define pe_shaped3gradient(PAT1,PAT2,PAT3,WIDTH,LVL1,LVL2,LVL3,STEP2,VMULT2,WAIT)        \
           phase_encode3_oblshapedgradient(PAT1,PAT2,PAT3,(double)(WIDTH),               \
                                (double)(LVL1),(double)(LVL2),(double)(LVL3),            \
                                0.0,(double)(STEP2),0.0, 0,(int)VMULT2, 0,               \
                                0.0,(double)(nv/2),0.0, 1, (int)(WAIT+0.5), 0)

#define var_shaped3gradient(PAT1,PAT2,PAT3,WIDTH,LVL1,LVL2,LVL3,STEP2,VMULT2,WAIT)        \
           phase_encode3_oblshapedgradient(PAT1,PAT2,PAT3,(double)(WIDTH),               \
                                (double)(LVL1),(double)(LVL2),(double)(LVL3),            \
                                0.0,(double)(STEP2),0.0, 0,(int)VMULT2, 0,               \
                                0.0,1.0,0.0, 1, (int)(WAIT+0.5), 0)

#define pe_shaped3gradient_dual(PAT1,PAT2,PAT3,PAT4,PAT5,PAT6,STAT1,STAT2,STAT3,STAT4,STAT5,STAT6,STEP2,STEP5,VMULT2,VMULT5,WIDTH,WAIT) \
         d_phase_encode3_oblshapedgradient(PAT1,PAT2,PAT3,PAT4,PAT5,PAT6,          \
                                     (double)(STAT1),(double)(STAT2),(double)(STAT3),(double)(STAT4),(double)(STAT5),(double)(STAT6),     \
                                     0.0,(double)(STEP2),0.0,0.0,(double)(STEP5),0.0,     \
                                     0,(int)VMULT2,0,0, (int)VMULT5,0,                 \
                                     0.0,0.0,0.0,0.0,0.0,0.0,            \
                                     (double)(WIDTH), 1, (int)(WAIT+0.5), 0)


#define pe2_shaped3gradient(PAT1,PAT2,PAT3,WIDTH,STAT1,STAT2,STAT3,STEP2,STEP3,VMULT2,VMULT3,WAIT) \
           phase_encode3_oblshapedgradient(PAT1,PAT2,PAT3,(double)(WIDTH),                       \
                                (double)(STAT1),(double)(STAT2),(double)(STAT3),                 \
                                0.0,(double)(STEP2),(double)(STEP3), 0,(int)VMULT2, (int)VMULT3, \
                                0.0,0.0,0.0, 1, (int)(WAIT+0.5), 0)

#define var2_shaped3gradient(PAT1,PAT2,PAT3,WIDTH,STAT1,STAT2,STAT3,STEP2,STEP3,VMULT2,VMULT3,WAIT) \
           phase_encode3_oblshapedgradient(PAT1,PAT2,PAT3,(double)(WIDTH),                       \
                                (double)(STAT1),(double)(STAT2),(double)(STAT3),                 \
                                0.0,(double)(STEP2),(double)(STEP3), 0,(int)VMULT2, (int)VMULT3, \
                                0.0,1.0,1.0, 1, (int)(WAIT+0.5), 0)

/*
#define d_pe2_shaped3gradient(PAT1,PAT2,PAT3,PAT4,PAT5,PAT6,STAT1,STAT2,STAT3,STAT4,STAT5,STAT6,STEP2,STEP3,STEP5,STEP6,VMULT2,VMULT3,VMULT5,VMULT6,WIDTH,WAIT) \
         d_phase_encode3_oblshapedgradient(PAT1,PAT2,PAT3,PAT4,PAT5,PAT6,          \
                                     (double)(STAT1),(double)(STAT2),(double)(STAT3),(double)(STAT4),(double)(STAT4),(double)(STAT6),     \
                                     0.0,(double)(STEP2),(double)(STEP3),0.0,(double)(STEP5),(double)(STEP6),     \
                                     0,(int)VMULT2, (int)VMULT3,0,(int)VMULT5, (int)VMULT6,                 \
                                     0.0,0.0,0.0,0.0,0.0,0.0,            \
                                     (double)(WIDTH), 1, (int)(WAIT+0.5), 0)

*/


#define pe3_shaped3gradient(PAT1,PAT2,PAT3,WIDTH,STAT1,STAT2,STAT3,STEP1,STEP2,STEP3,VMULT1,VMULT2,VMULT3,WAIT) \
  phase_encode3_oblshapedgradient(PAT1,PAT2,PAT3,(double)WIDTH, (double)STAT1, (double)STAT2, (double)STAT3, \
  (double)STEP1, (double)STEP2, (double)STEP3, (int)VMULT1, (int)VMULT2, (int)VMULT3,                        \
  (double)(nv3/2), (double)(nv/2), (double)(nv2/2), 1, (int)(WAIT+0.5), 0)

#define var3_shaped3gradient(PAT1,PAT2,PAT3,WIDTH,STAT1,STAT2,STAT3,STEP1,STEP2,STEP3,VMULT1,VMULT2,VMULT3,WAIT) \
  phase_encode3_oblshapedgradient(PAT1,PAT2,PAT3,(double)WIDTH, (double)STAT1, (double)STAT2, (double)STAT3, \
  (double)STEP1, (double)STEP2, (double)STEP3, (int)VMULT1, (int)VMULT2, (int)VMULT3,                        \
  1.0,1.0,1.0, 1, (int)(WAIT+0.5), 0)


#define csi3_shaped3gradient(PAT1,PAT2,PAT3,WIDTH,STAT1,STAT2,STAT3,STEP1,STEP2,STEP3,VMULT1,VMULT2,VMULT3,WAIT) \
  phase_encode3_oblshapedgradient(PAT1,PAT2,PAT3,(double)WIDTH, (double)STAT1, (double)STAT2, (double)STAT3, \
  (double)STEP1, (double)STEP2, (double)STEP3, (int)VMULT1, (int)VMULT2, (int)VMULT3,                        \
  (double)(nv/2), (double)(nv2/2), (double)(nv3/2), 1, (int)(WAIT+0.5), 0)


#define pe3_shaped3gradient_dual(PAT1,PAT2,PAT3,PAT1B,PAT2B,PAT3B, \
								STAT1,STAT2,STAT3,STAT1B,STAT2B,STAT3B, \
								STEP1,STEP2,STEP3,STEP1B,STEP2B,STEP3B, \
								VMULT1,VMULT2,VMULT3,VMULT1B,VMULT2B,VMULT3B, \
								WIDTH, WAIT) \
  d_phase_encode3_oblshapedgradient(PAT1,PAT2,PAT3,PAT1B,PAT2B,PAT3B, \
											(double)(STAT1),(double)(STAT2),(double)(STAT3),(double)(STAT1B),(double)(STAT2B),(double)(STAT3B),     \
                                     		(double)(STEP1),(double)(STEP2),(double)(STEP3),(double)(STEP1B),(double)(STEP2B),(double)(STEP3B),     \
                                     		(int)VMULT1,(int)VMULT2,(int)VMULT3,(int)VMULT1B, (int)VMULT2B,(int)VMULT3B,                 \
                                     		0.0,0.0,0.0,0.0,0.0,0.0,            \
                                     		(double)(WIDTH), 1, (int)(WAIT+0.5), 0)


#define pe_gradient(STAT1,STAT2,STAT3,STEP2,VMULT2)                             \
        phase_encode3_gradient((double)STAT1,(double)STAT2,(double)STAT3,       \
              0.0,(double)STEP2,0.0, 0,(int)VMULT2,0, 0.0,(double)(nv/2.0),0.0)

#define pe_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP2,VMULT2,WAIT)                  \
           phase_encode3_oblshapedgradient(PAT,PAT,PAT,(double)(WIDTH),                   \
                                (double)(STAT1),(double)(STAT2),(double)(STAT3),          \
                                0.0,(double)(STEP2),0.0, 0,(int)VMULT2, 0,                \
                                0.0,(double)(nv/2),0.0, 1, (int)(WAIT+0.5), 0)

#define var_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP2,VMULT2,WAIT)                  \
           phase_encode3_oblshapedgradient(PAT,PAT,PAT,(double)(WIDTH),                   \
                                (double)(STAT1),(double)(STAT2),(double)(STAT3),          \
                                0.0,(double)(STEP2),0.0, 0,(int)VMULT2, 0,                \
                                0.0,1.0,0.0, 1, (int)(WAIT+0.5), 0)

#define pe2_gradient(STAT1,STAT2,STAT3,STEP2,STEP3,VMULT2,VMULT3)         \
        phase_encode3_gradient((double)STAT1,(double)STAT2,(double)STAT3, \
              0.0,(double)STEP2,(double)STEP3, 0,(int)VMULT2,(int)VMULT3, \
              0.0,(double)(nv/2.0),(double)(nv2/2.0))


#define pe3_gradient(STAT1,STAT2,STAT3,STEP1,STEP2,STEP3,VMULT1,VMULT2,VMULT3) \
        phase_encode3_gradient((double)STAT1,(double)STAT2,(double)STAT3,      \
              (double)STEP1,(double)STEP2,(double)STEP3,                       \
              (int)VMULT1,(int)VMULT2,(int)VMULT3,                             \
              (double)(nv3/2.0),(double)(nv/2.0),(double)(nv2/2.0))


#define pe2_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP2,STEP3,VMULT2,VMULT3,WAIT)     \
           phase_encode3_oblshapedgradient(PAT,PAT,PAT,(double)(WIDTH),                    \
                                (double)(STAT1),(double)(STAT2),(double)(STAT3),           \
                                0.0,(double)(STEP2),(double)(STEP3),                       \
                                0,(int)VMULT2,(int)VMULT3,                                 \
                                0.0,(double)(nv/2.0),(double)(nv2/2.0),                    \
                                1, (int)(WAIT+0.5), 0)

#define var2_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP2,STEP3,VMULT2,VMULT3,WAIT)    \
           phase_encode3_oblshapedgradient(PAT,PAT,PAT,(double)(WIDTH),                    \
                                (double)(STAT1),(double)(STAT2),(double)(STAT3),           \
                                0.0,(double)(STEP2),(double)(STEP3),                       \
                                0,(int)VMULT2,(int)VMULT3,                                 \
                                0.0,1.0,1.0,                                               \
                                1, (int)(WAIT+0.5), 0)


#define pe3_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP1,STEP2,STEP3,VMULT1,VMULT2,VMULT3,WAIT) \
           phase_encode3_oblshapedgradient(PAT,PAT,PAT,(double)(WIDTH),                             \
                                (double)(STAT1),(double)(STAT2),(double)(STAT3),                    \
                                (double)(STEP1),(double)(STEP2),(double)(STEP3),                    \
                                (int)VMULT1,(int)VMULT2,(int)VMULT3,                                \
                                (double)(nv3/2.0),(double)(nv/2.0),(double)(nv2/2.0),               \
                                1, (int)(WAIT+0.5), 0)

#define var3_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP1,STEP2,STEP3,VMULT1,VMULT2,VMULT3,WAIT) \
           phase_encode3_oblshapedgradient(PAT,PAT,PAT,(double)(WIDTH),                             \
                                (double)(STAT1),(double)(STAT2),(double)(STAT3),                    \
                                (double)(STEP1),(double)(STEP2),(double)(STEP3),                    \
                                (int)VMULT1,(int)VMULT2,(int)VMULT3,                                \
                                1.0,1.0,1.0, 1, (int)(WAIT+0.5), 0)

#define csi3_shapedgradient(PAT,WIDTH,STAT1,STAT2,STAT3,STEP1,STEP2,STEP3,VMULT1,VMULT2,VMULT3,WAIT) \
           phase_encode3_oblshapedgradient(PAT,PAT,PAT,(double)(WIDTH),                             \
                                (double)(STAT1),(double)(STAT2),(double)(STAT3),                    \
                                (double)(STEP1),(double)(STEP2),(double)(STEP3),                    \
                                (int)VMULT1,(int)VMULT2,(int)VMULT3,                                \
                                (double)(nv/2.0),(double)(nv2/2.0),(double)(nv3/2.0),               \
                                1, (int)(WAIT+0.5), 0)

#define init_rfpattern(pattern_name,pulse_struct,steps)  \
                    init_RFpattern(pattern_name, pulse_struct, IRND(steps))

#define init_decpattern(pattern_name,pulse_struct,steps)  \
                    init_DECpattern(pattern_name, pulse_struct, IRND(steps))

#define init_gradpattern(pattern_name,pulse_struct,steps)  \
                    init_Gpattern(pattern_name, pulse_struct, IRND(steps))

/*---------------------------------------------------------------*/
/*- End SISCO defines 						-*/
/*---------------------------------------------------------------*/

/*---------------------------------------------------------------*/
/*- temporary defines to disable some imaging related functions -*/
/*---------------------------------------------------------------*/

#define calc_amp_tc(chan,eccno,amp,tc)   notImplemented()

/*---------------------------------------------------------------*/
/*- End temporary defines                                       -*/
/*---------------------------------------------------------------*/

#ifndef DPS
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
extern void F_initval(double value, int index);
extern void G_initval(double value, int index);
extern void settable(int tablename, int numelements, int tablearray[]);
extern void inittable(int tablename, int numelements);
extern void getelem(int tablename, int indxptr, int dstptr);
extern void putelem(int tablename, int indxptr, int dstptr);
extern void getTabSkip(int tablename, int indxptr, int dstptr);

extern void loop_check();
extern void peloop(char state, double max_count, int apv1, int apv2);
extern void peloop2(char state, double max_count, int apv1, int apv2);
extern void endpeloop(char state, int apv1);
extern void msloop(char state, double max_count, int apv1, int apv2);
extern void endmsloop(char state, int apv1);
extern void starthardloop(int count);
extern void endhardloop();
extern int loop(int count, int counter);
extern void endloop(int counter);
extern void ifzero(int rtvar);
extern void ifmod2zero(int rtvar);
extern void elsenz(int rtvar);
extern void endif(int rtvar);
extern void push(int word);
extern int pop();
extern int validrtvar(int rtvar);
extern void looptimepush(double word);
extern double looptimepop();

extern void rgradient(char axis, double value);
extern void setreceiver(int phaseptr);
extern int  loadtable(char *infilename);
extern void tablesop(int operationtype, int tablename, int scalarval, int modval);
extern void tabletop(int operationtype, int table1name, int table2name, int modval);

#include "Bridge.h"
#endif

#endif

#else

/*---------------------------------------------------------------*/
/*- The following are necessary for dps to work                 -*/
/*---------------------------------------------------------------*/

/***************

#define offsetlist(posarray, gssval, resfrq, listarray, nsval, mode)                \
    notImplemented()

#define gen_offsetlist(posarray, gssval, resfrq, listarray, nsval, mode, rfch)      \
    notImplemented()

#define offsetglist(posarray, gssvalarr, resfrq, listarray, nsval, mode)            \
    notImplemented()

#define gen_offsetglist(posarray, gssvalarr, resfrq, listarray, nsval, mode, rfch)  \
    notImplemented()

#define shapelist(pat, width, listarray, nsval, mode)                               \
    notImplemented()

#define gen_shapelist(pat, width, listarray, nsval, mode, rfch)                     \
    notImplemented()

#define shapedpulselist(listid, width, rtvar1, rof1, rof2, mode, rtvar2)            \
    genRFShapedPulseWithOffset(width, "shapedpulselist", (int)rtvar1, (double)rof1, (double)rof2, 0.0, OBSch)

#define gen_shapedpulselist(listid, width, rtvar1, rof1, rof2, mode, rtvar2, rfch)  \
    genRFShapedPulseWithOffset(width, "gen_shapedpulselist", (int)rtvar1, (double)rof1, (double)rof2, 0.0, rfch)

#define shapedpulseoffset(pat, width, rtvar, rof1, rof2, offset)                    \
    genRFShapedPulseWithOffset(width, "shapedpulseoffset", (int)rtvar, (double)rof1, (double)rof2, (double)offset, OBSch)

#define gen_shapedpulseoffset(pat, width, rtvar, rof1, rof2, offset, rfch)                    \
    genRFShapedPulseWithOffset(width, "shapedpulseoffset", (int)rtvar, (double)rof1, (double)rof2, (double)offset, rfch)


#define nowait_loop(loops, rtvar1, rtvar2)                      \
                    loop(rtvar1, rtvar2)

#define nowait_endloop(rtvar)                                   \
                    endloop(rtvar)

#define nwloop(loops, rtvar1, rtvar2)                      \
                    loop(rtvar1, rtvar2)

#define endnwloop(rtvar)                                   \
                    endloop(rtvar)

***************/

#endif
