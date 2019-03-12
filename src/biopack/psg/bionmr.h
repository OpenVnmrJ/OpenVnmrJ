/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/**********************************************************************
   Boban John    2 June 98
   2  June 98  BJ Pbox_psg.h  corrected for channel independance
   18 June 98  BJ modified for compatibility with Protein Pack
   Jan to March 99  MRB implemented all pulses and decoupling in PPack.
   Added "gly" as C13 band. Ron Venters, Duke
   Added additional functions July 2006  by Bernhard Brutscher, Grenoble.
    (h_shapedpulse, hshapedfiles, h_shapedpw,c_shapefiles,c_shapedpulse,
     c_shapedpw,c_shapedfiles2,c_shapedpulse2,c_shapedpw2,c_simshapedpulse,
     c_simshapedpw,nshapefiles,n_shapedpulse,n_shapedpw,cn_simshapedpulse,
     cn_simshapedpw,hc_simshapedpulse,hc_simshapedpw,h_simshapedpulse,
     h_simshapedpw,h_sim3shapedpulse,h_sim3shapedpw)
   Added additional function Sept 2006 by Perttu Permi, U.Helsinki
    (ihn_nh_evol_se_train)
   Made compatible with VnmrJ2.2C versions later than July 2007, D.Iverson,Var.
   Added additional functions April 2008  by Bernhard Brutscher, Grenoble.
   (nh_evol_train, ch_simshapedpulse,ch_simshapedpulsepw,ch_simshapedpulse2
    and ch_simshapedpulse2pw,hn_simshapedpulse,hn_simshapepulsepw)

***********************************************************************/

#ifndef DPS
#include <stdlib.h>
#endif
#include "Pbox_psg.h"
#define F_OK 0

#ifndef PWRF_DELAY
#define PWRF_DELAY 0.5e-6
#endif

/* xxxxxxxxxxxxx  GLOBAL PARAMETERS FOR USE IN BIOPACK  xxxxxxxxxxxxxxxx */

 static int	icosel = 0;

 static double	tau2 = 0.0,
		kappa = 0.0,
  		lambda = 0.0, 
		pwHs = 0.0, 
		widthHd = 0.0, 
		pwHd = 0.0;


/* xxxxxxxxxx   SET C13 OFFSET TO MIDDLE OF CHOSEN FREQUENCY BAND   xxxxxxx */
/* xxxxxxxxxx            AND CREATE GLOBAL PARAMETERS               xxxxxxx */

 static double 		CURR_C13_DOF=0.0,
 			REF_C13_DOF=0.0,
 			SPW, STEPS, REF_PWR, REF_PW90, 
                        REF_PWR_C, REF_PW90_C,
                        REF_PWR_N, REF_PW90_N;
 static char 		CURR_C13_OFFSET[MAXSTR], 
                        SNAME[MAXSTR], SNAME_C[MAXSTR],SNAME_N[MAXSTR];

 double get_c13dof(char *fband)
 {
 double c13dof = 0.0;
 if (! strcmp(fband, "ca"))           c13dof=REF_C13_DOF-((175-56)*dfrq);
else if (! strcmp(fband, "ca_edge"))    c13dof=REF_C13_DOF-((175-67)*dfrq);
else if (! strcmp(fband, "ca_ctmq"))    c13dof=REF_C13_DOF-((175-70)*dfrq);
 else if (! strcmp(fband, "ca_b"))    c13dof=REF_C13_DOF-((175-56.25)*dfrq);
 else if (! strcmp(fband, "aliph"))   c13dof=REF_C13_DOF-((175-42)*dfrq);
 else if (! strcmp(fband, "methyl"))   c13dof=REF_C13_DOF-((175-19.0)*dfrq);
 else if (! strcmp(fband, "ctoc"))   c13dof=REF_C13_DOF-((175-30)*dfrq);
 else if (! strcmp(fband, "cb"))      c13dof=REF_C13_DOF-((175-35)*dfrq);
 else if (! strcmp(fband, "cb_l"))    c13dof=REF_C13_DOF-((175-91)*dfrq);
 else if (! strcmp(fband, "cb_r"))    c13dof=REF_C13_DOF-((175-22)*dfrq);
 else if (! strcmp(fband, "cab"))     c13dof=REF_C13_DOF-((175-46)*dfrq);
 else if (! strcmp(fband, "allch"))   c13dof=REF_C13_DOF-((175-70)*dfrq);
 else if (! strcmp(fband, "co_ca"))   c13dof=REF_C13_DOF-((175-110)*dfrq);
 else if (! strcmp(fband, "arom"))    c13dof=REF_C13_DOF-((175-125)*dfrq);
 else if (! strcmp(fband, "arom_aliph"))    c13dof=REF_C13_DOF-((175-80)*dfrq);
 else if (! strcmp(fband, "idcal"))   c13dof=REF_C13_DOF-((175-(-23.5))*dfrq);
 else if (! strcmp(fband, "co"))      c13dof=REF_C13_DOF;
 else if (! strcmp(fband, "gly"))
    c13dof=REF_C13_DOF-((174-43)*dfrq);
 else { printf("\nFrequency Band Specified is not defined! Abort!"); psg_abort(1); }
 return c13dof;
 }

 void set_c13offset(char *fband)
 {
   double dof;   dof=getval("dof");
   if (fabs(REF_C13_DOF) < 1.0e-6)       REF_C13_DOF=dof;
   CURR_C13_DOF=get_c13dof(fband);
   strcpy(CURR_C13_OFFSET, fband);
   decoffset(CURR_C13_DOF);
 }




/* xxxxxxxxxxx     RETRIEVE PARAMETERS FROM .RF FILE HEADER     xxxxxxxxxxxx */ 

shape getRshapeinfo(char *shname)  
{
  shape    rshape;
  FILE     *inpf;
  int      j;
  char     ch, str[3*MAXSTR];
  extern char userdir[], systemdir[];

  strcpy(rshape.name, shname);
  fixname(rshape.name);
  (void) sprintf(str, "%s/shapelib/%s.RF" , userdir, rshape.name);
  if ((inpf = fopen(str, "r")) == NULL)
  {
    (void) sprintf(str, "%s/shapelib/%s.RF", systemdir, rshape.name);
    if ((inpf = fopen(str, "r")) == NULL)
    {
      abort_message("Pbox_psg, getRshapeinfo : Cannot find \"%s\"\n", str);
    }
  }
  j = fscanf(inpf, "%c %s %lf %lf %lf\n", &ch, str, &rshape.pw, 
                                          &rshape.pwr, &rshape.pwrf);
  fclose(inpf);

  if ((j == 5) && (ch == '#') && (str[0] == 'P') && (str[1] == 'b') && 
      (str[2] == 'o') && (str[3] == 'x'))
    rshape.pw /= 1000000.0;
  else
    rshape.pw = 0.0, rshape.pwr = 0.0, rshape.pwrf = 0.0;

  return rshape; 
}



/* xxxxxxxxxxxxxxxxxxxxxxxxxxx  BEGIN C13 PULSES  xxxxxxxxxxxxxxxxxxxxxxxxxxx */

/*   CALCULATE 13C PULSE LENGTH, STEPS AND RETURN LENGTH TO PULSE SEQUENCE    */

double c13pulsepw(char *excband, char *nullband, char *c13shape, double c13flip) 
{
 char		cshape[MAXSTR];
 double		nullshift, destbanddof, 
  		sfrq = getval("sfrq");


/* DETERMINE NULL POINT */
 destbanddof = get_c13dof(excband);
 nullshift   = fabs(get_c13dof(nullband) - destbanddof);


/*   CALCULATE FLIP ANGLE (pulse length) FOR THE SELECTIVE PULSE   */
 if ( ! strcmp(c13shape, "") )   strcpy(cshape, "square"); 
 else  				 strcpy(cshape, c13shape); 

 if ( ! strcmp(cshape, "square") )
  {
     if ((c13flip-90.0) < 1.0e-3)          SPW = sqrt(15.0)/(4.0*nullshift); 
     else if ((c13flip-180.0) < 1.0e-3)    SPW = sqrt(3.0)/(2.0*nullshift); 
     else 
    {printf("\n%6.1f degree flip not implemented !  Abort!", c13flip);psg_abort(1);}
     STEPS = SPW*5.0e6;   STEPS = (int)(STEPS + 0.5);    SPW = STEPS/5.0e6;
  }

 else if  ( ! strcmp(cshape, "sinc") )
  {
     if ((c13flip-90.0) < 1.0e-3)          SPW = 88.8*600.0/(sfrq*1.0e6); 
     else if ((c13flip-180.0) < 1.0e-3)    SPW = 80.5*600.0/(sfrq*1.0e6); 
     else
    {printf("\n%6.1f degree flip not implemented !  Abort!", c13flip);psg_abort(1);}
     STEPS=SPW*5.0e6;  STEPS=2.0*((int)(STEPS/2.0))+1.0;  SPW=(STEPS+1.0)/5.0e6;
  }
 else          {printf("\n  c13shape not implemented !    Abort!");   psg_abort(1);}

 return(SPW);
}


/* xxxxxxxxx   MAKE SHAPE WITH PBOX AND PLACE IN USERDIR/SHAPELIB   xxxxxxxxx */

void c13shapefiles(char *excband, char *nullband, char *c13shape, double c13flip)
{
 FILE 		*fp;
 char		cshape[MAXSTR], fs[MAXSTR], fname[MAXSTR];
 extern char	userdir[];
 double		spw, freqshift, destbanddof, steps,
 		pwC    = getval("pwC"),
 		pwClvl = getval("pwClvl");


/* DETERMINE FREQUENCY SHIFT */
 destbanddof = get_c13dof(excband);
 freqshift   = (destbanddof - CURR_C13_DOF);
 

/* CALCULATE PULSE LENGTH & NO. OF STEPS */
c13pulsepw(excband, nullband, c13shape, c13flip); steps=STEPS; spw=SPW;


/*   MAKE C13 SHAPE FILE NAME    */
 if ( ! strcmp(c13shape, "") )   strcpy(cshape, "square"); 
 else  				 strcpy(cshape, c13shape); 
 (void) sprintf(SNAME, "pp.%s.%s.%s.%s.%d.RF", CURR_C13_OFFSET, cshape, excband,
					 nullband, (int)(c13flip+0.5) );
 
/*   GENERATE SHAPE FILE IF REQUIRED   */
 (void) sprintf(fname, "%s/shapelib/%s", userdir, SNAME);

 if ((d2_index == 0) && (d3_index == 0))
  {
     (void) sprintf(fname, "%s/shapelib/Pbox.inp", userdir);
     if ( (fp=fopen(fname, "w")) != NULL )
       {
         int ret __attribute__((unused));
         fprintf(fp, "\n Pbox Input File ");
         fprintf(fp, "\n Carrier at: %s   Exc max at: %s      Null at: %s", 
                    			   CURR_C13_OFFSET, excband, nullband);
         fprintf(fp, "\n { %s %14.8f %10.1f 1.0 0.0 %6.1f } ", cshape, spw, 
							   freqshift, c13flip);
         fprintf(fp, "\n name = %s  steps = %4d  sucyc = n  attn = e ", SNAME, 
								 (int)(steps));
         fprintf(fp, "\n RF Calibration Data: ref_pwr = %4d  ref_pw90 = %5.1f", 
						     (int)(pwClvl), pwC*1.0e6);
         fclose(fp);
         sprintf(fs,"cd $vnmruser/shapelib; Pbox > /tmp/${LOGNAME}_bionmrfile");
         ret = system(fs);
	 /*printf("\nFreq Shift = %5.0f  Pulse Width = %0.7f  File Name:%s", 
							freqshift, spw, SNAME);*/
       }
     else          { printf("\nUnable to write Pbox.inp file !");  psg_abort(1); }
  }
}


/* xxxxxxxxxxxxxxxxxxxxxxxxx    c13pulse    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx */

void c13pulse(char *excband, char *nullband, char *c13shape,
              double c13flip, codeint phase2, double rofa, double rofb) 
{
 shape		pwxshape;
 double		pwClvl = getval("pwClvl"),
 		compC  = getval("compC");

/* CHECK FOR ZERO FLIP & EXECUTE NULL PULSE IF ZERO */
 if (c13flip <= 1.0e-6)
   { decpower(pwClvl); decpwrf(4095.0);
     decshaped_pulse("", 0.0, phase2, rofa, rofb);
     decpower(pwClvl); decpwrf(4095.0); return; }


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
c13shapefiles(excband, nullband, c13shape, c13flip);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwxshape = getRshapeinfo(SNAME);
 decpower(pwxshape.pwr);
 if (pwxshape.pwrf)   decpwrf(pwxshape.pwrf*compC); 
 else                 decpwrf(4095.0); 
 decshaped_pulse(pwxshape.name, pwxshape.pw, phase2, rofa, rofb);
 decpwrf(4095.0);
 decpower(pwClvl);
}


/* xxxxxxxxxxxxxxxxxxxxxxxxx     sim_c13pulse    xxxxxxxxxxxxxxxxxxxxxxxxxxx  */

void sim_c13pulse(char *obsshape, char *excband, char *nullband, char *c13shape,
                  double obspw, double c13flip, codeint phase1, codeint phase2,
                  double rofa, double rofb) 
{
 shape		pwxshape;
 double		pwClvl = getval("pwClvl"),
 		compC  = getval("compC");

/* CHECK FOR ZERO FLIP & EXECUTE NULL PULSE IF ZERO */
 if (c13flip <= 1.0e-6)
   { decpower(pwClvl); decpwrf(4095.0);
     simshaped_pulse(obsshape, "", obspw, 0.0, phase1, phase2, rofa, rofb);
     decpower(pwClvl); decpwrf(4095.0); return; }

  
/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
c13shapefiles(excband, nullband, c13shape, c13flip);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwxshape = getRshapeinfo(SNAME);
 decpower(pwxshape.pwr);
 if (pwxshape.pwrf)      decpwrf(pwxshape.pwrf*compC);
 else   		 decpwrf(4095.0);
 simshaped_pulse(obsshape, pwxshape.name, obspw, pwxshape.pw, phase1, phase2,
 								rofa, rofb);
 decpwrf(4095.0);
 decpower(pwClvl);
}


/* xxxxxxxxxxxxxxxxxxxxxxxxx     sim3_c13pulse     xxxxxxxxxxxxxxxxxxxxxxxxx  */

