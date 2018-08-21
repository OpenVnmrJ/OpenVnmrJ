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

#include  "vnmrsys.h"			/*  VNMR include file */
#include  "group.h"			/*  VNMR include file */
#include  "variables.h"			/*  VNMR include file */

/*  See vconfig.c for an explanation of the compiler sysbols SUN and VMS.  */

#include  "vconfig.h"
#include  "interface.h"
#include  "system_panel.h"

#define  SYSTEM_PANEL_MESSAGE_LEN	60

extern char		*present_not_present[];
extern char		 no_yes[];
extern int		 cur_line_item_panel;

#define NO_YES_SIZE	2

static int		generic_choices[ NUM_SYSTEM ];
static int		lock_freq_used = 0;
static double		cur_lock_freq;

/*  Table of labels for the Panel Items.  */

static char	*generic_label[ NUM_SYSTEM ];

/*  Define table of values for `h1freq', `traymax', etc.	*/

static double	h1freq_value[] = {
	 85.0,
	100.0,
	200.0,
	300.0,
	400.0,
	500.0,
	600.0,
        700.0,
	750.0,
	800.0,
	900.0,
	127.0,
	170.0
};

static double	traymax_value[] = {
	0.0,
	9.0,
	50.0,
	100.0
};

static char	port_value[] = { 'n', 'a', 'b', 'e' };

static double	apint_value[] = {
	1.0,
	2.0,
	3.0,
	3.0
};

#define  HYDRA_AP_CHOICE	3

static double	dmf_value[] = {
	9900.0,
	32700.0,
	2e6
};

/*  Special note for VT controller:

    A value of 1 is reserved for Varian's VT controller.  Since the
    Varian model has never been offered on Sun-based spectrometers
    (VXR-5000, VXR Series S, Unity), it is left out of this table.
    The value 2 represents the Oxford VT Controller.  The user
    interface gives the choices "Not Present" and "Present".		*/

static double	vt_value[] = {
	0.0,
	2.0
};

static double	fifo_loop_size[] = {
	63.0,
	1024.0,
	2048.0
};

/*  It's more convenient to initialize this table using
    programs rather than with static definitions.	*/

static double	num_rfchans[ CONFIG_MAX_RF_CHAN ];

static double	zero_one[] = {
	0.0,
	1.0
};

/*  Define system types, as a choice in CONFIG's user interface
    and as values for the parameter `system'.  Each table must
    have the same number of non-NULL entries.  The program relies
    on the last entry in `system_choice' being NULL.		*/


#define IMAGER	3
#define SPECTROMETER_CHOICE	1

static char	*system_choice[] = {
	"Data Station",
	"Spectrometer",
	 NULL
};
static char	*system_value[] = {
	"datastation",
	"spectrometer"
};

static char	*console_choice[] = {
	"VXR-S",
	"Unity",
	"UnityPlus",
	"UnityInova",
	"Gemini 2000",
	"Mercury",
	"Imager",
	 NULL
};
static char	*console_value[] = {
	"vxrs",
	"unity",
	"uplus",
	"inova",
	"g2000",
	"mercury",
	"inova"
};

/* The system hwconfig value is a special case of the system selections */
static double sys_hwconfig_value[] = {
	0.0,
	1.0,				/* 2.0, 3.0 */
	5.0
};

static char	*shimset_choice[] = {
	"Varian 13 Shims",
	"Varian 14 Shims",
	"Varian 15 Shims",
	"Oxford 15 Shims",
	"Oxford 18 Shims",
	"Varian 18 Shims",
	"Varian 20 Shims",
	"Varian 23 Shims",
	"Varian 26 Shims",
	"Varian 28 Shims",
	"Varian 29 Shims",
	"Varian 35 Shims",
	"Varian 40 Shims",
	"Ultra 18 Shims",
	"Ultra 39 Shims",
	"Whole Body Shims",
	 NULL
};
/*  Order is chronological */
static double	shimset_value[] = {
	1.0,
	10.0,
	15.0,
	8.0,
        2.0,
        6.0,
        7.0,
	3.0,
        12.0,
        4.0,
        13.0,
        14.0,
	9.0,
        16.0,
        5.0,
	11.0
};

/*  The strings at index 2 and 3 can be modified.
    See setup_sample_changer_choices.  All strings
    for a given index should be the same length.
    If not, there is a possibility that the
    neighboring string could get overwritten.  */

static char *sample_changer_choice[] = {
	"None",
	"Carousel",
	"SMS 50 Sample    ",
	"SMS 100 Sample    ",
	"VAST",
	"NMS",
	"LC-NMR",
        "768 AS",
	 NULL
};
static double	sample_changer_value[] = {
	0.0,
	9.0,
	50.0,
	100.0,
	96.0,
	48.0,
	1.0,
        (96.0 * 8.0)
};

/*
 * Number of Receivers
 */
static char *nr_choice[] = {
    "1",
    "2",
    "4"
};
static double nr_value[] = {
    1.0,
    2.0,
    4.0
};

/*  Define Audio Filter types.  The value is a single character.
    As with `system_choice', the last entry in `filter_choice'
    must be NULL.						*/

static char	*filter_choice[] = {
	" 100kHz Elliptical ",
	" 100kHz Butterworth ",
	" 200kHz Butterworth ",
	" 500kHz Elliptical ",
	 NULL
};
static char	filter_value[] = { 'e', 'b', '2', '5' };

static char	*sw_choice[] = {
	" 100 kHz ",
	" 200 kHz ",
	" 500 kHz ",
	"  1 MHz  ",
	"  2 MHz  ",
	"  5 MHz  ",
	 NULL
};
static double	sw_value[] = {
	100000.0,
	200000.0,
	500000.0,
	1000000.0,
	2000000.0,
	5000000.0
};

static char	*nb_choice[] = {
	" 100 kHz ",
	" 200 kHz ",
	" 500 kHz ",
	 NULL
};
static double	nb_value[] = {
	100000.0,
	200000.0,
	500000.0
};

static double  iffreq_value[] = {
	10.5,
	20.0
};
static char	*iffreq_choice[] = {
	" 10.5 MHz ",
	" 20.0 MHz ",
        NULL
};

static struct param_interface	sys_iface_table[ NUM_SYSTEM ];

/************************************************************************/

/*  lock_freq_iface
    system_iface

    are Special Subroutines with their address placed in the
    interface table.  Please review the descrption in interface.h  */

/*  lock_freq_iface is doubly special because the corresponding
    entry in `generic_choices' is not used.  The user enters the
    lock frequency directly from the keyboard.  The program reads
    the value from the Graphical User Interface.  See
    `system_text_item_input'.

    Please note the user's input is obtained from the Graphical User
    Interface using lockfreq_from_obj.  This subroutine justs works
    with the static variables `lock_freq_used' and `cur_lock_freq'. */

