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

#include  <stdio.h>

#include  "console_data.h"

#include  "vconfig.h"
#include  "interface.h"
#include  "system_panel.h"
#include  "rf_panel.h"
#include  "grad_panel.h"


/*  Public progams and interfaces:

        is_system_hydra
        getConfigIntParameter
        getConfigCharParameter
        getConfigDoubleParameter
        configConsole

        setConfigIntParameter
        setConfigDoubleParameter

        set_cwb_file
        is_cwb_file
        write_cwb_file			*/

#define  QUERY_CONSOLE	1
#define  HYDRA_SPECIAL	2
#define  NOT_AVAILABLE	3

/*  These defines were taken from hardware.h, SCCS category vwacq.	*/

#define DSP_PROM_NO_DSP		0
#define DSP_PROM_NO_DOWNLOAD	1
#define DSP_PROM_DOWNLOAD	2

struct _console_data_access {
	int	panel_index;
	int	item_index;
	int	support;
	int	msgForValue;
};

	
static struct _console_data_access	console_data_access[] = {
	{ SYSTEM_PANEL_INDEX,  SYS_BUTTON,	QUERY_CONSOLE,	GET_SYSTEMTYPE },
	{ SYSTEM_PANEL_INDEX,  CONS_BUTTON,	QUERY_CONSOLE,	GET_CONSOLETYPE },
	{ SYSTEM_PANEL_INDEX,  H1_BUTTON,	QUERY_CONSOLE,  GET_H1FREQ },
	{ SYSTEM_PANEL_INDEX,  TRAY_BUTTON,	NOT_AVAILABLE,	NO_MESSAGE },
	{ SYSTEM_PANEL_INDEX,  SHIM_BUTTON,	QUERY_CONSOLE,	GET_SHIMSET },
	{ SYSTEM_PANEL_INDEX,  NR_BUTTON,	HYDRA_SPECIAL,	GET_NUMRCVRS },
	{ SYSTEM_PANEL_INDEX,  AF_BUTTON,	HYDRA_SPECIAL,	GET_AUDIOFIL },
	{ SYSTEM_PANEL_INDEX,  VT_BUTTON,	QUERY_CONSOLE,	GET_VT },
	{ SYSTEM_PANEL_INDEX,  DMF_BUTTON,	HYDRA_SPECIAL,	GET_DMF },
	{ SYSTEM_PANEL_INDEX,  SW_BUTTON,	QUERY_CONSOLE,	GET_MAXSW },
	{ SYSTEM_PANEL_INDEX,  NB_BUTTON,	QUERY_CONSOLE,	GET_MAXNB },
	{ SYSTEM_PANEL_INDEX,  APINF_BUTTON,	HYDRA_SPECIAL,	GET_APIFACE },
	{ SYSTEM_PANEL_INDEX,  FIFO_BUTTON,	QUERY_CONSOLE,	GET_FIFOLPSIZE },
	{ SYSTEM_PANEL_INDEX,  ROTOR_SYNC,	NOT_AVAILABLE,	NO_MESSAGE },
	{ SYSTEM_PANEL_INDEX,  PROTUNE,		NOT_AVAILABLE,	NO_MESSAGE },
	{ SYSTEM_PANEL_INDEX,  LOCK_FREQ,	QUERY_CONSOLE,	GET_LOCKFREQ },
	{ SYSTEM_PANEL_INDEX,  IFFREQ_BUTTON,	QUERY_CONSOLE,	GET_IFFREQ },
	{ SYSTEM_PANEL_INDEX,  NUM_RF_CHANS,	QUERY_CONSOLE,	GET_NUMRFCHAN },
	{ SYSTEM_PANEL_INDEX,  AXIAL_GRADIENT,	QUERY_CONSOLE,	GET_GRAD },
	{ SYSTEM_PANEL_INDEX,  SYSTEMP_MSG,	NOT_AVAILABLE,	NO_MESSAGE },

