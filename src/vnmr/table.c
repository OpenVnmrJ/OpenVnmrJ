/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*------------------------------------------------------------------------------
|
|	table.c
|
|	This module contains code to scan the available commands and
|	return a pointer to the function.  The command names and
|	pointer addresses are located in commands.h
+-----------------------------------------------------------------------------*/
#include <stdio.h>
#include <unistd.h>
#include <sys/file.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "tools.h"
#include "vnmrsys.h"
#include "wjunk.h"

/*  Each command has four features.
 *  searchName is a string which is compared to an input name. If a 
 *  match is found, then useName is delivered as argv[0] to the function
 *  pointed to as func.  The graphcmd field is used to determine if a 
 *  function should be called automatically if it is active and a parameter
 *  has been changed.  Typically, searchName and useName are identical.
 *  The hidecommand command will change searchName so that a macro with
 *  the same name will be used.  For example, dg will become Dg. If the
 *  renamed command is called,  useName is delivered to the function so that
 *  potential aliases will be recognized.  Note that the REEXEC feature
 *  will not work for hidden names.  This is because the graphics or text display
 *  will be set with the useName value, (for example, dg) and this will not
 *  match any of the searchName values (since dg is now Dg).
 *  searchName must be defined as char  searchName[] rather than char *searchName
 *  Otherwise, changing searchName[0] can fail
 */
struct _cmd { char   searchName[32];
              char   *useName;
	      int   (*func)();
	      int    graphcmd;
	      int    redoType;
	    };
typedef struct _cmd cmd;

struct _cmd_t {
              char   *useName;
	      int   (*func)();
	      int    graphcmd;
	      int    redoType;
            };
   
   /****************************************
     redoType:
         0: none graphics command
         1: reexec graphics command only
         2: reexec graphics command with argment 'again'
         3: reexec graphics command only whith argument 'redisplay'
         4: a processing func and will run graphics func later
         5: will save command and arguments for redisplay if this do graphics
         6: will save command and arguments for redisplay
         7: will save the lastest command (multiple calls) for redisplay
        10: reexec in print screen mode only
   *********************************************/

typedef struct _cmd_t cmd_t;


#define REEXEC 		1
#define NO_REEXEC	0

extern int Bnmr;
#ifdef VNMRJ
extern int jShowInput;
#endif


/* commands MUST be alphabetized */
extern int acosy();
extern int acq();
extern int acqdisp();
extern int acqhwcmd();
extern int acqproc_msge();
extern int acqsend();
extern int acqstatus();
extern int acqupdt();
extern int addsub();
extern int adept();
extern int ai();
extern int analyze();

#ifdef VNMRJ
extern int annotation();
#endif

extern int aph();
extern int appdir();
extern int appendCmd(int argc, char *argv[], int retc, char *retv[]);
extern int atCmd();
extern int auargs(int argc, char *argv[], int retc, char *retv[]);
extern int autocmd();
extern int autogo();
extern int autoname();
extern int autoq(int argc, char *argv[], int retc, char *retv[]);
extern int autora();
extern int autosa();
extern int averag();
extern int axis_info();
extern int bgmode_is();
extern int banner();
extern int bc();
extern int beeper();
extern int bigendian();
extern int bphase();
extern int calcdim();
extern int calcECC();
extern int calibxy();
extern int cexpCmd();
extern int mkchsums();
extern int chchsums();
extern int chkname(int argc, char *argv[], int retc, char *retv[]);
extern int Chmod(int argc, char *argv[], int retc, char *retv[]);
extern int cmdHistory(int argc, char *argv[], int retc, char *retv[]);
extern int cmdlineOK(int argc, char *argv[], int retc, char *retv[]);
extern int clear();
extern int continprepare();
extern int continread();
extern int convertbruker(int argc, char *argv[], int retc, char *retv[]);
extern int convertdelta(int argc, char *argv[], int retc, char *retv[]);
extern int convertqone(int argc, char *argv[], int retc, char *retv[]);
extern int convertmagritek(int argc, char *argv[], int retc, char *retv[]);
extern int convertoxford(int argc, char *argv[], int retc, char *retv[]);
extern int cpFilesInFile();
extern int create();
extern int createparams();
extern int CSschedule(int argc, char *argv[], int retc, char *retv[]);
extern int data_dir();		/* magnetom */
extern int datafit(int argc, char *argv[], int retc, char *retv[]);
extern int dc2d();
extern int dcon();
extern int dconi();
extern int ddf();
extern int ddif();
#ifdef VNMRJ
extern int decc_compare();
extern int decc_load();
extern int decc_purge();
extern int decc_purgewhat();
extern int decc_save();
#endif
extern int debuger();
extern int decay_gen();
#ifndef VNMRJ
extern int delem();
#endif
extern int deleteREC();
extern int delexp();
extern int delexpdata();
extern int dels();
extern int destroy();
extern int destroygroup();
extern int devicenames();
extern int df2d();
extern int dfid();
#ifdef VNMRJ
extern int dfid2();
#endif
extern int dfww();
extern int disp1Dmap();
extern int disp3Dmap();
extern int dispAcqError();
extern int display();
extern int dg();
extern int dlalong();
extern int dli();
extern int dll();
extern int dosyfit();
extern int dpcon();
extern int dpf();
extern int dpir();
extern int dps();
extern int pps();
extern int drawxy();
extern int dres();
extern int ds();
extern int dsn();
extern int dsp();
extern int dsww();
extern int ds2d();
extern int dscale();
extern int echo();
#ifndef VNMRJ
extern int eleminfo();
#endif
extern int encipher(int argc, char *argv[], int retc, char *retv[]);
/* extern int epi_recon();      */
extern int ernst();
extern int errlog();
extern int exec(int argc, char *argv[], int retc, char *retv[]);
extern int execexp(int argc, char *argv[], int retc, char *retv[]);
extern int exists();
extern int expactive();
extern int export_files();			/* magnetom */
extern int expl_cmd();
extern int f_cmd();
extern int fidarea();
extern int fiddle();
extern int fidmax();
extern int fidproc(int argc, char *argv[], int retc, char *retv[]);
#ifndef VNMRJ
extern int files();
extern int filesinfo();
#endif
extern int fitspec();
extern int flashc();			/* flashc reformatting */
extern int flip();
extern int flush();
extern int flush2(int argc, char *argv[], int retc, char *retv[]);
extern int flushpars(int argc, char *argv[], int retc, char *retv[]);
extern int foldcc();
extern int foldj();
extern int foldt();
extern int format();
#ifdef VNMRJ
extern int frameAction();
//extern int frameCmd();
#endif
extern int fp();
extern int f_read();
extern int fsave();
extern int ft();
extern int ft2d();
extern int full();
extern int graphoff();
extern int getdatadim();
extern int getDataStat();
extern int getdfstat();
extern int getdsstat();
extern int getMaxIndex();
extern int getFidName();
extern int getFileOwner();
extern int getfile();
extern int getfilestat();
extern int getLastData();
extern int getMouseButtons();
extern int readheader(int argc, char *argv[], int retc, char *retv[]);
extern int readpars();
extern int readpar();
extern int getP11Dir();
extern int canWrite();
extern int p11datamirror();
extern int getplottertype();
extern int getplotterpens();
extern int getRandom();
extern int getstatus();
extern int gettype();
extern int getvalue4name();
extern int getvalue();
extern int gin();
extern int gmaplistfunc();
extern int gplan();
#ifdef VNMRJ
extern int gplan_update();
#endif
extern int gradfit();
extern int graph_is();
extern int groupcopy();
extern int gxyzfit();
extern int gxyzmap();
extern int gxyzmapo();
extern int gzfit();
extern int h2cal();
extern int hpa();
extern int help();
extern int ia_start();
extern int ilfid();
extern int import_files();			/* magnetom */
extern int initTclDg();
extern int inset();
extern int interact();
static int iscmdopen();
extern int isDataRecord();
extern int isFDAdata();
extern int isFDAdir();
extern int isFDAsystem();
extern int isRecUpdated();
static int isvnmrj();
extern int jacqupdtOn();
extern int jacqupdtOff();
extern int jacqupdt();
extern int jexp();
extern int downsizefid();
extern int replacetraces();
#ifdef VNMRJ
extern int imagefile();
extern int isimagebrowser();
extern int isCSIMode();
extern int jFunc();
extern int jEvent();
extern int jMove();
extern int jRegion();
extern int jFunc2();
extern int jEvent2();
extern int jMove2();
extern int jRegion2();
extern int jMove3();
extern int setpage();
#endif
extern int iplot();
extern int jplot();
extern int large();
extern int leftshiftfid(int argc, char *argv[], int retc, char *retv[]);
extern int length();
extern int ll2d();
extern int ln();
#ifdef VNMRJ
extern int liMMap();
extern int llMMap();
extern int locki();
extern int locklc();
#endif
extern int lookup();
extern int lpcmd();
extern int macroCat();
extern int macroCp();
extern int macroDir();
extern int macroLd();
extern int macroLine(int argc, char *argv[], int retc, char *retv[]);
extern int macroRm();
extern int magicvars();
extern int magnetom();			/* magnetom */
extern int makefid();
extern int makeslice();
extern int mark();
extern int menu();
extern int mfdata();
extern int mp();
extern int mstat();
extern int mspec();
int mvCmd();
int listvnmrcmd();
extern int nextexp(int argc, char *argv[], int retc, char *retv[]);
extern int noise();
extern int nmrExit();
extern int nmr_draw();
extern int nmr_write();
extern int writef1(int argc, char *argv[], int retc, char *retv[]);
extern int writefile();
extern int writespectrum();
extern int Off();
extern int onCancel(int argc, char *argv[], int retc, char *retv[]);
extern int output_sdf();			/* ImageBrowser */
extern int ovjCount(int argc, char *argv[], int retc, char *retv[]);
extern int p11_action();
extern int p11_switchOpt();
extern int page();
extern int paramcopy(int argc, char *argv[], int retc, char *retv[]);
extern int pcmap();
extern int do_pcss();
extern int peak2d();
extern int phase_fid();
extern int pipeRead();
extern int plot1Dmap();
extern int plot3Dmap();
extern int powerfit();
extern int print_on();
extern int printoff();
extern int profile_int();
extern int proj();
extern int prune();
extern int purgeCache();
extern int quadtt();
extern int raster_dump();
extern int readf1(int argc, char *argv[], int retc, char *retv[]);
extern int readfile(int argc, char *argv[], int retc, char *retv[]);
extern int readhw();
extern int readlk();
extern int readparam();
extern int readspectrum(int argc, char *argv[], int retc, char *retv[]);
extern int readstr(int argc, char *argv[], int retc, char *retv[]);
extern int real();
extern int recon();
extern int recon_all();
extern int recon3D();
extern int recon_mm();
extern int recordOff();
extern int releaseConsole();
extern int resume();
extern int region();
extern int rfdata();
extern int rights();
extern int rotate();
extern int rsdata();
extern int rt();
extern int rtphf();
extern int rts();
extern int rtx();
extern int Rinput();
extern int s_cmd();
extern int savecolors();
extern int savefdfspec();
extern int scalefid(int argc, char *argv[], int retc, char *retv[]);
extern int selbuf();
extern int selecT();
#ifdef VNMRJ
extern int serverport();
#endif
extern int set_mag_login();			/* magnetom */
extern int set_node_name();			/* magnetom */
extern int set_password();			/* magnetom */
extern int set_remote_dir();		/* magnetom */
extern int set_user_name();			/* magnetom */
extern int set3dproc();
extern int setcolor();
extern int setdgroup();
extern int setenumeral();
extern int setfont();
extern int setfrq();
extern int setgrid();
extern int setgroup();
extern int sethw();
extern int sethwshim(int argc, char *argv[], int retc, char *retv[]);
extern int setlimit();
extern int setplotdev();
extern int setprintdev();
extern int setprotect();
extern int settype();
extern int setvalue();
extern int setvalue4name();
extern int setwin();
extern int setButtonState();
extern int shellcmds(int argc, char *argv[], int retc, char *retv[]);
extern int shimamp(int argc, char *argv[], int retc, char *retv[]);
extern int shimnames();
extern int show_local_files();		/* magnetom */
extern int show_remote_files();		/* magnetom */
extern int showbuf();
extern int small();
extern int solvinfo();
extern int sortCmd(int argc, char *argv[], int retc, char *retv[]);
extern int spa();
extern int spins();
extern int splmodprepare();
extern int splmodread();
extern int sread();
extern int string();
extern int Strstr();
extern int substr();
extern int svf();
extern int svr_FDA();
extern int svphf();
extern int svs();
extern int swrite();
extern int takeFocus();
extern int tcdata();
extern int tclCmd();
extern int tempcal();
extern int testPart11();
extern int teststr();
extern int text();
extern int text_is();
extern int tnmr_draw();
#ifdef VNMRJ
extern int trackCursor();
#endif
extern int tune();
extern int unit();
extern int unixtime();
extern int vnmrSleep(int argc, char *argv[], int retc, char *retv[]);
extern int vnmr_unlock();
extern int vnmrInfo();
#ifdef VNMRJ
extern int vnmrj_close();
#endif
extern int vnmrjcmd();
extern int werr();
#ifdef VNMRJ
extern int window_repaint();
extern int window_refresh();
extern int worst1();
#endif
extern int writefid();
extern int writeparam();
#ifdef VNMRJ
extern int writeToVnmrJcmd();
#endif
extern int wrspec();
extern int wsram();
extern int wti();
#ifdef VNMRJ
extern int xoroff();
extern int xoron();
#endif
extern int zerofillfid(int argc, char *argv[], int retc, char *retv[]);
extern int zeroneg();
static int numCmds = 0;
static int vcount = 0;
static int vIndex = 0;
static int aipcount = 0;

