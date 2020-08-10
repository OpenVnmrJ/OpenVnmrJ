/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "group.h"
#include "pvars.h"
#include "abort.h"
#include "cps.h"

#define MAXPATHL 128
#define MAXSTR 256
#define STDVAR 70

extern int newacq;
extern int acqiflag;
extern int dps_flag;
extern int dpsTimer;
extern int checkflag;
extern int nomessageflag;
extern int setupflag;
extern char  curexp[];
extern char  filepath[];
extern char  filexpath[];
extern char  fileRFpattern[];	/* path for obs & dec RF pattern file */
extern char  filegrad[];	/* path for Gradient file */
extern char  filexpan[];	/* path for Future Expansion file */
static int   erroropen = 0;
static FILE *errorfile;
static char  errorpath[MAXPATHL];
static int   cmdOpen = 0;
static FILE *cmdFile;
static char  cmdPath[MAXPATHL];
static int   filesOpen = 0;
static FILE *filesFile;
static char  filesPath[MAXPATHL];

extern int deliverMessageSuid(char *interface, char *message );
extern void closepipe2(int success);

void vnmremsg(const char *paramstring);
void vnmrmsg(const char *paramstring);
void release_console();


/*-----------------------------------------------------------
|
|   abort()/1
|   For Abnormal termination of the child process PSG
|	This routine is called, which deletes the files
|	created in acqqueue directory before the process is
|	terminated.
|
|				Author Greg Brissey  7/13/86
+-----------------------------------------------------------*/

void closeFiles()
{
    const char  *mess = "ABORT(): could not delete %s\n";
    char  tmpPath[MAXPATHL];

  /* delete file filexpath and filepath from acqqueue directory */
    if (strcmp(filepath,"") != 0)
    {
      if ( (unlink(filepath) == -1) && (errno != ENOENT) )
      {
        text_error(mess,filepath);
      }
    }
    if (strcmp(filexpath,"") != 0)
    {
      if ( (unlink(filexpath) == -1) && (errno != ENOENT) )
      {
        text_error(mess,filexpath);
      }
    }
    if (strcmp(fileRFpattern,"") != 0)
    {
      if ( (unlink(fileRFpattern) == -1) && (errno != ENOENT) )
      {
        text_error(mess,fileRFpattern);
      }
    }
    if (strcmp(filegrad,"") != 0)
    {
      if ( (unlink(filegrad) == -1) && (errno != ENOENT) )
      {
        text_error(mess,filegrad);
      }
    }
    if (strcmp(filexpan,"") != 0)
    {
      if ( (unlink(filexpan) == -1) && (errno != ENOENT) )
      {
        text_error(mess,filexpan);
      }
    }
    if (strcmp(filexpath,"") != 0)
    {
      strcpy(tmpPath,filexpath);
      strcat(tmpPath,".RTpars");
      if ( (unlink(tmpPath) == -1) && (errno != ENOENT) )
      {
        text_error(mess,tmpPath);
      }
    }
    if (strcmp(filexpath,"") != 0)
    {
      strcpy(tmpPath,filexpath);
      strcat(tmpPath,".Tables");
      if ( (unlink(tmpPath) == -1) && (errno != ENOENT) )
      {
        text_error(mess,tmpPath);
      }
    }
}

void psg_abort(int error)
{
    closeFiles();
    if (dpsTimer)
    {
      close_error(1);   /* 1 arg means failure/abort */
      exit(1);
    }
    text_error("P.S.G. Aborted.");
    if (newacq)
    {
      if (!acqiflag && !dps_flag && !checkflag)
        release_console();
    }
    close_error(1);     /* 1 arg means failure/abort */
    exit(error);
}

void text_message(const char *format, ...)
{
   va_list vargs;
   char  emessage[1024];

   if (dpsTimer || nomessageflag)
      return;
   va_start(vargs, format);
   vfprintf(stdout,format,vargs);
   va_end(vargs);
   if ( *(format+(strlen(format)-1)) != '\n')
      fprintf(stdout,"\n");
   va_start(vargs, format);
   vsprintf(emessage,format,vargs);
   va_end(vargs);
   if (newacq)
   {
     while (emessage[strlen(emessage)-1] == '\n')
        emessage[strlen(emessage)-1] = '\0';
     vnmrmsg(emessage);
   }
}

void warn_message(const char *format, ...)
{
   va_list vargs;
   char  emessage[1024];

   if (dpsTimer || nomessageflag)
      return;
   va_start(vargs, format);
   vfprintf(stdout,format,vargs);
   va_end(vargs);
   if ( *(format+(strlen(format)-1)) != '\n')
      fprintf(stdout,"\n");
   va_start(vargs, format);
   vsprintf(emessage,format,vargs);
   va_end(vargs);
   if (newacq)
   {
     while (emessage[strlen(emessage)-1] == '\n')
        emessage[strlen(emessage)-1] = '\0';
     vnmremsg(emessage);
   }
}

