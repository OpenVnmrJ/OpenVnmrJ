/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/************************************************************************
* wg.c
*	Routines that will generate WaveForm Gen patterns that will be
*	executed in a pulse sequence.
*************************************************************************/


#include <sys/file.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <netinet/in.h>
#include "oopc.h"
#include "group.h"
#include "acodes.h"
#include "rfconst.h"
#include "acqparms.h"
#include "vnmrsys.h"
#include "macros.h"
#include "apdelay.h"
#include "aptable.h"
#include "vfilesys.h"
#include "pvars.h"
#include "abort.h"

extern int rcvroff_flag;	/* global receiver off flag */
extern int rcvr_hs_bit;		/* old = 0x8000, new = 0x1 */
extern int bgflag;		/* print diagnostics if not 0 */
extern int checkflag;		/* check sequence only. No acquisition */
extern char filexpath[];	/* ex: $vnmrsys/acqque/exp1.russ.012345 */
extern char fileRFpattern[];	/* absolute path name of RF load file */
extern char filexpan[];	        /* absolute path name of xpansion load file */
extern int curfifocount;	/* current fifo count */
extern char rfwg[];
extern char gradtype[];
extern int      ap_interface;
extern int	rfp270_hs_bit;
extern int	dc270_hs_bit;
extern int	dc2_270_hs_bit;
extern int	newacq;

extern int isBlankOn(int device);
extern void	setHSLdelay();
extern void HSgate(int ch, int state);
extern int putFile(const char *fname);

#define N_HSLS_PER_CHANNEL	5
#define WFG_HSL_OFFSET		2
#define DECOUPLE		1
#define SPINLOCK		( wtg_active ? 2 : DECOUPLE )
/* #define WFG_OFFSET_DELAY	( (ap_interface < 4) ? 1.5e-6 : 0.45e-6 ) */
#define WFG_TIME_RESOLUTION	( (ap_interface < 4) ? 2 : 1 )
#define TIME_AMPL_GATE_BLANK	~( ((unsigned int)7 << 29) | 0x7ffff )
#define TIME_GATE_BLANK		~( ((unsigned int)7 << 29) | 0x400ff )
#define WFG_PULSE_FINAL_STATE	( (ap_interface < 4) ? TIME_AMPL_GATE_BLANK :\
					TIME_GATE_BLANK )

#define AMPGATE		0x10000000
#define MINWGTIME	2.0e-7
#define INOVA_MINWGSETUP	7.5e-7	/* time to allow RF wg's to 	*/
					/* process instruction blks.	*/
#define INOVA_MINWGCMPLT	5.0e-7  /* time for inova RF wg's to 	*/
					/* complete waveforms.		*/
/* MINGRADTIME is 2 usec in WFG ticks */
#define MINGRADTIME	39

#define  IB_START	0x00000000
#define  IB_STOP	0x20000000
#define  IB_SCALE	0x40000000
#define  IB_DELAYTB	0x60000000
#define  IB_WAITHS	0x80000000
#define  IB_PATTB	0xa0000000
#define  IB_LOOPEND	0xc0000000
#define  IB_SEQEND	0xe0000000

/* Unique pattern tags indicate how patterns are used */
#define  TAG_CONST	1	/* Pattern commands are fixed */
#define  TAG_VAMP	2	/* Amplitude word changed in real time */

#define	 D_FULLP	0x07fff
#define	 D_FULLM	0x08001
#define	 D_ZERO		0
#define  D_HLFP		0x04000
#define  D_HLFM		0x0C000
#define  D_34P		0x06000
#define  D_34M		0x0a000
#define  D_14P		0x02000
#define  D_14M		0x0e000
#define  D_15P		0x01999
#define  D_15M		0x0e667
#define  D_45P		0x06666
#define  D_45M		0x0999a

#define  NO_PULSE	0
#define  HARD_PULSE	1
#define  SOFT_PULSE	2

/* 
		instruction block contents

31  29 28 27   24 23 		16 15		8 7		0
 0 0 0  G U x x x  |---start address (16 bits)--| x x x x x x x x  
   (start address specification 7-89 board )
 0 0 1  G x x x x  |--- finish address + 1    --| |- delay mult-|
   (stop address specification )
 0 1 0  G x x x x  | loop count |  | amplitude scalar 16 bits   |	
   (loop and scale specification )
 0 1 1  G |-- (28 bit delay specification -1) times 50 ns ------|
   (delay specification 200 ns mimimum to 13.7 sec maximum      )
   (times 0-255 delay count 3422 seconds  about 1 hour)
 1 0 0  G x x x x x x x x x x x x x x x x x x x x x x x x x x x x
   (wait for high speed line command)
 1 0 1  G |-- (28 bit pattern specification -1) times 50ns
   (time for each pattern state times the pattern rel dur field)
 1 1 0  G x x x x x x x x x x x x x x x x x x x x x x x x x x x x
   ( end of loop word ) 
 1 1 1  G x x x x x x x x x x x x x x x x x x x x x x x x x x x x
   ( end of sequence word )


		pattern content

		gradient model

31           24 23 		     		8 7		0	
x x x x x x x x  |------ 16 bit data------------| |--- dwell ---|

		rf model

 31   30  29  28                 19   18  17               8  7             0	
| 2 | 1 |amp |--- 10 bits phase ---| hard |-- 10 bits amp --| |--- dwell ---|
| spares|gate|                     | pulse|

*/

#define pat_strt(addr1)	(IB_START | ((addr1 & 0x0ffff) << 8) | 0x08000000)
#define pat_fstrt(addr1)	(IB_START | ((addr1 & 0x0ffff) << 8))
#define pat_stop(addr2,delaym) (IB_STOP  | ((addr2 & 0x0ffff) << 8) | (delaym & 0x0ff))
#define pat_scale(loops,scale)	(IB_SCALE|((loops & 0x0ff)<<16)|(scale & 0x0ffff))
#define pat_wait_hs	(IB_WAITHS)
#define pat_delay(dtime)	(IB_DELAYTB | (dtime & 0x0fffffff))
#define pat_time(pattime)	(IB_PATTB |   (pattime & 0x0fffffff))
#define pat_loop	(IB_LOOPEND)
#define pat_end		(IB_SEQEND)

#define G_VALUE(value,dwell)	(((value & 0x0ffff) << 8) | (dwell & 0x0ff))
#define RF_VALUE(amp,phase,dwell) (((amp & 0x03ff) << 8) | ((phase & 0x03ff) << 19) | (dwell & 0x0ff) | 0x20000000)
#define T_VALUE(value,dwell)	(((value & 0x00ffffff) << 8) | (dwell & 0x0ff))

/* template control ok 

   ib control preliminary
*/
/*
   Preliminary Waveform Generator Software

   By default instruction blocks 
   are matched and terminated with 
   an end of pattern command. 
   Each block is then pointed to 
   and executed.  Under certain conditions,
   it is permissable to fall through to 
   the next ib (they are made up sequentially).
   If a block terminates normally, one can
   start the next ib with just a start command.
   If a block falls through (i.e. no end of pattern),
   the next block will START IMMEDITATELY! 
   Some provision is made to support these
   tricks via the r__cntrl field 
   handle h1->r__cntrl & MASKS
   Additionally, rf gating mode1 etc will 
   use these features 
   --- THESE ARE LIKELY TO BE VERY CONFUSING ---
   --- AND ARE NOT YET IMPLEMENTED           ---
   ---  1st try 1-25-90 pah                  ---
   */
/* r__cntrl MASKS defined */
#define ALLWGMODE	1
#define FALLTHRU 	2
#define ZOO		FALLTHRU | ALLWGMODE
/* bits 2-15 reserved */

/*  
    advance mode supports needs
    zoo mode
    set flags 
    counts on sequential 
    points to cur_ib explicitly !!!
    inhibits matching and generation of blocks past
    ix == 1 
    turns off termination word
    explict termination word needed
*/
struct r_data
{
   int   nfields;
   double f1,f2,f3,f4,f5;
   struct r_data *rnext;
};

static struct r_list
{
   char tname[MAXSTR];
   int  suffix;
   int  index;
   double max_val;
   double tot_val;
   int   nvals;
   struct r_data *rdata;
   struct r_list *rlink;
}
raw_root;

struct p_list 
{
  int   rindex;
  int   scale_pwrf;
  double scl_factor;
  int	wg_st_addr;
  int  telm,ticks,status,cycle_size;
  int   *data;
  struct p_list *plink;
}; 

struct ib_list
{
  char tname[MAXSTR];	/* array no 2D   */
  double width, 	/* array or 2D   */
	amp;		/* array or 2D   */
  double factor;
  int  iloop;		/* array or 2D	 */
  int  bstat;		/* a key word matches for 2d/dynamic 
			   or decoupler precision*/
  int  nticks;          /* total number of 50 ns ticks in the pattern;
                           currently used only for WFG spinlocks */
  int  apstaddr;
  struct ib_list *ibnext;
};

#define WGRADIENT 2
#define WRFPULSE  3
#define WDECOUPLER  4
#define NWGS	8
#define COMMENTCHAR '#'
#define LINEFEEDCHAR '\n'

static struct pat_root
{
   int cur_tmpl;
   int cur_ib;
   int r__cntrl;
   int r__index;
   int ap_base;
   struct p_list  *p_base;
   struct ib_list *ib_base;
}  
Root[NWGS];

typedef struct pat_root *handle;

/***************************************
   changes to this structure REQUIRE
   changes to XR! 
   a global write header!
***************************************/
struct gwh 
{  
  unsigned short ap_addr,
		 wg_st_addr,
		 n_pts,
		 gspare; 
};

static int d_in_RF = 0;
static int read_in_pattern(struct r_data *rdata, int *lpntr, int *ticker,
                           int tp, double scl);
static void ib_safe();
static struct p_list   *resolve_pattern();
static struct p_list   *pat_list_search();
static struct p_list   *pat_file_search();
static struct r_list   *pat_file_found();
static struct r_list   *read_pat_file();
static struct pat_root *get_handle();
static struct ib_list  *resolve_ib();
static struct ib_list  *create_pulse_block();
static struct ib_list  *create_dec_block();
static struct ib_list  *create_grad_block();

static char filep3[MAXSTR];		/* library load mgmt */
static FILE  *p__open();
static int wtg_active = FALSE;
extern char userdir[],systemdir[];
extern int grad_flag;
static FILE *global_fw = NULL;
static double dpf;

int getlineFIO(FILE *fd,char *targ,int mxline,int *p2eflag);
void point_wg(int which_wg, int where_on_wg);
void dump_data();  /* template data */
void file_read(FILE *fd, struct r_list *Vx);
void file_conv(struct r_list *indat, struct p_list *phead);
void INCwrite_wg(int APaddr, int loops, int a[], codeint x[]);
void Vwrite_wg(int which_wg, int instr_tag, codeint vloops,
               int a_const, int incr, codeint vmult);
void command_wg(int which_wg, int cmd_wg);
void write_wg(int which_wg, int where_on_wg, int what);
void pointgo_wg(int which_wg, int where_on_wg);
int wgtb(double tb, int itick, int n);
void put_chain();
static int get_chain();
int no_grad_wg(char gid);
int get_wg_apbase(char which);
int chgdeg2bX(double rphase);

/**********************************************
*  New definitions and declarations to allow  *
*  re-mapping of the waveform generators to   *
*  different RF transmitter boards.           *
**********************************************/

#define MINDELAY	0.2e-6	/* minimum delay for the ACB */
#define MINWGRES        0.05e-6 /* min res 50 ns             */

struct wfg_map
{
   double factor;
   int  hs_line;
   int  ap_baseaddr;
   int  on_off_state;
};
 
static struct wfg_map   wfgen[MAX_RFCHAN_NUM+1];
static int              waveboard[MAX_RFCHAN_NUM+1],
			pgdrunning[MAX_RFCHAN_NUM+1],
                        wfg_locked;

/*************************************************************
   bookkeeping functions 
   set path names and initialize 
   data structures
*************************************************************/
void init_wg_list()
{
    int i;

/*******************************************
*  Code to allow re-mapping of waveform    *
*  generators to different RF transmitter  *
*  devices.                                *
*******************************************/

    wfg_locked = FALSE;
    for (i = 0; i < (MAX_RFCHAN_NUM+1); i++)
    {
       waveboard[i] = i;
       pgdrunning[i] = FALSE;
    }
 

    if (ap_interface == 4)
    { /* start with 1; skip location 0 */
       for (i = 1; i <= MAX_RFCHAN_NUM; i++)
          wfgen[waveboard[i]].hs_line  = 1 <<
		   ( (i - 1)*N_HSLS_PER_CHANNEL + WFG_HSL_OFFSET );
    }
    else
    {
       wfgen[waveboard[TODEV]].hs_line  = WFG1;
       wfgen[waveboard[DODEV]].hs_line  = WFG2;
       wfgen[waveboard[DO2DEV]].hs_line = WFG3;
    }

    wfgen[waveboard[TODEV]].ap_baseaddr  = 0x0c10;
    wfgen[waveboard[DODEV]].ap_baseaddr  = 0x0c18;
    wfgen[waveboard[DO2DEV]].ap_baseaddr = 0x0c48;
    wfgen[waveboard[DO3DEV]].ap_baseaddr = 0x0c40;
    wfgen[waveboard[TODEV]].on_off_state  = 0;
    wfgen[waveboard[DODEV]].on_off_state  = 0;
    wfgen[waveboard[DO2DEV]].on_off_state = 0;
    wfgen[waveboard[DO3DEV]].on_off_state = 0;
    wfgen[waveboard[TODEV]].factor  = 1.0;
    wfgen[waveboard[DODEV]].factor  = 1.0;
    wfgen[waveboard[DO2DEV]].factor = 1.0;
    wfgen[waveboard[DO3DEV]].factor = 1.0;

/********
*  END  *
********/

    strcpy(filep3,systemdir); /* $vnmrsystem */
    strcat(filep3,"/acqqueue/ldcontrol");
    /*
      read the structure from environment
      for load control 
    */
    if (get_chain())
    {
      for (i=0; i < NWGS; i++)
      {
        Root[i].cur_tmpl = 0x0fff0;
        Root[i].cur_ib   = 0;
        Root[i].r__cntrl = 0;
	Root[i].ap_base = get_wg_apbase((char) i); 
        Root[i].p_base   = NULL;
        Root[i].ib_base  = NULL;
      }
    }
    raw_root.tname[0] = '\0';
    raw_root.index = 0;
    raw_root.rlink = NULL;
    raw_root.rdata = NULL;
    strcpy(fileRFpattern,filexpath); 
    strcat(fileRFpattern,".RF"); 
/************************************************************* 
    prepare the global file segment for writing
    file name is correct
**************************************************************/
    if (!checkflag)
    {
       global_fw = fopen(fileRFpattern,"w");
       if (global_fw == NULL) 
       {
         text_error("could not create RF segment");
         psg_abort(1);
       }
    }
}

static double get_wfg_scale(dev)
int dev;
{
   extern double rfchan_getpwrf();
   double val;

   if (ap_interface < 4)
	val = 4095.0;
   else
   {
	val = rfchan_getpwrf(dev);
	if (newacq)
	{
	   val = ((val/4095.0)*32767.0) + 0.5;
	   if (val > 32767.0) val = 32767.0;
	   if (val < 0.0) val = 0.0;
	}
   }
   return(val);
}

static void set_wfgen(int dev, int on_off, double scl)
{
   wfgen[waveboard[dev]].on_off_state  = on_off;
   wfgen[waveboard[dev]].factor  = scl;
   HSgate(wfgen[waveboard[dev]].hs_line, on_off);
}

int waveon(int dev, double *scale_val)
{
   *scale_val = 1.0;
   if ( (dev < 1) || (dev > NUMch) )
      return(0);
   else if ( is_y(rfwg[dev-1]) && wfgen[waveboard[dev]].on_off_state)
   {
      *scale_val = wfgen[waveboard[dev]].factor;
      return(1);
   }
   else
      return(0);
}

int getWfgAPaddr(int rfdevice)
{
   return( is_y(rfwg[rfdevice-1]) ? wfgen[waveboard[rfdevice]].ap_baseaddr
		: 0 );
}

void init_ecc()
{
   FILE *x_f1;
   char filename[MAXSTR];               /* eddy current filename */
   int j;
   codeint apouts;
   codeint apvals[32];

   strcpy(filename,systemdir);
   strcat(filename,"/acqqueue/eddy.out");
   x_f1 = fopen(filename,"r");
   if (x_f1 != NULL)
   {
     /*
     if it can be opened so send it
     */
     while ( (fread(&apouts,sizeof(codeint),1,x_f1) == 1) &&
        (apouts > 0)  && (apouts < 32) )
        {
          if (fread(&apvals[0],sizeof(codeint),(int) apouts+1,x_f1) == apouts+1)
          {
             delay( 1.0e-3);
             putcode(APBOUT);
             putcode((codeint) apouts-1);
             for (j=1; j<=apouts; j++)
                putcode((codeint) apvals[j]);
          }
        }
     fclose(x_f1);
     unlink(filename);
   }
}

/************************************************************* 
      wg_close cleans up the global file 
      should do sanity checks
**************************************************************/
void wg_close()
{   
  dump_data();
  put_chain();
}

/*************************************************************

   resolve the details for the named pattern by 
   a) finding them in the template tree
   b) causing them to be loaded 
   failure aborts psg

*************************************************************/
static struct p_list *resolve_pattern(hy,nn,tp,amp)
handle hy;
char *nn;
int tp;
double amp;
{
   struct p_list *T1;
   struct r_list *R1;
   if ((int)strlen(nn) < 1)  /* pattern name bad abort */
   {
       text_error("null pattern name ABORTING..");
       psg_abort(1);
   }
   /*
   hook into the correct tree 
   */
   if ((R1 = pat_file_found(nn,tp)) == NULL)
   {
      if ((R1 = read_pat_file(nn,tp)) == NULL)
      {
         abort_message("pattern %s was not found", nn);
      }
   }
   if ((T1 = pat_list_search(hy,R1,amp)) != NULL) 
     return((struct p_list *) T1);
   /* pattern was not already acquired so find it in file system */
   if ((T1 = pat_file_search(hy,R1,amp)) != NULL) 
     return((struct p_list *) T1);
   
    return((struct p_list *) NULL);
}

