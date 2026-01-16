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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include "dpsdef.h"

#ifndef EXIT_SUCCESS
#define	EXIT_SUCCESS    0
#endif

#define   paramLen	256
#define   MAXPATH	256
#define   CMDLEN	256
#define   INTGARRAY	101
#define   FLOATARRAY	100
#define   INTG		99
#define   FLOAT		98
#define   STRG		97
#define   CHAR		96
#define   SHORT		95
#define   NONE		94

/**
#define   TODEV		1
#define   DODEV		2
#define   DO2DEV	3
#define   DO3DEV	4
#define   DO4DEV	5
#define   DO5DEV	6
**/


char   *cmd_3[] = {"add", "dbl", "hlv", "sub", "sli" };
static int  cmd3Num = 5;

char   *cmd_4[] = {"decr", "divn", "incr", "mod2", "mod4", "modn", 
		   "mult", "loop", "pwrf", "pwrm", "rfon", "CEnd" };
static int  cmd4Num = 12;

char   *cmd_5[] = {"ttadd", "ttdiv", "ttsub", "tsadd", "tsdiv", "tssub",
		   "abort", "delay", "pulse", "xgate", "endif", "power",
   		   "decon", "ipwrf", "ipwrm", "sp1on", "sp2on", "rfoff",
		   "vscan", "vfreq", "sp3on", "sp4on", "sp5on", "sp_on",
		   "CMark", "recon", "S_sli" };
static int  cmd5Num = 26;

char   *cmd_6[] = {"assign", "idelay", "ipulse", "ifzero", "elsenz",
		   "decpwr", "status", "xmtron", "decoff", "dec2on",
		   "dec3on", "vdelay", "offset", "rlpwrf", "rlpwrm",
		   "rcvron", "sp1off", "sp2off", "var1on", "var2on",
		   "msloop", "peloop", "S_vsli", "HSgate", "ttmult",
		   "tsmult", "sp3off", "sp4off", "sp5off", "sp_off",
		   "CDelay", "CPulse", "dec4on", "dec5on", "recoff",
		   "rotate", "ifrtGT", "ifrtGE", "ifrtLT", "ifrtLE",
                   "ifrtEQ", "sample", "endacq", "rlloop", "kzloop",
                   "nwloop", "printf" }; 
static int  cmd6Num = 46;

char   *cmd_7[] = {"hsdelay", "rgpulse", "endloop", "txphase", "dcphase",
   		   "acquire", "rlpower", "xmtroff", "dec2off", "dec3off",
		   "initval", "blankon", "ioffset", "rcvroff", "lk_hold",
		   "getelem", "obspwrf", "decpwrf", "G_Delay", "G_Pulse",
		   "G_Power", "voffset", "poffset", "G_Sweep", "var1off",
		   "var2off", "dec4off", "dec5off", "putcode", "ifrtNEQ",
                   "peloop2", "fprintf" };
static int  cmd7Num = 31;

char   *cmd_8[] = {"obspulse", "decpulse", "irgpulse", "simpulse",
		   "decphase", "declvlon", "stepsize", "spinlock",
		   "obsprgon", "decprgon", "incdelay", "gradient",
		   "blankoff", "genpulse", "obspower", "decpower",
		   "dec2pwrf", "dec3pwrf", "decblank", "obsblank",
		   "G_Offset", "dps_show", "stepfreq", "dec4pwrf",
		   "dec5pwrf", "lcsample", "set4Tune", "startacq",
                   "vobspwrf", "vdecpwrf" };
static int  cmd8Num = 30;

char   *cmd_9[] = {"iobspulse", "idecpulse", "dec2pulse", "dec3pulse",
		   "sim3pulse", "sim4pulse", "decoffset", "obsoffset",
		   "dec2prgon", "dec3prgon", "xmtrphase", "dec2phase",
		   "dec3phase", "declvloff", "obsprgoff", "decprgoff",
		   "apovrride", "rotorsync", "setstatus", "initdelay",
		   "lk_sample", "dec2power", "dec3power", "dec2blank",
		   "dec3blank", "G_RTDelay", "endpeloop", "endmsloop",
		   "vgradient", "rgradient", "Sgradient", "pgradient",
		   "Wgradient", "lgradient", "loadtable", "CSet2Mark",
		   "CGradient", "dec4pulse", "dec4prgon", "dec4phase",
		   "dec4power", "dec4blank", "dec5pulse", "dec5prgon",
		   "dec5phase", "dec5power", "dec5blank", "shapelist",
                   "vdec2pwrf", "vdec3pwrf", "F_initval", "rlendloop",
                   "kzendloop", "endnwloop"  };
static int  cmd9Num = 54;

char   *cmd_10[]= {"decrgpulse","idec2pulse","genrgpulse","gensaphase",
		   "G_Simpulse","zgradpulse","phaseshift","dcplrphase",
		   "prg_dec_on","dec2prgoff","dec3prgoff","decblankon",
		   "obsunblank","decunblank","blankingon","dec2offset",
		   "dec3offset","init_vscan","Svgradient","Wvgradient",
		   "Pvgradient","magradient","vagradient","S_getarray",
		   "obs_pw_ovr","CGradPulse","dec4prgoff","dec4offset",
		   "dec5prgoff","dec5offset","dec_pw_ovr","obs_pw_ovr",
		   "ClearTable","pbox_pulse","pbox_decon","offsetlist",
                   "endpeloop2","GroupPulse" };
static int  cmd10Num = 38;

char   *cmd_11[] = {"shapedpulse", "idecrgpulse", "dec2rgpulse", "dec3rgpulse",
		    "dec2unblank", "dec3unblank", "blankingoff", "decblankoff",
		    "dcplr2phase", "dcplr3phase", "obsstepsize", "decstepsize", 
		    "decspinlock", "genspinlock", "endhardloop", "rotorperiod",
		    "setreceiver", "prg_dec_off", "S_vgradient", "incgradient",
		    "pe_gradient", "magradpulse", "simgradient", "vagradpulse",
		    "vdelay_list", "gensimpulse", "statusdelay", "dec4rgpulse",
		    "dec4unblank", "dec5rgpulse", "dec5unblank", "hdwshiminit",
		    "pbox_xmtron", "pbox_decoff", "pbox_dec2on", "pbox_dec3on",
		    "XmtNAcquire", "offsetglist", "TGroupPulse", "parallelend",
                    "gen_poffset", "nowait_loop" };
static int  cmd11Num = 42;

char   *cmd_12[] = {"shaped_pulse", "idec2rgpulse", "idec3rgpulse",
		    "dec2spinlock", "dec3spinlock", "gen2spinlock",
		    "dec2stepsize", "dec3stepsize", "gensim2pulse",
		    "gensim3pulse", "S_sethkdelay", "obl_gradient",
		    "Sincgradient", "Wincgradient", "pe2_gradient",
		    "pe3_gradient", "poffset_list", "simgradpulse",
		    "observepower", "CShapedPulse", "init_wg_list",
		    "idec4rgpulse", "dec4spinlock", "dec4stepsize",
		    "idec5rgpulse", "dec5spinlock", "dec5stepsize",
		    "shapedvpulse", "rotate_angle", "pbox_xmtroff",
		    "pbox_dec2off", "pbox_dec3off", "grad_advance",
                    "parallelsync", "decshapelist" };
static int  cmd12Num = 35;

char   *cmd_13[] = {"starthardloop", "set_observech", "sync_on_event",
		    "S_shapedpulse", "S_incgradient", "decouplepower",
		    "pbox_decpulse", "pbox_spinlock", "pbox_simpulse",
		    "SweepNAcquire", "triggerSelect", "gen_shapelist",
                    "parallelstart", "dec2shapelist", "dec3shapelist",
                    "S_setloopsize" };
static int  cmd13Num = 16;

char   *cmd_14[] = {"shapedgradient", "decshapedpulse", "simshapedpulse",
		    "apshaped_pulse", "CGradientPulse", "pbox_dec2pulse",
		    "pbox_dec3pulse", "pbox_sim3pulse", "gen_offsetlist",
                    "rot_angle_list", "obsprgonOffset", "decprgonOffset",
                    "set_angle_list", "setactivercvrs", "nowait_endloop",
                     };
                    // "GroupXmtrPhase", "getorientation", "GroupPhaseStep" };
                    // "decshapelistpw"
static int  cmd14Num = 15;

char   *cmd_15[] =
         { "decshaped_pulse", "simshaped_pulse", "genshaped_pulse",
	   "shapedvgradient", "position_offset", "readMRIUserByte",
           "setMRIUserGates", "shapedpulselist", "gen_offsetglist",
           "dec2prgonOffset", "dec3prgonOffset" };
          // "gen_shapelistpw", "dec2shapelistpw", "dec3shapelistpw"
static int  cmd15Num = 11;

char   *cmd_16[] = {"dec2shaped_pulse","dec3shaped_pulse","sim3shaped_pulse",
		    "sim4shaped_pulse","S_decshapedpulse","S_simshapedpulse",
		    "S_shapedgradient","oblique_gradient","shaped2Dgradient",
		    "mashapedgradient","vashapedgradient","create_freq_list",
		    "dec4shaped_pulse","dec5shaped_pulse","pbox_decspinlock",
		    "writeMRIUserByte","initRFGroupPulse","init_global_list" };
static int  cmd16Num = 17;

char   *cmd_17[] = {"apshaped_decpulse","shapedincgradient","pe_shapedgradient",
		    "shaped_V_gradient","S_position_offset","oblique_gradpulse",
		    "mashapedgradpulse","simshapedgradient","vashapedgradpulse",
		    "create_delay_list","pbox_dec2spinlock","pbox_dec3spinlock",
		    "close_global_list","ShapedXmtNAcquire","shapedpulseoffset",
                    "modifyRFGroupPwrf","modifyRFGroupPwrC","modifyRFGroupName",
                    "create_angle_list","exe_grad_rotation" };
static int  cmd17Num = 20;

char   *cmd_18[]=
    { "shaped_2D_gradient","apshaped_dec2pulse","gen_apshaped_pulse",
      "S_oblique_gradient","obl_shapedgradient","pe2_shapedgradient",
      "pe3_shapedgradient","zero_all_gradients","simshapedgradpulse",
      "create_offset_list","pe_shaped3gradient","modifyRFGroupOnOff",
      "var_shapedgradient","gen_shapelist_init","decshapedpulselist",
      "get_freqlist_table", "MagnusSelectFPower" };
static int  cmd18Num = 17;

char   *cmd_19[] = {"shaped_2D_Vgradient","gensim2shaped_pulse",
                    "gensim3shaped_pulse","shaped_INC_gradient",
                    "obl_shaped3gradient","pe3_shaped3gradient",
                    "pe2_shaped3gradient","gen_shapedpulselist",
                    "modifyRFGroupSPhase","csi3_shapedgradient",
                    "SweepNOffsetAcquire","var_shaped3gradient",
                    "var2_shapedgradient","var3_shapedgradient",
                    "dec2shapedpulselist","dec3shapedpulselist" };
static int  cmd19Num = 16;

char   *cmd_20[] = {"position_offset_list","pe_oblshapedgradient",
                    "create_rotation_list","csi3_shaped3gradient",
                    "var2_shaped3gradient","var3_shaped3gradient",
                    "decshapedpulseoffset" };
static int  cmd20Num = 7;

char   *cmd_21[] = {"phase_encode_gradient","gen_shapedpulseoffset",
                    "create_rot_angle_list","modifyRFGroupFreqList",
                    "dec2shapedpulseoffset","dec3shapedpulseoffset" };
static int  cmd21Num = 6;

char   *cmd_22[] = {"oblique_shapedgradient","phase_encode3_gradient",
		    "S_position_offset_list" };
static int  cmd22Num = 3;

char   *cmd_23[] = {"S_phase_encode_gradient","pe_shaped3gradient_dual"};
static int  cmd23Num = 2;

char   *cmd_24[] = {"S_oblique_shapedgradient","S_phase_encode3_gradient",
                    "pe3_shaped3gradient_dual" };
static int  cmd24Num = 3;

char   *cmd_25[] = {"pe_oblique_shapedgradient"};
static int  cmd25Num = 1;

char   *cmd_26[] = {"genRFShapedPulseWithOffset", "pe_oblique_shaped3gradient",
                    "obl_imaging_shapedgradient" };
static int  cmd26Num = 3;

char   *cmd_27[] = {"phase_encode_shapedgradient", "pe3_oblique_shaped3gradient" };
static int  cmd27Num = 2;

char   *cmd_28[] = {"phase_encode3_shapedgradient","S_pe_oblique_shaped3gradient" };
static int  cmd28Num = 2;

char   *cmd_29[] = {"S_phase_encode_shapedgradient"};
static int  cmd29Num = 1;

char   *cmd_30[] = {"S_phase_encode3_shapedgradient"};
static int  cmd30Num = 1;

char   *cmd_31[] = {"phase_encode3_oblshapedgradient"};
static int  cmd31Num = 1;

char   op_ch[14] = {'+', '-', '*', '/', '=', '|', '&', '!', '%', '>', '<',
                  '^', '?', ':'};

char   *reserve_3[] = {"int", "for" };
char   *reserve_4[] = {"char", "long", "void", "auto", "goto", "else", "case"};
char   *reserve_5[] = {"float", "union", "short", "break", "while", "entry"};
char   *reserve_6[] = {"double", "struct", "extern", "static", "return", "sizeof", "switch"};
char   *reserve_7[] = {"typedef", "default" };
char   *reserve_8[] = {"unsigned", "register", "continue" };
int	res_num[6] = {2, 7, 6, 7, 2, 3};
// char   *chanName[] = {"OBSch", "OBSch", "DECch", "DEC2ch", "DEC3ch", "DEC4ch", "DEC5ch"};
char   *chanName[] = {"1", "1", "2", "3", "4", "5", "6"};

FILE           *fout;
char	       *fzeroStr = "0.00";
char	       *zeroStr = "0.00";
char	       *rof1Str = "rof1";
char	       *rof2Str = "rof2";
char	       *pwStr = "pw";
char	       *oneStr = "1.00";
char	       *foneStr = "1.00";
char	       *atStr = "at";
char	       *psi = "psi";
char	       *phi = "phi";
char	       *theta = "theta";
char           *str_in;
char           *cur_line;
char           *tempnam();
char            s_token[5120];
char            argArray[2048];
char            argStr[2048*2];
char            timeStr[1024];
char            cmd_name[CMDLEN*3];
char            cmd_spare[CMDLEN];
char            include_path[MAXPATH * 2];
char            input[2048];
char            ddstr[1024];
char            tmpStr[64];
char		*argPtrs[64];
// int		argPtrs[64];
int		argNum;
int             phase_2;
int             compilePhase;
int		cmdptr;
int		Define;
int		timeIndex;
int		debug;
int		timeMode;
int		timeCode;
int		rtCode;
int		timeVnum, timeFnum;
int		timeVals[6], timeFvals[6];
int		castFlag[12];
int             line_num;

/*  imaging phase_encode_shapedgradient info */
char		*patStr[4];
char		*loopStr[4];
char		*waitStr[4];
char		*multStr[4];
char		*widthStr[4];
char		*lvlStr[4];
char		*stepStr[4];
char		*limStr[4];
char		*angStr[4];
char   *grad_name[] = {"x", "y", "z", "z" };

struct routine
{
   char            name[paramLen];
   struct routine *next;
};
struct routine *s_rout,
               *t_rout,
               *p_rout;

struct routine *s_def,
               *t_def,
               *p_def;

int G_delay();
int G_pulse();
int G_power();
int G_offset();
int G_rtdelay();
int G_simpulse();
int G_sync();
int translate_show();
void cat_arg(char *dstr, char *format, ...);
void cleansource();
void get_args();
void add_def_list();
void chk_def();
void parsesource();
void eat_quot();
void eat_singleQuote();
void eat_bracket();
void adjustment();
void changename();


void
print_usage()
{
	fprintf(stderr, " Usage: dps_ps_gen [ -Dname ] [ -Idirectory ] file.c\n");
	fprintf(stderr, "  -Dname \n");
	fprintf(stderr, "     Define name as 1.\n");
	fprintf(stderr, "  -Idirectory \n");
	fprintf(stderr, "     Add directory into the search path for #include files.\n");
}

int
main(int argc, char *argv[])
{
   char            tmp_file1[MAXPATH], tmp_file2[MAXPATH+32];
   char            time_file1[MAXPATH], time_file2[MAXPATH+32];
   char            tmp_source[MAXPATH];
   char            sourcei[MAXPATH];
   char            baseName[128];
   char		   *tptr, *sptr, *rptr;
   int		   tlen, k, fd;
   int ret __attribute__((unused));

   if (argc <= 1)
   {
	print_usage();
	exit(0);
   }
   include_path[0] = '\0';
   baseName[0] = '\0';
   debug = 0;
   timeMode = 0;
   rptr = NULL;
   sprintf(input, "gcc -E ");
   for(k = 1; k < argc; k++)
   {
      if (argv[k][0] == '-')
      {
	 if (argv[k][1] == 'd')
	    debug = 1;
	 else
	 {
	    if (argv[k][1] == 'I')
	    {
	        strcat (include_path, argv[k]);
		strcat (include_path, " ");
	    }
	    strcat (input, argv[k]);
	    strcat (input, " ");
	 }
      }
      else if ((tptr = strrchr(argv[k], '.')) != NULL)
      {
	 if (*(tptr+1) == 'c')
	 {
	    rptr = argv[k];
	    if ((sptr = strrchr(argv[k], '\\')) != NULL)
		sptr++;
	    else
		sptr = argv[k];
	    tlen = tptr - sptr;
	    strncpy(baseName, sptr, tlen);
      	    baseName[tlen] = '\0';
	 }
      }
   }
   if ((int) strlen(baseName) <= 0 || rptr == NULL)
   {
	print_usage();
	exit(0);
   }
   strcat (input, " ");
   if (debug)
	fprintf(stderr, "file: %s base: %s \n", rptr, baseName);
   if ((int) strlen(include_path) <= 0)
   {
        sprintf(include_path, "-I%s/psg", getenv("vnmrsystem"));
        strcat(input, include_path);
   }
   strcat (input, " ");
   strcat(input, rptr);
   strcat (input, " > ");
   sprintf(sourcei, "%s.i", baseName);
   strcat(input, sourcei);
   if (debug)
	fprintf(stderr, " dps_ps_gen: %s\n", input);
   ret = system(input);
   compilePhase = 0;
   if (argc > 3)
   {
      if (strcmp(argv[3], "1") == 0)
	 compilePhase = 1;	/* the second phase of conversion  */
   }

/*
   sprintf(tmp_source, "%s", tempnam("/tmp", "dps"));
*/
   if (debug)
      sprintf(tmp_source, "/tmp/dpsi");
   else {
      sprintf(tmp_source, "/tmp/dpsiXXXXXX");
      fd = mkstemp(tmp_source);
      if (fd < 0)
      {
         fprintf(stderr, "dps_ps_gen(1):  can not open file %s \n", tmp_source);
         exit(0);
      }
      close(fd);
   }
   fout = fopen(tmp_source, "w");
   if (fout == NULL)
   {
      fprintf(stderr, "dps_ps_gen(1):  can not open file %s \n", tmp_source);
      exit(0);
   }
   cleansource(sourcei, 1);  /*  create new source file without comments */
   fclose(fout);

/*
   sprintf(tmp_file1, "%s", tempnam("/tmp", "dps"));
*/
   if (debug)
      sprintf(tmp_file1, "/tmp/dpsx");
   else {
      sprintf(tmp_file1, "/tmp/dpsxXXXXXX");
      fd = mkstemp(tmp_file1);
      if (fd < 0)
      {
         fprintf(stderr, "dps_ps_gen(1):  can not open file %s \n", tmp_file1);
         exit(0);
      }
      close(fd);
   }
   sprintf(tmp_file2, "%s.c", tmp_file1);

/*
   sprintf(time_file1, "%s", tempnam("/tmp", "dps"));
*/
   sprintf(time_file1, "/tmp/dpstXXXXXX");
   fd = mkstemp(time_file1);
   if (fd < 0)
   {
      fprintf(stderr, "dps_ps_gen(1):  can not open file %s \n", time_file1);
      exit(0);
   }
   close(fd);
   sprintf(time_file2, "%s.c", time_file1);

   fout = fopen(tmp_file1, "w");
   if (fout == NULL)
   {
      fprintf(stderr, "dps_ps_gen(2):  can not open file %s. \n", tmp_file1);
      exit(0);
   }
   fprintf(fout, "\n#ifndef LINT\n");
/*
   fprintf(fout, "extern  FILE   *dpsdata;\n");
*/
#ifdef AIX
   fprintf(fout, "int x_pulsesequence();\n");
   fprintf(fout, "int t_pulsesequence();\n");
   fprintf(fout, "main(argc,argv)\n");
   fprintf(fout, "int     argc; char    *argv[];\n{\n");
   fprintf(fout, "   set_dps_func (x_pulsesequence, t_pulsesequence);\n");
   fprintf(fout, "   psg_main(argc,argv);\n}\n");
#endif

   phase_2 = 0;
   s_rout = NULL;
   s_def = NULL;
   s_token[0] = '\0';
   parsesource(tmp_source);

   fprintf(fout, "\n#endif\n");
   fclose(fout);
/*
      unlink(tmp_source);
*/
   /* if there is subroutine */
   if ( (s_rout != NULL) && (s_rout->next != NULL) )
   {
      phase_2 = 1;
      if ((fout = fopen(tmp_file2, "w")) == NULL)
      {
	 fprintf(stderr, "dps_ps_gen(3): can not open file %s.\n", tmp_file2);
	 exit(0);
      }
	/* change all subroutine names */
      parsesource(tmp_file1);
      fclose(fout);
      if (!debug)
         unlink(tmp_file1);
   }
   else
   {
      sprintf(cmd_name, "mv %s  %s", tmp_file1, tmp_file2);
      ret = system(cmd_name);
   }


   fout = fopen(time_file1, "w");
   if (fout != NULL)
   {
   	fprintf(fout, "\n#ifndef LINT\n");
	phase_2 = 0;
	timeMode = 1;
/*
	cmd6Num = 31;
	cmd7Num = 27;
*/
	rtCode = 0;
	s_rout = NULL;
	s_def = NULL;
	s_token[0] = '\0';
	parsesource(tmp_source, 0);
   	fprintf(fout, "\n#endif\n");
   	fclose(fout);

        /* if there is subroutine */
        if ( (s_rout != NULL) && (s_rout->next != NULL) )
   	{
	   phase_2 = 1;
	   if ((fout = fopen(time_file2, "w")) == NULL)
	   {
	     fprintf(stderr, "dps_ps_gen(4): can not open file %s.\n", time_file2);
	     exit(0);
           }
	   /* change all subroutine names */
	   parsesource(time_file1, 0);
	   fclose(fout);
	   unlink(time_file1);
   	}
	else
	{
	  sprintf(cmd_name, "mv %s  %s", time_file1, time_file2);
 	  ret = system(cmd_name);
   	}
   }

   unlink(tmp_source);
   fout = fopen(tmp_file1, "w");
   if (fout == NULL)
   {
      fprintf(stderr, "dps_ps_gen(2):  can not open file %s. \n", tmp_file1);
      exit(0);
   }
   fprintf(fout, "\n#pragma GCC diagnostic ignored \"-Wall\"\n");
   fprintf(fout, "#pragma GCC diagnostic ignored \"-Wextra\"\n");
   fprintf(fout, "#pragma GCC diagnostic ignored \"-Wunused\"\n");
   fprintf(fout, "#pragma GCC diagnostic ignored \"-Wuninitialized\"\n");
   fprintf(fout, "#pragma GCC diagnostic ignored \"-Wunused-variable\"\n");
#ifndef MACOS
   fprintf(fout, "#pragma GCC diagnostic ignored \"-Wunused-but-set-variable\"\n");
#endif
   fclose(fout);
   sprintf(input, "cat %s %s %s %s > %sdps.c",rptr,tmp_file1,tmp_file2,time_file2,baseName);
   if (debug)
	fprintf(stderr, "%s \n", input);
   ret = system(input);
   unlink(tmp_file1);
   unlink(tmp_file2);
   unlink(time_file2);
   if (!debug)
     unlink(sourcei);
   return(EXIT_SUCCESS);
}


/*  print argument excluded '"' */
char *clean_print(char *str, int do_print)
{
	int   n, len, quota;
	char  *data;
	static char  tstr[512];

	len = strlen(str);
	tstr[0] = '\0';
	if (len <= 0)
	{
	    if (do_print)
	        strcat(argStr, " ?? ");
	    return(tstr);
	}
	data = str;
	quota = 0;
	n = 0;
	while(len > 0)
	{
	    if (*data != ' ')
	    {
		if (*data == '"')
		{
		    if (quota)
			break;
		    n = 0;
		    quota++;
		}
		else
		    tstr[n++] = *data;
	    }
	    data++;
	    len--;
	}
	tstr[n] = '\0';
	if (do_print)
	{
	    if (n > 0)
	        cat_arg(argStr, "%s", tstr);
	    else
	        strcat(argStr, " ?? ");
	}
	return(tstr);
}

char *clean_print_int(char *str, int do_print)
{
	int   len;
	char  *data;
	static char  tstr[512];

        data = clean_print(str, do_print);
	len = strlen(data);
	if (len <= 0)
            return data;
        if (strncmp(data, "(int)", 5) == 0)
            return data;
	sprintf(tstr, "(int)(%s)", data);
        return (tstr);
}

void cat_arg(char *dstr, char *format, ...)
{
	va_list   vargs;

	va_start(vargs,format);
        vsprintf(ddstr, format, vargs);
        va_end(vargs);
	strcat(dstr, ddstr);
}

void cat_targ(char *dstr, char *format, ...)
{
	va_list   vargs;
	char	  tstr[256];

	va_start(vargs,format);
        vsprintf(tstr, format, vargs);
        va_end(vargs);
	strcat(dstr, tstr);
}


void
print_format(int ival, ...)
{
	va_list   args;
	int       num;
	char     *str;

	va_start(args, ival);
        num = ival;
	while (num > 0)
	{
		if (num <= argNum) /* print argument */
		{
		     // str = &argArray[0] + argPtrs[num-1];
		     str = argPtrs[num-1];
		     clean_print(str, 1);
		}
		else
		{
		     if (num == INTG || num == SHORT)
		        strcat(argStr, " %d ");
		     else if (num == FLOAT)
		        strcat(argStr, " %.9f ");
		     else if (num == STRG)
		        strcat(argStr, " ?%s ");
		     else if (num == CHAR)
		        strcat(argStr, " ?%c ");
		     else
		        strcat(argStr, " null ");
		}
	        num = va_arg(args, int);
	}
	if (num == 0)
	    strcat(argStr, "\\n\"");
	va_end(args);
}

/*VARARGS*/
void
print_args(int ival, ...)
{
	va_list   args;
	int       num;
	char     *str;

	va_start(args, ival);
        num = ival;
	while (num > 0)
	{
		if (num <= argNum)
		{
		     // str = &argArray[0] + argPtrs[num-1];
		     str = argPtrs[num-1];
		     if ( (str == NULL) || ((int) strlen(str) <= 0) )
		         strcat(argStr, ", -1");
		     else {
                         if (str == oneStr) {
		            strcat(argStr, ", 1");
                         }
                         else {
                            if (str == zeroStr)
		               strcat(argStr, ", 0");
                            else
		               cat_arg(argStr, ", (int)(%s)", str);
                         }
                     }
		}
		else
		     strcat(argStr, ", 0");
	        num = va_arg(args, int);
	}
	va_end(args);
}

void
print_fargs(int ival, ...)
{
	va_list   args;
	int       num, len, isFloat;
	char     *str, *str2;

	va_start(args, ival);
        num = ival;
	while (num > 0)
	{
	   len = 0;
	   if (num <= argNum)
	   {
		// str = &argArray[0] + argPtrs[num-1];
		str = argPtrs[num-1];
		len = strlen(str);
		if (len > 0)
		{
	   	    isFloat = 0;
		    str2 = str;
		    while (*str2 != '\0')
		    {
			if (*str2 < '0' || *str2 > '9')
			{
			    isFloat = 1;
			    break;
			}
			str2++;
		    }
		    if (isFloat)
		        cat_arg(argStr, ", (float)(%s)", str);
		    else
		        cat_arg(argStr, ", %s.0", str);
		}
	   }
	   if (len == 0)
		strcat(argStr, ", -1.0");
	   num = va_arg(args, int);
	}
	va_end(args);
}

/* short int */
void
print_sargs(int ival, ...)
{
	va_list   args;
	int       num;
	char     *str;

	va_start(args, ival);
        num = ival;
	while (num > 0)
	{
		if (num <= argNum)
		{
		     // str = &argArray[0] + argPtrs[num-1];
		     str = argPtrs[num-1];
		     if ( (str == NULL) || ((int) strlen(str) <= 0) )
		         strcat(argStr, ", -1");
		     else
		         cat_arg(argStr, ", (codeint)(%s)", str);
		}
		else
		     strcat(argStr, ", 0");
	        num = va_arg(args, int);
	}
	va_end(args);
}

void
print_str(int ival, ...)
{
	va_list   args;
	int       num;
	char     *str;

	va_start(args, ival);
        num = ival;
	while (num > 0)
	{
		if (num <= argNum)
		{
		     // str = &argArray[0] + argPtrs[num-1];
		     str = argPtrs[num-1];
		     if ( (str == NULL) || ((int) strlen(str) <= 0) )
		         strcat(argStr, ", \"?\"");
		     else
		         cat_arg(argStr, ", %s", str);
		}
		else
		     strcat(argStr, ", \"?\"");
	        num = va_arg(args, int);
	}
	va_end(args);
}


void
print_code(int code, char *name)
{
	sprintf(argStr, "%d %s ",code, name);
	timeCode = code;
}

void
print_dummy_code(char *name)
{
	print_code(DUMMY, name);
	strcat(argStr, " 0 0 0 \\n\"");
	timeCode = DUMMY;
}


void
print_statement()
{
	int	k, p;
	char   *timer, *rtStr;

	if (timeMode)
	{
	   sprintf(timeStr, ",%d,%d", timeVnum, timeFnum);
	   for (k = 0; k < timeVnum; k++)
	   {
	     	timer = zeroStr;
		p = timeVals[k];
		if (p > 0 && p <= argNum)
                {
		   // The third argument is the gradient name. It is cast to
		   // an int, which is wrong if compiled as 64-bit.
		   // The argument is not actually used, so jut substitute a 0
		   if ( (timeCode == PESHGR) && (p == 3) )
		      timer = "0";
		   else
	     	       timer = argPtrs[p - 1];
                }
		else if (p >= 90)
		{
		    switch (p) {
			case 90:  	/*0 */
				timer = zeroStr;
				break;
			case 91:  	/* 1 */
				timer = oneStr;
				break;
			case 92:
				timer = tmpStr;
				break;
			}
		}
		cat_targ(timeStr, ",(int)%s", timer);
	   }
	   if (timeVnum > 0 || timeFnum > 0)
	   {
	      for (k = timeVnum; k < 4; k++)
		cat_targ(timeStr, ",0");
	   }
	   if (timeFnum > 0)
	   {
/*
	      strcat(timeStr, " ");
*/
	      for (k = 0; k < timeFnum; k++)
	      {
	     	timer = fzeroStr;
		p = timeFvals[k];
		if (p > 0 && p <= argNum)
                {
	     	   // timer = &argArray[0] + argPtrs[p - 1];
	     	   timer = argPtrs[p - 1];
                }
		else if (p >= 90)
		{
		    switch (timeFvals[k]) {
			case 90:  	/* rof1 */
				timer = rof1Str;
				break;
			case 91:  	/* rof2 */
				timer = rof2Str;
				break;
			case 92:  	/* pw */
				timer = pwStr;
				break;
			case 93:  	/* at */
				timer = atStr;
				break;
			}
		}
		cat_targ(timeStr, ",(double)%s", timer);
	     }
/**
	     for (k = timeFnum; k < 3; k++)
		cat_targ(timeStr, ",%s", fzeroStr);
**/
	   }
	   if (rtCode > 300 && rtCode < 303) /* msloop | peloop */
	   {
		k = rtCode - 300;
	     	// rtStr = &argArray[0] + argPtrs[k - 1];
	     	rtStr = argPtrs[k - 1];
	        fprintf(fout, "DPStimer(%d, (int)%s%s)", timeCode, rtStr, timeStr);
	   }
	   else
           {
	       fprintf(fout, "DPStimer(%d,%d%s)", timeCode, rtCode, timeStr);
	   }
	}
	else
	{
	   if (timeIndex > 0 && timeIndex <= argNum)
           {
	     // timer = &argArray[0] + argPtrs[timeIndex-1];
	     timer = argPtrs[timeIndex-1];
           }
	   else
	     timer = fzeroStr;
	   if (timeIndex >= 90) {
	      switch (timeIndex) {
		   case 90:  	/* rof1 */
			timer = rof1Str;
			break;
		   case 91:  	/* rof2 */
			timer = rof2Str;
			break;
		   case 92:  	/* pw */
			timer = pwStr;
			break;
		   case 93:  	/* at */
			timer = atStr;
			break;
		}
	   }
	   fprintf(fout, "DPSprint(%s, \"%s)", timer, argStr);
	}
}

void
change_code(char *pname, char *name, int code, int arg_num)
{
	int       num, n;
	char     *str;

	if (argNum < arg_num)
	{
	      print_dummy_code(name);
	      print_statement();
	      return;
	}
	sprintf(argStr, "dps_%s(\"%s\", %d, %d",pname, name, code, DELAY);

	for(num = 0; num < arg_num; num++)
	{
	    // str = &argArray[0] + argPtrs[num];
	    str = argPtrs[num];
            switch (castFlag[num])
            {
               case INTG:
                      if (strstr(str, "(int)") == str)
                          cat_arg(argStr, ",%s", str);
                      else
                          cat_arg(argStr, ",(int)%s", str);
                      break;
               case FLOAT:
                      if (strstr(str, "(double)") == str)
	                  cat_arg(argStr, ",%s", str);
                      else
	                  cat_arg(argStr, ",(double)%s", str);
                      break;
               case SHORT:
                      if (strstr(str, "(codeint)") == str)
	                  cat_arg(argStr, ",%s", str);
                      else
	                  cat_arg(argStr, ",(codeint)%s", str);
                      break;
               case FLOATARRAY:
	              cat_arg(argStr, ",%s", str);
                      break;
               case INTGARRAY:
	              cat_arg(argStr, ",%s", str);
                      break;
               case CHAR:
	              cat_arg(argStr, ",%s", str);
                      break;
               default:
                      n = (int) strlen(str);
                      if (n > 2) {
                          if ((str[0] == '\'') && (str[n-1] == '\'')) {
                              str[0] = '\"';
                              str[n-1] = '\"';
                          }
                      }
	              cat_arg(argStr, ", %s", str);
                      break;
            }

            /************
	    if (castFlag[num] == INTG)
	       cat_arg(argStr, ",(int)%s", str);
	    else if (castFlag[num] == FLOAT)
	       cat_arg(argStr, ",(double)%s", str);
	    else if (castFlag[num] == SHORT)
	       cat_arg(argStr, ",(codeint)%s", str);
	    else {
               n = (int) strlen(str);
               if (n > 2) {
                   if ((str[0] == '\'') && (str[n-1] == '\'')) {
                       str[0] = '\"';
                       str[n-1] = '\"';
                   }
               }
               
	       cat_arg(argStr, ", %s", str);
            }
            *************/
	}
	for(num = 0; num < arg_num; num++)
	{
	    // str = &argArray[0] + argPtrs[num];
	    str = argPtrs[num];
	    str = clean_print(str, 0);
	    cat_arg(argStr, ", \"%s\"", str);
	}
	fprintf(fout, "%s)", argStr);
	for(num = 0; num < 12; num++)
	    castFlag[num] = 0;
}