static int
lock_freq_iface( direction )
int direction;
{
	int	active_flag, r;
	varInfo lock_freq_info;

	if (direction == UPDATE_PARAM) {
		if (lock_freq_used)
		  active_flag = ACT_ON;
		else
		  active_flag = ACT_OFF;
		r = P_setactive( SYSTEMGLOBAL, "lockfreq", active_flag );
		if (lock_freq_used == 0)
		  return( 0 );

		r = P_setreal( SYSTEMGLOBAL, "lockfreq", cur_lock_freq, 1 );
		if (r == 0)
		  return( 0 );
		else
		  return( -1 );
	}
	else if (direction == SET_CHOICE) {
		lock_freq_used = 0;
		r = P_getVarInfo( SYSTEMGLOBAL, "lockfreq", &lock_freq_info );
		if (r != 0) {
			r = P_creatvar( SYSTEMGLOBAL, "lockfreq", T_REAL );
			if (r != 0)
			  return( -1 );
			else
			  return( 0 );
		}
		else if (lock_freq_info.active == ACT_OFF)
		  return( 0 );				/* Not used */

		r = P_getreal( SYSTEMGLOBAL, "lockfreq", &cur_lock_freq, 1 );
		if (r != 0)
		  return( -1 );
		lock_freq_used = 1;
	}
	else
	  return( -1 );
}

static int
system_iface( direction )
int direction;
{
	int	iter, r, system_choice, table_limit;
	char	system_buf[ 24 ];

	if (direction == SET_CHOICE) {
		r = P_getstring(
				 SYSTEMGLOBAL,
				"system",
				&system_buf[ 0 ],
				 1,
				 sizeof( system_buf )
		);
		if (r != 0)
		  system_choice = 0;
		else {
			table_limit = sizeof( system_value ) / sizeof( char * );
			system_choice = -1;		/* which means unknown */
			for (iter = 0; iter < table_limit; iter++)
			  if (strcmp( &system_buf[ 0 ], system_value[ iter ] ) == 0) {
				system_choice = iter;
				break;
			  }
		}

		return( system_choice );
	}
	else if (direction == UPDATE_PARAM) {
		table_limit = sizeof( system_value ) / sizeof( char * );
		system_choice = generic_choices[ SYS_BUTTON ];
		if (system_choice < 0)
		  system_choice = 0;
		else if (system_choice >= table_limit)
		  system_choice = table_limit-1;
		
		r = config_setstring( "system", system_value[ system_choice ], 1 );
		return( r );
	}
	else
	  return( -1 );
}

static int
console_iface( direction )
int direction;
{
	int	iter, r, console_choice, table_limit;
	char	console_buf[ 12 ];
	static	setup_sample_changer_choices();

	if (direction == SET_CHOICE) {
		r = P_getstring(
				 SYSTEMGLOBAL,
				"Console",
				&console_buf[ 0 ],
				 1,
				 sizeof( console_buf )
		);
		if (r != 0)
		  console_choice = 0;
		else {
			table_limit = sizeof( console_value ) / sizeof( char * );
			console_choice = -1;		/* which means unknown */
			for (iter = 0; iter < table_limit; iter++)
			  if (strcmp( &console_buf[ 0 ], console_value[ iter ] ) == 0) {
				console_choice = iter;
				break;
			  }
		}

		if (console_choice > -1)
		  setup_sample_changer_choices( console_value[ console_choice ] );

		return( console_choice );
	}
	else if (direction == UPDATE_PARAM) {
		table_limit = sizeof( console_value ) / sizeof( char * );
		console_choice = generic_choices[ CONS_BUTTON ];
		if (console_choice < 0)
		  console_choice = 0;
		else if (console_choice >= table_limit)
		  console_choice = table_limit-1;
		
		r = config_setstring("Console", console_value[console_choice],
                     1 );
		return( r );
	}
	else
	  return( -1 );
}

#define  DMF_INDEX	11		/* index into "parmax" */
#define  SW_INDEX	5		/* index into "parmax" */

/*  Special note on table sizes.  Only tables defined in this source
    file can have the size computed by software.  External tables
    (for example, `no_yes') must have the size entered by hand.		*/

