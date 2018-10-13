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

/*  I believe that small programs like this one are best edited as a single file.
    Although it takes a few more seconds to compile, with ever increasing CPU speeds
    this delay will only become less noticable.  Furthermore the make process, which
    is integral to any software we make, is made greatly simple.  No elaborate
    dependencies exist.  The make program can essentially compile it on its own,
    using its own builtin rules.

    A more serious drawback is that a single file lacks modularity and is less
    likely to be reused.  To try and address that, I separate the program into
    sections, each demarked with a line of astericks. ('*')  Programs which are
    internal to a section are declared static.					*/
    

#include <stdio.h>
#include <string.h>
#include <malloc.h>


/*  Values for parent_prog  */

#define  LOADVNMR	0
#define  GETOPTIONAL	1

struct _global_data {
	char	*cmd_name;			/* a duplicate of argv[ 0 ] */
	int	 parent_prog;
} global_data;

int
get_yes_or_no( prompt, default_val )
char *prompt;
int default_val;
{

/*  Program returns 0 for No, 1 for Yes.  You (the programmer) give it a default,
    which it will return if it doesn't recognize the user's input.  So it is a
    one-try program; no inifinite loop from a stubborn user who won't type Y or N.  */

	char	*aval;
	int	 retval;
	char	 user_response[ 122 ];

	fputs( prompt, stdout );

/*  Use fgets to prevent malicious user from overwriting the stack
    by typing in thousands of characters.				*/

	aval = fgets( &user_response[ 0 ], sizeof( user_response ) - 2, stdin );
	if (aval == NULL)
	  retval = default_val;
	else if (user_response[ 0 ] == 'N' || user_response[ 0 ] == 'n')
	  retval = 0;
	else if (user_response[ 0 ] == 'Y' || user_response[ 0 ] == 'y')
	  retval = 1;
	else
	  retval = default_val;

	return( retval );
}

/*  Start with a complete file name (acqbin.tar.Z)
    Return the corresponding name (acqbin), allocated from the heap.  */

char *
fname_to_name( fname )
char *fname;
{
	int	 len;
	char	*qaddr, *haddr;

	if (fname == NULL)
	  return( NULL );

	qaddr = strchr( fname, '.' );
	if (qaddr == NULL) {			/* fancy that... no extension */
		len = strlen( fname );
	}
	else {
		len = qaddr - fname;
	}

	haddr = malloc( (len+1) * sizeof( char ) );	/* space for '\0' !! */
	if (haddr == NULL)
	  return( NULL );
	strncpy( haddr, fname, len );
	haddr[ len ] = '\0';

	return( haddr );			/* a Heap address !! */
}

/*  Is sa a substring of sb (starting at the start).
    Returns 0 if yes, -1 if not.			*/

int
start_anchored_cmp( sa, sb )
char *sa;
char *sb;
{
	int	la, lb;

	if (sa == NULL && sb == NULL)
	  return( 0 );

	if (sa == NULL || sb == NULL)
	  return( -1 );

	la = strlen( sa );
	lb = strlen( sb );

	if (la > lb)
	  return( -1 );

	return( strncmp( sa, sb, la ) );
}

/********************************************************************************/

/* These data structures/objects allow a series of Table of Contents, a list of
   entries from an archive on the installation tape, to be stored in the memory
   of the computer.  The `toc_item' represents a single entry; the `toc_list'
   represents a list built of `toc_item's.

   The list is a series of addresses to the individual items, ordered by the
   order they appear in the list.  This order is ultimately established by the
   order in which they appear in the table of contents from the disk file.  It
   is important because it is the order in which entries appear in the archive.

   The list works much like the "argument vector" which any C main program
   receives.  All addresses are allocated from the heap.  It is not expected
   that any entry will need to be discarded or free'd while the program runs.	*/

/*  Values for the status field  */

#define  UNKNOWN	0
#define  DEPENDENT	1
#define  REJECTED	2
#define  SELECTED	3

