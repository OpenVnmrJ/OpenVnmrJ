/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*
 */
#ifndef DPS

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "acqparms2.h"
#include "abort.h"
#include "group.h"
#include "rfconst.h"
#include "aptable.h"
#include "apdelay.h"
#include "wetfuncs.h"

extern int initializeSeq;
extern double getval(const char *name);

#define dcplrphase(phaseptr)					\
			setSAP(phaseptr,DODEV)

#define decphase(phaseptr)                            		\
			setphase90(DODEV,phaseptr)

#define decoffset(value)					\
			offset(value,DODEV)

#define decpower(value)						\
			rlpower(value,DODEV)

#define decprgoff()						\
			prg_dec_off(2,DODEV)

#define decprgon(name, pp_90, pp_res)				\
			prg_dec_on(name,pp_90,pp_res,DODEV)

#define decpwrf(value)						\
			rlpwrf(value,DODEV)

#define decpulse(width, phaseptr)   				\
			decrgpulse(width,phaseptr,0.0,0.0)

/* #define decshaped_pulse(shape, pws, phs, rx1, rx2)		\
/* 			genshaped_pulse(shape,pws,phs,rx1,rx2,0.0,0.0,DODEV)
/*  */
#define decspinlock(name, pp_90, pp_res, phase, nloops)      \
			genspinlock(name,pp_90,pp_res,phase,nloops,DODEV)

#define decstepsize(step)					\
			stepsize(step,DODEV)

#define obsblank()						\
			rcvron()

#define obsoffset(value)					\
			offset(value,TODEV)

#define obspower(value)						\
			rlpower(value,TODEV)

#define obsprgoff()						\
			prg_dec_off(2,TODEV)

#define obsprgon(name, pp_90, pp_res)				\
			prg_dec_on(name, pp_90, pp_res, TODEV)

#define obspulse()                                              \
			rgpulse(pw,oph,rof1,rof2)

#define obspwrf(value)						\
			rlpwrf(value,TODEV)

#define obsstepsize(step)					\
			stepsize(step,TODEV)

#define obsunblank()						\
			rcvroff()

#define pulse(width,phaseptr)                  			\
			rgpulse(width,phaseptr,rof1,rof2)

#define setautoincrement(tname)					\
			Table[tname - BASEINDEX]->auto_inc =	\
			TRUE

#define setdivnfactor(tname, dfactor) 				\
			Table[tname - BASEINDEX]->divn_factor =	\
			dfactor

#define setExpTime(duration)    g_setExpTime( (double) duration)

/* #define shaped_pulse(shape, pws, phs,rx1, rx2)			\
/* 			genshaped_pulse(shape,pws,phs,rx1,rx2,0.0,0.0,TODEV)
/* */
#define spinlock(name, pp_90, pp_res, phase, nloops)      \
			genspinlock(name,pp_90,pp_res,phase,nloops,TODEV)

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

#define txphase(phaseptr)					\
			setphase90(TODEV,phaseptr)

#define	vagradient(glevel, theta, phi)				\
			VAGRAD(glevel)

#define xmtrphase(phaseptr)					\
			setSAP(phaseptr,TODEV)
#endif

#include "PboxM_psg.h"
