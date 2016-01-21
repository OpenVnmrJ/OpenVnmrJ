/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef TESTING
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <string.h>
#include <sys/types.h>

#include "asm.h"
#ifdef VNMRJ
#include "acquisition.h"
#include "group.h"
#include "pvars.h"
extern int is_acqproc_active();
#endif

/* don't change these defines, Greg B. */
#define OK 0
#define ERROR -1
#define ENDOFFILE -2
#define LKRETRY 10
#define MAXSTR  256
#define MAXPATHL  128

extern void unlockit(const char *lockPath, const char *idPath);
extern int lockit(const char *lockPath, const char *idPath, time_t timeout);

struct _info_map_ {
		int mapped;		/* true if autoinfo has been read for mapping */
		char* userdir;
		char* datadir;
		char* status;
		char* eoe;
	  };

static char UserDir[12] = { 'U', 'S', 'E', 'R', 'D', 'I', 'R', ':', '\0' };
static char DataDir[12] = { 'D', 'A', 'T', 'A', ':', '\0' };
static char Status[12] =  { 'S', 'T', 'A', 'T', 'U', 'S', ':',  '\0' };
static char EoE[12]    =  { '-', '-', '-', '-', '-', '-', '-', '-', '-', '-', '\0' };
static struct _info_map_ infomap = { 0, UserDir, DataDir, Status, EoE };

static char  data_key[1024];  /* used in asmStripAutodir_dotFid */

int read_info_file(char *autodir);
int lockfile(char *lockfile);
int unlockfile(char *lockfile);
/*------------------------------------------------------------------------

	Do Not put anything to this file that is dependent on other files.
	Because this file is used by Vnmr, Acqproc and Autoproc.

+------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
|
|   Modified   Author     Purpose
|   --------   ------     -------
|   3/17/88   Greg B.    1.  Modified update_sample_info to close appropriate
|			      files in error returns that were missing before.
|
|   11/10/89   Greg B.   1.  Modified read_sample_info(), write_sample_info(),
|			     update_sample_info(). Improved Error reporting
|			     of read , write & update routines.
|
|   9/3/96    Greg B.    1.  Complete Overhaul of all function to handle the new variable length
|			     entries now possible 
+------------------------------------------------------------------------*/

/*--------------------------------------------------------------------
| read_sample_info(stream,s)/2
|	reads one entry from the enterfile
+--------------------------------------------------------------------*/
int read_sample_info(FILE *stream, struct sample_info *s)
{
    char eolch;
    char textline[256];
    char *prompt,*text;
    int stat,line,j;

    j=0;
    line = 0;
    while ( (stat = fscanf(stream,"%[^\n]%c", textline, &eolch)) != EOF )
    {
       j++;
       if ( stat != 2 )
       {
	   fprintf(stderr,"read_sample_info: automation file format error, line %d\n",j);
           return(ERROR);
       }

       /* extract the string tokens, Prompt and Text */
       prompt = (char*) strtok(textline," ");
       text = (char*) strtok(NULL,"\n");

       if (strcmp(prompt,":") == 0)
          continue;				/* skip comments */

       if (strncmp(prompt,infomap.eoe,10) == 0) /* end of entry */
       {
	   strncpy(s->prompt_entry[line].eprompt,prompt,MAX_PROMPT_LEN);	/* 11 char */
	   s->prompt_entry[line].eprompt[MAX_PROMPT_LEN-1] = '\0'; /* be sure null term string */
	   /* build up a 80 char EOE string */
	   strcpy(s->prompt_entry[line].etext,s->prompt_entry[line].eprompt);	/* 11 char */
	   strcat(s->prompt_entry[line].etext,s->prompt_entry[line].eprompt);	/* 11 char */
	   strcat(s->prompt_entry[line].etext,s->prompt_entry[line].eprompt);	/* 11 char */
	   strcat(s->prompt_entry[line].etext,s->prompt_entry[line].eprompt);	/* 11 char */
	   strcat(s->prompt_entry[line].etext,s->prompt_entry[line].eprompt);	/* 11 char */
	   strcat(s->prompt_entry[line].etext,s->prompt_entry[line].eprompt);	/* 11 char */
	   strcat(s->prompt_entry[line].etext,s->prompt_entry[line].eprompt);	/* 11 char */
	   strcat(s->prompt_entry[line].etext,s->prompt_entry[line].eprompt);	/* 11 char */
	   break;	/* yes then stop */
       }
       else
       {
          if (prompt != NULL)
	  {
	    strncpy(s->prompt_entry[line].eprompt,prompt,MAX_PROMPT_LEN);	/* 11 char */
	    s->prompt_entry[line].eprompt[MAX_PROMPT_LEN-1] = '\0';	/* just to be sure null terminated string */
	  }
	  else
	    s->prompt_entry[line].eprompt[0] = '\0';	

          if (text != NULL)
	  {
	     strncpy(s->prompt_entry[line].etext,text,MAX_TEXT_LEN);	/* 128 char */
	     s->prompt_entry[line].etext[MAX_TEXT_LEN-1] = '\0';
	  }
	  else
	    s->prompt_entry[line].etext[0] = '\0';	

	  line++;
       }
    }
    if (stat == EOF )
	return(ENDOFFILE); 
    else
        return (OK);
}

/*--------------------------------------------------------------------
| write_sample_info(stream,s)/2
|	writes one entry to the specified stream (sampleinfo file,stdout,etc.)
+--------------------------------------------------------------------*/
int write_sample_info(FILE *stream, struct sample_info *s)
{
    int line;
    
    line=0;
    while (1)
    {
      if ( strncmp(s->prompt_entry[line].eprompt,infomap.eoe,10) != 0 )
      {
         if ( 
	      fprintf(stream,
	        "%11s %s\n", s->prompt_entry[line].eprompt,  s->prompt_entry[line].etext ) == EOF
            )
              return(ERROR);
      }
      else
      {
         if ( 
	      fprintf(stream,"%s\n", s->prompt_entry[line].etext ) == EOF
            )
              return(ERROR);
	 break;
      }
      line++;
    }
    return (OK);
}

