/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/************************************************************************
 * This is the Mercruy-Vx Version of wg.c
 *	Routines that will generate WaveForm Gen patterns that will be
 *	executed in a pulse sequence.
 *************************************************************************/
 
/* 
The WFG for Mercury-Vx is loaded via the APbus. 
Addresses for High band WFG are:
0xAA76	Instruction register
	0xBA01	reset
	0xBA80	start pattern once
	0xBAC0	start and loop forever

0xAA75	Load or execute register (16 bits yyxx)
	0xBAxx  low byte of register
	0xBAyy	high byte of register

0xaa74	Write to memory using 0xAA75 register (autoincrement)
	Takes three x 4 bytes, Low byte first

Instruction block:
31  29 28 27   24 23 		16 15		8 7		0
 0 0 |amplitude scalar (14 bits)|  |- start address (16-bits) --|
 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0   |------- time scalar  -------|
 0 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0   |-- end  address (16 bits) --|


Data block:
31                    20 19                   8 7 6             0
|-- phase (12 bits) ---| |-- ampl (12 bits) --| g |-dwell 7-bit-|
                                                a
                                                t
                                                e
Mmeory: 32 kword (128 kbyte)
	0x0 -- 0x7FFF

Minimum time step is 500 ns. Resolution 25 ns.
*/

/**************************
*      Include Files      *
**************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <string.h>
#include <math.h>
#include "acodes.h"
#include "rfconst.h"
#include "acqparms2.h"
#include "vfilesys.h"
#include "abort.h"

/**************************
*  Externals Declaration  *
**************************/
extern char	fileRFpattern[];/* absolute path name of RF load file */
extern char	filexpath[];	/* ex: $vnmrsys/acqque/exp1.russ.012345 */
extern char	userdir[],systemdir[];

extern double	cur_tpwrf, cur_dpwrf;
extern int	bgflag;		/* print diagnostics if not 0 */
extern int	cardb;		/* hi or low band observe */
extern int	curfifocount;	/* current fifo count */
extern int	hb_dds_bits;

/**************************
*         Defines         *
**************************/
/*------------------------------------*/
/* ap high speed line bit definitions */
/*------------------------------------*/
#define SP1	  1     /* 0x01 flag line, not off board  */
#define RCVR	  2     /* 0x02 receiver HS line          */
#define RFPC180	  4     /* 0x04 low band phase 180 bit    */
#define RFPC90	  8     /* 0x08 low band phase 90 bit     */
#define CTXON	 16     /* 0x10 low band tx gate line     */
#define RFPH180	 32     /* 0x20 high band phase 90 bit    */
#define RFPH90	 64     /* 0x40 high band phase 90 bit    */
#define HTXON	128     /* 0x80 high band tx gate line    */
/* transmit  high speed line bits */
#define DC180	 32      /* decoupler phase 180 bit */
#define DC90	 64      /* decoupler phase 90 bit */
#define DC270	 96      /* decoupler phase 90/180 bits */
#define RFPC270	 12      /* carbon/BB phase 90/180 bits */
#define RFPH270	 96      /* Hydrogen phase 90 bit */

#define WRFPULSE		3
#define WDECOUPLER		4
#define NWGS			2
#define COMMENTCHAR		'#'
#define LINEFEEDCHAR		'\n'
#define MINWGTIME		5.0e-7
#define WFG_OFFSET_DELAY	2.0e-6
#define PRG_START_DELAY		2.0e-6
#define WFG_START_DELAY		7.2e-6
#define WFG_STOP_DELAY		200e-9

#define DECOUPLE		1
#define WFG_TIME_RESOLUTION	( 1 )
#define WFG_PULSE_FINAL_STATE	~(0xFFF00080)

#define MVX_MINWGSETUP		5.0e-7	/* time to allow RF wg's to 	*/
					/* process instruction blks.	*/

#define pat_time(pattime)	(0x80000000 | (pattime  & 0x0ffff))
#define pat_stop(end_addr)	(0x40000000 | (end_addr & 0x07fff))

#define is_y(target)		((target == 'y') || (target == 'Y'))


