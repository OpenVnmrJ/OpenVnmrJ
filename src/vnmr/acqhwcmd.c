/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include "vnmrsys.h"
#include "ACQ_SUN.h"
#include "STAT_DEFS.h"
#include "group.h"
#include "params.h"
#include "locksys.h"			/* defines AUTOMATION */
#include "variables.h"
#include "shims.h"
#include "acquisition.h"
#include "pvars.h"
#include "sockinfo.h"
#include "tools.h"
#include "wjunk.h"


#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>

#include "sockets.h"

#define  ACQHARDWARE	16
#define  READACQHW	17
#define  QUEQUERY	18
#define  ACCESSQUERY	19

#define  FALSE		0
#define  TRUE		1
#define  COMPLETE	0
#define  ERROR		-1
#define  OFF_DEEP_END	-3
#define  MAX_REQUESTS	10
#define  STRLEN 	128

/* success/error for Vnmr/Magical interface */

#define  FAILED		0
#define  SUCCEEDED	1
#define  STARTED	2

#define  NOT_FOUND	-2

#define  EXEC_GO	0			/* taken from go.c */

extern int	 mode_of_vnmr;
extern int       nvAcquisition();
extern Socket*   connect2Jpsg(int,char*);
extern void      P_endPipe(int fd);
extern int       J_sendVPVars(int tree, char *name, int fd);
extern int       isJpsgReady();
extern int       init_rtvars(char *filename, int number);
extern int       rtn_index(char *search_name);
extern int       expdir_to_expnum(char *expdir);
extern int       getword(FILE *path, char word[], int limit, char lower[], int use_custom);
extern int       talk_2_acqproc(char *hostname, char *username, int cmd,
                   char *msg_for_acqproc, char *msg_for_vnmr, int mfv_len );
extern int       is_acqproc_active();
extern int       get_ia_stat(char *hostname, char *username );
extern int       getAcqSample();
extern int       getAcqLSDV();
extern int       getAcqTemp();
extern int       getAcqTempSet();
extern int       getAcqSpin();
extern int       getAcqSpinSet();
extern int       getAcqTickCountError();
extern int       getAcqStatusInt(int index, int *val);
extern int       GetAcqStatus(int to_index, int from_index, char *host, char *user);
extern int       getAcqRemainingTime();
extern int       getAcqInQue();
extern void      vnmr_tolower(char *s);
extern int       getparm(char *varname, char *vartype, int tree,
                   void *varaddr, int size);
extern int       is_datastation();
extern void      getlkfreqDDsi(double lockfreq, int h1freq, int *parts);
extern int       getlkfreqapword(double lockfreq, int h1freq );
extern void      oneLongToTwoShort(int lval, short *sval );
extern void      sleepSeconds(int secs);
extern void      sleepMilliSeconds(int msecs);


static int	sethw_wait;
static int	cur_table_entry;
static int	cur_req_num;

static int	want_to_sleep;

static int	AllowUpdatingFlag = 0;
void report_acq_perm_error(char *cmdname, int errorval );
int  ok_to_acquire();
int talk_to_acqproc(int cmd, char *msg1, char *msg2, int msg2len);
int release_console2();
static int check_ShimPowerHw(int index, int *newval);

int
get_h1freq()
{
   double h1freq;

   if (getparm("h1freq", "REAL", SYSTEMGLOBAL, &h1freq, 1))
      return( -1 );

   return( (int) (h1freq + 0.5) );
}


/*  Since the lock parameters (z0, lockphase, lockpower, etc.) are in
    the GLOBAL tree and the shim parameters are in the CURRENT tree,
    we try both trees for each parameter.				*/

static int	 tree_array[ 3 ] = { CURRENT, GLOBAL, SYSTEMGLOBAL };

#define  INTEGER	1
#define  REAL_NUMBER	2

#define  MAIN_INDEX	1
#define  AUX_INDEX	(MAIN_INDEX+1)
#define  CUR_TREE	(AUX_INDEX+1)
#define  VNMR_VALUE	(CUR_TREE+1)
#define  HW_VALUE	(VNMR_VALUE+1)
#define  VAL_IS_VALID	(HW_VALUE+1)
#define  NEW_VAL_STORED (VAL_IS_VALID+1)
#define  TYPE_OF_ENTRY	(NEW_VAL_STORED+1)

struct req_hw_value {
	double	dval;
	int	type_of_entry;
	int	main_index;
	int	aux_index;
	int	cur_tree;
	int	vnmr_value;
	int	hw_value;
	int	val_is_valid;		/* refers to `hw_value' field.	*/
	int	new_val_stored;
};

static struct req_hw_value	cur_req_table[ MAX_REQUESTS ];

static void init_req_table()
{
	int	iter;

	for (iter = 0; iter  < MAX_REQUESTS; iter++) {
		cur_req_table[ iter ].main_index = -1;
		cur_req_table[ iter ].val_is_valid = FALSE;
		cur_req_table[ iter ].new_val_stored = FALSE;
	}

	cur_req_num = 0;
}

static int
get_request_table_item(int index, int item, int *retaddr )
{
	int	retval;

	if (index < 0 || index >= MAX_REQUESTS)
	  return( ERROR );

	switch (item) {
	  case MAIN_INDEX:
		retval = cur_req_table[ index ].main_index;
		break;

	  case AUX_INDEX:
		retval = cur_req_table[ index ].aux_index;
		break;

	  case CUR_TREE:
		retval = cur_req_table[ index ].cur_tree;
		break;

	  case VNMR_VALUE:
		retval = cur_req_table[ index ].vnmr_value;
		break;

	  case HW_VALUE:
		retval = cur_req_table[ index ].hw_value;
		break;

	  case VAL_IS_VALID:
		retval = cur_req_table[ index ].val_is_valid;
		break;

	  case NEW_VAL_STORED:
		retval = cur_req_table[ index ].new_val_stored;
		break;

	  case TYPE_OF_ENTRY:
		retval = cur_req_table[ index ].type_of_entry;
		break;

	  default:
		return( ERROR );
	}

	*retaddr = retval;
	return( COMPLETE );
}

static int
get_request_table_realnumber(int index, double *valaddr )
{
	if (index < 0 || index >= MAX_REQUESTS)
	  return( ERROR );

	if (cur_req_table[ index ].type_of_entry != REAL_NUMBER)
	  return( ERROR );

	*valaddr = cur_req_table[ index ].dval;
	return( COMPLETE );
}

static int
store_request_table_item(int index, int item, int value )
{
	if (index < 0 || index >= MAX_REQUESTS)
	  return( ERROR );

	switch (item) {
	  case MAIN_INDEX:
		cur_req_table[ index ].main_index = value;
		break;

	  case AUX_INDEX:
		cur_req_table[ index ].aux_index = value;
		break;

	  case CUR_TREE:
		cur_req_table[ index ].cur_tree = value;
		break;

	  case VNMR_VALUE:
		cur_req_table[ index ].vnmr_value = value;
		break;

	  case HW_VALUE:
		cur_req_table[ index ].hw_value = value;
		break;

	  case VAL_IS_VALID:
		cur_req_table[ index ].val_is_valid = value;
		break;

	  case NEW_VAL_STORED:
		cur_req_table[ index ].new_val_stored = value;
		break;

	  default:
		return( ERROR );
	}

	return( COMPLETE );
}


static int
store_sethw_realnumber(int main_index, int aux_index, int cur_tree,
                       double new_value )
{
	if (cur_req_num >= MAX_REQUESTS)
	  return( -1 );

	cur_req_table[ cur_req_num ].main_index = main_index;
	cur_req_table[ cur_req_num ].aux_index  = aux_index;
	cur_req_table[ cur_req_num ].cur_tree = cur_tree;
	cur_req_table[ cur_req_num ].dval = new_value;

	cur_req_table[ cur_req_num ].val_is_valid = FALSE;
	cur_req_table[ cur_req_num ].new_val_stored = FALSE;

	cur_req_table[ cur_req_num ].type_of_entry = REAL_NUMBER;
	cur_req_num++;

	return( 0 );
}

static int
store_sethw_request(int main_index, int aux_index, int cur_tree, int new_value )
{
	if (cur_req_num >= MAX_REQUESTS)
	  return( -1 );

	cur_req_table[ cur_req_num ].main_index = main_index;
	cur_req_table[ cur_req_num ].aux_index  = aux_index;
	cur_req_table[ cur_req_num ].cur_tree = cur_tree;
	cur_req_table[ cur_req_num ].vnmr_value = new_value;

	cur_req_table[ cur_req_num ].val_is_valid = FALSE;
	cur_req_table[ cur_req_num ].new_val_stored = FALSE;

	cur_req_table[ cur_req_num ].type_of_entry = INTEGER;
	cur_req_num++;

	return( 0 );
}
 
static int
store_readhw_request(int main_index, int aux_index )
{
	if (cur_req_num >= MAX_REQUESTS)
	  return( -1 );

	cur_req_table[ cur_req_num ].main_index = main_index;
	cur_req_table[ cur_req_num ].aux_index  = aux_index;

	cur_req_table[ cur_req_num ].cur_tree = 0;
	cur_req_table[ cur_req_num ].val_is_valid = FALSE;
	cur_req_table[ cur_req_num ].type_of_entry = INTEGER;
	cur_req_num++;

	return( 0 );
}

static int
entry_upto_date(int entry_num )
{
	if (entry_num < 0 || entry_num >= cur_req_num)
	  return( 0 );

	if (cur_req_table[ entry_num ].val_is_valid == FALSE)
	  return( 0 );
	if (cur_req_table[ entry_num ].vnmr_value !=
		cur_req_table[ entry_num ].hw_value)
	  return( 0 );

	return( 131071 );
}

static int
number_current_requests()
{
	return( cur_req_num );
}

/*  End of programs for the Current Request Table.  */

#define  WRONG_TABLE	-3
#define  OUT_OF_RANGE	-7
#define  NO_Z5_COILS	-9

#define NUMBER		1
#define STRING		2
#define VTCMD		3
#define SPNRCMD		4
#define STATUS		5
#define NOARG		6


#ifdef VNMRJ
static char *console_idle_params[] = { NULL };
#else 
static char *console_idle_params[] = { "eject", 
				       "lockpower",
				       "lockgain",
				       "lockphase",
				       "lockfreq",
				       "lock",
				       "lockrate",
				       "spin",
				       "spinner",
                                       "liqbear",
                                       "pneufault",
                                       "ipii",
				       "srate",
				       "tune",
				       "vt",
				       "loc",
				       "temp",
                                       "setMasBearSpan",
                                       "setMasBearAdj",
                                       "setMasBearMax",
                                       "setMasActiveSetPoint",
                                       "setMasProfileSetting",
                                       "masOff",
                                       "masOn",
					NULL };
#endif 

static int
is_console_required_idle(char *pname )
{
    char **taddr;

/* A char * may be ++'ed until it points to '\0'
   So a char ** may be ++'ed until it points to NULL.	*/

    taddr = &console_idle_params[ 0 ];
    while (*taddr != NULL)
    {
        if (strcmp( *taddr, pname ) == 0)
          return( 1 );
	else
	  taddr++;
    }

    return( 0 );
}


/*  Keep creating new #define's for situations like spinner and the VT,
    which do not fall into the Yes/No (STRING) or change code, value
    (NUMBER) mode.  Then extend program_acodes_from_table.		*/


static int	 eject_acodes[] = { EJECT, INSERT, EJECTOFF };
static int	 spinner_acodes[] = { BEARON, BEAROFF, BUMPSAMPLE };
/* static int	 tune_acodes[] = { STARTTUNE, STOPTUNE }; */
static int	 lkmode_acodes[] = { LKON, LKOFF };
static int	 pneu_acodes[] = { -1, 0, WARNING_MSG, HARD_ERROR };
static int	 ipii_acodes[] = { -1, 0 };
static char	*on_off_table[] = { "on", "off", "xoff", NULL };
static char	*spinner_table[] = { "on", "off", "bump", NULL };
static char     *pneu_table[] = { "clear", "n", "w", "y", NULL};
static char     *ipii_table[] = { "reset", NULL};

/*
 *  Hardware Function Table has:
 *	Name of parameter - if NULL, look in Auxiliary table
 *      Address of Auxiliary table - if NULL, no such table
 *	characteristic of user's value
 *		Table of Strings (ON/OFF for now)
 *		Numeric value
 *		VT command
 *		spinner command
 *	corresponding VM02 acode
 *
 *  End of Table is found when both name and auxiliary table
 *  are set to NULL.
 *
 *  An Auxiliary table is present when several parameters key
 *  off of the same acode.  If such a table is present, then
 *  3 VM02 acodes are required, the acode in the table, the
 *  index into the auxiliary table and the acode encoding the
 *  value.  The program expects the auxiliary table to be
 *  organized so its index is the (2nd) acode for that
 *  parameter.
 *
 *  If the user's value is looked up in a table, the address
 *  of that table is NOT present in the hardware function
 *  table.  We just use the on/off table by default.
 *
 *  The program assumes those parameters which look up their
 *  value in a Table require only 1 acode for the
 *  corresponding operation.  Thus "eject"/"on" maps to EJECT
 *  and "eject"/"off" maps to INSERT.  The acode field becomes
 *  the address of a table of values; the index is the same as
 *  the index which locates the user-entered value in its
 *  table.  This property is not directly encoded in the
 *  hardware function table; rather it is assumed if the user
 *  value is not numeric.
 *
 *  Other parameters require 2 acodes, one from this table
 *  and one encoding the (numeric) value.
 *
 *  If the user's value is numeric, the corresponding acode
 *  is that numeric value.
 *
 *  Special Notes:
 *    VT is a special case and receives a special characteristic
 *    
 *    TUNE is available only to the Gemini system.  None of the
 *    other options are available to the Gemini
 *								*/

