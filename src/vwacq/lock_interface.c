/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* lock_interface.c  11.1 07/09/07 - lock interface routines */
/* 
 */


#define _POSIX_SOURCE /* defined when source is to be POSIX-compliant */

#include <vxWorks.h>
#include <stdio.h>

#include "commondefs.h"
#include "logMsgLib.h"
#include "hostAcqStructs.h"
#include "autoObj.h"
#include "acqcmds.h"
#include "lock_interface.h"
#include "hardware.h"
#include "timeconst.h"
#include "lkapio.h"
#include "acodes.h"

#define  LSDV_LKMODE		0x02	/* defines in lkapio.c */
#define  LKPREAMPBIT	0x04
#define  LKHOLD		12
#define  LKSAMPLE	13
#define  AP_LOCKDELAY		AP_MIN_DELAY_CNT
#define  AP_LATCH4LOCK          0xff00

/*  The reliability of the system is increased if one uses a latch value
    of 0xff00.  The special value of 0xef00 is required only when dealing
    with PTS transmitters, not an issue with the lock transceiver board.  */

#ifndef TRUE
#define TRUE 1
#endif
#ifndef DUMMYINT
#define DUMMYINT 0
#endif

/*
modification history
--------------------
5-16-95,gmb  created , modified 
*/
/*

lock interface routines for console base autolock

*/

extern AUTO_ID   pTheAutoObject; /* Automation Object */
extern STATUS_BLOCK currentStatBlock;           /* Acqstat-like status block */

/* Static for Lock/Dueterium Power setting  Static (only one ever) */
/* These are not the the lkapio.c lkapregtable[] since the ap address are dynamic
   and follow the channel that the lock/decoupler is assigned
*/
static int lkdecPwrAddr; /* lock/dec atten ap addr (base on channel number of lock/dec) */
static int lkdecPwr;	/* the un-corrected request power setting for the decoupler */
static int lkdecApDelay; /* the ap delay requested */


/****************************************************************
*  Programs to change lock system parameters by stuffing APBUS
*  words in the FIFO.
****************************************************************/

int
setLockPar( int acode, int value, int startfifo )
{
	unsigned short	  apvalue, latch, lockapreg;
	int		  tindex;
	int		(*converter)();
	int		(*storevalue)();

    DPRINT2( 9, "do lock A-code starts with acode = %d and value = %d\n", acode, value );
	tindex = acode2index( acode );
	if (tindex < 0) {
    		DPRINT1(0, "error getting index for lock A-code %d\n",acode);
		return( -1 );
	}
	latch = AP_LATCH4LOCK;
	lockapreg = index2apreg( tindex );
	converter = index2converter( tindex );
	if (converter != NULL)
	  apvalue = (*converter)( value );
	else
	  apvalue = value;
    DPRINT2( 9, "do lock A-code, AP register: 0x%x, AP value: 0x%x\n", lockapreg, apvalue );
        if (startfifo)
	   writeapwordStandAlone( lockapreg, (apvalue & 0xff) | latch, AP_LOCKDELAY );
        else
	   writeapword( lockapreg, (apvalue & 0xff) | latch, AP_LOCKDELAY );

	storevalue = index2valuestore( tindex );
	if (storevalue != NULL)
	  (*storevalue)( value );
        return(0);
}

/*  The lock power presents two difficulties.  First, for levels above 48 dB,
    it is necessary to turn off a 20 dB attenuation in the magnet leg and then
    subtract 20 from the original value.  Second, if the lock power goes to 0,
    it is desired to turn off the whole lock system by writing a special value
    to AP bus register b51.  Currently this is not done, since the sync signal
    would then be lost and the FIFO would never start in lock display.		*/

