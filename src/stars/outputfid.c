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

#include <unistd.h>
#include <sys/file.h>
#include <netinet/in.h>

#include "starsprog.h"

#define S_DATA 0x1
#define S_32 0x4
#define S_COMPLEX 0x10
#define VERSID 65  /* This should be Vnmr 3.5 */

extern int np,sites;
extern float *spec;

void outputfid(argv5)
char argv5[];

/* output calculated FID in Vnmr format */
{
int fidfile;
struct datafilehead fhd;
struct datablockhead bh;
int i,nwrite,*intspec;
int totalBytes;

fidfile = creat(argv5,0666);
fhd.nblocks = htonl(1);
fhd.ntraces = htonl(1);
fhd.np = htonl(2*np);
fhd.ebytes = htonl(4);  /* double precision integers*/
fhd.tbytes = htonl(np*8);
totalBytes = np*8;
fhd.bbytes = htonl(np*8+sizeof(struct datablockhead));
fhd.vers_id = htons(VERSID);
fhd.status = htons(S_DATA + S_32 + S_COMPLEX);
fhd.nbheaders = htonl(1);
/*write fileheader */
nwrite = write(fidfile,&fhd,sizeof(struct datafilehead));
if (nwrite != sizeof(struct datafilehead)) nrerror("error in write datafilehead");
bh.scale = 0;
bh.status = htons(S_DATA+S_32+S_COMPLEX);
bh.index = htons(1);
bh.mode = 0;
bh.ctcount = htonl(1024);
bh.lpval = 0.0;
bh.rpval = 0.0;
bh.lvl = 0.0;
bh.tlt = 0.0;
nwrite = write(fidfile,&bh,sizeof(struct datablockhead));
if (nwrite != sizeof(struct datablockhead)) nrerror("error in write datablockhead");
intspec = intvector(0,2*np);
for (i=0; i<2*np; i++) intspec[i] = htonl((int)(spec[i]*32768));
nwrite = write(fidfile,intspec,totalBytes);
if (nwrite != totalBytes) nrerror("Error: Error writing fid file ");
free_intvector(intspec,0,2*np);
}
