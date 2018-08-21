/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <math.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include "ACQPROC_strucs.h"
#include "statusextern.h"
#include "STAT_DEFS.h"
#include "acqstat_item.h"


#define IDLE 1
#define ACQUIRE 2
#define LOCK 3
#define SHIM 4
#define FID 5

#define OFF 0
#define REGULATED 1
#define NOTREG 2
#define NOTPRESENT 3
#define ON 1
#define GATED 2
#define HIDE 0
#define SHOWIT 1

static int LKstat = 3;
static int Spinstat = 3;
static int Sampstat = 3;
static int Decstat = 3;
static int VTstat = 3;
static float LockPercent = 0.0;
static float Locklevel = -1.0;
static int infoState = 0;

extern  AcqStatBlock  CurrentStatBlock;
extern int acq_ok;
extern int newAcq;
extern int useInfostat;
extern int StatPortId;
static char infoModeOn[6] = "uuu";

#define TXT_LEN 19
/* #define AIRVal  dispSpec + 1 */

static void write_info_mode(char *txt,int key,int stat);
static void write_shim_info(int shimset, int dacno, int value);
static void write_info_val(int item, int value);
static void checkLogEntry(int item, char *txt );
static void find_item_string( int item_no, char *txt );
static int isStatlogItem(int item, char *txt);

/* list of currently supported status items */
#define NUM_STAT_ITEMS 7

static int statlogitems[NUM_STAT_ITEMS]={StatusVal,SPNVal,SPNVal2,VTVal,VTVal2,LKVal,RFMonVal};

static char statlog[256]="";
static int logging=0;
const char root_hdr[]="<statlog>\n";
const char root_end[]="</statlog>\n";

static void checkLogEntry(int item, char *txt ){
    char name[TXT_LEN]="";
    char file[64]="";
    char path[256];
    struct timeval clock;
    FILE * fp;
    int newfile=0;

    if(!logging)
        return;
    if(strlen(statlog)==0)
        return;
    if(txt==NULL || strlen(txt)==0)
        return;

    if(!isStatlogItem(item,txt))
        return;
    switch(item){
    case StatusVal:
        strcpy(file,"event");
        break;
    case SPNVal:
    case SPNVal2:
        strcpy(file,"spin");
        break;
    case VTVal:
    case VTVal2:
        strcpy(file,"vt");
        break;
    case LKVal:
        strcpy(file,"lock");
        break;
    case RFMonVal:
        strcpy(file,"rfmon");
        break;
    }
    sprintf(path,"%s/%s.xml",statlog,file);
    find_item_string(item, name);
    gettimeofday(&clock, NULL); /* get time of connect */

    fp = fopen(path, "r+");
    if (fp==NULL){
        newfile=1;
        fp = fopen(path, "w");
    }
    else{
        newfile=0;
    }
    if(fp==NULL)
        return;
    if(newfile)
        fprintf(fp,"%s",root_hdr);
    else
        fseek(fp,-strlen(root_end),SEEK_END);

    fprintf(fp,"<%s value=\"%s\" time=\"%ld\"/>\n",name,txt,clock.tv_sec);
    fprintf(fp,"%s",root_end);
    fflush(fp);
    fclose(fp);
}
void setLogFilePath(char *path){
    int i;
    logging=0;
    if(path !=NULL)
        strcpy(statlog,path);
	if(strlen(statlog)>0)
	    logging=1;
    checkLogEntry(StatusVal,"RESTART");
}

static int isStatlogItem(int item, char *txt){
	int i;
	for(i=0;i<NUM_STAT_ITEMS;i++){
		if(item==statlogitems[i]){
			return 1;
		}
	}
	return 0;
}

static int getFTS(const char *path, int *val)
{
   FILE *stream;
   char data[1024];
   char *addr;

   data[0] = '\0';

   if ( (stream = popen(path, "r")) != NULL)
   {
      while ( (addr = fgets( data, sizeof(data), stream )) == NULL)
      {
         if (errno != EINTR || (feof(stream)))
         {
            pclose(stream);
            return( -1 );
         }
      }
      pclose(stream);
      *val = atoi(data);
      return( 0 );
   }
   return( -1 );
}

