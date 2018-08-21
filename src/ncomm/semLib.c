/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#ifndef VNMRS_WIN32
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <sys/types.h>
  #include <sys/ipc.h>
  #include <sys/sem.h>
  #include <sys/stat.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <signal.h>
  #include <errno.h>
  #include "errLogLib.h"
  #include "semLib.h"
  #include "mfileObj.h"
#else
  #include <Windows.h>
  #include <stdio.h>
  #include "inttypes.h"
  #include "errLogLib.h"
  #include "semLib.h"
#endif

/*
modification history
--------------------
5-17-06,gmb  ifdefs for native win32 compilation with VS 2003
9-17-96,rol  clear umask before opening files; restore afterwards
             lets most of Vnmr utilize umask whereas here its effects
             must be suppressed for multiple users of Vnmr
5-03-95,gmb  added semaphore database in /tmp to cleanup lost semaphores
6-24-94,gmb  created

*/


/*
DESCRIPTION
This library provide a VxWorks style interface the the System V
semaphore routines. The semaphore consists of a 3 members.
The 1st is the semaphore value. 2nd is a process counter or how many
pocesses have opened this semaphore for use. This allows the semaphore
to be remove from the system when the last process closes it. 3rd
member is a lock to avoid race conditions in certain semaphore
operations.

Interface provided:

    SEM_HANDLE semId = semCreate((key_t) SEMKEY, int intialValue)
    SEM_HANDLE semId = semOpen((key_t) SEMKEY, int intialValue)
    semTake(semID)
    semGive(semID)
    semDelete(semID)

    Note: Present semTake has no timeout parameter.
	  This maybe a future enhancement if required.
*/


#ifndef VNMRS_WIN32

/* UNIX using System V semaphores */

#define INITCOUNT 20000
#define SEMVALUE  0
#define PROC_CNT   1
#define CREAT_CLOSE_RACELCK 2

#ifndef SEM_A
#define SEM_A 0200
#endif
#ifndef SEM_R
#define SEM_R 0400
#endif

#define SEM_OWNR_PERM (SEM_A | SEM_R)
#define SEM_GRP_PERM  ((SEM_A >> 3) | (SEM_R >> 3))
#define SEM_WRLD_PERM ((SEM_A >> 6) | (SEM_R >> 6))
#define SEM_ALL_PERM  (SEM_OWNR_PERM | SEM_GRP_PERM | SEM_WRLD_PERM)
/*
* Define operations for semop on semaphores 
*/

static struct sembuf	semLockOps[2] = 
{
   {CREAT_CLOSE_RACELCK, 0, 0},		/* wait for race lock to equal 0 */
   {CREAT_CLOSE_RACELCK, 1, SEM_UNDO} 	/* then increment race lock to 1 */
					/* ie lock it */
					/* UNDO to release lock if precess exits
			   		   before explicity unlocking */
};

static struct sembuf	semFinCreatOps[2] = 
{
   {PROC_CNT, -1, SEM_UNDO},		/* decrement Process Cntr with undo */
					/* on exit UNDO to adjust process 
					   counter if process exits prior to 
					   closing sem */
   {CREAT_CLOSE_RACELCK, -1, SEM_UNDO}  /* then decrement race lock back */
					/* to 0 */
};


static struct sembuf	semOpenOps[1] = 
{
   {PROC_CNT, -1, SEM_UNDO}	 	/* decrement [1] (process cntr) with */
					/* undo on exit */
};


static struct sembuf	semCloseOps[3] = 
{
   {CREAT_CLOSE_RACELCK, 0, 0},	 	/* wait for race lock to equal 0 */
   {CREAT_CLOSE_RACELCK, 1, SEM_UNDO}, 	/* then incr race lock to 1 */
					/* (ie lock it) */
   {PROC_CNT, 1, SEM_UNDO}	 	/* then incrment process cntr */
};


static struct sembuf	semUnlockOps[1] = 
{
   {CREAT_CLOSE_RACELCK, -1, SEM_UNDO}	/* decrement race lock back to 0 */
};

static struct sembuf	semOpOps[1] = 
{
	{SEMVALUE, 99, SEM_UNDO}		/* decr or incr semaphore with */
					/* undo on exit, the 99 is set */
					/* to the actual amount to add
			   		   or subtract (pos or neg) */
};

