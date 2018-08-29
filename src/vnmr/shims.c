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

#include <stdio.h>
#include <string.h>

#if !defined(STANDALONE) && !defined(ACQSTAT)
#include "group.h"
#include "pvars.h"
#endif 
#ifdef VNMRJ
#include "sockinfo.h"
#endif

/* To add a new shim set, the typical steps include:
 *  1. In shims.c, increment the MAX_SHIMSET define and
 *     add the valid_mask entry
 *  2. Update gzfit.c
 *  3. Increment max value for shimset in conpar and conpar.mr400
 *  4. Add new entry to config window
 *  5. Add index to switch in establishShimType in nvacq/shimHandlers.c
 *  6. Add index to switch in show_SHIM in bin/showconsole.c
 *  7. Add acq/acqigroupXX and acq/dgsXX files and update SConstruct.
 *  8. Add index to maclib/shimset
 *  9. Various gshim macros.  Augmap, gmapshims, gmapsys
 * 10. More 3dshim macros.  gxyzautoshim, gxyzfitshims, gxyzmapfield,
 *                          gxyzsetmap, gxyzsetshimgroup, gxyzmapsys
 * 11. shimtab/shimsetXX_<shimtype>
 * 12. For Inova, there are two choices. Update mapShimset() in this
 *     file or update vwacq/X_interp.c to handle additional cases in
 *     establishShimType(). This latter requires re-builds Inova console
 *     software, which I have been trying to avoid.
 */

#define MAX_SHIMS 48
#define MAX_SHIMSET 27
#define NUM_SHIMSET  MAX_SHIMSET+1
#define Z0         1
#define ISACTIVE(a)   ( valid_mask[shimset][((a) / 16)] & (1 << ((a) % 16)) )
#ifdef VNMRJ
#define ISTRANSVERSE(a)   ( transverse[((a) / 16)] & (1 << ((a) % 16)) )
#endif

static const char **sh_names;

static
const char *sh_names_std[MAX_SHIMS] = { 
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

static
const char *sh_names_12[MAX_SHIMS] = { 
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


static
unsigned short valid_mask[NUM_SHIMSET][4] = {
      { 0,0,0,0 },                  /* shimset 0  NOT USED   */
      { 0x00fe,0x00ff,0,0 },        /* shimset 1  Agilent 13 */
      { 0x01fe,0x0fff,0,0 },        /* shimset 2  Oxford  18 */
      { 0x03d6,0xffff,0,0 },        /* shimset 3  Agilent 23 */
      { 0xf7d6,0xffff,0,0 },        /* shimset 4  Agilent 28 */
      { 0xfffe,0xffff,0x0fff,0 },   /* shimset 5  Ultra   39 */
      { 0x01d6,0x0fff,0,0 },        /* shimset 6  Agilent 18 */
      { 0x03d6,0x3fff,0,0 },        /* shimset 7  Agilent 21 */
      { 0x00d4,0x0f3f,0,0 },        /* shimset 8  Oxford  15 */
      { 0xf7d6,0xffff,0x33ff,0 },   /* shimset 9  Agilent 40 */
      { 0x01fe,0x00ff,0,0 },        /* shimset 10 Agilent 14 */
      { 0x0014,0x003f,0,0 },        /* shimset 11 Whole Body */
      { 0xf1d6,0xffff,0,0 },        /* shimset 12 Agilent 26 */
      { 0xf3d6,0xffff,0x00c0,0 },   /* shimset 13 Agilent 29 */
      { 0xf3d6,0xffff,0x33c3,0 },   /* shimset 14 Agilent 35 */
      { 0x00d6,0x0f3f,0,0 },        /* shimset 15 Agilent 15 */
      { 0x003e,0x0fff,0x0c00,0 },   /* shimset 16 Ultra   18 */
      { 0xf3d6,0xffff,0,0 },        /* shimset 17 Agilent 27 */
      { 0x0056,0x0fff,0,0 },        /* shimset 18 Agilent Combo uImage   */
      { 0xf7d6,0xffff,0,0 },        /* shimset 19 Agilent 28 Thin (51mm) */
      { 0xf7d6,0xffff,0x00c3,0 },   /* shimset 20 Agilent 32 */
      { 0x31d6,0xffff,0,0 },        /* shimset 21 Agilent 24 */
      { 0xf7d6,0xffff,0,0 },        /* shimset 22 Oxford  28 */
      { 0xf7d6,0xffff,0,0 },        /* shimset 23 Agilent 28 Thin (54mm) */
      { 0xf3d6,0xffff,0,0 },        /* shimset 24 PremiumCompact+ 27     */
      { 0xf7d6,0xffff,0,0 },        /* shimset 25 PremiumCompact+ 28     */
      { 0xf7d6,0xffff,0,0 },        /* shimset 26 Agilent 32 (28 ch)     */
      { 0xf7d6,0xffff,0,0 },        /* shimset 27 Agilent 40 (28 ch)     */
   };

#ifdef VNMRJ
static unsigned short transverse[4] = {0xf000,0xffff,0xffff,0xffff};
#endif

static int shimset = 1;

#if !defined(STANDALONE) && !defined(ACQSTAT)
int
init_shimnames(int tree)
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
     shimset = -1;			/* if unrecognized tree, set all to active */
   }
   return( shimset );
}
#endif 