struct hw_funcs {
	char	*name;
	int	 shims;
	int	 userchar;
	int	 acode;
	int	 ok_to_verify;
};

static struct hw_funcs shimlock[] = {
   { "lockpower",0,	    NUMBER, LKPOWER,		      TRUE },
   { "lockgain", 0,	    NUMBER, LKGAIN,		      TRUE },
   { "lockphase",0,	    NUMBER, LKPHASE,		      TRUE },
   {  NULL,	 MAX_SHIMS, NUMBER, SHIMDAC,		      TRUE },
   { "eject",	 0,	    STRING, 0,   TRUE },
   { "spin",	 0,	    NUMBER, SETSPD,		      FALSE },
   { "spinner",	 0,	    SPNRCMD,0, FALSE },
   { "liqbear", 0,	    NUMBER, BEARING,		      FALSE },
   { "srate",	 0,	    NUMBER, SETRATE,		      FALSE },
   { "tune",	 0,	    NUMBER, SET_TUNE,		      FALSE },
   { "vt",	 0,	    VTCMD,  0,			      FALSE },
   { "vtairflow",0,         NUMBER, VTAIRFLOW,                FALSE },
   { "vtairlimits",0,       NUMBER, VTAIRLIMITS,              FALSE },
   { "pneufault",  0,       PNEUFAULT, 0,  FALSE },
   { "ipii",     0,         IPII,   0,     FALSE },
   { "loc",	 0,	    NUMBER, SETLOC,	  	      FALSE },
   { "start",	 0,	    NOARG,  SYNC_FREE,	  	      FALSE },
   { "status",	 0,	    STATUS, SETSTATUS,	  	      FALSE },
   { "temp",	 0,	    VTCMD,  SETTEMP,		      FALSE },
   { "lock",	 0,	    STRING, 0,  FALSE },
   { "lockrate", 0,	    NUMBER, LKRATE,		      FALSE },
   { "lockfreq", 0,	    NUMBER, LKFREQ,		      FALSE },
   { "statrate", 0,	    NUMBER, STATRATE,		      FALSE },
   { "gtunepx",	 0,	    NUMBER, GTUNE_PX,		      TRUE },
   { "gtuneix",	 0,	    NUMBER, GTUNE_IX,		      TRUE },
   { "gtunepy",	 0,	    NUMBER, GTUNE_PY,		      TRUE },
   { "gtuneiy",	 0,	    NUMBER, GTUNE_IY,		      TRUE },
   { "gtunepz",	 0,	    NUMBER, GTUNE_PZ,		      TRUE },
   { "gtuneiz",	 0,	    NUMBER, GTUNE_IZ,		      TRUE },
   { "genable",	 0,	    NUMBER, GPAENABLE,		      TRUE },
   { "gstatus",	 0,	    NUMBER, GPASTATUS,		      TRUE },
   { "setMasBearSpan",	 0, NUMBER, SETBEARSPAN,	      FALSE },
   { "setMasBearAdj",	 0, NUMBER, SETBEARADJ,		      FALSE },
   { "setMasBearMax",	 0, NUMBER, SETBEARMAX,		      FALSE },
   { "setMasActiveSetPoint", 0,	NUMBER, SETASP,		      FALSE },
   { "setMasProfileSetting", 0,	NUMBER, SETPROFILE,	      FALSE },
   { "masOff",	 0,         NUMBER, MASOFF,     	      FALSE },
   { "masOn",	 0,         NUMBER, MASON,      	      FALSE },
   {  NULL,	 0,	    0,      0,			      0 }
};

static struct hw_funcs *hw_tables[] = {
	shimlock,
	NULL
};

static int
find_shim_string(char *string, int num )
{
	int	indx;
        const char   *shim;

	if (num == 0)
	  return( -1 );
	if (string == NULL)
	  return( -1 );
	if (strcmp( "", string ) == 0)
	  return( -1 );
        init_shimnames(SYSTEMGLOBAL);
	for (indx = 0; indx < num ; indx++)
        {
           shim = get_shimname(indx);
	   if ((shim != NULL) && (strcmp( shim, string ) == 0))
		  return( indx );
        }

	return( -1 );
}

static int
find_table_string(char *string, char *table[] )
{
	int	indx;

	if (table == NULL)
	  return( -1 );
	if (string == NULL)
	  return( -1 );

	for (indx = 0; ; indx++) {
		if (table[ indx ] == NULL)
		  break;
		else if (strcmp( table[ indx ], string ) == 0)
		  return( indx );
	}

	return( -1 );
}

/*  This program returns 1 if the sethw program requires the parameter
    to be the only one entered on the command line; 0 otherwise.	*/

static int
is_parameter_required_to_be_alone(int main_index)
{
	int	retval;

	if (shimlock[ main_index ].userchar == VTCMD)
	  retval = 1;
	else
	  retval = 0;

	return( retval );
}

static int
hw_table_lookup(int table_num, char *hw_name, int *main_index_addr,
                int *aux_index_addr )
{
	int		 iter, ival;
	struct hw_funcs *cur_table, *cur_entry;

	*main_index_addr = -1;
	*aux_index_addr = -1;

/*  Prevent segmented violations  */

	if (table_num < 0)
	  return( OFF_DEEP_END );
	for (iter = 0; iter <= table_num; iter++)
	  if (hw_tables[ iter ] == NULL)
	    return( OFF_DEEP_END );

/*  Search the requested table  */

	cur_table = hw_tables[ table_num ];
	for (iter = 0; ; iter++) {
		cur_entry = &cur_table[ iter ];
		if (cur_entry->name == NULL && cur_entry->shims == 0)
		  break;
		else if (cur_entry->shims != 0) {
			ival = find_shim_string( hw_name, cur_entry->shims );
			if (ival > -1) {
				*main_index_addr = iter;
	                        *aux_index_addr = ival;
				return( COMPLETE );
			}
		}
		else if (strcmp( cur_entry->name, hw_name ) == 0) {
			*main_index_addr = iter;
			return( COMPLETE );
		}
	}

	return( NOT_FOUND );
}

/*  This program uses the Current Request Table AND the Hardware Function
    Table.  We cheat a bit in that we get the main index and the auxiliary
    index direct from the table, rather than using the Request Table Interface.

    This program assumes there is only 1 Hardware Function Table, shim/lock.

    Since the program returns an address in the Hardware Function Table or
    the shim names table, do not modify the value at that address.		*/

static const char *
request_index_to_param_name(int index )
{
	int		 main_index, aux_index;
	struct hw_funcs *cur_entry;

	if (index < 0 || index >= cur_req_num) {
		Werrprintf(
   "program error in request index to parameter name, index out of range"
		);
		return( NULL );
	}

	main_index = cur_req_table[ index ].main_index;
	if (main_index < 0) {
		Werrprintf(
   "program error in request index to parameter name, main index out of range"
		);
		return( NULL );
	}
	aux_index  = cur_req_table[ index ].aux_index;

	cur_entry = &shimlock[ main_index ];
	if (cur_entry->shims > 0)
          return( get_shimname( aux_index ) );
	else if (cur_entry->name != NULL)
	  return( cur_entry->name );
	else
	  return( NULL );
}

/*  End of programs for the Hardware Function Tables.	*/

static int	transp_command_index;
static int	transparent_command[ 254 ];

static int
init_transparent_command()
{
	transp_command_index = 1;
	return( 0 );
}

static int
insert_acode_in_transparent(int acode )
{
	if (transp_command_index >= (int) sizeof( transparent_command ) /(int) sizeof( int ))
	  return( -1 );

	transparent_command[ transp_command_index ] = acode;
	transp_command_index++;
	return( 0 );
}

static int
transparent_to_ascii(char *msgptr, int msglen )
{
	int	curlen, iter, quicklen;
	char	quickstr[ 12 ];

	curlen = 0;
	*msgptr = '\0';
        transparent_command[ 0 ] = transp_command_index - 1;
	for (iter = 0; iter < transp_command_index; iter++) {
		sprintf( &quickstr[ 0 ], "%d,", transparent_command[ iter ] );
		quicklen = strlen( &quickstr[ 0 ] );
		if (curlen + quicklen >= msglen)
		  return( -1 );

		strcat( msgptr, &quickstr[ 0 ] );
		curlen += quicklen;
	}

	return( 0 );
}

/*  Generic verification of acquisition hardware commands.  */

static int verify_hwcmd(char *argv[], int quiet )
{
	if (is_datastation())
        {
	    if (!quiet) {
		Werrprintf( "%s:  available on spectrometer only", argv[ 0 ] );
	    }
	    return( -1 );
	}
	if ( mode_of_vnmr != AUTOMATION )
        {
		int ival;

		ival = ok_to_acquire();
                if (ival != -2)
                    release_console2();
                if ( ! strcmp(argv[0], "readhw") && ( (ival == -3) || (ival == -8) ) )
                    ival = 0;
		if ( (ival != 0) && (ival != -4) )  /* okay if autoproc running */
                {
                   if (!quiet)
			report_acq_perm_error( argv[ 0 ], ival );
		    return( ival );
		}
	}

	return( 0 );
}

/*  See comment where static array `tree_array' is defined.  */

static int find_tree(char *param_name, int *cur_tree_addr )
{
	int	cur_tree, iter, ival;
	vInfo	var_info;

	for (iter = 0; iter < (int) sizeof( tree_array ) / (int) sizeof( int ); iter++) {
		cur_tree = tree_array[ iter ];
		ival = P_getVarInfo( cur_tree, param_name, &var_info );
		if (ival == 0) {
			*cur_tree_addr = cur_tree;
			return( COMPLETE );
		}
	}

	return( ERROR );
}

static int
scan_for_arg(int argc, char *argv[], char *arg_val)
{
	int	iter;

	for (iter = 0; iter < argc; iter++)
	  if (strcmp( arg_val, argv[ iter ] ) == 0)
	    return( iter );

	return( -1 );					/* which means not found */
}

#define  NUMBER_OF_KEYWORDS	2
#define  WAIT			0
#define  NO_WAIT		1

/*  As a side effect, this routines sets `sethw_wait'	*/

static int
verify_keywords_sethw(int argc, char *argv[], int *first_arg_addr,
                      int *new_argc_addr )
{
	int	 cur_index, first_arg, iter, new_argc;
	int	 keyword_var[ NUMBER_OF_KEYWORDS ];
	char	*keyword_vec[ NUMBER_OF_KEYWORDS ];

	keyword_var[ WAIT ] = 0;
	keyword_vec[ WAIT ] = "wait";
	keyword_var[ NO_WAIT ] = 0;
	keyword_vec[ NO_WAIT ] = "nowait";

	first_arg = 1;
	new_argc = argc;

	for (iter = 0; iter < NUMBER_OF_KEYWORDS; iter++) {
		cur_index = scan_for_arg( argc, argv, keyword_vec[ iter ]);
		if (cur_index < 0)
		  continue;

		if (iter > 0 && keyword_var[ 0 ] != 0) {
			Werrprintf(
	    "Set hardware:  cannot use both %s and %s arguments",
	     keyword_vec[ 0 ], keyword_vec[ iter ]
			);
			return( -1 );
		}
		if (cur_index != 1 && cur_index != argc-1) {
			Werrprintf(
    "Set hardware:  `%s' must be the first or last argument", keyword_vec[ iter ]
			);
			return( -1 );
		}

		keyword_var[ iter ] = 1;
		if (cur_index == 1)
		  first_arg = 2;
		else
		  new_argc--;
	}

/*  The default is to wait for the set hardware operation to complete.  */

	if (keyword_var[ NO_WAIT ] != 0)
	  sethw_wait = 0;
	else
	  sethw_wait = 131071;

	*first_arg_addr = first_arg;
	*new_argc_addr  = new_argc;
	return( 0 );
}

static int
verify_value_from_name(char *pname, vInfo *vinfoaddr, int value )
{
	double	shimMax, shimMin;

	if (vinfoaddr == NULL)
	  return( COMPLETE );		/* Check to prevent segmented violations. */

/*  Following taken from shimdisplay.c in the ACQI programs.  */

	if (vinfoaddr->prot & P_MMS) {	/* if max, min, step are to be found in */
		int	pindex;			  /* parmax, parmin and parstep */
		double	tmpval;

		pindex = (int) (vinfoaddr->minVal + 0.1);
		if (P_getreal( SYSTEMGLOBAL, "parmin", &tmpval, pindex ))
		  tmpval = -2048.0;
		shimMin = tmpval;
		pindex = (int) (vinfoaddr->maxVal + 0.1);
		if (P_getreal( SYSTEMGLOBAL, "parmax", &tmpval, pindex ))
		  tmpval = 2047.0;
		shimMax = tmpval;
	}
	else
	{
		shimMin = vinfoaddr->minVal;
		shimMax = vinfoaddr->maxVal;
	}

	if ((double) value < shimMin ) {
		Werrprintf(
    "Set hardware: value of %d for %s is too small", value, pname
		);
		return( OUT_OF_RANGE );
	}
	else if ((double) value > shimMax) {
		Werrprintf(
    "Set hardware: value of %d for %s is too large", value, pname
		);
		return( OUT_OF_RANGE );
	}

	return( COMPLETE );
}