extern int clrshellcnt(int argc, char *argv[], int retc, char *retv[]);
extern int showshellcnt(int argc, char *argv[], int retc, char *retv[]);

static double	ctimes[32];
static int clock_index=0;
static int pushtime(int argc, char *argv[], int retc, char *retv[]) {
	double		dtime;
	struct timeval	tv;
	gettimeofday( &tv, NULL );

	dtime=( (double) (tv.tv_sec) + ((double) (tv.tv_usec)) / 1.0e6 );
	ctimes[clock_index]=dtime;
	Winfoprintf("%d BEGIN TIME ",clock_index);

	clock_index++;
	clock_index=clock_index>31?31:clock_index;
	return(0);
}
static int poptime(int argc, char *argv[], int retc, char *retv[]) {
	double		dtime;
	struct timeval	tv;
	gettimeofday( &tv, NULL );
	dtime=( (double) (tv.tv_sec) + ((double) (tv.tv_usec)) / 1.0e6 );
	clock_index--;
	clock_index=clock_index<0?0:clock_index;
	Winfoprintf("%d END TIME = %g seconds",clock_index,dtime-ctimes[clock_index]);
	return(0);
}

static int deltatime(int argc, char *argv[], int retc, char *retv[]) {
	double		dtime;
	struct timeval	tv;
	gettimeofday( &tv, NULL );
	dtime=( (double) (tv.tv_sec) + ((double) (tv.tv_usec)) / 1.0e6 );
	int index=clock_index-1;
	index=index<0?0:index;
	Winfoprintf("%d DELTA TIME = %g seconds",index,dtime-ctimes[index]);
	return(0);
}

