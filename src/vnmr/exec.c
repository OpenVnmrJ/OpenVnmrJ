/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*----------------------------------------------------------------------------
|       exec.c
|
|       These routines are the meat of the interpreter.  It takes the
|       various execution trees built up by the interpreter and
|       executes them.  These codes handle all the operations, logic
|       decisions, etc.
+----------------------------------------------------------------------------*/
#include "vnmrsys.h"
#include "group.h"
#include "node.h"
#include "params.h"
#include "allocate.h"
#include "variables.h"
#ifdef UNIX
#include "magic.gram.h"
#else 
#include "magic_gram.h"
#endif 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"

char           *Dx_string = NULL; /* pointer to list of changed variable names */
extern char    *Wgetscreendisplay();
extern int      assignPair(pair *p, varInfo *v, int i);
extern int      appendPair(pair *d, pair *s);
extern void     cleanPair(pair *p);
extern void     tossTemps();
extern int      assignReal(double d, varInfo *v, int i);
extern int      assignString(const char *s, varInfo *v, int i);
extern int    (*builtin())();
extern int      checkParm(pair *p, varInfo *v, char *name);
extern int      Eflag;
#ifdef VNMRJ
extern int	jShowArray;
extern void     jExpressionError();
extern void     appendJvarlist(const char *name);
#endif 
extern int do_EQ(pair *a, pair *b, pair *c);
extern int do_NE(pair *a, pair *b, pair *c);
extern int do_LT(pair *a, pair *b, pair *c);
extern int do_GT(pair *a, pair *b, pair *c);
extern int do_LE(pair *a, pair *b, pair *c);
extern int do_GE(pair *a, pair *b, pair *c);
extern int do_PLUS(pair *a, pair *b, pair *c);
extern int do_MINUS(pair *a, pair *b, pair *c);
extern int do_MULT(pair *a, pair *b, pair *c);
extern int do_DIV(pair *a, pair *b, pair *c);
extern int do_MOD(pair *a, pair *b, pair *c);
extern int do_NEG(pair *a, pair *b);
extern int do_TRUNC(pair *a, pair *b);
extern int do_SQRT(pair *a, pair *b);
extern int do_NOT(pair *a, pair *b);
extern int do_AND(pair *a, pair *b, pair *c);
extern int do_OR(pair *a, pair *b, pair *c);
extern int do_SIZE(pair *a, pair *b);
extern int do_TYPEOF(pair *a, pair *b);

extern int      isActive();
extern int      listLength();
extern int      valueOf();
extern int      execPError;
extern node    *findMacro();
extern node    *loadMacro();
extern Rval    *newRval();
extern void     popTree();
extern void     pushTree();
extern void     showTree();
extern void     showValue();
extern int run_calcdim();
extern void saveMacro(char *n, node *p, int i);
extern void Wclearerr();
extern void check_datastation();
extern void revalidatePrintCommand(char *name);
extern void printRvals2(FILE *f, int b, Rval *r);
extern void  clearVar(char *name);
extern void startExecGraphFunc(char *cmd, int reexec, int redo);
extern void saveExecGraphFunc(char *name, int redo, int argc, char *argv[]);
extern void endExecGraphFunc(char *name, int reexec, int redo, int status);
extern void doGraphOff(char *);

static int      ignoreBomb;
static int      mustReturn;
node           *doingNode;
int             M0flag = 0;  /* debugging flag for failed commands */
int             Mflag = 0; /* debugging flag for tracing macros and commands */
int             M1flag = 0;  /* debugging flag for macros only */
int             M2flag = 0; /* debugging flag for tracing macros and commands */
int             M3flag = 0; /* debugging flag for tracing macros and commands */
int             MflagVJcmd=0;
int             MflagIndent=0;
static int      execStringRes = 0;
static int      execLevel = 0;
static int      _L_ = 0;  // Line number of executing macro

static int cmCnt = -1; /* number of commas in the CMLIST element */
static int cmIter = 0; /* loop count through CMLIST elements */
static int cmList = 0; /* test to avoid multiple CMLIST elements in a single expression */
static int cmUsed = 0; /* test to avoid CM and CMLIST in a single expression */

static int execL( node *, varInfo **, char **, int *, int *, char * );
static int editpar( char [], char *, int, int );
static int is_print_safe_macro(char *);
static double real_unit( char [], double );
extern char *buildValue();
extern void printRvals(FILE *f, int index, char *sep, int b, Rval *r);
void checkarray(char *, short);
int execR(node *n, pair *p, char *macroName);
int execS(node *n, char *macroName);

#define SCROLL 4

#ifdef  DEBUG
extern int      Dflag;
#define DPRINT(level, str) \
	if (Dflag >= level) fprintf(stderr,str)
#define DPRINT1(level, str, arg1) \
	if (Dflag >= level) fprintf(stderr,str,arg1)
#define DPRINT2(level, str, arg1, arg2) \
	if (Dflag >= level) fprintf(stderr,str,arg1,arg2)
#define DPRINT3(level, str, arg1, arg2, arg3) \
	if (Dflag >= level) fprintf(stderr,str,arg1,arg2,arg3)
#define DPRINT4(level, str, arg1, arg2, arg3, arg4) \
	if (Dflag >= level) fprintf(stderr,str,arg1,arg2,arg3,arg4)
#define DSHOWFLAVOUR(level, arg1) \
	if (Dflag >= level) showFlavour(stderr,arg1)
#define DSHOWPAIR(level, arg1, arg2) \
	if (Dflag >= level) showPair(stderr,arg1,arg2)
#define DSHOWTREE(level, arg1, arg2, arg3) \
	if (Dflag >= level) showTree(arg1,arg2,arg3)
#define DSHOWVAR(level, arg1, arg2) \
	if (Dflag >= level) showVar(stderr,arg1,arg2)
#else 
#define DPRINT(level, str) 
#define DPRINT1(level, str, arg2) 
#define DPRINT2(level, str, arg1, arg2) 
#define DPRINT3(level, str, arg1, arg2, arg3) 
#define DPRINT4(level, str, arg1, arg2, arg3, arg4) 
#define DSHOWFLAVOUR(level, arg1)
#define DSHOWPAIR(level, arg1, arg2) 
#define DSHOWTREE(level, arg1, arg2, arg3)
#define DSHOWVAR(level, arg1, arg2)
#endif 

static int   isBgPrint = 0; /* in print screen mode (background process)  */
static int   saveArgs = 0;  // save command and arguments
static int   redoType = 0;
static char parentMacro[MAXSTR];

char *getParentMacro() {
   return parentMacro;
}

/*  Called when a command is cancelled */
void dontIgnoreBomb()
{
   ignoreBomb = 0;
}

void dontSaveArgs()
{
   saveArgs = 0;
}

void setPrintScrnMode(int s)
{
     isBgPrint = s;
}

/*------------------------------------------------------------------------------
|
|       isTrue/1
|
|       This function returns true (non-zero) if the given pair represents
|       a non-zero REAL value or a STRING of non-zero length.  Otherwise
|       this function returns false (zero).
|
+-----------------------------------------------------------------------------*/

int isTrue(pair *p)
{   Rval *r;

    if (p)
    {   if ( (r=p->R) )
        {   while (r)
            {   switch (p->T.basicType)
                { case T_UNDEF:     break;
                  case T_REAL:      if (r->v.r != 0.0)
                                        return(1);
                                    break;
                  case T_STRING:    if (0 < strlen(r->v.s))
                                        return(1);
                                    break;
                }
                r = r->next;
            }
            return(0);
        }
        else
            return(0);
    }
    else
        return(0);
}

void setExecLevel(int n)
{
    execLevel = n;
}


/*------------------------------------------------------------------------------
|
|       appendvarlist
|
|       This procedure maintains the list of parameters which have been assigned
|
+-----------------------------------------------------------------------------*/

void appendvarlist(const char *name)
{
    char str[MAXSTR];

    if (Dx_string == NULL)
    {
        sprintf(str,"'%s'",name);
        Dx_string = newString(str);
#ifdef VNMRJ
        appendJvarlist(name);
#endif 
    }
    else
    {
        sprintf(str,",'%s',",name);
        if (strstr(Dx_string,str) == NULL)
        {
            str[strlen(str)-1] = '\0'; // remove trailing comma
            Dx_string = newCatId(Dx_string,str,"appendvarlist");
#ifdef VNMRJ
            appendJvarlist(name);
#endif 
        }
    }
}

/*------------------------------------------------------------------------------
|
|       procArgv/4
|
|       This procedure traverses an arg list (CM tree) evaluating each
|       item and converting the result to strings which are placed in the
|       argv list (as req'd).
|
+-----------------------------------------------------------------------------*/