/*  This program is barely more than a Stub.  It responds to "reset"
    only.  The parameter name is provided in anticipation that several
    parameters may get service from this program.

    Special Note:  The host computer (this program) assumes the reset
    operation works.  If requested to wait (and verify), it goes ahead
    and requests an interactive stat block (get_ia_stat).  But it does
    not check anything concerning VT, utilizing the fact that VT is not
    a parameter with a tree.  See process_ia_stat and store_hw_values.

                  * * * * * * * * * * * * * * * * * * * *

    Extended to implement "temp" as a parameter.  Although numeric,
    temp is special because we actually want a floating point value
    to permit precision to the nearest 0.1 of a degree.  We send the
    console an integer value by multiplying the submitted value by 10.	*/

static int
program_vt(char *pname, char *valstr, vInfo *vinfoaddr,
           int *want_to_sleep_addr )
{
	int		ival, nval, nvtc;
	double		fval, vtc;

	if (P_getreal( CURRENT, "vtc", &vtc, 1 ) != 0) {
		Werrprintf(
	   "Set hardware cannot program the VT when the vtc parameter is not defined"
		);
		return( ERROR );
	}
	nvtc = (int) (vtc * 10.0);

	if (strcmp( "temp", pname ) == 0) {
		fval = atof( valstr );
		if (vinfoaddr != NULL) {
			ival = verify_value_from_name( pname, vinfoaddr, (int) fval );
			if (ival != COMPLETE)
			  return( ERROR );
		}
		nval = (int) (fval * 10.0);

		ival = insert_acode_in_transparent( SETTEMP );
		ival = insert_acode_in_transparent( nval );
		ival = insert_acode_in_transparent( nvtc );
                setInfoTempSetPoint(nval);
                setInfoTempOnOff(1);
	}
	if (strcmp( "vt", pname ) == 0) {
		if (strcmp( valstr, "reset" ) == 0) {
			ival = insert_acode_in_transparent( RESET_VT );
			if (ival != 0) {
				Werrprintf(
	   "Set hardware:  too many parameters in console command"
				);
				return( ERROR);
			}
			*want_to_sleep_addr = TRUE;
		}
		else if (strcmp( valstr, "off" ) == 0) {
			ival = insert_acode_in_transparent( SETTEMP );
			ival = insert_acode_in_transparent( 30000 );	/* to turn if off */
			ival = insert_acode_in_transparent( nvtc );
			if (ival != 0) {
				Werrprintf(
	   "Set hardware:  too many parameters in console command"
				);
				return( ERROR);
			}
			*want_to_sleep_addr = FALSE;
		}
		else {
			Werrprintf(
	   "Set hardware:  can't use '%s' with the %s parameter", valstr, pname
			);
			return( ERROR );
		}
	}

	return( COMPLETE );
}

/*
   Note:  The host application inserts a count of the total number of
           change codes (codes and parameters) but does not preface any
           individual change code operation with the number of codes for
           that operation.
 */

static int
program_acodes_from_table(char *pname, char *valstr, vInfo *vinfoaddr,
          struct hw_funcs *cur_entry, int aux_index, int *valaddr )
{
	int		 acode_val, ival, nval;
	int		*acode_table;
	short		 svals[ 2 ];
	double		 dval;

/*  At this time strings are expected to be either "on" or "off".  The
    corresponding entry in the A-code table gives the A-code.
    Extended to work with spinner commands: yes, no and bump.
 */

	switch (cur_entry->userchar) {

          case IPII:
	  case STRING:
          case PNEUFAULT:
	  case SPNRCMD:
		if (cur_entry->userchar == SPNRCMD)
                {
		  nval = find_table_string( valstr, spinner_table );
		  acode_table = &spinner_acodes[0];
                }
                else if (cur_entry->userchar == PNEUFAULT)
                {
                  nval = find_table_string( valstr, pneu_table );
		  acode_table = &pneu_acodes[0];
                }
                else if (cur_entry->userchar == IPII)
                {
                  nval = find_table_string( valstr, ipii_table );
		  acode_table = &ipii_acodes[0];
                }
		else
                {
		  nval = find_table_string( valstr, on_off_table );
                  acode_table = (strcmp( cur_entry->name, "lock" )) ?
                                 &eject_acodes[0] : &lkmode_acodes[0]; 
                }
		if (nval < 0) {
			Werrprintf(
	   "Set hardware:  can't use '%s' with the %s parameter", valstr, pname
			);
			return( ERROR );
		}
		acode_val = acode_table[ nval ];
                if ( (acode_val == BEARON) || (acode_val == BEAROFF) )
                {
                   nval = (acode_val == BEAROFF) ? 0 : getInfoSpinSetSpeed();
                   setInfoSpinOnOff((nval == 0) ? 0 : 1);
                   /* Select high speed or low speed spinner first */
		   ival = insert_acode_in_transparent( SETMASTHRES );
		   ival = insert_acode_in_transparent( getInfoSpinner() );
		   ival = insert_acode_in_transparent( SETSPD );
		   ival = insert_acode_in_transparent( nval );
                }
                if (strcmp( cur_entry->name, "lock" ) == 0)
		{
		   ival = insert_acode_in_transparent( LKMODE );
		   ival = insert_acode_in_transparent( acode_val );
		}
                else if (strcmp( cur_entry->name, "pneufault") == 0)
                {  ival = insert_acode_in_transparent( PNEUFAULT );
                   ival = insert_acode_in_transparent( acode_val );
                }
                else if (strcmp( cur_entry->name, "ipii") == 0)
                {  ival = insert_acode_in_transparent( IPII );
                   ival = insert_acode_in_transparent( acode_val );
                }
                else
                {
		   ival = insert_acode_in_transparent( acode_val );
                }
		if (ival != 0) {
			Werrprintf(
	   "Set hardware:  too many parameters in console command"
			);
			return( ERROR );
		}
		break;

	  case VTCMD:

	    /* program vt squawks if something goes wrong  */

		ival = program_vt( pname, valstr, vinfoaddr, &want_to_sleep );
		if (ival != COMPLETE)
		  return( ERROR );
		nval = 0;			/* default/dummy value required */
		break;

	  case NUMBER:

		if (strcmp( pname, "lockfreq" ) == 0) {
                   if ( ! nvAcquisition() ) {
			dval = atof( valstr );
			nval = getlkfreqapword( dval, get_h1freq() );
			if (nval < 0) {
				Werrprintf(
		   "Set hardware: cannot set lock frequency with this command on this system"
				);
				return( ERROR );
			}
			oneLongToTwoShort( nval, &svals[ 0 ] );

		/*  The AP Bus observes the DEC convention of the low order
		    byte/word first, the high order byte/word second, opposite
		    the SPARC/Motorola convention.				*/

			ival = insert_acode_in_transparent( cur_entry->acode );
			if (ival == 0)
			  ival = insert_acode_in_transparent( svals[ 1 ] );
			if (ival == 0)
			  ival = insert_acode_in_transparent( svals[ 0 ] );
			if (ival != 0) {
				Werrprintf(
		   "Set hardware:  too many parameters in console command"
				);
				return( ERROR );
			}
			return( COMPLETE );
                   }
		   else {
                        int parts[2];
                        dval = atof( valstr );
                        getlkfreqDDsi(dval, get_h1freq(), parts);
                        ival = insert_acode_in_transparent( cur_entry->acode );
                        if (ival == 0)
                          ival = insert_acode_in_transparent( parts[ 0 ] );
                        if (ival == 0)
                          ival = insert_acode_in_transparent( parts[ 1 ] );
                        if (ival != 0) {
                                Werrprintf(
                   "Set hardware:  too many parameters in console command"
                                );
                                return( ERROR );
                        }
                        return( COMPLETE );
                   }
		}

		nval = atoi( valstr );

		if (vinfoaddr != NULL) {
			ival = verify_value_from_name( pname, vinfoaddr, nval );
			if (ival != COMPLETE)
			  return( ERROR );
		}
                if (cur_entry->acode == SETSPD)
                {
                   /* record new spin speed */
                   setInfoSpinSetSpeed(nval);
                   setInfoSpinOnOff((nval == 0) ? 0 : 1);
                   /* Select high speed or low speed spinner first */
		   ival = insert_acode_in_transparent( SETMASTHRES );
		   ival = insert_acode_in_transparent( getInfoSpinner() );
                }
                else if (cur_entry->acode == SETRATE)
                {
                   /* record new spin speed */
                   setInfoSpinSetRate(nval);
                   setInfoSpinOnOff((nval == 0) ? 0 : 1);
                   /* Select high speed or low speed spinner first */
		   ival = insert_acode_in_transparent( SETMASTHRES );
		   ival = insert_acode_in_transparent( getInfoSpinner() );
                }
                else if (cur_entry->acode == SHIMDAC)
                {
                   if (check_ShimPowerHw(aux_index, &nval))
                   {
                      Werrprintf("Set hardware: shim current limit would be exceeded");
                   }
                }
		ival = insert_acode_in_transparent( cur_entry->acode );
		if (aux_index > -1) {
			ival = insert_acode_in_transparent( aux_index );
		}
		ival = insert_acode_in_transparent( nval );
		if (ival != 0) {
			Werrprintf(
	   "Set hardware:  too many parameters in console command"
			);
			return( ERROR );
		}
		break;

	  case NOARG:
		ival = insert_acode_in_transparent( cur_entry->acode );
		nval = 0;
		break;
	  case STATUS:
		ival = insert_acode_in_transparent( cur_entry->acode );
		if ( ! strcmp(valstr,"idle") )
                   nval = ACQ_IDLE;
		else if ( ! strcmp(valstr,"shim") )
                   nval = ACQ_SHIMMING;
                else if ( ( ival = (int)  strtol(valstr, (char **) NULL, 10) ) )
                   nval = ival;
                else
                   nval = ACQ_PREP;
		ival = insert_acode_in_transparent( nval );
		break;
	  default:
		Werrprintf( "Program error in set hardware. No %d case.",cur_entry->userchar );
		return( ERROR );
		break;
	}

/*  For strings, the value stored here is the index into
    the table of strings (currently only the on/off table).

    For numbers, the value stored here is the actual new value.  */

	*valaddr = nval;
	return( COMPLETE );
}

static int make_sethw_report(char *acqproc_msg, int quiet )
{
	char	preamble[ 20 ], user_report[ 80 ];

	vnmr_tolower( acqproc_msg );
	if (strcmp( acqproc_msg, "started" ) == 0)
	  return( 0 );

	strcpy( &preamble[ 0 ], "set hardware" );

/*  The reply from Acqproc indicates an error has occurred. 
    Check for expected situations.				*/

	if (strcmp( acqproc_msg, "automation" ) == 0) {
		sprintf( &user_report[ 0 ],
		   "Cannot %s, spectrometer in automation mode", &preamble[ 0 ]
		);
	}

/* Expproc (INOVA) may tell us the experiment ID,
   so only compare upto the length of "acquiring".  */

	else if (strncmp( acqproc_msg, "acquiring", strlen( "acquiring" ) ) == 0) {
		sprintf( &user_report[ 0 ],
		   "Cannot %s, acquisition in progress", &preamble[ 0 ]
		);
	}
	else if ((strcmp( acqproc_msg, "interactive" ) == 0) ||
		 (strcmp( acqproc_msg, "shimming" ) == 0)) {
		sprintf( &user_report[ 0 ],
		   "Cannot %s during interactive acquisition", &preamble[ 0 ]
		);
	}
	else {

/*  %.40s serves to truncate the output at 40 characters.	*/

		sprintf(
			&user_report[ 0 ],
			"%s:  Acquisition responded %.40s",
			&preamble[ 0 ], acqproc_msg
		);
	}
	if (!quiet)
	   Werrprintf( &user_report[ 0 ] );
	return( -1 );
}

static int
get_console_status(char *status_buf, int status_len )
{
    int	ival;

    ival = talk_to_acqproc( QUERYSTATUS, "", status_buf, status_len );
    if (ival != 0)
      return( -1 );
    else
      return( 0 );
}

static int
is_console_idle(char *console_status, int status_len )
{
    int ival;

    ival = get_console_status( console_status, status_len );
    if (ival != 0)
      return( 0 );
    else
      return( strcmp( console_status, "IDLE" ) == 0 );
}

