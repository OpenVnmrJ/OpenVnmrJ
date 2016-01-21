/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************************
*   HISTORY:
*     Revision 1.2  2006/08/29 17:25:30  deans
*     1. changed sgl includes to use quotes (vs brackets)
*     2. added ncomm sources
*     3. mods to Makefile
*     4. added makenvpsg.sgl and makenvpsg.sgl.lnx
*
*     Revision 1.1  2006/08/23 14:10:01  deans
*     *** empty log message ***
*
*     Revision 1.1  2006/08/22 23:30:01  deans
*     *** empty log message ***
*
*     Revision 1.3  2006/07/11 20:09:58  deans
*     Added explicit prototypes for getvalnowarn etc. to sglCommon.h
*     - these are also defined in  cpsg.h put can't #include that file because
*       gcc complains about the "extern C" statement which is only allowed
*       when compiling with g++ (at least for gcc version 3.4.5-64 bit)
*
*     Revision 1.2  2006/07/07 20:10:19  deans
*     sgl library changes
*     --------------------------
*     1.  moved most core functions in sgl.c to sglCommon.c (new).
*     2. sgl.c now contains only pulse-sequence globals and
*          parameter initialization functions.
*     3. sgl.c is not built into the nvpsg library but
*          is compiled in with the user sequence using:
*          include "sgl.c"
*        - as before ,so sequences don't need to be modified
*     4. sgl.h is no longer used and has been removed from the project
*
*     Revision 1.1  2006/07/07 01:12:40  mikem
*     modification to compile with psg
*
***************************************************************************/
#include "sglCommon.h"
#include "sglPrepulses.h"


/***********************************************************************
*  Function Name: create_fatsat
*  Example:    create_fatsat();
*  Purpose:    Calculates and creates the FATSAT crusher gradient, and
*              calibrates the RF pulsepower
*  Input
*     Formal:  
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void create_fatsat()
{
  char    fsatswap[MAXSTR],fsatswapref[MAXSTR];
  double  fsatpulses;
  getstrnwarn("fsatswap",fsatswap);
  getstrnwarn("fsatswapref",fsatswapref);
  fsatpulses = getvalnwarn("fsatpulses");
  if (fsatpulses == 0) fsatpulses = 1;
  if (fsat[0]=='y') { 
    if (fsatswap[0] == 'y') {
      if (fsatswapref[0] == 'y') {
        resto = resto + fsatfrq;
        fsatfrq = -fsatfrq;
      } else
        fsatfrq = 0;
    }
    /* RF power ***************************************/ 
    shape_rf(&fsat_rf,"pfsat",fsatpat,pfsat,flipfsat,rof1,rof2);
    calc_rf(&fsat_rf,"tpwrfsat","tpwrfsatf");

    /* Crusher gradient *******************************/ 
    init_generic(&fsatcrush_grad,"fsatcrush",gcrushfs,tcrushfs);
/*  Sine shape gives error when fixed ramps are specified
    fsatcrush_grad.shape = SINE;
*/
    calc_generic(&fsatcrush_grad,WRITE,"gcrushfs","tcrushfs");

    /* Total duration of fat sat pre-pulse ************/
    fsatTime = fsatpulses*(fsatcrush_grad.duration + fsat_rf.rfDuration + rof1 + rof2);
  }
}

/***********************************************************************
*  Function Name: fatsat
*  Example:    fatsat();
*  Purpose:    Pulse sequence event code for the FATSAT pulse
*  Input
*     Formal: 
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal: 
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void fatsat()
{
  int     i;
  double  fsatpulses;
  fsatpulses = getvalnwarn("fsatpulses");
  if (fsatpulses == 0) fsatpulses = 1;

  if (fsat[0] == 'y') {
    /*  FATSAT PULSE  *************************/       
    obspower(fsat_rf.powerCoarse);
    obspwrf(fsat_rf.powerFine);
    for (i=0; i<fsatpulses; i++) {
      shapedpulseoffset(fsat_rf.pulseName,fsat_rf.rfDuration,zero,rof1,rof2,fsatfrq);
      obl_shapedgradient(fsatcrush_grad.name,fsatcrush_grad.duration,fsatcrush_grad.amp,fsatcrush_grad.amp,fsatcrush_grad.amp,WAIT);
    }
  } 
}   
   
/***********************************************************************
*  Function Name: create_mtc
*  Example:    create_mtc();
*  Purpose:    Calculates and creates the MTC crusher gradient, and
*              calibrates the RF pulse power
*  Input
*     Formal:  
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void create_mtc()
{
  if (mt[0]=='y') { 
    /* RF power ***************************************/ 
    shape_rf(&mt_rf,"pmt",mtpat,pmt,flipmt,rof1,rof2);
    calc_rf(&mt_rf,"tpwrmt","tpwrmtf");

    /* Crusher gradient *******************************/ 
    init_generic(&mtcrush_grad,"mtcrush",gcrushmt,tcrushmt);
/*  Sine shape gives error when fixed ramps are specified
    mtcrush_grad.shape = SINE; 
*/
    calc_generic(&mtcrush_grad,WRITE,"gcrushmt","tcrushmt");
  
    /* Total duration of MTC pre-pulse ****************/
    mtTime = mtcrush_grad.duration + mt_rf.rfDuration + rof1 + rof2;
 }   
}

/***********************************************************************
*  Function Name: mtc
*  Example:    mtc();
*  Purpose:    Pulse sequence event code for the MTC pulse
*  Input
*     Formal:  *crusher - crusher gradient structure
*              *rf      - RF pulse structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *crusher - crusher gradient structure
*              *rf      - RF pulse structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void mtc()
{
if (mt[0] == 'y') 
   {
   /*  MTC PULSE  *************************/       
   obspower(mt_rf.powerCoarse);
   obspwrf(mt_rf.powerFine);
   shapedpulseoffset(mt_rf.pulseName,mt_rf.rfDuration,zero,rof1,rof2,mtfrq);
   obl_shapedgradient(mtcrush_grad.name,mtcrush_grad.duration,
       mtcrush_grad.amp,mtcrush_grad.amp,mtcrush_grad.amp,WAIT);
   } 
}


/***********************************************************************
*  Function Name: create_satbands
*  Example:    create_satbands();
*  Purpose:    Calculates and creates the SATBAND selection and crusher gradient
*              sets RF pulse power
*  Input
*     Formal:  Uses standard parameters satthk, satpos, sphi, spsi, stheta,
               satflip, psat, sat_grad, satamp, satcrush_grad
*     Private: none
*     Public:  none
*  Output
*     Return:  total satbands time
*     Formal:  none
*     Private: none
*     Public:  none
*  Notes:
***********************************************************************/
void create_satbands()
{
	int     nthk,npos,npsi,nphi,ntheta,i,_i;
	double  minthk;
	
	if( sat[0] == 'y' )
	{
		if ((ix > 1) && !sglarray) return;
		/* Get values for parameters, check for matching array sizes */
		flipsat = getval("flipsat");
		nthk = getarray("satthk",satthk);
		npos = getarray("satpos",satpos);
		npsi = getarray("spsi",satpsi);
		nphi = getarray("sphi",satphi);
		ntheta = getarray("stheta",sattheta);
		nsat = nthk;
		if ( (npos != nsat) || (npsi != nsat) || (nphi != nsat) || (ntheta != nsat) ) {
			abort_message("ERROR create_satbands: Mismatch in array size of satband parameters.\n");
		}
		
		/* Smallest satthk value determines selection gradient amplitude */
		minthk = satthk[0];
		for (i=1; i<nsat; i++) {
			minthk = MIN(minthk,satthk[i]);
		}
		
		/* Initialize satband gradient, crusher, and RF pulse *******/
		init_slice(&sat_grad,"gsat",minthk);
		init_generic(&satcrush_grad,"satcrush",gcrushsat,tcrushsat);
		shape_rf(&sat_rf,"psat",satpat,psat,flipsat,rof1,rof2);
		
		/* Calculate RF power, slice and crusher grads ****/
		calc_rf(&sat_rf,"satpwr","satpwrf");
		calc_slice(&sat_grad,&sat_rf,WRITE,"");
		calc_generic(&satcrush_grad,WRITE,"","");
		
		/* Calculate list of satband gradient amplitudes **/
		for (i=0; i<nsat; i++) {
			/* Slice grad is inversely proportional to band thickness **/
			satamp[i] = sat_grad.amp*minthk/satthk[i];
		}
		
		/* Compute total satband loop time, including crushers **/
		satTime = nsat*(sat_grad.duration + satcrush_grad.duration + 10e-6);
	}
}  


/***********************************************************************
*  Function Name: satbands
*  Example:    satbands();
*  Purpose:    Pulse sequence event code for the SATBANDS
*  Input
*     Formal:  *crusher - crusher gradient structure
*              *rf      - RF pulse structure
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  *crusher - crusher gradient structure
*              *rf      - RF pulse structure
*     Private: none
*     Public:  none
*  Notes:      none
***********************************************************************/
void satbands()
{

   int i,shapelist1;
   double  freqlist[MAXNSAT];

  /* Satband sequence events */
  if (sat[0] == 'y') {
    offsetglist(satpos,satamp,0,freqlist,nsat,'i');
    shapelist1 = shapelist(sat_rf.pulseName,sat_grad.rfDuration,freqlist,nsat,sat_grad.rfFraction,'i');
    obspower(sat_rf.powerCoarse);
    obspwrf(sat_rf.powerFine);
    delay(4e-6);
    for (i=0; i<nsat; i++) {

      /* Set slice select Euler angles **************************/
      set_rotation_matrix(satpsi[i],satphi[i],sattheta[i]);

      /* Start slice gradient, then apply RF pulse **************/
      obl_shapedgradient(sat_grad.name,sat_grad.duration,0,0,satamp[i],NOWAIT);
      delay(sat_grad.tramp);
      shapedpulselist(shapelist1,sat_grad.rfDuration,oph,rof1,rof2,'i',i);
      delay(sat_grad.tramp);

      /* Apply crusher gradient *********************************/
      rotate();

      obl_shapedgradient(satcrush_grad.name,satcrush_grad.duration,
          satcrush_grad.amp,satcrush_grad.amp,satcrush_grad.amp,WAIT);
    }
  }
}


/***********************************************************************
*  Function Name: create_inversion_recovery
*  Example:    create_inversion_recovery();
*  Purpose:    Creates slice selective IR gradient and crusher gradient
*              sets RF pulse power
*
***********************************************************************/
void create_inversion_recovery()
{

  if (ir[0] == 'y') {

    /* Calculate RF */
    shape_rf(&ir_rf,"pi",pipat,pi,flipir,rof1,rof2);
    calc_rf(&ir_rf,"tpwri","tpwrif");

    /* Calculate slice select gradient */
    if (thkirfact == 0.0) thkirfact=1.0; /* thkirfact may not be in parameter set */
    init_slice(&ssi_grad,"ssi",thk*thkirfact);
    calc_slice(&ssi_grad,&ir_rf,WRITE,"");
    putvalue("gssir",ssi_grad.ssamp);
    irgradTime = ssi_grad.duration;

    /* Calculate slice select crusher */
    if (tcrushir>0.0) {
      init_generic(&ircrush_grad,"ircrush",gcrushir,tcrushir);
      calc_generic(&ircrush_grad,WRITE,"","");
      irgradTime += ircrush_grad.duration;
    }

  }

}