/*  Values for the req_opt field  */

#define  REQUIRED	1
#define  OPTIONAL	2
#define  UPGRADE	3
#define  VKERNEL	4

/* For better results on a SPARC, place the largest items (type=double) first.	*/

typedef struct _toc_item {
	double	 size;
	char	*name;
	char	*fname;
	char	*dname;
	int	 status;
	int	 req_opt;
} toc_item;


#define  COMMON  1
#define  ARCHD   2

typedef struct _toc_list {
	toc_item	**items;	/* compiler forbids *items[] */
	int		  count;
} toc_list;

toc_list	common_list, archd_list;


static toc_list *
get_this_list( select_toc ) 
int select_toc;
{
	toc_list	*toc_l_addr;

	if (select_toc == COMMON)
	  toc_l_addr = &common_list;
	else if (select_toc == ARCHD)
	  toc_l_addr = &archd_list;
	else
	  toc_l_addr = NULL;

	return( toc_l_addr );
}

int
init_toc( select_toc )
int select_toc;
{
	toc_item	**qaddr;
	toc_list	*toc_l_addr;

	toc_l_addr = get_this_list( select_toc );
	if (toc_l_addr == NULL)
	  return( -1 );

	qaddr = (toc_item **) malloc( sizeof( toc_item *) );
	if (qaddr == NULL)
	  return( -1 );

	toc_l_addr->items = qaddr;
	toc_l_addr->count = 0;
	toc_l_addr->items[ toc_l_addr->count ] = NULL;

	return( 0 );
}

int
add_an_entry( select_toc, fname, dname, size, req_opt )
int select_toc;
char *fname;
char *dname;
double size;
char *req_opt;
{
	char		*dname_on_heap, *fname_on_heap, *qaddr, *simple_name;
	int		 retval;
	toc_item	*item_on_heap, **newaddr;
	toc_list	*toc_l_addr;

	toc_l_addr = get_this_list( select_toc );
	if (toc_l_addr == NULL)
	  return( -1 );

	simple_name = fname_to_name( fname );
	if (simple_name == NULL)
	  return( -1 );

	dname_on_heap = malloc( (strlen( dname ) + 1) * sizeof( char ) );
	if (dname_on_heap == NULL) {
		free( simple_name );
		return( -1 );
	}

	strcpy( dname_on_heap, dname );
	fname_on_heap = malloc( (strlen( fname ) + 1) * sizeof( char ) );
	if (fname_on_heap == NULL) {
		free( dname_on_heap );
		free( simple_name );
		return( -1 );
	}

	strcpy( fname_on_heap, fname );

	item_on_heap = (toc_item *) malloc( sizeof( toc_item ) );
	if (item_on_heap == NULL) {
		free( fname_on_heap );
		free( dname_on_heap );
		free( simple_name );
		return( -1 );
	}

	toc_l_addr->count += 1;
	newaddr = (toc_item **) realloc(
		 toc_l_addr->items, (toc_l_addr->count + 1) * sizeof( toc_item * )
	);
		/* you must allocate an extra for the NULL terminating address  */

	if (newaddr == NULL) {
		free( item_on_heap );
		free( fname_on_heap );
		free( dname_on_heap );
		free( simple_name );
		return( -1 );
	}

	item_on_heap->name = simple_name;	/* simple name addresses into the heap */

	if (strcmp( req_opt, "req" ) == 0)
	  item_on_heap->req_opt = REQUIRED;
	else if (strcmp( req_opt, "upg" ) == 0)
	  item_on_heap->req_opt = UPGRADE;
	else if (strcmp( req_opt, "ker" ) == 0)
	  item_on_heap->req_opt = VKERNEL;
	else
	  item_on_heap->req_opt = OPTIONAL;

	item_on_heap->status = UNKNOWN;
	item_on_heap->size = size;
	item_on_heap->fname = fname_on_heap;
	item_on_heap->dname = dname_on_heap;

	toc_l_addr->items = newaddr;
	toc_l_addr->items[ toc_l_addr->count-1 ] = item_on_heap;
	toc_l_addr->items[ toc_l_addr->count ] = NULL;

	return( 0 );
}