static int
init_interface_table()
{
	extern int	gradients_present();		/* in grad_panel.c */

	sys_iface_table[ SYS_BUTTON ].type_of_entry = SPECIAL_ENTRY;
	sys_iface_table[ SYS_BUTTON ].a.ctable_addr = NULL;
	sys_iface_table[ SYS_BUTTON ].b.special_sub = system_iface;
	sys_iface_table[ SYS_BUTTON ].table_size    = 0;

	sys_iface_table[ CONS_BUTTON ].type_of_entry = SPECIAL_ENTRY;
	sys_iface_table[ CONS_BUTTON ].a.ctable_addr = NULL;
	sys_iface_table[ CONS_BUTTON ].b.special_sub = console_iface;
	sys_iface_table[ CONS_BUTTON ].table_size    = 0;

	sys_iface_table[ H1_BUTTON ].type_of_entry  = TABLE_OF_DOUBLE;
	sys_iface_table[ H1_BUTTON ].a.dtable_addr  = &h1freq_value[ 0 ];
	sys_iface_table[ H1_BUTTON ].b.param_name   = "h1freq";
	sys_iface_table[ H1_BUTTON ].table_size     =
					sizeof( h1freq_value ) / DSIZE;
	sys_iface_table[ H1_BUTTON ].param_index    = SCALER;
	sys_iface_table[ H1_BUTTON ].default_choice = 0;

	sys_iface_table[ TRAY_BUTTON ].type_of_entry  = TABLE_OF_DOUBLE;
	sys_iface_table[ TRAY_BUTTON ].a.dtable_addr  = &sample_changer_value[ 0 ];
	sys_iface_table[ TRAY_BUTTON ].b.param_name   = "traymax";
	sys_iface_table[ TRAY_BUTTON ].table_size     =
					sizeof( sample_changer_value ) / DSIZE;
	sys_iface_table[ TRAY_BUTTON ].param_index    = SCALER;
	sys_iface_table[ TRAY_BUTTON ].default_choice = 0;

	sys_iface_table[ PORT_BUTTON ].type_of_entry  = TABLE_OF_CHAR;
	sys_iface_table[ PORT_BUTTON ].a.ctable_addr  = &port_value[ 0 ];
	sys_iface_table[ PORT_BUTTON ].b.param_name   = "smsport";
	sys_iface_table[ PORT_BUTTON ].table_size     = sizeof( port_value );
	sys_iface_table[ PORT_BUTTON ].param_index    = SCALER;
	sys_iface_table[ PORT_BUTTON ].default_choice = 0;

	sys_iface_table[ SHIM_BUTTON ].type_of_entry  = TABLE_OF_DOUBLE;
	sys_iface_table[ SHIM_BUTTON ].a.dtable_addr  = &shimset_value[ 0 ];
	sys_iface_table[ SHIM_BUTTON ].b.param_name   = "shimset";
	sys_iface_table[ SHIM_BUTTON ].table_size     =
					sizeof( shimset_value ) / DSIZE;
	sys_iface_table[ SHIM_BUTTON ].param_index    = SCALER;
	sys_iface_table[ SHIM_BUTTON ].default_choice = 0;

	sys_iface_table[ NR_BUTTON ].type_of_entry  = TABLE_OF_DOUBLE;
	sys_iface_table[ NR_BUTTON ].a.dtable_addr  = &nr_value[ 0 ];
	sys_iface_table[ NR_BUTTON ].b.param_name   = "numrcvrs";
	sys_iface_table[ NR_BUTTON ].table_size     = sizeof(nr_value) / DSIZE;
	sys_iface_table[ NR_BUTTON ].param_index    = SCALER;
	sys_iface_table[ NR_BUTTON ].default_choice = 0;

	sys_iface_table[ AF_BUTTON ].type_of_entry  = TABLE_OF_CHAR;
	sys_iface_table[ AF_BUTTON ].a.ctable_addr  = &filter_value[ 0 ];
	sys_iface_table[ AF_BUTTON ].b.param_name   = "audiofilter";
	sys_iface_table[ AF_BUTTON ].table_size     = sizeof( filter_value );
	sys_iface_table[ AF_BUTTON ].param_index    = SCALER;
	sys_iface_table[ AF_BUTTON ].default_choice = 0;

	sys_iface_table[ VT_BUTTON ].type_of_entry  = TABLE_OF_DOUBLE;
	sys_iface_table[ VT_BUTTON ].a.dtable_addr  = &vt_value[ 0 ];
	sys_iface_table[ VT_BUTTON ].b.param_name   = "vttype";
	sys_iface_table[ VT_BUTTON ].table_size     = sizeof( vt_value ) / DSIZE;
	sys_iface_table[ VT_BUTTON ].param_index    = SCALER;
	sys_iface_table[ VT_BUTTON ].default_choice = 0;

	sys_iface_table[ DMF_BUTTON ].type_of_entry  = TABLE_OF_DOUBLE;
	sys_iface_table[ DMF_BUTTON ].a.dtable_addr  = &dmf_value[ 0 ];
	sys_iface_table[ DMF_BUTTON ].b.param_name   = "parmax";
	sys_iface_table[ DMF_BUTTON ].table_size     = sizeof( dmf_value ) / DSIZE;
	sys_iface_table[ DMF_BUTTON ].param_index    = DMF_INDEX;
	sys_iface_table[ DMF_BUTTON ].default_choice = 0;

	sys_iface_table[ SW_BUTTON ].type_of_entry  = TABLE_OF_DOUBLE;
	sys_iface_table[ SW_BUTTON ].a.dtable_addr  = &sw_value[ 0 ];
	sys_iface_table[ SW_BUTTON ].b.param_name   = "parmax";
	sys_iface_table[ SW_BUTTON ].table_size     = sizeof( sw_value ) / DSIZE;
	sys_iface_table[ SW_BUTTON ].param_index    = SW_INDEX;
	sys_iface_table[ SW_BUTTON ].default_choice = 0;

	sys_iface_table[ NB_BUTTON ].type_of_entry  = TABLE_OF_DOUBLE;
	sys_iface_table[ NB_BUTTON ].a.dtable_addr  = &nb_value[ 0 ];
	sys_iface_table[ NB_BUTTON ].b.param_name   = "maxsw_loband";
	sys_iface_table[ NB_BUTTON ].table_size     = sizeof( nb_value ) / DSIZE;
	sys_iface_table[ NB_BUTTON ].param_index    = SCALER;
	sys_iface_table[ NB_BUTTON ].default_choice = 0;

	sys_iface_table[ APINF_BUTTON ].type_of_entry  = TABLE_OF_DOUBLE;
	sys_iface_table[ APINF_BUTTON ].a.dtable_addr  = &apint_value[ 0 ];
	sys_iface_table[ APINF_BUTTON ].b.param_name   = "apinterface";
	sys_iface_table[ APINF_BUTTON ].table_size     =
					 sizeof( apint_value ) / DSIZE;
	sys_iface_table[ APINF_BUTTON ].param_index    = SCALER;
	sys_iface_table[ APINF_BUTTON ].default_choice = 0;

	sys_iface_table[ FIFO_BUTTON ].type_of_entry  = TABLE_OF_DOUBLE;
	sys_iface_table[ FIFO_BUTTON ].a.dtable_addr  = &fifo_loop_size[ 0 ];
	sys_iface_table[ FIFO_BUTTON ].b.param_name   = "fifolpsize";
	sys_iface_table[ FIFO_BUTTON ].table_size     =
					sizeof( fifo_loop_size ) / DSIZE;
	sys_iface_table[ FIFO_BUTTON ].param_index    = SCALER;
	sys_iface_table[ FIFO_BUTTON ].default_choice = 0;

	sys_iface_table[ ROTOR_SYNC ].type_of_entry  = TABLE_OF_DOUBLE;
	sys_iface_table[ ROTOR_SYNC ].a.dtable_addr  = &zero_one[ 0 ];
	sys_iface_table[ ROTOR_SYNC ].b.param_name   = "rotorsync";
	sys_iface_table[ ROTOR_SYNC ].table_size     = sizeof( zero_one ) / DSIZE;
	sys_iface_table[ ROTOR_SYNC ].param_index    = SCALER;
	sys_iface_table[ ROTOR_SYNC ].default_choice = 0;

	sys_iface_table[ PROTUNE ].type_of_entry  = TABLE_OF_CHAR;
	sys_iface_table[ PROTUNE ].a.ctable_addr  = &no_yes[ 0 ];
	sys_iface_table[ PROTUNE ].b.param_name   = "atune";
	sys_iface_table[ PROTUNE ].table_size     = NO_YES_SIZE;
	sys_iface_table[ PROTUNE ].param_index    = SCALER;
	sys_iface_table[ PROTUNE ].default_choice = 0;

	sys_iface_table[ LOCK_FREQ ].type_of_entry = SPECIAL_ENTRY;
	sys_iface_table[ LOCK_FREQ ].a.ctable_addr = NULL;
	sys_iface_table[ LOCK_FREQ ].b.special_sub = lock_freq_iface;
	sys_iface_table[ LOCK_FREQ ].table_size    = 0;

	sys_iface_table[ IFFREQ_BUTTON ].type_of_entry = TABLE_OF_DOUBLE;
	sys_iface_table[ IFFREQ_BUTTON ].a.dtable_addr = &iffreq_value[ 0 ];
	sys_iface_table[ IFFREQ_BUTTON ].b.param_name  = "iffreq";
	sys_iface_table[ IFFREQ_BUTTON ].table_size    =
					 sizeof( iffreq_value ) / DSIZE;
	sys_iface_table[ IFFREQ_BUTTON ].param_index   = SCALER;
	sys_iface_table[ IFFREQ_BUTTON ].default_choice = 0;	/* 10.5 MHz */

	sys_iface_table[ NUM_RF_CHANS ].type_of_entry  = TABLE_OF_DOUBLE;
	sys_iface_table[ NUM_RF_CHANS ].a.dtable_addr  = &num_rfchans[ 0 ];
	sys_iface_table[ NUM_RF_CHANS ].b.param_name   = "numrfch";
	sys_iface_table[ NUM_RF_CHANS ].table_size     = CONFIG_MAX_RF_CHAN;
	table_of_double_from_limit( &num_rfchans[ 0 ], 1.0, CONFIG_MAX_RF_CHAN );
	sys_iface_table[ NUM_RF_CHANS ].param_index    = SCALER;
	sys_iface_table[ NUM_RF_CHANS ].default_choice = 0;

	sys_iface_table[ AXIAL_GRADIENT ].type_of_entry  = SPECIAL_ENTRY;
	sys_iface_table[ AXIAL_GRADIENT ].a.ctable_addr  = NULL;
	sys_iface_table[ AXIAL_GRADIENT ].b.special_sub  = gradients_present;
	sys_iface_table[ AXIAL_GRADIENT ].table_size     = 0;

/*  No initialization for the System Panel Message entry at the bottom (for now)  */

}

