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
#include  <ctype.h>
#include  <string.h>

#include  "vnmrsys.h"			/*  VNMR include file */
#include  "group.h"			/*  VNMR include file */
#include  "symtab.h"			/*  VNMR include file */
#include  "variables.h"			/*  VNMR include file */

#include  "vconfig.h"


/************************************************************************/
/*	find_null_addr( char **array_addr, int limit )			*/
/*									*/
/*	panel_choices_from_limit_of_index(				*/
/*		char *choice_array[];					*/
/*		char *fmt;						*/
/*		int limit_of_index;					*/
/*	)								*/
/*									*/
/*	panel_choices_from_table_of_double(				*/
/*		char *choice_array[];					*/
/*		char *fmt;						*/
/*		double *table_of_double;				*/
/*		int table_size;						*/
/*	)								*/
/*									*/
/*	free_table_of_memory( char *table_addr[] )			*/
/*									*/
/*	table_of_double_from_limit(					*/
/*		double *table_of_double;				*/
/*		double first_value;					*/
/*		int limit;						*/
/*	)								*/
/*									*/
/*	find_index_entry( double value, int limit )			*/
/*									*/
/*	find_table_char(						*/
/*		char value,						*/
/*		char *table_addr,					*/
/*		int table_size						*/
/*	)								*/
/*									*/
/*	find_table_string(						*/
/*		char *value,						*/
/*		char *table_addr[],					*/
/*		int table_size						*/
/*	)								*/
/*									*/
/*	find_table_double(						*/
/*		double value,						*/
/*		double *table_addr,					*/
/*		int table_size						*/
/*	)								*/
/*									*/
/*	cmp_double( double x1, double x2 )				*/
/*									*/
/*	set_size_array_real( char *name, int new_size )			*/
/*									*/
/*	config_setreal( char *name, double value, int index )		*/
/*									*/
/*	config_setstring( char *name, char *value, int index )		*/
/*									*/
/*	set_array_real_using_table(					*/
/*		char *name,						*/
/*		double *table_of_real,					*/
/*		int table_size,						*/
/*		int *table_of_choices,					*/
/*		int number_choices					*/
/*	)								*/
/*									*/
/*	set_element_array_real_using_table(				*/
/*		char *param_name,					*/
/*		int param_index,					*/
/*		int choice,						*/
/*		double *table_of_real,					*/
/*		int table_size						*/
/*	)								*/
/*									*/
/*	get_choice_using_element_real_array(				*/
/*		char *param_name,					*/
/*		int param_index,					*/
/*		double *table_of_real,					*/
/*		int table_size						*/
/*	)								*/
/*									*/
/*	find_first_nonblank( char *a_string )				*/
/************************************************************************/


int
find_null_addr( array_addr, limit )
char **array_addr;
int limit;
{
	int	iter;

	iter = 0;
	while (array_addr[ iter ] != NULL) {
		if (iter >= limit)
		  return( -1 );			/* means it was not found */
		else
		  iter++;
	}

	return( iter );
}


/*  These two routines make a Table of Choices for a Panel Item (an array
    of string addresses), given either a limit for an index or a table of
    floating point (double precision).  For the Limit of Index, the choices
    are "1", "2", ... up to the limit.  The strings are allocated from the
    heap.  Use `free_table_of_memory' to deallocate this memory.	  */

int
panel_choices_from_limit_of_index( choice_array, fmt, limit_of_index )
char *choice_array[];
char *fmt;
int limit_of_index;
{
	int		 iter, length, value;
	char		 quick_buf[ MAX_LABEL_LEN + 2 ];
	extern char	*malloc();

/*  Error if limit implies too many choices.
    Error if the format string is too long.
    Add 6 to account for the integer itself.	*/

	if (limit_of_index > MAX_NUM_CHOICES)
	  return( -1 );
	if (strlen( fmt ) > (unsigned int) MAX_LABEL_LEN + 6)
	  return( -1 );

	for (iter = 0; iter < limit_of_index; iter++) {
		value = iter+1;
		sprintf( &quick_buf[ 0 ], fmt, value );
		length = strlen( &quick_buf[ 0 ] );
		choice_array[ iter ] = malloc( length + 1 );
		strcpy( choice_array[ iter ], &quick_buf[ 0 ] );
	}
	choice_array[ limit_of_index ] = NULL;
	return( 0 );
}