/*************************************************************
    see if template is in the tree
*************************************************************/
static struct p_list *pat_list_search(hertz,rhead,amp)
handle hertz;
struct r_list *rhead;
double amp;
{
   int pwrf;
   struct p_list *scan;

   if (rhead->suffix == WGRADIENT)
      pwrf = 32767;
   else
      pwrf = (int) (amp + 0.1);
   scan = hertz->p_base;
   while ((scan != NULL) &&
          ((scan->rindex != rhead->index) || (scan->scale_pwrf != pwrf)) )
   {
     scan = scan->plink;
   }
   return((struct p_list *) scan);
}

static struct r_list *pat_file_found(nn,tp)
char *nn;
int tp;
{
   struct r_list *scan;

   scan = raw_root.rlink;
   while ((scan != NULL) &&
          ((strncmp(scan->tname,nn,MAXSTR) != 0) || (tp != scan->suffix)) )
   {
     scan = scan->rlink;
   }
   return( (struct r_list *) scan );
}

static struct r_list *read_pat_file(nn,tp)
char *nn;
int tp;
{
   FILE *fd;
   struct r_list **scan;
   struct r_list *rhead;
   int last_index;

   fd = p__open(nn,tp); 
   if (fd == NULL)
      return(NULL);
   rhead = (struct r_list *) malloc(sizeof(struct r_list));
   strcpy(rhead->tname,nn);
   rhead->suffix = tp;
   rhead->rdata = NULL;
   rhead->rlink = NULL;
   file_read(fd,rhead);
   fclose(fd);
   scan = &(raw_root.rlink);
   last_index = 0;
   while (*scan != NULL)
   {
     last_index = (*scan)->index;
     scan = &((*scan)->rlink);

   }
   rhead->index = last_index + 1;
   *scan = rhead;
   return( (struct r_list *) rhead );
}

#ifdef DEBUG
disp_pat_line(rscan,cnt)
struct r_data *rscan;
int cnt;
{
  fprintf(stderr,"index= %d fields= %d f1= %g f2= %g f3= %g f4= %g f5= %g\n",
         cnt,rscan->nfields,rscan->f1,rscan->f2,rscan->f3,rscan->f4,rscan->f5); 
}

disp_pat(scan)
struct r_list *scan;
{
  struct r_data *rscan;
  int cnt;
  fprintf(stderr,"pat name= %s id= %d index= %d max= %g num= %d total= %g\n",
             scan->tname,scan->suffix,scan->index,scan->max_val,
             scan->nvals,scan->tot_val); 
   rscan = scan->rdata;
   cnt = 0;
   while (rscan != NULL)
   {
     cnt++;
     disp_pat_line(rscan,cnt);
     rscan = rscan->rnext;
   }
}

show_raw_pat()
{
   struct r_list *scan;

   scan = raw_root.rlink;
   while (scan != NULL)
   {
     disp_pat(scan);
     scan = scan->rlink;
   }
}
#endif

/*************************************************************
   find the pattern information and cause a load
*************************************************************/
static struct p_list *pat_file_search(hertz,rhead,amp)
handle hertz;
struct r_list *rhead;
double amp;
{ 
    /* we add to the linked list if successful */
    struct p_list *phead;

    phead = (struct p_list *) malloc(sizeof(struct p_list));
    phead->rindex = rhead->index;
    if (rhead->suffix == WGRADIENT)
       phead->scale_pwrf = 32767;
    else
    {
	if (newacq)
	   phead->scale_pwrf = 4095;
	else
	   phead->scale_pwrf = (int) (amp + 0.1);
    }
    phead->telm = rhead->nvals;
    file_conv(rhead,phead);
    /* file_read fills in telm, ticks, llist */
    /*
      LIFO lists are easier 
    */
    phead->plink = hertz->p_base;
    hertz->p_base = phead;
    hertz->cur_tmpl -= phead->telm;
    if (hertz->cur_tmpl < 0)
    {
       abort_message("WFG: pattern %s overruns memory .. ABORT",rhead->tname);
    }
    phead->wg_st_addr = hertz->cur_tmpl;
    if ((hertz->cur_tmpl - hertz->cur_ib) < 20 )
       text_error("WFG: almost out of memory!!\n");
    return((struct p_list *) phead);
}

static int secondword;
static int moreticks;
static int quarterbit_mask;
static int oldmask;
/* Inova only */

/* double word provisions */
/*************************************************************
	file is found, 
	read in and numbers of elements and ticks are 
	tallied, a side chain linked list is formed.
*************************************************************/
void file_conv(struct r_list *indat, struct p_list *phead)
{
    struct r_data *raw_data;
    int tick_cnt, cnt;
    int *lpntr;
    double scl = 0.0;
    char snarf[MAXSTR];
    int add_words;

    add_words = 0;
    secondword = 0;
    moreticks  = 0;
    tick_cnt = 0;
    if (P_getstring(CURRENT,"snarf",snarf,1,255))
    {
     snarf[0] = '\0';
    } 
    /* works for Inova's and UPLUS's */
    if ((snarf[0] != 'N') && (ap_interface > 3))
      {
      quarterbit_mask = 0x7ff;
      oldmask = 0x3ff;
      }
    else 
      {
      quarterbit_mask  = 0x7fe;
      oldmask = 0x7ff;
      }
    if (indat->suffix == WDECOUPLER)
       phead->data = (int *) malloc(2*phead->telm * sizeof(int));
    else
       phead->data = (int *) malloc((phead->telm+1) * sizeof(int));
    lpntr = phead->data;
    raw_data = indat->rdata;
    if (indat->suffix != WGRADIENT)
    {
/*
 *    This function only scales about 10 percent of the total power
 *    scl = (phead->scale_pwrf + 9.0*4095.0)/40950.0;
 */
/*
 *    This function only scales all of the total power
 */
      scl = phead->scale_pwrf/4095.0;
      scl *= 1023.0 / indat->max_val;
      phead->scl_factor = scl * indat->tot_val / (phead->telm * indat->max_val);
    }
    cnt = phead->telm;
    while (cnt--)
    {
       /* for a blocks worth of data */
	 secondword = read_in_pattern(raw_data,lpntr,&tick_cnt,indat->suffix,scl);
	 lpntr++;
	 if (secondword)
	 {
	   *lpntr = secondword;
	    lpntr++;
            tick_cnt += moreticks;
            add_words++;
	 }
         raw_data = raw_data->rnext;
    }
    if (indat->suffix == WRFPULSE)
    { /* preserves phase for Unity; preserves phase and
         amplitude for Unity+ */
       phead->telm += 1;
       *lpntr = (*(lpntr - 1)) & WFG_PULSE_FINAL_STATE;
    }
    phead->ticks= tick_cnt;
    phead->telm += add_words;
}

static double setdefault(tp,index)
int tp,index;
{
   double value = 0.0;

   if (tp == WRFPULSE)
   {
      if (index == 2)
         value = 1023.0;
      else if (index == 3)
         value = 1.0;
      else if (index == 4)
         value = 1.0;
   }
   else if (tp == WDECOUPLER)
   {
      if (index == 3)
         value = 1023.0;
      else if (index == 4)
         value = 0.0;
   }
   else  /* Gradient */
   {
      if (index == 2)
         value = 1.0;
      else if (index == 3)
         value = 0.0;
      else if (index == 4)
         value = 0.0;
   }
   return(value);
}

void file_read(FILE *fd, struct r_list *Vx)
{
    extern double setdefault();
    char parse_me[MAXSTR];
    float inf1,inf2,inf3,inf4,inf5;
    struct r_data **rlast,*rnew,*rtmp;
    int stat,maxticks=255;
    int eof_flag;

    Vx->nvals = 0;
    Vx->max_val = 0.0;
    Vx->tot_val = 0.0;
    rlast = &(Vx->rdata);
    do
    {
       do				/* until not a comment or blank*/
       {
         stat=getlineFIO(fd,parse_me,MAXSTR,&eof_flag);
       }
       while ( ((parse_me[0] == COMMENTCHAR) || (parse_me[0] == LINEFEEDCHAR)) && (stat != EOF) );

       if (stat != EOF) 
       {
           /* now it is not a comment so attempt to parse it */
          stat = sscanf(parse_me,"%f %f %f %f %f", &inf1, &inf2, &inf3, &inf4, &inf5);
          if ( (stat == 0) ||
               ((Vx->suffix == WDECOUPLER) && (stat == 1)) )
          {
               text_error("error in pattern file");
               psg_abort(1);
          }
          if ( (Vx->suffix == WRFPULSE) && (stat >= 3) && (inf3 < 0.5) )
          {
             /*
              * This is a line with zero duration.  Use its amplitude
              * for scaling only.
              */
              if (inf2 > Vx->max_val)
                   Vx->max_val = inf2;
          }
          else if ( (Vx->suffix == WDECOUPLER) && (inf1 < 0.5) )
          {
             /*
              * This is a line with zero duration.  Use its amplitude
              * for scaling only.
              */
              if ( (stat >= 3) && (inf3 > Vx->max_val) )
                   Vx->max_val = inf3;
          }
          else
          {
             Vx->nvals++;
             *rlast = rnew = (struct r_data *) malloc(sizeof(struct r_data));
             rnew->nfields = stat;
             rnew->f1 = inf1;
             rnew->f2 = (stat >= 2) ? inf2 : setdefault(Vx->suffix,2);
             rnew->f3 = (stat >= 3) ? inf3 : setdefault(Vx->suffix,3);
             rnew->f4 = (stat >= 4) ? inf4 : setdefault(Vx->suffix,4);
             rnew->f5 = 0.0;
             if (Vx->suffix == WGRADIENT)
             {
		/*------------------------------------------------------*/
		/* Code to duplicate lines that have more than 255 	*/
		/* ticks.						*/
		/*------------------------------------------------------*/
		while (rnew->f2 > maxticks)
		{
		   rtmp = rnew;
		   rlast = &(rnew->rnext);
		   Vx->nvals++;
		   *rlast=rnew=(struct r_data *)malloc(sizeof(struct r_data));
		   rnew->nfields = rtmp->nfields;
		   rnew->f1 = rtmp->f1;
		   rnew->f2 = rtmp->f2 - maxticks;
		   rnew->f3 = rtmp->f3;
		   rnew->f4 = rtmp->f4;
		   rnew->f5 = rtmp->f5;
		   rtmp->f2 = maxticks;
		}
	     }
             if (Vx->suffix == WRFPULSE)
             {
                inf2 = rnew->f2;
                if (inf2 > Vx->max_val)
                   Vx->max_val = inf2;
                Vx->tot_val += inf2;
             }
             else if (Vx->suffix == WDECOUPLER)
             {
                inf3 = rnew->f3;
                if (inf3 > Vx->max_val)
                   Vx->max_val = inf3;
                Vx->tot_val += inf3;
             }
             rnew->rnext = NULL;
             rlast = &(rnew->rnext);
          }
       }
    }
    while ((stat != EOF) && (eof_flag != EOF));
}

/************************************************************* 
    find and open for reading the pattern file
*************************************************************/
static FILE *p__open(nn,id)
char *nn;
int id;
{
    FILE *fd;
    char pulsepath[MAXSTR],tag[8];
    /*
      add type descriptor and 
      form path name to local pulse shape library 
      and absolute path name of template file 
    */
    tag[0] = '\0';
    switch (id)
    {
        case WRFPULSE:	
                           strcpy(tag,".RF");
                        break;
        case WGRADIENT: 
                           strcpy(tag,".GRD");
                        break;
        case WDECOUPLER: 
                           strcpy(tag,".DEC");
                        break;
    }
    fd = NULL;
    if (appdirFind(nn,"shapelib",pulsepath,tag,R_OK) )
       fd = fopen(pulsepath,"r");
    if (fd == NULL)
    {
       abort_message("can't find shape template %s\n",nn);
    }
    putFile(pulsepath);
    return(fd);
}

/************************************************************* 
   a file is read in and parsed
*************************************************************/

static int read_in_pattern(struct r_data *rdata, int *lpntr, int *ticker,
                           int tp, double scl)
{  
   float scl_amp;
   /* these are FLOATS */
   int if1,if2,if3,if4;
   int stat,idataf;
   int wordtwo;

   stat = rdata->nfields;
   *lpntr = 0;
   wordtwo = 0;
   moreticks = 0;
   if (tp == WGRADIENT)
   {
     /* f1 is amplitude, f2 is duration, f3 is "user" bits */
     if1 = ((int) rdata->f1) & 0x0ffff; /* 16 bit number */
     if2 = ((int) rdata->f2) & 0x0ff;   /*  8 bit number */
     if3 = ((int) rdata->f3) & 0x03;	/*  2 bit number */
     if ((stat == 1) || (if2 < 1))
        if2 = 1;			/* default to 1 */
     (*ticker) += if2;
     *lpntr = ((if3 << 30) | (if1 << 8) | if2); 
   }
   if (tp == WRFPULSE)
   {
     /* f1 is phase in degrees f2 is amplitude f3 is duration */
     if1 = chgdeg2bX(rdata->f1);       	    /* 11/10 bits phase */
    
     scl_amp = rdata->f2 * scl;
     if2 = ((int) scl_amp) & oldmask;     /* 10 bits amplitude plus hard line */
     if3 = ((int) rdata->f3) & 0x0ff;      /*  8 bits dwell time */
     if4 = ((int) rdata->f4) & 0x07;       /*  3 bits gate + 2 spares */
     /* no zero or negative time states */
     if (if3 < 1) 
       if3 = 1;
     if (stat == 1)
     {
        if2 = 0x3ff;
        if3 = 1;			 /* default to 1 */
     }
     if (stat == 2)
     {
        if3 = 1;			 /* default to 1 */
     }
     (*ticker) += if3;
     if (stat < 4)
     {
      if4 = 0x020000000;
     }
     else
     {
      if4 <<= 29;
     }
     *lpntr = ((if1 << 18) | (if2 << 8) | if3 | if4 ); 
   }
   if (tp == WDECOUPLER)
   {
   /* 
      first  field - tip angle is multiples of 90 degrees 
      second field - phase angle in degrees  -- default zero
      third  field - amplitude 0-1023        -- default 1023
      fourth field - transmitter gate 0-7    -- default on 1
   */
      if1 =  (int) (rdata->f1/dpf + 0.5);
      if (if1 > 255) 
      {
	  moreticks = if1 - 255;
	  if1 = 255;
      }
      (*ticker) += if1;
      if (stat < 2)
          if2 = 0;              /* phase zero */
      else
      {
         if2 = chgdeg2bX(rdata->f2);       	    /* 11/10 bits phase */
      }
      scl_amp = (stat < 3) ? 1023.0 : rdata->f3;
      scl_amp *= scl;
      if3 = (((int) scl_amp) & oldmask) << 8; /* 10 bits amplitude */
      if (stat < 4) 
          if4 = ( wtg_active ? (0x1) << 29 : 0 );
      else
          if4 = (((int) rdata->f4) & 0x07) << 29;
      idataf = (if2 << 18 | if3 | if4);
      *lpntr = (if1 | idataf);
      if (moreticks > 0)
         wordtwo = (idataf | moreticks);
   }
   return(wordtwo);
}

int chgdeg2bX(double rphase)
/************************************************************************
* chgdeg2bX does 0.25 degree resolution 
* it is probably only valid for U+/Inova
* 1/4 or 1/2 selection is done here 
* using quartermask_bit = 0x7ff or 0x7fe...
* It returns an integer value.
*************************************************************************/
{
 int iphase;
 if (ap_interface > 3)			/* rotational sense is opposite */
					/* for UNITYplus                */
    iphase = ((int) ((rphase + 0.125)*100.0)) % 36000;
 else
    iphase = ((int) ((-rphase + 0.125)*100.0)) % 36000;
 /* hundreths of degree scale convenient for debugging */
 while (iphase <    0) 
    iphase += 36000;
 while (iphase > 36000) 
    iphase -= 36000;
 iphase = ((iphase/9000)*512) + ((iphase % 9000)/25);
 iphase &= quarterbit_mask; 
 return(iphase);
}

int chgdeg2b(double rphase)
/************************************************************************
* chgdeg2b will change the input from degrees to bits.  It needs to
* make the change in accordance with the way the ram is set up.  The
* 9,8 msbs are the 180,90 deg lines. The 0-7 are 0.5 deg increments
* up to 89.5 deg which means 179 bits out of 255.
* It returns an integer value.
 *************************************************************************/
{
 int iphase;
 if (ap_interface > 3)                  /* rotational sense is opposite */
	   /* for UNITYplus                */
    iphase = ((int) ((rphase + 0.25)*10.0)) % 3600;
 else
    iphase = ((int) ((-rphase + 0.25)*10.0)) % 3600;
 while (iphase <    0)
   iphase += 3600;
 while (iphase > 3600)
   iphase -= 3600;
 iphase = ((iphase/900)*256) + ((iphase % 900)/5);
 return(iphase);
}

/*
   updated to handle last line w/o cr/lf 
*/

int getlineFIO(fd,targ,mxline,p2eflag)
FILE *fd;
char *targ;
int mxline;
int *p2eflag;
{   
   char *tt;
   int cnt,tmp;
   tt = targ;
   cnt = 0;
   do
   {  
    tmp = getc(fd);
    if (cnt < mxline)
      *tt++ = (char) tmp;    /* read but discard chars after mxline */
    cnt++;
   }
   while ((tmp != EOF) && (tmp != '\n'));
   if (cnt >= mxline)
      targ[mxline -1] = '\0';
   else 
      *tt = '\0';	/* ensure a null terminated list */
   if (tmp == EOF)
      *p2eflag = EOF; 
   else 
      *p2eflag = cnt;
   if ((tmp == EOF) && (cnt < 2)) /* normal file end */
     return(EOF);
   else
     return(cnt);
}

