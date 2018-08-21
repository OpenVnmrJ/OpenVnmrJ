/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include "ACode32.h"
#include "macros.h"
#include "acqparms.h"
#include "abort.h"

extern int bgflag;	            /* debugging flag */

#define STKMAX  20
#define LOOP_TICKER      1
#define IFZ_TRUE_TICKER  2
#define IFZ_FALSE_TICKER 3
#define RL_LOOP_TICKER   6

static  int loopstack[90];	    /* stack for software loops */
static  int stackindex = 0;         /* number of loop 1-32  32-MAX */
static  int loopstack2[90][2];         /* stack for software loop count */
static  int stackindex2 = 0;         /* number of loop 1-32  32-MAX */

/* looping timing support */
static double looptimestack[STKMAX];
static int timestackindex = 0;

/* externalize these for explicit acquires */

extern void pushloop(int);
extern void poploop(int);
extern int  get_acqvar(int index);
extern void F_initval(double value, int rtindex);
void pushloopcount(int, int);
int poploopcount();

/*****************************************************
    loop_check()

    Has the interface set the values of ni,nf
    nv,nv2,nv3,ne & ns correctly. If not abort.
******************************************************/
void loop_check()
{
    int    nfp;             /*nf product*/
    int    counts[5];       /*values of loop count variables*/
    int    k;               /*loop index counter*/

    counts[0] = IRND(ne);      /*Number of echoes*/
    counts[1] = IRND(ns);      /*Number of slices*/
    counts[2] = IRND(nv);      /*Number of 2D views*/
    counts[3] = IRND(nv2);     /*Number of 3D views*/
    counts[4] = IRND(nv3);     /*Number of 4D views*/

/*--------------------------------------------------------
    nfp is the product of all compressed loop counts
    present in seqcon.
---------------------------------------------------------*/
    nfp = 1;
    for (k=0; k<5; k++) {
        if (seqcon[k] == 'c'  &&  counts[k] > 0) {
            nfp = nfp*counts[k];
        }
    }

    if ((int)(nf + 0.5) != nfp) {
        printf("loop_check: nf set incorrectly.\n");
        printf("Use setloop command to reset nf.\n");
        psg_abort(1);
    }
}

/********************************************************
                         peloop()

    Procedure to provide a switchable phase encode loop header.
    The APV variables hold the max count and the current count
    for the loop.
*********************************************************/
void peloop(char state, double max_count, int apv1, int apv2)
{
    switch(state) {
        /*-------------------------------------------------
         Standard encode loop: Put the current 2D
	 increment value into APV2.  APV2 counts 0 up to
	 maxcount-1. APV1 holds the maximum count.
        -------------------------------------------------*/
        case 's':
            if (max_count > 0.5) {
                F_initval(max_count,apv1);
                F_initval((double)(d2_index),apv2);
            }
            else {
                assign(zero,apv1);
                assign(zero,apv2);
            }
            break;

        /*-------------------------------------------------
         Compressed Encode Loop: APV2 counts 0 up because
	 of loop(). APV1 holds the maximum count.      
        -------------------------------------------------*/
        case 'c':
            if (max_count > 0.5) {
                F_initval(max_count,apv1);
                loop(apv1,apv2);
            }
            else {
                F_initval(1.0,apv1);
                loop(apv1,apv2);
            }
            break;


        /*-------------------------------------------------
         Any other state is invalid
        -------------------------------------------------*/
        default:
            text_error("peloop: Illegal loop state\n");
            psg_abort(1);
    }
}


/********************************************************
                         peloop2()

    Procedure to provide a switchable phase encode loop header.
    The APV variables hold the max count and the current count
    for the loop.
*********************************************************/
void peloop2(char state, double max_count, int apv1, int apv2)
{
    switch(state) {
        /*-------------------------------------------------
         Standard encode loop: Put the current 2D
	 increment value into APV2.  APV2 counts 0 up to
	 maxcount-1. APV1 holds the maximum count.
        -------------------------------------------------*/
        case 's':
            if (max_count > 0.5) {
                F_initval(max_count,apv1);
                initval((double)(d3_index),apv2); /*****/
            }
            else {
                assign(zero,apv1);
                assign(zero,apv2);
            }
            break;

        /*-------------------------------------------------
         Compressed Encode Loop: APV2 counts 0 up because
	 of loop(). APV1 holds the maximum count.      
        -------------------------------------------------*/
        case 'c':
            if (max_count > 0.5) {
                F_initval(max_count,apv1);
                loop(apv1,apv2);
            }
            else {
                F_initval(1.0,apv1);
                loop(apv1,apv2);
            }
            break;


        /*-------------------------------------------------
         Any other state is invalid
        -------------------------------------------------*/
        default:
            text_error("peloop: Illegal loop state\n");
            psg_abort(1);
    }
}


