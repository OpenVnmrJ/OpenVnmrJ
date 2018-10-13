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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#define  HSLINE   24
#define  COMMENT  57
#define  ROTOR    0x80
#define  EXTCLK   0x40
#define  INTRP    0x20
#define  STAG     0x10
#define  CTC      0x08
#define  CLOOP    0x06
#define  SLOOP    0x04
#define  ELOOP    0x02
#define  APBUS    0x01
#define  CTCST    0x0C
#define  CTCEN    0x0A
#define  DELAY    0x00
#define  DBASE    100
#define  DINCR    10
#define  APDATA   29
#define  APDATALEN   8
#define  APCHIP    9
#define  APCHIPLEN 4 
#define  APREG    13
#define  APREGLEN 8 
#define  CHNUM 	  6 
#define  CHNUMP1  7 
#define  TWO_24  16777216.0      /* 2 to_the_power 24 */

#define HS_LINE_MASK    0x07FFFFFF
#define CW_DATA_FIELD_MASK 0x00FFFFFF
#define DATA_FIELD_SHFT_IN_HSW  5
#define DATA_FIELD_SHFT_IN_LSW  27

#define APBUS_CNTRL_BIT 0x100000


#define  MEMSIZE  2048
#define  BINARY	  0
#define  ASCII	  1

static int   n;
static int   max_len; /* the max. length of a line */
static int   bufPtr = 9999;
static int   fbufPtr = 9999;
static int   fifoNum = 0;
static int   fifoWord = 64;
static int   fifoFormat = BINARY;
static int   botmIndex = 1;
static int   scrollIndex = 1;
static int   hilitIndex = 0;
static int   line_gap;
static double totaltime = 0.0;
static int   loopCnt = 0;
static  int   ctcInLoop = 0;
static  int   loopactive = 0;

/*  lock tranceiver values */
int   lkduty, lkrate, lkpulse;
int   lkpower, rcvrpower;
double lkphase, lkfreq;
int   lk_power_table[49] = {
	 0,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  4,
	 4,  4,  5,  6,  6,  7,  8,  9, 10, 11, 13, 14,
	16, 18, 20, 23, 25, 28, 32, 36, 40, 45, 51, 57, 
	64, 72, 80, 90,101,113,128,143,161,180,202,227, 255};

/* power of each channel */

int    obs_preamp;
int    rcvr_amp;
int    rcvr_filter;

static char   infile[128];
static char   outfile[128];
static char   bfifo[98];
static char   comstr[512] = " ";



char   *ctl_names[10] = {"ROTO", " EXT", "INTR", "  ID", " CTC",
			 "STRT", " END", "  AP", "    ", " R/W" };
char   *hs_names[5] = { " 180", "  90", " WFG", "XMTR", "RCVR" };
char   *ap_bits[5] = {"  Bit 55", "  Bit 26", "  Bit  0"};

struct _xmtr_state {
		char *activepattern;
                struct _ib_block *activePatIB;
		double  freq;
		int	fine_attn;
		int	coarse_attn;
		int	dmm;
		int	active;
		int     phase90;
                int     activephase90;	/* phase during a pulse */
		double   phase;
		double   dmf;
  		double   updateTime;
		double   onTime;
		double   offTime;
	        double   phase90ChngTime; /* time which phase90 changed */
		int updated;
		int      amp_sel;
		int      hs_sel;
		int	dec_hl;
		int	mixer_sel;
	};

struct _xmtr_state  xmtr_state[CHNUM+1];
struct _xmtr_state  active_xmtr_state[CHNUM+1];

struct _stm_state {
		int  cntrlReg;
		int rcvrphase;
		int  TagReg;
		long MaxSum;
		long SrcAddr;
		long DstAddr;
		long NP;
		long NT;
  		double   updateTime;
		double   onTime;
		double   offTime;
		int updated;

	};

struct _stm_state stm_state[4];

struct _acquire_state {
                int active;
		int rcvrphase;
		int   loopCnt;
		int   ctcInLoop;
		int   loopactive;
		long   Np;
		double loopTime;
  		double   updateTime;
		double   onTime;
		double   offTime;
		int updated;

	};

struct _acquire_state acquire_state;

static FILE*   patfile;
static  int writepattern;
static  int   activewfg = -1;
static  unsigned int   activeibaddr = 0;
static  unsigned int   activepataddr = 0;

#define TYPE_IB 1
#define TYPE_PAT 2

struct _rfwfg_state {
		char *activepattern;
		int  activepatternindex;
                int numpats;	/* number of patterns loaded into wfg */
                int numibs;	/* number of instruction blocks, may out number patterns do to reuse of patterns */
		char *filenames[512];   /* file names of patterns in this wfg */
		unsigned long ibaddr[512];	/* instr block addr within wfg */
		unsigned long pataddr[512];	/* pat addr in wfg that instr block points to */
		unsigned long loadedpataddr[512];   /* patterns addr i wfg loaded */
		struct _ib_block *patternIBs[512];
		int type;		/* use to indicate whether it's an IB or Pattern being written */
	};

static int rfwfg_msb = 0;
static unsigned long wfgtmpval;

struct _rfwfg_state rfwfg_state[CHNUM+1];

struct _ib_block  {
		      int  msb;		/* flag to indicate which lsb or msb working on */
		      unsigned long tmpval;	/* used to build 16 bit for 2 8bit words */
		      unsigned long start_addr;
		      unsigned long end_addr;
		      unsigned int delaycnt;
		      unsigned int loopcnt;
		      unsigned int ampscaler;
		      unsigned long delaytimecnt;
		      unsigned long pattimecnt;
		      char *patternName;
	         };

static struct _ib_block ib_block;

/*
struct _xmtr_info {
		int	hs_sel;
		int	amp_sel;
		int	dec_hl;
		int	mixer_sel;
		int	fine_attn;
		int	coarse_attn;
		int	dmm;
		int	active;
		double   phase;
		        }
		double   dmf;
	};

struct _xmtr_info  xmtr_info[CHNUM+1];
*/

int    charPtr;
int    errorNum;
char   tmpstr[128];
char   convert_bit_ch();
int    create_info_list();
void   disp_info();
void   draw_title();
void   draw_title2();

static printmod = 3;
static char *wfgfilepath;

double calcDelay(long data);

main(argc, argv)
int	argc;
char	**argv;
{
	int	i;
	char	*p;

	infile[0] = '\0';
        if (argc < 2)
        {
	   printf("\n");
	   printf("This program reads the fifo output file generated from a go('debug2') and prints the translated output.\n");
	   printf("  It can also generate a tabular output of channel information.\n");
	   printf("\n");
	   printf("Usage: fifotrans 'debug2 fifo output file' [output mode (3-full output, 2-condensed, 1-tabular)] [pattern file path]\n");
	   printf("       e.g. fifotran /tmp/test.Fifo 1 /vnmr1/vnmrsys/patfiles \n");
	   printf("\n");
	   exit(0);
        }
        if (argc > 2)
        {
	    printmod = atoi(argv[2]);
        }
        if (argc > 3)
        {
	    wfgfilepath = argv[3];
        }
        else
	   wfgfilepath = ".";
/*
	for (i = 1; i < argc; ++i)
   	{
      	   p = argv[i];
      	   if (*p == '-')
		i++;
	   else
	   {
		sprintf(infile, argv[i]);
		break;
	   }
	}
*/
        max_len = COMMENT + 24;
	fifoNum = 1;
	create_info_list(argv[1]);
}

init_vals()
{
	int	k;

	lkpulse = lkduty = lkrate  = 0;
	lkphase = lkpower = lkfreq = 0;
	rcvr_amp = 0;
	obs_preamp = 0;
	for(k = 0; k < CHNUM+1; k++)
	{
	     xmtr_state[k].activepattern = "none";
	     xmtr_state[k].freq = 0.0;
	     xmtr_state[k].fine_attn = 0;
	     xmtr_state[k].coarse_attn = 0;
	     xmtr_state[k].dmm = 0;
	     xmtr_state[k].phase = 0;
	     xmtr_state[k].phase90 = 0;
	     xmtr_state[k].activephase90 = 0;
	     xmtr_state[k].phase90ChngTime = 0.0;
	     xmtr_state[k].dmf = 0;
	     xmtr_state[k].active = 0;
	     xmtr_state[k].onTime = 0;
	     xmtr_state[k].offTime = 0;
	     xmtr_state[k].updated = 0;
	     xmtr_state[k].dec_hl = 0;
	     xmtr_state[k].mixer_sel = 0;
	     active_xmtr_state[k].activepattern = "none";
	     active_xmtr_state[k].freq = 0.0;
	     active_xmtr_state[k].fine_attn = 0;
	     active_xmtr_state[k].coarse_attn = 0;
	     active_xmtr_state[k].dmm = 0;
	     active_xmtr_state[k].phase = 0;
	     active_xmtr_state[k].phase90 = 0;
	     active_xmtr_state[k].activephase90 = 0;
	     active_xmtr_state[k].phase90ChngTime = 0.0;
	     active_xmtr_state[k].dmf = 0;
	     active_xmtr_state[k].active = 0;
	     active_xmtr_state[k].onTime = 0;
	     active_xmtr_state[k].offTime = 0;
	     active_xmtr_state[k].updated = 0;
	     active_xmtr_state[k].dec_hl = 0;
             rfwfg_state[k].activepattern = "none";
             rfwfg_state[k].numpats = 0;
             rfwfg_state[k].numibs = 0;
	}

        stm_state[0].rcvrphase = 0;
        stm_state[0].cntrlReg = 0;
        stm_state[0].TagReg = 0;
        stm_state[0].MaxSum = 0;
        stm_state[0].SrcAddr = 0;
        stm_state[0].DstAddr = 0;
        stm_state[0].NP = 0;
        stm_state[0].NT = 0;
        stm_state[0].updateTime = 0.0;
        stm_state[0].onTime = 0.0;
        stm_state[0].offTime = 0.0;
        stm_state[0].updated = 0;

        acquire_state.active = 0;
        acquire_state.rcvrphase = 0;
        acquire_state.loopCnt = 0;
        acquire_state.ctcInLoop = 0;
        acquire_state.updateTime = 0.0;
        acquire_state.onTime = 0.0;
        acquire_state.offTime = 0.0;
        acquire_state.updated = 0;
        ib_block.msb = 0;
}

int
create_info_list(char *infile)
{
	int	len, k, x, ret;
        /* FILE   *fd; */
        int fd;
        unsigned char   buf[26];
        unsigned char   buf2[256];
	int  convert_fifo();
        long fifo1a,fifo1b,fifo2a,fifo2b;
        long fifowords[4];
        unsigned long cntrl, data, hsline;
	

        if (strlen(infile) <= 0)
              return(0);

        if ((fd = open(infile, O_RDONLY)) == NULL)
	{
	      perror(infile);
              return(-1);
	}
	init_vals();

        /* prev_node = null; */
     if (printmod == 1)
       printf("T/Acq Abs Time (usec)   channel       fpwr   pwr   qphase  sphase    freq   		start  duration  wfg\n");
       /* printf("Abs Time (usec)   channel   pwr    phase   freq   duration\n");    */
        while ((ret = read(fd,fifowords,4*4)) > 0)
        {
	      len = 0;
	      k = 0;
              translate(fifowords);
              translate(&fifowords[2]);
        }
       if (printmod > 1)
        outputPat();
	return(0);
} 

outputPat()
{
   int index,idx;
   int numpats,numibs;
   long ibaddr, pataddr;
   char *name;

   for(index=0; index < CHNUM; index++)
   {
      numpats = rfwfg_state[index].numpats;
      numibs = rfwfg_state[index].numibs;
      printf("\n wfg %d: ibs=%d, pat=%d\n",index+1,numibs,numpats);

      for(idx=0; idx < numibs; idx++)
      {
	 ibaddr = rfwfg_state[index].ibaddr[idx];
	 pataddr = rfwfg_state[index].pataddr[idx];
         printf("    ibaddr[%d]: 0x%lx, pataddr: 0x%lx\n",idx,ibaddr,pataddr);
      }
      for(idx=0; idx < numpats; idx++)
      {
	 name = rfwfg_state[index].filenames[idx];
	 pataddr = rfwfg_state[index].loadedpataddr[idx];
         printf("    Loaded Pattern: %d  filename: '%s', pat addr: 0x%lx\n",idx,name,pataddr);
      }
   }
}

