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
/****************************************************************/
/*								*/
/*	######  ######        # #     # ######			*/
/*	#     # #     #      #  #  #  # #     #			*/
/*	#     # #     #     #   #  #  # #     #			*/
/*	######  #     #    #    #  #  # ######			*/
/*	#   #   #     #   #     #  #  # #   #			*/
/*	#    #  #     #  #      #  #  # #    #			*/
/*	#     # ######  #        ## ##  #     #			*/
/*								*/
/*	input / output functions for bp_3d			*/
/*								*/
/****************************************************************/
/*								*/
/*	authors:	Martin Staemmler			*/
/*								*/
/*	Institute for biomedical Engineering (IBMT)		*/
/*	D - 66386 St. Ingbert, Germany				*/
/*	Ensheimerstrasse 48					*/
/*	Tel.: (+49) 6894 980251					*/
/*	Fax:  (+49) 6894 980400					*/
/*								*/
/****************************************************************/
/*								*/
/*	date:		15.02.92				*/
/*	revision:	initial; release			*/
/*								*/
/****************************************************************/
#include	<stdio.h>
#include	"bp.h"

/****************************************************************
* Old File header for three dimensional volume image data 	*
*  Added for display with disp3d and subsequently removed in	*
*  conjuction with changes to disp3d & ImageBrowser - m. howitt	*
*****************************************************************/
/* struct head3D {	/* File header for new format output file */
/*   long magic;	/* Magic number for new format output file */
/*   long slowdim;	/* Dimension of slowest changing index */
/*   long meddim;	/* Dimension of medium changing index */
/*   long fastdim;	/* Dimension of fastest changing index */
/* }; */
/* Magic number ofor the new format output file */
/* #define FFT3D_MAGIC 0xb29b7d8f */

int access_file (char *name, struct datafilehead *fh, struct datablockhead *bh, int bigendian)
{
int		filesize;
FILE		*fp;

if ((fp = fopen (name, "r")) == NULL) {
    fprintf (stderr,"BP_RDWR: can't access %s\n", name);
    return (ERROR);
    }

/* get filesize to check for consistancy */
fseek (fp,0L,SEEK_END);
filesize = (int)ftell(fp);
fseek (fp,0L,SEEK_SET);

/* read file header for data structure information */
if (fread ((char *)fh, sizeof(struct datafilehead), 1, fp) != 1) {
    fprintf (stderr,"BP_RDWR: can't read fh\n");
    return (ERROR);
    }

if(bigendian)
   DATAFILEHEADER_CONVERT_NTOH(fh);

/* check if file is complete */
if (sizeof(struct datafilehead) + fh->nblocks * fh->bbytes > filesize) {
    fprintf (stderr,"BP_RDWR: inconsistant file size\n");
    fprintf (stderr,"BP_RDWR: filesize = %d expected filesize = %d\n",
                     filesize, (sizeof(struct datafilehead) + 
					(fh->nblocks * fh->bbytes)));
    return (ERROR);
    }

/* read block information of first block */
if (fread ((char *)bh, sizeof(struct datablockhead), 1, fp) != 1) {
    fprintf (stderr,"BP_RDWR: can't read first blockhead\n");
    return (ERROR);
    }

 if(bigendian)
   DATABLOCKHEADER_CONVERT_NTOH(bh);

fclose (fp);
return (TRUE);
}
/**********************************/
read_vnmr_data (name,fh,from,to,inc,p_data, bigendian)
char		*name;
int		from;
int		to;
int		inc;
char		*p_data;
struct		datafilehead	*fh;
int bigendian;
{
int		act_trace;
FILE		*fp;
int l1;
int             iblk, ipt;
float *p_float;
int *p_int;
struct          datablockhead  *bhp;

if ((fp = fopen (name, "r")) == NULL) {
    fprintf (stderr,"BP_RDWR: can't open %s\n", name);
    return (0);
    }

/* check if from and to is within the given # of traces */
if (from < 0 || from > fh->nblocks * fh->ntraces) {
    fprintf (stderr,"BP_RDWR: index from = %d ouside limits\n",from);
    return (ERROR);
    }
if (to < 0 || to > fh->nblocks * fh->ntraces) {
    fprintf (stderr,"BP_RDWR: index to = %d ouside limits\n",to);
    return (ERROR);
    }

if ((inc == 1) && (to - from <= fh->ntraces) && (from % fh->ntraces == 0)) {
    fseek (fp, sizeof(struct datafilehead) +
          (from / fh->ntraces) * fh->bbytes +
          sizeof (struct datablockhead), 0);
    if (fread ((char *)p_data, (int)fh->tbytes * (to - from), 1, fp) != 1) {
        fprintf (stderr,"BP_RDWR: can't fast read\n");
        return (ERROR);
        }
    if(bigendian) /* byte swap the data */
      {
	if(fh->status & S_FLOAT)
	  for(ipt=0;ipt<(to-from)*fh->np;ipt++)
	    {
	      p_float=(float *)p_data;
	      memcpy(&l1,(p_float+ipt),sizeof(int));
	      l1=ntohl(l1);
	      memcpy((p_float+ipt),&l1,sizeof(int));
	    }
	else
	  {
	    p_int = (int *)p_data;
	    for (ipt=0;ipt<2*(fh->np);ipt++)
	      {
		l1=ntohl(*(p_int+ipt));
		*(p_int+ipt)=l1;
	      }
	  }
      }
    fclose (fp);
    return (TRUE);
    }
else { 
    for (act_trace = from; act_trace < to; act_trace += inc) {
        /* seek forward to act_trace */
        fseek (fp, sizeof(struct datafilehead) +
              (act_trace / fh->ntraces) * fh->bbytes +
              (act_trace % fh->ntraces) * fh->tbytes +
              sizeof (struct datablockhead), 0);
        if (fread ((char *)p_data, (int)fh->tbytes, 1, fp) != 1) {
            fprintf (stderr,"BP_RDWR: can't read trace=%d\n", act_trace);
            return (ERROR);
            }
	if(bigendian) /* byte swap the data */
	  {
	    if(fh->status & S_FLOAT)
	      for(ipt=0;ipt<fh->np;ipt++)
		{
		  p_float=(float *)p_data;
		  memcpy(&l1,(p_float+ipt),sizeof(int));
		  l1=ntohl(l1);
		  memcpy((p_float+ipt),&l1,sizeof(int));
		}
	    else
	      {
		p_int = (int *)p_data;
		for (ipt=0;ipt<2*(fh->np);ipt++)
		  {
		    l1=ntohl(*(p_int+ipt));
		    *(p_int+ipt)=l1;
		  }
	      }
	  }
        p_data += fh->tbytes;
        }
    fclose (fp);
    return (TRUE);
    } 
}

