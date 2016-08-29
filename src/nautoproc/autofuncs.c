/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <signal.h>

#include <errno.h>

#include "errLogLib.h"
#include "mfileObj.h"
#include "shrMLib.h"
#include "shrexpinfo.h"
#include "asm.h"
#include "msgQLib.h"
#include "expQfuncs.h"
#include "procQfuncs.h"

/************************************************************************
 * Declarations for routines that have no include file for us to use.
 ************************************************************************/
extern int read_sample_info(FILE *stream,            /* asmfuncs.c      */
               struct sample_info *s);
extern int write_sample_info(FILE *stream,           /* asmfuncs.c      */
               struct sample_info *s);
extern int get_sample_info(struct sample_info *s,    /* asmfuncs.c      */
               char *keyword, char *info,
               int size, int *entry);
extern int update_sample_info(                       /* asmfuncs.c      */
               char *autodir, char *fname,
               char *match_target, char *match_value,
               char *update_target, char *update_value);
extern int entryQ_length(char *autodir,              /* asmfuncs.c      */
               char *filename);
extern int lockfile(char *lockfile);                 /* asmfuncs.c      */
extern int unlockfile(char *lockfile);               /* asmfuncs.c      */
extern int shutdownComm(void);                       /* commfuncs.c     */
extern void expQRelease(void);                       /* expQfuncs.c     */


#ifndef OK
#define OK 0
#endif
#ifndef ERROR
#define ERROR -1
#endif
#ifndef ENDOFFILE
#define ENDOFFILE -2
#endif

typedef struct _psgQentry_ {
				char ExpIdStr[256];
				char ExpInfoStr[256];
				struct sample_info SampleInfo;
			   }  PSG_Q_ENTRY;

#define MAXPATHL 256

#define NOENTRY 1
#define ENTRY_PRESENT 2

extern char autodir[];         /* automation directory */
extern char enterpath[];       /* path to enter file */
extern char samplepath[];      /* path to sampleinfo file */
extern char psgQpath[];        /* path to psg Queue directory */
extern char doneQpath[];       /* path to done Queue directory */
extern char systemdir[];       /* vnmr system directory */
extern pid_t VnmrPid;

/* Owner uid & gid of enterQ file */
extern uid_t enter_uid;
extern gid_t enter_gid;

SHR_EXP_INFO expInfo = NULL;   /* start address of shared Exp. Info Structure */

static pid_t child;
static char catSampInfo[256];
static char catDoneQ[256]; 

extern MSG_Q_ID pExpMsgQ;
extern MSG_Q_ID pRecvMsgQ;

PSG_Q_ENTRY psgQentry;
struct sample_info enterQentry;

static int ignoreResume = 0;
static int enterQempty = 0;

/****************************************************************
*
* getPsgQentry - get entry from psgQ
*
* RETURNS
*   NOENTRY or ENTRY_PRESENT
*
*			Author Greg Brissey 11-9-94
*/
int getPsgQentry(char *filename, PSG_Q_ENTRY *pPsgQentry)
{
    char eolch;
    char textline[256];
    FILE *stream;
    int stat;

    if (lockfile(filename) == ERROR)    /* lock file for exclusive use */
    {
        errLogRet(ErrLogOp,debugInfo,
                  "getPsgQentry: could not lock '%s' file\n",filename);
    }

    stream = fopen(filename,"r");

    if (stream == NULL)  /* does file exist? */
    {
        unlockfile(filename);   /* unlock file */
        DPRINT1(1,"psgQ file '%s' is empty.\n",filename);
        return(NOENTRY);
    }

    if (fscanf(stream,"%[^\n]%c", textline, &eolch) <= 0)
    {
        fclose(stream);
        unlockfile(filename);       /* remove lock file */
        DPRINT1(1,"getPsgQentry: 1st line in %s: fscanf read error\n",
                filename);
        return(ERROR);
    }
    else
    {
        char *sptr,*token;
        sptr = textline;

        DPRINT1(1,"getPsgQentry: 1st line in psgQ: '%s'\n",textline);
        token = strtok(sptr," ");
        strcpy(pPsgQentry->ExpIdStr,token);
        token = strtok(NULL,"\n");
        strcpy(pPsgQentry->ExpInfoStr,token);
        DPRINT2(2,"getPsgQentry: ExpIdStr: '%s', ExpInfoStr: '%s'\n",
		pPsgQentry->ExpIdStr,pPsgQentry->ExpInfoStr);
    }
    stat = read_sample_info(stream,&(pPsgQentry->SampleInfo));

    if ((stat == ENDOFFILE) || (stat == ERROR))
    {
        unlockfile(filename);   /* unlock file */
        fclose(stream);
        DPRINT1(1,"Autoproc: '%s' file is empty.\n",filename);
        return(NOENTRY);
    }

    fclose(stream);

    unlockfile(filename);       /* remove lock file */
    return(ENTRY_PRESENT);
}