char
convert_bit_ch(bit_num, buf)
int	bit_num;
char    *buf;
{
	static  char  retdata;
	int	      val;

	val = 0;
	while (bit_num > 0)
	{
	    if (*buf == '1')
		val = val * 2 + 1;
	    else
		val = val * 2;
	    bit_num--;
	    buf++;
	}
	if (val <= 9)
	    retdata = val + '0';
	else
	    retdata = 'A' + val - 10;
	return(retdata);
}
	
	

/*  convert char to binary */
convert_ch_bit(sbuf, buf, num)
char  *sbuf;
char  *buf;
int   num;
{
	int	k, val;
	char	*ch;

    ch = sbuf;
    while (num > 0)
    {
	if (*ch >= 'a')
	   val = 10 + *ch - 'a';
	else if (*ch >= 'A')
	   val = 10 + *ch - 'A';
	else
	   val = *ch - '0';	
	switch (val) {
	   case  0:
		sprintf(buf, "0000");
		break;
	   case  1:
		sprintf(buf, "0001");
		break;
	   case  2:
		sprintf(buf, "0010");
		break;
	   case  3:
		sprintf(buf, "0011");
		break;
	   case  4:
		sprintf(buf, "0100");
		break;
	   case  5:
		sprintf(buf, "0101");
		break;
	   case  6:
		sprintf(buf, "0110");
		break;
	   case  7:
		sprintf(buf, "0111");
		break;
	   case  8:
		sprintf(buf, "1000");
		break;
	   case  9:
		sprintf(buf, "1001");
		break;
	   case  10:
		sprintf(buf, "1010");
		break;
	   case  11:
		sprintf(buf, "1011");
		break;
	   case  12:
		sprintf(buf, "1100");
		break;
	   case  13:
		sprintf(buf, "1101");
		break;
	   case  14:
		sprintf(buf, "1110");
		break;
	   case  15:
		sprintf(buf, "1111");
		break;
	   default:
		fprintf(stderr, "Error: unknown number '%c'\n", ch);
	}
	ch ++;
	buf = buf + 4;
	num --;
    }
}


int
bin2dec(start, len)
int	start, len;
{
	int	val;

	val = 0;
	while (len > 0)
	{
	    if (bfifo[start++] == '1')
		val = val * 2 + 1;
	    else
		val = val * 2;
	    len--;
	}
	return(val);
}


convert_b_d(buf, start, len, total)
char   *buf;
int	start, len;
int	*total;
{
	char    tbuf[16];
	int	val;

	val = 0;
	while (len > 0)
	{
	    if (bfifo[start++] == '1')
		val = val * 2 + 1;
	    else
		val = val * 2;
	    len--;
	}
	sprintf(tbuf, "%d ", val);
	len = strlen(tbuf);
	strcpy(buf, tbuf);
	*total = *total + len;
}


int
cal_delay(buf, start, len, total)
char   *buf;
int	start, len;
int	*total;
{
	char    tbuf[16];
	double	val;
	int	base;

	val = 0;
	while (len > 0)
	{
	    if (bfifo[start++] == '1')
		val = val * 2 + 1;
	    else
		val = val * 2;
	    len--;
	}
/*
	val = val - 7;
*/
	val = val - 5;
	if (val < 0)
	    val = 0;
/*
	val = 100 + val * 10;
	val = 37.5 + val * 12.5;
*/
	val = 100 + val * 12.5;
	base = 0;
	while (val > 1000 && base < 4)
	{
	     base++;
	     val = val / 1000;
	}
	switch (base) {
	   case 0:
		sprintf(tbuf, "%g ns", val);
		break;
	   case 1:
		sprintf(tbuf, "%g us", val);
		break;
	   case 2:
		sprintf(tbuf, "%g ms", val);
		break;
	   case 3:
		sprintf(tbuf, "%g sec", val);
		break;
	}
	strcpy(buf, tbuf);
	len = strlen(tbuf);
	*total = *total + len;
	return(len);
}

double calcDelay(data)
{
    char    tbuf[16];
    double	val;
    double  time;
    int	    base;
    data = data -5;
    if (data < 0)
       data = 0;
    val = 100 + data * 12.5;
    time = val * 1.0E-9;  /* nsec */
    base = 0;
    while (val > 1000 && base < 4)
    {
	base++;
	val = val / 1000;
    }
    switch (base) {
        case 0:
            sprintf(tbuf, "%g ns", val);
            break;
        case 1:
            sprintf(tbuf, "%g us", val);
            break;
        case 2:
            sprintf(tbuf, "%g ms", val);
            break;
        case 3:
            sprintf(tbuf, "%g sec", val);
            break;
    }
    strcpy(comstr, tbuf);
    return(time);
}

#define HS_LINE_MASK    0x07FFFFFF
#define CW_DATA_FIELD_MASK 0x00FFFFFF
#define DATA_FIELD_SHFT_IN_HSW  5
#define DATA_FIELD_SHFT_IN_LSW  27

/* This is the main routine that end up print the results either in
   a 3-verbose to a 1-tabular form (used by Robin's Simulation SW)
*/
translate(long *fifowords)
{
   char *extractName(char*);
   unsigned long cntrl, data, hsline, data2;
   double time,timeon;
    int	k;
   /* printf("0x%8lx 0x%8lx\n",fifowords[0],fifowords[1]); */
   cntrl = fifowords[0] & 0xFF000000;
   cntrl = cntrl >> 24;
   data = ((fifowords[0] & CW_DATA_FIELD_MASK) << DATA_FIELD_SHFT_IN_HSW);
   data2 = (fifowords[1] & ~HS_LINE_MASK);
   data2 = data2 >> DATA_FIELD_SHFT_IN_LSW;
   data = data | data2;
   hsline = fifowords[1] & HS_LINE_MASK;

   if (printmod == 3)
      printf("Cntrl: 0x%2lx, Data: %10ld (0x%8lx), HSline: 0x%8lx : ",
                cntrl,data,data,hsline);

   decodeHS(hsline);
   if (cntrl == 0L)
   {
      time = calcDelay(data);
      totaltime += time;
      if (printmod == 3)
         printf("%s, time = %lf\n",comstr,totaltime);
   }
   else /* if (cntrl == APBUS) */
   {
       ap_decode(cntrl,data);
       /* ap_bus(data); */
      if (printmod == 3)
       printf("%s, time = %lf\n",comstr,totaltime);
   }
   /* decodeHS(hsline); */

   if (printmod > 0)
   {
     for(k = 1; k < CHNUM; k++)
     {
       /* if ( xmtr_state[k].updated == 1)   /* only print if something has changed on the channel */
       if ( (xmtr_state[k].updated == 1) || (active_xmtr_state[k].updated == 1) )   /* only print if something has changed on the channel */
       {
        if (printmod > 1)    /* verbose output generated here.. */
	{
	  printf("\n -------------------------------------------------------- \n");
	  printf("chan %d, pwr: %d(f), %d(c), phase: %d(q) %f(sap), freq: %lf MHz\n",
	       k, xmtr_state[k].fine_attn, xmtr_state[k].coarse_attn, 
	       xmtr_state[k].activephase90, xmtr_state[k].phase, 
	       xmtr_state[k].freq  * 1.0e-6);


          if (active_xmtr_state[k].active == 1)
          {
	     printf(" --- Xmtr ON\n");
	      printf("--- Active  chan %d, pwr: %d(f), %d(c), phase: %d(q) %f(sap), freq: %lf MHz\n",
	       k, active_xmtr_state[k].fine_attn, active_xmtr_state[k].coarse_attn, 
	       active_xmtr_state[k].activephase90, active_xmtr_state[k].phase, 
	       active_xmtr_state[k].freq  * 1.0e-6);
          
            timeon = active_xmtr_state[k].offTime - active_xmtr_state[k].onTime;
            if (timeon > 0.0 )
            {
		 struct _ib_block *ib;

	         ib = active_xmtr_state[k].activePatIB;
                 if ( ib != NULL)
                 {
	            printf("  ---- TimeOn: %lf, Off: %lf, Duration: %lf usec, Pattern: '%s' PatTime: %d nsec\n",
		      active_xmtr_state[k].onTime * 1.0e6, active_xmtr_state[k].offTime * 1.0e6, 
		      timeon * 1.0e6,extractName(ib->patternName),((ib->pattimecnt)+1) * 50);
	         }
		 else
                 {
	            printf("  ---- TimeOn: %lf, Off: %lf, Duration: %lf usec, Pattern: 'none' PatTime: 0 nsec\n",
		    active_xmtr_state[k].onTime * 1.0e6, active_xmtr_state[k].offTime * 1.0e6,
		    timeon * 1.0e6);
                 }
		   

	 	 copyXmtr2ActiveXmtr(k);

                 active_xmtr_state[k].onTime = active_xmtr_state[k].offTime;
	         active_xmtr_state[k].activepattern = "none";
	         active_xmtr_state[k].activePatIB = NULL;
                 active_xmtr_state[k].updated = 0;
             }
          }


#ifdef XXX
	  if ((xmtr_state[k].onTime != 0.0) && (xmtr_state[k].offTime != 0.0))
          {
	     double timeon = xmtr_state[k].offTime - xmtr_state[k].onTime;
           
	      printf("  ---- TimeOn: %lf, Off: %lf, Duration: %lf usec",
		xmtr_state[k].onTime * 1.0e6, xmtr_state[k].offTime * 1.0e6, 
		timeon * 1.0e6);
	      xmtr_state[k].onTime = 0.0;
              printf("   pattern name: '%s'",xmtr_state[k].activepattern);
              /* printf("\nresetting xmtr pattern to none.\n"); */
	      xmtr_state[k].activepattern = "none";
          }
#endif
	  printf("\n");
	  xmtr_state[k].updated = 0;
	  printf("\n -------------------------------------------------------- \n");
        }
        else  /* tabular output generated here */
        {
            if (active_xmtr_state[k].updated == 1)
            {
	      /* time, Trans #, fine atten, coarse atten, phase90, sap, frequency */
	      printf("t %15.4lf\t %4d\t%8d  %4d\t%4d  %4f\t%12.8lf\t",
	        active_xmtr_state[k].updateTime * 1.0e6,
		k, 
		active_xmtr_state[k].fine_attn, active_xmtr_state[k].coarse_attn,
		active_xmtr_state[k].activephase90, active_xmtr_state[k].phase,
		active_xmtr_state[k].freq  * 1.0e-6);

            /* if ((xmtr_state[k].onTime != 0.0) && (xmtr_state[k].offTime != 0.0)) */
            /* if ((active_xmtr_state[k].onTime != 0.0) && (active_xmtr_state[k].offTime != 0.0)) */
               timeon = active_xmtr_state[k].offTime - active_xmtr_state[k].onTime;
               if (timeon > 0.0 )
               {
		 struct _ib_block *ib;

	         ib = active_xmtr_state[k].activePatIB;
                 if ( ib != NULL)
                 {
                   printf("%15.4lf  %8.4lf  %s  %d",
                      active_xmtr_state[k].onTime * 1.0e6,
                      timeon * 1.0e6,extractName(ib->patternName),((ib->pattimecnt)+1) * 50);
		   /*printf("\n active pattern: '%s' & '%s' pattime: %d nsec\n",
			active_xmtr_state[k].activepattern, extractName(ib->patternName), 
			((ib->pattimecnt)+1) * 50 ); */
	         }
		 else
                   printf("%15.4lf  %8.4lf  none  0",
		       active_xmtr_state[k].onTime * 1.0e6,timeon * 1.0e6);
		   

		 copyXmtr2ActiveXmtr(k);

                 active_xmtr_state[k].onTime = active_xmtr_state[k].offTime;
	         active_xmtr_state[k].activepattern = "none";
	         active_xmtr_state[k].activePatIB = NULL;
               }
               else
	         printf("\t 0.0     0.0   none  0");
               active_xmtr_state[k].updated = 0;
               printf("\n");
             }
             else
             {
		if (xmtr_state[k].updated == 1) 
                {
	           /* time, Trans #, fine atten, coarse atten, phase90, sap, frequency */
	           printf("t %15.4lf\t %4d\t%8d  %4d\t%4d  %4f\t%12.8lf\t",
	             xmtr_state[k].updateTime * 1.0e6,
		     k, 
		     xmtr_state[k].fine_attn, xmtr_state[k].coarse_attn,
		     xmtr_state[k].activephase90, xmtr_state[k].phase,
		     xmtr_state[k].freq  * 1.0e-6);
                     xmtr_state[k].updated = 0;
	             printf("\t 0.0     0.0   none  0");
                     xmtr_state[k].updated = 0;
                     printf("\n");
		}
	     }
            /* xmtr_state[k].updated = 0; */

	}
       }
     }
     
     if (acquire_state.updated == 1)
     {
         double timeon;
        if (printmod > 1)
	{
	  printf("\n -------------------------------------------------------- \n");
	  timeon = acquire_state.offTime - acquire_state.onTime;
	  printf("acquire: np %d, receiver phase: %d, ---- TimeOn: %lf, Off: %lf, Duration: %lf usec",
	        acquire_state.Np, acquire_state.rcvrphase, acquire_state.onTime,acquire_state.offTime,timeon* 1.0E6);
	  printf("\n");
	  acquire_state.updated = 0;
	  printf("\n -------------------------------------------------------- \n");
        }
        else
        {
	  /* time, 0, Np, 0, phase, 0.0, 0.0 */
	  timeon = acquire_state.offTime - acquire_state.onTime;
	    printf("a %15.4lf\t %4d\t%8d  %4d\t%4d  %4f\t%12.8lf\t",
	        acquire_state.updateTime * 1.0e6,
		0, 
		acquire_state.Np, 0,
		acquire_state.rcvrphase, 0.0,
		0.0);

               printf("%15.4lf  %8.4lf none  0",
                acquire_state.onTime * 1.0e6,
                timeon*1.0E6);
                acquire_state.onTime = 0.0;
            printf("\n");
            acquire_state.updated = 0;

	}
       }
     }
}

