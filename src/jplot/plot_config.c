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

#include "PlotConfig.h"
#include "PlotFile.h"
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/signal.h>

static char  vcmd[512];
static char  tmp[512];
static char  userPlotDir[256];
static char  sysPlotDir[256];
static char  *userDir = NULL;
static char  *sysDir = NULL;
static int   prog_id = -1;
static int   inPipe = -1;
static int   outPipe = -1;
static int   outSocket = -1;
static jobject  main_obj;

JNIEXPORT jint JNICALL Java_PlotConfig_makeImage
 (JNIEnv *env, jclass  jcl, jobject obj, jint id, jint width, jint height)
{
	jint  ret;
	FILE  *fd;
	int   k, ok;
	char  ff[125];

	main_obj = obj;
	ret = id;
	system("rm *ppm");
	sprintf(vcmd, "pstopnm -xsize %d -ysize %d dcon.ps", width, height);
	system(vcmd);
	k = 0;
	ok = 0;
	while (k < 10)
	{
	    if ((fd = fopen("dcon001.ppm", "r")) != NULL)
	    {
		ok = 1;
		break;
	    }
	    k++;
	    sleep(1);
	}
	if (ok == 0)
	{
	    ret = 0;
	    return ret;
	}
	fclose(fd);
	sprintf(ff, "dcon%d.gif", id);
	sprintf(vcmd, "rm -rf %s", ff);
	system(vcmd);
	sprintf(vcmd, "ppmtogif dcon001.ppm > %s", ff);
	system(vcmd);
        k = 0;
        ok = 0;
        while (k < 10)
        {
            if ((fd = fopen(ff, "r")) != NULL)
            {
                ok = 1;
                break;
            }
	    k++;
	    sleep(1);
        }
        if (ok == 0)
        {
            ret = 0;
            return ret;
        }
	fclose(fd);
	sprintf(vcmd, "ls -l  %s", ff);
	system(vcmd);
	return   ret;
}

JNIEXPORT void JNICALL Java_PlotConfig_loadImage
  (JNIEnv *env, jclass  jcl)
{
}

JNIEXPORT jstring JNICALL Java_PlotConfig_getEnv
  (JNIEnv *env, jclass jcl, jstring name)
{
	const char  *cstr;
	char	    *data;
	jstring     jret;

	cstr = (*env)->GetStringUTFChars(env, name, NULL);
	if ((data = (char *)getenv(cstr)) != NULL)
	{
	    jret = (*env)->NewStringUTF(env, data);
	}
	else
	    jret = NULL;
	(*env)->ReleaseStringUTFChars(env, name, cstr);
	return (jret);
}


typedef  struct _file_rec {
        int     id;
        char    *name;
        struct _file_rec  *next;
        } FILE_REC;

static  FILE_REC   *def_list = NULL;



