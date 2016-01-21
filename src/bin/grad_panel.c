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
#include  <string.h>
#include  <dirent.h>

#include  "vnmrsys.h"			/*  VNMR include file */
#include  "group.h"			/*  VNMR include file */
#include  "vconfig.h"
#include  "interface.h"

#include "grad_panel.h"

static int		 gradient_choice[ NUM_GRADIENT_BUTTONS ];
static char		*grad_label[ NUM_GRADIENT_BUTTONS ];
static struct param_interface	grad_iface_table[ NUM_GRADIENT_BUTTONS ];

/*  Display type of gradients; select corresponding value from gradient
    value.  Do not exceed MAX_NUM_CHOICES entries in either array; else
    the program will violate its segments, write a core file and halt.	*/

/* HIDE the PFG+WFG option */
static char		*type_of_gradients[] = {
	" None ",
	" WFG + GCU ",
	" Gradient Coordinate Rotator ",
	/*
	" PFG + WFG ",
	*/
	" Performa I ",
	" Performa II/III ",
	" Performa II/III + WFG ",
	" Performa IV ",
	" Performa IV + WFG ",
	" Performa XYZ ",
	" Performa XYZ + WFG ",
	" SIS (12 bit) ",
	" Homospoil ",
/*	" Shim DAC ", */
	 NULL
};
static char	gradient_value[] = { 'n', 'w', 'r', 'l', 'p', 'q', 'c', 'd', 't', 'u', 's', 'h', 'a' };
static double	gradient_stepsz[] = {
	0.0,
	32767.0,
	32767.0,
	2047.0,
	32767.0,
	32767.0,
	32767.0,
	32767.0,
	32767.0,
	32767.0,
	2047.0,
	32767.0,
/*	65535.0, */
};

static char     *sysgcoil_choices[MAXnGCOIL+1];
static char	sysgcoil[GCOILmaxnchar];
static int	ncoilchoices;

/************************************************************************/

static int
init_grad_labels()
{
	grad_label[ X_GRADIENT ] = "X Axis";
	grad_label[ Y_GRADIENT ] = "Y Axis";
	grad_label[ Z_GRADIENT ] = "Z Axis";
	grad_label[ GRAD_COIL ]  = "System Gradient Coil";
}

static int
init_interface_table()
{
	grad_iface_table[X_GRADIENT].type_of_entry = TABLE_OF_CHAR;
	grad_iface_table[X_GRADIENT].a.ctable_addr = gradient_value;
	grad_iface_table[X_GRADIENT].table_size    = sizeof(gradient_value);

	grad_iface_table[Y_GRADIENT].type_of_entry = TABLE_OF_CHAR;
	grad_iface_table[Y_GRADIENT].a.ctable_addr = gradient_value;
	grad_iface_table[Y_GRADIENT].table_size    = sizeof(gradient_value);

	grad_iface_table[Z_GRADIENT].type_of_entry = TABLE_OF_CHAR;
	grad_iface_table[Z_GRADIENT].a.ctable_addr = gradient_value;
	grad_iface_table[Z_GRADIENT].table_size    = sizeof(gradient_value);
}

int
init_grad_choices()
{
	int	iter, param_defined, result;
	char	gradient_buf[ NUM_GRADIENTS+1 ];

	init_interface_table();
	init_grad_labels();
	init_console_table();
	param_defined = 1;

/*  Special note on P_getstring:

    The last argument tells P_getstring the total size of the buffer
    in the 3rd argument, including the terminating NULL character.
    So to get NUM_GRADIENTS non-null characters, you must tell it
    the buffer has space for one more character.			*/

	result = P_getstring(
		SYSTEMGLOBAL, "gradtype", &gradient_buf[ 0 ], 1, NUM_GRADIENTS+1
	);
	if (result != 0)
	  param_defined = 0;

	for (iter = 0; iter < NUM_GRADIENTS; iter++) {
		if (param_defined)
		  gradient_choice[ iter ] = find_table_char(
			 gradient_buf[ iter ],
			&gradient_value[ 0 ],
			 sizeof( gradient_value )
		  );
		else
		  gradient_choice[ iter ] = NOT_PRESENT;
	}

	/* Generate Gradient coil choices, always allocate None */

	if ( (sysgcoil_choices[0]= (char *)malloc(strlen("none")+1)) !=NULL)
		strcpy(sysgcoil_choices[0],"None") ;
	result = P_getstring(SYSTEMGLOBAL, "sysgcoil", &sysgcoil[0], 1,
                                                        GCOILmaxnchar);
	ncoilchoices = getsysgcoil_choices();
	if ( (result == 0) && strlen(sysgcoil) )
        {
	   gradient_choice[ GRAD_COIL ] = find_table_string(
			sysgcoil,
			&sysgcoil_choices[ 0 ],
			 ncoilchoices);
        }
	else
        {
           gradient_choice[ GRAD_COIL ] = NOT_PRESENT;
        }
	
}

