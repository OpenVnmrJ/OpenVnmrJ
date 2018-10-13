/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

/*
			The VNMR Lock System


Whenever a VNMR process wants to access an experiment, it is expected to
lock that experiment before accessing any data.  Establishing this lock
is the responsibility of the person writing the VNMR program.  Nothing
exists in UNIX (or VMS) to prevent two processes from accessing a single
experiment simultaneously.

Two levels of locking are required.  The second level exists to prevent
a race between processes, wherein one finds an experiment is available,
but before it can establish a lock, another process has already placed
a lock on the experiment.  This race is prevented by insisting that a
process obtain the secondary lock before checking for a primary lock or
obtaining the primary lock.  We use the O_EXCL bit with the secondary
lock to guarantee exclusive and sequential access.

The primary lock file is in the User's VNMR directory.  Its name is
`lock_N.primary', where N is the experiment number 1 to 9.  The
secondary lock file has a similar name:  `lock_N.secondary'.  For a
particular experiment, the primary and the secondary lock file names
are unique.

Both the primary and the secondary locks are released by deleting the
corresponding file.

The primary lock file contains the Mode of VNMR, the InterNet host name
and the ID number of the VNMR process.  The Mode of VNMR is a small
number, with values defined in `locksys.h'.

The VNMR system obtains a primary lock at bootup, when joining another
experiment and when the target experiment of the "mf", "mp" or "md" 
command is not the current experiment.  In the latter case, the program
has two experiments locked at the same time.  The program also keeps
two experiments locked briefly while changing experiments, keeping the
lock on the old experiment until a lock is established on the new
experiment.								*/

#include "vnmrsys.h"
#include "locksys.h"
#include "tools.h"
#include "wjunk.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>

#ifdef UNIX
#include <sys/types.h>
#include <sys/stat.h>
#else 
#include file
#define F_OK		0
#include "unix_io.h"
#endif 

#include <dirent.h>
#include <string.h>
#include <signal.h>


#define SEC_LOCK_COUNT	10

int locklc_(char *cmd, int pid);
extern pid_t HostPid;
extern int nscmp(char *s1, char *s2);
extern FILE *popen_call(char *cmdstr, char *mode);
extern int pclose_call(FILE *pfile);
extern int expdir_to_expnum(char *expdir);

static void get_primary_lockfile(int expn, char *userptr, char *tptr )
{
	char	tmpbuf[ MAXPATHL ];

	sprintf( &tmpbuf[ 0 ], "lock_%d.primary", expn );
	strcpy( tptr, userptr );
#ifdef UNIX
	strcat( tptr, "/" );
#endif 
	strcat( tptr, &tmpbuf[ 0 ] );
}

/*  Assumes the process has the secondary lock for the experiment.  */

static int make_primary_lock(int expn, int mode )
{
	char	 lock_file_name[ MAXPATHL ];
	FILE	*lkfptr;

	get_primary_lockfile( expn, userdir, &lock_file_name[ 0 ] );
	if (access( &lock_file_name[ 0 ], F_OK ) == 0)
	  unlink( &lock_file_name[ 0 ] );
	lkfptr = fopen( &lock_file_name[ 0 ], "w" );
	if (lkfptr == NULL) {
		return( -1 );
	}
	fprintf( lkfptr, "%d %s %d\n", mode, &HostName[ 0 ], HostPid );
	fclose( lkfptr );
	return( 0 );
}

