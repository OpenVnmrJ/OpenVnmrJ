/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "REV_NUMS.h"
#include "group.h"
#include "variables.h"
#include "abort.h"
#include "pvars.h"

#ifdef NVPSG
#include "macros.h"
#endif

#ifndef G2000
#include "acqparms.h"
#include "lc.h"
#include "apdelay.h"
#else
#include "acqparms2.h"
#include "lc_gem.h"
#endif

#if !defined(G2000) || defined(MERCURY)
#include "aptable.h"

extern  int     last_table;
#endif
#ifndef G2000
extern void initparms();
extern void initparms_img();
extern int lcsample();
extern void setGlobalDouble(const char *name, double val);
#endif
#include "Pbox_psg.h"
#include "cps.h"

#include "dpsdef.h"

extern  int     grad_flag;
extern  int     dps_flag;
extern  int     checkflag;
extern  int     newacq;
extern  char     **cnames; /* pointer array to variable names */
extern  double   **cvals;  /* pointer array to variable values */

#define    LAST_CODE    555
#define    MAXLEVEL	16
#define	   PLOT		2  /* plot dps */
#define	   REDO		3

#define    PARAMLEN     12 
#define    GVMAX        52
#define    MSLOOP2      (MSLOOP+200)
#ifdef     PELOOP2
#undef     PELOOP2
#endif
#define    PELOOP2      (PELOOP+200)

int	dpsTimer = 0;
static int	dpsOn = 1;
static int  	dpsSkip = 0;
static int  	niFlag = 0;
static int   	ni2Flag = 0;
static int   	ni3Flag = 0;
static int   	gloop = 0;
static int  	loopLevel;
static int  	gotAcquire;
static int  	vFlag;
static int  	totalNum;
static int  	offset_list_id = 256;
static int  	RTval[RTMAX];
static int   	gloopRt[5];
static int      loopRcnt[MAXLEVEL];
static int      loopDo[MAXLEVEL];
static int      tableNum = 0;
static int      ixNum = 2;
static int      outOff = 0;
static int      debugTimer = 0;
static int      rfGroupNum = 0;
static int      shapeListNum = 0;
static int      parallelChan = 0;
static char     dpsfilepath[512];
static char     tmpData[32];
static const char	*grad_label[] = { "x", "y", "z" };
static FILE     *dpsdata;
static double   incDelays[5];
static double  	ssNum;
static double   timers[MAXLEVEL];
static double   parallelTimes[TOTALCH];
static double   totalTime;
static double   rotorTime;
static double   ssTime;

double dps_fval[9];

typedef struct _array_name {
		char *name;
		int   id;
		struct _array_name *next;
	} VNAME_LIST;
static VNAME_LIST *table_list = NULL;

static VNAME_LIST *shape_list = NULL;

typedef struct _array_data {
		char *name;
		int   code;
		int   id;
		int   chan;
		int   size;
		int   type;
		int   xtype;
		double   *vars;
		char   **vstrs;
		struct _array_data *next;
	} ARRAY_DATA;
static ARRAY_DATA *array_list = NULL;
static ARRAY_DATA *com_array_list = NULL;

typedef struct _gvars {
		char  name[PARAMLEN+1];
		double *vaddr;
	} GVARS;

static GVARS  globals[GVMAX];

typedef struct _rtnode {
		int	code, subcode;
		int	vnum, fnum;
		int	vals[4];
		double	width, fval;
		struct _rtnode *next;
	} RTNODE;

static RTNODE  *first_rtnode = NULL;
static RTNODE  *cur_rtnode = NULL;
static RTNODE  *loopStack[MAXLEVEL];

typedef struct r_node {
        char    *name;
        int     code, id;
        int     size;
        double  angle_set[10][3];
        struct  r_node *next;
     } ROTATION_NODE;

static ROTATION_NODE *rotateList = NULL;

typedef struct a_node {
        char    *name;
        int     code, id;
        int     size;
        double  angle_set[12];
        struct  a_node *next;
     } ANGLE_NODE;

static ANGLE_NODE *angleList = NULL;

typedef struct rf_node {
        int     id;
        double  width;
        struct  rf_node *next;
     } RFGROUP_NODE;

static RFGROUP_NODE *rfgrpList = NULL;

extern void x_pulsesequence();
extern void t_pulsesequence();

static void simulate_dps_code(int ct_num);
static void write_dps_array(char *name, int code, int id);
static void set_dps_gvars();
static void set_dps_arrays();
static double cal_dps_timer(int id);

#ifdef AIX
extern  void (*dps_func)();
extern  void (*dps_tfunc)();
#endif

extern void setVs4dps(int start);
#if !defined(G2000) || defined(MERCURY)
extern void inittablevar();
#endif

static void
parse_array(char *arrayStr)
{
        int     k, id, lpr, lp2;
	char	aname[64];
        char    *dptr;

	if (arrayStr == NULL)
	   return;
        dptr = arrayStr;
        while (*dptr == ' ' || *dptr == '\t')
           dptr++;
        k = 0;
        lpr = 0;
        lp2 = 0;
        id = 0;
        while (*dptr != '\0')
        {
	    switch (*dptr) {
	      case ' ':
	      case ',':
	      case '(':
	      case ')':
                	aname[k] = '\0';
                	if (k > 0)
			{
			    if (lpr == 0)
			    {
                    	        write_dps_array(aname, SARRAY, id);
				id++;
			    }
			    else
			    {
                    	        write_dps_array(aname, XARRAY, id);
				lp2 = 1;
			    }
			}
			k = 0;
			if (*dptr == '(')
			{
			    lpr++;
			}
			else if (*dptr == ')')
			{
			    lpr--;
			    if ((lpr == 0) && lp2)
			    {
				id++;
				lp2 = 0;
			    }
			}
			break;
	      default:
                	aname[k++] = *dptr;
			break;
	    }
	    dptr++;
	}
        if (k > 0)
        {
             aname[k] = '\0';
	     if (lpr == 0)
             	write_dps_array(aname, SARRAY, id);
	     else
             	write_dps_array(aname, XARRAY, id);
        }
}

static int
is_ni_name(char *name)
{
	int	ret;

	ret = 0;
	if (*name == 'd' && (int)strlen(name) == 2)
	{
	     if (name[1] == '2' && niFlag)
		ret = 2;
	     else if (name[1] == '3' && ni2Flag)
		ret = 3;
	     else if (name[1] == '4' && ni3Flag)
		ret = 4;
	}
	return(ret);
}

static int
dps_find(char *name)
{
	int	k;

	for (k = 0; k < GVMAX; k++)
	{
		if (strcmp(globals[k].name, name) == 0)
		   return (k);
	}
	return(-1);
}

static void
dps_set(int item, char *name, double val)
{
	if (strcmp(globals[item].name, name) == 0)
	{
		*globals[item].vaddr = val;
	}
}



static void
reset_dps_parms(char *expdir)
{
        char    name[32], dname[64];
	char    tmpstr[128];
        int     type, dindex;
        float   fval;
	FILE    *arrayFd;

        sprintf(dpsfilepath, "%s/%s", expdir, DPS_ARRAY);
        if ((arrayFd = fopen(dpsfilepath, "r")) == NULL)
           return;
        while (fgets(tmpstr, 126, arrayFd) != NULL)
        {
            if (sscanf(tmpstr, "%s%d", name, &type) != 2)
                continue;
            if (type == T_REAL)
            {
		dindex = 0;
                if (sscanf(tmpstr, "%s%d%f%d",name,&type,&fval,&dindex) != 4)
		{
                   if (sscanf(tmpstr, "%s%d%f", name, &type, &fval) != 3)
                        continue;
		}
                if (strcmp(name, "ni") == 0)
		{
                        strcpy(name, "d2");
			d2_index = dindex;
#ifdef G2000
			d2 = fval;
#endif
		}
                else if (strcmp(name, "ni2") == 0)
		{
                        strcpy(name, "d3");
#ifndef G2000
			d3_index = dindex;
#endif
		}
                else if (strcmp(name, "ni3") == 0)
		{
                        strcpy(name, "d4");
#ifndef G2000
			d4_index = dindex;
#endif
		}
                P_setreal(CURRENT,name, fval, 1);
#ifdef G2000
		if (strcmp(name, "d2")== 0)
			d2 = fval;
#endif
            }
            else if (type == T_STRING)
            {
                if (sscanf(tmpstr, "%s%d%s", name, &type, dname) != 3)
                        continue;
                P_setstring(CURRENT,name, dname, 1);
            }
        }
	fsync(fileno(arrayFd));
        fclose(arrayFd);
#ifndef G2000
        initparms();
#endif
}


#if !defined(G2000) || defined(MERCURY)
static void
dump_tbl_info()
{
    int	 n, k, k2, k3, pk, size, divn, *tval;

    for(n = 0; n < MAXTABLE; n++)
    {
	size = Table[n]->table_size;
	if (size > 0)
	{
	   pk = 0;
	   k2 = 24;
	   tval = Table[n]->table_pointer;
	   if (Table[n]->divn_factor > 1)
		divn = Table[n]->divn_factor;
	   else
		divn = 1;
	   size = size * divn;
	   k3 = divn;
	   for (k = 0; k < size; k++)
	   {
	      if (k2 >= 24)
	      {
    	         fprintf(dpsdata, "\n %d %d %d %d 1 %d ", APTBL, n, size,
			Table[n]->auto_inc, pk);
		 k2 = 0;
	      }
	      fprintf(dpsdata, "%d ", *tval);
	      k3--;
	      if (k3 <= 0)
	      {
		 k3 = divn;
		 tval++;
	      }
	      k2++;
	      pk++;
	   }
	   fprintf(dpsdata, "\n");
	}
    }
}
#endif

static void
dump_common_array()
{
	int	    k, vsize, p, n, item;
	double      *data;
	ARRAY_DATA  *clist;
	ROTATION_NODE  *r_node;
        ANGLE_NODE *a_node;


	if (dpsTimer != 0)
	    return;

	clist = com_array_list;
	while (clist != NULL)
	{
	    item = 0;
	    p = 12;
	    vsize = clist->size;
	    data = clist->vars;
            for (k = 0; k < vsize; k++)
	    {
	   	if (p >= 12)
	   	{
       	          fprintf(dpsdata, "\n %d %d %d %d ",clist->code,clist->id, T_REAL,vsize);
       	          fprintf(dpsdata, "%s %d ",clist->name,item);
	          p = 0;
	   	}
                fprintf(dpsdata, "%g  ", data[k]);
	   	p++;
	   	item++;
	    }
     	    fprintf(dpsdata, "\n");
	    if (clist->chan > 0)
     	       fprintf(dpsdata, "%d %d %d %d\n", LISTDEV, clist->code, clist->id, clist->chan);
	    clist = clist->next;
	}
	r_node = rotateList;
        while (r_node != NULL) {
            p = r_node->size;
            if (p > 10)
                 p = 10;
            for (k = 0; k < 3; k++) {
       	         fprintf(dpsdata, "%d %d \"%s\" %d %d %d ",r_node->code,r_node->id,
                         r_node->name, k, r_node->size, p);
                 for (n = 0; n < p; n++) {
       	              fprintf(dpsdata, "%g ",r_node->angle_set[n][k]);
                 }
       	         fprintf(dpsdata, " \n");
            }
            r_node = r_node->next;
        }
	a_node = angleList;
        while (a_node != NULL) {
            p = a_node->size;
            if (p > 10)
                 p = 10;
       	    fprintf(dpsdata, "%d %d \"%s\" %d %d %d ",a_node->code,a_node->id,
                         a_node->name, 0, a_node->size, p);
            for (n = 0; n < p; n++) {
       	        fprintf(dpsdata, "%g ",a_node->angle_set[n]);
            }
       	    fprintf(dpsdata, " \n");
            a_node = a_node->next;
        }
}


static void
add_delay_node(double width)
{
	RTNODE  *new_node;

	new_node = cur_rtnode->next;
	if (new_node == NULL)
	{
	     new_node = (RTNODE *) calloc(1, sizeof(RTNODE));
	     if (new_node == NULL)
		return;
	     new_node->next = NULL;
	     cur_rtnode->next = new_node;
	}
	new_node->code = DELAY;
	new_node->subcode = DELAY;
	new_node->width = width;
	cur_rtnode = new_node;
}


static void
initrtvars()
{
        setVs4dps(V1);
	ct = CT;
	oph = OPH;
	bsval = BSVAL;
	bsctr = BSCTR;
	ssval = SSVAL;
	ssctr = SSCTR;
	zero = ZERO;
	one = ONE;
	two = TWO;
	three = THREE;
	id2 = ID2;
#ifdef NVPSG
	ctss = CTSS;
	ntrt = NTRT;
        npr_ptr = NPR_PTR;
        rttmp = RTTMP;
        spare1rt = SPARE1RT;
#endif
#ifndef G2000
	id3 = ID3;
	id4 = ID4;
#endif
}

