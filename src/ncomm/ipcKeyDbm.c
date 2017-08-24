/*
 * Copyright (C) 2015  University of Oregon
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Apache License, as specified in the LICENSE file.
 *
 * For more information, see the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>

#include <errno.h>

#include "errLogLib.h"
#include "shrMLib.h"
#include "ipcKeyDbm.h"

/*
modification history
--------------------
7-13-94,gmb  created
*/


/*
DESCRIPTION

    This library provides an interface into a data base of
 Inter-Process Communication (IPC) Keys. These
Keys are used in Message Q, Semaphores, and Shared Memory
facilities of System V.  The data base contains a searchable
id string to allow other process to obtain it's Key, 
pid of the process so that signals may be sent to it,
flag indicate if the process is active. And a path for
shared memory applications.
    
 A typical senerio would be:
    #include "ipcKeyDbm.h"
     IPC_KEY_DBM_ID keyId;
     IPC_KEY_DBM_DATA dbEntry;

   1. Create Database,  ipcKeyDbmCreate(...) 
        get a ipc Key dbm    
     keyId = ipcKeyDbmCreate("/usr24/greg/MsgQKeyDBM", 8);


   2. Update present Process Key,pid,etc. in database  
        Update Key, etc. used by Acqproc MsgQ    
     ipcKeySet(keyId, "Acqproc",getpid(), ACQPROC_MSGQ_KEY, NULL);

     ipcKeyDbmShow(keyId);     display entries   

   3. Get a Key of another Process ipcKeyDbmGet(.....)
        I want to talk to Sendprco on its MsgQ   
        get the info I need to know   
     ipcKeyGet(keyId, "Sendproc",&dbEntry);  
 
   4. Close Dbm
     ipcKeyDbmClose(keyId);


 It is envisioned that there would several databases of known file paths.
These databases would be for message Qs and Shared Memory. However this
could be extended for channel or socket IPC methods with a change
to the database entry structure.

*/



/**************************************************************
*
*  ipcKeyDbmCreate - Create and initialize IPC Key DataBase.
*
* This routine creates/opens a System V based IPC Key DataBase
* for the number of entries given.
* If the DataBase already exists it will NOT be initialized.
* The Actually IPC Key will be generated from the filename given 
* here and an integer key given in the ipcKeyDbmSet() call.
* See ftok().
*
* RETURNS:
* IPC DBM Id, or NULL if Error
*
*       Author Greg Brissey 7/14/94
*/
IPC_KEY_DBM_ID  ipcKeyDbmCreate(char* filename, int keyId, int dbmEntries )
/* key_t key - key to semaphore */
/* int dbmEntries - number of entries in dbm */
{
  int bytesize;
  IPC_KEY_DBM_ID keydbm;

 /* malloc space for IPC_KEY_DBM_ID object */
   if ( (keydbm = (IPC_KEY_DBM_ID) (malloc(sizeof(IPC_KEY_DBM_OBJ)))) == NULL )
   {
      errLogSysRet(ErrLogOp,debugInfo,"ipcKeyDbmCreate: IPC_KEY_DBM_OBJ malloc error\n");
      return((IPC_KEY_DBM_ID) NULL);
   }

   memset((void *)keydbm,0,sizeof(IPC_KEY_DBM_OBJ));

   if ( (keydbm->filepath = (char *) (malloc(strlen(filename) + 1))) == NULL )   
   {
     errLogSysRet(ErrLogOp,debugInfo,"ipcKeyDbmCreate: filename malloc error\n");
     free(keydbm);
     return((IPC_KEY_DBM_ID) NULL);
   }
   strcpy(keydbm->filepath,filename);


   keydbm->entries = dbmEntries;	  /* Number of Key Entries in DBM */

   bytesize = sizeof(IPC_KEY_DBM_DATA) * dbmEntries;

   if ( (keydbm->dbmdata = 
	 shrmCreate(filename,keyId,bytesize)) == NULL)
   {
      free(keydbm->filepath);
      free(keydbm);
      errLogRet(ErrLogOp,debugInfo,"ipcKeyDbmCreate: shrmCreate failed\n");
      return((IPC_KEY_DBM_ID) NULL);
   }

   if ( keydbm->dbmdata->shrmem->byteLen == 0 )	/* file doesn't already exits */
   {
      /* it is new so initialize it to zip */
      memset((void *)keydbm->dbmdata->shrmem->mapStrtAddr,0,bytesize);	
   }

   return(keydbm);
}
/**************************************************************
*
*  ipcKeyDbmClose - Close the IPC Key DataBase for this Process.
*
* This routine closes the shared Key DataBase for this process. The
* file of the shared Key DataBase is retained. Delete this file
* if it is not needed.
*
* RETURNS:
* VOID
*
*       Author Greg Brissey 7/14/94
*/
void ipcKeyDbmClose(IPC_KEY_DBM_ID dbmId)
/* IPC_KEY_DBM_ID dmbId - IPC Key DBM ID */
{
 
   if (dbmId != NULL)
   {
      shrmRelease(dbmId->dbmdata);  /* unmap shared memory */
      free(dbmId->filepath);
      free(dbmId);
   }
   return;
}   

