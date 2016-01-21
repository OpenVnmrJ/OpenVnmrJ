/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/**************************************************************/ 
/****      PROGRAM NAME: phcor                             ****/
/****      AUTOMATIC PHASE CORRECTION OF FID               ****/
/****      Updated for integers and floating point numbers ****/
/****      Last update: Feb 13, 2007                       ****/ 
/**************************************************************/
 
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
     FILE *in_file,*out_file,*ph_file,*phcor_file;
     double real,imag,phi,avphase,phase_1,abs_data,rmsd,snr,abs_xy0,abs_xy;
     double x[500],y[500],x0[500],y0[500],dphase[500];
     int data_int;
     short data_short;
     int i,j,k,no_points,no_skip,np,no_bytes,no_sd,no_fid,sizef,sizeb;
     int bit[16],n,file_status,fs;
     double p;
     int nblks;
     union u_tag {
      float fval;
      int   ival;
     } uval;


/*** CONTROL OF INPUT PARAMETERS ***********************/
     if (argc < 2)
         {
         printf("\nName of input file was not passed!\n");
         exit (3);
         }  

     if (argc < 3)
         {
         printf("\nName of output file was not passed!\n");
         exit (3);
         }
         
     if (argc < 4)
         {
         printf("\nNumber of points for phase correction was not read!\n");
         exit (3);
         }

     if (argc < 5)
         {
         printf("\nNumber of skipped points was not read!\n");
         exit (3);
         }
         
     if (argc < 6)
         {
         printf("\nNumber points for RMSD was not read!\n");
         exit (3);
         }

     if (argc < 7)
         {
         printf("\nMultiplication factor for S/N was not read!\n");
         exit (3);
         }

     if (argc < 8)
         {
         printf("Name of phase file was not passed!\n\n");
         exit (3);
         }     

     if (argc < 9)
         {
         printf("\nFID# was not read!\n");
         exit (3);
         }     

     if (argc < 10)
         {
         printf("\nName of file with averaged phase was not passed!\n");
         exit (3);
         }     
         
/*** OPEN THE FILES ***********************************/         
     if ((in_file=fopen(argv[1],"rb"))==NULL)
        {
        printf("\nCan not open input file");
        exit (3);
        }

     rewind(in_file); 
     
     sizef = sizeof(datafilehead);
     sizeb = sizeof(datablockhead);

     if (fread(&datafilehead,sizef,1,in_file) != 1)
         {
         fprintf (stderr, "Error in reading input data (datafilehead)\n");
         exit (3);
         }
         
     file_status = ntohs(datafilehead.status);

     if ((out_file=fopen(argv[2],"wb"))==NULL)
        {
        printf("\nCan not open output file");
        exit (3);
        }

     rewind(out_file);   

     if (fwrite(&datafilehead,sizef,1,out_file) != 1)
         {
         fprintf (stderr, "Error in writting output data (datafilehead)\n");
         exit (3);
         }

     if ((ph_file=fopen(argv[7],"wb"))==NULL)
        {
        printf("\nCan not open output phase file");
        exit (3);
        }

     rewind(ph_file);

     if ((phcor_file=fopen(argv[9],"wb"))==NULL)
        {
        printf("\nCan not open output phcor file");
        exit (3);
        }

     rewind(phcor_file);

/*** STATUS OF THE FILE **********************/
     fs = file_status;
     
     for (n=15;n>=0;n--)
         {
         p = pow(2.0,(double)n);
         bit[n] = 0;
         if (fs >= (int)p)
            {
            fs += -(int)p;
            bit[n] = 1;
            }
         }
           
     fprintf (stderr, "\n********* FID PROCESSING **********\n");
     fprintf (stderr, "FILE STATUS = %d\n",file_status);

     if (bit[3] == 0)
        {
        fprintf (stderr, "bit[3] = %d   Type of data: INTEGER\n",bit[3]);
        }
     if (bit[3] == 1)
        {
        fprintf (stderr, "bit[3] = %d   Type of data: FLOATING POINT\n",bit[3]);
        }

