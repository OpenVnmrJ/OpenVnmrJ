/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
// Original due to D.H.Zhou and C. Reinstra UIUC
// Modified and extended by ceb 2/1/05 -> present 
// Modified and extended by dmr 10/14/05 -> present 

// Contents:
// getr1462() - MPSEQ
// getr1825() - MPSEQ
// getspc5() - MPSEQ
// getpostc7() - MPSEQ
// getfslg1() - MPSEQ1
// getblew12() - MPSEQ
// getdumbo() - MPSEQ
// getbaba() - WMPSEQ
// getxy8() - WMPSEQ
// getcp() - CP
// gettppm() - TPPM
// getspinal() - SPINAL

//======================================
//  Declare common character arrays
//======================================

#define NUP 3
#define NLOW 3
#define NSUFFIX 13
#define NVAR 21

char up[NUP],low[NLOW],suffix[NSUFFIX],var[NVAR],varName[MAXSTR];

//=======================================
// Define sizes of structure members
//=======================================

#define NTICK_CTR 8
#define NPW 32768
#define NCH 5
#define NPH_SUPER 32
#define NPATTERN 32
#define NTAU    32

//=================================================================================
//  	    	 UTILITY FUNCTIONS
//=================================================================================

double roundoff(double par, double res)
{
   return( ((int) (par/res + 0.5)) * res);
}

double roundphase(double par, double res)
{ 
   double parval = par;    
   while (parval >= 360.0) parval = parval - 360.0;
   while (parval < 0.0) parval = parval + 360.0; 
   parval = ((int) (parval/res + 0.5)) * res;
   return(parval);
}  

double roundamp(double par, double res)
{ 
   double parval = par;    
   while (parval > 1023) parval = 1023;
   while (parval < 0.0) parval = 0.0; 
   parval = ((int) (parval/res + 0.5)) * res;
   return(parval);
}    

//================================================================================
//   sizeof(parval) called in subroutines always returns 4 for the pointer,
//   instead of returning the character array size; so provide it
//   as the 3rd argument. This Getstr() is supposed to be called only by getcp and
//   other initilization functions in this head file. 
//=================================================================================

void Getstr(char *parname, char *parval, int arrsize)
{
   char str[MAXSTR];  // getstr() always returns an char array of size MAXSTR 
   getstr(parname,str); 
   strncpy(parval,str,arrsize);
   if (strlen(str) >= arrsize) *(parval+arrsize-1) = '\0';
}

//===============================================================================
//  	    	    	DECLARE STRUCTURES
//===============================================================================

//===================================================
//   MPSEQ: arrayed phase only 
//===================================================

typedef struct {
   char   seqName[NSUFFIX];       //Xc7,Yspc5 etc...
   int    nelem;    	    	  //number of base elements to output
   int    n[NPW];     	          //clock ticks consumed by the pulses
   int    n90;		          //clock ticks consumed in shortest pulse
   double pw[NPW];		  //pulse widths
   double a;	    	    	  //rf amplitude
   double t;	    	    	  //duration of the sequence
   double phInt;      	          //internal phase accumulation bookkeeping
   double phAccum;                //phase accumulated due to offset
   double of; 	    	    	  //frequency
   char   ch[NCH];     	          //output channel
   char   pattern[NPATTERN];	  //shape file name
   double telem;   	    	  //duration of base element
   int    iSuper;	    	  //internal phase shift book keeping 
   double phSuper[NPH_SUPER];	  //Super cycle, e.g. {0,360/7,2*360/7 ...}
   int    nphSuper;		  //size of super cycle for specific sequence
   double phBase[NPW];            //base phase cycle, eg {0,180,0}			
   int    nphBase;                //number of elements in the base phase list
   int    nRec;                   //copy number
} MPSEQ;

//===================================================
//   WMPSEQ: arrayed phase and gate 
//===================================================

typedef struct {
   char   seqName[NSUFFIX];       //Xc7,Yspc5 etc...
   int    nelem;    	    	  //number of base elements to output
   int    n[NPW];     	          //clock ticks consumed by the pulses
   int    n90;		          //clock ticks consumed in shortest pulse
   double pw[NPW];		  //pulse widths
   double a;	    	    	  //rf amplitude
   double t;	    	    	  //duration of the sequence
   double phInt;      	          //internal phase accumulation bookkeeping
   double phAccum;                //phase accumulated due to offset
   double of; 	    	    	  //frequency
   char   ch[NCH];     	          //output channel
   char   pattern[NPATTERN];	  //shape file name
   double telem;   	    	  //duration of base element
   int    iSuper;	    	  //internal phase shift book keeping 
   double phSuper[NPH_SUPER];	  //Super cycle, e.g. {0,360/7,2*360/7 ...}
   int    nphSuper;		  //size of super cycle for specific sequence
   double gateBase[NPW];          //base gate pattern eg {1,0,1,1,0,1,1...}
   double phBase[NPW];            //base phase cycle, eg {0,180,0....}			
   int    nphBase;                //number of elements in the base phase list
   int    nRec;                   //copy number
} WMPSEQ;

//===================================================
//   MPSEQ1: arrayed phase and offset
//===================================================