static cmd_t vnmr_table[] = {
#ifdef VNMRJ
    {"isimagebrowser", isimagebrowser, NO_REEXEC, 0},
	{"jFunc"      , jFunc,		NO_REEXEC, 0},
	{"jEvent"     , jEvent,		NO_REEXEC, 0},
	{"jMove"      , jMove,		NO_REEXEC, 0},
	{"jFunc2"      , jFunc2,		NO_REEXEC, 0},
	{"jEvent2"     , jEvent2,	NO_REEXEC, 0},
	{"jMove2"      , jMove2,	NO_REEXEC, 0},
	{"jMove3"      , jMove3,	NO_REEXEC, 0},
#endif
	{"exists"     , exists,		NO_REEXEC, 10},
	{"substr"     , substr,		NO_REEXEC, 10},
	{"ds"         , ds,		REEXEC, 3},
	{"dconi"      , dconi,	        REEXEC, 2},
	{"df"         , dfid,	        REEXEC, 1},
#ifdef VNMRJ
	{"dg"         , dg,		NO_REEXEC, 0},
#else
	{"dg"         , dg,		REEXEC, 0},
#endif
	{"dfid"       , dfid,	        REEXEC, 1},
#ifdef VNMRJ
	{"dfid2"      , dfid2,	        REEXEC, 1},
#endif
	{"aa"		, acqproc_msge,	NO_REEXEC, 0},
	{"abortallacqs"	, acqproc_msge,	NO_REEXEC, 0},
	{"abs"        , ln,		NO_REEXEC, 10},
	{"acos"       , ln,		NO_REEXEC, 10},
	{"acosy"      , acosy,		NO_REEXEC, 10},
	{"acosyold"   , acosy,		NO_REEXEC, 0},
	{"Acq"        , acq,		NO_REEXEC, 0},
	{"acqdebug"   , acqproc_msge,	NO_REEXEC, 0},
	{"acqdequeue" , acqproc_msge,	NO_REEXEC, 0},
	{"acqdisp"    , acqdisp,	NO_REEXEC, 0},
	{"acqi"       , ia_start,	NO_REEXEC, 0},
	{"acqipctst"  , acqproc_msge,	NO_REEXEC, 0},
	{"acqsend"    , acqsend,	NO_REEXEC, 0},
	{"acqstatus"  , acqstatus,	NO_REEXEC, 0},
	{"add"        , addsub,		NO_REEXEC, 10},
	{"addi"       , addsub,		NO_REEXEC, 1},
	{"adept"      , adept,		NO_REEXEC, 10},
	{"ai"         , ai,		NO_REEXEC, 10},
	{"analyze"    , analyze,	NO_REEXEC, 10},
#ifdef VNMRJ
	{"annotation" , annotation,	NO_REEXEC, 0},
	{"pannotation" , annotation,	NO_REEXEC, 0},
#endif
	{"ap"         , dg,		NO_REEXEC, 10},
	{"aph"        , aph,		NO_REEXEC, 10},
	{"aph0"       , aph,		NO_REEXEC, 10},
	{"aphb"       , aph,		NO_REEXEC, 10},
	{"aphold"     , aph,		NO_REEXEC, 10},
	{"appdir"     , appdir,		NO_REEXEC, 0},
	{"append"     , appendCmd,	NO_REEXEC, 0},
	{"appendstr"  , appendCmd,	NO_REEXEC, 0},
	{"asin"       , ln,		NO_REEXEC, 10},
	{"atan"       , ln,		NO_REEXEC, 10},
	{"atan2"      , ln,		NO_REEXEC, 10},
	{"atcmd"      , atCmd,		NO_REEXEC, 0},
	{"auargs"     , auargs, 	NO_REEXEC, 0},
	{"auto"       , autocmd,	NO_REEXEC, 0},
	{"autogo"     , autogo, 	NO_REEXEC, 0},
	{"autoname"   , autoname, 	NO_REEXEC, 0},
	{"autoq"      , autoq,  	NO_REEXEC, 0},
	{"autora"     , autora, 	NO_REEXEC, 0},
	{"autosa"     , autosa, 	NO_REEXEC, 0},
	{"av"         , ai,		NO_REEXEC, 10},
	{"av1"	     ,	ai,		NO_REEXEC, 10},
	{"av2"	     ,	ai,		NO_REEXEC, 10},
	{"averag"     , averag,		NO_REEXEC, 10},
	{"axis"       , axis_info,	NO_REEXEC, 10},
	{"bgmode_is"  , bgmode_is, 	NO_REEXEC, 0},
	{"banner"     , banner,		NO_REEXEC, 2},
	{"bc"         , bc,		NO_REEXEC, 10},
	{"bc2d"       ,	bc,		NO_REEXEC, 10},
	{"beepoff"    , beeper,		NO_REEXEC, 0},
	{"beepon"     , beeper,		NO_REEXEC, 0},
	{"bigendian"  , bigendian,	NO_REEXEC, 0},
	{"box"        , nmr_draw,	NO_REEXEC, 5},
	{"bph"	      , bphase,	NO_REEXEC, 0},
	{"calcdim"    , calcdim,	NO_REEXEC, 10},
	{"calcECC"    , calcECC,	NO_REEXEC, 10},
#ifdef VNMRJ
	{"calibxy"    , calibxy,	NO_REEXEC, 0},
#endif
	{"cat"	     , shellcmds,	NO_REEXEC, 0},
	{"cd"	     , shellcmds,	NO_REEXEC, 0},
	{"cdc"        , ai,		NO_REEXEC, 10},
	{"center"     , full,		NO_REEXEC, 10},
	{"CEXP"       , cexpCmd,	NO_REEXEC, 0},
	{"chchsums"   , chchsums,	NO_REEXEC, 0},
	{"mkchsums"   , mkchsums,	NO_REEXEC, 0},
	{"chkname"    , chkname,	NO_REEXEC, 10},
	{"chmod"      , Chmod,		NO_REEXEC, 0},
	{"clear"      , clear,		NO_REEXEC, 10},
	{"clradd"     , addsub,		NO_REEXEC, 10},
	{"cmdHistory" , cmdHistory,	NO_REEXEC, 0},
	{"cmdlineOK"  , cmdlineOK,	NO_REEXEC, 0},
	{"copy"	      , shellcmds,	NO_REEXEC, 0},
	{"copyf"      , appendCmd,	NO_REEXEC, 0},
	{"copystr"    , appendCmd,	NO_REEXEC, 0},
        {"continprepare", continprepare,NO_REEXEC, 0},
        {"continread" , continread,     NO_REEXEC, 0},
        {"convertbruker", convertbruker,NO_REEXEC, 0},
        {"convertdelta", convertdelta,  NO_REEXEC, 0},
        {"convertqone", convertqone,  NO_REEXEC, 0},
        {"convertmagritek", convertmagritek,NO_REEXEC, 0},
        {"convertoxford", convertoxford,NO_REEXEC, 0},
	{"cos"	     , ln,		NO_REEXEC, 10},
	{"cp"	     , shellcmds,	NO_REEXEC, 0},
	{"cpFilesInFile", cpFilesInFile, NO_REEXEC, 0},
	{"create"     , create,		NO_REEXEC, 0},
	{"createparams", createparams,   NO_REEXEC, 0},
	{"CSschedule" , CSschedule,     NO_REEXEC, 0},
	{"ctext"      , text,		NO_REEXEC, 0},
	{"cz"         , dli,		NO_REEXEC, 0},
	{"da"         , dg,		NO_REEXEC, 0},
	{"data_dir"   , data_dir,	NO_REEXEC, 0},
	{"datafit"    , datafit,	NO_REEXEC, 0},
	{"db"         , ai,		NO_REEXEC, 10},
	{"dc"         , ai,		NO_REEXEC, 10},
	{"dc2d"       , dc2d,		NO_REEXEC, 10},
	{"dcon"       , dcon,	        NO_REEXEC, 3},
	{"dconn"      , dcon,	        NO_REEXEC, 10},
	{"ddf"        , ddf,		NO_REEXEC, 10},
	{"ddff"       , ddf,		NO_REEXEC, 10},
	{"ddfp"       , ddf,		NO_REEXEC, 10},
	{"ddif"       , ddif,		NO_REEXEC, 10},
#ifdef VNMRJ
	{"decc_compare", decc_compare,	NO_REEXEC, 0},
	{"decc_load"  , decc_load,	NO_REEXEC, 0},
	{"decc_purge" , decc_purge,	NO_REEXEC, 0},
	{"decc_purgewhat",decc_purgewhat,NO_REEXEC, 0},
	{"decc_save"  , decc_save,	NO_REEXEC, 0},
#endif
	{"debug"      , debuger,	NO_REEXEC, 0},
	{"decay_gen"  , decay_gen,	NO_REEXEC, 0},
	{"delbuf"     , selbuf,		NO_REEXEC, 0},
#ifndef VNMRJ
	{"delem"      , delem,		NO_REEXEC, 0},
#endif
	{"delete"     , shellcmds,	NO_REEXEC, 0},
	{"deleteREC"  , deleteREC,	NO_REEXEC, 0},
	{"DELEXP"     , delexp,		NO_REEXEC, 0},
	{"delexpdata" , delexpdata,	NO_REEXEC, 0},
	{"dels"       , dels,		NO_REEXEC, 0},
	{"destroy"    , destroy,	NO_REEXEC, 0},
	{"destroygroup",destroygroup,	NO_REEXEC, 0},
	{"devicenames",devicenames,	NO_REEXEC, 0},
	{"df2d"       , df2d,		NO_REEXEC, 1},
	{"dfs"        , dfww,		NO_REEXEC, 5},
	{"dfsa"       , dfww,		NO_REEXEC, 5},
	{"dfsan"      , dfww,		NO_REEXEC, 5},
	{"dfsh"       , dfww,		NO_REEXEC, 5},
	{"dfshn"      , dfww,		NO_REEXEC, 5},
	{"dfsn"       , dfww,		NO_REEXEC, 5},
	{"dfww"       , dfww,		NO_REEXEC, 5},
	{"dfwwn"      , dfww,		NO_REEXEC, 5},
	{"dir"	     , shellcmds,	NO_REEXEC, 0},
	{"display"    , display,	NO_REEXEC, 0},
#ifdef VNMRJ
	{"disp1Dmap"    , disp3Dmap,	NO_REEXEC, 10},
	{"disp3Dmap"    , disp3Dmap,	NO_REEXEC, 10},
#endif
	{"dlalong"    , dlalong,	NO_REEXEC, 0},
        {"do_pcss"    , do_pcss,        NO_REEXEC, 10},
	{"dosyfit"    , dosyfit,	NO_REEXEC, 10},
	{"dosyfitv"   , dosyfit,	NO_REEXEC, 10},
	{"dpcon"      , dpcon,	        NO_REEXEC, 1},
	{"dpconn"     , dpcon,	        NO_REEXEC, 5},
	{"dpf"        , dpf,	        NO_REEXEC, 7},
	{"dpir"       , dpir,	        NO_REEXEC, 10},
	{"dpirN"      , dpir,	        NO_REEXEC, 5},
	{"dps"        , dps,	        NO_REEXEC, 2},
	{"dpsn"       , dps,	        NO_REEXEC, 2},
        {"dpsvj"      , dps,            NO_REEXEC, 0},
	{"draw"       , nmr_draw,	NO_REEXEC, 5},
	{"drawxy"     , drawxy,		NO_REEXEC, 10},
	{"dres"       , dres,		NO_REEXEC, 10},
	{"dsn"        , dsn,		NO_REEXEC, 10},
	{"dsp"        , dsp,		NO_REEXEC, 10},
	{"dss"        , dsww,		NO_REEXEC, 5},
	{"dssa"       , dsww,		NO_REEXEC, 5},
	{"dssan"      , dsww,		NO_REEXEC, 5},
	{"dssh"       , dsww,		NO_REEXEC, 5},
	{"dsshn"      , dsww,		NO_REEXEC, 5},
	{"dssn"       , dsww,		NO_REEXEC, 5},
	{"dsww"       , dsww,		NO_REEXEC, 5},
	{"dswwn"      , dsww,		NO_REEXEC, 5},
	{"ds2d"       , ds2d,		NO_REEXEC, 5},
	{"ds2dn"      , ds2d,		NO_REEXEC, 5},
	{"dscale"     , dscale,		NO_REEXEC, 0},
	{"dscalen"    , dscale,		NO_REEXEC, 0},
	{"echo"       , echo,		NO_REEXEC, 0},
#ifndef VNMRJ
	{"eleminfo"   , eleminfo,	NO_REEXEC, 0},
#endif
	/*	{"epi_recon"  , epi_recon,     	NO_REEXEC, 0}, */
	{"encipher"   , encipher,	NO_REEXEC, 0},
	{"ernst"      , ernst,		NO_REEXEC, 0},
	{"errlog"     , errlog,		NO_REEXEC, 0},
	{"exec"       , exec,		NO_REEXEC, 12},
	{"execexp"    , execexp,	NO_REEXEC, 12},
	{"vnmrexit"   , nmrExit,	NO_REEXEC, 0},
	{"exp"        , ln,		NO_REEXEC, 0},
	{"expactive"  , expactive,	NO_REEXEC, 0},
	{"expl"       , expl_cmd,	NO_REEXEC, 5},
	{"exptime"    , dps,	        NO_REEXEC, 0},
	{"f"          , f_cmd,		NO_REEXEC, 0},
	{"fidarea"    , fidarea,	NO_REEXEC, 0},
	{"fiddle"     , fiddle,		NO_REEXEC, 5},
	{"fiddled"    , fiddle,		NO_REEXEC, 5},
	{"fiddleu"    , fiddle,		NO_REEXEC, 5},
	{"fiddle2d"   , fiddle,		NO_REEXEC, 5},
	{"fiddle2D"   , fiddle,		NO_REEXEC, 5},
	{"fiddle2Dd"  , fiddle,		NO_REEXEC, 5},
	{"fiddle2dd"  , fiddle,		NO_REEXEC, 5},
	{"fidmax"     , fidmax,		NO_REEXEC, 10},
	{"fidproc"    , fidproc,	NO_REEXEC, 10},
	{"wfidproc"   , fidproc,	NO_REEXEC, 10},
	{"fitspec"    , fitspec,	NO_REEXEC, 10},
#ifndef VNMRJ
	{"files"      , files,		NO_REEXEC, 0},
	{"filesinfo"  , filesinfo,	NO_REEXEC, 0},
#endif
	{"flashc"     , flashc,		NO_REEXEC, 0},
	{"flip"       , flip,		NO_REEXEC, 0},
	{"flush"      , flush,		NO_REEXEC, 0},
	{"flush2"     , flush2,		NO_REEXEC, 0},
	{"flushpars"  , flushpars,	NO_REEXEC, 0},
	{"fr"         , s_cmd,		NO_REEXEC, 0},
	{"focus"      , takeFocus,	NO_REEXEC, 0},
	{"foldcc"     , foldcc,		NO_REEXEC, 4},
	{"foldj"      , foldj,		NO_REEXEC, 4},
	{"foldt"      , foldt,		NO_REEXEC, 4},
	{"format"     , format,		NO_REEXEC, 10},
#ifdef VNMRJ
	{"frameAction" , frameAction,	NO_REEXEC, 0},
	//{"framecmd"   ,frameCmd,	NO_REEXEC, 0},
#endif
	{"fp"         , fp,		NO_REEXEC, 0},
	{"fread"      , f_read,		NO_REEXEC, 0},
	{"fsave"      , fsave,		NO_REEXEC, 0},
	{"ft"         , ft,		NO_REEXEC, 4},
	{"ft1d"       , ft2d,		NO_REEXEC, 4},
	{"ft2d"       , ft2d,		NO_REEXEC, 4},
	{"full"       , full,		NO_REEXEC, 0},
	{"fullt"      , full,		NO_REEXEC, 10},
	{"graphoff"   , graphoff,	NO_REEXEC, 10},
	{"getdatadim" , getdatadim,	NO_REEXEC, 10},
	{"getDataStat", getDataStat,	NO_REEXEC, 10},
	{"getdfstat"  , getdfstat,	NO_REEXEC, 10},
	{"getdsstat"  , getdsstat,	NO_REEXEC, 10},
	{"getdgroup"  , gettype,	NO_REEXEC, 10},
        {"geterror"   , dispAcqError,	NO_REEXEC, 0},
      	{"getFidName" , getFidName,     NO_REEXEC, 10},
      	{"getFileOwner", getFileOwner,  NO_REEXEC, 0},
	{"getfile"    , getfile,	NO_REEXEC, 0},
        {"getfilestat", getfilestat,    NO_REEXEC, 0},
	{"getLastData", getLastData,	NO_REEXEC, 0},
	{"getMaxIndex", getMaxIndex,	NO_REEXEC, 0},
	{"readpars",    readpars,	NO_REEXEC, 0},
	{"readpar",     readpar,	NO_REEXEC, 0},
	{"readspectrum",readspectrum,	NO_REEXEC, 0},
	{"getstatus"  , getstatus,	NO_REEXEC, 0},
	{"getlimit"   , gettype,	NO_REEXEC, 10},
	{"getll"      , dll,		NO_REEXEC, 10},
	{"getmag"     , import_files,	NO_REEXEC, 0},
	{"getplottertype", getplottertype, NO_REEXEC, 0},
	{"getplotterpens", getplotterpens, NO_REEXEC, 0},
	{"getreg"     , region,		NO_REEXEC, 10},
	{"gettxt"     , rt,		NO_REEXEC, 0},
	{"gettype"    , gettype,	NO_REEXEC, 10},
	{"getvalue"   , getvalue,	NO_REEXEC, 10},
	{"getvalue4name", getvalue4name,NO_REEXEC, 10},
	{"getP11Dir", getP11Dir,NO_REEXEC, 10},
	{"canWrite", canWrite,NO_REEXEC, 10},
	{"p11datamirror", p11datamirror,NO_REEXEC, 10},
	{"gin"        , gin,		NO_REEXEC, 0},
	{"gmaplistfunc",gmaplistfunc,	NO_REEXEC, 0},
        {"gplan"      , gplan,          NO_REEXEC, 10},
#ifdef VNMRJ
        {"gplan_update", gplan_update,   REEXEC, 1},
#endif
	{"gradfit"    ,gradfit,		NO_REEXEC, 10},
	{"graphis"    , graph_is,	NO_REEXEC, 10},
	{"groupcopy"  , groupcopy,	NO_REEXEC, 0},
#ifdef VNMRJ
	{"gxyzfit"    , gxyzfit,	NO_REEXEC, 0},
	{"gxyz0fit"    , gxyzfit,	NO_REEXEC, 0},
	{"gxyzmap"    , gxyzmap,	NO_REEXEC, 0},
	{"gxyzmapo"    , gxyzmap,	NO_REEXEC, 0},
#endif
	{"gzfit"      , gzfit,		NO_REEXEC, 0},
	{"h2cal"      , h2cal,		NO_REEXEC, 0},
	{"halt"       , acqproc_msge,	NO_REEXEC, 0},
	{"halt2"      , acqproc_msge,	NO_REEXEC, 0},
	{"help"       , help,		NO_REEXEC, 0},
	{"hidecommand", mvCmd,		NO_REEXEC, 0},
	{"host"        ,shellcmds,		NO_REEXEC, 0},
	{"hpa"        , hpa,		NO_REEXEC, 0},
	{"hztomm"     , nmr_draw,	NO_REEXEC, 10},
	{"ilfid"      , ilfid,		NO_REEXEC, 10},
#ifdef VNMRJ
	{"imagefile"  , imagefile,	NO_REEXEC, 0},
#endif
	{"initrtvar"  , acqupdt,	NO_REEXEC, 0},
	{"initTclDg"  , initTclDg,	NO_REEXEC, 0},
	{"input"      , Rinput,		NO_REEXEC, 0},
	{"inset"      , inset,		NO_REEXEC, 0},
	{"integ"      , dli,		NO_REEXEC, 10},
	{"interact"   , interact,	NO_REEXEC, 0},
	{"iscmdopen"  , iscmdopen,	NO_REEXEC, 0},
	{"isCSIMode"  , isCSIMode,	NO_REEXEC, 0},
	{"isvnmrj"    , isvnmrj,	NO_REEXEC, 10},
      	{"isDataRecord", isDataRecord,  NO_REEXEC, 0},
      	{"isFDAdata"  , isFDAdata,      NO_REEXEC, 0},
      	{"isFDAdir"   , isFDAdir,       NO_REEXEC, 0},
        {"isFDAsystem", isFDAsystem,    NO_REEXEC, 0},
        {"isRecUpdated",isRecUpdated,   NO_REEXEC, 0},
	{"jexp"       , jexp,		NO_REEXEC, 0},
	{"jread"      , pipeRead,	NO_REEXEC, 0},
	{"downsizefid"       , downsizefid,		NO_REEXEC, 0},
	{"replacetraces"       , replacetraces,		NO_REEXEC, 0},
#ifdef VNMRJ
	{"jRegion"    , jRegion,	NO_REEXEC, 0},
	{"jRegion2"    , jRegion2,	NO_REEXEC, 0},
#endif
	{"iplot"      , iplot,		NO_REEXEC, 0},
	{"jplot"      , jplot,		NO_REEXEC, 0},
	{"large"      , large,		NO_REEXEC, 0},
#ifdef VNMRJ
	{"lccmd"      , vnmrjcmd,	NO_REEXEC, 0},
#endif
	{"left"       , full,		NO_REEXEC, 10},
	{"length"     , length,		NO_REEXEC, 10},
	{"lock"       , acq,		NO_REEXEC, 0},
	{"lf"         , shellcmds,	NO_REEXEC, 0},
	{"ln"         , ln,		NO_REEXEC, 10},
	{"ll2d"       , ll2d,		NO_REEXEC, 10},
#ifdef VNMRJ
	{"liMMap"       , liMMap,	NO_REEXEC, 10},
	{"llMMap"       , llMMap,	NO_REEXEC, 10},
	{"locki"      , locki,		REEXEC,    0},
	{"locklc"     , locklc,		NO_REEXEC, 0},
#endif
	{"lookup"     , lookup,		NO_REEXEC, 10},
	{"lp"         , lpcmd,		NO_REEXEC, 0},
	{"ls"         , shellcmds,	NO_REEXEC, 0},
	{"lsfid"      , leftshiftfid,	NO_REEXEC, 0},
	{"macrocat"   , macroCat,	NO_REEXEC, 0},
	{"macrocp"    , macroCp,	NO_REEXEC, 0},
	{"macrodir"   , macroDir,	NO_REEXEC, 0},
	{"macrold"    , macroLd,	NO_REEXEC, 10},
	{"macroLine"  , macroLine,	NO_REEXEC, 0},
	{"macrorm"    , macroRm,	NO_REEXEC, 0},
	{"macrosyscp" , macroCp,	NO_REEXEC, 0},
	{"macrosysdir", macroDir,	NO_REEXEC, 0},
	{"magicvars"  , magicvars,	NO_REEXEC, 0},
	{"magnetom"   , magnetom,	NO_REEXEC, 0},
	{"makefid"    , makefid,	NO_REEXEC, 0},
	{"makeslice"  , makeslice,	NO_REEXEC, 0},
	{"mark"       , mark,		NO_REEXEC, 0},
	{"md"         , mp,		NO_REEXEC, 0},
	{"menu"       , menu,		NO_REEXEC, 0},
	{"mf"         , mp,		NO_REEXEC, 0},
	{"mfblk"      , mfdata,		NO_REEXEC, 0},
	{"mfclose"    , mfdata,		NO_REEXEC, 0},
	{"mfdata"     , mfdata,		NO_REEXEC, 0},
	{"mfopen"     , mfdata,		NO_REEXEC, 0},
	{"mftrace"    , mfdata,		NO_REEXEC, 0},
	{"mkdir"      , shellcmds,	NO_REEXEC, 0},
	{"mousebuttons", getMouseButtons,NO_REEXEC, 0},
	{"move"       , nmr_draw,	NO_REEXEC, 6},
	{"mp"         , mp,		NO_REEXEC, 0},
	{"mstat"      , mstat,		NO_REEXEC, 0},
	{"mv"         , shellcmds,	NO_REEXEC, 0},
	{"mspec"      , mspec,	NO_REEXEC, 0},
#ifdef VNMRJ
	{"ndps"       , dps,	        NO_REEXEC, 0},
#endif
	{"newmenu"    , menu,		NO_REEXEC, 0},
	{"nextexp"    , nextexp,	NO_REEXEC, 0},
	{"nl"         , dll,		NO_REEXEC, 10},
	{"nl2"        , dll,		NO_REEXEC, 10},
	{"nli"        , dli,		NO_REEXEC, 10},
	{"nll"        , dll,		NO_REEXEC, 10},
	{"nm"         , ai,		NO_REEXEC, 10},
	{"noise"      , noise,		NO_REEXEC, 0},
	{"numreg"     , region,		NO_REEXEC, 0},
	{"off"        , Off,		NO_REEXEC, 0},
	{"on"         , Off,		NO_REEXEC, 0},
        {"onCancel"   , onCancel,       NO_REEXEC, 0},
        {"ovjCount"   , ovjCount,       NO_REEXEC, 0},
	{"p1"         , ernst,		NO_REEXEC, 0},
        {"p11_switchOpt"  , p11_switchOpt, NO_REEXEC, 0},
        {"p11_action" , p11_action,     NO_REEXEC, 0},
	{"pacosy"     , acosy,		NO_REEXEC, 0},
	{"padept"     , adept,		NO_REEXEC, 0},
	{"paramcopy"  , paramcopy,	NO_REEXEC, 0},
	{"plotpage"   , page,		NO_REEXEC, 0},
	{"plotxy"     , drawxy,		NO_REEXEC, 0},
	{"pap"        , dg,		NO_REEXEC, 0},
	{"pcmapapply" , pcmap,		NO_REEXEC, 0},
	{"pcmapclose" , pcmap,		NO_REEXEC, 0},
	{"pcmapgen"   , pcmap,		NO_REEXEC, 0},
	{"pcmapopen"  , pcmap,		NO_REEXEC, 0},
	{"pcon"       , dpcon,	        NO_REEXEC, 0},
	{"peak"       , dll,		NO_REEXEC, 10},
	{"maxpeak"    , dll,		NO_REEXEC, 10},
	{"peakmin"    , dll,		NO_REEXEC, 10},
	{"peak2d"     , peak2d,		NO_REEXEC, 10},
	{"pen"        , nmr_draw,	NO_REEXEC, 6},
	{"pexpl"      , expl_cmd,	NO_REEXEC, 0},
	{"phase_fid"  , phase_fid,      NO_REEXEC, 0},
	{"pfww"       , dfww,	        NO_REEXEC, 0},
	{"pa"         , ai,		NO_REEXEC, 10},
	{"pa1"        , ai,		NO_REEXEC, 10},
	{"ph"         , ai,		NO_REEXEC, 10},
	{"ph1"	     , ai,		NO_REEXEC, 10},
	{"ph2"	     , ai,		NO_REEXEC, 10},
	{"pir"        , dpir,	        NO_REEXEC, 0},
	{"pirN"        , dpir,	        NO_REEXEC, 0},
	{"pl"         , dsww,		NO_REEXEC, 0},
	{"pll2d"      , ll2d,		NO_REEXEC, 0},
	{"plww"       , dsww,		NO_REEXEC, 0},
	{"pl2d"       , ds2d,		NO_REEXEC, 0},
	{"plfid"      , dfww,	        NO_REEXEC, 0},
#ifdef VNMRJ
	{"plot1Dmap"    , disp3Dmap,	NO_REEXEC, 0},
	{"plot3Dmap"    , disp3Dmap,	NO_REEXEC, 0},
#endif
#ifdef VNMRJ
	{"pmlcmd"     , vnmrjcmd,	NO_REEXEC, 0},
#endif
	{"pow"        , ln,		NO_REEXEC, 10},
	{"powerfit"    , powerfit,	NO_REEXEC, 0},
	{"ppf"        , dpf,	        NO_REEXEC, 0},
	{"pps"        , dps,	        NO_REEXEC, 0},
	{"pread"      , pipeRead,	NO_REEXEC, 0},
	{"printon"    , print_on,	NO_REEXEC, 0},
	{"printoff"   , printoff,	NO_REEXEC, 0},
	{"profile_int", profile_int,	NO_REEXEC, 0},
	{"proj"       , proj,		NO_REEXEC, 0},
	{"prune"      , prune,		NO_REEXEC, 0},
	{"pscale"     , dscale,		NO_REEXEC, 0},
	{"psgupdateon", jacqupdtOn,	NO_REEXEC, 0},
	{"psgupdateoff", jacqupdtOff,	NO_REEXEC, 0},
	{"purge"      , purgeCache,	NO_REEXEC, 0},
	{"putf"       , export_files,	NO_REEXEC, 0},
	{"puti"       , export_files,	NO_REEXEC, 0},
	{"putp"       , export_files,	NO_REEXEC, 0},
	{"puttxt"     , rt,		NO_REEXEC, 0},
	{"pw"         , ernst,		NO_REEXEC, 0},
	{"pwd"        , shellcmds,	NO_REEXEC, 0},
	{"pwr"	      , ai,		NO_REEXEC, 10},
	{"pwr1"	      , ai,		NO_REEXEC, 10},
	{"pwr2"	      , ai,		NO_REEXEC, 10},
	{"quadtt"     , quadtt,		NO_REEXEC, 0},
	{"r"          , s_cmd,		NO_REEXEC, 0},
	{"ra"         , acq,		NO_REEXEC, 0},
	{"random"     , getRandom,	NO_REEXEC, 0},
	{"readf1"     , readf1,		NO_REEXEC, 0},
	{"readfile"   , readfile,	NO_REEXEC, 0},
	{"readhw"     , readhw,		NO_REEXEC, 0},
	{"readheader" , readheader,	NO_REEXEC, 0},
	{"readlk"     , readlk,		NO_REEXEC, 0},
	{"readparam"  , readparam,	NO_REEXEC, 0},
	{"readstr"    , readstr, 	NO_REEXEC, 0},
	{"real"       , real,		NO_REEXEC, 0},
	{"recon"      , recon,		NO_REEXEC, 0},
	{"recon_all"  , recon_all,     	NO_REEXEC, 0},
	{"recon3D"    , recon3D,        NO_REEXEC, 0},
	{"recon_mm"  , recon_mm,     	NO_REEXEC, 0},
	{"recordOff"  , recordOff,	NO_REEXEC, 0},
	{"releaseConsole", releaseConsole, NO_REEXEC, 0},
	{"rename"     , shellcmds,	NO_REEXEC, 0},
	{"region"     , region,		NO_REEXEC, 0},
	{"resume"     , resume,		NO_REEXEC, 0},
	{"rfblk"      , rfdata,		NO_REEXEC, 0},
	{"rfdata"     , rfdata,		NO_REEXEC, 0},
	{"rftrace"    , rfdata,		NO_REEXEC, 0},
	{"right"      , full,		NO_REEXEC, 10},
	{"rights"     , rights,		NO_REEXEC, 0},
	{"rjexp"      , jexp,		NO_REEXEC, 0},
	{"rm"         , shellcmds,	NO_REEXEC, 0},
	{"rmdir"      , shellcmds,	NO_REEXEC, 0},
	{"rmfile"     , shellcmds,	NO_REEXEC, 0},
	{"rotate"     , rotate,		NO_REEXEC, 4},
	{"rsapply"    , rsdata,		NO_REEXEC, 0},
	{"RT"         , rt,		NO_REEXEC, 0},
	{"RTP"        , rt,		NO_REEXEC, 0},
	{"rtphf"      , rtphf,		NO_REEXEC, 0},
	{"rts"        , rts,		NO_REEXEC, 0},
	{"rtv"        , rt,		NO_REEXEC, 0},
	{"rtx"        , rtx,		NO_REEXEC, 0},
	{"s"          , s_cmd,		NO_REEXEC, 0},
	{"sa"         , acqproc_msge,	NO_REEXEC, 0},
	{"savecolors" , savecolors,	NO_REEXEC, 0},
	{"savefdfspec" , savefdfspec,	NO_REEXEC, 0},
	{"scalefid"   , scalefid,	NO_REEXEC, 0},
	{"selbuf"     , selbuf,		NO_REEXEC, 0},
	{"select"     , selecT,		NO_REEXEC, 0},
#ifdef VNMRJ
	{"serverport",  serverport,	NO_REEXEC, 0},
#endif
	{"set_mag_login", set_mag_login, NO_REEXEC, 0},
	{"set_node_name", set_node_name, NO_REEXEC, 0},
	{"set_password", set_password,   NO_REEXEC, 0},
	{"set_remote_dir", set_remote_dir,  NO_REEXEC, 0},
	{"set_user_name", set_user_name, NO_REEXEC, 0},
	{"set3dproc"  ,	set3dproc,	NO_REEXEC, 0},
	{"setcolor"   , setcolor,	NO_REEXEC, 0},
	{"setdgroup"  , setdgroup,	NO_REEXEC, 0},
	{"setenumeral", setenumeral,	NO_REEXEC, 0},
	{"setfont"    , setfont,	NO_REEXEC, 0},
	{"setfrq"     , setfrq,		NO_REEXEC, 0},
	{"setgrid"    , setgrid,	NO_REEXEC, 0},
	{"setgroup"   , setgroup,	NO_REEXEC, 0},
	{"sethw"      , sethw,		NO_REEXEC, 0},
	{"sethwshim"  , sethwshim,	NO_REEXEC, 0},
	{"setlimit"   , setlimit,	NO_REEXEC, 0},
#ifdef VNMRJ
	{"setpage"    , setpage,	NO_REEXEC, 0},
#endif
	{"setplotdev" , setplotdev,	NO_REEXEC, 0},
	{"setprintdev", setprintdev,	NO_REEXEC, 0},
	{"setprotect" , setprotect,	NO_REEXEC, 0},
	{"setrtvar"   , acqupdt,	NO_REEXEC, 0},
	{"settype"    , settype,	NO_REEXEC, 0},
	{"setvalue"   , setvalue,	NO_REEXEC, 0},
	{"setvalue4name", setvalue4name,NO_REEXEC, 0},
	{"setwin"     , setwin,		NO_REEXEC, 0},
	{"setButtonState", setButtonState,NO_REEXEC, 0},
	{"shell"      , shellcmds,	NO_REEXEC, 0},
	{"shelli"     , shellcmds,	NO_REEXEC, 0},
	{"shim"       , acq,		NO_REEXEC, 0},
	{"shimamp"    , shimamp,	NO_REEXEC, 0},
	{"shimnames"  , shimnames,	NO_REEXEC, 0},
	{"show_local_files",  show_local_files,   NO_REEXEC, 0},
	{"show_remote_files", show_remote_files,  NO_REEXEC, 0},
	{"showbuf"    , showbuf,	NO_REEXEC, 0},
	{"sin"        , ln,	 	NO_REEXEC, 10},
	{"sleep"      , vnmrSleep,	NO_REEXEC, 0},
	{"small"      , small,		NO_REEXEC, 0},
	{"solvinfo"   , solvinfo,	NO_REEXEC, 0},
	{"sortCmd"    , sortCmd,	NO_REEXEC, 0},
	{"spa"        , spa,		NO_REEXEC, 0},
	{"spadd"      , addsub,		NO_REEXEC, 10},
	{"spin"       , acq,		NO_REEXEC, 0},
	{"spins"      , spins,		NO_REEXEC, 0},
        {"splmodprepare", splmodprepare,NO_REEXEC, 0},
        {"splmodread" , splmodread,     NO_REEXEC, 0},
	{"spmin"      , addsub,		NO_REEXEC, 10},
	{"spmax"      , addsub,		NO_REEXEC, 10},
	{"spsub"      , addsub,		NO_REEXEC, 10},
	{"sread"      , sread,		NO_REEXEC, 0},
	{"string"     , string,		NO_REEXEC, 10},
	{"strstr"     , Strstr,		NO_REEXEC, 10},
	{"sub"        , addsub,		NO_REEXEC, 10},
	{"svd"        , raster_dump,	NO_REEXEC, 0},
	{"SVF"        , svf,		NO_REEXEC, 0},
	{"Svfname"    , autoname,	NO_REEXEC, 0},
        {"svr"        , svr_FDA,        NO_REEXEC, 0},
        {"svr_as"     , svr_FDA,        NO_REEXEC, 0},
        {"svr_r"      , svr_FDA,        NO_REEXEC, 0},
        {"svr_p"      , svr_FDA,        NO_REEXEC, 0},
	{"svs"        , svs,		NO_REEXEC, 0},
	{"Svsname"    , autoname,	NO_REEXEC, 0},
	{"SVP"        , svf,		NO_REEXEC, 0},
	{"svdat"      , output_sdf,	NO_REEXEC, 0},
	{"svsdfd"     , output_sdf,	NO_REEXEC, 0},
	{"svphf"      , svphf,		NO_REEXEC, 0},
	{"swrite"     , swrite,		NO_REEXEC, 0},
	{"systemtime" , unixtime,	NO_REEXEC, 0},
	{"tan"        , ln,	 	NO_REEXEC, 10},
        {"tbox"       , tnmr_draw,	NO_REEXEC, 10},
	{"tcapply"    , tcdata,	 	NO_REEXEC, 0},
	{"tcclose"    , tcdata,	 	NO_REEXEC, 0},
	{"tcopen"     , tcdata,	 	NO_REEXEC, 0},
	{"tcl"	     , tclCmd,		NO_REEXEC, 0},
	{"tempcal"    , tempcal,	NO_REEXEC, 0},
	{"testPart11" , testPart11,	NO_REEXEC, 0},
	{"teststr"    , teststr,   	NO_REEXEC, 0},
	{"text"       , text,		NO_REEXEC, 0},
	{"textis"     , text_is,	NO_REEXEC, 0},
	{"touch"      , shellcmds,	NO_REEXEC, 0},
	{"tune"       , tune,	        NO_REEXEC, 0},
#ifdef VNMRJ
	{"trackCursor", trackCursor,    NO_REEXEC, 0},
#endif
	{"uname"      , shellcmds,	NO_REEXEC, 0},
	{"unit"       , unit,		NO_REEXEC, 0},
	{"unixtime"   , unixtime,	NO_REEXEC, 0},
	{"updtparam"  , jacqupdt,	NO_REEXEC, 0},
	{"vnmr_unlock", vnmr_unlock,	NO_REEXEC, 0},
	{"vnmrinfo"   , vnmrInfo,	NO_REEXEC, 0},
#ifdef VNMRJ
	{"vnmrj_close", vnmrj_close,	NO_REEXEC, 0},
#endif
	{"vnmrjcmd"   , vnmrjcmd,	NO_REEXEC, 0},
	{"w"          , shellcmds,	NO_REEXEC, 0},
	{"wbs"        , werr,		NO_REEXEC, 0},
	{"wdone"      , werr,		NO_REEXEC, 0},
	{"werr"       , werr,		NO_REEXEC, 0},
	{"wexp"       , werr,		NO_REEXEC, 0},
	{"wft"        , ft,		NO_REEXEC, 4},
	{"wft1d"      , ft2d,		NO_REEXEC, 4},
	{"wft2d"      , ft2d,		NO_REEXEC, 4},
	{"wnt"        , werr,		NO_REEXEC, 0},
#ifdef VNMRJ
	{"repaint"    , window_repaint,	NO_REEXEC, 0},
	{"refresh"    , window_refresh,	NO_REEXEC, 0},
	{"worst1"     , worst1,		NO_REEXEC, 0},
#endif
	{"write"      , nmr_write,	NO_REEXEC, 5},
	{"writef1"    , writef1,	NO_REEXEC, 0},
	{"writefile"  , writefile,	NO_REEXEC, 0},
	{"writefid"   , writefid,	NO_REEXEC, 0},
	{"writeparam" , writeparam,	NO_REEXEC, 0},
	{"writespectrum", writespectrum, NO_REEXEC, 0},
#ifdef VNMRJ
	{"writeToVnmrJcmd", writeToVnmrJcmd, NO_REEXEC, 0},
#endif
	{"wrspec"     , wrspec,		NO_REEXEC, 0},
	{"wsram"      , wsram,		NO_REEXEC, 0},
#ifdef VNMRJ
	{"wti"        , wti,		REEXEC, 1},
	{"xoron"      , xoron,		NO_REEXEC, 0},
	{"xoroff"     , xoroff,		NO_REEXEC, 0},
	{"pushtime"   , pushtime,       NO_REEXEC, 0},
	{"deltatime"  , deltatime,      NO_REEXEC, 0},
	{"poptime"    , poptime,        NO_REEXEC, 0},
	{"clrshellcnt", clrshellcnt,    NO_REEXEC, 0},
	{"showshellcnt",showshellcnt,   NO_REEXEC, 0},
#else
	{"wti"        , wti,		NO_REEXEC, 1},
#endif
	{"z"          , dli,		NO_REEXEC, 0},
	{"zerofillfid", zerofillfid,	NO_REEXEC, 0},
	{"zeroneg"    , zeroneg,	NO_REEXEC, 0},
	{"listvnmrcmd"    , listvnmrcmd,	NO_REEXEC, 0},
	{NULL         , NULL,           0, 0 }	
	      };


