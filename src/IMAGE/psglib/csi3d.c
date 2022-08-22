/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*****************************************************************************/
/*                                                                           */
/*	SI (3D-FSW): 3-DIMENSIONAL SPECTROSCOPIC IMAGING                     */
/*		     BY FOURIER SERIES WINDOW FOR AN OPTIMIZED REGION        */
/*                   (INCREMENTS GRADIENT MULTIPLIERS WITH EXTERNAL TABLE)   */
/*                   WITH BILEVEL B2 (NOE and/or decoupling)                 */
/*                                                                           */
/*         siaxes[0]    first SI phase-encode dimension                      */
/*         siaxes[1]    second SI phase-encode dimension                     */
/*         siaxes[2]    third SI phase-encode dimension                      */
/*                                                                           */
/*                                                                           */
/*   for half-spin echo, set spinecho=y and fuleko=n, else set spinecho=n        */
/*   for full-spin echo, set spinecho=y and fuleko=y, else set spinecho=n        */
/*                                                                           */
/*   AP table used for gradient multipliers is in ~/vnmrsys/tablib           */
/*                                                                           */
/*****************************************************************************/

/* @(#)si3dfsw.c    11/24/93 */

/**[1] INCLUDE FILES AND C PREPROCESSOR MATERIAL******************************/
#include <standard.h>
#include "sgl.c"

GENERIC_GRADIENT_T       gcrush_grad;  // crusher in bistro period
GENERIC_GRADIENT_T       gspoil_grad;  // spoiler after bistro