void
print_pesh_code(char *name)
{
     int k;
// char   *grad_name[] = {"x", "y", "z", "z" };

     argStr[0] = '\0';
     for (k = 0; k < 3; k++) {
        cat_arg(argStr, "%d %s ",PESHGR,name);
        strcat(argStr, " 12 3 5 ");
        strcat(argStr, grad_name[k]);
        strcat(argStr, " ?%s ");
        clean_print(loopStr[k], 1);
        strcat(argStr, " %d ");
        clean_print(waitStr[k], 1);
        strcat(argStr, " %d ");
        clean_print(multStr[k], 1);
        strcat(argStr, " %d ");
        clean_print(widthStr[k], 1);
        strcat(argStr, " %.9f ");
        clean_print(lvlStr[k], 1);
        strcat(argStr, " %.9f ");
        clean_print(stepStr[k], 1);
        strcat(argStr, " %.9f ");
        clean_print(limStr[k], 1);
        strcat(argStr, " %.9f ");
        clean_print(angStr[k], 1);
        strcat(argStr, " %.9f ");
        strcat(argStr, "\\n");
     } 
     strcat(argStr, "\",");
     for (k = 0; k < 3; k++) {
        strcat(argStr, "\n      ");
        // clean_print(patStr[k], 1);
        strcat(argStr, patStr[k]);
        strcat(argStr, ", (int)");
        clean_print(loopStr[k], 1);
        strcat(argStr, ", (int)");
        clean_print(waitStr[k], 1);
        strcat(argStr, ", (int)");
        clean_print(multStr[k], 1);
        strcat(argStr, ", (float)");
        clean_print(widthStr[k], 1);
        strcat(argStr, ", (float)");
        clean_print(lvlStr[k], 1);
        strcat(argStr, ", (float)");
        clean_print(stepStr[k], 1);
        strcat(argStr, ", (float)");
        clean_print(limStr[k], 1);
        strcat(argStr, ", (float)");
        clean_print(angStr[k], 1);
        if (k < 2)
           strcat(argStr, ",");
     } 
     
}

void
print_oblsh_code(char *name)
{
    print_code(OBLSHGR, name);
    strcat(argStr, " 12 2 3 x ?%s ");
    print_format(11,INTG,-1);  /* loop */
    strcat(argStr, " 0.00 0 ");  /* wait */
    print_format(4,FLOAT,5,FLOAT, 8, FLOAT, -1);  /* width. level, angle */
    cat_arg(argStr, "\\n%d %s 12 2 3 y ?%%s ",OBLSHGR,name);
    print_format(11,INTG,-1);
    strcat(argStr, " 0.00 0 ");
    print_format(4,FLOAT,6,FLOAT,9,FLOAT, -1);
    cat_arg(argStr, "\\n%d %s 12 2 3 z ?%%s ",OBLSHGR,name);
    print_format(11,INTG, 12,INTG, -1);  /*  loop, wait */
    print_format(4,FLOAT,7,FLOAT,10,FLOAT, -1);
    strcat(argStr, "\\n\",\n");
    strcat(argStr, "     ");
    strcat(argStr, argPtrs[0]); /* pattern */
//     clean_print(argPtrs[0], 1); /* pattern */
    print_args(11, 0);
    print_fargs(4,5,8, 0);
    strcat(argStr, ",\n     ");
    strcat(argStr, argPtrs[1]); /* pattern */
  //  clean_print(argPtrs[1], 1); /* pattern */
    print_args(11, 0);
    print_fargs(4,6,9, 0);
    strcat(argStr, ",\n     ");
    strcat(argStr, argPtrs[2]); /* pattern */
  //  clean_print(argPtrs[2], 1); /* pattern */
    print_args(11, 12, 0);
    print_fargs(4,7,10, 0);
    timeIndex = 4;
    timeFnum = 1;
    timeFvals[0] = 4;
    timeVnum = 2;
    timeVals[0] = 11;  /* loop */
    timeVals[1] = 12;  /* wait */
}


void
obl_code(char *name, int code, int arg_num, int arg_num2)
{
	int       num;
	char     *str;

	if (argNum < arg_num)
	{
             if (argNum != arg_num2) {  /* nvpsg */
	        print_dummy_code(name);
	        print_statement();
	        return;
             }
             arg_num = arg_num2;
	}
	sprintf(argStr, "dps_oblique_gradient (\"%s\", %d", name, code);

	if (arg_num == 3)
	{
	    print_fargs(1, 2, 3, 0);
	    strcat(argStr, ", psi,phi,theta");
	    for(num = 0; num < argNum; num++)
	    {
		// str = &argArray[0] + argPtrs[num];
		str = argPtrs[num];
		str = clean_print(str, 0);
		cat_arg(argStr, ", \"%s\"", str);
	    }
	    strcat(argStr, ", \"psi\",\"phi\",\"theta\"");
	}
	else
	{
	    print_fargs(1, 2, 3, 4, 5, 6, 0);
	    for(num = 0; num < 6; num++)
	    {
		// str = &argArray[0] + argPtrs[num];
		str = argPtrs[num];
		str = clean_print(str, 0);
		cat_arg(argStr, ", \"%s\"", str);
	    }
	}
	
	fprintf(fout, "%s)", argStr);
}

void
pe_obl_code(char *name, int code, int arg_num)
{
	int       num;
	char     *str;

	if (argNum < arg_num)
	{
	      print_dummy_code(name);
	      print_statement();
	      return;
	}
	sprintf(argStr, "dps_pe_oblique_gradient (\"%s\", %d", name, code);

	print_str(1, 2, 3, 0);
	print_fargs(4, 5, 6, 7, 8, 0);
	print_sargs(9, 0);
	if (arg_num == 11)
	{
	    strcat(argStr, ",nv/2,psi,phi,theta");
	    print_args(10, 11, 0);
	    for(num = 0; num < 9; num++)
	    {
		// str = &argArray[0] + argPtrs[num];
		str = argPtrs[num];
		str = clean_print(str, 0);
		cat_arg(argStr, ", \"%s\"", str);
	    }
	    strcat(argStr, ", \"nv/2\",\"psi\",\"phi\",\"theta\"");
	    for(num = 9; num < 11; num++)
	    {
		// str = &argArray[0] + argPtrs[num];
		str = argPtrs[num];
		str = clean_print(str, 0);
		cat_arg(argStr, ", \"%s\"", str);
	    }
	}
	else
	{
	    print_fargs(10, 11, 12, 13, 0);
	    print_args(14, 15, 0);
	    for(num = 0; num < argNum; num++)
	    {
		// str = &argArray[0] + argPtrs[num];
		str = argPtrs[num];
		str = clean_print(str, 0);
		cat_arg(argStr, ", \"%s\"", str);
	    }
	}
	fprintf(fout, "%s)", argStr);
}

void
pe_oblshaped_code(char *name, int code, int arg_num)
{
	int       num;
	char     *str;

	if (argNum < arg_num)
	{
	      print_dummy_code(name);
	      print_statement();
	      return;
	}
	sprintf(argStr, "dps_pe_oblshapedgradient(\"%s\", %d", name, code);

	print_str(1, 2, 3, 0);
	print_fargs(4, 5, 6, 7, 8, 9, 10, 0);
	print_args(11, 12, 13, 0);
	print_args(17, 18, 0);
	for(num = 3; num < 13; num++)
	{
		str = argPtrs[num];
		str = clean_print(str, 0);
		cat_arg(argStr, ", \"%s\"", str);
	}
	str = clean_print(argPtrs[16], 0);
	cat_arg(argStr, ", \"%s\"", str);
	str = clean_print(argPtrs[17], 0);
	cat_arg(argStr, ", \"%s\"", str);
	fprintf(fout, "%s)", argStr);
}

int
get_parallel_chan_id(int n)
{
       char *str; 
       int   ch;

       if (n > argNum) 
          return (0);
       if (argPtrs[n-1] == NULL)
          return (0);
       ch = 0;
       str = clean_print(argPtrs[n-1], 0);
       if (!strcmp(str,"obs"))
          ch = TODEV;
       else if (!strcmp(str,"dec"))
          ch = DODEV;
       else if (!strcmp(str,"dec2"))
          ch = DO2DEV;
       else if (!strcmp(str,"dec3"))
          ch = DO3DEV;
       else if (!strcmp(str,"dec4"))
          ch = DO4DEV;
       else if (!strcmp(str,"grad"))
          ch = GRADDEV;
       else if (!strcmp(str,"rcvr"))
          ch = RCVRDEV;
       return (ch);
}

void
gen_shapedPulseWithOffset(char *name)
{
      timeIndex = 2;
      if (timeMode)
     	  print_code(PULSE, name);
      else
     	  print_code(OFFSHP, name);
      strcat(argStr, " 1 1 1 2 ");
      print_format(4,FLOAT,5,FLOAT, -1);
      /* channel, name, rtvar1, width, phase */
      print_format(INTG, STRG,3,INTG,2,FLOAT, 9,FLOAT,0);
      print_fargs(4, 5, 0);
      print_args(8, 0);
      print_str(1, 0);
      print_args(3, 0);
      print_fargs(2, 9, 0);
      timeFnum = 3;
      timeFvals[0] = 2;
      timeFvals[1] = 4;
      timeFvals[2] = 5;
}


void
poffset_list_code(char *name, int code, int arg_num)
{
	int       num;
	char     *str;

	if (argNum < arg_num)
	{
	      print_dummy_code(name);
	      print_statement();
	      return;
	}
	sprintf(argStr, "dps_poffset_list(\"%s\", %d", name, code);

	if (arg_num == 4) /* poffset_list */
	{
	    strcat(argStr, ", 1");
	    str = &argArray[0];
	    str = clean_print(str, 0);
	    cat_arg(argStr, ", %s", str);
	    print_fargs(2, 3, 0);
	    strcat(argStr, ", resto, -1");
	    print_args(4, 0);
	    for(num = 0; num < 3; num++)
	    {
		// str = &argArray[0] + argPtrs[num];
		str = argPtrs[num];
		str = clean_print(str, 0);
		cat_arg(argStr, ", \"%s\"", str);
	    }
	    strcat(argStr, ",\"resto\", \"0\"");
	    // str = &argArray[0] + argPtrs[3];
	    str = argPtrs[3];
	    str = clean_print(str, 0);
	    cat_arg(argStr, ", \"%s\"", str);
	}
	else
	{
	    print_args(5, 0);
	    str = &argArray[0];
	    str = clean_print(str, 0);
	    cat_arg(argStr, ", %s", str);
	    print_fargs(2, 3, 4, 0);
	    print_args(6, 7, 0);
	    for(num = 0; num < 4; num++)
	    {
		// str = &argArray[0] + argPtrs[num];
		str = argPtrs[num];
		str = clean_print(str, 0);
		cat_arg(argStr, ", \"%s\"", str);
	    }
	    for(num = 5; num < 7; num++)
	    {
		// str = &argArray[0] + argPtrs[num];
		str = argPtrs[num];
		str = clean_print(str, 0);
		cat_arg(argStr, ", \"%s\"", str);
	    }
	}
	fprintf(fout, "%s)", argStr);
}

void
pbox_code(char *fname, char *name, int dev, int arg_num)
{
	int       num;
	char     *str;

	if (argNum < arg_num)
	{
	      print_dummy_code(name);
	      print_statement();
	      return;
	}
	sprintf(argStr, "dps_%s(\"%s\", %d", fname, name, dev);
	for(num = 0; num < arg_num; num++)
	{
	    // str = &argArray[0] + argPtrs[num];
	    str = argPtrs[num];
	    cat_arg(argStr, ", %s", str);
	}
	for(num = 0; num < arg_num; num++)
	{
	    // str = &argArray[0] + argPtrs[num];
	    str = argPtrs[num];
	    str = clean_print(str, 0);
	    cat_arg(argStr, ", \"%s\"", str);
	}
	fprintf(fout, "%s)", argStr);
}

void
create_rotation_list()
{
	char     *str;

	if (argNum < 3)
	{
	      print_dummy_code("create_rotation_list");
	      print_statement();
	      return;
	}
	sprintf(argStr, "dps_create_rotation_list(");
	str = argPtrs[0];
	str = clean_print(str, 0);
	cat_arg(argStr, "\"%s\", %d", str, ROTATELIST);
	str = argPtrs[1];
	str = clean_print(str, 0);
	cat_arg(argStr, ", %s", str);
	str = argPtrs[2];
	str = clean_print_int(str, 0);
	cat_arg(argStr, ", %s", str);
	fprintf(fout, "%s)", argStr);
}

void
create_angle_list()
{
	char     *str;

	if (argNum < 3)
	{
	      print_dummy_code("create_angle_list");
	      print_statement();
	      return;
	}

	sprintf(argStr, "dps_create_angle_list(");
	str = argPtrs[0];
	str = clean_print(str, 0);
	cat_arg(argStr, "\"%s\", %d", str, ANGLELIST);
	str = argPtrs[1];
	str = clean_print(str, 0);
	cat_arg(argStr, ", %s", str);
	str = argPtrs[2];
	str = clean_print_int(str, 0);
	cat_arg(argStr, ", %s", str);
	fprintf(fout, "%s)", argStr);
}

int
loadtable_code()
{
	char	*str;

	str = &argArray[0];
	if (argNum >= 1 && ((int) strlen(str) > 0))
	{
            sprintf(argStr, "dps_loadtable(%s)", str);
            fprintf(fout, "%s", argStr);
            return(1);
        }
        return(0);
}

void
insert_angles(int index)
{
     int  k, d1, d2;

     d1 = argNum - 1;
     d2 = argNum + 2;
     for (k = index; k <= argNum; k++) {
         argPtrs[d2] = argPtrs[d1];
         d2--;
         d1--;
     }
     argNum += 3;
     d1 = index - 1;
     argPtrs[d1] = psi;
     argPtrs[d1+1] = phi;
     argPtrs[d1+2] = theta;
}   

void
insert_arg(char *s, int index)
{
     int  k, d1, d2;

     d1 = argNum - 1;
     d2 = argNum;
     for (k = index; k <= argNum; k++) {
         argPtrs[d2] = argPtrs[d1];
         d2--;
         d1--;
     }
     argNum++;
     argPtrs[index-1] = s;
}

/* dump  content before command token */
void
dump_previous()
{
   s_token[cmdptr] = '\0';
   fprintf(fout, "%s", s_token);
   s_token[0] = '\0';
   cmdptr = 0;
   cmdptr = 0;
}

void
phase_encode3_oblshapedgradient(char *token)
{
    timeFnum = 1;
    timeFvals[0] = 4;
    timeVnum = 4;
    timeVals[0] = 17;  /* loop */
    timeVals[1] = 18;  /* wait */

    timeIndex = 4;
    print_code(PESHGR, token);
    strcat(argStr, " 12 3 4 x ");
    print_format(STRG, 17,INTG, -1); // loop
    strcat(argStr, " 0.00  0 ");
    print_format(11,INTG, -1);  // multiple
    print_format(4,FLOAT,5,FLOAT,8,FLOAT,14,FLOAT,-1);
    //  width, level, step, multiple2

    cat_arg(argStr, " \\n%d %s 12 3 4 y ", PESHGR, token);
    print_format(STRG, 17,INTG, -1);
    strcat(argStr, " 0.00  0 ");
    print_format(12,INTG, -1);
    print_format(4,FLOAT,6,FLOAT,9,FLOAT,15,FLOAT,-1);

    cat_arg(argStr, " \\n%d %s 12 3 4 z ", PESHGR, token);
    print_format(STRG, 17,INTG,18,INTG,13,INTG, -1);
    print_format(4,FLOAT,7,FLOAT,10,FLOAT,16,FLOAT,0);
    strcat(argStr, "\n           ");
    print_str(1, 0);
    print_args(17,11, 0);
    print_fargs(4,5,8,14, 0);

    strcat(argStr, "\n           ");
    print_str(2, 0);
    print_args(17,12, 0);
    print_fargs(4,6,9,15, 0);

    strcat(argStr, "\n           ");
    print_str(3, 0);
    print_args(17,18,13, 0);
    print_fargs(4,7,10,16, 0);
    timeFnum = 1;
    timeFvals[0] = 4;
    timeVnum = 4;
    timeVals[0] = 17;  /* loop */
    timeVals[1] = 18;  /* wait */
}

void
gen_shapelist_init(char *token, int chan)
{
    if (argNum < 6) {
        print_dummy_code(token);
        print_statement();
        return;
    }
    if (argNum < 7) {
        argPtrs[6] = chanName[chan];
        argNum = 7;
        sprintf(tmpStr, "(1.0-%s)", argPtrs[4]);
        argPtrs[4] = tmpStr;
    }
    castFlag[1] = FLOAT;
    castFlag[2] = FLOATARRAY;
    castFlag[3] = FLOAT;
    castFlag[4] = FLOAT;
    castFlag[5] = CHAR;
    castFlag[6] = INTG;
    change_code("new_shapelist", token, SHLIST, 7);
}

void
gen_shapelistpw(char *token)
{
    print_dummy_code(token);
}

void
gen_shapedpulselist(char *token, int chan)
{
    if (argNum < 7) {
        print_dummy_code(token);
        print_statement();
        return;
    }
    if (argNum < 8) {
        argPtrs[7] = chanName[chan];
        argNum = 8;
    }
    castFlag[0] = INTG;
    castFlag[1] = FLOAT;
    castFlag[2] = INTG;
    castFlag[3] = FLOAT;
    castFlag[4] = FLOAT;
    castFlag[5] = CHAR;
    castFlag[6] = INTG;
    castFlag[7] = INTG;
    change_code("new_shapedpulselist", token, OFFSHP, 8);
}

void
gen_shapedpulseoffset(char *token, int chan)
{
    if (argNum < 6) {
        print_dummy_code(token);
        return;
    }
    if (argNum < 7) {
        argPtrs[6] = chanName[chan];
        argNum = 7;
    }
    insert_arg(argPtrs[5], 7);
    insert_arg(argPtrs[5], 9);
    gen_shapedPulseWithOffset(token);
}

int
compare_3(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd3Num; loop++)
	 {
	    if (strcmp(token, cmd_3[loop]) == 0)
	    {
		dump_previous();
		print_code(RVOP, token);
		get_args(fin);
		switch (loop) {
		  case  0:	/* add  */
			    cat_arg(argStr, "%d 3 0 ", RADD);
			    print_format(1,INTG, 2,INTG, 3,INTG, 0);
			    print_args(1, 2, 3, 0);
			    rtCode = RADD;
			    timeVnum = 3;
			    break;
		  case  1:	/* dbl  */
			    cat_arg(argStr, "%d 2 0 ", RDBL);
			    print_format(1,INTG, 2,INTG, 0);
			    print_args(1, 2, 0);
			    rtCode = RDBL;
			    timeVnum = 2;
			    break;
		  case  2:	/* hlv  */
			    cat_arg(argStr, "%d 2 0 ", RHLV);
			    print_format(1,INTG, 2,INTG, 0);
			    print_args(1, 2, 0);
			    rtCode = RHLV;
			    timeVnum = 2;
			    break;
		  case  3:	/* sub  */
			    cat_arg(argStr, "%d 3 0 ", RSUB);
			    print_format(1,INTG, 2,INTG, 3,INTG, 0);
			    print_args(1, 2, 3, 0);
			    rtCode = RSUB;
			    timeVnum = 3;
			    break;
		  default:
			    strcat(argStr, " 989 0 0");
			    print_format(0);
			    rtCode = 989;
			    timeVnum = 0;
			    break;
		}
		if (timeMode && timeVnum > 0)
		{
		    timeVals[0] = 1;
		    timeVals[1] = 2;
		    timeVals[2] = 3;
		}
		print_statement();
		return(1);
	    }
	 }
	 return (0);
}


int
compare_4(char *token, FILE *fin)
{
	int	loop;

	for (loop = 0; loop < cmd4Num; loop++)
	{
	    if (strcmp(token, cmd_4[loop]) == 0)
	    {
		dump_previous();
		if (loop < 7)
		  print_code(RVOP, token);
		get_args(fin);
		switch (loop) {
		  case  0:	/* decr  */
			    cat_arg(argStr, "%d 1 0 ", RDECR);
			    print_format(1,INTG, 0);
			    print_args(1, 0);
			    rtCode = RDECR;
			    timeVnum = 1;
			    break;
		  case  1:	/* divn  */
			    cat_arg(argStr, "%d 3 0 ", RDIVN);
			    print_format(1,INTG, 2,INTG, 3,INTG, 0);
			    print_args(1, 2, 3, 0);
			    rtCode = RDIVN;
			    timeVnum = 3;
			    break;
		  case  2:	/* incr  */
			    cat_arg(argStr, "%d 1 0 ", RINCR);
			    print_format(1,INTG, 0);
			    print_args(1, 0);
			    rtCode = RINCR;
			    timeVnum = 1;
			    break;
		  case  3:	/* mod2  */
			    cat_arg(argStr, "%d 2 0 ", RMOD2);
			    print_format(1,INTG, 2, INTG, 0);
			    print_args(1, 2, 0);
			    rtCode = RMOD2;
			    timeVnum = 2;
			    break;
		  case  4:	/* mod4  */
			    cat_arg(argStr, "%d 2 0 ", RMOD4);
			    print_format(1,INTG, 2, INTG, 0);
			    print_args(1, 2, 0);
			    rtCode = RMOD4;
			    timeVnum = 2;
			    break;
		  case  5:	/* modn  */
			    cat_arg(argStr, "%d 3 0 ", RMODN);
			    print_format(1,INTG, 2,INTG, 3,INTG, 0);
			    print_args(1, 2, 3, 0);
			    rtCode = RMODN;
			    timeVnum = 3;
			    break;
		  case  6:	/* mult  */
			    cat_arg(argStr, "%d 3 0 ", RMULT);
			    print_format(1,INTG, 2,INTG, 3,INTG, 0);
			    print_args(1, 2, 3, 0);
			    rtCode = RMULT;
			    timeVnum = 3;
			    break;
		  case  7:	/* loop  */
			    if (argNum > 2) /* gemini */
			    {
		  	        print_code(GLOOP, token);
			        strcat(argStr, " 1 3 0 ");
                                print_format(2,INTG, 3, INTG, 1, INTG, 0);
                                print_args(2, 3, 1, 0);
				rtCode = 3;
                            }
                            else
                            {
		  	        print_code(SLOOP, token);
			        strcat(argStr, " 1 2 0 ");
			        print_format(1,INTG, 2, INTG, 0);
			        print_args(1, 2, 0);
				rtCode = 2;
                            }
			    timeVnum = 3;
			    break;
		  case  8:	/* pwrf  */
		  	    print_code(PWRF, token);
			    print_format(INTG, -1);
			    strcat(argStr, " 1 0 ");
			    print_format(1,INTG, 0);
			    print_args(2, 1, 0);
			    break;
		  case  9:	/* pwrm  */
		  	    print_code(PWRM, token);
			    print_format(INTG, -1);
			    strcat(argStr, " 1 0 ");
			    print_format(1,INTG, 0);
			    print_args(2, 1, 0);
			    break;
		  case  10:	/* rfon  */
		  	    print_code(DEVON, token);
			    print_format(INTG, -1);
			    strcat(argStr, " 1 0 ");
			    strcat(argStr, " 1 1 \\n\"");
			    print_args(1, 0);
			    break;
		  case  11:	/* CEnd  */
		  	    print_code(CEND, token);
			    timeIndex = 1;
			    strcat(argStr, " 0 0 1 ");
			    print_format(1, FLOAT, 0);
			    print_fargs(1, 0);
			    timeFnum = 1;
			    timeFvals[0] = 1;
			    break;
		  default:
	      		    print_dummy_code(token);
			    break;
		}
		if (timeMode && loop <= 7)
		{
		   timeVals[0] = 1;
		   timeVals[1] = 2;
		   timeVals[2] = 3;
		}
		print_statement();
		return(1);
	    }
	}
	return (0);
}


int
compare_5(char *token, FILE *fin)
{
	int	loop;

	for (loop = 0; loop < cmd5Num; loop++)
	{
	    if (strcmp(token, cmd_5[loop]) == 0)
	    {
		dump_previous();
		get_args(fin);
		if (loop < 3)
		  print_code(TBLOP, token);
		else if (loop < 6)
		  print_code(TSOP, token);
		else if (loop == 6) /*  abort */
		{
		    fprintf(fout, "abort()");
		    return(1);
		}
		switch (loop) {
		  case  0:	/* ttadd  */
			    cat_arg(argStr, "%d 3 0 ", RADD);
			    print_format(1,INTG, 2,INTG, 3,INTG, 0);
			    print_args(1, 2, 3, 0);
			    rtCode = RADD;
			    timeVnum = 3;
			    break;
		  case  1:	/* ttdiv  */
			    cat_arg(argStr, "%d 3 0 ", RDIVN);
			    print_format(1,INTG, 2,INTG, 3,INTG, 0);
			    print_args(1, 2, 3, 0);
			    rtCode = RDIVN;
			    timeVnum = 3;
			    break;
		  case  2:	/* ttsub  */
			    cat_arg(argStr, "%d 3 0 ", RSUB);
			    print_format(1,INTG, 2,INTG, 3,INTG, 0);
			    print_args(1, 2, 3, 0);
			    rtCode = RSUB;
			    timeVnum = 3;
			    break;
		  case  3:	/* tsadd  */
			    cat_arg(argStr, "%d 3 0 ", RADD);
			    print_format(1,INTG, 2,INTG, 3,INTG, 0);
			    print_args(1, 2, 3, 0);
			    rtCode = RADD;
			    timeVnum = 3;
			    break;
		  case  4:	/* tsdiv  */
			    cat_arg(argStr, "%d 3 0 ", RDIVN);
			    print_format(1,INTG, 2,INTG, 3,INTG, 0);
			    print_args(1, 2, 3, 0);
			    rtCode = RDIVN;
			    timeVnum = 3;
			    break;
		  case  5:	/* tssub  */
			    cat_arg(argStr, "%d 3 0 ", RSUB);
			    print_format(1,INTG, 2,INTG, 3,INTG, 0);
			    print_args(1, 2, 3, 0);
			    rtCode = RSUB;
			    timeVnum = 3;
			    break;
		  case  7:	/* delay  */
		  	    print_code(DELAY, token);
			    timeIndex = 1;
			    strcat(argStr, " 1 0 1 ");
			    print_format(1, FLOAT, 0);
			    print_fargs(1, 0);
			    timeFnum = 1;
			    timeFvals[0] = 1;
			    break;
		  case  8:	/* pulse  */
		  	    print_code(PULSE, token);
			    timeIndex = 1;
			/* numch, shaped, int_num, float_num, rg1, rg2 */
			    strcat(argStr," 1 0 1 1 rof1 %.9f rof2 %.9f ");
			    cat_arg(argStr," %d ", TODEV);
			/*  phase, width */
			    print_format(2,INTG, 1, FLOAT, 0);
			    strcat(argStr, ", rof1, rof2");
			    print_args(2, 0);
			    print_fargs(1, 0);
			    timeFnum = 3;
			    timeFvals[0] = 1;
			    timeFvals[1] = 90;  /* rof1 */
			    timeFvals[2] = 91;  /* rof2 */
			    break;
		  case  9:	/* xgate  */
		  	    print_code(XGATE, token);
			    strcat(argStr, " 1 0 1 ");
			    print_format(1, FLOAT, 0);
			    print_fargs(1, 0);
			    break;
		  case  10:	/* endif  */
		  	    print_code(ENDIF, token);
			    strcat(argStr, " 0 1 0 ");
			    print_format(1,INTG, 0);
			    print_args(1, 0);
			    timeVnum = 1;
			    timeVals[0] = 1;
			    break;
		  case  11:	/* power  */
		  	    print_code(POWER, token);
			    print_format(INTG, -1);
			    strcat(argStr, " 1 0 ");
			    print_format(1,INTG, 0);
			    print_args(2, 1,0);
			    break;
		  case  12:	/* decon  */
		  	    print_code(DEVON, token);
			    cat_arg(argStr, " %d 1 0 on 1", DODEV);
			    print_format(0);
			    break;
		  case  13:	/* ipwrf  */
	      		    print_dummy_code(token);
			    break;
		  case  14:	/* ipwrm  */
	      		    print_dummy_code(token);
			    break;
		  case  15:	/* sp1on  */
		  	    print_code(SPON, token);
			    strcat(argStr, " 1 1 0 1 1 \\n\"");
			    break;
		  case  16:	/* sp2on  */
		  	    print_code(SPON, token);
			    strcat(argStr, " 1 1 0 2 1 \\n\"");
			    break;
		  case  17:	/* rfoff  */
		  	    print_code(DEVON, token);
			    print_format(INTG, -1);
			    strcat(argStr, " 1 0 off 0 \\n\"");
			    print_args(1, 0);
			    break;
		  case  18:	/* vsacn  */
		  	    print_code(VSCAN, token);
			    strcat(argStr, " 0 1 0 ");
			    print_format(1, INTG, 0);
			    print_args(1, 0);
			    break;
		  case  19:	/* vfreq  */
		  	    print_code(VFREQ, token);
			    strcat(argStr, " 1 2 0 ");
			    print_format(1, INTG, 2, INTG, 0);
			    print_args(1, 2, 0);
			    break;
		  case  20:	/* sp3on  */
		  	    print_code(SPON, token);
			    strcat(argStr, " 1 1 0 3 1 \\n\"");
			    break;
		  case  21:	/* sp4on  */
		  	    print_code(SPON, token);
			    strcat(argStr, " 1 1 0 4 1 \\n\"");
			    break;
		  case  22:	/* sp5on  */
		  	    print_code(SPON, token);
			    strcat(argStr, " 1 1 0 5 1 \\n\"");
			    break;
		  case  23:	/* sp_on  */
		  	    print_code(SPON, token);
			    strcat(argStr, " 1 1 0 %d 1 \\n\"");
			    print_args(1, 0);
			    break;
		  case  24:	/* CMark  */
		  	    print_code(CMARK, token);
			    strcat(argStr, " 0 0 0 0 0 \\n\"");
			    break;
		  case  25:	/* recon  */
		     	    print_code(RCVRON, token);
			    strcat(argStr, " 1 2 0 on 1 0 0 \\n\"");
			    break;
		  default:
	      		    print_dummy_code(token);
			    break;
		}
		if (timeMode && loop < 6)
		{
		   timeVals[0] = 1;
		   timeVals[1] = 2;
		   timeVals[2] = 3;
		}
		print_statement();
		return(1);
	    }
	}
	return (0);
}

