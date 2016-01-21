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

#define  MAX_NUM_CHOICES	20		/* for panel choices	*/

/*  Programs assume:
	That NOT_PRESENT is 0
	That NOT_PRESENT is the smallest value for a PANEL CHOICE ITEM
	That PRESENT is 1
	That NO_YES_SIZE is 2						*/

#define  NOT_PRESENT		0
#define  PRESENT		1
#define  NO_YES_SIZE		2		/* Size of `no_yes' table  */
#define  MAX_LABEL_LEN		40

/*  define symbol for each RF channel.  Use convention from PSG of
    numbering them 1 .. maximum number of channels.  You could also
    use TODEV, DODEV, DO2DEV if you wish... but define them first.	*/

#define  RFCHAN1		1
#define  RFCHAN2		2
#define  RFCHAN3		3
#define  RFCHAN4		4

/*  If you want to extend the CONFIG MAX RF CHAN parameter, you must
    extend parmax, parmin and parstep to accomodate added step sizes
    and upper limits for safety on coarse attenuation.  See rf_panel.c    */

#define  CONFIG_MAX_RF_CHAN	5