int
panel_choices_from_table_of_double( choice_array, fmt, table_of_double, table_size )
char *choice_array[];
char *fmt;
double *table_of_double;
int table_size;
{
	int		 iter, length;
	char		 quick_buf[ MAX_LABEL_LEN + 2 ];
	double		 value;
	extern char	*malloc();

/*  Error if limit implies too many choices.
    Error if the format string is too long.
    Add 6 to account for the integer itself.	*/

	if (table_size > MAX_NUM_CHOICES)
	  return( -1 );
	if (strlen( fmt ) > (unsigned int) (MAX_LABEL_LEN + 12))
	  return( -1 );

	for (iter = 0; iter < table_size; iter++) {
		value = table_of_double[ iter ];
		sprintf( &quick_buf[ 0 ], fmt, value );
		length = strlen( &quick_buf[ 0 ] );
		choice_array[ iter ] = malloc( length + 1 );
		strcpy( choice_array[ iter ], &quick_buf[ 0 ] );
	}
	choice_array[ table_size ] = NULL;
	return( 0 );
}

/*  This is a very dangerous routine unless `table_addr' is the address
    of a list of address into the heap, terminated with a NULL address.  */

int
free_table_of_memory( table_addr )
char *table_addr[];
{
	int	iter;

	iter = 0;
	while( table_addr[ iter ] != NULL) {
		free( table_addr[ iter ] );
		table_addr[ iter ] = NULL;
		iter++;
	}

	return( 0 );
}

/*  stores "first value", "first value + 1.0", ... 
    stores count of "limit" values.			*/

table_of_double_from_limit( table_of_double, first_value, limit )
double *table_of_double;
double first_value;
int limit;
{
	double	dval;

	dval = first_value;
	while (limit > 0) {
		*(table_of_double++) = dval;
		dval++;
		limit--;
	}
}


/************************************************************************/

int
find_index_entry( value, limit )
double value;
int limit;
{
	int	iter;

	for (iter = 1; iter <= limit; iter++)
	  if (cmp_double((double) (iter), value) == 0)
	    return( iter-1 );

	return( -1 );
}

int
find_table_char( value, table_addr, table_size )
char value;
char *table_addr;
int table_size;
{

	int	iter;

	for (iter = 0; iter < table_size; iter++)
	  if (table_addr[ iter ] == value)
	    return( iter );

	return( -1 );
}

int
find_table_string( value, table_addr, table_size )
char *value;
char *table_addr[];
int table_size;
{

	int	iter;

	for (iter = 0; iter < table_size; iter++)
	  if (strcmp(table_addr[ iter ],value) == 0)
	    return( iter );

	return( -1 );
}

int
find_table_double( value, table_addr, table_size )
double value;
double *table_addr;
int table_size;
{
	int	iter;

	for (iter = 0; iter < table_size; iter++)
	  if (cmp_double(table_addr[ iter ], value) == 0)
	    return( iter );

	return( -1 );
}

/*  Compare two double precision numbers.  The two are found to be equal
    if their values are within PRECISION of each other, normalized by the
    average of their absolute value.

    This routine is designed to prevent problems caused by truncation
    and roundoff.

    It returns 0 if the two are considered equal, 1 if the first value is
    larger and -1 if the first value is smaller.			*/

#define  PRECISION	1e-6

int
cmp_double( x1, x2 )
double x1;
double x2;
{
	double		abs_diff, abs_x1, abs_x2, scale_f;

/*  Save some work if the two values are identical.	*/

	if (x1 == x2)
	  return( 0 );

/*  Obtain absolute value of each number.  */

	if (x1 < 0.0)
	  abs_x1 = -x1;
	else
	  abs_x1 = x1;
	if (x2 < 0.0)
	  abs_x2 = -x2;
	else
	  abs_x2 = x2;

/*  If either value is 0, that is a special case.  */

	if (x1 == 0.0) {
		if (abs_x2 < PRECISION)
		  return( 0 );
		else if (x2 < 0.0)
		  return( 1 );
		else
		  return( -1 );
	}
	else if (x2 == 0.0) {
		if (abs_x1 < PRECISION)
		  return( 0 );
		else if (x1 < 0.0)
		  return( -1 );
		else
		  return( 1 );
	}

/*  Come here if neither value is identical to zero.	*/

	scale_f = (abs_x1 + abs_x2) / 2.0;

	abs_diff = x1 - x2;
	if (abs_diff < 0.0)
	  abs_diff = -abs_diff;
	if (abs_diff / scale_f < PRECISION)
	  return( 0 );
	else if (x1 > x2)
	  return( 1 );
	else
	  return( -1 );
}

/************************************************************************/

extern symbol	**getTreeRoot();
extern varInfo	 *rfindVar();
extern varInfo	 *RcreateVar();