int create_file (char *name, struct datafilehead *fh, struct datablockhead *bh)
{
int		act_block;
FILE		*fp;

if ((fp = fopen (name, "w")) == NULL) {
    fprintf (stderr,"BP_RDWR: can't create %s\n", name);
    return (ERROR);
    }
/* write file header for data structure information */
if (fwrite ((char *)fh, sizeof(struct datafilehead), 1, fp) != 1) {
    fprintf (stderr,"BP_RDWR: can't write fh\n");
    return (ERROR);
    }
/* write block information for all blocks */
for (act_block = 0; act_block < fh->nblocks; act_block++) {
    fseek (fp, sizeof(struct datafilehead) +
           act_block * (fh->ntraces * fh->tbytes + 
           sizeof(struct datablockhead)), 0);
    bh->index = act_block;
    if (fwrite ((char *)bh, sizeof(struct datablockhead),
        1, fp) != 1) {
        fprintf (stderr,"BP_RDWR: can't write blockhead #=%d\n", act_block);
        return (ERROR);
        }
    }   /* end of loops for all blocks */
fclose (fp);
return (TRUE);
}

write_vnmr_data (name,fh,from,to,inc,p_data)
char		*name;
int		from;
int		to;
int		inc;
char		*p_data;
struct		datafilehead	*fh;
{
int		act_trace;
FILE		*fp;

if ((fp = fopen (name, "r+")) == NULL) {
    fprintf (stderr,"BP_RDWR: can't create %s\n", name);
    return (ERROR);
    }
/* check if from and to is within the given # of traces */
if (from < 0 || from > fh->nblocks * fh->ntraces) {
    fprintf (stderr,"BP_RDWR: index from = %d ouside limits\n",from);
    return (ERROR);
    }
if (to < 0 || to > fh->nblocks * fh->ntraces) {
    fprintf (stderr,"BP_RDWR: index to = %d ouside limits\n",to);
    return (ERROR);
    }

if ((inc == 1) && (to - from <= fh->ntraces) && (from % fh->ntraces == 0)) {
    fseek (fp, sizeof(struct datafilehead) +
          (from / fh->ntraces) * fh->bbytes +
          sizeof (struct datablockhead), 0);
    if (fwrite ((char *)p_data, (int)fh->tbytes * (to - from), 1, fp) != 1) {
        fprintf (stderr,"BP_RDWR: can't fast write\n"); 
        return (ERROR);
        }
    fclose (fp);
    return (TRUE);
    }
else { 
    for (act_trace = from; act_trace < to; act_trace += inc) {
        /* seek forward to trace = from */
        fseek (fp, sizeof(struct datafilehead) +
              (act_trace / fh->ntraces) * fh->bbytes +
              (act_trace % fh->ntraces) * fh->tbytes +
              sizeof (struct datablockhead), 0);
        if (fwrite ((char *)p_data, (int)fh->tbytes, 1, fp) != 1) {
            fprintf (stderr,"BP_RDWR: can't write trace=%d\n", act_trace);
            return (ERROR);
            }
        p_data += fh->tbytes;
        }
    fclose (fp);
    return (TRUE);
    } 
}