/**************************************************************
*
* blockSignals
*   block all signals 
*/
static void blockSignals(sigset_t *pOldMask)
{
   sigset_t	blockMask;
/*
   sigemptyset( &blockMask );
   sigaddset( &, SIGUSR1 );
   sigaddset( &, SIGUSR2 );
*/
   sigfillset( &blockMask );
   sigprocmask( SIG_SETMASK, &blockMask, pOldMask );
}

/**************************************************************
*
* unblockSignals
*   re-establish old signal mask
*/
static void unblockSignals(sigset_t *pOldMask)
{
   sigprocmask( SIG_SETMASK, pOldMask, NULL );
}



/* for Windows no need for DBM since when all reference to a semaphore are gone it is deleted by system
  unlike System V IPC semaphores
*/
/**************************************************************
*
* semDbmAdd
*   add an entry into the semaphore DBM
*   entry consists of PID & SEMID
*/
void semDbmAdd(int id)
{
   int semfd;
   char semstr[SEM_RECORD_SIZE];
   mode_t old_umask;

   old_umask = umask( 000 );
   semfd = open(SHR_SEM_USAGE_PATH,O_RDWR | O_CREAT | O_APPEND, 0666);
   if (semfd > 0)
   {
      memset(semstr,0,SEM_RECORD_SIZE);
      sprintf(semstr,"%d %d\n",getpid(),id);
      /* printf("semCreate: semstr= '%s'\n",semstr); */
      if (write(semfd,semstr,sizeof(semstr)) < 1)
	perror("semCreate: Write to sem dbm failed.");
      close(semfd);
   }
   umask( old_umask );
}

/**************************************************************
*
*  semClean()
*   1. read in semaphore DBM
*   2. Check PIDs is the process active, write active entries out 
*	to a temp file dbm, also compile a list of acitve SEMIDs
*   3. Reread semaphore DBM check SEMIDs against active ones,
*       if SEMID is non-active then remove it.
*   4. Copy tmp DBM of active processes into the original DBM
*   5. All Done
*				Author: Greg Brissey
*
*/
int semClean()
{
   int i;
   int semfd,tmpfd;
   char *tmptoken;
   char tmpfile[32];
   char semstr[SEM_RECORD_SIZE];
   int pid,semid;
   int stat;
   mode_t old_umask;
   struct semid_ds realbuf;
   union semun {
		int val;
		struct semid_ds *buf;
		ushort		*array;
	       } semctl_arg;

   old_umask = umask( 000 );
   semfd = open(SHR_SEM_USAGE_PATH,O_RDWR | O_APPEND, 0666);
   if (semfd < 0)
   {
      DPRINT1(1,"semClean: Semaphore DBM: '%s' not present.\n",SHR_SEM_USAGE_PATH);
      umask( old_umask );
      return(-1);
   } 
   strcpy(tmpfile,"/tmp/InovaXXXXXX");
   tmpfd = mkstemp(tmpfile);
   if (tmpfd < 0 )
   {
      perror("semClean: couldn't open temp sem dbm");
      close(semfd);
      umask( old_umask );
      return(-1);
   }

   umask( old_umask );
   i = 0;
   /* read in sem dbm find all active processes using semaphore */
   semctl_arg.buf = &realbuf;
   while (read(semfd,semstr,sizeof(semstr)) > 0)
   {
      tmptoken = (char*) strtok(semstr," ");
      pid = atoi(tmptoken);
      tmptoken = (char*) strtok(NULL," ");
      semid = atoi(tmptoken);
      if ( (stat=semctl(semid, 0, IPC_STAT, semctl_arg)) >= 0)
      {
         memset(semstr,0,SEM_RECORD_SIZE);
         sprintf(semstr,"%d %d\n",pid,semid);
         write(tmpfd,semstr,sizeof(semstr)); /* list of active pids */
      }
   }
   /* printf("semClean: Number of active semids: %d\n",i); */

   lseek(semfd,0,SEEK_SET);   /* back to the beginning */
   ftruncate(semfd, 0);		/* truncate file back to zero */
   lseek(semfd,0,SEEK_SET);   /* back to the beginning */
   lseek(tmpfd,0,SEEK_SET);   /* back to the beginning */

   /* now write back only active processes */
   while (read(tmpfd,semstr,sizeof(semstr)) > 0)
   {
        /* printf("write: '%s'\n",semstr); */
	write(semfd,semstr,sizeof(semstr));
   }
   close(semfd);
   close(tmpfd);
   unlink(tmpfile);
   return(0);
}
 