static int
verify_primary_lock(char *lockfile, int *modeptr, char *hostptr, int *pidptr )
{
	char	 field_1[ MAXPATHL ], field_2[ MAXPATHL ], field_3[ MAXPATHL ];
	int	 iter, ival, len3, mode_of_lock;
	FILE	*lkfptr;

	lkfptr = fopen( lockfile, "r" );
	if (lkfptr == NULL) {
		return( -1 );
	}

/*  the three scanf fields are separated by blank characters  */

	ival = fscanf( lkfptr, "%s%s%s", &field_1[ 0 ], &field_2[ 0 ], &field_3[ 0 ] );
	fclose( lkfptr );	/* be sure lkfptr is closed before returning */
	if (ival != 3)
	  return( -1 );

/*  field 1 contains the Mode of VNMR.
    field 2 contains the host name
    field 3 contains the Process ID.	*/

/*  Verify the mode of VNMR.  */

	mode_of_lock = atoi( &field_1[ 0 ] );
	if (mode_of_lock < FIRST_MODE || mode_of_lock > NUM_LOCK_MODES)
	  return( -1 );

/*  Verify the process ID contains digits only.  */

	len3 = strlen( &field_3[ 0 ] );
	for (iter = 0; iter < len3; iter++) {
		if (!isdigit( field_3[ iter ] ))
		  return( -1 );
	}

/*  This completes the lock file verification process.  Return stuff
    from the file to the calling program.  Return status of success.	*/

	if (modeptr != NULL)
	  *modeptr = mode_of_lock;
	if (hostptr != NULL)
	  strcpy( hostptr, &field_2[ 0 ] );
	if (pidptr != NULL)
	  *pidptr = atoi( &field_3[ 0 ] );

	return( 0 );
}

static int get_secondary_lock(int expn, int count )
{
	char	sname[ MAXPATHL ];
	int	iter, lfd;
        sigset_t omask, qmask;

#ifdef UNIX
	sprintf( &sname[ 0 ], "%s/lock_%d.secondary", userdir, expn );
#else 
	sprintf( &sname[ 0 ], "%slock_%d.secondary", userdir, expn );
#endif 

/*
 * There is an apparent bug with the open() system call.  If it is interuptted by a
 * signal, it exits with a return value of -1 and errno is set to EINTR.  However,
 * at least for the case below, the file is actually created and given permissions
 * of 000. The next and all future times though the loop, the open call will give an
 * error that the file already exists.  It could have been fixed by removing the file
 * if (errno == EINTR), but that would then fail if Sun ever fixes this bug.  Blocking
 * signals so that the open command will never be interuptted seems like a good
 * alternative.
 */
	if (count < 1)
	  count = 1;
        sigfillset( &qmask);
	for (iter = 0; iter < count; iter++) {
                sigprocmask( SIG_BLOCK, &qmask, &omask );
		lfd = open( &sname[ 0 ], O_CREAT | O_EXCL, 0666 );
                sigprocmask( SIG_SETMASK, &omask, NULL );
		if (lfd >= 0) {
			close( lfd );
			return( 0 );
		}
		sleep( 1 );
	}

	return( -1 );					/* not successful */
}

static int remove_secondary_lock(int expn )
{
	char	sname[ MAXPATHL ];

#ifdef UNIX
	sprintf( &sname[ 0 ], "%s/lock_%d.secondary", userdir, expn );
#else 
	sprintf( &sname[ 0 ], "%slock_%d.secondary", userdir, expn );
#endif 
	return( unlink( &sname[ 0 ] ) );
}

#ifdef SUN
/*  Acquisition mode represents a special case of background processing
    in which the parent of Vnmr is Acqproc.  To prevent a race between
    different processes, the Acqproc itself must establish the lock.  Thus
    when Vnmr runs in acquisition mode, at bootup it only checks for the
    existence of the primary lock file.                                 */

int checkAcqLock(int expn )
{
	char	host_of_lock[ MAXPATHL ], lock_file_name[ MAXPATHL ];
	int	pid_of_lock, mode_of_lock;
	
        if (expn < 1 || expn > MAXEXPS) return( -1 );

/*  The primary lock must already exist.
    The mode must be acquisition, the Host Name stored in the
    lock file must be that of the current host and the lock PID
    must be that of the parent of this process.			*/

	get_primary_lockfile( expn, userdir, &lock_file_name[ 0 ] );
	if (verify_primary_lock(
		&lock_file_name[ 0 ], &mode_of_lock, &host_of_lock[ 0 ], &pid_of_lock
	) != 0)
	  return( -1 );
	if (mode_of_lock != ACQUISITION)
	  return( -1 );
	if (strcmp( &host_of_lock[ 0 ], &HostName[ 0 ] ) != 0)
	  return( -1 );

	return( 0 );
}
#endif 

