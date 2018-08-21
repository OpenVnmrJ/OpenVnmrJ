/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#ifndef SOLIDDECSHAPES_H
#define SOLIDDECSHAPES_H

//Contents:

//Structures:
// TPPM                             TPPM Decoupling
// SPINAL                           SPINAL64 Decoupling
// SPINAL2                          SPINAL64 Decoupling - extra angle alp
// WALTZ16                          WALTZ16 Decoupling
// DSEQ                             Router to TPPM or SPINAL

//Implementation ("get") functions
// gettppm() - TPPM                 Build TPPM
// getspinal() - SPINAL             Build SPINAL
// getspinal2() - SPINAL            Build SPINAL2 - extra angle alp - with name SPINAL
// getspinal2s() - SPINAL           Build SPINAL2 - extra angle alp
// getwaltz() - WALTZ		    Build WALTZ
// getdseq() - DSEQ                 Choose TPPM SPINAL SPINAL2 or WALTZ with Hseq
// getdseq2() - DSEQ                obsolete
// setdseq() - DSEQ                 Choose TPPM SPINAL SPINAL2 or WALTZ with an Argument
// setdseq2() - DSEQ                obsolete

//Calculation of .DEC files
// make_tppm() - TPPM               Calculate TPPM
// make_spinal() - SPINAL           Calculate SPINAL
// make_spinal2() - SPINAL          Calculate SPINAL2 - extra angle alp
// make_waltz() - WALTZ             Calculate WALTZ
// make_paris() - PARIS             Calculate PARIS

//Delay-Adjust Functions

// adj_dseq() - DSEQ

//Run Time ("underscore") functions
// _tppm() - void                   Turn On TPPM (Run By _dseqon())
// _spinal - void                   Turn On SPINAL (Run By _dseqon())
// _spinal2- void                   Turn On SPINAL2 (Run By _dseqon())
// _waltz - void                    Turn On WALTZ (Run By _dseqon())
// _dseqon() - void                 Turn On TPPM SPINAL SPINAL2 or WALTZ
// _dseqon2() - void                obsolete
// _dseqoff() - void                Turn On TPPM SPINAL SPINAL2 or WALTZ
// _dseqoff2() - void               obsolete

//================================================
// Structures for Decoupling Elements
//================================================

//========
// TPPM
//========

typedef struct {
   int preset1;
   int preset2;
   double strtdelay;  //wavegen start delay (INOVA)
   double offstdelay; //wavegen offset delay (INOVA)
   double apdelay;    //ap bus and wfg stop delay (INOVA)
   double t;
   double a;
   double pw;
   double ph;
   double of;
   char   ch[NCH];
   int    hasArray;
   AR     array;
   char pattern[NPATTERN];
} TPPM;

//========
// SPINAL
//========

typedef struct {
   int preset1;
   int preset2;
   double strtdelay;  //wavegen start delay (INOVA)
   double offstdelay; //wavegen offset delay (INOVA)
   double apdelay;    //ap bus and wfg stop delay (INOVA)
   double t;
   double a;
   double pw;
   double ph;
   double of;
   char   ch[NCH];
   int    hasArray;
   AR     array;
   char pattern[NPATTERN];
} SPINAL;

//========
// SPINAL2
//========

typedef struct {
   int preset1;
   int preset2;
   double strtdelay;  //wavegen start delay (INOVA)
   double offstdelay; //wavegen offset delay (INOVA)
   double apdelay;    //ap bus and wfg stop delay (INOVA)
   double t;
   double a;
   double pw;
   double ph;
   double alp;
   double of;
   char   ch[NCH];
   int    hasArray;
   AR     array;
   char pattern[NPATTERN];
} SPINAL2;

//=========
// WALTZ16
//=========

typedef struct {
   int preset1;
   int preset2;
   double strtdelay;  //wavegen start delay (INOVA)
   double offstdelay; //wavegen offset delay (INOVA)
   double apdelay;    //ap bus and wfg stop delay (INOVA)
   double t;
   double a;
   double pw;
   double ph;
   double of; 
   char ch[NCH];
   int    hasArray;
   AR     array;
   char pattern[NPATTERN];
} WALTZ;

//========
// PARIS
//========

typedef struct {
   int preset1;
   int preset2;
   double strtdelay;  //wavegen start delay (INOVA)
   double offstdelay; //wavegen offset delay (INOVA)
   double apdelay;    //ap bus and wfg stop delay (INOVA)
   double t;
   double a;
   double pw;
   double of;
   char   ch[NCH];
   int    hasArray;
   AR     array;
   char pattern[NPATTERN];
} PARIS;

//================================================
// dseq manages the various decoupling structures
//================================================

typedef struct {
   TPPM t;
   SPINAL s;
   SPINAL2 r;
   WALTZ w;
   PARIS p;
   char name[10]; 
   char seq[14];
} DSEQ;

//===========================================
// Calculation of TPPM and SPINAL .DEC files
//===========================================

//=============
// Build TPPM
//=============

