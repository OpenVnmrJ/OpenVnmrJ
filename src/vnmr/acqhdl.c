/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*---------------------------------------------------------------------------
|	acqhandler.c
|
|    The acquistion handler now tracks the current element using the
|    parameter ``celem''; this is reported at FID complete.  09/01/88
|
|    `gain', `ct' get updated in both the current and the processed
|    parameter sets when data received from the acquisition system.
|							06/89
|
|    added recon() that attempt to reconnect this Vnmr user back to his
|		experiments that are active or queued.
| 
+---------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <ctype.h>
#include "vnmrsys.h"
#include "data.h"
#include "errorcodes.h"
#include "variables.h"
#include "group.h"
#include "tools.h"
#include "locksys.h"
#include "acqerrmsges.h"	/* contains static acq error msge struct */
#include "shims.h"	        /* contains shim dac information */
#include "whenmask.h"	        /* contains when mask information */
#include "acquisition.h"	        /* contains when mask information */
#include "pvars.h"
#include "wjunk.h"
#include "sockinfo.h"

/*---------------------------------------------------------------------------
|	acqhandler routines
|
+---------------------------------------------------------------------------*/

#include "ACQ_HAL.h"		/* defines codes describing at what */
				/* point the experiment stopped */
#include "ACQ_SUN.h"		/* defines done (or status) codes */

#define CHANGEPARAM   10		/*  Definition for "werr" command */
#define RECONREQUEST  20		/*  reconnect request */
#define FGREPLY 	5
#define FGNOREPLY 	6
#define MAXSTR 	256
#define RESUME_ACQ_BIT 0x1000
#define BUFLEN 100
#define NUM_MONTHS 12

#ifdef  DEBUG
extern int	Aflag;
#define APRINT0(str) \
	if (Aflag) Wscrprintf(str)
#define APRINT1(str, arg1) \
	if (Aflag) Wscrprintf(str,arg1)
#define APRINT2(str, arg1, arg2) \
	if (Aflag) Wscrprintf(str,arg1,arg2)
#define APRINT3(str, arg1, arg2, arg3) \
	if (Aflag) Wscrprintf(str,arg1,arg2,arg3)
#define APRINT4(str, arg1, arg2, arg3, arg4) \
	if (Aflag) Wscrprintf(str,arg1,arg2,arg3,arg4)
#else 
#define APRINT0(str) 
#define APRINT1(str, arg2) 
#define APRINT2(str, arg1, arg2) 
#define APRINT3(str, arg1, arg2, arg3) 
#define APRINT4(str, arg1, arg2, arg3, arg4) 
#endif 

extern char *fgets_nointr(char *datap, int count, FILE *filep );
extern int acquisition_ok(char *cmdname);
extern int is_acqproc_active();
extern void  Wturnoff_buttons();
extern int expdir_to_expnum(char *expdir);
extern void setMenuName(const char *nm);
extern void appendTopPanelParam();
extern void disp_expno();
extern int disp_current_seq();
extern void clearExpDir(const char *curexpdir);
extern int get_h1freq();
extern void p11_setcurrfidid();
extern int lockExperiment(int expn, int mode );

extern char     datadir[];		/* Set in smagic.c */
extern int      mode_of_vnmr;
extern int      showFlushDisp;
extern int	df_repeatscan;
extern char     UserName[];
extern int      skipFlush;

void saveGlobalPars(int sv, char *suff);

static int	cur_element;
static int	silentMode = 0;
int acqStartUseRtp = 0;
static int acqStartLog = 0;
static int werrCalling = 0;
static int whenProcessing = 0;

/* Need the backslashes in the following format statement or
 * else SCCS will substitute them as ID keywords
 */
static char	t_format[MAXPATH] = "%Y\%m%dT%H\%M\%S";

static char 	MonthStrings[NUM_MONTHS+1][4] = {
"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec",""
};

#ifdef NIRVANA

#define MAX_BRD_TYPE 7

static const char *brdTypeStr[8] = {
		"master%d: ", "rf%d: ", "pfg%d: ", "gradient%d: ", "lock%d: ", "ddr%d: ", 
		"reserved1_%d: ", "reserved2_%d: " };
#endif

static char	abortMsg[MAXSTR] = "";

struct UsrMsgList {
    int code;
    char *errmsg;
    struct UsrMsgList *next;
};

int calledFromWerr()
{
   return(werrCalling);
}

void setSilentMode(int newMode)
{
   silentMode = newMode;
}

void setAbortMsg(const char *str)
{
  strcpy(abortMsg,str);
}


/*
 *  This routine deletes the acqpar file when an experiment finishes,
 *  if there is a hard error,  or if the experiment is aborted.
 */
static void deleteAcqPar(char *expname )
{
    char  acqparfil[ MAXPATHL ];

    if (mode_of_vnmr == AUTOMATION)
       sprintf( acqparfil,"%s.fid/acqpar",datadir);
    else
       sprintf( acqparfil,"%s/%s/acqfil/acqpar", userdir,expname );
    unlink(acqparfil);
    strcat(acqparfil,"_hwpar");
    unlink(acqparfil);
}

/*  This routine sets the number of completed elements `celem' in
    the CURRENT and PROCESSED trees.  The main routine
    ``acqstatus'' sets the static variable ``cur_element'' to
    the desired value, except for EXP_COMPLETE, where this
    routine uses the value of ``arraydim''.  celem is created
    and initialized to 0.0 by go.
*/

static void set_current_element()
{
   if (cur_element < 1) {
      cur_element = 1;
   }
   if (P_setreal( CURRENT, "celem", (double) cur_element, 1 ))
   {
      P_creatvar( CURRENT, "celem", T_REAL );
      P_setgroup( CURRENT, "celem", G_ACQUISITION );
      P_setreal( CURRENT, "celem", (double) cur_element, 1 );
   }
   if (P_setreal( PROCESSED, "celem", (double) cur_element, 1 ))
   {
      P_creatvar( PROCESSED, "celem", T_REAL );
      P_setgroup( PROCESSED, "celem", G_ACQUISITION );
      P_setreal( PROCESSED, "celem", (double) cur_element, 1 );
   }
}

static void set_non_arrayed_real(int tree, const char *valname, double val)
{
   vInfo  info;
   int    arr;
  
   arr = ((P_getVarInfo(tree,valname,&info)) ? 1 : info.size);
   if (arr == 1)  /* only set value if the parameter is not arrayed */
      P_setreal( tree, valname, val, 1);
}

void fixup_lockfreq_inova()
{
    int diff_offsets, ival, lkof_exists, new_offset, old_offset, value;
    double console_lockfreq, cur_lockfreq, dval;
    extern double apvalue2lkfreq();

    ival = P_getreal( GLOBAL, "lkof", &dval, 1 );
    lkof_exists = (ival == 0);
    if (lkof_exists)
      old_offset = (int) (dval);
    else
    {
       P_creatvar( GLOBAL, "lkof", T_REAL );
       P_setreal( GLOBAL, "lkof", 0.0, 1 );
       old_offset = 0;
    }
    getExpStatusInt(EXP_LOCKFREQAP, &value);
    console_lockfreq = apvalue2lkfreq( value, get_h1freq() );
    ival = P_getreal( SYSTEMGLOBAL, "lockfreq", &cur_lockfreq, 1 );
    dval = (console_lockfreq - cur_lockfreq) * 1.0e6;

/* round up (or down) to the next larger integer in absolute value */

    if (dval < 0.0)
      dval = dval - 0.5;
    else if (dval > 0.0)
      dval = dval + 0.5;
    new_offset = (int) (dval);

/* Work-a-round to eliminate lock frequency offsets
   of -1 or 1 was put in because the granularity of
   the lock frequency (on INOVA at least) is 2.3 Hz. */

    if (new_offset < 2 && new_offset > -2)	
      new_offset = 0;
    diff_offsets = new_offset - old_offset;

/* Work-a-round insures changes in lock frequency offset are at least 2 Hz.  */

    if (diff_offsets < -2 || diff_offsets > 2)
       P_setreal( GLOBAL, "lkof", (double) new_offset, 1 );
}
	    