int
get_prompt( select_toc, index, prompt_addr, prompt_len, y_n_def )
int select_toc;
int index;
char *prompt_addr;
int prompt_len;						/* not used, for now. */
int y_n_def;
{
	toc_item	*this_item;
	toc_list	*toc_l_addr;

	toc_l_addr = get_this_list( select_toc );
	if (toc_l_addr == NULL)
	  return( -1 );

	this_item = toc_l_addr->items[ index ];
	if (this_item == NULL)
	  return( -1 );

	sprintf( prompt_addr, "Load %s (%gMb) (y or n) [%c]: ",
                 this_item->dname, this_item->size,
                 (y_n_def) ? 'y' : 'n');

	return( 0 );
}

int
mark_entry_dependent( select_toc, index )
int select_toc;
int index;
{
	toc_item	*this_item;
	toc_list	*toc_l_addr;

	toc_l_addr = get_this_list( select_toc );
	if (toc_l_addr == NULL)
	  return( -1 );

	this_item = toc_l_addr->items[ index ];
	if (this_item == NULL)
	  return( -1 );

	this_item->status = DEPENDENT;
	return( 0 );
}

int
mark_entry_selected( select_toc, index )
int select_toc;
int index;
{
	toc_item	*this_item;
	toc_list	*toc_l_addr;

	toc_l_addr = get_this_list( select_toc );
	if (toc_l_addr == NULL)
	  return( -1 );

	this_item = toc_l_addr->items[ index ];
	if (this_item == NULL)
	  return( -1 );

	this_item->status = SELECTED;
	return( 0 );
}

#define  IS_REQUIRED	1
#define  IS_DEPENDENT	2
#define  IS_SELECTED	3
#define  IS_UPGRADE	4
#define  IS_KERNEL	5

static int
examine_entry( select_toc, index, criterion, val_addr )
int select_toc;
int index;
int criterion;
int *val_addr;
{
	int		 retval;
	toc_item	*this_item;
	toc_list	*toc_l_addr;

	toc_l_addr = get_this_list( select_toc );
	if (toc_l_addr == NULL)
	  return( -1 );

	this_item = toc_l_addr->items[ index ];
	if (this_item == NULL)
	  return( -1 );

	retval = 0;
	if (criterion == IS_REQUIRED)
	  *val_addr = (this_item->req_opt == REQUIRED);
	else if (criterion == IS_DEPENDENT)
	  *val_addr = (this_item->status == DEPENDENT);
	else if (criterion == IS_SELECTED)
	  *val_addr = (this_item->status == SELECTED);
	else if (criterion == IS_UPGRADE)
	  *val_addr = (this_item->req_opt == UPGRADE);
	else if (criterion == IS_KERNEL)
	  *val_addr = (this_item->req_opt == VKERNEL);
	else
	  retval = -1;

	return( retval );
}

int
is_entry_required( select_toc, index, val_addr )
int select_toc;
int index;
int *val_addr;
{
	return( examine_entry( select_toc, index, IS_REQUIRED, val_addr ) );
}

int
is_entry_dependent( select_toc, index, val_addr )
int select_toc;
int index;
int *val_addr;
{
	return( examine_entry( select_toc, index, IS_DEPENDENT, val_addr ) );
}

int
is_entry_upgrade( select_toc, index, val_addr )
int select_toc;
int index;
int *val_addr;
{
	return( examine_entry( select_toc, index, IS_UPGRADE, val_addr ) );
}

int
is_entry_kernel( select_toc, index, val_addr )
int select_toc;
int index;
int *val_addr;
{
	return( examine_entry( select_toc, index, IS_KERNEL, val_addr ) );
}

int
is_entry_selected( select_toc, index, val_addr )
int select_toc;
int index;
int *val_addr;
{
	return( examine_entry( select_toc, index, IS_SELECTED, val_addr ) );
}

