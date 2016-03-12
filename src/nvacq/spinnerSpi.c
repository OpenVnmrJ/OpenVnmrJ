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
#include <string.h>
#include "Console_Stat.h"
#include "acqcmds.h"
#include "logMsgLib.h"
#include "spinner.h"
#include "spinObj.h"

extern Console_Stat	*pCurrentStatBlock;	/* Acqstat-like status block */
extern SPIN_ID		pTheSpinObject;		/* spinner object */

extern volatile unsigned int 	*pMASTER_SamplePresent;

extern int		sampleHasChanged;	/*Global Sample Change Flag*/

void liquidsSpinner(int *paramvec, int *index, int count)
{
int token;
   DPRINT( 1,"liquidsSpinner(): got here\n");
   DPRINT2(-1,"index=%d, count=%d\n",*index,count);
   while (*index < count)
   { 
      token = paramvec[*index]; (*index)++;
      switch(token) {
      case BEAROFF:
          DPRINT(-1, "SETRATE\n");
//   should speed be set to zero? But do not override value in struct.
          setBearingAir(0);
	  break;
      case BEARON:
          DPRINT(-1, "BEARON\n");
// should the speed be set again as well?
          setBearingAir(1);
          break;
      case BEARING:
	  token = paramvec[*index]; (*index)++;
          setBearingLevel(token);
          break;
      case BUMPSAMPLE:
          DPRINT( 1, "BUMPSAMPLE\n");
          bumpSample();
          break;
      case EJECT:
          DPRINT(-1,"EJECT\n");
          ejectSample();
          break;
      case INSERT:
          DPRINT(-1,"INSERT\n");
          insertSample();
	  break;
      case EJECTOFF:
          DPRINT(-1,"EJECTOFF\n");
          ejectOff();
	  break;
      case SETMASTHRES:
	  token = paramvec[*index]; (*index)++;
          DPRINT1(-1,"SETMASTHRES %d\n",token);
          setMASThres(token);
          break;
      case SETRATE:
	  token = paramvec[*index]; (*index)++;
          DPRINT1(-1,"SETRATE %d\n",token);
          setRate(token);
          break;
      case SETSPD:
	  token = paramvec[*index]; (*index)++;
          DPRINT1(-1,"SETSPD %d\n",token);
//  need to replace this with a call into spinObj
//    autoSpinSpeedSet( pTheAutoObject, (unsigned short) paramvec[ iter ] );
          setSpeed(token);
          pCurrentStatBlock->AcqSpinSet = (short) token;
          break;
      default:
	  printf("%d not supported by liquidsSpinner (yet)\n",token);
          break;
      } /* end of switch */
   } /* end of while */
}

int bearAirBeforeEjectValue;
int ejectSample()
{
   if (pTheSpinObject == NULL) return(ERROR);

   sampleHasChanged = TRUE;
// should bearing be on as well, speed zero?
   bearAirBeforeEjectValue = pTheSpinObject->bearDAC;
   setBearingAir(1);
   setOffTime(0);
   hsspi(2,0x14ffff);
   pTheSpinObject->ejectDAC = 0xffff;
   pCurrentStatBlock->AcqSample = (long) 0;
   pCurrentStatBlock->AcqLSDVbits &= ~LSDV_EJECT;
   return(OK);
}

int ejectOff()
{
   if (pTheSpinObject == NULL) return(ERROR);

   if (pTheSpinObject->ejectDAC)
   {
      hsspi(2,0x140000);
      pTheSpinObject->ejectDAC = 0x0;
      setBearingAir(0);
      pCurrentStatBlock->AcqLSDVbits |= LSDV_EJECT;
   }
   return(OK);
}

