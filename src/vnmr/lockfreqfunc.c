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

#include "vnmrsys.h"
#include "group.h"
#include "allocate.h"
#include "pvars.h"

#define	MAXSTR	256
#define	TWO_24	16777216.0	/* 2 to_the_power 24 */

extern int	bgflag;
extern void abort_message(const char *format, ...);

static int priv_getline(FILE *fd, char *targ, int mxline);
int lockfreqtab_read(char *lkfilename, int h1freq, double *synthif,
                     char *lksense, double *lockref);

void oneLongToTwoShort(int lval, short *sval )
{
	sval[ 0 ] = (short) (lval >> 16);
	sval[ 1 ] = (short) (lval & 0xffff);
}

static char *
getconsoletype()
{
	char		*tmpstr;

	tmpstr = (char *) allocate( MAXSTR );
	if (tmpstr == NULL) {
		return( NULL );
	}
	if (P_getstring(SYSTEMGLOBAL, "Console", tmpstr, 1, MAXSTR - 1) != 0) {
		if (P_getstring(GLOBAL, "Console", tmpstr, 1, MAXSTR - 1) != 0)
		  return( NULL );
	}

	return( tmpstr );
}

#define FACTOR4MERCURY  (double)(0x40000000/10e6)

static int getlkfreqapword_mercury(double lockfreq, int h1freq )
{
	int	r,v;
	double	lk_offset;
	int	bits32;

        r = v = 1;
	if      (h1freq == 200) { r=1;  v=1;  }
	else if (h1freq == 300) { r=13; v=23; }
	else if (h1freq == 400) { r=13; v=33; }

	lk_offset = (lockfreq - 20.0 * v / r) * 1e6;
	bits32 = (int) (lk_offset * FACTOR4MERCURY);

	return( bits32 );
}

static double getlkfreqoffset(double lockfreq, int h1freq )
{
	double	tmpfreq,synthif,lockref;
	char	lockfreqtab[MAXSTR];		/* Filename for lockfreqtab */
	char	lksense;


	synthif = 0.0;
	lockref = 0.0;
	lksense = '\0';
	strcpy(lockfreqtab,systemdir); /* $vnmrsystem */
	strcat(lockfreqtab,"/nuctables/lockfreqtab");
	if (lockfreqtab_read(lockfreqtab,h1freq,&synthif,&lksense, &lockref) == 0) {
		if (bgflag) {
		   fprintf(stderr,
			"lockfreqtab: synthif=%g lksense= %c lockref=%g\n",
				synthif,lksense,lockref);
		}
		if (lksense == '+')
		   tmpfreq =  ((lockfreq + synthif) - lockref);
		else
		   tmpfreq = ((lockfreq - synthif) - lockref);

		if (tmpfreq < 0.0) tmpfreq = (-1.0)*tmpfreq;
		if (bgflag)
       		   fprintf(stderr,"setlkfrqflt(): locksynth = %g \n",tmpfreq);
	}
	else {
        	return(-1);
	}

	return( tmpfreq );
}

static int
getlkfreqapword_inova(double lockfreq, int h1freq )
{
	double	tmpfreq;
	tmpfreq = getlkfreqoffset(lockfreq,h1freq);

	return( (((int)(tmpfreq*TWO_24 + 0.5))/40) );
}

void getlkfreqDDsi(double lockfreq, int h1freq, int *parts)
{
union cv {
   double tmpfreq;
   int    hilo[2];
} cv1;
        cv1.tmpfreq = getlkfreqoffset(lockfreq,h1freq);
#ifdef LINUX
        parts[0] = cv1.hilo[1];
        parts[1] = cv1.hilo[0];
#else
        parts[0] = cv1.hilo[0];
        parts[1] = cv1.hilo[1];
#endif
}

int getlkfreqapword(double lockfreq, int h1freq )
{
	char	*consoleval;
	int	 lkfreqapword;

	consoleval = getconsoletype();
	if (consoleval == NULL)
	  return( -1 );

	if (strcmp( consoleval, "inova" ) == 0) {
		lkfreqapword = getlkfreqapword_inova( lockfreq, h1freq );
	}
	else if (strcmp( consoleval, "mercury" ) == 0) {
		lkfreqapword = getlkfreqapword_mercury( lockfreq, h1freq );
	}
	else {
		lkfreqapword = -1;
	}

	release( consoleval );
	return ( lkfreqapword );
}


#define  H2FREQ_OVER_H1FREQ	(0.15351)

