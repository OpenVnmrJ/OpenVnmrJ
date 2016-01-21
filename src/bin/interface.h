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

/*  interface between VNMR config and the system global parameters
    interface between GUI-independent programs and the GUI-dependent support	*/

/*  The CONFIG program reads the "conpar" file and obtains the values of
    various global parameters.  These then define the initial choices for
    the panel items, or buttons, which the CONFIG program displays.  These
    choices are usually small integers, 0, 1, 2, up to the number of choices
    available in that panel item.  When CONFIG exits, it may write a new
    version of "conpar", using the choices the user made.

    The `param_interface' data structure helps define the interface between
    the "conpar" parameters and the panel items.  Each button has a param
    interface structure associated with it.  The type of entry field
    determines its function.

    NULL_ENTRY		Has no special function.  Included more as a
			place-holder for now.

    INDEX_ENTRY		The parameter is a real number with a value of
			1, 2, 3, up to the limit of index.  Thus the
			choice is 0, 1, 2; i. e. the parameter value
			minus 1.

    TABLE_OF_CHAR	The parameter is a Character (actually a String
			with 1 Character.  Expected characters values
			are stored in a table.  The choice is the index
			into this character table.  Since in general
			these Tables are NOT terminated with a NULL,
			never call `strlen' uisng the address of this
			table.  The size of the table is available in
			the  `table_size' field.

    TABLE_OF_DOUBLE	The parameter is a real number.  Expected values
			are stored in the table.  The choice is the index
			into this table.

    SPECIAL_ENTRY	A separate subroutine defines the interface.  See
			below for more information on this type of entry,
			including a description of the Argument List.

Special Note 1:  If the parameter value in CONPAR is not found in the designed
		 table, the corresponding choice is set to -1.  Thus if CONFIG
		 is running in display mode, it can report these parameters as
		 having unknown values.  But if CONFIG is running interactively,
		 the routine to create the panel should eliminate any choice
		 values of -1.

Special Note 2:  For Line Item Panels, the parameter associated with a Table
		 of Characters will be a string with one character for each
		 Line Item.  For example, the `rftype' parameter is a string
		 with 3 characters, since the software allows configuring up
		 to 3 RF channels.  The parameter associated with a Table of
		 Double will by arrayed with the number of entries defined by
		 the number of line items.

Special Note 3:  The param_index allows one to target a specific entry when
                 the corresponding parameter is an array.  SCALER implies the
		 parameter is not an array.  LOOKUP_INDEX is a special case -
		 see below.  The default choice was introduced along with the
		 IF frequency in the system panel.  It allows the program to
		 select a default choice (NOT the parameter value!) different
		 from 0, which was required for IF frequency.  All other items
		 default to 0.  Default choice has benn implemented in the
		 system panel only; the other panels currently can all defualt
		 to an initial choice of 0.					*/

#define  NULL_ENTRY	0
#define  INDEX_ENTRY	1
#define  TABLE_OF_CHAR	2
#define  TABLE_OF_DOUBLE	3
#define  SPECIAL_ENTRY	4

/*  Sun's version of C (probably Kernigan & Ritche's version too) does not
    allow anonymous unions.  Thus the trivial names for the two unions here.	*/

struct param_interface {
	int	type_of_entry;
	union {
		char	*ctable_addr;		/* For TABLE OF CHAR */
		double	*dtable_addr;		/* For TABLE OF DOUBLE */
		int	 limit_of_index;	/* For INDEX ENTRY */
	} a;
	union {
		int	(*special_sub)();	/* For SPECIAL ENTRY */
		char	*param_name;		/* all other entry types */
	} b;
	int	table_size;			/* For table entries only */
	int	param_index;			/* For all except SPECIAL ENTRY */
	int	default_choice;			/* For all except SPECIAL ENTRY */
};

#define  DSIZE		(sizeof( double ))
#define  SCALER		0
#define  LOOKUP_INDEX	-1

/*  the RF panels use LOOKUP_INDEX since each indexed parameter has a
    different index for each panel.  Rather than create multiple copies
    of the parameter interface table - the other fields are identical
    for each panel - this special value for the index reminds the
    programs the index must be looked up in a per-panel table.

    The RF panels can only have param_index set to SCALER or LOOKUP_INDEX.
    This is a consequence of the fact that there is only one interface
    table for all the panels.  See the rf panel program for more.	*/

/*  Special Subroutines expect 1 or 2 arguments.  The first defines the
    direction of data flow:
	CONPAR to Panel Choice	=>	SET_CHOICE
	Panel Choice to CONPAR	=>	UPDATE_PARAM

    When called with SET_CHOICE, the routine is expected to return the
    correct index for the Panel Choice Value it is responsible for.

    When called with UPDATE PARAM, it is expected to update the CONPAR
    parameters it is responsible for and return 0 if successful.

    Special Subroutines for Line Item Panels expect a 2nd argument
    giving the index of the line item.					*/

#define  SET_CHOICE	1
#define  UPDATE_PARAM	2

/*  Define indexes for GUI-independent panels  */

#define SYSTEM_PANEL_INDEX	0
#define FIRST_RF_PANEL_INDEX	(SYSTEM_PANEL_INDEX+1)
#define LAST_RF_PANEL_INDEX	(FIRST_RF_PANEL_INDEX+CONFIG_MAX_RF_CHAN-1)
#define GRAD_PANEL_INDEX	(LAST_RF_PANEL_INDEX+1)
#define NUM_GRADIENT_PANELS	1
#define NUM_PANELS		(GRAD_PANEL_INDEX+1)

/*  The VNMR config program has a System Panel, RF Channel Panels and a
    Gradient Panel.  The System Panel is part of the permenent panel set
    and is always shown.  The rest are Line Item Panels and only one is
    shown at a time.  A Configuration Menu allows one to select which
    Line Item Panel will appear.

    For list of panel items, see system_panel.h, rf_panel.h, grad_panel.h
    (Note:  A button is a subset of panel item)

    To add a new panel item, you make up a name for it and install it in
    the appropriate header file.  Then extend the corresponding C program
    program to create the kind of panel item you want.  This will require
    making up a label and identifying a callback program.  Unless
    compelling reasons exist, you should use the callback program which
    is already present for the type of panel item you are creating.  Make
    sure input with this new panel item will receive a desired response.
    You should also extend the init_XXX_choices and update_XXX_conpar
    programs.  Finally you will need to prepare an interface table entry.

    One can have:
		Panel Choice (traditional button)
		Panel Text Input (Lock Frequency)
		Panel Message

    The Panel Message does not respond to selection or other input and thus
    does not have a callback program.  Programs exist to set the message for
    Panel Message, to set the label for Panel Choice or Panel Text Input,
    to set the text for Panel Text Input and to read the current input (or
    choice) for Panel Choice and Text Input.

    It is best to have only one Panel Message per panel and to keep it the
    last item in the panel.  For appearance, the Panel message is spaced
    down one extra half row from the item above it.			*/
