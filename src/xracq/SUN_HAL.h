/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* --- Further change required in ``tutor'' for SUN4.  See notes
       which accompany the definition of this data structure.		*/

/* --- Key change made in ``exp_block'' and ``tutor'' data structures
       for SUN4 compatibility.  Inserted an extra short word after the
       ``cmd'' field so that the remaining longword and pointer fields
       (which are actually longwords) are longword aligned.
       NOTICE THAT THE FIRST 16 BITS OF ANY DATA STRUCTURE THAT HAL
       RECEIVES FROM THE HOST COMPUTER WILL ALWAYS BE THE COMMAND.	*/
       
/*****************************************************************************
******************************************************************************/
/*  BEWARE changes to any structures MUST adhere to SUN4 alignment rules !!! */
/*****************************************************************************
******************************************************************************/

/* --- Has anyone else already typedef these,check on other header files --- */
#if  !defined(ACQ_SUN) && !defined(ACQ_HALstruc)

/* ---- type definitions for consistency between Acquisition & Sun --- */
#ifdef SUN
typedef char  c68char;
typedef short c68int;
typedef long  c68long;
typedef unsigned long  c68ulong;
#else
typedef char  c68char;
typedef int   c68int;
typedef long  c68long;
typedef unsigned long  c68ulong;
#endif

#endif

/* --- define this header file name for conditional compile use of others */
#define SUN_HAL

/* --- SCSI ID Number of HAL Board --- */
#define HAL_SCSI_ID 2			/* modified 11/23/92, from 3  */
#ifndef GEMPLUS
#define HAL_DEVICE	"/dev/rsh0"	/* New Custom Driver, 2/18/88 */
#endif
#define HAL_BLOCK_SIZE	 512

#ifdef GEMPLUS
/* --- Ethernet defines for G2000 interface ------- */
#define  COMMANDS	0
#define  SET_HW		(COMMANDS+1)
#define  STATUS		(SET_HW+1)
#define  ACODES		(STATUS+1)
#define  DATA_P		(ACODES+1)
#define  ACQCODES	(DATA_P+1)
#define  ABORTP		(ACQCODES+1)

#define NUM_REMOTE_CHANNELS	(ABORTP+1)
#define HAL_DEVICE	"gemcon"
#endif

/* tape command definitions */
#define HRESER1 READY           /* dont use "test unit ready" command! */
#define HSENSE  SENSE           /* (if error,clear it &) read HAL status */
#define HRESET  REWIND          /* return to parsing mode (except from error) */
#define HSTATUS SKIP            /* transmit long status block upon next read */
#define HREAD   READ            /* read from HAL */
#define HWRITE  WRITE           /* write to HAL */
#define HFMARK  FMARK           /* file mark not used yet */
#define HERASE  ERASE           /* erase not used yet */


/* parsing command definitions */
/* uses structure exp_block */
#define CREAT_EXP	1	/* create the specified experiment */
#define START_EXP	2	/* start the specified experiment */
#define RSTART_EXP	3	/* restart the specified experiment */
#define STOP_EXP        4	/* stop acquisition */
#define RESUME_EXP      5	/* resume acquisition */
#define FINI_EXP	6	/* finish the specified experiment */
#define ABORT_EXP       7	/* abort acquisition */
#define KILL_EXP	8	/* kill the specified experiment */
#define RECV_ELE        9	/* receive parameters/acodes/fid/etc */
#define SEND_ELE        10	/* transmit parameters/acodes/fid/etc */
#define FINI_ELE        11	/* through with specified (exp, elem) */
#define KILL_ELE	12	/* kill the specified element */

/* uses structure tutor */
#define RECV_PRG        13	/* receive program */
#define EXEC_PRG        14	/* execute program */
#define RECX_PRG        15	/* receive program and execute */
#define SEND_RAM        16	/* dump memory */

/* uses structure interact */
#define INTERACTIVE     17	/* enter interactive mode */