int lockExperiment(int expn, int mode )
{
	char	lock_file_name[ MAXPATHL ];
	int	mode_of_lock, pid_of_lock, ret;

        if (expn < 1 || expn > MAXEXPS) return( -1 );
        if (mode < FIRST_MODE || mode > NUM_LOCK_MODES) return( -1 );
        if (mode == AUTOMATION) return( 0 );

	if (get_secondary_lock( expn, SEC_LOCK_COUNT ) != 0)
	  return( -2 );

/*  Do not forget to remove the secondary lock file
    before returning from this routine !!!		*/

	get_primary_lockfile( expn, userdir, &lock_file_name[ 0 ] );
	if (access( &lock_file_name[ 0 ], F_OK ) == 0) {
             pid_of_lock = 0;
             mode_of_lock = 0;
             ret = verify_primary_lock( &lock_file_name[ 0 ], &mode_of_lock,
                        NULL, &pid_of_lock);
             if (ret == 0) {
                  if (pid_of_lock > 0) {
                      if (mode_of_lock == mode)
                          ret = kill(pid_of_lock, 0);
                  }
                  if (ret == 0) {
			remove_secondary_lock( expn );
			return( mode_of_lock );
		  }
             }
	}

	if (make_primary_lock( expn, mode ) != 0) {
		remove_secondary_lock( expn );
		return( -3 );
	}

	remove_secondary_lock( expn );
	return( 0 );
}

/*  The `mode' argument is no longer user.  It is being kept to maintain
    compatibility with the older versions of the VNMR lock system.	*/

int unlockExperiment(int expn, int mode )
{
	char	lock_file_name[ MAXPATHL ];

        if (mode == AUTOMATION) return( 0 );

	get_primary_lockfile( expn, userdir, &lock_file_name[ 0 ] );
	return( unlink( &lock_file_name[ 0 ] ) );
}

int unlockAllExp(char *userptr, int target_pid, int mode )
{
	char		 fname[ MAXPATHL ];
	char		 host_of_lock[ MAXPATHL ];
	int		 expn, ival, mode_of_lock, pid_of_lock, ulen;
	DIR		*dir_stream;
	struct dirent	*cur_entry;

        if (mode < FIRST_MODE || mode > NUM_LOCK_MODES) return( -1 );
	ulen = strlen( userptr );
	if (ulen < 1 || ulen > MAXPATHL-20)
	  return( -1 );


	dir_stream = opendir( userptr );
	if (dir_stream == NULL) {
		perror( "access user's VNMR directory in unlock all exps" );
		return( -1 );
	}

	while ( (cur_entry = readdir( dir_stream )) != NULL ) {
		if (nscmp( "lock_*.primary", cur_entry->d_name )) {
			ival = sscanf( cur_entry->d_name, "lock_%d.primary", &expn );
			if (ival != 1)
			  continue;

			get_primary_lockfile( expn, userptr, &fname[ 0 ] );
			if (verify_primary_lock(
			    &fname[ 0 ], &mode_of_lock, &host_of_lock[ 0 ], &pid_of_lock
			) == 0) {
				if (mode_of_lock == mode &&
				    pid_of_lock == target_pid &&
				    strcmp( HostName, &host_of_lock[ 0 ] ) == 0)
				  unlink( &fname[ 0 ] );
			}
			else
			  unlink( &fname[ 0 ] );	/* remove bad lock file */
		}
	}

	return( 0 );
}

/*  Returns 0 if experiment in designated VNMR User Directory
      is not locked.
    Returns small positive number if locked (mode of lock).
    Returns negaive number if error.  Two errors:  the experiment
    number is out-of-range; the secondary lock could not be
    obtained.

    If this function reports an experiment is not locked, that
    does not guarantee that `lockExperiment' will succeed in
    locking the experiment.  Another process could lock it in
    the intervening interval.					*/