void sim3_c13pulse(char *obsshape, char *excband, char *nullband, char *c13shape,
                   char *dec2shape, double obspw, double c13flip, double dec2pw,
                   codeint phase1, codeint phase2, codeint phase3,
                   double rofa, double rofb) 
{
 shape		pwxshape;
 double		pwClvl = getval("pwClvl"),
 		compC  = getval("compC");

/* CHECK FOR ZERO FLIP & EXECUTE NULL PULSE IF ZERO */
 if (c13flip <= 1.0e-6)
   { decpower(pwClvl); decpwrf(4095.0);
     sim3shaped_pulse(obsshape, "", dec2shape, obspw, 0.0, dec2pw, phase1, 
						phase2, phase3, rofa, rofb);
     decpower(pwClvl); decpwrf(4095.0); return; }

  
/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
c13shapefiles(excband, nullband, c13shape, c13flip);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwxshape = getRshapeinfo(SNAME);
 decpower(pwxshape.pwr);
 if (pwxshape.pwrf)      decpwrf(pwxshape.pwrf*compC);
 else   		 decpwrf(4095.0);
 sim3shaped_pulse(obsshape, pwxshape.name, dec2shape, obspw, pwxshape.pw, 
				dec2pw, phase1, phase2, phase3, rofa, rofb);
 decpwrf(4095.0);
 decpower(pwClvl);
}



/* xxxxxx MAKE ADIABATIC SHAPE WITH PBOX AND PLACE IN USERDIR/SHAPELIB xxxxxx */

void c13adiab_files(char *excband, double bandwidth, char *c13shape, double pulsewidth)
{
 FILE 		*fp;
 char		cshape[MAXSTR], fs[MAXSTR], fname[MAXSTR];
 extern char	userdir[];
 double		spw, freqshift, destbanddof, steps, bwdth,
 		pwC    = getval("pwC"),
 		pwClvl = getval("pwClvl");


/* DETERMINE FREQUENCY SHIFT */
 destbanddof = get_c13dof(excband);
 freqshift   = (destbanddof - CURR_C13_DOF);
 

/* CALCULATE BANDWIDTH & NO. OF STEPS */
 if      ( ! strcmp(c13shape, "sech1") )
       	{ bwdth = bandwidth*dfrq + 3850.0;   	strcpy(cshape, "sech"); }
 else if ( ! strcmp(c13shape, "sech10") )
     	{ bwdth = bandwidth*dfrq + 310.0;   	strcpy(cshape, "sech"); }
 else 	{ bwdth = bandwidth*dfrq;		strcpy(cshape, c13shape); }


 if      (pulsewidth < 1.0e-3) 		{ steps = pulsewidth*1.0e6;
			   steps = (int)(steps + 0.5);   spw = steps/1.0e6;  }
 else if (pulsewidth < 2.0e-3) 		{ steps = pulsewidth*0.5e6;
			   steps = (int)(steps + 0.5);   spw = steps/0.5e6;  }
 else if (pulsewidth < 5.0e-3) 		{ steps = pulsewidth*0.2e6;
			   steps = (int)(steps + 0.5);   spw = steps/0.2e6;  }
 else    				{ steps = pulsewidth*0.1e6;
			   steps = (int)(steps + 0.5);   spw = steps/0.1e6;  }


/*   MAKE C13 SHAPE FILE NAME    */
 (void) sprintf(SNAME, "pp.%s.%s.%s.%d.%d.RF", CURR_C13_OFFSET, cshape, 
			excband, (int)(bandwidth+0.5), (int)(spw*1e3+0.5) );
 
/*   GENERATE SHAPE FILE IF REQUIRED   */
 (void) sprintf(fname, "%s/shapelib/%s", userdir, SNAME);
 if ((d2_index == 0) && (d3_index == 0))
  {
     (void) sprintf(fname, "%s/shapelib/Pbox.inp", userdir);
     if ( (fp=fopen(fname, "w")) != NULL )
       {
         int ret __attribute__((unused));
         fprintf(fp, "\n Pbox Input File ");
         fprintf(fp, "\n Carrier at: %s   Exc max at: %s   Bandwidth(ppm): %s", 
                    			   CURR_C13_OFFSET, excband, fs);
         fprintf(fp, "\n { %s %10.1f/%6.5f %10.1f } ", cshape, bwdth, spw, 
							   freqshift);
         fprintf(fp, "\n name = %s  steps = %4d  sucyc = n  attn = e ", SNAME, 
								 (int)(steps));
         fprintf(fp, "\n RF Calibration Data: ref_pwr = %4d  ref_pw90 = %5.1f", 
						     (int)(pwClvl), pwC*1.0e6);
         fclose(fp);
         sprintf(fs,"cd $vnmruser/shapelib; Pbox > /tmp/${LOGNAME}_bionmrfile");
         ret = system(fs);
	 /*printf("\nFreq Shift = %5.0f  Band Width = %5.0f  Pulse Width = %0.7f
  	 			  		File Name: %s",
 					freqshift, bandwidth*dfrq, spw, SNAME);*/
       }
     else          { printf("\nUnable to write Pbox.inp file !");  psg_abort(1); }
  }
}


/* xxxxxxxxxxxxxxxxxxxxxxx    c13adiab_inv_pulse   xxxxxxxxxxxxxxxxxxxxxxxxxx */

void c13adiab_inv_pulse(char *excband, double bandwidth, char *c13shape,
                        double pulsewidth, codeint phase2, double rofa, double rofb)
{
 shape		pwxshape;
 double		pwClvl = getval("pwClvl"),
 		compC  = getval("compC");

/* CHECK FOR ZERO FLIP & EXECUTE NULL PULSE IF ZERO */
 if (pulsewidth <= 1.0e-6)
   { decpower(pwClvl); decpwrf(4095.0);
     decshaped_pulse("", 0.0, phase2, rofa, rofb);
     decpower(pwClvl); decpwrf(4095.0); return; }


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
c13adiab_files(excband, bandwidth, c13shape, pulsewidth);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwxshape = getRshapeinfo(SNAME);
 decpower(pwxshape.pwr);
 if (pwxshape.pwrf)   decpwrf(pwxshape.pwrf*compC); 
 else                 decpwrf(4095.0); 
 decshaped_pulse(pwxshape.name, pwxshape.pw, phase2, rofa, rofb);
 decpwrf(4095.0);
 decpower(pwClvl);
}

/* xxxxxxxxxxxxxxxxxxxxxx    sim_c13adiab_inv_pulse   xxxxxxxxxxxxxxxxxxxxxxx */

void sim_c13adiab_inv_pulse(char *obsshape, char *excband, double bandwidth,
                            char *c13shape, double obspw, double pulsewidth,
                            codeint phase1, codeint phase2, double rofa, double rofb) 
{
 shape		pwxshape;
 double		pwClvl = getval("pwClvl"),
 		compC  = getval("compC");

/* CHECK FOR ZERO FLIP & EXECUTE NULL PULSE IF ZERO */
 if (pulsewidth <= 1.0e-6)
   { decpower(pwClvl); decpwrf(4095.0);
     decshaped_pulse("", 0.0, phase2, rofa, rofb);
     decpower(pwClvl); decpwrf(4095.0); return; }


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
c13adiab_files(excband, bandwidth, c13shape, pulsewidth);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwxshape = getRshapeinfo(SNAME);
 decpower(pwxshape.pwr);
 if (pwxshape.pwrf)   decpwrf(pwxshape.pwrf*compC); 
 else                 decpwrf(4095.0); 
 simshaped_pulse(obsshape, pwxshape.name, obspw, pwxshape.pw, phase1, phase2,
 								rofa, rofb);
 decpwrf(4095.0);
 decpower(pwClvl);
}

/* xxxxxxxxxxxxxxxxxxxxx    sim3_c13adiab_inv_pulse   xxxxxxxxxxxxxxxxxxxxxxx */

void sim3_c13adiab_inv_pulse(char *obsshape, char *excband, double bandwidth,
                  char *c13shape, char *dec2shape,  double obspw, double pulsewidth,
                  double dec2pw, codeint phase1, codeint phase2, codeint phase3,
                  double rofa, double rofb) 
{
 shape		pwxshape;
 double		pwClvl = getval("pwClvl"),
 		compC  = getval("compC");

/* CHECK FOR ZERO FLIP & EXECUTE NULL PULSE IF ZERO */
 if (pulsewidth <= 1.0e-6)
   { decpower(pwClvl); decpwrf(4095.0);
     decshaped_pulse("", 0.0, phase2, rofa, rofb);
     decpower(pwClvl); decpwrf(4095.0); return; }


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
c13adiab_files(excband, bandwidth, c13shape, pulsewidth);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwxshape = getRshapeinfo(SNAME);
 decpower(pwxshape.pwr);
 if (pwxshape.pwrf)   decpwrf(pwxshape.pwrf*compC); 
 else                 decpwrf(4095.0); 
 sim3shaped_pulse(obsshape, pwxshape.name, dec2shape, obspw, pwxshape.pw, 
				dec2pw, phase1, phase2, phase3, rofa, rofb);
 decpwrf(4095.0);
 decpower(pwClvl);
}
/* xxxxxxxxxxxxxxxxxxxxxxxxxxx  END C13 PULSES  xxxxxxxxxxxxxxxxxxxxxxxxxxxxx */





/* xxxxxxxxxxxxxxx    BEGIN SHIFTED PULSES FOR ANY CHANNEL    xxxxxxxxxxxxxxx */

/* xxxxxxxxx   MAKE SHAPE WITH PBOX AND PLACE IN USERDIR/SHAPELIB   xxxxxxxxx */

void fshapefiles(char *anyshape, double pwbw, double flip, double shift)
{
 FILE		*fp;
 char 	    fshape[MAXSTR], fs[MAXSTR], fname[MAXSTR];
 extern char	userdir[];


/*    MAKE SHAPE FILE NAME   */  
 if ( ! strcmp(anyshape, "") )      strcpy(fshape, "square"); 
 else  				 strcpy(fshape, anyshape); 
(void) sprintf(SNAME, "pp.%s.%d.%d.%d.RF", fshape,
                       (int)(pwbw*1e6+0.5), (int)(flip+0.5), (int)(shift+0.5) );


 /*   GENERATE SHAPE FILE IF REQUIRED   */
 (void) sprintf(fname, "%s/shapelib/%s", userdir, SNAME);
 if ((d2_index == 0) && (d3_index == 0))
  {
     (void) sprintf(fname, "%s/shapelib/Pbox.inp", userdir);
     if ( (fp=fopen(fname, "w")) != NULL )
       {
         int ret __attribute__((unused));
         fprintf(fp, "\n Pbox Input File ");
         fprintf(fp, "\n { %s %14.8f %12.1f 1.0 0.0 %6.1f } ", fshape, pwbw, 
								  shift, flip);
         fprintf(fp, "\n name = %s    sucyc = n", SNAME);
         fprintf(fp, "\n RF Calibration Data: ref_pwr = %4d  ref_pw90 = %5.1f", 
					(int)(REF_PWR), REF_PW90*1e6);
         fclose(fp);
         sprintf(fs,"cd $vnmruser/shapelib; Pbox > /tmp/${LOGNAME}_bionmrfile");
         ret = system(fs);
       /*printf("\nFreq Shift = %5.0f  Pulse Width = %0.7f  File Name:%s", 
							   shift, pwbw, SNAME);*/
       }
     else          { printf("\nUnable to write Pbox.inp file !");  psg_abort(1); }
  }
}


/* xxxxxxxxxxxxxxxxxxxxxxxxx     shiftedpulse     xxxxxxxxxxxxxxxxxxxxxxxxx  */

void shiftedpulse(char *anyshape, double pwbw, double flip, double shift,
                  codeint phase1, double rofa, double rofb) 
{
 shape 		pwshape;
 double 	compH = getval("compH");

		REF_PW90 = pw;
		REF_PWR = tpwr;


/* CHECK FOR ZERO FLIP & EXECUTE NULL PULSE IF ZERO */
 if (flip <= 1.0e-6)
   { obspower(tpwr); obspwrf(4095.0);
     shaped_pulse("", 0.0, phase1, rofa, rofb);
     obspower(tpwr); obspwrf(4095.0); return; }


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
fshapefiles(anyshape, pwbw, flip, shift);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwshape = getRshapeinfo(SNAME);
 obspower(pwshape.pwr);
 if (pwshape.pwrf)      obspwrf(pwshape.pwrf*compH);
 else			obspwrf(4095.0);
 shaped_pulse(pwshape.name,pwshape.pw,phase1,rofa,rofb);
 obspwrf(4095.0);
 obspower(tpwr);
}


/* xxxxxxxx    decshiftedpulse (not tested, Jan '99, MRB)     xxxxxxxxxxxxxx  */

void decshiftedpulse(char *anyshape, double pwbw, double flip, double shift,
                     codeint phase2, double rofa, double rofb) 
{
 shape		pwxshape;
 double		pwC    = getval("pwC"),
		pwClvl = getval("pwClvl"),
 		compC  = getval("compC");

		REF_PW90 = pwC;
		REF_PWR = pwClvl;


/* CHECK FOR ZERO FLIP & EXECUTE NULL PULSE IF ZERO */
 if (flip <= 1.0e-6)
   { decpower(pwClvl); decpwrf(4095.0);
     decshaped_pulse("", 0.0, phase2, rofa, rofb);
     decpower(pwClvl); decpwrf(4095.0); return; }


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
fshapefiles(anyshape, pwbw, flip, shift);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwxshape = getRshapeinfo(SNAME);
 decpower(pwxshape.pwr);
 if (pwxshape.pwrf)   decpwrf(pwxshape.pwrf*compC); 
 else                 decpwrf(4095.0); 
 decshaped_pulse(pwxshape.name, pwxshape.pw, phase2, rofa, rofb);
 decpwrf(4095.0);
 decpower(pwClvl);
}


/* xxxxxxxx    sim3shiftedpulse (not tested, Jan '99, MRB)    xxxxxxxxxxxxxx  */

void sim3shiftedpulse(char *obsshape, char *anyshape, char *dec2shape, double obspw,
                      double pwbw, double flip, double shift, double dec2pw,
                      codeint phase1, codeint phase2, codeint phase3,
                      double rofa, double rofb) 
{
 shape		pwxshape;
 double		pwC    = getval("pwC"),
		pwClvl = getval("pwClvl"),
 		compC  = getval("compC");

		REF_PW90 = pwC;
		REF_PWR = pwClvl;


/* CHECK FOR ZERO FLIP & EXECUTE NULL PULSE IF ZERO */
 if (flip <= 1.0e-6)
   { decpower(pwClvl); decpwrf(4095.0);
     sim3shaped_pulse(obsshape, "", dec2shape, obspw, 0.0, dec2pw, phase1, 
						phase2, phase3, rofa, rofb);
     decpower(pwClvl); decpwrf(4095.0); return; }


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
fshapefiles(anyshape, pwbw, flip, shift);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwxshape = getRshapeinfo(SNAME);
 decpower(pwxshape.pwr);
 if (pwxshape.pwrf)   decpwrf(pwxshape.pwrf*compC); 
 else                 decpwrf(4095.0); 
 sim3shaped_pulse(obsshape, pwxshape.name, dec2shape, obspw, pwxshape.pw,
				dec2pw, phase1, phase2, phase3, rofa, rofb);
 decpwrf(4095.0);
 decpower(pwClvl);
}


/* xxxxxxxx    dec2shiftedpulse (not tested, Jan '99, MRB)    xxxxxxxxxxxxxx  */

