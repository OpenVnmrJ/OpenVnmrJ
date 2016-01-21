/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* include this file after solidsmiscs.h */
// original from D.H. Zhou and C. Reinstra 
// Modified and extended by ceb 2/1/05 -> present 
// Modified and extended by dmr 10/14/05 -> present

//=========================================
// Spectrometer constants
//=========================================

#define TWOPI 6.28318531
#define DTCK 12.5e-9
#define DPH    0.043945

//=============================================================
//  	            DECOUPLING SEQUENCES
//=============================================================
    
//=================================================
//   Make a TPPM waveform for DSEQ Decoupling
//=================================================

void make_tppm(TPPM dec) 
{
   char shapepath[MAXSTR],str[MAXSTR];
   FILE *fp;
   extern char userdir[];  
   sprintf(shapepath,"%s/shapelib/",userdir);
   sprintf(str,"%s%s.DEC",shapepath,dec.pattern);

   if((fp = fopen(str,"w"))==NULL){
      printf("Error in make_tppm(): can not create file %s!\n", str);
      exit(-1);
   }

   fprintf(fp,"#      length       phase   amplitude   gate \n\
             # ------------------------------------------------- \n\
        ");
	
   fprintf(fp," 90.0    %6.1f  %6.1f   \n", dec.ph, 1023.0);
   fprintf(fp," 90.0    %6.1f  %6.1f   \n", 360.0 - dec.ph, 1023.0);
   fclose(fp);
}

//======================================================
//  void _tppm(TPPM dec) - Start TPPM decoupling
//======================================================

void _tppm(TPPM dec)
{
   int chnl; double aTPPM=dec.a;
   aTPPM = dec.a;
   chnl = 0;    
   if (!strcmp(dec.ch,"obs")) chnl = 1;
   else if (!strcmp(dec.ch,"dec")) chnl = 2;
   else if (!strcmp(dec.ch,"dec2")) chnl = 3;
   else if (!strcmp(dec.ch,"dec3")) chnl = 4;
   else {
      printf("_tppm() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }
   if (dec.a > 0.0) {
      switch (chnl) {
         case 1: 
            obspwrf(aTPPM); 
	    obsunblank();
            xmtron();
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
	       obsprgon(dec.pattern,dec.pw,90.0);
            } 
	    break;
         case 2: 
            decpwrf(aTPPM);
	    decunblank();  
            decon();
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               decprgon(dec.pattern,dec.pw,90.0);
            } 
            break;
         case 3: 
            dec2pwrf(aTPPM);
	    dec2unblank(); 
            dec2on();
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               dec2prgon(dec.pattern,dec.pw,90.0);
            }
            break;
         case 4: 
            dec3pwrf(aTPPM); 
            dec3unblank();
            dec3on();
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) { 
               dec3prgon(dec.pattern,dec.pw,90.0);
            } 
            break;
         default: 
            printf("_tppm() Error: Undefined Channel! < 0!\n");
            psg_abort(1);
            break;
      }
   }       
}

//=================================================
//   Make a SPINAL waveform for DSEQ Decoupling
//=================================================

void make_spinal(SPINAL dec) 
{
   char shapepath[MAXSTR],str[MAXSTR];
   FILE *fp;
   extern char userdir[]; 
   int n,i;
   int sign[8] = {1,-1,1,1,-1,1,1,-1}; 
   sprintf(shapepath,"%s/shapelib/",userdir);
   sprintf(str,"%s%s.DEC",shapepath,dec.pattern); 
       
   if((fp = fopen(str,"w"))==NULL){
      printf("Error in make_spinal(): can not create file %s!\n", str);
      exit(-1);
   }

   fprintf(fp,"#      length       phase   amplitude   gate \n\
    # ------------------------------------------------- \n ");
       
   for (i = 0; i<8; i++){
      n = i%8;
      fprintf(fp," 90.0    %6.1f  %6.1f   \n", sign[n]*(dec.ph), 1023.0);
      fprintf(fp," 90.0    %6.1f  %6.1f   \n", sign[n]*(-dec.ph), 1023.0);
      fprintf(fp," 90.0    %6.1f  %6.1f   \n", sign[n]*(1.5*dec.ph), 1023.0);
      fprintf(fp," 90.0    %6.1f  %6.1f   \n", sign[n]*(-1.5*dec.ph), 1023.0);
      fprintf(fp," 90.0    %6.1f  %6.1f   \n", sign[n]*(2.0*dec.ph), 1023.0);
      fprintf(fp," 90.0    %6.1f  %6.1f   \n", sign[n]*(-2.0*dec.ph), 1023.0);
      fprintf(fp," 90.0    %6.1f  %6.1f   \n", sign[n]*(1.5*dec.ph), 1023.0);
      fprintf(fp," 90.0    %6.1f  %6.1f   \n", sign[n]*(-1.5*dec.ph), 1023.0);
   }	 
   fclose(fp);
}

//======================================================
//  void _spinal(SPINAL dec) - Start SPINAL decoupling
//======================================================

void _spinal(SPINAL dec)
{
   int chnl; 
   double aSPINAL;
   aSPINAL = dec.a;
   chnl = 0;
   if (!strcmp(dec.ch,"obs")) chnl = 1;
   else if (!strcmp(dec.ch,"dec")) chnl = 2;
   else if (!strcmp(dec.ch,"dec2")) chnl = 3;
   else if (!strcmp(dec.ch,"dec3")) chnl = 4;
   else {
      printf("_spinal() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }

   if (dec.a > 0.0) {
      switch (chnl) {
         case 1: 
            obspwrf(aSPINAL); 
            obsunblank();
            xmtron();
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) { 
               obsprgon(dec.pattern,dec.pw,90.0); 
            }
	    break;
         case 2: 
            decpwrf(aSPINAL); 
            decunblank();
            decon();
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) { 
               decprgon(dec.pattern,dec.pw,90.0);
            } 
	    break;
         case 3: 
            dec2pwrf(aSPINAL); 
            dec2unblank();
            dec2on(); 
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               dec2prgon(dec.pattern,dec.pw,90.0); 
            }
            break;
         case 4: 
            dec3pwrf(aSPINAL); 
            dec3unblank();
            dec3on();
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) { 
               dec3prgon(dec.pattern,dec.pw,90.0);
            } 
	    break;
         default: 
            printf("_spinal() Error: Undefined Channel! < 0!\n");
            psg_abort(1);
            break;
      }
   }     
}

//==============================================
// DSEQ Decoupling Sequence Utilities
//==============================================

//==============================================
// Select a decoupling sequence from panel
//==============================================

