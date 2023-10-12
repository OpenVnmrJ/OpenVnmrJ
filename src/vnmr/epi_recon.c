/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/******************************************************************************
* File epi_reconc.c:  
*
* main epi reconstruction for single shot imaging (compressed fid data --> images)
* note: no conversion of fid file to standard or compressed format is performed
*
* 
******************************************************************************/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>
#include "data.h"
#include "group.h"
#include "mfileObj.h"
#include "symtab.h"
#include "vnmrsys.h"
#include "variables.h"
#include "process.h"
#include "epi_recon.h"
#include "phase_correct.h"

extern int write_fdf();
extern symbol **getTreeRoot();
extern varInfo *createVar();
extern void Vperror(char *);


/******************************************************************************/

/* the following courtesy of Numerical Recipes in C, Press, Teukolsy, et. al. */
#define SWAP(a,b) tempr=(a);(a)=(b);(b)=tempr
static void four1(data, nn, isign)
    float *data; unsigned long nn; int isign;
{
    unsigned long n,mmax,m,j,istep,i;
    double wtemp,wr,wpr,wpi,wi,theta;
    float tempr,tempi;
    
    n=nn << 1;
    j=1;
    for (i=1;i<n;i+=2) {
	if (j > i) {
	    SWAP(data[j],data[i]);
	    SWAP(data[j+1],data[i+1]);
	}
	m=n >> 1;
	while (m >= 2 && j > m) {
	    j -= m;
	    m >>= 1;
	}
	j += m;
    }
    mmax=2;
    while (n > mmax) {
	istep=mmax << 1;
	theta=isign*((2.0 * 3.14159265358979323846)/mmax);
	wtemp=sin(0.5*theta);
	wpr = -2.0*wtemp*wtemp;
	wpi=sin(theta);
	wr=1.0;
	wi=0.0;
	for (m=1;m<mmax;m+=2) {
	    for (i=m;i<=n;i+=istep) {
		j=i+mmax;
		tempr=wr*data[j]-wi*data[j+1];
		tempi=wr*data[j+1]+wi*data[j];
		data[j]=data[i]-tempr;
		data[j+1]=data[i+1]-tempi;
		data[i] += tempr;
		data[i+1] += tempi;
	    }
	    wr=(wtemp=wr)*wpr-wi*wpi+wr;
	    wi=wi*wpr+wtemp*wpi+wi;
	}
	mmax=istep;
    }	
    /* do a dc correction of sorts */
    data[1]=data[3];
    data[2]=data[4];
    /* shift zero frequency to center */
        for (i=0;i<nn/2;i++)
	{
	    j=nn/2+i;
	    SWAP(data[2*j+1],data[2*i+1]);
	    SWAP(data[2*j+2],data[2*i+2]);
	}

    return;
}
#undef SWAP

/*******************************************************************************/
/*******************************************************************************/
/*           EPI RECON                                                         */
/*******************************************************************************/
/*******************************************************************************/
epi_recon ( argc, argv, retc, retv )

/* 

Purpose:
-------
Routine epi_recon is the program for epi reconstruction.  

Arguments:
---------
argc  :  (   )  Argument count.
argv  :  (   )  Command line arguments.
retc  :  (   )  Return argument count.  Not used here.
retv  :  (   )  Return arguments.  Not used here

*/

int argc, retc;
char *argv[], *retv[];
/*ARGSUSED*/

