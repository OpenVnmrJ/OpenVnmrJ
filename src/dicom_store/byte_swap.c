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
/* Name        : byte_swap.c                                        */
/* Description : Swap bytes in 16 bit image files                   */
/* Created by  : Mindteck (India) limited                           */
/* Created on  : 03.07.2003                                         */
/* Modified on :                                                    */
/********************************************************************/
# include <stdio.h>
# include <stdlib.h>
# include "constants.h"

/*************************************************************************/
/* Function    : UsageError                                              */
/* Description : The function displays 'Function usage'                  */
/*************************************************************************/
static void UsageError()
{
    char msg[] = "Usage: byte_swap pixel-data swapped-data,\n\tpixel-data, full path of pixel-data file to be converted\n\tswapped-data , full path of swapped file \n\n";
    printf("%s", msg);
    return; 
}

/*************************************************************************/
/* Function    : WriteToLog                                              */
/* Description : The function writes log in  ../log/error.log            */
/*************************************************************************/

int WriteToLog (char* strMsg)
{
  FILE *fpServerLog;
  char *log;
  char strLogFile[LOG_FILE_NAME_LEN];

/* Create/Open the log file in append mode */
/*
  strcpy (strLogFile, CREATE_CTN_INPUT_LOG);
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

  fprintf (fpServerLog, "byte_swap:\t%s\n\n", strMsg);

/* Closes the log file */
  if (fpServerLog != NULL)
  {
      fclose(fpServerLog);
  }
  return SUCCESS;

}


