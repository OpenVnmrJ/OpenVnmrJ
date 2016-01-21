/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <pwd.h>
#include <sys/time.h>
#include "data.h"
#include "part11.h"
#include "locksys.h"
#include "allocate.h"
#include "buttons.h"
#include "pvars.h"
#include "wjunk.h"

#define NONFDA 0
#define FDA 1

FILE    *sessionAuditTrailFp = NULL;
FILE    *cmdHistoryFp = NULL;
FILE    *dataAuditTrailFp = NULL;

int part11System = NONFDA;
/* part11System >= 0 if part11Config exists and is successfully */
/* parsed, and sysOwnedSafecp (systemdir/p11/bin/safecp) also exists. */
/* part11System is a global variable checked before execute any part11 */
/* function (except for p11_init()) */

int nuserPart11Dirs = 0;
string userPart11Dirs[MAXWORDS];
string auditDir = "";
string sysFDAdir = "";
string part11RootPath = "";
string incPath = "";
string winCmd = "";
int incInc = 0;
string currentPart11Path = "";
part11_standard_files* part11_std_files = NULL;
part11_additional_files* part11_add_files = NULL;
extern int specIndex;
extern int mode_of_vnmr;
int okFDAdata = 0;
int showDataID = 1;
int showLink = 0;

extern void check_datastation();
extern void setGlobalPars();
extern int checkChecksums(char* rtpath, char* name, FILE* flagfp, char* t);
extern int rm_file(char* path);
extern int verify_copy(char *file_a, char *file_b );
extern int strStartsWith(char *s1, char *s2);
extern int strEndsWith(char *s1, char *s2);
extern int isAdirectory(char* filename);
extern int flush( int argc, char *argv[], int retc, char *retv[] );
extern int cp_file(char* path1, char* path2);
extern int setSafecpPath(char* safecpPath, char* dest);
extern int makeautoname(char *cmdname, char *a_name, char *sif_name, char *dirname,
                 int createflag, int replaceSpaceFlag,
                 char *suffix, char *notsuffix);
extern void currentDate(char* cstr, int len );
extern int brkPath(char* path, char* root, char* name);
extern int fix_automount_dir(char *input, char *output );
extern int disp_current_seq();
extern void appendTopPanelParam();
extern void set_vnmrj_rt_params(int do_call );
extern int text(int argc, char *argv[], int retc, char *retv[]);
extern int getChecksum(char* filename, char* sum);
extern int checkPart11Checksums(char* dirpath, FILE* outfp);
extern int checkPart11Checksums0(char* rootpath, FILE* outfp);
extern int fileExist(char* path);

#ifdef VNMRJ
extern int VnmrJViewId;
#endif 

string operatorID="";
string operatorFullName="";
string softwareVersion="";
int nlogin = 0;
static string auditTrail = "";
static string cmdHisFile = "";
static string p11console = "";
static string cmdTime = "";
static string currentCmd = "";
static string usrOwnedSafecp = "/vnmr/bin/safecp";
static string sysOwnedSafecp = "/vnmr/p11/bin/safecp";
static char p11Id[MAXSTR+1];
static int paramChanged = 0;
static int dataNewCmd = 0;
static int disNewCmd = 0;
static int paramNewCmd = 0;
static int debug = 0;

static char markerString[100] = "part11.c Marker: 080716";

string bootupTime;

static string recordAuditMenu = "/persistence/RecordAuditMenu";
static string systemAuditMenu = "/persistence/SystemAuditMenu";
static string cmdHistoryMenu = "/persistence/CmdHistoryMenu";
static string auditTableSelection = "/persistence/auditTableSelection";
static string sessionAudit = "/persistence/SessionAudit.vaudit";
static string userConfig = "/templates/vnmrj/properties/recConfig";

void createPart11globalpars();
void createPart11currentpars();
int p11_saveAuditTrail();
int p11_saveFDAfiles_processed(char* func, char* orig, char* dest);
int p11_isPart11Dir(string str);
int makeChksums(char* path, char *checksumFile, string* files, int nfiles);
int saveCurpar(char* path);
int readUserPart11Config();
int writeAuditTrails(char* func, char* origpath, char* destpath, char* subdir);
int isPart11Data();
int isUserOwned(string path);

static void getStrValues(char* buf, string* words, int* n, char* delimiter);
static void getFilenames(string rootpath, string name, string* files, int* n, int nmax);
static int file_is_link(char* filepath, char* newpath, char* curfid_linkpath);
static int safecp_file(char* safecpPath, char* origpath, char* destpath);
static int copytext(char *frompath, char *topath);
static void saveallshims(int rtsflag);
static int saveshim(const char* vname, int rtsflag);
void setDataID(string fidid, int processid);


static void p11TempName(char *buf)
{
    struct timeval clock;

    gettimeofday(&clock, NULL);
    sprintf(buf,"/tmp/p11_%ld_%06ld", clock.tv_sec, clock.tv_usec);
}

/******************/
int getNumOfLines(char* path)
/******************/
{
    FILE* fp;
    int i;
    char  buf[BUFSIZE];

    char test[100];
    strcpy(test, markerString);

    if((fp = fopen(path, "r"))) {

  	i = 0;
        while (fgets(buf,sizeof(buf),fp)) i++; 

    } else i = -1;

    if(fp)
        fclose(fp);

    if(debug)
	fprintf(stderr,"getNumOfLines %d\n", i);
    return(i);
}

// checkPart11Checksums(...) to check .RECs
// checkPart11Checksums0(...) to check a directory containing checksum file
// checkChecksums(...) to check any given checksum file.
/**************************/
int fidCheckedOk()
/**************************/
{
/* -2 fid not exists, -1 fid exists, but fidid not exists (old data), */
/* 1 fid exists, and checksum (of saved REC) or fidid ( of not saved data) matchs */
/* 0 fid exists, but checksum or fidid does not match. */ 

    int i, r, linked;
    string path, str, curfid_linkpath;
    char *ptr;

    sprintf(path,"%s%s",curexpdir,"/acqfil/fid");

    if(!fileExist(path)) {
	/* fid does not exist in curexp. */
	i = -2;
    } else if (okFDAdata == 1) {
        i = 1;
    } else if (isSymLink(path) == 0) {
        /* fid is linked */
        linked = file_is_link(path, "", curfid_linkpath );

	ptr=curfid_linkpath;
        if(linked && strstr(ptr+strlen(curfid_linkpath)-3,"fid") != NULL) {
            strcpy(str, "");
            strncat(str, curfid_linkpath, strlen(curfid_linkpath)-3);

            if(debug)fprintf(stderr,"fidCheckOk linkpath %s\n", str);

            if(strstr(str, ".REC") != NULL) r = checkPart11Checksums(str, NULL);
	    else r = checkPart11Checksums0(str, NULL);
            if(r == -1) i = -1;
            else if(r == 0) i = 1;
            else i = 0;

        } else i = -1;
    } else { // acqfil/fid not linked
/* rt_FDA always make link to fid if UserName matches, otherwise makes copy*/

	// assume file parameter is a path to data
        if(P_getstring(CURRENT, "file", str, 1, MAXSTR)) strcpy(str,"");
        ptr=str;
	if(!strrchr(ptr+strlen(str)-4,'.')) strcat(str,".fid");

	if(strstr(ptr+strlen(str)-4,".REC") != NULL || strstr(ptr+strlen(str)-4,".rec") != NULL) {
	    strcat(str,"/acqfil");
	}
	
        r = checkPart11Checksums0(str, NULL);
        if(r == -1) i = -1;
        else if(r == 0) i = 1;
        else i = 0;
    }

    okFDAdata = i;
    return(i);
}

/**************************/
int isFDAmode()
/**************************/
{
// return 1 if "fidOK", 0 corrupted or not p11 system. 
// return 2 if "fidOK" and is REC data.
    int i, linked;
    char dir[MAXSTR], curfid_linkpath[MAXSTR];
    string str;
 
    if(!part11System) {
        i = 0;
    } else {

	i = fidCheckedOk();

        sprintf(dir,"%s%s",curexpdir,"/acqfil/fid");
        if (isSymLink(dir) == 0) { // fid is linked 

            linked = file_is_link(dir, "", curfid_linkpath );
            if(i == 1 && linked && strstr(curfid_linkpath, ".REC")) i = 2;
	    else if(i != 1) i = 0;

        } else { // fid is not linked

            if(P_getstring(CURRENT, "file", str, 1, MAXSTR)) strcpy(str,"");
	    if(i == 1 && strstr(str,".REC") != NULL) i = 2;
	    else if(i != 1) i = 0;
        }
    }
  
    return(i);
}

/******************/
int updateLinkIcon()
/******************/
{
    int i = 0, linked;
    char dir[MAXSTR], curfid_linkpath[MAXSTR];
    string str;

    if(!part11System || !showLink) return(0);

    i = fidCheckedOk(); 

#ifdef VNMRJ

    sprintf(dir,"%s%s",curexpdir,"/acqfil/fid");

    if(!fileExist(dir)) {
	/* fid does not exist. */
	i = -2;
	writelineToVnmrJ("vnmrjcmd", "p11link none");
    } else if (isSymLink(dir) == 0) {
        /* fid is linked */
        linked = file_is_link(dir, "", curfid_linkpath );

        if(linked && strstr(curfid_linkpath, ".REC") != NULL &&
           i == 1) {
            sprintf(str,"p11link blueLink %s", curfid_linkpath); 
            writelineToVnmrJ("vnmrjcmd", str);
        } else if(i != 0) {
            sprintf(str,"p11link yellowLink %s", curfid_linkpath); 
            writelineToVnmrJ("vnmrjcmd", str);
        } else {
            sprintf(str,"p11link redLink %s", curfid_linkpath); 
            writelineToVnmrJ("vnmrjcmd", str);
        }

    } else {
	/* fid is not linked, but exist */
	if(i == 1) 
            writelineToVnmrJ("vnmrjcmd", "p11link blueNoLink");
	else if(i != 0) 
            writelineToVnmrJ("vnmrjcmd", "p11link yellowNoLink");
	else
            writelineToVnmrJ("vnmrjcmd", "p11link redNoLink");
    }
  
#endif 

    if(debug)fprintf(stderr,"updateLinkIcon %d\n", i);
    return(i);
}

int updateSvfdirIconNew()
{
    string dir, str;
    if(P_getstring(GLOBAL,"svfdir",dir,1, MAXSTR)) strcpy(dir,"");
    if(part11System) {
	sprintf(str,"p11svfdir blue %s", dir); 
	writelineToVnmrJ("vnmrjcmd", str);
	return(1);
    } else {
	sprintf(str,"p11svfdir yellow %s", dir); 
	writelineToVnmrJ("vnmrjcmd", str);
	return(0);
    }
}

/******************/
int updateSvfdirIcon()
/******************/
{
    int i;
    string dir, str;

    if(!part11System) return(-1);

#ifdef VNMRJ
    if(P_getstring(GLOBAL,"svfdir",dir,1, MAXSTR)) strcpy(dir,"");
   
    if(strlen(dir) == 0) i = -1;
    else i = p11_isPart11Dir(dir);

    if(i == -1) {
	writelineToVnmrJ("vnmrjcmd", "p11svfdir none");
    } else if(i == 1) {
	sprintf(str,"p11svfdir blue %s", dir); 
	writelineToVnmrJ("vnmrjcmd", str);
    } else {
	sprintf(str,"p11svfdir yellow %s", dir); 
	writelineToVnmrJ("vnmrjcmd", str);
    }
#endif 

    if(debug)fprintf(stderr,"updateSavrDirIcon %d\n", i);
    return(i);
}

/******************/
int updateCheckIcon()
/******************/
{
    int i;

    if(!part11System) return(-1);

#ifdef VNMRJ
    i = fidCheckedOk();
    if(i == -2) {
	writelineToVnmrJ("vnmrjcmd", "p11check none");
    } else if(i == -1) {
	writelineToVnmrJ("vnmrjcmd", "p11check yellow");
    } else if(i == 1) {
	writelineToVnmrJ("vnmrjcmd", "p11check blue");
    } else {
	writelineToVnmrJ("vnmrjcmd", "p11check red");
    } 

#endif 

    if(debug)fprintf(stderr,"updateCheckIcon %d\n", i);
    return(i);
}

/******************/
int updateFDAMode()
/******************/
{
    int i;

    if(!part11System) return (-1);

    i = isFDAmode();

#ifdef VNMRJ
    if(i == 2) writelineToVnmrJ("vnmrjcmd", "FDA on");
    else writelineToVnmrJ("vnmrjcmd","FDA off");
#endif 

    return(i);
}

/******************/
int p11_saveUserMacro(char* name)
/******************/
{
    string macroPath;
    double d;
    string str;

    if(strstr(currentCmd,"recordOff") != NULL) RETURN;

    if(!part11System && !P_getreal(GLOBAL, "recordSave", &d, 1) && d < 0.5) RETURN;

    if(P_getstring(GLOBAL, "acqmode", str, 1, MAXSTR)) strcpy(str, "");
    if(strcmp(str, "fidscan") == 0 || strcmp(str, "lock") == 0) RETURN;

    if(strstr(name, userdir) == NULL ||
       strstr(name, "jFunc") != NULL ) RETURN;
  
    if(debug)
	fprintf(stderr,"UserMacro %s\n", name);

    if(strlen(currentCmd) > 0 && strstr(name, currentCmd) != NULL)
        strcpy(currentCmd, name);

    /* copy macro only if path is not curexpdir/maclib */

    if(strstr(name, curexpdir) == NULL) {
        sprintf(macroPath, "%s%s", curexpdir, "/maclib");

        if(!fileExist(macroPath)) mkdir(macroPath, 0755);

        cp_file(name, macroPath);
    }

    if(cmdHistoryFp != NULL) 
	fprintf(cmdHistoryFp,"            userMacro %s\n", name);

    if(sessionAuditTrailFp != NULL) 
	fprintf(sessionAuditTrailFp,"           userMacro %s\n", name);

    RETURN;
}

/******************/
void p11_writeCmdHistory(char* buf)
/******************/
{
/* called by handler.c before cmd is sent to loadAndExec */ 

    int i;
    char mi[4];
    char* strptr;
    double d;
    char str[1024];

    strcpy(str, buf);

    /* don't save jFunc, vnmrjcmd, and empty lines to cmdHistory */

    if(debug)
	fprintf(stderr, "p11_writeCmdHistory: %d %s", strlen(str), str);

    if(strlen(str) == 0 || strcmp(str, "\n") == 0 ||
       (strncmp(str, "jFunc", 5) == 0 && strncmp(str, "jFunc(7,", 8) != 0) || 
       strstr(str, "jEvent") != NULL ||
       strstr(str, "isimagebrowser") != NULL ||
       strstr(str, "jRegion") != NULL ||
       strstr(str, "jMove") != NULL ||
       strstr(str, "repaint") != NULL ||
       strstr(str, "trackCursor") != NULL ||
       strstr(str, "vnmrjcmd") != NULL ||
       strstr(str, "spacedata") != NULL ||
       strstr(str, "spacerecord") != NULL ||
       strncmp(str, "setshim", 7) == 0 ) {
/*
  strcpy(currentCmd, "");
*/
	return;
    }

    /* p11_saveFDAfiles_processed proceeds only if dataNewCmd true */
    /* it will set dataNewCmd = 0 to avoid mutiple executions */
    /* by processing function(s) for a given cmd */
    dataNewCmd = 1;
    /* the same for executing p11_saveDisCmd */
    disNewCmd = 1;
    paramNewCmd = 1;

    /* display menu cmd */
    if(strncmp(str, "jFunc(7,", 8) == 0) {

        i = strcspn(str, ")") - 1;
        strptr = str+8;
        strcpy(mi, "");
        strncat(mi, strptr, i-7);
        i = atoi(mi);

	P_getstring(GLOBAL, "mstring", currentCmd, i, MAXSTR - 1);
	strcat(currentCmd, "\n");
	p11_saveDisCmd();

	if(debug)
            fprintf(stderr, "i %d %s\n", i, currentCmd);
    } else {

    	strcpy(currentCmd, "");
    	strncat(currentCmd, str, MAXSTR);
    }

    if(!part11System && !P_getreal(GLOBAL, "cmdHis", &d, 1) && d < 0.5) return;

    currentDate(cmdTime, MAXSTR);

    if(cmdHistoryFp != NULL) 
	fprintf(cmdHistoryFp,"%s cmd %s", cmdTime,currentCmd);

    if(sessionAuditTrailFp != NULL) 
	fprintf(sessionAuditTrailFp,"%s cmd %s", cmdTime,currentCmd);
}

/******************/
void p11_flush()
/******************/
{
    if(cmdHistoryFp != NULL) fflush(cmdHistoryFp);
    if(sessionAuditTrailFp != NULL) fflush(sessionAuditTrailFp);
}

/******************/
int resetPart11()
/******************/
{
/* reset part11 parameters to default values */
/* 1 = yes, 0 = no */

    part11System = NONFDA;
    okFDAdata = 0;
    strcpy(auditDir, "");
    strcpy(part11RootPath, "");
    strcpy(currentPart11Path, "");

    if(part11_std_files != NULL) free(part11_std_files);
    part11_std_files = 
	(part11_standard_files*)malloc(sizeof(part11_standard_files));

    part11_std_files->fid = 1;
    part11_std_files->procpar = 1;
    part11_std_files->log = 1;
    part11_std_files->text = 1;
    part11_std_files->global = 0;
    part11_std_files->conpar = 0;
    part11_std_files->usermaclib = 0;
    part11_std_files->shims = 0;
    part11_std_files->waveforms = 0;
    part11_std_files->data = 1;
    part11_std_files->phasefile = 1;
    part11_std_files->fdf = 0;
    part11_std_files->snapshot = 0;
    part11_std_files->cmdHistory = 1;
    part11_std_files->pulseSequence = 0;

    if(part11_add_files != NULL) free(part11_add_files);
    part11_add_files = 
	(part11_additional_files*)malloc(sizeof(part11_additional_files));
    part11_add_files->numOfFiles = 0;
    part11_add_files->fullpaths = NULL;

    RETURN;
}

/**************************/
static int file_is_link(char* filepath, char* newpath, char* curfid_linkpath)
/**************************/
{
/* whether curexpdir/acqfil/fid is linked? */
/* return 0 if not a valid link */
/*        1 if a link but newpath != curfid_linkpath */
/*        2 if a link and newpath == curfid_linkpath */
#ifdef UNIX
    char            curfid_link[ MAXPATH*2 ];
    int             ival;
    dev_t           exp_dev, fid_dev;
    ino_t           exp_ino, fid_ino;
    struct stat     stat_blk;

    strcpy(curfid_linkpath, "");

    if (isSymLink( &filepath[ 0 ] ) != 0)
        return( 0 );

/*  Locate the symbolic link (readlink).  */

    ival = readlink( &filepath[ 0 ], &curfid_link[ 0 ], sizeof( curfid_link ) );
    if (ival < 0)
        return( 0 );

/*  UNIX manual asserts `readlink' does not null-terminate the string;
    rather it returns the number of characters in the symbolic link path.  */

    curfid_link[ ival ] = '\0';
    strcpy(curfid_linkpath, curfid_link);

/*
 *  The two file paths point to the same file if the device and I-node
 *  fields in the stat block are both identical.  Return normal status
 *  if not successful.  Most likely two reasons are the file does not
 *  exist or the process has no access.
 */
    ival = stat( &curfid_link[ 0 ], &stat_blk );
    if (ival != 0)
        return( 0 );
    exp_ino = stat_blk.st_ino;
    exp_dev = stat_blk.st_dev;

    ival = stat( &newpath[ 0 ], &stat_blk );
    if (ival != 0) 
        return( 1 );
    fid_ino = stat_blk.st_ino;
    fid_dev = stat_blk.st_dev;

    if (fid_ino != exp_ino || fid_dev != exp_dev)
        return( 1 );
    else 
        return( 2 );
#else 
    strcpy(curfid_linkpath, "");
    return( 0 );                    /* no special action required on VMS */
#endif 
}

/******************/
int isPart11Data()
/******************/
// return 1 if "fidOK", else return 0
{
    int i = fidCheckedOk();
    if(i != 1) i = 0;

    return(i);
}

/******************/
void createIDpars()
/******************/
{
// these parameters will be saved/retrieved with data. 

    vInfo           paraminfo;

    if(P_getVarInfo(CURRENT, "username", &paraminfo) == -2) {
        P_creatvar(CURRENT, "username", ST_STRING);
        P_setstring(CURRENT, "username", "", 1);
    }

    if(P_getVarInfo(CURRENT, "p11console", &paraminfo) == -2) {
        P_creatvar(CURRENT, "p11console", ST_STRING);
        P_setstring(CURRENT, "p11console", "", 1);
    }

    if(P_getVarInfo(CURRENT, "samplename", &paraminfo) == -2) {
        P_creatvar(CURRENT, "samplename", ST_STRING);
        P_setstring(CURRENT, "samplename", "", 1);
    }

    if(P_getVarInfo(CURRENT, "time_run", &paraminfo) == -2) {
        P_creatvar(CURRENT, "time_run", T_STRING);
        P_setstring(CURRENT, "time_run", "", 1);
    }

    if(P_getVarInfo(CURRENT, "time_processed", &paraminfo) == -2) {
        P_creatvar(CURRENT, "time_processed", ST_STRING);
        P_setstring(CURRENT, "time_processed", "", 1);
    }

    if(P_getVarInfo(CURRENT, "time_saved", &paraminfo) == -2) {
        P_creatvar(CURRENT, "time_saved", ST_STRING);
        P_setstring(CURRENT, "time_saved", "", 1);
    }

    if(P_getVarInfo(CURRENT, "fidid", &paraminfo) == -2) {
        P_creatvar(CURRENT, "fidid", ST_STRING);
        P_setstring(CURRENT, "fidid", "", 1);
    }

    if(P_getVarInfo(CURRENT, "dataid", &paraminfo) == -2) {
        P_creatvar(CURRENT, "dataid", ST_STRING);
	P_setgroup(CURRENT,"dataid", G_PROCESSING);
        P_setstring(CURRENT, "dataid", "", 1);
    } else if(paraminfo.group != G_PROCESSING)
	P_setgroup(CURRENT,"dataid", G_PROCESSING);

    if(P_getVarInfo(CURRENT, "processid", &paraminfo) == -2) {
        P_creatvar(CURRENT, "processid", ST_REAL);
	P_setgroup(CURRENT,"processid", G_PROCESSING);
        P_setreal(CURRENT, "processid", (double)0, 1);
    } else if(paraminfo.group != G_PROCESSING)
	P_setgroup(CURRENT,"processid", G_PROCESSING);
}

/******************/
void p11_createpars()
/******************/
{
    createPart11globalpars();
    createPart11currentpars();
}

/******************/
void createPart11globalpars()
/******************/
{
    vInfo           paraminfo;

    if(P_getVarInfo(GLOBAL, "svfdir", &paraminfo) == -2) {
        P_creatvar(GLOBAL, "svfdir", ST_STRING);
        P_setstring(GLOBAL, "svfdir", "", 1);
    }

    if(P_getVarInfo(GLOBAL, "cmdHis", &paraminfo) == -2) {
        P_creatvar(GLOBAL, "cmdHis", ST_REAL);
        P_setreal(GLOBAL, "cmdHis", (double)0, 1);
    }

    if(P_getVarInfo(GLOBAL, "recordSave", &paraminfo) == -2) {
        P_creatvar(GLOBAL, "recordSave", ST_REAL);
        P_setreal(GLOBAL, "recordSave", (double)0, 1);
    }
}