/***********************************************************************
*  Function Name: calc_irTime
*  Example:    trmin = calc_irTime(tauti,trmin,mintr[0],tr,&trtype);
*  Purpose:    Calculates the time for all IR components:
*              Calculates the minimum time for all IR components
*              tauti and trmin are required for the calculation
*              Checks if trmin has been selected - if so returns the trmin
*              Otherwise, for distributed tr, tunes the IR prescription to fill the tr
*              tauti, trmin, tr are required for the calculation
*              trmin is returned so tr_delay can be calculated
*              trtype is set if appropriate (trtype=1 for non distributed tr)
*
***********************************************************************/
double calc_irTime(double tiadd,double trepmin,char mintrepflag,double trep,int *treptype)
{

  double trepmininput;

  if (ir[0] != 'y') return(0.0);

  /* Calculate the minimum irTime for all IR components */
  calc_minirTime(tiadd,trepmin);

  /* If min tr is selected initialize rtvars and return irTime */
  if ((mintrepflag == 'y') || (*treptype)) {
    init_vinvrec();
    return(irTime);
  }

  /* Take copy of input trepmin */
  trepmininput=trepmin;

  /* Add irTime to trepmin */
  trepmin += irTime;

  /* Abort if trep is too short */
  if (FP_LT(trep,trepmin)) {
    abort_message("TR too short.  Minimum TR = %.2fms\n",trepmin*1000+0.005);
  }

  /* Otherwise expand IR components to fill trep */
  trepmin = tune_irTime(tiadd,trepmin,trep,treptype);
  init_vinvrec();
  return(trepmin-trepmininput);

}


/***********************************************************************
*  Function Name: calc_minirTime
*  Example:    calc_minirTime(tauti,trmin);
*  Purpose:    Calculates the minimum time for all IR components
*              tauti and trmin are required for the calculation
*
***********************************************************************/
void calc_minirTime(double tiadd,double trepmin)
{

  if (ir[0] == 'y') {

    /* Check nsblock, the number of slices blocked together */
    /* Probably this has been done already for triggering, but we do again to be sure */
    check_nsblock();
    /* Set nsirblock = number of slices blocked together for inversion recovery */
    if (blockslices) nsirblock = nsblock;
    else nsirblock = ns;
    /* Check minimum inversion time */
    timin = ssi_grad.rfCenterBack + 4e-6 + tiadd;
    if (tcrushir>0.0) timin += ircrush_grad.duration;
    if (minti[0] == 'y') {
      ti = timin;
      putvalue("ti",ti+1.25e-8);
    }
    if (ti < timin)
      abort_message("TI too short.  Minimum TI = %.2fms\n",timin*1000+0.005);
    /* Check to see how we should prescribe ir */
    irmincycle = (trepmin/ns<irgradTime+4e-6) ? irgradTime+4e-6 : trepmin/ns;
    if (!irsequential && (ti >= (nsirblock-1)*irmincycle+timin)) {
      /* We can run fully interleaved ... or we have a standard slice loop */
      nirinterleave = nsirblock;
      ti1_delay = irmincycle - irgradTime - 4e-6;
      ti2_delay = ti - (nsirblock-1)*irmincycle - timin;
      ti3_delay = (trepmin/ns<irgradTime+4e-6) ? irgradTime+4e-6-trepmin/ns : 0.0;
    } else {
      /* Figure number of slices to interleave over */
      irmincycle = trepmin/ns + irgradTime + 8e-6;
      nirinterleave = (int)((ti-timin)/irmincycle);
      if (irsequential) nirinterleave = 0;
      ti2_delay = (ti-timin-nirinterleave*irmincycle)/(nirinterleave+1);
      ti1_delay = irmincycle - irgradTime + ti2_delay;
      ti3_delay = irgradTime + 8e-6 + ti2_delay;
      nirinterleave += 1; /* so nirinterleave represents the starting number of pulses
                             just as it does for fully interleaved above */
    }
    /* timin above includes a minimum 4 usec delay */
    ti2_delay += 4e-6;
    /* For minimum tr ti4_delay is zero */
    ti4_delay = 0.0;
    /* Calculate duration of IR components */
    irTime = (ns/nsirblock)*((nirinterleave-1)*(irgradTime+4e-6+ti1_delay)
            +(nsirblock-nirinterleave+1)*(irgradTime+4e-6+ti2_delay)
            +(nirinterleave-1)*ti3_delay);  
    /* For sequential inversion always prescribe a pulse */
    if (nirinterleave == 1) nsirblock = 1;

  }

}


/***********************************************************************
*  Function Name: tune_irTime
*  Example:    trmin=tune_irTime(tauti,trmin,tr,&trtype);
*  Purpose:    For distributed tr tune the IR prescription to fill the tr
*              tauti, trmin, tr and trtype are required for the calculation
*              For distributed tr a new trmin is returned so tr_delay can be calculated
*
***********************************************************************/
double tune_irTime(double tiadd,double trepmin,double trep,int *treptype)
{

  double irtrmin,irtr,maxcycle,cycle;
  int    i;

  if ((ir[0] != 'y') || (*treptype)) return(trepmin);
  else {
    /* For distributed tr spacing spread the IR timings to fill the specified tr */
    /* For minimum tr calculation maximum interleave is used */
    /* For distributed tr spacing figure the minimum interleave */
    trepmin-=irTime; trepmin/=ns; /* trepmin (usually trmin) for 1 slice */
    irtrmin=trepmin-tiadd+ssi_grad.rfCenterFront+ti; /* trepmin for 1 slice with IR */
    for (i=1;i<nirinterleave;i++) {
      irmincycle=irtrmin/i;
      irtr=irmincycle*(nsirblock+i-1);
      if (irtr<trep*nsirblock/ns) break;
    }
    nirinterleave=i; /* the minimum interleave */
    if (nirinterleave>1) { /* if interleave>1 stretch the cycle to fill tr */
      maxcycle=(ti-timin)/(nirinterleave-1); /* calculate the maximum cycle for this interleave */
      irtr=maxcycle*(nsirblock+nirinterleave-2)+irgradTime+trepmin; /* the tr with maximum cycle */
      if (irtr<=trep*nsirblock/ns) cycle=maxcycle; /* if trep allows, use maximum cycle */
      else cycle=(trep*nsirblock/ns-irgradTime-trepmin-ti+timin)/(nsirblock-1)-4e-6;
      ti1_delay=cycle-irgradTime-4e-6;
      if (nirinterleave == nsirblock) { /* fully interleaved */
        ti2_delay=ti-(nsirblock-1)*cycle-timin;
      } else {
        ti2_delay = ti-timin-(nirinterleave-1)*cycle;
        ti4_delay = cycle-trepmin-irgradTime-ti2_delay;
      }
      ti3_delay = cycle-trepmin;
      *treptype = 1; /* force non distributed tr in case maximum cycle is used */
    } else { /* otherwise just calculate distributed tr as normal */
      ti2_delay = ti-timin;
    }
    irTime = (ns/nsirblock)*((nirinterleave-1)*(irgradTime+4e-6+ti1_delay)
            +(nsirblock-nirinterleave)*(irgradTime+4e-6+ti2_delay+ti4_delay)
            +irgradTime+4e-6+ti2_delay
            +(nirinterleave-1)*ti3_delay); 
    /* Reset trepmin to include IR components */
    trepmin *=ns; trepmin += irTime;
    return(trepmin);
  }

}


/***********************************************************************
*  Function Name: init_vinvrec
*  Example:    init_vinvrec();
*  Purpose:    Initialize the real-time variables used for the IR module
*
***********************************************************************/
void init_vinvrec()
{

  /* Reserved real time variables used for IR:                     *
   * vslice_ctr    Slice counter                                   *
   * vslices       Total number of slices                          *
   * virblock      Number of slices per inversion recovery block   *
   * vnirpulses    Number of ir pulses first pass through ir block *
   * vir           Flag to prescribe inversion pulses              *
   * virslice_ctr  Inversion slice counter                         *
   * vnir          Number of ir pulses in loop (variable)          *
   * vnir_ctr      ir pulse loop counter                           *
   * vtest         Test variable                                   */

  assign(zero,vslice_ctr);
  F_initval((double)ns,vslices);
  F_initval((double)nsirblock,virblock);
  F_initval((double)nirinterleave,vnirpulses);

}


/***********************************************************************
*  Function Name: inversion_recovery
*  Example:    inversion_recovery();
*  Purpose:    Prescribe the IR module
*
***********************************************************************/
void inversion_recovery()
{

  int shapeIR;
  double freqIR[MAXSLICE];

  /* Reserved real time variables used for IR:                     *
   * vslice_ctr    Slice counter                                   *
   * vslices       Total number of slices                          *
   * virblock      Number of slices per inversion recovery block   *
   * vnirpulses    Number of ir pulses first pass through ir block *
   * vir           Flag to prescribe inversion pulses              *
   * virslice_ctr  Inversion slice counter                         *
   * vnir          Number of ir pulses in loop (variable)          *
   * vnir_ctr      ir pulse loop counter                           *
   * vtest         Test variable                                   */

  if (ir[0] == 'y') {

    offsetlist(pss,ssi_grad.ssamp,0,freqIR,ns,'c');
    shapeIR = shapelist(ir_rf.pulseName,ssi_grad.rfDuration,freqIR,ns,ssi_grad.rfFraction,'c');

    modn(vslice_ctr,virblock,vtest);
    ifzero(vtest);                    /* if the beginning of an ir block */
      assign(zero,vir);               /* set vir to apply inversion pulses */
      assign(vnirpulses,vnir);        /* set the number of inversion pulses */
    endif(vtest);
    ifzero(vir);                      /* if apply inversion pulses */
      loop(vnir,vnir_ctr);            /* loop over the number of inversion pulses */
        obspower(ir_rf.powerCoarse);
        obspwrf(ir_rf.powerFine);
        delay(4e-6);
        sub(vnir,one,vtest);
        ifzero(vtest);                /* if there is one pulse in the loop */
          delay(ti4_delay);
        endif(vtest);
        obl_shapedgradient(ssi_grad.name,ssi_grad.duration,0,0,ssi_grad.amp,NOWAIT);
        delay(ssi_grad.rfDelayFront);
        shapedpulselist(shapeIR,ssi_grad.rfDuration,oph,rof1,rof2,seqcon[1],virslice_ctr);
        delay(ssi_grad.rfDelayBack);
	if (tcrushir>0.0) 
          obl_shapedgradient(ircrush_grad.name,ircrush_grad.duration,ircrush_grad.amp,ircrush_grad.amp,ircrush_grad.amp,WAIT);
        sub(vnir,vnir_ctr,vtest);
        sub(vtest,one,vtest);
        ifzero(vtest);                /* if this is last pass through the loop */
          delay(ti2_delay);
        elsenz(vtest);
          delay(ti1_delay);
        endif(vtest);
        add(virslice_ctr,one,virslice_ctr); /* update ir slice counter */
      endloop(vnir_ctr);
      modn(virslice_ctr,virblock,vtest);
      ifzero(vtest);                  /* if ir slice is last in block */
        assign(one,vir);              /* don't apply inversion pulses */
      elsenz(vtest);
        assign(one,vnir);             /* else apply pulses one at a time */
      endif(vtest);
    elsenz(vir);                      /* else if don't apply inversion pulses */
      delay(ti3_delay);
    endif(vir);

    incr(vslice_ctr);                 /* increment the slice counter */
    ifrtEQ(vslice_ctr,vslices,vtest); /* if its the last slice */
      assign(zero,vslice_ctr);        /* zero the slice counter */
      assign(zero,virslice_ctr);      /* zero the ir slice counter */
    endif(vtest);

  }

}