void dec2shiftedpulse(char *anyshape, double pwbw, double flip, double shift,
                      codeint phase3, double rofa, double rofb) 
{
 shape		pwxshape;
 double		pwN    = getval("pwN"),
		pwNlvl = getval("pwNlvl"),
 		compN  = getval("compN");

		REF_PW90 = pwN;
		REF_PWR = pwNlvl;


/* CHECK FOR ZERO FLIP & EXECUTE NULL PULSE IF ZERO */
 if (flip <= 1.0e-6)
   { dec2power(pwNlvl); dec2pwrf(4095.0);
     dec2shaped_pulse("", 0.0, phase3, rofa, rofb);
     dec2power(pwNlvl); dec2pwrf(4095.0); return; }


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
fshapefiles(anyshape, pwbw, flip, shift);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwxshape = getRshapeinfo(SNAME);
 dec2power(pwxshape.pwr);
 if (pwxshape.pwrf)   dec2pwrf(pwxshape.pwrf*compN); 
 else                 dec2pwrf(4095.0); 
 dec2shaped_pulse(pwxshape.name, pwxshape.pw, phase3, rofa, rofb);
 dec2pwrf(4095.0);
 dec2power(pwNlvl);
}

/* xxxxxxxxxxxxxxxxx    END SHIFTED PULSES FOR ANY CHANNEL    xxxxxxxxxxxxxxx */




/* xxxxxxxxxxxxxxxxxxxxxxxxxxxx     DECOUPLING    xxxxxxxxxxxxxxxxxxxxxxxxxxx */

/* xxxxxxxxxxx     RETRIEVE PARAMETERS FROM .DEC FILE HEADER     xxxxxxxxxxxx */ 
shape getDshapeinfo(char *shname)		
{
  shape    dshape;
  FILE     *inpf;
  int      j;
  char     ch, str[3*MAXSTR];
  extern char userdir[], systemdir[];

  strcpy(dshape.name, shname);
  fixname(dshape.name);
  (void) sprintf(str, "%s/shapelib/%s.DEC" , userdir, dshape.name);
  if ((inpf = fopen(str, "r")) == NULL)
  {
    (void) sprintf(str, "%s/shapelib/%s.DEC", systemdir, dshape.name);
    if ((inpf = fopen(str, "r")) == NULL)
    {
      abort_message("bionmr.h, getDshapeinfo : Cannot find \"%s\"\n", str);
    }
  }
  j = fscanf(inpf, "%c %s %lf %lf %lf %lf %lf\n", &ch, str, &dshape.pw, &dshape.pwr, &dshape.pwrf, &dshape.dres, &dshape.dmf);
  fclose(inpf);

  if ((j == 7) && (ch == '#') && (str[0] == 'P') && (str[1] == 'b') && 
      (str[2] == 'o') && (str[3] == 'x'))
    dshape.pw /= 1000000.0;
  else
    dshape.pw = 0.0, dshape.pwr = 0.0, dshape.pwrf = 0.0, dshape.dres = 0.0, 
    dshape.dmf = 0.0;

  return dshape; 
}



/* xxxxxxxxxxxxxxxxxxxxxxx    BEGIN C13 DECOUPLING    xxxxxxxxxxxxxxxxxxxxxxx */
/* xxxxxxxxx   MAKE SHAPE WITH PBOX AND PLACE IN USERDIR/SHAPELIB   xxxxxxxxx */

void c13decfiles(char *decband, char *c13shape, double decbandwidth)
{
 FILE 		*fp;
 char  		cshape[MAXSTR], fs[MAXSTR], fname[MAXSTR];
 double  	freqshift, dwidth, 
 		pwC    = getval("pwC"),
 		pwClvl = getval("pwClvl");


/* DETERMINE FREQUENCY SHIFT AND BANDWIDTH */
 freqshift = get_c13dof(decband) - CURR_C13_DOF;
 dwidth = decbandwidth*0.25*sfrq;


/*   MAKE C13 DEC SHAPE FILE NAME    */
 if ( ! strcmp(c13shape, "") )   strcpy(cshape, "square"); 
 else  				 strcpy(cshape, c13shape); 

 (void) sprintf(SNAME, "pp.%s.%s.%s.%d.DEC",CURR_C13_OFFSET,
                       cshape,decband, (int)(decbandwidth+0.5) );


/*   GENERATE DECOUPLING SHAPE FILE IF REQUIRED   */
 (void) sprintf(fname, "%s/shapelib/%s", userdir, SNAME);
 if ((d2_index == 0) && (d3_index == 0))
   {
     (void) sprintf(fname, "%s/shapelib/Pbox.inp", userdir);
     if ( (fp=fopen(fname, "w")) != NULL )
       {                                                                        
         int ret __attribute__((unused));
         fprintf(fp, "\n Pbox Input File ");                                    
         fprintf(fp, "\n Carrier at: %s   Dec max at: %s  BandWidth: %8.1f",
				             CURR_C13_OFFSET, decband, dwidth); 
         fprintf(fp, "\n { %s %10.1f %10.1f } ", cshape, dwidth, freqshift);
         fprintf(fp, "\n name = %s   sucyc = d   type = d   attn = e ", SNAME);
         fprintf(fp, "\n RF Calibration Data: ref_pwr = %4d  ref_pw90 = %5.1f",
						     (int)(pwClvl), pwC*1.0e6);
         fclose(fp);
         sprintf(fs,"cd $vnmruser/shapelib; Pbox > /tmp/${LOGNAME}_bionmrfile");  
         ret = system(fs);                                                            
       /*printf("\nFreq Shift = %5.0f  Band Width = %10.0f  File Name:%s", 
					      freqshift, dwidth, SNAME);*/
       }
     else          { printf("\nUnable to write Pbox.inp file !");  psg_abort(1); }
   }
}

/* xxxxxxxxxxxxxxxxxxxxxxxxxxxx    c13decon    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx */

void c13decon(char *decband, char *c13shape, double decbandwidth)
{
 shape 		decshape;
 double  	compC  = getval("compC");

/* GENERATE DECOUPLING SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
   c13decfiles(decband, c13shape, decbandwidth);

/* GET SHAPE FILE AND TURN ON DECOUPLING */
   decshape = getDshapeinfo(SNAME);
   decpower(decshape.pwr);
   if (decshape.pwrf)   { decpwrf(decshape.pwrf*compC); }
   else     		{ decpwrf(4095.0*compC); }
#ifndef NVPSG
   decprgon(decshape.name, 1.0/decshape.dmf, decshape.dres);
   decon();	     				   	 /* turn on decoupling*/
#else
   decon();	     				   	 /* turn on decoupling*/
   decprgon(decshape.name, 1.0/decshape.dmf, decshape.dres);
#endif
 }


/* xxxxxxxxxxxxxxxxxxxxxxxxxxxx    c13decoff    xxxxxxxxxxxxxxxxxxxxxxxxxxxxx */

void c13decoff()

{ double 	pwClvl = getval("pwClvl");
#ifndef NVPSG
  decoff();         
  decprgoff();	     	       /* turn off decoupling */
#else
  decoff();         
  decprgoff();	     	       /* turn off decoupling */
#endif
  decpwrf(4095.0);  decpower(pwClvl);
}


/* xxxxxxxxxxxxxxxxxxxxxxxxxxxx    c13decpw    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx */
/* RETURN LENGTH OF ONE DECOUPLING CYCLE TO PULSE SEQUENCE */

double c13decpw(char *decband, char *c13shape, double decbandwidth)
{
   shape 	decshape;
   c13decfiles(decband, c13shape, decbandwidth);       /* generate shape name */
   decshape = getDshapeinfo(SNAME);		    /* return length of cycle */
   return(decshape.pw);
}

/* xxxxxxxxxxxxxxxxxxxxxxxxxxxx    c13decouple    xxxxxxxxxxxxxxxxxxxxxxxxxxx */
/* A DEFINED LENGTH OF C13 DECOUPLING. COMBINES c13decon, c13decpw, c13decoff */

void c13decouple(char *decband, char *c13shape, double decbandwidth, double ncyc_del)
{
 shape 		decshape;

   c13decon(decband, c13shape, decbandwidth);           /* turn on decoupling */
   if (ncyc_del < 1.0)
  	  	 delay(ncyc_del);	      /* decoupling for set time < 1s */
   else 	{ncyc_del = (int) (ncyc_del + 0.5);
		 decshape = getDshapeinfo(SNAME);
   		 delay(ncyc_del*decshape.pw);}      /* decoupling for ncycles */
   c13decoff();	     				   	/* turn off decoupling*/
 }



/* xxxxxxxxxxxxxxxxxxxxxxx     END C13 DECOUPLING     xxxxxxxxxxxxxxxxxxxxxxx */




/* xxxxxxxxxxxxxxxxxxxxxxx     BEGIN H1 DECOUPLING    xxxxxxxxxxxxxxxxxxxxxxx */
/* xxxxxxxxx   MAKE SHAPE WITH PBOX AND PLACE IN USERDIR/SHAPELIB   xxxxxxxxx */

void h1decfiles(char *h1shape, double decbandwidth, double shift)
{
 FILE 		*fp;
 char  		hshape[MAXSTR], fs[MAXSTR], fname[MAXSTR];
 double  	freqshift, dwidth;


/* DETERMINE FREQUENCY SHIFT AND BANDWIDTH */
 freqshift = shift*sfrq;
 dwidth = decbandwidth*sfrq;


/*   MAKE H1 DEC SHAPE FILE NAME    */
 if ( ! strcmp(h1shape, "") )   strcpy(hshape, "square"); 
 else  				 strcpy(hshape, h1shape); 

 (void) sprintf(SNAME, "pp.%s.%d.%d.DEC", hshape,
                (int)(decbandwidth+0.5), (int)(freqshift+0.5) );


/*   GENERATE DECOUPLING SHAPE FILE IF REQUIRED   */
 (void) sprintf(fname, "%s/shapelib/%s", userdir, SNAME);
 if ((d2_index == 0) && (d3_index == 0))
   {
     (void) sprintf(fname, "%s/shapelib/Pbox.inp", userdir);
     if ( (fp=fopen(fname, "w")) != NULL )
       {                                                                        
         int ret __attribute__((unused));
         fprintf(fp, "\n { %s %10.1f %10.1f } ", hshape, dwidth, freqshift);
         fprintf(fp, "\n name = %s   sucyc = d   type = d   attn = e ", SNAME);
         fprintf(fp, "\n RF Calibration Data: ref_pwr = %4d  ref_pw90 = %5.1f",
						        (int)(tpwr), pw*1.0e6);
         fclose(fp);
         sprintf(fs,"cd $vnmruser/shapelib; Pbox > /tmp/${LOGNAME}_bionmrfile");  
         ret = system(fs);                                                            
       /*printf("\nFreq Shift = %5.0f  Band Width = %10.0f  File Name:%s", 
					      freqshift, dwidth, SNAME);*/
       }
     else          { printf("\nUnable to write Pbox.inp file !");  psg_abort(1); }
   }
}

/* xxxxxxxxxxxxxxxxxxxxxxxxxxxx    h1decon    xxxxxxxxxxxxxxxxxxxxxxxxxxxxxx */

void h1decon(char *h1shape, double decbandwidth, double shift)
{
 shape 		decshape;
 double  	compH  = getval("compH");

/* GENERATE DECOUPLING SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
   h1decfiles(h1shape, decbandwidth, shift);

/* GET SHAPE FILE AND TURN ON DECOUPLING */
   decshape = getDshapeinfo(SNAME);
   obspower(decshape.pwr);
   if (decshape.pwrf)   { obspwrf(decshape.pwrf*compH); }
   else     		{ obspwrf(4095.0*compH); }
#ifndef NVPSG
   obsprgon(decshape.name, 1.0/decshape.dmf, decshape.dres);
   xmtron();	     				   	 /* turn on decoupling*/
#else
   xmtron();	     				   	 /* turn on decoupling*/
   obsprgon(decshape.name, 1.0/decshape.dmf, decshape.dres);
#endif
 }

/* xxxxxxxxxxxxxxxxxxxxxxxxxxxx    h1decoff    xxxxxxxxxxxxxxxxxxxxxxxxxxxxx */

void h1decoff()
{ 
#ifndef NVPSG
  xmtroff();         obsprgoff();	     	       /* turn off decoupling */
#else
  obsprgoff();	     xmtroff();           	       /* turn off decoupling */
#endif
  obspwrf(4095.0);  obspower(tpwr);
}


/* xxxxxxxxxxxxxxxxxxxxxxxxxxxx     h1decpw     xxxxxxxxxxxxxxxxxxxxxxxxxxxxx */
/* RETURN LENGTH OF ONE DECOUPLING CYCLE TO PULSE SEQUENCE */

double h1decpw(char *h1shape, double decbandwidth, double shift)
{
   shape 	decshape;

   h1decfiles(h1shape, decbandwidth, shift);           /* generate shape name */
   decshape = getDshapeinfo(SNAME);		    /* return length of cycle */
   return(decshape.pw);
}


/* xxxxxxxxxxxxxxxxxxxxxxxxxxxx     h1dec90pw   xxxxxxxxxxxxxxxxxxxxxxxxxxxxx */
/* RETURN LENGTH OF 90 DEGREE PULSE TO PULSE SEQUENCE */

double h1dec90pw(char *h1shape, double decbandwidth, double shift)
{
   shape 	decshape;

   h1decfiles(h1shape, decbandwidth, shift);           /* generate shape name */
   decshape = getDshapeinfo(SNAME);
   return(1.0/decshape.dmf);		       /* return 90 degree pulse time */
}


/* xxxxxxxxxxxxxxxxxxxxxxxxxxxx    h1decouple    xxxxxxxxxxxxxxxxxxxxxxxxxxx */
/* A DEFINED LENGTH OF h1 DECOUPLING. COMBINES h1decon, h1decpw, h1decoff */

void h1decouple(char *h1shape, double decbandwidth, double shift, double ncyc_del)
{
 shape 		decshape;

   h1decon(h1shape, decbandwidth, shift);               /* turn on decoupling */
   if (ncyc_del < 1.0)
  	  	 delay(ncyc_del);	      /* decoupling for set time < 1s */
   else 	{ncyc_del = (int) (ncyc_del + 0.5);
		 decshape = getDshapeinfo(SNAME);
   		 delay(ncyc_del*decshape.pw);}      /* decoupling for ncycles */
   h1decoff();	     				   	/* turn off decoupling*/
 }

/* xxxxxxxxxxxxxxxxxxxxxxxxxxxx    h1waltzon    xxxxxxxxxxxxxxxxxxxxxxxxxxxxx */

