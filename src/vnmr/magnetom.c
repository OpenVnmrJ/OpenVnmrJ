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
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
/* #include <sys/dir.h> */
#include <sys/file.h>
#include <sys/ioctl.h>

/* defines UNIX, SUN, VNMR, MAXPATHL, and extern's for system, user and
   current experiment directories */
#include "vnmrsys.h"

/* defines parameter basic- and sub-types, and information structures
   (no functions) */
#include "variables.h"

/* defines tree, group and display group, and active values
   (no functions) */
#include "group.h"

/* defines things for programs using the element routine for handling
   item selection in the canvas */
#include "element.h"

/* defines headers for VNMR data files */
#include "data.h"

/* defines graphics and display variables */
#include "init2d.h"
#include "pvars.h"
#include "wjunk.h"

extern double graysl;         /* value of the current "graysl" parameter */
extern double graycntr;       /* value of the current "grayctr" parameter */
static Elist *elist = NULL;   /* pointer to elementlist structure */

/* the name of the GLOBAL parameter used to control the destination
   directory on the Sun for files downloaded from the MAGNETOM */
#define NUMARIS_FILE_DEST "magdest"

/* sizes of arrays for holding information needed by DNI commands */
#define NODE_NAME_LEN 40
#define USER_NAME_LEN 40
#define PASSWORD_LEN  40
#define REMOTE_DIR_LEN MAXPATH

/* Rounding function definition */
#define IRND(x)		((x) >= 0 ? ((int)((x)+0.5)) : (-(int)(-(x)+0.5)))

static char mag_err_msg[] = "MAGNETOM file transfer not available";

static char NodeName [NODE_NAME_LEN];
static char UserID [USER_NAME_LEN];
static char Password [PASSWORD_LEN];
static char PassKey  [PASSWORD_LEN+3];
static char RemoteDir [REMOTE_DIR_LEN];

static char prompt_node[] = "Enter name for remote node:";
static char prompt_user[] = "Enter user name for remote node:";
static char prompt_pass[] = "Enter password for remote node:";
static char prompt_dir[]  = "Enter name for remote directory:";
static char prompt_file[] = "Enter name for remote file:";
static char prompt_src[]  = "Enter name for source VNMR file:";

static char  VMS_srch_spec[] = "*.*";
static char *VMS_extents[] = { "DAT", "IMA", "RAW" };
static int   VMS_extents_count = sizeof(VMS_extents) / sizeof(VMS_extents[0]);

static char  extent_msg[] = "Illegal file extension!";
static char  cvt_errmsg[] = "... error during conversion.";
static char  one_errmsg[] = "Please select only one file!";
static char  cnv_endmsg[] = "... finished";

/* the three commands available for exporting data to the MAGNETOM */

static char CMD_EXPORT_FILE[]  = "putf";
static char CMD_EXPORT_IMAGE[] = "puti";
static char CMD_EXPORT_PHASE[] = "putp";

/* functions local to this module */

static int set_access( /* void */ );
static int check_file_name( /* char *file_name */ );
static int check_extent( /* char *extent, char *VMS_extent */ );
static int dir_load( /* char *dirpath */ );
static int get_file_dir( /* char *file_dir, char *file_name */ );
static int float12bits (char *file);
static int pix12bits(char *outname);

static char *pathptr;
extern char *getenv( /* char *name */ );
extern char *getcwd( /* char *pathname */ );
extern char *get_cwd();
extern char *W_getInput(char *prompt, char *s, int n);
extern int isDirectory(char *filename);
extern void WblankCanvas();
extern int getNumberOfSelect(Elist *elist);
extern int displayElements(Elist *elist);
extern int sortElementByName(Elist *elist);
extern int releaseElementStruct(Elist *elist);
extern int addElement(Elist *elist, char *name, char *info);
extern int menu(int argc, char *argv[], int retc, char *retv[]);

/****************************************************************************
 check_magnetom

 This function checks for the existence of the stand-alone VNMR-MAGNETOM
 file transfer programs.

 INPUT ARGS:
   none
 OUTPUT ARGS:
   none
 RETURN VALUE:
   1           File transfer programs are readable and executable.
   0           File transfer programs not found.
 GLOBALS USED:
   systemdir   Name of the directory that contains the VNMR system-level files.
 GLOBALS CHANGED:
   none
 ERRORS:
   0           File transfer programs not readable or executable
               (see return from Unix "access()" (2V) call)
 EXIT VALUES:
   none
 NOTES:
   The status of the file-transfer programs is kept in a local static variable.
   This function should be called at the start of any externally-visible mag-
   netom functions, to prevent inadvertent command-line access to such functions.

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static int keep_files = 0; /* flag: 1=> do not delete intermediate files */

int check_magnetom()
{
   /**************************************************************************
   LOCAL VARIABLES:

   MagnetomOK  Static variable, to indicate status of file transfer programs.
   s           Buffer for building the full name of the file transfer programs.
   */
   static int MagnetomOK = 0;
   vInfo  info;
   double temp;

   if (MagnetomOK != 1)
   {
      char s[MAXPATH];

      /* first check for the "import files" program */
      (void)strcpy (s, systemdir);
      (void)strcat (s, "/bin/impsie2");

      /* now check for the "export files" program */
      if ( (MagnetomOK = (access (s, R_OK | X_OK) ) == 0))
      {
         (void)strcpy (s, systemdir);
         (void)strcat (s, "/bin/expsis");
         MagnetomOK = (access (s, R_OK | X_OK) == 0);
      }
   }
   if (keep_files == 0)
   {
      if (P_getVarInfo (GLOBAL, "keepem", &info) == 0 && info.active == 1)
      {
         P_getreal (GLOBAL, "keepem", &temp, 1);
         keep_files = (int)temp;
      }
   }
   return (MagnetomOK);

}  /* end of function "check_magnetom" */

/****************************************************************************
 magnetom

 This function sets up the magnetom menu buttons and clears the graphics
 canvas.

 INPUT ARGS:
   argc        The number of arguments from the calling (VNMR) function
               (not used).
   argv        The arguments from the calling (VNMR) function
               (not used).
   retc        The number of arguments returned to the calling (VNMR) function
               (not used).
   retv        The arguments returned to the calling (VNMR) function
               (not used).
 OUTPUT ARGS:
   none
 RETURN VALUE:
   RETURN      Function completed successfully.
 GLOBALS USED:
   mag_err_msg Error message "MAGNETOM file transfer not available";
 GLOBALS CHANGED:
   none
 ERRORS:
   ABORT       MAGNETOM software not installed.
 EXIT VALUES:
   none
 NOTES:
   The definitions for the menu-button labels and functions are in the file
   "magnetom", which should be in either the "<$vnmruser>/menulib" or
   "<$vnmrsystem>/menulib" directory.

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

/*ARGSUSED*/
int magnetom (argc, argv, retc, retv)
 int    argc;
 char **argv;
 int    retc;
 char **retv;
{
   /**************************************************************************
   LOCAL VARIABLES:

   args     List of arguments for the call to the "menu" function.
   */
   char *args[2];

   /* see if the magnetom link software is installed */
   if (check_magnetom()==0)
   {
      Werrprintf (mag_err_msg);
      ABORT;
   }
   /* set up the menu for the magnetom functions */
   args[0] = "menu";
   args[1] = "magnetom";   /* "<$vnmruser>/menulib" or "$vnmrsystem/menulib" */
   menu (2, args, 0, args);

   WblankCanvas();
   RETURN;

}  /* end of function "magnetom" */

/****************************************************************************
 set_mag_login

 This function sets up the menu buttons for the "login" sub-menu, which is
 accessed via the main "magnetom" menu.

 INPUT ARGS:
   argc        The number of arguments from the calling (VNMR) function
               (not used).
   argv        The arguments from the calling (VNMR) function
               (not used).
   retc        The number of arguments returned to the calling (VNMR) function
               (not used).
   retv        The arguments returned to the calling (VNMR) function
               (not used).
 OUTPUT ARGS:
   none
 RETURN VALUE:
   RETURN      Function completed successfully.
 GLOBALS USED:
   mag_err_msg Error message "MAGNETOM file transfer not available";
 GLOBALS CHANGED:
   none
 ERRORS:
   ABORT       MAGNETOM software not installed.
 EXIT VALUES:
   none
 NOTES:
   The definitions for the menu-button labels and functions are in the file
   "maglogin", which should be in either the "<$vnmruser>/menulib" or
   "<$vnmrsystem>/menulib" directory.

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

/*ARGSUSED*/
int set_mag_login (argc, argv, retc, retv)
 int    argc;
 char **argv;
 int    retc;
 char **retv;
{
   /**************************************************************************
   LOCAL VARIABLES:

   args     List of arguments for the call to the "menu" function.
   */
   char *args[2];

   /* see if the magnetom link software is installed */
   if (check_magnetom()==0)
   {
      Werrprintf(mag_err_msg);
      ABORT;
   }
   args[0] = "menu";
   args[1] = "maglogin";   /* "<$vnmruser>/menulib" or "$vnmrsystem/menulib" */
   menu(2, args, 0, args);
   RETURN;

}  /* end of function "set_mag_login" */

/****************************************************************************
 set_node_name

 This function sets the node name for the MAGNETOM-system computer; the name
 can be set via either an input argument or a response to a user prompt.

 INPUT ARGS:
   argc        The number of arguments from the calling (VNMR) function.
   argv        The arguments from the calling (VNMR) function: the second
               argument is (optionally) the name of the desired network node.
   retc        The number of arguments returned to the calling (VNMR) function
               (not used).
   retv        The arguments returned to the calling (VNMR) function
               (not used).
 OUTPUT ARGS:
   none
 RETURN VALUE:
   0           Function completed successfully.
   1           Node name was not set: no legal response to prompt.
 GLOBALS USED:
   NodeName    The name of the desired network node (MAGNETOM computer).
   mag_err_msg Error message "MAGNETOM file transfer not available";
   prompt_node User prompt "Enter name for remote node:".
 GLOBALS CHANGED:
   NodeName    The name of the desired network node (MAGNETOM computer).
 ERRORS:
   ABORT       MAGNETOM software not installed.
   1           No entry in response to prompt for the node name.
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

/*ARGSUSED*/
int set_node_name (argc, argv, retc, retv)
 int    argc;
 char **argv;
 int    retc;
 char **retv;
{
   /**************************************************************************
   LOCAL VARIABLES:

   temp     Buffer for the name of the desired node.
   ret_val  Return value for the result of the operation.
   */
   char temp[NODE_NAME_LEN];
   int  ret_val = 0;

   /* see if the magnetom link software is installed */
   if (check_magnetom()==0)
   {
      Werrprintf(mag_err_msg);
      ABORT;
   }
   /* see if the node name was specified in the argument list */
   if (argc > 1)
      (void)strcpy (NodeName, *(argv+1));

   /* prompt for the node name if not specified */
   else if (W_getInput (prompt_node, temp, sizeof(temp)) == NULL ||
            temp[0] == '\0')
      ret_val = 1;

   /* load the node name if specified */
   else
      (void)strcpy (NodeName, temp);

   return (ret_val);

}  /* end of function "set_node_name" */

/****************************************************************************
 set_user_name

 This function sets the name of the user account on the MAGNETOM-system
 computer; the name can be set via either an input argument or a response
 to a user prompt.

 INPUT ARGS:
   argc        The number of arguments from the calling (VNMR) function.
   argv        The arguments from the calling (VNMR) function: the second
               argument is (optionally) the name of the user account.
   retc        The number of arguments returned to the calling (VNMR) function
               (not used).
   retv        The arguments returned to the calling (VNMR) function
               (not used).
 OUTPUT ARGS:
   none
 RETURN VALUE:
   0           Function completed successfully.
   1           User name was not set: no legal response to prompt.
 GLOBALS USED:
   UserID    The name of the desired user account (MAGNETOM computer).
   mag_err_msg Error message "MAGNETOM file transfer not available";
   prompt_user User prompt "Enter user name for remote node:".
 GLOBALS CHANGED:
   UserID    The name of the desired user account (MAGNETOM computer).
 ERRORS:
   ABORT       MAGNETOM software not installed.
   1           No entry in response to prompt for the user name.
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

/*ARGSUSED*/
int set_user_name (argc, argv, retc, retv)
 int    argc;
 char **argv;
 int    retc;
 char **retv;
{
   /**************************************************************************
   LOCAL VARIABLES:

   temp     Buffer for the name of the desired user.
   ret_val  Return value for the result of the operation.
   */
   char temp[USER_NAME_LEN];
   int  ret_val = 0;

   /* see if the magnetom link software is installed */
   if (check_magnetom()==0)
   {
      Werrprintf(mag_err_msg);
      ABORT;
   }
   /* see if the user name was specified in the argument list */
   if (argc > 1)
      (void)strcpy (UserID, *(argv+1));

   /* prompt for the user name if not specified */
   else if (W_getInput (prompt_user, temp, sizeof(temp)) == NULL ||
            temp[0] == '\0')
      ret_val = 1;

   /* load the user name if specified */
   else
      (void)strcpy (UserID, temp);

   return (ret_val);

}  /* end of function "set_user_name" */

/****************************************************************************
 set_password

 This function sets the password for the user account on the MAGNETOM-system
 computer; the password can be set only by a response to a user prompt. The
 typed response will not be echoed to the screen.

 INPUT ARGS:
   argc        The number of arguments from the calling (VNMR) function
               (not used).
   argv        The arguments from the calling (VNMR) function
               (not used).
   retc        The number of arguments returned to the calling (VNMR) function
               (not used).
   retv        The arguments returned to the calling (VNMR) function
               (not used).
 OUTPUT ARGS:
   none
 RETURN VALUE:
   0           Function completed successfully.
   1           Password was not set: no legal response to prompt.
 GLOBALS USED:
   Password    The password for the desired user account (MAGNETOM computer).
   PassKey     The password switch string for SunLink commands; this is needed
               because the password switch string for "no password" must look
               like '-p ""', and is set by entering a blank at the prompt
   mag_err_msg Error message "MAGNETOM file transfer not available";
   prompt_pass User prompt "Enter password for remote node:".
 GLOBALS CHANGED:
   Password    The password for the desired user account (MAGNETOM computer).
   PassKey     The password switch string for SunLink commands.
 ERRORS:
   ABORT       MAGNETOM software not installed.
   1           No entry in response to prompt for the password.
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

/*ARGSUSED*/
int set_password (argc, argv, retc, retv)
 int    argc;
 char **argv;
 int    retc;
 char **retv;
{
   /**************************************************************************
   LOCAL VARIABLES:

   temp     Buffer for the password for the desired user account.
   ret_val  Return value for the result of the operation.
   */
   char temp[PASSWORD_LEN];
   int  ret_val = 0;

   /* see if the magnetom link software is installed */
   if (check_magnetom()==0)
   {
      Werrprintf(mag_err_msg);
      ABORT;
   }
   /* turn off echoing in the command window */
   /* sendEchoStateToMaster ("NOECHO"); */

   /* prompt for the password; return failure if nothing entered */
   if (W_getInput(prompt_pass,temp,sizeof(temp)) == NULL || temp[0] == '\0')
      ret_val = 1;
   else
   {
      /* scan past leading white space in the response string */
      int i;
      for (i = 0; isspace (temp[i]); ++i)
         ;
      /* if anything left in string, build PassKey string '-p <password>' */
      if (temp[i])
      {
         (void)strcpy (Password, temp+i);
         (void)strcpy (PassKey, "-p ");
         (void)strcat (PassKey, Password);
      }
      /* otherwise, build PassKey string '-p ""' */
      else
      {
         (void)strcpy (Password, " ");
         (void)strcpy (PassKey, "-p \"\"");
      }
   }
   /* turn command-line echoing back on */
   /* sendEchoStateToMaster ("ECHO"); */

   return (ret_val);

}  /* end of function "set_password" */