int procArgv(int i, node *n, int argc, char *argv[], char *macroName)
{   int      j,k;

    DPRINT2(3,"procArgv(%d): of %d...\n",i,argc);
    DSHOWTREE(3,0,"procArgv(_):    ",n);
    if (n)
    {   if (n->flavour == CM)
        {
            DPRINT1(3,"procArgv(%d): ...CM sublist, to left...\n",i);
            if ((j = procArgv(i,n->Lson,argc,argv,macroName)) == 0)
                return(j);
            DPRINT1(3,"procArgv(%d): ...to right...\n",j);
            if ((k = procArgv(j,n->Lson->Rbro,argc,argv,macroName)) == 0)
                return(k);
            DPRINT2(3,"procArgv(%d): ...finished at %d\n",i,k);
            return(k);
        }
        else
        {   pair p;

            DPRINT1(3,"procArgv(%d): ...item or expression...\n",i);
            if (execR(n,&p, macroName))
            {
                DPRINT1(3,"procArgv(%d):    source is...\n",i);
                DSHOWPAIR(3,"procArgv(_): ",&p);
                if ((0 <= i) && (i < argc))
                {   switch (p.T.basicType)
                    { case T_UNDEF:     argv[i] = newString("");
                                        break;
                      case T_REAL:      argv[i] = realString(p.R->v.r);
                                        break;
                      case T_STRING:    argv[i]  = p.R->v.s;
                                        p.R->v.s = NULL;
                                        break;
                    }
                    DPRINT2(3,"procArgv: argv[%d] = \"%s\"\n",i,argv[i]);
                }
                else
                    DPRINT2(3,"procArgv: %d outside range (0..%d)\n",i,argc-1);
            }
            else
                i = -1; /* so returns 0 */
            cleanPair(&p);
            return(i+1);
        }
    }
    else
        return(i);
}

/*------------------------------------------------------------------------------
|
|       procRetv/4
|
|       This function is used to assign the return values from a builtin
|       procedure to appropriate locations specified by a list of Lvals.
|
+-----------------------------------------------------------------------------*/

int procRetv(int i, node *n, int retc, char *retv[], char *macroName)
{   int      index,j,k;
    varInfo *v;

    DPRINT2(3,"procRetv: %d of %d...\n",i,retc);
    DSHOWTREE(3,0,"procRetv:    ",n);
    if (n)
    {   if (n->flavour == CM)
        {
            DPRINT1(3,"procRetv: ...CM sublist, %d to left...\n",i);
            j = procRetv(i,n->Lson,retc,retv, macroName);
            DPRINT1(3,"procRetv:                %d to right...\n",j);
            k = procRetv(j,n->Lson->Rbro,retc,retv, macroName);
            DPRINT1(3,"procRetv: ...finished at %d\n",k);
            return(k);
        }
        else
        {   char *name;
            int   shouldFree;

            DPRINT1(3,"procRetv: ...item %d...\n",i);
            if (execL(n,&v,&name,&index,&shouldFree, macroName))
            {
                DPRINT2(3,"procRetv: target is %s[%d]\n",name,index);
                if ((0 <= i) && (i < retc))
                {   if (retv[i])
                    {
#ifdef VNMRJXXX
			int diffval = 1;
			if ((name[0] != '$') && (name[0] != '%'))
			{
			    initStoredAttr(v);
			    diffval = compareValue(retv[i],v,index);
			}
#endif 
                        DPRINT2(3,"procRetv: retv[%d] was \"%s\"\n",i,retv[i]);
                        switch (v->T.basicType)
                        { case T_UNDEF:     if (isReal(retv[i]))
                                                assignReal(stringReal(retv[i]),v,index);
                                            else
                                                assignString(retv[i],v,index);
                                            break;
                          case T_REAL:      if (isReal(retv[i]))
                                                assignReal(stringReal(retv[i]),v,index);
                                            else
					    {
#ifdef VNMRJXXX
						diffval = 0;
#endif 
                                                if (name != NULL)
                                                   WerrprintfWithPos("%s expected REAL return value", name);
                                                else
                                                   WerrprintfWithPos("Expected REAL return value");
                                                if (macroName)
        					   WerrprintfLine3Only("Error macro:  %s,  line %d",
                                                          macroName, n->location.line);
                                            }
                                            break;
                          case T_STRING:    assignString(retv[i],v,index);
                                            break;
                        }
			if ((name[0] != '$') && (name[0] != '%'))
			{
#ifdef VNMRJXXX
			    if ((diffval==1) || (compareStoredAttr(v)==1))
#endif 
			        appendvarlist( name );
			}
                    }
                    else
                        DPRINT1(3,"procRetv: ...retv[%d] was NULL\n",i);
                }
                else
                    DPRINT1(3,"procRetv: ...retv[%d] is non-existant\n",i);
            }
            else
            {
                i = -1;
            }
            if (shouldFree)
            {
                DPRINT2(3,"procRetv: releasing name %s at 0x%08x\n",name,name);
                release(name);
            }
            return(i+1);
        }
    }
    else
        return(i);
}

int getMacroLine()
{
   return _L_;
}

/*------------------------------------------------------------------------------
|
|       execP/3
|
|       This procedure is used to set-up and call a procedure.  Note that both
|       the argv and retv blocks and any attached strings are disposed of here.
|       ALL STRINGS PASSED BACK IN retv MUST BE ALLOCATED OR EVERYTHING WILL
|       FALL APART!
|
+-----------------------------------------------------------------------------*/

static int execP(char *callname, char *n, int (*p)(), node *a, node *r, char *macroName, int macroLine)
{   char **argv;
    char **retv;
    int    argc;
    int    gives;
    int    i;
    int    retc;

    DPRINT(3,"execP: arg tree...\n");
    DSHOWTREE(3,0,"execP:    ",a);
    DPRINT(3,"execP: ...ret tree...\n");
    DSHOWTREE(3,0,"execP:    ",r);
    argc = listLength(a)+1;  /* calculate number of arguments */
    if ( (argv=(char **)allocateWithId((argc+1)*sizeof(char *),"execP/argv")) )
    {   for (i=0; i<argc+1; ++i)  /* null out pointers */
                argv[i] = NULL;
        argv[0] = newString(n);
        retc    = listLength(r);
        if ( (retv=(char **)allocateWithId((retc+1)*sizeof(char *),"execP/retv")) )
        {   int i;
            int ret;

            DPRINT1(3,"execP: ...process %d arg(s)...\n",argc);
            ret = procArgv(1,a,argc,argv, macroName);
            for (i=0; i<retc; ++i)
                retv[i] = NULL;
#ifdef      DEBUG
            if (3 <= Dflag)
                if (ret)
                    DPRINT(3,"execP arguments processed correctly\n");
                else
                    DPRINT(3,"execP arguments could not be processed \n");
            if (3 <= Dflag && ret)
                for (i=0; i<argc; ++i)
                    if (argv[i])
                        DPRINT2(3,"execP:  argv[%d] = \"%s\"\n",i,argv[i]);
                    else
                        DPRINT1(3,"execP:    argv[%d] = NULL\n",i);
            if (3 <= Dflag)
                if (ret)
                    DPRINT2(3,"execP: ...call procedure %d arg(s) upto %d returns\n",argc,retc);
                else
                    DPRINT(3,"execP: ... abandon procedure, bad arguments\n");
#endif 
            if (ret)
            {
                int showP = 0;
                if (M2flag && M1flag)
                {
                  showP = (MflagVJcmd || ( strcmp(n,"jFunc") && strcmp(n,"jMove") && strcmp(n,"jEvent") && strcmp(n,"vnmrjcmd")) );
                  if (showP)
                  {
                     int args;
                     fflush(stdout);
                     fflush(stderr);
                     fprintf(stderr,"%*scommand %s",MflagIndent,"",callname);
                     if (argc)
                        for (args=1; args < argc; args++)
                           fprintf(stderr,"%s'%s'",(args==1) ? "(" : ",",
                                (argv[args] == NULL) ? "null" : argv[args]);
                 
                     fprintf(stderr,"%s started from %s:%d\n", (argc>1) ? ")" : "",macroName,macroLine);
                     fflush(stderr);
                     MflagIndent++;
                  }
                }
                _L_ = macroLine;
                gives = !((*p)(argc,argv,retc,retv));
                DPRINT1(3,"execP: ...back from procedure, process upto %d returns...\n",retc);

#ifdef VNMRJ
                execLevel--;
                if (execLevel < 0)
                    execLevel = 0;
                if (saveArgs && (execLevel == 0))
                {
                   saveExecGraphFunc(callname, redoType, argc, argv); 
                   saveArgs = 0;
                }
#endif

                if (M2flag && M1flag && showP)
                {
                  int args;
                  if (MflagIndent)
                     MflagIndent--;
                  fflush(stdout);
                  fflush(stderr);
                  if (gives)
                  {
                      fprintf(stderr,"%*scommand %s",MflagIndent,"",callname);
                      if (retc)
                     for (args=0; args < retc; args++)
                        fprintf(stderr,"%s\"%s\"",(args==0) ? ":" : ",",
                                (retv[args] == NULL) ? "null" : retv[args]);
                      fprintf(stderr," returned\n");
                  }
                  else
                  {
                      fprintf(stderr,"%*scommand %s aborted\n",
                              MflagIndent,"",n);
                  }
                  fflush(stderr);
                }

                procRetv(0,r,retc,retv, macroName);
            }
            else
            {
               if (M2flag && M1flag)
               {
                  fprintf(stderr,"%*scommand %s failed\n",MflagIndent,"",n);
               }
               gives = 0;
            }
            DPRINT(3,"execP: ...release argv's...\n");
            for (i=0; i<argc; ++i)
                if (argv[i])
                    release(argv[i]);
            release(argv);
            DPRINT(3,"execP: ...release retv's...\n");
            for (i=0; i<retc; ++i)
                if (retv[i])
                    release(retv[i]);
            release(retv);
            DPRINT1(3,"execP: ...DONE (gives %d)\n",gives);
            return(gives);
        }
        else
        {   fprintf(stderr,"FATAL! out of memory\n");
            return(0);
        }
    }
    else
    {   fprintf(stderr,"FATAL! out of memory\n");
        return(0);
    }
}