/**************************************************************
*
*  ipcKeyGet - Get a DataBase entry based on the ID string
*
* This routine searches the database for the give search string 
* against the ID string. If found it copies the entry into
* the passed database entry structure (IPC_KEY_DBM_DATA) 
* pointer given.
*
* RETURNS:
*  0 if key match, -1 if not found;
*
*       Author Greg Brissey 7/14/94
*/
int ipcKeyGet(IPC_KEY_DBM_ID dbmId, char* idstr,IPC_KEY_DBM_DATA* entry)
/* dmbId - IPC Key DBM ID */
/* idstr - key string to search for in dbm */
/* entry - results will be copied into this struct ptr */
{
    int stat,i;
    IPC_KEY_DBM_DATA *key;

    if ((dbmId == NULL) || (entry == NULL))
    {
      errLogRet(ErrLogOp,debugInfo,"Dbm Object or Entry is a NULL Pointer\n");
      return(-1);
    }

    memset((void *)entry,0,sizeof(IPC_KEY_DBM_DATA));  /* clear given structure */

    key = (IPC_KEY_DBM_DATA *) dbmId->dbmdata->shrmem->mapStrtAddr;
    /* before searching update present entries pids as
        active or not.
    */
    for (i=0; i < dbmId->entries; i++)
    {
        if (key[i].pid != 0)
        {
            stat = kill(key[i].pid, 0); /* Hey, Are you out there ? */
            if ( (stat == -1) && (errno == ESRCH) )
                key[i].pidActive = 0;  /* Not active*/
            else
                key[i].pidActive = 1;	/* Active */
        }
    }

    if ( strlen(idstr) > (size_t) IPCKEY_MAXSTR_LEN )
    {
       errLogRet(ErrLogOp,debugInfo,
        "ipcKeyGet: Warning, search string '%s' (%d chars) is too long, maximum is %d\n",
	 idstr,strlen(idstr),IPCKEY_MAXSTR_LEN);
       errLogRet(ErrLogOp,debugInfo, "ipcKeyGet: The string will be truncated for search\n");
    }
    /* Now search List */
    /* if idstr is too long the strcmp only compares upto the NULL so strcmp is safe */
    for (i=0; i < dbmId->entries; i++)
    {
        if (strcmp(key[i].idstr,idstr) == 0)
        {
	    memcpy((void *)entry,(void *)&key[i],sizeof(IPC_KEY_DBM_DATA));
            return(0);
        }
    }
    return(-1);	/* entry not found */
}

