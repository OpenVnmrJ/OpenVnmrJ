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
/*                  from file                                 */
/*                                                                */
/*                                                                */
/*    read_raw_data <arg1> <arg2>                                 */
/*                                                                */
/*                  <arg1>  - dh (data header),                   */
/*                            ds (data status),                   */     
/*                            bh (block header),                  */
/*                            bs (block status)                   */
/*                            ch (hypercomplex header)            */
/*                            cs (hpercomplex status)             */
/*                            data (rawdata)                      */
/*                            max (maximum of each fid)           */
/*                  <arg2>  - input filename                      */
/*                                                                */
/*    If the compile option OUTPUT is defined, then the syntax    */
/*    is as follows                                               */
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
#include <arpa/inet.h>

#define  MAXSTR  256
#define  MAXARG  5
#define  MAXOPT  8

struct HEADER
	  {
          int	 nblocks;              /* number of blocks in file */
	  int	 ntraces;              /* number of traces in file */
	  int	 np;                   /* number of elements pre trace */
	  int 	 ebytes;               /* number of bytes per element */
	  int	 tbytes;               /* number of bytes per trace */
	  int	 bbytes;               /* number of bytes per block */
	  short  vers_id;              /* software version, file_di status bits */
	  short	 status;               /* status of whole file */
          int	 nbheaders;            /* number of block headers per block */
	   } header;


