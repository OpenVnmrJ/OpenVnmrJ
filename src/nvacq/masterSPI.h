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

#define NUM_SPI		3
#define SPI_24_BITS	0x080	/* not set is 16-bit */
#define SPI_RISING_EDGE	0x100   /* not set is falling edge */
#define SPI_RESTING_HI	0x200   /* not set is resting low */

struct _SPI_config {
	int	rate;
	int	bits;
};