static void
dump_dps_power()
{
    if (dpsdata == NULL)
       return;
#ifndef G2000
    fprintf(dpsdata, " %d 1  %f %f\n", RFPWR, tpwr, tpwrf);
    fprintf(dpsdata, " %d 2  %f %f\n", RFPWR, dpwr, dpwrf);
    fprintf(dpsdata, " %d 3  %f %f\n", RFPWR, dpwr2, dpwrf2);
    fprintf(dpsdata, " %d 4  %f %f\n", RFPWR, dpwr3, dpwrf3);
#else
    fprintf(dpsdata, " %d 1  %f %f\n", RFPWR, tpwr, 0);
#endif

}

static void
dump_dps_info()
{
    dump_dps_power();
/**
    fprintf(dpsdata, " %d 1  %d\n", RTADDR, v1);
    fprintf(dpsdata, " %d 15  %d\n", RTADDR, ct);
    fprintf(dpsdata, " %d 16  %d\n", RTADDR, oph);
    fprintf(dpsdata, " %d 17  %d\n", RTADDR, bsval);
    fprintf(dpsdata, " %d 18  %d\n", RTADDR, bsctr);
    fprintf(dpsdata, " %d 19  %d\n", RTADDR, zero);
    fprintf(dpsdata, " %d 20  %d\n", RTADDR, one);
    fprintf(dpsdata, " %d 21  %d\n", RTADDR, two);
    fprintf(dpsdata, " %d 22  %d\n", RTADDR, three);
    fprintf(dpsdata, " %d 23  %d\n", RTADDR, ssval);
    fprintf(dpsdata, " %d 24  %d\n", RTADDR, ssctr);
**/

#if !defined(G2000) || defined(MERCURY)
    fprintf(dpsdata, " %d %d  %d\n", RTADDR, T1, t1);
#endif
#ifndef G2000
    fprintf(dpsdata, " %d  %d\n", NEWACQ, newacq);
#endif
    fprintf(dpsdata, " %d  %d %d %d %d %d\n",RFID,OBSch, DECch, DEC2ch, DEC3ch, DEC4ch);
}

static void
exec_dps_timer()
{
	int	m;
        double  padCount, tempCount;
        vInfo   vinfo;

	totalTime = 0.0;
	ssTime = 0.0;
	for(m = 0; m < RTMAX; m++)
	    RTval[m] = 0;
	
        if(P_getreal(CURRENT,"ss",&ssNum,1) < 0)
	    ssNum = 0.0;
	if (! var_active("ss",CURRENT) )
	    ssNum=0;

	set_dps_gvars();
	set_dps_arrays();
	if (bs <= 0)
	    bs = nt;
	if (np <= 2)
	    np = 512;
	totalNum = 0;
	totalTime = cal_dps_timer(0);
	totalTime += ssTime;
        padCount = 1;
        tempCount = 1;
     	if ( P_getVarInfo(CURRENT,"pad",&vinfo) == 0 )
            padCount = vinfo.size;
     	if ( P_getVarInfo(CURRENT,"temp",&vinfo) == 0 )
            tempCount = vinfo.size;
        if (tempCount > padCount && padCount <= 1)
            padCount = tempCount;
        totalTime += padCount * pad;
}

static void
reset_channels()
{
#if !defined(G2000) &&  !defined(MERCURY)
/***
     OBSch = TODEV;
     DECch = DODEV;
     DEC2ch = DO2DEV;
     DEC3ch = DO3DEV;
     DEC4ch = DEC3ch + 1;
****/
#endif
}

static void
print_version()
{
#ifdef NVPSG
      fprintf(dpsdata, " %d %d  2\n",VERNUM, GO_PSG_REV);
#else
      fprintf(dpsdata, " %d %d\n",VERNUM, GO_PSG_REV);
#endif
}

void
createDPS(char *cmd, char *expdir, double arraydim, int array_num, char *array_str, int pipe2nmr)
{
        int     n;
        int  	dpsFlag;
	char	retdata[32];
        double  tmpval;
#if !defined(G2000) || defined(MERCURY)
        double  usertime;
        double  getExpTime();
#endif
	struct  stat   f_stat;

        dpsOn = 1;
        dpsSkip = 0;
        ix = 1;
        dpsdata = NULL;
#if !defined(G2000) || defined(MERCURY)
	loadtablecall = -1;
#endif
#ifndef G2000
#ifndef NVPSG
	hwlooping = 0;
	hwloopelements = 0;
	acqtriggers = 0;
	starthwfifocnt = 0;
#endif
	grad_flag = FALSE;
#endif

        initrtvars();
#if !defined(G2000) || defined(MERCURY)
	inittablevar();
#endif
#ifndef G2000
	initparms_img();
#endif

	dpsTimer = 0;
	vFlag = 0;
	dpsFlag = 1;
	if ((int)strlen(cmd) >= 5)
	{
	   switch (cmd[3]) {
	     case 't':
		dpsTimer = 2;
	   	if (cmd[4] > '0' && cmd[4] <= '9')
		   vFlag = cmd[4] - '0';
		if (vFlag == 0)
		{
		    freopen("/dev/null","a", stdout);
		    freopen("/dev/null","a", stderr);
                    outOff = 1;
		}
		else
		    dps_flag = 9;
		break;
	     case 'p':
		dpsFlag = PLOT;
		break;
	     case 'd':
		dps_flag = 9;
	   	if (cmd[4] == '4')
		   debugTimer = 1;
		break;
	     case '_':
		dpsFlag = REDO;
		if (cmd[4] != '0')
		   dps_flag = 9;
                if ((int)strlen(cmd) >= 6) {
                   ixNum = atoi(cmd+5);
                   if (ixNum < 2)
                      ixNum = 2;
                }
		break;
	   }
	}
	P_getreal(CURRENT, "ni", &tmpval, 1);
	if (tmpval > 1.5)
           niFlag = 1;
	else
           niFlag = 0;
	P_getreal(CURRENT, "ni2", &tmpval, 1);
	if (tmpval > 1.5)
           ni2Flag = 1;
	else
           ni2Flag = 0;
	P_getreal(CURRENT, "ni3", &tmpval, 1);
	if (tmpval > 1.5)
           ni3Flag = 1;
	else
           ni3Flag = 0;

/*
        if (dpsFlag >= REDO)
	{
           reset_dps_parms(expdir);
	}
*/
	// if (checkflag)
        //   dpsFlag = 1;
	if (dpsTimer == 0)
	{
	   sprintf(dpsfilepath, "%s/%s", expdir, DPS_DATA);
	   unlink(dpsfilepath);
           if (dpsFlag >= REDO)
	       sprintf(dpsfilepath, "%s/dpsxdata", expdir);
           if ((dpsdata = fopen(dpsfilepath, "w")) == NULL)
           {
               text_error("DPS: could not create file for dps, abort\n");
               return;
           }
           print_version();
        }
	if (dpsFlag < REDO)
	{
	   // if (!checkflag && (dpsTimer == 0))
	   if (dpsTimer == 0)
	   {
	   	dump_dps_info();
                fprintf(dpsdata, " %d %d\n", ARYDIM, (int)arraydim);
	   }
           if (arraydim > 1.0 && array_num > 0)
                parse_array(array_str);
	}
	if (dpsTimer == 0)
	{
	  if (vFlag > 1)
	   printf("start pulsesequence ...\n");
#if !defined(G2000) || defined(MERCURY)
	  inittablevar();
	  last_table = tableNum;
#endif
          reset_channels();
#ifdef  AIX
          if (dps_func != NULL)
            dps_func();
#else
          x_pulsesequence();
#endif
         /***
          if (checkflag) {
             if (dpsdata != NULL)
                fclose(dpsdata);
             return;
          }
          ***/
          if (dpsFlag >= REDO) {
	     ix = ixNum;
             if (dpsdata != NULL)
                fclose(dpsdata);
	     sprintf(dpsfilepath, "%s/dpsxdata", expdir);
	     unlink(dpsfilepath);
             sprintf(dpsfilepath, "%s/%s", expdir, DPS_DATA);
             if ((dpsdata = fopen(dpsfilepath, "w")) == NULL)
             {
               text_error("DPS: could not create file for dps, abort\n");
               return;
             }
             print_version();
	     if (dpsTimer == 0)
                 dump_dps_power();
             reset_dps_parms(expdir);
             reset_channels();
             if (!outOff) {
                freopen("/dev/null","a", stdout);
	        freopen("/dev/null","a", stderr);
                outOff = 1;
             }
#ifdef  AIX
             if (dps_func != NULL)
                dps_func();
#else
             x_pulsesequence();
#endif
          }

#if !defined(G2000) || defined(MERCURY)
          dump_tbl_info();
#endif
          dump_common_array();
	  fsync(fileno(dpsdata));
          fclose(dpsdata);
	  sprintf(dpsfilepath, "%s/%s", expdir, DPS_DATA);
          for( n = 0; n < 3; n++)
          {
             if (stat(dpsfilepath, &f_stat) >= 0)
             {
                if (f_stat.st_size > 0)
                   break;
             }
             usleep(100000); // 100 ms
          }
	  if (dpsFlag < REDO)
	     strcpy(retdata, "1 \n");
	  else
	     strcpy(retdata, "3 \n");
          write(pipe2nmr,retdata, strlen(retdata));
	}
	if (dpsFlag >= PLOT) {
           if (pipe2nmr >= 0)
	       close(pipe2nmr);
	   return;
        }
        ix = 1;
	sprintf(dpsfilepath, "%s/%s", expdir, DPS_TIME);
        if ((dpsdata = fopen(dpsfilepath, "w")) == NULL)
        {
           text_error("DPS: could not create file for dps, abort\n");
           return;
        }
	if (dpsTimer == 0)
        {
	   dpsTimer = 1;
           if (debugTimer > 0)
              vFlag = 1;
           if (!outOff && debugTimer < 1) {
              freopen("/dev/null","a", stdout);
	      freopen("/dev/null","a", stderr);
              outOff = 1;
           }
        }
        print_version();
	if (vFlag)
	   printf("start timer ...%d\n", debugTimer);
	exec_dps_timer();
#if !defined(G2000) || defined(MERCURY)
        usertime = getExpTime();
        if (usertime > 0.0)
           totalTime = usertime;
#endif
        fprintf(dpsdata, "%d %f\n", EXPTIME, totalTime);

	fsync(fileno(dpsdata));
        fclose(dpsdata);
	sprintf(dpsfilepath, "%s/%s", expdir, DPS_TIME);
        for( n = 0; n < 3; n++)
        {
             if (stat(dpsfilepath, &f_stat) >= 0)
             {
                 if (f_stat.st_size > 0)
                    break;
             }
             usleep(100000); // 100 ms
        }
	if (dpsTimer == 1)
	{
	   sprintf(retdata, "2 %f \n", totalTime);
           write(pipe2nmr,retdata, strlen(retdata));
	}
	close(pipe2nmr);
}


int DPSprint(double timev, const char *format, ...)
{
        va_list  vargs;

	if (dpsTimer != 0)
           return(1);
        if (dpsSkip)
        {
	   dpsSkip--;
           return(1);
        }
        if (dpsdata == NULL || dpsOn == 0)
           return(1);

        va_start(vargs, format);
        vfprintf(dpsdata, format, vargs);
        va_end(vargs);
        return(1);
}



void
dps_on()
{
        dpsOn = 1;
        dpsSkip = 0;
}

void
dps_off()
{
        dpsOn = 0;
}

void
dps_skip()
{
        dpsSkip = 1;
}

void
dps_show(char *nm, ...)
{
   (void) nm;
}