int
setLockPower( int lockpower_value, int startfifo )
{
	ushort	  lkpreampregvalue, lockpower, apvalue, latch, lockapreg;
	int	  tindex;
	int	(*converter)();
	int	(*storevalue)();

    DPRINT1( 9, "lockpower starts with value = %d\n", lockpower_value );
	lockpower = lockpower_value;
	tindex = acode2index( LOCKPOWER_I );
	if (tindex < 0) {
    		DPRINT(0, "error getting index for lockpower A-code\n");
		return( -1 );
	}

	latch = AP_LATCH4LOCK;

	if (lockpower <= 0) {
		lockpower = 0;
	}
	else if (lockpower > 68) {
		lockpower = 68;
	}

   /* set preamp atten using lock backing store */

	lkpreampregvalue = getlkpreampgainvalue(); 
	if (lockpower >  48) {
		lockpower = lockpower-20;
		lkpreampregvalue |= LKPREAMPBIT;
	}
	else
	  lkpreampregvalue &= (~LKPREAMPBIT);

        if (startfifo)
	   writeapwordStandAlone( getlkpreampgainapaddr(), lkpreampregvalue | latch, AP_LOCKDELAY );
        else
	   writeapword( getlkpreampgainapaddr(), lkpreampregvalue | latch, AP_LOCKDELAY );
	storelkpreampgainvalue( lkpreampregvalue );

	lockapreg = index2apreg( tindex );
	converter = index2converter( tindex );
	if (converter != NULL)
	  apvalue = (*converter)( lockpower );
	else
	  apvalue = lockpower;
    DPRINT2( 9, "lock power, AP register: 0x%x, AP value: 0x%x\n", lockapreg, apvalue );
        if (startfifo)
	   writeapwordStandAlone( lockapreg, (apvalue & 0xff) | latch, AP_LOCKDELAY );
        else
	   writeapword( lockapreg, (apvalue & 0xff) | latch, AP_LOCKDELAY );

	storevalue = index2valuestore( tindex );
	if (storevalue != NULL)
	  (*storevalue)( lockpower_value );
}

/*  
    The lockdec power presents several difficulties.  First, the decoupler 
    levels must be convert to an equivent lock power value. Max
    power is at 63 thus for the 68 db range , -5 to 63 dec db is 0 - 68 lock equiv db
    Second, for levels (lock equiv.) above 48 dB, it is necessary to turn off a 20 dB 
    attenuation in the magnet leg and then subtract 20 from the original value.  

    Since this channel shares the lock power 20db magnet leg attenuiator then
     the actual power setting must done just prior to enabling the decoupler
     feature of the lock/decoupler board and lock power restored when decoupler
     switched off.

     Fortunately there will be only one of these in a system...

		Author  Greg Brissey  4/16/97

     Scratch the above, the decoupler will not go through the lock preamp and thus not the
     20 db attenuator, so this function can be streamed lined, or not even used and PSG can
     set it directly with Apbus commands.

*/

int
setLockDecPower( int apaddr, int lkdecpower, int apdelay, int startfifo )
{
   ushort  lkpreampregvalue, lockapreg, apvalue;

   DPRINT2( 1, "setLockDecPower power = %d, equiv. lk power: %d\n", lkdecpower,lkdecpower-15 );


   lkdecpower -= 15;	/* 63 - 15 = 48 i.e. max atten of lock/dec */

   if (lkdecpower <= 0) 
   {
         lkdecpower = 0;
   }
   else if (lkdecpower > 48) 
   {
         lkdecpower = 48;
   }

   apvalue = cvtLkDbLinear( lkdecpower );
   DPRINT3(1, "setLockDecPower: lock/dec power, AP register: 0x%x, AP value: %d, 0x%x\n", 
		   apaddr, apvalue, apvalue );
   if (startfifo)
      writeapwordStandAlone( apaddr, (apvalue & 0xff) | AP_LATCH4LOCK, apdelay );
   else
      writeapword( apaddr, (apvalue & 0xff) | AP_LATCH4LOCK, apdelay );
   DPRINT3(1,"setLockDecPower: apadr: 0x%x,   val: 0x%x, delay: %d\n",
	   apaddr,(apvalue & 0xff) | AP_LATCH4LOCK,apdelay);


#ifdef XXXX
   if (startfifo > 1)   /* just store the values for future setting */
   {
	lkdecPwrAddr = apaddr;
	lkdecPwr = lkdecpower;
	lkdecApDelay = apdelay; 
   }
   else
   {
     apaddr = lkdecPwrAddr;
     lkdecpower = lkdecPwr;
     apdelay = lkdecApDelay; 

      if ((lkdecPwrAddr == 0) || (lkdecApDelay == 0))
      {
	DPRINT(-1,"setLockDecPower: Can't set Dec. Power, Values are not Initialized\n");
	return(-1);
      }

      lkdecpower -= 15;	/* 63 - 15 = 48 i.e. max atten of lock/dec */

      if (lkdecpower <= 0) 
      {
         lkdecpower = 0;
      }
      else if (lkdecpower > 48) 
      {
         lkdecpower = 48;
      }

      apvalue = cvtLkDbLinear( lkdecpower );
      DPRINT2(0, "setLockDecPower: lock/dec power, AP register: 0x%x, AP value: 0x%x\n", 
		   apaddr, apvalue );
      if (startfifo)
         writeapwordStandAlone( apaddr, (apvalue & 0xff) | AP_LATCH4LOCK, apdelay );
      else
         writeapword( apaddr, (apvalue & 0xff) | AP_LATCH4LOCK, apdelay );
      DPRINT3(0,"setLockDecPower: apadr: 0x%x,   val: 0x%x, delay: %d\n",
	   apaddr,(apvalue & 0xff) | AP_LATCH4LOCK,apdelay);
   }
#endif
   return(0);
}

