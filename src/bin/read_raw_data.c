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
/******************************************************************/
/*  read_raw_data - Program to extract data, header, and status   */
/*                  from FID file                                 */
/*                                                                */
/*                                                                */
/*    read_raw_data <arg1> <arg2> <arg3> arg<4>                   */
/*                                                                */
/*                  <arg1>  - dh (data header),                   */
/*                            ds (data status),                   */     
/*                            bh (block header),                  */
/*                            bs (block status)                   */
/*                            ch (hypercomplex header)            */
/*                            cs (hpercomplex status)             */
/*                            data (rawdata)                      */
/*                            max (maximum of each fid)           */
/*                  <arg2>  - displ (display) and file (output    */
/*                            result to file.                     */
/*                  <arg3>  - input filename and path. This       */
/*                            argument can be omitted and will    */
/*                            default to 'fid'.                   */
/*                  <arg4>  - output filename. This argument can  */
/*                            be omitted and will default to      */
/*                            'rawdata'.                          */
/*                                                                */
/*                                                                */
/*  Modification History                                          */
/*                                                                */
/*  Date        Author        Modifications                       */
/* ---------   -----------   ------------------------------------ */
/* 03-Dec-02   M. Meiler     Initial development                  */
/* 06-Dec-02   M. Meiler     Finished initial code                */
/*                                                                */
/******************************************************************/

#include <string.h> 
#include <stdio.h> 
#include <math.h> 
#include <stdlib.h>

#define  MAXSTR  256
#define  MAXARG  5
#define  MAXOPT  8

struct HEADER
	  {
          long	 nblocks;              /* number of blocks in file */
	  long	 ntraces;              /* number of traces in file */
	  long	 np;                   /* number of elements pre trace */
	  long 	 ebytes;               /* number of bytes per element */
	  long	 tbytes;               /* number of bytes per trace */
	  long	 bbytes;               /* number of bytes per block */
	  short  vers_id;              /* software version, file_di status bits */
	  short	 status;               /* status of whole file */
          long	 nbheaders;            /* number of block headers per block */
	   } header;


struct DATABLOCKHEAD
	   {
	   short   scale;	 	/* scaling factor */
	   short   status;		/* status of data in block */
	   short   index;               /* block index */
	   short   mode;                /* mode of data in block */
	   long    ctcount;             /* ct value for FID */
	   float   lpval;               /* f2 (2D-f1) left phase in phase file */
	   float   rpval;               /* f2 (2D-f1) right phase in phase file */
	   float   lvl;                 /* level of drift correction */
	   float   tlt;                 /* tilt drift correction */
	   } dbheader;

struct HYPERCOMPLEXHEADER
           {
	   short   s_spare1;            /* short word: spare */
	   short   status;              /* status wird for block header */
	   short   s_spare2;            /* short word: spare */
	   short   s_spare3;            /* short word: spare */
	   long    l_spare1;            /* long word:  spare */
	   float   lpval1;              /* 2D-f2 left phase */
	   float   rpval1;              /* 2D f2 right phase */
	   float   f_spare1;            /* float word: spare */
	   float   f_spare2;            /* float word: spare */
	   }hcompheader;

struct ACTION
           {
	   unsigned   header:1;               /* bit to display file header */
	   unsigned   header_status:1;        /* bit ot display file status */
	   unsigned   blockheader:1;          /* bit to display block header */
	   unsigned   blockheader_status:1;   /* bit to display block status */
	   unsigned   complex_blockheader:1;  /* bit to display hypercomplex block header */
	   unsigned   complex_status:1;       /* bit to dispaly hypercomplex status */
	   unsigned   data:1;                 /* bit ot display data */
	   unsigned   max:1;                  /* bit to extract max from data */
	   } action;             


/**********************************************************
 *      display header of FID file                        *
 **********************************************************/
void show_header(struct HEADER header)
{
     printf("\nHEADER \n");
     printf("===================================\n");
     printf("Blocks in file:          %ld\n",header.nblocks);
     printf("Traces per block:        %ld\n",header.ntraces);
     printf("Elements per trace:      %ld\n",header.np);
     printf("Bytes per element:       %ld\n",header.ebytes);
     printf("Bytes per trace:         %ld\n",header.tbytes);
     printf("Bytes per block:         %ld\n",header.bbytes);
     printf("Software Version:        %i\n",header.vers_id);
     printf("Status:                  %x[hex]\n",header.status);
     printf("Block headers per block: %ld\n",header.nbheaders);
     printf("===================================\n\n");
}

