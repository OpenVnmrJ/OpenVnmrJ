/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "oopc.h"
#include "group.h"
#include "variables.h"
#include "rfconst.h"
#include "acqparms.h"
#include "pvars.h"
#include "shims.h"
#include "CSfuncs.h"
#include "abort.h"

#define DPRTLEVEL 1
#define MAXGLOBALS 135
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

extern char    *ObjError(), *ObjCmd();

extern int      bgflag;
extern int rtinit_count;
extern char   **cnames;		/* pointer array to variable names */
extern double **cvals;		/* pointer array to variable values */
extern int      nnames;		/* number of variable names */
extern double   exptime;

extern double  d2_init;         /* Initial value of d2 delay, used in 2D/3D/4D experiments */
extern double  d3_init;         /* Initial value of d3 delay, used in 2D/3D/4D experiments */
extern double  d4_init;         /* Initial value of d4 delay, used in 2D/3D/4D experiments */

extern double getval();
extern double sign_add();
extern double   preacqtime;

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
static void setup(char *varptr, int lpindex, int varindex);
static int srchglobal(char *name);

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
|  initlpelements()  -  malloc space for the loop elements required
|			Author: Greg Brissey 6/2/89
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
    * ---  This is a 4 state parser, 0-1: separate variables /* ---
    * 2-4: diagonal set variables /* ---
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
/*  *varptr	 pointer to variable name */
/*  lpindex	 loop element index 0-# */
/*  varindex	 variable index 0-# (# of variable per loop element */
static void setup(char *varptr, int lpindex, int varindex)
{
   int             index;
   int             type;
   int             ret;
   char            mess[MAXSTR];
   vInfo           varinfo;	/* variable information structure */

   /* --- variable info  --- */
   if (ret = P_getVarInfo(CURRENT, varptr, &varinfo))
   {
      sprintf(mess, "Cannot find the variable: '%s'", varptr);
      text_error(mess);
      if (bgflag)
	 P_err(ret, varptr, ": ");
      psg_abort(1);
   }

   type = varinfo.basicType;

   index = find(varptr);
   if (index == NOTFOUND)
   {
      sprintf(mess, "variable '%s' does not exist.", varptr);
      text_error(mess);
      psg_abort(1);
   }
   if (bgflag)
      fprintf(stderr,
	      "SETUP: variable: '%s', lpindex = %d, varindex = %d, &varindex = %lx, index = %d \n",
	      varptr, lpindex, varindex, &varindex, index);

   if (bgflag)
      if (type == ST_REAL)
      {
	 fprintf(stderr,
	  "SETUP: cname[%d]: '%s', cvals[%d] = %5.2lf, &cvals[%d] = %lx \n",
	  index, cnames[index], index, *cvals[index], index, &cvals[index]);
      }
      else
      {
	 fprintf(stderr,
	    "SETUP: cname[%d]: '%s', cvals[%d] = '%s', &cvals[%d] = %lx \n",
	   index, cnames[index], index, cvals[index], index, &cvals[index]);
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
	"SETUP: #var = %d  var type: %d  var name: '%s', value = %5.2lf, &value = 0x%lx \n",
		 lpel[lpindex]->numvar, lpel[lpindex]->vartype[varindex],
		 lpel[lpindex]->lpvar[varindex],
		 *(*((double **) lpel[lpindex]->varaddr[varindex])),
		 (lpel[lpindex]->varaddr[varindex]));
	fprintf(stderr,
	 "SETUP: global index: %d, gobal var addr: 0x%lx, gobal var func: 0x%lx \n",
	   gindx,glblvars[gindx].glblvalue, glblvars[gindx].funcptr);
      }
      else
      {
	 fprintf(stderr,
	  "SETUP: #var = %d  var type: %d  var name: '%s', value = '%s', &value = 0x%lx \n",
		 lpel[lpindex]->numvar, lpel[lpindex]->vartype[varindex],
		 lpel[lpindex]->lpvar[varindex],
		 *((char **) lpel[lpindex]->varaddr[varindex]),
		 (lpel[lpindex]->varaddr[varindex]));
	fprintf(stderr,
	 "SETUP: global index: %d, gobal var addr: 0x%lx, gobal var func: 0x%lx \n",
	   gindx,glblvars[gindx].glblvalue, glblvars[gindx].funcptr);
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

arrayPS(index, numarrays)
int             index;
int             numarrays;
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
   char            mess[MAXSTR];

   name = lpel[index]->lpvar[0];
   nvals = lpel[index]->numvals;

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
	    if ( (strcmp(name,"pad") == NULL) || (strcmp(name,"temp") == NULL) )
	    {
		newpadindex = valindx;
	    }
*/

	 parmptr = (char **) lpel[index]->varaddr[varindx];

         curarrayindex = valindx - 1;
	 if (lpel[index]->vartype[varindx] != T_STRING)
	 {
	    if ((ret = A_getreal(CURRENT, name, parmptr, valindx)) < 0)
	    {
	       sprintf(mess, "Cannot find the variable: '%s'", name);
	       text_error(mess);
	       if (bgflag)
		  P_err(ret, name, ": ");
	       psg_abort(1);
	    }
	    gindx = lpel[index]->glblindex[varindx];
	    if (gindx >= 0) {
	       if (glblvars[gindx].glblvalue)
		  *(glblvars[gindx].glblvalue) = *((double *) *parmptr);
	       if (glblvars[gindx].funcptr)
		  (*glblvars[gindx].funcptr) (*((double *) *parmptr));
	    }
	 }
	 else
	 {
	    if ((ret = A_getstring(CURRENT, name, parmptr, valindx)) < 0)
	    {
	       sprintf(mess, "Cannot find the variable: '%s'", name);
	       text_error(mess);
	       if (bgflag)
		  P_err(ret, name, ": ");
	       psg_abort(1);
	    }
	    gindx = lpel[index]->glblindex[varindx];
	    if (gindx >= 0) {
	       if (glblvars[gindx].funcptr)
		  (*glblvars[gindx].funcptr) (((char *) *parmptr));
	    }
	 }

	 temptr = (double *) *parmptr;
	 if (bgflag)
	 {
	    if (lpel[index]->vartype[varindx] != T_STRING)
	    {
	       fprintf(stderr,
		 "Loop Element %d, Variable[%d] '%s', value[%d] = %10.4lf, g_addr: 0x%lx, g_func: 0x%lx\n", 
		       index, varindx, name, valindx, (double) *temptr, 
	               glblvars[gindx].glblvalue, glblvars[gindx].funcptr);
	    }
	    else
	    {
	       fprintf(stderr,
		    "Loop Element %d, Variable[%d] '%s', value[%d] = '%s', g_addr: 0x%lx, g_func: 0x%lx\n",
		       index, varindx, name, valindx, (char *) temptr,
			glblvars[gindx].glblvalue, glblvars[gindx].funcptr);
	    }
	 }
      }
      if (numarrays > 1)
	 arrayPS(index + 1, numarrays - 1);
      else
      {
	 totaltime = 0.0;	/* total timer events for a fid */
	 preacqtime = 0.0;	/* time for preacquisition delay */
	 ix++;			/* generating Acode for FID ix */
         if ((ix % 100) == 0)
            check_for_abort();
         set_lacqvar(fidctr, (codeulong) ix);    /*  fid number */
         if (ix > 1)		/* otherwise initialRF() acodes are over written */
         {
           Codeptr = Aacode;	/* reset point back to beginning of Code section */
           rtinit_count = 0;
         }
	 /* update lc elemid = ix */
	 createPS();

	 /* code offset calcs moved to write_acodes */

	 totaltime -= preacqtime;	/* remove pad before mult by nt */
         if (var_active("ss",CURRENT))
         {
            int ss = (int) (sign_add(getval("ss"), 0.0005));
            if (ss < 0)
               ss = -ss;
            else if (ix != 1)
               ss = 0;
            totaltime *= (nt + (double) ss);   /* mult by NT + SS */
         }
         else
	    totaltime *= nt;	/* mult by number of transients (NT) */
	 exptime += (totaltime + preacqtime); /* added back pad, add up times */
         if (ix == 1)
            first_done();
      }
   }
}