static int convertfid(char *expname, int result, int ctval )		/* Name taken from routine in ACQHDL */
{
        char            tempbuf[ 4 ], parampath[ MAXPATH ], console[ MAXSTR ];
	int		curproc,	/* Tree to use, CURRENT or PROCESSED */
			curtree;
        int             index, ival;
        int             errorproc;
        int             isz0active;
        int             statbufok;
        int             value;
        double		dbltmp;
        char            parpath[MAXPATH];

        curproc = (result == SETUP_CMPLT) ? CURRENT : PROCESSED;
        errorproc = ( (result == HARD_ERROR) || (result == SOFT_ERROR) ||
                      (result == EXP_ABORTED) );
	curtree = curproc;

/*  Set ``celem'' if STOP complete, FID complete, or experiment
    complete.  Silently create ``celem'' if it does not exist.	*/

	if ( (result == STOP_CMPLT) || (result == EXP_FID_CMPLT) ||
	     (result == BS_CMPLT) || (result == EXP_COMPLETE) )
        {
		set_current_element();
	}

/*  Set shim values; other stuff from acqpar.  */

        if (mode_of_vnmr == AUTOMATION)
           sprintf( parpath,"%s.fid/",datadir);
        else
           sprintf( parpath,"%s/%s/acqfil/", userdir,expname );
        if (result == SETUP_CMPLT)
           strcat( parpath, "supar_hwpar" );
        else
           strcat( parpath, "acqpar_hwpar" );
        statbufok = open_ia_stat(parpath,(result == SETUP_CMPLT),EXP_STRUC);
        if (ctval > 0)
	   P_setreal( curtree, "ct", (double) ctval, 1 );
        if (statbufok)
        {

            getExpStatusInt(EXP_LKPOWER, &value);
            P_setreal( GLOBAL, "lockpower", (double) value, 1 );
            getExpStatusInt(EXP_LKGAIN, &value);
            P_setreal( GLOBAL, "lockgain", (double) value, 1 );
            getExpStatusInt(EXP_LKPHASE, &value);
            P_setreal( GLOBAL, "lockphase", (double) value, 1 );
            getExpStatusInt(EXP_RCVRGAIN, &value);
	    set_non_arrayed_real( curtree, "gain", (double) value );
            getExpStatusShim(Z0, &value);
            set_non_arrayed_real( GLOBAL, "z0", (double) value );

            init_shimnames(SYSTEMGLOBAL);
            for (index=Z0 + 1; index <= MAX_SHIMS; index++)
            {
               const char *sh_name;
               if ((sh_name = get_shimname(index)) != NULL)
               {
                  getExpStatusShim(index, &value);
                  set_non_arrayed_real(curtree, sh_name, (double) value);
                  set_non_arrayed_real(CURRENT, sh_name, (double) value);
               }
            }

/*  Only VNMR runs this program.  Use the Console parameter; it
    is guaranteed to be correct for the current configuration. */

            if (P_getstring(SYSTEMGLOBAL, "Console", console, 1, MAXSTR - 1) == 0)
            {
               if (strcmp( console, "inova" ) == 0)
               {
                  ival = P_getparinfo(GLOBAL, "z0", &dbltmp, &isz0active);
                  if (ival == 0 && isz0active == 0)
                    fixup_lockfreq_inova();
               }
            }
        }

        /* If PsgFIle exists and in automation mode, copy it back to curexp */
        if (mode_of_vnmr == AUTOMATION)
        {
           sprintf( parpath,"%s.fid/PsgFile",datadir);
           if ( ! access(parpath,R_OK) )
           {
              char tmpStr2[MAXPATH*2];
              sprintf(tmpStr2,"cp -f %s %s/PsgFile",parpath,curexpdir);
              system(tmpStr2);
           }
        }


        /* If error condition, Just get lock and shim parameters */
        if (errorproc)
           RETURN;

/*  Certain parameters need to be set in the current as well as the
    processed tree when the host computer receives acquisition data.	*/

	if (result != SETUP_CMPLT)		/* that is, data received */
          {
                if (statbufok)
                {
                   getExpStatusInt(EXP_RCVRGAIN, &value);
		   set_non_arrayed_real( CURRENT, "gain", (double) value );
                }
                if (ctval > 0)
		   P_setreal( CURRENT, "ct", (double) ctval, 1 );
	  }

/* assume always in current experiment */
       if ( P_getstring(CURRENT,"hsrotor",&tempbuf[ 0 ],1,2) >= 0 )
       {
           if ( (tempbuf[0] == 'y') || (tempbuf[0] == 'Y') )
           {
       	      if ( P_getreal(CURRENT,"srate",&dbltmp ,1) >= 0 )
       	      {
                getExpStatusInt(EXP_SPINACT, &value);
                P_setreal( CURRENT, "srate", (double) value, 1 );
                P_setreal( PROCESSED, "srate", (double) value, 1 );
	      }
           }
       }

      if ((result == EXP_COMPLETE) || (result == SETUP_CMPLT) ||
	  (result == STOP_CMPLT))
      {
        if (mode_of_vnmr != AUTOMATION)
        {
	   strcpy( &parampath[ 0 ], userdir );
	   strcat( &parampath[ 0 ], "/global" );
	   if (P_save( GLOBAL, &parampath[ 0 ] ))
		Werrprintf("Error writing global parameters to %s",
                             &parampath[ 0 ]);
	}

	strcpy( &parampath[ 0 ], curexpdir );
	strcat( &parampath[ 0 ], "/curpar" );
	if (P_save( CURRENT, &parampath[ 0 ] )) {
	   Werrprintf( "Error writing current parameters to %s", &parampath[ 0 ]);
	   ABORT;
	}
	strcpy( &parampath[ 0 ], curexpdir );
	strcat( &parampath[ 0 ], "/procpar" );
	if (P_save( PROCESSED, &parampath[ 0 ] )) {
	   Werrprintf( "Error writing current parameters to %s", &parampath[ 0 ]);
	   ABORT;
	}
      }

      APRINT0("exiting convertfid\n" );
      RETURN;
}

static int acqhandler(char *expn, int scode, int ecode, int ctval )
{
    APRINT2("Entered acqhandler: %d %d\n", scode, ecode);
    if (scode == SETUP_CMPLT)
      if (ecode != EXEC_SHIM && ecode != EXEC_LOCK && ecode != EXEC_SAMPLE)
        RETURN;
    convertfid( expn, scode, ctval );
    RETURN;
}

static struct UsrMsgList *readUsrMsgList()
{
    char buf[128];
    char *msg;
    char path[MAXPATH];
    struct UsrMsgList *msglist, *entry;
    FILE *fd;

    sprintf(path, "%s/templates/acqerrmsgs", userdir);
    if ((fd=fopen(path, "r")) == NULL) {
	sprintf(path, "%s/user_templates/acqerrmsgs", systemdir);
	fd=fopen(path, "r");
    }
    msglist = (struct UsrMsgList *)malloc(sizeof(struct UsrMsgList));
    msglist->code = 0;
    msglist->errmsg = "";
    msglist->next = NULL;
    entry = msglist;
    if (fd) {
	while(fgets(buf, sizeof(buf), fd)) {
	    if (entry->code != 0) {
		entry->next = (struct UsrMsgList *)
		    malloc(sizeof(struct UsrMsgList));
		entry = entry->next;
		entry->next = NULL;
	    }
	    entry->code = (int)strtol(buf, &msg, 10);
	    for ( ; isspace(*msg); msg++); /* trim white space before msg */
	    entry->errmsg = strdup(msg);
	}
        fclose(fd);
    }
    return msglist;
}

static int getUsrErrMsg(char *buf, int wcode)
{
    static struct UsrMsgList *usrMsgList = NULL;
    struct UsrMsgList *uml;

    if (!usrMsgList) {
	usrMsgList = readUsrMsgList();
    }
    for (uml=usrMsgList; uml && uml->code; uml=uml->next) {
	if (uml->code == wcode) {
	    strncpy(buf, uml->errmsg, BUFLEN);
	    buf[BUFLEN-1] = '\0';
	    return 1;
	}
    }
    return 0;
}