/*** READ IN REFERENCE FID  *******************/
     np = ntohl(datafilehead.np);
     no_bytes = ntohl(datafilehead.ebytes);
     
     no_points = atoi(argv[3]);
     if (no_points > 500)
        {
        fprintf (stderr, "Number of points is too high (max = 500)");
        exit (3);
        }

     no_skip = atoi(argv[4]);
     if (no_skip > 100)
        {
        fprintf (stderr, "Number of skipped points is too high (max = 100)");
        exit (3);
        }
        
     no_sd = atoi(argv[5]);
     if (no_sd > (int)(np/8))
        {
        fprintf (stderr, "Number of points for RMSD calculation is too high (max = np/8)");
        exit (3);
        }  
      
     snr = atof(argv[6]);
     
     no_fid = atoi(argv[8]);
     nblks = ntohl(datafilehead.nblocks);
     if (no_fid > nblks)
        {
        fprintf (stderr, "FID# is too high (max = arraydim)");
        exit (3);
        }

     if (no_fid < 2)
        {
        fprintf (stderr, "FID# is too low (min = 2)");
        exit (3);
        }

     fprintf (stderr, "\nSkipped points = %d\n",no_skip);       
     fprintf (stderr, "Number of points for phase correction = %d\n",no_points);

     fseek(in_file,sizef+sizeb+2*no_skip*no_bytes,0);

     for (j=0;j<no_points;++j)
         {
         if (no_bytes == 2)
            {
            if (fread(&data_short,no_bytes,1,in_file) != 1)
               {
               fprintf (stderr, "Error in reading input data (reference FID, real)");
               exit (3);
               }
            x0[j] = (double) ntohs(data_short);
         
            if (fread(&data_short,no_bytes,1,in_file) != 1)
               {
               fprintf (stderr, "Error in reading input data (reference FID, imag)");
               exit (3);
               }
            y0[j] = (double) ntohs(data_short);
            }
         else
            {
            if (fread(&data_int,no_bytes,1,in_file) != 1)
               {
               fprintf (stderr, "Error in reading input data (reference FID, real)");
               exit (3);
               }
            uval.ival = ntohl(data_int);
            if (bit[3] == 0)
               x0[j] = (double)uval.ival;
            else
               x0[j] = (double)uval.fval;

            if (fread(&data_int,no_bytes,1,in_file) != 1)
               {
               fprintf (stderr, "Error in reading input data (reference FID, imag)");
               exit (3);
               }
            uval.ival = ntohl(data_int);
            if (bit[3] == 0)
               y0[j] = (double)uval.ival;
            else
               y0[j] = (double)uval.fval;
            }
         }


/*** CALCULATION OF NOISE LEVEL  ***********/ 
     fseek(in_file,sizef+sizeb+(np-2*no_sd)*no_bytes,0);
     
     rmsd = 0.0;
     for (j=0;j<(no_sd);++j)
         {
         if (no_bytes == 2)
            {
            if (fread(&data_short,no_bytes,1,in_file) != 1)
               {
               fprintf (stderr, "Error in reading data for RMSD (real)");
               exit (3);
               }
            real = (double) ntohs(data_short);
         
            if (fread(&data_short,no_bytes,1,in_file) != 1)
               {
               fprintf (stderr, "Error in reading data for RMSD (imag)");
               exit (3);
               }
            imag = (double) ntohs(data_short);
            }
         else
            {
            if (fread(&data_int,no_bytes,1,in_file) != 1)
               {
               fprintf (stderr, "Error in reading data for RMSD (real)");
               exit (3);
               }
            uval.ival = ntohl(data_int);
            if (bit[3] == 0)
               real = (double)uval.ival;
            else
               real = (double)uval.fval;
         
            if (fread(&data_int,no_bytes,1,in_file) != 1)
               {
               fprintf (stderr, "Error in reading data for RMSD (imag)");
               exit (3);
               }
            uval.ival = ntohl(data_int);
            if (bit[3] == 0)
               imag = (double)uval.ival;
            else
               imag = (double)uval.fval;
            }
           
          rmsd = rmsd+real*real+imag*imag;
          }
          
     rmsd = sqrt(rmsd/(no_sd-1));
     fprintf (stderr, "Noise level = %8.4f\n",rmsd);    
             
         