/***********************************************************************
*  Function Name: create_arterial_spin_label
*  Example:    create_arterial_spin_label();
*  Purpose:    Creates arterial spin labelling (ASL) components
*              sets RF pulse power
*
***********************************************************************/
void create_arterial_spin_label()
{
  /* The ASL module can not be used with the IR module */
  /* The ASL module uses the IR module real time variables, real time variables v41,v42 and real time tables t57-t60 */

  int numrfch;
  int caslsinemod=FALSE;
  int stardoubletag=FALSE;
  double power,powerlimit,aslb1max,fpwrscale;
  double minpss,maxpss,centrepss;
  double *pssval,*controlpos;
  int npssvals,sliceindex=0;
  double tmpval;
  double trisesave;
  char aslteststring[30];
  int asltestval=1,*tabvals;
  int i;

  /* The following real time variables reserved for IR are used in ASL: *
   * vslice_ctr    Slice counter                                        *
   * vslices       Total number of slices                               *
   * virblock      Number of slices per inversion recovery block        *
   * vnirpulses    Number of ir pulses first pass through ir block      *
   * vir           Flag to prescribe inversion pulses                   *
   * virslice_ctr  Inversion slice counter                              *
   * vnir          Number of ir pulses in loop (variable)               *
   * vnir_ctr      ir pulse loop counter                                */
  /* Rename some to be more meaningful */
  int vnps      = virblock;     /* Number of PS pulses */
  int vnips     = vnirpulses;   /* Number of IPS pulses */
  int vnq2      = vir;          /* Number of Q2TIPS pulses */
  int vspoil    = virslice_ctr; /* Gradient spoil multiplier */
  int vloop_ctr = vnir;         /* Loop counter */
  int vpwrf     = vnir_ctr;     /* Fine power */

  if (asl[0] == 'y') {

    /* Can't implement IR and ASL */
    if (ir[0] == 'y') return;

/* Set fine power scaling factor */
/* Need a mechanism to figure if DD2 or VNMRS. fpwrscale = 1.0 for VNMRS, 16.0 for DD2 */
fpwrscale=1.0;

    /* Only cater for separate tagging coil if there's a 3rd channel */
    /* Only cater for separate tagging coil for CASL */
    switch (asltype) {
      case CASL:
        /* If there's no 3rd channel then don't use asltagcoil */
        if (P_getreal(GLOBAL,"numrfch",&tmpval,1) < 0)
          numrfch=0;
        else
          numrfch = (int)tmpval;
        if (numrfch<3) asltagcoil[0]='n';
        break;
      default:
        asltagcoil[0]='n';
        break;
    }

    /* Don't implement presaturation or Q2TIPS with CASL */
    switch (asltype) {
      case CASL:
        ps[0]='n';
        q2tips[0]='n';
        break;
      default:
        break;
    }

    /* In compressed multislice mode CASL and STAR require particular controls */
    switch (asltype) {
      case CASL:
        if (seqcon[1]=='c' && ns>1) {
          /* The sine modulated control may not be required for a dedicated tag coil */
          if (asltagcoil[0] == 'y') {
            if (!strcmp(caslctrl,"")) {
              strcpy(caslctrl,"sinemod");
              putCmd("caslctrl = '%s'",caslctrl);
            }
          } else {
            strcpy(caslctrl,"sinemod");
            putCmd("caslctrl = '%s'",caslctrl);
          }
        }
        if (!strcmp(caslctrl,"sinemod")) caslsinemod=TRUE;
        break;
      case STAR:
        if (seqcon[1]=='c' && ns>1) {
          strcpy(starctrl,"doubletag");
          putCmd("starctrl = '%s'",starctrl);
        }
        if (!strcmp(starctrl,"doubletag")) stardoubletag=TRUE;
        break;
      default:
        break;
    }

    /* It seems to take time (a couple of secs) to phase ramp long shaped CASL pulses, so allow frequency to be set */
    switch (asltype) {
      case CASL:
        if (caslphaseramp[0]=='n' && caslsinemod) aslphaseramp=FALSE;
        break;
    }

    /* Set ASL tag pulse, control pulse and power */
    switch (asltype) {
      case CASL:
        shape_rf(&asl_rf,"pcasl",pcaslpat,pcasl,flipcasl,rof1,rof1);
        if (caslsinemod) shape_rf(&aslctrl_rf,"pcaslctrl","sine",pcasl,flipcasl,rof1,rof1);
        /* Calculate power (dB) required for the specified B1 (Hz) */
        if (asltagcoil[0] == 'y') {
          /* To trap for a CW power limit we need to know:
               1. 3rd channel full power (63 dB) in W.
               2. tag coil max CW power in W.
             To set a suitable power we need b1max for the tagging coil from a pulse calibration.
             NB. 37 dB (= 2.5 W for 63 dB = 1 kW at probe)
          */
          /* aslb1max is available for a dedicated tag coil */
          aslb1max=getvalnwarn("aslb1max"); 
          power = 63-20*log(aslb1max/caslb1)/log(10);
          powerlimit=getvalnwarn("asltagcoilpmax"); 
        } else {
          /* To trap for a CW power limit we need to know:
               1. 1st channel full power (63 dB) in W.
               2. tag coil max CW power in W.
             To set a suitable power we need b1max for the regular transmit coil from a pulse calibration.
             NB. 40 dB (= 5 W for 63 dB = 1 kW at probe)
          */
          /* b1max is readily available for a regular transmit coil */
          power = 63-20*log(b1max/caslb1)/log(10);
          powerlimit=getvalnwarn("aslpmax"); 
        }
        if (powerlimit<power) power=powerlimit;
        asl_rf.powerCoarse = ceil(power);
        asl_rf.powerFine = exp((asl_rf.powerCoarse-power)/-20*log(10))*FATTN_MAX;
        /* Round fine power for 16-bit resolution */
        asl_rf.powerFine*=16;
        asl_rf.powerFine = ROUND(asl_rf.powerFine);
        asl_rf.powerFine/=16;
        /* Write values to VnmrJ parameters */
        putvalue("tpwrcasl",asl_rf.powerCoarse);
        putvalue("tpwrcaslf",asl_rf.powerFine);
        if (caslsinemod) {
          /* Increase B1 by factor of pi/2 */
          power +=20*log(M_PI/2)/log(10);
          aslctrl_rf.powerCoarse = ceil(power);
          aslctrl_rf.powerFine = exp((aslctrl_rf.powerCoarse-power)/-20*log(10))*FATTN_MAX;
          /* Round fine power for 16-bit resolution */
          aslctrl_rf.powerFine*=16;
          aslctrl_rf.powerFine = ROUND(aslctrl_rf.powerFine);
          aslctrl_rf.powerFine/=16;
        } else {
          aslctrl_rf=asl_rf;
        }
        break;
      default:
        shape_rf(&asl_rf,"pasl",paslpat,pasl,flipasl,rof1,rof1); 
        calc_rf(&asl_rf,"tpwrasl","tpwraslf");
        aslctrl_rf=asl_rf;
        break;
    }

    /* Figure the appropriate slice range
       For a compressed multislice loop a single tag is applied for all slices
       For a standard multislice loop a tag is applied for each slice
    */
    minpss=pss[0]; maxpss=pss[0];
    if (seqcon[1] == 'c') {
      for (i=1;i<ns;i++) { /* figure minimum and maximum slice positions */
        if (pss[i]<minpss) minpss=pss[i];
        if (pss[i]>maxpss) maxpss=pss[i];
      }
    }

    /* Figure suitable aslthk, aslpos, aslpsi, aslphi and asltheta */
    /* For CASL set thickness so init_slice/calc_slice will produce required gradient caslgamp */
    switch (asltype) {
      case CASL:
        if ((FP_LT(asl_rf.flip,FLIPLIMIT_LOW)) || (FP_GT(asl_rf.flip,FLIPLIMIT_HIGH)))
          aslthk = 10*asl_rf.header.bandwidth/(asl_rf.rfDuration*caslgamp*nuc_gamma());
        else
          aslthk = 10*asl_rf.header.inversionBw/(asl_rf.rfDuration*caslgamp*nuc_gamma());
      default:
        break;
    }
    /* If graphically planned use the values that have been read in, otherwise set them */
    if (aslplan[0] == 'n') {
      switch (asltype) {
        case CASL:
          if (asltagrev[0] == 'y') aslpos = maxpss+aslgap+0.05*(thk+aslthk);
          else aslpos = minpss-aslgap-0.05*(thk+aslthk);
          break;
        case FAIR:
          aslthk = thk+asladdthk+10*maxpss-10*minpss;
          aslpos = minpss+0.5*(maxpss-minpss);
          break;
        default:
          aslthk = asltagthk;
          if (asltagrev[0] == 'y') aslpos = maxpss+aslgap+0.05*(thk+aslthk);
          else aslpos = minpss-aslgap-0.05*(thk+aslthk);
          break;
      }
      aslpsi=psi; aslphi=phi; asltheta=theta;
    }

    /* If tag gradient is oblique trise must be increased */
    trisesave=trise;
    if (!trisesqrt3 && FP_EQ(trampfixed,0.0)) {
      if (remainder(aslpsi,90.0) || remainder(aslphi,90.0) || remainder(asltheta,90.0))
        trise = trise*sqrt(3.0);
    }

    /* Calculate the tag gradient */
    init_slice(&asl_grad,"asl",aslthk);
    asl_grad.rollOut=FALSE;
    calc_slice(&asl_grad,&asl_rf,WRITE,"gasl");
    asltaggamp=asl_grad.ssamp;

    /* Calculate frequency offset and phase ramp for tag pulse */
    pssAsl[0]=aslpos;
    offsetlist(pssAsl,asl_grad.ssamp,0,freqAsl,1,'s');
    if (aslphaseramp) {
      if (asltagcoil[0] == 'y') shapeAsl = dec2shapelist(asl_rf.pulseName,asl_grad.rfDuration,freqAsl,1,asl_grad.rfFraction,'s');
      else shapeAsl = shapelist(asl_rf.pulseName,asl_grad.rfDuration,freqAsl,1,asl_grad.rfFraction,'s');
    }

    /* Figure suitable aslctrlthk, aslctrlpos, aslctrlpsi, aslctrlphi and aslctrltheta */
    switch (asltype) {
      case CASL:
        if (caslsinemod) { 
          aslctrlpsi=aslpsi; aslctrlphi=aslphi; aslctrltheta=asltheta;
          aslctrlthk=aslthk; aslctrlpos=aslpos;
        } else {
          /* Need to reflect tag plane in image slice */
          if (aslplan[0] == 'n') aslctrlpos=2*minpss-aslpos;
          else {
            /* Leave aslctrlpos at value read in unless there is more than one slice in the standard loop */
            P_getVarInfo(CURRENT,"pss",&dvarinfo);
            npssvals = (int)dvarinfo.size; 
            if (npssvals>1) {
              if ((pssval=(double *)malloc(npssvals*sizeof(double))) == NULL) nomem();
              S_getarray("pss",pssval,npssvals*sizeof(double)); /* NB S_getarray returns # elements */
              P_getVarInfo(CURRENT,"controlpos",&dvarinfo);
              if ((int)dvarinfo.size == npssvals) {
                if ((controlpos=(double *)malloc(npssvals*sizeof(double))) == NULL) nomem();
                S_getarray("controlpos",controlpos,npssvals*sizeof(double));
                for (i=0;i<npssvals;i++) {
                  if (FP_EQ(minpss,pssval[i])) sliceindex=i;
                }
                aslctrlpos=controlpos[sliceindex];
              }
              else {
                if (!checkflag) abort_message("ASL control positions (controlpos) have not been set correctly, run prep");
              }
            }
          }
        }
        break;
      case FAIR:
        /* The control is a global inversion with no offset */
        if (asltag == -1) asltaggamp=0.0;
        aslctrlpsi=aslpsi; aslctrlphi=aslphi; aslctrltheta=asltheta;
        aslctrlpos=0.0;
        break;
      case STAR:
        if (stardoubletag) {
          aslctrlpsi=aslpsi; aslctrlphi=aslphi; aslctrltheta=asltheta;
          aslctrlthk=aslthk; aslctrlpos=aslpos;
        } else {
          /* Need to reflect tag plane in image slice */
          if (aslplan[0] == 'n') aslctrlpos=2*minpss-aslpos;
          else {
            /* Leave aslctrlpos at value read in unless there is more than one slice in the standard loop */
            P_getVarInfo(CURRENT,"pss",&dvarinfo);
            npssvals = (int)dvarinfo.size; 
            if (npssvals>1) {
              if ((pssval=(double *)malloc(npssvals*sizeof(double))) == NULL) nomem();
              S_getarray("pss",pssval,npssvals*sizeof(double)); /* NB S_getarray returns # elements */
              P_getVarInfo(CURRENT,"controlpos",&dvarinfo);
              if ((int)dvarinfo.size == npssvals) {
                if ((controlpos=(double *)malloc(npssvals*sizeof(double))) == NULL) nomem();
                S_getarray("controlpos",controlpos,npssvals*sizeof(double));
                for (i=0;i<npssvals;i++) {
                  if (FP_EQ(minpss,pssval[i])) sliceindex=i;
                }
                aslctrlpos=controlpos[sliceindex];
              }
              else {
                if (!checkflag) abort_message("ASL control positions (controlpos) have not been set correctly, run prep");
              }
            }
          }
        }
        break;
      case PICORE:
        /* The control is an inversion at same offset as tag but without slice select gradient */
        if (asltag == -1) asltaggamp=0.0;
        aslctrlpsi=aslpsi; aslctrlphi=aslphi; aslctrltheta=asltheta;
        aslctrlpos=aslpos;
        break;
      default:
        break;
    }

    /* Calculate frequency offset and phase ramp for control pulse */
    pssAslCtrl[0]=aslctrlpos;
    offsetlist(pssAslCtrl,asl_grad.ssamp,0,freqAslCtrl,1,'s');
    switch (asltype) {
      /* FAIR and PICORE  control pulse is not in a slice selective gradient */
      case FAIR:
        shapeAslCtrl = shapelist(aslctrl_rf.pulseName,asl_grad.rfDuration,freqAslCtrl,1,0,'s');
        break;
      case PICORE:
        shapeAslCtrl = shapelist(aslctrl_rf.pulseName,asl_grad.rfDuration,freqAslCtrl,1,0,'s');
        break;
      default:
        if (aslphaseramp) {
          if (asltagcoil[0] == 'y') shapeAslCtrl = dec2shapelist(aslctrl_rf.pulseName,asl_grad.rfDuration,freqAslCtrl,1,asl_grad.rfFraction,'s');
          else shapeAslCtrl = shapelist(aslctrl_rf.pulseName,asl_grad.rfDuration,freqAslCtrl,1,asl_grad.rfFraction,'s');
        }
        break;
    }

    /* Calculate tag spoil gradient */
    if (tspoilasl > 0.0) {
      init_generic(&aslspoil_grad,"aslspoil",gspoilasl,tspoilasl);
      calc_generic(&aslspoil_grad,WRITE,"","");
    }

    /* Reset trise in case it was adjusted for oblique tag gradient */
    trise=trisesave;

    /* Initialize asl test string and parameter */
    strcpy(aslteststring,"");
    putCmd("asltestpars = ''");
    putCmd("asltesttag = 0");

    /* Presaturation */
    psTime = 0.0;
    if (ps[0] == 'y') {
      if (psplan[0] == 'n') {
        psthk=10*(maxpss-minpss)+thk+psaddthk; /* maxpss,minpss in cm */
        pspos=minpss+(maxpss-minpss)/2.0;
        pspsi=psi; psphi=phi; pstheta=theta;
      }
      if (wetps[0] == 'y') nps=4;
      if ((tabvals=(int *)malloc(nps*sizeof(int))) == NULL) nomem();
      if (wetps[0] == 'y') {
        flipps=flippsf*161.0;
        shape_rf(&ps_rf,"pps",pspat,pps,flippsf*161.0,rof1,rof2);
        calc_rf(&ps_rf,"tpwrps","tpwrpsf"); 
        tabvals[0] = (int)ROUND(fpwrscale*ps_rf.powerFine*81.4/flipps);
        tabvals[1] = (int)ROUND(fpwrscale*ps_rf.powerFine*101.4/flipps);
        tabvals[2] = (int)ROUND(fpwrscale*ps_rf.powerFine*69.3/flipps);
        tabvals[3] = (int)ROUND(fpwrscale*ps_rf.powerFine);
      } else {
        shape_rf(&ps_rf,"pps",pspat,pps,flipps,rof1,rof2); 
        calc_rf(&ps_rf,"tpwrps","tpwrpsf");
        for (i=0;i<nps;i++) tabvals[i] = (int)ROUND(fpwrscale*ps_rf.powerFine);
      }
      /* Use table t60 for PS fine powers */
      settable(t60,nps,tabvals);
      trisesave=trise;
      if (!trisesqrt3 && FP_EQ(trampfixed,0.0)) {
        if (remainder(pspsi,90.0) || remainder(psphi,90.0) || remainder(pstheta,90.0))
          trise = trise*sqrt(3.0);
      }
      init_slice(&ps_grad,"ps",psthk);
      calc_slice(&ps_grad,&ps_rf,WRITE,"");
      if (tspoilps > 0.0) {
        init_generic(&psspoil_grad,"psspoil",gspoilps,tspoilps);
        calc_generic(&psspoil_grad,WRITE,"","");
        psgamp = psspoil_grad.amp/pow(2.0,nps-1);
        for (i=0;i<nps;i++) tabvals[i] = (int)pow(2.0,nps-1-i);
        /* Use table t59 for PS spoil gradients */
        settable(t59,nps,tabvals);
      }
      free(tabvals);
      trise=trisesave;
      pssPs[0]=pspos;
      offsetlist(pssPs,ps_grad.ssamp,0,freqPs,1,'i');
      shapePs = shapelist(ps_rf.pulseName,ps_grad.rfDuration,freqPs,1,ps_grad.rfFraction,'i');
      psTime = nps*ps_grad.duration+GRADIENT_RES;
      if (tspoilps > 0.0) psTime +=nps*psspoil_grad.duration;
      putvalue("pstime",psTime);
      strcat(aslteststring,"PS");
      putCmd("asltestpars[%d] = 'PS'",asltestval);
      putCmd("asltesttag[%d] = 1",asltestval++);
      /* Hijack IR's virblock (vnps=virblock) for number of PS pulses */
      F_initval((double)nps,vnps);
    }

    /* Update asl test string and parameter (tag and control are always in test string) */
    if (strlen(aslteststring)>1) strcat(aslteststring,"/");
    strcat(aslteststring,"Tag/Ctrl");
    putCmd("asltestpars[%d] = 'Tag'",asltestval);
    putCmd("asltesttag[%d] = 1",asltestval++);
    putCmd("asltestpars[%d] = 'Ctrl'",asltestval);
    putCmd("asltesttag[%d] = -1",asltestval++);

    /* Multiple inversion recovery (MIR) pulses */
    if (mir[0] == 'y') {
      /* Background suppression with MIR pulses requires static background signal
         to be suppressed with either presaturation (PS) or in-plane saturation (IPS).
         If neither are selected force IPS since that is possible for all ASL methods.
      */
      if ((ps[0] != 'y') && (ips[0] != 'y')) {
        ips[0] = 'y';
        putCmd("ips = 'y'");
      }
      pmir = granularity(pmir,GRADIENT_RES);
      rofmir = granularity(rof1,GRADIENT_RES);
      shape_rf(&mir_rf,"pmir",mirpat,pmir,flipmir,rofmir,rofmir); 
      calc_rf(&mir_rf,"tpwrmir","tpwrmirf");
      irduration = mir_rf.rfDuration+2*rofmir;
      if (tspoilmir > 0.0) {
        init_generic(&mirspoil_grad,"mirspoil",gspoilmir,tspoilmir);
        calc_generic(&mirspoil_grad,WRITE,"","");
        irduration += mirspoil_grad.duration;
      }
      putCmd("irduration = %f",irduration);
    }

    /* In-plane saturation */
    ipsTime = 0.0;
    if (ips[0] == 'y') {
      if (ipsplan[0] == 'n') {
        ipsthk=10*(maxpss-minpss)+thk+ipsaddthk; /* maxpss,minpss in cm */
        ipspos=minpss+(maxpss-minpss)/2.0;
        ipspsi=psi; ipsphi=phi; ipstheta=theta;
      }
      if (wetips[0] == 'y') nips=4;
      if ((tabvals=(int *)malloc(nips*sizeof(int))) == NULL) nomem();
      if (wetips[0] == 'y') {
        flipips=flipipsf*161.0;
        shape_rf(&ips_rf,"pips",ipspat,pips,flipips,rof1,rof2);
        calc_rf(&ips_rf,"tpwrips","tpwripsf"); 
        tabvals[0] = (int)ROUND(fpwrscale*ips_rf.powerFine*81.4/flipips);
        tabvals[1] = (int)ROUND(fpwrscale*ips_rf.powerFine*101.4/flipips);
        tabvals[2] = (int)ROUND(fpwrscale*ips_rf.powerFine*69.3/flipips);
        tabvals[3] = (int)ROUND(fpwrscale*ips_rf.powerFine);
      } else {
        shape_rf(&ips_rf,"pips",ipspat,pips,flipips,rof1,rof2); 
        calc_rf(&ips_rf,"tpwrips","tpwripsf");
        for (i=0;i<nips;i++) tabvals[i] = (int)ROUND(fpwrscale*ips_rf.powerFine);
      }
      /* Use table t58 for IPS fine powers */
      settable(t58,nips,tabvals);
      trisesave=trise;
      if (!trisesqrt3 && FP_EQ(trampfixed,0.0)) {
        if (remainder(ipspsi,90.0) || remainder(ipsphi,90.0) || remainder(ipstheta,90.0))
          trise = trise*sqrt(3.0);
      }
      init_slice(&ips_grad,"ips",ipsthk);
      calc_slice(&ips_grad,&ips_rf,WRITE,"");
      if (tspoilips > 0.0) {
        init_generic(&ipsspoil_grad,"ipsspoil",gspoilips,tspoilips);
        calc_generic(&ipsspoil_grad,WRITE,"","");
        ipsgamp = ipsspoil_grad.amp/pow(2.0,nips-1);
        for (i=0;i<nips;i++) tabvals[i] = (int)pow(2.0,nips-1-i);
        /* Use table t57 for IPS spoil gradients */
        settable(t57,nips,tabvals);
      }
      free(tabvals);
      trise=trisesave;
      pssIps[0]=ipspos;
      offsetlist(pssIps,ips_grad.ssamp,0,freqIps,1,'c');
      shapeIps = shapelist(ips_rf.pulseName,ips_grad.rfDuration,freqIps,1,ips_grad.rfFraction,'c');
      ipsTime = nips*ips_grad.duration+GRADIENT_RES;
      if (tspoilips > 0.0) ipsTime +=nips*ipsspoil_grad.duration;
      putvalue("ipstime",ipsTime);
      strcat(aslteststring,"/IPS");
      putCmd("asltestpars[%d] = 'IPS'",asltestval);
      putCmd("asltesttag[%d] = 1",asltestval++);
      /* Hijack IR's vnirpulses (vnips=vnirpulses) for number of IPS pulses */
      F_initval((double)nips,vnips);
    }

    /* Q2TIPS */
    q2Time=0.0;
    if (q2tips[0] == 'y') {
      shape_rf(&q2_rf,"pq2",q2pat,pq2,flipq2,rof1,rof2); 
      calc_rf(&q2_rf,"tpwrq2","tpwrq2f"); 
      switch (asltype) {
        case STAR:
          q2plan[0]='n';
          break;
        case PICORE:
          q2plan[0]='n';
          break;
      }
      if (q2plan[0] == 'n') {
        switch (asltag) {
          case 1: /* Tag */
            q2psi=aslpsi; q2phi=aslphi; q2theta=asltheta;
            if (asltagrev[0] == 'y') q2pos=aslpos-0.05*(aslthk-q2thk);
            else q2pos=aslpos+0.05*(aslthk-q2thk);
            break;
          case -1: /* Control */
            q2psi=aslctrlpsi; q2phi=aslctrlphi; q2theta=aslctrltheta;
            if (asltype==FAIR) q2pos=aslpos+0.05*(aslthk-q2thk);
            else if (stardoubletag || asltype==PICORE) {
              if (asltagrev[0] == 'y') q2pos=aslctrlpos-0.05*(aslthk-q2thk);
              else q2pos=aslctrlpos+0.05*(aslthk-q2thk);
            } else {
              if (asltagrev[0] == 'y') q2pos=aslctrlpos+0.05*(aslthk-q2thk);
              else q2pos=aslctrlpos-0.05*(aslthk-q2thk);
            }
            break;
        }
      }
      trisesave=trise;
      if (!trisesqrt3 && FP_EQ(trampfixed,0.0)) {
        if (remainder(q2psi,90.0) || remainder(q2phi,90.0) || remainder(q2theta,90.0))
          trise = trise*sqrt(3.0);
      }
      init_slice(&q2_grad,"q2",q2thk);
      calc_slice(&q2_grad,&q2_rf,WRITE,"");
      if (tspoilq2 > 0.0) {
        init_generic(&q2spoil_grad,"q2spoil",gspoilq2,tspoilq2);
        calc_generic(&q2spoil_grad,WRITE,"","");
      }
      trise=trisesave;
      pssQ2[0]=q2pos;
      offsetlist(pssQ2,q2_grad.ssamp,0,freqQ2,1,'c');		
      shapeQ2 = shapelist(q2_rf.pulseName,q2_grad.rfDuration,freqQ2,1,q2_grad.rfFraction,'c');
      q2Time = nq2*q2_grad.duration+GRADIENT_RES;
      if (tspoilq2 > 0.0) q2Time += nq2*q2spoil_grad.duration;
      putvalue("q2time",q2Time);
      strcat(aslteststring,"/TagQ/CtrlQ");
      putCmd("asltestpars[%d] = 'TagQ'",asltestval);
      putCmd("asltesttag[%d] = 1",asltestval++);
      putCmd("asltestpars[%d] = 'CtrlQ'",asltestval);
      putCmd("asltesttag[%d] = -1",asltestval++);
      /* Hijack IR's vir (vnq2=vir) for number of Q2TIPS pulses */
      F_initval(nq2,vnq2);
    }

    /* Vascular Suppression */
    if (vascsup[0] == 'y' ) create_vascular_suppress();

    /* Return asl test string to parameter set */
    putCmd("aslteststring = '%s'",aslteststring);

    /* Hijack IR's vslice_ctr and vslices to count slices */
    assign(zero,vslice_ctr);
    F_initval((double)ns,vslices);

  }

}


