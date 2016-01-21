/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/* support routines for 'decctool'			*/
/* Digital Eddy Current Compensation			*/
/*							*/
/* decc_compare() 					*/
/*		compare all o_enabled with enabled	*/
/*                          o_taus    with taus         */
/*                          o_ampls   with ampls	*/
/*                          o_limits  with limits       */
/*                          o_scales  with scales       */
/*              if equal set deccflag[7]=1, else 0      */
/*							*/
/* decc_load(fln)					*/
/*              load values from fln. if no version is  */
/*              specified, use latest.			*/
/*							*/
/* decc_save(fln)					*/
/*              write values to fln and sysgcoil	*/
/*              ensure version number is incremented    */
/*              reset all o_* to * and deccflag[7]=0    */
/*              set deccflag[8] = new version		*/
/*							*/
/* decc_purge(...) arg1=filename.ext 			*/
/*		purge all files in /vnmr/imaging/decclib*/
/*		with name filename, and extension < ext */
/*							*/
#include <stdio.h>
#ifdef __INTERIX
#define _REENTRANT
#endif
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <math.h>
#include "vnmrsys.h"
#include "group.h"
#include "pvars.h"
#include "tools.h"
#include "wjunk.h"

static int debug = 0;

static char *terms[] = { "xx",  "yx",  "zx",
                         "xy",  "yy",  "zy",
                         "xz",  "yz",  "zz",
                         "xb0", "yb0", "zb0",
                         "b0b0"
                       };

static int nterms[13]  = { 6,3,3,3,6,3,3,3,6,4,4,4,4 };
static int bigCount = 0;

int decc_compare(int argc, char *argv[], int retc, char *retv[])
{
int i, same;
char oe[MAXSTR],e[MAXSTR];
double otaus, taus, oampls, ampls, olimits, limits, oscales, scales;

   (void)argv;
   if (argc>1) debug=1;
   else        debug=0;
   if (debug)
      Winfoprintf("decc_compare started");

   same=1;
   for (i=1; i<53; i++)
   {
      if (P_getstring(GLOBAL, "enabled", e, i, MAXSTR) < 0)
      {
         Werrprintf("decc_compare: cannot find 'enabled'");
         same=0; break;
      }
      if (P_getstring(GLOBAL, "o_enabled", oe, i, MAXSTR) < 0)
      {
         Werrprintf("decc_compare: cannot find 'o_enabled'");
         same=0; break;
      }
      if (e[0] != oe[0])
      {
         same=0; break;
      }

      if (P_getreal(GLOBAL, "taus", &taus, i) < 0)
      {
         Werrprintf("decc_compare: cannot find 'taus'");
         same=0; break;
      }
      if (P_getreal(GLOBAL, "o_taus", &otaus, i) < 0)
      {
         Werrprintf("decc_compare: cannot find 'o_taus'");
         same=0; break;
      }
      if (fabs(taus-otaus) > 1e-6)
      {
         same=0; break;
      }
         
      if (P_getreal(GLOBAL, "ampls", &ampls, i) < 0)
      {
         Werrprintf("decc_compare: cannot find 'ampls'");
         same=0; break;
      }
      if (P_getreal(GLOBAL, "o_ampls", &oampls, i) < 0)
      {
         Werrprintf("decc_compare: cannot find 'o_ampls'");
         same=0; break;
      }
      if (fabs(ampls-oampls) > 1e-6)
      {
         same=0; break;
      }
   }

   if (same != 0) 
   {
      for (i=1; i<8; i++)
      {
         if (P_getreal(GLOBAL, "limits", &limits, i) < 0)
         {
            Werrprintf("decc_compare: cannot find 'limits'");
            same=0; break;
         }
         if (P_getreal(GLOBAL, "o_limits", &olimits, i) < 0)
         {
            Werrprintf("decc_compare: cannot find 'o_limits'");
            same=0; break;
         }
         if (fabs(limits-olimits) > 5e-9)
         {
            same=0; break;
         }
      }
   }
	
   if (same != 0) 
   {
      for (i=1; i<11; i++)
      {
         if (P_getreal(GLOBAL, "scales", &scales, i) < 0)
         {
            Werrprintf("decc_compare: cannot find 'scales'");
            same=0; break;
         }
         if (P_getreal(GLOBAL, "o_scales", &oscales, i) < 0)
         {
            Werrprintf("decc_compare: cannot find 'o_scales'");
            same=0; break;
         }
         if (fabs(scales-oscales) > 5e-9)
         {
            same=0; break;
         }
      }
   }

   if (same != 0)
   {
      if (P_getstring(GLOBAL, "decctool_fln", e, 1, MAXSTR) < 0)
      {
         Werrprintf("decc_compare: cannot find 'decctool_fln'");
         same=0;
      }
      if (P_getstring(GLOBAL, "deccorig_fln", oe, 1, MAXSTR) < 0)
      {
         Werrprintf("decc_compare: cannot find 'deccorig_fln'");
         same=0;
      }
      if ( strcmp(e,oe) )
      {
Winfoprintf("decc_compare: '%s' '%s'\n",e,oe);
         same=0;
      }
   }
	
       
   if (P_setreal(GLOBAL, "deccflag", (double)same, 7) < 0)
   {
      Werrprintf("decc_compare: cannot set deccflag[7]");
   }
   appendvarlist("deccflag");
   if (retc > 0) 
      retv[0] = realString((double) same);
   RETURN; 
}

