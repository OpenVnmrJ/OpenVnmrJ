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
/* Name        : create_fdf_dicom.c                                     */
/* Description : Wrapper to CTN functions which creates a DICOM file*/
/*                                                 in part 10 format*/
/* Created by  : Mindteck (India) limited                           */
/* Created on  : 01.05.2003                                         */
/* Modified on :                                                    */
/********************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "constants.h"


/*************************************************************************/
/* Function    : UsageError                                              */
/* Description : The function displays 'Function usage'                  */
/*************************************************************************/
static void UsageError()
{
    char msg[] = "Usage:  create_fdf_dicom headerfile pixelfile dicomfile\n\
         headerfile, the input file \n\
         pixelfile,  the binary file from which raw pixel data is read \n\
         dicomfile,  path of the DICOM file to be created \n";
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
  strcpy (strLogFile, CREATE_DICOM_LOG);
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

  fprintf (fpServerLog, "create_fdf_dicom:\t%s\n\n", strMsg);

/* Closes the log file */
  if (fpServerLog != NULL)
  {
      fclose(fpServerLog);
  }
  return SUCCESS;

}

/************************************************************************/
/* Function    : main                                                   */
/* Description : The function creates a DICOM file by executing CTN     */
/*               programs dcm_create_object and dcm_ctnto10. A temporary*/
/*               DICOM file is created by dcm_create_object in bin      */
/*               directory. The name of this temporary file is temp.dcm.*/
/*               This file is used to create the DICOM file, with name  */
/*               given in output parameters, in part 10 format.         */
/* Parameters  : i)   header information data                           */
/*               ii)  image pixel data                                  */
/*               iii) the name of the dcm file                          */
/************************************************************************/

int main (int argc, char *argv[])
{
/* Declare variables */
  FILE *fpHeader;
  FILE *fpData;
  FILE *fpTemp;

  char strTag[TAG_LEN];
  char strValue[VALUE_LEN];
  char strHeaderDataFile[PATH_LEN];
  char strImageDataFile[PATH_LEN];
  char strOutputFile[PATH_LEN];
  char strLog[LOG_FILE_NAME_LEN];
  char strIP[IP_LEN];
  char strPort[PORT_LEN];
  char strCTNArgument[CTN_ARGUMENT_LEN];
  char strPath[PATH_LEN];
  char strCommand[COMMAND_LEN];
  char *tmp;
  static char strMsg[MSG_LEN];
  static char strBuffer[BUFFER_LEN];
  static char strTemporaryFile[PATH_LEN];
  int i=0;

/* Initialize variables    */
  strLog[0]            ='\0';
  strIP[0]             ='\0';
  strPort[0]           ='\0';
  strPath[0]           ='\0';
  strOutputFile[0]     ='\0';

/* Check for correct usage of the application */
  if (argc != 4) 
  {
      UsageError();
      return ERROR_USAGE;
  }
  
/* Name of the output DICOM file   */
  strcpy (strOutputFile, argv[3]);
/* Name of pixel data file         */
  strcpy (strImageDataFile, argv[2]);
/* Name of header information file */
  strcpy (strHeaderDataFile, argv[1]);

/* Open header file */
  if ((fpHeader = fopen (strHeaderDataFile, "r")) == NULL) 
  {
     strMsg[0]='\0';
     sprintf (strMsg, "Cannot open header file : %s", strHeaderDataFile);
     WriteToLog (strMsg);
     return ERROR_FILE_OPEN;
  }
  fclose (fpHeader);

/* Open data file */
  if ((fpData = fopen (strImageDataFile, "rb")) == NULL) 
  {
     strMsg[0]='\0';
     sprintf (strMsg, "Cannot open image data file : %s", strImageDataFile);
     WriteToLog (strMsg);
     return ERROR_FILE_OPEN;
  }
  fclose (fpData);

/* Set the path of the CTN execuatbles from the configuration file */
  tmp = (char *)getenv("DICOM_BIN_DIR");
  if (tmp == NULL)
      strcpy (strPath, "");
  else
      strcpy (strPath, tmp);


/* Create argument list for CTN command */ 
  strcpy (strTemporaryFile, TEMPORARY_DICOM_FILE);
  sprintf (strCTNArgument, "-i %s -p %s %s", strHeaderDataFile, strImageDataFile, strTemporaryFile);
  sprintf (strCommand, "%s/dcm_create_object %s", strPath, strCTNArgument);

/* Execute CTN command - dcm_create_object */
  system (strCommand);     

/* Open temporary file - temp.dcm */
  if ((fpTemp = fopen (strTemporaryFile, "rb")) == NULL) 
  {
     strMsg[0]='\0';
     sprintf (strMsg, "Cannot open temporary dicom file : %s", strTemporaryFile);
     WriteToLog (strMsg);
     return ERROR_FILE_OPEN;
  }
  fclose (fpTemp);

/* Create the command */
  strCommand[0] = '\0';
  sprintf (strCommand, "%s/dcm_ctnto10 %s %s", strPath, strTemporaryFile, strOutputFile);

/* Execute CTN command - dcm_ctnto10 */
  system (strCommand);     

  return SUCCESS;
}