/*------------------------------------------------------------
|  srchglobal()/1  sreaches global structure array for named
|		   variable, returns index into variable
+-----------------------------------------------------------*/
static int srchglobal(char *name)
{
   int             index;

   index = 0;
   while (index < MAXGLOBALS)
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
|  functions call for parameters effecting
|  frequencies .
+--------------------------------------------------*/
static
func4sfrq(value)
double 		value;
{
  if ( SetRFChanAttr(RF_Channel[OBSch],SET_OVRUNDRFLG,0.0,
                   SET_SPECFREQ,value,NULL) < 0 )
	psg_abort(1);
}
static
func4dfrq(value)
double 		value;
{
  if ( SetRFChanAttr(RF_Channel[DECch],SET_OVRUNDRFLG,0.0,
                   SET_SPECFREQ,value,NULL) < 0 )
	psg_abort(1);
}
static
func4dfrq2(value)
double          value;
{
  if ( RF_Channel[DEC2ch] != NULL)
  {
     if ( SetRFChanAttr(RF_Channel[DEC2ch],SET_OVRUNDRFLG,0.0,
                   SET_SPECFREQ,value,NULL) < 0 )
	psg_abort(1);
  }
  else if (ix < 2)
     text_error("WARNING: dfrq2 arrayed, but no 3rd channel present.\n");
}
static
func4dfrq3(value)
double          value;
{
  if (RF_Channel[DEC3ch] != NULL)
  {
     if ( SetRFChanAttr(RF_Channel[DEC3ch],SET_OVRUNDRFLG,0.0,
                   SET_SPECFREQ,value,NULL) < 0 )
	psg_abort(1);
  }
  else if (ix < 2)
     text_error("WARNING: dfrq3 arrayed, but no 4th channel present.\n");
}
static
func4dfrq4(value)
double          value;
{
  if (RF_Channel[DEC4ch] != NULL)
  {
     if ( SetRFChanAttr(RF_Channel[DEC4ch],SET_OVRUNDRFLG,0.0,
                   SET_SPECFREQ,value,NULL) < 0 )
	psg_abort(1);
  }
  else if (ix < 2)
     text_error("WARNING: dfrq4 arrayed, but no 5th channel present.\n");
}
static
func4tof(value)
double 		value;
{
  if ( SetRFChanAttr(RF_Channel[OBSch],SET_OVRUNDRFLG,0.0,
                   SET_OFFSETFREQ,value,NULL) < 0 )
	psg_abort(1);
}
static
func4dof(value)
double 		value;
{
  if ( SetRFChanAttr(RF_Channel[DECch],SET_OVRUNDRFLG,0.0,
                   SET_OFFSETFREQ,value,NULL) < 0 )
	psg_abort(1);
}
static
func4dof2(value)
double          value;
{
  if ( RF_Channel[DEC2ch] != NULL)
  {
     if ( SetRFChanAttr(RF_Channel[DEC2ch],SET_OVRUNDRFLG,0.0,
                   SET_OFFSETFREQ,value,NULL) < 0 )
	psg_abort(1);
  }
  else if (ix < 2)
     text_error("WARNING: dof2 arrayed, but no 3rd channel present.\n");
}
static
func4dof3(value)
double          value;
{
  if (RF_Channel[DEC3ch] != NULL)
  {
     if ( SetRFChanAttr(RF_Channel[DEC3ch],SET_OVRUNDRFLG,0.0,
                   SET_OFFSETFREQ,value,NULL) < 0 )
	psg_abort(1);
  }
  else if (ix < 2)
     text_error("WARNING: dof3 arrayed, but no 4th channel present.\n");
}
static
func4dof4(value)
double          value;
{
  if (RF_Channel[DEC4ch] != NULL)
  {
     if ( SetRFChanAttr(RF_Channel[DEC4ch],SET_OVRUNDRFLG,0.0,
                   SET_OFFSETFREQ,value,NULL) < 0 )
	psg_abort(1);
  }
  else if (ix < 2)
     text_error("WARNING: dof4 arrayed, but no 5th channel present.\n");
}
static
func4dmf(value)
double 		value;
{
  initdecmodfreq(value,2,INIT_APVAL);	/* set decoupler modulation freq */
}
static
func4dmf2(value)
double 		value;
{
  initdecmodfreq(value,2,INIT_APVAL);	/* set decoupler modulation freq */
}
static
func4dmf3(value)
double 		value;
{
  initdecmodfreq(value,2,INIT_APVAL);	/* set decoupler modulation freq */
}
static
func4dmf4(value)
double 		value;
{
  initdecmodfreq(value,2,INIT_APVAL);	/* set decoupler modulation freq */
}
/*---------------------------------------------------
|  functions call for integer parameters 
+--------------------------------------------------*/
static
func4spin(value)
double          value;
{
   spin = (int) sign_add(value,0.005);
   spinactive = TRUE;
   DPRINT(DPRTLEVEL,"func for spin called\n");
   return (0);
}
static
func4loc(value)
double          value;
{
  loc = (int) sign_add(value,0.005);
   DPRINT(DPRTLEVEL,"func for spin called\n");
   return (0);
}
static
func4dhp_dlp(value)
double          value;
{
   decouplerattn(INIT_APVAL);
   DPRINT(DPRTLEVEL,"func for dhp or dlp to set dec atten \n");
   return (0);
}
/*---------------------------------------------------
|  functions call for global double parameters effecting
|  low core or automation structure.
+--------------------------------------------------*/
static
func4nt(value)
double          value;
{
 long tempnt;
   tempnt = (codelong) (value + 0.005);
   set_lacqvar(ntrt, tempnt);
   DPRINT1(DPRTLEVEL,"func for nt called, new value: %ld\n", (codelong) (value + 0.005));
   return (0);
}
static
func4pad_temp(value)
double          value;
{
   oldpad = -1.0;  /* forces pad for either pad or temp change */
   DPRINT(DPRTLEVEL,"func for pad or temp called");
   return (0);
}

/*---------------------------------------------------
|  string functions call for global string parameters
+--------------------------------------------------*/
static
func4dmm(value)
char           *value;
{
   strcpy(dmm, value);
   dmmsize = strlen(dmm);
   DPRINT1(DPRTLEVEL,"func for dmm called value='%s'\n", value);
   return (0);
}
static
func4dm(value)
char           *value;
{
   strcpy(dm, value);
   dmsize = strlen(dm);
   DPRINT1(DPRTLEVEL,"func for dm called value='%s'\n", value);
   return (0);
}
static
func4dseq(value)
char           *value;
{
   strcpy(dseq, value);
   DPRINT1(DPRTLEVEL,"func for dseq called value='%s'\n", value);
   return (0);
}
static
func4dmm2(value)
char           *value;
{
   strcpy(dmm2, value);
   dmm2size = strlen(dmm2);
   DPRINT1(DPRTLEVEL,"func for dmm2 called value='%s'\n", value);
   return (0);
}
static
func4dm2(value)
char           *value;
{
   strcpy(dm2, value);
   dm2size = strlen(dm2);
   DPRINT1(DPRTLEVEL,"func for dm2 called value='%s'\n", value);
   return (0);
}
static 
func4dseq2(value) 
char           *value;
{ 
   strcpy(dseq2, value); 
   DPRINT1(DPRTLEVEL,"func for dseq2 called value='%s'\n", value); 
   return (0);
}
static
func4dmm3(value)
char           *value;
{
   strcpy(dmm3, value);
   dmm3size = strlen(dmm3);
   DPRINT1(DPRTLEVEL,"func for dmm3 called value='%s'\n", value);
   return (0);
}
static
func4dm3(value)
char           *value;
{
   strcpy(dm3, value);
   dm3size = strlen(dm3);
   DPRINT1(DPRTLEVEL,"func for dm3 called value='%s'\n", value);
   return (0);
}
static 
func4dseq3(value) 
char           *value;
{ 
   strcpy(dseq3, value); 
   DPRINT1(DPRTLEVEL,"func for dseq3 called value='%s'\n", value); 
   return (0);
}
static
func4dmm4(value)
char           *value;
{
   strcpy(dmm4, value);
   dmm4size = strlen(dmm4);
   DPRINT1(DPRTLEVEL,"func for dmm4 called value='%s'\n", value);
   return (0);
}
static
func4dm4(value)
char           *value;
{
   strcpy(dm4, value);
   dm4size = strlen(dm4);
   DPRINT1(DPRTLEVEL,"func for dm4 called value='%s'\n", value);
   return (0);
}
static 
func4dseq4(value) 
char           *value;
{ 
   strcpy(dseq4, value); 
   DPRINT1(DPRTLEVEL,"func for dseq4 called value='%s'\n", value); 
   return (0);
}
static
func4hs(value)
char           *value;
{
   strcpy(hs, value);
   hssize = strlen(hs);
   DPRINT1(DPRTLEVEL,"func for hs called value='%s'\n", value);
   return (0);
}
static
func4homo(value)
char           *value;
{
   strcpy(homo, value);
   homosize = strlen(homo);
   DPRINT1(DPRTLEVEL,"func for homo called value='%s'\n", value);
   return (0);
}
static
func4homo2(value)
char           *value;
{
   strcpy(homo2, value);
   homo2size = strlen(homo2);
   DPRINT1(DPRTLEVEL,"func for homo2 called value='%s'\n", value);
   return (0);
}
static
func4homo3(value)
char           *value;
{
   strcpy(homo3, value);
   homo3size = strlen(homo3);
   DPRINT1(DPRTLEVEL,"func for homo3 called value='%s'\n", value);
   return (0);
}
static
func4homo4(value)
char           *value;
{
   strcpy(homo4, value);
   homo4size = strlen(homo4);
   DPRINT1(DPRTLEVEL,"func for homo4 called value='%s'\n", value);
   return (0);
}
static
func4alock(value)
char           *value;
{
   strcpy(alock, value);
   getlockmode(alock,&lockmode);
   DPRINT1(DPRTLEVEL,"func for alock called value='%s'\n", value);
   return (0);
}
static
func4wshim(value)
char           *value;
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
   set_acqvar(id2, (codeint) d2_index );
   DPRINT(DPRTLEVEL,"func for d2 called\n");
   return (0);
}
static int func4d3(double value)
{
   if (getCSparIndex("d3") == -1)
      d3_index = get_curarrayindex();
   else
      d3_index = (int) ( (d3 - d3_init) / inc3D + 0.5);

   set_acqvar(id3, (codeint) d3_index );
   DPRINT(DPRTLEVEL,"func for d3 called\n");
   return (0);
}
static int func4d4(double value)
{
   if (getCSparIndex("d4") == -1)
      d4_index = get_curarrayindex();
   else
      d4_index = (int) ( (d4 - d4_init) / inc4D + 0.5);
   set_acqvar(id4, (codeint) d4_index );
   DPRINT(DPRTLEVEL,"func for d4 called\n");
   return (0);
}
static
func4phase1(value)
double          value;
{
  phase1 = (int) sign_add(value,0.005);
   DPRINT(DPRTLEVEL,"func for phase1 called\n");
   return (0);
}
static
func4phase2(value)
double          value;
{
  phase2 = (int) sign_add(value,0.005);
   DPRINT(DPRTLEVEL,"func for phase2 called\n");
   return (0);
}
static
func4phase3(value)
double          value;
{
  phase3 = (int) sign_add(value,0.005);
   DPRINT(DPRTLEVEL,"func for phase3 called\n");
   return (0);
}
static 
func4satmode(value) 
char           *value;
{ 
   strcpy(satmode, value); 
   DPRINT1(DPRTLEVEL,"func for satmode called value='%s'\n", value); 
   return (0);
}