char *extractName(char *path)
{
   char *nameptr;
   nameptr = (char*)strrchr(path,'/');
   /* printf("path: '%s', name: '%s'\n",path,nameptr+1); */
   return(nameptr+1);
}
copyXmtr2ActiveXmtr(int channum)
{
   active_xmtr_state[channum].freq = xmtr_state[channum].freq;
   active_xmtr_state[channum].dmm = xmtr_state[channum].dmm;
   active_xmtr_state[channum].dmf = xmtr_state[channum].dmf;
   active_xmtr_state[channum].amp_sel = xmtr_state[channum].amp_sel;
   active_xmtr_state[channum].hs_sel = xmtr_state[channum].hs_sel;
   active_xmtr_state[channum].dec_hl = xmtr_state[channum].dec_hl;
   active_xmtr_state[channum].mixer_sel = xmtr_state[channum].mixer_sel;
   active_xmtr_state[channum].fine_attn = xmtr_state[channum].fine_attn;
   active_xmtr_state[channum].coarse_attn = xmtr_state[channum].coarse_attn;
   active_xmtr_state[channum].phase = xmtr_state[channum].phase;
   active_xmtr_state[channum].activepattern = xmtr_state[channum].activepattern;

   active_xmtr_state[channum].phase90 =  xmtr_state[channum].phase90;
   active_xmtr_state[channum].activephase90 =  xmtr_state[channum].phase90;
   active_xmtr_state[channum].updateTime =  xmtr_state[channum].updateTime;
   active_xmtr_state[channum].activePatIB =  xmtr_state[channum].activePatIB;
}

decodeHS(unsigned long hsline)
{
    int k;
    /*
    printf("\n\n-------------------------------------------------------------------------------------\n\n");
    */
    for (k = 0; k < CHNUM; k++)
    {
      chkTrans(k,hsline);
    }
    /*
    printf("\n\n-------------------------------------------------------------------------------------\n\n");
    */
}

chkTrans(int physchan,unsigned long hsline)
{
   unsigned long xmtrGateBit,wfgGateBit,phase90Bit,phase180Bit,phase270Bit,phaseMaskBits; 
   unsigned phase90;
   int channum,phaseval,ibindex;

   
   /* transmitters */
   channum = physchan + 1;

      xmtrGateBit =   (1 << ((physchan * 5) + 1));
      wfgGateBit  =   (1 << ((physchan * 5) + 2));
      phase90Bit  =   (1 << ((physchan * 5) + 3));
      phase180Bit =   (1 << ((physchan * 5) + 4));
      phase270Bit =   (3 << ((physchan * 5) + 3));
      phaseMaskBits = (3 << ((physchan * 5) + 3));

   /* printf("\n\nchan: %d, xmtrGate: 0x%8lx, hsline: 0x%8lx\n",channum,xmtrGateBit,hsline); */
   /* printf("Xmtr gate: 0x%lx, Wfg gate: 0x%lx, active=%d\n",(hsline & xmtrGateBit),(hsline & wfgGateBit), 
			active_xmtr_state[channum].active);
   */

   phase90 = (hsline & phaseMaskBits) >> ((physchan * 5) + 3);
   phaseval = decodephs(phase90);
   /* if (((hsline & xmtrGateBit) | (hsline & wfgGateBit)) && (xmtr_state[channum].active == 0)) */
   if (((hsline & xmtrGateBit) | (hsline & wfgGateBit)) && (active_xmtr_state[channum].active == 0))
   {
	xmtr_state[channum].activephase90 =  xmtr_state[channum].phase90 = phaseval;
	xmtr_state[channum].updateTime =  totaltime;

/*
	printf("----- Xmtr: %d ON, onTime= %lf, phase90= %d\n",channum,
		xmtr_state[channum].onTime,xmtr_state[channum].activephase90);
	*/
        /* printf("\n setting xmtr %d pattern = '%s'\n",channum,rfwfg_state[physchan].activepattern); */
        if (hsline & wfgGateBit)
        {
	  xmtr_state[channum].activepattern = rfwfg_state[physchan].activepattern;
	  ibindex = rfwfg_state[physchan].activepatternindex;
	  xmtr_state[channum].activePatIB = rfwfg_state[physchan].patternIBs[ibindex];
        }
        else
        {
          xmtr_state[channum].activepattern = "none";
          xmtr_state[channum].activePatIB = NULL;
        }

        copyXmtr2ActiveXmtr(channum);

        active_xmtr_state[channum].active = 1;
	active_xmtr_state[channum].onTime = totaltime;
	active_xmtr_state[channum].offTime = 0;
	active_xmtr_state[channum].updated = 1;
   }
   else if ((!((hsline & xmtrGateBit) | (hsline & wfgGateBit)) && 
		(active_xmtr_state[channum].active == 1)))
   {
	active_xmtr_state[channum].active = 0;
	active_xmtr_state[channum].offTime = totaltime;
	active_xmtr_state[channum].updated = 1;
	active_xmtr_state[channum].updateTime = totaltime;
   }

   if (phaseval != xmtr_state[channum].phase90)
   {
	/*
       printf("------ Xmtr: %d phase change %d -> %d, Time= %lf\n",channum, 
              xmtr_state[channum].phase90,phaseval,totaltime);
       */
       xmtr_state[channum].phase90 = phaseval;
       xmtr_state[channum].updated = 1;
       xmtr_state[channum].updateTime = totaltime;
       if (active_xmtr_state[channum].active == 1)
       {
	  active_xmtr_state[channum].offTime = totaltime;
          active_xmtr_state[channum].updated = 1;
	 /*
	  printf("------ Xmtr: %d , onTime: %lf, offTime: %lf : duration: %lf\n",
		channum, active_xmtr_state[channum].onTime,active_xmtr_state[channum].offTime,
		active_xmtr_state[channum].offTime-active_xmtr_state[channum].onTime);
	 */
       }
   }
}

decodephs(unsigned long phase)
{
    int phaseval;
    switch(phase)
    {
       case 0x0:
		phaseval = 0;
		break;
       case 0x1:
		phaseval = 90;
		break;
       case 0x2:
		phaseval = 180;
		break;
       case 0x3:
		phaseval = 270;
		break;
    }
    return(phaseval);
}

add_comment (buf)
char  *buf;
{
	char   *comment;
	int    len, ctl_val, rlen;

	ctl_val = 0;
	for (len = 0; len < 8; len++)
	{
	    if (bfifo[len] == '1')
	        ctl_val = ctl_val * 2 + 1;
	    else
	        ctl_val = ctl_val * 2;
	}
	comment = buf + charPtr;
	len = 0;
	if (ctl_val == CLOOP)
	{
		strcpy(comment, "$set loop counter to ");
		len = 21;
		convert_b_d(comment + len, 13, 24, &len);
		charPtr += len;
		return;
	}
	if (bfifo[7] == '0' && bfifo[8] == '1')
	{
		strcpy(comment, " halt ... ");
		len = 10;
		charPtr += len;
		return;
	}

	switch (ctl_val) {
	   case  ROTOR:
		strcpy(comment, " rotor sync");
		len = 11;
		break;
	   case  EXTCLK:
		sprintf(comment, " set extern clock counter to ");
		len = 29;
		convert_b_d(comment + len, 9, 28, &len);
		break;
	   case  INTRP:
		strcpy(comment, " software interrupt ");
		len = 20;
		convert_b_d(comment + len, 33, 4, &len);
		break;
	   case  STAG:
		strcpy(comment, " id tag ");
		len = 8;
		convert_b_d(comment + len, 19, 18, &len);
		break;
	   case  CTC:
		strcpy(comment, " acquire data, and delay ");
		len = 25;
		cal_delay(comment + len, 9, 28, &len);
		break;
	   case  CTCST:
		strcpy(comment, "$acquire data, delay ");
		len = 21;
		comment += len;
		rlen = cal_delay(comment, 9, 28, &len);
		strcpy(comment+rlen, ", start loop ");
		len += 13;
		break;
	   case  CTCEN:
		strcpy(comment, "$acquire data, delay ");
		len = 21;
		comment += len;
		rlen = cal_delay(comment, 9, 28, &len);
		strcpy(comment+rlen, ", end loop ");
		len += 11;
		break;
	   case  SLOOP:
		strcpy(comment, "$start loop, delay ");
		len = 19;
		cal_delay(comment + len, 9, 28, &len);
		break;
	   case  ELOOP:
		strcpy(comment, "$end loop, delay ");
		len = 17;
		cal_delay(comment + len, 9, 28, &len);
		break;
	   case  APBUS:
/*
		strcpy(comment, " Ap bus: ");
		len = 9;
		charPtr += len;
		ap_bus(comment + len);
*/
		ap_bus(comment);
		return;
		break;
	   case  DELAY:
		sprintf(comment, " delay ");
		len = 7;
		cal_delay(comment + len, 9, 28, &len);
		break;
	   default:
		strcpy(comment, "? ??? ");
		len = 6;
		errorNum++;
		break;
	}
	charPtr += len;
}