/**********************************************************
 *      display header status from FID file               *
 **********************************************************/
void show_header_status (short status)
{
     printf("\nSTATUS - HEADER\n");
     printf("===================================\n");
     printf("Status bytes:     %x[hex]\n\n",status);
     printf("Bit 0:  %x\tNo data    / Data\n",header.status & 0x01);   
     printf("Bit 1:  %x\tFID        / Spectrum\n",header.status >> 1 & 0x01);
     printf("Bit 2:  %x\t16-bit     / 32-bit integer (if Bit 3  =0 otherwise ignored)\n",(header.status >> 2) & 0x01 );
     printf("Bit 3:  %x\tInteger    / Float\n",(header.status >> 3) & 0x01 );   
     printf("Bit 4:  %x\tReal       / Complex\n",(header.status >> 4) & 0x01 );
     printf("Bit 5:  %x\t -         / Hypercomplex\n",(header.status >> 5) & 0x01 );          
     printf("Bit 6:  - \tNot used\n");           
     printf("Bit 7:  %x\tnot Acqpar / Acqpar\n",(header.status >> 6) & 0x01);     
     printf("Bit 8:  %x\t1st FFT    / 2nd FFT\n",(header.status >> 7)  & 0x01);     
     printf("Bit 9:  %x\tRegular    / Transposed\n",(header.status >> 8) & 0x01);     
     printf("Bit 10: - \tNot used\n");                
     printf("Bit 11: %x\t -         / np dimension active\n",(header.status >> 9) & 0x01);     
     printf("Bit 12: %x\t -         / nf dimension active\n",(header.status >> 10) & 0x01);          	  
     printf("Bit 13: %x\t -         / ni dimension active\n",(header.status >> 11) & 0x01);     
     printf("Bit 14: %x\t -         / ni2 dimension active\n",(header.status >> 12) & 0x01);          
     printf("Bit 15: - \tNot used\n");                
     printf("===================================\n");

}

/**********************************************************
 *      display datablock header                          *
 **********************************************************/
void show_datablock_header(struct DATABLOCKHEAD dbheader, int counter)
{
     printf("\nDATABLOCK HEADER #%i \n",counter);
     printf("===================================\n");
     printf("Scaling factor:             %i\n",dbheader.scale);
     printf("Status of data in block:    %x[hex]\n",dbheader.status);
     printf("Block Index:                %i\n",dbheader.index);
     printf("Mode of data in block:      %i\n",dbheader.mode);
     printf("ct value of FID:            %ld\n",dbheader.ctcount);
     printf("Left phase in phase file:   %f\n",dbheader.lpval);
     printf("Right phase in phase file:  %f\n",dbheader.rpval);
     printf("Left drift correction:      %f\n",dbheader.lvl);
     printf("Tilt drift correction:      %f\n",dbheader.tlt);
     printf("===================================\n\n");
}

/**********************************************************
 *      display datablock header status                    *
 **********************************************************/
void show_datablock_status (short status, int counter)
{
     printf("\nSTATUS - DATABLOCK HEADER #%i \n",counter);
     printf("===================================\n");
     printf("Status bytes:     %x[hex]\n\n",status);
     printf("Bit 0:  %x\tNo data   / Data\n",dbheader.status & 0x1);   
     printf("Bit 1:  %x\tFID       / Spectrum\n",(dbheader.status >> 1) & 0x01);
     printf("Bit 2:  %i\t16-bit    / 32-bit integer (if Bit 3  =0 otherwise ignored)\n",(dbheader.status >> 2)  & 0x01);
     printf("Bit 3:  %x\tInteger   / Float\n",(dbheader.status >> 3) & 0x01);   
     printf("Bit 4:  %x\tReal      / Complex\n",(dbheader.status >> 4) & 0x01);
     printf("Bit 5:  %x\t -        / Hypercomplex\n",(dbheader.status >> 5) & 0x01);          
     printf("Bit 6:  - \tNot used\n");           
     printf("Bit 7:  %x\tmore blocks: absent / present\n",(dbheader.status >> 7) & 0x01);     
     printf("Bit 8:  %x\tnp real   / np complex\n",(dbheader.status >> 8)& 0x010);     
     printf("Bit 9:  %x\tnf real   / nf complex\n",(dbheader.status >> 9) & 0x01);     
     printf("Bit 10: - \tNot used\n");                
     printf("Bit 11: %x\tni real   / ni complex\n",(dbheader.status >> 11) & 0x01);     
     printf("Bit 12: %x\tni2 real  / ni2 complex\n",(dbheader.status >> 12) & 0x01);          	  
     printf("Bit 13: - \tNot used\n");           
     printf("Bit 14: - \tNot used\n");           
     printf("Bit 15: - \tNot used\n");           
     printf("===================================\n");

}