struct DATABLOCKHEAD
	   {
	   short   scale;	 	/* scaling factor */
	   short   status;		/* status of data in block */
	   short   index;               /* block index */
	   short   mode;                /* mode of data in block */
	   int    ctcount;             /* ct value for FID */
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
	   int    l_spare1;            /* int word:  spare */
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
     printf("Blocks in file:          %u\n",htonl(header.nblocks));
     printf("Traces per block:        %u\n",htonl(header.ntraces));
     printf("Elements per trace:      %u\n",htonl(header.np));
     printf("Bytes per element:       %u\n",htonl(header.ebytes));
     printf("Bytes per trace:         %u\n",htonl(header.tbytes));
     printf("Bytes per block:         %u\n",htonl(header.bbytes));
     printf("Software Version:        %i\n",htons(header.vers_id));
     printf("Status:                  %x[hex]\n",htons(header.status));
     printf("Block headers per block: %u\n",htonl(header.nbheaders));
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
     printf("Bit 0:  %x\tNo data    / Data\n",status & 0x01);   
     printf("Bit 1:  %x\tFID        / Spectrum\n",status >> 1 & 0x01);
     printf("Bit 2:  %x\t16-bit     / 32-bit integer (if Bit 3  =0 otherwise ignored)\n",(status >> 2) & 0x01 );
     printf("Bit 3:  %x\tInteger    / Float\n",(status >> 3) & 0x01 );   
     printf("Bit 4:  %x\tReal       / Complex\n",(status >> 4) & 0x01 );
     printf("Bit 5:  %x\t -         / Hypercomplex\n",(status >> 5) & 0x01 );          
     printf("Bit 6:  - \tNot used\n");           
     printf("Bit 7:  %x\tnot Acqpar / Acqpar\n",(status >> 6) & 0x01);     
     printf("Bit 8:  %x\t1st FFT    / 2nd FFT\n",(status >> 7)  & 0x01);     
     printf("Bit 9:  %x\tRegular    / Transposed\n",(status >> 8) & 0x01);     
     printf("Bit 10: - \tNot used\n");                
     printf("Bit 11: %x\t -         / np dimension active\n",(status >> 9) & 0x01);     
     printf("Bit 12: %x\t -         / nf dimension active\n",(status >> 10) & 0x01);          	  
     printf("Bit 13: %x\t -         / ni dimension active\n",(status >> 11) & 0x01);     
     printf("Bit 14: %x\t -         / ni2 dimension active\n",(status >> 12) & 0x01);          
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
     printf("Scaling factor:             %i\n",htons(dbheader.scale));
     printf("Status of data in block:    %x[hex]\n",htons(dbheader.status));
     printf("Block Index:                %i\n",htons(dbheader.index));
     printf("Mode of data in block:      %i\n",htons(dbheader.mode));
     printf("ct value of FID:            %u\n",htonl(dbheader.ctcount));
     printf("Left phase in phase file:   %f\n",(double) htonl(dbheader.lpval));
     printf("Right phase in phase file:  %f\n",(double) htonl(dbheader.rpval));
     printf("Left drift correction:      %f\n",(double) htonl(dbheader.lvl));
     printf("Tilt drift correction:      %f\n",(double) htonl(dbheader.tlt));
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
     printf("Bit 0:  %x\tNo data   / Data\n",status & 0x1);   
     printf("Bit 1:  %x\tFID       / Spectrum\n",(status >> 1) & 0x01);
     printf("Bit 2:  %i\t16-bit    / 32-bit integer (if Bit 3  =0 otherwise ignored)\n",(status >> 2)  & 0x01);
     printf("Bit 3:  %x\tInteger   / Float\n",(status >> 3) & 0x01);   
     printf("Bit 4:  %x\tReal      / Complex\n",(status >> 4) & 0x01);
     printf("Bit 5:  %x\t -        / Hypercomplex\n",(status >> 5) & 0x01);          
     printf("Bit 6:  - \tNot used\n");           
     printf("Bit 7:  %x\tmore blocks: absent / present\n",(status >> 7) & 0x01);     
     printf("Bit 8:  %x\tnp real   / np complex\n",(status >> 8)& 0x010);     
     printf("Bit 9:  %x\tnf real   / nf complex\n",(status >> 9) & 0x01);     
     printf("Bit 10: - \tNot used\n");                
     printf("Bit 11: %x\tni real   / ni complex\n",(status >> 11) & 0x01);     
     printf("Bit 12: %x\tni2 real  / ni2 complex\n",(status >> 12) & 0x01);          	  
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
     printf("Spare 1 (short):            %i\n",htons(hcompheader.s_spare1));
     printf("Status:                :    %x[hex]\n",htons(hcompheader.status));
     printf("Spare 2 (short):            %i\n",htons(hcompheader.s_spare2));
     printf("Spare 3 (short):            %i\n",htons(hcompheader.s_spare3));
     printf("Spare 4 (int):              %d\n",htonl(hcompheader.l_spare1));
     printf("2D-f2:left phase:           %f\n",(double) htonl(hcompheader.lpval1));
     printf("2D-f2 rigth phase:          %f\n",(double) htonl(hcompheader.rpval1));
     printf("Spare 5 (float):            %f\n",(double) htonl(hcompheader.f_spare1));
     printf("Spare 6 (float):            %f\n",(double) htonl(hcompheader.f_spare2));
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
     printf("Bit 0:  %x\t- / np: ph mode\n",status & 0x1);   
     printf("Bit 1:  %x\t- / np: av mode\n",(status >>1) & 0x01);
     printf("Bit 2:  %x\t- / np: pwr mode\n",(status >>2) & 0x01);
     printf("Bit 3:  - \tNot used\n");           
     printf("Bit 4:  %x\t- / nf: ph mode   (currently not used)\n",(status >> 4) & 0x01);   
     printf("Bit 5:  %x\t- / nf: av mode   (currently not used)\n",(status >> 5) & 0x01);
     printf("Bit 6:  %x\t- / nf: pwr mode  (currently not used)\n",(status >> 6) & 0x01);
     printf("Bit 7:  - \tNot used\n");           
     printf("Bit 8:  %x\t- / ni: ph mode   (currently not used)\n",(status >> 8) & 0x01);   
     printf("Bit 9:  %x\t- / ni: av mode   (currently not used)\n",(status >> 9) & 0x01);
     printf("Bit 10: %x\t- / ni: pwr mode  (currently not used)\n",(status >> 10) & 0x01);
     printf("Bit 11:  - \tNot used\n");           
     printf("Bit 12: %x\t- / ni2: ph mode  (currently not used)\n",(status >> 12) & 0x01);   
     printf("Bit 13: %x\t- / ni2: av mode  (currently not used)\n",(status >> 13) & 0x01);
     printf("Bit 14: %x\t- / ni2: pwr mode (currently not used)\n",(status >> 14) & 0x01);
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
     fprintf(fpout,"Blocks in file:          %u\n",htonl(header.nblocks));
     fprintf(fpout,"Traces per block:        %u\n",htonl(header.ntraces));
     fprintf(fpout,"Elements per trace:      %u\n",htonl(header.np));
     fprintf(fpout,"Bytes per element:       %u\n",htonl(header.ebytes));
     fprintf(fpout,"Bytes per trace:         %u\n",htonl(header.tbytes));
     fprintf(fpout,"Bytes per block:         %u\n",htonl(header.bbytes));
     fprintf(fpout,"Software Version:        %i\n",htons(header.vers_id));
     fprintf(fpout,"Status:                  %x[hex]\n",htons(header.status));
     fprintf(fpout,"Block headers per block: %u\n",htonl(header.nbheaders));
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
     fprintf(fpout,"Bit 0:  %x\tNo data    / Data\n",status & 0x01);   
     fprintf(fpout,"Bit 1:  %x\tFID        / Spectrum\n",status >> 1 & 0x01);
     fprintf(fpout,"Bit 2:  %x\t16-bit     / 32-bit integer (if Bit 3  =0 otherwise ignored)\n",(status >> 2) & 0x01 );
     fprintf(fpout,"Bit 3:  %x\tInteger    / Float\n",(status >> 3) & 0x01 );   
     fprintf(fpout,"Bit 4:  %x\tReal       / Complex\n",(status >> 4) & 0x01 );
     fprintf(fpout,"Bit 5:  %x\t -         / Hypercomplex\n",(status >> 5) & 0x01 );          
     fprintf(fpout,"Bit 6:  - \tNot used\n");           
     fprintf(fpout,"Bit 7:  %x\tnot Acqpar / Acqpar\n",(status >> 6) & 0x01);     
     fprintf(fpout,"Bit 8:  %x\t1st FFT    / 2nd FFT\n",(status >> 7)  & 0x01);     
     fprintf(fpout,"Bit 9:  %x\tRegular    / Transposed\n",(status >> 8) & 0x01);     
     fprintf(fpout,"Bit 10: - \tNot used\n");                
     fprintf(fpout,"Bit 11: %x\t -         / np dimension active\n",(status >> 9) & 0x01);     
     fprintf(fpout,"Bit 12: %x\t -         / nf dimension active\n",(status >> 10) & 0x01);          	  
     fprintf(fpout,"Bit 13: %x\t -         / ni dimension active\n",(status >> 11) & 0x01);     
     fprintf(fpout,"Bit 14: %x\t -         / ni2 dimension active\n",(status >> 12) & 0x01);          
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
     fprintf(fpout,"Scaling factor:             %i\n",htons(dbheader.scale));
     fprintf(fpout,"Status of data in block:    %x[hex]\n",htons(dbheader.status));
     fprintf(fpout,"Block Index:                %i\n",htons(dbheader.index));
     fprintf(fpout,"Mode of data in block:      %i\n",htons(dbheader.mode));
     fprintf(fpout,"ct value of FID:            %u\n",htonl(dbheader.ctcount));
     fprintf(fpout,"Left phase in phase file:   %f\n",(double) htonl(dbheader.lpval));
     fprintf(fpout,"Right phase in phase file:  %f\n",(double) htonl(dbheader.rpval));
     fprintf(fpout,"Left drift correction:      %f\n",(double) htonl(dbheader.lvl));
     fprintf(fpout,"Tilt drift correction:      %f\n",(double) htonl(dbheader.tlt));
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
     fprintf(fpout,"Bit 0:  %x\tNo data   / Data\n",status & 0x1);   
     fprintf(fpout,"Bit 1:  %x\tFID       / Spectrum\n",(status >> 1) & 0x01);
     fprintf(fpout,"Bit 2:  %i\t16-bit    / 32-bit integer (if Bit 3  =0 otherwise ignored)\n",(status >> 2)  & 0x01);
     fprintf(fpout,"Bit 3:  %x\tInteger   / Float\n",(status >> 3) & 0x01);   
     fprintf(fpout,"Bit 4:  %x\tReal      / Complex\n",(status >> 4) & 0x01);
     fprintf(fpout,"Bit 5:  %x\t -        / Hypercomplex\n",(status >> 5) & 0x01);          
     fprintf(fpout,"Bit 6:  - \tNot used\n");           
     fprintf(fpout,"Bit 7:  %x\tmore blocks: absent / present\n",(status >> 7) & 0x01);     
     fprintf(fpout,"Bit 8:  %x\tnp real   / np complex\n",(status >> 8)& 0x010);     
     fprintf(fpout,"Bit 9:  %x\tnf real   / nf complex\n",(status >> 9) & 0x01);     
     fprintf(fpout,"Bit 10: - \tNot used\n");                
     fprintf(fpout,"Bit 11: %x\tni real   / ni complex\n",(status >> 11) & 0x01);     
     fprintf(fpout,"Bit 12: %x\tni2 real  / ni2 complex\n",(status >> 12) & 0x01);          	  
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
     fprintf(fpout,"Spare 1 (short):            %i\n",htons(hcompheader.s_spare1));
     fprintf(fpout,"Status:                :    %x[hex]\n",htons(hcompheader.status));
     fprintf(fpout,"Spare 2 (short):            %i\n",htons(hcompheader.s_spare2));
     fprintf(fpout,"Spare 3 (short):            %i\n",htons(hcompheader.s_spare3));
     fprintf(fpout,"Spare 4 (int):              %d\n",htonl(hcompheader.l_spare1));
     fprintf(fpout,"2D-f2:left phase:           %f\n",(double) htonl(hcompheader.lpval1));
     fprintf(fpout,"2D-f2 rigth phase:          %f\n",(double) htonl(hcompheader.rpval1));
     fprintf(fpout,"Spare 5 (float):            %f\n",(double) htonl(hcompheader.f_spare1));
     fprintf(fpout,"Spare 6 (float):            %f\n",(double) htonl(hcompheader.f_spare2));
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
     fprintf(fpout,"Bit 0:  %x\t- / np: ph mode\n",status & 0x1);   
     fprintf(fpout,"Bit 1:  %x\t- / np: av mode\n",(status >>1) & 0x01);
     fprintf(fpout,"Bit 2:  %x\t- / np: pwr mode\n",(status >>2) & 0x01);
     fprintf(fpout,"Bit 3:  - \tNot used\n");           
     fprintf(fpout,"Bit 4:  %x\t- / nf: ph mode   (currently not used)\n",(status >> 4) & 0x01);   
     fprintf(fpout,"Bit 5:  %x\t- / nf: av mode   (currently not used)\n",(status >> 5) & 0x01);
     fprintf(fpout,"Bit 6:  %x\t- / nf: pwr mode  (currently not used)\n",(status >> 6) & 0x01);
     fprintf(fpout,"Bit 7:  - \tNot used\n");           
     fprintf(fpout,"Bit 8:  %x\t- / ni: ph mode   (currently not used)\n",(status >> 8) & 0x01);   
     fprintf(fpout,"Bit 9:  %x\t- / ni: av mode   (currently not used)\n",(status >> 9) & 0x01);
     fprintf(fpout,"Bit 10: %x\t- / ni: pwr mode  (currently not used)\n",(status >> 10) & 0x01);
     fprintf(fpout,"Bit 11:  - \tNot used\n");           
     fprintf(fpout,"Bit 12: %x\t- / ni2: ph mode  (currently not used)\n",(status >> 12) & 0x01);   
     fprintf(fpout,"Bit 13: %x\t- / ni2: av mode  (currently not used)\n",(status >> 13) & 0x01);
     fprintf(fpout,"Bit 14: %x\t- / ni2: pwr mode (currently not used)\n",(status >> 14) & 0x01);
     fprintf(fpout,"Bit 15:  - \tNot used\n");           
     fprintf(fpout,"===================================\n");

}
/**********************************************************
 *   show_syntax                                          *
 **********************************************************/