/****************************************************************************
 set_remote_dir

 This function sets the default VMS directory for the MAGNETOM-system computer;
 the name can be set via either an input argument or a response to a user prompt.

 INPUT ARGS:
   argc        The number of arguments from the calling (VNMR) function.
   argv        The arguments from the calling (VNMR) function: the second
               argument is (optionally) the name of the desired default VMS
               directory.
   retc        The number of arguments returned to the calling (VNMR) function
               (not used).
   retv        The arguments returned to the calling (VNMR) function
               (not used).
 OUTPUT ARGS:
   none
 RETURN VALUE:
   0           Function completed successfully.
   1           Default directory name was not set: no legal response to prompt.
 GLOBALS USED:
   RemoteDir   The name of the desired default directory (MAGNETOM computer).
   mag_err_msg Error message "MAGNETOM file transfer not available";
   prompt_dir  User prompt "Enter name for remote directory:".
 GLOBALS CHANGED:
   RemoteDir   The name of the desired default directory (MAGNETOM computer).
 ERRORS:
   ABORT       MAGNETOM software not installed.
   1           No entry in response to prompt for the directory name.
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

/*ARGSUSED*/
int set_remote_dir (argc, argv, retc, retv)
 int    argc;
 char **argv;
 int    retc;
 char **retv;
{
   /**************************************************************************
   LOCAL VARIABLES:

   temp     Buffer for the name of the desired default directory.
   */
   char temp [REMOTE_DIR_LEN];

   /* see if the magnetom link software is installed */
   if (check_magnetom()==0)
   {
      Werrprintf(mag_err_msg);
      ABORT;
   }
   /* check for a name in the input arguments */
   if (argc > 1)
      (void)strcpy (temp, *(argv+1));
   else if (W_getInput (prompt_dir, temp, sizeof(temp)) == NULL ||
            temp[0] == '\0')
      ABORT;

   /* if no special VMS characters in name, put response in brackets */
   if (strchr (temp, '$') == NULL &&
       strchr (temp, ':') == NULL &&
       strchr (temp, '[') == NULL   )
   {
      (void)strcpy (RemoteDir, "[");
      (void)strcat (RemoteDir, temp);
      (void)strcat (RemoteDir, "]");
   }
   /* otherwise, take it as it comes */
   else
      (void)strcpy (RemoteDir, temp);

   RETURN;

}  /* end of function "set_remote_dir" */

/****************************************************************************
 show_remote_files

 This function obtains a VMS directory listing of the default directory on
 the VAX node for the user account.

 INPUT ARGS:
   argc        The number of arguments from the calling (VNMR) function
               (not used).
   argv        The arguments from the calling (VNMR) function.
               (not used).
   retc        The number of arguments returned to the calling (VNMR) function
               (not used).
   retv        The arguments returned to the calling (VNMR) function
               (not used).
 OUTPUT ARGS:
   none
 RETURN VALUE:
   0           Function completed successfully.
   1           Default directory name was not set: no legal response to prompt.
 GLOBALS USED:
   elist       A pointer to an elementlist structure, for mouse selections.
   mag_err_msg Error message "MAGNETOM file transfer not available";
   UserID    The name of the desired user account (MAGNETOM computer).
   PassKey     The password switch string for SunLink commands.
   NodeName    The name of the desired network node (MAGNETOM computer).
   RemoteDir   The name of the desired default directory (MAGNETOM computer).
   VMS_srch_spec  The VMS directory search string "*.*" meaning all files.
 GLOBALS CHANGED:
   elist       A pointer to an elementlist structure, for mouse selections.
 ERRORS:
   ABORT       MAGNETOM software not installed.
               MAGNETOM access information not set.
               Can't access remote node.
               No files found in remote directory.
               Error occurred trying to read remote directory.
 EXIT VALUES:
   none
 NOTES:
   The SunLink "dnils" command returns a list of file names in the VMS dir-
   ectory. The first name returned is the name of the default VMS directory.
   Blank lines are possible.

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

/*ARGSUSED*/
int show_remote_files (argc, argv, retc, retv)
 int    argc;
 char **argv;
 int    retc;
 char **retv;
{
   /**************************************************************************
   LOCAL VARIABLES:

   cmd_buffer  Character buffer for building the SunLink "dnils" command.
   p           A character pointer, for parsing names in "cmd_buffer".
   cmd_fp      File pointer for the pipe to the SunLink "dnils" command.
   line_buffer Character buffer for lines returned by SunLink "dnils" in the
               pipe at "cmd_fp".
   first       A flag, to indicate that the first line from SunLink was read.
   len         The length of an input line, used to kill trailing newlines.
   */
   char  cmd_buffer[128];
   char *p;
   FILE *cmd_fp;
   char  line_buffer[256];
   int   first;
   int   len;

   /* see if the magnetom link software is installed */
   if (check_magnetom()==0)
   {
      Werrprintf(mag_err_msg);
      ABORT;
   }
   /* must clear out old elist if one already exists */
   if (elist != NULL)
   {
      releaseElementStruct (elist);
      WblankCanvas();
      elist = NULL;
   }
   /* make sure everything that is needed for the VAX access is set */
   if (set_access() != 0)
      ABORT;

   /* build the command for listing the files in the remote directory */
   (void)sprintf (cmd_buffer, "dnils -u %s %s '%s::%s%s'",
                  UserID, PassKey, NodeName, RemoteDir, VMS_srch_spec);

   Winfoprintf ("Getting file names in remote directory \"%s::%s\"",
               NodeName, RemoteDir);

   if ( (cmd_fp = popen (cmd_buffer, "r")) == NULL)
   {
      Werrprintf ("... can't access remote node \"%s\"", NodeName);
      ABORT;
   }
   /* read the file names returned from the command */
   for (first = 1; fgets (line_buffer, 256, cmd_fp) != NULL; )
   {
      /* if this is a non-trivial line read from the command ... */
      if (line_buffer[0] != '\n' && line_buffer[0] != '\0')
      {
         /* remove any trailing newline */
         len = strlen(line_buffer);
         if (line_buffer[len-1] == '\n')
            line_buffer[--len] = '\0';

         /* the first non-trivial line is the remote directory name */
         if (first == 1)
         {
            Winfoprintf ("%s", line_buffer);
            first = 0;
            continue;
         }
         for (p = line_buffer; (p = strtok (p," \t")) != NULL; p = NULL)
         {
            /* load an entry for the display in the graphics canvas */
            addElement (elist, p, "n");
         }
      }
   }
   /* check for errors from reading the remote directory */
   if (ferror (cmd_fp) || pclose (cmd_fp) != 0)
   {
      if (first == 1)
         Werrprintf ("... error reading remote directory");
      else
         Werrprintf ("... nothing matches \"%s\"", VMS_srch_spec);

      releaseElementStruct (elist);
      elist = NULL;

      (void)pclose (cmd_fp);

      ABORT;
   }
   /* display the files found on the remote node */
   sortElementByName (elist);
   displayElements (elist);

   RETURN;

}  /* end of function "show_remote_files" */

/****************************************************************************
 import_files

 This function transfers a file from the MAGNETOM to the Sun.

 INPUT ARGS:
   argc        The number of arguments from the calling (VNMR) function
               (not used).
   argv        The arguments from the calling (VNMR) function.
               (not used).
   retc        The number of arguments returned to the calling (VNMR) function
               (not used).
   retv        The arguments returned to the calling (VNMR) function
               (not used).
 OUTPUT ARGS:
   none
 RETURN VALUE:
   RETURN      Function completed successfully.
   ABORT       Error occurred getting file from MAGNETOM.
 GLOBALS USED:
   mag_err_msg Error message "MAGNETOM file transfer not available";
   elist       A pointer to an elementlist structure, for mouse selections.
   prompt_file User prompt "Enter name for remote file:".
   UserID    The name of the desired user account (MAGNETOM computer).
   PassKey     The password switch string for SunLink commands.
   NodeName    The name of the desired network node (MAGNETOM computer).
   RemoteDir   The name of the desired default directory (MAGNETOM computer).
 GLOBALS CHANGED:
   elist       A pointer to an elementlist structure, for mouse selections.
 ERRORS:
   ABORT       MAGNETOM software not installed.
               Environment variable NUMARIS_TEMPLATE not defined.
               Couldn't load name of current working directory.
               MAGNETOM access information not set.
               No file name was specified.
               Error selecting file name with mouse input.
               Invalid format for name of requested file.
               Can't access or create a temporary directory for MAGNETOM files.
               Can't access remote node.
               No files found in remote directory.
               Error occurred trying to read remote directory.
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

/*ARGSUSED*/
int import_files (argc, argv, retc, retv)
 int    argc;
 char **argv;
 int    retc;
 char **retv;
{
   /**************************************************************************
   LOCAL VARIABLES:

   p_string       Miscellaneous pointer for handling character strings.
   info           VNMR parameter structure, for getting the NUMARIS_FILE_DEST
                  parameter.
   dest_dir       A character buffer for storing the name of the Unix dest-
                  ination directory.
   default_dest   A flag: 1 => use current working directory as destination.
   file_name      The (VMS) name of the MAGNETOM file to transfer.
   mag_file       The full (Unix) name of the file after transfer to the Sun.
   dnicp_cmd      A character buffer for building the SunLink "dnicp" command.
   convert_cmd    A character buffer for building the command to convert the
                  NUMARIS file to VNMR format: system call to "impsie2".
   i              Index for guaranteeing the Unix name is all lower-case.
   picked         A flag: 1 => mouse used to pick a name, so clear selection.
   */
   char *p_string;
   vInfo info;
   char  dest_dir  [MAXPATH];
   int   default_dest = 0;
   char  file_name [MAXPATH];
   char  mag_file  [MAXPATH];
   char  dnicp_cmd [MAXPATH];
   char  convert_cmd [MAXPATH];
   char  dat_file [MAXPATH];
   int   i;
   int   picked = 0;

   /* see if the magnetom link software is installed */
   if (check_magnetom()==0)
   {
      Werrprintf(mag_err_msg);
      ABORT;
   }
   /* the NUMARIS_TEMPLATE environment variable must be defined for
      the file-format conversion program */
   if (getenv ("NUMARIS_TEMPLATE") == NULL)
   {
      Werrprintf ("Environment variable NUMARIS_TEMPLATE not defined");
      ABORT;
   }
   /* set the destination directory for files transferred from the MAGNETOM */
   if (P_getVarInfo (GLOBAL, NUMARIS_FILE_DEST, &info) == 0)
   {
      if (info.active == 0)
      {
         default_dest = 1;
         p_string = "active";
      }
      else if (P_getstring(GLOBAL,NUMARIS_FILE_DEST,dest_dir,1,MAXPATH - 1)!= 0 ||
               *dest_dir == '\0')
      {
         default_dest = 1;
         p_string = "set";
      }
   }
   else
   {
      default_dest = 1;
      p_string = "defined";
   }
   /* if the parameter was not found, load the name of the current
      working directory and display a message */
   if (default_dest == 1)
   {
     /*
      if (getcwd(dest_dir, sizeof( dest_dir ) -1 ) == NULL)
     */
      if ((pathptr = get_cwd()) == NULL)
      {
         Werrprintf ("import_files: getcwd() failed");
         ABORT;
      }
      strcpy(dest_dir, pathptr);
      Werrprintf("GLOBAL variable \"%s\" is not %s; file destination",
                 NUMARIS_FILE_DEST,p_string);
      Werrprintf (" directory is \"%s\"", dest_dir);
   }
   /* make sure that everything needed for the VAX access is set */
   if (set_access() != 0)
      ABORT;

   /* get the name of the MAGNETOM data file */
   if (argc > 1)
      strcpy (file_name, argv[1]);
   else
   {
      switch (getNumberOfSelect (elist))
      {
         case 0:
            if (W_getInput (prompt_file,file_name,sizeof(file_name)) == NULL ||
                file_name[0] == '\0')
               ABORT;
            break;
         case 1:
            if ( (p_string = getFirstSelection (elist)) == NULL)
            {
               Werrprintf ("Error selecting file name");
               ABORT;
            }
            (void)strcpy (file_name, p_string);
            picked = 1;
            break;
         default:
            Werrprintf (one_errmsg);
            ABORT;
      }
   }
   /* check for the proper form of the file name */
   if (check_file_name (file_name) < 0)
      ABORT;

   /* build the name of the directory that will hold the converted data
      file as a sub-directory under NUMARIS_FILE_DEST, by replacing the
      suffix on the MAGNETOM file name with "fid" */

   p_string = dest_dir + strlen(dest_dir);
   *p_string++ = '/';
   (void)strcpy (p_string, file_name);
   for (i = 0; (*p_string = file_name[i]) != '.'; ++i, ++p_string)
   {
      if (isalpha(*p_string) && isupper(*p_string))
         *p_string = tolower(*p_string);
   }
   (void)strcpy (p_string+1, "dat");
   (void)strcpy (dat_file, dest_dir);
   (void)strcpy (p_string+1, "fid");

   /* create a temporary directory for holding the MAGNETOM data file;
      the form of the directory name is "/usr/tmp/MAG.`pid`" */

   (void)sprintf (mag_file, "/usr/tmp/MAG.%d", getpid() );

   if (access (mag_file, W_OK) != 0)
   {
      if (errno != ENOENT || mkdir (mag_file, 0777) == -1)
      {
         Werrprintf ("Can't access temporary directory \"%s\" for data files",
                     mag_file);
         ABORT;
      }
   }
   /* build the SunLink network file copy command */

   (void)sprintf (dnicp_cmd, "dnicp -v -u %s %s '%s::%s%s' %s",
         UserID, PassKey, NodeName, RemoteDir, file_name, mag_file);

   Winfoprintf ("Getting MAGNETOM data file \"%s\"", file_name);

   /* copy the MAGNETOM data file from the VAX to the temporary directory */
   if (system (dnicp_cmd) == 0)
   {
      /* "dnicp" converts file names to lower-case */
      for (i = 0; file_name[i] != '\0'; ++i)
      {
         if (isalpha(file_name[i]) && isupper(file_name[i]))
            file_name[i] = tolower(file_name[i]);

         /* eliminate the VMS version number, if present */
         if (file_name[i] == ';')
         {
            file_name[i] = '\0';
            break;
         }
      }
      /* build the full Unix name of the MAGNETOM data file */
      p_string = mag_file + strlen (mag_file);
      *p_string = '/';
      (void)strcpy (p_string+1, file_name);

      /* leave "p_string" pointing at the end of the directory path, for
         removing the temporary directory when the conversion is done */

      /* create the destination directory for the converted MAGNETOM file */
      if (access (dest_dir, W_OK) != 0)
      {
         if (errno != ENOENT || mkdir (dest_dir, 0777) == -1)
         {
            Werrprintf ("Can't access destination directory \"%s\"", dest_dir);
            ABORT;
         }
      }
      /* build the command string to convert the data file */
      (void)sprintf (convert_cmd, "impsie2 -noexit '%s' %s", mag_file, dest_dir);

      Winfoprintf ("Converting intermediate file \"%s\"", mag_file);

      /* convert the data file */
      if (system (convert_cmd) != 0)
      {
         Werrprintf (cvt_errmsg);
         /* try to remove the data directory */
         (void)rmdir (dest_dir);
      }
      else
      {
         int   len1;
         char *p_type = NULL;

         len1 = strlen (dest_dir);
         strcpy (dest_dir + len1, "/fid");

         /* check the data file in the target directory:
            "fid" => spectra, "phasefile" = > images */

         if (access (dest_dir, F_OK) == 0)
            p_type = "spectroscopy";
         else if (errno == ENOENT)
         {
            strcpy (dest_dir + len1, "/phasefile");
            if (access (dest_dir, F_OK) == 0)
	    {
               p_type = "image";
	       /* change directory tag to .dat from .fid */
	       dest_dir[len1] = '\0';
	       sprintf(convert_cmd,"mv %s %s",dest_dir,dat_file);
	       if (system(convert_cmd) != 0)
		  Werrprintf ("Error in mv %s %s",dest_dir,dat_file);
	    }

         }
         if (p_type == NULL)
            Werrprintf ("File converted from %s not found", mag_file);
         else
            Werrprintf ("%s; \"%s\" has %s data.",cnv_endmsg,mag_file,p_type);

         /* remove the MAGNETOM (temporary) data file */
         if (keep_files == 0)
            (void)unlink (mag_file);

         /* remove the file name from the name string for the data file */
         *p_string = '\0';
      }
      /* clear the file selection */
      /*if (picked == 1)	*/
      /*   clearMarkedSelection();	*/
   }
   else
      Werrprintf ("... error getting file from MAGNETOM.");

   /* remove the temporary directory */
   if (keep_files == 0)
      (void)rmdir (mag_file);

   RETURN;

}  /* end of function "import_files" */

