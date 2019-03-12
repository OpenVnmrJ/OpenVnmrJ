/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef LINT
#endif
/* 
 */
/***************************************************************************
* $Source: /share/cvsimaging/CVS/vnmrsys/psglib/fssfp.c,v $
* $Revision: 1.4 $
* $Date: 2005/12/15 17:34:58 $
*
* Author: Michael L. Gyngell
*
*
***************************************************************************/

/*******
 *
 * An SSFP FID sequence "fssfp"
 *
 *******/

#include "sgl.c"

void pulsesequence()
{
	/* declaration of SGL kernel structures */
	SGL_KERNEL_INFO_T read, phase, slice, ss_pre, ss_post;


	/* declaration of internal variables */
	double freqlist[MAXNSLICE];
	double pe_steps;
	int shapelist1, table;
	double xtime, grad_duration, ror_pad,rod_pad;
	double temp_tr;

	double readAmp, phaseAmp, sliceAmp;
	double tepad, tepad2, temin2, htrmin, delayToRF, delayRFToAcq, delayAcqToRF;
	double rof_pad, delRof;

	double sliceRephTrim, sliceDephTrim;
	double readRephTrim, readDephTrim;

	int rfPhase[2] = {0,2};
	
	/* declaration of realtime variables */
	int  vpe_steps  = v1;
	int  vpe_ctr    = v2;
	int  vms_slices = v3;
	int  vms_ctr    = v4;
	int  vpe_offset = v5;
	int  vpe_index  = v6;
	int  vss        = v7;
	int  vssc       = v8;
	int  vacquire   = v9;
	int  vphase	= v10;
	
	settable(t2,2,rfPhase);

	/* setup phase encoding order */
	table = set_pe_order();

	init_mri();

	if( (sliceRephTrim = getvalnwarn("sliceRephTrim")) == 0.0 ) {
		sliceRephTrim = 1.0;
	}	
	
	if( (sliceDephTrim = getvalnwarn("sliceDephTrim")) == 0.0 ) {
		sliceDephTrim = 1.0;
	}	

	if( (readRephTrim = getvalnwarn("readRephTrim")) == 0.0 ) {
		readRephTrim = 1.0;
	}	
	
	if( (readDephTrim = getvalnwarn("readDephTrim")) == 0.0 ) {
		readDephTrim = 1.0;
	}	

	shape_rf( &p1_rf, "p1", p1pat, p1, flip1, rof1, rof2 );	// excitation pulse

	init_slice( &ss_grad, "ss", thk );					// slice gradient
	init_slice_refocus( &ssr_grad, "ssr" );				// slice refocus
	init_slice_refocus( &ssd_grad, "ssd" );				// slice refocus

	init_readout( &ro_grad, "ro", lro, np, sw );		// read gradient
	init_readout_refocus( &ror_grad, "ror" );			// read dephase
	init_readout_refocus( &rod_grad, "ror" );			// read dephase

	init_phase( &pe_grad, "pe", lpe, nv );				// phase gradient

	ss_grad.maxGrad = gmax * 0.57;
	ssr_grad.maxGrad = gmax * 0.57;
	ssd_grad.maxGrad = gmax * 0.57;
	ro_grad.maxGrad = gmax * 0.57;
	ror_grad.maxGrad = gmax * 0.57;
	rod_grad.maxGrad = gmax * 0.57;
	pe_grad.maxGrad = glimpe < 0.57? gmax*glimpe : gmax * 0.57;

	/* calculate the RF pulses, gradient pulses and their interdependencies */
	calc_rf( &p1_rf, "tpwr1", "tpwr1f" );
	calc_slice( &ss_grad, &p1_rf, NOWRITE, "gss" );

	ssr_grad.amp = ss_grad.amp;	
	ssr_grad.gmult = sliceRephTrim;
	ssr_grad.calcFlag = DURATION_FROM_MOMENT_AMPLITUDE;
	calc_slice_refocus( &ssr_grad, &ss_grad, NOWRITE, "gssr" );
	ssd_grad.amp = ss_grad.amp;	
	ssd_grad.gmult = sliceDephTrim; 
	ssd_grad.calcFlag = DURATION_FROM_MOMENT_AMPLITUDE;
	calc_slice_dephase( &ssd_grad, &ss_grad, NOWRITE, "gssd" ); 
	
	calc_readout( &ro_grad, NOWRITE, "gro", "sw", "at" );

	ror_grad.amp = ro_grad.amp;	
	ror_grad.calcFlag = DURATION_FROM_MOMENT_AMPLITUDE;

	rod_grad.amp = ro_grad.amp;	
	rod_grad.calcFlag = DURATION_FROM_MOMENT_AMPLITUDE;

	ror_grad.gmult = readRephTrim;
	calc_readout_refocus( &ror_grad, &ro_grad, NOWRITE, "gror" );
	rod_grad.gmult = readDephTrim;
	calc_readout_rephase( &rod_grad, &ro_grad, NOWRITE, "grod" );

	calc_phase( &pe_grad, NOWRITE, "gpe", "tpe" );

	/* work out the position of the markers */
	/* markerA */
	/* ss_grad.rfDelayFront indicates the starting point of the
	   RF pulse measured from the start of the slice gradient
       ( rof1:pulse length:rof2 ) */	

	double granulatedRFDelayFront = granularity( ss_grad.rfDelayFront, GRADIENT_RES );
	if( granulatedRFDelayFront > ss_grad.rfDelayFront ) {
		granulatedRFDelayFront -= GRADIENT_RES;
	}

	/* ss_grad.rfDelayBack indicates the end point of the
	   RF pulse measured to the end of the slice gradient
       ( rof1:pulse length:rof2 ) */	

	double granulatedRFDelayBack = granularity( ss_grad.rfDelayBack, GRADIENT_RES );
	if( granulatedRFDelayBack > ss_grad.rfDelayBack ) {
		granulatedRFDelayBack -= GRADIENT_RES;
	}
	
	double granulatedRFDelay = granulatedRFDelayFront < granulatedRFDelayBack ? granulatedRFDelayFront : granulatedRFDelayBack;

	double markerADelay = granulatedRFDelay;

	/* read and phase gradients can overlap the start or end of the slice gradient by max of granulatedRFDElay */

	double granulatedATDelayFront = granularity(ro_grad.atDelayFront, GRADIENT_RES);
	if( granulatedATDelayFront > ro_grad.atDelayFront ) {
		granulatedATDelayFront -= GRADIENT_RES;
	}
	double granulatedATDelayBack = granularity(ro_grad.atDelayBack, GRADIENT_RES);
	if( granulatedATDelayBack > ro_grad.atDelayBack ) {
		granulatedATDelayBack -= GRADIENT_RES;
	}
	double granulatedATDelay = granulatedATDelayFront < granulatedATDelayBack ? granulatedATDelayFront : granulatedATDelayBack;

	/* longest gradient between RF pulse and acquire dominates */

	xtime = ssr_grad.duration + granulatedRFDelay;
	xtime = xtime > ssd_grad.duration + granulatedRFDelay ? xtime : ssd_grad.duration + granulatedRFDelay;
	xtime = xtime > ror_grad.duration + granulatedATDelay ? xtime : ror_grad.duration + granulatedATDelay;
	xtime = xtime > rod_grad.duration + granulatedATDelay ? xtime : rod_grad.duration + granulatedATDelay;
	xtime = xtime > pe_grad.duration ? xtime : pe_grad.duration;

	ror_pad = xtime - ror_grad.duration - granulatedATDelay;
	rod_pad = xtime - rod_grad.duration - granulatedATDelay;

	/* make a gradient list */
	start_kernel( &sk );
	add_gradient( (void*)&ss_grad,  "slice",    	SLICE, START_TIME,	"",         0.0,	PRESERVE );
	add_gradient( (void*)&ssr_grad, "sliceReph", 	SLICE, BEHIND,		"slice",    0.0,	INVERT );
	add_gradient( (void*)&ror_grad, "readDeph", 	READ,  BEHIND,		"slice",   -granulatedRFDelay + ror_pad, INVERT );
	add_gradient( (void*)&ro_grad,  "read",     	READ,  BEHIND,		"readDeph", 0.0,	PRESERVE );	
	add_gradient( (void*)&pe_grad,  "phase",    	PHASE, SAME_START,	"readDeph", 0.0,	PRESERVE );
	add_gradient( (void*)&rod_grad, "readReph", 	READ,  BEHIND,		"read",     0.0,	INVERT );
	add_gradient( (void*)&pe_grad,  "rewind",		PHASE, SAME_END,	"readReph", 0.0, INVERT );
	add_gradient( (void*)&ss_grad,	"nextSlice",	SLICE, BEHIND,		"readReph", rod_pad - granulatedRFDelay, PRESERVE );
	add_gradient( (void*)&ssd_grad,	"sliceDeph",	SLICE, BEFORE,		"nextSlice",    0, INVERT );

	add_marker( "markerA", SAME_START, "slice", granulatedRFDelay );
	add_marker( "markerB", SAME_START, "nextSlice", granulatedRFDelay );

	/* get the minimum echo time */
	temin = get_timing( FROM_RF_CENTER_OF, "slice", TO_ECHO_OF, "read" );
	temin2 = get_timing( FROM_ECHO_OF, "read", TO_RF_CENTER_OF, "nextSlice" );
	
	htrmin = MAX( temin, temin2 );
	
	if( minte[0] == 'y' ){
		te = htrmin;
	}
	
	tepad = granularity( te - temin, GRADIENT_RES );
	tepad2 = granularity( te - temin2, GRADIENT_RES );

	te = temin + tepad;	
	putCmd("setvalue('te', %f, 'current')\n", te );

	if( tepad>0.0 )		change_timing( "readDeph", tepad );
	if( tepad2>0.0 )	change_timing( "nextSlice", tepad2 );

	tr = get_timing( FROM_START_OF, "slice", TO_START_OF, "nextSlice" );
	putvalue("tr", tr );

	delayRFToAcq = get_timing( FROM_RF_PULSE_OF, "slice", TO_ACQ_OF, "read" );
	delayAcqToRF = get_timing( FROM_ACQ_OF, "read", TO_RF_PULSE_OF, "nextSlice" );

	set_comp_info( &ss_pre, "ss_pre" );
	write_comp_grads_snippet( NULL, NULL, &ss_pre, "START_OF_KERNEL", "markerA" );

	set_comp_info( &read, "ro" );
	set_comp_info( &phase, "pe" );
	set_comp_info( &slice, "ss" );
	write_comp_grads_snippet( &read, &phase, &slice, "markerA", "markerB" );

	set_comp_info( &ss_post, "ss_post" );
	write_comp_grads_snippet( NULL, NULL, &ss_post, "markerB", "END_OF_KERNEL" );

	/* Set up frequency offset pulse shape list ********/   	
	offsetlist(pss,ss_grad.ssamp,0,freqlist,ns,seqcon[1]);
	shapelist1 = shapelist(p1_rf.pulseName,ss_grad.rfDuration,freqlist,ns,ss_grad.rfFraction, seqcon[1]);

	/* Set pe_steps for profile or full image **********/   	
	pe_steps = prep_profile(profile[0],nv,&pe_grad,&pe_grad);/* profile[0] is n y or r */
	F_initval(pe_steps/2.0,vpe_offset);

	g_setExpTime(trmean*(ntmean*pe_steps*arraydim + (1+fabs(ssc))*arraydim));

	/* Shift DDR for pro *******************************/   	
	roff = -poffset(pro,ro_grad.roamp);

	/* PULSE SEQUENCE */
	status( A );
	rotate();
        triggerSelect(trigger);
	obsoffset( resto );
	delay( GRADIENT_RES );
	initval( 1+fabs( ssc ), vss );
	
	obspower( p1_rf.powerCoarse );
	obspwrf( p1_rf.powerFine );
	delay( GRADIENT_RES );

	assign(one,vacquire);         // real-time acquire flag
	setacqvar(vacquire);          // Turn on acquire when vacquire is zero 
					
	obl_shapedgradient(ss_pre.name,ss_pre.dur,0,0,ss_pre.amp,NOWAIT);		
	sp1on();
	delay(GRADIENT_RES);
	sp1off();
	delay(ss_pre.dur-GRADIENT_RES );
	msloop( seqcon[1], ns, vms_slices, vms_ctr );
		
		assign(vss,vssc);

		peloop( seqcon[2], pe_steps, vpe_steps, vpe_ctr );

			sub(vpe_ctr,vssc,vpe_ctr);     // vpe_ctr counts up from -ssc
			assign(zero,vssc);
			if (seqcon[2] == 's')
				assign(zero,vacquire); // Always acquire for non-compressed loop
			else {
				ifzero(vpe_ctr);
				assign(zero,vacquire); // Start acquiring when vpe_ctr reaches zero
				endif(vpe_ctr);
			}
		
			if (table)
				getelem(t1,vpe_ctr,vpe_index);
			else {
				ifzero(vacquire);
					sub(vpe_ctr,vpe_offset,vpe_index);
				elsenz(vacquire);
					sub(zero,vpe_offset,vpe_index);
				endif(vacquire);
			}		
			
			pe_shaped3gradient( read.name, phase.name, slice.name,
								read.dur, read.amp, 0, slice.amp,
								-pe_grad.increment, vpe_index, NOWAIT );
			delay(ss_grad.rfDelayFront - granulatedRFDelay);
			shapedpulselist( shapelist1, ss_grad.rfDuration, oph, rof1, rof2, seqcon[1], vms_ctr );

			delay( delayRFToAcq - alfa );
			startacq(alfa);
			acquire( np, 1/ro_grad.bandwidth );
			endacq();
			delay( delayAcqToRF - ss_grad.rfDelayFront + granulatedRFDelay - GRADIENT_RES );
			sp1on();
			delay(GRADIENT_RES);
			sp1off();
			
		endpeloop( seqcon[2], vpe_ctr ); 

	endmsloop( seqcon[1], vms_ctr );

	obl_shapedgradient(ss_post.name,ss_post.dur,0,0,ss_post.amp,WAIT);
}
