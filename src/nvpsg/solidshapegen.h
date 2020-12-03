/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef SOLIDSHAPEGEN_H
#define SOLIDSHAPEGEN_H

//###################  Please Note ################################
// The choppers, make_shape, MPchopper and make_cp have been moved
// to the new file solidchoppers.h. The above functions in this file
// have been renamed with an appende "4" so they will not be used 
// automatically. No other changes have been made to this file. 

// Also genericInitShape is found in solidhoppers.h and it has 
// been renamed"
//#################################################################  

//Contents:

//Functions used for INOVA and VNMRS

// initobswfg() - void   Preset HS Delay of obs Waveform
// setobswfg() - void    Preset Amplitude and Waveform on obs
// clearobswfg - void    Clear the obs Wavegen with obsprgoff
// initdecwfg() - void   Preset HS Delay of dec Waveform
// setdecwfg() - void    Preset Amplitude and Waveform on dec
// cleardecwfg - void    Clear the dec Wavegen with decprgoff
// initdec2wfg() - void  Preset HS Delay of dec2 Waveform
// setdec2wfg() - void   Preset Amplitude and Waveform on dec2
// cleardec2wfg - void   Clear the dec2 Wavegen with dec2prgoff
// initdec3wfg() - void  Preset HS Delay of dec3 Waveform
// setdec3wfg() - void   Preset Amplitude and Waveform on dec
// cleardec3wfg - void   Clear the dec3 Wavegen with dec3prgoff

// obswfgHSon() - void   Run the INOVA Wavegen on the obs Channel
// obswfgHSoff() - void  Stop the INOVA Wavegen on the obs Channel
// decwfgHSon() - void   Run the INOVA Wavegen on the dec Channel
// decwfgHSoff() - void  Stop the INOVA Wavegen on the dec Channel
// dec2wfHSgon() - void  Run the INOVA Wavegen on the dec2 Channel
// dec2wfgHSoff() - void Stop the INOVA Wavegen on the dec2 Channel
// dec3wfgHSon() - void  Run the INOVA Wavegen on the dec3 Channel
// dec3wfgHSoff() - void Stop the INOVA Wavegen on the dec3 Channel

// obswfgon() - void   Start INOVA or VNMRS obs Waveform with preset
// obswfgoff() - void  Stop INOVA or VNMRS obs Waveform with preset
// decwfgon() - void   Start INOVA or VNMRS dec Waveform with preset
// decwfgoff() - void  Stop INOVA or VNMRS dec Waveform with preset
// dec2wfgon() - void  Start INOVA or VNMRS dec2 Waveform with preset
// dec2wfgoff() - void Stop INOVA or VNMRS dec2 Waveform with preset
// dec3wfgon() - void  Start INOVA or VNMRS dec3 Waveform with preset
// dec3wfgoff() - void Start INOVA or VNMRS dec3 Waveform with preset

//Calculation of .DEC files

// make_cp() - CP              Build CP
// MPchopper()- MPSEQ          Multiple Pulse Sequence (phase)
// MPinitializer() - MPSEQ     Setup Memory for Phase, Amp, etc Lists
// make_ramp() - RAMP          Tangent RAMP
// update_ramp() - RAMP
// update_shape() - SHAPE
// update_cp() - CP
// update_mpseq() - MPSEQ
// make_shape() - SHAPE        Build a Shaped Pulse with STATE's
// genericInitShape() - SHAPE  Initialize a Shaped Pulse with STATE's

//  Delay-Adjust Functions for INOVA

// adjdelay() - void       Adjust Delay with Value
// adj_initcp_() - void    Adjust Delay for _initcp_
// adj_setcp_() - void     Adjust Delay for _setcp_
// adj_cp_() - CP          Adjust Delay for _cp_
// adj_clearcp_() - void   Adjust Delay for _clearcp_
// adj_initmpseq() - void  Adjust Delay for _initmpseq
// adj_setmpseq() - void   Adjust Delay for _setmpseq
// adj_mpseq() - MPSEQ     Adjust Delay for _mpseq
// adj_clearmpseq() - void Adjust Delay for _clearmpseq
// adj_initramp() - void   Adjust Delay for _initramp
// adj_setramp() - void    Adjust Delay for _setramp
// adj_ramp() - RAMP       Adjust Delay for _ramp
// adj_clearramp() - void  Adjust Delay for _clearramp
// adj_initshape() - void  Adjust Delay for _initshape
// adj_setshape() - void   Adjust Delay for _setshape
// adj_shape() - SHAPE     Adjust Delay for _shape
// adj_clearshape() - void Adjust Delay for _clearshape

// Preset _underscore Functions for INOVA

// _initcp_() - void    Initialize HS Amplitude for CP
// _setcp_() - void     Set Ramped-CP Waveform
// _clearcp_() - void   Clear Ramped-CP Waveform
// _initmpseq() - void  Initialize HS Ampplitude for MPSEQ
// _setmpseq() - void   Set an MPSEQ Waveform
// _clearmpseq() - void Clear an MPSEQ Waveform
// _initramp() - void   Initialize HS Ampplitude for RAMP
// _setramp() - void    Set an RAMP Waveform
// _clearramp() - void  Clear an RAMP Waveform
// _initshape() - void  Initialize HS Ampplitude for SHAPE
// _setshape() - void   Set an SHAPE Waveform
// _clearshape() - void Clear an SHAPE Waveform

// Run time _underscore functions

// _cp_() - void               Run Ramped CP
// _mpseq() - void             Run an MPSEQ for time "t"
// _mpseqT() - void            Run an MPSEQ for time "t" where "t" is an argument
// _mpseqon() - void           Turn On an MPSEQ
// _mpseqoff() - void          Turn Off an MPSEQ
// _shape() - void             Run a generic SHAPE for time "t"
// _shapeon() - void           Turn On a SHAPE
// _shapeoff() - void          Turn Off a SHAPE
// _ramp - void                Run a Tangent RAMP
// _rampon - void              Turn On a RAMP
// _rampoff - void             Turn Off a RAMP
// _dream - void               Run a set of four DREAM tangent ramps.

// Functions to write structure contents

// dump_mpseq() - void         List the contents of MPSEQ
// dump_mpseq_array() - void   List the "array" structure of MPSEQ
// dump_cp() - void            List the contents of CP
// dump_cp_array() - void      List the "array" structure of CP
// dump_ramp() - void          List the contents of RAMP
// dump_ramp_array() - void    List the "array" structure of RAMP
// dump_shape() - void         List the contents of SHAPE
// dump_shape_array() - void   List the "array" structure of SHAPE

//=================================
// Wavegen Functions used by INOVA
//=================================

#define CH1WG 0x4     // Address of each Wavegen
#define CH2WG 0x80
#define CH3WG 0x1000
#define CH4WG 0x20000

extern void HSgate(int ch, int state);
extern void dps_on();
extern void dps_off();

void obswfgHSon()
{
   if (OBSch == 1) HSgate(CH1WG,TRUE);
   if (OBSch == 2) HSgate(CH2WG,TRUE);
   if (OBSch == 3) HSgate(CH3WG,TRUE);
   if (OBSch == 4) HSgate(CH4WG,TRUE);
}

void obswfgHSoff()
{
   if (OBSch == 1) HSgate(CH1WG,FALSE);
   if (OBSch == 2) HSgate(CH2WG,FALSE);
   if (OBSch == 3) HSgate(CH3WG,FALSE);
   if (OBSch == 4) HSgate(CH4WG,FALSE);
}

void decwfgHSon()
{
   if (DECch == 1) HSgate(CH1WG,TRUE);
   if (DECch == 2) HSgate(CH2WG,TRUE);
   if (DECch == 3) HSgate(CH3WG,TRUE);
   if (DECch == 4) HSgate(CH4WG,TRUE);
}

void decwfgHSoff()
{
   if (PWRF_DELAY > 0.0) {
      if (DECch == 1) HSgate(CH1WG,FALSE);
      if (DECch == 2) HSgate(CH2WG,FALSE);
      if (DECch == 3) HSgate(CH3WG,FALSE);
      if (DECch == 4) HSgate(CH4WG,FALSE);
   }
   else decprgoff();   
}

void dec2wfgHSon()
{
   if (DEC2ch == 1) HSgate(CH1WG,TRUE);
   if (DEC2ch == 2) HSgate(CH2WG,TRUE);
   if (DEC2ch == 3) HSgate(CH3WG,TRUE);
   if (DEC2ch == 4) HSgate(CH4WG,TRUE);
}

void dec2wfgHSoff()
{
   if (DEC2ch == 1) HSgate(CH1WG,FALSE);
   if (DEC2ch == 2) HSgate(CH2WG,FALSE);
   if (DEC2ch == 3) HSgate(CH3WG,FALSE);
   if (DEC2ch == 4) HSgate(CH4WG,FALSE);
}

void dec3wfgHSon()
{
   if (DEC3ch == 1) HSgate(CH1WG,TRUE);
   if (DEC3ch == 2) HSgate(CH2WG,TRUE);
   if (DEC3ch == 3) HSgate(CH3WG,TRUE);
   if (DEC3ch == 4) HSgate(CH4WG,TRUE);
}

void dec3wfgHSoff()
{
   if (DEC3ch == 1) HSgate(CH1WG,FALSE);
   if (DEC3ch == 2) HSgate(CH2WG,FALSE);
   if (DEC3ch == 3) HSgate(CH3WG,FALSE);
   if (DEC3ch == 4) HSgate(CH4WG,FALSE);
}

void initobswfg(lpattern,lstep,ldres,lamplitude)
double lstep,ldres,lamplitude;
char lpattern[MAXSTR];
{ 
   if (PWRF_DELAY > 0.0) {
      obspwrf(lamplitude);
      obsprgon(lpattern,lstep,ldres);
      delay(WFG_START_DELAY - WFG_OFFSET_DELAY);
      obsprgoff();
   }
}

void setobswfg(lpattern,lstep,ldres,lamplitude)
double lstep,ldres,lamplitude;
char lpattern[MAXSTR];
{  
   if (PWRF_DELAY > 0.0) {
      obspwrf(lamplitude);
      obsprgon(lpattern, lstep, ldres); 
      obswfgHSoff();
   }
}

void clearobswfg()
{  if (PWRF_DELAY > 0.0) {
      obsprgoff();
   }
}

void initdecwfg(lpattern,lstep,ldres,lamplitude)
double lstep,ldres,lamplitude;
char lpattern[MAXSTR];
{        
   decpwrf(lamplitude);
   if (PWRF_DELAY > 0.0) {
      decprgon(lpattern,lstep,ldres);
      delay(WFG_START_DELAY - WFG_OFFSET_DELAY);
      decprgoff();
   }
}

void setdecwfg(lpattern,lstep,ldres,lamplitude)
double lstep,ldres,lamplitude;
char lpattern[MAXSTR];
{      
   decpwrf(lamplitude);
   if (PWRF_DELAY > 0.0) {
      decprgon(lpattern, lstep, ldres);
      decwfgHSoff();
   } 
}

void cleardecwfg()
{   
   if (PWRF_DELAY > 0.0) {
      decprgoff();
   }
}

void initdec2wfg(lpattern,lstep,ldres,lamplitude)
double lstep,ldres,lamplitude;
char lpattern[MAXSTR];
{         
   dec2pwrf(lamplitude);
   if (PWRF_DELAY > 0.0) {
      dec2prgon(lpattern,lstep,ldres);
      delay(WFG_START_DELAY - WFG_OFFSET_DELAY);
      dec2prgoff();
   }
}

void setdec2wfg(lpattern,lstep,ldres,lamplitude)
double lstep,ldres,lamplitude;
char lpattern[MAXSTR];
{        
   dec2pwrf(lamplitude);
   if (PWRF_DELAY > 0.0) {
      dec2prgon(lpattern, lstep, ldres); 
      dec2wfgHSoff();
   }
}

void cleardec2wfg()
{   
   if (PWRF_DELAY > 0.0) {
      dec2prgoff(); 
   }
}

void initdec3wfg(lpattern,lstep,ldres,lamplitude)
double lstep,ldres,lamplitude;
char lpattern[MAXSTR];
{        
   dec3pwrf(lamplitude); 
   if (PWRF_DELAY > 0.0) {
      dec3prgon(lpattern,lstep,ldres);
      delay(WFG_START_DELAY - WFG_OFFSET_DELAY);
      dec3prgoff();
   }
}

void setdec3wfg(lpattern,lstep,ldres,lamplitude)
double lstep,ldres,lamplitude;
char lpattern[MAXSTR];
{        
   dec3pwrf(lamplitude);
   if (PWRF_DELAY > 0.0) {
      dec3prgon(lpattern, lstep, ldres);
      dec3wfgHSoff();
   }
}

void cleardec3wfg()
{
   dec3prgoff();
}

void obswfgon(lpattern,lstep,ldres,lpreset,lpredelay)
char lpattern[MAXSTR]; 
int lpreset;
double lstep,ldres,lpredelay;
{  
   if (PWRF_DELAY > 0.0) {
      if (lpreset == 0)
         obsprgon(lpattern,lstep,ldres);
      else obswfgHSon();
      dps_off(); delay(lpredelay); dps_on();
   }
   else obsprgon(lpattern,lstep,ldres);
}

void obswfgoff(lpreset)
int lpreset;
{
   if (PWRF_DELAY > 0.0) {
      if (lpreset == 0) obsprgoff();
      else obswfgHSoff();
   }
   else obsprgoff(); 
}

void decwfgon(lpattern,lstep,ldres,lpreset,lpredelay)
char lpattern[MAXSTR];
int lpreset;
double lstep,ldres,lpredelay;
{
   if (PWRF_DELAY > 0.0) {
      if (lpreset == 0) decprgon(lpattern,lstep,ldres);
      else decwfgHSon();
      dps_off(); delay(lpredelay); dps_on();
   }
   else decprgon(lpattern,lstep,ldres);
}

void decwfgoff(lpreset)
int lpreset;
{   
   if (PWRF_DELAY > 0.0) {
      if (lpreset == 0) decprgoff();
      else decwfgHSoff();
   }
   else decprgoff(); 
}

void dec2wfgon(lpattern,lstep,ldres,lpreset,lpredelay)
char lpattern[MAXSTR];
int lpreset;
double lstep,ldres,lpredelay;
{   
   if (PWRF_DELAY > 0.0) {
      if (lpreset == 0) dec2prgon(lpattern,lstep,ldres);
      else dec2wfgHSon();
      dps_off(); delay(lpredelay); dps_on();
   }
   else dec2prgon(lpattern,lstep,ldres);
}

void dec2wfgoff(lpreset)
int lpreset;
{   
   if (PWRF_DELAY > 0.0) {
      if (lpreset == 0) dec2prgoff();
      else dec2wfgHSoff();
   }   
   else dec2prgoff(); 
}

void dec3wfgon(lpattern,lstep,ldres,lpreset,lpredelay)
char lpattern[MAXSTR];
int lpreset;
double lstep,ldres,lpredelay;
{   
   if (PWRF_DELAY > 0.0) {
      if (lpreset == 0) dec3prgon(lpattern,lstep,ldres);
      else dec3wfgHSon();
      dps_off(); delay(lpredelay); dps_on();
   }
   else dec3prgon(lpattern,lstep,ldres); 
}

void dec3wfgoff(lpreset)
int lpreset;
{   
   if (PWRF_DELAY > 0.0) {
      if (lpreset == 0) dec3prgoff();
      else dec3wfgHSoff();
   }   
   else dec3prgoff(); 
}