JNIEXPORT jstring JNICALL Java_PlotFile_list_1templates
 (JNIEnv *env, jclass  jcl, jint order)
{
        FILE_REC       		*plist, *clist;
        static DIR     		*dirp = NULL;
        struct dirent   	*dp;
	int			addFlag, m;
	char			*vdata;
	static jstring         	jstr = NULL;
	static jint         	k = 1;
	const char 		*cstr;
	FILE  			*fd;

	if (jstr != NULL)
	{
	    cstr = (*env)->GetStringUTFChars(env, jstr, NULL);
	    (*env)->ReleaseStringUTFChars(env, jstr, cstr);
	}

	if (order == 1 && userDir != NULL)
	{
	    dirp = opendir(userPlotDir);
	    k = 1;
	}
	else if (order == 2 && sysDir != NULL)
	{
	    dirp = opendir(sysPlotDir);
	    k = 2;
	}
	if (order == 9)
	{
	   if ( k == 1)
	        sprintf(tmp, "%s/.default", userPlotDir);
	   else
	         sprintf(tmp, "%s/.default", sysPlotDir);
           if ((fd = fopen(tmp, "r")) == NULL)
	         return(NULL);
	   while (fgets(tmp, 510, fd) != NULL)
	   {
	     	m = strlen(tmp);
		vdata = tmp + m - 1;
		while (m > 0)
		{
		    if (*vdata == ' ' || *vdata == '\n')
			*vdata = '\0';
		    else
		        break;
		    m--;
		    vdata--;
		}
		vdata = tmp;
		while (*vdata == ' ' || *vdata == '\t')
		    vdata++;
		m = strlen(vdata);
		if (m > 0)
		{
	    	    jstr = (*env)->NewStringUTF(env, vdata);
            	    return(jstr);
		}
	   }
	   return(NULL);
	}

        if(dirp == NULL)
	{
	    jstr = (*env)->NewStringUTF(env, "");
            return(jstr);
	}

	addFlag = 0;
	while (1)
	{
	   dp = readdir(dirp);
	   if (dp == NULL)
		break;
	   if (dp->d_name[0] == '.')
                continue;
	   if ( !strcmp(dp->d_name, "menu"))
                continue;
	   plist = def_list;
	   addFlag = 1;
	   if (order == 1)
	   {
		while (plist != NULL)
		{
		    if (plist->next == NULL)
			break;
		    plist = plist->next;
		}
	   }
	   else
	   {
		while (plist != NULL)
		{
		    if (strcmp(plist->name, dp->d_name) == 0)
		    {
			addFlag = 0;
			break;
		    }
		    if (plist->next == NULL)
			break;
		    plist = plist->next;
		}
	   }
	   if (addFlag)
	   {
		clist = (FILE_REC *) malloc( sizeof(FILE_REC));
		if ( plist == NULL)
		    def_list = clist;
		else
		    plist->next = clist;
		clist->next = NULL;
		clist->name = (char *) malloc((int)strlen(dp->d_name) + 2);
		strcpy(clist->name, dp->d_name);
		break;
	    }
	}
	if (dp == NULL)
	{
            closedir(dirp);
	    dirp = NULL;
	    jstr = (*env)->NewStringUTF(env, "");
            return(jstr);
	}
	jstr = (*env)->NewStringUTF(env, dp->d_name);
        return(jstr);
}

JNIEXPORT jint JNICALL Java_PlotFile_access_1templates
  (JNIEnv *env, jclass jcl, jstring fname, jint mode)
{
	const char 	*cstr;
	struct stat     f_stat;
	jint		jret;
	int		fmode;

	if (mode == 1)
	    fmode = R_OK;
	if (mode == 2)
	    fmode = W_OK;
	cstr = (*env)->GetStringUTFChars(env, fname, NULL);
	if (cstr[0] != '/')
	    sprintf(tmp, "%s/%s", userPlotDir, cstr);
	jret = -2;  /* assume not exist  */
	if (stat(tmp, &f_stat) == 0)
	{
	    if (access(tmp, fmode) == 0)
		jret = 0;
	    else
		jret = -1;
	}
	else if (cstr[0] != '/')
	{
	    sprintf(tmp, "%s/%s", sysPlotDir, cstr);
	    if (stat(tmp, &f_stat) == 0)
	    {
	    	if (access(tmp, fmode) == 0)
		    jret = 1;
		else
		    jret = -1;
	    }
	}
	if (jret >= 0)
	{
	    if (cstr[0] == '/')
		jret = 2;   /* absolute path  */
	}
	(*env)->ReleaseStringUTFChars(env, fname, cstr);
	return(jret);
}

