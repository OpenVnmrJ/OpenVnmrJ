/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
// A set of INEPT and HSQC modules with optional 
// homodecoupling and an MPSEQ selector.

// Reference: M Pruski, J Wiench and T. Kobayashi 
// Ames Laboratory - 07/31/12
// Modified for VnmrJ3.2 -  D. Rice - 8/01/12

// Contents:

// THE MPDEC STRUCTURE AND UTILITIES
// struct MPDEC:           Structure for an MPSEQ Selector 

// MPDEC getmpdec():       Select an MPSEQ from a Parameter            
// MPDEC setmpdec():       Select an MPSEQ in the Sequence 

// INEPT OBJECTS WITH MPDEC HOMODECOUPLING

// void _ineptdec()        INEPT transfer with homodecoupling
// void _ineptrefdec()     Refocussed INEPT transfer with homodecoupling
// void _ineptdec2()       INEPT transfer with homodecoupling and a magic
//                         angle pulse
// void _ineptrefdec2()    Refocussed INEPT transfer with homodecoupling
//                         and a magic angle pulse 
// void _hsqcdec1()        HSQC transfer before with homodecoupling 
//                         and a magic angle pulse
// void _hsqcdec2()        HSQC transfer after with homodecoupling 
//                         and a magic angle pulse
//ADDITIONAL MPSEQ WAVEFORMS
//
// MPSEQ getpmlgsuper      Build PMLG with a supercycle.
//

//================================================
// Structure for an MPSEQ Selector 
//================================================
typedef struct {
   MPSEQ   mps;           // pointer to homodec. MPSEQ
   double  t;             // total time for n Cycles
   char    dm[14];        // decoupling mode 0 - off 2 - MPSEQ
   char    seqName[14];   // MPSEQ name 
   char    mpsName[14];   // name if different than seqName
   char    seq[14];       // homodec. sequence type: pmlg, fslg ...
} MPDEC;

//==============================================
// MPDEC Decoupling-sequence Utilities
//==============================================

//==============================================
// Select an MPSEQ From a Parameter
//==============================================

MPDEC getmpdec(char *name, int iph , double p, double phint, int iRec, int calc)
{
   MPDEC d;
   char *var;

   if (strlen(name) >= sizeof(d.seqName)) {
     printf("getmpdec() Error: name string is too long!%s \n",name);
     psg_abort(1);
   }
  
   strcpy(d.seqName,name);
   
   var = getname0("seq",d.seqName,"");
   
   Getstr(var,d.seq,sizeof(d.seq));

// dmXmpdec

   var = getname0("dm",d.seqName,"");
   Getstr(var,d.dm,sizeof(d.dm));

// mpsName - seqName for mpseq

   var = getname0(d.seq,d.seqName,"");
   strcpy(d.mpsName,d.seq);
   var = getname0("",d.seqName,"");
   strncat(d.mpsName,var,1);

   if (!strcmp(d.seq,"tppm") || !strcmp(d.seq,"spinal")) {
      d.mps = getspnl(name,iph,p,phint,iRec,calc);
      return d;
   }
   if (!strcmp(d.seq,"pmlg")) {
      d.mps = getpmlg(name,iph,p,phint,iRec,calc);
      return d;
   }
//   if (!strcmp(d.seq,"pmlgs")) {
//      d.mps = getpmlgsuper(name,iph,p,phint,iRec,calc);
//      return d;
//   }
   if (!strcmp(d.seq,"fslg")) {
      d.mps = getfslg(name,iph,p,phint,iRec,calc);
      return d;
   }
   if (!strcmp(d.seq,"sam")) {
      d.mps = getsamn(name,iph,p,phint,iRec,calc);
      return d;
   }
   printf("getmpdec() Error: Undefined Decoupling Sequence!%s \n",name);
   psg_abort(1);
}

//========================================
// Select an MPSEQ in the Pulse Sequence
//========================================