/*---------------------------------------------------
|  Auto structure parameter  functions
+--------------------------------------------------*/
static
func4lkpwr(value)
double          value;
{
   set_lockpower( (codeint) sign_add(value, 0.005));
   return (0);
}
static
func4lkgain(value)
double          value;
{
   set_lockgain( (codeint) sign_add(value, 0.005));
   return (0);
}
static
func4lkphase(value)
double          value;
{
   set_lockphase( (codeint) sign_add(value, 0.005));
   return (0);
}
static
func4rcgain(value)
double          value;
{
   set_recgain( (codeint) sign_add(gain,0.0005));
   gainactive = TRUE; /* non arrayable */
   return (0);
}
static
func4shimdac(value)
double          value;
{
   P_setstring(CURRENT,"load","y",0);
   return (0);
}

/*----------------------------------------------------
|  initglblstruc - load up a global structure element
+----------------------------------------------------*/
static
initglblstruc(index, name, glbladdr, function)
int             index;
char           *name;
double         *glbladdr;
int             (*function) ();

{
   if (index >= MAXGLOBALS)
   {
      fprintf(stdout, "initglblstruc: index: %d beyond limit %d\n", index, MAXGLOBALS);
      psg_abort(1);
   }
   strcpy(glblvars[index].gnam, name);
   glblvars[index].glblvalue = glbladdr;
   glblvars[index].funcptr = function;
}