/*------------------------------------------------------------------------------
|
|       macArgvRetv/2
|
|       This function is used to process arguments passed as a list of
|       expression trees.  These are evaluated and saved as positional temp
|       variables (i.e. %1, %2, ...).  This allows an arbitrary number of
|       expressions to be evaluated in a local symbol space and with the
|       resulting values availible for later use.
|
+-----------------------------------------------------------------------------*/

static int macArgvRetv(int i, node *n, int argc, char *macroName)
{   if (n)
    {   if (n->flavour == CM)
        {   int j,k;

            DPRINT1(3,"macArgvRetv(%d): ...a comma, go left\n",i);
            if ( (j=macArgvRetv(i,n->Lson,argc, macroName)) )
            {
                DPRINT2(3,"macArgvRetv(%d): ...then right from %d\n",i,j);
                if ( (k=macArgvRetv(j,n->Lson->Rbro,argc, macroName)) )
                {
                    DPRINT2(3,"macArgvRetv(%d): ...done at %d\n",i,k);
                    return(k);
                }
                else
                    return(0);
            }
            else
                return(0);
        }
        else
        {   int  give;
            pair p;

            DPRINT1(3,"macArgvRetv(%d): ...an item or expression\n",i);
            DPRINT2(3,"macArgvRetv(%d):    temPair at 0x%08x\n",i,&p);
            if (execR(n,&p, macroName))
            {   char     tmp[32];
                varInfo *v;

                sprintf(tmp,"%%%d",i);
                if ( (v=createVar(tmp)) )
                {   assignPair(&p,v,0);
                    DSHOWVAR(3,"macArgvRetv(_):    ",v);
                    if (M2flag && argc)
                       printRvals(stderr,i,"(",v->T.basicType,v->R);
                    give = i+1;
                }
                else
                    give = 0;
            }
            else
                give = 0;
            cleanPair(&p);
            return(give);
        }
    }
    else
        return(i);
}

/*------------------------------------------------------------------------------
|
|       placeMacReturns/3
|
|       Retrieve return values from temp symbol table, and place them at proper
|       Lvals (given as list).
|
+-----------------------------------------------------------------------------*/
int placeMacReturns(int i, node *n, char *macroName)
{   if (n)
    {   if (n->flavour == CM)
        {   int j,k;

            DPRINT1(3,"placeMacReturns(%d): ...a comma, go left\n",i);
            if ( (j=placeMacReturns(i,n->Lson, macroName)) )
            {
                DPRINT2(3,"placeMacReturns(%d): ...then right from %d\n",i,j);
                if ( (k=placeMacReturns(j,n->Lson->Rbro, macroName)) )
                {
                    DPRINT2(3,"placeMacReturns(%d): ...done at %d\n",i,k);
                    return(k);
                }
                else
                    return(0);
            }
            else
                return(0);
        }
        else
        {   char     tmp[32];
            int      give;
            varInfo *Sv;

            sprintf(tmp,"%%%d",i);
            DPRINT2(3,"placeMacReturn(%d): ...a target for \"%s\"\n",i,tmp);
            if ( (Sv=findVar(tmp)) )
            {   char    *name;
                int      index;
                int      shouldFree;
                varInfo *Dv;

                DPRINT2(3,"placeMacReturn(%d):    there is a \"%s\" (source)\n",i,tmp);
                if (execL(n,&Dv,&name,&index,&shouldFree, macroName))
                {   pair p;

                    DPRINT2(3,"placeMacReturn(%d): ...temPair at 0x%08x\n",i,&p);
                    DPRINT3(3,"placeMacReturn(%d):    there is a \"%s\"[%d] (dest)\n",i,name,index);
                    if (valueOf(Sv,0,&p))
                    {
#ifdef VNMRJXXX
			int diffval = 1;
			if ((name[0] != '$') && (name[0] != '%'))
			{
			   initStoredAttr(Dv);
			   diffval = comparePair(&p,Dv,index);
			}
#endif 
                        DPRINT1(3,"placeMacReturn(%d):    ...complete with a value\n",i);
                        if (assignPair(&p,Dv,index))
			{
                           give = i+1;
                           if (M2flag)
                              printRvals(stderr,i,":",Dv->T.basicType,Dv->R);
			   if ((name[0] != '$') && (name[0] != '%'))
			   {
#ifdef VNMRJXXX
			       if ((diffval == 1) || (compareStoredAttr(Dv)==1))
#endif 
			           appendvarlist( name );
			   }
			}
                        else
                        {
                           if (macroName)
                              WerrprintfLine3Only("Error macro:  %s,  line %d",
                                                  macroName, n->location.line);
                           give = 0;
                        }
                    }
                    else
                    {
                        DPRINT1(3,"placeMacReturn(%d):    ...but no value\n",i);
                        give = 0;
                    }
                    cleanPair(&p);
                }
                else
                    give = 0;
                if (shouldFree)
                {
                    DPRINT3(3,"placeMacReturn(%d): ...releasing name %s (at 0x%08x)\n",i,name,name);
                    release(name);
                }
            }
            else
            {
                DPRINT1(3,"placeMacReturn(%d):    nothing to give!\n",i);
                give = 0;
            }
            return(give);
        }
    }
    return(0);
}

/*------------------------------------------------------------------------------
|
|       execM/3
|
|       This procedure is used to set-up and call a macro.
|
+-----------------------------------------------------------------------------*/

static int execM(char *n, node *a, node *r, char *macroName, int macroLine)
{   varInfo *v;
    int argc;

    DPRINT(3,"execM: arg tree...\n");
    DSHOWTREE(3,0,"execM:    ",a);
    DPRINT(3,"execM: ...ret tree...\n");
    DSHOWTREE(3,0,"execM:    ",r);
    if ( (v=createVar("%#")) )
    {
        argc = listLength(a);
        assignReal((double) argc,v,0);
        v=createVar("%##");
        assignReal((double)listLength(r),v,0);
        if ( (v=createVar("%0")) )
        {   assignString(n,v,0);
            if (M2flag)
            {
               fflush(stdout);
               fflush(stderr);
               fprintf(stderr,"%*smacro %s",MflagIndent,"",n);
               MflagIndent++;
            }
            if (macArgvRetv(1,a,argc,macroName))
            {
                int   give;
                node *codeTree;
                int   res;

                if (M2flag)
                {
                   fprintf(stderr,"%s started from %s:%d\n", (argc>0) ? ")" : "",macroName,macroLine);
                }
                pushTree();
                if ((codeTree=findMacro(n)) == NULL)
                {
                    DPRINT(3,"execM: ...not in table, try to load\n");
                    if ( (codeTree=loadMacro(n,SEARCH, &res)) )
                    {
                        DPRINT(3,"execM: ...loaded, cache it\n");
                        saveMacro(n,codeTree,EXTENT_OF_CMD);
                        DPRINT(3,"execM: ...cached\n");
                    }
                }
                if (codeTree)
                {
                    DPRINT(3,"execM: ...execute it...\n");
                    give = execS(codeTree, n);
                    DPRINT1(3,"execM ...finished, gave %d\n",give);
                }
                else
                {
                    DPRINT(3,"execM: ...nothing to execute, give up\n");
                    if (macroName)
                       WerrprintfLine3Only("Error macro:  %s,  line %d", macroName, macroLine);
                    give = (res == 0);
                }
                popTree();
                DPRINT(3,"execM: ...popped to calling space, emplace return values\n");
                if (M2flag)
                {
                   if (MflagIndent)
                      MflagIndent--;
                   fprintf(stderr,"%*smacro %s",MflagIndent,"",n);
                }
                placeMacReturns(1,r,macroName);
                if (M2flag)
                {
                   fprintf(stderr," %s\n",(give) ? "returned" : "aborted");
                   fflush(stdout);
                   fflush(stderr);
                }
                tossTemps();
                DPRINT1(3,"execM: ...ALL DONE (gives %d)\n",give);
                return(give);
            }
            else
            {   pushTree();
                popTree();
                DPRINT(3,"execM: ...DIED\n");
                return(0);
            }
        }
        else
        {   WerrprintfWithPos("Can't create argument variable $0");
            return(0);
        }
    }
    else
    {   WerrprintfWithPos("Can't create argument variable $#");
        return(0);
    }
}