#define  NUM_LKFREQ_APREGS	3

static int
setLockFreqAPfromBytes( ushort *apvals, int startfifo )
{

/*  Although the AP bus register values have been broken down
    into bytes at this point, this program receives them as ushort,
    because the high-order byte has been set to AP_LATCH4LOCK.	*/

	ushort	  apregs[ NUM_LKFREQ_APREGS ];
	int	  iter, tindex, tval;
	int	(*storevalue)();

	tindex = xcode2index( LKFREQ );
	if (tindex < 0) {
    		DPRINT(0, "error getting index to set lock frequency\n");
		return( -1 );
	}

	apregs[ 0 ] = index2apreg( tindex );
	for (iter = 1; iter < NUM_LKFREQ_APREGS; iter++)
	  apregs[ iter ] = apregs[ iter-1 ] + 1;

	DPRINT3( 1, "lock frequency AP registers: 0x%4x 0x%4x 0x%4x\n",
		      apregs[ 0 ], apregs[ 1 ], apregs[ 2 ] );
	DPRINT3( 1, "lock frequency AP values:    0x%4x 0x%4x 0x%4x\n",
		      apvals[ 0 ], apvals[ 1 ], apvals[ 2 ] );

        if (startfifo)
	  for (iter = 0; iter < NUM_LKFREQ_APREGS; iter++)
	    writeapwordStandAlone( apregs[ iter ], apvals[ iter ], AP_LOCKDELAY );
        else
	  for (iter = 0; iter < NUM_LKFREQ_APREGS; iter++)
	    writeapword( apregs[ iter ], apvals[ iter ], AP_LOCKDELAY );

	storevalue = index2valuestore( tindex );
	if (storevalue != NULL) {
		int	lockfreq_value;

		lockfreq_value = ( (apvals[ 0 ] & 0xff) |
				  ((apvals[ 1 ] & 0xff) << 8) |
				  ((apvals[ 2 ] & 0xff) << 16) );

		(*storevalue)( lockfreq_value );
	}

	return( 0 );
}

int
setLockFreqAPfromShorts( ushort *lockfreq_apvalues, int startfifo )
{
	ushort	apvals[ NUM_LKFREQ_APREGS ];
	int	iter, ival, tval;

	tval = lockfreq_apvalues[ 0 ];
	for (iter = 0; iter < NUM_LKFREQ_APREGS; iter++) {
		apvals[ iter ] = (tval & 0xff) | AP_LATCH4LOCK;
		if (iter == 1)
		  tval = lockfreq_apvalues[ 1 ];	/* for the next iteration */
		else
		  tval = tval >> 8;
	}

	ival = setLockFreqAPfromBytes( &apvals[ 0 ], startfifo );
	return( ival );
}

int
setLockFreqAPfromLong( int lockfreq_apvalue, int startfifo )
{
	ushort	apvals[ NUM_LKFREQ_APREGS ];
	int	iter, ival, tindex, tval;

	tval = lockfreq_apvalue;
	for (iter = 0; iter < NUM_LKFREQ_APREGS; iter++) {
		apvals[ iter ] = (tval & 0xff) | AP_LATCH4LOCK;
		tval = tval >> 8;
	}

	ival = setLockFreqAPfromBytes( &apvals[ 0 ], startfifo );
	return( ival );
}


/*  Unlike do_lockpAcode this one does not set any register on the APBUS.  */

