/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef SOLIDELEMENTS_H
#define SOLIDELEMENTS_H

//=================================================================
//  Dynamic Assignment of V Variables and Table Numbers.
//=================================================================
//
//  settablenumber(55); phH90 = settablename(4,table); performs
//  the same action as #define phH90 t55; settable(phH90,4,table);
//  and also sets globatableindex = 54. The next settablename()
//  instruction sets t54.
//
//  setvvarnumber(90); phH90 = setvvarname(); performs the same
//  action as #define phH90 v90; and sets globalvvarindex = 89. The
//  next setvvarname() instruction sets v89. The variable v90 is
//  intialized to codeint "zero".
//
//  The table names count down (presumably from a large number)
//  so that they will not interfere with statically assigned tables
//  The v-variable names count down (presumably from a large number)
//  so that they will not interfere with the statically assigned 
//  v-variables. 
//

static int globaltableindex;
static int globalvvarindex;
static MODULE mod[64];

//==========================================
// Initialize the first tablenumber (=<60)
//==========================================

void settablenumber(int ltableindex)
{
   globaltableindex = ltableindex + BASEINDEX;
}

//=========================================
// Initialize the first v-variable number 
// (=<30 for VNMRS  =<14 for INOVA)
//=========================================

void setvvarnumber(int lvvarindex)
{
#ifndef NVPSG
   if (lvvarindex > 14)
      lvvarindex = 14;
#endif
   globalvvarindex = lvvarindex;
}

//=================================================
// Assign the next tablenumber and do settable()
//=================================================
int settablename(lnumelements,ltablearray)
   int lnumelements,ltablearray[];
{
   int tableindex = globaltableindex;   
   settable(tableindex,lnumelements,ltablearray);
   globaltableindex = globaltableindex - 1;
   return tableindex;
}

//=====================================================
// Assign the next v-variable number and assign "zero"
// INOVA dose not allow the assignment statment - the 
// value is 0 by default.
//=====================================================

int setvvarname()
{
   extern int dps_flag;
   int vvarindex = globalvvarindex;
   if (dps_flag != 1) {
      if (PWRF_DELAY == 0.0) {
         vvarindex = vvarindex + 29;
      }
      else {   
         vvarindex = vvarindex + 21;
      }
   }
   globalvvarindex = globalvvarindex - 1;
   if (PWRF_DELAY == 0.0) assign(zero,vvarindex);
   return vvarindex;
}

//===========================================================
// Initialize a dynamic codeint with a (double) Phase Value,
// and Minimum Stepsize.
//===========================================================

int initphase(double value, double phasestep)
{
   int phasename = setvvarname();
   while (value < 0.0 ) value = value + 360.0;
   while (value >= 360.0) value = value - 360.0;
   int value1 = (int) (value/phasestep + 0.1);
   value = (double) value1;
   initval(value,phasename);
   return(phasename);
}

//==================================
// UTILITY FUNCTIONS
//==================================

double roundoff(double par, double res)
{
   return( ((int) (par/res + 0.5 + 1.0e-14)) * res);
}

double roundphase(double par, double res)
{
   double parval = par;
   while (parval >= 360.0) parval = parval - 360.0;
   while (parval < 0.0) parval = parval + 360.0;
   parval = ((int) (parval/res + 0.5 + 1.0e-14)) * res;
   return(parval);
}

double roundphaseinc(double par, double res)
{
   double parval = par;
   parval = ((int) (parval/res + 0.5 + 1.0e-14)) * res;
   return(parval);
}

double roundcycle(double par, double res, double cycle)
{
   double parval = par;
   while (parval >= cycle) parval = parval - cycle;
   while (parval < 0.0) parval = parval + cycle;
   parval = ((int) (parval/res + 0.5 + 1.0e-14)) * res;
   return(parval);
}

double roundamp(double par, double res)
{
   double parval = par;
   while (parval > 1023.0) parval = 1023.0;
   while (parval < 0.0) parval = 0.0;
   parval = ((int) (parval/res + 0.5 + 1.0e-14)) * res;
   return(parval);
}