/*-----------------------------------------------------------------------
|  elemvalues/1
|       returns the number of values a particlur looping element has.
|	If the element is out of bounds then a negative one is return.
|
|                       Author Greg Brissey   6/5/87
+------------------------------------------------------------------------*/
elemvalues(elem)
int             elem;
{
   char           *name;
   char            mess[MAXSTR];
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
   if (ret = P_getVarInfo(CURRENT, name, &varinfo))
   {
      sprintf(mess, "Cannot find the variable: '%s'", name);
      text_error(mess);
      if (bgflag)
	 P_err(ret, name, ": ");
      psg_abort(1);
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
elemindex(fid, elem)
int             fid;
int             elem;
{
   int             i,
                   nvalues,
                   iteration,
                   prior_iter,
                   index;

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
|		     directly, thereby not having to call
|		     initparms() for each fid
|			Author: Greg Brissey 6/2/89
+-------------------------------------------------------*/
initglobalptrs()
{
   int index;
   int shim_index;
   double dum;

   index = 0;
   initglblstruc(index++, "d1", &d1, 0L);
   initglblstruc(index++, "d2", &d2, func4d2);
   initglblstruc(index++, "d3", &d3, func4d3);
   initglblstruc(index++, "d4", &d4, func4d4);
   initglblstruc(index++, "pw", &pw, 0L);
   initglblstruc(index++, "p1", &p1, 0L);
   initglblstruc(index++, "pw90", &pw90, 0L);
   initglblstruc(index++, "pad", &pad, func4pad_temp);
   initglblstruc(index++, "rof1", &rof1, 0L);
   initglblstruc(index++, "rof2", &rof2, 0L);
   initglblstruc(index++, "hst", &hst, 0L);
   initglblstruc(index++, "alfa", &alfa, 0L);
   initglblstruc(index++, "sw", &sw, 0L);
   initglblstruc(index++, "nf", &nf, 0L);
   initglblstruc(index++, "np", &np, 0L);
   initglblstruc(index++, "nt", &nt, func4nt);
   initglblstruc(index++, "sfrq", &sfrq, func4sfrq);
   initglblstruc(index++, "dfrq", &dfrq, func4dfrq);
   initglblstruc(index++, "dfrq2", &dfrq2, func4dfrq2);
   initglblstruc(index++, "dfrq3", &dfrq3, func4dfrq3);
   initglblstruc(index++, "dfrq4", &dfrq4, func4dfrq4);
   initglblstruc(index++, "fb", &fb, 0L);
   initglblstruc(index++, "bs", &bs, 0L);
   initglblstruc(index++, "tof", &tof, func4tof);
   initglblstruc(index++, "dof", &dof, func4dof);
   initglblstruc(index++, "dof2", &dof2, func4dof2);
   initglblstruc(index++, "dof3", &dof3, func4dof3);
   initglblstruc(index++, "dof4", &dof4, func4dof4);
   initglblstruc(index++, "gain", &gain, func4rcgain);
   initglblstruc(index++, "dlp", &dlp, func4dhp_dlp);  /* set old style dec atten */
   initglblstruc(index++, "dhp", &dhp, func4dhp_dlp);  /* set old style dec atten */
   initglblstruc(index++, "tpwr", &tpwr, 0L);
   initglblstruc(index++, "dpwr", &dpwr, 0L);
   initglblstruc(index++, "dpwr2", &dpwr2, 0L);
   initglblstruc(index++, "dpwr3", &dpwr3, 0L);
   initglblstruc(index++, "dpwr4", &dpwr4, 0L);
   if ( P_getreal(CURRENT,"tpwrm",&dum,1) == 0 )
      initglblstruc(index++, "tpwrm", &tpwrf, 0L);
   else
      initglblstruc(index++, "tpwrf", &tpwrf, 0L);
   if ( P_getreal(CURRENT,"dpwrm",&dum,1) == 0 )
      initglblstruc(index++, "dpwrm", &dpwrf, 0L);
   else
      initglblstruc(index++, "dpwrf", &dpwrf, 0L);
   initglblstruc(index++, "dpwrm2", &dpwrf2, 0L);
   initglblstruc(index++, "dpwrm3", &dpwrf3, 0L);
   initglblstruc(index++, "temp", &vttemp, func4pad_temp);
   initglblstruc(index++, "dmf", &dmf, func4dmf);
   initglblstruc(index++, "dmf2", &dmf2, func4dmf3);
   initglblstruc(index++, "dmf3", &dmf3, func4dmf3);
   initglblstruc(index++, "dmf4", &dmf4, func4dmf4);
   initglblstruc(index++, "dseq", 0L, func4dseq);
   initglblstruc(index++, "dseq2", 0L, func4dseq2);
   initglblstruc(index++, "dseq3", 0L, func4dseq3);
   initglblstruc(index++, "dseq4", 0L, func4dseq4);
   initglblstruc(index++, "phase", 0L, func4phase1);
   initglblstruc(index++, "phase2", 0L, func4phase2);
   initglblstruc(index++, "phase3", 0L, func4phase3);

/* status control parameters */
   initglblstruc(index++, "dmm", 0L, func4dmm);
   initglblstruc(index++, "dm", 0L, func4dm);
   initglblstruc(index++, "dmm2", 0L, func4dmm2);
   initglblstruc(index++, "dm2", 0L, func4dm2);
   initglblstruc(index++, "dmm3", 0L, func4dmm3);
   initglblstruc(index++, "dm3", 0L, func4dm3);
   initglblstruc(index++, "dmm4", 0L, func4dmm4);
   initglblstruc(index++, "dm4", 0L, func4dm4);
   initglblstruc(index++, "hs", 0L, func4hs);
   initglblstruc(index++, "homo", 0L, func4homo);
   initglblstruc(index++, "homo2", 0L, func4homo2);
   initglblstruc(index++, "homo3", 0L, func4homo3);
   initglblstruc(index++, "homo4", 0L, func4homo4);

/* lock parameters */
   initglblstruc(index++, "lockpower", 0L, func4lkpwr);
   initglblstruc(index++, "lockgain", 0L, func4lkgain);
   initglblstruc(index++, "lockphase", 0L, func4lkphase);

   for (shim_index= Z0 + 1; shim_index < MAX_SHIMS; shim_index++)
   {
      const char *sh_name;
      if ((sh_name = get_shimname(shim_index)) != NULL)
          initglblstruc( index++, sh_name, 0L, func4shimdac);
   }

   initglblstruc(index++, "spin", 0L, func4spin); 	/* assume spinactive = true */
   initglblstruc(index++, "vtc", &vtc, 0L); 	     	/* VT cooling gas  setting */
   initglblstruc(index++, "loc", 0L, func4loc);
   initglblstruc(index++, "alock", 0L, func4alock);
   initglblstruc(index++, "wshim", 0L, func4wshim);
   initglblstruc(index++, "vtwait", &vtwait, 0L); 	/* VT timeout setting */

   initglblstruc(index++, "pwx", &pwx, 0L);
   initglblstruc(index++, "pwxlvl", &pwxlvl, 0L);
   initglblstruc(index++, "tau", &tau, 0L);
   initglblstruc(index++, "satdly", &satdly, 0L);
   initglblstruc(index++, "satfrq", &satfrq, 0L);
   initglblstruc(index++, "satpwr", &satpwr, 0L);
   initglblstruc(index++, "satmode", 0L, func4satmode);
   initglblstruc(index++, "gradalt", &gradalt, 0L);

   /* MAX is 135 at present, change MAXGLOBALS if needed */

}