typedef struct {
   char   seqName[NSUFFIX];       //Xc7,Yspc5 etc...
   int    nelem;    	    	  //number of base elements to output
   int    n[NPW];     	          //clock ticks consumed by the pulses
   int    n90;		          //clock ticks consumed in shortest pulse
   double pw[NPW];		  //pulse widths
   double a;	    	    	  //rf amplitude
   double t;	    	    	  //duration of the sequence
   double phInt;      	          //internal phase accumulation bookkeeping
   double phAccum;                //phase accumulated due to offset
   double of[NPW]; 	    	  //frequency
   char   ch[NCH];     	          //output channel
   char   pattern[NPATTERN];	  //shape file name
   double telem;   	    	  //duration of base element
   int    iSuper;	    	  //internal phase shift book keeping 
   double phSuper[NPH_SUPER];	  //Super cycle, e.g. {0,360/7,2*360/7 ...}
   int    nphSuper;		  //size of super cycle for specific sequence
   double phBase[NPW];            //base phase cycle, eg {0,180,0}			
   int    nphBase;                //number of elements in the base phase list
   int    nRec;                   //copy number
} MPSEQ1;

//===================================================
//   AWMPSEQ1: arrayed gate, amplitude, phase and offset
//===================================================

typedef struct {
   char   seqName[NSUFFIX];       //Xc7,Yspc5 etc...
   int    nelem;    	    	  //number of base elements to output
   int    n[NPW];     	          //clock ticks consumed by the pulses
   int    n90;		          //clock ticks consumed in shortest pulse
   double pw[NPW];		  //pulse widths
   double a;                      //scaled rf amplitude
   double aBase[NPW];	    	  //rf amplitude array
   double t;	    	    	  //duration of the sequence
   double phInt;      	          //internal phase accumulation bookkeeping
   double phAccum;                //phase accumulated due to offset
   double of[NPW]; 	    	  //frequency
   char   ch[NCH];     	          //output channel
   char   pattern[NPATTERN];	  //shape file name
   double telem;   	    	  //duration of base element
   int    iSuper;	    	  //internal phase shift book keeping 
   double phSuper[NPH_SUPER];	  //Super cycle, e.g. {0,360/7,2*360/7 ...}
   int    nphSuper;		  //size of super cycle for specific sequence
   double gateBase[NPW];          //base gate pattern eg {1,0,1,1,0,1,1...}
   double phBase[NPW];            //base phase cycle, eg {0,180,0}			
   int    nphBase;                //number of elements in the base phase list
   int    nRec;                   //copy number
} AWMPSEQ1;

//========
//  CP 
//========

typedef struct {
   double a1;         // aHhx fine power of the source channel; offset value if ramped
   double a2;         // aXhx fine power of the destination channel; offset if ramped
   double d;          // dHX  delta parameter of tangent shape -d to +d
   double b;          // bHX  beta parameter of tangent shape 
   double t;          // tHX  contact time in sec after initcp() or getval()
   double of;         // ofHX offset frequency 
   char  sh[2];       // shHX shape of cp, "c", "l", "t" 
   char  ch[3];       // chHX ramp channel "fr" or "to"
   char  fr[5];       // frHX--the source channel "obs", "dec", "dec2", "dec3"
   char  to[5];       // toHX--the destination channel "obs", "dec", "dec2", "dec3"              
   char  pattern[32]; // shape file name 
   int   n90;         // clock ticks in min time interval
   int   n;           // clock ticks in pulse
   double phInt;      // internal phase bookkeeping
   double phAccum;    // phase accumulated during the shape
   int    nRec;       // copy number 
} CP;

//========
//  TPPM
//======== 

typedef struct {
   double t; 	      
   double a;  
   double pw; 
   double ph;
   double of; 
   char ch[5]; 
   char pattern[32];
} TPPM;

//========
//  Spinal
//======== 

typedef struct {
   double t;
   double a; 
   double pw; 
   double ph; 
   double of;
   char ch[5]; 
   char pattern[32];
} SPINAL;

//==========================================
//            ROUTERS
//==========================================

//===========
// dseq
//===========

// dseq manages the various decoupling structures 

typedef struct {
   TPPM t;
   SPINAL s;
   char name[10];
   char seq[14];
} DSEQ;

//====================================================================
//  	    	   FUNCTIONS TO  POPULATE STRUCTURES
//====================================================================
//
//      char* getname(char *varType, char *varSuffix, char *chXtra)
//
//=====================================================================
//
//  varType denotes the variable to be constructed, pw,a,sc etc
//
//  varSuffix is assumed to be of the form abcDEghi where
//          abc is normally used to indicate sequence name, eg c7,cp
//          DE indicates channel(s) involved in the sequence
//          ghi is a descriptor, eg mix,evolve
//

//  chXtra: is an optional explicit channel identifier. It overides
//          any captals that may be in  varType
//
//   if chXtra is NULL, the function returns the concatentation
//      (varType)(DE)(abcghi)
//
//  if chXtra is not NULL, the function returns the concatenation
//      (varType)(chXtra)(abcdeghi)
//
//
//   
//=====================================================================

char* getname(char *varType, char *varSuffix, char *chXtra)
{    
   int i,chCount, descrCount, typeCount, capCount;
   extern char varName[];
   char chDescr[NSUFFIX];
   char chSeqType[NSUFFIX];
   char chUP[NUP], chLOW[NUP];
    
//
// Break up the seqType into leading lowercase characters
// which may contain the sequence identifier, uppercase
// chars which may contain a channel identifier
// and trailing lowercase chars which may contain a descriptor
//
// Will fail if the caps section contains digits
    
   chCount = 0; descrCount = 0;typeCount = 0; capCount = 0;
   for (i = 0; i< strlen(varSuffix); i++){
      if (islower(varSuffix[i]) || isdigit(varSuffix[i])){
         if (capCount == 0) chSeqType[typeCount++] = varSuffix[i];
         else chDescr[descrCount++] = varSuffix[i];
      }
      else {
         chUP[capCount] = varSuffix[i];
         chLOW[capCount++] = (char)tolower(varSuffix[i]); 
      }
   }
   varName[0] = '\0';
   chSeqType[typeCount] = '\0';
   chDescr[descrCount] = '\0';
   chUP[capCount] = '\0';
   chLOW[capCount] = '\0';
   
//
//  if chXtra is NULL, then we must take the channel ID from varSuffix
//  
           
   if (strlen(chXtra) == 0){
      sprintf(varName,"%s%s%s%s",varType,chUP,chSeqType,chDescr);
      return varName;
   }    
   sprintf(varName,"%s%s%s%s%s",varType,chXtra,chSeqType,chLOW,chDescr);
   return varName; 
}