TPPM gettppm(char *name)
{
   TPPM dec;
   char *var;
   extern void make_tppm(TPPM dec);

   dec.preset1 = 0;
   dec.preset2 = 0;
   dec.strtdelay = WFG_START_DELAY - WFG_OFFSET_DELAY;
   dec.offstdelay = WFG_OFFSET_DELAY;
   dec.apdelay = PWRF_DELAY;

   dec.array = parsearry(dec.array);

// amplitude

   var = getname0("a","TPPM",name);
   dec.a = getval(var);

//pulse width

   var = getname0("pw","TPPM",name);
   dec.pw = getval(var);

//phase

   var = getname0("ph","TPPM",name);
   dec.ph = getval(var);
   dec.array = disarry(var, dec.array);

//channel

   var = getname0("ch","TPPM",name);
   Getstr(var,dec.ch,sizeof(dec.ch));

//cycle time

   dec.t = 64.0*dec.pw;

// create the shape

   int nRec = 0;
   char lpattern[NPATTERN];
   var = getname0("","TPPM",name);
   sprintf(lpattern,"%s%d",var,nRec);
   dec.hasArray = hasarry(dec.array, lpattern);
   int lix = arryindex(dec.array);
   var = getname0("","TPPM",name);
   sprintf(dec.pattern,"%s%d_%d",var,nRec,lix);
   if (dec.hasArray == 1) {
      make_tppm(dec);
   }
   return dec;
}

//===============
// Build SPINAL
//===============

SPINAL getspinal(char *name)
{
   SPINAL dec;
   char *var;
   extern void make_spinal(SPINAL dec);

   dec.preset1 = 0;
   dec.preset2 = 0;
   dec.strtdelay = WFG_START_DELAY - WFG_OFFSET_DELAY;
   dec.offstdelay = WFG_OFFSET_DELAY;
   dec.apdelay = PWRF_DELAY;

   dec.array = parsearry(dec.array);

//amplitude

   var = getname0("a","SPINAL",name);
   dec.a = getval(var);
//pulse width

   var = getname0("pw","SPINAL",name); 
   dec.pw = getval(var);

//phase

   var = getname0("ph","SPINAL",name);
   dec.ph = getval(var);  
   dec.array = disarry(var, dec.array);

//channel

   var = getname0("ch","SPINAL",name);
   Getstr(var,dec.ch,sizeof(dec.ch));

//cycle time

   dec.t = 64.0*dec.pw;
// create the shape

   int nRec = 0;  
   char lpattern[NPATTERN];
   var = getname0("","SPINAL",name);
   sprintf(lpattern,"%s%d",var,nRec);
   dec.hasArray = hasarry(dec.array, lpattern);
   int lix = arryindex(dec.array);
   var = getname0("","SPINAL",name);
   sprintf(dec.pattern,"%s%d_%d",var,nRec,lix);
   if (dec.hasArray == 1) {
      make_spinal(dec);
   }
   return dec;
}

//=============================
// Build SPINAL2 with 2 angles
//=============================

SPINAL2 getspinal2(char *name)
{
   SPINAL2 dec;
   char *var;
   extern void make_spinal2(SPINAL2 dec);

   dec.preset1 = 0;
   dec.preset2 = 0;
   dec.strtdelay = WFG_START_DELAY - WFG_OFFSET_DELAY;
   dec.offstdelay = WFG_OFFSET_DELAY;
   dec.apdelay = PWRF_DELAY;

   dec.array = parsearry(dec.array);

//amplitude

   var = getname0("a","SPINAL2",name);
   dec.a = getval(var);
//pulse width

   var = getname0("pw","SPINAL2",name); 
   dec.pw = getval(var);

//phase

   var = getname0("ph","SPINAL2",name);
   dec.ph = getval(var);  
   dec.array = disarry(var, dec.array);

//second phase

   var = getname0("alp","SPINAL2",name); 
   dec.alp = getval(var); 
   dec.array = disarry(var, dec.array);

//channel

   var = getname0("ch","SPINAL2",name);
   Getstr(var,dec.ch,sizeof(dec.ch));

//cycle time

   dec.t = 64.0*dec.pw;
// create the shape

   int nRec = 0;  
   char lpattern[NPATTERN];
   var = getname0("","SPINAL2",name);
   sprintf(lpattern,"%s%d",var,nRec);
   dec.hasArray = hasarry(dec.array, lpattern);
   int lix = arryindex(dec.array);
   var = getname0("","SPINAL2",name);
   sprintf(dec.pattern,"%s%d_%d",var,nRec,lix);
   if (dec.hasArray == 1) {
      make_spinal2(dec);
   }
   return dec;
}

//==============================================
// Build SPINAL2 with 2 angles with name SPINAL
//==============================================