static int
make_transparent_command(int argc, char *argv[], char *msgptr,
                         int msglen, int quiet )
{
    char	 errmsg[128];
    int		 cur_tree, iter, ival, nval, main_index, aux_index;
    int		 param_alone_flag;
    char		 console_status[ 256 ];
    vInfo		*vinfoaddr, var_info;
    struct hw_funcs *cur_table;

    cur_table = hw_tables[ 0 ];	/* there is only one table, for now */
    init_transparent_command();

    for (iter = 0; iter < argc; iter += 2) {
	/*
	 * On INOVA, most parameters (shims) may be modified even as
	 * an acquisition proceeds.  A few cannot (eject, anything
	 * requiring access to the FIFO).  Older systems do not allow
	 * any set hardware operations unless the console is idle.
	 * This is not checked though until the talk-to-acqproc
	 * operation.  See the main sethw program, below.
	 * If we get a parameter that is not to be modified on INOVA
	 * unless the system is idle, check this out here.  It does
	 * not matter whether this VNMR is running on INOVA or some
	 * other system, since the other systems would block the
	 * operation anyway.
	 */
	if (is_console_required_idle( argv[ iter ] )) {
	    if (is_console_idle( &console_status[ 0 ],
				 sizeof( console_status ) - 1) == 0)
	    {
		/*
		 * is_console_idle returns the message from
		 * Acqproc/Expproc to the calling program.  This
		 * message is expected to be the same as if a set
		 * hardware operation itself was attempted.  Thus if
		 * the console is not idle, use make_sethw_report to
		 * report the problem.
		 */
		make_sethw_report( &console_status[ 0 ], quiet );
		return( ERROR );
	    }
	}
	/*
	 * The first argument to hardware table lookup tells which
	 * table to search.  Since there is only one table for now,
	 * its value is 0.
	 */
	ival = hw_table_lookup( 0, argv[ iter ], &main_index, &aux_index );
	if (ival != COMPLETE) {
	    Werrprintf("Set hardware:  %s parameter not available\n",
		       argv[ iter ]);
	    return( ERROR );
	}

	/*
	 * New addition, December 1992.  This allows the programmer to
	 * specify that certain parameters should not appear on the
	 * same command line with other parameters.
	 */

	param_alone_flag = is_parameter_required_to_be_alone( main_index);
	if (param_alone_flag && argc > 2) {
	    strcpy(errmsg, "Set hardware:  %s parameter must be");
	    strcat(errmsg, " the only one in the command line\n");
	    Werrprintf(errmsg, argv[ iter ]);
	    return( ERROR );
	}

	ival = find_tree( argv[ iter ], &cur_tree );
	if (ival == COMPLETE) {
	    if (P_getVarInfo( cur_tree, argv[ iter ], &var_info ) != 0)
		vinfoaddr = NULL;
	    else
		vinfoaddr = &var_info;
	}
	else {
	    vinfoaddr = NULL;
	    cur_tree = NOT_FOUND;
	}
			
	nval = 0;
	ival = program_acodes_from_table(
					 argv[ iter ],
					 argv[ iter+1 ],
					 vinfoaddr,
					 &cur_table[ main_index ],
					 aux_index,
					 &nval
					 );
	if (ival != COMPLETE)
	    return( ERROR );

	/*
	 * At this point we assume:
	 * main_index is the main index value
	 * aux_index is the auxiliary index
	 * nval encodes the (new value):
	 *  directly for numbers, indirectly for strings.
	 */

	if (cur_table[ main_index ].ok_to_verify == TRUE) {
	    ival = store_sethw_request( main_index, aux_index, cur_tree, nval );
	    if (ival != 0) {
		strcpy(errmsg, "Set hardware: set no more than");
		strcat(errmsg, " %d parameters per command");
		Werrprintf(errmsg, MAX_REQUESTS);
		return( ERROR );
	    }
	}
	else if (strcmp( argv[ iter ], "lockfreq") == 0) {
	    ival = store_sethw_realnumber(
					  main_index,
					  aux_index,
					  cur_tree,
					  atof( argv[ iter + 1 ] )
					  );
	    if (ival != 0) {
		strcpy(errmsg, "Set hardware: set no more than");
		strcat(errmsg, " %d parameters per command");
		Werrprintf(errmsg, MAX_REQUESTS);
		return( ERROR );
	    }
	}
    }

    if (transparent_to_ascii( msgptr, msglen ) != 0) {
	strcpy(errmsg, "Set hardware:  You tried to set");
	strcat(errmsg, " too many parameters in one command");
	Werrprintf(errmsg);
	return( ERROR );
    }

    return( COMPLETE );
}

#ifdef XXX
static int tune_eject_arg(int argc, char *argv[] )
{
   int argfound = 0;
   int otherfound = 0;
   int iter;

   for (iter = 1; iter < argc; iter += 2)
   {

/*  Look for permitted arguments  */

      if (!argfound)
	argfound = ( (strcmp( argv[ iter ], "tune")  == 0) ||
	             (strcmp( argv[ iter ], "eject") == 0) ||
	             (strcmp( argv[ iter ], "spinner") == 0) ||
	             (strcmp( argv[ iter ], "temp") == 0) ||
	             (strcmp( argv[ iter ], "vt") == 0) ||
	             (strcmp( argv[ iter ], "spin")  == 0) );

/*  Look for disallowed arguments  */

      if (!otherfound)
	otherfound = ( !((strcmp( argv[ iter ], "tune")  == 0) ||
	                 (strcmp( argv[ iter ], "eject") == 0) ||
	                 (strcmp( argv[ iter ], "spinner") == 0) ||
	                 (strcmp( argv[ iter ], "temp") == 0) ||
	                 (strcmp( argv[ iter ], "vt") == 0) ||
	                 (strcmp( argv[ iter ], "spin")  == 0)) );
   }
   if (argfound && otherfound)
      return(-1);
   return(argfound);
}
#endif

static int find_arg(int argc, char *argv[], char *arg )
{
   int found = 0;
   int iter;

   for (iter = 1; iter < argc; iter += 2)
   {
      if (!found)
	found = (strcmp( argv[ iter ], arg) == 0);
   }
   return(found);
}


/*  Routine to verify SETHW command and prepare
    the command string to send to Acqproc.	*/

static int verify_sethw(int argc, char *argv[], char *msgptr,
                        int msglen, int quiet )
{
	int	first_arg;
        int     robot_arg;
	char	rftype[ 12 ];

/*  Defensive programming  */

	if (argc < 1) {
                Werrprintf( "logic error in verify_sethw" );
		return( -1 );
	}
	if (msglen < 7) {
                Werrprintf( "internal buffer too short in verify_sethw" );
		return( -1 );
	}

	if (strcmp( "sethw", argv[ 0 ] ) != 0) {
		Werrprintf( "%s:  command not supported", argv[ 0 ] );
		return( -1 );
	}

/*  Now we know the command is "sethw".  The arguments contain the
    parameter names and their desired values.  Thus there must be at
    least two arguments, for we expect an even number of arguments
    (parameter name, parameter value).

    Error messages do not use the command name (SETHW) so the messages
    can be more user-friendly.						*/

	if (argc < 2) {
		Werrprintf( "Set hardware command requires arguments" );
		return( -1 );
	}

/*  Look for keywords (wait, nowait).  If verify keywords (for)
    sethw returns -1, it displayed an error message.		*/

	first_arg = 1;
	if (verify_keywords_sethw( argc, argv, &first_arg, &argc ) != 0)
	  return( -1 );

/*  Now verify the number of remaining arguments is even.  */

	if (argc - first_arg <= 0) {
	  	Werrprintf(
	    "Set hardware:  parameter name and value required with a keyword"
		);
		return( -1 );
	}
	else if ((argc - first_arg) % 2 != 0) {
		Werrprintf(
	    "Set hardware requires parameter name, value pairs as arguments"
		);
		return( -1 );
	}
	if (getparm("rftype","string",SYSTEMGLOBAL,rftype,10)) {
		Werrprintf("Cannot find parameter 'rftype'");
		return(-1);
	}

/*  sethw used to handle the Gemini/Krikkit as a special case and only allow
    a limited range of arguments.  The low-cost systems are still a special
    case, but only because they allow "tune" as an argument.			*/

#if 0
        if ((rftype[0] == 'e') || (rftype[0] == 'f') )
        {
            if (tune_eject_arg(argc,argv) != 1)
            {
		Werrprintf("sethw can only be used for tune or eject on this system");
		return(-1);
            }
        }
        else
        {
            if (find_arg(argc,argv,"tune") != 0)
            {
		Werrprintf("sethw can not be used for tune on this system");
		return(-1);
            }
        }
#endif 

/*  The replacement programs are here ... */

        if ((rftype[0] != 'e') && (rftype[0] != 'f') )
        {
            if (find_arg(argc,argv,"tune") != 0)
            {
                if (!quiet)
		   Werrprintf("sethw can not be used for tune on this system");
		return(-1);
            }
        }

	init_req_table();
	cur_table_entry = -1;				/* no current table */

        if ( (robot_arg = find_arg(argc,argv,"robot") ) != 0)
        {
           if (robot_arg+1 < argc)
           {
              if ((int) strlen(argv[robot_arg+1]) > msglen - 20)
              {
                 Werrprintf("sethw('robot') command '%s' is too long", argv[robot_arg+1] );
	         return( -1 );
              }
              if ( ! strcmp(argv[robot_arg+1],"term") )
                 sethw_wait = 0;
              if (sethw_wait)
              {
                 /* remove any previous results */
                 sprintf(msgptr,"%s/sethw", curexpdir);
                 unlink(msgptr);
                 /* Added semi-colons to facilitate parsing */
                 sprintf(msgptr,"userCmd outfile %s/sethw; %s;",
                         curexpdir,argv[robot_arg+1]);
              }
              else
              {
                 sprintf(msgptr,"userCmd %s",argv[robot_arg+1]);
              }
	      return( 0 );
           }
           else
           {
              Werrprintf("sethw('robot') requires a second argument");
	      return( -1 );
           }
        }
        else
        {
	   if (make_transparent_command(argc - first_arg,
		&argv[ first_arg ], msgptr, msglen, quiet) != COMPLETE)
	      return( -1 );
        }

	return( 0 );
}

/*  verify data valid receives a preamble argument since it is called from
    both readhw and sethw, the latter to begin verifying returned values.	*/

static int
verify_data_valid(char *preamble)
{
   int valid_data;

   getExpStatusInt(EXP_VALID, &valid_data);
/*  First check for valid data and return COMPLETE if so.  */

	if ((valid_data == INIT_STRUC) ||
	    (valid_data == LK_STRUC) ||
	    (valid_data == EXP_STRUC) )
	  return( COMPLETE );

/*  Come here if the data is not valid.  Find out the problem and report it.	*/

	switch (valid_data) {
	  case EXP_ACTIVE:
		Werrprintf( "%s: acquisition in progress", preamble );
		break;

	  case ACQI_ACTIVE:
		Werrprintf( "%s: interactive acquisition active", preamble );
		break;

	  case CVT_ERROR:
		Werrprintf(
	   "%s: Acquisition did not recognize the command from VNMR", preamble);
		break;

	  case HAL_ERR1:
		Werrprintf( "%s: HAL error 1", preamble );
		break;

	  case HAL_ERR2:
		Werrprintf( "%s: HAL error 2", preamble );
		break;

	  case HAL_ERR3:
		Werrprintf( "%s: HAL error 3", preamble );
		break;

	  case HAL_ERR4:
		Werrprintf( "%s: HAL error 4", preamble );
		break;

	  default:
		Werrprintf(
	   "%s:  unrecognized status %d", preamble, valid_data);
		break;
	}

	return( ERROR );
}

/*  Get the lock-system value from the shim/lock block
    This program is not suitable for shims; the auxiliary index
    is the index into the tables of shims in the shim/lock
    block, so one should use it directly.			*/

static int
name_to_stat_val(char *pname, int *valaddr )
{
	int	retval;

	if (strcmp( pname, "lockpower" ) == 0) {
                getExpStatusInt(EXP_LKPOWER, valaddr);
		retval = COMPLETE;
	}
	else if (strcmp( pname, "lockgain" ) == 0) {
                getExpStatusInt(EXP_LKGAIN, valaddr);
		retval = COMPLETE;
	}
	else if (strcmp( pname, "lockphase" ) == 0) {
                getExpStatusInt(EXP_LKPHASE, valaddr);
		retval = COMPLETE;
	}
	else if (strcmp( pname, "gtunepx" ) == 0) {
                getExpStatusInt(EXP_GTUNEPX, valaddr);
		retval = COMPLETE;
	}
	else if (strcmp( pname, "gtuneix" ) == 0) {
                getExpStatusInt(EXP_GTUNEIX, valaddr);
		retval = COMPLETE;
	}
	else if (strcmp( pname, "gtunesx" ) == 0) {
                getExpStatusInt(EXP_GTUNESX, valaddr);
		retval = COMPLETE;
	}
	else if (strcmp( pname, "gtunepy" ) == 0) {
                getExpStatusInt(EXP_GTUNEPY, valaddr);
		retval = COMPLETE;
	}
	else if (strcmp( pname, "gtuneiy" ) == 0) {
                getExpStatusInt(EXP_GTUNEIY, valaddr);
		retval = COMPLETE;
	}
	else if (strcmp( pname, "gtunesy" ) == 0) {
                getExpStatusInt(EXP_GTUNESY, valaddr);
		retval = COMPLETE;
	}
	else if (strcmp( pname, "gtunepz" ) == 0) {
                getExpStatusInt(EXP_GTUNEPZ, valaddr);
		retval = COMPLETE;
	}
	else if (strcmp( pname, "gtuneiz" ) == 0) {
                getExpStatusInt(EXP_GTUNEIZ, valaddr);
		retval = COMPLETE;
	}
	else if (strcmp( pname, "gtunesz" ) == 0) {
                getExpStatusInt(EXP_GTUNESZ, valaddr);
		retval = COMPLETE;
	}
	else if (strcmp( pname, "gstatus" ) == 0) {
                getExpStatusInt(EXP_GSTATUS, valaddr);
		retval = COMPLETE;
        }
	else if (strcmp( pname, "genable" ) == 0) {
                *valaddr = -1;
		retval = COMPLETE;
	}
	else if (strcmp( pname, "eject" ) == 0) {
                *valaddr = 0;
		retval = COMPLETE;
	}
	else {
                retval = ERROR;
        }

	return( retval );
}