JNIEXPORT jint JNICALL Java_PlotFile_remove_1templates
  (JNIEnv *env, jclass jcl, jstring fname)
{
	const char 	*cstr;
	struct stat     f_stat;
	jint		jret;

	cstr = (*env)->GetStringUTFChars(env, fname, NULL);
	if (cstr[0] != '/')
	    sprintf(tmp, "%s/%s", userPlotDir, cstr);
	jret = 0;
	if (stat(tmp, &f_stat) == 0)
	{
	    if (access(tmp, W_OK) != 0)
		jret = 1;
	}
	else if (cstr[0] != '/')
	{
	    sprintf(tmp, "%s/%s", sysPlotDir, cstr);
	    if (stat(tmp, &f_stat) == 0)
	    {
	    	if (access(tmp, W_OK) != 0)
		    jret = 1;
	    }
	    else
		jret = 2;
	}
	(*env)->ReleaseStringUTFChars(env, fname, cstr);
	if (jret == 0)
	    unlink(tmp);
	return(jret);
}


JNIEXPORT void JNICALL Java_PlotConfig_remove_1file
  (JNIEnv *env, jclass jcl, jstring fname)
{
	const char 	*cfile;

	cfile = (*env)->GetStringUTFChars(env, fname, NULL);
	unlink(cfile);
	(*env)->ReleaseStringUTFChars(env, fname, cfile);
}


JNIEXPORT void JNICALL Java_PlotConfig_initEnv
  (JNIEnv *env, jclass jcl)
{
	char	*vdir;

	if ((vdir = (char *)getenv("vnmruser")) != NULL)
	{
	    userDir = (char *) malloc(strlen(vdir) + 2);
	    strcpy(userDir, vdir);
	    sprintf(userPlotDir, "%s/templates/plot", vdir);
	}
	if ((vdir = (char *)getenv("vnmrsystem")) != NULL)
	{
	    sysDir = (char *) malloc(strlen(vdir) + 2);
	    strcpy(sysDir, vdir);
	    sprintf(sysPlotDir, "%s/user_templates/plot", vdir);
	}
	prog_id = getpid();

}


JNIEXPORT void JNICALL Java_PlotConfig_initArg
  (JNIEnv *env, jclass jcl, jstring arg)
{
	const char 	*cstr;
	int		pid;

	cstr = (*env)->GetStringUTFChars(env, arg, NULL);
	if (cstr[0] == '-')
	{
	    switch (cstr[1])
            {
		case 'I': 
			 inPipe = atoi(&cstr[2]);
			 set_input_handler();
			 break;
		case 'O': 
			 outPipe = atoi(&cstr[2]);
			 break;
		case 'h':
			 initVnmrComm(&cstr[2]);
			 outSocket = 1;
			pid = getpid();
			sprintf(vcmd, "jplot('-proc', %d)\n", pid);
        		sendToVnmr(vcmd);
			break;
	    }
	}
	(*env)->ReleaseStringUTFChars(env, arg, cstr);
}

static
char *wait_4_vnmr_respond()
{
        static  char    rdata[256];
	int	k;

	rdata[0] = '\0';
	if (inPipe <= 0)
	    return(rdata);
	if((k = read(inPipe, rdata, 255)) <= 0)
        {
           if ( (k == -1) && (errno == EINTR))
	   {
              return(rdata);
	   }
	}
        if (k > 0)
        {
            rdata[k] = '\0';
            if (rdata[k-1] == '\n')
                 rdata[k-1] = '\0';
        }
        return(rdata);
}