#ifdef NIRVANA
static int makeCntlrIdStr(char *buf, int cntlrType, int cntlrNum)
{
  char *unkn = "Unknown Cntlr: ";
  if ( (cntlrType < 0) || (cntlrType >  MAX_BRD_TYPE) )
  {
    strcpy(buf, unkn );
  }
  else
  {
    sprintf(buf,brdTypeStr[cntlrType],cntlrNum);
  }
  return 0;
}
#endif

/******************************/
int dispAcqError(int argc, char *argv[], int retc, char *retv[])
{
char	estring[MAXSTR], *buf;
int     iter, wcode;

    if (argc < 2) 
    {
       double tmp;
       if ( P_getreal( PROCESSED, "acqstatus", &tmp, 2 ) )
       {
          Werrprintf("%s: Cannot get error from acqstatus[2]",argv[0]);
          RETURN;
       }
       wcode = (int) tmp;
    }
    else
    {
       wcode = atoi(argv[1]);
    }
    buf=&estring[0];
    *buf = '\0';
       
    iter = 0;
    while (acqerrMsgTable[ iter ].code != 0)
    {
      if (acqerrMsgTable[ iter ].code == wcode) {
          if (acqerrMsgTable[ iter ].errmsg == NULL) {
              *buf = '\0';
          }
          strcpy( buf, acqerrMsgTable[ iter ].errmsg );
          break;
      }
      else
        iter++;
    }

    if (*buf == 0)
    {
       if (wcode == 0)
         strcpy(buf,"No error");
       else
         strcpy(buf,"No such error");
    }

     if (retc>0) 
     {
        retv[0] = newString(buf);
     }
     else
	Winfoprintf( "%s", estring);
  

     RETURN;
}

/*------------------------------------------------------------------------
|
|	acqstatus/3
|
|	Routine to display the status of an acquisition, as determined
|	by acqProc.  The actual command string is generated by acqProc;
|	the master just executes it.
|
+-----------------------------------------------------------------------*/

static void cleanupVp(const char *dir)
{
   char tmpStr[MAXPATH];

   sprintf(tmpStr,"rm -rf %s",dir);
   system(tmpStr);

}

static void checkVpState(const char *acqdir, int doGlobals)
{
   char tmpStr[MAXPATH];
   const char *ptr;

   ptr = strrchr(acqdir,'/');
   if ( ! ptr)
   {
      ptr = acqdir;
   }
   else
   {
      ptr++;
   }
   if ( P_getstring(PROCESSED,"go_id",tmpStr,1,MAXPATH) || strcmp(tmpStr,ptr) )
   {
      char tmpStr2[MAXPATH*2];
      int ex;

      Wturnoff_buttons();		/* deactive any interactive programs */
      clearExpDir(curexpdir);
      execString("aipDeleteData\n");
      Wclear(2);
      P_treereset(CURRENT); /* clear tree */
      /* It may take some time for the files to appear. They are
       * written after PSG finishes, so putCmd changes can be included.
       * The Exp started message may be send before the procpar file
       * is there. The text file is the last one written. Wait for it.
       */
      sprintf(tmpStr,"%s/text",acqdir);
      ex = 0;
      while ( access(tmpStr,R_OK) && (ex < 500) )  /* 5 sec in 10 msec steps */
      {
         struct timespec timer;

         timer.tv_sec=0;
         timer.tv_nsec = 10000000;   /* 10 msec */
#ifdef __INTERIX
         usleep(timer.tv_nsec/1000);
#else
         nanosleep(&timer, NULL);
#endif
         ex++;
      }
      sprintf(tmpStr,"%s/procpar",acqdir);
      P_read(CURRENT,tmpStr); /* reread saved current tree if in vpmode */
      P_treereset(PROCESSED); /* clear tree */
      P_copy(CURRENT,PROCESSED); /* save current into processed tree if GO */
      sprintf(tmpStr,"%s/acqfil/fid",curexpdir);
      unlink(tmpStr);
      sprintf(tmpStr2,"%s/fid",acqdir);
      /* Wait for the fid file also */
      ex = 0;
      while ( access(tmpStr2,R_OK) && (ex < 50) )
      {
         struct timespec timer;

         timer.tv_sec=0;
         timer.tv_nsec = 10000000;   /* 10 msec */
#ifdef __INTERIX
         usleep(timer.tv_nsec/1000);
#else
         nanosleep(&timer, NULL);
#endif
         ex++;
      }
      link(tmpStr2,tmpStr);

      sprintf(tmpStr2,"cp %s/text %s/text",acqdir,curexpdir);
      system(tmpStr2);
      sprintf(tmpStr,"%s/PsgFile",acqdir);
      if ( ! access(tmpStr,R_OK) )
      {
         sprintf(tmpStr2,"cp -f %s %s/PsgFile",tmpStr,curexpdir);
         system(tmpStr2);
      }
      sprintf(tmpStr,"%s/sampling.sch",acqdir);
      if ( ! access(tmpStr,R_OK) )
      {
         sprintf(tmpStr2,"cp -f %s %s/sampling.sch; cp -f %s %s/acqfil/sampling.sch",
                          tmpStr,curexpdir,tmpStr,curexpdir);
         system(tmpStr2);
      }
      setMenuName("main");
      Wsetgraphicsdisplay("");
      appendTopPanelParam();
      disp_current_seq();
      disp_status("        ");
      disp_expno();
   }
   if (doGlobals)
      saveGlobalPars(2,"_");  /* return saved parameters into global */

}

static int makeAcqerrMsg(char *buf, int wcode )
{
    char tbuf[ 80 ];
    int iter;

    if (getUsrErrMsg(buf, wcode)) {
	RETURN;
    }

    iter = 0;
    while (acqerrMsgTable[ iter ].code != 0)
      if (acqerrMsgTable[ iter ].code == wcode) {
          if (acqerrMsgTable[ iter ].errmsg == NULL) {
              *buf = '\0';
              ABORT;
          }
          strcpy( buf, acqerrMsgTable[ iter ].errmsg );
          RETURN;
      }
      else
        iter++;

    sprintf( &tbuf[ 0 ], "Acquistion error, code = %d", wcode );
    strcpy( buf, &tbuf[ 0 ] );
    ABORT;
}

static void convertDateFromString(char *cstr )
{
/* Convert string from ctime() format to t_format */
    char date[30], mon[4];
    int i, len, doff, coff;

    len = strlen( cstr );
    if (len > 23)
    {
      strcpy(date,"");
      date[0] = '\0';
      doff = 0; coff = 20;      /* year */
      for (i=0; i<4; i++)
        date[doff+i] = cstr[coff+i];
      coff = 4;                 /* month */
      for (i=0; i<3; i++)
        mon[i] = cstr[coff+i];
      mon[3] = '\0';
      for (i=0; i<NUM_MONTHS; i++)
      {
        if (strcmp(mon,MonthStrings[i]) == 0)
        {
          len = i+1;
          break;
        }
        else
          len = 1;
      }
      sprintf(mon,"%2d",len);
      doff = 4;
      for (i=0; i<2; i++)
        date[doff+i] = mon[i];
      doff = 6; coff = 8;       /* day */
      for (i=0; i<2; i++)
        date[doff+i] = cstr[coff+i];
      date[8] = 'T';
      doff = 9; coff = 11;      /* hour */
      for (i=0; i<2; i++)
        date[doff+i] = cstr[coff+i];
      doff = 11; coff = 14;     /* minute */
      for (i=0; i<2; i++)
        date[doff+i] = cstr[coff+i];
      doff = 13; coff = 17;     /* second */
      for (i=0; i<2; i++)
        date[doff+i] = cstr[coff+i];
      date[15] = '\0';
      len = strlen(date);
      for (i=0; i<len; i++)
        if (date[i] == ' ')
          date[i] = '0';
      strcpy(cstr, date);
    }
    else
    {
      strcpy(cstr,"");
    }
}