DSEQ getdseq(char *name) {
   DSEQ d;
   char var[13];
    
//Identify the sequence
   
   sprintf(var,name);
   strcat(var,"seq");
   Getstr(var,d.seq,sizeof(d.seq));
   if (!strcmp(d.seq,"tppm")) {
      d.t = gettppm(name);
   return d;
   }
   if (!strcmp(d.seq,"spinal")) {
      d.s = getspinal(name);
      return d; 
   }
   printf("getdseq() Error: Undefined Decoupling Sequence!%s \n",name);
   psg_abort(1);  	
}

//=========================================================
// Assign the decoupling sequence in the pulse sequence
//=========================================================

DSEQ setdseq(char *name, char *seq)
{
   DSEQ d;
   sprintf(d.seq,seq);
   if (!strcmp(d.seq,"tppm")){
      d.t = gettppm(name);
      return d;
   }
   if (!strcmp(d.seq,"spinal")){
      d.s = getspinal(name);
      return d; 
   }
   printf("setdseq() Error: Undefined Decoupling Sequence!%s \n",name);
   psg_abort(1);  
}

//==========================================================
// Turn On and Off a DSEQ
//==========================================================

void _dseqon(DSEQ dseq) {
   if (!strcmp(dseq.seq,"tppm")) {
      _tppm(dseq.t);
      return;
   }
   if (!strcmp(dseq.seq,"spinal")){
      _spinal(dseq.s);
      return;
   }
   printf("Error in _deseqon. Unrecognized Sequence! \n");
   psg_abort(1);
} 

void _dseqoff(DSEQ dseq) {
   
   if (!strcmp(dseq.seq,"tppm")) {
      if (dseq.t.a > 0.0) {
         if (!strcmp(dseq.t.ch,"obs")) {
            if ((dseq.t.pw > 0.5e-6) && (dseq.t.ph > 0.0)) {
               obsprgoff(); 
            }
            xmtroff(); 
            obsunblank(); 
         }
         if (!strcmp(dseq.t.ch,"dec")) {
            if ((dseq.t.pw > 0.5e-6) && (dseq.t.ph > 0.0)) {
               decprgoff(); 
            }
            decoff();
            decunblank();
         }
         if (!strcmp(dseq.t.ch,"dec2")) {
            if ((dseq.t.pw > 0.5e-6) && (dseq.t.ph > 0.0)) {
               dec2prgoff();
            } 
            dec2off();
            dec2unblank();
         } 
         if (!strcmp(dseq.t.ch,"dec3")) {
            if ((dseq.t.pw > 0.5e-6) && (dseq.t.ph > 0.0)) {
               dec3prgoff(); 
            }
            dec3off();
            dec3unblank();
         }
      }
   }
   if (!strcmp(dseq.seq,"spinal")) {
      if (dseq.s.a > 0.0) {
         if (!strcmp(dseq.s.ch,"obs"))  {
            if ((dseq.s.pw > 0.5e-6) && (dseq.s.ph > 0.0)) {
               obsprgoff(); 
            }
	    xmtroff();
	    obsunblank();
         }
         if (!strcmp(dseq.s.ch,"dec"))  {
            if ((dseq.s.pw > 0.5e-6) && (dseq.s.ph > 0.0)) {
               decprgoff(); 
            }
            decoff();
            decunblank();
         }
         if (!strcmp(dseq.s.ch,"dec2")) {
            if ((dseq.s.pw > 0.5e-6) && (dseq.s.ph > 0.0)) {
               dec2prgoff(); 
            }
	    dec2off();
	    dec2unblank();
         }
         if (!strcmp(dseq.s.ch,"dec3")) {
            if ((dseq.s.pw > 0.5e-6) && (dseq.s.ph > 0.0)) {
               dec3prgoff(); 
            }
            dec3off();
            dec3unblank();
         }
      }
   }   
} 

//==========================================================
//                 cp make_cp(CP cp)
//     Create pattern for constant, linear or tangent ramp
//==========================================================
 
CP make_cp(CP cp)
{
   FILE *fp;
   char shapepath[MAXSTR];
   extern char userdir[];
   int nmin = 16; 
   int ntick,nrpuls,naccumpuls; 
   double norm,mean;
   double at;
   double t;
   double ph,dph,aLast,aCurrent; 
   sprintf(shapepath,"%s/shapelib/%s.DEC",userdir,cp.pattern);
  
   if (!strcmp(cp.ch,"fr") ) {
      mean = cp.a1;
   }
   else {
      mean=cp.a2;
   }
   if (strcmp(cp.sh,"c") == 0) {
      cp.b = 1.0e12; 
      cp.d=0;
   } 
   else if (strcmp(cp.sh,"l") == 0) cp.b = 1.0e12;       
   norm = 1023.0/(mean + fabs(cp.d));              
 
   if (fabs(cp.of) > 1.0)
      dph = 360.0*cp.of*DTCK;           
   else 
      dph = 0.0;
       
   cp.n90 = nmin;                      
   cp.t = nmin*DTCK*(int)(cp.t/(nmin*DTCK) + 0.5); 
   cp.n = (int)(cp.t/DTCK + 0.5);                 
   
   if((fp = fopen(shapepath,"w"))==NULL) {
      printf("Error in make_cp(): can not create file %s!\n", shapepath);
      exit(-1);
   } 
   if ( strcmp(cp.sh,"c") == 0 && fabs(cp.of)  < 1.0) {
      fprintf(fp," %3.1f %3.3f %3.1f  \n ", 90.0*cp.n/nmin , 0.0,  1023.0 );
      fclose(fp);
      return cp;
   }   
   at = atan(cp.d/cp.b);                           
   nrpuls = cp.n;
   aCurrent = norm*(mean - cp.d);
   aCurrent = (double)( (int)(aCurrent + 0.5));
   aLast = aCurrent;
   ph = 0;
   t = 0;
   ntick = 0;
   naccumpuls = 0;
   while (nrpuls > 0) {
      if (nrpuls == cp.n) {       
         if (fabs(cp.phInt) >= DPH) {                         
            ph =  ph + cp.phInt;
            cp.phInt = 0;
         }
         fprintf(fp,"   %6.3f   %6.3f %6.1f \n",90.0, ph, aCurrent );
         t += (nmin - 1)*DTCK;
         nrpuls -= (nmin-1);
         cp.phInt += (nmin-1)*dph;
         naccumpuls = nmin - 1;
         ntick = nmin - 1;
         aLast = aCurrent;
         aCurrent = norm*(mean - cp.b*tan(at*(1.0 - 2.0*t/cp.t))); 
         aCurrent = (double)( (int) (aCurrent + 0.5) );
         continue;
      }
      ntick++;
      nrpuls--;
      naccumpuls++;
      t += DTCK;
      cp.phInt += dph;
      aCurrent = norm*(mean - cp.b*tan(at*(1.0 - 2.0*t/cp.t)));
      aCurrent = (double)( (int) (aCurrent + 0.5) );
      if (ntick == nmin) {              
         ntick = 0;
         if (nrpuls == nmin) {    
             if (fabs(cp.phInt) >= DPH) {                         
                 ph =  ph + cp.phInt;
                 cp.phInt = 0.0;
             }
             if ( fabs(aCurrent - aLast) >= 1.0) {
                 aLast = aCurrent;
             }
             fprintf(fp,"   %6.3f   %6.3f %6.1f \n", (90.0/nmin)*naccumpuls,ph,aCurrent);
             cp.phInt += nmin*dph;
             naccumpuls = 0;
             break;
         }
         if (fabs(cp.phInt) >= DPH || fabs(aCurrent - aLast) >= 1.0){
             ph = ph + cp.phInt;
             cp.phInt = 0.0;
             aLast = aCurrent;
             fprintf(fp,"   %6.3f   %6.3f %6.1f \n", (90.0/nmin)*naccumpuls,ph,aCurrent);
             naccumpuls = 0;
         }
      }
   }
   cp.phAccum = ph;
   fclose(fp);
   return cp;
}