/******************/
void createPart11currentpars()
/******************/
{
    vInfo           paraminfo;

    createIDpars();

/* paramChanged is in default group (G_ACQUISITION), */
/* so not to be saved in procpar or curpar */
    if(P_getVarInfo(CURRENT, "paramChanged", &paraminfo) == -2) {
        P_creatvar(CURRENT, "paramChanged", ST_REAL);
        P_setreal(CURRENT, "paramChanged", (double)0, 1);
    }

    if(P_getVarInfo(CURRENT, "disCmd", &paraminfo) == -2) {
        P_creatvar(CURRENT, "disCmd", ST_STRING);
	P_setgroup(CURRENT,"disCmd", G_DISPLAY);
        P_setstring(CURRENT, "disCmd", "", 1);
    }

    if(P_getVarInfo(CURRENT, "datname", &paraminfo) == -2) {
        P_creatvar(CURRENT, "datname", ST_STRING);
	P_setgroup(CURRENT,"datname", G_PROCESSING);
        P_setstring(CURRENT, "datname", "", 1);
    }

    if(P_getVarInfo(CURRENT, "acqpath", &paraminfo) == -2) {
        P_creatvar(CURRENT, "acqpath", ST_STRING);
	P_setgroup(CURRENT,"acqpathe", G_PROCESSING);
        P_setstring(CURRENT, "acqpath", "", 1);
    }
}

/******************/
void writeCmdHisMenu()
/******************/
{
/* "/persistence/CmdHistoryMenu" hard coded */

    char str[MAXSTR];
    FILE* fp;

    sprintf(str, "%s%s", userdir, cmdHistoryMenu);
    if((fp = fopen(str, "w"))) {
    	fprintf(fp, "%s | %s | %s\n", cmdHisFile, "current data",
                "cmdHistory");
    	fprintf(fp, "%s | %s | %s\n", auditTrail, "current session",
                "cmdHistory");
        fclose(fp);
    }
}

/******************/
void getUserPart11Dirs(char* user, string* dirs, int* ndirs)
/******************/
{
/* each user can have only MAXWORDS part11Dirs */
/* "/adm/users/profiles/user" hard coded */

    FILE* usrfp;
    string words[MAXWORDS];
    int nwords, j;
    char str[MAXSTR];
    char  buf[BUFSIZE];

    j = 0;
    // get dirs from /vnmr/adm/users/profiles/p11/
    sprintf(str, "%s%s%s", systemdir, "/adm/users/profiles/p11/", user);
    if((usrfp = fopen(str, "r"))) {

        while (fgets(buf,sizeof(buf),usrfp)) {

            if(strlen(buf) > 1 && buf[0] != '#' && buf[0] != '%' && buf[0] != '@') {
    		nwords = MAXWORDS;
	        if(strstr(buf, ":") == NULL)
                    getStrValues(buf, words, &nwords, " "); 
		else 
                    getStrValues(buf, words, &nwords, ":"); 
	        if(nwords > 1 && j < (MAXWORDS - 1)) { 
                    if(debug)
                        Winfoprintf("getUserPart11Dirs words %s\n", words[1]);	

                    strcpy(dirs[j], words[1]);
                    j++;
		}
	    }
        }
        fclose(usrfp);
    } else {
	sprintf(dirs[j], "%s/data/%s",sysFDAdir,user); 
    }

    // get dirs from /vnmr/adm/users/profiles/data/
    sprintf(str, "%s%s%s", systemdir, "/adm/users/profiles/data/", user);
    if((usrfp = fopen(str, "r"))) {

        while (fgets(buf,sizeof(buf),usrfp)) {

            if(strlen(buf) > 1 && buf[0] != '#' && buf[0] != '%' && buf[0] != '@') {
    		nwords = MAXWORDS;
	        if(strstr(buf, ":") == NULL)
                    getStrValues(buf, words, &nwords, " "); 
		else 
                    getStrValues(buf, words, &nwords, ":"); 
	        if(nwords > 1 && j < (MAXWORDS - 1)) { 
                    if(debug)
                        Winfoprintf("getUserPart11Dirs words %s\n", words[1]);	

                    strcpy(dirs[j], words[1]);
                    j++;
		}
	    }
        }
        fclose(usrfp);
    }
    *ndirs = j;
}

/******************/
void writeAuditMenu()
/******************/
{
    char str[MAXSTR];
    string words[MAXWORDS], dirs[MAXWORDS];
    char  buf[BUFSIZE];
    FILE* fp = NULL;
    FILE* usrfp;
    int i, j, nwords, ndirs;

/* write SystemAuditMenu */
/* unix and auditing not implemented */

    sprintf(str, "%s%s", userdir, systemAuditMenu);
    if(!fileExist(str) && (fp = fopen(str, "w"))) {
    	fprintf(fp, "%s%s | %s | %s\n", auditDir, "sessions", "sessions audit trails", 
		"s_auditTrailFiles");
    	fprintf(fp, "%s%s | %s | %s\n", auditDir, "user", "user account audit trails", 
		"u_auditTrailFiles");
    	fprintf(fp, "%s%s | %s | %s\n", auditDir, "aaudit", "auditing audit trails", 
		"a_auditTrailFiles");
        fclose(fp);
    }

/* write RecordAuditMenu */
/* based on /vnmr/adm/users/userlist and /vnmr/adm/users/profiles/p11 */

    sprintf(str, "%s%s", userdir, recordAuditMenu);
    if(!fileExist(str) && (fp = fopen(str, "w"))) {
	
	/* user part11 dirs */
    	sprintf(str, "%s%s", systemdir, "/adm/users/userlist");
	if((usrfp = fopen(str, "r"))) {

            while (fgets(buf,sizeof(buf),usrfp)) {

                if(strlen(buf) > 1 && buf[0] != '#' && buf[0] != '%' && buf[0] != '@') {
                    nwords = MAXWORDS;
                    getStrValues(buf, words, &nwords, " "); 
                    if(nwords > 0) 
                        for(i=0; i<nwords; i++) {	

                            if(debug)
                                Winfoprintf("writeAuditMenu words %s\n", words[i]);	

                            /* words are user names */
                            /* get part11Dirs for user words[i] */
                            /* from/vnmr/adm/users/profiles/p11 */
                            getUserPart11Dirs(words[i], dirs, &ndirs);
                            for(j=0; j<ndirs; j++) { 
                                fprintf(fp, "%s | %s | %s\n", dirs[j], dirs[j], "records");
                            }
                        }
                }
            }
            fclose(usrfp);
	}
        fclose(fp);
    }
}

/******************/
void initSvfdir()
/******************/
{
    string dir;

    if(P_getstring(GLOBAL, "svfdir", dir, 1, MAXSTR - 1))
        strcpy(dir, "");

    if(!part11System && p11_isPart11Dir(dir)) {
        sprintf(dir, "%s%s", userdir, "/data");
    } 
/*
    if(part11System == FDA && !p11_isPart11Dir(dir)) {
        if(nuserPart11Dirs >0) strcpy(dir, userPart11Dirs[0]);
        else strcpy(dir, "");
    } 
*/
    if(strlen(dir) == 0 || dir[0] == '\n' || dir[0] == '\0') {

        if(part11System && nuserPart11Dirs > 0) {
            strcpy(dir, userPart11Dirs[0]);
        } else { 
            sprintf(dir, "%s%s", userdir, "/data");
        }
    }
     
    P_setstring(GLOBAL,"svfdir",dir,0);
#ifdef VNMRJ
    // writelineToVnmrJ("pnew", "1 svfdir");
    appendJvarlist("svfdir");
#endif 
}

/******************/
void make_checksum(char *key, char *dest)
/******************/
{
    char  buf[BUFSIZE];
    string path, str, file, filekey, dirkey;
    string files[MAXFILES], fs[MAXFILES];
    char* ptr;
    int i, n, nf, k1, k2;
    FILE* fp;
    FILE* rfp;
    sprintf(path, "%s/p11/%s", systemdir, "part11Config");

    if(!(fp = fopen(path, "r"))) return;

    sprintf(filekey, "file:%s:", key); 
    k1 = strlen(filekey);
    sprintf(dirkey, "dir:%s:", key); 
    k2 = strlen(dirkey);

    n = 0;
    while (fgets(buf,sizeof(buf),fp)) {
        if((ptr = strstr(buf,filekey)) == buf) {
            ptr += k1;
            if(strstr(ptr,":yes\n") != NULL) {
                strcpy(str,"");
                strncat(str, ptr, strlen(ptr)-5);
		sprintf(file, "%s/%s", dest, str);
		if((rfp = fopen(file, "r"))) {
                    strcpy(files[n],str);
                    n++;
		}

                if(rfp) 
		    fclose(rfp);
            }
        } else if((ptr = strstr(buf,dirkey)) == buf) {
            ptr += k2;
            nf = 0;
            if(strstr(ptr,":yes\n") != NULL) {
                strcpy(str,"");
                strncat(str, ptr, strlen(ptr)-5);
                sprintf(path, "%s/%s", dest, str);
		if(!isAdirectory(path)) {
                    if((rfp = fopen(path, "r"))) {
                        strcpy(files[n],str);
                        n++;
                    }
                    if(rfp)
                        fclose(rfp);
		} else {
                    getFilenames(path, "", fs, &nf, MAXFILES);
                    for(i=0; i<nf; i++) {
                        sprintf(file, "%s/%s", path, fs[i]);
                        if((rfp = fopen(file, "r"))) {
                            sprintf(files[n], "%s/%s", str, fs[i]);
                            n++;
                        }
                        if(rfp)
                            fclose(rfp);
                    }
		}
            }
	}
    }
    if(fp)
        fclose(fp);

    makeChksums(dest, "checksum", files, n);
}

// p11_action('showDataID',1/0) to toggle on/off dataID display
// p11_action('showLink',1/0) to toggle on/off display link icon
// p11_action('process',cmd) to flush parameters after executing given cmd.
// p11_action('auto'/'study'/'standar', path) to generate checksums for given path.
/******************/
int p11_action(int argc, char *argv[], int retc, char *retv[])
/******************/
{
    char str[MAXSTR];
    extern int     start_from_ft;   /* set by ft if ds is to be executed */

    if ( (argc == 2) && ! strcmp(argv[1],"showDataID") )
    {
       if (retc > 0)
          retv[0]= (char *)realString((double)showDataID);
       else
          Winfoprintf("showDataID= %d",showDataID); 
    }
    if(argc < 3) RETURN;
    if(!part11System) RETURN;

    if ( ! strcmp(argv[1],"showDataID") )
    {
       showDataID = atoi(argv[2]);
       RETURN;
    }

    if ( ! strcmp(argv[1],"showLink") )
    {
       int b = atoi(argv[2]);
       if(!b) writelineToVnmrJ("vnmrjcmd", "p11link none");
       showLink = b;
       RETURN;
    }

    if ( ! strcmp(argv[1],"process") )
    {
       strcpy(currentCmd,argv[2]);
       strcat(currentCmd, "\n");
       p11_saveFDAfiles_processed("p11_action", "-", "datdir");
       start_from_ft = 1;
       RETURN;
    }

    if(!fileExist(argv[2]) || !isUserOwned(argv[2])) RETURN;

    sprintf(str,"%s/checksum", argv[2]);
    if(fileExist(str)) unlink(str);

    if(!strcasecmp(argv[1],"automation") || !strcasecmp(argv[1],"auto")) {
        make_checksum("auto", argv[2]);
    } else if(!strcasecmp(argv[1],"study")) {
        make_checksum("study", argv[2]);
    } else if(!strcasecmp(argv[1],"record")) {
        make_checksum("standard", argv[2]);
    } 

    RETURN;
}

// get user or operator full name from operatorlist.
// assume operatorlist contains all users and operators.
// assume format is "vnmr1  vnmr1;null;30;System Administrator;yes;AllLiquids"
void getFullName(char *fullname) 
{
   FILE* fp;
   char  buf[BUFSIZE];
   string path, operator;
   sprintf(path, "%s%s", systemdir, "/adm/users/operators/operatorlist");

   if(P_getstring(GLOBAL, "operator", operator, 1, MAXSTR - 1))
	strcpy(operator,UserName);

   strcpy(fullname,operator); 
   if((fp = fopen(path, "r"))) {
      int len = strlen(operator);
      while (fgets(buf,sizeof(buf),fp)) {
        if(strlen(buf) > 1 && buf[0] != '#') {
	   if(strncmp(buf,operator, len) == 0) {
    	     string words[MAXWORDS];
	     int nwords = 0;
             getStrValues(buf, words, &nwords, ";"); 
	     if(nwords == 5) {
		strcpy(fullname,words[3]);
	     }
	   } 
	}
      } 
      fclose(fp);
   }
}

void getSoftwareVersion(char *version) 
{
   FILE* fp;
   char  buf[BUFSIZE];
   string path;
   sprintf(path, "%s%s", systemdir, "/vnmrrev");

   strcpy(version,"");
   if((fp = fopen(path, "r"))) {
      while (fgets(buf,sizeof(buf),fp)) {
        if(strlen(buf) > 1 && buf[0] != '#') {
	   if(buf[strlen(buf)-1] == '\n') strncat(version,buf,strlen(buf)-1);
           else strcat(version,buf);
	   strcat(version," ");
        }
      }
      fclose(fp);
   }
}

/******************/
int p11_switchOpt(int argc, char *argv[], int retc, char *retv[])
/******************/
/* call this in operatorlogin macro */
{
    string str;

    (void) retc;
    (void) retv;
    currentDate(bootupTime, MAXSTR);

/* save audit trail before change operatorID. */
/* and open new session audit file and cmd history file. */
/* skip this for first operator switch (which is actually first login) */
    if(nlogin > 0) {
        p11_saveAuditTrail();

    
        if(mode_of_vnmr != AUTOMATION) {
            sprintf(auditTrail, "%s%s", userdir, sessionAudit);
            sessionAuditTrailFp = fopen(auditTrail, "w");
        } else sessionAuditTrailFp = NULL;

        if(mode_of_vnmr != AUTOMATION && part11_std_files->cmdHistory) {
            sprintf(cmdHisFile, "%s%s", userdir, "/cmdHistory");
/*
            if(fileExist(cmdHisFile)) cmdHistoryFp = fopen(cmdHisFile, "a");
            else 
*/
            cmdHistoryFp = fopen(cmdHisFile, "w");
        } else cmdHistoryFp = NULL;
    }

    if(argc > 1) {
        strcpy(str,argv[1]);
    } else {
        P_getstring(GLOBAL, "operator", str, 1, MAXSTR - 1);
    } 
    if(strlen(str) == 0) strcpy(str, "?");
    sprintf(operatorID, "%s:%s", UserName, str);

    getFullName(operatorFullName);

    nlogin++;

    RETURN;
}

/******************/
int p11_init()
/******************/
{
    char str[MAXSTR];
    double d;
    int i;
    nlogin = 0;

/* called in bootup.c when vnmr is booted. */

/* get bootup time */

    currentDate(bootupTime, MAXSTR);

/* get p11console name */

    P_getstring(SYSTEMGLOBAL, "Console", str, 1, MAXSTR - 1);
    if(strlen(str) == 0) strcpy(str, "?");

    sprintf(p11console, "%s:%s", HostName, str);

/* init operatorID */

    P_getstring(GLOBAL, "operator", str, 1, MAXSTR - 1);
    if(strlen(str) == 0)
        sprintf(operatorID, "%s", UserName);
    else
        sprintf(operatorID, "%s:%s", UserName, str);

    getFullName(operatorFullName);
    getSoftwareVersion(softwareVersion);

/* create disCmd */
    p11_createpars();
    if(!P_getreal(CURRENT, "paramChanged", &d, 1)) paramChanged = (int)d;
    else paramChanged = 0;

/* initialize and set part11 parameters */

    resetPart11();

    readPart11Config();

/* determine whether current acqfil data is part11Data */

    if(part11System) {
        P_setreal(GLOBAL, "recordSave", (double)1, 1);
    } else {
        P_setreal(GLOBAL, "recordSave", (double)0, 1);
    }

/* open cmdHistory file. if file exists, open to append */
/* cmdHistory may cross sessions. */
/* it is emptied when data is retrieved with rt */

    if(mode_of_vnmr != AUTOMATION && part11_std_files->cmdHistory) {
        sprintf(cmdHisFile, "%s%s", userdir, "/cmdHistory");
	cmdHistoryFp = fopen(cmdHisFile, "w");
    } else cmdHistoryFp = NULL;

/* write file for fileMenu of command history popup */

    writeCmdHisMenu();

/* do the following even not a part11System */
    getUserPart11Dirs(UserName, userPart11Dirs, &nuserPart11Dirs);
    for(i=0; i<nuserPart11Dirs; i++) {
    	if(!strEndsWith(userPart11Dirs[i],"/")) strcat(userPart11Dirs[i], "/");
    }

    //initSvfdir();

    if(!part11System) {
/*
  resetPart11();
*/
	RETURN;
    }

/* open session auditTrail file, a /tmp file. */
/* a /var/tmp file. will save to systemdir/p11/auditTrails when exit */
/* filename will reflect time span of the auditing. */

/*
  p11TempName(auditTrail);
  strcat(auditTrail, ".vaudit");
*/
    if(mode_of_vnmr != AUTOMATION) {
        sprintf(auditTrail, "%s%s", userdir, sessionAudit);
        sessionAuditTrailFp = fopen(auditTrail, "a");
    } else sessionAuditTrailFp = NULL;
/*
  writeAuditMenu();
*/
    okFDAdata = 0;
    updateSvfdirIcon();
    updateCheckIcon(); 
    updateLinkIcon(); 
    RETURN;
}

/******************/
void displayPart11Config(FILE *fp)
/******************/
{
/* for debugging */

    int i;

    fprintf(fp,"part11System %d\n", part11System);
    fprintf(fp,"auditDir %s\n", auditDir);
    fprintf(fp,"part11RootPath %s\n", part11RootPath);
    fprintf(fp,"part11_std_files: \n");
    if(part11_std_files->fid) fprintf(fp," %s\n", "fid");
    if(part11_std_files->procpar) fprintf(fp," %s\n", "procpar");
    if(part11_std_files->log) fprintf(fp," %s\n", "log");
    if(part11_std_files->text) fprintf(fp," %s\n", "text");
    if(part11_std_files->global) fprintf(fp," %s\n", "global");
    if(part11_std_files->conpar) fprintf(fp," %s\n", "conpar");
    if(part11_std_files->usermaclib) fprintf(fp," %s\n", "usermaclib");
    if(part11_std_files->shims) fprintf(fp," %s\n", "shims");
    if(part11_std_files->waveforms) fprintf(fp," %s\n", "waveforms");
    if(part11_std_files->data) fprintf(fp," %s\n", "data");
    if(part11_std_files->phasefile) fprintf(fp," %s\n", "phasefile");
    if(part11_std_files->fdf) fprintf(fp," %s\n", "fdf");
    if(part11_std_files->snapshot) fprintf(fp," %s\n", "snapshot");
    if(part11_std_files->cmdHistory) fprintf(fp," %s\n", "cmdHistory");
    if(part11_std_files->pulseSequence) fprintf(fp," %s\n", "pulseSequence");
    fprintf(fp,"part11_add_files: \n");
    for(i=0; i<part11_add_files->numOfFiles; i++)
    	fprintf(fp," %s\n", part11_add_files->fullpaths[i]);
}

/******************/
int readPart11Config()
/******************/
{
/* part11 parameters are set by systemdir/p11/part11Config file */

/* the following is how part11Config is formatted (for example) */
/* if user path is specified, use it, otherwise use system path. */

/* auditDir:system:/vnmr/p11/auditTrails:system:yes */
/* part11Dir:system:/vnmr/p11/data:system:yes */
/* part11Dir:hel:/vnmr/p11/part1/data/demo:system:no */
/* part11Dir:chin:/usr25/chin/vnmrsys/p11/data:dan:no */
/* part11Dir:dan:/usr25/dan/vnmrsys/p11/data:dan:no */
/* file:standard:cmdHistory:yes */
/* file:standard:fid:yes */
/* file:standard:procpar:yes */
/* file:standard:log:yes */
/* file:standard:text:yes */
/* file:standard:global:yes */
/* file:standard:conpar:yes */
/* file:standard:shims:no */
/* file:standard:waveforms:no */
/* file:standard:pulseSequence:no */
/* file:standard:data:yes */
/* file:standard:phasefile:yes */
/* file:standard:fdf:no */
/* file:standard:snapshot:yes */
/* file:additional:usermaclib:no */

/* word[1] for auditDir and part11Dir can be system or user. */
/* word[3] is the real owner of the directory, which can be different */
/* from word[1]. */

/* but for now, system path is always owned by system. */
/* audirDir has only system path. part11Dir may have user path, */
/* but owner (word[3]) can only be system or the user. */

/* for a user path, if word[3] == word[1], owner is the user, */
/* otherwise owner is system. */

/* here system is vnmr1, and the path is systemdir. */
/* to allow either system or user own the data, two copies of safecp */
/* must exist, one is usrOwnedSafecp (systemdir/bin/safecp -rwxr-xr-x), and the other */
/* is sysOwnedSafecp (systemdir/p11/bin/safecp -rwsr-sr-x). */ 

/* safecp differ from cp by not allowing overwrite if file already exists */
/* and not copying file permissions (always setting 0644). */

/* usrOwnedSafecp sets the owner of the destination file as user */
/* sysOwnedSafecp sets the owner of the destination file as */
/* the owner of sysOwnedSafecp, which is the system (vnmr1). */
 
    char path[MAXPATHL];
    char  buf[BUFSIZE];
    FILE* fp;
    string words[MAXWORDS];
    int i, nwords;
    string str, tmpstrs[MAXWORDS]; 

    sprintf(part11RootPath, "%s%s", systemdir, "/p11/");

    sprintf(sysOwnedSafecp, "%s%s", part11RootPath, "bin/safecp");
    sprintf(usrOwnedSafecp, "%s%s", systemdir, "/bin/safecp");

    sprintf(path, "%s%s", part11RootPath, "dataID_off");
    if ( ! access(path,F_OK) )
       showDataID = 0;
    else
       showDataID = 1;

    sprintf(path, "%s%s", part11RootPath, "part11Config");

/* sysOwnedSafecp has to exist for a part11System to cp session auditTrail */
/* to part11/auditTrails. */
/*
  if(!fileExist(sysOwnedSafecp)) ABORT;
*/
    if(!(fp = fopen(path, "r"))) {
	
        if(P_getstring(GLOBAL, "appmode", str, 1, MAXSTR)) strcpy(str, "");

	if(strcmp(str,"imaging") == 0)
            sprintf(path, "%s%s", systemdir, "/imaging/templates/vnmrj/properties/recConfig");
	else 
            sprintf(path, "%s%s", systemdir, "/templates/vnmrj/properties/recConfig");

        if(!(fp = fopen(path, "r"))) ABORT;
    }

    part11System = NONFDA;
    while (fgets(buf,sizeof(buf),fp)) {

        if(strlen(buf) > 1 && buf[0] != '#' && buf[0] != '%' && buf[0] != '@') {
            nwords = MAXWORDS;
            getStrValues(buf, words, &nwords, ":"); 

            if(nwords > 1) {
/*	
  Winfoprintf("words %s %s \n", words[0], words[1]);	
*/
		if(strcmp(words[0], "dataType") == 0 
                   && (strstr(words[1], "non-FDA") != NULL ||
                       strstr(words[1], "Non-FDA") != NULL)) 
		    part11System = NONFDA;
		else if(strcmp(words[0], "dataType") == 0 
                        && strstr(words[1], "FDA") != NULL) 
		    part11System = FDA;
		else if(strcmp(words[0], "dataType") == 0 
                        && strstr(words[1], "Both") != NULL) 
		    part11System = FDA;
		else if(strcmp(words[0], "auditDir") == 0) 
                    strcpy(auditDir, words[1]);
		else if(strcmp(words[0], "part11Dir") == 0) 
                    strcpy(sysFDAdir, words[1]);
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "fid") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->fid = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "procpar") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->procpar = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "log") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->log = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "text") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->text = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "global") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->global = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "conpar") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->conpar = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "usermaclib") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->usermaclib = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "shims") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->shims = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "waveforms") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->waveforms = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "data") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->data = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "phasefile") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->phasefile = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "fdf") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->fdf = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "snapshot") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->snapshot = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "cmdHistory") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->cmdHistory = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "standard") == 0
                        && strcmp(words[2], "pulseSequence") == 0 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->pulseSequence = 2;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "additional") == 0
                        && strcmp(words[3], "yes") == 0 
                        && part11_add_files->numOfFiles < MAXWORDS) {
                    strcpy(tmpstrs[part11_add_files->numOfFiles],words[2]);
                    (part11_add_files->numOfFiles)++;
                }
            }
        }
    }

