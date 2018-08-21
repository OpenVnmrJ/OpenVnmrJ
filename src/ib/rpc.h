/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
*									
*
   MODIFIED by CG to use the modified version of svc.h
*/

/*
 * rpc.h, Just includes the billions of rpc header files necessary to
 * do remote procedure calling.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifndef _rpc_rpc_h
#define	_rpc_rpc_h

#include <rpc/types.h>		/* some typedefs */
#include <netinet/in.h>

/* external data representation interfaces */
#include <rpc/xdr.h>		/* generic (de)serializer */

/* Client side only authentication */
#include <rpc/auth.h>		/* generic authenticator (client side) */

/* Client side (mostly) remote procedure call */
#include <rpc/clnt.h>		/* generic rpc stuff */

/* semi-private protocol headers */
#include <rpc/rpc_msg.h>	/* protocol for rpc messages */
#include <rpc/auth_unix.h>	/* protocol for unix style cred */
#include <rpc/auth_des.h>	/* protocol for des style cred */

/* Server side only remote procedure callee */
// MODIFIED #include <rpc/svc.h>		/* service manager and multiplexer */
#include "svc.h"
#include <rpc/svc_auth.h>	/* service side authenticator */

EXTERN_FUNCTION ( extern int registerrpc, (u_long, u_long, u_long, char*(*)(), xdrproc_t, xdrproc_t)); 

EXTERN_FUNCTION ( extern int netname2user, (char [],int *,int *,int *,int *));
EXTERN_FUNCTION ( extern int user2netname, (char [], int, char*));
EXTERN_FUNCTION ( extern int netname2host, (char *,char *,int));
EXTERN_FUNCTION ( extern int key_setsecret, (char *));
EXTERN_FUNCTION ( extern int key_gendes,	(des_block *));
EXTERN_FUNCTION ( extern int key_encryptsession, (char *,des_block *));
EXTERN_FUNCTION ( extern int key_decryptsession, (char *,des_block *));
EXTERN_FUNCTION ( extern int host2netname,	( char *,char *,char *));
EXTERN_FUNCTION ( extern int getrpcport,	(char *,int,int,int));
EXTERN_FUNCTION ( extern int getnetname,	(char **));
EXTERN_FUNCTION ( extern int getsecretkey,	(char*, char*, char*));
EXTERN_FUNCTION ( extern int bindresvport,	( int, struct sockaddr_in *));
/*EXTERN_FUNCTION ( extern int callrpc,		( char *, int, int, int, xdrproc_t, xdrproc_t, char *, char *));*/
EXTERN_FUNCTION ( extern void get_myaddress,	(struct sockaddr_in *));

#endif /*!_rpc_rpc_h*/
