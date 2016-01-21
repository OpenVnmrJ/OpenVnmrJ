/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "ap_device.p"

/* attenuator  device local properties */
c68long curattn;	/* current value for attenuation */
c68long maxattn;	/* max value of device possible */
c68long minattn;	/* min value of device possible */
c68long offsetattn;	/* value offset added to dev_value, (artifical 0 pt) */
c68long rtptr4attn;	/* for "static" setting of power, this field is      */
			/* identical to curattn.  When set using a Low Core  */
			/* (real time) Variable, it stores the offset to the */
			/* variable in Low Core. */
c68long rfband;		/* low band = 0, high band = 1. This is with  	*/
			/* the SIS am/pm rf .  The band setting is in 	*/
			/* the msb of the attenuation byte.		*/
c68int setselect;	/* selection of settings to make rfband, power  */
			/* or both for SIS Unity rf.			*/