MPDEC setmpdec(char *name, int iph, double p, double phint, int iRec, int calc)
{
   MPDEC d;
   char *var;

   if (strlen(name) >= sizeof(d.seqName)) {
     printf("setmpdec() Error: name string is too long!%s \n",name);
     psg_abort(1);
   }

   strcpy(d.seqName,name);
   
   var = getname0("seq",d.seqName,"");
   
   Getstr(var,d.seq,sizeof(d.seq));

// dmXmpdec

   var = getname0("dm",d.seqName,"");
   Getstr(var,d.dm,sizeof(d.dm));

   if (!strcmp(d.seq,"tppm") || !strcmp(d.seq,"spinal")) {
      d.mps = getspnl(name,iph,p,phint,iRec,calc);
      return d;
   }
   if (!strcmp(d.seq,"pmlg")) {
      d.mps = getpmlg(name,iph,p,phint,iRec,calc);
      return d;
   }
//   if (!strcmp(d.seq,"pmlgs")) {
//      d.mps = getpmlgsuper(name,iph,p,phint,iRec,calc);
//      return d;
//   }
   if (!strcmp(d.seq,"fslg")) {
      d.mps = getfslg(name,iph,p,phint,iRec,calc);
      return d;
   }
   if (!strcmp(d.seq,"sam")) {
      d.mps = getsamn(name,iph,p,phint,iRec,calc);
      return d;
   }
   printf("setmpdec() Error: Undefined Decoupling Sequence!%s \n",name);
   psg_abort(1);
}

// ---------------------------------------
// INEPT Transfer Between Two Channels
// ---------------------------------------

void _ineptdec(GP in, int ph1, int ph2, int ph3, int ph4, MPDEC d1, 
                                                           int phd1)
{
   int chnl = 0;
   if (!strcmp(in.ch1,"obs")) chnl = 1;
   else if (!strcmp(in.ch1,"dec")) chnl = 2;
   else if (!strcmp(in.ch1,"dec2")) chnl = 3;
   else if (!strcmp(in.ch1,"dec3")) chnl = 4;
   else {
      printf("_ineptdec() Error: Undefined Source Channel. Abort!\n");
      psg_abort(1);
   }

   int chnl2 = 0;
   if (!strcmp(in.ch2,"obs")) chnl2 = 1;
   else if (!strcmp(in.ch2,"dec")) chnl2 = 2;
   else if (!strcmp(in.ch2,"dec2")) chnl2 = 3;
   else if (!strcmp(in.ch2,"dec3")) chnl2 = 4;
   else {
      printf("_ineptdec() Error: Undefined Destination Channel. Abort!\n");
      psg_abort(1);
   }

   if (chnl == chnl2) {
      printf("_ineptdec() Error: Source and Destination on Same Channel. Abort!\n");
      psg_abort(1);
   }

   double thd1 = d1.t;
   double t1a = in.t1;
   double t1b = in.t2;

   gen_RFphase(ph1,chnl); gen_RFphase(ph2,chnl2);
   gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
   gen_RFunblank(chnl); gen_RFunblank(chnl2);

   if (!strcmp(d1.dm,"y")) {
      t1a = t1a - thd1;
      _mpseqon(d1.mps,phd1);
      delay(thd1);
      _mpseqoff(d1.mps);
      gen_RFpwrf(in.a1,chnl);gen_RFpwrf(in.a2,chnl2);
      gen_RFunblank(chnl);gen_RFunblank(chnl2);
   }
   delay(t1a);
   gen_RFsimPulse(in.pw1,in.pw2,ph1,ph2,0.0,0.0,chnl,chnl2);
   gen_RFphase(ph3, chnl); gen_RFphase(ph4, chnl2);

   if (!strcmp(d1.dm,"y")) {
      t1b = t1b -thd1;
      _mpseqon(d1.mps,phd1);
      delay(thd1);
      _mpseqoff(d1.mps);
      gen_RFpwrf(in.a1, chnl);gen_RFpwrf(in.a2, chnl2);
      gen_RFunblank(chnl);gen_RFunblank(chnl2);
   }
   delay(t1b);
   gen_RFsimPulse(in.pw3,in.pw4,ph3,ph4,0.0,0.0,chnl,chnl2);
}

//---------------------------------------------------------------------
// INEPT Transfer Between Two Channels with Multiplet Refocussing
//--------------------------------------------------------------------