int
storeLockPar( int acode, int value )
{
	int		  tindex;
	int		(*storevalue)();

    DPRINT2( 9, "store lock A-code starts with acode = %d and value = %d\n", acode, value );
	tindex = acode2index( acode );
	if (tindex < 0) {
    		DPRINT1(0, "error getting index for lock A-code %d\n", acode );
		return( -1 );
	}

	storevalue = index2valuestore( tindex );
	if (storevalue != NULL)
	  (*storevalue)( value );
        return(0);
}

int init_dac()
{
  DPRINT(0,"init_dac: Not Implemented, does nothing\n");
}


int set_lk_hw(int type, int value)
{
   switch (type)
   {
      case SET_Z0:
		{
    	   	       short dacs[8];
           	       int bytes;
           	       dacs[0]=0;  /* Ingored */
		       dacs[1]=(short) Z0; /* Z0 DAC */
		       dacs[2]=(short) value;
		       shimHandler( dacs, 3 );
        	}
                       break;

      case SET_LKFREQ:
		       setLockFreqAPfromLong( value, TRUE );
                       break;

      case SET_POWER:
	               setLockPower( value, TRUE);
                       break;
      case SET_PHASE:
	               setLockPar(LOCKPHASE_I, value, TRUE);
                       break;
      case SET_MODE:
                       if (value == 8)
	                  setLockPar(LOCKTC, DUMMYINT, TRUE);
                       else if (value == 11)
	                  setLockPar(LOCKACQTC, DUMMYINT, TRUE);
                       else
                       {
                          if ( (value == 3) || (value == 5) || (value == 7) )
                             value = 1;
	                  setLockPar(LOCKMODE_I, value, TRUE);
                       }
                       break;
      case SET_GAIN:
	               setLockPar(LOCKGAIN_I, value, TRUE);
                       break;
      case SET_LK2KCF:
     		       writeapwordStandAlone(0x0b51, 0x0a, AP_MIN_DELAY_CNT ); /* 0x0a set 2kHz*/
	               setLockPar(LOCKTC, DUMMYINT, TRUE);
                       break;
      case SET_LK2KCS:
     		       writeapwordStandAlone(0x0b51, 0x0a, AP_MIN_DELAY_CNT ); /* 0x0a set 2kHz*/
	               setLockPar(LOCKACQTC, DUMMYINT, TRUE);
                       break;
      case SET_LK20HZ:
                       writeapwordStandAlone(0x0b51, 0x1a, AP_MIN_DELAY_CNT ); /* 0x1a set 20Hz*/
	               setLockPar(LOCKTC, DUMMYINT, TRUE);
                       break;
      default:         value = 0;
   }
   return(value);
}

/*  Since this program now can return the lock frequency AP word,
    it must be capable of returning a 32-bit quantity.  Therefore
    it is relying on the fact that sizeof( int ) == sizeof( long ).
    If this is not so, you must write a separate get_lkfreq_ap
    and remove the macro of the same name from lock_interface.h  */

int get_lk_hw(int type)
{
   int value=0;
   switch (type)
   {
      case GET_LOCKED: 
		       value = autoLockSense(pTheAutoObject);
                       break;

      case GET_LKFREQ:
		       value = getLockFreqAP();
                       break;

      case GET_Z0: 
		       /* 1 =  Vnmr Z0 DAC */
		       value = (int) currentStatBlock.stb.AcqShimValues[ Z0 ];
                       break;
      case GET_Z0_LIMIT: 
		       value = dac_limit();  /* requires an SU etc */
                       break;
      case GET_POWER:
		       value = (int) currentStatBlock.stb.AcqLockPower;
                       break;
      case GET_PHASE:
		       value = (int) currentStatBlock.stb.AcqLockPhase;
                       break;
      case GET_MODE:
		       value = ( currentStatBlock.stb.AcqLSDVbits & LSDV_LKMODE ) ? 
					1 : 0;
                       break;
      case GET_GAIN:
		       value = (int) currentStatBlock.stb.AcqLockGain;
                       break;
      default:         value = 0;
   }
   return(value);
}

/* Tdelay(), Ldelay(), Tcheck() moved to serialDevice.c */
 
/*----------------------------------------------------------------------*/
/* time_t secondClock(&chkval,mode) - mode = 1 - start clock		*/
/*				   mode = 0 - elasped time in seconds	*/
/*----------------------------------------------------------------------*/
time_t secondClock(time_t *chkval,int mode)
{
   if (mode)
   {
     *chkval =  time(NULL);
     return(0);
   }
   else
   {
      return(time(NULL) - *chkval);
   }
}