/* if auditDir does not contain part11RootPath, set it to default */
/*
  if(strstr(auditDir, part11RootPath) == NULL) {
  strcpy(auditDir, part11RootPath);
  strcat(auditDir, "auditTrails/");
  } 
*/

/* auditDir should end with "/" */
    i = strlen(auditDir);
    if(i > 0 && auditDir[i-1] != '/')
        strcat(auditDir, "/");

/* copy additional file path */
    if(part11_add_files->numOfFiles > 0) {
	part11_add_files->fullpaths = 
	    (string*)realloc(part11_add_files->fullpaths, sizeof(string)*
                             (part11_add_files->numOfFiles));
	for(i=0; i<part11_add_files->numOfFiles; i++)
            strcpy(part11_add_files->fullpaths[i],tmpstrs[i]);
    }
 
    if(fp)
        fclose(fp);

/*
  if(fp = fopen("/usr26/hel/vnmrbg/TMP3/jk", "w"))
  displayPart11Config(fp);
  fclose(fp);
*/
    
    RETURN;
}

/**************************/
int p11_isPart11Dir(string str) 
/**************************/
{
/* given a str, determine whether it is in userPart11Dirs */

    if(!part11System) return 0;
    else return 1;
/*
    int i;

    getUserPart11Dirs(UserName, userPart11Dirs, &nuserPart11Dirs);
    for(i=0; i<nuserPart11Dirs; i++) {
    	if(!strEndsWith(userPart11Dirs[i],"/")) strcat(userPart11Dirs[i], "/");
    }
    
    if(strlen(str) == 0 || str[0] == '\n' || str[0] == '\0') return(0); 

    if(str[strlen(str)-1] != '/') strcat(str, "/");

    for(i=0; i<nuserPart11Dirs; i++) {

	if(debug)
            fprintf(stderr, "p11_isPart11Dir %s %s\n", str, userPart11Dirs[i]);

	if(strStartsWith(str, userPart11Dirs[i])) return(1);
    }

    return(0);
*/
}

/**************************/
int isParfileDiff(char* file1, char* file2)
/**************************/
{
    FILE *f1, *f2;
    char buf1[BUFSIZE], buf2[BUFSIZE];
    char *str1, *str2;

    if(!(f1 = fopen(file1, "r"))) {
        return(1);
    }
    if(!(f2 = fopen(file2, "r"))) {
    	fclose(f1);
        return(1);
    }

    strcpy(buf1, "");
    strcpy(buf2, "");
    str1 = buf1;
    str2 = buf2;
    while(str1 != NULL || str2 != NULL) {
        str1 = fgets(buf1,sizeof(buf1),f1);
	str2 = fgets(buf2,sizeof(buf2),f2);
	if((strstr(buf1,"time") == buf1 && strstr(buf2,"time") == buf2) ||
	   (strcmp(buf1,"file") == 0 && strcmp(buf2,"file") == 0) || 
	   (strncmp(buf1,"paramChanged", 12) == 0 &&
	    strncmp(buf2,"paramChanged", 12) == 0) ||
	   (strncmp(buf1,"disCmd", 6) == 0 &&
	    strncmp(buf2,"disCmd", 6) == 0) ) {
	    /* skip the enext two lines */
	    str1 = fgets(buf1,sizeof(buf1),f1);
	    str1 = fgets(buf1,sizeof(buf1),f1);
	    str2 = fgets(buf2,sizeof(buf2),f2);
	    str2 = fgets(buf2,sizeof(buf2),f2);
	} else if(strcmp(buf1, buf2) != 0) {
            if(debug) {
		fprintf(stderr,"isParfileDiff %s", buf1); 
		fprintf(stderr,"isParfileDiff %s\n", buf2); 
            }
            fclose(f1);
            fclose(f2);
            return(1);
	} 
    }

    fclose(f1);
    fclose(f2);
    return(0);
}

/**************************/
int hasParamChanged()
/**************************/
{
/* write out tmppar and compare checksum of it */
/* with that of prevpar */

    int i = 0;
    string tmppar, prevpar; 
   
    sprintf( prevpar, "%s%s", curexpdir, "/prevpar");
    if(!fileExist(prevpar)) i = 1;
    else {
  
        /* save params */
        sprintf( tmppar, "%s%s", curexpdir, "/tmppar");
	unlink(tmppar);
	saveCurpar(tmppar);
	i = isParfileDiff(tmppar, prevpar);
    }

    return(i);
}

void p11_updateParamChanged()
{
/* called by p11_writeCmdHistory */

    double d;
    string str;

    if(strstr(currentCmd,"trackCursor") != NULL) return;
    if(strstr(currentCmd,"recordOff") != NULL) return;

    if(!part11System && !P_getreal(GLOBAL, "recordSave", &d, 1) && d < 0.5) return;

    if(P_getstring(GLOBAL, "acqmode", str, 1, MAXSTR)) strcpy(str, "");
    if(strcmp(str, "fidscan") == 0 || strcmp(str, "lock") == 0) return;

    if(!paramNewCmd) return;
    paramNewCmd = 0;
    if( strncmp(currentCmd, "cpADMIN", 7) == 0) {
        updateSvfdirIcon();
    }
}

/**************************/
void p11_updateParamChangedOld()
/**************************/
{
/* called by p11_writeCmdHistory */

    double d;
    string str;

    if(strstr(currentCmd,"trackCursor") != NULL) return;
    if(strstr(currentCmd,"recordOff") != NULL) return;

    if(!part11System && !P_getreal(GLOBAL, "recordSave", &d, 1) && d < 0.5) return;

    if(P_getstring(GLOBAL, "acqmode", str, 1, MAXSTR)) strcpy(str, "");
    if(strcmp(str, "fidscan") == 0 || strcmp(str, "lock") == 0) return;

    if(!paramNewCmd) return;
    paramNewCmd = 0;

    if(strncmp(currentCmd, "RT", 2) == 0 ||
	strncmp(currentCmd, "locaction", 9) == 0 ) {
        if(!P_getstring(CURRENT, "file", str, 1, MAXSTR) &&
           strstr(str, ".REC/") != NULL &&
           strstr(str,"/acqfil") == NULL) {
	     char *ptr=str;
	     if(strrchr(ptr+strlen(str)-4,'.')) {
               strcat(str,"/acqfil");
             }
             checkPart11Checksums0(str, stdout);
        }
    }

    if( strncmp(currentCmd, "RT", 2) == 0 ||
       strncmp(currentCmd, "svr", 3) == 0 ||
       strstr(currentCmd, "svf") != NULL ||
       strncmp(currentCmd, "jexp", 4) == 0 ||
       strncmp(currentCmd, "locaction", 9) == 0 ||
       strncmp(currentCmd, "savefid", 7) == 0 ||
       strncmp(currentCmd, "xmaction", 9) == 0 ||
       strncmp(currentCmd, "testPart11", 2) == 0 ||
       strncmp(currentCmd, "bootup", 6) == 0 ||
       strncmp(currentCmd, "acqstatus", 9) == 0 ||
       strcmp(currentCmd, "go") == 0 ||
       strcmp(currentCmd, "au") == 0 ||
       strncmp(currentCmd, "rt", 2) == 0 ) {
	okFDAdata = 0;
        updateCheckIcon(); 
        updateLinkIcon(); 
#ifdef VNMRJ
        // writelineToVnmrJ("pnew", "1 file");
        appendJvarlist("file");
#endif 
    }
/*
    if(!part11Data) {
	paramChanged = 0;
    } else {
	if(debug)
            fprintf(stderr, "******p11_updateParamChanged\n");

        paramChanged = hasParamChanged();
    }

    if(part11System && !P_setreal(CURRENT, "paramChanged", (double)paramChanged, 1)) { 
#ifdef VNMRJ
        // writelineToVnmrJ("pnew", "1 paramChanged");
        appendJvarlist("paramChanged");
#endif 
    }
*/
}

/**************************/
void p11_updateDisChanged()
/**************************/
{
/* called by processJMouse (buttons.c) */

    double d;
    string str;

    if(strstr(currentCmd,"recordOff") != NULL) return;

    if(!part11System && !P_getreal(GLOBAL, "recordSave", &d, 1) && d < 0.5) return;

    if(P_getstring(GLOBAL, "acqmode", str, 1, MAXSTR)) strcpy(str, "");
    if(strcmp(str, "fidscan") == 0 || strcmp(str, "lock") == 0) return;
/*
    if(!part11Data) {
	paramChanged = 0;
    } else {
	if(debug)
            fprintf(stderr, "******p11_updateDisChanged\n");

        paramChanged = hasParamChanged();
    }

    if(part11System && !P_setreal(CURRENT, "paramChanged", (double)paramChanged, 1)) {
#ifdef VNMRJ
        // writelineToVnmrJ("pnew", "1 paramChanged");
        appendJvarlist("paramChanged");
#endif 
    }
*/
}

/**************************/
static void report_copy_file_error(int errorval, char* filename )
/**************************/
{
    if (errorval == SIZE_MISMATCH)
        Werrprintf( "'%s' file not completely copied", filename );
    else if (errorval == NO_SECOND_FILE)
        Werrprintf( "Failed to create '%s' file", filename );
    else 
        Werrprintf( "Problem copying '%s' file", filename );
}

/**************************/
static int safecp_file(char* safecpPath, char* origpath, char* destpath)
/**************************/
{
    int r;

    r = safecp_file_verify(&safecpPath[0],  &origpath[ 0 ], &destpath[ 0 ] );
    if (r != 0) {
        report_copy_file_error( r, destpath );
        ABORT;
    }
    RETURN;
}

/**************************/
int safecp_file_verify(char* safecpPath, char* file_a, char* file_b )
/**************************/
{
    int      ival, l1, l2, l3;
    char     cp_cmd[BUFSIZE];

    l1 = strlen(safecpPath);
    if (l1 < 1) return( -1 );
    l2 = strlen(file_a);
    if (l2 < 1) return( -1 );
    l3 = strlen(file_b);
    if (l3 < 1) return( -1 );

    ival = strlen(safecpPath) + strlen(file_a) + strlen(file_b) + 3;
    if(ival > BUFSIZE) return(-1);
    sprintf(cp_cmd, "%s %s %s", safecpPath, file_a, file_b );

    system( cp_cmd );

    if(strstr(file_b,".REC") == NULL) return 0;

    ival = verify_copy( file_a, file_b );
    return( ival );
}

/**************************/
int saveCurpar(char* path)
/**************************/
{
    if (P_save(CURRENT,path))
    {   Werrprintf("Problem saving current parameters in '%s'.",path);
        ABORT;
    }
    RETURN;
}

/**************************/
int saveProcpar(char* path)
/**************************/
{
    P_treereset(TEMPORARY);           /* clear the tree first */
    P_copy(PROCESSED,TEMPORARY);
    P_copygroup(CURRENT,TEMPORARY,G_DISPLAY);
    P_copygroup(CURRENT,TEMPORARY,G_PROCESSING);
    if (P_save(TEMPORARY,path))
    {   Werrprintf("Problem saving procpar parameters in '%s'.",path);
        ABORT;
    }
    RETURN;
}

/**************************/
void writeLineForLoc(string path, char* type)
/**************************/
{
    char tmppath[3*MAXPATH];
    char *ptmp = "";
    int i;
    string currentPart11Path;

    if(path[strlen(path)-1] == '/') { 
	strcpy(currentPart11Path, "");
	strncat(currentPart11Path, path, strlen(path)-1);
    } else {
	strcpy(currentPart11Path, path);
    }

    if (currentPart11Path[0]=='/')
        strcpy(tmppath,currentPart11Path);
    else
    {
        if (getcwd(tmppath,MAXPATH) == NULL)
            strcpy(tmppath,"");
        strcat(tmppath,"/");
        strcat(tmppath,currentPart11Path);
        strncpy(currentPart11Path,tmppath,MAXPATH);
        i = strlen(currentPart11Path);
        if (currentPart11Path[i] != '\0')
            currentPart11Path[i] = '\0';
    }
    for (i=strlen(currentPart11Path); i>0; i--)
    {
        ptmp = &currentPart11Path[i-1];
        if (*ptmp == '/')
        {
            ptmp++;
            break;
        }
    }
    sprintf(tmppath,"%s %s %s %s",UserName, type, ptmp,currentPart11Path);
    if (strlen(tmppath) > 3*MAXPATH)
        Winfoprintf("WARNING: %s cannot add file to database, filename too long!\n","svr");
    else {
#ifdef VNMRJ
        writelineToVnmrJ("svr",tmppath);
#endif 
//????
        if(debug)
            Winfoprintf("writelineToVnmrJ %s %s\n","svr",tmppath); 
    }
}

/**************************/
void save_recordInfo()
/**************************/
{
    vInfo           paraminfo;

    if(P_getVarInfo(CURRENT, "username", &paraminfo) == -2) {
        P_creatvar(CURRENT, "username", ST_STRING);
    }
    P_setstring(CURRENT, "username", UserName, 1);

    currentDate(cmdTime, MAXSTR);
    P_setstring(CURRENT, "time_saved", cmdTime, 1);
}

/**************************/
static int save_acqfil(char* orig, char* dest)
/**************************/
/* copy all relevant files in orig (curexpdir/acqfil) to dest */

/*  Remember the original path is constrained
    to MAXPATH-32 or fewer letters.             */
{

    char origpath[MAXPATH],destpath[MAXPATH];
    char file[MAXPATH], safecpPath[MAXPATH];
    char infile[MAXPATH];
    string str, files[MAXSTR];
    int i, nfiles;

    if(fileExist(dest)) {
	Werrprintf("cannot save: %s already exists.\n",dest); 
	ABORT;
    }

    if(part11_std_files == NULL) ABORT; 

    flush(0,NULL,0,NULL);

    save_recordInfo();

    if(!strEndsWith(orig,"/")) strcat(orig, "/");
    if(!strEndsWith(dest,"/")) strcat(dest, "/");

/*  save data in curexpdir/acqfil to currentPart11Path/acqfil */

    setSafecpPath(safecpPath, dest);

    if(part11_std_files->text) { 
        sprintf( &origpath[ 0 ], "%s%s", orig, "text");
        sprintf( &destpath[ 0 ], "%s%s", dest, "text");
        if(!fileExist(origpath)) {
 	   sprintf(origpath, "%s/text", curexpdir);
        }
        if(fileExist(origpath)) safecp_file(safecpPath, origpath, destpath);
    }

    if(part11_std_files->log) {
        sprintf( &origpath[ 0 ], "%s%s", orig, "log");
        sprintf( &destpath[ 0 ], "%s%s", dest, "log");
        if(fileExist(origpath)) safecp_file(safecpPath, origpath, destpath);
    }

    if(part11_std_files->fid) {
        sprintf( &origpath[ 0 ], "%s%s", orig, "fid");
        sprintf( &destpath[ 0 ], "%s%s", dest, "fid");
        if(fileExist(origpath)) safecp_file(safecpPath, origpath, destpath);
    }

    if(part11_std_files->procpar) {
	string fidid, dataid, dataid2;
        P_getstring(CURRENT, "dataid", dataid, 1, MAXSTR); 
        if(!P_getstring(PROCESSED, "fidid", fidid, 1, MAXSTR) && strlen(fidid)>8) { 
          sprintf(dataid2, "%.8s000", fidid);
          P_setstring(CURRENT, "dataid", dataid2, 1); 
    	}

        sprintf( &origpath[ 0 ], "%s%s", orig, "procpar");
        sprintf( &destpath[ 0 ], "%s%s", dest, "procpar");
        if(fileExist(origpath)) unlink(origpath);
	saveProcpar(origpath);
        if(fileExist(origpath)) {
	  string tmp;
	  safecp_file(safecpPath, origpath, destpath);
	  strcpy(tmp,"");
	  strncat(tmp,dest,strlen(dest)-7); 
          sprintf( &destpath[ 0 ], "%s%s", tmp, "procpar");
	  // save redundant procpar in .REC
	  safecp_file(safecpPath, origpath, destpath);
	}

        P_setstring(CURRENT, "dataid", dataid, 1); 
        /* save display params */
        sprintf( &origpath[ 0 ], "%s%s", orig, "curpar");
        sprintf( &destpath[ 0 ], "%s%s", dest, "curpar");
        saveCurpar(origpath);
        if(fileExist(origpath)) {
            safecp_file(safecpPath, origpath, destpath);
            /* copy curpar to datdir so if data is not processed */
            /* and saved, hasParamChanged will use this curpar */
            /* this curpar will be overwritten by save_datdir */ 
/*
  sprintf(file, "%s%s", curexpdir, "/datdir/curpar");
  unlink(file);
  copy_file(origpath, file);
*/
        }
    }

    if(part11_std_files->global) {
        sprintf( &origpath[ 0 ], "%s%s", orig, "global");
        sprintf( &destpath[ 0 ], "%s%s", dest, "global");
        if(fileExist(origpath)) safecp_file(safecpPath, origpath, destpath);
    }

    if(part11_std_files->conpar) {
        sprintf( &origpath[ 0 ], "%s%s", orig, "conpar");
        sprintf( &destpath[ 0 ], "%s%s", dest, "conpar");
        if(fileExist(origpath)) safecp_file(safecpPath, origpath, destpath);
    }

    if (mode_of_vnmr == AUTOMATION)
       strcpy(infile,orig);
    else
       strcpy(infile,curexpdir);

    if(part11_std_files->usermaclib) {
	
        nfiles = 0;
        sprintf(file,"%s/%s",infile, "maclib");
        getFilenames(file, "", files, &nfiles, MAXSTR);
        for(i=0; i<nfiles; i++) {
            sprintf( &origpath[ 0 ], "%s/%s", file, files[i]);
            sprintf( &destpath[ 0 ], "%s%s/%s", dest, "maclib", files[i]);
            if(fileExist(origpath)) safecp_file(safecpPath, origpath, destpath);
        }
    }

    if(part11_std_files->shims) {
        nfiles = 0;
        sprintf(file,"%s/%s",infile, "shims");
        getFilenames(file, "", files, &nfiles, MAXSTR);
        for(i=0; i<nfiles; i++) {
            sprintf( &origpath[ 0 ], "%s/%s", file, files[i]);
            sprintf( &destpath[ 0 ], "%s%s/%s", dest, "shims", files[i]);
            if(fileExist(origpath)) safecp_file(safecpPath, origpath, destpath);
        }
    }

    if(part11_std_files->waveforms) {
        nfiles = 0;
        sprintf(file,"%s/%s",infile, "shapelib");
        getFilenames(file, "", files, &nfiles, MAXSTR);
        for(i=0; i<nfiles; i++) {
            sprintf( &origpath[ 0 ], "%s/%s", file, files[i]);
            sprintf( &destpath[ 0 ], "%s%s/%s", dest, "shapelib", files[i]);
            if(fileExist(origpath)) safecp_file(safecpPath, origpath, destpath);
        }
    }

    if(part11_std_files->pulseSequence) {
        nfiles = 0;
        sprintf(file,"%s/%s",infile, "psglib");
        getFilenames(file, "", files, &nfiles, MAXSTR);
        for(i=0; i<nfiles; i++) {
            sprintf( &origpath[ 0 ], "%s/%s", file, files[i]);
            sprintf( &destpath[ 0 ], "%s%s/%s", dest, "psglib", files[i]);
            if(fileExist(origpath)) safecp_file(safecpPath, origpath, destpath);
        }
    }

    if(part11_std_files->cmdHistory) {
        sprintf( &origpath[ 0 ], "%s%s", orig, "cmdHistory");
        sprintf( &destpath[ 0 ], "%s%s", dest, "cmdHistory");
        if(fileExist(origpath)) safecp_file(safecpPath, origpath, destpath);
    }

    if(incInc) {
        nfiles = 0;
        getFilenames(incPath, "", files, &nfiles, MAXSTR);
        for(i=0; i<nfiles; i++) {
            sprintf( &origpath[ 0 ], "%s%s", incPath, files[i]);
            sprintf( &destpath[ 0 ], "%s%s/%s", dest, "incFiles", files[i]);
            if(fileExist(origpath)) safecp_file(safecpPath, origpath, destpath);
        }
    }

#ifdef VNMRJ
    writeLineForLoc(dest, "vnmr_rec_data");
    if(debug)
        Winfoprintf("part11: record is saved as %s\n", dest);	
#endif 

    if(mode_of_vnmr != AUTOMATION) Winfoprintf("%s is saved.\n", dest);

    if(part11System && strstr(dest,".REC") != NULL) {

        sprintf( &origpath[ 0 ], "%s%s", orig, "auditTrail");
        sprintf( &destpath[ 0 ], "%s%s", dest, "auditTrail");

/* write audit trail to system/auditTrail and curexpdir/acqfil/auditTrail */
        writeAuditTrails("save_acqfil", orig, dest, orig); 

/* copy audit trail */
        if(fileExist(origpath)) safecp_file(safecpPath, origpath, destpath);

        makeChecksums(dest,"checksum");

        if(strstr(safecpPath,"/p11/bin/safecp") == NULL) {
            sprintf(str, "chmod -R g+w %s", dest);
            system(str);
        }
    }
    RETURN;
}

char *get_p11_id()
{
     if (part11System == NONFDA || !showDataID)
         return NULL;
     return p11Id;
}

/**************************/
void p11_displayDataID(char* str)
/**************************/
{
    string dataid, cmd;

    strcpy(winCmd, str);

    if(!part11System || !showDataID || !strStartsWith(str, "d")) return;

    if(debug) fprintf(stderr, "p11_displayDataID %s\n", str);

    if(strStartsWith(str,"df"))
    { 
   	if(!P_getstring(CURRENT, "fidid", dataid, 1, MAXSTR))
        {
            sprintf(cmd, "%s%s%s","write('graphics',wc*0.5,wc2,'", dataid, "')\n");
            sprintf(p11Id, "%s", dataid);
            execString(cmd);
        }
    }
    else if(strStartsWith(str,"ds") || strStartsWith(str,"dcon"))
    {
   	if(!P_getstring(CURRENT, "fidid", dataid, 1, MAXSTR)
   	 && !P_getstring(CURRENT, "dataid", dataid, 1, MAXSTR))
        {
            sprintf(cmd, "%s%s%s","write('graphics',wc*0.8,wc2,'", dataid, "')\n");
            sprintf(p11Id, "%s", dataid);
            execString(cmd);
        }
    }
}

