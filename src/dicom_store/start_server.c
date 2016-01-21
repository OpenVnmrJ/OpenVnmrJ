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

/************************************************************************/
/* Name        : start_server.c                                         */
/* Description : Starts the image archive server using the              */
/*                     CTN program : image_server                       */
/* Created by  : Mindteck (India) Limited                               */
/* Created on  : 01.05.2003                                             */
/* Modified on :                                                        */
/************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "constants.h"

/*************************************************************************/
/* Function    : UsageError                                              */
/* Description : The function displays 'Function usage'                  */
/*************************************************************************/
static void UsageError(void)
{
    char msg[] = "Usage:  start_server\n";
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
  strcpy (strLogFile, START_SERVER_LOG);
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

/* Closes the log file */
  if (fpServerLog != NULL)
  {
      fclose(fpServerLog);
  }
  return SUCCESS;

}

/*************************************************************************/
/* Function    : main                                                    */
/* Description : The function reads the configuration file               */
/*               - <installed-directory>/dicom/mr/conf/start_server.conf,*/
/*               and prepares the command line arguments for the CTN     */
/*               program image_server. Next it executes the CTN          */
/*               program to start the image server application. The      */
/*               image server is to be run in the Solaris machine        */
/*************************************************************************/

int main (int argc, char *argv[])
{
/* Declare variables */
  static char strTag[TAG_LEN]; 
  static char strValue[VALUE_LEN]; 
  static char strConfigurationFile[PATH_LEN];
  static char strControlDatabase[256];
  static char strLog[LOG_FILE_NAME_LEN];
  static char strIP[IP_LEN];
  static char strHost[IP_LEN];
  static char strPort[PORT_LEN];
  static char strCTNArgument[CTN_ARGUMENT_LEN];
  static char strPath[PATH_LEN];
  static char strCommand[COMMAND_LEN];
  static char strMsg[MSG_LEN];
  static char strBuffer[BUFFER_LEN];
  char *tmp;
  FILE *fpConf;

/* Initialize variables                                             */
  strCTNArgument[0]='\0';  /* The argument list of CTN command      */
  strCommand[0]    ='\0';  /* The CTN command to start image server */
  strTag[0]        ='\0';  /* Tag of the parameter in configuration file */
  strValue[0]      ='\0';  /* Value of the corresponding tag        */  
  strMsg[0]        ='\0';  /* Error message to be written in log    */  
  strBuffer[0]     ='\0';  /* Buffer                                */

/* Set default values to tag-values */
  strcpy (strControlDatabase, DEFAULT_STR_DB);
  strcpy (strLog,  DEFAULT_STR_LOG);
  strcpy (strHost, DEFAULT_STR_HOST);
  strcpy (strPort, DEFAULT_STR_PORT);
  strcpy (strPath, DEFAULT_STR_CTN_BIN_PATH);

/* Check for correct usage of the application */
  if( argc != 1)
  {
      UsageError();
      return ERROR_USAGE;
  }

  tmp = (char *)getenv("DICOM_BIN_DIR");
  if (tmp != NULL)
     strcpy (strPath, tmp);
/* Open the configuration file */
  tmp = (char *)getenv("DICOM_CONF_DIR");
  if (tmp != NULL) {
      sprintf (strConfigurationFile, "%s/%s", tmp, STORE_IMAGE_CONF);
  }
  else
      sprintf (strConfigurationFile, "%s", STORE_IMAGE_CONF);
  if ((fpConf = fopen (strConfigurationFile, "r")) == NULL)
  {
     strMsg[0]='\0';
     sprintf (strMsg, "Cannot open configuration file : %s", strConfigurationFile);
     WriteToLog (strMsg);
  }

  if (fpConf != NULL)
  {
/* Read the configuration file, if it exists */
    while (!feof (fpConf))
    {
      if (feof(fpConf) != 0)
      {
          break;
      }

      strTag[0]=0;
      strValue[0]=0;
      strBuffer[0]=0;
/* Read each line of the configuration file */
      fgets (strBuffer, 80, fpConf);

/* Skip lines which starts with '#' */
      if (strBuffer[0]=='#' )
      {
          continue;
      }

/* Assign values to tags and its value */
      sscanf (strBuffer, "%s : %s", strTag, strValue);

      if (strcmp (strTag, "DB") == 0)
      {
/* Read control database name from the configuration file */
         sprintf (strControlDatabase, "-f %s ", strValue);
      }
      else if (strcmp (strTag, "HOST") == 0)
      {
/* Read the TCP/IP address of server from the configuration file */
         sprintf (strHost, " -n %s ", strValue);
      }
      else if (strcmp (strTag, "PORT") == 0)
      {
/* Read TCP/IP port of server from the configuration file */
         sprintf (strPort, " -i -q -v %s ", strValue); /* -i -v */
      }
      else if (strcmp (strTag, "LOG") == 0)
      {
/* Read log file name from the configuration file */
         sprintf (strLog, " -l %s", strValue);
      }
    } 
/* Close the configuration file */
    fclose (fpConf);
       
  }

/* Create the argument list for the CTN command : image_server */
  sprintf (strCTNArgument, "%s %s %s %s ", strControlDatabase, strHost, strLog, strPort);  
 
/* Create the CTN command, along with arguments which is to be executed */
  sprintf (strCommand, "%s/image_server -s %s ", strPath, strCTNArgument);

  strMsg[0]='\0';
  sprintf (strMsg, "Attempting to start image Server...\n\tExecuting ... %s", strCommand);
  WriteToLog (strMsg);
  
/* Execute the CTN command 'image_server', to start the image archive server */
  system (strCommand);  

  strMsg[0]='\0';
  sprintf (strMsg, "Image Server is stopped", strHost);
  WriteToLog (strMsg);

  return SUCCESS;
}
