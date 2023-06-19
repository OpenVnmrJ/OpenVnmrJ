/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
  arrayfuncs
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "group.h"
#include "variables.h"
#include "acqparms.h"
#include "abort.h"
#include "pvars.h"
#include "shims.h"
#include "cps.h"
#include "CSfuncs.h"

#include "PSGFileHeader.h"

#include "safestring.h"

#define DPRTLEVEL 1
#define MAXGLOBALS 136
#define MAXGVARLEN 40
#define MAXVAR 40
#define MAXLOOPS 80
#define MAXSTR 256

#define FALSE 0
#define TRUE 1
#define ERROR 1
#define NOTFOUND -1
#define NOTREE -1

#ifdef DEBUG
#define DPRINT(level, str) \
        if (bgflag >= level) fprintf(stdout,str)
#define DPRINT1(level, str, arg1) \
        if (bgflag >= level) fprintf(stdout,str,arg1)
#define DPRINT2(level, str, arg1, arg2) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2)
#define DPRINT3(level, str, arg1, arg2, arg3) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2,arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4) \
        if (bgflag >= level) fprintf(stdout,str,arg1,arg2,arg3,arg4)
#else
#define DPRINT(level, str)
#define DPRINT1(level, str, arg2)
#define DPRINT2(level, str, arg1, arg2)
#define DPRINT3(level, str, arg1, arg2, arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4)
#endif

extern int      bgflag;
extern int rtinit_count;
extern char   **cnames;		/* pointer array to variable names */
extern double **cvals;		/* pointer array to variable values */
extern int      nnames;		/* number of variable names */
extern double   exptime;
extern int     ss2val;

extern double  d2_init; 	/* Initial value of d2 delay, used in 2D/3D/4D experiments */
extern double  d3_init; 	/* Initial value of d3 delay, used in 2D/3D/4D experiments */
extern double  d4_init; 	/* Initial value of d4 delay, used in 2D/3D/4D experiments */

extern double psDuration();
extern double getExtraLoopTime();
extern double   preacqtime;

extern void AcodeManager_startSubSection(int);
extern void AcodeManager_endSubSection();
extern void initscan();
extern void endofscan();
extern void endofExperiment(int);
extern void nextcodeset();
extern void MainSystemSync();
extern void resolve_endofscan_actions();
extern void func4sfrq(double);
extern void func4dfrq(double);
extern void func4dfrq2(double);
extern void func4dfrq3(double);
extern void func4dfrq4(double);

extern int setshimflag(char *wshim, int *flag);
extern int set_acqvar(int index,int val);
extern void dfltacq();
extern void getlockmode(char *alock, int *mode);
extern void first_done();
extern int createPS(int arrayDim);
extern void check_for_abort();
extern int A_getstring(int tree, const char *name, char **straddr, int index);
extern int A_getreal(int tree, const char *name, double **valaddr, int index);
extern void reset();



/*-----------------------------------------------------------------------
|  structure loopelemt:
|	Looping element for an arrayed experiment.
|	Information needed for an looping element:
|	1. The number of variables in the looping element.
|	2. The number of values, this is constant  for all
|	   variables in a loop element  (check by go)
|	3. The array of pointers to the variable names in the
|	   in the looping element.
|	4. The array of pointers to the addresses of the variable values
|	   in the looping element.
|	5. The array of indecies  into the global structure for the updateing
|	    of the global values
+------------------------------------------------------------------------*/
struct _loopelemt
{
   char           *lpvar[MAXVAR];	/* pointers to variable name */
   char           *varaddr[MAXVAR];	/* pointers to where the variable's
					 * value ptr goes */
   int             numvar;	/* number of variables in loop element */
   int             numvals;	/* number of values, vars must have same #,
				 * chk in go */
   int             vartype[MAXVAR];	/* Basic Type of variable, string,
					 * real */
   int             glblindex[MAXVAR];	/* indexs of variable in to global
					 * structure */
};
typedef struct _loopelemt loopelemt;

static loopelemt *lpel[MAXLOOPS];	/* An array of pointers to looping
					 * elements */

static int numberLoops = 0;

/*-----------------------------------------------------------------------
|  structure glblvar:
|	global variable information to update the variable if it has been
|	arrayed.
|	Information needed for an looping element:
|	1. The variable name.
|	2. The the address of the global variable, so that it can be updated
|	3. Function that is called, to handle lc variable or others that require
|	   some type of translation, or addition information that depends on this
|	   variable.
+------------------------------------------------------------------------*/
struct _glblvar
{
   double         *glblvalue;
   int             (*funcptr) ();
   char            gnam[MAXGVARLEN];
};
typedef struct _glblvar glblvar;

static glblvar  glblvars[MAXGLOBALS];
static int glblCount = 0;