/**************************/
static void save_datdir(char* orig, char* dest)
/**************************/
{
/* copy processed data */
/* the same as in save_acqfil: write and copy audit trail */
/* copy the rest files */

    char path1[MAXPATH], path2[MAXPATH];
    char file[MAXPATH], safecpPath[MAXPATH];
    string str, files[MAXSTR];
    int i, nfiles, sec;
    string cdumpCmd;
    FILE *fp = NULL;
    char *strptr;
/*
  redrawCanvas();
  p11_displayDataID(winCmd);
*/
    if(fileExist(dest)) {
	Werrprintf("cannot save: %s already exists.\n",dest); 
	return;
    }

    if(part11_std_files == NULL) return; 

    // set dataid to reflect datdirxxx
    strptr = dest + (strlen(dest)-9);
    if(strstr(strptr,"datdir") == strptr) {
	char fidid[MAXSTR], cmd[16];
        if(!P_getstring(CURRENT, "fidid", fidid, 1, MAXSTR) && strlen(fidid)>8) {
	  i = atoi(strptr+6);
          setDataID(fidid,i);
          Wgetgraphicsdisplay(cmd, 16);
          if(strstr(cmd,"d") == cmd) {
	     strcat(cmd,"\n");
	     execString(cmd); 
	  }  
	}
    }

    if(!strEndsWith(orig, "/")) strcat(orig, "/");
    if(!strEndsWith(dest, "/")) strcat(dest, "/");

    flush(0,NULL,0,NULL);
    sprintf(file, "%s%s", orig, "spec.jpeg");
    if(fileExist(file)) unlink(file);

    if(!strStartsWith(winCmd, "df") && strStartsWith(winCmd, "d")) {
        sprintf(cdumpCmd, "%s%s%s", "vnmrjcmd('GRAPHICS', 'cdump', '", orig, "spec', 'jpeg')\n");
        execString(cdumpCmd);
    }
    save_recordInfo();

    setSafecpPath(safecpPath, dest);

    if(part11_std_files->procpar) {
        sprintf(path1, "%s%s", orig, "procpar");
        sprintf(path2, "%s%s", dest, "procpar");
	saveProcpar(path1);
        if(fileExist(path1)) safecp_file(safecpPath, path1, path2);

        /* save display params */
        sprintf(path1, "%s%s", orig, "curpar");
        sprintf(path2, "%s%s", dest, "curpar");
        saveCurpar(path1);
        if(fileExist(path1)) safecp_file(safecpPath, path1, path2);
    }
   
    if(part11_std_files->phasefile) {
        sprintf(path1, "%s%s", orig, "phasefile");
        sprintf(path2, "%s%s", dest, "phasefile");
        if(fileExist(path1)) safecp_file(safecpPath, path1, path2);
    }
  
    if(part11_std_files->fdf) {
        nfiles = 0;
        sprintf(file,"%s/%s",curexpdir, "recon");
        if(fileExist(file)) {
            getFilenames(file, "", files, &nfiles, MAXSTR);
            for(i=0; i<nfiles; i++) {
                sprintf( path1, "%s/%s", file, files[i]);
                sprintf( path2, "%s%s/%s", dest, "recon", files[i]);
                if(fileExist(path1)) safecp_file(safecpPath, path1, path2);
            }
        }
    }
   
    if(part11_std_files->data) {
        sprintf(path1, "%s%s", orig, "data");
        sprintf(path2, "%s%s", dest, "data");
        if(fileExist(path1)) safecp_file(safecpPath, path1, path2);
    }
   
    if(part11_std_files->cmdHistory) {
    	if(cmdHistoryFp != NULL) fflush(cmdHistoryFp);
        sprintf(path2, "%s%s", dest, "cmdHistory");
        if(fileExist(cmdHisFile)) safecp_file(safecpPath, cmdHisFile, path2);
    }
   
    if(part11_std_files->text) {
        sprintf(path1, "%s/%s", curexpdir, "text");
        sprintf(path2, "%s%s", dest, "text");
        if(fileExist(path1)) safecp_file(safecpPath, path1, path2);
    }
   
    if(part11_std_files->global) {
        sprintf(path1, "%s%s", orig, "global");
        sprintf(path2, "%s%s", dest, "global");
        if(fileExist(path1)) unlink(path1); 
	P_save(GLOBAL,path1);
        if(fileExist(path1)) safecp_file(safecpPath, path1, path2);
    }
   
    if(part11_std_files->conpar) {
        sprintf(path1, "%s%s", orig, "conpar");
        sprintf(path2, "%s%s", dest, "conpar");
        if(fileExist(path1)) unlink(path1); 
	P_save(SYSTEMGLOBAL,path1);
        if(fileExist(path1)) safecp_file(safecpPath, path1, path2);
    }
   
    if(part11_std_files->usermaclib) {
        nfiles = 0;
        sprintf(file,"%s/%s",curexpdir, "maclib");
        getFilenames(file, "", files, &nfiles, MAXSTR);
        for(i=0; i<nfiles; i++) {
            sprintf( path1, "%s/%s", file, files[i]);
            sprintf( path2, "%s%s/%s", dest, "maclib", files[i]);
            if(fileExist(path1)) safecp_file(safecpPath, path1, path2);
        }
    }

    if(incInc) {
        nfiles = 0;
        getFilenames(incPath, "", files, &nfiles, MAXSTR);
        for(i=0; i<nfiles; i++) {
            sprintf( path1, "%s%s", incPath, files[i]);
            sprintf( path2, "%s%s/%s", dest, "incFiles", files[i]);
            if(fileExist(path1)) safecp_file(safecpPath, path1, path2);
        }
    }

    if(part11_std_files->snapshot && mode_of_vnmr == AUTOMATION) {
        nfiles = 0;
        getFilenames(orig, "", files, &nfiles, MAXSTR);
        for(i=0; i<nfiles; i++) if(strncmp(files[i], "spec.", 5) == 0) {
            sprintf( path1, "%s%s", orig, files[i]);
            sprintf( path2, "%s%s", dest, files[i]);
            if(fileExist(path1)) safecp_file(safecpPath, path1, path2);
        }
    } else if(part11_std_files->snapshot &&
              !strStartsWith(winCmd, "df") && strStartsWith(winCmd, "d")) {
        sprintf(path1, "%s%s", orig, "spec.jpeg");
        sprintf(path2, "%s%s", dest, "spec.jpeg");
        sec = 0;
        while(sec < 5 && !fileExist(path1)) {
            sleep(1);
            sec++;
        }
        while(fileExist(path1) && !(fp = fopen(path1, "r"))) {
            sleep(1);
        }
        if(fileExist(path1) && fp) {
            safecp_file(safecpPath, path1, path2);
            fclose(fp);
        }
    }
#ifdef VNMRJ
    writeLineForLoc(dest, "vnmr_rec_data");
    if(debug)
        Winfoprintf("part11: record is saved as %s\n", dest);	
#endif 

    if(mode_of_vnmr != AUTOMATION) Winfoprintf("%s is saved.\n", dest);

    if(part11System && strstr(dest,".REC") != NULL) {
        writeAuditTrails("save_datdir", orig, dest, "datdir"); 

        sprintf(path1, "%s%s", orig, "auditTrail");
        sprintf(path2, "%s%s", dest, "auditTrail");
        if(fileExist(path1)) safecp_file(safecpPath, path1, path2);

    
        makeChecksums(dest, "checksum");
        if(strstr(safecpPath,"/p11/bin/safecp") == NULL) {
            sprintf(str, "chmod -R g+w %s", dest);
            system(str);
        }

    }
}

/**************************/
time_t getTimeSaved(string path) 
/**************************/
{
    struct stat     stat_path;
    int i;

    i = stat(path, &stat_path );
    if(i == 0) 
	return stat_path.st_mtime;
    else return -1; 
}

/******************/
void getFilenames_1level(string rootpath, string name, string* files, int* n, int nmax)
/******************/
{
/* get file and dir names in rootpath/name. */
/* rootpath or name may be empty. search only one level. */
/* n is number of files and dirs. files contains name/... */ 

    DIR             *dirp;
    struct dirent   *dp;
    string dir, child;

    if(rootpath[strlen(rootpath)-1] != '/')
      sprintf(dir, "%s/%s", rootpath, name);
    else 
      sprintf(dir, "%s%s", rootpath, name);

    if(strlen(dir) == 0 || !fileExist(dir)) return;

    if((*n) >= (nmax-1)) {
	return;
    } else if(!isAdirectory(dir)) {
	return;
    } else {
        if ( (dirp = opendir(dir)) ) {
            if(strlen(name) > 0 && name[strlen(name)-1] != '/')
                strcat(name,"/");
            for (dp = readdir(dirp); dp != NULL && (*n) < (nmax-1); dp = readdir(dirp)) {

                if(debug)
                    fprintf(stderr," dir %s %s\n", dir, dp->d_name);
                if (*dp->d_name != '.') {

                    sprintf(child,"%s%s",name,dp->d_name);
        	    strcpy(files[*n], child);
        	    (*n)++;	
                }
            }

            closedir(dirp);
        }
    }
    return;
}

/**************************/
static int getLastVersion(char* path, char* name)
/**************************/
{
    int i, n;
    time_t t, last = 0;

    string file, files[MAXSTR];
    int nfiles = 0;

    getFilenames_1level(path, "", files, &nfiles, MAXSTR);

    n = -1;
    for(i=0; i<nfiles; i++) {
	sprintf(file, "%s/%s", path, files[i]);
        if(isAdirectory(file) && (strstr(files[i], ".dat") != NULL ||
                                  strstr(files[i], "datdir") != NULL)) {
            t = getTimeSaved(file); 
            if(t > last) {
		last = t;
		n = i;
            }
	}
    }

    if(n != -1) strcpy(name,files[n]);
    else strcpy(name, "");
    return(n + 1);
}

/**************************/
int getLastData(int argc, char *argv[], int retc, char *retv[])
/**************************/
{
    string path, name;
    int i;

    if (argc > 1) {

	strcpy(path, argv[1]);

	i = getLastVersion(path, name);

        if (retc > 0) retv[0]= newString(name);

    } else if (retc > 0) retv[0]= newString(""); 

    RETURN;
}

#ifdef XXX
/**************************/
static int getLastVersionNum(char* path, char* prefix, char* version)
/**************************/
/* get next version of a dir/file with given path and prefix */
/* by appending version number (001-999) to prefix and see */
/* whether dir/file exists. */
{
    int i, n, max = 999;
    char file[MAXSTR], last[MAXSTR];

    strcpy(version, "");
    for(i=1; i<max; i++) {
        strcpy(last, version);
   	if(i > 99) sprintf(version, "%d",i);
   	else if(i > 9) sprintf(version, "0%d",i);
   	else sprintf(version, "00%d",i);
        
	sprintf(file, "%s%s%s", path, prefix, version); 
	
	if(!fileExist(file)) break;
    }

    strcpy(version, last);
    return(i-1);
}
#endif

/**************************/
static int getNextVersion(char* path, char* prefix, char* version)
/**************************/
/* get next version of a dir/file with given path and prefix */
/* by appending version number (001-999) to prefix and see */
/* whether dir/file exists. */
{
    int i, max = 999;
    char file[MAXSTR];

    for(i=1; i<max; i++) {
   	if(i > 99) sprintf(version, "%d",i);
   	else if(i > 9) sprintf(version, "0%d",i);
   	else sprintf(version, "00%d",i);
        
	sprintf(file, "%s%s%s", path, prefix, version); 
	
	if(!fileExist(file)) {
	    strcat(file, ".dat");
	    if(!fileExist(file)) break;
	}
    }

    return(i);
}

/**************************/
int link_file(char* path, char* newpath)
/**************************/
/* try hard link. if failed try sift link */
{
    if (!access(path,F_OK)) {  
/* don't try hard link
   if (link(path,newpath)) {  
*/
        if (symlink(path,newpath)) {
            Werrprintf("cannot link file %s\n", path);
            ABORT;
        }
/*
  }
*/
    }

    RETURN;
}

/**************************/
static int copy_fid(char* curr, char* rec)
/**************************/
{
 
    int ival = 1;
    char path[MAXSTR], recpath[MAXSTR];

    if(!strEndsWith(curr, "/")) strcat(curr, "/");
    if(!strEndsWith(rec, "/")) strcat(rec, "/");

    sprintf(recpath, "%s%s", rec, "fid");
    sprintf(path, "%s%s", curr, "fid");
    if(fileExist(recpath)) {
        unlink(path);
	ival = copy_file(recpath, path);

     	sprintf(recpath, "%s%s", rec, "log");
     	sprintf(path, "%s%s", curr, "log");
    	unlink(path);
     	if(fileExist(recpath)) {
	    copy_file(recpath, path);
     	}

    }
    
    return(ival);
}

/**************************/
static int link_fid(char* curr, char* rec)
/**************************/
{
/* after acqfil is saved, remove fid and log in acqfil, and make */
/* link to the saved recsuffix/acqfil. */
/* if linked, unlink. but according the man unlink, rm is safer. */
/* return 0 if fid is successfully linked, otherwise return 1. */
 
    int ival = 1;
    char path[MAXSTR], recpath[MAXSTR];

    if(!strEndsWith(curr, "/")) strcat(curr, "/");
    if(!strEndsWith(rec, "/")) strcat(rec, "/");

    sprintf(recpath, "%s%s", rec, "fid");
    sprintf(path, "%s%s", curr, "fid");
    if(fileExist(recpath)) {
        unlink(path);
	ival = link_file(recpath, path);

        sprintf(recpath, "%s%s", rec, "log");
        sprintf(path, "%s%s", curr, "log");
    	unlink(path);
        if(fileExist(recpath)) {
	    link_file(recpath, path);
     	}
    }

    return(ival);
}

/**************************/
static int copy_acqfil(char* curr, char* rec)
/**************************/
/* called in rt_FDA */
{
    char path[MAXSTR], recpath[MAXSTR];

    if(!strEndsWith(curr, "/")) strcat(curr, "/");
    if(!strEndsWith(rec, "/")) strcat(rec, "/");

    sprintf(path, "%s%s", curr, "text");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "procpar");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "global");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "conpar");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curexpdir, "/maclib");
    if(fileExist(path)) rm_file(path);
    sprintf(path, "%s%s", curexpdir, "/shims");
    if(fileExist(path)) rm_file(path);
    sprintf(path, "%s%s", curexpdir, "/shapelib");
    if(fileExist(path)) rm_file(path);
    sprintf(path, "%s%s", curexpdir, "/psglib");
    if(fileExist(path)) rm_file(path);
    sprintf(path, "%s%s", curexpdir, "/incFiles");
    if(fileExist(path)) rm_file(path);
    sprintf(path, "%s%s", curr, "cmdHistory");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "auditTrail");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "checksum");
    if(fileExist(path)) unlink(path);

    /* also remove datdir files */
    sprintf(path, "%s/datdir/procpar", curexpdir);
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s/datdir/curpar", curexpdir);
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s/datdir/cmdHistory", curexpdir);
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s/datdir/auditTrail", curexpdir);
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s/datdir/checksum", curexpdir);
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s/datdir/text", curexpdir);
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s/datdir/global", curexpdir);
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s/datdir/conpar", curexpdir);
    if(fileExist(path)) unlink(path);

    sprintf(recpath, "%s%s", rec, "text");
    sprintf(path, "%s%s", curr, "text");
    if(fileExist(recpath)) copy_file(recpath, path);

    sprintf(recpath, "%s%s", rec, "procpar");
    sprintf(path, "%s%s", curr, "procpar");
    if(fileExist(recpath)) copy_file(recpath, path);

    sprintf(recpath, "%s%s", rec, "curpar");
    sprintf(path, "%s%s", curr, "curpar");
    if(fileExist(recpath)) copy_file(recpath, path);

    sprintf(recpath, "%s%s", rec, "global");
    sprintf(path, "%s%s", curr, "global");
    if(fileExist(recpath)) copy_file(recpath, path);
   
    sprintf(recpath, "%s%s", rec, "conpar");
    sprintf(path, "%s%s", curr, "conpar");
    if(fileExist(recpath)) copy_file(recpath, path);
   
    sprintf(recpath, "%s%s", rec, "shims");
    sprintf(path, "%s%s", curexpdir, "/shims");
    if(fileExist(recpath)) copy_file(recpath, path);
   
    sprintf(recpath, "%s%s", rec, "shapelib");
    sprintf(path, "%s%s", curexpdir, "/shapelib");
    if(fileExist(recpath)) copy_file(recpath, path);
    
    sprintf(recpath, "%s%s", rec, "psglib");
    sprintf(path, "%s%s", curexpdir, "/psglib");
    if(fileExist(recpath)) copy_file(recpath, path);
    
    sprintf(recpath, "%s%s", rec, "maclib");
    sprintf(path, "%s%s", curexpdir, "/maclib");
    if(fileExist(recpath)) copy_file(recpath, path);
    
    sprintf(recpath, "%s%s", rec, "incFiles");
    sprintf(path, "%s%s", curexpdir, "/incFiles");
    if(fileExist(recpath)) copy_file(recpath, path);
    
    sprintf(recpath, "%s%s", rec, "cmdHistory");
    sprintf(path, "%s%s", curr, "cmdHistory");
    if(fileExist(recpath)) copy_file(recpath, path);

    if(part11System) {

        sprintf(recpath, "%s%s", rec, "auditTrail");
        sprintf(path, "%s%s", curr, "auditTrail");
        if(fileExist(recpath)) copy_file(recpath, path);

        writeAuditTrails("copy_acqfil", rec, curr, "acqfil");

        sprintf(recpath, "%s%s", rec, "checksum");
        sprintf(path, "%s%s", curr, "checksum");
        if(fileExist(recpath)) copy_file(recpath, path);
    }

    RETURN;
}

/**************************/
int append_file(char* file1, char* file2)
/**************************/
/* append file1 to file2 */
{
    int f1, f2, n;
    char buf[BUFSIZE];

    if( (f1 = open(file1, O_RDONLY, 0)) == -1) {
        fprintf(stderr, "vnmr svr: cannot open %s to read\n", file1);
        ABORT;
    }
    if( (f2 = open(file2, O_WRONLY | O_APPEND, 0)) == -1) {
        fprintf(stderr, "vnmr svr: cannot open %s to write.\n", file2);
        close(f1);
        ABORT;
    }

    while((n = read(f1, buf, BUFSIZE)) > 0)
        if(write(f2, buf, n) != n) {
            fprintf(stderr, "vnmr srv: write error on file %s.\n", file2);
            ABORT;
        }

    close(f1);
    close(f2);

    RETURN;
}

/**************************/
int p11_saveAuditTrail()
/**************************/
/* called in bootup when exit. */
{
    char path[MAXPATH], exitTime[MAXSTR];
    char safecpPath[MAXPATH];

    if(sessionAuditTrailFp == NULL || mode_of_vnmr == BACKGROUND) {
      if(cmdHistoryFp != NULL) fclose(cmdHistoryFp);
      cmdHistoryFp = NULL;
      RETURN;
    }

    if(sessionAuditTrailFp)
        fclose(sessionAuditTrailFp);
    sessionAuditTrailFp = NULL;
    execString("p11write('savesession')\n");
    if (VnmrJViewId != 1)
    {
       if(cmdHistoryFp != NULL) fclose(cmdHistoryFp);
       cmdHistoryFp = NULL;
       RETURN;
    }

    currentDate(exitTime, MAXSTR);

    sprintf(path, "%s%s%s^%s^%s^%s:%d.vaudit", auditDir, "sessions/", bootupTime, exitTime, 
            p11console, operatorID, nlogin); 

    if(fileExist(auditTrail)) {

   	setSafecpPath(safecpPath, path);

	safecp_file(safecpPath, auditTrail, path);
        unlink(auditTrail);
    }
    if(cmdHistoryFp != NULL) fclose(cmdHistoryFp);
 
    RETURN;
}

/**************************/
int p11_writeAuditTrails_S(char* func, char* origpath, char* destpath)
/**************************/
{
/* write session wise audit trail */

    if(sessionAuditTrailFp != NULL) { 
        currentDate(cmdTime, MAXSTR);
        fprintf(sessionAuditTrailFp, "%s audit: %s %s", cmdTime, func, currentCmd); 
        fprintf(sessionAuditTrailFp, "                     orig path: %s\n", origpath); 
        fprintf(sessionAuditTrailFp, "                     dest path: %s\n", destpath); 
    }
    RETURN;
}

/**************************/
int p11_writeAuditTrails_D(char* func, char* origpath, char* destpath, char* subdir)
/**************************/
{
/* write record based audit trail in curexpdir/subdir/auditTrail. */
/* subdir is either acqfil or datdir. */

    char file[MAXSTR];

    if(strcmp(subdir, "") == 0) return(-1);

    if(subdir[0] == '/') 
        sprintf(file, "%s/auditTrail", subdir);
    else
        sprintf(file, "%s/%s/auditTrail", curexpdir, subdir);

    if(fileExist(file)) dataAuditTrailFp = fopen(file, "a");
    else dataAuditTrailFp = fopen(file, "w");
    if(dataAuditTrailFp != NULL) { 
        currentDate(cmdTime, MAXSTR);
        fprintf(dataAuditTrailFp, "%s audit: %s %s %s %s", cmdTime, 
                p11console, operatorID, func, currentCmd); 
        fprintf(dataAuditTrailFp, "                     orig path: %s\n", origpath); 
        fprintf(dataAuditTrailFp, "                     dest path: %s\n", destpath); 
    }

    if(dataAuditTrailFp)
        fclose(dataAuditTrailFp);

    RETURN;
}

/**************************/
int writeAuditTrails(char* func, char* origpath, char* destpath, char* subdir)
/**************************/
{
    p11_writeAuditTrails_S(func, origpath, destpath); 
    p11_writeAuditTrails_D(func, origpath, destpath, subdir); 
    RETURN;
}

void init_dataid() {

    P_setstring(CURRENT, "fidid", "", 1);
    P_setstring(CURRENT, "dataid", "", 1);
    P_setreal(CURRENT, "processid", (double)0, 1);
}