/*--------------------------------------------------------------------
| get_sample_info(s,keyword,info,size,entryline)/5
|	get text entry for the key,  also optional returns entry index
+--------------------------------------------------------------------*/
int get_sample_info(struct sample_info *s, const char *keyword, char *info, int size, int *entry)
{
    int line;
    const char *mapkey;
    
    
    if (strcmp(keyword,"USERDIR:") == 0)
    {
	mapkey = infomap.userdir;
    }
    else if (strcmp(keyword,"DATA:") == 0)
    {
	mapkey = infomap.datadir;
    }
    else if (strcmp(keyword,"STATUS:") == 0)
    {
	mapkey = infomap.status;
    }
    else if (strcmp(keyword,"EOE:") == 0)
    {
	mapkey = infomap.eoe;
	/*
	strncpy(info,infomap.eoe,size);
        info[size-1] = '\0';
        if (entry != NULL)
	   *entry = -1;
	return(strlen(infomap.eoe));
	*/
    }
    else
	mapkey = keyword;

    line=0;
    while (line < MAX_ENTRIES)
    {
      if (strncmp(mapkey,s->prompt_entry[line].eprompt,10) == 0)
      {
	  strncpy(info,s->prompt_entry[line].etext,size);
          info[size-1] = '\0';
        if (entry != NULL)
	   *entry = line;
	  return(strlen(s->prompt_entry[line].etext));
      }
      else if (strncmp(s->prompt_entry[line].eprompt,infomap.eoe,10) == 0)
      {
	   break;
      }
      line++;
    }
    strcpy(info,"");  /* null string */ 
    return (0);
}


/*---------------------------------------------------------------------------
|
|  asmStripAutodir_dotFid - 
|	Takes a filepath and strips the autodir path and '.fid' extyension off
|     a precusor to call update_sample_info() for updated the status using the
|     data field as a unique locator.
|
|				Author:  Greg Brissey 9/24/96
+--------------------------------------------------------------------------*/
char *asmStripAutodir_dotFid(char *autodir, char *datapath)
{
   char *ptr1;

   /* strip out the autodir path and the '.fid' extension */
   ptr1 = strstr(datapath,autodir);
   if (ptr1 != NULL)
     strcpy(data_key,ptr1 + strlen(autodir) + 1);
   else
     strcpy(data_key,datapath);

   /* printf("datafile: '%s', autoname: '%s'\n",datapath,data_key); */
   ptr1 = strstr(data_key,".fid");
   if (ptr1 != NULL)
      *ptr1 = '\0';
   /* printf("asm keyname: '%s'\n",data_key); */
   return(data_key);
}

/*---------------------------------------------------------------------------
|  update_sample_info()/5
|
|  update_sample_info - 
|	Takes a automation directory and filename of sample_info structure (e.g., doneQ,enterq,sampleinfo)
|	Searches through the file for a match between the match_target value
|	  and the match_value;
|       Upon finding the match it then updates the update_target with the
|	  update_value;
|       For example: You want to update the doneQ  'STATUS' entry to Complete,
|		     for the experiment inwhich the DATA entry matches with
|		     '/vnmr/auto/greg.1201' (i.e. DATA is a unique identifier).
|       The call would be:
|	  update_sample_info("systemdir/auto","doneQ",
|			     "DATA:","/vnmr/auto/greg.1201",
|			     "STATUS","Complete");
|
|				Author:  Greg Brissey 1/28/88
+--------------------------------------------------------------------------*/
int update_sample_info(char *autodir, char *filename, char *match_target,
                       char *match_value, char *update_target, char *update_value)
{
    FILE *update_file,*tmp_file;
    char filepath[128],value[MAX_TEXT_LEN];
    struct sample_info	sample_entry;
    int stat,entryline,foundit;
    
    strncpy(filepath,autodir,110);
    strcat(filepath,"/");
    strcat(filepath,filename);

    /* if autoinfo mapping file hase not be read do so now */
    if (infomap.mapped != 1)		/* true if autoinfo has been read for mapping */
    {
	read_info_file(autodir);
    }


    if (lockfile(filepath) == ERROR) /* lock file for excusive use */
    {
        fprintf(stderr,"update_sample_info: could not lock '%s' file.\n",filepath);
    }
 
    update_file = fopen(filepath,"r");
    if (update_file == NULL)  /* does file exist? */
    {
        unlockfile(filepath);   /* unlock file */
        fprintf(stderr,"update_sample_info: '%s' file is not present for reading.\n",
			filepath);
        return(ERROR);
    }

    tmp_file = fopen("/tmp/update_sample_info.tmp","w");
    if (tmp_file == NULL)  /* does file exist? */
    {
        unlockfile(filepath);   /* unlock file */
        fclose(update_file);
        fprintf(stderr,
	    "update_sample_info: temp file '/tmp/update_sample_info.tmp' could not be created.\n");
        return(ERROR);
    }
 
    /* read through the the file writing to a tmp file as changes are made */
    foundit = 0;
    while ( (stat = read_sample_info(update_file,&sample_entry)) != ENDOFFILE)
    {
	if (stat == ERROR)
	{
           unlockfile(filepath);   /* unlock file */
    	   fclose(update_file);
    	   fclose(tmp_file);
           fprintf(stderr,
	    "update_sample_info: read error on file '%s'.\n",filepath);
           return(ERROR);
	}

        /* Once found and changed we can skip all this */
        if (!foundit)
        {
	  get_sample_info(&sample_entry,match_target,value,MAX_TEXT_LEN,&entryline);
          if (strncmp(value,match_value,MAX_TEXT_LEN) == 0)
          {
	     get_sample_info(&sample_entry,update_target,value,MAX_TEXT_LEN,&entryline);

	     /* If updating STATUS and STATUS: Shimming, then remove this entry */
             if ( (strcmp(infomap.status,update_target) == 0) && 
	          ( (strcmp(update_value,"Complete") == 0) || (strcmp(update_value,"Error") == 0)) &&
		  (strcmp(sample_entry.prompt_entry[entryline].etext,"Shimming") == 0) )
             {
		/* elimenate Shimming Experiment from doneQ */
	        foundit = 1;
		continue;	/* don't update, don't write it out */
             }

	     strncpy(sample_entry.prompt_entry[entryline].etext,update_value,MAX_TEXT_LEN);
	     sample_entry.prompt_entry[entryline].etext[MAX_TEXT_LEN-1] = '\0';
	     foundit = 1;
          }
        }

	if (write_sample_info(tmp_file,&sample_entry) == ERROR)
	{
           unlockfile(filepath);   /* unlock file */
    	   fclose(update_file);
    	   fclose(tmp_file);
           fprintf(stderr,
	    "update_sample_info: write error to temp file '/tmp/update_sample_info.tmp'.\n");
           return(ERROR);
	}
    }

    fclose(update_file);
    fclose(tmp_file);

    /* Changes are complete in the tmp file, now copy the tmp file into the orginal file */
    tmp_file = fopen("/tmp/update_sample_info.tmp","r");
    if (tmp_file == NULL)  /* does file exist? */
    {
        unlockfile(filepath);   /* unlock file */
        fprintf(stderr,
          "update_sample_info: temp file '/tmp/update_sample_info.tmp' could not be opened to be read.\n");
        return(ERROR);
    }

    update_file = fopen(filepath,"w");
    if (update_file == NULL)  /* does file exist? */
    {
        unlockfile(filepath);   /* unlock file */
        fclose(tmp_file);
        fprintf(stderr,"update_sample_info: '%s' file is not present for writing.\n",
			filepath);
        return(ERROR);
    }

    while ( ( stat = read_sample_info(tmp_file,&sample_entry)) != ENDOFFILE)
    {
	if (stat == ERROR)
	{
           unlockfile(filepath);   /* unlock file */
    	   fclose(update_file);
    	   fclose(tmp_file);
           fprintf(stderr,
	    "update_sample_info: read error on file '/tmp/update_sample_info.tmp'.\n");
           return(ERROR);
	}
	if (write_sample_info(update_file,&sample_entry) == ERROR)
	{
           unlockfile(filepath);   /* unlock file */
    	   fclose(update_file);
    	   fclose(tmp_file);
           fprintf(stderr,
	    "update_sample_info: write error to file '%s'.\n",filepath);
           return(ERROR);
	}
    }

    fclose(update_file);
    fclose(tmp_file);

    unlockfile(filepath);   /* unlock file */
    return(OK); 
}