ap_decode(int ctl_val, unsigned long data)
{
	char   *comment;
        char  tmpstr[256];
        char  tmpstr2[256];
	int    len, rlen;
        double time;

	len = 0;
	if (ctl_val == CLOOP)
	{
		sprintf(comstr, "$set loop counter to %d",data);
		loopCnt = data + 1;
		acquire_state.loopCnt = data + 1; /* count 799 is really a count of 800 */
		return;
	}

	switch (ctl_val) {
	   case  ROTOR:
                if (acquire_state.active == 1)
		{
		   acquire_state.active = 0;
		   acquire_state.offTime = totaltime;
		   acquire_state.updated = 1;
                }
		strcpy(comstr, " rotor sync");
		break;
	   case  EXTCLK:
                if (acquire_state.active == 1)
		{
		   acquire_state.active = 0;
		   acquire_state.offTime = totaltime;
		   acquire_state.updated = 1;
                }
		sprintf(comstr, " set extern clock counter to %d",data);
		break;
	   case  INTRP:
                if (acquire_state.active == 1)
		{
		   acquire_state.active = 0;
		   acquire_state.offTime = totaltime;
		   acquire_state.updated = 1;
                }
		strcpy(comstr, " software interrupt ");
		/* convert_b_d(comstr + len, 33, 4, &len); */
		break;
	   case  STAG:
                if (acquire_state.active == 1)
		{
		   acquire_state.active = 0;
		   acquire_state.offTime = totaltime;
		   acquire_state.updated = 1;
                }
		strcpy(comstr, " id tag ");
		/* convert_b_d(comstr + len, 19, 18, &len); */
		break;
	   case  CTC:
                if (acquire_state.active != 1)
		{
		   acquire_state.active = 1;
		   acquire_state.onTime = totaltime;
		   acquire_state.updateTime = totaltime;
		   acquire_state.rcvrphase = stm_state[0].rcvrphase;
		   acquire_state.Np = stm_state[0].NP;
                }
		strcpy(tmpstr, " acquire data, and delay ");
		time = calcDelay(data);
                strcpy(tmpstr2,comstr);
                strcpy(comstr,tmpstr);
                strcat(comstr,tmpstr2);
                if (loopactive)
  		{
		    ctcInLoop++;
		    acquire_state.ctcInLoop += 1;
		}
                else
		  totaltime += time;
		break;
	   case  CTCST:
                if (acquire_state.active != 1)
		{
		   acquire_state.active = 1;
		   acquire_state.onTime = totaltime;
		   acquire_state.updateTime = totaltime;
		   acquire_state.rcvrphase = stm_state[0].rcvrphase;
		   acquire_state.Np = stm_state[0].NP;
                }
		ctcInLoop = 1;
		loopactive = 1;
		acquire_state.ctcInLoop = 1;
		acquire_state.loopactive = 1;
		strcpy(tmpstr, "$acquire data, delay ");
		time = calcDelay(data);
                strcpy(tmpstr2,comstr);
                strcpy(comstr,tmpstr);
                strcat(comstr,tmpstr2);
		break;
	   case  CTCEN:
		ctcInLoop++;
		loopactive = 0;
		acquire_state.ctcInLoop += 1;
		acquire_state.loopactive = 0;
		strcpy(tmpstr, "$acquire data, delay ");
		time = calcDelay(data);
                strcpy(tmpstr2,comstr);
                strcpy(comstr,tmpstr);
                strcat(comstr,tmpstr2);
 	        totaltime += (time * ctcInLoop) * loopCnt;
                acquire_state.loopTime = (time * ctcInLoop) * loopCnt;
		/* printf("\nCTCEN: time=%lf, inloop: %d, loopcnt: %d\n\n",time,ctcInLoop,loopCnt,totaltime); */
		break;
	   case  SLOOP:
                if (acquire_state.active == 1)
		{
		   acquire_state.active = 0;
		   acquire_state.offTime = totaltime;
		   acquire_state.updated = 1;
                }
		strcpy(comstr, "$start loop, delay ");
		len = 19;
		/* cal_delay(comment + len, 9, 28, &len); */
		break;
	   case  ELOOP:
		strcpy(comment, "$end loop, delay ");
		len = 17;
		/* cal_delay(comment + len, 9, 28, &len); */
		break;
	   case  APBUS:
                if (acquire_state.active == 1)
		{
		   acquire_state.active = 0;
		   acquire_state.offTime = totaltime;
		   acquire_state.updated = 1;
                }
		ap_bus(data);
		return;
		break;
/*
	   case  DELAY:
		sprintf(comment, " delay ");
		len = 7;
		cal_delay(comment + len, 9, 28, &len);
		break;
*/
	   default:
                if (acquire_state.active == 1)
		{
		   acquire_state.active = 0;
		   acquire_state.offTime = totaltime;
		   acquire_state.updated = 1;
                }
		strcpy(comment, "? ??? ");
		len = 6;
		errorNum++;
		break;
	}
}

ap_bus(unsigned long data)
{
	int   reg_num, k, chip_num;
        char outbuf[512];
        totaltime += (100.0 * 1.0e-9);

  /*
	if (bfifo[8] == '1')
	{
	    sprintf(outbuf, "Ap: read ");
            strcpy(comstr,outbuf);
	    charPtr += 9;
	    return;
	}
  */
	chip_num = data >> 24;
	if (chip_num == 7)  /*  AP & F */
	{
	    ap_f(outbuf,data);
            strcpy(comstr,outbuf);
	    return;
	}
	reg_num = (data >> 16) & 0xFF;
	if (chip_num == 0xB)
	{
	    if (reg_num >= 0x90 && reg_num <= 0xCB)
	    {
		xmtr_control(outbuf, reg_num,data & 0xffff);
                strcpy(comstr,outbuf);
		return;
	    }
	    if (reg_num >= 0x50 && reg_num <= 0x57)
	    {
		lock_control(outbuf, reg_num);
                strcpy(comstr,outbuf);
		return;
	    }
	    if (reg_num >= 0x48 && reg_num <= 0x4f)
	    {
		magnet_leg_interface(outbuf, reg_num);
                strcpy(comstr,outbuf);
		return;
	    }
	    if (reg_num >= 0x40 && reg_num <= 0x43)
	    {
		rcvr_interface(outbuf, reg_num);
                strcpy(comstr,outbuf);
		return;
	    }
	    if (reg_num >= 0x30 && reg_num <= 0x3F)
	    {
		amp_route(outbuf, reg_num, data & 0xffff);
                strcpy(comstr,outbuf);
		return;
	    }
	}
	if (chip_num == 0xC)
	{
	    wfggrd(outbuf, data );
            strcpy(comstr,outbuf);
            return;
        }
        if (chip_num == 0xD)
        {
	    if ((reg_num >= 0) && ( reg_num <= 0xf))
  	       strcpy(outbuf,"MSR board");
	    else if ((reg_num >= 0x10) && ( reg_num <= 0x1f))
  	       strcpy(outbuf,"BOB board");
	    else
	       strcpy(outbuf," ???? ");
            strcpy(comstr,outbuf);
            return;
        }
	if (chip_num == 0xE)
	{
             int stm_num = 0;

	    /* sprintf(outbuf, "  STM... "); */
            /* STM_AP_ADR      0x0E00,  STM_AP_ADR2     0x0E20 
	       STM_AP_ADR3     0x0E40,  STM_AP_ADR4     0x0E60
            */
 
	    if (reg_num > 0x5f)  /* E60 */
            {
              stm_num = 3;
	      reg_num -= 0x60;
            }
            else if (reg_num > 0x3f) /* E40 */
            {
              stm_num = 2;
	      reg_num -= 0x40;
            }
            else if (reg_num > 0x1f)  /* E20 */
            {
              stm_num = 1;
	      reg_num -= 0x20;
            }
            else
	      stm_num = 0;

            stm_control(outbuf,stm_num,reg_num,data & 0xffff);
            strcpy(comstr,outbuf);
	    return;
	}
	sprintf(outbuf, "???? ");
        strcpy(comstr,outbuf);
}

ap_f(char *outbuf,long data)
{
	int    		reg_num, k, m, dataVal, ptsno;
	static int      freqIndex = 0, mode;
	static double	freq;
        static int      f1, f2;

	reg_num = (data >> 16) & 0xFF;
	dataVal = data & 0xFF;
	/* printf("ap_f data: 0x%8lx, reg: 0x%x, dataVal: 0x%lx\n",data,reg_num,dataVal); */
	switch (reg_num) {
	  case  0x20:
		freqIndex = 0;
		freq = 0.0;
		if (dataVal == 0)
		{
		   mode = 0;  /* direct mode */
		   sprintf(outbuf, " set frequency in direct mode ");
		}
		else if (dataVal == 4)
		{
		   mode = 1;  /* overrange mode */
		   sprintf(outbuf, " set frequency in overrange mode ");
		}
		else if (dataVal == 0x0C)
		{
		   mode = 2;  /* underrange mode */
		   sprintf(outbuf, " set frequency in underrange mode ");
		}
		else
		{
		   	sprintf(outbuf, " unknown mode %2X ", dataVal);
		}
		break;
	  case  0x21:
		 f1 = 0;
		 f2 = 0;
		/*  data in fifo is reversed */
		 dataVal = ~dataVal; 
		 dataVal &= 0xFF;
                 f1 = dataVal & 0xF;
                 f2 = ( dataVal >> 4 ) & 0xF;
		m = 0;
		switch (freqIndex) {
		   case 0:
		      freq = f1 * 0.1 + f2;
		      break;
		   case 1:
		      freq = freq + f1 * 10 + f2 * 100;
		      break;
		   case 2:
		      freq = freq + f1 * 1000 + f2 * 10000;
		      break;
		   case 3:
		      freq = freq + f1 * 100000 + f2 * 1.0e6;
		      break;
		   case 4:
		      freq = freq + f1 * 10.0e6 + f2 * 100.0e6;
		      break;
		   default:
		      {
			m = 1;
		        sprintf(outbuf, "? Error: over frequency range ");
		      }
		}
                /* printf("ap_f freq: index: %d, f1=%d, f2=%d, freq: %lf\n",
		freqIndex, f1,f2,freq);	
		*/
		freqIndex++;
		if (m == 0)
		{
		   sprintf(tmpstr, "   \(%d\) frequency is %g Hz ", freqIndex,  freq);
		   strcpy(outbuf, tmpstr);
		}
		break;
	  case  0x22:
		if (dataVal > 0)
		{
		   if ((dataVal != 1) & (dataVal != 2))
                   {
		     ptsno = 2;
                     while(1)
                     {
		        dataVal = dataVal >> 1;
			if (dataVal == 1)
			   break;
			ptsno++; 
                     }
                   }
	           else
		     ptsno = dataVal;
		   if (mode == 1)
		        freq = freq + 100000;
		   else if (mode == 2)
		        freq = 100000 - freq;
		   k = 0;
		   if (ptsno <= CHNUM)
                   {
			if (xmtr_state[ptsno].freq != freq)
			{
			   xmtr_state[ptsno].freq = freq;
			   xmtr_state[ptsno].updated = 1;
			   xmtr_state[ptsno].updateTime = totaltime;
			   /* update active if freq changed during pulse active */
       			   if (active_xmtr_state[ptsno].active == 1)
       			   {
	  		     active_xmtr_state[ptsno].offTime = totaltime;
          		     active_xmtr_state[ptsno].updated = 1;
			   }
		        }
                   }
		      
		   if (freq > 1000.0)
		   {
			freq = freq / 1000.0;
			k++;
		   }
		   if (freq > 1000.0)
		   {
			freq = freq / 1000.0;
			k++;
		   }
		   sprintf(outbuf, " set channel %d frequency to ", ptsno);
		   if (k == 0)
		   {
			sprintf(tmpstr, "%g Hz ", freq);
		   }
		   else if (k == 1)
		   {
			sprintf(tmpstr, "%g KHz ", freq);
		   }
		   else
		   {
			sprintf(tmpstr, "%g MHz ", freq);
		   }
		   strcat(outbuf, tmpstr);
		}
		else
		{
		   sprintf(outbuf, " reset Ap latch bits ");
		}
		break;
	   default:
		sprintf(outbuf, "? Error: unknown register 0x%2X ", reg_num);
		break;
	}
}



