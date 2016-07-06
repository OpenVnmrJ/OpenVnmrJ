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
/*  Low-level interface to the serial-I/O driver of VxWorks which implements
    the Greg Brissey/Varian Serial Device protocol.

    The protocol works like this.  The computer starts a command by sending a
    single letter to the device.  The device echoes this letter, plus some
    other stuff optionally, followed by an EOM (^C).  See the program cmdecho,
    with second argument CMD.

    Next the computer can send additional characters, parameters and values.
    See the program cmdecho, with second argument ECHO, and the program
    echoval or sendnvals.

    Finally the device outputs a <C/R> (or <L/F>), followed by a Dot.  This
    signifies thes device is ready for another command.  See the program
    cmddone.

    There are two compile-time switches, SERIAL_DEBUG, which turns on a bunch
    of useful debug messages, and NOTIMEOUT, which would cause the application
    to wait forever instead of timing out.

    This program is for VxWorks only.  It expects the serial driver to:
        open a connection to the serial port.
        read a character
        write a character
        flush the port:               ioctl( ... FIOFLUSH ... )
        tell the calling task how
        many chars are waiting:       ioctl( ... FIONREAD ... )
        set the baud rate:            ioctl( ... FIOBAUDRATE ... )
        
    May 1995.									*/

#include <stdio.h>
#include <vxWorks.h> 
#include <ioLib.h>

#include "serialDevice.h"
#include "logMsgLib.h"

#define CR 13
#define EOM 3 /* for testing only , originally EOM = 3 it stopped "cmdecho"
                                      any time it received 3 Dec */ 
#define ECHO 1
#define TIMEOUT 98
#define DONETIMEOUT -424242
#define PERIOD 46
#define LF 10

/* cmdecho time out values respective to ports */
typedef struct _portinfo {
		   int portd;
		   int echoTimeout;
		   int doneTimeout;
        	         } PORT_INFO;