/****************************************************************************
 show_local_files

 This function displays a listing of file names in the requested directory.
 If no directory name is passed in the arguments, the current working
 directory is used.

 INPUT ARGS:
   argc        The number of arguments from the calling (VNMR) function.
   argv        The arguments from the calling (VNMR) function: the second
               argument is (optionally) the name of the desired directory.
   retc        The number of arguments returned to the calling (VNMR) function
               (not used).
   retv        The arguments returned to the calling (VNMR) function
               (not used).
 OUTPUT ARGS:
   none
 RETURN VALUE:
   RETURN      Function completed successfully.
   ABORT       Error occurred displaying a listing of requested directory.
 GLOBALS USED:
   mag_err_msg Error message "MAGNETOM file transfer not available";
 GLOBALS CHANGED:
   none
 ERRORS:
   ABORT       MAGNETOM software not installed.
               Couldn't load name of current working directory.
               Error occurred while trying to display directory listing.
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

/*ARGSUSED*/
int show_local_files (argc, argv, retc, retv)
 int    argc;
 char **argv;
 int    retc;
 char **retv;
{
   /**************************************************************************
   LOCAL VARIABLES:

   dir         A pointer to the name of the desired directory.
   curr_dir    A character buffer for storing the name of the current working
               directory.
   */
   char *dir;
   char  curr_dir[MAXPATH];

   /* see if the magnetom link software is installed */
   if (check_magnetom()==0)
   {
      Werrprintf(mag_err_msg);
      ABORT;
   }
   /* get the current working directory if none specified */
   if (argc == 1)
   {
      /* if ( (dir = getcwd(curr_dir, sizeof( curr_dir ) - 1 )) == NULL) */
      if ((pathptr = get_cwd()) == NULL)
      {
         Werrprintf ("show_local_files: getcwd() failed");
         ABORT;
      }
      strcpy(curr_dir, pathptr);
      dir = &curr_dir[0];
   }
   else
      dir = *(argv+1);

   /* Now setup the elements */
   if (dir_load (dir) == 0)
   {
      Winfoprintf("Current directory is \"%s\"", dir);
      sortElementByName(elist);
      displayElements(elist);
   }
   else
   {
      Werrprintf("Problems loading \"%s\" directory", dir);
      ABORT;
   }
   RETURN;

}  /* end of function "show_local_files" */

/****************************************************************************
 save_parameters

 This function writes the current and processed parameters to disk files
 "curpar" and "procpar", respectively.

 INPUT ARGS:
   none
 OUTPUT ARGS:
   none
 RETURN VALUE:
   0           Current and processed parameters written to disk.
   -1          Error occurred while trying to write parameters.
 GLOBALS USED:
   curexpdir   Name of the current experiment directory.
 GLOBALS CHANGED:
   none
 ERRORS:
   return -1   No valid experiment number in current experiment name.
   msg only    Error returned by function "P_save()".
 EXIT VALUES:
   none
 NOTES:
   This function is used instead of function "flush()" because THAT function
   closes all the data files!

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static int save_parameters()
{
   /**************************************************************************
   LOCAL VARIABLES:

   par_msg     Error message for error return from function "P_save".
   parampath   For building the name of the parameter file.
   length      The length of the current experiment directory name.
   pend        A pointer to the end of the parameter path string.
   */
   static char par_msg[] = "problem saving %s parameters";
   char  parampath[MAXPATH];
   int   length;
   char *pend;

   if ( (length = strlen (curexpdir)) == 0 || (curexpdir[length-1] - '0') < 1)
   {
      Werrprintf ("no current experiment, can't save parameters");
      return -1;
   }
   (void)strcpy (parampath, curexpdir);
   pend = parampath + length;

   /* save the current parameters */
   (void)strcpy (pend, "/curpar");
   if (P_save (CURRENT, parampath))
      Werrprintf (par_msg, "current");

   /* save the processed parameters */
   (void)strcpy (pend, "/procpar");
   if (P_save (PROCESSED, parampath))
      Werrprintf (par_msg, "processed");

   return 0;

}  /* end of function "save_parameters" */

/****************************************************************************
 export_files

 This function transfers a file from the Sun to the MAGNETOM. There are three
 entry points:
   CMD_EXPORT_FILE  [ (target_name [,source_name] ) ]
   CMD_EXPORT_IMAGE [ (target_name) ]
   CMD_EXPORT_PHASE [ (target_name) ]
 INPUT ARGS:
   argc        The number of arguments from the calling (VNMR) function.
   argv        The arguments from the calling (VNMR) function: the second
               argument is (optionally) the name of the VMS file on the VAX,
               the third argument (CMD_EXPORT_FILE only) is (optionally) the
               name of the Unix file on the Sun.
   retc        The number of arguments returned to the calling (VNMR) function
               (not used).
   retv        The arguments returned to the calling (VNMR) function
               (not used).
 OUTPUT ARGS:
   none
 RETURN VALUE:
   RETURN      Function completed successfully.
   ABORT       Error occurred getting file from MAGNETOM.
 GLOBALS USED:
   mag_err_msg Error message "MAGNETOM file transfer not available";
   elist       A pointer to an elementlist structure, for mouse selections.
   prompt_src  User prompt "Enter name for source VNMR file:".
   prompt_file User prompt "Enter name for remote file:".
   VMS_extents Array of VMS file name extensions, for building file names.
   UserID    The name of the desired user account (MAGNETOM computer).
   PassKey     The password switch string for SunLink commands.
   NodeName    The name of the desired network node (MAGNETOM computer).
   RemoteDir   The name of the desired default directory (MAGNETOM computer).
 GLOBALS CHANGED:
   elist       A pointer to an elementlist structure, for mouse selections.
 ERRORS:
   ABORT       MAGNETOM software not installed.
               MAGNETOM access information not set.
               No file name was specified.
               Error selecting file name with mouse input.
               Invalid format for name of source data file.
               No name specified for target file.
               Invalid format for name of target data file.
               Missing source parameter or data file.
               Can't access or create a temporary directory for intermediate
                  files.
               For pixel data: raster dump and conversion operation failed
                  (see function "pix12bits()" or "float12bits()").
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

/*ARGSUSED*/
int export_files (argc, argv, retc, retv)
 int    argc;
 char **argv;
 int    retc;
 char **retv;
{
   /**************************************************************************
   LOCAL VARIABLES:

   pixel_data     A flag: 1 => source file contains pixel data.
   p_string       Miscellaneous pointer for handling character strings.
   temp_dir       A character buffer for holding the name of a temporary
                  directory used for intermediate files.
   mag_file       The (VMS) name of the file after transfer to the MAGNETOM.
   src_dir        A character for holding the name of the directory that
                  contains the desired source data & parameter files.
   src_file       The name of the (Unix) source file to send to the MAGNETOM
   dnicp_cmd      A character buffer for building the SunLink "dnicp" command.
   convert_cmd    A character buffer for building the command to convert the
                  NUMARIS file to VNMR format: system call to "impsie2".
   which_extent   The index in the list of the VMS or Unix file extensions.
                  Currently only "IMA" is used. Note that Unix extension is
                  the lower-case version of the VMS one; the SunLink "dnicp"
                  command does case-conversion on the file names.
   picked         A flag: 1 => mouse used to pick a name, so clear selection.
   to_disk        A flag: 1 => flush parameters to disk before exporting.
   info           VNMR parameter structure, for getting the NUMARIS_FILE_DEST
                  parameter.
   dest_dir       A character buffer for storing the name of the Unix dest-
                  ination directory.
   file_name      The (VMS) name of the MAGNETOM file to transfer.
   i              Index for guaranteeing the Unix name is all lower-case.
   */
   int   pixel_data = 0;
   char *p_string;
   char  temp_dir [MAXPATH];
   char  mag_file [MAXPATH];
   char  src_dir  [MAXPATH];
   char  src_file [MAXPATH];
   char  dnicp_cmd [MAXPATH];
   char  convert_cmd [MAXPATH];
   int   which_extent;
   int   picked = 0;
   int   to_disk = 0;
   static char name_msg[] = "Must enter the name of a file.";

   /* see if the magnetom link software is installed */
   if (check_magnetom() == 0)
   {
      Werrprintf(mag_err_msg);
      ABORT;
   }
   /* make sure that everything needed for the VAX access is set */
   if (set_access() != 0)
      ABORT;

   /* decide whether it's a VNMR data file or pixel data */
   if (strcmp (*argv, CMD_EXPORT_FILE) == 0)
   {
      /* check for the source file name in the 3rd argument */
      if (argc < 3 || *(argv+2) == NULL || **(argv+2) == '\0')
      {
         /* get the name of the source VNMR data file */
         switch (getNumberOfSelect (elist))
         {
            case 0:
               if (W_getInput (prompt_src,src_file,sizeof(src_file)) == NULL ||
                   src_file[0] == '\0')
                  ABORT;
               break;
            case 1:
               if ( (p_string = getFirstSelection (elist)) == NULL)
               {
                  Werrprintf ("Error selecting file name");
                  ABORT;
               }
               if (*p_string == '/')
                  ++p_string;
               (void)strcpy (src_file, p_string);
               picked = 1;
               break;
            default:
               Werrprintf (one_errmsg);
               ABORT;
         }
      }
      else
         (void)strcpy (src_file, *(argv+2));
   }
   else if (strcmp (*argv, CMD_EXPORT_IMAGE) == 0)
   {
      pixel_data = 1;
   }
   else if (strcmp (*argv, CMD_EXPORT_PHASE) == 0)
   {
      pixel_data = 3;
   }
   /* get the name for the target file on the MAGNETOM system */
   if (argc == 1 || *(argv+1) == NULL || **(argv+1) == '\0')
   {
      if (W_getInput (prompt_file, mag_file, sizeof(mag_file)) == NULL ||
          mag_file[0] == '\0')
         ABORT;
   }
   else
      (void)strcpy (mag_file, *(argv+1));

   /* build a name, of the form "/usr/tmp/MAG.`pid`", for a temporary
      directory that will hold intermediate files; do it now so that
      the full name for the temporary pixel data file can be built */

   (void)sprintf (temp_dir, "/usr/tmp/MAG.%d", getpid() );

   /* check for the proper extension on the target (MAGNETOM) file
      name; add the proper extension if none was supplied */
   if (strrchr (mag_file, '.') == NULL)
   {
      (void)strcat (mag_file, ".");
      (void)strcat (mag_file, VMS_extents[1]);
   }
   /* all files will be "IMA" */
   else if ( (which_extent = check_file_name (mag_file)) != 1)
   {
      if (which_extent >= 0)
         Werrprintf (extent_msg);
      ABORT;
   }
   /* build a temporary name for the pixel data (source) file */
   if (pixel_data)
   {
      /* get the path name for the fid and procpar files for this data */
      if ( (which_extent = get_file_dir (src_dir, "acqfil/fid")) < 0)
      {
         if (which_extent == -1)
            Werrprintf (name_msg);
         else if (which_extent == -2)
            Werrprintf ("Missing fid file");
         else
            Werrprintf ("Missing parameter file");
         ABORT;
      }
      (void)sprintf (src_file, "%s/%s.pixdata", temp_dir, mag_file);

      /* need to write the parameters to disk, for the conversion */
      to_disk = 1;
   }
   /* otherwise, get the path name for the source file */
   else
   {
      if ( (which_extent = get_file_dir (src_dir, src_file)) < 0)
      {
         if (which_extent == -1)
            Werrprintf (name_msg);
         else if (which_extent == -2)
            Werrprintf ("Missing fid file \"%s\"", src_file);
         else
            Werrprintf ("Missing parameter file for \"%s\"", src_file);
         ABORT;
      }
      else if (which_extent > 0)
      {
         (void)sprintf (src_file, "%s/acqfil/fid", src_dir);

         /* need to write the parameters to disk, for the conversion */
         to_disk = 1;
      }
      else
         (void)sprintf (src_file, "%s/fid", src_dir);
   }
   /* create the temporary directory for holding the intermediate data files */
   if (access (temp_dir, W_OK) != 0)
   {
      if (errno != ENOENT || mkdir (temp_dir, 0777) == -1)
      {
         Werrprintf ("Can't access temporary directory \"%s\" for data files",
                     temp_dir);
         ABORT;
      }
   }
   /* if pixel data, build the intermediate ".pixdata" file for input
      to the translation program */
   if (pixel_data)
   {
      /* build the intermediate ".pixdata" file */
      which_extent = (pixel_data == 1 ? pix12bits (src_file) :
                                        float12bits (src_file) );
      if (which_extent == -1)
      {
         (void)unlink (temp_dir);
         ABORT;
      }
   }
   /* build the full name of the temporary intermediate (NUMARIS) target file;
      retain a pointer to the end of the directory name, for removing it */
   p_string = temp_dir + strlen(temp_dir);
   *p_string = '/';
   (void)strcpy (p_string + 1, mag_file);

   /* build the command string to convert the source file into
      the intermediate target file in the temporary directory */
   (void)sprintf (convert_cmd, "expsis -e %s %s %s", src_dir, src_file, temp_dir);

   Winfoprintf ("Converting intermediate file \"%s\"", src_file);

   /* if needed, write parameters to disk for the conversion program */
   if (to_disk != 0)
   {
      if (save_parameters() != 0)
         ABORT;
   }
   if (system (convert_cmd) == 0)
   {
      Winfoprintf ("%s.", cnv_endmsg);

      /* build the SunLink network file copy command */
      (void)sprintf (dnicp_cmd, "dnicp -v -u %s %s %s '%s::%s%s'",
                     UserID, PassKey, temp_dir, NodeName, RemoteDir, mag_file);

      /* copy the VNMR data/pixel file from the temporary directory to the VAX */
      if (system (dnicp_cmd) == 0)
      {
         Winfoprintf ("... file transferred to MAGNETOM.");

         /* remove the intermediate target (NUMARIS) file from the
            temporary directory */
         if (keep_files == 0)
            (void)unlink (temp_dir);
      }
      else
         Werrprintf ("... error sending file to MAGNETOM.");
   }
   else
      Werrprintf (cvt_errmsg);

   /* if pixel data, remove the intermediate (".pixdata") source file */
   if (pixel_data)
      if (keep_files == 0)
         (void)unlink (src_file);

   /* terminate the path in the name of the temporary directory,
      then remove the temporary directory */
   *p_string = '\0';
   (void)rmdir (temp_dir);

   /* clear the file selection */
   /*if (picked == 1) */
   /*   clearMarkedSelection(); */

   RETURN;

}  /* end of function "export_files" */