//===============================================
// CP make_cp() - Build Original Tangent-Ramp CP
//===============================================

#ifndef NVPSG
CP make_cp(CP cp)
{
   FILE *fp;
   char shapepath[MAXSTR*3];
   extern char userdir[];
   int nmin = cp.n90;
   int ntick,nrpuls,naccumpuls,chnl;
   double norm,mean;
   double at;
   double t;
   double ph,dph,aLast,aCurrent;
   sprintf(shapepath,"%s/shapelib/%s.DEC",userdir,cp.pattern);

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

   if (fabs(cp.of) > 1.0)
      dph = 360.0*cp.of*DTCK;
   else
      dph = 0.0;

   cp.n90 = nmin;
   cp.t = nmin*DTCK*(int)(cp.t/(nmin*DTCK) + 0.5);
   cp.n = (int)(cp.t/DTCK + 0.5);

   char lpattern[NPATTERN];
   int lix = arryindex(cp.array);
   sprintf(lpattern,"%s%d",getname0("",cp.seqName,""),cp.nRec);
   savet(cp.t, lix, lpattern); //Save the total duration to the MODULE structure. 

   if((fp = fopen(shapepath,"w"))==NULL) {
      printf("Error in make_cp(): can not create file %s!\n", shapepath);
      psg_abort(1);
   }
   if ( strcmp(cp.sh,"c") == 0 && fabs(cp.of)  < 1.0) {
      fprintf(fp,"%10.1f %10.3f %6.1f\n", 90.0*cp.n/nmin,0.0,1023.0);
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
         fprintf(fp,"%10.1f %10.3f %6.1f\n",90.0,ph,aCurrent);
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
             fprintf(fp,"%10.1f %10.3f %6.1f\n",(90.0/nmin)*naccumpuls,ph,aCurrent);
             cp.phInt += nmin*dph;
             naccumpuls = 0;
             break;
         }
         if (fabs(cp.phInt) >= DPH || fabs(aCurrent - aLast) >= 1.0) {
             ph = ph + cp.phInt;
             cp.phInt = 0.0;
             aLast = aCurrent;
             fprintf(fp,"%10.1f %10.3f %6.1f\n",(90.0/nmin)*naccumpuls,ph,aCurrent);
             naccumpuls = 0;
         }
      }
   }
   cp.phAccum = ph;
   fclose(fp);
   return cp;
}
#endif

//==========================================================
// _initcp_(CP cp) - Initialize HS Amplitude for _cp_
//==========================================================

void _initcp_(CP cp, double lamplitude)
{
   char *chRamp;
   int  nchRamp = 0;

   if (strcmp(cp.ch,"fr") == 0)
      chRamp = cp.fr;
   else chRamp = cp.to;

   if (strcmp(chRamp,"obs") == 0)       nchRamp = 1;
   else if (strcmp(chRamp,"dec") == 0)  nchRamp = 2;
   else if (strcmp(chRamp,"dec2") == 0) nchRamp = 3;
   else if (strcmp(chRamp,"dec3") == 0) nchRamp = 4;
   else {
      printf("Error from _initcp_(): Invalid Ramp Channel!\n");
      psg_abort(1);
   }

   dps_off();
   switch(nchRamp) {
      case 1:
         obsunblank();
         if (cp.t > 0.0) {
            if ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0)) {
               initobswfg("PRESET",5.0e-6,90.0,lamplitude);
            }
	 }
         break;
      case 2:
         decunblank();
         if (cp.t > 0.0) {
            if ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0)) {
               initdecwfg("PRESET",5.0e-6,90.0,lamplitude);
            }
	 }
         break;
      case 3:
         dec2unblank();
	 if (cp.t > 0.0) {
            if ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0)) {
               initdec2wfg("PRESET",5.0e-6,90.0,lamplitude);
            }
	 }
         break;
      case 4:
         dec3unblank();
	 if (cp.t > 0.0) {
            if ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0)) {
               initdec3wfg("PRESET",5.0e-6,90.0,lamplitude);
            }
	 }
         break;
      default:
         printf("Error from _initcp_(): Invalid Ramp Channel!\n");
         psg_abort(1);
         break;
   }
   dps_on();
}

//==========================================================
// _setcp_(CP cp) - Preset Wavegen for _cp_
//==========================================================

void _setcp_(CP cp)
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

      printf("Error from _setcp_(): Invalid Ramp Channel!\n");
      psg_abort(1);
   }

   if (strcmp(chConst,"obs") == 0)       nchConst = 1;
   else if (strcmp(chConst,"dec") == 0)  nchConst = 2;
   else if (strcmp(chConst,"dec2") == 0) nchConst = 3;
   else if (strcmp(chConst,"dec3") == 0) nchConst = 4;
   else {
      printf("Error from _setcp_(): Invalid Const Channel!\n");
      psg_abort(1);
   }

   dps_off();
   switch(nchConst) {
      case 1:
         obsunblank();
         break;
      case 2:
         decunblank();
         break;
      case 3:
         dec2unblank();
         break;
      case 4:
         dec3unblank();
         break;
      default:
         printf("Error from _setcp_(): Invalid Const Channel!\n");
         psg_abort(1);
         break;
   }

   switch(nchRamp) {
      case 1:
         obsunblank();
         break;
      case 2:
         decunblank();
         break;
      case 3:
         dec2unblank();
         break;
      case 4:
         dec3unblank();
         break;
      default:
         printf("Error from _setcp_(): Invalid Ramp Channel!\n");
         psg_abort(1);
         break;
   }

   switch(nchConst) {
      case 1:
         obspwrf(aConst);
         break;
      case 2:
         decpwrf(aConst);
         break;
      case 3:
         dec2pwrf(aConst);
         break;
      case 4:
         dec3pwrf(aConst);
         break;
      default:
         printf("Error from _setcp_(): Invalid Const Channel!\n");
         psg_abort(1);
      break;
   }

   switch(nchRamp) {
      case 1:
         if ((cp.t > 0.0) && ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0))) {
            setobswfg(cp.pattern,cp.n90*12.5e-9,90.0,aRamp);
         }
         else {
            obspwrf(aRamp);
         }
         break;
      case 2:
         if ((cp.t > 0.0) && ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0))) {
            setdecwfg(cp.pattern,cp.n90*12.5e-9,90.0,aRamp);
         }
         else {
            decpwrf(aRamp);
         }
         break;
      case 3:
         if ((cp.t > 0.0) && ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0))) {
            setdec2wfg(cp.pattern,cp.n90*12.5e-9,90.0,aRamp);
         }
         else {
            dec2pwrf(aRamp);
	 }
         break;
      case 4:
         if ((cp.t > 0.0) && ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0))) {
            setdec3wfg(cp.pattern,cp.n90*12.5e-9,90.0,aRamp);
         }
         else {
            dec3pwrf(aRamp);
         }
         break;
      default:
         printf("Error from _setcp_(): Invalid Ramp Channel!\n");
         psg_abort(1);
         break;
   }
   dps_on();
}

//==============================================================
// _cp_(CP cp int phase1, int phase2) - Original _cp_ Module
//==============================================================
         
void _cp_(CP cp, int phase1, int phase2)
{
   double aRamp,aConst;
   char  *chRamp, *chConst;
   int cphase, rphase;
   int  nchRamp = 0, nchConst = 0;
   if (strcmp(cp.ch,"fr") == 0) {
      aRamp = cp.a1 + fabs(cp.d);
      aConst = cp.a2;
      chRamp = cp.fr;
      chConst = cp.to;
      rphase = phase1;
      cphase = phase2;
   }
   else {
      aRamp = cp.a2 + fabs(cp.d);
      aConst = cp.a1;
      chRamp = cp.to;
      chConst = cp.fr;
      rphase = phase2;
      cphase = phase1;
   }

   if (strcmp(chRamp,"obs") == 0)       nchRamp = 1;
   else if (strcmp(chRamp,"dec") == 0)  nchRamp = 2;
   else if (strcmp(chRamp,"dec2") == 0) nchRamp = 3;
   else if (strcmp(chRamp,"dec3") == 0) nchRamp = 4;
   else {

      printf("Error from _cp_(): Invalid Ramp Channel!\n");
      psg_abort(1);
   }

   if (strcmp(chConst,"obs") == 0)       nchConst = 1;
   else if (strcmp(chConst,"dec") == 0)  nchConst = 2;
   else if (strcmp(chConst,"dec2") == 0) nchConst = 3;
   else if (strcmp(chConst,"dec3") == 0) nchConst = 4;
   else {
      printf("Error from _cp_(): Invalid Const Channel!\n");
      psg_abort(1);
   }

   int p1 = 0;
   int p2 = 0;
   int p3 = 0;
   int p4 = 0;
   int p5 = 0;

   if (cp.preset1 == 0) { p1 = 0; p2 = 0;};
   if (cp.preset1 == 1) { p1 = 1; p2 = 0;};
   if (cp.preset1 == 2) { p1 = 1; p2 = 1;};
   if (cp.preset2 == 0) { p3 = 0; p4 = 0;};
   if (cp.preset2 == 1) { p3 = 1; p4 = 0;};
   if (cp.preset2 == 2) { p3 = 1; p4 = 1;};
   if (cp.preset3 == 1) p5 = 1;

   switch(nchConst) {
      case 1:
         obsunblank();
         if (p1 == 0) {
            txphase(cphase);
            xmtron();
         }
         break;
      case 2:
         decunblank();
            if (p1 == 0) {
            decphase(cphase);
            decon();
         }
         break;
      case 3:
         dec2unblank();
         if (p1 == 0) {
            dec2phase(cphase);
            dec2on();
         }
         break;
      case 4:
         dec3unblank();
         if (p1 == 0) {
            dec2phase(cphase);
            dec2on();
         }
         break;
      default:
         printf("Error from _cp_(): Invalid Const Channel!\n");
         psg_abort(1);
         break;
   }

   switch(nchRamp) {
      case 1:
         obsunblank();
         if (p1 == 0) {
            txphase(rphase);
            xmtron();
         }
         break;
      case 2:
         decunblank();
         if (p1 == 0) {
            decphase(rphase);
            decon();
         }
         break;
      case 3:
         dec2unblank();
         if (p1 == 0) {
            dec2phase(rphase);
            dec2on();
         }
         break;
      case 4:
         dec3unblank();
         if (p1 == 0) {
            dec3phase(rphase);
            dec3on();
         }
         break;
      default:
         printf("Error from _cp_(): Invalid Ramp Channel!\n");
         psg_abort(1);
         break;
   }

   if ((PWRF_DELAY == 0.0) || (p2 == 0) || ((strcmp(cp.sh,"c") == 0) && (fabs(cp.of) < 1.0))) {
      switch(nchRamp) {
         case 1:
            obspwrf(aRamp);
            break;
         case 2:
            decpwrf(aRamp);
            break;
         case 3:
            dec2pwrf(aRamp);
            break;
         case 4:
            dec3pwrf(aRamp);
            break;
         default:
            printf("Error from _cp_(): Invalid Ramp Channel!\n");
            psg_abort(1);
            break;
      }
   }

   if ((p5 == 0) || (PWRF_DELAY == 0.0)) {
      switch(nchConst) {
         case 1:
            obspwrf(aConst);
            break;
         case 2:
            decpwrf(aConst);
            break;
         case 3:
            dec2pwrf(aConst);
            break;
         case 4:
            dec3pwrf(aConst);
            break;
         default:
            printf("Error from _cp_(): Invalid Const Channel!\n");
            psg_abort(1);
         break;
      }
   }

   switch(nchRamp) {
      case 1:
         if (cp.t > 0.0) {
            if ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0)) {
               obswfgon(cp.pattern,cp.n90*12.5e-9,90.0,p2,cp.offstdelay);
            }
	 }
         if (p1 != 0) {
            txphase(rphase);
            xmtron();
         }
         break;
      case 2:
         if (cp.t > 0.0) {
            if ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0)) {
               decwfgon(cp.pattern,cp.n90*12.5e-9,90.0,p2,cp.offstdelay);
            }
	 }
         if (p1 != 0) {
            decphase(rphase);
            decon();
         }
         break;
      case 3:
         if (cp.t > 0.0) {
            if ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0)) {
               dec2wfgon(cp.pattern,cp.n90*12.5e-9,90.0,p2,cp.offstdelay);
            }
	 }
         if (p1 != 0) {
            dec2phase(rphase);
            dec2on();
         }
         break;
      case 4:
         if (cp.t > 0.0) {
            if ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0)) {
               dec3wfgon(cp.pattern,cp.n90*12.5e-9,90.0,p2,cp.offstdelay);
            }
	 }
         if (p1 != 0) {
            dec3phase(rphase);
            dec3on();
         }
         break;
      default:
         printf("Error from _cp_(): Invalid Ramp Channel!\n");
         psg_abort(1);
         break;
   }

   if (p1 != 0) {
      switch(nchConst) {
         case 1:
            txphase(cphase);
            xmtron();
	    break;
         case 2:
            decphase(cphase);
            decon();
	    break;
         case 3:
            dec2phase(cphase);
            dec2on();
            break;
         case 4:
            dec3phase(cphase);
	    dec3on();
	    break;
         default:
            printf("Error from _cp_(): Invalid Const Channel!\n");
            psg_abort(1);
            break;
      }
   }

   delay(cp.t);

   if (p3 != 0) {
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
         default:
            printf("Error from _cp_(): Invalid Const Channel!\n");
            psg_abort(1);
            break;
      }
   }

   switch(nchRamp) {
      case 1:
         if (p3 != 0) xmtroff();
	 if (cp.t > 0.0) {
            if ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0)) {
               obswfgoff(p4);
            }
	 }
         if (p3 == 0) xmtroff();
         obsunblank();
         break;
      case 2:
         if (p3 != 0) decoff();
	 if (cp.t > 0.0) {
            if ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0)) {
               decwfgoff(p4);
            }
	 }
         if (p3 == 0)  decoff();
         decunblank();
         break;
      case 3:
         if (p3 != 0) dec2off();
	 if (cp.t > 0.0) {
            if ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0)) {             
              dec2wfgoff(p4);             
            }
	 }
         if (p3 == 0) dec2off();
         dec2unblank();
         break;
      case 4:
         if (p3 != 0) dec3off();
	 if (cp.t > 0.0) {
            if ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0)) {
               dec3wfgoff(p4);
	    }
         }
         if (p3 == 0) dec3off();
         dec3unblank();
         break;
      default:
         printf("Error from _cp_(): Invalid Ramp Channel!\n");
         psg_abort(1);
         break;
   }

   if (p3 == 0) {
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
         default:
            printf("Error from _cp_(): Invalid Const Channel!\n");
            psg_abort(1);
            break;
      }
   }
}

//=============================================================
//  _clearcp_(CP *cp) - clear the wavegen with "prgoff()"
//============================================================= 

