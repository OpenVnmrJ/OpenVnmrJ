/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "abort.h"

extern int bgflag;	/* debugging flag */
extern int newacq;

#define STKMAX  20

int validrtvar(codeint rtvar);

static  codeint loopstack[90];	/* sydo stack for software loops */
static  codeint stackindex = 0;		/* number of loop 1-14  14-MAX */

/* looping timing support */
static double looptimestack[STKMAX];
static int timestackindex = 0;

extern  codeint pop();
void push(codeint word);
extern  double looptimepop();
void looptimepush(double word);
/*-------------------------------------------------------------------
|
|	loop(count,counter)
|	This is the software dynamic looping routine.
|	count is a real time variable (v1-v14) of the # of times to cycle
|	counter is a real time variable (v1-v14) that will be the loop counter
|	Software loop is terminated by the endloop element.
|
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   6/15/89   Greg B.     1. Use new global parameters to calc acode offsets 
+--------------------------------------------------------------------*/
/* codeint count;	 number of times to cycle */
/* codeint counter;	 offset to real time variable tobe the loop counter */
void loop(codeint count, codeint counter)
{
    codeint ifoffset;	/* offset to beginning of loop */
    codeint ioffset;	/* offset to branch to skip loop */

    if (bgflag)
	fprintf(stderr,"loop(): count: %d,  counter: %d \n",count,counter);
    notinhwloop("loop");
    if ( validrtvar(count) && validrtvar(counter) )
    {
	assign(zero,counter); /* zero counter range 1 - 2e31-1 */
	putcode(IFMIFUNC);
	putcode(count);
	putcode(one);	/* if count < 1 skip loop */
/*	ioffset = (int) (Codeoffset - codestart); */
/*	ioffset = (int) (Codeptr - Alc); */
	ioffset = (int) (Codeptr - Aacode);
	putcode(0);	/* branch offset tobe updated by endloop */
/*	ifoffset = (int) (Codeoffset - startofAcode); */
	ifoffset = (int) (Codeptr - Aacode);
	push(ifoffset);	/* push start loop position on to stack */
	push(ioffset);	/* push branch position on to stack */
	push(count);	/* push count on to stack */
	push(counter);	/* push counter on to stack */
    } 				 /* sydo stack */
    else
    {
	text_error("A non-realtime variable specified in call to loop.\n");
	psg_abort(1);
    }
    /* Init timing */
    looptimepush(totaltime);
}
/*-------------------------------------------------------------------
|
|	endloop(counter)
|	This is the software dynamic looping routine.
|	counter is a real time variable (v1-v14) that will be the loop counter
|	Software loop is terminated by this endloop element.
|	the counter is compared to the loop counter to be sure loops
|	 have been nested properly
|
+--------------------------------------------------------------------*/
/* codeint    counter;	offset to real time variable tobe the loop counter */
void endloop(codeint counter)
{
    codeint icount;		/* previous count push on stack by loop() */
    codeint lcounter;	/* previous counter push on stack by loop() */
    codeint looppos;	/* Offset to beginning of loop */
    codeint branchpos;	/* Offset to skip loop branch */
    codeint endloopoffset;/* offset to this endloop branch */
    codeint *branchptr;	/* absolute address of ifzero branch offset */
    double thislooptime;	/* timing variable */
    int	thisloopcount;		/* timing variable */

    if (bgflag)
	fprintf(stderr,"endloop(): counter: %d \n",counter);
    notinhwloop("endloop");
    lcounter = pop();	/* pop realtime loop counter off of stack */
    icount = pop();	/* pop realtime count off of stack */
    branchpos = pop();	/* pop branch position off of stack */
    looppos = pop();	/* pop loop posisition off of stack */
    if (counter == lcounter)
    {
	incr(counter);		/* increment counter at end of loop */
	putcode(IFMIFUNC);	/* if minus go to next loop or statement */
	putcode(lcounter);	/* real time loop counter (v1-v14) */
	putcode(icount);	/* real time count,  to do loop */
	putcode(looppos);	/* offset to beginning of loop */
/*        endloopoffset = (codeint) (Codeoffset - startofAcode);*/
        endloopoffset = (codeint) (Codeptr - Aacode);
/*        branchptr = codestadr +  branchpos; */
        branchptr = Aacode +  branchpos;
       *branchptr = endloopoffset;/*update loop branch to proper offset */
    }
    else
    {
	text_error("Illegal nesting of software loops\n");
        psg_abort(1);
    }
    /* Update time for loop */
    thisloopcount = get_acqvar(icount); /* get count for this loop	*/
    thislooptime = totaltime - looptimepop();

    /* Add time to totaltime.  The time to add is only for the loops 	*/
    /* greater than one.  The first loop has already been added to the	*/
    /* totaltime. 							*/
    if (thisloopcount > 1)		
	totaltime += (thislooptime * (thisloopcount - 1));
}
/*-----------------------------------------------------------------
|
|	ifzero(pointer)
|	real time 'if'  statement.
|	if real time variable (v1-v14) = 0) then do Acodes to
|	the elsenz or endif statement.
+-----------------------------------------------------------------*/
/* codeint rtvar;	offset to real time variable v1-v14 */
void ifzero(codeint rtvar)
{   codeint offset;

    notinhwloop("ifzero");
    putcode(IFNZFUNC);
    putcode(rtvar);
    putcode(zero);		/* offset to real time zero */
    offset = (codeint) (Codeptr - Aacode);  /* relative from beginning*/
    putcode(0);		/* branch offset tobe updated later */
    push(offset);	/* push location of branch offset onto stack */
    push(rtvar);		/* what v# using */
    if (bgflag)
	fprintf(stderr,"ifzero(): rtvar: %d, offset: %d \n",rtvar,offset);
}
/*-----------------------------------------------------------------
|
|	ifmod2zero(pointer)
|	real time 'if'  statement.
|	if real time variable (v1 % 2) = 0) then do Acodes to
|	the elsenz or endif statement.
+-----------------------------------------------------------------*/
/* codeint rtvar;	offset to real time variable v1-v14 */
void ifmod2zero(codeint rtvar)
{   codeint offset;

    notinhwloop("ifzero");
    putcode(IFNZFUNC);
    putcode(rtvar);
    putcode(zero);		/* offset to real time zero */
    offset = (codeint) (Codeptr - Aacode);  /* relative from beginning*/
    putcode(0);		/* branch offset tobe updated later */
    push(offset);	/* push location of branch offset onto stack */
    push(rtvar);		/* what v# using */
    if (bgflag)
	fprintf(stderr,"ifmod2zero(): rtvar: %d, offset: %d \n",rtvar,offset);
}
/*-----------------------------------------------------------------
|	 elsenz(pointer)
|	Used in conjunction with the ifzero statment.
|	if real time variable (v1-v14) != 0) then do Acodes to
|	the endif statement.
+-----------------------------------------------------------------*/
void elsenz(codeint rtvar)
{   codeint ifrtvar;	/* real time variable used in last ifzero */
    codeint ifzerobranch;	/* offset to ifzero branch offset */
    codeint elsenzoffset;	/* offset to this elsnz branch */
    codeint *branchptr;	/* absolute address of ifzero branch offset */
    codeint offset;	/* offset location of this elsenz branch offset */

    notinhwloop("elsenz");
    ifrtvar = pop();
    if (bgflag)
	fprintf(stderr,
	  "elsenz(): rtvar: %d, ifzero(): rtvar: %d \n",rtvar,ifrtvar);
    if (rtvar != ifrtvar)
    {
	text_error("Mismatched ifzero & elsenz elements\n");
	psg_abort(1);
    }
    ifzerobranch = pop();	/* offset location of ifzero branch */
    putcode(BRANCH);		/* make ifzero code jump past the elsenz */
/*    offset = (codeint) (Codeoffset - codestart); */
    offset = (codeint) (Codeptr - Aacode);
    putcode(0);			/* branch offset to updated by endif */
/*    elsenzoffset = (codeint) (Codeoffset - startofAcode); */
    elsenzoffset = (codeint) (Codeptr - Aacode);
/*    branchptr = codestadr +  ifzerobranch; */
    branchptr = Aacode +  ifzerobranch;
    *branchptr = elsenzoffset;	/* update ifzero branch to proper offset */
    if (newacq)
    {
	/* For inova interpreter update ifzero acode. */
	branchptr = branchptr - 3;
	*branchptr = IFNZBFUNC;
    }
    if (bgflag)
	fprintf(stderr,"elsenz(): rtvar: %d, offset: %d \n",rtvar,offset);

    push(offset);	/* push where to branch offset onto stack */
    push(rtvar);	/* what v# using */
}
/*-----------------------------------------------------------------
|	endif(pointer)
|	use in conjuction with ifzero or ifzero elsenz statements
|	closes the if or if else statement.
+-----------------------------------------------------------------*/
void endif(codeint rtvar)
{   codeint ifrtvar;	/* real time variable used in last ifzero */
    codeint ifzerobranch;	/* offset to ifzero branch offset */
    codeint endifoffset;	/* offset to this endif */
    codeint *branchptr;	/* absolute address of ifzero branch offset */

    notinhwloop("endif");
    ifrtvar = pop();
    if (bgflag)
	fprintf(stderr,
	   "endif(): rtvar: %d, ifzero(): rtvar: %d \n",rtvar,ifrtvar);
    if (rtvar != ifrtvar)
    {
	text_error("Mismatched ifzero or elsenz with endif elements\n");
	psg_abort(1);
    }
    ifzerobranch = pop();	/* offset location of ifzero or elsenz branch */
/*    endifoffset = (codeint) (Codeoffset - startofAcode); */
/*    branchptr = codestadr +  ifzerobranch;	*/
    endifoffset = (codeint) (Codeptr - Aacode);
    branchptr = Aacode +  ifzerobranch;
    *branchptr = endifoffset;/*update ifzero or elsnz branch to proper offset */
}
/*-----------------------------------------------------------------
|	push
|	pushes a word on to the sydo stack
|
+-------------------------------------------------------------------*/
void push(codeint word)
{
    if (bgflag)
	fprintf(stderr,"push(): stack: %d, word: %d \n",stackindex,word);
    if ( stackindex < 90)
    {
    	loopstack[stackindex] = word;
    	stackindex++;
    }
    else
    {
	text_error("Too many nested loops or if statements(MAX 20)\n");
	psg_abort(1);
    }
}
/*-----------------------------------------------------------------
|	pop()
|	pops word from the sydo stack
|
+-------------------------------------------------------------------*/
codeint pop()
{
    codeint word;

    if (stackindex > 0)
    {
    stackindex--;
    word = loopstack[stackindex];
    }
    else
    {
	text_error("missing loop() or ifzero()\n"); 
	psg_abort(1);
    }
    if (bgflag)
	fprintf(stderr,"pop(): stack: %d, word: %d \n", stackindex,word);
    return(word);
}
/*---------------------------------------------------------------
|
|	validrtvar(rtvar)
|	checks to see if the real time variable rtvar is a valid one 
|
+-----------------------------------------------------------------*/
#define OK 1
#define NOTOK 0
/* codeint rtvar;		an offset to real time variable */
int validrtvar(codeint rtvar)
{
    if (v1 <= rtvar && rtvar <= v14)
        return(OK);
    if ( zero <= rtvar && rtvar <= three)
	return(OK);
    if (rtvar == oph)
	return(OK);
    if (rtvar == ct)
	return(OK);
    if (rtvar == bsctr)
	return(OK);
    return(NOTOK);		/* not a valid real time variable */
}

