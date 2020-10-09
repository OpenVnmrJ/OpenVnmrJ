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
/* Name        : split_fdf.c                                        */
/* Description : Splits the Flexible Data format file into data and */
/*                                                   header portion */
/* Created by  : Mindteck (India) limited                           */
/* Created on  : 01.05.2003                                         */
/* Modified on :                                                    */
/********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "constants.h"

static int nBits = 8;
static int dataBits;
static char strMsg[MSG_LEN];
static char inputs[512];

/*************************************************************************/
/* Function    : UsageError                                              */
/* Description : The function displays 'Function usage'                  */
/*************************************************************************/
static void UsageError(void)
{
    char msg[] = "Usage:  split_fdf inputfile headerfile datafile\n\
                  inputfile, the complete path of FDF file to be split\n\
                  headerfile, the complete path of header file\n\
                  datafile, the complete path of datafile\n";
    printf("%s", msg);
    return; 
}

/*************************************************************************/
/* Function    : WriteToLog                                              */
/* Description : The function writes log in file ../log/error.log        */
/*************************************************************************/

int WriteToLog (char* msg)
{
  FILE *fpServerLog;
  char *log;
  char strLogFile[LOG_FILE_NAME_LEN];

/* Create/Open the log file in append mode */
  log = (char *)getenv("LOGFILE");
  if (log == NULL || (int) strlen(log) <= 0)
     return ERROR_FILE_OPEN;
  strcpy (strLogFile, log);
  
  if ((fpServerLog = fopen (strLogFile, "a+")) == NULL)
  {
     printf ("Cannot open the log file : %s\n\n", strLogFile);
     return ERROR_FILE_OPEN;
  }

  fprintf (fpServerLog, "split_fdf:\t%s\n\n", msg);

/* Closes the log file */
  if (fpServerLog != NULL)
  {
      fclose(fpServerLog);
  }
  return SUCCESS;

}

void readConf()
{
    FILE *fpConf;
    char strConfFile[PATH_LEN];
    char strTag[TAG_LEN];
    char strValue[VALUE_LEN];
    char *tmp;

    tmp = (char *)getenv("DICOM_CONF_DIR");
    if (tmp != NULL)
        sprintf (strConfFile, "%s/%s", tmp, STORE_IMAGE_CONF);
    else
        sprintf (strConfFile, "%s", STORE_IMAGE_CONF);
    if ((fpConf = fopen (strConfFile, "r")) == NULL)
	return;

    while (fgets(inputs, 512, fpConf) != NULL) {
        strValue[0] = '\0';
        if (sscanf (inputs, "%s : %s", strTag, strValue) == 2) 
        {
            if (strcmp (strTag, "BITS") == 0)
            {
                if ((int) strlen(strValue) > 0)
                   nBits = atoi(strValue);
            }
	}
     }
     fclose(fpConf);
     if (nBits != 8 && nBits != 16)
        nBits = 8;
}

void convertInt32(FILE *fin, FILE *fout)
{
    int   count;
    int   iMax;
    int   iMin;
    int   iDiv;
    int   iData;
    int   k;
    short sData;
    char  cData;
    long  xPos;

    xPos = ftell(fin);
    iMax = 0;
    iMin = 99999;
    while (!feof(fin)) {
        fread( &iData, 4, 1, fin );
        if (iData > iMax)
            iMax = iData;
        if (iData < iMin)
            iMin = iData;
    }
    if (nBits == 8)
       iDiv = (iMax - iMin) / 250;
    else
       iDiv = (iMax - iMin) / 4096;
    fseek(fin, xPos, SEEK_SET); 
    count = 0;
    if (iMin < 0)
        iMin = 0 - iMin;
    else
        iMin = 0;
    while (!feof(fin)) {
        fread( &iData, 4, 1, fin );
        k = (iData + iMin) / iDiv;
        if (nBits == 8) {
           if (k > 245)
              cData = 255;
           else
              cData = k;
           fwrite( &cData, 1, 1, fout );
           count++;
        }
        else {
           if (k > 4080)
              sData = 4096;
           else
              sData = k;
           fwrite( &cData, 1, 1, fout );
           count++;
        }
    }
}