int set_size_array_real( name, new_size )
char *name;
int new_size;
{
	int	cur_size, iter, result;
	vInfo	info;

	result = P_getVarInfo( SYSTEMGLOBAL, name, &info );
	if (result != 0) {
		result = P_creatvar( SYSTEMGLOBAL, name, T_REAL );
		if (result != 0)
		  return( result );
	}
	else if (info.basicType != T_REAL)
	  return( -1 );

	cur_size = info.size;
	if (cur_size >= new_size)
	  return( 0 );				/* Already big enough!! */

	for (iter = cur_size+1; iter <= new_size; iter++) {
		result = config_setreal( name, 0.0, iter );
		if (result != 0)
		  return( result );
	}

	return( 0 );
}

/*  `config_setreal' is similar to P_setreal, except:
	1.  Understands the tree is SYSTEMGLOBAL
	2.  Attempts to create the parameter if it doesn't exist.	*/

int
config_setreal( name, value, index )
char *name;
double value;
int index;
{
	symbol	**root;
	varInfo *v;

	if (root = getTreeRoot(getRoot( SYSTEMGLOBAL )))
	  if (v = rfindVar( name, root )) {
		if (assignReal( value, v, index ))
		  return( 0 );
		else
		  return( -99 );
	  }

/*  Create the real number parameter if it does not exist.  */

	  else {
		v = RcreateVar( name, root, T_REAL );
		v->Ggroup = G_ACQUISITION;
		if (assignReal( value, v, index ))
		  return( 0 );
		else
		  return( -99 );
	  }
	else
	  return( -1 );			/* No such tree (should not happen) */
}

/*  `config_setstring' is similar to P_setstring, except:
	1.  Understands the tree is SYSTEMGLOBAL
	2.  Attempts to create the parameter if it doesn't exist.	*/

int config_setstring( name, value, index )
char *name;
char *value;
int index;
{
	symbol	**root;
	varInfo *v;

	if (root = getTreeRoot(getRoot( SYSTEMGLOBAL )))
	  if (v = rfindVar( name, root )) {
		if (assignString( value, v, index ))
		  return( 0 );
		else
		  return( -99 );
	  }

/*  Create the string parameter if it does not exist.  */

	  else {
		v = RcreateVar( name, root, T_STRING );
		if (assignString( value, v, index ))
		  return( 0 );
		else
		  return( -99 );
	  }
	else
	  return( -1 );			/* No such tree (should not happen) */
}


/*  Set all the values in a real number parameter array, using a table
    of choices to look up the actual value in a table of real numbers.	*/

int
set_array_real_using_table(
	name,
	table_of_real,
	table_size,
	table_of_choices,
	number_choices
)
char *name;
double *table_of_real;
int table_size;
int *table_of_choices;
int number_choices;
{
	int	cur_choice, iter, r;
	double	dval;

/*  Delete all values in the old array  */

	r = config_setreal( name, 0.0, 0);
	if (r != 0)
	  return( r );

/*  You can make an array by just setting values, starting at index = 1.  */

	for (iter = 0; iter < number_choices; iter++) {
		cur_choice = table_of_choices[ iter ];
		r = config_setreal( name, table_of_real[ cur_choice ], iter+1 );
		if (r != 0)
		  break;
	}

	return( r );
}

/*  Set one element in a real number parameter array, using a choice
    value to look up the actual value in a table of real numbers.	*/

int
set_element_array_real_using_table(
	param_name,
	param_index,
	mychoice,
	table_of_real,
	table_size
)
char *param_name;
int param_index;
int mychoice;
double *table_of_real;
int table_size;
{
	int	r;

	if (mychoice < 0)
	  mychoice = 0;
	else if (mychoice >= table_size)
	  mychoice = table_size-1;

	r = config_setreal( param_name, table_of_real[ mychoice ], param_index );
	return( r );
}

/*  Look up the desired element in the arrayed parameter.  Then find the
    value in the table of double.  Return the index or -1 if not found.  */

int
get_choice_using_element_real_array(
	param_name,
	param_index,
	table_of_real,
	table_size
)
char *param_name;
int param_index;
double *table_of_real;
int table_size;
{
	int	iter, r;
	double	dval;

	r = P_getreal( SYSTEMGLOBAL, param_name, &dval, param_index );
	if (r != 0)
	  return( 0 );			/* tell calling routine to use default */
					/* choice if parameter not found. */
	return(
		find_table_double( dval, table_of_real, table_size )
	);
}

char *
find_first_nonblank( a_string )
char *a_string;
{
	while (isspace( *a_string ))
	  a_string++;
	return( a_string );
}