int
compare_6(char *token, FILE *fin)
{
	int	loop, k;

        loop = -1;
	for (k = 0; k < cmd6Num; k++)
	{
	    if (strcmp(token, cmd_6[k]) == 0)
            {
                 loop = k;
                 break;
            }
        }
        if (loop < 0) 
	    return (0);
                 
	dump_previous();
	if (loop < 1)  /* assign */
	     print_code(RVOP, token);
	else if (loop == 24)  /* ttmult */
	     print_code(TBLOP, token);
	else if (loop == 25)  /* tsmult */
	     print_code(TSOP, token);
	get_args(fin);
	switch (loop) {
	     case  0:	/* assign  */
			    cat_arg(argStr, "%d 2 0 ", RASSIGN);
			    print_format(1,INTG,  2,INTG, 0);
			    print_args(1, 2, 0);
			    rtCode = RASSIGN;
			    timeVnum = 1;
			    timeVals[0] = 1;
			    break;
	     case  1:    /* idelay */
		     	    print_code(DELAY, token);
			    timeIndex = 1;
			    strcat(argStr, " 1 0 1 ");
			    print_format(1, FLOAT, 0);
			    print_fargs(1, 0);
			    timeFnum = 1;
			    timeFvals[0] = 1;
			    break;
  	     case  2:    /* ipulse */
		     	    print_code(PULSE, token);
			    timeIndex = 1;
			    strcat(argStr," 1 0 1 1 rof1 %.9f rof2 %.9f ");
			    cat_arg(argStr," %d ", TODEV);
			    print_format(2, INTG, 1, FLOAT, 0);
			    strcat(argStr, ", rof1, rof2");
			    print_args(2, 0);
			    print_fargs(1, 0);
			    timeFnum = 3;
			    timeFvals[0] = 1;
			    timeFvals[1] = 90;
			    timeFvals[2] = 91;
		  	    break;
	     case  3:    /* ifzero */
		     	    print_code(IFZERO, token);
			    strcat(argStr, " 0 1 0 ");
			    if (argNum > 1)
			       k = 2;
			    else
			       k = 1;
			    print_format(k, INTG, 0);
			    print_args(k, 0);
			    timeVnum = 1;
			    timeVals[0] = k;
			    break;
	     case  4:    /* elsenz */
		     	    print_code(ELSENZ, token);
			    strcat(argStr, " 0 1 0 ");
			    print_format(1, INTG, 0);
			    print_args(1, 0);
			    timeVnum = 1;
			    timeVals[0] = 1;
			    break;
	     case  5:    /* decpwr */
		     	    print_code(DECPWR, token);
			    cat_arg(argStr, " %d 0 1 ", DODEV);
			    print_format(1, FLOAT, 0);
			    print_fargs(1, 0);
			    break;
	     case  6:    /* status */
		     	    print_code(STATUS, token);
			    strcat(argStr, " 0 1 0 ");
			    print_format(1, INTG, 0);
			    print_args(1, 0);
			    break;
  	     case  7:    /* xmtron */
		     	    print_code(DEVON, token);
			    cat_arg(argStr, " %d 1 0 on 1 \\n\"", TODEV);
			    break;
	     case  8:    /* decoff */
		     	    print_code(DEVON, token);
			    cat_arg(argStr, " %d 1 0 off 0 \\n\"", DODEV);
			    break;
	     case  9:    /* dec2on */
		     	    print_code(DEVON, token);
			    cat_arg(argStr, " %d 1 0 on 1 \\n\"", DO2DEV);
			    break;
	     case 10:    /* dec3on */
		     	    print_code(DEVON, token);
			    cat_arg(argStr, " %d 1 0 on 1 \\n\"", DO3DEV);
			    break;
	     case 11:    /* vdelay */
		     	    print_code(VDELAY, token);
			    strcat(argStr, " 1 2 0 ");
			    print_format(2, INTG, 1,INTG, 0);
			    print_args(2, 1, 0);
			    timeVnum = 2;
			    timeVals[0] = 1;
			    timeVals[1] = 2;
			    break;
	     case 12:    /* offset */
		     	    print_code(OFFSET, token);
			    print_format(INTG, -1);
			    strcat(argStr, " 0 1 ");
			    print_format(1, FLOAT, 0);
			    print_args(2, 0);
			    print_fargs(1, 0);
			    break;
	     case 13:    /* rlpwrf */
		     	    print_code(RLPWRF, token);
			    print_format(INTG, -1);
			    strcat(argStr, " 0 1 ");
			    print_format(1, FLOAT, 0);
			    print_args(2, 0);
			    print_fargs(1, 0);
			    break;
	     case 14:    /* rlpwrm */
		     	    print_code(RLPWRM, token);
			    print_format(INTG, -1);
			    strcat(argStr, " 0 1 ");
			    print_format(1, FLOAT, 0);
			    print_args(2, 0);
			    print_fargs(1, 0);
			    break;
	     case 15:    /* rcvron */
		     	    print_code(RCVRON, token);
			    strcat(argStr, " 1 2 0 on 1 1 1 \\n\"");
			    break;
	     case 16:    /* sp1off */
		     	    print_code(SPON, token);
			    strcat(argStr, " 1 1 0 1 0 \\n\"");
			    break;
	     case 17:    /* sp2off */
		     	    print_code(SPON, token);
			    strcat(argStr, " 1 1 0 2  0 \\n\"");
			    break;
	     case 20:    /* msloop */
		     	    print_code(MSLOOP, token);
			    strcat(argStr, " 1 3 1 ");
			    print_format(CHAR,INTG,3,INTG,4,INTG, 2,FLOAT, 0);
			    print_str(1, 0);
			    print_args(3, 3, 4, 0);
			    print_fargs(2, 0);
			    timeVnum = 2;
			    timeVals[0] = 3;
			    timeVals[1] = 4;
			    timeFnum = 1;
			    timeFvals[0] = 2;
			    rtCode = 301;
			    break;
	     case 21:    /* peloop */
		     	    print_code(PELOOP, token);
			    strcat(argStr, " 1 3 1 ");
			    print_format(CHAR,INTG,3,INTG,4,INTG, 2,FLOAT, 0);
			    print_str(1, 0);
			    print_args(3, 3, 4, 0);
			    print_fargs(2, 0);
			    timeVnum = 2;
			    timeVals[0] = 3;
			    timeVals[1] = 4;
			    timeFnum = 1;
			    timeFvals[0] = 2;
			    rtCode = 301;
			    break;
	     case 24:	/* ttmult  */
	     case 25:	/* tsmult  */
			    cat_arg(argStr, "%d 3 0 ", RMULT);
			    print_format(1,INTG, 2,INTG, 3,INTG, 0);
			    print_args(1, 2, 3, 0);
			    timeVnum = 3;
			    timeVals[0] = 1;
			    timeVals[1] = 2;
			    timeVals[2] = 3;
			    rtCode = RMULT;
			    break;
	     case 26:    /* sp3off */
		     	    print_code(SPON, token);
			    strcat(argStr, " 1 1 0 3 0 \\n\"");
			    break;
	     case 27:    /* sp4off */
		     	    print_code(SPON, token);
			    strcat(argStr, " 1 1 0 4 0 \\n\"");
			    break;
	     case 28:    /* sp5off */
		     	    print_code(SPON, token);
			    strcat(argStr, " 1 1 0 5 0 \\n\"");
			    break;
	     case 29:    /* sp_off */
		     	    print_code(SPON, token);
			    strcat(argStr, " 1 1 0 %d 0 \\n\"");
			    print_args(1, 0);
			    break;
	     case 30:	/* CDelay  */
		  	    print_code(CDELAY, token);
			    timeIndex = 1;
			    strcat(argStr, " 1 0 1 ");
			    print_format(1, FLOAT, 0);
			    print_fargs(1, 0);
			    timeFnum = 1;
			    timeFvals[0] = 1;
			    break;
	     case 31:	/* CPulse  */
			    timeIndex = 2;
		     	    print_code(CPULSE, token);
			    strcat(argStr, " 1 0 1 1 ");
			    print_format(4, FLOAT, -1);
			    strcat(argStr, " 0.0 0.0 ");
			    print_format(INTG, 3, INTG, 2, FLOAT, 0);
			    print_fargs(4, 0);
			    print_args(1, 3, 0);
			    print_fargs(2, 0);
			    timeFnum = 2;
			    timeFvals[0] = 2;
			    timeFvals[1] = 4;
		  	    break;
	     case 32:    /* dec4on */
		     	    print_code(DEVON, token);
			    cat_arg(argStr, " %d 1 0 on 1 \\n\"", DO4DEV);
			    break;
	     case 33:    /* dec5on */
		     	    print_code(DEVON, token);
			    cat_arg(argStr, " %d 1 0 on 1 \\n\"", DO5DEV);
			    break;
	     case 34:    /* recoff */
		     	    print_code(RCVRON, token);
			    strcat(argStr, " 1 2 0 off 0 1 1 \\n\"");
			    break;
	     case 35:    /* rotate */
		     	    print_code(ROTATE, token);
			    strcat(argStr, " 1 0 6  psi %.9f phi %.9f theta %.9f ");
			    strcat(argStr, " offsetx %.9f offsety %.9f offsetz %.9f \\n\"");
			     strcat(argStr, ", (float)psi, (float)phi, (float)theta");
			     strcat(argStr, ", (float)offsetx, (float)offsety, (float)offsetz ");
			    break;
	     case 36:    /* ifrtGT */
		     	    print_code(IFRT, token);
			    strcat(argStr, " 0 4 0 ");
                            sprintf(tmpStr, "GT %d ", RT_GT);
			    strcat(argStr, tmpStr);
			    print_format(1, INTG,2,INTG,3,INTG, 0);
			    print_args(1,2,3, 0);
			    timeVnum = 1;
			    timeVals[0] = 1;
			    break;
	     case 37:    /* ifrtGE */
		     	    print_code(IFRT, token);
			    strcat(argStr, " 0 4 0 ");
                            sprintf(tmpStr, "GE %d ", RT_GE);
			    strcat(argStr, tmpStr);
			    print_format(1, INTG,2,INTG,3,INTG, 0);
			    print_args(1,2,3, 0);
			    timeVnum = 1;
			    timeVals[0] = 1;
			    break;
	     case 38:    /* ifrtLT */
		     	    print_code(IFRT, token);
			    strcat(argStr, " 0 4 0 ");
                            sprintf(tmpStr, "LT %d ", RT_LT);
			    strcat(argStr, tmpStr);
			    print_format(1, INTG,2,INTG,3,INTG, 0);
			    print_args(1,2,3, 0);
			    timeVnum = 1;
			    timeVals[0] = 1;
			    break;
	     case 39:    /* ifrtLE */
		     	    print_code(IFRT, token);
			    strcat(argStr, " 0 4 0 ");
                            sprintf(tmpStr, "LE %d ", RT_LE);
			    strcat(argStr, tmpStr);
			    print_format(1, INTG,2,INTG,3,INTG, 0);
			    print_args(1,2,3, 0);
			    timeVnum = 1;
			    timeVals[0] = 1;
			    break;
	     case 40:    /* ifrtEQ */
		     	    print_code(IFRT, token);
			    strcat(argStr, " 0 4 0 ");
                            sprintf(tmpStr, "EQ %d ", RT_EQ);
			    strcat(argStr, tmpStr);
			    print_format(1, INTG,2,INTG,3,INTG, 0);
			    print_args(1,2,3, 0);
			    timeVnum = 1;
			    timeVals[0] = 1;
			    break;
	     case 41:    /* sample */
			    print_code(SAMPLE, token);
                            timeIndex = 1;
                            strcat(argStr, " 1 0 1 ");
                            print_format(1, FLOAT, 0);
                            print_fargs(1, 0);
                            timeFnum = 1;
                            timeFvals[0] = 1;
			    break;
	     case 42:    /* endacq */
	      		    print_dummy_code(token);
			    break;
	     case 43:    /* rlloop */
                            print_code(RLLOOP, token);
                            strcat(argStr, " 1 3 0 ");
                            print_format(1,INTG, 2,INTG, 3, INTG, 0);
                            print_args(1,2,3,0);
                            rtCode = 3;
                            timeVnum = 3;
		            timeVals[0] = 1;
		            timeVals[1] = 2;
		            timeVals[2] = 3;
			    break;
	     case 44:    /* kzloop */
                            print_code(KZLOOP, token);
                            strcat(argStr, " 1 2 1 ");
                            print_format(2,INTG, 3, INTG, -1);
			    print_format(1, FLOAT, 0);
                            print_args(2,3,-1);
			    print_fargs(1, 0);
                            timeFnum = 1;
		            timeFvals[0] = 1;
			    break;
	     case 45:    /* nwloop */
                            print_code(NWLOOP, token);
                            strcat(argStr, " 1 2 1 ");
                            print_format(2,INTG, 3, INTG, -1);
			    print_format(1, FLOAT, 0);
                            print_args(2,3,-1);
			    print_fargs(1, 0);
                            timeFnum = 1;
		            timeFvals[0] = 1;
                            timeVnum = 2;
		            timeVals[0] = 2;
		            timeVals[1] = 3;
			    break;
	     default:
	      		    print_dummy_code(token);
			    break;
        }
	print_statement();
	return(1);
}


int
compare_7(char *token, FILE *fin)
{
	int	loop;

	for (loop = 0; loop < cmd7Num; loop++)
	{
	    if (strcmp(token, cmd_7[loop]) == 0)
	    {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* hsdelay */
		     	    print_code(DELAY, token);
			    timeIndex = 1;
			    strcat(argStr, " 1 0 1 ");
			    print_format(1, FLOAT, 0);
			    print_fargs(1, 0);
			    timeFnum = 1;
			    timeFvals[0] = 1;
			    break;
		case  1:     /* rgpulse */
			    timeIndex = 1;
		     	    print_code(PULSE, token);
			    strcat(argStr, " 1 0 1 1 ");
			    print_format(3, FLOAT, 4,FLOAT, -1);
			    cat_arg(argStr," %d ", TODEV);
			    print_format(2, INTG, 1, FLOAT, 0);
			    print_fargs(3, 4, 0);
			    print_args(2, 0);
			    print_fargs(1, 0);
			    timeFnum = 3;
			    timeFvals[0] = 1;
			    timeFvals[1] = 3;
			    timeFvals[2] = 4;
		  	    break;
		case  2:     /* endloop */
		     	    print_code(ENDSP, token);
			    strcat(argStr, " 1 1 0 ");
			    print_format(1,INTG, 0);
			    print_args(1, 0);
			    timeVnum = 1;
			    timeVals[0] = 1;
		  	    break;
		case  3:     /* txphase */
		     	    print_code(PHASE, token);
			    cat_arg(argStr, " %d 1 0 ", TODEV);
			    print_format(1,INTG, 0);
			    print_args(1, 0);
		  	    break;
		case  4:     /* dcphase */
	      		    print_dummy_code(token);
		  	    break;
		case  5:     /* acquire */
			    timeIndex = 2;
		     	    print_code(ACQUIRE, token);
			    strcat(argStr, " 1 0 2 ");
			    print_format(1, FLOAT, 2, FLOAT, 0);
			    print_fargs(1, 2, 0);
			    timeFnum = 2;
			    timeFvals[0] = 1;
			    timeFvals[1] = 2;
		  	    break;
		case  6:     /* rlpower */
		     	    print_code(RLPWR, token);
			    print_format(INTG, -1);
			    strcat(argStr, " 0 1 ");
			    print_format(1, FLOAT, 0);
			    print_args(2, 0);
			    print_fargs(1, 0);
		  	    break;
		case  7:     /* xmtroff */
		     	    print_code(DEVON, token);
			    cat_arg(argStr, " %d 1 0 off 0 \\n\"", TODEV);
			    break;
		case  8:     /* dec2off */
		     	    print_code(DEVON, token);
			    cat_arg(argStr, " %d 1 0 off 0 \\n\"", DO2DEV);
			    break;
		case  9:     /* dec3off */
		     	    print_code(DEVON, token);
			    cat_arg(argStr, " %d 1 0 0ff 0 \\n\"", DO3DEV);
			    break;
		case  10:     /* initval */
		     	    print_code(SETVAL, token);
			    strcat(argStr, " 0 1 1 ");
			    print_format(2, INTG, 1, FLOAT, 0);
			    print_args(2, 0);
			    print_fargs(1, 0);
			    timeFnum = 1;
			    timeFvals[0] = 1;
			    timeVnum = 1;
			    timeVals[0] = 2;
			    break;
		case  11:     /* blankon */
		     	    print_code(AMPBLK, token);
			    print_format(INTG, -1);
			    strcat(argStr, " 1 0 on 0 \\n\"");
			    print_args(1, 0);
		  	    break;
		case  12:     /* ioffset */
		     	    print_code(OFFSET, token);
			    print_format(INTG, -1);
			    strcat(argStr, " 0 1 ");
			    print_format(1, FLOAT, 0);
			    print_args(2, 0);
			    print_fargs(1, 0);
			    break;
		case  13:     /* rcvroff */
		     	    print_code(RCVRON, token);
			    strcat(argStr, " 1 2 0 off 0 1 1 \\n\"");
			    break;
		case  14:     /* lk_hold */
		     	    print_code(LKHLD, token);
			    strcat(argStr, " 1 0 0 ");
			    print_format(0);
			    break;
		case  15:     /* getelem */
		     	    print_code(GETELEM, token);
			    strcat(argStr, " 0 3 0 ");
			    print_format(1, INTG, 2, INTG, 3, INTG, 0);
			    print_args(1, 2, 3, 0);
			    timeVnum = 3;
			    timeVals[0] = 1;
			    timeVals[1] = 2;
			    timeVals[2] = 3;
			    break;
		case  16:     /* obspwrf */
		     	    print_code(RLPWRF, token);
			    cat_arg(argStr, " %d 0 1 ", TODEV);
			    print_format(1, FLOAT, 0);
			    print_fargs(1, 0);
			    break;
		case  17:     /* decpwrf */
		     	    print_code(RLPWRF, token);
			    cat_arg(argStr, " %d 0 1 ", DODEV);
			    print_format(1, FLOAT, 0);
			    print_fargs(1, 0);
			    break;
		case  18:     /* G_Delay */
			    if (!G_delay())
	      		        print_dummy_code(token);
			    break;
		case  19:     /* G_Pulse */
			    if (!G_pulse())
	      		        print_dummy_code(token);
			    break;
		case  20:     /* G_Power */
			    if (!G_power())
	      		        print_dummy_code(token);
			    break;
		case  21:     /* voffset */
		     	    print_code(VOFFSET, token);
			    strcat(argStr, " 1 2 0 ");
			    print_format(1, INTG, 2, INTG, 0);
			    print_args(1, 2, 0);
			    break;
		case  22:     /* poffset */
		     	    print_code(POFFSET, token);
			    cat_arg(argStr, " %d 0 3 ", TODEV);
			    print_format(1, FLOAT, 2, FLOAT, -1);
			    strcat(argStr, " resto %.9f\\n\"");
			    print_fargs(1, 2, 0);
			    strcat(argStr, ", resto");
			    break;
		case  26:     /* dec4off */
		     	    print_code(DEVON, token);
			    cat_arg(argStr, " %d 1 0 0ff 0 \\n\"", DO4DEV);
			    break;
		case  27:     /* dec5off */
		     	    print_code(DEVON, token);
			    cat_arg(argStr, " %d 1 0 0ff 0 \\n\"", DO5DEV);
			    break;
		case  28:     /* putcode */
	      		    print_dummy_code(token);
		  	    break;
		case  29:     /* ifrtNEQ */
		     	    print_code(IFRT, token);
			    strcat(argStr, " 0 4 0 ");
                            sprintf(tmpStr, "NEQ %d ", RT_NE);
			    strcat(argStr, tmpStr);
			    print_format(1, INTG,2,INTG,3,INTG, 0);
			    print_args(1,2,3, 0);
			    timeVnum = 1;
			    timeVals[0] = 1;
		  	    break;
		case  30:     /* peloop2 */
		     	    print_code(PELOOP2, token);
			    strcat(argStr, " 1 3 1 ");
			    print_format(CHAR,INTG,3,INTG,4,INTG, 2,FLOAT, 0);
			    print_str(1, 0);
			    print_args(3, 3, 4, 0);
			    print_fargs(2, 0);
			    timeVnum = 2;
			    timeVals[0] = 3;
			    timeVals[1] = 4;
			    timeFnum = 1;
			    timeFvals[0] = 2;
			    rtCode = 301;
		  	    break;
		default:
	      		    print_dummy_code(token);
			    break;
		}
		print_statement();
		return(1);
	    }
	}
	return (0);
}