xmtr_control(outbuf, reg_num, data)
char    *outbuf;
unsigned int	reg_num,data;
{
	static int   chan = 1;
	int	     degree;
	static double dmf1, dmf2, dmf3;
	double phase;

        strcpy(outbuf," ????? ");
	if (reg_num >= 0xC0)
	{
	    reg_num = reg_num - 0xC0;
	    chan = 4;
	}
	else if (reg_num >= 0xB0)
	{
	    reg_num = reg_num - 0xB0;
	    chan = 3;
	}
	else if (reg_num >= 0xA0)
	{
	    reg_num = reg_num - 0xA0;
	    chan = 2;
	}
	else
	{
	    reg_num = reg_num - 0x90;
	    chan = 1;
	}
	switch (reg_num) {
	  case  0:
		/* amp_sel = bin2dec(29, 4) 36 - 28 = 8 */
		/* hs_sel  = bin2dec(33, 4); 36 - 32 = 4 */
		data = data & 0xff;
		xmtr_state[chan].amp_sel = (data >> 4) & 0xF;
		xmtr_state[chan].hs_sel = data & 0xf;
		xmtr_state[chan].updated = 1;
		xmtr_state[chan].updateTime = totaltime;
		if (xmtr_state[chan].hs_sel == 0)
		{
		   sprintf(outbuf, "? Error: bad HS Line selection '0' ");
		   charPtr += 35;
		   errorNum++;
		   return;
		}
		sprintf(outbuf, " xmtr %d use group %d of HSline, ", chan, xmtr_state[chan].hs_sel);
		outbuf += 31;
		if (xmtr_state[chan].amp_sel == 1)
		    sprintf(outbuf, "and high band amp ");
		else
		    sprintf(outbuf, "and low band amp  ");
		charPtr += 18;
		break;
	  case  1:
		/* bin2dec(b,n) == 36 - (b-1) - (n-1)  & n */
		xmtr_state[chan].mixer_sel =  (data >> 8) & 0x1; /*bin2dec(29, 1);*/
		xmtr_state[chan].dec_hl = (data >> 9) & 0x1; bin2dec(28, 1);
		xmtr_state[chan].updated = 1;
		xmtr_state[chan].updateTime = totaltime;
		if (xmtr_state[chan].dec_hl == 1)
		{
		    sprintf(outbuf, " set xmtr %d decoupler to CW mode ", chan);
		}
		else if (xmtr_state[chan].mixer_sel)
		{
		    sprintf(outbuf, " set xmtr %d mixer to high band ", chan);
		}
		else
		{
		    sprintf(outbuf, " set xmtr %d mixer to low band ", chan);
		}
		break;
	  case  2:
		sprintf(outbuf, " overwrite transmitter / receiver pair ");
		break;
	  case  3:
		break;
	  case  4:
		/* degree = bin2dec(29, 8);   36 - 28 = 8 */
		degree = data & 0xFF;
		phase = (double) degree * 0.25;
		if (xmtr_state[chan].phase != phase)
 		{
		   xmtr_state[chan].phase = phase;
		   xmtr_state[chan].updated = 1;
		   xmtr_state[chan].updateTime = totaltime;
                   if (active_xmtr_state[chan].active == 1)
                   {
	              active_xmtr_state[chan].offTime = totaltime;
                      active_xmtr_state[chan].updated = 1;
		   }
 		}
		sprintf(tmpstr, "   (1) small-angle phase %g degree ", xmtr_state[chan].phase);
		strcpy(outbuf, tmpstr);
		break;
	  case  5:
		/* degree = bin2dec(35, 2);  36 - 34 = 2 */
		degree = data & 0x3;
		printf("case 5 xmtr_control\n");
		xmtr_state[chan].phase += (double) degree * 64;
		xmtr_state[chan].updated = 1;
		xmtr_state[chan].updateTime = totaltime;
		if (xmtr_state[chan].phase > 90.0)
		    xmtr_state[chan].phase += 38.0;
                if (active_xmtr_state[chan].active == 1)
                {
	              active_xmtr_state[chan].offTime = totaltime;
                      active_xmtr_state[chan].updated = 1;
		}
		sprintf(tmpstr, "   (2) set xmtr %d phase %g degree ", chan, xmtr_state[chan].phase);
		strcpy(outbuf, tmpstr);
		break;
	  case  6:
		if ( xmtr_state[chan].fine_attn != data)
		{
		   xmtr_state[chan].fine_attn = data;
		   xmtr_state[chan].updated = 1;
		   xmtr_state[chan].updateTime = totaltime;
                   if (active_xmtr_state[chan].active == 1)
                   {
	              active_xmtr_state[chan].offTime = totaltime;
                      active_xmtr_state[chan].updated = 1;
		   }
		}
		sprintf(tmpstr, "   (1) linear amplitude %d ", xmtr_state[chan].fine_attn);
		strcpy(outbuf, tmpstr);
		break;
	  case  7:
		printf("case 7 xmtr_control\n");
		xmtr_state[chan].fine_attn += data * 256;
		xmtr_state[chan].updated = 1;
		xmtr_state[chan].updateTime = totaltime;
                if (active_xmtr_state[chan].active == 1)
                {
	              active_xmtr_state[chan].offTime = totaltime;
                      active_xmtr_state[chan].updated = 1;
		}
		sprintf(tmpstr, "   (2) xmtr %d linear amplitude is %d ", chan, xmtr_state[chan].fine_attn);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	  case  8:
		degree = data;
		dmf1 = (double) degree * 4.76837;
		sprintf(tmpstr, "   (1) xmtr %d DMF %g Hz ", chan, dmf1);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	  case  9:
		/* 36 - 28 = 8 */
		degree = data & 0xff; /* bin2dec(29, 8); */
		dmf2 = (double) degree * 1220.7031;  /* 4.76 * 2^8  */
		sprintf(tmpstr, "   (2) xmtr %d DMF %g Hz ", chan, dmf2 + dmf1);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	  case  10:
		/* 36 - 33 = 3 */
		degree = data & 0x7; /*bin2dec(34, 3); */
		dmf3 = (double) degree * 312500;  /*  4.76 * 2^16 */
		xmtr_state[chan].dmf = dmf3 + dmf2 + dmf1;
		sprintf(tmpstr, "   (3) xmtr %d DMF %g Hz ", chan, xmtr_state[chan].dmf);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	  case  11:
		/* 36 - 32 = 4, 36 - 28 = 8 */
		xmtr_state[chan].dmm = data & 0xf; /* bin2dec(33, 4); */
		xmtr_state[chan].active = (data >> 8) & 0x1; /* bin2dec(29, 1); */
		if (!xmtr_state[chan].active)
		{
		   sprintf(outbuf, " disable xmtr %d DMF ", chan);
		   return;
		}
		else
		{
		   sprintf(outbuf, " enable xmtr %d DMF, set mode to ",chan);
		   charPtr += 32;
		   outbuf += 32;
		   switch (xmtr_state[chan].dmm) {
		      case 0:
		      case 8:
			   strcpy(tmpstr, "CW ");
		           break;
		      case 1:
		      case 9:
			   strcpy(tmpstr, "Square wave ");
		           break;
		      case 2:
		      case 10:
			   strcpy(tmpstr, "FM-FM ");
		           break;
		      case 3:
		      case 11:
			   strcpy(tmpstr, "External ");
		           break;
		      case 4:
		      case 12:
			   strcpy(tmpstr, "Waltz ");
		           break;
		      case 5:
		      case 13:
			   strcpy(tmpstr, "GARP ");
		           break;
		      case 6:
		      case 14:
			   strcpy(tmpstr, "MLEV ");
		           break;
		      case 7:
		      case 15:
			   strcpy(tmpstr, "XY32 ");
		           break;
		   }
		   strcpy(outbuf, tmpstr);
		   charPtr += strlen(tmpstr);
		}
		break;
	  default:
		sprintf(outbuf, "? Error: unknown register %2X ", reg_num);
		charPtr += 29;
		errorNum++;
		break;
	}
}


stm_control(char *outbuf, int stm_num, int reg_num, long data)
{
    int i = stm_num;

    switch(reg_num)
    {
       case 0x0 : 	/*  Control Register */ 
		stm_state[i].cntrlReg = data;
		stm_state[i].rcvrphase = data & 0x3;
    		sprintf(outbuf,"STM %d Cntrl Reg: 0x%lx",i,data);
		stm_state[i].updated = 1;
		stm_state[i].updateTime = totaltime;
		stmcntrlinfo(outbuf,data);
		break;
       case 0x2 : 	/*  Tag Word (16 bits) */ 
	        stm_state[i].TagReg = data;
    		sprintf(outbuf,"STM %d Tag Reg: 0x%lx",i,data);
		stm_state[i].updated = 1;
		stm_state[i].updateTime = totaltime;
		break;

       case 0x4 :	/* Spares */
       case 0x6 :
       case 0x8 :
       case 0xa :
    		sprintf(outbuf,"STM %d Spare Regs",i);
		stm_state[i].updated = 1;
		stm_state[i].updateTime = totaltime;
		break;

       case 0x0c:	/* STM_AP_MAXSUM0  0x0c    Maxsum word 0 */
		stm_state[i].MaxSum = data;
    		sprintf(outbuf,"STM %d LSW MaxSum",i);
		break;

       case 0x0e: 	/* STM_AP_MAXSUM1  0x0e    Maxsum word 1 */
		stm_state[i].MaxSum |=  data << 16;
		stm_state[i].updated = 1;
		stm_state[i].updateTime = totaltime;
    		sprintf(outbuf,"STM %d LSW & MSW MaxSum: %ld (0x%lx)",i,stm_state[i].MaxSum,stm_state[i].MaxSum);
		break;

       case 0x10: 	/* STM_AP_SRC_ADR0 0x10    Source Address low word (16 bits) */
		stm_state[i].SrcAddr = data;
    		sprintf(outbuf,"STM %d LSW SrcAddr",i);
		break;
       case 0x12: 	/* STM_AP_SRC_ADR1 0x12    Source Address High word (16 bits) */
		stm_state[i].SrcAddr |= data << 16;
    		sprintf(outbuf,"STM %d LSW & MSW SrcAddr: 0x%lx",i,stm_state[i].SrcAddr);
		stm_state[i].updated = 1;
		stm_state[i].updateTime = totaltime;
		break;
       case 0x14: 	/* STM_AP_DST_ADR0 0x14    Destination Address low word (16 bits) */
		stm_state[i].DstAddr = data;
    		sprintf(outbuf,"STM %d LSW DstAddr",i);
		break;
       case 0x16: 	/* STM_AP_DST_ADR1 0x16    Destination Address High word (16 bits) */
		stm_state[i].DstAddr |= data << 16;
    		sprintf(outbuf,"STM %d LSW & MSW DstAddr: 0x%lx",i,stm_state[i].DstAddr);
		stm_state[i].updated = 1;
		stm_state[i].updateTime = totaltime;
		break;
       case 0x18: 	/* STM_AP_NP_CNT0  0x18    Remaining # Points low word (16 bits) */
		stm_state[i].NP = data;
    		sprintf(outbuf,"STM %d LSW NP",i);
		break;
       case 0x1a: 	/* STM_AP_NP_CNT1  0x1a    Remaining # Points High word (16 bits) */
		stm_state[i].NP |= data << 16;
    		sprintf(outbuf,"STM %d LSW & MSW NP: %ld (0x%lx)",i,stm_state[i].NP,stm_state[i].NP);
		stm_state[i].updated = 1;
		stm_state[i].updateTime = totaltime;
		break;
       case 0x1c: 	/* STM_AP_NTR_CNT0 0x1c    Remaining # Transients low word (16 bits) */
		stm_state[i].NT = data;
    		sprintf(outbuf,"STM %d LSW NT",i);
		break;
       case 0x1e: 	/* STM_AP_NTR_CNT1 0x1e    Remaining # Transients High word (16 bits) */
		stm_state[i].NP |= data << 16;
    		sprintf(outbuf,"STM %d LSW & MSW NT: %ld (0x%lx)",i,stm_state[i].NT,stm_state[i].NT);
		stm_state[i].updated = 1;
		stm_state[i].updateTime = totaltime;
		break;

    }
}