static void set_vnmrj_acq_times(int statusCode, int errorCode, char *logpath, time_t *timeaddr)
{
   char s_submitted[MAXSTR], s_run[MAXSTR];

   statusCode = statusCode & 0xFF;   /* remove Nirvana controller ID info */

/*  This function is called only if we are in the current experiment. */
/*  The errorCode equal to 0 distinguishes a resumed acquisition      */
   if ((statusCode == EXP_STARTED) && (errorCode == 0))
   {
      /*if (cftime(s_run,t_format,timeaddr) > 0)*/
      if (strftime(s_run, MAXSTR, t_format, localtime(timeaddr) ) > 0)
      {
         P_setstring(PROCESSED,"time_run",s_run,1);
         P_setstring(CURRENT,"time_run",s_run,1);
      }
   }
   else if (statusCode == EXP_COMPLETE)
   {
/* set time_run if not previously set */
      if ((P_getstring(PROCESSED,"time_run",s_run,1,MAXSTR) == 0) &&
          (P_getstring(PROCESSED,"time_submitted",
                                  s_submitted,1,MAXSTR) == 0))
      {
         if ((strcmp(s_run,"") != 0) && (strcmp(s_submitted,s_run) == 0))
         {
            FILE  *flog;
            if ((flog = fopen(logpath, "r")) != NULL)
            {
               while (fgets_nointr(s_run, sizeof(s_run), flog) != NULL)
               {
                  if (strstr(s_run, "Experiment started") != NULL)
                  {
                     convertDateFromString( s_run );
                     P_setstring(CURRENT,"time_run",s_run,1);
                     P_setstring(PROCESSED,"time_run",s_run,1);
                     break;
                  }
               }
               fclose(flog);
            }
         }
      }
/* set time_complete */
      if (strftime(s_run, MAXSTR, t_format, localtime(timeaddr) ) > 0)
      {
         P_setstring(CURRENT,"time_complete",s_run,1);
         P_setstring(PROCESSED,"time_complete",s_run,1);
      }
   }
}

static int log_acq_message(int auto_mode, char *autodir, char *userdir, char *expn, 
                    int statusCode, int errorCode, int elemval, char *vpdir )
{
   char	statusMsg[ 256 ], tempbuf[ BUFLEN ];
   char mode[2];

#ifdef NIRVANA
   int controllerType;
   int controllerNum;

   controllerType = statusCode >> 16;
   controllerNum = controllerType & 0xFFF;
   controllerType = (controllerType >> 12) & 0xF;

#endif

   statusCode = statusCode & 0xFF;

   statusMsg[0] = '\0';
   strcpy(mode,"a");
   switch (statusCode) {

	  case EXP_FID_CMPLT:
		break;

	  case BS_CMPLT:
		sprintf( &statusMsg[ 0 ], "BS %d completed", errorCode );
		break;

	  case HARD_ERROR:
                makeAcqerrMsg( &tempbuf[ 0 ], errorCode );
                if (errorCode == (HDWAREERROR+STMERROR) )
                {
                   int rcvr, nperr;
                   char	msg[ 256 ];
                   getExpStatusInt(EXP_RCVRNPERR, &rcvr);
                   getExpStatusInt(EXP_NPERR, &nperr);
		   sprintf( &msg[ 0 ], &tempbuf[ 0 ], rcvr, abs(nperr),
                   (nperr < 0) ? "greater than np." : "less than np.");
		   sprintf( &statusMsg[ 0 ], "FID %d: %s",elemval, &msg[ 0 ]);
                }
                else
		   sprintf( &statusMsg[ 0 ], "FID %d: %s",elemval, &tempbuf[ 0 ]);
		break;

	  case SOFT_ERROR:
	  case WARNING_MSG:
                makeAcqerrMsg( &tempbuf[ 0 ], errorCode );
		sprintf( &statusMsg[ 0 ], "FID %d: %s",elemval, &tempbuf[ 0 ]);
		break;

	  case EXP_ABORTED:
		strcpy( &statusMsg[ 0 ], "Acquisition aborted");
		break;

	  case SETUP_CMPLT:
		break;

	  case STOP_CMPLT:
		strcpy( &statusMsg[ 0 ], "Acquisition stopped");
		if (errorCode == STOP_EOS)
		  strcat( &statusMsg[ 0 ], " at current transient" );
		else if (errorCode == STOP_EOB)
		  strcat( &statusMsg[ 0 ], " at block size" );
		else if (errorCode == STOP_EOF)
		  strcat( &statusMsg[ 0 ], " at current FID" );
		break;

	  case EXP_COMPLETE:
		strcpy( &statusMsg[ 0 ], "Acquisition complete");
		break;

	  case EXP_STARTED:
		if (errorCode == 0)
                {
		   strcpy( &statusMsg[ 0 ], "Experiment started");
                   strcpy(mode,"w");
                }
                else if (errorCode == RESUME_ACQ_BIT)
                {
		   strcpy( &statusMsg[ 0 ], "Experiment resumed");
                }
		break;
	}

        if (strlen(statusMsg))
        {
           char   logpath[MAXPATH];
           FILE  *flog;
           struct timeval clock;
           char   date[30];
           int    len;

           /* --- get date and time of day --- */
           gettimeofday(&clock, NULL);
           strcpy(date, ctime(&(clock.tv_sec)) );
           len = strlen(date);
           date[len-1] = '\0';		/* remove the embedded CR in string */
           date[len] = '\0';		/* remove the embedded CR in string */

           if (auto_mode)
              sprintf( logpath,"%s.fid/log",autodir);
           else if ( ! strcmp(expn,"vp") )
              sprintf( logpath,"%s/log", vpdir );
           else
              sprintf( logpath,"%s/%s/acqfil/log", userdir,expn );
           if ((flog = fopen(logpath,mode)) != NULL)
           {
              fprintf(flog,"%s: %s\n", date, statusMsg);
              fclose(flog);
              if (! strcmp(expn,"vp") )
              {
                 char explog[MAXPATH];
                 sprintf( explog,"%s/acqfil/log", curexpdir );
                 unlink(explog);
                 link(logpath,explog);
              }
           }

/* curexp is always correct when it gets here, but test anyway in case we change it */
	   strcpy(statusMsg,userdir);
	   strcat(statusMsg,"/");
	   strcat(statusMsg,expn);
	   if ( ! strcmp(expn,"vp") || ! strcmp(statusMsg,curexpdir) )
	   {
	      set_vnmrj_acq_times(statusCode, errorCode, logpath, &(clock.tv_sec));
	   }
           sprintf( logpath,"%s.fid/time_run",autodir);
           if (acqStartLog)
           {
              if ((flog = fopen(logpath,mode)) != NULL)
              {
                 char s_run[MAXSTR];
                 if (strftime(s_run, MAXSTR, t_format,
                              localtime(&(clock.tv_sec))) > 0)
                    fprintf(flog,"%s\n", s_run);
                 fclose(flog);
              }
           }
           else if ( (skipFlush == 0) &&  ! access(logpath, F_OK))
           {
              char s_run[MAXSTR];
              if ((flog = fopen(logpath, "r")) != NULL)
              {
                 while (fgets_nointr(s_run, sizeof(s_run), flog) != NULL)
                 {
                    if ((strlen(s_run) > 1) && (s_run[strlen(s_run)-1] == '\n'))
                       s_run[strlen(s_run)-1] = '\0';
                    P_setstring(CURRENT,"time_run",s_run,1);
                    P_setstring(PROCESSED,"time_run",s_run,1);
                    break;
                 }
                 fclose(flog);
              }
              unlink(logpath);
           }
        }
	RETURN;
}

void checkAcqStart(char *cmd)
{
   int index, key, exp, proc, exp2, dcode, ecode;
   int ret;
   char rem[128];

   ret = dcode = ecode = 0;
   acqStartUseRtp = 0;
   acqStartLog = 0;
   ret = sscanf(cmd,"acqsend(%d,'%d',%d,%d,'exp%d',%d,%d,%s)",
          &index, &key, &exp, &proc, &exp2, &dcode, &ecode, rem);
   if ( (ret == 8) && (dcode == EXP_STARTED) )
   {
      acqStartUseRtp = 1;
      if (ecode == 0)  /* don't set for ra */
         acqStartLog = 1;
   }
}