//=============================================================
//   void _cp_(CP cp)
//   output constant, linear or tangent ramp
//=============================================================

void _cp_(CP cp)
{ 
   double aRamp,aConst;
   char  *chRamp, *chConst;
   int  nchRamp = 0, nchConst = 0;
   if (strcmp(cp.ch,"fr") == 0) {
      aRamp = cp.a1 + fabs(cp.d);
      aConst = cp.a2;
      chRamp = cp.fr; 
      chConst = cp.to;
   }
   else { 
      aRamp = cp.a2 + fabs(cp.d);  
      aConst = cp.a1;
      chRamp = cp.to; 
      chConst = cp.fr;
   }  
   if (strcmp(chRamp,"obs") == 0)       nchRamp = 1;
   else if (strcmp(chRamp,"dec") == 0)  nchRamp = 2;
   else if (strcmp(chRamp,"dec2") == 0) nchRamp = 3;
   else if (strcmp(chRamp,"dec3") == 0) nchRamp = 4;
   else {
      psg_abort(1);
   }
    
   if (strcmp(chConst,"obs") == 0)       nchConst = 1;
   else if (strcmp(chConst,"dec") == 0)  nchConst = 2;
   else if (strcmp(chConst,"dec2") == 0) nchConst = 3;
   else if (strcmp(chConst,"dec3") == 0) nchConst = 4;
   else {
      printf("Error from _cp_(): Invalid Const Channel");
      psg_abort(1);
   }      
   switch(nchConst) {
      case 1: 
         obspwrf(aConst);
         obsunblank();
         xmtron();    
         break;  
      case 2: 
         decpwrf(aConst);
         decunblank();
         decon();     
         break;  
      case 3: 
         dec2pwrf(aConst);
         dec2unblank();
         dec2on();   
         break;
      case 4: 
         dec3pwrf(aConst);
         dec3unblank();
         dec3on();   
         break;
   }
         
   switch(nchRamp) {
      case 1:  
         obspwrf(aRamp); xmtron();
         if ( strcmp(cp.sh,"c") == 0 && fabs(cp.of <= 1.0)){
	    obsunblank();
            delay(cp.t);
         } 
	 else {
            obsprgon(cp.pattern,cp.n90*12.5e-9,90.0);
            delay(cp.t);
            obsprgoff();
	    recoff();
         }
         xmtroff();
	 obsunblank();
         break;      
      case 2: 
         decpwrf(aRamp);decon(); 
         if ( strcmp(cp.sh,"c") == 0 && fabs(cp.of <= 1.0)){
	    decunblank();
            delay(cp.t);
         } 
	 else {
            decprgon(cp.pattern,cp.n90*12.5e-9,90.0);
            delay(cp.t);
            decprgoff();
         }
         decoff();
	 decunblank(); 
         break;
      case 3: 
         dec2pwrf(aRamp);dec2on(); 
         if ( strcmp(cp.sh,"c") == 0 && fabs(cp.of <= 1.0)){
	    dec2unblank();
            delay(cp.t);
         } 
	 else {
            dec2prgon(cp.pattern,cp.n90*12.5e-9,90.0);
            delay(cp.t);
            dec2prgoff();
         }
         dec2off();
	 dec2unblank(); 
         break;
      case 4: 
         dec3pwrf(aRamp);dec3on(); 
	 if ( strcmp(cp.sh,"c") == 0 && fabs(cp.of <= 1.0)){
	    dec3unblank();
            delay(cp.t);
         } 
	 else {
            dec3prgon(cp.pattern,cp.n90*12.5e-9,90.0);
            delay(cp.t);
            dec3prgoff();
         }
         dec3off();
	 dec3unblank(); 
         break;
      default: 
         printf("Error from _cp_(): Invalid Ramp Channel");
         psg_abort(1);
         break;
      }

   switch(nchConst) {
      case 1:  
         xmtroff(); 
         obsunblank();
	 break;
      case 2:  
         decoff(); 
         decunblank(); 
	 break;
      case 3:
         dec2off(); 
         dec2unblank();
         break;
      case 4:  
	 dec3off();
         dec3unblank(); 
	 break;
   } 
}  

//=======================================================================
// MPchopper() -  provides phase only
//=======================================================================