void convertInt8(FILE *fin, FILE *fout)
{
    int   count;
    int   iMax;
    int   iMin;
    int   iDiv;
    int   k;
    short sData;
    char  cData;
    long  xPos;

    xPos = ftell(fin);
    iMax = 0;
    iMin = 9999;
    while (!feof(fin)) {
        fread( &cData, 1, 1, fin );
        if (cData > iMax)
            iMax = cData;
        if (cData < iMin)
            iMin = cData;
    }
    iDiv = (iMax - iMin) / 4096;
    fseek(fin, xPos, SEEK_SET); 
    count = 0;
    if (iMin < 0)
        iMin = 0 - iMin;
    else
        iMin = 0;
    while (!feof(fin)) {
        fread( &cData, 1, 1, fin );
        k = cData + iMin;
        sData =  k / iDiv;
        if (sData > 4080)
           cData = 4096;
        fwrite( &sData, 2, 1, fout );
        count++;
    }
}


void convertInt16(FILE *fin, FILE *fout)
{
    int   count;
    int   iMax;
    int   iMin;
    int   iDiv;
    int   k;
    short sData;
    char  cData;
    long  xPos;

    xPos = ftell(fin);
    iMax = 0;
    iMin = 9999;
    while (!feof(fin)) {
        fread( &sData, 2, 1, fin );
        if (sData > iMax)
            iMax = sData;
        if (sData < iMin)
            iMin = sData;
    }
    iDiv = (iMax - iMin) / 250;
    fseek(fin, xPos, SEEK_SET); 
    count = 0;
    if (iMin < 0)
        iMin = 0 - iMin;
    else
        iMin = 0;
    while (!feof(fin)) {
        fread( &sData, 2, 1, fin );
        k = sData + iMin;
        cData =  k / iDiv;
        if (cData > 245)
           cData = 255;
        fwrite( &cData, 1, 1, fout );
        count++;
    }
}


void convertFloat(FILE *fin, FILE *fout)
{
    float fData;
    float fV;
    float fMax;
    float fMin;
    float fDiv;
    short sData;
    int   len;
    int   count;
    long  xPos;
    char  cData;

    fMax = -10.0;
    fMin = 1.0;
    len = sizeof(float);
    xPos = ftell(fin);
    while (!feof(fin)) {
        fread( &fData, len, 1, fin );
        if (fData > fMax)
            fMax = fData;
        if (fData < fMin)
            fMin = fData;
    }
    if (nBits == 8)
        fDiv = (fMax - fMin) / 250;
    else
        fDiv = (fMax - fMin) / 4096;
    if (fMin < 0)
        fMin = 0 - fMin;
    else
        fMin = 0;
    fseek(fin, xPos+len, SEEK_SET); 
    count = 0;
    if (nBits == 8) {
        while (!feof(fin)) {
           fread( &fData, len, 1, fin );
           fV = ((fData + fMin) / fDiv) * 1.2;
    	   if (fV > 250)
     		fV = 255;
           cData = (int) fV;
           fwrite( &cData, 1, 1, fout );
           count++;
        }
    }
    else {
        while (!feof(fin)) {
           fread( &fData, len, 1, fin );
           fV = ((fData + fMin) / fDiv) * 1.2;
    	   if (fV > 4080)
     		fV = 4096;
           sData = (int) fV;
           fwrite( &sData, 2, 1, fout);
           count++;
        }
    }
}
 

/********************************************************************/
/* Function    : main                                               */
/* Description : Splits the Flexible Data Format file into data and */
/*               header portion. The name of the FDF file, which is */
/*               to be split, the header file and the data file in  */
/*               which the FDF files are split come as parameters   */
/*               The FDF file consists of header information &      */
/*               binary data. They are separated by a NULL character*/
/* Parameters  : Command line argument : i)  the fdf file           */
/*                                       ii) the header file        */
/*                                       iii)the data file          */ 
/********************************************************************/