/*---  STM APbus Control Register Bit Positions ---*/

#define STM_AP_PHASE_CYCLE_MASK  0x3        /* bit 0 - 1 Mode bits */
#define STM_AP_REVERSE_PHASE_CYCLE  0x4     /* bit 2 - Mode bit */
#define STM_AP_MEM_ZERO          0x8    /* bit 3 - Memory zero source data */
#define STM_AP_SINGLE_PRECISION  0x10   /* bit 4 - 1=16 bit, 0=32 bit */
#define STM_AP_ENABLE_ADC1           0x20   /* bit 5 - Enable STM */
#define STM_AP_ENABLE_ADC2           0x40   /* bit 6 - Enable STM */
#define STM_AP_ENABLE_STM            0x80   /* bit 7 - Enable STM */
/* the one bit loads both np and address, loading separately is not an option */
#define STM_AP_RELOAD_NP_ADDRS       0x100  /* bit 8 - Reload np and address*/
/* #define STM_AP_RELOAD_NP          0x100  /* bit 8 - Reload np */
/* #define STM_AP_RELOAD_ADDRS       0x200  /* bit 9 - Reload Addresses */
#define ADM_AP_OVFL_ITRP_MASK     0x800 /* bit 11 - En. HS DTM ADC's OverFlow */
#define STM_AP_RTZ_ITRP_MASK     0x1000 /* bit 12 - En. Remaining transients==0 */
#define STM_AP_RPNZ_ITRP_MASK    0x2000 /* bit 13 - En. Remaining Pts != 0  */
#define STM_AP_MAX_SUM_ITRP_MASK 0x4000 /* bit 14 - Enable Maxsum interrupt */
#define STM_AP_IMMED_ITRP_MASK   0x8000 /* bit 15 - Enable Immediate interrupt */
         
/* ---- STM AP Interrupt Mask Bit Positions (via VME) ------ */
#define AP_RTZ_ITRBIT   0x1
#define AP_RPNZ_ITRBIT  0x2
#define AP_MAX_ITRBIT   0x4
#define AP_IMMED_ITRBIT 0x8
#define VME_RTZ_ITRBIT  RTZ_ITRP_MASK  /* 0x10 */
#define VME_RPNZ_ITRBIT RPNZ_ITRP_MASK  /* 0x20 */
#define VME_MAX_ITRBIT  MAX_SUM_ITRP_MASK  /* 0x40 */
#define VME_IMMED_ITRBIT APBUS_ITRP_MASK  /* 0x80 */
         
  /*   New for 5 MHz High Speed  STM/ADC */
#define AP_ADCOVFL_ITRBIT  0x100
#define VME_ADCOVFL_ITRBIT  0x200
         

stmcntrlinfo(char *outbuf,int data)
{
    char tmpbuf[256];
    int phase = decodephs(data&0x3);
    sprintf(tmpbuf," - phase: %d, ",phase);
    strcat(outbuf,tmpbuf);

    if ( 0x8 & data )
       strcat(outbuf," MemZero, ");
    if ( 0x10 & data )
       strcat(outbuf," Single Prec, ");
    if ( 0x20 & data )
       strcat(outbuf," Enable ADC1, ");
    if ( 0x40 & data )
       strcat(outbuf," Enable ADC2, ");
    if ( 0x80 & data )
       strcat(outbuf," Enable STM, ");
    if ( 0x100 & data )
       strcat(outbuf," Reload Np & Addrs, ");
    if ( 0x800 & data )
       strcat(outbuf," HS Ovflow Itrp Enable, ");
    if ( 0x1000 & data )
       strcat(outbuf," Remaining NT Zero Itrp Enable, ");
    if ( 0x2000 & data )
       strcat(outbuf," Remaining NP Zero Itrp Enable, ");
    if ( 0x4000 & data )
       strcat(outbuf," MaxSum Itrp Enable, ");
    if ( 0x8000 & data )
       strcat(outbuf," Immediate Itrp Enable, ");
}

stmphase(int data)
{
}

wfggrd(char *outbuf, unsigned long data)
{
  int reg_num;

  reg_num = (data >> 16) & 0xff;
  if (reg_num >= 0x10 && reg_num < 0x50)
  {
    rfwfggrd(outbuf,reg_num,data);
  }
  else if (reg_num >= 0x50 && reg_num < 0x60)
  {
     pfg2(outbuf,reg_num,data);
  }
  else if (reg_num >= 0x60 && reg_num < 0x70)
  {
     pfg1(outbuf,reg_num,data);
  }
  else if (reg_num >= 0x88 && reg_num < 0x90)
  {
     triax(outbuf,reg_num,data);
  }
  else
    sprintf(outbuf," C%x - ???? ",reg_num);
}

rfwfggrd(char *outbuf, int reg_num, unsigned long data)
{
/** The Waveform Generator has  ap address of
 * 0xc10 - rf xmtr1
 * 0xc18 - rf dec1
 * 0xc20 - X gradient
 * 0xc28 - Y gradient
 * 0xc30 - Z gradient
 * 0xc38 - R gradient (unused memorial to David Foxall)
 * 0xc40 - rf dec 3
 * 0xc48 - rf dec 4

***********************************
 * This class only deals with a wavegen used for gradients.
 *
 * Register Definitions
 ***********************************
 * Write Control functions
 * Reg 0 (base+0)       - Select start address in Wfg memory
 * Reg 1 (base+1)       - Write data to be stored in Wfg memory
 *                        address automatically increments after each write.
 * Reg 2 (base+2)       - Write control register
 * Reg 3 (base+3)       - Direct output register (for gradient values)
 *
 * Control Register definition
 * bit 0                - Start command
 * bit 1                - infinite loop
 * bit 2                - mode
 * bit 3                - point and go ?
 * bit 7                - board reset
 *
 *



   RF Wfg
  IB_START =   0x00000000;
  IB_STOP =    0x20000000;
  IB_SCALE =   0x40000000;
  IB_DELAYTB = 0x60000000;
  IB_WAITHS =  0x80000000;
  IB_PATTB =   0xa0000000;
  IB_LOOPEND = 0xc0000000;
  IB_SEQEND =  0xe0000000;
  AMPGATE = 0x10000000;


 * ADDR_REG  = 0;
 * LOAD_REG  = 1;
 * CMD_REG   = 2;
 * DIRECT_REG= 3;
 * CMDGO_REG = 4; 
 * CMDLOOP_REG = 6; 
 */

  if (reg_num >= 0x10 && reg_num <= 0x17)
  {
    sprintf(outbuf,"Xmtr 1 WFG");
    activewfg = 0;
    addrfcmd(outbuf,reg_num - 0x10,data);
  }
  else if (reg_num >= 0x18 && reg_num <= 0x1a)
  {
    sprintf(outbuf,"Xmtr 2 WFG");
    activewfg = 1;
    addrfcmd(outbuf,reg_num - 0x18,data);
  }
  else if (reg_num >= 0x40 && reg_num <= 0x47)
  {
    sprintf(outbuf,"Xmtr 3 WFG");
    activewfg = 2;
    addrfcmd(outbuf,reg_num - 0x40,data);
  }
  else if (reg_num >= 0x48 && reg_num <= 0x4a)
  {
    sprintf(outbuf,"Xmtr 4 WFG");
    activewfg = 3;
    addrfcmd(outbuf,reg_num - 0x48,data);
  }
  else if (reg_num >= 0x20 && reg_num <= 0x27)
  {
    sprintf(outbuf,"X Gradient WFG");
    addrfcmd(outbuf,reg_num - 0x20,data);
  }
  else if (reg_num >= 0x28 && reg_num <= 0x29)
  {
    sprintf(outbuf,"Y Gradient WFG");
    addrfcmd(outbuf,reg_num - 0x28);
  }
  else if (reg_num >= 0x30 && reg_num <= 0x37)
  {
    sprintf(outbuf,"Z Gradient WFG");
    addrfcmd(outbuf,reg_num - 0x30,data);
  }
  else if (reg_num >= 0x38 && reg_num <= 0x39)
  {
    sprintf(outbuf,"R Gradient WFG");
    addrfcmd(outbuf,reg_num - 0x38,data);
  }
  else
    sprintf(outbuf," ???? ");
}
 
addrfcmd(char *outbuf,int reg_num,unsigned long data)
{
  char tmpstr[80];
  char name[80];
  char *filename;
  int index,len;
  int cnt,present;
  unsigned long addr;
 /*
 * Reg 0 (base+0)       - Select start address in Wfg memory
 * Reg 1 (base+1)       - Write data to be stored in Wfg memory
 *                        address automatically increments after each write.
 * Reg 2 (base+2)       - Write control register
 */
    if (reg_num == 0)
    {
       addr = data & 0xffff;

       sprintf(tmpstr,"- Select Start Addr: 0x%lx",addr);
       strcat(outbuf,tmpstr);
       /* WARNING, WARNING the constant 0xf000 is am empirical guess, may not hold true for all cases!! */
       if ( addr < 0xf000)  /* add below 0xf000 are instruction blocks */
       {
          int ibindex;

          /* if ib address already been seen */
          activeibaddr = -1;
          present = 0;
          for (cnt = 0; cnt <  rfwfg_state[activewfg].numibs; cnt++)
          {
	     /*printf("%d: IB addr: 0x%lx, ibaddr: 0x%lx, Patadr: 0x%lx\n",cnt,addr,
		rfwfg_state[activewfg].ibaddr[cnt],rfwfg_state[activewfg].pataddr[cnt]); */
	     if (addr <= rfwfg_state[activewfg].ibaddr[cnt])
             {
		present = 1;
		break;
             }
          }
	  rfwfg_state[activewfg].type = TYPE_IB;
          if (present == 0)
          {
	     rfwfg_state[activewfg].numibs++;

             ibindex = rfwfg_state[activewfg].numibs - 1;
	     rfwfg_state[activewfg].ibaddr[ibindex] = addr;
          }
          else
          {
              activeibaddr = addr;	/* since not -1, then this is a selection to start a pattern */
          }
       }
       else
       {
	  rfwfg_state[activewfg].type = TYPE_PAT;
          rfwfg_state[activewfg].numpats++;
          index = rfwfg_state[activewfg].numpats - 1;
          activepataddr = addr;
	  /* see if we are going to load it */
          for (cnt = 0; cnt <  rfwfg_state[activewfg].numpats; cnt++)
          {
	     /* printf("%d: New addr: 0x%lx, Patadr: 0x%lx\n",cnt,
		activepataddr,rfwfg_state[activewfg].loadedpataddr[cnt]); */
	     if (addr == rfwfg_state[activewfg].ibaddr[cnt])
             {
		present = 1;
		break;
             }
          }
	  if (!present)
          {
               if (patfile != NULL)
               {
		  fclose(patfile);
               }
	       /* sprintf(name,"./wfg_%d_pat_0x%lx",activewfg+1,activepataddr); */
	       sprintf(name,"%s/wfg_%d_pat_0x%lx",wfgfilepath,activewfg+1,activepataddr);
               len = strlen(name);
               
	       rfwfg_state[activewfg].filenames[index] = (char*) malloc(len+2);
               rfwfg_state[activewfg].loadedpataddr[index] = activepataddr;
	       strcpy(rfwfg_state[activewfg].filenames[index],name);
               if (index < rfwfg_state[activewfg].numibs)
	       {
		   rfwfg_state[activewfg].patternIBs[index]->patternName = 
			rfwfg_state[activewfg].filenames[index];
               }
	       /* printf("\n>>> pat name: '%s'\n",rfwfg_state[activewfg].filenames[index]); */
	       /* open stream for writing */
	       patfile = fopen(name,"w");	
               if (patfile == NULL)
	       {
		 printf("Error: file '%s' could not be open\n",name);
		 perror("Error: ");
		 exit(1);
	       }
               writepattern = 1;
               fprintf(patfile,"amp  phase   duration ticks\n");
	       
	       /* open stream for writing */
          }
       }
    }
    else if (reg_num == 1)
    {
       if (rfwfg_state[activewfg].type == TYPE_IB)
       {
           strcat(outbuf," - Write IB: ");
	   decodeIB(outbuf,data);
       }
       else if (rfwfg_state[activewfg].type == TYPE_PAT)
       {
           strcat(outbuf," - Write Pattern: ");
	   decodePat(outbuf,data);
       }
    }
    else if (reg_num == 2)
    {
       strcat(outbuf," - Write cntrl Reg:");
       if (data & 0x80)
	 strcat(outbuf," Reset,");
       if (data & 0x2)
	 strcat(outbuf," Infinite Loop,");

       /* printf("Active WFG: %d, active IB: 0x%lx\n",activewfg,activeibaddr); */
       if (data & 0x1)
       {
         present = 0;
	 strcat(outbuf," Start,");
	  /* find ib of this ib addr */
          for (cnt = 0; cnt <  rfwfg_state[activewfg].numibs; cnt++)
          {
	     /*printf("%d: IB addr: 0x%lx, ibaddr: 0x%lx, Patadr: 0x%lx\n",activewfg,cnt,addr,
		rfwfg_state[activewfg].ibaddr[cnt],rfwfg_state[activewfg].pataddr[cnt]);*/
	     if (activeibaddr == rfwfg_state[activewfg].ibaddr[cnt])
             {
		/* get pattern address */
		addr = rfwfg_state[activewfg].pataddr[cnt];
		break;
             }
          }
	  /*printf("match index: %d, pattern match addr: 0x%lx\n",cnt,addr); */
          /* find loaded pattern */
          for (cnt = 0; cnt <  rfwfg_state[activewfg].numpats; cnt++)
          {
	      if (addr == rfwfg_state[activewfg].loadedpataddr[cnt])
              {
         	present = 1;
		filename = rfwfg_state[activewfg].filenames[cnt];
                rfwfg_state[activewfg].activepatternindex = cnt;
              }
	  }
	  /* printf("pat match index; %d, filename: '%s'\n",cnt,filename); */
          if (present == 1)
	  {
 	     sprintf(tmpstr,"- Pattern: '%s'",filename);
             strcat(outbuf,tmpstr);
	     /* assuming wfg 1 is attached to channel 1 */
             /* printf("\n set wfg %d pattern: '%s'\n",activewfg,filename); */
             rfwfg_state[activewfg].activepattern = filename;
	     /* xmtr_state[activewfg].activepattern = filename; */
          }
       }
    }
    else if (reg_num == 3)
       strcat(outbuf," - Direct Output Reg.");
    else
	strcat(outbuf," - unknown Reg.");
}