/*************************************************************
   load control 
   put_chain stores the templates 
   loaded 
*************************************************************/
void put_chain()
{
#ifdef PROTO
   FILE *fw;
   struct p_list *uu;
   int i,j;
   fw = fopen(filep3,"w");
   if (fw != NULL) 
   { 
      for (i = 0; i < NWGS; i++)
      {
        if ((j = fwrite(&(Root[i]),sizeof(struct pat_root),1,fw)) != 1)
        {
          text_error("load file write error on allocation info\n");
          psg_abort(1);
        }
        uu = Root[i].p_base;
        while (uu != NULL)
        {
          if ((j = fwrite(uu,sizeof(struct p_list),1,fw)) != 1)
          {
            text_error("load file write error on plist\n");
            psg_abort(1);
          }
          uu = uu->plink;
        }
      }
      fclose(fw);
      chmod(filep3,00666);
   }
#endif
}

static int get_chain()
{
   return(1);
#ifdef PROTO
   FILE *fr;
   struct p_list *uu,**vv;
   int i,j;
   fr = fopen(filep3,"r");
   if (fr == NULL) 
     return(1);
   for (i = 0; i < NWGS; i++)
   {
     if ((j = fread(&(Root[i]),sizeof(struct pat_root),1,fr)) != 1)
     {
       text_error("load file error read on root %d\n",j);
       psg_abort(1);
     }
     Root[i].cur_ib = 0;
     Root[i].ib_base  = NULL;
     /* we don't save instruction trees */
     vv = &(Root[i].p_base);
     while (*vv != NULL)
     {
       uu = (struct p_list *) malloc(sizeof(struct p_list));
       if ((j = fread(uu,sizeof(struct p_list),1,fr)) != 1)
       {
         text_error("load file read error on plist  %d\n",j);
         psg_abort(1);
       }
       /* the link "plink" is erroneous but we use non-NULL as a continue flag */
       uu->status=1;
       /* we ground the data link and flag as already read in */
       *vv = uu;
       vv = &(uu->plink);
     }
   }
   fclose(fr);
   return(0);
#endif
}
/*****************************************
    dump_ib()/4
    adds the ib the the global file
*****************************************/
int dump_ib(apb,dest,arr,numb)
int apb,
    dest,
    numb;
int *arr;
{   
    struct gwh ibh;
#ifdef LINUX
    int *ptr;
    int cnt;
#endif

    if (global_fw == NULL) 
    {
       if (checkflag)
          return(1);
       text_error("no instruction block file ");
       psg_abort(1);
    }
    ibh.n_pts     = htons( (short) numb );
    ibh.wg_st_addr= htons( (short) dest );
    ibh.ap_addr   = htons( (short)  apb );
    ibh.gspare    = htons( (short) 0xabcd );
    if ((fwrite(&ibh,sizeof(ibh),1,global_fw)) != 1)
    {
      text_error("instruction block write failed");
      psg_abort(1);
    }
#ifdef LINUX
    ptr = arr;
    for (cnt=0; cnt < numb; cnt++)
    {
       *ptr = htonl( *ptr );
       ptr++;
    }
#endif
    if (fwrite(arr,sizeof(int),numb,global_fw) != numb)
    {
      text_error("instruction block write failed");
      psg_abort(1);
    }
    /* valid data in .RF file */
    d_in_RF = 1;
    return(1);
}

void dump_data()   /* template data */
{
   struct p_list *uu;
   struct gwh glblh;
   int i,j,terminator;
#ifdef LINUX
   int cnt;
   int *ptr;
#endif

   terminator = htonl(0xa5b6c7d8);
   if (global_fw == NULL) 
   {
     if (checkflag)
       return;
     text_error("pattern data write failed\n");
     psg_abort(1);
   }
   for (i = 0; i < NWGS; i++)
   {
     uu = Root[i].p_base;
     while (uu != NULL)
     {
       if (uu->data != NULL)
       { /* there is data to send */
         glblh.n_pts = htons( (short) uu->telm);
         glblh.wg_st_addr = htons( (short) uu->wg_st_addr);
         glblh.gspare    = htons( (short) 0xfedc);
         glblh.ap_addr = htons( (short) get_wg_apbase((char) i));
	 /*
	 printf("glbh = %x  %x   %x \n",glblh.ap_addr,glblh.wg_st_addr,glblh.n_pts);
	 */
         if ((j= fwrite(&glblh,sizeof(glblh),1,global_fw)) != 1)
         {
	   text_error("error on dump file %d",j);
	   psg_abort(1);
         }
#ifdef LINUX
         ptr = uu->data;
         for (cnt=0; cnt < uu->telm; cnt++)
         {
            *ptr = htonl( *ptr );
            ptr++;
         }
#endif
         if ((j = fwrite(uu->data,sizeof(int),uu->telm,global_fw)) != uu->telm)
         {
           text_error("load file write error on plist  %d\n",j);
           psg_abort(1);
         }
	 d_in_RF = 1;   /* valid data in .RF file */
       }
       uu = uu->plink; /* next link */
     }
   }
   /* write of the terminator and close go together */
   if ((j = fwrite(&terminator,sizeof(int),1,global_fw)) != 1)
   {
      text_error("global RF load file write error on terminator\n");
      psg_abort(1);
   }
   fclose(global_fw);
/**********************************************
test to see in file has any data - if not 
remove it and send nothing - fileRFpattern = '\0'
**********************************************/
    if (d_in_RF == 0)
    {
      unlink(fileRFpattern);
      strcpy(fileRFpattern,"\0");
    }
}

/***************************************************
advanced mode support -  almost completely untested!
needs array and 2d reset support and C loop support
initial modes only to shaped_pulse!!!
***************************************************/
int setWGmode(which,what)
char which;
int  what;
{
   int cmd,dib[2];
   handle h1;
   dib[0] = pat_end;
   dib[1] = pat_end;
   /* a distinctive maker */
   cmd = 0;
   h1 = get_handle(which); /* hook up to the correct list */
   if (FALLTHRU & what)
   {
      point_wg(h1->ap_base,h1->cur_ib);
      cmd = 1;
   }
   if (ALLWGMODE & what)
      cmd |= 4;
   /* 
     at least for now - we don't match this stop word !
   */
   if (!(FALLTHRU & what))
   {
     dump_ib(h1->ap_base,h1->cur_ib,dib,2);
     h1->cur_ib += 2;
   }
   h1->r__cntrl = what;
   command_wg(h1->ap_base,(cmd | 0x20)); /* Assume for rf */
   return(h1->cur_ib);
}
/***************************************************************
		user interface and instruction block control
   the linked list structure here contains only internal data
   rf phase cycles right 
   elements are sized and stored correctly
   2D gradients in work for amplitude
   2D dynamic gradients nyi
   2D rf nyi
*/

/********************************
*  Section for Pulse functions  *
********************************/


/*-----------------------------------------------
|                                               |
|               genshaped_pulse()/8             |
|                                               |
+----------------------------------------------*/
void genshaped_pulse(char *name, double width, codeint phase,
                     double rx1, double rx2, double g1,
                     double g2, int rfdevice)
{          
   char			wfg_label;
   int                  temp;
   handle               h1;
   struct ib_list       *sldr;
   extern void		setHSLdelay();
   extern double	get_wfg_scale();
   double		amp_factor;
 
 
   if (width <  MINWGTIME)
   {
      delay(rx1 + rx2 + WFG_START_DELAY + WFG_STOP_DELAY);
      return;
   }
 
   if ( (rfdevice < 1) || (rfdevice > NUMch) )
   {
      abort_message("shaped_pulse: invalid RF channel %d",
		rfdevice);
   }

   if (!is_y(rfwg[rfdevice-1]))
   {
      G_apshaped_pulse(name,width,phase,rx1,rx2,rfdevice);
      return;
   }
 
   wfg_locked = TRUE;

   switch (rfdevice)
   {
      case TODEV:	wfg_label = 'o'; break;
      case DODEV:	wfg_label = 'd'; break;
      case DO2DEV:	wfg_label = 'e'; break;
      case DO3DEV:	wfg_label = 'f'; break;
      default:		wfg_label = 'o'; break;
   }
 
   h1 = get_handle(wfg_label);
   temp = h1->ap_base;
   amp_factor = get_wfg_scale(rfdevice);

/**********************************************
*  phase I - produce or get ib start address  *
**********************************************/
 
   if ( (sldr = resolve_ib(h1, name, width, amp_factor, 1, 0)) == NULL )
   {
      if ( (sldr = create_pulse_block(h1, name, width, g1, g2,
                     amp_factor )) == NULL )
      {
         text_error("genshaped_pulse: no ib\n");
         psg_abort(1);
      }
   }
          
/*********************************************
*  The block has joined the global file.     *
*  phase II - point to ib and fire: 10.2 us  *
*********************************************/
 
   if ((h1->r__cntrl & FALLTHRU) == 0) 
   {
      setHSLdelay();
      if (newacq)
      {
      	pointgo_wg(temp, sldr->apstaddr);
	G_Delay(DELAY_TIME, INOVA_MINWGSETUP, 0); /* wfg overhead time */
      }
      else
      {
      	point_wg(temp, sldr->apstaddr);
      	command_wg(temp, 0x25);
      }
   }
   else
   {
      fprintf(stderr, "test level one:  rfdevice = %d\n", rfdevice);
   }
 
/***************************
*  phase III - coordinate  *
***************************/
 
   SetRFChanAttr(RF_Channel[rfdevice], SET_RTPHASE90, phase, NULL);

   if (ap_interface < 4)
   {
      HSgate(rcvr_hs_bit, TRUE);
      if (rfdevice == DODEV)
	sisdecblank(OFF);
   }
   else
   {
      if (newacq)
	  HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
      else
	  SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, OFF, 0);
      SetRFChanAttr(RF_Channel[rfdevice], SET_RCVRGATE, OFF, 0);
   }

   G_Delay(DELAY_TIME, rx1-g1, 0);
   set_wfgen(rfdevice, TRUE, sldr->factor);
   G_Delay(DELAY_TIME, WFG_OFFSET_DELAY, 0);

#ifdef MOD_XMTRGATE
   SetRFChanAttr(RF_Channel[rfdevice], SET_XMTRGATE, ON, NULL);
#endif

   G_Delay(DELAY_TIME, g1 + width + g2, 0);

#ifdef MOD_XMTRGATE
   SetRFChanAttr(RF_Channel[rfdevice], SET_XMTRGATE, OFF, NULL);
#endif

   set_wfgen(rfdevice, FALSE, sldr->factor);
   G_Delay(DELAY_TIME, rx2-g2, 0);

   if (ap_interface < 4)
   {
      if (!rcvroff_flag)
         HSgate(rcvr_hs_bit, FALSE);
      if (rfdevice == DODEV)
	sisdecblank(ON);
   }
   else
   {
      if (newacq) {
	 if (!rcvroff_flag)		/* turn receiver back on only if */
	   HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
      }
      else {
         if ( isBlankOn(OBSch) )
           SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, ON, 0);
      }
      if ( isBlankOn(rfdevice) )
         SetRFChanAttr(RF_Channel[rfdevice], SET_RCVRGATE, ON, 0);
   }
   if (newacq)
   {
	G_Delay(DELAY_TIME, INOVA_MINWGCMPLT, 0);
   }

   if ((h1->r__cntrl & ALLWGMODE) == 0)
   {
      setHSLdelay();
      if (ap_interface < 4)
         command_wg(temp, 0x20);  
   }
   else
   {
      fprintf(stderr, "test level two:  rfdevice = %d\n", rfdevice);
   }
}

/*-----------------------------------------------
|                                               |
|               genshaped_rtamppulse()/8        |
|                                               |
+----------------------------------------------*/
void genshaped_rtamppulse(name, width, tpwrfrt, phase, rx1, rx2, g1,
                        g2, rfdevice)
char    *name;
codeint phase;
codeint tpwrfrt;
int     rfdevice;
double  width,
        rx1,
        rx2,
        g1,
        g2;
{          
   char			wfg_label;
   int                  temp;
   handle               h1;
   struct ib_list       *sldr;
   extern void		setHSLdelay();
   extern double	get_wfg_scale();
   double		amp_factor;
 
 
   if (width <  MINWGTIME)
   {
      delay(rx1 + rx2 + WFG_START_DELAY + WFG_STOP_DELAY);
      return;
   }
 
   if ( (rfdevice < 1) || (rfdevice > NUMch) )
   {
      abort_message("shaped_pulse: invalid RF channel %d",
		rfdevice);
   }

   if (!is_y(rfwg[rfdevice-1]))
   {
      G_apshaped_pulse(name,width,phase,rx1,rx2,rfdevice);
      return;
   }
 
   wfg_locked = TRUE;

   switch (rfdevice)
   {
      case TODEV:	wfg_label = 'o'; break;
      case DODEV:	wfg_label = 'd'; break;
      case DO2DEV:	wfg_label = 'e'; break;
      case DO3DEV:	wfg_label = 'f'; break;
      default:		wfg_label = 'o'; break;
   }
 
   h1 = get_handle(wfg_label);
   temp = h1->ap_base;
   amp_factor = get_wfg_scale(rfdevice);

/**********************************************
*  phase I - produce or get ib start address  *
**********************************************/
 
   if ( (sldr = resolve_ib(h1, name, width, amp_factor, 1, 0)) == NULL )
   {
      if ( (sldr = create_pulse_block(h1, name, width, g1, g2,
                     amp_factor )) == NULL )
      {
         text_error("genshaped_pulse: no ib\n");
         psg_abort(1);
      }
   }
          
/*********************************************
*  The block has joined the global file.     *
*  phase II - point to ib and fire: 10.2 us  *
*********************************************/
 
   if ((h1->r__cntrl & FALLTHRU) == 0) 
   {
      setHSLdelay();
      if (newacq)
      {
        /* 
         *  overwrite to place the correct amp
         *  Note that the calculation will be amp = 0 + (1*tpwrfrt)
         */
        point_wg(temp,sldr->apstaddr+2);
        Vwrite_wg(temp+1, pat_scale(1,(int) amp_factor), one, 0, 1, tpwrfrt);
      	pointgo_wg(temp, sldr->apstaddr);
	G_Delay(DELAY_TIME, INOVA_MINWGSETUP, 0); /* wfg overhead time */
      }
      else
      {
      	point_wg(temp, sldr->apstaddr);
      	command_wg(temp, 0x25);
      }
   }
   else
   {
      fprintf(stderr, "test level one:  rfdevice = %d\n", rfdevice);
   }
   /***************************
   *  phase III - coordinate  *
   ***************************/
 
   SetRFChanAttr(RF_Channel[rfdevice], SET_RTPHASE90, phase, NULL);

   if (ap_interface < 4)
   {
      HSgate(rcvr_hs_bit, TRUE);
      if (rfdevice == DODEV)
	sisdecblank(OFF);
   }
   else
   {
      if (newacq)
	  HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
      else
	  SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, OFF, 0);
      SetRFChanAttr(RF_Channel[rfdevice], SET_RCVRGATE, OFF, 0);
   }

   G_Delay(DELAY_TIME, rx1-g1, 0);
   set_wfgen(rfdevice, TRUE, sldr->factor);
   G_Delay(DELAY_TIME, WFG_OFFSET_DELAY, 0);

#ifdef MOD_XMTRGATE
   SetRFChanAttr(RF_Channel[rfdevice], SET_XMTRGATE, ON, NULL);
#endif

   G_Delay(DELAY_TIME, g1 + width + g2, 0);

#ifdef MOD_XMTRGATE
   SetRFChanAttr(RF_Channel[rfdevice], SET_XMTRGATE, OFF, NULL);
#endif

   set_wfgen(rfdevice, FALSE, sldr->factor);
   G_Delay(DELAY_TIME, rx2-g2, 0);

   if (ap_interface < 4)
   {
      if (!rcvroff_flag)
         HSgate(rcvr_hs_bit, FALSE);
      if (rfdevice == DODEV)
	sisdecblank(ON);
   }
   else
   {
      if (newacq) {
	 if (!rcvroff_flag)		/* turn receiver back on only if */
	   HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
      }
      else {
         if ( isBlankOn(OBSch) )
           SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, ON, 0);
      }
      if ( isBlankOn(rfdevice) )
         SetRFChanAttr(RF_Channel[rfdevice], SET_RCVRGATE, ON, 0);
   }
   if (newacq)
   {
	G_Delay(DELAY_TIME, INOVA_MINWGCMPLT, 0);
   }

   if ((h1->r__cntrl & ALLWGMODE) == 0)
   {
      setHSLdelay();
      if (ap_interface < 4)
         command_wg(temp, 0x20);  
   }
   else
   {
      fprintf(stderr, "test level two:  rfdevice = %d\n", rfdevice);
   }
}


/*-----------------------------------------------
|                                               |
|             gensim2shaped_pulse()/12          |
|                                               |
+----------------------------------------------*/
gensim2shaped_pulse(name1, name2, width1, width2, phase1,
                   phase2, rx1, rx2, g1, g2, rfdevice1,
                   rfdevice2)
char    *name1,
        *name2;
codeint phase1,
        phase2;
int     rfdevice1,
        rfdevice2;