int create_phf_file (char *name, struct datafilehead *fh, struct datablockhead *bh)
{
int		act_block;
FILE		*fp;

if ((fp = fopen (name, "w")) == NULL) {
    fprintf (stderr,"BP_RDWR: can't create %s\n", name);
    return (ERROR);
    }
/* write file header for data structure information */
if (fwrite ((char *)fh, sizeof(struct datafilehead), 1, fp) != 1) {
    fprintf (stderr,"BP_RDWR: can't write fh\n");
    return (ERROR);
    }
/* write 1st block header for data structure information */
if (fwrite ((char *)bh, sizeof(struct datablockhead), 1, fp) != 1) {
    fprintf (stderr,"BP_RDWR: can't write fh\n");
    return (ERROR);
    }
/* write block information for all blocks */
for (act_block = 0; act_block < fh->nblocks; act_block++) {
    fseek (fp, sizeof(struct datafilehead) + sizeof(struct datablockhead) +
           act_block * (fh->ntraces * fh->tbytes + 
           sizeof(struct datablockhead)), 0);
    bh->index = act_block;
    if (fwrite ((char *)bh, sizeof(struct datablockhead),
        1, fp) != 1) {
        fprintf (stderr,"BP_RDWR: can't write blockhead #=%d\n", act_block);
        return (ERROR);
        }
    }   /* end of loops for all blocks */
fclose (fp);
return (TRUE);
}

write_phf_data (name,fh,from,to,inc,p_data)
char		*name;
int		from;
int		to;
int		inc;
char		*p_data;
struct		datafilehead	*fh;
{
int		act_trace;
FILE		*fp;

if ((fp = fopen (name, "r+")) == NULL) {
    fprintf (stderr,"BP_RDWR: can't create %s\n", name);
    return (ERROR);
    }
/* check if from and to is within the given # of traces */
if (from < 0 || from > fh->nblocks * fh->ntraces) {
    fprintf (stderr,"BP_RDWR: index from = %d ouside limits\n",from);
    return (ERROR);
    }
if (to < 0 || to > fh->nblocks * fh->ntraces) {
    fprintf (stderr,"BP_RDWR: index to = %d ouside limits\n",to);
    return (ERROR);
    }

if ((inc == 1) && (to - from <= fh->ntraces) && (from % fh->ntraces == 0)) {
    fseek (fp, sizeof(struct datafilehead) + sizeof(struct datablockhead) +
          (from / fh->ntraces) * fh->bbytes +
          sizeof (struct datablockhead), 0);
    if (fwrite ((char *)p_data, (int)fh->tbytes * (to - from), 1, fp) != 1) {
        fprintf (stderr,"BP_RDWR: can't fast write\n"); 
        return (ERROR);
        }
    fclose (fp);
    return (TRUE);
    }
else { 
    for (act_trace = from; act_trace < to; act_trace += inc) {
        /* seek forward to trace = from */
        fseek (fp, sizeof(struct datafilehead) + sizeof(struct datablockhead) +
              (act_trace / fh->ntraces) * fh->bbytes +
              (act_trace % fh->ntraces) * fh->tbytes +
              sizeof (struct datablockhead), 0);
        if (fwrite ((char *)p_data, (int)fh->tbytes, 1, fp) != 1) {
            fprintf (stderr,"BP_RDWR: can't write trace=%d\n", act_trace);
            return (ERROR);
            }
        p_data += fh->tbytes;
        }
    fclose (fp);
    return (TRUE);
    } 
}


FILE *create_3d_file(name, fh, bh)
char		*name;
struct		datafilehead	*fh;
struct		datablockhead	*bh;
{
int		act_block;
FILE		*fp;
/* struct head3D 	h3D; */

if ((fp = fopen (name, "w")) == NULL) {
    fprintf(stderr,"BP_RDWR: can't create %s\n", name);
    return (NULL);
    }

/*------------------------------------------------------------	*/
/* Old disp3d data format removed so only data is output. FDF	*/
/* header is created and glued on to data by accompaning macro.	*/
/* 					m. howitt 8-19-96	*/
/*------------------------------------------------------------	*/
/* h3D.magic = FFT3D_MAGIC; */
/* h3D.fastdim = fh->np; */
/* h3D.meddim = fh->ntraces; */
/* h3D.slowdim = fh->nblocks; */
/* printf("3D Output header: fastdim= %d meddim= %d slowdim=%d\n", */
/* 			h3D.fastdim,h3D.meddim,h3D.slowdim); */
/* write file header for data structure information */
/* if (fwrite((char *)&h3D, sizeof(struct head3D), 1, fp) != 1) { */
/*     fprintf(stderr,"BP_RDWR: can't write fh\n"); */
/*     close(fp); */
/*     return(NULL); */
/*     } */
/* fclose (fp); */

return(fp);
}