static void write_dps_array(char *name, int code, int id)
{
        int     i, k, len, item, k2;
        double  fval;
        vInfo   vinfo;
	ARRAY_DATA *p_list, *new_list;

     
     if (vFlag)
	printf(" add array: %s  type = %d  id= %d\n", name, code, id);
     new_list = (ARRAY_DATA *) calloc(1, sizeof(ARRAY_DATA));
     if (new_list != NULL)
     {
	new_list->name = (char *) malloc(strlen(name) + 1);
	strcpy(new_list->name, name);
	new_list->id = id;
	new_list->xtype = code;
	if (array_list == NULL)
	    array_list = new_list;
	else
	{
     	    p_list = array_list;
	    while (p_list->next != NULL)
		p_list = p_list->next;
	    p_list->next = new_list;
	}
     }
     
     if (dpsTimer != 0)
        return;
     if ( P_getVarInfo(CURRENT,name,&vinfo) != 0 )
        return;
     k = vinfo.size;
     if (vinfo.size < 1 || vinfo.basicType == 0)
        return;
     if ((code != XARRAY) && (name[0] == 'd') && ((int) strlen(name) == 2))
     {
#ifndef G2000
	if (name[1] >= '2' && name[1] <= '4')
#else
	if (name[1] == '2')
#endif
	{
	    P_getreal(CURRENT,name,&fval,1);
	    if (name[1] == '2')
	    {
                if (niFlag)
		{
		  fprintf(dpsdata, " %d -1 %d %d ",code, vinfo.basicType, vinfo.size);
                  fprintf(dpsdata, " ni 0 %f %.9f\n", fval, inc2D);
                  return;
		}
            }
#ifndef G2000
	    else if (name[1] == '3')
	    {
                if (ni2Flag)
		{
		  fprintf(dpsdata, " %d -1 %d %d ",code, vinfo.basicType, vinfo.size);
                  fprintf(dpsdata, " ni2 0 %f %.9f\n", fval, inc3D);
                  return;
		}
            }
	    else if (name[1] == '4')
	    {
                if (ni3Flag)
		{
		  fprintf(dpsdata, " %d -1 %d %d ",code, vinfo.basicType, vinfo.size);
                  fprintf(dpsdata, " ni3 0 %f %.9f\n", fval, inc4D);
                  return;
		}
            }
#endif
        }
     }

     k = vinfo.size;
     item = 0;
     if (vinfo.basicType == T_REAL)
     {
	k2 = 12;
        for (i = 1; i <= k; i++)
	{
	   if (k2 >= 12)
	   {
       	      fprintf(dpsdata, "\n %d %d %d %d %s %d ",code, id, 
				vinfo.basicType, vinfo.size, name, item);
	      k2 = 0;
	   }
           if(P_getreal(CURRENT,name,&fval,i) >= 0)
                fprintf(dpsdata, "%g  ",fval);
           else
                fprintf(dpsdata, "0.0 ");
	   item++;
	   k2++;
	}
     }
     else if (vinfo.basicType == T_STRING)
     {
	len = 20 + strlen(name);
	k2 = 20;
        for (i = 1; i <= vinfo.size; i++)
        {
	   if (k2 >= 20 || len > 500)
	   {
              fprintf(dpsdata, "\n %d %d %d %d %s %d ",code, id, 
				vinfo.basicType, vinfo.size, name, item);
	      len = 20 + strlen(name);
	      k2 = 0;
	   }
	   if ( P_getstring(CURRENT, name, tmpData, i, 30) >= 0)
	   {
                if (strlen(tmpData) < 1)
                    sprintf(tmpData, "''");
                fprintf(dpsdata, "%s ", tmpData);
	        len = len + strlen(tmpData) + 1;
	   }
           else
	   {
                fprintf(dpsdata, "null ");
	        len = len + 5;
	   }
	   item++;
	   k2++;
        }
     }
     fprintf(dpsdata, "\n");
}


static void
dps_clear_fval()
{
	int	k;

	for (k = 0; k < 9; k++)
	    dps_fval[k] = 0.0;
}


static void
dps_print_grad(int code, const char *name, const char *v_name,
               const char *w_name, double width)
{
	int	n;

	for (n = 0; n < 3; n++)
	{
	    fprintf(dpsdata, "%d %s 11 0 3 %s %s %f %s %f 0 0.0 \n", code, name,
		grad_label[n], v_name, dps_fval[n], w_name, width);
	}
}


void
dps_magradient(char *name, int code, int dcode, double level, double theta, double phi,
               char *s1, char *s2, char *s3)
{
	if (dpsTimer != 0)
	    return;
#ifndef G2000
	dps_clear_fval();
#ifdef NVPSG
       // obl_gradient(0.0,0.0,level);
        dps_fval[2] = level;
#else
	S_oblique_gradient(0.0,0.0,level, phi, 0.0, theta);
#endif
	dps_print_grad(code, name, s1, "0", 0.0);
#endif
}

void
dps_simgradient(char *name, int code, int dcode, double level, char *s1)
{
	if (dpsTimer != 0)
	    return;
#ifndef G2000
	dps_clear_fval();
#ifdef NVPSG
        // obl_gradient(level,level,level);
        dps_fval[0] = level;
        dps_fval[1] = level;
        dps_fval[2] = level;
#else
	S_oblique_gradient(level,level,level, 90.0, 0.0, 90.0);
#endif
	dps_print_grad(code, name, s1, "0", 0.0);
#endif
}

void
dps_magradpulse(char *name, int code, int dcode,
                double level, double width, double theta, double phi,
                char *s1, char *s2, char *s3, char *s4)
{
	if (dpsTimer != 0)
	{
	    add_delay_node(width);
	    return;
	}
#ifndef G2000
	dps_clear_fval();
#ifdef NVPSG
//        oblique_gradpulse(0.0,0.0,level, phi, 0.0, theta, width); 
        dps_fval[2] = level;
#else
	S_oblique_gradpulse(0.0,0.0,level, phi, 0.0, theta, width);
#endif
	dps_print_grad(code, name, s1, s2, width);
	fprintf(dpsdata, "%d delay 1 0 1 %s %f \n", dcode, s2, width);
	dps_clear_fval();
	dps_print_grad(code, name, "0", "0", 0.0);
#endif
}

void
dps_mashapedgradient(name, code,dcode, pat,level,width,theta,phi,loop,wait,s1,s2,s3,s4,s5,s6,s7)
char	*name;
int	code, dcode, loop, wait;
char    *pat;
double  level, width, theta, phi;
char	*s1,*s2,*s3,*s4,*s5,*s6,*s7;
{
	if (dpsTimer != 0)
	{
	    if (wait)
	        add_delay_node(width * (double) loop);
	    return;
	}
	dps_clear_fval();
#ifndef G2000
#ifdef NVPSG
/**
        phase_encode3_oblshapedgradient(pat,"","",width,0.0,0.0,level,
                   0.0,0.0,0.0, 0,0,0, 0.0,0.0,0.0, (int)(loop+0.5),(int)(wait+0.5),0);
**/
        dps_fval[2] = level;
#else
	S_oblique_shapedgradient(pat,width,0.0,0.0,level,phi,0.0,theta,(int)(loop+0.5),(int)(wait+0.5));
#endif
	fprintf(dpsdata, "%d %s 12 2 2 x %s ", code, name, pat);
	fprintf(dpsdata, "%s %d NOWAIT 0  %s %f %s %f \n", s6, loop, s3, width, s2, dps_fval[0]);
	fprintf(dpsdata, "%d %s 12 2 2 y %s ", code, name, pat);
	fprintf(dpsdata, "%s %d NOWAIT 0  %s %f %s %f \n", s6, loop, s3, width, s2, dps_fval[1]);
	fprintf(dpsdata, "%d %s 12 2 2 z %s ", code, name, pat);
	fprintf(dpsdata, "%s %d %s %d  %s %f %s %f \n", s6, loop, s7, wait,s3, width, s2, dps_fval[2]);
#endif
}

void
dps_mashapedgradpulse(name, code, dcode, pat,level,width,theta,phi, s1,s2,s3,s4,s5)
char	*name;
int	code, dcode;
char    *pat;
double  level, width, theta, phi;
char	*s1,*s2,*s3,*s4,*s5;
{
	if (dpsTimer != 0)
	{
	    add_delay_node(width);
	    return;
	}
#ifndef G2000
	dps_clear_fval();
#ifdef NVPSG
/*
        phase_encode3_oblshapedgradient(pat,"","",width,0.0,0.0,level,
                   0.0,0.0,0.0, 0,0,0, 0.0,0.0,0.0, 1,WAIT,0);
*/
        dps_fval[2] = level;
#else
	S_oblique_shapedgradient(pat,width,0.0,0.0,level,phi,0.0,theta,0,WAIT);
#endif
	fprintf(dpsdata, "%d %s 12 2 2 x %s ", code, name, pat);
	fprintf(dpsdata, "0 0 NOWAIT 0  %s %f %s %f \n", s3, width, s2, dps_fval[0]);
	fprintf(dpsdata, "%d %s 12 2 2 y %s ", code, name, pat);
	fprintf(dpsdata, "0 0 NOWAIT 0  %s %f %s %f \n", s3, width, s2, dps_fval[1]);
	fprintf(dpsdata, "%d %s 12 2 2 z %s ", code, name, pat);
	fprintf(dpsdata, "0 0 WAIT 1  %s %f %s %f \n", s3, width, s2, dps_fval[2]);
#endif
}

void
dps_oblique_gradpulse(name, code,dcode, h1,h2,h3,ang1,ang2,ang3,width,s1,s2,s3,s4,s5,s6,s7)
char	*name;
int	code, dcode;
double  h1, h2, h3, ang1,ang2,ang3, width;
char	*s1,*s2,*s3,*s4,*s5,*s6,*s7;
{
	if (dpsTimer != 0)
	{
	    add_delay_node(width);
	    return;
	}
#ifndef G2000
	dps_clear_fval();
#ifdef NVPSG
 //     oblique_gradpulse(h1,h2,h3,ang1,ang2,ang3,width);
        dps_fval[0] = h1;
        dps_fval[1] = h2;
        dps_fval[2] = h3;
#else
	S_oblique_gradpulse(h1,h2,h3,ang1,ang2,ang3,width);
#endif
	dps_print_grad(code, name, s1, s7, width);
	fprintf(dpsdata, "%d delay 1 0 1 %s %f \n", dcode, s7, width);
	dps_clear_fval();
	dps_print_grad(code, name, "0", "0", 0.0);
#endif
}

void
dps_pe_oblique_gradient(name,code,pat1,pat2,pat3,width,lvl1,lvl2,lvl3,
                        step2,vmult2,lim2,ang1,ang2,ang3,wait,tag, s1, s2, s3,
			s4,s5,s6,s7,s8,s9,s10,s11,s12,s13,s14,s15)
char	*name;
int	code;
char    *pat1, *pat2, *pat3;
double  width;
double  lvl1,lvl2,lvl3;
double  ang1,ang2,ang3;
double  step2,lim2;
int	 wait,tag;
codeint vmult2;
char	*s1,*s2,*s3,*s4,*s5,*s6,*s7,*s8,*s9;
char	*s10,*s11,*s12,*s13,*s14,*s15;
{
#ifndef NVPSG
	char    c1, c2, c3;
	char	*n1, *n2, *n3;
	int	v1, v2, v3;
#endif

	if (dpsTimer != 0)
	{
	    if (wait)
	    	add_delay_node(width);
	    return;
	}
#ifndef G2000
	dps_clear_fval();
#ifdef NVPSG
/*
        phase_encode3_oblshapedgradient(pat1,pat2,pat3,width,lvl1,lvl2,lvl3,
                   0.0,step2,0.0, 0,vmult2,0, 0.0,lim2,0.0,1,wait,0);
*/
     /*  pattern, loop, wait, width, level, angle */
	fprintf(dpsdata, "%d %s 12 2 3 ?x ?%s 0 0 0 0 ",
		OBLSHGR, name, pat1);
	fprintf(dpsdata, "%s %.9f %s %.9f %s %.9f\n",
		 s4, width, s5, lvl1, s11,ang1);

	fprintf(dpsdata, "%d %s 12 2 3 ?z ?%s 0 0 0 0 ",
		OBLSHGR, name, pat3);
	fprintf(dpsdata, "%s %.9f %s %.9f %s %.9f\n",
		 s4, width, s7, lvl3, s13,ang3);

     /*  pattern, loop, wait, mult, width, level, step, lim, angle */
	fprintf(dpsdata, "%d %s 12 3 5 ?y ?%s 0 0 %s %d %s %d ",
		PESHGR, name, pat2, s14, wait, s9, vmult2);
	fprintf(dpsdata, "%s %.9f %s %.9f %s %.9f %s %.9f %s %.9f \n",
		 s4, width, s6,lvl2, s8,step2, s10,lim2, s12,ang2);
#else
	S_pe_oblique_shaped3gradient(pat1,pat2,pat3,width,lvl1,lvl2,lvl3,
                        step2,vmult2,lim2,ang1,ang2,ang3,wait,tag);
	if (dps_fval[3] == 1.0) 
	{
	   c1 = 'x'; c2 = 'y'; c3 = 'z';
	   v1 = 0; v2 = 1; v3 = 2;
	   n1 = pat1; n2 = pat2; n3 = pat3;
	}
	else if (dps_fval[3] == 2.0) 
	{
	   c1 = 'y'; c2 = 'x'; c3 = 'z';
	   v1 = 1; v2 = 0; v3 = 2;
	   n1 = pat2; n2 = pat1; n3 = pat3;
	}
	else
	{
	   c1 = 'z'; c2 = 'x'; c3 = 'y';
	   v1 = 2; v2 = 0; v3 = 1;
	   n1 = pat3; n2 = pat1; n3 = pat2;
	}
	fprintf(dpsdata, "%d %s 12 3 4 ?%c ?%s one 1 NOWAIT 0 %s %d ",
		PEOBLVG, name, c1, n1, s9, vmult2);
	fprintf(dpsdata, "%s %.9f %s %.9f %s %.9f %s %.9f \n",
		 s4,width, s5,dps_fval[v1], s6,dps_fval[4], s8,step2);

	fprintf(dpsdata, "%d %s  12 3 4 ?%c ?%s loop 1 NOWAIT 0 %s %d ",
		PEOBLG, name, c2, n2, s9, vmult2);
	fprintf(dpsdata, "%s %.9f %s %.9f %s %.9f %s %.9f \n",
		 s4,width, s5,dps_fval[v2], s6,dps_fval[4], s8,step2);

	fprintf(dpsdata, "%d %s  12 3 4 ?%c ?%s loop 1 %s %d %s %d ", 
		PEOBLG, name, c3, n3, s14, wait, s9, vmult2);
	fprintf(dpsdata, "%s %.9f %s %.9f %s %.9f %s %.9f \n",
		 s4,width, s5,dps_fval[v3], s6,dps_fval[4], s8,step2);
#endif
	dps_clear_fval();
#endif
}