/* uses structure serial_block */
#define TRANSPARENT     18	/* serial emulation (transparent mode) */

/* can use any structure (no arguments passed) */
#define VERBOSE         19	/* turn on verbose HAL serial output */
#define SILENT          20	/* turn off verbose HAL serial output */
				/* do we want the same for ACQ? */
#define SUPER_ABORT     21	/* re-enter HALmon */

/* uses structure exorcist */
#define SELFTEST        22	/* self-test */

/* no arguments passed */
#define POP_ACQ_PROG	23	/* reset acq buffer size flag to what it was */
				/*  before last program was downloaded. */

/* Global Segment Definitions */
/* Global segments are defined by the negated experiment id (x_id) and 	*/
/* element number set to the following definitions.  M. Howitt 900214	*/
/* added to NMRID source by Greg Brissey 6/14/90   900614 */
#define ACODE_GLOBALSEG	-1L
#define RF_GLOBALSEG	-2L
#define GRAD_GLOBALSEG	-3L
#define XPAN_GLOBALSEG	-4L

/* structure for interactive modes */
/* OK as is for the SUN4 */

struct interact {
                c68int  cmd;
                c68int  mode;           /* lock, fid display, shim?? */
                c68long idatasize;      /* total size of data table in bytes */
		c68int	d_p_f;		/* dble_precision_flag in fiddisplay */
		c68int	fid_scale;	/* scale for ybars (100 times) */
                };

/* The value of constructs like 'ab' is implementation-defined.
 * Use the following macro to construct a system independent value. */
#define CHR_PAIR(x, y) (0x100*(x) + (y))

#define LMODE   CHR_PAIR('l', 'o')    /* lock display (LOCKI) */
#define FMODE   CHR_PAIR('f', 'i')    /* FID display */
#define SMODE   CHR_PAIR('s', 'h')    /* interactive shimming (SHIMI) */
#define AMODE   CHR_PAIR('a', 's')    /* auto shimming */
#define GMODE	CHR_PAIR('g', 'm')    /* Buffered FID display */
#define TMODE   CHR_PAIR('t', 'u')    /* Swept tune */

/* structure for self test */
/* OK as is for the SUN4 */

struct exorcist         {
                        c68int cmd;
                        c68int target1;
                        c68int test1;
                        c68int target2;
                        c68int test2;
                        c68int target3;
                        c68int test3;
                        c68int target4;
                        c68int test4;
                        };


/* structure for download, execute, download&execute commands   */
/* Modified for the SUN4, inserting a dummy shortword following */
/* the ``cmd'' field and swapping the order of ``exec_addr''    */
/* and ``execute'', for the first field is an address (4 bytes)	*/

struct tutor            {
                        c68int  cmd;
			c68int  dummy_for_sun4;
                        c68long srec_size;      /* bytes to receive */
                        c68char *base_addr;     /* a VERSAbus address! */
                        c68char *offset;        /* download offset from base */
                        c68char *exec_addr;     /* execute offset from base */
                        c68int  execute;        /* execute/not if 1/0 */
                        c68int  mpu;            /* which processor executes */
			c68long ram_adr;	/* start address in HAL 4 mem dump */
			c68long ram_siz;	/* data size of mem dump (SEND_RAM)*/
                        };

/* defines for tutor.mpu */
#define HALMPU  0
#define ACQMPU  1


/* Interleaving structure passed from the host - contains experiment id and fid
   number of the set of acquisition interpreter arguments to use when at
   end-of-scan, BS, and NT.  These "symbolic pointers" have to be resolved
   by the HAL memory management routines into ACQbus addresses before they
   can be passed to the acquisition interpreter. 
   elem was changed to ulong tobe capable of large number of fids in 3D
   exp was change to a long only to maintain the SUN4/SUN3 alignment
   for struct _ACQstat (statblock).
*/
struct s_ptr    {
                        c68long exp;     /* which experiment id */
                        c68ulong elem;    /* which element number within expt */
                        };