/**************************/
void p11_init_acqfil(char* func, char* orig, char* dest)
/**************************/
/* called in rt, but not rt_FDA. orig is the data path. */
{
    char file[MAXSTR];

    okFDAdata = 0;
#ifdef VNMRJ
/*
  writelineToVnmrJ("pnew", "1 file");
*/
#endif 

    sprintf(file, "%s/curpar", dest);
    if(fileExist(file)) unlink(file);
    sprintf(file, "%s/global", dest);
    if(fileExist(file)) unlink(file);
    sprintf(file, "%s/conpar", dest);
    if(fileExist(file)) unlink(file);
    sprintf(file, "%s%s", curexpdir, "/maclib");
    if(fileExist(file)) rm_file(file);
    sprintf(file, "%s%s", curexpdir, "/shims");
    if(fileExist(file)) rm_file(file);
    sprintf(file, "%s%s", curexpdir, "/shapelib");
    if(fileExist(file)) rm_file(file);
    sprintf(file, "%s%s", curexpdir, "/psglib");
    if(fileExist(file)) rm_file(file);
    sprintf(file, "%s%s", curexpdir, "/incFiles");
    if(fileExist(file)) rm_file(file);
    sprintf(file, "%s/cmdHistory", dest);
    if(fileExist(file)) unlink(file);
    sprintf(file, "%s/auditTrail", dest);
    if(fileExist(file)) unlink(file);
    sprintf(file, "%s/checksum", dest);
    if(fileExist(file)) unlink(file);

    /* also remove datdir files */
    sprintf(file, "%s/datdir/procpar", curexpdir);
    if(fileExist(file)) unlink(file);
    sprintf(file, "%s/datdir/curpar", curexpdir);
    if(fileExist(file)) unlink(file);
    sprintf(file, "%s/datdir/cmdHistory", curexpdir);
    if(fileExist(file)) unlink(file);
    sprintf(file, "%s/datdir/auditTrail", curexpdir);
    if(fileExist(file)) unlink(file);
    sprintf(file, "%s/datdir/checksum", curexpdir);
    if(fileExist(file)) unlink(file);
    sprintf(file, "%s/datdir/text", curexpdir);
    if(fileExist(file)) unlink(file);
    sprintf(file, "%s/datdir/global", curexpdir);
    if(fileExist(file)) unlink(file);
    sprintf(file, "%s/datdir/conpar", curexpdir);
    if(fileExist(file)) unlink(file);

    if(!part11System) return;

    writeAuditTrails(func, orig, dest, dest); 
}

/**************************/
int p11_saveFDAfiles_raw(char* func, char* orig, char* dest)
/**************************/
/* called in go. */
/* orig is empty, dest is where the fid will be */
/* dest is curexpdir/acqfil for non-automation */
/* and auto dir for automation */
{
    char destpath[MAXPATH];
    double d;
    string str;

    if(strstr(currentCmd,"recordOff") != NULL) RETURN;

    if(!part11System && !P_getreal(GLOBAL, "recordSave", &d, 1) && d < 0.5) RETURN;

    if(P_getstring(GLOBAL, "acqmode", str, 1, MAXSTR)) strcpy(str, "");
    if(strcmp(str, "fidscan") == 0 || strcmp(str, "lock") == 0) RETURN;

    if(part11_std_files == NULL) RETURN; 

    createPart11currentpars();

    P_setstring(CURRENT,"acqpath",dest,1);
    //P_setstring(CURRENT,"file",dest,1);

    p11_init_acqfil(func, orig, dest);

    init_dataid();

    readUserPart11Config();

    if(part11_std_files->cmdHistory) {

	if(cmdHistoryFp != NULL) fflush(cmdHistoryFp);
	sprintf(destpath, "%s%s", dest, "/cmdHistory");

	if(fileExist(destpath)) unlink(destpath);
	if(fileExist(cmdHisFile)) copy_file(cmdHisFile, destpath);
    } 

    if(part11_std_files->global) {
	sprintf(destpath, "%s%s", dest, "/global");
	if (P_save(GLOBAL,destpath))
        {   Werrprintf("Problem saving global parameters in '%s'.",destpath);
            ABORT;
        } 
    }

    if(part11_std_files->conpar) {
	sprintf(destpath, "%s%s", dest, "/conpar");
	if (P_save(SYSTEMGLOBAL,destpath))
        {   Werrprintf("Problem saving conpar parameters in '%s'.",destpath);
            ABORT;
        } 
    }

/* macro prepRecFiles saves the following files in curexpdir */ 
    if(part11_std_files->shims) {
	execString("prepRecFiles('shims')\n");
    }

    if(part11_std_files->waveforms) {
	execString("prepRecFiles('waveforms')\n");
    }

    if(part11_std_files->pulseSequence) {
	execString("prepRecFiles('pulseSequence')\n");
    }

    okFDAdata = 1;
    RETURN;
}

/**************************/
void writeChecksum(FILE* fp, char* rootpath, char* file, int nfiles)
/**************************/
{
    char sum[MAXSTR];
    string filename;

    sprintf(filename, "%s%s", rootpath, file);
    if(debug)fprintf(stderr, "writeChecksum %s\n", filename);
    if(getChecksum(filename, sum)) {

// the following lines modify sum to make it non-standard so it 
// cannot be regenerated by santard md5 routine. 
// Comment out the following 8 lines to use standard md5.
	char str[MAXSTR];
	int last;
        if(isdigit(sum[strlen(sum)-1])) {
            sprintf(str,"%c",sum[strlen(sum)-1]);
            last = atoi(str);
        } else last = 0;

        sprintf(str, "%d", last+nfiles);
        strcat(sum, str);

        fprintf(fp, "sum:%s:%s\n",file, sum);
    }
}

/**************************/
int makeChksums(char* path, char *checksumfile, string* files, int nfiles)
/**************************/
{
    FILE* fp;
    char sumfile[MAXSTR], file[MAXSTR], curTime[MAXSTR];
    int i;
    string rootpath, safecpPath;

    currentDate(curTime, MAXSTR);

    strcpy(rootpath,path);

    if(!strEndsWith(rootpath, "/")) strcat(rootpath, "/");

    p11TempName(sumfile);

    if(!(fp = fopen(sumfile, "w"))) ABORT;

    fprintf(fp, "time stamp: %s\n",curTime);
    fprintf(fp, "operator full name:%s\n",operatorFullName);
    fprintf(fp, "operator ID:%s\n",operatorID);
    fprintf(fp, "software version:%s\n",softwareVersion);
    fprintf(fp, "console:%s\n",p11console);
    fprintf(fp, "rootpath:%s\n",rootpath);
    fprintf(fp, "n_files:%d\n",nfiles);

    for(i=0; i<nfiles; i++) {
	writeChecksum(fp, rootpath, files[i], nfiles);
    }
    if(fp)
        fclose(fp);

    // copy checksum 

    setSafecpPath(safecpPath, rootpath);
    sprintf(file, "%s%s", rootpath, checksumfile);
    if(fileExist(sumfile)) safecp_file(safecpPath, sumfile, file);

    unlink(sumfile);

    RETURN;
}

/**************************/
int makeChecksums(char *rootpath, char *checksumFile)
/**************************/
/* call this when fid acquisition is finished */
/* or datdir is saved */
/* assuming checksum file already initialized */
{
    char files[MAXFILES][MAXSTR];
    int nfiles = 0;

     getFilenames(rootpath, "", files, &nfiles, MAXFILES);
    return makeChksums(rootpath, checksumFile, files, nfiles);
}

// this will make a checksum file of default name,
// containing checksums of the file(s) in the path.
// if a directory path is given, files will be searched recursively.
int mkchecksums(char *path) 
{
    int b = 0;
    string sumpath;
    if(isAdirectory(path)) { // use checksum file 
       strcpy(sumpath,path);
       if(path[strlen(path)-1] != '/') strcat(sumpath,"/");
       strcat(sumpath,"checksum");
       if(fileExist(sumpath)) {
	  Winfoprintf("Cannot generate checksum: %s already exists\n",sumpath); 
	  return(0);
       } else b = makeChecksums(path, "checksum");
    } else { // use .filename
       string dir,name,sumName;
       string files[1];
       brkPath(path, dir,name);
       strcpy(files[0],name);
       strcpy(sumName,"checksum_");
       strcat(sumName,name);
       strcpy(sumpath,dir);
       strcat(sumpath,sumName);
       if(fileExist(sumpath)) {
	  Winfoprintf("Cannot generate checksum: %s already exists\n",sumpath); 
	  return(0);
       } else b = makeChksums(dir, sumName, files, 1);
    }

    if(part11System) 
       p11_writeAuditTrails_S("write checksum file", path, "");
    return(b);
}

// this will make a checksum file of given name checksumFile,
// containing checksums of the file(s) in the path.
int mkchecksums2(char *path, char *checksumFile) 
{
    int b = 0;
    string sumpath;
    strcpy(sumpath,path);
    if(path[strlen(path)-1] != '/') strcat(sumpath,"/");
    strcat(sumpath,checksumFile);
    if(fileExist(sumpath)) {
       Winfoprintf("Cannot generate checksum: %s already exists\n",sumpath); 
       return(0);
    } 

    if(isAdirectory(path)) {
       b = makeChecksums(path, checksumFile);
    } else {
       string dir,name;
       string files[1];
       brkPath(path, dir,name);
       strcpy(files[0],name);
       b = makeChksums(dir, checksumFile, files, 1);
    }
   
    if(part11System) 
       p11_writeAuditTrails_S("write checksum file", path, "");
    return(b);
}

void p11_mkchecksums(char *path) {
   if(part11System) mkchecksums(path);
}

int mkchsums(int argc, char *argv[], int retc, char *retv[])
{
    (void) argc;
    (void) argv;
    if (argc > 2) mkchecksums2(argv[1],argv[2]);
    else if (argc > 1) mkchecksums(argv[1]);
    else {
      Winfoprintf("Usage: mkchsums(path<,checksum file name>).");
    }
    RETURN; 
}

// Usage: chchsums(path)
// if path could be .REC/acis a acqfil, o.REC
/**************************/
int chchsums(int argc, char *argv[], int retc, char *retv[])
/**************************/
{
    (void) argc;
    (void) argv;
    (void) retc;
    (void) retv;
    int bad = 0;
    string path;
    int i;
    string cmd, line, cmdLine;
    FILE *tmpfp;
    string tmpfile;
    
    strcpy(cmd, "/vnmr/p11/bin/writeAaudit ");

    if (argc > 1) {

	strcpy(path, argv[1]);

        if(!isAdirectory(path)) { // see whether "checksum_" file exists
	  string dir, name, sumfile;
       	  brkPath(path, dir,name);
          if(name[0] != '.') {
	    strcpy(sumfile,dir);
	    strcat(sumfile,"checksum_");
	    strcat(sumfile,name);
	    if(fileExist(sumfile)) strcpy(path, sumfile);
	  }
        } else { // see whether /checksum file exists.
	  string sumfile;
          if(path[strlen(path)-1] != '/') sprintf(sumfile,"%s/%s",path,"checksum");
	  else sprintf(sumfile,"%s%s",path,"checksum");
	  if(fileExist(sumfile)) strcpy(path, sumfile);
 	}

        if(strEndsWith(path,"acqfil") ||
           strstr(path,"datdir") != NULL ||
           strEndsWith(path,".dat")) {
            Winfoprintf("part11: check record %s\n", path);
            bad = checkPart11Checksums0(path, stdout);
	} else if(strEndsWith(path,"/checksum") || 
                  strEndsWith(path,"/checksum/")) {
	    strcpy(tmpfile, "");
            strncat(tmpfile,path,strlen(path)-9);
	    bad = checkPart11Checksums0(tmpfile, stdout);
	} else if(!isAdirectory(path)) { // file contains checksum(s)
	    bad = checkChecksums("", path, stdout, "");
	} else {
   	    Winfoprintf("part11: check records in %s\n", path);
	    bad = checkPart11Checksums(path, NULL);
	}
	sprintf(line, "%s %s %s %s %s, %d %s", cmdTime, 
		"aaudit", UserName, "check records", path, bad, "corrupted");
	sprintf(cmdLine, "%s -l \"%s\"", cmd, line);
	system(cmdLine);

   	Winfoprintf("part11: check records completed in %s, %d %s\n", path, bad, "corrupted");
    } else {
        p11TempName(tmpfile);
    	tmpfp = fopen(tmpfile, "w");

        getUserPart11Dirs(UserName, userPart11Dirs, &nuserPart11Dirs);
        for(i=0; i<nuserPart11Dirs; i++) {
            if(!strEndsWith(userPart11Dirs[i],"/")) strcat(userPart11Dirs[i], "/");
            strcpy(path, userPart11Dirs[i]);
            Winfoprintf("part11: check records in %s\n", path);
            bad = checkPart11Checksums(path, NULL);
            sprintf(line, "%s %s %s %s %s, %d %s", cmdTime,
                    "aaudit", UserName, "check records", path, bad, "corrupted");
            if(tmpfp != NULL) fprintf(tmpfp, "%s\n", line);
            Winfoprintf("part11: check records completed in %s, %d %s\n",
                        path, bad, "corrupted");
        }
        if(tmpfp)
             fflush(tmpfp);
        sprintf(cmdLine, "%s -f %s", cmd, tmpfile);
        if(debug) fprintf(stderr, "%s -f %s", cmd, tmpfile);
        system(cmdLine);

        if(tmpfp)
    	    fclose(tmpfp);

	unlink(tmpfile);
    }

    if (retc > 0)
       retv[0]= (char *)realString((double)bad);

    RETURN;
}

/**************************/
int cpFilesInFile(int argc, char *argv[], int retc, char *retv[])
/**************************/
{
    FILE *fp;
    string cmd, path, dest;
    int nwords;
    char  buf[BUFSIZE];
    string words[MAXWORDS];
    string line, cmdLine;
    FILE *tmpfp;
    string tmpfile;

    (void) retc;
    (void) retv;
    if (argc > 1) {
        if(strstr(argv[1], "/") == NULL) {
	    sprintf(dest, "%s%s", userdir, "/p11");  
            if(!fileExist(dest)) mkdir(dest, 0777);
            sprintf(dest, "%s%s", userdir, "/p11/copies");
            if(!fileExist(dest)) mkdir(dest, 0777);
            sprintf(dest, "%s%s%s", userdir, "/p11/copies/", argv[1]);
	} else strcpy(dest, argv[1]);
    } else {
 	sprintf(dest, "%s%s", userdir, "/p11");	
	if(!fileExist(dest)) mkdir(dest, 0777);
 	sprintf(dest, "%s%s", userdir, "/p11/copies");	
	if(!fileExist(dest)) mkdir(dest, 0777);
    }

    p11TempName(tmpfile);
    tmpfp = fopen(tmpfile, "w");

    sprintf(path, "%s%s", userdir, auditTableSelection);
    if((fp = fopen(path, "r"))) { 
    	while(fgets(buf,sizeof(buf),fp)) {

            if(strlen(buf) > 1 && buf[0] != '#' && buf[0] != '%' && buf[0] != '@') {
                nwords = MAXWORDS;
                getStrValues(buf, words, &nwords, " "); 
                if(strcmp(words[0], "path") == 0) { 
                    sprintf(line, "%s %s %s %s %s to %s", cmdTime, 
                            "aaudit", UserName, "copied records", words[1], dest);
                    if(tmpfp != NULL) fprintf(tmpfp, "%s\n", line);
                    sprintf(cmd, "cp -rf %s %s", words[1], dest);
                    system(cmd);
                    Winfoprintf("part11: copy file %s to %s\n", words[1], dest);
                }
            }
	}
        if(tmpfp)
	    fflush(tmpfp);
    	strcpy(cmd,"/vnmr/p11/bin/writeAaudit -f ");
	if(debug) fprintf(stderr, "%s%s", cmd, tmpfile);
	sprintf(cmdLine, "%s%s", cmd, tmpfile);
        system(cmdLine);
    } 
    if(fp)
        fclose(fp);
    if(tmpfp)
        fclose(tmpfp);

    unlink(tmpfile);

    RETURN;
}

/**************************/
static int getProcdim()
/**************************/
{
    double d;
   
    if(!P_getreal(CURRENT, "procdim", &d, 1)) return((int)d);
    else return(0);
}

int getProcessid() {
    double d;
   
    if(!P_getreal(CURRENT, "processid", &d, 1)) return((int)d);
    else return(0);
}

/**************************/
void writeAudit_datdir(char* func, char* orig, char* dest)
/**************************/
/* called in ft, bc... when new data and phasefile are created */
/* orig and dest are curexpdir/acqfil, or curexpdir/datdir */
/* depending on the function (e.g., ft or bc...) that calls it */
{
    char ofile[MAXPATH], dfile[MAXPATH];
    char origpath[MAXSTR], destpath[MAXSTR];

    sprintf(origpath, "%s/%s", curexpdir, orig);
    sprintf(destpath, "%s/%s", curexpdir, dest);
    if(!strEndsWith(origpath, "/")) strcat(origpath, "/");
    if(!strEndsWith(destpath, "/")) strcat(destpath, "/");

/* restart auditTrail if the data is processed the first time. */

    if(getProcessid()>0) {
        sprintf(dfile, "%s/datdir/auditTrail", curexpdir);
        if(fileExist(dfile)) unlink(dfile);

        sprintf(ofile, "%s%s", origpath, "auditTrail");
        sprintf(dfile, "%s%s", destpath, "auditTrail");
    
        if(fileExist(dfile)) unlink(dfile);
        if(fileExist(ofile)) copy_file(ofile, dfile);
    }
    writeAuditTrails(func, origpath, destpath, "datdir"); 
}

/**************************/
void setDataID(string fidid, int processid)
/**************************/
{
    string dataid, str;

    if(strlen(fidid) < 1) return;
    if(processid < 0) sprintf(str, "00%dn", processid);
    else if(processid < 10) sprintf(str, "00%1d", processid);
    else if(processid < 100) sprintf(str, "0%2d", processid);
    else if(processid < 1000) sprintf(str, "%3d", processid);
    else sprintf(str, "%d", processid);

    sprintf(dataid, "%.8s%s", fidid, str);

    P_setreal(PROCESSED, "processid", processid, 1);
    P_copyvar(PROCESSED,CURRENT,"processid","processid");
    P_setstring(PROCESSED, "dataid", dataid, 1);
    P_copyvar(PROCESSED,CURRENT,"dataid","dataid");
}

/**************************/
int p11_saveFDAfiles_processed(char* func, char* orig, char* dest)
/**************************/
{
/* save datdir files */

    char ofile[MAXPATH], dfile[MAXPATH];
    double d;
    int i = 0;
    string fidid;
    string str;

    if(strstr(currentCmd,"recordOff") != NULL) RETURN;

    if(!part11System && !P_getreal(GLOBAL, "recordSave", &d, 1) && d < 0.5) RETURN;

    if(P_getstring(GLOBAL, "acqmode", str, 1, MAXSTR)) strcpy(str, "");
    if(strcmp(str, "fidscan") == 0 || strcmp(str, "lock") == 0) RETURN;

    if(getProcessid()==0) 
    	P_setreal(CURRENT, "processid", (double)1, 1);
/*
    if(!dataNewCmd || part11_std_files == NULL) RETURN;
    dataNewCmd = 0;

    if(strstr(currentCmd,"RT") != NULL || 
       strstr(currentCmd,"locaction('DoubleClick'") != NULL) {
	// do nothing
    } else {

        if(!P_getreal(CURRENT, "processid", &d, 1)) i = 1+(int)d;
        else i = 1;
        P_setreal(CURRENT, "processid", (double)i, 1);

        if(P_getstring(CURRENT, "fidid", fidid, 1, MAXSTR)) strcpy(fidid, "none");
        setDataID(fidid, i);
    }
*/

    if(debug) 
	Winfoprintf("******p11_saveFDAfiles_processed fidid processid %s %s %d\n",
		currentCmd, fidid, i);

    readUserPart11Config();

    if(part11_std_files->procpar) {
	sprintf(dfile, "%s%s", curexpdir, "/datdir/procpar");
	if(fileExist(dfile)) unlink(dfile);
	saveProcpar(dfile);
    }

    if(part11_std_files->global) {
	sprintf(dfile, "%s%s", curexpdir, "/datdir/global");
	if (P_save(GLOBAL,dfile))
        {   Werrprintf("Problem saving global parameters in '%s'.",dfile);
            ABORT;
        } 
    }

    if(part11_std_files->conpar) {
	sprintf(dfile, "%s%s", curexpdir, "/datdir/conpar");
	if (P_save(SYSTEMGLOBAL,dfile))
        {   Werrprintf("Problem saving conpar parameters in '%s'.",dfile);
            ABORT;
        } 
    }

    if(part11_std_files->text) {
    	sprintf(ofile, "%s/text", curexpdir);
    	if(fileExist(ofile)) {
            sprintf(dfile, "%s/datdir", curexpdir);
            copytext(curexpdir, dfile);
	}
    }

    if(part11_std_files->cmdHistory) {

    	if(cmdHistoryFp != NULL) fflush(cmdHistoryFp);

	sprintf(dfile, "%s%s", curexpdir, "/datdir/cmdHistory");

	if(fileExist(dfile)) unlink(dfile);
 	if(fileExist(cmdHisFile)) copy_file(cmdHisFile, dfile);
    } 

    if(!part11System) RETURN;
 
    writeAudit_datdir(func, orig, dest);
    RETURN;
}

static char *make_twoparam_command(char *command, char *param_a, char *param_b )
{
    char    *quick_str;
    int      len_param_a, len_param_b, quick_str_len,
        command_len;

    len_param_a = strlen( param_a );
    if (len_param_a < 1)
        return( NULL );
    len_param_b = strlen( param_b );
    if (len_param_b < 1)
        return( NULL );
    command_len = strlen( command );
    if (command_len < 1)
        return( NULL );

/* command length = length of paramater 1 + length of parameter 2
   + one character to separate the two parameters
   + length of command itself
   + one character to separate command from parameters
   + one character for the terminating null      */

    quick_str_len = len_param_a + len_param_b + strlen( command ) + 3;
    quick_str = (char*)allocateWithId( quick_str_len, "command" );
    if (quick_str == NULL)
        return( NULL );
    sprintf( quick_str, "%s %s %s", command, param_a, param_b );
    return( quick_str );
}

static int copy_file_verify2(char *file_a, char *file_b )
{
    char    *cp_cmd;
    int      ival;

#ifdef UNIX
    cp_cmd = make_twoparam_command( "cp -rf", file_a, file_b );
#else 
    cp_cmd = make_twoparam_command( "copy", file_a, file_b );
#endif 
    if (cp_cmd == NULL)
        return( -1 );
    system( cp_cmd );
    release( cp_cmd );

    ival = verify_copy( file_a, file_b );
    return( ival );
}

/**************************/
int copy_file(char* origpath, char* destpath)
/**************************/
/* standard cp */
{
    int r;
    r = copy_file_verify2( &origpath[ 0 ], &destpath[ 0 ] );
    if (r != 0) {
        report_copy_file_error( r, origpath );
        ABORT;
    }
    RETURN;
}

/**************************/
void p11_restartCmdHis()
/**************************/
// called by rt or rt_FDA to restart cmdHistory if file is too big, 
// but more importantly to update p11 icons.
{
    double d;
    struct stat fstat;

    /* restart cmdHistory */

    if(cmdHistoryFp == NULL) return;

    if(!part11System && !P_getreal(GLOBAL, "cmdHis", &d, 1) && d < 0.5) return;

    fclose(cmdHistoryFp);

    stat(cmdHisFile, &fstat);

    if(fstat.st_size > 1e4) { // file is big, restart cmdHistory
        cmdHistoryFp = fopen(cmdHisFile, "w");
        if(cmdHistoryFp != NULL) 
          fprintf(cmdHistoryFp,"%s cmd history restarted.\n", cmdTime);
    } else { // append to cmdHistory
        cmdHistoryFp = fopen(cmdHisFile, "a");
    }

    // update p11 icons after rt. 
    okFDAdata = 0;
    updateCheckIcon(); 
    updateLinkIcon(); 
}