static int
init_generic_labels()
{
	generic_label[ SYS_BUTTON ] = "System Type";
	generic_label[ CONS_BUTTON ] = "Console Type";
	generic_label[ H1_BUTTON ] = "Proton Frequency";
	generic_label[ TRAY_BUTTON ] = "Sample Changer";
	generic_label[ PORT_BUTTON ] = "Sample Changer Comm Port";
	generic_label[ SHIM_BUTTON ] = "Shim Set";
	generic_label[ NR_BUTTON ] = "Number of Receivers";
	generic_label[ AF_BUTTON ] = "Audio Filter Type";
	generic_label[ VT_BUTTON ] = "VT Controller";
	generic_label[ DMF_BUTTON ] = "Maximum DMF";
	generic_label[ SW_BUTTON ] = "Max Spectral Width";
	generic_label[ NB_BUTTON ] = "Max Narrowband Width";
	generic_label[ APINF_BUTTON ] = "AP Interface Type";
	generic_label[ FIFO_BUTTON ] = "Fifo Loop Size";
	generic_label[ ROTOR_SYNC ] = "Rotor Synchronization";
	generic_label[ PROTUNE ] = "Protune";
	generic_label[ LOCK_FREQ ] = "Lock Frequency";
	generic_label[ IFFREQ_BUTTON ] = "IF Frequency";
	generic_label[ NUM_RF_CHANS ] = "Number of RF Channels";
	generic_label[ AXIAL_GRADIENT ] = "Gradients";
}

int
init_generic_choices()
{
	int	iter, p_index, r;
	char	tempbuf[ 22 ];
	double	dval;
	int	(*routine_addr)();

	init_interface_table();
	init_generic_labels();
	init_console_table();
	for (iter = 0; iter < NUM_SYSTEM; iter++)
	  switch( sys_iface_table[ iter ].type_of_entry ) {

		case INDEX_ENTRY:
			r = P_getreal(
				 SYSTEMGLOBAL,
				 sys_iface_table[ iter ].b.param_name,
				&dval,
				 1
			);
			if (r != 0) {
				generic_choices[ iter ] =
					sys_iface_table[ iter ].default_choice;
				break;
			}

			generic_choices[ iter ] = find_index_entry(
				dval,
				sys_iface_table[ iter ].a.limit_of_index
			);
			break;

		case TABLE_OF_CHAR:
			p_index = (sys_iface_table[ iter ].param_index == SCALER) ?
				1 : sys_iface_table[ iter ].param_index;
			r = P_getstring(
				 SYSTEMGLOBAL,
				 sys_iface_table[ iter ].b.param_name,
				&tempbuf[ 0 ],
				 p_index,
				 20
			);
			if (r != 0) {
				generic_choices[ iter ] =
					sys_iface_table[ iter ].default_choice;
				break;
			}

			generic_choices[ iter ] = find_table_char(
				tempbuf[ 0 ],
				sys_iface_table[ iter ].a.ctable_addr,
				sys_iface_table[ iter ].table_size
			);
			break;

		case TABLE_OF_DOUBLE:
			p_index = (sys_iface_table[ iter ].param_index == SCALER) ?
				1 : sys_iface_table[ iter ].param_index;
			r = P_getreal(
				 SYSTEMGLOBAL,
				 sys_iface_table[ iter ].b.param_name,
				&dval,
				 p_index
			);
			if (r != 0) {
				generic_choices[ iter ] =
					sys_iface_table[ iter ].default_choice;
				break;
			}

			generic_choices[ iter ] = find_table_double(
				dval,
				sys_iface_table[ iter ].a.dtable_addr,
				sys_iface_table[ iter ].table_size
			);
			break;

		case SPECIAL_ENTRY:
			routine_addr = sys_iface_table[ iter ].b.special_sub;
			if (routine_addr != NULL)
			  generic_choices[ iter ] = (*routine_addr)( SET_CHOICE );
			else
			  generic_choices[ iter ] = 0;
			break;

		default:
			generic_choices[ iter ] = 0;
			break;
	}
}


#ifdef SUN

/*  If the value for a Generic Choice parameter is not supported, the
    corresponding position in the `generic_choice' array will contain -1.
    This is OK if CONFIG is just displaying current values, but is not
    OK if CONFIG is running in interactive mode.  Thus this routine to
    fix the generic choices, called by Make System Panel.		*/

static int
fix_generic_choices()
{
	int	iter, size_limit, this_choice;

	for (iter = 0; iter < NUM_SYSTEM; iter++) {
		this_choice = generic_choices[ iter ];

	/*  Regardless of the type of interface entry,
	    all choices must be 0 or larger...		*/

		if (this_choice < 0)
		  this_choice = 0;

		switch( sys_iface_table[ iter ].type_of_entry ) {
			case INDEX_ENTRY:
				size_limit =
				    sys_iface_table[ iter ].a.limit_of_index - 1;
				break;

			case TABLE_OF_CHAR:
			case TABLE_OF_DOUBLE:
				size_limit = sys_iface_table[ iter ].table_size;
				break;

			default:
				size_limit = 0;    /* which means not defined */
		}

		if (size_limit > 0 && this_choice > size_limit)
		  this_choice = size_limit;

		generic_choices[ iter ] = this_choice;
	}
}
#endif

#ifndef VMS

void
update_instrument_name()
{
	char i_name[ 128 ];
	if (gethostname(i_name, sizeof(i_name)) != 0)
	  printf("Could not get hostname.\n");
	else if (config_setstring( "instrument", i_name, 1 ) != 0)
	  printf("Could not set instrument name into conpar.\n");
}

/*  Call this subroutine to update the parameters based
    on the choices made in the generic config panel.	*/

int
update_generic_conpar()
{
	int	iter, p_index, r, this_choice;
	char	tempbuf[ 2 ];
	double	dval;
	int	(*routine_addr)();

	update_instrument_name();
	for (iter = 0; iter < NUM_SYSTEM; iter++)
	  switch( sys_iface_table[ iter ].type_of_entry ) {

		case INDEX_ENTRY:
			dval = (double) (generic_choices[ iter ] + 1);
			r = config_setreal(
				 sys_iface_table[ iter ].b.param_name,
				 dval,
				 1
			);
			break;

		case TABLE_OF_CHAR:
			p_index = (sys_iface_table[ iter ].param_index == SCALER) ?
				1 : sys_iface_table[ iter ].param_index;
			this_choice = generic_choices[ iter ];

		/*  Following statements insure the limits of
		    the Character table are not exceeded.	*/

			if (this_choice < 0)
			 this_choice = 0;
			else if (this_choice >= sys_iface_table[ iter ].table_size)
			 this_choice = sys_iface_table[ iter ].table_size - 1;

			tempbuf[ 0 ] =
			    sys_iface_table[ iter ].a.ctable_addr[ this_choice ];
			tempbuf[ 1 ] = '\0';
			r = config_setstring(
				 sys_iface_table[ iter ].b.param_name,
				&tempbuf[ 0 ],
				 p_index
			);
			break;

		case TABLE_OF_DOUBLE:
			p_index = (sys_iface_table[ iter ].param_index == SCALER) ?
				1 : sys_iface_table[ iter ].param_index;
			this_choice = generic_choices[ iter ];

		/*  Following statements insure the limits of
		    the Double Precision table are not exceeded.	*/

			if (this_choice < 0)
			 this_choice = 0;
			else if (this_choice >= sys_iface_table[ iter ].table_size)
			 this_choice = sys_iface_table[ iter ].table_size - 1;

			dval = sys_iface_table[ iter ].a.dtable_addr[ this_choice ];
			r = config_setreal(
				 sys_iface_table[ iter ].b.param_name,
				 dval,
				 p_index
			);
			break;

		case SPECIAL_ENTRY:
			routine_addr = sys_iface_table[ iter ].b.special_sub;
			if (routine_addr != NULL)
			  r = (*routine_addr)( UPDATE_PARAM );
			break;

		default:
			break;
	}
}