MPSEQ MPchopper(MPSEQ seq)
{
   FILE *fp;
   extern char userdir[];
   char shapepath[MAXSTR],str[MAXSTR];     
   int npuls,iph,ntick,nrpuls,naccumpuls;
   int i = 0;
   
//Establish the ticks in a minimum step, nmin, and save the value. 
  
   int nmin = 8;     // 100 ns step
   seq.n90 = nmin;

//Open the output file 
    
   sprintf(str,"%s/shapelib/",userdir);
   sprintf(shapepath,"%s%s.DEC",str,seq.pattern);         
   if ( (fp = fopen(shapepath,"w")) == NULL) {
      printf("Error in open of %s \n",shapepath);
      exit(-1);
   }

// Round each step to a multiple of the minimum step, nmin.
 
   for (i = 0; i < seq.nphBase; i++){
      seq.pw[i] = nmin*DTCK*(int)(seq.pw[i]/(nmin*DTCK) + 0.5);
      seq.n[i] = (int)(seq.pw[i]/DTCK + 0.5);
   }  
  
// Save the times for each pulse. 

   seq.telem = seq.pw[0];
   for(i=1;i<seq.nphBase;i++) {
      seq.telem += seq.pw[i];
   }
   
// Calculate total time and total pulses.

   seq.t = seq.telem*seq.nelem;
   npuls = seq.nphBase*seq.nelem; 
           
// Get initial phase and super cycle step.
   
   double ph = seq.phAccum;
   iph = seq.iSuper;  
   
// Determine the offset phase-step. 

   double dph = 0.0;  
   if (fabs(seq.of) > 1.0) dph = 360.0*seq.of*DTCK;
   	      
// Loop over the npuls pulses  
  
   ntick = 0;
   naccumpuls = 0;
    
   for (i = 0; i<npuls;i++) {
      nrpuls = seq.n[i%seq.nphBase];
      naccumpuls = 0;
      ntick = 0;

// Set the initial phase and supercycle step.
      
      if ( i%seq.nphBase == 0 && i != 0) {     
         iph = (iph + 1)%seq.nphSuper;
      }        
      if (fabs(seq.phInt) >= DPH){                         
         ph =  ph + seq.phInt;
         seq.phInt = 0.0;
      }
      
// If dph = 0.0, write the pulse as a block.
      
      if (dph == 0.0 ){                               
         fprintf(fp,"   %6.3f   %6.3f \n",(90.0/nmin)*seq.n[i%seq.nphBase],ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph]);
         continue;
      }
      
// If dph > 0.0, chop the pulse. 
 
      while(nrpuls > 0) {
         if (nrpuls == seq.n[i%seq.nphBase] ) {  // Always write the first step.
            nrpuls -= (nmin-1);
            if (fabs(seq.phInt) >= DPH) {                         
               ph =  ph + seq.phInt;
               seq.phInt = 0;
            }
            fprintf(fp,"   %6.3f   %6.3f \n",90.0, ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph]);
            seq.phInt += (nmin-1)*dph;   
            naccumpuls = nmin - 1;
            ntick = nmin - 1;     // Set nticks and running phase forward by nmin.
            continue;
         }
         ntick++;
         nrpuls--; 
         naccumpuls++;            // Increment the running duration between written steps.
         seq.phInt += dph;        // Increment the running phase between written steps. 
         if (ntick == nmin) {     // Decide whether to write a step at multiples of nmin.  	     
            ntick = 0;
            if (nrpuls == nmin) { // Always write the last step.
               if ( fabs(seq.phInt) >= DPH) { // Change ph if the delta-ph > DPH, else                       
                  ph =  ph + seq.phInt;       // leave a running remainder for the next pulse. 
                  seq.phInt = 0.0;
               }                 
               fprintf(fp,"   %6.3f   %6.3f \n", (90.0/nmin)*naccumpuls, ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph]);
               seq.phInt += nmin*dph; 
               naccumpuls = 0;
               break;
            }
            if (fabs(seq.phInt) >= DPH) { //Write a step if delta-ph > DPH, else                           
               ph =  ph + seq.phInt;      //accumulate the running phase.
               seq.phInt = 0;
               fprintf(fp,"   %6.3f   %6.3f  \n", (90.0/nmin)*naccumpuls, ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph]);
               naccumpuls = 0;
            }
         }   
      }
   }
   seq.phAccum = ph;  // Save the current phase for the next waveform. 
   fclose(fp);
   return seq;
}

//=======================================================================
// WMPchopper() provides phase and gate
//=======================================================================

WMPSEQ WMPchopper(WMPSEQ seq)
{
   FILE *fp;
   extern char userdir[];
   char shapepath[MAXSTR],str[MAXSTR];     
   int npuls, iph, ntick,nrpuls,naccumpuls;
   int i = 0;
   
//Establish the ticks in a minimum step, nmin, and save the value. 
    
   int nmin = 32;
   seq.n90 = nmin;

//Open the output file 
    
   sprintf(str,"%s/shapelib/",userdir);
   sprintf(shapepath,"%s%s.DEC",str,seq.pattern);            
   if ( (fp = fopen(shapepath,"w")) == NULL) {
      printf("Error in open of %s \n",shapepath);
      exit(-1);
   }

// Round each step to a multiple of the minimum step, nmin.
 
   for (i = 0; i < seq.nphBase; i++){
      seq.pw[i] = nmin*DTCK*(int)(seq.pw[i]/(nmin*DTCK) + 0.5);
      seq.n[i] = (int)(seq.pw[i]/DTCK + 0.5);
   }  
   
// Save the times for each pulse. 

   seq.telem = seq.pw[0];
   for(i=1;i<seq.nphBase;i++) {
      seq.telem += seq.pw[i];
   } 
   
// Calculate total time and total pulses. 
     
   seq.t = seq.telem*seq.nelem;
   npuls = seq.nphBase*seq.nelem;
           
// Get initial phase and super cycle step.
   
   double ph = seq.phAccum;
   iph = seq.iSuper;  

// Loop over the npuls pulses  
  
   ntick = 0;
   naccumpuls = 0;
   double dph = 0.0;
    
   for (i = 0; i<npuls;i++) {
      nrpuls = seq.n[i%seq.nphBase];
      naccumpuls = 0;
      ntick = 0;

// Determine the offset phase-step.
   
      if (fabs(seq.of) > 1.0)
         dph = 360.0*seq.of*DTCK;
      else
         dph = 0.0;	 
      
// Set the initial phase and supercycle step.
      
      if ( i%seq.nphBase == 0 && i != 0) {     
         iph = (iph + 1)%seq.nphSuper;
      }        
      if (fabs(seq.phInt) >= DPH){                         
         ph =  ph + seq.phInt;
         seq.phInt = 0.0;
      }
      
// If dph = 0.0, write the pulse as a block.
      
      if (dph == 0.0 ){                               
         fprintf(fp,"   %6.3f   %6.3f\n",(90.0/nmin)*seq.n[i%seq.nphBase],ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph], 1023.0, seq.gateBase[i%seq.nphBase]);
         continue;
      }
      
// If dph > 0.0, chop the pulse. 
 
      while(nrpuls > 0) {
         if (nrpuls == seq.n[i%seq.nphBase] ) {  // Always write the first step.
            nrpuls -= (nmin-1);
            if (fabs(seq.phInt) >= DPH) {                         
               ph =  ph + seq.phInt;
               seq.phInt = 0;
            }
            fprintf(fp,"   %6.3f   %6.3f\n",90.0, ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph], 1023.0, seq.gateBase[i%seq.nphBase]);
            seq.phInt += (nmin-1)*dph;   
            naccumpuls = nmin - 1;
            ntick = nmin - 1;     // Set nticks and running phase forward by nmin.
            continue;
         }
         ntick++;
         nrpuls--; 
         naccumpuls++;            // Increment the running duration between written steps.
         seq.phInt += dph;        // Increment the running phase between written steps. 
         if (ntick == nmin) {     // Decide whether to write a step at multiples of nmin.  	     
            ntick = 0;
            if (nrpuls == nmin) { // Always write the last step.
               if ( fabs(seq.phInt) >= DPH) { // Change ph if the delta-ph > DPH, else                       
                  ph =  ph + seq.phInt;       // leave a running remainder for the next pulse. 
                  seq.phInt = 0.0;
               }                 
               fprintf(fp,"   %6.3f   %6.3f\n", (90.0/nmin)*naccumpuls, ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph], 1023.0, seq.gateBase[i%seq.nphBase]);
               seq.phInt += nmin*dph; 
               naccumpuls = 0;
               break;	
            }
            if (fabs(seq.phInt) >= DPH) { //Write a step if delta-ph > DPH, else                           
               ph =  ph + seq.phInt;      //accumulate the running phase.
               seq.phInt = 0;
               fprintf(fp,"   %6.3f   %6.3f\n", (90.0/nmin)*naccumpuls, ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph], 1023.0, seq.gateBase[i%seq.nphBase]);
               naccumpuls = 0;
            }
         }   
      }
   }
   seq.phAccum = ph;  // Save the current phase for the next waveform. 
   fclose(fp);
   return seq;
}