void _clearcp_(CP cp)
{
   char  *chRamp;
   int  nchRamp = 0;
   if (strcmp(cp.ch,"fr") == 0) chRamp = cp.fr;
   else chRamp = cp.to;

   if (strcmp(chRamp,"obs") == 0)       nchRamp = 1;
   else if (strcmp(chRamp,"dec") == 0)  nchRamp = 2;
   else if (strcmp(chRamp,"dec2") == 0) nchRamp = 3;
   else if (strcmp(chRamp,"dec3") == 0) nchRamp = 4;
   else {
      printf("Error from _clearcp_(): Invalid Ramp Channel!\n");
      psg_abort(1);
   }

   dps_off();
   switch(nchRamp) {
      case 1:
      	 if ((cp.t > 0.0) && ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0))) {
            clearobswfg();
	 }
         obsunblank();
	 break;
      case 2:
      	 if ((cp.t > 0.0) && ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0))) {
            cleardecwfg();
	 }
         decunblank();
	 break;
      case 3:
      	 if ((cp.t > 0.0) && ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0))) {
            cleardec2wfg();
	 }
         dec2unblank();
         break;
      case 4:
      	 if ((cp.t > 0.0) && ((strcmp(cp.sh,"c") != 0) || (fabs(cp.of) >= 1.0))) {
	    cleardec3wfg();
	 }
         dec3unblank();
	 break;
      default:
         printf("Error from _clearcp_(): Invalid Ramp Channel!\n");
         psg_abort(1);
         break;
   }
   dps_on();
}

//===============================================================
// MPinitializer - Setup Memory for Phase, Amplitude, etc Lists
//===============================================================

void MPinitializer(MPSEQ *mp,int npw,int nph,int no,int na,int ng,int nBase,int nSuper)
{
   mp->n = (int*) malloc(nBase*sizeof(int));
   mp->pw = (double*) malloc(npw*sizeof(double));
   mp->phBase = (double*) malloc(nph*sizeof(double));
   mp->of = (double*) malloc(no*sizeof(double));
   mp->aBase = (double*) malloc(na*sizeof(double));
   mp->gateBase = (double*) malloc(ng*sizeof(double));
   mp->phSuper = (double*) malloc(nSuper*sizeof(double));

   mp->nphBase = nBase;   
   mp->nphSuper = nSuper;
   mp->npw = npw;
   mp->nph = nph;
   mp->no = no;
   mp->na = na;
   mp->ng = ng;

   if (PWRF_DELAY > 0.0) mp->n90 = 16; 
   else mp->n90 = 8;   
   mp->preset1 = 0;
   mp->preset2 = 0;
   mp->strtdelay = WFG_START_DELAY - WFG_OFFSET_DELAY;
   mp->offstdelay = WFG_OFFSET_DELAY;
   mp->apdelay = PWRF_DELAY;

   if (mp->na == 1) mp->aBase[0] = 1023.0;
   if (mp->ng == 1) mp->gateBase[0] = 0.0; 
}

//======================================
// MPchopper() - Generalized MPchopper
//======================================

#ifndef NVPSG
MPSEQ MPchopper(MPSEQ seq)
{
   FILE *fp;
   extern char userdir[];
   char shapepath[MAXSTR*3],str[MAXSTR];
   int npuls, iph, ntick,nrpuls,naccumpuls;
   int i = 0;
   double phase = 0.0;

// Set printmode for gate and amplitude defaults

   int printmode = 1;
   if ((seq.ng == 1) && (seq.gateBase[0] ==  0.0)) printmode = 2;
   if ((printmode == 2) && (seq.na == 1) && (seq.aBase[0] ==  1023.0)) printmode = 3;

// Obtain nmin ,the ticks per step, from seq.n90

   int nmin = seq.n90;  // Must be set in the get function

// Open the output file

   sprintf(str,"%s/shapelib/",userdir);
   sprintf(shapepath,"%s%s.DEC",str,seq.pattern);
   if ( (fp = fopen(shapepath,"w")) == NULL) {
      printf("Error in open of %s \n",shapepath);
      psg_abort(1);
   }

// Round each step to a multiple of the minimum step, nmin

   for (i = 0; i < seq.nphBase; i++) {
      seq.pw[i%seq.npw] = nmin*DTCK*(int)(seq.pw[i%seq.npw]/(nmin*DTCK) + 0.5);
      seq.n[i] = (int)(seq.pw[i%seq.npw]/DTCK + 0.5);
   }

// Save the time for the base cycle

   seq.telem = seq.pw[0];
   for(i=1;i<seq.nphBase;i++) {
      seq.telem += seq.pw[i%seq.npw];
   }

// Calculate total time and total pulses

   seq.t = seq.telem*seq.nelem;

   char lpattern[NPATTERN];
   sprintf(lpattern,"%s%d",getname0("",seq.seqName,""),seq.nRec);
   int lix = arryindex(seq.array);
   savet(seq.t, lix, lpattern);  //Save the total duration to the MODULE structure.
   npuls = seq.nphBase*seq.nelem;

// Get initial phase and super cycle step.

   double ph = seq.phAccum;
   iph = seq.iSuper;

// Loop over the npuls pulses

   ntick = 0;
   naccumpuls = 0;
   nrpuls = 0;
   double dph = 0.0;
   for (i = 0; i<npuls;i++) {     
      nrpuls = seq.n[i%seq.nphBase];
      naccumpuls = 0;
      ntick = 0;

// Determine the offset phase-step for each pulse.

      if (fabs(seq.of[i%seq.no]) > 1.0)
         dph = 360.0*seq.of[i%seq.no]*DTCK;
      else
         dph = 0.0; 

// Set the initial phase and supercycle step.

      if ( i%seq.nphBase == 0 && i != 0) {
         iph = (iph + 1)%seq.nphSuper;
      }
      if (fabs(seq.phInt) >= DPH) {
         ph =  ph + seq.phInt;
         seq.phInt = 0.0;
      }

// If dph = 0.0, write the pulse as a block.

      if (dph == 0.0 ) {
         phase = roundphase(ph+seq.phBase[i%seq.nph] + seq.phSuper[iph],360.0/8192);
         if (printmode == 1)
            fprintf(fp,"%10.1f %10.3f %6.1f %6.1f\n",(90.0/nmin)*seq.n[i%seq.nphBase],phase,seq.aBase[i%seq.na],seq.gateBase[i%seq.ng]);
         if (printmode == 2) 
            fprintf(fp,"%10.1f %10.3f %6.1f\n",(90.0/nmin)*seq.n[i%seq.nphBase],phase,seq.aBase[i%seq.na]);
         if (printmode == 3)
            fprintf(fp,"%10.1f %10.3f\n",(90.0/nmin)*seq.n[i%seq.nphBase],phase);
         continue;
      }

// If dph > 0.0, chop the pulse

      while(nrpuls > 0) {
         if (nrpuls == seq.n[i%seq.nphBase]) { // Always write the first step
            nrpuls -= (nmin-1);
            if (fabs(seq.phInt) >= DPH) {
               ph =  ph + seq.phInt;
               seq.phInt = 0;
            }
            phase = roundphase(ph+seq.phBase[i%seq.nph]+seq.phSuper[iph],360.0/8192);
            if (printmode == 1)
               fprintf(fp,"%10.1f %10.3f %6.1f %6.1f\n",90.0,phase,seq.aBase[i%seq.na],seq.gateBase[i%seq.ng]);
            if (printmode == 2)
               fprintf(fp,"%10.1f %10.3f %6.1f\n",90.0,phase,seq.aBase[i%seq.na]);
            if (printmode == 3)
               fprintf(fp,"%10.1f %10.3f\n",90.0,phase);
            seq.phInt += (nmin-1)*dph;
            naccumpuls = nmin - 1;
            ntick = nmin - 1; // Set nticks and running phase forward by nmin
            continue;
         }
         ntick++;
         nrpuls--;
         naccumpuls++; // Increment the running duration between written steps
         seq.phInt += dph; // Increment the running phase between written steps
         if ((ntick == nmin) && (nrpuls >= nmin)) {// Decide whether to write a step at multiples of nmin
            ntick = 0;
            if (nrpuls == nmin) { // Always write the last step
               if ( fabs(seq.phInt) >= DPH) { // Change ph if the delta-ph > DPH, else                       
                  ph =  ph + seq.phInt;       // leave a running remainder for the next pulse
                  seq.phInt = 0.0;
               }
               phase = roundphase(ph+seq.phBase[i%seq.nph]+seq.phSuper[iph],360.0/8192);
               if (printmode == 1)
                  fprintf(fp,"%10.1f %10.3f %6.1f %6.1f\n",(90.0/nmin)*naccumpuls,phase,seq.aBase[i%seq.na],seq.gateBase[i%seq.ng]);
               if (printmode == 2)
                  fprintf(fp,"%10.1f %10.3f %6.1f\n",(90.0/nmin)*naccumpuls,phase,seq.aBase[i%seq.na]);
               if (printmode == 3)
		  fprintf(fp,"%10.1f %10.3f\n",(90.0/nmin)*naccumpuls,phase);
               seq.phInt += nmin*dph;
               naccumpuls = 0;
               break;
            }
            if (fabs(seq.phInt) >= DPH) { //Write a step if delta-ph > DPH, else
               ph =  ph + seq.phInt;      //accumulate the running phase
               seq.phInt = 0;
               phase = roundphase(ph+seq.phBase[i%seq.nph]+seq.phSuper[iph],360.0/8192);
               if (printmode == 1)
                  fprintf(fp,"%10.1f %10.3f %6.1f %6.1f\n",(90.0/nmin)*naccumpuls,phase,seq.aBase[i%seq.na],seq.gateBase[i%seq.ng]);
               if (printmode == 2)
                  fprintf(fp,"%10.1f %10.3f %6.1f\n",(90.0/nmin)*naccumpuls,phase,seq.aBase[i%seq.na]);
               if (printmode == 3)
                  fprintf(fp,"%10.1f %10.3f\n",(90.0/nmin)*naccumpuls,phase);
               naccumpuls = 0;
            }
         }
      }   
   }

// Add an Amplitude Scale Step of 0.0 Duration.

   if (printmode == 1) fprintf(fp,"%10.1f %10.3f %6.1f %6.1f\n",0.0,0.0,1023.0,1.0);
   if (printmode == 2) fprintf(fp,"%10.1f %10.3f %6.1f\n",0.0,0.0,1023.0);
   seq.phAccum = ph;  // Save the current phase for the next waveform.   
   fclose(fp);
   return seq;
}
#endif

//=====================================================
// make_ramp() - Make a Tangent Ramp for CP1 and DREAM
//=====================================================

RAMP make_ramp(RAMP r)
{
   extern void dumpRAMP(RAMP r);
   FILE *fp;
   extern char userdir[];
   char shapepath[3*MAXSTR],str[MAXSTR+16];
   int ntick,nrpuls,naccumpuls;
   int nmin = r.n90;
   double norm,mean;
   norm = 0.0; mean = 0.0;
   double at;
   at = 0.0;
   double t;
   double ph,dph,aLast,aCurrent=0.0,phase;
   enum polarity {NORMAL, UP_UP, DOWN_DOWN, DOWN_UP, UP_DOWN};
   enum polarity POL = NORMAL;
   sprintf(str,"%s/shapelib/",userdir);
   sprintf(shapepath,"%s%s.DEC",str,r.pattern);
   if ((fp = fopen(shapepath,"w")) == NULL) {
      printf("Error in open of %s \n",shapepath);
      psg_abort(1);
   }
   if (strcmp(r.sh,"c") == 0) {
      r.b = 1.0e12;
      r.d = 0;
   }
   else if (strcmp(r.sh,"l") == 0) r.b = 1.0e12;
   if (strcmp(r.pol,"n") == 0 ) {
      POL = NORMAL;
      mean = r.a;
      norm = 1023.0/(mean + fabs(r.d));
   }
   if (strcmp(r.pol,"uu") == 0 ) {
      POL = UP_UP;
      mean = r.a;
      norm = 1023.0/(mean + fabs(r.d));
   }
   if (strcmp(r.pol,"dd") == 0 ) {
      POL = DOWN_DOWN;
      mean = r.a;
      norm = 1023.0/(mean + fabs(r.d));
   }
   if (strcmp(r.pol,"du") == 0 ) {
      POL = DOWN_UP;
      mean = r.a;
      norm = 1023.0/(mean + fabs(r.d));
   }
   if (strcmp(r.pol,"ud") == 0 ) {
      POL = UP_DOWN;
      mean = r.a;
      norm = 1023.0/(mean + fabs(r.d));
   }
   if (fabs(r.of) > 1.0) {
      dph = 360.0*r.of*DTCK;
   }
   else {
      dph = 0.0;
   }
   r.t = nmin*DTCK*(int)(r.t/(nmin*DTCK) + 0.5);
   r.n = (int)(r.t/DTCK + 0.5);

   char lpattern[NPATTERN];
   int lix = arryindex(r.array);
   sprintf(lpattern,"%s%d",getname0("",r.seqName,""),r.nRec);
   savet(r.t, lix, lpattern); //Save the total duration to a MODULE structure. 

   ph = r.phAccum;
   if (fabs(r.phInt) >= DPH){                         
      ph =  ph + r.phInt;
      r.phInt = 0.0;
   }
   t = 0;
   ntick = 0;
   naccumpuls = 0;
   if (strcmp(r.sh,"c") == 0 && fabs(r.of)  < 1.0) {
      fprintf(fp,"%10.1f %10.3f %6.1f\n",90.0*r.n/r.n90,0.0,1023.0);
   }
   else {
      nrpuls = r.n;
      switch (POL) {
         case NORMAL:
            at = atan(r.d/r.b);
            aCurrent = norm*(mean - r.d);
            break;
         case UP_UP:
            at = atan(fabs(r.d)/r.b); 
            aCurrent = norm*(mean + fabs(r.d));
            break;
         case UP_DOWN:
            at = atan(-1.0*fabs(r.d)/r.b);
            aCurrent = norm*(mean + fabs(r.d));
            break;
         case DOWN_DOWN:
            at = atan(fabs(r.d)/r.b);
            aCurrent = norm*(mean - fabs(r.d));
            break;
         case DOWN_UP:
            at = atan(1.0*fabs(r.d)/r.b);
            aCurrent = norm*(mean - fabs(r.d));
            break;
         default:
            printf("Error in make_ramp. Cannot determine RAMP polarity!\n");
            psg_abort(1);
      }
      aCurrent = (double)((int)(aCurrent + 0.5));
      if (aCurrent > 1023) aCurrent = 1023.0;
      if (aCurrent < 0) aCurrent = 0.0;
      aLast = aCurrent;
      while (nrpuls > 0) {
         if (nrpuls == r.n) {
            if (fabs(r.phInt) >= DPH) {
               ph =  ph + r.phInt;
               r.phInt = 0;
            }
            phase = roundphase(ph,360.0/8192);
            fprintf(fp,"%10.1f %10.3f %6.1f\n",90.0,phase,aCurrent);
            t += (r.n90 - 1)*DTCK;
            nrpuls -= (r.n90 - 1);
            r.phInt += (r.n90 - 1)*dph;
            naccumpuls = r.n90 - 1;
            ntick = r.n90 - 1;
            aLast = aCurrent;
            switch (POL) {
               case NORMAL:
               case UP_DOWN:
               case DOWN_UP:
                  aCurrent = norm*(mean - r.b*tan(at*(1.0 - 2.0*t/r.t)));
                  break;
               case UP_UP:
                  aCurrent = norm*(mean + fabs(r.b*tan(at*(1.0 - 2.0*t/r.t))));
                  break;
               case DOWN_DOWN:
                  aCurrent = norm*(mean - fabs(r.b*tan(at*(1.0 - 2.0*t/r.t))));
                  break;
            }
            aCurrent = (double)((int) (aCurrent + 0.5));
            if (aCurrent > 1023) aCurrent = 1023.0;
            if (aCurrent < 0) aCurrent = 0.0;
            continue;
         }
         ntick++;
         nrpuls--;
         naccumpuls++;
         t += DTCK;
         r.phInt += dph;
         switch (POL) {
            case NORMAL:
            case UP_DOWN:
            case DOWN_UP:
               aCurrent = norm*(mean - r.b*tan(at*(1.0 - 2.0*t/r.t)));
               break;
            case UP_UP:
               aCurrent = norm*(mean + fabs(r.b*tan(at*(1.0 - 2.0*t/r.t))));
               break;
            case DOWN_DOWN:
               aCurrent = norm*(mean - fabs(r.b*tan(at*(1.0 - 2.0*t/r.t))));
               break;
         }
         aCurrent = (double)( (int) (aCurrent + 0.5) );
         if (aCurrent > 1023) aCurrent = 1023.0;
         if (aCurrent < 0) aCurrent = 0.0;
         if (ntick == r.n90) {
            ntick = 0;
            if (nrpuls == r.n90) {
               if (fabs(r.phInt) >= DPH) {
                  ph =  ph + r.phInt;
                  r.phInt = 0.0;
               }
               if ( fabs(aCurrent - aLast) >= 1.0) {
                  aLast = aCurrent;
               }
               phase = roundphase(ph,360.0/8192);
               fprintf(fp,"%10.1f %10.3f %6.1f\n",(90.0/r.n90)*naccumpuls,phase,aCurrent);
               r.phInt += r.n90*dph;
               naccumpuls = 0;
            }
            if (fabs(r.phInt) >= DPH || fabs(aCurrent - aLast) >= 1.0) {
               ph = ph + r.phInt;
               r.phInt = 0.0;
               aLast = aCurrent;
               phase = roundphase(ph,360.0/8192);
               fprintf(fp,"%10.1f %10.3f %6.1f\n",(90.0/r.n90)*naccumpuls,phase,aCurrent);
               naccumpuls = 0;
            }
         }
      }
   }
   r.phAccum = ph;
   fclose(fp);
   return r;
} 
//=======================================================
//make_dream() - Calculate Four Tangent RAMPS for DREAM
//=======================================================

