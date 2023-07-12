/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/*----------------------------------------------------------------------
|
|	Header file for Shim Dac  definitions
|
+----------------------------------------------------------------------*/

#include <iostream.h>
#include <string.h>
#include "Shim.h"
#include "cpsg.h"
#include "pvars.h"

#if !defined(STANDALONE) && !defined(ACQSTAT)
#include "group.h"
#endif 

#define	MAXSTR	256
#define Z0         1
#define ISACTIVE(a)   ( valid_mask[shimset][((a) / 16)] & (1 << ((a) % 16)) )

extern "C" double sign_add(double,double);
extern "C" char	  load[MAXSTR];

char *sh_names_std[MAX_SHIMS] = { 
	"z0f",         /* Dac  0  */
	"z0",          /* Dac  1  */
	"z1",          /* Dac  2  */
	"z1c",         /* Dac  3  */
	"z2",          /* Dac  4  */
	"z2c",         /* Dac  5  */
	"z3",          /* Dac  6  */
	"z4",          /* Dac  7  */
	"z5",          /* Dac  8  */
	"z6",          /* Dac  9  */
	"z7",          /* Dac 10  */
	"z8",          /* Dac 11  */
	"zx3",         /* Dac 12  */
	"zy3",         /* Dac 13  */
	"z4x",         /* Dac 14  */
	"z4y",         /* Dac 15  */
	"x1",          /* Dac 16  */
	"y1",          /* Dac 17  */
	"xz",          /* Dac 18  */
	"xy",          /* Dac 19  */
	"x2y2",        /* Dac 20  */
	"yz",          /* Dac 21  */
	"x3",          /* Dac 22  */
	"y3",          /* Dac 23  */
	"yz2",         /* Dac 24  */
	"zx2y2",       /* Dac 25  */
	"xz2",         /* Dac 26  */
	"zxy",         /* Dac 27  */
	"z3x",         /* Dac 28  */
	"z3y",         /* Dac 29  */
	"z2x2y2",      /* Dac 30  */
	"z2xy",        /* Dac 31  */
	"z3x2y2",      /* Dac 32  */
	"z3xy",        /* Dac 33  */
	"z2x3",        /* Dac 34  */
	"z2y3",        /* Dac 35  */
	"z3x3",        /* Dac 36  */
	"z3y3",        /* Dac 37  */
	"z5x",         /* Dac 38  */
	"z5y",         /* Dac 39  */
	"z4x2y2",      /* Dac 40  */
	"z4xy",        /* Dac 41  */
	"z3c",         /* Dac 42  */
	"z4c",         /* Dac 43  */
	"x4",          /* Dac 44  */
	"y4",          /* Dac 45  */
	"",            /* Dac 46  */
	"",            /* Dac 47  */
   };

/*  Special list for shim set 12.
 *  x4 uses index of z4x
 *  y4 uses index of z4y
 */

char *sh_names_12[MAX_SHIMS] = { 
	"z0f",         /* Dac  0  */
	"z0",          /* Dac  1  */
	"z1",          /* Dac  2  */
	"z1c",         /* Dac  3  */
	"z2",          /* Dac  4  */
	"z2c",         /* Dac  5  */
	"z3",          /* Dac  6  */
	"z4",          /* Dac  7  */
	"z5",          /* Dac  8  */
	"z6",          /* Dac  9  */
	"z7",          /* Dac 10  */
	"z8",          /* Dac 11  */
	"zx3",         /* Dac 12  */
	"zy3",         /* Dac 13  */
	"x4",          /* Dac 14  this are special for shim set 12 */
	"y4",          /* Dac 15  this are special for shim set 12 */
	"x1",          /* Dac 16  */
	"y1",          /* Dac 17  */
	"xz",          /* Dac 18  */
	"xy",          /* Dac 19  */
	"x2y2",        /* Dac 20  */
	"yz",          /* Dac 21  */
	"x3",          /* Dac 22  */
	"y3",          /* Dac 23  */
	"yz2",         /* Dac 24  */
	"zx2y2",       /* Dac 25  */
	"xz2",         /* Dac 26  */
	"zxy",         /* Dac 27  */
	"z3x",         /* Dac 28  */
	"z3y",         /* Dac 29  */
	"z2x2y2",      /* Dac 30  */
	"z2xy",        /* Dac 31  */
	"z3x2y2",      /* Dac 32  */
	"z3xy",        /* Dac 33  */
	"z2x3",        /* Dac 34  */
	"z2y3",        /* Dac 35  */
	"z3x3",        /* Dac 36  */
	"z3y3",        /* Dac 37  */
	"z5x",         /* Dac 38  */
	"z5y",         /* Dac 39  */
	"z4x2y2",      /* Dac 40  */
	"z4xy",        /* Dac 41  */
	"z3c",         /* Dac 42  */
	"z4c",         /* Dac 43  */
	"",            /* Dac 44  used to be x4  */
	"",            /* Dac 45  used to be y4 */
	"",            /* Dac 46  */
	"",            /* Dac 47  */
   };

/*  These masks must match the names laid out in vconfig::system_panel.c	*/

/*
 * Useful for defining Hex values in valid_mask
 *
 * 1:  z4y z4x zy3 zx3 | z8 z7 z6 z5 | z4 z3 z2c z2 | z1c z1 z0 z0f
 * 
 * 2:  z2xy z2x2y2 z3y z3x | zxy xz2 zx2y2 yz2 | y3 x3 yz x2y2 | xy xz y1 x1
 * 
 * 3:  y4 x4 | z4c z3c z4xy z4x2y2 | z5y z5x z3y3 z3x3 | z2y3 z2x3 z3xy z3x2y2
 *
 */