//=======================================================================
// MPchopper1() provides phase and offset
//=======================================================================

MPSEQ1 MPchopper1(MPSEQ1 seq)
{
   FILE *fp;
   extern char userdir[];
   char shapepath[MAXSTR],str[MAXSTR];     
   int npuls, iph, ntick,nrpuls,naccumpuls;
   int i = 0;
   
//Establish the ticks in a minimum step, nmin, and save the value. 
  
   int nmin = 8;
   seq.n90 = nmin;

//Open the output file 
    
   sprintf(str,"%s/shapelib/",userdir);
   sprintf(shapepath,"%s%s.DEC",str,seq.pattern);         
   if ( (fp = fopen(shapepath,"w")) == NULL) {
      printf("Error in open of %s \n",shapepath);
      exit(-1);
   }

// Round each step to a multiple of the minimum step, nmin.
 
   for (i = 0; i < seq.nphBase; i++){
      seq.pw[i] = nmin*DTCK*(int)(seq.pw[i]/(nmin*DTCK) + 0.5);
      seq.n[i] = (int)(seq.pw[i]/DTCK + 0.5);
   }  
  
// Save the times for each pulse. 

   seq.telem = seq.pw[0];
   for(i=1;i<seq.nphBase;i++) {
      seq.telem += seq.pw[i];
   }
   
// Calculate total time and total pulses. 
     
   seq.t = seq.telem*seq.nelem;
   npuls = seq.nphBase*seq.nelem; 
           
// Get initial phase and super cycle step.
   
   double ph = seq.phAccum;
   iph = seq.iSuper;  

// Loop over the npuls pulses  
  
   ntick = 0;
   naccumpuls = 0;
   double dph = 0.0;
    
   for (i = 0; i<npuls;i++) {
      nrpuls = seq.n[i%seq.nphBase];
      naccumpuls = 0;
      ntick = 0;

// Determine the offset phase-step for each pulse. 
   
      if (fabs(seq.of[i%seq.nphBase]) > 1.0)
         dph = 360.0*seq.of[i%seq.nphBase]*DTCK;
      else
         dph = 0.0;	 
      
// Set the initial phase and supercycle step.
      
      if ( i%seq.nphBase == 0 && i != 0) {     
         iph = (iph + 1)%seq.nphSuper;
      }        
      if (fabs(seq.phInt) >= DPH){                         
         ph =  ph + seq.phInt;
         seq.phInt = 0.0;
      }
      
// If dph = 0.0, write the pulse as a block.
      
      if (dph == 0.0 ){                               
         fprintf(fp,"   %6.3f   %6.3f \n",(90.0/nmin)*seq.n[i%seq.nphBase],ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph]);
         continue;
      }
      
// If dph > 0.0, chop the pulse. 
 
      while(nrpuls > 0) {
         if (nrpuls == seq.n[i%seq.nphBase] ) {  // Always write the first step.
            nrpuls -= (nmin-1);
            if (fabs(seq.phInt) >= DPH) {                         
               ph =  ph + seq.phInt;
               seq.phInt = 0;
            }
            fprintf(fp,"   %6.3f   %6.3f \n",90.0, ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph]);
            seq.phInt += (nmin-1)*dph;   
            naccumpuls = nmin - 1;
            ntick = nmin - 1;     // Set nticks and running phase forward by nmin.
            continue;
         }
         ntick++;
         nrpuls--; 
         naccumpuls++;            // Increment the running duration between written steps.
         seq.phInt += dph;        // Increment the running phase between written steps. 
         if (ntick == nmin) {     // Decide whether to write a step at multiples of nmin.  	     
            ntick = 0;
            if (nrpuls == nmin) { // Always write the last step.
               if ( fabs(seq.phInt) >= DPH) { // Change ph if the delta-ph > DPH, else                       
                  ph =  ph + seq.phInt;       // leave a running remainder for the next pulse. 
                  seq.phInt = 0.0;
               }                 
               fprintf(fp,"   %6.3f   %6.3f \n", (90.0/nmin)*naccumpuls, ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph]);
               seq.phInt += nmin*dph; 
               naccumpuls = 0;
               break;
            }
            if (fabs(seq.phInt) >= DPH) { //Write a step if delta-ph > DPH, else                           
               ph =  ph + seq.phInt;      //accumulate the running phase.
               seq.phInt = 0;
               fprintf(fp,"   %6.3f   %6.3f  \n", (90.0/nmin)*naccumpuls, ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph]);
               naccumpuls = 0;
            }
         }   
      }
   }
   seq.phAccum = ph;  // Save the current phase for the next waveform. 
   fclose(fp);
   return seq;
}

//=======================================================================
// AWMPchopper1() provides amplitude, phase, gate and offset
//=======================================================================