/***********************************************************************
*  Function Name: calc_aslTime
*  Example:    trmin = calc_aslTime(tauasl,trmin,&trtype);
*  Purpose:    Calculates the time for all ASL components:
*              Calculates the minimum time for all ASL components
*              tauasl and trmin are required for the calculation
*              trmin is returned so tr_delay can be calculated
*              trtype is set if appropriate (trtype=1 for non distributed tr)
*
***********************************************************************/
double calc_aslTime(double asladd,double trepmin,int *treptype)
{
  /* The ASL module can not be used with the IR module */
  /* The ASL module uses the IR module real time variables, real time variables v41,v42 and real time tables t57-t60 */

  int stardoubletag=FALSE;
  double asltimin;        /* minimum ASL inflow time */
  double q2timin;         /* minimum ASL inflow time to Q2TIPS*/
  double trepmininput;    /* copy of input trepmin */
  double mirTime;         /* the duration over which MIR pulses should operate */
  double eventtime;       /* the duration of events in mirTime */
  double taupreir;        /* duration of components up to pulse centre */
  double taupostir;       /* duration of components after pulse centre */
  double mirt1[MAXMIR];   /* the T1s of static background */
  double irtime[MAXMIR];  /* the timings of MIR pulses */
  struct stat buf;        /* file info */
  char timefile[MAX_STR]; /* name of aslmirtime file */
  char timecmd[MAX_STR];  /* command to run aslmirtime */
  int i;
  FILE *fp;

  /* Use more meaningful names for v41,v42 */
  int vnmir     = v42;          /* Number of MIR pulses (before Q2TIPS if q2tips='y') */
  int vnmirq2   = v41;          /* Number of MIR pulses after Q2TIPS (if q2tips='y') */

  if ((ir[0] == 'y') || (asl[0] != 'y')) return(0.0);

  /* In compressed multislice mode STAR requires a double tag control */
  switch (asltype) {
    case STAR:
      if (!strcmp(starctrl,"doubletag")) stardoubletag=TRUE;
      break;
    default:
      break;
  }

  /* Check minimum ASL inflow time (TI) */
  asltimin=asladd;
  switch (asltype) {
    case CASL:
      asltimin += asl_grad.rfDelayBack+rof1;
      break;
    default:
      asltimin += asl_grad.rfCenterBack;
      break;
  }
  if (tspoilasl > 0.0) asltimin += aslspoil_grad.duration;
  if (ips[0] == 'y') asltimin += ipsTime;
  if (q2tips[0] == 'y') {
    /* Check minimum q2ti, the inflow time to Q2TIPS */
    /* NB. Q2TIPS option not available for CASL */
    q2timin = asl_grad.rfCenterBack+ipsTime+GRADIENT_RES+q2_grad.rfCenterFront;
    if (tspoilasl > 0.0) q2timin += aslspoil_grad.duration;
    if (minq2ti[0] == 'y') {
      q2ti = q2timin;
      putvalue("q2ti",q2ti);
    }
    if (FP_LT(q2ti,q2timin))
      abort_message("Q2TIPS Start time (q2ti) too short, minimum is %.3f ms\n",q2timin*1000);
    q2ti_delay = q2ti-q2timin;
    asltimin += q2Time + q2ti_delay; /* add Q2TIPS component to minimum inflow time */
  }
  if (minaslti[0] == 'y') {
    aslti = asltimin;
    putvalue("aslti",aslti);
  }
  if (FP_LT(aslti,asltimin))
    abort_message("ASL inflow time TI (aslti) too short, minimum is %.3f ms\n",asltimin*1000);

  /* Calculate the appropriate inflow delay */
  aslti_delay = aslti-asltimin;

  /* Calculate ASL duration */
  aslTime = psTime+GRADIENT_RES+asl_grad.duration+ipsTime+q2ti_delay+q2Time+aslti_delay;
  if (tspoilasl > 0.0) aslTime += aslspoil_grad.duration;
  if (stardoubletag) {
    aslTime += asl_grad.duration;
    if (tspoilasl > 0.0) aslTime += aslspoil_grad.duration;
  }

  /* Now that other ASL components are calculated figure/check MIR pulse timings */ 
  if (mir[0] == 'y') {
    /* Set up mirTime, the duration over which MIR pulses should operate */
    mirTime = aslti;
    if (ips[0] == 'y') {
      /* Assume signal is nulled at the end of IPS component */
      mirTime -= ipsTime;
      if (tspoilasl > 0.0) mirTime -= aslspoil_grad.duration;
      switch (asltype) {
        case CASL:
          mirTime -= asl_grad.rfDelayBack+rof1;
          break;
        default:
          mirTime -= asl_grad.rfCenterBack;
          break;
      }
    } else {
      /* Assume signal is nulled at the end of PS component */
      mirTime = aslti+asl_grad.rfCenterFront+GRADIENT_RES;
      if (stardoubletag) {
        mirTime += asl_grad.duration;
        if (tspoilasl > 0.0) mirTime += aslspoil_grad.duration;
      }
    }
    putvalue("mirtime",mirTime);

    /* If size of irtime is not correct, automatically calculate the times */
    P_getVarInfo(CURRENT,"irtime",&dvarinfo);
    if (FP_NEQ((double)nmir,dvarinfo.size)) {
      autoirtime[0]='y';
      putCmd("autoirtime = 'y'");
    }

    /* If auto calculation is requested use the aslmirtime binary to generate the 
       timings with a Broyden-Fletcher-Goldfarb-Shanno (BFGS) minimization algorithm  
       aslmirtime uses the BFGS minimization algorithm in the GNU Scientific Library (GSL).
       The GSL is free software published under the GNU General Public License (GPL).
       aslmirtime is published under the GPL at /vnmr/imaging/src.
    */
    /* Only perform auto calculation for first array element */
    if (autoirtime[0]=='y' && ix==1) {
      /* Figure the number of T1s and get their values */
      P_getVarInfo(CURRENT,"mirt1",&dvarinfo);
      getarray("mirt1",mirt1);
      /* Check the T1 values are not 0.0 */
      if (FP_EQ(dvarinfo.size,1.0) && FP_EQ(mirt1[0],0.0))
        abort_message("calc_aslTime MIR error: mirt1[1] is 0.0. Add appropriate T1 values\n");
      for (i=1;i<(int)dvarinfo.size;i++)
        if (FP_EQ(mirt1[i],0.0))
          abort_message("calc_aslTime MIR error: mirt1[%d] is 0.0. Reset T1 values and add appropriate T1 values\n",i+1);
      /* Check that the aslmirtime binary is present */
      sprintf(timefile,"/tmp/aslmirtimestatus_%s",getenv("USER"));
      sprintf(timecmd,"which aslmirtime > %s",timefile);
      system(timecmd);
      if (stat(timefile,&buf) == -1)
        abort_message("calc_aslTime MIR error: Unable to access file /tmp/aslmirtimestatus\n");
      if (buf.st_size == 0)
        abort_message("calc_aslTime MIR error: aslmirtime binary appears to be missing\n");
      /* If there's no abort_message the aslmirtime binary is present */
      /* Set system call to calculate MIR times */
      sprintf(timecmd,"aslmirtime -n %d -d %f -t %d",nmir,mirTime,(int)dvarinfo.size);
      for (i=0;i<(int)dvarinfo.size;i++) sprintf(timecmd,"%s %f",timecmd,mirt1[i]);
      /* Execute the system call */
      system(timecmd);
      sprintf(timefile,"/tmp/aslmirtime_%s.txt",getenv("USER"));
      /* Open timefile for reading */
      if ((fp=fopen(timefile,"r")) == NULL)
        abort_message("calc_aslTime MIR error: Unable to open %s\n",timefile);
      for (i=0;i<nmir;i++) {
        if (fscanf(fp,"%lf",&irtime[i]) != 1)
          abort_message("calc_aslTime MIR error: Problem reading %s\n",timefile);
      }
      /* Write irtime to parameter set */
      putCmd("irtime = 0");   /* re-initialize irtime */
      for (i=0;i<nmir;i++) putCmd("irtime[%d] = %f",i+1,irtime[i]);
    } else { /* otherwise just get the values from irtime */
      getarray("irtime",irtime);
    }

    /* Calculate mir delays */
    /* Set up eventtime, the duration of events to the first mir delay */
    eventtime = 0.0;     /* For IPS there are no events to MIR pulses */
    if (ips[0] != 'y') { /* Must be PS */
      eventtime = asl_grad.duration;
      if (tspoilasl > 0.0) eventtime += aslspoil_grad.duration;
      if (stardoubletag) eventtime *=2;
      eventtime += GRADIENT_RES;
      if (!aslphaseramp) eventtime += GRADIENT_RES;
    }
    /* Set up taupreir and taupostir, the duration of components before and after pulse centre */
    taupreir = rofmir+(1-mir_rf.header.rfFraction)*mir_rf.rfDuration;
    taupostir = mir_rf.header.rfFraction*mir_rf.rfDuration+rofmir;
    if (tspoilmir > 0.0) taupostir += mirspoil_grad.duration;
    /* Calculate the mir_delay times */
    for (i=0;i<nmir;i++) {
      mir_delay[i] = granularity(irtime[i]-eventtime-taupreir,GRADIENT_RES);
      eventtime += mir_delay[i]+irduration;
      if (FP_LT(mir_delay[i],GRADIENT_RES))
        abort_message("calc_aslTime MIR error: Delay before MIR pulse %d is negative. Increase inflow time or reduce number of MIR pulses\n",i+1);
      mir_delay[i] -= GRADIENT_RES;
    }

    /* Calculate the appropriate inflow delay */
    aslti_delay = mirTime-eventtime-asladd;
    if (FP_LT(aslti_delay,0.0))
      abort_message("calc_aslTime MIR error: Delay after last MIR pulse is negative. Increase inflow time or reduce number of MIR pulses\n");

    /* If Q2TIPS is prescribed, check there is no conflict and adjust timings appropriately */
    nmirq2=0; /* # MIR pulses after Q2TIPS is 0 if there is no Q2TIPS */
    if (q2tips[0] == 'y') {
      nmirq2=nmir;
      for (i=0;i<nmir;i++) {
        eventtime = mir_delay[i]+irduration;
        if (eventtime<q2ti_delay) {
          nmirq2--;
          q2ti_delay -= eventtime;
        } else {
          mir_delay[i]-=q2ti_delay+q2Time;
          if (FP_LT(mir_delay[i],0.0))
            abort_message("calc_aslTime MIR error: Conflict with MIR pulse %d timing (%.3f ms) and Q2TIPS \n",i+1,1e3*irtime[i]);
          break;
        }
      }
    }

    /* For Q2TIPS adjust nmir to correspond to the number of MIR pulses before Q2TIPS */
    nmir -= nmirq2;

    /* If create_delay_list is used for variable delay list use a single list for all array elements */
    /* if (ix==1) delayMir=create_delay_list(mir_delay,nmir+nmirq2); */

    /* Initialize real time variables for the number of MIR pulses */
    F_initval((double)nmir,vnmir);
    if (nmirq2>0) F_initval((double)nmirq2,vnmirq2);
  }

  /* Take copy of input trepmin */
  trepmininput=trepmin;

  /* Calculate ASL tr delay */
  asltr_delay = 0.0;
  if (ns > 1 ) {
    trepmin/=ns; /* trepmin (usually trmin) for 1 slice */
    if (minslicetr[0] == 'y') {
      slicetr = trepmin;
      putvalue("slicetr",slicetr);
    }
    if (FP_LT(slicetr,trepmin))
      abort_message("Slice TR (slicetr) too short, minimum is %.3f ms\n",trepmin*1000);
    asltr_delay = slicetr-trepmin;
    trepmin = slicetr*ns;
    *treptype = 1; /* force non distributed tr */
  }

  /* Add aslTime to trepmin */
  trepmin += aslTime;

  /* Return additional components to trepmin (usually trmin) */
  return(trepmin-trepmininput);

}


