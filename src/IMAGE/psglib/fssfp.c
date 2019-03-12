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
	/* declaration of internal variables */
	double freqlist[MAXNSLICE];
	double pe_steps;
	int shapelist1, table;
	double xtime, grad_duration, ror_pad;;
	double temp_tr;

	double readAmp, phaseAmp, sliceAmp;
	double tepad, trpad, delayRFToAcq, delayAcqToEnd;
	
	SGL_KERNEL_INFO_T read, phase, slice;

	double sliceRephTrim;
	
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

	/* initialize parameters */
	table = 0;
	
	/* setup phase encoding order */
	table = set_pe_order();

	init_mri();

	if( (sliceRephTrim = getvalnwarn("sliceRephTrim")) == 0.0 ) {
		sliceRephTrim = 1.0;
	}	

	/* initialize RF and gradient structures */
	shape_rf( &p1_rf, "p1", p1pat, p1, flip1, rof1, rof2 );	// excitation pulse

	init_slice( &ss_grad, "ss", thk );					// slice gradient
	init_slice_refocus( &ssr_grad, "ssr" );				// slice refocus

	init_readout( &ro_grad, "ro", lro, np, sw );		// read gradient
	init_readout_refocus( &ror_grad, "ror" );			// read dephase
	init_dephase( &spoil_grad, "spoil" );				// read spoiler

	init_phase( &pe_grad, "pe", lpe, nv );				// phase gradient

	ss_grad.maxGrad = gmax * 0.57;
	ssr_grad.maxGrad = gmax * 0.57;
	ro_grad.maxGrad = gmax * 0.57;
	ror_grad.maxGrad = gmax * 0.57;
	spoil_grad.maxGrad = gmax * 0.57;
	pe_grad.maxGrad = gmax * 0.57;

	/* calculate the RF pulses, gradient pulses and their interdependencies */
	calc_rf( &p1_rf, "tpwr1", "tpwr1f" );
	calc_slice( &ss_grad, &p1_rf, NOWRITE, "gss" );