void
init_shimnames_by_setnum(int newshimset)
{
    shimset = newshimset;
    sh_names = sh_names_std;
    if (shimset == 12) sh_names = sh_names_12;
}

static int
isactive(int index )
{
	if (shimset > 0 && shimset <= MAX_SHIMSET)
	  return( ISACTIVE(index) );
	else
	{
	  return( 131071 );
         }
}


int isRRI()
{
   return( (shimset==5) || (shimset==11) || (shimset==16) );
}

int isThinShim()
{
   return( (shimset==19) || (shimset==23) );
}

/* Used by PSG. Inova console only needs the type of shims. This avoids changing X_interp.c to handle
   all the different values of shimset used by gradient shimming. X_interp.c currently recognizes
   shimsets <= 19. So map higher values to one of the existing ones. If we update X_interp.c, this
   function could be removed. Currently only called by psg/initauto.c
 */
int mapShimset(int shimset1)
{
   if (shimset1 == 23)  /* Thin shims */
      return(19);
   if (shimset1 == 24)  /* 27 channel shims */
      return(17);
   if (shimset >= 20)   /* 24, 28, and 32 channel shims */
      return(4);
   return(shimset1);
}

int shimCount()		// how many valid shim is in this shimset?
{
int i,j, cnt, tmp;
   if ( (shimset <= 0) || (shimset > MAX_SHIMSET) )
          return(0);
   cnt=0;
   for (i=0; i<4; i++)
   {  tmp = valid_mask[shimset][i];
      for (j=0; j<16; j++)
      {
         if (tmp & 1) cnt++;
         tmp >>= 1;
      }
   }
   return(cnt);
}

const char *get_shimname(int index)
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

#ifdef VNMRJ
void get_transverseShimSums(int fromPars, int *pos, int *neg)
{
   *pos = *neg = 0;
   if (shimset > 0 && shimset <= MAX_SHIMSET)
   {
      register int index;
      double dbltmp;
      int hwShimVal;

      for (index=Z0 + 1; index <= MAX_SHIMS; index++)
      {
         if (ISACTIVE(index) && ISTRANSVERSE(index) )
         {
            if (fromPars)
            {
               P_getreal( CURRENT, sh_names[index], &dbltmp, 1);
            }
            else
            {
               getExpStatusShim(index, &hwShimVal);
               dbltmp = (double) hwShimVal;
            }
            if (dbltmp >= 0.0)
               *pos += dbltmp;
            else
               *neg -= dbltmp;
         }
      }
   }
   return;
}
#endif

int get_shimindex(char *shimname)
{
   int index;

   for (index = 0; index < MAX_SHIMS; index++)
   {
      if ( (strcmp(shimname,sh_names[index]) == 0) && isactive(index))
	 return(index);
   }
   return(-1);
}


/*
 * To use this tester, define STANDALONE above.  This simply prints out
 * the names of the active shims for each shim set.
 */

#ifdef STANDALONE
int main()
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