//=============================================================================
// sizeof(parval)called in subroutines always returns 4 for the pointer,
// instead of returning the character array size; so provide it
// as the 3rd argument. This Getstr() is supposed to be called only by getcp
// and other initialization functions in this head file.
//=============================================================================

void Getstr(char *parname, char *parval, int arrsize)
{
   char str[MAXSTR];  //getstr() always returns an char array of size MAXSTR
   getstr(parname,str);
   strncpy(parval,str,arrsize);
   if (strlen(str) >= arrsize) *(parval+arrsize-1) = '\0';
}

//=======================================
//Special Blanking for Channels 3 and 4
//=======================================

void _blank34()
{
   extern int NUMch;
   if (NUMch > 3) dec3blank();
   if (NUMch > 2) dec2blank();
}

void _unblank34()
{
   extern int NUMch;
   if (NUMch > 3) dec3unblank();
   if (NUMch > 2) dec2unblank();
}

//==================================================
// Redefinition of startacq() and endacq() for INOVA 
// VNMRJ21B and preceding versions only.
//==================================================

// void startacq(double ldelay)
// {
//   rcvron();
//   delay(ldelay);
// }

// void endacq()
// {
//    rcvroff();
// }

//=======================================================
//Underscore Offset Functions - For Use Before Decoupling 
//=======================================================

void _obsdoffset(double ldelay, double loffset)
{
   if (loffset != 0.0) {
      double dtof = tof + loffset;
      obsoffset(dtof);
      ldelay = ldelay - 4.0e-6;
      if (ldelay < 0.0) ldelay = 0.0;
   }
   double dd = ldelay;
   delay(dd);
}

void _decdoffset(double ldelay, double loffset) 
{
   if (loffset != 0.0) {
      double ddof = dof + loffset;
      decoffset(ddof); 
      ldelay = ldelay - 4.0e-6; 
      if (ldelay < 0.0) ldelay = 0.0;
   } 
   double dd = ldelay;
   delay(dd); 
} 

void _dec2doffset(double ldelay, double loffset) 
{
   if (loffset != 0.0) {
      double ddof2 = dof2 + loffset; 
      dec2offset(ddof2); 
      ldelay = ldelay - 4.0e-6; 
      if (ldelay < 0.0) ldelay = 0.0;
   } 
   double dd = ldelay;
   delay(dd); 
} 

void _dec3doffset(double ldelay, double loffset) 
{
   if (loffset != 0.0) {
      double ddof3 = dof3 + loffset;
      dec3offset(ddof3); 
      ldelay = ldelay - 4.0e-6; 
      if (ldelay < 0.0) ldelay = 0.0;
   } 
   double dd = ldelay;
   delay(dd); 
} 

//====================================================================
// Automatic Naming of Parameters
//
// char* getname0(char *varType, char *varSuffix, char *chXtra)
// varType denotes the variable to be constructed, pw,a,sc etc
// varSuffix is assumed to be of the form abcDEghi where
// abc is normally used to indicate sequence name, eg c7,cp
// DE indicates channel(s) involved in the sequence
// ghi is a descriptor, eg mix,evolve
// chXtra: is an optional explicit channel identifier. It overides
// any captals that may be in varType
// if chXtra is NULL, the function returns the concatentation
//    (varType)(DE)(abcghi)
// if chXtra is not NULL, the function returns the concatenation
//    (varType)(chXtra)(abcdeghi)
//=====================================================================

char varName[MAXSTR*3];