AWMPSEQ1 AWMPchopper1(AWMPSEQ1 seq)
{
   FILE *fp;
   extern char userdir[];
   char shapepath[MAXSTR],str[MAXSTR];     
   int npuls, iph, ntick,nrpuls,naccumpuls;
   int i = 0;
   
// Obtain nmin ,the ticks per step, from seq.n90.

   int nmin = seq.n90;

// Open the output file 
    
   sprintf(str,"%s/shapelib/",userdir);
   sprintf(shapepath,"%s%s.DEC",str,seq.pattern);         
   if ( (fp = fopen(shapepath,"w")) == NULL) {
      printf("Error in open of %s \n",shapepath);
      exit(-1);
   }

// Round each step to a multiple of the minimum step, nmin.
 
   for (i = 0; i < seq.nphBase; i++) {
      seq.pw[i] = nmin*DTCK*(int)(seq.pw[i]/(nmin*DTCK) + 0.5);
      seq.n[i] = (int)(seq.pw[i]/DTCK + 0.5);
   }  
  
// Save the times for each pulse. 

   seq.telem = seq.pw[0];
   for(i=1;i<seq.nphBase;i++) {
      seq.telem += seq.pw[i];
   }
   
// Calculate total time and total pulses. 
     
   seq.t = seq.telem*seq.nelem;
   npuls = seq.nphBase*seq.nelem; 
           
// Get initial phase and super cycle step.
   
   double ph = seq.phAccum;
   iph = seq.iSuper;  

// Loop over the npuls pulses  
  
   ntick = 0;
   naccumpuls = 0;
   double dph = 0.0;
    
   for (i = 0; i<npuls;i++) {
      nrpuls = seq.n[i%seq.nphBase];
      naccumpuls = 0;
      ntick = 0;

// Determine the offset phase-step for each pulse. 
   
      if (fabs(seq.of[i%seq.nphBase]) > 1.0)
         dph = 360.0*seq.of[i%seq.nphBase]*DTCK;
      else
         dph = 0.0;	 
      
// Set the initial phase and supercycle step.
      
      if ( i%seq.nphBase == 0 && i != 0) {     
         iph = (iph + 1)%seq.nphSuper;
      }        
      if (fabs(seq.phInt) >= DPH){                         
         ph =  ph + seq.phInt;
         seq.phInt = 0.0;
      }
      
// If dph = 0.0, write the pulse as a block.
      
      if (dph == 0.0 ){                               
         fprintf(fp,"   %6.3f   %6.3f   %6.3f   %6.3f\n",(90.0/nmin)*seq.n[i%seq.nphBase],ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph],seq.aBase[i%seq.nphBase], seq.gateBase[i%seq.nphBase]);
         continue;
      }
      
// If dph > 0.0, chop the pulse. 
 
      while(nrpuls > 0) {
         if (nrpuls == seq.n[i%seq.nphBase] ) {  // Always write the first step.
            nrpuls -= (nmin-1);
            if (fabs(seq.phInt) >= DPH) {                         
               ph =  ph + seq.phInt;
               seq.phInt = 0;
            }
            fprintf(fp,"   %6.3f   %6.3f  %6.3f  %6.3f\n",90.0, ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph],seq.aBase[i%seq.nphBase], seq.gateBase[i%seq.nphBase]);
            seq.phInt += (nmin-1)*dph;   
            naccumpuls = nmin - 1;
            ntick = nmin - 1;     // Set nticks and running phase forward by nmin.
            continue;
         }
         ntick++;
         nrpuls--; 
         naccumpuls++;            // Increment the running duration between written steps.
         seq.phInt += dph;        // Increment the running phase between written steps. 
         if (ntick == nmin) {     // Decide whether to write a step at multiples of nmin.  	     
            ntick = 0;
            if (nrpuls == nmin) { // Always write the last step.
               if ( fabs(seq.phInt) >= DPH) { // Change ph if the delta-ph > DPH, else                       
                  ph =  ph + seq.phInt;       // leave a running remainder for the next pulse. 
                  seq.phInt = 0.0;
               }                 
               fprintf(fp,"   %6.3f   %6.3f  %6.3f  %6.3f\n", (90.0/nmin)*naccumpuls, ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph], seq.aBase[i%seq.nphBase], seq.gateBase[i%seq.nphBase]);
               seq.phInt += nmin*dph; 
               naccumpuls = 0;
               break;
            }
            if (fabs(seq.phInt) >= DPH) { //Write a step if delta-ph > DPH, else                           
               ph =  ph + seq.phInt;      //accumulate the running phase.
               seq.phInt = 0;
               fprintf(fp,"   %6.3f   %6.3f  %6.3f  %6.3f\n", (90.0/nmin)*naccumpuls, ph + seq.phBase[i%seq.nphBase] + seq.phSuper[iph], seq.aBase[i%seq.nphBase], seq.gateBase[i%seq.nphBase]);
               naccumpuls = 0;
            }
         }   
      }
   }

// Add an Amplitude Scale Step of 0.0 Duration. 

   fprintf(fp,"   %6.3f   %6.3f  %6.3f  %6.3f\n", 0.0, 0.0, 1023.0, 1.0);
  
   seq.phAccum = ph;  // Save the current phase for the next waveform. 
   fclose(fp);
   return seq;
}

//=================================================================
//    Output a MPSEQ - seq.t - phase only
//=================================================================

void _mpseq(MPSEQ seq, codeint phase)
{
   int chnl; 
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_mpseq() Error: Undefined Channel! < 0!\n");
      printf("Value of chnl = %d\n",chnl);
      psg_abort(1);
   }

   switch (chnl) {
      case 1:
         obspwrf(seq.a);  
         txphase(phase);
         obsunblank();
         xmtron();
         obsprgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 obsprgoff();            
         xmtroff();
	 obsunblank();
	 break;
      case 2: 
         decpwrf(seq.a);
         decphase(phase);
         decunblank();
         decon();
         decprgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 decprgoff();           
         decoff();
	 decunblank();
         break; 	
      case 3: 
         dec2pwrf(seq.a);       
         dec2phase(phase);
         dec2unblank();
	 dec2on();
         dec2prgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 dec2prgoff();            
         dec2off();
	 dec2unblank();
	 break;
      case 4: 
         dec3pwrf(seq.a);
         dec3phase(phase);
         dec3unblank();
         dec3on();         
         dec3prgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 dec3prgoff();           
         dec3off();
	 dec3unblank();
         break;
      default: 
        printf("_mpseq() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
        break;
   }       
}

//===================================================================
//  Output WMPSEQ - seq.t - phase and gate
//===================================================================