static int getVersion(char *filename)
{
int k;
int version=-1;
   k = strlen(filename)-1;
   while (k>0)
   {
      if ( ! isdigit((int)filename[k]) )
         break;
      k--;
   }
   if (filename[k]=='.') 
   {  filename[k]='\000';
      version = atoi(&filename[k+1]);
   }
   return(version);
}

static int getBiggestVersion(char *fullpath)
{
char  filename[MAXSTR];
char  cmd[MAXSTR];
int   j,version, biggest;
FILE *fd;

   strcpy(cmd,"ls ");
   strcat(cmd,fullpath);
   strcat(cmd,".*");

   fd = popen(cmd, "r");
   if (fd==0)
   {
      Werrprintf("decc_version: problem opening %s*",fullpath);
      return(-1);
   }

   version=-1;
   biggest=0;
   bigCount=0;
   j = fscanf(fd,"%s",filename);
   while (j>0)
   {
      version = getVersion(filename);
      if (version > biggest) biggest=version;
      if (debug) Winfoprintf("decc_version: fln='%s', version=%d biggest=%d",
			filename,version, biggest);
      j = fscanf(fd,"%s",filename);
      bigCount++;
   }
   pclose(fd);
   return(biggest);
}

#define SKIP	0
#define TAUS	1
#define SCALES	2
#define LIMITS	3