/****************************************************************
*
* deletePsgQentry - delete entry from psgQ
*
* RETURNS
*   NOENTRY or ENTRY_PRESENT
*
*			Author Greg Brissey 11-9-94
*/
static void deletePsgQentry(char *filename,struct sample_info *s)
{
    char shellcmd[3*MAXPATHL];
    char tmp[91];
    int entryindex;

    if (lockfile(filename) == ERROR)    /* lock file for exusive use */
    {
        errLogRet(ErrLogOp,debugInfo,
                  "deletePsgQentry: could not lock '%s' file\n",filename);
    }
    /*
     * Sly way of determining psgQentry length.
     * We know 1 line for psg then sampleinfo size.
     */

    /* End-of-Entry index + 1 = size */
    get_sample_info(s,"EOE:",tmp,90,&entryindex);

    /*
     * 1 entry = (tail + (nlines+1))
     * nline = 1 (psg msge) + (index + 1) + 1
     */
    sprintf(shellcmd,"cd %s; tail -n +%d psgQ > psgQtmp; mv -f psgQtmp psgQ",
        autodir,entryindex+3);
  
    DPRINT1(1,"Autoproc shellcmd to delete psgQ entry:'%s'\n",shellcmd);
 
    system(shellcmd);           /* remove entry from psgQ sent to Acqproc */
 
    unlockfile(filename);       /* remove lock file */
}

/****************************************************************
*
* getEnterQentry - get entry from enterQ
*
* RETURNS
*   NOENTRY or ENTRY_PRESENT
*
*			Author Greg Brissey 11-9-94
*/
int getEnterQentry(char *enterfile, struct sample_info *pQentry)
{
    FILE *enter_file;
    int stat;

    if (lockfile(enterfile) == ERROR) /* lock file for exclusive use */
    {
        errLogRet(ErrLogOp,debugInfo,
                  "getEnterQentry: could not lock '%s' file\n",enterfile);
    }

    enter_file = fopen(enterfile,"r");
    if (enter_file == NULL)  /* does file exist? */
    {
        unlockfile(enterfile);   /* unlock file */
        errLogSysRet(ErrLogOp,debugInfo,
                     "getEnterQentry: '%s' file is not present.",enterfile);
        return(NOENTRY);
    }
    stat = read_sample_info(enter_file,pQentry);
    if ( (stat == ENDOFFILE) || (stat == ERROR) )
    {
        unlockfile(enterfile);   /* unlock file */
        if ( stat == ENDOFFILE )
        {
           DPRINT1(1,"enterQ file '%s' is empty.",enterfile);
        }
        else
        {
           errLogSysRet(ErrLogOp,debugInfo,
                        "getEnterQentry: Read Error on '%s' file ",enterfile);
        }
        fclose(enter_file);
        return(NOENTRY);
    }
    fclose(enter_file);

    unlockfile(enterfile);   /* unlock file */
 
    return(ENTRY_PRESENT);
}

#ifdef XXX
/****************************************************************
*
* deleteEnterQentry - delete entry from enterQ
*
* RETURNS
*   OK or ERROR
*
*			Author Greg Brissey 11-9-94
*/
void deleteEnterQentry(char *enterfile)
{
    char shellcmd[3*MAXPATHL];
    int entrylen;
    int tmp_euid;

    if (lockfile(enterfile) == ERROR) /* lock file for exclusive use */
    {
        errLogRet(ErrLogOp,debugInfo,
                  "deleteEnterQentry: could not lock '%s' file\n",enterfile);
    }

    entrylen = entryQ_length(autodir,enterfile);

/*
 *  sprintf(shellcmd,"tail +%d %s > /tmp/entertmp; mv -f /tmp/entertmp %s",
 *      N_SAMPLE_INFO_RECORDS+1,enterfile,enterfile);
 */

    sprintf(shellcmd,"tail +%d %s > /tmp/entertmp; mv -f /tmp/entertmp %s",
        entrylen+1,enterfile,enterfile);
    system(shellcmd);
 
    /* change the owner back to the orignal Owner & Group  */
    /* Since we just wrote it out as root */
    tmp_euid = geteuid();
    seteuid(getuid());   /* change the effective uid to root from vnmr1 */
    chown(enterfile, enter_uid, enter_gid);
    seteuid(tmp_euid);   /* change the effective uid back to vnmr1 */
 
    unlockfile(enterfile);   /* unlock file */
} 
#endif

/****************************************************************
*
* writeSampleInfo - write sample info out to sampleinfo file
*
* RETURNS
*   NOENTRY or ENTRY_PRESENT
*
*			Author Greg Brissey 11-9-94
*/
int writeSampleInfo(char* sampleinfo, struct sample_info *pQentry)
{
    FILE *sample_file;

    if (lockfile(sampleinfo) == ERROR) /* lock file for exusive use */
    {
        errLogRet(ErrLogOp,debugInfo,
                  "writeSampleInfo: could not lock '%s' file\n",sampleinfo);
    }
    sample_file = fopen(sampleinfo,"w");
    if (sample_file == NULL)  /* does file exist? */
    {
        unlockfile(sampleinfo);   /* unlock file */
        errLogSysRet(ErrLogOp,debugInfo,
                     "writeSampleInfo: '%s' file is not present ",sampleinfo);
        return(ERROR);
    }
 
    if ( write_sample_info(sample_file,pQentry) == ERROR)
    {
        unlockfile(sampleinfo);   /* unlock file */
        fclose(sample_file);
        errLogSysRet(ErrLogOp,debugInfo,
                     "writeSampleInfo: '%s' write error ",sampleinfo);
        return(ERROR);
    }
    fclose(sample_file);

    unlockfile(sampleinfo);   /* unlock file */

    return(OK);
}