/* cmdecho time out values respective to ports */
static short port2fd[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
			     0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static short portdMap[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  
			      0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static int portTimeOut[6] = { 600, 600, 600, 600, 600, 600 };

int baudRate = 9600;

/*****************************************************************
* initPort - opens and initializes selected serial port
*
*/
int initPort(char *device)
{
    int sPort;

    sPort = open(device, O_RDWR, 0777);
    if( sPort == -1 ) {
#ifdef SERIAL_DEBUG
        printf( "Could not open: %s\n", device );
#endif
        return( -1 );
    }
    ioctl(sPort, FIOBAUDRATE, baudRate);
    return (sPort);
}

#ifndef MAXPORT
#define  MAXPORT 4
#endif

#ifndef BASE_SERIAL_PORT
#define BASE_SERIAL_PORT "/TyMaster/"
#endif


/*****************************************************************
* initSerialPort - open port based on port number
*
*/
int initSerialPort( int port )
{
	char	serialInterface[ 12 ], digitBuffer[ 6 ];
	int	blen, dlen, iter, portd,portn;

	if (port < 0)
	  return( -1 );

        portn = port;	/* save port, since it gets trashed */

/*  Caution:  port 0 typically is the Console...  */

	if (port > MAXPORT)
	  return( -1 );

/*  Avoid use of printf programs.  The port number is guaranteed to be nonnegative  */

	if (port > 0) {
		int	digit, dindex, tval;

		dindex = 0;
		do {
			tval = port / 10;
			digit = port - tval * 10;
			digitBuffer[ dindex++ ] = digit + '0';
			port = tval;
		} while (port > 0);
		digitBuffer[ dindex ] = '\0';
	}
	else {
		digitBuffer[ 0 ] = '0';
		digitBuffer[ 1 ] = '\0';
	}

	dlen = strlen( &digitBuffer[ 0 ] );
	blen = strlen( BASE_SERIAL_PORT ) - 1;
	strcpy( &serialInterface[ 0 ], BASE_SERIAL_PORT );

/*  Formula for blen and the unusual parameters for the for-loop
    reduce the number of additions/subtractions required.	*/

	for (iter = 1; iter <= dlen; iter++)
	  serialInterface[ blen + iter ] = digitBuffer[ dlen - iter ];

	serialInterface[ blen + dlen + 1 ] = '\0';

	portd = initPort( &serialInterface[ 0 ] );
        portdMap[portd % 20] = (short) portn;
/*
        printf("initSerialPort: lport: %d, portd: %d, timeout: %d\n",
	 port, portd, portTimeOut[port]);
*/
	return( portd );
}

setSerialTimeout(int portd, int timeout)
{
int origTimeout,i;

   if (portd < 0)
      return( -1 );

/* Caution:  port 0 typically is the Console...  */
/* With the Nirvana and VxWorks 5.5 portnumbers are no longer 1,2,3,4... */
/* Instead the are fd (file descripters) which are 30, 31, or so */

   i=0;
   while (i < 10)
   {
      if (port2fd[i] == portd) break;
      i++;
   }
   if (i==10) 
   {
      i=0;
      while (i<10)
      {
         if (port2fd[i] == 0)
         {
            port2fd[i] = portd;
            break;
         }
         i++;
       }
   }
   if (i==10) return(-1);
   origTimeout = portTimeOut[i];
   portTimeOut[i] = timeout;

    DPRINT4(-1,"portd: %d, port: %d, origTimeout: %d, new TO: %d\n",
 		portd,i,origTimeout,portTimeOut[i]);

   return(origTimeout);
}

getSerialTimeout(int portd)
{
int i;

   if (portd < 0)
      return( -1 );

/*  Caution:  port 0 typically is the Console...  */
/* With the Nirvana and VxWorks 5.5 portnumbers are no longer 1,2,3,4... */
/* Instead the are fd (file descripters) which are 30, 31, or so */

   i=0;
   while (i < 10)
   {
      if (port2fd[i] == portd) break;
      i++;
   }
   if (i==10) return(-1);

   return(portTimeOut[i]);
          
}


/*****************************************************************
* pputchr - sending the alphabetical part of command to serial port
*
*/
int  pputchr( int port, char cmd ) 
{ 
    int  wbyte; 
    wbyte = write( port, &cmd, 1 );
    return( wbyte );
}

/*****************************************************************
* pifchr - if there is anything presents on the serial port,then 
*             those will be read by either cmdecho() or cmddone()
*
*/ 
int pifchr( int  port )
{
   int  status;
   int  nBytesUnread;
   status = ioctl( port, FIONREAD, (int) &nBytesUnread );
   /*printf( "ARE THERE ANYTHING ON THE PORT?: %d\n", nBytesUnread );*/
   return( (status) ? 0 : nBytesUnread );
}

/*****************************************************************
* clearinput - flushing the input buffer of the serial port 
*
*/
int clearinput( port )
{
    int  status;
    status = ioctl( port, FIORFLUSH, 0 );
    return( status );
}
/*****************************************************************
* clearoutput - flushing the output buffer of the serial port 
*
*/
int clearoutput( port )
{
    int  status;
    status = ioctl( port, FIOWFLUSH, 0 );
    return( status );
}

/*****************************************************************
* clearport - flushing input and output buffer of the serial port 
*
*/
int clearport( port )
{
    int  status;
    status = ioctl( port, FIOFLUSH, 0 );
    return( status );
}

/*****************************************************************
*
* cmdecho -  ( readPort, type )
*
*    type = 0 - receives from the peripheral the expanded form of
*               the command given till the EOM char is received
*
*    type = 1 - wait for just an echoed character
*
*/
cmdecho( int port, int type )
{
    char chr=0, charbuf1[128];
    int  rbyte=0, i, echo, timchk;
    int portl,timeout;
    echo = FALSE;
    portl = portdMap[port % 20];
    /* portl = portdMap[port]; */
    timeout = portTimeOut[portl];
    /* printf("cmdecho: portd: %d, port %d, timeout: %d \n",port,portl,timeout); */
    Ldelay( &timchk, timeout );
    while( chr != EOM && !echo )
    {
      if( pifchr ( port ))
      {
        rbyte=read(port, charbuf1, 1);    /* This is a BLOCKING read */
        if(type)  echo = TRUE;
        chr = charbuf1[0];
#ifdef SERIAL_DEBUG
        printf("   Received by 'cmdecho' :%4d Dec\t\t -->%2c\n", chr, chr);
#endif
      }
      else
      {  
#ifndef NOTIMEOUT
        /*printf( "  inside TIMETRACE of cmdecho \n" );*/
        if( Tcheck( timchk ))
        {
#ifdef SERIAL_DEBUG
            printf( "cmdecho timed out\n" );
#endif
            return( TIMEOUT );
        }
        /* taskDelay(calcSysClkTicks(17));  */
#endif
      } 
    }/* end of while */
    return (0);
}/* end of cmdecho */


/*****************************************************************
*     sendnvals(port, values[], n)  - Ouput n decimal values 
*                                     to RS232 peripheral
*
*      port   - integer port number
*      values - integer array of decimal values, sent
*		one character at a time, no echo expected
*
*      n      - the number of integers to send
*/
sendnvals( int port, int *values, int n)
{
int i, val;
int lflag, digit, divider;

   for (i=0; i<n; i++) {
      val = values[i];
      lflag=0;
      if (val != 0) {
         if (val < 0) {	// send a '-' sign
            pputchr(port,'-');
            val = -val;
         }
         for (divider=10000; divider>0; divider /= 10) {
            digit = val/divider;
            val -= (digit*divider);
            if ((digit>0) || lflag) {
               lflag = 1;
               pputchr(port, digit+'0');
            }
         }
      }
      else {
         pputchr(port,'0');
      }
      if (i != (n-1) ) pputchr(port,',');
   }
}
/*****************************************************************
*     echoval(port,value) -   Output decimal value to  
*                              RS232 peripheral       
*                                                    
*      port - integer port number                    
*      value- integer decimal value to be sent         
*              one character at a time with its echo
*              received.                               
*/
echoval( int port, int value )
{
    int divider,digit,lflag;
    lflag = 0;
    if (value != 0)
    {   
        if (value < 0)
        {
            pputchr(port,'-');
#ifdef SERIAL_DEBUG
            printf("Send '-' by 'echoval'    :%d         ----> %c\n\n", '-', '-');
#endif
            if (cmdecho(port,ECHO))  return(TIMEOUT);/*return err*/
            value = -value;
        }
        for (divider=10000;divider > 0;divider /= 10)
        {
            digit = value / divider;
            value -= (digit * divider);
            if (digit > 0 || lflag)
            {
                lflag = 1;
                digit += '0';
                pputchr(port,digit);
#ifdef SERIAL_DEBUG
                printf("Send digit by 'echoval'  :%d         ----> %c\n", digit, digit);
#endif
                if (cmdecho(port,ECHO))  return(TIMEOUT);/*return err*/
            }
        } /* end for */
    }
    else  /* value == 0 */
    {
        pputchr(port,'0');
#ifdef SERIAL_DEBUG
        printf("Send '0' by 'echoval'     : %d        ----> %c\n", '0', '0');
#endif
        if (cmdecho(port,ECHO))  return(TIMEOUT);/*return err*/
    }
    return(0);
}   

/*****************************************************************
* cmddone() - waits for the RS232 peripheral to give it's status after 
*               executing a command.(eg. robot, spinner)              
*                                                                    
*      error -  indicated by a control C (ie EOM)                      
*    noerror -  indicated by no control C (ie EOM)                   
*                                                                      
* Function returns any decimal sent by the peripheral prior to the    
*          ready sequence (this maybe 0-2^15)                        
*                                                                      
* Ready Sequence is: Carriage Return, Period or Linefeed, Period       
*                   This sequence means the peripheral is ready for    
*                   another command.                                   
*/
cmddone(int port, int timdown)   /* equi. cmdAck in unix */
{
   return(replydone(port, NULL, 0, timdown));
}

/*****************************************************************
* readreply() - returns the chars the RS232 peripheral sends back
*               to the application.  Based on cmddone.
*
* The ideal behind this program is:
*     The application sends some command to the serial device.
*     The device echos the command, plus some optional other
*     stuff, and then sends an EOM (^C).  The application relies
*     on cmdecho to accomplish all this.
*     Next the application calls this program.  It reads chars
*     from the serial port until a <CR><dot> or <LF><dot>
*     sequence is read.  The characters upto that final dot
*     are stored in the replybuf.
*
* Choose either cmddone or readreply; do not call both in a
* command cycle.
*
*/
readreply(int port, int timdown, char *replybuf, int replysize )
{
   return(replydone(port, replybuf, replysize, timdown));
}

/*****************************************************************
* replydone() - waits for the RS232 peripheral to give it's status after 
*               executing a command.(eg. robot, spinner)              
*                                                                    
*      error -  indicated by a control C (ie EOM)                      
*    noerror -  indicated by no control C (ie EOM)                   
*                                                                      
* If passed string buffer is NULL then the
*   Function returns any decimal sent by the peripheral prior to the    
*          ready sequence (this maybe 0-2^15)                        
*                                                                      
*  Otherwise the return string is copied into the the passed buffer
*     and 0 is return if no timeout
*
* Ready Sequence is: Carriage Return, Period or Linefeed, Period       
*                   This sequence means the peripheral is ready for    
*                   another command.                                   
*/
#define EOT 
#define EOM 3
replydone(int port, char *strbuf, int maxchr, int timdown)   /* equi. cmdAck in unix */
{
    char chrbuf[4], *strptr;
    int chr,rbyte,prevchr,error,timeout,done,sign,i,strcnt;

    chr = prevchr = 0;     /* present character & previous character */
    acqerrno = strcnt = error = 0;
    done = FALSE;
    sign = 1;
    strptr = strbuf;
    /* printf("cmddone: port %d, timeout: %d \n",port,timdown); */
    Ldelay(&timeout,timdown);              /* time down delay */
    while( chr != EOM && !done) /* wait for error(EOM) done(CR,.) code */
    {
      if( pifchr( port ))
      {
        rbyte = read( port, chrbuf, 1);
        chr = chrbuf[0];
#ifdef SERIAL_DEBUG
        printf("   Received by 'cmddone' :%4d Dec\t\t -->%2c\n", chr, chr);
#endif
        if (strptr != NULL)
        {
	    /* if replysize is 10, then bindex could go up to 9 but really
   	       should stop at 8 so index 9 remains the nul character.	*/
	   if (strcnt+1 < maxchr)
           {
	      *strptr++ = chr;
	      strcnt++;
	   }
        }
        if (chr == '-') sign = -1;  /* sign of value neg. */
        if (chr >= '0' && chr <= '9')  error = error*10 + (chr - '0');
        if (chr == PERIOD && ( prevchr == LF || prevchr == CR)) done = TRUE;
        prevchr = chr;
      }
      else
      {
#ifndef NOTIMEOUT
        /*printf( " inside TIMETRACE of cmddone \n" );*/
        if( Tcheck( timeout ))
        { 
#ifdef SERIAL_DEBUG
            printf( "cmddone timed out\n" );
#endif
            acqerrno = DONETIMEOUT;  /* return an unusual neg pattern for timeout */
            return( -1 );
        }
        /* taskDelay(calcSysClkTicks(17));  */
#endif
      }
    }       /* end of chr != EOM while */
   
    if (strbuf == NULL)
      return(error*sign);           /* return +/- error or status value */
    else
      return(0);		/* readreply returns 0  */
}

/*****************************************************************
* putstring - Send a string to a serial port
*/
int  putstring(int port, char * str)
{
    write(port, str, strlen(str));
}

/*****************************************************************
* getstring - Get a line from a serial port
*	Reads until the char "terminator" is encountered, or
*	the buffer is full, or timeout occurs.
*	Timeout time is in hundredths of a second.
*	If "buf" is non-null, it points to a buffer of length
*	"buflen" and the line is returned there and getstring
*	returns the value "buf".
*	If "buf" is null, returns a pointer to static storage.
*/
char *getstring(int port, char terminator, int timeout, char *buf, int buflen)
{
#define BUFLEN 101
    char chr;
    int i;
    static char linebuf[BUFLEN];
    int timchk;

    /* printf("getstring: %d, 0x%x, %d\n", port, terminator,timeout); */

    if (buf == NULL) {
	/* Use static stroage for string */
	buflen = BUFLEN;
	buf = linebuf;
    }
    Ldelay( &timchk, timeout );
    for (i=0, chr = ~terminator; i<buflen-1 && chr != terminator; )
    {
	if (pifchr(port))
	{
	    read(port, &chr, 1);
	    buf[i++] = chr;
  	    Ldelay( &timchk, timeout );	/* Update the timeout time */
	}
	else
	{
	    if (Tcheck(timchk))
	    {
		return NULL;
	    }
	    Tdelay(1);
	}
    }
    if (chr == terminator) --i;	/* Strip off the terminator character */
    buf[i] = '\0';
    return buf;
#undef BUFLEN
}

/****************************************************************
* getreply - Get a line from a serial port
*	Reads until a "nchars" characters have been received,
*	the buffer is full, or timeout occurs.
*	Only characters that appear in the "charlist" string
*	are counted in "nchars".
*	Timeout time is in hundredths of a second.
*	The pointer "buf" must point to a buffer of length
*	"buflen"; the line is returned there.
*	Returns the value "buf".
*/
char *getreply(int port, int timeout, char *buf, int buflen,
		char *charlist, int nchars)
{
    char chr;
    int i;
    int imptChars;
    int timchk;

    if (buf == NULL) {
	return NULL;
    }
    Ldelay( &timchk, timeout );
    for (i=imptChars=0; i<buflen-1 && imptChars<nchars; )
    {
	if (pifchr(port))
	{
	    read(port, &chr, 1);
	    buf[i++] = chr;
	    if (strchr(charlist, chr)) {
		imptChars++;
	    }
  	    Ldelay( &timchk, timeout );	/* Update the timeout time */
	}
	else
	{
	    if (Tcheck(timchk))
	    {
		return NULL;
	    }
	    Tdelay(1);
	}
    }
    buf[i] = '\0';
    return buf;
}

/*****************************************************************
*  Read a single byte from "port".
*  Returns the byte value, or -1 if no char available within "timeout".
*/
int getbyte(int port, int timeout)
{
    unsigned char chr;
    int timchk;

    Ldelay( &timchk, timeout );
    if (pifchr(port)) {
	read(port, &chr, 1);
    } else {
	if (Tcheck(timchk)) {
	    return -1;
	}
	Tdelay(1);
    }
    return chr;
}

/*****************************************************************
* Tdelay - Delay for # * 10 msec.
*  (eg. Tdelay(100) = 1 sec. delay)
*
*/
int  Tdelay( int val )
{
  taskDelay((sysClkRateGet() * val) / 100 );
  /* taskDelay(calcSysClkTicks(val*10));   perferred way, but the above does work, 3/8/06 GMB */
}

/*****************************************************************
* Ldelay - Setting up the timer with the elasped time passed to 'val'
*           that will used by Tcheck() to see if the time (delay)
*           requested has elasped .
*
*/
int  Ldelay( int *tickset, int val )
{
  *tickset = tickGet() + (( sysClkRateGet() * val) / 100);
}

/*****************************************************************
* Tcheck - Check if the Time delay started by Ldelay() is completed
*          if Done     returns 1 (TRUE)
*          if Not Done returns 0 (FALSE)
*
*/
int  Tcheck( int tickset )
{
  return ((tickGet() >= ((unsigned long)tickset)));
}  
