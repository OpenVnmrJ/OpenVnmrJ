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

#include <stdio.h>

#include "group.h"

#include "vconfig.h"
#include "interface.h"

#define  PAR_TABLE_SIZE		21
#define  SWINDEX		5
#define  FBINDEX		6
#define  DHPINDEX		9
#define  PULSEINDEX		13
#define  DELAYINDEX		14
#define  LPULSEINDEX		15
#define  SHIMINDEX		19

static char	*first_param_table[] = {
	"parmin",
	"parmax",
	"parstep",
	 NULL
};

/*  The shims work like this.  The parameter shimset stores the kind of shims
    present in the console.  It is specified as an integer, with valid values
    starting at 0 and including each subsequent integer up to (one minus) the
    number of different kinds of shims.

    The parmax and parmin arrays are accessed using the value from shimset.
    At this time parstep is always 1, so it doesn't get a table.		*/

static double	shim_parmax[] = { 2047.0,
				  2047.0, 2047.0, 32767.0, 32767.0,
				  32767.0, 32767.0, 32767.0, 2047.0,
				  32767.0, 2047.0, 32767.0 };
static double	shim_parmin[] = { -2047.0,
				  -2047.0, -2047.0, -32767.0, -32767.0,
				  -32767.0, -32767.0, -32767.0, -2047.0,
				  -32767.0, -2047.0, -32767.0 };

/*  Do Preliminary Fixes expands the size of parameters
    named in first param table to the current standard.  */

do_prelim_fixes()
{
	char	**tptr;

	tptr = &first_param_table[ 0 ];
	while (*tptr != NULL) {
		if (set_size_array_real( *tptr, PAR_TABLE_SIZE ) != 0)
		  return( -1 );
		tptr++;
	}

	return( 0 );
}

/*  Use index of NOT_PRESENT for the old output board,
    PRESENT for the new output board.			*/

#ifndef VMS
static double sw_step[]    = { -1.0e-07, -2.5e-08, -1.25e-08 };
static double delay_step[] = { 1.0e-07,  2.5e-08,  1.25e-08 };
static double pulse_step[] = { 0.1,      0.025,    0.0125 };
#endif

/*  Before do final fixes is called, the conpar tree shall
    have received any new values from the config session.	*/

static
do_fix_output_board(max_lb)
int max_lb;
{
	int	output_board_index, result;
	double	fifo_loop_size;
	
	result = P_getreal( SYSTEMGLOBAL, "fifolpsize", &fifo_loop_size, 1 );
	if (result == 0) {

/*  The Output Board Index is defined relative to the new output board.
 *  An index of PRESENT implies the acquisition controller board is present.
 *  An index of NOT PRESENT implies the old output board is present.
 *  The old output board has a fifo loop size of 63.  The Acquisition
 *  controller board has fifo loop sizes of 1024 or 2048.  The minimum
 *  time event is either 100 nanoseconds or 25 nanoseconds, respectively.
 *  New acquisition system has minimum time of 12.5 nsec.  It is selected
 *  if the max_lb is 500 KHz
 *
 *  Please note that FIFO loop size of 1023 AND 2048 map to an output
 *  board index of PRESENT...
 */

		if ( fifo_loop_size < 1000.0 )
		   output_board_index = 0;
                else
                   output_board_index = (max_lb == 500) ? 2 : 1;

		set_element_array_real_using_table(
			"parstep",
			 SWINDEX,
			 output_board_index, 
			&sw_step[ 0 ],
			 sizeof( sw_step ) / sizeof( double )
		);

		set_element_array_real_using_table(
			"parstep",
			 PULSEINDEX,
			 output_board_index, 
			&pulse_step[ 0 ],
			 sizeof( pulse_step ) / sizeof( double )
		);

		set_element_array_real_using_table(
			"parstep",
			 DELAYINDEX,
			 output_board_index, 
			&delay_step[ 0 ],
			 sizeof( delay_step ) / sizeof( double )
		);

		set_element_array_real_using_table(
			"parstep",
			 LPULSEINDEX,
			 output_board_index, 
			&pulse_step[ 0 ],
			 sizeof( pulse_step ) / sizeof( double )
		);
	}
}

static
do_fix_shims()
{
	int	i_shims, result;
	double	d_shims;

	result = P_getreal( SYSTEMGLOBAL, "shimset", &d_shims, 1 );
	if (result == 0) {
		i_shims = (int) (d_shims+0.1);

		set_element_array_real_using_table(
			"parmax",
			 SHIMINDEX,
			 i_shims,
			&shim_parmax[ 0 ],
			 sizeof( shim_parmax ) / sizeof( double )
		);

		set_element_array_real_using_table(
			"parmin",
			 SHIMINDEX,
			 i_shims,
			&shim_parmin[ 0 ],
			 sizeof( shim_parmin ) / sizeof( double )
		);

		config_setreal( "parstep", 1.0, SHIMINDEX );
	}
}

do_final_fixes()
{
#ifndef VMS
	char	amptype[ CONFIG_MAX_RF_CHAN+1 ];
	double	swmax;
        char    fbval[10];
	int	result;
	double	traymax;
        int     max_loband;

	result = P_getreal( SYSTEMGLOBAL, "traymax", &traymax, 1 );
	if (result == 0) {
		if (traymax == 0.0)
		  P_setactive( SYSTEMGLOBAL, "traymax", ACT_OFF );
		else
		  P_setactive( SYSTEMGLOBAL, "traymax", ACT_ON );
	}

	do_fix_shims();
        max_loband = 100;

	config_setreal( "parmin", 200.0, FBINDEX );
	config_setreal( "parstep", 200.0, FBINDEX );
	config_setreal( "parmax", 51200.0, FBINDEX );
	result = P_getstring( SYSTEMGLOBAL, "audiofilter", &fbval[ 0 ], 1, 4);
	if (result == 0)
        {
	   if (fbval[ 0 ] == 'b')
           {
	       config_setreal( "parmax", 49900.0, FBINDEX );
	       config_setreal( "parmin", 100.0, FBINDEX );
	       config_setreal( "parstep", 100.0, FBINDEX );
           }
	}
	result = P_getreal( SYSTEMGLOBAL, "maxsw_loband", &swmax, 1);
	if (result == 0)
        {
	    if (swmax > 499000){
                max_loband = 500;
		config_setreal( "parmax", 256000.0, FBINDEX );
		config_setreal( "parmin", 1000.0, FBINDEX );
		config_setreal( "parstep", 1000.0, FBINDEX );
	    }
	    else if (swmax > 199000){
                max_loband = 200;
		config_setreal( "parmax", 102400.0, FBINDEX );
		config_setreal( "parmin", 400.0, FBINDEX );
		config_setreal( "parstep", 400.0, FBINDEX );
	    }
	}
	do_fix_output_board(max_loband);
	result = P_getstring(
		SYSTEMGLOBAL, "amptype", &amptype[ 0 ], 1, CONFIG_MAX_RF_CHAN+1
	);
	if (result == 0) {
		if (amptype[ 1 ] == 'c')
		  config_setreal( "parmax", 255.0, DHPINDEX );
	}
#endif

	return( 0 );
}
