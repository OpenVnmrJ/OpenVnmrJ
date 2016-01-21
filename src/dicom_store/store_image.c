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

/********************************************************************/
/* Name        : store_image.c                                      */
/* Description : Stores image in the image server using the         */
/*                     CTN program : send_image                     */
/* Created by  : Mindteck (India) limited                           */
/* Created on  : 01.05.2003                                         */
/* Modified on : 03-16-2010 M.R. Kritzer                            */
/*             : changed to use dcmtk tools                         */
/********************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "constants.h"


/*************************************************************************/
/* Function    : UsageError                                              */
/* Description : The function displays 'Function usage'                  */
/*************************************************************************/
static void UsageError()
{
    char msg[] = "Usage: [-v -p] dicom_image,\n\
          \t-v  Verbose mode\n\
          \t-p  Ping server only \n\
          \tdicom_image = full path of image\n";
    printf("%s", msg);
    return; 
}

/*************************************************************************/
/* Function    : WriteToLog                                              */
/* Description : The function writes log in file ../log/error.log        */
/*************************************************************************/

int WriteToLog (char* strMsg)
{
  FILE *fpServerLog;
  char *log;
  char strLogFile[LOG_FILE_NAME_LEN];

/* Create/Open the log file in append mode */
/*
  strcpy (strLogFile, STORE_IMAGE_LOG);
*/

  log = (char *)getenv("LOGFILE");
  if (log == NULL || (int) strlen(log) <= 0)
     return ERROR_FILE_OPEN;
  strcpy (strLogFile, log);

  if ((fpServerLog = fopen (strLogFile, "a+")) == NULL)
  {
     printf ("Cannot open the log file : %s\n\n", strLogFile);
     return ERROR_FILE_OPEN;
  }

  fprintf (fpServerLog, "start_server:\t%s\n\n", strMsg);

  fclose(fpServerLog);
  return SUCCESS;
}



/************************************************************************/
/* Function    : main                                                   */
/* Description : The function reads the configuration file              */
/*               - <installed-directory>/dicom/mr/conf/store_image.conf,*/
/*               and prepares the command line arguments for the CTN    */
/*               program send_image. Next it executes the CTN           */
/*               program to send image to server application.           */
/*               The send_image program not only stores the DICOM       */
/*               file in the specified directory, but also stores       */
/*               some of the header information in DICOM file in the    */
/*               database.                                              */
/*               This application is to be run in the Solaris           */
/* Parameters  : i) The full path of the image to be stored in server   */
/************************************************************************/