/****************************************************************
*
* moveSampleinfoIntoDoneQ - write sample info into doneQ
*
* RETURNS
*   0 or -1 Error
*
*			Author Greg Brissey 11-9-94
*/
int moveSampleinfoIntoDoneQ(char *filename, struct sample_info *s_info)
{
    FILE  *sample_file;
 
    sample_file = fopen(filename,"a");
    if (sample_file == NULL)  /* does file exist? */
    {
        errLogSysRet(ErrLogOp, debugInfo,
	             "moveSampleinfoIntoDoneQ file: '%s'  does not exist",
                     filename);
        return(-1);
    }
 
    if ( write_sample_info(sample_file,s_info) == ERROR)
    {
        fclose(sample_file);
        errLogSysRet(ErrLogOp,debugInfo,
		"moveSampleinfoIntoDoneQ : '%s'  write error ",filename);
        return(-1);
    }
    fclose(sample_file);
    return(0);
}

/****************************************************************
*
* StartVnmr - forks and execl Vnmr in automation mode 
*
* RETURNS
*   fork Vnmr PID 
*
*			Author Greg Brissey 11-9-94
*/
pid_t StartVnmr()
{
    int ret;
 
    /*
     * Common Arguments for NORMAL or AUTOMATION Vnmr Arguments
     */  
    /*-u/big/usr2/greg/jaws/vnmruser*/

    /* sprintf(userdir,"-u%s",pQentry->udir_text); */

    /* USERDIR: is internally remapped if necessary */
    DPRINT(1,"StartVnmr: vnmrauto.\n");

    child = fork();  /* time to fork autoproc */
    if (child == 0)     /* if i am the child then exec autoproc */
    {
        sigset_t signalmask;
        struct passwd *pw;
        char execPath[MAXPATHL];

        /* set signal mask for child to zip */
        sigemptyset( &signalmask );
        sigprocmask( SIG_SETMASK, &signalmask, NULL );

        /*
         * Running as euid of Vnmr1, therefore must change to root to allow
         * us to the the gid and uid to the proper owner
         */
        seteuid(getuid());   /* change the effective uid to root from vnmr1 */

        /*
         * let Vnmr speak out, print output to console
         * since we do open /dev/console , you can't logout of the windowing
         * system while automation is running
         */
        freopen("/dev/null","r",stdin);
        freopen("/dev/console","a",stdout);
        freopen("/dev/console","a",stderr);

        /* change the user and group ID of the child so that VNMR
         *    will run with those ID's of the Owner of the EnterQ
         */
        if ( (pw = getpwuid(enter_uid)) )
        {
           setenv("HOME", pw->pw_dir, 1);
           setenv("USER", pw->pw_name, 1);
           DPRINT2(1,"set HOME to %s; USER to %s\n", pw->pw_dir, pw->pw_name);
        }
        if ( setgid(enter_gid) )
        {
            DPRINT(1,"BGprocess:  cannot set group ID\n");
        }

        if ( setuid(enter_uid) )
        {
            DPRINT(1,"BGprocess:  cannot set user ID\n");
        }
        DPRINT4(1,"forked Vnmr's uid: %d, euid: %d, gid: %d, egid: %d\n",
                getuid(),geteuid(),getgid(),getegid());

        /* automation go is done in Exp1 (e.g. -n1) */
/*
        ret = execl(vnmrpath,"Vnmr","-mauto","-hAutoproc","-i42",
                        "-n1",userdir,"auto_au",NULL);
 */
        strcpy(execPath,systemdir);
        strcat(execPath,"/bin/vnmrauto");
        DPRINT1(1,"execl(/bin/sh, sh, -c, %s, NULL)\n", execPath);
        ret = execl("/bin/sh","sh", "-c", execPath, NULL);
        errLogSysRet(ErrLogOp, debugInfo,
                     "StartVnmr: Vnmr could not execute. ");
        exit(1);
    }    
    return(child);
}        


static FILE *enterFilePtr = NULL;
void syncNextEnterQUtil(char *enterfile, int doneFile)
{
    if (doneFile) {
        if (enterFilePtr != NULL) {
            fclose(enterFilePtr);
            enterFilePtr = NULL;
        }
        return;
    }

    /* Does file exist? */
    enterFilePtr = fopen(enterfile,"r");
    if (enterFilePtr == NULL) {
        errLogSysRet(ErrLogOp, debugInfo,
                     "getNextEnterQEntry: '%s' file is not present.",
                     enterfile);
    }
}