static void find_item_string( int item_no, char *txt )
{
    switch ( item_no )
    {
	case StatusTitle:
	  strncpy(txt,"statusttl",TXT_LEN);
	  break;
	case PendTitle:
	  strncpy(txt,"pendtl",TXT_LEN);
	  break;
	case UserTitle:
	  strncpy(txt,"usertl",TXT_LEN);
	  break;
	case ExpTitle:
	  strncpy(txt,"exptl",TXT_LEN);
	  break;
	case ArrayTitle:
	  strncpy(txt,"arraytl",TXT_LEN);
	  break;
	case CT_Title:
	  strncpy(txt,"cttl",TXT_LEN);
	  break;
	case DecTitle:
	  strncpy(txt,"dectl",TXT_LEN);
	  break;
	case SampleTitle:
	  strncpy(txt,"sampletl",TXT_LEN);
	  break;
	case LockTitle:
	  strncpy(txt,"locktl",TXT_LEN);
	  break;
	case LKTitle:
	  strncpy(txt,"LKtl",TXT_LEN);
	  break;
	case CompTitle:
	  strncpy(txt,"comptl",TXT_LEN);
	  break;
	case RemainTitle:
	  strncpy(txt,"remaintl",TXT_LEN);
	  break;
	case DataTitle:
	  strncpy(txt,"datatl",TXT_LEN);
	  break;
	case SpinTitle:
	  strncpy(txt,"spintl",TXT_LEN);
	  break;
	case SPNTitle:
	  strncpy(txt,"SPNtl",TXT_LEN);
	  break;
	case SPN2Title:
	  strncpy(txt,"SPN2tl",TXT_LEN);
	  break;
	case VT_Title:
	  strncpy(txt,"vttl",TXT_LEN);
	  break;
	case VTTitle:
	  strncpy(txt,"VTtl",TXT_LEN);
	  break;
	case VT2Title:
	  strncpy(txt,"VT2tl",TXT_LEN);
	  break;
	case AIRTitle:
	  strncpy(txt,"AIRtl",TXT_LEN);
	  break;

	case StatusVal:
	  strncpy(txt,"status",TXT_LEN);
	  break;
	case PendVal:
	  strncpy(txt,"queue",TXT_LEN);
	  break;
	case UserVal:
	  strncpy(txt,"user",TXT_LEN);
	  break;
	case ExpVal:
	  strncpy(txt,"ExpId",TXT_LEN);
	  break;
	case ArrayVal:
	  strncpy(txt,"fid",TXT_LEN);
	  break;
	case CT_Val:
	  strncpy(txt,"ct",TXT_LEN);
	  break;
	case DecVal:
	  strncpy(txt,"dec",TXT_LEN);
	  break;
	case SampleVal:
	  strncpy(txt,"sample",TXT_LEN);
	  break;
	case LockVal:
	  strncpy(txt,"lock",TXT_LEN);
	  break;
	case SpinVal:
	  strncpy(txt,"spin",TXT_LEN);
	  break;
	case VT_Val:
	  strncpy(txt,"vt",TXT_LEN);
	  break;

	case CompVal:
	  strncpy(txt,"timedone",TXT_LEN);
	  break;
	case RemainVal:
	  strncpy(txt,"timeleft",TXT_LEN);
	  break;
	case DataVal:
	  strncpy(txt,"timestore",TXT_LEN);
	  break;
	case LKVal:
	  strncpy(txt,"lklvl",TXT_LEN);
	  break;
	case SPNVal:
	  strncpy(txt,"spinval",TXT_LEN);
	  break;
	case SPNVal2:
	  strncpy(txt,"spinset",TXT_LEN);
	  break;
	case MASSpeedLimit:
	  strncpy(txt,"masSpeedLimit",TXT_LEN);
	  break;
	case MASBearSpan:
	  strncpy(txt,"masBearSpan",TXT_LEN);
	  break;
	case MASBearAdj:
	  strncpy(txt,"masBearAdj",TXT_LEN);
	  break;
	case MASBearMax:
	  strncpy(txt,"masBearMax",TXT_LEN);
	  break;
	case MASActiveSetPoint:
	  strncpy(txt,"masActiveSetPoint",TXT_LEN);
	  break;
	case MASProfileSetting:
	  strncpy(txt,"masProfileSetting",TXT_LEN);
	  break;
	case GradCoilID:
	  strncpy(txt,"gradCoilId",TXT_LEN);
	  break;

	case SampleZoneVal:
	  strncpy(txt,"zone",TXT_LEN);
	  break;
	case SampleRackVal:
	  strncpy(txt,"rack",TXT_LEN);
	  break;

	case ProbeID1:
	  strncpy(txt,"probeId1",TXT_LEN);
	  break;
	case VTVal:
	  strncpy(txt,"vtval",TXT_LEN);
	  break;
	case VTVal2:
	  strncpy(txt,"vtset",TXT_LEN);
	  break;
	case FTSVal:
	  strncpy(txt,"ftsval",TXT_LEN);
	  break;

	case dispStatus:
	  strncpy(txt,"dstatus",TXT_LEN);
	  break;
	case dispIndex:
	  strncpy(txt,"dindex",TXT_LEN);
	  break;
	case dispSpec:
	  strncpy(txt,"dspec",TXT_LEN);
	  break;
	case AIRVal:
	  strncpy(txt,"air",TXT_LEN);
	  break;
	case lockGain:
	  strncpy(txt,"lockgain",TXT_LEN);
	  break;
	case lockPower:
	  strncpy(txt,"lockpower",TXT_LEN);
	  break;
	case lockPhase:
	  strncpy(txt,"lockphase",TXT_LEN);
	  break;
	case shimSet:
	  strncpy(txt,"shimset",TXT_LEN);
	  break;

        case RF10s1AvgVal:
          strncpy(txt,"10s_obspower",TXT_LEN);
          break;
        case RF5m1AvgVal:
          strncpy(txt,"5m_obspower",TXT_LEN);
          break;
        case RF10s1LimVal:
          strncpy(txt,"10s_obslimit",TXT_LEN);
          break;
        case RF5m1LimVal:
          strncpy(txt,"5m_obslimit",TXT_LEN);
          break;

        case RF10s2AvgVal:
          strncpy(txt,"10s_decpower",TXT_LEN);
          break;
        case RF5m2AvgVal:
          strncpy(txt,"5m_decpower",TXT_LEN);
          break;
        case RF10s2LimVal:
          strncpy(txt,"10s_declimit",TXT_LEN);
          break;
        case RF5m2LimVal:
          strncpy(txt,"5m_declimit",TXT_LEN);
          break;

        case RF10s3AvgVal:
          strncpy(txt,"10s_ch3power",TXT_LEN);
          break;
        case RF5m3AvgVal:
          strncpy(txt,"5m_ch3power",TXT_LEN);
          break;
        case RF10s3LimVal:
          strncpy(txt,"10s_ch3limit",TXT_LEN);
          break;
        case RF5m3LimVal:
          strncpy(txt,"5m_ch3limit",TXT_LEN);
          break;

        case RF10s4AvgVal:
          strncpy(txt,"10s_ch4power",TXT_LEN);
          break;
        case RF5m4AvgVal:
          strncpy(txt,"5m_ch4power",TXT_LEN);
          break;
        case RF10s4LimVal:
          strncpy(txt,"10s_ch4limit",TXT_LEN);
          break;
        case RF5m4LimVal:
          strncpy(txt,"5m_ch4limit",TXT_LEN);
          break;
        case RFMonVal:
          strncpy(txt,"rfmonval",TXT_LEN);
          break;

	default:
	  strcpy(txt,"undefined");
	  break;
    }
    txt[ TXT_LEN ] = '\0';
}

/*------------------------------------------------------------------
|
|       disp_string/2
|
|       This procedure displays a string
|       at the position according to panel item
|
+----------------------------------------------------------------------*/
disp_string(item_no,strval)
int   item_no;
char *strval;
/* display the processing status characters */
{
    char txt[TXT_LEN+1];

    if (Wissun())
    {
        if (strlen(strval) > 0 )
        {
	  if (useInfostat == 0)
	  {
            find_item_string( item_no, txt );
            if (StatPortId > 0)
              writestatToVnmrJ( txt, strval );
            else if(!logging)
              fprintf(stderr,"%s %s\n",txt,strval);
/* fprintf(stderr,"%s + %s\n",txt,strval); */
	  }
	  else
	  {
            set_item_string( item_no, strval);
            show_item( item_no, ON);
	  }
        }
        else
        {
	  if (useInfostat == 0)
	  {
            find_item_string( item_no, txt );
            if (StatPortId > 0)
              writestatToVnmrJ( txt, "-" );
            else if(!logging)
              fprintf(stderr,"%s - %s\n",txt,strval);
	  }
	  else
	  {
            set_item_string( item_no, " ");
            show_item( item_no, OFF);
	  }
	}
    }
}

/*------------------------------------------------------------------
|
|       disp_val/3
|
|       This procedure displays an 4 byte integer
|
+----------------------------------------------------------------------*/
disp_val(item_no,val,show)
int      item_no, show;
unsigned long val;
/* display a processing index, 0 clears field */
{   char s[TXT_LEN+1];
    char txt[TXT_LEN+1];

    if (val)
        sprintf(s,"%lu",val);
    else
        sprintf(s,"    ");

    if (Wissun() && !logging)
    {

        if (show)
        {
	    if (useInfostat == 0 )
	    {
              find_item_string( item_no, txt );
              if (StatPortId > 0)
                writestatToVnmrJ( txt, s );
              else
                fprintf(stderr,"%s %s\n",txt,s);
/* fprintf(stderr,"%s + %s\n",txt,s); */
	    }
	    else
	    {
              set_item_string( item_no, s );
              show_item( item_no, ON);
	    }
        }
        else
        {
	    if (useInfostat == 0 )
	    {
              find_item_string( item_no, txt );
              if (StatPortId > 0)
                writestatToVnmrJ( txt, "-" );
              else
                fprintf(stderr,"%s - %s\n",txt,s);
	    }
	    else
	    {
              set_item_string( item_no, s );
              show_item( item_no, OFF);
	    }
        }
    }
}