unsigned short valid_mask[NUM_SHIMSET][4] = {
      { 0,0,0,0 },                 /* valid_mask for shimset 0  */
				   /* Index 0 is NOT USED  */
      { 0x00fe,0x00ff,0,0 },       /* valid_mask for shimset 1  */
      { 0x01fe,0x0fff,0,0 },       /* valid_mask for shimset 2  */
      { 0x03d6,0xffff,0,0 },       /* valid_mask for shimset 3  */
      { 0xf7d6,0xffff,0,0 },       /* valid_mask for shimset 4  */
      { 0xfffe,0xffff,0x0fff,0 },  /* valid_mask for shimset 5  */
      { 0x01d6,0x0fff,0,0 },       /* valid_mask for shimset 6  */
      { 0x01d6,0x3fff,0,0 },       /* valid_mask for shimset 7  */
      { 0x00d4,0x0f3f,0,0 },       /* valid_mask for shimset 8 (SISCO)  */
      { 0xf7d6,0xffff,0x33ff,0 },  /* valid_mask for shimset 9  */
      { 0x01fe,0x00ff,0,0 },       /* valid_mask for shimset 10 */
      { 0x0014,0x003f,0,0 },       /* valid_mask for shimset 11 */
      { 0xf1d6,0xffff,0,0 },       /* valid_mask for shimset 12 */
      { 0xf3d6,0xffff,0x00c0,0 },  /* valid_mask for shimset 13 */
      { 0xf3d6,0xffff,0x33c3,0 },  /* valid_mask for shimset 14 */
      { 0x00d6,0x0f3f,0,0 },       /* valid_mask for shimset 15 */
      { 0x003e,0x0fff,0x0c00,0 },  /* valid_mask for shimset 16 */
   };


Shim::Shim()
{
   memset(codes, 0, sizeof(codes));  // this was missing
   shimset = 1;
   init_shimnames(GLOBAL);
}

#if !defined(STANDALONE) && !defined(ACQSTAT)

int  Shim::init_shimnames(int tree)
{
   double dbltmp;
   int    res;

   sh_names = sh_names_std;
   if ( (tree == SYSTEMGLOBAL) || (tree == GLOBAL) )
   {
      res = P_getreal( tree, "shimset", &dbltmp, 1);
      if (res >= 0)
      {
         shimset = (int) (dbltmp + 0.5);
         if (shimset < 1){
	    fprintf(stderr,"Illegal shimset number: %d\n", shimset);
            shimset = 1;
	 }
         else if (shimset > MAX_SHIMSET)
	 {
	    fprintf(stderr,"shimset number > MAX_SHIMSET (%d>%d)\n",
		    shimset, MAX_SHIMSET);
            shimset = MAX_SHIMSET;
	 }
         if (shimset == 12)
            sh_names = sh_names_12;
      }
      else
      {
         shimset = 1;		/* if recognized tree, default to Varian */
      }
   }
   else
   {
     shimset = -1;		/* if unrecognized tree, set all to active */
   }
   return( shimset );
}
#endif 

void Shim::init_shimnames_by_setnum(int newshimset)
{
    shimset = newshimset;
    sh_names = sh_names_std;
    if (shimset == 12) sh_names = sh_names_12;
}

int Shim::isactive( int index ) 	/* private */
{
   if (shimset > 0 && shimset <= MAX_SHIMSET)
   {
      return( ISACTIVE(index) );
   }
   else
   {
      return( 131071 );
   }
}

//---------------------------------------------------------------
char* Shim::get_shimname(int index)
{
   if ((index < MAX_SHIMS) && (index >= 0) && isactive(index))
   {
      return( sh_names[index]);
   }
   else
   {
      return(NULL);
   }
}

//---------------------------------------------------------------
int Shim::get_shimindex(char *shimname)
{
   int index;

   for (index = 0; index < MAX_SHIMS; index++)
   {
      if ( (strcmp(shimname,sh_names[index]) == 0) && isactive(index))
	 return(index);
   }
   return(-1);
}

/*------------------------------------------------------------------
|	loadshims()/0
|	Load shims if load == 'y'.  Will be set by arrayfuncs if
|	any shims are arrayed.
+-----------------------------------------------------------------*/
int* Shim::loadshims()
{
    char load[MAXSTR];
    if ((P_getstring(CURRENT,"load",load,1,15) >= 0) &&
        ((load[0] == 'y') || (load[0] == 'Y')) )
    {
        int index;
        char *sh_name;
        P_setstring(CURRENT,"load","n",0);
        codes[Z0] = MAX_SHIMS ;	/* use this slot, it is skipped */
        for (index= Z0 + 1; index < MAX_SHIMS; index++)
        {
           if ((sh_name = get_shimname(index)) != NULL)
           {
              codes[index] =  (int) sign_add(getval(sh_name), 0.0005);
           }
           else {
             codes[index] = 0;
           }
        }
        return (codes + 1);
    }
    return(NULL);
}

/*
 * To use this tester, define STANDALONE above.  This simply prints out
 * the names of the active shims for each shim set.
 */

#ifdef STANDALONE
main()
{
   int index;
   char *shimname;

   for (shimset=1; shimset<=MAX_SHIMSET; shimset++)
   {
      if (shimset == 12)
         sh_names = sh_names_12;
      else
         sh_names = sh_names_std;
      fprintf(stdout,"Shimset %d\n",shimset);
      for (index=0; index < MAX_SHIMS; index++)
      {
         if ( (shimname = get_shimname(index)) != NULL)
            fprintf(stdout,"shim[%d] = %s\n",index,shimname);
      }
      fprintf(stdout,"***********\n\n");
   }
}
#endif 