int getNextEnterQEntry(char *enterfile,
                       struct sample_info *pQentry)
{
    int stat;

    if (enterFilePtr == NULL) {
        syncNextEnterQUtil(enterfile, 0 /* start using */ );
        if (enterFilePtr == NULL)
            return(NOENTRY);
    }

    stat = read_sample_info(enterFilePtr, pQentry);
    if ( (stat == ENDOFFILE) || (stat == ERROR) ) {
        if ( stat == ERROR ) {
           errLogSysRet(ErrLogOp,debugInfo,
                        "getNextEnterQEntry: Read Error on '%s' file ",
                        enterfile);
        }
        syncNextEnterQUtil(enterfile, 1 /* done using */);
        return(NOENTRY);
    }

    return(ENTRY_PRESENT);
}


static int enterQCount = 0;
int GetNewExpCount(char *enterfile)
{
    int count=0, stat, loc, oldLoc=-1;
    FILE *my_enter_file=NULL, *my_temp_file=NULL;
    char lastPath[MAXPATHL], tempPath[MAXPATHL];

    enterQCount = 0;

    strcpy(lastPath,systemdir);
    strcat(lastPath,"/asm/info/lastEnterQ");

    strcpy(tempPath,systemdir);
    strcat(tempPath,"/asm/info/tempEnterQ");

    /*
     * On Non-VAST/Hermes systems, this directory is not writable
     *   so if we can't open this file for writing, it's probably
     *   fine (only Hermes needs this functionality).
     */
    my_enter_file = fopen(lastPath, "r");
    if (my_enter_file == NULL) {
        DPRINT(1, "Unable to open lastEnterQ file for reading.\r\n");
    }

    my_temp_file = fopen(tempPath, "w");
    if (my_temp_file == NULL) {
        DPRINT(1, "Unable to open tempEnterQ file for writing.\r\n");
        if (my_enter_file != NULL)
            fclose(my_enter_file);
        return 0;
    }

    while (getNextEnterQEntry(enterfile, &enterQentry)
           == ENTRY_PRESENT) {
        char rack[10], zone[10], well[10];

        /* Read entry from current EnterQ */
        get_sample_info(&enterQentry,"RACK:",rack,8,NULL);
        if (*rack == '\0') loc = 1000000L;
        else loc = atoi(rack) * 1000000L;
        get_sample_info(&enterQentry,"ZONE:",zone,8,NULL);
        if (*zone == '\0') loc += 10000L;
        else loc += (atoi(zone) * 10000L);
        get_sample_info(&enterQentry,"SAMPLE#:",well,8,NULL);
        loc += atoi(well);

        if (count) {
            fprintf(my_temp_file, "%d\n", loc);
            enterQCount++;
            count++;
            continue;
        }

        /* Read entries from last enterQ until a match is found */
        if (my_enter_file != NULL) {
            oldLoc = -1;
            while (1) {
                if ((stat=fscanf(my_enter_file,"%d\n", &oldLoc)) == 1) {

                    if (oldLoc == loc) {
                        /* A match - MATCH, write out and keep looking */
                        break;
                    }

                    /* No match - deleted from study queue? */
                }
                else {
                    if (stat != EOF) {
                        DPRINT1(1, "-----------Error %s reading lastEnterQ\n",
                                strerror(errno));
                    }

                    count = 1;

                    break;
                }
            }
        }

        /* If no prior enter queue file, all entries are NEW */
        else {
            count = 1;
        }

        enterQCount++;
        fprintf(my_temp_file, "%d\n", loc);

    }

    if (my_enter_file != NULL)
        fclose(my_enter_file);
    fflush(my_temp_file);
    fclose(my_temp_file);

    /* Copy tempEnterQ contents into lastEnterQ */
    my_enter_file = fopen(lastPath, "w");
    my_temp_file = fopen(tempPath, "r");
    if (my_enter_file == NULL) {
        DPRINT1(1, "Unable to open %s for writing\n", lastPath);
        return 0;
    }
    if (my_temp_file == NULL) {
        DPRINT1(1, "Unable to open %s for reading\n", tempPath);
        return 0;
    }
    DPRINT1(1, "Ending %s Contents=\n", lastPath);
    while (fscanf(my_temp_file, "%d\n", &loc) != EOF) {
      fprintf(my_enter_file, "%d\n", loc);
    }
    fclose(my_enter_file);
    fclose(my_temp_file);

    return count;
}