char *
get_entry_name( select_toc, index )
int select_toc;
int index;
{
	toc_item	*this_item;
	toc_list	*toc_l_addr;

	toc_l_addr = get_this_list( select_toc );
	if (toc_l_addr == NULL)
	  return( NULL );

	this_item = toc_l_addr->items[ index ];
	if (this_item == NULL)
	  return( NULL);
	else
	  return( this_item->name );
}

char *
get_entry_fname( select_toc, index )
int select_toc;
int index;
{
	toc_item	*this_item;
	toc_list	*toc_l_addr;

	toc_l_addr = get_this_list( select_toc );
	if (toc_l_addr == NULL)
	  return( NULL );

	this_item = toc_l_addr->items[ index ];
	if (this_item == NULL)
	  return( NULL);
	else
	  return( this_item->fname );
}

double
get_entry_size( select_toc, index )
int select_toc;
int index;
{
	toc_item	*this_item;
	toc_list	*toc_l_addr;

	toc_l_addr = get_this_list( select_toc );
	if (toc_l_addr == NULL)
	  return( -1.0 );

	this_item = toc_l_addr->items[ index ];
	if (this_item == NULL)
	  return( -1.0);
	else
	  return( this_item->size );
}

int
set_entry_size( select_toc, index, new_size )
int select_toc;
int index;
double new_size;
{
	toc_item	*this_item;
	toc_list	*toc_l_addr;

	toc_l_addr = get_this_list( select_toc );
	if (toc_l_addr == NULL)
	  return( -1 );

	this_item = toc_l_addr->items[ index ];
	if (this_item == NULL)
	  return( -1 );

	this_item->size = new_size;
	return( 0 );
}

/*  Someday rework this as a method utilizing review_toc.  */

int
is_independent_selected( name, sel_addr )
char *name;
int *sel_addr;
{
	int		 index, retval, selected;
	toc_item	**qaddr;
	toc_list	*toc_l_addr;

	toc_l_addr = &archd_list;
	if (toc_l_addr == NULL)
	  return( -1 );

	qaddr = toc_l_addr->items;
	retval = 0;

	index = 0;
	while (*qaddr != NULL) {
		if (start_anchored_cmp( name, (*qaddr)->name ) == 0) {
			retval = is_entry_selected( ARCHD, index, &selected );
			if (retval != 0)
			  break;
			if (selected) {
				*sel_addr = selected;
				break;
			}
		}
		index++;
		qaddr++;
	}

/* If the while loop terminated without executing a break,
   none of the independent entries were selected.		*/

	if (*qaddr == NULL)
	  *sel_addr = 0;

	return( retval );
}

/*  This is likely the most difficut method to understand.  It is an attempt to
    abstract the operation of looping over each item in a list.  The select_toc
    identifies the list.  Then for each entry in the list the callback program
    is called.  The interface object is the way the callback communicates with
    the program that called review_toc.  Note that the list/entry programs are
    ignorant of the structure of this interface object; it is private between
    the calling program and the callback program.  Similarly these two remain
    ignorant of the details of the list or the entries, including the number of
    entries.

    Normally the callback is called for each item in the list.  If the callback
    wants to stop this, it should return a non-zero value.  Conversely, it must
    return a 0 to continue the looping.						*/

int
review_toc( select_toc, callback, interface_obj )
int select_toc;
int (*callback)();
char *interface_obj;
{
	int		 index, ival;
	toc_item	**qaddr;
	toc_list	*toc_l_addr;

	toc_l_addr = get_this_list( select_toc );
	if (toc_l_addr == NULL)
	  return( -1 );

	qaddr = toc_l_addr->items;

	index = 0;
	while (*qaddr != NULL) {
		ival = (*callback)( select_toc, index, interface_obj );
		if (ival != 0)
		  break;
		qaddr++;
		index++;
	}

	return( ival );
}

/*  For debug and illustrative purposes only.  */