int main (int argc, char *argv[])
{
/* Declare variables */
  static BOOLEAN bQuiet;
  static BOOLEAN bPing;
  static char strTag[TAG_LEN];
  static char strValue[VALUE_LEN];
  static char strAETitleExternal[AE_TITLE_LEN];
  static char strAETitleCTN[AE_TITLE_LEN];
  static char strIP[IP_LEN];
  static char strPort[PORT_LEN];
  static char strCTNArgument[CTN_ARGUMENT_LEN];
  static char strPath[PATH_LEN];
  static char strCommand[COMMAND_LEN];
  static char strImage[IMAGE_NAME_LEN];
  static char strMsg[MSG_LEN];
  static char strConfFile[PATH_LEN];
  static char strBuffer[BUFFER_LEN+2];
  static char *dcmFile;
  char   *tmp;
  FILE *fpConf;
  static int i=0;
  static int ret;

/* Initialize variables                                                     */
  strCTNArgument[0]    ='\0'; /* The argument list of CTN command           */
  strCommand[0]        ='\0'; /* The CTN command to start image server      */
  strTag[0]            ='\0'; /* Tag of the parameter in configuration file */
  strValue[0]          ='\0'; /* Value of the corresponding tag             */  
  strImage[0]          ='\0'; /* Image sent to the server                   */
  bQuiet               = TRUE;/* send_image will not run in quiet mode      */
  bPing                = FALSE;

/* Set default values to tag-values */
/* AE title of Client Application             */
  strcpy (strAETitleExternal, DEFAULT_STR_AE_TITLE_EXTERNAL);
/* AE Title of CTN Application                */
  strcpy (strAETitleCTN, DEFAULT_STR_AE_TITLE_CTN);
/* IP address of the system                   */
  strcpy (strIP, DEFAULT_STR_HOST);
/* TCP/IP port of the system                  */
  strcpy (strPort, DEFAULT_STR_PORT);
/* The path of CTN binary                     */
  strcpy (strPath, DEFAULT_STR_CTN_BIN_PATH);

/* Check for correct usage of the application */
  if( argc < 2  ) 
  {
      UsageError();
      return ERROR_USAGE; 
  }
  dcmFile = NULL;
  i = 1;
  while (i < argc && argv[i][0] == '-') {
      switch (*(argv[i] + 1)) {
        case 'v':
                 bQuiet = FALSE;
                 break;
        case 'p':
                 bPing = TRUE;
                 break;
        default:
                 UsageError();
                 return ERROR_USAGE;
                 break;
      }
      i++;
  }
  if (i >= argc) {
      if (!bPing) {
        UsageError();
        return ERROR_USAGE; 
      }
  }
  else
      dcmFile = argv[i];
  if (dcmFile == NULL) {
      if (bPing)
	 dcmFile = "dummy";
      else
         return ERROR_USAGE; 
  }


  /* NB: these were set by the script dicom_store */
  tmp = (char *)getenv("DICOM_BIN_DIR");
  if (tmp != NULL) {
      sprintf (strPath, "%s/", tmp);
  }
  tmp = (char *)getenv("DICOM_CONF_DIR");
  if (tmp != NULL)
      sprintf (strConfFile, "%s/%s", tmp, STORE_IMAGE_CONF);
  else
      sprintf (strConfFile, "%s", STORE_IMAGE_CONF);

/* Open configuration file */
  fpConf = fopen (strConfFile, "r");
  if (fpConf == NULL) 
  {
     strMsg[0]='\0';
     sprintf (strMsg, "Cannot open configuration file : %s", STORE_IMAGE_CONF);
     WriteToLog (strMsg);
  }

  if (fpConf != NULL)
  {
/* Read the configuration file, if it exists */
     while (!feof (fpConf) )
     {
      	   if (feof(fpConf) != 0)
      	   {
          	break;
      	   }

      	   strTag[0]=0;
      	   strValue[0]=0;
      	   strBuffer[0]=0;
/* Read each line of the configuration file */
      	   fgets (strBuffer, BUFFER_LEN, fpConf);

/* Skip lines which starts with '#' */
      	   if (strBuffer[0]=='#' )
           {
               continue;
           }

/* Assign values to tags and its value */
      	   sscanf (strBuffer, "%s : %s", strTag, strValue);

      	   if(strcmp (strTag, "SCU_TITLE") == 0)
      	   {
/* Read the AE Title of Client Application from the configuration file */
         	sprintf (strAETitleExternal, " -aet %s ", strValue);
      	   }
      	   else if(strcmp (strTag, "AE_TITLE") == 0)
      	   {
/* Read the AE Title of CTN Application from the configuration file */
         	sprintf (strAETitleCTN, " -aec %s ", strValue);
      	   }
      	   else if(strcmp (strTag, "HOST") == 0)
      	   {
/* Read the TCP/IP address from the configuration file */
         	sprintf (strIP, " %s ", strValue);
      	   }
      	   else if(strcmp (strTag, "PORT") == 0)
      	   {
/* Read the TCP/IP port from the configuration file */
         	sprintf (strPort, " %s ", strValue);
      	   }
     } 
     fclose(fpConf);
  }

/* Create the argument list for the CTN command : send_image */
  if (bPing)
    // sprintf (strCTNArgument, "%s %s %s %s ", strAETitleExternal, strAETitleCTN, strIP, strPort);  
     sprintf (strCTNArgument, " %s %s  %s %s", strIP, strPort, strAETitleCTN, strAETitleExternal);  
  else
        // sprintf (strCTNArgument, "%s %s %s %s %s ", strAETitleExternal, strAETitleCTN, strIP, strPort, dcmFile);  
     sprintf (strCTNArgument, " %s %s %s %s ", strIP, strPort, strAETitleCTN,strAETitleExternal);  
  // sprintf (strCTNArgument, " %s %s %s %s %s/*.dcm ", strIP, strPort, strAETitleCTN,strAETitleExternal, dcmFile);  



/* Create the CTN command, along with arguments which is to be executed   */

/* The variable 'bQuiet' is preserved for later use, presently the        */
/* application is always run in non-quiet mode                            */

  if (bQuiet)
  {
      if (bPing)
	//         sprintf (strCommand, "%sdicom_echo -r 0 %s ", strPath, strCTNArgument);
         sprintf (strCommand, "%sechoscu %s ", strPath, strCTNArgument);
      else
	//         sprintf (strCommand, "%ssend_image -q %s ", strPath, strCTNArgument);
	//  sprintf (strCommand, "%sstorescu  %s ", strPath, strCTNArgument);
	sprintf (strCommand, "find %s -name \"*.dcm\" | xargs %sstorescu %s ", dcmFile, strPath, strCTNArgument); 
 }
  else
  {
      if (bPing)
	//         sprintf (strCommand, "%sdicom_echo -r 0 -v %s ", strPath, strCTNArgument);
         sprintf (strCommand, "%sechoscu -v %s ", strPath, strCTNArgument);
      else
	//         sprintf (strCommand, "%ssend_image  -v %s ", strPath, strCTNArgument);
	//  sprintf (strCommand, "%sstorescu  -v %s ", strPath, strCTNArgument);
	sprintf (strCommand, "find %s -name \"*.dcm\" | xargs %sstorescu  -v %s ", dcmFile, strPath, strCTNArgument);
  }
  
  strMsg[0]='\0';
  sprintf (strMsg, "Attempting to send image...\n\tExecuting ... %s", strCommand);
  WriteToLog (strMsg);



/* Execute the CTN command 'send_image' to send images in server */
   ret = system (strCommand);     
   if (ret == SUCCESS)
      ret = 0;
   else
      ret = 1;
   printf("%d\n", ret);
   return (ret);
}
