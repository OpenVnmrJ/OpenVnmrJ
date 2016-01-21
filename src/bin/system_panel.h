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

/*  The system panel */

/*  Define indices for buttons in the system panel.	*/

#define  SYS_BUTTON	0
#define  CONS_BUTTON	(SYS_BUTTON+1)
#define  H1_BUTTON	(CONS_BUTTON+1)
#define  TRAY_BUTTON	(H1_BUTTON+1)
#define  PORT_BUTTON	(TRAY_BUTTON+1)
#define  SHIM_BUTTON	(PORT_BUTTON+1)
#define  NR_BUTTON	(SHIM_BUTTON+1)
#define  AF_BUTTON	(NR_BUTTON+1)
#define  VT_BUTTON	(AF_BUTTON+1)
#define  DMF_BUTTON	(VT_BUTTON+1)
#define  SW_BUTTON	(DMF_BUTTON+1)
#define  NB_BUTTON	(SW_BUTTON+1)
#define  APINF_BUTTON	(NB_BUTTON+1)
#define  FIFO_BUTTON	(APINF_BUTTON+1)
#define    FIFO_IS_63	0
#define    FIFO_IS_1024	1
#define  ROTOR_SYNC	(FIFO_BUTTON+1)
#define  PROTUNE	(ROTOR_SYNC+1) 
#define  LOCK_FREQ	(PROTUNE+1) 
#define  IFFREQ_BUTTON	(LOCK_FREQ+1) 
#define  NUM_RF_CHANS	(IFFREQ_BUTTON+1)
#define  AXIAL_GRADIENT	(NUM_RF_CHANS+1)
#define  SYSTEMP_MSG	(AXIAL_GRADIENT+1)
#define  NUM_SYSTEM	(SYSTEMP_MSG+1)
