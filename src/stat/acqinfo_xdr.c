/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifdef USE_RPC

#include <sys/types.h>
#include <rpc/rpc.h>
#include "acqinfo.h"
/*-------------------------------------------------------------
|   acqinfo_xdr.c  - XDR routines to transfer acqinfo structure
|		     data via RPC
|
|  9-12-89	Generated via rpcgen.    Greg Brissey
|
+--------------------------------------------------------------*/

bool_t
xdr_a_string(xdrs, objp)
XDR            *xdrs;
a_string       *objp;
{
   if (!xdr_string(xdrs, objp, ~0))
   {
      return (FALSE);
   }
   return (TRUE);
}

bool_t
xdr_acqdata(xdrs, objp)
XDR            *xdrs;
acqdata        *objp;
{
   if (!xdr_int(xdrs, &objp->pid))
   {
      return (FALSE);
   }
   if (!xdr_int(xdrs, &objp->pid_active))
   {
      return (FALSE);
   }
   if (!xdr_int(xdrs, &objp->rdport))
   {
      return (FALSE);
   }
   if (!xdr_int(xdrs, &objp->wtport))
   {
      return (FALSE);
   }
   if (!xdr_int(xdrs, &objp->msgport))
   {
      return (FALSE);
   }
   if (!xdr_string(xdrs, &objp->host, ~0))
   {
      return (FALSE);
   }
   return (TRUE);
}

bool_t
xdr_ft3ddata(xdrs, objp)
XDR		*xdrs;
ft3ddata	*objp;
{
   if (!xdr_string(xdrs, &objp->autofilepath, ~0))
   {
      return (FALSE);
   }
   if (!xdr_string(xdrs, &objp->procmode, ~0))
   {
      return (FALSE);
   }
   if (!xdr_string(xdrs, &objp->username, ~0))
   {
      return (FALSE);
   }
   if (!xdr_string(xdrs, &objp->pathenv, ~0))
   {
      return (FALSE);
   }
   return (TRUE);
}

#endif  // USE_RPC