/**************************/
static int copy_data(char* curr, char* rec)
/**************************/
{
/* called in rt_FDA */
 
    int ival = 1;
    char path[MAXSTR], recpath[MAXSTR]; 

    if(!strEndsWith(curr, "/")) strcat(curr, "/");
    if(!strEndsWith(rec, "/")) strcat(rec, "/");

    sprintf(path, "%s%s", curr, "data");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "phasefile");
    if(fileExist(path)) unlink(path);

    sprintf(recpath, "%s%s", rec, "data");
    sprintf(path, "%s%s", curr, "data");
    if(fileExist(recpath)) copy_file(recpath, path);
   
    sprintf(recpath, "%s%s", rec, "phasefile");
    sprintf(path, "%s%s", curr, "phasefile");
    if(fileExist(recpath)) copy_file(recpath, path);

    return(ival);
}

/**************************/
int copy_datdir(char* curr, char* rec)
/**************************/
{
    char path[MAXSTR], recpath[MAXSTR];

    if(!strEndsWith(curr, "/")) strcat(curr, "/");
    if(!strEndsWith(rec, "/")) strcat(rec, "/");

    sprintf(path, "%s%s", curr, "procpar");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "curpar");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "snapshot");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "cmdHistory");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "auditTrail");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "checksum");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "text");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "global");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curr, "conpar");
    if(fileExist(path)) unlink(path);
    sprintf(path, "%s%s", curexpdir, "/maclib");
    if(fileExist(path)) rm_file(path);
    sprintf(path, "%s%s", curexpdir, "/incFiles");
    if(fileExist(path)) rm_file(path);
    sprintf(path, "%s%s", curexpdir, "/recon");
    if(fileExist(path)) rm_file(path);

/* 
   procpar and curpar will be directly loaded to vnmr and write to curexpdir/datdir 
   sprintf(recpath, "%s%s", rec, "procpar");
   sprintf(path, "%s%s", curr, "procpar");
   if(fileExist(recpath)) copy_file(recpath, path);

   sprintf(recpath, "%s%s", rec, "curpar");
   sprintf(path, "%s%s", curr, "curpar");
   if(fileExist(recpath)) copy_file(recpath, path);
*/
    sprintf(recpath, "%s%s", rec, "snapshot");
    sprintf(path, "%s%s", curr, "snapshot");
    if(fileExist(recpath)) copy_file(recpath, path);

    sprintf(recpath, "%s%s", rec, "cmdHistory");
    sprintf(path, "%s%s", curr, "cmdHistory");
    if(fileExist(recpath)) copy_file(recpath, path);

    sprintf(recpath, "%s%s", rec, "text");
    sprintf(path, "%s%s", curr, "text");
    if(fileExist(recpath)) copy_file(recpath, path);

    sprintf(recpath, "%s%s", rec, "global");
    sprintf(path, "%s%s", curr, "global");
    if(fileExist(recpath)) copy_file(recpath, path);

    sprintf(recpath, "%s%s", rec, "conpar");
    sprintf(path, "%s%s", curr, "conpar");
    if(fileExist(recpath)) copy_file(recpath, path);

    sprintf(recpath, "%s%s", rec, "maclib");
    sprintf(path, "%s%s", curexpdir, "/maclib");
    if(fileExist(recpath)) copy_file(recpath, path);

    sprintf(recpath, "%s%s", rec, "incFiles");
    sprintf(path, "%s%s", curexpdir, "/incFiles");
    if(fileExist(recpath)) copy_file(recpath, path);

    sprintf(recpath, "%s%s", rec, "recon");
    sprintf(path, "%s%s", curexpdir, "/recon");
    if(fileExist(recpath)) link_file(recpath, path);

    if(part11System) {
        sprintf(recpath, "%s%s", rec, "auditTrail");
        sprintf(path, "%s%s", curr, "auditTrail");
        if(fileExist(recpath)) copy_file(recpath, path);

        writeAuditTrails("copy_datdir", rec, curr, "datdir");

        sprintf(recpath, "%s%s", rec, "checksum");
        sprintf(path, "%s%s", curr, "checksum");
        if(fileExist(recpath)) copy_file(recpath, path);
    }

    RETURN;
}

/**************************/
int p11_isRecord(char* path)
/**************************/
{
    int ival = 0;
    if(strEndsWith(path, ".REC") || strEndsWith(path, ".REC/") || 
       strEndsWith(path, ".REC/acqfil") || 
       strEndsWith(path, ".REC/acqfil/")) ival = 1;
    else if(strstr(path, ".REC/datdir") != NULL || 
            (strstr(path, ".REC/") != NULL && 
             (strEndsWith(path, ".dat") || strEndsWith(path, ".dat/")))) ival = 2;

    if(strEndsWith(path, ".rec") || strEndsWith(path, ".rec/") || 
       strEndsWith(path, ".rec/acqfil") || 
       strEndsWith(path, ".rec/acqfil/")) ival = 3;
    else if(strstr(path, ".rec/datdir") != NULL ||
            (strstr(path, ".rec/") != NULL && 
             (strEndsWith(path, ".dat") || strEndsWith(path, ".dat/")))) ival = 4;

    return(ival);
}

/******************************/
static int copytext(char *frompath, char *topath)
/******************************/
{
    char  path[MAXPATH];
    FILE *infile,*outfile;
    int   ch;

    if(!strEndsWith(frompath, "/")) strcat(frompath, "/");
    if(!strEndsWith(topath, "/")) strcat(topath, "/");

    strcpy(path,frompath);
    strcat(path,"text");

    if ( (infile=fopen(path,"r")) )
    {
        strcpy(path,topath);
        strcat(path,"text");

        if ( (outfile=fopen(path,"w")) )
            while ((ch=getc(infile)) != EOF) putc(ch,outfile);
        else
        {
            fclose(infile);
            ABORT;
        }
        fclose(infile);
        fclose(outfile);
    }
    else
        ABORT;
    RETURN;
}

/******************************/
int overwrite_file(char* filepath)
/******************************/
/* return 0 is file is successfully removed */
/* otherwise return 1. */
{
    char answer[16];

    W_getInput("File exists, overwrite (enter y or n <return>)? "
               ,answer,15);
    if ((strcmp(answer,"y")==0) || (strcmp(answer,"yes")==0)) { 
        if(!isUserOwned(filepath)) {
            Werrprintf("cannot overwrite existing file: %s.\n",filepath);
            ABORT;
        } else {
            rm_file(filepath);	
            if(part11System && sessionAuditTrailFp != NULL) {
                fprintf(sessionAuditTrailFp, "%s audit: overwrite record\n", cmdTime);
                fprintf(sessionAuditTrailFp, "                     path: %s\n", filepath);
            }
            RETURN; 
        }
    } else ABORT;	 
}

#ifdef OLD
/***********************/
static void remove_3d(char* exppath )
/***********************/
{
    char    subdirpath[ MAXPATH+10 ], remove_it_cmd[ MAXPATH+10 ];

/*  Remove "datadir3d" subdirectory  */

    strcpy( &subdirpath[ 0 ], exppath );
#ifdef UNIX
    strcat( &subdirpath[ 0 ], "/datadir3d" );
    strcpy( &remove_it_cmd[ 0 ], "rm -rf " );
#else 
    vms_fname_cat( &subdirpath[ 0 ], "[.datadir3d]" );
    strcpy( &remove_it_cmd[ 0 ], "rm_recur " );
#endif 
    strcat( &remove_it_cmd[ 0 ], &subdirpath[ 0 ] );
    system( &remove_it_cmd[ 0 ] );

/*  Remove "info" subdirectory  */

    strcpy( &subdirpath[ 0 ], exppath );
#ifdef UNIX
    strcat( &subdirpath[ 0 ], "/info" );
    strcpy( &remove_it_cmd[ 0 ], "rm -rf " );
#else 
    vms_fname_cat( &subdirpath[ 0 ], "[.info]" );
    strcpy( &remove_it_cmd[ 0 ], "rm_recur " );
#endif 
    strcat( &remove_it_cmd[ 0 ], &subdirpath[ 0 ] );
    system( &remove_it_cmd[ 0 ] );

/*  Remove "auto" file  */

    strcpy( &subdirpath[ 0 ], exppath );
#ifdef UNIX
    strcat( &subdirpath[ 0 ], "/auto" );
    strcpy( &remove_it_cmd[ 0 ], "rm -f " );
#else 
    strcat( &subdirpath[ 0 ], "auto;" );
    strcpy( &remove_it_cmd[ 0 ], "delete " );
#endif 
    strcat( &remove_it_cmd[ 0 ], &subdirpath[ 0 ] );
    system( &remove_it_cmd[ 0 ] );

/*  Remove "coef" file  */

    strcpy( &subdirpath[ 0 ], exppath );
#ifdef UNIX
    strcat( &subdirpath[ 0 ], "/coef" );
    strcpy( &remove_it_cmd[ 0 ], "rm -f " );
#else 
    strcat( &subdirpath[ 0 ], "coef;" );
    strcpy( &remove_it_cmd[ 0 ], "delete " );
#endif 
    strcat( &remove_it_cmd[ 0 ], &subdirpath[ 0 ] );
    system( &remove_it_cmd[ 0 ] );
}
#endif

/**************************/
static void saveallshims(int rtsflag)
/**************************/
{
    int index;

    init_shimnames(-1);
    for (index=Z0 + 1; index <= MAX_SHIMS; index++)
    {
        const char *sh_name;
        if ((sh_name = get_shimname(index)) != NULL)
        {
            saveshim(sh_name,   rtsflag);
        }
    }
}

/******************/
static int update(const char *vname)
/******************/
{ double v;
    if (P_getreal(TEMPORARY,vname,&v,1)) ABORT;
    if (P_setreal(CURRENT,vname,v,0)) ABORT;
    RETURN;
}

/****************************/
static int saveshim(const char* vname, int rtsflag)
/****************************/
{
    if (rtsflag) {
        if (update( vname )) ABORT;
        /*if (P_copyvar(TEMPORARY,CURRENT,vname,vname)) ABORT;*/
    }
    else if (P_copyvar(CURRENT,TEMPORARY,vname,vname)) ABORT;
    RETURN;
}

/**************************/
int isRecUpdated(int argc, char *argv[], int retc, char *retv[])
/**************************/
{
    int i = 0;

    (void) argc;
    (void) argv;
    paramChanged = hasParamChanged();
    if(paramChanged) i = 1; 

    if (retc > 0)
    {
        retv[0]= (char *)realString((float)i);
    }

    RETURN;
}

/**************************/
int isFDAsystem(int argc, char *argv[], int retc, char *retv[])
/**************************/
{
    (void) argc;
    (void) argv;
    if (retc > 0)
    {
        retv[0]= (char *)realString((float)part11System);
    }

    RETURN;
}

/**************************/
int isFDAdir(int argc, char *argv[], int retc, char *retv[])
/**************************/
{
    int i;
    string path;

    if(argc < 2) i = 0;
    else {
	strcpy(path, argv[1]);
	i = p11_isPart11Dir(path);
    }
 
    if (retc > 0)
    {
        retv[0]= (char *)realString((float)i);
    }

    RETURN;
}

/**************************/
int isFDAdata(int argc, char *argv[], int retc, char *retv[])
/**************************/
{
    int i;

    (void) argc;
    (void) argv;
    i = isPart11Data();

    if (retc > 0)
    {
        retv[0]= (char *)realString((float)i);
    }

    RETURN;
}

/**************************/
int isDataRecord(int argc, char *argv[], int retc, char *retv[])
/**************************/
{
    (void) argc;
    (void) argv;

    int i = isFDAmode();
    if(i == 2) i = 1;
    else i = 0;
     
    if (retc > 0)
    {
        retv[0]= (char *)realString((float)i);
    }

    RETURN;
}

/**************************/
int testPart11(int argc, char *argv[], int retc, char *retv[])
/**************************/
{
    char origpath[MAXSTR], tstr[MAXSTR];

    int ival;
    char linkpath[MAXSTR];
    char path[MAXSTR];
 
    (void) argc;
    (void) argv;
    (void) retc;
    (void) retv;
    p11_saveFDAfiles_raw("testPart11", "", "");

    sprintf(path, "%s/datdir/data", curexpdir);
    if(fileExist(path)) rm_file(path);
    sprintf(path, "%s/datdir/phasefile", curexpdir);
    if(fileExist(path)) rm_file(path);

    execString("clear(2)\n");

    sprintf(path, "%s/acqfil/fid", curexpdir);
    ival = file_is_link(path, "", linkpath );

    if(ival) {
	rm_file(path);
        copy_file(linkpath, path);
    }

    sprintf(path, "%s/acqfil/log", curexpdir);
    ival = file_is_link(path, "", linkpath );

    if(ival) {
	rm_file(path);
        copy_file(linkpath, path);
    }
	
    currentDate(tstr, MAXSTR);

    P_setstring(CURRENT,"time_run",tstr,1);

    P_setstring(CURRENT,"username",UserName,1);
    P_setstring(CURRENT,"p11console",p11console,1);

    sprintf(path, "%s/acqfil/procpar", curexpdir);
    if(fileExist(path)) rm_file(path);
    if (P_save(PROCESSED,path))
    {   Werrprintf("Problem saving processed parameters in '%s'.",path);
        ABORT;
    }

    sprintf(path, "%s/acqfil/text", curexpdir);
    if(!fileExist(path)) {
        sprintf(origpath, "%s/text", curexpdir);
        copytext(origpath, path);
    }

    p11_setcurrfidid();

    RETURN;
}

/**************************/
void p11_saveDisCmd()
/**************************/
{
    char cmd[MAXSTR];
    double d;
    string str;

    if(strstr(currentCmd,"recordOff") != NULL) return;

    if(!part11System && !P_getreal(GLOBAL, "recordSave", &d, 1) && d < 0.5) return;

    if(P_getstring(GLOBAL, "acqmode", str, 1, MAXSTR)) strcpy(str, "");
    if(strcmp(str, "fidscan") == 0 || strcmp(str, "lock") == 0) return;

    if(!disNewCmd) return;
    disNewCmd = 0;
    strcpy(cmd, "");
    if(strStartsWith(currentCmd,"ds") || strStartsWith(currentCmd,"dcon") ||
       strStartsWith(currentCmd,"aipDisplay"))  
	if(currentCmd[strlen(currentCmd)-1] == '\n') {
	    strcpy(cmd, "");
	    strncat(cmd, currentCmd, strlen(currentCmd)-1);
	} else strcpy(cmd, currentCmd);
    else if(getProcdim() == 2) strcpy(cmd, "dconi");
    else if(getProcdim() == 1) strcpy(cmd, "ds");

    if(debug) 
	fprintf(stderr, "******p11_saveDisCmd %s\n",cmd);

    if(strlen(cmd) > 0)
    	P_setstring(CURRENT, "disCmd", cmd , 1);
}

/**************************/
int svr_FDA(int argc, char *argv[], int retc, char *retv[])
/**************************/
/* svr saves vnmr data as "electronic record" in directories of recsuffix */
/* root path and ownership of record directory is specified in part11Config file */
/* recsuffix consists acqfil and datdirXXX or .dat subdirectories for raw and */
/* processed data with XXX being version number of processing a data may be */
/* processed more than one times, and saved in the same recsuffix */

/* assume go and ft cammand saved all the necessary files (specified by */
/* part11Config) in curexpdir/acqfil and curexpdir/datdir */

/* svr: save record (default). if raw data is already saved */
/* (indicated by curexpdir/acqfil/fid being linked to a recsuffix/acqfil/fid), */
/* save only processed data, and to recsuffix/datdirXXX */
/* if raw data is not saved, save to a new recsuffix, remove files in */
/* curexpdir/acqfil and link curexpdir/acqfil to the new recsuffix/acqfil */

/* svr_as: if fid is not linked to a record, save current data (curexpdir/acqfil */
/* and curexpdir/datdir) to a new recsuffix */
/* always removed files in curexpdir/acqfil */
/* and link it to the new recsuffix/acqfil */

/* if curexpdir/acqfil/fid is a link to a recsuffix/acqfil */
/* copy all files in currently linked recsuffix (indicated by */
/* curexpdir/acqfil/fid) to a new recsuffix */
/* curexpdir/acqfil has to be a link to a recsuffix/acqfil */
/* don't change link */

/* svr_r: save only raw data (curexpdir/acqfil), and save to a new recsuffix */
/* if curexpdir/acqfil is not linked, removed files in curexpdir/acqfil */
/* and link it to the new recsuffix/acqfil */
/* "arch" may be passed as an argument */

/* svr_p: save only processed data, and save to a new recsuffix */

{ char filepath[MAXPATH],path[MAXPATH];
    char origpath[MAXPATH],destpath[MAXPATH];
    char curfid_linkpath[MAXPATH*2];
    char version[MAXSTR], linkpath[MAXPATH];
    char a_name[MAXSTR], recsuffix[MAXSTR];
    int  fid_linked;
    int nolog, no_arch, i;
    char datname[MAXSTR], name[MAXSTR], cmd[MAXSTR];
    int dataProcessed = 0;

    //if(!P_getreal(GLOBAL, "recordSave", &d, 1)) recordsave = (int)d;

    //if(!part11System && !recordsave) RETURN;
  
    int part11Data = isPart11Data(); 

    readUserPart11Config();
    strcpy(cmd,argv[0]);
    if(strcmp(cmd,"svf") == 0 || strcmp(cmd,"SVF") == 0) strcpy(cmd,"svr_as");

    if(debug) displayPart11Config(stderr);

    strcpy(name,"");
    strcpy(recsuffix,"");

    nolog = FALSE; /* nolog is not used */
    no_arch = TRUE;
    if ( !strcmp(cmd,"scr_r") ) {
        for (i = 1; i<argc; i++)
            if (!strcmp(argv[i],"nolog") ) nolog = TRUE;
        for (i = 1; i<argc; i++)
            if (!strcmp(argv[i],"arch") ) no_arch = FALSE;
    }
    if (nolog) argc--;
    if (!no_arch) argc--;

/* find out whether curexpdir/acqfil/fid is linked */
/* 1 is yes, 0 is no, if yes, get curfid_linkpath */
/* find out whether curexpdir/dardir/data is linked */
/* 1 is yes, 0 is no, if yes, get curfid_linkpath */
/* if a record is retrieved, only fid, log, data, phasefile are linked */
/* other files are copied. */
/* make sure when unlink/remove fid and data, other files are also unlinked/removed */

/* the 2nd argument of file_is_link is a str. */
/* if 1st argument (path) is linked and curfid_linkpath != str, 1 is returned */
/* if 1st argument (path) is linked and curfid_linkpath == str, 2 is returned */
/* here we use empty str, so either 1 or 0 will be returned. */

/* if a record does not contain fid, only data (and phasefile) will be linked by rt */
/* if both fid and data are linked, the should link to the same record */

    strcpy(linkpath, "");

    sprintf(path,"%s%s",curexpdir,"/acqfil/fid");
    fid_linked = file_is_link(path, "", curfid_linkpath );

    if(fid_linked && strlen(curfid_linkpath) > 10 &&
       (strEndsWith(curfid_linkpath, ".REC/acqfil/fid") ||
	strEndsWith(curfid_linkpath, ".rec/acqfil/fid"))) {
        strcpy(linkpath, "");
        strncat(linkpath, curfid_linkpath, strlen(curfid_linkpath)-10);
    } else {
	char str[MAXSTR];
        fid_linked = 0;
        if(P_getstring(CURRENT, "file", str, 1, MAXSTR)) strcpy(str,"");
        if(str[0] == '/' && !strEndsWith(str, "/")) strcat(str, "/");
        if(!access(str,F_OK) && 
		(strEndsWith(str,".REC/") || strEndsWith(str,".rec/") )) {
	   strcpy(linkpath,str);
	   strcat(str,"/acqfil/fid");
	   if(!access(str,F_OK)) {
	       	fid_linked = 1;
	   	strcpy(curfid_linkpath,str);
	   } else strcpy(linkpath, "");
	}
    }

    if(getProcessid() > 0) dataProcessed = 1;
    else dataProcessed = 0;

    paramChanged = hasParamChanged();

    if(debug) 
	fprintf(stderr, 
                "svr:cmd fid_linked dataProcessed paramChanged linkpath %s %d %d %d %s\n", 
                cmd, fid_linked, dataProcessed, paramChanged, linkpath);

    strcpy(currentPart11Path, "");

/* check arguments and get file name */
/* dataProcessed is 0 if fid is not processed, otherwise 1. */

    if(strcmp(cmd, "svr") == 0) {
        if(strlen(linkpath) == 0) 
        { Werrprintf("Abort: REC data not loaded.");
            ABORT;
        }
	strcpy(currentPart11Path, linkpath);
    } else { 
        /* if path is not gaven as argv[1] */ 
        if (argc<2)
        { W_getInput("File name (enter name and <return>)? ",filepath,MAXPATH-1);
            if (strlen(filepath)==0)
            { Werrprintf("No file name given, command aborted\n");
                ABORT;
            }
        }
        else if (argc!=2)
        { Werrprintf("usage - %s(filename)",cmd);
            ABORT;
        }
        else
            strcpy(filepath, argv[1]);
        if ((int) strlen(filepath) >= (MAXPATH-32))
        { Werrprintf("file path too long");
            ABORT;
        }
        if (verify_fname(filepath))
        { Werrprintf( "file path '%s' not valid", filepath );
            ABORT;
        }

        /* determine recsuffix */

        if(part11System && part11Data) {
            strcpy(recsuffix, ".REC");
        } else { 
            strcpy(recsuffix, ".rec");
        } 
	// respect user if .rec is explicitly specfied
        if(strEndsWith(filepath,".rec") || strEndsWith(filepath,".rec/")) strcpy(recsuffix,".rec");

        /* if only a name, add rootPath */
        if(filepath[0] != '/' && filepath[0] != '~') {
	     if (P_getstring(GLOBAL,"svfdir",a_name,1,MAXPATH))
		sprintf(a_name,"%s/data/%s",userdir,filepath);
	     else sprintf(a_name,"%s/%s",a_name,filepath); 
	     if(makeautoname("Svfname",a_name,"",path,FALSE,FALSE,recsuffix,recsuffix))
	        sprintf(path,"%s/data/%s",userdir,filepath);	     
        } else strcpy(path, filepath);

        if(strEndsWith(path,".REC") || strEndsWith(path,".rec")) {
            strcpy(filepath, "");
            strncat(filepath, path, strlen(path)-4);
            strcat(filepath, recsuffix);
        } else if(strEndsWith(path,".REC/") || strEndsWith(path,".rec/")) {
            strcpy(filepath, "");
            strncat(filepath, path, strlen(path)-5);
            strcat(filepath, recsuffix);
        } else {
	    strcpy(filepath, path);
            strcat(filepath, recsuffix);
        }

        strcpy(currentPart11Path, filepath);

        if(!strEndsWith(currentPart11Path,"/")) strcat(currentPart11Path, "/");

	if(debug) 
            fprintf(stderr,"currentPart11Path curexpdir %s %s\n", 
                    currentPart11Path, curexpdir);

        if(fileExist(currentPart11Path) && strcmp(currentPart11Path, linkpath) == 0) {
	    if(part11System == NONFDA) strcpy(cmd,"svr");
	    else {
              Werrprintf("cannot copy: %s already linked to curexp. command aborted.\n",
                       linkpath);
              disp_status("        ");
              ABORT;
	    }
        } else if(strcmp(cmd,"svr_p")!=0 &&
                  fileExist(currentPart11Path) && overwrite_file(currentPart11Path)) {
            Werrprintf("command aborted.\n");
            disp_status("        ");
            ABORT;
        }
    }
  
    if(strEndsWith(currentPart11Path, ".REC") && part11System == FDA && !part11Data) {
        Werrprintf("current data cannot be saved as a record, command aborted.\n");
        disp_status("        ");
        ABORT;
    };

    if(part11System == NONFDA && strstr(currentPart11Path, "REC") != NULL) {
        Werrprintf("current data cannot be saved as REC, command aborted.\n");
        disp_status("        ");
        ABORT;
    }

    Wturnoff_buttons();
    D_allrelease();

    if(P_getstring(CURRENT,"datname",datname,1, MAXSTR)) strcpy(datname, "datdir");
    if(strlen(datname) == 0) strcpy(datname, "datdir");

    disp_status("svr ");

/* notice currentPart11Path and linkpath end with "/", curexpdir does not */

    strcpy(destpath,"");
    strcpy(origpath,"");
    if (strcmp(cmd,"svr_as") == 0) {
/* save current acqfil and datdir to a new record */
/* always remove and link curexpdir/acqfil to the new record */

            sprintf(origpath, "%s%s", curexpdir, "/acqfil");
            sprintf(destpath, "%s%s", currentPart11Path, "acqfil");
            save_acqfil(origpath, destpath);
//    	    if(strEndsWith(filepath,".REC")) link_fid(origpath, destpath);

            if(dataProcessed) {
                sprintf(origpath, "%s%s", curexpdir, "/datdir");
                if(strcmp(datname, "datdir") == 0) 
                    sprintf(destpath, "%s%s001", currentPart11Path, "datdir");
                else sprintf(destpath, "%s%s%s", currentPart11Path, datname, "001.dat");
                save_datdir(origpath, destpath);
            }

    } else if (strcmp(cmd,"svr_r")==0) {
/* save acqfil to a new record */
/* don't change link. */

        sprintf(origpath, "%s%s", curexpdir, "/acqfil");
        sprintf(destpath, "%s%s", currentPart11Path, "acqfil");
        save_acqfil(origpath, destpath);

    } else if (strcmp(cmd,"svr_p")==0) {
/* save datdir to a new record */

        sprintf(origpath, "%s%s", curexpdir, "/datdir");
        if(strcmp(datname, "datdir") == 0) 
            sprintf(destpath, "%s%s001", currentPart11Path, "datdir");
        else sprintf(destpath, "%s%s%s", currentPart11Path, datname, "001.dat");
        save_datdir(origpath, destpath);

    } else {

/* default saving */

	if(debug) 
            fprintf(stderr, "svr cmd %s\n", cmd);

        if (!fid_linked) {
/* curfid is not a link, save acqfil and datdir to currentPart11Path*/
/* remove and link acqfil to the new recsuffix/acqfil */

            sprintf(origpath, "%s%s", curexpdir, "/acqfil");
            sprintf(destpath, "%s%s", currentPart11Path, "acqfil");
            save_acqfil(origpath, destpath);
            if(strEndsWith(filepath,".REC")) link_fid(origpath, destpath);

            if(dataProcessed) {
                sprintf(origpath, "%s%s", curexpdir, "/datdir");
                if(strcmp(datname, "datdir") == 0) 
                    sprintf(destpath, "%s%s001", currentPart11Path, "datdir");
                else sprintf(destpath, "%s%s%s", currentPart11Path, datname, "001.dat");
                save_datdir(origpath, destpath);
            }

        } else {
/* curfid is a link and currentPart11Path == linkpath */
/* don't save acqfil, save datdir with a new version number */

            if(dataProcessed && paramChanged) {
                getNextVersion(currentPart11Path, datname, version);

                sprintf(origpath, "%s%s", curexpdir, "/datdir");
                if(strcmp(datname, "datdir") == 0) 
                    sprintf(destpath, "%s%s%s", currentPart11Path, "datdir", version);
                else sprintf(destpath, "%s%s%s.dat", currentPart11Path, datname, version);
                save_datdir(origpath, destpath);
            }

        } 
    }

    if (!fid_linked && !no_arch)
    { 	FILE    *fd,*fd_text;
        double   loc;
        char    *ptr, solvent[20], tmp_text[130];
        strcpy(path,currentPart11Path);
        ptr = strrchr(path,'/');
        if (ptr==0) strcpy(path,"doneQ");
        else { *ptr = '\0'; strcat(path,"/doneQ"); }
        fd = fopen(path,"a");   /* append, or open if not existing */
        if (fd)
        {  P_getreal(PROCESSED,"loc",&loc,1);
            fprintf(fd,"  SAMPLE#: %d\n",(int)loc);
            fprintf(fd,"     USER: %s\n",UserName);
            fprintf(fd,"    MACRO: ??\n");
            P_getstring(PROCESSED,"solvent",solvent,1, 18);
            fprintf(fd,"  SOLVENT: %s\n",solvent);
            strcpy(path,currentPart11Path); strcat(path,"text");
            fd_text = fopen(path,"r");
            if(fd_text) {
                ptr = &tmp_text[0];
                while ( (*ptr = fgetc(fd_text)) != EOF && ptr-tmp_text < 128)
                {  if (*ptr == '\n') *ptr='\\';
                    ptr++;
                }
                fclose(fd_text); *ptr = '\0';
                fprintf(fd,"     TEXT: %s\n",tmp_text);
                /* chop '.fid' then only retain filename */
                strcpy(path,currentPart11Path); ptr = strrchr(path,'.'); *ptr = '\0';
                ptr = strrchr(path,'/'); if (ptr==0) ptr=path; else ptr++;
                fprintf(fd,"  USERDIR:\n");
                fprintf(fd,"     DATA: %s\n",ptr);
                fprintf(fd,"   STATUS: Saved\n");
                fprintf(fd,"------------------------------------------------------------------------------\n");
            }
            else
                Werrprintf("problem opening '%s'",path);

            fclose(fd);
        }
        else
            Werrprintf("problem opening '%s'",path);
    }

#ifdef VNMRJ
    if (!fid_linked || strcmp(cmd,"svr_as") == 0 ) {
        writeLineForLoc(currentPart11Path, "vnmr_record");
        if(debug)
            Winfoprintf("part11: record is saved as %s\n", currentPart11Path);
    }
#endif 

    if (retc > 0) retv[0]=newString(origpath);
    if (retc > 1) retv[1]=newString(destpath);

    /* hasParamChanged uses curexpdir/prevpar */
    sprintf(path, "%s%s", curexpdir, "/prevpar");
    saveCurpar(path);

    disp_status("        ");
    RETURN;
}

