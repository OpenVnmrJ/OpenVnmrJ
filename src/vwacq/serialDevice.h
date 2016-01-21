/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef INCserialDeviceh
#define INCserialDeviceh

/* ------------- Make C header file C++ compliant ------------------- */
#ifdef __cplusplus
extern "C" {
#endif

/* --------- ANSI/C++ compliant function prototypes --------------- */

#if defined(__STDC__) || defined(__cplusplus)

extern int initPort(char *device);
extern int initSerialPort( int port );
extern int  pputchr( int port, char cmd ) ;
extern int pifchr( int  port );
extern int clearport( int port );
extern int cmdecho( int port, int type );
extern int echoval( int port, int value );
extern int cmddone(int port, int timdown);
extern int readreply(int port, int timdown, char *replybuf, int replysize );
extern int  Tdelay( int val );
extern int  Ldelay( int *tickset, int val );
extern int  Tcheck( int tickset );

/* --------- NON-ANSI/C++ prototypes ------------  */
#else
 
extern int initPort();
extern int initSerialPort();
extern int  pputchr() ;
extern int pifchr();
extern int clearport();
extern int cmdecho();
extern int echoval();
extern int cmddone();
extern int readreply();
extern int  Tdelay();
extern int  Ldelay();
extern int  Tcheck();

#endif  /* __STDC__ */
 
#ifdef __cplusplus
}
#endif

#endif /* INCrngBlkLibh */