static void
power_format(char *buf, long uwatts, char justify, int width, char *units)
{
    float watts;

/* fprintf(stderr,"buf= 0x%lx, uwatts: %ld, just: '%c', width: %d, units: 0x%x\n",
		buf,uwatts,justify,width,units); */

    watts = uwatts * 1e-6;
    if (watts < 99.9) {
	switch (justify) {
	  case 'r': case 'R':
#ifndef __INTERIX   /* this format causes interix to leak memory */
	   sprintf(buf,"%*.1f", width, watts);
#else
	    sprintf(buf,"%4.1f", watts);   /* this too leaks memory */
#endif
	    break;
	  case 'l': case 'L':
	    sprintf(buf,"%.1f", watts);
	    break;
	}
    } else {
	switch (justify) {
	  case 'r': case 'R':
#ifndef __INTERIX   /* this format causes interix to leak memory */
	   sprintf(buf,"%*d", width, (int)(watts + 0.5));
#else
	   sprintf(buf,"%4d", (int)(watts + 0.5));   /* this too leaks memory */
#endif
	    break;
	  case 'l': case 'L':
	    sprintf(buf,"%d", (int)(watts + 0.5));
	    break;
	}
    }
    if (units && *units) {
	strcat(buf, " ");
	strcat(buf, units);
    }
}

static void
updateRfMon(unsigned long *newval,
	    unsigned long *oldval,
	    char justify,
	    int width,
	    char *unitstr,
	    int str_num)
{
    if (*oldval != *newval) {
	char buf[20];
	*oldval = *newval;
	power_format(buf, *newval, justify, width, unitstr);
	disp_string(str_num, buf);
    }
}

static double
calcRfMonPct(unsigned long power, unsigned long limit)
{
  double rtn;
  if ((limit <= 0) || (power <= 0))
    rtn = 0.0;
  else
    rtn = ((double)power) / ((double)limit);
  return( rtn );
}

static void
updateRfMonAvg(statblock)
AcqStatBlock *statblock;
{
	int i, ok=0;
	for (i=0; i<4 && ok==0; i++)
	{
	  if ( (statblock->AcqShortRfPower[i]) != (CurrentStatBlock.AcqShortRfPower[i]))
	    ok = 1;
	  if ( (statblock->AcqShortRfLimit[i]) != (CurrentStatBlock.AcqShortRfLimit[i]))
	    ok = 1;
	  if ( (statblock->AcqLongRfPower[i])  != (CurrentStatBlock.AcqLongRfPower[i]))
	    ok = 1;
	  if ( (statblock->AcqLongRfLimit[i])  != (CurrentStatBlock.AcqLongRfLimit[i]))
	    ok = 1;
	}
        /*
         * Set "ok" here because that what effectively happened in the
         * previous, faulty version, in which "i" in the above loop overruns
         * the array. Setting "ok" here causes the "rfmonval" to be
         * printed out every time. Do a final fix after the release.
         */
        //ok = 1;

	if (ok == 1)
	{
	  double dtmp=0.0, RFMonPct = 0.0;
	  char buf[20];
	  for (i=0; i<4; i++)
	  {
	    dtmp = calcRfMonPct( (statblock->AcqShortRfPower[i]), (statblock->AcqShortRfLimit[i]) );
	    if (dtmp > RFMonPct)
	      RFMonPct = dtmp;
	    dtmp = calcRfMonPct( (statblock->AcqLongRfPower[i]), (statblock->AcqLongRfLimit[i]) );
	    if (dtmp > RFMonPct)
	      RFMonPct = dtmp;
	  }
	  RFMonPct *= 100.0;
	  sprintf(buf, " %1.1f", RFMonPct);
	  disp_string(RFMonVal, buf);
  	  checkLogEntry(RFMonVal,buf);

	}
}

