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
#define AUX_RELAY_REG	0
#define AUX_LOCK_REG	1
#define	AUX_USEROUT_REG	2
#define AUX_TRGATE_REG	3

#define AUX_REG_POS		8
#define AUX_READ_BIT		0x800

#define AUX_LOCK_LLF1		0x01
#define AUX_LOCK_LLF2		0x02
#define AUX_LOCK_LOCKON		0x04	// negative logic 0=closed
#define AUX_LOCK_HOLD		0x08
#define AUX_LOCK_HOMOSPOIL	0x10