/**************************************************************
*
*  ipcKeySet - Set a DataBase entry based on ID string
*
* This routine searches the database for the given string 
* against the ID string. If found it copies the new 
* parameters into the database entry. If search string is not 
* found then the 1st free database entry is used.
* If the key equal -1 then a randomly chosen key is provided.
*
* RETURNS:
*  0 if entry was updated, -1 if error occurred;
*
*       Author Greg Brissey 7/14/94
*/
int ipcKeySet(IPC_KEY_DBM_ID dbmId, char* idstr, int maxlen, int pid,int keyindex, char *filename)
/* IPC_KEY_DBM_ID dmbId - IPC Key DBM ID */
/* char * idstr - key string to search for in dbm */
/* int 	maxlen - Maximum Length of message to put on queue */
/* int 	pid - Pid of process */
/* int keyindex - Seed key used to calc IPC Key, */
/* char * filename - file path to share memory IPC */
{
   int free,stat,i,j,unique;
   IPC_KEY_DBM_DATA *key,k;
   key_t ipckey;
   key_t keyList[256];

   key = (IPC_KEY_DBM_DATA *) dbmId->dbmdata->shrmem->mapStrtAddr;
   /* before searching update present entries pids as
   active or not.
   */
    j = 0;
    for (i=0,free=-1; i < dbmId->entries; i++)
    {
        if (key[i].pid != 0)
        {
            stat = kill(key[i].pid, 0); /* Hey, Are you out there ? */
            if ( (stat == -1) && (errno == ESRCH) )
            {
                key[i].pidActive = 0;	/* No body Home */
            }
            else
            {
                key[i].pidActive = 1;  /* He's Here */
		keyList[j++] = key[i].ipcKey; /* make list of active ipckeys */
            }
        }
    }
    if ( strlen(idstr) > (size_t) IPCKEY_MAXSTR_LEN )
    {
       errLogRet(ErrLogOp,debugInfo,
        "ipcKeySet: Warning, key string '%s' (%d chars) is too long, maximum is %d\n",
	 idstr,strlen(idstr),IPCKEY_MAXSTR_LEN);
       errLogRet(ErrLogOp,debugInfo, "ipcKeySet: The string will be truncated for search\n");
    }
    /* Now search List for Non-Active Entry with idstr */
    for (i=0,free=-1; i < dbmId->entries; i++)
    {
        /* Keep track of 1st free entry */
        if ( (key[i].pidActive == 0) && (free == -1) )
            free = i;

        if (strcmp(key[i].idstr,idstr) == 0)
        {
            free = i;
	    break;
        }
    }

    if (free != -1)
    {
        int size,size2;

        size = sizeof(k.idstr);
        size2 = sizeof(k.path);

        /* obtain unique key for the msgQ */
        if (keyindex >= 0)
        {
           ipckey = ftok(dbmId->filepath,keyindex); 
        }
	else  /* find a unique key for the application */
	{
	   unique = 0;
           while( !unique )  
	   {
	      ipckey = ftok(dbmId->filepath,(rand()+100));
	      unique = 1;
              for(i=0; i < j; i++)  /* search list of active keys */
	      {
		if (keyList[i] == ipckey)  /* if match not unqiue try again */
		{
		   unique = 0;
	           break;
		}
	      }
           }
	}

        /* Lock down structure then update it */
        shrmTake(dbmId->dbmdata);
        strncpy(key[free].idstr,idstr,sizeof(k.idstr));
        key[free].idstr[size-1] = '\0';
        /* besure option filename is present before copy */
        if ( filename != NULL)
        {
          if ( strlen(filename) > (size_t) IPCKEY_MAXSTR_LEN )
          {
             errLogRet(ErrLogOp,debugInfo,
                 "ipcKeySet: Warning, filepath string '%s' (%d chars) is too long, maximum is %d\n",
	      filename,strlen(filename),IPCKEY_MAXSTR_LEN);
             errLogRet(ErrLogOp,debugInfo, "ipcKeySet: The string will be truncated\n");
          }
           strncpy(key[free].path,filename,sizeof(k.path));
           key[free].path[size2-1] = '\0';
        }
        else
	{
           key[free].path[0] = '\0';
	}
        key[free].pid = pid;
        key[free].pidActive = 1;
        key[free].ipcKey = ipckey;
        key[free].asyncFlag = 0;
        key[free].maxLenMsg = maxlen;
        shrmGive(dbmId->dbmdata);
        return(0);
    }
    else
    {
        return(-1);
    }
}