/**********************************************************
 *      display hypercomplex datablock header             *
 **********************************************************/
void show_complex_header(struct HYPERCOMPLEXHEADER hcompheader,short block_no)
{
     printf("\nHYPERCOMPLEX DATABLOCK HEADER #%i \n",block_no);
     printf("===================================\n");
     printf("Spare 1 (short):            %i\n",hcompheader.s_spare1);
     printf("Status:                :    %x[hex]\n",hcompheader.status);
     printf("Spare 2 (short):            %i\n",hcompheader.s_spare2);
     printf("Spare 3 (short):            %i\n",hcompheader.s_spare3);
     printf("Spare 4 (long):             %ld\n",hcompheader.l_spare1);
     printf("2D-f2:left phase:           %f\n",hcompheader.lpval1);
     printf("2D-f2 rigth phase:          %f\n",hcompheader.rpval1);
     printf("Spare 5 (float):            %f\n",hcompheader.f_spare1);
     printf("Spare 6 (float):            %f\n",hcompheader.f_spare2);
     printf("===================================\n\n");
}

/**********************************************************
 *      display hypercompled header status                *
 **********************************************************/
void show_complex_status (short status,short block_no)
{
     printf("\nSTATUS - HYPERCOMPLEX DATABLOCK HEADER #%i \n",block_no);
     printf("===================================\n");
     printf("Status bytes:     %x[hex]\n\n",status);
     printf("Bit 0:  %x\t- / np: ph mode\n",hcompheader.status & 0x1);   
     printf("Bit 1:  %x\t- / np: av mode\n",(hcompheader.status >>1) & 0x01);
     printf("Bit 2:  %x\t- / np: pwr mode\n",(hcompheader.status >>2) & 0x01);
     printf("Bit 3:  - \tNot used\n");           
     printf("Bit 4:  %x\t- / nf: ph mode   (currently not used)\n",(hcompheader.status >> 4) & 0x01);   
     printf("Bit 5:  %x\t- / nf: av mode   (currently not used)\n",(hcompheader.status >> 5) & 0x01);
     printf("Bit 6:  %x\t- / nf: pwr mode  (currently not used)\n",(hcompheader.status >> 6) & 0x01);
     printf("Bit 7:  - \tNot used\n");           
     printf("Bit 8:  %x\t- / ni: ph mode   (currently not used)\n",(hcompheader.status >> 8) & 0x01);   
     printf("Bit 9:  %x\t- / ni: av mode   (currently not used)\n",(hcompheader.status >> 9) & 0x01);
     printf("Bit 10: %x\t- / ni: pwr mode  (currently not used)\n",(hcompheader.status >> 10) & 0x01);
     printf("Bit 11:  - \tNot used\n");           
     printf("Bit 12: %x\t- / ni2: ph mode  (currently not used)\n",(hcompheader.status >> 12) & 0x01);   
     printf("Bit 13: %x\t- / ni2: av mode  (currently not used)\n",(hcompheader.status >> 13) & 0x01);
     printf("Bit 14: %x\t- / ni2: pwr mode (currently not used)\n",(hcompheader.status >> 14) & 0x01);
     printf("Bit 15:  - \tNot used\n");           
     printf("===================================\n");

}

/**********************************************************
 *      write file header                                 *
 **********************************************************/
void write_header(struct HEADER header, FILE *fpout)
{ 
     fprintf(fpout,"\nHEADER \n");
     fprintf(fpout,"===================================\n");
     fprintf(fpout,"Blocks in file:          %ld\n",header.nblocks);
     fprintf(fpout,"Traces per block:        %ld\n",header.ntraces);
     fprintf(fpout,"Elements per trace:      %ld\n",header.np);
     fprintf(fpout,"Bytes per element:       %ld\n",header.ebytes);
     fprintf(fpout,"Bytes per trace:         %ld\n",header.tbytes);
     fprintf(fpout,"Bytes per block:         %ld\n",header.bbytes);
     fprintf(fpout,"Software Version:        %i\n",header.vers_id);
     fprintf(fpout,"Status:                  %x[hex]\n",header.status);
     fprintf(fpout,"Block headers per block: %ld\n",header.nbheaders);
     fprintf(fpout,"===================================\n\n");      
}
 
/**********************************************************
 *      write file header                                 *
 **********************************************************/