/*------------------------------------------------------------------------------
|
|       execMP/3
|
|       This procedure uses builtin/1 to determine whether a call is to a
|       procedure or macro then calls either execP or execM accordingly.
|
+-----------------------------------------------------------------------------*/

static int execMP(char *n, node *a, node *r, char *macroName, int macroLine)
{ 
    extern int  interuption;
    int give;
    int graphcmd, graphtype;
    int oldIgnoreBomb;
    int (*p)();
    char *newname;

    if(macroName) strcpy(parentMacro,macroName);

    DPRINT(3,"execMP: return mode set to OFF...\n");
    oldIgnoreBomb = ignoreBomb;
    ignoreBomb    = 0;
    mustReturn    = 0;
    if (execLevel == 0)
        saveArgs = 0;
    if ( (p=builtin(n,&newname, &graphcmd, &graphtype)) )
    {
        DPRINT1(3,"execMP: ...\"%s\" is builtin, assume procedure\n",n);
#ifdef VNMRJ
        if (isBgPrint != 0) {
            if (graphtype <= 0)
                RETURN;
        }
        else {
            redoType = graphtype;
            if (graphtype > 0 && graphtype < 10)
            {
               startExecGraphFunc(n, graphcmd, graphtype);
               if (graphtype >= 5 && graphtype <= 7) {
                   if (execLevel == 0)
                       saveArgs = 1;
               }
            }
        }
        if (graphtype != 12) // not exec command
            execLevel++;
#endif
        if (Mflag)
        {
           int showP;

           showP = (MflagVJcmd || ( strcmp(n,"jFunc") && strcmp(n,"jMove") && strcmp(n,"jEvent") && strcmp(n,"vnmrjcmd")) );
           if (showP)
           {
              fflush(stdout);
              fflush(stderr);
              fprintf(stderr,"%*scommand %s started from %s:%d\n",MflagIndent,"",n,macroName,macroLine);
              MflagIndent++;
           }
           give = execP(n,newname,p,a,r,macroName,macroLine);
           if (showP)
           {
              if (MflagIndent)
                 MflagIndent--;
              fprintf(stderr,"%*scommand %s %s\n",MflagIndent,"",n,(give) ? "returned" : "aborted");
              fflush(stdout);
              fflush(stderr);
           }
        }
        else
        {
           give = execP(n,newname,p,a,r,macroName,macroLine);
        }
        if ( ! give && macroName && (Mflag || M2flag || M3flag || M0flag) )
        {
           WerrprintfLine3Only("Error macro:  %s,  line %d", macroName, macroLine);
        }
#ifdef VNMRJ
        if (isBgPrint == 0) {
            if (graphtype > 0 && graphtype < 10)
                endExecGraphFunc(n, graphcmd, graphtype, give);
        }
        else
            revalidatePrintCommand(n);  //  for print screen
#endif
    }
    else
    {
        if (isBgPrint != 0) {
            if (is_print_safe_macro(n) == 0)
                RETURN;
        }
        DPRINT1(3,"execMP: ...\"%s\" is not builtin, assume macro\n",n);
        if (Mflag)
        {
           fflush(stdout);
           fflush(stderr);
           fprintf(stderr,"%*smacro %s started from %s:%d\n",MflagIndent,"",n,macroName,macroLine);
           MflagIndent++;
           give = execM(n,a,r,macroName,macroLine);
           if (MflagIndent)
              MflagIndent--;
           fprintf(stderr,"%*smacro %s %s\n",MflagIndent,"",n,(give) ? "returned" : "aborted");
           fflush(stdout);
           fflush(stderr);
        }
        else
        {
           give = execM(n,a,r,macroName,macroLine);
        }
	doGraphOff(n);
    }
    DPRINT(3,"execMP: ...return mode reset to OFF\n");
    DPRINT1(3,"execMP: ...gives %d\n",give);
    if ( ! interuption)
      ignoreBomb = oldIgnoreBomb;
    mustReturn = 0;
    return(give);
}

/*------------------------------------------------------------------------------
|
|       execN/2
|
|       This function is used to evaluate a node for a name.  The flag passed
|       (via call by ref) indicates if the string passed back should be released
|       when not needed anymore (was allocated here!).
|
+-----------------------------------------------------------------------------*/

char *execN(node *n, int *shouldFree, char *macroName)
{   char *name = NULL;
    node *oldNode;
    int   gotError;

    oldNode   = doingNode;
    doingNode = n;
    gotError = 0;
    switch (n->flavour)
    { case DOLLAR:  {   char *tmp;
                        pair  p;

                        DPRINT1(3,"execN: DOLLAR, temPair at 0x%08x\n",&p);
                        if (execR(n->Lson,&p, macroName))
                            if (p.T.size == 1)
                            {   switch (p.T.basicType)
                                { case T_UNDEF:     tmp         = NULL;
                                                    *shouldFree = 0;
                                                    break;
                                  case T_REAL:      tmp         = intString((int)(p.R->v.r));
                                                    *shouldFree = 1;
                                                    break;
                                  case T_STRING:    tmp         = newString(p.R->v.s);
                                                    *shouldFree = 1;
                                                    break;
                                  default:          tmp = NULL;
                                                    *shouldFree = 0;
                                                    break;
                                }
                            }
                            else
                            {   WerrprintfWithPos("${...} only applies to scalar values");
                                tmp         = NULL;
                                *shouldFree = 0;
                                gotError = 1;
                            }
                        else
                        {   tmp         = NULL;
                            *shouldFree = 0;
                        }
                        cleanPair(&p);
                        if (tmp)
                        {   name = newCatIdDontTouch("$",tmp,"execN");
                            if (*shouldFree)
                                release(tmp);
                            *shouldFree = 1;
                        }
                        else
                            name = NULL;
                    }
                    break;
      case LC:      {   pair p;

                        DPRINT1(3,"execN: LC, temPair at 0x%08x\n",&p);
                        execStringRes = 0;
                        if (execR(n->Lson,&p, macroName))
                            if (p.T.size == 1)
                            {
                                switch (p.T.basicType)
                                { case T_UNDEF:     name        = NULL;
                                                    *shouldFree = 0;
                                		    gotError = 1;
                                                    break;
                                  case T_REAL:      WerrprintfWithPos("{...} only applies to string expressions");
                                                    name        = NULL;
                                                    *shouldFree = 0;
                                		    gotError = 1;
                                                    break;
                                  case T_STRING:    name        = newString(p.R->v.s);
                                                    *shouldFree = 1;
                                                    if (! goodName(name))
                                                    {
                                                       if (strlen(name))
                                                       {
                                                          name = newCatId(name,"\n","execN");
                                                          execStringRes = (execString(name) == 0);
                                                          gotError = (execStringRes == 0);
                                                       }
                                                       else
                                                       {
                                		          gotError = 1;
                                                       }
                                                       release(name);
                                                       name        = NULL;
                                                       *shouldFree = 0;
                                                    }
                                                    break;
                                }
                            }
                            else
                            {   WerrprintfWithPos("{...} only applies to scalar string values");
                                name        = NULL;
                                *shouldFree = 0;
 			        gotError = 1;
                            }
                        else
                        {   name        = NULL;
                            *shouldFree = 0;
                            DPRINT(3,"execN: ...{} construct, no name\n");
                        }
                        cleanPair(&p);
                    }
                    break;
      case _NAME:   name        = n->v.s;
                    *shouldFree = 0;
                    break;
      default:      fprintf(stderr,"BUG! bad flavour noticed in exec(N) (=%d)\n",n->flavour);
                    name        = NULL;
                    *shouldFree = 0;
    }
    if (name)
    {   if (! goodName(name))
        {   WerrprintfWithPos("\"%s\" is not a valid variable name",name);
            if (*shouldFree)
            {   release(name);
                name        = NULL;
                *shouldFree = 0;
            }
        }
    }
    if (gotError && macroName) {
        WerrprintfLine3Only("Error macro:  %s,  line %d", macroName, n->location.line);
    }
#ifdef DEBUG
    if (3 <= Dflag)
    {   if (name)
            DPRINT1(3,"execN: ...name is \"%s\"",name);
        else
            DPRINT(3,"execN: ...no name");
        if (*shouldFree)
            DPRINT(3,", allocated\n");
        else
            DPRINT(3,", NOT allocated\n");
    }
#endif 
    doingNode = oldNode;
    return(name);
}

/*------------------------------------------------------------------------------
|
|       execR/2
|
|       This function is used to execute an "Rval" subtree (a subtree yielding
|       an Rval).
|
+-----------------------------------------------------------------------------*/