#define  IB_START       0x0
#define  IB_STOP        0x1
#define  IB_SCALE       0x2
#define  IB_DELAYTB     0x3
#define  IB_WAITHS      0x4
#define  IB_PATTB       0x5
#define  IB_LOOPEND     0x6
#define  IB_SEQEND      0x7

decodeIB(char *outbuf,unsigned long data)
{
    char tmpstr[80];
    unsigned int type;
    /* printf(">>>> raw data: 0x%lx\n", data & 0xffff); */
    if (ib_block.msb == 0)
    {
      ib_block.tmpval = data & 0xffff;
      ib_block.msb = 1;
      /* printf(">>>> tmpval: 0x%lx\n", ib_block.tmpval); */
      return(0);
    }
    else
    {
      ib_block.tmpval |= (data & 0xffff) << 16;;
      /* printf(">>>> tmpval: 0x%lx\n", ib_block.tmpval); */
      ib_block.msb = 0;
    }

    /* printf("\n>>>> tmpval = 0x%lx\n",ib_block.tmpval); */

    type = ib_block.tmpval >> 29;

    /* printf(" ==== type = %d \n",type); */

    if (type == 0)
    {
        int ibindex;
	ib_block.start_addr = (ib_block.tmpval >> 8) & 0xffff;
        sprintf(tmpstr,"pattern addr: 0x%lx",ib_block.start_addr);
        strcat(outbuf,tmpstr);
        ibindex = rfwfg_state[activewfg].numibs - 1;
	rfwfg_state[activewfg].pataddr[ibindex] = ib_block.start_addr; 
    }
    else if (type == 1)
    {
      ib_block.end_addr = (ib_block.tmpval >> 8) & 0xffff;
      ib_block.delaycnt = (ib_block.tmpval & 0xff);
      sprintf(tmpstr,"end addr: 0x%lx, delaycnt: %d",ib_block.end_addr,ib_block.delaycnt);
      strcat(outbuf,tmpstr);
    }
    else if (type == 2)
    {
      ib_block.loopcnt = (ib_block.tmpval >> 16) & 0xff;
      ib_block.ampscaler = ib_block.tmpval & 0xffff;
      sprintf(tmpstr,"loopcnt: %d, scaler: %d",ib_block.loopcnt,ib_block.ampscaler);
      strcat(outbuf,tmpstr);
    }
    else if (type == 4)
    {
      /* wait for HSline trigger */
      strcat(outbuf,"Wait 4 HS Trigger");
    }
    else if (type == 3)
    {
      ib_block.delaycnt = ib_block.tmpval & 0xfffffff;
      sprintf(tmpstr,"delaycnt: %d (%d ns)",ib_block.delaycnt,ib_block.delaycnt*50);
      strcat(outbuf,tmpstr);
    }
    else if (type == 5)
    {
      ib_block.pattimecnt = ib_block.tmpval & 0xfffffff;
      sprintf(tmpstr,"pattern time: %d (%d ns)",ib_block.pattimecnt,ib_block.pattimecnt*50);
      strcat(outbuf,tmpstr);
    }
    else if (type == 7)
    {
      int index;
      struct _ib_block *IB = (struct _ib_block *) malloc(sizeof(ib_block));
      memcpy(IB,&ib_block,sizeof(ib_block));
      /* end of sequence */
      strcat(outbuf,"End of Sequence");
      index = rfwfg_state[activewfg].numibs - 1;

      rfwfg_state[activewfg].patternIBs[index] = IB;
    }
}

decodePat(char *outbuf, unsigned long data)
{
    char tmpstr[80];
    int duration,phase,amp,phaseq;
    int index;
    long addr;
    float rphase;

    if (rfwfg_msb == 0)
    {
      wfgtmpval = data & 0xffff;
      rfwfg_msb = 1;
      /* printf(">>>> tmpval: 0x%lx\n", ib_block.tmpval); */
      return(0);
    }
    else
    {
      wfgtmpval |= (data & 0xffff) << 16;
      rfwfg_msb = 0;
      /* printf(">>>> tmpval: 0x%lx\n", ib_block.tmpval); */
    }

    index = rfwfg_state[activewfg].numpats - 1;
    addr = rfwfg_state[activewfg].pataddr[index];

    /* printf("\n >>>> pattern addr: 0x%lx \n",addr); */


    /* printf("\n >>>>>> wfgtmpval: 0x%lx\n",wfgtmpval); */
    duration = wfgtmpval & 0xff;
    amp = (wfgtmpval >> 8) & 0x3ff;
    phase = (wfgtmpval >> 18) & 0x7ff;
/*
    if (writepattern == 1)
	fprintf(patfile,"phase shfted: 0x%lx\n",phase);
*/

    phaseq = 0;
    if (phase & 0x400) 
       phaseq = 180;
    if (phase & 0x200)
	phaseq += 90;
/*
    if (writepattern == 1)
	fprintf(patfile,"phase90 : %d\n",phaseq);
*/
    phase &= 0x1ff;
/*
    if (writepattern == 1)
	fprintf(patfile,"phase&0x1ff : %d 0x%x \n",phase,phase);
*/
    rphase = (float) phase * 0.25;
    /* phase /= 4; */
/*
    if (writepattern == 1)
	fprintf(patfile,"rphase: %f \n",rphase);
*/
    rphase += phaseq;

    sprintf(tmpstr,"amp: %d, phase: %6.2f, time: %d",amp,rphase,duration);
    strcat(outbuf,tmpstr);

    if (writepattern == 1)
    {
	/* fprintf(patfile,"value: 0x%lx\n",wfgtmpval); */
        fprintf(patfile," %d \t%6.2f \t%d\n",amp,rphase,duration);
    }
}


/*****************************************************************************
* pfg1
* BASE_APADDR_X       =  0x0c60;
* BASE_APADDR_Y       =  0x0c64;
* BASE_APADDR_Z       =  0x0c68;
* BASE_APADDR_R       =  0x0c6c;
* 
* AMP_ADDR_REG        =  0x0;
* AMP_VALUE_REG       =  0x0;
* AMP_RESET_REG       =  0x2;
*
************************************************************************/
pfg1(outbuf,reg_num,data)
{
    if (reg_num == 0x60)
	strcpy(outbuf,"Performa 1 X Value");
    else if (reg_num == 0x62)
	strcpy(outbuf,"Performa 1 X Reset");
    else if (reg_num == 0x64)
	strcpy(outbuf,"Performa 1 Y Value");
    else if (reg_num == 0x66)
	strcpy(outbuf,"Performa 1 Y Reset");
    else if (reg_num == 0x68)
	strcpy(outbuf,"Performa 1 Z Value");
    else if (reg_num == 0x6a)
	strcpy(outbuf,"Performa 1 Z Reset");
    else if (reg_num == 0x6c)
	strcpy(outbuf,"Performa 1 R Value");
    else if (reg_num == 0x6e)
	strcpy(outbuf,"Performa 1 R Reset");
}

/***********************************************************************
* Performa 2 Controller 
*
*               REFERENCE DATA
*
*       ap buss address C50-C53,C54-C57,C58-C5b,C5c-C5f
*                         x       y       z    reserved
*
*       register 0
*               bit 0 - bit 2 address of Highland function
*               bit 3 - set to clear current dac to zero
*       register 1
*               all bits data to dac's
*               successive writes. order
*               9cZA, bcBC,bcDE,bcxx => ZABCDE (ABCDE <- setpoint dac)
*               bcxx causes strobes
*
*       register 2
*               bit 0   enable power stage
*               bit 1   reset?
*
*       register 3      status register
*               bit 0   highland ok ^
*
*       APSEL   0xA000   APWRT 0xB000   APWRTI 0x9000
************************************************************************/
pfg2(outbuf,reg_num,data)
{
}




/***********************************************************
*
*Triax
*
*     0x0c88         DAC data low byte
*     0x0c89         DAC data high byte
*     0x0c8a         DAC control and update 
*     0x0c8b         Amp enable and reset 
*
*   BASE_APADDR_X       =  0x0c88;
*   BASE_APADDR_Y       =  0x0c88;
*   BASE_APADDR_Z       =  0x0c88;
*   BASE_APADDR_R       =  0x0c88;
*
*   VALUE_SELECT_X        =  0x20;
*   VALUE_SELECT_Y        =  0x48;
*   VALUE_SELECT_Z        =  0x90;
*
*   CNTRL_ENABLE_X        =  0x100;
*   CNTRL_ENABLE_Y        =  0x200;
*   CNTRL_ENABLE_Z        =  0x400;
* 
*   AMP_ADDR_REG        =  0x0;
*   AMP_VALUE_REG       =  0x0;
*   AMP_RESET_REG       =  0x2;
* 
*   AMP_RESET_VALUE       =  0x4000;
*
*        case 'x': case 'X':
*            apbaseaddr = BASE_APADDR_X;
*            gradcntrlmask = CNTRL_ENABLE_X;
*            gradvalueaddr = VALUE_SELECT_X;
*            naxis = 0;
*      apreg then select then value
*
*/
triax(char *outbuf,int reg_num,unsigned long data)
{
    if (reg_num == 0x88)
         sprintf(outbuf,"Triax - DAC data low byte");
    else if (reg_num == 0x89)
         sprintf(outbuf,"Triax - DAC data high byte");
    else if (reg_num == 0x8a)
         sprintf(outbuf,"Triax - DAC Control & Update");
    else if (reg_num == 0x8b)
         sprintf(outbuf,"Triax - Amp enable & reset");
}