set_grad_choice(choice,item)
char	choice;
int	item;
{
	gradient_choice[ item ] = find_table_char(
                         choice,
                        &gradient_value[ 0 ],
                         sizeof( gradient_value )
                  );
	set_choice_in_panel_choice( GRAD_PANEL_INDEX,	
				 item, gradient_choice[item] );
}

#ifdef SUN

/*  If the values for the Axial Gradient parameters are not supported,
    the corresponding position in the `gradient_choice' array will
    contain -1.  This is OK if CONFIG is just displaying current values,
    but is not OK if CONFIG is running in interactive mode.  Thus this
    routine to fix the gradient choices, called by Make Gradient Panel.  */

static int
fix_grad_choices()
{
	int	iter, size_limit, this_choice;

	for (iter = 0; iter < NUM_GRADIENT_BUTTONS; iter++) {
	  switch (iter) {
	    case X_GRADIENT:
	    case Y_GRADIENT:
	    case Z_GRADIENT:
		size_limit = sizeof( gradient_value );
		this_choice = gradient_choice[ iter ];
		if (this_choice < 0)
		  this_choice = 0;
		else if (this_choice > size_limit)
		  this_choice = size_limit-1;
		break;

	  case GRAD_COIL:
		size_limit = ncoilchoices;
		this_choice = gradient_choice[ iter ];
		if (this_choice < 0)
		  this_choice = 0;
		else if (this_choice > size_limit)
		  this_choice = size_limit-1;

		break;

	  default:
		printf("dispatch gradient called from unknown panel");
		break;
	  }
		gradient_choice[ iter ] = this_choice;
	}
}

int
update_grad_conpar()
{
	int		iter, r, size_limit, this_choice;
	double		gradstepsz;
	char		gradient_buf[ NUM_GRADIENTS+1 ];
	extern int	gradients_selected();
        char	        tmpstr[GCOILmaxnchar];

	if (gradients_selected() == 0) {
		strcpy( &gradient_buf[ 0 ], "nnn" );
		gradstepsz = 0.0;
	}
	else {
		size_limit = sizeof( gradient_value );
		for (iter = 0; iter < NUM_GRADIENTS; iter++) {
			this_choice = gradient_choice[ iter ];
			if (this_choice < 0)
			  this_choice = 0;
			else if (this_choice >= size_limit)
			  this_choice = size_limit-1;

			gradient_buf[ iter ] = gradient_value[ this_choice ];
			gradstepsz = gradient_stepsz[ this_choice ];
		}
	}
	gradient_buf[ NUM_GRADIENTS ] = '\0';
	r = config_setstring( "gradtype", &gradient_buf[ 0 ], 1 );
	r = config_setreal( "gradstepsz", gradstepsz, 1 );

	if (gradients_selected() == 0) {
		strcpy( &sysgcoil[ 0 ], "" );
	}
	else {
		size_limit = ncoilchoices;
		this_choice = gradient_choice[ GRAD_COIL ];
		if (this_choice >= size_limit)
		  this_choice = size_limit-1;
		if (this_choice < 0)
		  this_choice = 0;
		if (size_limit && sysgcoil_choices[this_choice]){
		    strcpy(sysgcoil, sysgcoil_choices[this_choice]);
		}else{
		    strcpy(sysgcoil, "");
		}
	}

	r = config_setstring( "sysgcoil", &sysgcoil[ 0 ], 1 );

	return( r );
}
#endif


/*  A Special Subroutine, as defined for the Interface Table.	*/
    