int is_locked(int expn, int *modeptr, char *hostptr, int *pidptr )
{
	char	lock_file_name[ MAXPATHL ];
	int	ival, mode_of_lock;

        if (expn < 1 || expn > MAXEXPS) return( -1 );
	if (get_secondary_lock( expn, SEC_LOCK_COUNT ) != 0)
	  return( -2 );

	get_primary_lockfile( expn, userdir, &lock_file_name[ 0 ] );
	if (access( &lock_file_name[ 0 ], F_OK ) != 0) {
		remove_secondary_lock( expn );
		return( 0 );
	}

/*  We define `mode_of_lock' as a local variable in case the calling
    routine did not provide an location to store it (that is, modeptr
    is the NULL address.						*/

	ival = verify_primary_lock(
		&lock_file_name[ 0 ], &mode_of_lock, hostptr, pidptr
	);

	remove_secondary_lock( expn );
	if (ival != 0) {
		mode_of_lock = 0;		/* which means not locked */
	}
	else {
		if (modeptr != NULL)
		  *modeptr = mode_of_lock;
	}
	return( mode_of_lock );
}

/*  Following is compiled only for VNMR because the Acqproc does
    not use it 	*/

#ifdef VNMR

/*  Check unlock argument verifies the "unlock" command has a single
    argument, and that it is a valid experiment number.  It returns -1
    if unsuccessful; the experiment number if successful                */
 
static int check_unlock_args(int argc, char*argv[], int *force )
{
        int     badf, ival;
 
        *force = 0;
        ival = 0;
        badf = ((argc != 2) && (argc != 3));
        if ( !badf )
        {
          if ( !isReal( argv[ 1 ] ) ) {
                if ( strncmp( "exp", argv[ 1 ], 3 ) == 0 &&
                     isdigit( *(argv[ 1 ] + 3) ) )
                  ival = atoi( argv[ 1 ] + 3 );
                else
                  badf = 1;
          }
          else
            ival = atoi( argv[ 1 ] );
        }
        if ((argc == 3) && (strcmp("force",argv[2]) == 0))
           *force = 1;
 
        if ( badf ) {
                Werrprintf( "usage - %s(exp#) or %s(exp#,'force')",
                           argv[ 0 ], argv[0] );
                return( -1 );
        }
 
        if (ival < 1 || ival > MAXEXPS) {
                Werrprintf( "%s:  %d is not a valid experiment number",
                        argv[ 0 ], ival
                );
                return( -1 );
        }
        return( ival );
}

/*  Look for another process with the "ps xa" command.
    Returns 1 if found; 0 if not found; -1 if error.

    Returns 0 (which should tell the calling routine to
    proceed with unlocking the experiment) if current
    PID is the same as the argument to this routine.

    If it finds the process, it checks if the target is
    actually runnning Vnmr.				*/

