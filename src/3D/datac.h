/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef DATAC_H
#define DATAC_H

/*
  Include file to byte swap data file headers and data block headers
 */

#ifdef LINUX

#ifdef __INTERIX
#include <arpa/inet.h>
#else
#include <netinet/in.h>
#endif

static struct dbh_swap
{
   short s1;
   short s2;
   short s3;
   short s4;
   long  l1;
   long  l2;
   long  l3;
   long  l4;
   long  l5;
};
          
typedef union
{
   dblockhead *in_dbh;
   struct hypercmplxbhead *in_hch;
   struct dbh_swap *out;
} dbhUnion;

typedef union
{
   float *fval;
   int   *ival;
} floatInt;

#define DFH_CONVERT(data) \
{  \
   data->nblocks = ntohl(data->nblocks); \
   data->ntraces = ntohl(data->ntraces); \
   data->np = ntohl(data->np); \
   data->ebytes = ntohl(data->ebytes); \
   data->tbytes = ntohl(data->tbytes); \
   data->bbytes = ntohl(data->bbytes); \
   data->vers_id = ntohs(data->vers_id); \
   data->status = ntohs(data->status); \
   data->nbheaders = ntohl(data->nbheaders); \
}

#define DFH_CONVERT2(to, from) \
{  \
   to.nblocks = ntohl(from.nblocks); \
   to.ntraces = ntohl(from.ntraces); \
   to.np = ntohl(from.np); \
   to.ebytes = ntohl(from.ebytes); \
   to.tbytes = ntohl(from.tbytes); \
   to.bbytes = ntohl(from.bbytes); \
   to.vers_id = ntohs(from.vers_id); \
   to.status = ntohs(from.status); \
   to.nbheaders = ntohl(from.nbheaders); \
}

#define DFH_CONVERT3(to, from) \
{  \
   to.nblocks = ntohl(from->nblocks); \
   to.ntraces = ntohl(from->ntraces); \
   to.np = ntohl(from->np); \
   to.ebytes = ntohl(from->ebytes); \
   to.tbytes = ntohl(from->tbytes); \
   to.bbytes = ntohl(from->bbytes); \
   to.vers_id = ntohs(from->vers_id); \
   to.status = ntohs(from->status); \
   to.nbheaders = ntohl(from->nbheaders); \
}

#define DBH_CONVERT(data) \
{ \
   dbhUnion hU; \
                \
   hU.in_dbh = &data; \
   hU.out->s1 = htons(hU.out->s1); \
   hU.out->s2 = htons(hU.out->s2); \
   hU.out->s3 = htons(hU.out->s3); \
   hU.out->s4 = htons(hU.out->s4); \
   hU.out->l1 = htonl(hU.out->l1); \
   hU.out->l2 = htonl(hU.out->l2); \
   hU.out->l3 = htonl(hU.out->l3); \
   hU.out->l4 = htonl(hU.out->l4); \
   hU.out->l5 = htonl(hU.out->l5); \
}

#define DBH_CONVERT2(to, from) \
{ \
   dbhUnion hU_to, hU_from; \
                \
   hU_to.in_dbh = &to; \
   hU_from.in_dbh = &from; \
   hU_to.out->s1 = htons(hU_from.out->s1); \
   hU_to.out->s2 = htons(hU_from.out->s2); \
   hU_to.out->s3 = htons(hU_from.out->s3); \
   hU_to.out->s4 = htons(hU_from.out->s4); \
   hU_to.out->l1 = htonl(hU_from.out->l1); \
   hU_to.out->l2 = htonl(hU_from.out->l2); \
   hU_to.out->l3 = htonl(hU_from.out->l3); \
   hU_to.out->l4 = htonl(hU_from.out->l4); \
   hU_to.out->l5 = htonl(hU_from.out->l5); \
}

#define DBH_CONVERT3(to, from) \
{ \
   dbhUnion hU_to, hU_from; \
                \
   hU_to.in_dbh = &to; \
   hU_from.in_dbh = from; \
   hU_to.out->s1 = htons(hU_from.out->s1); \
   hU_to.out->s2 = htons(hU_from.out->s2); \
   hU_to.out->s3 = htons(hU_from.out->s3); \
   hU_to.out->s4 = htons(hU_from.out->s4); \
   hU_to.out->l1 = htonl(hU_from.out->l1); \
   hU_to.out->l2 = htonl(hU_from.out->l2); \
   hU_to.out->l3 = htonl(hU_from.out->l3); \
   hU_to.out->l4 = htonl(hU_from.out->l4); \
   hU_to.out->l5 = htonl(hU_from.out->l5); \
}

#else

#define DFH_CONVERT(data)
#define DBH_CONVERT(data)

#endif

#endif