//	ssr_grad.gmult = sliceRephTrim;
	calc_slice_refocus( &ssr_grad, &ss_grad, NOWRITE, "gssr" ); 
	
	calc_readout( &ro_grad, NOWRITE, "gro", "sw", "at" );
	calc_readout_refocus( &ror_grad, &ro_grad, NOWRITE, "gror" );

	calc_dephase( &spoil_grad, NOWRITE, at*gro - ro_grad.m0def, "gspoil", "tspoil");

	calc_phase( &pe_grad, NOWRITE, "gpe", "tpe" );

	/* longest gradient between RF pulse and acquire dominates */
	xtime = MAX( MAX( ss_grad.tramp + ssr_grad.duration,
						ror_grad.duration + ro_grad.tramp ),
						pe_grad.duration );

	ror_pad = xtime - ror_grad.duration - ro_grad.tramp;

	/* make a gradient list */
	start_kernel( &sk );
	add_gradient( (void*)&ss_grad,   "slice",    SLICE, START_TIME, "",         0.0,           PRESERVE );
	add_gradient( (void*)&ssr_grad,  "sliceRef", SLICE, BEHIND,     "slice",    0.0,           INVERT );
	add_gradient( (void*)&ror_grad,  "readDeph", READ,  BEHIND,     "slice",   -ss_grad.tramp + ror_pad, INVERT );
	add_gradient( (void*)&ro_grad,   "read",     READ,  BEHIND,     "readDeph", 0.0,           PRESERVE );
	add_gradient( (void*)&spoil_grad,"spoil",    READ,  BEHIND,     "read",    -ro_grad.tramp, PRESERVE );
	add_gradient( (void*)&pe_grad,   "phase",    PHASE, SAME_START, "readDeph", 0.0, PRESERVE );
	add_gradient( (void*)&pe_grad,   "rewind",   PHASE, BEHIND,   	"read",    -ro_grad.tramp, INVERT );


        /* Min TE ******************************************/
	temin = get_timing( FROM_RF_CENTER_OF, "slice", TO_ECHO_OF, "read" );

	if( minte[0] == 'y') {
		te = temin;
		putvalue("te", te );
	} else if( FP_LT(te, temin) ) {
		abort_message("TE is too short. TE is %f Minimum is %.4fms\n",te*1000.0, temin*1000);
	} else {
		tepad = granularity( te - temin, GRADIENT_RES );
		if( FP_GT(tepad, 0.0) ) {
			te = temin + tepad;
			change_timing( "readDeph", tepad );
			putvalue("te", te );
		}
	}	

        /* Min TR ******************************************/
	trmin = grad_list_duration();
	trmin += GRADIENT_RES;

	if (mintr[0] == 'y') {
 	  tr = trmin;
	} else if ( FP_LT(tr,trmin) ) {
		abort_message("TR is too short. TR is %f Minimum is %.4fms\n",tr*1000.0, trmin*1000);
	}
	trpad = granularity(tr - trmin, GRADIENT_RES);
	tr = trmin + trpad;
	putvalue("tr",tr);
	


	/* get some timing from the gradient list for the pulse program */
	delayRFToAcq = get_timing( FROM_RF_PULSE_OF, "slice", TO_ACQ_OF, "read" );
	delayAcqToEnd = get_timing( FROM_ACQ_OF, "read", TO, "END_OF_KERNEL" );
	/* perform gradient duty cycle calculations */
	if( checkduty[0] == 'y' )
	{
		init_duty_cycle();
 		duty_cycle_of_list();
		temp_tr = calc_duty_cycle( tr );
	
		if( mintr[0] == 'y' && temp_tr > tr )
		{
			tr = temp_tr;
			trpad = granularity(tr - trmin, GRADIENT_RES);	
			tr = trmin + trpad;
			temp_tr = calc_duty_cycle( tr );
		}
		/* return the duty cycle estimates to VJ */
		putCmd("setvalue('rmsx', %f, 'current')\n", rmsCurrents.x);					
		putCmd("setvalue('rmsy', %f, 'current')\n", rmsCurrents.y);					
		putCmd("setvalue('rmsz', %f, 'current')\n", rmsCurrents.z);					

		if( mintr[0] == 'n' && temp_tr > tr ) /* give a duty cycle warning if required */
		{
			abort_message("Duty cycle limits exceeded! Increase tr to %g ms",
						granularity(temp_tr, GRADIENT_RES)* 1000.0 );
		}
	}

	/* write that shapes to disk */
	set_comp_info( &read, ro_grad.name );
	set_comp_info( &phase, pe_grad.name );
	set_comp_info( &slice, ss_grad.name );

	write_comp_grads( &read, &phase, &slice ); 

	/* Set up frequency offset pulse shape list ********/   	
	offsetlist(pss,ss_grad.ssamp,0,freqlist,ns,seqcon[1]);
	shapelist1 = shapelist(p1_rf.pulseName,ss_grad.rfDuration,freqlist,ns,0, seqcon[1]);

	/* Set pe_steps for profile or full image **********/   	
	pe_steps = prep_profile(profile[0],nv,&pe_grad,&pe_grad);/* profile[0] is n y or r */
	F_initval(pe_steps/2.0,vpe_offset);

	g_setExpTime(trmean*(ntmean*pe_steps*arraydim + fabs(ssc)*arraydim));

	/* Shift DDR for pro *******************************/   	
	roff = -poffset(pro,ro_grad.roamp);

	/* PULSE SEQUENCE */
	status( A );
	rotate();
        triggerSelect(trigger);
	obsoffset( resto );
	delay( GRADIENT_RES );
	initval(fabs(ssc),vss);
	
	obspower( p1_rf.powerCoarse );
	obspwrf( p1_rf.powerFine );
	delay( GRADIENT_RES );

	assign(one,vacquire);         // real-time acquire flag
	setacqvar(vacquire);          // Turn on acquire when vacquire is zero 
					
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
			delay( ss_grad.rfDelayFront );
			shapedpulselist( shapelist1, ss_grad.rfDuration, oph, rof1, rof2, seqcon[1], vms_ctr );
			delay( delayRFToAcq - alfa );
			startacq(alfa);
			acquire( np, 1/ro_grad.bandwidth );
			endacq();
			delay( delayAcqToEnd + trpad );
			
		endpeloop( seqcon[2], vpe_ctr );
	endmsloop( seqcon[1], vms_ctr );
}
