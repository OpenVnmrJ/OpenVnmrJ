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

#define NEWNSR

#define NSR_READ	0x8000
#define NSR_INT_ADDR	(0x9 << 11)
#define NSR_LKA_ADDR	(0xd << 11)
#define NSR_SW12_ADDR	(0x0 << 11)
#define NSR_TM_ADDR	(0x8 << 11)


extern unsigned int NSR_words[];