int
compare_8(char *token, FILE *fin)
{
	int	loop;

	for (loop = 0; loop < cmd8Num; loop++)
	{
	    if (strcmp(token, cmd_8[loop]) == 0)
	    {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* obspulse */
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 rof1 %.9f rof2 %.9f ");
			 cat_arg(argStr, " %d oph %%d pw %%.9f \\n\"", TODEV);
			 strcat(argStr, ",rof1, rof2, oph, pw");
			 timeFnum = 3;
			 timeFvals[0] = 92;   /*  pw */
			 timeFvals[1] = 90;
			 timeFvals[2] = 91;
			 break;
		case  1:     /* decpulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 cat_arg(argStr, " 0 0.0 0 0.0 %d ", DODEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 0;
			 timeFvals[2] = 0;
			 break;
		case  2:     /* irgpulse */
		     	 print_code(PULSE, token);
			 timeIndex = 1;
		         strcat(argStr, " 1 0 1 1 ");
			 print_format(3, FLOAT, 4,FLOAT, -1);
			 cat_arg(argStr, " %d ", TODEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_fargs(3, 4, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 3;
			 timeFvals[2] = 4;
		  	 break;
		case  3:     /* simpulse */
			 timeIndex = 1;
		     	 print_code(SMPUL, token);
		         strcat(argStr, " 2 0 1 1 ");
			 print_format(5,FLOAT,6,FLOAT, -1);
			 print_format(INTG, 3, INTG, 1, FLOAT, -1);
			 print_format(INTG, 4, INTG, 2,FLOAT, 0);
			 print_fargs(5, 6, 0);
			 strcat(argStr, ", 1");
			 print_args(3, 0);
			 print_fargs(1, 0);
			 strcat(argStr, ", 2");
			 print_args(4, 0);
			 print_fargs(2, 0);
			 timeFnum = 6;
			 timeFvals[0] = 1;
			 timeFvals[1] = 2;
			 timeFvals[2] = 0;
			 timeFvals[3] = 0;
			 timeFvals[4] = 5;
			 timeFvals[5] = 6;
		  	 break;
		case  4:     /* decphase */
		     	 print_code(PHASE, token);
			 cat_arg(argStr, " %d 1 0 ", DODEV);
			 print_format(1,INTG, 0);
			 print_args(1, 0);
		  	 break;
		case  5:     /* declvlon */
		     	 print_code(DECLVL, token);
			 cat_arg(argStr, " %d  1 0 on 1 \\n\"", DODEV);
		  	 break;
		case  6:     /* stepsize */
		     	 print_code(PHSTEP, token);
			 print_format(INTG, -1);
			 strcat(argStr, " 0 1 ");
			 print_format(1, FLOAT, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
		  	 break;
		case  7:     /* spinlock */
		     	 print_code(SPINLK, token);
			 strcat(argStr, " 1 3 2 ");
			 print_format(STRG,INTG,4,INTG,5,INTG,2,FLOAT,3,FLOAT, 0);
			 print_str(1, 0);
			 print_args(4,4, 5, 0);
			 print_fargs(2, 3, 0);
			 timeFnum = 2;
			 timeFvals[0] = 2;
			 timeFvals[1] = 3;
			 timeVnum = 1;
			 timeVals[0] = 5;
		  	 break;
		case  8:     /* obsprgon */
		     	 print_code(PRGON, token);
			 cat_arg(argStr, " %d 0 3 ", TODEV);
			 print_format(STRG,FLOAT,2,FLOAT,3,FLOAT, 0);
			 print_str(1, 0);
			 print_fargs(2, 2, 3, 0);
		  	 break;
		case  9:     /* decprgon */
		     	 print_code(PRGON, token);
			 cat_arg(argStr, " %d 0 3 ", DODEV);
			 print_format(STRG,FLOAT, 2,FLOAT,3,FLOAT, 0);
			 print_str(1, 0);
			 print_fargs(2, 2, 3, 0);
		  	 break;
		case  10:     /* incdelay */
		     	 print_code(INCDLY, token);
			 strcat(argStr, " 1 2 0 ");
			 print_format(1,INTG, 2, INTG, 0);
			 print_args(1, 2, 0);
			 timeVnum = 2;
			 timeVals[0] = 1;
			 timeVals[1] = 2;
		  	 break;
		case  11:     /* gradient */
		     	 print_code(GRAD, token);
			 strcat(argStr, " 11 3 0 ");
			 print_format(CHAR, 2, INTG, -1);
			 strcat(argStr, " 0 0 0 0 \\n\"");
			 print_str(1, 0);
			 print_args(2, 0);
		  	 break;
		case  12:     /* blankoff */
		     	 print_code(AMPBLK, token);
			 print_format(INTG, -1);
			 strcat(argStr, " 1 0 off 1 \\n\"");
			 print_args(1, 0);
		  	 break;
		case  13:     /* genpulse */
		     	 print_code(PULSE, token);
			 timeIndex = 1;
			 strcat(argStr, " 1 0 1 1 ");
			 strcat(argStr, " rof1 %.9f rof2 %.9f ");
			 print_format(INTG, 2, INTG, 1, FLOAT, 0);
			 strcat(argStr, ", rof1, rof2");
			 print_args(3, 2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 90;
			 timeFvals[2] = 91;
			 break;
		case  14:     /* obspower */
		     	 print_code(RLPWR, token);
			 cat_arg(argStr, " %d 0 1 ", TODEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  15:     /* decpower */
		     	 print_code(RLPWR, token);
			 cat_arg(argStr, " %d 0 1 ", DODEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  16:     /* dec2pwrf */
		   	 print_code(RLPWRF, token);
			 cat_arg(argStr, " %d 0 1 ", DO2DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  17:     /* dec3pwrf */
		     	 print_code(RLPWRF, token);
			 cat_arg(argStr, " %d 0 1 ", DO3DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  18:     /* decblank */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 on 0 \\n\"", DODEV);
			 break;
		case  19:     /* obsblank */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 on 0 \\n\"", TODEV);
			 break;
		case  20:     /* G_Offset */
			 if (!G_offset())
	      		      print_dummy_code(token);
			 break;
		case  21:     /* dps_show */
			 if (!translate_show())
	      		      print_dummy_code(token);
			 timeCode = DUMMY;
			 break;
		case  23:     /* dec4pwrf */
		     	 print_code(RLPWRF, token);
			 cat_arg(argStr, " %d 0 1 ", DO4DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  24:     /* dec5pwrf */
		     	 print_code(RLPWRF, token);
			 cat_arg(argStr, " %d 0 1 ", DO5DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  25:     /* lcsample */
			 fprintf(fout, "dps_lcsample(\"lcsample\", 0)");
			 return (1);
			 break;
		case  26:     /* set4Tune */
			 timeIndex = 93;
			 if (timeMode)
		     	    print_code(DELAY, token);
			 else
		     	    print_code(XTUNE, token);
			 print_format(INTG, -1);
			 strcat(argStr, " 0 1 ");
			 print_format(2,FLOAT, 0);
			 print_args(1, 0);
			 print_fargs(2, 0);
			 timeFnum = 1;
			 timeFvals[0] = 93;  /* atStr */
			 break;
		case  27:     /* startacq */
		     	 print_code(ACQ1, token);
			 strcat(argStr, " 1 0 1 ");
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  28:     /* vobspwrf */
                         print_code(VRLPWRF, token);
                         cat_arg(argStr, " %d 1 0 ", TODEV);
                         print_format(1, INTG, 0);
                         print_args(1, 0);
			 break;
		case  29:     /* vdecpwrf */
		     	 print_code(VRLPWRF, token);
			 cat_arg(argStr, " %d 1 0 ", DODEV);
			 print_format(1, INTG, 0);
			 print_args(1, 0);
			 break;
		 default:
	      		 print_dummy_code(token);
			 break;
		}
		print_statement();
		return (1);
	    }
	 }
	 return (0);
}


int
compare_9(char *token, FILE *fin)
{
	int	loop;

	for (loop = 0; loop < cmd9Num; loop++)
	{
	    if (strcmp(token, cmd_9[loop]) == 0)
	    {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* iobspulse */
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 cat_arg(argStr, "rof1 %%.9f rof2 %%.9f %d ", TODEV);
			 strcat(argStr, "oph %d pw %.9f \\n\", rof1, rof2, oph, pw");
			 timeFnum = 3;
			 timeFvals[0] = 92;
			 timeFvals[1] = 90;
			 timeFvals[2] = 91;
			 break;
		case  1:     /* idecpulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 cat_arg(argStr, " 0 0.0 0 0.0 %d ", DODEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 0;
			 timeFvals[2] = 0;
			 break;
		case  2:     /* dec2pulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 cat_arg(argStr, " 0 0.0 0 0.0 %d ", DO2DEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 0;
			 timeFvals[2] = 0;
			 break;
		case  3:     /* dec3pulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 cat_arg(argStr, " 0 0.0 0 0.0 %d ", DO3DEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 0;
			 timeFvals[2] = 0;
			 break;
		case  4:     /* sim3pulse */
			 timeIndex = 1;
		     	 print_code(SMPUL, token);
		         strcat(argStr, " 3 0 1 1 ");
			 print_format(7,FLOAT,8,FLOAT, -1);
		         cat_arg(argStr, " %d ", TODEV);
			 print_format(4, INTG, 1,FLOAT, -1);
		         cat_arg(argStr, " %d ", DODEV);
			 print_format(5, INTG, 2,FLOAT, -1);
		         cat_arg(argStr, " %d ", DO2DEV);
			 print_format(6, INTG, 3,FLOAT, 0);
			 print_fargs(7, 8, 0);
			 print_args(4, 0);
			 print_fargs(1, 0);
			 print_args(5, 0);
			 print_fargs(2, 0);
			 print_args(6, 0);
			 print_fargs(3, 0);
			 timeFnum = 6;
			 timeFvals[0] = 1;
			 timeFvals[1] = 2;
			 timeFvals[2] = 3;
			 timeFvals[3] = 0;
			 timeFvals[4] = 7;
			 timeFvals[5] = 8;
		  	 break;
		case  5:     /* sim4pulse */
			 timeIndex = 1;
		     	 print_code(SMPUL, token);
		         strcat(argStr, " 4 0 1 1 ");
			 print_format(9,FLOAT,10,FLOAT, -1);
		         cat_arg(argStr, " %d ", TODEV);
			 print_format(5, INTG, 1,FLOAT, -1);
		         cat_arg(argStr, " %d ", DODEV);
			 print_format(6, INTG, 2,FLOAT, -1);
		         cat_arg(argStr, " %d ", DO2DEV);
			 print_format(7, INTG, 3,FLOAT, -1);
		         cat_arg(argStr, " %d ", DO3DEV);
			 print_format(8, INTG, 4,FLOAT, 0);
			 print_fargs(9, 10, 0);
			 print_args(5, 0);
			 print_fargs(1, 0);
			 print_args(6, 0);
			 print_fargs(2, 0);
			 print_args(7, 0);
			 print_fargs(3, 0);
			 print_args(8, 0);
			 print_fargs(4, 0);
			 timeFnum = 6;
			 timeFvals[0] = 1;
			 timeFvals[1] = 2;
			 timeFvals[2] = 3;
			 timeFvals[3] = 4;
			 timeFvals[4] = 9;
			 timeFvals[5] = 10;
		  	 break;
		case  6:     /* decoffset */
		     	 print_code(OFFSET, token);
			 cat_arg(argStr, " %d 0 1 ", DODEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  7:     /* obsoffset */
		     	 print_code(OFFSET, token);
			 cat_arg(argStr, " %d 0 1 ", TODEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  8:     /* dec2prgon */
		     	 print_code(PRGON, token);
			 cat_arg(argStr, " %d 0 3 ", DO2DEV);
			 print_format(STRG,FLOAT,2,FLOAT,3,FLOAT, 0);
			 print_str(1, 0);
			 print_fargs(2, 2, 3, 0);
		  	 break;
		case  9:     /* dec3prgon */
		     	 print_code(PRGON, token);
			 cat_arg(argStr, " %d 0 3 ", DO3DEV);
			 print_format(STRG,FLOAT,2,FLOAT,3,FLOAT, 0);
			 print_str(1, 0);
			 print_fargs(2, 2, 3, 0);
		  	 break;
		case  10:     /* xmtrphase */
		     	 print_code(SPHASE, token);
			 cat_arg(argStr, " %d 1 0 ", TODEV);
			 print_format(1, INTG, 0);
			 print_args(1, 0);
		  	 break;
		case  11:     /* dec2phase */
		     	 print_code(PHASE, token);
			 cat_arg(argStr, " %d 1 0 ", DO2DEV);
			 print_format(1, INTG, 0);
			 print_args(1, 0);
		  	 break;
		case  12:     /* dec3phase */
		     	 print_code(PHASE, token);
			 cat_arg(argStr, " %d 1 0 ", DO3DEV);
			 print_format(1, INTG, 0);
			 print_args(1, 0);
		  	 break;
		case  13:     /* declvloff */
		     	 print_code(DECLVL, token);
			 cat_arg(argStr, " %d 1 0 off 0 \\n\"", DODEV);
		  	 break;
		case  14:     /* obsprgoff */
		     	 print_code(PRGOFF, token);
			 cat_arg(argStr, " %d 1 0 off 0 \\n\"", TODEV);
		  	 break;
		case  15:     /* decprgoff */
		     	 print_code(PRGOFF, token);
			 cat_arg(argStr, " %d 1 0 off 0 \\n\"", DODEV);
		  	 break;
		case  16:     /* apovrride */
		     	 print_code(APOVR, token);
			 strcat(argStr, " 0 0 0 \\n\"");
		  	 break;
		case  17:     /* rotorsync */
		     	 print_code(ROTORS, token);
			 strcat(argStr, " 1 1 0 ");
			 print_format(1, INTG, 0);
			 print_args(1, 0);
			 timeVnum = 1;
			 timeVals[0] = 1;
		  	 break;
		case  18:     /* setstatus */
		     	 print_code(SETST, token);
			 print_format(INTG, -1);
			 strcat(argStr, " 3 1 ");
			 print_format(CHAR,INTG, 2,INTG, 4, INTG, 5, FLOAT, 0);
			 print_str(1, 3, 0);
			 print_args(4, 2, 4, 0);
			 print_fargs(5, 0);
		  	 break;
		case  19:     /* initdelay */
		     	 print_code(INITDLY, token);
			 strcat(argStr, " 0 1 1 ");
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 1;
			 timeFvals[0] = 1;
			 timeVnum = 1;
			 timeVals[0] = 2;
		  	 break;
		case  20:     /* lk_sample */
		     	 print_code(LKSMP, token);
			 strcat(argStr, " 1 0 0 ");
			 print_format(0);
		  	 break;
		case  21:     /* dec2power */
		     	 print_code(RLPWR, token);
			 cat_arg(argStr, " %d 0 1 ", DO2DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  22:     /* dec3power */
		     	 print_code(RLPWR, token);
			 cat_arg(argStr, " %d 0 1 ", DO3DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  23:     /* dec2blank */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 off 0 \\n\"", DO2DEV);
			 break;
		case  24:     /* dec3blank */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 off 0 \\n\"", DO3DEV);
			 break;
		case  25:     /* G_RTDelay */
			 if (!G_rtdelay())
	      		    print_dummy_code(token);
			 break;
		case  26:     /* endpeloop */
		case  27:     /* endmsloop */
		     	 print_code(ENDSISLP, token);
			 strcat(argStr, " 1 2 0 ");
			 print_format(CHAR,INTG, 2, INTG, 0);
			 print_str(1, 0);
			 print_args(2, 2, 0);
			 timeVnum = 1;
			 rtCode = 301;
			 timeVals[0] = 2;
			 break;
		case  28:     /* vgradient */
		     	 print_code(VGRAD, token);
			 strcat(argStr, " 11 3 0 ");
			 print_format(CHAR, 2, INTG, 3, INTG, 4, INTG, 0);
			 print_str(1, 0);
			 print_args(2, 3, 4, 0);
		  	 break;
		case  29:     /* rgradient */
		case  31:     /* pgradient */
		case  33:     /* lgradient */
		     	 print_code(RGRAD, token);
			 strcat(argStr, " 11 0 4 ");
			 print_format(CHAR, 2, FLOAT, -1);
			 strcat(argStr, " 0.00 0.0 0.00 0.0 gradalt %.9f\\n\"");
			 print_str(1, 0);
			 print_fargs(2, 0);
		         strcat(argStr, ", (float)gradalt");
		  	 break;
		case  30:     /* Sgradient */
		case  32:     /* Wgradient */
		     	 print_code(GRAD, token);
			 strcat(argStr, " 11 3 0 ");
			 print_format(CHAR, 2, INTG, -1);
			 strcat(argStr, " 0 0 0 0 \\n\"");
			 print_str(1, 0);
			 print_args(2, 0);
		  	 break;
		case  34:     /* loadtable */
			 if (loadtable_code())
			     return (1);
	      		 print_dummy_code(token);
		  	 break;
		case  35:     /* CSet2Mark */
		  	 print_code(CBACK, token);
			 strcat(argStr, " 0 0 0 0 0 \\n\"");
		  	 break;
		case  36:     /* CGradient */
		     	 print_code(CGRAD, token);
			 strcat(argStr, " 11 0 3 ");
			 print_format(CHAR, 2, FLOAT, -1);
			 strcat(argStr, " 0.00 0.0 0.00 0.0 \\n\"");
			 print_str(1, 0);
			 print_fargs(2, 0);
		  	 break;
		case  37:    /* dec4pulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 cat_arg(argStr, " 0 0.0 0 0.0 %d ", DO4DEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 0;
			 timeFvals[2] = 0;
			 break;
		case  38:     /* dec4prgon */
		     	 print_code(PRGON, token);
			 cat_arg(argStr, " %d 0 3 ", DO4DEV);
			 print_format(STRG,FLOAT,2,FLOAT,3,FLOAT, 0);
			 print_str(1, 0);
			 print_fargs(2, 2, 3, 0);
		  	 break;
		case  39:     /* dec4phase */
		     	 print_code(PHASE, token);
			 cat_arg(argStr, " %d 1 0 ", DO4DEV);
			 print_format(1, INTG, 0);
			 print_args(1, 0);
		  	 break;
		case  40:     /* dec4power */
		     	 print_code(RLPWR, token);
			 cat_arg(argStr, " %d 0 1 ", DO4DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  41:     /* dec4blank */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 off 0 \\n\"", DO4DEV);
			 break;
		case  42:     /* dec5pulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 cat_arg(argStr, " 0 0.0 0 0.0 %d ", DO5DEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 0;
			 timeFvals[2] = 0;
			 break;
		case  43:     /* dec5prgon */
		     	 print_code(PRGON, token);
			 cat_arg(argStr, " %d 0 3 ", DO5DEV);
			 print_format(STRG,FLOAT,2,FLOAT,3,FLOAT, 0);
			 print_str(1, 0);
			 print_fargs(2, 2, 3, 0);
		  	 break;
		case  44:     /* dec5phase */
		     	 print_code(PHASE, token);
			 cat_arg(argStr, " %d 1 0 ", DO5DEV);
			 print_format(1, INTG, 0);
			 print_args(1, 0);
		  	 break;
		case  45:     /* dec5power */
		     	 print_code(RLPWR, token);
			 cat_arg(argStr, " %d 0 1 ", DO5DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  46:     /* dec5blank */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 off 0 \\n\"", DO5DEV);
			 break;
		case  47:     /* shapelist */
                        /*********
			 castFlag[1] = FLOAT;
			 castFlag[2] = FLOATARRAY;
			 castFlag[3] = FLOAT;
			 castFlag[4] = FLOAT;
			 castFlag[5] = CHAR;
			 change_code(token, token, SHLIST, 6);
                        ********/
                         gen_shapelist_init(token, TODEV);
			 return (1);
		case  48:     /* vdec2pwrf */
		   	 print_code(VRLPWRF, token);
			 cat_arg(argStr, " %d 1 0 ", DO2DEV);
			 print_format(1, INTG, 0);
			 print_args(1, 0);
			 break;
		case  49:     /* vdec3pwrf */
		   	 print_code(VRLPWRF, token);
			 cat_arg(argStr, " %d 1 0 ", DO3DEV);
			 print_format(1, INTG, 0);
			 print_args(1, 0);
			 break;
		case  50:     /* F_initval */
                         print_code(SETVAL, token);
                         strcat(argStr, " 0 1 1 ");
                         print_format(2, INTG, 1, FLOAT, 0);
                         print_args(2, 0);
                         print_fargs(1, 0);
                         timeFnum = 1;
                         timeFvals[0] = 1;
                         timeVnum = 1;
                         timeVals[0] = 2;
                         break;
		case  51:     /* rlendloop */
                         print_code(RLLOOPEND, token);
                         strcat(argStr, " 1 1 0 ");
                         print_format(1,INTG, 0);
                         print_args(1, 0);
                         timeVnum = 1;
                         timeVals[0] = 1;
                         break;
		case  52:     /* kzendloop */
                         print_code(KZLOOPEND, token);
                         strcat(argStr, " 1 1 0 ");
                         print_format(1,INTG, 0);
                         print_args(1, 0);
                         timeVnum = 1;
                         timeVals[0] = 1;
                         break;
		case  53:     /* endnwloop */
                         print_code(NWLOOPEND, token);
                         strcat(argStr, " 1 1 0 ");
                         print_format(1,INTG, 0);
                         print_args(1, 0);
                         timeVnum = 1;
                         timeVals[0] = 1;
                         break;
		default:
	      		 print_dummy_code(token);
			 break;
		}
		print_statement();
		return (1);
	    }
	 }
	 return (0);
}


int
compare_10(char *token, FILE *fin)
{
	int	loop;

	for (loop = 0; loop < cmd10Num; loop++)
	{
	    if (strcmp(token, cmd_10[loop]) == 0)
	    {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* decrgpulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 print_format(3, FLOAT, 4,FLOAT, -1);
			 cat_arg(argStr, " %d ", DODEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_fargs(3, 4, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 3;
			 timeFvals[2] = 4;
			 break;
		case  1:     /* idec2pulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 cat_arg(argStr, " 0 0.0 0 0.0 %d ", DO2DEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 0;
			 timeFvals[2] = 0;
			 break;
		case  2:     /* genrgpulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 cat_arg(argStr, " 1 0 1 1 ");
			 print_format(3,FLOAT,4,FLOAT, -1);
			 print_format(INTG, 2, INTG, 1,FLOAT, 0);
			 print_fargs(3, 4, 0);
			 print_args(5, 2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 3;
			 timeFvals[2] = 4;
		  	 break;
		case  3:     /* gensaphase */
		     	 print_code(SPHASE, token);
			 print_format(INTG, -1);
			 strcat(argStr, " 1 0 ");
			 print_format(1, INTG, 0);
			 print_args(2, 1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 2;
			 timeFvals[1] = 4;
			 timeFvals[2] = 5;
		  	 break;
		case  4:     /* G_Simpulse */
			 if (!G_simpulse())
	      		     print_dummy_code(token);
		  	 break;
		case  5:     /* zgradpulse */
			 timeIndex = 2;
		     	 print_code(ZGRAD, token);
			 strcat(argStr, " 11 0 3 z ");
			 print_format(2, FLOAT, 1, FLOAT, -1);
			 strcat(argStr, " gradalt ");
			 print_format(FLOAT, 0);
			 print_fargs(2, 1, 0);
			 strcat(argStr, ", (float)gradalt");
			 timeFnum = 1;
			 timeFvals[0] = 2;
			 break;
		case  6:     /* phaseshift */
		     	 print_code(PSHIFT, token);
			 strcat(argStr, " %d 1 1 ");
			 print_format(2,INTG,  1, FLOAT, 0);
			 print_args(3, 2, 0);
			 print_fargs(1, 0);
			 break;
		case  7:     /* dcplrphase */
		     	 print_code(SPHASE, token);
			 cat_arg(argStr, " %d 1 0 ", DODEV);
			 print_format(1,INTG, 0);
			 print_args(1, 0);
			 break;
		case  8:     /* prg_dec_on */
		     	 print_code(PRGON, token);
			 strcat(argStr, " %d 0 3 ");
			 print_format(STRG,FLOAT,2,FLOAT,3,FLOAT, 0);
			 print_args(4, 0);
			 print_str(1, 0);
			 print_fargs(2, 2, 3, 0);
		  	 break;
		case  9:     /* dec2prgoff */
		     	 print_code(PRGOFF, token);
			 cat_arg(argStr, " %d 0 0 0", DO2DEV);
			 print_format(0);
			 break;
		case  10:    /* dec3prgoff */
		     	 print_code(PRGOFF, token);
			 cat_arg(argStr, " %d 0 0 0", DO3DEV);
			 print_format(0);
			 break;
		case  11:     /* decblankon */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 off 0 \\n\"", DODEV);
			 break;
		case  12:     /* obsunblank */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 on 1 \\n\"", TODEV);
			 break;
		case  13:     /* decunblank */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 on 1 \\n\"", DODEV);
			 break;
		case  14:     /* blankingon */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 off 0 \\n\"", TODEV);
			 break;
		case  15:     /*  dec2offset */
		     	 print_code(OFFSET, token);
			 cat_arg(argStr, " %d 0 1 ", DO2DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  16:     /*  dec3offset */
		     	 print_code(OFFSET, token);
			 cat_arg(argStr, " %d 0 1 ", DO3DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  17:     /* init_vscan */
		     	 print_code(INITSCAN, token);
			 strcat(argStr, " 0 1 1 ");
			 print_format(1,INTG, 2,FLOAT, 0);
			 print_args(1, 0);
			 print_fargs(2, 0);
			 break;
		case  18:    /* Svgradient */
		case  19:    /* Wvgradient */
		case  20:    /* Pvgradient */
		     	 print_code(VGRAD, token);
			 strcat(argStr, " 11 3 0 ");
			 print_format(CHAR, 2, INTG, 3, INTG, 4, INTG, 0);
			 print_str(1, 0);
			 print_args(2, 3, 4, 0);
		  	 break;
		case  21:    /* magradient */
			 castFlag[0] = FLOAT;
/**
			 change_code(token, token, MGRAD, 3);
**/
			 change_code("simgradient", token, MGRAD, 1);
			 return (1);
		case  22:    /* vagradient */
			 castFlag[0] = FLOAT;
			 castFlag[1] = FLOAT;
			 castFlag[2] = FLOAT;
			 change_code("magradient",token, MGRAD, 3);
			 return (1);
		case  24:    /* obs_pw_ovr */
	      		 print_dummy_code(token);
		  	 break;
		case  25:    /* CGradPulse */
			timeIndex = 3;
		     	print_code(CGPUL, token);
			strcat(argStr, " 11 0 2 ");
			print_format(STRG, 3, FLOAT, 2, FLOAT, 0);
			print_str(1, 0);
			print_fargs(3, 2, 0);
			timeFnum = 1;
			timeFvals[0] = 3;
			break;
		case  26:    /* dec4prgoff */
		     	 print_code(PRGOFF, token);
			 cat_arg(argStr, " %d 0 0 0", DO4DEV);
			 print_format(0);
			 break;
		case  27:     /*  dec4offset */
		     	 print_code(OFFSET, token);
			 cat_arg(argStr, " %d 0 1 ", DO4DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  28:    /* dec5prgoff */
		     	 print_code(PRGOFF, token);
			 cat_arg(argStr, " %d 0 0 0", DO5DEV);
			 print_format(0);
			 break;
		case  29:     /*  dec5offset */
		     	 print_code(OFFSET, token);
			 cat_arg(argStr, " %d 0 1 ", DO5DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  33:     /*  pbox_pulse */
			 pbox_code("pbox_pulse", token, TODEV, 4);
			 return (1);
			 break;
		case  34:     /*  pbox_decon */
			 pbox_code("pbox_xmtron", token, DODEV, 1);
			 return (1);
			 break;
		case  35:     /*  offsetlist */
	      		 print_dummy_code(token);
			 break;
		case  36:     /*  endpeloop2 */
		     	 print_code(ENDSISLP, token);
			 strcat(argStr, " 1 2 0 ");
			 print_format(CHAR,INTG, 2, INTG, 0);
			 print_str(1, 0);
			 print_args(2, 2, 0);
			 timeVnum = 1;
			 rtCode = 301;
			 timeVals[0] = 2;
			 break;
		case  37:     /*  GroupPulse */
			 castFlag[0] = INTG;
			 castFlag[1] = FLOAT;
			 castFlag[2] = FLOAT;
			 castFlag[3] = INTG;
			 castFlag[4] = INTG;
			 change_code(token, token, GRPPULSE, 5);
			 return (1);
		 default:
	      		 print_dummy_code(token);
			 break;
	        }
		print_statement();
		return (1);
	    }
	}
	return (0);
}


int
compare_11(char *token, FILE *fin)
{
	int	loop;

	for (loop = 0; loop < cmd11Num; loop++)
	{
	    if (strcmp(token, cmd_11[loop]) == 0)
	    {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* shapedpulse */
			 timeIndex = 2;
		     	 print_code(SHPUL, token);
			 strcat(argStr, " 1 1 1 1 ");
			 print_format(4, FLOAT, 5, FLOAT, -1);
			 cat_arg(argStr, " %d ", TODEV);
			 print_format(STRG, 3, INTG, 2, FLOAT, 0);
			 print_fargs(4, 5, 0);
			 print_str(1, 0);
			 print_args(3, 0);
			 print_fargs(2, 0);
			 timeFnum = 3;
			 timeFvals[0] = 2;
			 timeFvals[1] = 4;
			 timeFvals[2] = 5;
			 break;
		case  1:     /* idecrgpulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 print_format(3, FLOAT, 4,FLOAT, -1);
			 cat_arg(argStr, " %d ", DODEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_fargs(3, 4, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 3;
			 timeFvals[2] = 4;
			 break;
		case  2:     /* dec2rgpulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 print_format(3, FLOAT, 4,FLOAT, -1);
			 cat_arg(argStr, " %d ", DO2DEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_fargs(3, 4, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 3;
			 timeFvals[2] = 4;
			 break;
		case  3:     /* dec3rgpulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 print_format(3, FLOAT, 4,FLOAT, -1);
			 cat_arg(argStr, " %d ", DO3DEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_fargs(3, 4, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 3;
			 timeFvals[2] = 4;
			 break;
		case  4:     /* dec2unblank */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 on 1 \\n\"", DO2DEV);
			 break;
		case  5:     /* dec3unblank */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 on 1 \\n\"", DO3DEV);
			 break;
		case  6:     /* blankingoff */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 on 1 \\n\"", TODEV);
			 break;
		case  7:     /* decblankoff */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 on 1 \\n\"", DODEV);
			 break;
		case  8:     /* dcplr2phase */
		     	 print_code(SPHASE, token);
			 cat_arg(argStr, " %d 1 0 ", DO2DEV);
			 print_format(1,INTG, 0);
			 print_args(1, 0);
			 break;
		case  9:     /* dcplr3phase */
		     	 print_code(SPHASE, token);
			 cat_arg(argStr, " %d 1 0 ", DO3DEV);
			 print_format(1,INTG, 0);
			 print_args(1, 0);
			 break;
		case  10:    /* obsstepsize */
		     	 print_code(PHSTEP, token);
			 cat_arg(argStr, " %d 0 1 ", TODEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
		  	 break;
		case  11:    /* decstepsize */
		     	 print_code(PHSTEP, token);
			 cat_arg(argStr, " %d 0 1 ", DODEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
		  	 break;
		case  12:    /* decspinlock */
		     	 print_code(SPINLK, token);
			 cat_arg(argStr, " %d 3 2 ", DODEV);
			 print_format(STRG,INTG,4,INTG,5,INTG,2,FLOAT,3,FLOAT, 0);
			 print_str(1, 0);
			 print_args(4, 4, 5, 0);
			 print_fargs(2, 3, 0);
			 timeFnum = 2;
			 timeFvals[0] = 2;
			 timeFvals[1] = 3;
			 timeVnum = 1;
			 timeVals[0] = 5;
			 break;
		case  13:    /* genspinlock */
		     	 print_code(SPINLK, token);
			 strcat(argStr, " %d 3 2 ");
			 print_format(STRG,INTG,4,INTG,5,INTG,2,FLOAT,3,FLOAT, 0);
			 print_args(6, 0);
			 print_str(1, 0);
			 print_args(4, 4, 5, 0);
			 print_fargs(2, 3, 0);
			 timeFnum = 2;
			 timeFvals[0] = 2;
			 timeFvals[1] = 3;
			 timeVnum = 1;
			 timeVals[0] = 5;
			 break;
		case  14:    /* endhardloop */
		     	 print_code(ENDHP, token);
			 strcat(argStr, " 1 0 0 ");
			 print_format(0);
			 timeVnum = 1;
			 timeVals[0] = 0;
			 break;
		case  15:    /* rotorperiod */
		     	 print_code(ROTORP, token);
			 strcat(argStr, " 1 1 0 ");
			 print_format(1,INTG, 0);
			 print_args(1, 0);
			 timeVnum = 1;
			 timeVals[0] = 1;
			 break;
		case  16:    /* setreceiver */
		     	 print_code(SETRCV, token);
			 strcat(argStr, " 0 1 0 ");
			 print_format(1,INTG, 0);
			 print_args(1, 0);
			 break;
		case  17:    /* prg_dec_off */
		     	 print_code(PRGOFF, token);
			 strcat(argStr, " %d 0 0 0 \\n\"");
			 print_args(2, 0);
		  	 break;
		case  18:    /* S_vgradient */
		     	 print_code(VGRAD, token);
			 strcat(argStr, " 11 3 0 ");
			 print_format(CHAR, 2, INTG, 3, INTG, 4, INTG, 0);
			 print_str(1, 0);
			 print_args(2, 3, 4, 0);
		  	 break;
		case  19:    /* incgradient */
		     	 print_code(INCGRAD, token);
			 strcat(argStr, " 11 7 0 ");
			 print_format(CHAR, 2, INTG, 3, INTG, 4, INTG, 5, INTG, 6, INTG, 7, INTG, 8, INTG, 0);
			 print_str(1, 0);
			 print_args(2, 3, 4, 5, 6, 7, 8, 0);
		  	 break;
		case  20:    /* pe_gradient */
		     	 print_code(PEGRAD, token);
			 strcat(argStr, " 11 1 4 x 0.00 0 ");
			 print_format(1,FLOAT, -1);
			 strcat(argStr, " 0.00 0.0 0.00 0.0 psi %.9f ");
			 cat_arg(argStr, " \\n%d %s 11 1 4 y ", PEGRAD, token);
			 print_format(5,INTG, 2,FLOAT,4,FLOAT, -1);
			 strcat(argStr, " nv %.9f phi %.9f \\n ");
			 cat_arg(argStr, "%d %s 11 1 4 z 0.00 0 ",PEGRAD,token);
			 print_format(3,FLOAT, -1);
			 strcat(argStr, " 0.00 0.0 0.00 0.0 theta %.9f \\n\"");
			 print_fargs(1, 0);
			 strcat(argStr, ", psi");
			 print_args(5, 0);
			 print_fargs(2, 4, 0);
			 strcat(argStr, ", nv, phi");
			 print_fargs(3, 0);
			 strcat(argStr, ", theta");
		  	 break;
		case  21:    /* magradpulse */
			 castFlag[0] = FLOAT;
			 castFlag[1] = FLOAT;
/**
			 change_code(token,token, MGPUL, 4);
**/
			 change_code("simgradpulse",token, MGPUL, 2);
			 return (1);
		case  22:    /* simgradient */
			 castFlag[0] = FLOAT;
			 change_code(token,token, MGRAD, 1);
			 return (1);
		case  23:    /* vagradpulse */
			 castFlag[0] = FLOAT;
			 castFlag[1] = FLOAT;
			 castFlag[2] = FLOAT;
			 castFlag[3] = FLOAT;
			 change_code("magradpulse", token, MGPUL, 4);
			 return (1);
		case  24:    /* vdelay_list */
		     	 print_code(VDLST, token);
			 strcat(argStr, " 1 2 0 ");
			 print_format(2, INTG, 1, INTG, 0);
			 print_args(2, 1, 0);
			 timeVnum = 2;
			 timeVals[0] = 1;
			 timeVals[1] = 2;
			 break;
		case  25:    /* gensimpulse */
/*
		     	 print_code(SMPUL, token);
*/
	      		 print_dummy_code(token);
			 break;
		case  26:    /* statusdelay */
		     	 print_code(STATUS, token);
			 strcat(argStr, " 0 1 0 ");
			 print_format(1, INTG, -1);
			 cat_arg(argStr, " \\n%d %s 1 0 1 ", DELAY, token);
			 print_format(2, FLOAT, 0);
			 print_args(1, 0);
			 print_fargs(2, 0);
			 timeIndex = 2;
			 timeFnum = 1;
			 timeFvals[0] = 2;
			 break;
		case  27:     /* dec4rgpulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 print_format(3, FLOAT, 4,FLOAT, -1);
			 cat_arg(argStr, " %d ", DO4DEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_fargs(3, 4, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 3;
			 timeFvals[2] = 4;
			 break;
		case  28:     /* dec4unblank */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 on 1 \\n\"", DO4DEV);
			 break;
		case  29:     /* dec5rgpulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 print_format(3, FLOAT, 4,FLOAT, -1);
			 cat_arg(argStr, " %d ", DO5DEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_fargs(3, 4, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 3;
			 timeFvals[2] = 4;
			 break;
		case  30:     /* dec5unblank */
		     	 print_code(AMPBLK, token);
			 cat_arg(argStr, " %d 1 0 on 1 \\n\"", DO5DEV);
			 break;
		case  31:     /* hdwshiminit */
		     	 print_code(HDSHIMINIT, token);
			 strcat(argStr, " 1 1 0 2  0 \\n\"");
			 break;
		case  32:     /* pbox_xmtron */
			 pbox_code("pbox_xmtron", token, TODEV, 1);
			 return (1);
			 break;
		case  33:     /* pbox_decoff */
			 pbox_code("pbox_xmtroff", token, DODEV, 0);
			 return (1);
			 break;
		case  34:     /* pbox_dec2on */
			 pbox_code("pbox_xmtron", token, DO2DEV, 1);
			 return (1);
			 break;
		case  35:     /* pbox_dec3on */
			 pbox_code("pbox_xmtron", token, DO3DEV, 1);
			 return (1);
			 break;
		case  36:     /* XmtNAcquire */
			 timeIndex = 93;
			 if (timeMode)
		     	    print_code(DELAY, token);
			 else
		     	    print_code(XMACQ, token);
			 cat_arg(argStr, " %d 1 5 ", TODEV);
			 print_format(2, INTG, 1, FLOAT, 3, FLOAT, -1);
			 strcat(argStr, " at %.9f np %.9f  sw %.9f ");
			 print_format(0);
			 print_args(2, 0);
			 print_fargs(1, 3, 0);
			 strcat(argStr, ", at,np,sw");
			 timeFnum = 1;
			 timeFvals[0] = 93;
			 break;
		case  37:     /* offsetglist */
	      		 print_dummy_code(token);
			 break;
		case  38:     /*  TGroupPulse */
			 castFlag[0] = INTG;
			 castFlag[1] = FLOAT;
			 castFlag[2] = FLOAT;
			 castFlag[3] = INTG;
			 castFlag[4] = INTG;
			 change_code(token, token, GRPPULSE, 5);
			 return (1);
		case  39:     /*  parallelend */
                         print_code(PARALLELEND, token);
                         cat_arg(argStr, " 0 0 0 ");
                         print_format(0); 
			 break;
		case  40:     /*  gen_poffset */
                         print_code(POFFSET, token);
                         print_format(INTG, -1);
                         cat_arg(argStr, " 0 3 ");
                         print_format(1, FLOAT, 2, FLOAT, -1);
                         strcat(argStr, " resto %.9f\\n\"");
			 print_args(3, 0);
                         print_fargs(1, 2, 0);
                         strcat(argStr, ", resto");
			 break;
		case  41:     /*  nowait_loop */
                         print_code(NWLOOP, token);
                         strcat(argStr, " 1 2 1 ");
                         print_format(2,INTG, 3, INTG, -1);
                         print_format(1, FLOAT, 0);
                         print_args(2,3,-1);
                         print_fargs(1, 0);
                         timeFnum = 1;
                         timeFvals[0] = 1;
                         timeVnum = 2;
                         timeVals[0] = 2;
                         timeVals[1] = 3;
			 break;
		 default:
	      		 print_dummy_code(token);
			 break;
	        }
		print_statement();
		return (1);
	    }
	 }
	 return (0);
}

int
compare_12(char *token, FILE *fin)
{
	int	loop;

	for (loop = 0; loop < cmd12Num; loop++)
	{
	    if (strcmp(token, cmd_12[loop]) == 0)
	    {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* shaped_pulse */
			 timeIndex = 2;
		     	 print_code(SHPUL, token);
			 strcat(argStr, " 1 1 1 1 ");
			 print_format(4, FLOAT, 5, FLOAT, -1);
			 cat_arg(argStr, " %d ", TODEV);
			 print_format(STRG, 3, INTG, 2, FLOAT, 0);
			 print_fargs(4, 5, 0);
			 print_str(1, 0);
			 print_args(3, 0);
			 print_fargs(2, 0);
			 timeFnum = 3;
			 timeFvals[0] = 2;
			 timeFvals[1] = 4;
			 timeFvals[2] = 5;
			 break;
		case  1:     /* idec2rgpulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 print_format(3, FLOAT, 4,FLOAT, -1);
			 cat_arg(argStr, " %d ", DO2DEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_fargs(3, 4, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 3;
			 timeFvals[2] = 4;
			 break;
		case  2:     /* idec3rgpulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 print_format(3, FLOAT, 4,FLOAT, -1);
			 cat_arg(argStr, " %d ", DO3DEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_fargs(3, 4, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 3;
			 timeFvals[2] = 4;
			 break;
		case  3:     /* dec2spinlock */
		     	 print_code(SPINLK, token);
			 cat_arg(argStr, " %d 3 2 ", DO2DEV);
			 print_format(STRG,INTG,4,INTG,5,INTG,2,FLOAT,3,FLOAT,0);
			 print_str(1, 0);
			 print_args(4, 4, 5, 0);
			 print_fargs(2, 3, 0);
			 timeFnum = 2;
			 timeFvals[0] = 2;
			 timeFvals[1] = 3;
			 timeVnum = 1;
			 timeVals[0] = 5;
			 break;
		case  4:     /* dec3spinlock */
		     	 print_code(SPINLK, token);
			 cat_arg(argStr, " %d 3 2 ", DO3DEV);
			 print_format(STRG,INTG,4,INTG,5,INTG,2,FLOAT,3,FLOAT,0);
			 print_str(1, 0);
			 print_args(4, 4, 5, 0);
			 print_fargs(2, 3, 0);
			 timeFnum = 2;
			 timeFvals[0] = 2;
			 timeFvals[1] = 3;
			 timeVnum = 1;
			 timeVals[0] = 5;
			 break;
		case  5:     /* gen2spinlock */
		     	 print_code(SPINLK, token);
			 strcat(argStr, " %d 3 2 ");
			 print_format(STRG,INTG,7,INTG,9,INTG,3,FLOAT,5,FLOAT,-1);
			 cat_arg(argStr, " \\n%d %s ", SPINLK, token);
			 strcat(argStr, " %d 3 2 ");
			 print_format(STRG,INTG,8,INTG,9,INTG,4,FLOAT,6,FLOAT,0);
			 print_args(10, 0);
			 print_str(1, 0);
			 print_args(7, 7, 9, 0);
			 print_fargs(3, 5, 0);
			 print_args(11, 0);
			 print_str(2, 0);
			 print_args(8, 8, 9, 0);
			 print_fargs(4, 6, 0);
			 timeFnum = 2;
			 timeFvals[0] = 3;
			 timeFvals[1] = 5;
			 timeVnum = 1;
			 timeVals[0] = 9;
			 break;
		case  6:    /* dec2stepsize */
		     	 print_code(PHSTEP, token);
			 cat_arg(argStr, " %d 0 1 ", DO2DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
		  	 break;
		case  7:    /* dec3stepsize */
		     	 print_code(PHSTEP, token);
			 cat_arg(argStr, " %d 0 1 ", DO3DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
		  	 break;
		case  8:    /* gensim2pulse */
			 timeIndex = 1;
		     	 print_code(SMPUL, token);
		         strcat(argStr, " 2 0 1 1 ");
			 print_format(5,FLOAT,6,FLOAT, -1);
			 print_format(INTG, 3, INTG, 1, FLOAT, -1);
			 print_format(INTG, 4, INTG, 2,FLOAT, 0);
			 print_fargs(5, 6, 0);
			 print_args(7, 3, 0);
			 print_fargs(1, 0);
			 print_args(8, 4, 0);
			 print_fargs(2, 0);
			 timeFnum = 6;
			 timeFvals[0] = 1;
			 timeFvals[1] = 2;
			 timeFvals[2] = 0;
			 timeFvals[3] = 0;
			 timeFvals[4] = 5;
			 timeFvals[5] = 6;
		  	 break;
		case  9:    /* gensim3pulse */
			 timeIndex = 1;
		     	 print_code(SMPUL, token);
		         strcat(argStr, " 3 0 1 1 ");
			 print_format(7,FLOAT,8,FLOAT, -1);
			 print_format(INTG, 4, INTG, 1,FLOAT, -1);
			 print_format(INTG, 5, INTG, 2,FLOAT, -1);
			 print_format(INTG, 6, INTG, 3,FLOAT, 0);
			 print_fargs(7, 8, 0);
			 print_args(9,4, 0);
			 print_fargs(1, 0);
			 print_args(10,5, 0);
			 print_fargs(2, 0);
			 print_args(11,6, 0);
			 print_fargs(3, 0);
			 timeFnum = 6;
			 timeFvals[0] = 1;
			 timeFvals[1] = 2;
			 timeFvals[2] = 3;
			 timeFvals[3] = 0;
			 timeFvals[4] = 7;
			 timeFvals[5] = 8;
		  	 break;
		case  10:    /* S_sethkdelay */
		     	 print_code(DELAY, token);
			 strcat(argStr, " 1 0 1 ");
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 timeFnum = 1;
			 timeFvals[0] = 1;
		  	 break;
		case  11:    /* obl_gradient */
			 obl_code(token, OBLGRAD, 3, 3);
			 return (1);
/**
		     	 print_code(OBLGRAD, token);
			strcat(argStr, " 11 0 2 x ");
			print_format(1, FLOAT, -1);
			 strcat(argStr, " psi %.9f ");
			cat_arg(argStr, "\\n%d %s 11 0 2 y ", OBLGRAD, token);
			print_format(2,FLOAT, -1);
			 strcat(argStr, " phi %.9f ");
			cat_arg(argStr, "\\n%d %s 11 0 2 z ", OBLGRAD, token);
			print_format(3,FLOAT, -1);
			 strcat(argStr, " theta %.9f\\n\"");
			print_fargs(1, 0);
			 strcat(argStr, ",psi");
			print_fargs(2, 0);
			 strcat(argStr, ",phi");
			print_fargs(3, 0);
			 strcat(argStr, ",theta");
**/
		  	break;
		case  12:    /* Sincgradient */
		case  13:    /* Wincgradient */
		     	 print_code(INCGRAD, token);
			 strcat(argStr, " 11 7 0 ");
			print_format(CHAR, 2,INTG, 3,INTG, 4,INTG, 5,INTG, 6,INTG, 7,INTG, 8,INTG, 0);
			 print_str(1, 0);
			print_args(2, 3, 4, 5, 6, 7, 8, 0);
		  	break;
		case  14:    /* pe2_gradient */
		     	 print_code(PEGRAD, token);
			 strcat(argStr, " 11 1 4 x 0.00 0 ");
			 print_format(1,FLOAT,-1);
			 strcat(argStr, " 0.00 0.0 0.00 0.0 psi %.9f");
			 cat_arg(argStr, " \\n%d %s 11 1 4 y ", PEGRAD, token);
			 print_format(6,INTG,2,FLOAT,4,FLOAT, -1);
			 strcat(argStr, " nv %.9f phi %.9f \\n ");
			 cat_arg(argStr, "%d %s 11 1 4 z ",PEGRAD,token);
			 print_format(7,INTG,3,FLOAT,5,FLOAT, -1);
			 strcat(argStr, " nv2 %.9f theta %.9f\\n\"");
			 print_fargs(1, 0);
			 strcat(argStr, ", psi");
			 print_args(6, 0);
			 print_fargs(2,4, 0);
			 strcat(argStr, ",nv, phi");
			 print_args(7, 0);
			 print_fargs(3,5, 0);
			 strcat(argStr, ",nv2, theta");
		  	 break;
		case  15:    /* pe3_gradient */
		     	 print_code(PEGRAD, token);
			 strcat(argStr, " 11 1 4 x ");
			 print_format(7,INTG,1,FLOAT,4,FLOAT, -1);
			 strcat(argStr, " nv3 %.9f psi %.9f");
			 cat_arg(argStr, " \\n%d %s 11 1 4 y ", PEGRAD, token);
			 print_format(8,INTG,2,FLOAT,5,FLOAT, -1);
			 strcat(argStr, " nv %.9f phi %.9f");
			 cat_arg(argStr, " \\n%d %s 11 1 4 z ", PEGRAD, token);
			 print_format(9,INTG,3,FLOAT,6,FLOAT, -1);
			 strcat(argStr, " nv2 %.9f theta %.9f\\n\"");
			 print_args(7, 0);
			 print_fargs(1,4, 0);
			 strcat(argStr, ",nv3, psi");
			 print_args(8, 0);
			 print_fargs(2,5, 0);
			 strcat(argStr, ",nv, phi");
			 print_args(9, 0);
			 print_fargs(3,6, 0);
			 strcat(argStr, ",nv2, theta");
		  	 break;
		case  16:    /* poffset_list */
			 poffset_list_code(token, LOFFSET, 4);
			 return (1);
/*
		     	 print_code(POFFSET, token);
			 cat_arg(argStr, "%d 0 3 ", TODEV);
			 print_format(3, FLOAT, 2, FLOAT, -1);
			 strcat(argStr, " resto %.9f\\n\"");
			 print_fargs(3, 2, 0);
			 strcat(argStr, ",resto");
*/
		  	 break;
		case  17:    /* simgradpulse */
			 castFlag[0] = FLOAT;
			 castFlag[1] = FLOAT;
			 change_code(token, token, MGPUL, 2);
			 return (1);
		case  18:    /* observepower */
		     	 print_code(RLPWR, token);
			 cat_arg(argStr, " %d 0 1 ", TODEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  19:    /* CShapedPulse */
			 timeIndex = 3;
		     	 print_code(CSHPUL, token);
			 strcat(argStr, " 1 1 1 1 ");
			 print_format(5, FLOAT, -1);
			 strcat(argStr, " 0.0 0.0 ");
			 print_format(INTG, STRG, 4, INTG, 3, FLOAT, 0);
			 print_fargs(5, 0);
			 print_args(1, 0);
			 print_str(2, 0);
			 print_args(4, 0);
			 print_fargs(3, 0);
			 timeFnum = 2;
			 timeFvals[0] = 3;
			 timeFvals[1] = 5;
			 break;
		case  21:    /* idec4rgpulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 print_format(3, FLOAT, 4,FLOAT, -1);
			 cat_arg(argStr, " %d ", DO4DEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_fargs(3, 4, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 3;
			 timeFvals[2] = 4;
			 break;
		case  22:     /* dec4spinlock */
		     	 print_code(SPINLK, token);
			 cat_arg(argStr, " %d 3 2 ", DO4DEV);
			 print_format(STRG,INTG,4,INTG,5,INTG,2,FLOAT,3,FLOAT,0);
			 print_str(1, 0);
			 print_args(4, 4, 5, 0);
			 print_fargs(2, 3, 0);
			 timeFnum = 2;
			 timeFvals[0] = 2;
			 timeFvals[1] = 3;
			 timeVnum = 1;
			 timeVals[0] = 5;
			 break;
		case  23:    /* dec4stepsize */
		     	 print_code(PHSTEP, token);
			 cat_arg(argStr, " %d 0 1 ", DO4DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
		  	 break;
		case  24:    /* idec5rgpulse */
			 timeIndex = 1;
		     	 print_code(PULSE, token);
			 strcat(argStr, " 1 0 1 1 ");
			 print_format(3, FLOAT, 4,FLOAT, -1);
			 cat_arg(argStr, " %d ", DO5DEV);
			 print_format(2, INTG, 1, FLOAT, 0);
			 print_fargs(3, 4, 0);
			 print_args(2, 0);
			 print_fargs(1, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 3;
			 timeFvals[2] = 4;
			 break;
		case  25:     /* dec5spinlock */
		     	 print_code(SPINLK, token);
			 cat_arg(argStr, " %d 3 2 ", DO5DEV);
			 print_format(STRG,INTG,4,INTG,5,INTG,2,FLOAT,3,FLOAT,0);
			 print_str(1, 0);
			 print_args(4, 4, 5, 0);
			 print_fargs(2, 3, 0);
			 timeFnum = 2;
			 timeFvals[0] = 2;
			 timeFvals[1] = 3;
			 timeVnum = 1;
			 timeVals[0] = 5;
			 break;
		case  26:    /* dec5stepsize */
		     	 print_code(PHSTEP, token);
			 cat_arg(argStr, " %d 0 1 ", DO5DEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
		  	 break;
		case  27:     /* shapedvpulse */
			 timeIndex = 2;
		     	 print_code(SHVPUL, token);
			 strcat(argStr, " 1 1 2 1 ");
			 print_format(5, FLOAT, 6, FLOAT, -1);
			 cat_arg(argStr, " %d ", TODEV);
			 print_format(STRG, 4, INTG, 3, INTG, 2, FLOAT, 0);
			 print_fargs(5, 6, 0);
			 print_str(1, 0);
			 print_args(4, 0);
			 print_args(3, 0);
			 print_fargs(2, 0);
			 timeFnum = 3;
			 timeFvals[0] = 2;
			 timeFvals[1] = 5;
			 timeFvals[2] = 6;
			 break;
		case  28:     /* rotate_angle */
		     	 print_code(ROTATE, token);
			 strcat(argStr, " 1 0 6 ");
			 print_format(1, FLOAT, 2, FLOAT, 3, FLOAT, 4, FLOAT, 
				5, FLOAT, 6, FLOAT, 0);
			 print_fargs(1, 2, 3, 4, 5, 6, 0);
			 break;
		case  29:     /* pbox_xmtroff */
			 pbox_code("pbox_xmtroff", token, TODEV, 0);
			 return (1);
			 break;
		case  30:     /* pbox_dec2off */
			 pbox_code("pbox_xmtroff", token, DO2DEV, 0);
			 return (1);
			 break;
		case  31:     /* pbox_dec3off */
			 pbox_code("pbox_xmtroff", token, DO3DEV, 0);
			 return (1);
			 break;
		case  32:     /* grad_advance */
	      		 print_dummy_code(token);
			 return (1);
			 break;
		case  33:     /* parallelsync */
                         print_code(PARALLELSYNC, token);
                         cat_arg(argStr, " 0 0 0 ");
                         print_format(0);
			 break;
		case  34:     /* decshapelist */
                         gen_shapelist_init(token, DODEV);
                         return(1);
		 default:
	      		 print_dummy_code(token);
			 break;
	        }
		print_statement();
		return (1);
	    }
	}
	return (0);
}


int
compare_13(char *token, FILE *fin)
{
	int	loop;
        int     k;

	 for (loop = 0; loop < cmd13Num; loop++)
	 {
	    if (strcmp(token, cmd_13[loop]) == 0)
	    {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* starthardloop */
		     	 print_code(HLOOP, token);
			 strcat(argStr, " 1 1 0 ");
			 print_format(1, INTG, 0);
			 print_args(1, 0);
			 timeVnum = 1;
			 timeVals[0] = 1;
			 break;
		case  1:     /* set_observech */
		     	 print_code(SETOBS, token);
			 strcat(argStr, " %d 0 0 \\n\"");
			 print_args(1, 0);
			 break;
		case  2:     /* sync_on_event */
			 if (!G_sync())
	      		     print_dummy_code(token);
			 break;
		case  3:     /* S_shapedpulse */
			 timeIndex = 2;
		     	 print_code(SHPUL, token);
			 strcat(argStr, " 1 1 1 1 ");
			 print_format(4, FLOAT, 5, FLOAT, -1);
			 cat_arg(argStr, " %d ", TODEV);
			 print_format(STRG, 3, INTG, 2, FLOAT, 0);
			 print_fargs(4, 5, 0);
			 print_str(1, 0);
			 print_args(3, 0);
			 print_fargs(2, 0);
			 timeFnum = 3;
			 timeFvals[0] = 2;
			 timeFvals[1] = 4;
			 timeFvals[2] = 5;
			 break;
		case  4:     /* S_incgradient */
		     	 print_code(INCGRAD, token);
			 strcat(argStr, " 11 7 0 ");
			 print_format(CHAR, 2,INTG, 3,INTG, 4,INTG, 5,INTG, 6,INTG, 7,INTG, 8,INTG, 0);
			 print_str(1, 0);
			 print_args(2, 3, 4, 5, 6, 7, 8, 0);
			 break;
		case  5:     /* decouplepower */
		     	 print_code(RLPWR, token);
			 cat_arg(argStr, " %d 0 1 ", DODEV);
			 print_format(1, FLOAT, 0);
			 print_fargs(1, 0);
			 break;
		case  6:     /* pbox_decpulse */
			 pbox_code("pbox_pulse", token, DODEV, 4);
			 return (1);
			 break;
		case  7:     /* pbox_spinlock */
			 pbox_code("pbox_spinlock", token, TODEV, 3);
			 return (1);
			 break;
		case  8:     /* pbox_simpulse */
			 pbox_code("pbox_simpulse", token, 2, 6);
			 return (1);
			 break;
		case  9:     /* SweepNAcquire */
			timeIndex = 93;
		        if (timeMode)
		     	    print_code(DELAY, token);
			else
		     	    print_code(SPACQ, token);
			strcat(argStr, " 1 0 1 5 rof1 %.9f rof2 %.9f %d ");
		 	print_format(3,INTG, -1);
			strcat(argStr, " at %.9f ");
		 	print_format(1,FLOAT,2,FLOAT, -1);
			strcat(argStr, " np %.9f sw %.9f ");
		 	print_format(0);
			strcat(argStr, ", rof1, rof2");
			print_args(4, 3, 0);
			strcat(argStr, ", at");
			print_fargs(1, 2, 0);
			strcat(argStr, ",np,sw");
			timeFnum = 1;
			timeFvals[0] = 93;
			break;
		case  10:     /* triggerSelect */
		  	print_code(TRIGGER, token);
			strcat(argStr, " 1 1 0 ");
			print_format(1, INTG, 0);
			print_args(1, 0);
			break;
		case  11:     /* gen_shapelist */
	      		 print_dummy_code(token);
			 break;
		case  12:     /* parallelstart */
		     	 print_code(PARALLELSTART, token);
                         k = get_parallel_chan_id(1);
                         sprintf(tmpStr, "%d", k);
                         insert_arg(tmpStr, 2);
			 cat_arg(argStr, " 0 1 0 ");
			 print_format(STRG,INTG, -1);
		 	 print_format(0);
			 print_str(1, 0);
			 print_args(2, 0);
                         timeVnum = 1;
                         timeVals[0] = 92;
			 break;
		case  13:     /* dec2shapelist */
                         gen_shapelist_init(token, DO2DEV);
                         return (1);
		case  14:     /* dec3shapelist */
                         gen_shapelist_init(token, DO3DEV);
                         return (1);
		default:
	      		 print_dummy_code(token);
			 break;
	        }
		print_statement();
		return (1);
	    }
	 }
	 return (0);
}


int
compare_14(char *token, FILE *fin)
{
	int	loop;

	for (loop = 0; loop < cmd14Num; loop++)
	{
	    if (strcmp(token, cmd_14[loop]) == 0)
            {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* shapedgradient */
			timeIndex = 2;
		     	print_code(SHGRAD, token);
			strcat(argStr, " 12 2 2 ");
		 	print_format(CHAR,STRG,5,INTG,6,INTG,2,FLOAT,3,FLOAT,0);
			print_str(4, 1, 0);
			print_args(5, 6, 0);
			print_fargs(2, 3, 0);
			timeFnum = 1;
			timeFvals[0] = 2;
			timeVnum = 2;
			timeVals[0] = 5;
			timeVals[1] = 6;
		 	break;
		case  1:     /* decshapedpulse */
			timeIndex = 2;
		     	print_code(SHPUL, token);
			strcat(argStr, " 1 1 1 1 ");
			print_format(4, FLOAT, 5, FLOAT, -1);
			cat_arg(argStr, " %d ", DODEV);
			print_format(STRG, 3, INTG, 2, FLOAT, 0);
			print_fargs(4, 5, 0);
			print_str(1, 0);
			print_args(3, 0);
			print_fargs(2, 0);
			timeFnum = 3;
			timeFvals[0] = 2;
			timeFvals[1] = 4;
			timeFvals[2] = 5;
			break;
		case  2:     /* simshapedpulse */
			timeIndex = 3;
		     	print_code(SMSHP, token);
		        strcat(argStr, " 2 1 1 1 ");
			print_format(7,FLOAT,8,FLOAT, -1);
			print_format(INTG,STRG,5,INTG,3,FLOAT, -1);
			print_format(INTG,STRG,6,INTG,4,FLOAT, 0);
			print_fargs(7, 8, 0);
	 		cat_arg(argStr, ", %d", TODEV);
			print_str(1, 0);
			print_args(5, 0);
			print_fargs(3, 0);
	 		cat_arg(argStr, ", %d", DODEV);
			print_str(2, 0);
			print_args(6, 0);
			print_fargs(4, 0);
			timeFnum = 6;
			timeFvals[0] = 3;
			timeFvals[1] = 4;
			timeFvals[2] = 0;
			timeFvals[3] = 0;
			timeFvals[4] = 7;
			timeFvals[5] = 8;
			break;
		case  3:     /* apshaped_pulse */
			timeIndex = 2;
		     	print_code(APSHPUL, token);
		        strcat(argStr, " 1 1 3 1 ");
			print_format(6,FLOAT,7,FLOAT, -1);
		        cat_arg(argStr, " %d ", TODEV);
			print_format(STRG,3,INTG,4,INTG,5,INTG,2,FLOAT,0);
			print_fargs(6, 7, 0);
			print_str(1, 0);
			print_args(3, 4, 5, 0);
			print_fargs(2, 0);
			timeFnum = 3;
			timeFvals[0] = 2;
			timeFvals[1] = 6;
			timeFvals[2] = 7;
			break;
		case  4:     /* CGradientPulse */
			timeIndex = 3;
		     	print_code(CGPUL, token);
			strcat(argStr, " 11 0 2 ");
			print_format(STRG, 3, FLOAT, 2, FLOAT, 0);
			print_str(1, 0);
			print_fargs(3, 2, 0);
			timeFnum = 1;
			timeFvals[0] = 3;
			break;
		case  5:     /* pbox_dec2pulse */
			pbox_code("pbox_pulse", token, DO2DEV, 4);
			return (1);
			break;
		case  6:     /* pbox_dec3pulse */
			pbox_code("pbox_pulse", token, DO3DEV, 4);
			return (1);
			break;
		case  7:     /* pbox_sim3pulse */
			pbox_code("pbox_sim3pulse", token, 3, 8);
			return (1);
			break;
		case  8:     /* gen_offsetlist */
	      		print_dummy_code(token);
			break;
		case  9:     /* rot_angle_list */
                        print_code(ROTATEANGLE, token);
                        strcat(argStr, " 1 3 0 ");
                        print_format(CHAR,INTG,1,INTG,3,INTG, 0);
                        print_str(2, 0);
                        print_args(1,1,3, 0);
			break;
		case  10:     /* obsprgonOffset */
		     	print_code(VPRGON, token);
			cat_arg(argStr, " %d 0 4 ", TODEV);
			print_format(STRG,FLOAT,2,FLOAT,3,FLOAT,4,FLOAT, 0);
			print_str(1, 0);
			print_fargs(2, 2, 3, 4, 0);
			break;
		case  11:     /* decprgonOffset */
		     	print_code(VPRGON, token);
			cat_arg(argStr, " %d 0 4 ", DODEV);
			print_format(STRG,FLOAT,2,FLOAT,3,FLOAT,4,FLOAT, 0);
			print_str(1, 0);
			print_fargs(2, 2, 3, 4, 0);
			break;
		case  12:     /* set_angle_list */
                        print_code(SETANGLE, token);
			cat_arg(argStr, " %d 4 0 ", TODEV);
                        print_format(1,INTG,STRG,INTG,CHAR,INTG,4,INTG, 0);
                        print_args(1, 0);
                        print_str(2, 0);
                        print_args(1,0);
                        print_str(3, 0);
                        print_args(1, 4, 0);
			break;
		case  13:     /* setactivercvrs */
                        print_code(ACTIVERCVR, token);
			cat_arg(argStr, " %d 0 0 ", TODEV);
                        print_format(1, STRG, 0);
			print_str(1, 0);
			break;
		case  14:     /* nowait_endloop */
                        print_code(NWLOOPEND, token);
                        strcat(argStr, " 1 1 0 ");
                        print_format(1,INTG, 0);
                        print_args(1, 0);
                        timeVnum = 1;
                        timeVals[0] = 1;
			break;
		 default:
	      		print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
            }
	}
	return (0);
}


int
compare_15(char *token, FILE *fin)
{
	int	loop;

	for (loop = 0; loop < cmd15Num; loop++)
	{
	    if (strcmp(token, cmd_15[loop]) == 0)
	    {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* decshaped_pulse */
			timeIndex = 2;
		     	print_code(SHPUL, token);
			strcat(argStr, " 1 1 1 1 ");
			print_format(4,FLOAT, 5,FLOAT, -1);
			cat_arg(argStr, " %d ", DODEV);
			print_format(STRG, 3, INTG, 2,FLOAT, 0);
			print_fargs(4, 5, 0);
			print_str(1, 0);
			print_args(3, 0);
			print_fargs(2, 0);
			timeFnum = 3;
			timeFvals[0] = 2;
			timeFvals[1] = 4;
			timeFvals[2] = 5;
			break;
		case  1:     /* simshaped_pulse */
			timeIndex = 3;
		     	print_code(SMSHP, token);
		        strcat(argStr, " 2 1 1 1 ");
			print_format(7,FLOAT,8,FLOAT, -1);
			print_format(INTG,STRG,5,INTG,3,FLOAT, -1);
			print_format(INTG,STRG,6,INTG,4,FLOAT, 0);
			print_fargs(7, 8, 0);
	 		cat_arg(argStr, ", %d", TODEV);
			print_str(1, 0);
			print_args(5, 0);
			print_fargs(3, 0);
	 		cat_arg(argStr, ", %d", DODEV);
			print_str(2, 0);
			print_args(6, 0);
			print_fargs(4, 0);
			timeFnum = 6;
			timeFvals[0] = 3;
			timeFvals[1] = 4;
			timeFvals[2] = 0;
			timeFvals[3] = 0;
			timeFvals[4] = 7;
			timeFvals[5] = 8;
			break;
		case  2:     /* genshaped_pulse */
			timeIndex = 2;
		     	print_code(SHPUL, token);
			strcat(argStr, " 1 1 1 1 ");
			print_format(4,FLOAT,5,FLOAT, -1);
			print_format(INTG,STRG,3,INTG, 2,FLOAT, 0);
			print_fargs(4, 5, 0);
			print_args(8, 0);
			print_str(1, 0);
			print_args(3, 0);
			print_fargs(2, 0);
			timeFnum = 3;
			timeFvals[0] = 2;
			timeFvals[1] = 4;
			timeFvals[2] = 5;
			break;
		case  3:     /* shapedvgradient */
			timeIndex = 2;
		     	print_code(SHVGRAD, token);
			strcat(argStr, " 12 4 3 ");
		 	print_format(CHAR,STRG,7,INTG,8,INTG,7,INTG,5,INTG, -1);
		 	print_format(2,FLOAT,3,FLOAT,4,FLOAT, 0);
			print_str(6, 1, 0);
			print_args(7, 8, 7, 5, 0);
			print_fargs(2, 3, 4, 0);
			timeFnum = 1;
			timeFvals[0] = 2;
			timeVnum = 2;
			timeVals[0] = 7;
			timeVals[1] = 8;
			break;
		case  4:     /* position_offset */
		     	print_code(POFFSET, token);
			strcat(argStr, " %d 0 3 ");
			print_format(1, FLOAT, 2, FLOAT, 3, FLOAT, 0);
			print_args(4, 0);
			print_fargs(1, 2, 3, 0);
			break;
		case  5:     /* readMRIUserByte */
		  	print_code(RDBYTE, token);
			strcat(argStr, " 1 1 1 ");
			print_format(1, INTG, 2, FLOAT, 0);
			print_args(1, 0);
			print_fargs(2, 0);
			break;
		case  6:     /* setMRIUserGates */
		  	print_code(SETGATE, token);
			strcat(argStr, " 1 1 0 ");
			print_format(1, INTG, 0);
			print_args(1, 0);
			break;
		case  7:     /* shapedpulselist */
                       /********
                        if (argNum >= 7) {
                           insert_arg("OBSch", 8);
                           insert_arg("0.0", 9);
                           strcpy(tmpStr, "\"shapedpulselist\"");
                           argPtrs[0] = tmpStr;
                           gen_shapedPulseWithOffset(token);
			}
                        *********/
                        if (argNum >= 7) {
                           gen_shapedpulselist(token, TODEV); 
			   return (1);
                        }
	      		print_dummy_code(token);
			break;
		case  8:     /* gen_offsetglist */
	      		print_dummy_code(token);
			break;
		case  9:     /* dec2prgonOffset */
		     	print_code(VPRGON, token);
			cat_arg(argStr, " %d 0 4 ", DO2DEV);
			print_format(STRG,FLOAT,2,FLOAT,3,FLOAT,4,FLOAT, 0);
			print_str(1, 0);
			print_fargs(2, 2, 3, 4, 0);
			break;
		case  10:     /* dec3prgonOffset */
		     	print_code(VPRGON, token);
			cat_arg(argStr, " %d 0 4 ", DO3DEV);
			print_format(STRG,FLOAT,2,FLOAT,3,FLOAT,4,FLOAT, 0);
			print_str(1, 0);
			print_fargs(2, 2, 3, 4, 0);
			break;
		case  11:     /* gen_shapelistpw */
	      		gen_shapelistpw(token);
			break;
		case  12:     /* dec2shapelistpw */
	      		gen_shapelistpw(token);
			break;
		case  13:     /* dec3shapelistpw */
	      		gen_shapelistpw(token);
			break;
		 default:
	      		print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	    }
	 }
	 return (0);
}


int
compare_16(char *token, FILE *fin)
{
	int	loop;

	for (loop = 0; loop < cmd16Num; loop++)
	{
	    if (strcmp(token, cmd_16[loop]) == 0)
	    {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* dec2shaped_pulse */
			timeIndex = 2;
		     	 print_code(SHPUL, token);
			 strcat(argStr, " 1 1 1 1 ");
			 print_format(4,FLOAT,5,FLOAT, -1);
			 cat_arg(argStr, " %d ", DO2DEV);
			 print_format(STRG,3,INTG,2,FLOAT, 0);
			 print_fargs(4, 5, 0);
			 print_str(1, 0);
			 print_args(3, 0);
			 print_fargs(2, 0);
			 timeFnum = 3;
			 timeFvals[0] = 2;
			 timeFvals[1] = 4;
			 timeFvals[2] = 5;
			 break;
		case  1:     /* dec3haped_pulse */
			timeIndex = 2;
		     	 print_code(SHPUL, token);
			 strcat(argStr, " 1 1 1 1 ");
			 print_format(4,FLOAT,5,FLOAT, -1);
			 cat_arg(argStr, " %d ", DO3DEV);
			 print_format(STRG,3,INTG,2,FLOAT, 0);
			 print_fargs(4, 5, 0);
			 print_str(1, 0);
			 print_args(3, 0);
			 print_fargs(2, 0);
			 timeFnum = 3;
			 timeFvals[0] = 2;
			 timeFvals[1] = 4;
			 timeFvals[2] = 5;
			 break;
		case  2:     /* sim3shaped_pulse */
			timeIndex = 4;
		     	 print_code(SMSHP, token);
		         strcat(argStr, " 3 1 1 1 ");
			 print_format(10,FLOAT,11,FLOAT, -1);
			 print_format(INTG,STRG,7,INTG,4,FLOAT, -1);
			 print_format(INTG,STRG,8,INTG,5,FLOAT, -1);
			 print_format(INTG,STRG,9,INTG,6,FLOAT, 0);
			 print_fargs(10, 11, 0);
			 cat_arg(argStr, ", %d", TODEV);
			 print_str(1, 0);
			 print_args(7, 0);
			 print_fargs(4, 0);
			 cat_arg(argStr, ", %d", DODEV);
			 print_str(2, 0);
			 print_args(8, 0);
			 print_fargs(5, 0);
			 cat_arg(argStr, ", %d", DO2DEV);
			 print_str(3, 0);
			 print_args(9, 0);
			 print_fargs(6, 0);
			 timeFnum = 6;
		 	 timeFvals[0] = 4;
			 timeFvals[1] = 5;
			 timeFvals[2] = 6;
			 timeFvals[3] = 0;
			 timeFvals[4] = 10;
			 timeFvals[5] = 11;
			 break;
		case  3:     /* sim4shaped_pulse */
			 timeIndex = 5;
	      		 print_dummy_code(token);
			 break;
		case  4:     /* S_decshapedpulse */
			timeIndex = 2;
		     	 print_code(SHPUL, token);
			strcat(argStr, " 1 1 1 1 ");
			print_format(4,FLOAT,5,FLOAT, -1);
			cat_arg(argStr, " %d ", DODEV);
			print_format(STRG,3,INTG, 2,FLOAT, 0);
			print_fargs(4, 5, 0);
			print_str(1, 0);
			print_args(3, 0);
			print_fargs(2, 0);
			timeFnum = 3;
			timeFvals[0] = 2;
			timeFvals[1] = 4;
			timeFvals[2] = 5;
			break;
		case  5:     /* S_simshapedpulse */
			timeIndex = 3;
		     	print_code(SMSHP, token);
		        strcat(argStr, " 2 1 1 1 ");
			print_format(7,FLOAT,8,FLOAT, -1);
			print_format(INTG,STRG,5,INTG,3,FLOAT, -1);
			print_format(INTG,STRG,6,INTG,4,FLOAT, 0);
			print_fargs(7, 8, 0);
	 		cat_arg(argStr, ", %d", TODEV);
			print_str(1, 0);
			print_args(5, 0);
			print_fargs(3, 0);
	 		cat_arg(argStr, ", %d", DODEV);
			print_str(2, 0);
			print_args(6, 0);
			print_fargs(4, 0);
			timeFnum = 6;
			timeFvals[0] = 3;
			timeFvals[1] = 4;
			timeFvals[2] = 0;
			timeFvals[3] = 0;
			timeFvals[4] = 7;
			timeFvals[5] = 8;
			break;
		case  6:     /* S_shapedgradient */
			timeIndex = 2;
		     	print_code(SHGRAD, token);
			strcat(argStr, " 12 2 2 ");
		 	print_format(CHAR,STRG,5,INTG,6,INTG,2,FLOAT,3,FLOAT,0);
			print_str(4, 1, 0);
			print_args(5, 6, 0);
			print_fargs(2, 3, 0);
			timeFnum = 1;
			timeFvals[0] = 2;
			timeVnum = 2;
			timeVals[0] = 5;
			timeVals[1] = 6;
		 	break;
		case  7:    /* oblique_gradient */
                        if (argNum == 3)
                           insert_angles(4);
			obl_code(token, OBLGRAD, 6, 3);
			return (1);
/**
		     	print_code(OBLGRAD, token);
			strcat(argStr, " 11 0 2 x ");
			print_format(1, FLOAT, 4, FLOAT, -1);
			cat_arg(argStr, "\\n%d %s 11 0 2 y ", OBLGRAD, token);
			print_format(2,FLOAT, 5, FLOAT, -1);
			cat_arg(argStr, "\\n%d %s 11 0 2 z ", OBLGRAD, token);
			print_format(3,FLOAT, 6, FLOAT, 0);
			print_fargs(1,4,2,5,3,6, 0);
**/
		  	break;
		case  8:     /* shaped2Dgradient */
			timeIndex = 2;
		     	print_code(SHGRAD, token);
			strcat(argStr, " 12 2 2 ");
		 	print_format(CHAR,STRG,5,INTG,6,INTG,2,FLOAT,3,FLOAT,0);
			print_str(4, 1, 0);
			print_args(5, 6, 0);
			print_fargs(2, 3, 0);
			timeFnum = 1;
			timeFvals[0] = 2;
			timeVnum = 2;
			timeVals[0] = 5;
			timeVals[1] = 6;
		 	break;
		case  9:     /* mashapedgradient */
			 castFlag[1] = FLOAT;
			 castFlag[2] = FLOAT;
			 castFlag[3] = INTG;
			 castFlag[4] = INTG;
/*
			change_code(token, token, SHGRAD, 7);
*/
			change_code("simshapedgradient", token, SHGRAD, 5);
			return (1);
		case  10:    /* vashapedgradient */
			castFlag[1] = FLOAT;
			castFlag[2] = FLOAT;
			castFlag[3] = FLOAT;
			castFlag[4] = FLOAT;
			castFlag[5] = INTG;
			castFlag[6] = INTG;
			change_code("mashapedgradient", token, SHGRAD, 7);
			return (1);
		case  11:     /* create_freq_list */
			castFlag[1] = INTG;
			castFlag[2] = INTG;
			castFlag[3] = INTG;
			change_code(token, token, DUMMY, 4);
			return (1);
		case  12:     /* dec4haped_pulse */
			timeIndex = 2;
		     	 print_code(SHPUL, token);
			 strcat(argStr, " 1 1 1 1 ");
			 print_format(4,FLOAT,5,FLOAT, -1);
			 cat_arg(argStr, " %d ", DO4DEV);
			 print_format(STRG,3,INTG,2,FLOAT, 0);
			 print_fargs(4, 5, 0);
			 print_str(1, 0);
			 print_args(3, 0);
			 print_fargs(2, 0);
			 timeFnum = 3;
			 timeFvals[0] = 2;
			 timeFvals[1] = 4;
			 timeFvals[2] = 5;
			 break;
		case  13:     /* dec5haped_pulse */
			timeIndex = 2;
		     	 print_code(SHPUL, token);
			 strcat(argStr, " 1 1 1 1 ");
			 print_format(4,FLOAT,5,FLOAT, -1);
			 cat_arg(argStr, " %d ", DO5DEV);
			 print_format(STRG,3,INTG,2,FLOAT, 0);
			 print_fargs(4, 5, 0);
			 print_str(1, 0);
			 print_args(3, 0);
			 print_fargs(2, 0);
			 timeFnum = 3;
			 timeFvals[0] = 2;
			 timeFvals[1] = 4;
			 timeFvals[2] = 5;
			 break;
		case  14:     /* pbox_decspinlock */
			 pbox_code("pbox_spinlock", token, DODEV, 3);
			 return (1);
			 break;
		case  15:     /* writeMRIUserByte */
		  	print_code(WRBYTE, token);
			strcat(argStr, " 1 1 0 ");
			print_format(1, INTG, 0);
			print_args(1, 0);
			break;
		case  16:     /* initRFGroupPulse */
			castFlag[0] = FLOAT;
			castFlag[1] = STRG;
			castFlag[2] = CHAR;
			castFlag[3] = FLOAT;
			castFlag[4] = FLOAT;
			castFlag[5] = FLOAT;
			castFlag[6] = NONE;
			castFlag[7] = INTG;
			change_code("initRFGroupPulse",token, INITGRPPULSE, 8);
			return (1);
		 default:
	      		print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	    }
	 }
	 return (0);
}



int
compare_17(char *token, FILE *fin)
{
	int	loop;

	for (loop = 0; loop < cmd17Num; loop++)
	{
	    if (strcmp(token, cmd_17[loop]) == 0)
	    {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* apshaped_decpulse */
			timeIndex = 2;
		     	print_code(APSHPUL, token);
		        strcat(argStr, " 1 1 3 1 ");
			print_format(6,FLOAT,7,FLOAT, -1);
		        cat_arg(argStr, " %d ", DODEV);
			print_format(STRG,3,INTG,4,INTG,5,INTG,2,FLOAT, 0);
			print_fargs(6, 7, 0);
			print_str(1, 0);
			print_args(3, 4, 5, 0);
			print_fargs(2, 0);
			timeFnum = 3;
			timeFvals[0] = 2;
			timeFvals[1] = 6;
			timeFvals[2] = 7;
			break;
		case  1:     /* shapedincgradient */
			timeIndex = 3;
		     	print_code(SHINCGRAD, token);
			strcat(argStr, " 12 5 5 ?%c ?%s ");
		 	print_format(11,INTG,12,INTG,8,INTG,9,INTG,10,INTG, -1);
		 	print_format(3,FLOAT,4,FLOAT,5,FLOAT,6,FLOAT,7,FLOAT, 0);
			print_args(1,2, 11, 12, 8, 9, 10, 0);
			print_fargs(3, 4, 5, 6, 7, 0);
			timeFnum = 1;
			timeFvals[0] = 3;
			timeVnum = 2;
			timeVals[0] = 11;  /* loop */
			timeVals[1] = 12;  /* wait */
		 	break;
		case  2:     /* pe_shapedgradient */
			timeIndex = 2;
		     	print_code(PESHGR, token);
	 		strcat(argStr, " 12 3 4 x ");
                        print_format(STRG, -1);
	 		strcat(argStr, " 1.00 1  0.00 0  0  0 ");
                         // loop, wait, multiple
		 	print_format(2,FLOAT,3,FLOAT, -1);
	 		strcat(argStr, " 0.00 0.0 0.0 0.0 ");
                        //  width, level, step, multiple2

	 		cat_arg(argStr," \\n%d %s 12 3 4 y ",PESHGR,token);
                        print_format(STRG, -1);
	 		strcat(argStr, "1.00 1  0.00  0 ");
		 	print_format(7, INTG, -1);
		 	print_format(2,FLOAT,4,FLOAT, 6,FLOAT, -1);
	 		strcat(argStr, " 0.00 0.0 ");

	 		cat_arg(argStr," \\n%d %s 12 3 4 z ",PESHGR,token);
                        print_format(STRG, -1);
	 		strcat(argStr, "  1.00 1 ");
		 	print_format(8,INTG,  -1);
	 		strcat(argStr, "  0.00 0 ");
		 	print_format(2,FLOAT,5,FLOAT,  -1);
	 		strcat(argStr, " 0.00 0.0  0.0 0.0");
		 	print_format(0, 0);
                        strcat(argStr, "\n           ");
			print_str(1, 0);
			print_fargs(2,3, 0);
                        strcat(argStr, "\n           ");
			print_str(1, 0);
			print_args(7, 0);
			print_fargs(2,4,6, 0);
                        strcat(argStr, "\n           ");
			print_str(1, 0);
			print_args(8, 0);
			print_fargs(2,5, 0);

			timeFnum = 1;
			timeFvals[0] = 2;
			timeVnum = 2;
			timeVals[0] = 91;  /* loops is 1 */
			timeVals[1] = 8;  /* wait */
		 	break;
		case  3:     /* shaped_V_gradient */
			timeIndex = 2;
		     	print_code(SHVGRAD, token);
			strcat(argStr, " 12 4 3 %c ?%s ");
		 	print_format(7,INTG,8,INTG,7,INTG,5,INTG, -1);
		 	print_format(2,FLOAT,3,FLOAT, 4,FLOAT, 0);
			print_args(6, 1, 7, 8, 7, 5, 0);
			print_fargs(2, 3, 4, 0);
			timeFnum = 1;
			timeFvals[0] = 2;
			timeVnum = 2;
			timeVals[0] = 7;
			timeVals[1] = 8;
			break;
		case  4:     /* S_position_offset */
		     	print_code(POFFSET, token);
			strcat(argStr, " %d 0 3 ");
			print_format(1, FLOAT, 2, FLOAT, 3, FLOAT, 0);
			print_args(4, 0);
			print_fargs(1, 2, 3, 0);
			break;
		case  5:     /* oblique_gradpulse */
			castFlag[0] = FLOAT;
			castFlag[1] = FLOAT;
			castFlag[2] = FLOAT;
			castFlag[3] = FLOAT;
			castFlag[4] = FLOAT;
			castFlag[5] = FLOAT;
			castFlag[6] = FLOAT;
			change_code(token, token, MGPUL, 7);
			return (1);
		case  6:     /* mashapedgradpulse */
			castFlag[1] = FLOAT;
			castFlag[2] = FLOAT;
/*
			change_code(token, token, SHGRAD, 5);
*/
			change_code("simshapedgradpulse", token, SHGRAD, 3);
			return (1);
		case  7:     /* simshapedgradient */
			castFlag[1] = FLOAT;
			castFlag[2] = FLOAT;
			castFlag[3] = INTG;
			castFlag[4] = INTG;
			change_code(token, token, SHGRAD, 5);
			return (1);
		case  8:     /* vashapedgradpulse */
			castFlag[1] = FLOAT;
			castFlag[2] = FLOAT;
			castFlag[3] = FLOAT;
			castFlag[4] = FLOAT;
			change_code("mashapedgradpulse", token, SHGRAD, 5);
			return (1);
		case  9:     /* create_delay_list */
			castFlag[1] = INTG;
			castFlag[2] = INTG;
			change_code(token, token, CDLST, 3);
			return (1);
		case  10:     /* pbox_dec2spinlock */
		        pbox_code("pbox_spinlock", token, DO2DEV, 3);
			return (1);
		case  11:     /* pbox_dec3spinlock */
		        pbox_code("pbox_spinlock", token, DO3DEV, 3);
			return (1);
		case  13:     /* ShapedXmtNAcquire */
			timeIndex = 93;
		        if (timeMode)
		     	    print_code(DELAY, token);
			else
		     	    print_code(SHACQ, token);
			strcat(argStr, " 1 1 1 5 rof1 %.9f rof2 %.9f %d ");
		 	print_format(STRG, -1);
		 	print_format(3,INTG,2,FLOAT,4,FLOAT, -1);
			strcat(argStr, " at %.9f np %.9f sw %.9f ");
		 	print_format(0);
			strcat(argStr, ", rof1, rof2");
			print_args(5, 0);
			print_str(1, 0);
			print_args(3, 0);
			print_fargs(2, 4, 0);
			strcat(argStr, ", at,np,sw");
			timeFnum = 1;
			timeFvals[0] = 93;
		 	break;
		case  14:     /* shapedpulseoffset */
                        /**********
                        if (argNum >= 6) {
                           insert_arg(argPtrs[5], 7);
                           insert_arg("OBSch", 8);
                           insert_arg(argPtrs[5], 9);
                           gen_shapedPulseWithOffset(token);
                        }
                        **********/
                        gen_shapedpulseoffset(token, TODEV);
		 	break;
		case  15:     /* modifyRFGroupPwrf */
		        print_code(GRPPWRF, token);
		        strcat(argStr, " 1 2 1 ");
		        print_format(1, INTG, 2, INTG, -1);
		        print_format(3,FLOAT, 0);
		        print_args(1,2, 0);
		        print_fargs(3, 0);
		 	break;
		case  16:     /* modifyRFGroupPwrC */
		        print_code(GRPPWR, token);
		        strcat(argStr, " 1 2 1 ");
		        print_format(1, INTG, 2, INTG, -1);
		        print_format(3,FLOAT, 0);
		        print_args(1,2, 0);
		        print_fargs(3, 0);
		 	break;
		case  17:     /* modifyRFGroupName */
		        print_code(GRPNAME, token);
		        strcat(argStr, " 1 2 0 ");
		        print_format(1, INTG,2, INTG, STRG, 0);
		        print_args(1,2, 0);
			print_str(3, 0);
		 	break;
		case  18:     /* create_angle_list */
			create_angle_list();
		        return (1);
		 	break;
		case  19:     /* exe_grad_rotation */
		  	print_code(EXECANGLE, token);
			strcat(argStr, " 1 0 0 0 0 \\n\"");
		 	break;
		default:
		     	print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	    }
	 }
	 return (0);
}



int
compare_18(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd18Num; loop++)
	 {
	     if (strcmp(token, cmd_18[loop]) == 0) 
             {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* shaped_2D_gradient */
			timeIndex = 2;
		      	print_code(SHGRAD, token);
			strcat(argStr, " 12 2 2 ");
		 	print_format(CHAR,STRG,5,INTG,6,INTG,2,FLOAT,3,FLOAT,0);
			print_str(4, 1, 0);
			print_args(5, 6, 0);
			print_fargs(2, 3, 0);
			timeFnum = 1;
			timeFvals[0] = 2;
			timeVnum = 2;
			timeVals[0] = 5;
			timeVals[1] = 6;
			break;
		case  1:     /* apshaped_dec2pulse */
			timeIndex = 2;
		     	print_code(APSHPUL, token);
		        strcat(argStr, " 1 1 3 1 ");
			print_format(6,FLOAT,7,FLOAT, -1);
		        cat_arg(argStr, " %d ", DO2DEV);
			print_format(STRG,3,INTG,4,INTG,5,INTG,2,FLOAT,0);
			print_fargs(6, 7, 0);
			print_str(1, 0);
			print_args(3, 4, 5, 0);
			print_fargs(2, 0);
			timeFnum = 3;
			timeFvals[0] = 2;
			timeFvals[1] = 6;
			timeFvals[2] = 7;
			break;
		case  2:     /* gen_apshaped_pulse */
			timeIndex = 2;
		     	print_code(APSHPUL, token);
		        strcat(argStr, " 1 1 3 1 ");
			print_format(6,FLOAT,7,FLOAT, -1);
			print_format(INTG,STRG,3,INTG,4,INTG,5,INTG,2,FLOAT,0);
			print_fargs(6, 7, 0);
			print_args(8, 0);
			print_str(1, 0);
			print_args(3, 4, 5, 0);
			print_fargs(2, 0);
			timeFnum = 3;
			timeFvals[0] = 2;
			timeFvals[1] = 6;
			timeFvals[2] = 7;
			break;
		case  3:     /* S_oblique_gradient */
			obl_code(token, OBLGRAD, 6, 3);
			return (1);
/**
		     	print_code(OBLGRAD, token);
			strcat(argStr, " 11 0 2 x ");
			print_format(1, FLOAT, 4, FLOAT,  -1);
			cat_arg(argStr, "\\n%d %s 11 0 2 y ", OBLGRAD, token);
			print_format(2, FLOAT, 5, FLOAT, -1);
			cat_arg(argStr, "\\n%d %s 11 0 2 z ", OBLGRAD, token);
			print_format(3,FLOAT, 6, FLOAT, 0);
			print_fargs(1,4, 2,5, 3,6, 0);
**/
		  	break;
		case  4:     /* obl_shapedgradient */
			timeIndex = 4;
                        if (argNum <= 6) {  /* nvpsg */
                            insert_arg(oneStr, 6);  /* loops */
                            insert_arg(argPtrs[0], 1);
                            insert_arg(argPtrs[0], 1);
                        }
                        if (argNum <= 9)
                            insert_angles(8);
                        print_oblsh_code(token);
/*
		     	print_code(OBLSHGR, token);
	 		strcat(argStr, " 12 2 3 x ?%s ");
		 	print_format(8,INTG,-1);
			strcat(argStr, " 0.00  0 ");
		 	print_format(4,FLOAT,5,FLOAT, -1);
			strcat(argStr, " psi %.9f ");
			cat_arg(argStr, "\\n%d %s 12 2 3 y ?%%s ",OBLSHGR,token);
		 	print_format(8,INTG,-1);
			strcat(argStr, " 0.00  0 ");
		 	print_format(4,FLOAT,6,FLOAT, -1);
			strcat(argStr, " phi %.9f ");
			cat_arg(argStr, "\\n%d %s 12 2 3 z ?%%s ",OBLSHGR,token);
		 	print_format(8,INTG, 9,INTG, -1);
		 	print_format(4,FLOAT,7,FLOAT, -1);
			strcat(argStr, " theta %.9f \\n\"");
			print_str(1, 0);
			print_args(8, 0);
			print_fargs(4,5, 0);
			strcat(argStr, ", psi");
			print_str(2, 0);
			print_args(8, 0);
			print_fargs(4,6, 0);
			strcat(argStr, ", phi");
			print_str(3, 0);
			print_args(8, 9, 0);
			print_fargs(4,7, 0);
			strcat(argStr, ", theta");
*/
			timeFnum = 1;
			timeFvals[0] = 4;
			timeVnum = 2;
			timeVals[0] = 11;  /* loop */
			timeVals[1] = 12;  /* wait */
		  	break;
		case  5:     /* pe2_shapedgradient */
			timeIndex = 2;
                        if (argNum <= 9)
                            insert_arg(oneStr, 10); /* wait */
		     	print_code(PESHGR, token);
	 		strcat(argStr, " 12 3 4 x ");
                        print_format(STRG, -1);
			strcat(argStr, " 1.00 1 0.00 0 0.00 0 ");
		 	print_format(2,FLOAT,3,FLOAT,-1);
			strcat(argStr, " 0.00 0.0 0.00 0.0 ");
			cat_arg(argStr, "\\n%d %s 12 3 4 y ",PESHGR,token);
                        print_format(STRG, -1);
			strcat(argStr, " 1.00 1 0.00 0 ");
		 	print_format(8,INTG,2,FLOAT,4,FLOAT,6,FLOAT, -1);
			strcat(argStr, " nv/2.0 %.9f ");
			cat_arg(argStr, "\\n%d %s 12 3 4 z ",PESHGR,token);
                        print_format(STRG, -1);
			strcat(argStr, " 1.00 1 ");
		 	print_format(10,INTG,-1);  /* wait */
		 	print_format(9,INTG,2,FLOAT,5,FLOAT,7,FLOAT, -1);
			strcat(argStr, " nv2/2.0 %.9f ");
                        print_format(0, 0);
                        strcat(argStr, "\n           ");
			print_str(1, 0);
			print_fargs(2,3, 0);
                        strcat(argStr, "\n           ");
			print_str(1, 0);
			print_args(8, 0);
			print_fargs(2,4,6, 0);
			strcat(argStr, ", nv/2.0");
                        strcat(argStr, "\n           ");
			print_str(1, 0);
			print_args(10,9, 0);
			print_fargs(2,5,7, 0);
			strcat(argStr, ", nv2/2.0");
			timeFnum = 1;
			timeFvals[0] = 2;
			timeVnum = 2;
			timeVals[0] = 91;  /* loops is 1 */
			timeVals[1] = 10;  /* wait is True */
		  	break;
		case  6:     /* pe3_shapedgradient */
			timeIndex = 2;
                        if (argNum <= 11)
                            insert_arg(oneStr, 12); /* wait */
		     	print_code(PESHGR, token);
	 		strcat(argStr, " 12 3 4 x ");
                        print_format(STRG, -1);
			strcat(argStr, " 1.00 1 0.00 0 ");
		 	print_format(9,INTG,2,FLOAT,3,FLOAT,6,FLOAT, -1);
			strcat(argStr, " nv3/2.0 %.9f ");
			cat_arg(argStr, "\\n%d %s 12 3 4 y ", PESHGR, token);
                        print_format(STRG, -1);
			strcat(argStr, " 1.00 1 0.00 0 ");
		 	print_format(10,INTG,2,FLOAT,4,FLOAT,7,FLOAT, -1);
			strcat(argStr, " nv/2.0 %.9f ");
			cat_arg(argStr, "\\n%d %s 12 3 4 z ", PESHGR, token);
                        print_format(STRG, -1);
			strcat(argStr, " 1.00 1 ");
		 	print_format(12,INTG,11,INTG,2,FLOAT,5,FLOAT,8,FLOAT, -1);
			strcat(argStr, " nv2/2.0 %.9f ");
                        print_format(0, 0);
                        strcat(argStr, "\n           ");
			print_str(1, 0);
			print_args(9, 0);
			print_fargs(2,3,6, 0);
			strcat(argStr, ", nv3/2.0");
                        strcat(argStr, "\n           ");
			print_str(1, 0);
			print_args(10, 0);
			print_fargs(2,4,7, 0);
			strcat(argStr, ", nv/2.0");
                        strcat(argStr, "\n           ");

			print_str(1, 0);
			print_args(12,11, 0);
			print_fargs(2,5,8, 0);
			strcat(argStr, ", nv2/2.0");
			timeFnum = 1;
			timeFvals[0] = 2;
			timeVnum = 2;
			timeVals[0] = 91;  /* loops is 1 */
			timeVals[1] = 12;  /* wait is True */
		  	break;
		case  7:     /* zero_all_gradients */
		    	print_code(RGRAD, token);
		    	strcat(argStr, " 11 0 3 x 0.00 0.0 0.00 0.0 0.00 0.0 \\n");
			cat_arg(argStr, " %d %s 11 0 3 y ", RGRAD, token);
                    	strcat(argStr, " 0.00 0.0 0.00 0.0 0.00 0.0 \\n");
			cat_arg(argStr, " %d %s 11 0 3 z ", RGRAD, token);
                    	strcat(argStr, " 0.00 0.0 0.00 0.0 0.00 0.0\\n\"");
		  	break;
		case  8:     /* simshapedgradpulse */
			castFlag[1] = FLOAT;
			castFlag[2] = FLOAT;
			change_code(token, token, SHGRAD, 3);
			return (1);
		case  9:     /* create_offset_list */
			castFlag[1] = INTG;
			castFlag[2] = INTG;
			castFlag[3] = INTG;
			change_code(token, token, DUMMY, 4);
			return (1);
		case  10:     /* pe_shaped3gradient */
                      //  insert_arg(zeroStr, 11); /* tag */
                      //  insert_angles(10);
                      //  insert_arg(zeroStr, 10); /* mult3 */
                      //  insert_arg(zeroStr, 9); /* mult1 */
                      //  insert_arg(fzeroStr, 9); /* step3 */
                      //  insert_arg(fzeroStr, 8); /* step1 */
                      //  pe_oblshaped_code(token, PESHGR, 18);
                      //  return (1);
                        timeIndex = 4;
                        print_code(PESHGR, token);
                        strcat(argStr, " 12 3 4 x ");
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 0.00 0 0.00 0 ");
                        print_format(4,FLOAT,5,FLOAT, -1);
                        strcat(argStr, " 0.00 0.0 0.00 0.0 ");
			cat_arg(argStr, "\\n%d %s 12 3 4 y ", PESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 0.00 0 ");
                        print_format(9,INTG,4,FLOAT,6,FLOAT,8,FLOAT, -1);
                        strcat(argStr, " 0.00 0.0 ");
			cat_arg(argStr, "\\n%d %s 12 3 4 z ", PESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 ");
                        print_format(10,INTG, -1);
                        strcat(argStr, " 0.00 0 ");
                        print_format(4,FLOAT,7,FLOAT, -1);
                        strcat(argStr, " 0.00 0.0 0.00 0.0 ");
                        print_format(0, 0);
                        strcat(argStr, "\n           ");
                        print_str(1, 0);
                        print_fargs(4,5, 0);
                        strcat(argStr, "\n           ");
                        print_str(2, 0);
                        print_args(9, 0);
                        print_fargs(4,6,8, 0);
                        strcat(argStr, "\n           ");
                        print_str(3, 0);
                        print_args(10, 0);
                        print_fargs(4,7, 0);
                        timeFnum = 1;
                        timeFvals[0] = 4;
                        timeVnum = 2;
                        timeVals[0] = 91;  /* loops is 1 */
                        timeVals[1] = 10;  /* wait is True */
		 	break;
		case  11:     /* modifyRFGroupOnOff */
		        print_code(GRPONOFF, token);
		        strcat(argStr, " 1 3 0 ");
		        print_format(1,INTG,2, INTG,3, INTG, 0);
		        print_args(1,2, 3, 0);
		 	break;
		case  12:     /* var_shapedgradient */
                        insert_arg(argPtrs[0], 1); // insert pattern name
                        insert_arg(argPtrs[0], 1);
                        insert_arg(zeroStr, 8); // STEP1
                        insert_arg(zeroStr, 10); // STEP3
                        insert_arg(zeroStr, 11); // VMULT1
                        insert_arg(zeroStr, 13); // VMULT3
                        insert_arg(zeroStr, 14); // nv3
                        insert_arg(oneStr, 15);  // nv
                        insert_arg(zeroStr, 16); // nv2
                        insert_arg(oneStr, 17);  // loops
                        phase_encode3_oblshapedgradient(token);
		 	break;
		case  13:     /* gen_shapelist_init */
                        gen_shapelist_init(token, TODEV);
                        return (1);
		case  14:     /* decshapedpulselist */
                        gen_shapedpulselist(token, DODEV);
                        return (1);
		 default:
		     	print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	     }
	 }
	 return (0);
}


int
compare_19(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd19Num; loop++)
	 {
	     if (strcmp(token, cmd_19[loop]) == 0)
             {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* shaped_2D_Vgradient */
		      timeIndex = 2;
		      print_code(SH2DVGR, token);
		      strcat(argStr, " 12 5 1 ?%c ?%s ");
		      print_format(7,INTG,8,INTG,3,INTG,4,INTG,5,INTG, -1);
		      print_format(2,FLOAT, 0);
		      print_args(6, 1, 7,8,3,4,5, 0);
		      print_fargs(2, 0);
		      timeFnum = 1;
		      timeFvals[0] = 2;
		      timeVnum = 2;
		      timeVals[0] = 7;  /* loops */
		      timeVals[1] = 8;  /* wait */
		      break;
		case  1:     /* gensim2shaped_pulse */
		        timeIndex = 3;
		     	print_code(SMSHP, token);
		        strcat(argStr, " 2 1 1 1 ");
			print_format(7,FLOAT,8,FLOAT, -1);
			print_format(INTG,STRG,5,INTG,3,FLOAT, -1);
			print_format(INTG,STRG,6,INTG,4,FLOAT, 0);
			print_fargs(7, 8, 0);
			print_args(11, 0);
			print_str(1, 0);
			print_args(5, 0);
			print_fargs(3, 0);
			print_args(12, 0);
			print_str(2, 0);
			print_args(6, 0);
			print_fargs(4, 0);
			timeFnum = 6;
			timeFvals[0] = 3;
			timeFvals[1] = 4;
			timeFvals[2] = 0;
			timeFvals[3] = 0;
			timeFvals[4] = 7;
			timeFvals[5] = 8;
			break;
		case  2:     /* gensim3shaped_pulse */
		        timeIndex = 4;
		     	print_code(SMSHP, token);
		        strcat(argStr, " 3 1 1 1 ");
			print_format(10,FLOAT,11,FLOAT, -1);
		  	print_format(INTG,STRG,7,INTG,4,FLOAT, -1);
			print_format(INTG,STRG,8,INTG,5,FLOAT, -1);
			print_format(INTG,STRG,9,INTG,6,FLOAT, 0);
			print_fargs(10, 11, 0);
			print_args(14, 0);
			print_str(1, 0);
			print_args(7, 0);
			print_fargs(4, 0);
			print_args(15, 0);
			print_str(2, 0);
			print_args(8, 0);
			print_fargs(5, 0);
			print_args(16, 0);
			print_str(3, 0);
			print_args(9, 0);
			print_fargs(6, 0);
			timeFnum = 6;
		 	timeFvals[0] = 4;
			timeFvals[1] = 5;
			timeFvals[2] = 6;
			timeFvals[3] = 0;
			timeFvals[4] = 10;
			timeFvals[5] = 11;
			break;
		case  3:     /* shaped_INC_gradient */
			timeIndex = 3;
		     	print_code(SHINCGRAD, token);
		        strcat(argStr, " 12 5 5 ?%c ?%s ");
		 	print_format(11,INTG,12,INTG,8,INTG,9,INTG,10,INTG, -1);
			print_format(3,FLOAT,4,FLOAT,5,FLOAT,6,FLOAT,7,FLOAT,0);
			print_args(1,2, 11,12,8,9,10, 0);
			print_fargs(3,4,5,6,7, 0);
			timeFnum = 1;
			timeFvals[0] = 3;
			timeVnum = 2;
			timeVals[0] = 11;  /* loop */
			timeVals[1] = 12;  /* wait */
		 	break;
		case  4:     /* obl_shaped3gradient */
                        insert_arg(oneStr, 8);  /* loops */
                        if (argNum <= 9)
                           insert_angles(8);
                        print_oblsh_code(token);
		 	break;
		case  5:     /* pe3_shaped3gradient */
                        // insert_arg(zeroStr, 15); /* tag */
                        // insert_angles(14);
                        // pe_oblshaped_code(token, PESHGR, 18);
			// return (1);
                        timeIndex = 4;
                        print_code(PESHGR, token);
                        strcat(argStr, " 12 3 4 x ");
                        print_format(STRG, -1); // loop
                        strcat(argStr, " 1.00 1 0.00 0 ");
                        print_format(11,INTG,4,FLOAT,5,FLOAT,8,FLOAT, -1);
                        strcat(argStr, " nv3/2 ");
                        print_format(FLOAT, -1);
                        cat_arg(argStr, " \\n%d %s 12 3 4 y ", PESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 0.00 0 ");
                        print_format(12,INTG,4,FLOAT,6,FLOAT,9,FLOAT, -1);
                        strcat(argStr, " nv/2 ");
                        print_format(FLOAT, -1);
                        cat_arg(argStr, " \\n%d %s 12 3 4 z ", PESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 ");
                        print_format(14,INTG,13,INTG, -1);
                        print_format(4,FLOAT,7,FLOAT,10,FLOAT, -1);
                        strcat(argStr, " nv2/2 ");
                        print_format(FLOAT, 0);
                        strcat(argStr, "\n           ");
                        print_str(1, 0);
                        print_args(11, 0);
                        print_fargs(4,5,8, 0);
                        strcat(argStr, ", nv3/2.0");
                        strcat(argStr, "\n           ");
                        print_str(2, 0);
                        print_args(12, 0);
                        print_fargs(4,6,9, 0);
                        strcat(argStr, ", nv/2.0");
                        strcat(argStr, "\n           ");

                        print_str(3, 0);
                        print_args(14,13, 0);
                        print_fargs(4,7,10, 0);
                        strcat(argStr, ", nv2/2.0");

                        timeFnum = 1;
                        timeFvals[0] = 4;
                        timeVnum = 2;
                        timeVals[0] = 91;  /* loops is 1 */
                        timeVals[1] = 14;  /* wait is True */
		 	break;
		case  6:     /* pe2_shaped3gradient */
                        // insert_arg(zeroStr, 8); /* STEP1 */
                        // insert_arg(zeroStr, 11); /* VMULT1 */
                        // insert_arg(zeroStr, 15); /* tag */
                        // insert_angles(14);
                        // pe_oblshaped_code(token, PESHGR, 18);
			// return (1);
                        
                        timeIndex = 4;
                        print_code(PESHGR, token);
                        strcat(argStr, " 12 3 4 x ");
                        print_format(STRG, -1); // loop
                        strcat(argStr, " 1.00 1 0.00 0 0.00 0 ");
                        print_format(4,FLOAT,5,FLOAT, -1);
                        strcat(argStr, " 0.00 0 0.00 0 ");
			cat_arg(argStr, "\\n%d %s 12 3 4 y ", PESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 0.00 0 ");
                        print_format(10,INTG,4,FLOAT,6,FLOAT,8,FLOAT, -1);
                        strcat(argStr, " 0.00 0.0 ");
			cat_arg(argStr, "\\n%d %s 12 3 4 z ", PESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 ");
                        print_format(12,INTG,11,INTG, -1);
                        print_format(4,FLOAT,7,FLOAT,9,FLOAT, -1);
                        strcat(argStr, " 0.00 0.0 ");
                        print_format(0, 0);
                        strcat(argStr, "\n           ");
                        print_str(1, 0);
                        print_fargs(4,5, 0);
                        strcat(argStr, "\n           ");
                        print_str(2, 0);
                        print_args(10, 0);
                        print_fargs(4,6,8, 0);
                        strcat(argStr, "\n           ");
                        print_str(3, 0);
                        print_args(12,11, 0);
                        print_fargs(4,7,9, 0);
                        timeFnum = 1;
                        timeFvals[0] = 4;
                        timeVnum = 2;
                        timeVals[0] = 91;  /* loops is 1 */
                        timeVals[1] = 12;  /* wait is True */
		 	break;
		case  7:     /* gen_shapedpulselist */
                        /*****
                        if (argNum >= 8) {
                           insert_arg("0.0", 9);
                           strcpy(tmpStr, "\"gen_shapedpulselist\"");
                           argPtrs[0] = tmpStr;
                           gen_shapedPulseWithOffset(token);
		 	}
                        ********/
                        gen_shapedpulselist(token, TODEV);
                        return (1);
		case  8:     /* modifyRFGroupSPhase */
		        print_code(GRPSPHASE, token);
		        strcat(argStr, " 1 2 1 ");
		        print_format(1, INTG, 2, INTG,3, FLOAT, 0);
		        print_args(1,2,0);
                        print_fargs(3, 0);
		 	break;
		case  9:     /* csi3_shapedgradient */
                        timeIndex = 2;
                        if (argNum <= 11)
                            insert_arg(oneStr, 12); /* wait */
                        print_code(PESHGR, token);
                        strcat(argStr, " 12 3 4 x ");
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 0.00 0 ");
                        print_format(9,INTG,2,FLOAT,3,FLOAT,6,FLOAT, -1);
                        strcat(argStr, " nv/2.0 %.9f ");
                        cat_arg(argStr, "\\n%d %s 12 3 4 y ", PESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 0.00 0 ");
                        print_format(10,INTG,2,FLOAT,4,FLOAT,7,FLOAT, -1);
                        strcat(argStr, " nv2/2.0 %.9f ");
                        cat_arg(argStr, "\\n%d %s 12 3 4 z ", PESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 ");
                        print_format(12,INTG,11,INTG,2,FLOAT,5,FLOAT,8,FLOAT, -1);
                        strcat(argStr, " nv3/2.0 %.9f ");
                        print_format(0, 0);
                        strcat(argStr, "\n           ");
                        print_str(1, 0);
                        print_args(9, 0);
                        print_fargs(2,3,6, 0);
                        strcat(argStr, ", nv/2.0");
                        strcat(argStr, "\n           ");
                        print_str(1, 0);
                        print_args(10, 0);
                        print_fargs(2,4,7, 0);
                        strcat(argStr, ", nv2/2.0");
                        strcat(argStr, "\n           ");

                        print_str(1, 0);
                        print_args(12,11, 0);
                        print_fargs(2,5,8, 0);
                        strcat(argStr, ", nv3/2.0");
                        timeFnum = 1;
                        timeFvals[0] = 2;
                        timeVnum = 2;
                        timeVals[0] = 91;  /* loops is 1 */
                        timeVals[1] = 12;  /* wait is True */
		 	break;
		case  10:     /* SweepNOffsetAcquire */
                        timeIndex = 93;
                        if (timeMode)
                            print_code(DELAY, token);
                        else
                            print_code(SPACQ, token);
                        strcat(argStr, " 1 0 1 6 rof1 %.9f rof2 %.9f %d ");
                        print_format(3,INTG, -1);
                        strcat(argStr, " at %.9f ");
                        print_format(1,FLOAT,2,FLOAT, -1);
                        strcat(argStr, " np %.9f sw %.9f ");
                        print_format(5,FLOAT, -1);
                        print_format(0);
                        strcat(argStr, ", rof1, rof2");
                        print_args(4, 3, 0);
                        strcat(argStr, ", at");
                        print_fargs(1, 2, 0);
                        strcat(argStr, ",np,sw");
                        print_fargs(5, 0);
                        timeFnum = 1;
                        timeFvals[0] = 93;
		 	break;
		case  11:     /* var_shaped3gradient */
                        insert_arg(zeroStr, 8); // STEP1
                        insert_arg(zeroStr, 10); // STEP3
                        insert_arg(zeroStr, 11); // VMULT1
                        insert_arg(zeroStr, 13); // VMULT3
                        insert_arg(zeroStr, 14); // nv3
                        insert_arg(oneStr, 15);  // nv
                        insert_arg(zeroStr, 16); // nv2
                        insert_arg(oneStr, 17);  // loops
                        phase_encode3_oblshapedgradient(token);
		 	break;
		case  12:     /* var2_shapedgradient */
                        insert_arg(argPtrs[0], 1); // insert pattern name
                        insert_arg(argPtrs[0], 1);
                        insert_arg(zeroStr, 8); // STEP1
                        insert_arg(zeroStr, 11); // VMULT1
                        insert_arg(zeroStr, 14); // nv3
                        insert_arg(oneStr, 15);  // nv
                        insert_arg(zeroStr, 16); // nv2
                        insert_arg(oneStr, 17);  // loops
                        phase_encode3_oblshapedgradient(token);
		 	break;
		case  13:     /* var3_shapedgradient */
                        insert_arg(argPtrs[0], 1); // insert pattern name
                        insert_arg(argPtrs[0], 1);
                        insert_arg(zeroStr, 14); // nv3
                        insert_arg(oneStr, 15);  // nv
                        insert_arg(zeroStr, 16); // nv2
                        insert_arg(oneStr, 17);  // loops
                        phase_encode3_oblshapedgradient(token);
		 	break;
		case  14:     /* dec2shapedpulselist */
                        gen_shapedpulselist(token, DO2DEV);
                        return (1);
		case  15:     /* dec3shapedpulselist */
                        gen_shapedpulselist(token, DO3DEV);
                        return (1);
		 default:
		     	print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	     }
	 }
	 return (0);
}


int
compare_20(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd20Num; loop++)
	 {
	     if (strcmp(token, cmd_20[loop]) == 0)
             {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* position_offset_list */
			poffset_list_code(token, LOFFSET, 7);
			return (1);
/*
		     	print_code(POFFSET, token);
			strcat(argStr, " %d 0 3 ");
			print_format(3, FLOAT, 2, FLOAT, 1, FLOAT, 0);
			print_args(5, 0);
			print_fargs(3, 2, 1, 0);
*/
			break;
		case  1:     /* pe_oblshapedgradient */
			pe_obl_code(token, OBLGRAD, 11);
			return (1);
			break;
		case  2:     /* create_rotation_list */
			create_rotation_list();
			return (1);
			break;
		case  3:     /* csi3_shaped3gradient */
                        timeIndex = 4;
                        print_code(PESHGR, token);
                        strcat(argStr, " 12 3 4 x ");
                        print_format(STRG, -1); // loop
                        strcat(argStr, " 1.00 1 0.00 0 ");
                        print_format(11,INTG,4,FLOAT,5,FLOAT,8,FLOAT, -1);
                        strcat(argStr, " nv/2 ");
                        print_format(FLOAT, -1);
                        cat_arg(argStr, " \\n%d %s 12 3 4 y ", PESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 0.00 0 ");
                        print_format(12,INTG,4,FLOAT,6,FLOAT,9,FLOAT, -1);
                        strcat(argStr, " nv2/2 ");
                        print_format(FLOAT, -1);
                        cat_arg(argStr, " \\n%d %s 12 3 4 z ", PESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 ");
                        print_format(14,INTG,13,INTG, -1);
                        print_format(4,FLOAT,7,FLOAT,10,FLOAT, -1);
                        strcat(argStr, " nv3/2 ");
                        print_format(FLOAT, 0);
                        strcat(argStr, "\n           ");
                        print_str(1, 0);
                        print_args(11, 0);
                        print_fargs(4,5,8, 0);
                        strcat(argStr, ", nv/2.0");
                        strcat(argStr, "\n           ");
                        print_str(2, 0);
                        print_args(12, 0);
                        print_fargs(4,6,9, 0);
                        strcat(argStr, ", nv2/2.0");
                        strcat(argStr, "\n           ");
                        print_str(3, 0);
                        print_args(14,13, 0);
                        print_fargs(4,7,10, 0);
                        strcat(argStr, ", nv3/2.0");

                        timeFnum = 1;
                        timeFvals[0] = 4;
                        timeVnum = 2;
                        timeVals[0] = 91;  /* loops is 1 */
                        timeVals[1] = 14;  /* wait is True */
			break;
		case  4:     /* var2_shaped3gradient */
                        insert_arg(zeroStr, 8); // STEP1
                        insert_arg(zeroStr, 11); // VMULT1
                        insert_arg(zeroStr, 14); // nv3
                        insert_arg(oneStr, 15);  // nv
                        insert_arg(zeroStr, 16); // nv2
                        insert_arg(oneStr, 17);  // loops
                        phase_encode3_oblshapedgradient(token);
			break;
		case  5:     /* var3_shaped3gradient */
                        insert_arg(zeroStr, 14); // nv3
                        insert_arg(oneStr, 15);  // nv
                        insert_arg(zeroStr, 16); // nv2
                        insert_arg(oneStr, 17);  // loops
                        phase_encode3_oblshapedgradient(token);
			break;
		case  6:     /* decshapedpulseoffset */
                        gen_shapedpulseoffset(token, DODEV);
			break;
		 default:
		     	print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	     }
	 }
	 return (0);
}


int
compare_21(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd21Num; loop++)
	 {
	     if (strcmp(token, cmd_21[loop]) == 0)
             {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* phase_encode_gradient */
                        if (argNum == 6)
                           insert_angles(7);
		     	print_code(PEGRAD, token);
		      	strcat(argStr, " 11 1 4 x 0.00 0 ");
			print_format(1,FLOAT, -1);
			strcat(argStr, " 0.00 0.0 0.00 0.0 ");
			print_format(7,FLOAT, -1);
			cat_arg(argStr, " \\n%d %s 11 1 4 y ", PEGRAD, token);
			print_format(5,INTG,2,FLOAT,4,FLOAT,6,FLOAT,8,FLOAT, -1);
			cat_arg(argStr, " \\n%d %s 11 1 4 z ", PEGRAD, token);
		      	strcat(argStr, " 0.00 0 ");
			print_format(3,FLOAT, -1);
			strcat(argStr, " 0.00 0.0 0.00 0.0 ");
			print_format(9,FLOAT, 0);
			print_fargs(1,7, 0);
			print_args(5, 0);
			print_fargs(2,4,6,8, 3, 9, 0);
		  	break;
		case  1:     /* gen_shapedpulseoffset */
                        /******
                        if (argNum >= 7) {
                           insert_arg(argPtrs[5], 7);
                           insert_arg(argPtrs[5], 9);
                           gen_shapedPulseWithOffset(token);
                        }
                        ********/
                        gen_shapedpulseoffset(token, TODEV);
		  	break;
		case  2:     /* create_rot_angle_list */
			create_rotation_list();
			return (1);
			break;
		case  3:     /* modifyRFGroupFreqList */
			castFlag[0] = INTG;
			castFlag[1] = INTG;
			castFlag[2] = NONE;
			change_code("modifyRFGroupFreqList",token, GRPFRQLIST, 3);
			return (1);
		case  4:     /* dec2shapedpulseoffset */
                        gen_shapedpulseoffset(token, DO2DEV);
			break;
		case  5:     /* dec3shapedpulseoffset */
                        gen_shapedpulseoffset(token, DO3DEV);
			break;
		 default:
		     	print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	     }
	 }
	 return (0);
}



int
compare_22(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd22Num; loop++)
	 {
	     if (strcmp(token, cmd_22[loop]) == 0)
             {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* oblique_shapedgradient */
                        if (argNum == 9) { /* nvpsg */
                            insert_angles(8);
                        }    
			timeIndex = 4;
		     	print_code(OBLSHGR, token);
	 		strcat(argStr, " 12 2 3 x ");
		 	print_format(STRG, 11,INTG, -1);
			strcat(argStr, " 0.00  0 ");
		 	print_format(4,FLOAT,5,FLOAT,8,FLOAT, -1);
			cat_arg(argStr, " \\n%d %s 12 2 3 y ", OBLSHGR, token);
		 	print_format(STRG, 11,INTG, -1);
			strcat(argStr, " 0.00  0 ");
		 	print_format(4,FLOAT,6,FLOAT,9,FLOAT, -1);
			cat_arg(argStr, " \\n%d %s 12 2 3 z ", OBLSHGR, token);
		 	print_format(STRG, 11,INTG, 12,INTG, -1);
		 	print_format(4,FLOAT,7,FLOAT,10,FLOAT, 0);
			print_str(1, 0);
			print_args(11, 0);
			print_fargs(4,5,8, 0);
			print_str(2, 0);
			print_args(11, 0);
			print_fargs(4,6,9, 0);
			print_str(3, 0);
			print_args(11,12, 0);
			print_fargs(4,7,10, 0);
			timeFnum = 1;
			timeFvals[0] = 4;
			timeVnum = 2;
			timeVals[0] = 11;  /* loop */
			timeVals[1] = 12;  /* wait */
		  	break;
		case  1:     /* phase_encode3_gradient */
		     	print_code(PEGRAD, token);
		      	strcat(argStr, " 11 1 4 x ");
			print_format(7,INTG, -1);
			print_format(1,FLOAT,4,FLOAT, 10,FLOAT,13,FLOAT, -1);
			cat_arg(argStr, " \\n%d %s 11 1 4 y ", PEGRAD, token);
			print_format(8,INTG, -1);
			print_format(2,FLOAT,5,FLOAT,11,FLOAT,14,FLOAT, -1);
			cat_arg(argStr, " \\n%d %s 11 1 4 z ", PEGRAD, token);
			print_format(9,INTG, -1);
			print_format(3,FLOAT,6,FLOAT,12,FLOAT,15,FLOAT, 0);
			print_args(7, 0);
			print_fargs(1,4,10,13, 0);
			print_args(8, 0);
			print_fargs(2,5,11,14, 0);
			print_args(9, 0);
			print_fargs(3,6,12,15,0);
		  	break;
		case  2:     /* S_position_offset_list */
			poffset_list_code(token, LOFFSET, 7);
			return (1);
		  	break;
		 default:
		     	print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	     }
	 }
	 return (0);
}


int
compare_23(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd23Num; loop++)
	 {
	     if (strcmp(token, cmd_23[loop]) == 0)
             {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* S_phase_encode_gradient */
		     	print_code(PEGRAD, token);
		      	strcat(argStr, " 11 1 4 x ");
		      	strcat(argStr, " 0.00 0 ");
			print_format(1,FLOAT, -1);
			strcat(argStr, " 0.00 0.0 0.00 0.0 ");
			print_format(7,FLOAT, -1);
			cat_arg(argStr, " \\n%d %s 11 1 4 y ", PEGRAD, token);
			print_format(5,INTG,2,FLOAT,4,FLOAT,6,FLOAT,8,FLOAT, -1);
			cat_arg(argStr, " \\n%d %s 11 1 4 z ", PEGRAD, token);
		      	strcat(argStr, " 0.00 0 ");
			print_format(3,FLOAT, -1);
			strcat(argStr, " 0.00 0.0 0.00 0.0 ");
			print_format(9,FLOAT, 0);
			print_fargs(1,7,0);
			print_args(5, 0);
			print_fargs(2,4,6,8, 3, 9, 0);
		  	break;
		case  1:     /* pe_shaped3gradient_dual */
                        timeIndex = 17;
                        print_code(DPESHGR, token);
                        strcat(argStr, " 12 3 4 x ");
                        print_format(STRG, -1); // shape pattern
                        strcat(argStr, " 1.00 1  0.00 0  0.00 0 "); // loop, wait, mult
                        print_format(17,FLOAT,7,FLOAT, -1); // width, stat
                        strcat(argStr, " 0.00 0  0.00 0 \\n"); // step, lim
                        cat_arg(argStr, "%d %s 12 3 4 x ", DPESHGR2, token);
                        print_format(STRG, -1); // shape pattern
                        strcat(argStr, " 1.00 1 0.00 0 0.00 0 "); 
                        print_format(17,FLOAT,10,FLOAT, -1); // width, stat
                        strcat(argStr, " 0.00 0  0.00 0 \\n"); // step, lim

                        cat_arg(argStr, "%d %s 12 3 4 y ", DPESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1  0.00 0 ");
                        print_format(15,INTG, -1);
                        print_format(17,FLOAT,8,FLOAT,13,FLOAT,-1); // width,stat, step
                        strcat(argStr, " 0.00 0.0 \\n"); // lim2
                        cat_arg(argStr, "%d %s 12 3 4 y ", DPESHGR2, token);
                        print_format(STRG, -1); // shape pattern
                        strcat(argStr, " 1.00 1  0.00 0 "); 
                        print_format(16,INTG, -1); // mult
                        print_format(17,FLOAT,11,FLOAT,14,FLOAT, -1);
                        strcat(argStr, " 0.00  0.0 \\n");

                        cat_arg(argStr, "%d %s 12 3 4 z ", DPESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 ");
                        print_format(18,INTG, -1); // wait
                        strcat(argStr, " 0.00 0 "); // mult
                        print_format(17,FLOAT,9,FLOAT, -1);
                        strcat(argStr, " 0.00 0  0.00 0.0 \\n");
                        cat_arg(argStr, "%d %s 12 3 4 z ", DPESHGR2, token);
                        print_format(STRG, -1); // shape pattern
                        strcat(argStr, " 1.00 1 0.00 0 "); 
                        // print_format(18,INTG, -1); // wait
                        strcat(argStr, " 0.00 0 "); // mult
                        print_format(17,FLOAT,12,FLOAT, -1);
                        strcat(argStr, " 0.00 0  0.00 0.0 ");
                        print_format(0, 0);

                        strcat(argStr, "\n           ");
                        print_str(1, 0);
                        print_fargs(17, 7, 0);
                        strcat(argStr, "\n           ");
                        print_str(4, 0);
                        print_fargs(17, 10, 0);

                        strcat(argStr, "\n           ");
                        print_str(2, 0);
                        print_args(15, 0);
                        print_fargs(17,8,13, 0);
                        strcat(argStr, "\n           ");
                        print_str(5, 0);
                        print_args(16, 0);
                        print_fargs(17,11,14, 0);

                        strcat(argStr, "\n           ");
                        print_str(3, 0);
                        print_args(18, 0);
                        print_fargs(17,9, 0);
                        strcat(argStr, "\n           ");
                        print_str(6, 0);
                        // print_args(18, 0);
                        print_fargs(17,12, 0);

                        timeFnum = 1;
                        timeFvals[0] = 17;
                        timeVnum = 2;
                        timeVals[0] = 91;  /* loops is 1 */
                        timeVals[1] = 18;  /* wait is True */

		  	break;
		 default:
		     	print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	     }
	 }
	 return (0);
}




int
compare_24(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd24Num; loop++)
	 {
	     if (strcmp(token, cmd_24[loop]) == 0)
             {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* S_oblique_shapedgradient */
			timeIndex = 2;
		     	print_code(OBLSHGR, token);
	 		strcat(argStr, " 12 2 3 x ");
		 	print_format(STRG, 9,INTG, -1);
			strcat(argStr, " 0.00  0 ");
		 	print_format(2,FLOAT,3,FLOAT,6,FLOAT, -1);
			cat_arg(argStr, " \\n%d %s 12 2 3 y ", OBLSHGR, token);
		 	print_format(STRG, 9,INTG, -1);
			strcat(argStr, " 0.00  0 ");
		 	print_format(2,FLOAT,4,FLOAT,7,FLOAT, -1);
			cat_arg(argStr, " \\n%d %s 12 2 3 z ", OBLSHGR, token);
		 	print_format(STRG, 9,INTG, 10,INTG, -1);
		 	print_format(2,FLOAT,5,FLOAT,8,FLOAT, 0);
			print_str(1, 0);
			print_args(9, 0);
			print_fargs(2,3,6, 0);
			print_str(1, 0);
			print_args(9, 0);
			print_fargs(2,4,7, 0);
			print_str(1, 0);
			print_args(9,10, 0);
			print_fargs(2,5,8, 0);
			timeFnum = 1;
			timeFvals[0] = 2;
			timeVnum = 2;
			timeVals[0] = 9;  /* loop */
			timeVals[1] = 10;  /* wait */
		  	break;
		case  1:     /* S_phase_encode3_gradient */
		     	print_code(PEGRAD, token);
		      	strcat(argStr, " 11 1 4 x ");
			print_format(7,INTG, -1);
			print_format(1,FLOAT,4,FLOAT,10,FLOAT,13,FLOAT, -1);
			cat_arg(argStr, " \\n%d %s 11 1 4 y ", PEGRAD, token);
			print_format(8,INTG, -1);
			print_format(2,FLOAT,5,FLOAT,11,FLOAT,14,FLOAT, -1);
			cat_arg(argStr, " \\n%d %s 11 1 4 z ", PEGRAD, token);
			print_format(9,INTG, -1);
			print_format(3,FLOAT,6,FLOAT,12,FLOAT,15,FLOAT, 0);
			print_args(7, 0);
			print_fargs(1,4,10,13, 0);
			print_args(8, 0);
			print_fargs(2,5,11,14, 0);
			print_args(9, 0);
			print_fargs(3,6,12,15, 0);
		  	break;
		case  2:     /* pe3_shaped3gradient_dual */
                        if (argNum < 26) {
		     	    print_dummy_code(token);
		            print_statement();
		            return (1);
                        }
                        timeIndex = 25;
                        print_code(DPESHGR, token);
                        strcat(argStr, " 12 3 4 x ");
                        print_format(STRG, -1); // shape pattern
                        strcat(argStr, " 1.00 1  0.00 0 "); // loop, wait
                        print_format(19,INTG, -1); // mult
                        print_format(25,FLOAT,7,FLOAT,13,FLOAT, -1); // width, stat,step
                        strcat(argStr, "  0.00 0 \\n"); // lim
                        cat_arg(argStr, "%d %s 12 3 4 x ", DPESHGR2, token);
                        print_format(STRG, -1); // shape pattern
                        strcat(argStr, " 1.00 1 0.00 0 ");
                        print_format(22,INTG, -1); // mult
                        print_format(25,FLOAT,10,FLOAT,16,FLOAT,-1); // width, stat,step
                        strcat(argStr, "  0.00 0 \\n"); // step, lim

                        cat_arg(argStr, "%d %s 12 3 4 y ", DPESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1  0.00 0 ");
                        print_format(20,INTG, -1);
                        print_format(25,FLOAT,8,FLOAT,14,FLOAT,-1); // width, stat,step
                        strcat(argStr, " 0.00 0.0 \\n"); // lim2
                        cat_arg(argStr, "%d %s 12 3 4 y ", DPESHGR2, token);
                        print_format(STRG, -1); // shape pattern
                        strcat(argStr, " 1.00 1  0.00 0 ");
                        print_format(23,INTG, -1); // mult
                        print_format(25,FLOAT,11,FLOAT,17,FLOAT,-1); // width, stat,step
                        strcat(argStr, " 0.00  0.0 \\n");

                        cat_arg(argStr, "%d %s 12 3 4 z ", DPESHGR, token);
                        print_format(STRG, -1);
                        strcat(argStr, " 1.00 1 ");
                        print_format(26,INTG, 21,INTG, -1); // wait, mult
                        print_format(25,FLOAT,9,FLOAT,15,FLOAT,-1); // width, stat,step
                        strcat(argStr, " 0.00 0 \\n");
                        cat_arg(argStr, "%d %s 12 3 4 z ", DPESHGR2, token);
                        print_format(STRG, -1); // shape pattern
                        strcat(argStr, " 1.00 1 ");
                        print_format(26,INTG, 24,INTG, -1); // wait, mult
                        print_format(25,FLOAT,12,FLOAT,18,FLOAT,-1); // width, stat,step
                        strcat(argStr, " 0.00 0 ");
                        print_format(0, 0);

                        strcat(argStr, "\n           ");
                        print_str(1, 0);
                        print_args(19, 0);
                        print_fargs(25, 7, 13, 0);
                        strcat(argStr, "\n           ");
                        print_str(4, 0);
                        print_args(22, 0);
                        print_fargs(25, 10, 16, 0);

                        strcat(argStr, "\n           ");
                        print_str(2, 0);
                        print_args(20, 0);
                        print_fargs(25,8,14, 0);
                        strcat(argStr, "\n           ");
                        print_str(5, 0);
                        print_args(23, 0);
                        print_fargs(25,11,17, 0);

                        strcat(argStr, "\n           ");
                        print_str(3, 0);
                        print_args(26, 21, 0);
                        print_fargs(25,9, 15, 0);
                        strcat(argStr, "\n           ");
                        print_str(6, 0);
                        print_args(26, 24, 0);
                        print_fargs(25,12, 18, 0);

                        timeFnum = 1;
                        timeFvals[0] = 25;
                        timeVnum = 2;
                        timeVals[0] = 91;  /* loops is 1 */
                        timeVals[1] = 26;  /* wait is True */

		  	break;
		default:
		     	print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	     }
	 }
	 return (0);
}

int
compare_25(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd25Num; loop++)
	 {
	     if (strcmp(token, cmd_25[loop]) == 0)
             {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* pe_oblique_shapedgradient */
                        if (argNum <= 12)
                            insert_angles(11);
			pe_obl_code(token, OBLGRAD, 15);
			return (1);
		  	break;
	        }
		print_statement();
		return (1);
	     }
	 }
	 return (0);
}


int
compare_26(char *token, FILE *fin)
{
	int	loop;

	for (loop = 0; loop < cmd26Num; loop++)
	{
	     if (strcmp(token, cmd_26[loop]) == 0)
             {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* genRFShapedPulseWithOffset */
			timeIndex = 1;
			 if (timeMode)
		     	    print_code(PULSE, token);
			 else
		     	    print_code(OFFSHP, token);
			 strcat(argStr, " 1 1 1 2 ");
			 print_format(4,FLOAT,5,FLOAT, -1);
			 print_format(INTG, STRG,3,INTG,1,FLOAT, 6,FLOAT,0);
			 print_fargs(4, 5, 0);
			 print_args(7, 0);
			 print_str(2, 0);
			 print_args(3, 0);
			 print_fargs(1, 6, 0);
			 timeFnum = 3;
			 timeFvals[0] = 1;
			 timeFvals[1] = 4;
			 timeFvals[2] = 5;
		  	break;
		case  1:     /* pe_oblique_shaped3gradient */
                        insert_angles(11);
			pe_obl_code(token, OBLGRAD, 15);
			return (1);
		  	break;
		case  2:     /* obl_imaging_shapedgradient */
                        if (argNum <= 7) {
                            insert_arg(argPtrs[0], 1);
                            insert_arg(argPtrs[0], 1);
                        }
                        if (argNum <= 9)
                            insert_angles(8);
                        print_oblsh_code(token);
		  	break;
		 default:
		     	print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	     }
	}
	return (0);
}



int
compare_27(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd27Num; loop++)
	 {
	     if (strcmp(token, cmd_27[loop]) == 0)
             {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* phase_encode_shapedgradient */
                        if (argNum <= 11)
                            insert_angles(9);
                        if (argNum < 13) {
		     	    print_dummy_code(token);
			    break;
                        }
			timeIndex = 2;
                        patStr[0] = argPtrs[0];
                        loopStr[0] = argPtrs[11];
                        waitStr[0] = zeroStr;
                        multStr[0] = zeroStr;
                        widthStr[0] = argPtrs[1];
                        lvlStr[0] = argPtrs[2];
                        stepStr[0] = fzeroStr;
                        limStr[0] = fzeroStr;
                        angStr[0] = argPtrs[8];

                        patStr[1] = argPtrs[0];
                        loopStr[1] = argPtrs[11];
                        waitStr[1] = zeroStr;
                        multStr[1] = argPtrs[6];
                        widthStr[1] = argPtrs[1];
                        lvlStr[1] = argPtrs[3];
                        stepStr[1] = argPtrs[5];
                        limStr[1] = argPtrs[7];
                        angStr[1] = argPtrs[9];

                        patStr[2] = argPtrs[0];
                        loopStr[2] = argPtrs[11];
                        waitStr[2] = argPtrs[12];
                        multStr[2] = zeroStr;
                        widthStr[2] = argPtrs[1];
                        lvlStr[2] = argPtrs[4];
                        stepStr[2] = fzeroStr;
                        limStr[2] = fzeroStr;
                        angStr[2] = argPtrs[10];
                        print_pesh_code(token);
/*
		     	print_code(PESHGR, token);
	 		strcat(argStr, " 12 3 5 x ?%s ");
		        print_format(12, INTG, -1);
	 		strcat(argStr, " 0.00  0 zero 0 ");
		        print_format(2,FLOAT,3,FLOAT,-1);
	 		strcat(argStr, " 0 0.0 ");
		        print_format(8,FLOAT,9,FLOAT,-1);
	 		cat_arg(argStr, " \\n%d %s 12 3 5 y ?%%s ",PESHGR,token);
		        print_format(12, INTG, -1);
	 		strcat(argStr, " 0.00  0 ");
		        print_format(7, INTG, -1);
		        print_format(2,FLOAT,4,FLOAT,6,FLOAT,8,FLOAT,10,FLOAT,-1);
	 		cat_arg(argStr, " \\n%d %s 12 3 5 z ?%%s ",PESHGR,token);
		        print_format(12, INTG, 13, INTG, -1);
	 		strcat(argStr, " zero 0 ");
		        print_format(2,FLOAT,5,FLOAT, -1);
	 		strcat(argStr, " 0 0.0 ");
		        print_format(8,FLOAT,11,FLOAT,0);
			print_str(1, 0);
			print_args(12, 0);
			print_fargs(2,3,8,9, 0);
			print_str(1, 0);
			print_args(12, 7, 0);
			print_fargs(2,4,6,8,10, 0);
			print_str(1, 0);
			print_args(12,13, 0);
			print_fargs(2,5,8,11, 0);
*/
			timeFnum = 1;
			timeFvals[0] = 2;
			timeVnum = 2;
			timeVals[0] = 12;
			timeVals[1] = 13;
		 	break;
		case  1:     /* pe3_oblique_shaped3gradient */
                        if (argNum <= 18)
                            insert_angles(17);
                        if (argNum < 20) {
		     	    print_dummy_code(token);
			    break;
                        }
			timeIndex = 4;
                        patStr[0] = argPtrs[0];
                        loopStr[0] = argPtrs[19];
                        waitStr[0] = zeroStr;
                        multStr[0] = argPtrs[10];
                        widthStr[0] = argPtrs[3];
                        lvlStr[0] = argPtrs[4];
                        stepStr[0] = argPtrs[7];
                        limStr[0] = argPtrs[13];
                        angStr[0] = argPtrs[16];

                        patStr[1] = argPtrs[1];
                        loopStr[1] = argPtrs[19];
                        waitStr[1] = zeroStr;
                        multStr[1] = argPtrs[11];
                        widthStr[1] = argPtrs[3];
                        lvlStr[1] = argPtrs[5];
                        stepStr[1] = argPtrs[8];
                        limStr[1] = argPtrs[14];
                        angStr[1] = argPtrs[17];

                        patStr[2] = argPtrs[2];
                        loopStr[2] = argPtrs[19];
                        waitStr[2] = argPtrs[20];
                        multStr[2] = argPtrs[12];
                        widthStr[2] = argPtrs[3];
                        lvlStr[2] = argPtrs[6];
                        stepStr[2] = argPtrs[9];
                        limStr[2] = argPtrs[15];
                        angStr[2] = argPtrs[18];
                        print_pesh_code(token);
			timeFnum = 1;
			timeFvals[0] = 4;
			timeVnum = 2;
			timeVals[0] = 20;
			timeVals[1] = 21;
		 	break;
		default:
		     	print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	     }
	 }
	 return (0);
}


int
compare_28(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd28Num; loop++)
	 {
	     if (strcmp(token, cmd_28[loop]) == 0)
             {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* phase_encode3_shapedgradient */
                        if (argNum < 18)
                           insert_angles(15);
			timeIndex = 2;
		     	print_code(PESHGR, token);
	 		strcat(argStr, " 12 3 5 x ");
		        print_format(STRG, 18,INTG, -1);
	 		strcat(argStr, " 0.00  0 ");
		        print_format(9,INTG, -1);
		        print_format(2,FLOAT,3,FLOAT,6,FLOAT,12,FLOAT,15,FLOAT,-1);
	 		cat_arg(argStr, " \\n%d %s 12 3 5 y ", PESHGR, token);
		        print_format(STRG, 18,INTG, -1);
	 		strcat(argStr, " 0.00  0 ");
		        print_format(10,INTG, -1);
		        print_format(2,FLOAT,4,FLOAT,7,FLOAT,13,FLOAT,16,FLOAT,-1);
	 		cat_arg(argStr, " \\n%d %s 12 3 5 z ", PESHGR, token);
		        print_format(STRG, 18,INTG,19,INTG, 11,INTG, -1);
		        print_format(2,FLOAT,5,FLOAT,8,FLOAT,14,FLOAT,17,FLOAT,0);
			print_str(1, 0);
			print_args(18,9, 0);
			print_fargs(2,3,6,12,15, 0);
			print_str(1, 0);
			print_args(18,10, 0);
			print_fargs(2,4,7,13,16, 0);
			print_str(1, 0);
			print_args(18,19,11, 0);
			print_fargs(2,5,8,14,17, 0);
			timeFnum = 1;
			timeFvals[0] = 2;
			timeVnum = 2;
			timeVals[0] = 18;
			timeVals[1] = 19;
		  	break;
		case  1:     /* S_pe_oblique_shaped3gradient */
			pe_obl_code(token, OBLGRAD, 15);
			return (1);
		  	break;
		default:
		     	print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	     }
	 }
	 return (0);
}


int
compare_29(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd29Num; loop++)
	 {
	     if (strcmp(token, cmd_29[loop]) == 0)
             {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* S_phase_encode_shapedgradient */
		     timeIndex = 2;
		     print_code(PESHGR, token);
	 	     strcat(argStr, " 12 3 5 x ?%s ");
		     print_format(12,INTG, -1); /* loop */
	 	     strcat(argStr, " 0.00  0 0.00 0 "); /* wait mult */
		     print_format(2,FLOAT,3,FLOAT, -1); /* width, level */
	 	     strcat(argStr, " 0.00 0.0 0.00 0.0 "); /* step, limit */
		     print_format(9,FLOAT, -1);
	 	     cat_arg(argStr, "\\n%d %s 12 3 5 y ?%%s ", PESHGR, token);
		     print_format(12,INTG, -1);
	 	     strcat(argStr, " 0.00  0 ");
		     print_format(7, INTG, -1);
		     print_format(2,FLOAT,4,FLOAT,6,FLOAT,8,FLOAT,10,FLOAT, -1);
	 	     cat_arg(argStr, "\\n%d %s 12 3 5 z ?%%s ",PESHGR,token);
		     print_format(12,INTG, 13, INTG,  -1);
	 	     strcat(argStr, " 0.00 0 ");
		     print_format(2,FLOAT,5,FLOAT, -1);
	 	     strcat(argStr, " 0.00 0.0 0.00 0.0 ");
		     print_format(11,FLOAT, 0);
		     print_args(1,12, 0);
		     print_fargs(2,3,9, 0);
		     print_args(1,12,7, 0);
		     print_fargs(2,4,6,8,10, 0);
		     print_args(1,12,13, 0);
		     print_fargs(2,5,11,0);
		     timeFnum = 1;
		     timeFvals[0] = 2;
		     timeVnum = 2;
		     timeVals[0] = 12;  /* loop */
		     timeVals[1] = 13;  /* wait */
		     break;
		default:
		     print_dummy_code(token);
		     break;
	        }
		print_statement();
		return (1);
	     }
	 }
	 return (0);
}

int
compare_30(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd30Num; loop++)
	 {
	     if (strcmp(token, cmd_30[loop]) == 0)
             {
		dump_previous();
		get_args(fin);
	        switch (loop) {
		case  0:     /* S_phase_encode3_shapedgradient */
		        timeIndex = 2;
		     	print_code(PESHGR, token);
	 		strcat(argStr, " 12 3 5 x ");
		        print_format(STRG, 18,INTG, -1);
	 		strcat(argStr, " 0.00  0 ");
		        print_format(9,INTG, -1); /* mult */
		        print_format(2,FLOAT,3,FLOAT,6,FLOAT,12,FLOAT,15,FLOAT,-1);
	 		cat_arg(argStr, " \\n%d %s 12 3 5 y ", PESHGR, token);
		        print_format(STRG, 18,INTG, -1);
	 		strcat(argStr, " 0.00  0 ");
		        print_format(10,INTG, -1);
		        print_format(2,FLOAT,4,FLOAT,7,FLOAT,13,FLOAT,16,FLOAT,-1);
	 		cat_arg(argStr, " \\n%d %s 12 3 5 z ", PESHGR, token);
		        print_format(STRG, 18,INTG,19,INTG,11,INTG, -1);
		        print_format(2,FLOAT,5,FLOAT,8,FLOAT,14,FLOAT,17,FLOAT,0);
			print_str(1, 0);
			print_args(18,9, 0);
			print_fargs(2,3,6,12,15, 0);
			print_str(1, 0);
			print_args(18,10, 0);
			print_fargs(2,4,7,13,16, 0);
			print_str(1, 0);
			print_args(18,19,11, 0);
			print_fargs(2,5,8,14,17, 0);
		        timeFnum = 1;
		        timeFvals[0] = 2;
		     	timeVnum = 2;
		     	timeVals[0] = 18;  /* loop */
		     	timeVals[1] = 19;  /* wait */
		  	break;
		default:
		        print_dummy_code(token);
			break;
	        }
		print_statement();
		return (1);
	     }
	 }
	 return (0);
}

int
compare_31(char *token, FILE *fin)
{
	int	loop;

	 for (loop = 0; loop < cmd31Num; loop++)
	 {
	     if (strcmp(token, cmd_31[loop]) == 0)
         {
            dump_previous();
            get_args(fin);
	        switch (loop) {
              case  0:     /* phase_encode3_oblshapedgradient */
		        timeFnum = 1;
		        timeFvals[0] = 4;
		     	timeVnum = 4;
		     	timeVals[0] = 17;  /* loop */
		     	timeVals[1] = 18;  /* wait */

		        timeIndex = 4;
		     	print_code(PESHGR, token);
                        strcat(argStr, " 12 3 4 x ");
		        print_format(STRG, 17,INTG, -1); // loop
                        strcat(argStr, " 0.00  0 ");
		        print_format(11,INTG, -1);  // multiple
		        print_format(4,FLOAT,5,FLOAT,8,FLOAT,14,FLOAT,-1);
                        //  width, level, step, multiple2

                        cat_arg(argStr, " \\n%d %s 12 3 4 y ", PESHGR, token);
		        print_format(STRG, 17,INTG, -1);
                        strcat(argStr, " 0.00  0 ");
		        print_format(12,INTG, -1);
		        print_format(4,FLOAT,6,FLOAT,9,FLOAT,15,FLOAT,-1);

                        cat_arg(argStr, " \\n%d %s 12 3 4 z ", PESHGR, token);
		        print_format(STRG, 17,INTG,18,INTG,13,INTG, -1);
		        print_format(4,FLOAT,7,FLOAT,10,FLOAT,16,FLOAT,0);

                        strcat(argStr, "\n           ");
                        print_str(1, 0);
                        print_args(17,11, 0);
                        print_fargs(4,5,8,14, 0);

                        strcat(argStr, "\n           ");
                        print_str(2, 0);
                        print_args(17,12, 0);
                        print_fargs(4,6,9,15, 0);

                        strcat(argStr, "\n           ");
                        print_str(3, 0);
                        print_args(17,18,13, 0);
                        print_fargs(4,7,10,16, 0);
		        timeFnum = 1;
		        timeFvals[0] = 4;
		     	timeVnum = 4;
		     	timeVals[0] = 17;  /* loop */
		     	timeVals[1] = 18;  /* wait */
                break;
              default:
		        print_dummy_code(token);
                break;
	        }
            print_statement();
            return (1);
	     }
	 }
	 return (0);
}


int
cmdcheck(FILE *fin, char *str)
{
   char            token[CMDLEN];
   int             length;

   length = strlen(str);
   if (length >= CMDLEN)
      return (0);
   strcpy(token, str);
   timeIndex = -1;
   switch (length)
   {
      case 3:
	 return (compare_3(token, fin));
	 break;

      case 4:
	 return (compare_4(token, fin));
	 break;

      case 5:
	 return (compare_5(token, fin));
	 break;

      case 6:
	 return (compare_6(token, fin));
	 break;

      case 7:
	 return (compare_7(token, fin));
	 break;

      case 8:
	 return (compare_8(token, fin));
	 break;

      case 9:
	 return (compare_9(token, fin));
	 break;

      case 10:
	 return (compare_10(token, fin));
	 break;

      case 11:
	 return (compare_11(token, fin));
	 break;

      case 12:
	 return (compare_12(token, fin));
	 break;

      case 13:
	 return (compare_13(token, fin));
	 break;

      case 14:
	 return (compare_14(token, fin));
	 break;
	    
      case 15:
	 return (compare_15(token, fin));
	 break;

      case 16:
	 return (compare_16(token, fin));
	 break;

      case 17:
	 return (compare_17(token, fin));
	 break;

      case 18:
	 return (compare_18(token, fin));
	 break;

      case 19:
	 return (compare_19(token, fin));
	 break;

      case 20:
	 return (compare_20(token, fin));
	 break;

      case 21:
	 return (compare_21(token, fin));
	 break;

      case 22:
	 return (compare_22(token, fin));
	 break;

      case 23:
	 return (compare_23(token, fin));
	 break;

      case 24:
	 return (compare_24(token, fin));
	 break;

      case 25:
	 return (compare_25(token, fin));
	 break;

      case 26:
	 return (compare_26(token, fin));
	 break;

      case 27:
	 return (compare_27(token, fin));
	 break;

      case 28:
	 return (compare_28(token, fin));
	 break;

      case 29:
	 return (compare_29(token, fin));
	 break;

      case 30:
	 return (compare_30(token, fin));
	 break;

      case 31:
	 return (compare_31(token, fin));
	 break;

      default:
	 break;
   }
   return (0);
}


int
IsCfile(char *data)
{
   int    inlen;
   char  *inptr1, *inptr2;
   char   in_name[256];

   if(!strncmp(data, "#include ", 9))
   {
       if ((inptr1= strchr(data, '<')) != NULL)
       {
           if ((inptr2= strchr(data, '>')) != NULL)
           {
               inlen = inptr2-inptr1-1;
	       if (inlen < 3)
		   return(0);
               strncpy(in_name, inptr1+1, inlen);
               in_name[inlen] = '\0';
               if(in_name[inlen-2] == '.' && in_name[inlen-1] == 'c')
	       {
   		  cleansource(in_name, 0);
		  return(1);
	       }
           }
      }
      if ((inptr1= strchr(data, '"')) != NULL)
      {
          if ((inptr2= strchr(data, '"')) != NULL)
          {
               inlen = inptr2-inptr1-1;
	       if (inlen < 3)
		   return(0);
               strncpy(in_name, inptr1+1, inlen);
               in_name[inlen] = '\0';
               if(in_name[inlen-2] == '.' && in_name[inlen-1] == 'c')
	       {
   		  cleansource(in_name, 0);
		  return(1);
	       }
          }
      }
   }
   return(0);
}


static int
is_good_name(char **name, int num)
{

	int	k;

	for (k = 0; k < num; k++)
	{
	    if (strcmp(cmd_spare, name[k]) == 0)
		return(0);
	}
	return(1);
}


static int
is_real_proc(char *s_str, int ptr1, int ptr2)
{
	char  *buf;
	int   blen, k, ret;

	buf = s_str + ptr1;
	blen = ptr2 - ptr1 + 1;
	while (blen > 0)
	{
	    if(*buf == ' ' || *buf == '\n' || *buf == '\t')
	    {
	       buf++;
	       blen--;
	    }
	    else
		break;
	}
	if (*buf == ';' || *buf == '(')
	    return(0);
	for (k = 0; k < 14; k++)
        {
            if (*buf == op_ch[k])
                return(0);
        }
	k = strlen(cmd_spare);
	if(k <= 0)
	    return(0);
	switch (k) {
	    case 3:
		ret = is_good_name(reserve_3, res_num[0]);
		break;
	    case 4:
		ret = is_good_name(reserve_4, res_num[1]);
		break;
	    case 5:
		ret = is_good_name(reserve_5, res_num[2]);
		break;
	    case 6:
		ret = is_good_name(reserve_6, res_num[3]);
		break;
	    case 7:
		ret = is_good_name(reserve_7, res_num[4]);
		break;
	    case 8:
		ret = is_good_name(reserve_8, res_num[5]);
		break;
	    default:
		ret = 1;
		break;
	}
        return(ret);
}

int is_proc_char(char c) {

    if (c >= '0' && c <= '9')
         return (1);
    if (c >= 'A' && c <= 'Z')
         return (1);
    if (c >= 'a' && c <= 'z')
         return (1);
    if (c == '_' || c == '.')
         return (1);

    return (0);
}

void
change_proc_name(char *source)
{
     char *ptr, *src;
     char ch;
     int  clen, toChange;

     src = source;
     ptr = source;
     clen = (int) strlen(cmd_name);
     while (1) {
         ptr = strstr(src, cmd_name);
         if (ptr == NULL)
             break;
         ch = ' ';
         while (src != ptr) {
             ch = *src;
             fprintf(fout, "%c", ch);
             src++;
         }
         toChange = 0;
         if (!is_proc_char(ch)) {
             ch = *(ptr + clen);
             if (!is_proc_char(ch)) {
                 toChange = 1;
                 if (timeMode)
                    fprintf(fout, "t_%s", cmd_name);    
                 else
                    fprintf(fout, "x_%s", cmd_name);    
                 src = src + clen;
             }
         }
         if (toChange == 0) {  // move forward one character
             fprintf(fout, "%c", *src);
             src++;
         }
         if (*src == '\0') 
             break;
     }
     if (*src != '\0')
         fprintf(fout, "%s", src);
}

void
parsesource(char *filename)
{
   int   ptrn, insert, exist, newline;
   int   p_flag, ptri;
   int   bslash, other_def, inbrace;
   int   braceL, braceR, Include;
   int	 ck_flag, ck_ptr, dump, cl_flag;
   int   prog, brace_ptr;
   int   k;
   char  *tmpstr;
   FILE  *fin;
   int  isExtern;

   if (debug) {
       fprintf(stderr, "parsesource file: %s \n", filename);
       fprintf(stderr, "    parse phase %d\n", phase_2);
   }
   fin = fopen(filename, "r");
   if (fin == NULL)
   {
          fprintf(stderr, "dps_ps_gen(4): can not open file %s\n", filename);
          exit(0);
   }

   for (ptri = 0; ptri < 12; ptri++)
	castFlag[ptri] = 0;
   p_flag = 0;
   cmd_spare[0] = '\0';
   ptri = 0;
   bslash = 0;
   braceL = 0;
   braceR = 0;
   Define = 0;
   Include = 0;
   inbrace = 0;
   ck_flag = 0;
   ck_ptr = 0;
   cl_flag = 0;
   dump = 0;
   prog = 0;
   brace_ptr = 0;
   line_num = 0;
   isExtern = 0;
   while (fgets(input, 1023, fin) != NULL)
   {
      line_num++;
      cur_line = str_in = input;
      Define = 0;
      Include = 0;
      other_def = 0;
      newline = 0;
      if (input[0] == '#')
      {
	 if (!prog && !phase_2 && !compilePhase)
	 {
		s_token[0] = '\0';
		ptri = 0;
		continue;
	 }
	 tmpstr = input+1;
	 while( *tmpstr == ' ' || *tmpstr == '\t' )
		tmpstr++;
	 if (strncmp(tmpstr, "define ", 7) == 0)
	 {
		if (!phase_2)
		{

	 	   add_def_list(cmd_name, &ptri, tmpstr-input+7);
		   cmd_name[0] = '\0';
		}
		Define = 1;
	 }
	 else if (strncmp(tmpstr, "include ", 8) == 0)
		Include = 1;
	 else 
	        other_def = 1;
         brace_ptr = 0;
	 p_flag = 0;
      }

      ptrn = 0;
      for (;;)
      {
      	   insert = 0;
	   if ('a' <= *str_in && *str_in <= 'z')
		insert = 1;
     	   else if ('A' <= *str_in && *str_in <= 'Z')
	 	insert = 1;
	   else if ('0' <= *str_in && *str_in <= '9')
	 	insert = 1;
	   else if (*str_in == '_' || *str_in == '$')
	 	insert = 1;
	   else if (ptrn > 0)
	   {
	 	cmd_name[ptrn] = '\0';
	 	if (!phase_2 && s_def != NULL)
		    chk_def(s_token, cmd_name, &ptri);
		ptrn = 0;
		if (!isExtern && !prog) {
                   if (strcmp(cmd_name, "extern") == 0)
			isExtern = 1;
		}
     	   }
	   if (insert)
	   {
	 	if (ptrn == 0)
		    cmdptr = ptri;
		cmd_name[ptrn++] = *str_in;
	   }
	   if (bslash)
	   {
		if (bslash == 2)
		    bslash = 0;
		else
		    bslash = 2;
	   }
		
	   switch (*str_in) {
	   case '\\':
		if (bslash)
		    bslash = 0;
	        else
		    bslash = 1;
		s_token[ptri++] = *str_in;
		cmd_name[0] = '\0';
		str_in++;
		break;
	    case '\0':
	    case '\n':
		newline = 1;
		s_token[ptri++] = '\n';
		if (Include)
	    	{
		    s_token[ptri] = '\0';
		    if(!IsCfile(s_token))
		    {
		        if (compilePhase || prog)
			   fprintf(fout, "%s", s_token);
		    }
	  	    ptri = 0;
		    s_token[0] = '\0';
	        }
		if (prog && (Define || other_def))
		    dump = 1;
		break;

	    case ',':
	    case ';':
		s_token[ptri++] = *str_in;
		cmd_name[0] = '\0';
	        if (*str_in == ';' && !prog) {
      		    if (isExtern && inbrace == 0) {
		        cmd_spare[0] = '\0';
         		isExtern = 0;
			p_flag = 0;
			ptri = 0;
		    }
		    else if (braceR > 0 && inbrace == 0) {
			if (brace_ptr > 0 && brace_ptr < ptri) {
			    exist = 0;
			    k = brace_ptr;
			    while (k < ptri -1) {
				if (s_token[k] != ' ' && s_token[k] != '\t') {
				    exist = 1;
				    break;
				}
				k++;
			    }
			    if (!exist) {
				ptri = 0;
				p_flag = 0;
				brace_ptr = 0;
		        	cmd_spare[0] = '\0';
			    }
			}
		    }
                }
		str_in++;
		if (prog || phase_2)
		{
		    dump = 1;
		    break;
		}
		if (ck_flag && p_flag == 2)
		{
		    ck_flag = 0;
		    if (!is_real_proc(s_token, ck_ptr, ptri - 1))
		    {
			p_flag = 0;
			cl_flag = 1;
		    }
		}
		if (!p_flag)
		{
		    cl_flag = 1;
		}
		break;

	    case '=':
		s_token[ptri++] = *str_in;
		str_in++;
		p_flag = 0;
		cmd_spare[0] = '\0';
		if (prog)
		    cmd_name[0] = '\0';
		break;

	    case '"':
		s_token[ptri++] = *str_in;
		str_in++;
		if (!bslash)
		{
		    eat_quot(fin, &ptri);
		}
		if (!prog && !inbrace)
		{
		    cl_flag = 1;
		}
		break;
	    case '[':
		s_token[ptri++] = *str_in;
		str_in++;
		if (!inbrace)
		    eat_bracket(fin, &ptri);
		if (prog)
		    cmd_name[0] = '\0';
/**
		if (!prog && !inbrace)
		    cl_flag = 1;
**/
		break;
	    case '(':
		s_token[ptri++] = *str_in;
		str_in++;
                brace_ptr = 0;
		if (ptrn != 0)
		    cmd_name[ptrn] = '\0';
		if (!prog)
		{
		    if (Define && !phase_2 && ((int)strlen(cmd_name) > 0) )
		    {
			if (cmdcheck(fin, cmd_name) == 1)
			{
			    ptri = 0;
			    break;
			}
		    }
		    p_flag = 1;
		    braceL++;
		    if (braceL - braceR == 1 && cmd_spare[0] == '\0')
	            {
			adjustment(s_token, &ptri);
			if ((int) strlen(cmd_name) > 0)
			{
			    strcpy(cmd_spare, cmd_name);
			    ck_ptr = 0;
			}
		    }
		    break;
		}
		if (phase_2)
		{
		    exist = 0;
		    p_rout = s_rout;
		    while (p_rout != NULL)
		    {
			if (strcmp(p_rout->name, cmd_name) == 0)
			{
			     exist = 1;
			     break;
			}
	 		if (p_rout->next != NULL)
			     p_rout = p_rout->next;
			else
			     break;
	       	    }
		    if (exist)
			changename(s_token, &ptri);
		    inbrace++;
	    	}
		else if ((int) strlen(cmd_name) > 0)
		{
		    if (cmdcheck(fin, cmd_name) == 1)
			ptri = 0;
		    else
			inbrace++;
		}
		cmd_name[0] = '\0';
	    
		break;

	    case ')':
		s_token[ptri++] = *str_in;
		str_in++;
		if (!prog)
		{
		    braceR++;
		    if (braceR == braceL && p_flag)
		    {
   			brace_ptr = ptri;
		        p_flag = 2;
			if (ck_ptr <= 0)
                        {
			   ck_flag = 1;
			   ck_ptr = ptri;
			}
		    }
		    else
		    {
			if (p_flag)
			    p_flag = 1;
		    }
		}
		else
		    inbrace--;
		break;

	    case '{':
		s_token[ptri++] = *str_in;
		str_in++;
         	brace_ptr = 0;
		if (strcmp(cmd_name, "struct") == 0) 
			cmd_spare[0] = '\0';
		else if (prog || phase_2)
		{
		    prog++;
		    break;
		}
		if (p_flag == 2) /*  it is a procedure */
		{
		    if ((int) strlen(cmd_spare) > 0)
		    {
			strcpy(cmd_name, cmd_spare);
			prog = 1;
			exist = 0;
			p_rout = s_rout;
			while (p_rout != NULL)
			{
			    if (strcmp(p_rout->name, cmd_name) == 0)
			    {
				exist = 1;
				break;
			    }
			    if (p_rout->next != NULL)
		 		p_rout = p_rout->next;
			    else
				break;
			}
			if (!exist)
			{
			    t_rout = (struct routine *)
			    malloc(sizeof(struct routine));
			    strcpy(t_rout->name, cmd_name);
			    t_rout->next = NULL;
			    if (s_rout == NULL)
				s_rout = t_rout;
			    else
				p_rout->next = t_rout;
			}
			s_token[ptri] = '\0';
			change_proc_name(s_token);
                        /******
			changename(s_token, &ptri);
			s_token[ptri++] = '\n';
			s_token[ptri] = '\0';
			fprintf(fout, "%s", s_token);
                        ******/
			p_flag = 0;
		    }
		    ptri = 0;
		    s_token[0] = '\0';
		    braceL = 0;
		    braceR = 0;
		    inbrace = 0;
         	    isExtern = 0;
		    cmd_name[0] = '\0';
		}
		break;
	    case '}':
		s_token[ptri++] = *str_in;
		str_in++;
		if (!prog || phase_2)
		    break;
		if (prog)
		{
		    if (*str_in == '\n')
		        s_token[ptri++] = '\n';
		    else
		        s_token[ptri++] = ' ';
/*
		    s_token[ptri++] = '\n';
*/
		    s_token[ptri] = '\0';
		    fprintf(fout, "%s", s_token);
		}
		ptri = 0;
		ptrn = 0;
		s_token[0] = '\0';
		braceL = 0;
		braceR = 0;
		p_flag = 0;
		cmd_spare[0] = '\0';
		prog--;
         	isExtern = 0;
		break;
	    case ' ':
	    case '\t':
		s_token[ptri++] = *str_in;
		str_in++;
		break;
	    case '\'':
		s_token[ptri++] = *str_in;
                str_in++;
                if (!bslash)
                {
                    eat_singleQuote(fin, &ptri);
                }
		break;
	    default:
		s_token[ptri++] = *str_in;
		str_in++;
		if (!insert && prog)
		    cmd_name[0] = '\0';
		break;
	   } /* end of switch */
	   if (dump)
	   {
		s_token[ptri] = '\0';
		fprintf(fout, "%s", s_token);
		ptri = 0;
         	brace_ptr = 0;
		s_token[0] = '\0';
		dump = 0;
	   }
	   if (cl_flag)
	   {
		cl_flag = 0;
		s_token[ptri] = '\0';
		if (compilePhase)
		    fprintf(fout, "%s", s_token);
		ptri = 0;
		ptrn = 0;
         	brace_ptr = 0;
		s_token[0] = '\0';
		cmd_spare[0] = '\0';
	   }
	   if (newline)
		break;
      }  /* for loop */
   } /* while loop */
   fclose(fin);
   if (debug)
       fprintf(stderr, "parsesource file  done \n");
}


void
eat_quot(FILE *fin, int *ptr)
{
	int	 index, slash;

	index = *ptr;
	slash = 0;
	for(;;)
	{
	     s_token[index++] = *str_in;
	     switch (*str_in) {
		case '\\' :
			if(slash)
			    slash = 0;
			else
			    slash = 1;
			break;
		case '\0' :   
		case '\n' :   
	    		if (fgets(input, 1023, fin) == NULL)
	    		{
                            if (debug)
                            {
	       		       fprintf(stderr, " syntax error in line %d, -%s-\n",line_num, s_token);
	       		       fprintf(stderr, " abort!\n");
                            }
	       		    exit(0);
	    		}
			str_in = input;
			slash = 0;
		        break;
		case '"' :   
			if (!slash)
			{
			    *ptr = index;
			    str_in++;
			    return;
			}
			break;
		default :
			slash = 0;
			break;
	    }
	    str_in++;
	}
}

void eat_singleQuote(FILE *fin, int *ptr)
{
        int      index, slash;

        index = *ptr;
        slash = 0;
        for(;;)
        {
             s_token[index++] = *str_in;
             switch (*str_in) {
                case '\\' :
                        if(slash)
                            slash = 0;
                        else
                            slash = 1;
                        break;
                case '\0' :
                case '\n' :
                        if (fgets(input, 1023, fin) == NULL)
                        {
                            if (debug)
                            {
                               fprintf(stderr, " syntax error in line %d, -%s-\n", line_num,s_token);
                               fprintf(stderr, " abort!\n");
                            }
                            exit(0);
                        }
                        str_in = input;
                        slash = 0;
                        break;
                case '\'' :
                        if (!slash)
                        {
                            *ptr = index;
                            str_in++;
                            return;
                        }
                        break;
                default :
                        slash = 0;
                        break;
            }
            str_in++;
        }
}


void
eat_bracket(FILE *fin, int *ptr)
{
	int	 index, bracket, slash, quot;

	index = *ptr;
	bracket = 0;
	slash = 0;
	quot = 0;
	for(;;)
	{
	     s_token[index++] = *str_in;
	     switch (*str_in) {
		case '\\' :
			if(slash)
			    slash = 0;
			else
			    slash = 1;
			break;
		case '"' :
			if(slash)
			    slash = 0;
			else
			{
			    if (quot)
			      quot = 0;
			    else
			      quot = 1;
			}
			break;
		case '\0' :   
		case '\n' :   
	    		if (fgets(input, 1023, fin) == NULL)
	    		{
                            if (debug)
                            {
	       		       fprintf(stderr, " syntax error in line %d, %s\n",line_num, s_token);
	       		       fprintf(stderr, " abort!\n");
                            }
	       		    exit(0);
	    		}
			str_in = input;
			slash = 0;
		        break;
		case '[' :   
			if (!quot)
			    bracket--;
			slash = 0;
		        break;
		case ']' :   
			if (!quot)
			{
			    bracket++;
			    if (bracket == 1)
			    {
				*ptr = index;
				str_in++;
				return;
			    }
			}
			slash = 0;
			break;
		default :
			slash = 0;
			break;
	    }
	    str_in++;
	}
}



/*  to comsume the comment and set the string pointer behind the comment  */
int 
remove_comment()
{
   for (;;)
   {
      if (*str_in == '\n' || *str_in == '\0')
      {
	 return (0);
      }
      if (*str_in++ == '*')
      {
	 if (*str_in == '/')
	 {
	    str_in++;
	    return (1);
	 }
      }
   }
}


/*   add "X_"  to the subroutine name  */
void
changename(char *source, int *index)
{
   int             count;
   int             pn;
   int		ptrx, ptrs, ptrt;

   count = 0;
   pn = strlen(cmd_name);
   ptrt = pn - 1;
   ptrx = *index + 1;
   ptrs = *index - 1;
   *index = *index + 2;
   while (ptrs >= 0)
   {
      source[ptrx] = source[ptrs];
      ptrx--;
      if (source[ptrs] == cmd_name[ptrt])
      {
	 count++;
	 ptrt--;
      }
      else
      {
	 count = 0;
	 ptrt = pn - 1;
      }
      if (count == pn)
      {
	 source[ptrx--] = '_';
	 if (timeMode)
	    source[ptrx] = 't';
	 else
	    source[ptrx] = 'x';
	 return;
      }
      ptrs--;
   }
}


/*   to retrieve the arguments of the PSG function  */
void
get_args(FILE *fin)
{
   int  argptr;
   int  pl, pr;
   int	bl, br;
   int  quot, slash;
   char tmp_input[2048];

   tmp_input[0] = '\0';

   argNum = 0;
   argptr = 0;
   argPtrs[0] = &argArray[0];
   timeFnum = 0;
   timeVnum = 0;
   rtCode = 0;
   
   pr = 0;
   pl = 0;
   br = 0;
   bl = 0;
   quot = 0;
   slash = 0;
   for (;;)
   {
      switch (*str_in)
      {
	 case '(':
	    if (!quot)
		pl++;
	    argArray[argptr++] = *str_in;
	    str_in++;
	    slash = 0;
	    break;
	 case ')':
	    if (!quot)
		pr++;
	    if (pl >= pr)
	    {
		argArray[argptr++] = *str_in;
		str_in++;
	    }
	    else
	    {
		argArray[argptr++] = '\0';
		argNum++;
		// argPtrs[argNum] = argptr;
		argPtrs[argNum] = &argArray[0] + argptr;
		str_in++;
		return;
	    }
	    slash = 0;
	    break;
	 case ',':
	    if (pl > pr || bl != br || quot)
	    {
		argArray[argptr++] = *str_in;
		str_in++;
	    }
	    else
	    {
		argArray[argptr++] = '\0';
		argNum++;
		// argPtrs[argNum] = argptr;
		argPtrs[argNum] = &argArray[0] + argptr;
		str_in++;
	    }
	    slash = 0;
	    break;
	 case '\0':
	 case '\n':
            if ( tmp_input[0] == '\0' )
               strcpy(tmp_input,input);
	    if (fgets(input, 1023, fin) == NULL)
	    {
		fprintf(stderr, " unexpected end-of-line at %s\n", tmp_input);
		fprintf(stderr, " abort!\n");
		exit(0);
	    }
	    str_in = input;
	    slash = 0;
	    break;
	 case '\t':
	 case ' ':
	    if (quot)
	    	argArray[argptr++] = *str_in;
	    str_in++;
	    slash = 0;
	    break;

	 case '[':
	    if (!quot)
	    	bl++;
	    argArray[argptr++] = *str_in;
	    str_in++;
	    slash = 0;
	    break;
	 case ']':
	    if (!quot)
	    	br++;
	    argArray[argptr++] = *str_in;
	    str_in++;
	    slash = 0;
	    break;
	 case '"':
	    if (!slash)
	    {
	        if(!quot)
		    quot = 1;
		else
		    quot = 0;
	    }
	    argArray[argptr++] = *str_in;
	    str_in++;
	    slash = 0;
	    break;
	 case '\\':
	    if (slash)
		slash = 0;
	    else
		slash = 1;
	    argArray[argptr++] = *str_in;
	    str_in++;
	    break;
	 default:
	    argArray[argptr++] = *str_in;
	    str_in++;
	    slash = 0;
	    break;
      }
   }
}



void
adjustment(char *source, int *ptr)
{
   int             loop,
                   index;

   index = *ptr - 1;
   while (index >= 0)
   {
      if (source[index] == ';')
         break;
      else
         index--;
   }
   if (index >= 0)
   {
      if (compilePhase)
      {
         strncpy(ddstr, source, index + 1);
         ddstr[index+1] = '\0';
         fprintf(fout, "%s", ddstr);
      }
      loop = 0;
      index++;
      while (index < *ptr)
         source[loop++] = source[index++];
      *ptr = loop;
   }
}


/* collect all defined variables */
void
add_def_list(char *token, int *index, int len)
{
	int  n, insert, ptr;
	
	ptr = *index;
	for (n = 0; n < len; n++)
	   s_token[ptr++] = *str_in++;
	while (*str_in != '\0' && (*str_in == ' ' || *str_in == '\t'))
	   s_token[ptr++] = *str_in++;
	n = 0;
	for(;;)
	{
      	   insert = 0;
      	   if ('a' <= *str_in && *str_in <= 'z')
	 	insert = 1;
	   else if ('A' <= *str_in && *str_in <= 'Z')
	 	insert = 1;
	   else if ('0' <= *str_in && *str_in <= '9')
	 	insert = 1;
	   else if (*str_in == '_')
	 	insert = 1;
           if (insert)
           {
	  	if (n == 0)
		     cmdptr = ptr;
	 	token[n++] = *str_in;
      	   }
	   s_token[ptr++] = *str_in;
	   str_in++;
           if (!insert)
      	   {
		*index = ptr;
	 	token[n] = '\0';
		if ( n <= 0)
		    return;
		t_def = (struct routine *)
			malloc(sizeof(struct routine));
		strcpy(t_def->name, token);
		t_def->next = NULL;
		if (s_def == NULL)
			s_def = t_def;
		else
			p_def->next = t_def;
		p_def = t_def;
		changename(s_token, index);
		token[0] = '\0';
		return;
           }
	}
}


/* check variables if it is a defined var then change it's name */
void
chk_def(char *source, char *token, int *ptr)
{
	int  exist;
	struct routine *pp_def;

	exist = 0;
	pp_def = s_def;
	while (pp_def != NULL)
	{
	  if (strcmp(pp_def->name, token) == 0)
	  {
	     exist = 1;
	     break;
	  }
          pp_def = pp_def->next;
       }
       if (exist)
	  changename(source, ptr);
}



void
cleansource(char *filename, int exit_flag)
{
   int   	newline, bslash;
   int   	comment, ptri, Include;
   char         source[MAXPATH];
   char 	*tmpstr;
   FILE         *fin;

   fin = fopen(filename, "r");
   if (fin == NULL)
   {
      sprintf(source, "%s/psglib/%s", getenv("vnmruser"), filename);
      if((fin = fopen(source, "r")) == NULL)
      {
          sprintf(source, "%s/psg/%s", getenv("vnmrsystem"), filename);
          fin = fopen(source, "r");
      }
      if (fin == NULL)
      {
          fprintf(stderr, " dps_ps_gen(5): can not open file %s\n", filename);
	  if (exit_flag)
		exit(0);
          return;
      }
   }

   comment = 0;
   ptri = 0;
   bslash = 0;
   Define = 0;
   while (fgets(input, 1023, fin) != NULL)
   {
      str_in = input;
      Define = 0;
      newline = 0;
      Include = 0;
      if (input[0] == '#' && !comment)
      {
	 tmpstr = input+1;
	 while( *tmpstr == ' ' || *tmpstr == '\t' )
		tmpstr++;
	 if (strncmp(tmpstr, "define ", 7) == 0)
		Define = 1;
	 else if (strncmp(tmpstr, "include ", 8) == 0)
		Include = 1;
      }
      if (comment)
      {
	 if (!remove_comment())
	    continue;
	 else
	    comment = 0;
      }

      for (;;)
      {
	   switch (*str_in) {
	   case '/':
		bslash = 0;
		if (*(str_in + 1) == '*')
	   	{
		    comment = 1;
		    str_in = str_in + 2;
		    if (!remove_comment())
		 	 newline = 1;
		    else
			 comment = 0;
	    	}
		else
		{
		    s_token[ptri++] = *str_in;
	            str_in++;
	    	}
	    	break;
	   case '\\':
		if (Define && !bslash && (*(str_in+1) == '\n' || *(str_in+1) == '\0'))
	    	{
		    if (fgets(input, 1023, fin) == NULL)
		    {
                         if (debug)
                         {
	 		    fprintf(stderr, " syntax error in line %d, '%s'\n",line_num, s_token);
	 		    fprintf(stderr, " abort!\n");
                         }
	 		 exit(0);
	    	    }
		    str_in = input;	
		    break;
		}
		else if (bslash)
		    bslash = 0;
	        else
		    bslash = 1;
		s_token[ptri++] = *str_in;
		str_in++;
		break;
	    case '\0':
	    case '\n':
		newline = 1;
		s_token[ptri++] = '\n';
		s_token[ptri] = '\0';
		if (Include)
	    	{
		    if(!IsCfile(s_token))
			   fprintf(fout, "%s", s_token);
	  	    ptri = 0;
		    s_token[0] = '\0';
		    break;
	        }
		fprintf(fout, "%s", s_token);
	  	ptri = 0;
		s_token[0] = '\0';
		break;
	    default:
		bslash = 0;
		s_token[ptri++] = *str_in;
		str_in++;
		break;
	   } /* end of switch */
	   if (newline)
		break;
      }		/* for loop */
   }		/* while loop */
   fclose(fin);
}


int
Is_zero_arg(char *para)
{
	int	len, nlen, count;
	char    *np;

	len = strlen(para);
	count = 0;
	np = para;
	nlen = 0;
	while (count < len)
	{
	    if (*para == '(')
	    {
		np = para + 1;
		nlen = 0;
	    }
	    else if (*para != ')')
	        nlen++;
	    count++;
	    para++;
	}
	if (nlen <= 0)
	    return(0);
	if (nlen >= 4)
	{
	    if (strncmp(np, "NULL", 4) == 0)
	        return(1);
	}
	count = 0;
	while (count < nlen)
	{
	    if (*np != '0' && *np != '.' && *np != ' ')  /* value may be 0 or 0.0 */
		return(0);
	    np++;
	    count++;
	}
	return(1);
}


int
G_delay()
{
	int    pindex;
	char   *param;

	pindex = 1;
	while (pindex < argNum)
	{
	      // param = &argArray[0] + argPtrs[pindex-1];
	      param = argPtrs[pindex-1];
	      if (strcmp(param, "DELAY_TIME") == 0)
	      {
		   timeIndex = pindex + 1;
		   print_code(DELAY, "G_Delay");
		   strcat(argStr, " 1 0 1 ");
		   print_format(pindex+1, FLOAT, 0);
		   print_fargs(pindex+1, 0);
		   timeFnum = 1;
		   timeFvals[0] = pindex + 1;;
		   return(1);
	      }
	      pindex += 2;
	}
	return(0);
}


int
G_pulse()
{
	int    pindex, pdev, pw, ph, ps1, ps2;
	char   *param;

	pindex = 1;
	pdev = 0;
	pw = ph = 0;
	ps1 = ps2 = 0;
	while (pindex < argNum)
	{
	      // param = &argArray[0] + argPtrs[pindex-1];
	      param = argPtrs[pindex-1];
	      if (strcmp(param, "PULSE_WIDTH") == 0)
		    pw = pindex + 1;
	      else if (strcmp(param, "PULSE_DEVICE") == 0)
		    pdev = pindex + 1;
	      else if (strcmp(param, "PULSE_PHASE") == 0)
		    ph = pindex + 1;
	      else if (strcmp(param, "PULSE_PRE_ROFF") == 0)
		    ps1 = pindex + 1;
	      else if (strcmp(param, "PULSE_POST_ROFF") == 0)
		    ps2 = pindex + 1;
	      pindex += 2;
	}
	if (!pdev || !pw) 
	      return(0);
	timeIndex = pw;
	print_code(PULSE, "G_Pulse");
	strcat(argStr, " 1 0 1 1 ");
	timeFnum = 3;
	timeFvals[0] = pw;
	if (ps1)
	{
	    // param = &argArray[0] + argPtrs[ps1 - 1];
	    param = argPtrs[ps1 - 1];
	    cat_arg(argStr, " %s %%.9f ", param);
	    timeFvals[1] = ps1;
	}
	else
	{
	    strcat(argStr, " rof1 %.9f ");
	    timeFvals[1] = 90;
	}
	if (ps2)
	{
	    // param = &argArray[0] + argPtrs[ps2 - 1];
	    param = argPtrs[ps2 - 1];
	    cat_arg(argStr, "%s %%.9f ", param);
	    timeFvals[2] = ps2;
	}
	else
	{
	    strcat(argStr, "rof2 %.9f ");
	    timeFvals[2] = 91;
	}
	strcat(argStr, "%d ");
	if (ph)
	    print_format( ph, INTG, -1);
	else
	    strcat(argStr, " 0 %d ");
	print_format(pw, FLOAT, 0);
	if (ps1)
	{
	    // param = &argArray[0] + argPtrs[ps1 - 1];
	    param = argPtrs[ps1 - 1];
	    cat_arg(argStr, ", %s, ", param);
	}
	else
	    strcat(argStr, ", rof1, ");
	if (ps2)
	{
	    // param = &argArray[0] + argPtrs[ps2 - 1];
	    param = argPtrs[ps2 - 1];
	    cat_arg(argStr, "%s ", param);
	}
	else
	    strcat(argStr, "rof2 ");
	print_args(pdev, 0);
	if (ph)
	    print_args(ph, 0);
	else
	    strcat(argStr, ", 0");
	print_fargs(pw, 0);
	return(1);
}


int
G_power()
{
	int    pindex, pdev, ph;
	char   *param;

	pindex = 1;
	pdev = 0;
	ph = 0;
	while (pindex < argNum)
	{
	      // param = &argArray[0] + argPtrs[pindex-1];
	      param = argPtrs[pindex-1];
	      if (strcmp(param, "POWER_DEVICE") == 0)
		    pdev = pindex + 1;
	      else if (strcmp(param, "POWER_VALUE") == 0)
		    ph = pindex + 1;
	      pindex += 2;
	}
	if (!pdev || !ph)
	      return(0);
	print_code(POWER, "G_Power");
	print_format(INTG, -1);
	strcat(argStr, " 1 0 ");
	print_format(ph,INTG, 0);
	print_args(pdev, ph, 0);
	return(1);
}

int
G_offset()
{
	int    pindex, pdev, poffset;
	char   *param;

	pindex = 1;
	pdev = 0;
	poffset = 0;
	while (pindex < argNum)
	{
	      // param = &argArray[0] + argPtrs[pindex-1];
	      param = argPtrs[pindex-1];
	      if (strcmp(param, "OFFSET_DEVICE") == 0)
		    pdev = pindex + 1;
	      else if (strcmp(param, "OFFSET_FREQ") == 0)
		    poffset = pindex + 1;
	      pindex += 2;
	}
	if (!pdev || !poffset)
	      return(0);
	print_code(OFFSET, "G_Offset");
	print_format(INTG, -1);
	strcat(argStr, " 0 1 ");
	print_format(poffset, FLOAT, 0);
	print_args(pdev, 0);
	print_fargs(poffset, 0);
	return(1);
}


int
G_rtdelay()
{
	int    pindex, code, base, count, baseformat;
	int    vnum, fnum;
	char   *param;

	pindex = 1;
	code = 0;
	base = 0;
	count = 0;
        vnum = fnum = 0;
	baseformat = INTG;
	while (pindex < argNum)
	{
	      // param = &argArray[0] + argPtrs[pindex-1];
	      param = argPtrs[pindex-1];
	      if (strcmp(param, "RTDELAY_MODE") == 0)
	      {
	      	  // param = &argArray[0] + argPtrs[pindex];
	      	  param = argPtrs[pindex];
	          if (strcmp(param, "TCNT") == 0)
		      code = VDELAY;
		  else
		  {
	              if (strcmp(param, "SET_INITINCR") == 0)
		      {
		      	    code = INITDLY;
			    baseformat = FLOAT;
			    vnum = 1;
			    fnum = 1;
		      }
	              else if (strcmp(param, "SET_RTINCR") == 0)
		      	    code = INCDLY;
		  }
	      }
	      else if (strcmp(param, "RTDELAY_TBASE") == 0)
		    base = pindex + 1;
	      else if (strcmp(param, "RTDELAY_COUNT") == 0)
		    count = pindex + 1;
	      else if (strcmp(param, "RTDELAY_INCR") == 0)
		    count = pindex + 1;
	      pindex += 2;
	}
	if (code <= 0)
	      return(0);
	print_code(code, "G_RTdelay" );
	cat_arg(argStr, " 1 %d %d ", vnum, fnum);
	print_format(base, INTG, count, baseformat, 0);
	print_args(base, 0);
	if (baseformat == FLOAT)
	   print_fargs(count, 0);
	else
	   print_args(count, 0);
	if (timeMode)
	{
	   if (code == INITDLY)
	   {
		timeFnum = 1;
		timeVnum = 1;
		timeFvals[0] = 1;
		timeVals[0] = 2;
	   }
	   else
	   {
		timeVnum = 2;
		timeVals[0] = 1;
		timeVals[1] = 2;
	   }
	}
	return(1);
}

int
G_sync()
{
	int    pindex, base, which;
	char   *param;

	base = 0;
	which = 0;
	// param = &argArray[0] + argPtrs[0];
	param = argPtrs[0];
	if (strcmp(param, "HS_Rotor") == 0)
	    which = ROTORS;
	else if (strcmp(param, "Ext_Trig") == 0)
	    which = XGATE;
	if (which == 0)
	    return(0);
	pindex = 2;
	while (pindex < argNum)
	{
	      // param = &argArray[0] + argPtrs[pindex-1];
	      param = argPtrs[pindex-1];
	      if (strcmp(param, "SET_DBVALUE") == 0)
		    base = pindex + 1;
	      else if (strcmp(param, "SET_RTPARAM") == 0)
		    base = pindex + 1;
	      if (base > 0)
		    break;
	      pindex++;
	}
	if (base == 0)
	    return(0);
	print_code(which, "sync_on_event");
	if (which == ROTORS)
	{
		strcat(argStr, " 1 1 0 ");
		print_format(base, INTG, 0);
		timeVnum = 1;
		timeVals[0] = base;
	}
	else
	{
		strcat(argStr, " 1 0 1 ");
		print_format(base, FLOAT, 0);
	}
	print_fargs(base, 0);
	return(1);
}


int
G_simpulse()
{
	int    pindex, n, rof1, rof2;
	int	phase_num, device_num, width_num;
	int	phase[6], device[6], width[6];
	char   *param;

	pindex = 1;
	rof1 = 0;
	rof2 = 0;
	device_num = 5;
        phase_num = 0;
	while (device_num >= 0)
	{
		phase[device_num] = 0;
		device[device_num] = 0;
		width[device_num] = 0;
		device_num--;
	}
	device_num = 0;
	while (pindex < argNum)
	{
	      // param = &argArray[0] + argPtrs[pindex-1];
	      param = argPtrs[pindex-1];
	      if (strcmp(param, "PULSE_PHASE") == 0)
	      {
		  phase_num = 0;
		  pindex++;
		  while (pindex < argNum && phase_num < 6)
		  {
	      		// param = &argArray[0] + argPtrs[pindex-1];
	      		param = argPtrs[pindex-1];
			if (Is_zero_arg(param))
			     break;
			phase[phase_num] = pindex;
			pindex++;
			phase_num++;
		  }
	      }
	      else if (strcmp(param, "PULSE_DEVICE") == 0)
	      {
		  device_num = 0;
		  pindex++;
		  while (pindex < argNum && device_num < 6)
		  {
	      		// param = &argArray[0] + argPtrs[pindex-1];
	      		param = argPtrs[pindex-1];
			if (Is_zero_arg(param))
			     break;
			device[device_num] = pindex;
			pindex++;
			device_num++;
		  }
	      }
	      else if (strcmp(param, "PULSE_WIDTH") == 0)
	      {
		  if (device_num != 0)
		        width_num = device_num;
		  else if (phase_num != 0)
			width_num = phase_num;
		  else
		  {
			fprintf(stdout,"G_Simpulse: PULSE_WIDTH must be used after\n");
                 	fprintf(stdout,"            PULSE_PHASE or PULSE_DEVICE\n\n");
			exit(0);
		  }
		  pindex++;
		  n = 0;
		  while (pindex < argNum && width_num > 0)
		  {
			width[n++] = pindex;
			pindex++;
			width_num--;
		  }
		  if (width[0] > 0)
		        timeIndex = width[0];
	      }
	      else if (strcmp(param, "PULSE_PRE_ROFF") == 0)
	      {
		  pindex++;
		   rof1 = pindex;
	      }
	      else if (strcmp(param, "PULSE_POST_ROFF") == 0)
	      {
		  pindex++;
		   rof2 = pindex;
	      }
	      pindex++;
	}
	if (device_num <= 0)
	      return(0);
	print_code(SMPUL, "G_Simpulse");
	cat_arg(argStr, " %d 0 1 1 ", device_num);
	if (rof1)
	    print_format(rof1, FLOAT, -1);
	else
	    strcat(argStr, " rof1 %.9f ");
	if (rof2)
	    print_format(rof2, FLOAT, -1);
	else
	    strcat(argStr, " rof2 %.9f ");

	n = 0;
	while (n < device_num)
	{
	    strcat(argStr, " %d ");
	    if (phase[n] > 0)
		print_format(phase[n], INTG, -1);
	    else
	        strcat(argStr, " 0 %d ");
	    if (width[n] > 0)
		print_format(width[n], FLOAT, -1);
	    else
	        strcat(argStr, " 0.0 %.9f ");
	    n++;
	}
	strcat(argStr, " \\n\"");
	if (rof1)
	    print_fargs(rof1, 0);
	else
	    strcat(argStr, ", rof1");
	if (rof2)
	    print_fargs(rof2, 0);
	else
	    strcat(argStr, ", rof2");

	n = 0;
	while (n < device_num)
	{
	    print_args(device[n], 0);
	    if (phase[n] > 0)
	         print_args(phase[n], 0);
	    else
	         strcat(argStr, ", 0 ");
	    if (width[n] > 0)
	         print_fargs(width[n], 0);
	    else
	         strcat(argStr, ", 0.0 ");
	    n++;
	}
	if (timeMode)
	{
	    timeFnum = 6;
	    n = 0;
	    while (n < device_num)
	    {
		timeFvals[n] = width[n];
		n++;
	    }
	    while (n < device_num)
	    {
		timeFvals[n] = 0;
		n++;
	    }
	    if (rof1)
		timeFvals[4] = rof1;
	    else
		timeFvals[4] = 90;
	    if (rof2)
		timeFvals[5] = rof2;
	    else
		timeFvals[5] = 91;
	}
	return(1);
}


int
query_device(int num)
{
	char   *par;
	int	dev;

	dev = 0;
	// par = &argArray[0] + argPtrs[num - 1];
	par = argPtrs[num - 1];
	if (strcmp(par, "\"obs\"") == 0)
		dev = TODEV;
	else if (strcmp(par, "\"dec\"") == 0)
		dev = DODEV;
	else if (strcmp(par, "\"dec2\"") == 0)
		dev = DO2DEV;
	else if (strcmp(par, "\"dec3\"") == 0)
		dev = DO3DEV;
	else if (strcmp(par, "\"dec4\"") == 0)
		dev = DO4DEV;
	else if (strcmp(par, "\"dec5\"") == 0)
		dev = DO5DEV;
	return(dev);
}


int
translate_show()
{
	char  *name, *par;
	int   len, dev;

	par = &argArray[0];
	name = clean_print(par, 0);
	len = (int) strlen(name);
	switch (len) {
	 case  5:
		if (strcmp(name, "delay") == 0)
		{
	    	    print_code(DELAY, name);
            	    timeIndex = 2;
            	    strcat(argStr, " 1 0 1 ");
            	    print_format(2, FLOAT, 0);
            	    print_fargs(2, 0);
		}
		else if (strcmp(name, "pulse") == 0)
		{
		    dev = query_device(2);
		    if (dev <= 0)
			return(0);
		    print_code(PULSE, name);
                    timeIndex = 4;
		    strcat(argStr," 1 0 1 1 rof1 %.9f rof2 %.9f ");
		    cat_arg(argStr, " %d oph %%d ", dev);
            	    print_format(3, FLOAT, 0);
		    strcat(argStr, ",rof1, rof2, oph ");
		    print_fargs(4, 0);
		}
		else
		    return(0);
		break;
	 case  8:
		if (strcmp(name, "simpulse") == 0)
		{
		    print_code(SMPUL, name);
                    timeIndex = 3;
		    strcat(argStr, " 2 0 1 1 rof1 %.9f rof2 %.9f ");
		    cat_arg(argStr, " %d oph %%d ", TODEV);
            	    print_format(2, FLOAT, -1);
		    cat_arg(argStr, " %d oph %%d ", DODEV);
            	    print_format(4, FLOAT, 0);
		    strcat(argStr, ",rof1, rof2, oph ");
		    print_fargs(3, 0);
		    strcat(argStr, ", oph ");
		    print_fargs(5, 0);
		}
		else
		    return(0);
		break;
	 case  9:
		if (strcmp(name, "sim3pulse") == 0)
		{
		    print_code(SMPUL, name);
                    timeIndex = 3;
		    strcat(argStr, " 3 0 1 1 rof1 %.9f rof2 %.9f ");
		    cat_arg(argStr, " %d oph %%d ", TODEV);
            	    print_format(2, FLOAT, -1);
		    cat_arg(argStr, " %d oph %%d ", DODEV);
            	    print_format(4, FLOAT, -1);
		    cat_arg(argStr, " %d oph %%d ", DO2DEV);
            	    print_format(6, FLOAT, 0);
		    strcat(argStr, ",rof1, rof2, oph ");
		    print_fargs(3, 0);
		    strcat(argStr, ", oph ");
		    print_fargs(5, 0);
		    strcat(argStr, ", oph ");
		    print_fargs(7, 0);
		}
		else if (strcmp(name, "rgradient") == 0)
		{
		    print_code(RGRAD, name);
		    strcat(argStr, " 11 0 4 ");
                    print_format(CHAR, 3, FLOAT, -1);
                    strcat(argStr, " 0.00 0.0 0.00 0.0 gradalt %.9f\\n\"");
                    print_str(2, 0);
                    print_fargs(3, 0);
		    strcat(argStr, ", (float)gradalt");
		}
		else if (strcmp(name, "vgradient") == 0)
		{
		    print_code(VGRAD, name);
		    strcat(argStr, " 11 3 0 ");
		    print_format(CHAR, 3, INTG, 4, INTG, 5, INTG, 0);
                    print_str(2, 0);
                    print_args(3, 4, 5, 0);
		}
		else
		    return(0);
		break;
	 case 10:
		if (strcmp(name, "zgradpulse") == 0)
		{
		    print_code(ZGRAD, name);
                    timeIndex = 3;
		    strcat(argStr, " 11 0 3 z ");
                    print_format(3, FLOAT, 2, FLOAT, -1);
		    strcat(argStr, " gradalt ");
		    print_format(FLOAT, 0);
                    print_fargs(3, 2, 0);
		    strcat(argStr, ", (float)gradalt");
		}
		else
		    return(0);
		break;
	 case 11:
	 case 12:
		if ((strcmp(name, "shape_pulse") == 0) ||
		    (strcmp(name, "shaped_pulse") == 0))
		{
		    dev = query_device(2);
		    if (dev <= 0)
			return(0);
		    timeIndex = 4;
                    print_code(SHPUL, name);
                    strcat(argStr, " 1 1 1 1 rof1 %.9f rof2 %.9f ");
                    cat_arg(argStr, " %d ", dev);
                    print_format(STRG, -1);
                    strcat(argStr, " oph %d ");
                    print_format(4, FLOAT, 0);
                    strcat(argStr, ", rof1, rof2");
                    print_str(3, 0);
                    strcat(argStr, ", oph ");
                    print_fargs(4, 0);
		}
		else
		    return(0);
		break;
	 case 14:
		if (strcmp(name, "shapedgradient") == 0)
		{
		    timeIndex = 3;
                    print_code(SHGRAD, name);
		    strcat(argStr, " 12 2 2 ");
                    print_format(CHAR,STRG,6,INTG,7,INTG,3,FLOAT,4,FLOAT,0);
                    print_str(5, 2, 0);
                    print_args(6, 7, 0);
                    print_fargs(3, 4, 0);
		}
		else
		    return(0);
		break;
	 case 15:
		if (strcmp(name, "simshaped_pulse") == 0)
		{
		    timeIndex = 3;
		    print_code(SMSHP, name);
                    strcat(argStr, " 2 1 1 1 rof1 %.9f rof2 %.9f ");
                    print_format(INTG,STRG, -1);
                    strcat(argStr, " oph %d ");
                    print_format(3,FLOAT, INTG,STRG, -1);
                    strcat(argStr, " oph %d ");
                    print_format(4,FLOAT, 0);
                    strcat(argStr, ", rof1, rof2");
                    cat_arg(argStr, ", %d", TODEV);
                    print_str(2, 0);
                    strcat(argStr, ", oph");
                    print_fargs(3, 0);
                    cat_arg(argStr, ", %d", DODEV);
                    print_str(4, 0);
                    strcat(argStr, ", oph");
                    print_fargs(5, 0);
		}
		else
		    return(0);
		break;
	 case 16:
		if (strcmp(name, "sim3shaped_pulse") == 0)
		{
		    timeIndex = 3;
		    print_code(SMSHP, name);
                    strcat(argStr, " 3 1 1 1 rof1 %.9f rof2 %.9f ");
                    print_format(INTG,STRG, -1);
                    strcat(argStr, " oph %d ");
                    print_format(3,FLOAT, INTG,STRG, -1);
                    strcat(argStr, " oph %d ");
                    print_format(5,FLOAT, INTG,STRG, -1);
                    strcat(argStr, " oph %d ");
                    print_format(7,FLOAT, 0);
                    strcat(argStr, ", rof1, rof2");
                    cat_arg(argStr, ", %d", TODEV);
                    print_str(2, 0);
                    strcat(argStr, ", oph");
                    print_fargs(3, 0);
                    cat_arg(argStr, ", %d", DODEV);
                    print_str(4, 0);
                    strcat(argStr, ", oph");
                    print_fargs(5, 0);
                    cat_arg(argStr, ", %d", DO2DEV);
                    print_str(6, 0);
                    strcat(argStr, ", oph");
                    print_fargs(7, 0);
		}
		else if (strcmp(name, "shaped2Dgradient") == 0)
		{
		    timeIndex = 3;
		    print_code(SHGRAD, name);
		    strcat(argStr, " 12 2 2 ");
		    print_format(CHAR,STRG,6,INTG,7,INTG,3,FLOAT,4,FLOAT,0);
                    print_str(5, 2, 0);
                    print_args(6, 7, 0);
                    print_fargs(3, 4, 0);
		}
		else
		    return(0);
		break;
	 default:
		return(0);
		break;
	}
	return(1);
}