void
dps_pe_oblshapedgradient(name,code,pat1,pat2,pat3,width,lvl1,lvl2,lvl3,
             step1,step2,step3,mult1,mult2,mult3,wait,tag,
             s1,s2,s3,s4,s5,s6,s7,s8,s9,s10,s11,s12)
char	*name;
int	code;
char    *pat1, *pat2, *pat3;
double  width;
double  lvl1,lvl2,lvl3;
double  step1,step2,step3;
codeint mult1,mult2,mult3;
int	wait,tag;
/* s1: width, s2: lvl1, .... */
char	*s1,*s2,*s3,*s4,*s5,*s6,*s7,*s8,*s9;
char	*s10,*s11,*s12;
{
	if (dpsTimer != 0)
	{
	    if (wait)
	    	add_delay_node(width);
	    return;
	}
#ifndef G2000
#ifdef NVPSG
     /*  pattern, loop, wait, mult, width, level, step, lim, angle */
	fprintf(dpsdata, "%d %s 12 3 5 ?x ?%s 0 0 0 0 %s %d ",
		PESHGR, name, pat1, s8, mult1);
	fprintf(dpsdata, "%s %.9f %s %.9f %s %.9f 0 0.0 0 0.0 \n",
		 s1, width, s2,lvl1, s5,step1);

	fprintf(dpsdata, "%d %s 12 3 5 ?y ?%s 0 0 0 0 %s %d ",
		PESHGR, name, pat2, s9, mult2);
	fprintf(dpsdata, "%s %.9f %s %.9f %s %.9f 0 0.0 0 0.0 \n",
		 s1, width, s3,lvl2, s6,step2);

	fprintf(dpsdata, "%d %s 12 3 5 ?z ?%s 0 0 %s %d %s %d ",
		PESHGR, name, pat3, s11, wait, s10, mult3);
	fprintf(dpsdata, "%s %.9f %s %.9f %s %.9f 0 0.0 0 0.0 \n",
		 s1, width, s4,lvl3, s7,step3);
#endif
#endif

}

void
dps_simgradpulse(char *name, int code, int dcode, double level, double width, char *s1, char *s2)
{
	if (dpsTimer != 0)
	{
	    add_delay_node(width);
	    return;
	}
#ifndef G2000
	dps_clear_fval();
#ifdef NVPSG
//      oblique_gradpulse(level, level, level, 90.0, 0.0, 90.0, width);
        dps_fval[0] = level;
        dps_fval[1] = level;
        dps_fval[2] = level;
#else
	S_oblique_gradpulse(level, level, level, 90.0, 0.0, 90.0, width);
#endif
	dps_print_grad(code, name, s1, s2, width);
	fprintf(dpsdata, "%d delay 1 0 1 %s %f \n", dcode, s2, width);
	dps_clear_fval();
	dps_print_grad(code, name, "0", "0", 0.0);
#endif
}


void
dps_simshapedgradient(name, code,dcode, pat,level,width,loop,wait,s1,s2,s3,s4,s5)
char	*name;
int	code, dcode, loop, wait;
char    *pat;
double  level, width;
char	*s1,*s2,*s3,*s4,*s5;
{
	if (dpsTimer != 0)
	{
	    if (wait)
	    	add_delay_node(width * (double)loop);
	    return;
	}
#ifndef G2000
	dps_clear_fval();
#ifdef NVPSG
/**
        phase_encode3_oblshapedgradient(pat,"","",width,level,level,level,
                   0.0,0.0,0.0, 0,0,0, 0.0,0.0,0.0,loop,wait,0);
**/
        dps_fval[0] = level;
        dps_fval[1] = level;
        dps_fval[2] = level;
#else
	S_oblique_shapedgradient(pat,width,level,level,level,90.0,0.0,90.0,loop, wait);
#endif
	fprintf(dpsdata, "%d %s 12 2 2 x %s ", code, name, pat);
	fprintf(dpsdata, "%s %d NOWAIT 0  %s %f %s %f \n",s4, loop, s3, width, s2, dps_fval[0]);
	fprintf(dpsdata, "%d %s 12 2 2 y %s ", code, name, pat);
	fprintf(dpsdata, "%s %d NOWAIT 0  %s %f %s %f \n",s4, loop, s3, width, s2, dps_fval[1]);
	fprintf(dpsdata, "%d %s 12 2 2 z %s ", code, name, pat);
	fprintf(dpsdata, "%s %d %s %d %s %f %s %f \n",s4, loop, s5, wait, s3, width, s2, dps_fval[2]);
#endif
}


void
dps_simshapedgradpulse(name, code, dcode, pat, h, width, s1, s2, s3)
char	*name;
int	code, dcode;
char    *pat;
double  h, width;
char	*s1,*s2,*s3;
{
	if (dpsTimer != 0)
	{
	    add_delay_node(width);
	    return;
	}
#ifndef G2000
	dps_clear_fval();
#ifdef NVPSG
/*
        phase_encode3_oblshapedgradient(pat,"","",width,h,h,h,
                 0.0,0.0,0.0, 0,0,0, 0.0,0.0,0.0, 1, WAIT, 0);
*/
        dps_fval[0] = h;
        dps_fval[1] = h;
        dps_fval[2] = h;
#else
	S_oblique_shapedgradient(pat,width,h,h,h,90.0,0.0,90.0,0,WAIT);
#endif
	fprintf(dpsdata, "%d %s 12 2 2 x %s ", code, name, pat);
	fprintf(dpsdata, "0 0 NOWAIT 0  %s %f %s %f \n", s3, width, s2, dps_fval[0]);
	fprintf(dpsdata, "%d %s 12 2 2 y %s ", code, name, pat);
	fprintf(dpsdata, "0 0 NOWAIT 0  %s %f %s %f \n", s3, width, s2, dps_fval[1]);
	fprintf(dpsdata, "%d %s 12 2 2 z %s ", code, name, pat);
	fprintf(dpsdata, "0 0 WAIT 1  %s %f %s %f \n", s3, width, s2, dps_fval[2]);
#endif
}


void
dps_oblique_gradient(char *name, int code,
                     double f1, double f2, double f3, double f4, double f5, double f6,
                     char *s1, char *s2, char *s3, char *s4, char *s5, char *s6)
{
	if (dpsTimer != 0)
	    return;
#ifndef G2000
	dps_clear_fval();
#ifdef NVPSG
        // oblique_gradient(f1, f2, f3);
        dps_fval[0] = f1;
        dps_fval[1] = f2;
        dps_fval[2] = f3;
#else
	S_oblique_gradient(f1, f2, f3, f4, f5, f6);
#endif
	fprintf(dpsdata, "%d %s 11 0 3 %s %s %f %s %f %s %f\n", code, name,
			grad_label[0], s1, f1, s1, dps_fval[0], s4, f4);
	fprintf(dpsdata, "%d %s 11 0 3 %s %s %f %s %f %s %f \n", code, name,
			grad_label[1], s2, f2, s2, dps_fval[1], s5, f5);
	fprintf(dpsdata, "%d %s 11 0 3 %s %s %f %s %f  %s %f\n", code, name,
			grad_label[2], s3, f3, s3, dps_fval[2], s6, f6);
#endif
}

static int
add_com_array_list(char *name, int code, int id, int chan, int size, double data[])
{
	ARRAY_DATA  *d_list, *n_list;

        if (name == NULL)
           return (id);
	if (vFlag > 1)
	   printf(" array '%s': size= %d  list= %d\n", name, size, id);
	d_list = com_array_list;
	while (d_list != NULL)
	{
             if (strcmp(name, d_list->name) == 0)
             {
	         if (d_list->vars == data && d_list->chan == chan)
	         {
                     if (id < 0 || (id == d_list->id)) 
                     {
		        if (d_list->size < size)
		            d_list->size = size;
		        return (d_list->id);
                     }
	         }
	     }
	     if (d_list->next == NULL)
		break;
	     d_list = d_list->next;
	}
	n_list = (ARRAY_DATA *) malloc(sizeof(ARRAY_DATA));
	if (n_list == NULL)
	     return (id);
        if (id < 0) {
            id = offset_list_id;
            offset_list_id++;
        }
	n_list->name = (char *) malloc(strlen(name) + 1);
	strcpy(n_list->name, name);
	n_list->code = code;
	n_list->id = id;
	n_list->chan = chan;
	n_list->size = size;
	n_list->type = T_REAL;
	n_list->vars = data;
	n_list->next = NULL;
	if (com_array_list == NULL)
	     com_array_list = n_list;
	else
	     d_list->next = n_list;
        return (id);
}


int
dps_poffset_list(char *name, int code, int chan, double parray[],
                 double f1, double f2, double f3, int v1, int v2,
                 char *s1, char *s2, char *s3, char *s4, char *s5, char *s6)
{
        int id = 0;

	if (dpsTimer != 0)
	    return (id);
	id = add_com_array_list(s1, PARRAY, v1, 1, (int)f2, parray);
	fprintf(dpsdata, "%d %s %d 2 4 ", code, name, chan);
	// if (v1 < 0)  it is from poffset_list */
	// else  it is from position_offset_list */
	fprintf(dpsdata, "%s %d ", s5, id);
	fprintf(dpsdata, "%s %d %s %f ", s6, v2, s1, parray[0]);
	fprintf(dpsdata, "%s %f %s %f %s %f\n", s2, f1, s3, f2, s4, f3);
	return (id);
}


int
dps_create_delay_list(char *name, int code, int dcode, double parray[],
                      int v1, int v2, char *s1, char *s2, char *s3)
{
        int id;

	id = add_com_array_list(s1, DARRAY, v2, TODEV, v1, parray);
	return (id);
}

void dps_lcsample(char *name, int code)
{
	if (dpsTimer != 0)
	    return;
#ifndef G2000
	dps_clear_fval();
	lcsample();
	if (dps_fval[0] < 1.0 && dps_fval[1] < 1.0)
	    return;
	fprintf(dpsdata, "%d ifzero 0 1 0 ix %d \n", IFZERO, IX);
	if (dps_fval[0] > 0.0)
	{
	    fprintf(dpsdata, "%d lcsample-sp1on 1 1 0 1 1 \n", SPON);
	    fprintf(dpsdata, "%d lcsample-delay 1 0 1 0.5 0.5 \n", DELAY);
	    fprintf(dpsdata, "%d lcsample-sp1off 1 1 0 1 0 \n", SPON);
	}
	if (dps_fval[1] > 0.0)
	{
	    fprintf(dpsdata, "%d lcsample-xgate  1 0 1 ntrig %.9f \n", XGATE, dps_fval[1]);
	    fprintf(dpsdata, "%d lcsample-delay 1 0 1 dtrig %.9f \n", DELAY, dps_fval[2]);
	}
	fprintf(dpsdata, "%d endif 0 1 0 ix %d \n", ENDIF, IX);
	dps_clear_fval();
#endif
}


int
dps_create_freq_list(char *name, int code, int dcode, double parray[],
                     int v1, int v2, int v3,
                     char *s1, char *s2, char *s3, char *s4)
{
        int id;

        id = add_com_array_list(s1, FARRAY, v3, v2, v1, parray);
        return (id);
}

int
dps_create_offset_list(char *name, int code, int dcode, double parray[],
                       int v1, int v2, int v3,
                       char *s1, char *s2, char *s3, char *s4)
{
        int id;

	id = add_com_array_list(s1, OARRAY, v3, v2, v1, parray);
        return(id);
}