/* Command structure for acquisition code/data transfered between the host
   and HAL.  This structure will be followed by a "tape write" containing
   the low core and the automation parameter structure (if parsiz <> 0); a
   "tape write" containing the acodes (if codesiz <> 0); and a "tape write"
   containing the fid data (if fid_size <> 0) */

/* Modified for the SUN4, with the change designed to be self-evident */

struct exp_block        {
                        c68int  cmd;            /* effectively struct id */
                        c68int  aligncmd;       /* unused for 680xx - SPARC alignment */
			c68ulong sub_cmd;	/* dummy_for_sun4, now used for sa */
                        c68long max_size;       /* for upward compatibility */
                        c68long  x_id;           /* unique experiment id */
                        c68ulong  l_num; 	/* element number */
                        c68ulong  tot_l;	/* total number of elements */
                        c68long  pipe_l;         /* # of elements to pipeline */
                        c68long parsiz;         /* size of lc + auto structs */
                        c68long codesiz;
                        c68long RFsiz;          /* size of RF waveform data */
                        c68long gradsiz;        /* size of gradient waveforms */
                        c68long xpansiz;        /* reserved for expansion */
                        c68long datasiz;        /* size of transferred data */
                        c68long datatable;      /* size of data table needed */
			c68long	BSused;		/* BS used if TRUE */
			c68long	BSil;		/* BS interleaving if TRUE */
                        struct  s_ptr at_eos;   /* where to go at end-of-scan */
                        struct  s_ptr at_bs;    /* where to go at block size */
                        struct  s_ptr at_nt;    /* where to go at ct == nt */
                        struct  s_ptr at_err;   /* where to go when error */
                        struct  s_ptr at_abort; /* where to go when abort */
                        };


/* special values for exp_block fields */
/* avoid checking for transmission of segments with this size in FINI_EXP */
#define DISCARD         0x80000000L
#define RETAIN          0x0L


/* used for transparent mode, transmit/receive serial data to/from acq */
/* OK as is for the SUN4 */
/* message increased to make total size 512 bytes */

struct serial_data      {
                        c68int  cmd;            /* effectively struct id */
                        c68int  message[255];	/* was 510 characters */
                        };


/* used for determining sizes of sense messages and error messages */
struct fanmail	{
		char *fodder;
		};


