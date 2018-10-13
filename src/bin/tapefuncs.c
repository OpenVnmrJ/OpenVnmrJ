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
/*  tapefuncs.c - used with tape_main.c to assemble 'tape' */
#include <sys/types.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>

static struct mtop mtcmd;	/* mag tape command structure */
static int fd;			/* descriptor for 9-track tape */
static long  nchr;

extern int errno;

/*---------------------------------------------------------------
|
|	Mount Tape
|
+-------------------------------------------------------------*/
mounttape( tmode )
int tmode;
{
    fd = open("/dev/rmt12",tmode,0666);
    if (fd < 0) {
        perror("open error");
	fprintf(stderr,"No Tape on Tape Drive?\n");
        return(1);
    }
    return(0);
}
/*---------------------------------------------------------------
|
|	Dismount Tape
|
+-------------------------------------------------------------*/
dismount()
{
    mtcmd.mt_op = MTOFFL;
    mtcmd.mt_count = 1L;
    if (ioctl(fd,MTIOCTOP,&mtcmd) == -1)
    {
        perror("rewind error");
        return(1);
    }
    close(fd);
    return(0);
}

/*---------------------------------------------------------------
|
|	Rewind Tape
|
+-------------------------------------------------------------*/
rewind()
{
    mtcmd.mt_op = MTREW;
    mtcmd.mt_count = 1L;
    if (ioctl(fd,MTIOCTOP,&mtcmd) == -1)
    {
        perror("rewind error");
        return(1);
    }
    return(0);
}

/*---------------------------------------------------------------
|
|	tapeskip/2
|	motion    1 = forward  space file
|		  2 = backward space file
|		  3 = forward  space record
|		  4 = backward space record
|	count:	repetion of action
|
+-------------------------------------------------------------*/
tapeskip(motion,count)
int motion;
long count;
{
    mtcmd.mt_op = motion;
    mtcmd.mt_count = count;
    if (ioctl(fd,MTIOCTOP,&mtcmd) == -1)
    {
        perror("tapeskip error");
        return(1);
    }
    return(0);
}

/*---------------------------------------------------------------
|
|	Read Tape
|	readtape()/2  pointer to buffer, size of buffer
|	returns number of bytes read, or -1 for error
|
+-------------------------------------------------------------*/
int readtape(buffer,bufsize)
char *buffer;
int bufsize;
{
    nchr = read(fd,buffer,bufsize);
    if (nchr == -1)
    {   fprintf(stderr,"errno = %d\n",errno);
        perror("read error");
        return(nchr);
    }
    return(nchr);
}

/*---------------------------------------------------------------
|
|	Write Tape
|	writetape()/2  pointer to buffer, size of buffer
|	returns number of bytes written, or -1 for error
|
+-------------------------------------------------------------*/
int writetape(buffer,bufsize)
char *buffer;
int bufsize;
{
    nchr = write(fd,buffer,bufsize);
    if (nchr == -1)
    {
        perror("writetape error");
        return(nchr);
    }
    return(nchr);
}