void abort_message(const char *format, ...)
{
   va_list vargs;
   char  emessage[1024];

   if (dpsTimer || nomessageflag)
   {
      closeFiles();
      close_error(1);       /* 1 arg means fail/abort */
      exit(1);
   }
   va_start(vargs, format);
   vfprintf(stdout,format,vargs);
   va_end(vargs);
   if ( *(format+(strlen(format)-1)) != '\n')
      fprintf(stdout,"\n");
   if (!erroropen)
   {
      if (P_getstring(GLOBAL, "userdir", errorpath, 1, MAXPATHL) >= 0)
      {
         strcat(errorpath,"/psg.error");
         if ( (errorfile=fopen(errorpath,"w+")) )
            erroropen = 1;
       }
   }
   if (erroropen)
   {
      va_start(vargs, format);
      vfprintf(errorfile,format,vargs);
      va_end(vargs);
      fprintf(errorfile,"P.S.G. Aborted....\n");
      fprintf(stdout,"P.S.G. Aborted....\n");
   }
   va_start(vargs, format);
   vsprintf(emessage,format,vargs);
   va_end(vargs);
   closeFiles();
   if (newacq)
   {
     while (emessage[strlen(emessage)-1] == '\n')
        emessage[strlen(emessage)-1] = '\0';
     vnmremsg(emessage);
     if (!acqiflag && !dps_flag && !checkflag)
       release_console();
   }
   close_error(1);    /* 1 arg means fail/abort */
   exit(1);
}

/*-----------------------------------------------------------
|
|   text_error()
|   Writes messages to a file named "psg.error" in the
|       vnmruser (env variable) directory.
|
+-----------------------------------------------------------*/
void text_error(const char *format, ...)
{
   va_list vargs;
   char  emessage[1024];

   if (nomessageflag)
      return;
   va_start(vargs, format);
   vfprintf(stdout,format,vargs);
   va_end(vargs);
   if ( *(format+(strlen(format)-1)) != '\n')
      fprintf(stdout,"\n");
   if (!erroropen)
   {
      if (P_getstring(GLOBAL, "userdir", errorpath, 1, MAXPATHL) >= 0)
      {
         strcat(errorpath,"/psg.error");
         if ( (errorfile=fopen(errorpath,"w+")) )
            erroropen = 1;
       }
   }
   if (erroropen)
   {
      va_start(vargs, format);
      vfprintf(errorfile,format,vargs);
      va_end(vargs);
   }
   if (newacq)
   {
     va_start(vargs, format);
     vsprintf(emessage,format,vargs);
     va_end(vargs);
     while (emessage[strlen(emessage)-1] == '\n')
        emessage[strlen(emessage)-1] = '\0';
     vnmremsg(emessage);
   }
}

/*-----------------------------------------------------------
|
|   close_error(int abortcode)
|   Closes the error message file named "psg.error" in the
|       vnmruser (env varaible) directory.
|
+-----------------------------------------------------------*/
void close_error(int abortcode)
{
  if (erroropen)
  {
    fclose(errorfile);
    erroropen = 0;
    chmod(errorpath,0660);
  }
  closepipe2(abortcode);
}

void setupPsgFile()
{
    if (acqiflag || (setupflag > 0) || dps_flag)	
    {
       /* disable writing a psgFile */
       filesOpen = -1;
    }
    else
    {
       /* initialize filesPath and remove old copy of psgFile */
       sprintf(filesPath,"%s/psgFile",curexp);
       if (access( filesPath, W_OK ) == 0)
          unlink(filesPath);
    }
}
/*-----------------------------------------------------------
|
|   putFile()
|   Writes filename to a file named "psgFile" in the
|       curexp directory.
|
+-----------------------------------------------------------*/
int putFile(const char *fname)
{
   if (filesOpen == 0)
   {
      if ( (filesFile=fopen(filesPath,"w+")) )
         filesOpen = 1;
   }
   if (filesOpen == 1)
      fprintf(filesFile,"%s\n",fname);
   return(0);
}

/*-----------------------------------------------------------
|
|   closePsgFile()
|   Closes the file named "psgFile" in the
|       curexp directory.
|
+-----------------------------------------------------------*/
int closePsgFile()
{
  if (filesOpen == 1)
  {
    fclose(filesFile);
    chmod(filesPath,0660);
  }
  filesOpen = -1; /* Do not allow more than one putFile */
  return(0);
}