void _wmpseq(WMPSEQ seq,codeint phase)
{
   int chnl; 
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_wmpseq() Error: Undefined Channel! < 0!\n");
      printf("Value of chnl = %d\n",chnl);
      psg_abort(1);
   }
   switch (chnl) {
      case 1: 
         obspwrf(seq.a); 
         txphase(phase);
         obsunblank();
         xmtron();
         obsprgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 obsprgoff();            
         xmtroff();
	 obsunblank();
	 break;
      case 2: 
         decpwrf(seq.a);
         decphase(phase);
         decunblank();
         decon();
         decprgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 decprgoff();           
         decoff();
	 decunblank();
         break; 	
      case 3: 
         dec2pwrf(seq.a);      
         dec2phase(phase);
         dec2unblank();
	 dec2on();
         dec2prgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 dec2prgoff();            
         dec2off();
	 dec2unblank();
	 break;
      case 4: 
         dec3pwrf(seq.a);
         dec3phase(phase);
         dec3unblank();
         dec3on();         
         dec3prgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 dec3prgoff();           
         dec3off();
	 dec3unblank();
         break;
      default: 
        printf("_wmpseq() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
        break;
   }       
}

//===================================================================
//  Output AWMPSEQ1 - seq.t - amplitude, phase, gate and offset
//===================================================================

void _awmpseq1(AWMPSEQ1 seq,codeint phase)
{
   int chnl; 
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_awmpseq1() Error: Undefined Channel! < 0!\n");
      printf("Value of chnl = %d\n",chnl);
      psg_abort(1);
   }
   switch (chnl) {
      case 1: 
         obspwrf(seq.a); 
         txphase(phase);
         obsunblank();
         xmtron();
         obsprgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 obsprgoff();            
         xmtroff();
	 obsunblank();
	 break;
      case 2: 
         decpwrf(seq.a);
         decphase(phase);
         decunblank();
         decon();
         decprgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 decprgoff();           
         decoff();
	 decunblank();
         break; 	
      case 3: 
         dec2pwrf(seq.a);      
         dec2phase(phase);
         dec2unblank();
	 dec2on();
         dec2prgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 dec2prgoff();            
         dec2off();
	 dec2unblank();
	 break;
      case 4: 
         dec3pwrf(seq.a);
         dec3phase(phase);
         dec3unblank();
         dec3on();         
         dec3prgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 dec3prgoff();           
         dec3off();
	 dec3unblank();
         break;
      default: 
        printf("_awmpseq1() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
        break;
   }       
}

//===================================================================
//  Output MPSEQ1 - seq.t - phase and offset
//===================================================================

void _mpseq1(MPSEQ1 seq,codeint phase)
{
   int chnl; 
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_mpseq1() Error: Undefined Channel! < 0!\n");
      printf("Value of chnl = %d\n",chnl);
      psg_abort(1);
   }
   switch (chnl) {
      case 1: 
         obspwrf(seq.a); 
         txphase(phase);
         obsunblank();
         xmtron();
         obsprgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 obsprgoff();            
         xmtroff();
	 obsunblank();
	 break;
      case 2: 
         decpwrf(seq.a);
         decphase(phase);
         decunblank();
         decon();
         decprgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 decprgoff();           
         decoff();
	 decunblank();
         break; 	
      case 3: 
         dec2pwrf(seq.a);      
         dec2phase(phase);
         dec2unblank();
	 dec2on();
         dec2prgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 dec2prgoff();            
         dec2off();
	 dec2unblank();
	 break;
      case 4: 
         dec3pwrf(seq.a);
         dec3phase(phase);
         dec3unblank();
         dec3on();         
         dec3prgon(seq.pattern,seq.n90*12.5e-9,90.0);
         delay(seq.t);
	 dec3prgoff();           
         dec3off();
	 dec3unblank();
         break;
      default: 
        printf("_mpseq1() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
        break;
   }       
}
//===================================================================
//  Output MPSEQ for time t
//===================================================================