{
/*   Local Variables: */
    FILE *f1;
    FILE *table_ptr;
    float *pc;
    char filepath[MAXPATHL];
    char tablefile[MAXPATHL];
    char petable[MAXSTR];
    char rscfilename[MAXPATHL];
    double ncomp, necho, nf, ni;
    double rslices,rnf;
    int npe,nro;
    int slices, etl, nblocks,ntraces, slice_reps;
    int slicesperblock;
    int error,error1,error2;
    int slice_compression=TRUE; /* a good assumption for epi */
    int flash_converted=FALSE;
    char str[MAXSTR];
    vInfo info;
    fdfInfo picInfo;
    char aname[SHORTSTR];
    char arraystr[MAXSTR];
    char *ptr;
    char *imptr;
    double rnshots, rfc;
    int imglen,dimfirst,dimafter,imfound;
    int icnt,ic1,ic2;
    int ispb;
    int blockctr;
    float *slicedata;
    float *sorted;
    float *ftemp;
    double rimflag;
    double wc, wc2;
    int imflag=TRUE; /* flag indicating image or reference scan */
    int multi_shot=FALSE;
    dpointers 	inblock, outblock;
    dfilehead	fid_file_head, data_file_head;
    int status, bstatus;
    int ipe,iro;
    long *iptr;
    double *dptr, *dp1, *dp2;
    double dtemp;
    float *fptr, *nrptr;
    int pc_offset, soffset, sort_offset;
    char epi_pc[MAXSTR];
    float templine[2*MAXPE];
    float a,b,c,d;
    float t1, t2, *pt1, *pt2, *pt3, *pt4, *pt5, *pt6;
    char *fargv[]={"epi_recon","nf","1"};
    int fargc=3;
    int ipt, npts;
    int im_slice=-1;
    int pc_slice=-1;
    int disp_mode;
    int outblockctr;
    int pc_option=MAX_OPTION+1;
    int pc_done=FALSE;
    int transposed=FALSE;
    int done;
    int *index_array;
    int itab;
    int nechoes;
    int *view_order;
    int first_image=TRUE;
    int i, min_view, iview;

    /*******************/
    /* executable code */
    /*******************/
    
    /* read vnmr parameters */
    error=P_getstring(CURRENT, "epi_images", picInfo.imdir, 1, MAXSTR);  
    if(!error)
    {
	error=P_getstring(CURRENT,"epi_pc", epi_pc, 1, MAXSTR);
	if(error)
	    (void)strcpy(epi_pc,"POINTWISE");
    }
    else  /* try the resource file to get recon particulars */
    {
	(void)strcpy(rscfilename,curexpdir);
	(void)strcat(rscfilename,"/epi_recon.rsc");
	f1=fopen(rscfilename,"r");
	if(f1)
	{
	    (void)fgets(epi_pc,MAXSTR,f1);
	    (void)sscanf(epi_pc,"image directory=%s",picInfo.imdir);
	    (void)fgets(epi_pc,MAXSTR,f1);
	    (void)fclose(f1);

	}
	else /* use some defaults if rsc file isn't there */
	{
	    (void)strcpy(epi_pc,"POINTWISE");
	    (void)strcpy(picInfo.imdir,"testepi");
	}
    }
    /* image directory is relative to current directory */
    picInfo.fullpath=FALSE;
    
    /* decipher phase correction option */
    if(strstr(epi_pc,"OFF")||strstr(epi_pc,"off"))
	pc_option=OFF; 
    else if(strstr(epi_pc,"POINTWISE")||strstr(epi_pc,"pointwise"))
	pc_option=POINTWISE; 
    else if(strstr(epi_pc,"LINEAR")||strstr(epi_pc,"linear"))
	pc_option=LINEAR; 
    else if(strstr(epi_pc,"QUADRATIC")||strstr(epi_pc,"quadratic"))
	pc_option=QUADRATIC; 
    else if(strstr(epi_pc,"CENTER_PAIR")||strstr(epi_pc,"center_pair")) 
	pc_option=CENTER_PAIR; 
    else if(strstr(epi_pc,"PAIRWISE")||strstr(epi_pc,"pairwise")) 
	pc_option=PAIRWISE;
    if(pc_option!=OFF)
    {
	if((pc_option>MAX_OPTION)||(pc_option<MIN_OPTION))
	{
	    Vperror("epi_recon: Invalid phase correction option");
	    ABORT;
	}
    }


    /* these won't change - for block & file header update*/	
    status = (S_DATA|S_FLOAT|S_COMPLEX);
    bstatus =status;
    bstatus |= NF_CMPLX;
    bstatus |= NP_CMPLX;
    bstatus |= NI_CMPLX;

    disp_mode  = NP_PHMODE;
    disp_mode |= NF_PHMODE;
    disp_mode |= NI_PHMODE;
 
    /* start out fresh */
    D_close(D_USERFILE);
    D_close(D_DATAFILE);

  /* open the fid file */
  (void)strcpy ( filepath, curexpdir );
  (void)strcat ( filepath, "/acqfil/fid" );  
  (void)strcpy(picInfo.fidname,filepath);
  error = D_gethead(D_USERFILE, &fid_file_head);
  if (error)
  {
      error = D_open(D_USERFILE, filepath, &fid_file_head);   
      if(error)
      {
	  Vperror("epi_recon: Error opening fid file");
	  ABORT;
      }      
  }
  
  
  /* figure out if compressed in slice dimension */   
  error=P_getstring(CURRENT, "seqcon", str, 1, MAXSTR);  
  if(error)
  {
      Vperror("epi_recon: Error getting seqcon");	
      D_close(D_USERFILE);
      ABORT;
  }
  if(str[1] == 's')slice_compression=FALSE;
  
  
  error=P_getreal(CURRENT,"ns",&rslices,1);
  if(error)
  {
      Vperror("epi_recon: Error getting ns");
      D_close(D_USERFILE);
      ABORT;
  }

  ntraces=fid_file_head.ntraces;

  error=P_getreal(PROCESSED,"nf",&rnf,1);
  if(error)
  {
      Vperror("epi_recon: Error getting nf");
      D_close(D_USERFILE);
      ABORT;
  }
  
  nblocks=fid_file_head.nblocks;
  nro=fid_file_head.np/2;
  
  error=P_getreal(CURRENT,"nshots",&rnshots,1); 
  if(error)
  {
      /*
      Vperror("epi_recon: Error getting nshots");
      D_close(D_USERFILE);
      ABORT;
      */
      rnshots=1.0;
  }

  if(slice_compression) /* check if flash converted */
  {
      error1=P_getreal(PROCESSED,"flash_converted",&rfc,1);
      error2=P_getreal(CURRENT,"flash_converted",&rfc,1);
      if((error1==0)||(error2==0))
	  flash_converted=TRUE;
      
      P_getVarInfo(CURRENT, "pss", &info);
      slices = info.size;
      slice_reps=nblocks;
      npe=(int)rnf/slices;
      slicesperblock=slices;
      if(flash_converted)
      {
	  npe=(int)rnf;
	  slicesperblock=1;
      }
  }	
  else
  {
      slices=(int)rslices;
      slice_reps=nblocks/slices;
      npe=(int)rnf;
      slicesperblock=1;
  }	
  etl=npe; /* assuming single shot for now */
   
  
  multi_shot=FALSE;
  if(rnshots > 1.0)
  {
      multi_shot=TRUE;
      etl=npe/rnshots;
      etl=npe;  /* not how it should be, but .... */

      /* open the table file */
      error=P_getstring(GLOBAL, "userdir", tablefile, 1, MAXPATHL);  
      if(error)
      {
	  Vperror("epi_recon: Error getting userdir");	
	  D_close(D_USERFILE);
	  ABORT;
      }
      (void)strcat(tablefile,"/tablib/" );  
      error=P_getstring(CURRENT, "petable", petable, 1, MAXSTR);  
      if(error)
      {
	  Vperror("epi_recon: Error getting petable");	
	  D_close(D_USERFILE);
	  ABORT;
      }
      (void)strcat(tablefile,petable);  
      table_ptr = fopen(tablefile,"r");
      if(!table_ptr)
      {	
	  Vperror("epi_recon: Error opening table file");	
	  D_close(D_USERFILE);
	  ABORT;
      }	
      /* read in table for view sorting */
      view_order = (int *)allocateWithId(npe*sizeof(int), "epi_recon");
      while (fgetc(table_ptr) != '=') ;
      itab = 0;
      done = FALSE;
      min_view=0;
      while ((itab<npe) && !done)
       {
	   if(fscanf(table_ptr, "%d", &iview) == 1)
	   {
	       view_order[itab]=iview;
	       if(iview<min_view)	
		   min_view=iview;
	       itab++;	
	   }
	   else	
	       done = TRUE;
       }
       (void)fclose(table_ptr);
      if(itab != npe)
      {
	  Vperror("epi_recon: Error wrong phase sorting table size");	
	  D_close(D_USERFILE);
	  (void)releaseAllWithId("epi_recon");
	  ABORT;
      }
      /* make it start from zero */
       for(i=0;i<npe;i++)
	   view_order[i]-=min_view;
  }	

    
  /* update the data file header (output) */
  data_file_head.nblocks=slices*slice_reps;
  data_file_head.ntraces=npe;
  data_file_head.np=2*nro;
  data_file_head.ebytes=fid_file_head.ebytes;
  data_file_head.tbytes=data_file_head.ebytes*data_file_head.np;
  data_file_head.bbytes=data_file_head.tbytes*data_file_head.ntraces + sizeof(dblockhead);
  data_file_head.nbheaders=1;
  data_file_head.status=(status | S_NP | S_NF | S_NI);
  data_file_head.status |= S_SPEC;
  data_file_head.status |= S_TRANSF;
  
  /* open the image data file */
  (void)strcpy ( filepath, curexpdir );
  (void)strcat ( filepath, "/datdir/data" );
  error=D_newhead(D_DATAFILE, filepath, &data_file_head);
  if(error)
  {
      Vperror("epi_recon: Error creating data file/header");
      D_close(D_USERFILE);
      D_close(D_DATAFILE);
      ABORT;
  }
  

  /*****************************************************************************/ 
  /* get array to see how reference scan data was interleaved with acquisition */
  /*****************************************************************************/ 
  imglen=1;
  dimfirst=1;
  dimafter=1;

  error=P_getVarInfo(CURRENT,"image",&info); /* read image variable */
  if(error)
  {
      Vperror("epi_recon: Error getting image");
      D_close(D_USERFILE);
      D_close(D_DATAFILE);
      ABORT;
  }

  imglen=info.size;

  error=P_getstring(CURRENT,"array", arraystr, 1, MAXSTR); /* read array variable */
  if(error)
  {
      Vperror("epi_recon: Error getting array");
      D_close(D_USERFILE);
      D_close(D_DATAFILE);
      ABORT;
  }

  imptr=strstr(arraystr,"image");
  if(imptr != NULL)  /* don't bother if no image */
  {
      imfound=0;
      /* interrogate array to find position of 'image' */	
      ptr=strtok(arraystr,",");
      while(ptr != NULL)
      {
	  if(ptr == imptr)
	  {
	      imfound=1;
	  }
	  /* is this a jointly arrayed thing? */
	  if(strstr(ptr,"("))
	  {
	      while(strstr(ptr,")")==NULL) /* move to end of jointly arrayed list */
		  ptr=strtok(NULL,",");
	      
	      *(ptr+strlen(ptr)-1)='\0';
	  }
	  strcpy(aname,ptr);
	  error=P_getVarInfo(CURRENT,aname,&info);
	  if(error)
	  {
	      Vperror("epi_recon: Error getting something");
	      D_close(D_USERFILE);
	      D_close(D_DATAFILE);
	      ABORT;
	  }

	  if(imfound)
	  {
	      if(ptr != imptr)
		  dimafter *= info.size;	 /* get dimension of this variable */	    
	  }
	  else
	      dimfirst *= info.size;

	  ptr=strtok(NULL,",");
      }
    }
  else
      dimfirst=nblocks;
  
  if(flash_converted)
      dimafter*=slices;

  /* set up fdf header structure */
  picInfo.nro=nro;
  picInfo.npe=npe;
  picInfo.fovro=10.;
  picInfo.fovpe=10.;
  picInfo.slices=slices;	
  picInfo.echo=1;
  picInfo.echoes=1;
  error=P_getreal(CURRENT,"te",&dtemp,1);
  if(error)
  {
      Vperror("epi_recon: epi_recon: Error getting te");
      D_close(D_USERFILE);
      D_close(D_DATAFILE);
      ABORT;
  }
  picInfo.te=(float)SEC_TO_MSEC*dtemp;
  error=P_getreal(CURRENT,"tr",&dtemp,1);
  if(error)
  {
      Vperror("epi_recon: epi_recon: Error getting tr");
      D_close(D_USERFILE);
      D_close(D_DATAFILE);
      ABORT;
  }
  picInfo.tr=(float)SEC_TO_MSEC*dtemp;
  picInfo.ro_size=nro;
  picInfo.pe_size=npe;
  error=P_getstring(CURRENT,"seqfil",picInfo.seqname,1,MAXSTR);
  if(error)
  {
      Vperror("epi_recon: epi_recon: Error getting sequence name");
      D_close(D_USERFILE);
      D_close(D_DATAFILE);
      ABORT;
  }
  error=P_getreal(CURRENT,"ti",&dtemp,1);
  if(error)
  {
      Vperror("epi_recon: epi_recon: Error getting ti");
      D_close(D_USERFILE);
      D_close(D_DATAFILE);
      ABORT;
  }
  picInfo.ti=(float)SEC_TO_MSEC*dtemp;
  picInfo.image=1.0;
  picInfo.array_index=1;
  picInfo.orientation[0]=1.0;
  picInfo.orientation[1]=0.0;
  picInfo.orientation[2]=0.0;
  picInfo.orientation[3]=0.0;
  picInfo.orientation[4]=1.0;
  picInfo.orientation[5]=0.0;
  picInfo.orientation[6]=0.0;
  picInfo.orientation[7]=0.0;
  picInfo.orientation[8]=1.0;


  /* read in data */
  blockctr=-1;
  outblockctr=-1;
  npts=npe*nro; /* number of pixels per slice */

  slicedata=(float *)allocateWithId(npts*2*sizeof(float),"epi_recon");
  pc=(float *)allocateWithId(etl*nro*slices*2*sizeof(float),"epi_recon");
  if(multi_shot)
      sorted=(float *)allocateWithId(2*npts*sizeof(float),"epi_recon");

/* loop on arrayed variables before image in 'array' */
  for(ic2=0;ic2<dimfirst;ic2++) 
  {
      for(icnt=0;icnt<imglen;icnt++)  /* loop on image */
      {
	  error=P_getreal(CURRENT,"image",&rimflag,(icnt+1));
	  if(error)
	  {
	      Vperror("epi_recon: Error getting image element");
	      (void)releaseAllWithId("epi_recon");
	      D_close(D_USERFILE);
	      D_close(D_DATAFILE);
	      ABORT;
	  }
	  imflag=(int)rimflag;

	  /* loop in arrayed variables after image in 'array' */
	  for(ic1=0;ic1<dimafter;ic1++)
	  {
	      ++blockctr;
	      ++outblockctr;
	      if((imflag)||(pc_option!=OFF))
	      {
		  /* read in a block of data */
		  if (D_getbuf(D_USERFILE,nblocks,blockctr,&inblock))
		  {
		      if (inblock.head == NULL) 
		      {
			  Vperror("epi_recon: Error reading fid block");
			  (void)releaseAllWithId("epi_recon");
			  D_close(D_USERFILE);
			  D_close(D_DATAFILE);
			  ABORT;
		      }	
		  }
		  iptr=(long *)inblock.data; /* start of raw data */
		  for(ispb=0;ispb<slicesperblock;ispb++)
		  {
		      /* update slice counters */
		      if((imflag)&&(icnt<2))
			  im_slice++;
		      else
			  pc_slice++;

		      error=D_allocbuf(D_DATAFILE, outblockctr, &outblock);
		      if(error)
		      {
			  Vperror("epi_recon: Error allocating data block");
			  (void)releaseAllWithId("epi_recon");
			  D_close(D_USERFILE);
			  D_close(D_DATAFILE);
			  ABORT;
		      }
		      /* scale & convert everything */
		      fptr=outblock.data;
		      nechoes=npe;
		      if(!imflag)
			  nechoes=etl;

		      for(ipe=0;ipe<nechoes;ipe++)
		      {
			  soffset=ipe*2*nro;
			  for(iro=0;iro<2*nro;iro++)
			  {
			      *fptr = *iptr++;
			      *fptr *= IMAGE_SCALE;
			      *(slicedata+soffset++)=*fptr++;
			  }
			  soffset-=2*nro;
			  if(ipe%2) /* time reverse data */
			  {
			      pt1=slicedata+soffset;
			      pt2=pt1+1;
			      pt3=pt1+2*(nro-1);
			      pt4=pt3+1;
			      for(iro=0;iro<nro/2;iro++)
			      {
				  t1=*pt1;
				  t2=*pt2;
				  *pt1=*pt3;
				  *pt2=*pt4;
				  *pt3=t1;
				  *pt4=t2;
				  pt1+=2;
				  pt2+=2;
				  pt3-=2;
				  pt4-=2;
			      }	
			  }

			  /* read direction ft */
			  nrptr=slicedata+soffset;
			  nrptr=nrptr-1;  /* four1 counts arrays as 1->n */
			  (void)four1(nrptr, (unsigned long)nro, 1);

			  /* phase correction application and/or sort data*/
			  if(imflag&&((pc_done)||(multi_shot)))
			  {
			      if(multi_shot)
				  sort_offset=view_order[ipe]*2*nro;
			      if(pc_done)
			      {
				  pc_offset=im_slice*etl*nro*2+ipe*nro*2;
				  pt1=slicedata+soffset;
				  pt2=pt1+1;
				  pt3=pc+pc_offset;
				  pt4=pt3+1;
				  pt5=pt1;
				  if(multi_shot)
				      pt5=sorted+sort_offset;
				  pt6=pt5+1;
				  for(iro=0;iro<nro;iro++)	
				  {
				      a=*pt1;	
				      b=*pt2;
				      c=*pt3;
				      d=*pt4;
				      *pt5=a*c-b*d;	
				      *pt6=a*d+b*c;
				      pt1+=2;
				      pt2+=2;
				      pt3+=2;
				      pt4+=2;
				      pt5+=2;
				      pt6+=2;
				  }
			      }	
			      else   /* only sort data, no phase correction */
			      {
				  pt1=slicedata+soffset;
				  pt2=pt1+1;
				  pt5=sorted+sort_offset;
				  pt6=pt5+1;
				  for(iro=0;iro<nro;iro++)
				  {
				      *pt5=*pt1;	
				      *pt6=*pt2;
				      pt1+=2;
				      pt2+=2;
				      pt5+=2;
				      pt6+=2;
				  }
			      }
			  }
		      }	/* end of loop on ipe (phase encode) */
		  
		  
		      if(multi_shot&&imflag) /* shuffle pointers around */
		      {
			  ftemp=slicedata;
			  slicedata=sorted;
		      }

		      /* compute phase correction if reference scan */
		      if((!imflag)&&(pc_option!=OFF))
		      {
			  pc_done=TRUE;
			  pc_offset=pc_slice*etl*nro*2;
			  (void)pc_calc(slicedata, (pc+pc_offset), nro, etl, pc_option, transposed);
		      }	
		  
		      if(imflag)
		      {
			  /* do phase direction ft */
			  for(iro=0;iro<nro;iro++)
			  {
			      /* get the ky line */
			      pt1=slicedata+2*iro;
			      pt2=pt1+1;
			      nrptr=templine;
			      for(ipe=0;ipe<npe;ipe++)
			      {
				  *nrptr++=*pt1;
				  *nrptr++=*pt2;
				  pt1+=2*nro;	
				  pt2=pt1+1;
			      }
			      nrptr=templine;
			      nrptr=nrptr-1;
			      (void)four1(nrptr, (unsigned long)npe, 1); 
			      /* write it back */
			      pt1=slicedata+2*iro;
			      pt2=pt1+1;
			      nrptr=templine;
			      for(ipe=0;ipe<npe;ipe++)
			      {
				  *pt1=*nrptr++;
				  *pt2=*nrptr++;
				  pt1+=2*nro;	
				  pt2=pt1+1;
			      }
			  }
		      }
		  
		      /* write magnitude slice data to output block */
			fptr=outblock.data;
			pt1=slicedata;
			pt2=pt1+1;
			for(ipt=0;ipt<npts;ipt++)
			{
			    a=*pt1;
			    b=*pt2;
			    *fptr++=(float)sqrt((double)(a*a+b*b));
			    pt1+=2;
			    pt2+=2;
			}

			if(multi_shot&&imflag) /* restore pointers */
			    slicedata=ftemp;


			/* write data to DATAFILE */
			/*
			setheader(&outblock, bstatus, disp_mode ,outblockctr,0);
		  
			outblock.head->rpval = 0;
			outblock.head->lpval = 0;
			outblock.head->scale = 0;
		  
			error = D_markupdated(D_DATAFILE,outblockctr);
			if(error)
			{
			    (void)sprintf(str,"epi_recon: Error updating data block %d",outblockctr);
			    Vperror(str);
			    (void)free(pc);
			    (void)free(slicedata);
			    D_close(D_USERFILE);
			    D_close(D_DATAFILE);
			    ABORT;
			}
		  
			*/


			error = D_release(D_DATAFILE, outblockctr);
			if(error)
			{
			    Vperror("epi_recon: Error releasing data block");
			    (void)releaseAllWithId("epi_recon");
			    D_close(D_USERFILE);
			    D_close(D_DATAFILE);
			    ABORT;
			}

			if(imflag)
			{
			    picInfo.slice=im_slice+1;
			    (void)write_fdf(icnt,outblock.data, &picInfo, &first_image);
			}
		  }	 
	      
		  error = D_release(D_USERFILE, blockctr);
		  if(error)
		  {
		      Vperror("epi_recon: Error releasing fid block");
		      (void)releaseAllWithId("epi_recon");
		      D_close(D_USERFILE);
		      D_close(D_DATAFILE);
		      ABORT;
		  }
	      }   
	  }  
      }  
  }   



/*
  error=D_flush(D_DATAFILE);
  if(error)
  {
      Vperror("epi_recon: Error flushing");
      (void)free(pc);
      (void)free(slicedata);pwd

      D_close(D_USERFILE);
      D_close(D_DATAFILE);
      ABORT;
  }
*/

  (void)releaseAllWithId("epi_recon");

  return(0);
}


