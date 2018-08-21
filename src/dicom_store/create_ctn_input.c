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
/* Name        : create_ctn_input.c                                 */
/* Description : A macro dicomhdr(), written by Varian, converts    */
/*               the procpar file to a text file, which contains    */
/*               DICOM tags, VR, and values. However the format is  */
/*               not compatible with the one expected by CTN. Hence */
/*               this program accepts the output of the macro and   */
/*               converts it to CTN compatible input file           */
/* Created by  : Mindteck (India) limited                           */
/* Created on  : 01.05.2003                                         */
/* Modified on : 02.07.2003 - Sohini - 001                          */ 
/*------------------------------------------------------------------*/
/* Modification Log :                                               */
/* 001 - Facilitate byte swapping for correct image in DICOM Viewer */
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
    char msg[] = "Usage: create_ctn_input macro-output header-input ctn-input,\n\tmacro-output, full path of macro_output file to be converted\n\theader-input , full path of header file \n\tctn-input, full path of CTN compatible header file \n\n";
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
  log = (char *)getenv("LOGFILE");
  if (log == NULL || (int) strlen(log) <= 0)
     return ERROR_FILE_OPEN;
  strcpy (strLogFile, log);
  if ((fpServerLog = fopen (strLogFile, "a+")) == NULL)
  {
     printf ("Cannot open the log file : %s\n\n", strLogFile);
     return ERROR_FILE_OPEN;
  }

  fprintf (fpServerLog, "create_ctn_input:\t%s\n\n", strMsg);

/* Closes the log file */
  if (fpServerLog != NULL)
  {
      fclose(fpServerLog);
  }
  return SUCCESS;

}