int semCreate(key_t key, int initval)
/* key_t key - key to semaphore */
/* int initval - initial value of semaphore */
{
   register int id, semvalue;
   sigset_t signalMask;

   union semun {
		int val;
		struct semid_ds *buf;
		ushort		*array;
	       } semctl_arg;


   if ( (key == IPC_PRIVATE) || (key == (key_t) -1) )
   {
     errLogRet(ErrLogOp,debugInfo,"semCreate: Invalid Key given.\n");
     return(-1); /* not for private sems or invalid key */
   }

   blockSignals(&signalMask);  /* block signals while working on the semaphores */

makeitagain:

   if ( (id = semget(key, 3, (SEM_ALL_PERM | IPC_CREAT))) < 0)
   {
      errLogSysRet(ErrLogOp,debugInfo,"semCreate: semget() error");
      unblockSignals(&signalMask);
      return(-1);  /* permission problem or kernel table full */
   }

   /* grab sempore and apply race lock to avoid race conditions */
   if (semop(id, &semLockOps[0], 2) < 0)
   {
      /* some one deleted before we could lock it, make it again Sam */
      if (errno == EINVAL)
	  goto makeitagain;
      errLogSysRet(ErrLogOp,debugInfo,
	"semCreate: semid: semop() can't take locking semaphore");
      semctl(id, 0, IPC_RMID, 0);
      unblockSignals(&signalMask);
      return(-1);
   }

   if ( (semvalue = semctl(id, 1, GETVAL, 0)) < 0 )
   {
      errLogSysRet(ErrLogOp,debugInfo,"semCreate: semid: %d, semctl() can't GETVAL:",
	id);
      semctl(id, 0, IPC_RMID, 0);
      unblockSignals(&signalMask);
      return(-1);
   }

   if (semvalue == 0)	/* 1st time so initialize values */
   {
      semctl_arg.val = initval;
      if (semctl(id, SEMVALUE, SETVAL, semctl_arg) < 0)
      {
         errLogSysRet(ErrLogOp,debugInfo,
	    "semCreate: semid: %d, semctl() can't SETVAL to SEMVALUE",
		id);
         semctl(id, 0, IPC_RMID, 0);
         unblockSignals(&signalMask);
         return(-1);
      }

      semctl_arg.val = INITCOUNT;
      if (semctl(id, PROC_CNT, SETVAL, semctl_arg) < 0)
      {
         errLogSysRet(ErrLogOp,debugInfo,
	    "semCreate: semid: %d, semctl() can't SETVAL to PROC_CNT",
		id);
         semctl(id, 0, IPC_RMID, 0);
         unblockSignals(&signalMask);
         return(-1);
      }
   }

   /* Decrement Process Counter and release Race Lock */
   if (semop(id, &semFinCreatOps[0], 2) < 0)
   {
         errLogSysRet(ErrLogOp,debugInfo,
	    "semCreate: semid: %d, semctl() can't finish create",
		id);
         semctl(id, 0, IPC_RMID, 0);
         unblockSignals(&signalMask);
         return(-1);
   }

   semDbmAdd(id);

   unblockSignals(&signalMask);

   return(id);
}

/**************************************************************
*
*  semOpen - Open a semaphore.
*
* This routine attempts to:
* 1. Opens a System V semaphore that already exist. 
* 2. If 1. fails then the semaphore is created.
*
*
* RETURNS:
* semaphore ID, else -1 
*
*       Author Greg Brissey 6/24/94
*/
int semOpen(key_t key, int initval)
/* key_t key - key to semaphore */
/* int initval - initial value of semaphore if it needs to be created */
{
   int id;
   sigset_t signalMask;

   /* printf("semOpen\n"); */
   if ( (key == IPC_PRIVATE) || (key == (key_t) -1) )
   {
     errLogRet(ErrLogOp,debugInfo,"semOpen: Invalid Key(%d) given.\n",key);
     return(-1); /* not for private sems or invalid key */
   }

   blockSignals(&signalMask);   /* block signals while working on the semaphores */

   if ( (id = semget(key, 3, 0)) < 0)
   {
      /* printf("semOpen: calling semCreate(%d,%d)\n",key,initval); */
      /* semCreate also decrements Process count. */
      if ( (id = semCreate(key, initval)) < 0)
      {
   	 unblockSignals(&signalMask);
         return(-1);  /* doesn't exist  or kernel table full */
      }
   }
   else
   {
     /* Decrement Process count. No Need to lock */
     if (semop(id, &semOpenOps[0], 1) < 0)
     {
        errLogSysRet(ErrLogOp,debugInfo,"semOpen: semid: %d semop error",id);
   	unblockSignals(&signalMask);
        return(-1);
     }
   }
   DPRINT1(1,"semOpen: SEMID: %d\n",id);

   unblockSignals(&signalMask);

   return(id);
}