/****************************************************************************
 data_dir

 This function changes the current working directory to the one specified by
 the VNMR global parameter NUMARIS_FILE_DEST. If the parameter is not defined,
 changes to the default (Unix) directory.

 INPUT ARGS:
   argc        The number of arguments from the calling (VNMR) function
               (not used).
   argv        The arguments from the calling (VNMR) function.
               (not used).
   retc        The number of arguments returned to the calling (VNMR) function
               (not used).
   retv        The arguments returned to the calling (VNMR) function
               (not used).
 OUTPUT ARGS:
   none
 RETURN VALUE:
   RETURN      Function completed successfully.
   ABORT       Error occurred changing to data directory.
 GLOBALS USED:
   mag_err_msg Error message "MAGNETOM file transfer not available";
 GLOBALS CHANGED:
   none
 ERRORS:
   ABORT       MAGNETOM software not installed.
   (error msg) Call to "getcwd()" failed.
   (error msg) Call to "chdir()" failed.
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

/*ARGSUSED*/
int data_dir (argc, argv, retc, retv)
 int    argc;
 char **argv;
 int    retc;
 char **retv;
{
   /**************************************************************************
   LOCAL VARIABLES:

   info        A parameter information structure, used for the name of the
               destination directory contained in the GLOBAL parameter
               NUMARIS_FILE_DEST.
   dir_name    The name of the destination directory for files transferred
               from the MAGNETOM, found in GLOBAL parameter NUMARIS_FILE_DEST.
   */
   vInfo info;
   char  dirname[MAXPATH];

   /* see if the magnetom link software is installed */
   if (check_magnetom()==0)
   {
      Werrprintf(mag_err_msg);
      ABORT;
   }
   /* set the destination directory for files transferred from the MAGNETOM */
   if (P_getVarInfo (GLOBAL, NUMARIS_FILE_DEST, &info) != 0 ||
       info.active == 0 ||
       P_getstring (GLOBAL,NUMARIS_FILE_DEST,dirname,1,MAXPATH - 1) != 0 ||
       *dirname == '\0')
   {
      /* if the global parameter was not found, load the name of the current
         working directory */
      /* if (getcwd(dirname, sizeof( dirname ) - 1) == NULL)  */
      if ((pathptr = get_cwd()) == NULL)
      {
         Werrprintf ("data_dir: getcwd() failed");
         ABORT;
      }
      strcpy(dirname, pathptr);
   }
   /* change to the data directory */
   if (chdir (dirname) == -1)
   {
      Werrprintf ("Can't change to data directory \"%s\"", dirname);
      ABORT;
   }
   RETURN;

}  /* end of function "data_dir" */

/****************************************************************************
 get_file_dir

 This function assumes that the input file name is a VNMR "fid" file, and
 tries to find it and its associated "procpar" file. If the files are found,
 the directory path for them is returned. Two anomalies are allowed: if
 "acqfil/fid" or "fid" are input, the returned path will be the current
 experiment directory.

 INPUT ARGS:
   file_dir    A pointer to a buffer for the returned path name.
   file_name   The name of the fid file to find.
 OUTPUT ARGS:
   file_dir    The returned path name.
 RETURN VALUE:
   0           Fid and procpar files found; file_dir contains the path name.
   1,2         "fid" file in current experiment was requested.
   -1          No file name pointer supplied, or file name is empty.
   -2          "fid" file missing.
   -3          "procpar" file missing.
 GLOBALS USED:
   curexpdir   The name of the current experiment directory.
 GLOBALS CHANGED:
   none
 ERRORS:
   return -1   No file name pointer supplied, or file name is empty.
   return -2   Fid file missing.
   return -3   Procpar file missing.
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static int get_file_dir (file_dir, file_name)
 char *file_dir;
 char *file_name;
{
   /**************************************************************************
   LOCAL VARIABLES:

   length   The length of the input filename string.
   flag     A flag for building the path name:
               0 => nothing special.
               1 => "acqfil" in path for "fid", remove for "procpar" file search.
               2 => add "acqfil" to path, then same as 1.
   path1    A buffer for holding the edited file name.
   path2    A buffer for holding the fully-qualified file name.
   ppath2   A pointer into "path2", for editing it for "procpar" file search.
   */
   int   length;
   int   flag = 0;
   char  path1[MAXPATH];
   char  path2[MAXPATH];
   char *ppath2;

   if (file_name == NULL)
      return -1;

   /* scan past leading blanks, and check for something remaining */
   while (isspace (*file_name))
      ++file_name;

   if (*file_name == '\0')
      return -1;

   /* move the name into a buffer for editing, and set the length */
   (void)strcpy (path1, file_name);
   length = strlen(path1);

   /* if the last character is a path separator, eliminate it */
   if (*(path1 + length - 1) == '/')
   {
      *(path1 + length - 1) = '\0';
      --length;
   }

   /* check for a request for the current acquisition fid */
   if (length >= 10 && strcmp (path1 + length - 10, "acqfil/fid") == 0)
      flag = 1;

   /* for short names, check for "fid" (implying the current experiment),
      or append ".fid/fid" (implying a simple file name) */
   else if (length < 4)
   {
      if (strcmp (path1, "fid") == 0)
         flag = 2;
      else
         (void)strcat (path1, ".fid/fid");
   }
   /* build the explicit name of the fid file, if not specified */
   else if (strcmp (path1 + length - 4, "/fid") != 0)
   {
      if (strcmp (path1 + length - 4, ".fid") != 0)
         (void)strcat (path1, ".fid");
      (void)strcat (path1, "/fid");
   }
   /* at this point, there is either "acqfil/fid" or "fid" (flag != 0),
      or a (possibly relative) file name like "h1.fid/fid" */

   /* if a relative path entered, build a full path to the "fid" file */
   if (*path1 != '/')
   {
      /* if the current acquisition file is requested, use the current
         experiment directory */
      if (flag != 0)
      {
         /* prepend the current experiment directory */
         (void)strcpy (path2, curexpdir);
         if (flag == 2)
            (void)strcat (path2, "/acqfil");
      }
      /* otherwise, use the current working directory */
      else
      {
         /* prepend the current working directory */
         /* (void)getcwd (path2, sizeof( path2 ) -1 ); */
	 pathptr = get_cwd();
	 strcpy(path2, pathptr);
      }
      (void)strcat (path2, "/");
      (void)strcat (path2, path1);
   }
   else
      (void)strcpy (path2, path1);

   /* try to access the "fid" file in the specified path */
   if (access (path2, R_OK) == -1)
      return -2;

   /* now build the name for the "procpar" file: find "/fid" string in path */
   ppath2 = strrchr (path2, '/');

   /* remove the "acqfil/" segment if the current acquisition was requested */
   if (flag != 0)
      ppath2 -= 7;

   (void)strcpy (ppath2 + 1, "procpar");

   /* try to access the "procpar" file that accompanies the "fid" file */
   if (access (path2, R_OK) == -1)
      return -3;

   /* eliminate the trailing "/procpar" and return the path */
   *ppath2 = '\0';

   (void)strcpy (file_dir, path2);

   return flag;

}  /* end of function "get_file_dir" */

/****************************************************************************
 set_access

 This function checks that all items required for access to the MAGNETOM
 are set, then initializes the element list structure (for mouse input).

 INPUT ARGS:
   none
 OUTPUT ARGS:
   none
 RETURN VALUE:
   RETURN      All items required for VAX access are set.
   ABORT       Not all items required for VAX access are set
 GLOBALS USED:
   NodeName    The name of the desired network node (MAGNETOM computer).
   UserID    The name of the desired user account (MAGNETOM computer).
   Password    The password for the desired user account (MAGNETOM computer).
   RemoteDir   The name of the desired default directory (MAGNETOM computer).
 GLOBALS CHANGED:
   elist       A pointer to an elementlist structure, for mouse selections.
 ERRORS:
   ABORT       Name of desired network node (MAGNETOM computer) unset
               Name of desired user account (MAGNETOM computer) unset.
               Password for desired user account (MAGNETOM computer) unset.
               Name of desired default directory (MAGNETOM computer) unset.
               Error occurred while initializing element list structure
               (see function "initElementStruct()" ).
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static int set_access()
{
   if (NodeName[0] == '\0' && set_node_name(0,(char**)NULL,0,(char**)NULL) != 0)
      ABORT;
   if (UserID[0] == '\0' && set_user_name(0,(char**)NULL,0,(char**)NULL) != 0)
      ABORT;
   if (Password[0] == '\0' && set_password(0,(char**)NULL, 0, (char**)NULL) != 0)
      ABORT;
   if (RemoteDir[0] == '\0' && set_remote_dir(0,(char**)NULL,0,(char**)NULL) != 0)
      ABORT;

   /* initialize the element list structure, if not set */
   if (elist == NULL && !(elist = initElementStruct()) )
   {
      Werrprintf("set_access: error from 'initElementStruct'");
      ABORT;
   }
   RETURN;

}  /* end of function "set_access" */

/****************************************************************************
 check_file_name

 This function checks for the proper format of the input (VMS) file name:
   1. no VMS wild-card characters ("*?") are allowed.
   2. file name extension is present, and one of the allowed ones.

 INPUT ARGS:
   file_name   The (VMS) file name to check.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   i           The index into the array of allowed VMS file extensions, of
               the extension on the file name.
   -1          File name is not in legal format.
 GLOBALS USED:
   VMS_extents Array of VMS file name extensions, for building file names.
   VMS_extents_count Number of strings in VMS_extents.
   extent_msg  Error message "Illegal file extension!".
 GLOBALS CHANGED:
   none
 ERRORS:
   (error msg) Wild-card characters are not allowed.
   (error msg) Missing file extension.
   (error msg) Illegal file extension
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static int check_file_name (file_name)
 char *file_name;
{
   /**************************************************************************
   LOCAL VARIABLES:

   p_ext    A character pointer for checking the file name extension.
   i        Index for checking the file extensions against the allowed ones.
   */
   char *p_ext;
   int   i;

   /* check for VMS wild-card characters in the file name */
   if (strpbrk (file_name, "*?") != NULL)
   {
      Werrprintf ("Wild-card characters are not allowed!");
      return -1;
   }
   /* check for an extension on the file name */
   if ( (p_ext = strchr (file_name, '.')) == NULL)
   {
      Werrprintf ("Missing file extension!");
      return -1;
   }
   /* point at the first character of the file extension */
   ++p_ext;

   /* check the file extension against the allowed ones */
   for (i = 0; i < VMS_extents_count; ++i)
   {
      if (check_extent (p_ext, VMS_extents[i]) == 0)
         return i;
   }
   Werrprintf (extent_msg);
   return -1;

}  /* end of function "check_file_name" */

/****************************************************************************
 check_extent

 This function does a case-insensitive comparison of the input file name
 extension against the supplied (VMS) extension, which is assumed to be
 upper-case. Return 0 if the extensions match, or the integer difference
 between the mis-matched characters if the extensions don't match.

 INPUT ARGS:
   extent      The file name extension to be checked.
   VMS_extent  The allowed VMS file name extension to check against.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   0           File name extension matches the supplied VMS extension.
   != 0        File name extension does not match the supplied VMS extension.
               File name extension contains extra alphanumeric characters.
 GLOBALS USED:
   extent_msg  Error message "Illegal file extension!".
 GLOBALS CHANGED:
   none
 ERRORS:
   (error msg) Extra alphanumeric characters at end of input file extension.
 EXIT VALUES:
   none
 NOTES:
   Be careful about using this: the VMS extent MUST be upper-case, or this
   function will give a false result.

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static int check_extent (extent, VMS_extent)
 char *extent;
 char *VMS_extent;
{
   /**************************************************************************
   LOCAL VARIABLES:

   c     Contains the upper-case variant of the current character in the file
         name extension.
   */
   char c;

   /* do a case-insensitive comparison of the input extension with the
      required one */
   for ( ; *VMS_extent != '\0'; ++extent, ++VMS_extent)
   {
      c = *extent;
      if (islower(c))
         c = toupper(c);

      if (c != *VMS_extent)
         return (*VMS_extent - c);
   }
   /* any remaining characters should not be alphabetic */
   if (isalnum(*extent))
   {
      Werrprintf (extent_msg);
      return -1;
   }
   return 0;

}  /* end of function "check_extent" */

