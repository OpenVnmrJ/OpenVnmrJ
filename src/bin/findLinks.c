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


#include <sys/stat.h>
#include <stdio.h>
/*#include "fcntl.h" */
/*#include "unistd.h" */
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>

#ifndef PATH_MAX
#define PATH_MAX    1024    /* max # of characters in a path name */
#endif

/* output file pointer for use during the recursive calls to desend the tree */
FILE *outputFilePtr;


void writeOutLinks(char *dir);


/**************************************************************************
    findLinks

    Given a single arg of a directory, look recursively from there down
    for symbolic links.  Also look up from there checking its parents.
    Write all links found to a file (/tmp/findLinksList).  Write
    the current path and the canonical path. 

    Usage: findLinks directory_path output_file_path

*************************************************************************/

int main(int argc, char *argv[]) {
    struct stat     stbuf;
    char            canonPath[PATH_MAX];
    char            output[PATH_MAX];
    char            dir[PATH_MAX];
    

    if(argc > 2) {

        /* Open the output file. */
        outputFilePtr = fopen(argv[2], "w");

        // catch problem with fopen
        if(outputFilePtr == 0) {
            printf("findLinks Cannot find %s\n", argv[2]);
            exit(EXIT_SUCCESS);
        }

        /* Write an identifying heading */
        fputs("Links List\n", outputFilePtr);

        if(lstat(argv[1], &stbuf) == -1) {
            printf("findLinks Cannot find %s\n", argv[1]);
        }
        else {
            /* Take care of the directory passed in.  Calls to writeOutLinks()
               only take care of directories below this.
            */
            if((stbuf.st_mode & S_IFLNK) == S_IFLNK) {
                /* It is a symbolic link */
              // I think this printf is overloading the stdout buffer
              //                printf("True, %s is a Link\n", argv[1]);

                /* Write to the output file. */
                realpath(argv[1], canonPath);
                sprintf(output, "%s  %s\n", argv[1], canonPath);
                fputs(output, outputFilePtr);
            }
            /* We need to check the parent directories above agrv[1] to
               see if there are any symbolic links above this.  That means
               starting at the end of the string, and backing up to each
               '/' and checking the path before that.
            */
            strcpy(dir, argv[1]);
            while(strlen(dir) > 1) {
                /* Strip off end up and including last '/'   */
                while(strlen(dir) > 1 && dir[strlen(dir) -1] != '/') {
                    // If it is not slash, terminate here
                    dir[strlen(dir) -1] = '\0';
                }

                /* We should either have an empty string, or we should
                   have trimmed backwards to the next slash.  If empty,
                   we are finished, else, we need to check to see if this
                   directory is a link. 
                */
                if(strlen(dir) > 1) {
                    /* the last char should be slash, remove it. */
                    dir[strlen(dir) -1] = '\0';
                    /* Check for link */
                    if(lstat(dir, &stbuf) == -1) {
                        printf("Cannot find %s\n", dir);
                    }
                    else {
                        if((stbuf.st_mode & S_IFLNK) == S_IFLNK) {
                            /* It is a symbolic link */
                          // I think this printf is overloading the stdout buffer
                          //  printf("True, %s is a Link\n", dir);

                            /* Write to the output file. */
                            realpath(dir, canonPath);
                            sprintf(output, "%s  %s\n", dir, canonPath);
                            fputs(output, outputFilePtr);
                        }
                    }
                }
            }
            
            /* Start the recursive calls to writeOutLinks() to desend 
               the tree 
            */
            writeOutLinks(argv[1]);
        }

        fclose(outputFilePtr);
    }
    else {
        printf("Usage: findLinks directory_path output_file_path\n");
    }
    exit(EXIT_SUCCESS);
}

/* Recursively call this to go down the directory tree looking for links. */
void writeOutLinks(char *dir) {
    struct stat     stbuf;
    struct dirent   *dp;
    DIR             *dirp;
    char            subdir[PATH_MAX];
    int             len;
    char            *endname;
    char            canonPath[PATH_MAX];
    char            output[PATH_MAX];


    /* Open the directory */
    if ( (dirp = opendir(dir)) == NULL)
       return;

    /* while more sub directories in the list */
        /* readdir() basically steps the dirp until it ends up null. */
     while ((dp = readdir(dirp)) != NULL)
     {
            /* Skip . and .. */
            if(strcmp(dp->d_name, ".") == 0 ||
                           strcmp(dp->d_name, "..") == 0)
                continue;

            /* skip .fid and .xml files */
            len = strlen(dp->d_name);
            endname = dp->d_name + len -4;
            if(strcmp(endname, ".fid") == 0 || strcmp(endname, ".xml") == 0)
                continue;

            /* Create the full path */
            sprintf(subdir, "%s/%s", dir, dp->d_name);

            /* Now see if this listing entry is a directory or a link */
            /* If it is a directory, call this function recursively, if it */
            /* is a link, add it to the link list. */
        
            if(lstat(subdir, &stbuf) == -1) {
                printf("Cannot find %s\n", subdir);
            }
            else {
                /* Is it a symbolic link? */
                if((stbuf.st_mode & S_IFLNK) == S_IFLNK) {

                    /* It is a symbolic link */
                  // I think this printf is overloading the stdout buffer
                  // printf("True, %s is a Link\n", subdir);


                    /* Write to the output file. */
                    realpath(subdir, canonPath);
                    sprintf(output, "%s  %s\n", subdir, canonPath);
                    fputs(output, outputFilePtr);

                    /* Check to see if this is part of a cyclic link.  Do this
                       by checking to see if the parent of this subdir
                       (which is 'dir') starts with the canonical path of this 
                       subdir. */
 
                    /* Compare only up to the length of canonPath */
                    if(strncmp(dir, canonPath, strlen(canonPath)) == 0) {
                        printf("Cyclic Link found, skipping %s\n" , subdir);
                        /* Do not go any further down this path */
                        continue;
                    }

                    /* If we hit a link, we still want to desend and look
                       for more links, however, we need to go on with the
                       conanical path from here.  Overwrite subdir.
                    */
                    strcpy(subdir, canonPath);
                }

                /* Is it a directory or a link?*/
                if((stbuf.st_mode & S_IFDIR) == S_IFDIR ||
                    (stbuf.st_mode & S_IFLNK) == S_IFLNK) {
                    /* Call back into this function to desend the tree. */
                    writeOutLinks(subdir);
                }
            }
     }
     closedir(dirp);
     return;
}