JNIEXPORT jstring JNICALL Java_PlotConfig_talk_1to_1vnmr
  (JNIEnv *env, jclass jcl, jstring mess, jstring arg, jstring arg2, jint orient, jint wait)
{
	const char 	*cstr, *param, *param2;
	static jstring	jret = NULL;
	int		toDo;
	char		*ans, plotter[12];

	cstr = (*env)->GetStringUTFChars(env, mess, NULL);
	param = (*env)->GetStringUTFChars(env, arg, NULL);
	param2 = (*env)->GetStringUTFChars(env, arg2, NULL);
	toDo = 0;
	if (strcmp(cstr, "load") == 0)
	    toDo = 1;	
	if (outSocket)
	{
	    if (orient == 1)
		strcpy(plotter, "PS_A");
	    else
		strcpy(plotter, "PS_AR");
	    sprintf(vcmd, "jplot('-macro', %d, %d, '%s', '%s', '%s', %d)\n", toDo, prog_id, param, param2, plotter, wait);
            sendToVnmr(vcmd);
	    if (wait > 0)
	       ans = wait_4_vnmr_respond();
	    else
	    {
		strcpy(tmp, "null");
		ans = tmp;
	    }
	}
	else if (outPipe > 0)
	{
/**
	    sprintf(vcmd, "3 ,\"%s\" ,\"-macro\" ,\"%d\" ,\"%d\"%c",
                         prog_name, vcmd, proc_pid, ESC);
            write (outPipe, vcmd, strlen(vcmd));
            sync();
**/
	}
	(*env)->ReleaseStringUTFChars(env, mess, cstr);
	(*env)->ReleaseStringUTFChars(env, arg, param);
	(*env)->ReleaseStringUTFChars(env, arg2, param2);
	jret = (*env)->NewStringUTF(env, ans);
        return(jret);
}

JNIEXPORT jstring JNICALL Java_PlotConfig_get_1tmp_1name
  (JNIEnv *env, jclass jcl, jstring arg)
{
	const char 	*cstr;
	static jstring	jret = NULL;

	cstr = (*env)->GetStringUTFChars(env, arg, NULL);
	sprintf(tmp, "%s", tempnam("/tmp", cstr));
	jret = (*env)->NewStringUTF(env, tmp);
	(*env)->ReleaseStringUTFChars(env, arg, cstr);
	return(jret);
}
   
void read_vnmr_input(sig)
int	sig;
{
        char    rdata[256];
	FILE    *fd;

	if (sig != SIGUSR1)
	    return;
	sprintf(tmp, "%s/.cmd", userPlotDir);
	if ((fd = fopen(tmp, "r")) == NULL)
	   return;
	while (fgets(rdata, 120, fd) != NULL)
	{
	   if (strncmp(rdata, "exit", 4) == 0)
	   {
		fclose(fd);
		exit(0);
	   }
	   if (strncmp(rdata, "close", 5) == 0)
	   {
		fclose(fd);
		exit(0);
	   }
	}
}


set_input_handler()
{
	struct sigaction intserv;
	sigset_t         qmask;

	sigemptyset( &qmask );
	sigaddset( &qmask, SIGUSR1 );
    intserv.sa_handler = read_vnmr_input;
    intserv.sa_mask    = qmask;
    intserv.sa_flags   = 0;
    sigaction(SIGUSR1,&intserv,0L);
}

JNIEXPORT jint JNICALL Java_PlotConfig_accessFile
  (JNIEnv *env, jclass jcl, jstring fname, jint mode)
{
	const char 	*cstr;
	struct stat     f_stat;
	jint		jret;
	int		fmode;

	if (mode == 1)
	    fmode = R_OK;
	if (mode == 2)
	    fmode = W_OK;
	cstr = (*env)->GetStringUTFChars(env, fname, NULL);
	if (cstr[0] != '/')
	    sprintf(tmp, "%s/%s", userPlotDir, cstr);
	jret = -2;  /* assume not exist  */
	if (stat(tmp, &f_stat) == 0)
	{
	    if (access(tmp, fmode) == 0)
		jret = 1;
	    else
		jret = -1;
	}
	else if (cstr[0] != '/')
	{
	    sprintf(tmp, "%s/%s", sysPlotDir, cstr);
	    if (stat(tmp, &f_stat) == 0)
	    {
	    	if (access(tmp, fmode) == 0)
		    jret = 2;
		else
		    jret = -1;
	    }
	}
	if (jret >= 0)
	{
	    if (cstr[0] == '/')
		jret = 3;   /* absolute path  */
	}
	(*env)->ReleaseStringUTFChars(env, fname, cstr);
	return(jret);
}