/**************************************************************
*
*  semDelete - Remove a semaphore from system.
*
* This routine removes a System V semaphore that exist. 
*This routine is used were the application already knowns that
*all other processes are no longer using this semaphore.
*  
*
* RETURNS:
* VOID 
*
*       Author Greg Brissey 6/24/94
*/
void semDelete(int id)
/* int id - Semaphore ID */
{
   if(semctl(id, 0, IPC_RMID, 0) < 0 )
   {
      if ( (errno != EINVAL) && (errno != EPERM) )
      {
	errLogSysRet(ErrLogOp,debugInfo,
		"semDelete: semclt() can't delete semaphore: %d",id);
      }
   }
}


/**************************************************************
*
*  semClose - Close a semaphore.
*
* This routine closes a System V semaphore that already exist. 
*This is the normal routine used by an application to close the 
*a semaphore that it is done with or prior to exiting. 
*  
*
* RETURNS:
* VOID
*
*       Author Greg Brissey 6/24/94
*/
void semClose(int id)
/* int id - Semaphore ID */
{
    int semval;
   sigset_t signalMask;

   blockSignals(&signalMask);

    /* First Obtain Race Lock */
    if (semop(id, &semCloseOps[0], 3) < 0)
    {
	errLogSysRet(ErrLogOp,debugInfo,
		"semClose: semid: %d, semop() can't get lock",id);
    }

    /* if last process to be using semaphore then remove it, else
	decrement the process count */

    if ( (semval = semctl(id, PROC_CNT, GETVAL, 0)) < 0)
    {
	errLogSysRet(ErrLogOp,debugInfo,
		"semClose: semid: %d, semctl() can't GETVAL on PROC_CNT",id);
    }
 
    if (semval > INITCOUNT)
    {
      errLogRet(ErrLogOp,debugInfo,"semClose: PROC_CNT greater than initial value");
    }
    else if (semval == INITCOUNT)
    {
        DPRINT1(1,"semClose: delete semaphore: %d\n",id);
	semDelete(id);
    }
    else
    {
        DPRINT1(1,"semClose: just decrement process count for semaphore: %d\n",id);
        if (semop(id, &semUnlockOps[0], 1) < 0)
	   errLogSysRet(ErrLogOp,debugInfo,
		"semClose: semid: %d, semop() can't Unlock",id);
    }

    unblockSignals(&signalMask);
}

static void sem_op(int id, int value)
/* int id - semaphore id */
/* int value - amount to increment or decrement semaphore */
{
   if ( (semOpOps[0].sem_op = value) == 0)
     errLogRet(ErrLogOp,debugInfo,
		"sem_op: semid: %d, can't have value = 0",id);
   if ( semop(id, &semOpOps[0], 1) < 0)
     errLogSysRet(ErrLogOp,debugInfo,
		"sem_op: semid: %d, semop() error",id);
   return;
}


/**************************************************************
*
*  semTake - Take a semaphore block if already taken.
*
* This routine takes a System V semaphore and blocks if already taken. 
*  
*
* RETURNS:
* VOID
*
*       Author Greg Brissey 6/24/94
*/
void semTake(int id)
/* int id - semaphore id */
{
   sigset_t signalMask;

   blockSignals(&signalMask); /* block signals while working on the semaphores */

    sem_op(id, -1);

   unblockSignals(&signalMask);
   return;
}