int decc_load(int argc, char *argv[], int retc, char *retv[])
{
char    decclibpath[MAXSTR];
char    filename[MAXSTR];
char    fullpath[MAXSTR];
char    oneline[MAXSTR];
char    delimiters[10], yesno[10];
char	cmd[MAXPATH];
char	sysgcoil[MAXSTR];
#ifdef LINUX
char   *ptr,*ptr2;
#else
char   *ptr,*ptr2, *strtok_r();
#endif
double  taus, ampls, scales, limits;
int     i,version;
int     param,first,last;
int	dotflag;
FILE *fd;
   (void) retc;
   (void) retv;
   if (argc>2) debug=1;
   else        debug=0;
   if (argc < 2)
   {
      Werrprintf("Usage -- decc_load(filename)");
      RETURN;
   }

   version = first = last = 0;
   param = SKIP;
   strcpy(filename,argv[1]);
   strcpy(decclibpath,systemdir);
   strcat(decclibpath,"/imaging/decclib/");
   strcpy(fullpath,decclibpath);

   if (filename[0] != '.') 
   {
      dotflag=0;
      if ((version = getVersion(filename)) < 0)
      { 
         // check decclib for highest version 
         strcat(fullpath,filename);
         version = getBiggestVersion(fullpath);
      }

      if (version == 0) 
         sprintf(fullpath,"%s%s",decclibpath,filename);
      else
         sprintf(fullpath,"%s%s.%d",decclibpath,filename,version);
   }
   else
   {
      dotflag=1;
      sprintf(fullpath,"%s%s", fullpath, filename);
   }
   Winfoprintf("decc_load: loading '%s'",fullpath);
   fd = fopen(fullpath, "r");
   if (fd == NULL)
   {  Werrprintf("decc_load: can not open %s",fullpath);
      RETURN;
   }

   strcpy(delimiters,"\n \t");
   ptr = fgets(oneline,MAXSTR,fd);
   if (debug) Winfoprintf("dec_load: oneline='%s'",oneline);
   while (ptr != NULL)
   {
      ptr2 = strtok_r(ptr,delimiters,&ptr);
      if (ptr2 == 0)
      {  ptr = fgets(oneline,MAXSTR,fd);
         continue;	// skip blank lines
      }
      if (debug) Winfoprintf("decc_load: linetype='%s'",ptr2);
      if ( (strcmp(ptr2,"filename") == 0) && (dotflag==1) )
      {  ptr2 = strtok_r(ptr,delimiters,&ptr);
          strcpy(filename,ptr2);
          if (debug) Winfoprintf("decc_load: filename='%s'",filename);
          if (P_setstring(GLOBAL,"deccorig_fln", filename, 1) < 0)
          {
             Werrprintf("decc_load: cannot set 'deccorig_fln'");
          }
          if (P_setstring(GLOBAL,"decctool_fln", filename, 1) < 0)
          {
             Werrprintf("decc_load: cannot set 'decctool_fln'");
          }
          version = getVersion(filename);
          param=SKIP;
      }
      else if (strncmp(ptr2,"da",2) == 0) { param=SKIP; }
      else if (strncmp(ptr2,"xx",2) == 0) { param=TAUS;   first=1;  last=6;  }
      else if (strncmp(ptr2,"yx",2) == 0) { param=TAUS;   first=7;  last=9;  }
      else if (strncmp(ptr2,"zx",2) == 0) { param=TAUS;   first=10; last=12; }
      else if (strncmp(ptr2,"xy",2) == 0) { param=TAUS;   first=13; last=15; }
      else if (strncmp(ptr2,"yy",2) == 0) { param=TAUS;   first=16; last=21; }
      else if (strncmp(ptr2,"zy",2) == 0) { param=TAUS;   first=22; last=24; }
      else if (strncmp(ptr2,"xz",2) == 0) { param=TAUS;   first=25; last=27; }
      else if (strncmp(ptr2,"yz",2) == 0) { param=TAUS;   first=28; last=30; }
      else if (strncmp(ptr2,"zz",2) == 0) { param=TAUS;   first=31; last=36; }
      else if (strncmp(ptr2,"xb",2) == 0) { param=TAUS;   first=37; last=40; }
      else if (strncmp(ptr2,"yb",2) == 0) { param=TAUS;   first=41; last=44; }
      else if (strncmp(ptr2,"zb",2) == 0) { param=TAUS;   first=45; last=48; }
      else if (strncmp(ptr2,"b0",2) == 0) { param=TAUS;   first=49; last=52; }
      else if (strncmp(ptr2,"ec",2) == 0) { param=SCALES; first=1;  last=4;  }
      else if (strncmp(ptr2,"sh",2) == 0) { param=SCALES; first=5;  last=7;  }
      else if (strncmp(ptr2,"to",2) == 0) { param=SCALES; first=8;  last=10; }
      else if (strncmp(ptr2,"sl",2) == 0) { param=LIMITS; first=1;  last=4;  }
      else if (strncmp(ptr2,"du",2) == 0) { param=LIMITS; first=5;  last=7;  }

      switch(param)
      {  case TAUS:
              for (i=first; i<=last; i++)
              {  ptr2 = strtok_r(ptr,delimiters,&ptr);
                 if (*ptr2 == '*')
                 {  strcpy(yesno,"n");
                    ptr2++;
                 }
                 else
                 {  strcpy(yesno,"y");
                 }
                 if (P_setstring(GLOBAL,"enabled", yesno, i) < 0)
                 {  Werrprintf("decc_load: cannot set 'enabled'");
                 }
                 if (P_setstring(GLOBAL,"o_enabled", yesno, i) < 0)
                 {  Werrprintf("decc_load: cannot set 'o_enabled'");
                 }
                 taus = atof(ptr2) * 1000.0;
                 if (P_setreal(GLOBAL,"taus",taus,i) < 0)
                 {  Werrprintf("decc_load: cannot set 'taus'");
                 }
                 if (P_setreal(GLOBAL,"o_taus",taus,i) < 0)
                 {  Werrprintf("decc_load: cannot set 'o_taus'");
                 }
                 ptr2 = strtok_r(ptr,delimiters,&ptr);
                 ampls = atof(ptr2) * 100.0;
                 if (P_setreal(GLOBAL,"ampls",ampls,i) < 0)
                 {  Werrprintf("decc_load: cannot set 'ampls'");
                 }
                 if (P_setreal(GLOBAL,"o_ampls",ampls,i) < 0)
                 {  Werrprintf("decc_load: cannot set 'o_ampls'");
                 }
                 if (debug)
                    Winfoprintf("decc_load: i=%d e=%s t=%f a=%f",
					i,yesno, taus, ampls);
              }
              break;
         case SCALES:
              for (i=first; i<=last; i++)
              {  ptr2 = strtok_r(ptr,delimiters,&ptr);
                 if (ptr2 == 0)
                    scales = 0.0;
                 else
                    scales = atof(ptr2);
                 if (P_setreal(GLOBAL,"scales",scales,i) < 0)
                 {  Werrprintf("decc_load: cannot set 'scales'");
                 }
                 if (P_setreal(GLOBAL,"o_scales",scales,i) < 0)
                 {  Werrprintf("decc_load: cannot set 'o_scales'");
                 }
              }
              break;
         case LIMITS:
              for (i=first; i<=last; i++)
              {  ptr2 = strtok_r(ptr,delimiters,&ptr);
                 if (ptr2 == 0)
                    limits = 0.0;
                 else
                    limits = atof(ptr2);
                 if (P_setreal(GLOBAL,"limits",limits,i) < 0)
                 {  Werrprintf("decc_load: cannot set 'limits'");
                 }
                 if (P_setreal(GLOBAL,"o_limits",limits,i) < 0)
                 {  Werrprintf("decc_load: cannot set 'o_limits'");
                 }
              }
              break;
      }
      ptr = fgets(oneline,MAXSTR,fd);
   }
   if (P_setreal(GLOBAL, "deccflag", 1.0, 7) < 0) // Unmodified
   {
      Werrprintf("decc_load cannot set deccflag[7]");
   }
   if (P_setreal(GLOBAL,"deccflag", (double)(version),8) < 0)
   {
      Werrprintf("decc_load: cannot set 'deccflag[8]' to version");
   }
 
   appendvarlist("enabled,taus,ampls,scales,limits,deccflag,deccorig_fln,decctool_fln");
   if (P_getstring(SYSTEMGLOBAL,"sysgcoil", sysgcoil, 1, MAXSTR) < 0)
   {
      Werrprintf("decc_load: cannot find 'sysgcoil'");
   }
   else
   {
      strcpy(cmd,"cp ");	// "cp "
      strcat(cmd,fullpath);     // "cp /vnmr/imaging/decclib/new.3"
      strcat(cmd," ");     	// "cp /vnmr/imaging/decclib/new.3"
      strcat(cmd,decclibpath);  // "cp .../new.3 /vnmr/imaging/decclib"
      strcat(cmd,"/.");		// "cp .../new.3 .../."
      strcat(cmd,sysgcoil);	// "cp .../new.3 .../.sysgcoil"
      system(cmd);
   }
   if (debug) Winfoprintf("decc_load: Done!");
   fclose(fd);
   RETURN;
}