/***********************************************************************
*  Function Name: arterial_spin_label
*  Example:    arterial_spin_label();
*  Purpose:    Plays out arterial spin labelling (ASL) components
*
***********************************************************************/
void arterial_spin_label()
{
  /* The ASL module can not be used with the IR module */
  /* The ASL module uses the IR module real time variables, real time variables v41,v42 and real time tables t57-t60 */

  int stardoubletag=FALSE;
  int caslnorfcontrol=FALSE;
  int i;

  /* The following real time variables reserved for IR are used in ASL: *
   * vslice_ctr    Slice counter                                        *
   * vslices       Total number of slices                               *
   * virblock      Number of slices per inversion recovery block        *
   * vnirpulses    Number of ir pulses first pass through ir block      *
   * vir           Flag to prescribe inversion pulses                   *
   * virslice_ctr  Inversion slice counter                              *
   * vnir          Number of ir pulses in loop (variable)               *
   * vnir_ctr      ir pulse loop counter                                */
  /* Rename some to be more meaningful */
  int vnps      = virblock;     /* Number of PS pulses */
  int vnips     = vnirpulses;   /* Number of IPS pulses */
  int vnq2      = vir;          /* Number of Q2TIPS pulses */
  int vspoil    = virslice_ctr; /* Gradient spoil multiplier */
  int vloop_ctr = vnir;         /* Loop counter */
  int vpwrf     = vnir_ctr;     /* Fine power */
  /* Use more meaningful names for v41,v42 */
  int vnmir     = v42;          /* Number of MIR pulses (before Q2TIPS if q2tips='y') */
  int vnmirq2   = v41;          /* Number of MIR pulses after Q2TIPS (if q2tips='y') */

  if (asl[0] == 'y') {

    /* Can't implement IR and ASL */
    if (ir[0] == 'y') return;

    /* CASL with dedicated tagging coil may use a 'no RF' control */
    /* Compressed multislice mode STAR requires a double tag control */
    switch (asltype) {
      case CASL:
        if ((asltagcoil[0] == 'y') && !strcmp(caslctrl,"norf")) caslnorfcontrol=TRUE;
        break;
      case STAR:
        if (!strcmp(starctrl,"doubletag")) stardoubletag=TRUE;
        break;
      default:
        break;
    }

    delay(asltr_delay);

    ifzero(vslice_ctr); /* If first slice play out ASL components */

      /* Presaturation */
      if (ps[0] == 'y') {
        set_rotation_matrix(pspsi,psphi,pstheta);
        obspower(ps_rf.powerCoarse);
        delay(GRADIENT_RES);
// Need to set vobspwrfstepsize(1.0) for DD2 here
// if (DD2) vobspwrfstepsize(1.0);
        loop(vnps,vloop_ctr);
          getelem(t60,vloop_ctr,vpwrf);
          getelem(t59,vloop_ctr,vspoil);
          vobspwrf(vpwrf);
          obl_shapedgradient(ps_grad.name,ps_grad.duration,0.0,0.0,ps_grad.ssamp,NOWAIT);
          delay(ps_grad.rfDelayFront);
          shapedpulselist(shapePs,ps_grad.rfDuration,oph,rof1,rof1,'i',0);
          delay(ps_grad.rfDelayBack);
          if (tspoilps > 0.0)
            var3_shapedgradient(psspoil_grad.name,psspoil_grad.duration,0.0,0.0,0.0,psgamp,psgamp,psgamp,vspoil,vspoil,vspoil,WAIT);
        endloop(vloop_ctr);
      }

      /* Set rotation for tag or control pulse */
      switch (asltag) {
        case -1: /* Control */
          set_rotation_matrix(aslctrlpsi,aslctrlphi,aslctrltheta);
          break;
        default:
          set_rotation_matrix(aslpsi,aslphi,asltheta);
          break;
      }

      /* Set power, offset and PDD switching for asltagcoil tag or control pulse if required */
      if (asltagcoil[0] == 'y') {
        /* Systems with 3rd channel may be configured for transmit sense, in which case 
           there may just be one frequency synthesizer that is used for channel 1 and 
           channel 3 which means we are already properly at resto with obsoffset(resto).
           This config requires system global rfGroupMap='10111' rather than rfGroupMap='HLHHH'
           If there is a 3rd channel frequency synthesizer then we need to set with dec2offset.
           So set dec2offset in any case. NB dof2=resto is set in aslset. 
        */
        dec2offset(resto);
        /* The coil config can be for three different coils, volume coil for XMT, 
           surface coil array for RCV and dedicated tag coil for CASL. 
           A 3 channel PDD is required with a channel dedicated to each coil.
           3 spare line outs were used to switch the PDD as required. 
           Normally sp3on/sp3off is used to tune/detune the XMT coil and RCV coils, 
           but in this experiment that does not work because when the tag coil is tuned
           for CASL both XMT and RCV coils need to be detuned.
           The default is that at rest the volume coil is on resonance.
        */
        asl_xmtoff();    /* switch regular transmit volume coil off-resonance */
        asl_tagcoilon(); /* switch tag coil on-resonance */
                         /* if (volumercv[0] == 'n') asl_xmtoff()/asl_xmton() will also be required about acquisition */
        switch (asltag) {
          case 1: /* Tag */
            if (!aslphaseramp) {
              obsoffset(resto+freqAsl[0]);
              dec2offset(resto+freqAsl[0]);
            }
            dec2power(asl_rf.powerCoarse);
            dec2pwrf(asl_rf.powerFine);
            break;
          case -1: /* Control */
            if (!aslphaseramp) {
              obsoffset(resto+freqAslCtrl[0]);
              dec2offset(resto+freqAslCtrl[0]);
            }
            dec2power(aslctrl_rf.powerCoarse);
            dec2pwrf(aslctrl_rf.powerFine);
            break;
        }
      } else {
        switch (asltag) {
          case 1: /* Tag */
            if (!aslphaseramp) obsoffset(resto+freqAsl[0]);
            obspower(asl_rf.powerCoarse);
            obspwrf(asl_rf.powerFine);
            break;
          case -1: /* Control */
            if (!aslphaseramp) obsoffset(resto+freqAslCtrl[0]);
            obspower(aslctrl_rf.powerCoarse);
            obspwrf(aslctrl_rf.powerFine);
            break;
        }
      }
      delay(GRADIENT_RES);

      /* Apply additional STAR tag if required */
      if (stardoubletag) {
        switch (asltag) {
          case -1: /* Control */
            obl_shapedgradient(asl_grad.name,asl_grad.duration,0.0,0.0,asltaggamp,NOWAIT);
            delay(asl_grad.rfDelayFront);
            shapedpulselist(shapeAslCtrl,asl_grad.rfDuration,oph,rof1,rof1,'s',zero);
            delay(asl_grad.rfDelayBack);
            if (tspoilasl > 0.0)
              obl_shapedgradient(aslspoil_grad.name,aslspoil_grad.duration,0.0,0.0,aslspoil_grad.amp,WAIT);
            break;
          default: /* Otherwise */
            delay(asl_grad.duration);
            if (tspoilasl > 0.0) delay(aslspoil_grad.duration);
            break;
          }
      }

      /* Apply tag or control pulse */
      switch (asltype) {
        case CASL:
          switch (asltag) {
            case 1: /* Tag */
              obl_shapedgradient(asl_grad.name,asl_grad.duration,0.0,0.0,asltaggamp,NOWAIT);
              delay(asl_grad.rfDelayFront);
              if (asltagcoil[0] == 'y') {
                if (aslphaseramp) dec2shapedpulselist(shapeAsl,asl_grad.rfDuration,oph,rof1,rof1,'s',zero);
                else dec2shaped_pulse(asl_rf.pulseName,asl_grad.rfDuration,oph,rof1,rof1);
              } else {
                if (aslphaseramp) shapedpulselist(shapeAsl,asl_grad.rfDuration,oph,rof1,rof1,'s',zero);
                else rgpulse(asl_grad.rfDuration,oph,rof1,rof1);
              }
              delay(asl_grad.rfDelayBack);
              break;
            case -1: /* Control */
              obl_shapedgradient(asl_grad.name,asl_grad.duration,0.0,0.0,asltaggamp,NOWAIT);
              if (caslnorfcontrol) delay(asl_grad.duration);
              else {
                delay(asl_grad.rfDelayFront);
                if (asltagcoil[0] == 'y') {
                  if (aslphaseramp) dec2shapedpulselist(shapeAslCtrl,asl_grad.rfDuration,oph,rof1,rof1,'s',zero);
                  else dec2shaped_pulse(aslctrl_rf.pulseName,asl_grad.rfDuration,oph,rof1,rof1);
                } else {
                  if (aslphaseramp) shapedpulselist(shapeAslCtrl,asl_grad.rfDuration,oph,rof1,rof1,'s',zero);
                  else shaped_pulse(aslctrl_rf.pulseName,asl_grad.rfDuration,oph,rof1,rof1);
                }
                delay(asl_grad.rfDelayBack);
              }
              break;
            case 0: /* No pulse */
              obl_shapedgradient(asl_grad.name,asl_grad.duration,0.0,0.0,asltaggamp,NOWAIT);
              delay(asl_grad.duration);
              break;
          }
          /* Reset PDD switching to default state if need be */
          if (asltagcoil[0] == 'y') {
            asl_xmton();      /* switch regular transmit volume coil on-resonance */
            asl_tagcoiloff(); /* switch tag coil off-resonance */
                              /* if (volumercv[0] == 'n') asl_xmtoff()/asl_xmton() will also be required about acquisition */
          }
          break;
        default:
          switch (asltag) {
            case 1: /* Tag */
              obl_shapedgradient(asl_grad.name,asl_grad.duration,0.0,0.0,asltaggamp,NOWAIT);
              delay(asl_grad.rfDelayFront);
              shapedpulselist(shapeAsl,asl_grad.rfDuration,oph,rof1,rof1,'s',zero);
              delay(asl_grad.rfDelayBack);
              break;
            case -1: /* Control */
              obl_shapedgradient(asl_grad.name,asl_grad.duration,0.0,0.0,asltaggamp,NOWAIT);
              delay(asl_grad.rfDelayFront);
              shapedpulselist(shapeAslCtrl,asl_grad.rfDuration,oph,rof1,rof1,'s',zero);
              delay(asl_grad.rfDelayBack);
              break;
            case 0: /* No pulse */
              obl_shapedgradient(asl_grad.name,asl_grad.duration,0.0,0.0,asl_grad.ssamp,NOWAIT);
              delay(asl_grad.duration);
              break;
          }
          break;
      }

      if (tspoilasl > 0.0)
        obl_shapedgradient(aslspoil_grad.name,aslspoil_grad.duration,0.0,0.0,aslspoil_grad.amp,WAIT);

      if (!aslphaseramp) { 
        obsoffset(resto); 
        if (asltagcoil[0] == 'y') dec2offset(resto);
        delay(GRADIENT_RES); 
      }

      /* In Plane Saturation */
      if (ips[0] == 'y') {
        set_rotation_matrix(ipspsi,ipsphi,ipstheta);
        obspower(ips_rf.powerCoarse);
        delay(GRADIENT_RES);
// Need to set vobspwrfstepsize(1.0) for DD2 here
// if (DD2) vobspwrfstepsize(1.0);
        loop(vnips,vloop_ctr);
          getelem(t58,vloop_ctr,vpwrf);
          getelem(t57,vloop_ctr,vspoil);
          vobspwrf(vpwrf);
          obl_shapedgradient(ips_grad.name,ips_grad.duration,0.0,0.0,ips_grad.ssamp,NOWAIT);
          delay(ips_grad.rfDelayFront);
          shapedpulselist(shapeIps,ips_grad.rfDuration,oph,rof1,rof1,'c',zero);
          delay(ips_grad.rfDelayBack);
          if (tspoilips > 0.0)
            var3_shapedgradient(ipsspoil_grad.name,ipsspoil_grad.duration,0.0,0.0,0.0,ipsgamp,ipsgamp,ipsgamp,vspoil,vspoil,vspoil,WAIT);
        endloop(vloop_ctr);
      }

      /* MIR pulses */
      if (mir[0] == 'y' && nmir>0) {
        /* If create_delay_list is used for variable delays use loop/endloop and vdelay_list */
        /* loop(vnmir,vloop_ctr); */
        for (i=0;i<nmir;i++) {
          set_rotation_matrix(psi,phi,theta);
          obspower(mir_rf.powerCoarse);
          obspwrf(mir_rf.powerFine);
          delay(GRADIENT_RES);
          /* vdelay_list(delayMir,vloop_ctr); */
          delay(mir_delay[i]);
          shapedpulseoffset(mir_rf.pulseName,mir_rf.rfDuration,zero,rofmir,rofmir,0.0);
          if (tspoilmir > 0.0)
            obl_shapedgradient(mirspoil_grad.name,mirspoil_grad.duration,-mirspoil_grad.amp,0.0,-mirspoil_grad.amp,WAIT);
        }
        /* endloop(vloop_ctr); */
      }

      /* Q2TIPS */
      if (q2tips[0] == 'y') {
        delay(q2ti_delay);
        set_rotation_matrix(q2psi,q2phi,q2theta);
        obspower(q2_rf.powerCoarse);
        obspwrf(q2_rf.powerFine);
        delay(GRADIENT_RES);
        loop(vnq2,vloop_ctr);
          obl_shapedgradient(q2_grad.name,q2_grad.duration,0.0,0.0,q2_grad.amp,NOWAIT);
          delay(q2_grad.rfDelayBack);
          shapedpulselist(shapeQ2,q2_grad.rfDuration,oph,rof1,rof1,'c',zero);
          delay(q2_grad.rfDelayBack);
          if (tspoilq2 > 0.0)
            obl_shapedgradient(q2spoil_grad.name,q2spoil_grad.duration,q2spoil_grad.amp,q2spoil_grad.amp,q2spoil_grad.amp,WAIT);
        endloop(vloop_ctr);
        /* Apply any MIR pulses that are after Q2TIPS */
        if (mir[0] == 'y' && nmirq2>0) {
          /* If create_delay_list is used for variable delays use loop/endloop and vdelay_list */
          /* loop(vnmirq2,vloop_ctr); */
          int j;
          for (i=0;i<nmirq2;i++) {
            set_rotation_matrix(psi,phi,theta);
            obspower(mir_rf.powerCoarse);
            obspwrf(mir_rf.powerFine);
            delay(GRADIENT_RES);
            add(vloop_ctr,vnmir,vtest);
            /* vdelay_list(delayMir,vtest); */
            j=i+nmir;
            delay(mir_delay[j]);
            shapedpulseoffset(mir_rf.pulseName,mir_rf.rfDuration,zero,rofmir,rofmir,0.0);
            if (tspoilmir > 0.0)
              obl_shapedgradient(mirspoil_grad.name,mirspoil_grad.duration,-mirspoil_grad.amp,0.0,-mirspoil_grad.amp,WAIT);
          }
          /* endloop(vloop_ctr); */
        }
      }

      /* Return to standard imaging rotation */
      rotate();

      delay(aslti_delay);

    endif(vslice_ctr); /* endif for first slice */

    incr(vslice_ctr);                 /* increment the slice counter */
    ifrtEQ(vslice_ctr,vslices,vtest); /* if its the last slice */
      assign(zero,vslice_ctr);        /* zero the slice counter to play out ASL module next pass */
    endif(vtest);

  }
}