SPINAL2 getspinal2s(char *name)
{
   SPINAL2 dec;
   char *var;
   extern void make_spinal2(SPINAL2 dec);

   dec.preset1 = 0;
   dec.preset2 = 0;
   dec.strtdelay = WFG_START_DELAY - WFG_OFFSET_DELAY;
   dec.offstdelay = WFG_OFFSET_DELAY;
   dec.apdelay = PWRF_DELAY;

   dec.array = parsearry(dec.array);

//amplitude

   var = getname0("a","SPINAL",name);
   dec.a = getval(var);
//pulse width

   var = getname0("pw","SPINAL",name); 
   dec.pw = getval(var);

//phase

   var = getname0("ph","SPINAL",name);
   dec.ph = getval(var);  
   dec.array = disarry(var, dec.array);

//second phase

   var = getname0("alp","SPINAL",name); 
   dec.alp = getval(var); 
   dec.array = disarry(var, dec.array);

//channel

   var = getname0("ch","SPINAL",name);
   Getstr(var,dec.ch,sizeof(dec.ch));

//cycle time

   dec.t = 64.0*dec.pw;
// create the shape

   int nRec = 0;  
   char lpattern[NPATTERN];
   var = getname0("","SPINAL",name);
   sprintf(lpattern,"%s%d",var,nRec);
   dec.hasArray = hasarry(dec.array, lpattern);
   int lix = arryindex(dec.array);
   var = getname0("","SPINAL",name);
   sprintf(dec.pattern,"%s%d_%d",var,nRec,lix);
   if (dec.hasArray == 1) {
      make_spinal2(dec);
   }
   return dec;
}

//===============
// Build WALTZ-16
//===============

WALTZ getwaltz(char *name)
{
   WALTZ dec;
   char *var;
   extern void make_waltz(WALTZ dec);

   dec.preset1 = 0;
   dec.preset2 = 0;
   dec.strtdelay = WFG_START_DELAY - WFG_OFFSET_DELAY;
   dec.offstdelay = WFG_OFFSET_DELAY;
   dec.apdelay = PWRF_DELAY;

   dec.array = parsearry(dec.array);

//amplitude

   var = getname0("a","WALTZ",name);
   dec.a = getval(var);
//pulse width

   var = getname0("pw","WALTZ",name); 
   dec.pw = getval(var);

//channel

   var = getname0("ch","WALTZ",name);
   Getstr(var,dec.ch,sizeof(dec.ch));

//cycle time

   dec.t = 64.0*dec.pw;
// create the shape

   int nRec = 0;  
   char lpattern[NPATTERN];
   var = getname0("","WALTZ",name);
   sprintf(lpattern,"%s%d",var,nRec);
   dec.hasArray = hasarry(dec.array, lpattern);
   int lix = arryindex(dec.array);
   var = getname0("","WALTZ",name);
   sprintf(dec.pattern,"%s%d_%d",var,nRec,lix);
   if (dec.hasArray == 1) {
      make_waltz(dec);
   }
   return dec;
}   

//=============
// Build PARIS
//=============

PARIS getparis(char *name)
{
   PARIS dec;
   char *var;
   extern void make_paris(PARIS dec);

   dec.preset1 = 0;
   dec.preset2 = 0;
   dec.strtdelay = WFG_START_DELAY - WFG_OFFSET_DELAY;
   dec.offstdelay = WFG_OFFSET_DELAY;
   dec.apdelay = PWRF_DELAY;

   dec.array = parsearry(dec.array);

// amplitude

   var = getname0("a","PARIS",name);
   dec.a = getval(var);

//pulse width

   var = getname0("pw","PARIS",name);
   dec.pw = getval(var);

//channel

   var = getname0("ch","PARIS",name);
   Getstr(var,dec.ch,sizeof(dec.ch));

//cycle time

   dec.t = 2.0*dec.pw;
 
// create the shape

   int nRec = 0;
   char lpattern[NPATTERN];
   var = getname0("","PARIS",name);
   sprintf(lpattern,"%s%d",var,nRec);   
   dec.hasArray = hasarry(dec.array, lpattern);
   int lix = arryindex(dec.array);
   var = getname0("","PARIS",name);
   sprintf(dec.pattern,"%s%d_%d",var,nRec,lix);
   if (dec.hasArray == 1) {
      make_paris(dec);
   }
   return dec;
}

//=================================================
// Make a TPPM waveform for DSEQ Decoupling
//=================================================

void make_tppm(TPPM dec)
{
   char shapepath[MAXSTR+16],str[3*MAXSTR];
   FILE *fp;
   int i;
   extern char userdir[];
   sprintf(shapepath,"%s/shapelib/",userdir);
   sprintf(str,"%s%s.DEC",shapepath,dec.pattern);

   if((fp = fopen(str,"w"))==NULL) {
      abort_message("Error in make_tppm(): can not create file %s!\n", str);
   }

   for (i = 0; i<8; i++) {
      fprintf(fp," 90.0 %10.3f\n", dec.ph);
      fprintf(fp," 90.0 %10.3f\n", -dec.ph);
      fprintf(fp," 90.0 %10.3f\n", dec.ph);
      fprintf(fp," 90.0 %10.3f\n", -dec.ph);
      fprintf(fp," 90.0 %10.3f\n", dec.ph);
      fprintf(fp," 90.0 %10.3f\n", -dec.ph);
      fprintf(fp," 90.0 %10.3f\n", dec.ph);
      fprintf(fp," 90.0 %10.3f\n", -dec.ph);
   }
   fclose(fp);
}

//==============================================
// Underscore Function to Start TPPM decoupling
//==============================================