void write_header_status(short status, FILE *fpout)
{ 
     fprintf(fpout,"\nSTATUS - HEADER\n");
     fprintf(fpout,"===================================\n");
     fprintf(fpout,"Status bytes:     %x[hex]\n\n",status);
     fprintf(fpout,"Bit 0:  %x\tNo data    / Data\n",header.status & 0x01);   
     fprintf(fpout,"Bit 1:  %x\tFID        / Spectrum\n",header.status >> 1 & 0x01);
     fprintf(fpout,"Bit 2:  %x\t16-bit     / 32-bit integer (if Bit 3  =0 otherwise ignored)\n",(header.status >> 2) & 0x01 );
     fprintf(fpout,"Bit 3:  %x\tInteger    / Float\n",(header.status >> 3) & 0x01 );   
     fprintf(fpout,"Bit 4:  %x\tReal       / Complex\n",(header.status >> 4) & 0x01 );
     fprintf(fpout,"Bit 5:  %x\t -         / Hypercomplex\n",(header.status >> 5) & 0x01 );          
     fprintf(fpout,"Bit 6:  - \tNot used\n");           
     fprintf(fpout,"Bit 7:  %x\tnot Acqpar / Acqpar\n",(header.status >> 6) & 0x01);     
     fprintf(fpout,"Bit 8:  %x\t1st FFT    / 2nd FFT\n",(header.status >> 7)  & 0x01);     
     fprintf(fpout,"Bit 9:  %x\tRegular    / Transposed\n",(header.status >> 8) & 0x01);     
     fprintf(fpout,"Bit 10: - \tNot used\n");                
     fprintf(fpout,"Bit 11: %x\t -         / np dimension active\n",(header.status >> 9) & 0x01);     
     fprintf(fpout,"Bit 12: %x\t -         / nf dimension active\n",(header.status >> 10) & 0x01);          	  
     fprintf(fpout,"Bit 13: %x\t -         / ni dimension active\n",(header.status >> 11) & 0x01);     
     fprintf(fpout,"Bit 14: %x\t -         / ni2 dimension active\n",(header.status >> 12) & 0x01);          
     fprintf(fpout,"Bit 15: - \tNot used\n");                
     fprintf(fpout,"===================================\n");
}
 
/**********************************************************
 *      write datablock header                            *
 **********************************************************/
void write_datablock_header (struct DATABLOCKHEAD dbheader, FILE *fpout)
{
     fprintf(fpout,"\nDATABLOCK HEADER #%i \n",dbheader.index);
     fprintf(fpout,"===================================\n");
     fprintf(fpout,"Scaling factor:             %i\n",dbheader.scale);
     fprintf(fpout,"Status of data in block:    %x[hex]\n",dbheader.status);
     fprintf(fpout,"Block Index:                %i\n",dbheader.index);
     fprintf(fpout,"Mode of data in block:      %i\n",dbheader.mode);
     fprintf(fpout,"ct value of FID:            %ld\n",dbheader.ctcount);
     fprintf(fpout,"Left phase in phase file:   %f\n",dbheader.lpval);
     fprintf(fpout,"Right phase in phase file:  %f\n",dbheader.rpval);
     fprintf(fpout,"Left drift correction:      %f\n",dbheader.lvl);
     fprintf(fpout,"Tilt drift correction:      %f\n",dbheader.tlt);
     fprintf(fpout,"===================================\n\n");
}

/**********************************************************
 *      write datablock status                            *
 **********************************************************/