/* Sense info returned by HAL */
/*
 software modes:
 HAL operates in several modes:
        HALbug          a modification of the VM02 VERSAbug; no SCSI
                        communications capability with the host
        HALtape         Sysgen SC4000 tape controller emulation
        state machine   Sysgen SC4000 tape controller compatibility with
                        customization of most of the SCSI commands for
                        acquisition control and communications
 The HAL operating mode is determined from the host by the software mode
 indication protocol defined immediately in the following section.
 (It should be noted that the HALbug mode cannot be discriminated from a
 "power off" or "hung" condition by the host, since the HALbug mode does
 not perform any SC4000 emulation functions!)

 software mode indication protocol (tape emulation and state machine):
 Setting of the HALTAPE condition indicates that HAL is in the tape
 emulation mode and has not entered the state machine (HALmon()) since
 entering the tape emulation mode.  When this condition is clear then HAL is
 in the state machine and ready to accept parse blocks and other state machine
 commands from the host.
 Error exits from the state machine to the tape
 emulation mode will set HALFERR to indicate this condition to the host.
 (TO BE IMPLEMENTED: error exits to tape emulation from the state
 machine should also set other conditions as appropriate to indicate to
 the host the reason for the error exit.)

NEED TO UPDATE AS OF 870310! (statblock buffer queue/BS protocol/etc)
 active statblock/statblock buffer queue protocol:
 the active statblock residing in shared memory is copied into a buffer
 queue under some circumstances

 error protocol (state machine):
 Setting of any of the following conditions singly or in combination
 results in a transition to the error state.  Once in the error state,
 all further SCSI commands except "request sense" are ignored
 (terminated with BUSY).  Receipt of the "request sense" command clears
 all of the following conditions that are set.
        NOACQSW
        NOACQHW
        HALTAPE (should never be set in the state machine, but cleared anyway)
        HALFERR (special case: causes return to HALtape() from HALmon())
        HALFULL
        ACQFULL
        BADSCSI
        PERROR
        SQERROR
        ACQDEAD (unimplemented as of yet)
        SCSI_ERR        (unimplemented as of yet)

The following error conditions also set statblock.HALstatus (definitions of
possible values follow the sense block definitions).  This provides a
means of expanding these error conditions into more specific
descriptions.  The value of statblock.HALstatus remains unchanged until
a read of the HAL status structure is performed (HAL "STATUS" command, at
which point it is set to HALOK), or the current value is overwritten by a
subsequent error or attention condition.
        HALFULL
        ACQFULL
        BADSCSI
        PERROR
        SQERROR

The error protocol, therefore, can be broken into two types: long and
short.  The long error protocol is used only for those conditions involving
conditions that set statblock.HALstatus.  It consists of clearing the error
state by doing a SCSI "request sense", followed by a HAL STATUS command to
interrogate and clear statblock.HALstatus.  The HAL STATUS phase is
optional.  The short error protocol is used for all conditions that do
not set statblock.HALstatus.  It consists of only the SCSI "request sense"
phase of the long error protocol.

attention protocol (state machine):
The occurrance of a non-error condition that requires service from the
host (ex: reading the data buffer after a block size or CT == NT, or
reception of a warning message from acquisition (ex: low noise),
or requesting non-resident acodes and/or parameters for an element of a
multi-dimensional experiment that is next to execute but not resident in
HAL) will set the HALATTN condition in the sense block, as well as
statblock.HALstatus.  This condition will not be cleared until a HAL
STATUS command is issued.  The value of statblock.HALstatus will remain
unchanged until either the HAL STATUS command is issued (sets value to
HALOK) or statblock.HALstatus is overwritten by a subsequent error or attention
condition.
(NOTE: SETTING OF statblock.HALstatus IS CURRENTLY NOT DONE FOR
ATTENTION CONDITIONS - IT IS ASSUMED THAT THE HOST WILL INTERROGATE
statblock.ExpDoneCode and statblock.ExpErrorCode to determine the cause
of the attention condition)

miscellaneous conditions:
PARSEM: used to indicate that HAL is in the parse state of the state
machine.  This is neither an error or attention condition.

PWR_UP: used for sc4000 emulation in the tape emulation.
*/

#ifdef ERRORFILE
struct fanmail SenseMsg[] =
	{
#else
#ifndef NOMSGSTRUCTS
extern struct fanmail SenseMsg[];
#endif
#endif
/* byte 1 and 2 */
#define HALIDLE 0x0000
#define SENSEMASK	0x7f7f
#ifdef ERRORFILE
	"none of the following conditions ",
#endif


/* byte 2 (lsb of sblock[2]) */
#define PWR_UP  0x0081
#ifdef ERRORFILE
	"power on or reset since last operation ",
#endif

/* reserved */
#define HALFERR 0x0082
#ifdef ERRORFILE
	"fatal error return from HALmon ",
#endif

/* reserved */
#define HALTAPE 0x0084
#ifdef ERRORFILE
	"HAL tape emulation mode ",
#endif

#define PARSEM  0x0088
#ifdef ERRORFILE
	"HAL in parse state ",
#endif

#define SQERROR 0x0090
#ifdef ERRORFILE
	"statblock buffer queue error ",
#endif

#define PERROR  0x00a0
#ifdef ERRORFILE
	"protocol error - read active HAL status block ",
#endif

#define BADSCSI 0x00c0
#ifdef ERRORFILE
	"SCSI command not used or unexpected ",
#endif


/* byte 1 (msb of sblock[2]) */
/* a placeholder bit */
#define DUMBIT	0x8000
#ifdef ERRORFILE
	/* a placeholder message */
	"this message should never appear!",
#endif

#define ACQFULL 0x8100
#ifdef ERRORFILE
	"no room in ACQ RAM for arguments ",
#endif

#define HALFULL 0x8200
#ifdef ERRORFILE
	"experiment table, element table, or data buffers full ",
#endif

#define SCSI_ERR 0x8400
#ifdef ERRORFILE
	"SCSI driver error during a data transfer phase ",
#endif

#define HALATTN 0x8800
#ifdef ERRORFILE
	"statblock buffer queue not empty ",
#endif

#define ACQDEAD 0x9000
#ifdef ERRORFILE
	"VERSAbus bus trap or ACQ doesn't acknowledge interrupts ",
#endif
	/* use for missing ACQ hardware also? */

#define RFAULT	0xa000
#ifdef ERRORFILE
	"ACQ sleeping on resource (element, HAL DRAM, ACQ RAM) ",
#endif

#define NOACQSW 0xc000
#ifdef ERRORFILE
	"no ACQ software downloaded "
	/* (or in vbug after boot? ) */
	};