/***********************************************************************
*  Function Name: asl_xmton
*  Example:    asl_xmton();
*  Purpose:    Switches regular transmit coil on-resonance in dedicated 
*              ASL tagging coil configuration
*
***********************************************************************/
void asl_xmton()
{
  if ((asl[0] == 'y') && (asltagcoil[0] == 'y'))
    sp1off(); /* spare1 off switches volume coil on-resonance. Rapid PDD Auto Negative */
}


/***********************************************************************
*  Function Name: asl_xmtoff
*  Example:    asl_xmtoff();
*  Purpose:    Switches regular transmit coil off-resonance in dedicated 
*              ASL tagging coil configuration
*
***********************************************************************/
void asl_xmtoff()
{
  if ((asl[0] == 'y') && (asltagcoil[0] == 'y'))
    sp1on(); /* spare1 on switches volume coil off-resonance. Rapid PDD Auto Negative */
}


/***********************************************************************
*  Function Name: asl_tagcoilon
*  Example:    asl_tagcoilon();
*  Purpose:    Switches dedicated ASL tagging coil on-resonance in dedicated 
*              ASL tagging coil configuration
*
***********************************************************************/
void asl_tagcoilon()
{
  if ((asl[0] == 'y') && (asltagcoil[0] == 'y'))
    sp2on();  /* spare2 on switches tag coil on-resonance. Rapid PDD Auto Positive */
}