//========================================
//Implement Waveforms with "get" functions
//========================================

//=======================
// Build R_14_2_6
//=======================

MPSEQ getr1426(char *seqName, int iph, double p, double phint, int iRec ) 
{
   int logic = 0;
   MPSEQ r; 
   extern MPSEQ MPchopper(MPSEQ r);
   char *var;   
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1){
        printf("Error in makeMPSEQ! The type name %s is invalid !\n",seqName);
        exit(-1);
   } 
   sprintf(r.seqName,seqName); 
   logic = (ix==1); 
      
   r.nphBase = 2;
   r.phBase[0] = 0.0;
   r.phBase[1] = 180.0;
 
   r.nphSuper = 2;
   r.phSuper[0] = 77.143;
   r.phSuper[1] = 282.857;
 
   r.phAccum = p;
   r.iSuper = iph;
   r.nRec = iRec;
   r.phInt = phint;
                                             
//qXr1426

   var = getname("q",r.seqName,"");
   r.nelem = getval(var);
   logic = (logic || isarry(var)); 
 
//pwXr1426 

   var = getname("pw",r.seqName,""); 
   r.pw[0] = getval(var); 
   r.pw[0] = 0.25*r.pw[0];                  
   r.pw[1] = 3.0*r.pw[0];                  
   logic = (logic || isarry(var));
   
//ofXr1426

   var = getname("of",r.seqName,"");
   r.of = getval(var);  
   logic = (logic || isarry(var));
 
//chXr1426

   Getstr(getname("ch",r.seqName,""),r.ch,sizeof(r.ch));
  
//aXr1426

   var = getname("a",r.seqName,"");       
   r.a = getval(var);
      
// create shapefile name
   
   if (logic) {
      sprintf(r.pattern,"%s%d_%ld",getname("",r.seqName,""),r.nRec,ix); 
   }
   else {
      sprintf(r.pattern,"%s%d_1",getname("",r.seqName,""),r.nRec); 
   }   
   r = MPchopper(r);
   r.iSuper = iph + r.nelem%2;     
   return r;
}

//=======================
// Build R_18_2_5
//=======================

MPSEQ getr1825(char *seqName, int iph, double p, double phint, int iRec ) 
{
   int logic = 0;
   MPSEQ r; 
   extern MPSEQ MPchopper(MPSEQ r);
   char *var;   
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1){
        printf("Error in makeMPSEQ! The type name %s is invalid !\n",seqName);
        exit(-1);
   } 
   sprintf(r.seqName,seqName); 
   logic = (ix==1); 
      
   r.nphBase = 1;
   r.phBase[0] = 0.0;
 
   r.nphSuper = 2;
   r.phSuper[0] = 50.0;
   r.phSuper[1] = 310.0;
 
   r.phAccum = p;
   r.iSuper = iph;
   r.nRec = iRec;
   r.phInt = phint;
                                             
//qXr1825

   var = getname("q",r.seqName,"");
   r.nelem = getval(var);
   logic = (logic || isarry(var)); 
 
//pwXr1825 

   var = getname("pw",r.seqName,""); 
   r.pw[0] = getval(var); 
   r.pw[0] = 0.5*r.pw[0];                 
   logic = (logic || isarry(var));
   
//ofXr1825

   var = getname("of",r.seqName,"");
   r.of = getval(var);  
   logic = (logic || isarry(var));
 
//chXr1825

   Getstr(getname("ch",r.seqName,""),r.ch,sizeof(r.ch));
  
//aXr1825

   var = getname("a",r.seqName,"");       
   r.a = getval(var);
      
// create shapefile name
   
   if (logic) {
      sprintf(r.pattern,"%s%d_%ld",getname("",r.seqName,""),r.nRec,ix); 
   }
   else {
      sprintf(r.pattern,"%s%d_1",getname("",r.seqName,""),r.nRec); 
   }   
   r = MPchopper(r);
   r.iSuper = iph + r.nelem%2;     
   return r;
}

//=============
//  Build SPC5
//=============

MPSEQ getspc5(char *seqName, int iph, double p, double phint, int iRec ) 
{
   int logic = 0;
   MPSEQ spc5; 
   extern MPSEQ MPchopper(MPSEQ spc5);
   char *var;
   
    if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1){
        printf("Error in makeMPSEQ! The type name %s is invalid !\n",seqName);
        exit(-1);
   }   
   sprintf(spc5.seqName,seqName);  
   logic = (ix==1);	
                                 
   spc5.nphBase = 3;
   spc5.phBase[0] = 0.0;
   spc5.phBase[1] = 180.0;
   spc5.phBase[2] = 0.0;
 
   spc5.nphSuper = 10;
   spc5.phSuper[0] = 0.0;
   spc5.phSuper[1] = 72.0;
   spc5.phSuper[2] = 144.0;
   spc5.phSuper[3] = 216.0;
   spc5.phSuper[4] = 288.0;
   spc5.phSuper[5] = 180.0;
   spc5.phSuper[6] = 252.0;
   spc5.phSuper[7] = 324.0;
   spc5.phSuper[8] = 36.0;
   spc5.phSuper[9] = 108.0;
 
   spc5.phAccum = p;
   spc5.iSuper = iph;
   spc5.nRec = iRec;
   spc5.phInt = phint;
             
