/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */
#include <sys/file.h>
#include <sys/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

/*---- Include endian conversion handler ----*/
#include "byteswap_linux.h"

/*--------------------------------------------------*/
/*---compiling for little endian systems (Linux) ---*/
/*--------------------------------------------------*/
#define	LINUX	1


struct datafilehead main_header;
struct datablockhead block_header;


float dphi(float x1,float x2,float y1,float y2)
    {
    float iph;
    iph = (float)(5729.57795e-6*atan2((x1*y2-x2*y1),(x1*x2+y1*y2)));
    return iph ;
    } 
    /*dphi*/

void getphase(int si, int idx, int wd2, float *wbuff1, float *wbuff, int ni, int nf)
{
  int j;
  float n11, n12, n21, n22, m1, m2;
  
  n11=0.0;n12=0.0;
  n21=0.0;n22=0.0;
  for (j=0; j<si/16; j++)
  {
    n11 += wbuff[ni*si+2*j]*wbuff[ni*si+2*j]+wbuff[ni*si+2*j+1]*wbuff[ni*si+2*j+1];
    n12 += wbuff[nf*si+2*j]*wbuff[nf*si+2*j]+wbuff[nf*si+2*j+1]*wbuff[nf*si+2*j+1];
  }
  for (j=7*si/16; j<si/2; j++)
  {
    n21 += wbuff[ni*si+2*j]*wbuff[ni*si+2*j]+wbuff[ni*si+2*j+1]*wbuff[ni*si+2*j+1];
    n22 += wbuff[nf*si+2*j]*wbuff[nf*si+2*j]+wbuff[nf*si+2*j+1]*wbuff[nf*si+2*j+1];
  }
  if (n12>n11)
  {
    n12 = n11;
  }
  else
  {
    n11 = n12;
  }
  if (n22>n21)
  {
    n22 = n21;
  }
  else
  {
    n21 = n22;
  }
  if (n22>n11)
  {
    n22 = n11;
  }
  else
  {
    n11 = n22;
  }
  n11 = n11*16/si;
/*end determination of RMS noise level */
  for (j=si/4-wd2-1; j<si/4+wd2+1; j++)
  {
    wbuff1[idx*si+2*j] = dphi(wbuff[ni*si+2*j],wbuff[nf*si+2*j],wbuff[ni*si+2*j+1],wbuff[nf*si+2*j+1]);
    m1 = wbuff[ni*si+2*j]*wbuff[ni*si+2*j]+wbuff[ni*si+2*j+1]*wbuff[ni*si+2*j+1];
    m2 = wbuff[nf*si+2*j]*wbuff[nf*si+2*j]+wbuff[nf*si+2*j+1]*wbuff[nf*si+2*j+1];
    if ((m1>=100*n11) && (m2>=100*n11))
    {
      wbuff1[idx*si+2*j+1] = 1/m2+1/m1;
    }
    else
    {
      wbuff1[idx*si+2*j+1] = 0.0;
    }
  }
/*
wbuff[i*np+2*j]=dphi((float)in_data[2*i].lo[2*j],(float)in_data[2*i+1].lo[2*j],(float)in_data[2*i].lo[2*j+1],(float)in_data[2*i+1].lo[2*j+1]);
*/
}

void getdata(int *si, int *nproj, float *wbuff, char infile_name1[])
    