/*--------------------------------------------------------------------------
|
|	lockfile(name)
|
|	This function addes the '.lock' to name passed and attempts to
|       exclusively create it. If already present then it will retry up to
|	LKRETRY times waiting 1 sec inbetween attempts.
|
|				Author:  Greg Brissey 1/28/88
+--------------------------------------------------------------------------*/
int lockfile(char *lockfile)
{
    char lock[1024];
    int lockfd,iter;

    strcpy(lock,lockfile);
    strcat(lock,".lock");

/*  Use the O_EXCL bit to insure exclusive access.  */
 
    for (iter = 0; iter < LKRETRY; iter++)
    {
	lockfd = open( lock, O_CREAT | O_EXCL, 0666 );
	if (lockfd >= 0) 
	{
	    close(lockfd);
	    return( OK );               /* Got it */
	}
	sleep(1);
    }
    return(ERROR);
}
/*--------------------------------------------------------------------------
|
|	unlockfile(name)
|
|	This function removes the '.lock' file generated by lockfile().
|
|				Author:  Greg Brissey 1/28/88
+--------------------------------------------------------------------------*/
int unlockfile(char *lockfile)
{
    char lock[1024];
 
    strcpy(lock,lockfile);
    strcat(lock,".lock");

    if (unlink(lock) != 0)	/* remove lock file */
    {
	fprintf(stderr,"unlockfile: could not remove lock file:'%s'\n",lock);
	return(ERROR);
    }
    return(OK);
}


/*--------------------------------------------------------------------------
|
|	entryQ_length(autodir,filename)
|
|	This function reads the file and returns the length of one automation Entry
|
|				Author:  Greg Brissey 9/3/96
+--------------------------------------------------------------------------*/
int entryQ_length(char *autodir, char *filename)
{
    FILE *entryQ_file;
    char filepath[128];
    char textline[80];
    char eolch;
    int stat,lines;

    /* if autoinfo mapping file hase not be read do so now */
    if (infomap.mapped != 1)		/* true if autoinfo has been read for mapping */
    {
	read_info_file(autodir);
    }

    if ( filename[0] != '/' )  /* if filename absolute path then don't use autodir */
    {
      strncpy(filepath,autodir,110);
      strcat(filepath,"/");
      strcat(filepath,filename);
    }
    else
    {
      strcpy(filepath,filename);
    }

    entryQ_file = fopen(filepath,"r");
    if (entryQ_file == NULL)  /* does file exist? */
    {
        return(ERROR);
    }

    lines=0;
    while ( (stat = fscanf(entryQ_file,"%[^\n]%c", textline, &eolch)) != EOF )
    {
       lines++;
       if ( stat != 2 )
       {
	   fprintf(stderr,"entryQ_length: '%s' file format error, line %d\n",filepath,lines);
	   fclose(entryQ_file);
           return(ERROR);
       }

       if (strncmp(textline,infomap.eoe,10) == 0)
       {
	   fclose(entryQ_file);
	   return(lines);
       }
    }

    fclose(entryQ_file);
    fprintf(stderr,"Warning: No End-of-Entry Marker Found\n");
    return(lines);
}