int NotifyRoboNewExp(char *samplepath)
{
    int i, stat, retVal=0, newCount, sampno=0, oldsampno=0;

    /* Lock file for exclusive use */
    if (lockfile(samplepath) == ERROR) {
        errLogRet(ErrLogOp, debugInfo,
                  "NotifyRoboNewExp: could not lock '%s' file\n",
                  samplepath);
        return 0;
    }

    newCount = GetNewExpCount(samplepath);
    syncNextEnterQUtil(samplepath, 1);

    if (newCount > 0) {

        retVal = newCount;

        /* Skip the Old Entries */
        for (i=0; i<(enterQCount-newCount); i++) {
            stat = getNextEnterQEntry(samplepath, &enterQentry);

            if (stat != ENTRY_PRESENT) {
                DPRINT(0, "NotifyRoboNewExp: error retrieving new "
                          "experiments from enterQ");
                syncNextEnterQUtil(samplepath, 1);
                unlockfile(samplepath);
                return 0;
            }
        }

        /* Last Entries in the Queue are the New Ones - Notify Roboproc */
        for ( ; i<enterQCount; i++) {
            char parm[64];
            char rack[10], zone[10], well[10], exptime[10], pcstime[10];
            int iRack, iZone, iWell, iExpTime, iPcsTime;

            stat = getNextEnterQEntry(samplepath, &enterQentry);

            if (stat != ENTRY_PRESENT) {
                DPRINT(0, "NotifyRoboNewExp: error retrieving new "
                        "experiments from enterQ");
                syncNextEnterQUtil(samplepath, 1);
                unlockfile(samplepath);
                return (enterQCount-i);
            }

            /*
             * Queue this sample with Roboproc immediately (via Expproc).
             *
             * Only Hermes Sample Changer needs to queue the samples
             *  prior to them being requested by the console.
             *
             * But since Autoproc doesn't know what sample changer
             *  is installed, we'll at least limit propogation of
             *  this message to experiments that have "EXPTIME"
             *  defined in their experiment parameters.
             *
             * In any case, Roboproc will ignore this message for any
             *  sample changer other than Hermes.
             */
          
            get_sample_info(&enterQentry,"EXPTIME:",exptime,8,NULL);
            if (*exptime != '\0')
            {
                /* Aargh, xmsubmit (vnmrJ) appends '#' but enter does not */
                get_sample_info(&enterQentry,"RACK:",rack,8,NULL);
                if (*rack == '\0')
                    get_sample_info(&enterQentry,"RACK#:",rack,8,NULL);

                get_sample_info(&enterQentry,"ZONE:",zone,8,NULL);
                if (*zone == '\0')
                    get_sample_info(&enterQentry,"ZONE#:",zone,8,NULL);

                get_sample_info(&enterQentry,"SAMPLE#:",well,8,NULL);

                iRack = atoi(rack);
                iZone = atoi(zone);
                iWell = atoi(well);

                sampno = iRack * 1000000L + iZone * 10000L + iWell;
                if (sampno != oldsampno)
                {
                    oldsampno = sampno;
                    get_sample_info(&enterQentry,"CONDITION:",pcstime,8,NULL);
                    iExpTime = atoi(exptime);
                    if (*pcstime == '\0')
                        iPcsTime = 0;
                    else
                        iPcsTime = atoi(pcstime);

                    sprintf(parm, "queueSample %d %d %d %d %d\n",
                            iRack, iZone, iWell, iExpTime, iPcsTime);
                    usleep(100000);  /* sleep 100 ms */
                    DPRINT1(1,"AUTOPROC SENDING \'%s\' TO EXPPROC\n", parm);
                    sendMsgQ(pExpMsgQ,parm,strlen(parm),MSGQ_NORMAL,WAIT_FOREVER);
                }
            }
        }
    }

    syncNextEnterQUtil(samplepath, 1);
    unlockfile(samplepath);
    return retVal;
}

/**************************************************************
 * Sample Id (barcode) for current sample has been read by
 * by Roboproc.  Save in study queue directory.
 **************************************************************/
int SampleId(char *arg)
{
    struct sample_info entry;
    char *barcode, tmp[64];
    FILE *stream;
    int stat, idx;

    barcode = strtok(NULL," ");
    DPRINT1(0, "Sample Barcode Read: %s\n", barcode);

    if (lockfile(samplepath) == ERROR)    /* lock file for exclusive use */
    {
        errLogRet(ErrLogOp, debugInfo,
                  "SampleId: could not lock '%s' file\n", samplepath);
    }

    if ((stream = fopen(samplepath,"r")) == NULL)  /* does file exist? */
    {
        unlockfile(samplepath);   /* unlock file */
        DPRINT1(0, "Autoproc: sampleinfo file '%s' is empty.\n", samplepath);
        return (0);
    }

    stat = read_sample_info(stream, &entry);

    if ((stat == ENDOFFILE) || (stat == ERROR))
    {
        unlockfile(samplepath);   /* unlock file */
        fclose(stream);
        DPRINT1(0, "Autoproc: '%s' file is empty.\n", samplepath);
        return (0);
    }

    get_sample_info(&entry, "MACRO:", tmp, 64, &idx);

    {
        char *ptr1, *ptr2, path[256];

        ptr1 = strstr(tmp, "startq('");
        if (ptr1 != NULL) {
            ptr2 = strstr(ptr1+8, "')");
            if (ptr2 != NULL) {
                int len = ptr2 - (ptr1+8);
                FILE *bcFile;

                memset(path, 0, sizeof(path));
                sprintf(path, "%s/", autodir);
                strncat(path, ptr1+8, len);
                strncat(path,"/barcode", 8);
                DPRINT1(0, "Study Queue File = %s\n", path);
                bcFile = fopen(path, "w");
                if (bcFile != NULL) {
                    int tmp_euid;
                    extern uid_t enter_uid;
                    extern gid_t enter_gid;

                    fprintf(bcFile, "%s\n", barcode);
                    fclose(bcFile);

                    /* change the owner back to the orignal Owner & Group  */
                    /* Since we just wrote it out as root */
                    tmp_euid = geteuid();
                    seteuid(getuid());
                    chown(path, enter_uid, enter_gid);
                    seteuid(tmp_euid);
                }
            }
        }
    }

    fclose(stream);
    unlockfile(samplepath);       /* remove lock file */
    return(0);
}