/*------------------------------------------------------------------
|
|       UpdateStatScrn()
|       Update the acquisition status screen
+------------------------------------------------------------------*/
updatestatscrn(statblock)
AcqStatBlock *statblock;
{
    static int firsttime = 1;
    static float damp = 0.4;    /* Lock level damping; 0=>none, 1=>complete */
    static int lastFTS = -500;
    static int skipFTS = 0;
    float damplevel;
    int i;
    struct tm *tmtime;
    char  *chrptr;
    char  datetim[27];
    /*struct timeval clock;
    struct timezone tzone;
     */
    /* ---- get the present local date and time --- */
    /*gettimeofday(&clock,&tzone);
    timedate = clock.tv_sec;
     */

    /*
    if (useInfostat == 0 && StatPortId <= 0)
    {
    fprintf(stderr,"Acqstate = %d, InQue = %d, User:'%s', Exp:'%s'\n",
	statblock->Acqstate,statblock->AcqExpInQue,statblock->AcqUserID,
	statblock->AcqExpID);
    fprintf(stderr,"Fid Elem = %lu, CT = %ld, LSDV = 0x%hx \n",
	statblock->AcqFidElem,statblock->AcqCT,statblock->AcqLSDV);
    fprintf(stderr,"Cmplt Time = %ld, Rem T = %ld, Data T = %ld \n",
	statblock->AcqCmpltTime,statblock->AcqRemTime,statblock->AcqDataTime);
    fprintf(stderr,
        "Lk Lvl = %hd, SpinS = %hd, SpinA: %hd, VTS:%hd, VTA:%hd Samp: %hd\n",
	statblock->AcqLockLevel,statblock->AcqSpinSet,statblock->AcqSpinAct,
	statblock->AcqVTSet,statblock->AcqVTAct,statblock->AcqSample);
    }
     */
    if (firsttime) {
        initCurrentStatBlock(statblock);
    }

    if (CurrentStatBlock.Acqstate != statblock->Acqstate)
    {
        /*fprintf(stderr,"Stat\n");*/
        CurrentStatBlock.Acqstate = statblock->Acqstate;
        infoState = 0;
        /*
        showstatus();
         */
    }
    if (useInfostat == 0)
        showInfostatus();
    else
        showstatus();

    if (statblock->Acqstate < ACQ_IDLE)
    {   if (statblock->Acqstate > ACQ_INACTIVE)
    {
        disp_val(PendVal,(unsigned long) 0,HIDE);
        return(0);
    }
    }

    if (CurrentStatBlock.AcqExpInQue != statblock->AcqExpInQue)
    {
        int mode;
        /*fprintf(stderr,"Que\n");*/
        CurrentStatBlock.AcqExpInQue = statblock->AcqExpInQue;
        mode = (CurrentStatBlock.AcqExpInQue) ? SHOWIT : HIDE;
        disp_val(PendVal,(unsigned long) CurrentStatBlock.AcqExpInQue,mode);
    }

    if (strcmp(CurrentStatBlock.AcqUserID,statblock->AcqUserID) != 0)
    {
        /*fprintf(stderr,"User\n");*/
        strcpy(CurrentStatBlock.AcqUserID,statblock->AcqUserID);
        disp_string(UserVal,CurrentStatBlock.AcqUserID);
    }

    if (strcmp(CurrentStatBlock.AcqExpID,statblock->AcqExpID) != 0)
    {
        /*fprintf(stderr,"Exp\n");*/
        strcpy(CurrentStatBlock.AcqExpID,statblock->AcqExpID);
        disp_string(ExpVal,CurrentStatBlock.AcqExpID);
    }

    if (strcmp(CurrentStatBlock.AcqUserID,"") != 0)
    {
        if (CurrentStatBlock.AcqFidElem != statblock->AcqFidElem)
        {
            CurrentStatBlock.AcqFidElem = statblock->AcqFidElem;
            /*fprintf(stderr,"FID\n");*/
            if (CurrentStatBlock.AcqFidElem > 0)
                disp_val(ArrayVal,(unsigned long) CurrentStatBlock.AcqFidElem,SHOWIT);
            else
                disp_val(ArrayVal,(unsigned long) 0,HIDE);
        }

        if (CurrentStatBlock.AcqCT != statblock->AcqCT)
        {
            CurrentStatBlock.AcqCT = statblock->AcqCT;
            /*fprintf(stderr,"CT\n");*/
            if (CurrentStatBlock.AcqCT > 0L)
                disp_val(CT_Val,(unsigned long) CurrentStatBlock.AcqCT,SHOWIT);
            else
                disp_val(CT_Val,(unsigned long) 0,HIDE);
        }
    }
    else
    {
        CurrentStatBlock.AcqFidElem = 0L;
        CurrentStatBlock.AcqCT = 0L;
        if (useInfostat == 0)
        {
            if (infoState == 0)
            {
                disp_val(ArrayVal,(unsigned long) CurrentStatBlock.AcqFidElem,HIDE);
                disp_val(CT_Val,(unsigned long) CurrentStatBlock.AcqCT,HIDE);
            }
            if (CurrentStatBlock.Acqstate == ACQ_INACTIVE
                    || CurrentStatBlock.Acqstate == ACQ_IDLE
                    || CurrentStatBlock.Acqstate == ACQ_REBOOT
                    || CurrentStatBlock.Acqstate == ACQ_INTERACTIVE)
                infoState = 1;
            else /* if (CurrentStatBlock.Acqstate == ACQ_ACQUIRE) */
                infoState = 0;
        }
        else
        {
            disp_val(ArrayVal,(unsigned long) CurrentStatBlock.AcqFidElem,HIDE);
            disp_val(CT_Val,(unsigned long) CurrentStatBlock.AcqCT,HIDE);
        }
    } 

    if (CurrentStatBlock.AcqLSDV != statblock->AcqLSDV)
    {
        /*fprintf(stderr,"LSDV\n");*/
        CurrentStatBlock.AcqLSDV = statblock->AcqLSDV;
        showLSDV();
    }

    if (CurrentStatBlock.AcqCmpltTime != statblock->AcqCmpltTime)
    {
        CurrentStatBlock.AcqCmpltTime = statblock->AcqCmpltTime;
        /*fprintf(stderr,"CmpltT\n");*/
        if (CurrentStatBlock.AcqCmpltTime > 0L)
        {
            tmtime = localtime(&(CurrentStatBlock.AcqCmpltTime));
            chrptr = asctime(tmtime);
            strcpy(datetim,chrptr);
            datetim[19] = 0;
            disp_string(CompVal,&datetim[11]);
        }
        else
            disp_string(CompVal,"");	/* blank out value */

    }

    if (CurrentStatBlock.AcqRemTime != statblock->AcqRemTime)
    {
        int hrs,mins,sec;
        long time;
        char remtime[20];

        remtime[0] = 0;
        CurrentStatBlock.AcqRemTime = statblock->AcqRemTime;
        /*fprintf(stderr,">>> RemTime %ld\n",CurrentStatBlock.AcqRemTime);*/
        if (CurrentStatBlock.AcqRemTime > 0L)
        {
            time = statblock->AcqRemTime;
            hrs = (int) (time / 3600L);
            mins = (int) ( (time % 3600L) / 60L );
            sec = (int) (time % 60L);
            sprintf(remtime,"%02d:%02d:%02d",hrs,mins,sec);
            disp_string(RemainVal,remtime);
        }
        else
            disp_string(RemainVal,""); /* blank out value */
    }

    if (CurrentStatBlock.AcqDataTime != statblock->AcqDataTime)
    {
        CurrentStatBlock.AcqDataTime = statblock->AcqDataTime;
        /*fprintf(stderr,"DataT\n");*/
        if (CurrentStatBlock.AcqDataTime > 0L)
        {
            tmtime = localtime(&(CurrentStatBlock.AcqDataTime));
            chrptr = asctime(tmtime);
            strcpy(datetim,chrptr);
            datetim[19] = 0;
            datetim[10] = '-';
            // disp_string(DataVal,&datetim[11]);
            disp_string(DataVal,&datetim[4]);
        }
        else
            disp_string(DataVal,"");	/* blank out value */
    }

    /* ---  calculate the lock level percentage --- */
    Locklevel = (float)statblock->AcqLockLevel;
    if (Locklevel > 1300.0)
    {
        Locklevel = 1300.0 / 16.0 + (Locklevel - 1300) / 15.0;
        if (Locklevel > 100.0 ) Locklevel = 100.0;
    }
    else
        Locklevel /= 16.0;

    /* Damp change in Locklevel */
    if (LockPercent <= 100 && LockPercent >= 0) {
        damplevel = damp * LockPercent + (1 - damp) * Locklevel;
        Locklevel = rint(damplevel * 16) / 16;
    }
    /*fprintf(stderr,"old=%.4f, damp=%.4f, new=%.4f\n",
            LockPercent, damplevel, Locklevel);*/

    if (rint(LockPercent * 16) / 16 != Locklevel)
    {
        char buf[20];

        /*fprintf(stderr,"LkLvl\n");*/
        if (Locklevel > 0)
        {
            sprintf(buf,"%4.1f",Locklevel);
        }
        else
            buf[0]=0;
        disp_string(LKVal,buf);
        checkLogEntry(LKVal,buf);

    }
    LockPercent = damplevel;


#ifndef __INTERIX   /* exclude this, during the call to these functions
 *  a memory leak occurs, it appears to be relate to the
 *  power_format() call */
    /* Update RF power monitor info */
    for (i=0; i<4; i++) {
        updateRfMon(&statblock->AcqShortRfPower[i],
                &CurrentStatBlock.AcqShortRfPower[i],
                'R', 4, 0,
                RF10s1AvgVal + i * 4);
        updateRfMon(&statblock->AcqShortRfLimit[i],
                &CurrentStatBlock.AcqShortRfLimit[i],
                'L', 4, "W",
                RF10s1LimVal + i * 4);
        updateRfMon(&statblock->AcqLongRfPower[i],
                &CurrentStatBlock.AcqLongRfPower[i],
                'R', 4, 0,
                RF5m1AvgVal + i * 4);
        updateRfMon(&statblock->AcqLongRfLimit[i],
                &CurrentStatBlock.AcqLongRfLimit[i],
                'L', 4, "W",
                RF5m1LimVal + i * 4);
    }
    updateRfMonAvg(statblock);
#endif   /* __INTERIX */

    if (strcmp(CurrentStatBlock.probeId1, statblock->probeId1) != 0)
    {
        /* This is already a string, copy the string and send it on. */
        strcpy(CurrentStatBlock.probeId1, statblock->probeId1);
        disp_string(ProbeID1, CurrentStatBlock.probeId1);
    }

    if (strcmp(CurrentStatBlock.gradCoilId, statblock->gradCoilId) != 0)
    {
        /* This is already a string, copy the string and send it on. */
        strcpy(CurrentStatBlock.gradCoilId, statblock->gradCoilId);
        disp_string(GradCoilID, CurrentStatBlock.gradCoilId);
    }

    if (CurrentStatBlock.AcqSpinSet != statblock->AcqSpinSet)
    {
        char buf[20];

        CurrentStatBlock.AcqSpinSet = statblock->AcqSpinSet;
        /*fprintf(stderr,"Spin Set\n");*/
        if (CurrentStatBlock.AcqSpinSet >= 0)
        {
            if (!SPNpan2)  {
                show_item(SPN2Title, ON);
                SPNpan2 = 1;
            }
            sprintf(buf,"%d Hz",CurrentStatBlock.AcqSpinSet);
        }
        else
        {
            if (SPNpan2)  {
                show_item( SPN2Title, ON);
                SPNpan2 = 0;
            }
            buf[0]=0;
        }
        disp_string(SPNVal2,buf);
        checkLogEntry(SPNVal2,buf);
    }

    if (CurrentStatBlock.AcqSpinAct != statblock->AcqSpinAct)
    {
        char buf[20];

        CurrentStatBlock.AcqSpinAct = statblock->AcqSpinAct;
        /*fprintf(stderr,"Spin Act\n");*/
        if (CurrentStatBlock.AcqSpinAct >= 0) {
            /*sprintf(buf,"%d Hz",CurrentStatBlock.AcqSpinAct);*/
            sprintf(buf,"%u Hz",(unsigned int) CurrentStatBlock.AcqSpinAct);
            checkLogEntry(SPNVal,buf);
        }
        else
            buf[0]=0;
        disp_string(SPNVal,buf);
    }

    if (CurrentStatBlock.AcqSpinSpeedLimit != statblock->AcqSpinSpeedLimit)
    {
        char buf[20];

        CurrentStatBlock.AcqSpinSpeedLimit = statblock->AcqSpinSpeedLimit;
        if (CurrentStatBlock.AcqSpinSpeedLimit >= 0)
            sprintf(buf,"%u",(unsigned int)CurrentStatBlock.AcqSpinSpeedLimit);
        else
            buf[0]=0;
        disp_string(MASSpeedLimit,buf);
    }

    if (CurrentStatBlock.AcqSpinSpan != statblock->AcqSpinSpan)
    {
        char buf[20];

        CurrentStatBlock.AcqSpinSpan = statblock->AcqSpinSpan;
        if (CurrentStatBlock.AcqSpinSpan >= 0)
            sprintf(buf,"%u",(unsigned int) CurrentStatBlock.AcqSpinSpan);
        else
            buf[0]=0;
        disp_string(MASBearSpan,buf);
    }

    if (CurrentStatBlock.AcqSpinAdj != statblock->AcqSpinAdj)
    {
        char buf[20];

        CurrentStatBlock.AcqSpinAdj = statblock->AcqSpinAdj;
        /* Allow negative numbers */
        sprintf(buf,"%i",(int) CurrentStatBlock.AcqSpinAdj);
        disp_string(MASBearAdj,buf);
    }

    if (CurrentStatBlock.AcqSpinMax != statblock->AcqSpinMax)
    {
        char buf[20];

        CurrentStatBlock.AcqSpinMax = statblock->AcqSpinMax;
        if (CurrentStatBlock.AcqSpinMax >= 0)
            sprintf(buf,"%u",(unsigned int) CurrentStatBlock.AcqSpinMax);
        else
            buf[0]=0;
        disp_string(MASBearMax,buf);
    }

    if (CurrentStatBlock.AcqSpinActSp != statblock->AcqSpinActSp)
    {
        char buf[20];

        CurrentStatBlock.AcqSpinActSp = statblock->AcqSpinActSp;
        if (CurrentStatBlock.AcqSpinActSp >= 0)
            sprintf(buf,"%u",(unsigned int) CurrentStatBlock.AcqSpinActSp);
        else
            buf[0]=0;
        disp_string(MASActiveSetPoint,buf);
    }

    if (CurrentStatBlock.AcqSpinProfile != statblock->AcqSpinProfile)
    {
        char buf[20];

        CurrentStatBlock.AcqSpinProfile = statblock->AcqSpinProfile;
        if (CurrentStatBlock.AcqSpinProfile >= 0)
            sprintf(buf,"%u",(unsigned int) CurrentStatBlock.AcqSpinProfile);
        else
            buf[0]=1;
        disp_string(MASProfileSetting,buf);
    }

    if (CurrentStatBlock.AcqVTSet != statblock->AcqVTSet)
    {
        char buf[20];

        CurrentStatBlock.AcqVTSet = statblock->AcqVTSet;
        /*fprintf(stderr,"VT Set\n");*/
        if (CurrentStatBlock.AcqVTSet != 30000)
        {
            if (!VTpan2)  {
                show_item( VT2Title, ON);
                VTpan2 = 1;
            }
            sprintf(buf,"%4.1f C",(float) CurrentStatBlock.AcqVTSet/10);
        }
        else
        {
            if (VTpan2)  {
                show_item( VT2Title, ON);
                VTpan2 = 0;
            }
            buf[0]=0;
        }
        disp_string(VTVal2,buf);
        checkLogEntry(VTVal2,buf);

    }

    if (CurrentStatBlock.AcqVTAct != statblock->AcqVTAct)
    {
        char buf[20];

        CurrentStatBlock.AcqVTAct = statblock->AcqVTAct;
        /*fprintf(stderr,"VT Act\n");*/
        if (CurrentStatBlock.AcqVTAct != 30000)
        {
            if (!VTpan1)  {
                show_item( VTTitle, ON);
                VTpan1 = 1;
            }
            sprintf(buf,"%4.1f C",(float) CurrentStatBlock.AcqVTAct/10);
        }
        else
        {
            if (VTpan1)  {
                show_item( VTTitle, ON);
                VTpan1 = 0;
            }
            buf[0]=0;
        }
        disp_string(VTVal,buf);
        checkLogEntry(VTVal,buf);
    }

    if ( !skipFTS &&  ! access("/vnmr/bin/fts",X_OK) )
    {
        char buf[20];
        int  val;

        if ( ! getFTS("/vnmr/bin/fts",  &val) && (val != lastFTS) )
        {
           lastFTS = val;
           sprintf(buf,"%4.1f C",( (float) lastFTS / 10.0));
           disp_string(FTSVal,buf);
        }
    }
    skipFTS++;
    skipFTS %= 16;

    if (CurrentStatBlock.AcqSample != statblock->AcqSample)
    {
        CurrentStatBlock.AcqSample = statblock->AcqSample;
        if (CurrentStatBlock.AcqSample > 0)
            disp_val(SampleVal,(unsigned long) CurrentStatBlock.AcqSample,SHOWIT);
        else
            disp_val(SampleVal,(unsigned long) 0,HIDE);
    }

    if (useInfostat == 0)  /* VnmrJ VNMRS system */
    {
        if (CurrentStatBlock.AcqRack != statblock->AcqRack)
        {
            CurrentStatBlock.AcqRack = statblock->AcqRack;
            if (CurrentStatBlock.AcqRack > 0)
                disp_val(SampleRackVal,(unsigned long) CurrentStatBlock.AcqRack,SHOWIT);
            else
                disp_val(SampleRackVal,(unsigned long) 0,HIDE);
        }
        if (CurrentStatBlock.AcqZone != statblock->AcqZone)
        {
            CurrentStatBlock.AcqZone = statblock->AcqZone;
            if (CurrentStatBlock.AcqZone > 0)
                disp_val(SampleZoneVal,(unsigned long) CurrentStatBlock.AcqZone,SHOWIT);
            else
                disp_val(SampleZoneVal,(unsigned long) 0,HIDE);
        }
    }

    if (CurrentStatBlock.Acqstate <= ACQ_INACTIVE) {
        return 0;
    } else if (firsttime) {
        firsttime = 0;
    }

    if (CurrentStatBlock.AcqLockGain != statblock->AcqLockGain) {
        CurrentStatBlock.AcqLockGain = statblock->AcqLockGain;
        write_info_val(lockGain, statblock->AcqLockGain);
    }
    if (CurrentStatBlock.AcqLockPower != statblock->AcqLockPower) {
        CurrentStatBlock.AcqLockPower = statblock->AcqLockPower;
        write_info_val(lockPower, statblock->AcqLockPower);
    }
    if (CurrentStatBlock.AcqLockPhase != statblock->AcqLockPhase) {
        CurrentStatBlock.AcqLockPhase = statblock->AcqLockPhase;
        write_info_val(lockPhase, statblock->AcqLockPhase);
    }
    if (CurrentStatBlock.AcqShimSet != statblock->AcqShimSet) {
        CurrentStatBlock.AcqShimSet = statblock->AcqShimSet;
        write_info_val(shimSet, statblock->AcqShimSet);
    }

    for (i=0; i<MAX_SHIMS_CONFIGURED; i++) {
        if (CurrentStatBlock.AcqShimValues[i] != statblock->AcqShimValues[i]) {
            CurrentStatBlock.AcqShimValues[i] = statblock->AcqShimValues[i];
            write_shim_info(statblock->AcqShimSet,
                    i, statblock->AcqShimValues[i]);
        }
    }
    return(0);
}
/*-------------------------------------------------------------------
|
|       showstatus()
|         display the apropriate acqusition status message
|
+-------------------------------------------------------------------*/
showstatus()
{
    char message[50];

    switch(CurrentStatBlock.Acqstate)
    {
        case ACQ_INACTIVE:
                        strcpy(message,"Inactive");
                        break;
        case ACQ_REBOOT:
                        strcpy(message,"Rebooted");
                        break;
        case ACQ_IDLE:
                        strcpy(message,"Idle");
                        break;
        case ACQ_PARSE:
                        strcpy(message,"Active");
                        break;
        case ACQ_PREP:
                        strcpy(message,"Working");
                        break;
        case ACQ_SYNCED:
                        strcpy(message,"Ready");
                        break;
        case ACQ_ACQUIRE:
                        strcpy(message,"Acquiring");
                        break;
        case ACQ_PAD:
                        strcpy(message,"Pre-Acquisition");
                        break;
        case ACQ_VTWAIT:
                        strcpy(message,"VT Regulation");
                        break;
        case ACQ_SPINWAIT:
                        strcpy(message,"Spin Regulation");
                        break;
        case ACQ_AGAIN:
                        strcpy(message,"Auto Set Gain");
                        break;
        case ACQ_ALOCK:
                        strcpy(message,"Auto Locking");
                        break;
        case ACQ_AFINDRES:
                        strcpy(message,"Lock: Find Res.");
                        break;
        case ACQ_APOWER:
                        strcpy(message,"Lock: Adj. Power");
                        break;
        case ACQ_APHASE:
                        strcpy(message,"Lock: Adj. Phase");
                        break;
        case ACQ_FINDZ0:
                        strcpy(message,"Find Z0");
                        break;
        case ACQ_SHIMMING:
                        strcpy(message,"Shimming");
                        break;
        case ACQ_SMPCHANGE:
                        strcpy(message,"Changing Sample");
                        break;
        case ACQ_RETRIEVSMP:
                        strcpy(message,"Retrieving Sample");
                        break;
        case ACQ_LOADSMP:
                        strcpy(message,"Loading Sample");
                        break;
	case ACQ_INTERACTIVE:
                        strcpy(message,"Interactive");
                        break;
	/*
        case ACQ_FID:
                        strcpy(message,"FID Display");
                        break;
       */
        case ACQ_TUNING:
                        strcpy(message,"Tuning");
                        break;
        case ACQ_PROBETUNE:
                        strcpy(message,"Probe Tuning");
                        break;
	default:
                        strcpy(message, (acq_ok) ? "Active" : "Inactive");
			break;
    }
    disp_string(StatusVal,message);
}
/*-------------------------------------------------------------------
|
|       showInfostatus()
|         display the apropriate acqusition status message
|
+-------------------------------------------------------------------*/
showInfostatus()
{
    char message[50];
 
    switch(CurrentStatBlock.Acqstate)
    {
        case ACQ_INACTIVE:
                        strcpy(message,"Inactive");
                        break;
        case ACQ_REBOOT:
                        strcpy(message,"Rebooted");
                        break;
        case ACQ_IDLE:
                        strcpy(message,"Idle");
                        break;
        case ACQ_PARSE:
                        strcpy(message,"Active");
                        break;
        case ACQ_PREP:
                        strcpy(message,"Working");
                        break;
        case ACQ_SYNCED:
                        strcpy(message,"Ready");
                        break;
        case ACQ_ACQUIRE:
                        strcpy(message,"Acquiring");
                        break;
        case ACQ_PAD:
                        strcpy(message,"PreAcq");
                        break;
        case ACQ_VTWAIT:
                        strcpy(message,"VTReg");
                        break;
        case ACQ_SPINWAIT:
                        strcpy(message,"SpinReg");
                        break;
        case ACQ_AGAIN:
                        strcpy(message,"AutoGain");
                        break;
        case ACQ_HOSTGAIN:
                        strcpy(message,"HostGain");
                        break;
        case ACQ_ALOCK:
                        strcpy(message,"AutoLock");
                        break;
        case ACQ_AFINDRES:
                        strcpy(message,"LockFindRes");
                        break;
        case ACQ_APOWER:
                        strcpy(message,"LockPower");
                        break;
        case ACQ_APHASE:
                        strcpy(message,"LockPhase");
                        break;
        case ACQ_FINDZ0:
                        strcpy(message,"FindZ0");
                        break;
        case ACQ_SHIMMING:
                        strcpy(message,"Shimming");
                        break;
        case ACQ_HOSTSHIM:
                        strcpy(message,"HostShim");
                        break;
        case ACQ_SMPCHANGE:
                        strcpy(message,"ChangeSample");
                        break;
        case ACQ_RETRIEVSMP:
                        strcpy(message,"RetrieveSample");
                        break;
        case ACQ_LOADSMP:
                        strcpy(message,"LoadSample");
                        break;
        case ACQ_ACCESSSMP:
                        strcpy(message,"AccessOpen");
                        break;
        case ACQ_ESTOPSMP:
                        strcpy(message,"ESTOP");
                        break;
        case ACQ_MMSMP:
                        strcpy(message,"MagnetMotion");
                        break;
        case ACQ_HOMESMP:
                        strcpy(message,"RobotInitialize");
                        break;
	case ACQ_INTERACTIVE:
                        strcpy(message,"Interactive");
                        break;
	/*
        case ACQ_FID:
                        strcpy(message,"FIDDisplay");
                        break;
       */
        case ACQ_TUNING:
                        strcpy(message,"Tuning");
                        break;
        case ACQ_PROBETUNE:
                        strcpy(message,"ProbeTune");
                        break;
	default:
                        strcpy(message, (acq_ok) ? "Active" : "Inactive");
			break;
    }
    disp_string(StatusVal,message);
}    
/*-------------------------------------------------------
|
|       returns the LSDV status word (Lock,Spin,Decoupler,VT)
|       each status is coded in two bits
|
|                       |  Spinner Funcs    |
|       |  VT   |  Dec. |Pro|Mod| Ej| Spin  |  Lock | LkMode|
|       -----------------------------------------------------
|       |   |   |   |   |   |   |   |   |   |   |   |   |   |LSDV Word.
|       -----------------------------------------------------
|
|       0 = Off
|       1 = Regulated or On; hardware Lock ON;
|       2 = Non-Regulated or Gated (dec); hardware autolock ON;
|
|       SPinner: Mod- Mode 0=Speed(Hz); 1=Flow (time)
|                Ej - Eject 0=Off; 1=On;
|                Pro - Probe Type 0=Liquids; 1=Solids;
|
+--------------------------------------------------------*/
showLSDV()
{
    char message[20];
    int status;
    int stat;

    status = CurrentStatBlock.AcqLSDV;                                   
    stat = status & 0x0003;
    if (debug)
       fprintf(stderr,"LSDV = 0x%x\n",status);

    /*  ------------- N o t  U S E D ---------------------------------------
    switch(stat)
    {
	case 0:
		if (lockchoice != 0)
		{
		    panel_set_value(LK_lock_choice,0);
		    lockchoice = 0;
		    /*printf("Hardware Lock: OFF  ");*/
		/*}
		break;
	case 1:
		if (lockchoice != 2)
		{
		    panel_set_value(LK_lock_choice,2);
		    lockchoice = 2;
		    /*printf("Hardware Lock: AUTO  ");*/
		/*}
		break;
	case 2:
		if (lockchoice != 1)
		{
		    panel_set_value(LK_lock_choice,1);
		    lockchoice = 1;
		    /*printf("Hardware Lock: ON  ");*/
		/*}
		break;
    }
    ----------------------------------------------------------------- */

    stat = (status >> 2) & 0x0003;
    if (debug)
        fprintf(stderr,"Lockstate = 0x%x\n",stat);
    if (LKstat != stat)
    {
        getmode(stat,message);
        disp_string(LockVal,message);
	LKstat = stat;
	write_info_mode("lockon",0,stat);
    }

    stat = (status >> 4) & 0x0003;
    if (debug)
        fprintf(stderr,"Spinstate = 0x%x\n",stat);
    if (Spinstat != stat)
    {
        getmode(stat,message);
    	disp_string(SpinVal,message);
	Spinstat = stat;
	write_info_mode("spinon",1,stat);
    }

    /*   sample status ------------------------------- */
    stat = (status >> 6) & 0x0001;
    if (useInfostat == 0)
    {
       if (Sampstat != stat)
       {
          getspinmode(stat,message);
          disp_string(AIRVal,message);
          Sampstat = stat;
       }
    }

/*    if (!stat)
    {
	if (Sampstat != 0)
	{
	    panel_set_value(LK_sample_choice,0);
	    sampchoice = Sampstat = 0;
            if (useInfostat == 0)
    	      disp_string(AIRVal,"insert"); /* doesn't work? */
	    /*printf("INSERT  ");*/
	/*}
    }
    else
    {
	if (Sampstat != 1)
	{
	    panel_set_value(LK_sample_choice,1);
	    ParameterLine(2,13,YELLOW,"SPIN:EJECTED");
	    Sampstat = sampchoice = 1;
            if (useInfostat == 0)
    	      disp_string(AIRVal,"eject"); /* doesn't work? */
	    /*printf("EJECT  ");*/
/*	}
    }
    stat = (status >> 7) & 0x0001;
    if (!stat)
    {
	/*printf("Speed  ");*/
    /*}
    else
    {
	/*printf("Rate  ");*/
    /*}
    stat = (status >> 8) & 0x0001;
    if (!stat)
    {
	/*printf("Liquids  ");*/
    /*}
    else
    {
	/*printf("Solids  ");*/
    /*}
   ------------------------------------------------------------  */

    stat = (status >> 9) & 0x0003;
    if (debug)
        fprintf(stderr,"Decstate = 0x%x\n",stat);
    if (Decstat != stat)
    {
        switch(stat)
        {
            case 0:
                            strcpy(message,"Off");
                            break;
            case 1:
                            strcpy(message,"On");
                            break;
            case 2:
                            strcpy(message,"Gated");
                            break;
	    default:
                            strcpy(message,"");
                            break;
        }
        disp_string(DecVal,message);
        Decstat = stat;
    }

    stat = (status >> 11) & 0x001f;
    if (debug)
       fprintf(stderr,"VTstate = 0x%x\n",stat);
    if (stat == NOTPRESENT)
    { 
	    if (VTpan)  {
                show_item( VT_Title, OFF);
/*              disp_string(VT_Val,""); */
	        VTpan = 0; 
	        }
            getmode(stat,message);
            disp_string(VT_Val,message);
	    write_info_mode("vton",2,stat);
    }
    else
    {
	if (!VTpan)  {
	    show_item( VT_Title, ON);
	    VTpan = 1; 
	}
        if (VTstat != stat)
        {
            getmode((stat&0x3),message);
            
//	    if (status >> 13) 
//              strcat(message,"HW error");
            disp_string(VT_Val,message);
	    VTstat = stat;
	    write_info_mode("vton",2,stat);
        }
    }
}
/*-----------------------------------------------------------------
|
|       getmode()
|          returns the string message discribing the condition of the
|          device, (Regulated, Not Regulated, Off)
+------------------------------------------------------------------*/
getmode(bits,msgeptr)
int bits;
char *msgeptr;
{
  if (useInfostat == 0)
  {
    switch(bits)
    {
        case OFF:
                        strcpy(msgeptr,"Off");
                        break;
        case REGULATED:
                        strcpy(msgeptr,"Regulated");
                        break;
        case NOTREG:
                        strcpy(msgeptr,"NotReg");
                        break;
        case NOTPRESENT:
                        strcpy(msgeptr,"NotPresent");
                        break;
        default:
                        *msgeptr = 0;
                        break;
    }
  }
  else
  {
    switch(bits)
    {
        case OFF:
                        strcpy(msgeptr,"Off");
                        break;
        case REGULATED:
                        strcpy(msgeptr,"Regulated");
                        break;
        case NOTREG:
                        strcpy(msgeptr,"Not Reg.");
                        break;
        case NOTPRESENT:
                        strcpy(msgeptr,"Not Present.");
                        break;
        default:
                        *msgeptr = 0;
                        break;
    }
  }
}
#define EJECT 0
#define INSERT 1
/*-----------------------------------------------------------------
|
|       getspinmode()
|          returns the string message discribing the condition of the
|          device, (insert, eject)
+------------------------------------------------------------------*/
getspinmode(bits,msgeptr)
int bits;
char *msgeptr;
{
  if (useInfostat == 0)
  {
    switch(bits)
    {
        case EJECT:
                        strcpy(msgeptr,"eject");
                        break;
        case INSERT:
                        strcpy(msgeptr,"insert");
                        break;
        default:
                        strcpy(msgeptr,"");
                        break;
    }
  }
  else
  {
    switch(bits)
    {
        case EJECT:
                        strcpy(msgeptr,"Eject");
                        break;
        case INSERT:
                        strcpy(msgeptr,"Insert");
                        break;
        default:
                        strcpy(msgeptr,"");
                        break;
    }
  }
}
/*-----------------------------------------------------------------
|
|       write_info_mode()
|          sends the message string describing whether
|	   mode is turned on (true, false)
+------------------------------------------------------------------*/
static void write_info_mode(txt,key,stat)
char *txt;
int key, stat;
/* txt="lockon" key=0; txt="spinon" key=1; txt="vton" key=2 */
{
  char tmpOn[6];
  if (useInfostat == 0  && !logging)
  {
    switch(stat)
    {
      case OFF:
      case NOTPRESENT:
	strcpy(tmpOn,"false");
	break;
      case REGULATED:
      case NOTREG:
	strcpy(tmpOn,"true");
	break;
      default:
	strcpy(tmpOn,"false");
	break;
    }
    if (infoModeOn[key] != tmpOn[0])
    {
      infoModeOn[key] = tmpOn[0];
      if (StatPortId > 0)
        writestatToVnmrJ( txt, tmpOn );
      else
        fprintf(stderr,"%s %s\n",txt,tmpOn);
    }
  }
}