/*------------------------------------------------------------------------
|       dir_load
|
|  This routine loads up the element structure with the files from the
|  requested directory.  It returns a 0 if everything was ok, 1 if not.
|  A direct copy of "directoryLoad()" in files.c, which depends on a
|  static variable "elist" in that file (standard VNMR shit-for-brains
|  approach to code reusability). You figure it out!
+--------------------------------------------------------------------------*/
static int dir_load (dirpath)
 char *dirpath;
{
   DIR           *dirp;
   struct dirent *dp;

   /* Open the directory and add file names to the element structure */
   if ( (dirp = opendir(dirpath)) != (DIR *)NULL)
   {
      /* must clear out old elist if one already exists */
      if (elist != NULL)
      {
         releaseElementStruct (elist);
         WblankCanvas();
         elist = NULL;
      }
      if (!(elist = initElementStruct()))
      {
         (void)closedir(dirp);
         return 1;
      }
      for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp))
      {
         if (*dp->d_name != '.')     /* no . files in element structure */
         {
            char newname[MAXPATH];

            /* If file is directory, add a slash in front of it */
            sprintf(newname, "%s/%s", dirpath, dp->d_name);
            if (isDirectory(newname))
            {
               char newname[MAXPATH];

               (void) sprintf(newname, "/%s", dp->d_name);
               addElement(elist, newname, "d");
            }
            else
            {
               addElement(elist, dp->d_name, "n");
            }
         }
      }
      (void)closedir(dirp);
      return 0;
   }
   else
   {
      return 1;
   }
}  /* end of function "dir_load" */

char NUM_GRAY_PARAM[] = "numgray";  /* number of gray levels for NUMARIS */

int NUM_GRAY = 4096;  /* default number of gray levels (12 bits) */
int MIN_GRAY = 0;     /* default minimum gray-level value */
int MAX_GRAY = 4095;  /* default maximum gray-level value (12 bits) */

/* VNMR parameter name for the expansion flag for phase data: */
/*    0 = use "vs" scaled to NUM_GRAY levels (default)        */
/*    1 = expand "maxpixel" data value to MAX_GRAY levels     */
/*    2 = expand data min/max to MAX_GRAY levels              */
/*    4 = expand "least max" with 1 bit overhead to "numgray" */
char EXP12BIT_PARAM[] = "exp12bit";

/* for expansion method 1, parameter that contains data value */
/* for maximum display intensity; set by "putpi" macro        */
char MAXPIXEL_PARAM[] = "maxpixel";

/****************************************************************************
 set_gray_range

 This function sets the number of gray levels, and the maximum gray-level
 value, for the pixel conversion/expansion routines.

 INPUT ARGS:
   none
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 GLOBALS USED:
   NUM_GRAY       Number of gray levels (12 bits)
   MAX_GRAY       Maximum gray-level value (12 bits).
   NUM_GRAY_PARAM Name of the VNMR GLOBAL parameter used to set NUM_GRAY
 GLOBALS CHANGED:
   NUM_GRAY       Number of gray levels (12 bits).
   MAX_GRAY       Maximum gray-level value (12 bits).
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:
   The VNMR parameter, the name of which is defined by the NUM_GRAY_PARAM string,
   must be active for NUM_GRAY and MAX_GRAY values to be other than the default.

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static void set_gray_range()
{
   /**************************************************************************
   LOCAL VARIABLES:

   info     VNMR parameter structure.
   temp     For the value of the VNMR parameter.
   */
   vInfo  info;
   double temp;

   if (P_getVarInfo (GLOBAL, NUM_GRAY_PARAM, &info) == 0 && info.active == 1)
   {
      P_getreal (GLOBAL, NUM_GRAY_PARAM, &temp, 1);
      NUM_GRAY = (int)temp;
      MAX_GRAY = NUM_GRAY - 1;
   }
}  /* end of function "set_gray_range" */

/************************************************************************
*									*
*  Charly Gatot								*
*  Spectroscopy Imaging Systems Corporation				*
*  Fremont, CA	94538							*
*									*
*************************************************************************
*									*
*  Description								*
*  -----------								*
*									*
*  This file contains the routines to convert the graphics (screen)	*
*  image to 12 bits pixel data to be used by NUMARIS program.  The 	*
*  result output file will be named "xxxxx" which contains		*
*  a header (struct plainhead) followed by data (16 bits/data) with	*
*  dynamic range from 0 to 4095.  					*
*									*
*  The result pixel data will be inserted into NUMARIS file and be	*
*  displayed into MAGNETOM.  It is assumed that MAGNETOM will be using	*
*  universal colormap.  Its gray level will be range from 0 - 255 with	*
*  256 entries.  Consequently, our conversion program will ignore the	*
*  Vnmr graphics colormap entry.					*
*									*
*  We used a very simple algorithm, linear mapping, to map the (pixel) 	*
*  6 bit graphics image data to 12 bits pixel data.  Since we ignore	*
*  the Vnmr colormap (because of MAGNETOM's universal colormap), 	*
*  changing Vnmr colormap will not change the mapping value.  However, 	*
*  changing the vertical scale (Vnmr's vs) will definitely change the	*
*  result mapping value.						*
*									*
*  The MAGNETOM will only take the data size of 32, 64, 126, 256, or 	*
*  512 in width and height (square image), hence we will round up	*
*  the 12 bits pixel data size to obtain the required size by inserting *
*  black pixels.  For graphics data size greter than the 512, error	*
*  message will be printed out, and no action will be taken.		*
*									*
*************************************************************************/
/* #include <suntool/sunview.h>	*/
/* #include <suntool/canvas.h>	*/
#include <fcntl.h>

/* Structure for plain file header */
typedef struct plainhead
{
   int width;		/* width of data */
   int height;		/* height of data */
   int minimum;		/* minimum of data */
   int maximum;		/* maximum of data */
   int numgray;		/* number of gray levels */
   float fov;		/* field-of-view for this image */
   int slices;		/* always 1 (ignore this) */
}Plainhead;

/* Structure for position */
typedef struct _point
{
   int x,y ;	/* position on the screen */
}Point;

/* extern Canvas canvas; */
extern int mnumxpnts, mnumypnts, right_edge;

/* Functions in this fle */
static char *read_graphics_data();
static short *convert_graphics_data();
static int write_16bits_data();
static void build_lookup_table();
static int find_roundup_size();

/************************************************************************
*									*
*  This function will convert an image from the graphics screen to	*
*  12 bits image data to be used by NUMARIS program.			*
*  The result of the image file will contain a header (struct plainhead)*
*  followed by image data (16 bits/pixel) with the dynamic range from 0 *
*  to 4095.  The data size will be square (width==height) and rounded	*
*  up to 32,64,128,256, and 512.					*
*									*
*  Return 0 for success and -1 for failure.				*
*									*
*  Created  : 07/30/90  Charly Gatot					*
*									*/
static int pix12bits(char *outname)
{
   char *indata;		/* graphics data (as input) */
   short *outdata;		/* (12 bits) pixel data (as output) */
   int width, height;		/* width and height of graphics data */
   int minimum, maximum;	/* minimum and maximum of graphics data */

   /* It allocates image buffer (pointed by indata) and copy graphics   */
   /* screen data into it.						*/
   if ((indata = read_graphics_data(&width, &height)) == NULL)
      return(-1);

   Winfoprintf("Converting pixel begins........");

   /* It allocate 16 bits pixel data (pointed by outdata), convert     */
   /* graphics data (pointed by indata), and store it into "outdata".  */
   /* Note that the width and height might be changed so that the      */
   /* result pixel data will be squarewith its size explained above.   */
   if ((outdata = convert_graphics_data(indata, &width, &height, &minimum, &maximum)) == NULL)
   {
      (void)free(indata);
      return(-1);
   }
   if (write_16bits_data(outname, outdata, width, height, minimum, maximum, 1.0) == -1)
   {
      (void)free(indata);
      (void)free((char *)outdata);
      return(-1);
   }
   Winfoprintf("Converting pixel completes.");

   (void)free(indata);
   (void)free((char *)outdata);

   return(0);

}  /* end of function "pix12bits" */

/************************************************************************
*									*
*  Allocate memory for image buffer and copy the graphics screen image  *
*  into (allocated) image buffer.					*
*									*
*  Return address of data buffer or NULL.				*
*									*
*  Created  : 07/30/90  Charly Gatot.					*
*									*/
static char *
read_graphics_data(width, height)
int *width, *height;	/* width and height of data */
{
   char *indata = NULL;	/* pointer of graphics data address */
   /* Pixrect *pr;	*/	/* Memory pixrect containing image data */
   int stx,sty;		/* Starting point of image data on the screen */
   /* Point p1,p2;	*/	/* Points used by ROI box */

   /* If the box active, get the image size inside the box, else use the */
   /* default size.							 */
/*  if (box_on(&p1, &p2))	*/
/*   { */
/*      stx = p1.x + 1;	*/
/*      sty = p1.y + 1;	*/
/*      *height = p2.y - p1.y - 1;	*/
/*      *width = p2.x - p1.x - 1;	*/

      /* The size of image must be at least 16 x 16 */
/*      if ((*width < 16) || (*height < 16))	*/
/*      {	*/
/*	 Werrprintf("The size of the \"box\" is too small");	*/
/*	 return(NULL);	*/
/*      }	*/
/*   }	*/
/*   else */	/* default image size */
/*   {	*/
      stx = 0;
      sty = 0;

      /* Determize the default size of image */
      *width = mnumxpnts - right_edge;
      *height = mnumypnts;
/*   }	*/
   /* Adjust memory alignment to 4 bytes boundary */
   *width -=  (*width % 4);

   /* Check the image limit size */
   if ((*width > 512) || (*height > 512))
   {
      Werrprintf("Image size is too large. Maximum size is 512 x 512");
      return(NULL);
   }
   /* Create image data buffer */
   /*if ((indata = (char *)malloc((uint)(*width * *height))) == NULL) */
   /*{ */
   /*   Werrprintf("%s malloc: couldn't allocate memory", sub_msg); */
   /*   return(NULL); */
   /*} */
   /* Create memory pixrect */
   /*if ((pr = mem_point(*width, *height, 8, (short *)indata)) == NULL) */
   /*{ */
   /*   free(indata); */
   /*   Werrprintf("%s mem_point: couldn't allocate pixrect"); */
   /*   return(NULL); */
   /*} */
   /* Copy graphics image data into image buffer */
   /*pw_read(pr, 0, 0, *width, *height, PIX_SRC, canvas_pixwin(canvas), */
/*	   stx, sty) ; */

/*   pr_destroy(pr); */

   return(indata);

}  /* end of function "read_graphics_data" */

/************************************************************************
*									*
*  Allocate memory for pixel data, convert graphics data to 12 bits 	*
*  pixel data by using graphics colormap, and store its result into	*
*  pixel buffer.							*
*  Note that the width and height of input data might be changed to	*
*  to be the same value (to produce square image) with its size	rounded	*
*  up to 32, 64, 128, 256, or 512.					*
*									*
*  Return address of data buffer or NULL.				*
*									*
*  Created  : 07/30/90  Charly Gatot					*
*									*/
static short *
convert_graphics_data(indata, width, height, minimum, maximum)
char *indata;		/* input (graphics) data */
int *width, *height;	/* width and height of input image */
int *minimum, *maximum;	/* minimum and maximum pixel values */
{
   short *outdata;		/* output (12 bits pixel) data */
   int table[256];		/* colormap mapping table */
   int outsize;			/* output image size */
   register u_char *pin;	/* pointer of input data */
   register u_short *pout;	/* pointer of output data */
   register int i,j;		/* loop counters */
   int startx, starty;		/* starting location to fill in result data */
   register u_short gmin =  0;	/* minimum pixel value */
   register u_short gmax =  0;	/* maximum pixel value */

   build_lookup_table(table);

   /* Find the round-up legal size.  */
   if (*width > *height)
      outsize = find_roundup_size(*width);
   else
      outsize = find_roundup_size(*height);

   /* Allocate memory for result data */
   if ((outdata=(short *)malloc((uint)(outsize*outsize*sizeof(short)))) == NULL)
   {
      Werrprintf("convert_graphics_data: malloc: couldn't allocate memory");
      return(NULL);
   }
   startx = (outsize - *width)/2;
   starty = (outsize - *height)/2;

   if (startx > 0 || starty > 0)
      gmin = MIN_GRAY;

   pin = (u_char *)indata;
   pout = (u_short *)outdata;

   /* Fill in all the (rounded) top part to be black pixels */
   for (j=0; j<starty; j++)
   {
      for (i=0; i<outsize; i++)
	 *pout++ = MIN_GRAY;
   }
   for (j=0; j<*height; j++)
   {
      /* Fill in all the (rounded) left part to be black pixel */
      for (i=0; i<startx; i++)
	 *pout++ = MIN_GRAY;

      /* Fill in the middle part with mapping pixel result */
      for (i=0; i<*width; i++, pin++, pout++)
      {
	 *pout = table[*pin];
	 if (*pout < gmin)
	    gmin = *pout;
	 if (*pout > gmax)
	    gmax = *pout;
      }

      /* Fill in all the (rounded) right part to be black pixel */
      for (i=0; i<startx; i++)
	 *pout++ = MIN_GRAY;
   }
   /* Fill in all the (rounded) bottom part to be black pixel */
   for (j=(starty + *height); j<outsize; j++)
   {
      for (i=0; i<outsize; i++)
	 *pout++ = MIN_GRAY;
   }
   *width = outsize;
   *height = outsize;
   *minimum = gmin;
   *maximum = gmax;

   return(outdata);

}  /* end of function "convert_graphics_data" */