#if !defined(G2000) || defined(MERCURY)
void
dps_loadtable(char *filename)
{
	VNAME_LIST  *v_node, *n_node;
	int	t_num;
	
	v_node = table_list;
	while (v_node != NULL)
	{
	   if (strcmp(filename, v_node->name) == 0)
		return;
	   if (v_node->next == NULL)
		break;
	   v_node = v_node->next;
	}
	t_num = last_table;
	loadtable(filename);
	t_num = last_table - t_num;
	if (t_num > 0)
	{
	   tableNum += t_num;
	}
	n_node = (VNAME_LIST *) calloc(1, sizeof(VNAME_LIST));
	if (n_node == NULL)
	     return;
	n_node->name = (char *) malloc(strlen(filename) + 1);
	strcpy(n_node->name, filename);
	if (table_list == NULL)
	   table_list = n_node;
	else
	   v_node->next = n_node;
}
#endif


static int
get_RTval(int rtindex)
{
	if (rtindex <= 0 || rtindex >= RTMAX)
	    return(0);
	return (RTval[rtindex]);
}

static void
set_RTval(int rtindex, double fval)
{
	double  value;

	if (rtindex > 0 && rtindex < RTMAX)
	{
	    value = (fval > 0.0) ? (fval + 0.5) : (fval - 0.5);
	    RTval[rtindex] = (int)value;
	}
}

static double cal_dps_timer(int id)
{
	ARRAY_DATA  *a_list;
	char	    *pname, *sptr;
	int	    nct, v_index, g_index, is_ni, k, a_num;
	double	    fval, inc_ni, t_time;
	double	    nt2, ss2;
	double	    ss_time;

	a_list = array_list;
	pname = NULL;
	t_time = 0.0;
	ss_time = 0.0;
	if (id == 1)
	{
           if (!outOff) {
	      freopen("/dev/null","a", stdout);
	      freopen("/dev/null","a", stderr);
              outOff = 1;
           }
	}
	while (a_list != NULL)
	{
	    if (a_list->id == id)
	    {
		pname = a_list->name;
		break;
	    }
	    a_list = a_list->next;
	}
	if (pname != NULL)  /* it is an array */
	{
	    inc_ni = 0.0;
	    is_ni = 0;
	    if (a_list->xtype != XARRAY)
	    {
		is_ni = is_ni_name(pname);
		if (is_ni == 2)
		     inc_ni = inc2D;
#ifndef G2000
		if (is_ni == 3)
		     inc_ni = inc3D;
		if (is_ni == 4)
		     inc_ni = inc4D;
#endif
	    }
	    g_index = dps_find(pname);
	    v_index = g_index;
	    if (g_index < 0)
	        v_index = find(pname);
	    if (v_index < 0)
		return(t_time);
	    a_num = a_list->size;
	    if (is_ni)
	    {
	      /* if it is ni then  do the first and the last only */
		fval = a_list->vars[0];
		t_time = cal_dps_timer(id + 1);
		fval = a_list->vars[0] + (a_num - 1) * inc_ni;
		dps_set(v_index, pname, fval);
                setGlobalDouble(pname, fval);
		t_time = t_time + cal_dps_timer(id + 1);
		t_time = (t_time * a_num) / 2.0;
		if (vFlag > 1)
                   printf(" array %s= %f \n", pname, fval);
	    }
	    else
	    {
		for(k = 0; k < a_num; k++)
		{
		    a_list = array_list;
		    while (a_list != NULL)
		    {
			if (a_list->id == id)
			{
		    	   pname = a_list->name;
	    	    	   g_index = dps_find(pname);
	    	    	   v_index = g_index;
	    	    	   if (g_index < 0)
	        		v_index = find(pname);
	    	    	   if (v_index < 0)
				return(t_time);
	    	       	   if (a_list->type == T_REAL)
		       	   {
			  	fval = a_list->vars[k];
		    	  	if (g_index >= 0)
				   dps_set(v_index, pname, fval);
		    	  	else
				   *cvals[v_index] = fval;
			  	if (vFlag > 1)
                   	           printf(" array %s= %f \n", pname, fval);
		       	   }
		       	   else if (a_list->type == T_STRING)
		       	   {
				sptr = a_list->vstrs[k];
                	  	P_setstring(CURRENT,pname, sptr, 1);
                          	if (vFlag > 1)
                            	   printf(" array %s= %s \n", pname, sptr);
		       	   }
			}
			a_list = a_list->next;
		    }
		    t_time = t_time + cal_dps_timer(id + 1);
		}
	    }
	}
	else
	{
	    t_time = 0.0;
	    if (first_rtnode == NULL)
	    {
		first_rtnode = (RTNODE *) calloc(1, sizeof(RTNODE));
		if (first_rtnode == NULL)
			return(t_time);
		first_rtnode->code = DUMMY;
		first_rtnode->next = NULL;
	    }
	    cur_rtnode = first_rtnode;

#if !defined(G2000) || defined(MERCURY)
	    inittablevar();
	    last_table = tableNum;
#endif
#ifdef  AIX
            if (dps_tfunc != NULL)
                dps_tfunc();
#else
            t_pulsesequence();
#endif
	    ix++;
	    if (cur_rtnode->next != NULL)
		cur_rtnode->next->code = LAST_CODE;

	    nct = 0;
	    nt2 = nt;
	    if (ssNum < 0.0)
		ss2 = -ssNum;
	    else
	    {
		ss2 = ssNum;
		ssNum = 0.0;
	    }
	    fval = 0.0;
	    t_time = 0.0;

            if (nt2 > 16.0)
                nt2 = 16.0;
	    while (nt2 > 0.0 || ss2 > 0.0)
	    {
	    	simulate_dps_code(nct);
		fval = 0.0;
	    	for(k = 0; k < MAXLEVEL; k++)
	    	    fval = fval + timers[k] * (double)loopRcnt[k];
	    	if (!gotAcquire)
		    fval += at;
#ifndef NVPSG
                /* 0.008 or 0.01 second overhead of housekeeping */
	        if (newacq)
	            fval += 0.008;  /* 8 msecond */
		else
	            fval += 0.01;  /* 10 msecond */
#endif
                t_time += fval;
		if (nct == 0)
		    ss_time = fval * ss2;
		nct++;
		nt2 -= 1.0;
		ss2 = 0.0;
	    	if (loopDo[0] == 0)
		   break;
	    }
            if (nct > 1)
                fval = t_time / (double) nct;
            if (nt > 16.0)
                nt2 = nt2 + nt - 16.0;
	    t_time = t_time + fval * nt2;
	    if (ssNum < 0.0)
	        t_time += ss_time;
	    else
	        ssTime += ss_time;
	    if (vFlag > 1)
		printf("  nt= %f  time= %f\n", nt, t_time);
	    if (vFlag)
	    {
		totalNum++;
		fprintf(stderr, "  time(%d): %f \n",totalNum, t_time);
	    }
	}
	return(t_time);
}


int
DPStimer(int code, int subcode, int vnum, int fnum, ...)
{
	RTNODE  *new_node;
        int v1, v2, v3, v4;
        double  f1, f2, f3, f4, f5, f6;
        va_list  vargs;

        v1 = v2 = v3 = v4 = 0;
        f1 = f2 = f3 = f4 = f5 = f6 = 0.0;
	new_node = cur_rtnode->next;
	if (new_node == NULL)
	{
	     new_node = (RTNODE *) calloc(1, sizeof(RTNODE));
	     if (new_node == NULL)
		return(1);
	     new_node->next = NULL;
	     cur_rtnode->next = new_node;
	}
	new_node->code = code;
	new_node->subcode = subcode;
	new_node->vnum = vnum;
	new_node->fnum = fnum;
	new_node->width = 0.0;
        va_start(vargs, fnum);
	if (fnum > 0 || vnum > 0)
	{
             v1 = va_arg(vargs, int);
             v2 = va_arg(vargs, int);
             v3 = va_arg(vargs, int);
             v4 = va_arg(vargs, int);
	     new_node->vals[0] = v1;
	     new_node->vals[1] = v2;
	     new_node->vals[2] = v3;
	     new_node->vals[3] = v4;
	}

	switch (code) {
	 case RVOP:
		if (vnum <= 0)
		    new_node->code = DUMMY;
		break;
	 case DELAY:
	 case ZGRAD:
	 case SAMPLE:
                if (fnum > 0)
                   f1 = va_arg(vargs, double);
		new_node->width = f1;
		break;
	 case PULSE:
	 case SHPUL:
	 case APSHPUL:
                if (fnum >= 3) {
                   f1 = va_arg(vargs, double);
                   f2 = va_arg(vargs, double);
                   f3 = va_arg(vargs, double);
		   new_node->width = f1 + f2 +f3;
                }
		break;
	 case SHGRAD:
	 case PESHGR:
	 case SHVGRAD:
	 case OBLSHGR:
	 case SH2DVGR:
	 case SHINCGRAD:
                if (fnum > 0)
                   f1 = va_arg(vargs, double);
		if (v2 != 0)  /*  wait */
		   new_node->width = f1 * (double) v1;
		break;
	 case SMPUL:
	 case SMSHP:
                if (fnum < 6)
		   break;
                f1 = va_arg(vargs, double);
                f2 = va_arg(vargs, double);
                f3 = va_arg(vargs, double);
                f4 = va_arg(vargs, double);
                f5 = va_arg(vargs, double);
                f6 = va_arg(vargs, double);
		if (f2 > f1)
		    f1 = f2;
		if (f3 > f1)
		    f1 = f3;
		if (f4 > f1)
		    f1 = f4;
		new_node->width = f1 + f5 + f6;
		break;
	 case SETVAL:
                if (fnum > 0)
                   f1 = va_arg(vargs, double);
		new_node->fval = f1;
		break;
	 case ACQUIRE:
                if (fnum > 1) {
                   f1 = va_arg(vargs, double);
                   f2 = va_arg(vargs, double);
		   new_node->width = f1 * f2 / 2.0;
                }
		break;
	 case INITDLY:
                if (fnum > 0) {
                   f1 = va_arg(vargs, double);
		   if (v1 < 5)
		       incDelays[v1] = f1;
                }
		break;
	 case INCDLY:
		if (v2 >= 5)
		    new_node->code = DUMMY;
		break;
	 case SPINLK:
                if (fnum > 0) {
                   f1 = va_arg(vargs, double);
		   new_node->width = f1 * (double) v1;
                }
		break;
	 case MSLOOP:
                if (fnum > 0)
                    f1 = va_arg(vargs, double);
		if (subcode == 'c')
		{
		    if (f1 <= 0.5)
			new_node->fval = 1.0;
		    else
			new_node->fval = f1;
		}
		else if (subcode == 's')
		{
		    if (f1 <= 1.0)
		        new_node->code = MSLOOP2;
		    else
		        new_node->code = DUMMY;
		}
		else
		    new_node->code = DUMMY;
		break;
	 case PELOOP:
                if (fnum > 0)
                    f1 = va_arg(vargs, double);
		if (subcode == 'c')
		{
		    if (f1 <= 0.5)
			new_node->fval = 1.0;
		    else
			new_node->fval = f1;
		}
		else if (subcode == 's')
		    new_node->code = PELOOP2;
		else
		    new_node->code = DUMMY;
		break;
	 case ENDSISLP:
		if (subcode != 'c')
		    new_node->code = DUMMY;
		break;
	 case SLOOP:
		if (subcode == 3) /* GEMINI's loop */
		{
		    new_node->vals[0] = v2;
		    new_node->vals[1] = v3;
		}
		break;
	 case GLOOP:
		gloop = 1;
		gloopRt[v1] = v3;
		break;
	 case KZLOOP: // kzloop
                if (fnum > 0)
                   f1 = va_arg(vargs, double);
                if (f1 >= 0.0)
		   new_node->width = f1;
		break;
	 case NWLOOP:
                if (fnum > 0)
                   f1 = va_arg(vargs, double);
                if (f1 >= 0.0)
		   new_node->fval = f1;
		break;
	}
	cur_rtnode = new_node;
        va_end(vargs);
	return(1);
}