int
show_the_entries( select_toc )
int select_toc;
{
	int		 index;
	toc_item	**qaddr;
	toc_list	*toc_l_addr;

	toc_l_addr = get_this_list( select_toc );
	if (toc_l_addr == NULL)
	  return( -1 );

	qaddr = toc_l_addr->items;

	index = 0;
	while (*qaddr != NULL) {
		printf( "%d:  %s   %s   %s   %g ",
			   index+1,
			 (*qaddr)->name,
			 (*qaddr)->fname,
			 (*qaddr)->dname,
			 (*qaddr)->size
		);

		if ( (*qaddr)->status == SELECTED)
		  printf( "SELECTED" );
		else
		  printf( "REJECTED" );

		printf( "\n" );
		qaddr++;
		index++;
	}
}

/********************************************************************************/

/*  Programs to interact with the table of contents (input)
    and the user's choices (output).				*/

static FILE	*fp_toc;

int
init_toc_input( fname )
char *fname;
{
	int	retval;

	fp_toc = fopen( fname, "r" );
	if (fp_toc == NULL)
	  retval = -1;
	else
	  retval = 0;

	return( retval );
}

int
get_next_archive( name_of_ar, req_opt, disk_space, disp_name )
char *name_of_ar;
char *req_opt;
char *disk_space;
char *disp_name;
{
	int	ival, retval;

	do {
		ival = fscanf( fp_toc, "%s%s%s\n", name_of_ar, req_opt, disk_space );
		if (ival < 3)
		  retval = -1;
		else {
			char	*qaddr;

			fgets( disp_name, 120, fp_toc );
			qaddr = strchr( disp_name, '\n' );
			if (qaddr != NULL)
			  *qaddr = '\0';
			retval = 0;
		}

/*  Skip over comments, 1st character is '#'  */

	}
	  while (*name_of_ar == '#');

	return( retval );
}

int
close_toc_input()
{
	fclose( fp_toc );
	fp_toc = NULL;
}


static FILE	*fp_choices;

int
init_choices_output( fname )
char *fname;
{
	int	retval;

	fp_choices = fopen( fname, "w" );
	if (fp_choices == NULL)
	  retval = -1;
	else
	  retval = 0;

	return( retval );
}

int
writeout_a_choice( name )
char *name;
{
	fprintf( fp_choices, name );
	fprintf( fp_choices, "\n" );
}

int
close_choices_output()
{
	fclose( fp_choices );
	fp_choices = NULL;
}


/********************************************************************************/

int
read_a_toc( toc_name, toc_type )
char *toc_name;
int toc_type;
{
	char		name_of_archive[ 122 ], display_name[ 122 ],
			req_opt[ 22 ], disk_space[ 22 ];
	int		ival;
	extern double	atof();

	ival = init_toc_input( toc_name );
	if (ival != 0) {
		char	quickmsg[ 82 ];

		sprintf( &quickmsg[ 0 ], "error accessing %s", toc_name );
		perror( &quickmsg[ 0 ] );
		return( ival );
	}

	while (get_next_archive(
		&name_of_archive[ 0 ],
		&req_opt[ 0 ],
		&disk_space[ 0 ],
		&display_name[ 0 ]
	) == 0) {
		ival = add_an_entry(
			 toc_type,
			&name_of_archive[ 0 ],
			&display_name[ 0 ],
			 atof( &disk_space[ 0 ] ),
			&req_opt[ 0 ]
		);
		if (ival != 0)
		  break;
	}

	return( ival );
}

/********************************************************************************/


typedef struct _dependency_interface_obj {
	double	 size;
	char	*name;
	int	 base_list_index;
	int	 found_base_entry;
} dependency_interface_obj;