/*----------------------------------------------------------------------*/
/* getLockLevel(void) 							*/
/*----------------------------------------------------------------------*/
int getLockLevel(void)
{
   extern AUTO_ID pTheAutoObject;
   int lkval;
   lkval = autoLkValueGet(pTheAutoObject);
   DPRINT1(1,"getLockLevel: lock level: %d\n",lkval);
   return (lkval);
}

/*--------------------------------------------------------------*/
/* calcgain(level,requested level)				*/
/*   Calculate a new lock gain change for a new level		*/
/*   (long) level - present signal level			*/
/*   (long) requested level - level desired			*/
/*								*/
/*   First the level is divide by 2 untill the level is close   */
/*   to the requested level. For each division or mult by 2     */
/*   a corresponding 6db in gain is needed.			*/
/*   After division or mult by two the percent of the remainder */
/*   to the requested level is obtained. For each 16% off       */
/*   1db of gain change is needed.				*/
/*--------------------------------------------------------------*/
int calcgain(long level,long reqlevel)
{
   long value,diff,percnt,percnt1;
   int cnt,coarse,fine,sign;

   value = level;
   sign = cnt = 0;

   diff = reqlevel - value;
   if (diff < 0L)
      diff = -diff;
   percnt1 = (diff*1000L) / reqlevel;	/* % diff of level to req. level */

  /*--- count number of times divided or mult by 2 to reach target value --*/
   if (level > reqlevel) 
   {
      while(level > reqlevel)
      {
         level >>= 1;
         cnt--;
      }
      sign = 1;	/* gain change in positive direction */
   }
   else
   {
      if (level < reqlevel)
      {
         while(level < reqlevel)
         {
            level <<= 1;
            cnt++;
         }
         sign = -1; /* gain change in negative direct */
      }
   }
   coarse = 6 * cnt;	/* course db adjustment (6db double signal)  */

 /*-- calc percent diff from value to reqlevel for each 16% its 1db  */

   /*fine = ((((reqlevel - level)*1000L) / level) / 16L) / 10L;*/
   diff = reqlevel - level;
   if (diff < 0L)
      diff = -diff;
   percnt = (diff*1000L) / level;
   fine = ((percnt / 16L) + 5L) / 10L;
   fine *= sign;
   DPRINT7(1,
    "Calcgain: value=%ld req=%ld %%dif=%ld, coarse=%d, dif=%ld, %%=%ld, fine=%d\n",
       value,reqlevel,percnt1/10,coarse,diff,percnt,fine);

   if (percnt1 < 160L)
      return(0);	/* % < 16% no gain change is needed */
   else
      return(coarse + fine);
}

setlksample()
{
   set_lk_hw(SET_MODE,LKSAMPLE);
}
set2khz()
{
   set_lk_hw(SET_LK2KCS,0);
}
#ifdef DEBUG
/* simple test routines */
setz0(int value)
{
   int val;
   set_lk_hw(SET_Z0,value);
   val = get_lk_hw(GET_Z0);
   printf("value = %d\n",val);
}
slkpwr(int value)
{
   int val;
   set_lk_hw(SET_POWER,value);
   val = get_lk_hw(GET_POWER);
   printf("value = %d\n",val);
}

slkphase(int value)
{
   int val;
   set_lk_hw(SET_PHASE,value);
   val = get_lk_hw(GET_PHASE);
   printf("value = %d\n",val);
}
slkgain(int value)
{
   int val;
   set_lk_hw(SET_GAIN,value);
   val = get_lk_hw(GET_GAIN);
   printf("value = %d\n",val);
}
set20hz()
{
   set_lk_hw(SET_LK20HZ,0);
}
lked()
{
    int val;
    val = get_lk_hw(GET_LOCKED);
    printf("Locked: %d\n",val);
}
#endif

get_all_dacs(arr, len)
int arr[];
int len;
{
   int index;

   for (index=0; index < len; index++)
      arr[index] = (int) currentStatBlock.stb.AcqShimValues[ index ];
}

#ifdef XXXX
main(argc,argv)
int argc;
char *argv[];
{
   int mode;               	/* mode to use */	
   int lkpwr,lkphase,lkgain,lkmode;
   int ret;

   ret = s_main(mode,&lkpwr,&lkphase,&lkgain,&lkmode,argc);
   DPRINT1(0,"AutoLock return val is %d\n",ret);
   return(0);
}
#endif