/************************************************************************
*									*
*  Write the pixel data into xxxxx file.  Its asks 			*
*  confirmation from user if its file exists.				*
*									*
*  Return 0 for success and -1 for failure.				*
*									*
*  Created  : 07/30/90  Charly Gatot					*
*									*/
static int
write_16bits_data(outname, data, width, height, minimum, maximum, fov)
char *outname;		/* name of output file */
short *data;		/* pointer of data */
int width, height;	/* width and height of data */
int minimum, maximum;	/* minimum and maximum of data */
double fov;		/* field-of-view */
{
   static char sub_msg[] = "write_16bits_data:";
   char answer[3];	/* answer from confirmation */
   int fdout;		/* file pointer */
   Plainhead phead;	/* header */

   /* If file already exists, get confirmation from user */
   if (access(outname,F_OK) == 0)
   {
      W_getInput ("File already exists.  Overwrite (Y/N)?", answer,
      sizeof(answer));

      if ((answer[0] != 'Y') && (answer[0] != 'y'))
      {
	 Winfoprintf("Cancel .... no action taken.");
	 return(-1);
      }
   }
   /* Create outfile */
   if ((fdout = open(outname,O_WRONLY | O_CREAT, 0666)) == -1)
   {
      Werrprintf("%s couldn't open %s for writing", sub_msg, outname);
      return(-1);
   }
   /* Write plain header */
   phead.width = width;
   phead.height = height;
   phead.minimum = minimum;
   phead.maximum = maximum;
   phead.numgray = NUM_GRAY;
   phead.fov = (float)fov;
   phead.slices = 1;
   if (write(fdout, (char *)&phead, sizeof(Plainhead)) != sizeof(Plainhead))
   {
      Werrprintf("%s couldn't write header.", sub_msg);
      (void)close(fdout);
      return(-1);
   }
   /* Write image data */
   if (write(fdout, (char *)data, sizeof(short)*width*height) !=
       (sizeof(short)*width*height))
   {
      Werrprintf("%s couldn't write image data", sub_msg);
      (void)close(fdout);
      return(-1);
   }
   (void)close(fdout);

   return(0);

}  /* end of function "write_16bits_data" */

/************************************************************************
*									*
*  Build up lookup table to map Vnmr graphics pixel into 12 bits pixel.	*
*									*
*  Created  : 07/30/90  Charly Gatot					*
*									*/
static void
build_lookup_table(table)
int table[];
{
   register int i;

   /* set the gray-scale range from the parameter tree */
   set_gray_range();

   /* Initialize look-up table to be black */
   for (i=0; i<256; i++)
      table[i] = MIN_GRAY ;

   for (i=0; i<64; i++)
      table[i+48] = i * NUM_GRAY/64 ;

   /* Major colors are WHITE */
   table[1]  = MAX_GRAY; /* color 1 */
   table[5]  = MAX_GRAY; /* CYAN    */
   table[7]  = MAX_GRAY; /* GREEN   */
   table[12] = MAX_GRAY; /* RED     */
   table[9]  = MAX_GRAY; /* YELLOW  */
   table[13] = MAX_GRAY; /* color 2 */
   table[14] = MAX_GRAY; /* MAGENTA */
   table[15] = MAX_GRAY; /* BLUE    */

   /* Make 2D absolute colors to be gray scale */
   for (i=0; i<16; i++)
      table[17+i] = i * 256/16 ;

   /* Make 2D phased display to be gray scale */
   for (i=0; i<15; i++)
      table[33+i] = i * 256/16 + 8;

}  /* end of function "build_lookup_table" */

/************************************************************************
*									*
*  Return the next (rounded up) value of 32, 64, 128, 256, or 512.	*
*									*
*  Created  : 07/30/90  Charly Gatot					*
*									*/
static int
find_roundup_size(val)
int val;
{
   if (val <= 32)
      return(32);
   if (val <= 64)
      return(64);
   if (val <= 128)
      return(128);
   if (val <= 256)
      return(256);
   else
      return(512);

}  /* end of function "find_roundup_size" */

static int   Size;   /* number of points needed to fill out an image line  */
static int   Ximage; /* number of image points in an image line            */
static int   Xdata;  /* number of data points available to fill image line */
static int   Xfill;  /* number of data points to fill with minimum value   */
static short MinVal; /* minimum value for filling out edges of image lines */
static int   D1, D2; /* decision-variable deltas for expanding/compressing */
                     /* data points to fill out image lines                */

/* pointer to function to use for filling out image lines with data points */
void (*LineFill)( /* float *indata, short *outdata, double fm, double fb */ );

/****************************************************************************
 line_same

 This function fits the number of data points into a same-size image line.

 INPUT ARGS:
   indata      A pointer to the start of the data points to be loaded into
               the image line.
   outdata     A pointer to the start of the image line that gets the data.
   fm          The slope factor for expanding data values.
   fb          The intercept factor for expanding data values.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 GLOBALS USED:
   Size        The number of points needed to fill out an image line.
   Ximage      The number of image points in an image line.
   Xdata       The number of data points available for an image line.
   Xfill       The number of data points to fill with minimum value.
   MinVal      The minimum value for filling out edges of image lines.
   MAX_GRAY    The default maximum gray-level value (12 bits).
   MIN_GRAY    The default minimum gray-level value (12 bits).
 GLOBALS CHANGED:
   none
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:
   This function is called via the "LineFill" global variable.

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

void line_same (indata, outdata, fm, fb)
 float *indata;
 short *outdata;
 double fm;
 double fb;
{
   /**************************************************************************
   LOCAL VARIABLES:

   p_in     A fast pointer to the data being loaded into the image line.
   p_out    A fast pointer to the start of the image line.
   max_gray A fast variable to hold the maximum gray-level value.
   min_gray A fast variable to hold the minimum gray-level value.
   val12    A fast variable for testing the converted gray-level value.
   i        A counter for the length of the image line.
   */
   register float *p_in  = indata;
   register short *p_out = outdata;
   register short  max_gray = (short)MAX_GRAY;
   register short  min_gray = (short)MIN_GRAY;
   register short  val12;
   register int    i;

   /* fill the left size of the image line with minimum data values */
   if (Xfill > 0)
      for (i = Xfill; i > 0; --i)
         *p_out++ = MinVal;

   for (i = Xdata; i > 0; --i, ++p_in, ++p_out)
   {
      /* convert a data value to 12 bits, and test against limits */
      if ( (val12 = (short) (fm * (*p_in) + fb)) > max_gray)
         val12 = max_gray;
      else if (val12 < min_gray)
         val12 = min_gray;

      /* load the data value into the output buffer */
      *p_out = val12;
   }
   /* fill the right size of the image line with minimum data values */
   if (Xfill > 0)
      for (i = Size - (Ximage + Xfill); i > 0; --i)
         *p_out++ = MinVal;

}  /* end of function "line_same" */

/****************************************************************************
 line_expand

 This function fits the number of data points into a LONGER image line.

 INPUT ARGS:
   indata      A pointer to the start of the data points to be loaded into
               the image line.
   outdata     A pointer to the start of the image line that gets the data.
   fm          The slope factor for expanding data values.
   fb          The intercept factor for expanding data values.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 GLOBALS USED:
   Size        The number of points needed to fill out an image line.
   Ximage      The number of image points in an image line.
   Xdata       The number of data points available for an image line.
   Xfill       The number of data points to fill with minimum value.
   MinVal      The minimum value for filling out edges of image lines.
   D1, D2      Decision-variable deltas used for expanding/compressing the
               data points to fill out an image line (see Foley & van Dam).
   MAX_GRAY    The default maximum gray-level value (12 bits).
   MIN_GRAY    The default minimum gray-level value (12 bits).
 GLOBALS CHANGED:
   none
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:
   This function is called via the "LineFill" global variable.
   This function uses Bresenham's decision-variable algorithm; for a formal
   development of the method, see Section 11.2.2, "Bresenham's Line Algorithm",
   in Foley & van Dam, "Fundamentals of Interactive Computer Graphics" (1981).

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

void line_expand (indata, outdata, fm, fb)
 float *indata;
 short *outdata;
 double fm;
 double fb;
{
   /**************************************************************************
   LOCAL VARIABLES:

   p_in     A fast pointer to the data being loaded into the image line.
   p_out    A fast pointer to the start of the image line.
   max_gray A fast variable to hold the maximum gray-level value.
   min_gray A fast variable to hold the minimum gray-level value.
   dv       The decision variable (see Foley & van Dam).
   val12    A fast variable for testing the converted gray-level value.
   i        A counter for the length of the image line.
   */
   register float *p_in  = indata;
   register short *p_out = outdata;
   register short  max_gray = (short)MAX_GRAY;
   register short  min_gray = (short)MIN_GRAY;
   register int    dv;
   register short  val12;
   register int    i;

   /* fill the left size of the image line with minimum data values */
   if (Xfill > 0)
      for (i = Xfill; i > 0; --i)
         *p_out++ = MinVal;

   /* set the starting value of the decision variable */
   dv = D1 - Ximage;

   /* convert the first data value to 12 bits, and test against limits */
   if ( (val12 = (short) (fm * (*p_in) + fb)) > max_gray)
      val12 = max_gray;
   else if (val12 < min_gray)
      val12 = min_gray;

   for (i = Ximage; i > 0; --i, ++p_out)
   {
      /* load the data value into the output buffer */
      *p_out = val12;

      /* adjust the decision variable */
      if (dv < 0)
         dv += D1;
      else
      {
         dv += D2;

         /* move to the next input point */
         ++p_in;

         /* convert this data value to 12 bits, and test against limits */
         if ( (val12 = (short) (fm * (*p_in) + fb)) > max_gray)
            val12 = max_gray;
         else if (val12 < min_gray)
            val12 = min_gray;
      }
   }
   /* fill the right size of the image line with minimum data values */
   if (Xfill > 0)
      for (i = Size - (Ximage + Xfill); i > 0; --i)
         *p_out++ = MinVal;

}  /* end of function "line_expand" */

/****************************************************************************
 line_compress

 This function fits the number of data points into a SHORTER image line.

 INPUT ARGS:
   indata      A pointer to the start of the data points to be loaded into
               the image line.
   outdata     A pointer to the start of the image line that gets the data.
   fm          The slope factor for expanding data values.
   fb          The intercept factor for expanding data values.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 GLOBALS USED:
   Ximage      The number of image points in an image line.
   Xdata       The number of data points available for an image line.
   Xfill       The number of data points to fill with minimum value.
   MinVal      The minimum value for filling out edges of image lines.
   D1, D2      Decision-variable deltas used for expanding/compressing the
               data points to fill out an image line (see Foley & van Dam).
   MAX_GRAY    The default maximum gray-level value (12 bits).
   MIN_GRAY    The default minimum gray-level value (12 bits).
 GLOBALS CHANGED:
   none
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:
   This function is called via the "LineFill" global variable.
   This function uses Bresenham's decision-variable algorithm; for a formal
   development of the method, see Section 11.2.2, "Bresenham's Line Algorithm",
   in Foley & van Dam, "Fundamentals of Interactive Computer Graphics" (1981).

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

void line_compress (indata, outdata, fm, fb)
 float *indata;
 short *outdata;
 double fm;
 double fb;
{
   /**************************************************************************
   LOCAL VARIABLES:

   p_in     A fast pointer to the data being loaded into the image line.
   p_out    A fast pointer to the start of the image line.
   max_gray A fast variable to hold the maximum gray-level value.
   min_gray A fast variable to hold the minimum gray-level value.
   dv       The decision variable (see Foley & van Dam).
   val12    A fast variable for testing the converted gray-level value.
   i        A counter for the number of data points to load.
   max_val  The maximum value of an input data point, which causes the
            maximum of sequential deleted data points to be loaded.
   */
   register float *p_in  = indata;
   register short *p_out = outdata;
   register short  max_gray = (short)MAX_GRAY;
   register short  min_gray = (short)MIN_GRAY;
   register int    dv;
   register short  val12;
   register int    i;
   register float  max_val = (float)0.0;

   /* fill the left size of the image line with minimum data values */
   if (Xfill > 0)
      for (i = Xfill; i > 0; --i)
         *p_out++ = MinVal;

   /* set the starting value of the decision variable */
   dv = D1 - Xdata;

   for (i = Xdata; i > 0; --i, ++p_in)
   {
      if (*p_in > max_val)
         max_val = *p_in;

      /* adjust the decision variable */
      if (dv < 0)
         dv += D1;
      else
      {
         dv += D2;

         /* convert a data value to 12 bits, and test against limits */
         if ( (val12 = (short) (fm * (max_val) + fb)) > max_gray)
            val12 = max_gray;
         else if (val12 < min_gray)
            val12 = min_gray;

         /* load the data value into the output buffer, and move to the
            next point */
         *p_out++ = val12;

         max_val = (float)0.0;
      }
   }
   /* fill the right size of the image line with minimum data values */
   if (Xfill > 0)
      for (i = Size - (Ximage + Xfill); i > 0; --i)
         *p_out++ = MinVal;

}  /* end of function "line_compress" */

/****************************************************************************
 line_fill_init

 This function sets the function and control variables used for filling out
 image lines with data points.

 INPUT ARGS:
   size        The number of points needed to fill out an image line.
   x_image     The number of points in an image line to fill with image data.
   x_data      The number of data points available for an image line.
   x_fill      The number of points in an image line to fill with the minimum
               data value.
   min_val     The data value for edge filling.
 OUTPUT ARGS:
   none
 RETURN VALUE:
   none
 GLOBALS USED:
   Size        The number of points needed to fill out an image line.
   Ximage      The number of image points in an image line.
   Xdata       The number of data points available for an image line.
   Xfill       The number of data points to fill with minimum value.
   MinVal      The minimum value for filling out edges of image lines.
   D1, D2      Decision-variable deltas used for expanding/compressing the
               data points to fill out an image line (see Foley & van Dam).
   LineFill    A pointer to the function to use for filling out image lines
               with data points.
 GLOBALS CHANGED:
   Size        The number of points needed to fill out an image line.
   Ximage      The number of image points in an image line.
   Xdata       The number of data points available for an image line.
   Xfill       The number of data points to fill with minimum value.
   MinVal      The minimum value for filling out edges of image lines.
   D1, D2      Decision-variable deltas used for expanding/compressing the
               data points to fill out an image line (see Foley & van Dam).
   LineFill    A pointer to the function to use for filling out image lines
               with data points.
 ERRORS:
   none
 EXIT VALUES:
   none
 NOTES:
   This function uses Bresenham's decision-variable algorithm; for a formal
   development of the method, see Section 11.2.2, "Bresenham's Line Algorithm",
   in Foley & van Dam, "Fundamentals of Interactive Computer Graphics" (1981).

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

void line_fill_init (size, x_image, x_data, x_fill, min_val)
 int size;
 int x_image;
 int x_data;
 int x_fill;
 int min_val;
{
   /**************************************************************************
   LOCAL VARIABLES:

   none
   */

   /* set the sizes of the input and output lines */
   Size   = size;
   Ximage = x_image;
   Xdata  = x_data;
   Xfill  = x_fill;
   MinVal = (short)min_val;

   /* the data points exactly fit the number of display points */
   if (x_data == x_image)
   {
      LineFill = line_same;
   }
   /* expand (repeat some of) the data points to fill out the display points */
   else if (x_data < x_image)
   {
      D1 = 2*x_data;
      D2 = D1 - 2*x_image;

      LineFill = line_expand;
   }
   /* compress (delete some of) the data points to fit the display points */
   else
   {
      D1 = 2*x_image;
      D2 = D1 - 2*x_data;

      LineFill = line_compress;
   }
}  /* end of function "line_fill_init" */

