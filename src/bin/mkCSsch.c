/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/* mkCSsch.c - make Compressive Sensing sampling schedule. Eriks Kupce.  
   Routine for generating weighted nD random sampling schedules (n < 5)
   usage: mkCSsch <-w> fname ni nimax <ni2 ni2max> <ni3 ni3max> <skey> 

   The probability distribution functions must be stored in the current experiment
   with filenames "sampling_ni.pro", "sampling_ni2.pro" and "sampling_ni3.pro". The
   -w option indicates weighted sparse sampling (SPARSE = 'w'). Missing files indicate
   uniform random distribution.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "vnmrio.h"
#include "random.h"
        
int main(argc,argv)   /* number of random points must be less than 50 %, otherwise do the linear prediction */
int argc;
char *argv[];
{
  char        fname[MAXSTR];
  int         i, ac, ws, ni, nimax, ni2, ni2max, ni3, ni3max, skey;

  ac=argc; i=1; ws=0; skey = 0; 
  ni=1; ni2=1; ni3=1; nimax=1; ni2max=1; ni3max=1;

  if((ac > 1) && (argv[i][0]=='-'))
  {
    ac--; 
    if(argv[i][1]=='w') ws++;                // SPARSE = 'w'
    else if (argv[i][1]=='n') exit(0);       // SPARSE = 'n'
    i++;
  }
  if(ac < 4) vn_error("Usage: mkCSsch <-w> fname ni nimax <ni2 ni2max> <ni3 ni3max> <seed> ");
  if(ac > 3)
  {  
    strcpy(fname, argv[i]);        
    i++;
    if(sscanf(argv[i], "%d", &ni) == 0)    
      vn_error("Usage: mkCSsch <-w> fname ni nimax <ni2 ni2max> <ni3 ni3max> <seed> ");
    i++;
    if(sscanf(argv[i], "%d", &nimax) == 0)  
      vn_error("Usage: mkCSsch <-w> fname ni nimax <ni2 ni2max> <ni3 ni3max> <seed> ");
    i++;
    if(ni < 1)    ni=1;
    if(nimax <= ni) nimax=1;
  }
  if(ac > 4)
  {
    if(sscanf(argv[i], "%d", &skey) == 0) 
      vn_error("Usage: mkCSsch <-w> fname ni nimax <ni2 ni2max> <ni3 ni3max> <seed> ");
    i++;
  }
  if(ac > 5)
  {
    ni2 = skey; skey = 0;
    if(sscanf(argv[i], "%d", &ni2max) == 0) 
      vn_error("Usage: mkCSsch <-w> fname ni nimax <ni2 ni2max> <ni3 ni3max> <seed> ");
    i++;
    if(ni2 < 1)    ni2=1;
    if(ni2max <= ni2) ni2max=1;
  }
  if(ac > 6)
  {
    if(sscanf(argv[i], "%d", &skey) == 0)  
      vn_error("Usage: mkCSsch <-w> fname ni nimax <ni2 ni2max> <ni3 ni3max> <seed> ");
    i++;
  }
  if(ac > 7)
  {
    ni3=skey; skey = 0;
    if(sscanf(argv[i], "%d", &ni3max) == 0) 
      vn_error("Usage: mkCSsch <-w> fname ni nimax <ni2 ni2max> <ni3 ni3max> <seed> ");
    i++;
    if(ni3 < 1)    ni3=1;
    if(ni3max <= ni3) ni3max=1;
  } 
  if(ac > i)
  {
    if(sscanf(argv[i], "%d", &skey) == 0)
      vn_error("Usage: mkCSsch <-w> fname ni nimax <ni2 ni2max> <ni3 ni3max> <seed> ");
  }
  if(skey == 0) skey=169;  
  
  (void) make_sch(fname, ni, ni2, ni3, nimax, ni2max, ni3max, skey, ws);
           
  printf("sampling schedule done.\n"); 

  return(1); 
}