double  width1,
        width2,
        rx1,
        rx2,
        g1,
        g2;
{          
   char 		wfg_label[2];
   int                  i,
                        tmp_rfdevice,
                        temp1,
                        temp2,
                        ok,
                        devshort,
                        devlong,
			exec_p1 = FALSE,
			exec_p2 = FALSE;
   double               pwshort,
                        pwlong,
                        factorshort,
                        factorlong,
                        centertime;
   handle               h1,
                        h2;
   struct ib_list       *sldr1,
			*sldr2;
   extern void		setHSLdelay();
   extern double	get_wfg_scale();
   double		amp_factor;

                                
/***************************************
*  Check for valid RF device entries.  *
***************************************/

   ok = (  ((rfdevice1 > 0) || (rfdevice1 <= NUMch))
        && ((rfdevice2 > 0) || (rfdevice2 <= NUMch))
        && (rfdevice1 != rfdevice2) );
   if (!ok)
   {
      abort_message("gensim2shaped_pulse(): invalid combination of RF channels %d, %d\n",
		rfdevice1, rfdevice2);
   }

   if ( (RF_Channel[rfdevice1] == NULL) ||
        (RF_Channel[rfdevice2] == NULL) )
   {
      text_error("sim2shaped_pulse():  both RF channels are not present\n");
      psg_abort(1);
   }

/***************************************
*  Return if no pulse time is greater  *
*  than the minimum.  A 28 us delay    *
*  is done for determinism.            *
***************************************/
 
   if ( (width1 <  MINWGTIME) &&
	(width2 < MINWGTIME) )
   {
      delay( rx1 + rx2 + WFG2_START_DELAY + WFG2_STOP_DELAY );
      return;
   }

/***********************************
*  Check for the existence of the  *
*  two WFG's.                      *
***********************************/

   if ((!is_y(rfwg[rfdevice1-1])) || (!is_y(rfwg[rfdevice2-1])))
   {
      abort_message("gensim2shaped_pulse(): no waveform generator on channels %d and/or %d\n",
		rfdevice1, rfdevice2);
   }

/***********************************
*  Check for invalid pulse times.  *
***********************************/

   if ( (width1 < 0.0) || (width2 < 0.0) )
   {
      if (ix == 1)
      {
         text_error(
	    "\ngensim2shaped_puslse():  improper pulse value set to zero.\n");
         text_error("A negative pulse width is not executable.\n");
      }

      width1 = ( (width1 < 0.0) ? 0.0 : width1 );
      width2 = ( (width2 < 0.0) ? 0.0 : width2 );
   }
   else if ( ((width1 > 0.0) && (width1 < MINWGTIME)) ||
             ((width2 > 0.0) && (width2 < MINWGTIME)) )
   {
      if (ix == 1)
      {
         text_error(
	    "\ngensim2shaped_puslse():  improper pulse value set to zero.\n");
         text_error("A non-zero pulse width < 0.2 us is not executable!\n");
      }

      width1 = ( ((width1 > 0.0) && (width1 < MINWGTIME)) ? 0.0 : width1 );
      width2 = ( ((width2 > 0.0) && (width2 < MINWGTIME)) ? 0.0 : width2 );
   }

/************************************
*  Lock down WFG mapping and setup  *
*  WFG handles.                     *
************************************/

   wfg_locked = TRUE;
   tmp_rfdevice = rfdevice1;

   for (i = 0; i < 2; i++)
   {
      switch (tmp_rfdevice)
      {
         case TODEV:	wfg_label[i] = 'o'; break;
         case DODEV:	wfg_label[i] = 'd'; break;
         case DO2DEV:	wfg_label[i] = 'e'; break;
         case DO3DEV:	wfg_label[i] = 'f'; break;
         default:	break;
      }
 
      tmp_rfdevice = rfdevice2;
   }
 
   h1 = get_handle(wfg_label[0]);
   temp1 = h1->ap_base;
   h2 = get_handle(wfg_label[1]);
   temp2 = h2->ap_base;
 
/************************************************
*  phase I - produce or get ib start addresses  *
************************************************/

   if (width1 >= MINWGTIME)
   {
      amp_factor = get_wfg_scale(rfdevice1);
      exec_p1 = TRUE;
      if ( (sldr1 = resolve_ib(h1, name1, width1, amp_factor, 1, 0)) == NULL )
      {
         if ( (sldr1 = create_pulse_block(h1, name1, width1, g1, g2,
                         amp_factor)) == NULL )
         {
            text_error("sim2shaped_pulse(): no ib for 1st pulse\n");
            psg_abort(1);
         }
      }
   }
 
   if (width2 >= MINWGTIME) 
   {
      amp_factor = get_wfg_scale(rfdevice2);
      exec_p2 = TRUE;
      if ( (sldr2 = resolve_ib(h2, name2, width2, amp_factor, 1, 0)) == NULL )
      {
         if ( (sldr2 = create_pulse_block(h2, name2, width2, g1, g2,
                         amp_factor)) == NULL )
         {
            text_error("sim2shaped_pulse(): no ib for 2nd pulse\n");
            psg_abort(1);
         }
      }
   }
   
 
/***********************************************
*  The blocks have joined the global file.     *
*  phase II - point to ib's and fire: 20.2 us  *
***********************************************/

   setHSLdelay();	/* allows HS lines to be reset if necessary */

   if (exec_p1)
   {
      if (newacq)
      {
      	pointgo_wg(temp1, sldr1->apstaddr);
        G_Delay(DELAY_TIME, INOVA_MINWGSETUP, 0);
      }
      else
      {
      	point_wg(temp1, sldr1->apstaddr);
      	command_wg(temp1, 0x25);
      }
   }
   else
   {
      delay(WFG_START_DELAY - WFG_OFFSET_DELAY);
   }

   if (exec_p2)
   {
      if (newacq)
      {
      	pointgo_wg(temp2, sldr2->apstaddr);
        G_Delay(DELAY_TIME, INOVA_MINWGSETUP, 0);
		/* CHANGED */
      }
      else
      {
      	point_wg(temp2, sldr2->apstaddr);
      	command_wg(temp2, 0x25);
      }
   }
   else
   { 
      delay(WFG_START_DELAY - WFG_OFFSET_DELAY); 
   }
/*
   if (newacq)
	G_Delay(DELAY_TIME, INOVA_MINWGSETUP, 0); 
*/


/*************************
*  Order pulses/devices  *
*************************/
 
   if ( (fabs(width1 - width2) <= (2 * MINDELAY)) &&
             (fabs(width1 - width2) > 0.0) )
   {
      if (width1 > width2)
      {
         width1 += 2.1 * MINDELAY;
      }
      else if (width2 > width1)
      {
         width2 += 2.1 * MINDELAY;
      }
   }
 
   if (width2 > width1)
   {
      pwshort = width1;
      pwlong = width2;
      devshort = rfdevice1;
      devlong = rfdevice2;
      factorshort = (exec_p1) ? sldr1->factor : 1.0;
      factorlong = (exec_p2) ? sldr2->factor : 1.0;
   }
   else
   {
      pwshort = width2;
      pwlong = width1;
      devshort = rfdevice2;
      devlong = rfdevice1;
      factorshort = (exec_p2) ? sldr2->factor : 1.0;
      factorlong = (exec_p1) ? sldr1->factor : 1.0;
   }
 
/***************************
*  phase III - coordinate  *
***************************/

   centertime = (pwlong - pwshort) / 2.0;
 
   SetRFChanAttr(RF_Channel[rfdevice1], SET_RTPHASE90, phase1, NULL);
   SetRFChanAttr(RF_Channel[rfdevice2], SET_RTPHASE90, phase2, NULL);

   if (ap_interface < 4)
   {
      sisdecblank(OFF);
      HSgate(rcvr_hs_bit, TRUE);
   }
   else
   {
      if (newacq)
	  HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
      else
	  SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, OFF, 0);
      SetRFChanAttr(RF_Channel[rfdevice1], SET_RCVRGATE, OFF, 0);
      SetRFChanAttr(RF_Channel[rfdevice2], SET_RCVRGATE, OFF, 0);
   }

   delayer(rx1-g1, FALSE);

#ifdef MOD_XMTRGATE
   SetRFChanAttr(RF_Channel[devlong], SET_XMTRGATE, ON, NULL);
   SetRFChanAttr(RF_Channel[devshort], SET_XMTRGATE, ON, NULL);
#endif
 
   set_wfgen(devlong, TRUE, factorlong);
   delayer(centertime+g1, FALSE);
   set_wfgen(devshort, TRUE, factorshort);
   delayer(pwshort+WFG2_OFFSET_DELAY, FALSE);
   set_wfgen(devshort, FALSE, factorshort);
   delayer(centertime+g2, FALSE);
   set_wfgen(devlong, FALSE, factorlong);

#ifdef MOD_XMTRGATE
   SetRFChanAttr(RF_Channel[devlong], SET_XMTRGATE, OFF, NULL);
   SetRFChanAttr(RF_Channel[devshort], SET_XMTRGATE, OFF, NULL);
#endif
 
   delayer(rx2-g2, FALSE);
   if (ap_interface < 4)
   {
      if (!rcvroff_flag)
         HSgate(rcvr_hs_bit, FALSE);
      sisdecblank(ON);
   }
   else
   {
      if (newacq) {
	 if (!rcvroff_flag)		/* turn receiver back on only if */
	   HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
      }
      else {
         if ( isBlankOn(OBSch) )
           SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, ON, 0);
      }
      if ( isBlankOn(rfdevice1) )
         SetRFChanAttr(RF_Channel[rfdevice1], SET_RCVRGATE, ON, 0);
      if ( isBlankOn(rfdevice2) )
         SetRFChanAttr(RF_Channel[rfdevice2], SET_RCVRGATE, ON, 0);
   }

 
/****************
*  reset WFG's  *
****************/

   setHSLdelay();

   if (newacq)
   {
      G_Delay(DELAY_TIME, INOVA_MINWGCMPLT, 0);
   }
   else
   {
      ( exec_p1 && (ap_interface < 4) ? command_wg(temp1, 0x20) :
		   delay(WFG_STOP_DELAY) );
      ( exec_p2 && (ap_interface < 4) ? command_wg(temp2, 0x20) :
		   delay(WFG_STOP_DELAY) );
   }
}