#define  TABLES  21
static cmd *table = NULL;
static int cmdIndex[TABLES+1];

static void
disp_cmd_index(char *name)
{
    char d;
    int  n1, n2, c;
    cmd  *p;

    d = name[0];
    if (d <= 'a') {
        n1 = 0;
        n2 = 1;
    }
    else {
        n1 = d - 'a';
        if (n1 >= TABLES)
            n1 = TABLES - 1;
        n2 = n1 + 1;
    }
    n1 = cmdIndex[n1];
    n2 = cmdIndex[n2];
    p = table + n1;
    c = 1;
    while (n1 < n2) {
	if (strcmp(p->searchName,name) == 0) {
            fprintf(stderr, "  search for command '%s'", name);
            fprintf(stderr, " took  %d steps. \n", c);
            return;
        }
	p += 1;
        n1++;
        c++;
    }
    fprintf(stderr, "  command '%s' does not exist.", name);
}

int listvnmrcmd(int argc, char *argv[], int retc, char *retv[])
{
    int  k, a, b, n, doList;
    cmd  *p;
    char *sname;

    (void) retc;
    (void) retv;
    if (vIndex <= 0)
        return(0);
    doList = 0;
    sname = NULL;
    for (k = 1; k < argc; k++) {
        if (strcmp(argv[k], "-list") == 0)
            doList = 1;
        else
            sname = argv[k];
    }
    if (sname != NULL) {
        disp_cmd_index(sname);
        if (doList == 0)
            return (0);
    }
    
    fprintf(stderr, "vnmr cmds %d  aips %d  total %d \n", vcount, aipcount, numCmds);
    for (k = 0; k < TABLES; k++) {
        fprintf(stderr, " %2d. %c", k, 'a' + k);
        if (k == TABLES - 1)
           fprintf(stderr, "...z");
        fprintf(stderr, " from %d to %d => %d\n", cmdIndex[k], cmdIndex[k+1]-1, cmdIndex[k+1] - cmdIndex[k]);
        if (doList) {
            a = cmdIndex[k];
            b = cmdIndex[k+1];
            n = 0;
            p = table + a;
            fprintf(stderr, "    ");
            while (a < b) {
               fprintf(stderr, "%s  ", p->searchName);
               n++;
               if (n > 3) {
                    fprintf(stderr, "\n    ");
                    n = 0;
               }
               a++;
               p++;
            } 
            if (n != 0)
               fprintf(stderr, "\n");
            fprintf(stderr, "\n");
        }
    }
    return(0);
}