void _ineptrefdec(GP in, int ph1, int ph2, int ph3, int ph4, int ph5, 
                      int ph6, MPDEC d1, int phd1, MPDEC d2, int phd2)
{
   int chnl = 0;
   if (!strcmp(in.ch1,"obs")) chnl = 1;
   else if (!strcmp(in.ch1,"dec")) chnl = 2;
   else if (!strcmp(in.ch1,"dec2")) chnl = 3;
   else if (!strcmp(in.ch1,"dec3")) chnl = 4;
   else {
      printf("_ineptrefdec() Error: Undefined Source Channel. Abort!\n");
      psg_abort(1);
   }

   int chnl2 = 0;
   if (!strcmp(in.ch2,"obs")) chnl2 = 1;
   else if (!strcmp(in.ch2,"dec")) chnl2 = 2;
   else if (!strcmp(in.ch2,"dec2")) chnl2 = 3;
   else if (!strcmp(in.ch2,"dec3")) chnl2 = 4;
   else {
      printf("_ineptrefdec() Error: Undefined Destination Channel. Abort!\n");
      psg_abort(1);
   }

   if (chnl == chnl2) {
      printf("_ineptrefdec() Error: Source and Destination on Same Channel. Abort!\n");
      psg_abort(1);
   }
   double thd1 = d1.t;
   double thd2 = d2.t;
   double t1a = in.t1;
   double t1b = in.t2;
   double t2a = in.t3;
   double t2b = in.t4;

   gen_RFphase(ph1,chnl); gen_RFphase(ph2,chnl2);
   gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
   gen_RFunblank(chnl); gen_RFunblank(chnl2);

   if (!strcmp(d1.dm,"y")) {
      t1a = t1a - thd1;
      _mpseqon(d1.mps,phd1);
      delay(thd1);
      _mpseqoff(d1.mps);
      gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t1a);
   gen_RFsimPulse(in.pw1,in.pw2,ph1,ph2,0.0,0.0,chnl,chnl2);
   gen_RFphase(ph3,chnl); gen_RFphase(ph4,chnl2);

   if (!strcmp(d1.dm,"y")) {
      t1b = t1b - thd1;
      _mpseqon(d1.mps,phd1);
      delay(thd1);
      _mpseqoff(d1.mps);
      gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2, chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t1b);
   gen_RFsimPulse(in.pw3,in.pw4,ph3,ph4,0.0,0.0,chnl,chnl2);
   gen_RFphase(ph5,chnl); gen_RFphase(ph6,chnl2);

   if (!strcmp(d2.dm,"y")) {
      t2a = t2a - thd2;
      _mpseqon(d2.mps,phd2);
      delay(thd2);
      _mpseqoff(d2.mps);
      gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t2a);
   gen_RFsimPulse(in.pw1,in.pw2,ph5,ph6,0.0,0.0,chnl,chnl2);

   if (!strcmp(d2.dm,"y")) {
      t2b = t2b - thd2;
      _mpseqon(d2.mps,phd2);
      delay(thd2);
      _mpseqoff(d2.mps);
      gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t2b);
}

// ----------------------------------------------------------
// INEPT Transfer Between Two Channels - Version 2
// ----------------------------------------------------------

void _ineptdec2(GP in, int ph1, int ph2, int ph3, int ph4, MPDEC d1, 
                                          int phd1, double pwHshort)
{
   int chnl = 0;
   if (!strcmp(in.ch1,"obs")) chnl = 1;
   else if (!strcmp(in.ch1,"dec")) chnl = 2;
   else if (!strcmp(in.ch1,"dec2")) chnl = 3;
   else if (!strcmp(in.ch1,"dec3")) chnl = 4;
   else {
      printf("_ineptdec2() Error: Undefined Source Channel. Abort!\n");
      psg_abort(1);
   }

   int chnl2 = 0;
   if (!strcmp(in.ch2,"obs")) chnl2 = 1;
   else if (!strcmp(in.ch2,"dec")) chnl2 = 2;
   else if (!strcmp(in.ch2,"dec2")) chnl2 = 3;
   else if (!strcmp(in.ch2,"dec3")) chnl2 = 4;
   else {
      printf("_ineptdec2() Error: Undefined Destination Channel. Abort!\n");
      psg_abort(1);
   }

   if (chnl == chnl2) {
      printf("_ineptdec2() Error: Source and Destination on Same Channel. Abort!\n");
      psg_abort(1);
   }
   double thd1 = d1.t;
   double t1a = in.t1;
   double t1b = in.t2;

   gen_RFphase(ph1,chnl); gen_RFphase(ph2,chnl2);
   gen_RFpwrf(in.a1, chnl); gen_RFpwrf(in.a2, chnl2);
   gen_RFunblank(chnl); gen_RFunblank(chnl2);

   if (!strcmp(d1.dm,"y")) {
      if (pwHshort > 0.0)  {
         if (!strcmp(d1.mps.ch,"obs")) {
            obspwrf(d1.mps.a); 
            rgpulse(pwHshort,three,0.0,0.0);
            obsunblank();
         } 
         if (!strcmp(d1.mps.ch,"dec")) {
            decpwrf(d1.mps.a);
            decrgpulse(pwHshort,three,0.0,0.0);  
            decunblank();
         } 
      }
      t1a = t1a - thd1;
      _mpseqon(d1.mps,phd1);
      delay(thd1);
      _mpseqoff(d1.mps);

      if (pwHshort > 0.0)  {
         if (!strcmp(d1.mps.ch,"obs")) {
            obspwrf(d1.mps.a);
            rgpulse(pwHshort,one,0.0,0.0);
            obsunblank();
         }
         if (!strcmp(d1.mps.ch,"dec")) {
            decpwrf(d1.mps.a);
            decrgpulse(pwHshort,one,0.0,0.0);
            decunblank();
         } 
      }
      gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t1a);
   gen_RFsimPulse(in.pw1,in.pw2,ph1,ph2,0.0,0.0,chnl,chnl2);
   gen_RFphase(ph3,chnl); gen_RFphase(ph4,chnl2);

   if (!strcmp(d1.dm,"y")) {      
      if (pwHshort > 0.0) {
         if (!strcmp(d1.mps.ch,"obs")) {
            obspwrf(d1.mps.a);
            rgpulse(pwHshort,three,0.0,0.0); 
            obsunblank();
         }
         if (!strcmp(d1.mps.ch,"dec")) {
            decpwrf(d1.mps.a); decphase(three);
            decrgpulse(pwHshort,three,0.0,0.0);
            decunblank();
         }
      }
      t1b = t1b - thd1;
      _mpseqon(d1.mps,phd1);
      delay(thd1);
      _mpseqoff(d1.mps);

      if (pwHshort > 0.0)  {
         if (!strcmp(d1.mps.ch,"obs")) {
            obspwrf(d1.mps.a); txphase(one);
            rgpulse(pwHshort,one,0.0,0.0);
            obsunblank();
         } 
         if (!strcmp(d1.mps.ch,"dec")) {
            decpwrf(d1.mps.a); decphase(one);
            decrgpulse(pwHshort,one,0.0,0.0);
            decunblank();
         }
      }
      gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t1b);
   gen_RFsimPulse(in.pw3,in.pw4,ph3,ph4,0.0,0.0,chnl,chnl2);
}