int acqstatus(int argc, char *argv[], int retc, char *retv[])
{
	char		statusMsg[ BUFLEN ], tempbuf[ BUFLEN ];
	int		errorCode, statusCode, encodedStatusCode, r;
	double		c_val;
        int             ctval;
        int             elemval;
        int             userMsg = 0;
        FILE           *fd;
        char            temp2buf[ BUFLEN ];

#ifdef NIRVANA
        int cntlrType;
        int cntlrNum;
#endif 


        (void) retc;
        (void) retv;
	APRINT2("entered acqstatus, argc = %d  retc = %d\n", argc, retc );

/*  Command must have two or three arguments in addition to its name...  */

	if (argc < 3) {
		Werrprintf(
	"'acqstatus' called with wrong number of arguments"
		);
		ABORT;
	}

/*  Second argument, the status code, must be a real number.  */

	if (!isReal( argv[ 2 ] )) {
		Werrprintf(
	"'acqstatus' called with 2nd argument invalid"
		);
		ABORT;
	}
	else
	  encodedStatusCode = (int) stringReal( argv[ 2 ] );

#ifdef NIRVANA
       cntlrType = encodedStatusCode >> 16;
       cntlrNum = cntlrType & 0xFFF;
       cntlrType = (cntlrType >> 12) & 0xF;
#endif

       statusCode = encodedStatusCode & 0xFF;

	errorCode = -1;
	if (3 < argc)
	  if (isReal( argv[ 3 ] ))
	    errorCode = (int) stringReal( argv[ 3 ] );

	elemval = -1;
	if (4 < argc)
	  if (isReal( argv[ 4 ] ))
	    elemval = (int) stringReal( argv[ 4 ] );

	ctval = -1;
	if (5 < argc)
	  if (isReal( argv[ 5 ] ))
	    ctval = (int) stringReal( argv[ 5 ] );
        if ( ! strcmp(argv[1],"vp") )
        {
           checkVpState(argv[6], statusCode != EXP_STARTED);
	   if (statusCode == EXP_COMPLETE)
              execString("vpMsg\n");
        }

        if (statusCode != EXP_STARTED )
        {
           if (statusCode == EXP_ABORTED) /* Don't erase acqstatus[3] */
           {
              P_setreal( CURRENT,   "acqstatus", (double) statusCode, 1 );
              P_setreal( PROCESSED, "acqstatus", (double) statusCode, 1 );
           }
           else
           {
              P_setreal( CURRENT,   "acqstatus", (double) statusCode, 0 );
              P_setreal( PROCESSED, "acqstatus", (double) statusCode, 0 );
           }
           P_setreal( CURRENT,   "acqstatus", (double) errorCode,  2 );
           P_setreal( PROCESSED, "acqstatus", (double) errorCode,  2 );

#ifdef NIRVANA

           P_setreal( CURRENT,   "acqstatus", (double) cntlrType, 3 );
           P_setreal( CURRENT,   "acqstatus", (double) cntlrNum, 4 );
           P_setreal( PROCESSED,   "acqstatus", (double) cntlrType, 3 );
           P_setreal( PROCESSED,   "acqstatus", (double) cntlrNum, 4 );
#endif

        }

/*  Begin the status message with the first argument.  */
        if (mode_of_vnmr == AUTOMATION)
	   strcpy( &statusMsg[ 0 ], "auto");
        else
	   strcpy( &statusMsg[ 0 ], argv[ 1 ] );
        strcat( &statusMsg[ 0 ], ":  " );
	switch (statusCode) {

/*  strncat is used to prevent exceeding the bounds of the
    buffer for the status message.				*/

	/*  FID complete.  The error code is the number of the
	    element which completed.				*/

	  case EXP_FID_CMPLT:
                /* No message on FID complete */
		cur_element = elemval;
		acqhandler( argv[ 1 ], EXP_FID_CMPLT, errorCode, ctval );
		break;

	/*  BlockSize complete.  The error code is the number
            of completed blocks in the current experiment.	*/

	  case BS_CMPLT:
		cur_element = elemval;
		sprintf( &tempbuf[ 0 ], "BS %d completed", errorCode );
		strncat( &statusMsg[ 0 ], &tempbuf[ 0 ], 80 );
		acqhandler( argv[ 1 ], BS_CMPLT, errorCode, ctval );
#ifdef VNMRJ
		Wseterrorkey("ai");
#endif 
		break;

	  case HARD_ERROR:
                sprintf(temp2buf,"%s/acqqueue/acqmsg.lock",systemdir);
                fd = fopen(temp2buf,"w");
                makeAcqerrMsg( &tempbuf[ 0 ], errorCode );
                if (fd)
                {
                   char temp3buf[BUFLEN];

                   fprintf(fd,"%s\n",tempbuf);
                   fclose(fd);
                   sprintf(temp3buf,"%s/acqqueue/acqmsg",systemdir);
                   link(temp2buf,temp3buf);
                   unlink(temp2buf);
                }

#ifdef NIRVANA
                makeCntlrIdStr( statusMsg, cntlrType, cntlrNum);
                if (errorCode == (HDWAREERROR+STMERROR) )
                {
                   int rcvr, nperr;
                   getExpStatusInt(EXP_RCVRNPERR, &rcvr);
                   getExpStatusInt(EXP_NPERR, &nperr);
		   sprintf( &temp2buf[ 0 ], &tempbuf[ 0 ], rcvr, abs(nperr),
                   (nperr < 0) ? "greater than np." : "less than np.");
                }
                else
		   strncat( temp2buf, &tempbuf[ 0 ], 80 );

               strncat( statusMsg, temp2buf, 80 );

#else
                if (errorCode == (HDWAREERROR+STMERROR) )
                {
                   int rcvr, nperr;
                   getExpStatusInt(EXP_RCVRNPERR, &rcvr);
                   getExpStatusInt(EXP_NPERR, &nperr);
		   sprintf( &statusMsg[ 0 ], &tempbuf[ 0 ], rcvr, abs(nperr),
                   (nperr < 0) ? "greater than np." : "less than np.");
                }
                else
		   strncat( &statusMsg[ 0 ], &tempbuf[ 0 ], 80 );
#endif
		acqhandler( argv[ 1 ], HARD_ERROR, errorCode, 0 );
                deleteAcqPar(argv[1]);
#ifdef VNMRJ
		Wseterrorkey("ae");
#endif 
		break;

	  case SOFT_ERROR:
                makeAcqerrMsg( &tempbuf[ 0 ], errorCode );
		strncat( &statusMsg[ 0 ], &tempbuf[ 0 ], 80 );
		acqhandler( argv[ 1 ], SOFT_ERROR, errorCode, 0 );
#ifdef VNMRJ
		Wseterrorkey("ae");
#endif 
		break;

	  case WARNING_MSG:
                makeAcqerrMsg( &tempbuf[ 0 ], errorCode );
		strncat( &statusMsg[ 0 ], &tempbuf[ 0 ], 80 );
		acqhandler( argv[ 1 ], SOFT_ERROR, errorCode, 0 );
#ifdef VNMRJ
		Wseterrorkey("aw");
#endif 
		break;

	  case EXP_ABORTED:
#ifdef VNMRJ
		Wseterrorkey("ae");
#endif 
                if (strlen(abortMsg) > 1)
                {
                   userMsg = 1;
		   strncpy( &statusMsg[ 0 ], abortMsg, 80);
#ifdef VNMRJ
		   Wseterrorkey("ai");
#endif 
                }
                else
                {
		   strncat( &statusMsg[ 0 ], "Acquisition aborted", 80 );
                }
                setAbortMsg("");
		acqhandler( argv[ 1 ], EXP_ABORTED, errorCode, 0 );
                deleteAcqPar(argv[1]);
		break;

	  case SETUP_CMPLT:
		strncat( &statusMsg[ 0 ], "Setup Complete", 80 );
		acqhandler( argv[ 1 ], SETUP_CMPLT, errorCode, 0 );
#ifdef VNMRJ
		Wseterrorkey("ai");
#endif 
		break;

	/*  Stop complete.  The error code indicates where
	    the acquisition stopped.  The next argument should 
	    contain the element the experiment stopped at.
	    But guard against those cases where it is not.	*/

	  case STOP_CMPLT:
		strncat( &statusMsg[ 0 ], "Acquisition stopped", 80 );
		cur_element = elemval;
		acqhandler( argv[ 1 ], STOP_CMPLT, errorCode, ctval );
		if (errorCode == STOP_EOS)
		  strncat( &statusMsg[ 0 ], " at current transient", 80 );
		else if (errorCode == STOP_EOB)
		  strncat( &statusMsg[ 0 ], " at block size", 80 );
		else if (errorCode == STOP_EOF)
		  strncat( &statusMsg[ 0 ], " at current FID", 80 );
#ifdef VNMRJ
		Wseterrorkey("ae");
#endif 
		break;

	  case EXP_COMPLETE:
		strncat( &statusMsg[ 0 ], "Acquisition complete", 80 );
	        statusMsg[ 79 ] = '\0';	/* Insure string is terminated */
                if (!silentMode)
		{
#ifdef VNMRJ
		   Wseterrorkey("ai");
#endif 
	           Werrprintf( "%s", &statusMsg[ 0 ] );
		}
		APRINT0("ready to call acqhandler\n" );
		r = P_getreal( CURRENT, "arraydim", &c_val, 1 );
		cur_element = (r != 0) ? 1 : (int) (c_val+0.1);

		p11_setcurrfidid();

		acqhandler( argv[ 1 ], EXP_COMPLETE, errorCode, ctval );
                deleteAcqPar(argv[1]);
		break;

	  case EXP_STARTED:
		strncat( &statusMsg[ 0 ], "Experiment started", 80 );
		cur_element = 0;
                skipFlush = 1;
                sprintf(tempbuf,"%s/acqqueue/acqmsg",systemdir);
                unlink(tempbuf);
#ifdef VNMRJ
		Wseterrorkey("ai");
#endif 
		break;
	}

	statusMsg[ 79 ] = '\0';		/* Insure string is terminated */

/*  Werrprintf generates a beep; Winfoprintf does not.	*/

	if ( (statusCode == BS_CMPLT) || (statusCode == EXP_STARTED) )
        {
          if (!silentMode)
	     Winfoprintf( "%s", &statusMsg[ 0 ] );
        }
	else if ((statusCode != EXP_COMPLETE) && (statusCode != EXP_FID_CMPLT))
        {
          if (userMsg)
	     Winfoprintf( "%s", &statusMsg[ 0 ] );
          else
	     Werrprintf( "%s", &statusMsg[ 0 ] );
        }
        log_acq_message( (mode_of_vnmr == AUTOMATION), datadir, userdir, argv[1], 
                     encodedStatusCode, errorCode, elemval, argv[6] );
	RETURN;
}