void _tppm(TPPM dec)
{
   int chnl;
   double aTPPM;
   aTPPM = dec.a;
   chnl = 0;
   if (!strcmp(dec.ch,"obs")) chnl = 1;
   else if (!strcmp(dec.ch,"dec")) chnl = 2;
   else if (!strcmp(dec.ch,"dec2")) chnl = 3;
   else if (!strcmp(dec.ch,"dec3")) chnl = 4;
   else {
      abort_message("_tppm() Error: Undefined Channel! < 0!\n");
   }

   int p1 = 0;

   if (dec.preset1 == 0) p1 = 0;
   if (dec.preset1 == 1) p1 = 1;

   if (dec.a > 0.0) {
      switch (chnl) {
         case 1:
            obsunblank();
            if (p1 == 0) xmtron();
            obspwrf(aTPPM);
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               obswfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) xmtron();
	    break;
         case 2:
            decunblank();
            if (p1 == 0) decon();
            decpwrf(aTPPM);                   
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               decwfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) decon();
	    break;
         case 3:
            dec2unblank();
            if (p1 == 0) dec2on();
            dec2pwrf(aTPPM);
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) { 
               dec2wfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) dec2on();
            break;
         case 4:
            dec3unblank();
            if (p1 == 0) dec3on();
            dec3pwrf(aTPPM);
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               dec3wfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) dec3on();
	    break;
         default:
            abort_message("_tppm() Error: Undefined Channel! < 0!\n");
            break;
      }
   }
}

//=============================================
// Make a SPINAL Waveform for DSEQ Decoupling
//=============================================

void make_spinal(SPINAL dec)
{
   char shapepath[MAXSTR+16],str[3*MAXSTR];
   FILE *fp;
   extern char userdir[];
   int n,i;
   int sign[8] = {1,-1,-1,1,-1,1,1,-1};
   sprintf(shapepath,"%s/shapelib/",userdir);
   sprintf(str,"%s%s.DEC",shapepath,dec.pattern);

   if((fp = fopen(str,"w"))==NULL) {
      abort_message("Error in make_spinal(): can not create file %s!\n", str);
   }

   for (i = 0; i<8; i++) {
      n = i%8;
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(dec.ph));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(-dec.ph));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(1.5*dec.ph));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(-1.5*dec.ph));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(2.0*dec.ph));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(-2.0*dec.ph));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(1.5*dec.ph));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(-1.5*dec.ph));
   }
   fclose(fp);
}

//================================================
// Underscore Function to Start SPINAL decoupling
//================================================

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
      abort_message("_spinal() Error: Undefined Channel! < 0!\n");
   }

   int p1 = 0;
   if (dec.preset1 == 0) p1 = 0;
   if (dec.preset1 == 1) p1 = 1;

   if (dec.a > 0.0) {
      switch (chnl) {
         case 1: 
            obsunblank();
            if (p1 == 0) xmtron();
            obspwrf(aSPINAL);
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               obswfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) xmtron();
	    break;
         case 2:
            decunblank();
            if (p1 == 0) decon();
            decpwrf(aSPINAL);
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               decwfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) decon();
	    break;
         case 3:
            dec2unblank();
            if (p1 == 0) dec2on();
            dec2pwrf(aSPINAL);
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               dec2wfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) dec2on();
            break;
         case 4:
            dec3unblank();
            if (p1 == 0) dec3on();
            dec3pwrf(aSPINAL);                        
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               dec3wfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) dec3on();
	    break;
         default:
            abort_message("_spinal() Error: Undefined Channel! < 0!\n");
            break;
      }
   }
}

//=============================================
// Make a SPINAL2 Waveform for DSEQ Decoupling
//=============================================

void make_spinal2(SPINAL2 dec)
{
   char shapepath[MAXSTR+16],str[3*MAXSTR];
   FILE *fp;
   extern char userdir[];
   int n,i;
   int sign[8] = {1,-1,-1,1,-1,1,1,-1};
   sprintf(shapepath,"%s/shapelib/",userdir);
   sprintf(str,"%s%s.DEC",shapepath,dec.pattern);

   if((fp = fopen(str,"w"))==NULL) {
      abort_message("Error in make_spinal2(): can not create file %s!\n", str);
   }

   for (i = 0; i<8; i++) {
      n = i%8;
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(dec.ph));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(-dec.ph));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(dec.ph+dec.alp));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(-dec.ph-dec.alp));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(dec.ph+2.0*dec.alp));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(-dec.ph-2.0*dec.alp));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(dec.ph+dec.alp));
      fprintf(fp," 90.0 %10.3f\n", sign[n]*(-dec.ph-dec.alp));
   }
   fclose(fp);
}

//================================================
// Underscore Function to Start SPINAL2 decoupling
//================================================

