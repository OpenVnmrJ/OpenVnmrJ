/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "group.h"
#include "variables.h"
#include "acqparms2.h"
#include "pvars.h"
#include "shims.h"
#include "abort.h"
#include "cps.h"
#include "CSfuncs.h"
#ifndef PSG_LC
#define PSG_LC
#endif
#include "lc_gem.h"

#define DPRTLEVEL	1
#define MAXGLOBALS	125
#define MAXGVARLEN	20
#define MAXVAR		10
#define MAXLOOPS	20
#define MAXSTR		256

#define FALSE		0
#define TRUE		1
#define ERROR		1
#define NOTFOUND	-1

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

extern char   **cnames;		/* pointer array to variable names */

extern int      bgflag;
extern int	padflag;
extern int	rtinit_count;

extern double   cur_dpwrf, cur_tpwrf;
extern double **cvals;		/* pointer array to variable values */
extern double   exptime;

extern double  d2_init;         /* Initial value of d2 delay, used in 2D/3D/4D experiments */

extern double   preacqtime;

extern Acqparams *Alc;
extern autodata *Aauto;

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
   int	numvar;		  /* # of variables in loop element */
   int	numvals;	  /* # of values, vars must have same #, chk in go */
   int	vartype[MAXVAR];  /* Basic Type of variable, string, real */
   char	*lpvar[MAXVAR];	  /* pointers to variable name */
   char	*varaddr[MAXVAR]; /* pointers to where the variable's  value ptr goes */
   int	glblindex[MAXVAR];/* indexs of variable in to global structure */
};

typedef struct _loopelemt loopelemt;

static loopelemt *lpel[MAXLOOPS];/* An array of pointers to looping elements */

static int numberLoops = 0;
static void setup(char *varptr, int lpindex, int varindex);
static int srchglobal(char *name);

/*-----------------------------------------------------------------------
|  structure glblvar:
|	global variable information to update the variable if it has been
|	arrayed.
|	Information needed for an looping element:
|	1. The variable name.
|	2. The the address of the global variable, so that it can be updated
|	3. Function that is called, to handle variable that require some type
|	   of translation, or addition information that depends on this
|	   variable.
+------------------------------------------------------------------------*/
struct _glblvar
{
   char            gnam[MAXGVARLEN];
   double         *glblvalue;
   int             (*funcptr) ();
};
typedef struct _glblvar glblvar;

static glblvar  glblvars[MAXGLOBALS];