/****************************************************************************
 set_expand_factor

 This function computes the floating-point multiplier to use for expanding
 the VNMR phase data into the desired number of NUMARIS gray levels that
 is set by the VNMR parameter NUM_GRAY_PARAM. If an error occurs, 0 is
 returned.

 INPUT ARGS:
   x_data      The number of data points in the horizontal display direction.
   y_data      The number of data points in the vertical display direction.
   pm          A pointer to the slope factor for expanding data values.
   pb          A pointer to the intercept factor for expanding data values.
   pmin        The address of a variable to hold the minimum pixel value.
   pmax        The address of a variable to hold the maximum pixel value.
 OUTPUT ARGS:
   pm          The slope factor for expanding data values.
   pb          The intercept factor for expanding data values.
   pmin        The minimum pixel value found, set with "IRND()".
   pmax        The maximum pixel value found, set with "IRND()".
 RETURN VALUE:
   (int)       The result of the operation; returns 0 on error.
 GLOBALS USED:
   EXP12BIT_PARAM Name of the VNMR parameter that sets the expansion method.
   vs          The value of VNMR's "vs" (vertical scale) parameter.
   NUM_GRAY    The number of gray-scale levels for converting to integers.
   MAX_GRAY    The maximum gray-scale value for converting to integers.
   graysl      The value of the current "graysl" parameter.
   graycntr    The value of the current "grayctr" parameter.
 GLOBALS CHANGED:
   none
 ERRORS:
   (msg)       Can't read a trace from the VNMR phase file.
 EXIT VALUES:
   none
 NOTES:
   The "threshold to max" method (EXP12BIT_PARAM = 1) computes the data value
   to be placed at maximum intensity from "th"/"vs". It uses a parameter
   MAXPIXEL_PARAM created by the "putpi" macro.
   The "gray-level correction" method (EXP12BIT_PARAM = 3) uses the current
   gray-level center and slope values to compute the equation of a line
   describing the intensity scale: y = mx + b. Along with "vs", these values
   are rescaled to the value of NUM_GRAY_PARAM for the data value conversion.
   The "least maximum" method (EXP12BIT_PARAM = 4) finds the smallest data
   value that results in VNMR maximum gray level, then computes the floating-
   point multiplier that sets that value to a value that takes 1 bit less
   than the requested number of NUMARIS gray levels (that is, 1 bit of gray-
   level overhead).

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

int set_expand_factor (x_data, y_data, pm, pb, pmin, pmax)
 int     x_data;
 int     y_data;
 double *pm;
 double *pb;
 int    *pmin;
 int    *pmax;
{
   /**************************************************************************
   LOCAL VARIABLES:

   sub_msg     The name of this function, for error messages.
   exp_flg     A flag to determine which expansion method to use; set from
               VNMR parameter EXP12BIT_PARAM:
                  0 = use "vs" scaled to NUM_GRAY levels (default)
                  1 = expand "maxpixel" data value to MAX_GRAY levels
                  2 = expand data min/max to MAX_GRAY levels
                  4 = expand "least max" with 1 bit overhead to NUM_GRAY
   m           The floating-point slope expansion value to use.
   b           The floating-point intercept expansion value to use.
   trace       A counter for the trace read from the data.
   indata      A fast pointer to the floating-point data read from the file.
   dtmp        A scratch variable for retrieving parameter values.
   npoints     A fast counter for the number of data points in a trace.
   fmin        A fast variable to hold the minimum data value.
   fmax        A fast variable to hold the maximum data value.
   least_fmax  A fast variable to hold the smallest data value that causes
               maximum VNMR display intensity.
   */
   static   char   sub_msg[] = "set_expand_factor:";
            vInfo  info;
            int    exp_flg = 0;
            double m;
            double b = 0.0;
            int    trace;
            double dtmp;
   register float *indata;
   register int    npoints;
   register float  fmin = 1.0e12;
   register float  fmax = 0.0;
   register float  least_fmax = 1.0e12;

   /* get the expansion flag from the parameter file */
   if (P_getVarInfo (GLOBAL, EXP12BIT_PARAM, &info) == 0 && info.active == 1)
   {
      P_getreal (GLOBAL, EXP12BIT_PARAM, &dtmp, 1);
      exp_flg = (int)dtmp;
   }
   /* find the data min/max, for simple expansions */
   if (exp_flg != 4)
   {
      /* search the data, trace by trace, for the min/max values */
      for (trace = y_data; --trace >= 0; )
      {
         if ( (indata = gettrace (trace, 0)) == (float *)NULL)
            return 0;

         /* search the trace for the min/max data values */
         for (npoints = x_data; npoints-- > 0; ++indata)
         {
            if (*indata > fmax)
               fmax = *indata;
            else if (*indata < fmin)
               fmin = *indata;
         }
      }
   }
   /* use VNMR's "vs" parameter, scaled to the current range (DEFAULT) */
   if (exp_flg == 0)
   {
      m = vs * (double)NUM_GRAY / 64.0;
   }
   else if (exp_flg == 1)
   {
      /* get the parameter containing the user's maximum data value from
         the parameter set */
      if (P_getVarInfo (CURRENT, MAXPIXEL_PARAM, &info) != 0 || info.active != 1)
      {
         Werrprintf ("%s '%s' parameter not available for method %d",
                     sub_msg, MAXPIXEL_PARAM, exp_flg);
         return 0;
      }
      /* set the data value for maximum display intensity */
      P_getreal (CURRENT, MAXPIXEL_PARAM, &dtmp, 1);

      /* expand the threshold value into MAX_GRAY levels */
      m = (double)MAX_GRAY / dtmp;
   }
   /* linear expansion of full dynamic range of data into MAX_GRAY levels */
   else if (exp_flg == 2)
   {
      m = (double)MAX_GRAY / fmax;
   }
   /* use VNMR's "vs" parameter, scaled to the current vertical scale,
      gray-level slope and center (DEFAULT) */
   else if (exp_flg == 3)
   {
      /* compute the intercept from  graycntr = graysl*NUM_GRAY/64 + b
         where graycntr = NUM_GRAY/2 and graysl is scaled to NUM_GRAY levels */
      b = (double)NUM_GRAY*(0.5 - graysl*graycntr/64.0);

      /* compute the slope, with "vs" for data point expansion, so that
         we can use  y = mx + b  */
      m = vs * graysl * (double)NUM_GRAY / 64.0;
   }
   /* find the "least maximum" and maximum data values, then set the
      expansion factor with 1-bit overhead in the gray level */
   else if (exp_flg == 4)
   {
      /* search the data, trace by trace, for the limit values */
      for (trace = y_data; --trace >= 0; )
      {
         if ( (indata = gettrace (trace, 0)) == (float *)NULL)
            return 0;

         /* search the trace for the limit data values */
         for (npoints = x_data; npoints-- > 0; ++indata)
         {
            /* check this point for a maximum VNMR gray level */
            if (vs * (*indata) >= 64.0)
            {
               /* is it the smallest value for the maximum VNMR gray level? */
               if (*indata < least_fmax)
                  least_fmax = *indata;
            }
            /* set the min/max data values */
            if (*indata > fmax)
               fmax = *indata;
            else if (*indata < fmin)
               fmin = *indata;
         }
      }
      /* be sure that the "least maximum" value was set */
      if (least_fmax == 1.0e12)
         least_fmax = fmax;

      /* set the expansion factor to allow 1 bit overhead */
      m = (double)(NUM_GRAY/2 - 1) / least_fmax;
   }
   else
   {
      Werrprintf ("%s illegal expand method %d", sub_msg, exp_flg);
      return 0;
   }
   /* set the expansion slope and offset values */
   *pm = m; *pb = b;

   /* set the min/max pixel values to be found in the data */
   if ( (*pmin = IRND (m * fmin + b)) < MIN_GRAY)
      *pmin = MIN_GRAY;
   if ( (*pmax = IRND (m * fmax + b)) > MAX_GRAY)
      *pmax = MAX_GRAY;

   return 1;

}  /* end of function "set_expand_factor" */

/****************************************************************************
 convert_float_data

 This function converts floating-point data to 12-bit pixel data.

 INPUT ARGS:
   outdata     A pointer to the output pixel buffer.
   size        The dimension of the square image.
   x_data      The number of data points in each row (trace).
   y_data      The number of rows (traces) of data.
   x_fov       The horizontal field-of-view, in cm.
   y_fov       The vertical field-of-view, in cm.
   pmin        The address of a variable to hold the minimum pixel value.
   pmax        The address of a variable to hold the maximum pixel value.
 OUTPUT ARGS:
   pmin        The minimum pixel value found, set with "IRND()".
   pmax        The maximum pixel value found, set with "IRND()".
 RETURN VALUE:
   0           Image successfully converted and loaded.
   -1          Error building output image.
 GLOBALS USED:
   NUM_GRAY    The number of gray-scale levels for converting to integers.
   MAX_GRAY    The maximum gray-scale value for converting to integers.
   LineFill    A pointer to the function used for filling out image lines
               with data points; set by function "line_fill_init()".
 GLOBALS CHANGED:
   none
 ERRORS:
   -1          Can't read data trace: "gettrace()" returned error.
   -1          Can't allocate temporary memory for building image.
 EXIT VALUES:
   none
 NOTES:
   This function uses Bresenham's decision-variable algorithm; for a formal
   development of the method, see Section 11.2.2, "Bresenham's Line Algorithm",
   in Foley & van Dam, "Fundamentals of Interactive Computer Graphics" (1981).

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

int convert_float_data (outdata, size, x_data, y_data, x_fov, y_fov, pmin, pmax)
 short *outdata;
 int    size;
 int    x_data;
 int    y_data;
 double x_fov;
 double y_fov;
 int   *pmin;
 int   *pmax;
{
   /**************************************************************************
   LOCAL VARIABLES:

   sub_msg     The name of this function, for error messages.
   trace       A counter for the trace read from the data.
   m           The slope for the expansion multiplier.
   b           The intercept for the expansion multiplier.
   max_val     A pointer to a buffer used for generating the max data values
               for traces that will be removed during image compression.
   dv, d1, d2  The decision variable and delta's (see Foley & van Dam).
   x_image     The horizontal size of the image, adjusted for aspect ratio.
   x_fill      The number of fill points required at each of an image line.
   y_image     The vertical size of the image, adjusted for aspect ratio.
   y_fill      The number of fill lines required at the image top and bottom.
   indata      A fast pointer to the input data; register is desired since
               the pointer is changed during image compression.
   p_out       A fast pointer to the output buffer; register is desired since
               the pointer is changed during image filling.
   i, j        Counters used for image filling and expansion/compression.
   */
   static   char   sub_msg[] = "convert_float_data:";
            int    trace;
            double m;
            double b;
            float *max_val;
            int    dv, d1, d2;
            int    x_image, x_fill;
            int    y_image, y_fill;
   register float *indata;
   register short *p_out;
            int    i, j;

   /* set the gray-scale range from the parameter tree */
   set_gray_range();

   /* set the data expansion factor for the image */
   if (set_expand_factor (x_data, y_data, &m, &b, pmin, pmax) == 0)
      return -1;

   /* set the aspect ratio for the image */
   if (x_fov < y_fov)
   {
      x_image = IRND ((x_fov/y_fov)*(double)size);
      y_image = size;
   }
   else if (x_fov > y_fov)
   {
      x_image = size;
      y_image = IRND ((y_fov/x_fov)*(double)size);
   }
   else
   {
      x_image = y_image = size;
   }
   /* compute the number of fill points for the edges of the image */
   x_fill = (size - x_image)/2;
   y_fill = (size - y_image)/2;

   /* initialize the line-fill routine */
   line_fill_init (size, x_image, x_data, x_fill, *pmin);

   /* set a fast pointer to the output buffer */
   p_out = outdata;

   /* fill the top rows of the output image with minimum pixel values */
   if (y_fill > 0)
      for (i = y_fill; i > 0; --i)
         for (j = size; j > 0; --j)
            *p_out++ = MinVal;

   /* build a square image from the data: there are 3 possibilities:
      the data traces exactly fit the number of display lines */
   if (y_data == y_image)
   {
      for (trace = y_data; --trace >= 0; )
      {
         if ( (indata = gettrace (trace, 0)) == (float *)NULL)
            return -1;

         (*LineFill)(indata, p_out, m, b);
         p_out += size;
      }
   }
   /* expand (repeat some of) the data traces to fill out the display lines */
   else if (y_data < y_image)
   {
      /* set the adjustment values for the decision variable */
      d1 = 2*y_data;
      d2 = d1 - 2*y_image;

      /* set the starting value of the decision variable */
      dv = d1 - y_image;

      /* traces are numbered [0...y_data - 1] */
      trace = y_data - 1;

      /* starting with the last data trace, load the middle display lines */
      if ( (indata = gettrace (trace, 0)) == (float *)NULL)
         return -1;

      for (i = y_image; i > 0; --i)
      {
         /* load the data trace into the output buffer */
         (*LineFill)(indata, p_out, m, b);
         p_out += size;

         /* adjust the decision variable */
         if (dv < 0)
            dv += d1;
         else
         {
            dv += d2;

            /* get the next data trace */
            if ( (indata = gettrace (--trace, 0)) == (float *)NULL)
               return -1;
         }
      }
   }
   /* compress (delete some of) the data traces to fit into the display lines */
   else
   {
      /* allocate a maximum buffer */
      if ((max_val=(float *)calloc((uint)x_data,sizeof(float))) == (float *)NULL)
      {
         Werrprintf ("%s can't allocate memory for building image", sub_msg);
         return -1;
      }
      /* set the adjustment values for the decision variable */
      d1 = 2*y_image;
      d2 = d1 - 2*y_data;

      /* set the starting value of the decision variable */
      dv = d1 - y_data;

      /* starting with the last data trace, load the display lines */
      for (trace = y_data; --trace >= 0; )
      {
         if ( (indata = gettrace (trace, 0)) == (float *)NULL)
            return -1;

         /* load this trace into the maximum buffer */
         for (i = 1; i <= x_data; ++i)
            if (*(indata+i) > *(max_val+i))
               *(max_val+i) = *(indata+i);

         /* adjust the decision variable */
         if (dv < 0)
            dv += d1;
         else
         {
            dv += d2;

            /* load the maximum buffer into the output buffer */
            (*LineFill)(max_val, p_out, m, b);
            p_out += size;

            /* clear the maximum buffer */
            (void)memset ((char *)max_val, '\0', x_data * sizeof(float));
         }
      }
      free ((char *)max_val);
   }
   /* fill the bottom rows of the output image with minimum pixel values */
   if (y_fill > 0)
      for (i = size - (y_fill + y_image); i > 0; --i)
         for (j = size; j > 0; --j)
            *p_out++ = MinVal;

   return 0;

}  /* end of function "convert_float_data" */

