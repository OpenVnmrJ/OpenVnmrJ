/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* copy a file to a new file, and set permissions 0644 (-rw-r--r--).
 * cannot copy directories. one file at a time.
 * take two arguments f1, f2.
 * f1 has to be a file
 * f2 can be ".", "/.", "./...", "~/...", or any existing or 
 * non-existing directory, or any non-existing file
 * directory will be created if not exist.
 * new directory permission is drwxr-xr-x
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/statvfs.h>

#define MAXPATHL	256
#define BUFSIZE	1024	
#define PERMS 0644
#define RETURN         return(0)
#define ABORT         return(1)

void getFilename(char* path, char* name);

int main(int argc, char *argv[])
{
    int f1, f2, n;
    char buf[BUFSIZE];
    char name[MAXPATHL], origpath[MAXPATHL], destpath[MAXPATHL];

    int  diskIsFull;
    int  ival;
    struct stat     stat_blk;

/* if wild card is used, it will spand to all the files in the directory.*/
/* argc will be bigger than 3 (2 + number of files in that directory). */
/* abort if argc != 3 */
    if(argc != 3) {
	fprintf(stderr, "usage: safecp f1 f2\n"); 
	ABORT;
    }

/* remove ending "/" of argv[1], and copy it to origpath. */
    ival = strlen(argv[1]) - 1;
    if(argv[1][ival] == '/') {
	strcpy(origpath, "");
	strncat(origpath, argv[1], ival);
    }
    else strcpy(origpath, argv[1]);
 
/* abort if origpath is a dir */
    if(stat( origpath, &stat_blk ) != -1 && S_ISDIR(stat_blk.st_mode)) {
        fprintf(stderr, "safecp: cannot copy directory %s.\n", 
	origpath);
        ABORT;
    }

/* abort if cannot open origpath. */
    if( (f1 = open(origpath, O_RDONLY, 0)) == -1) {
	fprintf(stderr, "safecp: cannot open %s.\n", origpath);
	ABORT;
    }

/* fix destination file name: attach origpath filename if argv[2] */
/* ends with ".", "/.", or "/". copy fixed path to destpath. */
    ival = strlen(argv[2]) - 1;
    if(argv[2][ival] == '.') { 
	/* argv[2] ends with ".", replace it with origpath filename */
	getFilename(origpath, name);
	strcpy(destpath, ""); 
	strncat(destpath, argv[2], ival); 
	strcat(destpath, name); 
    } else if(argv[2][ival] == '/') { 
	/* argv[2] ends with "/", attach origpath filename */ 
	getFilename(origpath, name);
	strcpy(destpath, argv[2]); 
	strcat(destpath, name); 
    } else strcpy(destpath, argv[2]);

/* attach origpath filename, if destpath is an existing directory. */
    if(stat(destpath, &stat_blk) != -1 && S_ISDIR(stat_blk.st_mode)) {
	/* destpath exists and is a dir, attach origpath filename */
	getFilename(origpath, name);
	if(destpath[strlen(destpath)-1] != '/') strcat(destpath, "/");
	strcat(destpath, name); 
    }
/*
	fprintf(stderr,"orig, dest %s %s\n", origpath, destpath);
*/
/* abort if destpath exists */
    if(fileExist(destpath)) {
	fprintf(stderr, 
	"safecp: %s already exists, cannot overwrite.\n", destpath);
	ABORT;
    } else if( (f2 = createFile(destpath, PERMS)) == -1) {
	fprintf(stderr, "safecp: cannot create %s.\n", destpath);
	ABORT;
    }

/* check disk space */
    ival = isDiskFullFile(destpath, origpath, &diskIsFull );
    if (ival == 0 && diskIsFull) {
      	fprintf(stderr, 
	"safecp: problem copying %s, disk is full.\n", destpath);
      	ABORT;
    }
     
/* copy file */
    while((n = read(f1, buf, BUFSIZE)) > 0)
	if(write(f2, buf, n) != n) {
	    fprintf(stderr, "safecp: write error on file %s.\n", destpath);
	    ABORT;
	}

    close(f1);
    close(f2);
    RETURN;
}

int isDiskFullFile( path, path_2, resultadr )
char *path;
char *path_2;
int *resultadr;
{
        int             ival;
        int             blocksAvail, blocksRequired;
        struct stat     stat_blk;
        struct statvfs  partitionStatus;

        if (resultadr == NULL) {
                return( -1 );
        }

        ival = statvfs( path, &partitionStatus );
        if (ival != 0) {
                char    perror_msg[ MAXPATHL+16 ];

                strcpy( &perror_msg[ 0 ], "Cannot access " );
                strcat( &perror_msg[ 0 ], path );
                perror( &perror_msg[ 0 ] );
                return( -1 );
        }

        blocksAvail = partitionStatus.f_bavail;

/*  If we can't get a stat block on the 2nd file, report the disk is full
    if less than 1% is free; that is, the number of blocks required is 1%
    of the number of blocks total in the partition.
*/

        ival = stat( path_2, &stat_blk );
        if (ival != 0) {
                blocksRequired = partitionStatus.f_blocks / 100;
                if (blocksRequired < 10)
                  blocksRequired = 10;
        }
        else {
                blocksRequired = stat_blk.st_size;
                if (partitionStatus.f_frsize < 1)
                  blocksRequired = blocksRequired / 1024;
                else
                  blocksRequired = blocksRequired /  partitionStatus.f_frsize;
        }

        if (blocksAvail <= blocksRequired)
          *resultadr = 1;
        else
          *resultadr = 0;

        return( 0 );
}

int fileExist(char* path)
{
	int ival;
        struct stat     stat_blk;

	errno = 0;
        ival = stat( path, &stat_blk );
/* ival = 0 if stat is successful, ival = -1 if failed */
/* do not use ival to determine whether file exists, */
/* because stat fail when file exists but size overflow, */
/* search permission is denied... */

/* if the file does nit exist, errno will be set to ENOENT */
/*
	fprintf(stderr,"errno %d\n", errno);
	fprintf(stderr," ENOENT %d\n", ENOENT);
	fprintf(stderr," ival %d %s\n", ival, path);
*/
	if(errno == ENOENT) return(0);
	else return(1);
}

void getFilename(char* path, char* name) 
{
/* get file name, not file path. */

    char *strptr, *tokptr;
    char tmpstr[MAXPATHL];

/* copy path to tmpstr so it won't be modified! */
    strcpy(tmpstr, path);

    if(strstr(tmpstr, "/") == NULL) strcpy(name, tmpstr);
    else {
	strptr = tmpstr;
	while ((tokptr = (char*) strtok(strptr, "/")) != (char *) 0) {
		
	    strcpy(name, tokptr);
	    strptr = (char *) 0;	
	}
    }
}

int createFile(char *path, int perms)
{
    char dirpath[MAXPATHL], name[MAXPATHL], dir[MAXPATHL];
    char *strptr, *tokptr;
    int ival;

    getFilename(path, name);

    ival = strlen(name);
    strcpy(dirpath, "");
    strncat(dirpath, path, strlen(path) - ival);

    if(dirpath[0] == '/') strcpy(dir, "/");
    else strcpy(dir, "");
    strptr = dirpath;
    while ((tokptr = (char*) strtok(strptr, "/")) != (char *) 0) {
		
    	strcat(dir, tokptr);
	strcat(dir, "/");
	strptr = (char *) 0;	

	if(!fileExist(dir)) {
	    mkdir(dir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
/*
	    fprintf(stderr,"created dir %s\n", dir);
*/
	}
    }

    return(creat(path, perms));
}
