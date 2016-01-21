/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* common properities of devices */

#include "common.p"

/* Common properities of devices */
c68int  dev_state;	/* is device , present or not or inactive */
c68int  dev_cntrl;	/* ToggleOnly (On,Off), ValueOnly, ToggleValue */
c68int  dev_channel;	/* Channel device is associated with (Obs,Dec,etc.) */
