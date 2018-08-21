/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* 
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
#include "graphics.h"
#include "group.h"
#include "node.h"
#include "params.h"
#include "symtab.h"
#include "variables.h"
#include "magic.gram.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char           *Dx_string; /* pointer to list of changed variable names */
extern char    *newCatId();
extern char    *newCatIdDontTouch();
extern char    *newString();
extern char    *intString();
extern char    *realString();
extern char    *Wgetscreendisplay();
extern char    *buildValue();
extern double   stringReal();
extern int      assignPair();
extern int    (*builtin())();
extern int      checkParm();
extern int      Eflag;
extern int      do_DIV();
extern int      do_MINUS();
extern int      do_MOD();
extern int      do_MULT();
extern int      do_NEG();
extern int      do_NOT();
extern int      do_PLUS();
extern int      do_SIZE();
extern int      do_SQRT();
extern int      do_TRUNC();
extern int      do_TYPEOF();
extern int      isActive();
extern int      listLength();
extern int      valueOf();
extern node    *findMacro();
extern node    *load();
extern Rval    *newRval();
extern varInfo *createVar();
extern varInfo *findVar();
extern void     popTree();
extern void     pushTree();
extern void     showTree();
extern void     showValue();
static double	real_unit();
static int      ignoreBomb;
static int      mustReturn;
static int      numMacRets;
static Rval     macRetVals[64];
node           *doingNode;
static int execL(node *n, varInfo **v, char **name, int *i, int *shouldFree);

#define SCROLL 4

/*#define DEBUG*/

#ifdef  DEBUG
static int Dflag = 3;
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

/*------------------------------------------------------------------------------
|
|       isTrue/1
|
|       This function returns true (non-zero) if the given pair represents
|       a non-zero REAL value or a STRING of non-zero length.  Otherwise
|       this function returns false (zero).
|
+-----------------------------------------------------------------------------*/