/***********************************************************************
*  Function Name: asl_tagcoiloff
*  Example:    asl_tagcoiloff();
*  Purpose:    Switches dedicated ASL tagging coil off-resonance in dedicated 
*              ASL tagging coil configuration
*
***********************************************************************/
void asl_tagcoiloff()
{
  if ((asl[0] == 'y') && (asltagcoil[0] == 'y'))
    sp2off(); /* spare2 off switches tag coil off-resonance. Rapid PDD Auto Positive */
}


/***********************************************************************
*  Function Name: create_vascular_suppress
*  Example:    create_vascular_suppress();
*  Purpose:    Creates vascular suppression (VS) components
*
***********************************************************************/
void create_vascular_suppress()
{
  vsTime=0.0;
  if (vascsup[0] == 'y' ) {
    init_generic(&vs_grad,"vs",gvs,tdeltavs);
    calc_generic(&vs_grad,NOWRITE,"","");
    /* adjust duration, so tdeltavs is from start ramp up to start ramp down */
    if ((ix==1) || sglarray) {
      vs_grad.duration += vs_grad.tramp; 
      calc_generic(&vs_grad,WRITE,"","");
    }
    vsTime=2*vs_grad.duration;
    putvalue("vstime",vsTime);
    bvalvs = bval(gvs,tdeltavs,tdeltavs); /* Assuming one direction only? otherwise, ... */
    putvalue("bvalvs",bvalvs);            /* bval(sqrt(3*gvs*gvs),tdeltavs,tdeltavs); */
  }
}


/***********************************************************************
*  Function Name: vascular_suppress
*  Example:    vascular_suppress();
*  Purpose:    Plays out vascular suppression (VS) components
*
***********************************************************************/
void vascular_suppress()
{
  if (vascsup[0] == 'y' ) {
    obl_shapedgradient(vs_grad.name,vs_grad.duration,vs_grad.amp,vs_grad.amp,vs_grad.amp,WAIT);
    obl_shapedgradient(vs_grad.name,vs_grad.duration,-vs_grad.amp,-vs_grad.amp,-vs_grad.amp,WAIT);
  }
}


