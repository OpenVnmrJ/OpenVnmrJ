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
#include <ctype.h>

#include "vnmrsys.h"			/* VNMR include file */
#include "group.h"			/* VNMR include file */
#include "vconfig.h"			/* vconfig include file */
#include "interface.h"			/* vconfig include file */
#include "rf_panel.h"			/* vconfig include file */

#define  RF_PANEL_MESSAGE_LEN	40

extern char		*present_not_present[];
extern char		*rf_chan_name[];
extern char		 no_yes[];

static int		 RF_chan_choice[ CONFIG_MAX_RF_CHAN*NUM_RF_BUTTONS ];

static char		*rf_label[ NUM_RF_BUTTONS ];
static char		*stepsize_label[ 2 ];

static int		 upper_limit_used[ CONFIG_MAX_RF_CHAN ];
static int		 upper_limit_value[ CONFIG_MAX_RF_CHAN ];
static int		 is_synthesizer_present[ CONFIG_MAX_RF_CHAN ];

static int		 upper_limit_index[ CONFIG_MAX_RF_CHAN ] = {
	17,
	9,			/* indexes into `parmax' and `parmin' */
	18,			/* for coarse attenuation (cattn)     */
        21,
	23
};
static int		 stepsize_index[ CONFIG_MAX_RF_CHAN ] = {
	7,
	8,			/* indexes into `parstep'  */
	16,			/* for tof, dof, ...       */
	20,
	22
};

/*  Tables of values and corresponding choices displayed.
    Each table must be no larger than MAX_NUM_CHOICES.
    Program will fail due to a Segment Violation if this
    is not adhered to.					*/

static double		 fattn_value[] = { 0.0, 4095.0 };
static double		 stepsize_value[] = { 0.1, 0.2, 1.0, 100.0 };

/*  Define RF types, as a choice in CONFIG's user interface and
    as values for the parameter `system'.  Each table must have
    the same number of non-NULL entries.  The program relies on
    the last entry in `rf_choice' being NULL.  See where the
    button for RF Choice is created.				*/

#define  DIRECT_SYN	1

static char		*rf_desc[] = {
	"U+ Direct Synthesis",
	"U+ H1 Only",
	"Deuterium Decoupler",
	"Direct Synthesis",
	"Broadband",
	"Fixed Frequency",
	"SIS Modulator",
	 NULL
};
static char		*rf_choice[] = {
	" U+ Direct Synthesis ",
	" U+ H1 Only ",
	" Deuterium Decoupler ",
	" Direct Synthesis ",
	" Broadband ",
	" Fixed Frequency ",
	" SIS Modulator ",
	 NULL
};
static char		rf_value[] = { 'd', 'd', 'l', 'c', 'b', 'a', 'm' };

/*  Similarly for PTS synthesizer frequencies.
    The program relies the last entry being NULL.	*/

static char		*pts_choice[] = {
	"Not Present",
	" PTS 160 ",
	" PTS 200 ",
	" PTS 250 ",
	" PTS 320 ",
	" PTS 500 ",
	" PTS 620 ",
	" PTS 1000 ",
	 NULL
};
static double		pts_value[] = {
	0.0,
	160.0,
	200.0,
	250.0,
	320.0,
	500.0,
	620.0,
	1000.0,
};

/*  The program relies on the last entry in `amptype_choice' being NULL.  */

/*  The Hydra/UnityPlus allows 4 channels.  Unity allows only 3 channels.
    The 4th channel can be set up so it is sharing the amplifier with the
    3rd channel.  At this time no other channel can be set up this way.

    The (static) program channel_can_share_amplifier tells the other
    rf panel programs if a channel can be set up to share.

    The amptype choice table is defined with one additional choice.
    Initially it is set to NULL.  If the channel allows this sharing,
    the (presumably NULL) entry is replaced with "Shared".		*/

static char		*amptype_choice[] = {
	"Class C",
	"Linear Full Band",
	"Linear Low  Band",
	"Linear Broadband",
	 NULL,
	 NULL
};
static char		amptype_value[] = { 'c', 'a', 'l', 'b', 'n' };
static char		shared_entry[] = "Shared";

static char		*overrange_choice[] = {
	"Not Present",
	" 10000 Hz  ",
	" 100000 Hz ",
	 NULL
};

static double		overrange_value[] = {
	0.0,
	10000.0,
	100000.0,
};

#define  INDEX_FOR_SHARED	4

/*  The program relies on the last entry in `cattn_choice' being NULL.	*/

static char		*cattn_choice[] = {
	" Not Present ",
	" 63 dB ",
	" 79 dB ",
	" 63.5 dB (SIS)",
	 NULL
};
static double		 cattn_value[] = { 0.0, 63.0, 79.0, 127.0 };

static struct param_interface	rf_iface_table[ NUM_RF_BUTTONS ];

/************************************************************************/

static int
channel_can_share_amplifier( this_channel )
int this_channel;
{
	if (this_channel == RFCHAN4-RFCHAN1)
	  return( 131071 );
	else
	  return( 0 );
}

/*  Special programs section:

    The Upper Limit Interface is a Safety Limit for Coarse Attenuators.
    The operator can set an upper limit to prevent too much power from
    going to the probe.  This upper limit must be less than the
    magnitude of the attenuation on the channel.			*/

static double
get_current_cattn_val( this_channel )
{
	int	c_choice, c_index, quicksize;

	c_index = this_channel * NUM_RF_BUTTONS + COARSE_ATTN;
	quicksize = rf_iface_table[ COARSE_ATTN ].table_size;

	c_choice = RF_chan_choice[ c_index ];
	if (c_choice < 0)				/* Prevent inadvertant */
	  c_choice = 0;					/* access of memory    */
	else if (c_choice >= quicksize)			/* out of range of the */
	  c_choice = quicksize-1;			/* coarse attn array   */

	return( cattn_value[ c_choice ] );
}

static double
get_current_amptype( this_channel )
{
	int	c_choice, c_index, quicksize;

	c_index = this_channel * NUM_RF_BUTTONS + TYPE_OF_AMP;
	quicksize = rf_iface_table[ TYPE_OF_AMP ].table_size;

	c_choice = RF_chan_choice[ c_index ];
	if (c_choice < 0)			/* Prevent inadvertant */
	  c_choice = 0;				/* access of memory    */
	else if (c_choice >= quicksize)		/* out of range of the */
	  c_choice = quicksize-1;		/* coarse attn array   */

	return( amptype_value[ c_choice ] );
}