#ifdef XXX
/*
 * Merge the standard command table with the aip command table .
 */
int
new_merge_command_table(cmd_t *aipCmd)
{
    cmd *cmds;
    cmd_t *cmdPtr;
    int   n, k, loop;
    char  ch0, ch1, ch2;

    vcount = 0;
    aipcount = 0;
    cmdPtr = vnmr_table;
    while (cmdPtr->useName != NULL) {
        vcount++;
        cmdPtr++;
    }
    if (aipCmd != NULL) {
        cmdPtr = aipCmd;
        while (cmdPtr->useName != NULL) {
            aipcount++;
            cmdPtr++;
        }
    }
    numCmds = vcount + aipcount;
    table = cmds = (cmd *)malloc(sizeof(cmd) * (numCmds + 2));
    if (cmds == NULL)
	return 0;
   
    cmdIndex[0] = 0;
    ch1 = 'A' - 1;
    ch2 = 'a';
    n = 0;
    for (k = 1; k < TABLES; k++) {
        cmdPtr = vnmr_table;
        loop = 1;
        while (loop < 3) {
           while (cmdPtr->useName != NULL) {
              ch0 = cmdPtr->useName[0];
              if (ch0 > ch1  && ch0 <= ch2) {
                  cmds->useName = cmdPtr->useName;
                  cmds->func = cmdPtr->func;
                  cmds->graphcmd = cmdPtr->graphcmd;
                  cmds->redoType = cmdPtr->redoType;
                  strcpy(cmds->searchName, cmds->useName);
                  cmds++;
                  n++;
              }
              cmdPtr++;
           }
           loop++;
           if (aipCmd != NULL)
              cmdPtr = aipCmd;
           else
              loop++;
        }
        cmdIndex[k] = n; 
        ch1 = ch2;
        ch2 = ch1 + 1;
    }
    cmdPtr = vnmr_table;
    while (cmdPtr->useName != NULL) {
        ch0 = cmdPtr->useName[0];
        if (ch0 > ch1) {
            cmds->useName = cmdPtr->useName;
            cmds->func = cmdPtr->func;
            cmds->graphcmd = cmdPtr->graphcmd;
            cmds->redoType = cmdPtr->redoType;
            strcpy(cmds->searchName, cmds->useName);
            cmds++;
            n++;
        }
        cmdPtr++;
    }
    if (aipCmd != NULL)
    {
        cmdPtr = aipCmd;
        while (cmdPtr->useName != NULL) {
            ch0 = cmdPtr->useName[0];
            if (ch0 > ch1) {
                cmds->useName = cmdPtr->useName;
                cmds->func = cmdPtr->func;
                cmds->graphcmd = cmdPtr->graphcmd;
                cmds->redoType = cmdPtr->redoType;
                strcpy(cmds->searchName, cmds->useName);
                cmds++;
                n++;
            }
            cmdPtr++;
        }
    }
    vIndex = n;
    cmdIndex[TABLES] = n; 
    for (k = n; k <= numCmds+1; k++) { 
        cmds->useName = NULL;
        cmds->searchName[0] = '\0';
        cmds++;
    }
    return(1);
}
#endif