/*** COPY OF THE FIRST BLOCK  **************/
     fseek(in_file,sizeof(datafilehead),0);
   
     if (fread(&datablockhead,sizeb,1,in_file) != 1)
        {
        fprintf (stderr, "Error in reading input data (datablockhead)\n");
        exit (3);
        }  
  
     if (fwrite(&datablockhead,sizeb,1,out_file) != 1)
        {
        fprintf (stderr, "Error in writting output data (datablockhead)\n");
        exit (3);
        }
                      
     for (j=0;j<np;++j)
         {
         if (no_bytes == 2)
            {
            if (fread(&data_short,no_bytes,1,in_file) != 1)
               {
               fprintf (stderr, "Error in reading input data (FID 1)");
               exit (3);
               }
            if (fwrite(&data_short,sizeof(data_short),1,out_file) != 1)
               {
               fprintf (stderr, "Error in writting output data (FID 1)");
               exit (3);
               }
            }
         else
            {
            if (fread(&data_int,no_bytes,1,in_file) != 1)
               {
               fprintf (stderr, "Error in reading input data (FID 1)");
               exit (3);
               }
            if (fwrite(&data_int,sizeof(data_int),1,out_file) != 1)
               {
               fprintf (stderr, "Error in writting output data (FID 1)");
               exit (3);
               }
            }
         }
         
