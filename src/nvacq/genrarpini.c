/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <fcntl.h>
#include "stdio.h"
/* // IP 172.16.0.20
// MAC 00:60:93:03:10:01
*/
void prtIP(unsigned long IP)
{
    unsigned char *p;

    p = (char *)&IP;
    printf ("%.1d.%.1d.%.1d.%.1d\n",p[0],p[1],p[2],p[3]);
}

void prtMAC(unsigned char *pMAC)
{
    printf(" %02x:%02x:%02x:%02x:%02x:%02x\n",
           pMAC[0],pMAC[1],pMAC[2],pMAC[3],pMAC[4],pMAC[5]);
}


int MACstr2Long(char *macstr, unsigned char *pMAC)
{
      unsigned short val1,val2,val3,val4,val5,val6;
      int result;

      /* printf("MAC str: '%s'\n",macstr); */
      result = sscanf(macstr,"%2hx:%2hx:%2hx:%2hx:%2hx:%2hx",&val1,&val2,&val3,&val4,&val5,&val6);
      if (result != 6)  /* if didn't convert six values then there is something wrong! */
        return -1;

      pMAC[0]= (unsigned char) val1;
      pMAC[1]= (unsigned char) val2;
      pMAC[2]= (unsigned char) val3;
      pMAC[3]= (unsigned char) val4;
      pMAC[4]= (unsigned char) val5;
      pMAC[5]= (unsigned char) val6;
      /* printf("MAC: "); prtMAC(pMAC); */
      return 0;
}

int IPstr2Long(char *IPstr, unsigned long *pIP)
{
      // unsigned short val1,val2,val3,val4,val5,val6;
      unsigned long IPvalue;
      unsigned char *sp;
      int result;

      // sp = (short *) &IPvalue;
      sp = (char *) pIP;
      /* printf("MAC str: '%s'\n",macstr); */
      result = sscanf(IPstr,"%3hd.%3hd.%3hd.%3hd",&(sp[0]),&(sp[1]),&(sp[2]),&(sp[3]));
      if (result != 4)  /* if didn't convert six values then there is something wrong! */
      {
          printf ("Fialed to convert: %d\n",result);
          return -1;
      }
      printf ("%.1d.%.1d.%.1d.%.1d\n",sp[0],sp[1],sp[2],sp[3]);
      // *pIP = IPvalue; 
      return  0;
}

prtUsage()
{
  printf("Usage:   genrarpini MAC IP), MAC must match the card's MAC based on type and position.\n");
  printf("E.G.     genrarpini 00:60:93:03:10:01 172.16.0.20 (e.g. rf1)\n");
  printf("         genrarpini 00:60:93:03:10:02 172.16.0.21 (e.g. rf2)\n");
  printf("         genrarpini 00:60:93:03:00:01 172.16.0.10 (e.g. master1)\n");
  printf("         genrarpini 00:60:93:03:50:01 172.16.0.50 (e.g. ddr1)\n");
  
// IP 172.16.0.20
// MAC 00:60:93:03:10:01
}
main(int argc, char* argv[])
{
    char *macstr, *IPstr;
    unsigned long IPlong, N_IPLong;
    unsigned char MAC[6], NMAC[6];
    unsigned short *npm, *hpm;
    int i;
    size_t fd;
    if (argc < 3) {
       prtUsage();
       exit(0);
    }

    printf("argc: %d,    argv1: '%s', argv2: '%s'\n", argc, argv[1], argv[2]);
    // macstr = "00:60:93:03:10:01";
    // IPstr = "172.16.0.20";
    macstr = argv[1];
    IPstr = argv[2];
    MACstr2Long(macstr,MAC);
    IPstr2Long(IPstr, &IPlong);
    
    prtIP(IPlong);
    prtMAC(MAC);
   fd = open("./rarp.ini",O_RDWR | O_TRUNC |O_CREAT);
   if (fd != 0) {
       write(fd, MAC, 6 );
       write(fd, &IPlong, 4);
       close(fd);
   }
}