/********************************************************
              endpeloop()

    Procedure to close switchable phase encode loop
*********************************************************/
void endpeloop(char state, int apv1)
{
    if (state == 'c') {
        endloop(apv1);
    }
}


/**********************************************************
                        msloop()

        Procedure to provide a sequence switchable
        multislice loop header.

        The two APV variables hold the values of the 
        maximum count (apv1) and the current counter
        value (apv2).
***********************************************************/
void msloop(char state, double max_count, int apv1, int apv2)
{
/*-----------------------------------------------------------
    state='c' designates compressed mode.  Set up the loop()
    of a loop/endloop pair in the sequence.  APV1 is used
    internally to hold the maximium count for the loop.
    APV2 counts 0 to maxcount-1.
-----------------------------------------------------------*/
    if (state == 'c') {
        if (max_count > 0.5) {
            F_initval(max_count,apv1);
            loop(apv1,apv2);
        }
        else {
            F_initval(1.0,apv1);
            loop(apv1,apv2);
        }
    }

/*-----------------------------------------------------------
    state='s' designates standard arrayed mode. APV1 is
    used to hold the max count. APV2 counts 0 up.
-----------------------------------------------------------*/
    else if (state == 's') {
	if (max_count > 1) {
            text_error("msloop: Illegal std multislice array\n");
            psg_abort(1);
	}
        F_initval(max_count,apv1);
        F_initval(0.0,apv2);
    }

/*-----------------------------------------------------------
    Illegal state
-----------------------------------------------------------*/
    else {
        text_error("msloop: Illegal looping state\n");
        psg_abort(1);
    }       
}






/*******************************************************
                       endmsloop()

    Procedure to close switchable multislice loop.
********************************************************/
void endmsloop(char state, int apv1)
{
    if (state == 'c') {
        endloop(apv1);
    }
}



static int hardLoopCycles = 0;
static double extraLoopTime = 0.0;

/*-------------------------------------------------------------------
|
|       starthardloop(count)
|       The only looping routine in DDsi system is the loop
|       The starthardloop will call this loop routine as well
|       count   is a real time variable (v1-v32) of the # of times to cycle
|       loop is terminated by the endhardloop element.
+--------------------------------------------------------------------*/

void starthardloop(int count)
{
   hardLoopCycles = loop(count,vtmp34);
}


double getExtraLoopTime()
{
   double tmp;
   tmp = extraLoopTime;
   extraLoopTime = 0.0;
   return(tmp);
}


/*-------------------------------------------------------------------
|
|       endhardloop()
|       The only looping routine in DDsi system is the loop
|       The starthardloop will call this loop routine as well
|       count   is a real time variable (v1-v32) of the # of times to cycle
|       loop is terminated by the endhardloop element.
+--------------------------------------------------------------------*/

void endhardloop()
{
  endloop(vtmp34);
}


/*-------------------------------------------------------------------
|
|	loop(count,counter)
|	This is the only looping routine in DDsi system
|	The starthardloop will call this loop routine as well
|	count   is a real time variable (v1-v32) of the # of times to cycle
|	counter is a real time variable (v1-v32) that will be the loop counter
|	loop is terminated by the endloop element.
+--------------------------------------------------------------------*/