/*-----------------------------------------------
|                                               |
|            gensim3shaped_pulse()/16           |
|                                               |
+----------------------------------------------*/
void gensim3shaped_pulse(char *name1, char *name2, char *name3,
                    double width1, double width2, double width3,
                    codeint phase1, codeint phase2, codeint phase3,
                    double rx1, double rx2, double g1, double g2,
                    int rfdevice1, int rfdevice2, int rfdevice3)
{          
   char         	wfg_label[3];
   int          	i,
			tmp_rfdevice,
                	devshort, enddevshort,
                	devmed, enddevmed,
                	devlong, enddevlong,
			devtmp,
			temp1 = 0,
			temp2 = 0,
			temp3,
			nsoftpul,
                	execshort, endexecshort,
                	execmed, endexecmed,
                	execlong, endexeclong,
			exectmp,
			exec_p1 = NO_PULSE,
			exec_p2 = NO_PULSE,
			exec_p3 = NO_PULSE;
   double       	centertime1,  /* start center longest two pulses  */
                	centertime2,  /* start center shortest two pulses */
                	centertime3,  /* end center shortest two pulses */
                	centertime4,  /* end center longest two pulses */
                        factorshort, endfactorshort,
                        factormed, endfactormed,
                        factorlong, endfactorlong,
			factortmp,
                	pwshort,
                	pwmed,
                	pwlong;
   handle		h1,
			h2,
			h3;
   struct ib_list	*sldr1,
                        *sldr2,
			*sldr3;
   extern double	get_wfg_scale();
   double		amp_factor;
 
 
/***************************************
*  Check for valid RF device entries.  *
***************************************/

   if ( (rfdevice1 == rfdevice2) || (rfdevice1 == rfdevice3) ||
                (rfdevice2 == rfdevice3) )
   {
      text_error(
	  "gensim3shaped_pulse():  all 3 RF devices must be different\n");
      psg_abort(1);
   }

/***************************************
*  Return if no pulse time is greater  *
*  than the minimum.  A 42 us delay    *
*  is done for determinism.            *
***************************************/

   if ( (width1 < MINWGTIME) &&
	(width2 < MINWGTIME) &&
	(width3 < MINWGTIME) )
   {
      delay( rx1 + rx2 + WFG3_START_DELAY + WFG3_STOP_DELAY );
      return;
   }
 
/***********************************
*  Check for invalid pulse times.  *
***********************************/

   if ( (width1 < 0.0) || (width2 < 0.0) || (width3 < 0.0) )
   {
      if (ix == 1)
      {
         text_error(
	    "\ngensim3shaped_pulse():  improper pulse value set to zero.\n");
         text_error("A negative pulse width is not executable.\n");
      }

      width1 = ( (width1 < 0.0) ? 0.0 : width1 );
      width2 = ( (width2 < 0.0) ? 0.0 : width2 );
      width3 = ( (width3 < 0.0) ? 0.0 : width3 );
   }
   else if ( ((width1 > 0.0) && (width1 < MINWGTIME)) ||
             ((width2 > 0.0) && (width2 < MINWGTIME)) ||
             ((width3 > 0.0) && (width3 < MINWGTIME)) )
   {
      if (ix == 1)
      {
         text_error(
	    "\ngensim3shaped_pulse():  improper pulse value set to zero.\n");
         text_error("A non-zero pulse width < 0.2 us is not executable!\n");
      }

      width1 = ( ((width1 > 0.0) && (width1 < MINWGTIME)) ? 0.0 : width1 );
      width2 = ( ((width2 > 0.0) && (width2 < MINWGTIME)) ? 0.0 : width2 );
      width3 = ( ((width3 > 0.0) && (width3 < MINWGTIME)) ? 0.0 : width3 );
   }

/************************************
*  Lock down WFG mapping and setup  *
*  WFG handles.                     *
************************************/

   wfg_locked = TRUE;
   tmp_rfdevice = rfdevice1;
 
   for (i = 0; i < 3; i++)
   {
      switch (tmp_rfdevice)
      {
         case TODEV:	wfg_label[i] = 'o'; break;
         case DODEV:	wfg_label[i] = 'd'; break;
         case DO2DEV:	wfg_label[i] = 'e'; break;
         case DO3DEV:	wfg_label[i] = 'f'; break;
         default:	break;
      }

      tmp_rfdevice = ( (i == 0) ? rfdevice2 : rfdevice3 );
   }

/************************************************
*  phase I - produce or get ib start addresses  *
************************************************/

   if (width1 >= MINWGTIME)
   {
      if ( (rfdevice1 < 1) || (rfdevice1 > NUMch))
      {
	 abort_message("gensim3shaped_pulse():  RF device %d not within bounds 1 - %d\n",
			rfdevice1,NUMch);
      }
      if ( (RF_Channel[rfdevice1] == NULL) )
      {
         abort_message("gensim3shaped_pulse(): RF Channel device %d not present\n",
			rfdevice1);
      }
      if (strlen(name1))
      {
         if (!is_y(rfwg[rfdevice1-1]))
         {
            text_error("sim3shaped_pulse(): channel %d must have a WFG\n",
                        rfdevice1);
            psg_abort(1);
         }
         exec_p1 = SOFT_PULSE;
         h1 = get_handle(wfg_label[0]);
         temp1 = h1->ap_base;
	 amp_factor = get_wfg_scale(rfdevice1);
         if ( (sldr1 = resolve_ib(h1, name1, width1, amp_factor, 1, 0)) == NULL)
         {
            if ((sldr1 = create_pulse_block(h1, name1, width1, g1, g2,
				amp_factor)) == NULL)
            {
               text_error("gensim3shaped_pulse(): no ib for 1st pulse\n");
               psg_abort(1);
            }
         }
      }
      else
         exec_p1 = HARD_PULSE;
   }
 
   if (width2 >= MINWGTIME)
   {
      if ( (rfdevice2 < 1) || (rfdevice2 > NUMch))
      {
         abort_message("gensim3shaped_pulse():  RF device %d not within bounds 1 - %d\n",
			rfdevice2,NUMch);
      }
      if ( (RF_Channel[rfdevice2] == NULL) )
      {
         abort_message("gensim3shaped_pulse(): RF Channel device %d not present\n",
			rfdevice2);
      }
      if (strlen(name2))
      {
         if (!is_y(rfwg[rfdevice2-1]))
         {
            text_error("sim3shaped_pulse(): channel %d must have a WFG\n",
                        rfdevice2);
            psg_abort(1);
         }
         exec_p2 = SOFT_PULSE;
         h2 = get_handle(wfg_label[1]);
         temp2 = h2->ap_base;
	 amp_factor = get_wfg_scale(rfdevice2);
         if ( (sldr2 = resolve_ib(h2, name2, width2, amp_factor, 1, 0)) 
								== NULL )
         {
            if ((sldr2 = create_pulse_block(h2, name2, width2, g1, g2,
				amp_factor)) == NULL)
            {
               text_error("gensim3shaped_pulse(): no ib for 2nd pulse\n");
               psg_abort(1);
            }
         }
      }
      else
         exec_p2 = HARD_PULSE;
   }

   if (width3 >= MINWGTIME) 
   { 
      if ( (rfdevice3 < 1) || (rfdevice3 > NUMch))
      {
         abort_message("gensim3shaped_pulse():  RF device %d not within bounds 1 - %d\n",
			rfdevice3,NUMch);
      }
      if ( (RF_Channel[rfdevice3] == NULL) )
      {
         abort_message("gensim3shaped_pulse(): RF Channel device %d not present\n",
			rfdevice3);
      }
      if (strlen(name3))
      {
         if (!is_y(rfwg[rfdevice3-1]))
         {
            text_error("sim3shaped_pulse(): channel %d must have a WFG\n",
                        rfdevice3);
            psg_abort(1);
         }
         exec_p3 = SOFT_PULSE;
         h3 = get_handle(wfg_label[2]);
         temp3 = h3->ap_base;
	 amp_factor = get_wfg_scale(rfdevice3);
         if ( (sldr3 = resolve_ib(h3, name3, width3, amp_factor, 1, 0))
								 == NULL )      
         { 
            if ((sldr3 = create_pulse_block(h3, name3, width3, g1, g2,
				amp_factor)) == NULL)          {
               text_error("gensim3shaped_pulse(): no ib for 3rd pulse\n"); 
               psg_abort(1); 
            }  
         }  
      }
      else
         exec_p3 = HARD_PULSE;
   }

/***********************************************
*  The blocks have joined the global file.     *
*  phase II - point to ib's and fire: 30.2 us  *
***********************************************/

   setHSLdelay();	/* allows HS lines to be reset if necessary */
 
   if (exec_p1 == SOFT_PULSE)
   {
      if (newacq)
      {
      	pointgo_wg(temp1, sldr1->apstaddr);
	G_Delay(DELAY_TIME, INOVA_MINWGSETUP, 0);
      }
      else
      {
      	point_wg(temp1, sldr1->apstaddr);
      	command_wg(temp1, 0x25);
      }
   }
   else
   {
      delay(WFG_START_DELAY - WFG_OFFSET_DELAY);
   }
 
   if (exec_p2 == SOFT_PULSE)
   {
      if (newacq)
      {
      	pointgo_wg(temp2, sldr2->apstaddr);
	G_Delay(DELAY_TIME, INOVA_MINWGSETUP, 0);
      }
      else
      {
      	point_wg(temp2, sldr2->apstaddr);
      	command_wg(temp2, 0x25);
      }
   }
   else
   {
      delay(WFG_START_DELAY - WFG_OFFSET_DELAY);
   }

   if (exec_p3 == SOFT_PULSE)
   {
      if (newacq)
      {
      	pointgo_wg(temp3, sldr3->apstaddr);
	G_Delay(DELAY_TIME, INOVA_MINWGSETUP, 0);
      }
      else
      {
      	point_wg(temp3, sldr3->apstaddr);
      	command_wg(temp3, 0x25);
      }
   }
   else
   {
      delay(WFG_START_DELAY - WFG_OFFSET_DELAY);
   }
/*
   if (newacq)
	G_Delay(DELAY_TIME, INOVA_MINWGSETUP, 0);
*/


/*************************
*  Order pulses/devices  *
*************************/

   if (width1 > width2)
   {
      if (width1 > width3)
      {
         pwlong = width1;
         devlong = rfdevice1;
         execlong = exec_p1;
         factorlong = (exec_p1 == SOFT_PULSE) ? sldr1->factor : 1.0;
 
         if (width3 > width2)
         {
            pwshort = width2;
            pwmed = width3;
            devshort = rfdevice2;
            devmed = rfdevice3;
            execshort = exec_p2;
            execmed = exec_p3;
            factorshort = (exec_p2 == SOFT_PULSE) ? sldr2->factor : 1.0;
            factormed = (exec_p3 == SOFT_PULSE) ? sldr3->factor : 1.0;
         }
         else
         {
            pwshort = width3;
            pwmed = width2;
            devshort = rfdevice3;
            devmed = rfdevice2;
            execshort = exec_p3;
            execmed = exec_p2;
            factorshort = (exec_p3 == SOFT_PULSE) ? sldr3->factor : 1.0;
            factormed = (exec_p2 == SOFT_PULSE) ? sldr2->factor : 1.0;
         }
      }  
      else
      {  
         pwlong = width3;
         pwmed = width1;
         pwshort = width2;
         devlong = rfdevice3;
         devmed = rfdevice1;
         devshort = rfdevice2;
         execlong = exec_p3;
         execmed = exec_p1;
         execshort = exec_p2;
         factorlong = (exec_p3 == SOFT_PULSE) ? sldr3->factor : 1.0;
         factormed = (exec_p1 == SOFT_PULSE) ? sldr1->factor : 1.0;
         factorshort = (exec_p2 == SOFT_PULSE) ? sldr2->factor : 1.0;
      }
   }  
   else
   {
      if (width2 > width3)
      {
         pwlong = width2;
         devlong = rfdevice2;
         execlong = exec_p2;
         factorlong = (exec_p2 == SOFT_PULSE) ? sldr2->factor : 1.0;
 
         if (width3 > width1)
         {
            pwshort = width1;
            pwmed = width3;
            devshort = rfdevice1;
            devmed = rfdevice3;
            execshort = exec_p1;
            execmed = exec_p3;
            factorshort = (exec_p1 == SOFT_PULSE) ? sldr1->factor : 1.0;
            factormed = (exec_p3 == SOFT_PULSE) ? sldr3->factor : 1.0;
         }
         else
         {
            pwshort = width3;
            pwmed = width1;
            devshort = rfdevice3;
            devmed = rfdevice1;
            execshort = exec_p3;
            execmed = exec_p1;
            factorshort = (exec_p3 == SOFT_PULSE) ? sldr3->factor : 1.0;
            factormed = (exec_p1 == SOFT_PULSE) ? sldr1->factor : 1.0;
         }
      }
      else
      {
         pwlong = width3;
         pwmed = width2;
         pwshort = width1;
         devlong = rfdevice3;
         devmed = rfdevice2;
         devshort = rfdevice1;
         execlong = exec_p3;
         execmed = exec_p2;
         execshort = exec_p1;
         factorlong = (exec_p3 == SOFT_PULSE) ? sldr3->factor : 1.0;
         factormed = (exec_p2 == SOFT_PULSE) ? sldr2->factor : 1.0;
         factorshort = (exec_p1 == SOFT_PULSE) ? sldr1->factor : 1.0;
      }
   }

   centertime4 = centertime1 = (pwlong - pwmed) / 2.0;
   centertime3 = centertime2 = (pwmed - pwshort) / 2.0;
   endexecshort = execshort; enddevshort=devshort; endfactorshort=factorshort;
   endexecmed = execmed; enddevmed=devmed; endfactormed=factormed;
   endexeclong = execlong; enddevlong=devlong; endfactorlong=factorlong;

/***************************************************
*  phase III - coordinate and adjust shaped pulses *
****************************************************/
 
   nsoftpul = 0;
   if (execshort == SOFT_PULSE) nsoftpul++;
   if (execmed == SOFT_PULSE) nsoftpul++;
   if (execlong == SOFT_PULSE) nsoftpul++;

   if (nsoftpul == 3)
   {
      pwshort = pwshort + WFG3_OFFSET_DELAY;
   }
   if (nsoftpul == 2)
   {
      if (execlong == SOFT_PULSE)
      {
	if (execmed == SOFT_PULSE)
	{
	   centertime2 = centertime2 + WFG3_OFFSET_DELAY;
	}
	else /* execshort == SOFT_PULSE */
	{
	   centertime1 = centertime1 + WFG3_OFFSET_DELAY;
	   if (centertime2 < WFG3_OFFSET_DELAY)
	   {
	      pwshort = pwshort + centertime2;
	      centertime2 = WFG3_OFFSET_DELAY - centertime2;
	      exectmp = execshort; devtmp = devshort;  factortmp = factorshort;
	      execshort = execmed; devshort = devmed; factorshort = factormed;
	      execmed = exectmp; devmed = devtmp; factormed = factortmp;
	   }
	   else
	   {
	      centertime2 = centertime2 - WFG3_OFFSET_DELAY;
	      pwshort = pwshort + WFG3_OFFSET_DELAY;
	   }
	}
      }
      else /* execmed and execshort are softpulses */
      {
	if ( (centertime1+centertime2) < WFG3_OFFSET_DELAY)
	{
	   pwshort = pwlong;
	   centertime1 = centertime2;
	   centertime2 = WFG3_OFFSET_DELAY - (centertime1+centertime2);
	   exectmp = execmed; devtmp = devmed;  factortmp = factormed;
	   execmed = execshort; devmed = devshort; factormed = factorshort;
	   execshort = execlong; devshort = devlong; factorshort = factorlong;
	   execlong = exectmp; devlong = devtmp; factorlong = factortmp;
	}
	else if (centertime1 < WFG3_OFFSET_DELAY)
	{
	   centertime2 = centertime2 + centertime1 - WFG3_OFFSET_DELAY;
	   centertime1 = WFG3_OFFSET_DELAY - centertime1;
	   pwshort = pwshort + WFG3_OFFSET_DELAY;
	   exectmp = execmed; devtmp = devmed;  factortmp = factormed;
	   execmed = execlong; devmed = devlong; factormed = factorlong;
	   execlong = exectmp; devlong = devtmp; factorlong = factortmp;
	}
	else
	{
	   centertime1 = centertime1 - WFG3_OFFSET_DELAY;
	   pwshort = pwshort + WFG3_OFFSET_DELAY;
	}
      }
   }
   if (nsoftpul == 1)
   {
      if (execlong == SOFT_PULSE)
	centertime1 = centertime1 + WFG3_OFFSET_DELAY;
      else if (execmed == SOFT_PULSE)
      {
	if (centertime1 < WFG3_OFFSET_DELAY)
	{
	   centertime2 = centertime2 + centertime1;
	   centertime1 = WFG3_OFFSET_DELAY - centertime1;
	   exectmp = execmed; devtmp = devmed;  factortmp = factormed;
	   execmed = execlong; devmed = devlong; factormed = factorlong;
	   execlong = exectmp; devlong = devtmp; factorlong = factortmp;
	}
	else
	{
	   centertime1 = centertime1 - WFG3_OFFSET_DELAY;
	   centertime2 = centertime2 + WFG3_OFFSET_DELAY;
	}
      }
      else  /* execshort == SOFT_PULSE */
      {
	if ( (centertime1+centertime2) < WFG3_OFFSET_DELAY)
	{
	   pwshort = pwmed;
	   centertime2 = centertime1;
	   centertime1 = WFG3_OFFSET_DELAY - (centertime1+centertime2);
	   exectmp = execshort; devtmp = devshort;  factortmp = factorshort;
	   execshort = execmed; devshort = devmed; factorshort = factormed;
	   execmed = execlong; devmed = devlong; factormed = factorlong;
	   execlong = exectmp; devlong = devtmp; factorlong = factortmp;
	}
	else if (centertime2 < WFG3_OFFSET_DELAY)
	{
	   centertime2 = WFG3_OFFSET_DELAY - centertime2;
	   exectmp = execshort; devtmp = devshort;  factortmp = factorshort;
	   execshort = execmed; devshort = devmed; factorshort = factormed;
	   execmed = exectmp; devmed = devtmp; factormed = factortmp;
	   pwshort = pwmed;
	}
	else
	{ 
	   centertime2 = centertime2 - WFG3_OFFSET_DELAY;
	   pwshort = pwshort + WFG3_OFFSET_DELAY;
	}
      }
   }


/***********************
*  Execute pulses      *
************************/

   if (exec_p1 != NO_PULSE)
      SetRFChanAttr(RF_Channel[rfdevice1], SET_RTPHASE90, phase1, 0);
   if (exec_p2 != NO_PULSE)
      SetRFChanAttr(RF_Channel[rfdevice2], SET_RTPHASE90, phase2, 0);
   if (exec_p3 != NO_PULSE)
      SetRFChanAttr(RF_Channel[rfdevice3], SET_RTPHASE90, phase3, 0);

   if (ap_interface < 4)
   {
      HSgate(rcvr_hs_bit, TRUE);
      sisdecblank(OFF);
   }
   else
   {
      if (newacq)
	  HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
      else
	  SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, OFF, 0);
      if (exec_p1 != NO_PULSE)
         SetRFChanAttr(RF_Channel[rfdevice1], SET_RCVRGATE, OFF, 0);
      if (exec_p2 != NO_PULSE)
         SetRFChanAttr(RF_Channel[rfdevice2], SET_RCVRGATE, OFF, 0);
      if (exec_p3 != NO_PULSE)
         SetRFChanAttr(RF_Channel[rfdevice3], SET_RCVRGATE, OFF, 0);
   }

   delayer(rx1-g1, FALSE);		/* pre-pulse delay          */


   if (execlong == SOFT_PULSE)
      set_wfgen(devlong, TRUE, factorlong);
   else if (execlong == HARD_PULSE)
      SetRFChanAttr(RF_Channel[devlong], SET_XMTRGATE, ON, 0);

   delayer(centertime1+g1, FALSE);

   if (execmed == SOFT_PULSE)
      set_wfgen(devmed, TRUE, factormed);
   else if (execmed == HARD_PULSE)
      SetRFChanAttr(RF_Channel[devmed], SET_XMTRGATE, ON, 0);

   delayer(centertime2, FALSE);

   if (execshort == SOFT_PULSE)
      set_wfgen(devshort, TRUE, factorshort);
   else if (execshort == HARD_PULSE)
      SetRFChanAttr(RF_Channel[devshort], SET_XMTRGATE, ON, 0);

   delayer(pwshort, FALSE);

   if (endexecshort == SOFT_PULSE)
      set_wfgen(enddevshort, FALSE, endfactorshort);
   else if (endexecshort == HARD_PULSE)
      SetRFChanAttr(RF_Channel[enddevshort], SET_XMTRGATE, OFF, 0);

   delayer(centertime3, FALSE);

   if (endexecmed == SOFT_PULSE)
      set_wfgen(enddevmed, FALSE, endfactormed);
   else if (endexecmed == HARD_PULSE)
      SetRFChanAttr(RF_Channel[enddevmed], SET_XMTRGATE, OFF, 0);

   delayer(centertime4+g2, FALSE);

   if (endexeclong == SOFT_PULSE)
      set_wfgen(enddevlong, FALSE, endfactorlong);
   else if (endexeclong == HARD_PULSE)
      SetRFChanAttr(RF_Channel[enddevlong], SET_XMTRGATE, OFF, 0);


   delayer(rx2-g1, FALSE);	/* post-pulse delay         */

   if (ap_interface < 4)
   {
      if (!rcvroff_flag)
         HSgate(rcvr_hs_bit, FALSE);
      sisdecblank(ON);
   }
   else
   {
      if (newacq) {
	 if (!rcvroff_flag)		/* turn receiver back on only if */
	   HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
      }
      else {
         if ( isBlankOn(OBSch) )
           SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, ON, 0);
      }
      if ( (exec_p1 != NO_PULSE) && isBlankOn(rfdevice1) )
         SetRFChanAttr(RF_Channel[rfdevice1], SET_RCVRGATE, ON, 0);
      if ( (exec_p2 != NO_PULSE) &&  isBlankOn(rfdevice2) )
         SetRFChanAttr(RF_Channel[rfdevice2], SET_RCVRGATE, ON, 0);
      if ( (exec_p3 != NO_PULSE) &&  isBlankOn(rfdevice3) )
         SetRFChanAttr(RF_Channel[rfdevice3], SET_RCVRGATE, ON, 0);
   }

/****************
*  reset WFG's  *
****************/

   setHSLdelay();

   if (newacq)
   {
	G_Delay(DELAY_TIME, INOVA_MINWGCMPLT, 0);
   }
   else
   {
      if ( (exec_p1 == SOFT_PULSE) && (ap_interface < 4) )
          command_wg(temp1, 0x20);
      else
         delay(WFG_STOP_DELAY);
      if ( (exec_p2 == SOFT_PULSE) && (ap_interface < 4) )
          command_wg(temp2, 0x20);
      else
         delay(WFG_STOP_DELAY);
      if ( (exec_p3 == SOFT_PULSE) && (ap_interface < 4) )
          command_wg(temp3, 0x20);
      else
         delay(WFG_STOP_DELAY);
   }
}


/*-----------------------------------------------
|						|
|	       gen2spinlock()/11		|
|						|
+----------------------------------------------*/
gen2spinlock(name1, name2, pw90_1, pw90_2, dres_1, dres_2, phs_1,
		phs_2, ncycles, rfdevice1, rfdevice2)
char	*name1,
	*name2;
codeint	phs_1,
	phs_2;
int	ncycles,
	rfdevice1,
	rfdevice2;