void _spinal2(SPINAL2 dec)
{
   int chnl;
   double aSPINAL2;
   aSPINAL2 = dec.a;
   chnl = 0;
   if (!strcmp(dec.ch,"obs")) chnl = 1;
   else if (!strcmp(dec.ch,"dec")) chnl = 2;
   else if (!strcmp(dec.ch,"dec2")) chnl = 3;
   else if (!strcmp(dec.ch,"dec3")) chnl = 4;
   else {
      abort_message("_spinal2() Error: Undefined Channel! < 0!\n");
   }

   int p1 = 0;
   if (dec.preset1 == 0) p1 = 0;
   if (dec.preset1 == 1) p1 = 1;

   if (dec.a > 0.0) {
      switch (chnl) {
         case 1: 
            obsunblank();
            if (p1 == 0) xmtron();
            obspwrf(aSPINAL2);
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               obswfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) xmtron();
	    break;
         case 2:
            decunblank();
            if (p1 == 0) decon();
            decpwrf(aSPINAL2);
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               decwfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) decon();
	    break;
         case 3:
            dec2unblank();
            if (p1 == 0) dec2on();
            dec2pwrf(aSPINAL2);
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               dec2wfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) dec2on();
            break;
         case 4:
            dec3unblank();
            if (p1 == 0) dec3on();
            dec3pwrf(aSPINAL2);                        
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               dec3wfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) dec3on();
	    break;
         default:
            abort_message("_spinal2() Error: Undefined Channel! < 0!\n");
            break;
      }
   }
}

//=============================================
// Make a WALTZ Waveform for DSEQ Decoupling
//=============================================

void make_waltz(WALTZ dec)
{
   char shapepath[MAXSTR+16],str[3*MAXSTR];
   FILE *fp;
   extern char userdir[];
   int i;
   sprintf(shapepath,"%s/shapelib/",userdir);
   sprintf(str,"%s%s.DEC",shapepath,dec.pattern);

   if((fp = fopen(str,"w"))==NULL) {
      abort_message("Error in make_waltz(): can not create file %s!\n", str);
   }

   for (i = 0; i<8; i++) {
      fprintf(fp,"  90.0   270.0\n");
      fprintf(fp," 180.0     0.0\n");
      fprintf(fp,"  90.0   180.0\n");
      fprintf(fp," 180.0   270.0\n");
      fprintf(fp,"  90.0    90.0\n");
      fprintf(fp," 180.0   180.0\n");
      fprintf(fp,"  90.0     0.0\n");
      fprintf(fp," 180.0   180.0\n");
      fprintf(fp,"  90.0   270.0\n");
   }
   fclose(fp);
}

//==============================================
// Underscore Function to Start WALTZ decoupling
//==============================================

void _waltz(WALTZ dec)
{
   int chnl;
   double aWALTZ;
   aWALTZ = dec.a;
   chnl = 0;
   if (!strcmp(dec.ch,"obs")) chnl = 1;
   else if (!strcmp(dec.ch,"dec")) chnl = 2;
   else if (!strcmp(dec.ch,"dec2")) chnl = 3;
   else if (!strcmp(dec.ch,"dec3")) chnl = 4;
   else {
      abort_message("_waltz() Error: Undefined Channel! < 0!\n");
   }

   int p1 = 0;

   if (dec.preset1 == 0) p1 = 0;
   if (dec.preset1 == 1) p1 = 1;

   if (dec.a > 0.0) {
      switch (chnl) {
         case 1:
            obsunblank();
            if (p1 == 0) xmtron();
            obspwrf(aWALTZ);
            if (dec.pw > 0.5e-6) {
               obswfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) xmtron();
	    break;
         case 2:
            decunblank();
            if (p1 == 0) decon();
            decpwrf(aWALTZ);                   
            if (dec.pw > 0.5e-6) {
               decwfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) decon();
	    break;
         case 3:
            dec2unblank();
            if (p1 == 0) dec2on();
            dec2pwrf(aWALTZ);
            if (dec.pw > 0.5e-6) { 
               dec2wfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) dec2on();
            break;
         case 4:
            dec3unblank();
            if (p1 == 0) dec3on();
            dec3pwrf(aWALTZ);
            if ((dec.pw > 0.5e-6) && (dec.ph > 0.0)) {
               dec3wfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) dec3on();
	    break;
         default:
            abort_message("_waltz() Error: Undefined Channel! < 0!\n");
            break;
      }
   }
}

//======================================
// Make a PARIS waveform for Recoupling
//======================================

void make_paris(PARIS p)
{
   char shapepath[MAXSTR+16],str[3*MAXSTR];
   FILE *fp;
   extern char userdir[];
   int i;
   sprintf(shapepath,"%s/shapelib/",userdir);
   sprintf(str,"%s%s.DEC",shapepath,p.pattern);

   if((fp = fopen(str,"w"))==NULL) {
      abort_message("Error in make_paris(): can not create file %s!\n", str);
   } 

   for (i = 0; i<8; i++) {
      fprintf(fp," 90.0   0.000 \n");
      fprintf(fp," 90.0 180.000 \n"); 
      fprintf(fp," 90.0   0.000 \n");
      fprintf(fp," 90.0 180.000 \n"); 
      fprintf(fp," 90.0   0.000 \n");
      fprintf(fp," 90.0 180.000 \n"); 
      fprintf(fp," 90.0   0.000 \n");
      fprintf(fp," 90.0 180.000 \n");
   }
   fclose(fp);
}