static int
upper_limit_iface( direction, this_channel )
int direction;
int this_channel;
{
	int	choice_index, ival, result;
	double	dval;

/*  For UPDATE_PARAM, the two parameters are "parmax" and "parmin".
    Corresponding values in both arrays are affected, so we use the
    same index array for both "parmax" and "parmin".  Originally
    the program only worked with "parmax", so the index table is
    an upper limit index table.						*/

	if (direction == UPDATE_PARAM) {
		int	cval, l_limit, result_1, result_2;

		cval = (int) (get_current_cattn_val( this_channel ) + 0.5);
		if (cval == 79)
		  l_limit = -16;
		else
		  l_limit = 0;

		dval = (double) (upper_limit_value[ this_channel ]);
		result_1 = config_setreal(
			"parmax", dval, upper_limit_index[ this_channel ]
		);
		
		dval = (double) l_limit;
		result_2 = config_setreal(
			"parmin", dval, upper_limit_index[ this_channel ]
		);

		dval = (double) 1.0;
		result_2 = config_setreal(
			"parstep", dval, upper_limit_index[ this_channel ]
		);


/*  If one of the results show an error, return that result.	*/

		if (result_1 != 0)
		  result = result_1;
		else if (result_2 != 0)
		  result = result_2;
		else
		  result = 0;

		return( result );
	}

/*  For SET_CHOICE, the RF Panel program effectively ignores the return
    value, since it never consults the choice corresponding to Upper Limit.
    The user enters the value directly and the CONPAR parameter is
    updated from the user's input, as obtained from the SunView object.	*/

	else if (direction == SET_CHOICE) {
		int	cval;
		double	maxval, minval;

		result = P_getreal(
			 SYSTEMGLOBAL,
			"parmax",
			&dval,
			 upper_limit_index[ this_channel ]
		);
		if (result != 0) {
			upper_limit_used[ this_channel ] = 0;
			upper_limit_value[ this_channel ] = 0;
			return( 0 );
		}
		else {

	/*  With 63 dB or 79 dB attenuators are present,
	    the upper limit on the upper limit is always 63.
	    With 63.5 dB attenuators the upper limit is 127.
	    With old VXR's (cattn == 0) the upper limit is 255.	*/

			upper_limit_used[ this_channel ] = 1;

	/*  But the lower limit on the upper limit changes
	    between 63 and 79 dB attenuators, as shown below.	*/

			cval = (int) (get_current_cattn_val( this_channel ) + 0.5);
			if (cval == 79)
			  minval = -16.0;
			else
			  minval = 0.0;

			if ((cval == 79) || (cval == 63))
			  maxval =  63.0;
			else if (cval == 127)
			  maxval = 127.0;
			else if (cval == 0)
			  maxval = 255.0;
			else
			  maxval = 63.0;

			if (dval < minval)
			  dval = 0.0;
			else if (dval > maxval)
			  dval = maxval;
			ival = (int) (dval+0.1);
			upper_limit_value[ this_channel ] = ival;
			return( 0 );
		}
	}
	else
	  return( -1 );
}

/*  Special note on table sizes.  Only tables defined in this source
    file can have the size computed by software.  External tables
    (for example, `no_yes') must have the size entered by hand.		*/