/*------------------------------------------------------------------------
|
|	acqdisp/(0 or 1)
|
|       If no argument passed,
|	this routine displays the value of pslabel in the Acq: field
|	at position 72 line 2
|       If argument passed,
|	this routine displays upto an 8 byte word in the status panel
|	at position 72 line 2
|
+-----------------------------------------------------------------------*/

int acqdisp(int argc, char *argv[], int retc, char *retv[])
{
    (void) retc;
    (void) retv;
    if (argc == 1)
       disp_current_seq();
    else
       disp_acq(argv[1]);
    RETURN;
}


/*------------------------------------------------------------------------
|
|	werr/1
|
|	Change value of "wexp", "wnt", "wbs" or "werr" and notify
|	Acqproc of the change.  This always turns the bit on for
|       the requested processing. If the parameter is empty, acqsend
|       will turn the bit off.  This prevents different experiments
|       from interacting, since these commands always effect the current
|       executing experiemnt. wexp and werr are never changed.
|       Special case of wnt('GA_OFF') turns off ga processing.
|
+-----------------------------------------------------------------------*/
/*
    Extended discussion.  It used to be that werr,wexp, etc, would turn the
    bit on or off, depending on if the argument was ''.  There are two potential
    problems.  If a person in exp2 sets wexp(''), wexp processing in exp1 will
    be turned off. The second problem is, if they change these fields for a queued
    experiment, the processing may not become active for the queued experiment.

    The new scheme is to turn on the flags for wexp and werr when au is executed
    and never turn them off.  There is only one wexp event and hopefully no werr
    events. There is no overhead for this, since "experiment complete" and "error"
    messages are always sent to Vnmrbg, even if there is no processing to do.
    (Log files need to be updated.) When these event messages are received, the
    parameters can be checked to see what processing, if any, should be done.

    The wbs and wnt flags are more useful. There could be a lot of extra overhead
    if these messages are sent when there is no processing to do.  The new scheme
    is to turn the wbs and wnt flags on when au is executed, whether or not the
    parameters are set to the empty string. Then, whenever a wbs or wnt message
    is received by Vnmrbg, it will check the wbs or wnt parameter to see if it
    is an empty string. If it is, Vnmrbg will send a message to the procs to turn off
    the wbs or wnt flag.
 */
int werr(int argc, char *argv[], int retc, char *retv[])
{
	char	*valptr, str[ MAXSTR ];
	int	when_mask, iter, vlen;
        int     on_off;


	if ( (argc < 2) && retc)
        {
           retv[0] = intString(whenProcessing);
           RETURN;
        }
	if (argc < 2) {
		Werrprintf( "%s:  command requires an argument", argv[ 0 ] );
		ABORT;
	}

	valptr = argv[ 1 ];
	vlen = strlen( valptr );

/*  Convert "back quotes" (`) into normal single quotes so as to help
    the user avoid hassles with escaping single quotation marks.	*/

        on_off = 1;
	if ( ! strcmp( argv[ 0 ], "wnt" ) &&  ! strcmp( argv[ 1 ], "GA_OFF" ))
        {
           on_off = 0;
        }
        else
        {
	   for (iter = 0; iter < vlen; iter++)
	     if ( *(valptr+iter) == '`' ) *(valptr+iter) = '\'';

	   if (P_setstring( CURRENT, argv[0], valptr, 1 )) {
		Werrprintf( "Failed to set value for %s",argv[0]);
		ABORT;
	   }
	   appendvarlist( argv[0] ); /* send new variable name to vnmrj */
        }
	if (!acquisition_ok( argv[ 0 ] ))
	  RETURN;
        if (!is_acqproc_active())
          RETURN;

        when_mask = 0;
	if (strcmp( argv[ 0 ], "wsu" ) == 0)
           when_mask = WHEN_SU_PROC;
	else if (strcmp( argv[ 0 ], "wbs" ) == 0)
           when_mask = WHEN_BS_PROC;
	else if (strcmp( argv[ 0 ], "wnt" ) == 0)
           when_mask = (on_off) ? WHEN_NT_PROC : (WHEN_NT_PROC | WHEN_GA_PROC);
	else 
           RETURN;  /* never reset werr, wexp */

	sprintf( &str[ 0 ], "%s,%d,%d", UserName, when_mask, on_off);
	APRINT1("ready to send !%s! to Acqproc\n", str );

	if (send2Acq( CHANGEPARAM, str ) < 0) {
		Werrprintf( "%s: failed to notify Acquisition of changed value",
			argv[ 0 ]
		);
		ABORT;
	}
	RETURN;
}
/*------------------------------------------------------------------------
|
|	recon/0
|
|       attempts to reconnect this Vnmr user back to his
|		experiments that are active or queued.
|	9/13/91 GMB
|
+-----------------------------------------------------------------------*/