DREAM make_dream(DREAM d) {
    extern RAMP make_ramp(RAMP r);

//Make the four tangent shapes for DREAM

    d.Ruu = make_ramp(d.Ruu);
    d.Rud = make_ramp(d.Rud);
    d.Rdu = make_ramp(d.Rdu);
    d.Rdd = make_ramp(d.Rdd);

//Create a v-variable to count the shapes

    d.vcount = setvvarname();
    initval(0.0,d.vcount);

    return d;
}

//==============================================================
//  Update RAMP - Used to run make_ramp() following a change to
//  the RAMP module
//==============================================================

RAMP update_ramp(RAMP seq, double p, double phint, int iRec)
{
   char *var;
   extern RAMP make_ramp(RAMP seq);
   seq.phInt = phint;
   seq.phAccum = p;
   seq.nRec = iRec;
   seq.calc = 1;

   char lpattern[NPATTERN];
   var = getname0("",seq.seqName,"");
   sprintf(lpattern,"%s%d",var,seq.nRec);
   seq.hasArray = hasarry(seq.array, lpattern);
   int lix = arryindex(seq.array);
   var = getname0("",seq.seqName,"");
   sprintf(seq.pattern,"%s%d_%d",var,seq.nRec,lix);
   if (seq.hasArray == 1) {
      seq = make_ramp(seq);
   }
   seq.t = gett(lix, lpattern);
   return seq;
}

//================================================================
//  Update SHAPE - Used to run make_shape() following a change to
//  the SHAPE module
//================================================================

SHAPE update_shape(SHAPE seq, double p, double phint, int iRec)
{
   char *var;
   extern SHAPE make_shape(SHAPE seq);
   seq.pars.phInt = phint;
   seq.pars.phAccum = p;
   seq.pars.nRec = iRec;
   seq.pars.calc = 1;

   char lpattern[NPATTERN];
   var = getname0("",seq.pars.seqName,"");
   sprintf(lpattern,"%s%d",var,seq.pars.nRec);
   seq.pars.hasArray = hasarry(seq.pars.array, lpattern);
   int lix = arryindex(seq.pars.array);
   var = getname0("",seq.pars.seqName,"");
   sprintf(seq.pars.pattern,"%s%d_%d",var,seq.pars.nRec,lix);
   if (seq.pars.hasArray == 1) {
      seq = make_shape(seq);
   }  
   seq.pars.t = gett(lix, lpattern);  
   return seq;
}

//==========================================================
//  Update CP - Used to run make_cp() following a change to
//  the CP module
//==========================================================

CP update_cp(CP seq, double p, double phint, int iRec)
{
   char *var;
   extern CP make_cp(CP seq);
   seq.phInt = phint;
   seq.phAccum = p;
   seq.nRec = iRec;
   seq.calc = 1;

   char lpattern[NPATTERN];
   var = getname0("",seq.seqName,"");
   sprintf(lpattern,"%s%d",var,seq.nRec);
   seq.hasArray = hasarry(seq.array, lpattern);
   int lix = arryindex(seq.array);
   var = getname0("",seq.seqName,"");
   sprintf(seq.pattern,"%s%d_%d",var,seq.nRec,lix);
   if (seq.hasArray == 1) {
      seq = make_cp(seq);
   }  
   seq.t = gett(lix, lpattern);
   return seq;
}

//=============================================================
//  Update MPSEQ - Used to run MPchopper following a change to
//  the MPSEQ module
//=============================================================

MPSEQ update_mpseq(MPSEQ seq, int iph, double p, double phint, int iRec)
{
   char *var;
   extern MPSEQ MPchopper(MPSEQ seq);
   seq.phInt = phint;
   seq.phAccum = p;
   seq.iSuper = iph;
   seq.nRec = iRec;
   seq.calc = 1;

   char lpattern[NPATTERN];
   var = getname0("",seq.seqName,"");
   sprintf(lpattern,"%s%d",var,seq.nRec);
   seq.hasArray = hasarry(seq.array, lpattern);
   int lix = arryindex(seq.array);
   var = getname0("",seq.seqName,"");
   sprintf(seq.pattern,"%s%d_%d",var,seq.nRec,lix);
   if (seq.hasArray == 1) {
      seq = MPchopper(seq);
      seq.iSuper = iph + seq.nelem%seq.nphSuper;
   }
   seq.t = gett(lix, lpattern);   
   return seq;
}

//======================================================
// Function Definitions that Deal with Shaped Pulses
//======================================================

//======================
// Make a Generic Shape
//======================

#ifndef NVPSG
SHAPE make_shape(SHAPE s)
{
   FILE *fp;
   extern char userdir[];
   char shapepath[MAXSTR*3],str[MAXSTR];
   int ntick,nrpuls,naccumpuls;
   double t,ph,dph,phShapeCurrent,phShapeLast,aLast,aCurrent,phase,gate;
   STATE state;

// Open the output file

   sprintf(str,"%s/shapelib/",userdir);
   sprintf(shapepath,"%s%s.DEC",str,s.pars.pattern);
   if ((fp = fopen(shapepath,"w")) == NULL) {
      printf("Error in open of %s \n",shapepath);
      psg_abort(1);
   }

// Round the time to the nearest number of steps, determine the number of ticks

   s.pars.t = s.pars.n90*DTCK*(int)(s.pars.t/(s.pars.n90*DTCK) + 0.5);
   s.pars.n = (int)(s.pars.t/DTCK + 0.5);

   char lpattern[NPATTERN];
   int lix = arryindex(s.pars.array);
   sprintf(lpattern,"%s%d",getname0("",s.pars.seqName,""),s.pars.nRec);
   savet(s.pars.t, lix, lpattern); //  Save the total duration to the MODULE structure

// Initialize the phase, the time and the ticks

   ph = s.pars.phAccum;
   t = 0;
   ntick = 0;
   naccumpuls = 0;
   nrpuls = s.pars.n;

// Set the Phase increment for an offset

   if (fabs(s.pars.of) > 1.0) {
      dph = 360.0*s.pars.of*DTCK;
   }
   else {
      dph = 0.0;
   }

// Get the state at time zero

   state =  s.get_state(t,s.pars);
   aCurrent = (double)((int)( state.a + 0.5));
   if (aCurrent > 1023.0) aCurrent = 1023;
   if (aCurrent < 0.0) aCurrent = 0.0;
   aLast = aCurrent;
   phShapeCurrent = state.p;
   phShapeLast = phShapeCurrent;
   gate = state.g;

//Calculate the shape and count ticks down from the initial "nrpul"

   while (nrpuls > 0) {
      if (nrpuls == s.pars.n) {               // Always write the first step
         if (fabs(s.pars.phInt) >= DPH) {  
            ph =  ph + s.pars.phInt;          // Add in the initial phase
            s.pars.phInt = 0;
         }
         phase = roundphase(ph+phShapeCurrent,360.0/8192);
         fprintf(fp,"%10.1f %10.3f %6.1f %6.1f\n",90.0,phase,aCurrent,gate);

         t += (s.pars.n90 - 1)*DTCK; //Set nticks and running phase forward by s.pars.n90
         nrpuls -= (s.pars.n90-1);
         s.pars.phInt += (s.pars.n90-1)*dph;
         naccumpuls = s.pars.n90 - 1;
         ntick = s.pars.n90 - 1; 

         aLast = aCurrent; //Save the phase and amplitude and get the next state
         phShapeLast = phShapeCurrent;
         state =  s.get_state(t,s.pars);
         aCurrent = (double)((int)(state.a + 0.5));
         phShapeCurrent = state.p;
         gate = state.g;
         continue;
      }
      ntick++; //Increment the time, nticks and get the next state
      nrpuls--;
      naccumpuls++;
      t += DTCK;
      s.pars.phInt += dph;
      state =  s.get_state(t,s.pars);
      aCurrent = (double)((int)( state.a + 0.5));
      if (aCurrent > 1023.0) aCurrent = 1023;
      if (aCurrent < 0.0) aCurrent = 0.0;
      phShapeCurrent = state.p;
      gate = state.g;

      if ((ntick == s.pars.n90) && (nrpuls >= s.pars.n90)) { // Decide whether to write a step at multiples of s.pars.n90
         ntick = 0;
         if (nrpuls == s.pars.n90) {          //Always write the last step
            if (fabs(s.pars.phInt) >= DPH) {
               ph =  ph + s.pars.phInt;
               s.pars.phInt = 0.0;
            }
            if (fabs(aCurrent - aLast) >= 1.0) aLast = aCurrent;
            if (fabs(phShapeCurrent - phShapeLast) >=DPH) phShapeLast = phShapeCurrent;
            phase = roundphase(ph+phShapeCurrent,360.0/8192);
            fprintf(fp,"%10.1f %10.3f %6.1f %6.1f\n",(90.0/s.pars.n90)*naccumpuls,phase,aCurrent,gate);
            s.pars.phInt += s.pars.n90*dph;
            naccumpuls = 0;
            break;
         }
         if (fabs(s.pars.phInt) >= DPH ||
            fabs(aCurrent - aLast) >= 1.0 ||
            fabs(phShapeCurrent - phShapeLast ) >= DPH) { //Write a new step if different or delta-ph > DPH, else
                                                          //or delta-ph > DPH, else set the running phase.
            ph = ph + s.pars.phInt;
            s.pars.phInt = 0.0;
            aLast = aCurrent;
            phShapeLast = phShapeCurrent;
            phase = roundphase(ph+phShapeCurrent,360.0/8192);
            fprintf(fp,"%10.1f %10.3f %6.1f %6.1f\n",(90.0/s.pars.n90)*naccumpuls,phase,aCurrent,gate);//
            naccumpuls = 0;
         }
      }
   }   
   fprintf(fp,"%10.1f %10.3f %6.1f %6.1f\n",0.0,0.0,1023.0,0.0);
   s.pars.phAccum = ph;
   fclose(fp);
   return s;
}
#endif

//============================
// Initialize a Generic Shape
//============================

#ifndef NVPSG
SHAPE genericInitShape(SHAPE s, char *name, double p, double phint, int iRec)
{
   char *var;
   if ((strlen(name) > NSUFFIX) || (strlen(name)) < 2) {
      printf("Error in genericInitShape! The  name %s is invalid !\n",name);
      psg_abort(-1);
   }
   sprintf(s.pars.seqName,"%s",name);

// Obtain Phase Arguments

   s.pars.phAccum = p;
   s.pars.phInt = phint;
   s.pars.nRec = iRec;

// Supply INOVA Start Delays

   if (PWRF_DELAY > 0.0) s.pars.n90 = 16;
   else s.pars.n90 = 8;
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
   sprintf(lpattern,"%s%d",var,s.pars.nRec);
   s.pars.hasArray = hasarry(s.pars.array, lpattern);
   int lix = arryindex(s.pars.array);
   var = getname0("",s.pars.seqName,"");
   if (s.pars.calc > 0) {
      sprintf(s.pars.pattern,"%s%d_%d",var,s.pars.nRec,lix);
      if (s.pars.hasArray == 1) {
         s = make_shape(s);
      }
       s.pars.t = gett(lix, lpattern);
   }
   return s;
}
#endif

//=======================================
// Initialize the HS amplitude for MPSEQ
//=======================================

void _initmpseq(MPSEQ seq, double lamplitude)
{
   int chnl;
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_initmpseq() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }

   dps_off();
   switch (chnl) {
     case 1:
        obsunblank();
        if (seq.nelem > 0) {
           initobswfg("PRESET",5.0e-6,90.0,lamplitude);
	}
        break;
     case 2:
        decunblank();
        if (seq.nelem > 0) {
           initdecwfg("PRESET",5.0e-6,90.0,lamplitude);
	}
        break;
     case 3:
        dec2unblank();
        if (seq.nelem > 0) {
           initdec2wfg("PRESET",5.0e-6,90.0,lamplitude);
        }
        break;
     case 4:
        dec3unblank();
        if (seq.nelem > 0) {
           initdec3wfg("PRESET",5.0e-6,90.0,lamplitude);
        }
        break;
     default:
        printf("_initmpseq() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
        break;
   }
   dps_on();
}

//========================
//  Set an MPSEQ Waveform
//========================

void _setmpseq(MPSEQ seq)
{
   int chnl;
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_setmpseq() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }

   dps_off();
   switch (chnl) {
     case 1:
        obsunblank();
        if (seq.nelem > 0) {
           setobswfg(seq.pattern,seq.n90*12.5e-9,90.0,seq.a);
        }
        else {
           obspwrf(seq.a);
        }
        break;
     case 2:
        decunblank();
        if (seq.nelem > 0) {
           setdecwfg(seq.pattern,seq.n90*12.5e-9,90.0,seq.a);
        }        
        else {
           decpwrf(seq.a);
        }
        break;
     case 3:
        dec2unblank();
        if (seq.nelem > 0) {
           setdec2wfg(seq.pattern,seq.n90*12.5e-9,90.0,seq.a);
        }
        else {
           dec2pwrf(seq.a);
        }
        break;
     case 4:
        dec3unblank();
        if (seq.nelem > 0) {
           setdec3wfg(seq.pattern,seq.n90*12.5e-9,90.0,seq.a);
        }
        else {
           dec3pwrf(seq.a);
        }
        break;
     default:
        printf("_setmpseq() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
        break;
   }
   dps_on();
}

//========================================================
// Underscore Function for a MPSEQ - seq.t - phase only
//========================================================