//qXspc5

   var = getname("q",spc5.seqName,"");
   spc5.nelem = getval(var);
   logic = (logic || isarry(var));
 
//pwXspc5  
 
   var = getname("pw",spc5.seqName,"");
   spc5.pw[1] = getval(var); 
   logic = (logic || isarry(var));
   spc5.pw[0] = 0.25*spc5.pw[1];
   spc5.pw[2] = 0.75*spc5.pw[1];
   
//ofXspc5
 
   var = getname("of",spc5.seqName,"");
   spc5.of = getval(var);  
   logic = (logic || isarry(var));
   
//chXspc5
 
   Getstr(getname("ch",spc5.seqName,""),spc5.ch,sizeof(spc5.ch)); 
   
//aXspc5
 
   var = getname("a",spc5.seqName,"");       
   spc5.a = getval(var);
     
// create shapefile name
   
   if (logic) {
    sprintf(spc5.pattern,"%s%d_%ld",getname("",spc5.seqName,""),spc5.nRec,ix); 
   }
   else {
    sprintf(spc5.pattern,"%s%d_1",getname("",spc5.seqName,""),spc5.nRec); 
   }
   spc5 = MPchopper(spc5); 
   spc5.iSuper = iph +  spc5.nelem%5;    
   return spc5; 
}

//=============================================
// Build POSTC7
//=============================================

MPSEQ getpostc7(char *seqName, int iph, double p, double phint, int iRec) 
{
   int logic = 0;
   MPSEQ c7;
   char *var;
   extern MPSEQ MPchopper(MPSEQ c7);
   int i;
 
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1){
        printf("Error in makeMPSEQ! The type name %s is invalid !\n",seqName);
        exit(-1);
   }  
   sprintf(c7.seqName,seqName);  
   logic = (ix==1);
   
   c7.phInt = phint;
   c7.phAccum = p;
   c7.iSuper = iph;

   c7.nphBase = 3;
   c7.phBase[0] = 0.0;
   c7.phBase[1] = 180.0;
   c7.phBase[2] = 0.0;

   double dph = 360.0/7.0;
   c7.nphSuper = 7;
   c7.phSuper[0] = 0;
   for( i = 1; i< c7.nphSuper; i++)
      c7.phSuper[i] = c7.phSuper[i-1] + dph;

   c7.phAccum = p;
   c7.iSuper = iph;
   c7.nRec = iRec;
   c7.phInt = phint;

// Construct parameter names and get values

 
//qXc7

   var = getname("q",c7.seqName,"");
   c7.nelem = getval(var);
   logic = (logic || isarry(var));
 
//pwXc7
 
   var = getname("pw",c7.seqName,"");
   c7.pw[1] = getval(var);
   logic = (logic || isarry(var));
   c7.pw[0] = 0.25*c7.pw[1];
   c7.pw[2] = 0.75*c7.pw[1];                                          
 
//ofXc7

   var = getname("of",c7.seqName,"");
   c7.of = getval(var);
   logic = (logic || isarry(var));
  
//aXc7

   var = getname("a",c7.seqName,"");
   c7.a = getval(var);
  
//chXc7

   Getstr(getname("ch",c7.seqName,""),c7.ch,sizeof(c7.ch));
   
// create shapefile name
   
   if (logic) {
      sprintf(c7.pattern,"%s%d_%ld",getname("",c7.seqName,""),c7.nRec,ix); 
   }
   else {
      sprintf(c7.pattern,"%s%d_1",getname("",c7.seqName,""),c7.nRec); 
   }
   c7 = MPchopper(c7); 
   c7.iSuper = iph + c7.nelem%7; 
   return c7; 
}

//======
// FSLG
//======

MPSEQ1 getfslg1(char *seqName, int iph, double p, double phint, int iRec)
{
   int logic = 0;
   char *var;
   MPSEQ1 f;
   extern MPSEQ1 MPchopper1(MPSEQ1 f);
   
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1){
        printf("Error in makeMPSEQ1! The type name %s is invalid !\n",seqName);
        exit(-1);
   }  
   sprintf(f.seqName,seqName);
   logic = (ix==1);
      
   f.nphBase = 2;
   f.phBase[0] = 0.0;
   f.phBase[1] = 180.0;
   f.nphSuper = 1;
   f.phSuper[0] = 0.0;
   f.phAccum = p;
   f.iSuper = iph;
   f.nRec = iRec;
   f.phInt = phint; 
 
// Construct parameter names and get values

//chXfslg

   Getstr(getname("ch",f.seqName,""),f.ch,sizeof(f.ch)); 
 
//aXfslg

   var = getname("a",f.seqName,"");
   f.a = getval(var);  

//pwXfslg

   var = getname("pw",f.seqName,"");
   f.pw[0] = getval(var); 
   f.pw[1] = f.pw[0];
   logic = (logic || isarry(var));

//nXfslg

   var = getname("n",f.seqName,"");  
   f.nelem = (int) getval(var); 
   logic = (logic || isarry(var));
   
//ofXfslg

   var = getname("of",f.seqName,""); 
   double loffset = getval(var);
   logic = (logic || isarry(var));   