	{ FIRST_RF_PANEL_INDEX,  NAME_OF_CHAN,	NOT_AVAILABLE,	NO_MESSAGE },
	{ FIRST_RF_PANEL_INDEX,  TYPE_OF_RF,	HYDRA_SPECIAL,	GET_RFTYPE },
	{ FIRST_RF_PANEL_INDEX,  TYPE_OF_SYN,	QUERY_CONSOLE,	GET_PTSBASE },
	{ FIRST_RF_PANEL_INDEX,  LATCHING,	HYDRA_SPECIAL,	GET_LATCHING },
	{ FIRST_RF_PANEL_INDEX,  OVERRANGE,	NOT_AVAILABLE,	NO_MESSAGE },
	{ FIRST_RF_PANEL_INDEX,  STEPSIZE,	HYDRA_SPECIAL,	GET_STEPSIZE },
	{ FIRST_RF_PANEL_INDEX,  COARSE_ATTN,	QUERY_CONSOLE,	GET_CATTNBASE },
	{ FIRST_RF_PANEL_INDEX,  UPPER_LIMIT,	NOT_AVAILABLE,	NO_MESSAGE },
	{ FIRST_RF_PANEL_INDEX,  FINE_ATTN,	HYDRA_SPECIAL,	GET_FATTNBASE },
	{ FIRST_RF_PANEL_INDEX,  WAVE_FORM_GEN, QUERY_CONSOLE,	GET_WFGBASE },
	{ FIRST_RF_PANEL_INDEX,  TYPE_OF_AMP,	QUERY_CONSOLE,	GET_AMPBASE },
	{ FIRST_RF_PANEL_INDEX,  RF_PANEL_MSG,	NOT_AVAILABLE,	NO_MESSAGE },
	{ GRAD_PANEL_INDEX,	X_GRADIENT,	QUERY_CONSOLE,	GET_GRADTYPE },
	{ GRAD_PANEL_INDEX,	Y_GRADIENT,	QUERY_CONSOLE,	GET_GRADTYPE },
	{ GRAD_PANEL_INDEX,	Z_GRADIENT,	QUERY_CONSOLE,	GET_GRADTYPE }
};

/*  No gradient panel ...  for now.	*/


/*  Explanation:

	NOT_AVAILABLE:  At this time the particular information cannot be determined.
			Examples are items that REQUIRE user input and items where
			the corresponding console data message is not implemented.

	HYDRA_SPECIAL:	The desired information is not available from the console.
			However, if the console is of the Hydra design, we know
			what the value will be.  One can argue the console data
			object should contain this information; I prefer to keep it
			here since it reflects the VNMR config program and not
			information extracted from the console.

	QUERY_CONSOLE:	The information MAY be present in the console.  A lot of
			the methods require the console be of the Hydra design.
			However several different values are possible, even if
			it is a Hydra.						    */

#define  SIZEOF_ARRAY( X )	sizeof( X ) / sizeof( X[ 0 ] )

static int
obtain_access_support( panel, item, msg_addr )
int panel;
int item;
int *msg_addr;
{
	int	iter, table_item, table_panel, table_size;

	if (panel > FIRST_RF_PANEL_INDEX && panel <= LAST_RF_PANEL_INDEX)
	  panel = FIRST_RF_PANEL_INDEX;

	table_size = SIZEOF_ARRAY( console_data_access );
	for (iter = 0; iter < table_size; iter++) {
		table_panel = console_data_access[ iter ].panel_index;
		table_item  = console_data_access[ iter ].item_index;
		if (panel == table_panel && item == table_item) {
			*msg_addr = console_data_access[ iter ].msgForValue;
			return( console_data_access[ iter ].support );
		}
	}

	return( NOT_AVAILABLE );
}

int
is_system_hydra()
{
	char			response[ MIN_RESPONSE_LEN ];
	struct _objectMsg	consoleMsg;

	consoleMsg.request = IS_SYSTEM_HYDRA;
	consoleMsg.r_addr = &response[ 0 ];
	consoleMsg.r_len = sizeof( response );
	if (getValueFromConsole( &consoleMsg ) != 0)
	  return( 0 );
	if (response[ 0 ] == '0')
	  return( 0 );
	else
	  return( 1 );
}

