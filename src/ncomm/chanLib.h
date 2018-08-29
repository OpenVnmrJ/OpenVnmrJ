/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
modification history
--------------------
01b,15Aug94    conforms to de facto WRS standards for include files
               transformed into a Public Interface Include file
01a,Aug94      written
*/

#ifndef __INCchanLibh
#define __INCchanLibh

#ifdef __cplusplus
extern "C" {
#endif

#define  NUM_CHANS	 8

#if defined(__STDC__) || defined(__cplusplus)
extern int openChannel( int channel, int access, int options );
extern int connectChannel( int channel );
extern int readChannel( int channel, char *datap, int bcount );
extern int flushChannel( int channel);
extern int writeChannel( int channel, char *datap, int bcount );
extern int closeChannel( int channel );
extern int registerChannelAsync( int channel, void (*callback)() );
#else
extern int openChannel();
extern int connectChannel();
extern int readChannel();
extern int flushChannel();
extern int writeChannel();
extern int closeChannel();
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus  */

#endif /* __INCsemLibh */