//ofdXfslg

   var = getname("ofd",f.seqName,""); 
   f.of[0] = getval(var);  
   f.of[1] = - f.of[0];   
   f.of[0] = f.of[0] + loffset;
   f.of[1] = f.of[1] + loffset;
   logic = (logic || isarry(var));

//write the shape file

   if (logic) {
      sprintf(f.pattern,"%s%d_%ld",getname("",f.seqName,""),f.nRec,ix); 
   }
   else {
      sprintf(f.pattern,"%s%d_1",getname("",f.seqName,""),f.nRec); 
   }   
    
   f = MPchopper1(f);       
   f.iSuper = iph + f.nelem%2;  
   return f;
}

//======================
//   PMLG
//======================

MPSEQ getpmlg(char *seqName, int iph,double p, double phint, int iRec)
{
   int logic = 0;
   MPSEQ pm; 
   int i;
   char *var; 
   extern MPSEQ MPchopper(MPSEQ pm); 
   
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1){
      printf("Error in makeMPSEQ! The type name %s is invalid !\n",seqName);
      exit(-1);
   }  
   sprintf(pm.seqName,seqName);
   
   pm.nphSuper = 1;
   pm.phSuper[0] = 0.0;
   pm.phAccum = p;
   pm.iSuper = iph;
   pm.nRec = iRec;
   pm.phInt = phint; 
   logic = (ix==1); 
   
//qXpmlg is pm.nphBase/2 

   var = getname("q",pm.seqName,"");  
   pm.nphBase = (int) getval(var);
   if (pm.nphBase>NPW/2) { 
      printf("Error in getpmlg()! Too many steps, q!"  );
      exit(-1);
   }   
   
// Set PMLG Phase Array   

   double sign = 1;
   if (pm.nphBase < 0.0) { 
      pm.nphBase = -pm.nphBase;
      sign = - sign; 
   } 
   double obsstep = 360.0/8192;      
   double delta = 360.0/(sqrt(3)*pm.nphBase);  
   double val = 0.0;
   for (i = 0; i < pm.nphBase; i++) {
      pm.phBase[i] = sign*(i*delta + delta/2.0);
      pm.phBase[i] = roundphase(pm.phBase[i],obsstep);
      val = pm.phBase[i];
   }   
   for (i = pm.nphBase; i < 2*pm.nphBase; i++) {
      pm.phBase[i] = 180.0 + sign*((2*pm.nphBase - i)*delta - delta/2.0);
      pm.phBase[i] = roundphase(pm.phBase[i],obsstep);
      val = pm.phBase[i];
   }         
   logic = (logic || isarry(var));
 
//pwXpmlg (360 pulse)
 
   var = getname("pw",pm.seqName,"");
   double pw360 = getval(var);
   pw360 = roundoff(pw360,0.1e-6);
   
// Set PMLG pulse width array   
  
   double pwlast = 0.0; 
   for (i = 0; i < pm.nphBase; i++) { 
      pm.pw[i] = roundoff((i+1)*pw360/pm.nphBase,0.1e-6) - pwlast;
      pwlast = pwlast + pm.pw[i];   
      pm.pw[i + pm.nphBase] = pm.pw[i];      
   }   
   logic = (logic || isarry(var));
   
// reset nphBase to the total

   pm.nphBase = pm.nphBase*2;  

//chXpmlg

   Getstr(getname("ch",pm.seqName,""),pm.ch,sizeof(pm.ch)); 
   
//nXpmlg 

   var = getname("n",pm.seqName,"");  
   pm.nelem = (int) getval(var); 
   logic = (logic || isarry(var));

//aXpmlg
   var = getname("a",pm.seqName,"");
   pm.a = getval(var);  
   
//ofXpmlg

   var = getname("of",pm.seqName,""); 
   pm.of = getval(var);
   logic = (logic || isarry(var)); 
   
   if (logic) {
     sprintf(pm.pattern,"%s%d_%ld",getname("",pm.seqName,""),pm.nRec,ix); 
   }
   else {
     sprintf(pm.pattern,"%s%d_1",getname("",pm.seqName,""),pm.nRec); 
   } 
   pm = MPchopper(pm); 
   return pm;      
}

//======================
//   BLEW12
//======================

MPSEQ getblew(char *seqName, int iph,double p, double phint, int iRec)
{
   int logic = 0;
   MPSEQ pm; 
   int i;
   char *var; 
   extern MPSEQ MPchopper(MPSEQ pm); 
   
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1){
      printf("Error in makeMPSEQ! The type name %s is invalid !\n",seqName);
      exit(-1);
   }  
   sprintf(pm.seqName,seqName);
   logic = (ix==1); 
   
   pm.nphSuper = 1;
   pm.phSuper[0] = 0.0;
   pm.phAccum = p;
   pm.iSuper = iph;
   pm.nRec = iRec;
   pm.phInt = phint;   
      
// Set BLEW12 Phase Array   

   pm.nphBase = 12; 
   pm.phBase[0] = 0.0;
   pm.phBase[1] = 90.0;
   pm.phBase[2] = 180.0;
   pm.phBase[3] = 90.0;
   pm.phBase[4] = 0.0;
   pm.phBase[5] = 90.0;
   pm.phBase[6] = 270.0;
   pm.phBase[7] = 180.0;
   pm.phBase[8] = 270.0;
   pm.phBase[9] =  0.0;
   pm.phBase[10] = 270.0;
   pm.phBase[11] = 180.0;   
 