/*-----------------------------------------------------------------
|
|       write_shim_info()
|          sends the message string giving the shim setting
+------------------------------------------------------------------*/
static void write_shim_info(shimset, dacno, value)
     int shimset;
     int dacno;
     int value;
{
    extern char *get_shimname();
    extern void init_shimnames_by_setnum();
    char val[20];
    char name[20];
    char *shimname;

    if (useInfostat == 0 && !logging) {
	sprintf(val, "%d", value);
	init_shimnames_by_setnum(shimset);
	if (shimname = get_shimname(dacno)) {
	    if (*shimname == '\0') {
		sprintf(name, "shim_%d", dacno);
		shimname = name;
	    }
	    if (StatPortId > 0)
		writestatToVnmrJ(shimname, val);
	    else
		fprintf(stderr,"%s %s\n", shimname, val);
	}
    }
}

/*-----------------------------------------------------------------
|
|       write_info_val()
+------------------------------------------------------------------*/
static void write_info_val(item, value)
     int item;
     int value;
{
    char name[TXT_LEN+1];
    char val[20];

    if (useInfostat == 0 && !logging) {
	sprintf(val, "%d", value);
	find_item_string(item, name);
	if (StatPortId > 0)
	    writestatToVnmrJ(name, val);
	else
	    fprintf(stderr,"%s %s\n", name, val);
    }
}