int decc_save(int argc, char *argv[], int retc, char *retv[])
{
char    sysgcoil[MAXSTR];
char    filename[MAXSTR];
char    decclibpath[MAXSTR];
char    fullpath[MAXSTR];
char    cmd[MAXSTR];
char   *tmpPtr, *tmpPtr2;
char    enabled[MAXSTR];
double  taus,ampls,scales,limits;
int     done, biggest;
int     i, j,k;
FILE   *fd,*fd2;

   (void) retc;
   (void) retv;
   if (argc>2) debug=1;
   else        debug=0;
   if (argc < 2)
   {
      Werrprintf("Usage -- decc_save(filename)");
      RETURN;
   }
   if (P_getstring(SYSTEMGLOBAL,"sysgcoil", sysgcoil, 1, MAXSTR) < 0)
   {
      Werrprintf("decc_save: cannot find 'sysgcoil'");
      RETURN;
   }

   strcpy(filename,argv[1]);
   if (debug) Winfoprintf("decc_save: inputfile='%s'\n",filename);
   if (filename[0] == '.')
   {  Werrprintf("decc_save: filename cannot start with a '.'");
      ABORT;
   }

   // strip version if any
   getVersion(filename);
   if (debug) Winfoprintf("decc_save: minus version='%s'\n",filename);

   // check decclib for highest version 
   strcpy(decclibpath,systemdir);
   strcat(decclibpath,"/imaging/decclib/");
   strcpy(fullpath,decclibpath);
   strcat(fullpath,filename);

   biggest = getBiggestVersion(fullpath);

   tmpPtr = tmpPtr2 = filename;
   sprintf(fullpath,"%s%s.%d",decclibpath,tmpPtr,biggest+1);

   done=0;
   while ( ! done)
   {
      Winfoprintf("decc_save: saving to '%s'",fullpath);
      // save to file
      fd = fopen(fullpath,"w");
      if (fd==0)
      {
         Werrprintf("decc_save: problem opening %s",fullpath);
         RETURN;
      }
      fprintf(fd,"filename\t%s.%d\n",tmpPtr2,biggest+1);
      fd2 = popen("date","r");
      fgets(cmd,MAXSTR,fd2);
      pclose(fd2);
      fprintf(fd,"date\t%s\n",cmd);
      
      i=1; j=0; k=0;
      while (j<13)
      {
         fprintf(fd,"%s",terms[j]);		// "xx" etc.
         k += nterms[j];
         while (i<=k)
         {
            fputc((int)'\t',fd);
            if (P_getstring(GLOBAL,"enabled",enabled,i,MAXSTR) < 0)
            {
               Werrprintf("decc_save: cannot find 'enabled'");
            }
            if (enabled[0]=='n') fputc((int)'*',fd);
            if (P_getreal(GLOBAL,"taus",&taus,i) < 0)
            {
               Werrprintf("decc_save: cannot find 'taus'");
            }
            if (P_getreal(GLOBAL,"ampls",&ampls,i) < 0)
            {
               Werrprintf("decc_save: cannot find 'ampls'");
            }
            fprintf(fd,"%g %g",taus/1000.0,ampls/100.0);
            i++;
         }
         fputc((int)'\n',fd);
         j++;
      }

      fprintf(fd,"eccscale");
      for (i=1; i<5; i++)
      {
         if (P_getreal(GLOBAL,"scales",&scales,i) < 0)
         {
            Werrprintf("decc_save: cannot find 'scales'");
         }
         fprintf(fd,"\t%g",scales);
      }
      fputc((int)'\n',fd);

      fprintf(fd,"shimscale");
      for (i=5; i<8; i++)
      {
         if (P_getreal(GLOBAL,"scales",&scales,i) < 0)
         {
            Werrprintf("decc_save: cannot find 'scales'");
         }
         fprintf(fd,"\t%g",scales);
      }
      fputc((int)'\n',fd);

      fprintf(fd,"totalscale");
      for (i=8; i<11; i++)
      {
         if (P_getreal(GLOBAL,"scales",&scales,i) < 0)
         {
            Werrprintf("decc_save: cannot find 'scales'");
         }
         fprintf(fd,"\t%g",scales);
      }
      fputc((int)'\n',fd);

      fprintf(fd,"slewlimit");
      for (i=1; i<5; i++)
      {
         if (P_getreal(GLOBAL,"limits",&limits,i) < 0)
         {
            Werrprintf("decc_save: cannot find 'limits'");
         }
         fprintf(fd,"\t%g",limits);
      }
      fputc((int)'\n',fd);

      fprintf(fd,"dutylimit");
      for (i=5; i<9; i++)
      {
         if (P_getreal(GLOBAL,"limits",&limits,i) < 0)
         {
            Werrprintf("decc_save: cannot find 'limits'");
         }
         fprintf(fd,"\t%g",limits);
      }
      fputc((int)'\n',fd);
      fclose(fd);

      // check if we need to save to .sysgcoil
      if (tmpPtr==sysgcoil) done=1;
      else
      {  
         tmpPtr=sysgcoil;
         sprintf(fullpath,"%s.%s",decclibpath,tmpPtr);
      }
   }
   // Now all is saved,
   // deccflag[8]=biggest+1 (version)
   // set o_enabled=enabled, o_taus=taus, o_ampls=ampls
   //     o_limits=limits,   o_scales=scales
   // and deccflag[7]=1 (Unmodified).
   if (P_setreal(GLOBAL,"deccflag", (double)(biggest+1),8) < 0)
   {
      Werrprintf("decc_save: cannot set 'deccflag[8]'i to version");
   }

   for (i=1; i< 53; i++)
   {   
      if (P_getstring(GLOBAL,"enabled",enabled,i,MAXSTR) < 0)
      {
         Werrprintf("decc_save: cannot find 'enabled'");
         break;
      }
      if (P_getreal(GLOBAL,"taus",&taus,i) < 0)
      {
         Werrprintf("decc_save: cannot find 'taus'");
         break;
      }
      if (P_getreal(GLOBAL,"ampls",&ampls,i) < 0)
      {
         Werrprintf("decc_save: cannot find 'ampls'");
         break;
      }

      if (P_setstring(GLOBAL,"o_enabled", enabled, i) < 0)
      {
         Werrprintf("decc_save: cannot find 'o_enabled'");
         break;
      }
      if (P_setreal(GLOBAL,"o_taus", taus, i) < 0)
      {
         Werrprintf("decc_save: cannot find 'o_taus'");
         break;
      }
      if (P_setreal(GLOBAL,"o_ampls", ampls, i) < 0)
      {
         Werrprintf("decc_save: cannot find 'o_ampls'");
         break;
      }
   }

   for (i=1; i<11; i++)
   {
      if (P_getreal(GLOBAL,"scales",&scales,i) < 0)
      {
         Werrprintf("decc_save: cannot find 'scales'");
         break;
      }
      if (P_setreal(GLOBAL,"o_scales", scales, i) < 0)
      {
         Werrprintf("decc_save: cannot find 'o_scales'");
         break;
      }
   }

   for (i=1; i<8; i++)
   {
      if (P_getreal(GLOBAL,"limits",&limits,i) < 0)
      {
         Werrprintf("decc_save: cannot find 'limits'");
         break;
      }
      if (P_setreal(GLOBAL,"o_limits", limits, i) < 0)
      {
         Werrprintf("decc_save: cannot find 'o_limits'");
         break;
      }
   } 

   if (P_setreal(GLOBAL, "deccflag", 1.0, 7) < 0) // Unmodified
   {
      Werrprintf("decc_save: cannot set deccflag[7]");
   }
   if (debug) Winfoprintf("decc_save: saved fln='%s',version=%d\n",
				filename,biggest+1);

   sprintf(fullpath,"%s.%d",filename,biggest+1);
   if (P_setstring(GLOBAL,"deccorig_fln", fullpath, 1) < 0)
   {
      Werrprintf("decc_load: cannot set 'deccorig_fln'");
   }
   if (P_setstring(GLOBAL,"decctool_fln", fullpath, 1) < 0)
   {
      Werrprintf("decc_load: cannot set 'decctool_fln'");
   }

   appendvarlist("deccflag deccorig_fln decctool_fln");
   RETURN;
}