static int
process_ia_stat( )
{
    int hwval, iter, ival, main_index, aux_index, type_of_entry;
    struct hw_funcs *cur_table, *cur_entry;
    char errmsg[128];

    /*  Check for a software error internal to VNMR.  */
    if (cur_req_num < 1 || cur_req_num >= MAX_REQUESTS) {
	strcpy(errmsg,"Programming error in process shim/lock block");
	if (cur_req_num < 1) {
	    strcat(errmsg,", no current requests");
	}
	else {
	    strcat(errmsg,", too many current requests");
	}
	Werrprintf(errmsg);
	return( ERROR );
    }

    /*
     * Warning: This program is a Hack, quite dependent on the
     * structure of the shim/lock block, filled with assumptions about
     * that block and the fact that it only processes shim/lock
     * blocks.
     *
     * It is not as bad as before.  It still assumes it works with a
     * shim/lock data block.  The aux_index may be an index into the
     * sh_dacs.  See also name_to_stat_val, defined above.
     */

    cur_table = hw_tables[ 0 ];		/* there is only one table, for now */

    for (iter = 0; iter < cur_req_num; iter++) {

	/* We assume that get request table item works.  See the checks at the
	 * start of this program.  Also see the limits of the for loop.
	 */

	ival = get_request_table_item( iter, TYPE_OF_ENTRY, &type_of_entry );

	/*
	 * Mark entries ('vt', for example) that are not parameters as
	 * valid (for now).  Fake a valid value by copying the VNMR
	 * value to the hardware value.
	 */

	if (type_of_entry != INTEGER) {
	    ival = get_request_table_item( iter, VNMR_VALUE, &hwval );
	    ival = store_request_table_item( iter, HW_VALUE, hwval );
	    ival = store_request_table_item( iter, VAL_IS_VALID, TRUE );
	    continue;
	}

	ival = get_request_table_item( iter, MAIN_INDEX, &main_index );
	cur_entry = &cur_table[ main_index ];
	if (cur_entry->shims != 0) {
	    /*  Shim DAC values.  */
	    ival = get_request_table_item( iter, AUX_INDEX, &aux_index );
	    getExpStatusShim(aux_index, &hwval);
	}
	else {
	    ival = name_to_stat_val( cur_entry->name, &hwval );
	    if (ival != COMPLETE) {
		strcpy(errmsg,"Programming error in process shim/lock block");
		strcat(errmsg,", failed to find %s");
		Werrprintf(errmsg, cur_entry->name);
		return( ERROR );
	    }
	}
	ival = store_request_table_item( iter, HW_VALUE, hwval );
	ival = store_request_table_item( iter, VAL_IS_VALID, TRUE );
    }

    return( COMPLETE );

}

static int verify_readhw(int argc, char *argv[] )
{
	int	aux_index, iter, ival, main_index;


	init_req_table();
        ival = 0;

/*  See comments in make_transparent_command about hw_table_lookup.  */

	for (iter = 1; iter < argc; iter++) {
		ival = hw_table_lookup( 0, argv[ iter ], &main_index, &aux_index );
		if (ival < 0) {
			Werrprintf(
		    "Read hardware: %s parameter not available", argv[ iter ]
			);
			return( ERROR );
		}

		ival = store_readhw_request( main_index, aux_index);
		if (ival != 0) {
			Werrprintf(
	    "Read hardware:  no more than %d parameters per command", MAX_REQUESTS
			);
			return( -1 );
		}
	}

	if (ival != COMPLETE)
	  return( -1 );

	return( 0 );
}

/*  READHW uses this subroutine to send hardware values to Magical,
    using the return argument vector.					*/

static int return_hw_values(int retc, char *retv[] )
{
	const char	*param_name;
	char	 readhw_report[ 120 ], tmpbuf[ 20 ];
	int	 iter, ival, pval, rspace, tlen;

/*  `retc' should not be less than 0, but let's be sure.  */

	if (retc < 0)
	  retc = 0;

	if (retc > cur_req_num) {
		Werrprintf( "logic error in return hardware values" );
		return( -1 );
	}

/*  As in process_ia_stat, we assume a call to get request table item works.	*/

	if (retc > 0) {
		for (iter = 0; iter < retc; iter++) {
			ival = get_request_table_item( iter, HW_VALUE, &pval );
			retv[ iter ] = realString( (double) pval );
		}
	}

	if (cur_req_num > retc) {
		strcpy( &readhw_report[ 0 ], "Read hardware:  " );
		rspace = 120 - strlen( &readhw_report[ 0 ] );
		for (iter = retc; iter < cur_req_num; iter++) {
			ival = get_request_table_item( iter, HW_VALUE, &pval );
			param_name = request_index_to_param_name( iter );
			if (param_name == NULL) {
				return( -1 );
			}

			sprintf( &tmpbuf[ 0 ], "%s = %d", param_name, pval );
			tlen = strlen( &tmpbuf[ 0 ] ) + 2;
			if (tlen >= rspace) {
				Werrprintf(
		    "Read hardware:  internal buffer overflow"
				);
				return( -1 );
			}

			if (iter > retc)
			  strcat( &readhw_report[ 0 ], ", " );
			strcat( &readhw_report[ 0 ], &tmpbuf[ 0 ] );
		}
		Winfoprintf( &readhw_report[ 0 ] );
	}

	return( 0 );
}

/*  Report those parameters which did not accept the value assigned to them.  */ 

#ifdef XXX
/* See comment where this function is called */
static void report_balky_params()
{
	const char *param_name;
	char	 error_report[ 120 ];
	char	*epilogue = " are not at specified value";
	int	 cur_index, iter, number_balky, rspace, tlen;
	int	 index_balky[ MAX_REQUESTS ];

	number_balky = 0;
	for (iter = 0; iter < cur_req_num; iter++)
	  if (cur_req_table[ iter ].val_is_valid == TRUE) {
		if (cur_req_table[ iter ].vnmr_value !=
		    cur_req_table[ iter ].hw_value)
		{
			index_balky[ number_balky ] = iter;
			number_balky++;
		}
	  }

	if (number_balky < 1)
	  return;

/*  Distinction between 1 and more than 1 parameter just promotes
    good English grammar.  The epilogue string is plural. 		*/

	cur_index = index_balky[ 0 ];
	if (number_balky == 1) {
		param_name = request_index_to_param_name( cur_index );
		if (param_name == NULL)
		  return;
		sprintf( &error_report[ 0 ],
	    "Set hardware: %s is not at specified value", param_name
		);
	}
	else {
		param_name = request_index_to_param_name( cur_index );
		if (param_name == NULL)
		  return;
		sprintf( &error_report[ 0 ], "Set hardware: %s", param_name );
		rspace = 119 - strlen( &error_report[ 0 ] ) - strlen( epilogue );

		for (iter = 1; iter < number_balky; iter++) {
			cur_index = index_balky[ iter ];
			param_name = request_index_to_param_name( cur_index );
			if (param_name == NULL)
			  return;
			tlen = strlen( param_name ) + 2;
			if (tlen > rspace)
			  break;
			else {
				strcat( &error_report[ 0 ], ", " );
				strcat( &error_report[ 0 ], param_name );
			}
		}

	/*  First value for rspace included room for the epilogue.  */

		strcat( &error_report[ 0 ], epilogue );
	}

	Werrprintf( &error_report[ 0 ] );
}
#endif

static int count_sethw_complete()
{
	int	iter, ival;

	ival = 0;
	for (iter = 0; iter < cur_req_num; iter++)
	  if (entry_upto_date( iter ))
	    ival++;

	return( ival );
}

static int store_hw_values()
{
	const char	*param_name;
	int	 cur_tree, iter, ival, pval, type_of_entry;
	double	 dval = 0.0;

	for (iter = 0; iter < cur_req_num; iter++) {
		ival = get_request_table_item( iter, TYPE_OF_ENTRY, &type_of_entry );
		if (ival != COMPLETE) {
			Werrprintf( "Logic error storing values in sethw" );
			return( -1 );
		}
		param_name = request_index_to_param_name( iter );
		if (param_name == NULL) {
			return( -1 );
		}

		if ( !entry_upto_date( iter ) )
		  continue;
		if (cur_req_table[ iter ].new_val_stored == TRUE) 
		  continue;

	/*  recall the tree this parameter is assigned to.
	    if no tree ('vt', for example), skip to next.	*/

		ival = get_request_table_item( iter, CUR_TREE, &cur_tree );
		if (ival != COMPLETE) {
			Werrprintf(
	    "Set hardware: Unknown parameter tree for %s parameter", param_name
			);
			return( -1 );
		}
		if (cur_tree == NOT_FOUND)
		  continue;

		if (type_of_entry == REAL_NUMBER) {
			ival = get_request_table_realnumber( iter, &dval );
		}
		else {
			ival = get_request_table_item( iter, HW_VALUE, &pval );
			dval = (double) pval;
		}

		ival = P_setreal( cur_tree, param_name, dval, 1 );
		if (ival != 0) {
			Werrprintf(
	    "Set hardware:  cannot store value for %s", param_name
			);
			return( -1 );
		}

		store_request_table_item( iter, NEW_VAL_STORED, TRUE );
	}

	return( 0 );
}

int readhw(int argc, char *argv[], int retc, char *retv[] )
{
        int errval,errcount;
        errcount = 0;
/*  Enforce the following restriction:

	readhw('z1','z2','z3'):r1,r2            "is allowed"
	readhw('z1','z2'):r1,r2,r3              "is not allowed"	*/

	if (argc < 2) {
		Werrprintf( "Read hardware command requires arguments" );
		return( -1 );
	}
	if (retc > argc-1) {
           if (strcmp(argv[1],"temp") && strcmp(argv[1],"loc") && strcmp(argv[1],"spin") ) /* temp, loc, and spin are special cases */
           {
		Werrprintf(
	    "Read hardware:  cannot return more values than arguments entered"
		);
		ABORT;
           }
	}
	if ( (errval = verify_hwcmd( argv,
                      ((strcmp(argv[1],"status") == 0) && retc) )) != 0)
        {
           if ( (strcmp(argv[1],"status") == 0) && retc)
           {
              /* verify_hwcmd considers Tuning an error.  For determining
                 status, it is not an error */
              if (errval != -8)
              {
                 retv[ 0 ] = realString((double) errval);
                 RETURN;
              }
           }
           else
           {
	      ABORT;
           }
        }
        if (strcmp(argv[1],"status") == 0)
        {
           int acqstate = 0;
           if (GETACQSTATUS(HostName,UserName) >= 0)
           {
              getAcqStatusInt(STATE, &acqstate);
           }
           if ( (acqstate == ACQ_HOSTGAIN) || (acqstate == ACQ_HOSTSHIM) ||
                (acqstate == ACQ_FINDZ0) || (acqstate == ACQ_PROBETUNE) )
              acqstate = ACQ_ACQUIRE;
           if (retc > 0)
              retv[ 0 ] = realString((double) acqstate);
           else
              Winfoprintf("Acquisition status is %d",acqstate);
	   RETURN;
        }
        if (strcasecmp(argv[1],"remainingtime") == 0)
        {
           int secs = 0;
           if (GETACQSTATUS(HostName,UserName) >= 0)
           {
              secs = getAcqRemainingTime();
           }
           if (retc > 0)
              retv[ 0 ] = realString((double) secs);
           else
              Winfoprintf("Remaining time is %d seconds",secs);
	   RETURN;
        }
        if (strcasecmp(argv[1],"inqueue") == 0)
        {
           int inque = 0;
           if (GETACQSTATUS(HostName,UserName) >= 0)
           {
              inque = getAcqInQue();
           }
           if (retc > 0)
              retv[ 0 ] = realString((double) inque);
           else
              Winfoprintf("Items in queue is %d",inque);
	   RETURN;
        }
        if (strcmp(argv[1],"temp") == 0)
        {
           int temp = 0;
           int tempset = 0;
           int stat = 0;

           if (GETACQSTATUS(HostName,UserName) >= 0)
           {
              temp = getAcqTemp();
              stat = getAcqLSDV();
              stat = (stat >> 11) & 0x0003;
           }
           if (retc > 0)
           {
              retv[ 0 ] = realString((double) temp / 10.0);
              if (retc > 1)
              {
                 retv[ 1 ] = realString((double) stat);
                 if (retc > 2)
                 {
                    tempset = getAcqTempSet();
                    retv[ 2 ] = realString((double) tempset / 10.0);
                 }
              }
           }
           else
           {
              if (stat == 3)
              {
                 Winfoprintf("Cannot read temperature. No controller.");
              }
              else
              {
                 char *ptr;
                 if (stat == 0)
                    ptr = "Regulation off";
                 else if (stat == 1)
                    ptr = "Regulated";
                 else
                    ptr = "Not regulated";
                 Winfoprintf("Temperature is %6.2f ( %s )",
                     (double) temp / 10.0, ptr);
              }
           }
	   RETURN;
        }
        if (strcmp(argv[1],"spin") == 0)
        {
           int spin = 0;
           int spinset = 0;
           int stat = 0;

           if (GETACQSTATUS(HostName,UserName) >= 0)
           {
              spin = getAcqSpin();
              stat = getAcqLSDV();
              stat = (stat >> 4) & 0x0003;
           }
           if (retc > 0)
           {
              retv[ 0 ] = intString( spin );
              if (retc > 1)
              {
                 retv[ 1 ] = intString(stat);
                 if (retc > 2)
                 {
                    spinset = getAcqSpinSet();
                    retv[ 2 ] = intString( spinset );
                 }
              }
           }
           else
           {
              if (stat == 3)
              {
                 Winfoprintf("Cannot read spinner. No controller.");
              }
              else
              {
                 char *ptr;
                 if (stat == 0)
                    ptr = "Regulation off";
                 else if (stat == 1)
                    ptr = "Regulated";
                 else
                    ptr = "Not regulated";
                 Winfoprintf("spin speed is %d ( %s )", spin, ptr);
              }
           }
	   RETURN;
        }
        if (strcmp(argv[1],"loc") == 0)
        {
           int loc = 0;
           if (GETACQSTATUS(HostName,UserName) >= 0)
           {
              loc = getAcqSample();
           }
           if (retc > 0)
           {
              retv[ 0 ] = intString(loc);
              if (retc > 1)
              {
                 int stat;
                 stat = getAcqLSDV();
                 stat = ( (stat & 0x4000) == 0x4000) ? 1 : 0;
                 retv[ 1 ] = intString(stat);
              }
           }
           else
              Winfoprintf("Sample location is %d", loc);
	   RETURN;
        }

        if (strcmp(argv[1],"cntlrsyncfail") == 0)
        {
           int syncfail = 0;
           if (GETACQSTATUS(HostName,UserName) >= 0)
           {
              syncfail = getAcqTickCountError();
           }
           if (retc > 0)
           {
              retv[ 0 ] = intString(syncfail);
           }
           else
              Winfoprintf("Controller timing sync error is %d", syncfail);
           RETURN;
        }

	if (verify_readhw( argc, argv ) != 0)
	  ABORT;
	
	while (get_ia_stat( HostName, UserName ) != 0)
        {
           /* Try once more */
           sleepSeconds(1);
           if (errcount++ > 3) ABORT;
        }
	if (verify_data_valid( "Read hardware" ) != COMPLETE)
	  ABORT;
	if (process_ia_stat( ) != COMPLETE)
	  ABORT;
	if (return_hw_values( retc, retv ) != 0)
	  ABORT;
	RETURN;
}