//pwXblew (360 pulse)
 
   var = getname("pw",pm.seqName,"");
   double pw = getval(var);
   pw = roundoff(pw/4.0,0.1e-6);
   logic = (logic || isarry(var));
   
// Set BLEW pulse width array  

   for (i = 0; i < pm.nphBase; i++) { 
      pm.pw[i] = pw;    
   }         

//chXblew

   Getstr(getname("ch",pm.seqName,""),pm.ch,sizeof(pm.ch)); 
   
//nXblew 

   var = getname("n",pm.seqName,"");  
   pm.nelem = (int) getval(var); 
   logic = (logic || isarry(var));

//aXblew
   var = getname("a",pm.seqName,"");
   pm.a = getval(var);  
   
//ofXblew

   var = getname("of",pm.seqName,""); 
   pm.of = getval(var);
   logic = (logic || isarry(var));

   if (logic) {
     sprintf(pm.pattern,"%s%d_%ld",getname("",pm.seqName,""),pm.nRec,ix); 
   }
   else {
     sprintf(pm.pattern,"%s%d_1",getname("",pm.seqName,""),pm.nRec); 
   } 
   pm = MPchopper(pm); 
   return pm;      
}

//======================
//   DUMBO
//======================

MPSEQ getdumbo(char *seqName, int iph,double p, double phint, int iRec)
{
   int logic = 0;
   MPSEQ pm; 
   int i;
   char *var; 
   extern MPSEQ MPchopper(MPSEQ pm); 
   
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1){
      printf("Error in makeMPSEQ! The type name %s is invalid !\n",seqName);
      exit(-1);
   }  
   sprintf(pm.seqName,seqName);
   logic = (ix==1); 
   
   pm.nphSuper = 1;
   pm.phSuper[0] = 0.0;
   pm.phAccum = p;
   pm.iSuper = iph;
   pm.nRec = iRec;
   pm.phInt = phint; 
       
// Set DUMBO Phase Array 


printf("timeI ix = %d d2_index = %d logic = %d\n",ix,d2_index,logic);
//phXdumbo

   var = getname("ph",pm.seqName,"");
   double ph = getval(var); 
   logic = (logic || isarry(var)); 

   pm.nphBase = 64;
   double phBase[64] = {31.824000, 111.959215, 140.187768, 118.272283,
   86.096791,   73.451274,  71.082669,  53.716611,  21.276000,   2.170197,
   12.992428,   28.379246,   5.986118, 302.048261, 248.763031, 264.564610,
   348.552000,  74.378423,  94.391707,  45.146603, 343.815209, 323.373522, 
   341.072484, 353.743630, 333.756000, 296.836742, 274.284097, 271.756242,
   268.693882, 255.782366, 257.225816, 304.420775, 124.420775,  77.225816,
    75.782366,  88.693882,  91.756242,  94.284097, 116.836742, 153.756000,
   173.743630, 161.072484, 143.373522, 163.815209, 225.146603, 274.391707,
   254.378423, 168.552000,  84.564610,  68.763031, 122.048261, 185.986118,
   208.379246, 192.992428, 182.170197, 201.276000, 233.716611, 251.082669,
   253.451274, 266.096791, 298.272283, 320.187768, 291.959215, 211.824000};
  
  for (i = 0; i < pm.nphBase; i++) { 
     pm.phBase[i] = phBase[i] + ph;
  }        
		    
//pwXdumbo (360 pulse)
 
   var = getname("pw",pm.seqName,"");
   double pw = getval(var);
   pw = roundoff(pw,0.1e-6);
   logic = (logic || isarry(var));
   
// Set DUMBO pulse width array   

   double pwlast = 0.0; 
   for (i = 0; i < pm.nphBase; i++) { 
      pm.pw[i] = roundoff((i+1)*pw/pm.nphBase,0.1e-6) - pwlast;
      pwlast = pwlast + pm.pw[i];   
      pm.pw[i + pm.nphBase] = pm.pw[i];      
   }   
  
//chXdumbo

   Getstr(getname("ch",pm.seqName,""),pm.ch,sizeof(pm.ch)); 
   
//nXdumbo 

   var = getname("n",pm.seqName,"");  
   pm.nelem = (int) getval(var); 
   logic = (logic || isarry(var));

//aXdumbo
   var = getname("a",pm.seqName,"");
   pm.a = getval(var);  
   
//ofXdumbo

   var = getname("of",pm.seqName,""); 
   pm.of = getval(var);
   logic = (logic || isarry(var)); 

   printf("timeF ix = %d d2_index = %d logic = %d\n",ix,d2_index,logic); 
   
   if (logic) {
     sprintf(pm.pattern,"%s%d_%ld",getname("",pm.seqName,""),pm.nRec,ix); 
   }
   else {
     sprintf(pm.pattern,"%s%d_1",getname("",pm.seqName,""),pm.nRec); 
   } 
   pm = MPchopper(pm); 
   return pm;      
}

//==========
// BABA
//==========

