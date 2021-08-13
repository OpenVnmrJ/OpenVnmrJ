/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************/
/****      PROGRAM: header_change.c                     ****/
/****      For updating the header of FID (ct)          ****/
/****      Last update: Feb 13, 2007                    ****/ 
/***********************************************************/
 
/**[1] INCLUDE FILES **************************************/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

/**[2] GENEREL DECLERATIONS *******************************/

struct { 
/* Used at start of each data file (FIDs, spectra, 2D) */
	int  nblocks;     /* number of blocks in file */
	int  ntraces;     /* number of traces per block */
	int  np;          /* number of elements per trace */
	int  ebytes;      /* number of bytes per element */
	int  tbytes;      /* number of bytes per trace */
	int  bbytes;      /* number of bytes per block */
	short vers_id;     /* software version, file_id status bits */
	short status;      /* status of whole file */
	int  nbheaders;   /* number of block headers per block */
       } datafilehead;

struct {
/* Each file block contains the following header */
	short scale;       /* scaling factor */
	short status;      /* status of data in block */
	short index;       /* block index */
	short mode;        /* mode of data in block */ 
	int  ctcount;     /* ct value for FID */
	float lpval;       /* f2 (2D-f1) left phase in phasefile */
	float rpval;       /* f2 (2D-f1) right phase in phasefile */
	float lvl;         /* left drift correction */
	float tlt;         /* tilt drift correction */
       } datablockhead;

// From data.h

#define S_VAR           0x0     /* 1 = Varian data               */ 
#define S_QONE          0x800   /* 1 = Q-One data               */ 
#define S_MAKEFID       0x1000  /* 1 = data from makefid        */
#define S_JEOL          0x2000  /* 1 = JEOL data                */ 
#define S_BRU           0x4000  /* 1 = Bruker data              */ 
#define S_MAG           0x1800  /* 1 = Magritek data              */ 
#define P_VENDOR_ID     0x7800  /* preserves vendor ID status   */
#define S_COMPLEX       0x10    /* 0 = real         1 = complex    */
#define VERSION 1