/**************************************************************
*
*  Resume -  start next Experiment from enter Queue
*
*					Updated: 10-4-95
*					Updated: 12-1-03 dsillman
*  Actions of Resume:
*       0. Determine if new experiments have been added since
*            the last time we ran.  If so, notify roboproc
*            if appropriate ( if we're Hermes ).
*	1. If an Experiment is activitely acquiring, NO Action
*       2. If the ExpQ has entries, then No Action
*           ( I.E. run queued non-automation Exp before Starting 
*		   automation. And never queue more than one automation
*		   experiment at a time)
*	3. Check psgQ, if entry present read both the Exp Queue info and
*	    the sampleinfo from the psgQ. 
*	   a. copy the sample info file into the doneQ
*	   b. if the Exp ID String is not "NOGO" then place 
*	      the Exp. entry into the Exp Queue (to start Exp) and
*	      send msge to Expproc to check it's Q
*	   c. delete entry from psgQ
*	4. Otherwise if no (Autoproc forked) Vnmr running check enterQ, 
*	   if entry present then:
*	   a. Write the enterQ entry (i.e. sampleinfo) to the sampleinfo
*	      file located in 'autodir/exp1/sampleinfo'
*	      (go & psg expect this file here)
*	   b. Fork Vnmr (with sampleinfo arg, gets the users directory from this)
*	        The Vnmr runs auto_au macro.
*	      retain Vnmr's PID for future reference
*	   c. delete entry from EnterQ
*	     NOTE: at this point the forked Vnmr will produce a psgQ entry
*	           This entry will either contain a valid Exp. to run or
*		   have the key word "NOGO" as the Exp. ID string indecating
*		   that no go was performed.
*		   When this Vnmr exits the GrimReaper will call Resume()
*		   
*	5. If there was no entry in the EnterQ then check the following to
*	   determine if automation has completed:
*	    1. already know at this point that:
*	      a.  no entries in enterQ, no entries in psgQ
*	    2. check for no entries queued or active in processing
*	    3. No messages pending	
*	  Thus: Automation is complete when:
*        	no active Experiment, no active or queued processing,
*	 	no entries in enterQ, no entries in psgQ
*		no queued messages for Autoproc
*
* RETURNS:
* 0 , else -1
*
*       Author Greg Brissey 9/6/94
*/
int Resume(char *argstr)
{
   struct stat statbuf;

   sprintf(catSampInfo,"cat %s",samplepath);
   sprintf(catDoneQ,"cat %s",doneQpath);
   
   /* Is there an actively acquiring Experiment */
   DPRINT(1,"Resume Invoked //////////////////// \n");

   if (VnmrPid == 0)
   {
       int newcount = NotifyRoboNewExp(enterpath);
       (void) newcount;
       DPRINT1(1,"New Exp Added?  %d\n", newcount);
   }

   DPRINT1(1,"Active Exp?: %d\n",activeExpQentries());
   if (activeExpQentries() > 0)
   {
     DPRINT(1,"Experiment already being acquired, Resume skipped\n");
     return(0);			/* Yes, no action */
   }

   /* Is there any queued Experiments */
   DPRINT1(1,"Queued Exp?: %d\n",expQentries());
   if (expQentries() > 0)
   {
     DPRINT(1,"Experiment already queued, Resume skipped\n");
     return(0);			/* Yes, no action */
   }

   if (ignoreResume != 0)
   {
      DPRINT(1,"Ignoring Resume\n");
      if (VnmrPid)
      {
         DPRINT(1,"Vnmr still active. Done with Resume ///////////////////  \n");
         return(0);
      }
      DPRINT3(1,"Terminate? - BG processing Queued: %d, Active: %d, Pending Msges: %d\n",
		procQentries(), activeProcQentries(), msgesInMsgQ(pRecvMsgQ));
      if ( (activeProcQentries() < 1) && (procQentries() < 1) && 
	   (msgesInMsgQ(pRecvMsgQ) == 0)  )
      {
	 /* tell Expproc to tell Roboproc to clear the sample changer */
         DPRINT(1,"AUTOPROC SENDING \'roboclear\' TO EXPPROC\n");
         sendMsgQ(pExpMsgQ, "roboclear", strlen("roboclear"),
                  MSGQ_NORMAL, WAIT_FOREVER);

	 /* request to Expproc to terminate, Expproc sends Ok2Die */
         sendMsgQ(pExpMsgQ,"autoRq2Die",strlen("autoRq2Die"),
			MSGQ_NORMAL,WAIT_FOREVER); 
         fprintf(stdout,"ignoreResume=1,Autoproc Sent request to End of Automation to Expproc\n");
   	 DPRINT(1,"ignoreResume=1,Autoproc Sent request to End of Automation to Expproc\n");
      }
      return(0);
   }

   if (getPsgQentry(psgQpath, &psgQentry) == ENTRY_PRESENT)
   {

#ifdef DEBUG
      DPRINT(1,"Process PSG Q entry\n");
      DPRINT2(1,"ExpIdStr: '%s', ExpInfoStr: '%s'\n",
			psgQentry.ExpIdStr, psgQentry.ExpInfoStr);
      DPRINT(1,"SampleInfo from psgQ <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< \n");
      if (DebugLevel)
      {
        write_sample_info(stdout,&psgQentry.SampleInfo);   /* diagnostic output */
      }
#endif

      /* copy the sampleinfo (from psgQ) to the DoneQ 
	psg updates the status & data text fields then puts it into psgQ
        if psg not executed (i.e. no go) then the Vnmr 'auto_au' macro 
	generated the psgQ entry.
      */
      moveSampleinfoIntoDoneQ(doneQpath,&(psgQentry.SampleInfo));

#ifdef DEBUG
      DPRINT(1,"doneQ  <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< \n");
      if (DebugLevel)
      {
        system(catDoneQ);
      }
#endif

      /* Id String is NOGO then a go/au was not executed for this experiment */
      if ( strcmp(psgQentry.ExpIdStr,"NOGO") != 0)
      {
        /* Add psgQ entry to Exp Q tell Expproc to check its Q */
        expQaddToTail(NORMALPRIO, psgQentry.ExpIdStr, psgQentry.ExpInfoStr);
        sendMsgQ(pExpMsgQ,"chkExpQ",strlen("chkExpQ"),MSGQ_NORMAL,WAIT_FOREVER); 
      }
      else
      {
	/* send a resume to ourselves otherwise Autoproc might stop and never
	   get restarted (no more resumes)
        */
        sendMsgQ(pRecvMsgQ,"resume",strlen("resume"),MSGQ_NORMAL,WAIT_FOREVER); 
      }
      deletePsgQentry(psgQpath,&(psgQentry.SampleInfo));
   }
   else if ( (VnmrPid == 0) && (enterQempty) )
   {
      /* if no more active Experiment, no more active or queued processing
         no more EnterQ, no more psgQ , no pending messages -> automation DONE, exit */
      DPRINT3(1,"Done? - BG processing Queued: %d, Active: %d, Pending Msges: %d\n",
              procQentries(), activeProcQentries(), msgesInMsgQ(pRecvMsgQ));
      if ( (activeProcQentries() < 1) && (procQentries() < 1) &&
           (msgesInMsgQ(pRecvMsgQ) == 0)  )
      {
         /* tell Expproc to tell Roboproc to clear the sample changer */
         DPRINT(1,"AUTOPROC SENDING \'roboclear\' TO EXPPROC\n");
         sendMsgQ(pExpMsgQ, "roboclear", strlen("roboclear"), MSGQ_NORMAL, WAIT_FOREVER);

         /* request to Expproc to terminate, Expproc sends Ok2Die */
         sendMsgQ(pExpMsgQ,"autoRq2Die",strlen("autoRq2Die"),MSGQ_NORMAL,WAIT_FOREVER);
         DPRINT(1,"Autoproc Sent request to End of Automation to Expproc\n");
      }
      enterQempty = 0;
   }
   else if (VnmrPid == 0)	/* If Vnmr running don't start one */
   {
      char lastPath[MAXPATHL];
      /* read next enterQ entry */
      DPRINT(1,"No psgQ Entry & No Vnmr Running, Run Vnmr\n");

      VnmrPid = StartVnmr();    /* fork Vnmr  */

      strcpy(lastPath,systemdir);
      strcat(lastPath,"/asm/info/lastEnterQ");

      if (stat(lastPath, &statbuf) == 0)
      {
          char shellcmd[2*strlen(lastPath)+100];
          sprintf(shellcmd, "tail -n +2 %s > /tmp/entertmp; "
                            "mv -f /tmp/entertmp %s",
                     lastPath, lastPath);
          system(shellcmd);
      }
   }
   DPRINT(1,"Done with Resume ///////////////////  \n");
   return(0);			/* Yes, no action */
}