int main (int argc, char *argv[])
{
/* Declare variables */
    FILE *fpFDF;
    FILE *fpFDFHeader;
    FILE *fpFDFData;
    char chChar;
    char d2[12];
    char *data, *d1;
    int  nChar;
    int  isFloat;

/* Initialize variables */
   chChar        =' ';
   strMsg[0]     ='\0';

/* Check for correct usage of the application */
  if( argc != 4) 
  {
      UsageError();
      return ERROR_USAGE;
  }

  readConf();

/* Open the Varian formatted 'Flexible Data Format' file */
    if( (fpFDF = fopen (argv[1], "r")) == NULL )
    {
         strMsg[0]='\0';
         sprintf (strMsg, "Cannot open flexible data format file : %s", argv[1]);
         WriteToLog (strMsg);
         return ERROR_FILE_OPEN;
    }

/* Create a new file in write mode for writing data portion of the FDF file */
    if( (fpFDFData = fopen (argv[3], "wb+")) == NULL )
    {
         fclose (fpFDF);
         strMsg[0]='\0';
         sprintf (strMsg, "Cannot create/open datafile : %s", argv[3]);
         WriteToLog (strMsg);
         return ERROR_FILE_OPEN;
    }

/* Create a new file in write mode for writing header portion of the FDF file */
    if( (fpFDFHeader = fopen (argv[2], "w+")) == NULL)
    {
         fclose (fpFDF);
         fclose (fpFDFData);
         strMsg[0]='\0';
         sprintf (strMsg, "Cannot create/open headerfile : %s", argv[2]);
         WriteToLog (strMsg);
         return ERROR_FILE_OPEN;
    }

/* check the data format and data bits */
    isFloat = -1;
    dataBits = 0;
    while ((data = fgets(inputs, 510, fpFDF)) != NULL)
    {
	if (isFloat < 0) {
           d1 = strstr(data, " *storage");
           if (d1 != NULL) {
              d1 = strstr(data, "\"float\"");
              if (d1 != NULL)
                  isFloat = 1;
              else {
                  d1 = strstr(data, "\"integer\"");
                  if (d1 != NULL)
                     isFloat = 0;
              }
           }
	}
	if (dataBits <= 0) {
           d1 = strstr(data, " bits");
           if (d1 != NULL) {
              while (*d1 != '\0') {
                 if (*d1 == '=') {
                    d1++;
                    break; 
                 }
                 d1++;
              }
              while (*d1 != '\0') {
                 if (*d1 == ' ' || *d1 == '\t')
                      d1++;
                 else
                    break; 
              }
 	      nChar = 0;
              while (*d1 != '\0') {
                  if (*d1 == ' ' || *d1 == ';')
                      break;
                  d2[nChar++] = *d1;
                  d1++;
              }
              d2[nChar] = '\0';
              if (nChar > 0) {
                  dataBits = atoi(d2);
              }
           }
	}
	if (isFloat >= 0 && dataBits != 0)
            break;
    }
    fseek(fpFDF, 0, SEEK_SET); 

/* Read character by character from the FDF file and write in the header file 
   till the first null character is encountered */ 

    nChar = fgetc (fpFDF); 
    while (!feof(fpFDF))
    {
       chChar = (char) nChar; 
       if( chChar == '\0' ) 
       {
           break;
       }
       fputc (chChar, fpFDFHeader);
       nChar = fgetc (fpFDF);
    }
    if (isFloat > 0 || dataBits != nBits) {
       fprintf (fpFDFHeader, "float  bits = %d;\n", nBits);
       fprintf (fpFDFHeader, "char  *storage = \"integer\";\n");
    }
    fprintf (fpFDFHeader, "\0");

/* Close the header file */
    fclose (fpFDFHeader);

/* Read character by character from the FDF file and write in the data file 
   after the first null character is encountered */ 

    if (isFloat > 0) {
	convertFloat(fpFDF, fpFDFData);
    }
    else {
        if (nBits == dataBits) {
            fread (&chChar, sizeof( char ), 1, fpFDF);
            while (!feof(fpFDF))
            {
               fwrite( &chChar, sizeof( char ), 1, fpFDFData );
               fread( &chChar, sizeof( char ), 1, fpFDF );
            }
        }
        else {
            if (dataBits == 16)
	       convertInt16(fpFDF, fpFDFData);
            if (dataBits == 8)
	       convertInt8(fpFDF, fpFDFData);
            if (dataBits == 32)
	       convertInt32(fpFDF, fpFDFData);
        }
    }

/* Close the data file */
    fclose (fpFDFData);

/* Close the Flexible Data Format file */
    fclose (fpFDF);

    return SUCCESS;
}