/* sethw reworked to RETURN even if an error occurs, provided there is
   a return argument to receive the error status.  sethw only ABORTS if
   there is an error and there are no return arguments (plus in 2 very
   special cases where get_ia_stat fails, which should never happen).	*/

int sethw(int argc, char *argv[], int retc, char *retv[] )
{
	char		message[ 256 ];
	int		number_set;
        int             stat = SUCCEEDED;
        int cmd;
        int errcount;
	errcount = 0;
	want_to_sleep = FALSE;
        /* retc == -1 is special case from sethwshim() to suppress messages but not fill in retv */
        cmd = ACQHARDWARE;

	if (is_datastation())
        {
	    if (retc == 0)
            {
		Werrprintf( "%s:  available on spectrometer only", argv[ 0 ] );
	    }
	    stat = FAILED;
	}

       
	if (stat != FAILED)
	  if (verify_sethw( argc, argv, &message[ 0 ], sizeof( message ),
                            (retc != 0) ) != 0)
	    stat = FAILED;

	if (stat != FAILED)
	  if (is_acqproc_active() == 0)
	  {
		if (retc == 0) {
			disp_acq("");
			Werrprintf("The acquisition system is not active.");
		}
		stat = FAILED;
	  }

	if (stat != FAILED)
        {
          cmd = find_arg(argc,argv,"robot") ? ROBOT : ACQHARDWARE;
	  if (talk_to_acqproc( cmd,
                               &message[ 0 ], &message[ 0 ], sizeof(message)) )
	    stat = FAILED;
        }

	if (stat != FAILED)
	  if (make_sethw_report( &message[ 0 ], (retc != 0) ) != 0)
	    stat = FAILED;

/*  abort now, if a problem has been detected  */

	if (stat == FAILED) {
        	if (retc > 0) {
			retv[ 0 ] = realString( (double) FAILED);
			RETURN;
		}
		else
		  ABORT;
	}

/*  At this time, the VNMR program only checks the new values once.
    The set hardware software is designed to accomodate more than
    one such check.  An important unresolved issue is the value of
    `number_set':  is it the cumulative number set or the number
    set in this iteration?  Currently it is the cumulative number.	*/


	if ((cmd == ROBOT) && sethw_wait)
        {
           char path[MAXPATH];
           char msg[MAXPATH];
           int max = 300; /* wait up to 30 seconds */
           int res;

           sprintf(path, "%s/sethw", curexpdir);
           while ( access(path,R_OK) && (max > 0) )
           {
              max--;
              sleepMilliSeconds(100);
           }
           if ( ! access(path,R_OK) )
           {
               FILE *fd;

               strcpy(msg,"");
               fd = fopen(path,"r");
               fscanf(fd, "%d %[^\n]\n", &res, msg);
               fclose(fd);
               unlink(path);
               if (msg[strlen(msg)-1] == '\r')
                  msg[strlen(msg)-1] = '\0'; 
           }
           else
           {
              res = -1;
              strcpy(msg,"No result file found");
           }
           
           if (retc > 0)
           {
              retv[ 0 ] = intString(res);
              if (retc > 1)
                 retv[1] = newString(msg);
           }
           else
              Winfoprintf("sethw: %s",msg);
           RETURN;
        }
	else if (sethw_wait)
        {
                int valid_data;

		if (want_to_sleep) {
        		if (retc == 0)
			  Winfoprintf( "This operation takes a moment" );
			sleepSeconds( 7 );
		}

	       while (get_ia_stat( HostName, UserName ) != 0)
               {
                 /* Try once more */
                 sleepSeconds(1);
                 if (errcount++ > 3) ABORT;
	       }

                getExpStatusInt(EXP_VALID, &valid_data);
		if (want_to_sleep || ( (valid_data != INIT_STRUC) &&
	               (valid_data != LK_STRUC) &&
	               (valid_data != EXP_STRUC) ) )
                {
                    /* try once more */
                    sleepSeconds(5);
		    if (get_ia_stat( HostName, UserName ) != 0)
		      ABORT;
                }
		if (verify_data_valid( "Verify set hardware" ) != COMPLETE)
                {
                   stat = FAILED;
                }
                else if (number_current_requests() > 0)
		{
		   if (process_ia_stat( ) != COMPLETE)
                   {
			  stat = FAILED;
                   }
                   else
                   {
			number_set = count_sethw_complete();
			if ((number_set > 0) && (store_hw_values() != 0) )
			   stat = FAILED;
#ifdef XXX
                        /* This can randomly fail. Probably a race condition between a message
                         * sent to the lock controller board to update the stat block and the
                         * stat block being sent to the host.
                         */
			else if (number_set != cur_req_num)
                        {
                          /*  Probably should not report balky parameters each iteration
                              if you check the values in the hardware more than once.	*/
			  if (retc != 0)
			    report_balky_params();
                          stat = FAILED;
                        }
#endif
                   }
		}
	}
	else
        {
		number_set = cur_req_num;
                stat = STARTED;
        	if (retc == 0)
		  Winfoprintf( "Set hardware:  operation started" );
	}

        if (stat != FAILED)
        {
           if ((stat == SUCCEEDED) && (retc == 0))
		Winfoprintf( "Set hardware: operation complete" );
           if (stat == STARTED)
              stat = SUCCEEDED;
        }
        if (retc > 0)
           retv[ 0 ] = realString( (double) stat);

/*  Returns normal status, even if none of the values in
    the hardware agree with values provided by the user.	*/

	RETURN;
}

static int getShimPair(int index)
{
   if (index == 16)
      return(18);
   if (index == 18)
      return(16);

   if (index == 17)
      return(21);
   if (index == 21)
      return(17);

   if (index == 20)
      return(25);
   if (index == 25)
      return(20);

   if (index == 19)
      return(27);
   if (index == 27)
      return(19);

   if (index == 12)
      return(22);
   if (index == 22)
      return(12);

   if (index == 13)
      return(23);
   if (index == 23)
      return(13);

   return(0);
}

int sethwshim(int argc, char *argv[], int retc, char *retv[])
{
   int shimVal;
   char *argv2[5];
   char *retv2[2];
   char newarg[128];

   if (argc != 3)
   {
      Werrprintf("usage - %s('shimname',shimvalue)", argv[0]);
      ABORT;
   }
   argv2[0] = "sethw";
   argv2[1] = "nowait";
   argv2[2] = argv[1];
   argv2[3] = argv[2];
   argv2[4] = NULL;
   retv2[0] = NULL;
   retv2[1] = NULL;
   shimVal = atoi( argv[ 2 ] );
   init_shimnames(SYSTEMGLOBAL);
   if ( isThinShim() )
   {
      int shimIndex;
      int shimPairIndex;

      shimIndex = get_shimindex(argv[1]);
      if (shimIndex < 0)
      {
         Werrprintf("%s: Unknown shim %s", argv[0], argv[1]);
         ABORT;
      }
      if ( (shimPairIndex = getShimPair(shimIndex) ))
      {
         int shimVal2;
         int token1;
         int token2;
         int rangeChecked = 0;

         getExpStatusShim(shimPairIndex, &shimVal2);
         token1 = shimVal + shimVal2;
         token2 = shimVal - shimVal2;
         if (token1 > 32767)
         {
            shimVal = 32767 - shimVal2;
            rangeChecked = 1;
         }
         else if (token1 < -32767)
         {
            shimVal = -32767 - shimVal2;
            rangeChecked = 1;
         }
         else if (token2 > 32767)
         {
            shimVal = 32767 + shimVal2;
            rangeChecked = 1;
         }
         else if (token2 < -32767)
         {
            shimVal = -32767 + shimVal2;
            rangeChecked = 1;
         }
         if ( rangeChecked )
         {
           if ( (shimVal != 32767) && (shimVal != -32767) )
           {
              getExpStatusShim(shimIndex, &token1);
              if (token1 == shimVal)
              {
                 if (shimVal2 >= 0)
                    shimVal--;
                 else
                    shimVal++;
              }
           }
         }
         sprintf(newarg,"%d",shimVal);
         argv2[3] = newarg;
      }
   }
   else if (get_shimindex(argv[1]) < 0)
   {
      Werrprintf("%s: Unknown shim %s", argv[0], argv[1]);
      ABORT;
   }
   if (retc)
      retv[0] = intString(shimVal);
   return( sethw(4,argv2,-1,retv2) );
}

static char *parse_acqproc_queue(char *queue_addr, char *entry_addr,
                                 int entry_len )
{
	int	 cur_len;
	char	*next_entry;

	if (queue_addr == NULL)			/* This should not happen */
	  return( NULL );

	if (*queue_addr == '\0')		/* Then there are no more entries */
	  return( NULL );

	next_entry = strchr( queue_addr, '\n' );
	if (next_entry == NULL)
	  cur_len = strlen( queue_addr );
	else
	  cur_len = next_entry - queue_addr;

	if (cur_len >= entry_len) {
		strncpy( entry_addr, queue_addr, entry_len-1 );
		entry_addr[ entry_len-1 ] = '\0';
	}
	else {
		strncpy( entry_addr, queue_addr, cur_len );
		entry_addr[ cur_len ] = '\0';
	}

/*  next entry will be non-NULL if-and-only-if another entry is present.
    If so, add one to next entry to skip the new-line character.
    If not, add the length of the current entry so next entry
    becomes the address of the terminating NUL character.

    Return address of the next entry.					*/

	if (next_entry != NULL)
	  next_entry++;
	else
	  next_entry = queue_addr + cur_len;

	return( next_entry );
}

int talk_to_acqproc(int cmd, char *msg1, char *msg2, int msg2len)
{
   return( talk_2_acqproc(HostName,UserName,cmd,msg1,msg2,msg2len) );
}

/*  "active" here means an acquisition is active or queued  */
/*  If this_expnum = 0, then just check user name           */
int is_exp_active( int this_expnum )
{
	int	 entry_num, ival, queue_expnum, goFlag;
	char	*qptr;
	char	 queue_report[ 256 ], cur_entry[ 82 ], queue_user[ 82 ];

	if (is_acqproc_active() == 0)
	  return( -1 );

	ival = talk_to_acqproc( QUEQUERY, "", &queue_report[ 0 ], sizeof( queue_report ) );
	if (ival != 0)
	  return( -1 );		/* get acqproc queue calls talk to acqproc
				   which makes its own error reports.		*/

	entry_num = 0;
	qptr = &queue_report[ 0 ];
	while (
	  (qptr = parse_acqproc_queue( qptr, &cur_entry[ 0 ], sizeof( cur_entry ) )) != NULL
	      ) {
		entry_num++;
		sscanf( &cur_entry[0], "%d %s %d", &queue_expnum, &queue_user[0], &goFlag );
		if ( this_expnum && (this_expnum != queue_expnum) )
		  continue;
		if (strcmp( UserName, &queue_user[ 0 ] ) != 0)
		  continue;
		return( entry_num ); /* entry_num == 1 is active experiment */
                                     /* entry_num >= 2 is queued */
	}

	return( 0 );			/* If it is never found, it is not active */
}

/*  "acquiring" here means an acquisition is active or queued but not in wexp processing.
 *  An acquisition is considered "active" if it is finished acquiring but wexp processing
 *  is occuring and the acquisition was started with a "wait" flag.  Thus, is_exp_acquiring
 *  yields the same results, except in the case of wexp processing on an acquisition which
 *  has the "wait" flag on.  is_exp_active will return a TRUE in this case. is_exp_acquiring
 *  will return a FALSE.  is_exp_acquiring is used by rt to allow rt in the wexp macro.
 *  is_exp_active is used by go to determine if another expirement is active.
 */