int execR(node *n, pair *p, char *macroName)
{   int   give, gotError;
    node *oldNode;

    DPRINT(2,"execR: ");
    DSHOWFLAVOUR(2,n);
    DPRINT(2,"...\n");
    gotError = 0;
    oldNode        = doingNode;
    if (p)
    {
        doingNode      = n;
        p->T.size      = 0;
        p->T.basicType = T_UNDEF;
        p->R           = NULL;
        switch (n->flavour)
        {
          case CMLIST:
                        DPRINT(2,"execR: ...CMLIST\n");
                        cmList++;
                        if (n->Lson->flavour != CM)
                        {
                            DPRINT(2,"execR: ...CMLIST with no CM\n");
                            give = execR(n->Lson,p, macroName);
                            break;
                        }
                        else if ( (oldNode->flavour != CM) && (oldNode->flavour != EQ) )
                        {
                            int iter;
                            node *n2;

                            if (cmList > 1)
                            {
                               WerrprintfWithPos("No more than one [--.--]");
                               give = 0;
                               gotError = 1;
                               break;
                            }
                            if (cmUsed == 1)
                            {
                               WerrprintfWithPos("Cannot combine , with [--.--]");
                               give = 0;
                               gotError = 1;
                               break;
                            }
                            n2 = n->Lson;
                            cmCnt = 0;
                            while (n2 && (n2->flavour == CM) )
                            {
                               cmCnt++;
                               n2 = n2->Lson;
                            }
                            if (cmIter != 0)
                            {
                               n2 = n->Lson;
                               for (iter = 0; iter <= cmCnt - cmIter; iter++)
                                  n2 = n2->Lson;
                               n2 = n2->Rbro;
                            }
                            
                            cmIter++;
                            if (!n2)
                            {
                               cmIter = 0;
                               give = 1;
                            }
                            else
                            {
                               give = execR(n2,p, macroName);
                            }
                            break;
                        }
                        else
                        {
                           n = n->Lson;
                         /* if oldnode->flavour == CM, fall through to CM case */
                        }
          case CM:      if (execR(n->Lson,p, macroName))
                        {   pair Tp;

                            DPRINT1(3,"execR: temPair at 0x%08x\n",&Tp);
                            if (execR(n->Lson->Rbro,&Tp, macroName))
                                if (appendPair(p,&Tp))
                                {
                                    DSHOWPAIR(3,"execR: ",p);
                                    give = 1;
                                }
                                else
                                {
                                    gotError = 1;
                                    give = 0;
                                }
                            else
                                give = 0;
                            cleanPair(&Tp);
                        }
                        else
                            give = 0;
                        break;
          case EQ:      {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y, macroName))
                                    give = do_EQ(p,&x,&y);
                                else
                                    give = 0;
                                cleanPair(&y);
                            }
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case ID:      {   char    *name;
                            int      shouldFree;
                            varInfo *v;

                            if ( (name=execN(n->Lson,&shouldFree, macroName)) )
                                if ( (v=findVar(name)) )
                                {
                                    give = valueOf(v,0,p);
                                    if (! give)
                                    {
                                       WerrprintfWithPos("Variable \"%s\" is undefined!",name);
                                       gotError = 1;
                                    }
                                }
                                else
                                {
#ifdef VNMRJ
                                    extern int jExpressUse;
                                    if ( ! jExpressUse || macroName )
                                       WerrprintfWithPos("Variable \"%s\" is undefined!",name);
				    jExpressionError();
#else 
                                    WerrprintfWithPos("Variable \"%s\" is undefined!",name);
#endif 
                                    gotError = 1;
                                    give = 0;
                                }
                            else
                                give = 0;
                            if (shouldFree)
                            {
                                DPRINT1(3,"execR: ...releasing name %s\n",name);
                                release(name);
                            }
                        }
                        break;
          case LB:      {   char *name;
                            int   shouldFree;

                            if ( (name=execN(n->Lson,&shouldFree, macroName)) )
                            {   varInfo *v;

                                if ( (v=findVar(name)) )
                                {   pair Ip;

                                    DPRINT1(3,"execR: ...temPair at 0x%08x\n",&Ip);
                                    if (execR(n->Lson->Rbro,&Ip, macroName))
                                        if ((Ip.T.size == 1) && (Ip.T.basicType == T_REAL))
                                        {   int i;

                                            i    = (int)(Ip.R->v.r);
                                            give = valueOf(v,i,p);
                                            if (!give)
                                            {
                                               WerrprintfWithPos("%s[%d] index out of bounds",
                                                                  name,i);
#ifdef VNMRJ
					       jExpressionError();
#endif 
                                               gotError = 1;
                                            }
                                        }
                                        else
                                        {
                                           if (Ip.T.basicType == T_STRING)
                                              WerrprintfWithPos("Index for %s['%s'] must be numeric",name,Ip.R->v.s);
                                           else
                                              WerrprintfWithPos("Index for %s must be numeric scalar",name);
#ifdef VNMRJ
					   jExpressionError();
#endif 
                                           give = 0;
                                           gotError = 1;
                                        }
                                    else
                                        give = 0;
                                    cleanPair(&Ip);
                                }
                                else
                                {
#ifdef VNMRJ
                                    extern int jExpressUse;
                                    if ( ! jExpressUse || macroName )
                                       WerrprintfWithPos("Variable \"%s\" is undefined!",name);
				    jExpressionError();
#else 
                                    WerrprintfWithPos("Variable \"%s\" is undefined!",name);
#endif 
                                    give = 0;
                                    gotError = 1;
                                }
                            }
                            else
                                give = 0;
                            if (shouldFree)
                            {
                                DPRINT1(3,"execR: ...releasing name %s\n",name);
                                release(name);
                            }
                        }
                        break;
          case GT:      {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y, macroName))
                                    give = do_GT(p,&x,&y);
                                else
                                    give = 0;
                                cleanPair(&y);
                            }
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case GE:      {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y, macroName))
                                    give = do_GE(p,&x,&y);
                                else
                                    give = 0;
                                cleanPair(&y);
                            }
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case LE:      {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y, macroName))
                                    give = do_LE(p,&x,&y);
                                else
                                    give = 0;
                                cleanPair(&y);
                            }
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case LT:      {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y, macroName))
                                    give = do_LT(p,&x,&y);
                                else
                                    give = 0;
                                cleanPair(&y);
                            }
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case MINUS:   {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y, macroName))
                                    give = do_MINUS(p,&x,&y);
                                else
                                    give = 0;
                                cleanPair(&y);
                            }
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case DIV:     {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y, macroName))
                                    give = do_DIV(p,&x,&y);
                                else
                                    give = 0;
                                cleanPair(&y);
                            }
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case MOD:     {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y, macroName))
                                    give = do_MOD(p,&x,&y);
                                else
                                    give = 0;
                                cleanPair(&y);
                            }
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case MULT:     {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y, macroName))
                                    give = do_MULT(p,&x,&y);
                                else
                                    give = 0;
                                cleanPair(&y);
                            }
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case NE:      {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y, macroName))
                                    give = do_NE(p,&x,&y);
                                else
                                    give = 0;
                                cleanPair(&y);
                            }
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case _NEG:    {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                                give = do_NEG(p,&x);
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case TRUNC:   {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                                give = do_TRUNC(p,&x);
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case SQRT:   {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                                give = do_SQRT(p,&x);
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case NOT:   {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                                give = do_NOT(p,&x);
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case SIZE:    {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                                give = do_SIZE(p,&x);
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case TYPEOF:  {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {
                                give = do_TYPEOF(p,&x);
                                if (!give)
                                {
                                   if (x.T.basicType == T_STRING)
                                      Werrprintf("typeof error: variable '%s' does not exist",
                                              x.R->v.s);
                                   else
                                      Werrprintf("typeof given a real number (%g). Must be a parameter name",
                                              x.R->v.r);
                                   gotError = 1;
                                }
                            }
                            else
                            {
                                give = 0;
                            }
                            cleanPair(&x);
                        }
                        break;
          case PLUS:    {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y, macroName))
                                    give = do_PLUS(p,&x,&y);
                                else
                                    give = 0;
                                cleanPair(&y);
                            }
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case AND:     {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y, macroName))
                                    give = do_AND(p,&x,&y);
                                else
                                    give = 0;
                                cleanPair(&y);
                            }
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case OR:      {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x, macroName))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y, macroName))
                                    give = do_OR(p,&x,&y);
                                else
                                    give = 0;
                                cleanPair(&y);
                            }
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case REAL:    p->T.size      = 1;
                        p->T.basicType = T_REAL;
                        p->R           = newRval();
                        p->R->v.r      = n->v.r;
                        give           = 1;
                        break;
          case UNIT:    p->T.size      = 1;
                        p->T.basicType = T_REAL;
                        p->R           = newRval();
                        p->R->v.r      = stringReal(n->v.s);
                        p->R->v.r      = real_unit(n->v.s,p->R->v.r);
                        give           = 1;
                        break;
          case STRING:  p->T.size      = 1;
                        p->T.basicType = T_STRING;
                        p->R           = newRval();
                        p->R->v.s      = newString(n->v.s);
                        give           = 1;
                        break;
          default:      fprintf(stderr,"BUG! bad flavour (=%d) noticed in execR\n",n->flavour);
                        exit(1);
        }
    }
    else
        give = 0;
#ifdef DEBUG
    if (3 <= Dflag)
        if (give)
            DSHOWPAIR(3,"execR: ",p);
        else
            DPRINT(3,"execR: ...FAILED\n");
#endif 
    if (gotError && macroName) {
      WerrprintfLine3Only("Error macro:  %s,  line %d", macroName, n->location.line);
    }
    doingNode = oldNode;
    return(give);
}