void _mpseq(MPSEQ seq, int phase)
{
   int chnl;
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_mpseq() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }

   int m1 = 0;
   if ((seq.ng > 1) && (seq.gateBase[0] == 0.0)) m1 = 1;

   int p1 = 0;
   int p2 = 0;
   int p3 = 0;
   int p4 = 0;

   if (seq.preset1 == 0) { p1 = 0; p2 = 0;};
   if (seq.preset1 == 1) { p1 = 1; p2 = 0;};
   if (seq.preset1 == 2) { p1 = 1; p2 = 1;};
   if (seq.preset2 == 0) { p3 = 0; p4 = 0;};
   if (seq.preset2 == 1) { p3 = 1; p4 = 0;};
   if (seq.preset2 == 2) { p3 = 1; p4 = 1;};

   switch (chnl) {
      case 1:
         obsunblank();
         if ((p1 == 0) && (m1 == 0)) {
            txphase(phase);
            xmtron();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) obspwrf(seq.a);
         if (seq.nelem > 0) {
            obswfgon(seq.pattern,seq.n90*12.5e-9,90.0,p2,seq.offstdelay);
         }
         if ((p1 != 0) && (m1 == 0)) {
            txphase(phase);
            xmtron();
         }
         delay(seq.t);
         if (p3 != 0) xmtroff();
         if (seq.nelem > 0) {
	    obswfgoff(p4);
         }
	 if (p3 == 0) xmtroff();
	 obsunblank();
	 break;
      case 2:
         decunblank();
         if ((p1 == 0) && (m1 == 0)) {
            decphase(phase);
            decon();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) decpwrf(seq.a);
         if (seq.nelem > 0) {
            decwfgon(seq.pattern,seq.n90*12.5e-9,90.0,p2,seq.offstdelay);
         }
          if ((p1 != 0) && (m1 == 0)) {
            decphase(phase);
            decon();
         }
         delay(seq.t);
         if (p3 != 0) decoff();
         if (seq.nelem > 0) {
	    decwfgoff(p4);
         }
	 if (p3 != 0) decoff();
	 decunblank();
         break;
      case 3:
         dec2unblank();
         if ((p1 == 0) && (m1 == 0)) {
            dec2phase(phase);
            dec2on();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) dec2pwrf(seq.a);
         if (seq.nelem > 0) {
            dec2wfgon(seq.pattern,seq.n90*12.5e-9,90.0,p2,seq.offstdelay);
         }
         if ((p1 != 0) && (m1 == 0)) {
            dec2phase(phase);
            dec2on();
         }
         delay(seq.t);
         if (p3 != 0) dec2off();
         if (seq.nelem > 0) {
	    dec2wfgoff(p4);
         }
	 if (p3 == 0) dec2off();
	 dec2unblank();
	 break;
      case 4:
         dec3unblank();
         if (p1 == 0) {
            dec3phase(phase);
            dec3on();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0))  dec3pwrf(seq.a);
         if (seq.nelem > 0) {
            dec3wfgon(seq.pattern,seq.n90*12.5e-9,90.0,p2,seq.offstdelay);
         }
         if (p1 != 0) {
            dec3phase(phase);
            dec3on();
         }
         delay(seq.t);
         if (p3 != 0) dec3off();
         if (seq.nelem > 0) {
	    dec3wfgoff(p4);
         }
	 if (p3 == 0) dec3off();
	 dec3unblank();
         break;
      default:
         printf("_mpseq() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }
}

//==========================
//  Clear an MPSEQ Waveform
//==========================