/*--------------------------------------------------------------------------
|
|	read_info_file(basepath)
|
|	This function reads the autoinfo file to remap the KeyWord used in the
|	Automation Entry/Done Queue.
|
|				Author:  Greg Brissey 9/3/96
+--------------------------------------------------------------------------*/
int read_info_file(char *autodir)
{
    FILE *info_file;
    char mappath[128];
    char textline[80];
    char eolch;
    int  j;
    int stat;
    char *keyword,*mapword;
    
    strncpy(mappath,autodir,110);
    strcat(mappath,"/autoinfo");
   
    info_file = fopen(mappath,"r");
    if (info_file == NULL)  /* does file exist? */
    {
	infomap.mapped = 1;		/* setting will be defaults */
        return(OK);
    }

    stat = 1;
    j=0;
    /* while ( stat != EOF ) */
    while ( (stat = fscanf(info_file,"%[^\n]%c", textline, &eolch)) != EOF )
    {
       /* stat = fscanf(info_file,"%[^\n]%c", textline, &eolch); */
       j++;
       if ( stat != 2 )
       {
	   fprintf(stderr,"read_info_file: autoinfo file format error, line %d\n",j);
           fclose(info_file);
           return(-1);
       }

       /* printf("textline: '%s' \n",textline); */

       /* extract the string tokens, Key Word and Mapped Word */
       keyword = (char*) strtok(textline," ");
       mapword = (char*) strtok(NULL," ");
       /* printf("keyword: '%s' mapped word: '%s'\n",keyword,mapword); */

       if ( strcmp(keyword,":") != 0)	/* Not a comment */
       { 

          if (strcmp(keyword,"USERDIR:") == 0)
          {
	      strncpy(UserDir,mapword,11);
	      UserDir[11] = '\0'; 
          }
          else if (strcmp(keyword,"DATA:") == 0)
          {
	      strncpy(DataDir,mapword,11);
	      DataDir[11] = '\0'; 
          }
          else if (strcmp(keyword,"STATUS:") == 0)
          {
	      strncpy(Status,mapword,11);
	      Status[11] = '\0'; 
          }
          else if (strcmp(keyword,"EOE:") == 0)
          {
	      strncpy(EoE,mapword,11);
	      EoE[11] = '\0'; 
          }
      }
    }

    infomap.mapped = 1;		/* Key Word have been remapped */
    fclose(info_file);
    return(0);
}

#ifdef DBXTESTING

/* cc -g -xF -xsb -DDBXTESTING asmfuncs.c */
main()
{
    int len;
    struct sample_info      s;
    SAMPLE_INFO ns;
    FILE *info_file;
    char info[120];

    read_info_file(".");
    len = entryQ_length(".","GregAutoEntry");
    printf("len: %d\n",len);
    len = entryQ_length(".","/vxwks/greg/inova/nautoproc/GregAutoEntry");
    printf("len: %d\n",len);
     info_file = fopen("./GregAutoEntry","r");
    read_sample_info(info_file,&ns);
    fclose(info_file);
     info_file = fopen("./tmpentry","w");
    write_sample_info(info_file,&ns);
    fclose(info_file);
    len = get_sample_info (&ns,"DATA:",info,120,NULL);
    printf("DATA: '%s', len: %d\n",info,len);
    len = get_sample_info (&ns,"USERDIR:",info,120,NULL);
    printf("USERDIR: '%s', len: %d\n",info,len);
    len = get_sample_info (&ns,"STATUS:",info,120,NULL);
    printf("STATUS: '%s', len: %d\n",info,len);
    len = get_sample_info (&ns,"SAMPLE#:",info,120,NULL);
    printf("SAMPLE: '%s', len: %d\n",info,len);
    len = get_sample_info (&ns,"EOE:",info,120,NULL);
    printf("EOE: '%s', len: %d\n",info,len);

    update_sample_info(".","GregAutoEntry",
                            "DATA:", "/userdir/greg/auto/file1234" ,
                            "STATUS:", "Complete" );

    update_sample_info(".","GregAutoEntry",
                            "DATA:", "/userdir/greg/auto/file1236" ,
                            "STATUS:", "Active" );

    getPsgQentry("./psgQ",&ns);
    deletePsgQentry("./psgQ",&ns);

    deleteEnterQentry(".","enterQ");

}
getPsgQentry(char *filename,SAMPLE_INFO *s)
{
    char eolch;
    char textline[256];
    FILE *stream;
    int stat;

    stream = fopen(filename,"r");

    if (stream == NULL)  /* does file exist? */
    {
        return(-1);
    }

    if (fscanf(stream,"%[^\n]%c", textline, &eolch) <= 0)
    {
      fclose(stream);
      return(ERROR);
    }
    else
    {
       char *sptr,*token;
       sptr = textline;
       

       token = (char*) strtok(sptr," ");
       token = (char*) strtok(NULL,"\n");
    }
    stat = read_sample_info(stream,s);

    if ((stat == ENDOFFILE) || (stat == ERROR))
    {
        fclose(stream);
        return(-1);
    }

    fclose(stream);

    return(0);
}

deletePsgQentry(char *filename,struct sample_info *s)
{
   char shellcmd[3*MAXPATHL];
   char tmp[91];
   int entryindex;

   /* sly way of determining psgQentry length, we know 1 line for psg then sampleinfo size  */
   get_sample_info(s,"EOE:",tmp,90,&entryindex);  /* End-of-Entry index + 1 = size */

    /*   (tail +(nlines+1))  */
    sprintf(shellcmd,"tail +%d %s > /tmp/psgQtmp; mv -f /tmp/psgQtmp %s",
        entryindex+3,filename,filename);
 
    system(shellcmd);           /* remove entry from psgQ sent to Acqproc */
 
}

int deleteEnterQentry(char *autodir, char *enterfile)
{
   char shellcmd[3*MAXPATHL];
   int entrylen;
   int tmp_euid;

   entrylen = entryQ_length(autodir,enterfile);

/*
    sprintf(shellcmd,"tail +%d %s > /tmp/entertmp; mv -f /tmp/entertmp %s",
        N_SAMPLE_INFO_RECORDS+1,enterfile,enterfile);
*/
    sprintf(shellcmd,"tail +%d %s > /tmp/entertmp; mv -f /tmp/entertmp %s",
        entrylen+1,enterfile,enterfile);
    system(shellcmd);
 
} 

#endif 

#ifdef NEVER_DEFINED

Test Files:

autoinfo file
-------------
/* for some reason the GNU compile see these : lines even with the #ifdef, strange!  */
/* therefore I commented them out */
/* : This is the Mapping File  (No Blank Lines) */
/* : User's Directory Key Mapping */
/* USERDIR: USERDIR: */
/* : User's Data Directory Key Mapping */
/* DATA: DATA: */
/* : Experiment's Status Key Mapping */
/* STATUS: STATUS: */
/* : Automation Entry File's End-of-Entry (EOE) marker  (1st 10 characters) */
/* EOE ---------- */