//-----------------------------------------------------------------
// INEPT Transfer Between Two Channels with Multiplet Refocussing
//-----------------------------------------------------------------

void _ineptrefdec2(GP in, int ph1, int ph2, int ph3, int ph4, int ph5,
                      int ph6, MPDEC d1, int phd1, MPDEC d2, int phd2, 
                                   double pwHshort1, double pwHshort2)
{
   int chnl = 0;
   if (!strcmp(in.ch1,"obs")) chnl = 1;
   else if (!strcmp(in.ch1,"dec")) chnl = 2;
   else if (!strcmp(in.ch1,"dec2")) chnl = 3;
   else if (!strcmp(in.ch1,"dec3")) chnl = 4;
   else {
      printf("_ineptrefdec2() Error: Undefined Source Channel. Abort!\n");
      psg_abort(1);
   }

   int chnl2 = 0;
   if (!strcmp(in.ch2,"obs")) chnl2 = 1;
   else if (!strcmp(in.ch2,"dec")) chnl2 = 2;
   else if (!strcmp(in.ch2,"dec2")) chnl2 = 3;
   else if (!strcmp(in.ch2,"dec3")) chnl2 = 4;
   else {
      printf("_ineptrefdec2() Error: Undefined Destination Channel. Abort!\n");
      psg_abort(1);
   }

   if (chnl == chnl2) {
      printf("_ineptrefdec2() Error: Source and Destination on Same Channel. Abort!\n");
      psg_abort(1);
   }
   double thd1 = d1.t;
   double thd2 = d2.t;
   double t1a = in.t1;
   double t1b = in.t2;
   double t2a = in.t3;
   double t2b = in.t4;

   gen_RFphase(ph1,chnl); gen_RFphase(ph2,chnl2);
   gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
   gen_RFunblank(chnl); gen_RFunblank(chnl2);

   if(!strcmp(d1.dm,"y")) {
      if (pwHshort1 > 0.0) {
         if (!strcmp(d1.mps.ch,"obs")) {
            obspwrf(d1.mps.a);
            rgpulse(pwHshort1,three,0.0,0.0);
            obsunblank();
         }
         if (!strcmp(d1.mps.ch,"dec")) {
            decpwrf(d1.mps.a); decphase(three);
            decrgpulse(pwHshort1,three,0.0,0.0);
            decunblank();
         }
      }
      t1a = t1a - thd1;
      _mpseqon(d1.mps,phd1);
      delay(thd1);
      _mpseqoff(d1.mps);

      if (pwHshort1 > 0.0)  {
         if (!strcmp(d1.mps.ch,"obs")) {
            obspwrf(d1.mps.a);
            rgpulse(pwHshort1,one,0.0,0.0);  
            obsunblank();
         }
         if (!strcmp(d1.mps.ch,"dec")) {
            decpwrf(d1.mps.a);
            decrgpulse(pwHshort1,one,0.0,0.0);  
            decunblank();
         } 
      }
      gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t1a);
   gen_RFsimPulse(in.pw1,in.pw2,ph1,ph2,0.0,0.0,chnl,chnl2);
   gen_RFphase(ph3,chnl); gen_RFphase(ph4,chnl2);

   if (!strcmp(d1.dm,"y")) {
      if (pwHshort1 > 0.0) {
         if (!strcmp(d1.mps.ch,"obs")) {
            obspwrf(d1.mps.a);
            rgpulse(pwHshort1,three,0.0,0.0);
            obsunblank();
         }
         if (!strcmp(d1.mps.ch,"dec")) {
            decpwrf(d1.mps.a);
            decrgpulse(pwHshort1,three,0.0,0.0);
            decunblank();
         }
      }
      t1b = t1b - thd1;
      _mpseqon(d1.mps,phd1);
      delay(thd1);
      _mpseqoff(d1.mps);

      if (pwHshort1 > 0.0)  {
         if (!strcmp(d1.mps.ch,"obs")) {
            obspwrf(d1.mps.a);
            rgpulse(pwHshort1,one,0.0,0.0);  
            obsunblank();
         } 
         if (!strcmp(d1.mps.ch,"dec")) {
            decpwrf(d1.mps.a);
            decrgpulse(pwHshort1,one,0.0,0.0);  
            decunblank();
         } 
      }
      gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t1b);
   gen_RFsimPulse(in.pw3, in.pw4, ph3, ph4, 0.0, 0.0, chnl, chnl2);
   gen_RFphase(ph5, chnl); gen_RFphase(ph6, chnl2);

   if (!strcmp(d2.dm,"y")) {
      if (pwHshort2 > 0.0) {
         if (!strcmp(d2.mps.ch,"obs")) {
            obspwrf(d2.mps.a);
            rgpulse(pwHshort2,three,0.0,0.0);  
            obsunblank();
         } 
          if (!strcmp(d2.mps.ch,"dec")) {
          decpwrf(d2.mps.a); decphase(three);  // magic angle pulse
          decrgpulse(pwHshort2,three,0.0,0.0);  
          obsunblank(); decunblank(); dec2unblank();
          } 
       }

      t2a = t2a - thd2;
      _mpseqon(d2.mps,phd2);
      delay(thd2);
      _mpseqoff(d2.mps);

      if (pwHshort2 > 0.0) {
         if (!strcmp(d2.mps.ch,"obs")) {
            obspwrf(d2.mps.a);
            rgpulse(pwHshort2,one,0.0,0.0);  
            obsunblank();
         } 
         if (!strcmp(d2.mps.ch,"dec")) {
            decpwrf(d2.mps.a); decphase(one);
            decrgpulse(pwHshort2,one,0.0,0.0);  
            decunblank();
         } 
      }
      gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t2a);
   gen_RFsimPulse(in.pw1,in.pw2,ph5,ph6,0.0,0.0,chnl,chnl2);

   if (!strcmp(d2.dm,"y")) {
      if( pwHshort2 > 0.0) {
         if (!strcmp(d2.mps.ch,"obs")) {
            obspwrf(d2.mps.a);
            rgpulse(pwHshort2,three,0.0,0.0);  
            obsunblank();
         } 
         if (!strcmp(d2.mps.ch,"dec")) {
            decpwrf(d2.mps.a);
            decrgpulse(pwHshort2,one,0.0,0.0);  
            decunblank();
         } 
      }
      t2b = t2b - thd2;
      _mpseqon(d2.mps,phd2);
      delay(thd2);
      _mpseqoff(d2.mps);

      if(pwHshort2>0)  {
         if (!strcmp(d2.mps.ch,"obs")) {
            obspwrf(d2.mps.a);
            rgpulse(pwHshort2,one,0.0,0.0);  
            obsunblank();
         } 
         if (!strcmp(d2.mps.ch,"dec")) {
            decpwrf(d2.mps.a);
            decrgpulse(pwHshort2,one,0.0,0.0);  
            decunblank();
         } 
      }
      gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t2b);
}