/*** PHASE CORRECTION  **********************/           
     fprintf (stderr, "\n******** PHASE CALCULATION *********\n");
     fprintf (stderr, " Phase calculation for FID[%d]\n\n",no_fid);
     fprintf (stderr, " Point  Ampl(0)   Ampl(i)   Phase\n");
     
     fprintf(phcor_file,"   S/N = %5.1f\n\n",snr);
     
     for (i=1;i<nblks;++i)
         {
         if (fread(&datablockhead,sizeb,1,in_file) != 1)
             {
             fprintf (stderr, "Error in reading input data (datablockhead)\n");
             exit (3);
             }  

         if (fwrite(&datablockhead,sizeb,1,out_file) != 1)
            {
            fprintf (stderr, "Error in writting output data (datablockhead)\n");
            exit (3);
            }
            
         fseek(in_file,2*no_skip*no_bytes,1); 
         
         avphase = 0.0;
         phase_1 = 0.0;
         
         k = 0;
         for (j=0;j<no_points;++j)
             {
             if (no_bytes == 2)
                {
                if (fread(&data_short,no_bytes,1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (real)");
                   exit (3);
                   }
                x[j] = (double) ntohs(data_short);
                
                if (fread(&data_short,no_bytes,1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (imag)");
                   exit (3);
                   }
                y[j] = (double) ntohs(data_short);
                }
             else
                {
                if (fread(&data_int,no_bytes,1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (real)");
                   exit (3);
                   }
                uval.ival = ntohl(data_int);
                if (bit[3] == 0)
                   x[j] = (double)uval.ival;
                else
                   x[j] = (double)uval.fval;
                
                if (fread(&data_int,no_bytes,1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (imag)");
                   exit (3);
                   }
                uval.ival = ntohl(data_int);
                if (bit[3] == 0)
                   y[j] = (double)uval.ival;
                else
                   y[j] = (double)uval.fval;
                }
             
             abs_xy0 = sqrt(x0[j]*x0[j]+y0[j]*y0[j]);
             abs_xy = sqrt(x[j]*x[j]+y[j]*y[j]);

             if ((abs_xy0 > snr*rmsd) && (abs_xy > snr*rmsd))
                {
                dphase[j] = atan2((x0[j]*y[j]-x[j]*y0[j]),(x0[j]*x[j]+y0[j]*y[j]));
                if (k == 0) phase_1 = dphase[j];
                if ((k > 0) && (abs(dphase[j]-phase_1) > 1.5* M_PI)) dphase[j] = - dphase[j];
                avphase = avphase+dphase[j];
                k = k + 1;
                
                if (i == no_fid-1)
                   {
                   fprintf (stderr, " %4d  %8.2f  %8.2f  %+5.1f\n",
                                      j+1,abs_xy0,abs_xy,dphase[j]*180.0/ M_PI);
                   fprintf(ph_file,"  %4d    %8.2f\n",j+1,dphase[j]*180.0/ M_PI);
                   }                
                }
             }

         if (k == 0)
            {
            avphase = 0;
            }
         else
            {       
            avphase = avphase/k;
            }
         
         if (i == 1) fprintf (stderr,"\n****** CALCULATED PHASE DIFFERENCE ******\n");
         if (i == 1) fprintf (stderr,"FID[%3d]  Reference fid\n",i);
         fprintf (stderr, "FID[%3d]  points = %3d  phase = %+6.2f\n",i+1,k,avphase*180.0/M_PI);
         fprintf(phcor_file,"  %4d   %4d   %8.2f\n",i+1,k,avphase*180.0/M_PI);
                
         fseek(in_file,-(2*(no_points+no_skip)*no_bytes),1);      
         for (j=0;j<np/2;++j)
             {
             if (no_bytes == 2)
                {
                if (fread(&data_short,no_bytes,1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (real)");
                   exit (3);
                   }
                real = (double) ntohs(data_short);
                
                if (fread(&data_short,no_bytes,1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (imag)");
                   exit (3);
                   }
                imag = (double) ntohs(data_short);
                }
             else
                {
                if (fread(&data_int,no_bytes,1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (real)");
                   exit (3);
                   }
                uval.ival = ntohl(data_int);
                if (bit[3] == 0)
                   real = (double)uval.ival;
                else
                   real = (double)uval.fval;
                
                if (fread(&data_int,no_bytes,1,in_file) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (imag)");
                   exit (3);
                   }
                uval.ival = ntohl(data_int);
                if (bit[3] == 0)
                   imag = (double)uval.ival;
                else
                   imag = (double)uval.fval;
                }

             phi=atan2(imag,real);
             
             phi=phi-avphase;

             abs_data=sqrt(real*real+imag*imag);

             real=abs_data*cos(phi);  
             imag=abs_data*sin(phi);   

             if (no_bytes == 2)
                {
                data_short = (short)real;
                data_short = htons(data_short);
                if (fwrite(&data_short,sizeof(data_short),1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data");
                   exit (3);
                   }
 
                data_short = (short)imag;
                data_short = htons(data_short);
                if (fwrite(&data_short,sizeof(data_short),1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data");
                   exit (3);
                   }
                }
             else
                {
                if (bit[3] == 0)
                   uval.ival = (int)real;
                else
                   uval.fval = (float)real;
                data_int = ntohl(uval.ival);
                if (fwrite(&data_int,sizeof(data_int),1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data");
                   exit (3);
                   }
 
                if (bit[3] == 0)
                   uval.ival = (int)imag;
                else
                   uval.fval = (float)imag;
                data_int = ntohl(uval.ival);
                if (fwrite(&data_int,sizeof(data_int),1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data");
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

      if (fclose(ph_file) != 0) 
         {
         printf ("Error closing output phase file"); 
         exit (3);      
         }

      if (fclose(phcor_file) != 0) 
         {
         printf ("Error closing phcor file"); 
         exit (3);      
         }
      exit(EXIT_SUCCESS);
}