int
set_lockfreq_inactive()
{
	lock_freq_used = 0;
}

static int
lockfreq_from_text( lf_text )
char *lf_text;
{
	int		 tflag;
	extern double	 atof();

/*  Program returns -1 if something wrong:
      lockfreq text is not a real number or the character 'n'
    Else it returns 0 and
      sets lockfreq used and current lockfreq
      lockfreq used is non-zero if and onlyif the lockfreq text is a real number.  */

	tflag = strlen( lf_text ) <= 0;
	if (tflag == 0)				/* that is, string length > 0 */
	  tflag = strcmp( lf_text, "n" ) == 0;
	if (tflag) {				/* that is, 0-length string or "n"  */
		lock_freq_used = 0;
		cur_lock_freq = 0.0;
	}
	else {
		if (isReal( lf_text ) == 0)
		  return( -1 );
		lock_freq_used = 1;
		cur_lock_freq = atof( lf_text );
	}

	return( 0 );
}

int
lockfreq_from_obj()
{
	int	 tflag;
	char	*lf_text;

	tflag = get_text_from_panel_text_input( SYSTEM_PANEL_INDEX, LOCK_FREQ, &lf_text );
	if (tflag != 0)
	  return( -1 );
	else
	  return( lockfreq_from_text( lf_text ) );
}
#endif

/*  This function allows the Axial Gradient choice to be exported Read-Only.
    An external function still can't write to the array of Generic Choices;
    nor can it examine an arbitrary Generic Choice.			    */

int
gradients_selected()
{
	return( generic_choices[ AXIAL_GRADIENT ] );
}

/*  This function returns the number of RF channels, as selected by the user.	*/

int
num_rf_chans_selected()
{
	return( generic_choices[ NUM_RF_CHANS ] + 1 );
}


#ifdef SUN
static
special_update_choice( panel_item, new_choice )
int panel_item;
int new_choice;
{
	if (panel_item == NUM_RF_CHANS)
		fix_num_RF_chans( new_choice, generic_choices[ NUM_RF_CHANS ] );
	else if (panel_item == AXIAL_GRADIENT)
		fix_gradient_presence( new_choice, generic_choices[ AXIAL_GRADIENT ] );
}

static
setup_sample_changer_choices( type_of_console )
char *type_of_console;
{
	static	system_notify_choice_proc();

/*  For each index keep the sample changer choice strings the same length  */

	if (strcmp( type_of_console, "inova" ) == 0 ||
	    strcmp( type_of_console, "mercury" ) == 0) {
		strcpy( sample_changer_choice[ 2 ], "SMS 50 Sample    " );
		strcpy( sample_changer_choice[ 3 ], "SMS 100 Sample    " );
	}
	else {
		strcpy( sample_changer_choice[ 2 ], "SMS/ASM 50 Sample" );
		strcpy( sample_changer_choice[ 3 ], "SMS/ASM 100 Sample" );
	}

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 TRAY_BUTTON,
		 generic_label[ TRAY_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ TRAY_BUTTON ],
		&sample_changer_choice[ 0 ]
	);
}


static
system_notify_choice_proc( panel, button, selection )
int panel;
int button;
int selection;
{
int i, j, r;
	switch (button) {
	  case SYS_BUTTON:
	  case H1_BUTTON:
	  case SHIM_BUTTON:
	  case NR_BUTTON:
	  case AF_BUTTON:
	  case VT_BUTTON:
	  case DMF_BUTTON:
	  case SW_BUTTON:
	  case NB_BUTTON:
	  case APINF_BUTTON:
	  case ROTOR_SYNC:
	  case PROTUNE:
	  case FIFO_BUTTON:
	  case IFFREQ_BUTTON:
		generic_choices[ button ] = selection;
		break;

	  case TRAY_BUTTON:
		if (!strcmp(sample_changer_choice[ selection ], "768 AS")) {
			char *ptr;

			/* 768 AS (Hermes) Sample Changer Selected */
			j = sizeof(port_value) / sizeof(char);
			ptr = port_value;
			for (i=0; i < j; i++, ptr++) {
				if (port_value[i] == 'e') {
					generic_choices[ PORT_BUTTON ] = i;
					set_choice_in_panel_choice(
					  SYSTEM_PANEL_INDEX, PORT_BUTTON,
					  generic_choices[ PORT_BUTTON ]);
					break;
				}
			}

		}
		else {
			if (!strcmp(sample_changer_choice[
		                generic_choices[TRAY_BUTTON]],"768 AS")) {

				/* 768 AS Sample Changer De-selected */
				generic_choices[ PORT_BUTTON ] =
				  sys_iface_table[PORT_BUTTON].default_choice;
				set_choice_in_panel_choice(
				  SYSTEM_PANEL_INDEX, PORT_BUTTON,
				  generic_choices[ PORT_BUTTON ]);
			}
		}
                /* If available, update traymax limits */
                r = P_setlimits(SYSTEMGLOBAL, "traymax", 768.0, 1.0, 0.0);
                if (r != 0)
                   printf("Unable to update traymax to 768.");

		generic_choices[ button ] = selection;
		break;

	  case PORT_BUTTON:
		if (port_value[ selection ] == 'e') {
			int i, j;
			char **ptr;

			/* Only Allow Ethernet Comm Port if 768 AS Selected */
			if (!strcmp(sample_changer_choice[
			            generic_choices[TRAY_BUTTON]],"768 AS")) {
				generic_choices[ button ] = selection;
			}
			else {
				printf("Ethernet only available on 768 AS "
				       "sample changer\r\n");
				set_choice_in_panel_choice(
				  SYSTEM_PANEL_INDEX, PORT_BUTTON,
				  generic_choices[ PORT_BUTTON ]);
			}
		}
		else {
			if (port_value[generic_choices[PORT_BUTTON]] == 'e') {

				/* Ethernet Comm Port De-selected */
				generic_choices[ TRAY_BUTTON ] =
				  sys_iface_table[TRAY_BUTTON].default_choice;
				set_choice_in_panel_choice(
				  SYSTEM_PANEL_INDEX, TRAY_BUTTON,
				  generic_choices[ TRAY_BUTTON ]);
			}
			generic_choices[ button ] = selection;
		}
 
		break;

/*  You must fix the number of RF channels (or the presence of
    gradients) before placing the new value in the generic choices
    array; otherwise, the fix number of RF channels routine will
    think nothing needs to be done!				*/

	  case NUM_RF_CHANS:
	  case AXIAL_GRADIENT:
		special_update_choice( button, selection );
		generic_choices[ button ] = selection;
		break;

	  case CONS_BUTTON:
		generic_choices[ button ] = selection;
		setup_sample_changer_choices( console_value[ selection ] );
		break;

	  default:
		printf("system notify choice proc called with unknown item");
		break;
	}
}