static int
init_interface_table()
{
	rf_iface_table[ NAME_OF_CHAN ].type_of_entry = NULL_ENTRY;
	rf_iface_table[ NAME_OF_CHAN ].a.ctable_addr = NULL;
	rf_iface_table[ NAME_OF_CHAN ].b.param_name  = NULL;
	rf_iface_table[ NAME_OF_CHAN ].table_size    = 0;

	rf_iface_table[ TYPE_OF_RF ].type_of_entry = TABLE_OF_CHAR;
	rf_iface_table[ TYPE_OF_RF ].a.ctable_addr = &rf_value[ 0 ];
	rf_iface_table[ TYPE_OF_RF ].b.param_name  = "rftype";
	rf_iface_table[ TYPE_OF_RF ].table_size    = sizeof( rf_value );
	rf_iface_table[ TYPE_OF_RF ].param_index   = SCALER;

	rf_iface_table[ TYPE_OF_SYN ].type_of_entry = TABLE_OF_DOUBLE;
	rf_iface_table[ TYPE_OF_SYN ].a.dtable_addr = &pts_value[ 0 ];
	rf_iface_table[ TYPE_OF_SYN ].b.param_name  = "ptsval";
	rf_iface_table[ TYPE_OF_SYN ].table_size    = sizeof( pts_value ) / DSIZE;
	rf_iface_table[ TYPE_OF_SYN ].param_index   = SCALER;

	rf_iface_table[ LATCHING ].type_of_entry = TABLE_OF_CHAR;
	rf_iface_table[ LATCHING ].a.ctable_addr = &no_yes[ 0 ];
	rf_iface_table[ LATCHING ].b.param_name  = "latch";
	rf_iface_table[ LATCHING ].table_size    = NO_YES_SIZE;
	rf_iface_table[ LATCHING ].param_index   = SCALER;

/*
	rf_iface_table[ OVERRANGE ].type_of_entry = TABLE_OF_CHAR;
	rf_iface_table[ OVERRANGE ].a.ctable_addr = &no_yes[ 0 ];
	rf_iface_table[ OVERRANGE ].b.param_name  = "overrange";
	rf_iface_table[ OVERRANGE ].table_size    = NO_YES_SIZE;
*/
	rf_iface_table[ OVERRANGE ].type_of_entry = TABLE_OF_DOUBLE;
	rf_iface_table[ OVERRANGE ].a.dtable_addr = &overrange_value[ 0 ];
	rf_iface_table[ OVERRANGE ].b.param_name  = "overrange";
	rf_iface_table[ OVERRANGE ].table_size    = sizeof( overrange_value ) / DSIZE;
	rf_iface_table[ OVERRANGE ].param_index   = SCALER;

	rf_iface_table[ STEPSIZE ].type_of_entry = TABLE_OF_DOUBLE;
	rf_iface_table[ STEPSIZE ].a.dtable_addr = &stepsize_value[ 0 ];
	rf_iface_table[ STEPSIZE ].b.param_name  = "parstep";
	rf_iface_table[ STEPSIZE ].table_size    = sizeof( stepsize_value ) / DSIZE;
	rf_iface_table[ STEPSIZE ].param_index   = LOOKUP_INDEX;

	rf_iface_table[ COARSE_ATTN ].type_of_entry = TABLE_OF_DOUBLE;
	rf_iface_table[ COARSE_ATTN ].a.dtable_addr = &cattn_value[ 0 ];
	rf_iface_table[ COARSE_ATTN ].b.param_name  = "cattn";
	rf_iface_table[ COARSE_ATTN ].table_size    = sizeof( cattn_value ) / DSIZE;
	rf_iface_table[ COARSE_ATTN ].param_index   = SCALER;

	rf_iface_table[ UPPER_LIMIT ].type_of_entry = SPECIAL_ENTRY;
	rf_iface_table[ UPPER_LIMIT ].a.ctable_addr = NULL;
	rf_iface_table[ UPPER_LIMIT ].b.special_sub = upper_limit_iface;
	rf_iface_table[ UPPER_LIMIT ].table_size    = 0;

	rf_iface_table[ FINE_ATTN ].type_of_entry = TABLE_OF_DOUBLE;
	rf_iface_table[ FINE_ATTN ].a.dtable_addr = &fattn_value[ 0 ];
	rf_iface_table[ FINE_ATTN ].b.param_name  = "fattn";
	rf_iface_table[ FINE_ATTN ].table_size    = sizeof( fattn_value ) / DSIZE;
	rf_iface_table[ FINE_ATTN ].param_index   = SCALER;

	rf_iface_table[ WAVE_FORM_GEN ].type_of_entry = TABLE_OF_CHAR;
	rf_iface_table[ WAVE_FORM_GEN ].a.ctable_addr = &no_yes[ 0 ];
	rf_iface_table[ WAVE_FORM_GEN ].b.param_name  = "rfwg";
	rf_iface_table[ WAVE_FORM_GEN ].table_size    = NO_YES_SIZE;
	rf_iface_table[ WAVE_FORM_GEN ].param_index   = SCALER;

	rf_iface_table[ TYPE_OF_AMP ].type_of_entry = TABLE_OF_CHAR;
	rf_iface_table[ TYPE_OF_AMP ].a.ctable_addr = &amptype_value[ 0 ];
	rf_iface_table[ TYPE_OF_AMP ].b.param_name  = "amptype";
	rf_iface_table[ TYPE_OF_AMP ].table_size    = sizeof( amptype_value );
	rf_iface_table[ TYPE_OF_AMP ].param_index   = SCALER;

/*  No initialization for the RF message item at the bottom (for now)  */

}

static int
init_rf_labels()
{
	stepsize_label[ NOT_PRESENT ] = "Frequency Step Size";
	stepsize_label[ PRESENT ] = "     Step Size";

	rf_label[ NAME_OF_CHAN ] = "Configuring Channel";
	rf_label[ TYPE_OF_RF ] = "Type of RF";
	rf_label[ TYPE_OF_SYN ] = "Synthesizer";
	rf_label[ LATCHING ] = "     Latching";
	rf_label[ OVERRANGE ] =	"     Frequency Overrange";
	rf_label[ STEPSIZE ] =	stepsize_label[ NOT_PRESENT ];
	rf_label[ COARSE_ATTN ] = "Coarse Attenuator";
	rf_label[ UPPER_LIMIT ] = "     Upper Limit";
	rf_label[ FINE_ATTN ] = "Fine Attenuator";
	rf_label[ WAVE_FORM_GEN ] = "Waveform Generator";
	rf_label[ TYPE_OF_AMP ] = "Type of Amplifier";
}

static int
check_for_syn( this_channel )
int this_channel;
{
	int	retval, rf_choice, rf_index, syn_choice, syn_index;

	rf_index   = this_channel*NUM_RF_BUTTONS + TYPE_OF_RF;
	rf_choice  = RF_chan_choice[ rf_index ];
	syn_index  = this_channel*NUM_RF_BUTTONS + TYPE_OF_SYN;
	syn_choice = RF_chan_choice[ syn_index ];
	if (rf_choice == DIRECT_SYN && syn_choice > NOT_PRESENT)
	  retval = PRESENT;
	else
	  retval = NOT_PRESENT;

	return( retval );
}

static
set_rf_choice_from_table_of_char( this_button )
int this_button;
{
	char	buf_of_char[ CONFIG_MAX_RF_CHAN+1 ];
	int	choice_index, param_defined, result, this_channel;

	param_defined = 1;

/*  Special note on P_getstring:

    The last argument tells P_getstring the total size of the buffer
    in the 3rd argument, including the terminating NULL character.
    So to get CONFIG_MAX_RF_CHAN non-null characters, you must tell
    it the buffer has space for one more character.			*/

	result = P_getstring(
		 SYSTEMGLOBAL,
		 rf_iface_table[ this_button ].b.param_name,
		&buf_of_char[ 0 ],
		 1,
		 CONFIG_MAX_RF_CHAN+1
	);

	if (result != 0)
	  param_defined = 0;

	for (this_channel = 0; this_channel < CONFIG_MAX_RF_CHAN; this_channel++) {
	   choice_index = this_channel*NUM_RF_BUTTONS + this_button;

	   if (param_defined && buf_of_char[ this_channel ] != '\0')
           {
	      RF_chan_choice[ choice_index ] = find_table_char(
			buf_of_char[ this_channel ],
			rf_iface_table[ this_button ].a.ctable_addr,
			rf_iface_table[ this_button ].table_size);
              if (!strcmp(rf_iface_table[ this_button ].b.param_name,"rftype")
                  && (buf_of_char[ this_channel ] == 'd') )
              {
	         char	tmpstr[ 128];
                 
	         P_getstring(SYSTEMGLOBAL, "rfchtype", tmpstr, this_channel+1,
		             127);
	         RF_chan_choice[ choice_index ] += (strcmp(tmpstr,
                         rf_desc[RF_chan_choice[choice_index]])) ? 1 : 0;
              }
           }
	   else
	      RF_chan_choice[ choice_index ] = 0;
	}
}