static void
rt_val_cmd(RTNODE *node)
{
	int	v1, v2, v3, dv, data, k;

	v1 = node->vals[0];
	v2 = node->vals[1];
	v3 = node->vals[2];
	switch (node->subcode) {
	 case  RADD:
		     data = get_RTval(v1) + get_RTval(v2);
		     dv = v3;
		     break;
	 case  RDBL:
		     data = get_RTval(v1) * 2;
		     dv = v2;
		     break;
	 case  RHLV:
		     data = get_RTval(v1) / 2;
		     dv = v2;
		     break;
	 case  RSUB:
		     data = get_RTval(v1) - get_RTval(v2);
		     dv = v3;
		     break;
	 case  RDECR:
		     data = get_RTval(v1) - 1;
		     dv = v1;
		     break;
	 case  RDIVN:
		     k = get_RTval(v2);
		     if (k == 0)
			k = 1;
		     data = get_RTval(v1) / k;
		     dv = v3;
		     break;
	 case  RINCR:
		     data = get_RTval(v1) + 1;
		     dv = v1;
		     break;
	 case  RMOD2:
		     data = get_RTval(v1) % 2;
		     dv = v2;
		     break;
	 case  RMOD4:
		     data = get_RTval(v1) % 4;
		     dv = v2;
		     break;
	 case  RMODN:
		     k = get_RTval(v2);
		     if (k == 0)
			k = 1;
		     data = get_RTval(v1) % k;
		     dv = v3;
		     break;
	 case  RMULT:
		     data = get_RTval(v1) * get_RTval(v2);
		     dv = v3;
		     break;
	 case  RASSIGN:
		     data = get_RTval(v1);
		     dv = v2;
		     break;
	 default:
		     return;
		     break;
	}
	set_RTval(dv, (double) data);
}


#if !defined(G2000) || defined(MERCURY)
static void
rt_tbl_cmd(int ttop, RTNODE *node)
{
	int	v1, v2, v3;
	int	opcode;

	switch (node->subcode) {
	 case  RADD:
		     opcode = TADD;
		     break;
	 case  RDIVN:
		     opcode = TDIV;
		     break;
	 case  RSUB:
		     opcode = TSUB;
		     break;
	 case  RMULT:
		     opcode = TMULT;
		     break;
	 default:
		     return;
		     break;
	}
	v1 = node->vals[0];
	v2 = node->vals[1];
	v3 = node->vals[2];
	if (ttop)
	     tabletop(opcode, v1, v2, v3);
	else
	     tablesop(opcode, v1, v2, v3);
}

static int
dps_get_ap(int tnum, int sindex, int dindex)
{
	int 	vnum, *tval;

	if (tnum < t1 || tnum > t60)
	   return(0);
	tnum = tnum - t1;
	if (Table[tnum]->table_size <= 0)
	   return(0);
	if (sindex >= v1 && sindex <= three)
	    sindex = get_RTval(sindex);
	if (dindex >= v1 && dindex <= three)
	    dindex = get_RTval(dindex);
	if (Table[tnum]->auto_inc || sindex < 0)
	{
	   vnum = Table[tnum]->indexptr;
	   if (vnum >= Table[tnum]->table_size - 1)
		Table[tnum]->indexptr = 0;
	   else
	        Table[tnum]->indexptr++;
	}
	else
	   vnum = sindex;
	if (Table[tnum]->divn_factor > 1)
	   vnum = vnum / Table[tnum]->divn_factor;
	vnum = vnum % Table[tnum]->table_size;
	tval = Table[tnum]->table_pointer + vnum;
	if (dindex >= 0 && dindex < Table[tnum]->table_size)
	{
	   dindex = dindex % Table[tnum]->table_size;
	   *(Table[tnum]->table_pointer + dindex) = *tval;
	}
	return(*tval);
}
#endif


#ifndef G2000
static double  
dps_vdelay_op(int code,  int list,  int rtindex)
{
	ARRAY_DATA  *d_list;
	int	    v_num;
	double	    fval;

	if (rtindex >= v1 && rtindex <= three)
	    v_num = get_RTval(rtindex);
	else if (rtindex >= t1 && rtindex <= t60)
	{
	    v_num = dps_get_ap(rtindex, -1, -1);
	}
	else
	    return(0.0);

	fval = 0.0;
	if (v_num < 0)
	    v_num = 0;
	if (code == VDLST)
	{
	  d_list = com_array_list;
	  if (v_num > 0)
		v_num--;
	  while (d_list != NULL)
	  {
	     if (d_list->id == list)
	     {
		  if (v_num < d_list->size)
		      fval = d_list->vars[v_num];
		  break;
	     }
	     d_list = d_list->next;
	  }
	  if (vFlag > 1)
	     printf("   vdelay_list: (%d) %f\n",v_num, fval);
	  return(fval);
	}
	switch (list) {
	  case 1:  /* NSEC  */
		fval = 1.0e-9 * (double) v_num;
		break;
	  case 2:  /* USEC  */
		fval = 1.0e-6 * (double) v_num;
		break;
	  case 3:  /* MSEC  */
		fval = 1.0e-3 * (double) v_num;
		break;
	  case 4:  /* SEC  */
		fval = (double) v_num;
		break;
	}
	return(fval);
}
#endif


static
RTNODE *dps_if_stmt(RTNODE *node, int ifstmt)
{
	int	level, val, v1, v2;
	RTNODE  *tnode;

        val = 0;
	if (ifstmt == 1)
	{
	    val = get_RTval(node->vals[0]);
	    if (val == 0)
		return(node);
	}
	if (ifstmt == 2)
	{
            v1  = get_RTval(node->vals[1]);
            v2  = get_RTval(node->vals[2]);
            switch (node->vals[0]) {
                case RT_LT:
                         if (v1 < v2)
                            val = 1;
                         break;
                case RT_GT:
                         if (v1 > v2)
                            val = 1;
                         break;
                case RT_GE:
                         if (v1 >= v2)
                            val = 1;
                         break;
                case RT_LE:
                         if (v1 < v2)
                            val = 1;
                         break;
                case RT_EQ:
                         if (v1 == v2)
                            val = 1;
                         break;
                case RT_NE:
                         if (v1 != v2)
                            val = 1;
                         break;
            }
            set_RTval(node->vals[3], val);
            if (val != 0)
                return(node);
	}
	level = 0;
	tnode = node;
	while (tnode->next != NULL)
	{
	    tnode = tnode->next;
            switch (tnode->code) {
             case IFZERO:
                        level++;
                        break;
             case IFRT:
                        level++;
                        break;
             case ELSENZ:
                        if (ifstmt != 0 && level == 0)
                            return(tnode);
                        break;
             case ENDIF:
                        if (level == 0)
                            return(tnode);
                        level--;
                        break;
	    }
	}
	return(tnode);
}


static void
set_dps_loop(RTNODE *node, int vindex)
{
	if (loopLevel >= MAXLEVEL - 1)
	    return;
	loopLevel++;
	timers[loopLevel] = 0.0;
	loopRcnt[loopLevel] = get_RTval(vindex);
	if (loopRcnt[loopLevel] < 1)
	    loopRcnt[loopLevel] = 1;
	loopDo[loopLevel] = 0;
	loopStack[loopLevel] = node;
	if (vFlag > 1)
	    printf("  loop(%d): %d loops level: %d\n", node->code, loopRcnt[loopLevel], loopLevel);
}


static void
set_gemini_loop(RTNODE *node, int count)
{
	if (loopLevel >= MAXLEVEL - 1)
	    return;
	loopLevel++;
	loopRcnt[loopLevel] = count;
	if (loopRcnt[loopLevel] < 1)
	    loopRcnt[loopLevel] = 1;
	loopDo[loopLevel] = 0;
	loopStack[loopLevel] = node;
	if (vFlag > 1)
	    printf("  loop(%d): %d loops level: %d\n", node->code, loopRcnt[loopLevel], loopLevel);
}


static
RTNODE *end_dps_loop(RTNODE *node, int vindex)
{
	int	k;
	double  ftime;

	if (loopDo[loopLevel])
	{
	    if (loopRcnt[loopLevel] > 1)
	    {
		loopRcnt[loopLevel]--;
		if (gloop)
		   set_RTval(gloopRt[vindex], (double) loopRcnt[loopLevel]);
		else
		   set_RTval(vindex, (double) loopRcnt[loopLevel]);
                if (loopStack[loopLevel] != NULL)
		   return(loopStack[loopLevel]);
                else
                   return (node);
	    }
	}
	ftime = 0.0;
	for (k = loopLevel; k < MAXLEVEL; k++)
	{
	    ftime += timers[k];
	    timers[k] = 0.0;
	    loopDo[k] = 0;
	}
	if (vFlag)
            printf(" loop time %g  count %d \n", ftime, loopRcnt[loopLevel]);
	if (loopRcnt[loopLevel] < 1)
	    loopRcnt[loopLevel] = 1;
	ftime = ftime * (double) loopRcnt[loopLevel];
	loopRcnt[loopLevel] = 1;
	if (loopLevel > 0)
	    loopLevel--;
	timers[loopLevel] += ftime;
	return(node);
}

static
void set_parallel_time()
{
    double ftime;
    int    k;

    if (parallelChan <= 0 || loopLevel < 1)
        return; 
    if (vFlag)
        printf(" previous parallel channel %d \n", parallelChan);
    ftime = 0.0;
    for (k = loopLevel; k < MAXLEVEL; k++)
    {
        ftime += timers[k];
        timers[k] = 0.0;
        loopDo[k] = 0;
    }
    if (vFlag)
        printf(" previous parallel channel %d time: %g\n", parallelChan, ftime);
    parallelTimes[parallelChan] = ftime;
    loopLevel--;
}

static
void start_parallel_time(RTNODE *node)
{
      if (vFlag)
          printf("parallelstart \n");
      set_parallel_time();
      parallelChan = node->vals[0];
      if (vFlag)
          printf(" set parallel channel %d\n", parallelChan);
      if (parallelChan >= TOTALCH || parallelChan < 0) {
          parallelChan = 0;
          return;
      }
      set_dps_loop(node, 0);
}

static
void end_parallel_time(RTNODE *node)
{
      int k;
      double t;

      if (vFlag)
          printf("parallelend \n");
      set_parallel_time();
      t = 0.0;
      for (k = 1; k < TOTALCH; k++)
      {
           if (parallelTimes[k] > t)
              t = parallelTimes[k];
           parallelTimes[k] = 0.0;
      }
      if (vFlag)
          printf(" parallel time: %g \n", t);
      timers[loopLevel] += t;
      parallelChan = 0;
}