EnterQ File
-----------

   SAMPLE#: 1
      USER: greg
     MACRO: h1
   SOLVENT: dmso
      TEXT: test
   USERDIR: /export/home/vnmrsys
      DATA: 
    STATUS: Queued
--------------------------------------------------------------------------------
   SAMPLE#: 2
      USER: greg
     MACRO: c13
   SOLVENT: C6D6
      TEXT: Entry
   USERDIR: /export/home/vnmrsys
      DATA: 
    STATUS: Queued
--------------------------------------------------------------------------------
   SAMPLE#: 3
      USER: greg
     MACRO: hc
   SOLVENT: acetone
      TEXT: Entry
   USERDIR: /export/home/vnmrsys
      DATA: /userdir/greg/auto/file1234
    STATUS: Queued
--------------------------------------------------------------------------------
   SAMPLE#: 4
      USER: greg
     MACRO: hcdept
   SOLVENT: CD3OD
      TEXT: Entry
   USERDIR: /export/home/vnmrsys
      DATA: 
    STATUS: Queued
--------------------------------------------------------------------------------
   SAMPLE#: 5
      USER: greg
     MACRO: cdept
   SOLVENT: Cyclohexane
      TEXT: Entry
   USERDIR: /export/home/vnmrsys
      DATA: 
    STATUS: Queued
--------------------------------------------------------------------------------


psgQ file
---------

/vnmr/acqqueue/data 1 auto
   SAMPLE#: 1
      USER: greg
     MACRO: h1
   SOLVENT: dmso
      TEXT: test
   USERDIR: /export/home/vnmrsys
      DATA: //export/home/vnmr3.8c/parlib/s2pul
    STATUS: Active
--------------------------------------------------------------------------------
/vnmr/acqqueue/data 1 auto
   SAMPLE#: 2
      USER: greg
     MACRO: h1
   SOLVENT: dmso
      TEXT: test
   USERDIR: /export/home/vnmrsys
      DATA: //export/home/vnmr3.8c/parlib/s2pul
    STATUS: Active
--------------------------------------------------------------------------------
/vnmr/acqqueue/data 1 auto
   SAMPLE#: 3
      USER: greg
     MACRO: h1
   SOLVENT: dmso
      TEXT: test
   USERDIR: /export/home/vnmrsys
      DATA: //export/home/vnmr3.8c/parlib/s2pul
    STATUS: Active
--------------------------------------------------------------------------------
#endif 
#endif  /* TESTING */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>


#ifdef VNMRJ
#include "vnmrsys.h"
#include "tools.h"
#include "wjunk.h"

extern int getAutoDir(char *str, int maxlen);
extern int send2Acq(int cmd, char *msg_for_acq);
extern char  UserName[];
extern char  HostName[];
extern pid_t    HostPid;
#endif

#ifdef TESTING
#define DEBUG
#define VNMRJ

#define MAXSTR 256
#define MAXPATH 256
#define RETURN return(0)
#define ABORT  return(1)
#define getAutoDir(str, maxlen) strcpy(str,".")
#define intString(arg) ""

char     UserName[MAXPATH];
char     HostName[MAXPATH];
pid_t    HostPid;

static void Werrprintf(char *fmt, char *arg)
{
   fprintf(stderr,fmt, arg);
}
#endif
#ifdef TESTING
#define DPRINT(str)      fprintf(stderr,str)
#define DPRINT1(str,arg) fprintf(stderr,str,arg)
#else
#define DPRINT(str)
#define DPRINT1(str,arg)
#endif

#ifndef MAXPATH
#define MAXPATH 256
#endif

#ifdef TESTING
static void sleepnano(int nanos)
{
   struct timespec req;

   DPRINT1("sleep for %g sec.\n", nanos / 1e9);
   req.tv_sec = 0;
   req.tv_nsec = nanos;
#ifdef __INTERIX
         usleep(req.tv_nsec/1000);
#else
         nanosleep( &req, NULL);
#endif
}
#endif

void sleepSeconds(int secs)
{
   struct timespec req;
   req.tv_sec = secs;
   req.tv_nsec = 0;
#ifdef __INTERIX
         usleep(req.tv_nsec/1000);
#else
         nanosleep( &req, NULL);
#endif
}

void sleepMilliSeconds(int msecs)
{
   struct timespec req;
   req.tv_sec = msecs / 1000;
   req.tv_nsec = (msecs % 1000) * 1000000;
#ifdef __INTERIX
         usleep(req.tv_nsec/1000);
#else
         nanosleep( &req, NULL);
#endif
}

#ifdef VNMRJ
static void blockedByGet(char *dir, char *name)
{
   struct timespec req;
   char blockpath[MAXPATH];
   int  cnt = 0;
#ifdef TESTING
   int  blk = 0;
#endif

   sprintf(blockpath,"%s/%s",dir,name);
   /* The get operation should be fast, but avoid endless loop with cnt */
   while ( ! access(blockpath, F_OK) && (cnt < 20) )
   {
#ifdef TESTING
      DPRINT("blocked by get\n");
      blk = 1;
#endif
#ifdef __INTERIX
      usleep(500000);
#else
      req.tv_sec = 0;
      req.tv_nsec = 500000000; /* .5 secs */
      nanosleep( &req, NULL);
#endif
      cnt++;
   }
#ifdef TESTING
   if (blk)
      DPRINT("get block removed\n");
#endif
}

/* getQueueFile returns a negative number on error.
 *     -1 if it can't open the output file
 *     -2 if there is a syntax error in the enterQ
 * It returns a 0 if there are no more entrys.
 * Also returns 0 if no entries because there is no enterQ file.
 * It returns a 1 if it got an entry.
 */