int recon(int argc, char *argv[], int retc, char *retv[])
{
	char	acqmsg[ 1028 ];

        (void) argc;
        (void) argv;
        (void) retc;
        (void) retv;
   /*  This test is in bootup.c   */

	if (!acquisition_ok(argv[0]))
	{
		Werrprintf("%s:  cannot be run in this mode", argv[0]);
		ABORT;
	}

/*  Notify the Acqproc that something is up.  */

	sprintf( &acqmsg[ 0 ], "%s", UserName);
	APRINT1("ready to send !%s! to Acquisition\n", acqmsg );

	if (send2Acq( RECONREQUEST, acqmsg ) < 0) {
		Werrprintf( "%s: failed to reach Acquisition",
			argv[ 0 ]
		);
		ABORT;
	}
	RETURN;
}

/* The global parameter saveglobal is an array of global and conpar parameter names.
 * If the sv flag is 1, the parameters from the saveglobal parameter will
 * be saved in the current tree, except the names will have a suffix (suff) attached.
 * If the sv flag is 2, the parameters from the saveglobal parameter, with the
 * suffix attached, will be copied from the current tree back to the global or
 * conpar tree,
 * If the sv flag is 3, the parameters from the saveglobal parameter, with the
 * suffix attached, will be deleted from the current and processed tree,
 * If the sv flag is 4, the parameters from the saveglobal parameter, with the
 * suffix attached, will be deleted from the current tree,
 */
void saveGlobalPars(int sv, char *suff)
{  vInfo  info;
   int index;
   int fromTree;
   int num;
   char varpar[MAXSTR];/* name of variable with array of parameters */
   char gstr[MAXSTR];  /* name of a parameter from the par array */
   char cstr[MAXSTR];  /* name of a parameter from the par array with suffix */

   strcpy(varpar,"saveglobal");
   if (sv == 1)
   {
      fromTree = GLOBAL;
   }
   else
   {
      fromTree = CURRENT;
      strcat(varpar,suff);
   }
   if (P_getVarInfo(fromTree,varpar,&info))
      return;
   num = info.size;
   for (index=1; index <= num; index++)
   {
      if (!P_getstring(fromTree,varpar,gstr,index,MAXSTR))
      {
         if (sv == 1)
         {
            strcpy(cstr,gstr);
            strcat(cstr,suff);
            if (P_copyvar(GLOBAL,CURRENT,gstr,cstr))
               P_copyvar(SYSTEMGLOBAL,CURRENT,gstr,cstr);
         }
         else if (sv == 2)
         {
            strcpy(cstr,gstr);
            strcat(cstr,suff);
            if (P_getVarInfo(GLOBAL,gstr,&info) == 0)
               P_copyvar(CURRENT,GLOBAL,cstr,gstr);
            else if (P_getVarInfo(SYSTEMGLOBAL,gstr,&info) == 0)
               P_copyvar(CURRENT,SYSTEMGLOBAL,cstr,gstr);
         }
         else if (sv == 3)
         {
            strcat(gstr,suff);
	    P_deleteVar(CURRENT,gstr);
	    P_deleteVar(PROCESSED,gstr);
         }
         else if (sv == 4)
         {
            strcat(gstr,suff);
	    P_deleteVar(CURRENT,gstr);
         }
      }
   }
   if (sv == 1)
   {
      /* save the varpar parameter */
      strcpy(cstr,varpar);
      strcat(cstr,suff);
      P_copyvar(GLOBAL,CURRENT,varpar,cstr);
   }
   else if (sv == 3)
   {
      /* delete the varpar parameter */
      P_deleteVar(CURRENT,varpar);
      P_deleteVar(PROCESSED,varpar);
   }
   else if (sv == 4)
   {
      /* delete the varpar parameter */
      P_deleteVar(CURRENT,varpar);
   }
}

static void execfromacqproc(int todo)
{
   char buf[MAXSTR+2];
   char tmpstr[MAXSTR];

   whenProcessing = 1;
   buf[0]='\0';
   if (todo & WHEN_SU_PROC)
   {
      if (!P_getstring(CURRENT,"wsu",tmpstr,1,MAXSTR))
      {
         if (strlen(tmpstr))
         {
            sprintf(buf,"%s\n",tmpstr);
            execString(buf);
         }
      }
   }
   else if (todo & WHEN_EXP_PROC)
   {
      if (!P_getstring(CURRENT,"wnt",tmpstr,1,MAXSTR))
      {
         if (strlen(tmpstr))
         {
            sprintf(buf,"%s\n",tmpstr);
            execString(buf);
         }
      }
      if (!P_getstring(CURRENT,"wexp",tmpstr,1,MAXSTR))
      {
         if (strlen(tmpstr))
         {
            sprintf(buf,"%s\n",tmpstr);
            execString(buf);
         }
      }
      if (!P_getstring(CURRENT,"wdone",tmpstr,1,MAXSTR))
      {
         if (strlen(tmpstr))
         {
            sprintf(buf,"%s\n",tmpstr);
            execString(buf);
         }
      }
   }
   else if (todo & WHEN_BS_PROC)
   {
      if (!P_getstring(CURRENT,"wbs",tmpstr,1,MAXSTR))
      {
         df_repeatscan = 1;
         if (strlen(tmpstr))
         {
            sprintf(buf,"%s\n",tmpstr);
            execString(buf);
         }
         else /* turn off future wbs processing */
         {
	    sprintf( tmpstr, "%s,%d,0", UserName, WHEN_BS_PROC);
	    send2Acq( CHANGEPARAM, tmpstr );  /* ignore failure */
         }
         df_repeatscan = 0;
      }
   }
   else if (todo & WHEN_NT_PROC)
   {
      if (!P_getstring(CURRENT,"wnt",tmpstr,1,MAXSTR))
      {
         if (strlen(tmpstr))
         {
            sprintf(buf,"%s\n",tmpstr);
            execString(buf);
         }
         else /* turn off future wnt processing */
         {
	    sprintf( tmpstr, "%s,%d,0", UserName, WHEN_NT_PROC);
	    send2Acq( CHANGEPARAM, tmpstr );  /* ignore failure */
         }
      }
   }
   else if (todo & WHEN_ERR_PROC)
   {
      if (!P_getstring(CURRENT,"werr",tmpstr,1,MAXSTR))
      {
         if (strlen(tmpstr))
         {
            sprintf(buf,"%s\n",tmpstr);
            werrCalling = 1;
            execString(buf);
            werrCalling = 0;
         }
      }
   }
   else if (todo & WHEN_GA_PROC)
   {
      strcpy(buf,"wft('acq')\n");
      execString(buf);
   }
   whenProcessing = 0;
}