static
set_rf_choice_from_table_of_double( this_button )
int this_button;
{
	int	choice_index, result, this_channel;
	double	dval;

/*  The type of entry in this parameter interface entry (this_button serves as
    as the index into the table) is TABLE_OF_DOUBLE; that is how this program
    got called.  Now the parameter index can be either SCALER or LOOKUP_INDEX;
    a non-zero index cannot be used in the RF panels.  At this time only the
    STEPSIZE is a LOOKUP_INDEX entry.  Other combinations are errors.		*/

	for (this_channel = 0; this_channel < CONFIG_MAX_RF_CHAN; this_channel++) {
		choice_index = this_channel*NUM_RF_BUTTONS + this_button;

		if (rf_iface_table[ this_button ].param_index == SCALER)
		  result = P_getreal(
			 SYSTEMGLOBAL,
			 rf_iface_table[ this_button ].b.param_name,
			&dval,
			 this_channel+1
		  );
		else if (this_button == STEPSIZE)
		  result = P_getreal(
			 SYSTEMGLOBAL,
			 rf_iface_table[ this_button ].b.param_name,
			&dval,
			 stepsize_index[ this_channel ]
		  );
		else {

	/*  Here is the error clause.  Print the message only once per button  */

			if (this_channel == 0)
			  printf(
	   "program error in set rf choice from table of double, index = %d\n", this_button
			  );
			result = -1;
		}

		if (result == 0)
		  RF_chan_choice[ choice_index ] = find_table_double(
			dval,
			rf_iface_table[ this_button ].a.dtable_addr,
			rf_iface_table[ this_button ].table_size
		  );
		else
		  RF_chan_choice[ choice_index ] = 0;
	}
}

static
set_rf_choice_from_special_entry( this_button )
int this_button;
{
	int	choice_index, this_channel;
	int	(*routine_addr)();

	for (this_channel = 0; this_channel < CONFIG_MAX_RF_CHAN; this_channel++) {
		choice_index = this_channel*NUM_RF_BUTTONS + this_button;
		routine_addr = rf_iface_table[ this_button ].b.special_sub;
		if (routine_addr != NULL)
		  RF_chan_choice[ choice_index ] =
			(*routine_addr)( SET_CHOICE, this_channel );
		else
		  RF_chan_choice[ choice_index ] = 0;
	}
}

int
init_rf_choices()
{
	int	iter;

	init_interface_table();
	init_rf_labels();
	init_console_table();
	for (iter = 0; iter < NUM_RF_BUTTONS; iter++)
	  switch (rf_iface_table[ iter ].type_of_entry) {

		case TABLE_OF_CHAR:
			set_rf_choice_from_table_of_char( iter );
			break;

		case TABLE_OF_DOUBLE:
			set_rf_choice_from_table_of_double( iter );
			break;

		case SPECIAL_ENTRY:
			set_rf_choice_from_special_entry( iter );
			break;

	/*  Special note:
	    No INDEX_ENTRY in the RF interface table currently.  */

		default:
			break;
	  }

	for (iter = 0; iter < CONFIG_MAX_RF_CHAN; iter++)
	  is_synthesizer_present[ iter ] = check_for_syn( iter );
}

#ifndef VMS

/*  update_rf_char_conpar and update_rf_double_conpar are called only once for
    each button.  They receive the number of RF channels as an argument, since
    they should only update values in conpar for actual channels configured to
    be present on this system.  In contrast are the set choice programs which
    must make a choice for all possible channels as specified when this program
    got compiled.								*/

static int
update_rf_char_conpar( this_button, number_of_rf_chans )
int this_button;
int number_of_rf_chans;
{
	char	buf_of_char[ CONFIG_MAX_RF_CHAN+1 ];
	int	choice, choice_index, r, this_channel;

	if (number_of_rf_chans > CONFIG_MAX_RF_CHAN)
	  number_of_rf_chans = CONFIG_MAX_RF_CHAN;

	for (this_channel = 0; this_channel < number_of_rf_chans; this_channel++) {
		choice_index = this_channel*NUM_RF_BUTTONS + this_button;
		choice = RF_chan_choice[ choice_index ];
		if (choice < 0)
		  choice = 0;
		else if (choice > rf_iface_table[ this_button ].table_size)
		  choice = rf_iface_table[ this_button ].table_size;

		buf_of_char[ this_channel ] =
		  rf_iface_table[ this_button ].a.ctable_addr[ choice ];
                if (!strcmp(rf_iface_table[ this_button ].b.param_name,"rftype")
                         && (buf_of_char[ this_channel ] == 'd') )
                   config_setstring( "rfchtype", rf_desc[ choice ],
                                  this_channel+1);
	}
	buf_of_char[ number_of_rf_chans ] = '\0';

/*  The char parameters are strings.  The length is the
    number of RF channels for this system.  The previous
    statement served to set the length of this string.	*/

	r = config_setstring(
		 rf_iface_table[ this_button ].b.param_name,
		&buf_of_char[ 0 ],
		 1
	);
}