/*------------------------------------------------------------------------------
|
|       execL/5
|
|       This function is used to execute an "Lval subtree" (one that yields
|       an Lval).
|
+-----------------------------------------------------------------------------*/

static int execL( node *n, varInfo **v, char **name, int *i, int *shouldFree,
                  char *macroName )
{  int   give, gotError;
   node *oldNode;

   DPRINT(2,"execL: ");
   DSHOWFLAVOUR(2,n);
   DPRINT(2,"...\n");
   oldNode   = doingNode;
   doingNode = n;
   give = gotError = 0;
   switch (n->flavour)
   { case ID:      if ( ((*name)=execN(n->Lson,shouldFree,macroName)) )
                   {  if ( ((*v)=findVar(*name)) )
                      {  (*i) = 0;
                         give = 1;
                      }
                      else  /* create only local and temp variables */
                         if (*(*name) == '$' || *(*name) == '%')
                         {  if ( ((*v)=createVar(*name)) )
                            {  (*i) = 0;
                               give = 1;
                            }
                            else
                            {  WerrprintfWithPos("Can't create \"%s\"",*name);
                               give = 0;
                               gotError = 1;
                            }
                         }
                         else
                         {  WerrprintfWithPos("Variable \"%s\" doesn't exist.",*name);
                            give = 0;
                            gotError = 1;
                         }
                   }
                   else
                      give = 0;
                   break;
     case LB:
	           if ( ((*name)=execN(n->Lson,shouldFree,macroName)) )
                      if ( ((*v)=findVar(*name)) )
                      {  pair p;

                         DPRINT1(3,"execL: ...temPair at 0x%08x\n",&p);
                         if (execR(n->Lson->Rbro,&p, macroName))
                            if ((p.T.size == 1) && (p.T.basicType == T_REAL))
                            {  int j;

                               j = (int)(p.R->v.r);
                               if ((0 < j) && (j <= ((*v)->T.size+1)))
                               {  (*i) = j;
                                  give = 1;
                               }
                               else
                               {  WerrprintfWithPos("%s[%d]: index out of bounds",*name,j);
                                  give = 0;
                                  gotError = 1;
                               }
                            }
                            else
                            {  WerrprintfWithPos("Index must be numeric scalar");
                               give = 0;
                               gotError = 1;
                            }
                         else
                            give = 0;
                         cleanPair(&p);
                      }
                      else					/* did NOT find name */
                      {  if (*name[0] == '$')
                         { if ( ((*v)=createVar(*name)) )	/* only create temporary arrays */
                           {  pair p;

                              DPRINT1(3,"execL: ...temPair at 0x%08x\n",&p);
                              if (execR(n->Lson->Rbro,&p, macroName))
                              {
                                 if ((p.T.size == 1) && (p.T.basicType == T_REAL))
                                 {  int j;

                                    j = (int)(p.R->v.r);
                                    if ((0 < j) && (j <= ((*v)->T.size+1)))
                                    {  (*i) = j;
                                       give = 1;
                                    }
                                    else
                                    { 
                                       WerrprintfWithPos("%s index out of bounds", *name);
                                       give = 0;
                                       gotError = 1;
                                    }
                                 }
                                 else
                                 { 
                                    WerrprintfWithPos("%s index must be numeric scalar", *name);
                                    give = 0;
                                    gotError = 1;
                                 }
                              }
                              cleanPair(&p);
                           }
                           else
                           {  WerrprintfWithPos("Can't create \"%s\"",*name);
                              give = 0;
                              gotError = 1;
                           }
                         }
                         else
                         {   WerrprintfWithPos("Variable \"%s\" doesn't exist.",*name);
                             give = 0;
                             gotError = 1;
                         }
                     }
                   else
                      give = 0;
                   break;
     default:      WerrprintfWithPos("BUG! bad flavour (=%d) noticed in execL",n->flavour);
                   give        = 0;
                   *shouldFree = 0;
                   gotError = 1;
   }
#ifdef DEBUG
   if (3 <= Dflag)
      if (give)
      {  if (*name)
            DPRINT2(3,"execL: ...\"%s\"[%d]...\n",*name,*i);
         else
            DPRINT2(3,"execL: ...0x%08x[%d]...\n",*v,*i);
         DSHOWVAR(3,"execL:    ",*v);
         DPRINT1(3,"execL: ...gives %d\n",give);
      }
      else
         DPRINT1(3,"execL: ...FAILED, gives %d\n",give);
#endif 
   if (gotError && macroName) {
      WerrprintfLine3Only("Error macro:  %s,  line %d", macroName, n->location.line);
   }
   doingNode = oldNode;
   return(give);
}

/*------------------------------------------------------------------------------
|
|       execS/1
|
|       This function is used to execute a "statement level" subtree.
|
|	NOTE: This routine returns 1 if normal completion, 0 if not (backwards
|	      from macro/function return codes).
|
+-----------------------------------------------------------------------------*/