/**************************************************************
*
*  semGive - Give a semaphore back.
*
* This routine gives a System V semaphore back. 
*  
*
* RETURNS:
* VOID
*
*       Author Greg Brissey 6/24/94
*/
void semGive(int id)
/* int id - semaphore id */
{
   sigset_t signalMask;

   blockSignals(&signalMask);   /* block signals while working on the semaphores */

    sem_op(id, 1);

   unblockSignals(&signalMask);
   return;
}



/* vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv*/

#else /* VNMRS_WIN32  Windows Port for System V Semaphore routines */

/* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/

/* 
  For Windows there is no need for the key DBM since when all references to a 
  Windows semaphore are gone it is deleted by system unlike System V IPC semaphores
*/

/**************************************************************
*
*  semCreate - Create a Windows Equivilent System V semaphore.
*
* This routine attempts to:
* 1. Creae a Windows semaphore. 
*
*
* RETURNS:
* semaphore ID, else NULL 
*
*       Author Greg Brissey 5/17/2006
*/
SEM_HANDLE semCreate(key_t key, int initval)
/* key_t key - Windows Semaphore Name */
/* int initval - initial value of semaphore, typically 1 */
{
	SEM_HANDLE id;
	char keyIdName[MAX_PATH];
	sprintf(keyIdName,"Global\\%d",key); /* Use Global name space */
	id = CreateSemaphore(NULL,(long) initval, (long) 1, keyIdName);
	if (id == 0)
	{
          LPVOID lpMsgBuf;
          if (FormatMessage( 
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_SYSTEM | 
                FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL,
                GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                (LPTSTR) &lpMsgBuf,
                0,
                NULL ))
            {       
	        // Display the string.
	        //MessageBox( NULL, (LPCTSTR)lpMsgBuf, "Error", MB_OK | MB_ICONINFORMATION );
	        printf("SemaphoreName: '%s', %s",keyIdName,lpMsgBuf);
	        // Free the buffer.
	        LocalFree( lpMsgBuf );
	    }
	}
   return(id);
}
/**************************************************************
*
*  semOpen - Open a semaphore.
*
* This routine attempts to:
* 1. Opens a WIndows semaphore that may or maynot already exist. 
* 2. If 1. fails then the semaphore is created.
*
*
* RETURNS:
* semaphore ID, else NULL 
*
*       Author Greg Brissey 5/17/2006
*/
SEM_HANDLE semOpen(key_t key, int initval)
/* key_t key - key to semaphore */
/* int initval - initial value of semaphore if it needs to be created */
{
	SEM_HANDLE id;
	id = semCreate(key, initval);
	return (id);
}

/**************************************************************
*
*  semDelete - Close the handle to the Windows semaphore.
*
* RETURNS:
* VOID 
*
*       Author Greg Brissey 5/17/2006
*/
void semDelete(SEM_HANDLE SemId)
{
	if (SemId != NULL)
	   CloseHandle(SemId);
}


/**************************************************************
*
*  semClose - Close a Windows semaphore.
*
* This routine closes a Windows semaphore that already exist. 
* Here fro compatibilty with Unix API
*  
*
* RETURNS:
* VOID
*
*       Author Greg Brissey 5/17/2006
*/
void semClose(SEM_HANDLE id)
/* int id - Semaphore ID */
{
	semDelete(id);
}

/**************************************************************
*
*  semTake - Take a semaphore block if already taken.
*
* This routine takes a Windows semaphore and blocks if already taken. 
*  
*
* RETURNS:
* VOID
*
*       Author Greg Brissey 5/17/2006
*/
void semTake(SEM_HANDLE semId)
/* int id - semaphore id */
{
	DWORD dwResult;
	if (semId == NULL)
	    return;

	dwResult = WaitForSingleObject(semId, INFINITE);
	switch(dwResult) {
		case WAIT_OBJECT_0:
			  /* printf("got semaphore\n"); */
			 break;
		 case WAIT_TIMEOUT:
			 printf("timed out on semaphore\n");
			 break;
	}
   return;
}



/**************************************************************
*
*  semGive - Give a semaphore back.
*
* This routine gives a Windows semaphore back. 
*  
*
* RETURNS:
* VOID
*
*       Author Greg Brissey 5/17/2006
*/
void semGive(SEM_HANDLE semId)
/* int id - semaphore id */
{
	if (semId == NULL)
		return;

	if (!ReleaseSemaphore(semId, 1, NULL) )
	{
		// error
	}
   return;
}


#endif