static int
update_rf_double_conpar( this_button, number_of_rf_chans )
int this_button;
int number_of_rf_chans;
{
	int	choice_array[ CONFIG_MAX_RF_CHAN ];
	int	choice, choice_index, r, this_channel;

	if (number_of_rf_chans > CONFIG_MAX_RF_CHAN)
	  number_of_rf_chans = CONFIG_MAX_RF_CHAN;

	for (this_channel = 0; this_channel < number_of_rf_chans; this_channel++) {
		choice_index = this_channel*NUM_RF_BUTTONS + this_button;
		choice = RF_chan_choice[ choice_index ];
		if (choice < 0)
		  choice = 0;
		else if (choice > rf_iface_table[ this_button ].table_size)
		  choice = rf_iface_table[ this_button ].table_size;

		choice_array[ this_channel ] = choice;
	}

/*  The double precision parameters are mostly arrays.  The
    size of these arrays is the number of RF channels in this
    system.  The set array real using table sets all the
    values in one fell swoop.  The exception (for now) is the
    STEPSIZE.  Its choices are stored in selected indexes of
    parstep.  This exception is encoded by setting the
    parameter index in the parameter interface table to be
    LOOKUP_ENTRY.						*/

	if (rf_iface_table[ this_button ].param_index == SCALER)
	  r = set_array_real_using_table(
		 rf_iface_table[ this_button ].b.param_name,
		 rf_iface_table[ this_button ].a.dtable_addr,
		 rf_iface_table[ this_button ].table_size,
		&choice_array[ 0 ],
		 number_of_rf_chans
	  );
	else if (this_button == STEPSIZE) {
		for (this_channel = 0; this_channel < number_of_rf_chans; this_channel++) {
		  r = set_element_array_real_using_table(
			 rf_iface_table[ this_button ].b.param_name,
			 stepsize_index[ this_channel ],
			 choice_array[ this_channel ],
			&stepsize_value[ 0 ],
			 sizeof( stepsize_value ) / DSIZE
		  );
		  config_setreal("parmax", 1e9,stepsize_index[ this_channel ]);
		  config_setreal("parmin",-1e9,stepsize_index[ this_channel ]);
          }
	}
}

static
update_rf_special_conpar( this_button, number_of_rf_chans )
int this_button;
int number_of_rf_chans;
{
	int	ival, this_channel;
	int	(*routine_addr)();

	for (this_channel = 0; this_channel < number_of_rf_chans; this_channel++) {
		routine_addr = rf_iface_table[ this_button ].b.special_sub;
		if (routine_addr != NULL)
		  ival = (*routine_addr)( UPDATE_PARAM, this_channel );
	}
}


int
update_rf_conpar()
{
	int	iter, number_of_rf_chans;

	number_of_rf_chans = num_rf_chans_selected();

	for (iter = 0; iter < NUM_RF_BUTTONS; iter++)
	  switch (rf_iface_table[ iter ].type_of_entry) {

		case TABLE_OF_CHAR:
			update_rf_char_conpar( iter, number_of_rf_chans );
			break;

		case TABLE_OF_DOUBLE:
			update_rf_double_conpar( iter, number_of_rf_chans );
			break;

		case SPECIAL_ENTRY:
			update_rf_special_conpar( iter, number_of_rf_chans );
			break;

		default:
			break;
	  }
}
#endif

#ifdef SUN

int
set_upper_limit_in_obj( RF_chan_index )
{
	int	b_index;
	char	quick_str[ 14 ];

	b_index = RF_chan_index * NUM_RF_BUTTONS + UPPER_LIMIT;
	sprintf( &quick_str[ 0 ], "%d", upper_limit_value[ RF_chan_index ] );
	set_text_in_panel_text_input(
		RF_chan_index + FIRST_RF_PANEL_INDEX, b_index, &quick_str[ 0 ]
	);
	return( 0 );
}

static int
is_an_integer( text )
char *text;
{
	int	iter, len;

	len = strlen( text );
	if (len < 1)
	  return( 0 );

	for (iter = 0; iter < len; iter++)
	  if ( isdigit( text[ iter ] ) == 0)
	    return( 0 );

	return( 131071 );
}

static int
upper_limit_from_text( RF_chan_index, ul_text, ul_int )
int RF_chan_index;
char **ul_text;
int *ul_int;
{
	int	 b_index, c_value, l_limit, retval, u_limit, used, value, value_ok;

	b_index = RF_chan_index * NUM_RF_BUTTONS + UPPER_LIMIT;
	c_value = (int) (get_current_cattn_val( RF_chan_index ));

	if (c_value == 79) {
		l_limit = -16;
		u_limit = 63;
	}
	else if (c_value == 127) {
		l_limit = 0;
		u_limit = 127;
	}
	else {
		if ((RF_chan_index == 1) && 
			(get_current_amptype( RF_chan_index ) == 'c'))
		   u_limit = 255;
		else 
		   u_limit = 63;
		l_limit = 0;
	}
	*ul_int = u_limit;	/* Upper limit constrained value. */

/*  Rules for interpeting the text:
      If nothing entered, then let atoi return 0 and use this as the default
      If text entered, it must have only digits (enforced by is an integer)
      The value must be within the limited computed immediately above.		*/

	if (strlen( ul_text ) < 1)
	  value_ok = 1;
	else
	  value_ok = is_an_integer( ul_text );
	if (value_ok)
	  value = atoi( ul_text );

	if (value_ok && (value >= l_limit && value <= u_limit)) {
		used = 1;
		retval = 0;
	}
	else {

	/*  If the value from the Graphical User Interface is
	    not valid, substitute a value of u_limit -13 as a default.	*/

		value = u_limit - 13;
		used = 0;
		retval = value;
	}

	upper_limit_value[ RF_chan_index ] = value;
	upper_limit_used[ RF_chan_index ]  = used;

	return( retval );
}

int
upper_limit_from_obj( RF_chan_index, ul_int )
int RF_chan_index;
int *ul_int;
{
	int	 RF_panel_index, tflag;
	char	*ul_text;

	RF_panel_index = RF_chan_index + FIRST_RF_PANEL_INDEX;
	tflag = get_text_from_panel_text_input( RF_panel_index, UPPER_LIMIT, &ul_text, ul_int );
	if (tflag != 0)
	  return( -1 );
	else
	  return( upper_limit_from_text( RF_chan_index, ul_text, ul_int ) );
}

static int
fix_RF_step_size( this_channel )
int this_channel;
{
	int	label_index, stepsize_index;

	label_index = check_for_syn( this_channel );
	if (label_index == is_synthesizer_present[ this_channel ])
	  return( 0 );

/*  Difference is in the Panel Label; the Menu Choices remain the same.  */

	set_label_for_panel(
		  this_channel + FIRST_RF_PANEL_INDEX,
		  STEPSIZE,
		  stepsize_label[ label_index ]
	);
	is_synthesizer_present[ this_channel ] = label_index;
	return( 0 );
}

