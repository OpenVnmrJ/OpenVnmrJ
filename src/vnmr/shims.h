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


#define MAX_SHIMS 48    /*  maximum number of shim dacs */
#define Z0         1    /*  index for the lock offset (Z0) dac */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * initializes the shim set to the appropriate group
 * For example,  Varian or Oxford
 * The argument passed in the parameter tree to find the 'shimset' variable
 * This is passed since the variable is in the SYSTEMGLOBAL tree in Vnmr
 * but in the GLOBAL tree in PSG
 * init_shimnames will return the value of the shim set as an integer
 */
extern int init_shimnames( int tree );

/*
 * This returns a pointer to the name of a dac.  If a dac does not
 * exist for the supplied index,  or if the dac name is not appropriate
 * for the selected shim set,  a NULL (zero) is returned.
 */
extern const char *get_shimname( int index );
/*
 * This returns the index from the name of a shim.  If a shim does not
 * exist for the supplied name,  or if the shim name is not appropriate
 * for the selected shim set,  a Minus One  (-1) is returned.
 */
extern int   get_shimindex( char *shimname);
/*
 * This returns the sum of the positive values of the transverse shims
 * and the absolute value of the sum of the negative values of the
 * transverse shims
 */
extern void get_transverseShimSums(int fromPars, int *pos, int *neg);

extern int isRRI();

extern int isThinShim();

extern int mapShimset(int shimset1);
#ifdef __cplusplus
}
#endif