void h1waltzon(char *h1shape, double decbandwidth, double shift)
{
 shape 		decshape;
 double  	compH  = getval("compH");

/* GENERATE DECOUPLING SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
   h1decfiles(h1shape, decbandwidth, shift);

/* GET SHAPE FILE AND TURN ON DECOUPLING */
   decshape = getDshapeinfo(SNAME);
   obspower(decshape.pwr);
   if (decshape.pwrf)   { obspwrf(decshape.pwrf*compH); }
   else     		{ obspwrf(4095.0*compH); }
   txphase(one);      rgpulse(1.0/decshape.dmf, one, 2.0e-6, 0.0);
   txphase(zero);     delay(2.0e-6);
#ifndef NVPSG
   obsprgon(decshape.name, 1.0/decshape.dmf, decshape.dres);
   xmtron();	     				   	 /* turn on decoupling*/
#else
   xmtron();	     				   	 /* turn on decoupling*/
   obsprgon(decshape.name, 1.0/decshape.dmf, decshape.dres);
#endif
 }

/* xxxxxxxxxxxxxxxxxxxxxxxxxxxx    h1waltzoff    xxxxxxxxxxxxxxxxxxxxxxxxxxxx */

void h1waltzoff(char *h1shape, double decbandwidth, double shift)
{
 shape 		decshape;

/* GENERATE DECOUPLING SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
   h1decfiles(h1shape, decbandwidth, shift);

/* GET SHAPE FILE AND TURN ON DECOUPLING */
   decshape = getDshapeinfo(SNAME);
#ifndef NVPSG
   xmtroff();         
   obsprgoff();	     	       /* turn off decoupling */
#else
   obsprgoff();	     	       /* turn off decoupling */
   xmtroff();         
#endif
   txphase(three);      rgpulse(1.0/decshape.dmf, three, 2.0e-6, 0.0);
   obspwrf(4095.0);  obspower(tpwr);
}




/* xxxxxxxxxxxxxxxxxxxxxxx     END H1 DECOUPLING      xxxxxxxxxxxxxxxxxxxxxxx */







/* xxxxxxxx   INSTALLATION OF ANY DECOUPLING SHAPES IN SHAPELIB   xxxxxxxxxxx */
/* THE DECSHAPE NAME PROVIDED MUST EXIST IN WAVELIB */

void installdecshape(char *decshape)
{
 FILE 		*fp;
 char  		fs[MAXSTR], fname[MAXSTR];

/*   MAKE DEC SHAPE FILE NAME    */
 (void) sprintf(SNAME, "%s.DEC", decshape);

/*   GENERATE DECOUPLING SHAPE FILE IF REQUIRED   */
 (void) sprintf(fname, "%s/shapelib/%s", userdir, SNAME);
 if ((d2_index == 0) && (d3_index == 0))
   {
     (void) sprintf(fname, "%s/shapelib/Pbox.inp", userdir);
     if ( (fp=fopen(fname, "w")) != NULL )
       {                                                                        
         int ret __attribute__((unused));
         fprintf(fp, "\n { %s } ", decshape);
         fprintf(fp, "\n name = %s   sucyc = d   type = d  attn = e ", SNAME);
         fclose(fp);
         sprintf(fs,"cd $vnmruser/shapelib; Pbox > /tmp/${LOGNAME}_bionmrfile");
	 ret = system(fs);                        
       }
     else          { printf("\nUnable to write Pbox.inp file !");  psg_abort(1); }
   }
}






/*  xxxxxxxxxxxxx   TRIPLE RESONANCE NH EVOLUTION & SE TRAIN   xxxxxxxxxxxxx  */

/*  xxxxxxxxxxxxxxxxxxxxxxxx   nh_evol_se_train   xxxxxxxxxxxxxxxxxxxxxxxxxx  */
/*  for g...._nh pulse sequences  */

void nh_evol_se_train(char *excband, char *nullband)
{
/* DECLARE AND LOAD VARIABLES */
char	    c1shape[MAXSTR], c2shape[MAXSTR],

	    mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
 	    TROSY[MAXSTR];			    /* do TROSY on N15 and H1 */

double	    timeTN = getval("timeTN"),     /* constant time for 15N evolution */
 
   	pwS9,					  /* length of last C13 pulse */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
	dpwr2 = getval("dpwr2"),		  /* power for N15 decoupling */

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
	gzcal  = getval("gzcal"),            /* g/cm to DAC conversion factor */
        gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt3 = getval("gt3"),
	gt5 = getval("gt5"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6");

    getstr("mag_flg",mag_flg);
    getstr("TROSY",TROSY);

/*   INITIALIZE VARIABLES   */
    if (! strcmp(excband, "co"))
	  { strcpy(c1shape, "sinc"); strcpy(c2shape, "square"); }
    else  { strcpy(c1shape, "square"); strcpy(c2shape, "sinc"); }

    pwS9 = c13pulsepw(nullband, excband, c2shape, 180.0); 


/*  xxxxx  LAST COMMON HALF OF PULSE SEQUENCE FOLLOWS  xxxxx  */
/*  xxxxx  OPTIONS FOR N15 EVOLUTION  xxxxxx  */

	c13pulse(excband, nullband, c1shape, 90.0, zero, 2.0e-6, 0.0);

    	if ( ((! strcmp(excband, "ca")) || (! strcmp(excband, "cab"))) &&
	      (dm3[B] == 'y') )		         /*optional 2H decoupling off */
           {dec3rgpulse(1/dmf3, three, 0.0, 0.0); dec3blank();
            setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3); dec3blank();}
	dcplrphase(zero);
	zgradpulse(gzlvl3, gt3);
        if (TROSY[A]=='y') { h1decoff(); }
     	delay(2.0e-4);
	dec2rgpulse(pwN, t8, 0.0, 0.0);

	decphase(zero);
	dec2phase(t9);
	delay(timeTN - WFG3_START_DELAY - tau2);
							 /* WFG3_START_DELAY  */
	sim3_c13pulse("", excband, nullband, c1shape, "", 0.0, 180.0, 2.0*pwN, 
						zero, zero, t9, 2.0e-6, 2.0e-6);

	dec2phase(t10);

if (TROSY[A]=='y')
{
    if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
	{
	  txphase(t4);
          delay(timeTN - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY
						 - 2.0*PWRF_DELAY - 2.0e-6);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')  magradpulse(icosel*gzcal*gzlvl1, gt1);
          else  zgradpulse(icosel*gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4);
	}
    else
	{
	  txphase(t4);
          delay(timeTN - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY
 	      - 2.0*PWRF_DELAY - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else  zgradpulse(icosel*gzlvl1, gt1);	   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4);      			           /* WFG_START_DELAY */
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2);
	}
}
else
{
    if (tau2 > kappa + PRG_STOP_DELAY)
	{
          delay(timeTN - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY 
						- 2.0*PWRF_DELAY - 2.0e-6);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2 - kappa - PRG_STOP_DELAY - POWER_DELAY - PWRF_DELAY);
          h1decoff();		     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
	  txphase(t4);
          delay(kappa - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else    zgradpulse(icosel*gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4);
	}
    else if (tau2 > (kappa - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY - 2.0e-6))
	{
          delay(timeTN + tau2 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
          h1decoff();		     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
	  txphase(t4); 			/* WFG_START_DELAY  + 2.0*POWER_DELAY */
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(kappa - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY - 1.0e-6 - gt1 
					        - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else    zgradpulse(icosel*gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4);
	}
    else if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
	{
          delay(timeTN + tau2 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
          h1decoff();		     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
	  txphase(t4);
          delay(kappa - tau2 - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY
 								    - 2.0e-6);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else    zgradpulse(icosel*gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4);
	}
    else
	{
          delay(timeTN + tau2 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
          h1decoff();		     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
	  txphase(t4);
    	  delay(kappa - tau2 - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY
			         - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else    zgradpulse(icosel*gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2);
	}
}
/* xxxxxxxxx  SE TRAIN  xxxxxxxxx */
	if (TROSY[A]=='y')  rgpulse(pw, t4, 0.0, 0.0);
	else                sim3pulse(pw, 0.0, pwN, t4, zero, t10, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl5, gt5);
	if (TROSY[A]=='y')   delay(lambda - 0.65*(pw + pwN) - gt5);
	else   		     delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	dec2phase(t11);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(pw, 0.0, pwN, one, zero, t11, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl6, gt5);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	dec2phase(t10);
	zgradpulse(gzlvl6, gt5);

	if (TROSY[A]=='y')
	     {delay(lambda - 1.3*pwN - gt5);
	      dec2rgpulse(pwN, t10, 0.0, 0.0);
	      delay((gt1/10.0) + 1.0e-4 - 0.3*pwN + 2.0*GRADIENT_DELAY
		 					+ POWER_DELAY);  }
	else {delay(lambda - 0.65*(pw + pwN) - gt5);
	      rgpulse(pw, zero, 0.0, 0.0); 
	      delay((gt1/10.0) + 1.0e-4 - 0.3*pw + 2.0*GRADIENT_DELAY
							+ POWER_DELAY*2.0);  }
	rgpulse(2.0*pw, zero, 0.0, 0.0);

	dec2power(dpwr2);				       /* POWER_DELAY */
	decpower(dpwr);				       /* POWER_DELAY */
        if (mag_flg[A] == 'y')    magradpulse(gzcal*gzlvl2, gt1/10.0);
        else   zgradpulse(gzlvl2, gt1/10.0);            /* 2.0*GRADIENT_DELAY */

	rcvron();
	statusdelay(C, 1.0e-4);
	setreceiver(t12);
}	



 
/*  xxxxxxxxxxxxxxxxxxxxxxxx   hn_evol_se_train   xxxxxxxxxxxxxxxxxxxxxxxxxx  */
/*  for ghn.... pulse sequences  */

void hn_evol_se_train(char *excband, char *nullband)
{
/* DECLARE AND LOAD VARIABLES */
char	    c1shape[MAXSTR], c2shape[MAXSTR],

	    mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
 	    TROSY[MAXSTR];			    /* do TROSY on N15 and H1 */

double	timeTN = getval("timeTN"),	   /* constant time for 15N evolution */
  	pwS9,					  /* length of last C13 pulse */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
	dpwr2 = getval("dpwr2"),		  /* power for N15 decoupling */

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
	gzcal  = getval("gzcal"),            /* g/cm to DAC conversion factor */
        gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt4 = getval("gt4"),				   /* other gradients */
	gt5 = getval("gt5"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6");

    getstr("mag_flg",mag_flg);
    getstr("TROSY",TROSY);

/*   INITIALIZE VARIABLES   */
    if (! strcmp(excband, "co"))
	  { strcpy(c1shape, "sinc"); strcpy(c2shape, "square"); }
    else  { strcpy(c1shape, "square"); strcpy(c2shape, "sinc"); }

    pwS9 = c13pulsepw(nullband, excband, c2shape, 180.0); 

/*  xxxxx  LAST COMMON HALF OF PULSE SEQUENCE FOLLOWS  xxxxx  */
/*  xxxxx  OPTIONS FOR N15 EVOLUTION  xxxxxx  */

	dcplrphase(zero);
	dec2phase(t8);
	zgradpulse(gzlvl4, gt4);
 	delay(2.0e-4);
        if (TROSY[A]=='n')      h1waltzon("WALTZ16", widthHd, 0.0);
	dec2rgpulse(pwN, t8, 0.0, 0.0);

	decphase(zero);
	dec2phase(t9);
	delay(timeTN - WFG3_START_DELAY - tau2);
							 /* WFG3_START_DELAY  */
	sim3_c13pulse("", excband, nullband, c1shape, "", 0.0, 180.0, 2.0*pwN, 
						zero, zero, t9, 2.0e-6, 2.0e-6);

	dec2phase(t10);

if (TROSY[A]=='y')
{    if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.5e-4 + pwHs)
	{
	  txphase(three);
          delay(timeTN - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY
						- 2.0*PWRF_DELAY - 2.0e-6);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2 - gt1 - 2.0*GRADIENT_DELAY - 1.5e-4 - pwHs);
          if (mag_flg[A]=='y')  magradpulse(icosel*gzcal*gzlvl1, gt1);
          else  zgradpulse(icosel*gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4 - POWER_DELAY - PWRF_DELAY);
   	  shiftedpulse("sinc", pwHs, 90.0, 0.0, three, 2.0e-6, 0.0);
	  txphase(t4);
	  delay(0.5e-4 - POWER_DELAY - PWRF_DELAY - WFG_START_DELAY - 2.0e-6);
	}

    else if (tau2 > pwHs + 0.5e-4)
	{
	  txphase(three);
          delay(timeTN - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY
		- 2.0*PWRF_DELAY - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else  zgradpulse(icosel*gzlvl1, gt1);	   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4 - POWER_DELAY - PWRF_DELAY);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2 - pwHs - 0.5e-4);
    	  shiftedpulse("sinc", pwHs, 90.0, 0.0, three, 2.0e-6, 0.0);
	  txphase(t4);
	  delay(0.5e-4 - POWER_DELAY - PWRF_DELAY - WFG_START_DELAY - 2.0e-6);
	}
    else
	{
	  txphase(three);
          delay(timeTN - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY
	  - 2.0*PWRF_DELAY - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - 1.5e-4 - pwHs);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else  zgradpulse(icosel*gzlvl1, gt1);	   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4 - POWER_DELAY - PWRF_DELAY);
    	  shiftedpulse("sinc", pwHs, 90.0, 0.0, three, 2.0e-6, 0.0);
	  txphase(t4);
	  delay(0.5e-4 - POWER_DELAY - PWRF_DELAY - WFG_START_DELAY - 2.0e-6);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2);
	}
}
else
{
    if (tau2 > (kappa + PRG_STOP_DELAY + pwHd + 2.0e-6))
	{
          delay(timeTN - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY 
	  					- 2.0*PWRF_DELAY - 2.0e-6);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6 - POWER_DELAY
								 - PWRF_DELAY);
	  h1waltzoff("WALTZ16", widthHd, 0.0);
	  txphase(t4);
          delay(kappa - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else    zgradpulse(icosel*gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4);
	}
    else if (tau2 > (kappa - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY 
	  					- 2.0*PWRF_DELAY - 2.0e-6))
	{
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6
 						  - POWER_DELAY - PWRF_DELAY);
	  h1waltzoff("WALTZ16", widthHd, 0.0);
	  txphase(t4);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(kappa - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY 
	  	- 2.0*PWRF_DELAY - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else    zgradpulse(icosel*gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4);
	}
    else if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
	{
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6
 						  - POWER_DELAY - PWRF_DELAY);
	  h1waltzoff("WALTZ16", widthHd, 0.0);
	  txphase(t4);
          delay(kappa - tau2 - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY 
	  					   - 2.0*PWRF_DELAY - 2.0e-6);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else    zgradpulse(icosel*gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4);
	}
    else
	{
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6
 						  - POWER_DELAY - PWRF_DELAY);
	  h1waltzoff("WALTZ16", widthHd, 0.0);
	  txphase(t4);
    	  delay(kappa - tau2 - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY 
	  	- 2.0*PWRF_DELAY - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else    zgradpulse(icosel*gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2);
	}
}
/* xxxxxxxxx  SE TRAIN  xxxxxxxxx */
	if (TROSY[A]=='y')  rgpulse(pw, t4, 0.0, 0.0);
	else                sim3pulse(pw, 0.0, pwN, t4, zero, t10, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl5, gt5);
	if (TROSY[A]=='y')   delay(lambda - 0.65*(pw + pwN) - gt5);
	else   		     delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	dec2phase(t11);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(pw, 0.0, pwN, one, zero, t11, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl6, gt5);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	dec2phase(t10);
	zgradpulse(gzlvl6, gt5);

	if (TROSY[A]=='y')
	     {delay(lambda - 1.3*pwN - gt5);
	      dec2rgpulse(pwN, t10, 0.0, 0.0);
	      delay((gt1/10.0) + 1.0e-4 - 0.3*pwN + 2.0*GRADIENT_DELAY
		 					+ POWER_DELAY);  }
	else {delay(lambda - 0.65*(pw + pwN) - gt5);
	      rgpulse(pw, zero, 0.0, 0.0); 
	      delay((gt1/10.0) + 1.0e-4 - 0.3*pw + 2.0*GRADIENT_DELAY
							+ POWER_DELAY*2.0);  }
	rgpulse(2.0*pw, zero, 0.0, 0.0);

	dec2power(dpwr2);				       /* POWER_DELAY */
	decpower(dpwr);				       /* POWER_DELAY */
        if (mag_flg[A] == 'y')    magradpulse(gzcal*gzlvl2, gt1/10.0);
        else   zgradpulse(gzlvl2, gt1/10.0);            /* 2.0*GRADIENT_DELAY */

	rcvron();
	statusdelay(C, 1.0e-4);
	setreceiver(t12);
}		 
 