void show_syntax (void)
{
printf("\nread_raw_data will display the content of data files.\n");
printf("\nSYNTAX :\n");
#ifdef OUTPUT
printf("read_raw_data <arg1> <arg2> <arg3> <arg4>\n\n");
printf("              <arg1> = dh | ds | bh | bs | ch | cs | data | max \n");
printf("              <arg2> = disp | file\n");
printf("              <arg3> = source filename (default = fid) \n");
printf("              <arg4> = target filename (default = rawdata) \n\n");
#else
printf("read_raw_data <arg1> <arg2>\n");
printf("              <arg1> = dh | ds | bh | bs | ch | cs | data | max \n");
printf("              <arg2> = filename\n");
printf("For example:\n");
printf("read_raw_data \"dh ds max\" /home/vnmr1/vnmrsys/exp1/datdir/data\n");
printf("read_raw_data \"dh ds bh bs\" /home/vnmr1/vnmrsys/exp1/datdir/data\n");
#endif
printf("\n");
printf("Where: dh - data header\n");
printf("       ds - data status\n");
printf("       bh - block header\n");
printf("       bs - block status\n");
printf("       ch - hypercomplex header\n");
printf("       cs - hypercomplex status\n");
printf("       data - data in block\n");
printf("       max  - maximum of data in block\n");
printf("\n");
}