static int
system_notify_text_proc( panel, button, text )
int panel;
int button;
char *text;
{
	int	ival;

	if (button != LOCK_FREQ)
	  return( -1 );

	set_message_in_panel( SYSTEM_PANEL_INDEX, SYSTEMP_MSG, "" );
	ival = lockfreq_from_text( text );
	if (ival != 0) {
		set_message_in_panel( SYSTEM_PANEL_INDEX, SYSTEMP_MSG,
	    "Please enter a numeric value or 'n' for the lock frequency"
		);
		config_bell();
		set_text_in_panel_text_input( panel, button, "n" );
		lock_freq_used = 0;
	}

	return( ival );
}


makeSystemPanel()
{
	int	 ival;
	int	 cur_gradient_type, cur_num_rf_chans;
	char	 quick_str[ SYSTEM_PANEL_MESSAGE_LEN ];
	char	*menu_choices[ MAX_NUM_CHOICES+1 ];
	char	 type_of_console[ MAXPATHL ];

	fix_generic_choices();

/*  Before the GUI stuff was abstracted and separated, the program
    finagled some choices to maximize the window width.  This does
    not appear to be needed so it was taken out.			*/

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,			/* select a panel */
		 SYS_BUTTON,				/* button index */
		 generic_label[ SYS_BUTTON ],		/* buttton label */
		(char *) system_notify_choice_proc,
		 generic_choices[ SYS_BUTTON ],		/* initial value (index) */
		&system_choice[ 0 ]
	);

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,			/* select a panel */
		 CONS_BUTTON,				/* button index */
		 generic_label[ CONS_BUTTON ],		/* buttton label */
		(char *) system_notify_choice_proc,
		 generic_choices[ CONS_BUTTON ],	/* initial value (index) */
		&console_choice[ 0 ]
	);

	menu_choices[ 0 ] = "  85 ";
	menu_choices[ 1 ] = " 100 ";
	menu_choices[ 2 ] = " 200 ";
	menu_choices[ 3 ] = " 300 "; 
	menu_choices[ 4 ] = " 400 "; 
	menu_choices[ 5 ] = " 500 "; 
	menu_choices[ 6 ] = " 600 "; 
	menu_choices[ 7 ] = " 700 "; 
	menu_choices[ 8 ] = " 750 ";
	menu_choices[ 9 ] = " 800 ";
	menu_choices[ 10 ] = " 900 ";
	menu_choices[ 11 ] = "  3T ";
	menu_choices[ 12 ] = "  4T ";
	menu_choices[ 13 ] = NULL;
 	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 H1_BUTTON,
		 generic_label[ H1_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ H1_BUTTON ],
		&menu_choices[ 0 ]
	);

	P_getstring(
		 SYSTEMGLOBAL,
		"Console",
		&type_of_console[ 0 ],
		 1,
		 sizeof( type_of_console ) - 1
	);
	setup_sample_changer_choices( &type_of_console[ 0 ] );

	/*config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 TRAY_BUTTON,
		 generic_label[ TRAY_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ TRAY_BUTTON ],
		&sample_changer_choice[ 0 ]
	);*/

	menu_choices[ 0 ] = " Not Used";
	menu_choices[ 1 ] = " Port A";
	menu_choices[ 2 ] = " Port B";
        menu_choices[ 3 ] = " Ethernet";
        menu_choices[ 4 ] = NULL;

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 PORT_BUTTON,
		 generic_label[ PORT_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ PORT_BUTTON ],
		&menu_choices[ 0 ]
	);

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 SHIM_BUTTON,
		 generic_label[ SHIM_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ SHIM_BUTTON ],
		&shimset_choice[ 0 ]
	);

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 NR_BUTTON,
		 generic_label[ NR_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ NR_BUTTON ],
		&nr_choice[ 0 ]
	);

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 AF_BUTTON,
		 generic_label[ AF_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ AF_BUTTON ],
		&filter_choice[ 0 ]
	);

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 VT_BUTTON,
		 generic_label[ VT_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ VT_BUTTON ],
		&present_not_present[ 0 ]
	);

	menu_choices[ 0 ] = " 9900  ";
	menu_choices[ 1 ] = " 32700 ";
	menu_choices[ 2 ] = " 2.0e6 ";
	menu_choices[ 3 ] =  NULL;
	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 DMF_BUTTON,
		 generic_label[ DMF_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ DMF_BUTTON ],
		&menu_choices[ 0 ]
	);

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 SW_BUTTON,
		 generic_label[ SW_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ SW_BUTTON ],
		&sw_choice[ 0 ]
	);

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 NB_BUTTON,
		 generic_label[ NB_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ NB_BUTTON ],
		&nb_choice[ 0 ]
	);

	menu_choices[ 0 ] = " Type 1";
	menu_choices[ 1 ] = " Type 2";
	menu_choices[ 2 ] = " Type 3";
	menu_choices[ 3 ] = "  N/A  ";
	menu_choices[ 4 ] =  NULL;
	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 APINF_BUTTON,
		 generic_label[ APINF_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ APINF_BUTTON ],
		&menu_choices[ 0 ]
	);

	menu_choices[ 0 ] = " 63   ";
	menu_choices[ 1 ] = " 1024 ";
	menu_choices[ 2 ] = " 2048 ";
	menu_choices[ 3 ] =  NULL;
	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 FIFO_BUTTON,
		 generic_label[ FIFO_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ FIFO_BUTTON ],
		&menu_choices[ 0 ]
	);

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 ROTOR_SYNC,
		 generic_label[ ROTOR_SYNC ],
		(char *) system_notify_choice_proc,
		 generic_choices[ ROTOR_SYNC ],
		&present_not_present[ 0 ]
	);

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 PROTUNE,
		 generic_label[ PROTUNE ],
		(char *) system_notify_choice_proc,
		 generic_choices[ PROTUNE ],
		&present_not_present[ 0 ]
	);

	config_panel_text_input_item(
		 SYSTEM_PANEL_INDEX,
		 LOCK_FREQ,
		 generic_label[ LOCK_FREQ ],
		(char *) system_notify_text_proc
	);

	if (lock_freq_used) {
		sprintf( &quick_str[ 0 ], "%.12g", cur_lock_freq );
		set_text_in_panel_text_input(
			 SYSTEM_PANEL_INDEX,
			 LOCK_FREQ, 
			&quick_str[ 0 ]
		);
	}

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 IFFREQ_BUTTON,
		 generic_label[ IFFREQ_BUTTON ],
		(char *) system_notify_choice_proc,
		 generic_choices[ IFFREQ_BUTTON ],
		&iffreq_choice[ 0 ]
	);

	ival = panel_choices_from_limit_of_index(
		&menu_choices[ 0 ], " %d ", CONFIG_MAX_RF_CHAN
	);
	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 NUM_RF_CHANS,
		 generic_label[ NUM_RF_CHANS ],
		(char *) system_notify_choice_proc,
		 generic_choices[ NUM_RF_CHANS ],
		&menu_choices[ 0 ]
	);
	free_table_of_memory( &menu_choices[ 0 ] );