static void setup(char *varptr, int lpindex, int varindex);
static int srchglobal(const char *name);
/*static initglblstruc(int index, char *name, double *glbladdr, int *(function)()); */
/*static void initglblstruc();*/

int numLoops()
{
   return(numberLoops);
}

int varsInLoop(int index)
{
   return( (lpel[index]) ? lpel[index]->numvar : 0);
}

int valuesInLoop(int index)
{
   return( (lpel[index]) ? lpel[index]->numvals : 0);
}

char *varNameInLoop(int index, int index2)
{
   return( (lpel[index] && lpel[index]->lpvar[index2]) ? lpel[index]->lpvar[index2] : NULL);
}

/*------------------------------------------------------------------
|  initlpelements()  -  zero pointers. They will be malloc'ed as needed.
+------------------------------------------------------------------*/
void initlpelements()
{
   int             i;

   for (i = 0; i < MAXLOOPS; i++)
      lpel[i] = (loopelemt *) 0;
   return;
}

void resetlpelements()
{
   int             i;

   for (i = 0; i < MAXLOOPS; i++)
   {
      if (lpel[i])
         free(lpel[i]);
      lpel[i] = (loopelemt *) 0;
   }
   numberLoops = 0;
   return;
}

/*------------------------------------------------------------------
|  printlpel()  -  print lpel strcuture contents
+------------------------------------------------------------------*/
void printlpel()
{
   int             i;
   int             j;

   for (i = 0; i < MAXLOOPS; i++)
   {
      if (lpel[i])
      {
         int num;

         num = lpel[i]->numvar;
         fprintf(stderr, "lpel[%d] contains: numvar= %d with numvalues= %d\n",
	      i, lpel[i]->numvar, lpel[i]->numvals);
         for (j=0; j<num; j++)
         {
            fprintf(stderr, "Loop element %d: Variable[%d] '%s'\n",
	      i, j, lpel[i]->lpvar[j]);
         }
      }
   }
}