int
merge_command_table(void *aipCmd)
{
    cmd *cmds;
    cmd_t *cmdPtr;
    int ncmds;

    for (cmdPtr=vnmr_table, ncmds=0; cmdPtr->useName; ncmds++, cmdPtr++);
    if (aipCmd != NULL)
       for (cmdPtr=(cmd_t *)aipCmd; cmdPtr->useName; ncmds++, cmdPtr++);
    table = cmds = (cmd *)malloc(sizeof(cmd) * (ncmds+1));
    if (cmds == NULL) {
	return 0;
    }
    cmdPtr = vnmr_table;
    while (cmdPtr->useName)
    {
       cmds->useName = cmdPtr->useName;
       cmds->func = cmdPtr->func;
       cmds->graphcmd = cmdPtr->graphcmd;
       cmds->redoType = cmdPtr->redoType;
       strcpy(cmds->searchName, cmds->useName);
       cmdPtr++;
       cmds++;
    }
    if (aipCmd != NULL)
    {
       cmdPtr = (cmd_t *)aipCmd;
       while (cmdPtr->useName)
       {
          cmds->useName = cmdPtr->useName;
          cmds->func = cmdPtr->func;
          cmds->graphcmd = cmdPtr->graphcmd;
          cmds->redoType = cmdPtr->redoType;
          strcpy(cmds->searchName, cmds->useName);
          cmdPtr++;
          cmds++;
       }
    }
    cmds->useName = NULL;
    cmds->func = NULL;
    cmds->graphcmd = 0;
    cmds->searchName[0] = '\0';
    return 1;
}
    