/*** PROGRAM BEGIN *************************************/
int main(int argc, char *argv[])
{
/*** VARIABLE DECLARATION **************************************/
     FILE *in_file;
     FILE *out_file = NULL;
     int i,j,no_points;
     short data_short;
     int  data_int ;
     int bit[16],n,file_status,fs;
     double x;
     int nblks;
     int ctval;
     int no_bytes;
     int versID = 0;

/*** CONTROL OF INPUT PARAMETERS ******************************/
     if (argc<2)
         {
         printf("\nName of input file was not passed!\n");
         exit (3);
         }
         
      if (argc<3)
         {
         printf("\nName of output file was not passed!\n");
         exit (3);
         }
         
      if (argc<4)
         {
         printf("\nNew CT was not passed!\n");
         exit (3);
         }

      versID = ( ! strcmp("versid",argv[2]) );
         
/*** OPENING THE FILES AND READING THE HEADER ****************/

      if ((in_file=fopen(argv[1],"r+"))==NULL)
         {
         printf("\nCan not open input file");
         exit (3);
         }
      rewind(in_file);  
        
     if ( ! versID)
     {
      if ((out_file=fopen(argv[2],"wb"))==NULL)
         {
         printf("\nCan not open output file");
         exit (3);
         }
         
      rewind(out_file);
     }
      
         
/*** WRITE A HEADER OF AN OUTPUT FILE *****************/
     if (fread(&datafilehead,sizeof(datafilehead),1,in_file) != 1)
         {
         fprintf (stderr, "Error in reading input data (datafilehead)\n");
         exit (3);
         }
         
      if ( versID)
      {
         short vers_id;
         short status;
         vers_id = ntohs(datafilehead.vers_id);
         status = ntohs(datafilehead.status);
         if ( ! (status & S_COMPLEX) )
         {
            status  &= ~(0x40);
            status |= S_COMPLEX;
            datafilehead.status = htons(status);
         }
         vers_id &= ~P_VENDOR_ID;
         if (argv[3][0] == 'V')
            vers_id |= S_VAR;
         if (argv[3][0] == 'B')
            vers_id |= S_BRU;
         if (argv[3][0] == 'J')
            vers_id |= S_JEOL;
         if (argv[3][0] == 'Q')
            vers_id |= S_QONE;
         if (argv[3][0] == 'C')
            vers_id |= S_MAKEFID;
         if (argv[3][0] == 'M')
            vers_id |= S_MAG;
         vers_id |= VERSION;
        
         datafilehead.vers_id = htons(vers_id);
         rewind(in_file);  
         if (fwrite(&datafilehead,sizeof(datafilehead),1,in_file) != 1)
         {
            fprintf (stderr, "Error in writting output data (datafilehead)\n");
            exit (3);
         }
         if (fclose(in_file) != 0) 
         {
            printf ("Error closing input file"); 
            exit (3);      
         }
         exit(EXIT_SUCCESS);
      }
     if (fwrite(&datafilehead,sizeof(datafilehead),1,out_file) != 1)
        {
        fprintf (stderr, "Error in writting output data (datafilehead)\n");
        exit (3);
        }
        
/*** READ PARAMETERS **********************************/
     no_points = ntohl(datafilehead.np);
     
     
/*** FILE STATUS  ********************************/
     file_status = ntohs(datafilehead.status);
     fs = file_status;

     for (n=15;n>=0;n--)
         {
         x = pow(2.0,(double)n);
         bit[n] = 0;
         if (fs >= (int)x)
            {
            fs += -(int)x;
            bit[n] = 1;
            }
         }
     
     fprintf (stderr, "\n******* FID HEADER UPDATE *************\n");
     fprintf (stderr, "FILE STATUS = %d\n",file_status);
          
     if (bit[3] == 0)
        {
        fprintf (stderr, "Type of data: INTEGER\n");
        }
     if (bit[3] == 1)
        {
        fprintf (stderr, "Type of data: FLOATING POINT\n");
        }
        
     fprintf (stderr, "np = %d\n",no_points);


/*** WRITING THE DATA ********************************/

     nblks = ntohl(datafilehead.nblocks);
     no_bytes = ntohl(datafilehead.ebytes);
     ctval = atoi(argv[3]);
     for (i=0;i<nblks;++i)
         {
         if (fread(&datablockhead,sizeof(datablockhead),1,in_file) != 1)
            {
            fprintf (stderr, "Error in reading input data (datablockhead)\n");
            exit (3);
            }
         
         datablockhead.ctcount = htonl(ctval);
         fprintf (stderr, "block[%3d]  new CT = %3d\n",i+1,ctval);
             
         if (fwrite(&datablockhead,sizeof(datablockhead),1,out_file) != 1)
            {
            fprintf (stderr, "Error in writting output data (datablockhead)\n");
            exit (3);
            }
                     
         for (j=0;j<no_points;++j)
             {
             if (no_bytes == 2)
                {
                if (fread(&data_short,sizeof(data_short),1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (integer)\n");
                   exit (3);
                   }
                
                if (fwrite(&data_short,sizeof(data_short),1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data (integer) \n");
                   exit (3);
                   }
                }
             else
                {
                if (fread(&data_int,sizeof(data_int),1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (float)\n");
                   exit (3);
                   }

                if (fwrite(&data_int,sizeof(data_int),1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data\n");
                   exit (3);
                   }
                }
             }
         }    

 
/*** CLOSING FILES *************************************/
  
      if (fclose(in_file) != 0) 
         {
         printf ("Error closing input file"); 
         exit (3);      
         }

      if (fclose(out_file) != 0) 
         {
         printf ("Error closing output file"); 
         exit (3);      
         }     
    exit(EXIT_SUCCESS); 
}