/************************************************************************/
/* Function    : main                                                   */
/* Description : Swap bytes in 16 bit images                            */
/* Parameters  : i)  the total pixel data file - 16 bit                 */
/*             : ii) the name of converted pixel data file - 16 bit     */
/************************************************************************/
int main (int argc, char *argv[])
{
/* Declare variables */
    FILE* fpPixels;
    FILE* fpByteSwapped;
    FILE* fpParameter;

    static char strName[NAME_LEN];
    static char strEqualTo[EQUAL_TO_LEN];
    static char strTemp[VALUE_LEN];
    static char strBuffer[BUFFER_LEN];
    static char strMsg[MSG_LEN];
    char   d1;

    short nValue =0;
    short nValue1=0;
    short nValue2=0;

    short nMax=0;
    short nMin=255;

    int   nRow=0;
    int   nCol=0;
    int   nTotalRow=0;
    int   nTotalCol=0;
    int   nFileCount=1;
    int   nBits=0;
    int   nSlices=1;
    int   nOrientation=1;

    int   i=0;
    int   nCount=0;

/* Check for correct usage of the application */
    if (argc != 3) 
    {
        UsageError();
        return ERROR_USAGE;
    }

/* Open raw pixel data file in read mode */   
    fpPixels = fopen (argv[1], "r+b");
    if (fpPixels == NULL)
    {
       strMsg[0]='\0';
       sprintf (strMsg, "Cannot open pixel data file in read mode: %s", argv[1]);
       WriteToLog (strMsg);
       return ERROR_FILE_OPEN; 
    }

/* Open converted pixel data file in write mode */   
    fpByteSwapped = fopen (argv[2], "w+b");
    if (fpByteSwapped == NULL)
    {
       fclose (fpPixels);
       strMsg[0]='\0';
       sprintf (strMsg, "Cannot open converted data file in write mode: %s", argv[2]);
       WriteToLog (strMsg);
       return ERROR_FILE_OPEN; 
    }

/* Open parameter file : temp.out in read mode */   
/*
    fpParameter = fopen ("temp.out", "r+t");
  fpParameter = fopen ("../../bin/temp.out", "r+t");
*/
    fpParameter = fopen ("temp.out", "r+t");
    if (fpParameter == NULL)
    {
       fclose (fpPixels);
       fclose (fpByteSwapped);
       strMsg[0]='\0';
       sprintf (strMsg, "Cannot open file - temp.out in read mode: %s", argv[2]);
       WriteToLog (strMsg);
       return ERROR_FILE_OPEN; 
    }

/* Iterate through the parameter file */
    while (!feof(fpParameter))
    {
       
/* Read each line of the parameter file */
       fgets (strBuffer, 100, fpParameter);
/* Parse each line */
       i = sscanf (strBuffer, "%s %s %[^;]s",  strName, strEqualTo, strTemp);

       if (strcmp (strName, "Row")==0)
       {
/* Store value of 'Row' */
           nTotalRow = atoi(strTemp);
       }
       if (strcmp (strName, "Col")==0)
       {
/* Store value of 'Col' */
           nTotalCol = atoi(strTemp);
       }
       if (strcmp (strName, "FileCount")==0)
       {
/* Store value of 'File Count' */
           nFileCount = atoi(strTemp);
           if (nFileCount < 1)
              nFileCount = 1;
       }
       if (strcmp (strName, "Bits")==0)
       {
/* Store value of 'Bits Allocated' */
           nBits = atoi(strTemp);
       }
       if (strcmp (strName, "Slices")==0)
       {
/* Store value of 'Slices' */
           nSlices = atoi(strTemp);
           if (nSlices < 1)
              nSlices = 1;
       }
    }
/* Calculate the number of orientation */
    nOrientation = (nFileCount / nSlices);
    if (nOrientation < 1)
	nOrientation = 1;

/**
    printf ("nTotalRow=%d\n", nTotalRow);
    printf ("nTotalCol=%d\n", nTotalCol);
    printf ("nBits=%d\n", nBits);
    printf ("nSlices=%d\n", nSlices);
    printf ("nFileCount=%d\n", nFileCount);
    printf ("nOrientation=%d\n", nOrientation);
**/

    if (nBits > 16) {
       sprintf (strMsg, "Bit size %d is not supported.\n", nBits);
       WriteToLog (strMsg);
       fclose (fpParameter);
       fclose (fpPixels);
       fclose (fpByteSwapped);
       return ERROR_FILE_OPEN; 
    }
    if (nBits != 16)
    {
/* Return for non-16 bit images */
/*
       fclose (fpParameter);
       fclose (fpPixels);
       fclose (fpByteSwapped);
       printf ("\nFile cannot be opened in write mode\n");
       return -1;
*/
    }
    

    if (fseek(fpPixels, 0, SEEK_SET) != 0)
    {
/* Go to beginning of pixel data file */
      fclose (fpPixels);
      fclose (fpByteSwapped);
      sprintf (strMsg, "Cannot reposition file pointer\n");
      WriteToLog (strMsg);
      return -1;
    }

    if (nBits == 16) {
        for (i=0; i<nOrientation; ++i )
    	{
      	   for (nCount=1; nCount<=nSlices; ++nCount)
           {
              for (nRow = 0; nRow < nTotalRow ; nRow++)
              {
                  for (nCol = 0; nCol < nTotalCol ; nCol++)
          	  {
                    	/* Read 2 bytes */
            		fread(&nValue, 2, 1, fpPixels);

            		/* Swap the 2 bytes */
            		nValue1 = nValue2 = 0;
            		nValue1 = nValue >> 8 ;
            		nValue2 = nValue << 8 ;
            		nValue2 |= nValue1 ;

            		/* Write the swapped data in converted file */
            		fwrite(&nValue2, 2, 1, fpByteSwapped);
            	  }
              }
            }
        }
        fclose (fpByteSwapped);
    }
    else {
        for (i=0; i<nOrientation; ++i )
    	{
      	   for (nCount=1; nCount<=nSlices; ++nCount)
           {
              for (nRow = 0; nRow < nTotalRow ; nRow++)
              {
                  for (nCol = 0; nCol < nTotalCol ; nCol++)
          	  {
                    	/* Read 1 bytes */
            		fread(&d1, 1, 1, fpPixels);
            		fwrite(&d1, 1, 1, fpByteSwapped);
            	  }
              }
            }
        }
        fclose (fpByteSwapped);
    }
/* Close files */
    fclose (fpPixels);
    fclose (fpParameter);

    return SUCCESS;
}