static int
entry_dependency_cmp( toc_index, entry_index, dio_addr )
int toc_index;
int entry_index;
dependency_interface_obj *dio_addr;
{

/*  The table of contents here is the base TOC (sun4.toc)  */

	char	*base_entry_name;
	double	 base_entry_size;

	base_entry_name = get_entry_name( toc_index, entry_index );
	if (base_entry_name == NULL)
	  return( -1 );

	if (start_anchored_cmp( dio_addr->name, base_entry_name ) == 0) {
		dio_addr->found_base_entry = 1;
		base_entry_size = get_entry_size( toc_index, entry_index );
		if (base_entry_size < 0.0)
		  return( -1 );
		base_entry_size += dio_addr->size;

		set_entry_size( toc_index, entry_index, base_entry_size );
	}

	return( 0 );
}

static int
find_entry_dependency( toc_index, entry_index, dio_addr )
int toc_index;
int entry_index;
dependency_interface_obj *dio_addr;
{

/*  The table of contents here is the dependent TOC (common.toc)  */

	dio_addr->size = get_entry_size( toc_index, entry_index );
	dio_addr->name = get_entry_name( toc_index, entry_index );
	dio_addr->found_base_entry = 0;

	review_toc( dio_addr->base_list_index, entry_dependency_cmp, dio_addr );

	if (dio_addr->found_base_entry)
	  mark_entry_dependent( toc_index, entry_index );
	return( 0 );
}

int
find_any_dependencies( dependent_list, base_list )
int dependent_list;
int base_list;
{
	dependency_interface_obj	dio;

	dio.base_list_index = base_list;
	dio.name      = NULL;
	dio.size      = 0.0;

	review_toc( dependent_list, find_entry_dependency, &dio );

	return( 0 );
}

/*  Actually works for both loadvnmr and getoptions.  */

static int
loadvnmr_decide( toc_index, entry_index )
int toc_index;
int entry_index;
{
	int	 ival, retval;
	int	 decision, dependent, required, selected, upgrade, kernel;
	char	*name;
	char	 user_prompt[ 122 ];
        static int first_kernel = -1;

	decision = 0;
	selected = 0;
	retval = is_entry_required( toc_index, entry_index, &required );
	if (retval != 0)
	  return( retval );

	if (required) {
		if (global_data.parent_prog == LOADVNMR)
		  selected = 1;
		else
		  selected = 0;

		decision = 1;
	}

	if ( !decision ) {
		retval = is_entry_dependent( toc_index, entry_index, &dependent );
		if (retval != 0)
		  return( retval );

		if (dependent) {
			name = get_entry_name( toc_index, entry_index );
			if (name == NULL)
			  return( -1 );

			retval = is_independent_selected( name, &selected );
			if (retval != 0)
			  return( retval );

			decision = 1;
		}
	}

	if ( !decision ) {
		retval = is_entry_upgrade( toc_index, entry_index, &upgrade );
		if (retval != 0)
		  return( retval );

/*  Note:  For upgrade item, if loadvnmr is the parent program, the item is
	   not selected and a decision has been made.  If getoptions is the
	   parent program, selected/not selected is not determined and no
	   decision has been made.						*/
             
		if (upgrade)
		  if (global_data.parent_prog == LOADVNMR)
                  {
			selected = 0;
			decision = 1;
		  }
	}

	if ( !decision ) {
		retval = is_entry_kernel( toc_index, entry_index, &kernel );
		if (retval != 0)
		  return( retval );

/*  Note:  For kernel item, if loadvnmr is the parent program, the item is
	   not selected and a decision has been made.  If getoptions is the
	   parent program, selected/not selected is not determined and no
	   decision has been made.						*/
             
		if (kernel)
                {
		  if (global_data.parent_prog == LOADVNMR)
                  {
			selected = 0;
			decision = 1;
		  }
/* -1 means kernel image never seen; therefore ask about default;
/* 0  means does not want kernel
/* 1  means provide choice
 */
                  else if (first_kernel == -1)
                  {
		     first_kernel = get_yes_or_no( "Load SunOS kernel for acquisition (y or n) [n]: ", 0 );
                     if (!first_kernel)
                     {
			selected = 0;
			decision = 1;
                     }
                  }
                  else if (!first_kernel)
                  {
			selected = 0;
			decision = 1;
                  }
                }
	}

	if ( !decision ) {
		int y_n_default;

                y_n_default = (global_data.parent_prog == LOADVNMR);
		retval = get_prompt( toc_index, entry_index,
				&user_prompt[ 0 ], sizeof( user_prompt ) - 2,
                                y_n_default);
		if (retval != 0)
		  return( retval );

		selected = get_yes_or_no( &user_prompt[ 0 ], y_n_default );
	}

	if (selected)
	  mark_entry_selected( toc_index, entry_index );

	return( 0 );
}

