/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* --- Has anyone else already typedef these,check on other header files --- */
#if  !defined(SUN_HAL) && !defined(ACQ_SUN)

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
#define ACQ_HALstruc

/* --- Key change made in ``_ACQstat'' and ``a_args'' data structures
       for SUN4 compatibility.  Inserted an extra short word  to properly
       align structure for both SUN3 & SUN4.   --------- */
 

/*****************************************************************************
******************************************************************************/
/*  BEWARE changes to any structures MUST adhere to SUN4 alignment rules !!! */
/*****************************************************************************
******************************************************************************/

/*------------------------------------------------------------------------
|
|       The acquisition & HAL status structure passed back from HAL
|       This is the experimental information that is maintained by
|       HAL (and Acquisition) on the Completed and Active experiments
|       within acquisition.
|
|      added a lock word to structure for syncing between ACQ & HAL 3/18/87
|     6/1/91 readded HALmem2 to maintain even long alignment prior to Genstat
| WARNING: besure to maintain SUN4/SUN3 alignment, remember SUN4 aligns to 
|	   largest member of structure, SUN3 does not.
+-----------------------------------------------------------------------*/
/* defines for HALstatus are in SUN_HAL.h */
struct _ACQstat {
                    /* written to by HAL only if so noted */
                    c68int      statlock;      /* locking flag */
                    c68int      HALstatus;     /* HAL status */
		    c68int	HALfault;	/* type of HAL fault */
		    c68int      HALmem1;      /* unused, added for SUN4 alignment 6/1/91 */
		    struct s_ptr need_l;	/* waiting for (id,num) */
                    c68int      dblksiz;    /* size in bytes of a DRAM block */
                    c68int      dblkcnt;    /* # of allocatable DRAM blocks */
                    c68int      ablksiz;    /* size in bytes of an ARAM block */
                    c68int      ablkcnt;    /* # of allocatable ARAM blocks */
		    c68int	ltblsiz;	/* dimension of element table */
		    c68int      HALmem2;      /* unused, added for SUN4 alignment 6/1/91*/
/* ADD NO ELEMENTS TO BE ZEROED BY HAL PRIOR TO "GO" BEFORE Genstat !! */
                    c68int      Genstat;       /* General Status of Acq. */
                    c68int      ExpID;         /* filled in by HAL */
                    c68int      ExpDoneCode;   /* Acq. Done Code */
                    c68int      ExpErrorCode;  /* Acq. Error Code */
                    c68ulong    ExpFID;        /* present exp element FIBH*/
                    c68long     ExpCT;         /* Completed Transients */
                    c68long     ExpNT;         /* # of Transients to Complete */
                    c68long     ExpBSCT;       /* # of BS completed */
                    c68int      LSDVstates;    /* state of Lk,Spinner,Dec,VT */
                    c68int      AcqHSlines;    /* Acquisition High Speed lines*/
                    c68int      LockLevel;     /* Lock level,updated at BS */
                    c68int      SpinSet;       /* Spinner setting,updated BS */
                    c68int      SpinAct;       /* Actual spinner speed */
                    c68int      VTSet;         /* VT setting, updated at BS */
                    c68int      VTAct;         /* Actual VT Temperature */
                    c68int      TransPWR;      /* Transmitter power setting */
                    c68int      DecoupPWR;     /* Transmitter power setting */
                    c68int      SampleNum;     /* Sample Number in magnet */
                    c68int      ResperR;       /* Respiration Rate of critter */
                    c68int      PulseR;        /* Pulse Rate of critter */
                    c68int      Spares[10];    /* future use */
                };
typedef  struct _ACQstat ACQstatblock;

/*-------------------------------------------------------------------------
|
|       Structure that is filled in by HAL. A pointer to this type of structure
|       is given to acquisition, where all needed information is stored
|
+-------------------------------------------------------------------------*/
struct a_args   {
                        c68int   count; /* = sizeof(struct a_args) */
			c68int   Sun4_align;    /* correct alignment 4 SUN4 */
                        c68char *lc_ptr;
                        c68char *acode_ptr;
                        c68char *fid_table;
                        c68char *RF_ptr;        /* RF wave forms */
                        c68char *grad_ptr;      /* gradient wave forms */
                        c68char *xpan_ptr; /* for future expansion */
                        c68int atEOS;           /* TRUE if EOS vectoring */
                        c68int atBS;            /* TRUE if BS vectoring */
                        c68int atNT;            /* TRUE if NT vectoring */
                        c68int atERR;           /* TRUE if ERR vectoring */
                        c68int atABORT;         /* TRUE if ABORT vectoring */
                        };
/*-----------------------------------------------------------------
|	Structure used by Swept Tune for passing headers back and
|	forth. Swept Tune needs this header so that it knows the
|	frequencies and other experiment parameters used to gather
|	the data. 
+------------------------------------------------------------------*/
typedef  struct {
			c68int valid_data;
			c68int dummy;
			c68long error_code;
			c68long cal_type;
			c68long cal_fidnum;
			c68long expand;
			c68long points;
			c68long power;
			c68long	start;
			c68long stop;
			c68long sweeptime;
		} Tune_hdr;

/* Structure for passing swept tune blocks to xracq */
typedef  struct {
			c68int 	 apwords[14000];
			Tune_hdr header;
		} Bus_table;