int execS(node *n, char *macroName)
{
   extern int  interuption;
   extern int fromFile;
   int         give, gotError;
   node       *oldNode;
   static int  macro_active = 0;
   static int  ipa_macro_active = 0;

   give = 1;		/* give == 0 implies abort.  Default to no abort */
   gotError = 0;
   oldNode   = doingNode;
   if (interuption)
   {
      DPRINT(1,"execS: Interuption detected!, forced failure\n");
      give = 0;
   }
   else
   {
      DPRINT(2,"execS: ");
      DSHOWFLAVOUR(2,n);
      DPRINT(2,"...\n");
      doingNode = n;
      execPError = 0;
      
      if (mustReturn)
      {
         DPRINT(3,"execS: ...skip it, returning! (gives 1)\n");
         give = 1;
      }
      else
      {  switch (n->flavour)
         { case _Car:    {  char *name;
                            int   shouldFree;

                            if ( (name=execN(n->Lson->Lson,&shouldFree, macroName)) )
                               give = execMP(name,n->Lson->Rbro,n->Lson->Rbro->Rbro,macroName, n->location.line);
                            else
                               give = 0;
                            if (shouldFree)
                            {
                               DPRINT1(3,"execS: ...releasing name %s\n",name);
                               release(name);
                            }
                         }
                         break;
           case _Ca_:    {  char *name;
                            int   shouldFree;

                            if ( (name=execN(n->Lson->Lson,&shouldFree, macroName)) )
                               give = execMP(name,n->Lson->Rbro, (node *)NULL,macroName, n->location.line);
                            else
                               give = 0;
                            if (shouldFree)
                            {
                               DPRINT2(3,"execS: ...releasing name %s (at 0x%08x\n",name,name);
                               release(name);
                            }
                         }
                         break;
           case _C_r:    {  char *name;
                            int   shouldFree;

                            if ( (name=execN(n->Lson->Lson,&shouldFree, macroName)) )
                                give = execMP(name, (node *)NULL,n->Lson->Rbro,macroName, n->location.line);
                            else
                                give = 0;
                            if (shouldFree)
                            {
                               DPRINT2(3,"execS: ...releasing name %s (at 0x%08x\n",name,name);
                               release(name);
                            }
                         }
                         break;
           case _C__:    {  char *name;
                            int   shouldFree;

                            execStringRes = 0;
                            if ( (name=execN(n->Lson->Lson,&shouldFree, macroName)) )
                               give = execMP(name, (node *)NULL, (node *)NULL,macroName, n->location.line);
                            else
                               give = execStringRes;
                            if (shouldFree)
                            {
                               DPRINT2(3,"execS: ...releasing name %s (at 0x%08x\n",name,name);
                               release(name);
                            }
                         }
                         break;
           case CM:      if (execS(n->Lson, macroName))
                            give = execS(n->Lson->Rbro, macroName);
                         else
                            give = 0;
                         break;
           case DONT:    ignoreBomb = 0;
                         give = 1;		/* Don't let abortoff provoke abort */
                         break;
           case ELSE:    {  pair p;

                            DPRINT1(3,"execS: ...temPair at 0x%08x\n",&p);
                            if (execR(n->Lson,&p, macroName))
                               if (isTrue(&p))
                               {
                                   DPRINT(3,"execS: ...IF-THEN-ELSE-FI exp is true\n");
                                  give = execS(n->Lson->Rbro, macroName);
                               }
                               else
                               {
                                  DPRINT(3,"execS: ...IF-THEN-ELSE-FI exp is false\n");
                                  give = execS(n->Lson->Rbro->Rbro, macroName);
                               }
                            else
                            {
                               DPRINT(3,"execS: ...IF-THEN-ELSE-FI exp failed\n");
                               give = 0;
                            }
                            cleanPair(&p);
                         }
                         break;
           case EOL:     give = 1;
                         break;
           case EQ:      {  char    *name;
                            int      shouldFree;
                            int      i;
                            varInfo *v;

                            if (execL(n->Lson,&v,&name,&i,&shouldFree, macroName))
                            {  pair p;
                               pair Tp2;
                               pair *Tp;
                               int res;

                               DPRINT1(3,"execS: ...temPair at 0x%08x\n",&p);
                               cmCnt = -1;  /* number of commas in a CMLIST element */
                               cmIter = 0;
                               cmUsed = (n->Lson->Rbro->flavour == CM);
                               Tp = &p;
                               do
                               {
                                  DPRINT1(3,"execS: ...temPair at 0x%08x\n",Tp);
                                  cmList = 0;
                                  res = execR(n->Lson->Rbro,Tp, macroName);
                                  if (cmCnt && res)
                                  {
                                     /* A CMLIST is present */
                                     if (Tp != &p)
                                     {
                                        appendPair(&p,&Tp2);
                                        cleanPair( &Tp2 );
                                     }
                                     Tp = &Tp2;
                                  }
                               } while ((cmIter <= cmCnt) && res);
                               if (res)
                               {
                                  char *par_macro;

                                  if (!(v->prot & P_VAL))
                                  {
                                     if (!fromFile &&  (( *name == '$') || (*name == '%')) )
                                     {
                                        /* Dollar variables from the command line or panels can be
                                         * used as reals or strings
                                         */
                                        if ( p.T.basicType != v->T.basicType)
                                        {
                                           clearVar(name);
                                        }
                                     }
                                     if (checkParm(&p,v,name))
                                     {
#ifdef VNMRJXXX
					int diffval = 1;
                                        if ((!(v->prot & P_REX)) && (name[0] != '$'))
					{
					   initStoredAttr(v);
					   diffval = comparePair(&p,v,i);
					}
#endif 
					v->active = ACT_ON; /* Force active */
                                        give = assignPair(&p,v,i);
                                        if (M3flag)
                                        {
                                           fprintf(stderr,"%*s%s=",MflagIndent,"", name);
                                           printRvals2(stderr,v->T.basicType,v->R);
                                           fprintf(stderr,"\n");
                                        }
                                        if ((v->prot & P_MAC) && (!macro_active))
                                        {  macro_active = 1;
                                           par_macro = newString("_");
                                           par_macro = newCatId(par_macro,name,"execS");
                                           par_macro = newCatId(par_macro,"\n","execS");

                                           execString(par_macro);
                                           macro_active = 0;
                                           release(par_macro);
                                        }
                                        if (name[0] != '$')
                                        {
                                           if (v->Ggroup == G_ACQUISITION)
                                           {
                                             if ((v->prot & P_IPA) && (!ipa_macro_active))
                                             {
                                               ipa_macro_active = 1;
                                               par_macro = newString("_ipa('");
                                               par_macro = newCatId(par_macro,name,"execS");
                                               par_macro = newCatId(par_macro,"')\n","execS");

                                               execString(par_macro);
                                               ipa_macro_active = 0;
                                               release(par_macro);
                                             }
                                             if (!(v->prot & P_NOA))
                                               checkarray(name,v->T.size);
                                           }

                                    /* create a list of changed variables        */
                                    /* to be passed to the dg or dscommand if    */
                                    /* neccessary after command line is executed */
#ifdef VNMRJXXX
				           if ((diffval==1) || (compareStoredAttr(v)==1))
				           {
#endif 
                                              if (!(v->prot & P_REX))
                                                 appendvarlist(name);
#ifdef VNMRJ
                                              // if ((v->prot & P_JRX))
                                              if ( P_JRX )
                                                 appendJvarlist(name);
#endif 
#ifdef VNMRJXXX
                                           }
#endif 
                                      }
                                        if (v->prot & P_SYS)
                                          check_datastation();
                                     }
                                     else
                                     {
                                        give = 0;
                                        gotError = 1;
                                     }
                                  }
                                  else
                                  {  WerrprintfWithPos("entry of parameter %s is not allowed",name);
                                     give = 0;
                                     gotError = 1;
                                  }
                               }
                               else
                               {
                                  /* Tend to get a lot of these from xml panels
                                   * May put this test back in when panels are fixed
                                   * WerrprintfWithPos("failed to assign %s",name);
                                   */
                                  give = 0;
                               }
                               cleanPair(&p);
                            }
                            else
                            {
                               give = 0;
                            }
                            if (shouldFree)
                            {
                               DPRINT2(3,"execS: ...releasing name %s (at 0x%08x\n",name,name);
                               release(name);
                            }
                         }
                         break;
           case BOMB:    give = 0;
                         break;
           case EXIT:    mustReturn = 1;
                         DPRINT(3,"execS: ...return mode set to ON\n");
                         give = macArgvRetv(1,n->Lson,0,macroName);
                         break;
           case IGNORE:  ignoreBomb = 1;
                         give = 1;		/* Don't let aborton provoke abort */
                         break;
           case POP:     popTree();
                         give = 1;
                         break;
           case PUSH:    pushTree();
                         give = 1;
                         break;
           case SHOW:    {  char             *name;
                            char              s[1024];

                            int               shouldFree;
                            int               i;
                            varInfo          *v;

                            if (execL(n->Lson,&v,&name,&i,&shouldFree,macroName))
                            {  pair p;

                               Wclearerr(); /* clear error line */
                               if (valueOf(v,i,&p)) /* if we have a good value */
                               {  give = 1;
                                  if (isActive(v))
                                  {  if (0 < i) /* indexed variable ? */
                                        sprintf(s,"%s[%d] = ",name,i);
                                     else
                                        sprintf(s,"%s = ",name);
                                     strncat(s,buildValue(&p),512);
                                     DPRINT1(3,"execS:.temPair at 0x%08x\n",&p);
                                  }
                                  else
                                  {  if (0 < i) /* indexed variable ? */
                                        sprintf(s,"%s[%d] = Not Used ( ",name,i);
                                     else
                                        sprintf(s,"%s = Not Used ( ",name);
                                     DPRINT1(3,"execS:.temPair at 0x%08x\n",&p);
                                     strncat(s,buildValue(&p),512);
                                     strcat(s," )");
                                  }
#ifdef VNMRJ
				  Wseterrorkey("oi"); /* "oi" or "vi" */
#endif 
                                  Winfoprintf(" %s",s);
                               }       
                               else
                               {  WerrprintfWithPos("%s[%d] index out of bounds",name,i);
                                  give = 0;
                                  gotError = 1;
                               }
                               cleanPair(&p);
                            }
                            else
                               give = 0;
                            if (shouldFree)
                            {
                               DPRINT2(3,"execS: ...releasing name %s (at 0x%08x\n",name,name);
                               release(name);
                            }
                         }
                         break;
           case THEN:    {  pair p;

                            DPRINT1(3,"execS: ...temPair at 0x%08x\n",&p);
                            if (execR(n->Lson,&p, macroName))
                               if (isTrue(&p))
                               {
                                  DPRINT(3,"execS: IF-THEN-FI exp is true\n");
                                  give = execS(n->Lson->Rbro, macroName);
                               }
                               else
                               {
                                  DPRINT(3,"execS: IF-THEN-FI exp is false\n");
                                  give = 1;
                               }
                            else
                            {
                               DPRINT(3,"execS: IF-THEN-FI exp failed\n");
                               give = 0;
                            }
                            cleanPair(&p);
                         }
                         break;
           case WHILE:   {  int looping;
			    int ival;

                            looping = 1;
                            while (looping && !mustReturn)
                            {  pair p;

                               DPRINT1(3,"execS: ...temPair at 0x%08x\n",&p);
                               ival = execR(n->Lson,&p, macroName);
                               if (ival)
                                  if (isTrue(&p))
                                  {
                                     DPRINT(3,"execS: ...WHILE-DO-OD exp is true\n");
                                     ival = execS(n->Lson->Rbro, macroName);
                                     if (ival == 0)
                                     {  looping = 0;
                                        give    = 0;
                                     }
				     else
				      give = 1;
                                  }
                                  else
                                  {
                                     DPRINT(3,"execS: ...WHILE-DO-OD exp is false\n");
                                     looping = 0;
                                     give    = 1;
                                  }
                               else
                               {
                                  DPRINT(3,"execS: WHILE-DO-OD exp failed\n");
                                  looping = 0;
                                  give    = 0;
                               }
                               cleanPair(&p);
                            }
                         }
                         break;
           case REPEAT:  {  int looping;

                            looping = 1;
                            while (looping && !mustReturn)
                               if (execS(n->Lson->Rbro, macroName))
                               {  pair p;

                                  DPRINT1(3,"execS: ...temPair at 0x%08x\n",&p);
                                  if (execR(n->Lson,&p, macroName))
                                     if (isTrue(&p))
                                     {
                                        DPRINT(3,"execS: ...REPEAT-UNTIL exp is true\n");
				        looping = 0;
				        give    = 1;
                                     }
                                     else
                                     {
                                        DPRINT(3,"execS: ...REPEAT-UNTIL exp is false\n");
					give = 1;
                                     }
                                  else
                                  {
                                     DPRINT(3,"execS: ...REPEAT-UNTIL exp failed\n");
                                     looping = 0;
                                     give    = 0;
                                  }
                                  cleanPair(&p);
                               }
                               else
                               {  looping = 0;
                                  give    = 0;
                               }
                         }
                         break;
           default:      WerrprintfWithPos("BUG! execS has funny flavour (=%d)",n->flavour);
                         give = 0;
                         gotError = 1;
         }
      }
   }
   if (execPError != 0 || gotError) {
      if (macroName)
         WerrprintfLine3Only("Error macro:  %s,  line %d", macroName, n->location.line);
      execPError = 0;
   }
   if (give==0 && mustReturn==0 && ignoreBomb)
   {
      DPRINT3(3,"execS: ...give=%d, but mustReturn=%d and ignoreBomb=%d, ignoreable bomb!!\n",give,mustReturn,ignoreBomb);
      give = 1;
   }
   DPRINT(3,"execS: ...");
   DSHOWFLAVOUR(3,n);
   DPRINT1(3," gives %d\n",give);
   doingNode = oldNode;
   return(give);
}