//--------------------------------------------
// HSQC Transfer Between Two Channels Before 
//--------------------------------------------

void _hsqcdec1(GP in, int ph1, int ph2, MPDEC d1, int phd1, 
                                          double pwHshort1)
{
   int chnl = 0;
   if (!strcmp(in.ch1,"obs")) chnl = 1;
   else if (!strcmp(in.ch1,"dec")) chnl = 2;
   else if (!strcmp(in.ch1,"dec2")) chnl = 3;
   else if (!strcmp(in.ch1,"dec3")) chnl = 4;
   else {
      printf("_hsqcdec1() Error: Undefined Source Channel. Abort!\n");
      psg_abort(1);
   }

   int chnl2 = 0;
   if (!strcmp(in.ch2,"obs")) chnl2 = 1;
   else if (!strcmp(in.ch2,"dec")) chnl2 = 2;
   else if (!strcmp(in.ch2,"dec2")) chnl2 = 3;
   else if (!strcmp(in.ch2,"dec3")) chnl2 = 4;
   else {
      printf("_hsqcdec1() Error: Undefined Destination Channel. Abort!\n");
      psg_abort(1);
   }

   if (chnl == chnl2) {
      printf("_hsqcdec1() Error: Source and Destination on Same Channel. Abort!\n");
      psg_abort(1);
   }
   double thd1 = d1.t;
   double t1a = in.t1;
   double t1b = in.t2;

   gen_RFphase(ph1,chnl); gen_RFphase(ph2,chnl2);
   gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
   gen_RFunblank(chnl); gen_RFunblank(chnl2); 

   if (!strcmp(d1.dm,"y")) {
      if (pwHshort1 > 0.0) {
         if (!strcmp(d1.mps.ch,"obs")) {
            obspwrf(d1.mps.a);
            rgpulse(pwHshort1,three,0.0,0.0);  
            obsunblank();
         } 
         if (!strcmp(d1.mps.ch,"dec")) {
            decpwrf(d1.mps.a);
            decrgpulse(pwHshort1,three,0.0,0.0);  
            decunblank();
         } 
      }
      t1a = t1a - thd1;
      _mpseqon(d1.mps,phd1);
      delay(thd1);
      _mpseqoff(d1.mps);

      if (pwHshort1 > 0.0) {
         if (!strcmp(d1.mps.ch,"obs")) {
            obspwrf(d1.mps.a);
            rgpulse(pwHshort1,one,0.0,0.0);
            obsunblank();
         } 
         if (!strcmp(d1.mps.ch,"dec")) {
            decpwrf(d1.mps.a);
            decrgpulse(pwHshort1,one,0.0,0.0);  
            decunblank();
         } 
      }
      gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t1a);
   gen_RFsimPulse(in.pw1, in.pw2, ph1, ph2, 0.0, 0.0, chnl, chnl2);

   if (!strcmp(d1.dm,"y")) {
      if (pwHshort1 > 0.0) {
         if (!strcmp(d1.mps.ch,"obs")) {
            obspwrf(d1.mps.a);
            rgpulse(pwHshort1,three,0.0,0.0);  
            obsunblank();
         } 
         if (!strcmp(d1.mps.ch,"dec")) {
            decpwrf(d1.mps.a);
            decrgpulse(pwHshort1,three,0.0,0.0);  
            decunblank();
         } 
      }
      t1b = t1b - thd1;
      _mpseqon(d1.mps,phd1);
      delay(thd1);
      _mpseqoff(d1.mps);

      if (pwHshort1 > 0.0) {
         if (!strcmp(d1.mps.ch,"obs")) {
            obspwrf(d1.mps.a);
            rgpulse(pwHshort1,one,0.0,0.0);  
            obsunblank();
         } 
         if (!strcmp(d1.mps.ch,"dec")) {
            decpwrf(d1.mps.a);
            decrgpulse(pwHshort1,one,0.0,0.0);  
            decunblank();
         } 
      }
      gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t1b); 
}