/*  xxxxxxxxxxxxxxxxxxxxxxxx   NEW! NEW! NEW! NEW!  xxxxxxxxxxxxxxxxxxxxxxxx  */
/*  xxxxxxxxxxxxxxxxxxxxxxxx   hn_evol_se_train_bb  xxxxxxxxxxxxxxxxxxxxxxxx  */
/*  xxxxxxxxxxxxxxxxxxxxxxxx   NEW! NEW! NEW! NEW!  xxxxxxxxxxxxxxxxxxxxxxxx  */

void hn_evol_se_train_bb()
{
/* DECLARE AND LOAD VARIABLES */

char        mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
            TROSY[MAXSTR];                          /* do TROSY on N15 and H1 */

double  timeTN = getval("timeTN"),         /* constant time for 15N evolution */
        pwS9,pwS10,                             /* length of C13 pulses */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
        dpwr2 = getval("dpwr2"),                  /* power for N15 decoupling */

        gt1 = getval("gt1"),                   /* coherence pathway gradients */
        gzcal  = getval("gzcal"),            /* g/cm to DAC conversion factor */
        gzlvl1 = getval("gzlvl1"),
        gzlvl2 = getval("gzlvl2"),

        gt4 = getval("gt4"),                               /* other gradients */
        gt5 = getval("gt5"),
        gzlvl4 = getval("gzlvl4"),
        gzlvl5 = getval("gzlvl5"),
        gzlvl6 = getval("gzlvl6");

    getstr("mag_flg",mag_flg);
    getstr("TROSY",TROSY);

/*   INITIALIZE VARIABLES   */

    pwS9 = c13pulsepw("co", "ca", "sinc", 180.0);
    pwS10 = c13pulsepw("ca", "co", "square", 180.0);
/*  xxxxx  LAST COMMON HALF OF PULSE SEQUENCE FOLLOWS  xxxxx  */
/*  xxxxx  OPTIONS FOR N15 EVOLUTION  xxxxxx  */

        dcplrphase(zero);
        dec2phase(t8);
        zgradpulse(gzlvl4, gt4);
        delay(2.0e-4);
        if (TROSY[A]=='n')      h1waltzon("WALTZ16", widthHd, 0.0);
        dec2rgpulse(pwN, t8, 0.0, 0.0);

        decphase(zero);
        dec2phase(t9);
        delay(timeTN - WFG3_START_DELAY - tau2 + pwS10);
                                                         /* WFG3_START_DELAY  */
        sim3_c13pulse("", "co", "ca", "sinc", "", 0.0, 180.0, 2.0*pwN,
                                                zero, zero, t9, 2.0e-6, 2.0e-6);
       c13pulse("ca", "co", "square", 180.0, zero, 2.0e-6, 0.0);

        dec2phase(t10);
if (TROSY[A]=='y')
{    if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.5e-4 + pwHs)
        {
          txphase(three);
          delay(timeTN - WFG_START_DELAY - 2.0*POWER_DELAY
                                                - 2.0*PWRF_DELAY - 2.0e-6);
          delay(tau2 - gt1 - 2.0*GRADIENT_DELAY - 1.5e-4 - pwHs);
          if (mag_flg[A]=='y')  magradpulse(icosel*gzcal*gzlvl1, gt1);
          else  zgradpulse(icosel*gzlvl1, gt1);         /* 2.0*GRADIENT_DELAY */
          delay(1.0e-4 - POWER_DELAY - PWRF_DELAY);
          shiftedpulse("sinc", pwHs, 90.0, 0.0, three, 2.0e-6, 0.0);
          txphase(t4);
          delay(0.5e-4 - POWER_DELAY - PWRF_DELAY - WFG_START_DELAY - 2.0e-6);
        }

    else if (tau2 > pwHs + 0.5e-4)
        {
          txphase(three);
          delay(timeTN - WFG_START_DELAY - 2.0*POWER_DELAY
                - 2.0*PWRF_DELAY - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else  zgradpulse(icosel*gzlvl1, gt1);         /* 2.0*GRADIENT_DELAY */
          delay(1.0e-4 - POWER_DELAY - PWRF_DELAY);
          delay(tau2 - pwHs - 0.5e-4);
          shiftedpulse("sinc", pwHs, 90.0, 0.0, three, 2.0e-6, 0.0);
          txphase(t4);
          delay(0.5e-4 - POWER_DELAY - PWRF_DELAY - WFG_START_DELAY - 2.0e-6);
        }
    else
        {
          txphase(three);
          delay(timeTN  - WFG_START_DELAY - 2.0*POWER_DELAY
          - 2.0*PWRF_DELAY - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - 1.5e-4 - pwHs);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else  zgradpulse(icosel*gzlvl1, gt1);         /* 2.0*GRADIENT_DELAY */
          delay(1.0e-4 - POWER_DELAY - PWRF_DELAY);
          shiftedpulse("sinc", pwHs, 90.0, 0.0, three, 2.0e-6, 0.0);
          txphase(t4);
          delay(0.5e-4 - POWER_DELAY - PWRF_DELAY - WFG_START_DELAY - 2.0e-6);
          delay(tau2);
        }
}
else
{
    if (tau2 > (kappa + PRG_STOP_DELAY + pwHd + 2.0e-6))
        {
          delay(timeTN - WFG_START_DELAY - 2.0*POWER_DELAY
                                                - 2.0*PWRF_DELAY - 2.0e-6);
          delay(tau2 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6 - POWER_DELAY
                                                                 - PWRF_DELAY);
          h1waltzoff("WALTZ16", widthHd, 0.0);
          txphase(t4);
          delay(kappa - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else    zgradpulse(icosel*gzlvl1, gt1);       /* 2.0*GRADIENT_DELAY */
          delay(1.0e-4);
        }
    else if (tau2 > (kappa - WFG_START_DELAY - 2.0*POWER_DELAY
                                                - 2.0*PWRF_DELAY - 2.0e-6))
        {
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6
                                                  - POWER_DELAY - PWRF_DELAY);
          h1waltzoff("WALTZ16", widthHd, 0.0);
          txphase(t4);
          delay(kappa - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY
                - 2.0*PWRF_DELAY - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else    zgradpulse(icosel*gzlvl1, gt1);       /* 2.0*GRADIENT_DELAY */
          delay(1.0e-4);
        }
    else if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
        {
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6
                                                  - POWER_DELAY - PWRF_DELAY);
          h1waltzoff("WALTZ16", widthHd, 0.0);
          txphase(t4);
          delay(kappa - tau2  - WFG_START_DELAY - 2.0*POWER_DELAY
                                                   - 2.0*PWRF_DELAY - 2.0e-6);
          delay(tau2 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else    zgradpulse(icosel*gzlvl1, gt1);       /* 2.0*GRADIENT_DELAY */
          delay(1.0e-4);
        }
    else
        {
          delay(timeTN + tau2 - kappa - PRG_STOP_DELAY - pwHd - 2.0e-6
                                                  - POWER_DELAY - PWRF_DELAY);
          h1waltzoff("WALTZ16", widthHd, 0.0);
          txphase(t4);
          delay(kappa - tau2  - WFG_START_DELAY - 2.0*POWER_DELAY
                - 2.0*PWRF_DELAY - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else    zgradpulse(icosel*gzlvl1, gt1);       /* 2.0*GRADIENT_DELAY */
          delay(1.0e-4);
          delay(tau2);
        }
}
/* xxxxxxxxx  SE TRAIN  xxxxxxxxx */
        if (TROSY[A]=='y')  rgpulse(pw, t4, 0.0, 0.0);
        else                sim3pulse(pw, 0.0, pwN, t4, zero, t10, 0.0, 0.0);

        txphase(zero);
        dec2phase(zero);
        zgradpulse(gzlvl5, gt5);
        if (TROSY[A]=='y')   delay(lambda - 0.65*(pw + pwN) - gt5);
        else                 delay(lambda - 1.3*pwN - gt5);

        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

        zgradpulse(gzlvl5, gt5);
        txphase(one);
        dec2phase(t11);
        delay(lambda - 1.3*pwN - gt5);

        sim3pulse(pw, 0.0, pwN, one, zero, t11, 0.0, 0.0);

        txphase(zero);
        dec2phase(zero);
        zgradpulse(gzlvl6, gt5);
        delay(lambda - 1.3*pwN - gt5);

        sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

        dec2phase(t10);
        zgradpulse(gzlvl6, gt5);

        if (TROSY[A]=='y')
             {delay(lambda - 1.3*pwN - gt5);
              dec2rgpulse(pwN, t10, 0.0, 0.0);
              delay((gt1/10.0) + 1.0e-4 - 0.3*pwN + 2.0*GRADIENT_DELAY
                                                        + POWER_DELAY);  }
        else {delay(lambda - 0.65*(pw + pwN) - gt5);
              rgpulse(pw, zero, 0.0, 0.0);
              delay((gt1/10.0) + 1.0e-4 - 0.3*pw + 2.0*GRADIENT_DELAY
                                                        + POWER_DELAY*2.0);  }
        rgpulse(2.0*pw, zero, 0.0, 0.0);

        dec2power(dpwr2);                                      /* POWER_DELAY */
        decpower(dpwr);                                      /* POWER_DELAY */
        if (mag_flg[A] == 'y')    magradpulse(gzcal*gzlvl2, gt1/10.0);
        else   zgradpulse(gzlvl2, gt1/10.0);            /* 2.0*GRADIENT_DELAY */

        rcvron();
        statusdelay(C, 1.0e-4);
        setreceiver(t12);
}

/********************************************************************************/
/**************   NEW functions (IBS)  ******************************************/
/********************************************************************************/

/************** shaped pulse on 1H channel **************************************/

/* xxxxxxxxx   MAKE SHAPE WITH PBOX AND PLACE IN USERDIR/SHAPELIB   xxxxxxxxx */

void hshapefiles(char *anyshape, double bw, double shift)
{
 FILE		*fp;
 char 	    fshape[MAXSTR], fs[MAXSTR], fname[MAXSTR];
 extern char	userdir[];
 double    freqshift,dwidth;
 
 /* DETERMINE FREQUENCY SHIFT AND BANDWIDTH in Hz */
 freqshift = shift*sfrq;
 dwidth = bw*sfrq;



/*    MAKE SHAPE FILE NAME   */  
 if ( ! strcmp(anyshape, "") )      strcpy(fshape, "square"); 
 else  				 strcpy(fshape, anyshape); 
(void) sprintf(SNAME, "pp.%s.%d.%d.RF", fshape,
                       (int)(bw+0.5), (int)(shift+0.5) );


 /*   GENERATE SHAPE FILE IF REQUIRED   */
 (void) sprintf(fname, "%s/shapelib/%s", userdir, SNAME);
 if ((d2_index == 0) && (d3_index == 0))
  {
     (void) sprintf(fname, "%s/shapelib/Pbox.inp", userdir);
     if ( (fp=fopen(fname, "w")) != NULL )
       {
         int ret __attribute__((unused));
         fprintf(fp, "\n Pbox Input File ");
         fprintf(fp, "\n { %s %14.8f %12.1f } ", fshape, dwidth, freqshift);
         fprintf(fp, "\n name = %s    sucyc = n", SNAME);
         fprintf(fp, "\n RF Calibration Data: ref_pwr = %4d  ref_pw90 = %5.1f", 
					(int)(REF_PWR), REF_PW90*1e6);
         fclose(fp);
         sprintf(fs,"cd $vnmruser/shapelib; Pbox > /tmp/${LOGNAME}_bionmrfile");
         ret = system(fs);

       }
     else          { printf("\nUnable to write Pbox.inp file !");  psg_abort(1); }
  }
}


/* xxxxxxxxxxxxxxxxxxxxxxxxx     H shaped pulse     xxxxxxxxxxxxxxxxxxxxxxxxx   */
/*    parameters: anyshape: type of shape                                       */
/*                bw : band width (in ppm)                                      */
/*		  shift: frequency shift with respect to 1H carrier (in ppm)    */
/********************************************************************************/

void h_shapedpulse(char *anyshape, double bw, double shift,
                   codeint phase1, double rofa, double rofb) 
{
 shape 		pwshape;
 double 	compH = getval("compH");
                
                

		REF_PW90 = pw;
		REF_PWR = tpwr;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
hshapefiles(anyshape, bw, shift);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwshape = getRshapeinfo(SNAME);
 obspower(pwshape.pwr);
 if (pwshape.pwrf)      obspwrf(pwshape.pwrf*compH);
 else			obspwrf(4095.0);
 shaped_pulse(pwshape.name,pwshape.pw,phase1,rofa,rofb);
 obspwrf(4095.0);
 obspower(tpwr);
}


/* determine pulse length of n_shapedpulse */
double h_shapedpw(char *anyshape, double bw, double shift,
                  codeint phase1, double rofa, double rofb) 
{
 shape 		pwshape;

		REF_PW90 = pw;
		REF_PWR = tpwr;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
hshapefiles(anyshape, bw, shift);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwshape = getRshapeinfo(SNAME);
 return(pwshape.pw);
}
/************** shaped pulse on 13C (dec) channel **************************************/

/* xxxxxxxxx   MAKE SHAPE WITH PBOX AND PLACE IN USERDIR/SHAPELIB   xxxxxxxxx */

void cshapefiles(char *anyshape, double bw, double shift)
{
 FILE		*fp;
 char 	    fshape[MAXSTR], fs[MAXSTR], fname[MAXSTR];
 extern char	userdir[];
 double    freqshift,dwidth;
 
 /* DETERMINE FREQUENCY SHIFT AND BANDWIDTH */
 freqshift = shift*dfrq;
 dwidth = bw*dfrq;



/*    MAKE SHAPE FILE NAME   */  
 if ( ! strcmp(anyshape, "") )      strcpy(fshape, "square"); 
 else  				 strcpy(fshape, anyshape); 
(void) sprintf(SNAME_C, "pp.%s.%d.%d.RF", fshape,
                         (int)(bw+0.5), (int)(shift+0.5) );


 /*   GENERATE SHAPE FILE IF REQUIRED   */
 (void) sprintf(fname, "%s/shapelib/%s", userdir, SNAME_C);
 if ((d2_index == 0) && (d3_index == 0))
  {
     (void) sprintf(fname, "%s/shapelib/Pbox.inp", userdir);
     if ( (fp=fopen(fname, "w")) != NULL )
       {
         int ret __attribute__((unused));
         fprintf(fp, "\n Pbox Input File ");
         fprintf(fp, "\n { %s %14.8f %12.1f } ", fshape, dwidth, 
								  freqshift);
         fprintf(fp, "\n name = %s    sucyc = n", SNAME_C);
         fprintf(fp, "\n RF Calibration Data: ref_pwr = %4d  ref_pw90 = %5.1f", 
					(int)(REF_PWR_C), REF_PW90_C*1e6);
         fclose(fp);
         sprintf(fs,"cd $vnmruser/shapelib; Pbox > /tmp/${LOGNAME}_bionmrfile");
         ret = system(fs);
        } 
     else          { printf("\nUnable to write Pbox.inp file !");  psg_abort(1); }
  }
}


/* xxxxxxxxxxxxxxxxxxxxxxxxx     C shaped pulse     xxxxxxxxxxxxxxxxxxxxxxxx   */
/*   creates shape file for irradiation of a 13C band                          */
/*    parameters: anyshape: type of shape                                       */
/*                bw : band width (in ppm)                                      */
/*		  shift: frequency shift with respect to 1C carrier (in ppm)    */
/********************************************************************************/

void c_shapedpulse(char *anyshape, double bw, double shift,
                   codeint phase1, double rofa, double rofb) 
{
 shape 		pwshape;
 double 	compC = getval("compC"),
                pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl");
                
                

		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles(anyshape, bw, shift);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwshape = getRshapeinfo(SNAME_C);
 decpower(pwshape.pwr);
 if (pwshape.pwrf)      decpwrf(pwshape.pwrf*compC);
 else			decpwrf(4095.0);
 decshaped_pulse(pwshape.name,pwshape.pw,phase1,rofa,rofb);
 decpwrf(4095.0);
 decpower(pwClvl);
}
/*  determine pulse length of  c_shapedpulse  */

double c_shapedpw(char *anyshape, double bw, double shift,
                  codeint phase1, double rofa, double rofb) 
{
 shape 		pwshape;
 double 	pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl");
                

		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles(anyshape, bw, shift);


/* GET SHAPE FILE & EXECUTE PULSE */

 pwshape = getRshapeinfo(SNAME_C);
 return(pwshape.pw);
}


/************** shaped pulse on 13C (dec) channel (2 bands) **************************************/

/* xxxxxxxxx   MAKE SHAPE WITH PBOX AND PLACE IN USERDIR/SHAPELIB   xxxxxxxxx */

void cshapefiles2(char *anyshape1, char *anyshape2, double bw1, double bw2,
                  double shift1, double shift2)
{
 FILE		*fp;
 char 	    fshape1[MAXSTR],fshape2[MAXSTR], fs[MAXSTR], fname[MAXSTR];
 extern char	userdir[];
 double    freqshift1,dwidth1,freqshift2,dwidth2;
 
 /* DETERMINE FREQUENCY SHIFT AND BANDWIDTH */
 freqshift1 = shift1*dfrq;
 dwidth1 = bw1*dfrq;
 freqshift2 = shift2*dfrq;
 dwidth2 = bw2*dfrq;


/*    MAKE SHAPE FILE NAME   */  
 if ( ! strcmp(anyshape1, "") )      strcpy(fshape1, "square"); 
 else  				 strcpy(fshape1, anyshape1);
  if ( ! strcmp(anyshape2, "") )      strcpy(fshape2, "square"); 
 else  				 strcpy(fshape2, anyshape2);  
(void) sprintf(SNAME_C, "pp.%s.%d.%d.%d.%d.RF", fshape1, (int)(bw1+0.5),
                         (int)(shift1+0.5), (int)(bw2+0.5), (int)(shift2+0.5) );


 /*   GENERATE SHAPE FILE IF REQUIRED   */
 (void) sprintf(fname, "%s/shapelib/%s", userdir, SNAME_C);
 if ((d2_index == 0) && (d3_index == 0))
  {
     (void) sprintf(fname, "%s/shapelib/Pbox.inp", userdir);
     if ( (fp=fopen(fname, "w")) != NULL )
       {
         int ret __attribute__((unused));
         fprintf(fp, "\n Pbox Input File ");
         fprintf(fp, "\n { %s %14.8f %12.1f } ", fshape1, dwidth1, 
								  freqshift1);
	 fprintf(fp, "\n { %s %14.8f %12.1f } ", fshape2, dwidth2, 
								  freqshift2);
         fprintf(fp, "\n name = %s    sucyc = n", SNAME_C);
         fprintf(fp, "\n RF Calibration Data: ref_pwr = %4d  ref_pw90 = %5.1f", 
					(int)(REF_PWR_C), REF_PW90_C*1e6);
         fclose(fp);
         sprintf(fs,"cd $vnmruser/shapelib; Pbox > /tmp/${LOGNAME}_bionmrfile");
         ret = system(fs);
       }
     else          { printf("\nUnable to write Pbox.inp file !");  psg_abort(1); }
  }
}


/* xxxxxxxxxxxxxxxxxxxxxxxxx     C shaped pulse  2   xxxxxxxxxxxxxxxxxxxxxxxx   */
/*   creates shape file for irradiation of two different 13C bands              */
/*    parameters: anyshape: type of shape                                       */
/*                bw : band width (in ppm)                                      */
/*		  shift: frequency shift with respect to 1H carrier (in ppm)    */
/********************************************************************************/

void c_shapedpulse2(char *anyshape1, double bw1, double shift1, char *anyshape2,
                    double bw2, double shift2, codeint phase1, double rofa, double rofb) 
{
 shape 		pwshape;
 double 	compC = getval("compC"),
                pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl");
                

		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles2(anyshape1, anyshape2, bw1, bw2, shift1, shift2);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwshape = getRshapeinfo(SNAME_C);
 decpower(pwshape.pwr);
 if (pwshape.pwrf)      decpwrf(pwshape.pwrf*compC);
 else			decpwrf(4095.0);
 decshaped_pulse(pwshape.name,pwshape.pw,phase1,rofa,rofb);
 decpwrf(4095.0);
 decpower(pwClvl);
}

/*  determine pulse length of  c_shapedpulse2  */

double c_shapedpw2(char *anyshape1, double bw1, double shift1, char *anyshape2,
                   double bw2, double shift2, codeint phase1, double rofa, double rofb) 
{
 shape 		pwshape;
 double 	pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl");
                

		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles2(anyshape1, anyshape2, bw1, bw2, shift1, shift2);


/* GET SHAPE FILE & EXECUTE PULSE */

 pwshape = getRshapeinfo(SNAME_C);
 return(pwshape.pw);
}

/* xxxxxxxxxxxxxxxxxxxxxxxxx     C shaped pulse  2   xxxxxxxxxxxxxxxxxxxxxxxx                */
/*   creates shape file for irradiation of a selective 13C bands and a non selective 15N band*/
/*    parameters for 13C selective pulse :                                                   */
/*                anyshape: type of shape                                                    */
/*                bw : band width (in ppm)                                                   */
/*		  shift: frequency shift with respect to carrier (in ppm)                    */
/*********************************************************************************************/

void c_simshapedpulse(char *anyshapeC, double bwC, double shiftC, double pwH, double pwN,
                      codeint phase1, codeint phase2, codeint phase3,
                      double rofa, double rofb) 
{
 shape 		pwshape;
 double 	compC = getval("compC"),
                pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl");
                

		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles(anyshapeC, bwC, shiftC);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwshape = getRshapeinfo(SNAME_C);
 decpower(pwshape.pwr);
 if (pwshape.pwrf)      decpwrf(pwshape.pwrf*compC);
 else			decpwrf(4095.0);
 sim3shaped_pulse("",pwshape.name,"",pwH,pwshape.pw,pwN,phase1,phase2,phase3, rofa, rofb);
 decpwrf(4095.0);
 decpower(pwClvl);
}

/* determine pulse length of c_simshapedpulse */

double c_simshapedpw(anyshapeC,bwC,shiftC,pwH,pwN,phase1,phase2,phase3, rofa, rofb) 

char 		*anyshapeC ;
double 		bwC,shiftC,pwH,pwN,rofa, rofb;
codeint 	phase1, phase2, phase3;
{
 shape 		pwshape;
 double         max_pw,
                pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl");
                

		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles(anyshapeC, bwC, shiftC);


/* GET SHAPE FILE PARAMETERS */
 pwshape = getRshapeinfo(SNAME_C);
 
 if (pwshape.pw > pwH) max_pw = pwshape.pw;
    else max_pw = pwH;
 if (pwN > max_pw) max_pw = pwN;
 
 return(max_pw);
}



/************** shaped pulse on 15N (dec2) channel ****************************/

/* xxxxxxxxx   MAKE SHAPE WITH PBOX AND PLACE IN USERDIR/SHAPELIB   xxxxxxxxx */

void nshapefiles(char *anyshape, double bw, double shift)
{
 FILE		*fp;
 char 	    fshape[MAXSTR], fs[MAXSTR], fname[MAXSTR];
 extern char	userdir[];
 double    freqshift,dwidth;
 
 /* DETERMINE FREQUENCY SHIFT AND BANDWIDTH */
 freqshift = shift*dfrq2;
 dwidth = bw*dfrq2;

/*    MAKE SHAPE FILE NAME   */  
 if ( ! strcmp(anyshape, "") )      strcpy(fshape, "square"); 
 else  				 strcpy(fshape, anyshape); 
(void) sprintf(SNAME_N, "pp.%s.%d.%d.RF", fshape,
                         (int)(bw+0.5), (int)(shift+0.5) );


 /*   GENERATE SHAPE FILE IF REQUIRED   */
 (void) sprintf(fname, "%s/shapelib/%s", userdir, SNAME_N);

 if ((d2_index == 0) && (d3_index == 0))
  {
     (void) sprintf(fname, "%s/shapelib/Pbox.inp", userdir);
     if ( (fp=fopen(fname, "w")) != NULL )
       {
         int ret __attribute__((unused));
         fprintf(fp, "\n Pbox Input File ");
         fprintf(fp, "\n { %s %14.8f %12.1f } ", fshape, dwidth, 
								  freqshift);
         fprintf(fp, "\n name = %s    sucyc = n", SNAME_N);
         fprintf(fp, "\n RF Calibration Data: ref_pwr = %4d  ref_pw90 = %5.1f", 
					(int)(REF_PWR_N), REF_PW90_N*1e6);
         fclose(fp);
         sprintf(fs,"cd $vnmruser/shapelib; Pbox > /tmp/${LOGNAME}_bionmrfile");
         ret = system(fs);
        } 
     else          { printf("\nUnable to write Pbox.inp file !");  psg_abort(1); }
  }
}

/* xxxxxxxxxxxxxxxxxxxxxxxxx     N shaped pulse     xxxxxxxxxxxxxxxxxxxxxxxx   */
/*   creates shape file for irradiation of a 15N band                          */
/*    parameters: anyshape: type of shape                                       */
/*                bw : band width (in ppm)                                      */
/*		  shift: frequency shift with respect to 15N carrier (in ppm)    */
/********************************************************************************/

void n_shapedpulse(char *anyshape, double bw, double shift,
                   codeint phase1, double rofa, double rofb) 
{
 shape 		pwshape;
 double 	compN = getval("compN"),
                pwN =  getval("pwN"),
                pwNlvl =  getval("pwNlvl");
                
                

		REF_PW90_N = pwN;
		REF_PWR_N = pwNlvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
nshapefiles(anyshape, bw, shift);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwshape = getRshapeinfo(SNAME_N);
 dec2power(pwshape.pwr);
 if (pwshape.pwrf)      dec2pwrf(pwshape.pwrf*compN);
 else			dec2pwrf(4095.0);
 dec2shaped_pulse(pwshape.name,pwshape.pw,phase1,rofa,rofb);
 dec2pwrf(4095.0);
 dec2power(pwNlvl);
}

/* determine pulse length of n_shapedpulse */

double n_shapedpw(char *anyshape, double bw, double shift,
                  codeint phase1, double rofa, double rofb) 
{
 shape 		pwshape;
 double 	pwN =  getval("pwN"),
                pwNlvl =  getval("pwNlvl");
                               
		REF_PW90_N = pwN;
		REF_PWR_N = pwNlvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
nshapefiles(anyshape, bw, shift);


/* GET SHAPE FILE PARAMETERS */
 pwshape = getRshapeinfo(SNAME_N);
 return(pwshape.pw);
}

/* xxxxxxxxxxxxxxxxxxx     simultaneous C and N shaped pulses     xxxxxxxxxxxxxxxxxxx     */
/*   creates shape file for selective irradiation of a 13C band and a 15N band            */
/*    parameters: anyshapeC and anyshapeN : types of shape                                */
/*                bwC and bwN : band width (in ppm)                                       */
/*		  shiftC and shiftN: frequency shift with respect to carrier (in ppm)     */
/******************************************************************************************/

void cn_simshapedpulse(char *anyshapeC, double bwC, double shiftC, double pwH,
                       char *anyshapeN, double bwN, double shiftN,
                       codeint phase1, codeint phase2, codeint phase3,
                       double rofa, double rofb) 
{
 shape 		pwshapeC, pwshapeN;
 double 	compC = getval("compC"),
                pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl"),
		compN = getval("compN"),
                pwN =  getval("pwN"),
                pwNlvl =  getval("pwNlvl");
                

		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;
		REF_PW90_N = pwN;
		REF_PWR_N = pwNlvl;

/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles(anyshapeC, bwC, shiftC);
nshapefiles(anyshapeN, bwN, shiftN);
 
/* GET SHAPE FILE & EXECUTE PULSE */
 pwshapeC = getRshapeinfo(SNAME_C);
 pwshapeN = getRshapeinfo(SNAME_N);
 decpower(pwshapeC.pwr);
 dec2power(pwshapeN.pwr);
 if (pwshapeC.pwrf)      decpwrf(pwshapeC.pwrf*compC);
 else			decpwrf(4095.0);
  if (pwshapeN.pwrf)      dec2pwrf(pwshapeN.pwrf*compN);
 else			dec2pwrf(4095.0);
 sim3shaped_pulse("",pwshapeC.name,pwshapeN.name,pwH,pwshapeC.pw,pwshapeN.pw,phase1,phase2,phase3,rofa,rofb);
 decpwrf(4095.0);
 dec2pwrf(4095.0);
 decpower(pwClvl);
 dec2power(pwNlvl);
}

/* determine pulse length of cn_simshapedpulse */

double cn_simshapedpw(char *anyshapeC, double bwC, double shiftC, double pwH,
                      char *anyshapeN, double bwN, double shiftN,
                      codeint phase1, codeint phase2, codeint phase3,
                      double rofa, double rofb)
{
 shape 		pwshapeC, pwshapeN;
 double         pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl"),
                pwN =  getval("pwN"),
                pwNlvl =  getval("pwNlvl"),
                max_pw;

		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;
		REF_PW90_N = pwN;
		REF_PWR_N = pwNlvl;

/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles(anyshapeC, bwC, shiftC);
nshapefiles(anyshapeN, bwN, shiftN);

/* GET SHAPE FILE PARAMETERS */
 pwshapeC = getRshapeinfo(SNAME_C);
 pwshapeN = getRshapeinfo(SNAME_N);

 if (pwshapeN.pw > pwH) max_pw = pwshapeN.pw;
    else max_pw = pwH;
 if (pwshapeC.pw > max_pw) max_pw = pwshapeC.pw;
 
 return(max_pw);
 }
 
 
 
 /* xxxxxxxxxxxxxxxxxxx     simultaneous H and C shaped pulses     xxxxxxxxxxxxxxxxxxx     */
/*   creates shape file for selective irradiation of a 1H band and a 13C band            */
/*    parameters: anyshapeC and anyshapeN : types of shape                                */
/*                bwH and bwC : band width (in ppm)                                       */
/*		  shiftH and shiftC: frequency shift with respect to carrier (in ppm)     */
/******************************************************************************************/

void hc_simshapedpulse(char *anyshapeH, double bwH, double shiftH,
                       char *anyshapeC, double bwC, double shiftC,
                       codeint phase1, codeint phase2, double rofa, double rofb) 
{
 shape 		pwshapeC, pwshapeH;
 double 	compC = getval("compC"),
                pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl"),
		compH = getval("compH"),
                tpwr =  getval("tpwr"),
                pw =  getval("pw");
                

		REF_PW90 = pw;
		REF_PWR = tpwr;
		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;

/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles(anyshapeC, bwC, shiftC);
hshapefiles(anyshapeH, bwH, shiftH);
 
/* GET SHAPE FILE & EXECUTE PULSE */
 pwshapeH = getRshapeinfo(SNAME);
 pwshapeC = getRshapeinfo(SNAME_C);
 decpower(pwshapeC.pwr);
 obspower(pwshapeH.pwr);
 if (pwshapeC.pwrf)      decpwrf(pwshapeC.pwrf*compC);
 else			decpwrf(4095.0);
  if (pwshapeH.pwrf)      obspwrf(pwshapeH.pwrf*compH);
 else			obspwrf(4095.0);
 simshaped_pulse(pwshapeH.name,pwshapeC.name,pwshapeH.pw,pwshapeC.pw,phase1,phase2,rofa,rofb);
 decpwrf(4095.0);
 obspwrf(4095.0);
 decpower(pwClvl);
 obspower(tpwr);
}

/* determine pulse length of hc_simshapedpulse */

double hc_simshapedpw(char *anyshapeH, double bwH, double shiftH,
                      char *anyshapeC, double bwC, double shiftC,
                      codeint phase1, codeint phase2, double rofa, double rofb) 
{
 shape 		pwshapeC, pwshapeH;
 double 
                pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl"),
                tpwr =  getval("tpwr"),
                pw =  getval("pw"),
                max_pw;
                

		REF_PW90 = pw;
		REF_PWR = tpwr;
		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles(anyshapeC, bwC, shiftC);
hshapefiles(anyshapeH, bwH, shiftH);

/* GET SHAPE FILE PARAMETERS */
 pwshapeC = getRshapeinfo(SNAME_C);
 pwshapeH = getRshapeinfo(SNAME);

 if (pwshapeC.pw > pwshapeH.pw) max_pw = pwshapeC.pw;
    else max_pw = pwshapeH.pw;
 
 return(max_pw);
 }
 
 
 
  /* xxxxxxxxxxxxxxxxxxx     simultaneous H shaped and C hard pulses     xxxxxxxxxxx     */
/*   creates shape file for selective irradiation of a 1H band                           */
/*    parameters: anyshapeH  : types of shape                                            */
/*                bwH  : band width (in ppm)                                             */
/*		  shiftH : frequency shift with respect to carrier (in ppm)              */
/**************************************************************************************  */

void h_simshapedpulse(char *anyshapeH, double bwH, double shiftH, double pwC,
                      codeint phase1, codeint phase2, double rofa, double rofb) 
{
 shape 		pwshapeH;
 double 	
                pwClvl = getval("pwClvl"),
		compH = getval("compH"),
                tpwr =  getval("tpwr"),
                pw =  getval("pw");
                

		REF_PW90 = pw;
		REF_PWR = tpwr;

/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */

hshapefiles(anyshapeH, bwH, shiftH);
 
/* GET SHAPE FILE & EXECUTE PULSE */
 pwshapeH = getRshapeinfo(SNAME);
 
 obspower(pwshapeH.pwr);
  decpower(pwClvl);
  if (pwshapeH.pwrf)      obspwrf(pwshapeH.pwrf*compH);
 else			obspwrf(4095.0);
 
 simshaped_pulse(pwshapeH.name,"",pwshapeH.pw,pwC,phase1,phase2,rofa,rofb);

 obspwrf(4095.0);
 decpower(pwClvl);
 obspower(tpwr);
}

/* determine pulse length of hc_simshapedpulse */

double h_simshapedpw(char *anyshapeH, double bwH, double shiftH, double pwC,
                     codeint phase1, codeint phase2, double rofa, double rofb) 
{
 shape 		pwshapeH;
 double 	
                tpwr =  getval("tpwr"),
                pw =  getval("pw"),
                max_pw;
                

		REF_PW90 = pw;
		REF_PWR = tpwr;
		

/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */

hshapefiles(anyshapeH, bwH, shiftH);

/* GET SHAPE FILE PARAMETERS */
 
 pwshapeH = getRshapeinfo(SNAME);

 if (pwC > pwshapeH.pw) max_pw = pwC;
    else max_pw = pwshapeH.pw;
 
 return(max_pw);
 }
 
/*   xxxxxxxxxxxxxxxxxxx     simultaneous H shaped and C and N hard pulses     xxxxxxxxxxx     */
/*   creates shape file for selective irradiation of a 1H band                           */
/*    parameters: anyshapeH  : types of shape                                            */
/*                bwH  : band width (in ppm)                                             */
/*		  shiftH : frequency shift with respect to carrier (in ppm)              */
/**************************************************************************************  */

void h_sim3shapedpulse(char *anyshapeH, double bwH, double shiftH, double pwC,
                       double pwN, codeint phase1, codeint phase2, codeint phase3,
                       double rofa, double rofb) 
{
 shape 		pwshapeH;
 double 	
                pwClvl = getval("pwClvl"),
		pwNlvl = getval("pwNlvl"),
		compH = getval("compH"),
                tpwr =  getval("tpwr"),
                pw =  getval("pw");
                

		REF_PW90 = pw;
		REF_PWR = tpwr;

/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */

hshapefiles(anyshapeH, bwH, shiftH);
 
/* GET SHAPE FILE & EXECUTE PULSE */
 pwshapeH = getRshapeinfo(SNAME);
 
 obspower(pwshapeH.pwr);
  decpower(pwClvl);
  dec2power(pwNlvl);
  if (pwshapeH.pwrf)      obspwrf(pwshapeH.pwrf*compH);
 else			obspwrf(4095.0);
 
 sim3shaped_pulse(pwshapeH.name,"","",pwshapeH.pw,pwC,pwN,phase1,phase2,phase3,rofa,rofb);

 obspwrf(4095.0);
 decpower(pwClvl);
 dec2power(pwNlvl);
 obspower(tpwr);
}

/* determine pulse length of h_sim3shapedpulse */

double h_sim3shapedpw(char *anyshapeH, double bwH, double shiftH, double pwC,
                      double pwN, codeint phase1, codeint phase2, codeint phase3,
                      double rofa, double rofb)  
{
 shape 		pwshapeH;
 double 	
                tpwr =  getval("tpwr"),
                pw =  getval("pw"),
                max_pw;
                

		REF_PW90 = pw;
		REF_PWR = tpwr;
		

/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */

hshapefiles(anyshapeH, bwH, shiftH);

/* GET SHAPE FILE PARAMETERS */
 
 pwshapeH = getRshapeinfo(SNAME);

 if (pwC > pwshapeH.pw) max_pw = pwC;
    else max_pw = pwshapeH.pw;
  if (pwN > max_pw) max_pw = pwN;
  
 return(max_pw);
 }
 
 
/*  xxxxxxxxxxxxxxxxxxxxxxxx  ihn_evol_se_train   xxxxxxxxxxxxxxxxxxxxxxxxxx  */
/*  Perttu Permi, U. Helsinki  

 There are two major differences between ihn_evol and hn_evol in bionmr.h:

 i)  the 15N shift is decremented --> f2coef='1 0 -1 0 0 1 0 1'
 ii) 90 degree 13CO and 13Ca pulses are applied in addition to 13C
     180 pulses during back-transfer, whereas in hn_evol only 180 pulses
     are applied.                                                             */

void ihn_evol_se_train(char *excband, char *nullband)
{
/* DECLARE AND LOAD VARIABLES */
char	    c1shape[MAXSTR], c2shape[MAXSTR],

	    mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
 	    TROSY[MAXSTR];			    /* do TROSY on N15 and H1 */

double	timeTN = getval("timeTN"),	   /* constant time for 15N evolution */
        timeC = getval("timeC"),
        timeNCA = getval("timeNCA"),
        lambda = getval("lambda"),
  	pwS9 __attribute((unused)),					  /* length of last C13 pulse */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
	dpwr2 = getval("dpwr2"),		  /* power for N15 decoupling */

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
	gzcal  = getval("gzcal"),            /* g/cm to DAC conversion factor */
        gzlvl1 = getval("gzlvl1"),
	gzlvl2 = getval("gzlvl2"),

	gt4 = getval("gt4"),				   /* other gradients */
	gt5 = getval("gt5"),
	gzlvl4 = getval("gzlvl4"),
	gzlvl5 = getval("gzlvl5"),
	gzlvl6 = getval("gzlvl6");

    getstr("mag_flg",mag_flg);
    getstr("TROSY",TROSY);

/*   INITIALIZE VARIABLES   */
    if (! strcmp(excband, "co"))
	  { strcpy(c1shape, "sinc"); strcpy(c2shape, "square"); }
    else  { strcpy(c1shape, "square"); strcpy(c2shape, "sinc"); }

    pwS9 = c13pulsepw(nullband, excband, c2shape, 180.0); 

/*  xxxxx  LAST COMMON HALF OF PULSE SEQUENCE FOLLOWS  xxxxx  */
/*  xxxxx  OPTIONS FOR N15 EVOLUTION  xxxxxx  */

	dcplrphase(zero);
	dec2phase(t8);
	zgradpulse(gzlvl4, gt4);
 	delay(2.0e-4);
        if (TROSY[A]=='n')      h1waltzon("WALTZ16", widthHd, 0.0);
	dec2rgpulse(pwN, t8, 0.0, 0.0);
        c13pulse("co", "ca", c1shape, 90.0, one, 0.0,0.0);
        c13pulse("ca", "co", c2shape, 180.0, zero, 2.0e-6, 2.0e-6);
        delay(timeC);

        sim3_c13pulse("", "co", "ca", c1shape, "", 0.0, 180.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        c13pulse("ca", "co", c2shape, 180.0, zero, 2.0e-6,2.0e-6);

        delay(timeC);
        c13pulse("co", "ca", c1shape, 90.0, zero, 0.0,0.0);
         
        delay(tau2 + timeTN + 0.65*pwN);

        c13pulse("co", "ca", c1shape, 180.0, zero, 0.0,0.0);

        delay(timeNCA - timeTN - timeC + 0.65*pwN);

        dec2rgpulse(2.0*pwN,t9,0.0,0.0);
        c13pulse("ca", "co", c1shape, 180.0, zero, 0.0,0.0);
         
        delay(timeNCA - timeC - kappa - tau2 -PRG_STOP_DELAY - pwHd - 2.0e-6 -POWER_DELAY);
       
	dec2phase(t10);

	  h1waltzoff("WALTZ16", widthHd, 0.0);
	  txphase(t4);

          delay(kappa - gt1 - 2.0*GRADIENT_DELAY - 1.50e-4);
          if (mag_flg[A]=='y')    magradpulse(gzcal*gzlvl1, gt1); 
          else    zgradpulse(gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(1.50e-4);
	
/* xxxxxxxxx  SE TRAIN  xxxxxxxxx */
	if (TROSY[A]=='y')  rgpulse(pw, t4, 0.0, 0.0);
	else                sim3pulse(pw, 0.0, pwN, t4, zero, t10, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl5, gt5);
	if (TROSY[A]=='y')   delay(lambda - 0.65*(pw + pwN) - gt5);
	else   		     delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	zgradpulse(gzlvl5, gt5);
	txphase(one);
	dec2phase(t11);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(pw, 0.0, pwN, one, zero, t11, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl6, gt5);
	delay(lambda - 1.3*pwN - gt5);

	sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);

	dec2phase(t10);
	zgradpulse(gzlvl6, gt5);

	if (TROSY[A]=='y')
	     {delay(lambda - 1.3*pwN - gt5);
	      dec2rgpulse(pwN, t10, 0.0, 0.0);
	      delay((gt1/10.0) + 1.0e-4 - 0.3*pwN + 2.0*GRADIENT_DELAY
		 					+ POWER_DELAY);  }
	else {delay(lambda - 0.65*(pw + pwN) - gt5);
	      rgpulse(pw, zero, 0.0, 0.0); 
	      delay((gt1/10.0) + 1.0e-4 - 0.3*pw + 2.0*GRADIENT_DELAY
							+ POWER_DELAY);  }
	rgpulse(2.0*pw, zero, 0.0, 0.0);

	dec2power(dpwr2);				       /* POWER_DELAY */
        if (mag_flg[A] == 'y')    magradpulse(icosel*gzcal*gzlvl2, gt1/10.0);
        else   zgradpulse(icosel*gzlvl2, gt1/10.0);            /* 2.0*GRADIENT_DELAY */

	rcvron();
	statusdelay(C, 1.0e-4);
	setreceiver(t12);
}


/*  xxxxxxxxxxxxx   TRIPLE RESONANCE NH EVOLUTION & SE TRAIN   xxxxxxxxxxxxx  */

/*  xxxxxxxxxxxxxxxxxxxxxxxx   nh_evol_train   xxxxxxxxxxxxxxxxxxxxxxxxxx  */
/*  for g...._nh pulse sequences  */

void nh_evol_train(excband, nullband)

char		*excband, *nullband;
{
/* DECLARE AND LOAD VARIABLES */
char	    c1shape[MAXSTR], c2shape[MAXSTR],

	    mag_flg[MAXSTR],      /* magic-angle coherence transfer gradients */
 	    TROSY[MAXSTR];			    /* do TROSY on N15 and H1 */

double	    timeTN = getval("timeTN"),     /* constant time for 15N evolution */
 
   	pwS9,					  /* length of last C13 pulse */
        pwN = getval("pwN"),          /* N15 90 degree pulse length at pwNlvl */
	dpwr2 = getval("dpwr2"),		  /* power for N15 decoupling */

	gt1 = getval("gt1"),  		       /* coherence pathway gradients */
	gzcal  = getval("gzcal"),            /* g/cm to DAC conversion factor */
        gzlvl1 = getval("gzlvl1"),
	lambda2=getval("lambda2"),
	gt3 = getval("gt3"),
	gt5 = getval("gt5"),
	gzlvl3 = getval("gzlvl3"),
	gzlvl5 = getval("gzlvl5"),
        tpwrsf = getval("tpwrsf"),    /* fine power adustment for soft pulse  */
        tpwrs;                        /* power for the pwHs ("H2Osinc") pulse */

    getstr("mag_flg",mag_flg);
    getstr("TROSY",TROSY);

/*   INITIALIZE VARIABLES   */
    if (! strcmp(excband, "co"))
	  { strcpy(c1shape, "sinc"); strcpy(c2shape, "square"); }
    else  { strcpy(c1shape, "square"); strcpy(c2shape, "sinc"); }

    pwS9 = c13pulsepw(nullband, excband, c2shape, 180.0); 


/* selective H20 one-lobe sinc pulse *********************/
   pwHs = 1.7e-3*500.0/sfrq;       /* length of H2O flipback, 1.7ms at 500 MHz*/
    tpwrs = tpwr - 20.0*log10(pwHs/(pw*1.69));   /*needs 1.69 times more*/
    tpwrs = (int) (tpwrs);                        /*power than a square pulse */

/*  xxxxx  LAST COMMON HALF OF PULSE SEQUENCE FOLLOWS  xxxxx  */
/*  xxxxx  OPTIONS FOR N15 EVOLUTION  xxxxxx  */

	c13pulse(excband, nullband, c1shape, 90.0, zero, 2.0e-6, 0.0);

    	if ( ((! strcmp(excband, "ca")) || (! strcmp(excband, "cab"))) &&
	      (dm3[B] == 'y') )		         /*optional 2H decoupling off */
           {dec3rgpulse(1/dmf3, three, 0.0, 0.0); dec3blank();
            setstatus(DEC3ch, FALSE, 'w', FALSE, dmf3); dec3blank();}
	dcplrphase(zero);
	zgradpulse(gzlvl3, gt3);
        if (TROSY[A]=='y') { h1decoff(); }
     	delay(2.0e-4);
	dec2rgpulse(pwN, t8, 0.0, 0.0);

	decphase(zero);
	dec2phase(t9);
	delay(timeTN - WFG3_START_DELAY - tau2);
							 /* WFG3_START_DELAY  */
	sim3_c13pulse("", excband, nullband, c1shape, "", 0.0, 180.0, 2.0*pwN, 
						zero, zero, t9, 2.0e-6, 2.0e-6);

	dec2phase(t10);

if (TROSY[A]=='y')
{
    if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)
	{
	  txphase(t4);
          delay(timeTN - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY
						 - 2.0*PWRF_DELAY - 2.0e-6);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')  magradpulse(icosel*gzcal*gzlvl1, gt1);
          else  zgradpulse(icosel*gzlvl1, gt1);   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4);
	}
    else
	{
	  txphase(t4);
          delay(timeTN - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY
 	      - 2.0*PWRF_DELAY - 2.0e-6 - gt1 - 2.0*GRADIENT_DELAY - 1.0e-4);
          if (mag_flg[A]=='y')    magradpulse(icosel*gzcal*gzlvl1, gt1);
          else  zgradpulse(icosel*gzlvl1, gt1);	   	/* 2.0*GRADIENT_DELAY */
	  delay(1.0e-4);      			           /* WFG_START_DELAY */
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2);
	}
}
else
{
    if (tau2 > kappa + PRG_STOP_DELAY)
	{
          delay(timeTN - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY 
						- 2.0*PWRF_DELAY - 2.0e-6);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2 - kappa - PRG_STOP_DELAY - POWER_DELAY - PWRF_DELAY);
          h1decoff();		     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
	  txphase(t4);
          delay(kappa);
	}
    else if (tau2 > (kappa - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY - 2.0e-6))
	{
          delay(timeTN + tau2 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
          h1decoff();		     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
	  txphase(t4); 			/* WFG_START_DELAY  + 2.0*POWER_DELAY */
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(kappa - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY - 1.0e-6 );
	}
    else /*if (tau2 > gt1 + 2.0*GRADIENT_DELAY + 1.0e-4)*/
	{
          delay(timeTN + tau2 - kappa -PRG_STOP_DELAY -POWER_DELAY -PWRF_DELAY);
          h1decoff();		     /* POWER_DELAY+PWRF_DELAY+PRG_STOP_DELAY */
	  txphase(t4);
          delay(kappa - tau2 - pwS9 - WFG_START_DELAY - 2.0*POWER_DELAY
 								    - 2.0e-6);
	c13pulse(nullband, excband, c2shape, 180.0, zero, 2.0e-6, 0.0); /*pwS9*/
          delay(tau2 );
	}
}
/* xxxxxxxxx  SE TRAIN  xxxxxxxxx */
/* xxxxxxxxx  NOSE TRAIN  xxxxxxxxx */
	sim3pulse(pw, 0.0, pwN, zero, zero, zero, 0.0, 0.0);

	txphase(zero);
	dec2phase(zero);
	zgradpulse(gzlvl5, gt5);
        delay(50e-6);
	delay(lambda2 - pwHs - gt5 -POWER_DELAY -50e-6);

        if (tpwrsf<4095.0)
        {
         obspower(tpwrs+6.0); obspwrf(tpwrsf);
         shaped_pulse("H2Osinc", pwHs, two, 5.0e-5, 0.0);
         obspower(tpwr); obspwrf(4095.0);
        }
        else
        {
         obspower(tpwrs);
         shaped_pulse("H2Osinc", pwHs, two, 5.0e-5, 0.0);
         obspower(tpwr);
        }
     sim3pulse(2.0*pw, 0.0, 2.0*pwN, zero, zero, zero, 0.0, 0.0);
        if (tpwrsf<4095.0)
        {
         obspower(tpwrs+6.0); obspwrf(tpwrsf);
         shaped_pulse("H2Osinc", pwHs, two, 5.0e-5, 0.0);
         obspower(tpwr); obspwrf(4095.0);
        }
        else
        {
         obspower(tpwrs);
         shaped_pulse("H2Osinc", pwHs, two, 5.0e-5, 0.0);
         obspower(tpwr);
        }

	zgradpulse(gzlvl5, gt5);
        delay(50e-6);
	delay(lambda2 - pwHs - gt5- POWER_DELAY -50e-6 -1.0e-4);
	dec2power(dpwr2);				       /* POWER_DELAY */
	rcvron();
	statusdelay(C, 1.0e-4);
	setreceiver(t13);
}