static void simulate_dps_code(int ct_num)
{
	int	v1, delayEvent;
	int	kzloop;
	double	f1, exTime;
	RTNODE  *cnode;

	loopLevel = 0;
	gotAcquire = 0;
	rotorTime = 0.0;
	
	for(v1 = 0; v1 < MAXLEVEL; v1++)
	{
	    timers[v1] = 0.0;
	    loopRcnt[v1] = 1;
	    loopStack[loopLevel] = NULL;
	    loopDo[v1] = 0; /* the flag of actual loops to be run, default is no */
	}
	for(v1 = 0; v1 < 5; v1++)
	    incDelays[v1] = 0.0;
        parallelChan = 0;
	for(v1 = 0; v1 < TOTALCH; v1++)
            parallelTimes[v1] = 0.0;
	RTval[ct] = ct_num;
	cnode = first_rtnode;
	exTime = 0.0;
        kzloop = 0;
	while (cnode != NULL)
	{
	   delayEvent = 0;
#ifndef NVPSG
#if !defined(G2000) || defined(MERCURY)
	   exTime += 10.0e-5;
#else
	   exTime += 20.0e-5;
#endif
#endif
	   if (cnode->width > 0.0 && kzloop == 0)
	       timers[loopLevel] += cnode->width;
	   switch (cnode->code) {
#ifndef G2000
	    case  ACQUIRE:
			gotAcquire = 1;
			delayEvent = ACQUIRE_START_EVENT;
			break;
	    case  PELOOP2:
			if (cnode->fval > 0.5)
			{
			    set_RTval(cnode->vals[0], cnode->fval);
			    set_RTval(cnode->vals[1],(double)(nth2D-1));
			}
			else
			{
			    set_RTval(cnode->vals[0], 0.0);
			    set_RTval(cnode->vals[1], 0.0);
			}
			break;
	    case  GETELEM:
			dps_get_ap(cnode->vals[0],cnode->vals[1],cnode->vals[2]);
			break;
	    case  INCDLY:
			v1 = get_RTval(cnode->vals[0]);
			f1 = incDelays[cnode->vals[1]] * v1;
			timers[loopLevel] += f1;
			loopDo[loopLevel] = 1;
			break;
	    case  VDELAY:
	    case  VDLST:
			timers[loopLevel] += dps_vdelay_op(cnode->code,
				cnode->vals[0], cnode->vals[1]);
			loopDo[loopLevel] = 1;
			break;
	    case  OFFSET:
	    case  LOFFSET:
	    case  VOFFSET:
	    case  POFFSET:
	   		delayEvent = OFFSET_FREQ_EVENT;
			break;
#endif
#if !defined(G2000) || defined(MERCURY)
	    case  TBLOP:
			if (ct_num == 0)
			  rt_tbl_cmd(1, cnode);
			break;
	    case  TSOP:
			if (ct_num == 0)
			  rt_tbl_cmd(0, cnode);
			break;
#endif
	    case  RVOP:
			rt_val_cmd(cnode);
			break;
	    case  SLOOP:
			set_dps_loop (cnode, cnode->vals[0]);
			set_RTval(cnode->vals[1], (double) loopRcnt[loopLevel]);
			break;
	    case  GLOOP:
			set_gemini_loop (cnode, cnode->vals[1]);
			set_RTval(cnode->vals[2], (double) loopRcnt[loopLevel]);
			break;
	    case  HLOOP:
			set_dps_loop (cnode, cnode->vals[0]);
			break;
	    case  MSLOOP:
			set_RTval(cnode->vals[0], cnode->fval);
			set_RTval(cnode->vals[1], cnode->fval);
			set_dps_loop (cnode, cnode->vals[0]);
			break;
	    case  MSLOOP2:
			set_RTval(cnode->vals[0], cnode->fval);
			set_RTval(cnode->vals[1], 0.0);
			break;
	    case  PELOOP:
			set_RTval(cnode->vals[0], cnode->fval);
			set_RTval(cnode->vals[1], cnode->fval);
			set_dps_loop (cnode, cnode->vals[0]);
			break;
	    case  ENDSISLP:
			cnode = end_dps_loop(cnode, cnode->vals[0]);
			break;
	    case  ENDHP:
			cnode = end_dps_loop(cnode, 0);
			break;
	    case  ENDSP:
			cnode = end_dps_loop(cnode, cnode->vals[0]);
			break;
	    case  IFZERO:
			cnode = dps_if_stmt(cnode, 1);
			break;
	    case  ELSENZ:
			cnode = dps_if_stmt(cnode, 0);
			break;
	    case  IFRT:
			cnode = dps_if_stmt(cnode, 2);
			break;
	    case  SETVAL:
			set_RTval(cnode->vals[0], cnode->fval);
			break;
	    case  ROTORS:
			f1 = rotorTime * (double)get_RTval(cnode->vals[0]);
			timers[loopLevel] += f1;
			break;
	    case  ROTORP:
			rotorTime = 1.0e-9 * (double)get_RTval(cnode->vals[0]);
			break;
	    case  PARALLELSTART:
                        start_parallel_time(cnode);
			break;
	    case  PARALLELEND:
                        end_parallel_time(cnode);
			break;
	    case  PARALLELSYNC:
			break;
	    case  RLLOOP:
                        if (vFlag)
                          printf("start rlloop count: %d\n", cnode->vals[0]);
			set_RTval(cnode->vals[1], (double) cnode->vals[0]);
                        set_dps_loop (cnode, cnode->vals[1]);
                        set_RTval(cnode->vals[2], (double) loopRcnt[loopLevel]);
			break;
	    case  RLLOOPEND:
                        if (vFlag)
                           printf("end rlloop \n");
			cnode = end_dps_loop(cnode, cnode->vals[0]);
			break;
	    case  KZLOOP:
                        kzloop = 1;
			break;
	    case  KZLOOPEND:
                        kzloop = 0;
			break;
	    case  NWLOOP:
                        set_RTval(cnode->vals[0], cnode->fval);
                        set_RTval(cnode->vals[1], cnode->fval);
                        set_dps_loop (cnode, cnode->vals[0]);
			break;
	    case  NWLOOPEND:
			cnode = end_dps_loop(cnode, cnode->vals[0]);
			break;
	    case  LAST_CODE:
			return;
			break;
	   }
#ifndef G2000
	   if (delayEvent > 0)
		timers[loopLevel] += eventovrhead(delayEvent);
#endif
	   cnode = cnode->next;
	}
	timers[loopLevel] += exTime;
}



static void
insert_dps_gvar(const char *name, double *vaddr)
{
	static int  count = 0;

	if (count < GVMAX)
	{
		strncpy(globals[count].name, name, PARAMLEN);
		globals[count].vaddr = vaddr;
		count++;
	}
}

static void
set_dps_gvars()
{
	int	k;
	double	dum;

	for (k = 0; k < GVMAX; k++) {
		globals[k].name[0] = '0';
		globals[k].name[PARAMLEN] = '0';
        }
	insert_dps_gvar("d1", &d1);
	insert_dps_gvar("d2", &d2);
	insert_dps_gvar("pw", &pw);
	insert_dps_gvar("p1", &p1);
	insert_dps_gvar("pad", &pad);
	insert_dps_gvar("rof1", &rof1);
	insert_dps_gvar("rof2", &rof2);
	insert_dps_gvar("hst", &hst);
	insert_dps_gvar("alfa", &alfa);
	insert_dps_gvar("sw", &sw);
	insert_dps_gvar("np", &np);
	insert_dps_gvar("nt", &nt);
	insert_dps_gvar("sfrq", &sfrq);
	insert_dps_gvar("dfrq", &dfrq);
	insert_dps_gvar("fb", &fb);
	insert_dps_gvar("bs", &bs);
	insert_dps_gvar("tof", &tof);
	insert_dps_gvar("dof", &dof);
	insert_dps_gvar("gain", &gain);
	insert_dps_gvar("dlp", &dlp);
   	insert_dps_gvar("dhp", &dhp);
   	insert_dps_gvar("tpwr", &tpwr);
   	insert_dps_gvar("temp", &vttemp);
   	insert_dps_gvar("dmf", &dmf);
   	insert_dps_gvar("vtc", &vtc);
   	insert_dps_gvar("vtwait", &vtwait);

#ifndef G2000
   	insert_dps_gvar("dpwr", &dpwr);
#else
   	insert_dps_gvar("dpwr", &dhp);
#endif
#ifndef G2000
	insert_dps_gvar("d3", &d3);
	insert_dps_gvar("d4", &d4);
	insert_dps_gvar("pw90", &pw90);
	insert_dps_gvar("nf", &nf);
	insert_dps_gvar("dfrq2", &dfrq2);
	insert_dps_gvar("dfrq3", &dfrq3);
	insert_dps_gvar("dof2", &dof2);
	insert_dps_gvar("dof3", &dof3);
   	insert_dps_gvar("dpwr2", &dpwr2);
   	insert_dps_gvar("dpwr3", &dpwr3);
   	if ( P_getreal(CURRENT,"tpwrm",&dum,1) == 0 )
      	   insert_dps_gvar("tpwrm", &tpwrf);
   	else
      	   insert_dps_gvar("tpwrf", &tpwrf);
   	if ( P_getreal(CURRENT,"dpwrm",&dum,1) == 0 )
      	   insert_dps_gvar("dpwrm", &dpwrf);
   	else
      	   insert_dps_gvar("dpwrf", &dpwrf);
   	insert_dps_gvar("dpwrm2", &dpwrf2);
   	insert_dps_gvar("dpwrm3", &dpwrf3);
   	insert_dps_gvar("dmf2", &dmf2);
   	insert_dps_gvar("dmf3", &dmf3);
   	insert_dps_gvar("pwx", &pwx);
   	insert_dps_gvar("pwxlvl", &pwxlvl);
   	insert_dps_gvar("tau", &tau);
   	insert_dps_gvar("satdly", &satdly);
   	insert_dps_gvar("satfrq", &satfrq);
   	insert_dps_gvar("satpwr", &satpwr);
#endif
}

static void
set_dps_arrays()
{
        vInfo   vinfo;
	int	k, size;
	double  fval, *f_list;
	ARRAY_DATA *v_list;
	char	**vptr;


	if (array_list == NULL)
	    return;
	v_list = array_list;
     	while (v_list != NULL)
     	{
     	    if ( P_getVarInfo(CURRENT,v_list->name,&vinfo) != 0 )
	    {
		v_list->type = 0;
		v_list->size = 0;
	    }
	    else
	    {
	        v_list->type = vinfo.basicType;
	        v_list->size = vinfo.size;
	        if (vinfo.size <= 0)
	    	{
		    v_list->type = 0;
		    v_list->size = 0;
		}
	    }
	    if(v_list->type == T_REAL)
	    {
	        size = vinfo.size;
		if ((v_list->xtype != XARRAY) && (is_ni_name(v_list->name) > 0))
		   size = 1;
		v_list->vars = (double *) malloc(sizeof(double) * size);
		f_list = v_list->vars;
		for(k = 1; k <= size; k++)
		{
		   if(P_getreal(CURRENT,v_list->name,&fval,k) < 0)
				fval = 0.0;
		   f_list[k-1] = fval;
		}
	    }
	    else if(v_list->type == T_STRING)
	    {
	        size = vinfo.size;
		v_list->vstrs = (char **) malloc(sizeof(char *) * size);
		vptr = v_list->vstrs;
		for(k = 1; k <= size; k++)
		{
	   	   if ( P_getstring(CURRENT, v_list->name, tmpData, k, 30) >= 0)
		   {
		      vptr[k-1] = (char *) malloc(strlen(tmpData) + 1);
		      strcpy(vptr[k-1], tmpData);
		   }
		   else
		   {
		      vptr[k-1] = (char *) malloc(4);
		      strcpy(vptr[k-1], "n");
		   }
		}
	    }
	    v_list = v_list->next;
     	}
}

void
dps_pbox_pulse(char *name, int dev, shape *rshape, int iph, double d1, double d2,
               char *s1, char *s2, char *s3, char *s4)
{
     if (dpsTimer != 0)
          return;
    DPSprint(0.0, "%d power %d 0 1 %s->pwr %.9f \n",XRLPWR,dev,s1, (float)rshape->pwr);
    if (rshape->pwrf)
       DPSprint(0.0, "%d pwrf %d 0 1 %s->pwrf %.9f \n",XRLPWRF,dev,s1, (float)rshape->pwrf);
    DPSprint(rshape->pw, "%d %s  1 1 1 1 %s %.9f %s %.9f  %d  ?%s %s %d %s->pw %.9f \n",
       SHPUL,name,s3,d1,s4,d2, dev,rshape->name, s2,iph,s1,(float)rshape->pw);
}

void
dps_pbox_spinlock(char *name, int dev, shape *rshape, double d1, int iph,
                  char *s1, char *s2, char *s3)
{
    int loops;

     if (dpsTimer != 0)
          return;
    loops = (int) (0.5 + d1/rshape->pw);
    if (loops > 0)
    {
	DPSprint(0.0, "%d power %d 0 1 %s->pwr %.9f \n",XRLPWR,dev,s1, (float)rshape->pwr);
        if (rshape->pwrf)
           DPSprint(0.0, "%d pwrf %d 0 1 %s->pwrf %.9f \n",XRLPWRF,dev,s1, (float)rshape->pwrf);
	DPSprint(0.0, "%d %s  %d 3 2  ?%s %d %s %d %s/%s->pw %d  1.0/%s->dmf %.9f %s->dres %.9f \n",
	  SPINLK,name,dev, rshape->name,iph, s3,iph,s2,s1, loops, s1,(float)(1.0/rshape->dmf), s1, (float)(rshape->dres));
    }
}

void
dps_pbox_xmtron(char *name, int dev, shape *rshape, char *s1)
{
     if (dpsTimer != 0)
          return;
    DPSprint(0.0, "%d power %d 0 1 %s->pwr %.9f \n",XRLPWR,dev,s1, (float)rshape->pwr);
    if (rshape->pwrf)
       DPSprint(0.0, "%d pwrf %d 0 1 %s->pwrf %.9f \n",XRLPWRF,dev,s1, (float)rshape->pwrf);

    DPSprint(0.0, "%d %s %d 0 3 ?%s  %.9f 1.0/%s->dmf %.9f %s->dres %.9f \n",
	XPRGON,name,dev, rshape->name, (float)(1.0/rshape->dmf), s1,(float)(1.0/rshape->dmf), s1,(float)rshape->dres);
    DPSprint(0.0, "%d %s  %d 1 0 on 1 \n", XDEVON, name, dev);
}

void
dps_pbox_xmtroff(char *name, int dev)
{
     if (dpsTimer != 0)
          return;
    DPSprint(0.0, "%d %s  %d 1 0 on 0 \n", XDEVON, name, dev);
    DPSprint(0.0, "%d %s  %d 1 0 off 0 \n", XPRGOFF,name, dev);
}

void
dps_pbox_simpulse(name, dev, rshape, rshape2, iph, iph2, d1, d2, s1, s2,
		  s3, s4, s5, s6)