#endif
#define SENSIZE	sizeof(SenseMsg)/sizeof(struct fanmail)


/* defines for HALstatus and HALfault */
/* note that "transmitted" means host --> HAL; conversely for "received" */

#ifdef ERRORFILE
struct fanmail HalErrMsg[] =
	{
#else
#ifndef NOMSGSTRUCTS
extern struct fanmail HalErrMsg[];
#endif
#endif
#define NOMES0		0
#ifdef ERRORFILE
	"message unused",
#endif

#define HALOK           1
#define HIDLE           1
#ifdef ERRORFILE
	"no error conditions",
#endif

#define HACTIVE         2
#ifdef ERRORFILE
	"HAL active",
#endif

/* error values - should be cleared after sucessful HAL status transfer */
#define HERROR          3
#ifdef ERRORFILE
	"any error other than BADSCSI, PERROR, HALFULL, ACQFULL",
#endif

#define NOMES4		4
#ifdef ERRORFILE
	"message unused",
#endif

#define NOMES5		5
#ifdef ERRORFILE
	"message unused",
#endif

#define NOMES6		6
#ifdef ERRORFILE
	"message unused",
#endif

#define NOMES7		7
#ifdef ERRORFILE
	"message unused",
#endif

#define NOMES8		8
#ifdef ERRORFILE
	"message unused",
#endif

#define NOMES9		9
#ifdef ERRORFILE
	"message unused",
#endif

#define WBLKNE1         10
#ifdef ERRORFILE
	"cmd == WRITE && size != 1 block in ",
#endif

#define RBLKNE1         11
#ifdef ERRORFILE
	"cmd == READ && size != 1 block in ",
#endif

#define PARSIZ0         12
#ifdef ERRORFILE
	"element received with parameter size = 0 ",
#endif

#define CODESIZ0        13
#ifdef ERRORFILE
	"element received with acode size = 0 ",
#endif

#define DATATBL0        14
#ifdef ERRORFILE
	"element received with data table size = 0 ",
#endif

#define DATAOFLO        15      
#ifdef ERRORFILE
	"data table size greater than available DRAM ",
#endif

#define EXPEXISTS	16	
#ifdef ERRORFILE
	"experiment to create already exists ",
#endif

#define SEGSIZXC	17	
#ifdef ERRORFILE
	"segment size exceeds maximum requested ",
#endif

#define ILLSCSI         18      
#ifdef ERRORFILE
	"illegal SCSI ",
#endif

#define UXPSCSI         19      
#ifdef ERRORFILE
	"unexpected SCSI ",
#endif

#define GENSTATE        20
#ifdef ERRORFILE
	"unspecified state error ",
#endif

#define GENPARSE        21
#ifdef ERRORFILE
	"unspecified parse error ",
#endif

#define URCOGBLK        22      
#ifdef ERRORFILE
	"unrecognized parse block ",
#endif

#define EXPFULL         23      
#ifdef ERRORFILE
	"experiment table full ",
#endif

#define ELEMFULL        24      
#ifdef ERRORFILE
	"element table full ",
#endif

#define EXPALLDUN       25      
#ifdef ERRORFILE
	"new element received for experiment already done ",
#endif

#define ARGFULL         26      
#ifdef ERRORFILE
	"no room for arguments in vm02 RAM ",
#endif

#define PARFULL         27      
#ifdef ERRORFILE
	"no room for parameters in vm02 RAM ",
#endif

#define CODEFULL        28      
#ifdef ERRORFILE
	"no room for acodes in vm02 RAM ",
#endif

#define RFFULL          29      
#ifdef ERRORFILE
	"no room for RF patterns in vm02 RAM ",
#endif

#define GRADFULL        30      
#ifdef ERRORFILE
	"no room for gradient patterns in vm02 RAM ",
#endif

#define XPANFULL        31      
#ifdef ERRORFILE
	"no room for expansion segments in vm02 RAM ",
#endif

#define DATAFULL        32      
#ifdef ERRORFILE
	"no room for data table in HAL DRAM ",
#endif

#define ALLSIZ0         33 
#ifdef ERRORFILE
	"all sizes are 0 in global or transmit parse block ",
#endif

#define NOGLOPARS       34      
#ifdef ERRORFILE
	"global parameters not allowed ",
#endif

#define NOGLOCODE       35      
#ifdef ERRORFILE
	"can't find global acodes ",
#endif

#define NOGLORF         36      
#ifdef ERRORFILE
	"can't find global RF patterns ",
#endif

#define	BAD_BSV		37	
#ifdef ERRORFILE
	"invalid BS vector ",
#endif

#define EXPNFOUND       38      
#ifdef ERRORFILE
	"can't find experiment for receive/start/send/finish ",
#endif

#define ELEMNFOUND      39      
#ifdef ERRORFILE
	"can't find element for start/transmit/finish ",
#endif

#define ELENEVRUN       40      
#ifdef ERRORFILE
	"element for transmission or finish has never run ",
#endif

#define NO_L_1          41      
#ifdef ERRORFILE
	"can't find element 1 for start ",
#endif

#define ELEFALTD        42      
#ifdef ERRORFILE
	"element for finish is faulted ",
#endif

#define ELABORTD        43      
#ifdef ERRORFILE
	"element for transmission or finish has been aborted ",
#endif

#define PARSNSENT       44      
#ifdef ERRORFILE
	"parameters never sent for finished element ",
#endif

#define DATANSENT       45      
#ifdef ERRORFILE
	"data never sent for finished element ",
#endif

#define NOGLOGRAD       46      
#ifdef ERRORFILE
	"can't find global gradient patterns ",
#endif

#define NOGLOXPAN       47      
#ifdef ERRORFILE
	"can't find global expansion segment ",
#endif

#define NOGLODATA       48      
#ifdef ERRORFILE
	"global data not allowed ",
#endif

#define	BAD_EOSV	49	
#ifdef ERRORFILE
	"invalid EOS vector ",
#endif

#define	BAD_NTV		50	
#ifdef ERRORFILE
	"invalid CT == NT vector ",
#endif

#define PSIZNOTSIZ      51  
#ifdef ERRORFILE
	"read/write & scsi block size != ltbl p_siz in ",
#endif

#define CSIZNOTSIZ      52  
#ifdef ERRORFILE
	"read/write & scsi block size != ltbl c_siz in ",
#endif

#define RSIZNOTSIZ      53  
#ifdef ERRORFILE
	"read/write & scsi block size != ltbl r_siz in ",
#endif

#define GSIZNOTSIZ      54  
#ifdef ERRORFILE
	"read/write & scsi block size != ltbl g_siz in ",
#endif

#define XSIZNOTSIZ      55  
#ifdef ERRORFILE
	"read/write & scsi block size != ltbl x_siz in ",
#endif

#define DSIZNOTSIZ      56  
#ifdef ERRORFILE
	"read/write & scsi block size != ltbl d_siz in ",
#endif

#define DSIZOFLO        57      
#ifdef ERRORFILE
	"data block size greater than available DRAM ",
#endif

#define DBUFFULL        58      
#ifdef ERRORFILE
	"no room for data buffer in HAL DRAM ",
#endif

#define FREE_UFLO       59      
#ifdef ERRORFILE
	"statblock buffer free list underflow ",
#endif

#define CSIZNEG         60     
#ifdef ERRORFILE
	"global code size < 0 ",
#endif

#define ELSTILLRUN      61      
#ifdef ERRORFILE
	"element for finish is still running ",
#endif

#define RSIZNEG         62     
#ifdef ERRORFILE
	"global RF size < 0 ",
#endif

#define GSIZNEG         63     
#ifdef ERRORFILE
	"global gradient size < 0 ",
#endif

#define XSIZNEG         64     
#ifdef ERRORFILE
	"global expansion size < 0 ",
#endif

#define INOTIMP         65      
#ifdef ERRORFILE
	"unimplemented interactive mode ",
#endif

#define ELEXISTS        66      
#ifdef ERRORFILE
	"parse block received for existing element ",
#endif

#define DSIZNEG         67      
#ifdef ERRORFILE
	"data block size < 0 ",
#endif

#define STATQ_MT        68  
#ifdef ERRORFILE
	"attempted read of empty statblock buffer queue ",
#endif

#define GENQUEUE        69
#ifdef ERRORFILE
	"unspecified statblock queue error ",
#endif

#define NOPAR           70      
#ifdef ERRORFILE
	"can't find parameters ",
#endif

#define NOCODE          71      
#ifdef ERRORFILE
	"can't find acodes ",
#endif

#define NORF            72      
#ifdef ERRORFILE
	"can't find RF patterns ",
#endif

#define NOGRAD          73      
#ifdef ERRORFILE
	"can't find gradient patterns ",
#endif

#define NOXPAN          74      
#ifdef ERRORFILE
	"can't find expansion ",
#endif

#define NORDATA         75      
#ifdef ERRORFILE
	"can't find resumed data ",
#endif

#define STATNSAV        76      
#ifdef ERRORFILE
	"statblock never buffered for finished element ",
#endif

#define TMPU_ERR        77      
#ifdef ERRORFILE
	"unimplemented mpu for download and/or execute ",
#endif

#define ILL_BASE        78      
#ifdef ERRORFILE
	"illegal base address for download and/or execute ",
#endif

#define OSIZNOTSIZ      79  
#ifdef ERRORFILE
	"write object & scsi block size != srec_size in ",
#endif

#define PSIZNEG         80      
#ifdef ERRORFILE
	"received/transmitted parameter size < 0 ",
#endif

#define PSIZOFLO        81      
#ifdef ERRORFILE
	"parameter size > ACQ argument RAM size ",
#endif

#define FLBS_UFLO       82      
#ifdef ERRORFILE
	"statblock buffer free list underflow during BS service ",
#endif

#define FREE_OFLO       83      
#ifdef ERRORFILE
	"statblock buffer free list overflow ",
#endif

#define SQELEM_MISSING  84      
#ifdef ERRORFILE
	"statblock BS buffer points to a missing ltbl entry ",
#endif

#define S_DERROR        85      
#ifdef ERRORFILE
	"SCSI driver data transfer error ",
#endif

#define CANT_DOWNLOAD   86      
#ifdef ERRORFILE
	"can't download into ACQ RAM - ACQ is busy ",
#endif

#define CANT_EXEC       87   
#ifdef ERRORFILE
	"can't execute ACQ program - ACQ already busy ",
#endif

#define CANT_GO         88      
#ifdef ERRORFILE
	"can't start experiment - ACQ already busy ",
#endif

#define CANT_ABORT      89      
#ifdef ERRORFILE
	"can't abort experiment - ACQ already idle ",
#endif

#define CANT_LOCK       90      
#ifdef ERRORFILE
	"can't start lock machine - ACQ already busy ",
#endif

#define CANT_SHIM       91      
#ifdef ERRORFILE
	"can't start shim machine - ACQ already busy ",
#endif

#define BAD_SP          92      
#ifdef ERRORFILE
	"can't restart HALmon - saved stack pointer corrupt ",
#endif

#define BAD_PC          93      
#ifdef ERRORFILE
	"can't restart HALmon - saved program counter corrupt ",
#endif

#define STAT_HUNG       94      
#ifdef ERRORFILE
	"can't lock statblock ",
#endif

#define PIPEVOFLO       95      
#ifdef ERRORFILE
	"element pipeline exceeds vm02 RAM ",
#endif

#define PIPEDOFLO       96      
#ifdef ERRORFILE
	"element pipeline exceeds HAL DRAM ",
#endif

#define PIPELOFLO       97      
#ifdef ERRORFILE
	"element pipeline exceeds element table size ",
#endif

#define PIPELUFLO       98      
#ifdef ERRORFILE
	"element pipeline < 1 ",
#endif

#define	XSTARTB4	99	
#ifdef ERRORFILE
	"experiment was started already ",
#endif

#define	PIPENCOMPL	100	
#ifdef ERRORFILE
	"element pipeline not complete ",
#endif

#define CSIZOFLO        101     
#ifdef ERRORFILE
	"code size > ACQ argument RAM size ",
#endif

#define RSIZOFLO        102     
#ifdef ERRORFILE
	"RF size > ACQ argument RAM size ",
#endif

#define GSIZOFLO        103     
#ifdef ERRORFILE
	"gradient size > ACQ argument RAM size ",
#endif

#define XSIZOFLO        104     
#ifdef ERRORFILE
	"expansion size > ACQ argument RAM size ",
#endif

#define	PIPE_GT_TOT	105	
#ifdef ERRORFILE
	"element pipeline > total elements ",
#endif

#define T_L_LT1		106	
#ifdef ERRORFILE
	"total elements in experiment < 1 ",
#endif

#define	BADXID		107	
#ifdef ERRORFILE
	"experiment id out of range ",
#endif

#define	BADLNUM		108	
#ifdef ERRORFILE
	"element number out of range ",
#endif

#define LFAULT		109
#ifdef ERRORFILE
	"sleeping on reception of an element ",
#endif

#define PBFAULT		110
#ifdef ERRORFILE
	"sleeping on availability of a parameter buffer ",
#endif

#define DBFAULT		111
#ifdef ERRORFILE
	"sleeping on availability of a data buffer ",
#endif

#define DTFAULT		112
#ifdef ERRORFILE
	"sleeping on availability of a data table ",
#endif

#define BSFAULT		113
#ifdef ERRORFILE
	"sleeping on HOST BS service (data read) ",
#endif

#define BAD_EXEC	114	
#ifdef ERRORFILE
	"ACQ didn't execute program ",
#endif

#define EXPNFALTD	115	
#ifdef ERRORFILE
	"experiment is not faulted ",
#endif

#define ELENFALTD	116	
#ifdef ERRORFILE
	"element is not faulted ",
#endif

#define BADRSTART	117	
#ifdef ERRORFILE
	"restart of experiment failed ",
#endif

#define	CANTFINX	118	
#ifdef ERRORFILE
	"can't finish experiment ",
#endif

#define BADBSFLGS	119	
#ifdef ERRORFILE
	"contradictory BS flags "
	};
#endif
#define ERRMSGSIZ = sizeof(HalErrMsg)/sizeof(struct fanmail)