double	pw90_1,
	pw90_2,
	dres_1,
	dres_2;
{
   char                 wfg_label,
			name[MAXSTR];
   int                  i,
			rfdev,
			temp,
                        precfield;
   double		pw90,
			sltime,
			sltime2;
   handle               h1,
			h2,
			*htemp;
   struct ib_list       *sldr1 = NULL,
			*sldr2,
			*sldrtemp;
   extern double	get_wfg_scale();
   double		amp_factor;


   if (ncycles < 1)
      return;

/*********************************************
*  Setup both of the instruction blocks for  *
*  the two WFG spinlocks.                    *
*********************************************/

   wtg_active = ( (ap_interface < 4) ? FALSE : TRUE );
			/* only active for UNITY+ */

   for (i = 0; i < 2; i++)
   {
      switch (i)
      {
         case 0:   rfdev = rfdevice1;
                   htemp = &h1;
                   precfield = (int) (dres_1*10.0 + 0.5);
                   dpf = dres_1;
                   strcpy(name, name1);
                   pw90 = pw90_1;
                   break;
         default:  rfdev = rfdevice2;
                   htemp = &h2;
                   precfield = (int) (dres_2*10.0 + 0.5);
                   dpf = dres_2;
                   strcpy(name, name2);
                   pw90 = pw90_2;
      }
   
      if ( (rfdev < 1) || (rfdev > NUMch) )
      {
         abort_message("gen2spinlock(): invalid RF channel %d\n", rfdev);
      }

      if (!is_y(rfwg[rfdev-1]))
      {
         abort_message("gen2spinlock(): no waveform generator on channel %d\n",
		   rfdev);
      }

      if (pgdrunning[rfdev])
      {
         abort_message("gen2spinlock():  WFG already in use on channel %d\n",
		   rfdev);
      }

      switch (rfdev)
      {
         case TODEV:	wfg_label = 'o'; break;	  /* observe	 */
         case DODEV:	wfg_label = 'd'; break;	  /* 1st decoupler */
         case DO2DEV:	wfg_label = 'e'; break;	  /* 2nd decoupler */
         default:	wfg_label = 'f';	  /* 3rd decoupler */
      }
 
      *htemp = get_handle(wfg_label);
      if (dpf < 0.7)
      {
        text_error("tip angle resolution >= 1 degree");
        psg_abort(1);
      }

      amp_factor = get_wfg_scale(rfdev);
      if ( (sldrtemp = resolve_ib(*htemp, name, pw90, amp_factor, SPINLOCK,
		precfield)) == NULL )
      {
         if ( (sldrtemp = create_dec_block(*htemp, name, pw90, amp_factor,
		SPINLOCK, precfield)) == NULL )
         {
            text_error("gen2spinlock(): ib create failed!\n");
            psg_abort(1);
         }
      }

      switch (i)
      {
         case 0:   sldr1 = sldrtemp; break;
         default:  sldr2 = sldrtemp;
      }
   }

/*******************
*  Program WFG's.  *
*******************/

   wfg_locked = TRUE;

   if (ap_interface < 4)
   {
      HSgate(rcvr_hs_bit, TRUE);
   }
   else
   {
      if (newacq)
	  HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
      else
	  SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, OFF, 0);
      SetRFChanAttr(RF_Channel[rfdevice1], SET_RCVRGATE, OFF, 0);
      SetRFChanAttr(RF_Channel[rfdevice2], SET_RCVRGATE, OFF, 0);
   }

   setHSLdelay();
   temp = h1->ap_base;
   if (newacq) {
   	point_wg(temp, sldr1->apstaddr);
   	command_wg(temp, 0x27);
   	/* pointloop_wg(temp, sldr1->apstaddr); */
   }
   else
   {
   	point_wg(temp, sldr1->apstaddr);
   	command_wg(temp, 0x27);
   }
   temp = h2->ap_base;
   if (newacq)
   {
   	point_wg(temp, sldr2->apstaddr);
   	command_wg(temp, 0x27);
	G_Delay(DELAY_TIME, INOVA_MINWGSETUP-INOVA_MINWGCMPLT, 0); /* wfg overhead time */
   }
   else
   {
   	point_wg(temp, sldr2->apstaddr);
   	command_wg(temp, 0x27);
   }

   pgdrunning[rfdevice1] = TRUE;
   pgdrunning[rfdevice2] = TRUE;

   sltime  = (sldr1->nticks) * ncycles * 5.0e-8;
   sltime2 = (sldr2->nticks) * ncycles * 5.0e-8;
   sltime = ( (sltime < sltime2) ? sltime : sltime2 );
		/*
		   use the shortest spinlock time
		*/

   set_wfgen(rfdevice1, TRUE, sldr1->factor);
   set_wfgen(rfdevice2, TRUE, sldr2->factor);
   delay(WFG_OFFSET_DELAY);

   SetRFChanAttr(RF_Channel[rfdevice1], SET_RTPHASE90, phs_1, NULL);
   SetRFChanAttr(RF_Channel[rfdevice2], SET_RTPHASE90, phs_2, NULL);

   if (!wtg_active)
   {
      SetRFChanAttr(RF_Channel[rfdevice1], SET_XMTRGATE, ON, NULL);
      SetRFChanAttr(RF_Channel[rfdevice2], SET_XMTRGATE, ON, NULL);
      delay(sltime);
      SetRFChanAttr(RF_Channel[rfdevice1], SET_XMTRGATE, OFF, NULL);
      SetRFChanAttr(RF_Channel[rfdevice2], SET_XMTRGATE, OFF, NULL);
   }
   else
   {
      SetRFChanAttr(RF_Channel[rfdevice1], SET_XMTRGATE, OFF, NULL);
      SetRFChanAttr(RF_Channel[rfdevice2], SET_XMTRGATE, OFF, NULL);
      delay(sltime);
   }

   set_wfgen(rfdevice1, FALSE, sldr1->factor);
   set_wfgen(rfdevice2, FALSE, sldr2->factor);
   setHSLdelay();

   temp = h1->ap_base;
   command_wg(temp, 0xa0); /* stop now!! */
   temp = h2->ap_base;
   command_wg(temp, 0xa0); /* stop now!! */

   wtg_active = FALSE;

   if (ap_interface < 4)
   {
      if (!rcvroff_flag)
         HSgate(rcvr_hs_bit, FALSE);
   }
   else
   {
      if (newacq) {
	 if (!rcvroff_flag)		/* turn receiver back on only if */
	   HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
      }
      else {
         if ( isBlankOn(OBSch) )
           SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, ON, 0);
      }
      if ( isBlankOn(rfdevice1) )
         SetRFChanAttr(RF_Channel[rfdevice1], SET_RCVRGATE, ON, 0);
      if ( isBlankOn(rfdevice2) )
         SetRFChanAttr(RF_Channel[rfdevice2], SET_RCVRGATE, ON, 0);
   }
}


/*-----------------------------------------------
|						|
|		genspinlock()/6			|
|						|
|   name        -  name of pattern              |
|   timeper90   -  time for a 90 degree flip    |
|   deg_res     -  degree resolution (min. 0.7) |
|   phsval	-  phase of spin lock		|
|   ncycles	-  number of spinlock cycles	|
|   rfdevice    -  RF device                    |
|						|
+----------------------------------------------*/
void genspinlock(char *name, double pw_90, double deg_res, codeint phsval, int ncycles, int rfdevice)
{
   int		nticks; /* number of 50ns ticks per spinlock pattern */
   double	sltime; /* exact time of the spin lock in the WFG    */
   void		prg_dec_off();


   if ( (rfdevice < 1) || (rfdevice > NUMch) )
   {
      abort_message("genspinlock():  RF channel %d is invalid",
		rfdevice);
   }

   if (ncycles < 1)
      return;

   if (ap_interface < 4)
   {
      HSgate(rcvr_hs_bit, TRUE);
   }
   else
   {
      if (newacq)
	  HSgate(INOVA_RCVRGATE,TRUE);	/* turn receiver Off */
      else
	  SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, OFF, 0);
      SetRFChanAttr(RF_Channel[rfdevice], SET_RCVRGATE, OFF, 0);
   }

   wtg_active = ( (ap_interface < 4) ? FALSE : TRUE );
			/* only active for UNITY+ */
   nticks = prg_dec_on(name, pw_90, deg_res, rfdevice);

   if (nticks < 0)
   {
      abort_message("genspinlock():  WFG already in use on channel %d\n",
		rfdevice);
   }
   else if (nticks == 0)
   {
      abort_message("genspinlock():  cannot initiate WFG spin lock on channel %d\n",
		rfdevice);
   }

   sltime = nticks * ncycles * 5.0e-8;
   delay(WFG_OFFSET_DELAY);

   SetRFChanAttr(RF_Channel[rfdevice], SET_RTPHASE90, phsval, NULL);

   if (!wtg_active)
   {
      SetRFChanAttr(RF_Channel[rfdevice], SET_XMTRGATE, ON, NULL);
      delay(sltime);
      SetRFChanAttr(RF_Channel[rfdevice], SET_XMTRGATE, OFF, NULL);
   }
   else
   {
      SetRFChanAttr(RF_Channel[rfdevice], SET_XMTRGATE, OFF, NULL);
      delay(sltime);
   }

   prg_dec_off(2, rfdevice);
   wtg_active = FALSE;

   if (ap_interface < 4)
   {
      if (!rcvroff_flag)
         HSgate(rcvr_hs_bit, FALSE);
   }
   else
   {
      if (newacq) {
	 if (!rcvroff_flag)		/* turn receiver back on only if */
	   HSgate(INOVA_RCVRGATE,FALSE);	/* turn receiver On */
      }
      else {
         if ( isBlankOn(OBSch) )
           SetRFChanAttr(RF_Channel[OBSch],  SET_RCVRGATE, ON, 0);
      }
      if ( isBlankOn(rfdevice) )
         SetRFChanAttr(RF_Channel[rfdevice], SET_RCVRGATE, ON, 0);
   }
}


/*-----------------------------------------------
|						|
|		 prg_dec_on()/4			|
|						|
|   name	-  name of pattern		|
|   timeper90	-  time for a 90 degree flip	|
|   deg_res	-  degree resolution (min. 0.7)	|
|   rfdevice	-  RF device			|
|						|
+----------------------------------------------*/
int prg_dec_on(char *name, double pw_90, double deg_res, int rfdevice)
{
   char                 wfg_label;
   int                  temp,
                        precfield;
   handle               h1;
   struct ib_list       *sldr;
   extern void		setHSLdelay();
   extern double	get_wfg_scale();
   double		amp_factor;
 

   if ( (rfdevice < 1) || (rfdevice > NUMch) )
   {
      abort_message("prg_dec_on():  RF channel %d is invalid\n", rfdevice);
   }

   if (!is_y(rfwg[rfdevice-1]))
   {
      text_error("prg_dec_on():  no waveform generator on channel %d\n",
		rfdevice);
      text_error("WFG programmable decoupling or spinlocking not allowed");
      psg_abort(1);
   }

   if (pgdrunning[rfdevice])
   {
      delay(PRG_START_DELAY);
      return(-1);
   }
 
   switch (rfdevice)
   {
      case TODEV:	wfg_label = 'o'; break;		/* observe	 */
      case DODEV:	wfg_label = 'd'; break;		/* 1st decoupler */
      case DO2DEV:	wfg_label = 'e'; break;		/* 2nd decoupler */
      default:		wfg_label = 'f';		/* 3rd decoupler */
   }
 
   wfg_locked = TRUE;
   h1 = get_handle(wfg_label);
   if (deg_res < 0.7)
   {
     text_error("decoupling tip angle resolution >= 1 degree");
     psg_abort(1);
   }
 
   precfield = (int) (deg_res*10.0 + 0.5);
   dpf = deg_res;
 
/******************************************
*  precfield carries prec in 1/10 degree  *
*  resolution                             *
******************************************/
 
   amp_factor = get_wfg_scale(rfdevice);
   if ( (sldr = resolve_ib(h1, name, pw_90, amp_factor, DECOUPLE, precfield))
		== NULL )
   {
      if ( (sldr = create_dec_block(h1, name, pw_90, amp_factor, DECOUPLE,
		precfield)) == NULL )
      {
         text_error("decoupler: ib create failed!\n");
         psg_abort(1);
      }
   }
 
   setHSLdelay();	/* to allow HS lines to be reset if necessary */
   temp = h1->ap_base;
   if (newacq)
   {
   	point_wg(temp, sldr->apstaddr);
   	command_wg(temp, 0x27);
	G_Delay(DELAY_TIME, INOVA_MINWGSETUP-INOVA_MINWGCMPLT, 0); /* wfg overhead time */
   }
   else
   {
   	point_wg(temp, sldr->apstaddr);
   	command_wg(temp, 0x27);
   }
   pgdrunning[rfdevice] = TRUE;
   set_wfgen(rfdevice, TRUE, sldr->factor);
   return(sldr->nticks);
}


/*-----------------------------------------------
|                                               |
|                setprgmode()/2                 |
|                                               |
+----------------------------------------------*/
void setprgmode(int mode, int rfdevice)
{
   char         wfg_label;
   int          temp;
   handle       h1;
   extern void	setHSLdelay();


   if ( (rfdevice < 1) || (rfdevice > NUMch) )
   {
      abort_message("setprgmode(): invalid RF channel %d\n",
                rfdevice);
   }

   if (!is_y(rfwg[rfdevice-1]))
   {
      abort_message("setprgmode(): no waveform generator on channel %d\n",
                rfdevice);
   }
 
   if (!pgdrunning[rfdevice])
      return;
 
   if (ap_interface < 4)
   { /* VXR and UNITY systems */
      switch (rfdevice)
      {
         case TODEV:       wfg_label = 'o'; break;    /* observe	 */
         case DODEV:       wfg_label = 'd'; break;    /* 1st decoupler */
         case DO2DEV:      wfg_label = 'e'; break;    /* 2nd decoupler */
         default:          wfg_label = 'f';           /* 3rd decoupler */
      }
    
      h1 = get_handle(wfg_label);
      temp = h1->ap_base;
      setHSLdelay();
      command_wg( temp, (mode ? 0x26 : 0x22) );
   }
   else
   { /* UNITY-plus systems */
      set_wfgen(rfdevice, mode, 1.0);
   }
}


/*-----------------------------------------------
|						|
|		prg_dec_off()/2			|
|						|
|   stopmode	-   0 => finish loop  (works)	|
|		    1 => hold down HS line  (?)	|
|		    2 => kill now  (works)	|
|   rfdevice	-   RF device			|
|						|
+----------------------------------------------*/
void prg_dec_off(int stopmode, int rfdevice)
{
   char         wfg_label;
   int          temp;
   handle       h1;
 
 
   if ( (rfdevice < 1) || (rfdevice > NUMch) )
   {
      abort_message("prg_dec_off(): RF channel %d is invalid\n", rfdevice);
   }

   if ( (!is_y(rfwg[rfdevice-1])) || (!pgdrunning[rfdevice]) )
      return;
 
   switch (rfdevice)
   {
      case TODEV:	wfg_label = 'o'; break;		/* observe	 */
      case DODEV:	wfg_label = 'd'; break;		/* 1st decoupler */
      case DO2DEV:	wfg_label = 'e'; break;		/* 2nd decoupler */
      default:		wfg_label = 'f';		/* 3rd decoupler */
   }
 
   h1 = get_handle(wfg_label);
   temp = h1->ap_base;
   set_wfgen(rfdevice, FALSE, 1.0);
   switch (stopmode)
   {
      case 1:
                break;
      case 2:
		setHSLdelay();
		command_wg(temp, 0xa0); /* stop now!! */
                break;
      default:
		setHSLdelay();
		command_wg(temp, 0x20);    /* stop at end of pattern */
   }
 
   pgdrunning[rfdevice] = FALSE;
}


/*-----------------------------------
|                                   |
|       init_pgd_hslines()/1        |
|                                   |
+----------------------------------*/
void init_pgd_hslines(hslines)
int     *hslines;
{
   int  i;

   for (i = 1; i < (MAX_RFCHAN_NUM+1); i++)    /* Do not start at i = 0 ! */
   {
      if (pgdrunning[i])
         *hslines |= wfgen[waveboard[i]].hs_line;
   }
}


/*-----------------------------------------------
|						|
|	      reset_pgdflag()/0			|
|						|
+----------------------------------------------*/
void reset_pgdflag()
{
   int	i;

   for (i = 0; i < (MAX_RFCHAN_NUM+1); i++)
      pgdrunning[i] = FALSE;
}

/*-----------------------------------------------
|						|
|	      pgd_is_running()/1		|
|						|
+----------------------------------------------*/
int pgd_is_running(int channel)
{

      return (pgdrunning[channel]);
}

static void limitampInt(char gid, int *amp)
{
   int index;
   switch (gid) 
   {
     case 'x': case 'X': 
       index = 0; break;
     case 'y': case 'Y': 
       index = 1; break;
     case 'z': case 'Z': 
       index = 2; break;
     default: 
       index = 2; break;
   }
   if (is_u(gradtype[index]))
   {
      *amp /= 2;
      if (*amp > 16384)
         *amp = 16384;
      else if (*amp < -16384)
         *amp = -16384;
   }
}

static void limitamp(char gid, double *amp)
{
   int index;
   switch (gid) 
   {
     case 'x': case 'X': 
       index = 0; break;
     case 'y': case 'Y': 
       index = 1; break;
     case 'z': case 'Z': 
       index = 2; break;
     default: 
       index = 2; break;
   }
   if (is_u(gradtype[index]))
   {
      *amp /= 2.0;
      if (*amp > 16384.0)
         *amp = 16384.0;
      else if (*amp < -16384.0)
         *amp = -16384.0;
   }
}

void S_shapedgradient(char *name, double width, double amp,
                      char which, int loops, int wait_4_me)
{  
   struct ib_list *sldr;
   handle h1;
   int temp;
   if (width <  MINWGTIME) 
     return;
   if ((which == 'n') || (which == 'N'))
     return;
   if (no_grad_wg(which))
   {
     text_error("No gradient waveform generator");
     psg_abort(1);
   }
   limitamp(which, &amp);
   h1 = get_handle(which);
   sldr = resolve_ib(h1,name,width,amp,loops,0);
   if (sldr == NULL) 
   {
      sldr = create_grad_block(h1,name,width,amp,loops,0);
   } 
   if (sldr == NULL) 
   {
      text_error("shaped gradient: ib create failed!");
      psg_abort(1);
   }
   /* 
   the block has joined the global file 
   */
   temp = h1->ap_base;
   if (newacq)
   {
   	pointgo_wg(temp,sldr->apstaddr);
   }
   else
   {
   	point_wg(temp,sldr->apstaddr);
   	command_wg(temp, 0x11);
   }
   grad_flag = TRUE;
   if (wait_4_me)
   {
      width *= (double) loops;
      G_Delay(DELAY_TIME,width,0);
   }
}

shaped_2D_gradient(name,width,amp,which,loops,wait_4_me,tag)
/* only amp variation with 2d implemented! */
char *name,which;
double width,amp;
int wait_4_me,loops,tag;
{  
   struct ib_list *sldr;
   handle h1;
   int temp;
   if (width <  MINWGTIME) 
     return;
   if ((which == 'n') || (which == 'N'))
     return;
   if (no_grad_wg(which))
   {
     text_error("No gradient waveform generator");
     psg_abort(1);
   }
   limitamp(which, &amp);
   h1 = get_handle(which);
   grad_flag = TRUE;
   sldr = resolve_ib(h1,name,width,0.0,1,tag);
   /* set amp to zero so that it will always match despite
      amp changing in the call */
   if ((sldr == NULL) && (ix==1))
   {
      sldr = create_grad_block(h1,name,width,0.0,1,tag);
   } 
   if (sldr == NULL) 
   {
      text_error("shaped_2D_gradient: no match!\n");
      psg_abort(1);
   }
   /* 
      overwrite to place the correct amp
   */
   temp = h1->ap_base;
   /*     fine control to the top for flexiblity */
   /*     2 is 3rd field in IB 			 */
   write_wg(temp,sldr->apstaddr+2,pat_scale(loops,(int) amp));
   if (newacq)
   {
   	pointgo_wg(temp,sldr->apstaddr);
   }
   else
   {
   	point_wg(temp,sldr->apstaddr);
   	command_wg(temp,0x11);
   }
   if (wait_4_me)
      G_Delay(DELAY_TIME,width,0);
}

