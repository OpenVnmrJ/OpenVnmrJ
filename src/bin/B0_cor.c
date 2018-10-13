/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/***********************************************************/ 
/****      PROGRAM NAME: B0_cor.c                       ****/
/****      FREQUENCY SHIFT CORRECTION ON FID            ****/
/****      Entirely automated                           ****/
/****      Compatible with integers and floating points ****/
/****      Last update: Feb 13, 2007                    ****/ 
/***********************************************************/
 
/**[1] INCLUDE FILES **************************************/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>


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


/*** PROGRAM BEGIN *************************************/
int main(int argc, char *argv[])
{
/*** VARIABLE DECLARATION ******************************/
     FILE *in_file,*out_file,*freq_file;
     double real,imag,phi,abs_data;
     float sw,delta_f;
     int data_int;
     short data_short;
     int i,j,lsfid,no_bytes;
     int bit[16],n,file_status,fs;
     double x;
     int nblks, npts;
     union u_tag {
      float fval;
      int   ival;
     } uval;


/*** CONTROL OF INPUT PARAMETERS ***********************/
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
         printf("\nName of frequency file was not passed!\n");
         exit (3);
         }

     if ((in_file=fopen(argv[1],"rb"))==NULL)
        {
        printf("\nCan not open input file");
        exit (3);
        }

     rewind(in_file);  


     if (fread(&datafilehead,sizeof(datafilehead),1,in_file) != 1)
         {
         fprintf (stderr, "Error in reading input data (datafilehead)\n");
         exit (3);
         }  


     if ((out_file=fopen(argv[2],"wb"))==NULL)
        {
        printf("\nCan not open output file");
        exit (3);
        }

     rewind(out_file);   

     if (fwrite(&datafilehead,sizeof(datafilehead),1,out_file) != 1)
         {
         fprintf (stderr, "Error in writting output data (datafilehead)\n");
         exit (3);
         }

     if ((freq_file=fopen(argv[3],"rb"))==NULL)
        {
        printf("\nCan not open frequency file");
        exit (3);
        }

     fscanf(freq_file,"%f",&sw);
     fscanf(freq_file,"%d",&lsfid);

     no_bytes = ntohl(datafilehead.ebytes);
     

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

     fprintf (stderr, "\n******* B0 DRIFT CORRECTION ********\n");
     fprintf (stderr, "FILE STATUS = %d\n",file_status);
          
     if (bit[3] == 0)
        {
        fprintf (stderr, "bit[3] = %d   Type of data: INTEGER\n",bit[3]);
        }
     if (bit[3] == 1)
        {
        fprintf (stderr, "bit[3] = %d   Type of data: FLOATING POINT\n",bit[3]);
        }


/*** FREQUENCY SHIFT CORRECTION  *******************/

     fprintf (stderr, "sw = %8.1f Hz   lsfid = %d\n",sw,lsfid);
     fprintf (stderr,"\n Spectrum\t Frequency\n");

     nblks = htonl(datafilehead.nblocks);
     npts = htonl(datafilehead.np);
     for (i=0;i<nblks;++i)
         {

         fscanf(freq_file,"%f",&delta_f);
         fprintf(stderr," spect[%3d]\t%+7.3f Hz\n",i+1,delta_f);

         if (fread(&datablockhead,sizeof(datablockhead),1,in_file) != 1)
            {
            fprintf (stderr, "Error in reading input data (datablockhead)\n");
            exit (3);
            }  

         if (fwrite(&datablockhead,sizeof(datablockhead),1,out_file) != 1)
            {
            fprintf (stderr, "Error in writting output data (datablockhead)\n");
            exit (3);
            }

         for (j=0;j<npts/2;++j)
             {
             if (no_bytes == 2)
             {
                if (fread(&data_short,no_bytes,1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (short)\n");
                   exit (3);
                   }
                real=(double) ntohs(data_short);
 
                if (fread(&data_short,no_bytes,1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (short)\n");
                   exit (3);
                   }
                imag=(double) ntohs(data_short);
               
             }
             else
             {
                if (fread(&data_int,no_bytes,1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (int/float)\n");
                   exit (3);
                   }
                uval.ival = ntohl(data_int);
                if (bit[3] == 0)
                   real = (double) uval.ival;
                else
                   real = (double) uval.fval;
                if (fread(&data_int,no_bytes,1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (int/float)\n");
                   exit (3);
                   }
                uval.ival = ntohl(data_int);
                if (bit[3] == 0)
                   imag = (double) uval.ival;
                else
                   imag = (double) uval.fval;
             }             

             phi=atan2(imag,real);

             phi=phi+2.0* M_PI *delta_f*(j-lsfid)/sw;

             abs_data=sqrt(real*real+imag*imag);

             real=abs_data*cos(phi);  
             imag=abs_data*sin(phi);   

             if (no_bytes == 2)
                {
                data_short = (short)real;
                data_short = htons(data_short);
                if (fwrite(&data_short,no_bytes,1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data (short)\n");
                   exit (3);
                   }     
 
                data_short = (short)imag;
                data_short = htons(data_short);
                if (fwrite(&data_short,no_bytes,1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data (short)\n");
                   exit (3);
                   }
                }
              else
                {
                     
                if (bit[3] == 0)
                   uval.ival = (int)real;
                else
                   uval.fval = (float) real;
                data_int = htonl(uval.ival);
                if (fwrite(&data_int,no_bytes,1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data (int/float)\n");
                   exit (3);
                   }     
                if (bit[3] == 0)
                   uval.ival = (int)imag;
                else
                   uval.fval = (float) imag;
                data_int = htonl(uval.ival);
 
                if (fwrite(&data_int,no_bytes,1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data (int/float)\n");
                   exit (3);
                   }          
                }
            }
        }
   fprintf (stderr, "\nB0 CORRECTION WAS PERFORMED!\n");
   
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