//-------------------------------------------
// HSQC Transfer Between Two Channels After
//-------------------------------------------

void _hsqcdec2(GP in, int ph3, int ph4, int ph5, int ph6, MPDEC d2, 
                                        int phd2, double pwHshort2)
{
   int chnl = 0;
   if (!strcmp(in.ch1,"obs")) chnl = 1;
   else if (!strcmp(in.ch1,"dec")) chnl = 2;
   else if (!strcmp(in.ch1,"dec2")) chnl = 3;
   else if (!strcmp(in.ch1,"dec3")) chnl = 4;
   else {
      printf("_hsqcdec2() Error: Undefined Source Channel. Abort!\n");
      psg_abort(1);
   }

   int chnl2 = 0;
   if (!strcmp(in.ch2,"obs")) chnl2 = 1;
   else if (!strcmp(in.ch2,"dec")) chnl2 = 2;
   else if (!strcmp(in.ch2,"dec2")) chnl2 = 3;
   else if (!strcmp(in.ch2,"dec3")) chnl2 = 4;
   else {
      printf("_hsqcdec2() Error: Undefined Destination Channel. Abort!\n");
      psg_abort(1);
   }

   if (chnl == chnl2) {
      printf("_hsqcdec2() Error: Source and Destination on Same Channel. Abort!\n");
      psg_abort(1);
   }
   double thd2 = d2.t;
   double t2a = in.t3;
   double t2b = in.t4;
   gen_RFsimPulse(in.pw3,in.pw4,ph3,ph4,0.0,0.0,chnl,chnl2);
   gen_RFphase(ph5,chnl); gen_RFphase(ph6,chnl2);

   if (!strcmp(d2.dm,"y")) {
      if (pwHshort2 > 0.0) {
         if (!strcmp(d2.mps.ch,"obs")) {
            obspwrf(d2.mps.a);
            rgpulse(pwHshort2,three,0.0,0.0);  
            obsunblank();
         } 
         if (!strcmp(d2.mps.ch,"dec")) {
            decpwrf(d2.mps.a);
            decrgpulse(pwHshort2,three,0.0,0.0);  
            decunblank();
         } 
      }
      t2a = t2a - thd2;
      _mpseqon(d2.mps,phd2);
      delay(thd2);
      _mpseqoff(d2.mps);

      if (pwHshort2 > 0.0) {
         if (!strcmp(d2.mps.ch,"obs")) {
            obspwrf(d2.mps.a);
            rgpulse(pwHshort2,one,0.0,0.0);  
            obsunblank();
         } 
         if (!strcmp(d2.mps.ch,"dec")) {
            decpwrf(d2.mps.a);
            decrgpulse(pwHshort2,one,0.0,0.0);  
            decunblank();
         } 
      }
      gen_RFpwrf(in.a1, chnl); gen_RFpwrf(in.a2, chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t2a);
   gen_RFsimPulse(in.pw1,in.pw2,ph5,ph6,0.0,0.0,chnl,chnl2);

   if (!strcmp(d2.dm,"y")) {
      if (pwHshort2 > 0.0) {
         if (!strcmp(d2.mps.ch,"obs")) {
            obspwrf(d2.mps.a);
            rgpulse(pwHshort2,three,0.0,0.0);  
            obsunblank();
         } 
         if (!strcmp(d2.mps.ch,"dec")) {
            decpwrf(d2.mps.a);
            decrgpulse(pwHshort2,one,0.0,0.0);  
            decunblank();
         } 
      }
      t2b = t2b - thd2;
      _mpseqon(d2.mps,phd2);
      delay(thd2);
      _mpseqoff(d2.mps);

      if (pwHshort2 > 0.0) {
         if (!strcmp(d2.mps.ch,"obs")) {
          obspwrf(d2.mps.a);
          rgpulse(pwHshort2,one,0.0,0.0);
          decunblank();
         }
         if (!strcmp(d2.mps.ch,"dec")) {
          decpwrf(d2.mps.a);
          decrgpulse(pwHshort2,one,0.0,0.0);
          decunblank();
         }
      }
      gen_RFpwrf(in.a1,chnl); gen_RFpwrf(in.a2,chnl2);
      gen_RFunblank(chnl); gen_RFunblank(chnl2);
   }
   delay(t2b);
}

//============================
// Additional MPSEQ Waveforms
//============================

//==============================
// Build PMLG With a Supercycle
//==============================

MPSEQ getpmlgsuper(char *seqName, int iph ,double p, double phint, int iRec, int calc)
{
   MPSEQ pm;
   int i,j,k;
   char *var;
   extern MPSEQ MPchopper(MPSEQ pm);

   if (strlen(seqName) >= NSUFFIX  || strlen(seqName) < 1) {
      printf("Error in getpmlgsuper(). The type name %s is invalid!\n",seqName);
      psg_abort(1);
   }
   strcpy(pm.seqName,seqName);
   pm.calc = calc;
   pm.array = parsearry(pm.array);

// Obtain Phase Arguments

   pm.phAccum = p;
   pm.iSuper = iph;
   pm.nRec = iRec;
   pm.phInt = phint;

// Determine nphBase form qXpmlg

// qXpmlg

   var = getname0("q",pm.seqName,"");
   int nphBase = (int) getval(var);
   pm.array = disarry(var, pm.array);

   double sign = 1;
   if (nphBase < 0.0) {
      nphBase = -nphBase;
      sign = - sign;
   }
   int nsteps = nphBase;

// mXpmlg

   var = getname0("m",pm.seqName,"");
   int mPMLG = (int) getval(var);
   pm.array = disarry(var, pm.array);

   int ccw = 0;
   if (mPMLG < 0.0) {
      ccw = 2*nphBase - 1;
   }

// nXpmlg 

   var = getname0("n",pm.seqName,"");
   pm.nelem = (int) getval(var);
   pm.array = disarry(var, pm.array);

// Allocate Arrays

   j = 2;
   if (pm.nelem%2 == 0) {
      j = 2*j;
      pm.nelem = pm.nelem/2;
   }
   nphBase = j*nphBase;
   int nphSuper = 1;
   int npw = nphBase;
   int nph = nphBase;
   int nof = 1;
   int na = 1;
   int ng = 1;
   MPinitializer(&pm,npw,nph,nof,na,ng,nphBase,nphSuper);

// Set the Step Sizes

   pm.n90 = VNMRSN90;
   pm.n90m = VNMRSN90M;
   pm.trap = VNMRSTRAP;

   if (PWRF_DELAY > 0.0) {
      pm.n90 = INOVAN90;
      pm.n90m = INOVAN90M;
      pm.trap = INOVATRAP;
   }

// Set the Base Phase List

   double obsstep = 360.0/8192;
   double delta = 360.0/(sqrt(3)*nsteps);
   for (i = 0; i < pm.nphBase/j; i++) {
      k = ccw + mPMLG*i;
      pm.phBase[k] = sign*(i*delta + delta/2.0);
      pm.phBase[k] = roundphase(pm.phBase[k],obsstep);
   }
   for (i = pm.nphBase/j; i < 2*pm.nphBase/j; i++) {
      k = ccw + mPMLG*i;
      pm.phBase[k] = 180.0 + sign*((2*nsteps - i)*delta - delta/2.0);
      pm.phBase[k] = roundphase(pm.phBase[k],obsstep);
   }
   if ( j%4 == 0 ) {
      for (i = 0; i < pm.nphBase/2; i++) pm.phBase[nphBase - 1 - i] = pm.phBase[i];
   }

// Set the Supercycle Phase List

   pm.phSuper[0] = 0.0;

// Set the Delay List

//pwXpmlg (360 pulse)

   var = getname0("pw",pm.seqName,"");
   double pw360 = getval(var);
   pw360 = roundoff(pw360,12.5e-9*pm.n90);
   pm.array = disarry(var, pm.array);

   double pwlast = 0.0;
   for (i = 0; i < pm.nphBase/j; i++) {
      pm.pw[i] = roundoff((i+1)*pw360*j/pm.nphBase,12.5e-9*pm.n90) - pwlast;
      pwlast = pwlast + pm.pw[i];
      pm.pw[i + nsteps] = pm.pw[i];
   }
   if (j%4 == 0 ) {
      for (i = 0; i < 2*pm.nphBase/j; i++) pm.pw[nphBase/2 + i] = pm.pw[i];
   }

// Set the Offset List

//ofXpmlg

   var = getname0("of",pm.seqName,"");
   pm.of[0] = getval(var);
   pm.array = disarry(var, pm.array);

// Set the Amp and Gate Lists

   pm.aBase[0] = 1023.0;
   pm.gateBase[0] = 0.0;

// Set the Overall Amplitude, Elements and Channel

//aXpmlg

   var = getname0("a",pm.seqName,"");
   pm.a = getval(var);

//chXpmlg
   
   var = getname0("ch",pm.seqName,"");
   Getstr(var,pm.ch,sizeof(pm.ch));

// Create the Shapefile Name

   char lpattern[NPATTERN];
   var = getname0("",pm.seqName,"");
   sprintf(lpattern,"%s%d",var,pm.nRec);
   pm.hasArray = hasarry(pm.array, lpattern);
   int lix = arryindex(pm.array);
   if (pm.calc > 0) {
      var = getname0("",pm.seqName,"");
      sprintf(pm.pattern,"%s%d_%d",var,pm.nRec,lix);
      if (pm.hasArray == 1) {
         pm = MPchopper(pm); 
         pm.iSuper = iph + pm.nelem%pm.nphSuper;
      }
      pm.t = gett(lix, lpattern);
   }
   return pm;
}