/****************************************************************************
 float12bits

 This function converts floating-point absolute-value data values to 16-bit
 integers.

 INPUT ARGS:
   file        The name of the file to write (pixdata format).
 OUTPUT ARGS:
   none
 RETURN VALUE:
   0           Pixdata file written.
   -1          Error writing file.
 GLOBALS USED:
   phasehead.status   Status of input data file.
   fn          Number of phase data points in the horizontal direction?
   fn1         Number of phase data points in the vertical direction?
 GLOBALS CHANGED:
   none
 ERRORS:
   -1          No data in file (S_DATA flag in header unset).
   -1          Data is not floating-point (S_FLOAT flag in header unset).
   -1          Data is not absolute value (NF_AVMODE flag in header unset).
   -1          Can't allocate memory buffer for output data.
   -1          Can't build file: "convert_float_data()" returned error.
   -1          Output file not written: "write_16bits_data()" returned error.
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

static int float12bits (char *file)
{
   /**************************************************************************
   LOCAL VARIABLES:

   sub_msg     The name of this function, for error messages.
   err_msg     Message for parameter-retrieval errors.
   pe_fov      The name of the VNMR phase-encode parameter.
   ro_fov      The name of the VNMR read-out parameter.
   trace_parm  The name of the VNMR trace parameter.
   status      The file status, from the data file header.
   x_data      The number of columns in the input data.
   y_data      The number of rows in the input data.
   info        A structure for holding VNMR parameter information.
   x_fov       The field-of-view (cm.) for the image's horizontal axis.
   y_fov       The field-of-view (cm.) for the image's vertical axis.
   trace_buff  A buff for holding the value of the trace parameter (string).
   transp      A flag: 1 => image is transposed (trace = 'f2')
   size        The size of a square (NUMARIS) image.
   outdata     A pointer to the buffer used for the output (pixel) data.
   dmax        The maximum integer value read in the data.
   dmin        The minimum integer value read in the data.
   ret_val     The result of the operation in this function.
   */
   static char   sub_msg[] = "float12bits:";
   static char   err_msg[] = "%s can't get value of %s";
   static char   pe_fov[] = "lpe";
   static char   ro_fov[] = "lro";
   static char   trace_parm[] = "trace";
          short  status;
          int    x_data, y_data;
          vInfo  info;
          double x_fov, y_fov;
          char   trace_buff[4];
          int    transp = 0;
          int    size;
          short *outdata;
          int    dmax, dmin;
          int    ret_val;

   /* check the data file for correct information */
   status = phasehead.status & (S_DATA | S_FLOAT | NF_AVMODE);
   if (status != (S_DATA | S_FLOAT | NF_AVMODE) )
   {
      if ( !(status & S_DATA) )
         Werrprintf ("%s no data", sub_msg);
      else if ( !(status & S_FLOAT) )
         Werrprintf ("%s data is not floating-point", sub_msg);
      else
         Werrprintf ("%s data is not absolute value", sub_msg);
      return -1;
   }
   /* set the width and height of the image; these are data dimensions
      set in "init2d()" */
   x_data = fn  / 2;
   y_data = fn1 / 2;

   /* set the dimension for a square image */
   size = find_roundup_size (x_data > y_data ? x_data : y_data);

   /* find out the orientation of the image */
   if (P_getVarInfo (CURRENT, trace_parm, &info) != 0 || info.active != 1)
   {
      Werrprintf (err_msg, sub_msg, trace_parm);
      return -1;
   }
   P_getstring (CURRENT, trace_parm, trace_buff, 1, sizeof(trace_buff) );

   /* set the transposed flag, for switching the field-of-view values for a
      transposed image */
   transp = (strcmp (trace_buff, "f2") == 0);

   /* get the phase-encode field-of-view value (in cm.) */
   if (P_getVarInfo (CURRENT, pe_fov, &info) != 0 || info.active != 1)
   {
      Werrprintf (err_msg, sub_msg, pe_fov);
      return -1;
   }
   P_getreal (CURRENT, pe_fov, (transp ? &y_fov : &x_fov), 1);

   /* get the read-out field-of-view value (in cm.) */
   if (P_getVarInfo (CURRENT, ro_fov, &info) != 0 || info.active != 1)
   {
      Werrprintf (err_msg, sub_msg, ro_fov);
      return -1;
   }
   P_getreal (CURRENT, ro_fov, (transp ? &x_fov : &y_fov), 1);

   /* get space for the output (pixel) data */
   if ((outdata=(short *)malloc((uint)(size*size*sizeof(short))))==(short *)NULL)
   {
      Werrprintf ("%s can't allocate space for output data", sub_msg);
      return -1;
   }
   /* convert the data traces to 12-bit pixel values */
   if ( (ret_val = convert_float_data (outdata, size, x_data, y_data,
                                       x_fov, y_fov, &dmin, &dmax)) == 0)
   {
      /* write the pixel file */
      ret_val = write_16bits_data (file, outdata, size, size, dmin, dmax,
                                   (x_fov > y_fov ? x_fov : y_fov));
   }
   /* free the buffer used for the data conversion */
   (void)free ((char *)outdata);

   return ret_val;

}  /* end of function "float12bits" */

/****************************************************************************
 output_sdf

 This function outputs a (data or) phasefile to a file. The file will be
 output line by line.  The data output will default to single precision
 floating point format.  Initially it should output single precision
 floating point and the magnetom (12bit) 16bit integer format.
 entry point:
   output_sdf   (target_name [,format] )
		format = 'f' 32-bit float
			 'm' 16-bit magnetom
			 'b' 8-bit byte
 INPUT ARGS:
   argc        The number of arguments from the calling (VNMR) function.
   argv        The arguments from the calling (VNMR) function: the second
               argument is the name of the Unix file on the Sun;
	       the third argument is (optionally) a format code.
   retc        The number of arguments returned to the calling (VNMR) function
               (not used).
   retv        The arguments returned to the calling (VNMR) function
               (not used).
 OUTPUT ARGS:
   none
 RETURN VALUE:
   RETURN      Function completed successfully.
   ABORT       Error occurred outputing file.
 GLOBALS USED:
   fn          Number of phase data points in the horizontal direction?
   fn1         Number of phase data points in the vertical direction?
   vs          The value of VNMR's "vs" (vertical scale) parameter.
   NUM_GRAY    The number of gray-scale levels for converting to integers.
   MAX_GRAY    The maximum gray-scale value for converting to integers.
   graysl      The value of the current "graysl" parameter.
   graycntr    The value of the current "grayctr" parameter.
 GLOBALS CHANGED:
 ERRORS:
   ABORT       No file name was specified.
               No name specified for target file.
               Invalid format for name of target data file.
 EXIT VALUES:
   none
 NOTES:

 David Woodworth
 Spectroscopy Imaging Systems Corporation
 Fremont, California
*/

/*ARGSUSED*/
int output_sdf (argc, argv, retc, retv)
 int    argc;
 char **argv;
 int    retc;
 char **retv;
{
   /**************************************************************************
   LOCAL VARIABLES:

   outfile        The name of the output data file.
   fdout;         File handle for the output data file.
   buff           A buffer for holding user responses.
   which_form     The form of data to be written: 'f' => float
                                                  'm' or 'i' => short integer
                                                  'b' => byte
   trace          A counter for the traces in the data set.
   x_data         The number of horizontal data points in each trace.
   y_data         The number of vertical data points (traces).
   indata         A pointer to each trace of the input data.
   i              General-purpose counter of things.
   file_pmt       User prompt: "Enter the name of the output file: ";
   form_pmt       User prompt: "Enter the format of the output file (f/m): ";
   */
   char outfile [MAXPATH];
   int  fdout;
   char buff [32];
   char which_form = '\0';
   int  trace;
   int  x_data,
        y_data;
   int  i;
   register float *indata;
   static char file_pmt[] = "Enter the name of the output file: ";
   static char form_pmt[] = "Enter the format of the output file (f/m/b): ";
   int ftrace;
   int ntraces;
   int fpix;

   /* get the name for the target file */
   if (argc == 1 || *(argv+1) == NULL || **(argv+1) == '\0')
   {
      if (W_getInput (file_pmt, outfile, sizeof(outfile)) == NULL ||
          outfile[0] == '\0')
         ABORT;
   }
   else
      (void)strcpy (outfile, *(argv+1));

   /* check for accessibility of the output file */
   if (access (outfile, W_OK) == 0)
   {
      W_getInput ("File already exists. Overwrite (Y/N)?", buff, sizeof(buff));
      if (buff[0] != 'Y' && buff[0] != 'y')
      {
         Winfoprintf ("Cancelled");
         ABORT;
      }
   }
   else if (errno != ENOENT)
   {
      Werrprintf ("Can't access output file \"%s\"", outfile);
      ABORT;
   }

   set_gray_range();

   P_getreal (CURRENT, "grayctr", &graycntr, 1);

   /* get the form of output data: 32-bit floating point or 12-bit integer */
   if (argc >= 3 && *(argv+2) != NULL)
      which_form = **(argv + 2);

   while ( (which_form & 0xDF) != 'F' && (which_form & 0xDF) != 'M' && 
		(which_form & 0xDF) != 'I' && (which_form & 0xDF) != 'B')
   {
      if (W_getInput (form_pmt, buff, sizeof(buff)) == NULL || buff[0] == '\0')
         ABORT;
      which_form = (buff[0] & 0xDF);
   }

   if (argc >= 4 && argv[3] != NULL && (strcmp(argv[3], "all") == 0)) {
       ftrace = 0;
       ntraces = fn1 / 2;
       fpix = 0;
   } else {
       ftrace = fpnt1;
       ntraces = npnt1;
       fpix = fpnt;
   }

   /* open the output file for writing */
   if ( (fdout = open (outfile, O_WRONLY|O_CREAT|O_TRUNC, 0666)) == -1)
   {
      Werrprintf ("Couldn't open output file \"%s\" for writing", outfile);
      ABORT;
   }
   /* set the width and height of the image; these are data dimensions
      set in "init2d()" */
   x_data = fn  / 2;
   y_data = fn1 / 2;

   /* write data in floating-point format */
   if ( (which_form & 0xDF) == 'F')
   {
      /* for (trace = y_data; --trace >= 0; ) */
      for (trace = ftrace+ntraces; --trace >=ftrace;) 
      {
         /* get a data trace */
         /* if ( (indata = gettrace (trace, 0)) == (float *)NULL) */
         if ( (indata = gettrace (trace, fpix)) == (float *)NULL)
            RETURN;

         if (trace % 8 == 0)
            disp_index (trace);

         /* write the floating-point data to the file */
         if (write (fdout, indata, x_data*sizeof(float)) == -1)
         {
            Werrprintf ("Error writing floating-point data to file \"%s\"",
                        outfile);
            ABORT;
         }
      }
   }
   /* write data in integer format: 12 significant bits in 16-bit integers */
   else if ( ((which_form & 0xDF) == 'M') || ((which_form & 0xDF) == 'I') )
   {
      float  m = 1.0,        /* slope for int conversion function */
             b = 0.0;        /* intercept for int conversion function */
      short *outdata;        /* buffer for int conversion */
      register short *pout;  /* pointer into buffer for int conversion */

      /* compute the intercept from  graycntr = graysl*NUM_GRAY/64 + b	     */
      /* where graycntr = NUM_GRAY/2 and graysl is scaled to NUM_GRAY levels */
      if (graycntr < 0.5) graycntr=32.0;
      b = (float)(NUM_GRAY*(0.5 - graysl*graycntr/64.0));

      /* compute the slope, with "vs" for data point expansion, so that
         we can use  y = mx + b  */
      m = (vs * graysl * (double)NUM_GRAY / 64.0);

      if ( (outdata = (short *)malloc(x_data*sizeof(short))) == NULL)
      {
         Werrprintf ("Can't allocate space for output integer data");
         ABORT;
      }
      /* for (trace = y_data; --trace >= 0; ) */
      for (trace = ftrace+ntraces; --trace >=ftrace;)
      {
         /* get a data trace */
         /* if ( (indata = gettrace (trace, 0)) == (float *)NULL) */
         if ( (indata = gettrace (trace, fpix)) == (float *)NULL)
            RETURN;

         if (trace % 8 == 0)
            disp_index (trace);

         /* convert the points to integer format, scaled to the range */
         for (pout = outdata, i = 0; i < x_data; ++i, ++indata, ++pout)
         {
            if ( (*pout = (short)(m*(*indata) + b)) > MAX_GRAY)
               *pout = MAX_GRAY;
            else if (*pout < MIN_GRAY)
               *pout = MIN_GRAY;
         }
         /* write the integer data to the file */
         if (write (fdout, outdata, x_data*sizeof(short)) == -1)
         {
            Werrprintf ("Error writing integer data to file \"%s\"", outfile);
            ABORT;
         }
      }
      /* free the conversion buffer */
      free (outdata);
   }
   /* Assume byte format: 8-bit integers */
   else 
   {
      float  m = 1.0,        /* slope for int conversion function */
             b = 0.0;        /* intercept for int conversion function */
      char *outdata;        /* buffer for int conversion */
      char *pout;  /* pointer into buffer for int conversion */
      int ptmp;
      int num_gray_byte=256;
      int max_gray_byte=255;

      /* compute the intercept from  graycntr = graysl*num_gray_byte/64 + b
         where graycntr = num_gray_byte/2 and graysl is scaled to num_gray_byte levels */
      if (graycntr < 0.5) graycntr=32.0;
      b = (float)(num_gray_byte*(0.5 - graysl*graycntr/64.0));

      /* compute the slope, with "vs" for data point expansion, so that
         we can use  y = mx + b  */
      m = (vs * graysl * (double)num_gray_byte / 64.0);

      if ( (outdata = (char *)malloc(x_data*sizeof(char))) == NULL)
      {
         Werrprintf ("Can't allocate space for output integer data");
         ABORT;
      }
      /* for (trace = y_data; --trace >= 0; ) */
      for (trace = ftrace+ntraces; --trace >=ftrace;)
      {
         /* get a data trace */
         /* if ( (indata = gettrace (trace, 0)) == (float *)NULL) */
         if ( (indata = gettrace (trace, fpix)) == (float *)NULL)
            RETURN;

         if (trace % 8 == 0)
            disp_index (trace);

         /* convert the points to integer format, scaled to the range */
         for (pout = outdata, i = 0; i < x_data; ++i, ++indata, ++pout)
         {
            if ( (ptmp = (int)(m*(*indata) + b)) > max_gray_byte)
               ptmp = max_gray_byte;
            else if (ptmp < MIN_GRAY)
               ptmp = MIN_GRAY;
	    *pout = (char)ptmp;
         }
         /* write the integer data to the file */
         if (write (fdout, outdata, x_data*sizeof(char)) == -1)
         {
            Werrprintf ("Error writing integer data to file \"%s\"", outfile);
            ABORT;
         }
      }
      /* free the conversion buffer */
      free (outdata);
   }
   /* close the output file */
   close (fdout);

   RETURN;

}  /* end of function "output_sdf" */