static int
getConfigParameter( panel, item, r_addr, r_len )
int panel;
int item;
char *r_addr;
int r_len;
{
	int			ival, type_of_support;
	struct _objectMsg	consoleMsg;

	type_of_support = obtain_access_support( panel, item, &consoleMsg.request );
	if (type_of_support == NOT_AVAILABLE)
	  return( -1 );
	else {
		consoleMsg.r_addr = r_addr;
		consoleMsg.r_len = r_len;

/*  If requesting information about an RF channel, encode
    the channel into the message for the console data object.
    Channels are numbered starting at 0.			*/

		if ( panel >= FIRST_RF_PANEL_INDEX && panel <= LAST_RF_PANEL_INDEX )
		  consoleMsg.request = MAKE_RF_CHAN_MSG( consoleMsg.request,
				panel - FIRST_RF_PANEL_INDEX + 1
		  );
		if (panel == GRAD_PANEL_INDEX){
		    if (item >= X_GRADIENT && item <= Z_GRADIENT){
			/* Encode axis number into request */
			consoleMsg.request += item - X_GRADIENT;
		    }
		}
		ival = getValueFromConsole( &consoleMsg );
		if (ival != 0)
		  return( -1 );

		return( 0 );
	}
}

int
getConfigIntParameter( panel, item, v_addr )
int panel;
int item;
int *v_addr;
{
	int	ival;
	char	response[ MIN_RESPONSE_LEN ];

	ival = getConfigParameter( panel, item, &response[ 0 ], sizeof( response ) );
	if (ival != 0)
	  return( ival );

	*v_addr = atoi( &response[ 0 ] );
	return( 0 );
}

int
getConfigCharParameter( panel, item, v_addr )
int panel;
int item;
char *v_addr;
{
	int	ival;
	char	response[ MIN_RESPONSE_LEN ];

	ival = getConfigParameter( panel, item, &response[ 0 ], sizeof( response ) );
	if (ival != 0)
	  return( ival );

	v_addr[ 0 ] = response[ 0 ];
	return( 0 );
}

int
getConfigDoubleParameter( panel, item, v_addr )
int panel;
int item;
double *v_addr;
{
	int		ival;
	char		response[ MIN_RESPONSE_LEN ];
	extern double	atof();

	ival = getConfigParameter( panel, item, &response[ 0 ], sizeof( response ) );
	if (ival != 0)
	  return( ival );

	*v_addr = atof( &response[ 0 ] );
	return( 0 );
}

int
configConsole()
{
	if (consoleDataValid() == 0) {
		return( -1 );
	}

	getSystemPanelFromConsole();
	getRFPanelsFromConsole();
	getGradPanelFromConsole();

	updateSystemPanelUsingConsole();
	updateRFPanelsUsingConsole();
	updateGradPanelUsingConsole();

	updateDspPromType();

	return( 0 );
}

/* dsptype is a character parameter whose value is set based on dspPromType:

      dspPromType == 0   ==>  "n"
      dspPromType == 1   ==>  "0"
      dspPromType == 2   ==>  "1"

   Thus if the PROM type is greater than 0, we subtract one to get a numeric
   value which is converted to its corresponding ASCII representation.

   If any problem comes up getting a value for dspPromType,
   then dsptype is set to "n".						*/

static
updateDspPromType()
{
	int	result;
	int	dspPromType;
	char	dsptype[ 4 ];

	result = getDspPromType( &dspPromType );
	if (result != 0 || dspPromType == DSP_PROM_NO_DSP) {
		strcpy( &dsptype[ 0 ], "n" );
	}

/*  if getDspPromType returns 0, it guarantees a valid value */

	else {
		dspPromType--;
		sprintf( &dsptype[ 0 ], "%d", dspPromType );
	}

	config_setstring( "dsptype", &dsptype[ 0 ], 1 );
}

int
getDspPromType( int *v_addr )
{
	char			response[ MIN_RESPONSE_LEN ];
	struct _objectMsg	consoleMsg;

	consoleMsg.request = GET_DSPPROMTYPE;
	consoleMsg.r_addr = &response[ 0 ];
	consoleMsg.r_len = sizeof( response );
	if (getValueFromConsole( &consoleMsg ) != 0)
	  return( -1 );

	*v_addr = atoi( &response[ 0 ] );
	return( 0 );
}

/*  End of program to obtain data from the console  */


/*  Programs to store data in the console.  */

/*  The 3rd field (support) has a different meaning here.
    HYDRA_ONLY  ==>  verify the system is a Hydra before proceeding.	*/