/*  Be aware that `fix_gradient_panel' and `makeGradientPanel'
    assume that index 0 corresponds to None or Not Present.	*/

	config_panel_choice_item(
		 SYSTEM_PANEL_INDEX,
		 AXIAL_GRADIENT,
		 generic_label[ AXIAL_GRADIENT ],
		(char *) system_notify_choice_proc,
		 generic_choices[ AXIAL_GRADIENT ],
		&present_not_present[ 0 ]
	);

	config_panel_message(
		SYSTEM_PANEL_INDEX,
		SYSTEMP_MSG,
		SYSTEM_PANEL_MESSAGE_LEN
	);

/*  minus 1 argument tells the fix programs no previous fix has been done  */

	adjust_panel_size( SYSTEM_PANEL_INDEX );
	fix_num_RF_chans( generic_choices[ NUM_RF_CHANS ], -1 );
	fix_gradient_presence( generic_choices[ AXIAL_GRADIENT ], -1 );
}
#endif


/*  Program to print out current selections to a File Handle.  */

int
display_generic_params( outf )
FILE *outf;
{
	int		 iter, this_choice;
	char		 quick_str[ 42 ];
	char		*quick_addr;
	extern char	*find_first_nonblank();

	for (iter = 0; iter < NUM_SYSTEM; iter++) {

	/*  Do not display anything based on the Axial Gradient
	    button or the System Panel Message.			*/

		if (iter == AXIAL_GRADIENT || iter == SYSTEMP_MSG)
		  continue;

		strcpy( &quick_str[ 0 ], generic_label[ iter ] );
		strcat( &quick_str[ 0 ], ":" );

	/*  Format specifier "%-40s" pushes the string to the left and appends
	    blank characters to the right for a total length of 40 characters.  */

		fprintf( outf, "%-40s", &quick_str[ 0 ] );
		this_choice = generic_choices[ iter ];
		if (this_choice < 0) {
			fprintf( outf, "unknown\n" );
			continue;
		}

		switch (iter) {

		  case SYS_BUTTON:
			quick_addr = system_choice[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case CONS_BUTTON:
			quick_addr = console_choice[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case H1_BUTTON:
			fprintf( outf, "%g", h1freq_value[ this_choice ] );
			break;

		  case TRAY_BUTTON:
			if (this_choice == NOT_PRESENT)
			  fprintf( outf, "Not Used" );
			else
			  fprintf( outf, "%s", sample_changer_choice[ this_choice ] );
			break;

		  case PORT_BUTTON:
			if (this_choice == NOT_PRESENT)
			  fprintf( outf, "Not Used\n");
			else
			{
		          if (port_value[ this_choice ] == 'n')
			    fprintf( outf, "Not Used\n");
		          else
			    fprintf( outf, "%c\n", port_value[ this_choice ] );
  			}
			break;

		  case SHIM_BUTTON:
			quick_addr = shimset_choice[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case NR_BUTTON:
			quick_addr = nr_choice[ this_choice ];
			fprintf(outf, "%s", find_first_nonblank(quick_addr));
			break;

		  case AF_BUTTON:
			quick_addr = filter_choice[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case VT_BUTTON:
			quick_addr = present_not_present[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case DMF_BUTTON:
			fprintf( outf, "%g", dmf_value[ this_choice ] );
			break;

		  case SW_BUTTON:
			quick_addr = sw_choice[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case NB_BUTTON:
			quick_addr = nb_choice[this_choice];
			fprintf(outf,"%s", find_first_nonblank(quick_addr));
			break;

		  case APINF_BUTTON:
			fprintf( outf, "Type %d", this_choice+1 );
			break;

		  case FIFO_BUTTON:
			fprintf( outf, "%g", fifo_loop_size[ this_choice ] );
			break;

		  case ROTOR_SYNC:
			quick_addr = present_not_present[ this_choice];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case PROTUNE:
			quick_addr = present_not_present[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case LOCK_FREQ:
			if (lock_freq_used)
			  fprintf( outf, "%.12g", cur_lock_freq );
			else
			  fprintf( outf, "Not Used" );
			break;

		  case IFFREQ_BUTTON:
			quick_addr = iffreq_choice[this_choice];
			fprintf(outf,"%s", find_first_nonblank(quick_addr));
			break;

		  case NUM_RF_CHANS:
			fprintf( outf, "%d", num_rf_chans_selected() );
			break;

		  default:
			break;
		}

		fprintf( outf, "\n" );
	}
	fprintf( outf, "\n" );
}


/*  Programs to get data from console, display corresponding selections.  */

#define  NO_TYPE	0
#define  CHAR		(NO_TYPE+1)
#define  DOUBLE		(CHAR+1)
#define  SPECIAL	(DOUBLE+1)

static struct _console_interface {
	double	dval;			/* Put 1st due to SPARC data alignment */
	int	item_type;
	int	value_is_valid;
	char	cval;
}
  console_interface[ NUM_SYSTEM ];

static int
init_console_table()
{
	console_interface[ SYS_BUTTON ].item_type = DOUBLE;
	console_interface[ CONS_BUTTON ].item_type = CHAR;
	console_interface[ H1_BUTTON ].item_type =  DOUBLE;
	console_interface[ TRAY_BUTTON ].item_type = DOUBLE;
	console_interface[ PORT_BUTTON ].item_type = CHAR;
	console_interface[ SHIM_BUTTON ].item_type = DOUBLE;
	console_interface[ NR_BUTTON ].item_type = DOUBLE;
	console_interface[ AF_BUTTON ].item_type = CHAR;
	console_interface[ VT_BUTTON ].item_type = DOUBLE;
	console_interface[ DMF_BUTTON ].item_type = DOUBLE;
	console_interface[ SW_BUTTON ].item_type = DOUBLE;
	console_interface[ NB_BUTTON ].item_type = DOUBLE;
	console_interface[ APINF_BUTTON ].item_type = DOUBLE;
	console_interface[ FIFO_BUTTON ].item_type = DOUBLE;
	console_interface[ ROTOR_SYNC ].item_type = DOUBLE;
	console_interface[ PROTUNE ].item_type = CHAR;
	console_interface[ LOCK_FREQ ].item_type = DOUBLE;
	console_interface[ IFFREQ_BUTTON ].item_type = DOUBLE;
	console_interface[ NUM_RF_CHANS ].item_type = DOUBLE;
	console_interface[ AXIAL_GRADIENT ].item_type = CHAR;
}

int
getSystemPanelFromConsole()
{
	char	cval;
	int	index, item_type, ival, numrcvrs;
	double	dval;
        double     stmmemsize[10];
        int	choices[10] = { 0,1,2,3,4,5,6,7,8,9 };
        

	for (index = 0; index < NUM_SYSTEM; index++) {
		console_interface[ index ].value_is_valid = 0;
		item_type = console_interface[ index ].item_type;
		if (item_type != CHAR && item_type != DOUBLE)
		  continue;

		if (item_type == DOUBLE) {
			ival = getConfigDoubleParameter(
				SYSTEM_PANEL_INDEX, index, &dval
			);
			if (ival != 0) {
				console_interface[ index ].value_is_valid = 0;
			}
			else {
				console_interface[ index ].dval = dval;
				console_interface[ index ].value_is_valid = 131071;
			}
		}
		else {			/* item_type == CHAR */
			ival = getConfigCharParameter(
				SYSTEM_PANEL_INDEX, index, &cval
			);
			if (ival != 0) {
				console_interface[ index ].value_is_valid = 0;
			}
			else {
				console_interface[ index ].cval = cval;
				console_interface[ index ].value_is_valid = 131071;
			}
		}
	}
        numrcvrs = get_dtmmemsizes(stmmemsize);
        /* fprintf(stderr,"numrcvrs: %d, 1st value: %lf\n",numrcvrs,stmmemsize[0]); */
	/* call this to create parmeter if it doesn't exist */
	config_setreal("stmmemsize", stmmemsize[0], 1 );
	/* call this no matter if arrayed or not, to be sure parameter is set properly, ie. going from array to a single value */
        set_array_real_using_table("stmmemsize",stmmemsize,numrcvrs,choices,numrcvrs);
        
}


static int
get_choice_standard( index )
int index;
{
	int	console_item_type, new_choice, type_of_entry;

	console_item_type = console_interface[ index ].item_type;
	type_of_entry = sys_iface_table[ index ].type_of_entry;

/*  console item type == CHAR    ===> type of entry == TABLE_OF_CHAR
    console item type == DOUBLE  ===> type of entry == TABLE_OF_DOUBLE	*/

	if ((console_item_type == CHAR   && type_of_entry != TABLE_OF_CHAR) ||
	    (console_item_type == DOUBLE && type_of_entry != TABLE_OF_DOUBLE))
		return( -1 );

	if (console_item_type == CHAR) {
		new_choice = find_table_char(
			console_interface[ index ].cval,
			sys_iface_table[ index ].a.ctable_addr,
			sys_iface_table[ index ].table_size
		);
	}				/* console item type == DOUBLE */
	else {
		new_choice = find_table_double(
			console_interface[ index ].dval,
			sys_iface_table[ index ].a.dtable_addr,
			sys_iface_table[ index ].table_size
		);
	}

	return( new_choice );
}

int
updateSystemPanelUsingConsole()
{
	int	console_item_type, index, ival, new_choice, type_of_entry;

	for (index = 0; index < NUM_SYSTEM; index++) {
		if (index == CONS_BUTTON) {
			ival = getConfigIntParameter(
				 SYSTEM_PANEL_INDEX,
				 CONS_BUTTON,
				&new_choice
			);
			set_choice_in_panel_choice(
				SYSTEM_PANEL_INDEX,
				CONS_BUTTON,
				new_choice
			);
			continue;
		}
		if (console_interface[ index ].value_is_valid == 0)
		  continue;

		console_item_type = console_interface[ index ].item_type;
		if (console_item_type != CHAR && console_item_type != DOUBLE)
		  continue;

	/*  Take care of Lock Frequecy here...
	    Its entry type will not be a table.
	    See get_choice_standard.		*/

		if (index == LOCK_FREQ) {
			char	lockfreq_str[ 14 ];

		/*  Console stores/returns lock frequency in Hz.
		    program accepts input/display is in MHz.	*/

			lock_freq_used = 131071;
			cur_lock_freq = console_interface[ LOCK_FREQ ].dval;
			cur_lock_freq /= 1.0e6;
			sprintf( &lockfreq_str[ 0 ], "%g", cur_lock_freq );
			set_text_in_panel_text_input(
				 SYSTEM_PANEL_INDEX,
				 LOCK_FREQ,
				&lockfreq_str[ 0 ]
			);
			continue;		/* skip remainder of this iteration */
		}				/* move on to the next index/button */

	/*  System-Type is special and works like this.  If the query
	    to the console data object worked at all, the system is a 
	    spectrometer.  See console_data.c and get_systemtype.
	    The reason for this is the conpar parameter "system" can't
	    be made to fit the CHAR/DOUBLE schemes in use for all the
	    other parameters that VNMR config works with.

	    Except for that bit of obscurity, it works like other
	    panel choice items.						*/

		if (index == SYS_BUTTON) {
			new_choice = PRESENT;
			if (console_interface[ SYS_BUTTON ].dval == 
							IMAGER)
			    new_choice = SPECTROMETER_CHOICE;
		}
		else if (index == APINF_BUTTON) {
			if (is_system_hydra())
			  new_choice = HYDRA_AP_CHOICE;
			else
			  new_choice = get_choice_standard( index );
		}
		else if (index == AXIAL_GRADIENT) {
		        new_choice = gradients_present(SET_CHOICE);
		}
		else {
			new_choice = get_choice_standard( index );
		}

	/*  special_update_choice is required if the number of RF channels
	    changes or the presence/absence of gradients changes.		*/

		if (new_choice >= 0) {
			special_update_choice( index, new_choice );
			set_choice_in_panel_choice(
				SYSTEM_PANEL_INDEX,
				index,
				new_choice
			);
			generic_choices[ index ] = new_choice;
		}
	}
}


/*  Programs to write values to the (Hydra) console  */

storeSystemPanelInConsole()
{
	int	iter, ival, this_choice;

	for (iter = 0; iter < NUM_SYSTEM; iter++) {
		if (iter == LOCK_FREQ) {
			if ( lock_freq_used ) {

		/*  Console stores/returns lock frequency in Hz.
		    program accepts input/display is in MHz.	*/

				ival = setConfigDoubleParameter(
					SYSTEM_PANEL_INDEX,
					LOCK_FREQ,
					cur_lock_freq * 1.0e6
				);
			}
			else {
				printf(
			   "Lock frequency is not used, nothing new will be stored\n"
				);
			}

			continue;	/* Skip rest of for loop  */
		}
		else if (iter == H1_BUTTON) {
			this_choice = generic_choices[ H1_BUTTON ];
			if (this_choice < 0)
			 this_choice = 0;
			else if (this_choice >=
					sys_iface_table[ H1_BUTTON ].table_size)
			 this_choice = sys_iface_table[ H1_BUTTON ].table_size - 1;
			ival = setConfigDoubleParameter(
				SYSTEM_PANEL_INDEX,
				H1_BUTTON,
				h1freq_value[ this_choice ]
			);
			continue;
		}
		else if (iter == IFFREQ_BUTTON) {
		   this_choice = generic_choices[ IFFREQ_BUTTON ];
		   if (this_choice < 0)
			this_choice = 0;
		   else if (this_choice >= 
				   sys_iface_table[IFFREQ_BUTTON].table_size)
                        this_choice=sys_iface_table[IFFREQ_BUTTON].table_size-1;
		   setConfigDoubleParameter(
                                SYSTEM_PANEL_INDEX,
                                IFFREQ_BUTTON,
                                iffreq_value[ this_choice ] * 10.0
		   );
		   continue;
		}
		if (iter == SYS_BUTTON) {
			this_choice = generic_choices[ SYS_BUTTON ];
			if (this_choice < 0)
			 this_choice = 0;
			ival = setConfigDoubleParameter(
				SYSTEM_PANEL_INDEX,
				SYS_BUTTON,
			        sys_hwconfig_value[this_choice]);
		}

	/*  No other System Panel values are stored in the console currently !!  */

	}
}