{
    long    element_size, maxval=16384, scale_factor, temp;
    int     i, j, k,l, nfids_in, in_file, np;
    float   stor, max, tmp_conv;
    FILE    *in_ptr;
    char    string[100];
    union   precision {
		short  *sh;
		float   *lo;
	    } *in_data;


    /****
    * Determine the accessibility of the input directory.    ****/

    if ((access(infile_name1, F_OK | R_OK))) 
    {
	perror("Access");
	printf("Input file either does not exist, or cannot be read.\n");
	exit(2);
    }

    /****
    * Open input fid file.
    ****/
    strcpy(string, infile_name1);
    in_file = open(string, O_RDWR);
    if (in_file < 0) 
    {
	perror("Open");
	printf("Can't open input fid file.\n");
	exit(4);
    }

    /****
    * Read main_header at top of input file.
    ****/
    if (read(in_file, &main_header, 32) != 32) 
    {
	perror ("read");
	exit(5);
    }

    #ifdef LINUX
       /* Reverse byte order of file header */
       reversedfh(&main_header);
    #endif	

    #ifdef DEBUG_ENDIAN
    fprintf(stderr,"File header :\n\n"); 
    fprintf(stderr," nblocks : %d\n",main_header.nblocks ); 
    fprintf(stderr," ntraces : %d\n", main_header.ntraces); 
    fprintf(stderr," np : %d\n",main_header.np); 
    fprintf(stderr," ebytes : %d\n",main_header.ebytes); 
    fprintf(stderr," tbytes : %d\n",main_header.tbytes); 
    fprintf(stderr," bbytes : %d\n",main_header.bbytes); 
    fprintf(stderr," vers_id : %d\n",main_header.vers_id); 
    fprintf(stderr," status : %d\n",main_header.status); 
    fprintf(stderr," status hexa : 0x%16X\n\n",main_header.status); 
    fprintf(stderr," nbheaders : %d\n\n",main_header.nbheaders); 
    #endif

    np = main_header.np;
    *si=np;
    element_size = main_header.ebytes;
    nfids_in = main_header.nblocks;
    *nproj=nfids_in;
 
    /****
    * Allocate space for input data.  All calculations will be done
    * with "long" integers (normal "int" on the SUN), so wee will
    * allocate space for this, even if the data is "short".
    ****/
    in_data = (union precision *)malloc(nfids_in*sizeof(int*));
    for (i=0; i<nfids_in; i++) 
    {
	in_data[i].lo = (float *)malloc(np*sizeof(float));    
    }
    /****
    * Read in raw data.
    ****/
    for (i=0; i<nfids_in; i++) 
    {
    if (read(in_file, &block_header, 28) != 28) 
    {
	perror ("read2");
	exit(5);
    }
    #ifdef LINUX
	/* Reverse byte order of block header */
	reversedbh(&block_header);
    #endif

	
	#ifdef	DEBUG_ENDIAN
	fprintf(stderr,"  Block nbr reversed : %d\n",i );	
	fprintf(stderr,"Block header :\n\n"); 
	fprintf(stderr," scale : %d\n",block_header.scale );
	fprintf(stderr," status : %d\n",block_header.status ); 
	fprintf(stderr," index : %d\n", block_header.index); 
	fprintf(stderr," mode : %d\n",block_header.mode); 
	fprintf(stderr," ctcount : %d\n",block_header.ctcount); 
	fprintf(stderr," lpval : %d\n",block_header.lpval); 
	fprintf(stderr," rpval : %d\n",block_header.rpval); 
	fprintf(stderr," lvl : %d\n",block_header.lvl); 
	fprintf(stderr," tlt : %d\n",block_header.tlt); 
        fprintf(stderr,"  Value of np : %d\n",np );
	#endif	

	
    if (read(in_file, in_data[i].lo, np*sizeof(float)) != np*sizeof(float)) 
    {
	perror ("read3");
	exit(5);
    }
    }
    for (i=0; i<nfids_in; i++)
      {
        for (j=0; j<np; j++)
        {
         tmp_conv = (float)in_data[i].lo[j];
         #ifdef LINUX
           reverse4ByteOrder(1,(char *)&tmp_conv);
         #endif       
         wbuff[i*np+j]=tmp_conv;
        }
      }
    fprintf(stderr,"done reading .. ");
    fprintf(stderr,"%5d points in %3d projections (%2d el. size)\n",*si,nfids_in,element_size);
    close(in_file);
    free(in_data);
  }/*getdata*/


void putdata(int *si, float *wbuff, char infile_name1[])
    
{
    long    element_size, maxval=16384, scale_factor, temp;
    int     i, j, k,l, nfids_out, out_file, np;
    float   stor, max;
    FILE    *out_ptr;
    char    string[100];
    union   precision {
		short  *sh;
		float   *lo;
	    } *out_data;

    strcpy(string, infile_name1);
    out_file = open(string, O_RDWR);
    if (out_file < 0) 
    {
	perror("Open");
	printf("Can't open output fid file.\n");
	exit(4);
    }
    np = main_header.np;
    nfids_out = main_header.nblocks;

	#ifdef	DEBUG_ENDIAN
	  fprintf(stderr,"Putdata function - np : %d\n",np );	
	  fprintf(stderr,"Putdata function - nfids_out : %d\n",nfids_out  );
	#endif

 
    /****
    * Allocate space for output data.  All calculations will be done
    * with "long" integers (normal "int" on the SUN), so wee will
    * allocate space for this, even if the data is "short".
    ****/
    out_data = (union precision *)malloc(nfids_out*sizeof(int*));
    for (i=0; i<nfids_out; i++) 
    {
	out_data[i].lo = (float *)malloc(np*sizeof(float));    
    }
    #ifdef LINUX
       /* Reverse byte order of file header */
       reversedfh(&main_header);
    #endif
    write(out_file, &main_header, 32);

    for (i=0; i<nfids_out; i++)
      {
      for (j=0; j<np/2-1; j++)
      {
	out_data[i].lo[2*j] = wbuff[i*np+2*j];
	out_data[i].lo[2*j+1] = wbuff[i*np+2*j+1];

        #ifdef LINUX
          reverse4ByteOrder(1,(char *)&out_data[i].lo[2*j]);
          reverse4ByteOrder(1,(char *)&out_data[i].lo[2*j+1]);
        #endif 
      }
    }
    for (k=0; k<nfids_out; k++) {
	block_header.index = k + 1;
        #ifdef LINUX
	  /* Reverse byte order of block header */
	  reversedbh(&block_header);
        #endif
	write(out_file, &block_header, 28);
	write(out_file, out_data[k].lo, np*sizeof(float));
        #ifdef LINUX
	  /* Reverse back the byte order of block header for the next loop */
	  reversedbh(&block_header);
        #endif
    }
    close(out_file);
    free(out_data);
}