lock_control(outbuf, reg_num)
char    *outbuf;
int	reg_num;
{
	int	k, p;
	double  ff;

	switch (reg_num) {
	 case  0x50:   /* lk phase  */
		lkphase = (double) bin2dec(29, 8);
		lkphase = lkphase * 360 / 255;
		sprintf(tmpstr, " set lock phase to %g degree ", lkphase);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x51:  /* lk duty, rate  */
		lkduty = bin2dec(33, 4);
		lkrate = bin2dec(31, 2);
		lkpulse = bin2dec(29, 2);
		sprintf(tmpstr, " set lock duty cycle to %d%%, ", lkduty);
		if (lkpulse == 0) /* lk rate depends on bits 4, 5 */
		{
		    if (lkrate == 0)
			strcat(tmpstr, "lock rate 2 KHz ");
		    else if (lkrate == 1)
			strcat(tmpstr, "lock rate 20 Hz ");
		    else if (lkrate == 2)
			strcat(tmpstr, "lock rate 1 Hz ");
		    else
			strcat(tmpstr, "lock rate triggered by HSline ");
		}
		else
		{
		    if (lkpulse == 1)
			strcat(tmpstr, "lock x-on / r-off ");
		    else if (lkpulse == 2)
			strcat(tmpstr, "lock x-off / r-on ");
		    else
			strcat(tmpstr, "lock x-on / r-on  ");
		}
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x52:  /* lk power */
	 case  0x53:  /* rcvr gain */
		p = bin2dec(29, 8);
		k = 0;
		while (k < 50)
		{
		    if (lk_power_table[k] >= p)
			break;
		    k++;
		}
		if (reg_num == 0x52)
		{
		    lkpower = k;
		    sprintf(tmpstr, " set lock power to %d dB ", lkpower);
		}
		else
		{
		    rcvrpower = k;
		    sprintf(tmpstr, " set receiver gain to %d dB ", rcvrpower);
		}
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x54:  /* lk freq */
		strcpy(outbuf, "   lock frequency \(byte 1\) ");
		charPtr += 27;
		break;
	 case  0x55:
		strcpy(outbuf, "   lock frequency \(byte 2\) ");
		charPtr += 27;
		break;
	 case  0x56:
		lkfreq = lkfreq * 40000000 / TWO_24;
		ff = lkfreq;
		k = 0;
		if (ff > 1000.0)
		{
		   k++;
		   ff = ff / 1000.0;
		}
		if (ff > 1000.0)
		{
		   k++;
		   ff = ff / 1000.0;
		}
		if (k == 0)
		   sprintf(tmpstr, " set lock frequency to %g Hz ", ff);
		else if (k == 1)
		   sprintf(tmpstr, " set lock frequency to %g KHz ", ff);
		else
		   sprintf(tmpstr, " set lock frequency to %g MHz ", ff);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x57:
		strcpy(outbuf,"Lock Controller Status Reg.");
		break;
	}

}


magnet_leg_interface(outbuf, reg_num)
char    *outbuf;
int	reg_num;
{
	int	k, k2, x;

	switch (reg_num) {
	 case  0x48:   /* preamp */
		k = bin2dec(29, 8);
		if (k == 1)
		    sprintf(tmpstr, " xmtr/rcvr preamp will follow ATG1 ");
		else if (k == 2)
		    sprintf(tmpstr, " xmtr/rcvr preamp will follow ATG2 ");
		else
		{
		    sprintf(tmpstr, "? Error: bad number 0x%X for preamp gate ", k);
		    errorNum++;
		}
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x49:   /* relay */
		if (bfifo[36] == '0')
		    sprintf(tmpstr, " relay select high band, ");
		else
		    sprintf(tmpstr, " relay select low band, ");
		if (bfifo[35] == '0')
		    strcat(tmpstr, "mixer is high band, ");
		else
		    strcat(tmpstr, "mixer is low band, ");
		if (bfifo[34] == '0')
		    strcat(tmpstr, "high band from chan 3, ");
		else
		    strcat(tmpstr, "low band from chan 3, ");
		if (bfifo[33] == '0')
		    strcat(tmpstr, "high band from chan 4, ");
		else
		    strcat(tmpstr, "low band from chan 4, ");
		if (bfifo[32] == '0')
		    strcat(tmpstr, "LO from chan 1 ");
		else
		    strcat(tmpstr, "LO from chan 2 ");
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	
	 case  0x4A:   /* preamp */
		obs_preamp = 0;
		if (bfifo[35] == '1')
		    obs_preamp = 12;
		if (bfifo[36] == '1')
		    obs_preamp += 12;
		sprintf(outbuf, " set obs preamp attn to %2d dB ", obs_preamp);
		charPtr += 30;
		break;
	 case  0x4D:   /* solids amp */
		k = bin2dec(29, 4);
		k2 = bin2dec(33, 4);
		if ((k != 14 && k != 2) || (k2 != 14 && k2 != 2))
		{
		    sprintf(outbuf, "? Error:");
		    charPtr += 8;
		    outbuf += 8;
		}
		if (k == 14)  /* 0xE */
		    sprintf(tmpstr, " set solids amp low band to CW mode, ");
		else if (k == 2)
		    sprintf(tmpstr, " set solids amp low band to PULSE mode, ");
		else
		{
		    sprintf(tmpstr, " bad number 0x%X for low band, ",k);
		    errorNum++;
		}

		if (k2 == 14)
		    strcat(tmpstr, "high band to CW mode ");
		else if (k2 == 2)
		    strcat(tmpstr, "high band to PULSE mode ");
		else
		{
		    strcpy(outbuf, tmpstr);
		    x = strlen(tmpstr);
		    charPtr += x;
		    outbuf += x;
		    sprintf(tmpstr, "bad number 0x%X for high band ", k);
		    errorNum++;
		}
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x4E:   /* user bits  */
		k = bin2dec(29, 8);
		sprintf(outbuf, " set user bits to 0x%2X  ", k);
		charPtr += 23;
		break;
	}
}


rcvr_interface(outbuf, reg_num)
char    *outbuf;
int	reg_num;
{
	int	k;
	double  ff;

	switch (reg_num) {
	 case  0x40:   /* audio filter */
		rcvr_filter = 200 * bin2dec(29, 8) + 200;
		ff = (double) rcvr_filter;
		k = 0;
		if (ff > 1000.0)
		{
		   k++;
		   ff = ff / 1000.0;
		}
		if (ff > 1000.0)
		{
		   k++;
		   ff = ff / 1000.0;
		}
		sprintf(outbuf, " set rcvr filter to ");
		charPtr += 20;
		outbuf += 20;
		if (k == 0)
		   sprintf(tmpstr, "%g Hz ",ff);
		else if (k == 1)
		   sprintf(tmpstr, "%g KHz ",ff);
		else
		   sprintf(tmpstr, "%g MHz ",ff);
		strcpy(outbuf, tmpstr);
		charPtr += strlen(tmpstr);
		break;
	 case  0x42:   /* IF attenuator */
		rcvr_amp = 0;
		if (bfifo[31] == '1')
		   rcvr_amp = 2;
		if (bfifo[32] == '1')
		   rcvr_amp += 4;
		if (bfifo[33] == '1')
		   rcvr_amp += 6;
		if (bfifo[34] == '1')
		   rcvr_amp += 12;
		if (bfifo[35] == '1')
		   rcvr_amp += 12;
		rcvr_amp += obs_preamp;
		sprintf(outbuf, " set IF attn to %2d dB ", rcvr_amp);
		charPtr += 22;
		break;
	}
}

amp_route(outbuf, reg_num, data)
char  	*outbuf;
int	reg_num;
int	data;
{
	int	k, chan;

	if (reg_num <= 0x33 || (reg_num >= 0x38 && reg_num <= 0x3B))
	{
	    switch (reg_num) {
	 	case  0x30:   /* channel 4 power */
			chan = 4;
			break;
	 	case  0x31:   /* channel 3 power */
			chan = 3;
			break;
	 	case  0x32:   /* channel 2 power */
	 	case  0x3A:
			chan = 2;
			break;
	 	case  0x33:   /* channel 1 power */
	 	case  0x3B:
			chan = 1;
			break;
	 	case  0x38:   /* channel 6 power */
			chan = 6;
			break;
	 	case  0x39:   /* channel 5 power */
			chan = 5;
			break;
		}
		
		/* k = bin2dec(29, 8); 36 - 28 = 8 */
		k = data & 0xff;
		k = k / 2;
		sprintf(outbuf, " set channel %d coarse attn to %2d dB ", chan, k);
		if (xmtr_state[chan].coarse_attn != k)
		{
		   xmtr_state[chan].coarse_attn = k;
		   xmtr_state[chan].updated = 1;
		   xmtr_state[chan].updateTime = totaltime;
		}
		return;
	}

	switch (reg_num) {
	 case  0x34:
	 case  0x3C:
		set_amp_brick(outbuf, 1);
		break;
	 case  0x35:
		set_amp_brick(outbuf, 3);
		break;
	 case  0x3D:
		set_amp_brick(outbuf, 5);
		break;
	 case  0x36:
		/* k = bin2dec(29, 8); */
		k = data & 0xff;
		sprintf(outbuf, " set register 0x36 to 0x%2X ", k);
		break;
	}
	

}

set_amp_brick(outbuf, chan)
char  	*outbuf;
int     chan;
{
	int   k;

	k = bin2dec(33, 4);
	if (k == 0)
	    sprintf(tmpstr, " turn off amp brick %d,", chan);
	else if (k >= 8)
	    sprintf(tmpstr, " set amp brick %d to CW mode,", chan);
	else
	    sprintf(tmpstr, " set amp brick %d to PULSE mode,", chan);
	k = strlen(tmpstr);
	strcpy(outbuf, tmpstr);
	charPtr += k;
	outbuf += k;
	k = bin2dec(29, 4);
	if (k == 0)
	    sprintf(tmpstr, " turn off amp brick %d ", chan+1);
	else if (k >= 8)
	    sprintf(tmpstr, " set amp brick %d to CW mode ", chan+1);
	else
	    sprintf(tmpstr, " set amp brick %d to PULSE mode ", chan+1);
	strcpy(outbuf, tmpstr);
	charPtr += strlen(tmpstr);
}



int  convert_fifo(char *filename)
{
	char   tmp_file[128];
	int     n, line_len, count, tt, val;
        unsigned int   rdata;
        unsigned char  cdata, vbyte;
	FILE    *fout, *fin;


	sprintf(tmp_file, "%s", tempnam("/tmp", "fifo"));
        if ((fin = fopen(filename, "r")) == NULL)
	{
	      perror(filename);
              return(-1);
	}
        if ((fout = fopen(tmp_file, "w")) == NULL)
              return(-1);
	line_len = fifoWord / 8;
	n = 0;
	while ((rdata = fgetc(fin)) != EOF)
        {
           cdata = rdata;
	   vbyte = 0x80;
	   count = 0;
	   while (count < 2)
	   {
		val = 0;
		for(tt = 0; tt < 4; tt++)
		{
		    if (vbyte & cdata)
			val = val * 2 + 1;
		    else
			val = val * 2;
		    vbyte = vbyte >> 1;
		}
		fprintf(fout, "%1x", val);
		count++;
	   }
           n++;
           if (n == line_len)
           {
		fprintf(fout, "\n");
                n = 0;
           }
        }
        fclose(fout);
        fclose(fin);
	strcpy(filename, tmp_file);
	return(1);
}