int insertSample()
{
   if (pTheSpinObject == NULL) return(ERROR);

   resetLastSpinSpeed();
   if (pTheSpinObject->ejectDAC)
   {
      hsspi(2,0x149000); taskDelay(calcSysClkTicks(2000)); pTheSpinObject->ejectDAC = 0x9000;
      hsspi(2,0x148800); taskDelay(calcSysClkTicks(2000)); pTheSpinObject->ejectDAC = 0x8800;
      hsspi(2,0x148000); taskDelay(calcSysClkTicks(1000));  pTheSpinObject->ejectDAC = 0x8000;
      hsspi(2,0x147800); taskDelay(calcSysClkTicks(1000));  pTheSpinObject->ejectDAC = 0x7800;
      hsspi(2,0x147000); taskDelay(calcSysClkTicks(1000));  pTheSpinObject->ejectDAC = 0x7000;
      hsspi(2,0x146800); taskDelay(calcSysClkTicks(1000));  pTheSpinObject->ejectDAC = 0x6800;
      hsspi(2,0x140000); taskDelay(calcSysClkTicks(1000));  pTheSpinObject->ejectDAC = 0x0;

      setBearingAir(0);   		/* off, so the sample seats correctly */
      taskDelay(calcSysClkTicks(1000));
      setSpeed(pTheSpinObject->SPNspeedSet/1000);
      /* if (bearAirBeforeEjectValue)	/* turn bearing air back on if */
      /*   setBearingAir(1);               /* it was on before ejecting   */

      pCurrentStatBlock->AcqLSDVbits |= LSDV_EJECT;
      // still need to stop bearing and and reset speed if not zero.
   }
   return(OK);
}


int detectSample()
{
   return( *pMASTER_SamplePresent & 2);
}

void setBearingLevel(int level)
{
   if (pTheSpinObject == NULL) return;
   pTheSpinObject->bearLevel = level;
   pCurrentStatBlock->AcqPneuBearing = level;
   if (pTheSpinObject->bearDAC != 0)
      setBearingAir(1);
}


void setBearingAir(int OnOff)
{
   if (pTheSpinObject == NULL) return;
   if (OnOff) 
   {
      hsspi(2,(0x100000 | pTheSpinObject->bearLevel));	// DAC 0
      pTheSpinObject->bearDAC = pTheSpinObject->bearLevel;
   }
   else
   {
      hsspi(2,0x100000);	// DAC 0, value 0000
      pTheSpinObject->bearDAC = 0x0;
   }
}
   
//*** these are dummy routine, to be moved and expanded eventually *.
extern int globalLocked;
int lockSense()
{
   return ( globalLocked );

}

int spinValueGet()
{
   return ( getSpeed() );
}

int spinValueSet(int sampleSpinRate, int bumpFlag)
{
   if (pTheSpinObject == NULL) return;

   setSpeed(sampleSpinRate);
   return(OK);
}

void pneuTest()
{
char answer;
int done=0;
int status;
    printf("P n e u   T e s t\n\n");
    while ( ! done )
    {
       status = hsspi(2,0x3000000);
       printf("1  write/read bearing air    a  Ramp bearing DAC\n");
       printf("2  write/read  drive  air    b  Ramp  drive  DAC\n");
       printf("3  write/read  eject  air    c  Ramp  eject  DAC\n");
       printf("4  write/read    vt   air    d  Ramp    vt   DAC\n");
       printf("5  display status\n");
       printf("6  switch to %s\n", (status&0x800000) ? "solids" : "liquids");
       printf("7  walk flow LEDs\n");
       printf("8  clear VT threshold error latch\n");
       printf("9  sample present status\n");
       printf("0  Done.\n");
       printf("\nEnter selection:");
       answer = getchar();
       switch(answer) {
       case '1':
           printf("Bearing:\n");
           readWriteDac(0);
           break;
       case '2':
           printf("Drive:\n");
           readWriteDac(1);
           break;
       case '3':
           printf("Eject: (always reads as 0 (zero))\n");
           readWriteDac(2);
           break;
       case '4':
           printf("VT Air:\n");
           readWriteDac(3);
           break;
       case '5':
           displayPneuStatus();
           break;
       case '6':
           if (status&0x800000)
              hsspi(2,0x1000000);	// set to solids
           else
              hsspi(2,0x1000001);;	// set to liquids
           break;
       case '7':
           walkFlowLEDs();
           break;
       case '8':
           hsspi(2,0x50000001);
           break;
       case '9':
           printf("Sample %s at top    of upper barrel\n",
			(*pMASTER_SamplePresent & 0x1) ? "IS" : "IS NOT");
           printf("Sample %s at bottom of upper barrel\n",
			(*pMASTER_SamplePresent & 0x2) ? "IS" : "IS NOT");
           break;
       case 'a':
           rampPneuDac(0);
           break;
       case 'b':
           rampPneuDac(1);
           break;
       case 'c':
           rampPneuDac(2);
           break;
       case 'd':
           rampPneuDac(3);
           break;
       case '0':
           done = 1;
           break;
       default:
           printf("Huh??\n");
           break;
       }
       getchar();
       printf("\n\n");
   }
}