void write_datablock_status (short status, FILE *fpout)
{
     fprintf(fpout,"\nSTATUS - DATABLOCK HEADER #%i \n",dbheader.index);
     fprintf(fpout,"===================================\n");
     fprintf(fpout,"Status bytes:     %x[hex]\n\n",status);
     fprintf(fpout,"Bit 0:  %x\tNo data   / Data\n",dbheader.status & 0x1);   
     fprintf(fpout,"Bit 1:  %x\tFID       / Spectrum\n",(dbheader.status >> 1) & 0x01);
     fprintf(fpout,"Bit 2:  %i\t16-bit    / 32-bit integer (if Bit 3  =0 otherwise ignored)\n",(dbheader.status >> 2)  & 0x01);
     fprintf(fpout,"Bit 3:  %x\tInteger   / Float\n",(dbheader.status >> 3) & 0x01);   
     fprintf(fpout,"Bit 4:  %x\tReal      / Complex\n",(dbheader.status >> 4) & 0x01);
     fprintf(fpout,"Bit 5:  %x\t -        / Hypercomplex\n",(dbheader.status >> 5) & 0x01);          
     fprintf(fpout,"Bit 6:  - \tNot used\n");           
     fprintf(fpout,"Bit 7:  %x\tmore blocks: absent / present\n",(dbheader.status >> 7) & 0x01);     
     fprintf(fpout,"Bit 8:  %x\tnp real   / np complex\n",(dbheader.status >> 8)& 0x010);     
     fprintf(fpout,"Bit 9:  %x\tnf real   / nf complex\n",(dbheader.status >> 9) & 0x01);     
     fprintf(fpout,"Bit 10: - \tNot used\n");                
     fprintf(fpout,"Bit 11: %x\tni real   / ni complex\n",(dbheader.status >> 11) & 0x01);     
     fprintf(fpout,"Bit 12: %x\tni2 real  / ni2 complex\n",(dbheader.status >> 12) & 0x01);          	  
     fprintf(fpout,"Bit 13: - \tNot used\n");           
     fprintf(fpout,"Bit 14: - \tNot used\n");           
     fprintf(fpout,"Bit 15: - \tNot used\n");           
     fprintf(fpout,"===================================\n");
}
 
/**********************************************************
 *      write hypercomplex datablock header               *
 **********************************************************/
void write_complex_header (struct HYPERCOMPLEXHEADER hcompheader, short block_no, FILE *fpout)
{
     fprintf(fpout,"\nHYPERCOMPLEX DATABLOCK HEADER #%i \n",block_no);
     fprintf(fpout,"===================================\n");
     fprintf(fpout,"Spare 1 (short):            %i\n",hcompheader.s_spare1);
     fprintf(fpout,"Status:                :    %x[hex]\n",hcompheader.status);
     fprintf(fpout,"Spare 2 (short):            %i\n",hcompheader.s_spare2);
     fprintf(fpout,"Spare 3 (short):            %i\n",hcompheader.s_spare3);
     fprintf(fpout,"Spare 4 (long):             %ld\n",hcompheader.l_spare1);
     fprintf(fpout,"2D-f2:left phase:           %f\n",hcompheader.lpval1);
     fprintf(fpout,"2D-f2 rigth phase:          %f\n",hcompheader.rpval1);
     fprintf(fpout,"Spare 5 (float):            %f\n",hcompheader.f_spare1);
     fprintf(fpout,"Spare 6 (float):            %f\n",hcompheader.f_spare2);
     fprintf(fpout,"===================================\n\n");
} 

/**********************************************************
 *      write hypercomplex datablock status               *
 **********************************************************/
void write_complex_status (short status, short block_no, FILE *fpout)
{
     fprintf(fpout,"\nSTATUS - HYPERCOMPLEX DATABLOCK HEADER #%i \n",block_no);
     fprintf(fpout,"===================================\n");
     fprintf(fpout,"Status bytes:     %x[hex]\n\n",status);
     fprintf(fpout,"Bit 0:  %x\t- / np: ph mode\n",hcompheader.status & 0x1);   
     fprintf(fpout,"Bit 1:  %x\t- / np: av mode\n",(hcompheader.status >>1) & 0x01);
     fprintf(fpout,"Bit 2:  %x\t- / np: pwr mode\n",(hcompheader.status >>2) & 0x01);
     fprintf(fpout,"Bit 3:  - \tNot used\n");           
     fprintf(fpout,"Bit 4:  %x\t- / nf: ph mode   (currently not used)\n",(hcompheader.status >> 4) & 0x01);   
     fprintf(fpout,"Bit 5:  %x\t- / nf: av mode   (currently not used)\n",(hcompheader.status >> 5) & 0x01);
     fprintf(fpout,"Bit 6:  %x\t- / nf: pwr mode  (currently not used)\n",(hcompheader.status >> 6) & 0x01);
     fprintf(fpout,"Bit 7:  - \tNot used\n");           
     fprintf(fpout,"Bit 8:  %x\t- / ni: ph mode   (currently not used)\n",(hcompheader.status >> 8) & 0x01);   
     fprintf(fpout,"Bit 9:  %x\t- / ni: av mode   (currently not used)\n",(hcompheader.status >> 9) & 0x01);
     fprintf(fpout,"Bit 10: %x\t- / ni: pwr mode  (currently not used)\n",(hcompheader.status >> 10) & 0x01);
     fprintf(fpout,"Bit 11:  - \tNot used\n");           
     fprintf(fpout,"Bit 12: %x\t- / ni2: ph mode  (currently not used)\n",(hcompheader.status >> 12) & 0x01);   
     fprintf(fpout,"Bit 13: %x\t- / ni2: av mode  (currently not used)\n",(hcompheader.status >> 13) & 0x01);
     fprintf(fpout,"Bit 14: %x\t- / ni2: pwr mode (currently not used)\n",(hcompheader.status >> 14) & 0x01);
     fprintf(fpout,"Bit 15:  - \tNot used\n");           
     fprintf(fpout,"===================================\n");

}
/**********************************************************
 *   show_syntax                                          *
 **********************************************************/