shaped_2D_Vgradient(name,width,vx,vconst,vmult,which,loops,wait_4_me,tag)
/* only amp variation with 2d implemented! */
char *name,which;
double width;
int vx, vmult,vconst;
int wait_4_me,loops,tag;
{  
   struct ib_list *sldr;
   handle h1;
   int temp;
   if (width <  MINWGTIME) 
     return;
   if ((which == 'n') || (which == 'N'))
     return;
   if (no_grad_wg(which))
   {
     text_error("No gradient waveform generator");
     psg_abort(1);
   }
   h1 = get_handle(which);
   grad_flag = TRUE;
   sldr = resolve_ib(h1,name,width,0.0,1,tag);
   /* set amp to zero so that it will always match despite
      amp changing in the call */
   if ((sldr == NULL) && (ix==1))
   {
      sldr = create_grad_block(h1,name,width,0.0,1,tag);
   } 
   if (sldr == NULL) 
     psg_abort(1);

   /* limit dac values for selected amplifiers (l200) */ 
   limitampInt(which, &vconst);
   limitampInt(which, &vx);

   /* 
      overwrite to place the correct amp
   */
   temp = h1->ap_base;
   /*
   write_wg(temp,sldr->apstaddr+2,pat_scale(loops,(int) amp));
   Vwrite_wg(temp,sldr->apstaddr+2,sldr->mask,vconst,vx,vmult);
   */
   if (newacq)
   {
   	pointgo_wg(temp,sldr->apstaddr);
   }
   else
   {
   	pointgo_wg(temp,sldr->apstaddr);
   	command_wg(temp,0x11);
   }
   if (wait_4_me)
      G_Delay(DELAY_TIME,width,0);
}

/****************************************************************/
/* TWO new elements added by dlmattiello 2/26/03		*/
/* the goal is to seperate shaped_V_gradient into 2 functions   */
/* the first has point & Vwrite, the second has pointgo only	*/
/****************************************************************/
/* double */
prepWFGforPE(name,width,amp_const,amp_incr,amp_vmult,which,vloops,
		wait_4_me,tag)
/* Allows amplitude and loops to be realtime variables */
char *name,which;
double width,amp_const,amp_incr;
codeint amp_vmult,vloops;
int wait_4_me,tag;
{  
   struct ib_list *sldr;
   handle h1;
   int temp;

   validate_imaging_config("Shapedvgradient");

   if (width <  MINWGTIME) 
     return(0.0);
   if ((which == 'n') || (which == 'N'))
     return;
   if (no_grad_wg(which))
   {
     text_error("No gradient waveform generator");
     psg_abort(1);
   }

   h1 = get_handle(which);
   
   sldr = resolve_ib(h1,name,width,0.0,1,TAG_VAMP);

   /* set amp to zero so that it will always match despite
      amp changing in the call */
   if ((sldr == NULL) && (ix==1))
   {
      sldr = create_grad_block(h1,name,width,0.0,1,TAG_VAMP);
   } 
   if (sldr == NULL) 
   {
      text_error("prepWFGforPE: no match!\n");
      psg_abort(1);
   }

   /* test valid apbus range for amp_vmult,vloops */
   if (((amp_vmult < zero) || (amp_vmult > v14)) && ((amp_vmult < t1) || 
	(amp_vmult > t60))) 
   {
       abort_message("prepWFGforPE: amp_vmult illegal dynamic %d \n",
			       amp_vmult);
   }
   if ((amp_vmult >= t1) && (amp_vmult <= t60))
	amp_vmult = tablertv(amp_vmult);

   if (((vloops < zero) || (vloops > v14)) && ((vloops < t1) || 
	(vloops > t60))) 
   {
       abort_message("prepWFGforPE: vloops illegal dynamic %d \n",vloops);
   }
   if ((vloops >= t1) && (vloops <= t60))
	vloops = tablertv(vloops);

   /* limit dac values for selected amplifiers (l200) */ 
   limitamp(which, &amp_const);
   limitamp(which, &amp_incr);

   /* 
    *  overwrite to place the correct amp
    */
   temp = h1->ap_base;
   point_wg(temp,sldr->apstaddr+2);
   Vwrite_wg(temp+1, pat_scale(1,(int) amp_const), vloops, 
             (int)(amp_const + 0.5), (int)(amp_incr+0.5), amp_vmult);

   return;
}
/****************************************************************/
/* TWO new elements added by dlmattiello 2/26/03		*/
/* the goal is to seperate shaped_V_gradient into 2 functions   */
/* the first has point & Vwrite, the second has pointgo		*/
/****************************************************************/
/* double */
void doshapedPEgradient(name,width,amp_const,amp_incr,amp_vmult,which,vloops,
		wait_4_me,tag)
/* Allows amplitude and loops to be realtime variables */
char *name,which;
double width,amp_const,amp_incr;
codeint amp_vmult,vloops;
int wait_4_me,tag;
{  
   struct ib_list *sldr;
   handle h1;
   int temp;

   validate_imaging_config("Shapedvgradient");

   if (width <  MINWGTIME) 
     return;
   if ((which == 'n') || (which == 'N'))
     return;
   if (no_grad_wg(which))
   {
     text_error("No gradient waveform generator");
     psg_abort(1);
   }

   h1 = get_handle(which);
   
   sldr = resolve_ib(h1,name,width,0.0,1,TAG_VAMP);
   /* set amp to zero so that it will always match despite
      amp changing in the call */
   if ((sldr == NULL) && (ix==1))
   {
      sldr = create_grad_block(h1,name,width,0.0,1,TAG_VAMP);
   } 
   if (sldr == NULL) 
   {
      text_error("doshapedPEgradient: no match!\n");
      psg_abort(1);
   }

   /* test valid apbus range for amp_vmult,vloops */
   if (((amp_vmult < zero) || (amp_vmult > v14)) && ((amp_vmult < t1) || 
	(amp_vmult > t60))) 
   {
       abort_message("doshapedPEgradient: amp_vmult illegal dynamic %d \n",
			       amp_vmult);
   }
   if ((amp_vmult >= t1) && (amp_vmult <= t60))
	amp_vmult = tablertv(amp_vmult);

   if (((vloops < zero) || (vloops > v14)) && ((vloops < t1) || 
	(vloops > t60))) 
   {
       abort_message("doshapedPEgradient: vloops illegal dynamic %d \n",vloops);
   }
   if ((vloops >= t1) && (vloops <= t60))
	vloops = tablertv(vloops);

   /* limit dac values for selected amplifiers (l200) */ 
   limitamp(which, &amp_const);
   limitamp(which, &amp_incr);

   /* 
    *  overwrite to place the correct amp
    */
   temp = h1->ap_base;

   if (newacq)
   {
   	pointgo_wg(temp,sldr->apstaddr);
   }
   else
   {
   	point_wg(temp,sldr->apstaddr);
   	command_wg(temp,0x11);
   }
   if (wait_4_me)
	G_Delay(DELAY_TIME,width,0);

   return;
}

/****************************************************************/
/* SISCO WG Element						*/
/****************************************************************/
/* double */
shaped_V_gradient(name,width,amp_const,amp_incr,amp_vmult,which,vloops,
		wait_4_me,tag)
/* Allows amplitude and loops to be realtime variables */
char *name,which;
double width,amp_const,amp_incr;
codeint amp_vmult,vloops;
int wait_4_me,tag;
{  
   struct ib_list *sldr;
   handle h1;
   int temp;

   validate_imaging_config("Shapedvgradient");

   if (width <  MINWGTIME) 
     return(0.0);
   if ((which == 'n') || (which == 'N'))
     return;
   if (no_grad_wg(which))
   {
     text_error("No gradient waveform generator");
     psg_abort(1);
   }

   h1 = get_handle(which);
   /* sldr = resolve_ib(h1,name,width,&actual_width,0.0,1,TAG_VAMP); */
   sldr = resolve_ib(h1,name,width,0.0,1,TAG_VAMP);
   /* set amp to zero so that it will always match despite
      amp changing in the call */
   if ((sldr == NULL) && (ix==1))
   {
      /* ldr = create_grad_block(h1,name,width,&actual_width,0.0,1,TAG_VAMP); */
      sldr = create_grad_block(h1,name,width,0.0,1,TAG_VAMP);
   } 
   if (sldr == NULL) 
   {
      text_error("shapedvgradient: no match!\n");
      psg_abort(1);
   }

   /* test valid apbus range for amp_vmult,vloops */
   if (((amp_vmult < zero) || (amp_vmult > v14)) && ((amp_vmult < t1) || 
	(amp_vmult > t60))) 
   {
       abort_message("shapedvgradient: amp_vmult illegal dynamic %d \n",
			       amp_vmult);
   }
   if ((amp_vmult >= t1) && (amp_vmult <= t60))
	amp_vmult = tablertv(amp_vmult);

   if (((vloops < zero) || (vloops > v14)) && ((vloops < t1) || 
	(vloops > t60))) 
   {
       abort_message("shapedvgradient: vloops illegal dynamic %d \n",vloops);
   }
   if ((vloops >= t1) && (vloops <= t60))
	vloops = tablertv(vloops);

   /* limit dac values for selected amplifiers (l200) */ 
   limitamp(which, &amp_const);
   limitamp(which, &amp_incr);

   /* 
    *  overwrite to place the correct amp
    */
   temp = h1->ap_base;
   point_wg(temp,sldr->apstaddr+2);
   Vwrite_wg(temp+1, pat_scale(1,(int) amp_const), vloops, 
             (int)(amp_const + 0.5), (int)(amp_incr+0.5), amp_vmult);
   if (newacq)
   {
   	pointgo_wg(temp,sldr->apstaddr);
   }
   else
   {
   	point_wg(temp,sldr->apstaddr);
   	command_wg(temp,0x11);
   }
   if (wait_4_me)
	/* for now always assume vloops = 1 or zero */
	G_Delay(DELAY_TIME,width,0);
   /*    incdelay(0.0,actual_width,vloops); */
   /* return(actual_width); */
   return;
}



/****************************************************************/
/* TWO new elements added by dlmattiello 4/08/03                */
/* the goal is to seperate shaped_inc_gradient into 2 functions   */
/* the first has point_wg & INCwrite_wg, the second has pointgo         */
/****************************************************************/

/* double */
prepforshapedINCgradient(which, pattern, width, a0, a1, a2, a3,
			   x1, x2, x3, loops, wait)
char which;
char *pattern;
double width;
double a0, a1, a2, a3;
codeint x1, x2, x3;
int loops;
int wait;
{
   int a[4];
   handle h1;
   int i;
   struct ib_list *sldr;
   int temp;
   codeint x[4];

   validate_imaging_config("Shapedincgradient");

   if (width <  MINWGTIME){ 
       return(0.0);
   }
   if ((which == 'n') || (which == 'N'))
     return;
   if (no_grad_wg(which))
   {
     text_error("No gradient waveform generator");
     psg_abort(1);
   }

   h1 = get_handle(which);
   sldr = resolve_ib(h1, pattern, width, 0.0, 1, TAG_VAMP);
   /* sldr = resolve_ib(h1, pattern, width, &actual_width, 0.0, 1, TAG_VAMP); */
   /* Set amp to zero so that it will always match despite
      amp changing in the call */
   if ((sldr == NULL) && (ix==1))
   {
      sldr = create_grad_block(h1, pattern, width, 0.0, 1,
			       TAG_VAMP);
      /* sldr = create_grad_block(h1, pattern, width, &actual_width, 0.0, 1, */
      /* 		       TAG_VAMP); */
   } 
   if (sldr == NULL) 
   {
      text_error("shapedincgradient: no match!");
      psg_abort(1);
   }

   /* Copy arguments into arrays. Note that x[0] isn't used! */
   a[0] = a0;
   a[1] = a1;
   a[2] = a2;
   a[3] = a3;
   x[1] = x1;
   x[2] = x2;
   x[3] = x3;

    /* Test valid range */
    for (i=1; i<=3; i++){
	if ( ((x[i] < v1) || (x[i] > v14))
	    && x[i] != zero
	    && x[i] != ct)
	{
	    abort_message("shapedincgradient: x[%d] illegal RT variable: %d\n",
		    i, x[i]);
	}
    }

    /* limit dac values for selected amplifiers (l200) */
    for (i=0; i<=3; i++)
    {
	limitampInt(which, &a[i]);
    }
   
   /* 
    *  Overwrite instruction in WFG board to set the desired amplitude
    */
   temp = h1->ap_base;
   point_wg(temp, sldr->apstaddr+2);	/* Point to IB_SCALE command */
   INCwrite_wg(temp+1, loops, a, x);	/* Set amplitude and # of loops */
   if (wait){
       G_Delay(DELAY_TIME,width * loops,0);
       /* delay(actual_width * loops); */
   }
   /* return actual_width; */
   return;
}




/****************************************************************/
/* TWO new elements added by dlmattiello 2/26/03                */
/* the goal is to seperate shaped_V_gradient into 2 functions   */
/* the first has point & Vwrite, the second has pointgo         */
/****************************************************************/

/* double */
doshapedINCgradient(which, pattern, width, a0, a1, a2, a3,
			   x1, x2, x3, loops, wait)
char which;
char *pattern;
double width;
double a0, a1, a2, a3;
codeint x1, x2, x3;
int loops;
int wait;
{
   int a[4];
   handle h1;
   int i;
   struct ib_list *sldr;
   int temp;
   codeint x[4];

   validate_imaging_config("Shapedincgradient");

   if (width <  MINWGTIME){ 
       return(0.0);
   }
   if ((which == 'n') || (which == 'N'))
     return;
   if (no_grad_wg(which))
   {
     text_error("No gradient waveform generator");
     psg_abort(1);
   }

   h1 = get_handle(which);
   sldr = resolve_ib(h1, pattern, width, 0.0, 1, TAG_VAMP);
   /* sldr = resolve_ib(h1, pattern, width, &actual_width, 0.0, 1, TAG_VAMP); */
   /* Set amp to zero so that it will always match despite
      amp changing in the call */
   if ((sldr == NULL) && (ix==1))
   {
      sldr = create_grad_block(h1, pattern, width, 0.0, 1,
			       TAG_VAMP);
      /* sldr = create_grad_block(h1, pattern, width, &actual_width, 0.0, 1, */
      /* 		       TAG_VAMP); */
   } 
   if (sldr == NULL) 
   {
      text_error("shapedincgradient: no match!");
      psg_abort(1);
   }

   /* Copy arguments into arrays. Note that x[0] isn't used! */
   a[0] = a0;
   a[1] = a1;
   a[2] = a2;
   a[3] = a3;
   x[1] = x1;
   x[2] = x2;
   x[3] = x3;

    /* Test valid range */
    for (i=1; i<=3; i++){
	if ( ((x[i] < v1) || (x[i] > v14))
	    && x[i] != zero
	    && x[i] != ct)
	{
	    abort_message("shapedincgradient: x[%d] illegal RT variable: %d\n",
		    i, x[i]);
	}
    }

    /* limit dac values for selected amplifiers (l200) */
    for (i=0; i<=3; i++)
    {
	limitampInt(which, &a[i]);
    }
   
   /* 
    *  Overwrite instruction in WFG board to set the desired amplitude
    */
   temp = h1->ap_base;
   if (newacq)
   {
   	pointgo_wg(temp, sldr->apstaddr);	/* Point to first command */
   }
   else
   {
   	point_wg(temp, sldr->apstaddr);	/* Point to first command */
   	command_wg(temp, 0x11);		/* Execute commands (start grad) */
   }
   if (wait){
       G_Delay(DELAY_TIME,width * loops,0);
       /* delay(actual_width * loops); */
   }
   /* return actual_width; */
   return;
}

/****************************************************************/
/* SISCO WG Element						*/
/****************************************************************/

/* double */
shaped_INC_gradient(which, pattern, width, a0, a1, a2, a3,
			   x1, x2, x3, loops, wait)
char which;
char *pattern;
double width;
double a0, a1, a2, a3;
codeint x1, x2, x3;
int loops;
int wait;
{
   int a[4];
   handle h1;
   int i;
   struct ib_list *sldr;
   int temp;
   codeint x[4];

   validate_imaging_config("Shapedincgradient");

   if (width <  MINWGTIME){ 
       return(0.0);
   }
   if ((which == 'n') || (which == 'N'))
     return;
   if (no_grad_wg(which))
   {
     text_error("No gradient waveform generator");
     psg_abort(1);
   }

   h1 = get_handle(which);
   sldr = resolve_ib(h1, pattern, width, 0.0, 1, TAG_VAMP);
   /* sldr = resolve_ib(h1, pattern, width, &actual_width, 0.0, 1, TAG_VAMP); */
   /* Set amp to zero so that it will always match despite
      amp changing in the call */
   if ((sldr == NULL) && (ix==1))
   {
      sldr = create_grad_block(h1, pattern, width, 0.0, 1,
			       TAG_VAMP);
      /* sldr = create_grad_block(h1, pattern, width, &actual_width, 0.0, 1, */
      /* 		       TAG_VAMP); */
   } 
   if (sldr == NULL) 
   {
      text_error("shapedincgradient: no match!");
      psg_abort(1);
   }

   /* Copy arguments into arrays. Note that x[0] isn't used! */
   a[0] = a0;
   a[1] = a1;
   a[2] = a2;
   a[3] = a3;
   x[1] = x1;
   x[2] = x2;
   x[3] = x3;

    /* Test valid range */
    for (i=1; i<=3; i++){
	if ( ((x[i] < v1) || (x[i] > v14))
	    && x[i] != zero
	    && x[i] != ct)
	{
	    abort_message("shapedincgradient: x[%d] illegal RT variable: %d\n",
		    i, x[i]);
	}
    }

    /* limit dac values for selected amplifiers (l200) */
    for (i=0; i<=3; i++)
    {
	limitampInt(which, &a[i]);
    }
   
   /* 
    *  Overwrite instruction in WFG board to set the desired amplitude
    */
   temp = h1->ap_base;
   point_wg(temp, sldr->apstaddr+2);	/* Point to IB_SCALE command */
   INCwrite_wg(temp+1, loops, a, x);	/* Set amplitude and # of loops */
   if (newacq)
   {
   	pointgo_wg(temp, sldr->apstaddr);	/* Point to first command */
   }
   else
   {
   	point_wg(temp, sldr->apstaddr);	/* Point to first command */
   	command_wg(temp, 0x11);		/* Execute commands (start grad) */
   }
   if (wait){
       G_Delay(DELAY_TIME,width * loops,0);
       /* delay(actual_width * loops); */
   }
   /* return actual_width; */
   return;
}


int no_grad_wg(char gid)
{
   int index,val;
   switch (gid) 
   {
     case 'x': case 'X': 
       index = 0; break;
     case 'y': case 'Y': 
       index = 1; break;
     case 'z': case 'Z': 
       index = 2; break;
     default: 
       return(TRUE);   /* bad id */
   }
   val = is_r(gradtype[index]) || is_w(gradtype[index]) || is_q(gradtype[index]) || is_u(gradtype[index]);
   return(!val);
}