/**************************
*        Globals          *
**************************/
struct r_data
{
   int   nfields;
   double f1,f2,f3,f4,f5;
   struct r_data *rnext;
};

static struct r_list
{
   char tname[60];
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
  long   *data;
  struct p_list *plink;
}; 

struct ib_list
{
  char tname[60];	/* array no 2D   */
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

/* changes to thsi structure require changes to vwacq.o */
struct gwh 
{  
  unsigned short ap_addr,
		 wg_st_addr,
		 n_pts,
		 gspare; 
};

static double		dpf;
static int		d_in_RF = 0;
static int		read_in_pattern();
static long		*lpntr;
static struct p_list	*resolve_pattern();
static struct p_list	*pat_list_search();
static struct p_list	*pat_file_search();
static struct r_list	*pat_file_found();
static struct r_list	*read_pat_file();
static struct ib_list	*resolve_ib();
static struct ib_list	*create_pulse_block();
static struct ib_list	*create_dec_block();
static unsigned int	prev_phase;
static unsigned int	eps_phase;
static void		ib_safe();


static FILE		*p__open();
static FILE		*global_fw = NULL;


/**********************************************
*  New definitions and declarations to allow  *
*  re-mapping of the waveform generators to   *
*  different RF transmitter boards.           *
**********************************************/


static int              pgdrunning[NWGS];

static int getlineFIO(FILE *fd, char *targ, int mxline);
/*************************************************************
   bookkeeping functions 
   set path names and initialize 
   data structures
*************************************************************/
init_wg_list()
{
int i;

    for (i=0; i < NWGS; i++)
    {
       pgdrunning[i]    = FALSE;
       Root[i].cur_tmpl = 0x07FF0;
       Root[i].cur_ib   = 0;
       Root[i].r__cntrl = 0;
       Root[i].ap_base = get_wg_apbase((char) i); 
       Root[i].p_base   = NULL;
       Root[i].ib_base  = NULL;
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
    global_fw = fopen(fileRFpattern,"w");
    if (global_fw == NULL) 
    {
      text_error("could not create RF segment");
      psg_abort(1);
    }
}

static double get_wfg_scale(dev)
int dev;
{
double val;
   if (dev==TODEV)
      val = cur_tpwrf*64.0;
   else if (dev==DODEV)
      val = cur_dpwrf*64.0;
   else
   {  text_error("get_wfg_scale(): internal error");
      psg_abort(1);
   }
   return(val);
}

/************************************************************* 
      wg_close cleans up the global file 
      should do sanity checks
**************************************************************/
wg_close()
{   
  dump_data();
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
   /* hook into the correct tree */
   if ((R1 = pat_file_found(nn,tp)) == NULL)
   {
      if ((R1 = read_pat_file(nn,tp)) == NULL)
      {
         text_error("pattern %s was not found",nn);
         psg_abort(1); 
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
          ((strcmp(scan->tname,nn) != 0) || (tp != scan->suffix)) )
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
struct r_list	**scan;
struct r_list	*rhead;
int		last_index;

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
    char msg[256];

    phead = (struct p_list *) malloc(sizeof(struct p_list));
    phead->rindex = rhead->index;
    phead->scale_pwrf = 4095;
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
       sprintf(msg,"WFG: pattern %s overruns memory .. ABORT",rhead->tname);
       text_error(msg);
       psg_abort(1);
    }
    phead->wg_st_addr = hertz->cur_tmpl;
    if ((hertz->cur_tmpl - hertz->cur_ib) < 20 )
       text_error("WFG: almost out of memory!!\n");
    return((struct p_list *) phead);
}

/*************************************************************
	file is found, 
	read in and numbers of elements and ticks are 
	tallied, a side chain linked list is formed.
*************************************************************/
file_conv(indat,phead)
struct r_list *indat;
struct p_list *phead;
{
struct r_data *raw_data;
int tick_cnt, elem_cnt,cnt,stat, tmp;
double scl;
int add_words,length;

   add_words = 0;
   tick_cnt  = 0;
   elem_cnt  = 0;

/* decoupler patterns may not end at 0.0 degrees, but loop forever */
/* the first time it increments phase from zero, from then on the  */
/* first increment is from the last phase, resulting in an error   */
/* We therefor (software) loop though the pattern several times,   */
/* then quickly set the phase to zero, and (hardware) loop again   */
   if (indat->suffix == WDECOUPLER)
   {  length = 400/phead->telm + 1;
      length = length * phead->telm;
      phead->data = (long *) malloc( (3*length+1) * sizeof(long));
   }
   else
   {  phead->data = (long *) malloc((phead->telm+2) * sizeof(long));
      length = phead->telm;
   }
   lpntr = phead->data;
   /* This function only scales all of the total power */
   scl = phead->scale_pwrf/indat->max_val;
   phead->scl_factor = scl * indat->tot_val / (phead->telm * indat->max_val);
   prev_phase = 0;
   eps_phase  = 0;
   length = length / phead->telm;
   while (length--)
   {  raw_data = indat->rdata;
      cnt = phead->telm;
      while (cnt--)	/* for a blocks worth of data */
      {  add_words += read_in_pattern(raw_data,&tick_cnt,indat->suffix,scl);
         raw_data = raw_data->rnext;
      }
   }
/* after several (software) loops, force phase back to zero */
   if (indat->suffix == WDECOUPLER)
   {  *lpntr = 0x01 | (chgdeg2b(0.0)<<20);
      add_words++;
      tick_cnt += (*lpntr)&0x7F;
   }
   
   if (indat->suffix == WRFPULSE)
   { /* preserves phase and amplitude */
      *lpntr = ((*(lpntr-1))&WFG_PULSE_FINAL_STATE) | (chgdeg2b(0.0)<<20) ;
      add_words++;
      tick_cnt += (*lpntr)&0x7F;
      lpntr++;
      tmp=eps_phase;
      *lpntr = ((*(lpntr-1))&WFG_PULSE_FINAL_STATE) | (chgdeg2b(0.0)<<20);
      add_words++;
      eps_phase=tmp;
   }
   phead->ticks= tick_cnt;
   phead->telm = add_words;
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

file_read(fd,Vx)
FILE *fd;
struct r_list *Vx;
{
    extern double setdefault();
    char parse_me[MAXSTR];
    float inf1,inf2,inf3,inf4,inf5;
    struct r_data **rlast,*rnew,*rtmp;
    int stat,maxticks=255;

    Vx->nvals = 0;
    Vx->max_val = 0.0;
    Vx->tot_val = 0.0;
    rlast = &(Vx->rdata);
    do
    {
       do				/* until not a comment or blank*/
       {
         stat=getlineFIO(fd,parse_me,MAXSTR);
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
    while (stat != EOF);
}

/************************************************************* 
    find and open for reading the pattern file
*************************************************************/
static FILE *p__open(nn,id)
char *nn;
int id;
{
FILE	*fd;
char	pulsepath[MAXSTR],tag[8],tbf[256];
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

static int read_in_pattern(rdata,ticker,tp,scl)
struct r_data	*rdata;
int		*ticker;
int		tp;
double		scl;
{  
float	scl_amp;
int	if1,if2,if3,if4,if5;
int	nwords;
int	stat;
int	thisticks;
    

   stat = rdata->nfields;
/*   prev_phase = 0; */
   if (tp == WRFPULSE)
   {  /* f1 is phase in degrees f2 is amplitude f3 is duration */
      if1 = chgdeg2b(rdata->f1);		/* 12 bits phase */
      scl_amp = rdata->f2 * scl;
      if2 = ((int) scl_amp) & 0x0FFF;	/* 12 bits amplitude */
      if3 = ((int) rdata->f3) & 0x07F;	/*  7 bits dwell time */
      if4 = ((int) rdata->f4) & 0x01;	/*  1 bit gate */
      /* no zero or negative time states */
      if (if3 < 1) 
        if3 = 1;
      if (stat == 1)
      {  if2 = 0xFFF;
         if3 = 1;			/* default to 1 */
      }
      if (stat == 2)
      {  if3 = 2;			/* default to 1 */
      }
      (*ticker) += if3;
      if (stat < 4)
      {  if4 = 0x80;			/* XMTR Gate bit */
      }
      else
      {  if4 <<= 7;
      }
      *lpntr = ((if1 << 20) | (if2 << 8) | if3 | if4 ); 
      lpntr++;
      return(1);
   }
   if (tp == WDECOUPLER)
   {
   /* 
      first  field - tip angle is multiples of 90 degrees 
      second field - phase angle in degrees  -- default zero
      third  field - amplitude 0-1023        -- default 1023
      fourth field - transmitter gate 0-7    -- default on 1
   */
      thisticks =  (int) (rdata->f1/dpf + 0.5);
      scl_amp = (stat < 3) ? 1023.0 : rdata->f3;
      scl_amp *= scl;
      if3 = (((int) scl_amp) & 0x0FFF);	/* 12 bits amplitude */
      if (stat < 4) 
          if4 =  0x80;
      else
          if4 = (((int) rdata->f4) & 0x01) << 7;
      nwords = 0;
      while (thisticks > 0)
      {  if (thisticks > 127) 
         {
	    thisticks -= 127;
	    if1 = 127;
         }
         else
         {  if1 = thisticks;
            thisticks = 0;
         }
         (*ticker) += if1;
         if (stat < 2)
            if2 = chgdeg2b(0.0);		/* phase zero */
         else
            if2 = chgdeg2b(rdata->f2);		/* 12 bits phase */
         *lpntr = (if1 | (if2 << 20) | (if3<<8) | if4 );
         lpntr++;
         nwords++;
      }
      return(nwords);
   }
}

chgdeg2b(rphase)
/************************************************************************
* chgdeg2b will change the input from degrees to bits.  
* the dds will have a error of 0.05 degree/step, over a pattern
* this will add up.  We try to correct.
*
* Note that the DDS need a phase delta, Ie the phase change from
* the previous phase. But with this 12 bit scheme it also needs
* the phase increment for the frequency step (hb_dds_bits). These
* summed give the bit pattern that should go in the WFG.
* Because the WFG only keeps 12-bits, the hb_dds_bits has roundoff error
* if the error sums to > 20-bits, the phase is changed more
*************************************************************************/
double	rphase;
{
register unsigned int iphase;
   /* make sure 0.0 < rphase < 360.0 */
   rphase = -rphase;
   while (rphase > 360.0)
      rphase -= 360.0;
   while (rphase < 0.0)
      rphase += 360.0;
   /* roundoff rphase to 12 bits */
   iphase = (unsigned int) (rphase/360.0*4096.0);
   /* DDS requires delta-phase */
   iphase -= prev_phase;
   /* 0 < iphase < 4096 */
   while (iphase < 0)
      iphase += 4096;
   iphase %= 4096;
   /* prev_phase is absolute */
   prev_phase += iphase;
   prev_phase %= 4096;
   /* add upper 12 bits of bits already in DDS */
   iphase += hb_dds_bits >> 20;
   /* error = lower 20 bits of DDS */
   eps_phase += (hb_dds_bits&0xFFFFF);
   /* if error > 12th bit, add */
   iphase += (eps_phase>>20);
   /* anything above 20th bit was alrady added in previous line */
   eps_phase &= 0xFFFFF;
   return(iphase);
}

static int getlineFIO(FILE *fd, char *targ, int mxline)
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
     return(EOF);
   else
     return(cnt);
}

/*****************************************
    dump_ib()/4
    adds the ib the the global file
*****************************************/
int dump_ib(apb,dest,arr,numb)
int apb,
    dest,
    numb;
long *arr;
{   
    struct gwh ibh;
    if (global_fw == NULL) 
    {
       text_error("no instruction block file ");
       psg_abort(1);
    }
    ibh.ap_addr   = (short)  apb;
    ibh.wg_st_addr= (short) dest;
    ibh.n_pts     = (short) numb;
    ibh.gspare    = (short) 0xabcd;
    if ((fwrite(&ibh,sizeof(ibh),1,global_fw)) != 1)
    {
      text_error("instruction block write failed");
      psg_abort(1);
    }
    if (fwrite(arr,sizeof(long),numb,global_fw) != numb)
    {
      text_error("instruction block write failed");
      psg_abort(1);
    }
    /* valid data in .RF file */
    d_in_RF = 1;
    return(1);
}

dump_data()   /* template data */
{ struct p_list *uu;
   struct gwh glblh;
   int i,j,terminator;
   terminator = 0xa5b6c7d8;
   if (global_fw == NULL) 
   {
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
         glblh.ap_addr    = get_wg_apbase((char) i);
         glblh.wg_st_addr = uu->wg_st_addr;
         glblh.n_pts      = uu->telm;
         glblh.gspare     = (short) 0xfedc;
/*
	 printf("glbh = %x  %x   %x \n",glblh.ap_addr,glblh.wg_st_addr,glblh.n_pts);
*/
         if ((j= fwrite(&glblh,sizeof(glblh),1,global_fw)) != 1)
         {
	   text_error("error on dump file %d",j);
	   psg_abort(1);
         }
         if ((j = fwrite(uu->data,sizeof(long),uu->telm,global_fw)) != uu->telm)
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
   if ((j = fwrite(&terminator,sizeof(long),1,global_fw)) != 1)
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

/*****************************************************************************
		user interface and instruction block control
/* the linked list structure here contains only internal data */
/* rf phase cycles right 
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
genshaped_pulse(name, width, phase, rx1, rx2, g1,
                        g2, rfdevice)
char	*name;
codeint	phase;
double	width, rx1, rx2, g1, g2;
int	rfdevice;
{          
char		msge[128];
double		amp_factor;
double		get_wfg_scale();
handle          h1;
int		rfindex;
struct ib_list  *sldr;
 
/****************************************
* Check some parameters			* 
/***************************************/
   if ((int)strlen(name) < 1)  /* pattern name bad abort */
   {  text_error("genshaped_pulse(): null pattern name. ABORTING...");
      psg_abort(1);
   }
   if (width <  MINWGTIME)
   {  delay(rx1 + rx2 + WFG_START_DELAY + WFG_STOP_DELAY);
      return;
   }
   if ( (rfdevice != TODEV) && (rfdevice != DODEV) )
   {  sprintf(msge,"shaped_pulse: invalid RF device %d\n",rfdevice);
      text_error(msge);
      psg_abort(1);
   }
   if ( ((rfdevice==TODEV)&&(cardb)) || ((rfdevice==DODEV)&& (! cardb)) ||
        (!is_y(rfwg[0])) )
   {
      G_apshaped_pulse(name,width,phase,rx1,rx2,rfdevice);
      return;
   }
   if ( ((rfdevice==TODEV)&&(cardb)) || ((rfdevice==DODEV)&& (! cardb)) )
      rfindex=1;
   else
      rfindex=0;
   h1 = &Root[rfindex];
   amp_factor = get_wfg_scale(rfdevice);

/**********************************************
*  phase I - produce or get ib start address  *
**********************************************/
   if ( (sldr = resolve_ib(h1, name, width, amp_factor, 1, 0)) == NULL )
   {  if ( (sldr = create_pulse_block(h1, name, width, g1, g2,
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
   setphase90(rfdevice, phase);
 
/***************************
*  phase III - coordinate  *
***************************/
   delay(MVX_MINWGSETUP);	/* wfg overhead time */

   gate(RCVR,TRUE);		/* turn receiver Off */
   if ((rx1-g1-WFG_START_DELAY) > 0.0)
      delay(rx1-g1-WFG_START_DELAY);	/* ROF1 delay        */
   pointgo_wg(h1->ap_base, sldr->apstaddr);
   gate(HTXON,TRUE);
   delay(g1 + width + g2);	/* shape delay       */
   gate(HTXON,FALSE);
   delay(rx2-g2);		/* rof2 delay        */
   gate(RCVR,FALSE);		/* turn receiver On */
/*   correct_phase(0x18,1);	/* correct for phase error 24.15 usec) */
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
genspinlock(name, pw_90, deg_res, phsval, ncycles, rfdevice)
char	*name;
codeint	phsval;
double	pw_90, deg_res;
int	rfdevice, ncycles;
{
char	msge[128];
int	nticks;		/* number of 50ns ticks per spinlock pattern */
double	sltime;		/* exact time of the spin lock in the WFG    */
void	prg_dec_off();

   if ((int)strlen(name) < 1)  /* pattern name bad abort */
   {  text_error("genspinlock(): null pattern name. ABORTING...");
      psg_abort(1);
   }
   if ( (rfdevice != TODEV) && (rfdevice != DODEV) )
   {  sprintf(msge, "genspinlock():  RF channel %d is invalid", rfdevice);
      text_error(msge);
      psg_abort(1);
   }

   if (ncycles < 1)
      return;

   gate(RCVR, TRUE);
   setphase90(rfdevice, phsval);

   nticks = prg_dec_on(name, pw_90, deg_res, rfdevice);

   if (nticks < 0)
   {  sprintf(msge, "genspinlock():  WFG already in use on channel %d\n",
                rfdevice);
      text_error(msge);
      psg_abort(1);
   }
   else if (nticks == 0)
   {  sprintf(msge,
	    "genspinlock():  cannot initiate WFG spin lock on channel %d\n",
		rfdevice);
      text_error(msge);
      psg_abort(1);
   }

   sltime = nticks * ncycles * 2.5e-8;
   delay(WFG_OFFSET_DELAY);

   delay(sltime);

   prg_dec_off(2, rfdevice);
   gate(RCVR, FALSE);
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
int prg_dec_on(name, pw_90, deg_res, rfdevice)
char    *name;
double  pw_90, deg_res;
int     rfdevice;
{
char		msge[128];
double		get_wfg_scale(), amp_factor;
handle		h1;
int		precfield, rfindex, nticks;
struct ib_list	*sldr;

   if ( (rfdevice != TODEV) && (rfdevice != DODEV) )
   {  sprintf(msge,
	"prg_dec_on():  RF channel %d is invalid\n", rfdevice);
      text_error(msge);
      psg_abort(1);
   }
   if ( ((rfdevice==TODEV)&&(cardb)) || ((rfdevice==DODEV)&& (! cardb)) )
      rfindex=1;
   else
      rfindex=0;

   if (!is_y(rfwg[rfindex]))
   {  sprintf(msge,
	    "prg_dec_on(): no waveform generator on channel %d\n", rfdevice);
      text_error(msge);
      text_error("WFG programmable decoupling or spinlocking not allowed");
      psg_abort(1);
   }

   if (pgdrunning[rfindex])
   {
      delay(PRG_START_DELAY);
      return(-1);
   }
 
   h1 = &Root[rfindex];
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
   if ( (sldr = resolve_ib(h1, name, pw_90/90.0, amp_factor, DECOUPLE, precfield))
		== NULL )
   {
      if ( (sldr = create_dec_block(h1, name, pw_90/90.0, amp_factor, DECOUPLE,
		precfield)) == NULL )
      {
         text_error("decoupler: ib create failed!\n");
         psg_abort(1);
      }
   }
 
   pointloop_wg(h1->ap_base, sldr->apstaddr);
   gate(HTXON,TRUE);
   delay(MVX_MINWGSETUP); /* wfg overhead time */
   pgdrunning[rfindex] = TRUE;
   return(sldr->nticks);
}


/*-----------------------------------------------
|						|
|		prg_dec_off()/2			|
|						|
|   stopmode	-   ignored, only 2 works.	|
|		    0 => finish loop  		|
|		    1 => hold down HS line  	|
|		    2 => kill now	 	|
|   rfdevice	-   RF device			|
|						|
+----------------------------------------------*/
void prg_dec_off(stopmode, rfdevice)
int	stopmode;	/* Only one way, ignored */
int	rfdevice;
{
char	msge[128];
int	rfindex;
handle	h1;
 
   if ( (rfdevice != TODEV) && (rfdevice != DODEV) )
   {  sprintf(msge,"prg_dec_off(): RF channel %d is invalid\n", rfdevice);
      text_error(msge);
      psg_abort(1);
   }
   if ( ((rfdevice==TODEV)&&(cardb)) || ((rfdevice==DODEV)&& (! cardb)) )
      rfindex=1;
   else
      rfindex=0;

   gate(HTXON,FALSE);
   if ( (!is_y(rfwg[rfindex])) || (!pgdrunning[rfindex]) )
      return;
 
   h1 = &Root[rfindex];
  
   putcode(APBOUT);
   putcode(1);
   putcode(0xA000 | h1->ap_base + 2);
   putcode(0xB000 | (h1->ap_base&0xF00) | 1);
 
   pgdrunning[rfindex] = FALSE;
}

/*-----------------------------------------------
|						|
|	      pgd_is_running()/1		|
|						|
+----------------------------------------------*/
int pgd_is_running(rfdevice)
int	rfdevice;
{
   if ( ((rfdevice==TODEV)&&(cardb)) || ((rfdevice==DODEV)&& (! cardb)) )
      return( pgdrunning[1] );
   else
      return( pgdrunning[0] );
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

match_ib(sl,nm,wdth,tamp,lloops,key)
struct ib_list *sl;
double wdth, tamp;
int lloops,key;
char *nm;
{  
   int temp;
   temp = (strcmp(sl->tname,nm) == 0);
   temp = temp && (sl->amp   == tamp);
   temp = temp && (sl->width == wdth);
   temp = temp && (sl->iloop == lloops);
   temp = temp && (sl->bstat == key);
/* 
 *   fprintf(stderr,"-----match ib -------\n");
 *   fprintf(stderr,"%s   %s\n",sl->tname,nm); 
 *   fprintf(stderr,"%f   %f\n",sl->amp,tamp); 
 *   fprintf(stderr,"%f   %f\n",sl->width,wdth); 
 *   fprintf(stderr,"%d   %d\n",sl->iloop,lloops); 
 *   fprintf(stderr,"%d   %d\n",sl->bstat,key); 
 *
 */
   return(temp);
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
long tib[10],mark;
/* get pattern info */
struct p_list *test;
struct ib_list *temp;
   test = resolve_pattern(hx,nm,WRFPULSE,4095.0);
   if (test == NULL) psg_abort(1);
   tib[0] = (((int)(amp+0.5)&0x3FFF)<<16) + (test->wg_st_addr & 0xFFFF);
   tib[1] = pat_time(wgtb(w,test->ticks,WFG_TIME_RESOLUTION));
   tib[2] = pat_stop(test->wg_st_addr + test->telm);
   /* amp gate state on */
   mark = 3;
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
   hx->cur_ib += mark;
   dump_ib(hx->ap_base,temp->apstaddr,tib,mark);
   temp->ibnext = hx->ib_base;
   hx->ib_base = temp;
   ib_safe(hx);
   return(temp);
}

static struct ib_list *create_dec_block(hx,nm,w,a,ll,pf)
char	*nm;
double	w,a;
handle	hx;
int	ll,pf;
{
long		dib[6];
int		dectb; 
double		xx;
struct p_list	*test;
struct ib_list	*temp;
   test = resolve_pattern(hx,nm,WDECOUPLER,4095.0);
   if (test == NULL) psg_abort(1);
   dib[0] = ( ((int)a & 0x3FFF)<<16) + (test->wg_st_addr & 0xFFFF);
/* --------------------------------------- */
/* pf is in tenth of degrees               */
/* xx is time for each resolution step     */
/* --------------------------------------- */
   xx = w * ((double) pf)/ 10.0;
   dectb = wgtb(xx,1,WFG_TIME_RESOLUTION);
   dib[1] = pat_time(dectb);
   dib[2] = pat_stop(test->wg_st_addr + test->telm);
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
   hx->cur_ib += 3; /* care here */
   dump_ib(hx->ap_base,temp->apstaddr,dib,3);
   temp->ibnext = hx->ib_base;
   hx->ib_base = temp;
   ib_safe(hx);
   return(temp);
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
int wgtb(tb,itick,n)
double tb;
int itick,n;
{
   int j;
   char tbmsg[64];
   double frac,tx,ty;
   /*********************************************************/
   /* tb is in seconds - calculate in n multiples of 25 ns  */
   /* add 12.49 nanoseconds to the ENTIRE pattern to remove */
   /* problems of numerical representation 6.0 is 5.99999   */
   /*********************************************************/
   ty = tb * 1e6 * 40.0; 
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
     sprintf(tbmsg,"waveform timing error of %5.2f percent for shape of duration %g usec.",frac*100.0,tb*1e6);
     text_error(tbmsg);
   }
   if (j < 4) 
   {
     sprintf( tbmsg, "waveform minimum delay for shape of duration %g usec. is %d ns",
		tb*1e6,(int) (MINWGTIME * 1e+9 + 0.5) );
     text_error(tbmsg);
     psg_abort(1);
   }
   return(j-2);
}

correct_phase(ap_xmtr, n)
int	ap_xmtr, n;
{
unsigned int	error;
   error = n*eps_phase + hb_dds_bits;
   putcode(APBOUT);
   putcode(20);			/* n-1 */
   putcode(0xAA00 | ap_xmtr);
   putcode(0xBA00 | 0x14);
   putcode(0x9A00 | ((error)&0xFF));
   putcode(0xAA00 | ap_xmtr);
   putcode(0xBA00 | 0x15);
   putcode(0x9A00 | ((error>>8)&0xFF));
   putcode(0xAA00 | ap_xmtr);
   putcode(0xBA00 | 0x16);
   putcode(0x9A00 | ((error>>16)&0xFF));
   putcode(0xAA00 | ap_xmtr);
   putcode(0xBA00 | 0x17);
   putcode(0x9A00 | ((error>>24)&0xFF));
   putcode(0xAA00 | ap_xmtr);
   putcode(0xBA00 | 0x1E);
   putcode(0x9A00);
   putcode(0xAA00 | ap_xmtr);
   putcode(0xBA00 | 0x14);
   putcode(0x9A00);
   putcode(0xAA00 | ap_xmtr);
   putcode(0xBA00 | 0x15);
   putcode(0x9A00);
}


/*****************************************
    sets start address register for
    information loading or execution
    which_wg is the apbus base address
    works
*****************************************/

wg_reset()
{
   pgdrunning[0] = FALSE;
   gate(HTXON,FALSE);
   putcode(APBOUT);
   putcode(1);		/* n-1 */
   putcode(0xAA76);
   putcode(0xBA01);	/* reset */
}

pointgo_wg(which_wg,where_on_wg)
int which_wg,
    where_on_wg;
{
   putcode(APBOUT);
   putcode(5);	/* n-1*/
   putcode(0xA000 | (which_wg + 2) );
   putcode(0xB000 | (which_wg & 0xF00) + 0x01);	/* reset */
   putcode(0xA000 | (which_wg + 1) );
   putcode(0xB000 | (which_wg & 0xF00) | ( where_on_wg     & 0xFF) );
   putcode(0xB000 | (which_wg & 0xF00) | ((where_on_wg>>8) & 0xFF) );
   putcode(0x9000 | (which_wg & 0xF00) + 0x80);	/* start */
   curfifocount += 6; 			/* six ap buss words */
}

pointloop_wg(which_wg,where_on_wg)
int which_wg,
    where_on_wg;
{
   putcode(APBOUT);
   putcode(3);	/* n-1*/
   putcode(0xA000 | (which_wg + 1) );
   putcode(0xB000 | (which_wg & 0xF00) | ( where_on_wg     & 0xFF) );
   putcode(0xB000 | (which_wg & 0xF00) | ((where_on_wg>>8) & 0xFF) );
   putcode(0x9000 | (which_wg & 0xF00) + 0xC0);	/* start and loop */
   curfifocount += 4; 			/* four ap buss words */
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
	     
command_wg(which_wg,cmd_wg)
int which_wg,
    cmd_wg;
{
    putcode(WGCMD);
    putcode((codeint) which_wg);
    putcode((codeint) cmd_wg);
    curfifocount += 2; 			/* two ap buss words */
}

/*  for standardization keys to trees and bases are chars */

int get_wg_apbase(which)
char which;
{  
   int yy;
   switch (which)
   {
     case '\0': case 'O':  case 'o':  yy = 0x0A74;
				      break;
     case '\1': case 'D':  case 'd':  yy = 0x0A44;
				      break;
     default:   text_error("unknown reference to get_ap_base()");
		psg_abort(1);
   }
   return(yy);
}