void ShutDownProc()
{
   DPRINT(1,"ShutDownProc: \n");
   if (pExpMsgQ != NULL)
      closeMsgQ(pExpMsgQ);
   shutdownComm();
   expQRelease();
   activeExpQRelease();
   procQRelease();
   activeQRelease();
}


/*********************************************************************
*
* Ok2Die - Procedure and terminate your self
*
*/
int Ok2Die( char *arg )
{
    char fname[256];
    FILE *queueFile=NULL;

    /* Clear copy of lastEnterQ - enter queue is now empty */
    sprintf(fname, "%s/asm/info/lastEnterQ", getenv("vnmrsystem"));

    /*
     * On Non-VAST/Hermes systems, this directory is not writable
     *   so if we can't open this file for writing, it's probably
     *   fine (only Hermes needs this functionality).
     */
    if ((queueFile = fopen(fname, "w")) == NULL) {
        DPRINT1(1, "Ok2Die: unable to clear lastEnterQ file (%s)\r\n",
                fname);
    }
    else
        fclose(queueFile);

    ShutDownProc();
    fprintf(stdout,"End of Automation\n");
    DPRINT(1,"End of Automation\n");
    exit(0);
    return 0;
}

/*********************************************************************
*
* markDoneQcmplt - mark the doneQ status field to Error or Complete
*
*      After Procproc BG processing of Experiment "cmplt 'completion type'" 
*      command is sent to Autoproc, this routine receives the 'completion type'
*      and masks the doneQ entry accordingly.
*
*/
int markDoneQcmplt( char *arg )
{
    char *token;
    char data_key[1024];
    char buf[1024];
    int  cmpltType;
    char *keyptr;
    char *asmStripAutodir_dotFid();

    token = strtok(NULL," ");
    cmpltType = atoi(token);
    token = strtok(NULL," ");  /* autodir + autoname + '.fid' */
    strcpy(buf,token);

    DPRINT2(1,"markDoneQcmplt: completion type: '%s', data_field: '%s'\n",
        ((cmpltType == 0) ? "Complete" : "Error"),buf);



    /* strip out the autodir path and the '.fid' extension */
    keyptr = asmStripAutodir_dotFid(autodir,buf);
    strcpy(data_key,keyptr);


#ifdef DEBUG
    if (DebugLevel)
    {
        DPRINT(1,"markDoneQcmplt: doneQ  Prior to update "
                 "<<<<<<<<<<<<<<<<<<<<<<<<<< \n");
        system(catDoneQ);
    }
#endif

    DPRINT2(1,"markDoneQcmplt: autodir: '%s', data_key: '%s'\n",
            autodir,data_key);
    update_sample_info(autodir, "doneQ",
                       "DATA:", data_key,
                       "STATUS:", ((cmpltType == 0) ? "Complete" : "Error"));


#ifdef DEBUG
    if (DebugLevel)
    {
        DPRINT(1,"markDoneQcmplt: doneQ  after update "
                 "<<<<<<<<<<<<<<<<<<<<<<<<<<<<< \n");
        system(catDoneQ);
    }
#endif

    return 0;
}