static int getQueueFile(char *autodir, char *filename)
{
   char queuefile[MAXPATH];
   FILE *fd;
   FILE *outfd;
   char startword[MAXPATH];
   char tmpStr[2*MAXPATH + 10];
   char tmp[MAXPATH];
   char tmp2[MAXPATH];
   int  lines = 0;
   char eolch;
   int  stat;

   sprintf(queuefile,"%s/enterQ", autodir);
   if ( (fd = fopen(queuefile, "r")) == NULL)
   {
      return(0);
   }
   if ( (outfd = fopen(filename, "w")) == NULL)
   {
      fclose(fd);
      return(-1);
   }
   while ( (stat = fscanf(fd,"%[^\n]%c", tmpStr, &eolch)) != EOF )
   {
      if ( stat != 2 )
      {
         Werrprintf("enterQ: file format error: %s\n", tmpStr);
         fclose(fd);
         fclose(outfd);
         return(-2);
      }
      strcat(tmpStr,"\n");
      sscanf(tmpStr,"%s: %[^\n]\n", tmp, tmp2);
      if (lines == 0)
      {
         strcpy(startword, tmp);
      }
      else if ( ! strcmp(startword, tmp) )
      {
         break;
      }
      lines++;
      fprintf(outfd,"%s",tmpStr);
   }
   fclose(fd);
   fclose(outfd);
   if (lines)
   {
      sprintf(tmpStr,"mv %s %s.tmp", queuefile, queuefile);
      system(tmpStr);
      sprintf(tmpStr,"tail -n +%d %s.tmp > %s.tmp2", lines+1,
              queuefile, queuefile);
      system(tmpStr);
      sprintf(tmpStr,"mv %s.tmp2 %s", queuefile, queuefile);
      system(tmpStr);
      strcat(queuefile,".tmp");
      unlink(queuefile);
      strcat(queuefile,"2");
      unlink(queuefile);
      return(1);
   }
   return(0);
}

/* addQueueFile returns a negative number on error.
 *     0 if it can't read the input file
 * It returns a 1 for success
 */
static int addQueueFile(char *autodir, char *filename, char *queuename)
{
   char queuefile[MAXPATH];
   char cmd[2*MAXPATH + 10];
   char buf[1024];
   int infd, outfd;
   int n;

   if ( ! access(filename,R_OK) )
   {
      sprintf(queuefile,"%s/enterQ", autodir);
      if ( ! strcmp(queuename,"std") )
      {
         /* append to tail of autoqueue */
         infd = open(filename, O_RDONLY);
         outfd = open(queuefile, O_WRONLY|O_CREAT|O_APPEND|O_SYNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH);
         while ( (n = read(infd,buf, 1024)) > 0)
            write(outfd,buf,n);
         fsync(outfd);
         close(outfd);
         close(infd);
      }
      else
      {
         /* prepend to head of autoqueue */
         sprintf(cmd,"mv %s %s.tmp", queuefile, queuefile);
         system(cmd);
         sprintf(cmd,"cp %s %s", filename, queuefile);
         system(cmd);
         sprintf(cmd,"/bin/cat %s.tmp >> %s", queuefile, queuefile);
         system(cmd);
         strcat(queuefile,".tmp");
         unlink(queuefile);
      }
      return(1);
   }
   return(0);
}
#endif


/* lockAtcmd and unlockAtcmd are used by Vnmrbg and Atproc */

void lockAtcmd(const char *dir)
{
   char lockPath[MAXSTR];
   char idPath[MAXSTR];
   time_t lockSecs = 5; /* default lock timeout */
   const char *lockname = "lockAtcmd";

   sprintf(lockPath,"%s/acqqueue/%s",dir,lockname);
   sprintf(idPath,"%s/acqqueue/pq_%d",dir,getpid());
   lockit(lockPath,idPath,lockSecs);
}

void unlockAtcmd(const char *dir)
{
   char lockPath[MAXSTR];
   char idPath[MAXSTR];
   const char *lockname = "lockAtcmd";

   sprintf(lockPath,"%s/acqqueue/%s",dir,lockname);
   sprintf(idPath,"%s/acqqueue/pq_%d",dir,getpid());
   unlockit(lockPath,idPath);
}

/* lockPsgQ and unlockPsgQ are used by PSG and Autoproc */

void lockPsgQ(const char *dir)
{
   char lockPath[MAXSTR];
   char idPath[MAXSTR];
   const char *lockname = "lockPsgQ";
   time_t lockSecs = 5; /* default lock timeout */

   sprintf(lockPath,"%s/%s",dir,lockname);
   sprintf(idPath,"%s/pq_%d",dir,getpid());
   lockit(lockPath,idPath,lockSecs);
}

void unlockPsgQ(const char *dir)
{
   char lockPath[MAXSTR];
   char idPath[MAXSTR];
   const char *lockname = "lockPsgQ";

   sprintf(lockPath,"%s/%s",dir,lockname);
   sprintf(idPath,"%s/pq_%d",dir,getpid());
   unlockit(lockPath,idPath);
}


#ifdef VNMRJ
static int receivingMsg = 0;

void autoqMsgOff()
{
   if (receivingMsg)
   {
      if (is_acqproc_active())
      {
   	 send2Acq(AUTOQMSG, "recvmsg off");
      }
   }
   receivingMsg = 0;
}