/*  The System (also called the Generic) Panel uses this to establish if
    any Axial Gradients are present, in conjunction with the AXIAL GRADIENT
    button.  Return 1 if so and the direction is SET CHOICE; return 0 if
    not or if the direction is UPDATE PARAM.				*/

int
gradients_present( direction )
int direction;
{
	int	iter;

	if (direction == UPDATE_PARAM)
	  return( 0 );
	if (direction != SET_CHOICE)
	  return( -1 );

	for (iter = 0; iter < NUM_GRADIENTS; iter++)
	  if (gradient_choice[ iter ] > NOT_PRESENT)
	    return( 1 );

	return( 0 );
}

#ifdef SUN
static
gradient_notify_choice_proc( pindex, bindex, choice )
int pindex;
int bindex;
int choice;
{
	switch (bindex) {
	  case X_GRADIENT:
	  case Y_GRADIENT:
	  case Z_GRADIENT:
	  case GRAD_COIL:
		gradient_choice[ bindex ] = choice;
		break;

	  default:
		printf("dispatch gradient called from unknown panel");
		break;
	}

	return( 0 );
}

makeGradPanel()
{
	fix_grad_choices();

	config_panel_choice_item(
		 GRAD_PANEL_INDEX,
		 X_GRADIENT,
		 grad_label[ X_GRADIENT ],
		(char *) gradient_notify_choice_proc,
		 gradient_choice[ X_GRADIENT ],
		&type_of_gradients[ 0 ]
	);

	config_panel_choice_item(
		 GRAD_PANEL_INDEX,
		 Y_GRADIENT,
		 grad_label[ Y_GRADIENT ],
		(char *) gradient_notify_choice_proc,
		 gradient_choice[ Y_GRADIENT ],
		&type_of_gradients[ 0 ]
	);

	config_panel_choice_item(
		 GRAD_PANEL_INDEX,
		 Z_GRADIENT,
		 grad_label[ Z_GRADIENT ],
		(char *) gradient_notify_choice_proc,
		 gradient_choice[ Z_GRADIENT ],
		&type_of_gradients[ 0 ]
	);

	config_panel_choice_item(
		 GRAD_PANEL_INDEX,
		 GRAD_COIL,
		 grad_label[ GRAD_COIL ],
		(char *) gradient_notify_choice_proc,
		 gradient_choice[ GRAD_COIL ],
		&sysgcoil_choices[ 0 ]
	);
}
#endif

/************************************************************************/

int
display_grad_params( outf )
FILE *outf;
{
	int		 this_button, this_choice;
	char		 quick_str[ 42 ];
	char		*quick_addr;
	extern char	*find_first_nonblank();

	fprintf( outf, "Axial Gradients\n" );
	for (this_button = 0; this_button < NUM_GRADIENT_BUTTONS; this_button++) {

	/*  Indent 4 spaces when making the label.  */

		strcpy( &quick_str[ 0 ], "    " );
		strcat( &quick_str[ 0 ], grad_label[ this_button ] );
		strcat( &quick_str[ 0 ], ":" );

	/*  Format specifier "%-40s" pushes the string to the left and appends
	    blank characters to the right for a total length of 40 characters.  */

		fprintf( outf, "%-40s", &quick_str[ 0 ] );

		this_choice = gradient_choice[ this_button ];

	  switch (this_button) {
	     case X_GRADIENT:
	     case Y_GRADIENT:
	     case Z_GRADIENT:

		if (this_choice < 0)
		  fprintf( outf, "unknown" );
		else {
			quick_addr = type_of_gradients[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ));
		}
		fprintf( outf, "\n" );
		break;

	     case GRAD_COIL:

		if (this_choice < 0)
		  fprintf( outf, "unknown" );
		else {
			quick_addr = sysgcoil_choices[ this_choice ];
			fprintf( outf, "%s", find_first_nonblank( quick_addr ));
		}
		fprintf( outf, "\n" );
		break;

	     default:
		printf("dispatch gradient called from unknown panel");
		break;
	   }
	}
	fprintf( outf, "\n" );
}