int isTrue(p)                           pair *p;
{   Rval *r;

    if (p)
    {   if (r=p->R)
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

/*------------------------------------------------------------------------------
|
|       appendvarlist
|
|       This procedure maintains the list of parameters which have been assigned
|
+-----------------------------------------------------------------------------*/

void appendvarlist(name)        char *name;
{
    if (Dx_string == NULL)
        Dx_string = newString("'");
    else
        Dx_string = newCatId(Dx_string,",'","appendvarlist");
    Dx_string = newCatId(Dx_string,name,"appendvarlist");
    Dx_string = newCatId(Dx_string,"'","appendvarlist");
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

int procArgv(i,n,argc,argv)             int i; node *n; int argc; char *argv[];
{   int      index,j,k;
    varInfo *v;

    DPRINT2(3,"procArgv(%d): of %d...\n",i,argc);
    DSHOWTREE(3,0,"procArgv(_):    ",n);
    if (n)
    {   if (n->flavour == CM)
        {
            DPRINT1(3,"procArgv(%d): ...CM sublist, to left...\n",i);
            if ((j = procArgv(i,n->Lson,argc,argv)) == 0)
                return(j);
            DPRINT1(3,"procArgv(%d): ...to right...\n",j);
            if ((k = procArgv(j,n->Lson->Rbro,argc,argv)) == 0)
                return(k);
            DPRINT2(3,"procArgv(%d): ...finished at %d\n",i,k);
            return(k);
        }
        else
        {   pair p;

            DPRINT1(3,"procArgv(%d): ...item or expression...\n",i);
            if (execR(n,&p))
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

int procRetv(i,n,retc,retv)             int i; node *n; int retc; char *retv[];
{   int      index,j,k;
    varInfo *v;

    DPRINT2(3,"procRetv: %d of %d...\n",i,retc);
    DSHOWTREE(3,0,"procRetv:    ",n);
    if (n)
    {   if (n->flavour == CM)
        {
            DPRINT1(3,"procRetv: ...CM sublist, %d to left...\n",i);
            j = procRetv(i,n->Lson,retc,retv);
            DPRINT1(3,"procRetv:                %d to right...\n",j);
            k = procRetv(j,n->Lson->Rbro,retc,retv);
            DPRINT1(3,"procRetv: ...finished at %d\n",k);
            return(k);
        }
        else
        {   char *name;
            int   shouldFree;

            DPRINT1(3,"procRetv: ...item %d...\n",i);
            if (execL(n,&v,&name,&index,&shouldFree))
            {
                DPRINT2(3,"procRetv: target is %s[%d]\n",name,index);
                if ((0 <= i) && (i < retc))
                {   if (retv[i])
                    {
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
                                                WerrprintfWithPos("Expected REAL return value");
                                            break;
                          case T_STRING:    assignString(retv[i],v,index);
                                            break;
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

static int execP(n,p,a,r)               char *n; int (*p)(); node *a; node *r;
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
    if (argv=(char **)allocateWithId((argc+1)*sizeof(char *),"execP/argv"))
    {   for (i=0; i<argc+1; ++i)  /* null out pointers */
                argv[i] = NULL;
        argv[0] = newString(n);
        retc    = listLength(r);
        if (retv=(char **)allocateWithId((retc+1)*sizeof(char *),"execP/retv"))
        {   int i;
            int ret;

            DPRINT1(3,"execP: ...process %d arg(s)...\n",argc);
            ret = procArgv(1,a,argc,argv);
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
            {   gives = !((*p)(argc,argv,retc,retv));
                if (Eflag && !gives)
                   fprintf(stderr,"execP:Procedure '%s' aborted!\n",argv[0]);
                DPRINT1(3,"execP: ...back from procedure, process upto %d returns...\n",retc);
                procRetv(0,r,retc,retv);
            }
            else
                gives = 0;
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

static int macArgvRetv(i,n)             int i; node *n;
{   if (n)
    {   if (n->flavour == CM)
        {   int j,k;

            DPRINT1(3,"macArgvRetv(%d): ...a comma, go left\n",i);
            if (j=macArgvRetv(i,n->Lson))
            {
                DPRINT2(3,"macArgvRetv(%d): ...then right from %d\n",i,j);
                if (k=macArgvRetv(j,n->Lson->Rbro))
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
            if (execR(n,&p))
            {   char     tmp[32];
                varInfo *v;

                sprintf(tmp,"%%%d",i);
                if (v=createVar(tmp))
                {   assignPair(&p,v,0);
                    DSHOWVAR(3,"macArgvRetv(_):    ",v);
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

int placeMacReturns(i,n)                int i; node *n;
{   if (n)
    {   if (n->flavour == CM)
        {   int j,k;

            DPRINT1(3,"placeMacReturns(%d): ...a comma, go left\n",i);
            if (j=placeMacReturns(i,n->Lson))
            {
                DPRINT2(3,"placeMacReturns(%d): ...then right from %d\n",i,j);
                if (k=placeMacReturns(j,n->Lson->Rbro))
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
            if (Sv=findVar(tmp))
            {   char    *name;
                int      index;
                int      shouldFree;
                varInfo *Dv;

                DPRINT2(3,"placeMacReturn(%d):    there is a \"%s\" (source)\n",i,tmp);
                if (execL(n,&Dv,&name,&index,&shouldFree))
                {   pair p;

                    DPRINT2(3,"placeMacReturn(%d): ...temPair at 0x%08x\n",i,&p);
                    DPRINT3(3,"placeMacReturn(%d):    there is a \"%s\"[%d] (dest)\n",i,name,index);
                    if (valueOf(Sv,0,&p))
                    {
                        DPRINT1(3,"placeMacReturn(%d):    ...complete with a value\n",i);
                       if (assignPair(&p,Dv,index))
                           give = i+1;
                        else
                           give = 0;
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
}

/*------------------------------------------------------------------------------
|
|       execM/3
|
|       This procedure is used to set-up and call a macro.
|
+-----------------------------------------------------------------------------*/

static int execM(n,a,r)                 char *n; node *a; node *r;
{   varInfo *v;

    DPRINT(3,"execM: arg tree...\n");
    DSHOWTREE(3,0,"execM:    ",a);
    DPRINT(3,"execM: ...ret tree...\n");
    DSHOWTREE(3,0,"execM:    ",r);
    if (v=createVar("%#"))
    {   assignReal((double)listLength(a),v,0);
        if (v=createVar("%0"))
        {   assignString(n,v,0);
            if (macArgvRetv(1,a))
            {   int   execS();
                int   give;
                node *codeTree;

                pushTree();
                if ((codeTree=findMacro(n)) == NULL)
                {
                    DPRINT(3,"execM: ...not in table, try to load\n");
                    if (codeTree=load(n,SEARCH))
                    {
                        DPRINT(3,"execM: ...loaded, cache it\n");
                        saveMacro(n,codeTree,EXTENT_OF_CMD);
                        DPRINT(3,"execM: ...cached\n");
                    }
                }
                if (codeTree)
                {
                    DPRINT(3,"execM: ...execute it...\n");
                    give = execS(codeTree);
                    DPRINT1(3,"execM ...finished, gave %d\n",give);
                }
                else
                {
                    DPRINT(3,"execM: ...nothing to execute, give up\n");
                    give = 0;
                }
                popTree();
                DPRINT(3,"execM: ...popped to calling space, emplace return values\n");
                placeMacReturns(1,r);
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

static int execMP(n,a,r)                char *n; node *a; node *r;
{   int give;
    int oldIgnoreBomb;
    int (*p)();

    DPRINT(3,"execMP: return mode set to OFF...\n");
    oldIgnoreBomb = ignoreBomb;
    ignoreBomb    = 0;
    mustReturn    = 0;
    if (p=builtin(n))
    {
        DPRINT1(3,"execMP: ...\"%s\" is builtin, assume procedure\n",n);
/*      if (!isGraphCmd(n)) /* set to alpha if procedure is not graphic */
/*          Walpha(SCROLL); */
        give = execP(n,p,a,r);
    }
    else
    {
        DPRINT1(3,"execMP: ...\"%s\" is not builtin, assume macro\n",n);
        give = execM(n,a,r);
    }
    DPRINT(3,"execMP: ...return mode reset to OFF\n");
    DPRINT1(3,"execMP: ...gives %d\n",give);
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

char *execN(n,shouldFree)                       node *n; int *shouldFree;
{   char *name;
    node *oldNode;

    oldNode   = doingNode;
    doingNode = n;
    switch (n->flavour)
    { case DOLLAR:  {   char *tmp;
                        pair  p;

                        DPRINT1(3,"execN: DOLLAR, temPair at 0x%08x\n",&p);
                        if (execR(n->Lson,&p))
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
                                }
                            }
                            else
                            {   WerrprintfWithPos("${...} only applies to scalar values");
                                tmp         = NULL;
                                *shouldFree = 0;
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
                        if (execR(n->Lson,&p))
                            if (p.T.size == 1)
                            {   switch (p.T.basicType)
                                { case T_UNDEF:     name        = NULL;
                                                    *shouldFree = 0;
                                                    break;
                                  case T_REAL:      WerrprintfWithPos("{...} only applies to string expressions");
                                                    name        = NULL;
                                                    *shouldFree = 0;
                                                    break;
                                  case T_STRING:    name        = newString(p.R->v.s);
                                                    *shouldFree = 1;
                                                    if (! goodName(name))
                                                    {
                                                       name = newCatId(name,"\n","execN");
                                                       execString(name);
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

int execR(n,p)                   node *n; pair *p;
{   int   give;
    node *oldNode;

    DPRINT(2,"execR: ");
    DSHOWFLAVOUR(2,n);
    DPRINT(2,"...\n");
    if (p)
    {   oldNode        = doingNode;
        doingNode      = n;
        p->T.size      = 0;
        p->T.basicType = T_UNDEF;
        p->R           = NULL;
        switch (n->flavour)
        { case CM:      if (execR(n->Lson,p))
                        {   pair Tp;

                            DPRINT1(3,"execR: temPair at 0x%08x\n",&Tp);
                            if (execR(n->Lson->Rbro,&Tp))
                                if (appendPair(p,&Tp))
                                {
                                    DSHOWPAIR(3,"execR: ",p);
                                    give = 1;
                                }
                                else
                                    give = 0;
                            else
                                give = 0;
                            cleanPair(&Tp);
                        }
                        else
                            give = 0;
                        break;
          case EQ:      {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y))
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

                            if (name=execN(n->Lson,&shouldFree))
                                if (v=findVar(name))
                                    give = valueOf(v,0,p);
                                else
                                {   WerrprintfWithPos("Variable \"%s\" is undefined!",name);
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

                            if (name=execN(n->Lson,&shouldFree))
                            {   varInfo *v;

                                if (v=findVar(name))
                                {   pair Ip;

                                    DPRINT1(3,"execR: ...temPair at 0x%08x\n",&Ip);
                                    if (execR(n->Lson->Rbro,&Ip))
                                        if ((Ip.T.size == 1) && (Ip.T.basicType == T_REAL))
                                        {   int i;

                                            i    = (int)(Ip.R->v.r);
                                            give = valueOf(v,i,p);
                                        }
                                        else
                                        {   WerrprintfWithPos("Index must be numeric scalar");
                                            give = 0;
                                        }
                                    else
                                        give = 0;
                                    cleanPair(&Ip);
                                }
                                else
                                {   WerrprintfWithPos("Variable \"%s\" is undefined!",name);
                                    give = 0;
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
                            if (execR(n->Lson,&x))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y))
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
                            if (execR(n->Lson,&x))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y))
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
                            if (execR(n->Lson,&x))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y))
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
                            if (execR(n->Lson,&x))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y))
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
                            if (execR(n->Lson,&x))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y))
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
                            if (execR(n->Lson,&x))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y))
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
                            if (execR(n->Lson,&x))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y))
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
                            if (execR(n->Lson,&x))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y))
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
                            if (execR(n->Lson,&x))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y))
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
                            if (execR(n->Lson,&x))
                                give = do_NEG(p,&x);
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case TRUNC:   {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x))
                                give = do_TRUNC(p,&x);
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case SQRT:   {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x))
                                give = do_SQRT(p,&x);
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case NOT:   {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x))
                                give = do_NOT(p,&x);
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case SIZE:    {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x))
                                give = do_SIZE(p,&x);
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case TYPEOF:  {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x))
                                give = do_TYPEOF(p,&x);
                            else
                                give = 0;
                            cleanPair(&x);
                        }
                        break;
          case PLUS:    {   pair x;

                            DPRINT1(3,"execR: ...temPair at 0x%08x\n",&x);
                            if (execR(n->Lson,&x))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y))
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
                            if (execR(n->Lson,&x))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y))
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
                            if (execR(n->Lson,&x))
                            {   pair y;

                                DPRINT1(3,"execR: ...temPair at 0x%08x\n",&y);
                                if (execR(n->Lson->Rbro,&y))
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
                        p->R->v.r     *= real_unit(n->v.s);
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

static int execL(node *n, varInfo **v, char **name, int *i, int *shouldFree)
{  int   give;
   node *oldNode;

   DPRINT(2,"execL: ");
   DSHOWFLAVOUR(2,n);
   DPRINT(2,"...\n");
   oldNode   = doingNode;
   doingNode = n;
   switch (n->flavour)
   { case ID:      if ((*name)=execN(n->Lson,shouldFree))
                   {  if ((*v)=findVar(*name))
                      {  (*i) = 0;
                         give = 1;
                      }
                      else  /* create only local and temp variables */
                         if (*(*name) == '$' || *(*name) == '%')
                         {  if ((*v)=createVar(*name))
                            {  (*i) = 0;
                               give = 1;
                            }
                            else
                            {  WerrprintfWithPos("Can't create \"%s\"",*name);
                               give = 0;
                            }
                         }
                         else
                         {  WerrprintfWithPos("Variable \"%s\" doesn't exist.",*name);
                            give = 0;
                         }
                   }
                   else
                      give = 0;
                   break;
     case LB:
	           if ((*name)=execN(n->Lson,shouldFree))
                      if ((*v)=findVar(*name))
                      {  pair p;

                         DPRINT1(3,"execL: ...temPair at 0x%08x\n",&p);
                         if (execR(n->Lson->Rbro,&p))
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
                               }
                            }
                            else
                            {  WerrprintfWithPos("Index must be numeric scalar");
                               give = 0;
                            }
                         else
                            give = 0;
                         cleanPair(&p);
                      }
                      else					/* did NOT find name */
                      {  if (*name[0] == '$')
                         { if ((*v)=createVar(*name))		/* only create temporary arrays */
                           {  pair p;

                              DPRINT1(3,"execL: ...temPair at 0x%08x\n",&p);
                              if (execR(n->Lson->Rbro,&p))
                                 if ((p.T.size == 1) && (p.T.basicType == T_REAL))
                                 {  int j;

                                    j = (int)(p.R->v.r);
                                    if ((0 < j) && (j <= ((*v)->T.size+1)))
                                    {  (*i) = j;
                                       give = 1;
                                    }
                                    else
                                    {  WerrprintfWithPos("Index out of bounds");
                                       give = 0;
                                    }
                                 }
                                 else
                                 {  WerrprintfWithPos("Index must be numeric scalar");
                                    give = 0;
                                 }
                              cleanPair(&p);
                           }
                           else
                           {  WerrprintfWithPos("Can't create \"%s\"",*name);
                              give = 0;
                           }
                         }
                         else
                         {   WerrprintfWithPos("Variable \"%s\" doesn't exist.",*name);
                             give = 0;
                         }
                     }
                   else
                      give = 0;
                   break;
     default:      WerrprintfWithPos("BUG! bad flavour (=%d) noticed in execL",n->flavour);
                   give        = 0;
                   *shouldFree = 0;
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
int
execS(n)                            node *n;
{
    /*extern int  interuption;*/
    int         give;
    node       *oldNode;
    static int  macro_active = 0;

    /*if (interuption)
      {
      DPRINT(1,"execS: Interuption detected!, forced failure\n");
      give = 0;
      }
      else*/
    {
	DPRINT(2,"execS: ");
	DSHOWFLAVOUR(2,n);
	DPRINT(2,"...\n");
	oldNode   = doingNode;
	doingNode = n;
	if (mustReturn){
	    DPRINT(3,"execS: ...skip it, returning! (gives 1)\n");
	    give = 1;
	}else{
	    switch (n->flavour){
	      case _Car:
		{
		    char *name;
		    int   shouldFree;
		    
		    if (name=execN(n->Lson->Lson,&shouldFree)){
			give = execMP(name,n->Lson->Rbro,n->Lson->Rbro->Rbro);
		    }else{
			give = 0;
		    }
		    if (shouldFree){
			DPRINT1(3,"execS: ...releasing name %s\n",name);
			release(name);
		    }
		}
		break;
	      case _Ca_:
		{
		    char *name;
		    int   shouldFree;
		    
		    if (name=execN(n->Lson->Lson,&shouldFree)){
			give = execMP(name,n->Lson->Rbro,NULL);
		    }else{
			give = 0;
		    }
		    if (shouldFree){
			DPRINT2(3,"execS: ...releasing name %s (at 0x%08x\n",
				name,name);
			release(name);
		    }
		}
		break;
	      case _C_r:
		{
		    char *name;
		    int   shouldFree;

		    if (name=execN(n->Lson->Lson,&shouldFree)){
			give = execMP(name,NULL,n->Lson->Rbro);
		    }else{
			give = 0;
		    }
		    if (shouldFree){
			DPRINT2(3,"execS: ...releasing name %s (at 0x%08x\n",
				name,name);
			release(name);
		    }
		}
		break;
	      case _C__:
		{
		    char *name;
		    int   shouldFree;

		    if (name=execN(n->Lson->Lson,&shouldFree)){
			give = execMP(name,NULL,NULL);
		    }else{
			give = 0;
		    }
		    if (shouldFree){
			DPRINT2(3,"execS: ...releasing name %s (at 0x%08x\n",
				name,name);
			release(name);
		    }
		}
		break;
	      case CM:
		if (execS(n->Lson)){
		    give = execS(n->Lson->Rbro);
		}else{
		    give = 0;
		}
		break;
	      case DONT:
		ignoreBomb = 0;
		break;
	      case ELSE:
		{
		    pair p;

		    DPRINT1(3,"execS: ...temPair at 0x%08x\n",&p);
		    if (execR(n->Lson,&p)){
			if (isTrue(&p)){
			    DPRINT(3,"execS: ...IF-THEN-ELSE-FI exp true\n");
			    give = execS(n->Lson->Rbro);
			}else{
			    DPRINT(3,"execS: ...IF-THEN-ELSE-FI exp false\n");
			    give = execS(n->Lson->Rbro->Rbro);
			}
		    }else{
			DPRINT(3,"execS: ...IF-THEN-ELSE-FI exp failed\n");
			give = 0;
		    }
		    cleanPair(&p);
		}
		break;
	      case EOL:
		{
		    give = 1;
		    break;
		}
	      case EQ:
		{
		    char    *name;
		    int      shouldFree;
		    int      i;
		    varInfo *v;
		    
		    if (execL(n->Lson,&v,&name,&i,&shouldFree)){
			pair p;

			DPRINT1(3,"execS: ...temPair at 0x%08x\n",&p);
			if (execR(n->Lson->Rbro,&p)){
			    char *argv[2];
			    char  function[20];
			    int   argc;
			    char *par_macro;

			    if (!(v->prot & P_VAL)){
				if (checkParm(&p,v,name)){
				    v->active = ACT_ON; /* Force active */
				    give = assignPair(&p,v,i);
				    if ((v->prot & P_MAC) && (!macro_active)){
					macro_active = 1;
					par_macro = newString("_");
					par_macro = newCatId(par_macro,name,
							     "execS");
					par_macro = newCatId(par_macro,
							     "\n","execS");

					execString(par_macro);
					macro_active = 0;
					release(par_macro);
				    }
				    /*if ((v->Ggroup == G_ACQUISITION) &&
				      (name[0]!='$') && !(v->prot & P_NOA))
				      checkarray(name,v->T.size);*//*CMP*/
				    /* create a list of changed variables */
				    /* to be passed to the dg or ds cmd if */
				    /* neccessary after cmd line is executed */
				    if (!(v->prot & P_REX) && (name[0]!='$')){
					appendvarlist(name);
				    }
				    /*if (v->prot & P_SYS)
				      check_datastation();*//*CMP*/
				}else{
				    give = 0;
				}
			    }else{
				WerrprintfWithPos("entry of parameter %s is not allowed",name);
				give = 0;
			    }
			}else{
			    give = 0;
			}
			cleanPair(&p);
		    }else{
			give = 0;
		    }
		    if (shouldFree){
			DPRINT2(3,"execS: ...releasing name %s (at 0x%08x\n",
				name,name);
			release(name);
		    }
		}
		break;
	      case BOMB:
		give = 0;
		break;
	      case EXIT:
		mustReturn = 1;
		DPRINT(3,"execS: ...return mode set to ON\n");
		give = macArgvRetv(1,n->Lson);
		break;
	      case IGNORE:
		ignoreBomb = 1;
		break;
	      case POP:
		popTree();
		give = 1;
		break;
	      case PUSH:
		pushTree();
		give = 1;
		break;
	      case SHOW:
		{
		    char             *name;
		    char              s[1024];

		    int               shouldFree;
		    int               i;
		    varInfo          *v;

		    if (execL(n->Lson,&v,&name,&i,&shouldFree)){
			pair p;
			if (valueOf(v,i,&p)){
			    /* if we have a good value */
			    give = 1;
			    if (isActive(v)){
				if (0 < i){ /* indexed variable ? */
				    sprintf(s,"%s[%d] = ",name,i);
				}else{
				    sprintf(s,"%s = ",name);
				}
				strncat(s,buildValue(&p),80);
				DPRINT1(3,"execS:.temPair at 0x%08x\n",&p);
			    }else{
				if (0 < i){ /* indexed variable ? */
				     sprintf(s,"%s[%d] = Not Used",name,i);
				}else{
				    sprintf(s,"%s = Not Used",name);
				}
				DPRINT1(3,"execS:.temPair at 0x%08x\n",&p);
			    }
			    Winfoprintf(" %s",s);
			}else{
			    WerrprintfWithPos("%s[%d] index out of bounds",
					      name,i);
			    give = 0;
			}
			cleanPair(&p);
		    }else{
			give = 0;
		    }
		    if (shouldFree){
			DPRINT2(3,"execS: ...releasing name %s (at 0x%08x\n",
				name,name);
			release(name);
		    }
		}
		break;
	      case THEN:
		{
		    pair p;

		    DPRINT1(3,"execS: ...temPair at 0x%08x\n",&p);
		    if (execR(n->Lson,&p)){
			if (isTrue(&p)){
			    DPRINT(3,"execS: IF-THEN-FI exp is true\n");
			    give = execS(n->Lson->Rbro);
			}else{
			    DPRINT(3,"execS: IF-THEN-FI exp is false\n");
			    give = 1;
			}
		    }else{
			DPRINT(3,"execS: IF-THEN-FI exp failed\n");
			give = 0;
		    }
		    cleanPair(&p);
		}
		break;
	      case WHILE:
		{
		    int looping;
		    int ival;

		    looping = 1;
		    while (looping && !mustReturn){
			pair p;

			DPRINT1(3,"execS: ...temPair at 0x%08x\n",&p);
			ival = execR(n->Lson,&p);
			if (ival){
			       if (isTrue(&p)){
				   DPRINT(3,"execS: ...WHILE-DO-OD exp true\n");
				   ival = execS(n->Lson->Rbro);
				   if (ival == 0){
				       looping = 0;
				       give    = 0;
				   }else{
				       give = 1;
				   }
			       }else{
				   DPRINT(3,"execS:...WHILE-DO-OD exp false\n");
				   looping = 0;
				   give    = 1;
			       }
			}else{
			    DPRINT(3,"execS: WHILE-DO-OD exp failed\n");
			    looping = 0;
			    give    = 0;
			}
			cleanPair(&p);
		    }
		}
		break;
	      case REPEAT:
		{
		    int looping;

		    looping = 1;
		    while (looping && !mustReturn){
			if (execS(n->Lson->Rbro)){
			    pair p;

			    DPRINT1(3,"execS: ...temPair at 0x%08x\n",&p);
			    if (execR(n->Lson,&p)){
				if (isTrue(&p)){
				    DPRINT(3,"execS: ...REPEAT-UNTIL true\n");
				    looping = 0;
				    give    = 1;
				}else{
				    DPRINT(3,"execS: ...REPEAT-UNTIL false\n");
				    give = 1;
				}
			    }else{
				DPRINT(3,"execS: ...REPEAT-UNTIL exp failed\n");
				looping = 0;
				give    = 0;
			    }
			    cleanPair(&p);
			}else{
			    looping = 0;
			    give    = 0;
			}
		    }
		}
		break;
	      default:
		WerrprintfWithPos("BUG! execS has funny flavour (=%d)",
				  n->flavour);
		give = 0;
	    }
	}
    }
    if (give==0 && mustReturn==0 && ignoreBomb){
	DPRINT3(3,"execS: ...give=%d, but mustReturn=%d and ignoreBomb=%d, ignoreable bomb!!\n",
		give,mustReturn,ignoreBomb);
	give = 1;
    }
    DPRINT(3,"execS: ...");
    DSHOWFLAVOUR(3,n);
    DPRINT1(3," gives %d\n",give);
    doingNode = oldNode;
    return(give);
}

static double
real_unit(str_val)
  char str_val[];
{
    double val = 1.0;
    char units[256];
    double dummy;

    sscanf(str_val,"%g%s",&dummy,units);
    if (strcmp(units,"d") == 0){
	P_getreal(CURRENT,"dfrq",&val,1);
    }else if (strcmp(units,"k") == 0){
	val = 1000.0;
    }else if (strcmp(units,"p") == 0){
	P_getreal(CURRENT,"sfrq",&val,1);
    }else{
	Werrprintf("unknown unit %s",units);
    }
    return(val);
}