void show_syntax (void)
{
printf("\n\nSYNTAX :\n");
printf("read_raw_data <arg1> <arg2> <arg3> <arg4>\n\n");
printf("              <arg1> = dh | ds | bh | bs | ch | cs | data | max \n");
printf("              <arg2> = disp | file\n");
printf("              <arg3> = source filename (default = fid) \n");
printf("              <arg4> = target filename (default = rawdata) \n\n");
}


/**********************************************************
 *   Write MAX:                                           *
 **********************************************************/
void write_max (float max, long block_no, long block, int trace, FILE *fpout)
{
      if ((block == 1) && (trace ==0))
          {
	  fprintf(fpout,"\nMAXIMA(S) :\n");
	  fprintf(fpout,"=====================\n");
	  }
     fprintf(fpout,"BLOCK %d\t Trace %d ::  %f\n", block, trace, max);
}


/**********************************************************
 *   int -  display data                                  *
 **********************************************************/
void show_int_data (void *values, long size)
{
    int   i, line_elements;
    int   *ptr;

    ptr = (int *)values;
    line_elements =10;
    
    for (i= 0; i<size; i++)
       {
       if (i%line_elements ==0)
          printf("\n");
       printf("%i\t",*ptr);
       ptr++;
       } 
 
}

/**********************************************************
 *   short -  display data                                  *
 **********************************************************/
void show_short_data (void *values, long size)
{
    int    i, line_elements;
    short  *ptr;

    ptr = (short *)values;
    line_elements =10;
    
    for (i= 0; i<size; i++)
       {
       if (i%line_elements ==0)
          printf("\n");
       printf("%i\t",*ptr);
       ptr++;
       } 
 
}

/**********************************************************
 *   short -  display data                                  *
 **********************************************************/
void show_float_data (void *values, long size)
{
    int    i, line_elements;
    float  *ptr;

    ptr = (float *)values;
    line_elements =10;
    
    for (i= 0; i<size; i++)
       {
       if (i%line_elements ==0)
          printf("\n");
       printf("%f\t",*ptr);
       ptr++;
       } 
 
}


/**********************************************************
 *   int -  display hypercompled header status            *
 **********************************************************/
float find_absmax_int (void *values, long size)
{
    int i;
    float max_val;
    int   *ptr;
 
    ptr = (int *)values;
    /* initialize value */
    max_val = 0;
    
    /* find absolute maximum */
    for (i=0; i <size ; i++)
	{
        if (max_val < abs(*ptr) )
	    {
	     max_val = (float)abs(*ptr);
	     }
	
	ptr++;
	}
    /* return value */	
    return(max_val);
}

/**********************************************************
 *   short -   display hypercompled header status         *
 **********************************************************/
float find_absmax_short (void *values, long size)
{
    int i;
    float max_val;
    short   *ptr;
 
    ptr = (short *)values;
    /* initialize value */
    max_val = 0;
    
    /* find absolute maximum */
    for (i=0; i <size ; i++)
	{
        if (max_val < abs(*ptr) )
	    {
	     max_val = (float)abs(*ptr);
	     }
	
	ptr++;
	}
    /* return value */	
    return(max_val);
}

/**********************************************************
 *   float -   display hypercompled header status         *
 **********************************************************/
float find_absmax_float (void *values, long size)
{
    int i;
    float max_val;
    float   *ptr;
 
    ptr = (float *)values;
    /* initialize value */
    max_val = 0;
    
    /* find absolute maximum */
    for (i=0; i <size ; i++)
	{
        if (max_val < abs(*ptr) )
	    {
	     max_val = (float)abs(*ptr);
	     }
	
	ptr++;
	}
    /* return value */	
    return(max_val);
}

/**********************************************************
 *   assign action status                                 *
 **********************************************************/