/**************************************************************
*
*  ipcKeyClear - Clear a DataBase entry based on ID string
*
* This routine searches the database for the given string 
* against the ID string. If found it clears the entry, set pid = 0
*
* RETURNS:
*  0 if entry was updated, -1 if error occurred;
*
*       Author Greg Brissey 7/14/94
*/
int ipcKeyClear(IPC_KEY_DBM_ID dbmId, char* idstr)
/* IPC_KEY_DBM_ID dmbId - IPC Key DBM ID */
/* char * idstr - key string to search for in dbm */
{
   int free,stat,i;
   IPC_KEY_DBM_DATA *key;

   key = (IPC_KEY_DBM_DATA *) dbmId->dbmdata->shrmem->mapStrtAddr;
   /* before searching update present entries pids as
   active or not.
   */
    for (i=0,free=-1; i < dbmId->entries; i++)
    {
        if (key[i].pid != 0)
        {
            stat = kill(key[i].pid, 0); /* Hey, Are you out there ? */
            if ( (stat == -1) && (errno == ESRCH) )
                key[i].pidActive = 0;  /* Not active*/
            else
                key[i].pidActive = 1;	/* Active */
        }
        else /* if pid == 0 then clear structure, house keeping chore */
        { 
           shrmTake(dbmId->dbmdata);
           memset((char*) &key[i], 0, sizeof(IPC_KEY_DBM_DATA));
           shrmGive(dbmId->dbmdata);
        }
    }

    /* Now search List for Entry with idstr */
    for (i=0,free=-1; i < dbmId->entries; i++)
    {
        if (strcmp(key[i].idstr,idstr) == 0)
        {
            free = i;
	    break;
        }
    }

    if (free != -1)
    {
        /* Lock down structure then update it */
        shrmTake(dbmId->dbmdata);
        memset((char*) &key[free], 0, sizeof(IPC_KEY_DBM_DATA));
        shrmGive(dbmId->dbmdata);
        return(0);
    }
    else
    {
        return(-1);
    }
}
/**************************************************************
*
*  ipcKeySetAsync - Set the async flag true for the  DataBase entry
*
* This routine searches the database for the given string 
* against the ID string. If found it updates the async flag 
* parameter to TRUE (1) in the database entry. If search string is not 
* found then -1 is returned.
*
* RETURNS:
*  0 if entry was updated, -1 if error occurred;
*
*       Author Greg Brissey 10/12/94
*/
int ipcKeySetAsync(IPC_KEY_DBM_ID dbmId, char* idstr)
/* IPC_KEY_DBM_ID dmbId - IPC Key DBM ID */
/* char * idstr - key string to search for in dbm */
{
   int free,stat,i;
   IPC_KEY_DBM_DATA *key;

   key = (IPC_KEY_DBM_DATA *) dbmId->dbmdata->shrmem->mapStrtAddr;
   /* before searching update present entries pids as
   active or not.
   */
    for (i=0,free=-1; i < dbmId->entries; i++)
    {
        if (key[i].pid != 0)
        {
            stat = kill(key[i].pid, 0); /* Hey, Are you out there ? */
            if ( (stat == -1) && (errno == ESRCH) )
                key[i].pidActive = 0;  /* Not active*/
            else
                key[i].pidActive = 1;	/* Active */
        }
    }
    /* Now search List for Non-Active Entry with idstr */
    for (i=0,free=-1; i < dbmId->entries; i++)
    {
        /* Keep track of 1st free entry */
        if ( (key[i].pidActive == 0) && (free == -1) )
            free = i;

        if (strcmp(key[i].idstr,idstr) == 0)
        {
            free = i;
	    break;
        }
    }

    if (free != -1)
    {
        /* Lock down structure then update it */
        shrmTake(dbmId->dbmdata);
        key[free].asyncFlag = 1;
        shrmGive(dbmId->dbmdata);
        return(0);
    }
    else
    {
        return(-1);
    }
}

/**************************************************************
*
*  ipcKeyDbmShow - show information about IPC Key Dbm 
*
* This routine display the system state information of
* the IPC Key Dbm .
*
* RETURNS:
* void
*
*       Author Greg Brissey 7/14/94
*/
void  ipcKeyDbmShow(IPC_KEY_DBM_ID dbmId)
/* IPC_KEY_DBM_ID dbmId - IPC Key Dbm Id */
{
#ifdef DEBUG
   int i;
   IPC_KEY_DBM_DATA *key;
   if (dbmId != NULL)
   {
      shrmShow(dbmId->dbmdata);
      DPRINT1(-1, "\n%d Key Entries in Dbm.\n",dbmId->entries);
      key = (IPC_KEY_DBM_DATA *) dbmId->dbmdata->shrmem->mapStrtAddr;
      for(i=0; i < dbmId->entries; i++)
       DPRINT7(-1,"%d - '%s', pid: %d, active: %d key: %ld (0x%lx), path: '%s'\n",
		i,key[i].idstr, key[i].pid, key[i].pidActive, 
		key[i].ipcKey, key[i].ipcKey, key[i].path);
   }
#endif
   return;
}