int autoq(int argc, char *argv[], int retc, char *retv[])
{
   char autodir[MAXPATH];
   char lockPath[MAXPATH];
   char idPath[MAXPATH];
   char lockLogPath[MAXPATH];
   char idLogPath[MAXPATH];
   char logBasePath[MAXPATH];
   char *blockname = "autoqBlock";
   char *lockname = "lockEnterQ";
   time_t lockSecs = 5; /* default lock timeout */
   int  res = 1;

   if (argc < 2)
   {
      Werrprintf("usage: %s requires add, get, lock, unlock, sendmsg, or recvmsg argument",
                  argv[0]);
      ABORT;
   }
   if ( !strcmp(argv[1],"sendmsg") || !strcmp(argv[1],"recvmsg") )
   {
      res = 4;
   }
   else
   {
#ifdef VNMRJ
      if ( ! strcmp(argv[1],"add") )
         P_getstring(GLOBAL,"autodir",autodir,1,MAXPATH);
      else
#endif
         getAutoDir(autodir, MAXPATH);
      sprintf(lockPath,"%s/%s",autodir,lockname);
      sprintf(idPath,"%s/eq_%s_%s_%d",autodir,HostName,UserName,HostPid);
   }
   if ( ! strcmp(argv[1],"add") )
   {
      char queuename[MAXPATH];
      if (argc < 3)
      {
         Werrprintf("usage: %s('add',filename) requires a filename.",argv[0]);
         ABORT;
      }
      if (argc == 4)
         strcpy(queuename,argv[3]);
      else
         strcpy(queuename,"std");
      /* Wait for get operations to complete first */
      blockedByGet(autodir, blockname);
      if ( lockit(lockPath,idPath,lockSecs) )
      {
         res = addQueueFile(autodir, argv[2], queuename);
         unlockit(lockPath,idPath);
      }
      else
      {
         res = -4;
      }
   }
   else if ( ! strcmp(argv[1],"get") )
   {
      FILE *fd;
      char blockfile[MAXPATH];
      if (argc < 3)
      {
         Werrprintf("usage: %s('get',filename) requires a filename.",argv[0]);
         ABORT;
      }
      /* The get operation will block add and lock from other processes */
      sprintf(blockfile,"%s/%s",autodir,blockname);
      fd = fopen(blockfile,"w");
      fclose(fd);
      if ( lockit(lockPath,idPath,lockSecs) )
      {
         res = getQueueFile(autodir, argv[2]);
         unlockit(lockPath,idPath);
      }
      else
      {
         res = -4;
      }
#ifdef TESTING
      if (argc == 4)
         sleepnano(atoi(argv[3]));
#endif
      /* Allow access by add and lock from other processes */
      unlink(blockfile);
   }
   else if ( ! strcmp(argv[1],"lock") )
   {

      if (argc == 3)
      {
         lockSecs = atoi(argv[2]);
         if (lockSecs < 1)
            lockSecs = 1;
         else if (lockSecs > 15)
            lockSecs = 15;
      }
      /* Wait for get operations to complete first */
      blockedByGet(autodir, blockname);
      DPRINT1("lock called with %ld timeout\n",lockSecs);
      res = lockit(lockPath,idPath,lockSecs);
   }
   else if ( ! strcmp(argv[1],"unlock") )
   {
      unlockit(lockPath,idPath);
      res = 1;
   }
   else if ( ! strcmp(argv[1],"addAcct") )
   {      
      char accFile[MAXPATH];
      char idFile[MAXPATH];

      if (argc != 3)
      {
         Werrprintf("autoq('addAcct',filename) requires filename as the second argument");
         ABORT;
      }
      if ( access(argv[2], R_OK|W_OK) )
      {
         Werrprintf("autoq('addAcct',filename) filename does not exist or has wrong permissions");
         ABORT;
      }
      res = 0;
      strcpy(lockPath, getenv("vnmrsystem") );
      strcat(lockPath,"/adm/accounting/");
      strcpy(accFile, lockPath);
      strcat(accFile,"acctLog.xml");
      if ( !access(accFile,F_OK) )
      {
         FILE    *inputFd;
         int    bufSize=4095;
         char   buf[bufSize+1];
         int size = 0;

         inputFd = fopen(argv[2],"r");
         if (inputFd)
         {
            size = fread(buf, 1, bufSize, inputFd);
            /* Terminate the buffer */
            buf[size] = 0;
            fclose(inputFd);
         }
         unlink(argv[2]);
         if (size)
         {
            lockSecs = 2;
        
            // Create filepaths for the lock files to lock acctLog
            strcpy(idPath, lockPath);
            strcat(lockPath, "acctLogLock");
            sprintf(idFile, "acctLogLockId_%s_%d", HostName, HostPid);
            strcat(idPath, idFile);

            DPRINT1("lock called with %ld timeout\n",lockSecs);
            if ( lockit(lockPath,idPath,lockSecs) )
            {
               const char root_end[]="</accounting>\n";
               FILE    *xmlFd;

               xmlFd = fopen(accFile,"r+");
               if (xmlFd)
               {
                  /* find the closing /> at the end and put log info above that */
                  fseek(xmlFd,-strlen(root_end),SEEK_END);
                  /* Write to the log file */
                  fprintf(xmlFd,"%s",buf);
                  fprintf(xmlFd,"%s",root_end);
                  fflush(xmlFd);
                  fclose(xmlFd);
               }

               unlockit(lockPath,idPath);
               res = 1;
            }
         }
      }
   }
   else if ( ! strcmp(argv[1],"locklog") )
   {      
      lockSecs = 15;
        
      // Create filepaths for the lock files to lock acctLog
      strcpy(logBasePath, (char *)getenv("vnmrsystem") );
      strcat(logBasePath,"/adm/accounting/");
      sprintf(lockLogPath, "%s%s", logBasePath, "acctLogLock");
      sprintf(idLogPath, "%sacctLogLockId_%s_%d", logBasePath, HostName, HostPid);

      /* Wait for get operations to complete first */
      blockedByGet(autodir, blockname);
      DPRINT1("lock called with %ld timeout\n",lockSecs);

      res = lockit(lockLogPath,idLogPath,lockSecs);
   }
   else if ( ! strcmp(argv[1],"unlocklog") )
   {

      // Create filepaths for the lock files to lock acctLog
      strcpy(logBasePath, (char *)getenv("vnmrsystem") );
      strcat(logBasePath,"/adm/accounting/");
      sprintf(lockLogPath, "%s%s", logBasePath, "acctLogLock");
      sprintf(idLogPath, "%sacctLogLockId_%s_%d", logBasePath, HostName, HostPid);

      unlockit(lockLogPath,idLogPath);
      res = 1;
   }
   else if (res == 4) /* ( !strcmp(argv[1],"sendmsg") || !strcmp(argv[1],"recvmsg") ) */
   {
      if ( ! strcmp(argv[1],"recvmsg") )
      {
         if ( (argc != 3) || ( strcmp(argv[2],"on") && strcmp(argv[2],"off")) )
         {
            Werrprintf("autoq %s requires 'on' or 'off' as the second argument",argv[1]);
            ABORT;
         }
      }
      if (argc != 3)
      {
         Werrprintf("autoq %s requires another argument",argv[1]);
         ABORT;
      }
      res = 1;
      if (is_acqproc_active())
      {
         char msg[MAXSTR];

         sprintf(msg,"%s %s",argv[1],argv[2]);
   	 if (send2Acq(AUTOQMSG, msg) < 0)
            res = 0;
         if (res)
            receivingMsg = ( ! strcmp(argv[2],"on"));
      }
   }
   DPRINT1("operation returns %d\n",res);
   if (retc)
   {
      retv[0] = intString(res);
   }
   RETURN;
}
#endif