static int     func4alock();
static int     func4d2();
static int     func4dhp();
static int     func4dlp();
static int     func4dfrq();
static int     func4dm();
static int     func4dmf();
static int     func4dmm();
static int     func4dof();
static int     func4dpwrf();
static int     func4hs();
static int     func4lkgain();
static int     func4lkphase();
static int     func4lkpwr();
static int     func4loc();
static int     func4nt();
static int     func4pad_temp();
static int     func4rcgain();
static int     func4sfrq();
static int     func4shimdac();  /* !CHECK! */
static int     func4spin();
static int     func4tof();
static int     func4tpwrf();
static int     func4wshim();
static int     func4pplvl();
static void    initglblstruc();

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
+------------------------------------------------------------------*/
void initlpelements()
{
   int             i;

   for (i = 0; i < MAXLOOPS; i++)
      lpel[i] = (loopelemt *) 0;
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
#define NULLCHAR 0
#define RPRIN 0x29	/* right parenthesis */
#define LPRIN 0x28	/* left  parenthesis */
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
   if (bgflag) fprintf(stderr, "parse(): string: '%s' -----\n", string);
   /* ---  test the variables as we parse them --- */
   /* ---  This is a 4 state parser, 0-1: separate variables     */
   /* --- 			     2-4: diagonal set variables */
   while (1)
   {  switch (state)
      {  /* ---  start of variable name --- */
	 case 0:
	    if (bgflag)
	    {  fprintf(stderr, "Case 0: ");
	       if (*ptr == NULLCHAR)
	          fprintf(stderr, "letter: NULL, ");
               else
	          fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    if (letter(*ptr))	/* 1st letter go to state 1 */
	    {  varptr = ptr;
	       state = 1;
	       ptr++;
               *narrays += 1;
	    }
	    else
	    {  if (*ptr == LPRIN)	/* start of diagnal arrays */
	        { state = 2;
		  ptr++;
                  *narrays += 1;
	       }
	       else
	       {  if (*ptr == NULLCHAR) return (0); /* error? */
		  else		/* error */
		  {  text_error("Syntax error in variable array");
		     return (ERROR);
	    } } }
	    if (bgflag) fprintf(stderr, " state = %d \n", state);
	    break;
	    /* --- complete a single array variable till ',' --- */
	 case 1:
	    if (bgflag)
	    {  fprintf(stderr, "Case 1: ");
	       if (*ptr == NULLCHAR)
	          fprintf(stderr, "letter: NULL, ");
               else
	          fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    if (letter(*ptr) || digit(*ptr)) ptr++;
	    else
	    {  if (*ptr == COMMA)
	       {  *ptr = NULLCHAR;
		  setup(varptr, lpindex, varindex);
		  ptr++;
		  lpindex++;
		  state = 0;
	       }
	       else
	       {  if (*ptr == NULLCHAR)
		  {  setup(varptr, lpindex, varindex);
		     return (0);
		  }
		  else
		  {  text_error("Syntax Error in variable 'array'");
		     return (ERROR);
	    } } }
	    if (bgflag) fprintf(stderr, " state = %d \n", state);
	    break;
	    /* --- start of diagnal arrayed variables  'eg. (pw,d1)' --- */
	 case 2:
	    if (bgflag)
	    {  fprintf(stderr, "Case 2: ");
	       fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    /* if (letter1(*ptr)) */
	    if (letter(*ptr))
	    {  varptr = ptr;
	       state = 3;
	       ptr++;
	    }
	    else
	    {  text_error("Syntax Error in variable 'array'");
	       return (ERROR);
	    }
	    if (bgflag) fprintf(stderr, " state = %d \n", state);
	    break;
	    /* --- finish a diagonal arrayed variable  name --- */
	 case 3:
	    if (bgflag)
	    {  fprintf(stderr, "Case 3: ");
	       fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    if (letter(*ptr) || digit(*ptr))  ptr++;
	    else
	    {  if (*ptr == COMMA)
	       {  *ptr = NULLCHAR;
		  setup(varptr, lpindex, varindex);
		  ptr++;
		  varindex++;
		  state = 2;
	       }
	       else if (*ptr == RPRIN)
	       {  *ptr = NULLCHAR;
		  setup(varptr, lpindex, varindex);
		  ptr++;
		  varindex = 0; /* was == 0 8-9-90*/
		  state = 4;
	       }
	       else
	       {  text_error("Syntax Error in variable 'array'");
		  return (ERROR);
	       }
	    }
	    if (bgflag) fprintf(stderr, " state = %d \n", state);
	    break;
	    /* --- finish a diagonal arrayed variable  set --- */
	 case 4:
	    if (bgflag)
	    {  fprintf(stderr, "Case 4: ");
	       fprintf(stderr, "letter: '%c', ", *ptr);
	    }
	    if (*ptr == COMMA)
	    {  *ptr = NULLCHAR;
	       ptr++;
	       lpindex++;
	       varindex = 0;
	       state = 0;
	    }
	    else
	    {  if (*ptr == NULLCHAR) return (0);
	       else
	       {  text_error("Syntax Error in variable 'array'");
		  return (ERROR);
	       }
	    }
	    if (bgflag) fprintf(stderr, " state = %d \n", state);
	    break;
} } }
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
/* varptr	pointer to variable name */
/* lpindex	loop element index 0-# */
/* varindex	variable index 0-# (# of variable per loop element */
static void setup(char *varptr, int lpindex, int varindex)
{
int             index;
int             type;
int             ret;
char            mess[MAXSTR];
vInfo           varinfo;	/* variable information structure */
   /* --- variable info  --- */
   if ( (ret = P_getVarInfo(CURRENT, varptr, &varinfo)) )
   {  sprintf(mess, "Cannot find the variable: '%s'", varptr);
      text_error(mess);
      if (bgflag) P_err(ret, varptr, ": ");
      psg_abort(1);
   }
   type = varinfo.basicType;
   index = find(varptr);
   if (index == NOTFOUND)
   {  sprintf(mess, "variable '%s' does not exist.", varptr);
      text_error(mess);
      psg_abort(1);
   }
   if (bgflag)
   {  fprintf(stderr,
	      "SETUP: variable: '%s', lpindex = %d, varindex = %d, index = %d \n",
	      varptr, lpindex, varindex, index);
      if (type == ST_REAL)
      {
	 fprintf(stderr,
	  "SETUP: cname[%d]: '%s', cvals[%d] = %5.2lf \n",
	  index, cnames[index], index, *cvals[index]);
      }
      else
      {
	 fprintf(stderr,
	    "SETUP: cname[%d]: '%s', cvals[%d] = '%s' \n",
	   index, cnames[index], index, cvals[index]);
   }  }

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
   {  int gindx;
      gindx = lpel[lpindex]->glblindex[varindex];
      fprintf(stderr,
	"SETUP: #var = %d  var type: %d  var name: '%s',",
	 lpel[lpindex]->numvar, lpel[lpindex]->vartype[varindex],
	 lpel[lpindex]->lpvar[varindex]);
      if (type == ST_REAL)
      {  fprintf(stderr,
		" value = %5.2lf \n",
		 *(*((double **) lpel[lpindex]->varaddr[varindex])) );
      }
      else
      {  fprintf(stderr,
 		" value = '%s' \n",
		 *((char **) lpel[lpindex]->varaddr[varindex]) );
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

void arrayPS(index, numarrays)
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
         nth2D++;	/* if first arrayed variable (i.e. 2D variable) inc. */
/* --- for each of the arrayed variable(s) get its value --- */
      for (varindx = 0; varindx < nvars; varindx++)
      {
	 name = lpel[index]->lpvar[varindx];
	 parmptr = (char **) lpel[index]->varaddr[varindx];
         curarrayindex = valindx - 1;	/* for id2 */
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
	    fprintf(stderr,"Loop Element %d, Variable[%d] '%s', ",
			index, varindx, name);
	    if (lpel[index]->vartype[varindx] != T_STRING)
	    {
	       fprintf(stderr,"value[%d] = %10.4lf\n",valindx,(double) *temptr); 
	    }
	    else
	    {
	       fprintf(stderr,"value[%d] = '%s'\n",valindx, (char *) temptr);
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
         Alc->elemid = (unsigned long) ix;    /*  fid number */
         if (ix > 1)		/* else initialRF() acodes are over written */
         {  Codeptr = Aacode;	/* reset back to beginning of Code section */
            rtinit_count = 0;
         }
	 /* update lc elemid = ix */
	 meat();

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

/*-----------------------------------------------------------------
|	sign_add()/2
|  	 uses sign of first argument to decide to add or subtract 
|			second argument	to first
|	returns new value (double)
+------------------------------------------------------------------*/
double sign_add(double arg1, double arg2)
{
    if (arg1 >= 0.0)
	return(arg1 + arg2);
    else
	return(arg1 - arg2);
}

/*---------------------------------------------------
|  functions call for parameters effecting
|  frequencies .
+--------------------------------------------------*/
static
func4sfrq(value)
double 		value;
{
   sfrq = value;
}
static
func4dfrq(value)
double 		value;
{
   dfrq = value;
}
static
func4tof(value)
double 		value;
{
    tof = value;
}
static
func4dof(value)
double 		value;
{
   dof = value;
}

static
func4dmf(value)
double 		value;
{
    dmf = value;
}
/*---------------------------------------------------
|  functions call for integer parameters 
+--------------------------------------------------*/
static
func4spin(value)
double          value;
{
   spin = (int) sign_add(value,0.005);
   DPRINT(DPRTLEVEL,"func for spin called\n");
   return (0);
}
static
func4loc(value)
double          value;
{
  loc = (int) sign_add(value,0.005);
   DPRINT(DPRTLEVEL,"func for loc called\n");
   return (0);
}
static int func4d2(double value)
{
   short	*ptr;
   ptr = (short *)Alc;
   ptr = ptr + id2;
   if (getCSparIndex("d2") == -1)
      d2_index = get_curarrayindex();
   else
      d2_index = (int) ( (d2 - d2_init) / inc2D + 0.5);
   *ptr = (short) d2_index;
   DPRINT(DPRTLEVEL,"func for d2 to set id2\n");
   return (0);
}

static
func4dpwrf(value)
double		value;
{
   dpwrf = value;
   cur_dpwrf = value;
}

static
func4dhp(value)
double          value;
{
   dhp = value;
   DPRINT(DPRTLEVEL,"func for dhp to set dec atten \n");
   return (0);
}

static
func4dlp(value)
double          value;
{
   dlp = value;
   DPRINT(DPRTLEVEL,"func for dlp to set dec atten \n");
   return (0);
}
static
func4pplvl(value)
double	value;
{
   pplvl = value;
   DPRINT(DPRTLEVEL, "func for pplvl called\n");
   return (0);
}

static
func4tpwrf(value)
double		value;
{
   tpwrf = value;
   cur_tpwrf = value;
}

/*---------------------------------------------------
|  functions call for global double parameters effecting
|  low core or automation structure.
+--------------------------------------------------*/
static
func4nt(value)
double          value;
{
   nt = (long) (value + 0.005);
   Alc->nt = (long) (nt + 0.005);
   DPRINT1(DPRTLEVEL,"func for nt called, new value: %ld\n", nt);
   return (0);
}
static
func4pad_temp(value)
double          value;
{
   padflag = TRUE;
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
   mf_dmm = strlen(dmm);
   DPRINT1(DPRTLEVEL,"func for dmm called value='%s'\n", value);
   return (0);
}
static
func4dm(value)
char           *value;
{
   strcpy(dm, value);
   mf_dm = strlen(dm);
   DPRINT1(DPRTLEVEL,"func for dm called value='%s'\n", value);
   return (0);
}
static
func4hs(value)
char           *value;
{
   strcpy(hs, value);
   mf_hs = strlen(hs);
   DPRINT1(DPRTLEVEL,"func for hs called value='%s'\n", value);
   return (0);
}
/* static
 * func4homo(value)
 * char           *value;
 * {
 *    strcpy(homo, value);
 *    homosize = strlen(homo);
 *    DPRINT1(DPRTLEVEL,"func for homo called value='%s'\n", value);
 *    return (0);
 * }
 */
static
func4alock(value)
char           *value;
{
   strcpy(alock, value);
/*   getlockmode(alock,&lockmode); */
   DPRINT1(DPRTLEVEL,"func for alock called value='%s'\n", value);
   return (0);
}
static
func4wshim(value)
char           *value;
{
   strcpy(wshim, value);
/*   whenshim = setshimflag(wshim,&shimatanyfid); */ /* when to shim */
   DPRINT1(DPRTLEVEL,"func for wshim called value='%s'\n", value);
   return (0);
}

/*---------------------------------------------------
|  Auto structure parameter  functions
+--------------------------------------------------*/
static
func4lkpwr(value)
double          value;
{
   Aauto->lockpower = (codeint) sign_add(value, 0.005);
   return (0);
}
static
func4lkgain(value)
double          value;
{
   Aauto->lockgain = (codeint) sign_add(value, 0.005);
   return (0);
}
static
func4lkphase(value)
double          value;
{
   Aauto->lockphase = (codeint) sign_add(value, 0.005);
   return (0);
}
static
func4rcgain(value)
double          value;
{
   Aauto->recgain = (codeint) sign_add(gain,0.0005);
/*   gainactive = TRUE; */ /* non arrayable */
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
static void initglblstruc(index, name, glbladdr, function)
int             index;
char           *name;
double         *glbladdr;
int             (*function) ();

{
   if (index >= MAXGLOBALS)
   {
      fprintf(stdout, "initglblstruc: index: %d beyond limit %d\n",
		 index, MAXGLOBALS);
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
int elemvalues(int elem)
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
   if ( (ret = P_getVarInfo(CURRENT, name, &varinfo)) )
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
int elemindex(int fid, int elem)
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
void initglobalptrs()
{
   int index;
   int shim_index;
   double dum;

   index = 0;
   initglblstruc(index++, "d1", &d1, 0L);
   initglblstruc(index++, "d2", &d2, func4d2);
   initglblstruc(index++, "pw", &pw, 0L);
   initglblstruc(index++, "p1", &p1, 0L);
   initglblstruc(index++, "pad", &pad, func4pad_temp);
   initglblstruc(index++, "rof1", &rof1, 0L);
   initglblstruc(index++, "rof2", &rof2, 0L);
   initglblstruc(index++, "hst", &hst, 0L);
   initglblstruc(index++, "alfa", &alfa, 0L);
   initglblstruc(index++, "sw", &sw, 0L);
   initglblstruc(index++, "np", &np, 0L);
   initglblstruc(index++, "nt", &nt, func4nt);
   initglblstruc(index++, "sfrq", &sfrq, func4sfrq);
   initglblstruc(index++, "dfrq", &dfrq, func4dfrq);
   initglblstruc(index++, "fb", &fb, 0L);
   initglblstruc(index++, "bs", &bs, 0L);
   initglblstruc(index++, "tof", &tof, func4tof);
   initglblstruc(index++, "dof", &dof, func4dof);
   initglblstruc(index++, "gain", &gain, func4rcgain);
   initglblstruc(index++, "dlp", &dlp, func4dlp); 
   initglblstruc(index++, "dhp", &dhp, func4dhp);
   initglblstruc(index++, "tpwr", &tpwr, 0L);
   if ( P_getreal(CURRENT,"tpwrm",&dum,1) == 0 )
      initglblstruc(index++, "tpwrm", &tpwrf, func4tpwrf);
   else
      initglblstruc(index++, "tpwrf", &tpwrf, func4tpwrf);
   initglblstruc(index++, "dpwr", &dhp, 0L);
   if ( P_getreal(CURRENT,"dpwrm",&dum,1) == 0 )
      initglblstruc(index++, "dpwrm", &dpwrf, func4dpwrf);
   else
      initglblstruc(index++, "dpwrf", &dpwrf, func4dpwrf);
   initglblstruc(index++, "temp", &vttemp, func4pad_temp);
   initglblstruc(index++, "dmf", &dmf, func4dmf);
   initglblstruc(index++, "pplvl", &pplvl, func4pplvl);

/* status control parameters */
   initglblstruc(index++, "dmm", 0L, func4dmm);
   initglblstruc(index++, "dm", 0L, func4dm);
   initglblstruc(index++, "hs", 0L, func4hs);

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

   initglblstruc(index++, "spin", 0L, func4spin); /* assume spinactive = true */
   initglblstruc(index++, "vtc", &vtc, 0L);	/* VT cooling gas  setting */
   initglblstruc(index++, "loc", 0L, func4loc);
   initglblstruc(index++, "alock", 0L, func4alock);
   initglblstruc(index++, "wshim", 0L, func4wshim);
   initglblstruc(index++, "vtwait", &vtwait, 0L); 	/* VT timeout setting */
   initglblstruc(index++, "gradalt", &gradalt, 0L);
   initglblstruc(index++, "tau", &tau, 0L);

   /* MAX is 125 at present, change MAXGLOBALS if needed */

}