#ifdef XXX
static int
get_cmd_index(char *name)
{
    char d;
    int  n1, n2;
    cmd  *p;

    d = name[0];
    if (d <= 'a') {
        n1 = 0;
        n2 = 1;
    }
    else {
        n1 = d - 'a';
        if (n1 >= TABLES)
            n1 = TABLES - 1;
        n2 = n1 + 1;
    }
    n1 = cmdIndex[n1];
    n2 = cmdIndex[n2];
    p = table + n1;
    while (n1 < n2) {
	if (strcmp(p->searchName,name) == 0)
	    return(n1);
	p += 1;
        n1++;
    }
    return(-1);
}
#endif

/*------------------------------------------------------------------------------
|
|	builtin/1
|
|	This function returns a pointer to a function returning int (isn't
|	C declaration syntax strange) given a name.
|
+-----------------------------------------------------------------------------*/

int (*builtin(char *sName, char **uName, int *gcmd, int *gtype))()
{   cmd *p;
/*
    int  n;

    if (vIndex > 0) {
        n = get_cmd_index(sName);
        if (n >= 0) {
            p = table + n;
            *uName = p->useName;
            *gcmd = p->graphcmd;
            *gtype = p->redoType;
	    return(p->func);
        }
        return(NULL);
    }
*/
    p = table;
    while (p->searchName[0])
    {
	if (strcmp(p->searchName,sName) == 0)
        {
            *uName = p->useName;
            *gcmd = p->graphcmd;
            *gtype = p->redoType;
	    return(p->func);
        }
	else
	    p += 1;
    }
    return(NULL);
}