//==============================================
// Underscore Function to Start PARIS Recoupling
//==============================================

void _paris(PARIS dec)
{
   int chnl;
   double aPARIS;
   aPARIS = dec.a;
   chnl = 0;
   if (!strcmp(dec.ch,"obs")) chnl = 1;
   else if (!strcmp(dec.ch,"dec")) chnl = 2;
   else if (!strcmp(dec.ch,"dec2")) chnl = 3;
   else if (!strcmp(dec.ch,"dec3")) chnl = 4;
   else {
      abort_message("_paris() Error: Undefined Channel! < 0!\n");
   }

   int p1 = 0;

   if (dec.a > 0.0) {
      switch (chnl) {
         case 1:
            obsunblank();
            if (p1 == 0) xmtron();
            obspwrf(aPARIS);
            if (dec.pw > 0.5e-6) {
               obswfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) xmtron();
	    break;
         case 2:
            decunblank();
            if (p1 == 0) decon();
            decpwrf(aPARIS);                   
            if (dec.pw > 0.5e-6) {
               decwfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) decon();
	    break;
         case 3:
            dec2unblank();
            if (p1 == 0) dec2on();
            dec2pwrf(aPARIS);
            if (dec.pw > 0.5e-6) { 
               dec2wfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) dec2on();
            break;
         case 4:
            dec3unblank();
            if (p1 == 0) dec3on();
            dec3pwrf(aPARIS);
            if (dec.pw > 0.5e-6) {
               dec3wfgon(dec.pattern,dec.pw,90.0,0,dec.offstdelay);
            }
            if (p1 != 0) dec3on();
	    break;
         default:
            abort_message("_paris() Error: Undefined Channel! < 0!\n");
            break;
      }
   }
}

//==============================================
// DSEQ Decoupling Sequence Utilities
//==============================================

//==============================================
// Select a Decoupling Sequence From Panel
//==============================================

DSEQ getdseq(char *name)
{
   DSEQ d;
   char var[13];
   sprintf(var,"%s",name);
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
   if (!strcmp(d.seq,"spinal2")) {
      d.r = getspinal2(name);
      return d;
   }
   if (!strcmp(d.seq,"waltz")) {
      d.w = getwaltz(name);
      return d;
   }
   if (!strcmp(d.seq,"paris")) {
      d.p = getparis(name);
      return d;
   }
   abort_message("getdseq() Error: Undefined Decoupling Sequence!%s \n",name);
}

//====================================================================
// Select a Decoupling Sequence From Panel with Default 2-Angle Spinal
//====================================================================

DSEQ getdseq2(char *name)
{
   printf("Sequence uses getdseq2 statement which will be obsolete soon. Please, use getdseq instead which provides the same functionality\n");
   DSEQ dec=getdseq(name);
   return dec;
}

//=========================================================
// Assign the Decoupling Sequence in the Pulse Sequence
//=========================================================

DSEQ setdseq(char *name, char *seq)
{
   DSEQ d;
   sprintf(d.seq,"%s",seq);

   if (!strcmp(d.seq,"tppm")) {
      d.t = gettppm(name);
      return d;
   }
   if (!strcmp(d.seq,"spinal2")) {
      d.r = getspinal2(name);
      return d;
   }
   if (!strcmp(d.seq,"spinal")) {
      d.s = getspinal(name);
      return d;
   }
   if (!strcmp(d.seq,"waltz")) {
      d.w = getwaltz(name);
      return d;
   }
   if (!strcmp(d.seq,"paris")) {
      d.p = getparis(name);
      return d;
   }
   abort_message("setdseq() Error: Undefined Decoupling Sequence!%s \n",name);
}

//=================================================================================
// Assign the Decoupling Sequence in the Pulse Sequence with Default 2-Angle Spinal
//=================================================================================

DSEQ setdseq2(char *name, char *seq)
{
   printf("Sequence uses setdseq2 statement which will be obsolete soon. Please, use setdseq instead which provides the same functionality\n");
   DSEQ dec=setdseq(name,seq);
   return dec;
}


//==================================================================
// Underscore Function to adjust the delays before and after a DSEQ
//==================================================================

DSEQ adj_dseq(DSEQ a, double *b, double *c, int lp1, int lp2)
{
   double strtdelay;
   double offstdelay;
   double apdelay;
   if (!strcmp(a.seq,"spinal")) {
      strtdelay = a.s.strtdelay;
      offstdelay = a.s.offstdelay;
      apdelay = a.s.apdelay;
   }
   else {
      strtdelay = a.t.strtdelay;
      offstdelay = a.t.offstdelay;
      apdelay = a.t.apdelay;
   }

   if (!strcmp(a.seq,"spinal")) {
      a.s.preset1 = lp1;
      a.s.preset2 = lp2;
   }
   else {
      a.t.preset1 = lp1;
      a.t.preset2 = lp2;
   }

   if ((lp1 == 0) || (lp1 == 1)) {
      if (!strcmp(a.seq,"spinal")) {
         if (a.s.a > 0.0) {
            if ((a.s.pw > 0.5e-6) && (a.s.ph > 0.0)) {
               *b = *b - strtdelay - apdelay - offstdelay;
            }
            else {
               *b = *b - apdelay;
            }
         }
      }
      else {       
         if (a.t.a > 0.0) {
            if ((a.t.pw > 0.5e-6) && (a.t.ph > 0.0)) {
               *b = *b - strtdelay - apdelay - offstdelay;
            }
            else {
               *b = *b - apdelay;
            }
         }
      }
   }

   if ((lp2 == 0) || (lp2 == 1)) {
      if (!strcmp(a.seq,"spinal")) {
         if (a.s.a > 0.0) {
            *c = *c - apdelay;
         }
      }
      else {
         if (a.t.a > 0.0) {
            *c = *c - apdelay;
         }
      }
   }
   if (*b < 0.0) *b = 0.0;
   if (*c < 0.0) *c = 0.0;

   return a;
}

