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
#include "pvars.h"

extern int initializeSeq;
extern double getval(const char *name);
extern double getvalnwarn(const char *variable);
extern void   getstr(const char *variable, char buf[]);
extern void   getstrnwarn(const char *variable, char buf[]);
extern int DPSprint(double timev, const char *format, ...);
extern int DPStimer(int code, int subcode, int vnum, int fnum, ...);

extern void loop(codeint count,codeint counter);
extern void endloop(codeint counter);
extern void starthardloop(int rtindex);
extern void endhardloop();
extern void ifzero(codeint rtvar);
extern void ifmod2zero(codeint rtvar);
extern void elsenz(codeint rtvar);
extern void endif(codeint rtvar);
extern void assign(int a, int b);
extern void add(int a, int b, int c);
extern void sub(int a, int b, int c);
extern void dbl(int a, int b);
extern void hlv(int a, int b);
extern void initval(double rc, int  stl);
extern void modn(int a, int b, int c);
extern void mod4(int a, int b);
extern void mod2(int a, int b);
extern void mult(int a, int b, int c);
extern void divn(int a, int b, int c);
extern void incr(int a);
extern void decr(int a);
extern void settable(codeint tablename, int numelements, int tablearray[]);
extern void getelem(codeint tablename,codeint indxptr,codeint dstptr);
extern void delay(double delay);
extern void hsdelay(double time);
extern void rgpulse(double width, int phaseptr, double rx1, double rx2);
extern void decrgpulse(double width, int phaseptr, double rx1, double rx2);
extern void simpulse(double t1, double t2, int ph1, int ph2, double rx1, double rx2);
extern void declvlon();
extern void setSAP(int rtvar, int device);
extern void declvloff();
extern void gate(int gatebit, int on);
extern void stepsize(double step, int device);
extern void status(int f);
extern void rgradient(char axis, double value);
extern void zgradpulse(double gval,double gdelay);
extern void rcvron();
extern void rcvroff();
extern void decon();
extern void xmtron();
extern void decoff();
extern void xmtroff();
extern void obs_pw_ovr(int longpulse);
extern void dec_pw_ovr(int longpulse);
extern void lk_sample();
extern void lk_hold();
extern void acquire(double datapts,double dwell);
extern void offset(double freq, int device);
extern void rlpower(double value, int device);
extern void rlpwrf(double value, int device);
extern void setphase90(int device, int value);
extern void setreceiver(int phaseptr);

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
 * 			genshaped_pulse(shape,pws,phs,rx1,rx2,0.0,0.0,DODEV)
 */
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
 * 			genshaped_pulse(shape,pws,phs,rx1,rx2,0.0,0.0,TODEV)
 */
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
