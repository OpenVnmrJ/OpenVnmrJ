/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
/**********************************************************/ 
/****    PROGRAM: ecc3.c                               ****/
/****    Eddy current correction of FID                ****/
/****    Compatible with integer or floating point     ****/
/****    Author: Ivan Tkac                             ****/
/****    Last update: Aug 08, 2007                     ****/
/**********************************************************/
 
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


/**[3] PROGRAM BEGIN *************************************/
int main(int argc, char *argv[])
{
/**[3.1.] VARIABLE DECLARATION ***************************/
     FILE *in_file1,*in_file2,*out_file;
     int i,j,lsfid_w,lsfid_m,no_bytes,np_metab,np_water;
     double phi_w,phi_m,abs_data,real_w,imag_w,real_m,imag_m,phi_w_ref,phi_m_ref; 
     int   data_int;
     short data_short;
     int bit_m[16],bit_w[16],n,file_status_m,file_status_w,fs;
     double x;
     int nblks;
     int bbytes;
     union u_tag {
      float fval;
      int   ival;
     } uval;

         phi_w_ref = 0.0;
         phi_m_ref = 0.0;

/**[3.2.] CONTROL OF INPUT PARAMETERS ********************/
     if (argc<2)
        {
        fprintf(stderr,"\nName of input file1 (metabolites) was not passed!\n");
	exit (3);
        }

     if (argc<3)
        {
        fprintf(stderr,"\nName of input file2 (water) was not passed!\n");
	exit (3);
        }

     if (argc<4)
        {
	fprintf(stderr,"\nName of output file was not passed!\n");
	exit (3);
        }

     if (argc < 5)
         {
         printf("\nMetabolite lsfid was not read!\n");
         exit (3);
         }
         
     if (argc < 6)
         {
         printf("\nWater reference lsfid was not read!\n");
         exit (3);
         }

/*** COPY OF FILE HEAD **************************************************/
     if ((in_file1=fopen(argv[1],"rb"))==NULL)
        {
	printf("\nCan not open input metabolite file");
	exit (3);
        }
   
     if ((in_file2=fopen(argv[2],"rb"))==NULL)
        {
	printf("\nCan not open input water reference file");
	exit (3);
        }

     if ((out_file=fopen(argv[3],"wb"))==NULL )
        {
	fprintf(stderr, "\n Can't open output file " );
	exit (3);
        }
        
     if (fread(&datafilehead,sizeof(datafilehead),1,in_file2) != 1)
        {
        fprintf (stderr, "Error in reading datafilehead - water reference\n");
	exit (3);
        }
        
     np_water = ntohl(datafilehead.np);
     file_status_w = ntohs(datafilehead.status);

     if (fread(&datafilehead,sizeof(datafilehead),1,in_file1) != 1)
        {
        fprintf (stderr, "Error in reading datafilehead - metabolites\n");
	exit (3);
        }
        
     np_metab = ntohl(datafilehead.np);
     file_status_m = ntohs(datafilehead.status);
     no_bytes = ntohl(datafilehead.ebytes);
     
     if (fwrite(&datafilehead,sizeof(datafilehead),1,out_file) != 1)
        {
	fprintf (stderr, "Error in writting output data (datafilehead)\n");
	exit (3);
        }        
     
     lsfid_m = atof(argv[4]);
     lsfid_w = atof(argv[5]);
     
     fprintf (stderr, "\n******* EDDY CURRENT CORRECTION *******\n");
     fprintf (stderr, "np_metab = %d   np_water = %d\n",np_metab,np_water);
     fprintf (stderr, "lsfid(M) = %d   lsfid(W) = %d\n",lsfid_m,lsfid_w);
     

/*** STATUS OF THE METABOLITE FILE  ****************************************/
     fs = file_status_m;

     for (n=15;n>=0;n--)
         {
         x = pow(2.0,(double)n);
         bit_m[n] = 0;
         if (fs >= (int)x)
            {
            fs += -(int)x;
            bit_m[n] = 1;
            }
         }
         
     fs = file_status_w;
     
     for (n=15;n>=0;n--)
         {
         x = pow(2.0,(double)n);
         bit_w[n] = 0;
         if (fs >= (int)x)
            {
            fs += -(int)x;
            bit_w[n] = 1;
            }
         }

     fprintf (stderr, "\nFILE STATUS (metab) = %d\n",file_status_m);
          
     if (bit_m[3] == 0)
        {
        fprintf (stderr, "Type of data: INTEGER\n");
        }
     if (bit_m[3] == 1)
        {
        fprintf (stderr, "Type of data: FLOATING POINT\n");
        }
     
     fprintf (stderr, "\nFILE STATUS (water) = %d\n",file_status_w);
          
     if (bit_w[3] == 0)
        {
        fprintf (stderr, "Type of data: INTEGER\n\n");
        }
     if (bit_w[3] == 1)
        {
        fprintf (stderr, "Type of data: FLOATING POINT\n\n");
        }


     nblks = ntohl(datafilehead.nblocks);
     bbytes = ntohl(datafilehead.bbytes);
/*** EDDY CURRENT CORRECTION OF ARRAYED DATA *****************************/
     for (i=0;i<nblks;++i)
         {
         fseek(in_file1,sizeof(datafilehead)+i*bbytes,0);
         fseek(in_file2,sizeof(datafilehead),0);
         fseek(out_file,sizeof(datafilehead)+i*bbytes,0);
     
	 if (fread(&datablockhead,sizeof(datablockhead),1,in_file1) != 1)
            {
	    fprintf (stderr, "Error in reading input data - metab. (datablockhead)\n");
            exit (3);
	    }

	 if (fwrite(&datablockhead,sizeof(datablockhead),1,out_file) != 1)
            {
	    fprintf (stderr, "Error in writting output data (datablockhead)\n");
	    exit (3);
            }
         
         if (fread(&datablockhead,sizeof(datablockhead),1,in_file2) != 1)
            {
	    fprintf (stderr, "Error in reading input data - water ref. (datablockhead)\n");
            exit (3);
	    }
         
         if (lsfid_w > 0) fseek(in_file2,2*lsfid_w*no_bytes,1);
         
         
         if (lsfid_m > 0)
            {
            for (j=0;j<2*lsfid_m;++j)
                {
                if (no_bytes == 2)
                   {
                   if (fread(&data_short,sizeof(data_short),1,in_file1) != 1)
                      {
	              fprintf (stderr, "Error in reading input (metabolite)\n");
	              exit (3);
	              }
	              
	           if (fwrite(&data_short,sizeof(data_short),1,out_file) != 1)
                      {
                      fprintf (stderr, "Error in writting in output file");
                      exit (3);
                      }
                   }
                else
                   {
                   if (fread(&data_int,sizeof(data_int),1,in_file1) != 1)
                      {
	              fprintf (stderr, "Error in reading input (metabolite)\n");
	              exit (3);
	              }
	              
	           if (fwrite(&data_int,sizeof(data_int),1,out_file) != 1)
                      {
                      fprintf (stderr, "Error in writting in output file");
                      exit (3);
                      }
                   }
                }
            }
	           
	       
         for (j=0;j<(np_metab/2)-lsfid_m;++j)
             {
             if (j < (np_water/2)-lsfid_w)
                {
                if (no_bytes == 2)
                   {
	           if (fread(&data_short,sizeof(data_short),1,in_file2) != 1)
                      {
	              fprintf (stderr, "Error in reading input real (water)\n");
	              exit (3);
	              }
                   real_w = (double) ntohs(data_short);

	           if (fread(&data_short,sizeof(data_short),1,in_file2) != 1)
                      {
	              fprintf (stderr, "Error in reading integer imag (water)\n");
	              exit (3);
	              }
	           imag_w = (double) ntohs(data_short); 
                   }
                 else
                   {
                   if (fread(&data_int,sizeof(data_int),1,in_file2) != 1)
                      {
	              fprintf (stderr, "Error in reading floating point real (water)\n");
	              exit (3);
	              }
                   uval.ival = ntohl(data_int);
                   if (bit_w[3] == 0)
                      real_w = (double) uval.ival;
                   else
                      real_w = (double) uval.fval;

	           if (fread(&data_int,sizeof(data_int),1,in_file2) != 1)
                      {
	              fprintf (stderr, "Error in reading floating point imag (water)\n");
	              exit (3);
	              }
                   uval.ival = ntohl(data_int);
                   if (bit_w[3] == 0)
                      imag_w = (double) uval.ival;
                   else
                      imag_w = (double) uval.fval;
                   }
                }

             if (no_bytes == 2)
                {
                if (fread(&data_short,sizeof(data_short),1,in_file1) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (metabolites)\n");
                   exit (3);
                   }
                real_m = (double) ntohs(data_short);

                if (fread(&data_short,sizeof(data_short),1,in_file1) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (metabolites)\n");
                   exit (3);
                   }
                imag_m = (double) ntohs(data_short);
                }
             else
                {
                if (fread(&data_int,sizeof(data_int),1,in_file1) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (metabolites)\n");
                   exit (3);
                   }
                uval.ival = ntohl(data_int);
                if (bit_w[3] == 0)
                   real_m = (double) uval.ival;
                else
                   real_m = (double) uval.fval;

                if (fread(&data_int,sizeof(data_int),1,in_file1) != 1)
                   {
                   fprintf (stderr, "Error in reading input data (metabolites)\n");
                   exit (3);
                   }
                uval.ival = ntohl(data_int);
                if (bit_w[3] == 0)
                   imag_m = (double) uval.ival;
                else
                   imag_m = (double) uval.fval;
                }

             phi_w = 0.0;
             phi_m = 0.0;

             phi_w = atan2(imag_w,real_w);
             
             if (j == 0) phi_w_ref = phi_w;
             
             phi_w = phi_w - phi_w_ref;
             
             phi_m = atan2(imag_m,real_m);

             abs_data = sqrt(real_m*real_m+imag_m*imag_m);

             if (j == 0) phi_m_ref = phi_m; 

             phi_m = phi_m  - phi_m_ref - phi_w;

             real_m = abs_data * cos(phi_m);
             imag_m = abs_data * sin(phi_m);

             if (no_bytes == 2)
                {
                data_short = (short) real_m;
                data_short = htons(data_short);
                if (fwrite(&data_short,sizeof(data_short),1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data");
                   exit (3);
                   }

                data_short = (short)imag_m;
                data_short = htons(data_short);
                if (fwrite(&data_short,sizeof(data_short),1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data");
                   exit (3);
                   }
                }
             else
                {
                if (bit_m[3] == 0)
                   uval.ival = (int) real_m;
                else
                   uval.fval = (float) real_m;
                data_int = htonl(uval.ival);
                if (fwrite(&data_int,sizeof(data_int),1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data");
                   exit (3);
                   }

                if (bit_m[3] == 0)
                   uval.ival = (int) imag_m;
                else
                   uval.fval = (float) imag_m;
                data_int = htonl(uval.ival);
                if (fwrite(&data_int,sizeof(data_int),1,out_file) != 1)
                   {
                   fprintf (stderr, "Error in writting output data");
                   exit (3);
                   }
                }
             }
         fprintf(stderr, "ECC performed on FID[%d]\n",i+1);
         }


/*** CLOSING FILES *************************************/

      if (fclose(in_file1) != 0)
         {
         printf ("Error closing input file");
         exit (3);
         }
         
      if (fclose(in_file2) != 0)
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