void assign_status ( char status)
{
    if (status & 0x1) 
       action.header =1;
    if ((status >> 1) & 0x1) 
       action.header_status =1;
    if ((status >> 2) & 0x1) 
       action.blockheader =1;
    if ((status >> 3) & 0x1) 
       action.blockheader_status =1;
    if ((status >> 4) & 0x1) 
       action.complex_blockheader =1;
    if ((status >> 5) & 0x1) 
       action.complex_status =1;
    if ((status >> 6) & 0x1) 
       action.data =1;
    if ((status >> 7) & 0x1) 
       action.max =1;
}



/**********************************************************
 *      M A I N                                           *
 **********************************************************/

int main (int argc, char *argv[])
{
    /*-------------------*/
    /*  Define variables */
    /*-------------------*/    
    char    file[MAXSTR];        /* data file */
    char    *filename, *path;    /* filename and path */
    char    *outfile;            /* default output file */
    int     counter;             /* counter for looping (blocks)*/
    int     traces;              /* counter for looping (traces)*/
    void    *temp;               /* pointer to data*/
    void    *cmpres;             /* result of string comparision */
    float   max_value;           /* maximum data value */
    char    data_format;         /* data format 0=int_16, 1=int_32, 3=float*/
    long    data_size;	         /* size of data returned */
    long    read_status;         /* status of file read */
    long    write_status;        /* status of file write */
    char    arg_status;          /* status of arguments passed into program */
    char    *args[MAXOPT] ={"dh", "ds", "bh", "bs", "ch", "cs", "data", "max"};
    char    output;              /* flag containing output action [0] = display, [1] write to file */
    FILE   *fp, *fpout;
    
    int i;
    /*-----------------------*/
    /*  Initialize variables */
    /*-----------------------*/    
    path       = "/space/vnmr1/vnmrsys/mxm_code/";
    filename   = "fid";
    outfile    = "rawdata";
    arg_status = 0;
    output     = 0;
     
    /*------------------------------*/
    /* check number of args         */    
    /*------------------------------*/    
    if ((argc > MAXARG)  || (argc < 3))
       {
       printf("Wrong number of arguments ! \n");
       show_syntax();
       exit(0);
       }

    /*--------------------------*/
    /* check 1st argument value */
    /*--------------------------*/    
    for (counter =0; counter < MAXOPT; counter++)
       {
       cmpres = strstr(argv[1],args[counter]);
       if (cmpres != NULL)
	  {
           arg_status = arg_status + pow(2,counter);
	  }
       }	

    if (arg_status == 0)
	{
	printf("Invalid arguement   !\n");
	show_syntax();
	exit(0);
	}

    /* set argument flags for 1st argument*/	 
    assign_status(arg_status);

    /*---------------------*/
    /* check 2nd argument  */
    /*---------------------*/    
    if (argc > 2)
        {
	if (((cmpres = strstr(argv[2],"disp")) != NULL)  || ((cmpres = strstr(argv[2],"file")) != NULL))
            {
	    if ((cmpres = strstr(argv[2],"file")) != NULL)
	         {
		 output = 1;
		 }                               
	    }
	else
           {
	   printf("Invalid 2nd arguement   !\n");
           show_syntax();	   
	   exit(0);
	   }	
	} 

    /*---------------------------------*/
    /* check 3rd argument (input file) */
    /*---------------------------------*/
    if (argc >3)
        {
	filename = argv[3];
	strcpy(file,filename);
	}  
    else
        {
	strcpy(file,path);
        strcat(file,filename);
	}
    /*----------------------------------*/
    /* check 4th argument (output file) */
    /*----------------------------------*/
    if (argc > 4)
        {
	outfile = argv[4];
	}    

    /*--------------------------------------------------------------------*/
    /* open file for output                                               */
    /*--------------------------------------------------------------------*/
   if (output)
       {
        if ((fpout = fopen(outfile,"a")) == NULL)
	      {
	      printf("Error opening file <%s>\n",file);
	      exit(0);
	      }
       }

   
   /*---------------------------------------------------------------------*/
   /* open data file                                                      */
   /*---------------------------------------------------------------------*/
   if ((fp = fopen(file,"rb")) == NULL)
       printf("Error opening file <%s>\n",file);
   else
      {	
      /*-------------------*/
      /*   read header     */
      /*-------------------*/      
      if ((read_status=fread(&header, sizeof(header), 1, fp)) == 0)
	 {
	 printf("Error reading data header !\n");
	 exit(0);
	 }
      /*  show header */
      if ((action.header) && (!output))
 	   show_header(header);
      /*  show header status */		
      if ((action.header_status) && (!output))
	   show_header_status(header.status);
      /*  write header  */		
      if ((output) && (action.header))
	   write_header(header, fpout);
      /*  write header status */		
      if ((output) && (action.header_status))
	   write_header_status(header.status, fpout);

      /* loop for number of blocks in file */
      for (counter = 1; counter <= header.nblocks; counter++)
	 {
	 /*------------------------*/
	 /* read datablock header  */
	 /*------------------------*/	 
	 if ((read_status=fread(&dbheader, sizeof(dbheader), 1,fp)) == 0)
	     {
	     printf("Error reading block header !\n");
	     exit(0);
	     }
	 /* show block header */
	 if ((action.blockheader) && (!output))
              show_datablock_header(dbheader, counter);
	 /* show block header status */	   
	 if ((action.blockheader_status) && (!output))
              show_datablock_status(dbheader.status,counter);
         /* write datablock header */
	 if ((output) && (action.blockheader) )
	      write_datablock_header(dbheader, fpout);
         /* write datablock header status*/	      
  	 if ((output) && (action.blockheader_status))
              write_datablock_status(dbheader.status, fpout);
	      
         /*--------------------------------*/
	 /* read complex datablock header  */
         /*--------------------------------*/	 
         if (header.nbheaders > 1)
             {
	     if ((read_status=fread(&hcompheader, sizeof(hcompheader), 1,fp)) == 0)
	         {
        	 printf("Error reading hypercomplex block header !\n");
        	 exit(0);
	         }	  
             /* show hpercomplex block */
	     if ((action.complex_blockheader) && (!output))
		  show_complex_header(hcompheader,dbheader.index);
	     /* show hpercomplex block status */     
	     if ((action.complex_status) && (!output))
                  show_complex_status(hcompheader.status,dbheader.index);
             /* write hpercomplex block */
	     if ((output) && (action.complex_blockheader))
		  write_complex_header(hcompheader,dbheader.index, fpout);
	     /* write hpercomplex block status */     
	     if ((output) && (action.complex_status))
                  write_complex_status(hcompheader.status,dbheader.index, fpout);
	     }

         for (traces = 1; traces <= header.ntraces; traces++)
	      { 
	      /* allocate memory for data */
	      if (( temp =malloc(header.tbytes)) == NULL)
        	   printf("Error allocating memory !\n");
	      else
		  { 
		  /*---------------*/
		  /* read data     */
		  /*---------------*/
        	  if ((data_size=fread(temp,header.ebytes,header.np,fp)) == 0)
	              {
        	      printf("Error reading data !\n");
        	      exit(0);
	              }
		  /* evaluate status bit for format */
        	  data_format = header.status >> 2 &0x3;

      		  /* show data */
		  if (action.data)
		       {
		       printf("SHOW DATA :\n");
		       switch (data_format)
	        	  {
			  case   0: show_short_data(temp, header.np);
		        	    break;
			  case   1: show_int_data(temp, header.np);
	  	        	    break;
			  case   2: show_float_data(temp, header.np);
		        	    break;
			  default : printf("Invalid data format or file status !\n");
	        	  }
		       }  
		  /* write data to file */
		  if ((output) && (action.data))
	               {
		       if ((write_status=fwrite(temp,header.ebytes,header.np,fpout)) == 0)
	        	  {
        		  printf("Error writting data !\n");
        		  exit(0);
                	  }
		      }
      		  /*---------------*/
		  /* find max.     */
		  /*---------------*/
        	  if (action.max)
		       {
		       /* extract max value */               
		       switch (data_format)
			  {
			  case   0: max_value=find_absmax_short(temp,header.np);
				    break;
			  case   1: max_value=find_absmax_int(temp,header.np);
				    break;		  
			  case   2: max_value=find_absmax_float(temp,header.np);
				    break;		  
			  default : printf("Invalid data format or file status !\n");
        		  }
                	if (output)
			    {
			    write_max(max_value, header.nblocks, dbheader.index, traces, fpout);
			    }
                	else
			    {
			    printf("MAX VALUE %f\n",max_value);
			    }
	        	}           /* --- NED action.max --- */
        	   /* Free memory  */
        	   free(temp);	       
        	   }               /* ---- END MEMORY ALLOCATION ----*/
            }                      /* ---- END COUNTER (TRACES) ---- */
      }                            /* ---- END COUNTER (BLOCK) ---- */
      /* Close input (data) file    */
      fclose(fp);	
      
      /* Close output file */
      if (output) 
         fclose(fpout);
   }               /* ---- END OPEN FILE ----*/
exit(0);
}                  /*  ---- END MAIN */

