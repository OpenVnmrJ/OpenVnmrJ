/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/****************************************/
/* vnmrsys.h	vnmr system definitions */
/****************************************/

#ifndef vnmrsys_header_included
#define vnmrsys_header_included 

extern int BigEndian;

#define UNIX			/* for software on UNIX system */
#ifndef SUN
#define SUN			/* for software on sun system */
#endif
#define VNMR			/* this is the VNMR program */

#define MAXPATHL 128		/* maximum path length in vnmr environment */
#define MAX_FKEYS 8		/* maximum number of function keys */
#define MAXACQEXPS 9999		/* maximum number of acquisition expts */
#define MAXEXPS	9999		/* maximum number of experiments */

/*  There is a divergence for the maximun length of a path name.
 *  Solaris allows 1024 characters and defines MAXPATHLEN in sys/params.h.
 *  The file limits.h defines PATH_MAX as 1024 and _POSIX_PATH_MAX as 255.
 *  Vnmr defines MAXPATHL as 128.  However MAXPATHL has been used for many
 *  different things besides path names.  A new define MAXPATH will be
 *  conservatively set to the POSIX limit.  MAXPATH should not be used
 *  except for pathname variables.  MAXPATHL should be phased out.  However,
 *  at this point, it seems a little risky until MAXPATH is more widely
 *  implemented.  Therefore, for the time being, systemdir, userdir, and
 *  curexpdir will remain with MAXPATHL.
 *
 *  MAXSTR , STR128 or STR64 should be used for string variables.
 */

#define MAXPATH 256
#define MAXSTR  256
#define STR128  128
#define STR64   64

/* Place RETURN     in your c program command when you want a normal return */
/* Place ABORT      in your c program command when you want a run-time error*/
#define RETURN		return(0)  
#define ABORT		return(1)  

#define MAXSPEC  512

/* these file path names are made available to application programs */

extern char systemdir[];	/* vnmr system directory */
extern char userdir[];		/* vnmr user system directory */
extern char curexpdir[];	/* current experiment path */
extern char UserName[];
extern char  HostName[];

extern int Bnmr;
extern int interuption;


#ifdef VNMRJ
#ifdef __cplusplus
extern "C" {
#endif
int  execString(const char *buffer);
void appendvarlist(const char *name);
void releasevarlist();
int  writelineToVnmrJ(const char *cmd, const char *message );
void appendJvarlist(const char *name);
#ifdef __cplusplus
}
#endif

#endif   /* VNMRJ */

#endif