/*-----------------------------------------------------------------------
|
|       acqsend/3
|	This routine sends a message to the acq process.  It will be used
|	to send acqproc messages.  If a third argument is passed, acqsend
|       will execute that string
|
+-----------------------------------------------------------------------*/
int acqsend(int argc, char *argv[], int retc, char *retv[])
{
#ifdef UNIX
   int todo;
   int retval;
   int enumber;
   int currentExpn, targetExpn;
   char msge[128];
   

    if ((argc == 3) && !strcmp(argv[2],"check"))
    {
       if (!is_acqproc_active())
          RETURN;
       send2Acq(FGREPLY,argv[1]);
       RETURN;
    }
    werrCalling = 0;
    retc = 0;
    if ((argc != 8) && (argc != 10) && (argc != 11) )
    {
        Werrprintf("Illegal usage of acqsend");
        return(0);
    }
    
    currentExpn = expdir_to_expnum( curexpdir );
    APRINT2("acqsend: type=%d message='%s'\n",argv[1],argv[2]); 
    retval = atoi(argv[1]);
    targetExpn = atoi( argv[ 3 ] );
    todo = atoi(argv[4]);

#ifdef VNMRJ
/*
#define VNMRJ_NUM_VIEWPORTS 4
    if (currentExpn != targetExpn)
    {
      double dcurwin;
      int i;
      int jcurwinTargetExpn = 0;
      for (i=1; i<=VNMRJ_NUM_VIEWPORTS; i++)
      {
	if (P_getreal(GLOBAL,"jcurwin",&dcurwin,i) == 0)
	{
	  if (targetExpn == (int)(dcurwin + 0.5))
	  {
	    jcurwinTargetExpn = (int)(dcurwin + 0.5);
	    break;
	  }
	}
	else
	  break;
      }
      if (jcurwinTargetExpn != 0)
        writelineToVnmrJ("keyword","expression for other Vnmr to execute");
      else
        do the usual, send2Acq(msge);
    }
*/
#endif 
    /* targetExpn == 0 signals vpmode */
    if ((currentExpn != targetExpn) && (retval == FGREPLY) && (targetExpn != 0) )
    {
      int dobg;

      if (!todo)
      {
         enumber = atoi(argv[6]);
         if  ((enumber == EXP_FID_CMPLT) || (enumber == BS_CMPLT))
         {
            dobg = 0;
         }
         else
         {
            enumber = atoi(argv[3]);
            dobg =  ( lockExperiment( enumber, ACQUISITION ) == 0);
         }
      }
      else
      {
         enumber = atoi(argv[3]);
         dobg =  ( lockExperiment( enumber, ACQUISITION ) == 0);
      }
      if (dobg)
      {
         if (argc == 8)
            sprintf(msge,"%s,-2,acqsend(%d,'%s',%s,%s,'%s',%s,%s)",
                 argv[2],
                 FGNOREPLY,argv[2],argv[3],argv[4],argv[5],argv[6],argv[7]);
         else
            sprintf(msge,"%s,-2,acqsend(%d,'%s',%s,%s,'%s',%s,%s,%s,%s)",
                 argv[2],
                 FGNOREPLY,argv[2],argv[3],argv[4],argv[5],argv[6],argv[7],argv[8],argv[9]);
      }
      else
         sprintf(msge,"%s,0,",argv[2]);
    }
    else
    {
       if (todo)
       {
         if ((currentExpn == targetExpn) || (retval != FGREPLY) || (targetExpn == 0))
         {
           /* In vpmode, may need to read parameters before resetting globals */
           if (targetExpn)
              saveGlobalPars(2,"_");  /* return saved parameters into global */
           acqstatus(argc-4,&argv[4], retc , retv);
           if (retval == FGREPLY)
              disp_acq("ACQ:BUSY");
           execfromacqproc(todo);
           if (retval == FGREPLY)
              disp_acq(" ");
         }
         else
         {
           acqstatus(argc-4,&argv[4], retc , retv);
         }
       }
       else
       {
          acqstatus(argc-4,&argv[4], retc , retv);
          if (retval != FGREPLY)
             showFlushDisp = 0;
       }
       sprintf(msge,"%s,0,",argv[2]);
       if (targetExpn == 0)
       {
         enumber = atoi(argv[6]) & 0xFF;
         if  ( (enumber == HARD_ERROR) || (enumber == SOFT_ERROR) ||
               (enumber == EXP_ABORTED) || (enumber == STOP_CMPLT) ||
               (enumber == EXP_COMPLETE) )
         {
             cleanupVp(argv[10]);
         }
       }
    }

    if (retval == FGREPLY)
    {
    /*Create message packet in Greg Protocol to send to Acquisition Process */
       if ( is_acqproc_active() && (send2Acq(FGREPLY,msge) < 0) )
       {
	   Werrprintf("Message Failed to be Sent");
       }
    }
    return(0);
#else 
    return(1);		/*  VMS:  provoke abort */
#endif 
}

int vnmrInfo(int argc, char *argv[], int retc, char *retv[])
{
   int setNotGet;
   int setVal = 0;
   int retVal = 0;
   double setValD = 0.0;
   double retValD = 0.0;
   int useDouble = 0;

   if ( (argc == 3) && (strcmp("get",argv[1]) == 0) )
   {
      setNotGet = 0;
   }
   else if ( (argc == 4) && (strcmp("set",argv[1]) == 0) )
   {
      setNotGet = 1;
      if (isReal(argv[3]))
      {
         setValD = stringReal(argv[3]);
         setVal = (int) setValD;
      }
      else
      {
         Werrprintf( "'set' option to %s requires an numeric value as the third argument",
                   argv[0]);
         ABORT;
      }
   }
   else
   {
      Werrprintf( "usage is %s('get', 'keyword') or %s('set','keyword',value)",
                   argv[0],argv[0]);
      ABORT;
   }
   if (strcmp(argv[2],"spinOnOff") == 0)
   {
      retVal = (setNotGet) ? setInfoSpinOnOff(setVal) :
                             getInfoSpinOnOff();
   }
   else if (strcmp(argv[2],"spinSetSpeed") == 0)
   {
      retVal = (setNotGet) ? setInfoSpinSetSpeed(setVal) :
                             getInfoSpinSetSpeed();
   }
   else if (strcmp(argv[2],"spinUseRate") == 0)
   {
      retVal = (setNotGet) ? setInfoSpinUseRate(setVal) :
                             getInfoSpinUseRate();
   }
   else if (strcmp(argv[2],"spinSetRate") == 0)
   {
      retVal = (setNotGet) ? setInfoSpinSetRate(setVal) :
                             getInfoSpinSetRate();
   }
   else if (strcmp(argv[2],"spinSwitchSpeed") == 0)
   {
      retVal = (setNotGet) ? setInfoSpinSwitchSpeed(setVal) :
                             getInfoSpinSwitchSpeed();
   }
   else if (strcmp(argv[2],"spinSelect") == 0)
   {
      retVal = (setNotGet) ? setInfoSpinSelect(setVal) :
                             getInfoSpinSelect();
   }
   else if (strcmp(argv[2],"spinErrorControl") == 0)
   {
      retVal = (setNotGet) ? setInfoSpinErrorControl(setVal) :
                             getInfoSpinErrorControl();
   }
   else if (strcmp(argv[2],"spinExpControl") == 0)
   {
      retVal = (setNotGet) ? setInfoSpinExpControl(setVal) :
                             getInfoSpinExpControl();
   }
   else if (strcmp(argv[2],"insertEjectControl") == 0)
   {
      retVal = (setNotGet) ? setInfoInsertEjectExpControl(setVal) :
                             getInfoInsertEjectExpControl();
   }
   else if (strcmp(argv[2],"spinspeed") == 0)
   {
      if (setNotGet)
      {
         Werrprintf( "Can not use 'set' with %s keyword", argv[2]);
         ABORT;
      }
      retVal = getInfoSpinSpeed();
   }
   else if (strcmp(argv[2],"spinner") == 0)
   {
      if (setNotGet)
      {
         Werrprintf( "Can not use 'set' with %s keyword", argv[2]);
         ABORT;
      }
      retVal = getInfoSpinner();
   }
   else if (strcmp(argv[2],"tempOnOff") == 0)
   {
      retVal = (setNotGet) ? setInfoTempOnOff(setVal) :
                             getInfoTempOnOff();
   }
   else if (strcmp(argv[2],"tempSetPoint") == 0)
   {
      retValD = (setNotGet) ? (double) setInfoTempSetPoint((int) (setValD * 10.0)) :
                             (double) getInfoTempSetPoint() / 10.0;
      useDouble = 1;
   }
   else if (strcmp(argv[2],"tempErrorControl") == 0)
   {
      retVal = (setNotGet) ? setInfoTempErrorControl(setVal) :
                             getInfoTempErrorControl();
   }
   else if (strcmp(argv[2],"tempExpControl") == 0)
   {
      retVal = (setNotGet) ? setInfoTempExpControl(setVal) :
                             getInfoTempExpControl();
   }
   else
   {
      Werrprintf( "%s uses unrecognized keyword '%s'", argv[0],argv[2]);
      ABORT;
   }
   if (!setNotGet)
   {
      if (retc)
      {
         retv[0] = (useDouble) ? realString(retValD) : realString( (double)retVal );
      }
      else
      {
         if (useDouble)
            Winfoprintf("%s value is %g",argv[2],retValD);
         else
            Winfoprintf("%s value is %d",argv[2],retVal);
      }
   }
   RETURN;
}