/* count        number of times to cycle */
/* counter	offset to real time variable tobe the loop counter */
int loop(int count, int counter)
{
    int numcodes, code[3];
    int rtval=1;

    if ( validrtvar(count) && validrtvar(counter) )
    {
      push(counter);
      pushloopcount(count,counter);
      rtval = get_acqvar(count);
      if (rtval <= 0) rtval = 1;
      pushloop(rtval);
      code[0] = count;
      code[1] = counter;
      code[2] = 0;       // no of words to skip on controller to bypass loop for zero count
                         // is filled in later by ENDNVLOOP code
      numcodes= 3;
      broadcastCodes(NVLOOP,numcodes,code);    // changing no of args in NVLOOP, ENDNVLOOP acodes will
                                               // necessitate many changes in startTicker(),
                                               // loopStart(), loopEnd(), etc
      startTicker(LOOP_TICKER);
    } 
    else
    {
      text_error("A non-realtime variable specified in call to loop.\n");
      psg_abort(1);
    }
    return(rtval);
}

/*-------------------------------------------------------------------
|
|	endloop(counter)
|	This is the software dynamic looping routine.
|	counter is a real time variable (v1-v14) that will be the loop counter
|	Software loop is terminated by this endloop element.
|	the counter is compared to the loop counter to be sure loops
|       have been nested properly
+--------------------------------------------------------------------*/

/* counter	offset to real time variable tobe the loop counter */
void endloop(int counter)
{
    int startcounter;
    int numcodes, code[2];
    long long ticker= 0;

    if (validrtvar(counter))
    {
       startcounter = pop();
       if (counter == startcounter)
       {
         ticker = stopTicker(LOOP_TICKER);
         if (hardLoopCycles)
         {
            extraLoopTime += (double)(hardLoopCycles-1) *
                             ((double) ticker * 12.5e-9);
            hardLoopCycles = 0;
         }
         code[0] = 0;
         code[1] = counter;
         numcodes= 2;
         broadcastCodes(ENDNVLOOP,numcodes,code);  // changing no of args in NVLOOP, ENDNVLOOP acodes will
                                                   // necessitate many changes in startTicker(),
                                                   // loopStart(), loopEnd(), etc
         poploop(1);
         poploopcount();
       }
       else
       {
         text_error("illegal nesting of loops in pulse sequence\n");
         psg_abort(1);
       }
    }
    else
    {
	text_error("A non-realtime variable specified in call to endloop.\n");
        psg_abort(1);
    }
}


/*-------------------------------------------------------------------
|
|	rlloop(count, rtcount,counter)
|       count is an integer for the number of loops to do.
|	rtcount   is a real time variable (v1-v32) of the # of times to cycle
|	counter is a real time variable (v1-v32) that will be the loop counter
|	loop is terminated by the endloop element.
+--------------------------------------------------------------------*/

/* count        number of times to cycle */
/* counter	offset to real time variable tobe the loop counter */
int rlloop(int count, int rtcount, int counter)
{
    int numcodes, code[3];
    int rtval=1;

    if ( validrtvar(rtcount) && validrtvar(counter) )
    {
      if (count < 0)
         count = 0;
      initval( (double) count, rtcount);
      push(counter);
      pushloopcount(rtcount,counter);
      rtval = get_acqvar(rtcount);
      if (rtval <= 0) rtval = 1;
      pushloop(rtval);
      code[0] = rtcount;
      code[1] = counter;
      code[2] = 0;       // no of words to skip on controller to bypass loop for zero count
                         // is filled in later by ENDNVLOOP code
      numcodes= 3;
      broadcastLoopCodes(NVLOOP,numcodes,code, count, -1);
      startTicker(RL_LOOP_TICKER);
    } 
    else
    {
      abort_message("A non-realtime variable specified in call to rlloop.\n");
    }
    return(rtval);
}

/*-------------------------------------------------------------------
|
|	rlendloop(counter)
|	This is the software dynamic looping routine.
|	counter is a real time variable (v1-v14) that will be the loop counter
|	Software loop is terminated by this endloop element.
|	the counter is compared to the loop counter to be sure loops
|       have been nested properly
+--------------------------------------------------------------------*/

/* counter	offset to real time variable tobe the loop counter */
void rlendloop(int counter)
{
    int startcounter;
    int numcodes, code[2];
    long long ticker= 0;

    if (validrtvar(counter))
    {
       startcounter = pop();
       if (counter == startcounter)
       {
         ticker = stopTicker(RL_LOOP_TICKER);
         code[0] = 0;
         code[1] = counter;
         numcodes= 2;
         broadcastLoopCodes(ENDNVLOOP,numcodes,code, -1, ticker);
         poploop(1);
         poploopcount();
       }
       else
       {
         abort_message("illegal nesting of loops in pulse sequence\n");
       }
    }
    else
    {
	abort_message("A non-realtime variable specified in call to rlendloop.\n");
    }
}