/**[2] PULSE SEQUENCE CODE****************************************************/
void pulsesequence()
{

/**[2.1] DECLARATIONS*********************************************************/
        double sequence_error;
	double pwpi;
	//double nsteps1,nsteps2,nsteps3,gpe1,gpe2,gpe3;
	double base1,base2,base3,gbase1,gbase2,gbase3,conv;
	double dt,del1,del2,poscos,possin,negsin,satoffset,sattime;	
	int iticks,n_loop;
	char pwpat[MAXSTR],pwpipat[MAXSTR],crush[MAXSTR],spoil[MAXSTR];
	char petable[MAXSTR],gpepat[MAXSTR],bistro[MAXSTR];
	char fuleko[MAXSTR]; 
         
        double  tau1, tau2, te_delay1, te_delay2, tr_delay;
       
     

  init_mri();
  get_wsparameters();

/**[2.2] PARAMETER READ IN FROM EXPERIMENT************************************/
	roff=getval("roff");       //receiver offset
	
	pwpi=getval("pwpi");
	psat=getval("psat");
	satpwr=getval("satpwr");
//	tspoil=getval("tspoil");
 //       gspoil=getval("gspoil");
	dt=getval("dt");
	satoffset=getval("satoffset");
	n_loop=getval("n_loop");
        getstr("pwpat",pwpat);
        getstr("pwpipat",pwpipat);
        getstr("satpat",satpat);
        getstr("gpepat",gpepat);  
	getstr("petable",petable);
	getstr("fuleko",fuleko);
	getstr("bistro",bistro);
	getstr("crush",crush);
	getstr("spoil",spoil);
	del1=0.001;
	del2=0.001;
	
	
	
	sattime=psat+rof1+rof2;
	if (crush[0] == 'y')
	sattime=sattime+tcrush;
	sattime=sattime*8*n_loop;
	if (spoil[0] == 'y')
	sattime=sattime+tspoil;
	if ((ix==1) && (bistro[0] == 'y'))
	printf("saturation time = %f s\n", sattime);  

/* Initialize gradient structures *************************/
      
      init_phase(&pe_grad,"pe",lpe,nv);
      init_phase(&pe3_grad,"pe3",lpe3,nv3); //this is the slice direction
      init_generic(&gcrush_grad,"gcrush",gcrush,tcrush);
      init_generic(&gspoil_grad,"gspoil",gspoil,tspoil);

      calc_phase(&pe_grad,  NOWRITE, "gpe","tpe");
      calc_phase(&pe3_grad, NOWRITE, "gpe3","");
       
      calc_generic(&gcrush_grad,WRITE,"","");
      calc_generic(&gspoil_grad,WRITE,"","");

  /* Equalize refocus and PE gradient durations *************/
  calc_sim_gradient(&pe_grad, &pe_grad, &pe3_grad, 0,WRITE);



    /* Set pe_steps for profile or full image **********/   	
  pe_steps  = prep_profile(profile[0],nv, &pe_grad, &null_grad);
  pe3_steps = prep_profile(profile[2],nv3,&pe3_grad,&null_grad);
  pe2_steps = prep_profile(profile[1],nv3,&pe_grad,&null_grad);
	
/**[2.3] ERROR MESSAGES********************************************************/
	sequence_error=0.0;        
  	//if (d1<sattime) {  
        //	printf("Minimum d1 = %f s\n", sattime);  
  	//	sequence_error=1.0;  
  	//}  
	if (sequence_error > 0.5) {
		text_error("Sequence Terminated\n");
		psg_abort(1);
	}

/**[2.4] CALCULATIONS**********************************************************/
	

	if (spinecho[0] == 'y')  {
		
            
            del1=(te/2.0)-rof2-pe_grad.duration-rof1-(pwpi/2.0);

		  if ((fuleko[0] == 'n') || (fuleko[0] == 'N'))
		    del2=(te/2.0)-(pwpi/2.0)-rof2;
		  else
		    del2=(te/2.0)-(pwpi/2.0)-rof2-(at/2.0);

		if (del1 < 0.0) {
		   text_error("SEQUENCE ERROR: del1 evaluated as negative\n");
		   psg_abort(1);
		}
		if (del2 < 0.0) {
		   text_error("SEQUENCE ERROR: del2 evaluated as negative\n");
		   psg_abort(1);
		}
	}
	if (nv > 1.0) {
            
            base1=0.5-(nv/2.0);
            gbase1=pe_grad.increment*base1;
	}
	else  gbase1=0.0;

	if (nv > 1.0) {
	   
             base2=0.5-(nv/2.0);
             gbase2=pe_grad.increment*base2;
	}
	else  gbase2=0.0;

	if (nv3 > 1.0) {
	    base3=0.5-(nv3/2.0);
            gbase3=pe3_grad.increment*base3;    
	}
	else  gbase3=0.0;



       /* Return parameters to VnmrJ */
  
  putvalue("gpe",pe_grad.peamp);       // PE max grad amp
  putvalue("tpe",pe_grad.duration);    // PE grad duration
  putvalue("gpe3",pe3_grad.peamp);     // PE2 max grad amp
  putvalue("gpeincr",pe_grad.increment);     // PE2 max grad amp
  putvalue("gpe3incr",pe3_grad.increment);     // PE2 max grad amp


   /* Min TR *************************************************/   
	
  tau1 = 2*4e-6 + pw + te + at;
  if (ws[0] == 'y') {
    create_watersuppress();
    tau1 += wsTime;
  }

  if (ws[0] == 'y') {
    
    tau1 += sattime;
  }

  

  trmin = tau1 + 4e-6;   /* have at least 4us between gradient events */

  printf("trmin = %f s\n", trmin);  

  if (mintr[0] == 'y') {
    tr = trmin;  // ensure at least 4us between gradient events
    putvalue("tr",tr);
  }
  if ((trmin-tr) > 12.5e-9) {
    abort_message("TR too short.  Minimum TR= %.2fms\n",trmin*1000);
  }

  tr_delay = tr - tau1;
  if (tr_delay < 0.0) {
      abort_message("tr too short.  Minimum tr = %.2f ms\n",trmin*1000);
  }

  sgl_error_check(sglerror);

 // g_setExpTime(tr*(nt*arraydim)); //at this point it's not clear how to calculate if nt are varied, nt=1,1,1,2,2,1,etc

  
  
  

/**[2.5] PHASE CYCLE AND CONTROL ELEMENTS**************************************/
/*	assign(zero,oph);  ********/

/**[2.6] SEQUENCE ELEMENTS*****************************************************/
	loadtable(petable);
        initval((ix-1.0),v1);//v1 is array element counter
	initval(satpwr,v6); 
	initval(8*n_loop,v9); 
       /* PULSE SEQUENCE *************************************/
       rotate();
       obsoffset(resto);
       delay(4e-6);
       if (ticks) {
          xgate(ticks);
          grad_advance(gpropdelay);
        }

	/* TTL scope trigger **********************************/       
	 sp1on(); delay(4e-6); sp1off();

       status(A);
       //delay(d1-sattime);
	
	
	//status(B);
	loop(v9,v8); //v9 is the total number of steps in loop, v8 is the counter
        getelem(t4,v8,v12); //v12 is 0,6,12,... up to 8
        add(v6,v12,v13);     
        
        obspower(v13);
        
        if (bistro[0] =='y')
        shapedpulseoffset(satpat,psat,zero,rof1,rof2,satoffset);
       
        else
        delay(psat+rof1+rof2);
        if (crush[0] == 'y') {
        obl_shapedgradient(gcrush_grad.name,gcrush_grad.duration,gcrush_grad.amp,gcrush_grad.amp,gcrush_grad.amp,WAIT); 
	}
        endloop(v8);      
        
        if (spoil[0] == 'y') {
        obl_shapedgradient(gspoil_grad.name,gspoil_grad.duration,gspoil_grad.amp,gspoil_grad.amp,gspoil_grad.amp,WAIT);
        }
        
	//delay(dt);
	
        /* Prepulses ******************************************/       
	//if (sat[0]  == 'y') satbands();
	if (ws[0]   == 'y') watersuppress();

       
       obspower(tpwr1);
     
       delay(4e-6);
	
	shapedpulse(pwpat,pw,oph,rof1,rof2);

	/* PHASE ENCODE GRADIENTS ***********/
	
	getelem(t1,v1,v2);
        getelem(t2,v1,v3);
        getelem(t3,v1,v4);
       


  pe3_shapedgradient(pe_grad.name,pe_grad.duration,
  gbase1,gbase2,gbase3,
  pe_grad.increment,pe_grad.increment,pe3_grad.increment,
  v2,v3,v4,WAIT);

	/* REFOCUSING PULSE FOR SPIN ECHO *************/
	if ((spinecho[0] == 'y')) {
	   obspower(tpwr2);
          
           delay(4e-6);
	   delay(del1);
	   shapedpulse(pwpipat,pwpi,zero,rof1,rof2);
	   delay(del2);
	}
       

	/* ACQUISITION ********************************/
	//status(D);
	startacq(alfa);
	acquire(np,1.0/sw);
	endacq();
        delay(tr_delay);
}