int decc_purgewhat(int argc, char*argv[], int retc, char *retv[])
{
char   decclibpath[MAXSTR], fullpath[MAXSTR];
char   tmpStr[MAXSTR],      filename[MAXSTR];
int    biggest;
   if (argc>1) debug=2;
   else        debug=0;
   if (argc < 2)
   {  Werrprintf("Usage -- decc_purge(filename)");
      RETURN;
   }
   strcpy(filename,argv[1]);
   if (debug) Winfoprintf("decc_purgewhat: inputfile='%s'\n",filename);
   // strip version if any
   getVersion(filename);
   if (debug) Winfoprintf("decc_purgewhat: minus version='%s'\n",filename);

   // check decclib for highest version
   strcpy(decclibpath,systemdir);
   strcat(decclibpath,"/imaging/decclib/");
   strcpy(fullpath,decclibpath);
   strcat(fullpath,filename);

   biggest = getBiggestVersion(fullpath);
   if (debug) Winfoprintf("decc_purgewhat: count=%d\n",bigCount);

   if (bigCount != 1)
   {  sprintf(tmpStr,
      "This will delete all versions of \"%s\" except the latest: version %d",
      filename,biggest);
   }
   else
   {  sprintf(tmpStr,
      "This will delete the last and only version of \"%s\"",
      filename);
   }
   
   if (retc > 0) 
      retv[0] = newString(tmpStr);
   else
     Winfoprintf("%s",tmpStr);
      
   RETURN;
}

