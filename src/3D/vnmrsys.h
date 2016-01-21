/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*******************************************/
/* vnmrsys.h	3D vnmr system definitions */
/*******************************************/

#define UNIX			/* for software on UNIX system */
#define SUN			/* for software on sun system */

#define MAXPATHL 128		/* maximum path length in vnmr environment */
#define MAX_FKEYS 8		/* maximum number of function keys */

/* Place RETURN     in your c program command when you want a normal return */
/* Place ABORT      in your c program command when you want a run-time error*/
#define RETURN		return(0)  
#define ABORT		return(1)  


/* these file path names are made available to application programs */

extern char userdir[MAXPATHL];		/* vnmr user system directory */
extern int last_line;		/* last alpha line in window 4 */

struct wtparams
  { double sw,lb,sb,sbs,gf,gfs,awc;
    int lb_active,sb_active,sbs_active,gf_active,gfs_active,awc_active,wtflag;
    char lbname[6],sbname[6],sbsname[6],gfname[6],gfsname[6],awcname[6];
  };