static int
gettmpfreqsign_inova(int h1freq, double *psynthif, char *plksense, double *plockref )
{
	double	tmpfreq,synthif,lockref,lockfreq;
	char	lockfreqtab[MAXSTR];		/* Filename for lockfreqtab */
	char	lksense;

	synthif = 0.0;
	lockref = 0.0;
	lksense = '\0';
	lockfreq = H2FREQ_OVER_H1FREQ * (double) h1freq;	/* default lock frequency */
	strcpy(lockfreqtab,systemdir); /* $vnmrsystem */
	strcat(lockfreqtab,"/nuctables/lockfreqtab");
	if (lockfreqtab_read(lockfreqtab,h1freq,&synthif,&lksense, &lockref) == 0) {
		if (bgflag) {
		   fprintf(stderr,
			"lockfreqtab: synthif=%g lksense= %c lockref=%g\n",
				synthif,lksense,lockref);
		}
		if (lksense == '+')
		   tmpfreq =  ((lockfreq + synthif) - lockref);
		else
		   tmpfreq = ((lockfreq - synthif) - lockref);

		if (bgflag)
       		   fprintf(stderr,"gettmpfreqsign inova(): locksynth = %g \n",tmpfreq);

    /*  As a side-effect, return the values extracted from the lock frequency table file  */

		*plksense = lksense;
		*plockref = lockref;
		*psynthif = synthif;

	  /* You are in big trouble if tmpfreq is 0 for the default lock frequency. */

		if (tmpfreq < 0.0) 
		  return( -1 );
		else
		  return( 1 );
	}
	else {
        	return( 0 );
	}
}

static double
apvalue2lkfreq_inova(int apvalue, int h1freq )
{
	double	tmpfreq, synthif, reffreq, lockref, lockfreq;
	char	lksense;
	int	signoftmpfreq;

	synthif = 0.0;
	lockref = 0.0;
	lksense = '\0';

	signoftmpfreq = gettmpfreqsign_inova( h1freq, &synthif, &lksense, &lockref );
	if (signoftmpfreq == 0)
	  return( 0.0 );

	tmpfreq = (double) (apvalue) * 40.0 / TWO_24;
	if (signoftmpfreq > 0)
	  reffreq = lockref + tmpfreq;
	else
	  reffreq = lockref - tmpfreq;

	if (lksense == '+') {
		lockfreq = reffreq - synthif;
	}
	else {
		lockfreq = reffreq + synthif;
	}

	return( lockfreq );
}


double apvalue2lkfreq(int apvalue, int h1freq )
{
	char	*consoleval;
	double	 lockfreq;

	consoleval = getconsoletype();
	if (consoleval == NULL)
	  return( -1 );

	if (strcmp( consoleval, "inova" ) == 0) {
		lockfreq = apvalue2lkfreq_inova( apvalue, h1freq );
	}
	else {
		lockfreq = 0.0;
	}

	release( consoleval );
	return ( lockfreq );
}



#define COMMENTCHAR '#'
#define LINEFEEDCHAR '\n'

int lockfreqtab_read(char *lkfilename, int h1freq, double *synthif,
                     char *lksense, double *lockref)
{
 FILE *fd;
 char parse_me[MAXSTR];
 int stat,h1freq_file;
 double tmp_synthif,tmp_lockref;

    fd = fopen(lkfilename,"r");
    if (fd == NULL) 
    	return(-1);

    do
    {
       do				/* until not a comment or blank*/
       {
         stat=priv_getline(fd,parse_me,MAXSTR);
       }
       while ( ((parse_me[0] == COMMENTCHAR) || (parse_me[0] == LINEFEEDCHAR)) 	
							    && (stat != EOF) );
       if (stat != EOF) 
       {
           /* now it is not a comment so attempt to parse it */
          stat = sscanf(parse_me,"%d  %lf  %1s  %lf", &h1freq_file,
		&tmp_synthif, lksense, &tmp_lockref);
          if (stat != 4)
          {
		fclose(fd); 
#ifdef INTERACT
		text_error("PSG: Error in lockfreqtab file.");
#else
		abort_message("PSG: Error in lockfreqtab file.");
#endif
                return(-1);
          }
	  if ((h1freq == h1freq_file) && 
			((lksense[0] == '+') || (lksense[0] == '-')))
	  {
		*synthif = tmp_synthif;
		*lockref = tmp_lockref;
   		fclose(fd); 
		return(0);
	  }
       }
    }
    while (stat != EOF);

    fclose(fd); 
    return(-1);
}

static int priv_getline(FILE *fd, char *targ, int mxline)
{   
   char *tt;
   int cnt,tmp;
   tt = targ;
   cnt = 0;
   do
   {  
    tmp = getc(fd);
    if (cnt < mxline)
    {
      *tt++ = (char) tmp;    /* read but discard chars after mxline */
       cnt++;
    }
   }
   while ((tmp != EOF) && (tmp != '\n'));

   // check for windows end of line '\r\n'
   if (cnt && (targ[cnt-2] == '\r') && (tmp == '\n'))
   {
      tt = tt - 2;
      cnt--;
   }
   if (cnt >= mxline)
      targ[mxline -1] = '\0';
   else 
      *tt = '\0';	/* ensure a null terminated list */
   if (tmp == EOF)
     return(EOF);
   else
     return(cnt);
}
