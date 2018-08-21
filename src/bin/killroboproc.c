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
/* 
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <strings.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#ifndef LINUX
#include <procfs.h>
#endif

/*
 * Make this binary setuid to run as root:
 *   > su root
 *   > chown root:root killroboproc
 *   > chmod 4755 killroboproc
 */
    
int main (int argc, char *argv[])
{
#ifndef LINUX
    psinfo_t psinfo;
#endif
    FILE *pfile;
    DIR *dirp;
    struct dirent *dp;
    char buf[32], comm[256];
    pid_t pRobo;
    int result;

    /*
     * Remove all "current" files from asm/info directory
     */
    dirp = opendir("/vnmr/asm/info");
    while (dirp) {
        if ((dp = readdir(dirp)) != NULL) {
            if ((!strncmp(dp->d_name, "tempEnterQ", 10)) ||
                (!strncmp(dp->d_name, "lastEnterQ", 10)) ||
                (!strncmp(dp->d_name, "turbStat", 10)) ||
                (!strncmp(dp->d_name, "magStat", 10)) ||
                (!strncmp(dp->d_name, "1_1_", 4)) ||
                (!strncmp(dp->d_name, "1_2_", 4)) ||
                (!strncmp(dp->d_name, "2_1_", 4)) ||
                (!strncmp(dp->d_name, "2_2_", 4)) ||
                (!strncmp(dp->d_name, "3_1_", 4)) ||
                (!strncmp(dp->d_name, "3_2_", 4)) ||
                (!strncmp(dp->d_name, "4_1_", 4)) ||
                (!strncmp(dp->d_name, "4_2_", 4)) ||
                (!strcmp( dp->d_name, "current"))) {
                sprintf(buf, "/vnmr/asm/info/%s", dp->d_name);
                remove(buf);
                rewinddir(dirp);
            }
        }
        else
            break;
    }
    closedir(dirp);


    /*
     * Locate the Roboproc process
     */
    pRobo = -1;
    dirp = opendir("/proc");
    while (dirp) {
        if ((dp = readdir(dirp)) != NULL) {
#ifdef LINUX
            sprintf(buf, "/proc/%s/stat", dp->d_name);
            if ((pfile = fopen(buf, "r")) != NULL) {
	      if ((fscanf(pfile, "%d %s ", &pRobo, &comm) == 2) &&
                  (strstr(comm, "Roboproc") != NULL)) {
#else
            sprintf(buf, "/proc/%s/psinfo", dp->d_name);
            if ((pfile = fopen(buf, "r")) != NULL) {
	        fread((char *)&psinfo, sizeof(psinfo_t), 1, pfile);
                if (strstr(psinfo.pr_fname, "Roboproc") != NULL) {
#endif
                    pRobo = atoi(dp->d_name);
                    break;
	        }
	    }
	}
        else
          break;
    }
    closedir(dirp);
            
    if (pRobo < 0) {
        exit(0);
    }

    /* Kill Roboproc... */
    kill (pRobo, SIGQUIT);
    sleep(3);

    if (((result = kill (pRobo, 0)) < 0) && (errno == ESRCH)) {
        exit(0);
    }

    kill (pRobo, SIGKILL);
    sleep(3);

    if (((result = kill (pRobo, 0)) < 0) && (errno == ESRCH)) {
        exit(0);
    }

    exit(1);
}