/**********************************************************
 *   Write MAX:                                           *
 **********************************************************/
void write_max (float max, int block_no, int block, int trace, FILE *fpout)
{
      if ((block == 1) && (trace ==0))
          {
	  fprintf(fpout,"\nMAXIMA(S) :\n");
	  fprintf(fpout,"=====================\n");
	  }
     fprintf(fpout,"BLOCK %d\t Trace %d ::  %f\n", block, trace, max);
}

float htonf(float val)
{
   uint32_t rep;
   memcpy(&rep, &val, sizeof rep);
   rep = htonl(rep);
   memcpy(&val, &rep, sizeof rep);
   return(val);
}

/**********************************************************
 *   int -  display data                                  *
 **********************************************************/
void show_int_data (void *values, int size)
{
    int   i, line_elements;
    int   *ptr;

    ptr = (int *)values;
    line_elements =10;
    
    for (i= 0; i<size; i++)
       {
       if (i%line_elements ==0)
          printf("\n");
       printf("%i ", htonl(*ptr));
       ptr++;
       } 
 
}

/**********************************************************
 *   short -  display data                                  *
 **********************************************************/
void show_short_data (void *values, int size)
{
    int    i, line_elements;
    short  *ptr;

    ptr = (short *)values;
    line_elements =10;
    
    for (i= 0; i<size; i++)
       {
       if (i%line_elements ==0)
          printf("\n");
       printf("%i ",htons(*ptr));
       ptr++;
       } 
 
}