/*--------------------------------------------------------------------------
|   parse()/2
|	parse the variable 'array' and setup the loop element structures
+--------------------------------------------------------------------------*/
#define letter(c) ((('a'<=(c))&&((c)<='z'))||(('A'<=(c))&&((c)<='Z'))||((c)=='_')||((c)=='$')||((c)=='#'))
#define digit(c) (('0'<=(c))&&((c)<='9'))
#define COMMA 0x2C
#define RPRIN 0x29
#define LPRIN 0x28
/*--------------------------------------------------------------------------
+--------------------------------------------------------------------------*/
int parse(char *string, int *narrays)
{
   int             state;
   int             varindex;
   char           *ptr;
   char           *varptr;
   int             lpindex = 0;

   state = 0;
   *narrays = 0;
   ptr = string;
   varindex = 0;
   varptr = NULL;
   if (bgflag)
      fprintf(stderr, "parse(): string: '%s' -----\n", string);
   /* ---  test the variables as we parse them --- */
   /*
    * ---  This is a 4 state parser, 0-1: separate variables
    * 2-4: diagonal set variables
    * --
    */
   while (1)
   {
      switch (state)
      {
	    /* ---  start of variable name --- */
	 case 0:
	    if (bgflag)
	    {
	       fprintf(stderr, "Case 0: ");
	       fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    if (letter(*ptr))	/* 1st letter go to state 1 */
	    {
	       varptr = ptr;
	       state = 1;
	       ptr++;
               *narrays += 1;
	    }
	    else
	    {
	       if (*ptr == LPRIN)	/* start of diagnal arrays */
	       {
		  state = 2;
		  ptr++;
                  *narrays += 1;
	       }
	       else
	       {
		  if (*ptr == '\0')	/* done ? */
		     return (0);
		  else		/* error */
		  {
		     text_error("Syntax error in variable array");
		     return (ERROR);
		  }
	       }
	    }
	    if (bgflag)
	       fprintf(stderr, " state = %d \n", state);
	    break;
	    /* --- complete a single array variable till ',' --- */
	 case 1:
	    if (bgflag)
	    {
	       fprintf(stderr, "Case 1: ");
	       fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    if (letter(*ptr) || digit(*ptr))
	    {
	       ptr++;
	    }
	    else
	    {
	       if (*ptr == COMMA)
	       {
		  *ptr = '\0';
		  setup(varptr, lpindex, varindex);
		  ptr++;
		  lpindex++;
		  state = 0;
	       }
	       else
	       {
		  if (*ptr == '\0')
		  {
		     setup(varptr, lpindex, varindex);
		     return (0);
		  }
		  else
		  {
		     text_error("Syntax Error in variable 'array'");
		     return (ERROR);
		  }
	       }
	    }
	    if (bgflag)
	       fprintf(stderr, " state = %d \n", state);
	    break;
	    /* --- start of diagnal arrayed variables  'eg. (pw,d1)' --- */
	 case 2:
	    if (bgflag)
	    {
	       fprintf(stderr, "Case 2: ");
	       fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    /* if (letter1(*ptr)) */
	    if (letter(*ptr))
	    {
	       varptr = ptr;
	       state = 3;
	       ptr++;
	    }
	    else
	    {
	       text_error("Syntax Error in variable 'array'");
	       return (ERROR);
	    }
	    if (bgflag)
	       fprintf(stderr, " state = %d \n", state);
	    break;
	    /* --- finish a diagonal arrayed variable  name --- */
	 case 3:
	    if (bgflag)
	    {
	       fprintf(stderr, "Case 3: ");
	       fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    if (letter(*ptr) || digit(*ptr))
	    {
	       ptr++;
	    }
	    else
	    {
	       if (*ptr == COMMA)
	       {
		  *ptr = '\0';
		  setup(varptr, lpindex, varindex);
		  ptr++;
		  varindex++;
		  state = 2;
	       }
	       else if (*ptr == RPRIN)
	       {
		  *ptr = '\0';
		  setup(varptr, lpindex, varindex);
		  ptr++;
		  varindex = 0; /* was == 0 8-9-90*/
		  state = 4;
	       }
	       else
	       {
		  text_error("Syntax Error in variable 'array'");
		  return (ERROR);
	       }
	    }
	    if (bgflag)
	       fprintf(stderr, " state = %d \n", state);
	    break;
	    /* --- finish a diagonal arrayed variable  set --- */
	 case 4:
	    if (bgflag)
	    {
	       fprintf(stderr, "Case 4: ");
	       fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    if (*ptr == COMMA)
	    {
	       *ptr = '\0';
	       ptr++;
	       lpindex++;
	       varindex = 0;
	       state = 0;
	    }
	    else
	    {
	       if (*ptr == '\0')
	       {
		  return (0);
	       }
	       else
	       {
		  text_error("Syntax Error in variable 'array'");
		  return (ERROR);
	       }
	    }
	    if (bgflag)
	       fprintf(stderr, " state = %d \n", state);
	    break;
      }
   }
}

/*-------------------------------------------------------------------
|  setup - setup lpel sturcture..
|
|  Initialize looping elements for the arrayed experiament
|	IF the variable array = "sw,(pw,d1),dm" then
|        sw variable element is lpelement 0, one variable in loop element.
|        pw variable element is lpelement 1, two variables in loop element.
|        d1 variable element is lpelement 1, two variables in loop element.
|        dm variable element is lpelement 2, one variable in loop element.
|   There are 3 looping elements in this arrayed experiment.
+--------------------------------------------------------------------*/
/* varptr;	pointer to variable name */
/* lpindex;	loop element index 0-# */
/* varindex;	variable index 0-# (# of variable per loop element */
static void setup(char *varptr, int lpindex, int varindex)
{
   int             index;
   int             type;
   int             ret;
   vInfo           varinfo;	/* variable information structure */

   /* --- variable info  --- */
   if ( (ret = P_getVarInfo(CURRENT, varptr, &varinfo)) )
   {
      if (bgflag)
	 P_err(ret, varptr, ": ");
      abort_message("Cannot find the variable: '%s'", varptr);
   }

   type = varinfo.basicType;

   index = find(varptr);
   if (index == NOTFOUND)
   {
      abort_message("variable '%s' does not exist.", varptr);
   }
   if (bgflag)
   {
      fprintf(stderr,
	      "SETUP: variable: '%s', lpindex = %d, varindex = %d, index = %d \n",
	      varptr, lpindex, varindex, index);
      if (type == ST_REAL)
      {
	 fprintf(stderr,
	  "SETUP: cname[%d]: '%s', cvals[%d] = %5.2lf\n",
	  index, cnames[index], index, *cvals[index]);
      }
      else
      {
	 fprintf(stderr,
	    "SETUP: cname[%d]: '%s', cvals[%d] = '%s'\n",
	   index, cnames[index], index, (char *) cvals[index]);
      }
   }

   if ( ! lpel[lpindex] )
   {
      if ( (lpel[lpindex] = (loopelemt *) malloc(MAXLOOPS * sizeof(loopelemt))) == 0L)
      {
	 text_error("insuffient memory for loop elements.");
	 reset();
	 psg_abort(1);
      }
   }

   /* --- number of variables --- */
   lpel[lpindex]->numvar = varindex + 1;


   /* --- number values for loopelement --- */
   lpel[lpindex]->numvals = varinfo.size;

   /* --- Basic Type of the variable, string or real --- */
   lpel[lpindex]->vartype[varindex] = type;	/* varinfo.basicType */

   /* --- pointer to a variable name --- */

   lpel[lpindex]->lpvar[varindex] = cnames[index];

   /* --- pointer to the array of param ptrs --- */

   lpel[lpindex]->varaddr[varindex] = (char *) &cvals[index];

   /* --- index of variable in global structure --- */

   lpel[lpindex]->glblindex[varindex] = srchglobal(varptr);

   if (bgflag)
   {
      int gindx;

      gindx = lpel[lpindex]->glblindex[varindex];
      if (type == ST_REAL)
      {
	fprintf(stderr,
	"SETUP: #var = %d  var type: %d  var name: '%s', value = %5.2lf\n",
		 lpel[lpindex]->numvar, lpel[lpindex]->vartype[varindex],
		 lpel[lpindex]->lpvar[varindex],
		 *(*((double **) lpel[lpindex]->varaddr[varindex])));
      }
      else
      {
	 fprintf(stderr,
	  "SETUP: #var = %d  var type: %d  var name: '%s', value = '%s'\n",
		 lpel[lpindex]->numvar, lpel[lpindex]->vartype[varindex],
		 lpel[lpindex]->lpvar[varindex],
		 *((char **) lpel[lpindex]->varaddr[varindex]));
      }
   }
   numberLoops = lpindex+1;
}

/*-----------------------------------------------------------------------
|    arrayPS:
|	index = looping element index
|	numarrays = number of looping elements
|
|	This routine for each value of an arrayed variable for each
|	variable in the looping element is set in the PS param array.
|	Then this routine recusively calls itself until it reaches the
|	last looping element.
|	The net result are nested FOR loops for performing the arrayed
|	experiment.
|	For 2D experments an array of the d2 values is created and is the
|	 frist looping element(or only element).
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/6/89     Greg B.    1. Added Code to use new lpel structure members
|			  2. Basic restructuring for speed improvements
|			  3. Use global strcuture for updateing global parameters
|
+-----------------------------------------------------------------------*/
static int curarrayindex = 0;

int get_curarrayindex()
{
   return(curarrayindex);
}




static int acodeSubSectionOpen=0;

void arrayPS(int index, int numarrays, int arrayDim)
{
   char           *name;
   char          **parmptr;
   int             valindx;
   int             varindx;
   int             nvals,
                   nvars;
   int             ret;
   int             gindx;
   double         *temptr;
   int             ntss;
   double          fidtime;
   double          arraydimvar;


   name = lpel[index]->lpvar[0];
   nvals = lpel[index]->numvals;


   /* arraydimvar is really acqcycles, use of arraydim would be wrong since this also contains a
    * number of receivers factor, i.e.  arraydim = acqcycles * numrcvrs
    */
   if ((P_getreal(CURRENT,"acqcycles",&arraydimvar,1)) < 0)
   {
          text_error("arrayPS(): cannot read arraydim.");
          psg_abort(1);
   }

   /* --- For each value of the arrayed variable(s) --- */
   for (valindx = 1; valindx <= nvals; valindx++)
   {
      nvars = lpel[index]->numvar;


      if (index == 0)
         nth2D++;		/* if first arrayed variable (i.e. 2D
				   variable) increment */

      /* --- for each of the arrayed variable(s) get its value --- */
      for (varindx = 0; varindx < nvars; varindx++)
      {

	 name = lpel[index]->lpvar[varindx];


	 /* -- if pad 'arrayed' set its global padindex to value index -- */
	 /* -- preacqdelay can tell a new value has been entered and that */
	 /* the pad should be included in the Acodes, otherwise not */
	 /* do the same for temp being arrayed, this way a preacqdelay is */
	 /* done after each change in temperature			 */

/*
	    if ( (strcmp(name,"pad") == 0) || (strcmp(name,"temp") == 0) )
	    {
		newpadindex = valindx;
	    }
*/

	 parmptr = (char **) lpel[index]->varaddr[varindx];


         curarrayindex = valindx - 1;
	 if (lpel[index]->vartype[varindx] != T_STRING)
	 {

	    if ((ret = A_getreal(CURRENT, name, (double **) parmptr, valindx)) < 0)
	    {
	       if (bgflag)
		  P_err(ret, name, ": ");
	       abort_message("Cannot find the variable: '%s'", name);
	    }
	    gindx = lpel[index]->glblindex[varindx];
	    if (gindx >= 0) {

	       if (glblvars[gindx].glblvalue)
		  *(glblvars[gindx].glblvalue) = *((double *) *parmptr);
	       if (glblvars[gindx].funcptr)
               {
                  if (! acodeSubSectionOpen)
                  {
                    AcodeManager_startSubSection(ACODEHEADER+ix+1);
                    acodeSubSectionOpen=1;
                  }
		  (*glblvars[gindx].funcptr) (*((double *) *parmptr));
               }
	    }
	 }
	 else
	 {
	    if ((ret = A_getstring(CURRENT, name, parmptr, valindx)) < 0)
	    {
	       if (bgflag)
		  P_err(ret, name, ": ");
	       abort_message("Cannot find the variable: '%s'", name);
	    }
	    gindx = lpel[index]->glblindex[varindx];
	    if (gindx >= 0) {
	       if (glblvars[gindx].funcptr)
               {
                  if (! acodeSubSectionOpen)
                  {
                    AcodeManager_startSubSection(ACODEHEADER+ix+1);
                    acodeSubSectionOpen=1;
                  }
		  (*glblvars[gindx].funcptr) (((char *) *parmptr));
               }
	    }
	 }


	 temptr = (double *) *parmptr;
	 if (bgflag)
	 {
	    if (lpel[index]->vartype[varindx] != T_STRING)
	    {
	       fprintf(stderr,
		 "Loop Element %d, Variable[%d] '%s', value[%d] = %10.4lf\n",
		       index, varindx, name, valindx, (double) *temptr);
	    }
	    else
	    {
	       fprintf(stderr,
		    "Loop Element %d, Variable[%d] '%s', value[%d] = '%s'\n",
		       index, varindx, name, valindx, (char *) temptr);
	    }
	 }
      }


      if (numarrays > 1)
      {
	 arrayPS(index + 1, numarrays - 1, arrayDim);
      }
      else
      {
	 preacqtime = 0.0;	/* time for preacquisition delay */
         fidtime = psDuration();

	 ix++;			/* generating Acode for FID ix */

         if ((ix % 100) == 0)
            check_for_abort();
         if (ix > 1)		/* otherwise initialRF() acodes are over written */
         {
           Codeptr = Aacode;	/* reset point back to beginning of Code section */
           rtinit_count = 0;
         }

         if ( ! acodeSubSectionOpen)
         {
             AcodeManager_startSubSection(ACODEHEADER+ix);
             acodeSubSectionOpen=1;
         }

         if (ix == 1)
           MainSystemSync();

         initscan();

	 /* update lc elemid = ix */
	  createPS(arrayDim);

      dfltacq();

      resolve_endofscan_actions();

      endofscan();

      /* arraydimvar is really acqcycles, use of arraydim would be wrong since this also contains a
       * number of receivers factor, i.e.  arraydim = acqcycles * numrcvrs
       */

      if (ix != (unsigned int)arraydimvar)
         nextcodeset();
      else
	 endofExperiment(0);   /* means a real experiment */

      AcodeManager_endSubSection();
      acodeSubSectionOpen=0;
      totaltime = psDuration() - fidtime + getExtraLoopTime();

      if (valindx < nvals )
      {
         AcodeManager_startSubSection(ACODEHEADER+ix+1);
         acodeSubSectionOpen=1;
      }


	 /* code offset calcs moved to write_acodes */
         ntss = nt;
	 totaltime -= preacqtime;	/* remove pad before mult by nt */
         if (var_active("ss",CURRENT))
         {
            int ss = (int) (sign_add(getval("ss"), 0.0005));
            if (ss < 0)
               ss = -ss;
            else if (ix != 1)
            {
               ss = ss2val;
            }
            ntss = nt + ss;
         }
	 exptime += totaltime*ntss + preacqtime;

         if (ix == 1)
         {
            first_done();
         }
      }
   }
}

/*------------------------------------------------------------
|  srchglobal()/1  sreaches global structure array for named
|		   variable, returns index into variable
+-----------------------------------------------------------*/
static int srchglobal(const char *name)
{
   int             index;

   index = 0;
   while (index < glblCount)
   {
      if (strcmp(name, glblvars[index].gnam) == 0)
	 return (index);
      index++;
   }
   return (-1);
}

void setGlobalDouble(const char *name, double val)
{
   int             index;

   if ( (index = find(name)) != -1 )
   {
      *(cvals[index]) = val;
   }
}

/*---------------------------------------------------
|  functions call for integer parameters
+--------------------------------------------------*/
static int func4spin(double value)
{
   spin = (int) sign_add(value,0.005);
   spinactive = TRUE;
   DPRINT(DPRTLEVEL,"func for spin called\n");
   return (0);
}
static int func4loc(double value)
{
  loc = (int) sign_add(value,0.005);
   DPRINT(DPRTLEVEL,"func for spin called\n");
   return (0);
}

/*---------------------------------------------------
|  functions call for global double parameters effecting
|  low core or automation structure.
+--------------------------------------------------*/

static int func4pad_temp(double value)
{
   oldpad = -1.0;  /* forces pad for either pad or temp change */
   DPRINT(DPRTLEVEL,"func for pad or temp called");
   return (0);
}

/*---------------------------------------------------
|  string functions call for global string parameters
+--------------------------------------------------*/
static int func4dmm(char *value)
{
   strcpy(dmm, value);
   dmmsize = strlen(dmm);
   DPRINT1(DPRTLEVEL,"func for dmm called value='%s'\n", value);
   return (0);
}
static int func4dm(char *value)
{
   strcpy(dm, value);
   dmsize = strlen(dm);
   DPRINT1(DPRTLEVEL,"func for dm called value='%s'\n", value);
   return (0);
}
static int func4dseq(char *value)
{
   strcpy(dseq, value);
   DPRINT1(DPRTLEVEL,"func for dseq called value='%s'\n", value);
   return (0);
}
static int func4dmm2(char *value)
{
   strcpy(dmm2, value);
   dmm2size = strlen(dmm2);
   DPRINT1(DPRTLEVEL,"func for dmm2 called value='%s'\n", value);
   return (0);
}
static int func4dm2(char *value)
{
   strcpy(dm2, value);
   dm2size = strlen(dm2);
   DPRINT1(DPRTLEVEL,"func for dm2 called value='%s'\n", value);
   return (0);
}
static int func4dseq2(char *value)
{
   strcpy(dseq2, value);
   DPRINT1(DPRTLEVEL,"func for dseq2 called value='%s'\n", value);
   return (0);
}
static int func4dmm3(char *value)
{
   strcpy(dmm3, value);
   dmm3size = strlen(dmm3);
   DPRINT1(DPRTLEVEL,"func for dmm3 called value='%s'\n", value);
   return (0);
}
static int func4dm3(char *value)
{
   strcpy(dm3, value);
   dm3size = strlen(dm3);
   DPRINT1(DPRTLEVEL,"func for dm3 called value='%s'\n", value);
   return (0);
}
static int func4dseq3(char *value)
{
   strcpy(dseq3, value);
   DPRINT1(DPRTLEVEL,"func for dseq3 called value='%s'\n", value);
   return (0);
}
static int func4dmm4(char *value)
{
   strcpy(dmm4, value);
   dmm4size = strlen(dmm4);
   DPRINT1(DPRTLEVEL,"func for dmm4 called value='%s'\n", value);
   return (0);
}
static int func4dm4(char *value)
{
   strcpy(dm4, value);
   dm4size = strlen(dm4);
   DPRINT1(DPRTLEVEL,"func for dm4 called value='%s'\n", value);
   return (0);
}
static int func4dseq4(char *value)
{
   strcpy(dseq4, value);
   DPRINT1(DPRTLEVEL,"func for dseq4 called value='%s'\n", value);
   return (0);
}
static int func4hs(char *value)
{
   strcpy(hs, value);
   hssize = strlen(hs);
   DPRINT1(DPRTLEVEL,"func for hs called value='%s'\n", value);
   return (0);
}
static int func4homo(char *value)
{
   strcpy(homo, value);
   homosize = strlen(homo);
   DPRINT1(DPRTLEVEL,"func for homo called value='%s'\n", value);
   return (0);
}
static int func4homo2(char *value)
{
   strcpy(homo2, value);
   homo2size = strlen(homo2);
   DPRINT1(DPRTLEVEL,"func for homo2 called value='%s'\n", value);
   return (0);
}
static int func4homo3(char *value)
{
   strcpy(homo3, value);
   homo3size = strlen(homo3);
   DPRINT1(DPRTLEVEL,"func for homo3 called value='%s'\n", value);
   return (0);
}
static int func4homo4(char *value)
{
   strcpy(homo4, value);
   homo4size = strlen(homo4);
   DPRINT1(DPRTLEVEL,"func for homo4 called value='%s'\n", value);
   return (0);
}
static int func4alock(char *value)
{
   strcpy(alock, value);
   getlockmode(alock,&lockmode);
   DPRINT1(DPRTLEVEL,"func for alock called value='%s'\n", value);
   return (0);
}
static int func4wshim(char *value)
{
   strcpy(wshim, value);
   whenshim = setshimflag(wshim,&shimatanyfid); /* when to shim */
   DPRINT1(DPRTLEVEL,"func for wshim called value='%s'\n", value);
   return (0);
}
static int func4d2(double value)
{
   if (getCSparIndex("d2") == -1)
      d2_index = get_curarrayindex();
   else
      d2_index = (int) ( (d2 - d2_init) / inc2D + 0.5);
   set_acqvar(id2, (int) d2_index );
   DPRINT(DPRTLEVEL,"func for d2 called\n");
   return (0);
}
static int func4d3(double value)
{
   if (getCSparIndex("d3") == -1)
      d3_index = get_curarrayindex();
   else
      d3_index = (int) ( (d3 - d3_init) / inc3D + 0.5);
   set_acqvar(id3, (int) d3_index );
   DPRINT(DPRTLEVEL,"func for d3 called\n");
   return (0);
}
static int func4d4(double value)
{
   if (getCSparIndex("d4") == -1)
      d4_index = get_curarrayindex();
   else
      d4_index = (int) ( (d4 - d4_init) / inc4D + 0.5);
   set_acqvar(id4, (int) d4_index );
   DPRINT(DPRTLEVEL,"func for d4 called\n");
   return (0);
}
static int func4phase1(double value)
{
  phase1 = (int) sign_add(value,0.005);
   DPRINT(DPRTLEVEL,"func for phase1 called\n");
   return (0);
}
static int func4phase2(double value)
{
  phase2 = (int) sign_add(value,0.005);
   DPRINT(DPRTLEVEL,"func for phase2 called\n");
   return (0);
}
static int func4phase3(double value)
{
  phase3 = (int) sign_add(value,0.005);
   DPRINT(DPRTLEVEL,"func for phase3 called\n");
   return (0);
}
static int func4satmode(char *value)
{
   strcpy(satmode, value);
   DPRINT1(DPRTLEVEL,"func for satmode called value='%s'\n", value);
   return (0);
}


static int func4shimdac(double value)
{
   P_setstring(CURRENT,"load","y",0);
   return (0);
}

/*----------------------------------------------------
|  initglblstruc - load up a global structure element
+----------------------------------------------------*/
static void initglblstruc(const char *name, double *glbladdr, int (*function)() )
{
   int index;
   index = glblCount;
   if (index >= MAXGLOBALS)
   {
      fprintf(stdout, "initglblstruc: index: %d beyond limit %d\n", index, MAXGLOBALS);
      psg_abort(1);
   }
   OSTRCPY( glblvars[index].gnam, sizeof(glblvars[index].gnam), name);
   glblvars[index].glblvalue = glbladdr;
   glblvars[index].funcptr = function;
   glblCount++;
}

/*-----------------------------------------------------------------------
|  elemvalues/1
|       returns the number of values a particular looping element has.
|	If the element is out of bounds then a negative one is return.
|
|                       Author Greg Brissey   6/5/87
+------------------------------------------------------------------------*/
int elemvalues(int elem)
{
   char           *name;
   int             ret;
   vInfo           varinfo;	/* variable information structure */

   if ((elem < 1) || (elem > arrayelements))
   {
      fprintf(stdout,
	      "Warning, elemvalues() called with an invalid element: %d\n",
	      elem);
      return (-1);
   }

   name = lpel[elem - 1]->lpvar[0];
   if ( (ret = P_getVarInfo(CURRENT, name, &varinfo)) )
   {
      if (bgflag)
	 P_err(ret, name, ": ");
      abort_message("Cannot find the variable: '%s'", name);
   }
   return ((int) varinfo.size);	/* # of values variable has */
}

/*-----------------------------------------------------------------------
|  elemindex/2
|       returns which value the element is on for given FID number.
|	If the element is out of bounds then a negative one is return.
|
|                       Author Greg Brissey   6/5/87
+------------------------------------------------------------------------*/
int elemindex(int fid, int elem)
{
   int             i,
                   nvalues,
                   iteration,
                   prior_iter,
                   index = -1;

   if ((elem < 1) || (elem > arrayelements))
   {
      fprintf(stdout,
	      "Warning, elemindex() called with an invalid element: %d\n",
	      elem);
      return (-1);
   }
   fid -= 1;			/* calc requires fid number go from 0 up, not
				 * 1 up */
   prior_iter = fid;

   /*
    * The value an element is on is the mod of the number of cycles the prior
    * element has been through
    */
   for (i = arrayelements; elem <= i; i--)
   {
      nvalues = elemvalues(i);
      iteration = prior_iter / nvalues;
      index = (prior_iter % nvalues) + 1;
      if (bgflag > 1)
	 fprintf(stderr, "elemindex(): elem; %d, on value: %d\n", i, index);
      prior_iter = iteration;
   }
   return (index);
}

/*-------------------------------------------------------
| initglobalptrs() - initialize the global parameter
|		     structure, this allows arrayPS()
|		     to update the global parameter
|		     directly or call a support function,
             thereby not having to call
             no parameter == NULL
             no function == NULL
             typical use update arrayed value for pulse sequence pre-defines
             such as pw, d2 etc.
|		     initparms() for each fid
|			Author: Greg Brissey 6/2/89
|
+-------------------------------------------------------*/
void initglobalptrs()
{
   int shim_index;
   double dum;

   initglblstruc("d1", &d1, NULL);
   initglblstruc("d2", &d2, func4d2);
   initglblstruc("d3", &d3, func4d3);
   initglblstruc("d4", &d4, func4d4);
   initglblstruc("pw", &pw, NULL);
   initglblstruc("p1", &p1, NULL);
   initglblstruc("pw90", &pw90, NULL);
   initglblstruc("pad", &pad, func4pad_temp);
   initglblstruc("rof1", &rof1, NULL);
   initglblstruc("rof2", &rof2, NULL);
   initglblstruc("hst", &hst, NULL);
   initglblstruc("alfa", &alfa, NULL);
   initglblstruc("sw", &sw, NULL);
   initglblstruc("nf", &nf, NULL);
   initglblstruc("np", &np, NULL);
   initglblstruc("nt", &nt, NULL);
   initglblstruc("sfrq", &sfrq, NULL);
   initglblstruc("dfrq", &dfrq, NULL);
   initglblstruc("dfrq2", &dfrq2, NULL);
   initglblstruc("dfrq3", &dfrq3, NULL);
   initglblstruc("dfrq4", &dfrq4, NULL);
   initglblstruc("fb", &fb, NULL);
   initglblstruc("bs", &bs, NULL);
   initglblstruc("tof",  &tof,  NULL);
   initglblstruc("dof",  &dof,  NULL);
   initglblstruc("dof2", &dof2, NULL);
   initglblstruc("dof3", &dof3, NULL);
   initglblstruc("dof4", &dof4, NULL);
   initglblstruc("gain", &gain, NULL);
   initglblstruc("dlp", &dlp, NULL);
   initglblstruc("dhp", &dhp, NULL);
   initglblstruc("tpwr", &tpwr, NULL);
   initglblstruc("dpwr", &dpwr, NULL);
   initglblstruc("dpwr2", &dpwr2, NULL);
   initglblstruc("dpwr3", &dpwr3, NULL);
   initglblstruc("dpwr4", &dpwr4, NULL);
   if ( P_getreal(CURRENT,"tpwrm",&dum,1) == 0 )
      initglblstruc("tpwrm", &tpwrf, NULL);
   else
      initglblstruc("tpwrf", &tpwrf, NULL);
   if ( P_getreal(CURRENT,"dpwrm",&dum,1) == 0 )
      initglblstruc("dpwrm", &dpwrf, NULL);
   else
      initglblstruc("dpwrf", &dpwrf, NULL);
   initglblstruc("dpwrm2", &dpwrf2, NULL);
   initglblstruc("dpwrm3", &dpwrf3, NULL);
   initglblstruc("temp", &vttemp, func4pad_temp);
   initglblstruc("dmf",  &dmf,  NULL);
   initglblstruc("dmf2", &dmf2, NULL);
   initglblstruc("dmf3", &dmf3, NULL);
   initglblstruc("dmf4", &dmf4, NULL);
   initglblstruc("dseq", NULL, func4dseq);
   initglblstruc("dseq2", NULL, func4dseq2);
   initglblstruc("dseq3", NULL, func4dseq3);
   initglblstruc("dseq4", NULL, func4dseq4);
   initglblstruc("phase", NULL, func4phase1);
   initglblstruc("phase2", NULL, func4phase2);
   initglblstruc("phase3", NULL, func4phase3);

/* status control parameters */
   initglblstruc("dmm", NULL, func4dmm);
   initglblstruc("dm", NULL, func4dm);
   initglblstruc("dmm2", NULL, func4dmm2);
   initglblstruc("dm2", NULL, func4dm2);
   initglblstruc("dmm3", NULL, func4dmm3);
   initglblstruc("dm3", NULL, func4dm3);
   initglblstruc("dmm4", NULL, func4dmm4);
   initglblstruc("dm4", NULL, func4dm4);
   initglblstruc("hs", NULL, func4hs);
   initglblstruc("homo", NULL, func4homo);
   initglblstruc("homo2", NULL, func4homo2);
   initglblstruc("homo3", NULL, func4homo3);
   initglblstruc("homo4", NULL, func4homo4);

/* lock parameters */
   initglblstruc("lockpower", NULL, NULL);
   initglblstruc("lockgain",  NULL, NULL);
   initglblstruc("lockphase", NULL, NULL);

   for (shim_index= Z0 + 1; shim_index < MAX_SHIMS; shim_index++)
   {
      const char *sh_name;
      if ((sh_name = get_shimname(shim_index)) != NULL)
          initglblstruc( sh_name, NULL, func4shimdac);
   }

   initglblstruc("spin", NULL, func4spin); 	/* assume spinactive = true */
   initglblstruc("vtc", &vtc, NULL); 	     	/* VT cooling gas  setting */
   initglblstruc("loc", NULL, func4loc);
   initglblstruc("alock", NULL, func4alock);
   initglblstruc("wshim", NULL, func4wshim);
   initglblstruc("vtwait", &vtwait, NULL); 	/* VT timeout setting */

   initglblstruc("pwx", &pwx, NULL);
   initglblstruc("pwxlvl", &pwxlvl, NULL);
   initglblstruc("tau", &tau, NULL);
   initglblstruc("satdly", &satdly, NULL);
   initglblstruc("satfrq", &satfrq, NULL);
   initglblstruc("satpwr", &satpwr, NULL);
   initglblstruc("satmode", NULL, func4satmode);
   initglblstruc("roff", &roff, NULL);
   initglblstruc("gradalt", &gradalt, NULL);

   /* MAX is 136 at present, change MAXGLOBALS if needed */

}