void _mpseqT(MPSEQ seq, double t, codeint phase)
{
   int chnl; 
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_mpseqT() Error: Undefined Channel! < 0!\n");
      printf("Value of chnl = %d\n",chnl);
      psg_abort(1);
   }
   switch (chnl) {
      case 1:
         obspwrf(seq.a);  
         txphase(phase);
         obsunblank();
         xmtron();
         obsprgon(seq.pattern,seq.n90*12.5e-9,90.0);
	 delay(t);
	 obsprgoff();          
         xmtroff();
	 obsunblank();
	 break;
      case 2: 
         decpwrf(seq.a);
         decphase(phase);
         decunblank();
         decon();
         decprgon(seq.pattern,seq.n90*12.5e-9,90.0);
	 delay(t);
	 decprgoff();            
         decoff();
	 decunblank();
         break; 	  
      case 3:
         dec2pwrf(seq.a);
         dec2phase(phase);
         dec2unblank();
         dec2on();        
         dec2prgon(seq.pattern,seq.n90*12.5e-9,90.0);
	 delay(t);
	 dec2prgoff();              
         dec2off();
	 dec2unblank();
	 break;
      case 4: 
         dec3pwrf(seq.a);
         dec3phase(phase);
         dec3unblank();
         dec3on();         
         dec3prgon(seq.pattern,seq.n90*12.5e-9,90.0);
	 delay(t);
	 dec3prgoff();            
         dec3off();
	 dec3unblank();
	 break;
      default: 
         printf("_mpseqT() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }       
}

//=================================================================
// On and Off for MPSEQ - phase only
//=================================================================

void _mpseqon(MPSEQ seq, codeint phase)
{
   int chnl; 
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_mpseqon() Error: Undefined Channel! < 0!\n");
      printf("Value of chnl = %d\n",chnl);
      psg_abort(1);
   }
   switch (chnl) {
      case 1: 
         obspwrf(seq.a); 
         txphase(phase);
         obsunblank();
         xmtron();
         obsprgon(seq.pattern,seq.n90*12.5e-9,90.0);          
	 break;
      case 2: 
         decpwrf(seq.a);
         decphase(phase);
         decunblank();
         decon();
         decprgon(seq.pattern,seq.n90*12.5e-9,90.0);         
         break; 	  
      case 3:
         dec2pwrf(seq.a);
         dec2phase(phase);
         dec2unblank();
         dec2on();        
         dec2prgon(seq.pattern,seq.n90*12.5e-9,90.0);;   
	 break;
      case 4: 
         dec3pwrf(seq.a);
         dec3phase(phase);
         dec3unblank(); 
         dec3on();         
         dec3prgon(seq.pattern,seq.n90*12.5e-9,90.0);       
	 break;
      default: 
         printf("_mpseqon() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }       
}

void _mpseqoff(MPSEQ seq)
{
   int chnl; 
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_mpseqoff() Error: Undefined Channel! < 0!\n");
      printf("Value of chnl = %d\n",chnl);
      psg_abort(1);
   }
   switch (chnl) {
      case 1:  
         obsprgoff();
         xmtroff();
	 obsunblank();
	 break;
      case 2: 
         decprgoff(); 
         decoff();
	 decunblank();
         break; 	  
      case 3:
         dec2prgoff();  
         dec2off();
	 dec2unblank();
	 break;
      case 4: 
         dec3prgoff();
         dec3off();
	 dec3unblank();
         break;
      default: 
         printf("_mpseqoff() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }       
}

//=================================================================
// On and Off for WMPSEQ - phase and gate
//=================================================================

void _wmpseqon(WMPSEQ seq, codeint phase)
{
   int chnl; 
   chnl = 0;
   printf("%s\n",seq.ch);
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_wmpseqon() Error: Undefined Channel! < 0!\n");
      printf("Value of chnl = %d\n",chnl);
      psg_abort(1);
   }
   switch (chnl) {
      case 1:
         obspwrf(seq.a); 
         txphase(phase);
         obsunblank();
         xmtron();
         obsprgon(seq.pattern,seq.n90*12.5e-9,90.0);          
	 break;
      case 2:
         decpwrf(seq.a);
         decphase(phase);
         decunblank();
         decon();
         decprgon(seq.pattern,seq.n90*12.5e-9,90.0);         
         break; 	  
      case 3:
         dec2pwrf(seq.a);
         dec2phase(phase);
         dec2unblank();
         dec2on();        
         dec2prgon(seq.pattern,seq.n90*12.5e-9,90.0);;   
	 break;
      case 4:
         dec3pwrf(seq.a);
         dec3phase(phase);
         dec3unblank(); 
         dec3on();         
         dec3prgon(seq.pattern,seq.n90*12.5e-9,90.0);       
	 break;
      default: 
         printf("_wmpseqon() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }       
}

void _wmpseqoff(WMPSEQ seq)
{
   int chnl; 
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_wmpseqoff() Error: Undefined Channel! < 0!\n");
      printf("Value of chnl = %d\n",chnl);
      psg_abort(1);
   }
   switch (chnl) {
      case 1:  
         obsprgoff();
         xmtroff();
	 obsunblank();
	 break;
      case 2: 
         decprgoff(); 
         decoff();
	 decunblank();
         break; 	  
      case 3:
         dec2prgoff();  
         dec2off();
	 dec2unblank();
	 break;
      case 4: 
         dec3prgoff();
         dec3off();
	 dec3unblank();
         break;
      default: 
         printf("_wmpseqoff() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }       
}

//=================================================================
// On and Off for MPSEQ1 - phase and offset
//=================================================================

void _mpseqon1(MPSEQ1 seq, codeint phase)
{ 
   int chnl; 
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_mpseqon1() Error: Undefined Channel! < 0!\n");
      printf("Value of chnl = %d\n",chnl);
      psg_abort(1);
   }
   switch (chnl) {
      case 1:
         obspwrf(seq.a);  
         txphase(phase);
         obsunblank();
         xmtron();
         obsprgon(seq.pattern,seq.n90*12.5e-9,90.0);          
	 break;
      case 2: 
         decpwrf(seq.a);
         decphase(phase);
         decunblank();
         decon();	
         decprgon(seq.pattern,seq.n90*12.5e-9,90.0);  	      
         break; 	  
      case 3:
         dec2pwrf(seq.a);
         dec2phase(phase);
         dec2unblank();
         dec2on();        
         dec2prgon(seq.pattern,seq.n90*12.5e-9,90.0);;   
	 break;
      case 4: 
         dec3pwrf(seq.a);
         dec3phase(phase);
         dec3unblank();
         dec3on();         
         dec3prgon(seq.pattern,seq.n90*12.5e-9,90.0);       
	 break;
      default: 
         printf("_mpseqon1() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }       
}

void _mpseqoff1(MPSEQ1 seq)
{
   int chnl; 
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_mpseqoff1() Error: Undefined Channel! < 0!\n");
      printf("Value of chnl = %d\n",chnl);
      psg_abort(1);
   }
   switch (chnl) {
      case 1:  
         obsprgoff();
         xmtroff();
	 obsunblank();
	 break;
      case 2: 
         decprgoff(); 
         decoff();
	 decunblank();
         break; 	  
      case 3:
         dec2prgoff();  
         dec2off();
	 dec2unblank();
	 break;
      case 4: 
         dec3prgoff();
         dec3off();
	 dec3unblank();
         break;
      default: 
         printf("_mpseqoff1() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }       
}

//=================================================================
// On and Off for AWMPSEQ1 - amplitude, phase, gate and offset
//=================================================================

void _awmpseqon1(AWMPSEQ1 seq, codeint phase)
{  
   int chnl; 
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_awmpseqon() Error: Undefined Channel! < 0!\n");
      printf("Value of chnl = %d\n",chnl);
      psg_abort(1);
   }
   switch (chnl) {
      case 1:
         obspwrf(seq.a);  
         txphase(phase);
         obsunblank();
         xmtron();
         obsprgon(seq.pattern,seq.n90*12.5e-9,90.0);          
	 break;
      case 2: 
         decpwrf(seq.a);
         decphase(phase);
         decunblank();
         decon();	
         decprgon(seq.pattern,seq.n90*12.5e-9,90.0);  	      
         break; 	  
      case 3:
         dec2pwrf(seq.a);
         dec2phase(phase);
         dec2unblank();
         dec2on();        
         dec2prgon(seq.pattern,seq.n90*12.5e-9,90.0);;   
	 break;
      case 4: 
         dec3pwrf(seq.a);
         dec3phase(phase);
         dec3unblank();
         dec3on();         
         dec3prgon(seq.pattern,seq.n90*12.5e-9,90.0);       
	 break;
      default: 
         printf("_awmpseqon() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }       
}

void _awmpseqoff1(AWMPSEQ1 seq)
{
   int chnl; 
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_awmpseqoff1() Error: Undefined Channel! < 0!\n");
      printf("Value of chnl = %d\n",chnl);
      psg_abort(1);
   }
   switch (chnl) {
      case 1:  
         obsprgoff();
         xmtroff();
	 obsunblank();
	 break;
      case 2: 
         decprgoff(); 
         decoff();
	 decunblank();
         break; 	  
      case 3:
         dec2prgoff();  
         dec2off();
	 dec2unblank();
	 break;
      case 4: 
         dec3prgoff();
         dec3off();
	 dec3unblank();
         break;
      default: 
         printf("_awmpseqoff1() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }       
}