/*-------------------------------------------------------------------
|
|	kzloop(duration, rtcount,counter)
|       duration is the total time of the loop. The duration of a single
|       pass through the loop is determined and then the loop count is updated
|       Any remaining time is handled as a delay.
|	rtcount   is a real time variable (v1-v32) of the # of times to cycle
|	counter is a real time variable (v1-v32) that will be the loop counter
|	loop is terminated by the endloop element.
+--------------------------------------------------------------------*/

/* counter	offset to real time variable to be the loop counter */
int kzloop(double duration, int rtcount, int counter)
{
    int numcodes, code[3];
    int rtval=1;

    if ( validrtvar(rtcount) && validrtvar(counter) )
    {
      setKzDuration(duration);
      initval( (double) 0.0, rtcount);
      push(counter);
      pushloopcount(rtcount,counter);
      rtval = get_acqvar(rtcount);
      if (rtval <= 0) rtval = 1;
      pushloop(rtval);
      code[0] = rtcount;
      code[1] = counter;
      code[2] = 0;       // no of words to skip on controller to bypass loop for zero count
                         // is filled in later by ENDNVLOOP code
      numcodes= 3;
      broadcastLoopCodes(NVLOOP,numcodes,code, 0, -1);
      startTicker(RL_LOOP_TICKER);
    } 
    else
    {
      abort_message("A non-realtime variable specified in call to kzloop.\n");
    }
    return(rtval);
}

/*-------------------------------------------------------------------
|
|	kzendloop(counter)
|	This is the software dynamic looping routine.
|	counter is a real time variable (v1-v14) that will be the loop counter
|	Software loop is terminated by this endloop element.
|	the counter is compared to the loop counter to be sure loops
|       have been nested properly
+--------------------------------------------------------------------*/

/* counter	offset to real time variable tobe the loop counter */
void kzendloop(int counter)
{
    int startcounter;
    int numcodes, code[2];
    long long ticker= 0;
    double remainingDelay;

    if (validrtvar(counter))
    {
       startcounter = pop();
       if (counter == startcounter)
       {
         ticker = stopTickerKzLoop( &remainingDelay);
         code[0] = 0;
         code[1] = counter;
         numcodes= 2;
         broadcastLoopCodes(ENDNVLOOP,numcodes,code, -1, ticker);
         poploop(1);
         poploopcount();
         if (remainingDelay > 0.0)
            delay(remainingDelay);
       }
       else
       {
         abort_message("illegal nesting of loops in pulse sequence\n");
       }
    }
    else
    {
	abort_message("A non-realtime variable specified in call to kzendloop.\n");
    }
}

/*-----------------------------------------------------------------
|
|	ifzero(pointer)
|	real time 'if'  statement.
|	if real time variable (v1-v32) = 0) then do Acodes to
|	the elsenz or endif statement.
+-----------------------------------------------------------------*/

void ifzero(int rtvar)
{   
  if (validrtvar(rtvar))
  {
    startTicker(IFZ_TRUE_TICKER);
    ifzero_bridge(rtvar); 
  }
  else
  {
    text_error("A non-realtime variable specified in call to ifzero.\n");
    psg_abort(1);
  }
}

/*-----------------------------------------------------------------
|
|	ifmod2zero(pointer)
|	real time 'if mod2 '  statement.
|	if real time variable (v1-v32) = 0) then do Acodes to
|	the elsenz or endif statement.
+-----------------------------------------------------------------*/

void ifmod2zero(int rtvar)
{   
  if (validrtvar(rtvar))
  {
    startTicker(IFZ_TRUE_TICKER);
    ifmod2zero_bridge(rtvar); 
  }
  else
  {
    text_error("A non-realtime variable specified in call to ifmod2zero.\n");
    psg_abort(1);
  }
}


/*-----------------------------------------------------------------
|	 elsenz(pointer)
|	Used in conjunction with the ifzero statment.
|	if real time variable (v1-v32) != 0) then do Acodes to
|	the endif statement.
+-----------------------------------------------------------------*/