WMPSEQ getbaba(char *seqName, int iph, double p, double phint, int iRec)
{
   int logic = 0;
   char *var;
   WMPSEQ baba;
   extern WMPSEQ WMPchopper(WMPSEQ baba);
   
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1){
        printf("Error in makeWMPSEQ! The type name %s is invalid !\n",seqName);
        exit(-1);
   }  
   sprintf(baba.seqName,seqName);
   logic = (ix==1);
      
   baba.nphBase = 12;
   baba.phBase[0] = 0.0;
   baba.phBase[1] = 0.0;
   baba.phBase[2] = 0.0;
   baba.phBase[3] = 90.0;
   baba.phBase[4] = 270.0;
   baba.phBase[5] = 270.0;
   baba.phBase[6] = 0.0;
   baba.phBase[7] = 0.0;
   baba.phBase[8] = 0.0;
   baba.phBase[9] = 270.0;
   baba.phBase[10] = 90.0;
   baba.phBase[11] = 90.0;
   
   baba.gateBase[0] = 1.0;
   baba.gateBase[1] = 0.0;
   baba.gateBase[2] = 1.0;
   baba.gateBase[3] = 1.0;
   baba.gateBase[4] = 0.0;
   baba.gateBase[5] = 1.0;
   baba.gateBase[6] = 1.0;
   baba.gateBase[7] = 0.0;
   baba.gateBase[8] = 1.0;
   baba.gateBase[9] = 1.0;
   baba.gateBase[10] = 0.0;
   baba.gateBase[11] = 1.0;

   baba.nphSuper = 2;
   baba.phSuper[0] = 0.0;
   baba.phSuper[1] = 180.0;
   
   baba.phAccum = p;
   baba.iSuper = iph;
   baba.nRec = iRec;
   baba.phInt = phint; 
    
//Get standard system parameter "srate" and set tauR. 

   double tauR = 2.0*roundoff(1.0/(2.0*getval("srate")),0.2e-6);      
 
// Construct parameter names and get values

//chXbaba

   Getstr(getname("ch",baba.seqName,""),baba.ch,sizeof(baba.ch)); 
 
//aXbaba

   var = getname("a",baba.seqName,"");
   baba.a = getval(var);  

//pwXbaba

   var = getname("pw",baba.seqName,"");
   double lpw = roundoff(getval(var),0.2e-6); 
   
   if (lpw >= tauR/8.0) lpw = tauR/8.0; // lpw limited to 50% duty cycle
      
   baba.pw[0] = lpw;  
   baba.pw[1] = tauR/2.0 - 2.0*lpw;
   baba.pw[2] = lpw;
   baba.pw[3] = lpw;
   baba.pw[4] = tauR/2.0 - 2.0*lpw;
   baba.pw[5] = lpw;
   baba.pw[6] = lpw;
   baba.pw[7] = tauR/2.0- 2.0*lpw;
   baba.pw[8] = lpw;
   baba.pw[9] = lpw;
   baba.pw[10] = tauR/2.0 - 2.0*lpw;
   baba.pw[11] = lpw;
   
   logic = (logic || isarry(var));

//qXbaba

   var = getname("q",baba.seqName,"");  
   baba.nelem = (int) getval(var); 
   logic = (logic || isarry(var));
   
//ofXbaba

   var = getname("of",baba.seqName,""); 
   baba.of = getval(var);
   logic = (logic || isarry(var));   
   
//write the shape file

   if (logic) {
      sprintf(baba.pattern,"%s%d_%ld",getname("",baba.seqName,""),baba.nRec,ix); 
   }
   else {
      sprintf(baba.pattern,"%s%d_1",getname("",baba.seqName,""),baba.nRec); 
   }
  
   baba = WMPchopper(baba);  
   return baba;
}

//==========
// XY8
//==========

WMPSEQ getxy8(char *seqName, int iph, double p, double phint, int iRec)
{
   int logic = 0;
   char *var;
   WMPSEQ xy8;
   extern WMPSEQ WMPchopper(WMPSEQ xy8);
   
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 1){
        printf("Error in makeWMPSEQ! The type name %s is invalid !\n",seqName);
        exit(-1);
   }  
   sprintf(xy8.seqName,seqName);
   logic = (ix==1);
   
   xy8.nphBase = 4;   
   xy8.phBase[0] = 0.0;
   xy8.phBase[1] = 0.0;
   xy8.phBase[2] = 90.0;
   xy8.phBase[3] = 90.0;
      
   xy8.gateBase[0] = 0.0;
   xy8.gateBase[1] = 1.0;
   xy8.gateBase[2] = 0.0;
   xy8.gateBase[3] = 1.0;
    
   xy8.nphSuper = 4;
   xy8.phSuper[0] = 0.0; 
   xy8.phSuper[1] = 0.0;
   xy8.phSuper[2] = 90.0;
   xy8.phSuper[3] = 90.0;  
   
   xy8.phAccum = p;
   xy8.iSuper = iph;
   xy8.nRec = iRec;
   xy8.phInt = phint; 
   
//Get standard system parameter "srate" and set tauR. 

   double tauR = 2.0*roundoff(1.0/(2.0*getval("srate")),0.2e-6);      
 
// Construct parameter names and get values

//chXxy8

   Getstr(getname("ch",xy8.seqName,""),xy8.ch,sizeof(xy8.ch)); 
 
//aXxy8

   var = getname("a",xy8.seqName,"");
   xy8.a = getval(var);  
   logic = (logic || isarry(var));

//pwXxy8 (180 degree pulse)

   var = getname("pw",xy8.seqName,"");
   double lpw = roundoff(getval(var),0.2e-6); 
   
   if (lpw >= tauR/4.0) lpw = tauR/4.0; // lpw limited to 50% duty cycle

   xy8.pw[0] = tauR/2.0 - lpw;      
   xy8.pw[1] = lpw;  
   xy8.pw[2] = tauR/2.0 - lpw;
   xy8.pw[3] = lpw;   
   
   logic = (logic || isarry(var));

//qXxy8

   var = getname("q",xy8.seqName,"");  
   xy8.nelem = (int) getval(var); 
   logic = (logic || isarry(var));
   