/* Reset static mustReturn to 0.  Called from loadAndExec
 * Handles case where return it typed into the command line
 */
void clearMustReturn()
{
   mustReturn    = 0;
}

/*------------------------------------------------------------------------------
|
|       checkarray(name,size)
|         name is a parameter name
|         size is that parameter's array size
|
|       Whenever an acquisition parameter is changed,  this routine
|       checks whether the parameter 'array' must be updated.
|
|	NOTE: This routine is called from the EQ case of execS
|
+-----------------------------------------------------------------------------*/
void checkarray(char *name, short size)
{
   extern void skipGainTest();
   char arrayname[MAXPATH];
   int  arraylen;

   if (P_getactive(CURRENT,name) == -2)
   {
      /* If the parameter is not in the CURRENT tree, return */
      return;
   }
   if (P_getstring(CURRENT,"array",arrayname,1,MAXPATH-6))
      return;
   arraylen = strlen(arrayname);
   DPRINT4(1," name: %s size: %d arraylen: %d array: %s\n",
               name,size,arraylen,arrayname);
   if ((size <= 1) && (arraylen == 0))
   {
     if (strcmp(name,"array") == 0)
     {
        /* update arraydim */
        skipGainTest();
        run_calcdim();
     }
     /*  the parameter is not arrayed and 'array' is not set */
     /*  no change is the 'array' parameter                  */
     return;
   }
   else 
   {
      int  found = 0;
#ifdef VNMRJ
      char tmparray[MAXPATH];
      strcpy(tmparray,arrayname);
#endif 
      if (size > 1)
      {
        if (arraylen == 0)
        {
          /*  the parameter is arrayed and 'array' is not set */
          /*  set 'array' to the parameter name               */
          P_setstring(CURRENT,"array",name,1);
          found=1;
#ifdef VNMRJ
          strcpy(arrayname,name);
#endif 
        }
        else
        {
          /*  the parameter is arrayed and 'array' is set    */
          /*  add the parameter name to 'array' if necessary */
          editpar(arrayname,name,arraylen,1);
          found=1;
        }
      }
      else
      {
          /*  the parameter is not arrayed and 'array' is set     */
          /*  remove the parameter name from 'array' if necessary */
          found = editpar(arrayname,name,arraylen,0);
      }
#ifdef VNMRJ
      if (jShowArray == 1)
          appendvarlist("array");
      else
      {
          if (strcmp(tmparray,arrayname) != 0)
            appendvarlist("array");
      }
#endif 
      if ( (found) || (strcmp(name,"array") == 0) )
      {
         /* update arraydim */
         skipGainTest();
         run_calcdim();
      }
   }
#ifdef DEBUG
   P_getstring(CURRENT,"array",arrayname,1,MAXPATH-6);
   DPRINT1(1,"array: %s\n",arrayname);
#endif 
}

/*------------------------------------------------------------------------------
|
|       editpar(arr,name,len,addit)
|         arr is the 'array' parameter
|         name is a parameter name
|         len is the string length of the parameter 'array'
|         addit is a boolean to decide whether to add "name"
|           to the 'array' parameter or to remove it
|
|       This routine will either append "name" to the string 'arr'
|       or remove "name" from the string 'arr'.  When removing "name",
|       commas and parenthesis may also need to be removed.
|     
|
+-----------------------------------------------------------------------------*/
static int editpar(char arr[], char *name, int len, int addit)
{
  int i,j,k;
  int found;
  char tmp[256];

  i = 0;
  found = 0;
  k = 0;
  while ((i < len) && !found)
  {
    while ((i < len) &&
           ((arr[i] == ',') || (arr[i] == '(') || (arr[i] == ')')))
      i++;
    j = 0;
    k = i; /* k points to the first character of the name */
    while ((i < len) && (arr[i] != ',') && (arr[i] != ')'))
      tmp[j++] = arr[i++];
    tmp[j] = '\0';
    DPRINT3(1," editpar: first %d last %d token %s\n",k,i,tmp);
    found = (strcmp(tmp,name) == 0);
  }
  DPRINT2(1," editpar: addit %d found %d\n",addit,found);
  if (addit)
  {
    if (!found)
    {
      strcat(arr,",");
      strcat(arr,name);
      DPRINT1(1," editpar: setting array to %s\n",arr);
      P_setstring(CURRENT,"array",arr,1);
    }
  }
  else
  {
    if (found)
    {
      if (arr[i] == '\0')
      {
      /* "name" was found at the end of the string */
        if (k > 0)
          arr[k-1] = '\0'; /* remove the comma */
        else
          arr[k] = '\0';   /* "name was the only element of the string */
      }
      else if (((k > 0) && (arr[k-1] == '(')) || (arr[i] == ')'))
      {
        /* "name" was found next to a parenthesis */
        j = k;
        if (arr[i] == ')')
        {
          if ( (k) && (arr[k-1]== ',') )
            k--;  /* remove the preceding comma */
        }
        else if (arr[i]== ',')
          i++;    /* remove the following comma */
        while (i <= len)
          arr[k++] = arr[i++];
        i = 0;
        while (i < len)
        {
          found = 0;
          /* look for the left parenthesis */
          while ((i < len) && (arr[i] != '('))
            i++;
          /*  find a comma between the two parentheses */
          if (arr[i] == '(')
          {
            k = i;
            while ((i < len) && (arr[i] != ')') && !found)
              found = (arr[i++] == ',');
            /*  if no comma, remove the parentheses */
            if (!found)
            {
              arr[k] = ' ';
              arr[i] = ' ';
              if (k == i - 1)
              {
                 if ( (k==0) && (arr[i+1] == ',') )
                    arr[i+1] = ' ';
                  else if ( (k > 1) && (arr[k-1] == ',') )
                     arr[k-1] = ' ';
              }
              j = k = 0;
              while (j < len)
                if (arr[j] == ' ')
                  j++;
                else
                  arr[k++] = arr[j++];
            }
          }
        }
      }
      else
      {
        /* "name" was preceded or followed by a comma */
        if (k)
          k--;  /* remove the preceding comma */
        else
          i++;  /* remove the following comma */
        while (i <= len)
          arr[k++] = arr[i++];
      }
      DPRINT1(1," editpar: setting array to %s\n",arr);
      P_setstring(CURRENT,"array",arr,1);
    }
  }
  return(found);
}

static double real_unit(char str_val[], double startVal)
{
   char units[256];
   double dummy;
   extern double unitConvert();

#ifdef UNIX
   sscanf(str_val,"%lg%s",&dummy,units);
#else 
   sscanf(str_val,"%lf%s",&dummy,units);
#endif 
   return(unitConvert(units,startVal));
}

static  char *badMacro_2[] = { "aa", "cr", "ga", "go",  "lc",  "mf", "mm", "mt",
         "RQ", "vj", "vp", "xm", NULL };

static  char *badMacro_3[] = { "acq", "add", "aip",  "cex", "ecc", "fix",  "kil",
        "loc", "par",  NULL };

static  char *badMacro_4[] = { "auto", "boot", "jwin", "jplo", "prot",
        "pbox", "qtun", "zoom", NULL };

static  char *badMacro_5[] = { "array", "iplan", NULL };

static  char *badMacro_6[] = { "config", "jprint", NULL };

static int is_print_safe_macro(char *name)
{
    char  *token;
    int   k;

    if (strcmp(name, "au") == 0)
        return 0;
    k = 0;
    token = badMacro_2[k++];
    while (token != NULL) {
        if (strncmp(name, token, 2) == 0)
            return 0;
        token = badMacro_2[k++];
    }
    k = 0;
    token = badMacro_3[k++];
    while (token != NULL) {
        if (strncmp(name, token, 3) == 0)
            return 0;
        token = badMacro_3[k++];
    }
    k = 0;
    token = badMacro_4[k++];
    while (token != NULL) {
        if (strncmp(name, token, 4) == 0)
            return 0;
        token = badMacro_4[k++];
    }
    k = 0;
    token = badMacro_5[k++];
    while (token != NULL) {
        if (strncmp(name, token, 5) == 0)
            return 0;
        token = badMacro_5[k++];
    }
    k = 0;
    token = badMacro_6[k++];
    while (token != NULL) {
        if (strncmp(name, token, 6) == 0)
            return 0;
        token = badMacro_6[k++];
    }
    return 1;
}