/*********************************************************************
*
* Ignore - Ignore any resume sent from this point on 
*
*      The ignore message is received upon an 'autosa'
*      Thus all future resumes are ignore but the cmplt messages
*       will still be processed.
*
*/
int Ignore( char *arg )
{
    if ( ! strcmp(arg,"1"))
       enterQempty = 1;
    else
       ignoreResume = 1;
    DPRINT2(1,"Ignore: ignoreResume = %d enterQempty= %d \n",
            ignoreResume,enterQempty);
    return 0;
}

/*********************************************************************
*
* Listen - Listen any resume sent from this point on 
*
*      The listen message is received upon an 'autora'
*      Thus all future resumes are processed
*
*/
int Listen( char *arg )
{
    enterQempty = ignoreResume = 0;
    DPRINT1(1,"Listen: ignoreResume = %d \n",ignoreResume);
    return 0;
}

int resetAutoproc()
{
    DPRINT(1,"Reset: Clear Qs and Kill any BG processing.\n");
    return 0;
}


int terminate(char *str)
{
    DPRINT(1,"terminate: \n");
    closeMsgQ(pExpMsgQ);
    shutdownComm();
    expQRelease();
    activeExpQRelease();
    procQRelease();
    activeQRelease();
    exit(0);
    return 0;
}

int debugLevel(char *str)
{
    extern int DebugLevel;
    char *value;
    int  val;
    value = strtok(NULL," ");
    val = atoi(value);
    DPRINT1(0,"debugLevel: New DebugLevel: %d\n",val);
    DebugLevel = val;
    return(0);
}

/*--------------------------------------------------------------------
| getUserUid()
|       get the user's  uid & gid outof the passwd file
+-------------------------------------------------------------------*/
int getUserUid(char *user, int *uid, int *gid)
{
    struct passwd *pswdptr;

    if ( (pswdptr = getpwnam(user)) == ((struct passwd *) -1) )
    {
        *uid = *gid = -1;
        return(-1);
    }   
    *uid = pswdptr->pw_uid;
    *gid = pswdptr->pw_gid;
    DPRINT3(1,"getUserUid: user: '%s', uid = %d, gid = %d\n", user,*uid,*gid);
    return(0);
}