/*------------------------------------------------------------------------------
|
|	isReexecCmd/1
|
|	This function returns 1 if command should be re-executed, 0 if it
|	should not.
|
+-----------------------------------------------------------------------------*/

int isReexecCmd(char *sName)
{   cmd *p;
/*
    int  n;

    if (vIndex > 0) {
        n = get_cmd_index(sName);
        if (n >= 0) {
            p = table + n;
	    return(p->graphcmd);
        }
        return(0);
    }
*/

    p = table;
    while (p->searchName[0])
	if (strcmp(p->searchName,sName) == 0)
	    return(p->graphcmd);
	else
	    p += 1;
    return(0);
}

int getCmdRedoType(char *sName)
{
    cmd *p;

    p = table;
    while (p->searchName[0])
	if (strcmp(p->searchName,sName) == 0)
	    return(p->redoType);
	else
	    p += 1;
    return(0);
}


/*------------------------------------------------------------------------------
|
|	mvCmd/1
|
|	This function changes the first chareracter of a command between
|       upper and lower case.  Default is to set it to upper case.
|
+-----------------------------------------------------------------------------*/

int mvCmd(int argc, char *argv[], int retc, char *retv[])
{   cmd *p;

    if (argc != 2)
    {
       Werrprintf("Usage: %s requires command name or '?'", argv[0]);
       return(1);
    }
    p = table;
    if (strcmp(argv[1],"?") == 0)
    {
       int first = 1;

       while (p->searchName[0])
       {
          if (p->searchName[0] != p->useName[0])
          {
             if (first)
             {
                Wscrprintf("Renamed Command\t\tOriginal Name\n");
                first = 0;
             }
             Wscrprintf("%s\t\t\t%s\n",p->searchName,p->useName);
          }
	  p += 1;
       }
       if (first)
          Wscrprintf("No commands have been renamed\n");
       return(0);
    }
    while (p->searchName[0])
    {
	if (strcmp(p->searchName,argv[1]) == 0)
        {
            if (p->searchName[0] == p->useName[0])
            {
               if ( (p->searchName[0] >= 'A') && (p->searchName[0] <= 'Z') )
                  p->searchName[0] += 'a' - 'A';
               else
                  p->searchName[0] += 'A' - 'a';
               if (retc)
                  retv[ 0 ] = newString( p->searchName );
               else
                  Wscrprintf("'%s' is now hidden as '%s'\n",argv[1], p->searchName);
            }
            else
            {
               p->searchName[0] = p->useName[0];
               if (retc)
                  retv[ 0 ] = newString( p->searchName );
               else
                  Wscrprintf("'%s' is now returned as '%s'\n",argv[1], p->searchName);
            }
	    return(0);
        }
	else
	    p += 1;
    }
    return(0);
}

/*------------------------------------------------------------------------------
|
|	isACmd/1
|
|	This function returns 1 if if was given the name of a command
|	in the table, 0 otherwise
|
+-----------------------------------------------------------------------------*/

int isACmd(char *sName)
{   cmd *p;
/*
    int  n;

    if (vIndex > 0) {
        n = get_cmd_index(sName);
        if (n >= 0)
            return(1);
        return(0);
    }
*/
    p = table;
    while (p->searchName[0])
	if (strcmp(p->searchName,sName) == 0)
	    return(1);
	else
	    p += 1;
    return(0);
}

/*------------------------------------------------------------------------------
|
|	isInteractive/1
|
|	This function returns 1 if if was given the name of a command
|	in the table that is marked as reexecutable, 0 otherwise
|
+-----------------------------------------------------------------------------*/

int isInteractive(char *sName)
{   cmd *p;
/*
    int  n;

    if (vIndex > 0) {
        n = get_cmd_index(sName);
        if (n >= 0) {
            p = table + n;
	    if (p->graphcmd == REEXEC)
	       return(1);
        }
        return(0);
    }
*/

    p = table;
    while (p->searchName[0])
	if (strcmp(p->searchName,sName) == 0) {
	    if (p->graphcmd == REEXEC)
	      return(1);
	    else
	      return(0);
        }
	else
	  p += 1;
    return(0);
}

static int isvnmrj(int argc, char *argv[], int retc, char *retv[])
{
  if (!Bnmr)
  {
    if ((argc == 2) && !strcmp(argv[1],"linux") )
    {
      if (retc > 0)
#ifdef LINUX
        retv[0] = realString( 1.0 );
#else
        retv[0] = realString( 0.0 );
#endif
      else
#ifdef LINUX
        Winfoprintf("interface is Linux VNMRJ\n");
#else
        Winfoprintf("interface is not Linux VNMRJ\n");
#endif
      return(0);
    }
#ifdef VNMRJ
    if (retc > 0)
      retv[0] = realString( 1.0 );
    else
      Winfoprintf("interface is VNMRJ\n");
#else
    if (retc > 0)
      retv[0] = realString( 0.0 );
    else
      Winfoprintf("interface is VNMR\n");
#endif
  }
  else /* #ifdef VNMRJ doesn't work from background mode */
  {
#ifdef LINUX
    if ((argc == 2) && !strcmp(argv[1],"linux") )
    {
      if (retc > 0)
        retv[0] = realString( 1.0 );
      else
        Wscrprintf("interface is Linux VNMRJ\n");
    }
    else if (retc > 0)
      retv[0] = realString( 1.0 );
    else
      Wscrprintf("interface is VNMRJ\n");
#else
    FILE *revfile;
    char fstr[MAXPATH];
    int ret = 0;

    strcpy(fstr,systemdir);
    strcat(fstr,"/vnmrrev");
    if (access(fstr, F_OK) == 0) /* check R_OK ? */
    {
      revfile = fopen(fstr, "r");
      if ((fscanf(revfile,"%s",fstr)) != EOF)
        if ((fscanf(revfile,"%s",fstr)) != EOF)
	  if ((fstr[0]=='V') && (fstr[1]=='J'))
	    ret = 1;
      fclose(revfile);
    }
    if (ret == 1)
      if (retc > 0)
        retv[0] = realString( 1.0 );
      else
        Wscrprintf("interface is VNMRJ\n");
    else
      if (retc > 0)
        retv[0] = realString( 0.0 );
      else
        Wscrprintf("interface is VNMR\n");
#endif
  }
  return(0);
}

static int iscmdopen(int argc, char *argv[], int retc, char *retv[])
{
  (void) argc;
  (void) argv;
  if (!Bnmr)
  {
#ifdef VNMRJ
    if (retc > 0)
      retv[0] = intString( jShowInput );
    else
    {
      if (jShowInput != 1)
        Winfoprintf("command line is closed\n");
      else
        Winfoprintf("command line is open\n");
    }
#else
    if (retc > 0)
      retv[0] = intString( 1 );
    else
      Winfoprintf("command line is open\n");
#endif
  }
  else /* command line is closed in background mode */
  {
    if (retc > 0)
      retv[0] = intString( 0 );
    else
      Winfoprintf("command line is closed\n");
  }
  return(0);
}

#ifndef VNMRJ
int gplan(int argc, char *argv[], int retc, char *retv[])
{
  return(0);
}
#endif

#if defined(__INTERIX)
void sync()
{}
void initExpQs()
{}
void expQaddToTail()
{}
void expQaddToHead()
{}
void expQdelete()
{}
#endif
#ifdef __INTERIX
void AP_Device()
{}
#endif