/**************************/
int rt_FDA(int argc, char *argv[], int retc, char *retv[],
           char* filepath, int gettxtflag, int fidflag,
           int nolog, int doDisp, int doMenu, int do_update_params, int type)
/**************************/
{
/* this is called if cmd is rt, rtp, or rtv, and path ends with */
/* recsuffix/acqfil, or recsuffix/datdirXXX */ 
/* when retrieve data, all files in curexpdir/acqfil or curexpdir/datdir */
/* are linked to the record, except auditTrail is copied */
/* from record to curexpdir/acqfil or curexpdir/datdir */

    char path[MAXPATH],newpath[MAXPATH],filepath0[MAXPATH];
    char recsuffix[MAXSTR];
    char cmd[MAXSTR],dataOwner[MAXSTR];

    int r,i;
    extern void resetdatafiles();
    extern void setfilepaths();
    int fiderr = 1, dataerr = 1;
    string datname;

    (void) nolog;
    //if(!part11System && !P_getreal(GLOBAL, "recordSave", &d, 1) && d < 0.5) RETURN;

    if(filepath[strlen(filepath)-1] != '/') strcat(filepath, "/");

    if(strstr(filepath, ".REC") != NULL) strcpy(recsuffix, ".REC");
    else strcpy(recsuffix, ".rec");

    if(strEndsWith(filepath, ".REC/")) {
        if(getLastVersion(filepath, datname)) {
            strcat(filepath, datname);
            strcat(filepath, "/");
            type = 2;
        } else {
            strcat(filepath, "acqfil/");
            type = 1;
        }
    }
    else if(strEndsWith(filepath, ".rec/")) {
        if(getLastVersion(filepath, datname)) {
            strcat(filepath, datname);
            strcat(filepath, "/");
            type = 4;
        } else {
            strcat(filepath, "acqfil/");
            type = 3;
        }
    }

    if(!fileExist(filepath)) {
        Werrprintf("%s does not exists.", filepath);
	ABORT;
    }

    /* filepath is acqfil or datdir depending on type */
    /* filepath0 is always acqfil dir */ 
    if(type == 1 || type == 3)
     	strcpy(filepath0, filepath);
    else {
        brkPath(filepath, filepath0,datname);
	strcat(filepath0, "acqfil/");
    }

/**** the following are retrieved from filepath ****/

    if (gettxtflag)
    { /* now copy text file */
        if (strcmp(argv[0],"gettxt") == 0)
        {   if (copytext(filepath,curexpdir))
            {   Werrprintf("cannot read and copy text file");
                ABORT;
            }
        }
        else if (copytext(curexpdir,filepath))
        {   Werrprintf("cannot read and copy text file");
            ABORT;
        }
        text(0,argv,0,NULL);
 
        if (doDisp)
            disp_status("        ");
        RETURN;
    }

    /*Remove the ACQPAR file from the current experiment.  Prevents RA from
      working after RT is executed; you have to enter the GO command first.  */

    strcpy( path, &curexpdir[0] );
    strcat( path, "/acqfil/acqpar" );
    i = unlink( &path[ 0 ] );             /* assume it works and ignore result */

    /* get individual param if rtv */

    /* save shim values so as to retain them if no FID found */
    P_treereset(TEMPORARY);
    saveallshims(0);          /* Second argument of 0      */
    /* serves to move value from */
    /* CURRENT tree to TEMPORARY */


    /* read parameters into current parameter tree */
    /* if acqfil, read procpar, else read curpar */

/*
  if(type == 1 || type == 3) sprintf(path, "%s%s", filepath, "procpar");
  else 
*/
    sprintf(path, "%s%s", filepath, "procpar");

    if(fileExist(path)) {
        P_treereset(CURRENT);         /* clear the tree first */
        if ( (r=P_read(CURRENT,path)) )
        { Werrprintf("problem reading parameters from %s",path);
            P_treereset(TEMPORARY);
            disp_status("        ");
            ABORT;
        }
    }

    /* now copy text file */
    sprintf(path, "%s%s", filepath, "text");

    if(fileExist(path)) {
        if (copytext(filepath,curexpdir))
        {
            Werrprintf("cannot read and copy text file %s/text", filepath);
            P_treereset(TEMPORARY);
            disp_status("        ");
            execString("fixpar\n");
            ABORT;
        }
    }

#ifdef NOT_A_GOOD_P11_IDEA
/*  We used to retrieve global and conpar if they were saved in the record.
 *  There does not appear be be a good reason to do this. It can cause
 *  problems, especially if these records are retrieved on a different
 *  spectrometer.
 */
    sprintf(path, "%s%s", filepath, "global");

    if(fileExist(path)) {
        char saveOperator[MAXSTR] = "";
        P_getstring(GLOBAL,"operator",saveOperator,1,MAXSTR-1);
        P_treereset(GLOBAL);         /* clear the tree first */
        if (r=P_read(GLOBAL,path))
        { Werrprintf("problem reading parameters from %s",path);
            P_treereset(TEMPORARY);
            disp_status("        ");
            ABORT;
        }
        setGlobalPars();
        P_setstring(GLOBAL,"operator",saveOperator,1);
    }

    sprintf(path, "%s%s", filepath, "conpar");

    if(fileExist(path)) {
        P_treereset(SYSTEMGLOBAL);         /* clear the tree first */
        if (r=P_read(SYSTEMGLOBAL,path))
        { Werrprintf("problem reading parameters from %s",path);
            P_treereset(TEMPORARY);
            disp_status("        ");
            ABORT;
        }
        check_datastation();
    }
#endif

    if (!fidflag)
    {  P_setstring(CURRENT,"file",filepath, 0);
        saveallshims(1);     /* TEMPORARY to CURRENT */
        P_treereset(TEMPORARY);
        if (doDisp)
            disp_status("        ");
        execString("fixpar\n");
        releasevarlist();
        set_vnmrj_rt_params( do_update_params );
#ifdef VNMRJ
        if (do_update_params > 0)
        {
            appendTopPanelParam();
            disp_current_seq();
        }
#else 
        if (doDisp)
            execString("dg\n");
#endif 
        if ((!Bnmr) && (doMenu))
            execString("menu('main')\n");

        p11_restartCmdHis();
#ifdef VNMRJ
/*
  writelineToVnmrJ("pnew", "1 file");
*/
#endif 

        RETURN;
    }
/* remove data in data and phasefile */
/*
  set_nodata(); 
  remove_3d( curexpdir );
*/
    specIndex = 1;

/* so far the parameters are retrieved to the memory and text copied to curexpdir */
/* but no files are retrieved to curexpdir/acqfil or curexpdir/datdir yet*/

/* retrieve acqfil dir regardless the type */

    strcpy(newpath,curexpdir);
    strcat(newpath,"/acqfil");

    /* now check whether fid is in file */
    strcpy(path,filepath0);
    strcat(path,"fid");

    if(P_getstring(CURRENT, "username", dataOwner, 1, MAXSTR))
	strcpy(dataOwner, "");

    if (!access(path,F_OK) && strcmp(dataOwner, UserName) == 0) {

        /* yes, erase the current fid and link the new one */
        /* use `set_nodata' to close D_USERFILE
           before deleting curexp+'/acqfil/fid' */

        fiderr = link_fid(newpath, filepath0);

    } else if (!access(path,F_OK)) {
        /* copy fid */

        fiderr = copy_fid(newpath, filepath0);

    } else if(type == 1 || type == 3)

    { Winfoprintf("no fid in file, only parameters copied\n");
        P_setstring(CURRENT,"file",filepath0,0);
        saveallshims(1);
        P_treereset(TEMPORARY);
        if (doDisp)
            disp_status("        ");
        execString("fixpar\n");
        set_vnmrj_rt_params( do_update_params );
#ifdef VNMRJ
        if (do_update_params > 0)
        {
            appendTopPanelParam();
            disp_current_seq();
        }
#endif 
        RETURN;
    } 

    if (fiderr)
    {
        fix_automount_dir( path, path );
        if (symlink(path,newpath))
        { Werrprintf("rt_FDA:cannot link the fid file %s %s", path,newpath);
            P_treereset(TEMPORARY);
            disp_status("        ");
            execString("fixpar\n");
            set_vnmrj_rt_params( do_update_params );
#ifdef VNMRJ
            if (do_update_params > 0)
            {
                appendTopPanelParam();
                disp_current_seq();
            }
#endif 
            ABORT;
        }
    } 

/* copy all files except fid and log */
    copy_acqfil(newpath, filepath0);

    /* retrieve datdir */
    if(type == 2 || type == 4) {
        strcpy(newpath,curexpdir);
        strcat(newpath,"/datdir");
        copy_datdir(newpath, filepath);
        dataerr = copy_data(newpath, filepath);
    }

    /* if everything ok, copy parameters to PROCESSED */

    brkPath(filepath, path,datname);
    strcpy(filepath0,"");
    strncat(filepath0,path,strlen(path)-1);
    P_setstring(CURRENT,"file",filepath0,0);
    P_treereset(PROCESSED);       /* clear the tree first */
    P_copy(CURRENT,PROCESSED);
    P_treereset(TEMPORARY);
    resetdatafiles();
    if (doDisp)
        disp_status("        ");
    Wsetgraphicsdisplay("");
    execString("fixpar\n");
    releasevarlist();
    set_vnmrj_rt_params( do_update_params );
#ifndef VNMRJ
    if (doDisp)
        execString("dg\n");
#else 
    if (do_update_params > 0)
    {
        appendTopPanelParam();
        disp_current_seq();
    }
#endif 

    if (!Bnmr) {
        if (doMenu)
            execString("menu('main')\n");

        // process,display spectrum 
        if(type == 1 || type == 3) {
            sprintf(cmd,"recds('%s','%s')\n",filepath, "raw");
            execString(cmd);
        } else {
            sprintf(cmd,"recds('%s','%s')\n",filepath, "processed");
            execString(cmd);
        }

	if(debug)
            fprintf(stderr,"recds %s", cmd);
    }

    /* hasParamChanged uses curexpdir/prevpar */
    sprintf(path, "%s%s", curexpdir, "/prevpar");
    saveCurpar(path);
    /* save procpar and curpar to curexpdir/datdir, instead of copy from filepath */
    sprintf(path, "%s%s", curexpdir, "/datdir/procpar");
    saveProcpar(path);
    sprintf(path, "%s%s", curexpdir, "/datdir/curpar");
    saveCurpar(path);

    p11_restartCmdHis();
#ifdef VNMRJ
/*
  writelineToVnmrJ("pnew", "1 file");
*/
#endif 

    if(debug)
        Winfoprintf("part11: record is retrieved from %s\n", filepath);

    RETURN;
}

/******************/
int readUserPart11Config()
/******************/
{
    char path[MAXPATHL];
    char  buf[BUFSIZE];
    FILE* fp;
    string words[MAXWORDS];
    int nwords;

    if(part11_std_files == NULL) ABORT;

    if(part11_std_files->fid != 2) part11_std_files->fid = 0;
    if(part11_std_files->procpar != 2) part11_std_files->procpar = 0;
    if(part11_std_files->log != 2) part11_std_files->log = 0;
    if(part11_std_files->text != 2) part11_std_files->text = 0;
    if(part11_std_files->usermaclib != 2) part11_std_files->usermaclib = 0;
    if(part11_std_files->global != 2) part11_std_files->global = 0;
    if(part11_std_files->conpar != 2) part11_std_files->conpar = 0;
    if(part11_std_files->shims != 2) part11_std_files->shims = 0;
    if(part11_std_files->waveforms != 2) part11_std_files->waveforms = 0;
    if(part11_std_files->data != 2) part11_std_files->data = 0;
    if(part11_std_files->phasefile != 2) part11_std_files->phasefile = 0;
    if(part11_std_files->fdf != 2) part11_std_files->fdf = 0;
    if(part11_std_files->snapshot != 2) part11_std_files->snapshot = 0;
    if(part11_std_files->cmdHistory != 2) part11_std_files->cmdHistory = 0;
    if(part11_std_files->pulseSequence != 2) part11_std_files->pulseSequence = 0;
    strcpy(incPath, "");
    incInc = 0;

    sprintf(path, "%s%s", userdir, userConfig);

    if(!(fp = fopen(path, "r"))) ABORT;

    while (fgets(buf,sizeof(buf),fp)) {

        if(strlen(buf) > 1 && buf[0] != '#' && buf[0] != '%' && buf[0] != '@') {
            nwords = MAXWORDS;
            getStrValues(buf, words, &nwords, ":"); 

	    if(nwords > 3) {
/*
  Winfoprintf("words %s %s %s %s\n", words[0], words[1], words[2], words[3]);	
*/
		if(strcmp(words[0], "file") == 0 
                   && strcmp(words[1], "optional") == 0
                   && strcmp(words[2], "fid") == 0 
                   && part11_std_files->fid != 2 
                   && strstr(words[3], "yes") != NULL)
                    part11_std_files->fid = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "procpar") == 0 
                        && part11_std_files->procpar != 2 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->procpar = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "log") == 0 
                        && part11_std_files->log != 2 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->log = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "text") == 0 
                        && part11_std_files->text != 2 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->text = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "global") == 0 
                        && part11_std_files->global != 2 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->global = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "conpar") == 0 
                        && part11_std_files->conpar != 2 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->conpar = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "usermaclib") == 0 
                        && part11_std_files->usermaclib != 2 
                        && strstr(words[3], "yes") != NULL) {
                    part11_std_files->usermaclib = 1;
		}
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "shims") == 0 
                        && part11_std_files->shims != 2 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->shims = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "waveforms") == 0 
                        && part11_std_files->waveforms != 2 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->waveforms = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "data") == 0 
                        && part11_std_files->data != 2 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->data = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "phasefile") == 0 
                        && part11_std_files->phasefile != 2 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->phasefile = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "fdf") == 0 
                        && part11_std_files->fdf != 2 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->fdf = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "snapshot") == 0 
                        && part11_std_files->snapshot != 2 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->snapshot = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "cmdHistory") == 0 
                        && part11_std_files->cmdHistory != 2 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->cmdHistory = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "optional") == 0
                        && strcmp(words[2], "pulseSequence") == 0 
                        && part11_std_files->pulseSequence != 2 
                        && strstr(words[3], "yes") != NULL)
                    part11_std_files->pulseSequence = 1;
		else if(strcmp(words[0], "file") == 0 
                        && strcmp(words[1], "include") == 0) { 
                    if(fileExist(words[2]) && isAdirectory(words[2])) {
                        strcpy(incPath, words[2]);
                    } 
                    if(strlen(incPath) > 0 && !strEndsWith(incPath, "/")) 
			strcat(incPath, "/");
                    if(strlen(incPath) > 0 && strstr(words[3], "yes") != NULL) {
                        incInc = 1;
                    } 
		}
	    }
	}
    }
    fclose(fp);
    RETURN;
}

/**************************/
int recordOff(int argc, char *argv[], int retc, char *retv[])
/**************************/
{
/* dummy command when appearing in the some line with other command(s) */
/* part11 tracking is turned off for those command(s). */ 
    (void) argc;
    (void) argv;
    (void) retc;
    (void) retv;
    RETURN;
}

/**************************/
int getFidName(int argc, char *argv[], int retc, char *retv[])
/**************************/
/* get name only if data saved and fid linked */
/* otherwise name = "" */
{
    int fid_linked;
    string str, root, name;
    string path, curfid_linkpath;
    
    (void) argc;
    (void) argv;
    /* get fid linkpath */
    sprintf(path,"%s%s",curexpdir,"/acqfil/fid");
    fid_linked = file_is_link(path, "", curfid_linkpath );

    if(fid_linked && (strstr(curfid_linkpath, ".REC") != NULL ||
                      strstr(curfid_linkpath, ".rec") != NULL)) {
        strcpy(str,""); 
        strncat(str, curfid_linkpath, strlen(curfid_linkpath)-11);
        brkPath(str, root, name);
    } else if(fid_linked) {
        strcpy(str,""); 
        strncat(str, curfid_linkpath, strlen(curfid_linkpath)-4);
        brkPath(str, root, name); 
    } else strcpy(name, "");

    if (retc > 0)
    {
     	retv[0]=newString(name);
    }

    RETURN;
}

/**************************/
void p11_setcurrfidid()
/**************************/
{
    char str[MAXSTR], sum[MAXSTR];
    int last;
    int nfiles = 1;
    string filename;
    double d;

    if(strstr(currentCmd,"recordOff") != NULL) return;

    if(!part11System && !P_getreal(GLOBAL, "recordSave", &d, 1) && d < 0.5) return;

    if(P_getstring(GLOBAL, "acqmode", str, 1, MAXSTR)) strcpy(str, "");
    if(strcmp(str, "fidscan") == 0 || strcmp(str, "lock") == 0) return;

    sprintf(filename,"%s/acqfil/fid",curexpdir);

    if(getChecksum(filename, sum)) {

	if(isdigit(sum[strlen(sum)-1])) {
            sprintf(str,"%c",sum[strlen(sum)-1]);
            last = atoi(str);
	} else last = 0;

        sprintf(str, "%d", last+nfiles);
    	strcat(sum, str);
    } else strcat(sum, "none");

    P_setstring(PROCESSED, "fidid", sum, 1);

    P_setreal(PROCESSED, "processid", (double)0, 1);

    setDataID(sum, 0);

    P_setstring(PROCESSED,"username",UserName,1);

    P_setstring(PROCESSED,"p11console",p11console,1);

    P_copyvar(PROCESSED,CURRENT,"fidid","fidid");
    P_copyvar(PROCESSED,CURRENT,"processid","processid");
    P_copyvar(PROCESSED,CURRENT,"username","username");
    P_copyvar(PROCESSED,CURRENT,"p11console","p11console");

/* assuming go.c or acqhdl.c set CURRENT time_run */

    okFDAdata=1;
    updateCheckIcon();
    updateLinkIcon();
}

