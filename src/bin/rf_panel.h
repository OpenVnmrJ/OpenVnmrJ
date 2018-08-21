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

/*  RF channel panels (1 for each channel)  */

#define  NAME_OF_CHAN	0
#define  TYPE_OF_RF	(NAME_OF_CHAN+1)
#define  TYPE_OF_SYN	(TYPE_OF_RF+1)
#define  LATCHING	(TYPE_OF_SYN+1)
#define  OVERRANGE	(LATCHING+1)
#define  STEPSIZE	(OVERRANGE+1)
#define  COARSE_ATTN	(STEPSIZE+1)
#define  UPPER_LIMIT	(COARSE_ATTN+1)
#define  FINE_ATTN	(UPPER_LIMIT+1)
#define  WAVE_FORM_GEN	(FINE_ATTN+1)
#define  TYPE_OF_AMP	(WAVE_FORM_GEN+1)
#define  RF_PANEL_MSG	(TYPE_OF_AMP+1)
#define  NUM_RF_BUTTONS (RF_PANEL_MSG+1)