write_3d_data (fp,fh,from,to,inc,p_data)
FILE		*fp;
int		from;
int		to;
int		inc;
char		*p_data;
struct		datafilehead	*fh;
{
int		act_trace;

/* if ((fp = fopen (name, "r+")) == NULL) {
 *    fprintf (stderr,"BP_RDWR: can't create %s\n", name);
 *    return (ERROR);
 *    }
 */
/* check if from and to is within the given # of traces */
if (from < 0 || from > fh->nblocks * fh->ntraces) {
    fprintf(stderr,"BP_RDWR: index from = %d ouside limits\n",from);
    return(ERROR);
    }
if (to < 0 || to > fh->nblocks * fh->ntraces) {
    fprintf (stderr,"BP_RDWR: index to = %d ouside limits\n",to);
    return(ERROR);
    }

if ((inc == 1) && (to - from <= fh->ntraces) && (from % fh->ntraces == 0)) {
/*    fseek (fp, sizeof(struct datafilehead) +
 *          (from / fh->ntraces) * fh->bbytes +
 *          sizeof (struct datablockhead), 0);
 */
    if (fwrite((char *)p_data, (int)fh->tbytes * (to - from), 1, fp) != 1) {
        fprintf(stderr,"BP_RDWR: can't fast write\n"); 
        return(ERROR);
        }
    return (TRUE);
    }
else { 
    for (act_trace = from; act_trace < to; act_trace += inc) {
        /* seek forward to trace = from */
        fseek(fp, sizeof(struct datafilehead) +
              (act_trace / fh->ntraces) * fh->bbytes +
              (act_trace % fh->ntraces) * fh->tbytes +
              sizeof (struct datablockhead), 0);
        if (fwrite((char *)p_data, (int)fh->tbytes, 1, fp) != 1) {
            fprintf (stderr,"BP_RDWR: can't write trace=%d\n", act_trace);
            return (ERROR);
            }
        p_data += fh->tbytes;
        }
    return (TRUE);
    } 
}

int create_ib_file (char *name, struct datafilehead *fh, struct datablockhead *bh)
{
int		act_block;
FILE		*fp;

if ((fp = fopen (name, "w")) == NULL) {
    fprintf (stderr,"BP_RDWR: can't create %s\n", name);
    return (ERROR);
    }
/* write file header for data structure information */
/* if (fwrite ((char *)fh, sizeof(struct datafilehead), 1, fp) != 1) {
 *    fprintf (stderr,"BP_RDWR: can't write fh\n");
 *    return (ERROR);
 *    }
 */

fclose (fp);
return (TRUE);
}

write_ib_data (name,fh,from,to,inc,p_data)
char		*name;
int		from;
int		to;
int		inc;
char		*p_data;
struct		datafilehead	*fh;
{
int		act_trace;
FILE		*fp;

if ((fp = fopen (name, "r+")) == NULL) {
    fprintf (stderr,"BP_RDWR: can't create %s\n", name);
    return (ERROR);
    }
/* check if from and to is within the given # of traces */
if (from < 0 || from > fh->nblocks * fh->ntraces) {
    fprintf (stderr,"BP_RDWR: index from = %d ouside limits\n",from);
    return (ERROR);
    }
if (to < 0 || to > fh->nblocks * fh->ntraces) {
    fprintf (stderr,"BP_RDWR: index to = %d ouside limits\n",to);
    return (ERROR);
    }

if ((inc == 1) && (to - from <= fh->ntraces) && (from % fh->ntraces == 0)) {
    fseek (fp, (from / fh->ntraces) * fh->bbytes, 0);
    if (fwrite ((char *)p_data, (int)fh->tbytes * (to - from), 1, fp) != 1) {
        fprintf (stderr,"BP_RDWR: can't fast write\n"); 
        return (ERROR);
        }
    fclose (fp);
    return (TRUE);
    }
else { 
    for (act_trace = from; act_trace < to; act_trace += inc) {
        /* seek forward to trace = from */
        fseek (fp, (act_trace / fh->ntraces) * fh->bbytes +
                   (act_trace % fh->ntraces) * fh->tbytes , 0);
        if (fwrite ((char *)p_data, (int)fh->tbytes, 1, fp) != 1) {
            fprintf (stderr,"BP_RDWR: can't write trace=%d\n", act_trace);
            return (ERROR);
            }
        p_data += fh->tbytes;
        }
    fclose (fp);
    return (TRUE);
    } 
}