/***********************************************************************
*  Function Name: create_tag
*  Example:    create_tag();
*  Purpose:    Create RF tagging
*  Input
*     Formal:  * tag_grad      - tagging gradient structure 
*              * tagcrush_grad - tagging crusher gradient 
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  
*     Private: none
*     Public:  none
*
*  Notes:      Method by Wu et al, MRM 2002, 48(2), 389-393  (Sinc-modulated)
***********************************************************************/
void create_tag()
{
	double  zeroCross;                /* number of sinc cycles */
	double  gtag;                     /* tagging gradient strength */
	int     i;                        /* loop counter */
	double  t1rf;                     /* RF tag temporal width */
	double  t2rf;                     /* RF tag center - center duration */
	double  ttag;                     /* tag duration */
	int     rfsum;
	int     ntag2;                    /* number of tags devided by two */
	double  gtime;                    /* gradient tag duration */
	
	tagtime = 0;
	
	if (tag[0] =='y' ) 
	{
		zeroCross = 4;                  /* number of sinc cycles */
		
		t1rf = ptag/zeroCross;          /* 4 = number of sinc cycles */
		gtag = 1/(GAMMA_H*t1rf*wtag);   /* Tagging gradient amplitude */
		t2rf = 1/(GAMMA_H*gtag*dtag);   /* temporal separation of RF pulses */
		
		ntag2 = (int) (ptag/2/t2rf);    /* number of pulses on either side of the center */
		ntag  = ntag2*2 + 1;            /* force total number of pulses to be odd */
		
		rfsum = 0;
		for (i=0; i<ntag; i++) 
		{
			ttag = (i-ntag2)*t2rf/(ptag/2)*2*PI;
			if ((i-ntag2) == 0) 
			{
				rfamp[i] = 4095;
			}
			else
			{
				rfamp[i] = (int) (sin(ttag)/ttag*4095);
			}
			rfsum += rfamp[i];
		}
	
		/*****************************************************/
		/* Calculate Taggign RF power ************************/
		tag_rf.flip = fliptag;                 /* assign flip angle for RF tagging pulse */
		tag_rf.rfDuration = pw;                /* assign duration for RF tagging pulse */
		strcpy(tag_rf.pulseName,tagpat);       /* assign RF pulse name for RF tagging pulse */
		calcPower(&tag_rf,rfcoil);             /* calculate MTC RF power */
		if (sgldisplay) 
		{
	 		printf("------------------------ RF TAGGGING -----------------------\n");
			displayRf(&tag_rf);
			printf("------------------------ END RF TAGGGING -----------------------\n");
		}
		if (tag_rf.error) 
	 	{
	 		abort_message("Gradient library RF error --> see text window for more information. \n");
		}
		/****************************************************/
		/* Normalize total RF integral to 4095 (fine power) */
		for (i=0; i<ntag; i++) 
		{
			rfamp[i] = (int) ((tag_rf.powerFine/(double)rfsum)*rfamp[i]);
		}
		gtime  = t2rf-pw-rof1-rof2;               /* Only apply gradient between RF pulses */
		/* Adjust for granulated gradient duration */
		gtime = granularity(gtime,GRADIENT_RES);  /* enforce granularity for gradient */
		t2rf = rof1 + rof2 + pw + gtime;          /* recalculate temporal separation of RF pulses */
		t1rf = t2rf * dtag/wtag;                  /* recalculate t1rf but keep relation betweeen 
												   spatial width and separation */
		gtag = 1/(GAMMA_H*t2rf*dtag);             /* recalculate gradient strength with new values */
		
		if (gtime < 0)
		{
			abort_message("pulse width too large, decrease spatial tag separation \n");
		}
		if (MAX_SLEW_RATE/gtime < gtag)
		{
			abort_message("Tagging gradient to short, decrease spatial tag separation\n");
		}

		/***********************************************/
		/* RF tagging gradient *************************/ 
		tag_grad.m0= gtag*t2rf;                  
		tag_grad.duration= gtime;
		tag_grad.calcFlag = AMPLITUDE_FROM_MOMENT_DURATION;  /* use moment and duration since all previous */
														   /* calculation assume rectangular gradient pulses*/
		calcGeneric(&tag_grad);
		displayGeneric(&tag_grad);
		if (tag_grad.amp > gmax)
		{
			abort_message("Tag gradient too large (%.2f, max is %.2f), reduce pw, decrease dtag or increase wtag",tag_grad.amp,gmax);
		}
		if (sgldisplay)
		{
			displayGeneric(&tag_grad);
		}
		if (tag_grad.error)
		{
			abort_message("Gradient library error tagging gradient --> see text window for more information. \n");     
		}
		
		/***********************************************/
		/* RF tag crusher gradient *********************/ 
		tagcrush_grad.amp=gcrushtag;
		tagcrush_grad.duration = tcrushtag;
		calcGeneric(&tagcrush_grad);
		if (sgldisplay)
		{
			displayGeneric(&tagcrush_grad);
		}
		if (tagcrush_grad.error)
		{
			abort_message("Gradient library error tagcrush_grad--> see text window for more information. \n");  
		}
		/********************************************************/
		/* Calculate tagging segment duration -using binary AND */ 
		if (tagdir & 1)
		{
			tagtime += ((ntag-1)*t2rf) + (rof1+rof2+pw);
		}
		if (tagdir & 2) 
		{
			tagtime += ((ntag-1)*t2rf) + (rof1+rof2+pw);
		}
		tagtime += (tagcrush_grad.duration);	 
		
		/* Export VNMRJ variables */
		putvalue("tagpwr", tag_rf.powerCoarse);
		putvalue("tagpwrf", tag_rf.powerFine);
		putvalue("tcrushtag", tagcrush_grad.duration);
		putvalue("gcrushtag", tagcrush_grad.amp);
	}
}   
  

/***********************************************************************
*  Function Name: tag_sinc
*  Example:    tag_sinc();
*  Purpose:    
*  Input
*     Formal:  none
*     Private: none
*     Public:  none
*  Output
*     Return:  none
*     Formal:  noe
*     Private: none
*     Public:  none
*  Notes:     Tagging direction uses binary evaluation (AND).
***********************************************************************/
void tag_sinc()
   {
   int i;            /* loop counter */
/*   int     rfamp[1024];*/


   if (tag[0]=='y')
      {
      /*******************************/
      /* Tagging readout direction ***/
      if (tagdir & 1) 
	 {
	 obspower(tag_rf.powerCoarse);	 
	 for (i=0; i<ntag; i++) 
            {
            if (rfamp[i] >= 0) 
	       {
               obspwrf(rfamp[i]);
               pulse(pw,one);
               }
            else 
	       {
               obspwrf(-rfamp[i]);
               pulse(pw,three);
               }
            obl_shapedgradient(tag_grad.name,tag_grad.duration,tag_grad.amp,0,0,WAIT);
            }
	 }

      /*******************************/
      /* Tagging phase direction *****/
      if (tagdir & 2) 
	 {
	 obspower(tag_rf.powerCoarse);
	 for (i=0; i<ntag; i++) 
            {
            if (rfamp[i] >= 0) 
	       {
               obspwrf(rfamp[i]);
               pulse(pw,one);
               }
            else 
	       {
               obspwrf(-rfamp[i]);
               pulse(pw,three);
               }
	    obl_shapedgradient(tag_grad.name,tag_grad.duration,0,tag_grad.amp,0,WAIT);
            }
	 }

      if (tag[0]=='y') 
	 {
	 obl_shapedgradient(tagcrush_grad.name,tagcrush_grad.duration,
             tagcrush_grad.amp,tagcrush_grad.amp,0,WAIT);
	 }
      }	 
   }


/***********************************************************************
*  Function Name: asl stuff
*  Example:    
*  Purpose:    
*  Notes:     
***********************************************************************/
void create_asl() {
  double pssir[MAXSLICE],pss_q2tips[MAXSLICE],
         ir_dist;
  double freqIR[MAXSLICE],freqQ[MAXSLICE];
  int    s;


  if ((asltype != FAIR) && (asltype != PICORE) && (asltype != STAR) && (asltype != TAGOFF))
    abort_message("create_asl: Unknown tag type");

  /* Calculate all RF and Gradient shapes */    
  shape_rf(&ir_rf,"pi",pipat,pi,flipir,rof1,rof1); 
  calc_rf(&ir_rf,"tpwri","tpwrif"); 

  init_slice(&ssi_grad,"aslssi",irthk);
  calc_slice(&ssi_grad,&ir_rf,WRITE,"");
  
  init_generic(&aslcrush_grad,"aslcrush",gcrushir,tcrushir);
  calc_generic(&aslcrush_grad,WRITE,"","");
  
  
  if (diff[0] == 'y') {
    init_generic(&diff_grad,"asldiff",gdiff,tdelta);
    diff_grad.maxGrad = gmax;
    calc_generic(&diff_grad,NOWRITE,"","");
    /* adjust duration, so tdelta is from start ramp up to start ramp down */
    if ((ix == 1) || (sglarray)) {
      diff_grad.duration += diff_grad.tramp; 
      calc_generic(&diff_grad,WRITE,"","");
    }
  }


  /* Set up list of phase ramped pulses */
  /* Create list of slice positions for IR pulse, based on tag type */
  ir_dist = thk/10/2 + irgap + irthk/10/2;  // thk & irthk in mm
  asl_ssiamp = ssi_grad.amp;  // keep in asl_ssiamp; ssi_grad is only calculated for ix==1
  for (s = 0; s < ns; s++) {
    switch (asltype) {
      case TAGOFF:
      case FAIR:   // IR on imaging slice, selective vs non-selective
                   pssir[s]  = pss[s];
		   pss_q2tips[s] = pss[s] + (thk/10/2 + irgap + satthk[0]/10/2);
		   /* quipss with FAIR doesn't make sense, but set it just in case */
		   if (asltag == -1) asl_ssiamp = 0;
                   break;
      case STAR:   // IR proximal vs distal to imaging slice 
                   if (asltag == 1){
		     pssir[s]      = pss[s] + ir_dist;
		   }
		   else if (asltag == -1) {
		     pssir[s]      = pss[s] - ir_dist;
		   }
		   else {
		     pssir[s]      = pss[s] + ir_dist;  // not used
		   }
                   break;
      case PICORE: // IR proximal to imaging slice, selective vs non-selective
                   pssir[s]      = pss[s] + ir_dist;
		   if (asltag == -1) asl_ssiamp = 0;
                   break;
      default:     break;
    }
  }
  offsetlist(pssir,asl_ssiamp,0,freqIR,ns,seqcon[1]);
  asl_shapeIR = shapelist(ir_rf.pulseName,ssi_grad.rfDuration,freqIR,ns,ssi_grad.rfFraction,seqcon[1]);
  

  /* Set up Q2TIPS RF and Gradients */
  if (quipss[0] == 'y') {
    shape_rf(&sat_rf,"psat",satpat,psat,flipsat,rof1,rof2); 
    calc_rf(&sat_rf,"tpwrsat","tpwrsatf"); 

    init_slice(&sat_grad,"q2tips",satthk[0]);
    calc_slice(&sat_grad,&sat_rf,WRITE,"");

    init_generic(&qcrush_grad,"qcrush",qgcrush,qtcrush);
    calc_generic(&qcrush_grad,WRITE,"","");

    for (s = 0; s < ns; s++) {
      if (asltag == 1){
        pss_q2tips[s] = pss[s] + (thk/10/2 + irgap + satthk[0]/10/2);
      }
      else if (asltag == -1) {
        pss_q2tips[s] = pss[s] - (thk/10/2 + irgap + satthk[0]/10/2);
      }
      else {
        pss_q2tips[s] = pss[s] + (thk/10/2 + irgap + satthk[0]/10/2);
      }
    }


    offsetlist(pss_q2tips,sat_grad.ssamp,0,freqQ,ns,seqcon[1]);
    asl_shapeQtag = shapelist(sat_rf.pulseName,sat_grad.rfDuration,freqQ,ns,sat_grad.rfFraction,seqcon[1]);
    
    quipssTime   = nquipss*(sat_grad.duration + qcrush_grad.duration);
  
  }


}