int is_exp_acquiring(int this_expnum )
{
	int	 entry_num, ival, queue_expnum, goFlag;
	char	*qptr;
	char	 queue_report[ 256 ], cur_entry[ 82 ], queue_user[ 82 ];

	if (is_acqproc_active() == 0)
	  return( -1 );

	ival = talk_to_acqproc( QUEQUERY, "", &queue_report[ 0 ], sizeof( queue_report ) );
	if (ival != 0)
	  return( -1 );		/* get acqproc queue calls talk to acqproc
				   which makes its own error reports.		*/

	entry_num = 0;
	qptr = &queue_report[ 0 ];
	while (
	  (qptr = parse_acqproc_queue( qptr, &cur_entry[ 0 ], sizeof( cur_entry ) )) != NULL
	      ) {
		entry_num++;
		sscanf( &cur_entry[0], "%d %s %d", &queue_expnum, &queue_user[0], &goFlag);
		if (this_expnum != queue_expnum)
		  continue;
		if (goFlag == -1)
		  continue;
		if (strcmp( UserName, &queue_user[ 0 ] ) != 0)
		  continue;
		return( entry_num ); /* entry_num == 1 is active experiment */
                                     /* entry_num >= 2 is queued */
	}

	return( 0 );			/* If it is never found, it is not active */
}

/*  Searches the experiment queue for an acquisition that collects
    data, as judged by the value of the setup flag (queue_expflag)
    stored for each acquisition.  Requires new version of PSG and
    Expproc from spring 1998.						*/

int is_data_present(int this_expnum )
{
	int	 entry_num, ival, queue_expnum, queue_expflag;
	char	*qptr;
	char	 queue_report[ 256 ], cur_entry[ 82 ], queue_user[ 82 ];

	if (is_acqproc_active() == 0)
	  return( -1 );

	ival = talk_to_acqproc( QUEQUERY, "", &queue_report[ 0 ], sizeof( queue_report ) );
	if (ival != 0)
	  return( -1 );		/* get acqproc queue calls talk to acqproc
				   which makes its own error reports.		*/

	entry_num = 0;
	qptr = &queue_report[ 0 ];
	while (
	  (qptr = parse_acqproc_queue( qptr, &cur_entry[ 0 ], sizeof( cur_entry ) )) != NULL
	      ) {
		entry_num++;
		ival = sscanf( &cur_entry[ 0 ], "%d %s %d",
			       &queue_expnum, &queue_user[ 0 ], &queue_expflag );
	    
		if (ival < 3)			/* if no queue_expflag */
		  continue;			/* no point in proceeding */
		if (strcmp( UserName, &queue_user[ 0 ] ) != 0)
		  continue;
		if (this_expnum != queue_expnum)
		  continue;
		if (queue_expflag != EXEC_GO)
		  continue;
		return( entry_num ); /* entry_num == 1 is active experiment */
                                     /* entry_num >= 2 is queued */
	}

	return( 0 );			    /* If it is never found, */
}			/* no data to be acquired in this experiment */

/*  Please note I have put in a temporary work-a-round so that
    either OK or OK2 as a reply imply the console is available. 
    This won't work for ACQI or Qtune...			*/

int ok_to_acquire()
{
	int	ival;
	char	quick_msg[ 20 ];

	if (is_acqproc_active() == 0)
	  return( -2 );

	ival = talk_to_acqproc(
	    ACCESSQUERY, "acq\n", &quick_msg[ 0 ], sizeof( quick_msg )
	);

	if (strcmp( "OK", &quick_msg[ 0 ] ) == 0 ||
	    strcmp( "OK2", &quick_msg[ 0 ] ) == 0)
	  return( 0 );
	else if (strcmp( "auto", &quick_msg[ 0 ] ) == 0)
	  return( -4 );
	else if (strcmp( "INTERACTIVE", &quick_msg[ 0 ] ) == 0)
	  return( -5 );
	else if (strcmp( "BUSY", &quick_msg[ 0 ] ) == 0)
	  return( -6 );
	else if (strcmp( "DOWN", &quick_msg[ 0 ] ) == 0)
	  return( -7 );
	else if (strcmp( "TUNING", &quick_msg[ 0 ] ) == 0)
	  return( -8 );
	else
	  return( -3 );
}

void report_acq_perm_error(char *cmdname, int errorval )
{
	if (errorval == -2)
	  Werrprintf( "%s cannot proceed, acquisition not active", cmdname );
	else if (errorval == -3)
	  Werrprintf(
    "%s cannot proceed, another user's experiment is already active", cmdname
	  );
	else if (errorval == -4)
	  Werrprintf( "%s cannot proceed, system in automation mode", cmdname);
	else if (errorval == -5)
	  Werrprintf( "%s cannot proceed, system in interactive mode", cmdname);
	else if (errorval == -6)
	  Werrprintf( "%s cannot proceed now, abort of ACQI not complete", cmdname);
	else if (errorval == -7)
	  Werrprintf( "%s cannot proceed, console powered down or not connected", cmdname);
	else if (errorval == -8)
	  Werrprintf( "%s cannot proceed, console in tune mode", cmdname);
	else
	  Werrprintf( "%s cannot proceed, cannot communicate with acquisition", cmdname );
}

/* access to the console is required to start an acquisition.
   VNMR obtains this access.  See go.c, which calls ok_to_acquire,
   and ok_to_acquire, above.  Under normal circumstances, Expproc
   releases access to the console when the acquisition completes.
   Under various circumstances though VNMR needs to release
   access.  Thus this program.						*/

/* Same as release_console() but assumes is_acqproc_active is already known */
int release_console2()
{
	int	ival;
	char	quick_msg[ 20 ];

	ival = talk_to_acqproc(
	    RELEASECONSOLE, "acq\n", &quick_msg[ 0 ], sizeof( quick_msg )
	);

	if (ival != 0)
	  return( -1 );
	else
	  return( 0 );
}

int release_console()
{
	int	ival;
	char	quick_msg[ 20 ];

	if (is_acqproc_active() == 0)
	  return( -2 );

	ival = talk_to_acqproc(
	    RELEASECONSOLE, "acq\n", &quick_msg[ 0 ], sizeof( quick_msg )
	);

	if (ival != 0)
	  return( -1 );
	else
	  return( 0 );
}

/* If PSG aborts, it tells Vnmr this happened.  Vnmr now needs
   to release access to the console.  This command does that.	*/

int releaseConsole(int argc, char *argv[], int retc, char *retv[] )
{
   (void) argc;
   (void) argv;
   (void) retc;
   (void) retv;
   release_console();
   RETURN;
}


/******************************************************************************/

/*  These program are to be independent of the sethw programs.  They are
    put here since they also exchange data and commands with the acquisition
    console.								      */

/******************************************************************************/

static int
wsram_with_args(int argc, char *argv[] )
{
	int	iter, nval;
	char	message[ 1024 ];

/*  I made message larger than in sethw/readhw so this command will not be
    limited by the size of message-to-Acqproc.  Unlike the other commands,
    this one is not designed to be used by the user, rather by the config
    macro.  It should be able to set as much SRAM as it can in one fell
    swoop.

    There is a limit of 254 numbers, imposed by the do-changes system.	*/

	if (argc < 2) {
		Werrprintf( "%s program requires arguments", argv[ 0 ] );
		return( -1 );
	}

/*  Arguments are expected to-be pairs; with the name-of-the-command
    included, the count has to be an odd number.			*/

	else if (argc % 2 != 1) {
		Werrprintf( "%s requires offset/value pairs", argv[ 0 ] );
		return( -1 );
	}

	for (iter = 1; iter < argc; iter++)
	  if (isReal( argv[ iter ] ) == 0) {
		Werrprintf(
	   "%s can't use %s as an argument, use only numbers", argv[ 0 ], argv[ iter ]
		);
		return( -1 );
	  }

	init_transparent_command();
	insert_acode_in_transparent( WSRAM );		/* Write SRAM command */
	insert_acode_in_transparent( (argc-1)/2 );	/* Number of pairs */

	for (iter = 1; iter < argc; iter++) {
		nval = atoi( argv[ iter ] );
		if (insert_acode_in_transparent( nval ) != 0) {
			Werrprintf( "%s:  too many parameters", argv[ 0 ] );
			return( -1 );
		}
	}

	if (transparent_to_ascii( &message[ 0 ], sizeof( message ) ) != 0) {
		Werrprintf( "%s: internal buffer is too short", argv[ 0 ] );
		return( -1 );
	}

/*	Wscrprintf( "message for acquisition: %s\n", &message[ 0 ] );  */
	if (talk_to_acqproc(
		ACQHARDWARE, &message[ 0 ], &message[ 0 ], sizeof( message )
	) != 0)
	  return( -1 );

	return( 0 );
}

static int
wsram_with_file(int argc, char *argv[] )
{
	char	 word_buf[ 128 ], message[ 1024 ];
	int	 nval, word_cnt;
	FILE	*wsram_fptr;

	if (argc < 2) {
		Werrprintf( "%s program requires arguments", argv[ 0 ] );
		return( -1 );
	}

/*  Open the file with SRAM data  */

	wsram_fptr = fopen( argv[ 1 ], "r" );
	if (wsram_fptr == NULL) {
		Werrprintf( "%s:  cannot access %s", argv[ 0 ], argv[ 1 ] );
		return( -1 );
	};

/*  Count the number of words in this file  */

	word_cnt = 0;
	while (getword(wsram_fptr, &word_buf[ 0 ], sizeof( word_buf ), &word_buf[ 0 ], 0))
	  word_cnt++;

/*	Wscrprintf( "wsram with file found %d words\n", word_cnt );  */
	if ((word_cnt % 2) != 0 || word_cnt == 0) {
		if (word_cnt == 0)
		  Werrprintf(
	   "%s:  the file %s is empty", argv[ 0 ], argv[ 1 ]
		  );
		else
		  Werrprintf(
	   "%s:  the file %s is not composed of offset/value pairs", argv[ 0 ], argv[ 1 ]
		  );
		fclose( wsram_fptr );
		return( -1 );
	}

/*  Following line serves to position the file pointer at the start of the file  */

	rewind( wsram_fptr );

	init_transparent_command();
	insert_acode_in_transparent( WSRAM );		/* Write SRAM command */
	insert_acode_in_transparent( word_cnt/2 );	/* Number of pairs */
	while (getword(wsram_fptr, &word_buf[ 0 ], sizeof( word_buf ), &word_buf[ 0 ], 0)) {
		if ( !isReal( &word_buf[ 0 ] ) ) {
			Werrprintf(
	   "%s:  found %s in %s; not an integer", argv[ 0 ], &word_buf[ 0 ], argv[ 1 ]
			);
			fclose( wsram_fptr );
			return( -1 );
		}
		nval = atoi( &word_buf[ 0 ] );
		if (insert_acode_in_transparent( nval ) != 0) {
			Werrprintf( "%s:  too many parameters", argv[ 0 ] );
			fclose( wsram_fptr );
			return( -1 );
		}
	}
	fclose( wsram_fptr );

	if (transparent_to_ascii( &message[ 0 ], sizeof( message ) ) != 0) {
		Werrprintf( "%s: internal buffer is too short", argv[ 0 ] );
		return( -1 );
	}

/*	Wscrprintf( "message for acquisition: %s\n", &message[ 0 ] );  */
	if (talk_to_acqproc(
		ACQHARDWARE, &message[ 0 ], &message[ 0 ], sizeof( message )
	) != 0)
	  return( -1 );

	return( 0 );
}

int wsram(int argc, char *argv[], int retc, char *retv[] )
{
   int	ival = 1;

   if (is_acqproc_active() )
   {
      if (argc < 2)
      {
	  Werrprintf( "%s program requires arguments", argv[ 0 ] );
      }
      else
      {

/*  Note:  wsram( "123file" ) gets interpreted by wsram_with_args,
           NOT the desired wsram_with_file.  Don't use a file name
           with a digit as its first character.				*/
         if (isReal( argv[ 1 ] ))
           ival = wsram_with_args( argc, argv );
         else
           ival = wsram_with_file( argc, argv );
      }
   }
   if (retc > 0)
      retv[ 0 ] = realString( (ival == 0) ? (double) 1 : (double) 0 );
   RETURN;
}

/*  Method for other VNMR commands/programs to use.  Not a VNMR command  */
/* ACQ_SUN.h defines c68int */
int send_transparent_data(c68int *bufaddr, int bufsize )
{
	char	message[ 256 ];
	int	iter;
	
	init_transparent_command();
	for (iter = 0; iter < bufsize; iter++)
	  insert_acode_in_transparent( bufaddr[ iter ] );

	if (transparent_to_ascii( &message[ 0 ], sizeof( message ) ) != 0) {
		Werrprintf( "internal buffer is too short in send transparent data" );
		return( -1 );
	}

	if (talk_to_acqproc(
		ACQHARDWARE, &message[ 0 ], &message[ 0 ], sizeof( message )
	) != 0)
	  return( -1 );

	return( 0 );
}