static int
getsysgcoil_choices() 
{
        int    i,ngcoil; 
        DIR    *dirp;
        struct dirent *dp;
        char   dirpath[MAXPATHL];
 
        strcpy(&dirpath[0], systemdir);
        strcat(&dirpath[0], "/imaging/gradtables");

        /* need to deallocate memory */
        for (i=1; i<=MAXnGCOIL; i++) {
                free(sysgcoil_choices[i]);
                sysgcoil_choices[i] = (char *)0;
        }
 
        ngcoil=1; i=1; 

        if (dirp = opendir(dirpath)) {
 
            for(dp=readdir(dirp);dp!=NULL&&i<MAXnGCOIL;dp=readdir(dirp)) {
                if( strcmp(dp->d_name,"."   ) != 0 && /* exclude these */
                    strcmp(dp->d_name,".."  ) != 0 ){
 		
                    if (strlen(dp->d_name) > (unsigned int) GCOILmaxnchar ) {
			printf("Filename \"%s\" has exceeded the number of allowed characters.",
							dp->d_name);
                    }
		    else if ( (sysgcoil_choices[i]=
			(char *)malloc(strlen(dp->d_name)+1)) !=NULL) {
			strcpy(sysgcoil_choices[i],dp->d_name) ;
			i++;
                    }
                    else {   
			printf("Request for memory failed.");
			return (0);
                    }
                }        
            }
            if ((ngcoil=i) > MAXnGCOIL) {
                printf("Exceeded maximum number of gradient tables!");
                return (ngcoil);
            }
 
            closedir(dirp);
 	}
	/* Check for no gradient tables and insert none */
        return (ngcoil);
}


/*  Programs to get data from console, display corresponding selections.  */

#define  NO_TYPE	0
#define  CHAR		(NO_TYPE+1)
#define  DOUBLE		(CHAR+1)
#define  SPECIAL	(DOUBLE+1)

static struct _console_interface {
    double dval;		/* Put 1st due to SPARC data alignment */
    int	item_type;
    int	value_is_valid;
    char cval;
}
console_interface[NUM_GRADIENT_BUTTONS];

static int
init_console_table()
{
    console_interface[X_GRADIENT].item_type = CHAR;
    console_interface[Y_GRADIENT].item_type = CHAR;
    console_interface[Z_GRADIENT].item_type = CHAR;
    console_interface[GRAD_COIL].item_type = SPECIAL;
}

int
getGradPanelFromConsole()
{
    char cval;
    int	item_index, item_type, ival, panel;
    double dval;

    panel = GRAD_PANEL_INDEX;
    for (item_index = 0; item_index < NUM_GRADIENT_BUTTONS; item_index++) {
	console_interface[item_index].value_is_valid = 0;
	item_type = console_interface[item_index].item_type;

	switch (item_type) {
	  case DOUBLE:
	    ival = getConfigDoubleParameter(panel, item_index, &dval);
	    if (ival == 0) {
		console_interface[item_index].dval = dval;
		console_interface[item_index].value_is_valid = 131071;
	    }
	    break;
	  case CHAR:
	    ival = getConfigCharParameter(panel, item_index, &cval);
	    if (ival == 0) {
		console_interface[item_index].cval = cval;
		console_interface[item_index].value_is_valid = 131071;
	    }
	}
    }
}

int
updateGradPanelUsingConsole()
{
    int	chan_index, item;
    int	itype, new_choice, panel, etype, valid;
    int	check_fixed_freq;

    panel = GRAD_PANEL_INDEX;
    for (item = 0; item < NUM_GRADIENT_BUTTONS; item++) {
	itype = console_interface[item].item_type;
	etype = grad_iface_table[item].type_of_entry;
	valid = console_interface[item].value_is_valid;

	new_choice = -1;
	if (valid && itype == CHAR && etype == TABLE_OF_CHAR) {
	    new_choice =
		find_table_char(console_interface[item].cval,
				grad_iface_table[item].a.ctable_addr,
				grad_iface_table[item].table_size);
	} else if (valid && itype == DOUBLE && etype == TABLE_OF_DOUBLE) {
	    new_choice =
		find_table_double(console_interface[item].dval,
				  grad_iface_table[item].a.dtable_addr,
				  grad_iface_table[item].table_size);
	}

	if (new_choice >= 0) {
	    gradient_choice[item] = new_choice;
	    set_choice_in_panel_choice( panel, item, new_choice );
	}
    }
}