/******************/
static void getStrValues(char* buf, string* words, int* n, char* delimiter)
/******************/
{
/* break a string "buf" into "words" separated by delimiter */
/* return both words and number of words n. */

    int size;
    char *strptr, *tokptr;
    char  str[BUFSIZE];

    size = *n;

    /* remove newline if exists */
    if(buf[strlen(buf)-1] == '\n') {
        strcpy(str, "");
        strncat(str, buf, strlen(buf)-1);
    } else
        strcpy(str, buf);

    strptr = str;
    *n = 0;
    while ((tokptr = (char*) strtok(strptr, delimiter)) != (char *) 0) {

        if(strlen(tokptr) > 0) {
            strcpy(words[*n], tokptr);
            (*n)++;
        }
        strptr = (char *) 0;
    }
}

/******************/
static void getFilenames(string rootpath, string name, string* files, int* n, int nmax)
/******************/
{
/* recursively get file (not dir) names in rootpath/name. */
/* rootpath or name may be empty. */
/* n is number of files and dirs. files contains name/... */ 

    DIR             *dirp;
    struct dirent   *dp;
    string dir, child;

    if(rootpath[strlen(rootpath)-1] != '/')
       sprintf(dir, "%s/%s", rootpath, name);
    else
       sprintf(dir, "%s%s", rootpath, name);

    if(strlen(dir) == 0 || !fileExist(dir)) return;

    if((*n) >= (nmax-1)) {
	return;
    } else if(!isAdirectory(dir)) {
        strcpy(files[*n], name);
        (*n)++;
    } else {
        if ( (dirp = opendir(dir)) ) {
            if(strlen(name) > 0 && name[strlen(name)-1] != '/')
                strcat(name,"/");
            for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {

                if(debug)
                    fprintf(stderr," dir %s %s\n", dir, dp->d_name);
                if (*dp->d_name != '.') {

                    sprintf(child,"%s%s",name,dp->d_name);
                    getFilenames(rootpath, child, files, n, nmax);
                }
            }

            closedir(dirp);
        }
    }
    return;
}

/**************************/
int isUserOwned(string path)
/**************************/
/* 1 user owned */
/* 0 not owned or stat error */
{
    int ival1, ival2;
    struct stat     stat_path, stat_user;
    
    ival1 = stat( path, &stat_path );
    if(ival1 != 0) return 0; 
  
    ival2 = stat( userdir, &stat_user );
    if(ival2 != 0) return 0; 

    if(stat_path.st_uid == stat_user.st_uid) return 1;
    else return 0;
}

/**************************/
void getFileStat(string file, string key, string value)
/**************************/
/* acceptable key: owner, size, mtime... */
{
    struct stat     stat_path;
    int i;
    struct passwd* ps;
/*
  char t_format[] = "%\Y-%\m-%\d %\H:%\M:%\S";
*/
    char t_format[] = "%Y-%m-%d %H:%M:%S";

    i = stat(file, &stat_path );
    if(i == 0) {
        if(strcmp(key,"owner") == 0) {
            ps = (struct passwd*)getpwuid(stat_path.st_uid);
            if(ps != NULL) strcpy(value, ps->pw_name);
            else strcpy(value, "");
        } else if(strcmp(key,"size") == 0) {
            sprintf(value, "%d", (int) stat_path.st_size);
        } else if(strcmp(key,"mtime") == 0) {
            strftime(value, MAXSTR, t_format, localtime((long*)&(stat_path.st_mtime)));
        } else if(strcmp(key,"atime") == 0) {
            strftime(value, MAXSTR,  t_format, localtime((long*)&(stat_path.st_atime)));
        } else if(strcmp(key,"ctime") == 0) {
            strftime(value,  MAXSTR, t_format, localtime((long*)&(stat_path.st_ctime)));
        } else if(strcmp(key,"rdev") == 0) {
            sprintf(value, "%d", (int) stat_path.st_rdev);
        } else if(strcmp(key,"nlink") == 0) {
            sprintf(value, "%d", stat_path.st_nlink);
        } else if(strcmp(key,"uid") == 0) {
            sprintf(value, "%d", stat_path.st_uid);
        } else if(strcmp(key,"gid") == 0) {
            sprintf(value, "%d", stat_path.st_gid);
        } else if(strcmp(key,"blksize") == 0) {
            sprintf(value, "%d", (int) stat_path.st_blksize);
        } else if(strcmp(key,"blocks") == 0) {
            sprintf(value, "%d", (int) stat_path.st_blocks);
        }
    } else {
        strcpy(value, "");
    }

    return;
}

/**************************/
int getFileOwner(int argc, char *argv[], int retc, char *retv[])
/**************************/
{
    string owner;

    if(argc < 2) ABORT;

    if(retc > 0) {
	getFileStat(argv[1], "owner", owner);
	retv[0]= newString(owner); 
    }

    RETURN;
}

/**************************/
int getfilestat(int argc, char *argv[], int retc, char *retv[])
/**************************/
{
    string value;

    if(argc < 3) ABORT;

    if(retc > 0) {
	getFileStat(argv[1], argv[2], value);
	retv[0]= newString(value); 
    }

    RETURN;
}

/**************************/
int getDataStat(int argc, char *argv[], int retc, char *retv[])
/**************************/
/* -1 path is not linked or not exist, or is not .rec, .REC, .fid, or .par */
/* 0 no permision */
/* 1 .fid or .par */
/* 2 .rec */
/* 3 .REC */
{
    int i= -1;
    int linked;
    string exppath, path, curfid_linkpath;

    if(argc < 2) {

        sprintf(exppath,"%s%s",curexpdir,"/acqfil/fid");

        if(!fileExist(exppath)) {
            /* fid does not exist. */
            i = -1;
            strcpy(path, "");
        } else if (isSymLink(exppath) == 0) {
            /* fid is linked */
            linked = file_is_link(exppath, "", curfid_linkpath );
       
            if(!linked) {
                i = -1;
                strcpy(path, "");
            } else {
	  
                if(!isUserOwned(curfid_linkpath)) {
                    i = 0;
                    strcpy(path, "");
                } else {
                    if(strstr(curfid_linkpath, ".rec") != NULL) i = 2;
                    else if(strstr(curfid_linkpath, ".REC") != NULL) i = 3;
                    else if(strstr(curfid_linkpath, ".fid") != NULL) i = 1;
                    else if(strstr(curfid_linkpath, ".par") != NULL) i = 1;

                    if(i == 1 || i == 2) {
                        strcpy(path,""); 
                        strncat(path, curfid_linkpath, strlen(curfid_linkpath)-11);
                    } else {
                        strcpy(path,""); 
                        strncat(path, curfid_linkpath, strlen(curfid_linkpath)-4);
                    }
                }
            }
        } else {
            i = -1;
            strcpy(path, "");
        }

    } else {
	strcpy(path, argv[1]);
	if(strstr(path, ".rec") == NULL 
           && strstr(path, ".REC") == NULL
           && strstr(path, ".fid") == NULL
           && strstr(path, ".par") == NULL) i = -1;
	
	else {
            if(!isUserOwned(path)) i = 0;
            else {
                if(strstr(path, ".rec") != NULL) i = 2;
                else if(strstr(path, ".REC") != NULL) i = 3;
                else if(strstr(path, ".fid") != NULL) i = 1;
                else if(strstr(path, ".par") != NULL) i = 1;
            }
	}
    }
 
    if (retc > 0)
    {
        retv[0]= (char *)realString((float)i);
    }
    if (retc > 1)
    {
        retv[1]= newString(path);
    }

    RETURN;
}

/**************************/
int deleteREC(int argc, char *argv[], int retc, char *retv[])
/**************************/
{

    FILE *fp, *fpp;
    int linked;
    string exppath, path, curfid_linkpath, tmpfile, procparPath;
    string buf, str, root, name;
    char *ptmp;

    (void) retc;
    (void) retv;

    if(argc < 2) {
      Winfoprintf("Usage: deleteREC(fullpath)");
      ABORT;
    }

    strcpy(path, argv[1]);

    if(!isUserOwned(path)) {
      Winfoprintf("Cannot delete %s: permission denied.", path);
      ABORT;
    }

    ptmp = path +strlen(path)-4;
    if ( strcmp(ptmp,".fid") == 0 || strcmp(ptmp,".par") == 0) {
	strcpy(procparPath,path);
	strcat(procparPath,"/procpar");
    } else if( strcmp(ptmp,".REC") == 0 || strcmp(ptmp,".rec") == 0) {
	strcpy(procparPath,path);
	strcat(procparPath,"/acqfil/procpar");
    } else {
      Winfoprintf("Cannot delete %s: not valid path.", path);
      ABORT;
    }
 
    if ( (fpp = fopen(procparPath, "r")) ) {
        p11TempName(tmpfile);
        if ( (fp = fopen(tmpfile, "w")) ) {
/* write audit trail to a tmp file */
	    string words[MAXWORDS];
    	    int nwords;

            fprintf(fp, "path | %s\n", path);

            brkPath(path, root, name);
            fprintf(fp, "record name | %s\n", name);

            P_getstring(GLOBAL, "operator", str, 1, MAXSTR - 1);
            fprintf(fp, "del operator | %s\n", str);

            currentDate(str, MAXSTR);
            fprintf(fp, "time_delete | %s\n", str);

            getFileStat(path, "owner", str);
            fprintf(fp, "owner | %s\n", str);

	    while (fgets(buf,sizeof(buf),fpp)) {
	      if(strstr(buf,"operator_") == buf && fgets(buf,sizeof(buf),fpp)) {
	      	getStrValues(buf, words, &nwords, " ");
                if(nwords > 1) {
		  ptmp = words[1]; ptmp++;	
                  strcpy(str,"");
		  strncat(str,ptmp,strlen(words[1])-2);
		} else strcpy(str,"?");
	        fprintf(fp, "acq operator | %s\n", str); 
	      } else if(strstr(buf,"fidid") == buf && fgets(buf,sizeof(buf),fpp)) {
	      	getStrValues(buf, words, &nwords, " ");
                if(nwords > 1) {
		  ptmp = words[1]; ptmp++;	
                  strcpy(str,"");
		  strncat(str,ptmp,strlen(words[1])-2);
		} else strcpy(str,"?");
	     	fprintf(fp, "fidid | %s\n", str); 
	      } else if(strstr(buf,"fidid") == buf && fgets(buf,sizeof(buf),fpp)) {
	      	getStrValues(buf, words, &nwords, " ");
                if(nwords > 1) {
		  ptmp = words[1]; ptmp++;	
                  strcpy(str,"");
		  strncat(str,ptmp,strlen(words[1])-2);
		} else strcpy(str,"?");
	     	fprintf(fp, "dataid | %s\n", str); 
	      } else if(strstr(buf,"console") == buf && fgets(buf,sizeof(buf),fpp)) {
	      	getStrValues(buf, words, &nwords, " ");
                if(nwords > 1) {
		  ptmp = words[1]; ptmp++;	
                  strcpy(str,"");
		  strncat(str,ptmp,strlen(words[1])-2);
		} else strcpy(str,"?");
	     	fprintf(fp, "console | %s\n", str); 
	      } else if(strstr(buf,"seqfil") == buf && fgets(buf,sizeof(buf),fpp)) {
	      	getStrValues(buf, words, &nwords, " ");
                if(nwords > 1) {
		  ptmp = words[1]; ptmp++;	
                  strcpy(str,"");
		  strncat(str,ptmp,strlen(words[1])-2);
		} else strcpy(str,"?");
	     	fprintf(fp, "seqfil | %s\n", str); 
	      } else if(strstr(buf,"samplename") == buf && fgets(buf,sizeof(buf),fpp)) {
	      	getStrValues(buf, words, &nwords, " ");
                if(nwords > 1) {
		  ptmp = words[1]; ptmp++;	
                  strcpy(str,"");
		  strncat(str,ptmp,strlen(words[1])-2);
		} else strcpy(str,"?");
	     	fprintf(fp, "samplename | %s\n", str); 
	      } else if(strstr(buf,"time_run") == buf && fgets(buf,sizeof(buf),fpp)) {
	      	getStrValues(buf, words, &nwords, " ");
                if(nwords > 1) {
		  ptmp = words[1]; ptmp++;	
                  strcpy(str,"");
		  strncat(str,ptmp,strlen(words[1])-2);
		} else strcpy(str,"?");
	     	fprintf(fp, "time_run | %s\n", str); 
	      } else if(strstr(buf,"time_saved") == buf && fgets(buf,sizeof(buf),fpp)) {
	      	getStrValues(buf, words, &nwords, " ");
                if(nwords > 1) {
		  ptmp = words[1]; ptmp++;	
                  strcpy(str,"");
		  strncat(str,ptmp,strlen(words[1])-2);
		} else strcpy(str,"?");
	     	fprintf(fp, "time_saved | %s\n", str); 
	      } 
 	   } 

           fclose(fp);
	    
/* append tmp file to p11 trash */

           sprintf(str, "%s%s %s %s\n", systemdir, "/p11/bin/writeTrash -f", tmpfile, UserName);
           system(str);

           unlink(tmpfile);
	} else {
      	   Winfoprintf("Cannot delete %s: failed to record audit trail.", path);
      	   ABORT;
	}
        fclose(fpp);
    } else {
      Winfoprintf("Cannot delete %s: not valid data.", path);
      ABORT;
    }

    sprintf(exppath,"%s%s",curexpdir,"/acqfil/fid");

    linked = file_is_link(exppath, "", curfid_linkpath );

    if(linked && strstr(curfid_linkpath, path) != NULL) {
     	    sprintf(exppath,"%s%s",curexpdir,"/acqfil/fid");
            rm_file(exppath);
     	    sprintf(exppath,"%s%s",curexpdir,"/acqfil/log");
            rm_file(exppath);
#ifdef VNMRJ
	    okFDAdata = 0;
            updateCheckIcon(); 
            updateLinkIcon(); 
            writelineToVnmrJ("pnew", "1 file");
            appendJvarlist("file");
#endif 
    }

/* now delete the rec */
    rm_file(path);
    p11_writeAuditTrails_S("deleteREC", path, "");
    Winfoprintf("part11: data %s has been deleted\n", path);	

    RETURN;
}

static void copyFiles(char *safecpPath, char *orig, char *dest)
{
    char path1[MAXPATH], path2[MAXPATH];
    string files[MAXFILES];
    int i, nfiles=0;

    if(!fileExist(orig)) {
	Winfoprintf("p11 copyFiles error: original path %s not exist.",orig);
	return;
    }

    if(!isAdirectory(orig)) {
       if(fileExist(dest)) unlink(dest);
       safecp_file(safecpPath, orig, dest); 
    } else {
      getFilenames(orig, "", files, &nfiles, MAXFILES);
      for(i=0; i<nfiles; i++) {
        sprintf( path1, "%s/%s", orig, files[i]);
        sprintf( path2, "%s/%s", dest, files[i]);
        if(fileExist(path1)) {
           if(fileExist(path2)) unlink(path2);
	   safecp_file(safecpPath, path1, path2);
        }
      }
    }
}

int p11_copyFiles(char *orig, char *dest)
{
   if(!fileExist(orig)) return 0;

   char safecpPath[MAXPATH];
   setSafecpPath(safecpPath, dest);
   copyFiles(safecpPath, orig, dest);
   return 1;
}

/**************************/
int save_optFilesInParam(char* dest, char *param, char *type)
/**************************/
{
   vInfo paraminfo;
   int tree, i;
   char name[MAXSTR], value[MAXSTR], str[MAXSTR];
   char origpath[MAXSTR], destpath[MAXSTR], cmd[MAXSTR];
   char safecpPath[MAXPATH];
   char *strptr, *tokptr;
   int processed=0, all=0;

   tree = GLOBAL;
   if (P_getVarInfo(tree, param,  &paraminfo)) RETURN;

   setSafecpPath(safecpPath, dest);

   if(strcmp(type,"all")==0) all=1;
   if(!P_getstring(CURRENT,"time_processed",value,1, MAXSTR) &&
	strlen(value) > 0) processed=1;
/*
   if(processed) {
      sprintf(origpath,"%s/curpar",curexpdir);
      sprintf(destpath,"%s/curpar",dest);
      if(fileExist(origpath)) copyFiles(safecpPath, origpath, destpath);
   }
*/
   for(i=1; i<=paraminfo.size; i++)
    {
        if(P_getstring(tree, param, str, i, MAXSTR)) continue;

        if (!strrchr(str,':')) continue;

        strptr=str;
        tokptr = (char*) strtok(strptr, ":");
        if(!tokptr || strlen(tokptr) < 1) continue;

        strcpy(name, tokptr);
	if(strcmp(name,"fid") == 0 || strcmp(name,"procpar") == 0
		|| strcmp(name,"log") == 0 || strcmp(name,"text") == 0) continue;

        strptr = (char *) 0;
        tokptr = (char*) strtok(NULL, ":");
        if(!tokptr || strlen(tokptr) < 1) continue;

        strcpy(value, tokptr);
        if(strcmp(value,"no") == 0) continue;

//        Winfoprintf("Save optional file %s/%s\n",dest,name);

        if(all && strcmp(name,"global") == 0) {
	   sprintf(destpath, "%s%s", dest, "/global");
           if (P_save(GLOBAL,destpath))
           {   Werrprintf("Problem saving global parameters in '%s'.",destpath);
               ABORT;
           }
        } else if(all && strcmp(name,"conpar") == 0) {
	   sprintf(destpath, "%s%s", dest, "/conpar");
           if (P_save(SYSTEMGLOBAL,destpath))
           {   Werrprintf("Problem saving conpar parameters in '%s'.",destpath);
               ABORT;
           }
        } else if(all && strcmp(name,"shims") == 0) {
	   execString("prepRecFiles('shims')\n");
           sprintf(origpath,"%s/%s",curexpdir, "shims");
           sprintf(destpath,"%s/%s",dest, "shims");
	   if(fileExist(origpath)) copyFiles(safecpPath, origpath, destpath);
        } else if(all && strcmp(name,"waveforms") == 0) {
	   execString("prepRecFiles('waveforms')\n");
           sprintf(origpath,"%s/%s",curexpdir, "shapelib");
           sprintf(destpath,"%s/%s",dest, "shapelib");
	   if(fileExist(origpath)) copyFiles(safecpPath, origpath, destpath);
        } else if(all && strcmp(name,"pulseSequence") == 0) {
	   execString("prepRecFiles('pulseSequence')\n");
           sprintf(origpath,"%s/%s",curexpdir, "psglib");
           sprintf(destpath,"%s/%s",dest, "psglib");
	   if(fileExist(origpath)) copyFiles(safecpPath, origpath, destpath);
        } else if(processed && strcmp(name,"phasefile") == 0) {
	   sprintf(origpath, "%s%s", curexpdir, "/datdir/phasefile");
           sprintf(destpath, "%s%s", dest, "/datdir/phasefile");
           if(fileExist(origpath)) copyFiles(safecpPath, origpath, destpath); 
        } else if(processed && strcmp(name,"data") == 0) {
	   sprintf(origpath, "%s%s", curexpdir, "/datdir/data");
           sprintf(destpath, "%s%s", dest, "/datdir/data");
           if(fileExist(origpath)) copyFiles(safecpPath, origpath, destpath); 
        } else if(processed && strcmp(name,"snapshot") == 0 && !Bnmr) {
           int sec = 0;
	   string path1;
	   FILE *fp;
	   sprintf(destpath, "%s%s", dest, "/spec.jpeg");
    	   if(fileExist(destpath)) unlink(destpath);
           sprintf(cmd, "%s%s%s", "vnmrjcmd('GRAPHICS', 'cdump', '", dest, "/spec.', 'jpeg')\n");
           //Winfoprintf("Save snapshot file %s\n",cmd);
           execString(cmd);
           strcpy(path1,dest);
	   strcat(path1,"/spec.jpeg");
           while(sec < 5 && !fileExist(path1)) {
            sleep(1);
            sec++;
           }
           while(fileExist(path1) && !(fp = fopen(path1, "r"))) {
            sleep(1);
           }
        } else if(all && strcmp(name,"usermaclib") == 0) {
           sprintf(origpath,"%s/%s",curexpdir, "maclib");
           sprintf(destpath,"%s/%s",dest, "maclib");
	   if(fileExist(origpath)) copyFiles(safecpPath, origpath, destpath);
        } else if(all && strcmp(name,"cmdHistory") == 0) {
	   if(cmdHistoryFp != NULL) fflush(cmdHistoryFp);
           sprintf(destpath, "%s%s", dest, "/cmdHistory");
           if(fileExist(cmdHisFile)) copyFiles(safecpPath, cmdHisFile, destpath);
        }
    }
   RETURN;
}

// stdfiles is a global parameter contains a list standard files to be saved
// optfiles is a global parameter contains a list optional files to be saved
/**************************/
int save_optFiles(char* dest, char *type)
/**************************/
{
   flush(0,NULL,0,NULL);
   save_optFilesInParam(dest, "stdfiles", type);
   save_optFilesInParam(dest, "optfiles", type);
   p11_mkchecksums(dest);
   RETURN;
}

int getP11Dir(int argc, char *argv[], int retc, char *retv[])
{
   if(retc < 1) RETURN;

   string opt="user";
   if(argc > 1) strcpy(opt,argv[1]);

   if(strcmp(opt,"system") == 0) {
     retv[0] = newString(sysFDAdir); 
   } else {
     getUserPart11Dirs(UserName, userPart11Dirs, &nuserPart11Dirs);
     if(nuserPart11Dirs > 0 && strlen(userPart11Dirs[0]) > 0) 
	retv[0] = newString(userPart11Dirs[0]); 
     else {
	string str;
	sprintf(str,"%s/data/%s",sysFDAdir,UserName);
	retv[0] = newString(str); 
     }
   }

   RETURN;
}

int canWrite(int argc, char *argv[], int retc, char *retv[])
{
   string safecpPath;
   if(argc < 2 || retc < 1) RETURN;
   
   setSafecpPath(safecpPath, argv[1]);
   if(strstr(safecpPath,"/p11/bin/safecp")) retv[0] = realString(0);
   else retv[0] = realString(1);
    
   RETURN;
}

int p11datamirror(int argc, char *argv[], int retc, char *retv[])
{
   string safecpPath, dest, orig;
   if(argc < 3) RETURN;
   
   strcpy(orig,argv[1]);
   strcpy(dest,argv[2]);
   if(!fileExist(orig)) {
	Winfoprintf("p11datamirror error: original path %s not exist.",orig);
	ABORT;
   }

   setSafecpPath(safecpPath, argv[2]);
   copyFiles(safecpPath, orig, dest);
   
   RETURN; 
}

void p11_checkData() {

    double d;
    if(strstr(currentCmd,"recordOff") != NULL) return;

    if(!part11System && !P_getreal(GLOBAL, "recordSave", &d, 1) && d < 0.5) return;

    p11_createpars();

    okFDAdata = 0;
    updateCheckIcon(); 
    updateLinkIcon(); 
}