//ofXxy8

   var = getname("of",xy8.seqName,""); 
   xy8.of = getval(var);
   logic = (logic || isarry(var));   
   
//write the shape file

   if (logic) {
      sprintf(xy8.pattern,"%s%d_%ld",getname("",xy8.seqName,""),xy8.nRec,ix); 
   }
   else {
      sprintf(xy8.pattern,"%s%d_1",getname("",xy8.seqName,""),xy8.nRec); 
   }
  
   xy8 = WMPchopper(xy8);  
   return xy8;
}

//======================================================
// Build CP
//======================================================

CP getcp(char *seqName, double p, double phint, int iRec) 
{
   int logic = 0;
   CP cp; 
   char *var; 
   extern CP make_cp(CP cp);  
   cp.phAccum = p;
   cp.phInt = phint;
   cp.nRec = iRec;
   
   if (strlen(seqName) > NSUFFIX  || strlen(seqName) < 2){
      printf("Error in makeCP! The type name %s is invalid !\n",seqName);
      exit(-1);
   }
   logic = (ix==1); 
 
   char s1[2],s2[2];
   
   if (strcmp(seqName,"HX")==0) {
      sprintf(s1,"H");
      sprintf(s2,"X");  
   }
   if (strcmp(seqName,"HY")==0) {
      sprintf(s1,"H");
      sprintf(s2,"Y"); 
   }
   if (strcmp(seqName,"XY")==0) {
      sprintf(s1,"X");
      sprintf(s2,"Y");
   }
   if (strcmp(seqName,"YX")==0) {
      sprintf(s1,"Y");
      sprintf(s2,"X"); 
   }
            
// chHXsuffix 

   Getstr(getname("ch",seqName,""),cp.ch,sizeof(cp.ch));
    
// aHhxsuffix 

   var = getname("a",seqName,s1);
   cp.a1 = getval(var); 
   logic = (logic || (!strcmp(cp.ch,"fr") && isarry(var)));
    
// aXhxsuffix 

   var = getname("a",seqName,s2);
   cp.a2 = getval(var); 
   logic = (logic || (!strcmp(cp.ch,"to") && isarry(var)));
  
// dHXsuffix 

   var = getname("d",seqName,"");
   cp.d = getval(var);
   logic = (logic || isarry(var));
   
// bHXsuffix 

   var = getname("b",seqName,"");
   cp.b = getval(var); 
   logic = (logic || isarry(var));
   
// tHXsuffix 

   var = getname("t",seqName,"");
   cp.t = getval(var); 
   logic = (logic || isarry(var));
   
// ofHXsuffix 

   var = getname("of",seqName,"");
   cp.of = getval(var);
   logic = (logic || isarry(var));
   
// shHXsuffix 

   var = getname("sh",seqName,"");
   Getstr(var,cp.sh,sizeof(cp.sh));
   logic = (logic || isarry(var));
   
// frHXsuffix   

   Getstr(getname("fr",seqName,""),cp.fr,sizeof(cp.fr));
   
// toHXsuffix 

   Getstr(getname("to",seqName,""),cp.to,sizeof(cp.to));
      
   if (logic) {
      sprintf(cp.pattern,"%s_%ld",getname("",seqName,""),ix); 
   }
   else {
      sprintf(cp.pattern,"%s_1",getname("",seqName,"")); 
   }
   cp = make_cp(cp);
   return cp;
}

//=================================
// Build TPPM
//=================================

TPPM gettppm(char *name) 
{
   int logic = 0;
   TPPM dec; 
   char *var;
   extern void make_tppm(TPPM dec); 
   logic = (ix==1); 

// amplitude

   var = getname("a","TPPM",name);
   dec.a = getval(var);
   
//pulse width

   var = getname("pw","TPPM",name); 
   dec.pw = getval(var); 

//phase

   var = getname("ph","TPPM",name);  
   dec.ph = getval(var);  
   logic = (logic || isarry(var)); 
   
//channel

   Getstr(getname("ch","TPPM",name),dec.ch,sizeof(dec.ch)); 
 
//cycle time  

   dec.t = 2.0*dec.pw;	    	

// create the shape 

   if (logic) {
      sprintf(dec.pattern,"%s_%ld",getname("","TPPM",name),ix); 
   } 
   else {
      sprintf(dec.pattern,"%s_1",getname("","TPPM",name)); 
   }  
   make_tppm(dec);   
   return dec;
}

//=================================
// Build SPINAL
//=================================

SPINAL getspinal(char *name) 
{
   int logic = 0;
   SPINAL dec; 
   char *var;
   extern void make_spinal(SPINAL dec); 
   logic = (ix==1); 

//amplitude

   var = getname("a","SPINAL",name);
   dec.a = getval(var);
   logic = (logic || isarry(var));
   
//pulse width

   var = getname("pw","SPINAL",name); 
   dec.pw = getval(var);
   logic = (logic || isarry(var)); 

//phase

   var = getname("ph","SPINAL",name);  
   dec.ph = getval(var);  
   logic = (logic || isarry(var)); 
   
//channel

   Getstr(getname("ch","SPINAL",name),dec.ch,sizeof(dec.ch));
   logic = (logic || isarry(var)); 

//cycle time  
 
   dec.t = 64.0*dec.pw;	    	

// create the shape 

   if (logic) {
      sprintf(dec.pattern,"%s_%ld",getname("","SPINAL",name),ix); 
   } 
   else {
      sprintf(dec.pattern,"%s_1",getname("","SPINAL",name)); 
   }  
   make_spinal(dec);   
   return dec; 
}