/************************************************************************/
/* Function    : main                                                   */
/* Description : The function converts the output of the macro dicomhdr */
/*               to CTN compatible input file                           */
/* Parameters  : i)  the output of macro dicomhdr                       */
/*             : ii) the header file                                    */
/*               iii)the input of CTN program dcm_create_object         */
/*                                                      with option -i  */
/************************************************************************/
int main (int argc, char *argv[])
{
/* Declare variables */
    FILE *fpMacroOutput;
    FILE *fpHeader;
    FILE *fpCTNInput;
    FILE *fpTemp;

    int nRow = 0;
    int nCol = 0;
    int i = 0;
    int nSlices = 1;
    int nBits = 8;

    static char strMsg[MSG_LEN];
    static char strBuffer[BUFFER_LEN];
    static char strBufferLast[BUFFER_LEN];
    static char strGroup[GROUP_LEN];
    static char strElement[ELEMENT_LEN];
    static char strVR[VR_LEN];
    static char strTemp[VALUE_LEN];
    static char strValue[VALUE_LEN];
    static char strRow[VALUE_LEN];
    static char strColumn[VALUE_LEN];
    static char strDataType[DATA_TYPE_LEN];
    static char strName[NAME_LEN];
    static char strEqualTo[EQUAL_TO_LEN];

/* Initialize variables */
    strMsg[0]       ='\0'; /* Error message to be written in log */
    strBuffer[0]    ='\0'; /* Each line in macro output file */
    strBufferLast[0]='\0'; /* The same */
    strGroup[0]     ='\0'; /* The Group number of the tag */
    strElement[0]   ='\0'; /* The element number of the tag */
    strVR[0]        ='\0'; /* The Value Representation of t he Tag */
    strTemp[0]      ='\0'; /* The temporary variable to store tag value */
    strValue[0]     ='\0'; /* Tag value */

/* Check for correct usage of the application */
    if (argc != 4) 
    {
        UsageError();
        return ERROR_USAGE;
    }

/* Open the macro_output file for reading */
    fpMacroOutput = fopen (argv[1], "r");
    if (fpMacroOutput == NULL)
    {
        strMsg[0]='\0';
        sprintf (strMsg, "Cannot open macro output file : %s", argv[1]);
        WriteToLog (strMsg);
        return ERROR_FILE_OPEN; 
    }

/* Open the header file for reading */
    fpHeader = fopen (argv[2], "r");
    if (fpHeader == NULL)
    {
        strMsg[0]='\0';
        sprintf (strMsg, "Cannot open header file : %s", argv[2]);
        WriteToLog (strMsg);
        fclose (fpMacroOutput); 
        return ERROR_FILE_OPEN; 
    }

/* Create / Open ctn input file for writing data from macro_output */    
    fpCTNInput = fopen (argv[3], "w+");
    if (fpCTNInput == NULL)
    {
        strMsg[0]='\0';
        sprintf (strMsg, "Cannot open CTN Input file : %s", CTN_INPUT_FILE);
        WriteToLog (strMsg);
        fclose (fpMacroOutput); 
        fclose (fpHeader); 
        return ERROR_FILE_OPEN; 
    }


/*--------------------------------------------------------------------*/
/* START 001 -  Modified on 02.07.2003 - Sohini                       */
/* Facilitate byte swapping for correct image in Windows DICOM Viewer */
/*--------------------------------------------------------------------*/

/* Create / Open temporary output file for writing the following data:
   rows, columns, bits allocated, number of frames from macro_output */
    fpTemp = fopen ("temp.out", "w");
    if (fpTemp == NULL)
    {
        strMsg[0]='\0';
        sprintf (strMsg, "Cannot open temporary output file : temp.out");
        WriteToLog (strMsg);
        fclose (fpMacroOutput);
        fclose (fpHeader);
        fclose (fpCTNInput);
/*
        return ERROR_FILE_OPEN;
*/
    }

/*--------------------------------------------------------------------*/
/* END 001                                                            */
/*--------------------------------------------------------------------*/



/* All the 3 files could be successfully opened/created */
   while (!feof(fpMacroOutput))
   {
       strBuffer[0] ='\0';
       strGroup[0]  ='\0';
       strElement[0]='\0';
       strTemp[0]   ='\0';
       strValue[0]  ='\0';
       strVR[0]     ='\0';


/* Read each line of the macro output file */
       if (fgets (strBuffer, 250, fpMacroOutput) == NULL)
       {
           break;
       }

/* Ignore comment lines or blank lines */
       if ((strBuffer[0]=='#') || 
           (strBuffer[0]==' '))
       {
           continue;
       }

/* Parse each line read and store group, element, value of each tag in 
   temporary variables, to be later written in ctn input file */ 
       sscanf (strBuffer, "(%4s,%4s) %2s %[^#]s", strGroup, strElement, strVR, strTemp); 

/* Some of the tags are discarded, as they are not MR compatible storage tags */
       if ((strcmp (strGroup,  "0000") == 0) || 
           (strcmp (strElement,"0000") == 0) ||
           (strcmp (strElement,"0001") == 0) ||
           (strcmp (strGroup,  "2010") == 0) ||
           (strcmp (strGroup,  "2001") == 0) ||
           ((strcmp(strGroup,  "0019") == 0) &&
            (strcmp(strElement,"00fe") == 0))|| 
           ((strcmp(strGroup,  "0021") == 0) &&
            (strcmp(strElement,"00fd") == 0))||
           ((strcmp(strGroup,  "7fe0") == 0) &&
            (strcmp(strElement,"0010") == 0))|| 
           (strcmp (strVR,     "xs")   == 0) )
       {
           continue;
       }
/* skip BitsAllocated, BitsStored, HighBit, they will be set later */
       if (strcmp (strGroup, "0028") == 0) {
           if (strcmp (strElement,"0100") == 0 ||
               strcmp (strElement,"0101") == 0 ||
               strcmp (strElement,"0102") == 0 )
           continue;
       }
       if (strTemp[0]=='[')
       {
/* Read values within square brackets */
           sscanf (strTemp+1, "%[^]]s", strValue);
       }
       else if (strTemp[0] == '(')
       {
/* Read values within first brackets */
           sscanf (strTemp+1, "%[^)]s", strValue);
           strcpy (strValue, "0");
       }
       else
       {
/* Read values which are not within brackets */
           sscanf (strTemp, "%s", strValue);
       }

       if (strcmp (strBufferLast, strBuffer) == 0)
       {
           break;
       }
       strcpy (strBufferLast, strBuffer);

       if (strValue[0] != '\0')
       {
/* CTN value of MR SOP Class UID */
          if (strcmp (strValue, "=MRImageStorage") == 0)
          {
              strcpy (strValue, "1.2.840.10008.5.1.4.1.1.4");     
          }
/* CTN value of UID for Little Endian Implicit */
          else if (strcmp (strValue, "=LittleEndianImplicit") == 0)
          {
              strcpy (strValue, "1.2.840.10008.1.2");
          }
/* CTN value of UID for Little Endian Explicit */
          else if (strcmp (strValue, "=LittleEndianExplicit") == 0)
          {
              strcpy (strValue, "1.2.840.10008.1.2.1.");
          }

/* Write in CTN input file */
         if ( (strcmp(strVR,  "PN") == 0))
         {
/* Concatenate Person Name with ^ */
            for (i=0; i<strlen(strValue); ++i)
            {
                 if (strValue[i]==' ')
                 {
                    strValue[i]='^';
                 }
            }
            fprintf (fpCTNInput, "%4s %4s %s\n", strGroup, strElement, strValue);
         }
         else if (
/* Concatenate some of the strings used in database with . */
           ((strcmp(strGroup,  "0010") == 0) &&
            (strcmp(strElement,"0020") == 0))||
           ((strcmp(strGroup,  "0010") == 0) &&
            (strcmp(strElement,"0030") == 0))||
           ((strcmp(strGroup,  "0010") == 0) &&
            (strcmp(strElement,"0040") == 0))||
           ((strcmp(strGroup,  "0008") == 0) &&
            (strcmp(strElement,"0060") == 0))||
           ((strcmp(strGroup,  "0008") == 0) &&
            (strcmp(strElement,"1030") == 0))||
           ((strcmp(strGroup,  "0018") == 0) &&
            (strcmp(strElement,"1030") == 0))
            ) 
          { 
                for (i=0; i<strlen(strValue); ++i)
                {
                    if (strValue[i]==' ')
                    {
                      strValue[i]='.';
                    }
                }
                fprintf (fpCTNInput, "%4s %4s %s\n", strGroup, strElement, strValue);
          } 

          else if ( (strcmp (strVR, "CS") == 0) ||
                    (strcmp (strVR, "LO") == 0) )
          {
/* Place CS and LO strings in between double quotes */
          	fprintf (fpCTNInput, "%4s %4s \"%s\"\n", strGroup, strElement, strValue);
          }
          else
          {
          	fprintf (fpCTNInput, "%4s %4s %s\n", strGroup, strElement, strValue);
          }
       }
   }

/* Read one of headerxxxx.dat, where xxxx ranges from 0000 to no. of slices */
   while (fgets (strBuffer, 250, fpHeader) != NULL)
   { 
       strName[0]     ='\0';
       strDataType[0] ='\0';
       strEqualTo[0]  ='\0';

       if (strBuffer[0] == '#')
       {
/* Ignore comments */
           continue;
       }
       if (strBuffer[0] == '\0')
       {
/* Ignore blank lines */
           continue;
       }
        
       sscanf (strBuffer, "%s %s %s %[^;]s", strDataType, strName, strEqualTo, strTemp);

       if (strcmp (strName, "matrix[]")==0)
       {
/* Get the DICOM Row and Col values from tag matrix */
           sscanf  (strTemp, "{%d, %d}" , &nCol, &nRow);
/*
           fprintf (fpCTNInput, "0028 0010 %d\n", nRow);
           fprintf (fpCTNInput, "0028 0011 %d\n", nCol);
*/
       }
/*
       else if (strcmp (strName, "ro_size")==0)
       {
           fprintf (fpCTNInput, "0028 0010 %s\n", strTemp);
           nRow = atoi(strTemp);
       }
       else if (strcmp (strName, "pe_size")==0)
       {
           fprintf (fpCTNInput, "0028 0011 %s\n", strTemp);
           nCol = atoi(strTemp);
       }
       else if (strcmp (strName, "slices")==0)
*/
       else if (strcmp (strName, "slice_no")==0)
       {
/* Get the DICOM 'Number of slices' value from tag slices */
/*
           fprintf (fpCTNInput, "0028 0008 %s\n", strTemp);
*/
           nSlices = atoi(strTemp);
       }

/*--------------------------------------------------------------------*/
/* START 001 -  Modified on 02.07.2003 - Sohini                       */
/* Facilitate byte swapping for correct image in Windows DICOM Viewer */
/*--------------------------------------------------------------------*/
/* Get the DICOM 'Bits Allocated' value from tag - bits */

       else if (strcmp (strName, "bits")==0)
       {
           nBits = atoi(strTemp);
/*
           if (nBits <= 16)
              fprintf (fpCTNInput, "0028 0100 %s\n", strTemp);
*/
       }

/*--------------------------------------------------------------------*/
/* END 001                                                            */
/*--------------------------------------------------------------------*/
   }
   if (nBits > 16)
        nBits = 16;
   fprintf (fpCTNInput, "0028 0100 %d\n", nBits); /* BitsAllocated */
   fprintf (fpCTNInput, "0028 0101 %d\n", nBits); /* BitsStored */
   fprintf (fpCTNInput, "0028 0102 %d\n", nBits - 1); /* HighBit */

/* Close the macro output file */
   fclose (fpMacroOutput); 
/* Close the header file */
   fclose (fpHeader); 
/* Close the CTN input file */
   fclose (fpCTNInput); 


/*--------------------------------------------------------------------*/
/* START 001 -  Modified on 02.07.2003 - Sohini                       */
/* Facilitate byte swapping for correct image in Windows DICOM Viewer */
/*--------------------------------------------------------------------*/
   if (fpTemp != NULL) {
/*
      fprintf (fpTemp, "Row = %d\n", nRow);
      fprintf (fpTemp, "Col = %d\n", nCol);
*/
      fprintf (fpTemp, "Bits = %d\n", nBits);
/*
      fprintf (fpTemp, "Slices = %d\n", nSlices);
*/

      fclose (fpTemp); 
   }
/*--------------------------------------------------------------------*/
/* END 001                                                            */
/*--------------------------------------------------------------------*/

   return SUCCESS;
}
