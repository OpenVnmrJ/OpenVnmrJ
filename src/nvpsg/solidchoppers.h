/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
// File with interpolated chopper functions

// Contents: 

// make_shape()       - SHAPE  Make an interpolated shaped pulse with STATE's.   
// genericInitShape() - SHAPE  Initialize an interpolated shaped pulse with STATE's.
// MPchopper          - MPSEQ  Make an interpolated multiple pulse sequence.
// make_cp            - CP     Make an interpolated tangent ramp for CP.   
// make_ramp          - RAMP   Make a tangent ramp for DREAM.
// make_shape1()       - SHAPE  Make an interpolated (amplitude only) shaped pulse
//                      with STATE's.   
// genericInitShape1() - SHAPE  Special version used with make_shape1.

//=======================================
// Redeclare userDECShape
//=======================================

extern void userDECShape(char *name, struct _DECpattern *pa, double dres, int mode, int steps, int chan);

//========================================
// Make a Generic Shape with interpolation
//======================================== 

SHAPE make_shape(SHAPE s)
{
   FILE *fp;
   extern char userdir[];
   char shapepath[4*MAXSTR],str[MAXSTR+16];
   int ntick,nstep,nrpuls,naccumpuls,nstub,nindex,chnl,ninit,ntot,npuls;
   double t,dph,ph,phShape,phCurrent,phLast,phOut,ph0Out,phInc,aCurrent,
          aLast,aOut,a0Out,aInc,gCurrent,gLast,gOut,phase;
   STATE state;

// Set the channel

  chnl = 0;
  if (!strcmp(s.pars.ch,"obs")) chnl = OBSch;
  else if (!strcmp(s.pars.ch,"dec")) chnl = DECch;
  else if (!strcmp(s.pars.ch,"dec2")) chnl = DEC2ch;
  else if (!strcmp(s.pars.ch,"dec3")) chnl = DEC3ch;
  else {
        printf("make_shape() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
  }

// Open the output file for print.

   OSPRINTF( str, sizeof(str), "%s/shapelib/", userdir);
   OSPRINTF( shapepath, sizeof(shapepath), "%s%s.DEC", str, s.pars.pattern);
   if ((fp = fopen(shapepath,"w")) == NULL) {
      printf("Error in open of %s \n",shapepath);
      psg_abort(1);
   }

// Round the total time to steps, determine total ticks and steps.

   s.pars.t = s.pars.n90*DTCK*(int)(s.pars.t/(s.pars.n90*DTCK) + 0.5 + 1.0e-14);
   s.pars.n = (int)(s.pars.t/DTCK + 0.5 + 1.0e-14);   
   nstub = s.pars.n/s.pars.n90;

// Create a waveform structure. 

   DECpattern *pstub;
   pstub = (DECpattern *) malloc(nstub/s.pars.n90m*s.pars.nelem*sizeof(struct _DECpattern));

//  Save the total duration to the MODULE structure.

   char lpattern[NPATTERN];
   int lix = arryindex(s.pars.array);
   OSPRINTF( lpattern, sizeof(lpattern), "%s%d", getname0("",s.pars.seqName,""), s.pars.nRec);
   savet(s.pars.t, lix, lpattern);

// Initialize the time, phase and steps

   ph = s.pars.phAccum;
   nrpuls = s.pars.n;
   t = 0; 

// Abort if nrpuls < (s.pars.n90*s.pars.n90m)

   if (nrpuls < s.pars.n90*s.pars.n90m) {
      printf("Error - Pulse width too small\n");  
      psg_abort(1);
   }

// Initialize the number of shapes

   npuls = s.pars.nelem; 

// Initialize indicies and output variables

   ntick = 0;
   nstep = 0;
   naccumpuls = 0;
   nindex = 0;
   ntot = 0;

   aOut = 0.0;
   a0Out = 0.0;
   aInc = 0.0;
   phOut = 0.0;
   ph0Out = 0.0;
   phInc = 0.0;
   gOut = 0.0;
   aLast = 0.0;
   gLast = 0.0;
   phLast = 0.0;
   aCurrent = 0.0;
   gCurrent = 0.0;
   phCurrent = 0.0;
   phase=0.0;

// Set the phase increment for an offset

   if (fabs(s.pars.of) > 1.0) {
      dph = 360.0*s.pars.of*DTCK;
   }
   else {
      dph = 0.0;
   }

//Output the shape and count ticks down, nrpul to zero.

   int i;
   for (i = 0; i<npuls;i++) {
      ntick = 0;
      nstep = 0;
      naccumpuls = 0;
      t = 0.0;
      ninit = 1;
      while (nrpuls > 0) {
         if (ninit > 0) {                   
            state =  s.get_state(t,s.pars);                         
            aLast = state.a;
            phShape = state.p;
            gLast = state.g;        
            ph = ph + s.pars.phInt;
            s.pars.phInt = 0.0;
            phLast = ph + phShape;
            ninit = 0;
         }
         ntick++;
         nrpuls--;
         naccumpuls++;
         t += DTCK;
         s.pars.phInt += dph;
         if ((ntick == s.pars.n90) && (nrpuls >= s.pars.n90*(s.pars.n90m - 1))) {
            ntick = 0;
            nstep++; 
            state =  s.get_state(t,s.pars); 
            aCurrent = state.a; 
            phShape = state.p;
            gCurrent = state.g;
            ph =  ph + s.pars.phInt;
            s.pars.phInt = 0.0;
            phase = ph + phShape;
            phCurrent = phase;

            if (nrpuls == s.pars.n90*(s.pars.n90m - 1)) { 
               t += s.pars.n90*DTCK*(s.pars.n90m - 1);
               nrpuls -= s.pars.n90*(s.pars.n90m - 1);
               s.pars.phInt += s.pars.n90*dph*(s.pars.n90m - 1);
               naccumpuls += s.pars.n90*(s.pars.n90m - 1);
               ntick += s.pars.n90*(s.pars.n90m - 1);
               nstep += s.pars.n90m - 1;
 
               state =  s.get_state(t,s.pars);
               aCurrent = state.a; aCurrent = roundamp(state.a,1.0/WSD);
               phShape = state.p;
               gCurrent = state.g;               
               ph =  ph + s.pars.phInt;
//             s.pars.phInt = 0.0;   Carry phase ramp from last step.                     
               phase = ph + phShape;
               phCurrent = phase;
//             printf("aLast = %f aCurrent = %f\n",aLast,aCurrent);
               a0Out = roundamp(aLast,1.0/WSD);
               aOut = roundamp((aCurrent + aLast)/2.0,1.0/WSD);
               aInc = roundamp((aCurrent - aLast)/nstep,1.0/WSD);
//             printf("a0Out = %f aInc = %f\n",a0Out,aInc);
//             printf("phLast = %f phCurrent = %f\n",phLast,phCurrent);
               ph0Out = roundphase(phLast,360.0/(PSD*8192));
               phOut = roundphase((phCurrent + phLast)/2.0,360.0/(PSD*8192));
               phInc = roundphaseinc((phCurrent - phLast)/nstep,360.0/(PSD*8192));
//             printf("ph0Out = %f phInc = %f\n",ph0Out,phInc);
               gOut = gLast;
               if ((s.pars.trap > 0) && (nstep > 1)){ 
                  if (s.pars.trap > 1) {                                  
                     fprintf(fp,"%10.1f %13.6f %13.6f %6.1f %13.6f %13.6f\n",(90.0/s.pars.n90)*naccumpuls,
                           ph0Out,a0Out,gOut,phInc,aInc);
                  }
                  pstub[nindex].tip = (90.0/s.pars.n90)*naccumpuls;
                  pstub[nindex].phase = ph0Out;
                  pstub[nindex].phase_inc = phInc;
                  pstub[nindex].amp = a0Out;
                  pstub[nindex].amp_inc = aInc;
                  pstub[nindex].gate = gOut;
               }
               else {
                  fprintf(fp,"%10.1f %13.6f %13.6f %6.1f\n",(90.0/s.pars.n90)*naccumpuls,phOut,aOut,gOut);
               }
               nindex++; 
               ntot = ntot + nstep;
//             printf("nindex = %d nstep = %d ntot = %d\n",nindex,nstep,ntot);
               aLast = aCurrent;                                       
               phLast = phCurrent;
               gLast = gCurrent; 
               naccumpuls = 0;
               nstep = 0;
            }
            if (((nstep >= s.pars.n90m) && ((fabs(aCurrent - aLast) >= DWFM) || 
                                       (fabs(phCurrent - phLast) >= DPH) ||
                                       (fabs(gCurrent - gLast) > 0.0))) ||
                                       (nstep >= 255)) { 
//             printf("aLast = %f aCurrent = %f\n",aLast,aCurrent);
               a0Out = roundamp(aLast,1.0/WSD);
               aOut =  roundamp((aCurrent + aLast)/2.0,1.0/WSD);  
               aInc = roundamp((aCurrent - aLast)/nstep,1.0/WSD);
//             printf("a0Out = %f aInc = %f\n",a0Out,aInc);
//             printf("phLast = %f phCurrent = %f\n",phLast,phCurrent);
               ph0Out = roundphase(phLast,360.0/(PSD*8192));
               phOut = roundphase((phCurrent + phLast)/2.0,360.0/(PSD*8192)); 
               phInc = roundphaseinc((phCurrent - phLast)/nstep,360.0/(PSD*8192));
//             printf("ph0Out = %f phInc = %f\n",ph0Out,phInc);
               gOut = gLast;  
               if ((s.pars.trap > 0) && (nstep > 1)) { 
                  if (s.pars.trap > 1) {                       
                     fprintf(fp,"%10.1f %13.6f %13.6f %6.1f %13.6f %13.6f\n",(90.0/s.pars.n90)*naccumpuls,
                       ph0Out,a0Out,gOut,phInc,aInc);
                  }
                  pstub[nindex].tip = (90.0/s.pars.n90)*naccumpuls;
                  pstub[nindex].phase = ph0Out;
                  pstub[nindex].phase_inc = phInc;
                  pstub[nindex].amp = a0Out;
                  pstub[nindex].amp_inc = aInc;
                  pstub[nindex].gate = gOut;
               }
               else {
                  fprintf(fp,"%10.1f %13.6f %13.6f %6.1f\n",(90.0/s.pars.n90)*naccumpuls,phOut,aOut,gOut);
               }
               nindex++;
               ntot = ntot + nstep;
//             printf("nindex = %d nstep = %d ntot = %d\n",nindex,nstep,ntot);
               aLast = aCurrent;
               phLast = phCurrent;
               gLast = gCurrent; 
               naccumpuls = 0;
               nstep = 0;
            }
         }
      }
   }

// Make the DECShape and close the print file.

   if ((s.pars.trap > 0) && (ntot > 0)) {
      userDECShape(s.pars.pattern,pstub,90.0,1,nindex,chnl);
   }
   s.pars.phAccum = ph; 
   fclose(fp);                                        
   free(pstub);
   return s;
}

//=========================================================================
// Make a Generic Shape with interpolation on amplitude only (used for DFS)
//========================================================================= 

SHAPE make_shape1(SHAPE s)
{
   FILE *fp;
   extern char userdir[];
   char shapepath[4*MAXSTR],str[MAXSTR+16];
   int ntick,nstep,nrpuls,naccumpuls,nstub,nindex,chnl,ninit,ntot,npuls;
   double phInc __attribute__((unused));
   double t,dph,ph,phShape,phCurrent,phLast,phOut,ph0Out,aCurrent,
          aLast,aOut,a0Out,aInc,gCurrent,gLast,gOut,phase;
   STATE state;

// Set the channel

  chnl = 0;
  if (!strcmp(s.pars.ch,"obs")) chnl = OBSch;
  else if (!strcmp(s.pars.ch,"dec")) chnl = DECch;
  else if (!strcmp(s.pars.ch,"dec2")) chnl = DEC2ch;
  else if (!strcmp(s.pars.ch,"dec3")) chnl = DEC3ch;
  else {
        printf("make_shape() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
  }

// Open the output file for print.

   OSPRINTF( str, sizeof(str), "%s/shapelib/", userdir);
   OSPRINTF( shapepath, sizeof(shapepath), "%s%s.DEC", str, s.pars.pattern);
   if ((fp = fopen(shapepath,"w")) == NULL) {
      printf("Error in open of %s \n",shapepath);
      psg_abort(1);
   }

// Round the total time to steps, determine total ticks and steps.

   s.pars.t = s.pars.n90*DTCK*(int)(s.pars.t/(s.pars.n90*DTCK) + 0.5);
   s.pars.n = (int)(s.pars.t/DTCK + 0.5);
   nstub = s.pars.n/s.pars.n90;

// Create a waveform structure. 

   DECpattern *pstub;
   pstub = (DECpattern *) malloc(nstub/s.pars.n90m*s.pars.nelem*sizeof(struct _DECpattern));

//  Save the total duration to the MODULE structure.

   char lpattern[NPATTERN];
   int lix = arryindex(s.pars.array);
   OSPRINTF( lpattern, sizeof(lpattern), "%s%d", getname0("",s.pars.seqName,""), s.pars.nRec);
   savet(s.pars.t, lix, lpattern);

// Initialize the time, phase and steps

   ph = s.pars.phAccum;
   nrpuls = s.pars.n;
   t = 0; 

// Abort if nrpuls < (s.pars.n90*s.pars.n90m)

   if (nrpuls < s.pars.n90*s.pars.n90m) {
      printf("Error - Pulse width too small\n");  
      psg_abort(1);
   }

// Initialize the number of shapes

   npuls = s.pars.nelem; 

// Initialize indicies and output variables

   ntick = 0;
   nstep = 0;
   naccumpuls = 0;
   nindex = 0;
   ntot = 0;

   aOut = 0.0;
   a0Out = 0.0;
   aInc = 0.0;
   phOut = 0.0;
   ph0Out = 0.0;
   phInc = 0.0;
   gOut = 0.0;
   aLast = 0.0;
   gLast = 0.0;
   phLast = 0.0;
   aCurrent = 0.0;
   gCurrent = 0.0;
   phCurrent = 0.0;
   phase=0.0;

// Set the phase increment for an offset

   if (fabs(s.pars.of) > 1.0) {
      dph = 360.0*s.pars.of*DTCK;
   }
   else {
      dph = 0.0;
   }

//Output the shape and count ticks down, nrpul to zero.

   int i;
   for (i = 0; i<npuls;i++) {
      ntick = 0;
      nstep = 0;
      naccumpuls = 0;
      t = 0.0;
      ninit = 1;
      while (nrpuls > 0) {
         if (ninit > 0) {                   
            state =  s.get_state(t,s.pars);                         
            aLast = state.a;
            phShape = state.p;
            gLast = state.g;        
            ph = ph + s.pars.phInt;
            s.pars.phInt = 0.0;
            phLast = ph + phShape;
            ninit = 0;
         }
         ntick++;
         nrpuls--;
         naccumpuls++;
         t += DTCK;
         s.pars.phInt += dph;

         if ((ntick == s.pars.n90) && (nrpuls >= s.pars.n90*(s.pars.n90m - 1))) {
            ntick = 0;
            nstep++; 
            state =  s.get_state(t,s.pars); 
            aCurrent = state.a; 
            phShape = state.p;
            gCurrent = state.g;
            ph =  ph + s.pars.phInt;
            s.pars.phInt = 0.0;
            phase = ph + phShape;
            phCurrent = phase;

            if (nrpuls == s.pars.n90*(s.pars.n90m - 1)) {
               t += s.pars.n90*DTCK*(s.pars.n90m - 1);
               nrpuls -= s.pars.n90*(s.pars.n90m - 1);
               s.pars.phInt += s.pars.n90*dph*(s.pars.n90m - 1);
               naccumpuls += s.pars.n90*(s.pars.n90m - 1);
               ntick += s.pars.n90*(s.pars.n90m - 1);
               nstep += s.pars.n90m - 1;
 
               state =  s.get_state(t,s.pars);
               aCurrent = state.a; aCurrent = roundamp(state.a,1.0/WSD);
               phShape = state.p;
               gCurrent = state.g;               
               ph =  ph + s.pars.phInt;
//             s.pars.phInt = 0.0;   Carry phase ramp from last step.                    
               phase = ph + phShape;
               phCurrent = phase;
//             printf("aLast = %f aCurrent = %f\n",aLast,aCurrent);
               a0Out = roundamp(aLast,1.0/WSD);
               aOut = roundamp((aCurrent + aLast)/2.0,1.0/WSD);
               aInc = roundamp((aCurrent - aLast)/nstep,1.0/WSD);
//             printf("a0Out = %f aInc = %f\n",a0Out,aInc);
//             printf("phLast = %f phCurrent = %f\n",phLast,phCurrent);
               ph0Out = roundphase(phLast,360.0/(PSD*8192));
               phOut = roundphase((phCurrent + phLast)/2.0,360.0/(PSD*8192));
               phInc = roundphaseinc((phCurrent - phLast)/nstep,360.0/(PSD*8192));
//             printf("ph0Out = %f phInc = %f\n",ph0Out,phInc);
               gOut = gCurrent;
               if ((s.pars.trap > 0) && (nstep > 1)){ 
                  if (s.pars.trap > 1) {                                  
                     fprintf(fp,"%10.1f %13.6f %13.6f %6.1f %13.6f %13.6f\n",(90.0/s.pars.n90)*naccumpuls,
                           ph0Out,a0Out,gOut,0.0,aInc);
                  }
                  pstub[nindex].tip = (90.0/s.pars.n90)*naccumpuls;
                  pstub[nindex].phase = ph0Out;//ph0Out;
                  pstub[nindex].phase_inc = 0.0;//phInc;
                  pstub[nindex].amp = a0Out;
                  pstub[nindex].amp_inc = aInc;
                  pstub[nindex].gate = gOut;
               }
               else {
		    fprintf(fp,"%10.1f %13.6f %13.6f\n",(90.0/s.pars.n90)*naccumpuls,phOut,aOut);
               }
               nindex++; 
               ntot = ntot + nstep;
//             printf("nindex = %d nstep = %d ntot = %d\n",nindex,nstep,ntot);
               aLast = aCurrent;                                       
               phLast = phCurrent;
               gLast = gCurrent; 
               naccumpuls = 0;
               nstep = 0;
            }
            if (((nstep >= s.pars.n90m) && ((fabs(aCurrent - aLast) >= DWFM) || 
                                       (fabs(phCurrent - phLast) >= DPH) ||
                                       (fabs(gCurrent - gLast) > 0.0))) ||
                                       (nstep >= 255)) { 
//             printf("aLast = %f aCurrent = %f\n",aLast,aCurrent);
               a0Out = roundamp(aLast,1.0/WSD);
               aOut =  roundamp((aCurrent + aLast)/2.0,1.0/WSD);  
               aInc = roundamp((aCurrent - aLast)/nstep,1.0/WSD);
//             printf("a0Out = %f aInc = %f\n",a0Out,aInc);
//             printf("phLast = %f phCurrent = %f\n",phLast,phCurrent);
               ph0Out = roundphase(phLast,360.0/(PSD*8192));
               phOut = roundphase((phCurrent + phLast)/2.0,360.0/(PSD*8192)); 
               phInc = roundphaseinc((phCurrent - phLast)/nstep,360.0/(PSD*8192));
//             printf("ph0Out = %f phInc = %f\n",ph0Out,phInc);
               gOut = gCurrent;  
               if ((s.pars.trap > 0) && (nstep > 1)) { 
                  if (s.pars.trap > 1) {                       
                     fprintf(fp,"%10.1f %13.6f %13.6f %6.1f %13.6f %13.6f\n",(90.0/s.pars.n90)*naccumpuls,
                       ph0Out,a0Out,gOut,0.0,aInc);
                  }
                  pstub[nindex].tip = (90.0/s.pars.n90)*naccumpuls;
                  pstub[nindex].phase = ph0Out;//ph0Out;
                  pstub[nindex].phase_inc = 0.0;//phInc;
                  pstub[nindex].amp = a0Out;
                  pstub[nindex].amp_inc = aInc;
                  pstub[nindex].gate = gOut;
               }
               else {
                  fprintf(fp,"%10.1f %13.6f %13.6f\n",(90.0/s.pars.n90)*naccumpuls,phOut,aOut);
               }
               nindex++;
               ntot = ntot + nstep;
//             printf("nindex = %d nstep = %d ntot = %d\n",nindex,nstep,ntot);
               aLast = aCurrent;
               phLast = phCurrent;
               gLast = gCurrent; 
               naccumpuls = 0;
               nstep = 0;
            }
         }
      }
   }

// Make the DECShape and close the print file.

   if ((s.pars.trap > 0) && (ntot >  0)) {
      userDECShape(s.pars.pattern,pstub,90.0,1,nindex,chnl);
   }
   s.pars.phAccum = ph; 
   fclose(fp);                                        
   free(pstub);
   return s;
}

//============================
// Initialize a Generic Shape
//============================

SHAPE genericInitShape(SHAPE s, char *name, double p, double phint, int iRec)
{
   char *var;
   if ((strlen(name) >= NSUFFIX) || (strlen(name)) < 2) {
      printf("Error in genericInitShape()! The  name %s is invalid !\n",name);
      psg_abort(-1);
   }
   OSPRINTF( s.pars.seqName, sizeof(s.pars.seqName), "%s", name);

// Obtain Phase Arguments

   s.pars.phAccum = p;
   s.pars.phInt = phint;
   s.pars.nRec = iRec;

// Supply INOVA Start Delays

   s.pars.preset1 = 0;
   s.pars.preset2 = 0;
   s.pars.strtdelay = WFG_START_DELAY;
   s.pars.offstdelay = WFG_OFFSET_DELAY;
   s.pars.apdelay = PWRF_DELAY;

//output channel

   var = getname0("ch",name,"");
   Getstr(var,s.pars.ch,sizeof(s.pars.ch));

//amplitude

   var = getname0("a",name,"");
   s.pars.a = getval(var);
   s.pars.array = disarry(var, s.pars.array);

//offset

   var = getname0("of",name,"");
   s.pars.of = getval(var);
   s.pars.array = disarry(var, s.pars.array);

//Create the Shapefile Name

   char lpattern[NPATTERN];
   var = getname0("",s.pars.seqName,"");
   OSPRINTF( lpattern, sizeof(lpattern), "%s%d", var, s.pars.nRec);
   s.pars.hasArray = hasarry(s.pars.array, lpattern);
   int lix = arryindex(s.pars.array);
   var = getname0("",s.pars.seqName,"");
   if (s.pars.calc > 0) {
      OSPRINTF( s.pars.pattern, sizeof(s.pars.pattern), "%s%d_%d", var, s.pars.nRec, lix);
      if (s.pars.hasArray == 1) {
         s = make_shape(s);
      }
      s.pars.t = gett(lix, lpattern);
   }
   return s;
}

//========================================================================
// Initialize a Generic Shape with no phase interpolation (used for DFS)
//========================================================================

SHAPE genericInitShape1(SHAPE s, char *name, double p, double phint, int iRec)
{
   char *var;
   if ((strlen(name) >= NSUFFIX) || (strlen(name)) < 2) {
      printf("Error in genericInitShape()! The  name %s is invalid !\n",name);
      psg_abort(-1);
   }
   OSPRINTF( s.pars.seqName, sizeof(s.pars.seqName), "%s", name);

// Obtain Phase Arguments

   s.pars.phAccum = p;
   s.pars.phInt = phint;
   s.pars.nRec = iRec;

// Supply INOVA Start Delays

   s.pars.preset1 = 0;
   s.pars.preset2 = 0;
   s.pars.strtdelay = WFG_START_DELAY;
   s.pars.offstdelay = WFG_OFFSET_DELAY;
   s.pars.apdelay = PWRF_DELAY;

//output channel

   var = getname0("ch",name,"");
   Getstr(var,s.pars.ch,sizeof(s.pars.ch));

//amplitude

   var = getname0("a",name,"");
   s.pars.a = getval(var);
   s.pars.array = disarry(var, s.pars.array);

//offset

   var = getname0("of",name,"");
   s.pars.of = getval(var);
   s.pars.array = disarry(var, s.pars.array);

//Create the Shapefile Name

   char lpattern[NPATTERN];
   var = getname0("",s.pars.seqName,"");
   OSPRINTF( lpattern, sizeof(lpattern), "%s%d", var, s.pars.nRec);
   s.pars.hasArray = hasarry(s.pars.array, lpattern);
   int lix = arryindex(s.pars.array);
   var = getname0("",s.pars.seqName,"");
   if (s.pars.calc > 0) {
      OSPRINTF( s.pars.pattern, sizeof(s.pars.pattern), "%s%d_%d", var, s.pars.nRec, lix);
      if (s.pars.hasArray == 1) {
         s = make_shape1(s);
      }
      s.pars.t = gett(lix, lpattern);
   }
   return s;
}

//======================================
// MPchopper() - Generalized MPchopper
//======================================

MPSEQ MPchopper(MPSEQ seq)
{

   FILE *fp;
   extern char userdir[];
   char shapepath[4*MAXSTR],str[MAXSTR+16];
   int npuls,iph,ntick,nrpuls,naccumpuls,nindex,nstub,nstep,chnl,ninit,ntotal,ntot;
   int i = 0;
   double phase,ph,t,dph,phCurrent,phLast,phOut,ph0Out,phInc,
          aOut,aInc,gOut;

// Set the channel.

  chnl = 0;
  if (!strcmp(seq.ch,"obs")) chnl = OBSch;
  else if (!strcmp(seq.ch,"dec")) chnl = DECch;
  else if (!strcmp(seq.ch,"dec2")) chnl = DEC2ch;
  else if (!strcmp(seq.ch,"dec3")) chnl = DEC3ch;
  else {
        printf("MPchopper() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
  }

// Set printmode for gate and amplitude defaults.

   int printmode = 1;
   if ((seq.ng == 1) && (seq.gateBase[0] ==  0.0)) printmode = 2;
   if ((printmode == 2) && (seq.na == 1) && (seq.aBase[0] ==  1023.0)) printmode = 3;

// Set gate = 1.0 for seq.trap > 0.

   if ((seq.trap > 0) && (seq.ng == 1)) seq.gateBase[0] = 1.0;

// Open the output file

   OSPRINTF( str, sizeof(str), "%s/shapelib/", userdir);
   OSPRINTF( shapepath, sizeof(shapepath), "%s%s.DEC", str, seq.pattern);
   if ( (fp = fopen(shapepath,"w")) == NULL) {
      printf("Error in open of %s \n",shapepath);
      psg_abort(1);
   }

// Round each step to a multiple of the minimum step, seq.90

   for (i = 0; i < seq.nphBase; i++) {
      seq.pw[i%seq.npw] = seq.n90*DTCK*(int)(seq.pw[i%seq.npw]/(seq.n90*DTCK) + 0.5);
      seq.n[i] = (int)(seq.pw[i%seq.npw]/DTCK + 0.5);
   }

// Save the times for the base cycle and total duration

   seq.telem = seq.pw[0];
   ntotal = seq.n[0];
   for(i=1;i<seq.nphBase;i++) {
      seq.telem += seq.pw[i%seq.npw];
      ntotal += seq.n[i%seq.npw];
   }
   seq.t = seq.telem*seq.nelem;
   npuls = seq.nphBase*seq.nelem;
   nstub = ntotal*seq.nelem/seq.n90;

// Create a waveform structure. 

   DECpattern *pstub;
   pstub = (DECpattern *) malloc(nstub*sizeof(struct _DECpattern));

//Save the total duration to the MODULE structure.

   char lpattern[NPATTERN];
   OSPRINTF( lpattern, sizeof(lpattern), "%s%d", getname0("",seq.seqName,""), seq.nRec);
   int lix = arryindex(seq.array);
   savet(seq.t, lix, lpattern);

// Get initial phase and super cycle step.

   ph = seq.phAccum;
   ph = ph + seq.phInt;
   seq.phInt = 0.0;
   iph = seq.iSuper; 

// Loop over the npuls pulses

   aOut = 0.0;
   aInc = 0.0;
   phOut = 0.0;
   ph0Out = 0.0;
   phInc = 0.0;
   gOut = 0.0;
   phLast = 0.0;
   phCurrent = 0.0;

   ntick = 0;
   naccumpuls = 0;
   nindex = 0;
   ntot = 0;
   nstep = 0;
   dph = 0.0;
   phase = 0.0;
   ninit = 0.0;
   t = 0.0;
   for (i = 0; i<npuls;i++) { 
      nrpuls = seq.n[i%seq.nphBase];

// Abort if nrpuls < (s.pars.n90*s.pars.n90m)

   if (nrpuls < seq.n90*seq.n90m) {
      printf("Error - Individual pulse width too small\n");  
      psg_abort(1);
   }

      naccumpuls = 0;
      ntick = 0;

// Determine the offset phase-step for each pulse.

      if (fabs(seq.of[i%seq.no]) > 1.0)
         dph = 360.0*seq.of[i%seq.no]*DTCK;
      else
         dph = 0.0;

// Set the supercycle step with first pulse.

      if ( i%seq.nphBase == 0 && i != 0) {
         iph = (iph + 1)%seq.nphSuper;
      }
 
// If dph = 0.0, write the pulse as a block.

      if (dph == 0.0) {
         phOut = roundphase(seq.phInt+ph+seq.phBase[i%seq.nph]+seq.phSuper[iph],360.0/(PSD*8192));
         phInc = 0.0;
         aOut = seq.aBase[i%seq.na];
         aInc = 0.0;
         gOut = seq.gateBase[i%seq.ng];
         if (seq.trap > 0) {
            if (seq.trap > 1) {
               fprintf(fp,"%10.1f %13.6f %11.6f %6.1f %13.6f %13.6f\n",(90.0/seq.n90)*seq.n[i%seq.nphBase],
                       phOut,aOut,gOut,phInc,aInc);
            }
            pstub[nindex].tip = (90.0/seq.n90)*seq.n[i%seq.nphBase];
            pstub[nindex].phase = phOut;
            pstub[nindex].phase_inc = phInc;
            pstub[nindex].amp = aOut;
            pstub[nindex].amp_inc = aInc;
            pstub[nindex].gate = gOut;
         }
         else {
            if (printmode == 1)
               fprintf(fp,"%10.1f %13.6f %13.6f %6.1f\n",(90.0/seq.n90)*seq.n[i%seq.nphBase],phOut,aOut,gOut);
            if (printmode == 2) 
               fprintf(fp,"%10.1f %13.6f %6.1f\n",(90.0/seq.n90)*seq.n[i%seq.nphBase],phOut,aOut);
            if (printmode == 3)
               fprintf(fp,"%10.1f %13.6f\n",(90.0/seq.n90)*seq.n[i%seq.nphBase],phOut);
         }
         nindex++;
         nstep = seq.n[(i%seq.nphBase) / seq.n90];
         ntot = ntot + nstep;
      }
      else {

// If dph > 0.0, apply a phase ramp. 

         nstep = 0; 
         ninit = 1;
         while (nrpuls > 0) {
            if (ninit > 0) {                 
               aOut = roundamp(seq.aBase[i%seq.na],1.0/WSD);
               aInc = 0.0;
               gOut = seq.gateBase[i%seq.ng];
               ph = ph + seq.phInt;
               phase = ph + seq.phBase[i%seq.nph] + seq.phSuper[iph];
               phLast = phase;       
               ninit = 0;                                                                                 
            }
            ntick++;
            nrpuls--;
            naccumpuls++;
            t += DTCK;
            seq.phInt += dph;

            if ((ntick == seq.n90) && (nrpuls >= seq.n90*(seq.n90m - 1))) {
               ntick = 0;
               nstep++; 
               ph =  ph + seq.phInt;                                   
               seq.phInt = 0.0;         
               phase = ph + seq.phBase[i%seq.nph] + seq.phSuper[iph];
               phCurrent = phase; 
               if (nrpuls == seq.n90*(seq.n90m - 1)) {
                  t += seq.n90*DTCK*(seq.n90m - 1);
                  nrpuls -= seq.n90*(seq.n90m - 1);
                  seq.phInt += seq.n90*dph*(seq.n90m - 1);
                  naccumpuls += seq.n90*(seq.n90m - 1);
                  ntick += seq.n90*(seq.n90m - 1);  
                  nstep += seq.n90m - 1;
                  ph =  ph + seq.phInt;
                  seq.phInt = 0.0;
                  phase = ph + seq.phBase[i%seq.nph] + seq.phSuper[iph];
                  phCurrent = phase; 
//                printf("phLast = %f phCurrent = %f\n",phLast,phCurrent);
                  ph0Out = roundphase(phLast,360.0/(PSD*8192));
                  phOut = roundphase((phCurrent + phLast)/2.0,360.0/(PSD*8192));
                  phInc = roundphaseinc((phCurrent - phLast)/nstep,360.0/(PSD*8192));
//                printf("ph0Out = %f phInc = %f\n",ph0Out,phInc);
                  if ((seq.trap > 0) && (nstep > 1)){  
                     if (seq.trap > 1) {
                        fprintf(fp,"%10.1f %13.6f %11.6f %6.1f %13.6f %13.6f\n",(90.0/seq.n90)*naccumpuls,
                             ph0Out,aOut,gOut,phInc,aInc);
                     }
                     pstub[nindex].tip = (90.0/seq.n90)*naccumpuls;
                     pstub[nindex].phase = ph0Out;
                     pstub[nindex].phase_inc = phInc;
                     pstub[nindex].amp = aOut;
                     pstub[nindex].amp_inc = aInc;
                     pstub[nindex].gate = gOut;
                  }
                  else {
                     if (printmode == 1)
                        fprintf(fp,"%10.1f %13.6f %13.6f %6.1f\n",(90.0/seq.n90)*naccumpuls,phOut,aOut,gOut);
                     if (printmode == 2)
                        fprintf(fp,"%10.1f %13.6f %13.6f\n",(90.0/seq.n90)*naccumpuls,phOut,aOut);
                     if (printmode == 3)
                        fprintf(fp,"%10.1f %13.6f\n",(90.0/seq.n90)*naccumpuls,phOut);
                  }
                  nindex++;
                  ntot = ntot + nstep;
//                printf("nindex = %d nstep = %d ntot = %d\n",nindex,nstep,ntot);
                  phLast = phCurrent;
                  naccumpuls = 0;
                  nstep = 0;
               }
               if (((nstep >= seq.n90m) && (fabs(phCurrent - phLast) >= DPH)) ||
                                           (nstep >= 255)) {
//                printf("nstep = %d n90m = %d\n",nstep,seq.n90m);
//                printf("phLast = %f phCurrent = %f\n",phLast,phCurrent);
                  ph0Out = roundphase(phLast,360.0/(PSD*8192));
                  phOut = roundphase((phCurrent + phLast)/2.0,360.0/(PSD*8192)); 
                  phInc = roundphaseinc((phCurrent - phLast)/nstep,360.0/(PSD*8192));
//                printf("ph0Out = %f phInc = %f\n",ph0Out,phInc);
                  if ((seq.trap > 0) && (nstep > 1)) { 
                     if (seq.trap > 1) {                              
                        fprintf(fp,"%10.1f %13.6f %11.6f %6.1f %13.6f %13.6f\n",(90.0/seq.n90)*naccumpuls,
                                ph0Out,aOut,gOut,phInc,aInc);
                     }
                     pstub[nindex].tip = (90.0/seq.n90)*naccumpuls;
                     pstub[nindex].phase = ph0Out;
                     pstub[nindex].phase_inc = phInc;
                     pstub[nindex].amp = aOut;
                     pstub[nindex].amp_inc = aInc;
                     pstub[nindex].gate = gOut;
                  }
                  else {
                     if (printmode == 1)
                        fprintf(fp,"%10.1f %13.6f %13.6f %6.1f\n",(90.0/seq.n90)*naccumpuls,phOut,aOut,gOut);
                     if (printmode == 2)
                        fprintf(fp,"%10.1f %13.6f %13.6f\n",(90.0/seq.n90)*naccumpuls,phOut,aOut);
                     if (printmode == 3)
                        fprintf(fp,"%10.1f %13.6f\n",(90.0/seq.n90)*naccumpuls,phOut);
                  }
                  nindex++;
                  ntot = ntot + nstep;
//                printf("nindex = %d nstep = %d ntot = %d\n",nindex,nstep,ntot);
                  phLast = phCurrent;
                  naccumpuls = 0;
                  nstep = 0;
               }
            }
         }
      }
   }  
 
// Make the DECShape and close the print file.

   if ((seq.trap > 0) && (ntot > 0)) {
      userDECShape(seq.pattern,pstub,90.0,1,nindex,chnl);
   }
   seq.phAccum = ph;
   fclose(fp);
   free(pstub);
   return seq;
}

//===============================================
// CP make_cp() - Build Original Tangent-Ramp CP
//===============================================

CP make_cp(CP cp)
{
   FILE *fp;
   extern char userdir[];
   char shapepath[4*MAXSTR],str[MAXSTR+16];
   int ntick,nstep,nrpuls,naccumpuls,nstub,nindex,chnl,ninit,ntot;
   double norm,mean,at,t,ph,dph,phCurrent,phLast,phOut,ph0Out,phInc,
          aCurrent,aLast,aOut,a0Out,aInc,gOut,phase;  

// Set the waveform on the designated channel
   chnl = 0;
   if (!strcmp(cp.ch,"fr")) {
      mean = cp.a1;
      if (!strcmp(cp.fr,"obs")) chnl = OBSch;
      else if (!strcmp(cp.fr,"dec")) chnl = DECch;
      else if (!strcmp(cp.fr,"dec2")) chnl = DEC2ch;
      else if (!strcmp(cp.fr,"dec3")) chnl = DEC3ch;
      else {
        printf("make_cp() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
      }
   }
   else {
      mean=cp.a2;
      if (!strcmp(cp.to,"obs")) chnl = OBSch;
      else if (!strcmp(cp.to,"dec")) chnl = DECch;
      else if (!strcmp(cp.to,"dec2")) chnl = DEC2ch;
      else if (!strcmp(cp.to,"dec3")) chnl = DEC3ch;
      else {
        printf("make_cp() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
      }
   }

   norm = 1023.0/(mean + fabs(cp.d));
   at = atan(cp.d/cp.b);

// Open the output file for print.

   OSPRINTF( str, sizeof(str), "%s/shapelib/", userdir);
   OSPRINTF( shapepath, sizeof(shapepath), "%s%s.DEC", str, cp.pattern);
   if ((fp = fopen(shapepath,"w")) == NULL) {
      printf("Error in open of %s \n",shapepath);
      psg_abort(1);
   }
// Round the total time to steps, determine total ticks and steps.
   
   cp.t = cp.n90*DTCK*(int)(cp.t/(cp.n90*DTCK) + 0.5);
   cp.n = (int)(cp.t/DTCK + 0.5);
   nstub = cp.n/cp.n90;

// Create a waveform structure. 

   DECpattern *pstub;
   pstub = (DECpattern *) malloc(nstub*sizeof(struct _DECpattern));

//  Save the total duration to the MODULE structure.

   char lpattern[NPATTERN];
   int lix = arryindex(cp.array);
   OSPRINTF( lpattern, sizeof(lpattern), "%s%d", getname0("",cp.seqName,""), cp.nRec);
   savet(cp.t, lix, lpattern);

// Initialize the time, phase and steps

   ph = cp.phAccum;
   nrpuls = cp.n;
   t = 0.0;

// Abort if nrpuls < (s.pars.n90*s.pars.n90m)

   if (nrpuls < cp.n90*cp.n90m) {
      printf("Error - Pulse width too small\n");  
      psg_abort(1);
   }

// Initialize indicies and output variables

   ntick = 0;
   nstep = 0;
   naccumpuls = 0;
   nrpuls = cp.n;
   nindex = 0;
   ntot = 0;

   aOut = 0.0;
   a0Out = 0.0;
   aInc = 0.0;
   phOut = 0.0;
   ph0Out = 0.0;
   phInc = 0.0; 
   gOut = 0.0;
   aLast = 0.0;
   phLast = 0.0;
   aCurrent = 0.0;
   phCurrent = 0.0;
   phase = 0.0;
   dph = 0.0;

// Set the phase increment for an offset

   if (fabs(cp.of) > 1.0) {
      dph = 360.0*cp.of*DTCK;
   }
   else {
      dph = 0.0;
   }

// Write "constant CP with no offset" as a single element

   if ( strcmp(cp.sh,"c") == 0 && fabs(cp.of) < 1.0) {
      aOut = 1023.0;
      aInc = 0.0;
      phOut = 0.0;
      phInc = 0.0;
      gOut = 1.0;
      if (cp.trap > 0) { 
         if (cp.trap > 1) {                                  
            fprintf(fp,"%10.1f %13.6f %11.6f %6.1f %13.6f %13.6f\n",(90.0/cp.n90)*cp.n,
                    phOut,aOut,gOut,phInc,aInc);
         }
         pstub[nindex].tip = (90.0/cp.n90)*naccumpuls;
         pstub[nindex].phase = ph0Out;
         pstub[nindex].phase_inc = phInc;
         pstub[nindex].amp = a0Out;
         pstub[nindex].amp_inc = aInc;
         pstub[nindex].gate = gOut;
      }
      else {
         fprintf(fp,"%10.1f %13.6f %13.6f\n",(90.0/cp.n90)*cp.n,phOut,aOut);
      }
   }
   else {

//Output shape and count ticks down, nrpul to zero.

      ninit = 1;
      while (nrpuls > 0) {
         if (ninit > 0) {                                                                       
            aLast = norm*(mean - cp.b*tan(at*(1.0 - 2.0*t/cp.t)));
            gOut = 1.0;
            ph = ph + cp.phInt;                         
            cp.phInt = 0.0;
            phLast = ph;  
            ninit = 0;                                                                             
         }
         ntick++;
         nrpuls--;
         naccumpuls++;
         t += DTCK;
         cp.phInt += dph;

         if ((ntick == cp.n90) && (nrpuls >= cp.n90*(cp.n90m - 1))) {
            ntick = 0;
            nstep++; 
            aCurrent = norm*(mean - cp.b*tan(at*(1.0 - 2.0*t/cp.t)));                    
            ph =  ph + cp.phInt;                                   
            cp.phInt = 0.0;         
            phase = ph;
            phCurrent = phase;  

            if (nrpuls == cp.n90*(cp.n90m - 1)) {                   
               t += cp.n90*DTCK*(cp.n90m - 1);         
               nrpuls -= cp.n90*(cp.n90m - 1);
               cp.phInt += cp.n90*dph*(cp.n90m - 1);
               naccumpuls += cp.n90*(cp.n90m - 1);
               ntick += cp.n90*(cp.n90m - 1);  
               nstep += cp.n90m - 1;
 
               aCurrent = norm*(mean - cp.b*tan(at*(1.0 - 2.0*t/cp.t)));          
               ph =  ph + cp.phInt;
//             cp.phInt = 0.0;   Carry phase ramp from last step.                           
               phase = ph;
               phCurrent = phase;
//             printf("aLast = %f aCurrent = %f\n",aLast,aCurrent);
               a0Out = roundamp(aLast,1.0/WSD);
               aOut = roundamp((aCurrent + aLast)/2.0,1.0/WSD);
               aInc = roundamp((aCurrent - aLast)/nstep,1.0/WSD);
//             printf("a0Out = %f aInc = %f\n",a0Out,aInc);
               ph0Out = roundphase(phLast,360.0/(PSD*8192));
               phOut = roundphase((phCurrent + phLast)/2.0,360.0/(PSD*8192));
               phInc = roundphaseinc((phCurrent - phLast)/nstep,360.0/(PSD*8192));
               if ((cp.trap > 0) && (nstep > 1)) { 
                  if (cp.trap > 1) {                                  
                     fprintf(fp,"%10.1f %13.6f %11.6f %6.1f %13.6f %13.6f\n",(90.0/cp.n90)*naccumpuls,
                             ph0Out,a0Out,gOut,phInc,aInc);
                  }
                  pstub[nindex].tip = (90.0/cp.n90)*naccumpuls;
                  pstub[nindex].phase = ph0Out;
                  pstub[nindex].phase_inc = phInc;
                  pstub[nindex].amp = a0Out;
                  pstub[nindex].amp_inc = aInc;
                  pstub[nindex].gate = gOut;
               }
               else {
                  fprintf(fp,"%10.1f %13.6f %13.6f\n",(90.0/cp.n90)*naccumpuls,phOut,aOut);
               }
               nindex++; 
               ntot = ntot + nstep;
//             printf("nindex = %d nstep = %d ntot = %d\n",nindex,nstep,ntot);
               aLast = aCurrent;                                       
               phLast = phCurrent;
               naccumpuls = 0;
               nstep = 0;
            }
            if (((nstep >= cp.n90m) && ((fabs(aCurrent - aLast) >= DWFM) || 
                                      (fabs(phCurrent - phLast) >= DPH)))||
                                      (nstep >= 255.0)) {
//             printf("aLast = %f aCurrent = %f\n",aLast,aCurrent);
               a0Out = roundamp(aLast,1.0/WSD);
               aOut =  roundamp((aCurrent + aLast)/2.0,1.0/WSD);  
               aInc = roundamp((aCurrent - aLast)/nstep,1.0/WSD);
//             printf("a0Out = %f aInc = %f\n",a0Out,aInc);
               ph0Out = roundphase(phLast,360.0/(PSD*8192));
               phOut = roundphase((phCurrent + phLast)/2.0,360.0/(PSD*8192)); 
               phInc = roundphaseinc((phCurrent - phLast)/nstep,360.0/(PSD*8192));
               if ((cp.trap > 0) && (nstep > 1)) { 
                  if (cp.trap > 1) {                       
                     fprintf(fp,"%10.1f %13.6f %13.6f %6.1f %13.6f %13.6f\n",(90.0/cp.n90)*naccumpuls,
                             ph0Out,a0Out,gOut,phInc,aInc);
                  }
                  pstub[nindex].tip = (90.0/cp.n90)*naccumpuls;
                  pstub[nindex].phase = ph0Out;
                  pstub[nindex].phase_inc = phInc;
                  pstub[nindex].amp = a0Out;
                  pstub[nindex].amp_inc = aInc;
                  pstub[nindex].gate = gOut;
               }
               else {
                 fprintf(fp,"%10.1f %13.6f %13.6f\n",(90.0/cp.n90)*naccumpuls,phOut,aOut);
               }
               nindex++;
               ntot = ntot + nstep;
//             printf("nindex = %d nstep = %d ntot = %d\n",nindex,nstep,ntot);
               aLast = aCurrent;
               phLast = phCurrent;
               naccumpuls = 0;
               nstep = 0;
            }
         }
      }
   }

// Make the DECShape and close the print file.

   if ((cp.trap > 0) && (strcmp(cp.sh,"c") != 0) && (ntot > 0)) {
      userDECShape(cp.pattern,pstub,90.0,1,nindex,chnl);
   }
   cp.phAccum = ph; 
   fclose(fp);                                        
   free(pstub);
   return cp;
}