/*-----------------------------------------------------------
|
|   putCmd()
|   Writes commands to a file named "psgCmd" in the
|       curexp directory.
|
+-----------------------------------------------------------*/
void putCmd(const char *format, ...)
{
   va_list vargs;

   if (acqiflag || (setupflag > 0) || (dps_flag && !checkflag) )	
      return;
   if (cmdOpen == 0)
   {
      if (P_getstring(GLOBAL, "curexp", cmdPath, 1, MAXPATHL) >= 0)
      {
         strcat(cmdPath,"/psgCmd");
         if ( (cmdFile=fopen(cmdPath,"w+")) )
            cmdOpen = 1;
       }
   }
   if (cmdOpen == 1)
   {
      va_start(vargs, format);
      vfprintf(cmdFile,format,vargs);
      va_end(vargs);
      if ( *(format+(strlen(format)-1)) != '\n')
         fprintf(cmdFile,"\n");
   }
}

/*-----------------------------------------------------------
|
|   closeCmd()
|   Closes the command file named "psgCmd" in the
|       curexp directory.
|
+-----------------------------------------------------------*/
int closeCmd()
{
  if (cmdOpen == 1)
  {
    fsync( fileno(cmdFile) );
    fclose(cmdFile);
    chmod(cmdPath,0660);
    if (strlen(cmdPath) > 8)
    {
       int fd;
       cmdPath[strlen(cmdPath)-7] = '\0';
       fd = open(cmdPath,O_RDONLY);
       fsync(fd);
       close(fd);
    }
  }
  cmdOpen = -1; /* Do not allow more than one psgCmd */
  closePsgFile();
  return(0);
}

/*-----------------------------------------------------------------
|	vnmrmsg()/1
|	Sends normal message to vnmr.
+------------------------------------------------------------------*/
void vnmrmsg(const char *paramstring)
{
   int stat;
   char	message[MAXSTR];
   char addr[MAXSTR];

   stat = -1;
   if (getparm("vnmraddr","string",GLOBAL,addr,MAXSTR))
   {
	text_error("vnmrmsg: cannot get Vnmr address.\n");
   }
   else if (strcmp(addr,"Autoproc"))
   {
        int i = 0;
   	sprintf(message,"write('line3',`%s`,'noReformat')\n",paramstring);
        while ( (i<MAXSTR) && (message[i] != '\0') )
        {
           if (((message[i] < ' ') || (message[i] > '~') ) &&
               (message[i] != '\n') )
           {
             message[i] = ' ';
           }
           i++;
        }
   	stat = deliverMessageSuid(addr,message);
	if (stat < 0)
	{
	   text_error("vnmrmsg: Error sending msg:%s.\n",paramstring);
	}
   }
}

/*-----------------------------------------------------------------
|	vnmremsg()/1
|	Sends error to vnmr error window.
+------------------------------------------------------------------*/
void vnmremsg(const char *paramstring)
{
   int stat __attribute__((unused));
   char	message[MAXSTR];
   char addr[MAXSTR];

   stat = -1;
   if (getparm("vnmraddr","string",GLOBAL,addr,MAXSTR))
   {
	text_error("vnmremsg: cannot get Vnmr address.\n");
   }
   else if (strcmp(addr,"Autoproc"))
   {
        int i = 0;
   	sprintf(message,"write('error',`%s`,'noReformat')\n",paramstring);
        while ( (i<MAXSTR) && (message[i] != '\0') )
        {
           if (((message[i] < ' ') || (message[i] > '~') ) &&
               (message[i] != '\n') )
           {
             message[i] = ' ';
           }
           i++;
        }
   	stat = deliverMessageSuid(addr,message);
/*
	if (stat < 0)
	{
	   sprintf(addr,"vnmremsg: Error sending msg:%s.\n",paramstring);
           text_error(addr);
	}
 */
   }
   
}

/*-----------------------------------------------------------------
|	release_console()/1
|	Sends release console command to Vnmr
+------------------------------------------------------------------*/
void release_console()
{
   int stat;
   char	message[STDVAR];
   char addr[MAXSTR];

   stat = -1;
   if (getparm("vnmraddr","string",GLOBAL,addr,MAXSTR))
   {
	text_error("vnmremsg: cannot get Vnmr address.\n");
   }
   else if (strcmp(addr,"Autoproc"))
   {
   	sprintf(message,"releaseConsole\n");
   	stat = deliverMessageSuid(addr,message);
	if (stat < 0)
	{
	   text_error("vnmremsg: Error sending release console msg.\n");
	}
   }
   
}