void elsenz(int rtvar)
{ 
  if (validrtvar(rtvar))
  {
    startTicker(IFZ_FALSE_TICKER);
    elsenz_bridge(rtvar);
  }
  else
  {
    text_error("A non-realtime variable specified in call to elsenz.\n");
    psg_abort(1);
  }
}


/*-----------------------------------------------------------------
|	endif(pointer)
|	use in conjuction with ifzero or ifzero elsenz statements
|	closes the if or if else statement.
+-----------------------------------------------------------------*/

void endif(int rtvar)
{   
  if (validrtvar(rtvar))
  {
    endif_bridge(rtvar);
  }
  else
  {
    text_error("A non-realtime variable specified in call to endif.\n");
    psg_abort(1);
  }
}


/*-----------------------------------------------------------------
|	push
|	pushes a word on to the sydo stack
|
+-------------------------------------------------------------------*/
void push(int word)
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
int pop()
{
    int word = 0;

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

/*-----------------------------------------------------------------
|       pushloopcount
|       pushes a word on to the sydo stack
|
+-------------------------------------------------------------------*/
void pushloopcount(int loopindex, int loopctr)
{
    if (bgflag)
        fprintf(stderr,"pushloopcount(): loopindex: %d \n",loopindex);
    if ((stackindex2 < 90) && (stackindex2 >= 0))
    {
        loopstack2[stackindex2][0] = get_acqvar(loopindex);
        loopstack2[stackindex2][1] = loopctr;
        stackindex2++;
    }
    else
    {
        text_error("Too many nested loops or if statements(MAX 20)\n");
        psg_abort(1);
    }
}

/*-----------------------------------------------------------------
|       poploopcount()
|       pops word from the sydo stack
|
+-------------------------------------------------------------------*/
int poploopcount()
{
    int word = 0;

    if ((stackindex2 > 0) && (stackindex2 < 90))
    {
      stackindex2--;
      word = loopstack2[stackindex2][0];
    }
    else
    {
        text_error("missing loop() or ifzero()\n");
        psg_abort(1);
    }
    if (bgflag)
        fprintf(stderr,"poploopcount(): stack: %d, word: %d \n", stackindex2,word);
    return(word);
}

/*-----------------------------------------------------------------
|       getloopdepth()
+-------------------------------------------------------------------*/
int getloopdepth()
{
   int depth = 1;
   int i;

   for (i=0; i<stackindex2; i++)
   {
     depth *= loopstack2[i][0];
   }
   return depth;
}

/*-----------------------------------------------------------------
|       getcurrentloopcount()
+-------------------------------------------------------------------*/
int getcurrentloopcount()
{
   if (loopstack2[stackindex2-1][0] > 0)
     return (loopstack2[stackindex2-1][0]);
   else
     return 1;
}


/*-----------------------------------------------------------------
|       getloopcount_for_index(int loopind)
+-------------------------------------------------------------------*/
int getloopcount_for_index(int loopind)
{
   int i;
   if ( (loopind < v1) || (loopind > v42))
     return (-1);

   for (i=0; i<stackindex2; i++)
   {
     if (loopstack2[i][1] == loopind)
     {
       return loopstack2[i][0];
     }
   }
   return (-1);
}

/*---------------------------------------------------------------
|
|	validrtvar(rtvar)
|	checks to see if the real time variable rtvar is a valid one 
|
+-----------------------------------------------------------------*/
#define OK 1
#define NOTOK 0
int validrtvar(int rtvar)
{
    if (v1 <= rtvar && rtvar <= VVARMAX)
        return(OK);
    if ( zero <= rtvar && rtvar <= three)
	return(OK);
    if (rtvar == oph)
	return(OK);
    if (rtvar == ct)
	return(OK);
    if (rtvar == bsctr)
	return(OK);
    if (rtvar == ssctr)
        return(OK);
    if (rtvar == ctss)
        return(OK);
    if (rtvar == (oph - 8))
        return(OK);  /* code for il_cntr */
    if (rtvar == spare1rt)
        return(OK);
    return(NOTOK);		/* not a valid real time variable */
}

/*-----------------------------------------------------------------
|	looptimepush
|	pushes a word on to the looptime stack
|
+-------------------------------------------------------------------*/
/* word	 push on stack */
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
    double word = 0.0;

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