static int find_pid(int target_pid, char *curcmd )
{
	char	pipedata[ 122 ];
        char    field2[ 10 ], field3[ 10 ], field4[ 10 ], field5[ 122 ];
	char	*cptr;
	int	found_it, is_vnmr, iter, ival, proc_id;
	FILE	*fpipe;

	if (curcmd == NULL) curcmd = "unlock";

	if (target_pid == HostPid)		/* Added, 03/04/88 */
	  return( 0 );

#ifdef UNIX
#if defined (IRIX) || (SOLARIS)
	fpipe = popen_call( "ps -e", "r" );
#else 
	fpipe = popen_call( "ps xa", "r" );
#endif 
#else 
	fpipe = popen( "show system", "r" );
#endif 
	if (fpipe == NULL) {
		Werrprintf( "%s:  cannot search process table", curcmd );
		return( -1 );
	}

/*  First two lines of output from 'show system' command are ignored.	*/

#ifdef VMS
	cptr = fgets( &pipedata[ 0 ], 120, fpipe );
	cptr = fgets( &pipedata[ 0 ], 120, fpipe );
#endif 

	found_it = 0;
	is_vnmr = 0;
	do {
		cptr = fgets( &pipedata[ 0 ], 120, fpipe );
		if (cptr == NULL) break;

#ifdef UNIX
#if defined (IRIX) || (SOLARIS)
                ival = sscanf(
                        &pipedata[ 0 ], "%d%s%s%s",
                        &proc_id,
                        &field2[ 0 ],
                        &field3[ 0 ],
                        &field4[ 0 ]
                );
		if (ival != 4) continue;
#else 
                ival = sscanf(
                        &pipedata[ 0 ], "%d%8s%8s%8s%120s",
                        &proc_id,
                        &field2[ 0 ],
                        &field3[ 0 ],
                        &field4[ 0 ],
                        &field5[ 0 ]
                );
		if (ival != 5) continue;
#endif 
#else 
		ival = sscanf( &pipedata[ 0 ], "%x", &proc_id );
#endif 

	/*  When examining the "for" loop, remember the condition
	    for exiting is "less than", not "less than or equal".  */

		if (target_pid == proc_id) {
			found_it = 131071;
#ifdef UNIX
#if defined (IRIX) || (SOLARIS)
			for (iter = 0;
			     iter < strlen( &field4[ 0 ] ) - 3;
			     iter++) {
				is_vnmr = strncmp(
					&field4[ iter ], "Vnmr", 4 ) == 0;
				if (is_vnmr) break;
			}
#else 
			for (iter = 0;
			     iter < strlen( &field5[ 0 ] ) - 3;
			     iter++) {
				is_vnmr = strncmp(
					&field5[ iter ], "Vnmr", 4 ) == 0;
				if (is_vnmr) break;
			}
#endif 
#else 
			is_vnmr = 131071;
			break;
#endif 
		}

	} while (cptr != NULL && !found_it);

	pclose_call( fpipe );
	return( is_vnmr );
}

/*  Errors that will happen in normal operation are displayed with
    both Werrprintf and Wscrprintf, so the user can see the problem
    if another error overwrites the error line.                         */

int vnmr_unlock(int argc, char *argv[], int retc, char *retv[] )
{
	char		host_of_lock[ MAXPATHL ], jexpcmd[ 16 ],
			lock_file_name[ MAXPATHL ];
	int		expn, ival, mode_of_lock, pid_of_lock, force;

	expn = check_unlock_args( argc, argv, &force );
        if (expn == expdir_to_expnum(curexpdir))
           RETURN;
	if (expn < 1) ABORT;

	if (get_secondary_lock( expn, SEC_LOCK_COUNT ) != 0) {
		Werrprintf( "%s: cannot obtain secondary lock", argv[ 0 ] );
		ABORT;
	}
	get_primary_lockfile( expn, userdir, &lock_file_name[ 0 ] );
	ival = verify_primary_lock(
		&lock_file_name[ 0 ], &mode_of_lock, &host_of_lock[ 0 ], &pid_of_lock
	);
	remove_secondary_lock( expn );

	if (ival != 0) {
                if (access( &lock_file_name[ 0 ], F_OK ) == 0)
		  unlink( &lock_file_name[ 0 ] );
                sprintf( &jexpcmd[ 0 ], "jexp(%d)\n", expn );
                execString( &jexpcmd[ 0 ] );
                RETURN;
        }

	if (!force && (mode_of_lock == ACQUISITION)) {
		Werrprintf( "%s:  cannot remove acquisition lock", argv[ 0 ] );
		ABORT;
	}

	disp_status( "UNLOCK  " );
	if (!force && (strcmp( &host_of_lock[ 0 ], &HostName[ 0 ] ) != 0)) {
                Werrprintf( "%s:  experiment locked by remote host %s",
                        argv[ 0 ], &host_of_lock[ 0 ]
                );
                disp_status( "        " );
                ABORT;
        }
 
	ival = find_pid( pid_of_lock, argv[ 0 ] );
	if ( ival < 0) {
		disp_status( "        " );
		ABORT;
	}
	else if (!force && (ival > 0)) {
                Werrprintf(
                    "%s:  experiment locked by active process", argv[ 0 ]
                );
                disp_status( "        " );
                ABORT;
        }

	ival = unlink( &lock_file_name[ 0 ] );
	if (ival != 0) {
                Werrprintf( "%s:  unable to remove lock file", argv[ 0 ] );
                disp_status( "        " );
                ABORT;
        }

	Winfoprintf( "experiment %d unlocked", expn );
	disp_status( "        " );
	sprintf( &jexpcmd[ 0 ], "jexp(%d)\n", expn );
	execString( &jexpcmd[ 0 ] );
	RETURN;
}