/*-----------------------------------------------------------------
|	looptimepush
|	pushes a word on to the looptime stack
|
+-------------------------------------------------------------------*/
void looptimepush(double word)
{
    if (bgflag)
	fprintf(stderr,"looptimepush(): stack: %d, word: %g \n",
							timestackindex,word);
    if ( timestackindex < STKMAX)
    {
    	looptimestack[timestackindex] = word;
    	timestackindex++;
    }
    else
    {
	text_error("Too many nested loops or if statements(MAX 20)\n");
	psg_abort(1);
    }

}
/*-----------------------------------------------------------------
|	looptimepop()
|	pops word from the looptime stack
|
+-------------------------------------------------------------------*/
double looptimepop()
{
    double word;

    if (timestackindex > 0)
    {
    timestackindex--;
    word = looptimestack[timestackindex];
    }
    else
    {
	text_error("missing loop() or ifzero()\n"); 
	psg_abort(1);
    }
    if (bgflag)
	fprintf(stderr,"looptimepop(): stack: %d, word: %g \n", 
						timestackindex,word);
    return(word);
}

/* Dummy functions */

void parallelstart(const char *chanType)
{
   abort_message("parallel events not supported on this console\n"); 
}

double parallelend()
{
   abort_message("parallel events not supported on this console\n"); 
}