#ifdef TESTING

/*
 * These tests assume pwd is autodir. 
 * entry is an enterQ entry
 * si is an output sampleInfo pathname.
 * The pid is used in the lockfile naming scheme
 * A get operation takes precedence over add and lock.
 * The optional delay for get demonstrates the add or lock
 * waiting.
 *
 * ./asmfuncs lock <pid <timeout>>
 * ./asmfuncs unlock <pid>
 * ./asmfuncs add entry
 * ./asmfuncs add entry priority
 * ./asmfuncs add ent std <loops <pid <timeout in nsec>>>
 * ./asmfuncs add ent std 2000 100 500000000
 <* ./asmfuncs get si <pid <delay in nanosecs>>
 *
 * Typical test may be
 * ./asmfuncs add entry std 2000 100 500000000
 * to add 2000 entrties to the enterQ, with a .5 sec delay between addtions.
 * In a separate shell in the same directory.
 * ./asmfuncs lock
 * will cause the addition loop to wait.
 * ./asmfuncs unlock
 * will let the loop continue.
 * ./asmfuncs get si 200 700000000
 * holds the blocking file so that the add loop waits.
 * ./asmfuncs lock 8
 * but no unlock should timeout after 8 seconds.
 */

int main(int argc, char *argv[])
{
   char *args[4];
   int  i = 0;

   HostPid = 100l;
   strcpy(HostName,"se5");
   strcpy(UserName,"dan");
   if (argc < 2)
   {
      fprintf(stderr,"first argument needs to be add, get, lock, or unlock\n");
      exit(EXIT_FAILURE);
   }
   while (i < argc)
   {
      fprintf(stderr,"argv[%d]: %s\n",i,argv[i]);
      i++;
   }
   args[0] = "autoq";
   if ( ! strcmp(argv[1], "get") )
   {
      if (argc < 3)
      {
         fprintf(stderr,"get requires a filename\n");
         exit(EXIT_FAILURE);
      }
      args[1] = "get";
      args[2] = argv[2];
      if (argc == 4)
      {
         HostPid = atoi(argv[3]);
      }
      if (argc == 5)
      {
         /* timeout in nanosec */
         args[3] = argv[4];
         autoq(4, args, 0, NULL);
      }
      else
      {
         autoq(3, args, 0, NULL);
      }
   }
   else if ( ! strcmp(argv[1], "add") )
   {
      if (argc < 3)
      {
         fprintf(stderr,"add requires a filename\n");
         exit(EXIT_FAILURE);
      }
      args[1] = "add";
      args[2] = argv[2];
      if (argc >= 4)
      {
         int loops = 1;
         int nano = 5000000;
         if (argc >= 5)
         {
            HostPid = atoi(argv[4]);
            if (argc >= 6)
            {
               loops = atoi(argv[5]);
               if (argc == 7)
                  nano = atoi(argv[6]);
            }
         }
         args[3] = argv[3];
         DPRINT1("add %d times\n", loops);
         while (loops--)
         {
            autoq(4, args, 0, NULL);
            if (loops)
               sleepnano(nano);
         }
      }
      else
      {
         autoq(3, args, 0, NULL);
      }
   }
   else if ( ! strcmp(argv[1], "lock") )
   {
      args[1] = "lock";
      if (argc == 3)
      {
         args[2] = argv[2];
         autoq(3, args, 0, NULL);
      }
      else
      {
         autoq(2, args, 0, NULL);
      }
   }
   else if ( ! strcmp(argv[1], "unlock") )
   {
      args[1] = "unlock";
      autoq(2, args, 0, NULL);
   }
   else if ( ! strcmp(argv[1], "lockPsgQ") )
   {
      lockPsgQ(".");
   }
   else if ( ! strcmp(argv[1], "unlockPsgQ") )
   {
      unlockPsgQ(".");
   }
   else
   {
      fprintf(stderr,"first argument needs to be add, get, lock, unlock, lockPsgQ, or unlockPsgQ\n");
      exit(EXIT_FAILURE);
   }
   exit(EXIT_SUCCESS);
}

#endif 

#ifdef NEVER_DEFINED

Test Files:


EnterQ File
-----------

   SAMPLE#: 1
      USER: greg
     MACRO: h1
   SOLVENT: dmso
      TEXT: test
   USERDIR: /export/home/vnmrsys
      DATA: 
    STATUS: Queued
--------------------------------------------------------------------------------
   SAMPLE#: 2
      USER: greg
     MACRO: c13
   SOLVENT: C6D6
      TEXT: Entry
   USERDIR: /export/home/vnmrsys
      DATA: 
    STATUS: Queued
--------------------------------------------------------------------------------
   SAMPLE#: 3
      USER: greg
     MACRO: hc
   SOLVENT: acetone
      TEXT: Entry
   USERDIR: /export/home/vnmrsys
      DATA: /userdir/greg/auto/file1234
    STATUS: Queued
--------------------------------------------------------------------------------
   SAMPLE#: 4
      USER: greg
     MACRO: hcdept
   SOLVENT: CD3OD
      TEXT: Entry
   USERDIR: /export/home/vnmrsys
      DATA: 
    STATUS: Queued
--------------------------------------------------------------------------------
   SAMPLE#: 5
      USER: greg
     MACRO: cdept
   SOLVENT: Cyclohexane
      TEXT: Entry
   USERDIR: /export/home/vnmrsys
      DATA: 
    STATUS: Queued
--------------------------------------------------------------------------------
#endif 