/**
 * Call without arguments, or argv[1]='lock', to obtain a lock for
 * control of LC modules..
 * Call with argv[1]='unlock' to delete a previously acquired lock
 * file.  (Always returns TRUE.)
 * Returns 1 (TRUE) if lock is obtained, otherwise 0 (FALSE).
 * If successful, leaves a lock file behind in /vnmr/acqqueue.
 */
int locklc(int argc, char *argv[], int retc, char *retv[])
{
    double rtn;
    char *cmd = "lock";

    if (argc > 1) {
        cmd = argv[1];
    }
    rtn = locklc_(cmd, 0);
    if (retc > 0) {
        retv[0] = realString(rtn);
    }
    RETURN;
}

/**
 * Arg: cmd = 'unlock' removes the lock file; anything else tries to
 *      get the lock.
 * Arg: pid = 0 means to use this process's PID; anything else means
 *      to use that for the PID.
 */
int locklc_(char *cmd, int pid)
{
    /*
     * Create/open lockfile1 with exclusive access to avoid a race
     * with other processes; if we create it, we always delete this
     * file before we return.  Then create a lock file with our PID
     * appended to the name.  This file should be deleted when we are
     * done.  If we leave it when our process exits, it will likely
     * be cleaned up by the next process to request a lock.
     */
    char path[MAXPATH];
    char lockfile1[MAXPATH];
    char lockfile2[MAXPATH];
    int lockfd;
    DIR *dirp = NULL;
    struct dirent *dp;
    int mypid;

    char basename[] = "lock-LC.";
    int ok = 1;

    mypid = pid ? pid : HostPid;
    sprintf(path,"%s/acqqueue", systemdir);
    sprintf(lockfile1,"%s/%s", path, basename);
    sprintf(lockfile2,"%s/%s%d", path, basename, mypid);

    if (strcmp(cmd, "unlock") == 0) {
        unlink(lockfile2);
        return 1;
    }

    /*  O_EXCL bit requests exclusive access. */
    if ((lockfd = open(lockfile1, O_CREAT | O_EXCL, 0666)) < 0) {
        /* Looks like someone else is already getting the lock */
        ok = 0;
    }

    /* Check for a previous lock file owned by a live process */
    if (ok && (dirp = opendir(path))) {
        int len = strlen(basename);
        while ((dp = readdir(dirp)) != NULL) {
            char *name = dp->d_name;
            if (strncmp(name, basename, len) == 0 && name[len] != '\0') {
                /* Found a candidate - check if its process is alive */
                int ipid = atoi(name + len);
                if (ipid > 0 && ipid != mypid && kill(ipid, 0) == 0) {
                    /* Failed to get the lock */
                    ok = 0;
                    break;
                } else {
                    /* Delete old lock file (maybe mine) */
                    char pathname[MAXPATH];
                    sprintf(pathname,"%s/%s", path, name);
                    unlink(pathname);
                }
            }
        }
    }

    if (ok) {
        int fd;
        fd = creat(lockfile2, 0666);
        if (fd < 0) {
            /* This shouldn't happen, but ... */
            ok = 0;
        } else {
            fchmod(fd, 0666); /* Make sure others can delete it easily */
            close(fd);
        }
    }

    /* Clean up */
    if (dirp) {
        closedir(dirp);
    }
    if (lockfd >= 0) {
        unlink(lockfile1);
        close(lockfd);
    }

    return ok;
}
    
#endif