int decc_purge(int argc, char *argv[], int retc, char *retv[])
{
char   decclibpath[MAXSTR], fullpath[MAXSTR];
char   tmpFln[MAXSTR], filename[MAXSTR], cmd[MAXSTR];
int    j,version, biggest;
FILE  *fd;
   (void) retc;
   (void) retv;
   if (argc>2) debug=1;
   if (argc>2) debug=1;
   if (argc>2) debug=1;
   else        debug=0;
   if (argc < 2)
   {  Werrprintf("Usage -- decc_purge(filename)");
      RETURN;
   }
   strcpy(filename,argv[1]);
   if (debug) Winfoprintf("decc_purge: inputfile='%s'\n",filename);
   // strip version if any
   getVersion(filename);
   if (debug) Winfoprintf("decc_purge: minus version='%s'\n",filename);

   // check decclib for highest version 
   strcpy(decclibpath,systemdir);
   strcat(decclibpath,"/imaging/decclib/");
   strcpy(fullpath,decclibpath);
   strcat(fullpath,filename);

   biggest = getBiggestVersion(fullpath);
   if (debug) Winfoprintf("decc_purgewhat: count=%d\n",bigCount);

   strcpy(cmd,"ls ");
   strcat(cmd,fullpath);
   strcat(cmd,".*");
   
   // remove all-but-one or the-one
   if (bigCount != 1)
   {  fd = popen(cmd, "r");
      j = fscanf(fd,"%s",filename);
      while (j>0)
      {
         strcpy(tmpFln,filename);	// don't loose version
         version = getVersion(filename);
         if (version != biggest) 
         {  if (debug) Winfoprintf("deleting '%s'",tmpFln);
            if (unlink(tmpFln) != 0) 
               perror("decc_purge");
         }
         j = fscanf(fd,"%s",filename);
      }
      pclose(fd);
   }
   else	// the-one
   {  sprintf(tmpFln,"%s.%d",fullpath,biggest);
      if (debug) Winfoprintf("deleting '%s'",tmpFln);
      if (unlink(tmpFln) != 0)
         perror("decc_purge");
   }
   RETURN;
}
