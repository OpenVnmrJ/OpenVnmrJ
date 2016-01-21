/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#define _POSIX_SOURCE 1

#include <termios.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "errLogLib.h"
#include "termhandler.h"

#define BUF_SIZE 10

static int sPort = -1, termAttrValid=0;
static struct termios termAttrSaved;


/*   check POSIX book page 158, etc.. to fine tune terminal
     parameters
*/

/******************************************************************
* initPort - initialize terminal serial port
*
*
*/
int initPort(char *device, int robotype)
{
    struct termios t;
    struct stat buf;
    mode_t mode;
    int old_euid;

    /* fprintf(stderr,"dev: '%s', type = %d\n",device,robotype); */

    /* Before opening the serial port for RDWR will will attempt
     *  to set the permission so that the open will not fail.
     * Since this is typically ran as root there should be no
     *  problem in doing this
     */
    if ( stat(device, &buf) != 1)  /* read what it is,       */
                                   /* this is a device file, */
                                   /* caution required       */
    {
        /* Just OR in the bits needed, "read/write by other" */
        buf.st_mode |= (S_IROTH | S_IWOTH);

        /* Change to real ID, from effective user id */
        old_euid = geteuid();
        seteuid( getuid() );

        /* Don't care if it doesn't work */
        /*
         * if (chmod(device, buf.st_mode) == -1)
         *     errLogSysRet(ErrLogOp, debugInfo, "Couldn't chmod on : '%s'",
         *         device);
         */

        /* Go back to effective user Id */
        seteuid( old_euid );
    }

#ifdef MASPROC
    sPort = open(device, O_RDWR|O_NOCTTY|O_NONBLOCK);
#else
    sPort = open(device, O_RDWR|O_NOCTTY);
#endif
    if (sPort == -1)
        errLogSysQuit(ErrLogOp,debugInfo,"Couldn't open: '%s'",device);

    if (tcgetattr(sPort, &t) !=0)
    {
        errLogSysQuit(ErrLogOp, debugInfo,
          "initPort: tcgetattr() - Could get attributes from: '%s'",device);
    }

    /* Don't overwrite the original attributes */
    if (!termAttrValid)
    {
        /* save these attributes to be restored when exiting */
        memcpy(&termAttrSaved,&t,sizeof(struct termios));
        termAttrValid = 1;
    }

/*******************   ASM, SMS, NMS, pick and place robots *****************/

    if (robotype < GILSON215_SERIAL_PORT)
    {
        /* fprintf(stderr,"ASM_SMS_TYPE\n"); */
        t.c_cc[VMIN] = 4;     /* read pends until 4 chars arrive */
        t.c_cc[VTIME] = 1;    /* read pends until 0.1 seconds after
                               *   first char arrives
                               * The combination of VMIN/VTIME will cause the 
                               * read to return if the time between received
                               * chars exceeds 0.1 seconds or after 4 chars
                               * are received which ever comes first
                               */
        /* Serial Input Control Bits */
        t.c_iflag &= ~(BRKINT        /* ignore break
             | IGNPAR | PARMRK       /* Ignore parity        */
             | INPCK  |              /* Ignore parity        */
               ISTRIP |              /* DOn't mask to 7 bits */
               INLCR | IGNCR | ICRNL /* no ,cr. of <lf>      */
             | IXON );               /* Ignore STOP char     */
        t.c_iflag |= IGNBRK |        /* Ignore BREAK         */
              ISTRIP |               /* strip to 7 chars     */
              IXOFF;                 /* send XON & XOFF for
                                      * flow control
                                      */
        t.c_oflag &= ~(OPOST);       /* No output flags      */
        t.c_lflag &= ~(              /* No local flags. In   */
        ECHO | ECHOE|ECHOK|ECHONL|   /* particular, no echo  */
        ICANON |                     /* no canonical input   */
                                     /* processing,          */
        ISIG |                       /* no signals           */
        NOFLSH |                     /* no queue flush       */
        TOSTOP);                     /* and no job control   */

        t.c_cflag &= (               /* clear out old bits   */
         CSIZE |                     /* Character size       */
         CSTOPB |                    /* Two stop bits        */
         HUPCL |                     /* Hangup on last close */
         PARENB);                    /* Parity               */

        t.c_cflag |= (
             CLOCAL |                /* CLOCAL => No modem   */
             CREAD |                 /* enable receiver      */
             CS8);                   /* CS8 => 8-bit data    */

        /* Set terminal speeds */
        if (cfsetispeed(&t, B9600) == -1)
        {
            errLogSysQuit(ErrLogOp, debugInfo,
                "initPort: cfsetispeed() - Could set input speed for: '%s'",
                device);
        }
        if (cfsetospeed(&t, B9600) == -1)
        {
            errLogSysQuit(ErrLogOp, debugInfo,
                "initPort: cfsetospeed() - Could set output speed for: '%s'",
                device);
        }
    }

/*************************   Gilson Liquid Handler 215 **********************/

    else if (robotype == GILSON215_SERIAL_PORT)
    {
        /* fprintf(stderr,"GILSON215_SERIAL_PORT\n"); */
        t.c_cc[VMIN] = 0;     /* read pends till 2 sec elapsed then timeout */
        t.c_cc[VTIME] = 20;   /* read pends till 2.0 seconds has elapsed  */
                              /* The combination of VMIN/VTIME will cause the
                               * read to return if the time between received
                               * chars exceeds 0.1 seconds or after 4 chars
                               * are received which ever comes first
                               */

        /* Serial Input Control Bits */
        t.c_iflag &= ~(              /* Turn OFF the Bits For:   */
               IGNBRK                /* Ignore Break */
             | BRKINT                /* Signal Intrp On break */
             | IGNPAR                /* Ignore Parity        */
             | PARMRK                /* Mark Parity Errors   */
             | INPCK                 /* Enable Input Parity Check */
             | ISTRIP                /* Strip Characters to 7-bits */
             | INLCR                 /* Map NL to CR on Input */
             | IGNCR                 /* Ignore CR            */
             | ICRNL                 /* Map CR to NL on Input */
             | IXON                  /* Enable start/stop output control */
             | IXOFF                 /* Enable start/stop input control */
#ifndef _POSIX_SOURCE                /* Not POSIX: */
             | IUCLC                 /* Map Uppercase to lowercase on input */
             | IXANY                 /* Enable any char to restart output */
             | IMAXBEL               /* Echo BEL on input line too long */
#endif
             );

        /* Now Set the ones needed */
        t.c_iflag |= (               /* Turn On the Bits For : */
               IGNBRK                /* Ignore Break */
             | ISTRIP                /* Strip Charaters to 7-bits */
             | IXOFF                 /* Enable start/stop input control */
             );

        /* Output Pre-Processing Modes */
        t.c_oflag &= ~(OPOST);       /* No output flags, */
                                     /* No processing,*/
                                     /* No change to chars */


        /* Local Terminal Control */
        t.c_lflag &= ~(              /* Turn Off Bits For : */
             ISIG                    /* Enable Signals       */
             | ICANON                /* Canonical input (i.e. cooked) */
             | ECHO                  /* Enable echo          */
             | ECHOE                 /* Echo erase char as BS-SP-BS */
             | ECHOK                 /* Echo NL after Kill char */
             | ECHONL                /* Echo NL              */
             | NOFLSH                /* Disable flush after interp or quit */
             | TOSTOP                /* Send SIGTTOU for background output */
             | IEXTEN                /* Enable extend functions */
#ifndef _POSIX_SOURCE                /* Not POSIX: */
             | XCASE                 /* Canonical upper/lower presentation */
             | ECHOCTL               /* Echo cntrl char as ^char, del as ^? */
             | ECHOPRT               /* Echo erase char as char erased */
             | ECHOKE                /* BS-SP-BS erase entire line on  */
                                     /*    line kill */
             | FLUSHO                /* Output is being flushed */
             | PENDIN                /* Retype pending input at next read */
                                     /*    or input char */
#endif
             );

        /* Control Modes */
        t.c_cflag &= ~(              /* Turn OFF the Bits For:   */
              CSIZE                  /* Char Size Bits       */
             | CSTOPB                /* Two Stop Bits        */
             | PARENB                /* Parity Enable        */
             | PARODD                /* Odd Parity           */
             | HUPCL                 /* Hang up on last close */
             | CLOCAL                /* Local Line, else Modem */
#ifndef _POSIX_SOURCE                /* Not POSIX: */
             | CIBAUD                /* Input Baud rate if diff from output */
             | PAREXT                /* Ext parity for mark & space parity */
             | CRTSXOFF              /* Enable inbound h/w flow control */
             | CRTSCTS               /* Enable outbound h/w flow control */
/*           | CBAUDEXT              /* Indicates output speed > B38400 */
/*           | CIBAUDEXT             /* Indicates input speed > B38400 */
#endif
             );                      /* we control speed by separate call */

        t.c_cflag |= (               /* Turn On the Bits For : */
             CSTOPB                  /* Two Stop Bits        */
             | CLOCAL                /* Local Line, else Modem */
             | CREAD                 /* enable receiver      */
             | PARENB                /* enable Parity        */
             | CS8                   /* CS8 => 8-bit data    */
             );                      /* CS5, CS6, CS7, CS8 char sizes */
 
        /* Set terminal speeds */
        if (cfsetispeed(&t, B19200) == -1)
        {
            errLogSysQuit(ErrLogOp, debugInfo,
              "initPort: cfsetispeed() - Could set input speed for: '%s'",
              device);
        }
        if (cfsetospeed(&t, B19200) == -1)
        {
            errLogSysQuit(ErrLogOp,debugInfo,
              "initPort: cfsetospeed() - Could set output speed for: '%s'",
              device);
        }
    }

/*******************   Imaging Patient Table **********************/

    else  /* Patient Table settings */
    {
        /* fprintf(stderr,"PATIENT_TABLE_SERIAL_PORT\n"); */
        t.c_cc[VMIN] = 0;     /* read pends till 2 sec elapsed then timeout */
        t.c_cc[VTIME] = 20;   /* read pends till 2.0 seconds has elapsed  */
                              /* The combination of VMIN/VTIME will cause the
                               * read to return if the time between received
                               * chars exceeds 0.1 seconds or after 4 chars
                               * are received which ever comes first
                               */

        /* Serial Input Control Bits */
        t.c_iflag &= ~(              /* Turn OFF the Bits For:   */
               IGNBRK                /* Ignore Break */
             | BRKINT                /* Signal Intrp On break */
             | IGNPAR                /* Ignore Parity        */
             | PARMRK                /* Mark Parity Errors   */
             | INPCK                 /* Enable Input Parity Check */
             | ISTRIP                /* Strip Characters to 7-bits */
             | INLCR                 /* Map NL to CR on Input */
             | IGNCR                 /* Ignore CR            */
             | ICRNL                 /* Map CR to NL on Input */
             | IXON                  /* Enable start/stop output control */
             | IXOFF                 /* Enable start/stop input control */
#ifndef _POSIX_SOURCE                /* Not POSIX: */
             | IUCLC                 /* Map Uppercase to lowercase on input */
             | IXANY                 /* Enable any char to restart output */
             | IMAXBEL               /* Echo BEL on input line too long */
#endif
             );

        /* Now Set the ones needed */
        t.c_iflag |= (               /* Turn On the Bits For : */
               IGNBRK                /* Ignore Break */
             );

        /* Output Pre-Processing Modes */
        t.c_oflag &= ~(OPOST);       /* No output flags, */
                                     /* No processing, */
                                     /* No change to chars */


     /* Local Terminal Control */
        t.c_lflag &= ~(              /* Turn Off Bits For : */
             ISIG                    /* Enable Signals       */
             | ICANON                /* Canonical input (i.e. cooked) */
             | ECHO                  /* Enable echo          */
             | ECHOE                 /* Echo erase char as BS-SP-BS */
             | ECHOK                 /* Echo NL after Kill char */
             | ECHONL                /* Echo NL              */
             | NOFLSH                /* Disable flush after interp or quit */
             | TOSTOP                /* Send SIGTTOU for background output */
             | IEXTEN                /* Enable extend functions */
#ifndef _POSIX_SOURCE                /* Not POSIX: */
             | XCASE                 /* Canonical upper/lower presentation */
             | ECHOCTL               /* Echo ctrl char as ^char, del as ^? */
             | ECHOPRT               /* Echo erase char as char erased */
             | ECHOKE                /* BS-SP-BS erase entire line on */
                                     /*    line kill */
             | FLUSHO                /* Output is being flushed */
             | PENDIN                /* Retype pending input at next read */
                                     /*    or input char */
#endif
             );

        /* Control Modes */
        t.c_cflag &= ~(              /* Turn OFF the Bits For:   */
              CSIZE                  /* Char Size Bits       */
             | CSTOPB                /* Two Stop Bits        */
             | PARENB                /* Parity Enable        */
             | PARODD                /* Odd Parity           */
             | HUPCL                 /* Hang up on last close */
             | CLOCAL                /* Local Line, else Modem */
#ifndef _POSIX_SOURCE                /* Not POSIX: */
             | CIBAUD                /* Input Baud rate if diff from output */
             | PAREXT                /* Ext parity for mark & space parity */
             | CRTSXOFF              /* Enable inbound h/waflow control */
             | CRTSCTS               /* Enable outbound h/w flow control */
/*           | CBAUDEXT              /* Indicates output speed > B38400 */
/*           | CIBAUDEXT             /* Indicates input speed > B38400 */
#endif
             );                      /* we control speed by separate call */

        t.c_cflag |= (               /* Turn On the Bits For : */
               CLOCAL                /* Local Line, else Modem */
              | CREAD                /* enable receiver      */
              | CS8                  /* CS8 => 8-bit data    */
             );                      /* CS5, CS6, CS7, CS8 char sizes */
 
        /* Set terminal speeds */
        if (cfsetispeed(&t, B9600) == -1)
        {
            errLogSysQuit(ErrLogOp, debugInfo,
              "initPort: cfsetispeed() - Could set input speed for: '%s'",
              device);
        }
        if (cfsetospeed(&t, B9600) == -1)
        {
            errLogSysQuit(ErrLogOp, debugInfo,
              "initPort: cfsetospeed() - Could set output speed for: '%s'",
              device);
        }
    }



    /* clear serial port of any garbarge */
    if (tcflush(sPort,TCIOFLUSH) == -1)
    {
        errLogSysQuit(ErrLogOp, debugInfo,
          "initPort: tcflush() - Could flush serial device: '%s'", device);
    }

    /* set terminal attributes */
    if (tcsetattr(sPort,TCSANOW, &t) == -1)
    {
        errLogSysQuit(ErrLogOp, debugInfo,
          "initPort: tcsetattr() - Couldn't set atrtributes on device: '%s'",
          device);
    }

    return(sPort);
}

int flushPort(int sPort)
{
    return (tcflush(sPort, TCIOFLUSH));
}

int drainPort(int sPort)
{
  return (tcdrain(sPort));
}

/******************************************************************
* restorePort - restore terminal serial port to previous settings
*
*
*/
int restorePort(int sPort)
{
    /* reset terminal attributes */
    if (!termAttrValid)
        return 0;

    if (tcsetattr(sPort,TCSANOW, &termAttrSaved) == -1)
    {
        errLogSysRet(ErrLogOp, debugInfo,
          "initPort: tcsetattr() - Couldn't set atrtributes on device: %d",
          sPort);
        return(-1);
    }

    return(0);
}

/* write(sPort,buf,bufsize); tcdrain(sPort);   /* wait to all char are sent */
/* read(sPort,buf,bufsize); */