char* getname0(char *varType, char *varSuffix, char *chXtra)
{
   int i, descrCount, typeCount, capCount;
   extern char varName[];
   char chDescr[MAXSTR];
   char chSeqType[MAXSTR];
   char chUP[MAXSTR], chLOW[MAXSTR];

//====================================================================
// Break up the seqType into leading lowercase characters
// which may contain the sequence identifier, uppercase
// chars which may contain a channel identifier
// and trailing lowercase chars which may contain a descriptor.
// Will fail if the caps section contains digits
//====================================================================
    
   descrCount = 0;typeCount = 0; capCount = 0;
   for (i = 0; i< strlen(varSuffix); i++){
      if (islower(varSuffix[i]) || isdigit(varSuffix[i])) {
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

   if (strlen(chXtra) == 0) {
      sprintf(varName,"%s%s%s%s",varType,chUP,chSeqType,chDescr);
      return varName;
   }
   sprintf(varName,"%s%s%s%s%s",varType,chXtra,chSeqType,chLOW,chDescr);
   return varName;
}

//=============================================
//special getname1 for 2 channel names
//=============================================

char* getname1(char *varType, char *varSuffix, int chnl)
{
   int i, descrCount, typeCount, capCount;
   extern char varName[];
   char chDescr[MAXSTR];
   char chSeqType[MAXSTR];
   char chUP[MAXSTR], chLOW[MAXSTR], chID[MAXSTR];

//===============================================================
// Break up the seqType into leading lowercase characters
// which may contain the sequence identifier, uppercase
// chars which may contain a channel identifier
// and trailing lowercase chars which may contain a descriptor
// Will fail if the caps section contains digits
//================================================================

   descrCount = 0;typeCount = 0; capCount = 0;
   for (i = 0; i< strlen(varSuffix); i++) {
      if (islower(varSuffix[i]) || isdigit(varSuffix[i])) {
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
   chID[0] = '\0';

   if (chnl > 0) {
      chID[0] = chUP[chnl-1];
      chID[1] = '\0';
   }

   if (chnl > 0) {
      sprintf(varName,"%s%s%s%s%s",varType,chID,chLOW,chSeqType,chDescr);
   }
   else {
      sprintf(varName,"%s%s%s%s",varType,chUP,chSeqType,chDescr);
   }
   return varName;
}

//====================================================
// Parse "array" to save arrayed parameter-names
// and dimensions.
//====================================================

AR parsearry(AR params)
{
   double dummy[4096];
   char array[MAXSTR];
   char name[MAXSTR];
   getstr("array", array);
   int j = strlen(array);
   int i = 0; int k = 1;
   int r = 0; int s = 0; int t = 0;
   int p = 0; int q = 0;
   int prod = 1;
   int div = 0;

   params.a[0] ='x';
   params.a[1] = 'x';
   q = q + 2;
   params.b[0] = 2;
   params.e[0] = 1;
   params.f[0] = 0;
   t++; r++;

   while(i < j) {
      while((i < j) && (array[i] >= '0')) {
         params.a[q] = array[i];
	 name[p] = array[i];
         k = 0;
         i++; q++; p++;
      }
      if (array[i] == '(') s++;
      if (array[i] == ')') s--;
      if (k < 1) {
         params.b[t] = p;
         name[p] = '\0';
         params.e[t] = getarray(name,dummy);
         params.f[t] = r;
         if (s == 0) r++;
         t++;
      }
      p = 0;
      i++; k++;
   }
   params.a[q] = '\0';

   i = 0;
   while(i < t) {
      params.f[i] = r - 1 - params.f[i];
      i++;
   }
 
   i = 0;
   int ind;
   while(i < t) {
      ind = params.f[i];
      params.j[ind] = params.e[i];
      i++;
   }

   int larraydim = 1;
   i = 0;
   while (i < r) {
      larraydim = larraydim*params.j[i];
      i++;
   }

   int arraydim = getval("arraydim");
   int test1 = arraydim/larraydim;
   int test2 = arraydim%larraydim;

   if ((test1 > 1) && (test2 == 0)) {
      params.e[0] = test1;
      params.j[r-1] = test1; 
   }

   i = 0;
   while (i < r) {
      params.i[i] = 0;
      i++;
   }

   prod = 1;
   i = 0;
   while (i < r) {
      prod = prod*params.j[i]; 
      i++;
   }

   div = ix - 1;
   i = r - 1;
   while (i >= 0 ) {
      prod = prod/params.j[i];
      params.g[i] = div/prod;
      div = div%prod;
      i--;
   }

   params.c = t;
   params.d = r;
   return params;
}

AR disarry(char *arrpar, AR params) 
{
   int ind = 0;
   int i = 0; int j = 0; int k = 0; int l = 0; int m = 0;
   while (i < (params.c)) {
      l = l + params.b[i];
      if (params.b[i] == strlen(arrpar)) {
         m = 0; 
         j = k;
         while ((j < l) && (params.a[j] == arrpar[m])) {
            j++; m++;
         }
      }
      if (m == params.b[i]) {
         ind = params.f[i];
         params.i[ind] = 1;
      }
      k = k + params.b[i];
      i++;
   }
   return params;
}

int hasarry(AR params, char *pattern)
{
   int ret = 0;
   int arrindex = 0;
   int arrindexm = 0;
   int prod = 1;
   int i = 0; int test; int k; int currentmod;
   
   if (ix == 1) {
      k = 0;
      test = 1;
      while ((k < 64) && (test > 0)) {
         if ((mod[k].filled < 1) || ((strcmp(mod[k].pattern,pattern) == 0))) test = 0;
         k++;
      }
      currentmod = k - 1;
      strcpy(mod[currentmod].pattern,pattern);
      mod[currentmod].arrayindex = -1;
      mod[currentmod].filled = 1;
   }

   k = 0;
   test = 1; 
   while ((k < 64) && (test != 0))  {
      test = strcmp(mod[k].pattern,pattern);
      k++;
   }

   currentmod = 0;
   if (k < 64) {
      currentmod = k - 1;
   }
   else {
      printf("Error in hasarry(). Module not found");
      psg_abort(1);
   }

   while (i < (params.d)) {
      if (params.i[i] == 1) {
         arrindex = arrindex + params.g[i]*prod;
         prod = prod*params.j[i];
      }
      i++;
   }

   arrindexm = mod[currentmod].arrayindex;
   if ((arrindex - arrindexm) > 0) {
      ret = 1;
      mod[currentmod].arrayindex = arrindex;
   }
   return ret;
}

int arryindex(AR params)
{
   int arrindex = 0;
   int prod = 1;
   int i = 0;

   while (i < (params.d)) {
      if (params.i[i] == 1) {
         arrindex = arrindex + params.g[i]*prod;
         prod = prod*params.j[i];
      }
      i++;
   }
   return arrindex;
}

void savet(double time, int lix, char *pattern)
{
   int k = 0; int test;
   test = 1;
   while ((k < 64) && (test > 0)) {
      if ((mod[k].filled < 1) || ((strcmp(mod[k].pattern,pattern) == 0))) test = 0;
      k++;
   }
   if (k < 64) {
      mod[k - 1].t[lix] = time;
   }
   else {
      printf("Error in hasarry(). Module not found");
      psg_abort(1);
   }
}

double gett(int lix, char *pattern)
{
   double time = 0.0;
   int k = 0; int test;
   test = 1;
   while ((k < 64) && (test > 0)) {
      if ((mod[k].filled < 1) || ((strcmp(mod[k].pattern,pattern) == 0))) test = 0;
      k++;
   }
   if (k < 64) {
      time = mod[k - 1].t[lix];
   }
   else {
      printf("Error in hasarry(). Module not found");
      psg_abort(1);
   }
   return time; 
}

AR combine_array(AR a, AR b)
{
   AR c;
   int i;  
   if (strcmp(a.a,b.a) == 0) {
      strcpy(c.a,a.a);
      c.c = a.c;
      c.d = a.d;
      for (i = 0; i < a.c; i++) c.b[i] = a.b[i];
      for (i = 0; i < a.c; i++) c.e[i] = a.e[i];
      for (i = 0; i < a.c; i++) c.f[i] = a.f[i];
      for (i = 0; i < a.d; i++) c.g[i] = a.g[i];
      for (i = 0; i < a.d; i++) c.j[i] = a.j[i];
      for (i = 0; i < a.d; i++) {
         c.i[i] = a.i[i] + b.i[i]; 
         if (c.i[i] > 0) c.i[i] = 1;
         else c.i[i] = 0;
      }
      return c;
   }
   else { 
      printf("Error in combine_array(), Arrays are unequal");
      psg_abort(1);
   }
}
#endif