//==========================================================
// Underscore Function to Turn On and Off a DSEQ
//==========================================================

void dseqoff_emergency(char* message)
{
     xmtroff();
     decoff();
     dec2off();
     dec3off();
     obsprgoff();
     decprgoff();
     dec2prgoff();
     dec3prgoff();
     abort_message("%s",message);
}

void _dseqon(DSEQ dseq)
{
   if (!strcmp(dseq.seq,"tppm")) {
      _tppm(dseq.t);
      return;
   }
   if (!strcmp(dseq.seq,"spinal")) {
      _spinal(dseq.s);
      return;
   }
   if (!strcmp(dseq.seq,"spinal2")) {
      _spinal2(dseq.r);
      return;
   }
   if (!strcmp(dseq.seq,"waltz")) {
      _waltz(dseq.w);
      return;
   }
    if (!strcmp(dseq.seq,"paris")) {
      _paris(dseq.p);
      return;
   }
   abort_message("Error in _dseqon. Unrecognized Sequence! \n");
}

void _dseqoff(DSEQ dseq)
{
   int preset2;
   if (!strcmp(dseq.seq,"spinal")) {
      preset2 = dseq.s.preset2;
   }
   else if (!strcmp(dseq.seq,"spinal2")) {
      preset2 = dseq.r.preset2;
   }
   else if (!strcmp(dseq.seq,"waltz")) {
      preset2 = dseq.w.preset2;
   }
   else if (!strcmp(dseq.seq,"paris")) {
      preset2 = dseq.p.preset2;
   }
   else { //TPPM
      preset2 = dseq.t.preset2;
   }

   int p3 = 0;
   if (preset2 == 0) p3 = 0;
   if (preset2 == 1) p3 = 1;

   if (!strcmp(dseq.seq,"tppm")) {
      if (dseq.t.a > 0.0) {
         if (!strcmp(dseq.t.ch,"obs")) {
            if (p3 != 0) xmtroff();
            if ((dseq.t.pw > 0.5e-6) && (dseq.t.ph > 0.0)) {
               obsprgoff();
            }
            if (p3 == 0) xmtroff();
            obsunblank();
         }
         else if (!strcmp(dseq.t.ch,"dec")) {
            if (p3 != 0) decoff();
            if ((dseq.t.pw > 0.5e-6) && (dseq.t.ph > 0.0)) {
               decprgoff();
            }
            if (p3 == 0) decoff();
            decunblank();
         }
         else if (!strcmp(dseq.t.ch,"dec2")) {
            if (p3 != 0) dec2off();
            if ((dseq.t.pw > 0.5e-6) && (dseq.t.ph > 0.0)) {
               dec2prgoff();
            }
            if (p3 == 0) dec2off();
            dec2unblank();
         }
         else if (!strcmp(dseq.t.ch,"dec3")) {
            if (p3 != 0) dec3off();
            if ((dseq.t.pw > 0.5e-6) && (dseq.t.ph > 0.0)) {
               dec3prgoff();
            }
            if (p3 == 0) dec3off();
            dec3unblank();
         }
         else dseqoff_emergency("dseqoff with tppm: cannot recognize channel. Switching decoupling off\n");
      }
    }
   else if (!strcmp(dseq.seq,"spinal")) {
      if (dseq.s.a > 0.0) {
         if (!strcmp(dseq.s.ch,"obs"))  {
            if (p3 != 0) xmtroff();
            if ((dseq.s.pw > 0.5e-6) && (dseq.s.ph > 0.0)) {
               obsprgoff();
            }
	    if (p3 == 0) xmtroff();
	    obsunblank();
         }
         else if (!strcmp(dseq.s.ch,"dec"))  {
            if (p3 != 0)  decoff();
            if ((dseq.s.pw > 0.5e-6) && (dseq.s.ph > 0.0)) {
               decprgoff();
            }
            if (p3 == 0)  decoff();
            decunblank();
         }
         else if (!strcmp(dseq.s.ch,"dec2")) {
            if (p3 != 0) dec2off();
            if ((dseq.s.pw > 0.5e-6) && (dseq.s.ph > 0.0)) {
               dec2prgoff();
            }
	    if (p3 == 0) dec2off();
	    dec2unblank();
         }
         else if (!strcmp(dseq.s.ch,"dec3")) {
            if (p3 == 0) dec3off();
            if ((dseq.s.pw > 0.5e-6) && (dseq.s.ph > 0.0)) {
               dec3prgoff();
            }
            if (p3 == 0) dec3off();
            dec3unblank();
         }
         else dseqoff_emergency("dseqoff with spinal: cannot recognize channel. Switching decoupling off\n");
      }
   }
   else if (!strcmp(dseq.seq,"spinal2")) {
      if (dseq.r.a > 0.0) {
         if (!strcmp(dseq.r.ch,"obs"))  {
            if (p3 != 0) xmtroff();
            if ((dseq.r.pw > 0.5e-6) && (dseq.r.ph > 0.0)) {
               obsprgoff();
            }
	    if (p3 == 0) xmtroff();
	    obsunblank();
         }
         else if (!strcmp(dseq.r.ch,"dec"))  {
            if (p3 != 0)  decoff();
            if ((dseq.r.pw > 0.5e-6) && (dseq.r.ph > 0.0)) {
               decprgoff();
            }
            if (p3 == 0)  decoff();
            decunblank();
         }
         else if (!strcmp(dseq.r.ch,"dec2")) {
            if (p3 != 0) dec2off();
            if ((dseq.r.pw > 0.5e-6) && (dseq.r.ph > 0.0)) {
               dec2prgoff();
            }
	    if (p3 == 0) dec2off();
	    dec2unblank();
         }
         else if (!strcmp(dseq.r.ch,"dec3")) {
            if (p3 == 0) dec3off();
            if ((dseq.r.pw > 0.5e-6) && (dseq.r.ph > 0.0)) {
               dec3prgoff();
            }
            if (p3 == 0) dec3off();
            dec3unblank();
         }
         else dseqoff_emergency("dseqoff with spinal2: cannot recognize channel. Switching decoupling off\n");
      }
   }
   else if (!strcmp(dseq.seq,"waltz")) {
      if (dseq.w.a > 0.0) {
         if (!strcmp(dseq.w.ch,"obs"))  {
            if (p3 != 0) xmtroff();
            if (dseq.w.pw > 0.5e-6) {
               obsprgoff();
            }
	    if (p3 == 0) xmtroff();
	    obsunblank();
         }
         else if (!strcmp(dseq.w.ch,"dec"))  {
            if (p3 != 0)  decoff();
            if (dseq.w.pw > 0.5e-6) {
               decprgoff();
            }
            if (p3 == 0)  decoff();
            decunblank();
         }
         else if (!strcmp(dseq.w.ch,"dec2")) {
            if (p3 != 0) dec2off();
            if (dseq.w.pw > 0.5e-6) {
               dec2prgoff();
            }
	    if (p3 == 0) dec2off();
	    dec2unblank();
         }
         else if (!strcmp(dseq.w.ch,"dec3")) {
            if (p3 == 0) dec3off();
            if (dseq.w.pw > 0.5e-6) {
               dec3prgoff();
            }
            if (p3 == 0) dec3off();
            dec3unblank();
         }
         else dseqoff_emergency("dseqoff with waltz: cannot recognize channel. Switching decoupling off\n");
        }
      }
   else if (!strcmp(dseq.seq,"paris")) {
      if (dseq.p.a > 0.0) {
         if (!strcmp(dseq.p.ch,"obs"))  {
            if (p3 != 0) xmtroff();
            if (dseq.p.pw > 0.5e-6) {
               obsprgoff();
            }
	    if (p3 == 0) xmtroff();
	    obsunblank();
         }
         else if (!strcmp(dseq.p.ch,"dec"))  {
            if (p3 != 0)  decoff();
            if (dseq.p.pw > 0.5e-6) {
               decprgoff();
            }
            if (p3 == 0)  decoff();
            decunblank();
         }
         else if (!strcmp(dseq.p.ch,"dec2")) {
            if (p3 != 0) dec2off();
            if (dseq.p.pw > 0.5e-6) {
               dec2prgoff();
            }
	    if (p3 == 0) dec2off();
	    dec2unblank();
         }
         else if (!strcmp(dseq.p.ch,"dec3")) {
            if (p3 == 0) dec3off();
            if (dseq.p.pw > 0.5e-6) {
               dec3prgoff();
            }
            if (p3 == 0) dec3off();
            dec3unblank();
         }
         else dseqoff_emergency("dseqoff with paris: cannot recognize channel. Switching decoupling off\n");
      }
   }
   else dseqoff_emergency("dseqoff: unknown .seq value. Switching decoupling off\n"); 
}
//==================================================================
// Underscore Functions to Turn On and Off a DSEQ with 2-angle SPINAL
//==================================================================

void _dseqon2(DSEQ dseq)
{
  printf("Sequence uses _dseqon2 statement which will be obsolete soon.Please, use _dseqon instead which provides the same functionality\n");
  _dseqon(dseq);
}

void _dseqoff2(DSEQ dseq)
{
  printf("Sequence uses _dseqoff2 statement which will be obsolete soon. Please, use _dseqoff instead which provides the same functionality\n");
  _dseqoff(dseq);
}
#endif