int expactive(int argc, char *argv[], int retc, char *retv[])
{
   int this_expnum = 0;
   int auto_check = 0;
   int current_check = 0;
   int user_check = 0;
   int res;

   if (argc == 1)
      this_expnum = expdir_to_expnum( curexpdir );
   else if ( (argc == 2) && isReal( argv[ 1 ] ) )
      this_expnum = atoi( argv[ 1 ] );
   else if ( (argc == 2) && (strcmp(argv[1],"auto") == 0) )
      auto_check = 1;
   else if ( (argc == 2) && (strcmp(argv[1],"current") == 0) )
      current_check = 1;
   else if ( (argc == 2) && (strcmp(argv[1],"user") == 0) )
      user_check = 1;
   else
   {
      Werrprintf("usage - expactive with optional exp. number or 'auto', 'current' or 'user'");
      ABORT;
   }

/*  Auto check used to call ok_to_acquire.  This had a side effect
    of reserving the console.  So a new Acqproc/Expproc command
    QUERYSTATUS was introduced, which works similar to the one used
    in ok_to_acquire (ACCESSQUERY).  expactive now interacts directly
    with Acqproc/Expproc, rather than examining the value returned
    from ok_to_acquire.    April 23, 1997                            */

   if (auto_check)
   {
      int	ival = -1;
      char	quick_msg[ MAXPATH ];

      if (is_acqproc_active())
         ival = talk_to_acqproc(
	  QUERYSTATUS, "", &quick_msg[ 0 ], sizeof( quick_msg ));
      if (ival != 0)
         res = 0;
      else
         res = (strncmp( &quick_msg[ 0 ], "auto", strlen( "auto" ) ) == 0);
      if (retc > 0)
      {
         retv[ 0 ] = realString( (double) res );
         if (retc > 1)
         {
            getAutoDir(quick_msg, MAXPATH);
            retv[ 1 ] = newString(quick_msg);
         }
      }
      else
         Winfoprintf("System is%s in automation mode",(res==1) ? "" : " not");
   }
   else if (current_check)
   {
      int  expNum;
      char queue_report[ 256 ], cur_entry[ 82 ], this_user[ 82 ];

      expNum = -1;
      strcpy(this_user,"nobody");
      if (is_acqproc_active() &&
	  (talk_to_acqproc(QUEQUERY,"",&queue_report[0],sizeof(queue_report)) == 0))
      {
         parse_acqproc_queue(queue_report,cur_entry,sizeof(cur_entry));
         sscanf(cur_entry, "%d %s", &expNum, this_user);
      }
      if (retc > 0)
      {
         retv[ 0 ] = realString( (double) expNum );
         if (retc > 1)
            retv[ 1 ] = newString(this_user);
      }
      else if (expNum <= 0)
         Winfoprintf("No experiment active");
      else
         Winfoprintf("%s has an active acquisition in experiment %d",this_user, expNum);
   }
   else
   {
      res = -1;
      if (is_acqproc_active())
         res = is_exp_active( this_expnum );	/* A value of 0 means nothing */
						/* is active.  A value of -1  */
						/* means an error occured in  */
						/* is_exp_active, in which    */
						/* case we proceed.	      */
      if (retc > 0)
      {
         retv[ 0 ] = realString( (double) res );
      }
      else if (user_check)
      {
         if (res <= 0)
            Winfoprintf("User has no active acquisition");
         else if (res == 1)
            Winfoprintf("User has active Experiment");
         else if (res >= 2)
            Winfoprintf("User has queued Experiment");
      }
      else
      {
         if (res <= 0)
            Winfoprintf("Experiment %d has no active acquisition",this_expnum);
         else if (res == 1)
            Winfoprintf("Acquisition active in Experiment %d",this_expnum);
         else if (res >= 2)
            Winfoprintf("Acquisition queued (%d) in Experiment %d",res-1,this_expnum);
      }
   }
   RETURN;
}

#define MAXRTVARNAMELEN	20
#define FIX_RTVARS	36	/* from ..acqcmds.h */
#define HASH_RTVARMAX 	1000	/* max number of expected rtvars for	*/

int acqupdt(int argc, char *argv[], int retc, char *retv[])
{
   char	message[ 256 ],rtvar_name[MAXPATHL];
   int 	rtindex,rtval;

   (void) retc;
   (void) retv;
   if (!strcmp(argv[0],"initrtvar"))
   {
	int rtvarsize = HASH_RTVARMAX;
	if (argc > 2) 
	{ 
	   Werrprintf("Usage: initrtvar([<rtvar_filename>])");
	   ABORT;
	}
	else if (argc > 1)
	{
	   strcpy(rtvar_name,argv[1]);
	}
	else
	{
	   strcpy(rtvar_name,systemdir);
	   strcat(rtvar_name,"/acqqueue/rtvars.IPA");
	}
	while ((rtvarsize < HASH_RTVARMAX) && 
	   (init_rtvars(rtvar_name,rtvarsize) < 0)) rtvarsize += HASH_RTVARMAX;
	RETURN;
   } 
   if (!strcmp(argv[0],"setrtvar"))
   {
	if (argc != 3) 
	{ 
	   Werrprintf("Usage: setrtvar(<rtvar_name>,<value>)");
	   ABORT;
	}
	if ( (int)strlen(argv[1]) > MAXRTVARNAMELEN )
	{ 
	   Werrprintf("setrtvar: Invalid rtvar name.");
	   ABORT;
	}
   }
   else
   {
	Werrprintf ( "acqupdt: Incorrect command." );
	ABORT;
   }

   /* Check acquisition system is active 	*/
   /*   remove for speed			*/
   /* if (is_acqproc_active() == 0)	*/
   /* {	*/
   /* 	disp_acq("");	*/
   /* 	Werrprintf("The acquisition system is not active.");	*/
   /* 	return( -1 ); 	*/
   /* }	*/

   /* fprintf(stderr,"setrtvar: name: %s value %s\n",argv[1],argv[2]); */
   /* Put together command for acq */
   rtindex = rtn_index(argv[1]);
   rtval = atoi(argv[2]);
   sprintf( &message[ 0 ], "1;%d,%d,1,%d", FIX_RTVARS, rtindex, rtval );

   /* fprintf(stderr,"acqupdt:message:: %s\n",message); */
   /* send message */
   if (talk_to_acqproc(
   	RTAUPDT, &message[ 0 ], &message[ 0 ], sizeof( message )
   ) != 0)
     ABORT;


/*  Returns normal status, even if none of the values in
    the hardware agree with values provided by the user.	*/

   RETURN;
}

void jacqupdtOn(int argc, char *argv[], int retc, char *retv[])
{
  (void) argc;
  (void) argv;
  (void) retc;
  (void) retv;
  AllowUpdatingFlag = 1; 
}

void jacqupdtOff(int argc, char *argv[], int retc, char *retv[])
{
  (void) argc;
  (void) argv;
  (void) retc;
  (void) retv;
  AllowUpdatingFlag = 0; 
}

int allowJpsgUpdate()
{
  return(AllowUpdatingFlag);
}

/* JPSG varient of the above */
int jacqupdt(int argc, char *argv[], int retc, char *retv[])
{
   FILE *sd;
    int port;
    int i,tree;
    char *name;
    char portstr[50];
    char pidstr[50];
    char hostname[50];
    char readymsge[50];
    char ackbuf[256];
    char jpsgPIDFileName[82];
    char *updateCmd = "UpdateParm\n";
    Socket  *tSocket;

    int bytes;
    int result;
    vInfo info;

   (void) retc;
   (void) retv;
   if (!strcmp(argv[0],"updtparam"))
   {
	if (argc < 2) 
	{ 
	   Werrprintf("Usage: updtparam(<param_name>,[tree])");
	   ABORT;
	}
        name = argv[1];
	if ( (int)strlen(name) > MAXRTVARNAMELEN )
	{ 
	   Werrprintf(
	   "updtparam: Parameter Name '%s' exceeds %zd characters (max=%d)",
		name,strlen(name),MAXRTVARNAMELEN);
	   ABORT;
	 }
        if (AllowUpdatingFlag < 1)
        {
           Werrprintf("Not in Update Mode, updtparam('%s') not performed",name);
           ABORT;
        }
   }
   else
   {
	Werrprintf ( "updtparam: Incorrect command." );
	ABORT;
   }

   if (argc == 3)
     tree = getTreeIndex(argv[2]); 
   else
     tree = CURRENT;

   result = P_getVarInfo(tree,name,&info);
   if (result == -1)
   {
     Werrprintf(
      "updtparam: Parameter tree '%s' doesn't exist",argv[2]);
	   ABORT;
   }
   else if (result == -2)
   {
     Werrprintf(
      "updtparam: Parameter '%s' not found in tree '%s'",
	name, getRoot(tree));
	   ABORT;
   }


   /* Check acquisition system is active 	*/
   /*   remove for speed			*/
   /* if (is_acqproc_active() == 0)	*/
   /* {	*/
   /* 	disp_acq("");	*/
   /* 	Werrprintf("The acquisition system is not active.");	*/
   /* 	return( -1 ); 	*/
   /* }	*/

   /* ---------------------------------------------------- */
   /*  
       This should be in the ncomm library, but for I am putting it
       here so I can test this in one place
   */
    if (isJpsgReady())
    {
      sprintf(jpsgPIDFileName, "/vnmr/acqqueue/jinfo1.%s", UserName);
      sd = fopen(jpsgPIDFileName,"r");
      i = fscanf(sd,"%s %s %s %s",portstr,pidstr,hostname,readymsge);
      /*printf("conv: %d, port: '%s', pid: '%s', host: '%s', rdymsge: '%s'\n",
          i, portstr,pidstr,hostname,readymsge); */
      fclose(sd);
      /* printf(" Done --- \n "); */
      port = atoi(portstr);
      /* printf("port Id: '%s', #: %d\n",portstr, port); */
   }
   else
   {
     Werrprintf(
      "updtparam: Jpsg is not active");
     ABORT;
   }



    /* send this to Jpsg as an update command */
       tSocket = connect2Jpsg(port,hostname);
       if (tSocket == NULL)
       {
           Werrprintf("updtparam: Unable to Connect to JPSG.");
           ABORT;
       }
 
      writeSocket(tSocket,updateCmd,strlen(updateCmd));
      J_sendVPVars(tree,name,tSocket->sd);
      P_endPipe(tSocket->sd);
      /* printf(" COMPLETE \n"); */
      /* printf("Waiting for Ack.\n"); */
      bytes = readSocket(tSocket,ackbuf,256);
      /* printf("bytes recv: %d\n",bytes); */
      ackbuf[bytes]=0;
      /* printf("reply: '%s'\n",ackbuf); */
      if (strcmp(ackbuf,"ack") != 0)
      {
        Werrprintf(
          "updtparam: updtparam Acknowledge Failed\n");
         closeSocket(tSocket);
         ABORT;
      }
      closeSocket(tSocket);

   RETURN;
}

#define MAXAMP 5
#define DACMAX 32767

int check_ShimPowerPars()
{
   int pos, neg;
   double tmpval;
   static int initdone = 0;
   double target;
   
   if ( ! initdone)
   {
      init_shimnames(SYSTEMGLOBAL);
      initdone = 1;
   }
   if (isRRI())
      return(0);
   get_transverseShimSums(1, &pos, &neg);

   target = (MAXAMP * DACMAX) / 1.5;

   /* First assume 1.5 amp for DAC value 32767 */
   if ( (pos <= target) && (neg <= target) )
      return(0);
   if (P_getreal( SYSTEMGLOBAL, "shimsetamp", &tmpval, 1 ))
      tmpval = 1.0;
   target = (MAXAMP * DACMAX) / tmpval;
   if ( (pos <= target) && (neg <= target) )
      return(0);
   
   return(1);
}

static int check_ShimPowerHw(int index, int *newval)
{
   int pos, neg;
   double tmpval;
   static int initdone = 0;
   int oldval;
   double target;
   
   if ( ! initdone)
   {
      init_shimnames(SYSTEMGLOBAL);
      initdone = 1;
   }
   if (isRRI())
      return(0);
   getExpStatusShim(index, &oldval);
   get_transverseShimSums(0, &pos, &neg);

   if (oldval > 0)
      pos -= oldval;
   if (oldval < 0)
      neg += oldval;

   if ( *newval > 0)
      pos += *newval;
   if ( *newval < 0)
      neg -= *newval;

   target = (MAXAMP * DACMAX) / 1.5;
   /* First assume 1.5 amp for DAC value 32786 */
   if ( (pos <= target) && (neg <= target) )
      return(0);
   if (P_getreal( SYSTEMGLOBAL, "shimsetamp", &tmpval, 1 ))
      tmpval = 1.0;
   target = (MAXAMP * DACMAX) / tmpval;
   if ( (pos <= target) && (neg <= target) )
      return(0);
   
   if (*newval >= 0)
      *newval = oldval-1;
   else
      *newval = oldval+1;
   return(1);
}

int shimamp(int argc, char *argv[], int retc, char *retv[])
{
   int pos, neg;
   double tmpval;
   double target;
   
   init_shimnames(SYSTEMGLOBAL);
   pos = neg = 0;
   if ( ! isRRI())
      get_transverseShimSums(1, &pos, &neg);

   if (P_getreal( SYSTEMGLOBAL, "shimsetamp", &tmpval, 1 ))
      tmpval = 1.0;
   target = (MAXAMP * DACMAX) / tmpval;
   if (retc)
   {
      if (retc == 2)
      {
         retv[0] = realString((pos / target) * 100.0);
         retv[1] = realString((neg / target) * 100.0);

      }
      else
      {
         if (pos > neg)
            retv[0] = realString((pos / target) * 100.0);
         else
            retv[0] = realString((neg / target) * 100.0);
      }
   }
   else
   {
      Winfoprintf("Positive shim current is %3.1f %% of maximum. Negative shim current is %3.1f %% of maximum.",
         (pos / target) * 100.0, (neg / target) * 100.0);
   }
   
   RETURN;
}