char   *name;
int    dev;
shape  *rshape, *rshape2;
int    iph, iph2;
double d1, d2;
char   *s1, *s2, *s3, *s4, *s5, *s6;
{
     if (dpsTimer != 0)
          return;
    DPSprint(0.0, "%d power %d 0 1 %s->pwr %.9f \n",XRLPWR,TODEV, s1, (float)rshape->pwr);
    if (rshape->pwrf)
       DPSprint(0.0, "%d pwrf %d 0 1 %s->pwrf %.9f \n",XRLPWRF,TODEV,s1,(float)rshape->pwrf);
    DPSprint(0.0, "%d power %d 0 1 %s->pwr %.9f \n",XRLPWR,DODEV,s2, (float)rshape2->pwr);
    if (rshape2->pwrf)
       DPSprint(0.0, "%d pwrf %d 0 1 %s->pwrf %.9f \n",XRLPWRF,DODEV,s2, (float)rshape2->pwrf);
     DPSprint(rshape->pw, "%d %s 2 1 1 1 %s %.9f %s %.9f 1 ?%s %s %d %s->pw %.9f  2 ?%s %s %d %s->pw %.9f \n",
	SMSHP, name, s5,(float)d1, s6,(float)d2, rshape->name,s3, iph, s1, (float)rshape->pw, rshape2->name, s4,iph2, s2, (float)rshape2->pw);
}


void
dps_pbox_sim3pulse(name, dev, rshape,rshape2,rshape3,iph,iph2,iph3, d1,d2,
		s1,s2,s3,s4,s5,s6,s7,s8)
char   *name;
int    dev;
shape  *rshape, *rshape2, *rshape3;
int    iph, iph2, iph3;
double d1, d2;
char   *s1, *s2, *s3, *s4, *s5, *s6, *s7, *s8;
{
     if (dpsTimer != 0)
          return;
    DPSprint(0.0, "%d power %d 0 1 %s->pwr %.9f \n",XRLPWR,TODEV, s1, (float)rshape->pwr);
    if (rshape->pwrf)
       DPSprint(0.0, "%d pwrf %d 0 1 %s->pwrf %.9f \n",XRLPWRF,TODEV,s1,(float)rshape->pwrf);
    DPSprint(0.0, "%d power %d 0 1 %s->pwr %.9f \n",XRLPWR,DODEV,s2, (float)rshape2->pwr);
    if (rshape2->pwrf)
       DPSprint(0.0, "%d pwrf %d 0 1 %s->pwrf %.9f \n",XRLPWRF,DODEV,s2, (float)rshape2->pwrf);
    DPSprint(0.0, "%d power %d 0 1 %s->pwr %.9f \n",XRLPWR,DO2DEV,s3, (float)rshape3->pwr);
    if (rshape3->pwrf)
       DPSprint(0.0, "%d pwrf %d 0 1 %s->pwrf %.9f \n",XRLPWRF,DO2DEV,s3, (float)rshape3->pwrf);

    DPSprint(rshape->pw, "%d %s  3 1 1 1 %s %.9f %s %.9f  1 ?%s %s %d %s->pw %.9f  2 ?%s %s %d %s->pw %.9f  3 ?%s %s %d %s->pw %.9f \n",
      SMSHP,name,s7,(float)d1, s8,(float)d2, rshape->name,s4,iph,s1, (float)rshape->pw, rshape2->name, s5,iph2,s2,(float)rshape2->pw, rshape3->name,s6,iph3,s3, (float)rshape3->pw);

}


int
dps_create_rotation_list(name, code, angle_set, num_sets)
char   *name;
int    code, num_sets;
double *angle_set;
// double angle_set[][3];
{
     int n, k, r;
     ROTATION_NODE *node, *pnode;
     double *ptrSet;
    
      if (dpsTimer != 0)
          return(0);
     if (name == NULL)
         abort_message("invalid name of coordinate rotation angle sets (null). abort!\n");
     // if (num_sets <= 0)
     //     abort_message("invalid number of coordinate rotation angle sets (%d) in create_rotation_list command. abort!\n", num_sets);
     // if (num_sets > 16384)   /* 16K rotation angle sets */
     //  abort_message("number of gradient rotation angle sets (%d > 16384) in create_rotation_list command too large. abort!\n", num_sets);
     
     pnode = rotateList;
     n = 0;
     node = NULL;
     ptrSet = angle_set;
     while (pnode != NULL) {
          n = pnode->id;
          if (strcmp(name, pnode->name) == 0) {
              node = pnode;
              break;
          }
          if (pnode->next == NULL)
              break;
          pnode = pnode->next;
     }
   
     if (node == NULL) {
         node = (ROTATION_NODE *) malloc(sizeof(ROTATION_NODE));
         if (node == NULL)
             return 0;
         node->name = (char *) malloc(strlen(name) + 2);
         strcpy(node->name, name);
         node->id = n + 1;
         node->next = NULL;
         if (pnode == NULL)
             rotateList = node;
         else
             pnode->next = node;
     }
     node->code = code;
     node->size = num_sets;
     k = num_sets;
     if (k > 10)
         k = 10;
     for (r = 0; r < k; r++) {
         ptrSet = angle_set + r*3;
         node->angle_set[r][0] = *ptrSet;
         node->angle_set[r][1] = *(ptrSet+1);
         node->angle_set[r][2] = *(ptrSet+2);
     }
     for (r = k; r < 10; r++) {
         node->angle_set[r][0] = 0;
         node->angle_set[r][1] = 0;
         node->angle_set[r][2] = 0;
     }
     return node->id;
}

int
dps_create_angle_list(name, code, angle_set, num_sets)
char   *name;
int    code, num_sets;
double *angle_set;
{
     int n, k, r;
     ANGLE_NODE *node, *pnode;
    
      if (dpsTimer != 0)
          return(0);
     if (name == NULL)
         abort_message("invalid name of coordinate angle sets (null). abort!\n");
     pnode = angleList;
     n = 0;
     node = NULL;
     while (pnode != NULL) {
          n = pnode->id;
          if (strcmp(name, pnode->name) == 0) {
              node = pnode;
              break;
          }
          if (pnode->next == NULL)
              break;
          pnode = pnode->next;
     }
   
     if (node == NULL) {
         node = (ANGLE_NODE *) malloc(sizeof(ANGLE_NODE));
         if (node == NULL)
             return 0;
         node->id = n + 1;
         node->next = NULL;
         node->name = (char *) malloc(strlen(name) + 2);
         strcpy(node->name, name);
         if (pnode == NULL)
             angleList = node;
         else
             pnode->next = node;
     }
     node->code = code;
     node->size = num_sets;
     k = num_sets;
     if (k > 10)
         k = 10;
     for (r = 0; r < k; r++) {
         node->angle_set[r] = angle_set[r];
     }
     for (r = k; r < 10; r++) {
         node->angle_set[r] = 0;
     }
     
     return node->id;
}


int
dps_initRFGroupPulse(name, code,dcode, pw, pat, mode, cpwr, fpwr, sphase, fa,
                    num,s1,s2,s3,s4,s5,s6,s7, s8)
char	*name, *pat, mode;
int	code, dcode, num;
double  pw, cpwr, fpwr, sphase, *fa;
char	*s1,*s2,*s3,*s4,*s5,*s6,*s7, *s8;
{
        int n, k;
        RFGROUP_NODE *node, *pnode;

       rfGroupNum++;
       if (dpsTimer != 0)
       {
           node = (RFGROUP_NODE *) malloc(sizeof(RFGROUP_NODE));
           node->id = rfGroupNum;
           node->width = pw;
           node->next = NULL;
           if (rfgrpList == NULL)
               rfgrpList = node;
           else {
               pnode = rfgrpList;
               while (pnode->next != NULL)
                   pnode = pnode->next;
               pnode->next = node;
           }
           return (rfGroupNum);
       }
       DPSprint(0.0, "%d %s 1 %d %s %c 1 5 %s %d  ",code,name, rfGroupNum, pat,mode,  s8, num);
       DPSprint(0.0, "%s %.9f %s %.9f %s %.9f %s %.9f %s %.9f \n", s1, pw, s4, cpwr, s5, fpwr, s6, sphase, s7, fa[0]);  
       n = num;
       if (n > 10)
           n = 10;
       DPSprint(0.0, "%d %s 1 %d %d ",INITFRQLIST, name, rfGroupNum, n);
       for (k = 0; k < n; k++)
           DPSprint(0.0, "%.9f ", fa[k]);
       DPSprint(0.0, " \n");
       return (rfGroupNum);
}

void
dps_GroupPulse(name, code,dcode, grpId, rof1,rof2, vphase, vselect,
                    s1,s2,s3,s4,s5)
char	*name;
int	code, dcode, grpId, vphase, vselect;
double  rof1, rof2;
char	*s1,*s2,*s3,*s4,*s5;
{
        RFGROUP_NODE *node;

	if (dpsTimer != 0)
	{
            node = rfgrpList;
            while (node != NULL) {
                 if (node->id == grpId)
                        break;
                 node = node->next;
            }
            if (node != NULL)
	        add_delay_node(node->width);
	    return;
	}
       DPSprint(0.0, "%d %s 1 3 2  %s %d %s %d %s %d ",code,name, s1, grpId,
                s4, vphase, s5, vselect);
       DPSprint(0.0, " %s %.9f %s %.9f \n", s2, rof1, s3, rof2);
}

void
dps_TGroupPulse(name, code,dcode, grpId, rof1,rof2, vphase, vselect,
                    s1,s2,s3,s4,s5)
char	*name;
int	code, dcode, grpId, vphase, vselect;
double  rof1, rof2;
char	*s1,*s2,*s3,*s4,*s5;
{
	dps_GroupPulse(name, code,dcode, grpId, rof1,rof2, vphase, vselect,
                    s1,s2,s3,s4,s5);
}

void
dps_modifyRFGroupFreqList(name, code,dcode, grpId, num, fa,
                    s1,s2,s3)
char	*name;
int	code, dcode, grpId, num;
double  *fa;
char	*s1,*s2,*s3;
{
      if (dpsTimer != 0)
          return;
      DPSprint(0.0, "%d %s 1 2 1  %s %d %s %d ",code,name, s1, grpId,s2,num);
      DPSprint(0.0, "%s %.9f \n", s3, fa[0]);
}

int
dps_new_shapelist(char *name, int code, int dcode,
     char *pattern,double width,double *listarray,double nsval,double frac,char mode, int chan,
     char *s1, char *s2, char *s3, char *s4, char *s5, char *s6, char *s7)
{
     VNAME_LIST  *v_node, *n_node;

     if (dpsTimer != 0)
     {
         // add_delay_node(width);
         return 0;
     }
     // id = gen_shapelist_init(pattern, width, listarray, nsval, 1.0 - frac, mode, OBSch);
     if (pattern == NULL)
         return 0;
	
     v_node = shape_list;
     while (v_node != NULL)
     {
        if (strcmp(pattern, v_node->name) == 0)
	   return v_node->id;
	if (v_node->next == NULL)
	   break;
	v_node = v_node->next;
     }
     n_node = (VNAME_LIST *) calloc(1, sizeof(VNAME_LIST));
     if (n_node == NULL)
	return 0;
     shapeListNum++;
     n_node->name = (char *) malloc(strlen(pattern) + 1);
     n_node->id = shapeListNum;
     strcpy(n_node->name, pattern);
     if (shape_list == NULL)
        shape_list = n_node;
     else
         v_node->next = n_node;
     return n_node->id;
}

int
dps_shapelist(char *name, int code, int dcode, char *pattern, double width,
     double *listarray, double nsval, double frac, char mode,
     char *s1, char *s2, char *s3, char *s4, char *s5, char *s6)
{
    return dps_new_shapelist(name,code,dcode,
            pattern,width,listarray,nsval,(1.0-frac),mode,OBSch,
            s1,s2,s3,s4,s5,s6,"OBSch");
}

void
dps_new_shapedpulselist(char *name, int code, int dcode,
     int listId,double width,int rtvar,double rof1,double rof2,char mode,int vvar, int chan,
     char *s1,char *s2,char *s3,char *s4,char *s5,char *s6,char *s7, char *s8)
{
     VNAME_LIST  *v_node;

     if (dpsTimer != 0)
     {
         add_delay_node(width);
         return;
     }
     if (listId < 1)
         return;
     v_node = shape_list;
     while (v_node != NULL)
     {
        if (v_node->id == listId)
           break;
	v_node = v_node->next;
     }
     if (v_node == NULL)
         return;
     DPSprint(width, "%d %s 1 1 1 2 %s %g %s %g %d ?%s %s %d %s %g 0.0 0.0 \n",
         code,name, s4,rof1,s5,rof2, chan, v_node->name,s3, rtvar,s2,width); 
}

void
dps_shapedpulselist(char *name, int code, int dcode, int listId, double width,
     int rtvar, double rof1, double rof2, char mode, int vvar,
     char *s1, char *s2, char *s3, char *s4, char *s5, char *s6, char *s7)
{
    dps_new_shapedpulselist(name,code,dcode,
             listId,width,rtvar,rof1,rof2,mode,vvar,OBSch,
             s1,s2,s3,s4,s5,s6,s7, "OBSch");
}