void _clearmpseq(MPSEQ seq)
{
   int chnl;
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_mpseq() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }

    dps_off();
    switch (chnl) {
      case 1:
         if (seq.nelem > 0) {
            clearobswfg();
         }
         obsunblank();
	 break;
      case 2:
         if (seq.nelem > 0) {
            cleardecwfg();
         }
         decunblank();
         break;
      case 3:
         if (seq.nelem > 0) {
            cleardec2wfg();
         }
         dec2unblank();
	 break;
      case 4:
         if (seq.nelem > 0) {
            cleardec3wfg();
         }
         dec3unblank();
         break;
      default:
         printf("_clearmpseq() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }
   dps_on();
}

void _mpseqT(MPSEQ seq, double t, int phase)
{
   int chnl;
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_mpseqT() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }

   int m1 = 0;
   if ((seq.ng > 1) && (seq.gateBase[0] == 0.0)) m1 = 1;

   int p1 = 0;
   int p2 = 0;
   int p3 = 0;
   int p4 = 0;

   if (seq.preset1 == 0) { p1 = 0; p2 = 0;};
   if (seq.preset1 == 1) { p1 = 1; p2 = 0;};
   if (seq.preset1 == 2) { p1 = 1; p2 = 1;};
   if (seq.preset2 == 0) { p3 = 0; p4 = 0;};
   if (seq.preset2 == 1) { p3 = 1; p4 = 0;};
   if (seq.preset2 == 2) { p3 = 1; p4 = 1;};

   switch (chnl) {
      case 1:
         obsunblank();
         if ((p1 == 0) && (m1 == 0)) {
            txphase(phase);
            xmtron();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) obspwrf(seq.a);
         if (seq.nelem > 0) {
            obswfgon(seq.pattern,seq.n90*12.5e-9,90.0,p2,seq.offstdelay);
         }
         if ((p1 != 0) && (m1 == 0)) {
            txphase(phase);
            xmtron();
         }
         delay(t);
         if (p3 != 0) xmtroff();
         if (seq.nelem > 0) {
	    obswfgoff(p4);
         }
	 if (p3 == 0) xmtroff();
	 obsunblank();
	 break;
      case 2:
         decunblank();
         if ((p1 == 0) && (m1 == 0)) {
            decphase(phase);
            decon();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) decpwrf(seq.a);
         if (seq.nelem > 0) {
            decwfgon(seq.pattern,seq.n90*12.5e-9,90.0,p2,seq.offstdelay);
         }
         if ((p1 != 0) && (m1 == 0)) {
            decphase(phase);
            decon();
         }
         delay(t);
         if (p3 != 0) decoff();
         if (seq.nelem > 0) {
	    decwfgoff(p4);
	 }
	 if (p3 != 0) decoff();
	 decunblank();
         break;
      case 3:
         dec2unblank();
         if ((p1 == 0) && (m1 == 0)) {
            dec2phase(phase);
            dec2on();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) dec2pwrf(seq.a);
         if (seq.nelem > 0) {
            dec2wfgon(seq.pattern,seq.n90*12.5e-9,90.0,p2,seq.offstdelay);
         }
         if ((p1 != 0) && (m1 == 0)) {
            dec2phase(phase);
            dec2on();
         }
         delay(t);
         if (p3 != 0) dec2off();
         if (seq.nelem > 0) {
	    dec2wfgoff(p4);
         }
	 if (p3 == 0) dec2off();
	 dec2unblank();
	 break;
      case 4:
         dec3unblank();
         if ((p1 == 0) && (m1 == 0)) {
            dec3phase(phase);
            dec3on();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) dec3pwrf(seq.a);
         if (seq.nelem > 0) {
            dec3wfgon(seq.pattern,seq.n90*12.5e-9,90.0,p2,seq.offstdelay);
	 }
         if ((p1 != 0) && (m1 == 0)) {
            dec3phase(phase);
            dec3on();
         }
         delay(t);
         if (p3 != 0) dec3off();
         if (seq.nelem > 0) {
	    dec3wfgoff(p4);
	 }
	 if (p3 == 0) dec3off();
	 dec3unblank();
         break;
      default:
         printf("_mpseqT() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }
}

//============================================================
// Underscore Function for On and Off for MPSEQ - phase only
//============================================================

void _mpseqon(MPSEQ seq, int phase)
{
   int chnl;
   chnl = 0;
   if (!strcmp(seq.ch,"obs")) chnl = 1;
   else if (!strcmp(seq.ch,"dec")) chnl = 2;
   else if (!strcmp(seq.ch,"dec2")) chnl = 3;
   else if (!strcmp(seq.ch,"dec3")) chnl = 4;
   else {
      printf("_mpseqon() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }

   int m1 = 0;
   if ((seq.ng > 1) && (seq.gateBase[0] == 0.0)) m1 = 1;

   int p1 = 0;
   int p2 = 0;

   if (seq.preset1 == 0) { p1 = 0; p2 = 0;};
   if (seq.preset1 == 1) { p1 = 1; p2 = 0;};
   if (seq.preset1 == 2) { p1 = 1; p2 = 1;};

   switch (chnl) {
      case 1:
         obsunblank();
         if ((p1 == 0) && (m1 == 0)) {
            txphase(phase);
            xmtron();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) obspwrf(seq.a);
         if (seq.nelem > 0) {
            obswfgon(seq.pattern,seq.n90*12.5e-9,90.0,p2,seq.offstdelay);
	 }
         if ((p1 != 0) && (m1 == 0)) {
            txphase(phase);
            xmtron();
         }
	 break;
      case 2:
         decunblank();
         if ((p1 == 0) && (m1 == 0)) {
            decphase(phase);
            decon();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) decpwrf(seq.a);
	 if (seq.nelem > 0) {
            decwfgon(seq.pattern,seq.n90*12.5e-9,90.0,p2,seq.offstdelay);
	 }
         if ((p1 != 0) && (m1 == 0)) {
            decphase(phase);
            decon();
         }
         break;
      case 3:
         dec2unblank();
         if ((p1 == 0) && (m1 == 0)) {
            dec2phase(phase);
            dec2on();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) dec2pwrf(seq.a);
	 if (seq.nelem > 0) {
            dec2wfgon(seq.pattern,seq.n90*12.5e-9,90.0,p2,seq.offstdelay);
	 }
         if ((p1 != 0) && (m1 == 0)) {
            dec2phase(phase);
            dec2on();
         }
	 break;
      case 4:
         dec3unblank();
         if ((p1 == 0) && (m1 == 0)) {
            dec3phase(phase);
            dec3on();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) dec3pwrf(seq.a);
	 if (seq.nelem > 0) {
            dec3wfgon(seq.pattern,seq.n90*12.5e-9,90.0,p2,seq.offstdelay);
	 }
         if ((p1 != 0) && (m1 == 0)) {
            dec3phase(phase);
            dec3on();
         }
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
      psg_abort(1);
   }

   int p3 = 0;
   int p4 = 0;

   if (seq.preset2 == 0) { p3 = 0; p4 = 0;};
   if (seq.preset2 == 1) { p3 = 1; p4 = 0;};
   if (seq.preset2 == 2) { p3 = 1; p4 = 1;};

   switch (chnl) {
      case 1:
         if (p3 != 0) xmtroff();
         if (seq.nelem > 0) {
            obswfgoff(p4);
	 }
         if (p3 == 0) xmtroff();
         obsunblank();
	 break;
      case 2:
         if (p3 != 0) decoff();
	 if (seq.nelem > 0) {
            decwfgoff(p4);
	 }
         if (p3 == 0) decoff();
	 decunblank();
         break;
      case 3:
         if (p3 != 0) dec2off();
	 if (seq.nelem > 0) {
            dec2wfgoff(p4);
	 }
         if (p3 == 0) dec2off();
	 dec2unblank();
	 break;
      case 4:
         if (p3 != 0) dec3off();
	 if (seq.nelem > 0) {
            dec3wfgoff(p4);
	 }
         if (p3 == 0) dec3off();
	 dec3unblank();
         break;
      default:
         printf("_mpseqoff() Error: Undefined Channel! < 0!\n");
   }
}

//=======================================
// Initialize the HS amplitude for SHAPE
//=======================================

void _initshape(SHAPE s, double lamplitude)
{
   int chnl;
   chnl = 0;
   if (!strcmp(s.pars.ch,"obs")) chnl = 1;
   else if (!strcmp(s.pars.ch,"dec")) chnl = 2;
   else if (!strcmp(s.pars.ch,"dec2")) chnl = 3;
   else if (!strcmp(s.pars.ch,"dec3")) chnl = 4;
   else {
      printf("_initshape() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }
   dps_off();
   switch (chnl) {
     case 1:
        obsunblank();
	if (s.pars.t > 0.0) {
           initobswfg("PRESET",5.0e-6,90.0,lamplitude);
	}
        break;
     case 2:
        decunblank();
	if (s.pars.t > 0.0) {
           initdecwfg("PRESET",5.0e-6,90.0,lamplitude);
	}
        break;
     case 3:
        dec2unblank();
	if (s.pars.t > 0.0) {
           initdec2wfg("PRESET",5.0e-6,90.0,lamplitude);
	}
        break;
     case 4:
        dec3unblank();
	if (s.pars.t > 0.0) {
           initdec3wfg("PRESET",5.0e-6,90.0,lamplitude);
	}
        break;
     default:
        printf("_initshape() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
        break;
   }
   dps_on();
}

//========================
//  Set an SHAPE Waveform
//========================

void _setshape(SHAPE s)
{
   int chnl;
   chnl = 0;
   if (!strcmp(s.pars.ch,"obs")) chnl = 1;
   else if (!strcmp(s.pars.ch,"dec")) chnl = 2;
   else if (!strcmp(s.pars.ch,"dec2")) chnl = 3;
   else if (!strcmp(s.pars.ch,"dec3")) chnl = 4;
   else {
      printf("_setshape() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }

   dps_off();
   switch (chnl) {
     case 1:
        obsunblank();
	if (s.pars.t > 0.0) {
           setobswfg(s.pars.pattern,s.pars.n90*12.5e-9,90.0,s.pars.a);
	}
        else { 
           obspwrf(s.pars.a);
        }
        break;
     case 2:
        decunblank();
	if (s.pars.t > 0.0) {
           setdecwfg(s.pars.pattern,s.pars.n90*12.5e-9,90.0,s.pars.a);
	}        
        else { 
           decpwrf(s.pars.a);
        }
        break;
     case 3:
        dec2unblank();
	if (s.pars.t > 0.0) {
           setdec2wfg(s.pars.pattern,s.pars.n90*12.5e-9,90.0,s.pars.a);
	}        
        else { 
           dec2pwrf(s.pars.a);
        }
        break;
     case 4:
        dec3unblank();
	if (s.pars.t > 0.0) {
           setdec3wfg(s.pars.pattern,s.pars.n90*12.5e-9,90.0,s.pars.a);
	}
        else { 
           dec3pwrf(s.pars.a);
        }
        break;
     default:
        printf("_setshape() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
        break;
   }
   dps_on();
}

//==============================================
// Underscore Function to Output a Shaped Pulse
//==============================================

void _shape(SHAPE s, codeint phase)
{
  int chnl;
  chnl = 0;
  if (!strcmp(s.pars.ch,"obs")) chnl = 1;
  else if (!strcmp(s.pars.ch,"dec")) chnl = 2;
  else if (!strcmp(s.pars.ch,"dec2")) chnl = 3;
  else if (!strcmp(s.pars.ch,"dec3")) chnl = 4;
  else {
        printf("_shape() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
  }

  double ampl = s.pars.a;

   int p1 = 0;
   int p2 = 0;
   int p3 = 0;
   int p4 = 0;
   
   if (s.pars.preset1 == 0) { p1 = 0; p2 = 0;};
   if (s.pars.preset1 == 1) { p1 = 1; p2 = 0;};
   if (s.pars.preset1 == 2) { p1 = 1; p2 = 1;};
   if (s.pars.preset2 == 0) { p3 = 0; p4 = 0;};
   if (s.pars.preset2 == 1) { p3 = 1; p4 = 0;};
   if (s.pars.preset2 == 2) { p3 = 1; p4 = 1;};
   switch (chnl) {
      case 1:
         obsunblank();
         if (p1 == 0) {
            txphase(phase);
            xmtron();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) obspwrf(ampl);
	 if (s.pars.t > 0.0) {
            obswfgon(s.pars.pattern,s.pars.n90*12.5e-9,90.0,p2,s.pars.offstdelay);
	 }
         if (p1 != 0) {
            txphase(phase);
            xmtron();
         }
         delay(s.pars.t);
         if (p3 != 0) xmtroff();
	 if (s.pars.t > 0.0) {
            obswfgoff(p4);
	 }
         if (p3 == 0) xmtroff();
         obsunblank();
         break;
      case 2:
         decunblank();
         if (p1 == 0) {
            decphase(phase);
            decon();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) decpwrf(ampl);
	 if (s.pars.t > 0.0) {
            decwfgon(s.pars.pattern,s.pars.n90*12.5e-9,90.0,p2,s.pars.offstdelay);
	 }
         if (p1 != 0) {
            decphase(phase);
            decon();
         }
         delay(s.pars.t);
         if (p3 != 0) decoff();
	 if (s.pars.t > 0.0) {
            decwfgoff(p4);
	 }
         if (p3 == 0) decoff();
         decunblank();
         break;
      case 3:
         dec2unblank();
         if (p1 == 0) {
            dec2phase(phase);
            dec2on();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) dec2pwrf(ampl);
	 if (s.pars.t > 0.0) {
            dec2wfgon(s.pars.pattern,s.pars.n90*12.5e-9,90.0,p2,s.pars.offstdelay);
	 }
         if (p1 != 0) {
            dec2phase(phase);
            dec2on();
         }
         delay(s.pars.t);
         if (p3 != 0) dec2off();
	 if (s.pars.t > 0.0) {
            dec2wfgoff(p4);
	 }
         if (p3 == 0) dec2off();
         dec2unblank();
	 break;
      case 4:
         dec3unblank();
         if (p1 == 0) {
            dec3phase(phase);
            dec3on();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) dec3pwrf(ampl);
	 if (s.pars.t > 0.0) {
            dec3wfgon(s.pars.pattern,s.pars.n90*12.5e-9,90.0,p2,s.pars.offstdelay);
	 }
         if (p1 != 0) {
            dec3phase(phase);
            dec3on();
         }
         delay(s.pars.t);
         if (p3 != 0) dec3off();
	 if (s.pars.t > 0.0) {
            dec3wfgoff(p4);
	 }
         if (p3 == 0) dec3off();
         dec3unblank();
         break;
      default:
         printf("_shape() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }
}

//==========================
//  Clear a SHAPE Waveform
//==========================

void _clearshape(SHAPE s)
{
   int chnl;
   chnl = 0;
   if (!strcmp(s.pars.ch,"obs")) chnl = 1;
   else if (!strcmp(s.pars.ch,"dec")) chnl = 2;
   else if (!strcmp(s.pars.ch,"dec2")) chnl = 3;
   else if (!strcmp(s.pars.ch,"dec3")) chnl = 4;
   else {
      printf("_clearshape() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }
  
    dps_off();
    switch (chnl) {
      case 1:
      	 if (s.pars.t > 0.0) {
            clearobswfg();
	 }
         obsunblank();
	 break;
      case 2:
       	 if (s.pars.t > 0.0) {
            cleardecwfg();
	 }
         decunblank();
         break;
      case 3:
       	 if (s.pars.t > 0.0) {
            cleardec2wfg();
	 }
         dec2unblank();
	 break;
      case 4:
      	 if (s.pars.t > 0.0) {
            cleardec3wfg();
	 }
         dec3unblank();
         break;
      default:
         printf("_clearshape() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }
   dps_on();
}

//==============================================
//Underscore Functions to Turn Shapes On and Off
//==============================================

void _shapeon(SHAPE s, codeint phase)
{
   int chnl;
   chnl = 0;
   if (!strcmp(s.pars.ch,"obs")) chnl = 1;
   else if (!strcmp(s.pars.ch,"dec")) chnl = 2;
   else if (!strcmp(s.pars.ch,"dec2")) chnl = 3;
   else if (!strcmp(s.pars.ch,"dec3")) chnl = 4;
   else {
      printf("_shapeon() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }

   double ampl = s.pars.a;

   int p1 = 0;
   int p2 = 0;

   if (s.pars.preset1 == 0) { p1 = 0; p2 = 0;};
   if (s.pars.preset1 == 1) { p1 = 1; p2 = 0;};
   if (s.pars.preset1 == 2) { p1 = 1; p2 = 1;};

   switch (chnl) {
      case 1:
         obsunblank();
         if (p1 == 0) {
            txphase(phase);
            xmtron();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) obspwrf(ampl);
	 if (s.pars.t > 0.0) {
            obswfgon(s.pars.pattern,s.pars.n90*12.5e-9,90.0,p2,s.pars.offstdelay);
	 }
         if (p1 != 0) {
            txphase(phase);
            xmtron();
         }
         break;
      case 2:
         decunblank();
         if (p1 == 0) {
            decphase(phase);
            decon();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) decpwrf(ampl);
	 if (s.pars.t > 0.0) {
            decwfgon(s.pars.pattern,s.pars.n90*12.5e-9,90.0,p2,s.pars.offstdelay);
	 }
         if (p1 != 0) {
            decphase(phase);
            decon();
         }
         break;
      case 3:
         dec2unblank();
         if (p1 == 0) {
            dec2phase(phase);
            decon();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) dec2pwrf(ampl);
	 if (s.pars.t > 0.0) {
            dec2wfgon(s.pars.pattern,s.pars.n90*12.5e-9,90.0,p2,s.pars.offstdelay);
	 }
         if (p1 != 0) {
            dec2phase(phase);
            dec2on();
         }
         break;
      case 4:
         dec3unblank();
         if (p1 == 0) {
            dec3phase(phase);
            dec3on();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0)) dec3pwrf(ampl);
	 if (s.pars.t > 0.0) {
            dec3wfgon(s.pars.pattern,s.pars.n90*12.5e-9,90.0,p2,s.pars.offstdelay);
	 }
         if (p1 != 0) {
            dec3phase(phase);
            dec3on();
         }
         break;
      default:
         printf("_shapeon() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }
}

void _shapeoff(SHAPE s)
{
   int chnl;
   chnl = 0;
   if (!strcmp(s.pars.ch,"obs")) chnl = 1;
   else if (!strcmp(s.pars.ch,"dec")) chnl = 2;
   else if (!strcmp(s.pars.ch,"dec2")) chnl = 3;
   else if (!strcmp(s.pars.ch,"dec3")) chnl = 4;
   else {
      printf("_shapeoff() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }

   int p3 = 0;
   int p4 = 0;

   if (s.pars.preset2 == 0) { p3 = 0; p4 = 0;};
   if (s.pars.preset2 == 1) { p3 = 1; p4 = 0;};
   if (s.pars.preset2 == 2) { p3 = 1; p4 = 1;};

   switch (chnl) {
      case 1:
         if (p3 != 0) xmtroff();
	 if (s.pars.t > 0.0) {
            obswfgoff(p4);
	 }
         if (p3 == 0) xmtroff();
         obsunblank();
         break;
      case 2:
         if (p3 != 0) decoff();
	 if (s.pars.t > 0.0) {
            decwfgoff(p4);
	 }
         if (p3 == 0) decoff();
         decunblank();
         break;
      case 3:
         if (p3 != 0) dec2off();
	 if (s.pars.t > 0.0) {
            dec2wfgoff(p4);
	 }
         if (p3 == 0) dec2off();
         dec2unblank();
         break;
      case 4:
         if (p3 != 0) dec3off();
	 if (s.pars.t > 0.0) {
            dec3wfgoff(p4);
	 }
         if (p3 == 0) dec3off();
         dec3unblank();
         break;
      default:
         printf("_shapeoff() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }
}

//=======================================
// Initialize the HS amplitude for RAMP
//=======================================

void _initramp(RAMP r, double lamplitude)
{
   int chnl;
   chnl = 0;
   if (!strcmp(r.ch,"obs")) chnl = 1;
   else if (!strcmp(r.ch,"dec")) chnl = 2;
   else if (!strcmp(r.ch,"dec2")) chnl = 3;
   else if (!strcmp(r.ch,"dec3")) chnl = 4;
   else {
      printf("_initramp() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }
   dps_off();
   switch (chnl) {
      case 1:
         obsunblank();
	 if (r.t > 0.0) {
            if ( strcmp(r.sh,"c") != 0 || (fabs(r.of) >= 1.0)) {
              initobswfg("PRESET",5.0e-6,90.0,lamplitude);
            }
	 }
         break;
      case 2:
         decunblank();
	 if (r.t > 0.0) {
            if ( strcmp(r.sh,"c") != 0 || (fabs(r.of) >= 1.0)) {
               initdecwfg("PRESET",5.0e-6,90.0,lamplitude);
            }
	 }
         break;
      case 3:
         dec2unblank();
	 if (r.t > 0.0) {
            if ( strcmp(r.sh,"c") != 0 || (fabs(r.of) >= 1.0)) {
               initdec2wfg("PRESET",5.0e-6,90.0,lamplitude);
            }
	 }
         break;
      case 4:
         dec3unblank();
	 if (r.t > 0.0) {
            if ( strcmp(r.sh,"c") != 0 || (fabs(r.of) >= 1.0)) {
               initdec3wfg("PRESET",5.0e-6,90.0,lamplitude);
            }
	 }
         break;
      default:
         printf("_initramp() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }
   dps_on();
}

//========================
//  Set a RAMP Waveform
//========================

void _setramp(RAMP r)
{
   int chnl;
   chnl = 0;
   if (!strcmp(r.ch,"obs")) chnl = 1;
   else if (!strcmp(r.ch,"dec")) chnl = 2;
   else if (!strcmp(r.ch,"dec2")) chnl = 3;
   else if (!strcmp(r.ch,"dec3")) chnl = 4;
   else {
      printf("_setramp() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }

   double ampl = r.a + fabs(r.d);

   dps_off();
   switch (chnl) {
      case 1:
         obsunblank();
         if ((r.t > 0.0) && ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0))) {
            setobswfg(r.pattern,r.n90*12.5e-9,90.0,ampl);
         }
         else {
            obspwrf(ampl);
         }
         break;
      case 2:
         decunblank();
         if ((r.t > 0.0) && ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0))) {
            setdecwfg(r.pattern,r.n90*12.5e-9,90.0,ampl);
         }
         else {
            decpwrf(ampl);
         }
         break;
      case 3:
         dec2unblank();
         if ((r.t > 0.0) && ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0))) {
            setdec2wfg(r.pattern,r.n90*12.5e-9,90.0,ampl);
         }
         else {
            dec2pwrf(ampl);
         }
         break;
      case 4:
         dec3unblank();
         if ((r.t > 0.0) && ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0))) {
            setdec3wfg(r.pattern,r.n90*12.5e-9,90.0,ampl);
         }
         else {
            dec3pwrf(ampl);
         }
         break;
      default:
         printf("_setramp() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }
   dps_on();
}

//=============================================
//Underscore Function to Output a Tangent RAMP
//=============================================

void _ramp(RAMP r, codeint phase)
{
   int chnl;
   chnl = 0;
   if (!strcmp(r.ch,"obs")) chnl = 1;
   else if (!strcmp(r.ch,"dec")) chnl = 2;
   else if (!strcmp(r.ch,"dec2")) chnl = 3;
   else if (!strcmp(r.ch,"dec3")) chnl = 4;
   else {
      printf("_ramp() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
  }

  double ampl = r.a + fabs(r.d);

   int p1 = 0;
   int p2 = 0;
   int p3 = 0;
   int p4 = 0;

   if (r.preset1 == 0) { p1 = 0; p2 = 0;};
   if (r.preset1 == 1) { p1 = 1; p2 = 0;};
   if (r.preset1 == 2) { p1 = 1; p2 = 1;};
   if (r.preset2 == 0) { p3 = 0; p4 = 0;};
   if (r.preset2 == 1) { p3 = 1; p4 = 0;};
   if (r.preset2 == 2) { p3 = 1; p4 = 1;};

   switch (chnl) {
      case 1:
         obsunblank();
         if (p1 == 0) {
            txphase(phase);
            xmtron();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0) || ((strcmp(r.sh,"c") == 0) && (fabs(r.of) < 1.0))) obspwrf(ampl);
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               obswfgon(r.pattern,r.n90*12.5e-9,90.0,p2,r.offstdelay);
            }
	 }
         if (p1 != 0) {
            txphase(phase);
            xmtron();
         }
         delay(r.t);
         if (p3 != 0) xmtroff();
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               obswfgoff(p4);
            }
	 }
         if (p3 == 0) xmtroff();
         obsunblank();
         break;
      case 2:
         decunblank();
         if (p1 == 0) {
            decphase(phase);
            decon();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0) || ((strcmp(r.sh,"c") == 0) && (fabs(r.of) < 1.0))) decpwrf(ampl);
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               decwfgon(r.pattern,r.n90*12.5e-9,90.0,p2,r.offstdelay);
            }
	 }
         if (p1 != 0) {
            decphase(phase);
            decon();
         }
         delay(r.t);
         if (p3 != 0) decoff();
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               decwfgoff(p4);
            }
	 }
         if (p3 == 0) decoff();
         decunblank();
         break;
      case 3:
         dec2unblank();
         if (p1 == 0) {
            dec2phase(phase);
            dec2on();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0) || ((strcmp(r.sh,"c") == 0) && (fabs(r.of) < 1.0))) dec2pwrf(ampl);
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               dec2wfgon(r.pattern,r.n90*12.5e-9,90.0,p2,r.offstdelay);
            }
	 }
         if (p1 != 0) {
            dec2phase(phase);
            dec2on();
         }
         delay(r.t);
         if (p3 != 0) dec2off();
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               dec2wfgoff(p4);
            }
	 }
         if (p3 == 0) dec2off();
         dec2unblank();
	 break;
      case 4:
         dec3unblank();
         if (p1 == 0) {
            dec3phase(phase);
            dec3on();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0) || ((strcmp(r.sh,"c") == 0) && (fabs(r.of) < 1.0))) dec3pwrf(ampl);
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               dec3wfgon(r.pattern,r.n90*12.5e-9,90.0,p2,r.offstdelay);
            }
	 }
         if (p1 != 0) {
            dec3phase(phase);
            dec3on();
         }
         delay(r.t);
         if (p3 != 0) dec3off();
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               dec3wfgoff(p4);
            }
	 }
         if (p3 == 0) dec3off();
         dec3unblank();
         break;
      default:
         printf("_ramp() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }
}

//==========================
//  Clear a RAMP Waveform
//==========================

void _clearramp(RAMP r)
{
   int chnl;
   chnl = 0;
   if (!strcmp(r.ch,"obs")) chnl = 1;
   else if (!strcmp(r.ch,"dec")) chnl = 2;
   else if (!strcmp(r.ch,"dec2")) chnl = 3;
   else if (!strcmp(r.ch,"dec3")) chnl = 4;
   else {
      printf("_ramp() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
   }

    dps_off();
    switch (chnl) {
      case 1:
         if ((r.t > 0.0) && ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0))) {
            clearobswfg();
	 }
         obsunblank();
	 break;
      case 2:
         if ((r.t > 0.0) && ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0))) {
            cleardecwfg();
	 }
         decunblank();
         break;
      case 3:
          if ((r.t > 0.0) && ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0))) {
            cleardec2wfg();
         }
         dec2unblank();
	 break;
      case 4:
         if ((r.t > 0.0) && ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0))) {
            cleardec3wfg();
         }
         dec3unblank();
         break;
      default:
         printf("_clearramp() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }
   dps_on();
}

//==============================================
//Underscore Functions to Turn Ramps On and Off
//==============================================

void _rampon(RAMP r, codeint phase)
{
  int chnl;
  chnl = 0;
  if (!strcmp(r.ch,"obs")) chnl = 1;
  else if (!strcmp(r.ch,"dec")) chnl = 2;
  else if (!strcmp(r.ch,"dec2")) chnl = 3;
  else if (!strcmp(r.ch,"dec3")) chnl = 4;
  else {
        printf("_rampon() Error: Undefined Channel! < 0!\n");
        psg_abort(1);
  }

  double ampl = r.a + fabs(r.d);

   int p1 = 0;
   int p2 = 0;

   if (r.preset1 == 0) { p1 = 0; p2 = 0;};
   if (r.preset1 == 1) { p1 = 1; p2 = 0;};
   if (r.preset1 == 2) { p1 = 1; p2 = 1;};

   switch (chnl) {
      case 1:
         obsunblank();
         if (p1 == 0) {
            txphase(phase);
            xmtron();
         }         
         if ((PWRF_DELAY == 0.0) || (p2 == 0) || ((strcmp(r.sh,"c") == 0) && (fabs(r.of) < 1.0))) obspwrf(ampl);
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               obswfgon(r.pattern,r.n90*12.5e-9,90.0,p2,r.offstdelay);
            }
	 }
         if (p1 != 0) {
            txphase(phase);
            xmtron();
         }
         break;
      case 2:
         decunblank();
         if (p1 == 0) {
            decphase(phase);
            decon();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0) || ((strcmp(r.sh,"c") == 0) && (fabs(r.of) < 1.0))) decpwrf(ampl);
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               decwfgon(r.pattern,r.n90*12.5e-9,90.0,p2,r.offstdelay);
            }
	 }
         if (p1 != 0) {
            decphase(phase);
            decon();
         }
         break;
      case 3:
         decunblank();
         if (p1 == 0) {
            dec2phase(phase);
            dec2on();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0) || ((strcmp(r.sh,"c") == 0) && (fabs(r.of) < 1.0))) dec2pwrf(ampl);
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               dec2wfgon(r.pattern,r.n90*12.5e-9,90.0,p2,r.offstdelay);
            }
	 }
         if (p1 != 0) {
            dec2phase(phase);
            dec2on();
         }
         break;
      case 4:
         dec3unblank();
         if (p1 == 0) {
            dec3phase(phase);
            dec3on();
         }
         if ((PWRF_DELAY == 0.0) || (p2 == 0) || ((strcmp(r.sh,"c") == 0) && (fabs(r.of) < 1.0))) dec3pwrf(ampl);
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               dec3wfgon(r.pattern,r.n90*12.5e-9,90.0,p2,r.offstdelay);
            }
         }
         if (p1 != 0) {
            dec3phase(phase);
            decon();
         }
         break;
      default:
         printf("_rampon() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }
}