/**********************************************************
 *   short -  display data                                  *
 **********************************************************/
void show_float_data (void *values, int size)
{
    int    i, line_elements;
    float  *ptr;
    float tmp;

    ptr = (float *)values;
    line_elements =10;
    
    for (i= 0; i<size; i++)
       {
       tmp = htonf( *ptr );
       if (i%line_elements ==0)
          printf("\n");
       printf("%f ",tmp);
       ptr++;
       } 
    printf("\n");
 
}


/**********************************************************
 *   int -  display hypercompled header status            *
 **********************************************************/
float find_absmax_int (void *values, int size)
{
    int i;
    float max_val;
    int   *ptr;
    int val;
 
    ptr = (int *)values;
    /* initialize value */
    max_val = 0;
    
    /* find absolute maximum */
    for (i=0; i <size ; i++)
	{
        val = htonl( *ptr);
        if (max_val < abs(val) )
	    {
	     max_val = (float)abs(val);
	     }
	
	ptr++;
	}
    /* return value */	
    return(max_val);
}

/**********************************************************
 *   short -   display hypercompled header status         *
 **********************************************************/
float find_absmax_short (void *values, int size)
{
    int i;
    float max_val;
    short   *ptr;
    int val;
 
    ptr = (short *)values;
    /* initialize value */
    max_val = 0;
    
    /* find absolute maximum */
    for (i=0; i <size ; i++)
	{
        val = (int) htons( *ptr );
        if (max_val < (float) abs(val) )
	    {
	     max_val = (float)abs(val);
	     }
	
	ptr++;
	}
    /* return value */	
    return(max_val);
}

/**********************************************************
 *   float -   display hypercompled header status         *
 **********************************************************/