static int
decide_on_entry( toc_index, entry_index, interface_obj )
int toc_index;
int entry_index;
char *interface_obj;				/* NOT USED */
{
	int	ival;

	/*if (global_data.parent_prog == LOADVNMR)
	  ival = loadvnmr_decide( toc_index, entry_index );
	else
	  ival = getoptional_decide( toc_index, entry_index );*/

	ival = loadvnmr_decide( toc_index, entry_index );
	return( ival );
}

int
make_choices( toc_index )
int toc_index;
{
	review_toc( toc_index, decide_on_entry, NULL );
}

static int
record_entry( toc_index, entry_index, interface_obj )
int toc_index;
int entry_index;
char *interface_obj;				/* NOT USED */
{
	int	 retval, selected;
	char	*name;

	retval = is_entry_selected( toc_index, entry_index, &selected );
	if (retval != 0)
	  return( retval );

	if (selected) {
		name = get_entry_fname( toc_index, entry_index );
		if (name == NULL)
		  return( -1 );
		writeout_a_choice( name );
	}

	return( 0 );
}

int
record_choices( fname, toc_index )
char *fname;
int toc_index;
{
	int	ival;

	ival = init_choices_output( fname );
	if (ival != 0)
	  return( ival );

	review_toc( toc_index, record_entry, NULL );

	close_choices_output();
}

main( argc, argv )
int argc;
char *argv[];
{
	char	toc_name[ 122 ], choices_name[ 122 ];

	if (argc < 2) {
		fprintf( stderr,
			"%s was started with an incorrect number of parameters\n", argv[ 0 ]
		);
		exit( 1 );
	}

/*  Store global data  */

	global_data.cmd_name = argv[ 0 ];
	if (argc < 3) {
		global_data.parent_prog = LOADVNMR;
	}
	else {
		if (strcmp( argv[ 2 ], "getoptional" ) == 0)
		  global_data.parent_prog = GETOPTIONAL;
		else if (strcmp( argv[ 2 ], "loadvnmr" ) == 0)
		  global_data.parent_prog = LOADVNMR;
		else {
			fprintf( stderr,
			   "%s called with unknown parent program %s\n", argv[ 0 ], argv[ 2 ]
			);
			exit( 1 );
		}
	}

	init_toc( ARCHD );
	init_toc( COMMON );

	strcpy( &toc_name[ 0 ], argv[ 1 ] );
	strcat( &toc_name[ 0 ], ".toc" );
	read_a_toc( &toc_name[ 0 ], ARCHD );

	strcpy( &toc_name[ 0 ], "common" );
	strcat( &toc_name[ 0 ], ".toc" );
	read_a_toc( &toc_name[ 0 ], COMMON );

	/*if (global_data.parent_prog == LOADVNMR)
	  find_any_dependencies( COMMON, ARCHD );*/

	find_any_dependencies( COMMON, ARCHD );

	make_choices( ARCHD );
	make_choices( COMMON );

	strcpy( &choices_name[ 0 ], argv[ 1 ] );
	strcat( &choices_name[ 0 ], ".choices" );
	record_choices( &choices_name[ 0 ], ARCHD );

	strcpy( &choices_name[ 0 ], "common" );
	strcat( &choices_name[ 0 ], ".choices" );
	record_choices( &choices_name[ 0 ], COMMON );
	return( 0 );
}