int match_ib(sl,nm,wdth,tamp,lloops,key)
struct ib_list *sl;
double wdth, tamp;
int lloops,key;
char *nm;
{  
   int temp;
   temp = (strncmp(sl->tname,nm,MAXSTR) == 0);
   temp = temp && (fabs(sl->amp - tamp) < 1.e-3);
   temp = temp && (fabs(sl->width - wdth) < MINWGRES);
   temp = temp && (sl->iloop == lloops);
   temp = temp && (sl->bstat == key);
/* 
     fprintf(stderr,"-----match ib -------\n");
     fprintf(stderr,"%s   %s\n",sl->tname,nm); 
     fprintf(stderr,"%f   %f\n",sl->amp,tamp); 
     fprintf(stderr,"%f   %f\n",sl->width,wdth); 
     fprintf(stderr,"%d   %d\n",sl->iloop,lloops); 
     fprintf(stderr,"%d   %d\n",sl->bstat,key); 
*/
   return(temp);
}

/********************************************************
resolve_ib()/6
  finds an instruction block on a specified tree
  matches names, duration, amplitude, num loops
  and a key
  2D overwrite scheme sets the relevant field to
  zero writes in the correct one duration sequence
  execution - any false matches are suppressed with
  a unique key which must be in the sequence!! _LINE_
  key is used in decoupling to specify precision in the pattern.
********************************************************/
static struct ib_list *resolve_ib(hx,nm,wdth,tamp,lloops,key)
char *nm;
int lloops,key;
double wdth,tamp;
handle hx;
{
   struct ib_list *slider;
   slider = hx->ib_base;
   while ((slider != NULL) && !match_ib(slider,nm,wdth,tamp,lloops,key))
     slider = slider->ibnext;
   return(slider);
}

/* 
   instructions blocks have an obligatory 
   name  (start stop)
   pattern time
   amplitude  - could be implicit
   loop count - could be implicit
   optional -----
   wait hs trigger
   predelay
   postdelay
*/
/* 
   customize each call with a create_xxx_ib 
   to have full control 
*/

static struct ib_list *create_pulse_block(hx,nm,w,g1,g2,amp)
handle hx;
char *nm;
double w,g1,g2,amp;
{
   int tib[10],mark;
   /* get pattern info */
   struct p_list *test;
   struct ib_list *temp;
   if (newacq)
   	test = resolve_pattern(hx,nm,WRFPULSE,4095.0);
   else
	test = resolve_pattern(hx,nm,WRFPULSE,amp);
   if (test == NULL) psg_abort(1);
   tib[0] = pat_strt(test->wg_st_addr);
   tib[1] = pat_stop((test->wg_st_addr + test->telm),1);
   tib[2] = pat_scale(1,((int)amp));  /* loops and amplitude */
   /* amp gate state on */
   tib[3] = pat_wait_hs;
   mark = 4;
   if (g1 >= MINWGTIME)
   {
     tib[mark] = pat_delay(wgtb(g1,1,1)) | AMPGATE;
     mark++;
   }
   /* should check truncation */
   tib[mark] = pat_time(wgtb(w,test->ticks,WFG_TIME_RESOLUTION)) | AMPGATE;
   mark++;
   if (g2 >= MINWGTIME)
   {
       /* this is sometimes unnecessary */
       tib[mark] = tib[2] | AMPGATE;
       mark++;
       tib[mark] = pat_delay(wgtb(g2,1,1)) | AMPGATE;
       mark++;
   }
   tib[mark] = pat_end;
   mark++;
   /* attach to ib list and emit */
   temp = (struct ib_list *) malloc(sizeof(struct ib_list));
   strcpy(temp->tname,nm);
   temp->width = w;
   temp->iloop = 1;
   temp->amp   = amp;
   temp->factor = test->scl_factor;
   temp->bstat = 0; /* is a normal instruction block */
   temp->nticks = test->ticks;  /* only for completeness at this time! */
   temp->apstaddr = hx->cur_ib;
   /*
      fall through 
      this removes the termination word 
   */
   if ((hx->r__cntrl & FALLTHRU) != 0)
      mark--; 
   hx->cur_ib += mark;
   dump_ib(hx->ap_base,temp->apstaddr,tib,mark);
   temp->ibnext = hx->ib_base;
   hx->ib_base = temp;
   ib_safe(hx);
   return(temp);
}

static struct ib_list *create_dec_block(hx,nm,w,a,ll,pf)
char *nm;
double w,a;
handle hx;
int ll,pf;
{
   int dib[6];
   int dectb; 
   double xx;
   struct p_list *test;
   struct ib_list *temp;
   if (newacq)
   	test = resolve_pattern(hx,nm,WDECOUPLER,4095.0);
   else
   	test = resolve_pattern(hx,nm,WDECOUPLER,a);
   if (test == NULL) psg_abort(1);
   dib[0] = pat_strt(test->wg_st_addr);
   dib[1] = pat_stop((test->wg_st_addr + test->telm),1);
   dib[2] = pat_scale(ll,(int) a);  /* loops and amplitude */
   dib[3] = pat_wait_hs;
/* --------------------------------------- */
/* pf is in tenth of degrees               */
/* xx is time for each resolution step     */
/* --------------------------------------- */
   xx = (w/90.0) * ((double) pf)/ 10.0;
   dectb = wgtb(xx,1,WFG_TIME_RESOLUTION);
   dib[4] = pat_time(dectb);
   dib[5] = pat_end;
   /* attach to ib list and emit */
   temp = (struct ib_list *) malloc(sizeof(struct ib_list));
   strcpy(temp->tname,nm);
   temp->width = w;
   temp->amp   = a;
   temp->factor = test->scl_factor;
   temp->iloop = ll;
   temp->bstat = pf; 
   temp->nticks = (dectb + 1) * test->ticks;  /* for spinlocks with the WFG */
   /* 
   pf tells you about the pattern's definition
   */
   temp->apstaddr = hx->cur_ib;
   hx->cur_ib += 6; /* care here */
   dump_ib(hx->ap_base,temp->apstaddr,dib,6);
   temp->ibnext = hx->ib_base;
   hx->ib_base = temp;
   ib_safe(hx);
   return(temp);
}
 
static struct ib_list *create_grad_block(hx,nm,w,a,ll,key)
char *nm;
double w,a;
handle hx;
int ll,key;
{
   int gib[5];
   int ticker;
   /* get pattern info */
   struct p_list *test;
   struct ib_list *temp;
   test = resolve_pattern(hx,nm,WGRADIENT,a);
   if (test == NULL) psg_abort(1);
   gib[0] = pat_strt(test->wg_st_addr);
   gib[1] = pat_stop((test->wg_st_addr + test->telm),1);
   gib[2] = pat_scale(ll,(int) a);  /* loops and amplitude */
   ticker = wgtb(w,test->ticks,1);
   gib[3] = pat_time(ticker);
   gib[4] = pat_end;
   /* check the tick count to see if hardware can keep up */
   if (ticker <  MINGRADTIME)
   {
     text_error("WFG:increments of %s may be too short for gradient hardware!",nm);
   }
   /* attach to ib list and emit */
   temp = (struct ib_list *) malloc(sizeof(struct ib_list));
   strcpy(temp->tname,nm);
   temp->width = w;
   temp->amp   = a;
   temp->factor = 1.0;
   temp->iloop = ll;
   temp->bstat = key; 
   /* 0 is a normal instruction block */
   /* != 0 is a 2d keyed one          */
   temp->apstaddr = hx->cur_ib;
   hx->cur_ib += 5; /* care here */
   dump_ib(hx->ap_base,temp->apstaddr,gib,5);
   temp->ibnext = hx->ib_base;
   hx->ib_base = temp;
   ib_safe(hx);
   return(temp);
}

Gpattern *get_gradient_pattern(nm,elem_cnt,tick_cnt, ctl_flag)
char *nm;
int *elem_cnt,*tick_cnt, *ctl_flag;
{
   FILE *fd;
   struct r_list *rhead;
   struct r_data *raw_data;
   Gpattern   *gradpat_struct,*gptr;
   int cnt;
   if ((int)strlen(nm) < 1)  /* pattern name bad abort */
   {
	/* If null assign default values */
	*tick_cnt = 1;
	*elem_cnt = 1;
	gradpat_struct = (Gpattern *) malloc((2) * sizeof(Gpattern));
	gptr = gradpat_struct;
	gptr->amp = 0.0;
	gptr->time = 1.0;
	gptr->ctrl = 0;
	return(gradpat_struct);
   }
   fd = p__open(nm,WGRADIENT); 
   if (fd == NULL)
      return(NULL);
   rhead = (struct r_list *) malloc(sizeof(struct r_list));
   strcpy(rhead->tname,nm);
   rhead->suffix = WGRADIENT;
   rhead->rdata = NULL;
   rhead->rlink = NULL;
   file_read(fd,rhead);
   fclose(fd); 
   if (rhead == NULL)
   {
         text_error("pattern %s was not found",nm);
         psg_abort(1); 
   }
    *tick_cnt = 0;
    *ctl_flag = 0;
    raw_data = rhead->rdata;
    cnt = *elem_cnt = rhead->nvals;
    rhead->max_val = 0.0;
    gradpat_struct = (Gpattern *) malloc((rhead->nvals+1) * sizeof(Gpattern));
    gptr = gradpat_struct;
    while (cnt--)
    {
	int stat;
	stat = raw_data->nfields;
	gptr->amp = raw_data->f1;
	gptr->time = raw_data->f2;
	if ((stat == 1) || (gptr->time < 1.0))
           gptr->time = 1.0;			 /* default to 1 */
	if (stat < 3){
	   gptr->ctrl = 0;
	}else{
	   *ctl_flag = 1;
	   gptr->ctrl = (int)raw_data->f3;
	}
	*tick_cnt = *tick_cnt + (int)gptr->time;
	raw_data = raw_data->rnext;
	gptr++;
    }

   return(gradpat_struct);
}

/*********************************************
   
   ib_safe checks to see if ib has overwritten
   a template it aborts...
*********************************************/
static void ib_safe(hx)
handle hx;
{
   if ((hx->cur_ib + 5) > hx->cur_tmpl)
   {
      text_error("WFG: out of memory..ABORT");
      psg_abort(1);
   }
}

/*****************************************

   the following routines do inline commands
   and other common functions 
*****************************************/
int wgtb(double tb, int itick, int n)
{
   int j;
   double frac,tx,ty;
   /********************************************************/
   /* tb is in seconds - calculate in n multiples of 50 ns */
   /* add 24.9 nanoseconds to the ENTIRE pattern to remove */
   /* problems of numerical representation 6.0 is 5.99999  */
   /********************************************************/
   ty = tb * 1e6 * 20.0; 
   if (itick < 1) itick = 1; /* insurance */
   tx = (ty + 0.49) /((double) itick); /* tx = wg counts per tick ! */ 
   /* trunc to stay within pattern */
   j = (int) (tx);  
   j /= n;
   j *= n;		
   /* remove rounding for error calcultation */
   frac = (ty - ((double) (j * itick)))/ty;
   if (frac < 0.0) frac *= -1.0;
   if (frac > 0.02) 
   {
     text_error("waveform timing error of %5.2f percent for shape of duration %g usec.",frac*100.0,tb*1e6);
   }
   if (j < 4) 
   {
     abort_message("waveform minimum delay for shape of duration %g usec. is %d ns",
		tb*1e6,(int) (MINWGTIME * 1e+9 + 0.5) );
   }
   return(j-1);
}

/*****************************************
    sets start address register for
    information loading or execution
    which_wg is the apbus base address
    works
*****************************************/

void point_wg(int which_wg, int where_on_wg)
{
    putcode(WG3);
    putcode((codeint) which_wg);
    putcode((codeint) where_on_wg);
    curfifocount += 3; 			/* three ap buss words */
}

void pointgo_wg(int which_wg, int where_on_wg)
{
    putcode(WG3);
    putcode((codeint) which_wg | 0x4);
    putcode((codeint) where_on_wg);
    curfifocount += 3; 			/* three ap buss words */
}

void pointloop_wg(int which_wg, int where_on_wg)
{
    putcode(WG3);
    putcode((codeint) which_wg | 0x6);
    putcode((codeint) where_on_wg);
    curfifocount += 3; 			/* three ap buss words */
}

/*
    issues a command to the wg specified
    which_wg is the apbus base address
    0x00 stop
    0x01 start 
    0x02 infinite loop bit
    0x04 mode
    0x80 reset/abort 
    0x10 command to gradient (optional)
    0x20 command to rf	(optional)
*/    
	     
void command_wg(int which_wg, int cmd_wg)
{
    putcode(WGCMD);
    putcode((codeint) which_wg);
    putcode((codeint) cmd_wg);
    curfifocount += 2; 			/* two ap buss words */
}

void write_wg(int which_wg, int where_on_wg, int what)
{
    putcode(WGILOAD);
    putcode((codeint) which_wg);
    putcode((codeint) where_on_wg);
    putcode((codeint) 1);
    putcode((codeint) ((what >> 16) & 0x0ffff));
    putcode((codeint) (what & 0x0ffff));
    curfifocount += 8; 			/* eight ap buss words */
}

void Vwrite_wg(int which_wg, int instr_tag, codeint vloops,
               int a_const, int incr, codeint vmult)
{
    putcode(WGV3);
    putcode((codeint) which_wg);
    putcode((codeint) ((instr_tag >> 16) & 0x0ff00));
    putcode((codeint) (vloops & 0x00ff));
    putcode((codeint) vmult);
    putcode((codeint) incr);
    putcode((codeint) a_const);
    curfifocount += 5; 			/* five ap buss words */
}

void INCwrite_wg(int APaddr, int loops, int a[], codeint x[])
{
    putcode(WGI3);
    putcode((codeint) APaddr);
    putcode((codeint) ((IB_SCALE >> 16) & 0x0ff00));
    putcode((codeint) (loops & 0x00ff));
    putcode((codeint) x[1]);
    putcode((codeint) a[1]);
    putcode((codeint) x[2]);
    putcode((codeint) a[2]);
    putcode((codeint) x[3]);
    putcode((codeint) a[3]);
    putcode((codeint) a[0]);
    curfifocount += 5; 			/* Five AP bus words */
}

/*  for standardization keys to trees and bases are chars */

int get_wg_apbase(char which)
{  
   int yy;
   switch (which)
   {
     case '\0': case 'X':  case 'x':  yy = 0x0c20; break;
     case '\1': case 'Y':  case 'y':  yy = 0x0c28; break;
     case '\2': case 'Z':  case 'z':  yy = 0x0c30; break;
     case '\3': case 'R':  case 'r':  yy = 0x0c38; break;
     case '\4': case 'O':  case 'o':  yy = wfgen[waveboard[1]].ap_baseaddr;
				      break;
     case '\5': case 'D':  case 'd':  yy = wfgen[waveboard[2]].ap_baseaddr;
				      break;
     case '\6': case 'E':  case 'e':  yy = wfgen[waveboard[3]].ap_baseaddr;
				      break;
     case '\7': case 'F':  case 'f':  yy = wfgen[waveboard[4]].ap_baseaddr;
				      break;
     default:   text_error("unknown reference to get ap base");
		psg_abort(1);
   }
   return(yy);
}


/*-----------------------------------------------
|						|
|	          wfg_remap()/3			|
|						|
|   dev_brd1  -  "observe" WFG board is linked	|
|		 to this RF device		|
|   dev_brd2  -  "1st decoupler" WFG board is	|
|		 linked to this RF device	|
|   dev_brd3  -  "2nd decoupler" WFG board is	|
|		 linked to this RF device	|
|						|
+----------------------------------------------*/
void wfg_remap(dev_brd1, dev_brd2, dev_brd3)
int     dev_brd1,
        dev_brd2,
        dev_brd3;
{
   if ( (dev_brd1 == dev_brd2) || (dev_brd1 == dev_brd3) ||
        (dev_brd2 == dev_brd3) )
   {
      text_error("two WFG boards assigned the same control information\n");
      psg_abort(1);
   }
   else if ( (dev_brd1 < 1) || (dev_brd1 > MAX_RFCHAN_NUM) )
   {
      text_error("improper channel value for WFG control\n");
      psg_abort(1);
   }
   else if ( (dev_brd2 < 1) || (dev_brd2 > MAX_RFCHAN_NUM) )
   {
      text_error("improper channel value for WFG control\n");
      psg_abort(1);
   }
   else if ( (dev_brd3 < 1) || (dev_brd3 > MAX_RFCHAN_NUM) )
   {
      text_error("improper channel value for WFG control\n");
      psg_abort(1);
   }
   else if (wfg_locked)
   {
      text_error("cannot change WFG control informatino now\n");
      psg_abort(1);
   }

   wfg_locked = TRUE;
   waveboard[dev_brd1] = 1;
   waveboard[dev_brd2] = 2;
   waveboard[dev_brd3] = 3;
 
   Root[4].ap_base = wfgen[waveboard[1]].ap_baseaddr;
   Root[5].ap_base = wfgen[waveboard[2]].ap_baseaddr;
   Root[6].ap_base = wfgen[waveboard[3]].ap_baseaddr;
}


static struct pat_root *get_handle(which)
char which;
{
   struct pat_root *yy;
   switch (which)
   {
     case 'X':  case 'x':  yy = &Root[0]; break;
     case 'Y':  case 'y':  yy = &Root[1]; break;
     case 'Z':  case 'z':  yy = &Root[2]; break;
     case 'R':  case 'r':  yy = &Root[3]; break;
     case 'O':  case 'o':  yy = &Root[4]; break;
     case 'D':  case 'd':  yy = &Root[5]; break;
     case 'E':  case 'e':  yy = &Root[6]; break;
     case 'F':  case 'f':  yy = &Root[7]; break;
     default:   text_error("unknown reference to handle");
		psg_abort(1);
   }
   return(yy);
}