static struct _console_data_access	console_data_storage[] = {
	{ SYSTEM_PANEL_INDEX,  LOCK_FREQ,	HYDRA_ONLY,	SET_LOCKFREQ },
	{ SYSTEM_PANEL_INDEX,  H1_BUTTON,	HYDRA_ONLY,	SET_H1FREQ },
	{ SYSTEM_PANEL_INDEX,  IFFREQ_BUTTON,	HYDRA_ONLY,	SET_IFFREQ },
	{ SYSTEM_PANEL_INDEX,  SYS_BUTTON,      HYDRA_ONLY,  SET_SYSTEMTYPE },
	{ FIRST_RF_PANEL_INDEX,  TYPE_OF_SYN,	HYDRA_ONLY,	SET_PTSBASE }
};


static int
obtain_storage_support( panel, item, msg_addr )
int panel;
int item;
int *msg_addr;
{
	int	iter, table_item, table_panel, table_size;

	if (panel == GRAD_PANEL_INDEX)
	  return( NOT_AVAILABLE );

	if (panel > FIRST_RF_PANEL_INDEX && panel <= LAST_RF_PANEL_INDEX)
	  panel = FIRST_RF_PANEL_INDEX;

	table_size = SIZEOF_ARRAY( console_data_storage );
	for (iter = 0; iter < table_size; iter++) {
		table_panel = console_data_storage[ iter ].panel_index;
		table_item  = console_data_storage[ iter ].item_index;
		if (panel == table_panel && item == table_item) {
			*msg_addr = console_data_storage[ iter ].msgForValue;
			return( console_data_storage[ iter ].support );
		}
	}

	return( NOT_AVAILABLE );
}

static int
setConfigParameter( panel, item, r_addr )
int panel;
int item;
char *r_addr;
{
	int			type_of_support;
	struct _objectMsg	consoleMsg;

	type_of_support = obtain_storage_support( panel, item, &consoleMsg.request );
	if (type_of_support == NOT_AVAILABLE)
	  return( -1 );
	else if (type_of_support == HYDRA_SPECIAL) {
		if (!is_system_hydra())
		  return( -1 );
	}

	consoleMsg.r_addr = r_addr;
	consoleMsg.r_len = 0;	/* This field is not used when setting values */

/*  If storing data concerning an RF channel, encode the
    channel into the message for the console data object.
    Channels are numbered starting at 0.		*/

	if ( panel >= FIRST_RF_PANEL_INDEX && panel <= LAST_RF_PANEL_INDEX )
	  consoleMsg.request = MAKE_RF_CHAN_MSG( consoleMsg.request,
			panel - FIRST_RF_PANEL_INDEX + 1
	  );
	if (setValueInConsole( &consoleMsg ) != 0)
	  return( -1 );

	return( 0 );
}

int
setConfigIntParameter( panel, item, ival )
int panel;
int item;
int ival;
{
	int	tmpval;
	char	response[ MIN_RESPONSE_LEN ];

	sprintf( &response[ 0 ], "%d", ival );
	tmpval = setConfigParameter( panel, item, &response[ 0 ] );
	if (tmpval != 0)
	  return( tmpval );
	else
	  return( 0 );
}

int
setConfigDoubleParameter( panel, item, dval )
int panel;
int item;
double dval;
{
	int	ival, tmpval;
	char	response[ MIN_RESPONSE_LEN ];

/*  Currently the console data object only sets values which are integers.  */

	ival = (int) (dval+0.5);
	sprintf( &response[ 0 ], "%d", ival );

	tmpval = setConfigParameter( panel, item, &response[ 0 ] );
	if (tmpval != 0)
	  return( tmpval );
	else
	  return( 0 );
}

/*  End of programs to store data in the console  */


/*  console writeback file is allocated by set_cwb_file;
    remains allocated for the rest of the program.	*/

static char	*cwb_file = NULL;

int
set_cwb_file( newf )
char *newf;
{
	int		 newlen;
	char		*tmpaddr;
	extern char	*malloc();

	if (cwb_file) {
		free( cwb_file );
		cwb_file = NULL;
	}

	newlen = strlen( newf );
	if (newlen < 1)
	  return( -1 );

	tmpaddr = malloc( newlen+1 );
	if (tmpaddr == NULL)
	  return( -1 );

	strcpy( tmpaddr, newf );
	cwb_file = tmpaddr;

	return( 0 );
}

int
is_cwb_file()
{
	return( cwb_file != NULL );
}

int
write_cwb_file()
{
	FILE	*cwb_fptr;

	if (cwb_file == NULL)
	  return( -1 );

	return( writeConsoleSram( cwb_file ) );
}