static int
rfpanel_notify_choice_proc( pindex, bindex, choice )
int pindex;
int bindex;
int choice;
{
	int	RF_chan_index, RF_choice_array_offset;

	RF_chan_index = pindex - FIRST_RF_PANEL_INDEX;
	if (RF_chan_index < 0 || RF_chan_index >= CONFIG_MAX_RF_CHAN)
	  return( -1 );
	RF_choice_array_offset = RF_chan_index * NUM_RF_BUTTONS;

	switch (bindex) {
	  case NAME_OF_CHAN:
	  case RF_PANEL_MSG:
		break;

	  case LATCHING:
	  case OVERRANGE:
	  case STEPSIZE:
	  case COARSE_ATTN:
	  case FINE_ATTN:
	  case WAVE_FORM_GEN:
	  case TYPE_OF_AMP:
		RF_chan_choice[ RF_choice_array_offset + bindex ] = choice;
		break;

	  case TYPE_OF_RF:
               config_setstring( "rfchtype", rf_desc[ choice ],
                                  RF_chan_index+1);
	  case TYPE_OF_SYN:
		RF_chan_choice[ RF_choice_array_offset + bindex ] = choice;
		fix_RF_step_size( RF_chan_index );
		break;

	  default:
		fprintf( stderr, "Unknown button in dispatch RF choice\n" );
		break;
	}

	return( 0 );
}

static int
rfpanel_notify_text_proc( pindex, bindex, text )
int pindex;						/* GUI-independent panel index */
int bindex;
char *text;
{
	int	ival,u_limit;
	char	 quick_str[ RF_PANEL_MESSAGE_LEN ];

	if (bindex != UPPER_LIMIT)
	  return( -1 );

	set_message_in_panel( pindex, RF_PANEL_MSG, "" );
	ival = upper_limit_from_text( pindex - FIRST_RF_PANEL_INDEX, 
							text, &u_limit );
	if (ival != 0) {
		sprintf( &quick_str[ 0 ],
		    "Please enter a value between 0 and %3d", u_limit );
		set_message_in_panel( pindex, RF_PANEL_MSG,
			quick_str );
		config_bell();
	}

	return( ival );
}

/************************************************************************/
/*	Make the RF Panel						*/
/************************************************************************/

/*  If the values for the RF Channel parameters are not supported, the
    corresponding position in the `gradient_choice' array will contain
    -1.  This is OK if CONFIG is just displaying current values, but is
    not OK if CONFIG is running in interactive mode.  Thus this routine
    to fix the RF Channel choices, called by Make RF Channel Panel.

    When the GUI stuff was abstracted and separated, this program
    changed to fix only one channel's worth of choices per call.	*/

static int
fix_rf_chan_choices( this_channel )
int this_channel;
{
	int	choice, choice_index, this_button;

	for (this_button = 0; this_button < NUM_RF_BUTTONS; this_button++) {
		choice_index = this_channel*NUM_RF_BUTTONS + this_button;
		choice = RF_chan_choice[ choice_index ];
		if (choice < 0)
		  choice = 0;

		RF_chan_choice[ choice_index ] = choice;
	}

	return( 0 );
}

int
makeRFPanel( panel_index )
int panel_index;
{
	int		 iter;
	char  		*menu_choices[ MAX_NUM_CHOICES+1 ];

/*  iter is in the range [ 0 ... CONFIG_MAX_RF_CHAN )	*/

	iter = panel_index - FIRST_RF_PANEL_INDEX;

	/*  Set the current synthesizer choice to NOT_PRESENT, as we
	    make the step size label assuming no synthesizer is present.
	    Once this panel is sized, fix the step size to conform to
	    the actual synthesizer configuration.			*/

	is_synthesizer_present[ iter ] = NOT_PRESENT;

	menu_choices[ 0 ] = rf_chan_name[ iter ];
	menu_choices[ 1 ] = NULL;
	config_panel_choice_item(
		 panel_index,
		 NAME_OF_CHAN,			/* button index */
		 rf_label[ NAME_OF_CHAN ],	/* button label */
		(char *) NULL,			/* PANEL_NOTIFY_PROC */
		 0,				/* initial choice (index) */
		&menu_choices[ 0 ]
	);

		    /* Type of RF */

	config_panel_choice_item(
		 panel_index,
		 TYPE_OF_RF,
		 rf_label[ TYPE_OF_RF ],
		(char *) rfpanel_notify_choice_proc,
		 RF_chan_choice[ iter*NUM_RF_BUTTONS+TYPE_OF_RF ],
		&rf_choice[ 0 ]
	);

		    /* Type of Synthesizer */

	config_panel_choice_item(
		 panel_index,
		 TYPE_OF_SYN,
		 rf_label[ TYPE_OF_SYN ],
		(char *) rfpanel_notify_choice_proc,
		 RF_chan_choice[ iter*NUM_RF_BUTTONS+TYPE_OF_SYN ],
		&pts_choice[ 0 ]
	);

		    /* Is the phase constant over a large frequency range? */

	config_panel_choice_item(
		 panel_index,
		 OVERRANGE,
		 rf_label[ OVERRANGE ],
		(char *) rfpanel_notify_choice_proc,
		 RF_chan_choice[ iter*NUM_RF_BUTTONS+OVERRANGE ],
		&overrange_choice[ 0 ]
	);

		    /* Latching */

	config_panel_choice_item(
		 panel_index,
		 LATCHING,
		 rf_label[ LATCHING ],
		(char *) rfpanel_notify_choice_proc,
		 RF_chan_choice[ iter*NUM_RF_BUTTONS+LATCHING ],
		&present_not_present[ 0 ]
	);

		    /* Step size */

	panel_choices_from_table_of_double(
		&menu_choices[ 0 ],
		" %g Hz ",
		&stepsize_value[ 0 ],
		 sizeof( stepsize_value ) / DSIZE
	);
	config_panel_choice_item(
		 panel_index,
		 STEPSIZE,
		 rf_label[ STEPSIZE ],
		(char *) rfpanel_notify_choice_proc,
		 RF_chan_choice[ iter*NUM_RF_BUTTONS+STEPSIZE ],
		&menu_choices[ 0 ]
	);
	free_table_of_memory( &menu_choices[ 0 ] );

		    /* Presence of Coarse Attenuator */

	config_panel_choice_item(
		 panel_index,
		 COARSE_ATTN,
		 rf_label[ COARSE_ATTN ],
		(char *) rfpanel_notify_choice_proc,
		 RF_chan_choice[ iter*NUM_RF_BUTTONS+COARSE_ATTN ],
		&cattn_choice[ 0 ]
	);

	/*  The Upper Limit for the Coarse Attenuator is special, for
	    the user enters a number rather than selecting a value. 	*/

	config_panel_text_input_item(
		 panel_index,
		 UPPER_LIMIT,
		 rf_label[ UPPER_LIMIT ],
		(char *) rfpanel_notify_text_proc
	);
	if (upper_limit_used[ iter ] != 0) {

		char	quick_str[ 14 ];			/* room for %d */

		sprintf( &quick_str[ 0 ], "%d", upper_limit_value[ iter ] );
		set_text_in_panel_text_input(
			 panel_index,
			 UPPER_LIMIT,
			&quick_str[ 0 ]
		);
	}

		    /* Presence of Fine Attenuator */

	config_panel_choice_item(
		 panel_index,
		 FINE_ATTN,
		 rf_label[ FINE_ATTN ],
		(char *) rfpanel_notify_choice_proc,
		 RF_chan_choice[ iter*NUM_RF_BUTTONS+FINE_ATTN ],
		&present_not_present[ 0 ]
	);

		    /* Waveform Generator */

	config_panel_choice_item(
		 panel_index,
		 WAVE_FORM_GEN,
		 rf_label[ WAVE_FORM_GEN ],
		(char *) rfpanel_notify_choice_proc,
		 RF_chan_choice[ iter*NUM_RF_BUTTONS+WAVE_FORM_GEN ],
		&present_not_present[ 0 ]
	);

		    /* Type of Amplifier */

	if (channel_can_share_amplifier( iter ))
	  amptype_choice[ INDEX_FOR_SHARED ] = &shared_entry[ 0 ];
	else
	  amptype_choice[ INDEX_FOR_SHARED ] = NULL;

	config_panel_choice_item(
		 panel_index,
		 TYPE_OF_AMP,
		 rf_label[ TYPE_OF_AMP ],
		(char *) rfpanel_notify_choice_proc,
		 RF_chan_choice[ iter*NUM_RF_BUTTONS+TYPE_OF_AMP ],
		&amptype_choice[ 0 ]
	);

	config_panel_message(
		 panel_index,
		 RF_PANEL_MSG,
		 RF_PANEL_MESSAGE_LEN
	);

	fix_RF_step_size( panel_index - FIRST_RF_PANEL_INDEX );
}
#endif