void ch_simshapedpulse(anyshapeC,bwC, shiftC,shapeH,pwH,pwN, phase1, phase2,phase3, rofa, rofb) 

char 		*anyshapeC,*shapeH ;
double 		bwC,shiftC,pwH,pwN,rofa, rofb;
codeint 	phase1, phase2, phase3;
{
 shape 		pwshape;
 double 	compC = getval("compC"),
                pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl");
                

		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles(anyshapeC, bwC, shiftC);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwshape = getRshapeinfo(SNAME_C);
 decpower(pwshape.pwr);
 if (pwshape.pwrf)      decpwrf(pwshape.pwrf*compC);
 else			decpwrf(4095.0);
 sim3shaped_pulse(shapeH,pwshape.name,"",pwH,pwshape.pw,pwN,phase1,phase2,phase3, rofa, rofb);
 decpwrf(4095.0);
 decpower(pwClvl);
}

/* determine pulse length of c_simshapedpulse */

double ch_simshapedpw(anyshapeC,bwC,shiftC,shapeH,pwH,pwN,phase1,phase2,phase3, rofa, rofb) 

char 		*anyshapeC, *shapeH ;
double 		bwC,shiftC,pwH,pwN,rofa, rofb;
codeint 	phase1, phase2, phase3;
{
 shape 		pwshape;
 double         max_pw,
                pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl");
                

		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles(anyshapeC, bwC, shiftC);


/* GET SHAPE FILE PARAMETERS */
 pwshape = getRshapeinfo(SNAME_C);
 
 if (pwshape.pw > pwH) max_pw = pwshape.pw;
    else max_pw = pwH;
 if (pwN > max_pw) max_pw = pwN;
 
 return(max_pw);
}

 /******** generate double band 13C shape with 15N and H ****************/