float find_absmax_float (void *values, int size)
{
    int i;
    float max_val;
    float   *ptr;
    float val;
 
    ptr = (float *)values;
    /* initialize value */
    max_val = 0;
    
    /* find absolute maximum */
    for (i=0; i <size ; i++)
	{
        val = htonf( *ptr );
        if (max_val < fabs(val) )
	    {
	     max_val = fabs(val);
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
#ifdef OUTPUT
    char    *filename, *path;    /* filename and path */
    char    *outfile;            /* default output file */
#endif
    int     counter;             /* counter for looping (blocks)*/
    int     traces;              /* counter for looping (traces)*/
    void    *temp;               /* pointer to data*/
    void    *cmpres;             /* result of string comparision */
    float   max_value;           /* maximum data value */
    char    data_format;         /* data format 0=int_16, 1=int_32, 3=float*/
    int     data_size;	         /* size of data returned */
    size_t  read_status;         /* status of file read */
    size_t  write_status;        /* status of file write */
    char    arg_status;          /* status of arguments passed into program */
    char    *args[MAXOPT] ={"dh", "ds", "bh", "bs", "ch", "cs", "data", "max"};
    char    output;              /* flag containing output action [0] = display, [1] write to file */
    FILE   *fp;
    FILE   *fpout = NULL;
    
    /*-----------------------*/
    /*  Initialize variables */
    /*-----------------------*/    
#ifdef OUTPUT
    path       = "/home/vnmr1/vnmrsys/";
    filename   = "fid";
    outfile    = "rawdata";
#endif
    arg_status = 0;
    output     = 0;
     
    /*------------------------------*/
    /* check number of args         */    
    /*------------------------------*/    
    if ((argc > MAXARG)  || (argc < 3))
       {
       if (argc > 1)
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

#ifdef OUTPUT
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
#else
   strcpy(file,argv[2]);
#endif

   
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
	   show_header_status(htons(header.status));
      /*  write header  */		
      if ((output) && (action.header))
	   write_header(header, fpout);
      /*  write header status */		
      if ((output) && (action.header_status))
	   write_header_status(htons(header.status), fpout);

      /* loop for number of blocks in file */
      for (counter = 1; counter <= htonl(header.nblocks); counter++)
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
              show_datablock_status(htons(dbheader.status),counter);
         /* write datablock header */
	 if ((output) && (action.blockheader) )
	      write_datablock_header(dbheader, fpout);
         /* write datablock header status*/	      
  	 if ((output) && (action.blockheader_status))
              write_datablock_status(htons(dbheader.status), fpout);
	      
         /*--------------------------------*/
	 /* read complex datablock header  */
         /*--------------------------------*/	 
         if (htonl(header.nbheaders) > 1)
             {
	     if ((read_status=fread(&hcompheader, sizeof(hcompheader), 1,fp)) == 0)
	         {
        	 printf("Error reading hypercomplex block header !\n");
        	 exit(0);
	         }	  
             /* show hpercomplex block */
	     if ((action.complex_blockheader) && (!output))
		  show_complex_header(hcompheader,htonl(dbheader.index));
	     /* show hpercomplex block status */     
	     if ((action.complex_status) && (!output))
                  show_complex_status(htons(hcompheader.status),htonl(dbheader.index));
             /* write hpercomplex block */
	     if ((output) && (action.complex_blockheader))
		  write_complex_header(hcompheader,htonl(dbheader.index), fpout);
	     /* write hpercomplex block status */     
	     if ((output) && (action.complex_status))
                  write_complex_status(htons(hcompheader.status),htonl(dbheader.index), fpout);
	     }

         for (traces = 1; traces <= htonl(header.ntraces); traces++)
	      { 
	      /* allocate memory for data */
	      if (( temp =malloc(htonl(header.tbytes))) == NULL)
        	   printf("Error allocating memory !\n");
	      else
		  { 
		  /*---------------*/
		  /* read data     */
		  /*---------------*/
        	  if ((data_size=fread(temp,htonl(header.ebytes),htonl(header.np),fp)) == 0)
	              {
        	      printf("Error reading data !\n");
        	      exit(0);
	              }
		  /* evaluate status bit for format */
        	  data_format = htons(header.status) >> 2 &0x3;

      		  /* show data */
		  if (action.data)
		       {
		       printf("SHOW DATA :\n");
		       switch (data_format)
	        	  {
			  case   0: show_short_data(temp, htonl(header.np));
		        	    break;
			  case   1: show_int_data(temp, htonl(header.np));
	  	        	    break;
			  case   2: show_float_data(temp, htonl(header.np));
		        	    break;
			  default : printf("Invalid data format or file status !\n");
	        	  }
		       }  
		  /* write data to file */
		  if ((output) && (action.data))
	               {
		       if ((write_status=fwrite(temp,htonl(header.ebytes),htonl(header.np),fpout)) == 0)
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
			  case   0: max_value=find_absmax_short(temp,htonl(header.np));
				    break;
			  case   1: max_value=find_absmax_int(temp,htonl(header.np));
				    break;		  
			  case   2: max_value=find_absmax_float(temp,htonl(header.np));
				    break;		  
			  default : printf("Invalid data format or file status !\n");
        		  }
                	if (output)
			    {
			    write_max(max_value, htonl(header.nblocks), htonl(dbheader.index), traces, fpout);
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