void _rampoff(RAMP r)
{
   int chnl;
   chnl = 0;
   if (!strcmp(r.ch,"obs")) chnl = 1;
   else if (!strcmp(r.ch,"dec")) chnl = 2;
   else if (!strcmp(r.ch,"dec2")) chnl = 3;
   else if (!strcmp(r.ch,"dec3")) chnl = 4;
   else {
      printf("_rampoff() Error: Undefined Channel! < 0!\n");
      psg_abort(1);
  }

   int p3 = 0;
   int p4 = 0;

   if (r.preset2 == 0) { p3 = 0; p4 = 0;};
   if (r.preset2 == 1) { p3 = 1; p4 = 0;};
   if (r.preset2 == 2) { p3 = 1; p4 = 1;};

   switch (chnl) {
      case 1:
         if (p3 != 0) xmtroff();
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               obswfgoff(p4);
            }
	 }
         if (p3 == 0) xmtroff();
         obsunblank();
         break;
      case 2:
         if (p3 != 0) decoff();
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               decwfgoff(p4);
            }
         }
         if (p3 == 0) decoff();
         decunblank();
         break;
      case 3:
         if (p3 != 0) dec2off();
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               dec2wfgoff(p4);
            }
         }
         if (p3 == 0) dec2off();
         dec2unblank();
         break;
      case 4:
         if (p3 != 0) dec3off();
	 if (r.t > 0.0) {
            if ((strcmp(r.sh,"c") != 0) || (fabs(r.of) >= 1.0)) {
               dec3wfgoff(p4);
            }
	 }
         if (p3 == 0) dec3off();
         dec3unblank();
         break;
      default:
         printf("_rampoff() Error: Undefined Channel! < 0!\n");
         psg_abort(1);
         break;
   }
}

//=============================================
//Underscore Function to Output a DREAM Pulse
//=============================================

void _dream(DREAM d, codeint phase)
{
   mod4(ct,d.vcount);
   ifzero(d.vcount);
      _ramp(d.Ruu,phase);
   endif(d.vcount);
   sub(d.vcount,one,d.vcount);
   ifzero(d.vcount);
      _ramp(d.Rud,phase);
   endif(d.vcount);
   sub(d.vcount,one,d.vcount);
   ifzero(d.vcount);
      _ramp(d.Rdu,phase);
   endif(d.vcount);
   sub(d.vcount,one,d.vcount);
   ifzero(d.vcount);
      _ramp(d.Rdd,phase);
   endif(d.vcount);
}

//=======================================
// Wavegen Functions for VNMRS and INOVA
//=======================================

void adjdelay(double a, double *b)
{
   *b = *b - a;
   if (*b < 0.0) *b = 0.0;
}

void adj_initcp_(CP a, double *b)
{
   char str[MAXSTR];
   FILE *fp;
   extern char userdir[];
   sprintf(str,"%s/shapelib/PRESET.DEC",userdir);

   if((fp = fopen(str,"w"))==NULL) {
      printf("Error in adj_initcp(): can not create file PRESET.DEC!\n");
      psg_abort(1);
   }
   fprintf(fp," 90.0 0.0 1023.0\n");
   fclose(fp);

   if ((a.t > 0.0) && ((strcmp(a.sh,"c") != 0) || (fabs(a.of) >= 1.0))) {
      *b = *b - 2.0*a.strtdelay - 2.0*a.apdelay;
   }
   if (*b < 0.0) *b = 0.0;
}

void adj_setcp_(CP a, double *b)
{
   *b = *b - 2.0*a.apdelay;
   if ((a.t > 0.0) && ((strcmp(a.sh,"c") != 0) || (fabs(a.of) >= 1.0))) {
      *b = *b - a.strtdelay;
   }
   if (*b < 0.0) *b = 0.0;
}

CP adj_cp_(CP a, double *b, double *c, int lp1, int lp2, int lp3)
{
   a.preset1 = lp1;
   a.preset2 = lp2;
   a.preset3 = lp3;

// lp3 = 1 not allowed with "to" and ramp - switch to lp3 = 0

   if ((lp3 == 1) && (strcmp(a.ch,"to") == 0)) {
      lp3 = 0;
      a.preset3 = lp3;
   }
   if ((lp1 == 0) || (lp1 == 1)) {
      *b = *b - a.apdelay;
      if ((a.t > 0.0) && ((strcmp(a.sh,"c") != 0) || (fabs(a.of) >= 1.0))) {
         *b = *b - a.strtdelay - a.offstdelay;
      }
   }
   if (lp1 == 2) {
      if ((strcmp(a.sh,"c") == 0) && (fabs(a.of) < 1.0)) {
         *b = *b - a.apdelay;
      }
      if ((a.t > 0.0) && ((strcmp(a.sh,"c") != 0) || (fabs(a.of) >= 1.0))) {
         *b = *b - a.offstdelay;
      }
   }
   if (lp3 == 0) *b = *b - a.apdelay;
   if ((lp2 == 0) || (lp2 == 1)) {
      if ((a.t > 0.0) && ((strcmp(a.sh,"c") != 0) || (fabs(a.of) >= 1.0))) {
         *c = *c - a.apdelay;
      }
   }
   if (*b < 0.0) *b = 0.0;
   if (*c < 0.0) *c = 0.0;
   return a;
}

void adj_clearcp(CP a, double *b)
{
   if ((a.t > 0.0) && ((strcmp(a.sh,"c") != 0) || (fabs(a.of) >= 1.0))) {
      *b = *b - a.apdelay;
   }
   if (*b < 0.0) *b = 0.0;
}

void adj_initmpseq(MPSEQ a, double *b)
{
   char str[MAXSTR];
   FILE *fp;
   extern char userdir[];
   sprintf(str,"%s/shapelib/PRESET.DEC",userdir);

   if((fp = fopen(str,"w"))==NULL) {
      printf("Error in adj_initcp(): can not create file PRESET.DEC!\n");
      psg_abort(1);
   }
   fprintf(fp," 90.0 0.0 1023.0\n");
   fclose(fp);

   if (a.nelem > 0) {
      *b = *b - 2.0*a.strtdelay - 2.0*a.apdelay;
   }
   if (*b < 0.0) *b = 0.0;
}

void adj_setmpseq(MPSEQ a, double *b)
{
   *b = *b - a.apdelay;
   if (a.nelem > 0) {
      *b = *b - a.strtdelay;
   }
   if (*b < 0.0) *b = 0.0;
}

MPSEQ adj_mpseq(MPSEQ a, double *b, double *c, int lp1, int lp2)
{
   a.preset1 = lp1;
   a.preset2 = lp2;

   if ((lp1 == 0) || (lp1 == 1)) {
      *b = *b - a.apdelay;
      if (a.nelem > 0) {
         *b = *b - a.offstdelay - a.strtdelay;
      }
   }
   if ((a.nelem > 0) && (lp1 == 2)) *b = *b - a.offstdelay;
   if ((a.nelem > 0) && ((lp2 == 0) || (lp2 == 1))) *c = *c - a.apdelay;
   if (*b < 0.0) *b = 0.0;
   if (*c < 0.0) *c = 0.0;
   return a;
}

void adj_clearmpseq(MPSEQ a, double *b)
{
   if (a.nelem > 0) {
      *b = *b - a.apdelay;
   }
   if (*b < 0.0) *b = 0.0;
}

void adj_initramp(RAMP a, double *b)
{
   char str[MAXSTR];
   FILE *fp;
   extern char userdir[];
   sprintf(str,"%s/shapelib/PRESET.DEC",userdir);

   if((fp = fopen(str,"w"))==NULL) {
      printf("Error in adj_initcp(): can not create file PRESET.DEC!\n");
      psg_abort(1);
   }
   fprintf(fp," 90.0 0.0 1023.0\n");
   fclose(fp);

   if ((a.t > 0.0) && ((strcmp(a.sh,"c") != 0) || (fabs(a.of) >= 1.0))) {
      *b = *b - 2.0*a.strtdelay - 2.0*a.apdelay;
   }
   if (*b < 0.0) *b = 0.0;
}

void adj_setramp(RAMP a, double *b)
{
   *b = *b - a.apdelay;
   if ((a.t > 0.0) && ((strcmp(a.sh,"c") != 0) || (fabs(a.of) >= 1.0))) {
      *b = *b - a.strtdelay;
   }
   if (*b < 0.0) *b = 0.0;
}

RAMP adj_ramp(RAMP a, double *b, double *c, int lp1, int lp2)
{
   a.preset1 = lp1;
   a.preset2 = lp2;

   if ((lp1 == 0) || (lp1 == 1)) {
      *b = *b - a.apdelay;
      if ((a.t > 0.0) && ((strcmp(a.sh,"c") != 0) || (fabs(a.of) >= 1.0))) {
         *b = *b - a.strtdelay - a.offstdelay;
      }
   }
   if (lp1 == 2) {
      if ((strcmp(a.sh,"c") == 0) && (fabs(a.of) < 1.0)) {
         *b = *b - a.apdelay;
      }
      if ((a.t > 0.0) && ((strcmp(a.sh,"c") != 0) || (fabs(a.of) >= 1.0))) {
         *b = *b - a.offstdelay;
      }
   }
   if ((lp2 == 0) || (lp2 == 1)) {
      if ((a.t > 0.0) && ((strcmp(a.sh,"c") != 0) || (fabs(a.of) >= 1.0))) {
         *c = *c - a.apdelay;
      }
   }
   if (*b < 0.0) *b = 0.0;
   if (*c < 0.0) *c = 0.0;
   return a;
}

void adj_clearramp(RAMP a, double *b)
{
   if ((a.t > 0.0) && ((strcmp(a.sh,"c") != 0) || (fabs(a.of) >= 1.0))) {
      *b = *b - a.apdelay;
   }
   if (*b < 0.0) *b = 0.0;
}

void adj_initshape(SHAPE a, double *b)
{
   char str[MAXSTR];
   FILE *fp;
   extern char userdir[];
   sprintf(str,"%s/shapelib/PRESET.DEC",userdir);

   if((fp = fopen(str,"w"))==NULL) {
      printf("Error in adj_initcp(): can not create file PRESET.DEC!\n");
      psg_abort(1);
   }
   fprintf(fp,"90.0 0.0 1023.0\n");
   fclose(fp);

   if (a.pars.t > 0) {
      *b = *b - 2.0*a.pars.strtdelay - 2.0*a.pars.apdelay;
   }
   if (*b < 0.0) *b = 0.0;
}

void adj_setshape(SHAPE a, double *b)
{
   *b = *b - a.pars.apdelay;
   if (a.pars.t > 0) {
      *b = *b - a.pars.strtdelay;
   }
   if (*b < 0.0) *b = 0.0;
}

SHAPE adj_shape(SHAPE a, double *b, double *c, int lp1, int lp2)
{
   a.pars.preset1 = lp1;
   a.pars.preset2 = lp2;
   if ((lp1 == 0) || (lp1 == 1)) {
      *b = *b - a.pars.apdelay;
      if (a.pars.t > 0.0) {
         *b = *b - a.pars.offstdelay - a.pars.strtdelay;
      }
   }
   if ((a.pars.t > 0.0) && (lp1 == 2)) *b = *b - a.pars.offstdelay;
   if ((a.pars.t > 0.0) && ((lp2 == 0) || (lp2 == 1))) *c = *c - a.pars.apdelay;
   if (*b < 0.0) *b = 0.0;
   if (*c < 0.0) *c = 0.0;
   return a;
}

void adj_clearshape(SHAPE a, double *b)
{
   if (a.pars.t > 0.0) {
      *b = *b - a.pars.apdelay;
   }
   if (*b < 0.0) *b = 0.0;
}

//=======================================
// Functions to Write Structure Contents
//=======================================