void ch_simshapedpulse2(anyshape1,bw1,shift1,anyshape2,bw2, shift2,shapeH,pwH,pwN,phase1,phase2,phase3,rofa,rofb) 

char 		*anyshape1,*anyshape2,*shapeH ;
double 		bw1, bw2, shift1, shift2,pwH,pwN,rofa,rofb;
codeint 	phase1, phase2, phase3;
{
 shape 		pwshape;
 double 	compC = getval("compC"),
                pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl");
                

		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles2(anyshape1,anyshape2,bw1,bw2,shift1,shift2);


/* GET SHAPE FILE & EXECUTE PULSE */
 pwshape = getRshapeinfo(SNAME_C);
 decpower(pwshape.pwr);
 if (pwshape.pwrf)      decpwrf(pwshape.pwrf*compC);
 else			decpwrf(4095.0);
 sim3shaped_pulse(shapeH,pwshape.name,"",pwH,pwshape.pw,pwN,phase1,phase2,phase3, rofa, rofb);
 decpwrf(4095.0);
 decpower(pwClvl);
}

/* determine pulse length of c_simshapedpulse */

double ch_simshaped2pw(anyshape1,bw1,shift1,anyshape2, bw2, shift2,shapeH,pwH,pwN,phase1,phase2,phase3, rofa, rofb) 