/*-----------------------------------------------------------------
|	initCurrentStatBlock()
|       Initializes the global CurrentStatBlock to be different
|	from a given "statblock".  This ensures that the values
|	in the given statblock will be printed.
+------------------------------------------------------------------*/
initCurrentStatBlock(statblock)
AcqStatBlock *statblock;
{
    int i;

    CurrentStatBlock.AcqExpInQue = ~statblock->AcqExpInQue;
    strcpy(CurrentStatBlock.AcqUserID, "noID");
    strcpy(CurrentStatBlock.AcqExpID, "noID");
    CurrentStatBlock.AcqFidElem = ~statblock->AcqFidElem;
    CurrentStatBlock.AcqCT = ~statblock->AcqCT;
    CurrentStatBlock.AcqLSDV = ~statblock->AcqLSDV;
    LKstat = -1;
    Spinstat = -1;
    Decstat = -1;
    VTstat = -1;
    strcpy(infoModeOn, "uuu");
    CurrentStatBlock.AcqCmpltTime = ~statblock->AcqCmpltTime;
    CurrentStatBlock.AcqRemTime = ~statblock->AcqRemTime;
    CurrentStatBlock.AcqDataTime = ~statblock->AcqDataTime;
    LockPercent = 9999;
    CurrentStatBlock.AcqSpinSet = ~statblock->AcqSpinSet;
    CurrentStatBlock.AcqSpinAct = ~statblock->AcqSpinAct;
    CurrentStatBlock.AcqVTSet = ~statblock->AcqVTSet;
    CurrentStatBlock.AcqVTAct = ~statblock->AcqVTAct;
    CurrentStatBlock.AcqSample = ~statblock->AcqSample;
    CurrentStatBlock.AcqSpinSpeedLimit = ~statblock->AcqSpinSpeedLimit;
    CurrentStatBlock.AcqSpinSpan = ~statblock->AcqSpinSpan;
    CurrentStatBlock.AcqSpinAdj = ~statblock->AcqSpinAdj;
    CurrentStatBlock.AcqSpinMax = ~statblock->AcqSpinMax;
    CurrentStatBlock.AcqSpinActSp = ~statblock->AcqSpinActSp;
    CurrentStatBlock.AcqSpinProfile = ~statblock->AcqSpinProfile;
    strcpy(CurrentStatBlock.probeId1, "noID");
    strcpy(CurrentStatBlock.gradCoilId, "noID");

    CurrentStatBlock.AcqShimSet = ~statblock->AcqShimSet;
    CurrentStatBlock.AcqLockGain = ~statblock->AcqLockGain;
    CurrentStatBlock.AcqLockPower = ~statblock->AcqLockPower;
    CurrentStatBlock.AcqLockPhase = ~statblock->AcqLockPhase;
    for (i=0; i<MAX_SHIMS_CONFIGURED; i++) {
	CurrentStatBlock.AcqShimValues[i] = ~statblock->AcqShimValues[i];
    }
    for (i=0; i<4; i++) {
	CurrentStatBlock.AcqShortRfPower[i] = ~statblock->AcqShortRfPower[i];
	CurrentStatBlock.AcqShortRfLimit[i] = ~statblock->AcqShortRfLimit[i];
	CurrentStatBlock.AcqLongRfPower[i] = ~statblock->AcqLongRfPower[i];
	CurrentStatBlock.AcqLongRfLimit[i] = ~statblock->AcqLongRfLimit[i];
    }
}