void dump_mpseq(MPSEQ a)
{
   int i;
   printf("MPSEQ WAVEFORM INFORMATION\n"); 
   printf("seqName = %10s : Parameter Group Name\n",a.seqName);
   printf("pattern = %10s : Filename for Shape\n",a.pattern); 
   printf("nRec    = %10d : Copy Number in Sequence\n",a.nRec);
   printf("ch      = %10s : Channel\n",a.ch); 
   printf("a       = %10.0f : Scaled Amplitude\n",a.a); 
   printf("t       = %10.2f : Pattern Duration (microseconds)\n",a.t*1.0e6); 
   printf("nelem   = %10d : Base Cycles in Pattern\n",a.nelem); 
   printf("telem   = %10.2f : Base Cycle Duration (microseconds)\n",a.telem*1.0e6); 
   printf("n90     = %10d : Ticks (12.5 ns) per Minimum Step\n",a.n90);
   printf("nphBase = %10d : Base Cycle Steps\n",a.nphBase);
   printf("  DURATION      PHASE  AMPLITUDE       GATE     OFFSET\n");
   for (i = 0; i < a.nphBase; i++) {
      printf("%10.1f %10.1f %10.1f %10.1f %10.1f\n",a.pw[i%a.npw]*1e6,a.phBase[i%a.nph],
              a.aBase[i%a.no],a.gateBase[i%a.ng],a.of[i%a.no]);
   }
   printf("npw = %4d nph = %4d na = %5d ng = %5d no = %5d\n",a.npw,a.nph,a.na,a.ng,a.no);
   printf("nphSuper= %10d : Super Cycle Steps\n",a.nphSuper);
   printf("      STEP      PHASE\n");
   for (i = 0; i < a.nphSuper; i++) {
      printf("%10d %10.1f\n",i,a.phSuper[i%a.nph]);              
   }
   printf("OFFSET PHASE CONTROL:\n");
   printf("iSuper  = %10d : Next Super Cycle Step\n",a.iSuper);
   printf("phAccum = %10.4f : Degrees of Accumlated Phase\n",a.phAccum);
   printf("phInt   = %10.4f : Degrees Increment of Phase\n",a.phInt);
   printf("ARRAY CONTROL:\n");
   printf("calc    = %10d : = 1 for Calculation Complete\n",a.calc);
   printf("hasArray= %10d : = 1 for New Shapefile\n",a.hasArray);
   printf("INOVA PRESET CONTROL:\n");
   printf("preset1 = %10d : = 0,1,2 for none, partial and full Preset\n",a.preset1); 
   printf("preset2 = %10d : = 0,1,2 for none, partial and full Clear\n",a.preset2);
   printf("strtdelay = %8.2f : Start Delay (microseconds)\n",a.strtdelay*1.0e6); 
   printf("offstdelay= %8.2f : Offset Delay (microseconds)\n",a.offstdelay*1.0e6);
   printf("apdelay = %10.2f : AP Bus Delay (microseconds)\n",a.apdelay*1.0e6);
   printf("\n");
}
#endif

void dump_mpseq_array(MPSEQ a)
{
   int i,j,m,k,l;
   char temp[MAXSTR]; 
   printf("MPSEQ WAVEFORM ARRAY INFORMATION\n"); 
   printf("pattern = %10s : Filename for Shape\n",a.pattern); 
   printf("array.c = %10d : Number of Parameters\n", a.array.c);
   printf("     PARAM     COLUMN\n");
   m = strlen(a.array.a);
   for (i = 0; i < a.array.c; i++) {
      k = a.array.c - i - 1;
      m = m - a.array.b[k];
      l = 0;
      for (j = m; j < (m + a.array.b[k]); j++) {
         temp[l] = a.array.a[j];
         l++;
      }
      temp[l] = '\0'; 
      printf("%10s %10d\n",temp,a.array.f[k]);          
   }
   printf("array.d = %10d : Number of Array Columns\n", a.array.d);
   printf("    COLUMN   ARRAY_ON      INDEX       SIZE\n");
   for (i = 0; i < a.array.d; i++) {
      printf("%10d %10d %10d %10d\n",i,a.array.i[i],a.array.g[i],a.array.j[i]);     
   }
   printf("NEWSHAPE = %10d : = 1 For Shapefile Calculation\n",a.hasArray);
   printf("DURATION = %10.2f : Pattern Duration (microseconds)\n",a.t*1.0e6);
   printf("\n");
}

void dump_cp(CP a) 
{
   printf("CP WAVEFORM INFORMATION\n"); 
   printf("seqName = %10s : Parameter Group Name\n",a.seqName);
   printf("pattern = %10s : Filename for Shape\n",a.pattern); 
   printf("nRec    = %10d : Copy Number in Sequence\n",a.nRec);
   printf("fr      = %10s : Channel of Source Polarization\n",a.fr); 
   printf("to      = %10s : Channel of Final Polarization\n",a.to); 
   printf("ch      = %10s : Ramped Channel 'fr' or 'to' \n",a.ch); 
   printf("a1      = %10.0f : Scaled Amplitude of Source Channel\n",a.a1); 
   printf("a2      = %10.0f : Scaled Amplitude of Destination Channel\n",a.a2);
   printf("t       = %10.2f : Pattern Duration - Contact Time (microseconds)\n",a.t*1.0e6); 
   printf("sh      = %10s : Shape 'c', 'l', 't' \n",a.sh); 
   printf("d       = %10.0f : Ramp Width +/- d (overridden by 'sh')\n",a.d);
   printf("b       = %10.0f : Tangent Curvature (overridden by 'sh') \n",a.b); 
   printf("of      = %10.0f : Offset of Ramped Channel \n",a.of);
   printf("n90     = %10d : Ticks (12.5 ns) per Minimum Step\n",a.n90);
   printf("n       = %10d : Total Ticks (12.5 ns)\n",a.n);
   printf("OFFSET PHASE CONTROL of RAMPED CHANNEL\n");
   printf("phAccum = %10.4f : Degrees of Accumlated Phase\n",a.phAccum);
   printf("phInt   = %10.4f : Degrees Increment of Phase\n",a.phInt);
   printf("ARRAY CONTROL:\n");
   printf("calc    = %10d : = 1 for Calculation Complete\n",a.calc);
   printf("hasArray= %10d : = 1 for New Shapefile\n",a.hasArray);
   printf("INOVA PRESET CONTROL:\n");
   printf("preset1 = %10d : = 0,1,2 for none, partial and full Preset\n",a.preset1); 
   printf("preset2 = %10d : = 0,1,2 for none, partial and full Clear\n",a.preset2);
   printf("preset3 = %10d : = 1 Remove the Constant Amplitude Set\n",a.preset3);
   printf("strtdelay = %8.2f : Start Delay (microseconds)\n",a.strtdelay*1.0e6); 
   printf("offstdelay= %8.2f : Offset Delay (microseconds)\n",a.offstdelay*1.0e6);
   printf("apdelay = %10.2f : AP Bus Delay (microseconds)\n",a.apdelay*1.0e6);
   printf("\n");
}

void dump_cp_array(CP a)
{
   int i,j,m,k,l;
   char temp[MAXSTR]; 
   printf("CP WAVEFORM ARRAY INFORMATION\n"); 
   printf("pattern = %10s : Filename for Shape\n",a.pattern); 
   printf("array.c = %10d : Number of Parameters\n", a.array.c);
   printf("     PARAM     COLUMN\n");
   m = strlen(a.array.a);
   for (i = 0; i < a.array.c; i++) {
      k = a.array.c - i - 1;
      m = m - a.array.b[k];
      l = 0;
      for (j = m; j < (m + a.array.b[k]); j++) {
         temp[l] = a.array.a[j];
         l++;
      }
      temp[l] = '\0'; 
      printf("%10s %10d\n",temp,a.array.f[k]);          
   }
   printf("array.d = %10d : Number of Array Columns\n", a.array.d);
   printf("    COLUMN   ARRAY_ON      INDEX       SIZE\n");
   for (i = 0; i < a.array.d; i++) {
      printf("%10d %10d %10d %10d\n",i,a.array.i[i],a.array.g[i],a.array.j[i]);     
   }
   printf("NEWSHAPE = %10d : = 1 For Shapefile Calculation\n",a.hasArray);
   printf("DURATION = %10.2f : Pattern Duration (microseconds)\n",a.t*1.0e6);
   printf("\n");
}

void dump_ramp(RAMP a)
{
   printf("RAMP WAVEFORM INFORMATION\n"); 
   printf("seqName = %10s : Parameter Group Name\n",a.seqName);
   printf("pattern = %10s : Filename for Shape\n",a.pattern); 
   printf("nRec    = %10d : Copy Number in Sequence\n",a.nRec);
   printf("sh      = %10s : Shape 'c', 'l', 't' \n",a.sh); 
   printf("pol     = %10s : Polarity 'n','uu','ud','du','dd'\n",a.pol);
   printf("a       = %10.0f : Scaled Amplitude\n",a.a); 
   printf("t       = %10.2f : Pattern Duration - Contact Time (microseconds)\n",a.t*1.0e6); 
   printf("d       = %10.0f : Ramp Width +/- d \n",a.d);
   printf("b       = %10.0f : Tangent Curvature \n",a.b); 
   printf("of      = %10.0f : Offset\n",a.of);
   printf("n90     = %10d : Ticks (12.5 ns) per Minimum Step\n",a.n90);
   printf("n       = %10d : Total Ticks (12.5 ns)\n",a.n);
   printf("OFFSET PHASE CONTROL of RAMPED CHANNEL\n");
   printf("phAccum = %10.4f : Degrees of Accumlated Phase\n",a.phAccum);
   printf("phInt   = %10.4f : Degrees Increment of Phase\n",a.phInt);
   printf("ARRAY CONTROL:\n");
   printf("calc    = %10d : = 1 for Calculation Complete\n",a.calc);
   printf("hasArray= %10d : = 1 for New Shapefile\n",a.hasArray);
   printf("INOVA PRESET CONTROL:\n");
   printf("preset1 = %10d : = 0,1,2 for none, partial and full Preset\n",a.preset1); 
   printf("preset2 = %10d : = 0,1,2 for none, partial and full Clear\n",a.preset2);
   printf("strtdelay = %8.2f : Start Delay (microseconds)\n",a.strtdelay*1.0e6); 
   printf("offstdelay= %8.2f : Offset Delay (microseconds)\n",a.offstdelay*1.0e6);
   printf("apdelay = %10.2f : AP Bus Delay (microseconds)\n",a.apdelay*1.0e6);
   printf("\n");
}

void dump_ramp_array(RAMP a)
{
   int i,j,m,k,l;
   char temp[MAXSTR]; 
   printf("RAMP WAVEFORM ARRAY INFORMATION\n"); 
   printf("pattern = %10s : Filename for Shape\n",a.pattern); 
   printf("array.c = %10d : Number of Parameters\n", a.array.c);
   printf("     PARAM     COLUMN\n");
   m = strlen(a.array.a);
   for (i = 0; i < a.array.c; i++) {
      k = a.array.c - i - 1;
      m = m - a.array.b[k];
      l = 0;
      for (j = m; j < (m + a.array.b[k]); j++) {
         temp[l] = a.array.a[j];
         l++;
      }
      temp[l] = '\0'; 
      printf("%10s %10d\n",temp,a.array.f[k]);          
   }
   printf("array.d = %10d : Number of Array Columns\n", a.array.d);
   printf("    COLUMN   ARRAY_ON      INDEX       SIZE\n");
   for (i = 0; i < a.array.d; i++) {
      printf("%10d %10d %10d %10d\n",i,a.array.i[i],a.array.g[i],a.array.j[i]);     
   }
   printf("NEWSHAPE = %10d : = 1 For Shapefile Calculation\n",a.hasArray);
   printf("DURATION = %10.2f : Pattern Duration (microseconds)\n",a.t*1.0e6);
   printf("\n");
}

void dump_shape(SHAPE a)
{
   int i; 
   printf("SHAPE WAVEFORM INFORMATION\n"); 
   printf("seqName = %10s : Parameter Group Name\n",a.pars.seqName);
   printf("pattern = %10s : Filename for Shape\n",a.pars.pattern);  
   printf("nRec    = %10d : Copy Number in Sequence\n",a.pars.nRec);
   for (i = 0; i < 4; i++) {
      printf("dp[%d]   = %10.0f : Double for Calculation\n",i,a.pars.dp[i]);
   }
   for (i = 0; i < 4; i++) {
      printf("ip[%d]   = %10d : Integer for Calculation\n",i,a.pars.ip[i]);
   }
   printf("flag1[3]= %10s : Flag Parameter\n",a.pars.flag1); 
   printf("flag2[3]= %10s : Flag Parameter\n",a.pars.flag2); 
   printf("a       = %10.0f : Scaled Amplitude\n",a.pars.a); 
   printf("t       = %10.2f : Pattern Duration - Contact Time (microseconds)\n",a.pars.t*1.0e6); 
   printf("of      = %10.0f : Offset\n",a.pars.of);
   printf("n90     = %10d : Ticks (12.5 ns) per Minimum Step\n",a.pars.n90);
   printf("n       = %10d : Total Ticks (12.5 ns)\n",a.pars.n);
   printf("OFFSET PHASE CONTROL\n");
   printf("phAccum = %10.4f : Degrees of Accumlated Phase\n",a.pars.phAccum);
   printf("phInt   = %10.4f : Degrees Increment of Phase\n",a.pars.phInt);
   printf("ARRAY CONTROL:\n");
   printf("calc    = %10d : = 1 for Calculation Complete\n",a.pars.calc);
   printf("hasArray= %10d : = 1 for New Shapefile\n",a.pars.hasArray);
   printf("INOVA PRESET CONTROL:\n");
   printf("preset1 = %10d : = 0,1,2 for none, partial and full Preset\n",a.pars.preset1); 
   printf("preset2 = %10d : = 0,1,2 for none, partial and full Clear\n",a.pars.preset2);
   printf("strtdelay = %8.2f : Start Delay (microseconds)\n",a.pars.strtdelay*1.0e6); 
   printf("offstdelay= %8.2f : Offset Delay (microseconds)\n",a.pars.offstdelay*1.0e6);
   printf("apdelay = %10.2f : AP Bus Delay (microseconds)\n",a.pars.apdelay*1.0e6);
   printf("\n");
}

void dump_shape_array(SHAPE a)
{
   int i,j,m,k,l;
   char temp[MAXSTR]; 
   printf("SHAPE WAVEFORM ARRAY INFORMATION\n"); 
   printf("pattern = %10s : Filename for Shape\n",a.pars.pattern); 
   printf("array.c = %10d : Number of Parameters\n", a.pars.array.c);
   printf("     PARAM     COLUMN\n");
   m = strlen(a.pars.array.a);
   for (i = 0; i < a.pars.array.c; i++) {
      k = a.pars.array.c - i - 1;
      m = m - a.pars.array.b[k];
      l = 0;
      for (j = m; j < (m + a.pars.array.b[k]); j++) {
         temp[l] = a.pars.array.a[j];
         l++;
      }
      temp[l] = '\0'; 
      printf("%10s %10d\n",temp,a.pars.array.f[k]);          
   }
   printf("array.d = %10d : Number of Array Columns\n", a.pars.array.d);
   printf("    COLUMN   ARRAY_ON      INDEX       SIZE\n");
   for (i = 0; i < a.pars.array.d; i++) {
      printf("%10d %10d %10d %10d\n",i,a.pars.array.i[i],a.pars.array.g[i],a.pars.array.j[i]);     
   }
   printf("NEWSHAPE = %10d : = 1 For Shapefile Calculation\n",a.pars.hasArray);
   printf("DURATION = %10.2f : Pattern Duration (microseconds)\n",a.pars.t*1.0e6);
   printf("\n");
}