char 		*anyshape1,*anyshape2, *shapeH ;
double 		bw1, bw2, shift1, shift2,pwH,pwN,rofa, rofb;
codeint 	phase1, phase2, phase3;
{
 shape 		pwshape;
 double         max_pw,
                pwC =  getval("pwC"),
                pwClvl =  getval("pwClvl");
                

		REF_PW90_C = pwC;
		REF_PWR_C = pwClvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
cshapefiles2(anyshape1, anyshape2, bw1, bw2, shift1, shift2);


/* GET SHAPE FILE PARAMETERS */
 pwshape = getRshapeinfo(SNAME_C);
 
 if (pwshape.pw > pwH) max_pw = pwshape.pw;
    else max_pw = pwH;
 if (pwN > max_pw) max_pw = pwN;
 
 return(max_pw);
}


 
  /* xxxxxxxxxxxxxxxxxxx     simultaneous H and N shaped pulses     xxxxxxxxxxxxxxxxxxx     */
/*   creates shape file for selective irradiation of a 1H band and a 15N band            */
/*    parameters: anyshapeH and anyshapeN : types of shape                                */
/*                bwH and bwC : band width (in ppm)                                       */
/*		  shiftH and shiftC: frequency shift with respect to carrier (in ppm)     */
/******************************************************************************************/

void hn_simshapedpulse(anyshapeH,bwH,shiftH,anyshapeN,bwN,shiftN,phase1,phase2,rofa,rofb) 

char 		*anyshapeN,*anyshapeH ;
double 		bwN,shiftN,bwH,shiftH,rofa,rofb;
codeint 	phase1, phase2;
{
 shape 		pwshapeN, pwshapeH;
 double 	compN = getval("compN"),
                pwN =  getval("pwN"),
                pwNlvl =  getval("pwNlvl"),
		compH = getval("compH"),
                tpwr =  getval("tpwr"),
                pw =  getval("pw");
                

		REF_PW90 = pw;
		REF_PWR = tpwr;
		REF_PW90_N = pwN;
		REF_PWR_N = pwNlvl;

/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
nshapefiles(anyshapeN, bwN, shiftN);
hshapefiles(anyshapeH, bwH, shiftH);
 
/* GET SHAPE FILE & EXECUTE PULSE */
 pwshapeH = getRshapeinfo(SNAME);
 pwshapeN = getRshapeinfo(SNAME_N);
 dec2power(pwshapeN.pwr);
 obspower(pwshapeH.pwr);
 if (pwshapeN.pwrf)      dec2pwrf(pwshapeN.pwrf*compN);
 else			dec2pwrf(4095.0);
  if (pwshapeH.pwrf)      obspwrf(pwshapeH.pwrf*compH);
 else			obspwrf(4095.0);
 sim3shaped_pulse(pwshapeH.name,"",pwshapeN.name,pwshapeH.pw,0.0,pwshapeN.pw,phase1,zero,phase2,rofa,rofb);
 dec2pwrf(4095.0);
 obspwrf(4095.0);
 dec2power(pwNlvl);
 obspower(tpwr);
}

/* determine pulse length of hn_simshapedpulse */

double hn_simshapedpw(anyshapeH,bwH,shiftH,anyshapeN,bwN,shiftN,phase1,phase2,rofa,rofb) 

char 		*anyshapeN,*anyshapeH ;
double 		bwN,shiftN,bwH,shiftH,rofa,rofb;
codeint 	phase1, phase2;
{
 shape 		pwshapeN, pwshapeH;
 double 
                pwN =  getval("pwN"),
                pwNlvl =  getval("pwNlvl"),
                tpwr =  getval("tpwr"),
                pw =  getval("pw"),
                max_pw;
                

		REF_PW90 = pw;
		REF_PWR = tpwr;
		REF_PW90_N = pwN;
		REF_PWR_N = pwNlvl;


/* GENERATE PULSE SHAPE NAME & MAKE SHAPE FILES IF THEY DON'T EXIST */
nshapefiles(anyshapeN, bwN, shiftN);
hshapefiles(anyshapeH, bwH, shiftH);

/* GET SHAPE FILE PARAMETERS */
 pwshapeN = getRshapeinfo(SNAME_N);
 pwshapeH = getRshapeinfo(SNAME);

 if (pwshapeN.pw > pwshapeH.pw) max_pw = pwshapeN.pw;
    else max_pw = pwshapeH.pw;
 
 return(max_pw);
 }
 

/***************/
/* End of File */
/***************/