/************************************************************************/

static int
display_one_rf_chan( outf, this_channel )
FILE *outf;
int this_channel;
{
	int		 choice_index, this_button, this_choice;
	char		 quick_str[ 52 ];
	char		*quick_addr;
	extern char	*find_first_nonblank();

/*  Fix up the amplifier choice table because this program also uses it to
    help display the type of amplifier.  Prevents segmented violations...	*/

	if (channel_can_share_amplifier( this_channel ))
	  amptype_choice[ INDEX_FOR_SHARED ] = &shared_entry[ 0 ];
	else
	  amptype_choice[ INDEX_FOR_SHARED ] = NULL;

	fprintf( outf, "%s:\n", rf_chan_name[ this_channel ] );

	for (this_button = 0; this_button < NUM_RF_BUTTONS; this_button++) {

	/*  The Name of the RF Channel has just been shown;
	    do not display the RF Panel Message.		*/

		if (this_button == NAME_OF_CHAN ||
		    this_button == RF_PANEL_MSG)
		  continue;

	/*  Indent 4 spaces when making the label.  */

		strcpy( &quick_str[ 0 ], "    " );
		strcat( &quick_str[ 0 ], rf_label[ this_button ] );
		strcat( &quick_str[ 0 ], ":" );

	/*  Format specifier '%-40s" pushes the string to the left and appends
	    blank characters to the right for a total length of 40 characters.  */

		fprintf( outf, "%-40s", &quick_str[ 0 ] );

		choice_index = this_channel*NUM_RF_BUTTONS + this_button;
		this_choice = RF_chan_choice[ choice_index ];
		if (this_choice < 0) {
			fprintf( outf, "unknown\n" );
			continue;
		}

		switch (this_button) {

		  case TYPE_OF_RF:
			quick_addr = rf_choice[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case TYPE_OF_SYN:
			quick_addr = pts_choice[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case LATCHING:
			quick_addr = present_not_present[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case OVERRANGE:
			quick_addr = overrange_choice[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case STEPSIZE:
			fprintf( outf, "%g Hz", stepsize_value[ this_choice ] );
			break;

		  case COARSE_ATTN:
			quick_addr = cattn_choice[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case UPPER_LIMIT:
			if (upper_limit_used[ this_channel ] == 0)
			  fprintf( outf, "Not Used" );
			else
			  fprintf( outf, "%d dB", upper_limit_value[ this_channel ] );
			break;

		  case FINE_ATTN:
			quick_addr = present_not_present[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case WAVE_FORM_GEN:
			quick_addr = present_not_present[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  case TYPE_OF_AMP:
			quick_addr = amptype_choice[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ) );
			break;

		  default:
			break;
		}

		fprintf( outf, "\n" );
	}
	fprintf( outf, "\n" );
}

int
display_rf_params( outf )
FILE *outf;
{
	int	number_of_rf_chans, this_channel;

	number_of_rf_chans = num_rf_chans_selected();
	for (this_channel = 0; this_channel < number_of_rf_chans; this_channel++)
	  display_one_rf_chan( outf, this_channel );
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
  console_interface[ NUM_RF_BUTTONS ][ CONFIG_MAX_RF_CHAN ];

static int
init_console_table()
{
	int	chan_index;

	for (chan_index = 0; chan_index < CONFIG_MAX_RF_CHAN; chan_index++) {
		console_interface[ NAME_OF_CHAN ][ chan_index ].item_type = NO_TYPE;
		console_interface[ TYPE_OF_RF ][ chan_index ].item_type = CHAR;
		console_interface[ TYPE_OF_SYN ][ chan_index ].item_type = DOUBLE;
		console_interface[ LATCHING ][ chan_index ].item_type = CHAR;
/*
		console_interface[ OVERRANGE ][ chan_index ].item_type = CHAR;
*/
		console_interface[ OVERRANGE ][ chan_index ].item_type = DOUBLE;
		console_interface[ STEPSIZE ][ chan_index ].item_type = DOUBLE;
		console_interface[ COARSE_ATTN ][ chan_index ].item_type = DOUBLE;
		console_interface[ UPPER_LIMIT ][ chan_index ].item_type = NO_TYPE;
		console_interface[ FINE_ATTN ][ chan_index ].item_type = DOUBLE;
		console_interface[ WAVE_FORM_GEN ][ chan_index ].item_type = CHAR;
		console_interface[ TYPE_OF_AMP ][ chan_index ].item_type = CHAR;
		console_interface[ RF_PANEL_MSG ][ chan_index ].item_type = NO_TYPE;
	}
}

int
getRFPanelsFromConsole()
{
	char	cval;
	int	chan_index, item_index, item_type, ival, panel;
	double	dval;

	for (
	  panel = FIRST_RF_PANEL_INDEX, chan_index = 0;
	  panel <= LAST_RF_PANEL_INDEX;
	  panel++, chan_index++
	)
	  for (item_index = 0; item_index < NUM_RF_BUTTONS; item_index++) {
		console_interface[ item_index ][ chan_index ].value_is_valid = 0;
		item_type = console_interface[ item_index ][ chan_index].item_type;
		if (item_type != CHAR && item_type != DOUBLE)
		  continue;

		if (item_type == DOUBLE) {
			ival = getConfigDoubleParameter(
				panel, item_index, &dval
			);
			if (ival != 0) {
				console_interface[ item_index ][ chan_index ]
					.value_is_valid = 0;
			}
			else {
				console_interface[ item_index ][ chan_index ]
					.dval = dval;
				console_interface[ item_index ][ chan_index ]
					.value_is_valid = 131071;
			}
		}
		else {			/* item_type == CHAR */
			ival = getConfigCharParameter(
				panel, item_index, &cval
			);
			if (ival != 0) {
				console_interface[ item_index ][ chan_index ]
					.value_is_valid = 0;
			}
			else {
				console_interface[ item_index ][ chan_index ]
					.cval = cval;
				console_interface[ item_index ][ chan_index ]
					.value_is_valid = 131071;
			}
		}
	  }
}


int
updateRFPanelsUsingConsole()
{
	int	chan_index, choice_index, item_index;
	int	item_type, new_choice, panel, type_of_entry;
	int	check_fixed_freq;

	for (
	  panel = FIRST_RF_PANEL_INDEX, chan_index = 0;
	  panel <= LAST_RF_PANEL_INDEX;
	  panel++, chan_index++
	)
	  for (item_index = 0; item_index < NUM_RF_BUTTONS; item_index++) {
		item_type = console_interface[ item_index ][ chan_index ].item_type;
		if (item_type != CHAR && item_type != DOUBLE)
		  continue;

		if (console_interface[ item_index ][ chan_index ].value_is_valid == 0)
		  continue;

	/*  item_type == CHAR    ===> type_of_entry == TABLE_OF_CHAR
	    item_type == DOUBLE  ===> type_of_entry == TABLE_OF_DOUBLE	*/

		type_of_entry = rf_iface_table[ item_index ].type_of_entry;
		if ((item_type == CHAR   && type_of_entry != TABLE_OF_CHAR) ||
		    (item_type == DOUBLE && type_of_entry != TABLE_OF_DOUBLE))
			continue;

		if (item_type == CHAR) {
			new_choice = find_table_char(
				console_interface[ item_index ][ chan_index ].cval,
				rf_iface_table[ item_index ].a.ctable_addr,
				rf_iface_table[ item_index ].table_size
			);
		}
		else {				/* item_type == DOUBLE */
			new_choice = find_table_double(
				console_interface[ item_index ][ chan_index ].dval,
				rf_iface_table[ item_index ].a.dtable_addr,
				rf_iface_table[ item_index ].table_size
			);
		}

/* do something like:
   if (item_type = TYPE_OF_RF ) then get SF_OFFSET and add 1 to new_choice 
   if the bit is set
*/
/* To accomodate the H1 Only rftype, we check if a bit is set in the */
/* nvram. This is not really a choice on the rf-panel, so we cannot  */
/* deal with it as part of the for-loop. The rf_value-s specify 'd'  */
/* for both the full PTS and H1-Only PTS, so we have to do it a      */
/* funny way here. Refering to the rftype H1-Only as 'e' would have  */
/* made life easier all around  (including PSG) */
		if (item_index == TYPE_OF_RF)
		{	check_fixed_freq = get_sfbits();

			if (check_fixed_freq & (1<<chan_index) )
			{  new_choice++;
			}
		}

		if (new_choice >= 0) {
			choice_index = item_index+chan_index*NUM_RF_BUTTONS;
			RF_chan_choice[ choice_index ] = new_choice;
			set_choice_in_panel_choice( panel, item_index, new_choice );
		}
	  }
}


/*  Programs to write values to the (Hydra) console  */

storeRFPanelsInConsole()
{
	int	chan_index;
	int	choice, choice_index, item_index;
	int	ival, panel, type_of_entry;
	int	set_fixed_bits;
	double	dval;

/*  Only work with the configured number of RF channels  */
	set_fixed_bits=0;

	for (
	  chan_index = 0, panel = FIRST_RF_PANEL_INDEX;
	  chan_index < num_rf_chans_selected();
	  chan_index++, panel++
	)
	{
	   choice_index = TYPE_OF_RF+chan_index*NUM_RF_BUTTONS;
	   if (RF_chan_choice[ choice_index ] == 1)
		set_fixed_bits |= (1<<chan_index);

	  for (item_index = 0; item_index < NUM_RF_BUTTONS; item_index++) {
		type_of_entry = rf_iface_table[ item_index ].type_of_entry;
		choice_index = chan_index*NUM_RF_BUTTONS + item_index;
		choice = RF_chan_choice[ choice_index ];

		if (type_of_entry == TABLE_OF_DOUBLE) {
			dval = rf_iface_table[ item_index ].a.dtable_addr[ choice ],
			ival = setConfigDoubleParameter(
				panel, item_index, dval
			);
		}
	  }
	}
	/* save the set_fixed_bits to Sram */
	set_sfbits(set_fixed_bits);
}