void walkFlowLEDs()
{
int i;
   for (i=0; i<10; i++)
   {
      hsspi( 2, (0x2000000 | ( (~(1<<i)) & 0x3ff)) );
      taskDelay(calcSysClkTicks(500));  /* 500 ms, taskDelay(30); */
   }
   for (i=0; i<10; i++)
   {
      hsspi( 2, (0x2000000 | ( (1<<i) & 0x3ff)) );
      taskDelay(calcSysClkTicks(500));  /* 500 ms, taskDelay(30); */
   }
   hsspi(2,0x2000307); // set back to default
   
}


void displayPneuStatus()
{
int status;
 status = hsspi(2,0x03000000);
 printf("Status (0x%x):\n", (status&0xffffff) );
 printf("     power supply: %s\n", (status&0x001) ? "OK"           : "FAULT" );
 printf("       close loop: %s\n", (status&0x002) ? "ARMED"        : "OPEN");
 printf("     upper barrel: %s\n", (status&0x004) ? "OLD"          : "NEW");
 printf("    liquids meter: %s\n", (status&0x008) ? "NOT PRESENT"  : "PRESENT");
 printf("     solids meter: %s\n", (status&0x010) ? "NOT PRESENT"  : "PRESENT");
 printf("     VT threshold: %s\n", (status&0x020) ? "EXCEEDED"     : "OK");
 printf("         NB stack: %s\n", (status&0x040) ? "OK"           : "FAULT");
 printf("  pressure switch: %s\n", (status&0x080) ? "FAULT"        : "OK");
 printf("       pneu fault: %s\n", (status&0x100) ? "FAULT"        : "OK");
 printf("           config: 0x%x (%d)\n",((status>>9)&0xf),((status>>9)&0xf) );
 printf("        LED state: 0x%x (%d)\n",((status>>13)&0x3ff),((status>>13)&0x3ff));
 printf("            state: %s\n", (status&0x800000) ? "LIQUIDS" : "SOLIDS");
   
}

void readWriteDac( int dacNum)
{
int reply;
   printf("write 0x0000...");
   reply = hsspi(2, ( 0x00100000 | (dacNum<<17)) ); // ignore reply
   reply = hsspi(2, ( 0x04100000 | (dacNum<<17)) );
   printf("read 0x%04x (%d)\n",reply,reply);
   printf("write 0x5a5a...");
   reply = hsspi(2, ( 0x00105a5a | (dacNum<<17)) ); // ignore reply
   reply = hsspi(2, ( 0x04100000 | (dacNum<<17)) );
   printf("read 0x%04x (%d)\n",reply,reply);
   printf("write 0xa5a5...");
   reply = hsspi(2, ( 0x0010a5a5 | (dacNum<<17)) ); // ignore reply
   reply = hsspi(2, ( 0x04100000 | (dacNum<<17)) );
   printf("read 0x%04x (%d)\n",reply,reply);
   printf("write 0x0000...");
   reply = hsspi(2, ( 0x00100000 | (dacNum<<17)) ); // ignore reply
   reply = hsspi(2, ( 0x04100000 | (dacNum<<17)) );
   printf("read 0x%04x (%d)\n",reply,reply);
}

void rampPneuDac( int dacNum )
{
char answer;
int i;
   printf("Is the air valve disconnected from the DAC? [y/n]:");
   answer = getchar();
   getchar();
   if (answer != 'y') return;
   while (1) {
      for (i=0; i<6553; i++)
         hsspi(2,(0x00100000 | (dacNum<<17) | (i*10)) );
   }

}
